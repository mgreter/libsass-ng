/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Our own pooled memory allocator (see docs for more info)
/*****************************************************************************/
#include "memory_allocator.hpp"

#ifdef SASS_CUSTOM_ALLOCATOR
#include "thread_local.hpp"
#include "memory_pool.hpp"
#include <mutex>
#include <cassert>
#include <vector>

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Pools are never destroyed in release builds, since memory may be freed
  // on another thread (its slices then go onto that thread's free list).
  // To not leak one pool per thread that ever compiled something, the pool
  // of a finished thread is put on this list and reused by new threads.
  // Memory is then bounded by the number of concurrent threads. Both are
  // allocated once and never destroyed (no static destruction order).
  static std::mutex& sparePoolsMutex()
  {
    static std::mutex* mutex = new std::mutex();
    return *mutex;
  }

  static std::vector<MemoryPool*>& sparePools()
  {
    static std::vector<MemoryPool*>* pools = new std::vector<MemoryPool*>();
    return *pools;
  }

  // Get a pool for the current thread (reuse a spare one if possible)
  static MemoryPool* acquirePool()
  {
    {
      std::lock_guard<std::mutex> lock(sparePoolsMutex());
      std::vector<MemoryPool*>& spare(sparePools());
      if (!spare.empty()) {
        MemoryPool* reused = spare.back();
        spare.pop_back();
        return reused;
      }
    }
    return new MemoryPool();
  }

  // Called when a thread exits (via the thread local destructor)
  static void releasePool(MemoryPool* released)
  {
    try {
      std::lock_guard<std::mutex> lock(sparePoolsMutex());
      sparePools().push_back(released);
    }
    catch (...) {
      // Leaking the pool is the safe choice (we're in a destructor)
    }
  }

  // Holds the pool of the current thread and hands it back on thread exit
  // (c++11 `thread_local` destroys it, our `ThreadLocal<T>` deletes it).
  struct PoolHolder {
    MemoryPool* pool = nullptr;
    ~PoolHolder() {
      // Not in debug builds: they count allocations per thread and delete
      // the pool once that count drops to zero, which must not happen for
      // a pool that still holds live memory of a previous thread.
      #ifndef DEBUG
      if (pool) releasePool(pool);
      // Memory freed later on this thread (e.g. static destructors
      // after the main thread's thread locals) must not go into the
      // released pool, so make the next access acquire a pool again.
      pool = nullptr;
      #endif
    }
  };

  #ifdef DEBUG
  static THREAD_LOCAL(size_t) allocations = 0;
  #endif

  // Access the pool slot of the current thread. The holder is function
  // local: memory is already allocated during the static initialization
  // of other translation units, before file scope objects of this one
  // may have been constructed (the `ThreadLocal` shim then called its
  // empty `std::function` and aborted). The native `thread_local` is
  // destroyed on thread exit; the shim is allocated once and never
  // destroyed (its data is still released on thread exit).
  static inline MemoryPool*& threadPool()
  {
    #if (CC_HAS_THREAD_LOCAL+0) && USE_STD_THREAD_LOCAL
    static thread_local PoolHolder holder;
    return holder.pool;
    #else
    static ThreadLocal<PoolHolder>* holder = new ThreadLocal<PoolHolder>();
    return static_cast<PoolHolder&>(*holder).pool;
    #endif
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Allocate memory from the memory pool.
  // Memory pool is allocated on first call.
  void* allocateMem(size_t size)
  {
    MemoryPool*& pool(threadPool());
    if (pool == nullptr) {
      pool = acquirePool();
    }
    #ifdef DEBUG
    // Account for allocation
    ++allocations;
    #endif
    // Invoke implementation
    return (*pool).allocate(size);
  }

  // Release the memory from the pool.
  // Destroys the pool when it is emptied.
  void deallocateMem(void* ptr)
  {
    // Memory may be freed on a different thread than it was allocated on
    // (e.g. compile on one thread, delete the compiler on another). Such
    // a thread may not have a pool yet, so create one. Freed slices go
    // onto this thread's free list; arenas are never released in release
    // builds, so slices from other pools stay valid for reuse.
    MemoryPool*& pool(threadPool());
    #ifndef DEBUG
    if (pool == nullptr) {
      pool = acquirePool();
    }
    #endif

    #ifdef DEBUG

    // It seems thread_local variable might be discharged!?
    // But the destructors of e.g. static strings is still
    // called, although their memory was discharged too.
    // Fine with me as long as address sanitizer is happy.
    // if (pool == nullptr) { return; }

    // Can't free if nothing is allocated
    // ToDo: check if condition is ever hit
    if (pool == nullptr || allocations == 0)
    {
      // Leaking is the safe choice here; deallocation must not throw
      // and writing to stderr may itself allocate (e.g. at shutdown)
      assert(false && "Dealloc out of sync");
      return;
    }

    #endif

    // Invoke implementation
    (*pool).deallocate(ptr);

    #ifdef DEBUG
    // Ensure to remove pool when finished
    // Very broad condition, which will be bad when
    // you keep allocating only one object and delete
    // it again (not the case with LibSass).
    // May not ever be hit with static memory
    if (--allocations == 0) {
      delete pool;
      pool = nullptr;
    }
    #endif

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
