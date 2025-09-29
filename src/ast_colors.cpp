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
  
  static bool _isAnalogousChannelMissing(
    Logger& logger, Color* original,
    Color* output, int outputChannelIndex)
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

    ColorObj color1 = this->toSpace(method.space, pstate);
    ColorObj color2 = other->toSpace(method.space, pstate);

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

    if (method.space == ColorSpaces::hsl || method.space == ColorSpaces::hwb) {
      rv = Color::forSpaceInternal(pstate, method.space,
        missing1_0 && missing2_0 ? tl::optional<double>() :
          _interpolateHues(channel1_0, channel2_0, method.hue, weight),
        mixed1, mixed2, mixedAlpha);
    }
    else if (method.space == ColorSpaces::lch || method.space == ColorSpaces::oklch) {
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
    return space().isLegacy();
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
    if (space_ == ColorSpaces::lch || space_.name() == "oklch") {
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
    if (space == this->space_) {
      // dbg = true;
      return this;
    }
    // If const, we must always create a copy
    const Color* color = this;
    ColorObj converted = color->toSpace(space, pstate, legacyMissing);
    return converted.detach();;
  }

  Color* Color::toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing) const
  {
    if (space == this->space_) {
      ColorObj copy = SASS_MEMORY_NEW(Color, this);
      return copy.detach();
    }

    ColorObj converted = this->space_.convert(space, pstate, c0_, c1_, c2_, alpha_);

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
      return converted.detach();
    }

    return converted.detach();
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
      ColorObj rgb1 = toSpace(ColorSpaces::rgb, pstate());
      ColorObj rgb2 = rhs.toSpace(ColorSpaces::rgb, rhs.pstate());
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
