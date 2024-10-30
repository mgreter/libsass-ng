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

  ColorRgba::ColorRgba(
    const SourceSpan& pstate,
    double red,
    double green,
    double blue,
    double alpha,
    const sass::string& disp,
    bool parsed) :
    Color(pstate, alpha, disp, parsed),
    r_(red),
    g_(green),
    b_(blue)
  {}

  ColorRgba::ColorRgba(const ColorRgba* ptr)
    : Color(ptr),
    r_(ptr->r_),
    g_(ptr->g_),
    b_(ptr->b_)
  {}

  /////////////////////////////////////////////////////////////////////////

  bool ColorRgba::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      ColorRgba* rgba = color->toRGBA();
      return *this == *rgba;
    }
    return false;
  }

  bool ColorRgba::operator==(const ColorRgba& rhs) const
  {
    return r_ == rhs.r() &&
      g_ == rhs.g() &&
      b_ == rhs.b() &&
      a_ == rhs.a();
  }

  size_t ColorRgba::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(ColorRgba).hash_code());
      hash_combine(hash_, std::hash<double>{}(a_));
      hash_combine(hash_, std::hash<double>{}(r_));
      hash_combine(hash_, std::hash<double>{}(g_));
      hash_combine(hash_, std::hash<double>{}(b_));
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////

  ColorHsla* ColorRgba::copyAsHSLA() const
  {

    // Algorithm from http://en.wikipedia.org/wiki/wHSL_and_HSV#Conversion_from_RGB_to_HSL_or_HSV
    double r = r_ / 255.0;
    double g = g_ / 255.0;
    double b = b_ / 255.0;

    double max = std::max(r, std::max(g, b));
    double min = std::min(r, std::min(g, b));
    double delta = max - min;

    double h = 0;
    double s;
    double l = (max + min) / 2.0;

    if (NEAR_EQUAL(max, min)) {
      h = s = 0; // achromatic
    }
    else {
      if (l < 0.5) s = delta / (max + min);
      else         s = delta / (2.0 - max - min);

      if (r == max) h = (g - b) / delta + (g < b ? 6 : 0);
      else if (g == max) h = (b - r) / delta + 2;
      else if (b == max) h = (r - g) / delta + 4;
    }

    // HSL hsl_struct;
    h = h * 60;
    s = s * 100;
    l = l * 100;

    return SASS_MEMORY_NEW(ColorHsla,
      pstate(), h, s, l, a(), ""
    );
  }

  ColorHwba* ColorRgba::copyAsHWBA() const
  {

    // Algorithm from http://en.wikipedia.org/wiki/wHSL_and_HSV#Conversion_from_RGB_to_HSL_or_HSV
    double r = r_ / 255.0;
    double g = g_ / 255.0;
    double b = b_ / 255.0;

    double max = std::max(r, std::max(g, b));
    double min = std::min(r, std::min(g, b));
    double delta = max - min;

    double h = 0;

    if (NEAR_EQUAL(max, min)) {
      h = 0; // achromatic
    }
    else {
      if (r == max) h = (g - b) / delta + (g < b ? 6 : 0);
      else if (g == max) h = (b - r) / delta + 2;
      else if (b == max) h = (r - g) / delta + 4;
    }

    double _w = std::min(r, std::min(g, b));
    double _b = 1.0 - std::max(r, std::max(g, b));

    // HSL hsl_struct;
    h = h * 60;
    _w *= 100;
    _b *= 100;

    return SASS_MEMORY_NEW(ColorHwba, pstate_, h, _w, _b, a_);

  }

  ColorHsla* ColorRgba::toHSLA() const
  {
    return copyAsHSLA();
  }

  ColorHwba* ColorRgba::toHWBA() const
  {
    return copyAsHWBA();
  }

  ColorRgba* ColorRgba::copyAsRGBA() const
  {
    return SASS_MEMORY_COPY(this);
  }

  ColorRgba* ColorRgba::toRGBA() const
  {
    // This is safe, I know what I do!
    return const_cast<ColorRgba*>(this);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  ColorHsla::ColorHsla(
    const SourceSpan& pstate,
    double hue,
    double saturation,
    double lightness,
    double alpha,
    const sass::string& disp,
    bool parsed) :
    Color(pstate, alpha, disp, parsed),
    h_(absmod(hue, 360.0)),
    s_(clamp(saturation, 0.0, 100.0)),
    l_(clamp(lightness, 0.0, 100.0))
  {}

  ColorHsla::ColorHsla(const ColorHsla* ptr)
    : Color(ptr),
    h_(ptr->h_),
    s_(ptr->s_),
    l_(ptr->l_)
  {}

  /////////////////////////////////////////////////////////////////////////

  bool ColorHsla::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      ColorHsla* hsla = color->toHSLA();
      return *this == *hsla;
    }
    return false;
  }

  bool ColorHsla::operator==(const ColorHsla& rhs) const
  {
    return h_ == rhs.h() &&
      s_ == rhs.s() &&
      l_ == rhs.l() &&
      a_ == rhs.a();
  }

  size_t ColorHsla::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(ColorHsla).hash_code());
      hash_combine(hash_, std::hash<double>{}(a_));
      hash_combine(hash_, std::hash<double>{}(h_));
      hash_combine(hash_, std::hash<double>{}(s_));
      hash_combine(hash_, std::hash<double>{}(l_));
    }
    return hash_;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  ColorHwba::ColorHwba(
    const SourceSpan& pstate,
    double hue,
    double whiteness,
    double blackness,
    double alpha,
    const sass::string& disp,
    bool parsed) :
    Color(pstate, alpha, disp, parsed),
    h_(absmod(hue, 360.0)),
    w_(clamp(whiteness, 0.0, 100.0)),
    b_(clamp(blackness, 0.0, 100.0))
  {}

  ColorHwba::ColorHwba(const ColorHwba* ptr)
    : Color(ptr),
    h_(ptr->h_),
    w_(ptr->w_),
    b_(ptr->b_)
  {}

  /////////////////////////////////////////////////////////////////////////

  bool ColorHwba::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      ColorHwba* hwba = color->toHWBA();
      return *this == *hwba;
    }
    return false;
  }

  bool ColorHwba::operator==(const ColorHwba& rhs) const
  {
    return h_ == rhs.h() &&
      w_ == rhs.w() &&
      b_ == rhs.b() &&
      a_ == rhs.a();
  }

  size_t ColorHwba::hash() const
  {
    if (hash_ == 0) {
      hash_start(hash_, typeid(ColorHsla).hash_code());
      hash_combine(hash_, std::hash<double>{}(a_));
      hash_combine(hash_, std::hash<double>{}(h_));
      hash_combine(hash_, std::hash<double>{}(w_));
      hash_combine(hash_, std::hash<double>{}(b_));
    }
    return hash_;
  }


  ColorHwba* ColorHwba::copyAsHWBA() const
  {
    return SASS_MEMORY_COPY(this);
  }

  ColorRgba* ColorHwba::copyAsRGBA() const
  {
    double h = h_ / 360.0;
    double wh = w_ / 100.0;
    double bl = b_ / 100.0;
    double ratio = wh + bl;
    double v, f, n;
    if (ratio > 1) {
      wh /= ratio;
      bl /= ratio;
    }
    int i = (int)floor(6.0 * h);
    v = 1.0 - bl;
    f = 6.0 * h - i;
    if ((i & 1) != 0) {
       f = 1 - f;
     }
    n = wh + f * (v - wh);
    double r, g, b;
    switch (i) {
    default:
    case 6:
    case 0: r = v; g = n; b = wh; break;
    case 1: r = n; g = v; b = wh; break;
    case 2: r = wh; g = v; b = n; break;
    case 3: r = wh; g = n; b = v; break;
    case 4: r = n; g = wh; b = v; break;
    case 5: r = v; g = wh; b = n; break;
    }
    return SASS_MEMORY_NEW(ColorRgba,
      pstate_, r * 255.0, g * 255.0, b * 255.0, a_);
  }

  ColorHsla* ColorHwba::copyAsHSLA() const
  {
    ColorRgbaObj rgba(copyAsRGBA());
    return rgba->copyAsHSLA();
  }

  ColorHsla* ColorHwba::toHSLA() const
  {
    return copyAsHSLA();
  }

  ColorHwba* ColorHwba::toHWBA() const
  {
    return const_cast<ColorHwba*>(this);;
  }

  ColorRgba* ColorHwba::toRGBA() const
  {
    return copyAsRGBA();
  }



  /////////////////////////////////////////////////////////////////////////

  // hue to RGB helper function
  static double h_to_rgb(double m1, double m2, double h)
  {
    h = absmod(h, 1.0);
    if (h * 6.0 < 1) return m1 + (m2 - m1) * h * 6;
    if (h * 2.0 < 1) return m2;
    if (h * 3.0 < 2) return m1 + (m2 - m1) * (2.0 / 3.0 - h) * 6;
    return m1;
  }

  ColorRgba* ColorHsla::copyAsRGBA() const
  {
    double h = absmod(h_ / 360.0, 1.0);
    double s = clamp(s_ / 100.0, 0.0, 1.0);
    double l = clamp(l_ / 100.0, 0.0, 1.0);

    // Algorithm from the CSS3 spec: http://www.w3.org/TR/css3-color/#hsl-color.
    double m2;
    if (l <= 0.5) m2 = l * (s + 1.0);
    else m2 = (l + s) - (l * s);
    double m1 = (l * 2.0) - m2;
    // round the results -- consider moving this into the Color constructor
    double r = (h_to_rgb(m1, m2, h + 1.0 / 3.0) * 255.0);
    double g = (h_to_rgb(m1, m2, h) * 255.0);
    double b = (h_to_rgb(m1, m2, h - 1.0 / 3.0) * 255.0);

    return SASS_MEMORY_NEW(ColorRgba,
      pstate(), r, g, b, a(), ""
    );
  }

  ColorHwba* ColorHsla::copyAsHWBA() const
  {
    ColorRgbaObj rgba(copyAsRGBA());
    return rgba->copyAsHWBA();

    throw std::runtime_error("invalid");
    return nullptr;
  }

  ColorHsla* ColorHsla::copyAsHSLA() const
  {
    auto col = SASS_MEMORY_COPY(this);
    col->parsed(false); // Do better
    return col;
  }

  ColorRgba* ColorHsla::toRGBA() const
  {
    return copyAsRGBA();
  }

  ColorHwba* ColorHsla::toHWBA() const
  {
    return copyAsHWBA();
  }

  ColorHsla* ColorHsla::toHSLA() const
  {
    // This is safe, I know what I do!
    return const_cast<ColorHsla*>(this);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  double ColorSpaced::getChannel0() const { return c0_.value_or(0); }
  double ColorSpaced::getChannel1() const { return c1_.value_or(0); }
  double ColorSpaced::getChannel2() const { return c2_.value_or(0); }

  bool ColorSpaced::isChannel0Missing() const { return !c0_.has_value(); }
  bool ColorSpaced::isChannel1Missing() const { return !c1_.has_value(); }
  bool ColorSpaced::isChannel2Missing() const { return !c2_.has_value(); }

  ColorSpaced::ColorSpaced(const SourceSpan& pstate, const SassColorSpace space, double c0, double c1, double c2, double alpha, const sass::string& disp, bool parsed)
    : Color(pstate), space_(space), c0_(c0), c1_(c1), c2_(c2), alpha_(alpha)
  {



  }

  ColorSpaced::ColorSpaced(const ColorSpaced* ptr)
    : Color(ptr), space_(ptr->space_), c0_(ptr->c0_), c1_(ptr->c1_), c2_(ptr->c2_), alpha_(ptr->alpha_)
  {
  }


  bool ColorSpaced::operator==(const Value& rhs) const
  {
    if (const Color* color = rhs.isaColor()) {
      ColorHwba* hwba = color->toHWBA();
      return *this == *hwba;
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
      hash_start(hash_, typeid(ColorHsla).hash_code());
      //hash_combine(hash_, std::hash<double>{}(c1_));
      //hash_combine(hash_, std::hash<double>{}(c2_));
      //hash_combine(hash_, std::hash<double>{}(c3_));
      //hash_combine(hash_, std::hash<double>{}(c4_));
    }
    return hash_;
  }


  ColorRgba* ColorSpaced::copyAsRGBA() const
  {
    return nullptr;
  }

  ColorHwba* ColorSpaced::copyAsHWBA() const
  {
    return nullptr;
  }

  ColorHsla* ColorSpaced::copyAsHSLA() const
  {
    return nullptr;
  }

  ColorRgba* ColorSpaced::toRGBA() const
  {
    return nullptr;
  }

  ColorHwba* ColorSpaced::toHWBA() const
  {
    return nullptr;
  }

  ColorHsla* ColorSpaced::toHSLA() const
  {
    return nullptr;
  }

  double ColorSpace::toLinear(double channel)
  {
    throw std::runtime_error("toLinear not implemented");
  }

  double ColorSpace::fromLinear(double channel)
  {
    throw std::runtime_error("toLinear not implemented");
  }

  double* ColorSpace::transformationMatrix(ColorSpace dest)
  {
    throw std::runtime_error("matrix not implemented");
  }

  Color* ColorSpace::convertLinear(
    const ColorSpace& dest,
    tl::optional<double> red,
    tl::optional<double> green,
    tl::optional<double> blue,
    tl::optional<double> alpha,
    bool missingLightness,
    bool missingChroma,
    bool missingHue,
    bool missingA,
    bool missingB)
  {

    ColorSpace linearDest = dest;

    if (dest == ColorSpace::hsl || dest == ColorSpace::hwb) { linearDest == ColorSpace::srgb; }
    else if (dest == ColorSpace::lab || dest == ColorSpace::lch) { linearDest == ColorSpace::xyzd50; }
    else if (dest == ColorSpace::oklab || dest == ColorSpace::oklch) { linearDest == ColorSpace::lms; }

    tl::optional<double> transformedRed;
    tl::optional<double> transformedGreen;
    tl::optional<double> transformedBlue;

    if (linearDest == *this) {
      transformedRed = red;
      transformedGreen = green;
      transformedBlue = blue;
    }
    else {
      double linearRed = toLinear(red.value_or(0));
      double linearGreen = toLinear(green.value_or(0));
      double linearBlue = toLinear(blue.value_or(0));
    }

      return nullptr;
  }


  const ColorSpace ColorSpace::hwb = HwbColorSpace();
  const ColorSpace ColorSpace::hsl = HslColorSpace();
  const ColorSpace ColorSpace::lab = LabColorSpace();
  const ColorSpace ColorSpace::lch = LchColorSpace();
  const ColorSpace ColorSpace::oklab = OkLabColorSpace();
  const ColorSpace ColorSpace::oklch = OkLchColorSpace();

  const ColorSpace ColorSpace::srgb = SrgbColorSpace();
  const ColorSpace ColorSpace::xyzd50 = XyzD50ColorSpace();
  const ColorSpace ColorSpace::lms = LmsColorSpace();

  //const ColorSpace ColorSpacings::SRGB(str_srgb, SassColorSpace::SRGB, srgb_channels);

}
