/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "eval.hpp"

#include "compiler.hpp"
#include "extension.hpp"
#include "exceptions.hpp"
#include "ast_imports.hpp"
#include "css_imported.hpp"

#include "debugger.hpp"

namespace Sass {
  
  /////////////////////////////////////////////////////////////////////////
  // Helpers for forward rules
  /////////////////////////////////////////////////////////////////////////

  template <typename T>
  static void exposeUnfiltered(
    T& merged, const T& expose,
    const sass::string& prefix,
    const sass::string& errprefix,
    Logger& logger,
    bool viaImport)
  {

    for (auto& idx : expose) {
      if (idx.first.isPrivate()) continue;
      EnvKey key(prefix + idx.first.orig());
      auto it = merged.find(key);
      if (it == merged.end()) {
        merged.insert({ key, idx.second });
      }
      else if (idx.second != it->second) {
        if (!viaImport)
          throw Exception::RuntimeException(logger,
            "Two forwarded modules both define a "
            + errprefix + key.norm() + ".");
      }
    }
  }

  template <typename T>
  static void exposeFiltered(
    T& merged, const T& expose,
    const sass::string& prefix,
    const std::set<EnvKey>& filters,
    const sass::string& errprefix,
    Logger& logger,
    bool show,
    bool viaImport)
  {
    for (auto& idx : expose) {
      if (idx.first.isPrivate()) continue;
      EnvKey key(prefix + idx.first.orig());
      if (show == (filters.count(key) == 1)) {
        auto it = merged.find(key);
        if (it == merged.end()) {
          merged.insert({ key, idx.second });
        }
        else if (idx.second != it->second) {
          if (!viaImport)
            throw Exception::RuntimeException(logger,
              "Two forwarded modules both define a "
              + errprefix + key.norm() + ".");
        }
      }
    }
  }

  static void mergeForwards(
    EnvRefs* idxs,
    Module* module,
    ModRule* wconfig,
    Logger& logger,
    bool viaImport)
  {

    // Only should happen if forward was found in root stylesheet
    // Doesn't make much sense as there is nowhere to forward to
    if (idxs->module != nullptr) {
      // This is needed to support double forwarding (ToDo - need filter, order?)
      for (auto& entry : idxs->module->mergedFwdVar) { module->mergedFwdVar.insert(entry); }
      for (auto& entry : idxs->module->mergedFwdMix) { module->mergedFwdMix.insert(entry); }
      for (auto& entry : idxs->module->mergedFwdFn) { module->mergedFwdFn.insert(entry); }
    }

    if (wconfig->hasShowFilter) {
      exposeFiltered(module->mergedFwdVar, idxs->varIdxs, wconfig->prefix, wconfig->varFilters, "variable named $", logger, true, viaImport);
      exposeFiltered(module->mergedFwdMix, idxs->mixIdxs, wconfig->prefix, wconfig->callFilters, "mixin named ", logger, true, viaImport);
      exposeFiltered(module->mergedFwdFn, idxs->fnIdxs, wconfig->prefix, wconfig->callFilters, "function named ", logger, true, viaImport);
    }
    else if (wconfig->hasHideFilter) {
      exposeFiltered(module->mergedFwdVar, idxs->varIdxs, wconfig->prefix, wconfig->varFilters, "variable named $", logger, false, viaImport);
      exposeFiltered(module->mergedFwdMix, idxs->mixIdxs, wconfig->prefix, wconfig->callFilters, "mixin named ", logger, false, viaImport);
      exposeFiltered(module->mergedFwdFn, idxs->fnIdxs, wconfig->prefix, wconfig->callFilters, "function named ", logger, false, viaImport);
    }
    else {
      exposeUnfiltered(module->mergedFwdVar, idxs->varIdxs, wconfig->prefix, "variable named $", logger, viaImport);
      exposeUnfiltered(module->mergedFwdMix, idxs->mixIdxs, wconfig->prefix, "mixin named ", logger, viaImport);
      exposeUnfiltered(module->mergedFwdFn, idxs->fnIdxs, wconfig->prefix, "function named ", logger, viaImport);
    }
  }

  /////////////////////////////////////////////////////////////////////////
  // Helpers to compare with configs
  /////////////////////////////////////////////////////////////////////////

  static WithConfig* GetParentConfig(WithConfig* cfg)
  {
    if (cfg == nullptr) return cfg;
    cfg = cfg->parent;
    if (cfg == nullptr) return cfg;
    while (cfg->parent && !cfg->hasConfig) {
      cfg = cfg->parent;
    }
    return cfg;
  }

  static bool SameConfig(WithConfig* a, WithConfig* b)
  {
    if (a == nullptr) return false;
    if (b == nullptr) return false;
    a = GetParentConfig(a);
    b = GetParentConfig(b);
    return a == b;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::exposeFwdRule(ForwardRule* rule) const
  {
    if (_stylesheet->idxs->isImport) {
      _stylesheet->idxs->delayedMerge.push_back(rule);
    }
    else {
      if (rule->wasExposed()) return;
      rule->wasExposed(true);
      mergeForwards(rule->module32()->idxs,
        _stylesheet, rule, compiler, viaImport);
    }
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  EnvRefs* Eval::pudding(EnvRefs* idxs, bool intoRoot, EnvRefs* modFrame)
  {

    if (intoRoot) {

      // Check if we push the same stuff twice
      for (auto& fwd : modFrame->forwards) {

        // if (idxs == fwd) continue;

        // Checked, needed
        for (auto& var : idxs->varIdxs) {
          auto it = fwd->varIdxs.find(var.first);
          if (it != fwd->varIdxs.end()) {
            if (var.second == it->second) continue;
            throw Exception::ParserException(compiler,
              "$" + var.first.norm() + " is available "
              "from multiple global modules.");
          }
        }
        // Checked, needed
        for (auto& var : idxs->mixIdxs) {
          auto it = fwd->mixIdxs.find(var.first);
          if (it != fwd->mixIdxs.end()) {
            if (var.second == it->second) continue;
            throw Exception::ParserException(compiler,
              "Mixin \"" + var.first.norm() + "(...)\" is "
              "available from multiple global modules.");
          }
        }
        // Checked, needed
        for (auto& var : idxs->fnIdxs) {
          auto it = fwd->fnIdxs.find(var.first);
          if (it != fwd->fnIdxs.end()) {
            if (var.second == it->second) continue;
            throw Exception::ParserException(compiler,
              "Function \"" + var.first.norm() + "(...)\" is "
              "available from multiple global modules.");
          }
        }
      }

    }
    else {

      // No idea why this is needed!
      for (auto& var : modFrame->varIdxs) {
        idxs->module->mergedFwdVar.insert(var);
      }
      //for (auto& var : modFrame->mixIdxs) {
      //  idxs->mixIdxs.insert(var);
      //}
      //for (auto var : modFrame->fnIdxs) {
      //  idxs->fnIdxs.insert(var);
      //}

    }


    return idxs;

  }

  void Eval::exposeUseRule(UseRule* rule)
  {

    if (rule->wasExposed()) return;
    rule->wasExposed(true);
    // if (!rule->module32()) return;

    EnvRefs* frame(compiler.getCurrentScope());
    // EnvRefs* mod(compiler.getCurrentModule());

    if (rule->module32()->isBuiltIn) {

      if (rule->ns().empty()) {
        _stylesheet->idxs->forwards.push_back(rule->module32()->idxs);
      }
      else {
        if (_stylesheet->modimps.count(rule->ns())) {
          throw Exception::ModuleAlreadyKnown(compiler, rule->ns());
        }
        else {
          _stylesheet->modimps.insert({ rule->ns(),
            { rule->module32()->idxs, nullptr } });
        }

        if (const auto& mod = frame->module) {
          mod->moduse.insert({ rule->ns(),
            { rule->module32()->idxs, nullptr } });
        }
      }

    }
    else if (rule->root47()) {

      pudding(rule->root47()->idxs, rule->ns().empty(), frame);

      if (rule->ns().empty() && !rule->module32()->idxs->isImport) {

        for (const auto& asd : rule->root47()->idxs->varIdxs) {

          auto it = _stylesheet->idxs->varIdxs.find(asd.first);
          if (it != _stylesheet->idxs->varIdxs.end()) {
            const auto& var = compiler.varRoot.getVariable(it->second);
            if (var != nullptr && !var->isNull())
              throw Exception::RuntimeException(logger,
                "This module and the new module both define a variable named \"$" + asd.first.norm() + "\".");
          }

        }
        _stylesheet->idxs->forwards.push_back(rule->root47()->idxs);
      }
      else {

        if (_stylesheet->modimps.count(rule->ns()) > 0) {
          throw Exception::ModuleAlreadyKnown(compiler, rule->ns());
        }

        _stylesheet->modimps[rule->ns()] =
        { rule->root47()->idxs, rule->root47() };

        if (const auto& mod = frame->module) {
          mod->moduse[rule->ns()] =
          { rule->root47()->idxs, rule->root47() };
        }
      }


    }
    else {

      throw "Invalid state!";

    }

  }

  Value* Eval::visitForwardRule(ForwardRule* rule)
  {
    // Add this forward rule to the stack trace
    CallStackFrame callframe(logger, {
      rule->pstate(), Strings::forwardRule });
    // Now load (and parse) the module (to eval)
    if (Stylesheet* root = loadModRule(rule))
    {
      if (!root->isCompiled)
      {
        root->idxs->through = rule->prefix;
        ImportStackFrame iframe(compiler, root->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || rule->hasConfig);
        RAII_PTR(WithConfig, wconfig, rule);
        compileModule(root);
        exposeModule(root);
        rule->finalize(compiler);
      }
      else if (compiler.hasWithConfig || rule->hasConfig) {
        if (!SameConfig(rule, compiler.wconfig99)) {
          throw Exception::ParserException(compiler,
            "This module was already loaded, so it "
            "can't be configured using \"with\".");
        }
      }
      _stylesheet->upstream77.push_back(root);
    }
    exposeFwdRule(rule);
    return nullptr;
  }

  Value* Eval::visitUseRule(UseRule* rule)
  {
    // Add this use rule to the stack trace
    CallStackFrame callframe(logger, {
      rule->pstate(), Strings::useRule });
    // Now load (and parse) the module (to eval)
    if (Stylesheet* root = loadModRule(rule))
    {
      if (!root->isCompiled)
      {
        ImportStackFrame iframe(compiler, root->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || rule->hasConfig);
        RAII_PTR(WithConfig, wconfig, rule);
        compileModule(root);
        exposeModule(root);
        rule->finalize(compiler);
      }
      else if (rule->hasConfig) {
        if (!SameConfig(rule, compiler.wconfig99)) {
          throw Exception::ParserException(compiler,
            "This module was already loaded, so it "
            "can't be configured using \"with\".");
        }
      }
      _stylesheet->upstream77.push_back(root);
    }
    exposeUseRule(rule);
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::exposeImpRule1(IncludeImport* rule) const
  {

    EnvRefs* pframe = compiler.getCurrentScope();

    while (pframe->isImport) {
      pframe = pframe->pscope;
    }

    if (pframe->isInternal) {

      EnvRefs* cidxs = rule->root47()->idxs;

      // Merge it up through all imports
      for (auto& var : cidxs->varIdxs) {
        if (pframe->varIdxs.count(var.first) == 0) {
          pframe->createVariable(var.first);
        }
      }
      // Merge it up through all imports
      for (auto& fn : cidxs->fnIdxs) {
        if (pframe->fnIdxs.count(fn.first) == 0) {
          pframe->createFunction(fn.first, true);
        }
      }

      // Merge it up through all imports
      for (auto& mix : cidxs->mixIdxs) {
        if (pframe->mixIdxs.count(mix.first) == 0) {
          pframe->createMixin(mix.first);
        }
      }
    }

  }
  // EO exposeImpRule


  // Import shares this environment's variables,
  // functions, and mixins, but not its modules.
  // This is the main entry point from eval visitor
  void Eval::acceptDynamicSassImport(IncludeImport* rule)
  {
    BackTrace trace(rule->pstate(), Strings::importRule);
    CallStackFrame cframe(logger, trace);
    if (Stylesheet* root = resolveIncludeImport(rule)) {

      CssRootObj css = SASS_MEMORY_NEW(CssRoot, root->pstate());
      {
        // Only triggers for very vew specs if unscoped
        EnvScope envscope(compiler.varRoot, root->idxs);
        ImportStackFrame iframe(compiler, root->import);
        RAII_PTR(Stylesheet, _stylesheet, root);
        RAII_OBJ(CssParentNode, current, css);
        exposeImpRule1(rule); // May also be outside
        for (auto& item : root->elements()) {
          ValueObj child = item->accept(this);
          // if (child) delete child;
        }
      }

      if (rule->module32()->idxs->delayedMerge.size() > 0) {
        for (auto qwe : rule->module32()->idxs->delayedMerge) {
          const auto& scope = compiler.getCurrentScope();
          scope->forwards.push_back(qwe->module32()->idxs);
          RAII_FLAG(viaImport, true);
          exposeFwdRule(qwe);
        }
      }

      CssParentNodeObj oldcomp = root->compiled;
      root->compiled = nullptr;
      CssRootObj rv = _combineCss(root);
      root->compiled = oldcomp;
      rv->accept(this);

      ImportedCssVisitor visitor(*this);
      for (auto& child : css->elements()) {
        child->accept(&visitor);
      }

    }
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::importCssModule(String* url, Map* withMap, SourceSpan pstate)
  {

    bool hasWith = withMap && !withMap->empty();

    if (StringUtils::startsWith(url->value(), "sass:", 5)) {
      if (hasWith) throw Exception::RuntimeException(compiler,
        "Built-in module " + url->value() + " can't be configured.");
      return;
    }

    WithConfig configs(wconfig,
      toWithConfig(withMap), hasWith);
    RAII_PTR(WithConfig, wconfig, &configs);

    sass::string prev(pstate.getAbsPath());
    if (Stylesheet* sheet = loadModule(
      prev, url->value(), false))
    {
      if (!sheet->isCompiled) {
        ImportStackFrame iframe(compiler, sheet->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || hasWith);
        RAII_FLAG(plainCss, true);
        compileModule(sheet);
        wconfig->finalize(compiler);
      }
      else if (compiler.hasWithConfig || hasWith) {
        ImportStackFrame iframe(compiler, sheet->import);
        throw Exception::ParserException(compiler,
          sass::string(sheet->pstate().getImpPath())
          + " was already loaded, so it "
          "can't be configured using \"with\".");
      }
      // debug_ast(sheet);
      CssRootObj rv = _combineCss(sheet, true);
      // debug_ast(rv);
      // debug_ast(current->parent());
      if (rv != nullptr) rv->accept(this);
//       debug_ast(current->parent());
    }

  }


  sass::vector<WithConfigVar> Eval::toWithConfig(Map* withMap) {

    bool hasWith = withMap && !withMap->empty();

    sass::vector<WithConfigVar> withConfigs;

    if (hasWith) {
      sass::flatmap::env<EnvKey, ValueObj> config;
      config.reserve(withMap->elements().size());
      for (auto& kv : withMap->elements()) {
        String* name = kv.first->assertString(compiler, "with key");
        EnvKey kname(name->value());
        WithConfigVar kvar;
        kvar.name = name->value();
        kvar.value33 = kv.second;
        kvar.isGuarded41 = false;
        kvar.wasAssigned = false;
        kvar.pstate = name->pstate();
        withConfigs.push_back(kvar);
        if (config.count(kname) == 1) {
          throw Exception::RuntimeException(compiler,
            "The variable $" + kname.norm() + " was configured twice.");
        }
        config[name->value()] = kv.second;
      }
    }

    return withConfigs;
  }


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::compileModule(Stylesheet* root)
  {

    // Put env scope onto the stack and activate it
    EnvScope envscope(compiler.varRoot, root->idxs);

    visitStylesheet(root);

  }

  void Eval::visitStylesheet(Stylesheet* root)
  {

    if (root->isCompiled) return;
    root->isCompiled = true;

    // Make sure to chroot all selectors
    RAII_SELECTOR(selectorStack, nullptr);
    RAII_SELECTOR(originalStack, nullptr);

    // Create container where to put compiled css
    root->compiled = SASS_MEMORY_NEW(CssStyleRule,
      root->pstate(), nullptr, new Box());

    root->compiled->fromPlainCss22(false);

    RAII_OBJ(CssParentNode, current, root->compiled);
    RAII_PTR(Stylesheet, _stylesheet, root);
    // Register the currently active extension store
    RAII_PTR(ExtensionStore, _extensionStore, root->extender52);

    // Local count for top imports
    RAII_SIZE(_endOfImports, 0);

    for (const auto& child : root->elements()) {
      ValueObj value = child->accept(this);
      // if (value) delete value;
    }

    root->compiled->elements().insert(
      root->compiled->begin() + _endOfImports,
      std::make_move_iterator(_outOfOrderImports.begin()),
      std::make_move_iterator(_outOfOrderImports.end()));
    _outOfOrderImports.clear(); // moved all, reset it

    root->determineTransitivelyContainsExtensions();

  }

  void Eval::exposeModule(Stylesheet* root)
  {

    for (auto& var : compiler.getCurrentModule()->varIdxs) {
      ValueObj& slot(compiler.varRoot.getModVar(var.second));
      if (slot == nullptr) slot = SASS_MEMORY_NEW(Null, root->pstate());
    }

    size_t i; // find amount of head comments
    for (i = 0; i < current->size(); i++) {
      CssComment* head = current->at(i)->isaCssComment();
      if (head == nullptr) break; // not a comment
      root->precomments.push_back(head);
      // std::cerr << "REGCOM " << root->import->getFileName() << " => [" << head->text() << "]\n";
    }
    // Remove leading comments from compiled module css
    // Consumers must ensure to add the comments back in
    current->erase(current->begin(), current->begin() + i);

    assert(current->size() == 0);

    current->clear();

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::_extendModules(sass::vector<Stylesheet*> sortedModules)
  {

    std::unordered_map<sass::string, sass::vector<ExtensionStoreObj>> downstreamExtensionStores;

    /// Extensions that haven't yet been satisfied by some upstream module. This
    /// adds extensions when they're defined but not satisfied, and removes them
    /// when they're satisfied by any module.
    ExtSet unsatisfiedExtensions;

    for (Stylesheet* module : sortedModules) {

      const sass::string & key(module->import->getAbsPath());

      // Create a snapshot of the simple selectors currently in the
      // [ExtensionStore] so that we don't consider an extension "satisfied"
      // below because of a simple selector added by another (sibling)
      // extension.
      ExtSmplSelSet originalSelectors; // getSimpleSelectors

      auto beg = module->extender52->selectors54.begin();
      auto end = module->extender52->selectors54.end();
      while (beg != end) {
#ifdef USE_SOME_MAP
        originalSelectors.insert(beg.key());
#else
        originalSelectors.insert(beg->first);
#endif
        beg++;
      }

      // for (const std::pair<SimpleSelectorObj, ExtListSelSet> sel : module->extender52->selectors54) {
      //   originalSelectors.insert(sel.first);
      // }

      module->extender52->addNonOriginalSelectors(
        originalSelectors, unsatisfiedExtensions);

      auto downStreamIt = downstreamExtensionStores.find(key);
      if (downStreamIt != downstreamExtensionStores.end()) {
        module->extender52->addExtensions(downStreamIt->second);
      }

      if (module->extender52->extensionsBySimpleSelector.empty()) {
        continue;
      }

      for (auto& upstream : module->upstream77) {
        if (upstream == nullptr) continue;
        const sass::string & url(upstream->import->getAbsPath());
        downstreamExtensionStores[url].push_back(module->extender52);
      }

      module->extender52->delNonOriginalSelectors(
        originalSelectors, unsatisfiedExtensions);
    }

    if (!unsatisfiedExtensions.empty()) {
      const Extension* extension = *unsatisfiedExtensions.begin();
      throw Exception::UnsatisfiedExtend(logger, extension);
    }

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
