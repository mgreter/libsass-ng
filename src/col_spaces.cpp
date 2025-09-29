/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "col_spaces.hpp"

#include "ast_colors.hpp"

namespace Sass {

  /// A constant used to convert Lab to/from XYZ.
  const double labKappa = 24389.0 / 27.0; // 29^3/3^3;

  /// A constant used to convert Lab to/from XYZ.
  const double labEpsilon = 216.0 / 24389.0; // 6^3/29^3;

  const double d50[] = { 0.3457 / 0.3585, 1.00000, (1.0 - 0.3457 - 0.3585) / 0.3585 };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // const std::map<sass::string, SassColorSpace,
  //   StringLessThanInsensitive> cname_to_enum
  // const std::unordered_map<const sass::string, SassColorSpace,
  //   StringHashInsensitive, StringEqualityInsensitive> cname_to_enum
  // {
  //   { "rgb", SassColorSpace::RGB },
  //   { "hwb", SassColorSpace::HWB },
  //   { "hsl", SassColorSpace::HSL },
  //   { "srgb", SassColorSpace::SRGB },
  //   { "srgb-linear", SassColorSpace::SRGB_LINEAR },
  //   { "display-p3", SassColorSpace::DISPLAY_P3 },
  //   { "a98-rgb", SassColorSpace::A98RGB },
  //   { "prophoto-rgb", SassColorSpace::PROPHOTO_RGB },
  //   { "rec2020", SassColorSpace::REC2020 },
  //   { "xyz-d65", SassColorSpace::XYZ_D65 },
  //   { "xyz", SassColorSpace::XYZ_D50 },
  //   { "xyz-d50", SassColorSpace::XYZ_D50 },
  //   { "lab", SassColorSpace::LAB },
  //   { "lch", SassColorSpace::LCH },
  //   { "oklab", SassColorSpace::OKLAB },
  //   { "oklch", SassColorSpace::OKLCH }
  // };

  // const std::map<
  //   sass::string, std::reference_wrapper<const ColorSpace>,
  //   StringLessThanInsensitive> cname_to_space
  const std::unordered_map<const sass::string, std::reference_wrapper<const ColorSpace>,
    StringHashInsensitive, StringEqualityInsensitive> cname_to_space
  {
    { "rgb", ColorSpaces::rgb },
    { "hwb", ColorSpaces::hwb },
    { "hsl", ColorSpaces::hsl },
    { "srgb", ColorSpaces::srgb },
    { "srgb-linear", ColorSpaces::srgb_linear },
    { "display-p3", ColorSpaces::displayP3 },
    { "a98-rgb", ColorSpaces::a98rgb },
    { "prophoto-rgb", ColorSpaces::protophotoRgb },
    { "rec2020", ColorSpaces::rec2020 },
    { "xyz-d65", ColorSpaces::xyzd65 },
    { "xyz", ColorSpaces::xyzd65 },
    { "xyz-d50", ColorSpaces::xyzd50 },
    { "lab", ColorSpaces::lab },
    { "lch", ColorSpaces::lch },
    { "oklab", ColorSpaces::oklab },
    { "oklch", ColorSpaces::oklch }
  };

  const ColorSpace& ColorSpace::fromNameRef(Logger& logger, const String& space, const sass::string& name)
  {
    auto cs = cname_to_space.find(space.value());
    if (cs != cname_to_space.end()) return cs->second;
    throw Exception::UnknownColorSpace(logger, space, name);

    // if (StringUtils::equalsIgnoreCase(space.value(), "rgb")) return ColorSpaces::rgb;
    // if (StringUtils::equalsIgnoreCase(space.value(), "hwb")) return ColorSpaces::hwb;
    // if (StringUtils::equalsIgnoreCase(space.value(), "hsl")) return ColorSpaces::hsl;
    // if (StringUtils::equalsIgnoreCase(space.value(), "srgb")) return ColorSpaces::srgb;
    // if (StringUtils::equalsIgnoreCase(space.value(), "srgb-linear")) return ColorSpaces::srgb_linear;
    // if (StringUtils::equalsIgnoreCase(space.value(), "display-p3")) return ColorSpaces::displayP3;
    // if (StringUtils::equalsIgnoreCase(space.value(), "a98-rgb")) return ColorSpaces::a98rgb;
    // if (StringUtils::equalsIgnoreCase(space.value(), "prophoto-rgb")) return ColorSpaces::protophotoRgb;
    // if (StringUtils::equalsIgnoreCase(space.value(), "rec2020")) return ColorSpaces::rec2020;
    // if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d65")) return ColorSpaces::xyzd65;
    // if (StringUtils::equalsIgnoreCase(space.value(), "xyz")) return ColorSpaces::xyzd65;
    // if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d50")) return ColorSpaces::xyzd50;
    // if (StringUtils::equalsIgnoreCase(space.value(), "lab")) return ColorSpaces::lab;
    // if (StringUtils::equalsIgnoreCase(space.value(), "lch")) return ColorSpaces::lch;
    // if (StringUtils::equalsIgnoreCase(space.value(), "oklab")) return ColorSpaces::oklab;
    // if (StringUtils::equalsIgnoreCase(space.value(), "oklch")) return ColorSpaces::oklch;

  }

  const ColorSpace& ColorSpace::fromValueRef(Logger& logger, Value* value, const sass::string& name)
  {
    String* space_str = value->assertString(logger, name);
    space_str->assertUnquoted(logger, name); // may throw
    return fromNameRef(logger, *space_str, name);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static const ColorSpace& getLinearDest(const ColorSpace& dest)
  {
    if (dest == ColorSpaces::hsl || dest == ColorSpaces::hwb) { return ColorSpaces::srgb; }
    else if (dest == ColorSpaces::lab || dest == ColorSpaces::lch) { return ColorSpaces::xyzd50; }
    else if (dest == ColorSpaces::oklab || dest == ColorSpaces::oklch) { return ColorSpaces::lms; }
    return dest;
  }

  Color* ColorSpace::convertLinear(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> red,
    tl::optional<double> green,
    tl::optional<double> blue,
    tl::optional<double> alpha,
    bool missingLightness,
    bool missingChroma,
    bool missingHue,
    bool missingA,
    bool missingB) const
  {


    const ColorSpace& linearDest = getLinearDest(dest);

    // if (dest.name_ == "hsl" || dest.name_ == "hwb") { linearDest = ColorSpaces::srgb; }
    // else if (dest.name_ == "lab" || dest.name_ == "lch") { linearDest = ColorSpaces::xyzd50; }
    // else if (dest.name_ == "oklab" || dest.name_ == "oklch") { linearDest = ColorSpaces::lms; }

    tl::optional<double> transformedRed;
    tl::optional<double> transformedGreen;
    tl::optional<double> transformedBlue;

    if (linearDest.name_ == name_) {
      // std::cerr << "Do no linear conversion from " << name_ << " to " << linearDest.name_ << "\n";
      transformedRed = red;
      transformedGreen = green;
      transformedBlue = blue;
    }
    else {


      // std::cerr << "Do linear conversion from " << name_ << " to " << linearDest.name_ << "\n";
      // std::cerr << "Input " << red.value_or(0) << ", " << green.value_or(0) << ", " << blue.value_or(0) << "\n";

      double linearRed = toLinear(red.value_or(0));
      double linearGreen = toLinear(green.value_or(0));
      double linearBlue = toLinear(blue.value_or(0));
      const double* matrix = this->transformationMatrix(linearDest);

      // std::cerr << "Do conversion " << linearDest.name() << " => " << linearRed << ", " << linearGreen << ", " << linearBlue << "\n";
      // std::cerr << "Matrix " << matrix[0] << ", " << matrix[1] << ", " << matrix[2] << ", " << matrix[3] << "\n";
      transformedRed = linearDest.fromLinear(
        matrix[0] * linearRed +
        matrix[1] * linearGreen +
        matrix[2] * linearBlue);
      transformedGreen = linearDest.fromLinear(
        matrix[3] * linearRed +
        matrix[4] * linearGreen +
        matrix[5] * linearBlue);
      transformedBlue = linearDest.fromLinear(
        matrix[6] * linearRed +
        matrix[7] * linearGreen +
        matrix[8] * linearBlue);
    }

    //std::cerr << "default conversion "
    //  << transformedRed.value_or(-42) << ", "
    //  << transformedGreen.value_or(-42) << ", "
    //  << transformedBlue.value_or(-42) << "\n";

    if (dest == ColorSpaces::hsl || dest == ColorSpaces::hwb) {
      return ColorSpaces::srgb.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }
    else if (dest == ColorSpaces::lab || dest == ColorSpaces::lch) {
      return ColorSpaces::xyzd50.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }
    else if (dest.name() == "oklab" || dest.name() == "oklch") {
      return ColorSpaces::lms.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }

    return Color::forSpaceInternal(
      pstate, dest,
      red.has_value() ? transformedRed : red,
      green.has_value() ? transformedGreen : green,
      blue.has_value() ? transformedBlue : blue,
      alpha);
    // auto rv = Color::_forSpace(pstate, dest, 1, 1, 1, 1, )

    // return Color::forSpaceInternal(;
  }


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* SrgbColorSpace::translate(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> red,
    tl::optional<double> green,
    tl::optional<double> blue,
    tl::optional<double> alpha,
    bool missingLightness,
    bool missingChroma,
    bool missingHue) const
  {

    if (dest == ColorSpaces::hsl || dest == ColorSpaces::hwb)
    {

      double nr_red = red.value_or(0);
      double nr_green = green.value_or(0);
      double nr_blue = blue.value_or(0);
      double max = sass::max(sass::max(nr_red, nr_green), nr_blue);
      double min = sass::min(sass::min(nr_red, nr_green), nr_blue);
      double delta = max - min;

      double hue;
      if (max == min) {
        hue = 0;
      }
      else if (max == nr_red) {
        hue = 60 * (nr_green - nr_blue) / delta + 360;
      }
      else if (max == green) {
        hue = 60 * (nr_blue - nr_red) / delta + 120;
      }
      else {
        hue = 60 * (nr_red - nr_green) / delta + 240;
      }

      if (dest == ColorSpaces::hsl) {
        double lightness = (min + max) / 2;
        double saturation = lightness == 0 || lightness == 1
          ? 0.0
          : 100 * (max - lightness) / std::min(lightness, 1 - lightness);
        if (saturation < 0) {
          hue += 180;
          saturation = std::abs(saturation);
        }

        tl::optional<double> c0;
        if (!(missingHue || fuzzyEquals(saturation, 0.0, sass::epsilon))) {
          c0 = std::fmod(hue, 360.0);
        }
        tl::optional<double> c1;
        if (!missingChroma) c1 = saturation;
        tl::optional<double> c2;
        if (!missingLightness) c2 = lightness * 100;

        return Color::forSpaceInternal(
          pstate, dest, c0, c1, c2, alpha);

      }
      else {
        double whiteness = min * 100;
        double blackness = 100 - max * 100;
        auto rv = Color::forSpaceInternal(
          pstate, dest,
          missingHue || fuzzyGreaterThanOrEquals(whiteness + blackness, 100, 0.00001)
          ? tl::optional<double>()
          : std::fmod(hue, 360),
          whiteness,
          blackness,
          alpha);
        return rv;

      }
      return SASS_MEMORY_NEW(Color, pstate, ColorSpaces::rgb, 1, 1, 1, 1);
    }
    else if (dest == ColorSpaces::rgb) {
      return Color::rgb(pstate,
        red.has_value() ? red.value() * 255.0 : red,
        green.has_value() ? green.value() * 255.0 : green,
        blue.has_value() ? blue.value() * 255.0 : blue,
        alpha);
    }
    else if (dest == ColorSpaces::srgb_linear) {
      return Color::forSpaceInternal(pstate, dest,
        red.has_value() ? toLinear(red.value()) : red,
        green.has_value() ? toLinear(green.value()) : green,
        blue.has_value() ? toLinear(blue.value()) : blue,
        alpha);
    }
    else {
      return ColorSpace::convertLinear(dest,
        pstate, red, green, blue, alpha,
        missingLightness, missingChroma, missingHue);
    }
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* RgbColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> channel0,
    tl::optional<double> channel1,
    tl::optional<double> channel2,
    tl::optional<double> alpha) const
  {
    return ColorSpaces::srgb.translate(dest, pstate,
      channel0.has_value() ? channel0.value() / 255.0 : channel0,
      channel1.has_value() ? channel1.value() / 255.0 : channel1,
      channel2.has_value() ? channel2.value() / 255.0 : channel2,
      alpha);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


  /// Does a partial conversion of a single XYZ component to Lab.
  double _convertComponentToLabF(double component) {

    if (component > labEpsilon)
      return std::pow(component, 1.0 / 3.0) + 0.0;
    return (labKappa * component + 16.0) / 116.0;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* XyzD50ColorSpace::translate(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> x,
    tl::optional<double> y,
    tl::optional<double> z,
    tl::optional<double> alpha,
    bool missingLightness,
    bool missingChroma,
    bool missingHue,
    bool missingA,
    bool missingB) const
  {

    if (dest == ColorSpaces::lab || dest == ColorSpaces::lch) {
      // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
      // and http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
      double f0 = _convertComponentToLabF(x.value_or(0) / d50[0]);
      double f1 = _convertComponentToLabF(y.value_or(0) / d50[1]);
      double f2 = _convertComponentToLabF(z.value_or(0) / d50[2]);

      tl::optional<double> lightness;
      double a = 500 * (f0 - f1);
      double b = 200 * (f1 - f2);
      if (!missingLightness) { lightness = (116.0 * f1) - 16.0; }

      if (dest == ColorSpaces::lab) {
        auto rv = Color::lab(pstate, lightness,
          missingA ? tl::optional<double>() : a,
          missingB ? tl::optional<double>() : b,
          alpha);
        // std::cerr << "==lab== " << rv->debug() << "\n";
        return rv;
      }
      else {
        auto rv = Color::labToLch(pstate,
          ColorSpaces::lch, lightness, a, b, alpha,
          missingChroma, missingHue);
        // std::cerr << "==lch== " << rv->debug() << "\n";
        return rv;
      }
    }

    return ColorSpace::convertLinear(
      dest, pstate,
      x, y, z, alpha,
      missingLightness,
      missingChroma,
      missingHue,
      missingA,
      missingB);

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  double _cubeRootPreservingSign(double number)
  {
    return (std::signbit(number) ? -1.0 : +1.0)
      * std::pow(std::abs(number), 1.0 / 3.0);
  }

  Color* LmsColorSpace::translate(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lng,
    tl::optional<double> med,
    tl::optional<double> shrt,
    tl::optional<double> alpha,
    bool missingLightness,
    bool missingChroma,
    bool missingHue,
    bool missingA,
    bool missingB) const
  {

    if (dest == ColorSpaces::oklab) {
      // Algorithm from https://drafts.csswg.org/css-color-4/#color-conversion-code
      double longScaled = _cubeRootPreservingSign(lng.value_or(0));
      double mediumScaled = _cubeRootPreservingSign(med.value_or(0));
      double shortScaled = _cubeRootPreservingSign(shrt.value_or(0));

      return Color::oklab(pstate, dest,
        missingLightness ? tl::optional<double>() :
        ColorSpaces::lmsToOklab[0] * longScaled
        + ColorSpaces::lmsToOklab[1] * mediumScaled
        + ColorSpaces::lmsToOklab[2] * shortScaled,
        missingA ? tl::optional<double>() :
        ColorSpaces::lmsToOklab[3] * longScaled
        + ColorSpaces::lmsToOklab[4] * mediumScaled
        + ColorSpaces::lmsToOklab[5] * shortScaled,
        missingB ? tl::optional<double>() :
        ColorSpaces::lmsToOklab[6] * longScaled
        + ColorSpaces::lmsToOklab[7] * mediumScaled
        + ColorSpaces::lmsToOklab[8] * shortScaled,
        alpha);

      return nullptr;
    }
    else if (dest == ColorSpaces::oklch) {
      // This is equivalent to converting to OKLab and then to OKLCH, but we
      // do it inline to avoid extra list allocations since we expect
      // conversions to and from OKLCH to be very common.
      double longScaled = _cubeRootPreservingSign(lng.value_or(0));
      double mediumScaled = _cubeRootPreservingSign(med.value_or(0));
      double shortScaled = _cubeRootPreservingSign(shrt.value_or(0));

      return Color::labToLch(pstate, dest,
        missingLightness ? tl::optional<double>() :
        ColorSpaces::lmsToOklab[0] * longScaled
        + ColorSpaces::lmsToOklab[1] * mediumScaled
        + ColorSpaces::lmsToOklab[2] * shortScaled,
        ColorSpaces::lmsToOklab[3] * longScaled
        + ColorSpaces::lmsToOklab[4] * mediumScaled
        + ColorSpaces::lmsToOklab[5] * shortScaled,
        ColorSpaces::lmsToOklab[6] * longScaled
        + ColorSpaces::lmsToOklab[7] * mediumScaled
        + ColorSpaces::lmsToOklab[8] * shortScaled,
        alpha, missingChroma, missingHue);

    }

    return ColorSpace::convertLinear(
      dest, pstate,
      lng, med, shrt, alpha,
      missingLightness,
      missingChroma,
      missingHue,
      missingA,
      missingB);

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* OkLchColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lightness,
    tl::optional<double> chroma,
    tl::optional<double> hue,
    tl::optional<double> alpha) const
  {
    // std::cerr << "OKLCH Translate\n";
    double hueRadians = hue.value_or(0) * PI / 180.0;
    return ColorSpaces::oklab.translate(
      dest, pstate,
      lightness,
      chroma.value_or(0) * std::cos(hueRadians),
      chroma.value_or(0) * std::sin(hueRadians),
      alpha,
      !chroma.has_value(),
      !hue.has_value());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* OkLabColorSpace::translate(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lightness,
    tl::optional<double> a,
    tl::optional<double> b,
    tl::optional<double> alpha,
    bool missingChroma,
    bool missingHue) const
  {
    // std::cerr << "OKLAB Translate\n";
    if (dest.name() == "oklch") {
      return Color::labToLch(pstate, dest, lightness, a, b, alpha,
        missingChroma, missingHue);
    }

    bool missingLightness = !lightness.has_value();
    bool missingA = !a.has_value();
    bool missingB = !b.has_value();

    if (!lightness.has_value()) lightness = 0;
    if (!a.has_value()) a = 0;
    if (!b.has_value()) b = 0;
    // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
    return ColorSpaces::lms.translate(
      dest, pstate,
      std::pow(
        ColorSpaces::oklabToLms[0] * lightness.value_or(0) +
        ColorSpaces::oklabToLms[1] * a.value_or(0) +
        ColorSpaces::oklabToLms[2] * b.value_or(0),
        3) + 0.0,
      std::pow(
        ColorSpaces::oklabToLms[3] * lightness.value_or(0) +
        ColorSpaces::oklabToLms[4] * a.value_or(0) +
        ColorSpaces::oklabToLms[5] * b.value_or(0),
        3) + 0.0,
      std::pow(
        ColorSpaces::oklabToLms[6] * lightness.value_or(0) +
        ColorSpaces::oklabToLms[7] * a.value_or(0) +
        ColorSpaces::oklabToLms[8] * b.value_or(0),
        3) + 0.0,
      alpha,
      missingLightness,
      missingChroma,
      missingHue,
      missingA,
      missingB);
  }

  // Converts a legacy HSL/HWB hue to an RGB channel.
  // The algorithm comes from from the CSS3 spec:
  // http://www.w3.org/TR/css3-color/#hsl-color.
  static double hueToRgb(double m1, double m2, double hue) {

    while (hue < 0) hue += 1;
    while (hue > 1) hue -= 1;
    if (hue < 1.0 / 6.0) {
      return m1 + (m2 - m1) * hue * 6.0;
    }
    else if (hue < 1.0 / 2.0) {
      return m2;
    }
    else if (hue < 2.0 / 3.0) {
      return m1 + (m2 - m1) * (2.0 / 3.0 - hue) * 6.0;
    }
    else {
      return m1;
    }
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static double nnan(double val) {
    if (std::isnan(val) == false) return val;
    return std::numeric_limits<double>::quiet_NaN();
  }

  Color* HwbColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> hue,
    tl::optional<double> whiteness,
    tl::optional<double> blackness,
    tl::optional<double> alpha) const
  {
    // From https://www.w3.org/TR/css-color-4/#hwb-to-rgb
    double scaledHue = std::fmod(hue.value_or(0), 360.0) / 360.0;
    double scaledWhiteness = whiteness.value_or(0) / 100.0;
    double scaledBlackness = blackness.value_or(0) / 100.0;

    double sum = scaledWhiteness + scaledBlackness;
    if (sum > 1) {
      scaledWhiteness /= sum;
      scaledBlackness /= sum;
    }

    double factor = 1.0 - scaledWhiteness - scaledBlackness;

    // Non-null because an in-gamut HSL color is
    // guaranteed to be in-gamut for HWB as well.
    return ColorSpaces::srgb.translate(
      dest, pstate,
      nnan(hueToRgb(0.0, 1.0, scaledHue + 1.0 / 3.0) * factor + scaledWhiteness),
      nnan(hueToRgb(0.0, 1.0, scaledHue) * factor + scaledWhiteness),
      nnan(hueToRgb(0.0, 1.0, scaledHue - 1.0 / 3.0) * factor + scaledWhiteness),
      alpha,
      false, false,
      !hue.has_value());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* LchColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lightness,
    tl::optional<double> chroma,
    tl::optional<double> hue,
    tl::optional<double> alpha) const
  {
    double hueRadians = hue.value_or(0) * PI / 180.0;
    auto rv = ColorSpaces::lab.translate(dest, pstate,
      lightness,
      chroma.value_or(0) * std::cos(hueRadians),
      chroma.value_or(0) * std::sin(hueRadians),
      alpha,
      !chroma.has_value(),
      !hue.has_value());
    return rv;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  /// Converts an f-format component to the X or Z channel of an XYZ color.
  static double _convertFToXorZ(double component) {
    double cubed = std::pow(component, 3.0) + 0.0;
    return cubed > labEpsilon ? cubed : (116.0 * component - 16.0) / labKappa;
  }

  Color* LabColorSpace::translate(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lightness,
    tl::optional<double> a,
    tl::optional<double> b,
    tl::optional<double> alpha,
    bool missingChroma,
    bool missingHue) const
  {

    // std::cerr << "LAB.CONVERT " << lightness.value_or(-42) << "; " << a.value_or(-42) << ", " << b.value_or(-42) << "\n";

    if (dest == ColorSpaces::lab)
    {
      bool powerlessAB = !lightness.has_value() || fuzzyEquals(lightness.value(), 0, sass::epsilon);
      return Color::lab(pstate,
        lightness,
        !a.has_value() || powerlessAB ? tl::optional<double>() : a,
        !b.has_value() || powerlessAB ? tl::optional<double>() : b,
        alpha);
    }
    else if (dest == ColorSpaces::lch)
    {
      return Color::labToLch(pstate,
        dest, lightness, a, b, alpha,
        missingChroma, missingHue);
    }
    else
    {
      bool missingLightness = !lightness.has_value();
      if (missingLightness) lightness = 0;
      // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
      // and http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
      double f1 = (lightness.value() + 16.0) / 116.0;

      // std::cerr << "=> F1 " << f1 << "\n";

      return ColorSpaces::xyzd50.translate(
        dest, pstate,
        _convertFToXorZ((a.value_or(0)) / 500.0 + f1) * d50[0],
        (lightness.value() > labKappa * labEpsilon
          ? std::pow((lightness.value() + 16.0) / 116.0, 3.0) * 1.0
          : lightness.value() / labKappa) *
        d50[1],
        _convertFToXorZ(f1 - (b.value_or(0.0)) / 200.0) * d50[2],
        alpha,
        missingLightness,
        missingChroma,
        missingHue,
        !a.has_value(),
        !b.has_value());
    }

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* SrgbLinearColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> red,
    tl::optional<double> green,
    tl::optional<double> blue,
    tl::optional<double> alpha) const
  {
    if (dest == ColorSpaces::rgb || dest == ColorSpaces::hsl ||
      dest == ColorSpaces::hwb || dest == ColorSpaces::srgb)
    {
      return ColorSpaces::srgb.convert(
        dest, pstate,
        red.transform(srgbAndDisplayP3FromLinear),
        green.transform(srgbAndDisplayP3FromLinear),
        blue.transform(srgbAndDisplayP3FromLinear),
        alpha);
    }
    else {
      return ColorSpace::convert(dest,
        pstate, red, green, blue, alpha);
    }
  }


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Color* HslColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> hue,
    tl::optional<double> saturation,
    tl::optional<double> lightness,
    tl::optional<double> alpha) const
  {
    // Algorithm from the CSS3 spec: https://www.w3.org/TR/css3-color/#hsl-color.
    double scaledHue = std::fmod(hue.value_or(0) / 360.0, 1.0);
    double scaledSaturation = saturation.value_or(0) / 100.0;
    double scaledLightness = lightness.value_or(0) / 100.0;

    double m2 = scaledLightness <= 0.5
      ? scaledLightness * (scaledSaturation + 1)
      : scaledLightness + scaledSaturation -
      scaledLightness * scaledSaturation;
    double m1 = scaledLightness * 2 - m2;

    return ColorSpaces::srgb.translate(
      dest, pstate,
      hueToRgb(m1, m2, scaledHue + 1.0 / 3.0),
      hueToRgb(m1, m2, scaledHue),
      hueToRgb(m1, m2, scaledHue - 1.0 / 3.0),
      alpha,
      !lightness.has_value(),
      !saturation.has_value(),
      !hue.has_value());
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
