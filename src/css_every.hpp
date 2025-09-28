/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_CSS_EVERY_HPP
#define SASS_CSS_EVERY_HPP

#include "visitor_css.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class EveryCssVisitor : public CssVisitor<bool> {

  public:

    bool visitCssAtRule(CssAtRule* css) override;
    bool visitCssComment(CssComment* css) override;
    bool visitCssDeclaration(CssDeclaration* css) override;
    bool visitCssImport(CssImport* css) override;
    bool visitCssKeyframeBlock(CssKeyframeBlock* css) override;
    bool visitCssMediaRule(CssMediaRule* css) override;
    bool visitCssRoot(CssRoot* css) override;
    bool visitCssStyleRule(CssStyleRule* css) override;
    bool visitCssSupportsRule(CssSupportsRule* css) override;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
