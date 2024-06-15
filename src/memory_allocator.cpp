/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Our own pooled memory allocator (see docs for more info)
/*****************************************************************************/
#include "memory_allocator.hpp"

#ifdef SASS_CUSTOM_ALLOCATOR
#include "memory_pool.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // We must only use PODs for thread_local.
  // Objects get very unpredictable init order.
  static thread_local MemoryPool* pool;
  static thread_local size_t allocations;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Allocate memory from the memory pool.
  // Memory pool is allocated on first call.
  void* allocateMem(size_t size)
  {
    if (pool == nullptr) {
      pool = new MemoryPool();
    }
    // Account for allocation
    ++allocations;
    // Invoke implementation
    return pool->allocate(size);
  }

  // Release the memory from the pool.
  // Destroys the pool when it is emptied.
  void deallocateMem(void* ptr, size_t size)
  {

    // It seems thread_local variable might be discharged!?
    // But the destructors of e.g. static strings is still
    // called, although their memory was discharged too.
    // Fine with me as long as address sanitizer is happy.
    if (pool == nullptr) { return; }

    // Can't free if nothing is allocated
    // ToDo: check if condition is ever hit
    if (allocations == 0) { return; }

    // Invoke implementation
    pool->deallocate(ptr);

    // Ensure to remove pool when finished
    // Very broad condition, which will be bad when
    // you keep allocating only one object and delete
    // it again (not the case with LibSass).
    // May not ever be hit with static memory
    if (--allocations == 0) {
      delete pool;
      pool = nullptr;
    }

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
