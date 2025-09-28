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

    virtual ~ImportedCssVisitor() {}

  public:

    ImportedCssVisitor(Eval& eval);

    void visitCssAtRule(CssAtRule* css) override;
    void visitCssComment(CssComment* css) override;
    void visitCssDeclaration(CssDeclaration* css) override;
    void visitCssImport(CssImport* css) override;
    void visitCssKeyframeBlock(CssKeyframeBlock* css) override;
    void visitCssMediaRule(CssMediaRule* css) override;
    void visitCssRoot(CssRoot* css) override;
    void visitCssStyleRule(CssStyleRule* css) override;
    void visitCssSupportsRule(CssSupportsRule* css) override;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
