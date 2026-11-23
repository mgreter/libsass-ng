/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "fn_meta.hpp"

#include "eval.hpp"
#include "compiler.hpp"
#include "exceptions.hpp"
#include "ast_values.hpp"
#include "ast_callables.hpp"
#include "ast_expressions.hpp"
#include "string_utils.hpp"
#include "stylesheet.hpp"

namespace Sass {

  namespace Functions {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    namespace Meta {

      /*******************************************************************/

      static BUILT_IN_FN(typeOf)
      {
        sass::string copy(arguments[0]->type());
        return SASS_MEMORY_NEW(String,
          pstate, std::move(copy));
      }

      /*******************************************************************/

      static BUILT_IN_FN(inspect)
      {
        if (arguments[0] == nullptr) {
          return SASS_MEMORY_NEW(
            String, pstate, "null");
        }
        // Micro-optimization (2%)
        // Note: will report different pstate
        // Note: but not exactly wrong either
        if (const String* str = arguments[0]->isaString()) {
          if (!str->hasQuotes()) return arguments[0].ptr();
        }
        // Otherwise we must create a copy
        return SASS_MEMORY_NEW(String,
          pstate, arguments[0]->inspect());
      }

      /*******************************************************************/

      static BUILT_IN_FN(fnIf)
      {
        // Always evaluates both sides!
        return arguments[0]->isTruthy() ?
          arguments[1] : arguments[2];
      }

      /*******************************************************************/

      static BUILT_IN_FN(fnCalcName)
      {
        Calculation* calculation = arguments[0]->assertCalculation(compiler, Sass::Strings::calc);
        return SASS_MEMORY_NEW(String, calculation->pstate(), calculation->name().c_str(), true);
      }

      /*******************************************************************/

      static BUILT_IN_FN(fnCalcArgs)
      {
        Calculation* calculation = arguments[0]->assertCalculation(compiler, Sass::Strings::calc);
        const sass::vector<AstNodeObj>& args(calculation->arguments());
        ValueVector values; // args.size()

        for (auto& arg : args) {
          if (auto* value = arg->isaValue()) {
            if (auto* calcop = value->isaCalcOperation()) {
              values.push_back(SASS_MEMORY_NEW(String,
                calcop->pstate(), calcop->toString(), false));
            }
            else {
              values.push_back(value);
            }
          }
          else {
            values.push_back(SASS_MEMORY_NEW(String,
              arg->pstate(), arg->toString(), false));
          }
        }

        return SASS_MEMORY_NEW(List, calculation->pstate(), std::move(values));
      }

      /*******************************************************************/

      static BUILT_IN_FN(keywords)
      {
        ArgumentList* argumentList = arguments[0]->assertArgumentList(compiler, Sass::Strings::args);
        const ValueFlatMap* keywords = argumentList->keywords();
        MapObj map = SASS_MEMORY_NEW(Map, arguments[0]->pstate());
        if (keywords) for (const auto& kv : *keywords) {
          sass::string key = kv.first.norm(); // .substr(1);
          // Util::ascii_normalize_underscore(key);
          // Wrap string key into a sass value
          map->insert(SASS_MEMORY_NEW(String,
            kv.second->pstate(), std::move(key)
          ), kv.second);
        }
        return map.detach();
      }

      /*******************************************************************/

      static BUILT_IN_FN(featureExists)
      {
        // Like dart-sass (1.78), the function is deprecated
        compiler.addDeprecation(pstate, Logger::WARN_FEATURE_EXISTS, []() {
          return "The feature-exists() function is deprecated.\n"
            "\nMore info: https://sass-lang.com/d/feature-exists";
        });
        const String* feature = arguments[0]->assertString(compiler, "feature");
        const sass::string& name(feature->value());
        return SASS_MEMORY_NEW(Boolean, pstate,
          name == "global-variable-shadowing" ||
          name == "extend-selector-pseudoclass" ||
          name == "units-level-3" ||
          name == "at-error" ||
          name == "custom-property");
      }

      /*******************************************************************/

      // A module passed to a meta function (see `getNullableModule`)
      struct ModuleArg {
        // The scope with the module's own members
        EnvRefs* refs = nullptr;
        // The loaded stylesheet (nullptr for built-ins)
        Stylesheet* sheet = nullptr;
        // The module itself (stylesheet or built-in)
        Module* module = nullptr;
      };

      // Like dart-sass `_getNullableModule`, resolves a module reference or
      // the namespace of a module used by the current module. Returns false
      // for `null`, otherwise throws if [value] can't be resolved.
      static bool getNullableModule(Compiler& compiler, Value* value, ModuleArg& arg)
      {
        if (ModuleRef* ref = value->isaModuleRef()) {
          arg.refs = ref->refs();
          arg.sheet = ref->sheet();
          arg.module = ref->module();
          return true;
        }
        if (String* ns = value->isaString()) {
          auto parent = compiler.getCurrentModule();
          auto& moduse = parent->module->moduse;
          auto it = moduse.find(ns->value());
          // A module is not available before it was compiled
          if (it != moduse.end() && (it->second.second == nullptr
            || it->second.second->isCompiled))
          {
            arg.refs = it->second.first;
            // Only stylesheets are registered with a module
            arg.sheet = static_cast<Stylesheet*>(it->second.second);
            arg.module = arg.sheet;
            // Built-in modules are only registered by their scope
            if (arg.module == nullptr) {
              for (auto& builtin : compiler.modules64) {
                if (builtin.second->idxs == arg.refs) {
                  arg.module = builtin.second;
                  break;
                }
              }
            }
            return true;
          }
          throw Exception::RuntimeException(compiler,
            "There is no module with namespace \"" + ns->value() + "\".");
        }
        if (value->isNull()) return false;
        CallStackFrame csf(compiler, value->pstate());
        throw Exception::SassScriptException(value->toString() +
          " is neither a string nor a module reference.",
          compiler, value->pstate(), Strings::module);
      }

      // Like dart-sass `_getModule`, but `null` is not allowed
      static ModuleArg getModule(Compiler& compiler, Value* value)
      {
        ModuleArg arg;
        if (!getNullableModule(compiler, value, arg)) {
          CallStackFrame csf(compiler, value->pstate());
          throw Exception::SassScriptException(value->toString() +
            " is neither a string nor a module reference.",
            compiler, value->pstate(), Strings::module);
        }
        return arg;
      }

      /*******************************************************************/

      static BUILT_IN_FN(globalVariableExists)
      {
        String* variable = arguments[0]->assertString(compiler, Sass::Strings::name);
        auto parent = compiler.getCurrentModule();
        EnvKey name(variable->value());
        ModuleArg plugin;
        if (getNullableModule(compiler, arguments[1], plugin)) {
          return SASS_MEMORY_NEW(Boolean, pstate,
            plugin.refs->varIdxs.count(name) != 0);
        }
        bool hasVar = false;
        for (auto& global : parent->forwards) {
          if (global->varIdxs.count(name) != 0) {
            if (hasVar) {
              throw Exception::RuntimeException(compiler,
                "This variable is available from multiple global modules.");
            }
            hasVar = true;
          }
        }
        if (hasVar) return SASS_MEMORY_NEW(Boolean, pstate, true);
        EnvRef vidx = compiler.varRoot.findVarIdx(name, "", true);
        if (!vidx.isValid()) return SASS_MEMORY_NEW(Boolean, pstate, false);
        auto& var = compiler.varRoot.getVariable(vidx);
        return SASS_MEMORY_NEW(Boolean, pstate, !var.isNull());

      }

      /*******************************************************************/

      static BUILT_IN_FN(variableExists)
      {
        String* variable = arguments[0]->assertString(compiler, Sass::Strings::name);
        EnvKey name(variable->value());
        EnvRef vidx = compiler.varRoot.findVarIdx(name, "");

        bool hasVar = false;
        auto parent = compiler.getCurrentModule();
        for (auto& global : parent->forwards) {
          if (global->varIdxs.count(name) != 0) {
            if (hasVar) {
              throw Exception::RuntimeException(compiler,
                "This variable is available from multiple global modules.");
            }
            hasVar = true;
          }
        }
        if (hasVar) return SASS_MEMORY_NEW(Boolean, pstate, true);
        if (!vidx.isValid()) return SASS_MEMORY_NEW(Boolean, pstate, false);
        auto& var = compiler.varRoot.getVariable(vidx);
        return SASS_MEMORY_NEW(Boolean, pstate, !var.isNull());
      }

      /*******************************************************************/

      static BUILT_IN_FN(functionExists)
      {
        String* variable = arguments[0]->assertString(compiler, Sass::Strings::name);
        auto parent = compiler.getCurrentModule();
        EnvKey name(variable->value());
        ModuleArg plugin;
        if (getNullableModule(compiler, arguments[1], plugin)) {
          return SASS_MEMORY_NEW(Boolean, pstate,
            plugin.refs->fnIdxs.count(name) != 0);
        }
        bool hasFn = false;
        for (auto& global : parent->forwards) {
          if (global->fnIdxs.count(name) != 0) {
            if (hasFn) {
              throw Exception::RuntimeException(compiler,
                "This function is available from multiple global modules.");
            }
            hasFn = true;
          }
        }
        if (hasFn) return SASS_MEMORY_NEW(Boolean, pstate, true);
        EnvRef fidx = compiler.varRoot.findFnIdx(name, "");
        return SASS_MEMORY_NEW(Boolean, pstate, fidx.isValid());
      }

      /*******************************************************************/

      static BUILT_IN_FN(mixinExists)
      {
        String* variable = arguments[0]->assertString(compiler, Sass::Strings::name);
        EnvKey name(variable->value());

        auto parent = compiler.getCurrentModule();
        ModuleArg plugin;
        if (getNullableModule(compiler, arguments[1], plugin)) {
          return SASS_MEMORY_NEW(Boolean, pstate,
            plugin.refs->mixIdxs.count(name) != 0);
        }
        bool hasFn = false;
        for (auto& global : parent->forwards) {
          if (global->mixIdxs.count(name) != 0) {
            if (hasFn) {
              throw Exception::RuntimeException(compiler,
                "This mixin is available from multiple global modules.");
            }
            hasFn = true;
          }
        }
        if (hasFn) return SASS_MEMORY_NEW(Boolean, pstate, true);

        auto midx = compiler.varRoot.findMixIdx(name, "");
        return SASS_MEMORY_NEW(Boolean, pstate, midx.isValid());
      }


      /*******************************************************************/

      static BUILT_IN_FN(fnApply)
      {
        // auto mixin = ;
        Callable* callable = arguments[0]->assertMixin(compiler, Sass::Strings::mixin)->callable();
        auto arglist = arguments[1]->assertArgumentList(compiler, Sass::Strings::amount);

        if (auto mixin = callable->isaUserDefinedCallable()) {

          // An include expression must reference a mixin rule
          MixinRule* rule = mixin->declaration()->isaMixinRule();
          // Sanity assertion
          if (rule == nullptr) {
            throw Exception::RuntimeException(eval.logger,
              "Include doesn't reference a mixin!");
          }
          // Create new mixin for content block
          // Prepares the content block to be called later
          // Content blocks of includes are like mixins themselves
          CallableDeclaration* ctblk = nullptr;
          if (eval.content) ctblk = eval.content->declaration();
          CallableArgumentsObj args = SASS_MEMORY_NEW(CallableArguments, pstate,
            {}, {}, SASS_MEMORY_NEW(ValueExpression, callable->pstate(), arglist));
          eval.applyMixin(pstate, mixin->name(), mixin, ctblk, args);
          return SASS_MEMORY_NEW(Boolean, pstate, false);
        }
        else if (auto bc = callable->isaBuiltInCallable()) {

          // An include expression must reference a mixin rule
          // MixinRule* rule = bc->isaMixinRule();
          // Sanity assertion
          // if (rule == nullptr) {
          //   throw Exception::RuntimeException(eval.traces,
          //     "Include doesn't reference a mixin!");
          // }

          CallableDeclaration* ctblk = nullptr;
          if (eval.content) ctblk = eval.content->declaration();
          CallableArgumentsObj args = SASS_MEMORY_NEW(CallableArguments, pstate,
            {}, {}, SASS_MEMORY_NEW(ValueExpression, callable->pstate(), arglist));
          eval.applyMixin(pstate, bc->name(), bc, ctblk, args);
          return SASS_MEMORY_NEW(Boolean, pstate, false);
        }
        
        return SASS_MEMORY_NEW(Boolean, pstate, false);
      }

      /*******************************************************************/
      /*
      static BUILT_IN_FN(fnGetMixin)
      {
        String* name = arguments[0]->assertString(compiler, Sass::Strings::name);
        String* plugin = arguments[1]->assertStringOrNull(compiler, Sass::Strings::module);

        auto parent = compiler.getCurrentModule();
        if (plugin != nullptr) {
          auto pp = parent->module->moduse.find(plugin->value());
          if (pp != parent->module->moduse.end()) {
            EnvRefs* module = pp->second.first;
            auto it = module->mixIdxs.find(name->value());
            if (it != module->mixIdxs.end()) {
              return SASS_MEMORY_NEW(Mixin, pstate,
                compiler.varRoot.getMixin(it->second));
            }
          }
          else {
            throw Exception::RuntimeException(compiler,
              "There is no module with namespace \"" + plugin->value() + "\".");
          }
          return SASS_MEMORY_NEW(Boolean, pstate, false);
        }
        bool hasFn = false;
        for (auto& global : parent->forwards) {
          if (global->mixIdxs.count(name->value()) != 0) {
            if (hasFn) {
              throw Exception::RuntimeException(compiler,
                "This function is available from multiple global modules.");
            }
            hasFn = true;
          }
        }

        auto midx = compiler.varRoot.findMixIdx(name->value(), "");
        if (midx.isValid()) {
          const auto& callable = compiler.varRoot.getMixin(midx);
          return SASS_MEMORY_NEW(Mixin, pstate, callable);
        }
        else {
          throw Exception::RuntimeException(compiler,
            "Mixin not found: " + name->inspect());
        }

        // auto name = arguments[0]->assertString(compiler, "name");
        // //auto modul = arguments[1]->realNull ? .assertString("module");
        // 
        // envi
        // 
        //   // Always evaluates both sides!
        //   return arguments[0]->isTruthy() ?
        //   arguments[1] : arguments[0];
      }
      */

      /*******************************************************************/

      static BUILT_IN_FN(fnAcceptsContent)
      {
        Mixin* mixin = arguments[0]->assertMixin(compiler, Sass::Strings::mixin);
        if (const auto& callable = mixin->callable())
        {
          if (BuiltInCallable* builtin = callable->isaBuiltInCallable()) {
            return SASS_MEMORY_NEW(Boolean, pstate, builtin->acceptsContent());
          }
          else if (ExternalCallable* mixfn = callable->isaExternalCallable()) {
            if (/* CallableSignature* decl = */ mixfn->declaration()) {
              return SASS_MEMORY_NEW(Boolean, pstate, false);
            }
          }
          else if (UserDefinedCallable* mixfn = callable->isaUserDefinedCallable()) {
            if (CallableDeclaration* decl = mixfn->declaration()) {
              return SASS_MEMORY_NEW(Boolean, pstate, decl->hasContent());
            }
          }
          else {
            throw Exception::RuntimeException(compiler,
              "Unknown callable type " + mixin->type() + ".");
          }
        }
        else {
          throw Exception::RuntimeException(compiler,
            "Mixin has no callable associated.");
        }
        throw Exception::RuntimeException(compiler,
          "Mixin33 has no callable associated.");
      }

      /*******************************************************************/

      static BUILT_IN_FN(contentExists)
      {
        if (!eval.isInMixin()) {
          throw Exception::RuntimeException(compiler,
            "content-exists() may only be called within a mixin.");
        }
        return SASS_MEMORY_NEW(Boolean, pstate,
          eval.hasContentBlock());
      }

      /*******************************************************************/

      static BUILT_IN_FN(moduleVariables)
      {
        ModuleArg arg(getModule(compiler, arguments[0]));
        MapObj list = SASS_MEMORY_NEW(Map, pstate);
        {
          EnvRefs* refs = arg.refs;
          Module* root = arg.sheet;
          for (auto& entry : refs->varIdxs) {
            String* name = SASS_MEMORY_NEW(String, pstate,
              sass::string(entry.first.norm()), true);
            EnvRef vidx(refs, entry.second);
            list->insert({ name, compiler.
              varRoot.getVariable(vidx) });
          }
          if (root)
          for (auto& entry : root->mergedFwdVar) {
            String* name = SASS_MEMORY_NEW(String, pstate,
              sass::string(entry.first.norm()), true);
            EnvRef vidx(entry.second);
            list->insert({ name, compiler.
              varRoot.getVariable(vidx) });
          }
        }
        return list.detach();
      }

      /*******************************************************************/

      static BUILT_IN_FN(fnModuleMixins)
      {
        ModuleArg arg(getModule(compiler, arguments[0]));
        MapObj list = SASS_MEMORY_NEW(Map, pstate);
        {
          EnvRefs* refs = arg.refs;
          Module* root = arg.sheet;
          for (const auto& entry : refs->mixIdxs) {
            String* name = SASS_MEMORY_NEW(String, pstate,
              sass::string(entry.first.norm()), true);
            EnvRef fidx(refs, entry.second);
            const auto& callable = compiler.varRoot.getMixin(fidx);
            Mixin* fn = SASS_MEMORY_NEW(Mixin, pstate, callable);
            list->insert({ name, fn });
          }
          if (root)
            for (const auto& entry : root->mergedFwdMix) {
              String* name = SASS_MEMORY_NEW(String, pstate,
                sass::string(entry.first.norm()), true);
              EnvRef fidx(entry.second);
              const auto& callable = compiler.varRoot.getMixin(fidx);
              Mixin* fn = SASS_MEMORY_NEW(Mixin, pstate, callable);
              list->insert({ name, fn });
            }
        }
        return list.detach();
      }

      static BUILT_IN_FN(moduleFunctions)
      {
        ModuleArg arg(getModule(compiler, arguments[0]));
        MapObj list = SASS_MEMORY_NEW(Map, pstate);
        {
          EnvRefs* refs = arg.refs;
          Module* root = arg.sheet;
          for (const auto& entry : refs->fnIdxs) {
            String* name = SASS_MEMORY_NEW(String, pstate,
              sass::string(entry.first.norm()), true);
            EnvRef fidx(refs, entry.second);
            const auto& callable = compiler.varRoot.getFunction(fidx);
            Function* fn = SASS_MEMORY_NEW(Function, pstate, callable);
            list->insert({ name, fn });
          }
          if (root)
          for (const auto& entry : root->mergedFwdFn) {
            String* name = SASS_MEMORY_NEW(String, pstate,
              sass::string(entry.first.norm()), true);
            EnvRef fidx(entry.second);
            const auto& callable = compiler.varRoot.getFunction(fidx);
            Function* fn = SASS_MEMORY_NEW(Function, pstate, callable);
            list->insert({ name, fn });
          }
        }
        return list.detach();
      }

      /*******************************************************************/

      /// Like `_environment.findFunction`, but also returns built-in
      /// globally-available functions.
      static Callable* _getFunction(const EnvKey& name, Compiler& ctx, const sass::string& ns = "") {
        EnvRef fidx = ctx.varRoot.findFnIdx(name, "");
        if (!fidx.isValid()) return nullptr;
        return ctx.varRoot.getFunction(fidx);
      }

      /// Like `_environment.findFunction`, but also returns built-in
      /// globally-available functions.
      static Callable* _getMixin(const EnvKey& name, Compiler& ctx, const sass::string& ns = "") {
        EnvRef fidx = ctx.varRoot.findMixIdx(name, "");
        if (!fidx.isValid()) return nullptr;
        return ctx.varRoot.getMixin(fidx);
      }

      static BUILT_IN_FN(findFunction)
      {

        String* name = arguments[0]->assertString(compiler, Sass::Strings::name);
        bool css = arguments[1]->isTruthy(); // supports all values
        ModuleArg plugin;
        bool hasModule = getNullableModule(compiler, arguments[2], plugin);

        if (css && hasModule) {
          throw Exception::RuntimeException(compiler,
            "$css and $module may not both be passed at once.");
        }

        EnvKey varkey(name->value());

        if (css) {
          return SASS_MEMORY_NEW(Function, pstate, name->value());
        }

        CallableObj callable;

        auto parent = compiler.getCurrentModule();

        if (hasModule) {
          EnvRefs* module = plugin.refs;
          auto it = module->fnIdxs.find(varkey);
          if (it != module->fnIdxs.end()) {
            EnvRef fidx({ module, it->second });
            callable = compiler.varRoot.getFunction(fidx);
          }
        }
        else {

          callable = _getFunction(varkey, compiler);

          if (!callable) {

            for (auto global : parent->forwards) {
              auto it = global->fnIdxs.find(varkey);
              if (it != global->fnIdxs.end()) {
                if (callable) {
                  throw Exception::RuntimeException(compiler,
                    "This function is available from multiple global modules.");
                }
                EnvRef fidx({ global, it->second });
                callable = compiler.varRoot.getFunction(fidx);
                if (callable) break;
              }
            }
          }
        }


        if (callable == nullptr) {
          // Like dart-sass, the name is inspected (quotes and escapes)
          throw
            Exception::RuntimeException(compiler,
              "Function not found: " + name->inspect());
        }

        // Remember if it was looked up without a namespace, so
        // `call()` behaves like a direct call (see `Function`).
        bool unqualified = !hasModule;
        return SASS_MEMORY_NEW(Function, pstate, callable, unqualified);

      }


      static BUILT_IN_FN(findMixin)
      {

        String* name = arguments[0]->assertString(compiler, Sass::Strings::name);
        ModuleArg plugin;
        bool hasModule = getNullableModule(compiler, arguments[1], plugin);

        CallableObj callable;
        EnvKey varkey(name->value());

        auto parent = compiler.getCurrentModule();

        if (hasModule) {
          EnvRefs* module = plugin.refs;
          auto it = module->mixIdxs.find(varkey);
          if (it != module->mixIdxs.end()) {
            EnvRef fidx({ module, it->second });
            callable = compiler.varRoot.getMixin(fidx);
          }
        }
        else {

          callable = _getMixin(varkey, compiler);

          if (!callable) {

            for (auto global : parent->forwards) {
              auto it = global->mixIdxs.find(varkey);
              if (it != global->mixIdxs.end()) {
                if (callable) {
                  throw Exception::RuntimeException(compiler,
                    "This mixin is available from multiple global modules.");
                }
                EnvRef fidx({ global, it->second });
                callable = compiler.varRoot.getMixin(fidx);
                if (callable) break;
              }
            }
          }
        }

        if (callable == nullptr) {
          // Like dart-sass, the name is inspected (quotes and escapes)
          throw
            Exception::RuntimeException(compiler,
              "Mixin not found: " + name->inspect());
        }

        return SASS_MEMORY_NEW(Mixin, pstate, callable);

      }

      /*******************************************************************/

      static BUILT_IN_FN(call)
      {

        Value* function = arguments[0]->assertValue(compiler, "function");
        ArgumentList* args = arguments[1]->assertArgumentList(compiler, Sass::Strings::args);

        // ExpressionVector positional,
        //   EnvKeyMap<ExpressionObj> named,
        //   Expression* restArgs = nullptr,
        //   Expression* kwdRest = nullptr);

        ValueExpression* restArg = SASS_MEMORY_NEW(
          ValueExpression, args->pstate(), args);

        ValueExpression* kwdRest = nullptr;
        if (args->keywords() && !args->keywords()->empty()) {
          Map* map = args->keywordsAsSassMap();
          kwdRest = SASS_MEMORY_NEW(
            ValueExpression, map->pstate(), map);
        }

        CallableArgumentsObj invocation = SASS_MEMORY_NEW(
          CallableArguments, pstate, ExpressionVector{}, {}, restArg, kwdRest);

        if (String * str = function->isaString()) {
          compiler.addDeprecation(str->pstate(),
            Logger::WARN_STRING_CALL, [str]() {
              return "Passing a string to call() is deprecated and will be illegal in LibSass 5.0.0.\n"
                "\nRecommendation: call(get-function(" + str->inspect() + "))";
            });
          InterpolationObj itpl = SASS_MEMORY_NEW(Interpolation, pstate);
          itpl->append(SASS_MEMORY_NEW(String, pstate, sass::string(str->value())));
          FunctionExpressionObj expression = SASS_MEMORY_NEW(
            FunctionExpression, pstate, str->value(), invocation);
          return eval.acceptFunctionExpression(expression);
        }

        Function* fn = function->assertFunction(compiler, "function");
        if (fn->cssName().empty()) {
          if (fn->callable() == nullptr) return nullptr;
          return fn->callable()->execute(eval, invocation, pstate, fn->global());
        }
        else {
          sass::string strm;
          strm += fn->cssName();
          eval.renderArgumentInvocation(strm, invocation);
          return SASS_MEMORY_NEW(
            String, fn->pstate(),
            std::move(strm));
        }


      }

      /*******************************************************************/

      static BUILT_IN_FN(loadCss)
      {
        String* url = arguments[0]->assertStringOrNull(compiler, Strings::url);
        MapObj withMap = arguments[1]->assertMapOrNull(compiler, Strings::with);
        eval.importCssModule(url, withMap, pstate);
        return SASS_MEMORY_NEW(Null, SourceSpan::internal32("[LOADCSS]"));
      }

      /*******************************************************************/

      // Returns a module reference for a module or its namespace
      static BUILT_IN_FN(fnGetModule)
      {
        ModuleArg arg(getModule(compiler, arguments[0]));
        return SASS_MEMORY_NEW(ModuleRef, pstate,
          arg.refs, arg.sheet, arg.module);
      }

      /*******************************************************************/

      // Loads a module like `meta.load-css()`, but returns a
      // module reference instead of emitting its CSS.
      static BUILT_IN_FN(fnLoad)
      {
        String* url = arguments[0]->assertString(compiler, Strings::url);
        MapObj withMap = arguments[1]->assertMapOrNull(compiler, Strings::with);
        BuiltInMod* builtIn = nullptr;
        if (Stylesheet* sheet = eval.loadModuleForSassScript(
          url, withMap, pstate, &builtIn))
        {
          return SASS_MEMORY_NEW(ModuleRef, pstate,
            sheet->idxs, sheet, sheet);
        }
        return SASS_MEMORY_NEW(ModuleRef, pstate,
          builtIn->idxs, nullptr, builtIn);
      }

      /*******************************************************************/

      // Emits the CSS of a module (again), like `meta.load-css()`
      static BUILT_IN_FN(fnCss)
      {
        ModuleArg arg(getModule(compiler, arguments[0]));
        if (arg.sheet != nullptr) eval.emitModuleCss(arg.sheet);
        return SASS_MEMORY_NEW(Null, SourceSpan::internal32("[CSS]"));
      }

      /*******************************************************************/

      void registerFunctions(Compiler& compiler)
	    {

        BuiltInMod& module(compiler.createModule("meta"));

        compiler.registerBuiltInFunction(key_if, "$condition, $if-true, $if-false", fnIf);

        module.addMixin(key_apply, compiler.createBuiltInMixin(key_apply, "$mixin, $args...", fnApply, true));
        module.addMixin(key_load_css, compiler.createBuiltInMixin(key_load_css, "$url, $with: null", loadCss));
        module.addMixin(key_css, compiler.createBuiltInMixin(key_css, "$module", fnCss));

        module.addFunction(key_calc_name, compiler.registerBuiltInFunction(key_calc_name, "$calc", fnCalcName));
        module.addFunction(key_calc_args, compiler.registerBuiltInFunction(key_calc_args, "$calc", fnCalcArgs));
        module.addFunction(key_get_mixin, compiler.registerBuiltInFunction(key_get_mixin, "$name, $module: null", findMixin));
        module.addFunction(key_module_mixins, compiler.registerBuiltInFunction(key_module_mixins, "$module", fnModuleMixins));
        module.addFunction(key_accepts_content, compiler.registerBuiltInFunction(key_accepts_content, "$mixin", fnAcceptsContent));

        module.addFunction(key_feature_exists, compiler.registerBuiltInFunction(key_feature_exists, "$feature", featureExists));
        module.addFunction(key_type_of, compiler.registerBuiltInFunction(key_type_of, "$value", typeOf));
        module.addFunction(key_inspect, compiler.registerBuiltInFunction(key_inspect, "$value", inspect));
        module.addFunction(key_keywords, compiler.registerBuiltInFunction(key_keywords, "$args", keywords));
        module.addFunction(key_global_variable_exists, compiler.registerBuiltInFunction(key_global_variable_exists, "$name, $module: null", globalVariableExists));
        module.addFunction(key_variable_exists, compiler.registerBuiltInFunction(key_variable_exists, "$name", variableExists));
        module.addFunction(key_function_exists, compiler.registerBuiltInFunction(key_function_exists, "$name, $module: null", functionExists));
        module.addFunction(key_mixin_exists, compiler.registerBuiltInFunction(key_mixin_exists, "$name, $module: null", mixinExists));
        module.addFunction(key_content_exists, compiler.registerBuiltInFunction(key_content_exists, "", contentExists));
        module.addFunction(key_module_variables, compiler.createBuiltInFunction(key_module_variables, "$module", moduleVariables));
        module.addFunction(key_module_functions, compiler.registerBuiltInFunction(key_module_functions, "$module", moduleFunctions));
        module.addFunction(key_get_function, compiler.registerBuiltInFunction(key_get_function, "$name, $css: false, $module: null", findFunction));
        module.addFunction(key_call, compiler.registerBuiltInFunction(key_call, "$function, $args...", call));
        module.addFunction(key_get_module, compiler.registerBuiltInFunction(key_get_module, "$module", fnGetModule));
        module.addFunction(key_load, compiler.registerBuiltInFunction(key_load, "$url, $with: null", fnLoad));

	    }

      /*******************************************************************/

    }

  }

}
