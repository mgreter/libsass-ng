/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* This file contains all ast operator functions in one compile unit.        */
/*****************************************************************************/
#include "ast_selectors.hpp"
#include "ast_statements.hpp"
#include "callstack.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool CplxSelComponent::operator==(const CplxSelComponent& rhs) const
  {
    if (combinators_ != rhs.combinators_) return false;
    if (selector_ && rhs.selector_) return *selector_ == *rhs.selector_;
    return selector_ == nullptr && rhs.selector_ == nullptr;
  }

  bool CplxSelComponent::operator<(const CplxSelComponent& rhs) const
  {
    if (&rhs == this) return false;
    if (std::lexicographical_compare(
      combinators_.begin(), combinators_.end(),
      rhs.combinators_.begin(), rhs.combinators_.end(),
      ObjLessThanFn<SelectorCombinatorObj>)) return true;
    // Use inverted compare to determine equality
    // Could also use equality operator instead
    if (std::lexicographical_compare(
      rhs.combinators_.begin(), rhs.combinators_.end(),
      combinators_.begin(), combinators_.end(),
      ObjLessThanFn<SelectorCombinatorObj>)) return false;
    return ObjLessThanFn(selector_, rhs.selector_);
  }

  bool SelectorList::operator<(const SelectorList& rhs) const
  {
    if (&rhs == this) return false;
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<ComplexSelectorObj>);
  }

  bool ComplexSelector::operator<(const ComplexSelector& rhs) const
  {
    if (&rhs == this) return false;
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<CplxSelComponentObj>);
  }

  bool CompoundSelector::operator<(const CompoundSelector& rhs) const
  {
    if (&rhs == this) return false;
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<SimpleSelectorObj>);
  }

  bool IDSelector::operator<(const IDSelector& rhs) const
  {
    if (&rhs == this) return false;
    // ID has no namespace
    return name() < rhs.name();
  }

  bool TypeSelector::operator<(const TypeSelector& rhs) const
  {
    if (&rhs == this) return false;
    return std::tie(ns_, hasNs_, name_)
      < std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  }

  bool PlaceholderSelector::operator<(const PlaceholderSelector& rhs) const
  {
    if (&rhs == this) return false;
    // Placeholder has no namespace
    return name() < rhs.name();
  }


  bool ClassSelector::operator<(const ClassSelector& rhs) const
  {
    if (&rhs == this) return false;
    // Class has no namespace
    return name() < rhs.name();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool SelectorList::operator== (const SelectorList& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (get(i)->hash() != rhs.get(i)->hash()) return false;
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool ComplexSelector::operator== (const ComplexSelector& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool CompoundSelector::operator== (const CompoundSelector& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (get(i)->hash() != rhs.get(i)->hash()) return false;
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool IDSelector::operator== (const IDSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // ID has no namespace
    return name() == rhs.name();
  }

  bool TypeSelector::operator== (const TypeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Match equality hard
    return ns_ == rhs.ns_ &&
      hasNs_ == rhs.hasNs_ &&
      name_ == rhs.name_;
  }

  bool ClassSelector::operator== (const ClassSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Class has no namespace
    return name() == rhs.name();
  }

  bool PlaceholderSelector::operator== (const PlaceholderSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Placeholder has no namespace
    return name() == rhs.name();
  }

  bool AttributeSelector::operator<(const AttributeSelector& rhs) const
  {
    if (&rhs == this) return false;
    return std::tie(ns_, hasNs_, name_, value_, op_, modifier_)
      < std::tie(rhs.ns_, rhs.hasNs_, rhs.name_, rhs.value_, rhs.op_, rhs.modifier_);
  }

  bool PseudoSelector::operator<(const PseudoSelector& rhs) const
  {
    if (&rhs == this) return false;
    return std::tie(name_, argument_, isClass_)
      < std::tie(rhs.name_, rhs.argument_, rhs.isClass_)
      || (!(std::tie(rhs.name_, rhs.argument_, rhs.isClass_)
          < std::tie(name_, argument_, isClass_))
          && ObjLessThanFn(selector_, rhs.selector_));
  }

  bool AttributeSelector::operator== (const AttributeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    return nsMatch(rhs)
      && op() == rhs.op()
      && name() == rhs.name()
      && value() == rhs.value()
      && modifier() == rhs.modifier();
  }

  bool PseudoSelector::operator== (const PseudoSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    return name() == rhs.name()
      && argument() == rhs.argument()
      && isClass() == rhs.isClass()
      && ObjEquality()(selector(), rhs.selector());
  }

  // CSS Parent selectors have no distinction feature
  bool CssParentSelector::operator== (const CssParentSelector& rhs) const
  {
    return true;
  }

  bool CssParentSelector::operator< (const CssParentSelector& rhs) const
  {
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
