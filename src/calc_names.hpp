/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_CALC_NAMES_HPP
#define SASS_CALC_NAMES_HPP

#include "ast_fwd_decl.hpp"

#include "strings.hpp"
#include "logger.hpp"

namespace Sass {

  namespace Calc {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // We (prematurely?) optimize some conditions here
    // By abusing the value of enums to store some info

    // Five bits to make it actually unique
    // Two bits to store argument length
    // One bit if it has a css replacement

    enum CFN : uint8_t
    {
      OTHER = (0 << 3) + (0 << 2) + 0,
      SQRT  = (1 << 3) + (0 << 2) + 1,
      ABS   = (2 << 3) + (1 << 2) + 1,
      EXP   = (3 << 3) + (0 << 2) + 1,
      SIGN  = (4 << 3) + (0 << 2) + 1,
      SIN   = (5 << 3) + (0 << 2) + 1,
      COS   = (6 << 3) + (0 << 2) + 1,
      TAN   = (7 << 3) + (0 << 2) + 1,
      ASIN  = (8 << 3) + (0 << 2) + 1,
      ACOS  = (9 << 3) + (0 << 2) + 1,
      ATAN  = (10 << 3) + (0 << 2) + 1,
      MIN   = (11 << 3) + (1 << 2) + 0,
      MAX   = (12 << 3) + (1 << 2) + 0,
      POW   = (13 << 3) + (0 << 2) + 2,
      MOD   = (14 << 3) + (0 << 2) + 2,
      REM   = (15 << 3) + (0 << 2) + 2,
      CLAMP = (16 << 3) + (0 << 2) + 3,
      HYPOT = (17 << 3) + (0 << 2) + 0,
      ATAN2 = (18 << 3) + (0 << 2) + 2,
      LOG   = (19 << 3) + (0 << 2) + 2,
      ROUND = (20 << 3) + (1 << 2) + 3,
      CALC  = (21 << 3) + (0 << 2) + 1,
    };

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Parse name into a calc fn name (case insensitive)
    CFN Parse(const sass::string& name);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Arguments length is stored inside enum value
    inline size_t getArgumentsLength(CFN fn)
    {
      return fn & 3;
    }

    // Condition is stored in the 3rd bit
    inline bool hasCssReplacement(CFN fn)
    {
      return (fn & 4);
    }

    // Condition is stored in the 3rd bit
    inline bool hasCalculationVisitor(CFN fn)
    {
      return !((fn & 4) || fn == OTHER);
    }

    // Check is simple enough (not optimized)
    inline bool isSimpleTrigonometry(CFN fn)
    {
      return fn == SIN || fn == COS || fn == TAN;
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Helper to get back the function name
    // Will be all lowercase from given input
    inline const sass::string& ToString(CFN fn)
    {
      switch (fn) {
      case SQRT: return str_sqrt;
      case ABS: return str_abs;
      case EXP: return str_exp;
      case SIGN: return str_sign;
      case SIN: return str_sin;
      case COS: return str_cos;
      case TAN: return str_tan;
      case ASIN: return str_asin;
      case ACOS: return str_acos;
      case ATAN: return str_atan;
      case MIN: return str_min;
      case MAX: return str_max;
      case POW: return str_pow;
      case MOD: return str_mod;
      case REM: return str_rem;
      case CLAMP: return str_clamp;
      case HYPOT: return str_hypot;
      case ATAN2: return str_atan2;
      case LOG: return str_log;
      case ROUND: return str_round;
      case CALC: return str_calc;
      default: return str_empty;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  namespace Round {

    // Rounding strategy
    enum RNDSTRAT : unsigned char
    {
      UP,
      DOWN,
      NEAREST,
      TO_ZERO,
      OTHER
    };

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Parse name into a calc fn name (case insensitive)
    RNDSTRAT Parse(const sass::string& name);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Helper to get back the strategy name
    // Will be all lowercase from given input
    inline const sass::string& ToString(RNDSTRAT fn)
    {
      switch (fn) {
      case UP: return str_up;
      case DOWN: return str_down;
      case NEAREST: return str_nearest;
      case TO_ZERO: return str_to_zero;
      default: return str_empty;
      }
    }

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
