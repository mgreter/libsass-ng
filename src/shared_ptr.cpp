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

  // Queue of objects whose deletion was deferred by `RefCounted::release`.
  // The queue itself lives on the stack of the outermost release call; this
  // only points to it (we must only use PODs for thread-locals). Defined at
  // namespace scope so it outlives function-local statics holding objects.
  // Uses `std::vector` to stay independent of the custom allocator.
  static THREAD_LOCAL(std::vector<const RefCounted*>*) pendingReleases = nullptr;

  // See declaration in header for rationale.
  void RefCounted::release(const RefCounted* obj) noexcept
  {
    std::vector<const RefCounted*>*& pending = pendingReleases;
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
    std::vector<const RefCounted*> queue;
    pending = &queue;
    delete obj;
    while (!queue.empty()) {
      const RefCounted* next = queue.back();
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
