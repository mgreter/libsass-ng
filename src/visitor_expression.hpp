/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_VISITOR_EXPRESSION_HPP
#define SASS_VISITOR_EXPRESSION_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_fwd_decl.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
// An interface for [visitors][] that traverse SassScript expressions.
  // [visitors]: https://en.wikipedia.org/wiki/Visitor_pattern
  /////////////////////////////////////////////////////////////////////////

  template <typename T>
  class ExpressionVisitor {
  public:

    virtual T visitBinaryOpExpression(BinaryOpExpression*) = 0;
    virtual T visitBooleanExpression(BooleanExpression*) = 0;
    virtual T visitColorExpression(ColorExpression*) = 0;
    virtual T visitItplFnExpression(ItplFnExpression*) = 0;
    virtual T visitFunctionExpression(FunctionExpression*) = 0;
    virtual T visitIfExpression(IfExpression*) = 0;
    virtual T visitListExpression(ListExpression*) = 0;
    virtual T visitMapExpression(MapExpression*) = 0;
    virtual T visitNullExpression(NullExpression*) = 0;
    virtual T visitNumberExpression(NumberExpression*) = 0;
    virtual T visitParenthesizedExpression(ParenthesizedExpression*) = 0;
    virtual T visitSelectorExpression(SelectorExpression*) = 0;
    virtual T visitStringExpression(StringExpression*) = 0;
    virtual T visitSupportsExpression(SupportsExpression*) = 0;
    virtual T visitUnaryOpExpression(UnaryOpExpression*) = 0;
    virtual T visitValueExpression(ValueExpression*) = 0;
    virtual T visitVariableExpression(VariableExpression*) = 0;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ReplaceExpressionVisitor : public ExpressionVisitor<Expression*> {

  public:

    virtual Expression* visitBinaryOpExpression(BinaryOpExpression*) override = 0;
    virtual Expression* visitBooleanExpression(BooleanExpression*) override = 0;
    virtual Expression* visitColorExpression(ColorExpression*) override = 0;
    virtual Expression* visitItplFnExpression(ItplFnExpression*) override = 0;
    virtual Expression* visitFunctionExpression(FunctionExpression*) override = 0;
    virtual Expression* visitIfExpression(IfExpression*) override = 0;
    virtual Expression* visitListExpression(ListExpression*) override = 0;
    virtual Expression* visitMapExpression(MapExpression*) override = 0;
    virtual Expression* visitNullExpression(NullExpression*) override = 0;
    virtual Expression* visitNumberExpression(NumberExpression*) override = 0;
    virtual Expression* visitParenthesizedExpression(ParenthesizedExpression*) override = 0;
    virtual Expression* visitSelectorExpression(SelectorExpression*) override = 0;
    virtual Expression* visitStringExpression(StringExpression*) override = 0;
    virtual Expression* visitSupportsExpression(SupportsExpression*) override = 0;
    virtual Expression* visitUnaryOpExpression(UnaryOpExpression*) override = 0;
    virtual Expression* visitValueExpression(ValueExpression*) override = 0;
    virtual Expression* visitVariableExpression(VariableExpression*) override = 0;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  template <typename T>
  class ExpressionVisitable {
  public:
    virtual T accept(ExpressionVisitor<T>* visitor) = 0;
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
