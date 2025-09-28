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

    virtual bool visitAttributeSelector(AttributeSelector* attribute) final;
    virtual bool visitClassSelector(ClassSelector* klass) final;
    virtual bool visitComplexSelector(ComplexSelector* complex) override;
    virtual bool visitCompoundSelector(CompoundSelector* compound) final;
    virtual bool visitIDSelector(IDSelector* id) final;
    virtual bool visitPlaceholderSelector(PlaceholderSelector* placeholder) override;
    virtual bool visitPseudoSelector(PseudoSelector* pseudo) override;
    virtual bool visitSelectorList(SelectorList* list) override;
    virtual bool visitTypeSelector(TypeSelector* type) final;
    virtual bool visitCssParentSelector(CssParentSelector* parent) override;
    // virtual bool visitSelectorCombinator(SelectorCombinator* combinator) final;

  };

}

#endif
