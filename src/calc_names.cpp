/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "calc_names.hpp"

#include "character.hpp"
#include "strings.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static bool equalsIgnoreCaseAsciiOnlyConst(const char a, const char b) {
    return a == (b | Character::asciiCaseBit);
  }

  static bool equalsIgnoreCaseAsciiSafeConst(const char a, const char b) {
    return a == Character::toLowerCase(b);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Calc::CFN Calc::Parse(const sass::string& name)
  {
    // Pre-optimize by knowing the size
    // Since we want to parse the full string
    switch (name.size()) {
    case 3:
      if (std::equal(str_abs.begin(), str_abs.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return ABS; }
      if (std::equal(str_cos.begin(), str_cos.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return COS; }
      if (std::equal(str_exp.begin(), str_exp.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return EXP; }
      if (std::equal(str_log.begin(), str_log.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return LOG; }
      if (std::equal(str_min.begin(), str_min.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return MIN; }
      if (std::equal(str_max.begin(), str_max.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return MAX; }
      if (std::equal(str_mod.begin(), str_mod.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return MOD; }
      if (std::equal(str_pow.begin(), str_pow.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return POW; }
      if (std::equal(str_rem.begin(), str_rem.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return REM; }
      if (std::equal(str_sin.begin(), str_sin.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return SIN; }
      if (std::equal(str_tan.begin(), str_tan.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return TAN; }
      break;
    case 4:
      if (std::equal(str_asin.begin(), str_asin.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return ASIN; }
      if (std::equal(str_acos.begin(), str_acos.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return ACOS; }
      if (std::equal(str_atan.begin(), str_atan.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return ATAN; }
      if (std::equal(str_calc.begin(), str_calc.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return CALC; }
      if (std::equal(str_sign.begin(), str_sign.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return SIGN; }
      if (std::equal(str_sqrt.begin(), str_sqrt.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return SQRT; }
      break;
    case 5:
      if (std::equal(str_clamp.begin(), str_clamp.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return CLAMP; }
      if (std::equal(str_hypot.begin(), str_hypot.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return HYPOT; }
      if (std::equal(str_round.begin(), str_round.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return ROUND; }
      if (std::equal(str_atan2.begin(), str_atan2.end(), name.begin(), equalsIgnoreCaseAsciiSafeConst)) { return ATAN2; }
      break;
    case 9:
      if (std::equal(str_calc_size.begin(), str_calc_size.end(), name.begin(), equalsIgnoreCaseAsciiSafeConst)) { return SIZE; }
      break;
    default:
      break;
    }
    return OTHER;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Round::RNDSTRAT Round::Parse(const sass::string& name)
  {
    // Pre-optimize by knowing the size
    // Since we want to parse the full string
    switch (name.size()) {
    case 2:
      if (std::equal(str_up.begin(), str_up.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return UP; }
      break;
    case 4:
      if (std::equal(str_down.begin(), str_down.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return DOWN; }
      break;
    case 7:
      if (std::equal(str_nearest.begin(), str_nearest.end(), name.begin(), equalsIgnoreCaseAsciiOnlyConst)) { return NEAREST; }
      if (std::equal(str_to_zero.begin(), str_to_zero.end(), name.begin(), equalsIgnoreCaseAsciiSafeConst)) { return TO_ZERO; }
      break;
    default:
      break;
    }
    return OTHER;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

