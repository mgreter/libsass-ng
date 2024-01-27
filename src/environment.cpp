#include "fn_meta.hpp"

#include <cstring>

#include "eval.hpp"
#include "compiler.hpp"
#include "exceptions.hpp"
#include "ast_values.hpp"
#include "ast_callables.hpp"
#include "ast_expressions.hpp"
#include "string_utils.hpp"

#include "environment.hpp"
#include "preloader.hpp"

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

#include "parser_stylesheet.hpp"

#include "compiler.hpp"
#include "charcode.hpp"
#include "charcode.hpp"
#include "character.hpp"
#include "color_maps.hpp"
#include "exceptions.hpp"
#include "source_span.hpp"
#include "ast_imports.hpp"
#include "ast_supports.hpp"
#include "ast_statements.hpp"
#include "ast_expressions.hpp"
#include "parser_expression.hpp"

namespace Sass {

  // Import some namespaces
  using namespace Charcode;
  using namespace Character;
  using namespace StringUtils;




  AssignRule* StylesheetParser::readVariableDeclarationWithoutNamespace(
    const sass::string& ns, Offset start)
  {

    sass::string vname(variableName());

    // std::cerr << "ASSIGN == RULE\n";

    if (!ns.empty()) {
      assertPublicIdentifier(vname, start);
    }

    EnvKey name(vname);

    if (parsingCss()) {
      error("Sass variables aren't allowed in plain CSS.",
        scanner.relevantSpanFrom(start));
    }

    scanWhitespace();
    scanner.expectChar($colon);
    scanWhitespace();

    ExpressionObj value = readExpression();

    bool guarded = false;
    bool global = false;

    Offset flagStart(scanner.offset);
    while (scanner.scanChar($exclamation)) {
      sass::string flag = readIdentifier();
      if (flag == "default") {
        if (guarded) {
          compiler.addDeprecation(
            "!default should only be written once for each variable.\n"
            "This will be an error in LibSass 5.0.0.",
            scanner.relevantSpanFrom(flagStart),
            Logger::WARN_DUPE_VAR_FLAG);
        }
        guarded = true;
      }
      else if (flag == "global") {
        if (!ns.empty()) {
          error("!global isn't allowed for variables in other modules.",
            scanner.relevantSpanFrom(flagStart));
        }
        else if (global) {
          compiler.addDeprecation(
            "!global should only be written once for each variable.\n"
            "This will be an error in LibSass 5.0.0.",
            scanner.relevantSpanFrom(flagStart),
            Logger::WARN_DUPE_VAR_FLAG);
        }
        global = true;
      }
      else {
        error("Invalid flag name.",
          scanner.relevantSpanFrom(flagStart));
      }

      scanWhitespace();
      flagStart = scanner.offset;
    }

    expectStatementSeparator("variable declaration");

    // Skip to optional global scope
    EnvRefs* frame = global ?
      compiler.varRoot.stack.front() :
      compiler.varRoot.stack.back();

    SourceSpan pstate(scanner.relevantSpanFrom(start));

    bool hasVar = false;
    auto chroot = frame;
    while (chroot) {
      if (ns.empty()) {
        if (chroot->varIdxs.count(name)) {
          hasVar = true;
          break;
        }
      }
      if (/*chroot->isImport || */chroot->isSemiGlobal) {
        chroot = chroot->pscope;
      }
      else {
        break;
      }
    }

    AssignRule* declaration = SASS_MEMORY_NEW(AssignRule,
      scanner.relevantSpanFrom(start),
      name, ns,
      {}, value, guarded, global);

    if (ns.empty() && !hasVar) {
      frame->createVariable(name);
    }

    return declaration;
  }
  // EO readVariableDeclarationWithoutNamespace

  // Consumes a mixin declaration.
  // [start] should point before the `@`.
  MixinRule* StylesheetParser::readMixinRule(Offset start)
  {

    EnvRefs* frame = compiler.getCurrentScope();

    EnvFrame local(compiler, false);
    // Create space for optional content callable
    // ToDo: check if this can be conditionally done?
    local.idxs->createMixin(Keys::contentRule);
    // var precedingComment = lastSilentComment;
    // lastSilentComment = null;
    StringToken name = readIdentifierToken();

    if (StringUtils::startsWith(name.str, "--")) {
      compiler.addDeprecation(
        "Sass @mixin names beginning with -- are deprecated for forward-"
        "compatibility with plain CSS mixins.\n"
        "For details, see https://sass-lang.com/d/css-function-mixin",
        name.pstate, Logger::WARN_DOUBLE_DASH_MIXIN);
    }

    scanWhitespace();

    CallableSignatureObj arguments;
    if (scanner.peekChar() == $lparen) {
      arguments = parseArgumentDeclaration();
    }
    else {
      // Dart-sass creates this one too
      arguments = SASS_MEMORY_NEW(CallableSignature,
        scanner.relevantSpan(), sass::vector<ArgumentObj>()); // empty declaration
    }

    if (inMixin || inContentBlock) {
      error("Mixins may not contain mixin declarations.",
        scanner.relevantSpanFrom(start));
    }
    else if (inControlDirective) {
      error("Mixins may not be declared in control directives.",
        scanner.relevantSpanFrom(start));
    }

    scanWhitespace();
    RAII_FLAG(inMixin, true);
    RAII_FLAG(mixinHasContent, false);

    // while (frame->isImport) frame = frame->pscope;
    EnvRef midx = frame->createMixin(name.str);
    MixinRule* rule = withChildren<MixinRule>(
      &StylesheetParser::readChildStatement,
      start, name.str, arguments, local.idxs);
    // Mixins can't be created in loops
    // Must be on root, not even in @if
    // Therefore this optimization is safe
    rule->midx(midx);
    // rule->cidx(cidx);
    return rule;
  }
  // EO _mixinRule

  // Consumes a function declaration.
  // [start] should point before the `@`.
  FunctionRule* StylesheetParser::readFunctionRule(Offset start)
  {
    // Variables should not be hoisted through
    EnvRefs* parent = compiler.varRoot.stack.back();
    EnvFrame local(compiler, false);

    // var precedingComment = lastSilentComment;
    // lastSilentComment = null;
    Offset before(scanner.offset);
    StringToken name = readIdentifierToken();

    if (StringUtils::startsWith(name.str, "--")) {
      compiler.addDeprecation(
        "Sass @function names beginning with -- are deprecated for forward-"
        "compatibility with plain CSS mixins.\n"
        "For details, see https://sass-lang.com/d/css-function-mixin",
        name.pstate, Logger::WARN_DOUBLE_DASH_MIXIN);
    }

    sass::string normalized(name.str);

    scanWhitespace();

    CallableSignatureObj arguments = parseArgumentDeclaration();

    if (inMixin || inContentBlock) {
      error("Mixins may not contain function declarations.",
        scanner.relevantSpanFrom(start));
    }
    else if (inControlDirective) {
      error("Functions may not be declared in control directives.",
        scanner.relevantSpanFrom(start));
    }

    sass::string fname(StringUtils::unvendor(name.str));
    if (fname == "calc" || fname == "element" || fname == "expression" || fname == "url"
      || fname == "and" || fname == "or" || fname == "not" || fname == "clamp") {
      error("Invalid function name.",
        scanner.relevantSpanFrom(start));
    }

    scanWhitespace();
    FunctionRule* rule = withChildren<FunctionRule>(
      &StylesheetParser::readFunctionRuleChild,
      start, name.str, arguments, local.idxs);
    // This is the weird parts correspondant
    rule->fidx(parent->createFunction(name.str, true));
    return rule;
  }
  // EO readFunctionRule

  

}
