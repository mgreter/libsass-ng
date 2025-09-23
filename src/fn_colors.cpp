/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "fn_colors.hpp"

#include <iomanip>
#include "compiler.hpp"
#include "exceptions.hpp"
#include "ast_colors.hpp"
#include "strings.hpp"
#include "debugger.hpp"

#include "sources.hpp"
#include "parser_scss.hpp"

namespace Sass {

  namespace Functions {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Import string utility functions
    using namespace StringUtils;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Create typedef for color function callback
    typedef Value* (*colFn)(
      const sass::string& name,
      const ValueVector& arguments,
      const SourceSpan& pstate,
      Logger& logger,
      bool strict);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    static double coerceToDeg(const Number* number)
    {
      // Returns conversion factor `0` if not convertible
      if (double factor = number->getUnitConversionFactor(unit_deg)) {
        return number->value() * factor;
      }
      return number->value();
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Returns whether [value] is an unquoted string
    // that start with `var(` and contains `/`.
    static bool isVarSlash(Value* value)
    {
      if (value == nullptr) return false;
      const String* str = value->isaString();
      if (str == nullptr) return false;
      if (str->hasQuotes()) return false;
      return startsWith(str->value(), "var(", 4) &&
        str->value().find('/') != NPOS;
    }
    // EO isVarSlash

    // Returns whether [value] is an unquoted
    // string that start with `var(`.
    static bool isVar(const Value* value)
    {
      if (value == nullptr) return false;
      const String* str = value->isaString();
      if (str == nullptr) return false;
      if (str->hasQuotes()) return false;
      return startsWith(str->value(), "var(", 4);
    }
    // EO isVar

    // Returns whether [value] is an unquoted
    // string that start either with `calc(`,
    // "var(", "env(", "min(" or "max(".
    static bool isSpecialNumber(const Value* value)
    {
      if (value == nullptr) return false;
      if (/*const Calculation* calc = */value->isaCalculation()) {
        return true;
      }
      const String* str = value->isaString();
      if (str == nullptr) return false;
      if (str->hasQuotes()) return false;
      if (str->value().size() < 6) return false;
      return startsWith(str->value(), "calc(", 5)
        || startsWith(str->value(), "var(", 4)
        || startsWith(str->value(), "env(", 4)
        || startsWith(str->value(), "min(", 4)
        || startsWith(str->value(), "max(", 4)
        || startsWith(str->value(), "clamp(", 6);
    }
    // EO isSpecialNumber 

    // Implements regex check against /^[a-zA-Z]+\s*=/
    static bool isMsFilterStart(const sass::string& text)
    {
      auto it = text.begin();
      // The filter must start with alpha
      if (!Character::isAlphabetic(*it)) return false;
      while (it != text.end() && Character::isAlphabetic(*it)) ++it;
      while (it != text.end() && Character::isWhitespace(*it)) ++it;
      return it != text.end() && *it == '=';
    }
    // EO isMsFilterStart

/// Prints a deprecation warning if [hue] has a unit other than `deg`.
    static void checkAngle(Logger& logger, const Number* angle, const sass::string& name)
    {
      // if (!angle->hasUnits()) return;
      if (angle->hasCompatibleUnits(unit_deg, false)) return;
      logger.addDeprecation(angle->pstate(), Logger::WARN_ANGLE_CONVERT, [&]() {
        sass::string text = "$" + name + ": ";
        text += "Passing a unit other than deg (" + angle->inspect() + ") is deprecated.\n";
        text += "\nTo preserve current behavior: " + angle->unitSuggestion(name) + "\n";
        text += "\nSee https://sass-lang.com/d/color-units";
        return text;
        });
    }

    // Helper function for debugging
    // ToDo return EnvKey?
    static const sass::string& getColorArgName(
      size_t idx, const sass::string& name)
    {
      switch (idx) {
      case 0: return name[0] == Character::$h ? Strings::hue : Strings::red;
      case 1: return name[0] == Character::$h ? name[1] == Character::$s ? Strings::saturation : Strings::whiteness : Strings::green;
      case 2: return name[0] == Character::$h ? name[1] == Character::$s ? Strings::lightness : Strings::blackness : Strings::blue;
      default: throw std::runtime_error("Invalid input argument");
      }
    }
    // EO getColorArgName

    // Return value that will render as-is in css
    static String* getFunctionString(
      const sass::string& name,
      const SourceSpan& pstate,
      const ValueVector& arguments = {},
      SassSeparator separator = SASS_COMMA)
    {
      bool addComma = false;
      sass::sstream fncall;
      fncall << name << "(";
      sass::string sep(" ");
      if (separator == SASS_COMMA) sep = ", ";
      for (Value* arg : arguments) {
        if (addComma) fncall << sep;
        fncall << arg->toCss();
        addComma = true;
      }
      fncall << ")";
      return SASS_MEMORY_NEW(String,
        pstate, fncall.str());
    }
    // EO getFunctionString


    static bool _parseSlashChannels(
      const sass::string& name,
      Value* channels,
      ValueObj& components,
      ValueObj& alphaValue,
      const SourceSpan& pstate,
      Compiler& compiler)
    {
      // Get the list from the channels input variable (or throw)
      ValueVector list = channels->assertCommonListStyle(compiler, name, true);

      if (channels->hasSlashSeparator()) {

      }
      else {

      }

      return false;
    }

    static Value* parseChannels(
      const sass::string& name,
      Value* channels,
      SassColorSpace space,
      const sass::string& vname,
      const SourceSpan& pstate,
      Compiler& compiler)
    {

      // Check for css var
      if (isVar(channels)) {
        return SASS_MEMORY_NEW(
          String, pstate, name + "(" +
          channels->inspect() + ")");
      }

      auto originalChannels = channels;
      ValueObj alphaFromSlashList;
      if (channels->separator() == SASS_DIV) {

        // std::cerr << "List from slash\n";

        ListObj args = SASS_MEMORY_NEW(List, channels->pstate(),
          { channels->start(), channels->stop() });
        if (args->size() != 2) {
          sass::sstream message;
          message << "Only 2 slash-separated elements allowed, but ";
          message << channels->lengthAsList() << " ";
          message << pluralize("was", channels->lengthAsList(), "were");
          message << " passed.";
          throw Exception::SassScriptException(message.str(), compiler, pstate);
        }

        alphaFromSlashList = args->get(1);
        if (!isSpecialNumber(alphaFromSlashList)) {
          alphaFromSlashList->assertNumber(compiler, "alpha");
        }
        if (isVar(args->get(0))) {
          // std::cerr << "Doing shenanigans\n";
          return getFunctionString(name, pstate, { originalChannels });
          // return _functionString(name, [originalChannels]);
        }

        channels = args->get(0);

        // list = args;
      }

      // Check if argument is already a list
      ListObj list = channels->isaList();
      // If not create one and wrap value in it
      if (!list) {
        list = SASS_MEMORY_NEW(List,
          pstate, { channels->start(), channels->stop() });
      }

      // Check for invalid input arguments
      bool isBracketed = list->hasBrackets();
      bool isCommaSeparated = list->hasCommaSeparator();
      if (isCommaSeparated || isBracketed) {
        sass::sstream msg;
        msg << "$channels must be";
        if (isBracketed) msg << " an unbracketed";
        if (isCommaSeparated) {
          msg << (isBracketed ? "," : " a");
          msg << " space-separated";
        }
        msg << " list.";
        CallStackFrame csf(compiler, list->pstate());
        throw Exception::RuntimeException(compiler, msg.str());
      }

      // Check if we have a string as first argument
      if (list->size() > 0) {
        if (auto prefix = list->get(0)->isaString()) {
          if (prefix->hasQuotes() == false) {
            if (StringUtils::equalsIgnoreCase(prefix->value(), "from", 4)) {
              return new String(pstate, name + "(" + originalChannels->inspect() + ")");
            }
          }
        }
      }

      // Check if we have too many arguments
      if (list->size() > 3) {
        CallStackFrame csf(compiler, list->pstate());
        throw Exception::TooManyArguments(compiler, list->size(), 3);
      }
      // Check for not enough arguments
      if (list->size() < 3) {
        // Check if we have any css vars
        bool hasVar = false;
        for (Value* item : list->elements()) {
          if (isVar(item)) {
            hasVar = true;
            break;
          }
        }
        // Return function as-is back to be rendered as css
        if (hasVar || (!list->empty() && isVarSlash(list->last()))) {
          return getFunctionString(name, pstate, { originalChannels });
        }
        // Throw error for missing argument
        throw Exception::MissingArgument(compiler,
          getColorArgName(list->size(), name));
      }

      if (alphaFromSlashList) {
        ListObj copy = SASS_MEMORY_COPY(list);
        list->append(alphaFromSlashList);
        return list.detach();
      }


      // Check for the second argument
      Number* secondNumber = list->get(2)->isaNumber();
      String* secondString = list->get(2)->isaString();
      if (secondNumber && secondNumber->hasAsSlash()) {
        return SASS_MEMORY_NEW(List, pstate, {
          list->get(0), list->get(1),
          secondNumber->lhsAsSlash().ptr(),
          secondNumber->rhsAsSlash().ptr()
          });
      }
      if (secondString && !secondString->hasQuotes()
        && secondString->value().find('/') != NPOS) {
        return getFunctionString(name, pstate,
          list->elements(), list->separator());
      }
      // Return arguments
      return list.detach();

    }
    // EO parseChannels


    static Value* parseColorChannels(
      const sass::string& name,
      Value* channels,
      const SourceSpan& pstate,
      Compiler& compiler)
    {
      // Check for css var
      if (isVar(channels)) {
        return SASS_MEMORY_NEW(
          String, pstate, name + "(" +
          channels->inspect() + ")");
      }

      auto originalChannels = channels;
      ValueObj alphaFromSlashList;
      if (channels->separator() == SASS_DIV) {

        // std::cerr << "List from slash\n";

        ListObj args = SASS_MEMORY_NEW(List, channels->pstate(),
          { channels->start(), channels->stop() });
        if (args->size() != 2) {
          sass::sstream message;
          message << "Only 2 slash-separated elements allowed, but ";
          message << channels->lengthAsList() << " ";
          message << pluralize("was", channels->lengthAsList(), "were");
          message << " passed.";
          throw Exception::SassScriptException(message.str(), compiler, pstate);
        }

        alphaFromSlashList = args->get(1);
        if (!isSpecialNumber(alphaFromSlashList)) {
          alphaFromSlashList->assertNumber(compiler, "alpha");
        }
        if (isVar(args->get(0))) {
          // std::cerr << "Doing shenanigans\n";
          return getFunctionString(name, pstate, { originalChannels });
          // return _functionString(name, [originalChannels]);
        }

        channels = args->get(0);

        // list = args;
      }

      // Check if argument is already a list
      ListObj list = channels->isaList();
      // If not create one and wrap value in it
      if (list.isNull()) {
        list = SASS_MEMORY_NEW(List,
          pstate, { channels->start(), channels->stop() });
      }

      // Check for invalid input arguments
      bool isBracketed = list->hasBrackets();
      bool isCommaSeparated = list->hasCommaSeparator();
      if (isCommaSeparated || isBracketed) {
        sass::sstream msg;
        msg << "$channels must be";
        if (isBracketed) msg << " an unbracketed";
        if (isCommaSeparated) {
          msg << (isBracketed ? "," : " a");
          msg << " space-separated";
        }
        msg << " list.";
        CallStackFrame csf(compiler, list->pstate());
        throw Exception::RuntimeException(compiler, msg.str());
      }

      // Check if we have a string as first argument
      if (list->size() > 0) {
        if (auto prefix = list->get(0)->isaString()) {
          if (prefix->hasQuotes() == false) {
            if (StringUtils::equalsIgnoreCase(prefix->value(), "from", 4)) {
              return new String(pstate, name + "(" + originalChannels->inspect() + ")");
            }
          }
        }
      }

      // Check if we have too many arguments
      if (list->size() > 3) {
        CallStackFrame csf(compiler, list->pstate());
        throw Exception::TooManyArguments(compiler, list->size(), 3);
      }
      // Check for not enough arguments
      if (list->size() < 3) {
        // Check if we have any css vars
        bool hasVar = false;
        for (Value* item : list->elements()) {
          if (isVar(item)) {
            hasVar = true;
            break;
          }
        }
        // Return function as-is back to be rendered as css
        if (hasVar || (!list->empty() && isVarSlash(list->last()))) {
          return getFunctionString(name, pstate, { originalChannels });
        }
        // Throw error for missing argument
        throw Exception::MissingArgument(compiler,
          getColorArgName(list->size(), name));
      }

      if (alphaFromSlashList) {
        ListObj copy = SASS_MEMORY_COPY(list);
        list->append(alphaFromSlashList);
        return list.detach();
      }


      // Check for the second argument
      Number* secondNumber = list->get(2)->isaNumber();
      String* secondString = list->get(2)->isaString();
      if (secondNumber && secondNumber->hasAsSlash()) {
        return SASS_MEMORY_NEW(List, pstate, {
          list->get(0), list->get(1),
          secondNumber->lhsAsSlash().ptr(),
          secondNumber->rhsAsSlash().ptr()
        });
      }
      if (secondString && !secondString->hasQuotes()
        && secondString->value().find('/') != NPOS) {
        return getFunctionString(name, pstate,
          list->elements(), list->separator());
      }
      // Return arguments
      return list.detach();
    }
    // EO parseColorChannels

    // Handle one argument function invocation
    // Used by color functions rgb, hsl and hwb
    static Value* handleOneArgColorFn2(
      const sass::string& name,
      Value* argument,
      colFn function,
      Compiler& compiler,
      SassColorSpace space,
      const sass::string& vname,
      SourceSpan pstate,
      bool strict)
    {
      // Parse the color channel arguments
      ValueObj parsed = parseChannels(name, argument,
        space, vname, pstate, compiler);
      // Return if it is a string
      if (parsed->isaString()) {
        return parsed.detach();
      }
      // Execute function with list of arguments
      if (const List* list = parsed->isaList()) {
        return (*function)(name, list->elements(), pstate, compiler, strict);
      }
      // Otherwise return
      return argument;
      // Not sure if we must stringify
      // return SASS_MEMORY_NEW(String,
      //   pstate, argument->inspect());
    }
    // EO handleOneArgColorFn

    // Handle one argument function invocation
    // Used by color functions rgb, hsl and hwb
    static Value* handleOneArgColorFn(
      const sass::string& name,
      Value* argument,
      colFn function,
      Compiler& compiler,
      SourceSpan pstate,
      bool strict)
    {
      // Parse the color channel arguments
      ValueObj parsed = parseColorChannels(
        name, argument, pstate, compiler);
      // Return if it is a string
      if (parsed->isaString()) {
        return parsed.detach();
      }
      // Execute function with list of arguments
      if (const List* list = parsed->isaList()) {
        return (*function)(name, list->elements(), pstate, compiler, strict);
      }
      // Otherwise return
      return argument;
      // Not sure if we must stringify
      // return SASS_MEMORY_NEW(String,
      //   pstate, argument->inspect());
    }
    // EO handleOneArgColorFn

    /// Returns [color1] and [color2], mixed
    // together and weighted by [weight].
    static ColorRgba* mixColors(
      const Color* color1,
      const Color* color2,
      const Number* weight,
      const SourceSpan& pstate,
      Logger& logger)
    {
      ColorRgbaObj lhs(color1->toRGBA());
      ColorRgbaObj rhs(color2->toRGBA());
      // This algorithm factors in both the user-provided weight (w) and the
      // difference between the alpha values of the two colors (a) to decide how
      // to perform the weighted average of the two RGB values.
      // It works by first normalizing both parameters to be within [-1, 1], where
      // 1 indicates "only use color1", -1 indicates "only use color2", and all
      // values in between indicated a proportionately weighted average.
      // Once we have the normalized variables w and a, we apply the formula
      // (w + a)/(1 + w*a) to get the combined weight (in [-1, 1]) of color1. This
      // formula has two especially nice properties:
      //   * When either w or a are -1 or 1, the combined weight is also that
      //     number (cases where w * a == -1 are undefined, and handled as a
      //     special case).
      //   * When a is 0, the combined weight is w, and vice versa.
      // Finally, the weight of color1 is renormalized to be within [0, 1] and the
      // weight of color2 is given by 1 minus the weight of color1.
      double weightScale = weight->assertRange(
        0.0, 100.0, unit_percent, logger, "weight") / 100.0;
      double normalizedWeight = weightScale * 2.0 - 1.0;
      double alphaDistance = lhs->a() - color2->a();
      double combinedWeight1 = normalizedWeight * alphaDistance == -1
        ? normalizedWeight : (normalizedWeight + alphaDistance) /
        (1.0 + normalizedWeight * alphaDistance);
      double weight1 = (combinedWeight1 + 1.0) / 2.0;
      double weight2 = 1.0 - weight1;
      return SASS_MEMORY_NEW(ColorRgba, pstate,
        fuzzyRound(lhs->r() * weight1 + rhs->r() * weight2, logger.epsilon),
        fuzzyRound(lhs->g() * weight1 + rhs->g() * weight2, logger.epsilon),
        fuzzyRound(lhs->b() * weight1 + rhs->b() * weight2, logger.epsilon),
        lhs->a() * weightScale + rhs->a() * (1 - weightScale));
    }
    // EO mixColor

    static double scaleValue(
      double current,
      double scale,
      double max)
    {
      return current + (scale > 0.0 ? max - current : current) * scale;
    }

    // Asserts that [number] is a percentage or has no units, and normalizes the
    // value. If [number] has no units, its value is clamped to be greater than `0`
    // or less than [max] and returned. If [number] is a percentage, it's scaled to
    // be within `0` and [max]. Otherwise, this throws a [SassScriptException].
    // [name] is used to identify the argument in the error message.
    static double _percentageOrUnitless(
      const Number* number, double max,
      const sass::string& name,
      Logger& traces)
    {
      double value = 0.0;
      if (!number->hasUnits()) {
        value = number->value();
      }
      else if (number->hasUnit(Strings::percent)) {
        value = max * number->value() / 100;
      }
      else {
        CallStackFrame csf(traces, number->pstate());
        throw Exception::RuntimeException(traces,
          name + ": Expected " + number->inspect()
          + " to have no units or \"%\".");
      }
      // if (value < 0.0) return 0.0;
      // if (value > max) return max;
      return value;
    }

    static String* _functionRgbString(sass::string name, ColorRgba* color, Value* alpha, const SourceSpan& pstate)
    {
      sass::sstream fncall;
      fncall << name << "(";
      fncall << color->r() << ", ";
      fncall << color->g() << ", ";
      fncall << color->b() << ", ";
      fncall << alpha->inspect() << ")";
      return SASS_MEMORY_NEW(String,
        pstate, fncall.str());
    }

    static Value* handleTwoArgRgb(sass::string name, ValueVector arguments, const SourceSpan& pstate, Logger& logger, bool strict)
    {
      // Check if any `calc()` or `var()` are passed
      if (isVar(arguments[0])) {
        return getFunctionString(
          name, pstate, arguments);
      }
      else if (isVar(arguments[1])) {
        // if (const ColorSpaced* first = arguments[0]->isaColorSpaced()) {
        //   ColorRgbaObj rgba = first->toRGBA();
        //   return _functionRgbString(name,
        //     rgba, arguments[1], pstate);
        // }
        // else {
        //   return getFunctionString(
        //     name, pstate, arguments);
        // }
      }
      else if (!strict && isSpecialNumber(arguments[1])) {
        // if (const ColorSpaced* color = arguments[0]->assertColorSpaced(logger, Strings::color)) {
        //   ColorRgbaObj rgba = color->toRGBA();
        //   return _functionRgbString(name,
        //     rgba, arguments[1], pstate);
        // }
      }

      if (arguments[0]->isaColorSpaced()) {
        const ColorSpaced* color = arguments[0]->assertColorSpaced(logger, Strings::color);
        const Number* alpha = arguments[1]->assertNumber(logger, Strings::alpha);
        ColorSpacedObj copy = SASS_MEMORY_COPY(color);
        copy->alpha(_percentageOrUnitless(
          alpha, 1.0, "$alpha", logger));
        copy->parsed(false);
        return copy.detach();
      }

      const Color* color = arguments[0]->assertColor(logger, Strings::color);
      const Number* alpha = arguments[1]->assertNumber(logger, Strings::alpha);
      ColorObj copy = SASS_MEMORY_COPY(color);
      copy->a(_percentageOrUnitless(
        alpha, 1.0, "$alpha", logger));
      copy->parsed(false);
      return copy.detach();
    }


    tl::optional<double> _channelFromValue(
      Logger& logger,
      const ColorChannel& chnInfo,
      const Number* chnValue,
      bool clamp = true
    )
    {
      if (chnValue == nullptr) {
        return tl::optional<double>();
      }

      std::cerr << "channel " << chnInfo.name << " from value " << chnValue->value() << "\n";


      if (chnInfo.isLinear) {
        std::cerr << " Channel is linear\n";
        if (chnInfo.requiresPercent && !chnValue->hasUnit("%")) {
          std::cerr << "Must have unit of percent\n";
          return chnValue->value();
        }
        else if (chnInfo.lowerClamped == false && chnInfo.upperClamped == false) {
          return _percentageOrUnitless(chnValue, chnInfo.max, chnInfo.name, logger);
        }
        else if (clamp == false) {
          return _percentageOrUnitless(chnValue, chnInfo.max, chnInfo.name, logger);
        }
        else if (chnInfo.lowerClamped == true || chnInfo.upperClamped == true) {
          double val = _percentageOrUnitless(chnValue, chnInfo.max, chnInfo.name, logger);
          double min = chnInfo.lowerClamped ? chnInfo.min : -std::numeric_limits<double>::infinity();
          double max = chnInfo.upperClamped ? chnInfo.max : +std::numeric_limits<double>::infinity();
          return std::isnan(val) ? min : std::min(std::max(val, min), max);
        }
        else {
          return chnValue->value();
        }
      }
      else {
        // Coerce into degrees
        return absmod(coerceToDeg(chnValue), 360.0);
      }
    }

    ColorSpaced* _colorFromChannels(
      Logger& logger, const SourceSpan& pstate, const ColorSpace* space,
      Number* chn0, Number* chn1, Number* chn2,
      tl::optional<double> alpha,
      bool clamp = true, bool fromRgbFunction = false)
    {

      if (space == nullptr) {
        std::cerr << "space is nullptr";
        return SASS_MEMORY_NEW(ColorSpaced,
          pstate, ColorSpace::rgb,
          1, 1, 1, 1);
      }

      if (space == &ColorSpace::hsl) {

        auto a = _channelFromValue(logger, space->_channels[0], chn0, clamp);
        auto b = _channelFromValue(logger, space->_channels[1], chn1, clamp);
        auto c = _channelFromValue(logger, space->_channels[2], chn2, clamp);
        std::cerr << " RV " << a.value_or(0) << ", " << b.value_or(0) << ", " << c.value_or(0) << "\n";
        auto rv = SASS_MEMORY_NEW(ColorSpaced,
          pstate, *space,
          a,
          b,
          c,
          alpha);

        std::cerr << " => " << rv->getChannel0() << ", " <<
          rv->getChannel1() << ", " << rv->getChannel2() << "\n";

        return rv;

      }
      else if (space == &ColorSpace::hwb) {

        if (chn1 != nullptr && chn2 != nullptr) {
          chn1->assertHasUnits(logger, "%", str_whiteness);
          chn2->assertHasUnits(logger, "%", str_blackness);
          double whiteness = chn1->value();
          double blackness = chn2->value();
          if (whiteness + blackness > 100.0) {
            double oldWhiteness = whiteness;
            whiteness = whiteness / (whiteness + blackness) * 100.0;
            blackness = blackness / (oldWhiteness + blackness) * 100.0;
          }
          auto rv = SASS_MEMORY_NEW(ColorSpaced,
            pstate, *space,
            chn0->value(),
            whiteness,
            blackness,
            alpha);
          return rv;
        }

        auto a = _channelFromValue(logger, space->_channels[0], chn0, clamp);
        auto b = _channelFromValue(logger, space->_channels[1], chn1, clamp);
        auto c = _channelFromValue(logger, space->_channels[2], chn2, clamp);
        std::cerr << " RV " << a.value_or(0) << ", " << b.value_or(0) << ", " << c.value_or(0) << "\n";
        auto rv = SASS_MEMORY_NEW(ColorSpaced,
          pstate, *space,
          a,
          b,
          c,
          alpha);

        std::cerr << " => " << rv->getChannel0() << ", " <<
          rv->getChannel1() << ", " << rv->getChannel2() << "\n";

        return rv;

      }
      else if (space == &ColorSpace::rgb) {
        auto a = _channelFromValue(logger, space->_channels[0], chn0, clamp);
        auto b = _channelFromValue(logger, space->_channels[1], chn1, clamp);
        auto c = _channelFromValue(logger, space->_channels[2], chn2, clamp);
        std::cerr << " RV " << a.value_or(0) << ", " << b.value_or(0) << ", " << c.value_or(0) << "\n";
        auto rv = SASS_MEMORY_NEW(ColorSpaced,
          pstate, *space,
          a,
          b,
          c,
          alpha);

        std::cerr << " => " << rv->getChannel0() << ", " <<
          rv->getChannel1() << ", " << rv->getChannel2() << "\n";

        return rv;
      }
      else {
        auto a = _channelFromValue(logger, space->_channels[0], chn0, clamp);
        auto b = _channelFromValue(logger, space->_channels[1], chn1, clamp);
        auto c = _channelFromValue(logger, space->_channels[2], chn2, clamp);
        std::cerr << " RV " << a.value_or(0) << ", " << b.value_or(0) << ", " << c.value_or(0) << "\n";
        auto rv = SASS_MEMORY_NEW(ColorSpaced,
          pstate, *space,
          a,
          b,
          c,
          alpha);

        std::cerr << " => " << rv->getChannel0() << ", " <<
          rv->getChannel1() << ", " << rv->getChannel2() << "\n";

        return rv;
      }
      return nullptr;
    }


    Value* _parseNumberOrString(
      Compiler& ctx,
      const SourceSpan& pstate,
      const sass::string& data)
    {
      SourceDataObj src = new SourceString(
        "sass://color", data);
      ScssParser parser(ctx, src);
      try {
        return parser.readSingleNumber();
      }
      catch (const std::runtime_error _)
      {
        return SASS_MEMORY_NEW(String, pstate, data);
      }
    }

    std::pair<ValueObj, ValueObj> _parseSlashChannels2(
      Compiler& ctx, const SourceSpan& pstate,
      Value* input, const sass::string& fname
    )
    {
      // Get the list from the channels input variable (or throw)
      ValueVector list = input->assertCommonListStyle(ctx, fname, true);

      if (list.empty()) return { nullptr, nullptr };

      // Check if list is seperated by a slash
      if (input->separator() == SASS_DIV) {
        // Only allow slash list with two items
        if (list.size() == 2) {
          return { list[0], list[1] };
        }
        // Otherwise throw an error
        else {
          sass::sstream message;
          message << "Only 2 slash-separated elements allowed, but " << list.size()
            << " " << pluralize("was", input->lengthAsList(), "were") << " passed.";
          throw Exception::SassScriptException(
            message.str(), ctx, pstate, "$channels");
        }
      }

      // Check if last element is a string without quotes
      if (String* back = list.back()->isaString()) {
        if (back->hasQuotes() == false) {
          auto parts = StringUtils::split(back->value(), '/', false);
          if (parts.size() == 0) return { nullptr, nullptr };
          else if (parts.size() == 1) return { input, nullptr };
          else if (parts.size() == 2) {
            auto initial = SASS_MEMORY_NEW(List, pstate, {
              list.begin(), list.end() - 1 }, SASS_SPACE);
            initial->append(_parseNumberOrString(ctx, pstate, parts[0]));
            return { initial, _parseNumberOrString(ctx, pstate, parts[1]) };
          }
        }
      }

      // debug_ast(list.back());

      // Check if last element is a number with slashes
      if (Number* back = list.back()->isaNumber()) {
        std::cerr << "Has As Slash " << back->hasAsSlash() << "\n";
        if (back->hasAsSlash() == true) {
          auto initial = SASS_MEMORY_NEW(List, pstate, {
            list.begin(), list.end() - 1 }, SASS_SPACE);
          initial->append(back->lhsAsSlash().ptr());
          return { initial, back->rhsAsSlash().ptr() };
        }
      }

      return { input, nullptr };
    }

    bool isNone(Value* value) {

      String* str = value->isaString();
      if (str == nullptr) return false;
      if (str->hasQuotes()) return false;
      return StringUtils::equalsIgnoreCase(
        str->value(), "none", 4);
    }

    // This one is now very close to dart sass!!!!
    Value* _parseChannels(const sass::string& fname,
      Value* input, sass::string name,
      const SourceSpan& pstate,
      Compiler& ctx,
      const ColorSpace* space = nullptr)
    {

      // Check for css var
      if (isVar(input)) {
        // return function string
        return SASS_MEMORY_NEW(
          String, pstate, fname + "(" +
          input->inspect() + ")");
      }

      // If last can look like "1/none", which is passed as string
      auto sp = _parseSlashChannels2(ctx, pstate, input, fname);

      // debug_ast(sp.first);
      // debug_ast(sp.second);

      // If parsing failed, return the function string
      if (sp.first == nullptr && sp.second == nullptr) {
        return SASS_MEMORY_NEW(
          String, pstate, fname + "(" +
          input->inspect() + ")");
      }

      // First value is guaranteed
      ValueObj components = sp.first;
      ValueObj alphaValue = sp.second;

      ValueVector channels;
      String* spaceName = nullptr;

      // Get the list from the channels input variable (or throw)
      ValueVector list = components->assertCommonListStyle(ctx, fname, false);

      if (list.size() == 0) {
        throw Exception::SassScriptException(
          "Color component list may not be empty.",
          ctx, pstate);
      }

      if (String* str = list.front()->isaString()) {
        if (str->hasQuotes() == false) {
          if (StringUtils::equalsIgnoreCase(
            str->value(), "from", 4)) {
            return SASS_MEMORY_NEW(
              String, pstate, fname + "(" +
              input->inspect() + ")");
          }
        }
      }

      if (isVar(components)) {
        channels.push_back(components);
      }
      else {
        if (space == nullptr) {
          Value* first = list.front();
          list.erase(list.begin());
          spaceName = first->assertString(ctx, fname);
          spaceName->assertUnquoted(ctx, fname);
          if (isVar(spaceName) == false) {
            space = ColorSpace::fromName(ctx, *spaceName);
          }
          // Move list to channels
          channels = std::move(list);

          /*
                  if (space
                      case ColorSpace.rgb ||
                          ColorSpace.hsl ||
                          ColorSpace.hwb ||
                          ColorSpace.lab ||
                          ColorSpace.lch ||
                          ColorSpace.oklab ||
                          ColorSpace.oklch) {
                    throw SassScriptException(
                        "The color() function doesn't support the color space $space. Use "
                        "the $space() function instead.",
                        name);
                  }
          */

        }
        else {
          // ToDo: is this correct?
          channels = std::move(list);
        }

        for (int i = 0; i < channels.size(); i++) {
          auto channel = channels[i];
          if (!isSpecialNumber(channel) &&
            !channel->isaNumber() &&
            !isNone(channel)) {

            auto qwe = isNone(channel);

            throw Exception::SassScriptException(
              "Expected to be a number was",
              ctx, pstate);
          }
        }

      }

      if (alphaValue != nullptr) {
        if (isSpecialNumber(alphaValue)) {
          std::cerr << "Not yet implemented\n";
          /*
  return channels.length == 3 && _specialCommaSpaces.contains(space)
      ? _functionString(functionName, [...channels, alphaValue!])
      : _functionString(functionName, [input]);
      */
        }
      }

      tl::optional<double> alpha = 1.0;
      if (alphaValue != nullptr) {
        if (const String* astr = alphaValue->isaString()) {
          if (!astr->hasQuotes() && astr->value() == "none") alpha.reset();
        }
        if (alpha.has_value()) {
          Number* nr = alphaValue->assertNumber(ctx, name);
          alpha = _percentageOrUnitless(nr, 1, "alpha", ctx);
          alpha = std::max(0.0, std::min(alpha.value(), 1.0));
        }
      }

      // `space` will be null if either `components` or `spaceName` is a `var()`.
      // Again, we check this here rather than returning early in those cases so
      // that we can verify `alphaValue` even for colors we can't fully parse.
      if (space == nullptr) {
        return SASS_MEMORY_NEW(
          String, pstate, fname + "(" +
          input->inspect() + ")");
      }

      /*

if (channels.any((channel) => channel.isSpecialNumber)) {
  return channels.length == 3 && _specialCommaSpaces.contains(space)
      ? _functionString(
          functionName, [...channels, if (alphaValue != null) alphaValue])
      : _functionString(functionName, [input]);
}
*/

      if (channels.size() != 3) {
        throw Exception::SassScriptException(
          "The $space color space has 3 channels but $input has",
          ctx, pstate);
      }

      auto rv = _colorFromChannels(
        ctx, pstate, space,
        channels[0]->isaNumber(),
        channels[1]->isaNumber(),
        channels[2]->isaNumber(),
        alpha, true,
        space == &ColorSpace::rgb
      );

      return rv;

      // Return arguments
      // return list.detach();

      // debug_ast(input);
      // _parseSlashChannels
      // std::cerr << "foobar\n";
      /*
      ValueVector args = input->assertCommonListStyle(ctx, name, true);

      StringObj spaceName = args[0]->assertString(ctx, name)->assertUnquoted(ctx, name);
      auto space = ColorSpace::fromName(ctx, spaceName);
      int startIdx = 1;

      Value* alphaValue = nullptr;
      ValueVector values;

      for (int i = startIdx; i < args.size(); i++) {
        Value* channel = args[i];
        if (!isSpecialNumber(channel)) {

        }
        // Specially treat last component
        // May contain the alpha value
        if (i == args.size() - 1) {
          // std::cerr << " " << i << " sep " << args[i]->separator() << "\n";
          if (args[i]->hasSlashSeparator()) {
            if (Number* nr = args[i]->isaNumber()) {
              values.push_back(nr->lhsAsSlash().ptr());
              alphaValue = nr->rhsAsSlash().ptr();
            }
            else {
              values.push_back(args[i]);
            }
          }
          else {
            values.push_back(args[i]);
          }
        }
        else {
          values.push_back(args[i]);
        }
      }*/

      // if (!strict && (isSpecialNumber(_h) || isSpecialNumber(_w) || isSpecialNumber(_b) || isSpecialNumber(_a))) {
      //   sass::sstream fncall;
      //   fncall << name << "(";
      //   fncall << _h->inspect() << ", ";
      //   fncall << _w->inspect() << ", ";
      //   fncall << _b->inspect();
      //   if (_a) { fncall << ", " << _a->inspect(); }
      //   fncall << ")";
      //   return SASS_MEMORY_NEW(String, pstate, fncall.str());
      // }
      return nullptr;

    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    namespace Colors {

      /*******************************************************************/

      static BUILT_IN_FN(rgb4arg)
      {
        return _parseChannels(str_rgb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::rgb);

        return rgbFn(Strings::rgb,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(rgb3arg)
      {
        return rgbFn(Strings::rgb,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnRgb4arg)
      {
        return rgbFn(Strings::rgb,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(fnRgb3arg)
      {
        return rgbFn(Strings::rgb,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(rgb2arg)
      {
        return handleTwoArgRgb(Strings::rgb,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnRgb2arg)
      {
        return handleTwoArgRgb(Strings::rgb,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(rgb1arg)
      {
        return _parseChannels(str_rgb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::rgb);
      }

      static BUILT_IN_FN(fnRgb1arg)
      {
        auto rv = _parseChannels(str_oklab, arguments[0],
          "channels", pstate, compiler, &ColorSpace::rgb);
        return rv;

        // std::cerr << "Hello dear\n";
        // #if SassPreserveColorInfo
        // if (Color* color = arguments[0]->isaColor()) {
        //   Color* rgb = color->toRGBA();
        //   rgb->a(1.0);
        //   return rgb;
        // }
        // #endif
        // return handleOneArgColorFn(Strings::rgb,
        //   arguments[0], &rgbFn, compiler, pstate, true);
      }

      /*******************************************************************/

      static BUILT_IN_FN(rgba4arg)
      {
        return rgbFn(Strings::rgba,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(rgba3arg)
      {
        return rgbFn(Strings::rgba,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnRgba4arg)
      {
        return rgbFn(Strings::rgba,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(fnRgba3arg)
      {
        return rgbFn(Strings::rgba,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(rgba2arg)
      {
        return handleTwoArgRgb(Strings::rgba,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnRgba2arg)
      {
        return handleTwoArgRgb(Strings::rgba,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(rgba1arg)
      {
        return _parseChannels(str_rgb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::rgb);
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          return color->toRGBA();
        }
        #endif
        return handleOneArgColorFn(Strings::rgba,
          arguments[0], &rgbFn, compiler, pstate, false);
      }

      static BUILT_IN_FN(fnRgba1arg)
      {
        return _parseChannels(str_rgb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::rgb);
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          return color->toRGBA();
        }
        #endif
        return handleOneArgColorFn(Strings::rgba,
          arguments[0], &rgbFn, compiler, pstate, true);
      }

      /*******************************************************************/

      static BUILT_IN_FN(hsl4arg)
      {
        return hslFn(Strings::hsl,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(hsl3arg)
      {
        return hslFn(Strings::hsl,
          arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnHsl4arg)
      {
        return hslFn(Strings::hsl,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(fnHsl3arg)
      {
        return hslFn(Strings::hsl,
          arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(hsl2arg)
      {
        // hsl(123, var(--foo)) is valid CSS because --foo might be `10%, 20%`
        // and functions are parsed after variable substitution.
        if (isVar(arguments[0]) || isVar(arguments[1])) {
          return getFunctionString(Strings::hsl, pstate, arguments);
        }
        // Otherwise throw error for missing argument
        throw Exception::MissingArgument(compiler, key_lightness);
      }

      static BUILT_IN_FN(fnHsl2arg)
      {
        // Otherwise throw error for missing argument
        throw Exception::TooManyArguments(compiler, 2, 1);
      }

      static BUILT_IN_FN(hsl1arg)
      {
        auto rv = _parseChannels(str_hsl, arguments[0],
          "channels", pstate, compiler, &ColorSpace::hsl);
        return rv;
        /*
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          Color* hsl = color->toHSLA();
          hsl->a(1.0);
          return hsl;
        }
        #endif
        return handleOneArgColorFn(Strings::hsl,
          arguments[0], &hslFn, compiler, pstate, false);
        */
      }

      static BUILT_IN_FN(fnHsl1arg)
      {
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          Color* hsl = color->toHSLA();
          hsl->a(1.0);
          return hsl;
        }
        #endif
        return handleOneArgColorFn(Strings::hsl,
          arguments[0], &hslFn, compiler, pstate, true);
      }

      /*******************************************************************/

      static BUILT_IN_FN(hsla4arg)
      {
        return hslFn(Strings::hsla, arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(hsla3arg)
      {
        return hslFn(Strings::hsla, arguments, pstate, compiler, false);
      }

      static BUILT_IN_FN(fnHsla4arg)
      {
        return hslFn(Strings::hsla, arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(fnHsla3arg)
      {
        return hslFn(Strings::hsla, arguments, pstate, compiler, true);
      }

      static BUILT_IN_FN(hsla2arg)
      {
        // hsl(123, var(--foo)) is valid CSS because --foo might be `10%, 20%`
        // and functions are parsed after variable substitution.
        if (isVar(arguments[0]) || isVar(arguments[1])) {
          return getFunctionString(Strings::hsla, pstate, arguments);
        }
        // Otherwise throw error for missing argument
        throw Exception::MissingArgument(compiler, key_lightness);
      }

      static BUILT_IN_FN(fnHsla2arg)
      {
        // Otherwise throw error for missing argument
        throw Exception::TooManyArguments(compiler, 2, 1);
      }

      static BUILT_IN_FN(hsla1arg)
      {
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          return color->toHSLA();
        }
        #endif
        return handleOneArgColorFn(Strings::hsla,
          arguments[0], &hslFn, compiler, pstate, false);
      }

      static BUILT_IN_FN(fnHsla1arg)
      {
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          return color->toHSLA();
        }
        #endif
        return handleOneArgColorFn(Strings::hsla,
          arguments[0], &hslFn, compiler, pstate, true);
      }

      /*******************************************************************/

      static BUILT_IN_FN(hwb4arg)
      {
        auto rv = _parseChannels(str_hwb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::hwb);
        return rv;
        return hwbFn(Strings::hwb,
          arguments, pstate, compiler, false);
      }


      // static BUILT_IN_FN(hwb3arg)
      // {
      //   return hwbFn(Strings::hwb,
      //     arguments, pstate, compiler, false);
      // }

      static BUILT_IN_FN(fnHwb4arg)
      {
        auto rv = _parseChannels(str_hwb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::hwb);
        return rv;
        return hwbFn(Strings::hwb,
          arguments, pstate, compiler, true);
      }


      // static BUILT_IN_FN(fnHwb3arg)
      // {
      //   return hwbFn(Strings::hwb,
      //     arguments, pstate, compiler, true);
      // }

      //static BUILT_IN_FN(hwb2arg)
      //{
      //  return getFunctionString(Strings::hwb, pstate, arguments);
      //}

      // static BUILT_IN_FN(fnHwb2arg)
      // {
      //   // Otherwise throw error for missing argument
      //   throw Exception::TooManyArguments(compiler, 2, 1);
      // }

      static BUILT_IN_FN(hwb1arg)
      {
        auto rv = _parseChannels(str_hwb, arguments[0],
          "channels", pstate, compiler, &ColorSpace::hwb);
        return rv;

        /*
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          Color* hwb = color->toHWBA();
          hwb->a(1.0);
          return hwb;
        }
        #endif
        return handleOneArgColorFn(Strings::hwb,
          arguments[0], &hwbFn, compiler, pstate, false);
        */
      }

      static BUILT_IN_FN(fnHwb1arg)
      {
        #if SassPreserveColorInfo
        if (Color* color = arguments[0]->isaColor()) {
          Color* hwb = color->toHWBA();
          hwb->a(1.0);
          return hwb;
        }
        #endif
        ValueObj value = handleOneArgColorFn(Strings::hwb,
          arguments[0], &hwbFn, compiler, pstate, true);
        if (value->isaString()) {
          throw Exception::RuntimeException(compiler, "Expected "
            "numeric channels, got \"" + value->inspect() + "\".");
        }
        return value.detach();
      }

      /*******************************************************************/


      static BUILT_IN_FN(oklab)
      {
        auto rv = _parseChannels(str_oklab, arguments[0],
          "channels", pstate, compiler, &ColorSpace::oklab);
        return rv;
      }

      static BUILT_IN_FN(oklch)
      {
        return _parseChannels(str_oklch, arguments[0],
          "channels", pstate, compiler, &ColorSpace::oklch);
      }
      static BUILT_IN_FN(lab)
      {
        return _parseChannels(str_lab, arguments[0],
          "channels", pstate, compiler, &ColorSpace::lab);
      }
      static BUILT_IN_FN(lch)
      {
        return _parseChannels(str_lch, arguments[0],
          "channels", pstate, compiler, &ColorSpace::lch);
      }

      /*******************************************************************/

      // static BUILT_IN_FN(hwba4arg)
      // {
      //   return hwbFn(Strings::hwba, arguments, pstate, compiler, false);
      // }

      // static BUILT_IN_FN(hwba3arg)
      // {
      //   return hwbFn(Strings::hwba, arguments, pstate, compiler, false);
      // }

      // static BUILT_IN_FN(fnHwba4arg)
      // {
      //   return hwbFn(Strings::hwba, arguments, pstate, compiler, true);
      // }

      // static BUILT_IN_FN(fnHwba3arg)
      // {
      //   return hwbFn(Strings::hwba, arguments, pstate, compiler, true);
      // }

      // static BUILT_IN_FN(hwba2arg)
      // {
      //   return getFunctionString(Strings::hwba, pstate, arguments);
      // }

      // static BUILT_IN_FN(fnHwba2arg)
      // {
      //   throw Exception::TooManyArguments(compiler, 2, 1);
      // }

      // static BUILT_IN_FN(hwba1arg)
      // {
      //   #if SassPreserveColorInfo
      //   if (Color* color = arguments[0]->isaColor()) {
      //     return color->toHWBA();
      //   }
      //   #endif
      //   return handleOneArgColorFn(Strings::hwba,
      //     arguments[0], &hwbFn, compiler, pstate, false);
      // }
      // 
      // static BUILT_IN_FN(fnHwba1arg)
      // {
      //   #if SassPreserveColorInfo
      //   if (Color* color = arguments[0]->isaColor()) {
      //     return color->toHWBA();
      //   }
      //   #endif
      //   ValueObj value = handleOneArgColorFn(Strings::hwba,
      //     arguments[0], &hwbFn, compiler, pstate, true);
      //   if (value->isaString()) {
      //     throw Exception::RuntimeException(compiler, "Expected "
      //       "numeric channels, got \"" + value->inspect() + "\".");
      //   }
      //   return value.detach();
      // }

      /*******************************************************************/

      

      static BUILT_IN_FN(isInGamut)
      {

        const ColorSpaced* color = arguments[0]->assertColorSpaced(compiler, Strings::color);
        return SASS_MEMORY_NEW(Boolean, pstate, color->isInGamut());
      }

      static BUILT_IN_FN(isLegacy)
      {

        const ColorSpaced* color = arguments[0]->assertColorSpaced(compiler, Strings::color);
        return SASS_MEMORY_NEW(Boolean, pstate, color->isLegacy());
      }

      static BUILT_IN_FN(isPowerless)
      {

        const ColorSpaced* color = arguments[0]->assertColorSpaced(compiler, Strings::color);
        const String* channel = arguments[1]->assertString(compiler, "channel");
        bool chnValue = color->isChannelPowerless(compiler, channel, "color", "channel");
        return SASS_MEMORY_NEW(Boolean, pstate, chnValue);
      }

      /// Returns the [colorUntyped] as a [SassColor] in the color space specified by
      /// [spaceUntyped].
      ///
      /// If [legacyMissing] is false, this will convert missing channels in legacy
      /// color spaces to zero if a conversion occurs.
      ///
      /// Throws a [SassScriptException] if either argument isn't the expected type or
      /// if [spaceUntyped] isn't the name of a color space. If [spaceUntyped] is
      /// `sassNull`, it defaults to the color's existing space.
      const ColorSpaced* _colorInSpace(Compiler& compiler, Value* col, Value* spc, bool legacyMissing = true)
      {
        const ColorSpaced* color = col->assertColorSpaced(compiler, Strings::color);
        std::cerr << "Called color in space " << color->getChannel0() << ", "
          << color->getChannel1() << ", " << color->getChannel2() << "\n";
        if (spc == nullptr || spc->isNull()) return color;
        String* space = spc->assertString(compiler, "space");
        space->assertUnquoted(compiler, "space");
        const ColorSpace& cpsc = ColorSpace::fromNameRef(compiler, *space);
        ColorSpacedObj rv = color->toSpace(cpsc, col->pstate());
        return rv.detach();
      }

      static BUILT_IN_FN(channel)
      {

        const ColorSpaced* color = _colorInSpace(compiler, arguments[0], arguments[2]);
          // arguments[0]->assertColorSpaced(compiler, Strings::color);
        const String* channel = arguments[1]->assertString(compiler, "channel");

        int idx = color->getChannelIndex(compiler, channel, "color", "channel");
        if (idx == -1) return SASS_MEMORY_NEW(Number, pstate, color->alpha().value_or(0));
        auto chnInfo = color->space()._channels[idx];
        double chnValue = color->getChannel(idx);

        if (chnInfo.unit == "%") {
          if (chnInfo.isLinear) {
            if (chnInfo.max != 0) chnValue = chnValue * 100 / chnInfo.max;
          }
        }
        return SASS_MEMORY_NEW(Number, pstate, chnValue, chnInfo.unit);
      }
      

      static BUILT_IN_FN(isMissing)
      {
        const ColorSpaced* color = arguments[0]->assertColorSpaced(compiler, Strings::color);
        const String* channel = arguments[1]->assertString(compiler, "channel");
        bool missing = color->isChannelMissing(compiler, channel, "color", "channel");
        return SASS_MEMORY_NEW(Boolean, pstate, missing);
      }

      static BUILT_IN_FN(space)
      {
        const ColorSpaced* color = arguments[0]->assertColorSpaced(compiler, Strings::color);
        return SASS_MEMORY_NEW(String, pstate, color->space().name());
        // return _parseChannels(str_color, arguments[0], "description", pstate, compiler);
        // 
        // ColorRgbaObj rgba(color->toRGBA()); // This might create a copy
        // 
      }

      static BUILT_IN_FN(color)
      {

        // debug_ast(arguments[0]);

        return _parseChannels(str_color, arguments[0], "description", pstate, compiler);
        // const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        // ColorRgbaObj rgba(color->toRGBA()); // This might create a copy
        // return SASS_MEMORY_NEW(Number, pstate, Sass::round64(rgba->r(), compiler.epsilon));
      }

      static BUILT_IN_FN(red)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorRgbaObj rgba(color->toRGBA()); // This might create a copy
        return SASS_MEMORY_NEW(Number, pstate, Sass::round64(rgba->r(), compiler.epsilon));
      }

      static BUILT_IN_FN(green)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorRgbaObj rgba(color->toRGBA()); // This might create a copy
        return SASS_MEMORY_NEW(Number, pstate, Sass::round64(rgba->g(), compiler.epsilon));
      }

      static BUILT_IN_FN(blue)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorRgbaObj rgba(color->toRGBA()); // This might create a copy
        return SASS_MEMORY_NEW(Number, pstate, Sass::round64(rgba->b(), compiler.epsilon));
      }

      /*******************************************************************/

      static BUILT_IN_FN(invert)
      {
        const Number* weight = arguments[1]->assertNumber(compiler, Strings::weight);
        weight->checkPercent(compiler, Strings::weight);
        if (arguments[0]->isaNumber() || isSpecialNumber(arguments[0]) /* or isSpecialValue*/) {
          // Allow only the value `100` or a percentage (unit == `% `)
          const Number* weight = arguments[1]->assertNumber(compiler, Strings::weight);
          if (weight->value() != 100 || !weight->hasUnit(Strings::percent)) {
            throw Exception::RuntimeException(compiler,
              "Only one argument may be passed "
              "to the plain-CSS invert() function.");
          }
          // Return function string since first argument was a number
          // Need to remove the weight argument as it has a default value
          return getFunctionString(Strings::invert, pstate, { arguments[0] });
        }
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorRgbaObj inverse(color->copyAsRGBA()); // Make a copy!
        inverse->r(clamp(255.0 - inverse->r(), 0.0, 255.0));
        inverse->g(clamp(255.0 - inverse->g(), 0.0, 255.0));
        inverse->b(clamp(255.0 - inverse->b(), 0.0, 255.0));
        // Note: mixColors will create another unnecessary copy!
        return mixColors(inverse, color, weight, pstate, compiler);
      }

      static BUILT_IN_FN(fnInvert)
      {
        if (arguments[0]->isaNumber()) {
          compiler.addDeprecation(arguments[0]->pstate(),
            Logger::WARN_NUMBER_ARG, [arguments]() {
              return "Passing a number (" +
                arguments[0] + ") to color.invert() is deprecated.\n"
                "\nRecommendation: grayscale(" + arguments[0] + ")";
            });
        }

        if (isSpecialNumber(arguments[0])) {
          return getFunctionString(
            Strings::invert,
            pstate, arguments);
        }

        // if (arguments[0]->isaNumber()) {
        //   compiler.addWarning("Passing a number to "
        //     "color.invert() is deprecated.\n\nRecommendation: "
        //     "invert(" + arguments[0]->inspect() + ")",
        //     arguments[0]->pstate(),
        //     Logger::WARN_NUMBER_ARG);
        // }
        return invert(pstate, arguments, compiler, eval);
      }

      /*******************************************************************/

      static BUILT_IN_FN(hue)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorHslaObj hsla(color->toHSLA()); // This probably creates a copy
        return SASS_MEMORY_NEW(Number, pstate, hsla->h(), Strings::deg);
      }

      static BUILT_IN_FN(saturation)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorHslaObj hsla(color->toHSLA()); // This probably creates a copy
        return SASS_MEMORY_NEW(Number, pstate, hsla->s(), Strings::percent);
      }

      static BUILT_IN_FN(lightness)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorHslaObj hsla(color->toHSLA()); // This probably creates a copy
        return SASS_MEMORY_NEW(Number, pstate, hsla->l(), Strings::percent);
      }

      static BUILT_IN_FN(noLighten)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "lighten", "$lightness: ");
      }

      static BUILT_IN_FN(noDarken)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "darken", "$lightness: -");
      }

      static BUILT_IN_FN(whiteness)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        #if SassPreserveColorInfo
        ColorHwbaObj hwba(color->toHWBA());
        #else
        ColorRgbaObj rgba(color->copyAsRGBA());
        rgba->r(round64(rgba->r(), compiler.epsilon));
        rgba->g(round64(rgba->g(), compiler.epsilon));
        rgba->b(round64(rgba->b(), compiler.epsilon));
        ColorHwbaObj hwba(rgba->toHWBA());
        #endif
        return SASS_MEMORY_NEW(Number, pstate, hwba->w(), Strings::percent);
      }

      static BUILT_IN_FN(blackness)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        #if SassPreserveColorInfo
        ColorHwbaObj hwba(color->toHWBA());
        #else
        ColorRgbaObj rgba(color->copyAsRGBA());
        rgba->r(round64(rgba->r(), compiler.epsilon));
        rgba->g(round64(rgba->g(), compiler.epsilon));
        rgba->b(round64(rgba->b(), compiler.epsilon));
        ColorHwbaObj hwba(rgba->toHWBA());
        #endif
        return SASS_MEMORY_NEW(Number, pstate, hwba->b(), Strings::percent);
      }

      /*******************************************************************/

      static BUILT_IN_FN(adjustHue)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* degrees = arguments[1]->assertNumber(compiler, Strings::degrees);
        checkAngle(compiler, degrees, Strings::degrees);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->h(absmod(copy->h() + coerceToDeg(degrees), 360.0));
        return copy.detach();
      }

      static BUILT_IN_FN(noAdjustHue)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "adjust-hue", "$hue: ", Strings::degrees);
      }

      static BUILT_IN_FN(complement)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->h(absmod(copy->h() + 180.0, 360.0));
        return copy.detach();
      }

      /*******************************************************************/

      static BUILT_IN_FN(grayscale)
      {
        // Gracefully handle if number is passed
        if (arguments[0]->isaNumber() || isSpecialNumber(arguments[0])) {
          return getFunctionString(
            Strings::grayscale,
            pstate, arguments);
        }
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->s(0.0); // Simply reset the saturation
        return copy.detach(); // Return HSLA
      }

      static BUILT_IN_FN(lighten)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 100.0, amount, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->l(clamp(copy->l() + nr, 0.0, 100.0));
        return copy.detach(); // Return HSLA
      }

      static BUILT_IN_FN(darken)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 100.0, amount, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->l(clamp(copy->l() - nr, 0.0, 100.0));
        return copy.detach(); // Return HSLA
      }

      /*******************************************************************/

      static BUILT_IN_FN(saturate2arg)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 100.0, amount, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        if (copy->h() == 0 && nr > 0.0) copy->h(100.0);
        copy->s(clamp(copy->s() + nr, 0.0, 100.0));
        return copy.detach(); // Return HSLA
      }

      static BUILT_IN_FN(saturate1arg)
      {
        if (arguments[0]->isaNumber() || isSpecialNumber(arguments[0])) {
          return getFunctionString(
            Strings::saturate,
            pstate, arguments);
        }
        arguments[0]->assertNumber(compiler, Strings::amount);
        return getFunctionString(Strings::saturate, pstate, { arguments[0] });
      }

      static BUILT_IN_FN(desaturate)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 100.0, amount, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->s(clamp(copy->s() - nr, 0.0, 100.0));
        return copy.detach(); // Return HSLA
      }


      static BUILT_IN_FN(noFadeIn)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "fade-in", "$alpha: ");
      }
      
      static BUILT_IN_FN(noFadeOut)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "fade-out", "$alpha: -");
      }

      static BUILT_IN_FN(noTansparentize)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "transparentize", "$alpha: -");
      }

     
      static BUILT_IN_FN(noSaturate)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "saturate", "$saturation: ");
      }
        
      static BUILT_IN_FN(noDesaturate)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "desaturate", "$saturation: -");
      }

      /*******************************************************************/

      static BUILT_IN_FN(opacify)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 1.0, unit_none, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->a(clamp(copy->a() + nr, 0.0, 1.0));
        return copy.detach(); // Return HSLA
      }

      static BUILT_IN_FN(transparentize)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        const Number* amount = arguments[1]->assertNumber(compiler, Strings::amount);
        double nr = amount->assertRange(0.0, 1.0, unit_none, compiler, Strings::amount);
        ColorHslaObj copy(color->copyAsHSLA()); // Must make a copy!
        copy->a(clamp(copy->a() - nr, 0.0, 1.0));
        return copy.detach(); // Return HSLA
      }

      static BUILT_IN_FN(noOpacify)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "opacify", "$alpha: ");
      }


      static BUILT_IN_FN(toGamut)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "opacify", "$alpha: ");
      }

      /*
      static BUILT_IN_FN(noTransparentize)
      {
        throw Exception::DeprecatedColorAdjustFn(compiler,
          arguments, "transparentize", "$alpha: -");
      }
      */

      /*******************************************************************/

      static BUILT_IN_FN(alphaOne)
      {
        if (String * string = arguments[0]->isaString()) {
          if (!string->hasQuotes() && isMsFilterStart(string->value())) {
            // dart-sass has two different code paths for this deprecation!?
            // compiler.addDeprecation("Using color.alpha() for a Microsoft"
            //   "filter is deprecated.\n\n  Recommendation: " + fn->toCss(),
            //   arguments[0]->pstate(), Logger::WARN_MS_ALPHA);
            return getFunctionString(Strings::alpha, pstate, arguments);
          }
        }

        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        return SASS_MEMORY_NEW(Number, pstate, color->a());
      }

      static BUILT_IN_FN(alphaAny)
      {
        size_t size = arguments[0]->lengthAsList();

        if (size == 0) {
          throw Exception::MissingArgument(compiler, Keys::color);
        }

        bool isOnlyIeFilters = true;
        for (Value* value : arguments[0]->start()) {
          if (String* string = value->isaString()) {
            if (!isMsFilterStart(string->value())) {
              isOnlyIeFilters = false;
              break;
            }
          }
          else {
            isOnlyIeFilters = false;
            break;
          }
        }
        if (isOnlyIeFilters) {
          // Support the proprietary Microsoft alpha() function.
          return getFunctionString(Strings::alpha, pstate, arguments);
        }
        CallStackFrame csf(compiler, arguments[0]->pstate());
        throw Exception::TooManyArguments(compiler, size, 1);
      }


      static BUILT_IN_FN(opacity)
      {
        // Gracefully handle if number is passed
        if (arguments[0]->isaNumber() || isSpecialNumber(arguments[0])) {
          return getFunctionString("opacity",
            pstate, arguments);
        }
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        return SASS_MEMORY_NEW(Number, pstate, color->a());
      }

      static BUILT_IN_FN(noGrayscale)
      {
        if (arguments[0]->isaNumber()) {
          compiler.addDeprecation(arguments[0]->pstate(),
            Logger::WARN_NUMBER_ARG, [arguments]() {
              return "Passing a number (" +
                arguments[0] + ") to color.grayscale() is deprecated.\n"
                "\nRecommendation: grayscale(" + arguments[0] + ")";
            });
        }
        return grayscale(pstate, arguments, compiler, eval);
      }
        


      static BUILT_IN_FN(noOpacity)
      {
        if (arguments[0]->isaNumber()) {
          compiler.addDeprecation(arguments[0]->pstate(),
            Logger::WARN_NUMBER_ARG, [arguments]() {
              return "Passing a number (" +
                arguments[0] + ") to color.opacity() is deprecated.\n"
                "\nRecommendation: opacity(" + arguments[0] + ")";
            });
        }
        return opacity(pstate, arguments, compiler, eval);
      }

      static BUILT_IN_FN(ieHexStr)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ColorRgbaObj rgba = color->toRGBA(); // This might create a copy
        // clamp should not be needed here
        double r = clamp(rgba->r(), 0.0, 255.0);
        double g = clamp(rgba->g(), 0.0, 255.0);
        double b = clamp(rgba->b(), 0.0, 255.0);
        double a = clamp(rgba->a(), 0.0, 1.0) * 255.0;
        sass::sstream ss;
        ss << '#' << std::setw(2) << std::setfill('0') << std::uppercase;
        ss << std::hex << std::setw(2) << fuzzyRound(a, compiler.epsilon);
        ss << std::hex << std::setw(2) << fuzzyRound(r, compiler.epsilon);
        ss << std::hex << std::setw(2) << fuzzyRound(g, compiler.epsilon);
        ss << std::hex << std::setw(2) << fuzzyRound(b, compiler.epsilon);
        return SASS_MEMORY_NEW(String, pstate, ss.str());
      }

      static Number* getKwdNumber(ValueFlatMap* keywords, const EnvKey& name, Logger& logger)
      {
        if (keywords == nullptr) return nullptr;
        auto kv = keywords->find(name);
        // Return null since args are optional
        if (kv == keywords->end()) return nullptr;
        // Get the number object from found keyword
        Number* num = kv->second->assertNumber(logger, name.orig());
        // Only consume keyword once
        keywords->erase(kv);
        // Return the number
        return num;
      }

      static String* getKwdString(ValueFlatMap* keywords, const EnvKey& name, Logger& logger)
      {
        if (keywords == nullptr) return nullptr;
        auto kv = keywords->find(name);
        // Return null since args are optional
        if (kv == keywords->end()) return nullptr;
        // Get the number object from found keyword
        String* num = kv->second->assertString(logger, name.orig());
        // Only consume keyword once
        keywords->erase(kv);
        // Return the number
        return num;
      }

      /*
      Sass::Value* _updateComponents(const Sass::ValueVector& arguments,
        bool adjust = false, bool scale = false, bool change = false)
      {
        return arguments[0];
      }
      */

      const ColorSpace* _sniffLegacyColorSpace(const ValueFlatMap* kwds)
      {
        /*
        for each(auto kv in *kwds)
        {
          const sass::string& key = kv.first.norm();
          if (key == "red") return &ColorSpace::rgb;
          if (key == "green") return &ColorSpace::rgb;
          if (key == "blue") return &ColorSpace::rgb;
          if (key == "saturation") return &ColorSpace::hsl;
          if (key == "lightness") return &ColorSpace::hsl;
          if (key == "whiteness") return &ColorSpace::hwb;
          if (key == "blackness") return &ColorSpace::hwb;
        }
        if (kwds->count(key_hue) != 0)
          return &ColorSpace::hsl;
        else return nullptr;
        */
        return nullptr;
      }

      ColorSpaced* _colorInSpace(ColorSpaced* colorUntyped, const String* spaceUntyped, Compiler& compiler, bool legacyMissing = true)
      {
        ColorSpaced* color = colorUntyped->assertColorSpaced2(compiler, "color");
        if (spaceUntyped == nullptr) return color;
        if (spaceUntyped->isNull()) return color;
        const ColorSpace* space = ColorSpace::fromName(compiler, *spaceUntyped);
        ColorSpacedObj rv = color->toSpace(*space, colorUntyped->pstate(), legacyMissing);
        return rv.detach();
      }

      tl::optional<double> _adjustChannel(ColorSpaced* color, ColorChannel channel, tl::optional<double> oldValue, Number* adjustmentArg)
      {
        return 0;
      }

      ColorSpaced* _adjustColor(Logger& logger, ColorSpaced* color, Number** channelArgs, Number* alphaArg)
      {
        ColorSpacedObj rv = ColorSpaced::_forSpace(color->pstate(), color->space(),
          _adjustChannel(color, color->space()._channels[0], color->getChannel0(), channelArgs[0]),
          _adjustChannel(color, color->space()._channels[1], color->getChannel1(), channelArgs[1]),
          _adjustChannel(color, color->space()._channels[2], color->getChannel2(), channelArgs[2]),
          _adjustChannel(color, AlphaChannel, color->getAlpha(), alphaArg)
            .and_then([&](tl::optional<double> a) { return a; })
        );
        return rv.detach();
      }

      static BUILT_IN_FN(adjust)
      {
        Color* color2 = arguments[0]
          ->assertColor2(compiler, Strings::color);

        const Color* color = arguments[0]
          ->assertColor(compiler, Strings::color);
        ArgumentList* argumentList = arguments[1]
          ->assertArgumentList(compiler, "kwargs");
        if (!argumentList->empty()) {
          SourceSpan span(color->pstate());
          CallStackFrame frame(compiler, BackTrace(
            span, Strings::colorAdjust));
          throw Exception::RuntimeException(compiler,
            "Only one positional argument is allowed. All "
            "other arguments must be passed by name.");
        }

        // ToDo: solve without erase ...
        ValueFlatMap* kwds = argumentList->keywords();

        {
          Number* nr_r = getKwdNumber(kwds, key_red, compiler);
          Number* nr_g = getKwdNumber(kwds, key_green, compiler);
          Number* nr_b = getKwdNumber(kwds, key_blue, compiler);
          Number* nr_h = getKwdNumber(kwds, key_hue, compiler);
          Number* nr_s = getKwdNumber(kwds, key_saturation, compiler);
          Number* nr_l = getKwdNumber(kwds, key_lightness, compiler);
          Number* nr_a = getKwdNumber(kwds, key_alpha, compiler);
          Number* nr_wn = getKwdNumber(kwds, key_whiteness, compiler);
          Number* nr_bn = getKwdNumber(kwds, key_blackness, compiler);

          ColorObj copy = SASS_MEMORY_COPY(color2);

          return copy.detach();
        }



        String* str_space = getKwdString(kwds, key_space, compiler);

        if (dynamic_cast<ColorSpaced*>(color2)) {
          auto col = dynamic_cast<ColorSpaced*>(color2);
          std::cerr << "ASDASDASDASD " << col->space().name() << "\n";
          const ColorSpace* legacy = _sniffLegacyColorSpace(kwds);

          ColorSpacedObj bar = legacy != nullptr ?
            col->toSpace(*legacy, pstate, false).ptr() :
            _colorInSpace(col, str_space, compiler);

          {

            for (int i = 0; i < bar->space()._channelSize; i++)
            {

              const double oldChannel = bar->getChannel(i);
              double* channelArg = nullptr;
              const ColorChannel& channelInfo0 = bar->space()._channels[i];

            }

            const double oldChannels[] = {
              bar->getChannel0(),
              bar->getChannel1(),
              bar->getChannel2()
            };

            Number* channelArgs[] = {
              nullptr,
              nullptr,
              nullptr
            };

            auto qwe = kwds->find(key_lightness);
            if (qwe != kwds->end()) {
              channelArgs[0] = qwe->second->assertNumber(compiler, str_lightness);
            }

            for (int i = 0; i < bar->space()._channelSize; i++)
            {
              // std::cerr << ""
            }

            const ColorChannel& channelInfo0 = bar->space()._channels[0];
            const ColorChannel& channelInfo1 = bar->space()._channels[1];
            const ColorChannel& channelInfo2 = bar->space()._channels[2];

            return _adjustColor(compiler, bar, channelArgs, nullptr);
          }

        }

        if (str_space != nullptr /*  && color->space() */ ) {

        }
        else {

          Number* nr_r = getKwdNumber(kwds, key_red, compiler);
          Number* nr_g = getKwdNumber(kwds, key_green, compiler);
          Number* nr_b = getKwdNumber(kwds, key_blue, compiler);
          Number* nr_h = getKwdNumber(kwds, key_hue, compiler);
          Number* nr_s = getKwdNumber(kwds, key_saturation, compiler);
          Number* nr_l = getKwdNumber(kwds, key_lightness, compiler);
          Number* nr_a = getKwdNumber(kwds, key_alpha, compiler);
          Number* nr_wn = getKwdNumber(kwds, key_whiteness, compiler);
          Number* nr_bn = getKwdNumber(kwds, key_blackness, compiler);

          if (nr_h) checkAngle(compiler, nr_h, Strings::hue);
          if (nr_s) nr_s->checkPercent(compiler, Strings::saturation);
          if (nr_l) nr_l->checkPercent(compiler, Strings::lightness);

          double r = nr_r ? nr_r->assertRange(-255.0, 255.0, unit_none, compiler, Strings::red) : 0.0;
          double g = nr_g ? nr_g->assertRange(-255.0, 255.0, unit_none, compiler, Strings::green) : 0.0;
          double b = nr_b ? nr_b->assertRange(-255.0, 255.0, unit_none, compiler, Strings::blue) : 0.0;
          double s = nr_s ? nr_s->assertRange(-100.0, 100.0, unit_percent, compiler, Strings::saturation) : 0.0;
          double l = nr_l ? nr_l->assertRange(-100.0, 100.0, unit_percent, compiler, Strings::lightness) : 0.0;

          double wn = nr_wn ? nr_wn->assertHasUnits(compiler, Strings::percent, Strings::whiteness)->assertRange(-100.0, 100.0, nr_wn, compiler, Strings::whiteness) : 0.0;
          double bn = nr_bn ? nr_bn->assertHasUnits(compiler, Strings::percent, Strings::blackness)->assertRange(-100.0, 100.0, nr_bn, compiler, Strings::blackness) : 0.0;

          double a = nr_a ? nr_a->assertRange(-1.0, 1.0, nr_a, compiler, Strings::alpha) : 0.0;

          double h = nr_h ? coerceToDeg(nr_h) : 0.0; // Hue is a very special case

          if (kwds && !kwds->empty()) {
            throw Exception::UnknownNamedArgument(compiler, kwds);
          }

          bool hasRgb = nr_r || nr_g || nr_b;
          bool hasHsl = nr_s || nr_l;
          bool hasHwb = nr_wn || nr_bn;
          bool hasHue = nr_h != nullptr;

          if (hasRgb && hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL", "HWB" });
          else if (hasRgb && hasHue) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL/HWB" });
          else if (hasRgb && hasHsl) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL" });
          else if (hasRgb && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HWB" });
          else if (hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });
          else if (hasHwb && hasHsl) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });

          if (hasRgb) {
            ColorRgbaObj rgba = color->copyAsRGBA();
            if (nr_r) rgba->r(clamp(rgba->r() + r, 0.0, 255.0));
            if (nr_g) rgba->g(clamp(rgba->g() + g, 0.0, 255.0));
            if (nr_b) rgba->b(clamp(rgba->b() + b, 0.0, 255.0));
            if (nr_a) rgba->a(clamp(rgba->a() + a, 0.0, 1.0));
            return rgba.detach();
          }
          else if (hasHsl) {
            ColorHslaObj hsla = color->copyAsHSLA();
            if (nr_h) hsla->h(absmod(hsla->h() + h, 360.0));
            if (nr_s) hsla->s(clamp(hsla->s() + s, 0.0, 100.0));
            if (nr_l) hsla->l(clamp(hsla->l() + l, 0.0, 100.0));
            if (nr_a) hsla->a(clamp(hsla->a() + a, 0.0, 1.0));
            return hsla.detach();
          }
          else if (hasHwb || nr_h) { // hue can be shared!
            ColorHwbaObj hwba = color->copyAsHWBA();
            if (nr_h) hwba->h(absmod(hwba->h() + h, 360.0));
            if (nr_wn) hwba->w(clamp(hwba->w() + wn, 0.0, 100.0));
            if (nr_bn) hwba->b(clamp(hwba->b() + bn, 0.0, 100.0));
            if (nr_a) hwba->a(clamp(hwba->a() + a, 0.0, 1.0));
            return hwba.detach();
          }
          else if (nr_a) {
            ColorObj copy = SASS_MEMORY_COPY(color);
            if (nr_a) copy->a(clamp(copy->a() + a, 0.0, 1.0));
            return copy.detach();
          }

        }

        return arguments[0];
      }

      static BUILT_IN_FN(change)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ArgumentList* argumentList = arguments[1]
          ->assertArgumentList(compiler, "kwargs");
        if (!argumentList->empty()) {
          SourceSpan span(color->pstate());
          CallStackFrame frame(compiler, BackTrace(
            span, Strings::colorChange));
          throw Exception::RuntimeException(compiler,
            "Only one positional argument is allowed. All "
            "other arguments must be passed by name.");
        }

        // ToDo: solve without erase ...
        ValueFlatMap* keywords(argumentList->keywords());

        Number* nr_r = getKwdNumber(keywords, key_red, compiler);
        Number* nr_g = getKwdNumber(keywords, key_green, compiler);
        Number* nr_b = getKwdNumber(keywords, key_blue, compiler);
        Number* nr_h = getKwdNumber(keywords, key_hue, compiler);
        Number* nr_s = getKwdNumber(keywords, key_saturation, compiler);
        Number* nr_l = getKwdNumber(keywords, key_lightness, compiler);
        Number* nr_a = getKwdNumber(keywords, key_alpha, compiler);
        Number* nr_wn = getKwdNumber(keywords, key_whiteness, compiler);
        Number* nr_bn = getKwdNumber(keywords, key_blackness, compiler);

        if (nr_h) checkAngle(compiler, nr_h, Strings::hue);

        double r = nr_r ? nr_r->assertRange(0.0, 255.0, unit_none, compiler, Strings::red) : 0.0;
        double g = nr_g ? nr_g->assertRange(0.0, 255.0, unit_none, compiler, Strings::green) : 0.0;
        double b = nr_b ? nr_b->assertRange(0.0, 255.0, unit_none, compiler, Strings::blue) : 0.0;
        double s = nr_s ? nr_s->checkPercent(compiler, Strings::saturation)->assertRange(0.0, 100.0, unit_percent, compiler, Strings::saturation) : 0.0;
        double l = nr_l ? nr_l->checkPercent(compiler, Strings::lightness)->assertRange(0.0, 100.0, unit_percent, compiler, Strings::lightness) : 0.0;
        double a = nr_a ? nr_a->assertRange(0.0, 1.0, nr_a, compiler, Strings::alpha) : 0.0;
        double wn = nr_wn ? nr_wn->assertHasUnits(compiler, Strings::percent, Strings::whiteness)->assertRange(0.0, 100.0, nr_wn, compiler, Strings::whiteness) : 0.0;
        double bn = nr_bn ? nr_bn->assertHasUnits(compiler, Strings::percent, Strings::blackness)->assertRange( 0.0, 100.0, nr_bn, compiler, Strings::blackness) : 0.0;
        double h = nr_h ? coerceToDeg(nr_h) : 0.0; // Hue is a very special case

        if (keywords && !keywords->empty()) {
          throw Exception::UnknownNamedArgument(compiler, keywords);
        }

        bool hasRgb = nr_r != nullptr || nr_g != nullptr || nr_b != nullptr;
        bool hasHsl = nr_s != nullptr || nr_l != nullptr;
        bool hasHwb = nr_wn != nullptr || nr_bn != nullptr;
        bool hasHue = nr_h != nullptr;

        if (hasRgb && hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL", "HWB" });
        else if (hasRgb && hasHue) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL/HWB" });
        else if (hasRgb && hasHsl) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL" });
        else if (hasRgb && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HWB" });
        else if (hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });
        else if (hasHwb && hasHsl) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });

        if (hasRgb) {
          ColorRgbaObj rgba = color->copyAsRGBA();
          if (nr_r) rgba->r(clamp(r, 0.0, 255.0));
          if (nr_g) rgba->g(clamp(g, 0.0, 255.0));
          if (nr_b) rgba->b(clamp(b, 0.0, 255.0));
          if (nr_a) rgba->a(clamp(a, 0.0, 1.0));
          return rgba.detach();
        }
        else if (hasHsl) {
          ColorHslaObj hsla = color->copyAsHSLA();
          if (nr_h) hsla->h(absmod(h, 360.0));
          if (nr_s) hsla->s(clamp(s, 0.0, 100.0));
          if (nr_l) hsla->l(clamp(l, 0.0, 100.0));
          if (nr_a) hsla->a(clamp(a, 0.0, 1.0));
          return hsla.detach();
        }
        else if (hasHwb || nr_h) { // hue can be shared!
          ColorHwbaObj hwba = color->copyAsHWBA();
          if (nr_h) hwba->h(absmod(h, 360.0));
          if (nr_wn) hwba->w(clamp(wn, 0.0, 100.0));
          if (nr_bn) hwba->b(clamp(bn, 0.0, 100.0));
          if (nr_a) hwba->a(clamp(a, 0.0, 1.0));
          return hwba.detach();
        }
        else if (nr_a) {
          ColorObj copy = SASS_MEMORY_COPY(color);
          if (nr_a) copy->a(clamp(a, 0.0, 1.0));
          return copy.detach();
        }
        return arguments[0];
      }

      static BUILT_IN_FN(scale)
      {
        const Color* color = arguments[0]->assertColor(compiler, Strings::color);
        ArgumentList* argumentList = arguments[1]
          ->assertArgumentList(compiler, "kwargs");
        if (!argumentList->empty()) {
          SourceSpan span(color->pstate());
          CallStackFrame frame(compiler, BackTrace(
            span, Strings::scaleColor));
          throw Exception::RuntimeException(compiler,
            "Only one positional argument is allowed. All "
            "other arguments must be passed by name.");
        }

        // ToDo: solve without erase ...
        ValueFlatMap* keywords(argumentList->keywords());

        Number* nr_r = getKwdNumber(keywords, key_red, compiler);
        Number* nr_g = getKwdNumber(keywords, key_green, compiler);
        Number* nr_b = getKwdNumber(keywords, key_blue, compiler);
        Number* nr_s = getKwdNumber(keywords, key_saturation, compiler);
        Number* nr_l = getKwdNumber(keywords, key_lightness, compiler);
        Number* nr_wn = getKwdNumber(keywords, key_whiteness, compiler);
        Number* nr_bn = getKwdNumber(keywords, key_blackness, compiler);
        Number* nr_a = getKwdNumber(keywords, key_alpha, compiler);

        double r = nr_r ? nr_r->assertHasUnits(compiler, Strings::percent, Strings::red)->assertRange(-100.0, 100.0, nr_r, compiler, Strings::red) / 100.0 : 0.0;
        double g = nr_g ? nr_g->assertHasUnits(compiler, Strings::percent, Strings::green)->assertRange(-100.0, 100.0, nr_g, compiler, Strings::green) / 100.0 : 0.0;
        double b = nr_b ? nr_b->assertHasUnits(compiler, Strings::percent, Strings::blue)->assertRange(-100.0, 100.0, nr_b, compiler, Strings::blue) / 100.0 : 0.0;
        double s = nr_s ? nr_s->assertHasUnits(compiler, Strings::percent, Strings::saturation)->assertRange(-100.0, 100.0, nr_s, compiler, Strings::saturation) / 100.0 : 0.0;
        double l = nr_l ? nr_l->assertHasUnits(compiler, Strings::percent, Strings::lightness)->assertRange(-100.0, 100.0, nr_l, compiler, Strings::lightness) / 100.0 : 0.0;
        double wn = nr_wn ? nr_wn->assertHasUnits(compiler, Strings::percent, Strings::whiteness)->assertRange(-100.0, 100.0, nr_wn, compiler, Strings::whiteness) / 100.0 : 0.0;
        double bn = nr_bn ? nr_bn->assertHasUnits(compiler, Strings::percent, Strings::blackness)->assertRange(-100.0, 100.0, nr_bn, compiler, Strings::blackness) / 100.0 : 0.0;
        double a = nr_a ? nr_a->assertHasUnits(compiler, Strings::percent, Strings::alpha)->assertRange(-100.0, 100.0, nr_a, compiler, Strings::alpha) / 100.0 : 0.0;

        if (keywords && !keywords->empty()) {
          throw Exception::UnknownNamedArgument(compiler, keywords);
        }

        bool hasRgb = nr_r || nr_g || nr_b;
        bool hasHsl = nr_s || nr_l;
        bool hasHwb = nr_wn || nr_bn;

        if (hasRgb && hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL", "HWB" });
        else if (hasRgb && hasHsl) throw Exception::MixedParamGroups(compiler, "RGB", { "HSL" });
        else if (hasRgb && hasHwb) throw Exception::MixedParamGroups(compiler, "RGB", { "HWB" });
        else if (hasHsl && hasHwb) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });
        else if (hasHwb && hasHsl) throw Exception::MixedParamGroups(compiler, "HSL", { "HWB" });

        if (hasRgb) {
          ColorRgbaObj rgba = color->copyAsRGBA();
          if (nr_r) rgba->r(scaleValue(rgba->r(), r, 255.0));
          if (nr_g) rgba->g(scaleValue(rgba->g(), g, 255.0));
          if (nr_b) rgba->b(scaleValue(rgba->b(), b, 255.0));
          if (nr_a) rgba->a(scaleValue(rgba->a(), a, 1.0));
          return rgba.detach();
        }
        else if (hasHsl) {
          ColorHslaObj hsla = color->copyAsHSLA();
          if (nr_s) hsla->s(scaleValue(hsla->s(), s, 100.0));
          if (nr_l) hsla->l(scaleValue(hsla->l(), l, 100.0));
          if (nr_a) hsla->a(scaleValue(hsla->a(), a, 1.0));
          return hsla.detach();
        }
        else if (hasHwb) { // hue can be shared!
          ColorHwbaObj hwba = color->copyAsHWBA();
          if (nr_wn) hwba->w(scaleValue(hwba->w(), wn, 100.0));
          if (nr_bn) hwba->b(scaleValue(hwba->b(), bn, 100.0));
          if (nr_a) hwba->a(scaleValue(hwba->a(), a, 1.0));
          return hwba.detach();
        }
        else if (nr_a) {
          ColorObj copy = SASS_MEMORY_COPY(color);
          if (nr_a) copy->a(scaleValue(copy->a(), a, 1.0));
          return copy.detach();
        }
        return arguments[0];
      }

      static BUILT_IN_FN(mix)
      {
        const Color* color1 = arguments[0]->assertColor(compiler, "color1");
        const Color* color2 = arguments[1]->assertColor(compiler, "color2");
        const Number* weight = arguments[2]->assertNumber(compiler, "weight");
        weight->checkPercent(compiler, Strings::weight);
        return mixColors(color1, color2, weight, pstate, compiler);
      }

      /*******************************************************************/

      void registerFunctions(Compiler& ctx)
      {

        // Some functions are stricter if called from module namespace
        uint32_t idx_rgb_strict = ctx.createBuiltInOverloadFns(key_rgb, {
          std::make_pair("$red, $green, $blue, $alpha", fnRgb4arg),
          std::make_pair("$red, $green, $blue", fnRgb3arg),
          std::make_pair("$color, $alpha", fnRgb2arg),
          std::make_pair("$channels", fnRgb1arg),
        });
        uint32_t idx_rgb_loose = ctx.createBuiltInOverloadFns(key_rgb, {
          std::make_pair("$red, $green, $blue, $alpha", rgb4arg),
          std::make_pair("$red, $green, $blue", rgb3arg),
          std::make_pair("$color, $alpha", rgb2arg),
          std::make_pair("$channels", rgb1arg),
          });
        uint32_t idx_rgba_strict = ctx.createBuiltInOverloadFns(key_rgba, {
          std::make_pair("$red, $green, $blue, $alpha", fnRgba4arg),
          std::make_pair("$red, $green, $blue", fnRgba3arg),
          std::make_pair("$color, $alpha", fnRgba2arg),
          std::make_pair("$channels", fnRgba1arg),
        });
        uint32_t idx_rgba_loose = ctx.createBuiltInOverloadFns(key_rgba, {
          std::make_pair("$red, $green, $blue, $alpha", rgba4arg),
          std::make_pair("$red, $green, $blue", rgba3arg),
          std::make_pair("$color, $alpha", rgba2arg),
          std::make_pair("$channels", rgba1arg),
        });
        uint32_t idx_hsl_strict = ctx.createBuiltInOverloadFns(key_hsl, {
          std::make_pair("$hue, $saturation, $lightness, $alpha", fnHsl4arg),
          std::make_pair("$hue, $saturation, $lightness", fnHsl3arg),
          std::make_pair("$hue, $saturation", fnHsl2arg),
          std::make_pair("$channels", fnHsl1arg),
        });
        uint32_t idx_hsl_loose = ctx.createBuiltInOverloadFns(key_hsl, {
          std::make_pair("$hue, $saturation, $lightness, $alpha", hsl4arg),
          std::make_pair("$hue, $saturation, $lightness", hsl3arg),
          std::make_pair("$hue, $saturation", hsl2arg),
          std::make_pair("$channels", hsl1arg),
        });
        uint32_t idx_hsla_strict = ctx.createBuiltInOverloadFns(key_hsla, {
          std::make_pair("$hue, $saturation, $lightness, $alpha", fnHsla4arg),
          std::make_pair("$hue, $saturation, $lightness", fnHsla3arg),
          std::make_pair("$hue, $saturation", fnHsla2arg),
          std::make_pair("$channels", fnHsla1arg),
        });
        uint32_t idx_hsla_loose = ctx.createBuiltInOverloadFns(key_hsla, {
          std::make_pair("$hue, $saturation, $lightness, $alpha", hsla4arg),
          std::make_pair("$hue, $saturation, $lightness", hsla3arg),
          std::make_pair("$hue, $saturation", hsla2arg),
          std::make_pair("$channels", hsla1arg),
        });
        uint32_t idx_hwb_strict = ctx.createBuiltInOverloadFns(key_hwb, {
          std::make_pair("$hue, $whiteness, $blackness, $alpha: 1", fnHwb4arg),
          // std::make_pair("$hue, $whiteness, $blackness", fnHwb3arg),
          // std::make_pair("$color, $alpha", fnHwb2arg),
          std::make_pair("$channels", fnHwb1arg),
        });

        uint32_t idx_hwb_loose = ctx.createBuiltInOverloadFns(key_hwb, {
          std::make_pair("$hue, $whiteness, $blackness, $alpha: 1", hwb4arg),
          // std::make_pair("$hue, $whiteness, $blackness", hwb3arg),
          // std::make_pair("$color, $alpha", hwb2arg),
          std::make_pair("$channels", hwb1arg),
        });

        uint32_t idx_oklab_strict = ctx.createBuiltInFunction(key_oklab, "$channels", oklab);
        uint32_t idx_oklch_strict = ctx.createBuiltInFunction(key_oklch, "$channels", oklch);
        uint32_t idx_lab_strict = ctx.createBuiltInFunction(key_lab, "$channels", lab);
        uint32_t idx_lch_strict = ctx.createBuiltInFunction(key_lch, "$channels", lch);

        // uint32_t idx_hwba_strict = ctx.createBuiltInOverloadFns(key_hwba, {
        //   std::make_pair("$hue, $whiteness, $blackness, $alpha", fnHwba4arg),
        //   std::make_pair("$hue, $whiteness, $blackness", fnHwba3arg),
        //   std::make_pair("$color, $alpha", fnHwba2arg),
        //   std::make_pair("$channels", fnHwba1arg),
        // });
        // uint32_t idx_hwba_loose = ctx.createBuiltInOverloadFns(key_hwba, {
        //   std::make_pair("$hue, $whiteness, $blackness, $alpha", hwba4arg),
        //   std::make_pair("$hue, $whiteness, $blackness", hwba3arg),
        //   std::make_pair("$color, $alpha", hwba2arg),
        //   std::make_pair("$channels", hwba1arg),
        // });

        uint32_t idx_color = ctx.createBuiltInFunction(key_color, "$description", color);
        uint32_t idx_is_legacy = ctx.createBuiltInFunction(key_is_legacy, "$color", isLegacy);
        uint32_t idx_is_in_gamut = ctx.createBuiltInFunction(key_is_in_gamut, "$color, $space: null", isInGamut);

        uint32_t idx_channel = ctx.createBuiltInFunction(key_channel, "$color, $channel, $space: null", channel);
        uint32_t idx_is_powerless = ctx.createBuiltInFunction(key_is_powerless, "$color, $channel, $space: null", isPowerless);

        uint32_t idx_space = ctx.createBuiltInFunction(key_space, "$color", space);
        uint32_t idx_is_missing = ctx.createBuiltInFunction(key_is_missing, "$color, $channel", isMissing);



        uint32_t idx_red = ctx.createBuiltInFunction(key_red, "$color", red);
        uint32_t idx_green = ctx.createBuiltInFunction(key_green, "$color", green);
        uint32_t idx_blue = ctx.createBuiltInFunction(key_blue, "$color", blue);
        uint32_t idx_hue = ctx.createBuiltInFunction(key_hue, "$color", hue);
        uint32_t idx_lightness = ctx.createBuiltInFunction(key_lightness, "$color", lightness);
        uint32_t idx_saturation = ctx.createBuiltInFunction(key_saturation, "$color", saturation);
        uint32_t idx_blackness = ctx.createBuiltInFunction(key_blackness, "$color", blackness);
        uint32_t idx_whiteness = ctx.createBuiltInFunction(key_whiteness, "$color", whiteness);
        uint32_t idx_invert_strict = ctx.createBuiltInFunction(key_invert, "$color, $weight: 100%", fnInvert);
        uint32_t idx_invert_loose = ctx.createBuiltInFunction(key_invert, "$color, $weight: 100%", invert);
        uint32_t idx_grayscale_strict = ctx.createBuiltInFunction(key_grayscale, "$color", noGrayscale);
        uint32_t idx_grayscale_loose = ctx.createBuiltInFunction(key_grayscale, "$color", grayscale);
        uint32_t idx_complement = ctx.createBuiltInFunction(key_complement, "$color", complement);
        uint32_t idx_desaturate_strict = ctx.createBuiltInFunction(key_desaturate, "$color, $amount", noDesaturate);
        uint32_t idx_desaturate_loose = ctx.createBuiltInFunction(key_desaturate, "$color, $amount", desaturate);
        uint32_t idx_saturate_strict = ctx.createBuiltInFunction(key_saturate, "$color, $amount", noSaturate);
        uint32_t idx_saturate_loose = ctx.createBuiltInOverloadFns(key_saturate, {
          std::make_pair("$amount", saturate1arg),
          std::make_pair("$color, $amount", saturate2arg),
          });
        uint32_t idx_lighten_strict = ctx.createBuiltInFunction(key_lighten, "$color, $amount", noLighten);
        uint32_t idx_lighten_loose = ctx.createBuiltInFunction(key_lighten, "$color, $amount", lighten);
        uint32_t idx_darken_strict = ctx.createBuiltInFunction(key_darken, "$color, $amount", noDarken);
        uint32_t idx_darken_loose = ctx.createBuiltInFunction(key_darken, "$color, $amount", darken);
        uint32_t idx_adjust_hue_strict = ctx.createBuiltInFunction(key_adjust_hue, "$color, $degrees", noAdjustHue);
        uint32_t idx_adjust_hue_loose = ctx.createBuiltInFunction(key_adjust_hue, "$color, $degrees", adjustHue);
        uint32_t idx_adjust = ctx.registerBuiltInFunction(key_adjust_color, "$color, $kwargs...", adjust);
        uint32_t idx_change = ctx.registerBuiltInFunction(key_change_color, "$color, $kwargs...", change);
        uint32_t idx_scale = ctx.registerBuiltInFunction(key_scale_color, "$color, $kwargs...", scale);
        uint32_t idx_mix = ctx.registerBuiltInFunction(key_mix, "$color1, $color2, $weight: 50%", mix);


        uint32_t idx_to_gamut = ctx.createBuiltInFunction(key_to_gamut, "$color, $space, $method", toGamut);

        uint32_t idx_opacify_strict = ctx.createBuiltInFunction(key_opacify, "$color, $amount", noOpacify);
        uint32_t idx_opacify_loose = ctx.createBuiltInFunction(key_opacify, "$color, $amount", opacify);
        uint32_t idx_fade_in_strict = ctx.createBuiltInFunction(key_fade_in, "$color, $amount", noFadeIn);
        uint32_t idx_fade_in_loose = ctx.createBuiltInFunction(key_fade_in, "$color, $amount", opacify);
        uint32_t idx_fade_out_strict = ctx.createBuiltInFunction(key_fade_out, "$color, $amount", noFadeOut);
        uint32_t idx_fade_out_loose = ctx.createBuiltInFunction(key_fade_out, "$color, $amount", transparentize);
        uint32_t idx_transparentize_strict = ctx.createBuiltInFunction(key_transparentize, "$color, $amount", noTansparentize);
        uint32_t idx_transparentize_loose = ctx.createBuiltInFunction(key_transparentize, "$color, $amount", transparentize);
        uint32_t idx_ie_hex_str = ctx.createBuiltInFunction(key_ie_hex_str, "$color", ieHexStr);
        uint32_t idx_alpha = ctx.createBuiltInOverloadFns(key_alpha, {
          // This does not give deprecations
          std::make_pair("$color", alphaOne),
          std::make_pair("$args...", alphaAny),
          });
        uint32_t idx_opacity_strict = ctx.createBuiltInFunction(key_opacity, "$color", noOpacity);
        uint32_t idx_opacity_loose = ctx.createBuiltInFunction(key_opacity, "$color", opacity);

        ctx.exposeFunction(key_rgb, idx_rgb_loose);
        ctx.exposeFunction(key_rgba, idx_rgba_loose);
        ctx.exposeFunction(key_hsl, idx_hsl_loose);
        ctx.exposeFunction(key_hsla, idx_hsla_loose);
        ctx.exposeFunction(key_hwb, idx_hwb_loose);

        ctx.exposeFunction(key_oklab, idx_oklab_strict);
        ctx.exposeFunction(key_oklch, idx_oklch_strict);
        ctx.exposeFunction(key_lab, idx_lab_strict);
        ctx.exposeFunction(key_lch, idx_lch_strict);

        // ctx.exposeFunction(key_hwba, idx_hwba_loose);
        ctx.exposeFunction(key_space, idx_space);
        ctx.exposeFunction(key_color, idx_color);
        ctx.exposeFunction(key_red, idx_red);
        ctx.exposeFunction(key_green, idx_green);
        ctx.exposeFunction(key_blue, idx_blue);
        ctx.exposeFunction(key_hue, idx_hue);
        ctx.exposeFunction(key_lightness, idx_lightness);
        ctx.exposeFunction(key_saturation, idx_saturation);
        ctx.exposeFunction(key_blackness, idx_blackness);
        ctx.exposeFunction(key_whiteness, idx_whiteness);
        ctx.exposeFunction(key_invert, idx_invert_loose);
        ctx.exposeFunction(key_grayscale, idx_grayscale_loose);
        ctx.exposeFunction(key_complement, idx_complement);
        ctx.exposeFunction(key_desaturate, idx_desaturate_loose);
        ctx.exposeFunction(key_saturate, idx_saturate_loose);
        ctx.exposeFunction(key_lighten, idx_lighten_loose);
        ctx.exposeFunction(key_darken, idx_darken_loose);
        ctx.exposeFunction(key_adjust_hue, idx_adjust_hue_loose);
        ctx.exposeFunction(key_adjust_color, idx_adjust);
        ctx.exposeFunction(key_change_color, idx_change);
        ctx.exposeFunction(key_scale_color, idx_scale);
        ctx.exposeFunction(key_mix, idx_mix);
        // ctx.exposeFunction(key_to_gamut, idx_to_gamut);
        ctx.exposeFunction(key_opacify, idx_opacify_loose);
        ctx.exposeFunction(key_fade_in, idx_fade_in_loose);
        ctx.exposeFunction(key_fade_out, idx_fade_out_loose);
        ctx.exposeFunction(key_transparentize, idx_transparentize_loose);
        ctx.exposeFunction(key_ie_hex_str, idx_ie_hex_str);
        ctx.exposeFunction(key_alpha, idx_alpha);
        ctx.exposeFunction(key_opacity, idx_opacity_loose);

        BuiltInMod& module(ctx.createModule("color"));

        module.addFunction(key_space, idx_space);
        module.addFunction(key_color, idx_color);
        module.addFunction(key_channel, idx_channel);
        module.addFunction(key_is_legacy, idx_is_legacy);
        module.addFunction(key_is_missing, idx_is_missing);
        module.addFunction(key_is_in_gamut, idx_is_in_gamut);
        module.addFunction(key_is_powerless, idx_is_powerless);


        module.addFunction(key_rgb, idx_rgb_strict);
        module.addFunction(key_rgba, idx_rgba_strict);
        module.addFunction(key_hsl, idx_hsl_strict);
        module.addFunction(key_hsla, idx_hsla_strict);
        module.addFunction(key_hwb, idx_hwb_strict);

        module.addFunction(key_oklab, idx_oklab_strict);
        module.addFunction(key_oklch, idx_oklch_strict);
        module.addFunction(key_lab, idx_lab_strict);
        module.addFunction(key_lch, idx_lch_strict);

        // module.addFunction(key_hwba, idx_hwba_strict);
        module.addFunction(key_red, idx_red);
        module.addFunction(key_green, idx_green);
        module.addFunction(key_blue, idx_blue);
        module.addFunction(key_hue, idx_hue);
        module.addFunction(key_lightness, idx_lightness);
        module.addFunction(key_saturation, idx_saturation);
        module.addFunction(key_blackness, idx_blackness);
        module.addFunction(key_whiteness, idx_whiteness);
        module.addFunction(key_invert, idx_invert_strict);
        module.addFunction(key_grayscale, idx_grayscale_strict);
        module.addFunction(key_complement, idx_complement);
        module.addFunction(key_desaturate, idx_desaturate_strict);
        module.addFunction(key_saturate, idx_saturate_strict);
        module.addFunction(key_lighten, idx_lighten_strict);
        module.addFunction(key_darken, idx_darken_strict);
        module.addFunction(key_adjust_hue, idx_adjust_hue_strict);
        module.addFunction(key_adjust, idx_adjust);
        module.addFunction(key_change, idx_change);
        module.addFunction(key_scale, idx_scale);
        module.addFunction(key_mix, idx_mix);
        module.addFunction(key_to_gamut, idx_to_gamut);
        module.addFunction(key_opacify, idx_opacify_strict);
        module.addFunction(key_fade_in, idx_fade_in_strict);
        module.addFunction(key_fade_out, idx_fade_out_strict);
        module.addFunction(key_transparentize, idx_transparentize_strict);
        module.addFunction(key_ie_hex_str, idx_ie_hex_str);
        module.addFunction(key_alpha, idx_alpha);
        module.addFunction(key_opacity, idx_opacity_strict);

      }

    }

    /*******************************************************************/


    Value* rgbFn2(const sass::string& name, const ValueVector& arguments, const SourceSpan& pstate, Logger& logger, bool strict)
    {

      Value* _r = arguments[0];
      Value* _g = arguments[1];
      Value* _b = arguments[2];
      Value* _a = nullptr;
      if (arguments.size() > 3) {
        _a = arguments[3];
      }
      // Check if any `calc()` or `var()` are passed
      if (!strict && (isSpecialNumber(_r) || isSpecialNumber(_g) || isSpecialNumber(_b) || isSpecialNumber(_a))) {
        sass::sstream fncall;
        fncall << name << "(";
        fncall << _r->inspect() << ", ";
        fncall << _g->inspect() << ", ";
        fncall << _b->inspect();
        if (_a) { fncall << ", " << _a->inspect(); }
        fncall << ")";
        return SASS_MEMORY_NEW(String, pstate, fncall.str());
      }

      Number* r = _r->assertNumber(logger, Strings::red);
      Number* g = _g->assertNumber(logger, Strings::green);
      Number* b = _b->assertNumber(logger, Strings::blue);
      Number* a = _a ? _a->assertNumber(logger, Strings::alpha) : nullptr;

      return SASS_MEMORY_NEW(ColorSpaced, pstate, ColorSpace::rgb,
        fuzzyRound(_percentageOrUnitless(r, 255, "$red", logger), logger.epsilon),
        fuzzyRound(_percentageOrUnitless(g, 255, "$green", logger), logger.epsilon),
        fuzzyRound(_percentageOrUnitless(b, 255, "$blue", logger), logger.epsilon),
        _a ? _percentageOrUnitless(a, 1.0, "$alpha", logger) : 1.0, "", true); // Hmmm

    }

    Value* rgbFn(const sass::string& name, const ValueVector& arguments, const SourceSpan& pstate, Logger& logger, bool strict)
    {
      Value* _r = arguments[0];
      Value* _g = arguments[1];
      Value* _b = arguments[2];
      Value* _a = nullptr;
      if (arguments.size() > 3) {
        _a = arguments[3];
      }
      // Check if any `calc()` or `var()` are passed
      if (!strict && (isSpecialNumber(_r) || isSpecialNumber(_g) || isSpecialNumber(_b) || isSpecialNumber(_a))) {
        sass::sstream fncall;
        fncall << name << "(";
        fncall << _r->inspect() << ", ";
        fncall << _g->inspect() << ", ";
        fncall << _b->inspect();
        if (_a) { fncall << ", " << _a->inspect(); }
        fncall << ")";
        return SASS_MEMORY_NEW(String, pstate, fncall.str());
      }

      Number* r = _r->assertNumber(logger, Strings::red);
      Number* g = _g->assertNumber(logger, Strings::green);
      Number* b = _b->assertNumber(logger, Strings::blue);

      Number* a_nr = _a ? _a->assertNumber(logger, Strings::alpha) : nullptr;
      double a_val = _a ? _percentageOrUnitless(a_nr, 1.0, "$alpha", logger) : 1.0;
      a_val = std::isnan(a_val) ? 0.0 : std::max(0.0, std::min(1.0, a_val));
      tl::optional<double> a = a_val;

      return _colorFromChannels(logger, pstate,
        &ColorSpace::rgb, r, g, b, a, true, true);

      // return SASS_MEMORY_NEW(ColorRgba, pstate,
      //   fuzzyRound(_percentageOrUnitless(r, 255, "$red", logger), logger.epsilon),
      //   fuzzyRound(_percentageOrUnitless(g, 255, "$green", logger), logger.epsilon),
      //   fuzzyRound(_percentageOrUnitless(b, 255, "$blue", logger), logger.epsilon),
      //   _a ? _percentageOrUnitless(a, 1.0, "$alpha", logger) : 1.0, "", true); // Hmmm

    }



    /*******************************************************************/

    Value* hwbFn(const sass::string& name, const ValueVector& arguments, const SourceSpan& pstate, Logger& logger, bool strict)
    {
      Value* _h = arguments[0];
      Value* _w = arguments[1];
      Value* _b = arguments[2];
      Value* _a = nullptr;
      if (arguments.size() > 3) {
        _a = arguments[3];
      }
      // Check if any `calc()` or `var()` are passed
      if (!strict && (isSpecialNumber(_h) || isSpecialNumber(_w) || isSpecialNumber(_b) || isSpecialNumber(_a))) {
        sass::sstream fncall;
        fncall << name << "(";
        fncall << _h->inspect() << ", ";
        fncall << _w->inspect() << ", ";
        fncall << _b->inspect();
        if (_a) { fncall << ", " << _a->inspect(); }
        fncall << ")";
        return SASS_MEMORY_NEW(String, pstate, fncall.str());
      }

      Number* h = _h->assertNumber(logger, Strings::hue);
      Number* w = _w->assertNumber(logger, Strings::whiteness)
        ->assertHasUnits(logger, "%", Strings::whiteness);
      Number* b = _b->assertNumber(logger, Strings::blackness)
        ->assertHasUnits(logger, "%", Strings::blackness);
      Number* a = _a ? _a->assertNumber(logger, Strings::alpha) : nullptr;

      checkAngle(logger, h, Strings::hue);
      return SASS_MEMORY_NEW(ColorHwba, pstate,
        coerceToDeg(h),
        w->assertRange(0.0, 100.0, w, logger, Strings::whiteness),
        b->assertRange(0.0, 100.0, b, logger, Strings::blackness),
        _a ? _percentageOrUnitless(a, 1.0, "$alpha", logger) : 1.0, "", true);

    }

    /*******************************************************************/

    Value* hslFn(const sass::string& name, const ValueVector& arguments, const SourceSpan& pstate, Logger& logger, bool strict)
    {
      Value* _h = arguments[0];
      Value* _s = arguments[1];
      Value* _l = arguments[2];
      Value* _a = nullptr;
      if (arguments.size() > 3) {
        _a = arguments[3];
      }
      // Check if any `calc()` or `var()` are passed
      if (!strict && (isSpecialNumber(_h) || isSpecialNumber(_s) || isSpecialNumber(_l) || isSpecialNumber(_a))) {
        sass::sstream fncall;
        fncall << name << "(";
        fncall << _h->inspect() << ", ";
        fncall << _s->inspect() << ", ";
        fncall << _l->inspect();
        if (_a) { fncall << ", " << _a->inspect(); }
        fncall << ")";
        return SASS_MEMORY_NEW(String, pstate, fncall.str());
      }

      Number* h = _h->assertNumber(logger, Strings::hue);
      Number* s = _s->assertNumber(logger, Strings::saturation);
      Number* l = _l->assertNumber(logger, Strings::lightness);
      Number* a = _a ? _a->assertNumber(logger, Strings::alpha) : nullptr;

      checkAngle(logger, h, Strings::hue);
      s->checkPercent(logger, Strings::saturation);
      l->checkPercent(logger, Strings::lightness);
      return SASS_MEMORY_NEW(ColorHsla, pstate,
        coerceToDeg(h),
        clamp(s->value(), 0.0, 100.0),
        clamp(l->value(), 0.0, 100.0),
        _a ? _percentageOrUnitless(a, 1.0, "$alpha", logger) : 1.0, "", true);
    }

    /*******************************************************************/

  }

}
