/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_MEMORY_CONFIG_H
#define SASS_MEMORY_CONFIG_H

#include "settings.hpp"

/////////////////////////////////////////////////////////////////////////
// Memory allocator configurations
/////////////////////////////////////////////////////////////////////////

// Define memory alignment requirements
#define SASS_MEM_ALIGN alignof(void*)

// The number of bytes we use for our book-keeping before every
// memory fragment. Needed to know to which bucket we belongs on
// deallocations, or if it should go directly to the `free` call.
// Note: is actually `unsigned int`, but since we must pad the
// leading memory, so it boils down to max(SASS_MEM_ALIGN, ...)
// We assume that pointer size is always bigger than `unsigned int`
#define SassAllocatorBookSize alignof(void*)

// Bytes reserve for book-keeping on the arenas
// Currently unused and for later optimization
#define SassAllocatorArenaHeadSize 0

/////////////////////////////////////////////////////////////////////////
// Below settings should only be changed if you know what you do!
/////////////////////////////////////////////////////////////////////////

// Detail settings for pool allocator
#ifdef SASS_CUSTOM_ALLOCATOR

  // How many buckets should we have for the free-list
  // We have a bucket for every `SASS_MEM_ALIGN` * `SassAllocatorBuckets`
  // When something requests x amount of memory, we will pad the request
  // to be a multiple of `SASS_MEM_ALIGN` and then assign it either to
  // an existing bucket or directly use to malloc/free. Otherwise we will
  // chunk out a slice of the arena to store it in that memory.
  // Memory alignment is normally either 8 or 4 bytes (x64 vs x86)
  // So with 512 Buckets * 8, we serve all requests from 8b to 4kb
  #define SassAllocatorBuckets 512

  // The size of the memory pool arenas in bytes.
  // This determines the minimum allocated memory chunk.
  // Whenever we need more memory, we malloc that much.
  #define SassAllocatorArenaSize (1024 * 512)

#endif
// EO SASS_CUSTOM_ALLOCATOR

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
# ifdef DEBUG_SHARED_PTR
#  define DEBUG_MSVC_CRT_MEM
# endif
#endif

/////////////////////////////////////////////////////////////////////////
// Detection copied from thread_local.hpp (MPL-2.0 License)
/////////////////////////////////////////////////////////////////////////

#if defined(__MINGW32__) // mingw clang does not support non-trivial destructible types.
  // mingw gcc is still broken: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83562
// new gcc defines __has_feature but no cxx_thread_local
#elif (__clang__ + 0) // clang defines _MSC_VER as the cl builds it, or masquerades as gcc4.2, so check clang first
# if __has_feature(cxx_thread_local)
#   define CC_HAS_THREAD_LOCAL (!(_LIBCPP_VERSION + 0) || _LIBCPP_VERSION >= 4000)
# endif
// apple clang: no cxx_thread_local for iOS(and macOS if xcode<8),
//   no thread_local/__thread. opensource clang: targeting macOS 10.7+
// apple clang: always implemented in _tlv_atexit(apple/opensource) for
//   darwin, which is available in macOS10.10+/iOS8.0+(arm64,x86_64)/iOS9.0+(armv7)
// new clang/libc++7.0 supports osx10.4+ (compiler-rt) but not ios<8.0:
//   https://lists.llvm.org/pipermail/llvm-dev/2018-December/128364.html
// libstdc++(g++4.8+) and libc++4.0+(not apple). implemented in __cxa_thread_atexit in
// libc++abi, 4.0+ abi has a fallback if no __cxa_thread_atexit_impl (e.g. android<23)
#elif (_MSC_VER+0) >= 1900
# define CC_HAS_THREAD_LOCAL 1
#elif (__GNUC__*100+__GNUC_MINOR__) >= 408 // can't be clang
# if defined(__GNUC__) && __GNUC__ < 5
// Disable custom allocator for gcc before 5.0
#  define SASS_NO_PTHREAD
#  undef SASS_CUSTOM_ALLOCATOR
# endif
# define CC_HAS_THREAD_LOCAL 1
#endif

/////////////////////////////////////////////////////////////////////////
// Disable custom allocator if we are not thread local
/////////////////////////////////////////////////////////////////////////

#if (CC_HAS_THREAD_LOCAL+0) != 1
# undef SASS_CUSTOM_ALLOCATOR
#endif

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif
