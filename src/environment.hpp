/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_ENVIRONMENT_HPP
#define SASS_ENVIRONMENT_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_fwd_decl.hpp"
#include "environment_cnt.hpp"
#include "strings.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Constant similar to nullptr
  extern const EnvRef nullidx;

  /////////////////////////////////////////////////////////////////////////
  // Base class/struct for environment references. These can be variables,
  // functions or mixins. Each one belongs to an environment frame/scope,
  // as determined and seen during parsing. Similar to how C organizes
  // local variables via function stack pointers. This allows us to do
  // improvements during parse or eval phase, e.g. if a variable is first
  // seen, a new slot is assigned for it on the current frame/scope. When
  // we later see this variable being used, we know the static offset of
  // that variable on the frame. E.g. if a scope has two variables, one
  // will be at offset 0, the other at offset 1. When we later see one of
  // these variables being used, we know the offset to this variable to be
  // used to access it on the current active stack frame. This removes the
  // need for a dynamic lookup during runtime to improve performance.
  /////////////////////////////////////////////////////////////////////////
  class EnvRef
  {
  public:

    // The lexical frame pointer
    // Each parsed scope gets its own
    const EnvRefs* idxs = nullptr;

    // Local offset within the frame
    uint32_t offset = 0xFFFFFFFF;

    // Default constructor
    EnvRef() {}

    // Value constructor
    EnvRef(
      uint32_t offset) :
      offset(offset)
    {}

    // Value constructor
    EnvRef(
      const EnvRefs* idxs,
      uint32_t offset) :
      idxs(idxs),
      offset(offset)
    {}

    // Implement native equality operator
    inline bool operator==(const EnvRef& rhs) const {
      return idxs == rhs.idxs
        && offset == rhs.offset;
    }

    // Implement native inequality operator
    inline bool operator!=(const EnvRef& rhs) const {
      return idxs != rhs.idxs
        || offset != rhs.offset;
    }

    // Implement operator to allow use in sets
    inline bool operator<(const EnvRef& rhs) const {
      if (idxs < rhs.idxs) return true;
      return offset < rhs.offset;
    }

    // Check if reference is valid
    inline bool isValid() const { // 3%
      return offset != 0xFFFFFFFF;
    }

    // Check if entity is read-only
    inline bool isPrivate(uint32_t privateOffset) const {
      return idxs == nullptr &&
        offset <= privateOffset;
    }

    // Imports are transparent for variables, functions and mixins
    // We always need to create entities inside the parent scope
    bool isImport() const;

    // Flag if this scope is considered internal
    bool isInternal() const;

    // Rules like `@if`, `@for` etc. are semi-global (permeable).
    // Assignments directly in those can bleed to the root scope.
    bool isSemiGlobal() const;

    // Set to true once we are compiled via use or forward
    // An import does load the sheet, but does not compile it
    // Compiling it means hard-baking the config vars into it
    bool isCompiled() const;

    // Small helper for debugging
    sass::string toString() const;
  };

  /////////////////////////////////////////////////////////////////////////
  // 
  /////////////////////////////////////////////////////////////////////////

  // Runtime query structure
  // Created for every EnvFrame
  // Survives the actual EnvFrame
  class EnvRefs {
  public:

    // EnvRoot reference
    EnvRoot& root;

    // Parent is needed during runtime for
    // dynamic setter and getter by EnvKey.
    EnvRefs* pscope;

    const EnvRefs* nextScope() const;

    // Lexical scope entries
    VidxEnvKeyMap varIdxs;
    MidxEnvKeyMap mixIdxs;
    FidxEnvKeyMap fnIdxs;

    size_t varOffset = NPOS;
    size_t mixOffset = NPOS;
    size_t fnOffset = NPOS;

    // Any import may add forwarded entities to current scope
    // Since those scopes are dynamic and not global, we can't
    // simply insert our references. Therefore we must have the
    // possibility to hoist forwarded entities at any lexical scope.
    // All @use as "*" do not get exposed to the parent scope though.
    sass::vector<EnvRefs*> forwards;

    // Required
    sass::vector<ForwardRule*> delayedMerge;

    sass::string through;

    // Some scopes are connected to a module
    // Those expose some additional exports
    // Modules are global so we just link them
    Module* module = nullptr;

    // Imports are transparent for variables, functions and mixins
    // We always need to create entities inside the parent scope
    bool isImport = false;

    // Only set temporarily when evaluating through forwards
    // bool isForward = false;

    // Flag if this scope is considered internal
    bool isInternal = false;

    // Rules like `@if`, `@for` etc. are semi-global (permeable).
    // Assignments directly in those can bleed to the root scope.
    bool isSemiGlobal = false;

    // Set to true once we are compiled via use or forward
    // An import does load the sheet, but does not compile it
    // Compiling it means hard-baking the config vars into it
    bool isCompiled = false;

    // Value constructor
    EnvRefs(EnvRoot& root,
      EnvRefs* pscope,
      bool isImport,
      bool isInternal,
      bool isSemiGlobal) :
      root(root),
      pscope(pscope),
      isImport(isImport),
      isInternal(isInternal),
      isSemiGlobal(isSemiGlobal)
    {}

    /////////////////////////////////////////////////////////////////////////
    // Register an occurrence during parsing, reserving the offset.
    // Only structures are create when calling this, the real work
    // is done on runtime, where actual stack objects are queried.
    /////////////////////////////////////////////////////////////////////////

    // Register new variable on local stack
    // Invoked mostly by stylesheet parser
    EnvRef createVariable(const EnvKey& name);

    // Register new function on local stack
    // Mostly invoked by built-in functions
    // Then invoked for custom C-API function
    // Finally for every parsed function rule
    EnvRef createFunction(const EnvKey& name, bool special = false);

    // Register new mixin on local stack
    // Only invoked for mixin rules
    // But also for content blocks
    EnvRef createMixin(const EnvKey& name);

    void findVarIdxs(sass::vector<EnvRef>& vidxs, const EnvKey& name) const;

    // Get a mixin associated with the under [name].
    // Will lookup from the last runtime stack scope.
    // We will move up the runtime stack until we either
    // find a defined mixin or run out of parent scopes.

    // Get a function associated with the under [name].
    // Will lookup from the last runtime stack scope.
    // We will move up the runtime stack until we either
    // find a defined function or run out of parent scopes.


    // Get a value associated with the variable under [name].
    // If [global] flag is given, the lookup will be in the root.
    // Otherwise lookup will be from the last runtime stack scope.
    // We will move up the runtime stack until we either find a 
    // defined variable with a value or run out of parent scopes.


    // Get reference of entity with [name] under namespace [ns].
    // Namespaced entities can only be exported by actual modules.
    // Will process all parent scopes, skipping any imports as they are not
    // "real" modules, until a module is found that exports into given [ns].
    // Continues until a corresponding variable with [name] is found under [ns].
    EnvRef findVarIdx(const EnvKey& name, const sass::string& ns) const;
    EnvRef findFnIdx(const EnvKey& name, const sass::string& ns) const;
    EnvRef findMixIdx(const EnvKey& name, const sass::string& ns) const;

    // Get reference of entity with [name] without any namespace.
    // Non-namespaced entities are either directly declared in the root
    // stylesheet or via forwarded module entities into star "*" namespace.
    EnvRef findVarIdx(const EnvKey& name) const;
    EnvRef findFnIdx(const EnvKey& name) const;
    EnvRef findMixIdx(const EnvKey& name) const;

    bool hasNameSpace(const sass::string& ns) const;

    // Find function only in local frame


    EnvRef setModVar(const EnvKey& name, Value* value, bool guarded, const SourceSpan& pstate) const;


    EnvRef setModVar(const EnvKey& name, const sass::string& ns, Value* value, bool guarded, const SourceSpan& pstate);

  };

  /////////////////////////////////////////////////////////////////////////
  // EnvFrames are created during the parsing phase.
  /////////////////////////////////////////////////////////////////////////

  class EnvFrame {

  public:

    // Reference to stack to unwind from later
    sass::vector<EnvRefs*>& stack55;

    // Our runtime object
    EnvRefs* idxs;

  private:

  public:

    // Value constructor
    EnvFrame(
      Compiler& compiler,
      bool isSemiGlobal,
      bool isModule = false,
      bool isImport = false);

    // Destructor
    ~EnvFrame();

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class EnvRoot {

  public:

    // Main context
    Compiler& compiler;

    // Root runtime env
    EnvRefs* idxs;

  private:

    // We work together
    friend class Compiler;
    friend class EnvScope;
    friend class EnvFrame;
    friend class EnvRefs;

    // Growable runtime stack (get offset by xxxStackPtr).
    // These vectors are the main stacks during runtime.
    // When a scope with two variables is executed, two
    // new items are added to the variables stack. If the
    // same scope is called more than once, its variables
    // are added multiple times so we can revert to them.
    // variables[varStackPtr[vidx.frame] + vidx.offset]
    sass::vector<ValueObj> varStack;
    sass::vector<CallableObj> mixStack;
    sass::vector<CallableObj> fnStack;

  private:
    sass::vector<CallableObj> intFunction;
    sass::vector<CallableObj> intMixin;
    sass::vector<ValueObj> intVariables;

    // Last private accessible item
    uint32_t privateVarOffset = 0;
    uint32_t privateMixOffset = 0;
    uint32_t privateFnOffset = 0;

    // All created runtime variable objects.
    // Needed to track the memory allocations
    // And useful to resolve parents indirectly
    // Access it by absolute `frameOffset`
    sass::vector<EnvRefs*> scopes57;

  public:

    // Value constructor
    EnvRoot(Compiler& compiler);

    // Destructor
    ~EnvRoot();

    // Get value instance by stack index reference
    // Just converting and returning reference to array offset
    ValueObj& getVariable(const EnvRef& vidx);
    ValueObj& getModVar(const uint32_t offset);

    // Get function instance by stack index reference
    // Just converting and returning reference to array offset
    CallableObj& getFunction(const EnvRef& vidx);
    CallableObj& getModFn(const uint32_t offset);

    // Get mixin instance by stack index reference
    // Just converting and returning reference to array offset
    CallableObj& getMixin(const EnvRef& midx);
    CallableObj& getModMix(const uint32_t offset);

    // Set items on runtime/evaluation phase via references
    // Just converting reference to array offset and assigning
    void setVariable(const EnvRef& vidx, Value* value, bool guarded);


    void setModVar(const uint32_t offset, Value* value, bool guarded, const SourceSpan& pstate);

    // Set items on runtime/evaluation phase via references
    // Just converting reference to array offset and assigning
    void setFunction(const EnvRef& fidx, UserDefinedCallable* value, bool guarded);

    // Set items on runtime/evaluation phase via references
    // Just converting reference to array offset and assigning
    void setMixin(const EnvRef& midx, UserDefinedCallable* value, bool guarded);

    // Get a value associated with the variable under [name].
    // If [global] flag is given, the lookup will be in the root.
    // Otherwise lookup will be from the last runtime stack scope.
    // We will move up the runtime stack until we either find a 
    // defined variable with a value or run out of parent scopes.

    EnvRef findFnIdx(
      const EnvKey& name,
      const sass::string& ns = Strings::empty) const;

    EnvRef findMixIdx(
      const EnvKey& name,
      const sass::string& ns = Strings::empty) const;

    EnvRef findVarIdx(
      const EnvKey& name,
      const sass::string& ns = Strings::empty,
      bool global = false) const;


    void findVarIdxs(
      sass::vector<EnvRef>& vidxs,
      const EnvKey& name) const;

  };

  /////////////////////////////////////////////////////////////////////////
  // EnvScopes are created during evaluation phase. When we enter a parsed
  // scope, e.g. a function, mixin or style-rule, we create a new EnvScope
  // object on the stack and pass it the runtime environment and the current
  // stack frame (in form of a EnvRefs pointer). We will "allocate" the needed
  // space for scoped items and update any offset pointers. Once we go out of
  // scope the previous state is restored by unwinding the runtime stack.
  /////////////////////////////////////////////////////////////////////////
  // E.g. when a function has two lexical variables and is called twice,
  // we will push four actual variable/value instance onto the evaluation
  // stack. Once a function is finished, we unwind them from the stack.
  /////////////////////////////////////////////////////////////////////////

  class EnvScope {

  private:

    // Runtime environment
    EnvRoot& env;

    // Frame stack index references
    EnvRefs* idxs;

    // Remember previous "addresses"
    // Restored when we go out of scope
    size_t oldVarOffset;
    size_t oldMixOffset;
    size_t oldFnOffset;

  public:

    // Put frame onto stack
    EnvScope(
      EnvRoot& env,
      EnvRefs* idxs);
    // EO ctor

    // Restore old state on destruction
    ~EnvScope();
    // EO dtor

  };

  /////////////////////////////////////////////////////////////////////////
  // Base class for any scope. We want to keep the pointer
  // separate from the main object in this case here. They are
  // mostly managed by EnvRoot and stay alive with main context.
  /////////////////////////////////////////////////////////////////////////

  class Env
  {
  public:
    EnvRefs* idxs = nullptr;
    Env(EnvRefs* idxs)
      : idxs(idxs) {}
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
