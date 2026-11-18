/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Not much implementation here, as most of it is header only
// Implements some rough debug/leak detection system (devs only)
/*****************************************************************************/

#include "shared_ptr.hpp"

#ifdef DEBUG_SHARED_PTR
#include "debugger.hpp"
#include <iostream>
#include <typeinfo>
#include "source.hpp"
#endif

namespace Sass {

  // See declaration in header for rationale.
  void RefCounted::release(RefCounted* obj) noexcept
  {
    // The queue lives on the stack of the outermost call; the thread-local
    // only points to it. This keeps the thread-local trivially destructible,
    // so objects released during static destruction remain safe.
    // Uses `std::vector` to stay independent of the custom allocator.
    thread_local std::vector<RefCounted*>* pending = nullptr;
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
    std::vector<RefCounted*> queue;
    pending = &queue;
    delete obj;
    while (!queue.empty()) {
      RefCounted* next = queue.back();
      queue.pop_back();
      delete next;
    }
    pending = nullptr;
  }

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
