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

  typedef sass::vector<EnvKey> EnvKeys;

  using EnvKeySet = sass::set::unordered::env<EnvKey>;

  // these may (or may not) profit from a flat map implementation (also have fair lookups)
  class ValueFlatMap : public sass::map::unordered::env<EnvKey, ValueObj>, public RefCounted {};
  class ExpressionFlatMap : public sass::map::unordered::env<EnvKey, ExpressionObj>, public RefCounted {};

  // Keeping them sorted helps to keep lookups O(logn)
  // There seems no real down-side to not doing this
  typedef sass::map::unordered::env<EnvKey, uint32_t> VidxEnvKeyMap;
  typedef sass::map::unordered::env<EnvKey, uint32_t> MidxEnvKeyMap;
  typedef sass::map::unordered::env<EnvKey, uint32_t> FidxEnvKeyMap;

  // These get barely any use in tight-loops (no gains from optimizing)
  template<typename T> using EnvKeyMap = sass::map::unordered::env<EnvKey, T>;
  template<typename T> using ModuleMap = sass::map::unordered::str<sass::string, T>;

  IMPL_MEM_OBJ(ValueFlatMap);
  IMPL_MEM_OBJ(ExpressionFlatMap);

};

#endif
