/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_COL_CHANNEL_HPP
#define SASS_COL_CHANNEL_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

// #include "ast_nodes.hpp"
// #include "ast_values.hpp"
#include "shim/optional.hpp"
#include "ast_fwd_decl.hpp"
#include "ast_def_macros.hpp"
#include "memory_allocator.hpp"

class ColorSpaced;

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class GamutMapMethod;
  class ClipGamutMap;
  class LocalMindeGamutMap;


  class GamutMapMethod {

  public:

    // The Sass name of the gamut-mapping algorithm.
    sass::string name;

    static const ClipGamutMap clip;
    static const LocalMindeGamutMap localMinde;

  public:

    GamutMapMethod(const sass::string& name) : name(name) {}

    virtual ColorSpaced* map(ColorSpaced* color) const = 0;

    static const GamutMapMethod& fromName(Logger& logger,
      Value* value, const sass::string& vname);

  };

  class ClipGamutMap : public GamutMapMethod {

  public:

    ClipGamutMap() : GamutMapMethod("clip") {}
    ColorSpaced* map(ColorSpaced* color) const override final;

  };

  class LocalMindeGamutMap : public GamutMapMethod {
  public:
    LocalMindeGamutMap() : GamutMapMethod("local-minde") {}
    ColorSpaced* map(ColorSpaced* color) const override final;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class ColorChannel {

  public:
    double min = -9999999;
    double max = +9999999;
    bool requiresPercent = false;
    bool lowerClamped = false;
    bool upperClamped = false;
    bool conventionallyPercent = false;
    bool isLinear = false;

  public:

    sass::string name;
    bool isPolarAngle;
    sass::string unit;


    ColorChannel(sass::string name, bool isPolarAngle, sass::string unit)
      : name(name), isPolarAngle(isPolarAngle), unit(unit)
    {
    }

    bool isAnalogous(const ColorChannel& other) const {
      if (name == "x" && other.name == "red") return true;
      if (name == "red" && other.name == "x") return true;
      if (name == "y" && other.name == "green") return true;
      if (name == "green" && other.name == "y") return true;
      if (name == "z" && other.name == "blue") return true;
      if (name == "blue" && other.name == "z") return true;
      if (name == "chroma" && other.name == "saturation") return true;
      if (name == "saturation" && other.name == "chroma") return true;
      if (name == "lightness" && other.name == "lightness") return true;
      if (name == "hue" && other.name == "hue") return true;
      return false;
    }


    virtual ~ColorChannel() = default;
  };

  class LinearChannel : public ColorChannel {

  public:
    LinearChannel(sass::string name, double min, double max,
      bool requiresPercent, bool lowerClamped, bool upperClamped,
      bool conventionallyPercent = false, sass::string unit = "")
      : ColorChannel(name, false, unit)
    {
      this->min = min;
      this->max = max;
      this->requiresPercent = requiresPercent;
      this->lowerClamped = lowerClamped;
      this->upperClamped = upperClamped;
      this->conventionallyPercent = conventionallyPercent;
      this->isLinear = true;
    }

  };

  class HwbColorSpace;
  class HslColorSpace;
  class LabColorSpace;
  class LchColorSpace;
  class OkLabColorSpace;
  class OkLchColorSpace;
  class RgbColorSpace;

  class SrgbColorSpace;
  class SrgbLinearColorSpace;
  class XyzD50ColorSpace;
  class XyzD65ColorSpace;
  class Rec2020ColorSpace;
  class DisplayP3ColorSpace;
  class ProphotoRgbColorSpace;
  class A98RgbColorSpace;
  class LmsColorSpace;

  class ColorSpace {

    ADD_CONSTREF(sass::string, name);
    ADD_CONSTREF(SassColorSpace, space);

  public:

    int _channelSize;
    const ColorChannel* _channels;

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

    ColorSpace(const sass::string name, SassColorSpace space, const ColorChannel* channels, int channelSize)
      : name_(name), space_(space), _channels(channels), _channelSize(channelSize)
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

    virtual ColorSpaced* convertLinear(
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


    virtual ColorSpaced* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> channel0,
      tl::optional<double> channel1,
      tl::optional<double> channel2,
      tl::optional<double> alpha) const
    {
      // std::cerr << "CALL CONVERT LINEAR\n";
      return convertLinear(dest, pstate, channel0, channel1, channel2, alpha);
    }

    bool operator==(const ColorSpace& rhs) const {
      return rhs.space_ == space_;
    }

    bool operator!=(const ColorSpace& rhs) const {
      return rhs.space_ == space_;
    }


  };

  namespace ColorSpace2 {

    extern const HwbColorSpace hwb;
    extern const HslColorSpace hsl;
    extern const LabColorSpace lab;
    extern const LchColorSpace lch;
    extern const OkLabColorSpace oklab;
    extern const OkLchColorSpace oklch;
    extern const RgbColorSpace rgb;
    extern const SrgbColorSpace srgb;
    extern const SrgbLinearColorSpace srgb_linear;
    extern const XyzD50ColorSpace xyzd50;
    extern const XyzD65ColorSpace xyzd65;
    extern const Rec2020ColorSpace rec2020;
    extern const DisplayP3ColorSpace displayP3;
    extern const A98RgbColorSpace a98rgb;
    extern const ProphotoRgbColorSpace protophotoRgb;
    extern const LmsColorSpace lms;

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // extern const ColorChannel hsl_channel0;
  // extern const LinearChannel hsl_channel1;
  // extern const LinearChannel hsl_channel2;

  // const ColorChannel hsl_channels[3]{
  //   ColorChannel("hue", true, "deg"),
  //   LinearChannel("saturation", 0, 100, false, true, false),
  //   LinearChannel("lightness", 0, 100, true, false, false)
  // };

}

#endif
