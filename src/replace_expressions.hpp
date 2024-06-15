/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_REPLACE_EXPRESSIONS_HPP
#define SASS_REPLACE_EXPRESSIONS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "visitor_expression.hpp"

namespace Sass {

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
