#ifndef SASS_AST_CSS_HPP
#define SASS_AST_CSS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_nodes.hpp"
#include "ast_selectors.hpp"
#include "visitor_css.hpp"
#include "ast_statements.hpp"
#include "environment.hpp"
#include "extension.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Base class for all css related AST nodes.
  /////////////////////////////////////////////////////////////////////////

  class CssNode : public AstNode,
    public CssVisitable<void>,
    public CssVisitable<bool>
  {

  private:

    // Whether this was generated from the last node in a
    // nested Sass tree that got flattened during evaluation.
    // ADD_CONSTREF(bool, isGroupEnd);

  public:

    // Value constructor
    CssNode(const SourceSpan& pstate);

    // Copy constructor
    CssNode(const CssNode* ptr);

    // Needed here to avoid ambiguity from base-classes!??
    void accept(CssVisitor<void>* visitor) override = 0;
    bool accept(CssVisitor<bool>* visitor) override = 0;

    bool isInvisibleOtherThanBogusCombinators() const;

    virtual bool isInvisible() const;

    // Return if node should be printed (to be specialized).
    virtual bool isInvisibleCss() const;

    virtual bool isInvisibleHidingComments() const;

    // Returns the at-rule name for [node], or `null` if it's not an at-rule.
    virtual const sass::string& getAtRuleName() const { return Strings::empty; }

    virtual CssNode* produce() { return this; }

    // Is this really obsolete now?
    // size_t tabs() const { return 0; }
    // void tabs(size_t tabs) const { }

    // Declare up-casting methods
    DECLARE_ISA_CASTER(CssComment);
    DECLARE_ISA_CASTER(CssImport);
    DECLARE_ISA_CASTER(CssRoot);
    DECLARE_ISA_CASTER(CssAtRule);
    DECLARE_ISA_CASTER(CssMediaRule);
    DECLARE_ISA_CASTER(CssStyleRule);
    DECLARE_ISA_CASTER(CssKeyframeBlock);
    DECLARE_ISA_CASTER(CssSupportsRule);
    DECLARE_ISA_CASTER(CssParentNode);
    DECLARE_ISA_CASTER(CssDeclaration);
    FINALIZE_AST_NODE(CssNode);
  };
  // EO CssNode

  /////////////////////////////////////////////////////////////////////////
  // Base class for css nodes that can have children and a parent.
  /////////////////////////////////////////////////////////////////////////

  class CssParentNode : public CssNode,
    public Vectorized<CssNode>
  {
  private:

    // This must be a pointer to avoid circular references
    // Means it has a possibility of being a dangling pointer
    ADD_PROPERTY(CssParentNode*, parent);
    ADD_PROPERTY(bool, fromPlainCss22);

  public:

    // Value constructor
    CssParentNode(
      const SourceSpan& pstate,
      CssParentNode* parent,
      CssNodeVector&& children = {});

    // Copy constructor
    CssParentNode(
      const CssParentNode* ptr,
      bool childless = false);

    // Adds [node] as a child of the given [parent]. The parent
    // is copied unless it's the latter most child of its parent.
    void addChildAt(CssParentNode* node, bool outOfOrder = false);

    // Return false if a single item is visible
    bool isInvisibleCss() const override;

    // Must be implemented in derived classes
    virtual CssParentNode* copy(SASS_MEMORY_ARGS bool childless) const = 0;

    // Returns if items should bubble further up (to be specialized)
    virtual bool bubbles(bool stopAtMediaRule = false) const { return false; }

    // Helper function to bubble through parents
    CssParentNode* bubbleThrough(bool stopAtMediaRule = false)
    {
      return parent_ && bubbles(stopAtMediaRule) ?
        parent_->bubbleThrough(stopAtMediaRule) : this;
    }

    // Helper function to bubble through parents
    CssParentNode* bubbleThroughCss()
    {
      return parent_ && (parent_->isaCssStyleRule() || parent_->isaCssRoot()) ?
        parent_->bubbleThroughCss() : this;
    }

    /// Returns whether [this] is equal to [other], ignoring their child nodes.
    virtual bool equalsIgnoringChildren(CssNode* other) const { return this == other; }

    // Declare up-casting methods
    OVERRIDE_ISA_CASTER(CssRoot);
    OVERRIDE_ISA_CASTER(CssAtRule);
    OVERRIDE_ISA_CASTER(CssMediaRule);
    OVERRIDE_ISA_CASTER(CssStyleRule);
    OVERRIDE_ISA_CASTER(CssKeyframeBlock);
    OVERRIDE_ISA_CASTER(CssSupportsRule);
    // Define isaCssAtRule up-cast function
    IMPLEMENT_ISA_CASTER(CssParentNode);
    FINALIZE_AST_NODE(CssParentNode);
  };
  // EO CssParentNode

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Media Queries after they have been evaluated
  // Representing the static or resulting css
  class CssMediaQuery final : public AstNode {

    // The media type, for example "screen" or "print".
    // This may be `null`. If so, [features] will not be empty.
    ADD_CONSTREF(sass::string, type);

    // The modifier, probably either "not" or "only".
    // This may be `null` if no modifier is in use.
    ADD_CONSTREF(sass::string, modifier);

    // Is it an `and` or `or` group
    ADD_CONSTREF(bool, conjunction);

    // Feature queries, including parentheses.
    ADD_CONSTREF(StringVector, features);

  public:

    // Value copy constructor
    // Only used when merging
    CssMediaQuery(
      const SourceSpan& pstate,
      const sass::string& type,
      const sass::string& modifier,
      const StringVector& features);

    // Value move constructor
    CssMediaQuery(
      const SourceSpan& pstate,
      sass::string&& type,
      sass::string&& modifier,
      StringVector&& features);

    // Value move constructor
    CssMediaQuery(
      const SourceSpan& pstate,
      StringVector&& conditions,
      bool conjunction = true);

    // Returns true if this query is empty
    // Meaning it has no type and features
    bool empty() const {
      return type_.empty()
        && modifier_.empty()
        && features_.empty();
    }

    // Whether this media query matches all media types.
    bool matchesAllTypes() const {
      return type_.empty() || StringUtils::equalsIgnoreCase(type_, "all", 3);
    }

    // Check if two instances are considered equal
    bool operator==(const CssMediaQuery& rhs) const;

    // Merges this with [other] and adds a query that matches the intersection
    // of both inputs to [result]. Returns false if the result is unrepresentable
    CssMediaQuery* merge(CssMediaQuery* other);

    // IMPLEMENT_ISA_CASTER(CssMediaQuery);
    FINALIZE_AST_NODE(CssMediaQuery);
  };
  // EO CssMediaQuery

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssComment final : public CssNode
  {
  private:
    ADD_CONSTREF(sass::string, text);
    ADD_CONSTREF(bool, isPreserved);
    ADD_CONSTREF(bool, isNewline);
  public:
    CssComment(const SourceSpan& pstate,
      sass::string&& text,
      bool preserve = false,
      bool newline = false);
    CssComment(const CssComment* ptr);

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssComment(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssComment(this);
    }
    IMPLEMENT_ISA_CASTER(CssComment);
    FINALIZE_AST_NODE(CssComment);
  };
  // EO CssComment

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssDeclaration final : public CssNode
  {
  private:
    // The name of this declaration.
    ADD_CONSTREF(sass::string, name);
    // The value of this declaration.
    ADD_CONSTREF(ValueObj, value);
    // Was original declaration a custom property
    ADD_CONSTREF(bool, wasCustomProperty);
  public:
    CssDeclaration(const SourceSpan& pstate,
      const sass::string& name, Value* value,
      bool wasCustomProperty = false);
    CssDeclaration(const CssDeclaration* ptr);

    inline bool isCustomProperty() const {
      return name_[0] == '-'
        && name_[1] == '-';
    }

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssDeclaration(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssDeclaration(this);
    }

    IMPLEMENT_ISA_CASTER(CssDeclaration);
    FINALIZE_AST_NODE(CssDeclaration);
  };
  // EO CssDeclaration

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // A css import is static in nature and
  // can only have one single import url.
  class CssImport final : public CssNode
  {
  private:

    // The url including quotes.
    ADD_CONSTREF(sass::string, url);

    // The supports condition attached to this import.
    ADD_CONSTREF(sass::string, modifiers);

    // The media query attached to this import.
    // ADD_CONSTREF(CssMediaQueryVector, media);

    // Flag to hoist import to the top.
    // This case is possible if an `@import` within
    // an imported css file is inside a `CssStyleRule`.
    ADD_CONSTREF(bool, outOfOrder);

  public:

    // Standard value constructor
    CssImport(
      const SourceSpan& pstate,
      sass::string&& url,
      sass::string&& modifiers);

    // Copy constructor
    CssImport(const CssImport* ptr);

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssImport(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssImport(this);
    }

    IMPLEMENT_ISA_CASTER(CssImport);
    FINALIZE_AST_NODE(CssImport);
  };
  // EO CssImport

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssRoot final : public CssParentNode
  {
  public:

    // Value constructor
    CssRoot(
      const SourceSpan& pstate,
      CssNodeVector&& children = {});

    // Copy constructor
    CssRoot(
      const CssRoot* ptr,
      bool childless = false);

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssRoot(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssRoot(this);
    }

    CssNode* produce() final {
      CssNodeVector copy;
      for (CssNode* child : elements_) {
          copy.emplace_back(child->produce());
      }
      return SASS_MEMORY_NEW(CssRoot,
        pstate_, std::move(copy));
    }

    CssRoot* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssRoot, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    IMPLEMENT_ISA_CASTER(CssRoot);
    FINALIZE_AST_NODE(CssRoot);
  };
  // EO CssRoot

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssAtRule final : public CssParentNode
  {
  private:

    ADD_CONSTREF(sass::string, name);

    ADD_CONSTREF(sass::string, value);

    // Whether the rule has no children and should be emitted
    // without curly braces. This implies `children.isEmpty`,
    // but the reverse is not true - for a rule like `@foo {}`,
    // [children] is empty but [isChildless] is `false`.
    // It means we didn't see any `{` when parsed.
    ADD_CONSTREF(bool, isChildless);

  public:

    // Value move constructor
    CssAtRule(
      const SourceSpan& pstate,
      CssParentNode* parent,
      sass::string&& name,
      sass::string&& value,
      bool isChildless = false,
      CssNodeVector&& children = {});

    // Copy by ptr constructor
    CssAtRule(
      const CssAtRule* ptr,
      bool childless = false);

    bool isInvisibleCss() const final {
      return false;
    }

    // Returns the at-rule name for [node], or `null` if it's not an at-rule.
    const sass::string& getAtRuleName() const final {
      return name_;
    }

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssAtRule(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssAtRule(this);
    }

    CssAtRule* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssAtRule, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    CssAtRule* produce() final {
      CssAtRuleObj copy = SASS_MEMORY_NEW(CssAtRule, this, false);
      for (CssNode* child : elements_) {
        copy->append(child->produce());
      }
      return copy.detach();
    }

    // Define isaCssAtRule up-cast function
    IMPLEMENT_ISA_CASTER(CssAtRule);
    FINALIZE_AST_NODE(CssAtRule);
  };
  // EO CssAtRule

  /////////////////////////////////////////////////////////////////////////
  // A block within a `@keyframes` rule.
  // For example, `10% {opacity: 0.5}`.
  /////////////////////////////////////////////////////////////////////////
  class CssKeyframeBlock final : public CssParentNode
  {
  private:

    // The selector for this block.
    ADD_CONSTREF(StringVector, selector);

  public:

    // Value constructor
    CssKeyframeBlock(
      const SourceSpan& pstate,
      CssParentNode* parent,
      StringVector&& selector,
      CssNodeVector&& children = {});

    // Copy constructor
    CssKeyframeBlock(
      const CssKeyframeBlock* ptr,
      bool childless = false);

    // Return a copy with empty children
    // CssKeyframeBlock* copyWithoutChildren();

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssKeyframeBlock(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssKeyframeBlock(this);
    }

    CssKeyframeBlock* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssKeyframeBlock, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    IMPLEMENT_ISA_CASTER(CssKeyframeBlock);
    FINALIZE_AST_NODE(CssKeyframeBlock);
  };
  // EO CssKeyframeBlock

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssStyleRule final : public CssParentNode
  {
  private:

    SelectorListObj selector_;


    ADD_CONSTREF(BoxObj, boxsel);

    ADD_CONSTREF(SelectorListObj, original98);
    // ADD_CONSTREF(bool, fromPlainCss);

  public:

    SelectorList* selector() { return boxsel_ && boxsel_->_inner ? boxsel_->_inner->value : nullptr; }
    SelectorList* selector() const { return boxsel_ && boxsel_->_inner ? boxsel_->_inner->value : nullptr; }

    // Value constructor
    //CssStyleRule(
    //  const SourceSpan& pstate,
    //  CssParentNode* parent,
    //  SelectorList* selector,
    //  CssNodeVector&& children = {});

    CssStyleRule(
      const SourceSpan& pstate,
      CssParentNode* parent,
      Box* selector,
      CssNodeVector&& children = {});

    // Copy constructor
    CssStyleRule(
      const CssStyleRule* ptr,
      bool childless = false);

    // Selector and one child must be visible
    bool isInvisibleCss() const final;

    // Media rules are sometimes transparent, sometimes not
    bool bubbles(bool stopAtMediaRule) const final { return true; }

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssStyleRule(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssStyleRule(this);
    }

    CssStyleRule* produce() final {
      CssNodeVector copy;
      for (CssNode* child : elements_) {
        copy.emplace_back(child->produce());
      }
      return SASS_MEMORY_NEW(CssStyleRule,
        pstate_, parent_,
        boxsel_,
        std::move(copy));
    }

    // Declare via macro to allow line/col debugging
    CssStyleRule* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssStyleRule, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    // Define isaCssStyleRule up-cast function
    IMPLEMENT_ISA_CASTER(CssStyleRule);
    FINALIZE_AST_NODE(CssStyleRule);
  };
  // EO CssStyleRule

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class CssSupportsRule final : public CssParentNode
  {
  private:

    ADD_CONSTREF(ValueObj, condition);

  public:

    // Value constructor
    CssSupportsRule(
      const SourceSpan& pstate,
      CssParentNode* parent,
      ValueObj condition,
      CssNodeVector&& children = {});

    // Copy constructor
    CssSupportsRule(
      const CssSupportsRule* ptr,
      bool childless = false);

    // Returns the at-rule name for [node], or `null` if it's not an at-rule.
    const sass::string& getAtRuleName() const final { return Strings::supports; }

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssSupportsRule(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssSupportsRule(this);
    }

    // Declare via macro to allow line/col debugging
    CssSupportsRule* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssSupportsRule, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    // Define isaCssSupportsRule up-cast function
    IMPLEMENT_ISA_CASTER(CssSupportsRule);
    FINALIZE_AST_NODE(CssSupportsRule);
  };
  // EO CssSupportsRule

  /////////////////////////////////////////////////////////////////////////
  // A plain CSS `@media` rule after it has been evaluated.
  /////////////////////////////////////////////////////////////////////////
  class CssMediaRule final : public CssParentNode
  {
  private:

    // Queries for this media rule (might be nullptr).
    ADD_CONSTREF(CssMediaQueryVectorObj, queries);

  public:

    // Value constructor
    CssMediaRule(const SourceSpan& pstate,
      CssParentNode* parent,
      CssMediaQueryVector* queries,
      CssNodeVector&& children = {});

    // Copy constructor
    CssMediaRule(
      const CssMediaRule* ptr,
      bool childless = false);

    // Check if we or any children are invisible
    bool isInvisibleCss() const final {
      return queries_.isNull() || queries_->empty() ||
        CssParentNode::isInvisibleCss();
    }

    // Media rules are sometimes transparent, sometimes not
    bool bubbles(bool stopAtMediaRule) const final {
      return stopAtMediaRule == false;
    }

    // Returns the at-rule name for [node], or `null` if it's not an at-rule.
    const sass::string& getAtRuleName() const final { return Strings::media; }

    // Css visitor and rendering entry function
    void accept(CssVisitor<void>* visitor) final {
      return visitor->visitCssMediaRule(this);
    }
    bool accept(CssVisitor<bool>* visitor) final {
      return visitor->visitCssMediaRule(this);
    }

    // Check if two instances are considered equal
    // Used by Extension::assertCompatibleMediaContext
    bool operator==(const CssMediaRule& rhs) const;

    // Declare via macro to allow line/col debugging
    CssMediaRule* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CssMediaRule, this, childless);
    }

    bool equalsIgnoringChildren(CssNode* other) const final;

    // Define isaCssMediaRule up-cast function
    IMPLEMENT_ISA_CASTER(CssMediaRule);
    FINALIZE_AST_NODE(CssMediaRule);
  };
  // EO CssMediaRule

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
