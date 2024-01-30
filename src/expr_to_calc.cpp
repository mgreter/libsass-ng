#include "expr_to_calc.hpp"

#include "ast_expressions.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Expression* ExpressionToCalc::visitBinaryOpExpression(BinaryOpExpression* node)
  {
    if (node->operand() == SassOperator::MOD) {
      return new FunctionExpression(node->pstate(), "max", SASS_MEMORY_NEW(
        CallableArguments, node->pstate(), { node }, {}, {}), "math");
    }
    return ReplaceExpressionVisitor::visitBinaryOpExpression(node);
  }

  Expression* ExpressionToCalc::visitItplFnExpression(ItplFnExpression* node)
  {
    return node;
  }

  Expression* ExpressionToCalc::visitUnaryOpExpression(UnaryOpExpression* node)
  {
    if (node->optype() == UnaryOpType::PLUS) return node->operand();
    else if (node->optype() == UnaryOpType::MINUS) {
      return new BinaryOpExpression(node->pstate(),
        SassOperator::MUL, node->pstate(),
        SASS_MEMORY_NEW(NumberExpression, node->pstate(),
          SASS_MEMORY_NEW(Number, node->pstate(), - 1)),
        node->operand());
    }
    else return ReplaceExpressionVisitor::visitUnaryOpExpression(node);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static ExpressionToCalc visitor;

  FunctionExpression* ExpressionToCalc::sanitize(Expression* expr)
  {
    return SASS_MEMORY_NEW(FunctionExpression, expr->pstate(), "calc", SASS_MEMORY_NEW(
      CallableArguments, expr->pstate(), { expr->accept(&visitor) }, {}), "");
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

