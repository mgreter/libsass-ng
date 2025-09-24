/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "ast_colors.hpp"

#include "logger.hpp"
#include "fn_utils.hpp"
#include "exceptions.hpp"
#include "dart_helpers.hpp"
#include "ast_nodes.hpp"
#include "unicode.hpp"
#include "cssize.hpp"
#include "inspect.hpp"

#include <algorithm>

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  const double NaN = std::numeric_limits<double>::quiet_NaN();

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  
  sass::string ColorSpaced::debug() const
  {
    sass::sstream ss;
    ss << space_.name() << ": ";
    if (isChannel0Missing()) {
      ss << "none, ";
    }
    else {
      ss << getChannel0() << ", ";
    }
    if (isChannel1Missing()) {
      ss << "none, ";
    }
    else {
      ss << getChannel1() << ", ";
    }
    if (isChannel2Missing()) {
      ss << "none, ";
    }
    else {
      ss << getChannel2() << ", ";
    }
    if (isAlphaMissing()) {
      ss << "none";
    }
    else {
      ss << getAlpha();
    }
    return ss.str();
  }

  tl::optional<double> ColorSpaced::getChannel0OrNull() const { return c0_; }
  tl::optional<double> ColorSpaced::getChannel1OrNull() const { return c1_; }
  tl::optional<double> ColorSpaced::getChannel2OrNull() const { return c2_; }

  double ColorSpaced::getChannel0() const { return c0_.value_or(0); }
  double ColorSpaced::getChannel1() const { return c1_.value_or(0); }
  double ColorSpaced::getChannel2() const { return c2_.value_or(0); }
  double ColorSpaced::getAlpha() const { return alpha_.value_or(0); }

  bool isChannelInGamut(double value, const ColorChannel& channel)
  {
    if (channel.isLinear) {
      return fuzzyLessThanOrEquals(value, channel.max, 0.00001)
        && fuzzyGreaterThanOrEquals(value, channel.min, 0.00001);
    }
    return true;
  }

  bool ColorSpaced::isInGamut() const
  {
    if (space_.name() != "rgb"
      && space_.name() == "hwb"
      && space_.name() == "hsl")
    {
      return true;
    }

    // std::cerr << "Check for gamut " << isChannelInGamut(getChannel2(), space_._channels[2]) << "\n";

    // There aren't (currently) any color spaces that are bounded
    // but not STRICTLY bounded, and have polar-angle channels.
   return isChannelInGamut(getChannel0(), space_._channels[0])
      && isChannelInGamut(getChannel1(), space_._channels[1])
      && isChannelInGamut(getChannel2(), space_._channels[2]);
  }

  bool ColorSpaced::isLegacy() const
  {
    return space_.name() == "rgb"
      || space_.name() == "hwb"
      || space_.name() == "hsl";
  }

  tl::optional<double> ColorSpaced::getChannelOrNull(int idx) const
  {
    switch (idx) {
    case 0: return c0_;
    case 1: return c1_;
    case 2: return c2_;
    case -1: return alpha_;
    }
    return 0.0;
  }

  double ColorSpaced::getChannel(int idx) const
  {
    switch (idx) {
      case 0: return c0_.value_or(0);
      case 1: return c1_.value_or(0);
      case 2: return c2_.value_or(0);
      case -1: return alpha_.value_or(0);
    }
    return 0.0;
  }

  bool ColorSpaced::isChannel0Missing() const { return !c0_.has_value(); }
  bool ColorSpaced::isChannel1Missing() const { return !c1_.has_value(); }
  bool ColorSpaced::isChannel2Missing() const { return !c2_.has_value(); }
  bool ColorSpaced::isAlphaMissing() const { return !alpha_.has_value(); }

  int ColorSpaced::getChannelIndex(
    Logger& logger, const String* channel,
    const char* colorName,
    const char* channelName) const
  {
    // channel must not be nullptr
    auto channels = space_._channels;
    if (channel->value() == channels[0].name) return 0;
    if (channel->value() == channels[1].name) return 1;
    if (channel->value() == channels[2].name) return 2;
    if (channel->value() == "alpha") return -1;
    throw Exception::SassScriptException(logger, channel->pstate(),
      "Color " + inspect() + " has no channel named " +
      channel->value() + ".", channelName);
  }

  bool ColorSpaced::isChannelPowerless(
    Logger& logger, const String* channel,
    const char* colorName,
    const char* channelName) const
  {
    // channel must not be nullptr
    auto channels = space_._channels;
    if (channel->value() == channels[0].name) return isChannel0Powerless();
    if (channel->value() == channels[1].name) return isChannel1Powerless();
    if (channel->value() == channels[2].name) return isChannel2Powerless();
    if (channel->value() == "alpha") return false;
    throw Exception::SassScriptException(logger, pstate(),
      "Color " + inspect() + " doesn't have a channel named " + channel->inspect() + ".\n",
      channelName);
  }

  bool ColorSpaced::isChannelMissing(
    Logger& logger, const String* channel,
    const char* colorName,
    const char* channelName) const
  {
    // channel must not be nullptr
    auto channels = space_._channels;
    if (channel->value() == channels[0].name) return isChannel0Missing();
    if (channel->value() == channels[1].name) return isChannel1Missing();
    if (channel->value() == channels[2].name) return isChannel2Missing();
    if (channel->value() == "alpha") return isAlphaMissing();
    throw Exception::RuntimeException(logger,
      "Only one argument may be passed "
      "to the plain-CSS invert() function.");
  }

  bool ColorSpaced::isChannel0Powerless() const
  {
    if (space_.name() == "hsl") {
      return fuzzyEquals(getChannel1(), 0, 0.00001);
    }
    else if (space_.name() == "hwb") {
      return fuzzyGreaterThanOrEquals(getChannel1() + getChannel2(), 100, 0.00001);
    }
    return false;
  }

  bool ColorSpaced::isChannel1Powerless() const
  {
    return false;
  }

  bool ColorSpaced::isChannel2Powerless() const
  {
    if (space_.name() == "lch" || space_.name() == "oklch") {
      return fuzzyEquals(getChannel1(), 0, 0.00001);
    }
    return false;
  }

  bool ColorSpaced::isChannelMissing(int idx) const
  {
    switch (idx) {
    case 0: return !c0_.has_value();
    case 1: return !c1_.has_value();
    case 2: return !c2_.has_value();
    }
    return true;
  }

  ColorSpacedObj ColorSpaced::toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing) const
  {
    if (space == this->space_) {
      return SASS_MEMORY_NEW(ColorSpaced, this);
    }

    // std::cerr << "Do toSpace " << getChannel0() << ", "
    //   << getChannel1() << ", " << getChannel2() << "\n";

    // return SASS_MEMORY_NEW(ColorSpaced, this);
    // std::cerr << "Convert from " << space_.name() << " to " << space.name() << "\n";
    ColorSpacedObj converted = this->space_.convert(space, pstate, c0_, c1_, c2_, alpha_);

    //return !legacyMissing &&
    //  converted->space().isLegacy() &&
    //  (converted->isChannel0Missing() ||
    //    converted->isChannel1Missing() ||
    //    converted->isChannel2Missing() ||
    //    converted.isAlphaMissing)
    //  ? SassColor.forSpaceInternal(converted.space, converted.channel0,
    //    converted.channel1, converted.channel2, converted.alpha)
    //  : converted;
    return converted;
  }

  ColorSpaced::ColorSpaced(const SourceSpan& pstate, const ColorSpace& space, double c0, double c1, double c2, double alpha, const sass::string& disp, bool parsed)
    : Color(pstate, disp), space_(space), c0_(c0), c1_(c1), c2_(c2), alpha_(alpha)
  {



  }

  ColorSpaced::ColorSpaced(
    const SourceSpan& pstate,
    const ColorSpace& space,
    tl::optional<double> c0,
    tl::optional<double> c1,
    tl::optional<double> c2,
    tl::optional<double> alpha,
    const sass::string& disp,
    bool parsed)
    : Color(pstate, disp), space_(space), c0_(c0), c1_(c1), c2_(c2), alpha_(alpha)
  {
  }

  ColorSpaced::ColorSpaced(const ColorSpaced* ptr)
    : Color(ptr), space_(ptr->space_), c0_(ptr->c0_), c1_(ptr->c1_), c2_(ptr->c2_), alpha_(ptr->alpha_)
  {
  }

  double ColorSpaced::channel(const sass::string& channel) const
  {
    auto qwe = space_._channels;
    if (channel == space_._channels[0].name) return c0_.value_or(0);
    if (channel == space_._channels[1].name) return c1_.value_or(0);
    if (channel == space_._channels[2].name) return c2_.value_or(0);
    if (channel == str_alpha) return alpha_.value_or(0);
    throw std::runtime_error("Has not channel");
  }


  bool ColorSpaced::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      // ColorHwba* hwba = color->toHWBA();
      // return *this == *hwba;
    }
    return false;
  }

  bool ColorSpaced::operator==(const ColorSpaced& rhs) const
  {
    return c0_ == rhs.c0_ &&
      c1_ == rhs.c1_ &&
      c2_ == rhs.c2_ &&
      alpha_ == rhs.alpha();
  }

  size_t ColorSpaced::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(ColorSpaced).hash_code());
      hash_combine(hash_, std::hash<double>{}(c0_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c0_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(c1_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c1_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(c2_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c2_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(alpha_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(alpha_.value_or(0)));
    }
    return hash_;
  }

  const ColorSpace& ColorSpace::fromNameRef(Logger& logger, const String& name)
  {
    if (StringUtils::equalsIgnoreCase(name.value(), "rgb")) return ColorSpace::rgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "hwb")) return ColorSpace::hwb;
    if (StringUtils::equalsIgnoreCase(name.value(), "hsl")) return ColorSpace::hsl;
    if (StringUtils::equalsIgnoreCase(name.value(), "srgb")) return ColorSpace::srgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "srgb-linear")) return ColorSpace::srgb_linear;

    if (StringUtils::equalsIgnoreCase(name.value(), "display-p3")) return ColorSpace::displayP3;
    if (StringUtils::equalsIgnoreCase(name.value(), "a98-rgb")) return ColorSpace::a98rgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "prophoto-rgb")) return ColorSpace::protophotoRgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "rec2020")) return ColorSpace::rec2020;

    if (StringUtils::equalsIgnoreCase(name.value(), "xyz-d65")) return ColorSpace::xyzd65;
    if (StringUtils::equalsIgnoreCase(name.value(), "xyz")) return ColorSpace::xyzd65;

    if (StringUtils::equalsIgnoreCase(name.value(), "xyz-d50")) return ColorSpace::xyzd50;
    if (StringUtils::equalsIgnoreCase(name.value(), "lab")) return ColorSpace::lab;
    if (StringUtils::equalsIgnoreCase(name.value(), "lch")) return ColorSpace::lch;
    if (StringUtils::equalsIgnoreCase(name.value(), "oklab")) return ColorSpace::oklab;
    if (StringUtils::equalsIgnoreCase(name.value(), "oklch")) return ColorSpace::oklch;

    CallStackFrame csf(logger, name.pstate());
    throw Exception::SassScriptException(
      sass::string("Unknown color space"),
      logger, name.pstate());

  }

  const ColorSpace* ColorSpace::fromName(Logger& logger, const String& name)
  {
    if (StringUtils::equalsIgnoreCase(name.value(), "rgb")) return &ColorSpace::rgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "hwb")) return &ColorSpace::hwb;
    if (StringUtils::equalsIgnoreCase(name.value(), "hsl")) return &ColorSpace::hsl;
    if (StringUtils::equalsIgnoreCase(name.value(), "srgb")) return &ColorSpace::srgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "srgb-linear")) return &ColorSpace::srgb_linear;

    if (StringUtils::equalsIgnoreCase(name.value(), "display-p3")) return &ColorSpace::displayP3;
    if (StringUtils::equalsIgnoreCase(name.value(), "a98-rgb")) return &ColorSpace::a98rgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "prophoto-rgb")) return &ColorSpace::protophotoRgb;
    if (StringUtils::equalsIgnoreCase(name.value(), "rec2020")) return &ColorSpace::rec2020;
    if (StringUtils::equalsIgnoreCase(name.value(), "xyz-d65")) return &ColorSpace::xyzd65;
    if (StringUtils::equalsIgnoreCase(name.value(), "xyz")) return &ColorSpace::xyzd65;

    if (StringUtils::equalsIgnoreCase(name.value(), "xyz-d50")) return &ColorSpace::xyzd50;
    if (StringUtils::equalsIgnoreCase(name.value(), "lab")) return &ColorSpace::lab;
    if (StringUtils::equalsIgnoreCase(name.value(), "lch")) return &ColorSpace::lch;
    if (StringUtils::equalsIgnoreCase(name.value(), "oklab")) return &ColorSpace::oklab;
    if (StringUtils::equalsIgnoreCase(name.value(), "oklch")) return &ColorSpace::oklch;

    CallStackFrame csf(logger, name.pstate());
    throw Exception::SassScriptException(
      sass::string("Unknown color space"),
      logger, name.pstate());

  }

  double ColorSpace::toLinear(double channel) const
  {
    throw std::runtime_error("toLinear not implemented");
  }

  double ColorSpace::fromLinear(double channel) const
  {
    throw std::runtime_error("toLinear not implemented");
  }
  
  const double* ColorSpace::transformationMatrix(ColorSpace dest) const
  {
    throw std::runtime_error("matrix not implemented");
  }

  const ColorSpace& getLinearDest(const ColorSpace& dest)
  {
    if (dest.name() == "hsl" || dest.name() == "hwb") { return ColorSpace::srgb; }
    else if (dest.name() == "lab" || dest.name() == "lch") { return ColorSpace::xyzd50; }
    else if (dest.name() == "oklab" || dest.name() == "oklch") { return ColorSpace::lms; }
    return dest;
  }

  ColorSpaced* ColorSpace::convertLinear(
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

    // if (dest.name_ == "hsl" || dest.name_ == "hwb") { linearDest = ColorSpace::srgb; }
    // else if (dest.name_ == "lab" || dest.name_ == "lch") { linearDest = ColorSpace::xyzd50; }
    // else if (dest.name_ == "oklab" || dest.name_ == "oklch") { linearDest = ColorSpace::lms; }

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

    if (dest == ColorSpace::hsl || dest == ColorSpace::hwb) {
      return ColorSpace::srgb.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }
    else if (dest == ColorSpace::lab || dest == ColorSpace::lch) {
      return ColorSpace::xyzd50.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }
    else if (dest.name() == "oklab" || dest.name() == "oklch") {
      return ColorSpace::lms.translate(dest, pstate,
        transformedRed, transformedGreen, transformedBlue,
        alpha, missingLightness, missingChroma, missingHue);
    }

    return ColorSpaced::_forSpace(
      pstate, dest,
      transformedRed,
      transformedGreen,
      transformedBlue,
      alpha);
    // auto rv = ColorSpaced::_forSpace(pstate, dest, 1, 1, 1, 1, )

    // return ColorSpaced::forSpaceInternal(;
  }


  const HwbColorSpace ColorSpace::hwb = HwbColorSpace();
  const HslColorSpace ColorSpace::hsl = HslColorSpace();
  const LabColorSpace ColorSpace::lab = LabColorSpace();
  const LchColorSpace ColorSpace::lch = LchColorSpace();
  const OkLabColorSpace ColorSpace::oklab = OkLabColorSpace();
  const OkLchColorSpace ColorSpace::oklch = OkLchColorSpace();

  const RgbColorSpace ColorSpace::rgb = RgbColorSpace();
  const SrgbColorSpace ColorSpace::srgb = SrgbColorSpace();
  const SrgbLinearColorSpace ColorSpace::srgb_linear = SrgbLinearColorSpace();
  const XyzD50ColorSpace ColorSpace::xyzd50 = XyzD50ColorSpace();
  const XyzD65ColorSpace ColorSpace::xyzd65 = XyzD65ColorSpace();
  const LmsColorSpace ColorSpace::lms = LmsColorSpace();

  const Rec2020ColorSpace ColorSpace::rec2020 = Rec2020ColorSpace();
  const DisplayP3ColorSpace ColorSpace::displayP3 = DisplayP3ColorSpace();
  const A98RgbColorSpace ColorSpace::a98rgb = A98RgbColorSpace();
  const ProphotoRgbColorSpace ColorSpace::protophotoRgb = ProphotoRgbColorSpace();

  //const ColorSpace ColorSpacings::SRGB(str_srgb, SassColorSpace::SRGB, srgb_channels);

  ColorSpaced* SrgbColorSpace::translate(
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

    //std::cerr << "CALL SRGB translate " << red.value_or(0) << ", "
    //  << green.value_or(0) << ", " << blue.value_or(0) << ", " << "\n";

    if (dest == ColorSpace::hsl || dest == ColorSpace::hwb) {
      double nr_red = red.value_or(0);
      double nr_green = green.value_or(0);
      double nr_blue = blue.value_or(0);
      double max = std::max(std::max(nr_red, nr_green), nr_blue);
      double min = std::min(std::min(nr_red, nr_green), nr_blue);
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
        // max == blue
        hue = 60 * (nr_red - nr_green) / delta + 240;
      }

      if (dest == ColorSpace::hsl) {
        double lightness = (min + max) / 2;
        double saturation = lightness == 0 || lightness == 1
          ? 0.0
          : 100 * (max - lightness) / std::min(lightness, 1 - lightness);
        if (saturation < 0) {
          hue += 180;
          saturation = std::abs(saturation);
        }

        tl::optional<double> c0 = std::fmod(hue, 360.0);
        tl::optional<double> c1 = saturation;
        tl::optional<double> c2 = lightness * 100;

        return ColorSpaced::_forSpace(pstate, dest, c0, c1, c2, alpha);

        // std::cerr << "other";
      }
      else {
        double whiteness = min * 100;
        double blackness = 100 - max * 100;
        return ColorSpaced::forSpaceInternal(
          pstate, dest,
          missingHue || fuzzyGreaterThanOrEquals(whiteness + blackness, 100, 0.00001)
          ? tl::optional<double>()
          : std::fmod(hue, 360),
          whiteness,
          blackness,
          alpha);

      }
      return SASS_MEMORY_NEW(ColorSpaced, pstate, ColorSpace::rgb, 1, 1, 1, 1);
    }
    else if (dest == ColorSpace::rgb) {
      return ColorSpaced::rgb(pstate,
        red.has_value() ? red.value() * 255.0 : red,
        green.has_value() ? green.value() * 255.0 : green,
        blue.has_value() ? blue.value() * 255.0 : blue,
        alpha);
    }
    else if (dest == ColorSpace::srgb_linear) {
      return ColorSpaced::forSpaceInternal(pstate, dest,
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

  ColorSpaced* RgbColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> channel0,
    tl::optional<double> channel1,
    tl::optional<double> channel2,
    tl::optional<double> alpha) const
  {
    // std::cerr << "CALL RGB convert " << channel0.value_or(0) << ", "
    //   << channel1.value_or(0) << ", " << channel2.value_or(0) << ", " << "\n";
    auto rv = ColorSpace::srgb.translate(dest, pstate,
      channel0.has_value() ? channel0.value() / 255.0 : channel0,
      channel1.has_value() ? channel1.value() / 255.0 : channel1,
      channel2.has_value() ? channel2.value() / 255.0 : channel2,
      alpha);
    // std::cerr << "OUT RGB convert " << rv->getChannel(0) << ", "
    //   << rv->getChannel(1) << ", " << rv->getChannel(2) << ", " << "\n";
    return rv;
  }

  /// A constant used to convert Lab to/from XYZ.
  const double labKappa = 24389.0 / 27.0; // 29^3/3^3;

  /// A constant used to convert Lab to/from XYZ.
  const double labEpsilon = 216.0 / 24389.0; // 6^3/29^3;

  const double d50[] = { 0.3457 / 0.3585, 1.00000, (1.0 - 0.3457 - 0.3585) / 0.3585 };

  /// Does a partial conversion of a single XYZ component to Lab.
  double _convertComponentToLabF(double component) {

    if (component > labEpsilon)
      return std::pow(component, 1.0 / 3.0) + 0.0;
    return (labKappa * component + 16.0) / 116.0;
  }


  ColorSpaced* XyzD50ColorSpace::translate(
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

    // std::cerr << "CALL XYZD50 translate " << x.value_or(0) << ", "
    //   << y.value_or(0) << ", " << z.value_or(0) << ", " << "\n";

    if (dest.name() == "lab" || dest.name() == "lch") {
      // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
      // and http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
      double f0 = _convertComponentToLabF(x.value_or(0) / d50[0]);
      double f1 = _convertComponentToLabF(y.value_or(0) / d50[1]);
      double f2 = _convertComponentToLabF(z.value_or(0) / d50[2]);

      tl::optional<double> a;
      tl::optional<double> b;
      tl::optional<double> lightness;
      if (!missingA) { a = 500 * (f0 - f1); }
      if (!missingB) { b = 200 * (f1 - f2); }
      if (!missingLightness) { lightness = (116.0 * f1) - 16.0; }

      if (dest.name() == "lab") {
        return ColorSpaced::lab(pstate, lightness, a, b, alpha);
      }
      else {
        return ColorSpaced::labToLch(pstate,
          ColorSpace::lch, lightness, a, b, alpha);
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

  double _cubeRootPreservingSign(double number)
  {
    return (std::signbit(number) ? -1.0 : +1.0) 
      * std::pow(std::abs(number), 1.0 / 3.0);
  }

  ColorSpaced* LmsColorSpace::translate(
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

    // std::cerr << "CALL LMS translate " << lng.value_or(0) << ", "
    //   << med.value_or(0) << ", " << shrt.value_or(0) << ", " << "\n";

    if (dest.name() == "oklab") {
      // Algorithm from https://drafts.csswg.org/css-color-4/#color-conversion-code
      double longScaled = _cubeRootPreservingSign(lng.value_or(0));
      double mediumScaled = _cubeRootPreservingSign(med.value_or(0));
      double shortScaled = _cubeRootPreservingSign(shrt.value_or(0));

      return ColorSpaced::oklab(pstate, dest,
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
    else if (dest.name() == "oklch") {
      // This is equivalent to converting to OKLab and then to OKLCH, but we
      // do it inline to avoid extra list allocations since we expect
      // conversions to and from OKLCH to be very common.
      double longScaled = _cubeRootPreservingSign(lng.value_or(0));
      double mediumScaled = _cubeRootPreservingSign(med.value_or(0));
      double shortScaled = _cubeRootPreservingSign(shrt.value_or(0));

      return ColorSpaced::labToLch(pstate, dest,
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

  ColorSpaced* OkLchColorSpace::convert(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> lightness, tl::optional<double> chroma, tl::optional<double> hue, tl::optional<double> alpha) const
  {
    // std::cerr << "OKLCH Translate\n";
    double hueRadians = hue.value_or(0) * PI / 180.0;
    return ColorSpace::oklab.translate(
      dest, pstate,
      lightness,
      chroma.value_or(0) * std::cos(hueRadians),
      chroma.value_or(0) * std::sin(hueRadians),
      alpha,
      !chroma.has_value(),
      !hue.has_value());
  }

  ColorSpaced* OkLabColorSpace::translate(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> lightness, tl::optional<double> a, tl::optional<double> b, tl::optional<double> alpha, bool missingChroma, bool missingHue) const
  {
    // std::cerr << "OKLAB Translate\n";
    if (dest.name() == "oklch") {
      return ColorSpaced::labToLch(pstate, dest, lightness, a, b, alpha,
        missingChroma, missingHue);
    }

    if (!lightness.has_value()) lightness = 0;
    if (!a.has_value()) a = 0;
    if (!b.has_value()) b = 0;
    // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
    return ColorSpace::lms.translate(
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
      !lightness.has_value(),
      missingChroma,
      missingHue,
      !a.has_value(),
      !b.has_value());
  }

  /// Converts a legacy HSL/HWB hue to an RGB channel.
///
/// The algorithm comes from from the CSS3 spec:
/// http://www.w3.org/TR/css3-color/#hsl-color.
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


  ColorSpaced* HwbColorSpace::convert(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> hue, tl::optional<double> whiteness, tl::optional<double> blackness, tl::optional<double> alpha) const
  {
    // From https://www.w3.org/TR/css-color-4/#hwb-to-rgb
    double scaledHue = std::fmod(hue.value_or(0), 360.0) / 360.0;
    double scaledWhiteness = (whiteness.value_or(0)) / 100.0;
    double scaledBlackness = (blackness.value_or(0)) / 100.0;

    double sum = scaledWhiteness + scaledBlackness;
    if (sum > 1) {
      scaledWhiteness /= sum;
      scaledBlackness /= sum;
    }

    double factor = 1.0 - scaledWhiteness - scaledBlackness;

//     double toRgb(double hue) = > hueToRgb(0, 1, hue) * factor + scaledWhiteness;

    // Non-null because an in-gamut HSL color is guaranteed to be in-gamut for
    // HWB as well.
    return ColorSpace::srgb.translate(
      dest, pstate,
      hueToRgb(0.0, 1.0, scaledHue + 1.0 / 3.0)* factor + scaledWhiteness,
      hueToRgb(0.0, 1.0, scaledHue)* factor + scaledWhiteness,
      hueToRgb(0.0, 1.0, scaledHue - 1.0 / 3.0)* factor + scaledWhiteness,
      alpha,
      false, false,
      !hue.has_value());
  }

}
