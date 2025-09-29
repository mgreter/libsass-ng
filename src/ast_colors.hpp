/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_AST_COLORS_HPP
#define SASS_AST_COLORS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_nodes.hpp"
#include "col_consts.hpp"
#include "col_channel.hpp"
#include "col_spaces.hpp"
#include "ast_values.hpp"

namespace Sass {


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  
  ///////////////////////////////////////////////////////////////////////
  // A sass color in RGBA representation.
  ///////////////////////////////////////////////////////////////////////

  enum HueInterpolationMethod {
    shorter,
    longer,
    increasing,
    decreasing
  };

  class InterpolationMethod {

  public:

    const ColorSpace& space;

    HueInterpolationMethod hue = shorter;

    InterpolationMethod(
      const ColorSpace& space,
      HueInterpolationMethod hue = shorter) :
      space(space), hue(hue)
    {}

    static InterpolationMethod fromValue(Logger& ctx, Value* value, const sass::string& name);

    static HueInterpolationMethod hueFromValue(Logger& ctx, Value* value, const sass::string& name);

  };


  class Color final : public Value
  {
  private:

    ADD_CONSTREF(sass::string, disp);
    ADD_CONSTREF(bool, parsed);

    const ColorSpace& space_;
    ADD_PROPERTY(tl::optional<double>, c0);
    ADD_PROPERTY(tl::optional<double>, c1);
    ADD_PROPERTY(tl::optional<double>, c2);
    ADD_PROPERTY(tl::optional<double>, alpha);


  public:

    // ADD_PROPERTY(bool, frgb);
    bool forceRgb = false;

    Color* interpolate(
      Logger& logger,
      const SourceSpan& pstate,
      Color* other,
      InterpolationMethod method,
      double weight = 0.5,
      bool legacyMissing = true);

    // sass::string debug() const;

    const ColorSpace& space() const {
      return space_;
    }

    bool hasMissingChannels() const {
      return !c0_.has_value()
        || !c1_.has_value()
        || !c2_.has_value()
        || !alpha_.has_value();
    }

    Color* toGamut(const GamutMapMethod& method) {
      return isInGamut() ? this : method.map(this);
    }

    tl::optional<double> getChannel0OrNull() const;
    tl::optional<double> getChannel1OrNull() const;
    tl::optional<double> getChannel2OrNull() const;
    tl::optional<double> getChannelOrNull(int idx) const;
    tl::optional<double> getAlphaOrNull() const;

    double getChannel0() const;
    double getChannel1() const;
    double getChannel2() const;
    double getChannel(int idx) const;
    double getAlpha() const;

    bool isInGamut() const;

    bool isLegacy() const;

    bool isChannel0Missing() const;
    bool isChannel1Missing() const;
    bool isChannel2Missing() const;

    bool isChannelMissing(int idx) const;
    bool isAlphaMissing() const;

    int getChannelIndex(Logger& logger, const String* channel,
      const char* colorName, const char* channelName) const;

    bool isChannelMissing(Logger& logger, const String* channel, const sass::string& arg) const;
    bool isChannelMissing(Logger& logger, const sass::string& channel) const;

    bool isChannel0Powerless() const;
    bool isChannel1Powerless() const;
    bool isChannel2Powerless() const;
    bool isChannelPowerless(Logger& logger, const String* channel,
      const char* colorName, const char* channelName) const;

    virtual Color* toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing = true) const;

    virtual Color* toSpace(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing = true);
    // virtual Color* toSpace2(const ColorSpace& space, const SourceSpan& pstate, bool legacyMissing = true) const;

  public:


    static Color* _forSpace(
      const SourceSpan& pstate, const ColorSpace& space,
      tl::optional<double> c0, tl::optional<double> c1,
      tl::optional<double> c2, tl::optional<double> alpha
      /*,Logger& logger, format */)
    {
      Color* color = SASS_MEMORY_NEW(Color, pstate, space, c0, c1, c2,
        alpha/*.and_then([&](double a) { return tl::optional<double>(fuzzyAssertRange(a, 0, 1, logger)); })*/);
      // assert(space == ColorSpace::rgb);
      // assert(space != ColorSpace::lms);
      return color;
    }

    static Color* rgb(
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
    {
      return rgbInternal(pstate, red, green, blue, alpha);
    }

    static Color* hsl(
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
    {
      auto rv = forSpaceInternal(pstate,
        ColorSpace::hsl,
        red, green, blue, alpha);
      return rv;
    }

    static Color* hwb(
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
    {
      auto rv = forSpaceInternal(pstate,
        ColorSpace::hwb,
        red, green, blue, alpha);
      return rv;
    }

    static Color* rgbInternal(
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool forceRgb = false)
    {
      auto rv = _forSpace(pstate,
        ColorSpace::rgb,
        red, green, blue, alpha);
      if (rv != nullptr) rv->forceRgb = forceRgb;
      return rv;
    }

    static Color* lab(
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> a,
      tl::optional<double> b,
      tl::optional<double> alpha)
    {
      auto rv = _forSpace(pstate,
        ColorSpace::lab,
        lightness, a, b, alpha);
      return rv;
    }

    static Color* xyzD65(
      const SourceSpan& pstate,
      tl::optional<double> x,
      tl::optional<double> y,
      tl::optional<double> z,
      tl::optional<double> alpha)
    {
      auto rv = _forSpace(pstate,
        ColorSpace::xyzd65,
        x, y, z, alpha);
      return rv;
    }

    static tl::optional<double> _normalizeHue(
      tl::optional<double> hue, bool invert)
    {
      if (hue.has_value() == false) return hue;
      auto rv = std::fmod(std::fmod(hue.value(), 360.0)
        + 360.0 + (invert ? 180.0 : 0.0), 360.0);
      // std::cerr << "norm hue " << rv << "\n";
        return rv;
    }

    static Color* forSpaceInternal(
      const SourceSpan& pstate, const ColorSpace& space,
      tl::optional<double> c0, tl::optional<double> c1,
      tl::optional<double> c2, tl::optional<double> alpha)
    {

       // std::cerr << "=> forSpaceInternal " << space.name() << " " << c0.value_or(-32) << ", "
       //   << c1.value_or(-42) << ", " << c2.value_or(-52) << "\n";

      if (space == ColorSpace::hsl) {
        return _forSpace(pstate, space,
          _normalizeHue(c0, c1.value_or(0) < 0.0),
          c1.has_value() ? std::abs(c1.value()) : c1,
          c2, alpha);
      }
      else if (space == ColorSpace::hwb)
      {
        return _forSpace(pstate, space,
          _normalizeHue(c0, false),
          c1, c2, alpha);
      }
      else if (space == ColorSpace::lch || space == ColorSpace::oklch) {
        auto rv = _forSpace(pstate, space,
          c0,
          c1.has_value() ? std::abs(c1.value()) : c1,
          _normalizeHue(c2, c1.value_or(0) < 0.0), // fuzzyLessThan
          alpha);
        // std::cerr << "for space " << rv->debug() << "\n";
        return rv;
      }
      else {
        auto rv = _forSpace(pstate, space, c0, c1, c2, alpha);
        // std::cerr << "for space " << rv->debug() << "\n";
        return rv;
      }
    }

    Color* changeAlpha(double alpha) {
      return Color::forSpaceInternal(
        pstate_, space_,
        c0_, c1_, c2_,
        alpha);
    }

    static Color* oklab(
      const SourceSpan& pstate, const ColorSpace& space,
      tl::optional<double> lightness, tl::optional<double> a,
      tl::optional<double> b, tl::optional<double> alpha)
    {
      return Color::_forSpace(pstate,
        space, lightness, a, b, alpha);

    }

    static Color* labToLch(
      const SourceSpan& pstate, const ColorSpace& space,
      tl::optional<double> lightness, tl::optional<double> a,
      tl::optional<double> b, tl::optional<double> alpha,
      bool missingChroma = false, bool missingHue = false)
    {
      // Algorithm from https://www.w3.org/TR/css-color-4/#color-conversion-code
      tl::optional<double> chroma, hue;
      // Requires computation in any case
      double chromatic = std::sqrt(
        std::pow(a.value_or(0), 2.0) +
        std::pow(b.value_or(0), 2.0));

      if (missingChroma == false) {
        chroma = chromatic;
      }
      if (missingHue == false) {
        if (!fuzzyEquals(chromatic, 0.0, 0.0000001)) {
          hue = std::atan2(b.value_or(0), a.value_or(0)) * 180.0 / PI;
          if (hue < 0) hue.value() += 360.0;
        }
      }
      auto rv = Color::forSpaceInternal(pstate,
        space, lightness, chroma, hue, alpha);
      return rv;
    }

    // static ColorObj _forSpaceInternal(
    //   const SourceSpan& pstate, const ColorSpace& space,
    //   tl::optional<double>* c, tl::optional<double> alpha,
    //   Logger& logger/*, format */)
    // {
    //   ColorObj color = SASS_MEMORY_NEW(Color, pstate, space, c[0], c[1], c[2],
    //     alpha.and_then([&](double a) { return tl::optional<double>(fuzzyAssertRange(a, 0, 1, logger)); }));
    //   // assert(space == ColorSpace::rgb);
    //   assert(space != ColorSpace::lms);
    //   return color.detach();
    // }

    // Value constructor
    Color(const SourceSpan& pstate,
      const ColorSpace& space,
      double c0, double c1,
      double c2, double alpha = 1.0,
      const sass::string& disp = "",
      bool parsed = false);

    Color(const SourceSpan& pstate,
      const ColorSpace& space,
      tl::optional<double> c0,
      tl::optional<double> c1,
      tl::optional<double> c2,
      tl::optional<double> alpha,
      const sass::string& disp = "",
      bool parsed = false);

    // Copy constructor
    Color(const Color* ptr);

    double channel(const sass::string& channel) const;

    // Implement interface for base value class
    size_t hash() const final;
    SassValueType getTag() const final { return SASS_COLOR; }
    const sass::string& type() const final { return Strings::color; }

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) final {
      return visitor->visitColor(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) final {
      return visitor->visitColor(this);
    }

    // Implement some operations for base value class
    Value* plus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* minus(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* dividedBy(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* modulo(const Value* other, Logger& logger, const SourceSpan& pstate) const final;
    Value* remainder(const Value* other, Logger& logger, const SourceSpan& pstate) const final;

    // Im ement equality comparators for base value class
    bool operator==(const Value& rhs) const final;
    // Implement same class compare operator
    bool operator==(const Color& rhs) const;

    const Color* assertColor(Logger& logger, const sass::string& name = Strings::empty) const final { return this; }
    Color* assertColor2(Logger& logger, const sass::string& name = Strings::empty) final { return this; }

    // Copy operations for childless items
    Color* copy(SASS_MEMORY_ARGS bool childless) const final {
      return SASS_MEMORY_NEW_DBG(Color, this);
    }

    struct HashFunction
    {
      size_t operator()(const Color& color) const
      {
        return color.hash();
      }
    };

    IMPLEMENT_ISA_CASTER(Color);
  };



  ///////////////////////////////////////////////////////////////////////
  // A sass color in HSLA representation.
  ///////////////////////////////////////////////////////////////////////

  class ColorExpression final : public Expression
  {
  private:

    // Color wrapped inside this expression
    ADD_CONSTREF(ColorObj, value);

  public:

    // Value constructor
    ColorExpression(
      SourceSpan pstate,
      Color* color);

    // Expression visitor to sass values entry function
    Value* accept(ExpressionVisitor<Value*>* visitor) final {
      return visitor->visitColorExpression(this);
    }
    Expression* accept(ExpressionVisitor<Expression*>* visitor) final {
      return visitor->visitColorExpression(this);
    }

    // Return if expression can be used in calculations
    bool isCalcSafe() final { return false; }

    // Convert to string (only for debugging)
    sass::string toString() const final;

    // Implement specialized up-casting method
    IMPLEMENT_ISA_CASTER(ColorExpression);
  };

  ///////////////////////////////////////////////////////////////////////
  // A sass color in HSLA representation.
  ///////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
