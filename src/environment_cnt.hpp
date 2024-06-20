/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_ENVIRONMENT_CNT_H
#define SASS_ENVIRONMENT_CNT_H

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "hashing.hpp"
#include "flat_map.hpp"
#include "ast_fwd_decl.hpp"
#include "environment_key.hpp"
#include "comparators.hpp"
#include "containers.hpp"

namespace Sass {

  typedef sass::vector<EnvKey> EnvKeys;

  using EnvKeySet = sass::set::unordered::env<EnvKey>;

  // Performance comparison on MSVC and bolt-bench:
  // tsl::hopscotch_map is 10% slower than Sass::FlatMap
  // std::unordered_map a bit faster than tsl::hopscotch_map
  // Sass::FlapMap is 10% faster than any other container
  // Note: only due to our very specific usage patterns!
  class ValueFlatMap : public sass::flatmap::env<EnvKey, ValueObj, 0>, public RefCounted {};
  class ExpressionFlatMap : public sass::flatmap::env<EnvKey, ExpressionObj, 0>, public RefCounted {};

  // Keeping them sorted helps to keep lookups O(logn)
  // There seems no real down-side to not doing this
  typedef sass::flatmap::sorted::env<EnvKey, uint32_t, 0> VidxEnvKeyMap;
  typedef sass::flatmap::sorted::env<EnvKey, uint32_t, 0> MidxEnvKeyMap;
  typedef sass::flatmap::sorted::env<EnvKey, uint32_t, 0> FidxEnvKeyMap;

  // These get barely any use in tight-loops (no gains from optimizing)
  template<typename T> using EnvKeyMap = sass::map::unordered::env<EnvKey, T>;
  template<typename T> using ModuleMap = sass::map::unordered::str<sass::string, T>;

  IMPL_MEM_OBJ(ValueFlatMap);
  IMPL_MEM_OBJ(ExpressionFlatMap);

};

#endif
