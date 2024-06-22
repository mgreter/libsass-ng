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
  // Getting the compare operators correctly is extremly important!
  // Otherwise we cat get undefined behavior from STL containers!
  // These bugs are hard to debug and give no indication at all!
  // The less than check is rather nasty to implement, and we use
  // std::tuples to help with the common cases. Gets complicated when
  // we have mixed objects to compare by ptr value and regular values.
  /////////////////////////////////////////////////////////////////////////
  // For equality operator we can also use the hash value.
  // If the hash is not equal, the values can't be either.
  // And use cheap pointer check to eliminate self compare.
  /////////////////////////////////////////////////////////////////////////

  bool CssParentSelector::operator< (const CssParentSelector& rhs) const
  {
    return false;
  }

  bool SelectorList::operator<(const SelectorList& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<ComplexSelectorObj>);
  }

  bool ComplexSelector::operator<(const ComplexSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<CplxSelComponentObj>);
  }

  bool CompoundSelector::operator<(const CompoundSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<SimpleSelectorObj>);
  }

  bool CplxSelComponent::operator<(const CplxSelComponent& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Check if prefix is less than right hand
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
    // Prefix proved to be equal, now go for the tie
    return ObjLessThanFn(selector_, rhs.selector_);
  }

  bool PseudoSelector::operator<(const PseudoSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Check if prefix is less than right hand
    if (std::tie(name_, argument_, isClass_)
      < std::tie(rhs.name_, rhs.argument_, rhs.isClass_)) return true;
    // Check if prefix is equal to right hand
    // We already know it is now less than ...
    if (std::tie(name_, argument_, isClass_)
      < std::tie(rhs.name_, rhs.argument_, rhs.isClass_)) return false;
    // Prefix proved to be equal, now go for the tie
    return ObjLessThanFn(selector_, rhs.selector_);
  }

  bool AttributeSelector::operator<(const AttributeSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the set of values via tupple
    return std::tie(ns_, hasNs_, name_, value_, op_, modifier_)
      < std::tie(rhs.ns_, rhs.hasNs_, rhs.name_, rhs.value_, rhs.op_, rhs.modifier_);
  }

  bool TypeSelector::operator<(const TypeSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    return std::tie(ns_, hasNs_, name_)
      < std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  }

  bool IDSelector::operator<(const IDSelector& rhs) const
  {
    if (&rhs == this) return false;
    // ID has no namespace
    return name() < rhs.name();
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

  // CSS Parent selectors have no distinction feature
  bool CssParentSelector::operator== (const CssParentSelector& rhs) const
  {
    return true;
  }

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
    // Compare the whole list by ptr value
    return std::equal(
      begin(), end(), rhs.begin(),
      ObjEqualityFn<ComplexSelectorObj>);
  }

  bool ComplexSelector::operator== (const ComplexSelector& rhs) const
  {
    if (&rhs == this) return true;
    if (size() != rhs.size()) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Compare the whole list by ptr value
    return std::equal(
      begin(), end(), rhs.begin(),
      ObjEqualityFn<CplxSelComponentObj>);
  }

  bool CompoundSelector::operator== (const CompoundSelector& rhs) const
  {
    if (&rhs == this) return true;
    if (size() != rhs.size()) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Compare the whole list by ptr value
    return std::equal(
      begin(), end(), rhs.begin(),
      ObjEqualityFn<SimpleSelectorObj>);
  }

  bool CplxSelComponent::operator==(const CplxSelComponent& rhs) const
  {
    if (&rhs == this) return true;
    if (combinators_.size() != rhs.combinators_.size()) return false;
    // Check if prefix is different
    if (!std::equal(
      combinators_.begin(),
      combinators_.end(),
      rhs.combinators_.begin(),
      ObjEqualityFn<SelectorCombinatorObj>)) return false;
    // Prefix proved to be equal, now go for the tie
    return ObjEqualityFn(selector_, rhs.selector_);
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
      && ObjEqualityFn(selector(), rhs.selector());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


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


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
