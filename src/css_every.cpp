/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Base css visitor going through all children until one returns false
// Ensures all children are "true", used by `IsCssInvisibleVisitor`
/*****************************************************************************/
#include "css_every.hpp"

#include "ast_css.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Default implementation returning true on all accounts
  /////////////////////////////////////////////////////////////////////////

  bool Sass::EveryCssVisitor::visitCssAtRule(CssAtRule* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  bool EveryCssVisitor::visitCssComment(CssComment* css)
  {
    return true;
  }

  bool EveryCssVisitor::visitCssDeclaration(CssDeclaration* css)
  {
    return true;
  }

  bool EveryCssVisitor::visitCssImport(CssImport* css)
  {
    return true;
  }

  bool EveryCssVisitor::visitCssKeyframeBlock(CssKeyframeBlock* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  bool EveryCssVisitor::visitCssMediaRule(CssMediaRule* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  bool EveryCssVisitor::visitCssRoot(CssRoot* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  bool EveryCssVisitor::visitCssStyleRule(CssStyleRule* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  bool EveryCssVisitor::visitCssSupportsRule(CssSupportsRule* css)
  {
    for (const auto& child : css->elements()) {
      if (!child->accept(this)) return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

