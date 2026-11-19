/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_CAPI_GUARD_HPP
#define SASS_CAPI_GUARD_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include <exception>
#include <iostream>

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Exception barrier for the C-API. C++ exceptions must never propagate
  // out of an `extern "C"` function, since the caller can't handle them
  // (undefined behavior, usually ending in `std::terminate`). Functions
  // that may throw for valid input wrap their body with these helpers,
  // which report the error on stderr and return the given fallback.
  /////////////////////////////////////////////////////////////////////////

  inline void reportCapiException(const char* fn, const char* what) noexcept
  {
    try { std::cerr << "LibSass C-API error in " << fn << ": " << what << std::endl; }
    catch (...) {} // Nothing more we can do
  }

  template <typename R, typename F>
  inline R capiGuard(const char* fn, R fallback, F&& body) noexcept
  {
    try { return body(); }
    catch (const std::exception& e) { reportCapiException(fn, e.what()); }
    catch (...) { reportCapiException(fn, "unknown exception"); }
    return fallback;
  }

  /////////////////////////////////////////////////////////////////////////
  // C callers can pass any integer as an enum value. Check the range
  // before storing it, since out-of-range values of these enums are
  // undefined in C++ and later code only handles the declared values.
  /////////////////////////////////////////////////////////////////////////
  template <typename E>
  inline bool capiEnumInRange(E value, E first, E last) noexcept
  {
    const long long v = static_cast<long long>(value);
    return v >= static_cast<long long>(first)
      && v <= static_cast<long long>(last);
  }
  template <typename F>
  inline void capiGuardVoid(const char* fn, F&& body) noexcept
  {
    try { body(); }
    catch (const std::exception& e) { reportCapiException(fn, e.what()); }
    catch (...) { reportCapiException(fn, "unknown exception"); }
  }

}

#endif
