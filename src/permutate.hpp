/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Algorithm helpers fomr some dart-sass implementations
/*****************************************************************************/
#ifndef SASS_PERMUTATE_HPP
#define SASS_PERMUTATE_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include <cstddef>
#include <vector>
#include "backtrace.hpp"
#include "exceptions.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Returns the number of paths [permutate] will return for [in], or
  // `limit + 1` if there are more than [limit] (avoids any overflow).
  template <class T>
  size_t permutationCount(const sass::vector<sass::vector<T>>& in, size_t limit)
  {
    size_t count = 1;
    for (const auto& group : in) {
      if (group.empty()) return 0;
      if (count > limit / group.size()) return limit + 1;
      count *= group.size();
    }
    return count;
  }

  // Throws if a selector list grew to more than `SassMaxSelectorPaths`
  // complex selectors (e.g. by repeated `@extend` or deep nesting).
  inline void assertSelectorListLimit(size_t size,
    const BackTraces& traces, const SourceSpan& pstate)
  {
    if (size > SassMaxSelectorPaths) {
      // Add the location on a copy (leaves the callers stack intact)
      BackTraces stack(traces);
      stack.push_back(BackTrace(pstate));
      throw Exception::SelectorTooLarge(stack);
    }
  }

  // Throws if [permutate] would return more than `SassMaxSelectorPaths`
  // paths for [in], before allocating anything. Returns the count.
  template <class T>
  size_t assertPermutationLimit(const sass::vector<sass::vector<T>>& in,
    const BackTraces& traces, const SourceSpan& pstate)
  {
    const size_t count = permutationCount(in, SassMaxSelectorPaths);
    assertSelectorListLimit(count, traces, pstate);
    return count;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Returns a list of all possible paths through the given lists.
  //
  // For example, given `[[1, 2], [3, 4], [5, 6]]`, this returns:
  //
  // ```
  // [[1, 3, 5],
  //  [2, 3, 5],
  //  [1, 4, 5],
  //  [2, 4, 5],
  //  [1, 3, 6],
  //  [2, 3, 6],
  //  [1, 4, 6],
  //  [2, 4, 6]]
  // ```
  // 
  // Note: called `paths` in dart-sass
  template <class T>
  sass::vector<sass::vector<T>> permutate(
    const sass::vector<sass::vector<T>>& in,
    const BackTraces& traces, const SourceSpan& pstate)
  {

    const size_t count = assertPermutationLimit(in, traces, pstate);

    const size_t L = in.size();
    size_t n = 0;

    if (L == 0) return {};
    // Exit early if any entry is empty
    for (size_t i = 0; i < L; i += 1) {
      if (in[i].size() == 0) return {};
    }

    // std::array<size_t, L + 1> state;
    std::vector<size_t> state;
    state.resize(L + 1);
    // size_t* state = new size_t[L + 1]{};
    sass::vector<sass::vector<T>> out;
    out.reserve(count); // exact and bounded by the limit

    // First initialize all states for every permutation group
    for (size_t i = 0; i < L; i += 1) {
      state[i] = in[i].size() - 1;
    }
    while (true) {
      sass::vector<T> perm;
      perm.reserve(L);
      // Create one permutation for state
      for (size_t i = 0; i < L; i += 1) {
        perm.emplace_back(in.at(i).at(in[i].size() - state[i] - 1));
      }
      // Current group finished
      if (state[n] == 0) {
        // Find position of next decrement
        while (n < L && state[++n] == 0) {}

        if (n == L) {
          out.emplace_back(std::move(perm));
          break;
        }

        state[n] -= 1;

        for (size_t p = 0; p < n; p += 1) {
          state[p] = in[p].size() - 1;
        }

        // Restart from front
        n = 0;

      }
      else {
        state[n] -= 1;
      }
      out.emplace_back(std::move(perm));
    }

    // delete[] state;
    return out;
  }
  // EO permutate

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // ToDo: this variant is used in resolveParentSelectors
  // Returns a list of all possible paths through the given lists.
  //
  // For example, given `[[1, 2], [3, 4], [5, 6]]`, this returns:
  //
  // ```
  // [[1, 3, 5],
  //  [1, 3, 6],
  //  [1, 4, 5],
  //  [1, 4, 6],
  //  [2, 3, 5],
  //  [2, 3, 6],
  //  [2, 4, 5],
  //  [2, 4, 6]]
  // ```
  //
  // Used by resolveParentSelectors
  template <class T>
  sass::vector<sass::vector<T>> permutateAlt(
    const sass::vector<sass::vector<T>>& in,
    const BackTraces& traces, const SourceSpan& pstate) {

    const size_t count = assertPermutationLimit(in, traces, pstate);

    const size_t L = in.size();
    size_t n = in.size() - 1;

    if (L == 0) return {};
    // Exit early if any entry is empty
    for (size_t i = 0; i < L; i += 1) {
      if (in[i].size() == 0) return {};
    }

    std::vector<size_t> state;
    state.resize(L + 1);
    // size_t* state = new size_t[L];
    sass::vector<sass::vector<T>> out;
    out.reserve(count); // exact and bounded by the limit

    // First initialize all states for every permutation group
    for (size_t i = 0; i < L; i += 1) {
      state[i] = in[i].size() - 1;
    }

    while (true) {
      sass::vector<T> perm;
      perm.reserve(L);
      // Create one permutation for state
      for (size_t i = 0; i < L; i += 1) {
        perm.emplace_back(in.at(i).at(in[i].size() - state[i] - 1));
      }
      // Current group finished
      if (state[n] == 0) {
        // Find position of next decrement
        while (n > 0 && state[--n] == 0) {}
        // Check for end condition
        if (state[n] != 0) {
          // Decrease next on the left side
          state[n] -= 1;
          // Reset all counters to the right
          for (size_t p = n + 1; p < L; p += 1) {
            state[p] = in[p].size() - 1;
          }
          // Restart from end
          n = L - 1;
        }
        else {
          out.emplace_back(std::move(perm));
          break;
        }
      }
      else {
        state[n] -= 1;
      }
      out.emplace_back(std::move(perm));
    }

    // delete[] state;
    return out;
  }
  // EO permutateAlt

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
