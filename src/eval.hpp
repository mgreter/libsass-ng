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
      const SourceSpan& pstate);

    // Call built-in function with overloads
    Value* execute(
      BuiltInCallables* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate);

    // Used for user functions and also by
    // mixin includes and content includes.
    Value* execute(
      UserDefinedCallable* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate);

    // Call external C-API function
    Value* execute(
      ExternalCallable* callable,
      CallableArguments* arguments,
      const SourceSpan& pstate);

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
      const SourceSpan& pstate);

    // Call built-in function with overloads
    Value* _runBuiltInCallables(
      CallableArguments* arguments,
      BuiltInCallables* callable,
      const SourceSpan& pstate);

    // Helper for _runBuiltInCallable(s)
    Value* _callBuiltInCallable(
      ArgumentResults& evaluated,
      const SassFnPair& function,
      const SourceSpan& pstate);
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

    Value* visitBinaryOpExpression(BinaryOpExpression*);
    Value* visitBooleanExpression(BooleanExpression*);
    Value* visitColorExpression(ColorExpression*);
    Value* visitFunctionExpression(FunctionExpression*);
    void _checkAdjacentCalculationValues(const ValueVector& elements, const ListExpression* node);
    Value* visitIfExpression(IfExpression*);
    Value* visitListExpression(ListExpression*);
    Value* visitMapExpression(MapExpression*);
    Value* visitNullExpression(NullExpression*);
    Value* visitNumberExpression(NumberExpression*);
    Value* visitItplFnExpression(ItplFnExpression*);
    Value* visitParenthesizedExpression(ParenthesizedExpression*);
    Value* visitSelectorExpression(SelectorExpression*);
    Value* visitStringExpression(StringExpression*);
    Value* visitSupportsExpression(SupportsExpression*);
    Value* visitUnaryOpExpression(UnaryOpExpression*);
    Value* visitValueExpression(ValueExpression*);
    Value* visitVariableExpression(VariableExpression*);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Value* visitAtRootRule(AtRootRule* rule);
    Value* visitAtRule(AtRule* rule);
    bool BubbleMediaQuery(CssParentNode* node, CssMediaQueryVector& uses, bool chroot);
    Value* visitContentBlock(ContentBlock* rule);
    Value* visitContentRule(ContentRule* rule);
    Value* visitDebugRule(DebugRule* rule);
    Value* visitDeclaration(Declaration* rule);
    Value* visitEachRule(EachRule* rule);
    Value* visitErrorRule(ErrorRule* rule);
    Value* visitExtendRule(ExtendRule* rule);
    Value* visitForRule(ForRule* rule);
    Value* visitForwardRule(ForwardRule* rule);
    Value* visitFunctionRule(FunctionRule* rule);
    Value* visitIfRule(IfRule* rule);
    Value* visitImportRule(ImportRule* rule);
    Value* visitIncludeRule(IncludeRule* rule);
    Value* visitLoudComment(LoudComment* rule);
    Value* visitMediaRule(MediaRule* rule);
    Value* visitMixinRule(MixinRule* rule);
    Value* visitReturnRule(ReturnRule* rule);
    Value* visitSilentComment(SilentComment* rule);
    Value* visitStyleRule(StyleRule* rule);
    // visitStylesheet
    Value* visitSupportsRule(SupportsRule* rule);
    Value* visitUseRule(UseRule* rule);
    Value* visitAssignRule(AssignRule* rule);
    Value* visitWarnRule(WarnRule* rule);
    Value* visitWhileRule(WhileRule* rule);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void visitCssAtRule(CssAtRule* css);
    void visitCssComment(CssComment* css);
    void visitCssDeclaration(CssDeclaration* css);
    void visitCssImport(CssImport* css);
    void visitCssKeyframeBlock(CssKeyframeBlock* css);
    void visitCssMediaRule(CssMediaRule* css);
    void visitCssRoot(CssRoot* css);
    void visitCssStyleRule(CssStyleRule* css);
    void visitCssSupportsRule(CssSupportsRule* css);

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
