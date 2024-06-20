/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* This file contains all ast superselector functions in one compile unit.   */
/*****************************************************************************/
#include "ast_selectors.hpp"

#include "dart_helpers.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // To compare/debug dart-sass vs libsass you can use debugger.hpp:
  // c++: std::cerr << "result " << debug_vec(compound) << "\n";
  // dart: stderr.writeln("result " + compound.toString());
  /////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [list1] is a superselector of [list2].
  // That is, whether [list1] matches every element that
  // [list2] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  bool listIsSuperslector(
    const ComplexSelectors& list1,
    const ComplexSelectors& list2);

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [complex1] is a superselector of [complex2].
  // That is, whether [complex1] matches every element that
  // [complex2] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  static bool complexIsSuperselector(
    CplxSelComponentVector::const_iterator lhs_beg,
    CplxSelComponentVector::const_iterator lhs_end,
    CplxSelComponentVector::const_iterator rhs_beg,
    CplxSelComponentVector::const_iterator rhs_end);

  /////////////////////////////////////////////////////////////////////////
  // Returns all pseudo selectors in [compound] that have
  // a selector argument, and that have the given [name].
  /////////////////////////////////////////////////////////////////////////
  static PseudoSelectors _selectorPseudoArgs(
    const SimpleSelectors& compound, const sass::string& name, bool isClass = true)
  {
    PseudoSelectors rv;
    for (const SimpleSelectorObj& sel : compound) {
      if (const PseudoSelectorObj& pseudo = sel->isaPseudoSelector()) {
        if (pseudo->isClass() == isClass && pseudo->selector()) {
          if (sel->name() == name) {
            rv.emplace_back(pseudo);
          }
        }
      }
    }
    return rv;
  }
  // EO selectorPseudoNamed

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [simple1] is a superselector of [simple2].
  // That is, whether [simple1] matches every element that
  // [simple2] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  static bool simpleIsSuperselector(
    const SimpleSelector* simple1,
    const SimpleSelector* simple2)
  {

    if (simple1->isUniversal()) {
      // if (!simple2->isUniversal()) return false;
      return simple1->nsMatch(*simple2);
    }

    // If they are equal they are superselectors
    if (PtrObjEqualityFn(simple1, simple2)) {
      return true;
    }
    // Some selector pseudo-classes can match normal selectors.
    if (const PseudoSelector* pseudo = simple2->isaPseudoSelector()) {
      if (pseudo->selector() && isSubselectorPseudo(pseudo->normalized())) {
        for (auto& complex : pseudo->selector()->elements()) {
          // Make sure we have exactly one items
          if (complex->size() != 1) {
            return false;
          }
          // That items must be a compound selector
//          if (auto compound = complex->at(0)->isaCompoundSelector()) {
//            // It must contain the lhs simple selector
//            if (!compound->contains(simple1)) { 
//              return false;
//            }
//          }
        }
        return true;
      }
    }
    return false;
  }
  // EO simpleIsSuperselector

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [simple] is a superselector of [compound].
  // That is, whether [simple] matches every element that
  // [compound] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  /*
  static bool simpleIsSuperselectorOfCompound(
    const SimpleSelector* simple,
    const CompoundSelector* compound)
  {
    for (const SimpleSelectorObj& theirSimple : compound->elements()) {
      if (simpleIsSuperselector(simple, theirSimple)) {
        return true;
      }
    }
    return false;
  }
  */
  // EO simpleIsSuperselectorOfCompound

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  static bool typeIsSuperselectorOfCompound(
    const TypeSelector* type,
    const CompoundSelector* compound)
  {
    for (const SimpleSelectorObj& simple : compound->elements()) {
      if (const TypeSelector* rhs = simple->isaTypeSelector()) {
        if (!(*type == *rhs)) return true;
      }
    }
    return false;
  }
  // EO typeIsSuperselectorOfCompound

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  static bool idIsSuperselectorOfCompound(
    const IDSelector* id,
    const CompoundSelector* compound)
  {
    for (const SimpleSelectorObj& simple : compound->elements()) {
      if (const IDSelector* rhs = simple->isaIDSelector()) {
        if (!(*id == *rhs)) return true;
      }
    }
    return false;
  }
  // EO idIsSuperselectorOfCompound

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  static bool pseudoIsSuperselectorOfPseudo(
    const PseudoSelector* pseudo1,
    const PseudoSelector* pseudo2,
    const ComplexSelectorObj& parent
  )
  {
    if (!pseudo2->selector()) return false;
    if (pseudo1->name() == pseudo2->name()) {
      const SelectorList* list = pseudo2->selector();
      return listIsSuperslector(list->elements(), { parent });
    }
    return false;
  }
  // EO pseudoIsSuperselectorOfPseudo

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  static bool pseudoNotIsSuperselectorOfCompound(
    const PseudoSelector* pseudo1,
    const SimpleSelectors& compound2,
    const ComplexSelectorObj& parent)
  {
    for (const SimpleSelectorObj& simple2 : compound2) {
      if (const TypeSelectorObj& type2 = simple2->isaTypeSelector()) {
        if (const CompoundSelector* compound1 = parent->last()->selector()->isaCompoundSelector()) {
          if (typeIsSuperselectorOfCompound(type2, compound1)) return true;
        }
      }
      else if (const IDSelector* id2 = simple2->isaIDSelector()) {
        if (const CompoundSelector* compound1 = parent->last()->selector()->isaCompoundSelector()) {
          if (idIsSuperselectorOfCompound(id2, compound1)) return true;
        }
      }
      else if (const PseudoSelector* pseudo2 = simple2->isaPseudoSelector()) {
        if (pseudoIsSuperselectorOfPseudo(pseudo1, pseudo2, parent)) return true;
      }
    }
    return false;
  }
  // pseudoNotIsSuperselectorOfCompound

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [pseudo1] is a superselector of [compound2].
  // That is, whether [pseudo1] matches every element that [compound2]
  // matches, as well as possibly additional elements. This assumes that
  // [pseudo1]'s `selector` argument is not `null`. If [parents] is passed,
  // it represents the parents of [compound2]. This is relevant for pseudo
  // selectors with selector arguments, where we may need to know if the
  // parent selectors in the selector argument match [parents].
  /////////////////////////////////////////////////////////////////////////
  static bool _selectorPseudoIsSuperselector(
    const PseudoSelector* pseudo1,
    const SimpleSelectors& compound2,
    // ToDo: is this really the most convenient way to do this?
    CplxSelComponentVector::const_iterator parents_from,
    CplxSelComponentVector::const_iterator parents_to)
  {

    const auto& selector1 = pseudo1->selector();

    if (selector1 == nullptr) {
      throw ("Selector $pseudo1 must have a selector argument.");
    }

    // ToDo: move normalization function
    sass::string name(StringUtils::unvendor(pseudo1->normalized()));

    if (name == "is" || name == "matches" || name == "any" || name == "where") {

      PseudoSelectors pseudos =
        _selectorPseudoArgs(compound2, pseudo1->name());

      for (auto& selector2 : pseudos) {
        if (selector1->isSuperselectorOf(selector2->selector())) {
          // std::cerr << ("---- true1\n");
          return true;
        }
      }
      for (auto& complex1 : selector1->elements()) {
        if (!complex1->leadingCombinators().empty()) continue;
        CplxSelComponentVector parents(parents_from, parents_to);
        parents.push_back(SASS_MEMORY_NEW(CplxSelComponent,
          pseudo1->pstate(), {}, SASS_MEMORY_NEW(CompoundSelector,
             pseudo1->pstate(), compound2)));
        if (complexIsSuperselector(
          complex1->begin(),
          complex1->end(),
          parents.begin(),
          parents.end())) {
          return true;
        }
      }

      return false;
    }
    else if (name == "has" || name == "host" || name == "host-context") {
      PseudoSelectors pseudos =
        _selectorPseudoArgs(compound2, pseudo1->name(), name != "slotted");
      const SelectorList* selector1 = pseudo1->selector();
      for (const PseudoSelector* pseudo2 : pseudos) {
        const SelectorList* selector = pseudo2->selector();
        if (selector1->isSuperselectorOf(selector)) {
          return true;
        }
      }
    }
    else if (name == "not") {

      for (const ComplexSelectorObj& complex : pseudo1->selector()->elements()) {
        if (!pseudoNotIsSuperselectorOfCompound(pseudo1, compound2, complex)) return false;
      }
      return true;

    }
    else if (name == "current") {
      PseudoSelectors pseudos =
        _selectorPseudoArgs(compound2, pseudo1->name());
      for (const PseudoSelector* pseudo2 : pseudos) {
        if (PtrObjEqualityFn(pseudo1, pseudo2)) return true;
      }

    }
    else if (name == "nth-child" || name == "nth-last-child") {
      for (auto& simple2 : compound2) {
        if (const PseudoSelector* pseudo2 = simple2->isaPseudoSelector()) {
          if (pseudo1->name() != pseudo2->name()) continue;
          if (pseudo1->argument() != pseudo2->argument()) continue;
          if (pseudo1->selector()->isSuperselectorOf(pseudo2->selector())) return true;
        }
      }
      return false;
    }

    return false;

  }
  // EO selectorPseudoIsSuperselector

  /// If [compound] contains a pseudo-element, returns it and its index in
  /// [compound.components].
  //static PseudoSelector* _findPseudoElementIndexed(const CompoundSelector* compound, size_t& n)
  //{
  //  for (size_t i = 0; i < compound->elements().size(); i++) {
  //    const auto& simple = compound->elements()[i];
  //    if (const auto& pseudo = simple->isaPseudoSelector()) {
  //      if (pseudo->isElement()) {
  //        n = i; return pseudo;
  //      }
  //    }
  //  }
  //  return nullptr;
  //}



  /////////////////////////////////////////////////////////////////////////
  // Returns whether [compound1] is a superselector of [compound2].
  // That is, whether [compound1] matches every element that [compound2]
  // matches, as well as possibly additional elements. If [parents] is
  // passed, it represents the parents of [compound2]. This is relevant
  // for pseudo selectors with selector arguments, where we may need to
  // know if the parent selectors in the selector argument match [parents].
  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////
  // Returns whether [compound1] is a superselector of [compound2].
  // That is, whether [compound1] matches every element that [compound2]
  // matches, as well as possibly additional elements. If [parents] is
  // passed, it represents the parents of [compound2]. This is relevant
  // for pseudo selectors with selector arguments, where we may need to
  // know if the parent selectors in the selector argument match [parents].
  /////////////////////////////////////////////////////////////////////////
  // bool compoundIsSuperselector(
  //   const CompoundSelector* compound1,
  //   const CompoundSelector* compound2,
  //   const CplxSelComponentVector& parents)
  // {
  // }


  /// Like [compoundIsSuperselector] but operates on the underlying lists of
/// simple selectors.
///
/// The [compound1] and [compound2] are expected to have efficient
/// [Iterable.length] fields.
  static bool _compoundComponentsIsSuperselector(
    const SimpleSelectors& compound1,
    SimpleSelectors compound2,
    const sass::vector<CplxSelComponentObj>& parents)
  {
    if (compound1.empty()) return true;
    if (compound2.empty()) {
      compound2.push_back(
        new TypeSelector(
          SourceSpan::internal32("FAKE"),
          "*", "*"));
    }
    return compoundIsSuperselector(compound1, compound2, parents);
  }

  static PseudoSelector* _findPseudoElementIndexed(const SimpleSelectors& compound, size_t& n)
  {
    for (size_t i = 0; i < compound.size(); i++) {
      const auto& simple = compound[i];
      if (const auto& pseudo = simple->isaPseudoSelector()) {
        if (pseudo->isElement()) {
          n = i; return pseudo;
        }
      }
    }
    return nullptr;
  }
  

  bool compoundIsSuperselector(
    const SimpleSelectors& compound1,
    const SimpleSelectors& compound2,
    const CplxSelComponentVector& parents)
  {

    size_t n1; size_t n2;
    auto pseudo1 = _findPseudoElementIndexed(compound1, n1);
    auto pseudo2 = _findPseudoElementIndexed(compound2, n2);

    if (pseudo1 && pseudo2) {

      if (pseudo1->isSuperselectorAF(pseudo2)) {
        auto l1 = SimpleSelectors(compound1.begin(), compound1.begin() + n1);
        auto l2 = SimpleSelectors(compound2.begin(), compound2.begin() + n2);
        if (!_compoundComponentsIsSuperselector(l1, l2, parents)) return false;
        auto e1 = SimpleSelectors(compound1.begin() + n1 + 1, compound1.end());
        auto e2 = SimpleSelectors(compound2.begin() + n2 + 1, compound2.end());
        return _compoundComponentsIsSuperselector(e1, e2, parents);
      }
    }
    else if (pseudo1 || pseudo2) {
      return false;
    }

    // Every selector in [compound1.components] must have a matching selector in
    // [compound2.components].
    for (auto& simple1 : compound1) {
      const auto& pseudo = simple1->isaPseudoSelector();
      if (pseudo && pseudo->selector() != nullptr) {
        if (!_selectorPseudoIsSuperselector(pseudo, compound2,
          parents.begin(), parents.end())) {
          return false;
        }
      }
      else {
        bool any = false;
        for (auto& s2 : compound2) {
          if (simple1->isSuperselectorAF(s2)) {
            any = true;
            break;
          }
        }
        if (!any) {
          return false;
        }
      }
    }
    
    return true;
  }
  // EO compoundIsSuperselector



  bool SimpleSelector::isSuperselector(SimpleSelector* other) const
  {
    return simpleIsSuperselector(this, other);
  }


  static bool _compatibleWithPreviousCombinator(SelectorCombinator* previous,
    const CplxSelComponentVector& parents)
  {
    if (parents.empty()) return true;
    if (previous == nullptr) return true;

    // The child and next sibling combinators require that the *immediate*
    // following component be a superslector.
    if (!previous->isFollowingSibling()) return false;

    // The following sibling combinator does allow intermediate
    // components, but only if they're all siblings.

    for (auto& component : parents) {
      if (!component->combinators().empty()) {
        const auto& first = component->combinators().front();
        if (first->isFollowingSibling()) continue;
        if (first->isNextSibling()) continue;
      }
      return false;
    }

    return true;
  }

  /// Returns whether [combinator1] is a supercombinator of [combinator2].
  // That is, whether `X combinator1 Y` is a superselector of `X combinator2 Y`.
  static bool _isSupercombinator(
    SelectorCombinator* combinator1,
    SelectorCombinator* combinator2)
  {
    if (combinator1 == nullptr) return !combinator2 || combinator2->isChild();
    if (combinator2 == nullptr) return false;
    return combinator1->combinator() == combinator2->combinator() ||
      (combinator1 == nullptr && combinator2->isChild()) ||
      (combinator1->isFollowingSibling() &&
        combinator2->isNextSibling());
  }

  template<class T>
  T frontOrNull(sass::vector<T> asd) {
    if (asd.size() == 0) return nullptr;
    return asd.front();
  }

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [complex1] is a superselector of [complex2].
  // That is, whether [complex1] matches every element that
  // [complex2] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  static bool complexIsSuperselector(
    CplxSelComponentVector::const_iterator lhs_beg,
    CplxSelComponentVector::const_iterator lhs_end,
    CplxSelComponentVector::const_iterator rhs_beg,
    CplxSelComponentVector::const_iterator rhs_end)
  {


    if (lhs_beg == lhs_end) return false;
    if (rhs_beg == rhs_end) return false;

    if (!(*(lhs_end - 1))->combinators().empty()) return false;
    if (!(*(rhs_end - 1))->combinators().empty()) return false;

    size_t i1 = 0;
    size_t i2 = 0;

    SelectorCombinator* previousCombinator = nullptr;
    while (true) {

      size_t remaining1 = (lhs_end - lhs_beg) - i1;
      size_t remaining2 = (rhs_end - rhs_beg) - i2;

      if (remaining1 == 0 || remaining2 == 0) {
        return false;
      }

      // More complex selectors are never superselectors of less complex ones.
      if (remaining1 > remaining2) {
        return false;
      }

      const auto& component1 = *(lhs_beg + i1);

      if (component1->combinators().size() > 1) {
        return false;
      }
      if (remaining1 == 1) {

        CplxSelComponentVector parents(
          rhs_beg + i2,
          rhs_end - 1);

        for (auto& p : parents) {
          if (p->combinators().size() > 1) {
            return false;
          }
        }

        return compoundIsSuperselector(
          component1->selector()->elements(),
          (*(rhs_end - 1))->selector()->elements(),
          parents);
      }


      // Find the first index [endOfSubselector] in [complex2] such that
      // `complex2.sublist(i2, endOfSubselector + 1)` is a subselector of
      // [component1.selector].
      auto endOfSubselector = i2;
      CplxSelComponentVector parents; // nullable?
      while (true && endOfSubselector < size_t(rhs_end - rhs_beg)) {
        const auto& component2 = *(rhs_beg + endOfSubselector);
        if (component2->combinators().size() > 1) return false;
        if (compoundIsSuperselector(component1->selector()->elements(), component2->selector()->elements(), parents)) {
          break;
        }

        endOfSubselector++;
        if (endOfSubselector == size_t(rhs_end - rhs_beg) - 1) {
          // Stop before the superselector would encompass all of [complex2]
          // because we know [complex1] has more than one element, and consuming
          // all of [complex2] wouldn't leave anything for the rest of [complex1]
          // to match.
          return false;
        }

        // parents.clear();
        parents.push_back(component2);
      }


      if (!_compatibleWithPreviousCombinator(
        previousCombinator, parents)) {
        return false;
      }

      if (size_t(rhs_end - rhs_beg) <= endOfSubselector) {
        break;
      }
      const auto& component2 = *(rhs_beg + endOfSubselector);
      auto combinator1 = frontOrNull(component1->combinators());
      auto combinator2 = frontOrNull(component2->combinators());

      if (!_isSupercombinator(combinator1, combinator2)) {
        return false;
      }

      i1++;
      i2 = endOfSubselector + 1;
      previousCombinator = combinator1;

      if ((lhs_end - lhs_beg) - i1 == 1) {
        if (combinator1 && combinator1->isFollowingSibling()) {
          // The selector `.foo ~ .bar` is only a superselector of selectors that
          // *exclusively* contain subcombinators of `~`.
          // bool isEverySuper = true;
          for (size_t i3 = i2; i3 < size_t(rhs_end - rhs_beg) - 1; i3++) {
            const auto& component = *(rhs_beg + i3);
            if (!_isSupercombinator(combinator1, component->combinators().front())) {
              return false;
            }
          }
        }
        else if (combinator1 != nullptr) {
          // `.foo > .bar` and `.foo + bar` aren't superselectors of any selectors
          // with more than one combinator.
          if ((rhs_end - rhs_beg) - i2 > 1) {
            return false;
          }
        }
      }
    }

    return false;

  }
  // EO complexIsSuperselector

  /////////////////////////////////////////////////////////////////////////
  // Like [complexIsSuperselector], but compares [complex1]
  // and [complex2] as though they shared an implicit base
  // [SimpleSelector]. For example, `B` is not normally a
  // superselector of `B A`, since it doesn't match elements
  // that match `A`. However, it *is* a parent superselector,
  // since `B X` is a superselector of `B A X`.
  /////////////////////////////////////////////////////////////////////////
  bool complexIsParentSuperselector(
    const CplxSelComponentVector& complex1,
    const CplxSelComponentVector& complex2)
  {
    if (complex1.size() > complex2.size()) return false;
    // Dirty trick, since we guarantee that the structure is the same afterwards
    // It may have some impact on pre-calculated hashing, otherwise fully safe
    CplxSelComponentVector& lhs = const_cast<CplxSelComponentVector&>(complex1);
    CplxSelComponentVector& rhs = const_cast<CplxSelComponentVector&>(complex2);
    // std::cerr << "BOGUS ==> " << base->inspecter() << "\n";
    PlaceholderSelectorObj phs = SASS_MEMORY_NEW(PlaceholderSelector,
      SourceSpan::internal32("[BASE]"), "%<temp>");
    CplxSelComponentObj base = SASS_MEMORY_NEW(CplxSelComponent,
      SourceSpan::internal32("[BASE]"), {}, phs->wrapInCompound() );
    lhs.push_back(base);
    rhs.push_back(base);
    bool rv = complexIsSuperselector(
      lhs.begin(), lhs.end(),
      rhs.begin(), rhs.end());
    lhs.pop_back();
    rhs.pop_back();
    return rv;
  }
  // EO complexIsParentSuperselector

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [list] has a superselector for [complex].
  // That is, whether an item in [list] matches every element that
  // [complex] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  static bool listHasSuperslectorForComplex(
    ComplexSelectors list,
    ComplexSelectorObj complex)
  {
    // Return true if every [complex] selector on [list2]
    // is a super selector of the full selector [list1].
    for (const ComplexSelector* lhs : list) {
      // if (complexIsSuperselector(lhs->elements(), complex->elements())) {
      if (lhs->isSuperselectorOf(complex)) {
        return true;
      }
    }
    return false;
  }
  // listIsSuperslectorOfComplex

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [list1] is a superselector of [list2].
  // That is, whether [list1] matches every element that
  // [list2] matches, as well as possibly additional elements.
  /////////////////////////////////////////////////////////////////////////
  bool listIsSuperslector(
    const ComplexSelectors& list1,
    const ComplexSelectors& list2)
  {
    // Return true if every [complex] selector on [list2]
    // is a super selector of the full selector [list1].
    for (const ComplexSelectorObj& complex : list2) {
      if (!listHasSuperslectorForComplex(list1, complex)) {
        return false;
      }
    }
    return true;
  }
  // EO listIsSuperslector

  /////////////////////////////////////////////////////////////////////////
  // Implement selector methods (dispatch to functions)
  /////////////////////////////////////////////////////////////////////////

  bool SelectorList::isSuperselectorOf(const SelectorList* sub) const
  {
    return listIsSuperslector(elements(), sub->elements());
  }

  bool ComplexSelector::isSuperselectorOf(const ComplexSelector* sub) const
  {
    return leadingCombinators_.empty() &&
      sub->leadingCombinators_.empty() &&
      complexIsSuperselector(
        begin(), end(),
        sub->begin(),
        sub->end());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool SimpleSelector::isSuperselectorAF(SimpleSelector* other) const
  {
    if (PtrObjEqualityFn(this, other)) {
      return true;
    }
    if (auto pseudo = other->isaPseudoSelector()) {
      if (pseudo->isClass()) {
        const auto& list = pseudo->selector();
        if (list == nullptr) return false;
        if (isSubselectorPseudo(pseudo->normalized())) {
          for (auto& complex : list->elements()) {
            if (complex->empty()) continue;
            for (auto& simple : complex->last()->selector()->elements()) {
              if (!isSuperselectorAF(simple)) {
                return false;
              }
            }
          }
          return true;
        }
      }
    }
    return false;
  }

  bool TypeSelector::isSuperselectorAF(SimpleSelector* other) const
  {
    if (isUniversal()) {
      return nsMatch(*other);
    }
    if (SimpleSelector::isSuperselectorAF(other)) {
      return true;
    }
    if (auto type = other->isaTypeSelector()) {
      return name_ == type->name_ && nsMatch(*type);
    }
    return false;
  }

  bool PseudoSelector::isSuperselectorAF(SimpleSelector* other) const
  {
    const auto& selector = this->selector();
    if (selector == nullptr) return PtrObjEqualityFn((SimpleSelector*)this, other);
    if (auto pseudo = other->isaPseudoSelector()) {
      if (isElement() &&
        pseudo->isElement() &&
        normalized_ == "slotted" &&
        pseudo->name_ == name_)
      {
        if (pseudo->selector() == nullptr) return false;
        return selector->isSuperselectorOf(pseudo->selector());
      }
    }
    return false;
  }

  bool PseudoSelector::isSuperSelector(PseudoSelector* other) const
  {
    if (SimpleSelector::isSuperselectorAF(other)) return true;

    const auto& selector = this->selector();
    if (selector == nullptr) return this == other;
    if (other->isaPseudoSelector() &&
      isElement() &&
      other->isElement() &&
      normalized_ == "slotted" &&
      other->name_ == name_)
    {
      // Fall back to the logic defined in functions.dart, which knows how to
      // compare selector pseudoclasses against raw selectors.
      if (other->selector() == nullptr) return false;
      return selector->isSuperselectorOf(other->selector());
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
