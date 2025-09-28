/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_SEL_ANY_HPP
#define SASS_SEL_ANY_HPP

#include "visitor_selector.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class AnySelectorVisitor : public SelectorVisitor<bool> {

  public:

    bool visitAttributeSelector(AttributeSelector* attribute) final;
    bool visitClassSelector(ClassSelector* klass) final;
    bool visitComplexSelector(ComplexSelector* complex) override;
    bool visitCompoundSelector(CompoundSelector* compound) final;
    bool visitIDSelector(IDSelector* id) final;
    bool visitPlaceholderSelector(PlaceholderSelector* placeholder) override;
    bool visitPseudoSelector(PseudoSelector* pseudo) override;
    bool visitSelectorList(SelectorList* list) override;
    bool visitTypeSelector(TypeSelector* type) final;
    bool visitCssParentSelector(CssParentSelector* parent) override;
    // virtual bool visitSelectorCombinator(SelectorCombinator* combinator) final;

  };

}

#endif
