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

class Color;

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

    virtual Color* map(Color* color) const = 0;

    static const GamutMapMethod& fromName(Logger& logger,
      Value* value, const sass::string& vname);

  };

  class ClipGamutMap : public GamutMapMethod {

  public:

    ClipGamutMap() : GamutMapMethod("clip") {}
    virtual ~ClipGamutMap() {}
    Color* map(Color* color) const final;

  };

  class LocalMindeGamutMap : public GamutMapMethod {
  public:
    LocalMindeGamutMap() : GamutMapMethod("local-minde") {}
    virtual ~LocalMindeGamutMap() {}
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

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  const ColorChannel HslColorChannels[3]{
    ColorChannel("hue", true, "deg"),
    LinearChannel("saturation", 0, 100, false, true, false, false, "%"), // conf1
    LinearChannel("lightness", 0, 100, true, false, false, false, "%") // conf2
  };

  const ColorChannel HwbColorChannels[3]{
    ColorChannel("hue", true, "deg"),
    LinearChannel("whiteness", 0, 100, true, false, false, false, "%"), // conf2
    LinearChannel("blackness", 0, 100, true, false, false, false, "%") // conf2
  };

  const ColorChannel LabColorChannels[3]{
    LinearChannel("lightness", 0, 100, false, true, true, false, "%"),
    LinearChannel("a", -125, 125, false, false, false),
    LinearChannel("b", -125, 125, false, false, false)
  };

  const ColorChannel LchColorChannels[3]{
    LinearChannel("lightness", 0, 100, false, true, true, false, "%"),
    LinearChannel("chroma", 0, 150, false, true, false),
    ColorChannel("hue", true, "deg"),
  };

  const ColorChannel LmsColorChannels[3]{
    LinearChannel("long", 0, 1, false, false, false),
    LinearChannel("medium", 0, 1, false, false, false),
    LinearChannel("short", 0, 1, false, false, false)
  };

  const ColorChannel OkLabColorChannels[3]{
    LinearChannel("lightness", 0, 1, false, true, true, true, "%"),
    LinearChannel("a", -0.4, 0.4, false, false, false),
    LinearChannel("b", -0.4, 0.4, false, false, false)
  };

  const ColorChannel OkLchColorChannels[3]{
    LinearChannel("lightness", 0, 1, false, true, true, true, "%"),
    LinearChannel("chroma", 0, 0.4, false, true, false),
    ColorChannel("hue", true, "deg"),
  };

  const ColorChannel RgbColorChannels[3]{
    LinearChannel("red", 0, 1, false, false, false),
    LinearChannel("green", 0, 1, false, false, false),
    LinearChannel("blue", 0, 1, false, false, false)
  };

  const ColorChannel Rgb255ColorChannels[3]{
    LinearChannel("red", 0, 255, false, true, true),
    LinearChannel("green", 0, 255, false, true, true),
    LinearChannel("blue", 0, 255, false, true, true)
  };

  const ColorChannel XyzColorChannels[3]{
    LinearChannel("x", 0, 1, false, false, false),
    LinearChannel("y", 0, 1, false, false, false),
    LinearChannel("z", 0, 1, false, false, false)
  };

  // const ColorChannel Xyz255ColorChannels[3]{
  //   LinearChannel("x", 0, 255, false, true, true),
  //   LinearChannel("y", 0, 255, false, true, true),
  //   LinearChannel("z", 0, 255, false, true, true)
  // };

  const LinearChannel AlphaChannel("alpha", 0, 1, false, false, false);
}

#endif
