/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_COL_CHANNEL_HPP
#define SASS_COL_CHANNEL_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_fwd_decl.hpp"
#include "ast_def_macros.hpp"
#include "memory_allocator.hpp"
#include "shim/optional.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

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

    virtual Color* map(Color* color) const = 0;

    static const GamutMapMethod& fromName(Logger& logger,
      Value* value, const sass::string& vname);

  };

  class ClipGamutMap : public GamutMapMethod {

  public:

    ClipGamutMap() : GamutMapMethod("clip") {}
    // virtual ~ClipGamutMap() {}
    Color* map(Color* color) const final;

  };

  class LocalMindeGamutMap : public GamutMapMethod {
  public:
    LocalMindeGamutMap() : GamutMapMethod("local-minde") {}
    // virtual ~LocalMindeGamutMap() {}
    Color* map(Color* color) const final;

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
    bool isLinear = false;

  public:

    sass::string name;
    bool isPolarAngle;
    sass::string unit;

  private:

    ColorChannel(sass::string name, bool isPolarAngle, sass::string unit)
      : name(name), isPolarAngle(isPolarAngle), unit(unit)
    {
    }

  public:

    inline bool isAnalogous(const ColorChannel& other) const
    {
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

    static ColorChannel polar(sass::string name, bool isPolarAngle, sass::string unit)
    {
      ColorChannel channel(name, false, unit);
      return channel;
    }

      static ColorChannel linear(sass::string name, double min, double max,
      bool requiresPercent, bool lowerClamped, bool upperClamped,
      sass::string unit = "")
    {
      ColorChannel channel(name, false, unit);
      channel.min = min;
      channel.max = max;
      channel.requiresPercent = requiresPercent;
      channel.lowerClamped = lowerClamped;
      channel.upperClamped = upperClamped;
      channel.isLinear = true;
      return channel;
    }


    virtual ~ColorChannel() = default;
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  const ColorChannel HslColorChannels[3]{
    ColorChannel::polar("hue", true, "deg"),
    ColorChannel::linear("saturation", 0, 100, false, true, false, "%"), // conf1
    ColorChannel::linear("lightness", 0, 100, true, false, false, "%") // conf2
  };

  const ColorChannel HwbColorChannels[3]{
    ColorChannel::polar("hue", true, "deg"),
    ColorChannel::linear("whiteness", 0, 100, true, false, false, "%"), // conf2
    ColorChannel::linear("blackness", 0, 100, true, false, false, "%") // conf2
  };

  const ColorChannel LabColorChannels[3]{
    ColorChannel::linear("lightness", 0, 100, false, true, true, "%"), // conf4
    ColorChannel::linear("a", -125, 125, false, false, false), // conf3
    ColorChannel::linear("b", -125, 125, false, false, false) // conf3
  };

  const ColorChannel LchColorChannels[3]{
    ColorChannel::linear("lightness", 0, 100, false, true, true, "%"), // conf4
    ColorChannel::linear("chroma", 0, 150, false, true, false),// conf1
    ColorChannel::polar("hue", true, "deg"),
  };

  const ColorChannel LmsColorChannels[3]{
    ColorChannel::linear("long", 0, 1, false, false, false), // conf3
    ColorChannel::linear("medium", 0, 1, false, false, false), // conf3
    ColorChannel::linear("short", 0, 1, false, false, false) // conf3
  };

  const ColorChannel OkLabColorChannels[3]{
    ColorChannel::linear("lightness", 0, 1, false, true, true, "%"),
    ColorChannel::linear("a", -0.4, 0.4, false, false, false), // conf3
    ColorChannel::linear("b", -0.4, 0.4, false, false, false) // conf3
  };

  const ColorChannel OkLchColorChannels[3]{
    ColorChannel::linear("lightness", 0, 1, false, true, true, "%"),
    ColorChannel::linear("chroma", 0, 0.4, false, true, false),// conf1
    ColorChannel::polar("hue", true, "deg"),
  };

  const ColorChannel RgbColorChannels[3]{
    ColorChannel::linear("red", 0, 1, false, false, false), // conf3
    ColorChannel::linear("green", 0, 1, false, false, false), // conf3
    ColorChannel::linear("blue", 0, 1, false, false, false) // conf3
  };

  const ColorChannel Rgb255ColorChannels[3]{
    ColorChannel::linear("red", 0, 255, false, true, true), // conf4
    ColorChannel::linear("green", 0, 255, false, true, true), // conf4
    ColorChannel::linear("blue", 0, 255, false, true, true) // conf4
  };

  const ColorChannel XyzColorChannels[3]{
    ColorChannel::linear("x", 0, 1, false, false, false), // conf3
    ColorChannel::linear("y", 0, 1, false, false, false), // conf3
    ColorChannel::linear("z", 0, 1, false, false, false) // conf3
  };

  // const ColorChannel Xyz255ColorChannels[3]{
  //   LinearChannel("x", 0, 255, false, true, true),
  //   LinearChannel("y", 0, 255, false, true, true),
  //   LinearChannel("z", 0, 255, false, true, true)
  // };

  const ColorChannel AlphaChannel = ColorChannel::
    linear("alpha", 0, 1, false, false, false);
}

#endif
