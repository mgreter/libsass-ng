/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "modules.hpp"

#include "stylesheet.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Module::determineTransitivelyContainsExtensions()
  {
    if (transitivelyContainsExtensions)
    {
      std::cerr << "ALREADY SET TRANSITIVELY\n";
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
    // std::cerr << "TO MODULE " << url << " => " << transitivelyContainsExtensions << "\n";
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
