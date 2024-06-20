/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "ast_selectors.hpp"

#include "permutate.hpp"
#include "dart_helpers.hpp"

namespace Sass {

  static bool hasRootish(const CompoundSelector* compound)
  {
    for (const SimpleSelector* simple : compound->elements()) {
      if (const PseudoSelector* pseudo = simple->isaPseudoSelector()) {
        if (pseudo->isClass()) {
          if (pseudo->normalized() == "root") return true;
          if (pseudo->normalized() == "scope") return true;
          if (pseudo->normalized() == "host") return true;
          if (pseudo->normalized() == "host-context") return true;
        }
      }
    }
    return false;
  }
  // EO hasRoot

  /////////////////////////////////////////////////////////////////////////
  // Returns whether a [CompoundSelector] may contain only
  // one simple selector of the same type as [simple].
  /////////////////////////////////////////////////////////////////////////
  static bool isUnique(const SimpleSelector* simple)
  {
    if (simple->isaIDSelector()) return true;
    if (const PseudoSelector* pseudo = simple->isaPseudoSelector()) {
      if (pseudo->isPseudoElement()) return true;
    }
    return false;
  }
  // EO isUnique

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [complex1] and [complex2] need to be unified to
  // produce a valid combined selector. This is necessary when both
  // selectors contain the same unique simple selector, such as an ID.
  /////////////////////////////////////////////////////////////////////////
  static bool mustUnify(
    const CplxSelComponentVector& complex1,
    const CplxSelComponentVector& complex2)
  {

    sass::vector<const SimpleSelector*> uniqueSelectors1;
    for (const CplxSelComponent* component : complex1) {
      if (const CompoundSelector* compound = component->selector()) {
        for (const SimpleSelector* sel : compound->elements()) {
          if (isUnique(sel)) {
            uniqueSelectors1.emplace_back(sel);
          }
        }
      }
    }
    if (uniqueSelectors1.empty()) return false;
    for (const CplxSelComponent* component : complex2) {
      if (const CompoundSelector* compound = component->selector()) {
        for (const SimpleSelector* sel : compound->elements()) {
          if (isUnique(sel)) {
            for (auto check : uniqueSelectors1) {
              if (*check == *sel) return true;
            }
          }
        }
      }
    }

    return false;
  }
  // EO isUnique

  /////////////////////////////////////////////////////////////////////////
  // Helper function used by `weaveParents`
  /////////////////////////////////////////////////////////////////////////
  static bool cmpGroups(
    const CplxSelComponentVector& group1,
    const CplxSelComponentVector& group2,
    CplxSelComponentVector& select)
  {

    if (ListEquality(group1, group2, PtrObjEqualityFn<CplxSelComponent>))
    {
      select = group1;
      return true;
    }

    if (!group1.front()->selector()) {
      select.clear();
      return false;
    }
    if (!group2.front()->selector()) {
      select.clear();
      return false;
    }

    if (complexIsParentSuperselector(group1, group2)) {
      select = group2;
      return true;
    }
    if (complexIsParentSuperselector(group2, group1)) {
      select = group1;
      return true;
    }

    if (!mustUnify(group1, group2)) {
      select.clear();
      return false;
    }

    auto span = SourceSpan::internal32("[BASE]");
    CplxSelComponentVector comp1(group1);
    CplxSelComponentVector comp2(group2);
    ComplexSelectorObj q1 = SASS_MEMORY_NEW(ComplexSelector, span, std::move(comp1));
    ComplexSelectorObj q2 = SASS_MEMORY_NEW(ComplexSelector, span, std::move(comp2));
    ComplexSelectors unified = _unifyComplex({ q2, q1 }, span);
    if (unified.size() == 1) {
      select = unified[0]->elements();
    }
    else {
      return false;
    }
    return true;
  }
  // EO cmpGroups

  /////////////////////////////////////////////////////////////////////////
  // Helper function used by `weaveParents`
  /////////////////////////////////////////////////////////////////////////
  template <class T>
  bool checkForEmptyChild(const T& item) {
    return item.empty();
  }
  // EO checkForEmptyChild

  /////////////////////////////////////////////////////////////////////////
  // Helper function used by `weaveParents`
  /////////////////////////////////////////////////////////////////////////
  static bool cmpChunkForEmptySequence(
    const sass::vector<CplxSelComponentVector>& seq,
    const CplxSelComponentVector& group)
  {
    return seq.empty();
  }
  // EO cmpChunkForEmptySequence

  /////////////////////////////////////////////////////////////////////////
  // Helper function used by `weaveParents`
  /////////////////////////////////////////////////////////////////////////
  static bool cmpChunkForParentSuperselector(
    const sass::vector<CplxSelComponentVector>& seq,
    const CplxSelComponentVector& group)
  {
    return seq.empty() || complexIsParentSuperselector(seq.front(), group);
  }
  // EO cmpChunkForParentSuperselector

 /////////////////////////////////////////////////////////////////////////
 // Returns all orderings of initial subsequences of [queue1] and [queue2].
 // The [done] callback is used to determine the extent of the initial
 // subsequences. It's called with each queue until it returns `true`.
 // Destructively removes the initial subsequences of [queue1] and [queue2].
 // For example, given `(A B C | D E)` and `(1 2 | 3 4 5)` (with `|` denoting
 // the boundary of the initial subsequence), this would return `[(A B C 1 2),
 // (1 2 A B C)]`. The queues would then contain `(D E)` and `(3 4 5)`.
 /////////////////////////////////////////////////////////////////////////
  template <class T>
  sass::vector<sass::vector<T>> getChunks(
    sass::vector<T>& queue1, sass::vector<T>& queue2,
    const T& group, bool(*done)(const sass::vector<T>&, const T&)
  ) {

    sass::vector<T> chunk1;
    while (!done(queue1, group)) {
      chunk1.emplace_back(queue1.front());
      queue1.erase(queue1.begin());
    }

    sass::vector<T> chunk2;
    while (!done(queue2, group)) {
      chunk2.emplace_back(queue2.front());
      queue2.erase(queue2.begin());
    }

    if (chunk1.empty() && chunk2.empty()) return {};
    else if (chunk1.empty()) {
      return { chunk2 };
    }
    else if (chunk2.empty()) {
      return { chunk1 };
    }

    sass::vector<sass::vector<T>> result;
    result.emplace_back(chunk1);
    result.emplace_back(chunk2);
    result.front().insert(result.front().end(),
      std::make_move_iterator(chunk2.begin()),
      std::make_move_iterator(chunk2.end()));
    result.back().insert(result.back().end(),
      std::make_move_iterator(chunk1.begin()),
      std::make_move_iterator(chunk1.end()));
    return result;
  }
  // EO getChunks

  /////////////////////////////////////////////////////////////////////////
  // If the first element of [queue] has a `::root`
  // selector, removes and returns that element.
  /////////////////////////////////////////////////////////////////////////
  // static CplxSelComponentObj getFirstIfRoot(CplxSelComponentVector& queue) {
  // }
  // EO getFirstIfRoot

  static CplxSelComponentObj _firstIfRootish(CplxSelComponentVector& queue) {
    if (queue.empty()) return {};
    CplxSelComponent* first = queue.front();
    if (CompoundSelector* sel = first->selector()) {

      if (!hasRootish(sel)) return {};
      queue.erase(queue.begin());
      return first;

    }
    return {};
  }
  // EO getFirstIfRoot

  

  /////////////////////////////////////////////////////////////////////////
  // Returns [complex], grouped into sub-lists such that no sub-list
  // contains two adjacent [ComplexSelector]s. For example,
  // `(A B > C D + E ~ > G)` is grouped into `[(A) (B > C) (D + E ~ > G)]`.
  /////////////////////////////////////////////////////////////////////////
  static sass::vector<CplxSelComponentVector> groupSelectors(
    const CplxSelComponentVector& components)
  {
    sass::vector<CplxSelComponentVector> groups;
    CplxSelComponentVector group;
    for (const auto& component : components) {
      group.push_back(component);
      if (component->combinators().empty()) {
        groups.emplace_back(std::move(group));
        group.clear(); // needed after move?
      }
    }
    if (!group.empty()) {
      groups.emplace_back(group);
    }
    return groups;
  }
  // EO groupSelectors

  /////////////////////////////////////////////////////////////////////////
  // Extracts leading [Combinator]s from [components1] and [components2]
  // and merges them together into a single list of combinators.
  // If there are no combinators to be merged, returns an empty list.
  // If the combinators can't be merged, returns `null`.
  /////////////////////////////////////////////////////////////////////////
  /*
  static bool mergeInitialCombinators(
    CplxSelComponentVector& components1,
    CplxSelComponentVector& components2,
    CplxSelComponentVector& result)
  {

    CplxSelComponentVector combinators1;
    while (!components1.empty() && components1.front()->selector()) {
      // SelectorCombinator* front = components1.front()->isaSelectorCombinator();
      components1.erase(components1.begin());
      // combinators1.emplace_back(front);
    }

    CplxSelComponentVector combinators2;
    while (!components2.empty() && components2.front()->selector()) {
      // SelectorCombinator* front = components2.front()->isaSelectorCombinator();
      components2.erase(components2.begin());
      // combinators2.emplace_back(front);
    }

    // If neither sequence of combinators is a subsequence
    // of the other, they cannot be merged successfully.
    CplxSelComponentVector LCS = lcs<CplxSelComponentObj>(combinators1, combinators2);

    if (ListEquality(LCS, combinators1, PtrObjEqualityFn<CplxSelComponent>)) {
      result = combinators2;
      return true;
    }
    if (ListEquality(LCS, combinators2, PtrObjEqualityFn<CplxSelComponent>)) {
      result = combinators1;
      return true;
    }

    return false;

  }
  */
  // EO mergeInitialCombinators


  /////////////////////////////////////////////////////////////////////////
  // Expands "parenthesized selectors" in [complexes]. That is, if
  // we have `.A .B {@extend .C}` and `.D .C {...}`, this conceptually
  // expands into `.D .C, .D (.A .B)`, and this function translates
  // `.D (.A .B)` into `.D .A .B, .A .D .B`. For thoroughness, `.A.D .B`
  // would also be required, but including merged selectors results in
  // exponential output for very little gain. The selector `.D (.A .B)`
  // is represented as the list `[[.D], [.A, .B]]`.
  /////////////////////////////////////////////////////////////////////////

  /// Expands "parenthesized selectors" in [complexes].
  ///
  /// That is, if we have `.A .B {@extend .C}` and `.D .C {...}`, this
  /// conceptually expands into `.D .C, .D (.A .B)`, and this function translates
  /// `.D (.A .B)` into `.D .A .B, .A .D .B`. For thoroughness, `.A.D .B` would
  /// also be required, but including merged selectors results in exponential
  /// output for very little gain.
  ///
  /// The selector `.D (.A .B)` is represented as the list `[.D, .A .B]`.
  ///
  /// The [span] will be used for any new combined selectors.
  ///
  /// If [forceLineBreak] is `true`, this will mark all returned complex selectors
  /// as having line breaks.

  ComplexSelectors weave27(
    const ComplexSelectors& complexes,
    bool forceLineBreak)
  {

    if (complexes.empty()) return complexes;

    if (complexes.size() == 1) {
      return complexes;
    }

    ComplexSelectors prefixes;
    prefixes.emplace_back(complexes.front());

    for (size_t i = 1; i < complexes.size(); i += 1) {
      const ComplexSelectorObj& complex = complexes[i];
      if (complex->elements().size() == 1) {
        for (auto& prefix : prefixes) {
          prefix = prefix->concatenate(complex, complex->pstate(), forceLineBreak);
        }
        continue;
      }

      ComplexSelectors newPrefixes;
      for (const ComplexSelectorObj& prefix : prefixes) {
        ComplexSelectors weaveds
          = weaveParents(prefix, complex);
        if (weaveds.empty()) continue;
        for (const auto& parent : weaveds) {
          SourceSpan span(complex->pstate());
          ComplexSelectorObj asd = parent->withAdditionalComponent(
            complex->elements().back(), span, forceLineBreak);
          newPrefixes.push_back(asd);
        }
      }
      prefixes = newPrefixes;

    }
    return prefixes;

  }
  // EO weave

  static bool _mergeLeadingCombinators(
    const SelectorCombinatorVector& combinators1,
    const SelectorCombinatorVector& combinators2,
    SelectorCombinatorVector& result)
  {
    if (combinators1.empty()) {
      result = combinators2;
    }
    else if (combinators2.empty()) {
      result = combinators1;
    }
    else if (combinators1.size() > 1) {
      // Nothing to add in this case?
    }
    else if (combinators2.size() > 1) {
      // Nothing to add in this case?
    }
    else if (ListEquality(combinators1, combinators2, PtrObjEqualityFn<SelectorCombinator>)) {
      result = combinators1;
    }
    return true;
  }
    // // Allow null arguments just to make calls to `Iterable.reduce()` easier.
    // switch ((combinators1, combinators2)) {
    //   (null, _) || (_, null) = > null,
    //     (List(length: > 1), _) || (_, List(length: > 1)) = > null,
    //     ([], var combinators) || (var combinators, []) = > combinators,
    //     _ = > listEquals(combinators1, combinators2) ? combinators1 : null
    // };


  template<class T>
  T backOrNull(sass::vector<T> asd) {
    if (asd.size() == 0) return nullptr;
    return asd.back();
  }


  /////////////////////////////////////////////////////////////////////////
  // Interweaves [parents1] and [parents2] as parents of the same target
  // selector. Returns all possible orderings of the selectors in the
  // inputs (including using unification) that maintain the relative
  // ordering of the input. For example, given `.foo .bar` and `.baz .bang`,
  // this would return `.foo .bar .baz .bang`, `.foo .bar.baz .bang`,
  // `.foo .baz .bar .bang`, `.foo .baz .bar.bang`, `.foo .baz .bang .bar`,
  // and so on until `.baz .bang .foo .bar`. Semantically, for selectors A
  // and B, this returns all selectors `AB_i` such that the union over all i
  // of elements matched by `AB_i X` is identical to the intersection of all
  // elements matched by `A X` and all elements matched by `B X`. Some `AB_i`
  // are elided to reduce the size of the output.
  /////////////////////////////////////////////////////////////////////////
  static CompoundSelector* unifyCompound(
    CompoundSelector* compound1,
    CompoundSelector* compound2)
  {
    // Optimize case when nothing is changed
    if (compound1->empty()) return compound1;
    // Make a copy of the existing elements (ToDo: optimize)
    SimpleSelectors result(compound2->elements());
    for (const auto& simple : compound1->elements()) {
      result = simple->unify(result);
      if (result.empty()) return nullptr;
    }
    return SASS_MEMORY_NEW(CompoundSelector,
      compound1->pstate(), std::move(result));
  }


  static bool _mergeTrailingCombinators(const SourceSpan& span,
    CplxSelComponentVector& components1, CplxSelComponentVector& components2,
    sass::vector<sass::vector<CplxSelComponentVector>>& result)
  {

    // std::cerr << "process trailing\n";

    // for (const auto& c1 : components1) { if (c1) std::cerr << "merge trails in1 " << c1->inspecter() << "\n"; }
    // for (const auto& c2 : components2) { if (c2) std::cerr << "merge trails in2 " << c2->inspecter() << "\n"; }

    SelectorCombinatorVector combinators1, combinators2;
    if (components1.size() > 0) combinators1 = components1.back()->combinators();
    if (components2.size() > 0) combinators2 = components2.back()->combinators();

    if (combinators1.empty() && combinators2.empty()) return true;
    if (combinators1.size() > 1 || combinators2.size() > 1) return false;

    const auto& first1 = combinators1.empty() ? nullptr : combinators1.front();
    const auto& first2 = combinators2.empty() ? nullptr : combinators2.front();

    // if (first1 == nullptr) std::cerr << " first1 null\n";
    // else std::cerr << " first1 " << first1->toString() << "\n";
    // if (first2 == nullptr) std::cerr << " first2 null\n";
    // else std::cerr << " first2 " << first2->toString() << "\n";

    if (first1 != nullptr && first2 != nullptr)
    {
      if (first1->isFollowingSibling() && first2->isFollowingSibling()) {
        if (!components1.empty() && !components2.empty()) {
          // const auto& front = combinators1.front();
          const auto& component1 = components1.back();
          const auto& component2 = components2.back();
          if (component1->selector()->isSuperselectorOf(component2->selector())) {
            result.push_back({ { component2 } });
          }
          else if (component2->selector()->isSuperselectorOf(component1->selector())) {
            result.push_back({ { component1 } });
          }
          else {
            sass::vector<CplxSelComponentVector> choices;
            choices.push_back({ component1, component2 });
            choices.push_back({ component2, component1 });
            if (CompoundSelectorObj unified = unifyCompound(
              component1->selector(), component2->selector())) {
              choices.push_back({ SASS_MEMORY_NEW(CplxSelComponent,
                 span, { first1 }, unified) });
            }
            result.push_back(choices);
          }
          components1.pop_back(); // consumed
          components2.pop_back(); // consumed
        }
        else {
          std::cerr << "The container or combinator was empty!!!???\n";
        }
        // std::cerr << "Merge case 1\n";
      }
      else if (first1->isFollowingSibling() && first2->isNextSibling()) {
        const auto& next = components2.back();
        const auto& following = components1.back();

        // std::cerr << "next1 " << next->inspecter() << "\n";
        // std::cerr << "following1 " << following->inspecter() << "\n";

        if (following->selector()->isSuperselectorOf(next->selector())) {
          result.push_back({ { next } });
        }
        else if (auto unified = unifyCompound(following->selector(), next->selector()))
        {
          SelectorCombinatorVector asd = next->combinators();
          result.push_back({
            {following, next},
            { new CplxSelComponent(span, std::move(asd), unified)}
            });
        }
        else {
          result.push_back({
            {following, next}
            });
        }
        components1.pop_back(); // consumed
        components2.pop_back(); // consumed
        // std::cerr << "Merge case 2a\n";
      }
      else if (first1->isNextSibling() && first2->isFollowingSibling()) {
        const auto& next = components1.back();
        const auto& following = components2.back();

        // std::cerr << "next2 " << next->inspecter() << "\n";
        // std::cerr << "following2 " << following->inspecter() << "\n";

        if (following->selector()->isSuperselectorOf(next->selector())) {
          result.push_back({ { next } });
        }
        else if (auto unified = unifyCompound(following->selector(), next->selector()))
        {
          SelectorCombinatorVector asd = next->combinators();
          result.push_back({
            {following, next},
            { new CplxSelComponent(span, std::move(asd), unified)}
            });
        }
        else {
          result.push_back({
            {following, next}
            });
        }
        components1.pop_back(); // consumed
        components2.pop_back(); // consumed
        // std::cerr << "Merge case 2b\n";
      }
      else if (first1->isChild() && !first2->isChild()) {
        result.push_back({ { components2.back() } });
        components2.pop_back(); // has been consumed
        // std::cerr << "Merge case 3\n";
      }
      else if (!first1->isChild() && first2->isChild()) {
        result.push_back({ { components1.back() } });
        components1.pop_back(); // has been consumed
        // std::cerr << "Merge case 3\n";
      }
      else if (first1->combinator() == first2->combinator()) {

        if (!components1.empty() && !components2.empty()) {

          // const auto& front = combinators1.front();
          const auto& last1 = components1.back();
          const auto& last2 = components2.back();

          CompoundSelectorObj unified = unifyCompound(
            last1->selector(), last2->selector());
          if (unified == nullptr) return false;

          result.push_back({ { SASS_MEMORY_NEW(CplxSelComponent, span, { first1 }, unified) } });

          components1.pop_back();
          components2.pop_back();

          // std::cerr << " cmp " << unified->inspect() << "\n";


        }
        else {
          std::cerr << "The container or combinator was empty!!!???\n";
        }


        // std::cerr << "Merge case 4\n";
      }
      else {
        // std::cerr << "Merge case other\n";
        return false;
      }
    }
    else if (first1 != nullptr)
    {
      auto descendantComponents = backOrNull(components2);
      auto combinatorComponents = backOrNull(components1);

      //std::cerr << "descendantComponents " << descendantComponents->inspecter() << "\n";
      //std::cerr << "combinatorComponents " << combinatorComponents->inspecter() << "\n";

      if (first1->isChild()) {
        if (descendantComponents && descendantComponents->selector()->isSuperselectorOf(combinatorComponents->selector())) {
          components2.pop_back();
        }
      }
      result.push_back({ { components1.back() } });
      components1.pop_back();
      // std::cerr << "Merge case 5a\n";
    }
    else if (first2 != nullptr)
    {
      auto descendantComponents = backOrNull(components1);
      auto combinatorComponents = backOrNull(components2);
      if (first2->isChild()) {
        if (descendantComponents && descendantComponents->selector()->isSuperselectorOf(combinatorComponents->selector())) {
          components1.pop_back();
        }
      }
      result.push_back({ { components2.back() } });
      components2.pop_back();
      // std::cerr << "Merge case 5b\n";
    }
    else {
      return false;
    }

    return _mergeTrailingCombinators(span, components1, components2, result);;

  }



  /////////////////////////////////////////////////////////////////////////
  // Extracts trailing [Combinator]s, and the selectors to which they apply,
  // from [components1] and [components2] and merges them together into a
  // single list. If there are no combinators to be merged, returns an
  // empty list. If the sequences can't be merged, returns `null`.
  /////////////////////////////////////////////////////////////////////////
  static bool mergeFinalCombinators(
    CplxSelComponentVector& components1,
    CplxSelComponentVector& components2,
    sass::vector<sass::vector<CplxSelComponentVector>>& result)
  {

    if (components1.empty() || components1.back()->combinators().empty()) {
      if (components2.empty() || components2.back()->combinators().empty()) {
        return true;
      }
    }

    CplxSelComponentVector combinators1;
    while (!components1.empty() && components1.back()->combinators().size() != 0) {
      //SelectorCombinatorObj back = components1.back()->combinators();
      components1.erase(components1.end() - 1);
      //combinators1.emplace_back(back);
    }

    CplxSelComponentVector combinators2;
    while (!components2.empty() && components2.back()->combinators().size() != 0) {
      //SelectorCombinatorObj back = components2.back()->combinators();
      components2.erase(components2.end() - 1);
      //combinators2.emplace_back(back);
    }

    // reverse now as we used emplace_back (faster than new alloc)
    std::reverse(combinators1.begin(), combinators1.end());
    std::reverse(combinators2.begin(), combinators2.end());

    if (combinators1.size() > 1 || combinators2.size() > 1) {
      // If there are multiple combinators, something strange going on. If one
      // is a super-sequence of the other, use that, otherwise give up.
      auto LCS = lcs<CplxSelComponentObj>(combinators1, combinators2);
      if (ListEquality(LCS, combinators1, PtrObjEqualityFn<CplxSelComponent>)) {
        result.push_back({ combinators2 });
      }
      else if (ListEquality(LCS, combinators2, PtrObjEqualityFn<CplxSelComponent>)) {
        result.push_back({ combinators1 });
      }
      else {
        return false;
      }
      return true;
    }

    // This code looks complicated, but it's actually just a bunch of special
    // cases for interactions between different combinators.
    SelectorCombinatorObj combinator1, combinator2;
    //if (!combinators1.empty()) combinator1 = combinators1.back()->isaSelectorCombinator();
    //if (!combinators2.empty()) combinator2 = combinators2.back()->isaSelectorCombinator();

    if (!combinator1.isNull() && !combinator2.isNull()) {

      components1.pop_back();
      components2.pop_back();

      return mergeFinalCombinators(components1, components2, result);

    }
    return mergeFinalCombinators(components1, components2, result);

  }
  // EO mergeFinalCombinators


  ComplexSelectors weaveParents(
    ComplexSelector* prefix, ComplexSelector* base)
  {

    SelectorCombinatorVector lead;
    bool rs1 = _mergeLeadingCombinators(
      prefix->leadingCombinators(),
      base->leadingCombinators(),
      lead);

    // _mergeLeadingCombinators must report success or not
    if (rs1 == false) return {};

    if (base->empty()) {
      throw "Need base";
    }

    CplxSelComponentVector leads{};


    CplxSelComponentVector queue1(prefix->begin(), prefix->end());
    CplxSelComponentVector queue2(base->begin(),
      base->begin() == base->end() ? base->end() : base->end() - 1);

    sass::vector<sass::vector<CplxSelComponentVector>> trails{};
    bool ok = _mergeTrailingCombinators(
      base->pstate(), queue1, queue2, trails);

    if (ok == false) return {};

    // list comes out in reverse order for performance
    std::reverse(trails.begin(), trails.end());

    // Make sure there's at most one `:root` in the output.
    // Note: does not yet do anything in libsass (no root selector)
    CplxSelComponentObj root1(_firstIfRootish(queue1));
    CplxSelComponentObj root2(_firstIfRootish(queue2));

    if (!root1.isNull() && !root2.isNull()) {
      // CompoundSelectorObj root = root1->selector()->unifyWith(root2->selector());
      CompoundSelectorObj root = unifyCompound(root1->selector(), root2->selector());
      if (root.isNull()) return {}; // null
      queue1.insert(queue1.begin(), root.ptr()->wrapInComponent(root1->combinators()));
      queue2.insert(queue2.begin(), root.ptr()->wrapInComponent(root2->combinators()));
    }
    else if (!root1.isNull()) {
      queue1.insert(queue1.begin(), root1.ptr());
      queue2.insert(queue2.begin(), root1.ptr());
    }
    else if (!root2.isNull()) {
      queue1.insert(queue1.begin(), root2.ptr());
      queue2.insert(queue2.begin(), root2.ptr());
    }

    // group into sub-lists so no sub-list contains two adjacent ComplexSelectors.
    sass::vector<CplxSelComponentVector> groups1 = groupSelectors(queue1);
    sass::vector<CplxSelComponentVector> groups2 = groupSelectors(queue2);

    // The main array to store our choices that will be permutated
    sass::vector<sass::vector<CplxSelComponentVector>> choices;

    sass::vector<CplxSelComponentVector> LCS =
      lcs<CplxSelComponentVector>(groups1, groups2, cmpGroups);

    for (const auto& group : LCS) {

      // Create junks from groups1 and groups2
      sass::vector<sass::vector<CplxSelComponentVector>>
        chunks = getChunks<CplxSelComponentVector>(
          groups1, groups2, group, cmpChunkForParentSuperselector);

      // Create expanded array by flattening chunks2 inner
      sass::vector<CplxSelComponentVector>
        expanded = flattenInner(chunks);

      // Prepare data structures
      choices.emplace_back(expanded);
      choices.push_back({ group });
      if (!groups1.empty()) {
        groups1.erase(groups1.begin());
      }
      if (!groups2.empty()) {
        groups2.erase(groups2.begin());
      }

    }

    // Create junks from groups1 and groups2
    sass::vector<sass::vector<CplxSelComponentVector>>
      chunks = getChunks<CplxSelComponentVector>(
        groups1, groups2, {}, cmpChunkForEmptySequence);

    // Append chunks with inner arrays flattened
    choices.emplace_back(flattenInner(chunks));

    // append all trailing selectors to choices
    choices.insert(choices.end(),
      std::make_move_iterator(trails.begin()),
      std::make_move_iterator(trails.end()));

    // move all non empty items to the front, then erase the trailing ones
    choices.erase(std::remove_if(choices.begin(), choices.end(), checkForEmptyChild
      <sass::vector<CplxSelComponentVector>>), choices.end());

    auto perm = permutate(choices);

    ComplexSelectors foobar;
    for (const auto& path : perm) {
      CplxSelComponentVector comps;
      for (const auto& compis : path) {
        for (const auto& compa : compis) {
          comps.push_back(compa);
        }
      }
      auto cply = SASS_MEMORY_NEW(ComplexSelector, base->pstate(),
        lead, std::move(comps));
      foobar.push_back(cply);
    }

    return foobar;

  }
  // EO weaveParents

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
