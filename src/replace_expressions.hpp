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

    Expression* visitBinaryOpExpression(BinaryOpExpression*) override;
    Expression* visitBooleanExpression(BooleanExpression*) override;
    Expression* visitColorExpression(ColorExpression*) override;
    Expression* visitItplFnExpression(ItplFnExpression*) override;
    Expression* visitFunctionExpression(FunctionExpression*) override;
    Expression* visitIfExpression(IfExpression*) override;
    Expression* visitListExpression(ListExpression*) override;
    Expression* visitMapExpression(MapExpression*) override;
    Expression* visitNullExpression(NullExpression*) override;
    Expression* visitNumberExpression(NumberExpression*) override;
    Expression* visitParenthesizedExpression(ParenthesizedExpression*) override;
    Expression* visitSelectorExpression(SelectorExpression*) override;
    Expression* visitStringExpression(StringExpression*) override;
    Expression* visitSupportsExpression(SupportsExpression*) override;
    Expression* visitUnaryOpExpression(UnaryOpExpression*) override;
    Expression* visitValueExpression(ValueExpression*) override;
    Expression* visitVariableExpression(VariableExpression*) override;

    // Additional Helper (make virtual once we have a use for it)
    CallableArguments* visitCallableArguments(const CallableArguments* args);
    SupportsCondition* visitSupportsCondition(const SupportsCondition* args);
    Interpolation* visitInterpolation(const Interpolation* condition);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
