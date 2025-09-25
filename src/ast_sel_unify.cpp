/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* This file contains all ast unify functions in one compile unit.           */
/*****************************************************************************/
#include "ast_selectors.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Returns the contents of a [SelectorList] that matches only 
  // elements that are matched by both [complex1] and [complex2].
  // If no such list can be produced, returns `null`.
  /////////////////////////////////////////////////////////////////////////
  // ToDo: fine-tune API to avoid unnecessary wrapper allocations
  /////////////////////////////////////////////////////////////////////////
  ComplexSelectors _unifyComplex(
    const ComplexSelectors& complexes,
    const SourceSpan& pstate)
  {

    SelectorCombinatorObj leadingCombinator;
    SelectorCombinatorObj trailingCombinator;
    CompoundSelector* unifiedBase = nullptr;

    for (const ComplexSelector* complex : complexes)
    {
      if (complex->isUseless()) return {};

      if (complex->elements().size() == 1) {
        if (complex->hasOneLeadingCombinators()) {
          const SelectorCombinatorObj& lead(
            complex->getLeadingCombinator());
          if (leadingCombinator.isNull()) {
            leadingCombinator = lead;
          }
          else if (!ObjEqualityFn(leadingCombinator, lead)) {
            return {}; // Return empty list
          }
        }
      }

      if (complex->size() == 0) continue;
      // Get last compound of current complex selector
      // This is the one that will connect to next lead
      const auto& base = complex->last();

      if (base->combinators().size() == 1) {
        const SelectorCombinatorObj& trail(
          base->combinators().back());
        if (trailingCombinator != nullptr) {
          if (!ObjEqualityFn(trailingCombinator, trail)) {
            return {}; // Return empty list
          }
        }
        trailingCombinator = trail;
      }

      if (unifiedBase == nullptr) {
        unifiedBase = base->selector();
      }
      else {
        unifiedBase = unifyCompound(unifiedBase, base->selector());
        if (unifiedBase == nullptr) return {};
      }
    }
    // EO complexes loop

    ComplexSelectors withoutBases;
    for (const ComplexSelector* complex : complexes) {
      if (complex->size() < 2) continue;
      ComplexSelector* unbase = SASS_MEMORY_COPY(complex);
      unbase->elements().pop_back(); // remove last
      withoutBases.push_back(unbase); // add unbase
    }

    sass::vector<SelectorCombinatorObj> trailing;
    if (trailingCombinator != nullptr)
      trailing.push_back(trailingCombinator);
    CplxSelComponent* component = SASS_MEMORY_NEW(
      CplxSelComponent, pstate, std::move(trailing), unifiedBase);

    ComplexSelectorObj base = !leadingCombinator ?
      SASS_MEMORY_NEW(ComplexSelector, pstate, {}, { component }) :
      SASS_MEMORY_NEW(ComplexSelector, pstate, { leadingCombinator }, { component });

    if (withoutBases.empty()) {
      return weave27({ base }, false);
    }

    withoutBases.back() = withoutBases.back()
      ->concatenate(base, pstate, false);
    return weave27(withoutBases, false);
  }
  // EO unifyComplex

  /////////////////////////////////////////////////////////////////////////
  // Returns a [CompoundSelector] that matches only elements
  // that are matched by both [compound1] and [compound2].
  // If no such selector can be produced, returns `null`.
  /////////////////////////////////////////////////////////////////////////
//  CompoundSelector* CompoundSelector::unifyWith(sass::vector<CompoundSelectorObj> rhs)
//  {
//    if (empty()) return rhs;
//    CompoundSelectorObj unified = SASS_MEMORY_COPY(rhs);
//    for (const SimpleSelectorObj& sel : elements()) {
//      unified = sel->unifyWith(unified);
//      if (unified.isNull()) break;
//    }
//    return unified.detach();
//  }
  // EO CompoundSelector::unifyWith(CompoundSelector*)

  /////////////////////////////////////////////////////////////////////////
  // Returns the components of a [CompoundSelector] that matches only elements
  // matched by both this and [compound]. By default, this just returns a copy
  // of [compound] with this selector added to the end, or returns the original
  // array if this selector already exists in it. Returns `null` if unification
  // is impossible—for example, if there are multiple ID selectors.
  /////////////////////////////////////////////////////////////////////////
  // This is implemented in `selector/simple.dart` as `SimpleSelector::unify`
  /////////////////////////////////////////////////////////////////////////
  SimpleSelectors SimpleSelector::unify(
    const SimpleSelectors& others)
  {

    if (name_ == "host" || name_ == "host-context") {
      for (const SimpleSelector* simple : others) {
        const PseudoSelector* pseudo =
          simple->isaPseudoSelector();
        if (pseudo == nullptr) return {};
        if (pseudo->isHost()) continue;
        if (pseudo->selector()) continue;
        return {};
      }
    }
    // Optimize the simple cases
    else if (others.size() == 1) {
      if (others[0]->isUniversal()) {
        return others[0]->unify({ this });
      }
      else if (const PseudoSelector* pseudo = others[0]->isaPseudoSelector()) {
        if (pseudo->isHost() || pseudo->isHostContext())
          return others[0]->unify({ this });
      }
    }

    // Check if we are already part of other compound selectors
    for (const SimpleSelector* simple : others) {
      if (PtrObjEqualityFn<SimpleSelector>(simple, this))
        return others;
    }

    SimpleSelectors results;
    results.reserve(others.size() + 1);
    bool addedThis = false;
    for (SimpleSelector* simple : others) {
      // Make sure pseudo selectors always come last.
      if (!addedThis && simple->isaPseudoSelector()) {
        // if (isPseudoElement()) return {};
        results.push_back(this);
        addedThis = true;
      }
      results.push_back(simple);
    }
    if (!addedThis) {
      results.push_back(this);
    }
    return results;
  }
  // EO SimpleSelector::unifyWith(CompoundSelector*)

  /////////////////////////////////////////////////////////////////////////
  // This is implemented in `selector/pseudo.dart` as `PseudoSelector::unify`
  /////////////////////////////////////////////////////////////////////////
  SimpleSelectors PseudoSelector::unify(
    const SimpleSelectors& others)
  {
    if (name_ == "host" || name_ == "host-context") {
      for (const SimpleSelector* simple : others) {
        const PseudoSelector* pseudo =
          simple->isaPseudoSelector();
        if (pseudo == nullptr) return {};
        if (pseudo->isHost()) continue;
        if (pseudo->selector()) continue;
        return {};
      }
    }
    // Optimize the simple cases
    else if (others.size() == 1) {
      if (others[0]->isUniversal()) {
        return others[0]->unify({ this });
      }
      else if (const PseudoSelector* pseudo = others[0]->isaPseudoSelector()) {
        if (pseudo->isHost() || pseudo->isHostContext())
          return others[0]->unify({ this });
      }
    }

    // Check if we are already part of other compound selectors
    for (const SimpleSelector* simple : others) {
      if (PtrObjEqualityFn<SimpleSelector>(simple, this))
        return others;
    }

    SimpleSelectors results;
    results.reserve(others.size() + 1);
    bool addedThis = false;
    for (SimpleSelector* simple : others) {
      if (!addedThis && simple->isPseudoElement()) {
        if (isPseudoElement()) return {};
        results.push_back(this);
        addedThis = true;
      }
      results.push_back(simple);
    }
    if (!addedThis) {
      results.push_back(this);
    }
    return results;
  }

  /////////////////////////////////////////////////////////////////////////
  // This is implemented in `selector/id.dart` as `PseudoSelector::unify`
  /////////////////////////////////////////////////////////////////////////
  SimpleSelectors IDSelector::unify(
    const SimpleSelectors& rhs)
  {
    for (const SimpleSelector* sel : rhs) {
      if (const IDSelector* ids = sel->isaIDSelector()) {
        if (ids->name() != name()) return {};
      }
    }
    // Dispatch to base implementation
    return SimpleSelector::unify(rhs);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  SelectorNS* SelectorNS::unity(SelectorNS* rhs)
  {
    if (name_ == rhs->name() || rhs->isUniversal()) {
      if (nsEqual(*rhs) || rhs->isUniversalNs()) {
        return this;
      }
      if (isUniversalNs()) {
        return SASS_MEMORY_NEW(TypeSelector, pstate(),
          sass::string(this->name()),
          sass::string(rhs->ns()),
          hasNs() && rhs->hasNs());
      }
    }
    else if (name().empty() || isUniversal()) {
      if (nsEqual(*rhs) || rhs->isUniversalNs()) {
        return SASS_MEMORY_NEW(TypeSelector, pstate(),
          sass::string(rhs->name()),
          sass::string(this->ns()),
          hasNs() && rhs->hasNs());
      }
      if (isUniversalNs()) {
        return SASS_MEMORY_NEW(TypeSelector, pstate(),
          sass::string(rhs->name()),
          sass::string(rhs->ns()),
          hasNs() && rhs->hasNs());
      }
    }
    return nullptr;
  }


  /////////////////////////////////////////////////////////////////////////
  // This is implemented in `extend/functions.dart` as `unifyUniversalAndElement`
  // Returns a [SimpleSelector] that matches only elements that are matched by
  // both [lhs] and [rhs], which must both be either [UniversalSelector]s
  // or [TypeSelector]s. If no such selector can be produced, returns `null`.
  // Note: libsass handles universal selector directly within the type selector
  /////////////////////////////////////////////////////////////////////////
  //SimpleSelector* TypeSelector::unifyWith(const SimpleSelector* rhs2)
  //{
  //  if (auto rhs = rhs2->isaNameSpaceSelector()) {
  //    bool rhs_ns = false;
  //    if (!(nsEqual(*rhs) || rhs->isUniversalNs())) {
  //      if (!isUniversalNs()) {
  //        return nullptr;
  //      }
  //      rhs_ns = true;
  //    }
  //    bool rhs_name = false;
  //    if (!(name_ == rhs->name() || rhs->isUniversal())) {
  //      if (!(isUniversal())) {
  //        return nullptr;
  //      }
  //      rhs_name = true;
  //    }
  //    if (rhs_ns) {
  //      ns(rhs->ns());
  //      hasNs(rhs->hasNs());
  //    }
  //    if (rhs_name) name(rhs->name());
  //  }
  //  return this;
  //}
  // EO TypeSelector::unifyWith(const SimpleSelector*)

  SimpleSelectors TypeSelector::unifyUniversal(
    const SimpleSelectors& compound)
  {
    if (compound.size() == 0) {
      return { this };
    }
    else if (compound.size() > 0) {
      if (auto type = compound[0]->isaTypeSelector()) {
        auto unified = SelectorNS::unity(type);
        if (unified == nullptr) return {};
        SimpleSelectors rv;
        rv.push_back(unified);
        rv.insert(rv.end(),
          compound.begin() + 1,
          compound.end());
        return rv;
      }

      else if (auto pseudo = compound[0]->isaPseudoSelector()) {
        if (pseudo->isHost() || pseudo->isHostContext()) return {};
      }
      else {
        // std::cerr << "hasNs: " << hasNs_ << ", ns " << ns_ << "\n";
        if (!hasNs_ || ns_ == "*") {
          return compound;
        }
        else {
          SimpleSelectors rv;
          rv.push_back(this);
          rv.insert(rv.end(),
            compound.begin(),
            compound.end());
          return rv;
        }
      }
    }
    return compound;
  }

  /////////////////////////////////////////////////////////////////////////
  // This is implemented in `selector/type.dart` as `PseudoSelector::unify`
  /////////////////////////////////////////////////////////////////////////
  SimpleSelectors TypeSelector::unify(
    const SimpleSelectors& compound)
  {

    if (compound.empty()) return {};
    if (isUniversal()) {
      return unifyUniversal(compound);
    }
    const auto& first = compound.front();
    if (first->isUniversal()) {
      return first->unify({ this });
    }
    else if (const auto& type = first->isaTypeSelector()) {
      SelectorNS* unified = SelectorNS::unity(type);
      if (unified == nullptr) return {};
      if (unified->empty()) return {};
      SimpleSelectors result;
      result.push_back(unified);
      result.insert(result.end(),
        compound.begin() + 1,
        compound.end());
      return result;
    }
    else {
      SimpleSelectors result;
      result.push_back(this);
      result.insert(result.end(),
        compound.begin(),
        compound.end());
      return result;
    }
  }
  // EO TypeSelector::unify(SimpleSelectors)


  /////////////////////////////////////////////////////////////////////////
  // Unify two complex selectors. Internally calls `unifyComplex`
  // and then wraps the result in newly create ComplexSelectors.
  /////////////////////////////////////////////////////////////////////////
  SelectorList* ComplexSelector::unifyList(ComplexSelector* rhs)
  {
    ComplexSelectors list(
       _unifyComplex({ this, rhs }, pstate()));
    if (list.empty()) return nullptr;
    return SASS_MEMORY_NEW(SelectorList,
      pstate(), std::move(list));
  }
  // EO ComplexSelector::unifyWith(ComplexSelector*)

  /////////////////////////////////////////////////////////////////////////
  // only called from the sass function `selector-unify`
  /////////////////////////////////////////////////////////////////////////
  SelectorList* SelectorList::unifyWith(SelectorList* rhs)
  {
    ComplexSelectors selectors;
    // Unify all of children with RHS's children,
    // storing the results in `unified_complex_selectors`
    for (const ComplexSelectorObj& seq1 : elements()) {
      for (const ComplexSelectorObj& seq2 : rhs->elements()) {
        if (SelectorListObj unified = seq1->unifyList(seq2)) {
          selectors.insert(selectors.end(),
            unified->begin(), unified->end());
        }
      }
    }
    return SASS_MEMORY_NEW(SelectorList,
      pstate(), std::move(selectors));
  }
  // EO SelectorList::unifyWith(SelectorList*)

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
