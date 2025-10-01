/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Subclass of inspect, specializing in outputting valid css
/*****************************************************************************/
#ifndef SASS_CSS_CLONE_HPP
#define SASS_CSS_CLONE_HPP

#include "visitor_css.hpp"
#include "containers.hpp"
#include "ast_css.hpp"

namespace Sass {

  class CssClone : public CssVisitor<CssNode*> {

  public:

    // A map from selectors in the original stylesheet to selectors
    // generated for the new stylesheet using [ExtensionStore.clone].
    sass::map::unordered::ptr<SelectorListObj, BoxObj> oldToNewSelectors;

    virtual ~CssClone() {}

  public:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    CssClone(sass::map::unordered::ptr<SelectorListObj, BoxObj> oldToNewSelectors) :
        oldToNewSelectors(oldToNewSelectors)
    {}

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    CssAtRule* visitCssAtRule(CssAtRule* css) final;
    CssComment* visitCssComment(CssComment* css) final;
    CssDeclaration* visitCssDeclaration(CssDeclaration* css) final;
    CssImport* visitCssImport(CssImport* css) final;
    CssKeyframeBlock* visitCssKeyframeBlock(CssKeyframeBlock* css) final;
    CssMediaRule* visitCssMediaRule(CssMediaRule* css) final;
    CssRoot* visitCssRoot(CssRoot* css) final;
    CssStyleRule* visitCssStyleRule(CssStyleRule* css) final;
    CssSupportsRule* visitCssSupportsRule(CssSupportsRule* css) final;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
