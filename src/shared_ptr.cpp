/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Not much implementation here, as most of it is header only
// Implements some rough debug/leak detection system (devs only)
/*****************************************************************************/

#include "shared_ptr.hpp"

// See `releaseStateSlot` below (GCC on MinGW uses slow emutls)
#if defined(__MINGW32__) && defined(__GNUC__) && !defined(__clang__)
#define SASS_RELEASE_WIN32_TLS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef DEBUG_SHARED_PTR
#include "debugger.hpp"
#include <iostream>
#include <typeinfo>
#include "source.hpp"
#endif

namespace Sass {

  // Queue of objects whose deletion was deferred by `RefCounted::release`.
  // Uses `std::vector` to stay independent of the custom allocator.
  typedef std::vector<const RefCounted*> ReleaseQueue;

  #ifdef SASS_RELEASE_WIN32_TLS

  // GCC on MinGW implements `thread_local` through emutls, so every access
  // goes through `__emutls_get_address`, `pthread_getspecific` and a spin
  // lock in winpthreads. `release` runs for every deleted object, which made
  // this about 6% of the total compile time. Here we keep a pointer to the
  // per-thread state in a raw Win32 TLS slot instead, so the hot path is a
  // single `TlsGetValue`. The queue is kept between calls, which also saves
  // an allocation for every release that cascades.

  struct ReleaseState {
    ReleaseQueue queue;
    bool active = false;
  };

  // Marks a thread whose state was already destroyed (thread exit)
  static ReleaseState* const releaseStateGone =
    reinterpret_cast<ReleaseState*>(static_cast<uintptr_t>(1));

  // Allocated on first use (function-local static), so it
  // is valid even if objects are released during static init.
  static DWORD releaseStateSlot() noexcept
  {
    static const DWORD slot = TlsAlloc();
    return slot;
  }

  // Owns the state of one thread. Only touched once per thread, so the
  // emutls cost doesn't matter. It frees the state when the thread exits
  // and leaves a marker so late releases fall back to recursive delete.
  struct ReleaseStateOwner {
    ReleaseState state;
    ReleaseStateOwner() noexcept { TlsSetValue(releaseStateSlot(), &state); }
    ~ReleaseStateOwner() { TlsSetValue(releaseStateSlot(), releaseStateGone); }
  };

  // Returns nullptr if no state is available (recursive delete)
  static ReleaseState* getReleaseState() noexcept
  {
    const DWORD slot = releaseStateSlot();
    if (slot == TLS_OUT_OF_INDEXES) return nullptr;
    ReleaseState* state = static_cast<ReleaseState*>(TlsGetValue(slot));
    if (state == nullptr) {
      static thread_local ReleaseStateOwner owner;
      state = &owner.state;
    }
    return state == releaseStateGone ? nullptr : state;
  }

  // Create the state of the main thread during static initialization.
  // Otherwise it is created on the first release, which may only happen
  // during static destruction (e.g. `Colors::color_table`, if a program
  // exits without compiling anything). Creating a `thread_local` at that
  // point is broken with emutls and crashed on exit (e.g. `sassc -v`).
  // Only compiled with GCC (see SASS_RELEASE_WIN32_TLS)
  __attribute__((unused)) static const bool releaseStateMainThread =
    getReleaseState() != nullptr;

  // See declaration in header for rationale.
  void RefCounted::release(const RefCounted* obj) noexcept
  {
    ReleaseState* state = getReleaseState();
    if (state == nullptr) {
      delete obj;
      return;
    }
    if (state->active) {
      try {
        state->queue.push_back(obj);
        return;
      }
      catch (...) {
        // Out of memory: fall back to recursive delete
        delete obj;
        return;
      }
    }
    state->active = true;
    delete obj;
    ReleaseQueue& queue = state->queue;
    while (!queue.empty()) {
      const RefCounted* next = queue.back();
      queue.pop_back();
      delete next;
    }
    // Don't hold on to the memory of a huge cascade
    if (queue.capacity() > 4096) ReleaseQueue().swap(queue);
    state->active = false;
  }

  #else

  // The queue itself lives on the stack of the outermost release call; this
  // only points to it, so the thread-local is a trivially destructible POD.
  // This keeps it valid during static destruction and lets us use native
  // `thread_local` (the `THREAD_LOCAL` shim heap-allocates per thread).
  static thread_local ReleaseQueue* pendingReleases = nullptr;

  // See declaration in header for rationale.
  void RefCounted::release(const RefCounted* obj) noexcept
  {
    ReleaseQueue*& pending = pendingReleases;
    if (pending != nullptr) {
      try {
        pending->push_back(obj);
        return;
      }
      catch (...) {
        // Out of memory: fall back to recursive delete
        delete obj;
        return;
      }
    }
    ReleaseQueue queue;
    pending = &queue;
    delete obj;
    while (!queue.empty()) {
      const RefCounted* next = queue.back();
      queue.pop_back();
      delete next;
    }
    pending = nullptr;
  }

  #endif
  #ifdef DEBUG_SHARED_PTR
  void RefCounted::dumpMemLeaks() {
    if (!all.empty()) {
      std::cerr << "###################################\n";
      std::cerr << "# REPORTING MISSING DEALLOCATIONS #\n";
      std::cerr << "###################################\n";
      for (RefCounted* var : all) {
        if (AstNode* ast = dynamic_cast<AstNode*>(var)) {
          std::cerr << "LEAKED AST " << ast->getDbgFile() << ":" << ast->getDbgLine() << "\n";
          debug_ast(ast);
        }
        else if (SourceData* ast = dynamic_cast<SourceData*>(var)) {
          std::cerr << "LEAKED SOURCE " << ast->getDbgFile() << ":" << ast->getDbgLine() << "\n file: [" << ast->getAbsPath() << "]\n";
        }
        else {
          std::cerr << "LEAKED REF " << var->getDbgFile() << ":" << var->getDbgLine() << "\n";
        }
      }
      all.clear();
      deleted.clear();
      objCount = 0;
    }
  }
  size_t RefCounted::objCount = 0;
  sass::vector<RefCounted*> RefCounted::all;
  std::unordered_set<size_t> RefCounted::deleted;
  size_t RefCounted::maxRefCount = 0;
  #endif

  bool RefCounted::taint = false;
  // size_t RefCounted::moves = 0;
  // size_t RefCounted::copies = 0;
}
