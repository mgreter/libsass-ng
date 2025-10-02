/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Subclass of inspect, specializing in outputting valid css
// Mostly adding error cases for non css representable values
/*****************************************************************************/
#include "css_clone.hpp"

#include "charcode.hpp"
#include "character.hpp"
#include "ast_values.hpp"
#include "exceptions.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  CssAtRule* CssClone::visitCssAtRule(CssAtRule* css)
  {
    auto rv = SASS_MEMORY_COPY(css);
    for (auto& child : rv->elements())
      child = child->accept(this);
    return rv;
  }

  CssComment* CssClone::visitCssComment(CssComment* css)
  {
    return SASS_MEMORY_COPY(css);
  }

  CssDeclaration* CssClone::visitCssDeclaration(CssDeclaration* css)
  {
    return SASS_MEMORY_COPY(css);
  }

  CssImport* CssClone::visitCssImport(CssImport* css)
  {
    return SASS_MEMORY_COPY(css);
  }

  CssKeyframeBlock* CssClone::visitCssKeyframeBlock(CssKeyframeBlock* css)
  {
    auto rv = SASS_MEMORY_COPY(css);
    for(auto& child : rv->elements())
      child = child->accept(this);
    return rv;
  }

  CssMediaRule* CssClone::visitCssMediaRule(CssMediaRule* css)
  {
    auto rv = SASS_MEMORY_COPY(css);
    for (auto& child : rv->elements())
      child = child->accept(this);
    return rv;
  }

  CssRoot* CssClone::visitCssRoot(CssRoot* css)
  {
    auto rv = SASS_MEMORY_COPY(css);
    for (auto& child : rv->elements())
      child = child->accept(this);
    return rv;
  }

  CssStyleRule* CssClone::visitCssStyleRule(CssStyleRule* css)
  {
    // if (_oldToNewSelectors[node.selector] case var newSelector ? ) {
    auto sel = oldToNewSelectors.find(css->selector());
    if (sel != oldToNewSelectors.end())
    {
      auto rv = SASS_MEMORY_COPY(css);
      for (auto& child : rv->elements())
        child = child->accept(this);
      rv->boxsel(sel.value());
      return rv;
    }
    else {
      throw "Not same compilation";
    }

  }

  CssSupportsRule* CssClone::visitCssSupportsRule(CssSupportsRule* css)
  {
    auto rv = SASS_MEMORY_COPY(css);
    for (auto& child : rv->elements())
      child = child->accept(this);
    return rv;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

