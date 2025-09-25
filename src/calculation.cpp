/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "calculation.hpp"

#include "exceptions.hpp"
#include "eval.hpp"

namespace Sass {

  namespace Calc {

    /////////////////////////////////////////////////////////////////////////
    // Helpers for calculation value verification
    /////////////////////////////////////////////////////////////////////////

    static void verifyCompatibleCalcValue(Logger& logger, const Number* nr)
    {
      if (nr && nr->isValidCssUnit() == false) {
        throw Exception::IncompatibleCalcValue(
          logger, *nr, nr->pstate());
      }
    }
    // EO verifyCompatibleCalcValue

    // Verifies that all the numbers in [args] aren't known to be
    // incompatible with one another, and that they don't have units
    // that are too complex for calculations.
    static void verifyCompatibleNumberValues(Logger& logger,
      const SourceSpan& pstate, const ValueVector& args, bool strict = true)
    {
      // Note: this logic is largely duplicated in
      // _EvaluateVisitor._verifyCompatibleNumbers and
      // most changes here should also be reflected there.
      for (const ValueObj& arg : args) {
        verifyCompatibleCalcValue(
          logger, arg->isaNumber());
      }
      // Now check all values against each other
      for (unsigned int i = 0; i < args.size() - 1; i++) {
        if (const Number* lnr = args[i]->isaNumber()) {
          for (unsigned int j = i + 1; j < args.size(); j++) {
            if (const Number* rnr = args[j]->isaNumber()) {
              if (!lnr->hasPossiblyCompatibleUnits(rnr, strict)) {
                throw Exception::UnitMismatch(logger, *lnr, *rnr);
              }
            }
          }
        }
      }
    }
    // EO verifyCompatibleNumbers

    // Verifies that all the numbers in [args] aren't known to be
    // incompatible with one another, and that they don't have units
    // that are too complex for calculations.
    // Optimized version for the case of exactly two variables
    static void verifyCompatibleNumbers(Logger& logger,
      const SourceSpan& pstate, const Number* lhs, const Number* rhs)
    {
      // Note: this logic is largely duplicated in
      // _EvaluateVisitor._verifyCompatibleNumbers and 
      // most changes here should also be reflected there.
      verifyCompatibleCalcValue(logger, lhs);
      verifyCompatibleCalcValue(logger, rhs);
      if (lhs == nullptr || rhs == nullptr) return;
      if (!lhs->hasPossiblyCompatibleUnits(rhs, true)) {
        throw Exception::UnitMismatch(logger, *lhs, *rhs);
      }
    }
    // EO verifyCompatibleNumbers

    /////////////////////////////////////////////////////////////////////////
    // Helper to execute internal operation
    /////////////////////////////////////////////////////////////////////////

    Value* operate(
      Logger& logger, const SourceSpan& pstate,
      SassOperator op, AstNode* left, AstNode* right,
      bool inLegacySassFunction, bool simplify)
    {

      if (!simplify) {
        // Simple case when not simplifying
        return SASS_MEMORY_NEW(CalcOperation,
          pstate, op, left, right);
      }

      // Simplify both side of the operation
      AstNodeObj lhs = left->simplify(logger);
      AstNodeObj rhs = right->simplify(logger);
      // Check if any of the simplifieds are numbers
      Number* lnr = lhs ? lhs->isaNumber() : nullptr;
      Number* rnr = rhs ? rhs->isaNumber() : nullptr;

      // Additions and subtractions
      if (op == ADD || op == SUB) {
        // Case when both sides are numbers
        if (lnr != nullptr && rnr != nullptr) {
          // Is the operation valid
          if (inLegacySassFunction ?
            lnr->canCompareTo(rnr, false) :
            lnr->hasCompatibleUnits(rnr, true))
          {
            return op == ADD
              ? lnr->plus(rnr, logger, pstate)
              : lnr->minus(rnr, logger, pstate);
          }
        }
        // Make sure numbers are compatible for operation
        verifyCompatibleNumbers(logger, pstate, lnr, rnr);
        // Implement unary simplification
        if (rnr && rnr->value() < 0) {
          rnr->value(rnr->value() * -1);
          op = op == ADD ? SUB : ADD;
        }
        // Return a wrapped calc operation
        return SASS_MEMORY_NEW(CalcOperation,
          pstate, op, lhs, rhs);
      }
      // Valid for multiplication or division?
      else if (lnr != nullptr && rnr != nullptr) {
        return op == MUL
          ? lnr->times(rnr, logger, pstate)
          : lnr->dividedBy(rnr, logger, pstate);
      }

      // Return a wrapped calc operation
      return SASS_MEMORY_NEW(CalcOperation,
        pstate, op, lhs, rhs);
    }
    // EO operate

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `calc()` calculation with the given [argument].
    Value* calc_calc(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_calc);
      AstNodeObj simplified = args[0]->simplify(logger);
      if (Number* number = SASS_CAST(Number, simplified)) {
        return NumberObj::detach(number); // Code smell
      }
      if (Calculation* calc = SASS_CAST(Calculation, simplified)) {
        return CalculationObj::detach(calc); // Code smell
      }
      // Return a wrapped calculation
      return SASS_MEMORY_NEW(Calculation,
        pstate, str_calc, { simplified });
    }
    // EO calc_calc

    // Creates an `abs()` calculation with the given [argument].
    Value* calc_abs(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      if (const String* str = simplified->isaString()) {
        if (str->isVar()) return SASS_MEMORY_NEW(
          Calculation, pstate, str_abs, { simplified });
      }
      const Number* number = simplified->isaNumber();
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_abs, { simplified });
      if (number->hasUnit("%")) {
        logger.addDeprecation(number->pstate(),
          Logger::WARN_ABS_PERCENT, [number]() {
            return "Passing percentage units to the global abs() function is deprecated.\n"
              "In the future, this will emit a CSS abs() function to be resolved by the browser.\n"
              "To preserve current behavior: math.abs(" + number->inspect() + ")\n"
              "To emit a CSS abs() now: abs(#{" + number->inspect() + "})\n"
              "More info: https://sass-lang.com/d/abs-percent";
          });
      }
      return number->copyWithNewValue(std::abs(number->value()));
    }
    // EO calc_abs

    // Creates a `sqrt()` calculation with the given [argument].
    Value* calc_sqrt(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = simplified->isaNumber();
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_sqrt, { simplified });
      // No support for units square root
      number->assertUnitless(logger, str_number);
      return number->copyWithNewValue(std::sqrt(number->value()));
    }
    // EO calc_sqrt

    // Creates a `sign()` calculation with the given [argument].
    Value* calc_sign(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = simplified->isaNumber();
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_sign, { simplified });
      return number->copyWithNewValue(number->sign());
    }
    // EO calc_sign

    // Creates an `exp()` calculation with the given [argument].
    Value* calc_exp(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = simplified->isaNumber();
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_exp, { simplified });
      number->assertUnitless(logger, str_number);
      return number->copyWithNewValue(number->isNaN()
        ? number->value() : std::exp(number->value()));
    }
    // EO calc_exp

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `sin()` calculation with the given [argument].
    Value* calc_sin(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_angle);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_sin, { simplified });
      double factor = number->factorToUnits(unit_rad);
      if (factor == 0.0) throw Exception::NoAngleArgument(logger, number, str_angle);
      return SASS_MEMORY_NEW(Number, number->pstate(),
        std::sin(number->value() * factor));
    }
    // EO calc_sin

    // Creates a `cos()` calculation with the given [argument].
    Value* calc_cos(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_angle);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_cos, { simplified });
      double factor = number->factorToUnits(unit_rad);
      if (factor == 0.0) throw Exception::NoAngleArgument(logger, number, str_angle);
      return SASS_MEMORY_NEW(Number, number->pstate(),
        std::cos(number->value() * factor));
    }
    // EO calc_cos

    // Creates a `tan()` calculation with the given [argument].
    Value* calc_tan(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_angle);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_tan, { simplified });
      double factor = number->factorToUnits(unit_rad);
      if (factor == 0.0) throw Exception::NoAngleArgument(logger, number, str_angle);
      return SASS_MEMORY_NEW(Number, number->pstate(),
        std::tan(number->value() * factor));
    }
    // EO calc_tan

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates an `asin()` calculation with the given [argument].
    Value* calc_asin(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_asin, { simplified });
      number->assertNoUnits(logger, str_number);
      double degs = std::asin(number->value()) * Constants::Math::RAD_TO_DEG;
      return SASS_MEMORY_NEW(Number, number->pstate(), degs, unit_deg);
    }
    //EO calc_asin

    // Creates an `acos()` calculation with the given [argument].
    Value* calc_acos(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_acos, { simplified });
      number->assertNoUnits(logger, str_number);
      double degs = std::acos(number->value()) * Constants::Math::RAD_TO_DEG;
      return SASS_MEMORY_NEW(Number, number->pstate(), degs, unit_deg);
    }
    //EO calc_acos

    // Creates an `atan()` calculation with the given [argument].
    Value* calc_atan(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 1) throw Exception::TooManyArguments(logger, args.size(), 1);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_number);
      AstNodeObj simplified = args[0]->simplify(logger);
      const Number* number = SASS_CAST(Number, simplified);
      if (number == nullptr) return SASS_MEMORY_NEW(
        Calculation, pstate, str_atan, { simplified });
      number->assertNoUnits(logger, str_number);
      double degs = std::atan(number->value()) * Constants::Math::RAD_TO_DEG;
      return SASS_MEMORY_NEW(Number, number->pstate(), degs, unit_deg);
    }
    //EO calc_atan

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `log()` calculation with the given [number] and [base].
    Value* calc_log(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 2) throw Exception::TooManyArguments(logger, args.size(), 2);
      else if (args.size() < 1) throw Exception::MissingArgument(logger, str_log);
      AstNodeObj arg_nr = args[0] ? args[0]->simplify(logger) : nullptr;
      AstNodeObj arg_base = args.size() > 1 ? args[1]->simplify(logger) : nullptr;
      if (const Number* nr_value = SASS_CAST(Number, arg_nr)) {
        nr_value->assertNoUnits(logger, str_number);
        if (arg_base == nullptr) {
          return SASS_MEMORY_NEW(Number, pstate,
            std::log(nr_value->value()));
        }
        if (const Number* nr_base = SASS_CAST(Number, arg_base)) {
          nr_base->assertNoUnits(logger, str_base);
          return SASS_MEMORY_NEW(Number, pstate,
            std::log(nr_value->value()) /
              std::log(nr_base->value()));
        }
      }
      // Return wrapped calculation
      return SASS_MEMORY_NEW(Calculation,
        pstate, str_log, { arg_nr, arg_base });
    }
    // EO calc_log


    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `atan2()` calculation for [y] and [x].
    Value* calc_atan2(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 2) throw Exception::TooManyArguments(logger, args.size(), 2);
      else if (args.size() < 2) throw Exception::TooFewArguments(logger, args.size(), 2);
      AstNodeObj arg_y = args[0]->simplify(logger);
      AstNodeObj arg_x = args[1]->simplify(logger);
      if (NumberObj nr_y = arg_y->isaNumber()) {
        if (NumberObj nr_x = arg_x->isaNumber()) {
          if (unit_percent != *nr_y || unit_percent != *nr_x) {
            verifyCompatibleNumbers(logger, pstate, nr_y, nr_x);
            double factor = nr_x->getUnitConversionFactor(nr_y, true);
            if (factor != 0 && nr_y->hasCompatibleUnits(nr_x)) {
              return SASS_MEMORY_NEW(Number, pstate,
                std::atan2(nr_y->value(), nr_x->value() * factor)
                  * Constants::Math::RAD_TO_DEG, unit_deg);
            }
          }
        }
      }
      // Return wrapped calculation
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_atan2, { arg_y, arg_x });
    }
    // EO calc_atan2

    // Creates a `mod()` calculation with the given [dividend] and [modulus].
    Value* calc_mod(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 2) throw Exception::TooManyArguments(logger, args.size(), 2);
      else if (args.size() < 2) throw Exception::TooFewArguments(logger, args.size(), 2);
      AstNodeObj arg_div = args[0]->simplify(logger);
      AstNodeObj arg_mod = args[1]->simplify(logger);
      if (NumberObj nr_div = arg_div->isaNumber()) {
        if (NumberObj nr_mod = arg_mod->isaNumber()) {
          verifyCompatibleNumbers(logger, pstate, nr_div, nr_mod);
          double factor = nr_div->getUnitConversionFactor(nr_mod, true);
          if (factor != 0.0) return nr_div->modulo(nr_mod, logger, pstate);
          if (nr_div->isCustomUnit() || nr_mod->isCustomUnit()) {
            return SASS_MEMORY_NEW(Calculation, pstate,
              str_mod, { arg_div, arg_mod });
          }
          throw Exception::UnitMismatch(logger, nr_div, nr_mod);
        }
      }
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_mod, { arg_div, arg_mod });
    }
    // EO calc_mod

    // Creates a `pow()` calculation with the given [base] and [exponent].
    Value* calc_pow(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 2) throw Exception::TooManyArguments(logger, args.size(), 2);
      else if (args.size() < 2) throw Exception::TooFewArguments(logger, args.size(), 2);
      AstNodeObj arg_base = args[0]->simplify(logger);
      AstNodeObj arg_exp = args[1]->simplify(logger);
      if (Number* nr_base = arg_base->isaNumber()) {
        if (Number* nr_exp = arg_exp->isaNumber()) {
          nr_base->assertNoUnits(logger, str_base);
          nr_exp->assertNoUnits(logger, str_exp);
          return SASS_MEMORY_NEW(Number, pstate,
            std::pow(nr_base->value(), nr_exp->value()));
        }
      }
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_pow, { arg_base, arg_exp });
    }
    // EO calc_pow

    // Creates a `rem()` calculation with the given [dividend] and [modulus].
    Value* calc_rem(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.size() > 2) throw Exception::TooManyArguments(logger, args.size(), 2);
      else if (args.size() < 2) throw Exception::TooFewArguments(logger, args.size(), 2);
      AstNodeObj arg_div = args[0]->simplify(logger);
      AstNodeObj arg_mod = args[1]->simplify(logger);
      if (NumberObj nr_div = arg_div->isaNumber()) {
        if (NumberObj nr_mod = arg_mod->isaNumber()) {
          verifyCompatibleNumbers(logger, pstate, nr_div, nr_mod);
          double factor = nr_div->getUnitConversionFactor(nr_mod, true);
          if (factor == 0.0) {
            if (nr_div->isCustomUnit() || nr_mod->isCustomUnit()) {
              return SASS_MEMORY_NEW(Calculation, pstate,
                str_rem, { arg_div, arg_mod });
            }
            throw Exception::UnitMismatch(logger, nr_div, nr_mod);
          }
          ValueObj rv = nr_div->modulo(nr_mod, logger, pstate);
          const Number* result = rv->isaNumber();
          double div = nr_div->value(), mod = nr_mod->value();
          if (std::signbit(div) == std::signbit(mod)) return rv.detach();
          if (std::isinf(mod)) return nr_div.detach();
          if (result->value() == 0.0) return result->unaryMinus(logger, pstate);
          return result->minus(nr_mod, logger, pstate);
        }
      }
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_rem, { arg_div, arg_mod });
    }
    // EO calc_rem

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `min()` calculation with the given [arguments].
    Value* calc_min(Logger& logger, const SourceSpan& pstate, const ValueVector& args, bool strict)
    {
      if (args.empty()) throw Exception::MustHaveArguments(logger, str_min);
      // Convert the whole input array to simplified items
      sass::vector<AstNodeObj> simplifieds(args.size());
      std::transform(args.begin(), args.end(),
        simplifieds.begin(), [&](ValueObj value) {
          return value->simplify(logger);
        });
      // find min number now
      NumberObj min = nullptr;
      for (size_t i = 0; i < args.size(); i++) {
        Value* val = SASS_CAST(Value, simplifieds[i]);
        if (val->isaCalculation() || val->isaCalcOperation()) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation,
            pstate, str_min, std::move(simplifieds));
        }
        if (auto str = val->isaString()) {
          if (str->isVar()) {
            verifyCompatibleNumberValues(logger, pstate, args);
            return SASS_MEMORY_NEW(Calculation,
              pstate, str_min, std::move(simplifieds));
          }
        }
        Number* nr = strict ? val->assertNumber(logger, str_empty) : val->isaNumber();
        if (nr == nullptr) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation,
            pstate, str_min, std::move(simplifieds));
        }
        if (min == nullptr) { min = nr; continue; }
        // Will return zero if the units are to compatible
        double factor = nr->getUnitConversionFactor(min, false);
        if (factor == 0.0) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation,
            pstate, str_min, std::move(simplifieds));
        }
        if (min->value() > nr->value() * factor) min = nr;
      }
      // Return min number
      return min.detach();
    }
    // EO calc_min

    // Creates a `max()` calculation with the given [arguments].
    Value* calc_max(Logger& logger, const SourceSpan& pstate, const ValueVector& args, bool strict)
    {
      if (args.empty()) throw Exception::MustHaveArguments(logger, str_max);
      // Convert the whole input array to simplified items
      sass::vector<AstNodeObj> simplifieds(args.size());
      std::transform(args.begin(), args.end(),
        simplifieds.begin(), [&](ValueObj value) {
          return value->simplify(logger);
        });
      // find max number now
      NumberObj max = nullptr;
      for (size_t i = 0; i < simplifieds.size(); i++) {
        Value* val = SASS_CAST(Value, simplifieds[i]);
        if (val->isaCalculation() || val->isaCalcOperation()) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation, pstate,
            str_max, std::move(simplifieds));
        }
        if (auto str = val->isaString()) {
          if (str->isVar()) {
            verifyCompatibleNumberValues(logger, pstate, args);
            return SASS_MEMORY_NEW(Calculation, pstate,
              str_max, std::move(simplifieds));
          }
        }
        Number* nr = strict ? val->assertNumber(logger, str_empty) : val->isaNumber();
        if (nr == nullptr) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation, pstate,
            str_max, std::move(simplifieds));
        }
        if (max == nullptr) { max = nr; continue; }
        // Will return zero if the units are to compatible
        double factor = nr->getUnitConversionFactor(max, false);
        if (factor == 0.0) {
          verifyCompatibleNumberValues(logger, pstate, args);
          return SASS_MEMORY_NEW(Calculation, pstate,
            str_max, std::move(simplifieds));
        }
        if (max->value() < nr->value() * factor) max = nr;
      }
      // Return max number
      return max.detach();
    }
    // EO calc_max

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `hypot()` calculation with the given [arguments].
    Value* calc_hypot(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      if (args.empty()) throw Exception::MustHaveArguments(logger, str_hypot);
      // Convert the whole input array to simplified items
      sass::vector<AstNodeObj> simplifieds(args.size());
      std::transform(args.begin(), args.end(),
        simplifieds.begin(), [&](ValueObj value) {
          return value->simplify(logger);
        });
      // Verify early on that values are compatible
      verifyCompatibleNumberValues(logger, pstate, args);
      const Number* first = simplifieds[0]->isaNumber();
      if (first == nullptr || first->hasUnit("%")) {
        return SASS_MEMORY_NEW(Calculation, pstate,
          str_hypot, std::move(simplifieds));
      }
      // Otherwise sum up the squares of all values
      double subtotal = first->value() * first->value();
      // One arg is mandatory, rest is optional here
      for (size_t i = 1; i < simplifieds.size(); i++) {
        const Number* next = args[i]->isaNumber();
        if (next == nullptr) return SASS_MEMORY_NEW(Calculation,
          pstate, str_hypot, std::move(simplifieds));
        double factor = next->getUnitConversionFactor(first);
        if (factor == 0.0) {
          if (first->isCustomUnit() || next->isCustomUnit()) {
            return SASS_MEMORY_NEW(Calculation, pstate,
              str_hypot, std::move(simplifieds));
          }
          throw Exception::UnitMismatch(logger, *first, *next);
        }
        double value = next->value() * factor;
        subtotal += value * value; // square it
      }
      // Return the result in units of the first number
      return SASS_MEMORY_NEW(Number, pstate,
        std::sqrt(subtotal), first);
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    static Value* calc_clamp_1(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj simplified = args[0]->simplify(logger);
      // verifyCompatibleCalcValue(logger, simplified);
      if (simplified->isaString()) return SASS_MEMORY_NEW(
        Calculation, pstate, str_clamp, { simplified });
      throw Exception::TooFewArguments(logger, 1, 3);
    }
    // EO calc_clamp_1

    static Value* calc_clamp_2(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj arg_min = args[0]->simplify(logger);
      AstNodeObj arg_val = args[1]->simplify(logger);
      NumberObj nr_min = arg_min->isaNumber();
      NumberObj nr_val = arg_val->isaNumber();
      verifyCompatibleNumbers(logger, pstate, nr_min, nr_val);
      if (arg_min->isaString() || arg_val->isaString())
        return SASS_MEMORY_NEW(Calculation, pstate,
          str_clamp, { arg_min, arg_val });
      throw Exception::TooFewArguments(logger, 2, 3);
    }
    // EO calc_clamp_2

    static Value* calc_clamp_3(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj arg_min = args[0]->simplify(logger);
      AstNodeObj arg_val = args[1]->simplify(logger);
      AstNodeObj arg_max = args[2]->simplify(logger);
      NumberObj nr_min = arg_min->isaNumber();
      NumberObj nr_val = arg_val->isaNumber();
      NumberObj nr_max = arg_max->isaNumber();
      if (nr_min && nr_val && nr_max) {
        if (nr_min->hasCompatibleUnits(nr_val) && nr_max->hasCompatibleUnits(nr_val)) {
          if (nr_val->lessThanOrEquals(nr_min, logger, pstate)) return nr_min.detach();
          if (nr_val->greaterThanOrEquals(nr_max, logger, pstate)) return nr_max.detach();
          else return nr_val.detach();
        }
      }
      verifyCompatibleNumbers(logger, pstate, nr_min, nr_val);
      verifyCompatibleNumbers(logger, pstate, nr_min, nr_max);
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_clamp, { arg_min, arg_val, arg_max });
    }
    // EO calc_clamp_3

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `clamp()` calculation with the given [min], [value], and [max].
    Value* calc_clamp(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      switch (args.size()) {
      case 0: throw Exception::MissingArgument(logger, str_number);
      case 1: return calc_clamp_1(logger, pstate, args);
      case 2: return calc_clamp_2(logger, pstate, args);
      case 3: return calc_clamp_3(logger, pstate, args);
      default: throw Exception::TooManyArguments(logger, args.size(), 3);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    static Value* calc_round_1(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj arg_0 = args[0]->simplify(logger);
      if (auto nr_number = SASS_CAST(Number, arg_0)) {
        return nr_number->copyWithNewValue(
          std::round(nr_number->value()));
      }
      if (auto str_number = SASS_CAST(String, arg_0)) {
        return SASS_MEMORY_NEW(Calculation, pstate, str_round, { str_number });
      }
      CallStackFrame frame(logger, arg_0->pstate());
      throw Exception::SassScriptException("Single argument " +
        arg_0->toString() + " expected to be simplifiable.",
        logger, arg_0->pstate()
      );
    }
    // EO calc_round_1

    static Value* calc_round_2(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj arg_0 = args[0]->simplify(logger);
      AstNodeObj arg_1 = args[1]->simplify(logger);
      if (const Number* nr_number = SASS_CAST(Number, arg_0)) {
        if (const Number* nr_step = SASS_CAST(Number, arg_1)) {
          verifyCompatibleNumbers(logger, pstate, nr_number, nr_step);
          if (nr_number->hasCompatibleUnits(nr_step, true)) {
            return nr_number->copyWithNewValue(
              nr_number->roundWithStep(nr_step));
          }
          else {
            return new Calculation(pstate,
              str_round, { arg_0, arg_1 });
          }
        }
      }
      if (const String* strat = SASS_CAST(String, arg_0))
      {
        Round::RNDSTRAT strategy = Round::Parse(strat->value());
        if (strategy != Round::RNDSTRAT::OTHER)
        {
          if (const String* str_number = SASS_CAST(String, arg_1)) {
            if (str_number->isVar()) {
              return SASS_MEMORY_NEW(Calculation, pstate,
                str_round, { arg_0, arg_1 });
            }
          }
          throw Exception::SassScriptException(logger, pstate,
            "If strategy is not null, step is required.");
        }
      }
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_round, { arg_0, arg_1 });
    }
    // EO calc_round_2

    static Value* calc_round_3(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      // Only called when array length is verified 
      AstNodeObj arg_0 = args[0]->simplify(logger);
      AstNodeObj arg_1 = args[1]->simplify(logger);
      AstNodeObj arg_2 = args[2]->simplify(logger);
      if (String* strategy = SASS_CAST(String, arg_0))
      {
        // ToDo: should this be case insensitive or sensitive?
        Round::RNDSTRAT strat = Round::Parse(strategy->value());
        if (strat != Round::RNDSTRAT::OTHER)
        {
          // Try to cast arguments to numbers
          const Number* nr_number = SASS_CAST(Number, arg_1);
          const Number* nr_step = SASS_CAST(Number, arg_2);
          // Check if both sides ar valid numbers
          if (nr_number != nullptr && nr_step != nullptr)
          {
            // Check for compatibility to round to
            if (nr_number->hasCompatibleUnits(nr_step)) {
              return nr_number->copyWithNewValue(
                nr_number->roundWithStep(nr_step, strat));
            }
            else {
              // Return as wrapped calc value
              return new Calculation(pstate,
                str_round, { arg_0, arg_1, arg_2 });
            }
          }
          // Check if anything is not a number or string
          else if (nr_number == nullptr ||
            nr_step == nullptr ||
            SASS_CAST(String, arg_1) ||
            SASS_CAST(String, arg_2))
          {
            // Return as wrapped calc value
            return new Calculation(pstate,
              str_round, { arg_0, arg_1, arg_2 });
          }
          else {
            throw Exception::SassScriptException(logger, pstate,
              "If strategy is not null, step is required.");
          }
        }
        else if (strategy->isVar()) {
          return new Calculation(pstate,
            str_round, { arg_0, arg_1, arg_2 });
        }
        else if (arg_0->isaString()) {
          return new Calculation(pstate,
            str_round, { arg_0, arg_1, arg_2 });
        }
        else {
          CallStackFrame frame(logger, strategy->pstate());
          throw Exception::SassScriptException(strategy->toString() +
            " must be either nearest, up, down or to-zero.",
            logger, strategy->pstate());
        }
      }
      else if (arg_0 != nullptr) {
        CallStackFrame frame(logger, arg_0->pstate());
        throw Exception::SassScriptException(arg_0->toString() +
          " must be either nearest, up, down or to-zero.",
          logger, args[0]->pstate());
      }
      else {
        // Shouldn't happen, but play safe
        throw Exception::MissingArgument(
          logger, str_number);
      }
    }
    // EO calc_round_3

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Creates a `round()` calculation with the given [strategyOrNumber],
    // [numberOrStep], and [step]. Strategy must be nearest, up, down or to-zero.
    Value* calc_round(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      switch (args.size()) {
      case 0: throw Exception::MissingArgument(logger, str_number);
      case 1: return calc_round_1(logger, pstate, args);
      case 2: return calc_round_2(logger, pstate, args);
      case 3: return calc_round_3(logger, pstate, args);
      default: throw Exception::TooManyArguments(logger, args.size(), 3);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    static Value* calc_size_2(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      sass::vector<AstNodeObj> simplifieds(args.size());
      std::transform(args.begin(), args.end(),
        simplifieds.begin(), [&](ValueObj value) {
          return value ? value->simplify(logger) : nullptr;
        });
      return SASS_MEMORY_NEW(Calculation, pstate,
        str_hypot, std::move(simplifieds));
    }

    // Creates an `calc-size()` calculation with the given [basis] and [value].
    // The [basis] and [value] must be either a [SassNumber], a [SassCalculation],
    // an unquoted [SassString], or a [CalculationOperation].
    // This automatically simplifies the calculation. It throws an exception if
    // it can determine that the calculation will definitely produce invalid CSS.
    Value* calc_size(Logger& logger, const SourceSpan& pstate, const ValueVector& args)
    {
      switch (args.size()) {
      case 0: throw Exception::MissingArgument(logger, str_number);
      case 1: throw Exception::TooFewArguments(logger, args.size(), 3);
      case 2: return calc_size_2(logger, pstate, args);
      default: throw Exception::TooManyArguments(logger, args.size(), 3);
      }
        //    static SassCalculation calcSize(Object basis, Object ? value) {
      // var args = [basis, if (value != null) value];
      // _verifyLength(args, 2);
      // basis = _simplify(basis);
      // value = value.andThen(_simplify);
      // return SassCalculation._("calc-size", [basis, if (value != null) value]);
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Value* execute(Logger& logger, const SourceSpan& pstate,
      CFN fn, const ValueVector& args, bool global)
    {
      switch (fn) {
      case CFN::SQRT: return calc_sqrt(logger, pstate, args);
      case CFN::ABS: return calc_abs(logger, pstate, args);
      case CFN::EXP: return calc_exp(logger, pstate, args);
      case CFN::SIGN: return calc_sign(logger, pstate, args);
      case CFN::SIN: return calc_sin(logger, pstate, args);
      case CFN::COS: return calc_cos(logger, pstate, args);
      case CFN::TAN: return calc_tan(logger, pstate, args);
      case CFN::ASIN: return calc_asin(logger, pstate, args);
      case CFN::ACOS: return calc_acos(logger, pstate, args);
      case CFN::ATAN: return calc_atan(logger, pstate, args);
      case CFN::MIN: return calc_min(logger, pstate, args);
      case CFN::MAX: return calc_max(logger, pstate, args);
      case CFN::POW: return calc_pow(logger, pstate, args);
      case CFN::MOD: return calc_mod(logger, pstate, args);
      case CFN::REM: return calc_rem(logger, pstate, args);
      case CFN::CLAMP: return calc_clamp(logger, pstate, args);
      case CFN::HYPOT: return calc_hypot(logger, pstate, args);
      case CFN::ATAN2: return calc_atan2(logger, pstate, args);
      case CFN::LOG: return calc_log(logger, pstate, args);
      case CFN::ROUND: return calc_round(logger, pstate, args);
      case CFN::CALC: return calc_calc(logger, pstate, args);
      case CFN::SIZE: return calc_size(logger, pstate, args);
      default: throw Exception::RuntimeException(logger, "Bad calc CFN");
      }
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  } // EO Calculation

} // EO Sass
