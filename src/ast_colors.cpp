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

  const double NaN = std::numeric_limits<double>::quiet_NaN();

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  
  bool _isAnalogousChannelMissing(
    Logger& logger,
    Color* original,
    Color* output,
    int outputChannelIndex)
  {
    if (output->isChannelMissing(outputChannelIndex)) return true;
    if (*original == *output) return false;
    const ColorChannel& outputChannel = output->space()._channels[outputChannelIndex];
    tl::optional<const ColorChannel&> originalChannel;
    for (int i = 0; i < original->space()._channelSize; i++) {
      auto& channel = original->space()._channels[i];
      if (!channel.isAnalogous(outputChannel)) continue;
      originalChannel = channel;
      break;
    }
    if (!originalChannel.has_value()) return false;
    return original->isChannelMissing(logger, originalChannel.value().name);
  }

  double _interpolateHues(
    double hue1,
    double hue2,
    HueInterpolationMethod method,
    double weight)

  {
    // Algorithms from https://www.w3.org/TR/css-color-4/#hue-interpolation
    double diff = hue2 - hue1;
    switch (method) {
    case shorter:
      if (diff > 180.0) {
        hue1 += 360.0;
      }
      else if (diff < -180.0) {
        hue2 += 360.0;
      }
      break;

    case longer:
      if (diff > 0.0 && diff < 180.0) {
        hue2 += 360.0;
      }
      else if (diff > -180.0 && diff <= 0.0) {
        hue1 += 360.0;
      }
      break;

    case increasing:
      if (hue2 < hue1)
        hue2 += 360.0;
      break;

    case decreasing:
      if (hue1 < hue2)
        hue1 += 360.0;
      break;

    }

    auto rv = hue1 * weight + hue2 * (1.0 - weight);

    return rv;
  }

  Color* Color::interpolate(Logger& logger, const SourceSpan& pstate,
    Color* other, InterpolationMethod method, double weight, bool legacyMissing)
  {

    if (fuzzyEquals(weight, 0.0, logger.epsilon)) return other;
    if (fuzzyEquals(weight, 1.0, logger.epsilon)) return this;

    Color* color1 = this->toSpace(method.space, pstate);
    Color* color2 = other->toSpace(method.space, pstate);

    if (weight < 0 || weight > 1) {
      throw Exception::SassScriptException(logger,
        pstate, "Weight out of Range."); // unchecked
    }

    bool missing1_0 = _isAnalogousChannelMissing(logger, this, color1, 0);
    bool missing1_1 = _isAnalogousChannelMissing(logger, this, color1, 1);
    bool missing1_2 = _isAnalogousChannelMissing(logger, this, color1, 2);
    bool missing2_0 = _isAnalogousChannelMissing(logger, other, color2, 0);
    bool missing2_1 = _isAnalogousChannelMissing(logger, other, color2, 1);
    bool missing2_2 = _isAnalogousChannelMissing(logger, other, color2, 2);

    double channel1_0 = (missing1_0 ? color2 : color1)->getChannel0();
    double channel1_1 = (missing1_1 ? color2 : color1)->getChannel1();
    double channel1_2 = (missing1_2 ? color2 : color1)->getChannel2();
    double channel2_0 = (missing2_0 ? color1 : color2)->getChannel0();
    double channel2_1 = (missing2_1 ? color1 : color2)->getChannel1();
    double channel2_2 = (missing2_2 ? color1 : color2)->getChannel2();

    bool missing_alpha1 = this->isAlphaMissing();
    bool missing_alpha2 = other->isAlphaMissing();

    double alpha1 = this->getAlphaOrNull().value_or(other->getAlpha());
    double alpha2 = other->getAlphaOrNull().value_or(this->getAlpha());

    double thisMultiplier = (this->getAlphaOrNull().value_or(1)) * weight;
    double otherMultiplier = (other->getAlphaOrNull().value_or(1)) * (1.0 - weight);

    tl::optional<double> mixedAlpha = missing_alpha1 && missing_alpha2
      ? tl::optional<double>() : alpha1 * weight + alpha2 * (1 - weight);
    tl::optional<double> mixed0 = missing1_0 && missing2_0 ? tl::optional<double>() :
      (channel1_0 * thisMultiplier + channel2_0 * otherMultiplier) / mixedAlpha.value_or(1.0);
    tl::optional<double> mixed1 = missing1_1 && missing2_1 ? tl::optional<double>() :
      (channel1_1 * thisMultiplier + channel2_1 * otherMultiplier) / mixedAlpha.value_or(1.0);
    tl::optional<double> mixed2 = missing1_2 && missing2_2 ? tl::optional<double>()
      : (channel1_2 * thisMultiplier + channel2_2 * otherMultiplier) / mixedAlpha.value_or(1.0);

    ColorObj rv;

    if (method.space == ColorSpace::hsl || method.space == ColorSpace::hwb) {
      rv = Color::forSpaceInternal(pstate, method.space,
        missing1_0 && missing2_0 ? tl::optional<double>() :
          _interpolateHues(channel1_0, channel2_0, method.hue, weight),
        mixed1, mixed2, mixedAlpha);
    }
    else if (method.space == ColorSpace::lch || method.space == ColorSpace::oklch) {
      rv = Color::forSpaceInternal(pstate, method.space, mixed0, mixed1,
        missing1_2 && missing2_2 ? tl::optional<double>() :
          _interpolateHues(channel1_2, channel2_2, method.hue, weight),
        mixedAlpha);
    }
    else {
      rv = Color::forSpaceInternal(pstate,
        method.space, mixed0, mixed1, mixed2, mixedAlpha);
    }

    if (rv == nullptr) return nullptr;
    rv = rv->toSpace(space(), pstate, legacyMissing);
    return rv.detach();

  }

  // sass::string Color::debug() const
  // {
  //   sass::sstream ss;
  //   ss << space_.name() << ": ";
  //   if (isChannel0Missing()) {
  //     ss << "none, ";
  //   }
  //   else {
  //     ss << getChannel0() << ", ";
  //   }
  //   if (isChannel1Missing()) {
  //     ss << "none, ";
  //   }
  //   else {
  //     ss << getChannel1() << ", ";
  //   }
  //   if (isChannel2Missing()) {
  //     ss << "none, ";
  //   }
  //   else {
  //     ss << getChannel2() << ", ";
  //   }
  //   if (isAlphaMissing()) {
  //     ss << "none";
  //   }
  //   else {
  //     ss << getAlpha();
  //   }
  //   return ss.str();
  // }

  tl::optional<double> Color::getChannel0OrNull() const { return c0_; }
  tl::optional<double> Color::getChannel1OrNull() const { return c1_; }
  tl::optional<double> Color::getChannel2OrNull() const { return c2_; }
  tl::optional<double> Color::getAlphaOrNull() const { return alpha_; }

  double Color::getChannel0() const { return c0_.value_or(0); }
  double Color::getChannel1() const { return c1_.value_or(0); }
  double Color::getChannel2() const { return c2_.value_or(0); }
  double Color::getAlpha() const { return alpha_.value_or(0); }

  static bool isChannelInGamut(double value, const ColorChannel& channel)
  {
    if (channel.isLinear) {
      return fuzzyLessThanOrEquals(value, channel.max, sass::epsilon)
        && fuzzyGreaterThanOrEquals(value, channel.min, sass::epsilon);
    }
    return true;
  }

  bool Color::isInGamut() const
  {
    if (!space_.isBounded())
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

  bool Color::isLegacy() const
  {
    return space_ == ColorSpace::rgb
      || space_ == ColorSpace::hwb
      || space_ == ColorSpace::hsl;
  }

  tl::optional<double> Color::getChannelOrNull(int idx) const
  {
    switch (idx) {
    case 0: return c0_;
    case 1: return c1_;
    case 2: return c2_;
    case -1: return alpha_;
    }
    return 0.0;
  }

  double Color::getChannel(int idx) const
  {
    switch (idx) {
      case 0: return c0_.value_or(0);
      case 1: return c1_.value_or(0);
      case 2: return c2_.value_or(0);
      case -1: return alpha_.value_or(0);
    }
    return 0.0;
  }

  bool Color::isChannel0Missing() const { return !c0_.has_value(); }
  bool Color::isChannel1Missing() const { return !c1_.has_value(); }
  bool Color::isChannel2Missing() const { return !c2_.has_value(); }
  bool Color::isAlphaMissing() const { return !alpha_.has_value(); }

  int Color::getChannelIndex(
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

  bool Color::isChannelPowerless(
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

  bool Color::isChannelMissing(
    Logger& logger, const String* channel, const sass::string& arg) const
  {
    // channel must not be nullptr
    auto channels = space_._channels;
    if (channel->value() == channels[0].name) return isChannel0Missing();
    if (channel->value() == channels[1].name) return isChannel1Missing();
    if (channel->value() == channels[2].name) return isChannel2Missing();
    if (channel->value() == "alpha") return isAlphaMissing();
    throw Exception::RuntimeException(logger,
      "$" + arg + ": Color " + toString() + " doesn\'t have a channel named " + channel->toString() + ".");
  }

  bool Color::isChannelMissing(
    Logger& logger, const sass::string& channel) const
  {
    // channel must not be nullptr
    auto channels = space_._channels;
    if (channel == channels[0].name) return isChannel0Missing();
    if (channel == channels[1].name) return isChannel1Missing();
    if (channel == channels[2].name) return isChannel2Missing();
    if (channel == "alpha") return isAlphaMissing();
    throw Exception::RuntimeException(logger,
      "Color " + toString() + " doesn\'t have a channel named " + channel + ".");
  }

  bool Color::isChannel0Powerless() const
  {
    if (space_.name() == "hsl") {
      return fuzzyEquals(getChannel1(), 0, 0.00001);
    }
    else if (space_.name() == "hwb") {
      return fuzzyGreaterThanOrEquals(getChannel1() + getChannel2(), 100, 0.00001);
    }
    return false;
  }

  bool Color::isChannel1Powerless() const
  {
    return false;
  }

  bool Color::isChannel2Powerless() const
  {
    if (space_ == ColorSpace::lch || space_.name() == "oklch") {
      return fuzzyEquals(getChannel1(), 0, 0.00001);
    }
    return false;
  }

  bool Color::isChannelMissing(int idx) const
  {
    switch (idx) {
    case 0: return !c0_.has_value();
    case 1: return !c1_.has_value();
    case 2: return !c2_.has_value();
    }
    return true;
  }


  Color* Color::toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing)
  {
    // Can return without creating a copy
    if (space == space_) return this;
    // If const, we must always create a copy
    return ((const Color*)this)->toSpace(
      space, pstate, legacyMissing);
  }

  Color* Color::toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing) const
  {
    if (space == this->space_) {
      // std::cerr << "copy into same space\n";
      return SASS_MEMORY_NEW(Color, this);
    }

    Color* converted = this->space_.convert(space, pstate, c0_, c1_, c2_, alpha_);

    if (!legacyMissing &&
      converted->isLegacy() &&
      (converted->isChannel0Missing() ||
        converted->isChannel1Missing() ||
        converted->isChannel2Missing() ||
        converted->isAlphaMissing()))
    {
      return Color::forSpaceInternal(
        pstate, converted->space(),
        converted->getChannel0(),
        converted->getChannel1(),
        converted->getChannel2(),
        converted->getAlpha());
    }
    else {
      return converted;
    }

    return converted;
  }

  Color::Color(const SourceSpan& pstate, const ColorSpace& space, double c0, double c1, double c2, double alpha, const sass::string& disp, bool parsed)
    : Value(pstate), disp_(disp), parsed_(parsed), space_(space), c0_(c0), c1_(c1), c2_(c2), alpha_(alpha)
  {



  }

  Color::Color(
    const SourceSpan& pstate,
    const ColorSpace& space,
    tl::optional<double> c0,
    tl::optional<double> c1,
    tl::optional<double> c2,
    tl::optional<double> alpha,
    const sass::string& disp,
    bool parsed)
    : Value(pstate), disp_(disp), parsed_(parsed), space_(space), c0_(c0), c1_(c1), c2_(c2), alpha_(alpha)
  {
  }

  Color::Color(const Color* ptr)
    : Value(ptr), disp_(ptr->disp_), parsed_(ptr->parsed_), space_(ptr->space_), c0_(ptr->c0_), c1_(ptr->c1_), c2_(ptr->c2_), alpha_(ptr->alpha_)
  {
  }

  double Color::channel(const sass::string& channel) const
  {
    if (channel == space_._channels[0].name) return c0_.value_or(0);
    if (channel == space_._channels[1].name) return c1_.value_or(0);
    if (channel == space_._channels[2].name) return c2_.value_or(0);
    if (channel == str_alpha) return alpha_.value_or(0);
    throw std::runtime_error("Has not channel");
  }


  bool Color::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      // ColorHwba* hwba = color->toHWBA();
      return *this == *color;
    }
    return false;
  }

  bool Color::operator==(const Color& rhs) const
  {
    if (!fuzzyEquals(alpha_, rhs.alpha_, sass::epsilon)) return false;
    if (isLegacy()) {
      if (!rhs.isLegacy()) return false;
      if (space_ == rhs.space_) {
        return fuzzyEquals(c0_, rhs.c0_, sass::epsilon)
          &&   fuzzyEquals(c1_, rhs.c1_, sass::epsilon)
          &&   fuzzyEquals(c2_, rhs.c2_, sass::epsilon);
      }
      Color* rgb1 = toSpace(ColorSpace::rgb, pstate());
      Color* rgb2 = rhs.toSpace(ColorSpace::rgb, rhs.pstate());
      // std::cerr << "rgb1 " << rgb1->debug() << "\n";
      // std::cerr << "rgb2 " << rgb2->debug() << "\n";
      auto rv = fuzzyEquals(rgb1->c0_, rgb2->c0_, sass::epsilon)
        && fuzzyEquals(rgb1->c1_, rgb2->c1_, sass::epsilon)
        && fuzzyEquals(rgb1->c2_, rgb2->c2_, sass::epsilon);
      return rv;
    }
    return space_ == rhs.space_
      && fuzzyEquals(alpha_, rhs.alpha_, sass::epsilon)
      && fuzzyEquals(c0_, rhs.c0_, sass::epsilon)
      && fuzzyEquals(c1_, rhs.c1_, sass::epsilon)
      && fuzzyEquals(c2_, rhs.c2_, sass::epsilon);
  }

  size_t Color::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(Color).hash_code());
      hash_combine(hash_, std::hash<double>{}(c0_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c0_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(c1_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c1_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(c2_.has_value()));
      hash_combine(hash_, std::hash<double>{}(c2_.value_or(0)));
      hash_combine(hash_, std::hash<double>{}(alpha_.has_value()));
      hash_combine(hash_, std::hash<double>{}(alpha_.value_or(0)));
    }
    return hash_;
  }

  const ColorSpace& ColorSpace::fromValueRef(Logger& logger, Value* value, const sass::string& name)
  {
    // assert(value != nullptr, "Space value must not be null");
    String* space_str = value->assertString(logger, name);
    space_str->assertUnquoted(logger, name); // may throw
    return fromNameRef(logger, *space_str, name); // safe access

    // TODO: insert return statement here
  }

  const ColorSpace& ColorSpace::fromNameRef(Logger& logger, const String& space, const sass::string& name)
  {
    if (StringUtils::equalsIgnoreCase(space.value(), "rgb")) return ColorSpace::rgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "hwb")) return ColorSpace::hwb;
    if (StringUtils::equalsIgnoreCase(space.value(), "hsl")) return ColorSpace::hsl;
    if (StringUtils::equalsIgnoreCase(space.value(), "srgb")) return ColorSpace::srgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "srgb-linear")) return ColorSpace::srgb_linear;

    if (StringUtils::equalsIgnoreCase(space.value(), "display-p3")) return ColorSpace::displayP3;
    if (StringUtils::equalsIgnoreCase(space.value(), "a98-rgb")) return ColorSpace::a98rgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "prophoto-rgb")) return ColorSpace::protophotoRgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "rec2020")) return ColorSpace::rec2020;

    if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d65")) return ColorSpace::xyzd65;
    if (StringUtils::equalsIgnoreCase(space.value(), "xyz")) return ColorSpace::xyzd65;

    if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d50")) return ColorSpace::xyzd50;
    if (StringUtils::equalsIgnoreCase(space.value(), "lab")) return ColorSpace::lab;
    if (StringUtils::equalsIgnoreCase(space.value(), "lch")) return ColorSpace::lch;
    if (StringUtils::equalsIgnoreCase(space.value(), "oklab")) return ColorSpace::oklab;
    if (StringUtils::equalsIgnoreCase(space.value(), "oklch")) return ColorSpace::oklch;

    throw Exception::UnknownColorSpace(logger, space, name);

  }

  const ColorSpace* ColorSpace::fromName(Logger& logger, const String& space, const sass::string& name)
  {
    if (StringUtils::equalsIgnoreCase(space.value(), "rgb")) return &ColorSpace::rgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "hwb")) return &ColorSpace::hwb;
    if (StringUtils::equalsIgnoreCase(space.value(), "hsl")) return &ColorSpace::hsl;
    if (StringUtils::equalsIgnoreCase(space.value(), "srgb")) return &ColorSpace::srgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "srgb-linear")) return &ColorSpace::srgb_linear;

    if (StringUtils::equalsIgnoreCase(space.value(), "display-p3")) return &ColorSpace::displayP3;
    if (StringUtils::equalsIgnoreCase(space.value(), "a98-rgb")) return &ColorSpace::a98rgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "prophoto-rgb")) return &ColorSpace::protophotoRgb;
    if (StringUtils::equalsIgnoreCase(space.value(), "rec2020")) return &ColorSpace::rec2020;
    if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d65")) return &ColorSpace::xyzd65;
    if (StringUtils::equalsIgnoreCase(space.value(), "xyz")) return &ColorSpace::xyzd65;

    if (StringUtils::equalsIgnoreCase(space.value(), "xyz-d50")) return &ColorSpace::xyzd50;
    if (StringUtils::equalsIgnoreCase(space.value(), "lab")) return &ColorSpace::lab;
    if (StringUtils::equalsIgnoreCase(space.value(), "lch")) return &ColorSpace::lch;
    if (StringUtils::equalsIgnoreCase(space.value(), "oklab")) return &ColorSpace::oklab;
    if (StringUtils::equalsIgnoreCase(space.value(), "oklch")) return &ColorSpace::oklch;

    throw Exception::UnknownColorSpace(logger, space, name);

  }

  double ColorSpace::toLinear(double channel) const
  {
    std::cerr << "Not implemented for " << name_ << "\n";
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

  static const ColorSpace& getLinearDest(const ColorSpace& dest)
  {
    if (dest == ColorSpace::hsl || dest == ColorSpace::hwb) { return ColorSpace::srgb; }
    else if (dest == ColorSpace::lab || dest == ColorSpace::lch) { return ColorSpace::xyzd50; }
    else if (dest == ColorSpace::oklab || dest == ColorSpace::oklch) { return ColorSpace::lms; }
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

    auto rv = Color::forSpaceInternal(
      pstate, dest,
      red.has_value() ? transformedRed : red,
      green.has_value() ? transformedGreen : green,
      blue.has_value() ? transformedBlue : blue,
      alpha);
    return rv;
    // auto rv = Color::_forSpace(pstate, dest, 1, 1, 1, 1, )

    // return Color::forSpaceInternal(;
  }



  //const ColorSpace ColorSpacings::SRGB(str_srgb, SassColorSpace::SRGB, srgb_channels);

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

    // std::cerr << "CALL SRGB translate " << red.value_or(-42) << ", "
    //   << green.value_or(-42) << ", " << blue.value_or(-42) << ", "
    //   << alpha.value_or(-42) << "\n";

    if (dest == ColorSpace::hsl || dest == ColorSpace::hwb) {
      double nr_red = red.value_or(0);
      double nr_green = green.value_or(0);
      double nr_blue = blue.value_or(0);
      double max = sass::max(sass::max(nr_red, nr_green), nr_blue);
      double min = sass::min(sass::min(nr_red, nr_green), nr_blue);
      // if (std::isinf(max)) max = NaN;
      // if (std::isinf(min)) min = NaN;
      double delta = max - min;

      // double test = std::max(NaN, NaN);

      // std::cerr << " inter " << min << ", " << max << ", " << delta << "\n";

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

        // std::cerr << "  to hsl " << hue << ", " << saturation << ", " << lightness << "\n";

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

        // std::cerr << "other";
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
      return SASS_MEMORY_NEW(Color, pstate, ColorSpace::rgb, 1, 1, 1, 1);
    }
    else if (dest == ColorSpace::rgb) {
      return Color::rgb(pstate,
        red.has_value() ? red.value() * 255.0 : red,
        green.has_value() ? green.value() * 255.0 : green,
        blue.has_value() ? blue.value() * 255.0 : blue,
        alpha);
    }
    else if (dest == ColorSpace::srgb_linear) {
      auto rv = Color::forSpaceInternal(pstate, dest,
        red.has_value() ? toLinear(red.value()) : red,
        green.has_value() ? toLinear(green.value()) : green,
        blue.has_value() ? toLinear(blue.value()) : blue,
        alpha);
      return rv;
    }
    else {
      return ColorSpace::convertLinear(dest,
        pstate, red, green, blue, alpha,
        missingLightness, missingChroma, missingHue);
    }
    return nullptr;
  }

  Color* RgbColorSpace::convert(
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

    // std::cerr << "CALL XYZD50 translate " << x.value_or(-42) << ", "
    //   << y.value_or(-42) << ", " << z.value_or(-42) << ", "
    //   << alpha.value_or(-42) << "\n";

    if (dest == ColorSpace::lab || dest == ColorSpace::lch) {
      // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
      // and http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
      double f0 = _convertComponentToLabF(x.value_or(0) / d50[0]);
      double f1 = _convertComponentToLabF(y.value_or(0) / d50[1]);
      double f2 = _convertComponentToLabF(z.value_or(0) / d50[2]);

      tl::optional<double> lightness;
      double a = 500 * (f0 - f1);
      double b = 200 * (f1 - f2);
      if (!missingLightness) { lightness = (116.0 * f1) - 16.0; }

      if (dest == ColorSpace::lab) {
        auto rv = Color::lab(pstate, lightness,
          missingA ? tl::optional<double>() : a,
          missingB ? tl::optional<double>() : b,
          alpha);
        // std::cerr << "==lab== " << rv->debug() << "\n";
        return rv;
      }
      else {
        auto rv = Color::labToLch(pstate,
          ColorSpace::lch, lightness, a, b, alpha,
          missingChroma, missingHue);
        // std::cerr << "==lch== " << rv->debug() << "\n";
        return rv;
      }
    }

    auto rv = ColorSpace::convertLinear(
      dest, pstate,
      x, y, z, alpha,
      missingLightness,
      missingChroma,
      missingHue,
      missingA,
      missingB);

    return rv;
  }

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

    if (dest == ColorSpace::oklab) {
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
    else if (dest == ColorSpace::oklch) {
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

  Color* OkLchColorSpace::convert(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> lightness, tl::optional<double> chroma, tl::optional<double> hue, tl::optional<double> alpha) const
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

  Color* OkLabColorSpace::translate(const ColorSpace& dest, const SourceSpan& pstate,
    tl::optional<double> lightness, tl::optional<double> a, tl::optional<double> b, tl::optional<double> alpha,
    bool missingChroma, bool missingHue) const
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
      missingLightness,
      missingChroma,
      missingHue,
      missingA,
      missingB);
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

  static double nnan(double val) {
    if (std::isnan(val)) return NaN;
    return val;
  }

  Color* HwbColorSpace::convert(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> hue, tl::optional<double> whiteness, tl::optional<double> blackness, tl::optional<double> alpha) const
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
    return ColorSpace::srgb.translate(
      dest, pstate,
      nnan(hueToRgb(0.0, 1.0, scaledHue + 1.0 / 3.0)* factor + scaledWhiteness),
      nnan(hueToRgb(0.0, 1.0, scaledHue)* factor + scaledWhiteness),
      nnan(hueToRgb(0.0, 1.0, scaledHue - 1.0 / 3.0)* factor + scaledWhiteness),
      alpha,
      false, false,
      !hue.has_value());
  }

  Color* LchColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> lightness,
    tl::optional<double> chroma,
    tl::optional<double> hue,
    tl::optional<double> alpha) const
  {
    double hueRadians = hue.value_or(0) * PI / 180.0;
    auto rv = ColorSpace::lab.translate(dest, pstate,
      lightness,
      chroma.value_or(0) * std::cos(hueRadians),
      chroma.value_or(0) * std::sin(hueRadians),
      alpha,
      !chroma.has_value(),
      !hue.has_value());
    return rv;
  }

  /// Converts an f-format component to the X or Z channel of an XYZ color.
  double _convertFToXorZ(double component) {
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

    if (dest == ColorSpace::lab)
    {
      bool powerlessAB = !lightness.has_value() || fuzzyEquals(lightness.value(), 0, sass::epsilon);
      return Color::lab(pstate,
        lightness,
        !a.has_value() || powerlessAB ? tl::optional<double>() : a,
        !b.has_value() || powerlessAB ? tl::optional<double>() : b,
        alpha);
    }
    else if (dest == ColorSpace::lch)
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

      return ColorSpace::xyzd50.translate(
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

  Color* SrgbLinearColorSpace::convert(
    const ColorSpace& dest,
    const SourceSpan& pstate,
    tl::optional<double> red,
    tl::optional<double> green,
    tl::optional<double> blue,
    tl::optional<double> alpha) const
  {
    if (dest == ColorSpace::rgb || dest == ColorSpace::hsl ||
        dest == ColorSpace::hwb || dest == ColorSpace::srgb)
    {
      return ColorSpace::srgb.convert(
        dest, pstate,
        red.transform(srgbAndDisplayP3FromLinear),
        green.transform(srgbAndDisplayP3FromLinear),
        blue.transform(srgbAndDisplayP3FromLinear),
        alpha);
    }
    else {
      return ColorSpace::convert(dest, pstate, red, green, blue, alpha);
    }
  }


  Color* HslColorSpace::convert(const ColorSpace& dest, const SourceSpan& pstate, tl::optional<double> hue, tl::optional<double> saturation, tl::optional<double> lightness, tl::optional<double> alpha) const
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

    return ColorSpace::srgb.translate(
      dest, pstate,
      hueToRgb(m1, m2, scaledHue + 1.0 / 3.0),
      hueToRgb(m1, m2, scaledHue),
      hueToRgb(m1, m2, scaledHue - 1.0 / 3.0),
      alpha,
      !lightness.has_value(),
      !saturation.has_value(),
      !hue.has_value());
  }

  static double clampLikeCss(double val, double min, double max) {
    return std::isnan(val) ? min : std::min(std::max(val, min), max);
  }

  tl::optional<double> _clampChannel(tl::optional<double> value, const ColorChannel& channel)
  {
    if (value.has_value() == false) return value;
    if (channel.isLinear == false) return value;
    return clampLikeCss(value.value(), channel.min, channel.max);
  }

  const ClipGamutMap GamutMapMethod::clip = ClipGamutMap();
  const LocalMindeGamutMap GamutMapMethod::localMinde = LocalMindeGamutMap();


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
    auto lab1 = color1->toSpace(ColorSpace::oklab, color1->pstate());
    auto lab2 = color2->toSpace(ColorSpace::oklab, color2->pstate());

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
    auto originOklch = color->toSpace(ColorSpace::oklch, color->pstate());

    // The channel equivalents to `current` in the Color 4 algorithm.
    auto lightness = originOklch->getChannel0OrNull();
    auto hue = originOklch->getChannel2OrNull();
    auto alpha = originOklch->getAlphaOrNull();

    if (fuzzyGreaterThanOrEquals(lightness.value_or(0), 1.0, sass::epsilon)) {
      if (color->isLegacy()) return Color::rgb(
        color->pstate(), 255, 255, 255, color->getAlphaOrNull())
        ->toSpace(color->space(), color->pstate());
      return Color::forSpaceInternal(
        color->pstate(), color->space(),
        1, 1, 1, color->getAlphaOrNull());
    }
    else if (fuzzyLessThanOrEquals(lightness.value_or(0), 0.0, sass::epsilon)) {
      return Color::rgb(
        color->pstate(),
        0, 0, 0, color->getAlphaOrNull())
        ->toSpace(color->space(), color->pstate());
    }

    Color* clipped = color->toGamut(GamutMapMethod::clip);

    if (_deltaEOK(clipped, color) < _jnd) return clipped;

    double min = 0.0;
    double max = originOklch->getChannel1();
    bool minInGamut = true;
    while (max - min > _epsilon) {
      double chroma = (min + max) / 2.0;

      // In the Color 4 algorithm `current` is in Oklch, but all its actual uses
      // other than modifying chroma convert it to `color.space` first so we
      // just store it in that space to begin with.
      Color* current = ColorSpace::oklch.convert(
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
        if (_jnd - e < _epsilon) return clipped;
        minInGamut = false;
        min = chroma;
      }
      else {
        max = chroma;
      }
    }
    return clipped;

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

  ///////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

  ColorExpression::ColorExpression(
    SourceSpan pstate,
    Color* value) :
    Expression(std::move(pstate)),
    value_(value)
  {
  }

  // Convert to string (only for debugging)
  sass::string ColorExpression::toString() const
  {
    return value_->inspect();
  }

  ///////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

}
