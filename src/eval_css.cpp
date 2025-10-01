/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "eval.hpp"

#include "character.hpp"
#include "calculation.hpp"
#include "exceptions.hpp"
#include "extension.hpp"
#include <limits>

namespace Sass {

  void Eval::visitCssAtRule(CssAtRule* node)
  {

    if (node->empty()) {
      current->addChildAt(node, false);
      return;
    }

    sass::string normalized(StringUtils::unvendor(node->name()));
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


    if (!(!atRootExcludingStyleRule && readStyleRule != nullptr) || inKeyframes || node->name() == "font-face") {

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

    CssMediaQueryVectorObj parsed(node->queries());

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
      throw Exception::RuntimeException(logger,
        "Style rules may not be used within nested declarations.");
    }
    else if (inKeyframes && current->isaCssKeyframeBlock()) {
      CallStackFrame frame(logger, css->pstate());
      throw Exception::RuntimeException(logger,
        "Style rules may not be used within keyframe blocks.");
    }

    // if (css->selector()->toString() == "c") css->fromPlainCss(true);
    bool nest = !plainCss && !current->fromPlainCss22();
    // var nest = !(_styleRule?.fromPlainCss ?? false);
    // bool nesting = current ? !current->fromPlainCss() : true;

    // if (auto style = current->isaCssStyleRule()) {
    //   std::cerr << "DO nest " << !current->fromPlainCss22() << " into " << style->selector()->toString() << "\n";
    // }
    // else {
    //   std::cerr << "DO nest " << !current->fromPlainCss22() << " " << "\n";
    // }


    if (true) {
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

      if (nest) slist = slist->resolveParentSelectors(original(), logger, !atRootExcludingStyleRule);

      // Append new selector list to the stack
      RAII_SELECTOR(selectorStack, slist/*->copy(false)*/);
      // The copy is needed for parent reference evaluation
      // dart-sass stores it as `originalSelector` member
      RAII_SELECTOR(originalStack, SASS_MEMORY_COPY(slist));


      auto result = _extensionStore->addSelector(slist, mediaQueries);
      // else std::cerr << "no extension store\n";
      // check if selector must be extendable by downstream extends

      // Find the parent we should append to (bubble up)
      CssParentNode* chroot = current;
      if (nest) chroot = chroot->bubbleThroughCss();
      //else chroot = chroot->parent();
      // Create a new style rule at the correct parent
      // ModifiableBoxObj foo = nullptr;
      // if (css->boxsel()) {
      //   foo = css->boxsel()->_inner;
      // }
      // else {
      //   foo = new ModifiableBox(slist);
      // }
      CssStyleRuleObj child = SASS_MEMORY_NEW(CssStyleRule,
        css->pstate(), chroot, result);
      // CssStyleRuleObj child = nullptr;

      child->fromPlainCss22(css->fromPlainCss22());

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
            logger.addDeprecation(complex->pstate(),
              Logger::WARN_SEL_USELESS, [complex]() {
                return "The selector \""
                  + complex + "\" is invalid CSS.\n"
                  "It will be omitted from the generated CSS.\n"
                  "This will be an error in LibSass 5.0.0.\n\n"
                  "More info: https://sass-lang.com/d/bogus-combinators";
              });
          }
          else if (!complex->leadingCombinators().empty()) {
            logger.addDeprecation(complex->pstate(),
              Logger::WARN_SEL_ERROR, [complex]() {
                return "The selector \""
                  + complex + "\" is invalid CSS.\n"
                  "This will be an error in LibSass 5.0.0.\n\n"
                  "More info: https://sass-lang.com/d/bogus-combinators";
              });
          }
          else if (complex->isBogusOtherThanLeadingCombinator()) {
            logger.addDeprecation(complex->pstate(),
              Logger::WARN_SEL_BOGUS, [complex]() {
                return "The selector \"" + complex + "\" "
                  "is only valid for nesting\nIt shouldn't "
                  "have children other than style rules.\n"
                  "It will be omitted from the generated CSS.\n"
                  "This will be an error in LibSass 5.0.0.\n\n"
                  "More info: https://sass-lang.com/d/bogus-combinators";
              });
          }
          else {
            logger.addDeprecation(complex->pstate(),
              Logger::WARN_SEL_BOGUS, [complex]() {
                return "The selector \"" + complex + "\" "
                  "is only valid for nesting\nIt shouldn't "
                  "have children other than style rules.\n"
                  "This will be an error in LibSass 5.0.0.\n\n"
                  "More info: https://sass-lang.com/d/bogus-combinators";
              });
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
