/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_AST_VALUES_HPP
#define SASS_AST_VALUES_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "units.hpp"
#include "ast_nodes.hpp"
#include "ast_callables.hpp"
#include "calc_names.hpp"

namespace Sass {

  class Compiler;

  /////////////////////////////////////////////////////////////////////////
  // Errors from Sass_Values.
  /////////////////////////////////////////////////////////////////////////

  class CustomError final : public Value
  {
  private:

    ADD_CONSTREF(sass::string, message)

  public:

    // Value constructor
    CustomError(
      const SourceSpan& pstate,
      const sass::string& message);

    // Copy constructor
    CustomError(const CustomError* ptr);

    // Implement interface for base Value class
    size_t hash() const final { return 0; }
    SassValueType getTag() const final { return SASS_ERROR; }
    const sass::string& type() const final { return Strings::error; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const CustomError& rhs) const;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final;
    Value* accept(ValueVisitor<Value*>* visitor) final;

    // Copy operations for childless items
    CustomError* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CustomError, this);
    }

    IMPLEMENT_ISA_CASTER(CustomError);
  };

  /////////////////////////////////////////////////////////////////////////
  // Warnings from Sass_Values.
  /////////////////////////////////////////////////////////////////////////

  class CustomWarning final : public Value
  {
  private:

    ADD_CONSTREF(sass::string, message)

  public:

    // Value constructor
    CustomWarning(
      const SourceSpan& pstate,
      const sass::string& message);

    // Copy constructor
    CustomWarning(const CustomWarning* ptr);

    // Implement interface for base Value class
    size_t hash() const final { return 0; }
    SassValueType getTag() const final { return SASS_WARNING; }
    const sass::string& type() const final { return Strings::warning; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const CustomWarning& rhs) const;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final;
    Value* accept(ValueVisitor<Value*>* visitor) final;

    // Copy operations for childless items
    CustomWarning* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CustomWarning, this);
    }

  };

  ///////////////////////////////////////////////////////////////////////
  // The null value.
  ///////////////////////////////////////////////////////////////////////

  class Null final : public Value
  {
  public:

    // Value constructor
    Null(const SourceSpan& pstate);

    // Copy constructor
    Null(const Null* ptr);

    // Implement simple checkers for base value class
    bool isNull() const final { return true; }
    bool isBlank() const final { return true; }
    bool isTruthy() const final { return false; }

    // Implement interface for base Value class
    size_t hash() const final;

    SassValueType getTag() const final { return SASS_NULL; }
    const sass::string& type() const final { return Strings::null; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitNull(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitNull(this);
    }

    // Copy operations for childless items
    Null* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Null, this);
    }

    IMPLEMENT_ISA_CASTER(Null);
  };

  ///////////////////////////////////////////////////////////////////////
  // Base class for colors (either rgba or hsla).
  ///////////////////////////////////////////////////////////////////////


  ///////////////////////////////////////////////////////////////////////
  // A sass number with optional units
  ///////////////////////////////////////////////////////////////////////

  class Number final : public Value, public Units, public CalcItem
  {
  private:

    ADD_CONSTREF(double, value);
    // The representation of this number as two
    // slash-separated numbers, if it has one.
    ADD_CONSTREF(NumberObj, lhsAsSlash);
    ADD_CONSTREF(NumberObj, rhsAsSlash);

  public:

    SassSeparator separator() const final {
      return lhsAsSlash_ && rhsAsSlash_ ? SASS_DIV : SASS_UNDEF;
    }

    // Value constructor
    Number(
      const SourceSpan& pstate,
      double value = 0.0,
      const sass::string& units = "");

    // Value constructor
    Number(
      const SourceSpan& pstate,
      double value, Units units);

    // Copy constructor
    Number(const Number* ptr,
      bool childless = false);

		// Numbers can't be simplified further
		AstNode* simplify(Logger& logger) final { return this; }

    // Check if we have delayed value info
    inline bool hasAsSlash() const {
      return !lhsAsSlash_.isNull()
        && !rhsAsSlash_.isNull();
    }

    // Check if number matches [unit]
    inline bool hasUnit(const sass::string& unit) const {
      return numerators.size() == 1 &&
        denominators.empty() &&
        numerators.front() == unit;
    }

    // Copy this number object and assign a new value to it
    inline Number* copyWithNewValue(double value) const
    {
      return SASS_MEMORY_NEW(Number, pstate_, value, this);
    }

    // Copy this number object and assign new units to it
    inline Number* copyWithNewUnits(const Units& units) const
    {
      return SASS_MEMORY_NEW(Number, pstate_, value_, units);
    }

    // Round with strategy and step
    double roundWithStep(const Number* step,
      Round::RNDSTRAT strategy = Round::RNDSTRAT::NEAREST) const;

    inline double sign() const {
      if (value_ == 0.0) return value_;
      if (std::isnan(value_)) return value_;
      return std::signbit(value_) ? -1 : 1;
    }

    bool isNaN() const
    {
      return std::isnan(value_);
    }

    bool isInf() const
    {
      return std::isfinite(value_);
    }

    // cancel out unnecessary units
    // result will be in input units
    double reduce() override
    {
      // apply conversion factor
      return value_ *= this->Units::reduce();
    }

    // normalize units to defaults
    // needed to compare two numbers
    double normalize() override
    {
      // apply conversion factor
      return value_ *= this->Units::normalize();
    }

    Number* coerce(Logger& logger, Number& rhs);
    double coerceToUnit(Logger& logger, const Units& units, const sass::string& vname) const;
    double factorToUnits(const Units& units) const;

    // Implement delayed value fetcher
    Value* withoutSlash() final;

    Number* withoutSlash5();

    sass::string recommendation() const;

    // Implement interface for base Value class
    size_t hash() const final;
    SassValueType getTag() const final { return SASS_NUMBER; }
    const sass::string& type() const final { return Strings::number; }

    // Implement some comparators for base value class
    bool greaterThan(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    bool greaterThanOrEquals(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    bool lessThan(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    bool lessThanOrEquals(const Value* other, Logger& logger, const SourceSpan& pstate) const final;

    // Implement some operations for base value class (some of them may return a string)
    Value* plus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* minus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* dividedBy(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Number* times(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Number* modulo(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Number* remainder(const Value* other, Logger& logger, const SourceSpan& pstate) const final;

    // Implement unary operations for base value class
    Number* unaryPlus(Logger& logger, const SourceSpan& pstate) const final;
    Number* unaryMinus(Logger& logger, const SourceSpan& pstate) const final;

    // Implement type fetcher for base value class (throws in base implementation)
    Number* assertNumber(Logger& logger, const sass::string& name = Strings::empty) final { return this; }

    // Assert that this number has no unit and throws if any unit is present
    Number* assertNumberStrictWithoutUnit(Logger& logger, const sass::string& name = Strings::empty);

    // Implement number specific assertions
    long assertInt(Logger& logger, const sass::string& name = Strings::empty) const;

    // Asserts that [number] is a percentage or has no units, and normalizes the
    // value. If [number] has no units, its value is clamped to be greater than `0`
    // or less than [max] and returned. If [number] is a percentage, it's scaled to
    // be within `0` and [max]. Otherwise, this throws a [SassScriptException].
    // [name] is used to identify the argument in the error message.
    double assertPercentageOrUnitless(Logger& logger, double max, const sass::string& name) const;

    // Sin
    const Value* assertColorChannel(Logger& logger, const sass::string& channel, const sass::string& name = Strings::empty) const final { return this; }

    const Number* assertNumberStrictWithoutUnit(Logger& logger, const sass::string& name = Strings::empty) const;
		Number* assertHasUnits(Logger& logger, const sass::string& unit, const sass::string& name = Strings::empty);
    void assertNoUnits(Logger& logger, const sass::string& name = Strings::empty) const;
    double assertRange(Logger& logger, double min, double max, const Units& units, const sass::string& name = Strings::empty) const;

    double valueInRange(Logger& logger, double min, double max, const sass::string& name) const;
    double valueInRangeWithUnit(Logger& logger, double min, double max, const sass::string& name, const Units& units) const;

    const Number* checkPercent(Logger& logger, const sass::string& name) const;

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const Number& rhs) const;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitNumber(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitNumber(this);
    }

    // Copy operations for childless items
    Number* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Number, this, true);
    }

  private:

    Number* operate(double (*op)(double, double), const Number& rhs, Logger& logger, const SourceSpan& pstate) const;

    IMPLEMENT_ISA_CASTER(Number);
    FINALIZE_AST_NODE(Number);
  };

  ///////////////////////////////////////////////////////////////////////
  // A sass boolean (either true or false)
  ///////////////////////////////////////////////////////////////////////

  class Boolean final : public Value
  {
  private:

    ADD_CONSTREF(bool, value)

  public:

    // Value constructor
    Boolean(
      const SourceSpan& pstate,
      bool value = false);

    // Copy constructor
    Boolean(const Boolean* ptr);

    // Implement simple checkers for base value class
    bool isTruthy() const final { return value_; }

    // Implement interface for base Value class
    size_t hash() const final;
    SassValueType getTag() const final { return SASS_BOOLEAN; }
    const sass::string& type() const final { return Strings::boolean; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const Boolean& rhs) const;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitBoolean(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitBoolean(this);
    }

    // Copy operations for childless items
    Boolean* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Boolean, this);
    }

    IMPLEMENT_ISA_CASTER(Boolean);
    FINALIZE_AST_NODE(Boolean);
  };

  ///////////////////////////////////////////////////////////////////////
  // A sass string (optionally quoted on rendering)
  ///////////////////////////////////////////////////////////////////////
  class String final : public Value, public CalcItem
  {
  private:

    ADD_CONSTREF(sass::string, value);
    ADD_CONSTREF(bool, hasQuotes);

  public:

    // Value constructor
    String(
      const SourceSpan& pstate,
      const char* value,
      bool hasQuotes = false);

    String(
      const SourceSpan& pstate,
      sass::string&& value,
      bool hasQuotes = false);

    String(
      const SourceSpan& pstate,
      const sass::string& value,
      bool hasQuotes = false);

    // Copy constructor
    String(const String* ptr);

		AstNode* simplify(Logger& logger) final;

    // Check if value would render empty
    bool isBlank() const final {
      if (hasQuotes_) return false;
      return value_.empty();
    }

    bool isVar() const;

    // Implement interface for base Value class
    size_t hash() const final;
    SassValueType getTag() const final { return SASS_STRING; }
    const sass::string& type() const override { return Strings::string; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const String& rhs) const;

    bool isSpecialNumber(bool withNoneKwd = false) const final;

    // Implement type fetcher for base value class (throws in base implementation)
    String* assertString(Logger& logger, const sass::string& name = Strings::empty) final { return this; }

    const Value* assertColorChannel(Logger& logger, const sass::string& channel, const sass::string& name = Strings::empty) const final;

    // Implement type fetcher for base value class (throws in base implementation)
    const String* assertQuoted(Logger& logger, const sass::string& name = Strings::empty) const;
    const String* assertUnquoted(Logger& logger, const sass::string& name = Strings::empty) const;


    // Implement some operations for base value class
    Value* plus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;

    // Main entry point for Value Visitor pattern
    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitString(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitString(this);
    }

    // Copy operations for childless items
    String* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(String, this);
    }

    IMPLEMENT_ISA_CASTER(String);
    FINALIZE_AST_NODE(String);
  };

  ///////////////////////////////////////////////////////////////////////
  // A sass map (which keeps the insertion order)
  ///////////////////////////////////////////////////////////////////////
  class Map final : public Value, public Hashed<ValueObj, ValueObj>
  {
  private:

    // Helper for getPairAsList to avoid memory leaks
    // Returned by `Values::iterator::operator*()`
    ListObj itpair;

  public:

    // Value constructor
    Map(
      const SourceSpan& pstate,
      Hashed::ordered_map_type&& move = {});

    // Copy constructor
    Map(const Map* ptr);

    // Return the list separator
    SassSeparator separator() const final {
      return empty() ? SASS_UNDEF : SASS_COMMA;
    }

    // Return the length of this item as a list
    size_t lengthAsList() const override {
      return size();
    }

    // Search the position of the given value
    size_t indexOf(Value* value) final;

    // Return list with two items (key and value)
    Value* getPairAsList(size_t idx);

    // Only used for nth sass function
    // Doesn't allow overflow of index (throw error)
    // Allows negative index but no overflow either
    Value* getValueAt(Value* index, Logger& logger) final;

    // Implement interface for base Value class
    size_t hash() const final;
    SassValueType getTag() const final { return SASS_MAP; }
    const sass::string& type() const final { return Strings::map; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const Map& rhs) const;

    // Implement type fetcher for base value class (throws in base implementation)
    Map* assertMap(Logger& logger, const sass::string& name) override { return this; }

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitMap(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitMap(this);
    }
    // Copy operations for childless items
    Map* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Map, this);
    }

  protected:

    // Clone all items in-place
    Map* cloneChildren(SASS_MEMORY_ARGS_VOID) final {
      for (auto it = Hashed::begin(); it != Hashed::end(); ++it) {
        it.value() = it.value()->copy(SASS_MEMORY_PARAMS_VOID);
        it.value()->cloneChildren(SASS_MEMORY_PARAMS_VOID);
      }
      return this;
    }

    IMPLEMENT_ISA_CASTER(Map);
    FINALIZE_AST_NODE(Map);
  };

  ///////////////////////////////////////////////////////////////////////
  // Lists of values, both comma- and space-separated (distinguished by a
  // type-tag.) Also used to represent variable-length argument lists.
  ///////////////////////////////////////////////////////////////////////

  class List : public Value, public Vectorized<Value>
  {
  private:

    SassSeparator separator_;
    ADD_CONSTREF(bool, hasBrackets);

  public:

    // Value constructor
    List(const SourceSpan& pstate,
      const ValueVector& values = {},
      SassSeparator separator = SASS_SPACE,
      bool hasBrackets = false);

    // Value constructor
    List(const SourceSpan& pstate,
      ValueVector&& values,
      SassSeparator separator = SASS_SPACE,
      bool hasBrackets = false);

    // Copy constructor
    List(const List* ptr);

    // Return the list separator
    SassSeparator separator() const final {
      return separator_;
    }

    // Set the list separator
    void separator(SassSeparator separator) {
      separator_ = separator;
    }

    ValueVector asList() final;

    // Return the length of this item as a list
    size_t lengthAsList() const final {
      return size();
    }

    // Check if list has surrounding brackets
    bool hasBrackets() final {
      return hasBrackets_;
    }

    // Check if value would render empty
    bool isBlank() const final {
      if (hasBrackets_) return false;
      for (const Value* value : elements()) {
        if (!value->isBlank()) return false;
      }
      return true;
    }

    // Search the position of the given value
    size_t indexOf(Value* value) final;

    // Only used for nth sass function
    // Allows negative index but no overflow either
    // Doesn't allow overflow of index (throw error)
    Value* getValueAt(Value* index, Logger& logger) final;

    // Implement interface for base Value class
    size_t hash() const override;
    SassValueType getTag() const final { return SASS_LIST; }
    const sass::string& type() const override { return Strings::list; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const override;
    // Implement same class compare operator
    virtual bool operator==(const List& rhs) const;

    // Implement type fetcher for base value class (throws in base implementation)
    Map* assertMap(Logger& logger, const sass::string& name) final;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitList(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitList(this);
    }
    // Copy operations for childless items
    List* copy(SASS_MEMORY_ARGS bool childless) const override {
      return SASS_MEMORY_NEW_DBG(List, this);
    }

  protected:

    // Clone all items in-place
    List* cloneChildren(SASS_MEMORY_ARGS_VOID) override {
      for (ValueObj& entry : elements_) {
        entry = entry->copy(SASS_MEMORY_PARAMS_VOID);
        entry->cloneChildren(SASS_MEMORY_PARAMS_VOID);
      }
      return this;
    }

    IMPLEMENT_ISA_CASTER(List);
    FINALIZE_AST_NODE(List);
  };

  ///////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

  class ArgumentList final : public List
  {
  private:

    ValueFlatMapObj _keywords;

    mutable bool _wereKeywordsAccessed;

  public:

    // Value copy constructor
    ArgumentList(const SourceSpan& pstate,
      SassSeparator sep = SASS_SPACE,
      ValueVector&& values = {},
      ValueFlatMap* keywords = {});

    // Value move constructor
    ArgumentList(const SourceSpan& pstate,
      SassSeparator sep = SASS_SPACE,
      const ValueVector& values = {},
      ValueFlatMap* keywords = {});

    // Copy constructor
    ArgumentList(const ArgumentList* ptr);
    
    ValueFlatMap* keywords() {
      _wereKeywordsAccessed = true;
      return _keywords;
    }

    bool wereKeywordsAccessed() const {
      return _wereKeywordsAccessed;
    }

    bool hasAllKeywordsConsumed() const {
      return !_keywords || _keywords->empty() ||
        _wereKeywordsAccessed;
    }

    Map* keywordsAsSassMap() const;

    // Implement interface for base Value class
    size_t hash() const final;
    const sass::string& type() const final { return Strings::arglist; }

    ArgumentList* assertArgumentList(Logger& logger, const sass::string& name = Strings::empty) final {
      return this;
    }


    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const ArgumentList& rhs) const;

    // Copy operations for childless items
    ArgumentList* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(ArgumentList, this);
    }

  protected:

    // Clone all items in-place
    ArgumentList* cloneChildren(SASS_MEMORY_ARGS_VOID) final {
      if (_keywords) for (std::pair<EnvKey, ValueObj> it : *_keywords) {
        it.second = it.second->copy(SASS_MEMORY_PARAMS_VOID);
        it.second->cloneChildren(SASS_MEMORY_PARAMS_VOID);
      }
      return this;
    }

    IMPLEMENT_ISA_CASTER(ArgumentList);
    FINALIZE_AST_NODE(ArgumentList);
  };

  ///////////////////////////////////////////////////////////////////////
  // A sass function reference.
  ///////////////////////////////////////////////////////////////////////
  class Function final : public Value
  {
  private:

    ADD_CONSTREF(sass::string, cssName);
    ADD_CONSTREF(CallableObj, callable);

  public:

    // Value constructor
    Function(
      const SourceSpan& pstate,
      CallableObj callable);

    // Value constructor
    Function(
      const SourceSpan& pstate,
      const sass::string& cssName);

    // Copy constructor
    Function(const Function* ptr);

    // Implement interface for base Value class
    size_t hash() const final { return 0; }
    SassValueType getTag() const final { return SASS_FUNCTION; }
    const sass::string& type() const final { return Strings::function; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const Function& rhs) const;

    Function* assertFunction(Logger& logger, const sass::string& name = Strings::empty) final { return this; }

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitFunction(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitFunction(this);
    }
    // Copy operations for childless items
    Function* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Function, this);
    }

    IMPLEMENT_ISA_CASTER(Function);
    FINALIZE_AST_NODE(Function);
  };

  ///////////////////////////////////////////////////////////////////////
  // A calculation.
  ///////////////////////////////////////////////////////////////////////

  class Calculation final : public Value, public CalcItem
  {
  private:

    ADD_CONSTREF(sass::string, name)

    ADD_CONSTREF(sass::vector<AstNodeObj>, arguments)

  public:

    // Value constructor
    // Must move arguments
    Calculation(
      const SourceSpan& pstate,
      const sass::string& name,
      sass::vector<AstNodeObj>&& args);

    // Copy constructor (doesn't seem to be used)
    Calculation(const Calculation* ptr);

    // CalcOperation can't be simplified further
    AstNode* simplify(Logger& logger) final;

    // Implement simple checkers for base value class
    bool isNull() const final { return false; }
    bool isBlank() const final { return false; }
    bool isTruthy() const final { return true; }

    bool isSpecialNumber(bool withNoneKwd = false) const final { return true; }

    const Value* assertColorChannel(Logger& logger, const sass::string& channel, const sass::string& name = Strings::empty) const final { return this; }

    // Implement interface for base Value class
    size_t hash() const final;

    SassValueType getTag() const final { return SASS_CALCULATION; }
    const sass::string& type() const final { return Strings::calculation; }


    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;

    Value* plus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* minus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* unaryPlus(Logger& logger, const SourceSpan& pstate) const final;
    Value* unaryMinus(Logger& logger, const SourceSpan& pstate) const final;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitCalculation(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitCalculation(this);
    }

    // Copy operations for items with children
    Calculation* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Calculation, this);
    }

    Calculation* assertCalculation(Logger& logger, const sass::string& name = Strings::empty) final {
      return this;
    }

    IMPLEMENT_ISA_CASTER(Calculation);
    FINALIZE_AST_NODE(Calculation);
  };

  ///////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////


  class Mixin final : public Value
  {

  public:

    ADD_CONSTREF(CallableObj, callable);

  public:

    // Value constructor
    Mixin(
      const SourceSpan& pstate,
      Callable* callable);

    // Copy constructor
    Mixin(const Mixin* ptr);

    // CalcOperation can't be simplified further
    // AstNode* simplify(Logger& logger) final { return this; }

  // Assert and return a mixin value or throws if incompatible
    Mixin* assertMixin(Logger& logger, const sass::string& name = Strings::empty) final {
      return this;
    }

    // Implement simple checkers for base value class
    bool isNull() const final { return false; }
    bool isBlank() const final { return false; }
    bool isTruthy() const final { return true; }

    // Implement interface for base Value class
    size_t hash() const final;

    SassValueType getTag() const final { return SASS_MIXIN; }
    const sass::string& type() const final { return Strings::mixin; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    bool operator==(const Mixin& rhs) const;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitMixin(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitMixin(this);
    }

    // Copy operations for childless items
    Mixin* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Mixin, this);
    }

    IMPLEMENT_ISA_CASTER(Mixin);
    FINALIZE_AST_NODE(Mixin);
  };


  ///////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

  class CalcOperation : public Value, public CalcItem
  {

  public:

    ADD_CONSTREF(SassOperator, op);
    ADD_CONSTREF(AstNodeObj, left);
    ADD_CONSTREF(AstNodeObj, right);

  public:

    // Value constructor
    CalcOperation(
      const SourceSpan& pstate,
      const SassOperator op,
      AstNode* left,
      AstNode* right);

    // Copy constructor
    CalcOperation(const CalcOperation* ptr);

    // CalcOperation can't be simplified further
    AstNode* simplify(Logger& logger) final { return this; }

    // Implement simple checkers for base value class
    bool isNull() const final { return false; }
    bool isBlank() const final { return false; }
    bool isTruthy() const final { return true; }

    // Implement interface for base Value class
    size_t hash() const final;

    SassValueType getTag() const final { return SASS_CALC_OPERATION; }
    const sass::string& type() const final { return Strings::calcoperation; }

    // Implement equality comparators for base value class
    bool operator==(const Value& rhs) const final;

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitCalcOperation(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitCalcOperation(this);
    }

    // Copy operations for childless items
    CalcOperation* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(CalcOperation, this);
    }

    IMPLEMENT_ISA_CASTER(CalcOperation);
    FINALIZE_AST_NODE(CalcOperation);
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
