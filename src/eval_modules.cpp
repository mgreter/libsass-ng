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

  static bool IsCompatibleConfig(ModRule* rule, WithConfig* b)
  {
    Stylesheet* sheet = rule->root47();
    WithConfig* a = rule;
    if (a == b) return true;
    if (a == nullptr) return false;
    if (b == nullptr) return false;
    // if (!a->hasConfig) return true;
    // if (!b->hasConfig) return true;
    // a = GetParentConfig(a);
    // b = GetParentConfig(b);
    // if (a == b) return true;

    // if (b == nullptr) return true;

    for (auto name : b->config) {
      // std::cerr << "Check for " << name.first.norm() << "\n";
      if (sheet->defaults.contains(name.first)) return false;
    }

    // for (auto upstream : sheet->upstream77)

    return true;
  }

  // Whether the configuration chain starting at [cfg] has an unused
  // value for [key]. Only `@forward` rules pass outer configs through.
  static bool HasConfigValue(WithConfig* cfg, EnvKey key)
  {
    while (cfg) {
      // Implicit configs of imports never cause this error
      if (!cfg->isForward && !cfg->hasConfig) return false;
      if (cfg->hasHideFilter && cfg->varFilters.count(key.norm())) return false;
      if (cfg->hasShowFilter && !cfg->varFilters.count(key.norm())) return false;
      auto it = cfg->config.find(key);
      if (it != cfg->config.end() && !it->second.wasAssigned) return true;
      if (!cfg->isForward) return false;
      if (!cfg->prefix.empty()) key = EnvKey(cfg->prefix + key.orig());
      cfg = cfg->parent;
    }
    return false;
  }

  // Port of dart-sass `couldHaveBeenConfigured`: whether any public
  // variable of [sheet] could be affected by the config at [cfg].
  static bool CouldHaveBeenConfigured(Stylesheet* sheet, WithConfig* cfg)
  {
    for (const auto& var : sheet->idxs->varIdxs) {
      if (var.first.isPrivate()) continue;
      if (HasConfigValue(cfg, var.first)) return true;
    }
    for (const auto& var : sheet->mergedFwdVar) {
      if (var.first.isPrivate()) continue;
      if (HasConfigValue(cfg, var.first)) return true;
    }
    return false;
  }

  // Port of the dart-sass check in `_execute` for modules that are already
  // loaded: an explicit configuration with another origin is an error if it
  // could have configured the module (e.g. passed through `@forward`).
  static bool ConflictsWithConfig(Stylesheet* sheet, WithConfig* cfg, size_t origin)
  {
    if (origin == 0 || origin == sheet->cfgOrigin) return false;
    return CouldHaveBeenConfigured(sheet, cfg);
  }

  // Port of dart-sass `toImplicitConfiguration` for `@import`: if the
  // imported file forwards modules, the variables visible when the import
  // starts configure their `!default` variables. The values are put into
  // the config of the import rule, which is the parent config of the
  // `@forward` rules in the imported file, and removed again afterwards.
  class ImplicitConfig
  {
    WithConfig* rule;
    EnvKeyMap<WithConfigVar> previous;
    bool active = false;
  public:
    ImplicitConfig(Compiler& compiler, WithConfig* rule,
      Stylesheet* sheet, const SourceSpan& pstate) :
      rule(rule)
    {
      // Only needed if the imported file has forwards
      bool hasForwards = false;
      for (const auto& statement : sheet->elements()) {
        if (dynamic_cast<ForwardRule*>(statement.ptr())) {
          hasForwards = true;
          break;
        }
      }
      if (!hasForwards) return;
      EnvKeyMap<WithConfigVar> config;
      // Inner scopes are visited first and take precedence
      auto add = [&](const EnvKey& name, const EnvRef& vidx) {
        if (config.count(name)) return;
        Value* value = compiler.varRoot.getVariable(vidx);
        if (value == nullptr || value->isaNull()) return;
        WithConfigVar& cfgvar = config[name];
        cfgvar.pstate = pstate;
        cfgvar.name = name.orig();
        cfgvar.value33 = value;
      };
      // Same visibility rules as `EnvRefs::findVarIdxs`
      for (const EnvRefs* current = compiler.envstack.back();
        current; current = current->nextScope())
      {
        for (auto it = current->forwards.rbegin();
          it != current->forwards.rend(); it++)
        {
          const EnvRefs* fwds = *it;
          for (const auto& var : fwds->varIdxs) {
            if (var.first.isPrivate()) continue;
            add(var.first, EnvRef(fwds, var.second));
          }
          if (Module* mod = fwds->module) {
            if (!mod->idxs->isImport) {
              for (const auto& var : mod->mergedFwdVar) {
                if (var.first.isPrivate()) continue;
                add(var.first, var.second);
              }
            }
          }
        }
        if (current->isImport) continue;
        for (const auto& var : current->varIdxs) {
          add(var.first, EnvRef(current, var.second));
        }
      }
      std::swap(previous, rule->config);
      std::swap(rule->config, config);
      active = true;
    }
    ~ImplicitConfig()
    {
      if (active) std::swap(rule->config, previous);
    }
  };

  static bool SameConfig(WithConfig* a, WithConfig* b)
  {
    if (a == b) return true;
    if (a == nullptr) return false;
    if (b == nullptr) return false;
    // if (!a->hasConfig) return true;
    // if (!b->hasConfig) return true;
    a = GetParentConfig(a);
    b = GetParentConfig(b);
    if (a == b) return true;
    return false;
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
      // Probably for "with" configs!?
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
      // A `with` creates a new configuration, otherwise
      // the current configuration is passed through.
      size_t origin = rule->hasConfig
        ? ++compiler.cfgSerial : cfgOrigin;
      if (!root->isCompiled)
      {
        root->idxs->through = rule->prefix;
        ImportStackFrame iframe(compiler, root->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || rule->hasConfig);
        RAII_PTR(WithConfig, wconfig, rule);
        RAII_SIZE(cfgOrigin, origin);
        root->cfgOrigin = origin;
        compileModule(root);
        exposeModule(root);
        rule->finalize(compiler);
      }
      else if (ConflictsWithConfig(root, rule, origin)) {
        throw Exception::ParserException(compiler,
          "This module was already loaded, so it "
          "can't be configured using \"with\".");
      }
      else if (compiler.hasWithConfig || rule->hasConfig) {
        if (!IsCompatibleConfig(rule, compiler.wconfig99)) {
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
      // Only a `with` creates a (new) configuration
      size_t origin = rule->hasConfig
        ? ++compiler.cfgSerial : 0;
      if (!root->isCompiled)
      {
        ImportStackFrame iframe(compiler, root->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || rule->hasConfig);
        RAII_PTR(WithConfig, wconfig, rule);
        RAII_SIZE(cfgOrigin, origin);
        root->cfgOrigin = origin;
        compileModule(root);
        exposeModule(root);
        rule->finalize(compiler);
      }
      else if (ConflictsWithConfig(root, rule, origin)) {
        throw Exception::ParserException(compiler,
          "This module was already loaded, so it "
          "can't be configured using \"with\".");
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

    if (pframe->isInternal53) {

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
    else {

      EnvRefs* cidxs = rule->root47()->idxs;

      // Expose variables of the nested import to this scope
      auto& imports = pframe->imports;
      if (std::find(imports.begin(), imports.end(), cidxs) == imports.end()) {
        imports.push_back(cidxs);
      }

      /*

      // Merge it up through all imports
      for (auto& fn : cidxs->fnIdxs) {
        if (pframe->fnIdxs.count(fn.first) == 0) {
          pframe->createFunction(fn.first, true);
        }
      }
      */

      // Merge it up through all imports
      for (auto& mix : cidxs->mixIdxs) {
        if (pframe->mixIdxs.count(mix.first) == 0) {
          pframe->createMixin(mix.first);
          if (pframe->mixOffset == NPOS) {
            pframe->mixOffset = 0;
          }
          else {
            pframe->mixOffset++;
          }
        }
      }

      // Merge it up through all imports
      for (auto& fn : cidxs->fnIdxs) {
        if (pframe->fnIdxs.count(fn.first) == 0) {
          pframe->createFunction(fn.first);
          if (pframe->fnOffset == NPOS) {
            pframe->fnOffset = 0;
          }
          else {
            pframe->fnOffset++;
          }
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

      // Like dart-sass, modules forwarded by the imported file are
      // configured by the variables visible when the import starts.
      // Must be taken before the import can modify any of them.
      ImplicitConfig implicit(compiler, rule, root, rule->pstate());

      CssRootObj css = SASS_MEMORY_NEW(CssRoot, root->pstate());
      {
        // Only triggers for very vew specs if unscoped
        {
          RAII_PTR(Stylesheet, _stylesheet, root);
          ImportStackFrame iframe(compiler, root->import);
          EnvScope envscope(compiler.varRoot, root->idxs);
          // Imports only pass implicit configurations
          RAII_SIZE(cfgOrigin, 0);
          {
            RAII_OBJ(CssParentNode, current, css);
            exposeImpRule1(rule); // May also be outside
          }
          for (auto& item : root->elements()) {
            RAII_OBJ(CssParentNode, current, css);
            ValueObj child = item->accept(this);
            // if (child) delete child;
            // debug_ast(child);
          }
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
            
      CssRootObj oldcomp = root->compiled;
      root->compiled = nullptr;
      CssRootObj rv = _combineCss(root, true);
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

  void Eval::importCssModule(String* url, Map* withMap, const SourceSpan& pstate)
  {

    bool hasWith = withMap && !withMap->empty();

    if (StringUtils::startsWith(url->value(), "sass:", 5)) {
      if (hasWith) throw Exception::RuntimeException(compiler,
        "Built-in module " + url->value() + " can't be configured.");
      return;
    }

    // Owned by the evaluator (see `loadCssConfigs`)
    loadCssConfigs.emplace_back(new WithConfig(wconfig,
      toWithConfig(withMap), hasWith));
    WithConfig& configs = *loadCssConfigs.back();
    // Only uses its own `$with` config
    configs.isolated = true;
    RAII_PTR(WithConfig, wconfig, &configs);

    sass::string prev(pstate.getAbsPath());
    if (Stylesheet* sheet = loadModule(
      prev, url->value(), false))
    {
      if (!sheet->isCompiled) {
        ImportStackFrame iframe(compiler, sheet->import);
        LocalOption<bool> haswith(compiler.hasWithConfig,
          compiler.hasWithConfig || hasWith);
        // Only a `$with` creates a (new) configuration
        size_t origin = hasWith ? ++compiler.cfgSerial : 0;
        RAII_SIZE(cfgOrigin, origin);
        sheet->cfgOrigin = origin;
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

    // Like dart-sass `_execute`, a module is evaluated with fresh style
    // state. With `meta.load-css()` inside a style rule, at-rule or nested
    // declaration, the caller's context must not leak into the module's
    // own CSS (e.g. the caller's style rule was copied into the module's
    // at-rules, which its extension store doesn't know about).
    RAII_PTR(CssStyleRule, readStyleRule, nullptr);
    RAII_OBJ(CssMediaQueryVector, mediaQueries, nullptr);
    LocalOption<sass::string> flag_declarationName(declarationName, "");
    RAII_FLAG(inUnknownAtRule, false);
    RAII_FLAG(atRootExcludingStyleRule, false);
    RAII_FLAG(inKeyframes, false);
    LocalOption<sass::vector<CssImportObj>> flag_outOfOrderImports(
      _outOfOrderImports, sass::vector<CssImportObj>());

    // Create container where to put compiled css
    root->compiled = SASS_MEMORY_NEW(CssRoot,
      root->pstate());

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

  void Eval::_extendModules(const sass::vector<StylesheetObj>& sortedModules)
  {

    std::unordered_map<sass::string, sass::vector<ExtensionStoreObj>> downstreamExtensionStores;

    /// Extensions that haven't yet been satisfied by some upstream module. This
    /// adds extensions when they're defined but not satisfied, and removes them
    /// when they're satisfied by any module.
    ExtSet unsatisfiedExtensions;

    for (Stylesheet* module : sortedModules) {

      // RAII_PTR(Stylesheet, _stylesheet, module);
      // RAII_PTR(ExtensionStore, _extensionStore, module->extender52);

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
        // std::cerr << "++ ADD EXTENSION " << downStreamIt->first << "\n";
        module->extender52->addExtensions(downStreamIt->second);
      }

      if (module->extender52->extensionsBySelector.empty()) {
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
