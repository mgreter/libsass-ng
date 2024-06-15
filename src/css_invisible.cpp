/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "css_invisible.hpp"

#include "ast_css.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  IsCssInvisibleVisitor::IsCssInvisibleVisitor(
    bool includeBogus, bool includeComments) :
    includeBogus(includeBogus),
    includeComments(includeComments)
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool IsCssInvisibleVisitor::visitCssAtRule(CssAtRule * rule)
  {
    return false;
  }

  bool IsCssInvisibleVisitor::visitCssComment(CssComment* comment)
  {
    return includeComments && !comment->isPreserved();
  }

  bool IsCssInvisibleVisitor::visitCssStyleRule(CssStyleRule* rule)
  {
    if (includeBogus) {
      if (rule->selector()->isInvisible()) {
        return true;
      }
    }
    else {
      if (rule->selector()->isInvisibleOtherThanBogusCombinators()) {
        return true;
      }
    }
    return EveryCssVisitor::visitCssStyleRule(rule);
  }

  bool IsCssInvisibleVisitor::visitCssDeclaration(CssDeclaration* css)
  {
    return false;
  }

  bool IsCssInvisibleVisitor::visitCssImport(CssImport* css)
  {
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

