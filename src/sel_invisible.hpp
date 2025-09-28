/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_SEL_INVISIBLE_HPP
#define SASS_SEL_INVISIBLE_HPP

#include "visitor_selector.hpp"

#include "sel_any.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class IsInvisibleVisitor : public AnySelectorVisitor {

    /// Whether to consider selectors with bogus combinators invisible.
    bool includeBogus;

    bool visitSelectorList(SelectorList* list) final;
    bool visitComplexSelector(ComplexSelector* complex) override;
    bool visitPlaceholderSelector(PlaceholderSelector* placeholder) final;
    bool visitPseudoSelector(PseudoSelector* pseudo) override;

  public:

    virtual ~IsInvisibleVisitor() {}

    IsInvisibleVisitor(bool includeBogus);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
