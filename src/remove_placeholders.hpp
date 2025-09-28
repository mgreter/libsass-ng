/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_REMOVE_PLACEHOLDERS_HPP
#define SASS_REMOVE_PLACEHOLDERS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_css.hpp"
#include "ast_fwd_decl.hpp"
#include "visitor_css.hpp"

namespace Sass {

  class RemovePlaceholders :
    public CssVisitor<void> {

  public:

    virtual ~RemovePlaceholders() {}

  private:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Helper to traverse the tree recursively
    void acceptCssParentNode(CssParentNode*);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // The main functions to do the cleanup
    void remove_placeholders(SelectorList*);
    void remove_placeholders(SimpleSelector*);
    void remove_placeholders(CompoundSelector*);
    void remove_placeholders(ComplexSelector*);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  public:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Do not implement anything for these visitors
    void visitCssComment(CssComment* css) final {};
    void visitCssDeclaration(CssDeclaration* css) final {};
    void visitCssImport(CssImport* css) final {};

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Move further down into children to remove recursively
    void visitCssAtRule(CssAtRule* css) final { acceptCssParentNode(css); };
    void visitCssKeyframeBlock(CssKeyframeBlock* css) final { acceptCssParentNode(css); };
    void visitCssMediaRule(CssMediaRule* css) final { acceptCssParentNode(css); };
    void visitCssSupportsRule(CssSupportsRule* css) final { acceptCssParentNode(css); };

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Cleaning only makes sense on those nodes
    void visitCssRoot(CssRoot*) final;
    void visitCssStyleRule(CssStyleRule*) final;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
