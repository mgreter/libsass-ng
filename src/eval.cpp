/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "eval.hpp"

#include "cssize.hpp"
#include "sources.hpp"
#include "compiler.hpp"
#include "stylesheet.hpp"
#include "exceptions.hpp"
#include "ast_values.hpp"
#include "ast_imports.hpp"
#include "ast_selectors.hpp"
#include "ast_callables.hpp"
#include "ast_statements.hpp"
#include "ast_expressions.hpp"
#include "parser_selector.hpp"
#include "parser_media_query.hpp"
#include "parser_keyframe_selector.hpp"

#include "character.hpp"
#include "calculation.hpp"
#include "calc_names.hpp"
#include <limits>

#include "environment.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Eval::Eval(Compiler& compiler, Logger& logger, bool plainCss) :
    logger(logger),
    compiler(compiler),
    wconfig(compiler.wconfig99),
    plainCss(plainCss)
  {
    bool_true = SASS_MEMORY_NEW(Boolean, SourceSpan::internal32("[TRUE]"), true);
    bool_false = SASS_MEMORY_NEW(Boolean, SourceSpan::internal32("[FALSE]"), false);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  std::set<sass::string> SlashOperands{
    "calc", "clamp", "hypot", "sin", "cos", "tan", "asin", "acos", //
    "atan", "sqrt", "exp", "sign", "mod", "rem", "atan2", "pow", "log"
  };

  bool Eval::_operandAllowsSlash(const Expression* node) const {
    if (const auto* fn = node->isaFunctionExpression()) {
      sass::string name(StringUtils::toLowerCase(fn->name()));
      if (fn->ns().empty() && SlashOperands.count(name) > 0) {
        return !compiler.varRoot.findFnIdx(name, fn->ns()).isValid();
      }
      return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Helper function for the division
  Value* Eval::doDivision(Value* left, Value* right,
    BinaryOpExpression* node, Logger& logger, SourceSpan pstate) const
  {
    // bool allowSlash = node->allowsSlash();
    ValueObj result = left->dividedBy(right, logger, pstate);
    if (Number* rv = result->isaNumber()) {
      if (left && right) {
        if (node->allowsSlash()
          && _operandAllowsSlash(node->left())
          && _operandAllowsSlash(node->right()))
        {
          rv->lhsAsSlash(left->isaNumber());
          rv->rhsAsSlash(right->isaNumber());
        }
        else {
          sass::string msg = "Using the division operator `/` outside of calc() is deprecated.";
          msg += "\nThis will be removed in LibSass 5.0.0.\n";
          msg += "\nRecommendation: " + node->recommendation() + " or " + node->toCalc() + "\n";
          msg += "\nMore info and automated migrator: https://sass-lang.com/d/slash-div";
          logger.addDeprecation(msg, pstate, Logger::WARN_MATH_DIV);
        }
      } 
    }
    return result.detach();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Number* Eval::withoutSlash4(Number* number)
  {
    // Only numbers can have delayed slashes
    if (number == nullptr) return number;
    // Only create a new variable if required
    if (!number->hasAsSlash()) return number;
    // Create a deprecation warning for this case
    sass::string msg = "Using the division operator `/` is deprecated."
      "\nThis will be removed in LibSass 5.0.0.\n"
      "\nRecommendation: " + number->recommendation() + "\n"
      "\nMore info and automated migrator: https://sass-lang.com/d/slash-div";
    logger.addDeprecation(msg, number->pstate(), Logger::WARN_MATH_DIV);
    // Creates a new value (ensure to delete)
    return number->withoutSlash5();
  }

  Value* Eval::withoutSlash3(Value* value)
  {
    if (value == nullptr) return value;
    // Only numbers can have delayed slashes
    Number* nr = withoutSlash4(value->isaNumber());
    return nr ? nr : value;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Fetch unevaluated positional argument (optionally by name)
  // Will error if argument is missing or available both ways
  // Note: only needed for lazy evaluation in if expressions
  Expression* Eval::getArgument(
    ExpressionVector& positional,
    const ExpressionFlatMap* named,
    size_t idx, const EnvKey& name)
  {
    if (named && !named->empty()) {
      // Try to find the argument by name
      auto it = named->find(name);
      // Check if requested index is available
      if (positional.size() > idx) {
        // Check if argument is also known by name
        if (it != named->end()) {
          // Raise error since it's ambiguous
          throw Exception::ArgumentGivenTwice(
            logger, name);
        }
        // Return the positional value
        return positional[idx];
      }
      else if (it != named->end()) {
        // Return the expression
        return it->second;
      }
    }
    // Check if requested index is available
    else if (positional.size() > idx) {
      // Return the positional value
      return positional[idx];
    }
    // Raise error since nothing was found
    throw Exception::MissingArgument(
      logger, name);

  }

  // Fetch evaluated positional argument (optionally by name)
  // Will error if argument is missing or available both ways
  // Named arguments are consumed and removed from the hash
  Value* Eval::getParameter(
    ArgumentResults& results,
    size_t idx, const Argument* arg)
  {
    // Check if requested index is available
    if (results.positional().size() > idx) {
      if (results.hasNamed()) {
        // Try to find the argument by name
        auto it = results.named()->find(arg->name());
        // Check if argument is also known by name
        if (it != results.named()->end()) {
          // Raise error since it's ambiguous
          throw Exception::ArgumentGivenTwice(
            logger, arg->name());
        }
      }
      // Return the positional value
      return results.positional()[idx];
    }
    // Check if argument was found be name
    else {
      if (results.hasNamed()) {
        // Try to find the argument by name
        auto it = results.named()->find(arg->name());
        if (it != results.named()->end()) {
          // Get value object from hash
          // Need to hold onto the object
          ValueObj val = it->second;
          // Item has been consumed
          // Would destroy the value
          results.named()->erase(it);
          // Detach to survive
          return val.detach();
        }
        // Check if we have default values
        else if (!arg->defval().isNull()) {
          // Return evaluated expression
          return arg->defval()->accept(this);
        }
      }
      // Check if we have default values
      else if (!arg->defval().isNull()) {
        // Return evaluated expression
        return arg->defval()->accept(this);
      }
    }
    // Raise error since nothing was found
    throw Exception::MissingArgument(
      logger, arg->name());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  // Call built-in function with no overloads
  //*************************************************//
  Value* Eval::_runBuiltInCallable(
    CallableArguments* arguments,
    BuiltInCallable* callable,
    const SourceSpan& pstate)
  {
    ArgumentResults results(_evaluateArguments(arguments));
    const SassFnPair& tuple(callable->callbackFor(results));
    ValueObj rv = _callBuiltInCallable(results, tuple, pstate);
    rv = withoutSlash3(rv);
    return rv.detach();
  }
  // EO _runBuiltInCallable

  //*************************************************//
  // Call built-in function with overloads
  //*************************************************//
  Value* Eval::_runBuiltInCallables(
    CallableArguments* arguments,
    BuiltInCallables* callable,
    const SourceSpan& pstate)
  {
    ArgumentResults results(_evaluateArguments(arguments));
    const SassFnPair& tuple(callable->callbackFor(results));
    return _callBuiltInCallable(results, tuple, pstate);
  }
  // EO _runBuiltInCallables

  //*************************************************//
  // Helper for _runBuiltInCallable(s)
  //*************************************************//
  Value* Eval::_callBuiltInCallable(
    ArgumentResults& results,
    const SassFnPair& function,
    const SourceSpan& pstate)
  {

    // Here the strategy is to re-use the positional arguments if possible
    // In the end we need one continuous array to pass to the built-in callable
    // So we need to split out restargs into it's own array, where as in other
    // implementations we can re-use positional array for this purpose!

    // Get some items from passed parameters
    const SassFnSig& callback(function.second);
    const CallableSignature* prototype(function.first);
    if (!callback) throw std::runtime_error("Mixin declaration has no callback");
    if (!prototype) throw std::runtime_error("Mixin declaration has no prototype");
    const sass::vector<ArgumentObj>& parameters(prototype->arguments());

    // Get reference to positional arguments in the result object
    // Multiple calls to the same function may re-use the object
    ValueVector& positional(results.positional());

    // Needed here for a specific edge case: restargs must be consumed
    // Those can be consumed e.g. by passing them to other functions
    // Or simply by calling `keywords` on the rest arguments
    ArgumentListObj restargs;

    // If the callable accepts rest argument we can pass all unknown args
    // Also if we must pass rest args we must pass only the remaining parts
    if (prototype->restArg().empty() == false) {

      // Superfluous function arguments
      ValueVector superflous;

      // Check if more arguments provided than parameters
      if (positional.size() > parameters.size()) {
        // Move superfluous arguments into the array
        std::move(positional.begin() + parameters.size(),
          positional.end(), back_inserter(superflous));
        // Remove the consumed positional arguments
        positional.resize(parameters.size());
      }

      // Try to get named function parameters from argument results
      for (size_t i = positional.size(); i < parameters.size(); i += 1) {
        positional.push_back(getParameter(results, i, parameters[i]));
      }

      // Inherit separator from argument results
      SassSeparator separator(results.separator());
      // But make the default a comma instead of spaces
      if (separator == SASS_UNDEF) separator = SASS_COMMA;
      // Create the rest arguments (move remaining stuff)
      restargs = SASS_MEMORY_NEW(ArgumentList, pstate, separator,
        std::move(superflous), std::move(results.named()));
      // Append last parameter (rest arguments)
      positional.emplace_back(restargs);

    }
    // Function takes rest arguments, so superfluous arguments must
    // be passed to the function via the rest argument array
    else {

      // Check that all positional arguments are consumed
      if (positional.size() > parameters.size()) {
        throw Exception::TooManyArguments(logger,
          positional.size(), prototype->maxArgs());
      }

      // Try to get needed function parameters from argument results
      for (size_t i = positional.size(); i < parameters.size(); i += 1) {
        positional.push_back(getParameter(results, i, parameters[i]));
      }

      // Check that all named arguments are consumed
      if (results.hasNamed()) {
        throw Exception::TooManyArguments(
          logger, results.named());
      }

    }

    for (ValueObj& arg : positional) {
      arg = withoutSlash3(arg);
    }

    // Now execute the built-in function
    ValueObj result = callback(pstate,
      positional, compiler,
      *this); // 7%

    // If we had no rest arguments, this will be true
    if (restargs == nullptr) return result.detach();
    // Check if all keywords have been marked consumed, meaning we
    // either don't have any or somebody called `keywords` method
    if (restargs->hasAllKeywordsConsumed()) return result.detach();

    // Throw error since not all named arguments were consumed
    throw Exception::DuplicateKeyArgument(logger, restargs->keywords());
  }
  // EO _callBuiltInCallable

  //*************************************************//
  // Used for user functions and also by
  // mixin includes and content includes.
  //*************************************************//
  Value* Eval::_runUserDefinedCallable(
    CallableArguments* arguments,
    UserDefinedCallable* callable,
    const SourceSpan& pstate)
  {

    // Here the strategy is to put variables on the current function scope
    // Therefore we do not really need to results anymore once we set them
    // Therefore we can re-use the positional array for our restargs

    // Get some items from passed parameters
    CallableDeclaration* declaration(callable->declaration());
    CallableSignature* prototype(declaration->arguments());
    if (!prototype) throw std::runtime_error("Mixin declaration has no prototype");
    const sass::vector<ArgumentObj>& parameters(prototype->arguments());

    ArgumentResults results(_evaluateArguments(arguments));

    // Get reference to positional arguments in the result object
    // Multiple calls to the same function may re-use the object
    ValueVector& positional(results.positional());

    // Create the variable scope to pass args
    auto idxs = callable->declaration()->idxs;
    EnvScope envscope(compiler.varRoot, idxs);

    // Try to fetch arguments for all parameters
    for (uint32_t i = 0; i < parameters.size(); i += 1) {
      // Errors if argument is missing or given twice
      ValueObj value = getParameter(results, i, parameters[i]);
      // Check for deprecated division
      value = withoutSlash3(value);
      // Set lexical variable on scope
      compiler.varRoot.setVariable({ idxs, i },
        value->withoutSlash(), false);
    }

    // Needed here for a specific edge case: restargs must be consumed
    // Those can be consumed e.g. by passing them to other functions
    // Or simply by calling `keywords` on the rest arguments
    ArgumentListObj restargs;

    // If the callable accepts rest argument we can pass all unknown args
    // Also if we must pass rest args we must pass only the remaining parts
    if (prototype->restArg().empty() == false) {

      // Remove consumed items (vars already set)
      // This will leave the rest arguments behind
      if (positional.size() > parameters.size()) {
        positional.erase(positional.begin(),
          positional.begin() + parameters.size());
      }
      else {
        positional.clear();
      }

      // Inherit separator from argument results
      SassSeparator separator(results.separator());
      // But make the default a comma instead of spaces
      if (separator == SASS_UNDEF) separator = SASS_COMMA;
      // Create the rest arguments (move remaining stuff)
      restargs = SASS_MEMORY_NEW(ArgumentList, pstate, separator,
        std::move(positional), std::move(results.named()));
      // Set last lexical variable on scope
      compiler.varRoot.setVariable(
        { idxs, (uint32_t)parameters.size() },
        restargs.ptr(), false);

    }
    else {

      // Check that all positional arguments are consumed
      if (positional.size() > parameters.size()) {
        throw Exception::TooManyArguments(logger,
          positional.size(), parameters.size());
      }

      // Check that all named arguments are consumed
      if (results.hasNamed()) {
        throw Exception::TooManyArguments(
          logger, results.named());
      }

    }

    ValueObj result;
    // Process all statements within user defined function
    // Only the `@return` statement must return something!
    for (Statement* statement : declaration->elements()) {
      result = statement->accept(this);
      if (result != nullptr) break;
    }

    // If we had no rest arguments, this will be true
    if (restargs == nullptr) return result.detach();
    // Check if all keywords have been marked consumed, meaning we
    // either don't have any or somebody called `keywords` method
    if (restargs->hasAllKeywordsConsumed()) return result.detach();

    // Throw error since not all named arguments were consumed
    throw Exception::TooManyArguments(logger, restargs->keywords());

  }
  // EO _runUserDefinedCallable

  //*************************************************//
  // Call external C-API function
  //*************************************************//
  Value* Eval::_runExternalCallable(
    CallableArguments* arguments,
    ExternalCallable* callable,
    const SourceSpan& pstate)
  {

    // Here the strategy is to put variables into a sass list of Values

    // Get some items from passed parameters
    const EnvKey& name(callable->envkey());
    SassFunctionLambda lambda(callable->lambda());
    CallableSignature* prototype(callable->declaration());
    if (!lambda) throw std::runtime_error("C-API declaration has no callback");
    if (!prototype) throw std::runtime_error("C-API declaration has no prototype");
    const sass::vector<ArgumentObj>& parameters(prototype->arguments());

    ArgumentResults results(_evaluateArguments(arguments));
    ValueFlatMap* named(results.named());
    ValueVector& positional(results.positional());

    // Verify that the passed arguments are valid for this function
    prototype->verify(positional.size(), named, pstate, logger);

    // Process all prototype items which are not positional
    for (size_t i = positional.size(); i < parameters.size(); i++) {
      // Try to find name in passed arguments
      Argument* argument = parameters[i];
      const auto& name(argument->name());
      if (named)
      {
        const auto& it(named->find(name));
        // Check if we found the name
        if (it != named->end()) {
          // Append it to our positional args
          positional.emplace_back(it->second);
          named->erase(it); // consume argument
        }
        // Otherwise check if argument has a default value
        else if (!argument->defval().isNull()) {
          // Evaluate the expression into final value
          Value* defval(argument->defval()->accept(this));
          // Append it to our positional args
          positional.emplace_back(defval);
        }
        else {
          // This case should never happen due to verification
          throw std::runtime_error("Verify did not protect us!");
        }

      }
      // Otherwise check if argument has a default value
      else if (!argument->defval().isNull()) {
        // Evaluate the expression into final value
        Value* defval(argument->defval()->accept(this));
        // Append it to our positional args
        positional.emplace_back(defval);
      }
      else {
        // This case should never happen due to verification
        throw std::runtime_error("Verify did not protect us!");
      }
    }

    // Needed here for a specific edge case: restargs must be consumed
    // Those can be consumed e.g. by passing them to other functions
    // Or simply by calling `keywords` on the rest arguments
    ArgumentListObj restargs;

    // If the callable accepts rest argument we can pass all unknown args
    // Also if we must pass rest args we must pass only the remaining parts
    if (prototype->restArg().empty() == false) {
      // Superfluous function arguments
      ValueVector superflous;
      // Check if more arguments provided than parameters
      if (positional.size() > parameters.size()) {
        // Move superfluous arguments into the array
        std::move(positional.begin() + parameters.size(),
          positional.end(), back_inserter(superflous));
        // Remove the consumed positional arguments
        positional.resize(parameters.size());
      }

      SassSeparator separator = results.separator();
      if (separator == SASS_UNDEF) separator = SASS_COMMA;
      restargs = SASS_MEMORY_NEW(ArgumentList,
        prototype->pstate(), separator,
        std::move(superflous), std::move(named));
      positional.emplace_back(restargs);
    }

    // Create a new sass list holding parameters to pass to function
    struct SassValue* c_args = sass_make_list(SASS_COMMA, false);
    // First append all positional parameters to it
    for (size_t i = 0; i < positional.size(); i++) {
      sass_list_push(c_args, Value::wrap(positional[i]));
    }

    // Now invoke the function of the callback object
    struct SassValue* c_val = (*lambda)(
      c_args, compiler.wrap(), callable->cookie());
    // It may not return anything at all
    if (c_val == nullptr) return nullptr;
    // Unwrap the result into C++ object
    ValueObj value(&Value::unwrap(c_val));

    // Check for some specific return types to handle
    // Can't use throw in C code, so this has to do it
    if (CustomError* err = value->isaCustomError()) {
      sass::string message("C-API function " +
        name.orig() + ": " + err->message());
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      throw Exception::ParserException(logger, message);
    }
    // This will simply invoke the warning handler
    // ToDo: we should have another way to call this
    // We might want to warn beside returning a value
    else if (CustomWarning* warn = value->isaCustomWarning()) {
      sass::string message("C-API function "
        + name.orig() + ": " + warn->message());
      // warn->pstate(pstate);
      sass_delete_value(c_args);
      sass_delete_value(c_val);
      logger.addWarning(message,
        Logger::WARN_CAPI_FN);
    }
    sass_delete_value(c_val);
    sass_delete_value(c_args);

    // If we had no rest arguments, this will be true
    if (restargs == nullptr) return value.detach();
    // Check if all keywords have been marked consumed, meaning we
    // either don't have any or somebody called `keywords` method
    if (restargs->hasAllKeywordsConsumed()) return value.detach();

    // Throw error since not all named arguments were consumed
    throw Exception::TooManyArguments(logger, restargs->keywords());

  }
  // EO _runExternalCallable

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  // Call built-in function with no overloads
  //*************************************************//
  Value* Eval::execute(
    BuiltInCallable* callable,
    CallableArguments* arguments,
    const SourceSpan& pstate)
  {
    const EnvKey& key(callable->envkey());
    BackTrace trace(pstate, key.orig(), true);
    CallStackFrame frame(logger, trace);
    ValueObj rv = _runBuiltInCallable(
      arguments, callable, pstate);
    if (rv.isNull()) {
      throw Exception::RuntimeException(logger,
        "Function finished without @return.");
    }
    rv = rv->withoutSlash();
    return rv.detach();
  }

  //*************************************************//
  // Call built-in function with overloads
  //*************************************************//
  Value* Eval::execute(
    BuiltInCallables* callable,
    CallableArguments* arguments,
    const SourceSpan& pstate)
  {
    const EnvKey& key(callable->envkey());
    BackTrace trace(pstate, key.orig(), true);
    CallStackFrame frame(logger, trace);
    ValueObj rv = _runBuiltInCallables(arguments,
      callable, pstate);
    if (rv.isNull()) {
      throw Exception::RuntimeException(logger,
        "Function finished without @return.");
    }
    rv = rv->withoutSlash();
    return rv.detach();
  }

  //*************************************************//
  // Used for user functions and also by
  // mixin includes and content includes.
  //*************************************************//
  Value* Eval::execute(
    UserDefinedCallable* callable,
    CallableArguments* arguments,
    const SourceSpan& pstate)
  {
    RAII_FLAG(inMixin, false);
    const EnvKey& key(callable->envkey());
    BackTrace trace(pstate, key.orig(), true);
    CallStackFrame frame(logger, trace);
    ValueObj rv = _runUserDefinedCallable(
      arguments, callable, pstate);
    if (rv.isNull()) {
      throw Exception::RuntimeException(logger,
        "Function finished without @return.");
    }
    rv = rv->withoutSlash();
    return rv.detach();
  }

  //*************************************************//
  // Call external C-API function
  //*************************************************//
  Value* Eval::execute(
    ExternalCallable* callable,
    CallableArguments* arguments,
    const SourceSpan& pstate)
  {
    const EnvKey& key(callable->envkey());
    BackTrace trace(pstate, key.orig(), true);
    CallStackFrame frame(logger, trace);
    ValueObj rv = _runExternalCallable(
      arguments, callable, pstate);
    if (rv.isNull()) {
      throw Exception::RuntimeException(logger,
        "Function finished without @return.");
    }
    rv = rv->withoutSlash();
    return rv.detach();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  ArgumentResults Eval::_evaluateArguments(
    CallableArguments* arguments)
  {
    ArgumentResults results;
    results.reserve(
      arguments->positional().size() +
      (arguments->restArg() ? 1 : 0));
    // Get some items from passed parameters
    ValueVector& positional(results.positional());

    // Collect positional args by evaluating input arguments
    for (const auto& arg : arguments->positional())
    {
      ValueObj result(arg->accept(this));
      if (Number* nr = result->isaNumber()) {
        result = withoutSlash4(nr);
      }
      positional.emplace_back(result);
    }

    // Collect named args by evaluating input arguments
    if (arguments->hasNamed()) {
      for (const auto& kv : *arguments->named()) {
        ValueObj result(kv.second->accept(this));
        results.addNamed(kv.first, withoutSlash3(result));
      }
    }

    // Abort if we don't take any restargs
    if (arguments->restArg() == nullptr) {
      // ToDo : no test case for this!?
      results.separator(SASS_UNDEF);
      // if (qwe != results.positional().size()) {
      //   throw Exception::RuntimeException(compiler, "NO 1");
      // }
      return results;
    }

    // Evaluate the variable expression (
    ValueObj result = arguments->restArg()->accept(this);
    ValueObj rest = withoutSlash3(result);

    SassSeparator separator = SASS_UNDEF;

    if (Map* restMap = rest->isaMap()) {
      _addRestValueMap(results, restMap,
        arguments->restArg()->pstate());
    }
    else if (List* list = rest->isaList()) {
      std::copy(list->begin(), list->end(),
        std::back_inserter(positional));
      separator = list->separator();
      if (ArgumentList* args = rest->isaArgumentList()) {
        if (args->keywords()) {
          for (const auto& kv : *args->keywords()) {
            results.addNamed(kv.first, kv.second);
          }
        }
      }
    }
    else {
      positional.emplace_back(std::move(rest));
    }

    if (arguments->kwdRest() == nullptr) {
      results.separator(separator);
      //if (qwe != results.positional().size()) {
      //  size_t ot = arguments->size();
      //  std::cerr << "SUGGESTED " << qwe << " or " << ot << " => has " << results.positional().size() << "\n";
      //  throw Exception::RuntimeException(compiler, "NO 2");
      //}
      return results;
    }

    ValueObj keywordRest = arguments->kwdRest()->accept(this);

    if (Map* restMap = keywordRest->isaMap()) {
      _addRestValueMap(results, restMap, arguments->kwdRest()->pstate());
      results.separator(separator);
      //if (qwe != results.positional().size()) {
      //  std::cerr << "SUGGESTED " << qwe << " => has " << results.positional().size() << "\n";
      //  throw Exception::RuntimeException(compiler, "NO 3");
      //}
      return results;
    }

    CallStackFrame csf(logger, keywordRest->pstate());
    throw Exception::RuntimeException(logger,
      "Variable keyword arguments must be a map (was $keywordRest).");

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  /// Evaluates [expression] and calls `toCss()`.
  sass::string Eval::toCss(Expression* expression, bool quote)
  {
    ValueObj value = expression->accept(this);
    return value->toCss(quote);
  }

  /// Evaluates [interpolation] into a serialized string.
  ///
  /// If [trim] is `true`, removes whitespace around the result.
  /// If [warnForColor] is `true`, this will emit a warning for
  /// any named color values passed into the interpolation.
  sass::string Eval::acceptInterpolation(InterpolationObj interpolation, bool warnForColor, bool trim)
  {
    // Needed in loop
    ValueObj value;
    // Create CSS output options
    OutputOptions out(
      SASS_STYLE_TO_CSS,
      compiler.precision);
    // Create the emitter
    Cssize cssize(out);
    // Don't quote strings
    cssize.quotes = false;

    RAII_FLAG(inSupportsDeclaration, false);
    // Process all interpolants in the interpolation
    // Items in interpolations are only of three types
    // Performance optimized since it's used quite a lot
    for (Interpolant* itpl : interpolation->elements()) {
      if (itpl == nullptr) continue;
      switch (itpl->getType()) {
      case Interpolant::LiteralInterpolant:
        cssize.append_token(
          static_cast<ItplString*>(itpl)->text(),
          static_cast<ItplString*>(itpl));
        break;
      case Interpolant::ValueInterpolant:
        static_cast<Value*>(itpl)
          ->accept(&cssize);
        break;
      case Interpolant::ExpressionInterpolant:
          value = static_cast<Expression*>(itpl)->accept(this);
        if (warnForColor) {
          if (Color* color = value->isaColor()) {
            ColorRgbaObj rgba = color->toRGBA();
            double numval = rgba->r() * 0x10000
              + rgba->g() * 0x100 + rgba->b();
            if (const char* disp = color_to_name((int)numval)) {
              sass::sstream msg;
              msg << "You probably don't mean to use the color value ";
              msg << disp << " in interpolation here.\nIt may end up represented ";
              msg << "as " << rgba->inspect() <<", which will likely produce invalid ";
              msg << "CSS. Always quote color names when using them as strings or map ";
              msg << "keys (for example, \"" << disp << "\"). If you really want to ";
              msg << "use the color value, append it to an empty string to avoid ";
              msg << "this warning (e.g. use '\"\" + " << disp << "').";
              logger.addWarning(msg.str(), itpl->pstate(), Logger::WARN_COLOR_ITPL);
            }
          }
        }
        value->accept(&cssize);
        break;
      }
    }
    // ToDo: check it's using RVO
    return cssize.get_buffer(trim);
  }
  // EO acceptInterpolation

  /// Evaluates [interpolation] and wraps the result in a [SourceData].
  ///
  /// If [trim] is `true`, removes whitespace around the result.
  /// If [warnForColor] is `true`, this will emit a warning for
  /// any named color values passed into the interpolation.
  SourceData* Eval::interpolationToSource(InterpolationObj interpolation, bool warnForColor, bool trim, bool ws)
  {
    if (interpolation.isNull()) return nullptr;
    // pstate has 13 with ending
    sass::string result = acceptInterpolation(interpolation, warnForColor, trim);
    // Check if white-space only is disallowed; check and possibly abort
    if (!ws && StringUtils::isWhitespaceOnly(result)) return nullptr;
    // if (!ws && std::find(result.begin(), result.end(), std::isspace) == result.end()) return nullptr;
    return SASS_MEMORY_NEW(SourceItpl, interpolation->pstate(), std::move(result));
  }

  /// Evaluates [interpolation] and wraps the result in a [CssValue].
  ///
  /// If [trim] is `true`, removes whitespace around the result.
  /// If [warnForColor] is `true`, this will emit a warning for
  /// any named color values passed into the interpolation.
  sass::string Eval::interpolationToCssString(InterpolationObj interpolation,
    bool warnForColor, bool trim)
  {
    if (interpolation.isNull()) return str_empty;
    return acceptInterpolation(interpolation, warnForColor, trim);
  }

  /// Evaluates [interpolation] and parses the result into a [SelectorList].
  SelectorListObj Eval::interpolationToSelector(Interpolation* itpl, bool plainCss, bool allowParent)
  {
    // Create a new source data object from the evaluated interpolation
    if (SourceDataObj synthetic = interpolationToSource(itpl, true, false, false)) {
      // Everything parsed, will be parsed from perspective of local content
      // Pass the source-map in for the interpolation, so the scanner can
      // update the positions according to previous source-positions
      // Is a parser state solely represented by a source map or do we
      // need an intermediate format for them?
      // std::cerr << "EVAL STYLE RULE " << synthetic->content() << "\n";
      SelectorParser parser(compiler, synthetic, true, plainCss);
      parser.allowParent = allowParent; // && plainCss == false;
      return parser.parseSelectorList(); // comes detached!
    }
    // Otherwise interpolation resulted in white-space only
    CallStackFrame frame(compiler, BackTrace(itpl->pstate()));
    throw Exception::ParserException(compiler, "expected selector.");
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Eval::_evaluateMacroArguments(
    CallableArguments* arguments,
    ExpressionVector& positional)
  {

    if (arguments->restArg()) {

      ValueObj rest = arguments->restArg()->accept(this);

      if (Map* restMap = rest->isaMap()) {
        _addRestExpressionMap(arguments, restMap,
          arguments->restArg()->pstate());
      }
      else if (List* restList = rest->isaList()) {
        for (const ValueObj& value : restList->elements()) {
          positional.emplace_back(SASS_MEMORY_NEW(
            ValueExpression, value->pstate(), value));
        }
        // separator = list->separator();
        if (ArgumentList* args = rest->isaArgumentList()) {
          if (args->keywords()) {
            for (auto& kv : *args->keywords()) {
              arguments->addNamed(kv.first,
                SASS_MEMORY_NEW(ValueExpression,
                  kv.second->pstate(), kv.second));
            }
          }
        }
      }
      else {
        positional.emplace_back(SASS_MEMORY_NEW(
          ValueExpression, rest->pstate(), rest));
      }

    }

    if (arguments->kwdRest() == nullptr) {
      return;
    }

    ValueObj keywordRest = arguments->kwdRest()->accept(this);

    if (Map* restMap = keywordRest->isaMap()) {
      _addRestExpressionMap(arguments, restMap,
        arguments->restArg()->pstate());
      return;
    }

    throw Exception::RuntimeException(logger,
      "Variable keyword arguments must be a map (was $keywordRest).");

  }
  // EO _evaluateMacroArguments

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // CssStylesheet _combineCss(Module<Callable> root, { bool clone = false }) {
  // void _extendModules(List<Module<Callable>> sortedModules) {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Value* Eval::visitBooleanExpression(BooleanExpression* ex)
  {
    #ifdef SASS_ELIDE_COPIES
    return ex->value();
    #else
    return SASS_MEMORY_COPY(ex->value());
    #endif
  }

  Value* Eval::visitColorExpression(ColorExpression* ex)
  {
    #ifdef SASS_ELIDE_COPIES
    return ex->value();
    #else
    ColorObj color = ex->value();
    ColorObj copy = SASS_MEMORY_COPY(color);
    copy->disp(color->disp());
    return copy.detach();
#endif
  }

  Value* Eval::visitNumberExpression(NumberExpression* ex)
  {
    #ifdef SASS_ELIDE_COPIES
    return ex->value();
    #else
    return SASS_MEMORY_COPY(ex->value());
    #endif
  }

  Value* Eval::visitNullExpression(NullExpression* ex)
  {
    #ifdef SASS_ELIDE_COPIES
    return ex->value();
    #else
    return SASS_MEMORY_COPY(ex->value());
    #endif
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  //*************************************************//
  Value* Eval::visitListExpression(ListExpression* l)
  {
    // regular case for unevaluated lists
    ListObj ll = SASS_MEMORY_NEW(List, l->pstate(),
      ValueVector(), l->separator());
    ll->hasBrackets(l->hasBrackets());
    for (size_t i = 0, L = l->size(); i < L; ++i) {
      ll->append(l->get(i)->accept(this));
    }
    return ll.detach();
  }
  // EO visitListExpression

  //*************************************************//
  //*************************************************//
  Value* Eval::visitMapExpression(MapExpression* m)
  {
    ValueObj key;
    MapObj map(SASS_MEMORY_NEW(Map, m->pstate()));
    const ExpressionVector& kvlist(m->kvlist());
    for (size_t i = 0, L = kvlist.size(); i < L; i += 2)
    {
      // First evaluate the key
      key = kvlist[i]->accept(this);
      // Check for key duplication
      if (map->has(key)) {
        logger.callStack.emplace_back(kvlist[i]->pstate());
        throw Exception::DuplicateKeyError(logger, *map, *key);
      }
      // Second insert the evaluated value for key
      map->insertOrSet(key, kvlist[i + 1]->accept(this));
    }
    return map.detach();
  }
  // EO visitMapExpression

  //*************************************************//
  //*************************************************//
  Value* Eval::visitStringExpression(StringExpression* node)
  {
    // Don't use [performInterpolation] here because we need to get
    // the raw text from strings, rather than the semantic value.
    const Interpolation* itpl = node->text();
    sass::vector<sass::string> strings;
    RAII_FLAG(inSupportsDeclaration, false);
    for (const auto& item : itpl->elements()) {
      if (const ItplString* lit = item->isaItplString()) {
        strings.emplace_back(lit->text());
      }
      else {
        ValueObj result;
        if (Expression* ex = item->isaExpression()) {
          result = ex->accept(this);
        }
        else {
          result = item->isaValue();
        }
        if (const String* lit = result->isaString()) {
          strings.emplace_back(lit->value());
        }
        else if (!result->isNull()) {
          strings.emplace_back(result->toCss(false));
        }
      }
    }

    return SASS_MEMORY_NEW(String, node->pstate(),
      StringUtils::join(strings, ""), node->hasQuotes());
  }
  // EO visitStringExpression

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Value* Eval::visitSupportsExpression(SupportsExpression* expr)
  {
    sass::string text(_visitSupportsCondition(expr->condition()));
    return new String(expr->pstate(), std::move(text));
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  //*************************************************//
  Value* Eval::visitBinaryOpExpression(BinaryOpExpression* node)
  {

    Import* imp = compiler.import_stack.back();
    bool isPlainCss = imp->syntax == SASS_IMPORT_CSS;

    if (isPlainCss) {
      if (node->operand() != SassOperator::ASSIGN) {
        if (node->operand() != SassOperator::DIV) {
          // CallStackFrame frame(compiler, node->pstate());
          CallStackFrame frame2(compiler, node->opstate());
          throw Exception::SassScriptException(logger, node->pstate(),
            "Operators aren't allowed in plain CSS.");
        }
      }
    }
    ValueObj left, right;
    Expression* lhs = node->left();
    Expression* rhs = node->right();
    left = lhs->accept(this);
    switch (node->operand()) {
    case SassOperator::IESEQ:
      right = rhs->accept(this);
      return left->singleEquals(
        right, logger, node->pstate());
    case SassOperator::OR:
      if (left->isTruthy()) {
        return left.detach();
      }
      return rhs->accept(this);
    case SassOperator::AND:
      if (!left->isTruthy()) {
        return left.detach();
      }
      return rhs->accept(this);
    case SassOperator::EQ:
      right = rhs->accept(this);
      return ObjEqualityFn(left, right)
        ? bool_true : bool_false;
    case SassOperator::NEQ:
      right = rhs->accept(this);
      return ObjEqualityFn(left, right)
        ? bool_false : bool_true;
    case SassOperator::GT:
      right = rhs->accept(this);
      return left->greaterThan(right,
        logger, node->pstate())
        ? bool_true : bool_false;
    case SassOperator::GTE:
      right = rhs->accept(this);
      return left->greaterThanOrEquals(right,
        logger, node->pstate())
        ? bool_true : bool_false;
    case SassOperator::LT:
      right = rhs->accept(this);
      return left->lessThan(right,
        logger, node->pstate())
        ? bool_true : bool_false;
    case SassOperator::LTE:
      right = rhs->accept(this);
      return left->lessThanOrEquals(right,
        logger, node->pstate())
        ? bool_true : bool_false;
    case SassOperator::ADD:
      right = rhs->accept(this);
      return left->plus(right,
        logger, node->pstate());
    case SassOperator::SUB:
      right = rhs->accept(this);
      return left->minus(right,
        logger, node->pstate());
    case SassOperator::MUL:
      right = rhs->accept(this);
      return left->times(right,
        logger, node->pstate());
    case SassOperator::DIV:
      right = rhs->accept(this);
      return doDivision(left, right,
        node, logger, node->pstate());
    case SassOperator::MOD:
      right = rhs->accept(this);
      return left->modulo(right,
        logger, node->pstate());
    case SassOperator::ASSIGN:
      return nullptr;
    //  throw "Assign not implemented";
    }
    // Satisfy compiler
    return nullptr;
  }
  // visitBinaryOpExpression

  //*************************************************//
  //*************************************************//
  Value* Eval::visitUnaryOpExpression(UnaryOpExpression* node)
  {
    ValueObj operand = node->operand()->accept(this);
    switch (node->optype()) {
    case UnaryOpType::PLUS:
      return operand->unaryPlus(logger, node->pstate());
    case UnaryOpType::MINUS:
      return operand->unaryMinus(logger, node->pstate());
    case UnaryOpType::NOT:
      return operand->unaryNot(logger, node->pstate());
    case UnaryOpType::SLASH:
      return operand->unaryDivide(logger, node->pstate());
    }
    // Satisfy compiler
    return nullptr;
  }
  // EO visitUnaryOpExpression

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // This operates similar to a function call
  Value* Eval::visitIfExpression(IfExpression* node)
  {
    CallableArguments* arguments = node->arguments();
    CallStackFrame frame(logger, node->pstate());
    // We need to make copies here to preserve originals
    // We could optimize this further, but impact is slim
    const ExpressionFlatMap* named(arguments->named());
    ExpressionVector positional(arguments->positional());
    // Rest arguments must be evaluated in all cases
    // evaluateMacroArguments is only used for this
    _evaluateMacroArguments(node->arguments(), positional);
    ExpressionObj condition = getArgument(positional, named, 0, Keys::condition);
    ExpressionObj ifTrue = getArgument(positional, named, 1, Keys::ifTrue);
    ExpressionObj ifFalse = getArgument(positional, named, 2, Keys::ifFalse);
    if (positional.size() > 3) {
      throw Exception::TooManyArguments(
        logger, positional.size(), 3);
    }
    size_t nn = named ? named->size() : 0;
    if (positional.size() + nn > 3) { // arguments->size()
      EnvKeySet set;
      set.insert(Keys::condition);
      set.insert(Keys::ifTrue);
      set.insert(Keys::ifFalse);
      throw Exception::TooManyArguments(logger, named, set);
    }

    ValueObj rv = condition ? condition->accept(this) : nullptr;
    Expression* ex = rv && rv->isTruthy() ? ifTrue : ifFalse;
    if (ex == nullptr) return nullptr;
    ValueObj result(ex->accept(this));
    return (result = withoutSlash3(result)).detach();
  }

  Value* Eval::visitParenthesizedExpression(ParenthesizedExpression* ex)
  {
    Import* imp = compiler.import_stack.back();
    bool isPlainCss = imp->syntax == SASS_IMPORT_CSS;

    if (isPlainCss) {
      CallStackFrame frame(logger, ex->pstate());
      throw Exception::RuntimeException(logger,
        "Parentheses aren't allowed in plain CSS.");
    }

    // return ex->expression();
    if (ex->expression()) {
      return ex->expression()->accept(this);
    }
    return nullptr;
  }

  Value* Eval::visitSelectorExpression(SelectorExpression* p)
  {
    if (SelectorListObj& parents = original()) {
      return parents->toValue();
    }
    else {
      return SASS_MEMORY_NEW(Null, p->pstate());
    }
  }

  void Eval::renderArgumentInvocation(sass::string& strm, CallableArguments* args)
  {
    if (args->hasNamed()) {
      CallStackFrame frame(logger,
        args->pstate());
      throw Exception::RuntimeException(logger,
        "Plain CSS functions don't support keyword arguments.");
    }
    if (args->kwdRest() != nullptr) {
      CallStackFrame frame(logger,
        args->kwdRest()->pstate());
      throw Exception::RuntimeException(logger,
        "Plain CSS functions don't support keyword arguments.");
    }
    bool addComma = false;
    strm += "(";
    for (Expression* argument : args->positional()) {
      if (addComma) { strm += ", "; }
      else { addComma = true; }
      strm += toCss(argument);
    }
    if (ExpressionObj rest = args->restArg()) {
      if (addComma) { strm += ", "; }
      else { addComma = true; }
      strm += toCss(rest);
    }
    strm += ")";
  }

  Value* Eval::visitItplFnExpression(ItplFnExpression* cssfn)
  {
    // return ex->expression();
    if (cssfn->itpl()) {
      sass::string strm;
      strm += acceptInterpolation(cssfn->itpl(), false);
      renderArgumentInvocation(strm, cssfn->arguments());
      return SASS_MEMORY_NEW(
        String, cssfn->pstate(),
        std::move(strm));
    }
    return nullptr;
  }

  Value* Eval::visitValueExpression(ValueExpression* node)
  {
    // We have a bug lurking somewhere
    // without detach it gets deleted?
    ValueObj value = node->value();
    return value.detach();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Value* Eval::visitMixinRule(MixinRule* rule)
  {
    UserDefinedCallableObj callable =
      SASS_MEMORY_NEW(UserDefinedCallable,
        rule->pstate(), rule->name(), rule, nullptr);
    rule->midx(compiler.varRoot.findMixIdx(
      rule->name(), Strings::empty));
    compiler.varRoot.setMixin(
      rule->midx(), callable, false);
    return nullptr;
  }


  Value* Eval::visitFunctionRule(FunctionRule* rule)
  {
    UserDefinedCallableObj callable =
      SASS_MEMORY_NEW(UserDefinedCallable,
        rule->pstate(), rule->name(), rule, nullptr);
    rule->fidx(compiler.varRoot.findFnIdx(
      rule->name(), Strings::empty));
    compiler.varRoot.setFunction(
      rule->fidx(), callable, false);
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  // Evaluate and return lexical variable with `name`
  // Cache dynamic lookup results in `vidxs` member
  //*************************************************//
  Value* Eval::visitVariableExpression(VariableExpression* variable)
  {

    // Check if variable expression was already resolved
    if (variable->vidxs().empty()) {
      // Is variable on the local scope?
      if (variable->isLexical()) {
        // Find all idxs and fill vidxs
        compiler.varRoot.findVarIdxs(
          variable->vidxs(),
          variable->name());
      }
      // Variable is on module (root) scope
      else {
        EnvRef vidx = compiler.varRoot.findVarIdx(
          variable->name(), variable->ns());
        if (vidx.isValid()) variable->vidxs().push_back(vidx);
        // std::cerr << "INT FOUND " << vidx.offset << "\n";
      }

    }

    // Variables must be resolved from top to bottom
    // This has to do with the way how Sass handles scopes
    // E.g. in loops, the variable can first point to the outer
    // variable and later to the inner variable, if an assignment
    // exists after the first reference in that loop scope:
    // $a: 0; @for $i from 1 through 3 { @debug $a; $a: $i; } @debug $a
    // $b: 0; a { @for $i from 1 through 3 { @debug $b; $b: $i; } @debug $b }
    for (const EnvRef& vidx : variable->vidxs()) {
      Value* value = compiler.varRoot.getVariable(vidx);
      if (value != nullptr) return value->withoutSlash();
    }

    // If we reach this point we have an error
    // Mixin wasn't found and couldn't be executed
    CallStackFrame frame(logger, variable->pstate());

    // Check if variable was requested from a module and if that module actually exists
    if (variable->ns().empty() || compiler.envstack.back()->hasNameSpace(variable->ns())) {
      throw Exception::RuntimeException(logger, "Undefined variable.");
    }

    // Otherwise the module simply wasn't imported
    throw Exception::ModuleUnknown(logger, variable->ns());

  }
  // EO visitVariableExpression

  //*************************************************//
  // Execute function with `name` and return a Value
  // Cache dynamic lookup results in `fidx` member
  //*************************************************//
  Value* Eval::visitFunctionExpression(FunctionExpression* function)
  {

    // Check if function expression was already resolved
    if (!function->fidx().isValid()) {
      // Try to fetch the function by finding it by name
      // This may fail, as function expressions can also be
      // css functions if the function by name is not declared.
      function->fidx(compiler.varRoot.findFnIdx(function->name(), function->ns()));
    }

    const sass::string& fname(function->name());
    const auto& args = function->arguments();
    const auto& list = args->positional();
    CallableObj callable = nullptr;

    if (function->fidx().isValid()) {
      callable = compiler.varRoot.getFunction(function->fidx());
    }

    Import* imp = compiler.import_stack.back();
    bool isPlainCss = imp->syntax == SASS_IMPORT_CSS;

    if (!callable && !function->ns().empty())
    {

      const auto& ns(function->ns());
      auto& stack = compiler.envstack;
      if (stack.empty())
      {
        CallStackFrame csf(logger, function->pstate());
        throw Exception::ModuleUnknown(logger, ns);
      }
      else {
        bool hasNs = false;
        for (const EnvRefs* current = stack.back(); current; current = current->nextScope())
        {
         // if (current->isImport) continue;
          Module* mod = current->module;
          if (mod == nullptr) continue;
          auto it = mod->moduse.find(ns);
          if (it == mod->moduse.end()) continue;
          if (it->second.first) {
            hasNs = true;
            break;
          }
        }
        if (!hasNs) {
          CallStackFrame csf(logger, function->pstate());
          throw Exception::ModuleUnknown(logger, ns);
        }
      }

      CallStackFrame frame(logger, function->pstate());
      throw Exception::RuntimeException(logger, "Undefined function.");
    }

    if (!callable || (callable->isInternal() && function->ns().empty())) {

      Calc::CFN fn = Calc::Parse(fname);

      // Check for potential css replacement
      if (!args->hasNamed() && args->restArg().isNull())
      {
        if (Calc::hasCssReplacement(fn)) {
          if (std::all_of(list.begin(), list.end(),
            [&](const ExpressionObj& expression) {
              return expression->isCalcSafe(); }))
          {
            return visitCalcuation(fn, function, true);
          }

        }
      }

      if (Calc::hasCalculationVisitor(fn)) {
        return visitCalcuation(fn, function, false);
      }

      if (!callable)
      {
        // Convert to css function
        sass::string strm;
        strm += function->name();
        renderArgumentInvocation(
          strm, function->arguments());
        return SASS_MEMORY_NEW(
          String, function->pstate(),
          std::move(strm));
      }

    }
    else if (isPlainCss) {
      callable = SASS_MEMORY_NEW(PlainCssCallable,
        function->pstate(), function->name());
    }

    if (StringUtils::startsWith(fname, "--", 2) /* dart has some more conditions */) {
      compiler.addDeprecation(
        "Sass @function names beginning with -- are deprecated for forward-"
        "compatibility with plain CSS functions.\n"
        "For details, see https://sass-lang.com/d/css-function-mixin",
        function->span(), Logger::WARN_DOUBLE_DASH_MIXIN);
    }

    // Check if function is already defined on the frame/scope
    // Can fail if the function definition comes after the usage
    if (callable)
    {
      RAII_FLAG(inFunction, true);
      CallStackFrame frame(logger, function->pstate(), true);
      return callable->execute(*this,
        args, function->pstate());
    }

    // Only functions without namespace can be css-functions
    // Functions with namespace must be executed or fail

    // If we reach this point we have an error
    // Mixin wasn't found and couldn't be executed
    CallStackFrame frame(logger, function->pstate());
    // Otherwise the module simply wasn't imported
    throw Exception::ModuleUnknown(logger, function->ns());
  }
  // EO visitFunctionExpression

  void Eval::_checkAdjacentCalculationValues(const ValueVector& elements, const ListExpression* node)
  {
    for (size_t i = 1; i < elements.size(); i++) {
      const auto& previous = elements[i - 1];
      const auto& current = elements[i];
      if (previous->isaString() || current->isaString()) continue;

      const auto& previousNode = node->items()[i - 1];
      const auto& currentNode = node->items()[i];

      if (auto op = currentNode->isaUnaryOpExpression()) {
        // auto foo = op->optype();
        if ((op->optype() != UnaryOpType::PLUS) && (op->optype() != UnaryOpType::MINUS)) continue;
        throw Exception::OpNotCalcSafe(logger, op);
      }
      else if (auto nr = currentNode->isaNumberExpression()) {
        if (nr->value()->value() >= 0) {
          throw Exception::MissingMathOp(logger, previousNode, currentNode);
        }
        throw Exception::OpNotCalcSafe(logger, nr);
      }

      throw Exception::MissingMathOp(logger, previousNode, currentNode);


      // `calc(1 -2)` parses as a space-separated list whose second value is a
        // unary operator or a negative number, but just saying it's an invalid
        // expression doesn't help the user understand what's going wrong. We
        // add special case error handling to help clarify the issue.
      //  throw _exception(
      //    '"+" and "-" must be surrounded by whitespace in calculations.',
      //    currentNode.span.subspan(0, 1));
      //}
      //else {
      //  throw _exception('Missing math operator.',
      //    previousNode.span.expand(currentNode.span));
      //}
    }

  }

  Value* Eval::_visitCalculationExpression(Expression* node, bool inLegacySassFunction)
  {
    // std::cerr << "visit calc exp " << node->toString() << "\n";
    if (auto inner = node->isaParenthesizedExpression()) {
      // std::cerr << " eval parenthisez\n";
      ValueObj result = _visitCalculationExpression(inner->expression(), inLegacySassFunction);
      if (result->isaString()) return SASS_MEMORY_NEW(String,
        inner->pstate(), "(" + result->inspect() + ")");
      else return result.detach();
    }
    else if (auto inner = node->isaStringExpression()) {
      if (inner->isCalcSafe()) {
        // if (node.isCalculationSafe)
        // assert(!nod_visitCalculationExpressione.hasQuotes);
        sass::string text(inner->text()->getPlainString());
        StringUtils::makeLowerCase(text);
        if (text == str_pi) return SASS_MEMORY_NEW(Number, inner->pstate(), Constants::Math::C_PI);
        else if (text == str_e) return SASS_MEMORY_NEW(Number, inner->pstate(), Constants::Math::C_E);
        else if (text == str_infinity) return SASS_MEMORY_NEW(Number, inner->pstate(), std::numeric_limits<double>::infinity());
        else if (text == str_neg_infinity) return SASS_MEMORY_NEW(Number, inner->pstate(), -std::numeric_limits<double>::infinity());
        else if (text == str_nan) return SASS_MEMORY_NEW(Number, inner->pstate(), std::numeric_limits<double>::quiet_NaN());
        else { return SASS_MEMORY_NEW(String, inner->pstate(), acceptInterpolation(inner->text(), false), false); }
      }
      else {
        CallStackFrame frame(logger, inner->pstate());
        throw Exception::SassScriptException(
          "This expression can't be used in a calculation.",
          logger, inner->pstate());
      }
    }
    else if (node->isaNumberExpression()
      || node->isaVariableExpression()
      || node->isaFunctionExpression()
      || node->isaIfExpression())
      {
        //std::cerr << "Process expression\n";
        ValueObj result = node->accept(this);
        if (result->isaNumber()) {
          return result.detach();
        }
        if (result->isaCalculation()) {
          return result.detach();
        }
        if (auto str = result->isaString()) {
          if (str->hasQuotes() == false)
            return result.detach();
        }
        //std::cerr << "cant be used in calculon\n";
    }
    else if (auto inner = node->isaBinaryOpExpression()) {

      if (inner->isCalcSafeOp() == false) {
        if (inner->operand() == SassOperator::ADD)
          throw Exception::OpNotCalcSafe(logger, inner);
        if (inner->operand() == SassOperator::SUB)
          throw Exception::OpNotCalcSafe(logger, inner);
      }

      CallStackFrame frame(logger, inner->pstate());
      if (inner->operand() != ADD && inner->operand() != SUB) {
        if (inner->operand() != MUL && inner->operand() != DIV) {
          // Optimize to report span at operator
          throw Exception::SassScriptException(
            "This operation can't be used in a calculation.",
            compiler, inner->pstate());
        }
      }
      // Evaluate the arguments first in case second one throws an error
      ValueObj lhs(_visitCalculationExpression(inner->left(), inLegacySassFunction));
      ValueObj rhs(_visitCalculationExpression(inner->right(), inLegacySassFunction));
      return Calc::operate(logger, inner->pstate(), inner->operand(),
        lhs, rhs, inLegacySassFunction, !inSupportsDeclaration);
    }
    else {
      const ListExpression* list = node->isaListExpression();
      if (list && !list->hasBrackets() && list->separator() == SASS_SPACE && list->size() > 1) {
        sass::vector<ValueObj> elements;
        for (const auto& child : list->items()) {
          elements.push_back(_visitCalculationExpression(child, inLegacySassFunction));
        }

        _checkAdjacentCalculationValues(elements, list);

        for (size_t i = 0; i < elements.size(); i++) {
          if (elements[i]->isaCalcOperation()) {
            if (list->items()[i]->isaParenthesizedExpression()) {
              sass::string value("(" + elements[i]->inspect() + ")");
              elements[i] = SASS_MEMORY_NEW(String,
                elements[i]->pstate(), std::move(value));
            }
          }
        }

        sass::string joined;
        for (size_t i = 0; i < elements.size(); i++) {
          if (i != 0) joined += " ";
          joined += elements[i]->inspect();
        }
        return SASS_MEMORY_NEW(String,
          list->pstate(), std::move(joined));
      }
      else {
        CallStackFrame frame(logger, node->pstate());
        throw Exception::SassScriptException(
          "This expression can't be used in a calculation.",
          logger, node->pstate());
      }
    }
    return node->accept(this);
  }

  void Eval::_checkCalculationArguments(Calc::CFN fn, FunctionExpression* node, size_t maxArgs)
  {
    if (node->arguments()->positional().empty()) {
      CallStackFrame frame(logger, node->pstate());
      if (Calc::isSimpleTrigonometry(fn)) {
      // if (name == "sin" || name == "cos" || name == "tan") {
        throw Exception::SassScriptException(logger,
          node->pstate(), "Missing argument $angle.");
      }
      else if (maxArgs == 0) {
        throw Exception::MustHaveArguments(logger, Calc::ToString(fn));
      }
      else {
        throw Exception::MissingArgument(logger, "number");
      }
    }
    size_t size = node->arguments()->positional().size();
    if (maxArgs != 0 && size > maxArgs) {
      sass::sstream msg;
      msg << "Only " << maxArgs << " ";
      msg << pluralize("argument", maxArgs);
      msg << " allowed, but " << size;
      msg << pluralize(" was", size, " were");
      msg << " passed.";
      CallStackFrame frame(logger, node->pstate());
      throw Exception::SassScriptException(
        logger, node->pstate(), msg.str());
    }

  }

  void Eval::_checkCalculationArguments(Calc::CFN fn, FunctionExpression* node)
  {
    _checkCalculationArguments(fn, node,
      Calc::getArgumentsLength(fn));
  }

  // Name is already in lowercase (original name can be found on function node)
  Value* Eval::visitCalcuation(Calc::CFN fn, FunctionExpression* node, bool inLegacySassFunction)
  {

    if (node->arguments()->hasNamed()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::SassScriptException(logger, node->pstate(),
        "Keyword arguments can't be used with calculations.");
    }
    else if (node->arguments()->restArg() != nullptr) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::SassScriptException(logger, node->pstate(),
        "Rest arguments can't be used with calculations.");
    }

    _checkCalculationArguments(fn, node);

    const ExpressionVector& args(node->arguments()->positional());
    ValueVector arguments(args.size()); // pre-init
    // Transform and apply calculation visitor
    std::transform(args.begin(), args.end(),
      arguments.begin(), [&](const ExpressionObj& arg) {
        return _visitCalculationExpression(arg, inLegacySassFunction);
      });

    if (inSupportsDeclaration) {
      // Must slice items to subtype
      sass::vector<AstNodeObj> inputs;
      inputs.reserve(arguments.size());
      inputs.insert(inputs.end(),
        std::make_move_iterator(arguments.begin()),
        std::make_move_iterator(arguments.end()));
      return new Calculation(node->pstate(),
        node->name(), std::move(inputs));
    }

    // Add logger in case of error for reporting
    BackTrace trace(node->pstate(), Calc::ToString(fn), true);
    CallStackFrame frame(logger, trace, false);

    // Execute the calculation function now
    return Sass::Calc::execute(
      logger, node->pstate(), fn, arguments);
  }

  //*************************************************//
  // Helper to Execute/include a mixin (for meta apply)
  //*************************************************//

  Value* Eval::applyMixin(
    const SourceSpan& pstate, const EnvKey& name,
    Callable* callable,
    CallableDeclaration* ctblk,
    CallableArguments* arguments)
  {

    //std::cerr << "ApplyMixin: " << callable->name() << "\n";
    // debug_ast(ctblk);

    // 99% of all mixins are user defined (expect `load-css`)
    if (auto mixin = callable->isaUserDefinedCallable()) {

      // An include expression must reference a mixin rule
      MixinRule* rule = mixin->declaration()->isaMixinRule();

      // Sanity assertion
      if (rule == nullptr) {
        throw Exception::RuntimeException(logger,
          "Include doesn't reference a mixin!");
      }

      // Create new mixin for content block
      // Prepares the content block to be called later
      // Content blocks of includes are like mixins themselves
      UserDefinedCallableObj cmixin;

      // Check if a content block was passed to include
      if (ctblk != nullptr) {
        // Create a new temporary mixin
        // Attach current content block to it in order
        // for it to being restored when it is invoked.
        cmixin = SASS_MEMORY_NEW(UserDefinedCallable,
          pstate, name, ctblk, content);
        // Check if invoked mixin accepts a content block
        if (!rule->hasContent()) {
          CallStackFrame frame(logger, ctblk->pstate());
          throw Exception::RuntimeException(logger,
            "Mixin doesn't accept a content block.");
        }
      }

      // Change lexical status (RAII)
      // Influences e.g. `content-exists`
      RAII_FLAG(inMixin, true);

      // Add a special backtrace for include invocation
      CallStackFrame frame(logger, BackTrace(
        pstate, mixin->envkey().orig(), true));

      // Overwrite current content block mixin with new one
      // Even overwrite it if no new content block was given
      RAII_PTR(UserDefinedCallable, content, cmixin);

      // Return value can be ignored, but memory must still be collected
      return _runUserDefinedCallable(arguments, mixin, pstate);

    }
    // This is currently only used for `load-css` mixin
    else if (auto builtin = callable->isaBuiltInCallable()) {

      // An include expression must reference a mixin rule
      // MixinRule* rule = mixin->declaration()->isaMixinRule();

      // Create new mixin for content block
      // Prepares the content block to be called later
      // Content blocks of includes are like mixins themselves
      UserDefinedCallableObj cmixin;

      // Check if a content block was passed to include
      if (ctblk != nullptr) {
        // Create a new temporary mixin
        // Attach current content block to it in order
        // for it to being restored when it is invoked.
        cmixin = SASS_MEMORY_NEW(UserDefinedCallable,
          pstate, name, ctblk, content);

        if (!builtin->acceptsContent()) {
          CallStackFrame frame2(logger, BackTrace(
            pstate, cmixin->envkey().orig(), true));
          CallStackFrame frame(logger, ctblk->pstate());
          throw Exception::RuntimeException(logger,
            "Mixin doesn't accept a content block.");
        }
      }

      // Change lexical status (RAII)
      // Influences e.g. `content-exists`
      RAII_FLAG(inMixin, true);

      // Overwrite current content block mixin with new one
      // Even overwrite it if no new content block was given
      RAII_PTR(UserDefinedCallable, content, cmixin);

      // Return value can be ignored, but memory must still be collected
      return builtin->execute(*this, arguments, pstate);

    }

    throw Exception::RuntimeException(compiler,
      "Mixin has no callable associated.");

  }

  //*************************************************//
  // Execute/include a mixin (return value must be collected)
  // Cache dynamic lookup results in `midx` member
  //*************************************************//
  Value* Eval::visitIncludeRule(IncludeRule* include)
  {

    if (StringUtils::startsWith(include->name().orig(), "--", 2) /* dart has some more conditions */) {
      compiler.addDeprecation(
        "Sass @mixin names beginning with -- are deprecated for forward-"
        "compatibility with plain CSS mixins.\n"
        "For details, see https://sass-lang.com/d/css-function-mixin",
        include->span(), Logger::WARN_DOUBLE_DASH_MIXIN);
    }

    // Check if mixin expression was already resolved
    if (!include->midx().isValid()) {
      // Try to fetch the mixin by finding it by name
      include->midx(compiler.varRoot.findMixIdx(include->name(), include->ns()));
    }

    // Check if function expressions is resolved now
    // If not the expression is a regular css function
    if (include->midx().isValid()) {
      // Check if mixin is already defined on the frame/scope
      // Can fail if the mixin definition comes after the usage
      if (Callable* callable = compiler.varRoot.getMixin(include->midx())) {
        // CallStackFrame frame(logger, include->pstate(), true);
        ValueObj value = applyMixin(include->pstate(), include->name(),
          callable, include->content(), include->arguments());
        return nullptr;
      }
    }

    // If we reach this point we have an error
    // Mixin wasn't found and couldn't be executed
    CallStackFrame frame(logger, include->pstate());

    if (!include->midx().isValid()) {
      // If we reach this point we have an error
      // Mixin wasn't found and couldn't be executed
      // CallStackFrame frame(logger, include->pstate());
      throw Exception::RuntimeException(logger, "Undefined mixin.");
    }

    // Check if function was requested from a module and if that module actually exists
    if (include->ns().empty() || compiler.envstack.back()->hasNameSpace(include->ns())) {
      throw Exception::RuntimeException(logger, "Undefined mixin.");
    }

    // Otherwise the module simply wasn't imported
    throw Exception::ModuleUnknown(logger, include->ns());

  }
  // EO visitIncludeRule

  //*************************************************//
  // See visitContentRule and visitIncludeRule
  //*************************************************//
  Value* Eval::visitContentBlock(ContentBlock* rule)
  {
    throw std::runtime_error("Evaluation handles "
      "@include and its content block together.");
  }

  //*************************************************//
  // Invoke the current block mixin (if available)
  //*************************************************//
  Value* Eval::visitContentRule(ContentRule* c)
  {
    // Check if no content block can be called
    // This is no error by design, just ignore
    if (content == nullptr) return nullptr;

    // Get local reference to current content block
    UserDefinedCallable* current = content;

    // Reset lexical status (RAII)
    // Influences e.g. `content-exists`
    RAII_FLAG(inMixin, false);

    // Add a special backtrace for include invocation
    CallStackFrame frame(logger, BackTrace(
      c->pstate(), Strings::contentRule));

    // Reset lexical pointer for current content block to the
    // content block where the now invoked mixin was seen. This
    // allows the content calls to be "wrapped recursively".
    // Note: could have been implemented with a regular stack.
    RAII_PTR(UserDefinedCallable, content, current->content());

    // Execute the callable and return value which must be collected
    return _runUserDefinedCallable(c->arguments(), current, c->pstate());

  }
  // EO visitContentRule

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  //*************************************************//
  //*************************************************//
  void Eval::callExternalMessageOverloadFunction(Callable* fn, Value* message)
  {
    // We know that warn override function can only be external
    SASS_ASSERT(fn->isaExternalCallable(), "Custom callable must be external");
    ExternalCallable* def = static_cast<ExternalCallable*>(fn);
    SassFunctionLambda lambda = def->lambda();
    struct SassValue* c_args = sass_make_list(SASS_COMMA, false);
    sass_list_push(c_args, Value::wrap(message));
    struct SassValue* c_val = lambda(
      c_args, compiler.wrap(), def->cookie());
    sass_delete_value(c_args);
    sass_delete_value(c_val);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Value* Eval::visitDebugRule(DebugRule* node)
  {
    ValueObj message = node->expression()->accept(this);
    EnvRef fidx = compiler.varRoot.findFnIdx(Keys::debugRule, "");
    if (fidx.isValid()) {
      CallableObj& fn = compiler.varRoot.getFunction(fidx);
      callExternalMessageOverloadFunction(fn, message);
    }
    else {
      logger.addDebug(message->
        inspect(compiler.precision, false),
        node->pstate());
    }
    return nullptr;
  }

  Value* Eval::visitWarnRule(WarnRule* node)
  {
    ValueObj message = node->expression()->accept(this);
    EnvRef fidx = compiler.varRoot.findFnIdx(Keys::warnRule, "");
    if (fidx.isValid()) {
      CallableObj& fn = compiler.varRoot.getFunction(fidx);
      callExternalMessageOverloadFunction(fn, message);
    }
    else {
      sass::string result(message->toCss(false));
      CallStackFrame frame(logger, BackTrace(node->pstate()));
      logger.addWarning(result, Logger::WARN_RULE);
    }
    return nullptr;
  }

  Value* Eval::visitErrorRule(ErrorRule* node)
  {
    ValueObj message = node->expression()->accept(this);
    EnvRef fidx = compiler.varRoot.findFnIdx(Keys::errorRule, "");
    if (fidx.isValid()) {

      CallableObj& fn = compiler.varRoot.getFunction(fidx);
      callExternalMessageOverloadFunction(fn, message);
    }
    else {
      sass::string result(message->inspect());
      logger.callStack.push_back(BackTrace(node->pstate()));
      throw Exception::RuntimeException(logger, result);
    }
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Value* Eval::visitStyleRule(StyleRule* node)
  {

    if (!declarationName.empty()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Style rules may not be used within nested declarations.");
    }
    else if (inKeyframes && current->isaCssKeyframeBlock()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Style rules may not be used within keyframe blocks.");
    }

    // Create a scope for lexical block variables
    EnvScope scope(compiler.varRoot, node->idxs);

    //  std::cerr << "EVAL FOR " << current->toString() << " => " << current->fromPlainCss() << "\n";

    bool nest = current ? !current->fromPlainCss() : true;
    // bool nest = current ? !current->fromPlainCss() : true;
    bool nesting = current ? !current->fromPlainCss() : true;

    // Keyframe blocks have a specific syntax inside them
    // Therefore style rules render a bit different inside them
    if (inKeyframes) {
      // Find the parent we should append to (bubble up)
      CssParentNode* chroot = current; // ->bubbleThrough(true);
      if (nest) chroot = chroot->bubbleThrough(true);
      // Create a new keyframe parser from the evaluated interpolation
      KeyframeSelectorParser parser(compiler, SASS_MEMORY_NEW(SourceItpl,
        node->interpolation()->pstate(),
        acceptInterpolation(node->interpolation(), true, true)));
      // Invoke the keyframe parser and create a new CssKeyframeBlock
      CssKeyframeBlockObj child = SASS_MEMORY_NEW(CssKeyframeBlock, node->pstate(),
        chroot, parser.parse());
      // Add child to our parent
      chroot->addChildAt(child, false);
      // addChildAt(chroot, child);
      // Visit the remaining items at new child
      acceptChildrenAt(child, node);
    }
    // Regular style rule
    else if (node->interpolation()) {
      // Check current importer context
      Import* imp = compiler.import_stack.back();
      // bool plainCss = imp->syntax == SASS_IMPORT_CSS;
      bool wasCss = imp->syntax == SASS_IMPORT_CSS;
      // Evaluate the interpolation and try to parse a selector list
      SelectorListObj slist = interpolationToSelector(node->interpolation(), wasCss);

      if (nesting && wasCss) {
        if (_stylesheet->import->syntax == SASS_IMPORT_CSS) {
          for (const auto& complex : slist->elements()) {
            if (!complex->leadingCombinators().empty()) {
              const auto& first = complex->leadingCombinators().front();
              CallStackFrame frame(logger, first->pstate());
              throw Exception::RuntimeException(logger,
                "Top-level leading combinators aren't allowed in plain CSS.");
            }
          }
        }
      }

      if (_styleRule() == nullptr && wasCss) {
        for (const auto& complex : slist->elements()) {
          if (!complex->leadingCombinators().empty()) {
            CallStackFrame frame(logger, slist->pstate());
            // throw Exception::RuntimeException(compiler, "Top-level leading combinators aren't allowed in plain CSS.");
          }
        }
      }

      if (nest)
      {
        slist = slist->resolveParentSelectors(original(), logger,
          !atRootExcludingStyleRule, false);
      }

      // Append new selector list to the stack
      RAII_SELECTOR(selectorStack, slist);
      // The copy is needed for parent reference evaluation
      // dart-sass stores it as `originalSelector` member
      RAII_SELECTOR(originalStack, SASS_MEMORY_COPY(slist));

      // Make the new selectors known for the extender
      // If previous extend rules match this selector it will
      // immediately do the extending, extend rules that occur
      // later will apply the extending to the existing ones.
      _extensionStore->addSelector(slist, mediaQueries);

      // Find the parent we should append to (bubble up)
      CssParentNode* chroot = current; // ->bubbleThrough(true);
      if (nest) chroot = chroot->bubbleThrough(true);
      // Create a new style rule at the correct parent
      CssStyleRuleObj child = SASS_MEMORY_NEW(CssStyleRule,
        node->pstate(), chroot, slist);
      child->fromPlainCss(wasCss);
      // Add child to our parent
      chroot->addChildAt(child, true);
      // Register new child as style rule
      RAII_PTR(CssStyleRule, readStyleRule, child);
      // Reset specific flag (not in an at-rule)
      RAII_FLAG(atRootExcludingStyleRule, false);

      // Visit the remaining items at child
      ValueObj rv = acceptChildrenAt(child, node);

      if (!child->isInvisibleOtherThanBogusCombinators()) {
        for (const auto& complex : slist->elements()) {
          if (!complex->isBogusStrict()) continue;

          if (complex->isUseless()) {
            logger.addDeprecation("The selector \""
                + complex + "\" is invalid CSS.\n"
              "It will be omitted from the generated CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_USELESS);
          }
          else if (!complex->leadingCombinators().empty()) {
            if (!wasCss) {
            logger.addDeprecation("The selector \""
                + complex + "\" is invalid CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_ERROR);
            }
          }
          else if (complex->isBogusOtherThanLeadingCombinator()) {
            logger.addDeprecation("The selector \"" + complex + "\" "
              "is only valid for nesting\nIt shouldn't "
              "have children other than style rules.\n"
              "It will be omitted from the generated CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_BOGUS);
          }
          else {
            logger.addDeprecation("The selector \"" + complex + "\" "
              "is only valid for nesting\nIt shouldn't "
              "have children other than style rules.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_BOGUS);
          }
        }
      }
      return rv.detach();
    }
    // Consumed node
    return nullptr;
  }
  // EO visitStyleRule



  /// Returns the index of the first node in [statements] that comes after all
/// static imports.
  size_t Eval::_indexAfterImports(sass::vector<CssNodeObj> statements) {
    size_t lastImport = -1;

    for (size_t i = 0; i < statements.size(); i++) {
      if (statements[i]->isaCssImport()) {
        lastImport = i;
      }
      else if (statements[i]->isaCssComment()) {
        continue;
      }
      else {
        break;
      }
    }
    return lastImport + 1;
  }


  void Eval::_visitUpstreamModule(Stylesheet* current, sass::vector<Stylesheet*>& sorted, std::set<sass::string>& seen, CssRoot* css, sass::vector<CssNodeObj>& imports, bool clone)
  {
    // if (current->idxs->isImport) return;
    for (Stylesheet* upstream : current->upstream77) {
      // if (upstream->idxs->isImport) continue;
      if (upstream == nullptr) continue;
      if (seen.count(upstream->import->getAbsPath())) continue;
      seen.insert(upstream->import->getAbsPath()); // protected

      for (CssComment* head : upstream->precomments) {
        if (!css->empty()) css->append(head);
        else imports.push_back(head); 
      }
      _visitUpstreamModule(upstream, sorted, seen, css, imports, clone);
    }

    sorted.push_back(current);
    if (current->compiled) {
      CssParentNodeObj copy = current->compiled;
      auto& statements = copy->elements();
      auto index = _indexAfterImports(statements);
      sass::vector<CssNodeObj> rest;
      imports.insert(imports.end(), statements.begin(), statements.begin() + index);
      css->elements().insert(css->elements().end(), statements.begin() + index, statements.end());
    }

  }


  sass::vector<Stylesheet*> Eval::_topologicalModules(Stylesheet* root, CssRoot* css, sass::vector<CssNodeObj>& imports, bool clone)
  {
    // Construct a topological ordering using depth-first traversal, as in
    // https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search.
    std::set<sass::string> seen;
    sass::vector<Stylesheet*> sorted;
    // Probably more efficient to push and resort
    _visitUpstreamModule(root, sorted, seen, css, imports, clone);
    std::reverse(sorted.begin(), sorted.end());
    return sorted;
  }

  CssRoot* Eval::_combineCss(Stylesheet* root, bool clone)
  {
    CssRootObj mods = SASS_MEMORY_NEW(CssRoot, root->pstate());
    RAII_OBJ(CssParentNode, current, mods);
    RAII_PTR(ExtensionStore, _extensionStore, root->extender52);
    RAII_PTR(Stylesheet, _stylesheet, root);
    sass::vector<CssNodeObj> imports;
    auto sorted = _topologicalModules(root, mods, imports, clone);
    if (root->transitivelyContainsExtensions) _extendModules(sorted);
    mods->elements().insert(mods->elements().begin(), imports.begin(), imports.end());
    return mods.detach();
  }

  CssParentNode* Eval::hoistStyleRule(CssParentNode* node)
  {
    if (isInStyleRule()) {
      auto outer = SASS_MEMORY_RESECT(readStyleRule);
      node->addChildAt(outer, false);
      return outer;
    }
    else {
      return node;
    }
  }

  Value* Eval::visitSupportsRule(SupportsRule* node)
  {
    ValueObj condition = SASS_MEMORY_NEW(
      String, node->condition()->pstate(),
      _visitSupportsCondition(node->condition()));
    EnvScope envscope(compiler.varRoot, node->idxs);
    auto chroot = current->bubbleThrough(true);
    CssSupportsRuleObj css = SASS_MEMORY_NEW(CssSupportsRule,
      node->pstate(), chroot, condition);
    chroot->addChildAt(css, false);
    acceptChildrenAt(
      hoistStyleRule(css),
      node->elements());
    return nullptr;
  }

  CssParentNode* Eval::_trimIncluded(CssParentVector& nodes) const
  {

    CssParentNodeObj _root = getRoot();
    if (nodes.empty()) return _root;

    CssParentNode* parent = current;
    size_t innermostContiguous = sass::string::npos;
    for (size_t i = 0; i < nodes.size(); i++) {
      while (parent != nodes[i]) {
        innermostContiguous = sass::string::npos;
        parent = parent->parent();
      }
      if (innermostContiguous == sass::string::npos) {
        innermostContiguous = i;
      }
      parent = parent->parent();
    }

    if (parent != _root) return _root.detach();
    CssParentNode* root = nodes[innermostContiguous];
    nodes.resize(innermostContiguous);
    return root;

  }

  Value* Eval::visitAtRootRule(AtRootRule* node)
  {
    EnvScope envscope(compiler.varRoot, node->idxs);
    InterpolationObj itpl = node->query();
    AtRootQueryObj query;

    if (node->query()) {
      query = AtRootQuery::parse(
        interpolationToSource(
          node->query(), true),
        compiler);
    }
    else {
      query = AtRootQuery::defaultQuery(
        SourceSpan{ node->pstate() });
    }

    RAII_FLAG(inKeyframes, false);
    RAII_FLAG(inUnknownAtRule, false);
    RAII_FLAG(atRootExcludingStyleRule,
      query && query->excludesStyleRules());

    CssParentNode* parent = current;
    CssParentNode* orgParent = current;
    CssParentVector included;

    while (parent && parent->parent()) {
      // is!CssStylesheet (is!CssRootNode)
      if (!query->excludes(parent)) {
        included.emplace_back(parent);
      }
      parent = parent->parent();
    }
    CssParentNodeObj root = _trimIncluded(included);

    if (root.ptr() == orgParent) {
      acceptChildrenAt(root, node);
    }
    else {
      CssParentNode* innerCopy = included.empty() ?
        nullptr : SASS_MEMORY_RESECT(included.front());
      // if (innerCopy) innerCopy->clear();
      CssParentNode* outerCopy = innerCopy;
      auto it = included.begin();
      // Included is not empty
      if (it != included.end()) {
        if (++it != included.end()) {
          auto copy = SASS_MEMORY_RESECT(*it);
          copy->addChildAt(outerCopy, false);
          outerCopy = copy;
        }
      }

      if (outerCopy != nullptr) {
        root->addChildAt(outerCopy, false);
      }

      CssParentNode* newParent = innerCopy == nullptr ? root.ptr() : innerCopy;

      RAII_FLAG(inKeyframes, inKeyframes);
      RAII_FLAG(inUnknownAtRule, inUnknownAtRule);
      RAII_FLAG(atRootExcludingStyleRule, atRootExcludingStyleRule);
      CssMediaQueryVectorObj oldQueries = mediaQueries;

      if (query->excludesStyleRules()) {
        atRootExcludingStyleRule = true;
      }

      if (query->excludesMedia()) {
        mediaQueries = nullptr;
      }

      if (inKeyframes && query->excludesName("keyframes")) {
        inKeyframes = false;
      }

      if (inUnknownAtRule) {
        bool hasAtRuleInIncluded = false;
        for (auto& include : included) {
          // A flag on parent could save 1%
          if (include->isaCssAtRule()) {
            hasAtRuleInIncluded = true;
            break;
          }
        }
        if (!hasAtRuleInIncluded) {
          inUnknownAtRule = false;
        }
      }

      acceptChildrenAt(newParent, node);

      mediaQueries = oldQueries;

    }

    return nullptr;
  }

  Value* Eval::visitAtRule(AtRule* node)
  {
    sass::string name(interpolationToCssString(node->name(), true, false));
    sass::string value(interpolationToCssString(node->value(), true, true));

    if (node->empty()) {
      CssAtRuleObj css = SASS_MEMORY_NEW(CssAtRule,
        node->pstate(), current,
        std::move(name), std::move(value),
        node->isChildless());
      current->addChildAt(css, false);
      return nullptr;
    }

    EnvScope envscope(compiler.varRoot, node->idxs);

    sass::string normalized(StringUtils::unvendor(name));
    bool isKeyframe = normalized == "keyframes";
    RAII_FLAG(inUnknownAtRule, !isKeyframe);
    RAII_FLAG(inKeyframes, isKeyframe);


    auto pu = current->bubbleThrough(true);



    // ModifiableCssKeyframeBlock
    CssAtRuleObj css = SASS_MEMORY_NEW(CssAtRule,
      node->pstate(), pu,
      std::move(name),
      std::move(value),
      node->isChildless());

    // Adds new empty atRule to Root!
    pu->addChildAt(css, false);

    RAII_OBJ(CssParentNode, current, css);

    if (!(!atRootExcludingStyleRule && readStyleRule != nullptr) || inKeyframes || css->name() == "font-face") {

      for (const auto& child : node->elements()) {
        ValueObj val = child->accept(this);
      }


    }
    else {

      // If we're in a style rule, copy it into the at-rule so that
      // declarations immediately inside it have somewhere to go.
      // For example, "a {@foo {b: c}}" should produce "@foo {a {b: c}}".
      CssStyleRule* qwe = SASS_MEMORY_RESECT(readStyleRule);
      css->addChildAt(qwe, false);
      acceptChildrenAt(qwe, node->elements());

    }

    return nullptr;
  }

  bool Eval::BubbleMediaQuery(CssParentNode* node, CssMediaQueryVector& uses, bool chroot)
  {
    if (node->isaCssStyleRule()) return true; 
    if (chroot == false) return false;
    if (uses.empty()) return false;
    if (const auto& rule = node->isaCssMediaRule()) {
      if (rule->queries() == nullptr) return false;
      for (const auto& query : *rule->queries()) {
        return std::find_if(uses.begin(), uses.end(),
          [&](const CssMediaQueryObj& rhs) {
            return ObjEqualityFn(query, rhs);
          }) != uses.end();
      }
    }
    return false;
  }

  void Eval::_addChild(CssNode* node, bool(*through)(CssNode*)) const {
    CssParentNode* parent = current;
    while (through(parent)) {
      if (parent->parent() == nullptr) break;
      parent = parent->parent();
    }

    // If the parent has a (visible) following sibling, we shouldn't add to
    // the parent. Instead, we should create a copy and add it after the
    // interstitial sibling.
    //if (parent->hasFollowingSibling) {
    if (false) {
      // A node with siblings must have a parent
      CssParentNode* grandparent = parent->parent();
      if (parent->equalsIgnoringChildren(grandparent->elements().back())) {
        // If we've already made a copy of [parent] and nothing else has been
        // added after it, re-use it.
        parent = grandparent->elements().back()->isaCssParentNode();
      }
      else {
        parent = SASS_MEMORY_RESECT(parent);
        grandparent->append(parent);
      }
    }
    parent->append(node);
  }

  Value* Eval::visitMediaRule(MediaRule* node)
  {

    ExpressionObj mq;
    sass::string str_mq;
    const SourceSpan& state = node->query() ?
      node->query()->pstate() : node->pstate();
    EnvScope envscope(compiler.varRoot, node->idxs);
    if (node->query()) {
      str_mq = acceptInterpolation(node->query(), false);
    }

    bool bubbleQuery = true;

    MediaQueryParser parser(compiler, SASS_MEMORY_NEW(
      SourceItpl, state, std::move(str_mq)));

    CssMediaQueryVector uses;

    // Parse current media queries for local rule
    CssMediaQueryVectorObj parsed(parser.parse());

    CssMediaQueryVectorObj mergedQueries;

    if (!mediaQueries || mediaQueries->empty()) {
      mergedQueries = parsed;
      bubbleQuery = false;
    }
    else if (!parsed.isNull()) {

      mergedQueries =
      (mergeMediaQueries(mediaQueries, parsed, uses, bubbleQuery));

    }

    // Create a new CSS only representation of the media rule
    CssMediaRuleObj css = SASS_MEMORY_NEW(CssMediaRule,
      node->pstate(), current, mergedQueries);

    // auto chroot = current->bubbleThrough(true);
    CssParentNode* chroot = current;

    while (BubbleMediaQuery(chroot, uses, bubbleQuery)) {
      if (!chroot->parent()) break;
      chroot = chroot->parent();
    }


    // addChildAt(chroot, css);
    chroot->addChildAt(css, true);

    RAII_OBJ(CssParentNode, current, css);

    RAII_OBJ(CssMediaQueryVector, mediaQueries, mergedQueries);

    if (isInStyleRule()) {
      CssStyleRule* copy = SASS_MEMORY_RESECT(readStyleRule);
      css->addChildAt(copy, true);
      acceptChildrenAt(copy, node->elements());
    }
    else {
      for (auto& child : node->elements()) {
        ValueObj rv = child->accept(this);
      }
    }

    return nullptr;
  }

  Value* Eval::acceptChildren(const Vectorized<Statement>& children)
  {
    for (const auto& child : children) {
      ValueObj val = child->accept(this);
      if (val) return val.detach();
    }
    return nullptr;
  }

  Value* Eval::acceptChildren(const Vectorized<CssNode>& children)
  {
    for (const auto& child : children) {
      child->accept(this);
    }
    return nullptr;
  }

  Value* Eval::acceptChildrenAt(CssParentNode* parent,
    const Vectorized<Statement>& children)
  {
    RAII_OBJ(CssParentNode, current, parent);
    for (const auto& child : children) {
      ValueObj val = child->accept(this);
      if (val) return val.detach();
    }
    return nullptr;
  }

  Value* Eval::acceptChildrenAt(CssParentNode* parent,
    const Vectorized<CssNode>& children)
  {
    RAII_OBJ(CssParentNode, current, parent);
    for (const auto& child : children) {
      child->accept(this);
    }
    return nullptr;
  }

  /// Add parentheses if necessary.
  ///
  /// If [operator] is passed, it's the operator for the surrounding
  /// [SupportsOperation], and is used to determine whether parentheses are
  /// necessary if [condition] is also a [SupportsOperation].
  sass::string Eval::_parenthesize(SupportsCondition* condition) {
    SupportsNegation* negation = condition->isaSupportsNegation();
    SupportsOperation* operation = condition->isaSupportsOperation();
    SupportsAnything* anything = condition->isaSupportsAnything();
    if (negation != nullptr || operation != nullptr || anything != nullptr) {
      return "(" + _visitSupportsCondition(condition) + ")";
    }
    else {
      return _visitSupportsCondition(condition);
    }
  }

  sass::string Eval::_parenthesize(SupportsCondition* condition, SupportsOperation::Operand operand) {
    SupportsNegation* negation = condition->isaSupportsNegation();
    SupportsOperation* operation = condition->isaSupportsOperation();
    if (negation || (operation && operand != operation->operand())) {
      return "(" + _visitSupportsCondition(condition) + ")";
    }
    else {
      return _visitSupportsCondition(condition);
    }
  }

  /// Evaluates [condition] and converts it to a plain CSS string, with
  sass::string Eval::_visitSupportsCondition(SupportsCondition* condition)
  {
    if (SupportsOperation* operation = condition->isaSupportsOperation()) {
      sass::string strm;
      SupportsOperation::Operand operand = operation->operand();
      strm += _parenthesize(operation->left(), operand);
      strm += (operand == SupportsOperation::AND ? " and " : " or ");
      strm += _parenthesize(operation->right(), operand);
      return strm;
    }
    else if (SupportsNegation* negation = condition->isaSupportsNegation()) {
      return "not " + _parenthesize(negation->condition());
    }
    else if (SupportsInterpolation* interpolation = condition->isaSupportsInterpolation()) {
      return toCss(interpolation->value(), false);
    }
    else if (SupportsDeclaration* declaration = condition->isaSupportsDeclaration()) {
      RAII_FLAG(inSupportsDeclaration, true);
      return "(" + toCss(declaration->feature()) + ":"
          + (declaration->isCustomProperty() ? "" : " ")
          + toCss(declaration->value()) + ")";
    }
    else if (SupportsFunction* function = condition->isaSupportsFunction()) {
      return acceptInterpolation(function->name(), false)
        + "(" + acceptInterpolation(function->args(), false) + ")";
    }
    else if (SupportsAnything* anything = condition->isaSupportsAnything()) {
      return "(" + acceptInterpolation(anything->contents(), false) + ")";
    }
    else {
      return Strings::empty;
    }

  }

  /// Adds the values in [map] to [values].
  ///
  /// Throws a [RuntimeException] associated with [nodeForSpan]'s source
  /// span if any [map] keys aren't strings.
  ///
  /// If [convert] is passed, that's used to convert the map values to the value
  /// type for [values]. Otherwise, the [Value]s are used as-is.
  ///
  /// This takes an [AstNode] rather than a [FileSpan] so it can avoid calling
  /// [AstNode.span] if the span isn't required, since some nodes need to do
  /// real work to manufacture a source span.
  void Eval::_addRestValueMap(ArgumentResults& results, Map* map, const SourceSpan& pstate) {
    // convert ??= (value) = > value as T;

    for(const auto& kv : map->elements()) {
      if (String* str = kv.first->isaString()) {
        results.addNamed(str->value(), kv.second);
      }
      else {
        CallStackFrame frame(logger, pstate);
        throw Exception::RuntimeException(logger,
          "Variable keyword argument map must have string keys.\n" +
          kv.first->inspect() + " is not a string in " +
          map->inspect() + ".");
      }
    }
  }

  /// Adds the values in [map] to [values].
  void Eval::_addRestExpressionMap(CallableArguments* arguments, Map* map, const SourceSpan& pstate) {
    // convert ??= (value) = > value as T;

    for (const auto& kv : map->elements()) {
      if (String* str = kv.first->isaString()) {
        arguments->addNamed(str->value(), SASS_MEMORY_NEW(
          ValueExpression, map->pstate(), kv.second));
        // if (!values) values = SASS_MEMORY_NEW(ExpressionFlatMap);
        // values->insert(std::make_pair(str->value(), SASS_MEMORY_NEW(
        //   ValueExpression, map->pstate(), kv.second)));
      }
      else {
        CallStackFrame frame(logger, pstate);
        throw Exception::RuntimeException(logger,
          "Variable keyword argument map must have string keys.\n" +
          kv.first->inspect() + " is not a string in " +
          map->inspect() + ".");
      }
    }
  }


  CssMediaQueryVector* Eval::mergeMediaQueries(
    CssMediaQueryVector* lhs,
    CssMediaQueryVector* rhs,
    CssMediaQueryVector& uses,
    bool& valid)
  {
    CssMediaQueryVector queries;
    if (lhs == nullptr) return nullptr;
    if (rhs == nullptr) return nullptr;
    for (const CssMediaQueryObj& query1 : *lhs) {
      for (const CssMediaQueryObj& query2 : *rhs) {
        CssMediaQueryObj result(query1->merge(query2));
        if (result == nullptr) {
          valid = false;
          return rhs;
        }
        if (result && !result->empty()) {
          queries.emplace_back(result);
          uses.push_back(query1);
          uses.push_back(query2);
        }
        else {
          // return {};
        }
      }
    }
    return SASS_MEMORY_NEW(CssMediaQueryVector, std::move(queries));
  }


  Value* Eval::visitDeclaration(Declaration* node)
  {

    if (!isInStyleRule() && !inUnknownAtRule && !inKeyframes) {
      CallStackFrame csf(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Declarations may only be used within style rules.");
    }
    bool was_custom_property = node->isCustomProperty();
    if (!declarationName.empty() && was_custom_property) {
      CallStackFrame csf(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Declarations whose names begin with \"--\" may not be nested.");
    }

    sass::string name(interpolationToCssString(node->name(), true, false));

    // Apply BEM style selector aggregation
    if (!declarationName.empty()) {
      name = declarationName + "-" + name;
    }

    ValueObj cssValue;
    if (node->value()) {
      cssValue = node->value()->accept(this);
    }

    // If the value is an empty list, preserve it, because converting
    // it to CSS will throw an error that we want the user to see.
    if (cssValue != nullptr && (!cssValue->isBlank() || cssValue->lengthAsList() == 0))
    {
      // Only place we create css declarations!?
      current->append(SASS_MEMORY_NEW(CssDeclaration,
        node->pstate(), name, cssValue, was_custom_property));
    }
    else if (was_custom_property) {
      CallStackFrame frame(logger, node->value()->pstate());
      throw Exception::RuntimeException(logger,
        "Custom property values may not be empty.");
    }

    if (!node->empty()) {
      // Now produce the inner declarations
      LocalOption<sass::string> ll1(declarationName, name);
      for (Statement* child : node->elements()) {
        ValueObj result = child->accept(this);
      }
    }

    return nullptr;
  }
  // EO visitDeclaration

  Value* Eval::visitLoudComment(LoudComment* c)
  {
    if (inFunction) return nullptr;

    // Comments are allowed to appear between CSS imports.
    if (current.ptr() == _stylesheet->compiled && _endOfImports == _stylesheet->compiled->size()) {
      _endOfImports++;
    }


    sass::string text(acceptInterpolation(c->text(), false));
    bool preserve = text[2] == '!';
    current->append(SASS_MEMORY_NEW(CssComment, c->pstate(), std::move(text), preserve));
    return nullptr;
  }

  Value* Eval::visitIfRule(IfRule* i)
  {
    ValueObj rv;
    // Has a condition?
    if (i->predicate()) {
      // Execute the condition statement
      ValueObj condition = i->predicate()->accept(this);
      // If true append all children of this clause
      if (condition->isTruthy()) {
        // Create local variable scope for children
        EnvScope envscope(compiler.varRoot, i->idxs);
        rv = acceptChildren(i);
      }
      else if (i->alternative()) {
        // If condition is falsy, execute else blocks
        rv = visitIfRule(i->alternative());
      }
    }
    else {
      EnvScope envscope(compiler.varRoot, i->idxs);
      rv = acceptChildren(i);
    }
    // Is probably nullptr!?
    return rv.detach();
  }

  // For does not create a new env scope
  // But iteration vars are reset afterwards
  Value* Eval::visitForRule(ForRule* f)
  {
    BackTrace trace(f->pstate(), Strings::forRule);
    EnvScope envscope(compiler.varRoot, f->idxs);
    ValueObj low = f->lower_bound()->accept(this);
    ValueObj high = f->upper_bound()->accept(this);
    NumberObj sass_start = low->assertNumber(logger, "");
    NumberObj sass_end = high->assertNumber(logger, "");
    // Support compatible unit types (e.g. cm to mm)
    sass_end = sass_end->coerce(logger, sass_start);
    // Can only use integer ranges
    sass_start->assertInt(logger);
    sass_end->assertInt(logger);
    // check if units are valid for sequence
    if (sass_start->unit() != sass_end->unit()) {
      CallStackFrame csf(logger, f->pstate());
      throw Exception::UnitMismatch(
        logger, sass_start, sass_end);
    }
    double start = sass_start->value();
    double end = sass_end->value();
    // only create iterator once in this environment
    ValueObj val;
    if (start < end) {
      if (f->is_inclusive()) ++end;
      for (double i = start; i < end; ++i) {
        NumberObj it = SASS_MEMORY_NEW(Number,
          low->pstate(), i, sass_end->unit());
        compiler.varRoot.setVariable(
          { f->idxs, 0 }, it.ptr(), false);
        val = acceptChildren(f);
        if (val) break;
      }
    }
    else {
      if (f->is_inclusive()) --end;
      for (double i = start; i > end; --i) {
        NumberObj it = SASS_MEMORY_NEW(Number,
          low->pstate(), i, sass_end->unit());
        compiler.varRoot.setVariable(
          { f->idxs, 0 }, it.ptr(), false);
        val = acceptChildren(f);
        if (val) break;
      }
    }
    return val.detach();
  }

  Value* Eval::visitExtendRule(ExtendRule* e)
  {
   // std::cerr << "+++ EXTEND RULE " << selector()->inspect() << "\n";
    //std::cerr << "Visit extend\n";
    if (!isInStyleRule() /* || !declarationName.empty() */) {
      CallStackFrame csf(logger, e->pstate());
      throw Exception::RuntimeException(logger,
        "@extend may only be used within style rules.");
    }

    for (const auto& complex : readStyleRule->selector()->elements())
    {
      if (!complex->isBogusStrict()) continue;
      if (complex->isUseless()) {
        logger.addDeprecation("The selector \""
          + complex + "\" is invalid CSS.\n"
          "Therefore, it can't be an extender.\n"
          "This will be an error in LibSass 5.0.0.\n\n"
          "More info: https://sass-lang.com/d/bogus-combinators",
          complex->pstate(), Logger::WARN_SEL_USELESS_EXTEND);
      }
      else {
        logger.addDeprecation("The selector \""
          + complex + "\" is invalid CSS.\n"
          "Therefore, it shouldn't be an extender.\n"
          "This will be an error in LibSass 5.0.0.\n\n"
          "More info: https://sass-lang.com/d/bogus-combinators",
          complex->pstate(), Logger::WARN_SEL_USELESS_EXTEND);
      }
    }

    SelectorListObj slist = interpolationToSelector(
      e->selector(), plainCss, current == nullptr);

    // std::cerr << "visit extend [" << slist->inspect() << "]\n";

    if (slist) {

      for (const auto& complex : slist->elements()) {

        if (complex->size() != 1) {
          CallStackFrame csf(logger, complex->pstate());
          throw Exception::RuntimeException(logger,
            "complex selectors may not be extended.");
        }

        if (const CompoundSelector* compound = complex->first()->selector()) {

          if (compound->size() != 1) {

            sass::sstream sels; bool addComma = false;
            sels << "compound selectors may no longer be extended.\nConsider `@extend ";
            for (const auto& sel : compound->elements()) {
              if (addComma) sels << ", ";
              sels << sel->inspect();
              addComma = true;
            }
            sels << "` instead.\nSee https://sass-lang.com/d/extend-compound for details.";
            #if SassRestrictCompoundExtending
            CallStackFrame csf(logger, compound->pstate());
            throw Exception::RuntimeException(logger, sels.str());
            #else
            logger.addDeprecation(sels.str(), compound->pstate());
            #endif

            // Make this an error once deprecation is over
            for (SimpleSelectorObj simple : compound->elements()) {
              if (_extensionStore) _extensionStore->addExtension(selector(), simple, mediaQueries, e, e->is_optional());
              else std::cerr << "NO _extensionStore\n";
            }

          }
          else {
              if (_extensionStore) _extensionStore->addExtension(selector(), compound->first(), mediaQueries, e, e->is_optional());
              else std::cerr << "NO _extensionStore\n";
          }

        }
        else {
          CallStackFrame csf(logger, complex->pstate());
          throw Exception::RuntimeException(logger,
            "complex selectors may not be extended.");
        }
      }
    }

    return nullptr;
  }

  Value* Eval::visitEachRule(EachRule* e)
  {
    const EnvRefs* vidx(e->idxs);
    const sass::vector<EnvKey>& variables(e->variables());
    EnvScope envscope(compiler.varRoot, e->idxs);
    ValueObj expr = e->expressions()->accept(this);
    if (MapObj map = expr->isaMap()) {
      Map::ordered_map_type els(map->elements());
      for (const auto& kv : els) {
        ValueObj key = kv.first;
        ValueObj value = kv.second;
        if (variables.size() == 1) {
          List* variable = SASS_MEMORY_NEW(List,
            map->pstate(), { key, value }, SASS_SPACE);
          compiler.varRoot.setVariable({ vidx, 0 }, variable, false);
        }
        else {
          value = withoutSlash3(value);
          compiler.varRoot.setVariable({ vidx, 0 }, key, false);
          compiler.varRoot.setVariable({ vidx, 1 }, value, false);
        }
        ValueObj val = acceptChildren(e);
        if (val) return val.detach();
      }
      return nullptr;
    }

    ListObj list;
    if (List* slist = expr->isaList()) {
      list = SASS_MEMORY_NEW(List, expr->pstate(),
        slist->elements(), slist->separator());
      list->hasBrackets(slist->hasBrackets());
    }
    else {
      list = SASS_MEMORY_NEW(List, expr->pstate(),
        { expr }, SASS_COMMA);
    }
    for (size_t i = 0, L = list->size(); i < L; ++i) {
      Value* item = list->get(i);
      // check if we got passed a list of args (investigate)
      if (List* scalars = item->isaList()) { // Ex
        if (variables.size() == 1) {
          compiler.varRoot.setVariable({ vidx, 0 }, scalars, false);
        }
        else {
          for (size_t j = 0, K = variables.size(); j < K; ++j) {
            compiler.varRoot.setVariable({ vidx, (uint32_t)j },
              j < scalars->size() ? scalars->get(j)
              : SASS_MEMORY_NEW(Null, expr->pstate()), false);
          }
        }
      }
      else {
        if (variables.size() > 0) {
          compiler.varRoot.setVariable({ vidx, 0 }, item, false);
          for (size_t j = 1, K = variables.size(); j < K; ++j) {
            Value* res = SASS_MEMORY_NEW(Null, expr->pstate());
            compiler.varRoot.setVariable({ vidx, (uint32_t)j }, res, false);
          }
        }
      }
      ValueObj val = acceptChildren(e);
      if (val) return val.detach();
    }

    return nullptr;
  }

  Value* Eval::visitWhileRule(WhileRule* node)
  {

    // First condition runs outside
    EnvScope envscope(compiler.varRoot, node->idxs);
    Expression* condition = node->condition();
    ValueObj result = condition->accept(this);

    // Evaluate the first run in outer scope
    // All successive runs are from inner scope
    if (result->isTruthy()) {

      while (true) {
        result = acceptChildren(node);
        if (result) {
          return result.detach();
        }
        result = condition->accept(this);
        if (!result->isTruthy()) break;
      }

    }

    return nullptr;

  }

  Value* Eval::visitReturnRule(ReturnRule* rule)
  {
    ValueObj result(rule->value()->accept(this));
    return (result = withoutSlash3(result)).detach();
  }

  Value* Eval::visitSilentComment(SilentComment* c)
  {
    // current->append(c);
    return nullptr;
  }


  CssMediaQueryVector* Eval::evalMediaQueries(Interpolation* itpl)
  {
    SourceDataObj synthetic = interpolationToSource(itpl, true);
    MediaQueryParser parser(compiler, synthetic);
    return parser.parse();
  }

  void Eval::acceptStaticCssImport(StaticImport* rule)
  {
    // Create new CssImport object
    CssImportObj css = SASS_MEMORY_NEW(CssImport, rule->pstate(),
      interpolationToCssString(rule->url(), false, false),
      interpolationToCssString(rule->modifiers(), false, false));

    if (current != _stylesheet->compiled) {
      // std::cerr << "EVAL ADD IMPORT INTO SCOPE\n";
      current->append(css.ptr());
    }
    else if (_endOfImports == _stylesheet->compiled->size()) {
      // std::cerr << "EVAL ADD IMPORT TO END\n";
      _stylesheet->compiled->append(css.ptr());
      _endOfImports += 1;
    }
    else {
      // std::cerr << "EVAL ADD IMPORT OUT OF ORDER\n";
      _outOfOrderImports.push_back(css);
    }

    // _stylesheet->imports56.push_back(import);

//import->outOfOrder(rule->outOfOrder());
    if (rule->modifiers()) {
    //  if (auto supports = rule->supports()->isaSupportsDeclaration()) {
    //    sass::string feature(toCss(supports->feature()));
    //    sass::string value(toCss(supports->value()));
    //    import->supports(SASS_MEMORY_NEW(CssString,
    //      rule->supports()->pstate(),
    //      // Should have a CssSupportsCondition?
    //      // Nope, spaces are even further down
    //      feature + ": " + value));
    //  }
    //  else {
    //    import->supports(SASS_MEMORY_NEW(CssString, rule->supports()->pstate(),
    //      _visitSupportsCondition(rule->supports())));
    //  }
    //  // Wrap the resulting condition into a `supports()` clause
    //  import->supports()->text("supports(" + import->supports()->text() + ")");
    //
    //}
    //if (rule->media()) {
    //  import->media(evalMediaQueries(rule->media()));
    }
    // append new css import to result
//    current->append(import.ptr());

  }

  // Consume all imports in this rule
  Value* Eval::visitImportRule(ImportRule* rule)
  {
    for (const ImportBaseObj& import : rule->elements()) {
      // std::cerr << "Visit import rule " << typeid(*import).name() << "\n";
      if (StaticImport* stimp = import->isaStaticImport()) { acceptStaticCssImport(stimp); }
      else if (IncludeImport* stimp = import->isaIncludeImport()) { acceptDynamicSassImport(stimp); }
      else throw std::runtime_error("undefined behavior");
    }
    return nullptr;
  }

  /*#####################################################################*/
  /*#####################################################################*/


  Value* Eval::visitAssignRule(AssignRule* a)
  {

    // Optimize case where we know to what variable to assign to
    // This should potentially increase performance, but real-time
    // profiling only show a very minor increase (but keep anyway).
    if (a->vidx().isValid()) {
      assigne = &compiler.varRoot.getVariable(a->vidx());
      ValueObj result = a->value()->accept(this);
      compiler.varRoot.setVariable(a->vidx(),
        result, a->is_default());
      assigne = nullptr;
      return nullptr;
    }

    ValueObj result;

    const EnvKey& vname(a->variable());

    if (a->is_default()) {

      auto scope = compiler.getCurrentScope();

      // If we have a config and the variable is already set
      // we still overwrite the variable beside being guarded
      WithConfigVar* wconf = nullptr;
      if (wconfig && scope->isInternal && a->ns().empty()) {
        wconf = wconfig->getCfgVar(vname);
      }
      if (wconf) {
        // Via load-css
        if (wconf->value33) {
          if (!wconf->value33->isaNull())
          result = wconf->value33;
        }
        // Via regular load
        else if (wconf->expression44) {
          ValueObj val = wconf->expression44->accept(this);
          if (!val->isaNull()) result = val;
          //a->value(wconf->expression44);
        }
        a->is_default(wconf->isGuarded41);
      }
    }

    // Emit deprecation for new var with global flag
    if (a->is_global()) {

      auto rframe = compiler.envstack[0];
      auto it = rframe->varIdxs.find(a->variable());

      bool hasVar = false;

      if (it != rframe->varIdxs.end()) {
        EnvRef vidx(rframe, it->second);
        auto& value = compiler.varRoot.getVariable(vidx);
        if (value != nullptr) hasVar = true;
      }

      if (hasVar == false) {
        // libsass/variable-scoping/defaults-global-null
        // This check may not be needed, but we create a
        // superfluous variable slot in the scope
        for (auto& fwds : rframe->forwards) {
          auto it = fwds->varIdxs.find(a->variable());
          if (it != fwds->varIdxs.end()) {
            EnvRef vidx(it->second);
            auto& value = compiler.varRoot.getVariable(vidx);
            if (value != nullptr) hasVar = true;
          }

          auto fwd = fwds->module->mergedFwdVar.find(a->variable());
          if (fwd != fwds->module->mergedFwdVar.end()) {
            EnvRef vidx(fwd->second);
            auto& value = compiler.varRoot.getVariable(vidx);
            if (value != nullptr) hasVar = true;
          }
        }
      }

      if (hasVar == false) {

        // Check if we are at the global scope
        if (compiler.envstack.size() == 1) {
          logger.addDeprecation(
            "As of LibSass 5.0.0, !global assignments won't be able to declare new variables.\n"
            "\nSince this assignment is at the root of the stylesheet, the !global"
            " flag is unnecessary and can safely be removed.",
            a->pstate(), Logger::WARN_GLOBAL_ASSIGN);
        }
        else {
          logger.addDeprecation(
            "As of LibSass 5.0.0, !global assignments won't be able to declare new variables.\n"
            "\nRecommendation: add `$" + a->variable().orig() + ": null` at the stylesheet root.",
            a->pstate(), Logger::WARN_GLOBAL_ASSIGN_ROOT);
        }

      }

    }

    if (a->ns().empty()) {

      if (!a->is_global() && a->is_default()) {
        sass::vector<EnvRef> vidxs;
        if (compiler.envstack.size() > 0) {
          auto noda = compiler.envstack.back();
          //while (noda->isImport) noda = noda->pscope;
          noda->findVarIdxs(vidxs, a->variable());
          for (const auto& vidx : vidxs) {
            Value* value = compiler.varRoot.getVariable(vidx);
            //std::cerr << "Check var " << qwe.offset << " - " << asd << "\n";
            //if (asd != nullptr) std::cerr << " === " << asd->toString() << "\n";
            if (value == nullptr) continue;
            a->vidx(vidx);
            break;
          }
        }
      }
      if (!a->vidx().isValid()) {
        a->vidx(compiler.varRoot.findVarIdx(
          a->variable(), a->ns(), a->is_global()));
      }
      if (!a->vidx().isValid())
      {
        // Assignment must succeed!
        // Create variable if necessary!
        a->vidx(compiler.envstack.back()->createVariable(a->variable()));
        // CallStackFrame frame(logger, a->pstate());
        // throw Exception::RuntimeException(logger, "Undefined variable.");
      }
    //  std::cerr << "FOUND VARIABLE " << a->vidx().offset << "\n";
      //exit(1);
      assigne = &compiler.varRoot.getVariable(a->vidx());
      if (!result) result = a->value()->accept(this);
      if (result) result = withoutSlash3(result);
      compiler.varRoot.setVariable(
        a->vidx(),
        result,
        a->is_default());
      assigne = nullptr;

    }
    else {

      EnvRefs* mod = compiler.getCurrentModule();

      auto it = mod->module->moduse.find(a->ns());
      // if (it == )
      if (it == mod->module->moduse.end()) {
        // Access before module is loaded
        CallStackFrame csf(compiler, a->pstate());
        throw Exception::ModuleUnknown(compiler, a->ns());
      }
      else if (it->second.second && !it->second.second->isCompiled) {
        CallStackFrame csf(compiler, a->pstate());
        throw Exception::ModuleUnknown(compiler, a->ns());
      }

      if (!result) result = a->value()->accept(this);
      if (result) result = withoutSlash3(result);

      if (auto frame = compiler.getCurrentScope()) {
        a->vidx(frame->setModVar(
          a->variable(), a->ns(),
          result,
          a->is_default(),
          a->pstate()));
      }

    }

    if (!a->vidx().isValid())
    {
      if (a->ns().empty() || compiler.envstack.back()->hasNameSpace(a->ns())) {
        CallStackFrame frame(logger, a->pstate());
        throw Exception::RuntimeException(logger, "Undefined variable.");
      }
      else {
        CallStackFrame frame(logger, a->pstate());
        throw Exception::ModuleUnknown(logger, a->ns());
      }
    }

    return nullptr;
  }


  Stylesheet* Eval::_loadStylesheet(ModRule* rule)
  {

    // May not be defined yet
    Module* mod = rule->module32();

    // Nothing to be done for built-ins
    if (mod && mod->isBuiltIn) {
      return nullptr;
    }

    // Seems already loaded?
    if (rule->root47()) {
      return rule->root47();
    }

    RAII_PTR(WithConfig, wconfig, rule);

    auto sheet = loadModule(
      rule->prev51(), rule->url());
    rule->module32(sheet);
    rule->root47(sheet);

    return sheet;

  }

  Stylesheet* Eval::resolveIncludeImport(IncludeImport* rule)
  {
    // Seems already loaded?
    if (rule->root47()) {
      return rule->root47();
    }

    //if (rule->module32() && rule->module32()->isBuiltIn) {
    //  return nullptr;
    //}

    RAII_PTR(WithConfig, wconfig, rule);

    if (Stylesheet* sheet2 = loadModule(
      rule->prev51(),
      rule->url(),
      true
    )) {
      rule->module32(sheet2);
      rule->root47(sheet2);
      return sheet2;
    }

    return nullptr;

  }


  Stylesheet* Eval::loadModRule(ModRule* rule)
  {

    // May not be defined yet
    Module* mod = rule->module32();

    // Nothing to be done for built-ins
    if (mod && mod->isBuiltIn) {
      return nullptr;
    }

    // Seems already loaded?
    if (rule->root47()) {
      return rule->root47();
    }

    RAII_PTR(WithConfig, wconfig, rule);

    StylesheetObj sheet = loadModule(
      rule->prev51(), rule->url());

    rule->module32(sheet);
    rule->root47(sheet);

    return sheet;

  }


  // Called when loading an import (copy css)
  Stylesheet* Eval::loadModRule2(ModRule* rule)
  {

    // May not be defined yet
    Module* mod = rule->module32();

    // Nothing to be done for built-ins
    if (mod && mod->isBuiltIn) {
      return nullptr;
    }

    // Seems already loaded?
    if (rule->root47()) {
      return rule->root47();
    }

    RAII_PTR(WithConfig, wconfig, rule);

    StylesheetObj sheet = loadModule(
      rule->prev51(), rule->url());

    rule->module32(sheet);
    rule->root47(sheet);

    return sheet;

  }

  Stylesheet* Eval::loadModule(
    const sass::string& prev,
    const sass::string& url,
    bool isImport)
  {

    // Resolve final file to load
    const ImportRequest request(
      url, prev, false);

    // Search for valid imports (e.g. partials) on the file-system
    // Returns multiple valid results for ambiguous import path
    const sass::vector<ResolvedImport>& resolved(
      compiler.findIncludes(request, isImport));

    // Error if no file to import was found
    if (resolved.empty()) {
      throw Exception::UnknownImport(compiler);
    }
    // Error if multiple files to import were found
    else if (resolved.size() > 1) {
      throw Exception::AmbiguousImports(compiler, resolved);
    }

    // This is guaranteed to either load or error out!
    ImportObj loaded = compiler.loadImport(resolved[0]);
    ImportStackFrame iframe(compiler, loaded);

    sass::string abspath(loaded->getAbsPath());
    auto cached = compiler.sheets21.find(abspath);
    if (cached != compiler.sheets21.end()) {
      return cached->second;
    }

    // Permeable seems to have minor negative impact!?
    EnvFrame local(compiler, false, true, isImport); // correct
    Stylesheet* sheet = compiler.registerImport(loaded);
    sheet->idxs = local.idxs;
    sheet->import = loaded;
    return sheet;
  }

}
