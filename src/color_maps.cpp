/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "color_maps.hpp"

#include "string_utils.hpp"
#include "ast_colors.hpp"

namespace Sass
{

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  namespace ColorNames
  {
    const char aliceblue[] = "aliceblue";
    const char antiquewhite[] = "antiquewhite";
    const char cyan[] = "cyan";
    const char aqua[] = "aqua";
    const char aquamarine[] = "aquamarine";
    const char azure[] = "azure";
    const char beige[] = "beige";
    const char bisque[] = "bisque";
    const char black[] = "black";
    const char blanchedalmond[] = "blanchedalmond";
    const char blue[] = "blue";
    const char blueviolet[] = "blueviolet";
    const char brown[] = "brown";
    const char burlywood[] = "burlywood";
    const char cadetblue[] = "cadetblue";
    const char chartreuse[] = "chartreuse";
    const char chocolate[] = "chocolate";
    const char coral[] = "coral";
    const char cornflowerblue[] = "cornflowerblue";
    const char cornsilk[] = "cornsilk";
    const char crimson[] = "crimson";
    const char darkblue[] = "darkblue";
    const char darkcyan[] = "darkcyan";
    const char darkgoldenrod[] = "darkgoldenrod";
    const char darkgray[] = "darkgray";
    const char darkgrey[] = "darkgrey";
    const char darkgreen[] = "darkgreen";
    const char darkkhaki[] = "darkkhaki";
    const char darkmagenta[] = "darkmagenta";
    const char darkolivegreen[] = "darkolivegreen";
    const char darkorange[] = "darkorange";
    const char darkorchid[] = "darkorchid";
    const char darkred[] = "darkred";
    const char darksalmon[] = "darksalmon";
    const char darkseagreen[] = "darkseagreen";
    const char darkslateblue[] = "darkslateblue";
    const char darkslategray[] = "darkslategray";
    const char darkslategrey[] = "darkslategrey";
    const char darkturquoise[] = "darkturquoise";
    const char darkviolet[] = "darkviolet";
    const char deeppink[] = "deeppink";
    const char deepskyblue[] = "deepskyblue";
    const char dimgray[] = "dimgray";
    const char dimgrey[] = "dimgrey";
    const char dodgerblue[] = "dodgerblue";
    const char firebrick[] = "firebrick";
    const char floralwhite[] = "floralwhite";
    const char forestgreen[] = "forestgreen";
    const char magenta[] = "magenta";
    const char fuchsia[] = "fuchsia";
    const char gainsboro[] = "gainsboro";
    const char ghostwhite[] = "ghostwhite";
    const char gold[] = "gold";
    const char goldenrod[] = "goldenrod";
    const char gray[] = "gray";
    const char grey[] = "grey";
    const char green[] = "green";
    const char greenyellow[] = "greenyellow";
    const char honeydew[] = "honeydew";
    const char hotpink[] = "hotpink";
    const char indianred[] = "indianred";
    const char indigo[] = "indigo";
    const char ivory[] = "ivory";
    const char khaki[] = "khaki";
    const char lavender[] = "lavender";
    const char lavenderblush[] = "lavenderblush";
    const char lawngreen[] = "lawngreen";
    const char lemonchiffon[] = "lemonchiffon";
    const char lightblue[] = "lightblue";
    const char lightcoral[] = "lightcoral";
    const char lightcyan[] = "lightcyan";
    const char lightgoldenrodyellow[] = "lightgoldenrodyellow";
    const char lightgray[] = "lightgray";
    const char lightgrey[] = "lightgrey";
    const char lightgreen[] = "lightgreen";
    const char lightpink[] = "lightpink";
    const char lightsalmon[] = "lightsalmon";
    const char lightseagreen[] = "lightseagreen";
    const char lightskyblue[] = "lightskyblue";
    const char lightslategray[] = "lightslategray";
    const char lightslategrey[] = "lightslategrey";
    const char lightsteelblue[] = "lightsteelblue";
    const char lightyellow[] = "lightyellow";
    const char lime[] = "lime";
    const char limegreen[] = "limegreen";
    const char linen[] = "linen";
    const char maroon[] = "maroon";
    const char mediumaquamarine[] = "mediumaquamarine";
    const char mediumblue[] = "mediumblue";
    const char mediumorchid[] = "mediumorchid";
    const char mediumpurple[] = "mediumpurple";
    const char mediumseagreen[] = "mediumseagreen";
    const char mediumslateblue[] = "mediumslateblue";
    const char mediumspringgreen[] = "mediumspringgreen";
    const char mediumturquoise[] = "mediumturquoise";
    const char mediumvioletred[] = "mediumvioletred";
    const char midnightblue[] = "midnightblue";
    const char mintcream[] = "mintcream";
    const char mistyrose[] = "mistyrose";
    const char moccasin[] = "moccasin";
    const char navajowhite[] = "navajowhite";
    const char navy[] = "navy";
    const char oldlace[] = "oldlace";
    const char olive[] = "olive";
    const char olivedrab[] = "olivedrab";
    const char orange[] = "orange";
    const char orangered[] = "orangered";
    const char orchid[] = "orchid";
    const char palegoldenrod[] = "palegoldenrod";
    const char palegreen[] = "palegreen";
    const char paleturquoise[] = "paleturquoise";
    const char palevioletred[] = "palevioletred";
    const char papayawhip[] = "papayawhip";
    const char peachpuff[] = "peachpuff";
    const char peru[] = "peru";
    const char pink[] = "pink";
    const char plum[] = "plum";
    const char powderblue[] = "powderblue";
    const char purple[] = "purple";
    const char red[] = "red";
    const char rosybrown[] = "rosybrown";
    const char royalblue[] = "royalblue";
    const char saddlebrown[] = "saddlebrown";
    const char salmon[] = "salmon";
    const char sandybrown[] = "sandybrown";
    const char seagreen[] = "seagreen";
    const char seashell[] = "seashell";
    const char sienna[] = "sienna";
    const char silver[] = "silver";
    const char skyblue[] = "skyblue";
    const char slateblue[] = "slateblue";
    const char slategray[] = "slategray";
    const char slategrey[] = "slategrey";
    const char snow[] = "snow";
    const char springgreen[] = "springgreen";
    const char steelblue[] = "steelblue";
    const char tan[] = "tan";
    const char teal[] = "teal";
    const char thistle[] = "thistle";
    const char tomato[] = "tomato";
    const char turquoise[] = "turquoise";
    const char violet[] = "violet";
    const char wheat[] = "wheat";
    const char white[] = "white";
    const char whitesmoke[] = "whitesmoke";
    const char yellow[] = "yellow";
    const char yellowgreen[] = "yellowgreen";
    const char rebeccapurple[] = "rebeccapurple";
    const char transparent[] = "transparent";
  } // namespace ColorNames

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  namespace Colors
  {

    const SourceSpan color_table(SourceSpan::internal32("[COLOR TABLE]"));
    const Color aliceblue(color_table, ColorSpaces::rgb, 240, 248, 255, 1);
    const Color antiquewhite(color_table, ColorSpaces::rgb, 250, 235, 215, 1);
    const Color cyan(color_table, ColorSpaces::rgb, 0, 255, 255, 1);
    const Color aqua(color_table, ColorSpaces::rgb, 0, 255, 255, 1);
    const Color aquamarine(color_table, ColorSpaces::rgb, 127, 255, 212, 1);
    const Color azure(color_table, ColorSpaces::rgb, 240, 255, 255, 1);
    const Color beige(color_table, ColorSpaces::rgb, 245, 245, 220, 1);
    const Color bisque(color_table, ColorSpaces::rgb, 255, 228, 196, 1);
    const Color black(color_table, ColorSpaces::rgb, 0, 0, 0, 1);
    const Color blanchedalmond(color_table, ColorSpaces::rgb, 255, 235, 205, 1);
    const Color blue(color_table, ColorSpaces::rgb, 0, 0, 255, 1);
    const Color blueviolet(color_table, ColorSpaces::rgb, 138, 43, 226, 1);
    const Color brown(color_table, ColorSpaces::rgb, 165, 42, 42, 1);
    const Color burlywood(color_table, ColorSpaces::rgb, 222, 184, 135, 1);
    const Color cadetblue(color_table, ColorSpaces::rgb, 95, 158, 160, 1);
    const Color chartreuse(color_table, ColorSpaces::rgb, 127, 255, 0, 1);
    const Color chocolate(color_table, ColorSpaces::rgb, 210, 105, 30, 1);
    const Color coral(color_table, ColorSpaces::rgb, 255, 127, 80, 1);
    const Color cornflowerblue(color_table, ColorSpaces::rgb, 100, 149, 237, 1);
    const Color cornsilk(color_table, ColorSpaces::rgb, 255, 248, 220, 1);
    const Color crimson(color_table, ColorSpaces::rgb, 220, 20, 60, 1);
    const Color darkblue(color_table, ColorSpaces::rgb, 0, 0, 139, 1);
    const Color darkcyan(color_table, ColorSpaces::rgb, 0, 139, 139, 1);
    const Color darkgoldenrod(color_table, ColorSpaces::rgb, 184, 134, 11, 1);
    const Color darkgray(color_table, ColorSpaces::rgb, 169, 169, 169, 1);
    const Color darkgrey(color_table, ColorSpaces::rgb, 169, 169, 169, 1);
    const Color darkgreen(color_table, ColorSpaces::rgb, 0, 100, 0, 1);
    const Color darkkhaki(color_table, ColorSpaces::rgb, 189, 183, 107, 1);
    const Color darkmagenta(color_table, ColorSpaces::rgb, 139, 0, 139, 1);
    const Color darkolivegreen(color_table, ColorSpaces::rgb, 85, 107, 47, 1);
    const Color darkorange(color_table, ColorSpaces::rgb, 255, 140, 0, 1);
    const Color darkorchid(color_table, ColorSpaces::rgb, 153, 50, 204, 1);
    const Color darkred(color_table, ColorSpaces::rgb, 139, 0, 0, 1);
    const Color darksalmon(color_table, ColorSpaces::rgb, 233, 150, 122, 1);
    const Color darkseagreen(color_table, ColorSpaces::rgb, 143, 188, 143, 1);
    const Color darkslateblue(color_table, ColorSpaces::rgb, 72, 61, 139, 1);
    const Color darkslategray(color_table, ColorSpaces::rgb, 47, 79, 79, 1);
    const Color darkslategrey(color_table, ColorSpaces::rgb, 47, 79, 79, 1);
    const Color darkturquoise(color_table, ColorSpaces::rgb, 0, 206, 209, 1);
    const Color darkviolet(color_table, ColorSpaces::rgb, 148, 0, 211, 1);
    const Color deeppink(color_table, ColorSpaces::rgb, 255, 20, 147, 1);
    const Color deepskyblue(color_table, ColorSpaces::rgb, 0, 191, 255, 1);
    const Color dimgray(color_table, ColorSpaces::rgb, 105, 105, 105, 1);
    const Color dimgrey(color_table, ColorSpaces::rgb, 105, 105, 105, 1);
    const Color dodgerblue(color_table, ColorSpaces::rgb, 30, 144, 255, 1);
    const Color firebrick(color_table, ColorSpaces::rgb, 178, 34, 34, 1);
    const Color floralwhite(color_table, ColorSpaces::rgb, 255, 250, 240, 1);
    const Color forestgreen(color_table, ColorSpaces::rgb, 34, 139, 34, 1);
    const Color magenta(color_table, ColorSpaces::rgb, 255, 0, 255, 1);
    const Color fuchsia(color_table, ColorSpaces::rgb, 255, 0, 255, 1);
    const Color gainsboro(color_table, ColorSpaces::rgb, 220, 220, 220, 1);
    const Color ghostwhite(color_table, ColorSpaces::rgb, 248, 248, 255, 1);
    const Color gold(color_table, ColorSpaces::rgb, 255, 215, 0, 1);
    const Color goldenrod(color_table, ColorSpaces::rgb, 218, 165, 32, 1);
    const Color gray(color_table, ColorSpaces::rgb, 128, 128, 128, 1);
    const Color grey(color_table, ColorSpaces::rgb, 128, 128, 128, 1);
    const Color green(color_table, ColorSpaces::rgb, 0, 128, 0, 1);
    const Color greenyellow(color_table, ColorSpaces::rgb, 173, 255, 47, 1);
    const Color honeydew(color_table, ColorSpaces::rgb, 240, 255, 240, 1);
    const Color hotpink(color_table, ColorSpaces::rgb, 255, 105, 180, 1);
    const Color indianred(color_table, ColorSpaces::rgb, 205, 92, 92, 1);
    const Color indigo(color_table, ColorSpaces::rgb, 75, 0, 130, 1);
    const Color ivory(color_table, ColorSpaces::rgb, 255, 255, 240, 1);
    const Color khaki(color_table, ColorSpaces::rgb, 240, 230, 140, 1);
    const Color lavender(color_table, ColorSpaces::rgb, 230, 230, 250, 1);
    const Color lavenderblush(color_table, ColorSpaces::rgb, 255, 240, 245, 1);
    const Color lawngreen(color_table, ColorSpaces::rgb, 124, 252, 0, 1);
    const Color lemonchiffon(color_table, ColorSpaces::rgb, 255, 250, 205, 1);
    const Color lightblue(color_table, ColorSpaces::rgb, 173, 216, 230, 1);
    const Color lightcoral(color_table, ColorSpaces::rgb, 240, 128, 128, 1);
    const Color lightcyan(color_table, ColorSpaces::rgb, 224, 255, 255, 1);
    const Color lightgoldenrodyellow(color_table, ColorSpaces::rgb, 250, 250, 210, 1);
    const Color lightgray(color_table, ColorSpaces::rgb, 211, 211, 211, 1);
    const Color lightgrey(color_table, ColorSpaces::rgb, 211, 211, 211, 1);
    const Color lightgreen(color_table, ColorSpaces::rgb, 144, 238, 144, 1);
    const Color lightpink(color_table, ColorSpaces::rgb, 255, 182, 193, 1);
    const Color lightsalmon(color_table, ColorSpaces::rgb, 255, 160, 122, 1);
    const Color lightseagreen(color_table, ColorSpaces::rgb, 32, 178, 170, 1);
    const Color lightskyblue(color_table, ColorSpaces::rgb, 135, 206, 250, 1);
    const Color lightslategray(color_table, ColorSpaces::rgb, 119, 136, 153, 1);
    const Color lightslategrey(color_table, ColorSpaces::rgb, 119, 136, 153, 1);
    const Color lightsteelblue(color_table, ColorSpaces::rgb, 176, 196, 222, 1);
    const Color lightyellow(color_table, ColorSpaces::rgb, 255, 255, 224, 1);
    const Color lime(color_table, ColorSpaces::rgb, 0, 255, 0, 1);
    const Color limegreen(color_table, ColorSpaces::rgb, 50, 205, 50, 1);
    const Color linen(color_table, ColorSpaces::rgb, 250, 240, 230, 1);
    const Color maroon(color_table, ColorSpaces::rgb, 128, 0, 0, 1);
    const Color mediumaquamarine(color_table, ColorSpaces::rgb, 102, 205, 170, 1);
    const Color mediumblue(color_table, ColorSpaces::rgb, 0, 0, 205, 1);
    const Color mediumorchid(color_table, ColorSpaces::rgb, 186, 85, 211, 1);
    const Color mediumpurple(color_table, ColorSpaces::rgb, 147, 112, 219, 1);
    const Color mediumseagreen(color_table, ColorSpaces::rgb, 60, 179, 113, 1);
    const Color mediumslateblue(color_table, ColorSpaces::rgb, 123, 104, 238, 1);
    const Color mediumspringgreen(color_table, ColorSpaces::rgb, 0, 250, 154, 1);
    const Color mediumturquoise(color_table, ColorSpaces::rgb, 72, 209, 204, 1);
    const Color mediumvioletred(color_table, ColorSpaces::rgb, 199, 21, 133, 1);
    const Color midnightblue(color_table, ColorSpaces::rgb, 25, 25, 112, 1);
    const Color mintcream(color_table, ColorSpaces::rgb, 245, 255, 250, 1);
    const Color mistyrose(color_table, ColorSpaces::rgb, 255, 228, 225, 1);
    const Color moccasin(color_table, ColorSpaces::rgb, 255, 228, 181, 1);
    const Color navajowhite(color_table, ColorSpaces::rgb, 255, 222, 173, 1);
    const Color navy(color_table, ColorSpaces::rgb, 0, 0, 128, 1);
    const Color oldlace(color_table, ColorSpaces::rgb, 253, 245, 230, 1);
    const Color olive(color_table, ColorSpaces::rgb, 128, 128, 0, 1);
    const Color olivedrab(color_table, ColorSpaces::rgb, 107, 142, 35, 1);
    const Color orange(color_table, ColorSpaces::rgb, 255, 165, 0, 1);
    const Color orangered(color_table, ColorSpaces::rgb, 255, 69, 0, 1);
    const Color orchid(color_table, ColorSpaces::rgb, 218, 112, 214, 1);
    const Color palegoldenrod(color_table, ColorSpaces::rgb, 238, 232, 170, 1);
    const Color palegreen(color_table, ColorSpaces::rgb, 152, 251, 152, 1);
    const Color paleturquoise(color_table, ColorSpaces::rgb, 175, 238, 238, 1);
    const Color palevioletred(color_table, ColorSpaces::rgb, 219, 112, 147, 1);
    const Color papayawhip(color_table, ColorSpaces::rgb, 255, 239, 213, 1);
    const Color peachpuff(color_table, ColorSpaces::rgb, 255, 218, 185, 1);
    const Color peru(color_table, ColorSpaces::rgb, 205, 133, 63, 1);
    const Color pink(color_table, ColorSpaces::rgb, 255, 192, 203, 1);
    const Color plum(color_table, ColorSpaces::rgb, 221, 160, 221, 1);
    const Color powderblue(color_table, ColorSpaces::rgb, 176, 224, 230, 1);
    const Color purple(color_table, ColorSpaces::rgb, 128, 0, 128, 1);
    const Color red(color_table, ColorSpaces::rgb, 255, 0, 0, 1, "red", true);
    const Color rosybrown(color_table, ColorSpaces::rgb, 188, 143, 143, 1);
    const Color royalblue(color_table, ColorSpaces::rgb, 65, 105, 225, 1);
    const Color saddlebrown(color_table, ColorSpaces::rgb, 139, 69, 19, 1);
    const Color salmon(color_table, ColorSpaces::rgb, 250, 128, 114, 1);
    const Color sandybrown(color_table, ColorSpaces::rgb, 244, 164, 96, 1);
    const Color seagreen(color_table, ColorSpaces::rgb, 46, 139, 87, 1);
    const Color seashell(color_table, ColorSpaces::rgb, 255, 245, 238, 1);
    const Color sienna(color_table, ColorSpaces::rgb, 160, 82, 45, 1);
    const Color silver(color_table, ColorSpaces::rgb, 192, 192, 192, 1);
    const Color skyblue(color_table, ColorSpaces::rgb, 135, 206, 235, 1);
    const Color slateblue(color_table, ColorSpaces::rgb, 106, 90, 205, 1);
    const Color slategray(color_table, ColorSpaces::rgb, 112, 128, 144, 1);
    const Color slategrey(color_table, ColorSpaces::rgb, 112, 128, 144, 1);
    const Color snow(color_table, ColorSpaces::rgb, 255, 250, 250, 1);
    const Color springgreen(color_table, ColorSpaces::rgb, 0, 255, 127, 1);
    const Color steelblue(color_table, ColorSpaces::rgb, 70, 130, 180, 1);
    const Color tan(color_table, ColorSpaces::rgb, 210, 180, 140, 1);
    const Color teal(color_table, ColorSpaces::rgb, 0, 128, 128, 1);
    const Color thistle(color_table, ColorSpaces::rgb, 216, 191, 216, 1);
    const Color tomato(color_table, ColorSpaces::rgb, 255, 99, 71, 1);
    const Color turquoise(color_table, ColorSpaces::rgb, 64, 224, 208, 1);
    const Color violet(color_table, ColorSpaces::rgb, 238, 130, 238, 1);
    const Color wheat(color_table, ColorSpaces::rgb, 245, 222, 179, 1);
    const Color white(color_table, ColorSpaces::rgb, 255, 255, 255, 1);
    const Color whitesmoke(color_table, ColorSpaces::rgb, 245, 245, 245, 1);
    const Color yellow(color_table, ColorSpaces::rgb, 255, 255, 0, 1);
    const Color yellowgreen(color_table, ColorSpaces::rgb, 154, 205, 50, 1);
    const Color rebeccapurple(color_table, ColorSpaces::rgb, 102, 51, 153, 1);
    const Color transparent(color_table, ColorSpaces::rgb, 0, 0, 0, 0);
  } // namespace Colors


  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static const auto colors_to_names = std::unordered_map<int, const char*> {
    {240 * 0x10000 + 248 * 0x100 + 255, ColorNames::aliceblue},
    {250 * 0x10000 + 235 * 0x100 + 215, ColorNames::antiquewhite},
    {0 * 0x10000 + 255 * 0x100 + 255, ColorNames::cyan},
    {127 * 0x10000 + 255 * 0x100 + 212, ColorNames::aquamarine},
    {240 * 0x10000 + 255 * 0x100 + 255, ColorNames::azure},
    {245 * 0x10000 + 245 * 0x100 + 220, ColorNames::beige},
    {255 * 0x10000 + 228 * 0x100 + 196, ColorNames::bisque},
    {0 * 0x10000 + 0 * 0x100 + 0, ColorNames::black},
    {255 * 0x10000 + 235 * 0x100 + 205, ColorNames::blanchedalmond},
    {0 * 0x10000 + 0 * 0x100 + 255, ColorNames::blue},
    {138 * 0x10000 + 43 * 0x100 + 226, ColorNames::blueviolet},
    {165 * 0x10000 + 42 * 0x100 + 42, ColorNames::brown},
    {222 * 0x10000 + 184 * 0x100 + 135, ColorNames::burlywood},
    {95 * 0x10000 + 158 * 0x100 + 160, ColorNames::cadetblue},
    {127 * 0x10000 + 255 * 0x100 + 0, ColorNames::chartreuse},
    {210 * 0x10000 + 105 * 0x100 + 30, ColorNames::chocolate},
    {255 * 0x10000 + 127 * 0x100 + 80, ColorNames::coral},
    {100 * 0x10000 + 149 * 0x100 + 237, ColorNames::cornflowerblue},
    {255 * 0x10000 + 248 * 0x100 + 220, ColorNames::cornsilk},
    {220 * 0x10000 + 20 * 0x100 + 60, ColorNames::crimson},
    {0 * 0x10000 + 0 * 0x100 + 139, ColorNames::darkblue},
    {0 * 0x10000 + 139 * 0x100 + 139, ColorNames::darkcyan},
    {184 * 0x10000 + 134 * 0x100 + 11, ColorNames::darkgoldenrod},
    {169 * 0x10000 + 169 * 0x100 + 169, ColorNames::darkgray},
    {0 * 0x10000 + 100 * 0x100 + 0, ColorNames::darkgreen},
    {189 * 0x10000 + 183 * 0x100 + 107, ColorNames::darkkhaki},
    {139 * 0x10000 + 0 * 0x100 + 139, ColorNames::darkmagenta},
    {85 * 0x10000 + 107 * 0x100 + 47, ColorNames::darkolivegreen},
    {255 * 0x10000 + 140 * 0x100 + 0, ColorNames::darkorange},
    {153 * 0x10000 + 50 * 0x100 + 204, ColorNames::darkorchid},
    {139 * 0x10000 + 0 * 0x100 + 0, ColorNames::darkred},
    {233 * 0x10000 + 150 * 0x100 + 122, ColorNames::darksalmon},
    {143 * 0x10000 + 188 * 0x100 + 143, ColorNames::darkseagreen},
    {72 * 0x10000 + 61 * 0x100 + 139, ColorNames::darkslateblue},
    {47 * 0x10000 + 79 * 0x100 + 79, ColorNames::darkslategray},
    {0 * 0x10000 + 206 * 0x100 + 209, ColorNames::darkturquoise},
    {148 * 0x10000 + 0 * 0x100 + 211, ColorNames::darkviolet},
    {255 * 0x10000 + 20 * 0x100 + 147, ColorNames::deeppink},
    {0 * 0x10000 + 191 * 0x100 + 255, ColorNames::deepskyblue},
    {105 * 0x10000 + 105 * 0x100 + 105, ColorNames::dimgray},
    {30 * 0x10000 + 144 * 0x100 + 255, ColorNames::dodgerblue},
    {178 * 0x10000 + 34 * 0x100 + 34, ColorNames::firebrick},
    {255 * 0x10000 + 250 * 0x100 + 240, ColorNames::floralwhite},
    {34 * 0x10000 + 139 * 0x100 + 34, ColorNames::forestgreen},
    {255 * 0x10000 + 0 * 0x100 + 255, ColorNames::magenta},
    {220 * 0x10000 + 220 * 0x100 + 220, ColorNames::gainsboro},
    {248 * 0x10000 + 248 * 0x100 + 255, ColorNames::ghostwhite},
    {255 * 0x10000 + 215 * 0x100 + 0, ColorNames::gold},
    {218 * 0x10000 + 165 * 0x100 + 32, ColorNames::goldenrod},
    {128 * 0x10000 + 128 * 0x100 + 128, ColorNames::gray},
    {0 * 0x10000 + 128 * 0x100 + 0, ColorNames::green},
    {173 * 0x10000 + 255 * 0x100 + 47, ColorNames::greenyellow},
    {240 * 0x10000 + 255 * 0x100 + 240, ColorNames::honeydew},
    {255 * 0x10000 + 105 * 0x100 + 180, ColorNames::hotpink},
    {205 * 0x10000 + 92 * 0x100 + 92, ColorNames::indianred},
    {75 * 0x10000 + 0 * 0x100 + 130, ColorNames::indigo},
    {255 * 0x10000 + 255 * 0x100 + 240, ColorNames::ivory},
    {240 * 0x10000 + 230 * 0x100 + 140, ColorNames::khaki},
    {230 * 0x10000 + 230 * 0x100 + 250, ColorNames::lavender},
    {255 * 0x10000 + 240 * 0x100 + 245, ColorNames::lavenderblush},
    {124 * 0x10000 + 252 * 0x100 + 0, ColorNames::lawngreen},
    {255 * 0x10000 + 250 * 0x100 + 205, ColorNames::lemonchiffon},
    {173 * 0x10000 + 216 * 0x100 + 230, ColorNames::lightblue},
    {240 * 0x10000 + 128 * 0x100 + 128, ColorNames::lightcoral},
    {224 * 0x10000 + 255 * 0x100 + 255, ColorNames::lightcyan},
    {250 * 0x10000 + 250 * 0x100 + 210, ColorNames::lightgoldenrodyellow},
    {211 * 0x10000 + 211 * 0x100 + 211, ColorNames::lightgray},
    {144 * 0x10000 + 238 * 0x100 + 144, ColorNames::lightgreen},
    {255 * 0x10000 + 182 * 0x100 + 193, ColorNames::lightpink},
    {255 * 0x10000 + 160 * 0x100 + 122, ColorNames::lightsalmon},
    {32 * 0x10000 + 178 * 0x100 + 170, ColorNames::lightseagreen},
    {135 * 0x10000 + 206 * 0x100 + 250, ColorNames::lightskyblue},
    {119 * 0x10000 + 136 * 0x100 + 153, ColorNames::lightslategray},
    {176 * 0x10000 + 196 * 0x100 + 222, ColorNames::lightsteelblue},
    {255 * 0x10000 + 255 * 0x100 + 224, ColorNames::lightyellow},
    {0 * 0x10000 + 255 * 0x100 + 0, ColorNames::lime},
    {50 * 0x10000 + 205 * 0x100 + 50, ColorNames::limegreen},
    {250 * 0x10000 + 240 * 0x100 + 230, ColorNames::linen},
    {128 * 0x10000 + 0 * 0x100 + 0, ColorNames::maroon},
    {102 * 0x10000 + 205 * 0x100 + 170, ColorNames::mediumaquamarine},
    {0 * 0x10000 + 0 * 0x100 + 205, ColorNames::mediumblue},
    {186 * 0x10000 + 85 * 0x100 + 211, ColorNames::mediumorchid},
    {147 * 0x10000 + 112 * 0x100 + 219, ColorNames::mediumpurple},
    {60 * 0x10000 + 179 * 0x100 + 113, ColorNames::mediumseagreen},
    {123 * 0x10000 + 104 * 0x100 + 238, ColorNames::mediumslateblue},
    {0 * 0x10000 + 250 * 0x100 + 154, ColorNames::mediumspringgreen},
    {72 * 0x10000 + 209 * 0x100 + 204, ColorNames::mediumturquoise},
    {199 * 0x10000 + 21 * 0x100 + 133, ColorNames::mediumvioletred},
    {25 * 0x10000 + 25 * 0x100 + 112, ColorNames::midnightblue},
    {245 * 0x10000 + 255 * 0x100 + 250, ColorNames::mintcream},
    {255 * 0x10000 + 228 * 0x100 + 225, ColorNames::mistyrose},
    {255 * 0x10000 + 228 * 0x100 + 181, ColorNames::moccasin},
    {255 * 0x10000 + 222 * 0x100 + 173, ColorNames::navajowhite},
    {0 * 0x10000 + 0 * 0x100 + 128, ColorNames::navy},
    {253 * 0x10000 + 245 * 0x100 + 230, ColorNames::oldlace},
    {128 * 0x10000 + 128 * 0x100 + 0, ColorNames::olive},
    {107 * 0x10000 + 142 * 0x100 + 35, ColorNames::olivedrab},
    {255 * 0x10000 + 165 * 0x100 + 0, ColorNames::orange},
    {255 * 0x10000 + 69 * 0x100 + 0, ColorNames::orangered},
    {218 * 0x10000 + 112 * 0x100 + 214, ColorNames::orchid},
    {238 * 0x10000 + 232 * 0x100 + 170, ColorNames::palegoldenrod},
    {152 * 0x10000 + 251 * 0x100 + 152, ColorNames::palegreen},
    {175 * 0x10000 + 238 * 0x100 + 238, ColorNames::paleturquoise},
    {219 * 0x10000 + 112 * 0x100 + 147, ColorNames::palevioletred},
    {255 * 0x10000 + 239 * 0x100 + 213, ColorNames::papayawhip},
    {255 * 0x10000 + 218 * 0x100 + 185, ColorNames::peachpuff},
    {205 * 0x10000 + 133 * 0x100 + 63, ColorNames::peru},
    {255 * 0x10000 + 192 * 0x100 + 203, ColorNames::pink},
    {221 * 0x10000 + 160 * 0x100 + 221, ColorNames::plum},
    {176 * 0x10000 + 224 * 0x100 + 230, ColorNames::powderblue},
    {128 * 0x10000 + 0 * 0x100 + 128, ColorNames::purple},
    {255 * 0x10000 + 0 * 0x100 + 0, ColorNames::red},
    {188 * 0x10000 + 143 * 0x100 + 143, ColorNames::rosybrown},
    {65 * 0x10000 + 105 * 0x100 + 225, ColorNames::royalblue},
    {139 * 0x10000 + 69 * 0x100 + 19, ColorNames::saddlebrown},
    {250 * 0x10000 + 128 * 0x100 + 114, ColorNames::salmon},
    {244 * 0x10000 + 164 * 0x100 + 96, ColorNames::sandybrown},
    {46 * 0x10000 + 139 * 0x100 + 87, ColorNames::seagreen},
    {255 * 0x10000 + 245 * 0x100 + 238, ColorNames::seashell},
    {160 * 0x10000 + 82 * 0x100 + 45, ColorNames::sienna},
    {192 * 0x10000 + 192 * 0x100 + 192, ColorNames::silver},
    {135 * 0x10000 + 206 * 0x100 + 235, ColorNames::skyblue},
    {106 * 0x10000 + 90 * 0x100 + 205, ColorNames::slateblue},
    {112 * 0x10000 + 128 * 0x100 + 144, ColorNames::slategray},
    {255 * 0x10000 + 250 * 0x100 + 250, ColorNames::snow},
    {0 * 0x10000 + 255 * 0x100 + 127, ColorNames::springgreen},
    {70 * 0x10000 + 130 * 0x100 + 180, ColorNames::steelblue},
    {210 * 0x10000 + 180 * 0x100 + 140, ColorNames::tan},
    {0 * 0x10000 + 128 * 0x100 + 128, ColorNames::teal},
    {216 * 0x10000 + 191 * 0x100 + 216, ColorNames::thistle},
    {255 * 0x10000 + 99 * 0x100 + 71, ColorNames::tomato},
    {64 * 0x10000 + 224 * 0x100 + 208, ColorNames::turquoise},
    {238 * 0x10000 + 130 * 0x100 + 238, ColorNames::violet},
    {245 * 0x10000 + 222 * 0x100 + 179, ColorNames::wheat},
    {255 * 0x10000 + 255 * 0x100 + 255, ColorNames::white},
    {245 * 0x10000 + 245 * 0x100 + 245, ColorNames::whitesmoke},
    {255 * 0x10000 + 255 * 0x100 + 0, ColorNames::yellow},
    {154 * 0x10000 + 205 * 0x100 + 50, ColorNames::yellowgreen},
    {102 * 0x10000 + 51 * 0x100 + 153, ColorNames::rebeccapurple} };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  static const auto names_to_colors = std::unordered_map<sass::string, int>{
    {ColorNames::aliceblue, 240 * 0x10000 + 248 * 0x100 + 255},
    {ColorNames::antiquewhite, 250 * 0x10000 + 235 * 0x100 + 215},
    {ColorNames::cyan, 0 * 0x10000 + 255 * 0x100 + 255},
    {ColorNames::aquamarine, 127 * 0x10000 + 255 * 0x100 + 212},
    {ColorNames::azure, 240 * 0x10000 + 255 * 0x100 + 255},
    {ColorNames::beige, 245 * 0x10000 + 245 * 0x100 + 220},
    {ColorNames::bisque, 255 * 0x10000 + 228 * 0x100 + 196},
    {ColorNames::black, 0 * 0x10000 + 0 * 0x100 + 0},
    {ColorNames::blanchedalmond, 255 * 0x10000 + 235 * 0x100 + 205},
    {ColorNames::blue, 0 * 0x10000 + 0 * 0x100 + 255},
    {ColorNames::blueviolet, 138 * 0x10000 + 43 * 0x100 + 226},
    {ColorNames::brown, 165 * 0x10000 + 42 * 0x100 + 42},
    {ColorNames::burlywood, 222 * 0x10000 + 184 * 0x100 + 135},
    {ColorNames::cadetblue, 95 * 0x10000 + 158 * 0x100 + 160},
    {ColorNames::chartreuse, 127 * 0x10000 + 255 * 0x100 + 0},
    {ColorNames::chocolate, 210 * 0x10000 + 105 * 0x100 + 30},
    {ColorNames::coral, 255 * 0x10000 + 127 * 0x100 + 80},
    {ColorNames::cornflowerblue, 100 * 0x10000 + 149 * 0x100 + 237},
    {ColorNames::cornsilk, 255 * 0x10000 + 248 * 0x100 + 220},
    {ColorNames::crimson, 220 * 0x10000 + 20 * 0x100 + 60},
    {ColorNames::darkblue, 0 * 0x10000 + 0 * 0x100 + 139},
    {ColorNames::darkcyan, 0 * 0x10000 + 139 * 0x100 + 139},
    {ColorNames::darkgoldenrod, 184 * 0x10000 + 134 * 0x100 + 11},
    {ColorNames::darkgray, 169 * 0x10000 + 169 * 0x100 + 169},
    {ColorNames::darkgreen, 0 * 0x10000 + 100 * 0x100 + 0},
    {ColorNames::darkkhaki, 189 * 0x10000 + 183 * 0x100 + 107},
    {ColorNames::darkmagenta, 139 * 0x10000 + 0 * 0x100 + 139},
    {ColorNames::darkolivegreen, 85 * 0x10000 + 107 * 0x100 + 47},
    {ColorNames::darkorange, 255 * 0x10000 + 140 * 0x100 + 0},
    {ColorNames::darkorchid, 153 * 0x10000 + 50 * 0x100 + 204},
    {ColorNames::darkred, 139 * 0x10000 + 0 * 0x100 + 0},
    {ColorNames::darksalmon, 233 * 0x10000 + 150 * 0x100 + 122},
    {ColorNames::darkseagreen, 143 * 0x10000 + 188 * 0x100 + 143},
    {ColorNames::darkslateblue, 72 * 0x10000 + 61 * 0x100 + 139},
    {ColorNames::darkslategray, 47 * 0x10000 + 79 * 0x100 + 79},
    {ColorNames::darkturquoise, 0 * 0x10000 + 206 * 0x100 + 209},
    {ColorNames::darkviolet, 148 * 0x10000 + 0 * 0x100 + 211},
    {ColorNames::deeppink, 255 * 0x10000 + 20 * 0x100 + 147},
    {ColorNames::deepskyblue, 0 * 0x10000 + 191 * 0x100 + 255},
    {ColorNames::dimgray, 105 * 0x10000 + 105 * 0x100 + 105},
    {ColorNames::dodgerblue, 30 * 0x10000 + 144 * 0x100 + 255},
    {ColorNames::firebrick, 178 * 0x10000 + 34 * 0x100 + 34},
    {ColorNames::floralwhite, 255 * 0x10000 + 250 * 0x100 + 240},
    {ColorNames::forestgreen, 34 * 0x10000 + 139 * 0x100 + 34},
    {ColorNames::magenta, 255 * 0x10000 + 0 * 0x100 + 255},
    {ColorNames::gainsboro, 220 * 0x10000 + 220 * 0x100 + 220},
    {ColorNames::ghostwhite, 248 * 0x10000 + 248 * 0x100 + 255},
    {ColorNames::gold, 255 * 0x10000 + 215 * 0x100 + 0},
    {ColorNames::goldenrod, 218 * 0x10000 + 165 * 0x100 + 32},
    {ColorNames::gray, 128 * 0x10000 + 128 * 0x100 + 128},
    {ColorNames::grey, 128 * 0x10000 + 128 * 0x100 + 128},
    {ColorNames::green, 0 * 0x10000 + 128 * 0x100 + 0},
    {ColorNames::greenyellow, 173 * 0x10000 + 255 * 0x100 + 47},
    {ColorNames::honeydew, 240 * 0x10000 + 255 * 0x100 + 240},
    {ColorNames::hotpink, 255 * 0x10000 + 105 * 0x100 + 180},
    {ColorNames::indianred, 205 * 0x10000 + 92 * 0x100 + 92},
    {ColorNames::indigo, 75 * 0x10000 + 0 * 0x100 + 130},
    {ColorNames::ivory, 255 * 0x10000 + 255 * 0x100 + 240},
    {ColorNames::khaki, 240 * 0x10000 + 230 * 0x100 + 140},
    {ColorNames::lavender, 230 * 0x10000 + 230 * 0x100 + 250},
    {ColorNames::lavenderblush, 255 * 0x10000 + 240 * 0x100 + 245},
    {ColorNames::lawngreen, 124 * 0x10000 + 252 * 0x100 + 0},
    {ColorNames::lemonchiffon, 255 * 0x10000 + 250 * 0x100 + 205},
    {ColorNames::lightblue, 173 * 0x10000 + 216 * 0x100 + 230},
    {ColorNames::lightcoral, 240 * 0x10000 + 128 * 0x100 + 128},
    {ColorNames::lightcyan, 224 * 0x10000 + 255 * 0x100 + 255},
    {ColorNames::lightgoldenrodyellow, 250 * 0x10000 + 250 * 0x100 + 210},
    {ColorNames::lightgray, 211 * 0x10000 + 211 * 0x100 + 211},
    {ColorNames::lightgreen, 144 * 0x10000 + 238 * 0x100 + 144},
    {ColorNames::lightpink, 255 * 0x10000 + 182 * 0x100 + 193},
    {ColorNames::lightsalmon, 255 * 0x10000 + 160 * 0x100 + 122},
    {ColorNames::lightseagreen, 32 * 0x10000 + 178 * 0x100 + 170},
    {ColorNames::lightskyblue, 135 * 0x10000 + 206 * 0x100 + 250},
    {ColorNames::lightslategray, 119 * 0x10000 + 136 * 0x100 + 153},
    {ColorNames::lightsteelblue, 176 * 0x10000 + 196 * 0x100 + 222},
    {ColorNames::lightyellow, 255 * 0x10000 + 255 * 0x100 + 224},
    {ColorNames::lime, 0 * 0x10000 + 255 * 0x100 + 0},
    {ColorNames::limegreen, 50 * 0x10000 + 205 * 0x100 + 50},
    {ColorNames::linen, 250 * 0x10000 + 240 * 0x100 + 230},
    {ColorNames::maroon, 128 * 0x10000 + 0 * 0x100 + 0},
    {ColorNames::mediumaquamarine, 102 * 0x10000 + 205 * 0x100 + 170},
    {ColorNames::mediumblue, 0 * 0x10000 + 0 * 0x100 + 205},
    {ColorNames::mediumorchid, 186 * 0x10000 + 85 * 0x100 + 211},
    {ColorNames::mediumpurple, 147 * 0x10000 + 112 * 0x100 + 219},
    {ColorNames::mediumseagreen, 60 * 0x10000 + 179 * 0x100 + 113},
    {ColorNames::mediumslateblue, 123 * 0x10000 + 104 * 0x100 + 238},
    {ColorNames::mediumspringgreen, 0 * 0x10000 + 250 * 0x100 + 154},
    {ColorNames::mediumturquoise, 72 * 0x10000 + 209 * 0x100 + 204},
    {ColorNames::mediumvioletred, 199 * 0x10000 + 21 * 0x100 + 133},
    {ColorNames::midnightblue, 25 * 0x10000 + 25 * 0x100 + 112},
    {ColorNames::mintcream, 245 * 0x10000 + 255 * 0x100 + 250},
    {ColorNames::mistyrose, 255 * 0x10000 + 228 * 0x100 + 225},
    {ColorNames::moccasin, 255 * 0x10000 + 228 * 0x100 + 181},
    {ColorNames::navajowhite, 255 * 0x10000 + 222 * 0x100 + 173},
    {ColorNames::navy, 0 * 0x10000 + 0 * 0x100 + 128},
    {ColorNames::oldlace, 253 * 0x10000 + 245 * 0x100 + 230},
    {ColorNames::olive, 128 * 0x10000 + 128 * 0x100 + 0},
    {ColorNames::olivedrab, 107 * 0x10000 + 142 * 0x100 + 35},
    {ColorNames::orange, 255 * 0x10000 + 165 * 0x100 + 0},
    {ColorNames::orangered, 255 * 0x10000 + 69 * 0x100 + 0},
    {ColorNames::orchid, 218 * 0x10000 + 112 * 0x100 + 214},
    {ColorNames::palegoldenrod, 238 * 0x10000 + 232 * 0x100 + 170},
    {ColorNames::palegreen, 152 * 0x10000 + 251 * 0x100 + 152},
    {ColorNames::paleturquoise, 175 * 0x10000 + 238 * 0x100 + 238},
    {ColorNames::palevioletred, 219 * 0x10000 + 112 * 0x100 + 147},
    {ColorNames::papayawhip, 255 * 0x10000 + 239 * 0x100 + 213},
    {ColorNames::peachpuff, 255 * 0x10000 + 218 * 0x100 + 185},
    {ColorNames::peru, 205 * 0x10000 + 133 * 0x100 + 63},
    {ColorNames::pink, 255 * 0x10000 + 192 * 0x100 + 203},
    {ColorNames::plum, 221 * 0x10000 + 160 * 0x100 + 221},
    {ColorNames::powderblue, 176 * 0x10000 + 224 * 0x100 + 230},
    {ColorNames::purple, 128 * 0x10000 + 0 * 0x100 + 128},
    {ColorNames::red, 255 * 0x10000 + 0 * 0x100 + 0},
    {ColorNames::rosybrown, 188 * 0x10000 + 143 * 0x100 + 143},
    {ColorNames::royalblue, 65 * 0x10000 + 105 * 0x100 + 225},
    {ColorNames::saddlebrown, 139 * 0x10000 + 69 * 0x100 + 19},
    {ColorNames::salmon, 250 * 0x10000 + 128 * 0x100 + 114},
    {ColorNames::sandybrown, 244 * 0x10000 + 164 * 0x100 + 96},
    {ColorNames::seagreen, 46 * 0x10000 + 139 * 0x100 + 87},
    {ColorNames::seashell, 255 * 0x10000 + 245 * 0x100 + 238},
    {ColorNames::sienna, 160 * 0x10000 + 82 * 0x100 + 45},
    {ColorNames::silver, 192 * 0x10000 + 192 * 0x100 + 192},
    {ColorNames::skyblue, 135 * 0x10000 + 206 * 0x100 + 235},
    {ColorNames::slateblue, 106 * 0x10000 + 90 * 0x100 + 205},
    {ColorNames::slategray, 112 * 0x10000 + 128 * 0x100 + 144},
    {ColorNames::snow, 255 * 0x10000 + 250 * 0x100 + 250},
    {ColorNames::springgreen, 0 * 0x10000 + 255 * 0x100 + 127},
    {ColorNames::steelblue, 70 * 0x10000 + 130 * 0x100 + 180},
    {ColorNames::tan, 210 * 0x10000 + 180 * 0x100 + 140},
    {ColorNames::teal, 0 * 0x10000 + 128 * 0x100 + 128},
    {ColorNames::thistle, 216 * 0x10000 + 191 * 0x100 + 216},
    {ColorNames::tomato, 255 * 0x10000 + 99 * 0x100 + 71},
    {ColorNames::turquoise, 64 * 0x10000 + 224 * 0x100 + 208},
    {ColorNames::violet, 238 * 0x10000 + 130 * 0x100 + 238},
    {ColorNames::wheat, 245 * 0x10000 + 222 * 0x100 + 179},
    {ColorNames::white, 255 * 0x10000 + 255 * 0x100 + 255},
    {ColorNames::whitesmoke, 245 * 0x10000 + 245 * 0x100 + 245},
    {ColorNames::yellow, 255 * 0x10000 + 255 * 0x100 + 0},
    {ColorNames::yellowgreen, 154 * 0x10000 + 205 * 0x100 + 50},
    {ColorNames::rebeccapurple, 102 * 0x10000 + 51 * 0x100 + 153}
 };

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

  // const Color* name_to_color(const char* key)
  // {
  //   return name_to_color(sass::string(key));
  // }

  bool name_to_color(const sass::string& key, int& r, int& g, int& b, int& a)
  {
    sass::string lcKey = key;
    StringUtils::makeLowerCase(lcKey);
    if (lcKey == "transparent") {
      r = g = b = a = 0;
      return true;
    }
    auto p = names_to_colors.find(lcKey);
    if (p == names_to_colors.end()) return false;
    int composed = p->second;
    r = (composed & 0xFF0000) >> 16;
    g = (composed & 0x00FF00) >> 8;
    b = (composed & 0x0000FF) >> 0;
    a = 1; // always opaque
    return true;
  }
  /*
  const Color* name_to_color(const sass::string& key)
  {
    // case insensitive lookup. See #2462
    sass::string lcKey = key;
    StringUtils::makeLowerCase(lcKey);
    auto p = names_to_colors.find(lcKey);
    if (p != names_to_colors.end())
    {
      return nullptr;
    }

    if (lcKey == "red") {
      return nullptr;
    }
    return nullptr;
  }
  */

  const char* color_to_name(const int key)
  {
    auto p = colors_to_names.find(key);
    if (p != colors_to_names.end())
    {
      sass::string rv = p->second;
      // Match dart-sass output
      if (rv == "magenta")
      {
        return "fuchsia";
      }
      if (rv == "cyan")
      {
        return "aqua";
      }
      return p->second;
    }
    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

} // namespace Sass
