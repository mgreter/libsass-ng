/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "modules.hpp"

#include "stylesheet.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Module::Module(const Module* ptr) :
    Env(ptr->idxs),
    url(ptr->url),
    isBuiltIn(ptr->isBuiltIn),
    isLoaded(ptr->isLoaded),
    isCompiled(ptr->isCompiled),
    compiled(ptr->compiled),
    transitivelyContainsExtensions(ptr->transitivelyContainsExtensions),
    mergedFwdVar(ptr->mergedFwdVar),
    mergedFwdMix(ptr->mergedFwdMix),
    mergedFwdFn(ptr->mergedFwdFn),
    upstream77(ptr->upstream77),
    imports56(ptr->imports56),
    moduse(ptr->moduse),
    modimps(ptr->modimps),
    extender52(ptr->extender52),
    precomments(ptr->precomments)
  {
  }

  void Module::addExtension(
    const SelectorListObj& extender3,
    const SimpleSelectorObj& target,
    const CssMediaRuleObj& mediaQueryContext,
    const ExtendRuleObj& extend,
    bool is_optional) const
  {
    extender52->addExtension(extender3, target, mediaQueryContext->queries(), extend, is_optional);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Module::determineTransitivelyContainsExtensions()
  {
    if (transitivelyContainsExtensions)
    {
     // std::cerr << "ALREADY SET TRANSITIVELY\n";
    }
    if (extender52 && !extender52->isEmpty())
    {
      transitivelyContainsExtensions = true;
    }
    for (const auto& mod : upstream77) {
      if (mod->transitivelyContainsExtensions)
      {
        transitivelyContainsExtensions = true;
        break;
      }
    }
  }

  Module::Module(const sass::string& url, EnvRefs* idxs) :
    Env(idxs),
    url(url),
    extender52()
  {}

  // Check if there are any unsatisfied extends (will throw)

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  BuiltInMod::BuiltInMod(const sass::string& url, EnvRoot& root) :
    Module(url, new EnvRefs(
      root,
      nullptr,
      false,  // isImport
      true,   // isInternal
      false)) // isSemiGlobal
  {
    isBuiltIn = true;
    isLoaded = true;
    isCompiled = true;
  }

  BuiltInMod::~BuiltInMod()
  {
    delete idxs;
  }

  void BuiltInMod::addFunction(const EnvKey& name, uint32_t offset)
  {
    idxs->fnIdxs[name] = offset;
  }

  void BuiltInMod::addVariable(const EnvKey& name, uint32_t offset)
  {
    idxs->varIdxs[name] = offset;
  }

  void BuiltInMod::addMixin(const EnvKey& name, uint32_t offset)
  {
    idxs->mixIdxs[name] = offset;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
