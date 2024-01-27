/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_CSS_IMPORTED_HPP
#define SASS_CSS_IMPORTED_HPP

#include "visitor_css.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ImportedCssVisitor : public CssVisitor<void> {

  private:

    // Eval visitor in whose context this was created.
    // Required to actually add children to the tree
    Eval& eval;

  public:

    ImportedCssVisitor(Eval& eval);

    virtual void visitCssAtRule(CssAtRule* css);
    virtual void visitCssComment(CssComment* css);
    virtual void visitCssDeclaration(CssDeclaration* css);
    virtual void visitCssImport(CssImport* css);
    virtual void visitCssKeyframeBlock(CssKeyframeBlock* css);
    virtual void visitCssMediaRule(CssMediaRule* css);
    virtual void visitCssRoot(CssRoot* css);
    virtual void visitCssStyleRule(CssStyleRule* css);
    virtual void visitCssSupportsRule(CssSupportsRule* css);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
