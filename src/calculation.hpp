/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_CALCULATION_HPP
#define SASS_CALCULATION_HPP

#include "ast_fwd_decl.hpp"

#include "calc_names.hpp"

#include <cmath>

namespace Sass {

  namespace Calc {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // static Value* singleArgument(Logger& logger, const SourceSpan& pstate, const sass::string& name, AstNode* argument,
    //   Number* (*mathFunc)(Logger& logger, const SourceSpan& pstate, AstNode* argument, Number*), bool forbitUnits = false)
    // {
    //   AstNode* simplified = argument->simplify(logger);
    //   auto* number = dynamic_cast<Number*>(simplified);
    //   if (number == nullptr) return SASS_MEMORY_NEW(
    //     Calculation, argument->pstate(), name, { simplified });
    //   // if (forbitUnits) number->assertNoUnits();
    //   return mathFunc(logger, fn, argument, number);
    // }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Value* operate(Logger& logger, const SourceSpan& span,
      SassOperator op, AstNode* lhs, AstNode* rhs,
      bool inLegacySassFunction, bool simplify);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Execute calculation function with arguments
    Value* execute(Logger& logger, const SourceSpan& pstate,
      CFN fn, const ValueVector& args, bool global = false);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    /// Creates a `calc()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_calc(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates an `abs()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_abs(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `sqrt()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_sqrt(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `sign()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_sign(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates an `exp()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_exp(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `sin()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_sin(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `cos()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_cos(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `tan()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_tan(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates an `asin()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_asin(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates an `acos()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_acos(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates an `atan()` calculation with the given [argument].
    ///
    /// The [argument] must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_atan(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `log()` calculation with the given [number] and [base].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// If arguments contains exactly a single argument, the base is set to
    /// `math.e` by default.
    Value* calc_log(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `atan2()` calculation for [y] and [x].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// This may be passed fewer than two arguments, but only if one of the
    /// arguments is an unquoted `var()` string.
    Value* calc_atan2(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `mod()` calculation with the given [dividend] and [modulus].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// This may be passed fewer than two arguments, but only if one of the
    /// arguments is an unquoted `var()` string.
    Value* calc_mod(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `pow()` calculation with the given [base] and [exponent].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// This may be passed fewer than two arguments, but only if one of the
    /// arguments is an unquoted `var()` string.
    Value* calc_pow(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `rem()` calculation with the given [dividend] and [modulus].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// This may be passed fewer than two arguments, but only if one of the
    /// arguments is an unquoted `var()` string.
    Value* calc_rem(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `clamp()` calculation with the given [min], [value], and [max].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation].
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    ///
    /// This may be passed fewer than three arguments, but only if one of the
    /// arguments is an unquoted `var()` string.
    Value* calc_clamp(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /// Creates a `min()` calculation with the given [arguments].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation]. It must be passed at
    /// least one argument.
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_min(Logger& logger, const SourceSpan& pstate, const ValueVector& args, bool strict = false);

    /// Creates a `max()` calculation with the given [arguments].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation]. It must be passed at
    /// least one argument.
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_max(Logger& logger, const SourceSpan& pstate, const ValueVector& args, bool strict = false);

    /// Creates a `hypot()` calculation with the given [arguments].
    ///
    /// Each argument must be either a [SassNumber], a [SassCalculation], an
    /// unquoted [SassString], or a [CalculationOperation]. It must be passed at
    /// least one argument.
    ///
    /// This automatically simplifies the calculation, so it may return a
    /// [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    /// can determine that the calculation will definitely produce invalid CSS.
    Value* calc_hypot(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    // Creates a `round()` calculation with the given [strategyOrNumber],
    // [numberOrStep], and [step]. Strategy must be either nearest,
    // up, down or to-zero.
    //
    // Number and step must be either a [SassNumber], a [SassCalculation],
    // an unquoted [SassString], or a [CalculationOperation].
    //
    // This automatically simplifies the calculation, so it may return a
    // [SassNumber] rather than a [SassCalculation]. It throws an exception if it
    // can determine that the calculation will definitely produce invalid CSS.
    //
    // This may be passed fewer than two arguments, but only if one of the
    // arguments is an unquoted `var()` string.
    Value* calc_round(Logger& logger, const SourceSpan& pstate, const ValueVector& args);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
