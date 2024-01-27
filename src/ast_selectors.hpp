/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_AST_SELECTORS_HPP
#define SASS_AST_SELECTORS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_nodes.hpp"
#include "constants.hpp"
#include "visitor_selector.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Some helpers for superselector and weave parts
  /////////////////////////////////////////////////////////////////////////

  bool compoundIsSuperselector(
    const CompoundSelector* compound1,
    const CompoundSelector* compound2,
    const CplxSelComponentVector& parents = {});

  bool complexIsParentSuperselector(
    const CplxSelComponentVector& complex1,
    const CplxSelComponentVector& complex2);

  sass::vector<ComplexSelectorObj> weave(
    const sass::vector<ComplexSelectorObj>& complexes,
    bool forceLineBreak = false);

  // ToDo: What happens if we modify our parent?
  sass::vector<ComplexSelectorObj> weaveParents(
    ComplexSelector* prefix, ComplexSelector* base);

  sass::vector<ComplexSelectorObj> _unifyComplex(
    sass::vector<ComplexSelectorObj> complexes,
    const SourceSpan& pstate);

  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for CSS selectors.
  /////////////////////////////////////////////////////////////////////////

  class Selector : public AstNode,
    public SelectorVisitable<void>,
    public SelectorVisitable<bool>
  {
  protected:

    // Hash is only calculated once and afterwards the value
    // must not be mutated, which is the case with how sass
    // works, although we must be a bit careful not to alter
    // any value that has already been added to a set or map.
    // Must create a copy if you need to alter such an object.
    // Selectors are mostly used as keys in @extend rules.
    mutable size_t hash_;

  public:

    // Base value constructor
    Selector(const SourceSpan& pstate);

    // Base copy constructor
    Selector(const Selector* ptr);

    bool isUseless() const;

    bool isBogusStrict() const;
    bool isBogusLenient() const;

    bool isInvisibleOtherThanBogusCombinators() const;
    bool isBogusOtherThanLeadingCombinator() const;

    bool isInvisible() const;

    // To be implemented by specialization
    virtual size_t hash() const = 0;
    virtual unsigned long specificity() const = 0;
    // By default we return the regular specificity
    // Override this for selectors with children
    virtual unsigned long maxSpecificity() const { return specificity(); }
    virtual unsigned long minSpecificity() const { return specificity(); }

    // Specialized by CompoundSelector
    virtual bool hasPlaceholder() const { return false; }

    // Convert the selector to string, mostly for debugging
    sass::string inspect(int precision = SassDefaultPrecision) const;

    // Returns if any compound selector has an explicit parent `&` selector.
    // Only compound selectors are allowed to have this beside interpolations,
    // which are handled very different and separately. Pseudo-selector like
    // `:not` can also have an impact here, which is currently the sole use
    // for having this as a virtual function. It is certainly questionable
    // why a list returns true here if only one compound selector has it!?
    virtual const Selector* hasAnyExplicitParent() const { return nullptr; }

    // Calls the appropriate visit method on [visitor].
    // Needed here to avoid ambiguity from base-classes!??
    virtual void accept(SelectorVisitor<void>* visitor) override = 0;
    virtual bool accept(SelectorVisitor<bool>* visitor) override = 0;

    // To be implemented by specialization
    virtual bool operator==(const Selector& rhs) const = 0;

    // Base copy method with [childless] being void most of the times
    virtual Selector* copy(SASS_MEMORY_ARGS bool childless = false) const = 0;

    // Declare up-casting methods
    DECLARE_ISA_CASTER(IDSelector);
    DECLARE_ISA_CASTER(TypeSelector);
    DECLARE_ISA_CASTER(PseudoSelector);
    DECLARE_ISA_CASTER(ClassSelector);
    DECLARE_ISA_CASTER(AttributeSelector);
    DECLARE_ISA_CASTER(PlaceholderSelector);
    DECLARE_ISA_CASTER(SelectorNS);
    DECLARE_ISA_CASTER(SimpleSelector);
    DECLARE_ISA_CASTER(ComplexSelector);
    //DECLARE_ISA_CASTER(SelectorCombinator);
    DECLARE_ISA_CASTER(CompoundSelector);
    DECLARE_ISA_CASTER(SelectorList);
  };

  /////////////////////////////////////////////////////////////////////////
  // Abstract base class for simple selectors.
  /////////////////////////////////////////////////////////////////////////

  class SimpleSelector : public Selector
  {
  private:

    ADD_CONSTREF(sass::string, name);

  public:

    // Value constructor
    SimpleSelector(
      const SourceSpan& pstate,
      const sass::string& name);

    // Value constructor
    SimpleSelector(
      const SourceSpan& pstate,
      sass::string&& name);

    // Copy constructor
    SimpleSelector(
      const SimpleSelector* ptr);

    // Wrap inside another selector type
    ComplexSelector* wrapInComplex(SelectorCombinatorVector prefixes);
    CompoundSelector* wrapInCompound();

    // Implement hash functionality
    virtual size_t hash() const override;

    // Implement for cleanup phase
    virtual bool empty() const {
      return name().empty();
    }

    // Unify simple selector with multiple simple selectors
    virtual sass::vector<SimpleSelectorObj> unify(
      const sass::vector<SimpleSelectorObj>& other);

    // Returns true if name equals '*'
    bool isUniversal() const {
      return name_ == "*";
    }

    // Checker if the name
    virtual bool nsMatch(const SimpleSelector& r) const { return true; }

    virtual bool hasInvisible() const { return false; }

    virtual bool isSuperselector(SimpleSelector* other) const;

    virtual bool isSuperselectorAF(SimpleSelector* other) const;

    // This is a very interesting line, as it seems pointless, since the base class
    // already marks this as an unimplemented interface methods, but by defining this
    // line here, we make sure that callers know the return is a bit more specific.
    virtual SimpleSelector* copy(SASS_MEMORY_ARGS bool childless = false) const override = 0;

    IMPLEMENT_ISA_CASTER(SimpleSelector);

  };

  /////////////////////////////////////////////////////////////////////////
  // Base class for all selectors that support name-spaces
  /////////////////////////////////////////////////////////////////////////

  struct QualifiedName {
    sass::string name;
    sass::string ns;
    bool hasNs;
  };

  class SelectorNS : public SimpleSelector
  {
  private:

    ADD_CONSTREF(bool, hasNs);
    ADD_CONSTREF(sass::string, ns);

  public:

    // Value constructor
    SelectorNS(
      const SourceSpan& pstate,
      sass::string&& name,
      sass::string&& ns,
      bool hasNs = false);

    // Copy constructor
    SelectorNS(
      const SelectorNS* ptr);

    // Implement hash functionality
    virtual size_t hash() const override;

    // Implement for cleanup phase
    virtual bool empty() const override {
      return ns().empty() && SimpleSelector::empty();
    }

    // Returns true if namespaces match exactly
    bool nsEqual(const SelectorNS& rhs) const {
      return hasNs_ == rhs.hasNs_ && ns_ == rhs.ns_;
    }

    // Returns true if namespaces are considered compatible
    inline bool nsMatch(const SelectorNS& rhs) const {
      return isUniversalNs() || nsEqual(rhs);
    }

    // Returns true if namespace was explicitly set to '*'
    inline bool isUniversalNs() const {
      return hasNs_ && ns_ == "*";
    }

    // Up-casts the right hand side first to find specialization
    bool nsMatch(const SimpleSelector& rhs) const override final;

    // This is a very interesting line, as it seems pointless, since the base class
    // already marks this as an unimplemented interface methods, but by defining this
    // line here, we make sure that callers know the return is a bit more specific.
    virtual SelectorNS* copy(SASS_MEMORY_ARGS bool childless = false) const override = 0;

    IMPLEMENT_ISA_CASTER(SelectorNS);

    SelectorNS* unity(SelectorNS* rhs);
  };

  /////////////////////////////////////////////////////////////////////////
  // A placeholder selector. (e.g. `%foo`). This doesn't match any elements.
  // It's intended to be extended using `@extend`. It's not a plain CSS
  // selector — it should be removed before emitting a CSS document.
  /////////////////////////////////////////////////////////////////////////
  class PlaceholderSelector final : public SimpleSelector
  {
  public:

    // Value constructor
    PlaceholderSelector(
      const SourceSpan& pstate,
      const sass::string& name);

    // Copy constructor
    PlaceholderSelector(
      const PlaceholderSelector* ptr);

    // Implement specialized specificity function
    unsigned long specificity() const override final {
      return Constants::Specificity::Base;
    }

    virtual bool hasPlaceholder() const override final { return true; }


    // Returns whether this is a private selector.
    // That is, whether it begins with `-` or `_`.
    bool isPrivate93() const {
      if (name_[1] == 0) return false;
      return name_[1] == Character::$minus
        || name_[1] == Character::$underscore;
    }

    IMPLEMENT_SEL_COPY_IGNORE(PlaceholderSelector);
    IMPLEMENT_ACCEPT(void, Selector, PlaceholderSelector);
    IMPLEMENT_ACCEPT(bool, Selector, PlaceholderSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, PlaceholderSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(PlaceholderSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // A type selector. (e.g., `div`, `span` or `*`).
  // This selects elements whose name equals the given name.
  /////////////////////////////////////////////////////////////////////////

  class TypeSelector final : public SelectorNS
  {
  public:

    // Value constructor
    TypeSelector(
      const SourceSpan& pstate,
      sass::string&& name,
      sass::string&& ns,
      bool hasNs = false);

    // Copy constructor
    TypeSelector(
      const TypeSelector* ptr);

    bool isSuperselectorAF(SimpleSelector* other) const override final;

    // Implement specialized specificity function
    virtual unsigned long specificity() const override {
      return isUniversal() ? 0 : Constants::Specificity::Element;
    }

    sass::vector<SimpleSelectorObj> unifyUniversal(const sass::vector<SimpleSelectorObj>& compound);

    // Unify Type selector with multiple simple selectors
    // CompoundSelector* unifyWith(CompoundSelector*);
    virtual sass::vector<SimpleSelectorObj> unify(
      const sass::vector<SimpleSelectorObj>& other)
        override final;

    // Unify two simple selectors with each other
    // SimpleSelector* unifyWith(const SimpleSelector*);

    IMPLEMENT_SEL_COPY_IGNORE(TypeSelector);
    IMPLEMENT_ACCEPT(void, Selector, TypeSelector);
    IMPLEMENT_ACCEPT(bool, Selector, TypeSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, TypeSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(TypeSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // Class selectors  -- i.e., .foo.
  /////////////////////////////////////////////////////////////////////////

  class ClassSelector final : public SimpleSelector
  {
  public:

    // Value constructor
    ClassSelector(
      const SourceSpan& pstate,
      const sass::string& name);

    // Copy constructor
    ClassSelector(
      const ClassSelector* ptr);

    // Implement specialized specificity function
    virtual unsigned long specificity() const override {
      return Constants::Specificity::Class;
    }

    IMPLEMENT_SEL_COPY_IGNORE(ClassSelector);
    IMPLEMENT_ACCEPT(void, Selector, ClassSelector);
    IMPLEMENT_ACCEPT(bool, Selector, ClassSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, ClassSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(ClassSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // An ID selector (i.e. `#foo`). This selects elements 
  // whose `id` attribute exactly matches the given name.
  /////////////////////////////////////////////////////////////////////////

  class IDSelector final : public SimpleSelector
  {
  public:

    // Value constructor
    IDSelector(
      const SourceSpan& pstate,
      const sass::string& name);

    // Copy constructor
    IDSelector(
      const IDSelector* ptr);

    // Implement specialized specificity function
    virtual unsigned long specificity() const override {
      return Constants::Specificity::ID;
    }

    // Unify ID selector with multiple simple selectors
    // CompoundSelector* unifyWith(CompoundSelector*);
    virtual sass::vector<SimpleSelectorObj> unify(
      const sass::vector<SimpleSelectorObj>& other)
        override final;

    IMPLEMENT_SEL_COPY_IGNORE(IDSelector);
    IMPLEMENT_ACCEPT(void, Selector, IDSelector);
    IMPLEMENT_ACCEPT(bool, Selector, IDSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, IDSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(IDSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // An attribute selector. This selects for elements
  // with the given attribute, and optionally with a
  // value matching certain conditions as well.
  /////////////////////////////////////////////////////////////////////////

  class AttributeSelector final : public SelectorNS
  {

    // The operator that defines the semantics of [value].
    // If this is empty, this matches any element with the given property,
    // regardless of this value. It's empty if and only if [value] is empty.
    ADD_CONSTREF(sass::string, op);

    // An assertion about the value of [name].
    // The precise semantics of this string are defined by [op].
    // If this is `null`, this matches any element with the given property,
    // regardless of this value. It's `null` if and only if [op] is `null`.
    ADD_CONSTREF(sass::string, value);

      // The modifier which indicates how the attribute selector should be
    // processed. See for example [case-sensitivity][] modifiers.
    // [case-sensitivity]: https://www.w3.org/TR/selectors-4/#attribute-case
    // If [op] is empty, this is always empty as well.
    ADD_CONSTREF(char, modifier);

    // Defines if we parsed an identifier value. Dart-sass
    // does this check again in serialize.visitAttributeSelector.
    // We want to avoid this and do the check at parser stage.
    ADD_CONSTREF(bool, isIdentifier);

  public:

    // By value constructor
    AttributeSelector(
      const SourceSpan& pstate,
      struct QualifiedName&& name,
      sass::string&& op = "",
      sass::string&& value = "",
      bool isIdentifier = false,
      char modifier = 0);

    // Copy constructor
    AttributeSelector(
      const AttributeSelector* ptr);

    // Implement specialized specificity function
    virtual unsigned long specificity() const override {
      return Constants::Specificity::Attr;
    }

    IMPLEMENT_SEL_COPY_IGNORE(AttributeSelector);
    IMPLEMENT_ACCEPT(void, Selector, AttributeSelector);
    IMPLEMENT_ACCEPT(bool, Selector, AttributeSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, AttributeSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(AttributeSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // A pseudo-class or pseudo-element selector (e.g., `:content`
  // or `:nth-child`). The semantics of a specific pseudo selector
  // depends on its name. Some selectors take arguments, including
  // other selectors. Sass manually encodes logic for each pseudo
  // selector that takes a selector as an argument, to ensure that
  // extension and other selector operations work properly.
  /////////////////////////////////////////////////////////////////////////

  class PseudoSelector final : public SimpleSelector
  {

    // Like [name], but without any vendor prefixes.
    ADD_CONSTREF(sass::string, normalized);

    // The non-selector argument passed to this selector. This is
    // `null` if there's no argument. If [argument] and [selector]
    // are both non-`null`, the selector follows the argument.
    ADD_CONSTREF(sass::string, argument);

    // The selector argument passed to this selector. This is `null`
    // if there's no selector. If [argument] and [selector] are
    // both non-`null`, the selector follows the argument.
    ADD_CONSTREF(SelectorListObj, selector);

    // Whether this is syntactically a pseudo-class selector. This is
    // the same as [isClass] unless this selector is a pseudo-element
    // that was written syntactically as a pseudo-class (`:before`,
    // `:after`, `:first-line`, or `:first-letter`). This is
    // `true` if and only if [isSyntacticElement] is `false`.
    ADD_CONSTREF(bool, isSyntacticClass);

    // Whether this is a pseudo-class selector.
    // This is `true` if and only if [isPseudoElement] is `false`.
    ADD_CONSTREF(bool, isClass);

  public:

    bool isElement() const { return !isClass_; }

    // Value constructor
    PseudoSelector(
      const SourceSpan& pstate,
      const sass::string& name,
      bool element = false);

    // Copy constructor
    PseudoSelector(
      const PseudoSelector* ptr);

    bool isSuperSelector(PseudoSelector* other) const;

    bool isSuperselectorAF(SimpleSelector* other) const override final;

    // Returns true if there is a wrapped selector with an
    // explicit `&` parent selector. Certainly questionable
    // since the selector list may have compound selectors
    // with and some without explicit parent selector!?
    const Selector* hasAnyExplicitParent() const override final;

    bool hasInvisible() const override final;

    // Implement hash functionality
    size_t hash() const override final;

    // Implement for cleanup phase
    // Only considered empty if selector is
    // available but has no items in it.
    bool empty() const override final;

    // Whether this is a pseudo-element selector.
    // This is `true` if and only if [isClass] is `false`.
    // A pseudo-element is made of two colons (::) followed by the name.
    // The `::` notation is introduced by the current document in order to
    // establish a discrimination between pseudo-classes and pseudo-elements.
    // For compatibility with existing style sheets, user agents must also
    // accept the previous one-colon notation for pseudo-elements introduced
    // in CSS levels 1 and 2 (namely, :first-line, :first-letter, :before and
    // :after). This compatibility is not allowed for the new pseudo-elements
    // introduced in this specification.
    bool isPseudoElement() const { return !isClass(); }

    // Whether this is syntactically a pseudo-element selector.
    // This is `true` if and only if [isSyntacticClass] is `false`.
    bool isSyntacticElement() const { return !isSyntacticClass(); }

    bool isHost() const { return isClass_ && name_ == "host"; }

    bool isHostContext() const;

    // Returns a new [PseudoSelector] based on ourself,
    // but with the selector replaced with [selector].
    PseudoSelector* withSelector(SelectorList* selector);

    // Implement specialized specificity function
    virtual unsigned long specificity() const override {
      return isPseudoElement()
        ? Constants::Specificity::Element
        : Constants::Specificity::Pseudo;
    }

    // Unify Pseudo selector with multiple simple selectors
    // CompoundSelector* unifyWith(CompoundSelector*);
    virtual sass::vector<SimpleSelectorObj> unify(
      const sass::vector<SimpleSelectorObj>& other)
        override final;

    IMPLEMENT_SEL_COPY_IGNORE(PseudoSelector);
    IMPLEMENT_ACCEPT(void, Selector, PseudoSelector);
    IMPLEMENT_ACCEPT(bool, Selector, PseudoSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, PseudoSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(PseudoSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // Complex Selectors are the most important class of selectors.
  // A Selector List consists of Complex Selectors (separated by comma)
  // Complex Selectors are itself a list of Compounds and Combinators
  // Between each item there is an implicit ancestor of combinator
  /////////////////////////////////////////////////////////////////////////

  class ComplexSelector final : public Selector,
    public Vectorized<CplxSelComponent>
  {


    ADD_CONSTREF(bool, chroots);
    // line break before list separator
    ADD_CONSTREF(bool, hasPreLineFeed);

    // line break after the selector
    ADD_CONSTREF(bool, hasLineBreak);

    ADD_CONSTREF(SelectorCombinatorVector, leadingCombinators);

    // Calculate specificity only once
    mutable unsigned long specificity_ = 0xFFFFFFFF;
    mutable unsigned long maxSpecificity_ = 0xFFFFFFFF;
    mutable unsigned long minSpecificity_ = 0xFFFFFFFF;

  public:

    // bool isUseless() const;

    bool hasOneLeadingCombinators() const;
    SelectorCombinator* getLeadingCombinator() const;

    CompoundSelector* getSingleCompound() const;

    // const SelectorCombinatorVector& leadingCombinators() const;

      // Value constructor
    ComplexSelector(
      const SourceSpan& pstate,
      CplxSelComponentVector&& components = {});

    // Value constructor
    ComplexSelector(
      const SourceSpan& pstate,
      const SelectorCombinatorVector& leadingCombinators,
      const CplxSelComponentVector& components,
      bool hasLineBreak = false);

    // Value constructor
    ComplexSelector(
      const SourceSpan& pstate,
      SelectorCombinatorVector&& leadingCombinators,
      CplxSelComponentVector&& components,
      bool hasLineBreak = false);

    // Copy constructor
    ComplexSelector(
      const ComplexSelector* ptr,
      bool childless = false);

    ComplexSelector* withAdditionalCombinators(const SelectorCombinatorVector& others);

    ComplexSelector* concatenate(ComplexSelector* child, const SourceSpan& span, bool forceLineBreak);

    // Returns true if the first components
    // is a compound selector and fulfills
    // a few other criteria.
    bool hasInvisible() const;

    // Check if any of the selectors is/has a placeholder
    bool hasPlaceholder() const override final;

    // Wrap inside another selector type
    SelectorList* wrapInList();

    // Implement hash functionality
    size_t hash() const override final;

    const Selector* getExplicitParent() const;

    // Convert to value list
    List* toList() const;

    sass::vector<ComplexSelectorObj>
      resolveParentSelectors(
        SelectorList* parent,
        BackTraces& traces,
        bool implicit_parent = true);


    // Unify two complex selectors with each other
    SelectorList* unifyList(ComplexSelector* rhs);

    // Determine if given `this` is a sub-selector of `sub`
    bool isSuperselectorOf(const ComplexSelector* sub) const;

    // Specialize all specificity functions
    unsigned long specificity() const override final;
    unsigned long maxSpecificity() const override final;
    unsigned long minSpecificity() const override final;

    ComplexSelector* produce();

    IMPLEMENT_SEL_COPY_CHILDREN(ComplexSelector);
    IMPLEMENT_ACCEPT(void, Selector, ComplexSelector);
    IMPLEMENT_ACCEPT(bool, Selector, ComplexSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, ComplexSelector)

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(ComplexSelector);

    ComplexSelector* withAdditionalComponent(CplxSelComponent* component, SourceSpan& span, bool forceLineBreak);

  };

  /////////////////////////////////////////////////////////////////////////
  // Base class for complex selector components
  /////////////////////////////////////////////////////////////////////////

  // Enumerate all possible selector combinators. There is some
  // discrepancy with dart-sass. Opted to name them as in CSS33
  enum SelectorPrefix { CHILD /* > */, FOLLOWING /* ~ */, SIBLING /* + */ };

  class CplxSelComponent : public AstNode
  {

    ADD_CONSTREF(SelectorCombinatorVector, combinators);

    // This component's compound selector.
    ADD_CONSTREF(CompoundSelectorObj, selector);

    // line break after list separator
    ADD_CONSTREF(bool, hasPostLineBreak);

  public:

    sass::string inspect() const;

    sass::string inspecter() const;

    void appendCombinators(SelectorCombinatorVector trails);

    CplxSelComponent* withAdditionalCombinators(const SelectorCombinatorVector& combinators);

    // Value constructor
    //CplxSelComponent(
    //  const SourceSpan& pstate,
    //  bool hasPostLineBreak = false);

    // Value constructor
    CplxSelComponent(
      const SourceSpan& pstate,
      SelectorCombinatorVector&& combinators,
      CompoundSelector* selector = nullptr,
      bool hasPostLineBreak = false);

    // Copy constructor
    CplxSelComponent(
      const CplxSelComponent* ptr);

    // Implement hash functionality
    virtual size_t hash() const;
    //void cloneChildren(const Selector*) override;

    // By default we consider instances not empty
    virtual bool empty() const { return false; }

    virtual bool hasInvisible() const { return false; }

    // Wrap inside another selector type
    ComplexSelector* wrapInComplex2();

    ComplexSelector* wrapInComplex(SelectorCombinatorVector);

    // virtual CplxSelComponent* produce() = 0;

    // To be implemented by specialization
    bool operator==(const CplxSelComponent& rhs) const;

    const Selector* hasAnyExplicitParent() const;

    bool hasPlaceholder() const;


    // This is a very interesting line, as it seems pointless, since the base class
    // already marks this as an unimplemented interface methods, but by defining this
    // line here, we make sure that callers know the return is a bit more specific.
    // virtual CplxSelComponent* copy(SASS_MEMORY_ARGS bool childless = false) const override = 0;

  };

  /////////////////////////////////////////////////////////////////////////
  // A specific combinator between compound selectors
  /////////////////////////////////////////////////////////////////////////

  class SelectorCombinator : public AstNode {

    ADD_CONSTREF(SelectorPrefix, combinator);

    ADD_CONSTREF(bool, hasPostLineBreak);

  public:

    // Value constructor
    SelectorCombinator(
      const SourceSpan& pstate,
      SelectorPrefix combinator,
      bool hasPostLineBreak = false);

    // Copy constructor
    SelectorCombinator(
      const SelectorCombinator* ptr);

  public:

    // Some convenient boolean checkers
    bool isChild() const { return combinator_ == CHILD; }
    bool isNextSibling() const { return combinator_ == SIBLING; }
    bool isFollowingSibling() const { return combinator_ == FOLLOWING; }

    // Simple equality operators
    bool operator==(const SelectorCombinator& rhs) const {
      return combinator_ == rhs.combinator_;
    }
    bool operator!=(const SelectorCombinator& rhs) const {
      return combinator_ != rhs.combinator_;
    }

    const sass::string toString() const {
      switch (combinator_) {
        case CHILD: return ">";
        case SIBLING: return "+";
        case FOLLOWING: return "~";
        default: return "[NA]";
      }
    }


  };

  /*
  class SelectorCombinator final : public CplxSelComponent
  {
  public:

    // Enumerate all possible selector combinators. There is some
    // discrepancy with dart-sass. Opted to name them as in CSS33

  private:

    // Store the type of this combinator
    ADD_CONSTREF(Combinator, combinator);

  public:

    // Value constructor
    SelectorCombinator(
      const SourceSpan& pstate,
      Combinator combinator,
      bool hasPostLineBreak = false);

    // Copy constructor
    SelectorCombinator(
      const SelectorCombinator* ptr);

    // Matches the right-hand selector if it's a direct child of the left-
    // hand selector in the DOM tree. Dart-sass also calls this `child`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Child_combinator
    bool isChildCombinator() const { return combinator_ == CHILD; } // >

    // Matches the right-hand selector if it comes after the left-hand
    // selector in the DOM tree. Dart-sass class this `followingSibling`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/General_sibling_combinator
    bool isGeneralCombinator() const { return combinator_ == GENERAL; } // ~

    // Matches the right-hand selector if it's immediately adjacent to the
    // left-hand selector in the DOM tree. Dart-sass calls this `nextSibling`
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Adjacent_sibling_combinator
    bool isAdjacentCombinator() const { return combinator_ == ADJACENT; } // +

    // The combinators do not add anything to the specificity
    unsigned long specificity() const override final { return 0; }

    // Implement hash functionality
    size_t hash() const override final;

    CplxSelComponent* produce() override final {
      return this;
    }

    IMPLEMENT_SEL_COPY_IGNORE(SelectorCombinator);
    IMPLEMENT_ACCEPT(void, Selector, SelectorCombinator);
    IMPLEMENT_ACCEPT(bool, Selector, SelectorCombinator);
    IMPLEMENT_EQ_OPERATOR(Selector, SelectorCombinator);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(SelectorCombinator);
  };
  */
  /////////////////////////////////////////////////////////////////////////
  // A compound selector consists of multiple simple selectors. It will be
  // either implicitly or explicitly connected to its parent sass selector.
  // According to the specs we could also unify the tag selector into this,
  // as AFAICT only one tag selector is ever allowed. Further we could free
  // up the pseudo selectors from being virtual, as they must be last always.
  // https://github.com/sass/libsass/pull/3101
  /////////////////////////////////////////////////////////////////////////
  class CompoundSelector final : public Selector, public Vectorized<SimpleSelector>
  {

    // This is one of the most important flags for selectors.
    // The `&` parent selector can only occur at the start of
    // a compound selector. Interpolations `#{&}` are handle in
    // another code-path. If an explicit parent is given we will
    // not implicitly connect the selector to its scoped parent.
    ADD_CONSTREF(bool, withExplicitParent);

    ADD_CONSTREF(bool, hasPostLineBreak);

    // Calculate specificity only once
    mutable unsigned long specificity_ = 0xFFFFFFFF;
    mutable unsigned long maxSpecificity_ = 0xFFFFFFFF;
    mutable unsigned long minSpecificity_ = 0xFFFFFFFF;

  public:

    // Value Constructor
    CompoundSelector(
      const SourceSpan& pstate,
      bool hasPostLineBreak = false);

    // Value move Constructor
    CompoundSelector(
      const SourceSpan& pstate,
      sass::vector<SimpleSelectorObj>&& selectors,
      bool hasPostLineBreak = false);

    // Copy constructor
    CompoundSelector(
      const CompoundSelector* ptr,
      bool childless = false);

    // Returns true if any selector is invisible.
    bool hasInvisible() const;

    // Implement for cleanup phase
    // Dispatch to underlying list
    bool empty() const {
      return Vectorized::empty();
    }

    // Implement hash functionality
    size_t hash() const override final;

    // Unify two lists of simple selectors (not in dart)
    // CompoundSelector* unifyWith(CompoundSelector* rhs);

    const Selector* hasAnyExplicitParent() const override final;

    bool hasPlaceholder() const override final;

    // Resolve parents and form the final selector
    sass::vector<ComplexSelectorObj>
      resolveParentSelectors2(
        SelectorList* parent,
        BackTraces& traces,
        SelectorCombinatorVector prefixes,
        SelectorCombinatorVector tails,
        bool implicit_parent = true);

    // Determine if given `this` is a sub-selector of `sub`
    bool isSuperselectorOf(const CompoundSelector* sub) const;

    // Specialize all specificity functions
    unsigned long specificity() const override final;
    unsigned long maxSpecificity() const override final;
    unsigned long minSpecificity() const override final;

    CompoundSelector* produce();

    ComplexSelector* wrapInComplex3();

    ComplexSelector* wrapInComplex(SelectorCombinatorVector prefixes, SelectorCombinatorVector tails);

    CplxSelComponent* wrapInComponent(SelectorCombinatorVector postfixes);

    IMPLEMENT_SEL_COPY_CHILDREN(CompoundSelector);
    IMPLEMENT_ACCEPT(void, Selector, CompoundSelector);
    IMPLEMENT_ACCEPT(bool, Selector, CompoundSelector);
    IMPLEMENT_EQ_OPERATOR(Selector, CompoundSelector);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(CompoundSelector);
  };

  /////////////////////////////////////////////////////////////////////////
  // Comma-separated selector groups.
  /////////////////////////////////////////////////////////////////////////
  class SelectorList final : public Selector,
    public Vectorized<ComplexSelector>
  {
  private:

    // Calculate specificity only once
    mutable unsigned long maxSpecificity_ = 0xFFFFFFFF;
    mutable unsigned long minSpecificity_ = 0xFFFFFFFF;

  public:

    SelectorList* assertNotBogus(const sass::string& name);

    // Value move constructor
    SelectorList(
      const SourceSpan& pstate,
      sass::vector<ComplexSelectorObj>&& = {});

    // Copy constructor
    SelectorList(const SelectorList* ptr,
      bool childless = false);

    // Implement hash functionality
    size_t hash() const override final;

    // Unify two selector lists with each other
    SelectorList* unifyWith(SelectorList*);

    const Selector* getExplicitParent() const;

    // Convert to `List` or `Null`
    Value* toValue() const;

    SelectorList* resolveParentSelectors(
      SelectorList* parent,
      BackTraces& traces,
      bool implicit_parent = true);

    // Check if any of the selectors is/has a placeholder
    bool hasPlaceholder() const override final;

    // Determine if given `this` is a sub-selector of `sub`
    bool isSuperselectorOf(const SelectorList* sub) const;

    // This implementation is not available, don't call
    unsigned long specificity() const override final {
      throw std::runtime_error("specificity not implemented");
    }

    // Specialize min and max specificity functions
    unsigned long maxSpecificity() const override final;
    unsigned long minSpecificity() const override final;

    SelectorList* produce() {
      sass::vector<ComplexSelectorObj> copy;
      for (ComplexSelector* child : elements_) {
        copy.emplace_back(child->produce());
      }
      return SASS_MEMORY_NEW(SelectorList,
        pstate_, std::move(copy));
    }

    sass::string toString() const {
      return toValue()->toCss();
    }

    IMPLEMENT_SEL_COPY_CHILDREN(SelectorList);
    IMPLEMENT_ACCEPT(void, Selector, SelectorList);
    IMPLEMENT_ACCEPT(bool, Selector, SelectorList);
    IMPLEMENT_EQ_OPERATOR(Selector, SelectorList);

    // Implement final up-casting method
    IMPLEMENT_ISA_CASTER(SelectorList);
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
