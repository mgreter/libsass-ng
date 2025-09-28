#ifndef SASS_AST_HELPERS_H
#define SASS_AST_HELPERS_H

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"
#include <algorithm>
#include <functional>
#include "string_utils.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////#
  /////////////////////////////////////////////////////////////////////////#

  // ToDo: should this really be hard-coded
  // Note: most methods follow precision option
  const double NUMBER_EPSILON = 1e-12;

  // macro to test if numbers are equal within a small error margin
  // #define NEAR_EQUAL(lhs, rhs) ((lhs == rhs) || std::fabs(lhs - rhs) < NUMBER_EPSILON)

  // macro to test if numbers are equal within a small error margin
  // will also check for the case when both numbers are infinite
  #define NEAR_EQUAL_INF(lhs, rhs) ((lhs == rhs) || (std::fabs(lhs - rhs) < NUMBER_EPSILON))

  // macro to test if numbers are equal within a small error margin
  // will also check for the case when both numbers are infinite
  // #define FUZZY_EQUAL_INF(lhs, rhs, eps) ((lhs == rhs) || (std::fabs(lhs - rhs) < eps))

  /////////////////////////////////////////////////////////////////////////#
  // We define various functions and functors here.
  // Functions satisfy the BinaryPredicate requirement
  // Functors are structs used for e.g. unordered_map
  /////////////////////////////////////////////////////////////////////////#


  /////////////////////////////////////////////////////////////////////////#
  // Some STL helper functions
  /////////////////////////////////////////////////////////////////////////#

  // Check if all elements are equal
  // Currently only used in extend(lcs)
  template <class X, class Y,
    typename XT = typename X::value_type,
    typename YT = typename Y::value_type>
  inline bool ListEquality(const X& lhs, const Y& rhs,
    bool(*cmp)(const XT*, const YT*))
  {
    return lhs.size() == rhs.size() &&
      std::equal(lhs.begin(), lhs.end(),
        rhs.begin(), cmp);
  }

  // Return if Vector is empty
  template <class T>
  inline bool listIsEmpty(T* cnt) {
    return cnt && cnt->empty();
  }

  // Erase items from vector that match predicate
  // template<class T, class UnaryPredicate>
  // inline void listEraseItemIf(T& vec, UnaryPredicate* predicate)
  // {
  //   vec.erase(std::remove_if(vec.begin(), vec.end(), predicate), vec.end());
  // }

  // Check that every item in `lhs` is also in `rhs`
  // Note: this works by comparing the raw pointers
  template <typename T>
  inline bool listIsSubsetOrEqual(const T& lhs, const T& rhs) {
    for (const auto& item : lhs) {
      if (std::find(rhs.begin(), rhs.end(), item) == rhs.end())
        return false;
    }
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  // Returns whether [name] is the name of a pseudo-element
  // that can be written with pseudo-class syntax (CSS2 vs CSS3):
  // `:before`, `:after`, `:first-line`, or `:first-letter`
  /////////////////////////////////////////////////////////////////////////
  inline bool isFakePseudoElement(const sass::string& name)
  {
    switch (name.size()) {
    case 5: return StringUtils::equalsIgnoreCase(name, "after", 5);
    case 6: return StringUtils::equalsIgnoreCase(name, "before", 6);
    case 10: return StringUtils::equalsIgnoreCase(name, "first-line", 10);
    case 12: return StringUtils::equalsIgnoreCase(name, "first-letter", 12);
    default: return false;
    }
  }

  /////////////////////////////////////////////////////////////////////////
  // Names of pseudo selectors that take selectors as arguments,
  // and that are subselectors of their arguments.
  // For example, `.foo` is a superselector of `:matches(.foo)`.
  /////////////////////////////////////////////////////////////////////////
  inline bool isSubselectorPseudo(const sass::string& norm)
  {
    switch (norm.size()) {
    case 2: return StringUtils::equalsIgnoreCase(norm, "is", 2);
    case 3: return StringUtils::equalsIgnoreCase(norm, "any", 3);
    case 5: return StringUtils::equalsIgnoreCase(norm, "where", 5);
    case 7: return StringUtils::equalsIgnoreCase(norm, "matches", 7);
    case 9: return StringUtils::equalsIgnoreCase(norm, "nth-child", 9);
    case 14: return StringUtils::equalsIgnoreCase(norm, "nth-last-child", 14);;
    default: return false;
    }
  }
  // EO isSubselectorPseudo

  /////////////////////////////////////////////////////////////////////////#
  // Pseudo-class selectors that take unadorned selectors as arguments.
  /////////////////////////////////////////////////////////////////////////#
  inline bool isSelectorPseudoClass(const sass::string& test)
  {
    switch (test.size()) {
    case 2: return StringUtils::equalsIgnoreCase(test, "is", 2);
    case 3: return StringUtils::equalsIgnoreCase(test, "not", 3)
                || StringUtils::equalsIgnoreCase(test, "any", 3)
                || StringUtils::equalsIgnoreCase(test, "has", 3);
    case 4: return StringUtils::equalsIgnoreCase(test, "host", 4);
    case 5: return StringUtils::equalsIgnoreCase(test, "where", 5);
    case 7: return StringUtils::equalsIgnoreCase(test, "matches", 7)
                || StringUtils::equalsIgnoreCase(test, "current", 7);
    case 12: return StringUtils::equalsIgnoreCase(test, "host-context", 12);
    default: return false;
    }
  }
  // EO isSelectorPseudoClass

  /////////////////////////////////////////////////////////////////////////#
  // Pseudo-element selectors that take unadorned selectors as arguments.
  /////////////////////////////////////////////////////////////////////////#
  inline bool isSelectorPseudoElement(const sass::string& test)
  {
    return StringUtils::equalsIgnoreCase(test, "slotted", 7);
  }
  // EO isSelectorPseudoElement

  /////////////////////////////////////////////////////////////////////////#
  // Pseudo-element selectors that has binomials
  /////////////////////////////////////////////////////////////////////////#
  // inline bool isSelectorPseudoBinominal(const sass::string& test)
  // {
  //   return StringUtils::equalsIgnoreCase(test, "nth-child", 11)
  //     || StringUtils::equalsIgnoreCase(test, "nth-last-child", 14);
  // }
  // EO isSelectorPseudoBinominal

  /////////////////////////////////////////////////////////////////////////#
  /////////////////////////////////////////////////////////////////////////#

}

#endif
