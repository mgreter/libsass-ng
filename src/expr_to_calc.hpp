/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_EXPR_TO_CALC_HPP
#define SASS_EXPR_TO_CALC_HPP

#include "visitor_expression.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ExpressionToCalc : public ReplaceExpressionVisitor {

  protected:

    virtual Expression* visitBinaryOpExpression(BinaryOpExpression* rule) override final;
    virtual Expression* visitItplFnExpression(ItplFnExpression* rule) override final;
    virtual Expression* visitUnaryOpExpression(UnaryOpExpression* rule) override final;

  public:

    static FunctionExpression* sanitize(Expression* expr);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
