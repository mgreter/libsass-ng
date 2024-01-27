/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "stylesheet.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Stylesheet::Stylesheet(const SourceSpan& pstate, size_t reserve)
    : AstNode(pstate), Vectorized<Statement>(reserve), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

  Stylesheet::Stylesheet(const SourceSpan& pstate, StatementVector&& vec)
    : AstNode(pstate), Vectorized<Statement>(std::move(vec)), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

  void Module::addExtension(
    const SelectorListObj& extender3,
    const SimpleSelectorObj& target,
    const CssMediaRuleObj& mediaQueryContext,
    const ExtendRuleObj& extend,
    bool is_optional) const
  {
//UUU    for (Stylesheet* mod : upstream) {
//UUU      mod->addExtension(extend, target, mediaQueryContext, is_optional);
//UUU    }
    // std::cerr << "============ add extension\n";
    if (extender52) extender52->addExtension(extender3, target, mediaQueryContext->queries2(), extend, is_optional);
    else std::cerr << "!!!!! MODULE HAS NO EXTENSION STORE\n";
  }


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
