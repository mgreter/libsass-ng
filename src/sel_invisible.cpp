/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "sel_invisible.hpp"

#include "ast_selectors.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  IsInvisibleVisitor::IsInvisibleVisitor(
    bool includeBogus):
    includeBogus(includeBogus)
  { }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool IsInvisibleVisitor::visitSelectorList(SelectorList* list)
  {
    for (const auto& complex : list->elements()) {
      if (!visitComplexSelector(complex)) return false;
    }
    return true;
  }

  bool IsInvisibleVisitor::visitComplexSelector(ComplexSelector* complex)
  {
    return AnySelectorVisitor::visitComplexSelector(complex) ||
      (includeBogus && complex->isBogusOtherThanLeadingCombinator());
  }

  bool IsInvisibleVisitor::visitPlaceholderSelector(PlaceholderSelector* placeholder)
  {
    return true;
  }

  bool IsInvisibleVisitor::visitPseudoSelector(PseudoSelector* pseudo)
  {
    if (const auto& selector = pseudo->selector()) {
      if (pseudo->name() != "not") return selector->accept(this); 
      else return includeBogus && selector->isBogusLenient();
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

