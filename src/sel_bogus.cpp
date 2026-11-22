/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "sel_bogus.hpp"

#include "ast_selectors.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool IsBogusVisitor::visitComplexSelector(ComplexSelector* complex)
  {
    const auto& elements = complex->elements();
    if (elements.empty()) {
      return !complex->leadingCombinators().empty();
    }
    else if (complex->leadingCombinators().size()
      > (includeLeadingCombinator ? 0UL : 1UL))
    {
      return true;
    }
    else if (!elements.back()->combinators().empty()) {
      return true;
    }
    else {
      // Like dart-sass `any`: check all components (not just the first)
      for (const auto& component : elements) {
        if (component->combinators().size() > 1) return true;
        if (component->selector()->accept(this)) return true;
      }
    }
    return false;
  }

  bool IsBogusVisitor::visitPseudoSelector(PseudoSelector* pseudo)
  {
    const auto& selector = pseudo->selector();
    if (selector.isNull()) return false;
    if (pseudo->name() != "has") return selector->isBogusStrict();
    else return selector->isBogusOtherThanLeadingCombinator();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Strict includes leading combinators (like `isBogusStrict`)
  const IsBogusVisitor IsBogusVisitorStrict(true);
  const IsBogusVisitor IsBogusVisitorLenient(false);

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

