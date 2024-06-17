/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Our own pooled memory allocator (see docs for more info)
/*****************************************************************************/
#include "memory_allocator.hpp"

#ifdef SASS_CUSTOM_ALLOCATOR
#include "thread_local.hpp"
#include "memory_pool.hpp"

namespace Sass {

#ifdef SASS_OPTIMIZE_SINGLE_THREADED

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Keep one instance
  static THREAD_LOCAL(MemoryPool*) pool;

  // Allocate memory from the memory pool.
  // Memory pool is allocated on first call.
  void* allocateMem(size_t size)
  {
    if (pool == nullptr) pool = new MemoryPool();
    // Invoke implementation
    return pool->allocate(size);
  }

  // Release the memory from the pool.
  // Destroys the pool when it is emptied.
  void deallocateMem(void* ptr)
  {
    // Invoke implementation
    /* if (pool) */ pool->deallocate(ptr);
  }

#else

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // We must only use PODs for thread_local.
  // Objects get very unpredictable init order.
  static THREAD_LOCAL(MemoryPool*) pool = nullptr;
  #ifdef DEBUG
  static THREAD_LOCAL(size_t) allocations = 0;
  #endif

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Allocate memory from the memory pool.
  // Memory pool is allocated on first call.
  void* allocateMem(size_t size)
  {
    if (pool == nullptr) {
      pool = new MemoryPool();
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
      std::cerr << "Dealloc out of sync\n";
      return; }

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

#endif


}

#endif
