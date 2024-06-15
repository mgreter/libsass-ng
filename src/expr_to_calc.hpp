/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_EXPR_TO_CALC_HPP
#define SASS_EXPR_TO_CALC_HPP

#include "replace_expressions.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ExpressionToCalc : public ReplaceExpressionVisitor {

  protected:

    virtual Expression* visitBinaryOpExpression(BinaryOpExpression* rule) override final;
    virtual Expression* visitUnaryOpExpression(UnaryOpExpression* rule) override final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
