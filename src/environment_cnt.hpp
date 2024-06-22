/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_ENVIRONMENT_CNT_H
#define SASS_ENVIRONMENT_CNT_H

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "hashing.hpp"
#include "ast_fwd_decl.hpp"
#include "environment_key.hpp"
#include "comparators.hpp"
#include "containers.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Set of env-keys mainly used for error reporting
  // To determine during parsing if key is duplicated
  /////////////////////////////////////////////////////////////////////////

  using EnvKeySet = sass::set::unordered::env<EnvKey>;

  /////////////////////////////////////////////////////////////////////////
  // These get barely any use in tight-loops (no gains from optimizing)
  /////////////////////////////////////////////////////////////////////////

  template<typename T> using EnvKeyMap = sass::map::unordered::env<EnvKey, T>;
  template<typename T> using ModuleMap = sass::map::unordered::str<sass::string, T>;

  /////////////////////////////////////////////////////////////////////////
  // Having these sorted helps to keep lookups O(logn)
  // We must keep insertion order for these (check why?)
  /////////////////////////////////////////////////////////////////////////

  // Use stable map as that gives better lookup performance
  typedef sass::stblmap::env<EnvKey, uint32_t> VidxEnvKeyMap;
  typedef sass::stblmap::env<EnvKey, uint32_t> MidxEnvKeyMap;
  typedef sass::stblmap::env<EnvKey, uint32_t> FidxEnvKeyMap;

  /////////////////////////////////////////////////////////////////////////
  // these may (or may not) profit from a flat map implementation (also have fair lookups)
  // note that they must keep their insertion order, so only flat map or stable map
  // we only expect a few named args per function call, so no gain in keeping it sorted
  /////////////////////////////////////////////////////////////////////////

  // Use flat map as we expect very tiny input and not many lookups
  class ValueFlatMap : public sass::flatmap::env<EnvKey, ValueObj>, public RefCounted {};
  class ExpressionFlatMap : public sass::flatmap::env<EnvKey, ExpressionObj>, public RefCounted {};

  /////////////////////////////////////////////////////////////////////////
  // Allow fn args to be passed around by shared pointers
  /////////////////////////////////////////////////////////////////////////

  IMPL_MEM_OBJ(ValueFlatMap);
  IMPL_MEM_OBJ(ExpressionFlatMap);

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

};

#endif
