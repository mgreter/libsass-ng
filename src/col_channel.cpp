/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "col_channel.hpp"

#include "ast_colors.hpp"

namespace Sass {

  const ClipGamutMap GamutMapMethod::clip;
  const LocalMindeGamutMap GamutMapMethod::localMinde;


  static double clampLikeCss(double val, double min, double max) {
    return std::isnan(val) ? min : std::min(std::max(val, min), max);
  }

  static tl::optional<double> _clampChannel(tl::optional<double> value, const ColorChannel& channel)
  {
    if (value.has_value() == false) return value;
    if (channel.isLinear == false) return value;
    return clampLikeCss(value.value(), channel.min, channel.max);
  }


  Color* ClipGamutMap::map(Color* color) const
  {
    // std::cerr << "clip gammut " << color->debug() << "\n";
    return Color::forSpaceInternal(
      color->pstate(), color->space(),
      _clampChannel(color->getChannel0OrNull(), color->space()._channels[0]),
      _clampChannel(color->getChannel1OrNull(), color->space()._channels[1]),
      _clampChannel(color->getChannel2OrNull(), color->space()._channels[2]),
      color->getAlphaOrNull());
  }

  const GamutMapMethod& GamutMapMethod::fromName(Logger& logger,
    Value* value, const sass::string& vname)
  {
    if (value == nullptr) return GamutMapMethod::clip;
    if (value->isNull()) return GamutMapMethod::clip;
    String* mname = value->assertString(logger, vname);
    mname->assertUnquoted(logger, vname);
    if (mname->value() == "local-minde") return localMinde;
    else if (mname->value() == "clip") return clip;
    throw Exception::SassScriptException(logger, value->pstate(),
      "Unknown gamut map method \"" + mname->value() + "\".", vname);
  }

  /// Returns the ΔEOK measure between [color1] and [color2].
  double _deltaEOK(Color* color1, Color* color2) {
    // Algorithm from https://www.w3.org/TR/css-color-4/#color-difference-OK
    ColorObj lab1 = color1->toSpace(ColorSpaces::oklab, color1->pstate());
    ColorObj lab2 = color2->toSpace(ColorSpaces::oklab, color2->pstate());

    return std::sqrt(
      std::pow(lab1->getChannel0() - lab2->getChannel0(), 2) +
      std::pow(lab1->getChannel1() - lab2->getChannel1(), 2) +
      std::pow(lab1->getChannel2() - lab2->getChannel2(), 2));
  }


  Color* LocalMindeGamutMap::map(Color* color) const
  {

    /// A constant from the gamut-mapping algorithm.
    double _jnd = 0.02;

    /// A constant from the gamut-mapping algorithm.
    double _epsilon = 0.0001;


    // Algorithm from https://www.w3.org/TR/2022/CRD-css-color-4-20221101/#css-gamut-mapping-algorithm
    ColorObj originOklch = color->toSpace(ColorSpaces::oklch, color->pstate());

    // The channel equivalents to `current` in the Color 4 algorithm.
    tl::optional<double> lightness = originOklch->getChannel0OrNull();
    tl::optional<double> hue = originOklch->getChannel2OrNull();
    tl::optional<double> alpha = originOklch->getAlphaOrNull();

    if (fuzzyGreaterThanOrEquals(lightness.value_or(0), 1.0, sass::epsilon)) {
      if (color->isLegacy()) {
        ColorObj rv = Color::rgb(
          color->pstate(), 255, 255, 255, color->getAlphaOrNull());
        rv = rv->toSpace(color->space(), color->pstate());
        return rv.detach();
      }
      return Color::forSpaceInternal(
        color->pstate(), color->space(),
        1, 1, 1, color->getAlphaOrNull());
    }
    else if (fuzzyLessThanOrEquals(lightness.value_or(0), 0.0, sass::epsilon)) {
      ColorObj rv = Color::rgb(
        color->pstate(),
        0, 0, 0, color->getAlphaOrNull());
      rv = rv->toSpace(color->space(), color->pstate());
      return rv.detach();
    }

    ColorObj clipped = color->toGamut(GamutMapMethod::clip);

    if (_deltaEOK(clipped, color) < _jnd) return clipped.detach();

    double min = 0.0;
    double max = originOklch->getChannel1();
    bool minInGamut = true;

    while (max - min > _epsilon) {

      double chroma = (min + max) / 2.0;

      // In the Color 4 algorithm `current` is in Oklch, but all its actual uses
      // other than modifying chroma convert it to `color.space` first so we
      // just store it in that space to begin with.
      ColorObj current = ColorSpaces::oklch.convert(
        color->space(),
        color->pstate(),
        lightness,
        chroma,
        hue,
        alpha);

      // Per [this comment], the intention of the algorithm is to fall through
      // this clause if `minInGamut = false` without checking
      // `current.isInGamut` at all, even though that's unclear from the
      // pseudocode. `minInGamut = false` *should* imply `current.isInGamut =
      // false`.
      //
      // [this comment]: https://github.com/w3c/csswg-drafts/issues/10226#issuecomment-2065534713
      if (minInGamut && current->isInGamut()) {
        min = chroma;
        continue;
      }

      clipped = current->toGamut(GamutMapMethod::clip);
      double e = _deltaEOK(clipped, current);
      if (e < _jnd) {
        if (_jnd - e < _epsilon) return clipped.detach();
        minInGamut = false;
        min = chroma;
      }
      else {
        max = chroma;
      }
    }
    return clipped.detach();

  }

  InterpolationMethod InterpolationMethod::fromValue(Logger& logger, Value* value, const sass::string& name)
  {
    auto list = value->assertCommonListStyle(logger, name, false);

    if (list.empty()) {
      throw Exception::SassScriptException(logger, value->pstate(),
        "Expected a color interpolation method, got an empty list.",
        name);
    }

    const ColorSpace& space = ColorSpace::fromValueRef(logger, list[0], name);

    if (list.size() == 1) return InterpolationMethod(space);

    auto hueMethod = InterpolationMethod::hueFromValue(logger, list[1], name);

    if (list.size() == 2) {
      throw Exception::SassScriptException(logger, value->pstate(),
        "Expected unquoted string \"hue\" after " + value->toString() + ".",
        name);
    }
    else {
      auto str = list[2]->assertString(logger, name);
      str->assertUnquoted(logger, name);
      if (!StringUtils::equalsIgnoreCase(str->value(), "hue", 3)) {
        throw Exception::SassScriptException(logger, value->pstate(),
          "Expected unquoted string \"hue\" at the end of "
          + value->toString() + ", was " + list[2]->toString() + ".",
          name);
      }
      if (list.size() > 3) {
        throw Exception::SassScriptException(logger, value->pstate(),
          "Expected nothing after \"hue\" in " + value->toString() + ".",
          name);
      }
      if (!space.isPolar()) {
        throw Exception::SassScriptException(logger, value->pstate(),
          "Hue interpolation method \"" + list[1]->toString() + " hue\" may not be"
          " set for rectangular color space " + space.name() + ".",
          name);
      }
    }

    return InterpolationMethod(space, hueMethod);

  }

  HueInterpolationMethod InterpolationMethod::hueFromValue(
    Logger& logger, Value* value, const sass::string& name)
  {

    const String* string = value->assertString(logger, name);
    string->assertUnquoted(logger, name);

    if (StringUtils::equalsIgnoreCase(string->value(), "shorter", 7)) {
      return HueInterpolationMethod::shorter;
    }
    if (StringUtils::equalsIgnoreCase(string->value(), "longer", 6)) {
      return HueInterpolationMethod::longer;
    }
    if (StringUtils::equalsIgnoreCase(string->value(), "increasing", 10)) {
      return HueInterpolationMethod::increasing;
    }
    if (StringUtils::equalsIgnoreCase(string->value(), "decreasing", 10)) {
      return HueInterpolationMethod::decreasing;
    }

    throw Exception::SassScriptException(logger, value->pstate(),
      "Unknown hue interpolation method " + value->toCss() + ".",
      name);
  }


}
