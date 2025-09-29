/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_COL_SPACES_HPP
#define SASS_COL_SPACES_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_fwd_decl.hpp"
#include "shim/optional.hpp"
#include "ast_def_macros.hpp"
#include "col_consts.hpp"
#include "col_channel.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  /// A constant used in the rec2020 gamma encoding/decoding functions.
  constexpr double rec2020_alpha = 1.09929682680944;

  /// A constant used in the rec2020 gamma encoding/decoding functions.
  constexpr double rec2020_beta = 0.018053968510807;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ColorSpace {

    ADD_CONSTREF(sass::string, name);
    ADD_CONSTREF(SassColorSpace, space);

  public:

    int _channelSize;
    const ColorChannel* _channels;

    virtual ~ColorSpace() {}

    virtual bool isLegacy() const {
      return false;
    }

    virtual bool isPolar() const {
      return false;
    }

    virtual bool isBounded() const {
      return true;
    }

    static const ColorSpace& fromValueRef(Logger& logger, Value* value, const sass::string& name);
    static const ColorSpace& fromNameRef(Logger& logger, const String& space, const sass::string& name);

    static const ColorSpace* fromName(Logger& logger, const String& space, const sass::string& name);

    ColorSpace(const sass::string name, SassColorSpace space, const ColorChannel* channels, int channelSize = 3)
      : name_(name), space_(space), _channelSize(channelSize), _channels(channels)
    {
      // std::cerr << "init colorspace " << name << " " << this << "\n";
    }

    int getChannelIndex(const sass::string& name) const
    {
      for (int i = 0; i < _channelSize; i++) {
        if (_channels[i].name == name) return i;
      }
      return -1;
    }

    virtual double toLinear(double channel) const;
    virtual double fromLinear(double channel) const;
    virtual const double* transformationMatrix(ColorSpace dest) const;

    virtual Color* convertLinear(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool missingLightness = false,
      bool missingChroma = false,
      bool missingHue = false,
      bool missingA = false,
      bool missingB = false)
        const;


    virtual Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> channel0,
      tl::optional<double> channel1,
      tl::optional<double> channel2,
      tl::optional<double> alpha)
        const
    {
      // std::cerr << "CALL CONVERT LINEAR\n";
      return convertLinear(dest, pstate, channel0, channel1, channel2, alpha);
    }

    bool operator==(const ColorSpace& rhs) const {
      return rhs.space_ == space_; // compare ints
    }

    bool operator!=(const ColorSpace& rhs) const {
      return rhs.space_ == space_; // compare ints
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class A98RgbColorSpace : public ColorSpace
  {

  public:

    A98RgbColorSpace() : ColorSpace("a98-rgb",
      SassColorSpace::A98RGB, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final
    {
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(std::abs(channel), 563.0 / 256.0);
    }
    inline double fromLinear(double channel) const final
    {
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(std::abs(channel), 256.0 / 563.0);
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
      case SassColorSpace::LMS: return ColorSpaces::linearA98RgbToLms;
      case SassColorSpace::RGB: return ColorSpaces::linearA98RgbToLinearSrgb;
      case SassColorSpace::SRGB: return ColorSpaces::linearA98RgbToLinearSrgb;
      case SassColorSpace::SRGB_LINEAR: return ColorSpaces::linearA98RgbToLinearSrgb;
      case SassColorSpace::DISPLAY_P3: return ColorSpaces::linearA98RgbToLinearDisplayP3;
      case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::linearA98RgbToLinearProphotoRgb;
      case SassColorSpace::REC2020: return ColorSpaces::linearA98RgbToLinearRec2020;
      case SassColorSpace::XYZ_D65: return ColorSpaces::linearA98RgbToXyzD65;
      case SassColorSpace::XYZ_D50: return ColorSpaces::linearA98RgbToXyzD50;
      default: return ColorSpace::transformationMatrix(dest);
      }
    }

  };

  inline double srgbAndDisplayP3ToLinear(double channel) {
    double abs = std::abs(channel);
    if (abs <= 0.04045) return channel / 12.92;
    return (std::signbit(channel) ? -1 : +1) *
      std::pow((abs + 0.055) / 1.055, 2.4);
  }

  inline double srgbAndDisplayP3FromLinear(double channel) {
    double abs = std::abs(channel);
    if (abs <= 0.0031308) return channel * 12.92;
    return (std::signbit(channel) ? -1 : +1) *
      (1.055 * std::pow(abs, 1.0 / 2.4) - 0.055);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class DisplayP3ColorSpace : public ColorSpace
  {

  public:

    DisplayP3ColorSpace() : ColorSpace("display-p3",
      SassColorSpace::DISPLAY_P3, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final
    {
      return srgbAndDisplayP3ToLinear(channel);
    }

    inline double fromLinear(double channel) const final
    {
      return srgbAndDisplayP3FromLinear(channel);
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::LMS: return ColorSpaces::linearDisplayP3ToLms;
        case SassColorSpace::RGB: return ColorSpaces::linearDisplayP3ToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::linearDisplayP3ToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::linearDisplayP3ToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::linearDisplayP3ToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::linearDisplayP3ToLinearProphotoRgb;
        case SassColorSpace::REC2020: return ColorSpaces::linearDisplayP3ToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::linearDisplayP3ToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::linearDisplayP3ToXyzD50;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class HslColorSpace : public ColorSpace
  {

  public:

    HslColorSpace() : ColorSpace("hsl",
      SassColorSpace::HSL, HslColorChannels)
    {}

    bool isLegacy() const final { return true; }
    bool isPolar() const final { return true; }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> saturation,
      tl::optional<double> lightness,
      tl::optional<double> alpha)
        const final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class HwbColorSpace : public ColorSpace
  {

  public:
    HwbColorSpace() : ColorSpace("hwb",
      SassColorSpace::HWB, HwbColorChannels)
    {}

    bool isLegacy() const final { return true; }
    bool isPolar() const final { return true; }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
        const final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class LabColorSpace : public ColorSpace
  {

  public:
    LabColorSpace() : ColorSpace("lab",
      SassColorSpace::LAB, LabColorChannels)
    {}

    bool isBounded() const final { return false; }

    // Translate to linear srgb color space
    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha,
      bool missingChroma = false,
      bool missingHue = false)
        const;

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
      const final
    {
      // Simply call out to translate
      return translate(dest, pstate,
        hue, whiteness, blackness, alpha);
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class LchColorSpace : public ColorSpace
  {

  public:
    LchColorSpace() : ColorSpace("lch",
      SassColorSpace::LCH, LchColorChannels)
    {}

    bool isPolar() const final { return true; }
    bool isBounded() const final { return false; }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
        const final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class LmsColorSpace : public ColorSpace
  {

  public:

    LmsColorSpace() : ColorSpace("lms",
      SassColorSpace::LMS, LmsColorChannels)
    {
    }

    bool isBounded() const final { return false; }

    inline double toLinear(double channel) const final {
      return channel;
    }

    inline double fromLinear(double channel) const final {
      return channel;
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::RGB: return ColorSpaces::lmsToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::lmsToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::lmsToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::lmsToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::lmsToLinearProphotoRgb;
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::lmsToLinearDisplayP3;
        case SassColorSpace::REC2020: return ColorSpaces::lmsToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::lmsToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::lmsToXyzD50;
        default: return ColorSpace::transformationMatrix(dest);
      }

    }

    // Translate to linear srgb color space
    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool missingLightness = false,
      bool missingChroma = false,
      bool missingHue = false,
      bool missingA = false,
      bool missingB = false)
        const;

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
        const final
    {
      // Simply call out to translate
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class OkLabColorSpace : public ColorSpace
  {

  public:

    OkLabColorSpace() : ColorSpace("oklab",
      SassColorSpace::OKLAB, OkLabColorChannels)
    {}

    bool isBounded() const final { return false; }

    // Translate to linear srgb color space
    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha,
      bool missingChroma = false,
      bool missingHue = false) const;

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha)
        const final
    {
      // Simply call out to translate
      return translate(dest, pstate,
        lightness, chroma, hue, alpha);
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class OkLchColorSpace : public ColorSpace
  {

  public:

    OkLchColorSpace() : ColorSpace("oklch",
      SassColorSpace::OKLCH, OkLchColorChannels)
    {}

    bool isPolar() const final { return true; }
    bool isBounded() const final { return false; }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha)
        const final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ProphotoRgbColorSpace : public ColorSpace
  {

  public:

    ProphotoRgbColorSpace() : ColorSpace("prophoto-rgb",
      SassColorSpace::PROPHOTO_RGB, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final
    {
      double abs = std::abs(channel);
      if (abs <= 16.0 / 512) return channel / 16.0;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(abs, 1.8 / 1.0);

    }

    inline double fromLinear(double channel) const final
    {
      double abs = std::abs(channel);
      if (abs < 1.0 / 512) return 16.0 * channel;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(abs, 1.0 / 1.8);
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::RGB: return ColorSpaces::linearProphotoRgbToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::linearProphotoRgbToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::linearProphotoRgbToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::linearProphotoRgbToLinearA98Rgb;
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::linearProphotoRgbToLinearDisplayP3;
        case SassColorSpace::REC2020: return ColorSpaces::linearProphotoRgbToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::linearProphotoRgbToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::linearProphotoRgbToXyzD50;
        case SassColorSpace::LMS: return ColorSpaces::linearProphotoRgbToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class Rec2020ColorSpace : public ColorSpace
  {

  public:

    Rec2020ColorSpace() : ColorSpace("rec2020",
      SassColorSpace::REC2020, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final
    {
      double abs = std::abs(channel);
      if (abs < rec2020_beta * 4.5) return channel / 4.5;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow((abs + rec2020_alpha - 1.0)
          / rec2020_alpha, 1.0 / 0.45);

    }

    inline double fromLinear(double channel) const final
    {
      double abs = std::abs(channel);
      if (abs < rec2020_beta) return 4.5 * channel;
      return (std::signbit(channel) ? -1 : +1) *
        (rec2020_alpha * std::pow(abs, 0.45)
          - (rec2020_alpha - 1.0));
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::RGB: return ColorSpaces::linearRec2020ToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::linearRec2020ToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::linearRec2020ToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::linearRec2020ToLinearA98Rgb;
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::linearRec2020ToLinearDisplayP3;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::linearRec2020ToLinearProphotoRgb;
        case SassColorSpace::XYZ_D65: return ColorSpaces::linearRec2020ToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::linearRec2020ToXyzD50;
        case SassColorSpace::LMS: return ColorSpaces::linearRec2020ToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class RgbColorSpace : public ColorSpace
  {

  public:

    RgbColorSpace() : ColorSpace("rgb",
      SassColorSpace::RGB, Rgb255ColorChannels)
    {
    }

    bool isLegacy() const final { return true; }

    double toLinear(double channel) const final
    {
      return srgbAndDisplayP3ToLinear(channel / 255.0);
    }

    double fromLinear(double channel) const final
    {
      return srgbAndDisplayP3FromLinear(channel) * 255.0;
    }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> channel0,
      tl::optional<double> channel1,
      tl::optional<double> channel2,
      tl::optional<double> alpha)
        const final;
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class SrgbLinearColorSpace : public ColorSpace
  {

  public:

    SrgbLinearColorSpace() : ColorSpace("srgb-linear",
      SassColorSpace::SRGB_LINEAR, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final {
      return channel;
    }

    inline double fromLinear(double channel) const final {
      return channel;
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::linearSrgbToLinearDisplayP3;
        case SassColorSpace::A98RGB: return ColorSpaces::linearSrgbToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::linearSrgbToLinearProphotoRgb;
        case SassColorSpace::REC2020: return ColorSpaces::linearSrgbToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::linearSrgbToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::linearSrgbToXyzD50;
        case SassColorSpace::LMS: return ColorSpaces::linearSrgbToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
        const final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class SrgbColorSpace : public ColorSpace
  {

  public:

    SrgbColorSpace() : ColorSpace("srgb",
      SassColorSpace::SRGB, RgbColorChannels)
    {}

    inline double toLinear(double channel) const final {
      return srgbAndDisplayP3ToLinear(channel);
    }

    inline double fromLinear(double channel) const final {
      return srgbAndDisplayP3FromLinear(channel);
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::linearSrgbToLinearDisplayP3;
        case SassColorSpace::A98RGB: return ColorSpaces::linearSrgbToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::linearSrgbToLinearProphotoRgb;
        case SassColorSpace::REC2020: return ColorSpaces::linearSrgbToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::linearSrgbToXyzD65;
        case SassColorSpace::XYZ_D50: return ColorSpaces::linearSrgbToXyzD50;
        case SassColorSpace::LMS: return ColorSpaces::linearSrgbToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

    // Translate to linear srgb color space
    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool missingLightness = false,
      bool missingChroma = false,
      bool missingHue = false)
        const;

    // Convert to linear srgb color space
    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
        const final
    {
      // Simply call out to translate
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class XyzD50ColorSpace : public ColorSpace
  {

  public:

    XyzD50ColorSpace() : ColorSpace("xyz-d50",
      SassColorSpace::XYZ_D50, XyzColorChannels)
    {}

    bool isBounded() const final { return false; }

    inline double toLinear(double channel) const final {
      return channel;
    }

    inline double fromLinear(double channel) const final {
      return channel;
    }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::RGB: return ColorSpaces::xyzD50ToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::xyzD50ToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::xyzD50ToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::xyzD50ToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::xyzD50ToLinearProphotoRgb;
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::xyzD50ToLinearDisplayP3;
        case SassColorSpace::REC2020: return ColorSpaces::xyzD50ToLinearRec2020;
        case SassColorSpace::XYZ_D65: return ColorSpaces::xyzD50ToXyzD65;
        case SassColorSpace::LMS: return ColorSpaces::xyzD50ToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

    // Translate to linear srgb color space
    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool missingLightness = false,
      bool missingChroma = false,
      bool missingHue = false,
      bool missingA = false,
      bool missingB = false) const;

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
        const final
    {
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class XyzD65ColorSpace : public ColorSpace
  {

  public:

    XyzD65ColorSpace() : ColorSpace("xyz",
      SassColorSpace::XYZ_D65, XyzColorChannels)
    {}

    inline double toLinear(double channel) const final {
      return channel;
    }

    inline double fromLinear(double channel) const final {
      return channel;
    }

    bool isBounded() const final { return false; }

    inline const double* transformationMatrix(ColorSpace dest) const final
    {
      switch (dest.space()) {
        case SassColorSpace::RGB: return ColorSpaces::xyzD65ToLinearSrgb;
        case SassColorSpace::SRGB: return ColorSpaces::xyzD65ToLinearSrgb;
        case SassColorSpace::SRGB_LINEAR: return ColorSpaces::xyzD65ToLinearSrgb;
        case SassColorSpace::A98RGB: return ColorSpaces::xyzD65ToLinearA98Rgb;
        case SassColorSpace::PROPHOTO_RGB: return ColorSpaces::xyzD65ToLinearProphotoRgb;
        case SassColorSpace::DISPLAY_P3: return ColorSpaces::xyzD65ToLinearDisplayP3;
        case SassColorSpace::REC2020: return ColorSpaces::xyzD65ToLinearRec2020;
        case SassColorSpace::XYZ_D50: return ColorSpaces::xyzD65ToXyzD50;
        case SassColorSpace::LMS: return ColorSpaces::xyzD65ToLms;
        default: return ColorSpace::transformationMatrix(dest);
      }
    }

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  namespace ColorSpaces {

    static const HwbColorSpace hwb;
    static const HslColorSpace hsl;
    static const LabColorSpace lab;
    static const LchColorSpace lch;
    static const OkLabColorSpace oklab;
    static const OkLchColorSpace oklch;
    static const RgbColorSpace rgb;
    static const SrgbColorSpace srgb;
    static const SrgbLinearColorSpace srgb_linear;
    static const XyzD50ColorSpace xyzd50;
    static const XyzD65ColorSpace xyzd65;
    static const Rec2020ColorSpace rec2020;
    static const DisplayP3ColorSpace displayP3;
    static const A98RgbColorSpace a98rgb;
    static const ProphotoRgbColorSpace protophotoRgb;
    static const LmsColorSpace lms;

  };

}

#endif
