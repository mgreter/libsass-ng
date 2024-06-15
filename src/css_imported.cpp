#include "css_imported.hpp"

#include "ast_css.hpp"
#include "eval.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  ImportedCssVisitor::ImportedCssVisitor(Eval& eval)
    : eval(eval)
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static bool SkipNone(CssNode* node) {
    return false;
  }

  static bool SkipStyleRules(CssNode* node) {
    return node->isaCssStyleRule();
  }

  static bool SkipStyleAndMediaRules(CssNode* node) {
    return SkipStyleRules(node) || node->isaCssMediaRule();
  }

  void Sass::ImportedCssVisitor::visitCssAtRule(CssAtRule* css)
  {
    if (css->empty()) {
      eval.current->addChildAt(css, false);
    }
    else {
      eval._addChild(css, SkipStyleRules);
    }
  }

  void ImportedCssVisitor::visitCssComment(CssComment* css)
  {
    eval._addChild(css, SkipNone);
  }

  void ImportedCssVisitor::visitCssDeclaration(CssDeclaration* css)
  {
    std::cerr << "THIS SHOULD NOT HAPPEN, NO CSS KEY FRAME BLOCKS!!!!\n";
  }

  void ImportedCssVisitor::visitCssImport(CssImport* css)
  {
    if (eval.current != eval._stylesheet->compiled) {
      eval._addChild(css, SkipNone);
    }
    else if (eval._endOfImports == eval._stylesheet->compiled->size()) {
      eval._addChild(css, SkipNone);
      eval._endOfImports += 1;
    }
    else {
      eval._outOfOrderImports.push_back(css);
    }
  }

  void ImportedCssVisitor::visitCssKeyframeBlock(CssKeyframeBlock* css)
  {
    std::cerr << "THIS SHOULD NOT HAPPEN, NO CSS KEY FRAME BLOCKS!!!!\n";
  }

  void ImportedCssVisitor::visitCssMediaRule(CssMediaRule* css)
  {
    if (!eval.mediaQueries || eval.mediaQueries->empty()) {
      eval._addChild(css, SkipStyleAndMediaRules);
    }
    else {
      bool bubbleQuery = true;
      CssMediaQueryVector result; // not required, remove
      CssMediaQueryVectorObj merged = eval.mergeMediaQueries(eval.mediaQueries, css->queries(), result, bubbleQuery);
      if (merged == nullptr || merged->empty()) { eval._addChild(css, SkipStyleRules); }
      else { eval._addChild(css, SkipStyleAndMediaRules); }
    }
  }

  void ImportedCssVisitor::visitCssRoot(CssRoot* css)
  {
    for (auto& child : css->elements()) {
      child->accept(this);
    }
  }

  void ImportedCssVisitor::visitCssStyleRule(CssStyleRule* css)
  {
    eval._addChild(css, SkipStyleRules);
  }

  void ImportedCssVisitor::visitCssSupportsRule(CssSupportsRule* css)
  {
    eval._addChild(css, SkipStyleRules);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

