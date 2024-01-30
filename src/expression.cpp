#include "sel_any.hpp"

#include "ast_expressions.hpp"
#include "visitor_expression.hpp"

namespace Sass {


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  CallableArguments* ReplaceExpressionVisitor::visitCallableArguments(const CallableArguments* args)
  {
    ExpressionVector positional;
    for (const auto& pos : args->positional()) {
      positional.push_back(pos->accept(this));
    }
    ExpressionFlatMap named;
    for (const auto& kv : args->named()) {
      auto value = kv.second->accept(this);
      named.insert({ kv.first, value });
    }
    return SASS_MEMORY_NEW(CallableArguments, args->pstate(),
      std::move(positional), std::move(named),
      args->restArg() ? args->restArg()->accept(this) : nullptr,
      args->kwdRest() ? args->kwdRest()->accept(this) : nullptr);
  }

  SupportsCondition* ReplaceExpressionVisitor::visitSupportsCondition(const SupportsCondition* condition)
  {
    if (const SupportsOperation* op = condition->isaSupportsOperation())
    {
      SupportsConditionObj left = visitSupportsCondition(op->left());
      SupportsConditionObj right = visitSupportsCondition(op->right());
      return SASS_MEMORY_NEW(SupportsOperation, op->pstate(),
        std::move(left), std::move(right), op->operand());
    }
    else if (const SupportsNegation* neg = condition->isaSupportsNegation())
    {
      SupportsConditionObj cond(visitSupportsCondition(neg->condition()));
      return SASS_MEMORY_NEW(SupportsNegation, neg->pstate(), std::move(cond));
    }
    else if (const SupportsInterpolation* itpl = condition->isaSupportsInterpolation())
    {
      ExpressionObj expr(itpl->value()->accept(this));
      return SASS_MEMORY_NEW(SupportsInterpolation,
        itpl->pstate(), std::move(expr));

    }
    else if (const SupportsDeclaration* decl = condition->isaSupportsDeclaration()) {
      ExpressionObj feat(decl->feature()->accept(this));
      ExpressionObj expr(decl->value()->accept(this));
      return SASS_MEMORY_NEW(SupportsDeclaration,
        decl->pstate(), std::move(feat), std::move(expr));
    }
    // Otherwise we have a coding error
    throw "BUG: Unknown SupportsCondition";
  }

  Interpolation* ReplaceExpressionVisitor::visitInterpolation(const Interpolation* interpolation)
  {
    sass::vector<InterpolantObj> itpls;
    for (const auto& itpl : interpolation->elements()) {
      if (Expression* expr = itpl->isaExpression())
        itpls.emplace_back(expr->accept(this));
      else itpls.emplace_back(itpl);
    }
    return SASS_MEMORY_NEW(Interpolation,
      interpolation->pstate(), std::move(itpls));
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Expression* Sass::ReplaceExpressionVisitor::visitBinaryOpExpression(BinaryOpExpression* node)
  {
    return SASS_MEMORY_NEW(BinaryOpExpression,
      node->pstate(), node->operand(), node->opstate(),
      node->left()->accept(this), node->right()->accept(this));
  }

  Expression* ReplaceExpressionVisitor::visitBooleanExpression(BooleanExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitColorExpression(ColorExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitFunctionExpression(FunctionExpression* node)
  {
    CallableArgumentsObj arguments(visitCallableArguments(node->arguments()));
    return SASS_MEMORY_NEW(FunctionExpression, node->pstate(),
      node->name(), std::move(arguments), node->ns());
  }


  Expression* ReplaceExpressionVisitor::visitItplFnExpression(ItplFnExpression* node)
  {
    InterpolationObj name(visitInterpolation(node->itpl()));
    CallableArgumentsObj args(visitCallableArguments(node->arguments()));
    return SASS_MEMORY_NEW(ItplFnExpression, node->pstate(),
      std::move(name), std::move(args), node->ns());
  }

  Expression* ReplaceExpressionVisitor::visitIfExpression(IfExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitListExpression(ListExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitMapExpression(MapExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitNullExpression(NullExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitNumberExpression(NumberExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitParenthesizedExpression(ParenthesizedExpression* node)
  {
    return SASS_MEMORY_NEW(ParenthesizedExpression, node->pstate(), node->expression()->accept(this));
  }

  Expression* ReplaceExpressionVisitor::visitSelectorExpression(SelectorExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitStringExpression(StringExpression* node)
  {
    return SASS_MEMORY_NEW(StringExpression, node->pstate(), visitInterpolation(node->text()));
  }

  Expression* ReplaceExpressionVisitor::visitSupportsExpression(SupportsExpression* node)
  {
    return SASS_MEMORY_NEW(SupportsExpression, node->pstate(), visitSupportsCondition(node->condition()));
  }

  Expression* ReplaceExpressionVisitor::visitUnaryOpExpression(UnaryOpExpression* node)
  {
    return SASS_MEMORY_NEW(UnaryOpExpression, node->pstate(), node->optype(), node->operand()->accept(this));
  }

  Expression* ReplaceExpressionVisitor::visitValueExpression(ValueExpression* node)
  {
    return node;
  }

  Expression* ReplaceExpressionVisitor::visitVariableExpression(VariableExpression* node)
  {
    return node;
  }


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

