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
    cfgOrigin(ptr->cfgOrigin),
    import(ptr->import)
  {

  }

  Stylesheet::Stylesheet(const SourceSpan& pstate, size_t reserve)
    : AstNode(pstate), Vectorized<Statement>(reserve), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

  Stylesheet::Stylesheet(const SourceSpan& pstate, StatementVector&& vec)
    : AstNode(pstate), Vectorized<Statement>(std::move(vec)), Module(pstate.getSource()->getAbsPath(), nullptr)
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
