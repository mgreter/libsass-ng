/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_COLOR_MAPS_HPP
#define SASS_COLOR_MAPS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_colors.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  namespace ColorNames
  /////////////////////////////////////////////////////////////////////////
  {

    extern const char aliceblue[];
    extern const char antiquewhite[];
    extern const char cyan[];
    extern const char aqua[];
    extern const char aquamarine[];
    extern const char azure[];
    extern const char beige[];
    extern const char bisque[];
    extern const char black[];
    extern const char blanchedalmond[];
    extern const char blue[];
    extern const char blueviolet[];
    extern const char brown[];
    extern const char burlywood[];
    extern const char cadetblue[];
    extern const char chartreuse[];
    extern const char chocolate[];
    extern const char coral[];
    extern const char cornflowerblue[];
    extern const char cornsilk[];
    extern const char crimson[];
    extern const char darkblue[];
    extern const char darkcyan[];
    extern const char darkgoldenrod[];
    extern const char darkgray[];
    extern const char darkgrey[];
    extern const char darkgreen[];
    extern const char darkkhaki[];
    extern const char darkmagenta[];
    extern const char darkolivegreen[];
    extern const char darkorange[];
    extern const char darkorchid[];
    extern const char darkred[];
    extern const char darksalmon[];
    extern const char darkseagreen[];
    extern const char darkslateblue[];
    extern const char darkslategray[];
    extern const char darkslategrey[];
    extern const char darkturquoise[];
    extern const char darkviolet[];
    extern const char deeppink[];
    extern const char deepskyblue[];
    extern const char dimgray[];
    extern const char dimgrey[];
    extern const char dodgerblue[];
    extern const char firebrick[];
    extern const char floralwhite[];
    extern const char forestgreen[];
    extern const char magenta[];
    extern const char fuchsia[];
    extern const char gainsboro[];
    extern const char ghostwhite[];
    extern const char gold[];
    extern const char goldenrod[];
    extern const char gray[];
    extern const char grey[];
    extern const char green[];
    extern const char greenyellow[];
    extern const char honeydew[];
    extern const char hotpink[];
    extern const char indianred[];
    extern const char indigo[];
    extern const char ivory[];
    extern const char khaki[];
    extern const char lavender[];
    extern const char lavenderblush[];
    extern const char lawngreen[];
    extern const char lemonchiffon[];
    extern const char lightblue[];
    extern const char lightcoral[];
    extern const char lightcyan[];
    extern const char lightgoldenrodyellow[];
    extern const char lightgray[];
    extern const char lightgrey[];
    extern const char lightgreen[];
    extern const char lightpink[];
    extern const char lightsalmon[];
    extern const char lightseagreen[];
    extern const char lightskyblue[];
    extern const char lightslategray[];
    extern const char lightslategrey[];
    extern const char lightsteelblue[];
    extern const char lightyellow[];
    extern const char lime[];
    extern const char limegreen[];
    extern const char linen[];
    extern const char maroon[];
    extern const char mediumaquamarine[];
    extern const char mediumblue[];
    extern const char mediumorchid[];
    extern const char mediumpurple[];
    extern const char mediumseagreen[];
    extern const char mediumslateblue[];
    extern const char mediumspringgreen[];
    extern const char mediumturquoise[];
    extern const char mediumvioletred[];
    extern const char midnightblue[];
    extern const char mintcream[];
    extern const char mistyrose[];
    extern const char moccasin[];
    extern const char navajowhite[];
    extern const char navy[];
    extern const char oldlace[];
    extern const char olive[];
    extern const char olivedrab[];
    extern const char orange[];
    extern const char orangered[];
    extern const char orchid[];
    extern const char palegoldenrod[];
    extern const char palegreen[];
    extern const char paleturquoise[];
    extern const char palevioletred[];
    extern const char papayawhip[];
    extern const char peachpuff[];
    extern const char peru[];
    extern const char pink[];
    extern const char plum[];
    extern const char powderblue[];
    extern const char purple[];
    extern const char red[];
    extern const char rosybrown[];
    extern const char royalblue[];
    extern const char saddlebrown[];
    extern const char salmon[];
    extern const char sandybrown[];
    extern const char seagreen[];
    extern const char seashell[];
    extern const char sienna[];
    extern const char silver[];
    extern const char skyblue[];
    extern const char slateblue[];
    extern const char slategray[];
    extern const char slategrey[];
    extern const char snow[];
    extern const char springgreen[];
    extern const char steelblue[];
    extern const char tan[];
    extern const char teal[];
    extern const char thistle[];
    extern const char tomato[];
    extern const char turquoise[];
    extern const char violet[];
    extern const char wheat[];
    extern const char white[];
    extern const char whitesmoke[];
    extern const char yellow[];
    extern const char yellowgreen[];
    extern const char rebeccapurple[];
    extern const char transparent[];

  }

  /////////////////////////////////////////////////////////////////////////
  namespace Colors
  /////////////////////////////////////////////////////////////////////////
  {

    extern const ColorSpaced aliceblue;
    extern const ColorSpaced antiquewhite;
    extern const ColorSpaced cyan;
    extern const ColorSpaced aqua;
    extern const ColorSpaced aquamarine;
    extern const ColorSpaced azure;
    extern const ColorSpaced beige;
    extern const ColorSpaced bisque;
    extern const ColorSpaced black;
    extern const ColorSpaced blanchedalmond;
    extern const ColorSpaced blue;
    extern const ColorSpaced blueviolet;
    extern const ColorSpaced brown;
    extern const ColorSpaced burlywood;
    extern const ColorSpaced cadetblue;
    extern const ColorSpaced chartreuse;
    extern const ColorSpaced chocolate;
    extern const ColorSpaced coral;
    extern const ColorSpaced cornflowerblue;
    extern const ColorSpaced cornsilk;
    extern const ColorSpaced crimson;
    extern const ColorSpaced darkblue;
    extern const ColorSpaced darkcyan;
    extern const ColorSpaced darkgoldenrod;
    extern const ColorSpaced darkgray;
    extern const ColorSpaced darkgrey;
    extern const ColorSpaced darkgreen;
    extern const ColorSpaced darkkhaki;
    extern const ColorSpaced darkmagenta;
    extern const ColorSpaced darkolivegreen;
    extern const ColorSpaced darkorange;
    extern const ColorSpaced darkorchid;
    extern const ColorSpaced darkred;
    extern const ColorSpaced darksalmon;
    extern const ColorSpaced darkseagreen;
    extern const ColorSpaced darkslateblue;
    extern const ColorSpaced darkslategray;
    extern const ColorSpaced darkslategrey;
    extern const ColorSpaced darkturquoise;
    extern const ColorSpaced darkviolet;
    extern const ColorSpaced deeppink;
    extern const ColorSpaced deepskyblue;
    extern const ColorSpaced dimgray;
    extern const ColorSpaced dimgrey;
    extern const ColorSpaced dodgerblue;
    extern const ColorSpaced firebrick;
    extern const ColorSpaced floralwhite;
    extern const ColorSpaced forestgreen;
    extern const ColorSpaced magenta;
    extern const ColorSpaced fuchsia;
    extern const ColorSpaced gainsboro;
    extern const ColorSpaced ghostwhite;
    extern const ColorSpaced gold;
    extern const ColorSpaced goldenrod;
    extern const ColorSpaced gray;
    extern const ColorSpaced grey;
    extern const ColorSpaced green;
    extern const ColorSpaced greenyellow;
    extern const ColorSpaced honeydew;
    extern const ColorSpaced hotpink;
    extern const ColorSpaced indianred;
    extern const ColorSpaced indigo;
    extern const ColorSpaced ivory;
    extern const ColorSpaced khaki;
    extern const ColorSpaced lavender;
    extern const ColorSpaced lavenderblush;
    extern const ColorSpaced lawngreen;
    extern const ColorSpaced lemonchiffon;
    extern const ColorSpaced lightblue;
    extern const ColorSpaced lightcoral;
    extern const ColorSpaced lightcyan;
    extern const ColorSpaced lightgoldenrodyellow;
    extern const ColorSpaced lightgray;
    extern const ColorSpaced lightgrey;
    extern const ColorSpaced lightgreen;
    extern const ColorSpaced lightpink;
    extern const ColorSpaced lightsalmon;
    extern const ColorSpaced lightseagreen;
    extern const ColorSpaced lightskyblue;
    extern const ColorSpaced lightslategray;
    extern const ColorSpaced lightslategrey;
    extern const ColorSpaced lightsteelblue;
    extern const ColorSpaced lightyellow;
    extern const ColorSpaced lime;
    extern const ColorSpaced limegreen;
    extern const ColorSpaced linen;
    extern const ColorSpaced maroon;
    extern const ColorSpaced mediumaquamarine;
    extern const ColorSpaced mediumblue;
    extern const ColorSpaced mediumorchid;
    extern const ColorSpaced mediumpurple;
    extern const ColorSpaced mediumseagreen;
    extern const ColorSpaced mediumslateblue;
    extern const ColorSpaced mediumspringgreen;
    extern const ColorSpaced mediumturquoise;
    extern const ColorSpaced mediumvioletred;
    extern const ColorSpaced midnightblue;
    extern const ColorSpaced mintcream;
    extern const ColorSpaced mistyrose;
    extern const ColorSpaced moccasin;
    extern const ColorSpaced navajowhite;
    extern const ColorSpaced navy;
    extern const ColorSpaced oldlace;
    extern const ColorSpaced olive;
    extern const ColorSpaced olivedrab;
    extern const ColorSpaced orange;
    extern const ColorSpaced orangered;
    extern const ColorSpaced orchid;
    extern const ColorSpaced palegoldenrod;
    extern const ColorSpaced palegreen;
    extern const ColorSpaced paleturquoise;
    extern const ColorSpaced palevioletred;
    extern const ColorSpaced papayawhip;
    extern const ColorSpaced peachpuff;
    extern const ColorSpaced peru;
    extern const ColorSpaced pink;
    extern const ColorSpaced plum;
    extern const ColorSpaced powderblue;
    extern const ColorSpaced purple;
    extern const ColorSpaced red;
    extern const ColorSpaced rosybrown;
    extern const ColorSpaced royalblue;
    extern const ColorSpaced saddlebrown;
    extern const ColorSpaced salmon;
    extern const ColorSpaced sandybrown;
    extern const ColorSpaced seagreen;
    extern const ColorSpaced seashell;
    extern const ColorSpaced sienna;
    extern const ColorSpaced silver;
    extern const ColorSpaced skyblue;
    extern const ColorSpaced slateblue;
    extern const ColorSpaced slategray;
    extern const ColorSpaced slategrey;
    extern const ColorSpaced snow;
    extern const ColorSpaced springgreen;
    extern const ColorSpaced steelblue;
    extern const ColorSpaced tan;
    extern const ColorSpaced teal;
    extern const ColorSpaced thistle;
    extern const ColorSpaced tomato;
    extern const ColorSpaced turquoise;
    extern const ColorSpaced violet;
    extern const ColorSpaced wheat;
    extern const ColorSpaced white;
    extern const ColorSpaced whitesmoke;
    extern const ColorSpaced yellow;
    extern const ColorSpaced yellowgreen;
    extern const ColorSpaced rebeccapurple;
    extern const ColorSpaced transparent;

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  const ColorSpaced* name_to_color(const char*);
  const ColorSpaced* name_to_color(const sass::string&);
  const char* color_to_name(const int);

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
