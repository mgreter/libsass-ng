/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "eval.hpp"

#include "cssize.hpp"
#include "sources.hpp"
#include "compiler.hpp"
#include "stylesheet.hpp"
#include "exceptions.hpp"
#include "ast_values.hpp"
#include "ast_imports.hpp"
#include "ast_selectors.hpp"
#include "ast_callables.hpp"
#include "ast_statements.hpp"
#include "ast_expressions.hpp"
#include "parser_selector.hpp"
#include "parser_media_query.hpp"
#include "parser_keyframe_selector.hpp"

#include "preloader.hpp"

#include "character.hpp"
#include "calculation.hpp"
#include <limits>

namespace Sass {

  void Eval::visitCssAtRule(CssAtRule* node)
  {

    if (node->empty()) {
      current->addChildAt(node, false);
      return;
    }

    sass::string normalized(StringUtils::unvendor(node->name()->text()));
    bool isKeyframe = normalized == "keyframes";
    RAII_FLAG(inUnknownAtRule, !isKeyframe);
    RAII_FLAG(inKeyframes, isKeyframe);

    auto chroot = current->bubbleThrough(true);

    // ModifiableCssKeyframeBlock
    //CssAtRuleObj css = SASS_MEMORY_NEW(CssAtRule,
    //  node->pstate(), pu, name, value, node->isChildless());

    CssAtRuleObj copy = SASS_MEMORY_RESECT(node);

    // Adds new empty atRule to Root!
    chroot->addChildAt(copy, false);

    RAII_OBJ(CssParentNode, current, copy);


    if (!(!atRootExcludingStyleRule && readStyleRule != nullptr) || inKeyframes || node->name()->text() == "font-face") {

      for (const auto& child : node->elements()) {
        child->accept(this);
      }


    }
    else {

      // If we're in a style rule, copy it into the at-rule so that
      // declarations immediately inside it have somewhere to go.
      // For example, "a {@foo {b: c}}" should produce "@foo {a {b: c}}".
      CssStyleRule* qwe = SASS_MEMORY_RESECT(readStyleRule);
      node->addChildAt(qwe, false);
      acceptChildrenAt(qwe, node->elements());

    }

  }

  void Eval::visitCssComment(CssComment* css)
  {
    // Check if still in top headers
    if (isStillInTopHeaders()) {
      // Mark as valid top header
      _endOfImports += 1;
    }
    // Append to current css
    current->append(css);
  }

  void Eval::visitCssDeclaration(CssDeclaration* css)
  {
    current->append(css);
  }

  void Eval::visitCssImport(CssImport* css)
  {
    if (current != _stylesheet->compiled) {
      current->append(css);
    }
    else if (_endOfImports == _stylesheet->compiled->size()) {
      _stylesheet->compiled->append(css);
      _endOfImports += 1;
    }
    else {
      _outOfOrderImports.push_back(css);
    }
  }

  void Eval::visitCssKeyframeBlock(CssKeyframeBlock* css)
  {
    std::cerr << "Not Implemented CssKeyframeBlock\n";
  }

  void Eval::visitCssMediaRule(CssMediaRule* node)
  {

    CssMediaQueryVectorObj parsed(node->queries2());

    bool bubbleQuery = true;

    // Set of queries used for merge
    CssMediaQueryVector uses;

    CssMediaQueryVectorObj mergedQueries(mergeMediaQueries(
      mediaQueries, parsed, uses, bubbleQuery));

    if (mergedQueries == nullptr || mergedQueries->empty()) {
      if (mediaQueries && !mediaQueries->empty()) {
        // Skip rule when merged queries cancel each other
        // E.g. `not print` and `print` will never match
        return;
      }
      mergedQueries = parsed;
    }

    CssParentNode* chroot = current;
    while (BubbleMediaQuery(chroot, uses, bubbleQuery)) {
      if (!chroot->parent()) break;
      chroot = chroot->parent();
    }

    chroot->addChildAt(node, true);
    
  }

  void Eval::visitCssRoot(CssRoot* css)
  {
    for (const auto& statement : css->elements()) {
      statement->accept(this);
    }
  }

  void Eval::visitCssStyleRule(CssStyleRule* css)
  {

    if (!declarationName.empty()) {
      CallStackFrame frame(logger, css->pstate());
      throw Exception::RuntimeException(traces,
        "Style rules may not be used within nested declarations.");
    }
    else if (inKeyframes && current->isaCssKeyframeBlock()) {
      CallStackFrame frame(logger, css->pstate());
      throw Exception::RuntimeException(traces,
        "Style rules may not be used within keyframe blocks.");
    }

    // var nest = !(_styleRule?.fromPlainCss ?? false);
    bool nest = !plainCss;

    // bool nesting = current ? !current->fromPlainCss() : true;

    const auto& originalSelector = css->selector();
    if (nest) {
      // Temporary fix
      SelectorListObj slist = css->selector();
      for (auto& complex : slist->elements()) {
        for (auto& component : complex->elements()) {
          auto& compound = component->selector();
          if (compound->elements().empty()) continue;
          auto& head = compound->elements().front();
          if (head->isaCssParentSelector()) {
            compound->withExplicitParent(true);
            compound->erase(compound->begin());
            break;
          }
        }
      }

      // std::cerr << "EVAL FOR " << current->toString() << " => " << current->fromPlainCss() << "\n";

      // std::cerr << "RESOLVING [" << slist->inspect() << "]\n";
      /*if (!nest)*/ slist = slist->resolveParentSelectors(original(), traces, !atRootExcludingStyleRule);
      // std::cerr << "RESOLVED [" << slist->inspect() << "]\n";
      //if (slist->size() == 2) exit(1);
      // slist = slist->produce();
      // Append new selector list to the stack
      RAII_SELECTOR(selectorStack, slist/*->copy(false)*/);
      // The copy is needed for parent reference evaluation
      // dart-sass stores it as `originalSelector` member
      // RAII_SELECTOR(originalStack, slist->produce());
      RAII_SELECTOR(originalStack, SASS_MEMORY_COPY(slist));


      if (_extensionStore) _extensionStore->addSelector(slist, mediaStack.back());
      else std::cerr << "no extension store\n";
      // check if selector must be extendable by downstream extends

      // std::cerr << "ADD [" << slist->inspect() << "]\n";
      // Find the parent we should append to (bubble up)
      CssParentNode* chroot = current;
      /*if (!nest) */chroot = chroot->bubbleThroughCss();
      // Create a new style rule at the correct parent
      CssStyleRuleObj child = SASS_MEMORY_NEW(CssStyleRule,
        css->pstate(), chroot, slist);
      child->fromPlainCss(plainCss);
      // Add child to our parent
      chroot->addChildAt(child, true);
      // Register new child as style rule
      RAII_PTR(CssStyleRule, readStyleRule, child);
      // Reset specific flag (not in an at-rule)
      RAII_FLAG(atRootExcludingStyleRule, false);

      // Visit the remaining items at child
      ValueObj rv = acceptChildrenAt(child, css);

      if (!child->isInvisibleOtherThanBogusCombinators()) {
        for (const auto& complex : slist->elements()) {
          if (!complex->isBogusStrict()) continue;

          if (complex->isUseless()) {
            logger.addDeprecation("The selector \""
              + complex + "\" is invalid CSS.\n"
              "It will be omitted from the generated CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_USELESS);
          }
          else if (!complex->leadingCombinators().empty()) {
            logger.addDeprecation("The selector \""
              + complex + "\" is invalid CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_ERROR);
          }
          else if (complex->isBogusOtherThanLeadingCombinator()) {
            logger.addDeprecation("The selector \"" + complex + "\" "
              "is only valid for nesting\nIt shouldn't "
              "have children other than style rules.\n"
              "It will be omitted from the generated CSS.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_BOGUS);
          }
          else {
            logger.addDeprecation("The selector \"" + complex + "\" "
              "is only valid for nesting\nIt shouldn't "
              "have children other than style rules.\n"
              "This will be an error in LibSass 5.0.0.\n\n"
              "More info: https://sass-lang.com/d/bogus-combinators",
              complex->pstate(), Logger::WARN_SEL_BOGUS);
          }

        }
      }

    }

  }

  void Eval::visitCssSupportsRule(CssSupportsRule* css)
  {
    for (auto& child : css->elements()) {
      child->accept(this);
    }
  }

}
