/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "eval.hpp"

#include "character.hpp"
#include "calculation.hpp"
#include "exceptions.hpp"
#include "extension.hpp"
#include "css_clone.hpp"
#include <limits>

#include "debugger.hpp"

namespace Sass {


  void Eval::_visitUpstreamModule(Stylesheet* modu, CssRoot* css, sass::vector<StylesheetObj>& sorted,
    std::set<sass::string>& seen, sass::vector<CssNodeObj>& imports, bool clone)
  {

    StylesheetObj module = modu; // SASS_MEMORY_NEW(Stylesheet, modu);

    // if (current->idxs->isImport) return;
    if (clone)
    {

      if (module->compiled) {
        // std::cerr << "visit module " << module->url << "\n";
        sass::map::unordered::ptr<SelectorListObj, BoxObj> oldToNewSelectors;
        ExtensionStoreObj newExtender = module->extender52->clone(oldToNewSelectors);
        CssClone cloner(oldToNewSelectors);
        StylesheetObj copy2 = SASS_MEMORY_NEW(Stylesheet, module.ptr());
        copy2->compiled = cloner.visitCssRoot(module->compiled);
        copy2->extender52 = newExtender;
        // // newExtensionStore->selectors54 = 
        // // copy2->selectors54
        module = copy2;
      }
    }


    for (Stylesheet* upstream : module->upstream77) {
      // if (upstream->idxs->isImport) continue;
      if (upstream == nullptr) continue;
      if (seen.count(upstream->import->getAbsPath())) continue;
      seen.insert(upstream->import->getAbsPath()); // protected

      for (CssComment* head : upstream->precomments) {
        if (!css->empty()) css->append(head);
        else imports.push_back(head);
      }

      _visitUpstreamModule(upstream, css, sorted, seen, imports, clone);
    }

    sorted.push_back(module); // cloned one
    if (module->compiled) {

      // newExtensionStore = extensionStore.clone();
      // var(newExtensionStore, oldToNewSelectors) = extensionStore.clone();

        // debug_ast(module->compiled);
        CssRootObj copy = module->compiled;
        // debug_ast(copy);
        // /*if (clone)*/ copy = SASS_MEMORY_CLONE(copy);
        auto& statements = copy->elements();
        auto index = _indexAfterImports(statements);
        sass::vector<CssNodeObj> rest;
        imports.insert(imports.end(), statements.begin(), statements.begin() + index);
        css->elements().insert(css->elements().end(), statements.begin() + index, statements.end());


    }

  }



  CssRoot* Eval::_combineCss(Stylesheet* root, bool clone)
  {
    CssRootObj css = SASS_MEMORY_NEW(CssRoot, root->pstate());
    sass::vector<CssNodeObj> imports;

    std::set<sass::string> seen;
    sass::vector<StylesheetObj> modules;
    // Probably more efficient to push and resort
    _visitUpstreamModule(root, css, modules, seen, imports, clone);
    std::reverse(modules.begin(), modules.end());

    // auto modules = _topologicalModules(root, mods, imports, clone);
    root->determineTransitivelyContainsExtensions();

    RAII_OBJ(CssParentNode, current, css);
    RAII_PTR(ExtensionStore, _extensionStore, root->extender52);
    
    RAII_PTR(Stylesheet, _stylesheet, root);

    if (root->transitivelyContainsExtensions) _extendModules(modules);
    css->elements().insert(css->elements().begin(), imports.begin(), imports.end());
    return css.detach();
  }


  void Eval::visitCssAtRule(CssAtRule* node)
  {
    // Like dart-sass, not allowed within nested declarations
    if (!declarationName.empty()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "At-rules may not be used within nested declarations.");
    }

    if (node->empty()) {
      current->addChildAt(node, false);
      return;
    }

    sass::string normalized(StringUtils::unvendor(node->name()));
    bool isKeyframe = normalized == "keyframes";
    // Like dart-sass, flags are only set (nested at-rules inherit them)
    RAII_FLAG(inUnknownAtRule, inUnknownAtRule || !isKeyframe);
    RAII_FLAG(inKeyframes, inKeyframes || isKeyframe);

    // Like dart-sass (1.97.3), keep it nested with plain CSS nesting
    if (hasCssNesting()) {
      CssAtRuleObj copy = SASS_MEMORY_RESECT(node);
      current->addChildAt(copy, true);
      RAII_OBJ(CssParentNode, current, copy);
      for (const auto& child : node->elements()) {
        child->accept(this);
      }
      return;
    }
    auto chroot = current->bubbleThrough(true);

    // ModifiableCssKeyframeBlock
    //CssAtRuleObj css = SASS_MEMORY_NEW(CssAtRule,
    //  node->pstate(), pu, name, value, node->isChildless());

    CssAtRuleObj copy = SASS_MEMORY_RESECT(node);

    // Adds new empty atRule to Root!
    chroot->addChildAt(copy, false);

    RAII_OBJ(CssParentNode, current, copy);

    // Like dart-sass, we don't have to check for an unknown at-rule in a
    // style rule here, because the previous compilation has already bubbled
    // the at-rule to the root. Nested style rules are resolved against the
    // current one (e.g. `a { @include meta.load-css(...) }`) when visited.
    // Note: [node] belongs to the loaded module and must not be modified.
    for (const auto& child : node->elements()) {
      child->accept(this);
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

  // Re-adds an already evaluated keyframe block (e.g. from `meta.load-css`)
  // under the current parent, like dart-sass and `visitCssAtRule` do.
  void Eval::visitCssKeyframeBlock(CssKeyframeBlock* css)
  {
    // Add a copy without children (bubbles through style rules)
    CssParentNode* chroot = current->bubbleThrough(true);
    CssKeyframeBlockObj copy = SASS_MEMORY_RESECT(css);
    chroot->addChildAt(copy, false);
    // Visit the children with the copy as the parent
    RAII_OBJ(CssParentNode, current, copy);
    for (const auto& child : css->elements()) {
      child->accept(this);
    }
  }

  void Eval::visitCssMediaRule(CssMediaRule* node)
  {
    // Like dart-sass, not allowed within nested declarations
    if (!declarationName.empty()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Media rules may not be used within nested declarations.");
    }

    CssMediaQueryVectorObj parsed(node->queries());
    // Like dart-sass (1.97.3), keep it nested with plain CSS nesting
    if (hasCssNesting()) {
      CssMediaRuleObj css = SASS_MEMORY_NEW(CssMediaRule,
        node->pstate(), current, parsed);
      current->addChildAt(css, true);
      acceptChildrenAt(css, node->elements());
      return;
    }

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

    // Create a new rule instead of adding [node] itself, since [node]
    // belongs to the loaded module and its children must be evaluated
    // in the current context (e.g. nested in the current style rule).
    CssMediaRuleObj css = SASS_MEMORY_NEW(CssMediaRule,
      node->pstate(), current, mergedQueries);

    // Like dart-sass, the merged sources are empty if not merged
    if (bubbleQuery && mediaQueries && !mediaQueries->empty()) {
      collectMediaQuerySources(uses, parsed);
    }
    else {
      uses.clear();
    }

    CssParentNode* chroot = current;
    while (BubbleMediaQuery(chroot, uses, bubbleQuery)) {
      if (!chroot->parent()) break;
      chroot = chroot->parent();
    }

    chroot->addChildAt(css, true);

    RAII_OBJ(CssParentNode, current, css);
    RAII_OBJ(CssMediaQueryVector, mediaQueries, mergedQueries);
    LocalOption<CssMediaQueryVector> sources(mediaQuerySources, uses);

    // If we're in a style rule, copy it into the media rule so that
    // declarations immediately inside @media have somewhere to go.
    acceptChildrenAt(hoistStyleRule(css), node->elements());

  }

  void Eval::visitCssRoot(CssRoot* css)
  {
    for (const auto& statement : css->elements()) {
      statement->accept(this);
    }
  }

  // Whether [list] contains a parent selector `&`, either as an
  // explicit parent flag or a plain CSS parent selector, including
  // selectors nested inside pseudo selector arguments.
  bool Eval::containsParentSelector(const SelectorList* list)
  {
    if (list == nullptr) return false;
    for (const auto& complex : list->elements()) {
      for (const auto& component : complex->elements()) {
        const auto& compound = component->selector();
        if (compound == nullptr) continue;
        if (compound->withExplicitParent()) return true;
        for (const auto& simple : compound->elements()) {
          if (simple->isaCssParentSelector()) return true;
          if (const auto* pseudo = simple->isaPseudoSelector()) {
            if (containsParentSelector(pseudo->selector())) return true;
          }
        }
      }
    }
    return false;
  }

  // Returns [list] with a leading `&` of a compound selector (in plain css)
  // turned into an explicit parent, as used by `resolveParentSelectors`.
  // Only the first such compound of each complex selector is converted.
  // The selectors of compiled css are shared (e.g. emitted again with
  // `meta.load-css()`), so they are copied instead of changed in place.
  static SelectorList* withExplicitParents(SelectorList* list)
  {
    SelectorListObj result;
    for (size_t i = 0; i < list->size(); i += 1) {
      ComplexSelector* complex = list->get(i);
      for (size_t j = 0; j < complex->size(); j += 1) {
        CplxSelComponent* component = complex->get(j);
        CompoundSelector* compound = component->selector();
        if (compound == nullptr || compound->empty()) continue;
        if (!compound->first()->isaCssParentSelector()) continue;
        CompoundSelectorObj resolved = SASS_MEMORY_COPY(compound);
        resolved->withExplicitParent(true);
        resolved->erase(resolved->begin());
        CplxSelComponentObj copy = SASS_MEMORY_NEW(
          CplxSelComponent, component);
        copy->selector(resolved);
        ComplexSelectorObj changed = SASS_MEMORY_COPY(complex);
        changed->set(j, copy);
        if (result.isNull()) result = SASS_MEMORY_COPY(list);
        result->set(i, changed);
        break;
      }
    }
    return result.isNull() ? list : result.detach();
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
    // Like dart-sass, check the style rule (an at-rule may be in between)
    bool nest = !plainCss && (_styleRule() ? !_styleRule()->fromPlainCss22() : true);
    // Whether we are outside of any style rule
    bool topLevel = _styleRule() == nullptr;
    // Plain CSS rules with an explicit `&` are kept nested as-is
    if (nest && _styleRule() && css->fromPlainCss22() &&
      containsParentSelector(css->selector())) nest = false;
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
      // Selectors that are not nested are kept as-is
      if (nest) slist = withExplicitParents(slist);

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

      // Add child to our parent (only copy parent when bubbling)
      chroot->addChildAt(child, nest);
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

    // Like dart-sass, mark the end of a top-level group
    // (a blank line follows it in expanded output)
    if (topLevel && !current->empty()) {
      current->last()->isGroupEnd(true);
    }

  }

  void Eval::visitCssSupportsRule(CssSupportsRule* node)
  {
    // Like dart-sass, not allowed within nested declarations
    if (!declarationName.empty()) {
      CallStackFrame frame(logger, node->pstate());
      throw Exception::RuntimeException(logger,
        "Supports rules may not be used within nested declarations.");
    }
    // Like `visitSupportsRule`: add a copy of the rule (bubbling through
    // style rules) and evaluate the children inside of it. It used to
    // visit the children only, which dropped the `@supports` wrapper.
    // Like dart-sass (1.97.3), keep it nested with plain CSS nesting
    if (hasCssNesting()) {
      CssSupportsRuleObj copy = SASS_MEMORY_RESECT(node);
      current->addChildAt(copy, true);
      acceptChildrenAt(copy, node->elements());
      return;
    }
    CssParentNode* chroot = current->bubbleThrough(true);
    CssSupportsRuleObj copy = SASS_MEMORY_RESECT(node);
    chroot->addChildAt(copy, false);
    // If we're in a style rule, copy it into the supports rule so that
    // declarations immediately inside @supports have somewhere to go.
    acceptChildrenAt(hoistStyleRule(copy), node->elements());
  }

}
