/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// We define all these boilerplate types in order to be able
// to shift implementations accross the board for profiling.
/*****************************************************************************/
// We have two main traits we can optimize against:
// ordered/unordered: insert vs lookup speed
// guaranteed small vs potential big: cache locality
/*****************************************************************************/
// Main users are extend, environment and caching
/*****************************************************************************/
#ifndef SASS_SETTINGS_CONTAINERS_HPP
#define SASS_SETTINGS_CONTAINERS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "comparators.hpp"

#include "memory_allocator.hpp"

/////////////////////////////////////////////////////////////////////////
// Always load standard containers
/////////////////////////////////////////////////////////////////////////

// Load standard containers
// Good insert, good lookup
// Fair balance between them
#include <set>
#include <map>

// Load unordered containers
// Better lookup, worse insert
// Rehash is performance enemy
// Use when doing many lookups
#include <unordered_map>
#include <unordered_set>

/////////////////////////////////////////////////////////////////////////
// Load headers for small map container
/////////////////////////////////////////////////////////////////////////

// Our own implementation
#include "flat_map.hpp"

// Or use SFL (Zlib license) alternative
#if defined(SASS_USE_SFL_ORDERED_FLAT_MAP)
#include "sfl/small_flat_map.hpp"
#include "sfl/small_flat_set.hpp"
#endif
#if defined(SASS_USE_SFL_ORDERED_FLAT_SET)
#include "sfl/small_flat_set.hpp"
#endif
#if defined(SASS_USE_SFL_UNORDERED_FLAT_MAP)
#include "sfl/small_unordered_flat_map.hpp"
#endif
#if defined(SASS_USE_SFL_UNORDERED_FLAT_SET)
#include "sfl/small_unordered_flat_set.hpp"
#endif

/////////////////////////////////////////////////////////////////////////
// Load Tessil (MIT License) container headers
/////////////////////////////////////////////////////////////////////////

#if defined(SASS_USE_TSL_HOPSCOTCH_SET)
#include "tessil/hopscotch_set.h"
#endif

#if defined(SASS_USE_TSL_HOPSCOTCH_MAP)
#include "tessil/hopscotch_map.h"
#endif

#if defined(SASS_USE_TSL_BHOPSCOTCH_SET)
#include "tessil/bhopscotch_set.h"
#endif

#if defined(SASS_USE_TSL_BHOPSCOTCH_MAP)
#include "tessil/bhopscotch_map.h"
#endif

#if defined(SASS_USE_TSL_ROBIN_SET)
#include "tessil/robin_set.h"
#endif

#if defined(SASS_USE_TSL_ROBIN_MAP)
#include "tessil/robin_map.h"
#endif

#if defined(SASS_USE_TSL_ROBIN_PG_SET)
#include "tessil/robin_set.h"
#endif

#if defined(SASS_USE_TSL_ROBIN_PG_MAP)
#include "tessil/robin_map.h"
#endif

#if defined(SASS_USE_TSL_SPARSE_SET)
#include "tessil/sparse_set.h"
#endif

#if defined(SASS_USE_TSL_SPARSE_MAP)
#include "tessil/sparse_map.h"
#endif

/////////////////////////////////////////////////////////////////////////
// Load abseil (Apache 2.0 License) btree map/set headers
/////////////////////////////////////////////////////////////////////////

#if defined(SASS_USE_ABSEIL_BTREE_SET)
#include "abseil/btree_set.h"
#endif

#if defined(SASS_USE_ABSEIL_BTREE_MAP)
#include "abseil/btree_map.h"
#endif

/////////////////////////////////////////////////////////////////////////
// Load Tessil (MIT License) headers for stable map/set
// Containers iterate over entries by insertion order
/////////////////////////////////////////////////////////////////////////

#include "tessil/ordered_map.h"
#include "tessil/ordered_set.h"

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Different hash map types used by extender
  /////////////////////////////////////////////////////////////////////////

  template <typename K, typename CL> using STD_SORTED_SET =
    std::set<K, CL, Sass::Allocator<K>>;

  template <typename K, typename V, typename CL> using STD_SORTED_MAP =
    std::map<K, V, CL, Sass::Allocator<std::pair<const K, V>>>;

  template <typename K, typename CH, typename CE> using STD_UNSORTED_SET =
    std::unordered_set<K, CH, CE, Sass::Allocator<K>>;

  template <typename K, typename V, typename CH, typename CE> using STD_UNSORTED_MAP =
    std::unordered_map<K, V, CH, CE, Sass::Allocator<std::pair<const K, V>>>;

  template <typename K> using STD_SORTED_OBJ_SET = STD_SORTED_SET<K, ObjLessThan>;
  template <typename K> using STD_SORTED_PTR_SET = STD_SORTED_SET<K, ObjPtrLessThan>;
  template <typename K> using STD_SORTED_ENV_SET = STD_SORTED_SET<K, EnvKeyLessThan>;
  template <typename K> using STD_SORTED_STR_SET = STD_SORTED_SET<K, StringLessThan>;
  template <typename K> using STD_SORTED_STL_SET = STD_SORTED_SET<K, std::less<K>>;

  template <typename K> using STD_UNSORTED_OBJ_SET = STD_UNSORTED_SET<K, ObjHash, ObjEquality>;
  template <typename K> using STD_UNSORTED_PTR_SET = STD_UNSORTED_SET<K, ObjPtrHash, ObjPtrEquality>;
  template <typename K> using STD_UNSORTED_ENV_SET = STD_UNSORTED_SET<K, EnvKeyHash, EnvKeyEquality>;
  template <typename K> using STD_UNSORTED_STR_SET = STD_UNSORTED_SET<K, StringHash, StringEquality>;
  template <typename K> using STD_UNSORTED_STL_SET = STD_UNSORTED_SET<K, std::hash<K>, std::equal_to<K>>;

  template <typename K, typename V> using STD_SORTED_OBJ_MAP = STD_SORTED_MAP<K, V, ObjLessThan>;
  template <typename K, typename V> using STD_SORTED_PTR_MAP = STD_SORTED_MAP<K, V, ObjPtrLessThan>;
  template <typename K, typename V> using STD_SORTED_ENV_MAP = STD_SORTED_MAP<K, V, EnvKeyLessThan>;
  template <typename K, typename V> using STD_SORTED_STR_MAP = STD_SORTED_MAP<K, V, StringLessThan>;
  template <typename K, typename V> using STD_SORTED_STL_MAP = STD_SORTED_MAP<K, V, std::less<K>>;

  template <typename K, typename V> using STD_UNSORTED_OBJ_MAP = STD_UNSORTED_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using STD_UNSORTED_PTR_MAP = STD_UNSORTED_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using STD_UNSORTED_ENV_MAP = STD_UNSORTED_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using STD_UNSORTED_STR_MAP = STD_UNSORTED_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using STD_UNSORTED_STL_MAP = STD_UNSORTED_MAP<K, V, std::hash<K>, std::equal_to<K>>;

  /////////////////////////////////////////////////////////////////////////
  // Btree is an ordered implementation
  /////////////////////////////////////////////////////////////////////////

  #if defined(SASS_USE_ABSEIL_BTREE_SET)
  template <typename K, typename CL> using BTREE_SET =
    btree::btree_set<K, CL, Sass::Allocator<K>>;
  template <typename K> using BTREE_OBJ_SET = BTREE_SET<K, ObjLessThan>;
  template <typename K> using BTREE_PTR_SET = BTREE_SET<K, ObjPtrLessThan>;
  template <typename K> using BTREE_ENV_SET = BTREE_SET<K, EnvKeyLessThan>;
  template <typename K> using BTREE_STR_SET = BTREE_SET<K, StringLessThan>;
  template <typename K> using BTREE_STL_SET = BTREE_SET<K, std::less<K>>;
  #endif

  #if defined(SASS_USE_ABSEIL_BTREE_MAP)
  template <typename K, typename V, typename CL> using BTREE_MAP =
    btree::btree_map<K, V, CL, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V> using BTREE_OBJ_MAP = BTREE_MAP<K, V, ObjLessThan>;
  template <typename K, typename V> using BTREE_PTR_MAP = BTREE_MAP<K, V, ObjPtrLessThan>;
  template <typename K, typename V> using BTREE_ENV_MAP = BTREE_MAP<K, V, EnvKeyLessThan>;
  template <typename K, typename V> using BTREE_STR_MAP = BTREE_MAP<K, V, StringLessThan>;
  template <typename K, typename V> using BTREE_STL_MAP = BTREE_MAP<K, V, std::less<K>>;
  #endif

  /////////////////////////////////////////////////////////////////////////
  // Tessil is an unordered implementation
  /////////////////////////////////////////////////////////////////////////

  #if defined(SASS_USE_TSL_SPARSE_SET)
  template <typename K, typename CH, typename CE> using TSL_SPARSE_SET =
    tsl::sparse_set<K, CH, CE, Sass::Allocator<K>>;
  template <typename K> using TSL_SPARSE_OBJ_SET = TSL_SPARSE_SET<K, ObjHash, ObjEquality>;
  template <typename K> using TSL_SPARSE_PTR_SET = TSL_SPARSE_SET<K, ObjPtrHash, ObjPtrEquality>;
  template <typename K> using TSL_SPARSE_ENV_SET = TSL_SPARSE_SET<K, EnvKeyHash, EnvKeyEquality>;
  template <typename K> using TSL_SPARSE_STR_SET = TSL_SPARSE_SET<K, StringHash, StringEquality>;
  template <typename K> using TSL_SPARSE_STL_SET = TSL_SPARSE_SET<K, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_ROBIN_SET)
  template <typename K, typename CH, typename CE> using ROBIN_SET =
    tsl::robin_set<K, CH, CE, Sass::Allocator<K>>;
  template <typename K> using ROBIN_OBJ_SET = ROBIN_SET<K, ObjHash, ObjEquality>;
  template <typename K> using ROBIN_PTR_SET = ROBIN_SET<K, ObjPtrHash, ObjPtrEquality>;
  template <typename K> using ROBIN_ENV_SET = ROBIN_SET<K, EnvKeyHash, EnvKeyEquality>;
  template <typename K> using ROBIN_STR_SET = ROBIN_SET<K, StringHash, StringEquality>;
  template <typename K> using ROBIN_STL_SET = ROBIN_SET<K, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_ROBIN_PG_SET)
  template <typename K, typename CH, typename CE> using ROBIN_PG_SET =
    tsl::robin_pg_set<K, CH, CE, Sass::Allocator<K>>;
  template <typename K> using ROBIN_PG_OBJ_SET = ROBIN_PG_SET<K, ObjHash, ObjEquality>;
  template <typename K> using ROBIN_PG_PTR_SET = ROBIN_PG_SET<K, ObjPtrHash, ObjPtrEquality>;
  template <typename K> using ROBIN_PG_ENV_SET = ROBIN_PG_SET<K, EnvKeyHash, EnvKeyEquality>;
  template <typename K> using ROBIN_PG_STR_SET = ROBIN_PG_SET<K, StringHash, StringEquality>;
  template <typename K> using ROBIN_PG_STL_SET = ROBIN_PG_SET<K, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_HOPSCOTCH_SET)
  template <typename K, typename CH, typename CE> using HOPS_SET =
    tsl::hopscotch_set<K, CH, CE, Sass::Allocator<K>>;
  template <typename K> using HOPS_OBJ_SET = HOPS_SET<K, ObjHash, ObjEquality>;
  template <typename K> using HOPS_PTR_SET = HOPS_SET<K, ObjPtrHash, ObjPtrEquality>;
  template <typename K> using HOPS_ENV_SET = HOPS_SET<K, EnvKeyHash, EnvKeyEquality>;
  template <typename K> using HOPS_STR_SET = HOPS_SET<K, StringHash, StringEquality>;
  template <typename K> using HOPS_STL_SET = HOPS_SET<K, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_BHOPSCOTCH_SET)
  template <typename K, typename CH, typename CE, typename CL> using BHOPS_SET =
    tsl::bhopscotch_set<K, CH, CE, CL, Sass::Allocator<K>>;
  template <typename K> using BHOPS_OBJ_SET = BHOPS_SET<K, ObjHash, ObjEquality, ObjLessThan>;
  template <typename K> using BHOPS_PTR_SET = BHOPS_SET<K, ObjPtrHash, ObjPtrEquality, ObjPtrLessThan>;
  template <typename K> using BHOPS_ENV_SET = BHOPS_SET<K, EnvKeyHash, EnvKeyEquality, EnvKeyLessThan>;
  template <typename K> using BHOPS_STR_SET = BHOPS_SET<K, StringHash, StringEquality, StringLessThan>;
  template <typename K> using BHOPS_STL_SET = BHOPS_SET<K, std::hash<K>, std::equal_to<K>, std::less<K>>;
  #endif

  #if defined(SASS_USE_TSL_SPARSE_MAP)
  template <typename K, typename V, typename CH, typename CE> using TSL_SPARSE_MAP =
    tsl::sparse_map<K, V, CH, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V> using TSL_SPARSE_OBJ_MAP = TSL_SPARSE_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using TSL_SPARSE_PTR_MAP = TSL_SPARSE_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using TSL_SPARSE_ENV_MAP = TSL_SPARSE_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using TSL_SPARSE_STR_MAP = TSL_SPARSE_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using TSL_SPARSE_STL_MAP = TSL_SPARSE_MAP<K, V, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_ROBIN_MAP)
  template <typename K, typename V, typename CH, typename CE> using ROBIN_MAP =
    tsl::robin_map<K, V, CH, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V> using ROBIN_OBJ_MAP = ROBIN_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using ROBIN_PTR_MAP = ROBIN_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using ROBIN_ENV_MAP = ROBIN_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using ROBIN_STR_MAP = ROBIN_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using ROBIN_STL_MAP = ROBIN_MAP<K, V, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_ROBIN_PG_MAP)
  template <typename K, typename V, typename CH, typename CE> using ROBIN_PG_MAP =
    tsl::robin_pg_map<K, V, CH, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V> using ROBIN_PG_OBJ_MAP = ROBIN_PG_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using ROBIN_PG_PTR_MAP = ROBIN_PG_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using ROBIN_PG_ENV_MAP = ROBIN_PG_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using ROBIN_PG_STR_MAP = ROBIN_PG_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using ROBIN_PG_STL_MAP = ROBIN_PG_MAP<K, V, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_HOPSCOTCH_MAP)
  template <typename K, typename V, typename CH, typename CE> using HOPS_MAP =
    tsl::hopscotch_map<K, V, CH, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V> using HOPS_OBJ_MAP = HOPS_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using HOPS_PTR_MAP = HOPS_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using HOPS_ENV_MAP = HOPS_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using HOPS_STR_MAP = HOPS_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using HOPS_STL_MAP = HOPS_MAP<K, V, std::hash<K>, std::equal_to<K>>;
  #endif
  #if defined(SASS_USE_TSL_BHOPSCOTCH_MAP)
  template <typename K, typename V, typename CH, typename CE, typename CL> using BHOPS_MAP =
    tsl::bhopscotch_map<K, V, CH, CE, CL, Sass::Allocator<std::pair<const K, V>>>;
  template <typename K, typename V> using BHOPS_OBJ_MAP = BHOPS_MAP<K, V, ObjHash, ObjEquality, ObjLessThan>;
  template <typename K, typename V> using BHOPS_PTR_MAP = BHOPS_MAP<K, V, ObjPtrHash, ObjPtrEquality, ObjPtrLessThan>;
  template <typename K, typename V> using BHOPS_ENV_MAP = BHOPS_MAP<K, V, EnvKeyHash, EnvKeyEquality, EnvKeyLessThan>;
  template <typename K, typename V> using BHOPS_STR_MAP = BHOPS_MAP<K, V, StringHash, StringEquality, StringLessThan>;
  template <typename K, typename V> using BHOPS_STL_MAP = BHOPS_MAP<K, V, std::hash<K>, std::equal_to<K>, std::less<K>>;
  #endif

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  template <typename K, typename V, size_t N, typename CE> using SASS_FLAT_MAP =
    FlatMap<K, V, N, CE, Sass::Allocator<std::pair<K, V>>>;

  #if defined(SASS_USE_SFL_ORDERED_FLAT_MAP)
  template <typename K, typename V, size_t N, typename CE> using SFL_ORDERED_FLAT_MAP =
    sfl::small_flat_map<K, V, N, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_OBJ_MAP = SFL_ORDERED_FLAT_MAP<K, V, N, ObjLessThan>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_PTR_MAP = SFL_ORDERED_FLAT_MAP<K, V, N, ObjPtrLessThan>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_ENV_MAP = SFL_ORDERED_FLAT_MAP<K, V, N, EnvKeyLessThan>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_STR_MAP = SFL_ORDERED_FLAT_MAP<K, V, N, StringLessThan>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_STL_MAP = SFL_ORDERED_FLAT_MAP<K, V, N, std::less<K>>;
  #else
  template <typename K, typename V, size_t N> using FLAT_SORTED_OBJ_MAP = SASS_FLAT_MAP<K, V, N, ObjEquality>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_PTR_MAP = SASS_FLAT_MAP<K, V, N, ObjPtrEquality>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_ENV_MAP = SASS_FLAT_MAP<K, V, N, EnvKeyEquality>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_STR_MAP = SASS_FLAT_MAP<K, V, N, StringEquality>;
  template <typename K, typename V, size_t N> using FLAT_SORTED_STL_MAP = SASS_FLAT_MAP<K, V, N, std::equal_to<K>>;
  #endif

  #if (defined SASS_USE_SFL_UNORDERED_FLAT_MAP)
  template <typename K, typename V, size_t N, typename CE> using SFL_UNORDERED_FLAT_MAP =
    sfl::small_unordered_flat_map<K, V, N, CE, Sass::Allocator<std::pair<K, V>>>;
  template <typename K, typename V, size_t N> using FLAT_OBJ_MAP = SFL_UNORDERED_FLAT_MAP<K, V, N, ObjEquality>;
  template <typename K, typename V, size_t N> using FLAT_PTR_MAP = SFL_UNORDERED_FLAT_MAP<K, V, N, ObjPtrEquality>;
  template <typename K, typename V, size_t N> using FLAT_ENV_MAP = SFL_UNORDERED_FLAT_MAP<K, V, N, EnvKeyEquality>;
  template <typename K, typename V, size_t N> using FLAT_STR_MAP = SFL_UNORDERED_FLAT_MAP<K, V, N, StringEquality>;
  template <typename K, typename V, size_t N> using FLAT_STL_MAP = SFL_UNORDERED_FLAT_MAP<K, V, N, std::equal_to<K>>;
  #else
  template <typename K, typename V, size_t N> using FLAT_OBJ_MAP = SASS_FLAT_MAP<K, V, N, ObjEquality>;
  template <typename K, typename V, size_t N> using FLAT_PTR_MAP = SASS_FLAT_MAP<K, V, N, ObjPtrEquality>;
  template <typename K, typename V, size_t N> using FLAT_ENV_MAP = SASS_FLAT_MAP<K, V, N, EnvKeyEquality>;
  template <typename K, typename V, size_t N> using FLAT_STR_MAP = SASS_FLAT_MAP<K, V, N, StringEquality>;
  template <typename K, typename V, size_t N> using FLAT_STL_MAP = SASS_FLAT_MAP<K, V, N, std::equal_to<K>>;
  #endif

  /////////////////////////////////////////////////////////////////////////
  // We only know one implementation for this container type
  /////////////////////////////////////////////////////////////////////////

  template <typename K, typename V, typename CH, typename CE> using STBL_MAP =
    tsl::ordered_map<K, V, CH, CE, Sass::Allocator<std::pair<K, V>>, sass::vector<std::pair<K, V>>>;

  template <typename K, typename V> using STBL_OBJ_MAP = STBL_MAP<K, V, ObjHash, ObjEquality>;
  template <typename K, typename V> using STBL_PTR_MAP = STBL_MAP<K, V, ObjPtrHash, ObjPtrEquality>;
  template <typename K, typename V> using STBL_ENV_MAP = STBL_MAP<K, V, EnvKeyHash, EnvKeyEquality>;
  template <typename K, typename V> using STBL_STR_MAP = STBL_MAP<K, V, StringHash, StringEquality>;
  template <typename K, typename V> using STBL_STL_MAP = STBL_MAP<K, V, std::hash<K>, std::equal_to<K>>;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

namespace sass {

  namespace map {
    #if defined(SASS_USE_ABSEIL_BTREE_MAP)
    template <typename K, typename V> using ptr = Sass::BTREE_PTR_MAP<K, V>;
    template <typename K, typename V> using obj = Sass::BTREE_OBJ_MAP<K, V>;
    template <typename K, typename V> using env = Sass::BTREE_ENV_MAP<K, V>;
    template <typename K, typename V> using str = Sass::BTREE_STR_MAP<K, V>;
    template <typename K, typename V> using stl = Sass::BTREE_STL_MAP<K, V>;
    #define sass_map_itval(val) val->second
    #define env_map_itval(val) val->second
    // #elif defined(SASS_USE_TSL_HOPSCOTCH_MAP_SRT)
    // template <typename K, typename V> using ptr = Sass::HOPS_PTR_MAP<K, V>;
    // template <typename K, typename V> using obj = Sass::HOPS_OBJ_MAP<K, V>;
    // template <typename K, typename V> using env = Sass::HOPS_ENV_MAP<K, V>;
    // template <typename K, typename V> using str = Sass::HOPS_STR_MAP<K, V>;
    // template <typename K, typename V> using stl = Sass::HOPS_STL_MAP<K, V>;
    // #define sass_map_itval(val) val.value()
    // #define env_map_itval(val) val.value()
    #elif defined(SASS_USE_TSL_BHOPSCOTCH_MAP)
    template <typename K, typename V> using ptr = Sass::BHOPS_PTR_MAP<K, V>;
    template <typename K, typename V> using obj = Sass::BHOPS_OBJ_MAP<K, V>;
    template <typename K, typename V> using env = Sass::BHOPS_ENV_MAP<K, V>;
    template <typename K, typename V> using str = Sass::BHOPS_STR_MAP<K, V>;
    template <typename K, typename V> using stl = Sass::BHOPS_STL_MAP<K, V>;
    #define sass_map_itval(val) val.value()
    #define env_map_itval(val) val.value()
    #else
    template <typename K, typename V> using ptr = Sass::STD_SORTED_PTR_MAP<K, V>;
    template <typename K, typename V> using obj = Sass::STD_SORTED_OBJ_MAP<K, V>;
    template <typename K, typename V> using env = Sass::STD_SORTED_ENV_MAP<K, V>;
    template <typename K, typename V> using str = Sass::STD_SORTED_STR_MAP<K, V>;
    template <typename K, typename V> using stl = Sass::STD_SORTED_STL_MAP<K, V>;
    #define sass_map_itval(val) val->second
    #define env_map_itval(val) val->second
    #endif

    namespace unordered {
      #if defined(SASS_USE_TSL_ROBIN_MAP)
      template <typename K, typename V> using ptr = Sass::ROBIN_PTR_MAP<K, V>;
      template <typename K, typename V> using obj = Sass::ROBIN_OBJ_MAP<K, V>;
      template <typename K, typename V> using env = Sass::ROBIN_ENV_MAP<K, V>;
      template <typename K, typename V> using str = Sass::ROBIN_STR_MAP<K, V>;
      template <typename K, typename V> using stl = Sass::ROBIN_STL_MAP<K, V>;
      #define sass_map_unordered_itval(val) val.value()
      #define env_map_unordered_itval(val) val.value()
      #elif defined(SASS_USE_TSL_ROBIN_PG_MAP)
      template <typename K, typename V> using ptr = Sass::ROBIN_PG_PTR_MAP<K, V>;
      template <typename K, typename V> using obj = Sass::ROBIN_PG_OBJ_MAP<K, V>;
      template <typename K, typename V> using env = Sass::ROBIN_PG_ENV_MAP<K, V>;
      template <typename K, typename V> using str = Sass::ROBIN_PG_STR_MAP<K, V>;
      template <typename K, typename V> using stl = Sass::ROBIN_PG_STL_MAP<K, V>;
      #define sass_map_unordered_itval(val) val.value()
      #define env_map_unordered_itval(val) val.value()
      #elif defined(SASS_USE_TSL_HOPSCOTCH_MAP)
      template <typename K, typename V> using ptr = Sass::HOPS_PTR_MAP<K, V>;
      template <typename K, typename V> using obj = Sass::HOPS_OBJ_MAP<K, V>;
      template <typename K, typename V> using env = Sass::HOPS_ENV_MAP<K, V>;
      template <typename K, typename V> using str = Sass::HOPS_STR_MAP<K, V>;
      template <typename K, typename V> using stl = Sass::HOPS_STL_MAP<K, V>;
      #define sass_map_unordered_itval(val) val.value()
      #define env_map_unordered_itval(val) val.value()
      #elif defined(SASS_USE_TSL_BHOPSCOTCH_MAP)
      template <typename K, typename V> using ptr = Sass::BHOPS_PTR_MAP<K, V>;
      template <typename K, typename V> using obj = Sass::BHOPS_OBJ_MAP<K, V>;
      template <typename K, typename V> using env = Sass::BHOPS_ENV_MAP<K, V>;
      template <typename K, typename V> using str = Sass::BHOPS_STR_MAP<K, V>;
      template <typename K, typename V> using stl = Sass::BHOPS_STL_MAP<K, V>;
      #define sass_map_unordered_itval(val) val.value()
      #define env_map_unordered_itval(val) val.value()
      #else
      template <typename K, typename V> using ptr = Sass::STD_UNSORTED_PTR_MAP<K, V>;
      template <typename K, typename V> using obj = Sass::STD_UNSORTED_OBJ_MAP<K, V>;
      template <typename K, typename V> using env = Sass::STD_UNSORTED_ENV_MAP<K, V>;
      template <typename K, typename V> using str = Sass::STD_UNSORTED_STR_MAP<K, V>;
      template <typename K, typename V> using stl = Sass::STD_UNSORTED_STL_MAP<K, V>;
      #define sass_map_unordered_itval(val) val->second
      #define env_map_unordered_itval(val) val->second
      #endif
    }

  }

  namespace set {
    #if defined(SASS_USE_ABSEIL_BTREE_SET)
    template <typename K> using ptr = Sass::BTREE_PTR_SET<K>;
    template <typename K> using obj = Sass::BTREE_OBJ_SET<K>;
    template <typename K> using env = Sass::BTREE_ENV_SET<K>;
    template <typename K> using str = Sass::BTREE_STR_SET<K>;
    template <typename K> using stl = Sass::BTREE_STL_SET<K>;
    // #elif defined(SASS_USE_TSL_SPARSE_SET)
    // template <typename K> using ptr = Sass::TSL_SPARSE_PTR_SET<K>;
    // template <typename K> using obj = Sass::TSL_SPARSE_OBJ_SET<K>;
    // template <typename K> using env = Sass::TSL_SPARSE_ENV_SET<K>;
    // template <typename K> using str = Sass::TSL_SPARSE_STR_SET<K>;
    // template <typename K> using stl = Sass::TSL_SPARSE_STL_SET<K>;
    // #elif defined(SASS_USE_TSL_ROBIN_SET)
    // template <typename K> using ptr = Sass::ROBIN_PTR_SET<K>;
    // template <typename K> using obj = Sass::ROBIN_OBJ_SET<K>;
    // template <typename K> using env = Sass::ROBIN_ENV_SET<K>;
    // template <typename K> using str = Sass::ROBIN_STR_SET<K>;
    // template <typename K> using stl = Sass::ROBIN_STL_SET<K>;
    // #elif defined(SASS_USE_TSL_ROBIN_PG_SET)
    // template <typename K> using ptr = Sass::ROBIN_PG_PTR_SET<K>;
    // template <typename K> using obj = Sass::ROBIN_PG_OBJ_SET<K>;
    // template <typename K> using env = Sass::ROBIN_PG_ENV_SET<K>;
    // template <typename K> using str = Sass::ROBIN_PG_STR_SET<K>;
    // template <typename K> using stl = Sass::ROBIN_PG_STL_SET<K>;
    // #elif defined(SASS_USE_TSL_HOPSCOTCH_SET_SRT)
    // template <typename K> using ptr = Sass::HOPS_PTR_SET<K>;
    // template <typename K> using obj = Sass::HOPS_OBJ_SET<K>;
    // template <typename K> using env = Sass::HOPS_ENV_SET<K>;
    // template <typename K> using str = Sass::HOPS_STR_SET<K>;
    // template <typename K> using stl = Sass::HOPS_STL_SET<K>;
    #elif defined(SASS_USE_TSL_BHOPSCOTCH_SET)
    template <typename K> using ptr = Sass::BHOPS_PTR_SET<K>;
    template <typename K> using obj = Sass::BHOPS_OBJ_SET<K>;
    template <typename K> using env = Sass::BHOPS_ENV_SET<K>;
    template <typename K> using str = Sass::BHOPS_STR_SET<K>;
    template <typename K> using stl = Sass::BHOPS_STL_SET<K>;
    #else
    template <typename K> using ptr = Sass::STD_SORTED_PTR_SET<K>;
    template <typename K> using obj = Sass::STD_SORTED_OBJ_SET<K>;
    template <typename K> using env = Sass::STD_SORTED_ENV_SET<K>;
    template <typename K> using str = Sass::STD_SORTED_STR_SET<K>;
    template <typename K> using stl = Sass::STD_SORTED_STL_SET<K>;
    #endif

    namespace unordered {

      #if defined(SASS_USE_TSL_SPARSE_SET)
      template <typename K> using ptr = Sass::TSL_SPARSE_PTR_SET<K>;
      template <typename K> using obj = Sass::TSL_SPARSE_OBJ_SET<K>;
      template <typename K> using env = Sass::TSL_SPARSE_ENV_SET<K>;
      template <typename K> using str = Sass::TSL_SPARSE_STR_SET<K>;
      template <typename K> using stl = Sass::TSL_SPARSE_STL_SET<K>;
      #elif defined(SASS_USE_TSL_ROBIN_SET)
      template <typename K> using ptr = Sass::ROBIN_PTR_SET<K>;
      template <typename K> using obj = Sass::ROBIN_OBJ_SET<K>;
      template <typename K> using env = Sass::ROBIN_ENV_SET<K>;
      template <typename K> using str = Sass::ROBIN_STR_SET<K>;
      template <typename K> using stl = Sass::ROBIN_STL_SET<K>;
      #elif defined(SASS_USE_TSL_ROBIN_PG_SET)
      template <typename K> using ptr = Sass::ROBIN_PG_PTR_SET<K>;
      template <typename K> using obj = Sass::ROBIN_PG_OBJ_SET<K>;
      template <typename K> using env = Sass::ROBIN_PG_ENV_SET<K>;
      template <typename K> using str = Sass::ROBIN_PG_STR_SET<K>;
      template <typename K> using stl = Sass::ROBIN_PG_STL_SET<K>;
      #elif defined(SASS_USE_TSL_BHOPSCOTCH_SET)
      template <typename K> using ptr = Sass::BHOPS_PTR_SET<K>;
      template <typename K> using obj = Sass::BHOPS_OBJ_SET<K>;
      template <typename K> using env = Sass::BHOPS_ENV_SET<K>;
      template <typename K> using str = Sass::BHOPS_STR_SET<K>;
      template <typename K> using stl = Sass::BHOPS_STL_SET<K>;
      #elif defined(SASS_USE_TSL_HOPSCOTCH_SET)
      template <typename K> using ptr = Sass::HOPS_PTR_SET<K>;
      template <typename K> using obj = Sass::HOPS_OBJ_SET<K>;
      template <typename K> using env = Sass::HOPS_ENV_SET<K>;
      template <typename K> using str = Sass::HOPS_STR_SET<K>;
      template <typename K> using stl = Sass::HOPS_STL_SET<K>;
      #else
      template <typename K> using ptr = Sass::STD_UNSORTED_PTR_SET<K>;
      template <typename K> using obj = Sass::STD_UNSORTED_OBJ_SET<K>;
      template <typename K> using env = Sass::STD_UNSORTED_ENV_SET<K>;
      template <typename K> using str = Sass::STD_UNSORTED_STR_SET<K>;
      template <typename K> using stl = Sass::STD_UNSORTED_STL_SET<K>;
      #endif

    }
  }

  /////////////////////////////////////////////////////////////////////////
  // Stable insertion order map (for iterating over it)
  /////////////////////////////////////////////////////////////////////////

  namespace stblmap {
    // This will always use tessil implementation for insertion order
    template <typename K, typename V> using ptr = Sass::STBL_PTR_MAP<K, V>;
    template <typename K, typename V> using obj = Sass::STBL_OBJ_MAP<K, V>;
    template <typename K, typename V> using env = Sass::STBL_ENV_MAP<K, V>;
    template <typename K, typename V> using str = Sass::STBL_STR_MAP<K, V>;
    template <typename K, typename V> using stl = Sass::STBL_STL_MAP<K, V>;
  }

  /////////////////////////////////////////////////////////////////////////
  // Flat maps are expected to be tiny (only a few items)
  // Big stylesheets may add up to a few 100 items to env
  /////////////////////////////////////////////////////////////////////////

  namespace flatmap {
    // This may either use our own flat map or one from sfl
    template <typename K, typename V, size_t N = 0> using ptr = Sass::FLAT_PTR_MAP<K, V, N>;
    template <typename K, typename V, size_t N = 0> using obj = Sass::FLAT_OBJ_MAP<K, V, N>;
    template <typename K, typename V, size_t N = 0> using env = Sass::FLAT_ENV_MAP<K, V, N>;
    template <typename K, typename V, size_t N = 0> using str = Sass::FLAT_STR_MAP<K, V, N>;
    template <typename K, typename V, size_t N = 0> using stl = Sass::FLAT_STL_MAP<K, V, N>;

    namespace sorted {
      // This may either use our own flat map or one from sfl
      template <typename K, typename V, size_t N = 0> using ptr = Sass::FLAT_SORTED_PTR_MAP<K, V, N>;
      template <typename K, typename V, size_t N = 0> using obj = Sass::FLAT_SORTED_OBJ_MAP<K, V, N>;
      template <typename K, typename V, size_t N = 0> using env = Sass::FLAT_SORTED_ENV_MAP<K, V, N>;
      template <typename K, typename V, size_t N = 0> using str = Sass::FLAT_SORTED_STR_MAP<K, V, N>;
      template <typename K, typename V, size_t N = 0> using stl = Sass::FLAT_SORTED_STL_MAP<K, V, N>;
    }
  }

  /////////////////////////////////////////////////////////////////////////
  // Special namespace for (file-name) caching maps
  /////////////////////////////////////////////////////////////////////////

  namespace cachemap {
    // Prefer unordered map, since we do way more lookups than inserts
    template <typename K, typename V> using env = sass::map::unordered::env<K, V>;
    template <typename K, typename V> using str = sass::map::unordered::str<K, V>;
    template <typename K, typename V> using stl = sass::map::unordered::stl<K, V>;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  typedef sass::set::str<sass::string> stringset;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
