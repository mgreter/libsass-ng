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

    virtual Expression* visitBinaryOpExpression(BinaryOpExpression*) override;
    virtual Expression* visitBooleanExpression(BooleanExpression*) override;
    virtual Expression* visitColorExpression(ColorExpression*) override;
    virtual Expression* visitItplFnExpression(ItplFnExpression*) override;
    virtual Expression* visitFunctionExpression(FunctionExpression*) override;
    virtual Expression* visitIfExpression(IfExpression*) override;
    virtual Expression* visitListExpression(ListExpression*) override;
    virtual Expression* visitMapExpression(MapExpression*) override;
    virtual Expression* visitNullExpression(NullExpression*) override;
    virtual Expression* visitNumberExpression(NumberExpression*) override;
    virtual Expression* visitParenthesizedExpression(ParenthesizedExpression*) override;
    virtual Expression* visitSelectorExpression(SelectorExpression*) override;
    virtual Expression* visitStringExpression(StringExpression*) override;
    virtual Expression* visitSupportsExpression(SupportsExpression*) override;
    virtual Expression* visitUnaryOpExpression(UnaryOpExpression*) override;
    virtual Expression* visitValueExpression(ValueExpression*) override;
    virtual Expression* visitVariableExpression(VariableExpression*) override;

    // Additional Helper (make virtual once we have a use for it)
    CallableArguments* visitCallableArguments(const CallableArguments* args);

    SupportsCondition* visitSupportsCondition(const SupportsCondition* args);

    Interpolation* visitInterpolation(const Interpolation* condition);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
