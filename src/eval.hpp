/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_EVAL_HPP
#define SASS_EVAL_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

// #include "context.hpp"
// #include "extender.hpp"
#include "ast_supports.hpp"
#include "ast_callables.hpp"
#include "calc_names.hpp"

namespace Sass {

  /*#####################################################################*/
  /*#####################################################################*/

  class Eval :
    public StatementVisitor<Value*>,
    public ExpressionVisitor<Value*>,
    public CssVisitor<void> {

  public:

    // A pointer to the slot where we will assign to
    // Used to optimize self-assignments in functions
    ValueObj* assigne = nullptr;

    // Base references
    Logger& logger;
    Compiler& compiler;

    /////////////////////////////////////////////////////////////////////////
    // Options related to scoped css and selector production
    /////////////////////////////////////////////////////////////////////////

    // The current parent node in the output CSS tree.
    CssParentNodeObj current = nullptr;

    // The name of the current declaration parent. Used for BEM-
    // declaration blocks as in `div { prefix: { suffix: val; } }`;
    sass::string declarationName;

    // The current media queries, if any.
    CssMediaQueryVectorObj mediaQueries;

    // The selectors from style rules
    // ToDo: document why having two
    SelectorLists originalStack;
    SelectorLists selectorStack;

    /////////////////////////////////////////////////////////////////////////
    // Options related to scoped evaluation
    /////////////////////////////////////////////////////////////////////////

    // Whether we're currently executing a function.
    bool inFunction = false;

    // Whether we're currently building the output of an unknown at rule.
    bool inUnknownAtRule = false;

    // Whether we're directly within an `@at-root` rule excluding style rules.
    bool atRootExcludingStyleRule = false;

    CssStyleRule* _styleRule() const {
      return atRootExcludingStyleRule ? nullptr : readStyleRule;
    }

    // Whether we're currently building the output of a `@keyframes` rule.
    bool inKeyframes = false;

    // Whether we're currently evaluating a [SupportsDeclaration].
    // When this is true, calculations will not be simplified.
    bool inSupportsDeclaration = false;

    /////////////////////////////////////////////////////////////////////////
    // Parts below are related to handling modules and imports
    /////////////////////////////////////////////////////////////////////////

    // Alias into context
    Stylesheet* _stylesheet = nullptr;

    bool isStillInTopHeaders() const {
      const CssParentNode* compiled = _stylesheet->compiled;
      return compiled && _endOfImports == compiled->size();
    }

    ExtensionStore* _extensionStore = nullptr;

    // Alias into context
    WithConfig*& wconfig;


    bool viaImport = false;

  public:



    // The extend handler
    // ExtensionStore* extender2 = nullptr;

  public:

    void exposeUseRule(UseRule* rule);
    void exposeFwdRule(ForwardRule* rule) const;
    void exposeImpRule1(IncludeImport* rule) const;

    Value* doDivision(Value* left, Value* right, BinaryOpExpression* node, Logger& logger, SourceSpan pstate) const;

    inline Number* withoutSlash4(Number* value);
    inline Value* withoutSlash3(Value* value);

    size_t _indexAfterImports(sass::vector<CssNodeObj> statements);


    void _visitUpstreamModule(Stylesheet* upstream, sass::vector<Stylesheet*>& sorted, std::set<sass::string>& seen, CssRoot* css, sass::vector<CssNodeObj>& imports, bool clone);

    CssRoot* _combineCss(Stylesheet* module, bool clone = false);
    sass::vector<Stylesheet*> _topologicalModules(Stylesheet* root, CssRoot* css, sass::vector<CssNodeObj>& imports, bool clone);
    void _extendModules(sass::vector<Stylesheet*> sortedModules);


    EnvRefs* pudding(EnvRefs* idxs, bool intoRoot, EnvRefs* modFrame);



    // sass::map

    // The style rule that defines the current parent selector, if any.
    CssStyleRule* readStyleRule = nullptr;

    // Current content block
    UserDefinedCallable* content = nullptr;

    // Whether we're working with plain css.
    bool plainCss = false;

    // Whether we're currently executing a mixing.
    bool inMixin = false;

    // Whether we're currently executing an import.
    // dart has an _importSpan (check if related)
    bool inImport = false; // not in dart



    size_t _endOfImports = 0;

    sass::vector<CssImportObj> _outOfOrderImports;

    void compileModule(Stylesheet* module);
    void visitStylesheet(Stylesheet* module);
    void exposeModule(Stylesheet* module);

    sass::vector<WithConfigVar> toWithConfig(Map* withMap);

    void importCssModule(String* url, Map* config, SourceSpan pstate);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // ToDo: maybe create on demand for better pstate?
    // ToDo: do some benchmarks to check implications!
    BooleanObj bool_true;
    BooleanObj bool_false;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  public:

    // Value constructor
    Eval(Compiler& compiler,
      Logger& logger,
      bool isCss = false);

    bool _operandAllowsSlash(const Expression* node) const;

    // Query if we are in a mixin
    bool isInMixin() const {
      return inMixin;
    }

    // Query if use plain css
    bool isPlainCss() const {
      return plainCss;
    }

    // Query if we have a content block
    bool hasContentBlock() const {
      return content != nullptr;
    }

    // Check if there are any unsatisfied extends (will throw)
//    bool checkForUnsatisfiedExtends(Extension& unsatisfied) const {
//      return extender.checkForUnsatisfiedExtends(unsatisfied);
//    }

    // Another entry point for the `call` sass-function
    Value* acceptFunctionExpression(FunctionExpression* expression) {
      return visitFunctionExpression(expression);
    }

    // Converts the expression to css representation
    sass::string toCss(Expression* expression, bool quote = true);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Call built-in function with no overloads
    Value* execute(
      BuiltInCallable* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate,
      bool global = false);

    // Call built-in function with overloads
    Value* execute(
      BuiltInCallables* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate,
      bool global = false);

    // Used for user functions and also by
    // mixin includes and content includes.
    Value* execute(
      UserDefinedCallable* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate,
      bool global = false);

    // Call external C-API function
    Value* execute(
      ExternalCallable* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate,
      bool global = false);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  private:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    SelectorListObj& selector() { return selectorStack.back(); }
    SelectorListObj& original() { return originalStack.back(); }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Return root of current child
    CssParentNode* getRoot() const
    {
      CssParentNode* parent = current;
      while (parent->parent()) {
        parent = parent->parent();
      }
      return parent;
    }

    // Check if we currently build
    // the output of a style rule.
    bool isInStyleRule() const {
      return readStyleRule != nullptr &&
        !atRootExcludingStyleRule;
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  private:

    // Fetch unevaluated positional argument (optionally by name)
    // Will error if argument is missing or available both ways
    // Note: only needed for lazy evaluation in if expressions
    Expression* getArgument(
      ExpressionVector& positional,
      const ExpressionFlatMap* named,
      size_t idx, const EnvKey& name);

    // Fetch evaluated positional argument (optionally by name)
    // Will error if argument is missing or available both ways
    // Named arguments are consumed and removed from the hash
    Value* getParameter(
      ArgumentResults& evaled,
      size_t idx, const Argument* arg);

    // Call built-in function with no overloads
    Value* _runBuiltInCallable(
      CallableArguments* arguments,
      BuiltInCallable* callable,
      const SourceSpan& pstate,
      bool global = false);

    // Call built-in function with overloads
    Value* _runBuiltInCallables(
      CallableArguments* arguments,
      BuiltInCallables* callable,
      const SourceSpan& pstate,
      bool global = false);

    // Helper for _runBuiltInCallable(s)
    Value* _callBuiltInCallable(
      ArgumentResults& evaluated,
      const SassFnPair& function,
      const SourceSpan& pstate,
      bool global = false);
    public:
    // Used for user functions and also by
    // mixin includes and content includes.
    Value* _runUserDefinedCallable(
      CallableArguments* evaled,
      UserDefinedCallable* callable,
      const SourceSpan& pstate);

    // Call external C-API function
    Value* _runExternalCallable(
      CallableArguments* arguments,
      ExternalCallable* callable,
      const SourceSpan& pstate);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
  public:
    ArgumentResults _evaluateArguments(CallableArguments* arguments);
    void _addRestValueMap(ArgumentResults& results, Map* map, const SourceSpan& nodeForSpan);
    void _addRestExpressionMap(CallableArguments* arguments, Map* map, const SourceSpan& pstate);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    sass::string acceptInterpolation(InterpolationObj interpolation, bool warnForColor, bool trim = false);
    SourceData* interpolationToSource(InterpolationObj interpolation, bool warnForColor, bool trim = false, bool ws = true);
    sass::string interpolationToCssString(InterpolationObj interpolation, bool warnForColor, bool trim = false);
    SelectorListObj interpolationToSelector(Interpolation* interpolation, bool plainCss, bool allowParent = true);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void _evaluateMacroArguments(
      CallableArguments* arguments,
      ExpressionVector& positional);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void acceptStaticCssImport(StaticImport* import);
    void acceptDynamicSassImport(IncludeImport * import);


    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Evaluate all children at the currently existing block context
    Value* acceptChildren(const Vectorized<Statement>& children);
    Value* acceptChildren(const Vectorized<CssNode>& children);

    // Evaluate all children at a newly established current block context
    Value* acceptChildrenAt(CssParentNode* parent, const Vectorized<Statement>& children);
    Value* acceptChildrenAt(CssParentNode* parent, const Vectorized<CssNode>& children);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    public:
    void renderArgumentInvocation(sass::string& strm, CallableArguments* args);

    Value* _visitCalculationExpression(Expression* node, bool inLegacySassFunction);

    void _checkCalculationArguments(Calc::CFN fn, FunctionExpression* node, size_t maxArgs);

    void _checkCalculationArguments(Calc::CFN fn, FunctionExpression* node);

    Value* applyMixin(
      const SourceSpan& pstate, const EnvKey& name,
      Callable* callable,
      CallableDeclaration* ctblk,
      CallableArguments* arguments);

    private:

    Value* visitCalcuation(Calc::CFN fn, FunctionExpression* node, bool inLegacySassFunction);


    protected:
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Value* visitBinaryOpExpression(BinaryOpExpression*) override;
    Value* visitBooleanExpression(BooleanExpression*) override;
    Value* visitColorExpression(ColorExpression*) override;
    Value* visitFunctionExpression(FunctionExpression*) override;
    void _checkAdjacentCalculationValues(const ValueVector& elements, const ListExpression* node);
    Value* visitIfExpression(IfExpression*) override;
    Value* visitListExpression(ListExpression*) override;
    Value* visitMapExpression(MapExpression*) override;
    Value* visitNullExpression(NullExpression*) override;
    Value* visitNumberExpression(NumberExpression*) override;
    Value* visitItplFnExpression(ItplFnExpression*) override;
    Value* visitParenthesizedExpression(ParenthesizedExpression*) override;
    Value* visitSelectorExpression(SelectorExpression*) override;
    Value* visitStringExpression(StringExpression*) override;
    Value* visitSupportsExpression(SupportsExpression*) override;
    Value* visitUnaryOpExpression(UnaryOpExpression*) override;
    Value* visitValueExpression(ValueExpression*) override;
    Value* visitVariableExpression(VariableExpression*) override;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Value* visitAtRootRule(AtRootRule* rule) override;
    Value* visitAtRule(AtRule* rule) override;
    bool BubbleMediaQuery(CssParentNode* node, CssMediaQueryVector& uses, bool chroot);
    Value* visitContentBlock(ContentBlock* rule) override;
    Value* visitContentRule(ContentRule* rule) override;
    Value* visitDebugRule(DebugRule* rule) override;
    Value* visitDeclaration(Declaration* rule) override;
    Value* visitEachRule(EachRule* rule) override;
    Value* visitErrorRule(ErrorRule* rule) override;
    Value* visitExtendRule(ExtendRule* rule) override;
    Value* visitForRule(ForRule* rule) override;
    Value* visitForwardRule(ForwardRule* rule) override;
    Value* visitFunctionRule(FunctionRule* rule) override;
    Value* visitIfRule(IfRule* rule) override;
    Value* visitImportRule(ImportRule* rule) override;
    Value* visitIncludeRule(IncludeRule* rule) override;
    Value* visitLoudComment(LoudComment* rule) override;
    Value* visitMediaRule(MediaRule* rule) override;
    Value* visitMixinRule(MixinRule* rule) override;
    Value* visitReturnRule(ReturnRule* rule) override;
    Value* visitSilentComment(SilentComment* rule) override;
    Value* visitStyleRule(StyleRule* rule) override;
    // visitStylesheet
    Value* visitSupportsRule(SupportsRule* rule) override;
    Value* visitUseRule(UseRule* rule) override;
    Value* visitAssignRule(AssignRule* rule) override;
    Value* visitWarnRule(WarnRule* rule) override;
    Value* visitWhileRule(WhileRule* rule) override;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void visitCssAtRule(CssAtRule* css) override;
    void visitCssComment(CssComment* css) override;
    void visitCssDeclaration(CssDeclaration* css) override;
    void visitCssImport(CssImport* css) override;
    void visitCssKeyframeBlock(CssKeyframeBlock* css) override;
    void visitCssMediaRule(CssMediaRule* css) override;
    void visitCssRoot(CssRoot* css) override;
    void visitCssStyleRule(CssStyleRule* css) override;
    void visitCssSupportsRule(CssSupportsRule* css) override;

  public:
    Stylesheet* resolveIncludeImport(IncludeImport* rule);


    // Backbone loader function
    // Use by load-css directly
    Stylesheet* loadModule(
      const sass::string& prev,
      const sass::string& url,
      bool isImport = false);

    // Loading of parsed rules
    Stylesheet* loadModRule(ModRule* rule);
    Stylesheet* loadModRule2(ModRule* rule);

    Stylesheet* _loadStylesheet(ModRule* rule);

    void _addChild(CssNode* node, bool(*through)(CssNode* node)) const;


  private:
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void callExternalMessageOverloadFunction(Callable* fn, Value* message);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    sass::string _visitSupportsCondition(SupportsCondition* condition);
    sass::string _parenthesize(SupportsCondition* condition);
    sass::string _parenthesize(SupportsCondition* condition, SupportsOperation::Operand operand);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    CssParentNode* _trimIncluded(CssParentVector& nodes) const;

    CssParentNode* hoistStyleRule(CssParentNode* node);
public:

    CssMediaQueryVector* mergeMediaQueries(
      CssMediaQueryVector* lhs,
      CssMediaQueryVector* rhs,
      CssMediaQueryVector& uses,
      bool& valid);












    CssMediaQueryVector* evalMediaQueries(Interpolation* itpl);



    //void _verifyCompatibleNumbers(sass::vector<AstNode*> args, const SourceSpan& pstate);

};

}

#endif
