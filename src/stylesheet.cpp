/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "stylesheet.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Stylesheet::Stylesheet(const Stylesheet* ptr) :
    AstNode(ptr),
    Vectorized<Statement>(ptr->elements_),
    Module(ptr),
    plainCss(ptr->plainCss),
    hasExtends(ptr->hasExtends),
    import(ptr->import)
  {

  }

  Stylesheet::Stylesheet(const SourceSpan& pstate, size_t reserve)
    : AstNode(pstate), Vectorized<Statement>(reserve), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

  Stylesheet::Stylesheet(const SourceSpan& pstate, StatementVector&& vec)
    : AstNode(pstate), Vectorized<Statement>(std::move(vec)), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

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

}
