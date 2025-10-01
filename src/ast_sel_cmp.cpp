/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* This file contains all ast operator functions in one compile unit.        */
/*****************************************************************************/
#include "ast_selectors.hpp"
#include "ast_statements.hpp"

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

  bool SelectorList::operator==(const SelectorList& rhs) const
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

  bool SelectorList::operator<(const SelectorList& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<ComplexSelectorObj>);
  }

  size_t SelectorList::hash() const
  {
    if (Selector::hash_ == 0) {
      hash_start(Selector::hash_, typeid(this).hash_code());
      hash_combine(Selector::hash_, Vectorized<ComplexSelector>::hash());
    }
    return Selector::hash_;
  }

  Box* SelectorList::sealed()
  {
    auto mbox = SASS_MEMORY_NEW(ModifiableBox, this);
    return mbox->seal();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool ComplexSelector::operator==(const ComplexSelector& rhs) const
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

  bool ComplexSelector::operator<(const ComplexSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<CplxSelComponentObj>);
  }

  size_t ComplexSelector::hash() const
  {
    if (Selector::hash_ == 0) {
      hash_start(Selector::hash_, typeid(this).hash_code());
      hash_combine(Selector::hash_, Vectorized<CplxSelComponent>::hash());
    }
    return Selector::hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool CompoundSelector::operator==(const CompoundSelector& rhs) const
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

  bool CompoundSelector::operator<(const CompoundSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the whole list by ptr value
    return std::lexicographical_compare(
      begin(), end(), rhs.begin(), rhs.end(),
      ObjLessThanFn<SimpleSelectorObj>);
  }

  size_t CompoundSelector::hash() const
  {
    if (Selector::hash_ == 0) {
      hash_start(Selector::hash_, typeid(this).hash_code());
      hash_combine(Selector::hash_, Vectorized<SimpleSelector>::hash());
    }
    return Selector::hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool CplxSelComponent::operator==(const CplxSelComponent& rhs) const
  {
    if (&rhs == this) return true;
    // ToDo: has no hash interface?
    // Check that prefix size matches
    if (combinators_.size() !=
      rhs.combinators_.size())
        return false;
    #ifndef SASS_FORCE_CMP_HASH
//    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Check if prefix is different
    if (!std::equal(
      combinators_.begin(),
      combinators_.end(),
      rhs.combinators_.begin(),
      ObjEqualityFn<SelectorCombinatorObj>)) return false;
    // Prefix proved to be equal, now go for the tie
    return ObjEqualityFn(selector_, rhs.selector_);
  }

  bool CplxSelComponent::operator<(const CplxSelComponent& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // ToDo: has no hash interface?
    // Check that prefix size matches
    if (combinators_.size() <
      rhs.combinators_.size())
        return false;
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

  size_t CplxSelComponent::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      for (const auto& combinator : combinators_) {
        hash_combine(hash_, combinator->hash());
      }
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool SelectorCombinator::operator==(const SelectorCombinator& rhs) const
  {
    if (&rhs == this) return true;
    return combinator_ == rhs.combinator_;
  }

  bool SelectorCombinator::operator<(const SelectorCombinator& rhs) const
  {
    if (&rhs == this) return false;
    return combinator_ < rhs.combinator_;
  }

  size_t SelectorCombinator::hash() const
  {
    switch (combinator_) {
    case CHILD: return size_t(2329817243) + getHashSeed();
    case FOLLOWING: return size_t(24768578) + getHashSeed();
    case SIBLING: return size_t(2387651244) + getHashSeed();
    default: return size_t(7345484764) + getHashSeed();
    }
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool PseudoSelector::operator==(const PseudoSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Check if prefix is different (without isSyntacticClass)
    if (std::tie(rhs.name_, rhs.argument_, rhs.isClass_) != 
      std::tie(name_, argument_, isClass_)) return false;
    // Prefix proved to be equal, now go for the tie
    return ObjEqualityFn(selector(), rhs.selector());
  }

  bool PseudoSelector::operator<(const PseudoSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Check if prefix is less than right hand
    if (std::tie(name_, argument_, isClass_) <
      std::tie(rhs.name_, rhs.argument_, rhs.isClass_)) return true;
    // Check if prefix is equal to right hand
    // We already know it is now less than ...
    if (std::tie(rhs.name_, rhs.argument_, rhs.isClass_) <
      std::tie(name_, argument_, isClass_)) return false;
    // Prefix proved to be equal, now go for the tie
    return ObjLessThanFn(selector_, rhs.selector_);
  }

  size_t PseudoSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, name_);
      hash_combine(hash_, argument_);
      hash_combine(hash_, isClass_);
      if (selector_) hash_combine(
        hash_, selector_->hash());
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Could re-use "some" code from SelectorNS, but implement fully
  /////////////////////////////////////////////////////////////////////////

  bool AttributeSelector::operator==(const AttributeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Compare the set of values via tupple
    return std::tie(ns_, hasNs_, name_, value_, op_, modifier_, isIdentifier_) ==
      std::tie(rhs.ns_, rhs.hasNs_, rhs.name_, rhs.value_, rhs.op_, rhs.modifier_, rhs.isIdentifier_);
  }

  bool AttributeSelector::operator<(const AttributeSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    // Compare the set of values via tupple
    return std::tie(ns_, hasNs_, name_, value_, op_, modifier_) <
      std::tie(rhs.ns_, rhs.hasNs_, rhs.name_, rhs.value_, rhs.op_, rhs.modifier_);
  }

  size_t AttributeSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, ns_);
      hash_combine(hash_, hasNs_);
      hash_combine(hash_, name_);
      hash_combine(hash_, value_);
      hash_combine(hash_, op_);
      hash_combine(hash_, modifier_);
      hash_combine(hash_, isIdentifier_);
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Could re-use code from SelectorNS, but implement fully
  /////////////////////////////////////////////////////////////////////////

  bool TypeSelector::operator==(const TypeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Compare the set of values via tupple
    return std::tie(ns_, hasNs_, name_) ==
      std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  }

  bool TypeSelector::operator<(const TypeSelector& rhs) const
  {
    // Do simple pointer compare first
    if (&rhs == this) return false;
    return std::tie(ns_, hasNs_, name_) <
      std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  }

  size_t TypeSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, ns_);
      hash_combine(hash_, hasNs_);
      hash_combine(hash_, name_);
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Could re-use code from SimpleSelector, but implement fully
  /////////////////////////////////////////////////////////////////////////

  bool IDSelector::operator==(const IDSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // ID has no namespace
    return name() == rhs.name();
  }

  bool IDSelector::operator<(const IDSelector& rhs) const
  {
    if (&rhs == this) return false;
    // ID has no namespace
    return name() < rhs.name();
  }

  size_t IDSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, name_);
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Could re-use code from SimpleSelector, but implement fully
  /////////////////////////////////////////////////////////////////////////

  bool ClassSelector::operator==(const ClassSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // ID has no namespace
    return name() == rhs.name();
  }

  bool ClassSelector::operator<(const ClassSelector& rhs) const
  {
    if (&rhs == this) return false;
    // ID has no namespace
    return name() < rhs.name();
  }

  size_t ClassSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, name_);
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Could re-use code from SimpleSelector, but implement fully
  /////////////////////////////////////////////////////////////////////////

  bool PlaceholderSelector::operator==(const PlaceholderSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // ID has no namespace
    return name() == rhs.name();
  }

  bool PlaceholderSelector::operator<(const PlaceholderSelector& rhs) const
  {
    if (&rhs == this) return false;
    // ID has no namespace
    return name() < rhs.name();
  }

  size_t PlaceholderSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
      hash_combine(hash_, name_);
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // CSS Parent selectors have no distinction feature
  bool CssParentSelector::operator< (const CssParentSelector& rhs) const
  {
    return false;
  }

  // CSS Parent selectors have no distinction feature
  bool CssParentSelector::operator==(const CssParentSelector& rhs) const
  {
    return true;
  }

  size_t CssParentSelector::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(this).hash_code());
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  // Dont implement anything for abstract classes
  // Not sure typeid would work as expected for us
  // Although that is just a problem for `hash()`
  /////////////////////////////////////////////////////////////////////////

  // bool SimpleSelector::operator==(const SimpleSelector& rhs) const
  // {
  //   if (&rhs == this) return true;
  //   #ifndef SASS_FORCE_CMP_HASH
  //   if (hashed() != 0 && rhs.hashed() != 0)
  //   #endif
  //     if (hash() != rhs.hash()) return false;
  //   // ID has no namespace
  //   return name() == rhs.name();
  // }

  // bool SimpleSelector::operator<(const SimpleSelector& rhs) const
  // {
  //   if (&rhs == this) return false;
  //   // ID has no namespace
  //   return name() < rhs.name();
  // }

  // size_t SimpleSelector::hash() const
  // {
  //   if (hash_ == 0) {
  //     hash_start(hash_, typeid(this).hash_code());
  //     hash_combine(hash_, name_);
  //   }
  //   return hash_;
  // }

  /////////////////////////////////////////////////////////////////////////
  // Dont implement anything for abstract classes
  // Not sure typeid would work as expected for us
  // Although that is just a problem for `hash()`
  /////////////////////////////////////////////////////////////////////////

  // bool SelectorNS::operator==(const SelectorNS& rhs) const
  // {
  //   // Do simple pointer compare first
  //   if (&rhs == this) return false;
  //   // Compare the set of values via tupple
  //   return std::tie(ns_, hasNs_, name_) ==
  //     std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  // }
  // 
  // bool SelectorNS::operator<(const SelectorNS& rhs) const
  // {
  //   // Do simple pointer compare first
  //   if (&rhs == this) return false;
  //   // Compare the set of values via tupple
  //   return std::tie(ns_, hasNs_, name_) <
  //     std::tie(rhs.ns_, rhs.hasNs_, rhs.name_);
  // }
  // 
  // size_t SelectorNS::hash() const
  // {
  //   if (hash_ == 0) {
  //     hash_start(hash_, typeid(this).hash_code());
  //     hash_combine(hash_, ns_);
  //     hash_combine(hash_, hasNs_);
  //     hash_combine(hash_, name_);
  //   }
  //   return hash_;
  // }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
