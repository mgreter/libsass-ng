/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_PARSER_CSS_HPP
#define SASS_PARSER_CSS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "parser_scss.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssParser : public ScssParser
  {
  public:

    // Value constructor
    CssParser(
      Compiler& context,
      SourceDataObj source) :
      ScssParser(context, source)
    {}

  protected:

    // Whether this is a plain CSS stylesheet.
    bool parsingCss() const final { return true; }

    // Consumes a plain-CSS `@import` rule that disallows
    // interpolation. [start] should point before the `@`.
    ImportRule* readImportRule(Offset start) override;

    // Expression* namespacedExpression(sass::string ns, Offset start);

    // Consumes an expression that starts like an identifier.
    Expression* readIdentifierLike() final;

    // Consume a silent comment and throws error
    SilentComment* readSilentComment() final;

    // Consume a silent comment and throws error
    bool scanSilentComment() final;

    // Parse allowed at-rule statement and parse children via [child_parser] parser function
    Statement* readAtRule(Statement* (StylesheetParser::* child_parser)(), bool root = false) final;

    Expression* readNamespacedExpression(const sass::string& ns, Offset start) final;

    Expression* readParenthesizedExpression() final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
