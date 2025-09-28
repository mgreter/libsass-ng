/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_AST_COLORS_HPP
#define SASS_AST_COLORS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_nodes.hpp"
#include "col_channel.hpp"
#include "ast_values.hpp"

namespace Sass {

  class Compiler;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////


  namespace ColorSpaces {

    const double d50[] = { 0.3457 / 0.3585, 1.00000, (1.0 - 0.3457 - 0.3585) / 0.3585 };

    // The transformation matrix for converting LMS colors to OKLab.
    // Note that this can't be directly multiplied with [xyzD65ToLms]; see Color
    // Level 4 spec for details on how to convert between XYZ and OKLab.
    const double lmsToOklab[] = {
      00.21045426830931400, 00.79361777470230540, -0.00407204301161930,
      01.97799853243116840, -2.42859224204858000, 00.45059370961741100,
      00.02590404246554780, 00.78277171245752960, -0.80867575492307740
    };

    /// The transformation matrix for converting OKLab colors to LMS.
    ///
    /// Note that this can't be directly multiplied with [lmsToXyzD65]; see Color
    /// Level 4 spec for details on how to convert between XYZ and OKLab.
    const double oklabToLms[] = {
      01.00000000000000020, 00.39633777737617490, 00.21580375730991360, //
      00.99999999999999980, -0.10556134581565854, -0.06385417282581334,
      00.99999999999999990, -0.08948417752981180, -1.29148554801940940,
    };

    // The following matrices were precomputed using
    // https://gist.github.com/nex3/3d7ecfef467b22e02e7a666db1b8a316.

    /// The transformation matrix for converting linear-light srgb colors to
    /// linear-light display-p3.
    const double linearSrgbToLinearDisplayP3[] = {
      00.82246196871436230, 00.17753803128563775, 00.00000000000000000, //
      00.03319419885096161, 00.96680580114903840, 00.00000000000000000,
      00.01708263072112003, 00.07239744066396346, 00.91051992861491650,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// linear-light srgb.
    const double linearDisplayP3ToLinearSrgb[] = {
      01.22494017628055980, -0.22494017628055996, 00.00000000000000000, //
      -0.04205695470968816, 01.04205695470968800, 00.00000000000000000,
      -0.01963755459033443, -0.07863604555063188, 01.09827360014096630,
    };

    /// The transformation matrix for converting linear-light srgb colors to
    /// linear-light a98-rgb.
    const double linearSrgbToLinearA98Rgb[] = {
      00.71512560685562470, 00.28487439314437535, 00.00000000000000000, //
      00.00000000000000000, 01.00000000000000000, 00.00000000000000000,
      00.00000000000000000, 00.04116194845011846, 00.95883805154988160,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to
    /// linear-light srgb.
    const double linearA98RgbToLinearSrgb[] = {
      01.39835574396077830, -0.39835574396077830, 00.00000000000000000, //
      00.00000000000000000, 01.00000000000000000, 00.00000000000000000,
      00.00000000000000000, -0.04292898929447326, 01.04292898929447330,
    };

    /// The transformation matrix for converting linear-light srgb colors to
    /// linear-light rec2020.
    const double linearSrgbToLinearRec2020[] = {
      00.62740389593469900, 00.32928303837788370, 00.04331306568741722, //
      00.06909728935823208, 00.91954039507545870, 00.01136231556630917,
      00.01639143887515027, 00.08801330787722575, 00.89559525324762400,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to
    /// linear-light srgb.
    const double linearRec2020ToLinearSrgb[] = {
      01.66049100210843450, -0.58764113878854950, -0.07284986331988487, //
      -0.12455047452159074, 01.13289989712596030, -0.00834942260436947,
      -0.01815076335490530, -0.10057889800800737, 01.11872966136291270,
    };

    /// The transformation matrix for converting linear-light srgb colors to xyz.
    const double linearSrgbToXyzD65[] = {
      00.41239079926595950, 00.35758433938387796, 00.18048078840183430, //
      00.21263900587151036, 00.71516867876775590, 00.07219231536073371,
      00.01933081871559185, 00.11919477979462598, 00.95053215224966060,
    };

    /// The transformation matrix for converting xyz colors to linear-light srgb.
    const double xyzD65ToLinearSrgb[] = {
      03.24096994190452130, -1.53738317757009350, -0.49861076029300330, //
      -0.96924363628087980, 01.87596750150772060, 00.04155505740717561,
      00.05563007969699360, -0.20397695888897657, 01.05697151424287860,
    };

    /// The transformation matrix for converting linear-light srgb colors to lms.
    const double linearSrgbToLms[] = {
      00.41222146947076300, 00.53633253726173480, 00.05144599326750220, //
      00.21190349581782520, 00.68069955064523420, 00.10739695353694050,
      00.08830245919005641, 00.28171883913612150, 00.62997870167382210,
    };

    /// The transformation matrix for converting lms colors to linear-light srgb.
    const double lmsToLinearSrgb[] = {
      04.07674163607595800, -3.30771153925806200, 00.23096990318210417, //
      -1.26843797328503200, 02.60975734928768900, -0.34131937600265710,
      -0.00419607613867551, -0.70341861793593630, 01.70761469407461200,
    };

    /// The transformation matrix for converting linear-light srgb colors to
    /// linear-light prophoto-rgb.
    const double linearSrgbToLinearProphotoRgb[] = {
      00.52927697762261160, 00.33015450197849283, 00.14056852039889556, //
      00.09836585954044917, 00.87347071290696180, 00.02816342755258900,
      00.01687534092138684, 00.11765941425612084, 00.86546524482249230,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// linear-light srgb.
    const double linearProphotoRgbToLinearSrgb[] = {
      02.03438084951699600, -0.72763578993413420, -0.30674505958286180, //
      -0.22882573163305037, 01.23174254119010480, -0.00291680955705449,
      -0.00855882878391742, -0.15326670213803720, 01.16182553092195470,
    };

    /// The transformation matrix for converting linear-light srgb colors to
    /// xyz-d50.
    const double linearSrgbToXyzD50[] = {
      00.43606574687426936, 00.38515150959015960, 00.14307841996513868, //
      00.22249317711056518, 00.71688701309448240, 00.06061980979495235,
      00.01392392146316939, 00.09708132423141015, 00.71409935681588070,
    };

    /// The transformation matrix for converting xyz-d50 colors to linear-light
    /// srgb.
    const double xyzD50ToLinearSrgb[] = {
      03.13413585290011780, -1.61738599801804200, -0.49066221791109754, //
      -0.97879547655577770, 01.91625437739598840, 00.03344287339036693,
      00.07195539255794733, -0.22897675981518200, 01.40538603511311820,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// linear-light a98-rgb.
    const double linearDisplayP3ToLinearA98Rgb[] = {
      00.86400513747404840, 00.13599486252595164, 00.00000000000000000, //
      -0.04205695470968816, 01.04205695470968800, 00.00000000000000000,
      -0.02056038078232985, -0.03250613804550798, 01.05306651882783790,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to
    /// linear-light display-p3.
    const double linearA98RgbToLinearDisplayP3[] = {
      01.15009441814101840, -0.15009441814101834, 00.00000000000000000, //
      00.04641729862941844, 00.95358270137058150, 00.00000000000000000,
      00.02388759479083904, 00.02650477632633013, 00.94960762888283080,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// linear-light rec2020.
    const double linearDisplayP3ToLinearRec2020[] = {
      00.75383303436172180, 00.19859736905261630, 00.04756959658566187, //
      00.04574384896535833, 00.94177721981169350, 00.01247893122294812,
      -0.00121034035451832, 00.01760171730108989, 00.98360862305342840,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to
    /// linear-light display-p3.
    const double linearRec2020ToLinearDisplayP3[] = {
      01.34357825258433200, -0.28217967052613570, -0.06139858205819628, //
      -0.06529745278911953, 01.07578791584857460, -0.01049046305945495,
      00.00282178726170095, -0.01959849452449406, 01.01677670726279310,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// xyz.
    const double linearDisplayP3ToXyzD65[] = {
      00.48657094864821626, 00.26566769316909294, 00.19821728523436250, //
      00.22897456406974884, 00.69173852183650620, 00.07928691409374500,
      00.00000000000000000, 00.04511338185890257, 01.04394436890097570,
    };

    /// The transformation matrix for converting xyz colors to linear-light
    /// display-p3.
    const double xyzD65ToLinearDisplayP3[] = {
      02.49349691194142450, -0.93138361791912360, -0.40271078445071684, //
      -0.82948896956157490, 01.76266406031834680, 00.02362468584194359,
      00.03584583024378433, -0.07617238926804170, 00.95688452400768730,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// lms.
    const double linearDisplayP3ToLms[] = {
      00.48137985274995443, 00.46211837101131803, 00.05650177623872756, //
      00.22883194181124472, 00.65321681938356760, 00.11795123880518774,
      00.08394575232299319, 00.22416527097756642, 00.69188897669944040,
    };

    /// The transformation matrix for converting lms colors to linear-light
    /// display-p3.
    const double lmsToLinearDisplayP3[] = {
      03.12776897136187370, -2.25713576259163860, 00.12936679122976494, //
      -1.09100901843779790, 02.41333171030692250, -0.32232269186912466,
      -0.02601080193857045, -0.50804133170416700, 01.53405213364273730,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// linear-light prophoto-rgb.
    const double linearDisplayP3ToLinearProphotoRgb[] = {
      00.63168691934035890, 00.21393038569465722, 00.15438269496498390, //
      00.08320371426648458, 00.88586513676302430, 00.03093114897049121,
      -0.00127273456473881, 00.05075510433665735, 00.95051763022808140,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// linear-light display-p3.
    const double linearProphotoRgbToLinearDisplayP3[] = {
      01.63257560870691790, -0.37977161848259840, -0.25280399022431950, //
      -0.15370040233755072, 01.16670254724250140, -0.01300214490495082,
      00.01039319529676572, -0.06280731264959440, 01.05241411735282870,
    };

    /// The transformation matrix for converting linear-light display-p3 colors to
    /// xyz-d50.
    const double linearDisplayP3ToXyzD50[] = {
      00.51514644296811600, 00.29200998206385770, 00.15713925139759397, //
      00.24120032212525520, 00.69222254113138180, 00.06657713674336294,
      -0.00105013914714014, 00.04187827018907460, 00.78427647146852570,
    };

    /// The transformation matrix for converting xyz-d50 colors to linear-light
    /// display-p3.
    const double xyzD50ToLinearDisplayP3[] = {
      02.40393412185549730, -0.99003044249559310, -0.39761363181465614, //
      -0.84227001614546880, 01.79895801610670820, 00.01604562477090472,
      00.04819381686413303, -0.09738519815446048, 01.27367136933212730,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to
    /// linear-light rec2020.
    const double linearA98RgbToLinearRec2020[] = {
      00.87733384166365680, 00.07749370651571998, 00.04517245182062317, //
      00.09662259146620378, 00.89152732024418050, 00.01185008828961569,
      00.02292106270284839, 00.04303668501067932, 00.93404225228647230,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to
    /// linear-light a98-rgb.
    const double linearRec2020ToLinearA98Rgb[] = {
      01.15197839471591630, -0.09750305530240860, -0.05447533941350766, //
      -0.12455047452159074, 01.13289989712596030, -0.00834942260436947,
      -0.02253038278105590, -0.04980650742838876, 01.07233689020944460,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to xyz.
    const double linearA98RgbToXyzD65[] = {
      00.57666904291013080, 00.18555823790654627, 00.18822864623499472, //
      00.29734497525053616, 00.62736356625546600, 00.07529145849399789,
      00.02703136138641237, 00.07068885253582714, 00.99133753683763890,
    };

    /// The transformation matrix for converting xyz colors to linear-light a98-rgb.
    const double xyzD65ToLinearA98Rgb[] = {
      02.04158790381074600, -0.56500697427885960, -0.34473135077832950, //
      -0.96924363628087980, 01.87596750150772060, 00.04155505740717561,
      00.01344428063203102, -0.11836239223101823, 01.01517499439120540,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to lms.
    const double linearA98RgbToLms[] = {
      00.57643225961839410, 00.36991322261987963, 00.05365451776172635, //
      00.29631647054222465, 00.59167613325218850, 00.11200739620558686,
      00.12347825101427760, 00.21949869837199862, 00.65702305061372380,
    };

    /// The transformation matrix for converting lms colors to linear-light a98-rgb.
    const double lmsToLinearA98Rgb[] = {
      02.55403683861155660, -1.62197618068286990, 00.06793934207131327, //
      -1.26843797328503200, 02.60975734928768900, -0.34131937600265710,
      -0.05623473593749381, -0.56704183956690610, 01.62327657550439990,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to
    /// linear-light prophoto-rgb.
    const double linearA98RgbToLinearProphotoRgb[] = {
      00.74011750180477920, 00.11327951328898105, 00.14660298490623970, //
      00.13755046469802620, 00.83307708026948400, 00.02937245503248977,
      00.02359772990871766, 00.07378347703906656, 00.90261879305221580,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// linear-light a98-rgb.
    const double linearProphotoRgbToLinearA98Rgb[] = {
      01.38965124815152000, -0.16945907691487766, -0.22019217123664242, //
      -0.22882573163305037, 01.23174254119010480, -0.00291680955705449,
      -0.01762544368426068, -0.09625702306122665, 01.11388246674548740,
    };

    /// The transformation matrix for converting linear-light a98-rgb colors to
    /// xyz-d50.
    const double linearA98RgbToXyzD50[] = {
      00.60977504188618140, 00.20530000261929401, 00.14922063192409227, //
      00.31112461220464155, 00.62565323083468560, 00.06322215696067286,
      00.01947059555648168, 00.06087908649415867, 00.74475492045981980,
    };

    /// The transformation matrix for converting xyz-d50 colors to linear-light
    /// a98-rgb.
    const double xyzD50ToLinearA98Rgb[] = {
      01.96246703637688060, -0.61074234048150730, -0.34135809808271540, //
      -0.97879547655577770, 01.91625437739598840, 00.03344287339036693,
      00.02870443944957101, -0.14067486633170680, 01.34891418141379370,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to xyz.
    const double linearRec2020ToXyzD65[] = {
      00.63695804830129130, 00.14461690358620838, 00.16888097516417205, //
      00.26270021201126703, 00.67799807151887100, 00.05930171646986194,
      00.00000000000000000, 00.02807269304908750, 01.06098505771079090,
    };

    /// The transformation matrix for converting xyz colors to linear-light rec2020.
    const double xyzD65ToLinearRec2020[] = {
      01.71665118797126760, -0.35567078377639240, -0.25336628137365980, //
      -0.66668435183248900, 01.61648123663493900, 00.01576854581391113,
      00.01763985744531091, -0.04277061325780865, 00.94210312123547400,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to lms.
    const double linearRec2020ToLms[] = {
      00.61675578486544440, 00.36019840122646335, 00.02304581390809228, //
      00.26513305939263670, 00.63583937206784910, 00.09902756853951408,
      00.10010262952034828, 00.20390652261661452, 00.69599084786303720,
    };

    /// The transformation matrix for converting lms colors to linear-light rec2020.
    const double lmsToLinearRec2020[] = {
      02.13990673043465130, -1.24638949376061800, 00.10648276332596668, //
      -0.88473583575776740, 02.16323093836120070, -0.27849510260343340,
      -0.04857374640044396, -0.45450314971409640, 01.50307689611454040,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to
    /// linear-light prophoto-rgb.
    const double linearRec2020ToLinearProphotoRgb[] = {
      00.83518733312972350, 00.04886884858605698, 00.11594381828421951, //
      00.05403324519953363, 00.92891840856920440, 00.01704834623126199,
      -0.00234203897072539, 00.03633215316169465, 00.96600988580903070,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// linear-light rec2020.
    const double linearProphotoRgbToLinearRec2020[] = {
      01.20065932951740800, -0.05756805370122346, -0.14309127581618444, //
      -0.06994154955888504, 01.08061789759721400, -0.01067634803832895,
      00.00554147334294746, -0.04078219298657951, 01.03524071964363200,
    };

    /// The transformation matrix for converting linear-light rec2020 colors to
    /// xyz-d50.
    const double linearRec2020ToXyzD50[] = {
      00.67351546318827600, 00.16569726370390453, 00.12508294953738705, //
      00.27905900514112060, 00.67531800574910980, 00.04562298910976962,
      -0.00193242713400438, 00.02997782679282923, 00.79705920285163550,
    };

    /// The transformation matrix for converting xyz-d50 colors to linear-light
    /// rec2020.
    const double xyzD50ToLinearRec2020[] = {
      01.64718490467176600, -0.39368189813164710, -0.23595963848828266, //
      -0.68266410741738180, 01.64771461274440760, 00.01281708338512084,
      00.02966887665275675, -0.06292589642970030, 01.25355782018657710,
    };

    /// The transformation matrix for converting xyz colors to lms.
    const double xyzD65ToLms[] = {
      00.81902243799670300, 00.36190626005289034, -0.12887378152098788, //
      00.03298365393238846, 00.92928686158634330, 00.03614466635064235,
      00.04817718935962420, 00.26423953175273080, 00.63354782846943080,
    };

    /// The transformation matrix for converting lms colors to xyz.
    const double lmsToXyzD65[] = {
      01.22687987584592430, -0.55781499446021710, 00.28139104566596460, //
      -0.04057574521480084, 01.11228680328031730, -0.07171105806551635,
      -0.07637293667466007, -0.42149333240224324, 01.58692401983678180,
    };

    /// The transformation matrix for converting xyz colors to linear-light
    /// prophoto-rgb.
    const double xyzD65ToLinearProphotoRgb[] = {
      01.40319046337749790, -0.22301514479051668, -0.10160668507413790, //
      -0.52623840216330720, 01.48163196292346440, 00.01701879027252688,
      -0.01120226528622150, 00.01824640347962099, 00.91124722749150480,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// xyz.
    const double linearProphotoRgbToXyzD65[] = {
      00.75559074229692100, 00.11271984265940525, 00.08214534209534540, //
      00.26832184357857190, 00.71511525666179120, 00.01656289975963685,
      00.00391597276242580, -0.01293344283684181, 01.09807522083429450,
    };

    /// The transformation matrix for converting xyz colors to xyz-d50.
    const double xyzD65ToXyzD50[] = {
      01.04792979254499660, 00.02294687060160952, -0.05019226628920519, //
      00.02962780877005567, 00.99043442675388000, -0.01707379906341879,
      -0.00924304064620452, 00.01505519149029816, 00.75187428142813700,
    };

    /// The transformation matrix for converting xyz-d50 colors to xyz.
    const double xyzD50ToXyzD65[] = {
      00.95547342148807520, -0.02309845494876452, 00.06325924320057065, //
      -0.02836970933386358, 01.00999539808130410, 00.02104144119191730,
      00.01231401486448199, -0.02050764929889898, 01.33036592624212400,
    };

    /// The transformation matrix for converting lms colors to linear-light
    /// prophoto-rgb.
    const double lmsToLinearProphotoRgb[] = {
      01.73835514811572070, -0.98795094275144580, 00.24959579463572504, //
      -0.70704940153292660, 01.93437004444013820, -0.22732064290721150,
      -0.08407882206239634, -0.35754060521141334, 01.44161942727380970,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// lms.
    const double linearProphotoRgbToLms[] = {
      00.71544846056555340, 00.35279155007721186, -0.06824001064276530, //
      00.27441164900156710, 00.66779764984123670, 00.05779070115719616,
      00.10978443261622942, 00.18619829115002018, 00.70401727623375040,
    };

    /// The transformation matrix for converting lms colors to xyz-d50.
    const double lmsToXyzD50[] = {
      01.28858621817270600, -0.53787174449737450, 00.21358120275423640, //
      -0.00253387643187372, 01.09231679887191650, -0.08978292244004273,
      -0.06937382305734124, -0.29500839894431263, 01.18948682451211420,
    };

    /// The transformation matrix for converting xyz-d50 colors to lms.
    const double xyzD50ToLms[] = {
      00.77070004204311720, 00.34924840261939616, -0.11202351884164681, //
      00.00559649248368848, 00.93707234011367690, 00.06972568836252771,
      00.04633714262191069, 00.25277531574310524, 00.85145807674679600,
    };

    /// The transformation matrix for converting linear-light prophoto-rgb colors to
    /// xyz-d50.
    const double linearProphotoRgbToXyzD50[] = {
      00.79776664490064230, 00.13518129740053308, 00.03134773412839220, //
      00.28807482881940130, 00.71183523424187300, 00.00008993693872564,
      00.00000000000000000, 00.00000000000000000, 00.82510460251046020,
    };

    /// The transformation matrix for converting xyz-d50 colors to linear-light
    /// prophoto-rgb.
    const double xyzD50ToLinearProphotoRgb[] = {
      01.34578688164715830, -0.25557208737979464, -0.05110186497554526, //
      -0.54463070512490190, 01.50824774284514680, 00.02052744743642139,
      00.00000000000000000, 00.00000000000000000, 01.21196754563894520,
    };


  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  const LinearChannel AlphaChannel("alpha", 0, 1, false, false, false);



  const ColorChannel hsl_channels[3]{
    ColorChannel("hue", true, "deg"),
    LinearChannel("saturation", 0, 100, false, true, false, false, "%"),
    LinearChannel("lightness", 0, 100, true, false, false, false, "%")
  };

  const ColorChannel hwb_channels[3]{
    ColorChannel("hue", true, "deg"),
    LinearChannel("whiteness", 0, 100, true, false, false, false, "%"),
    LinearChannel("blackness", 0, 100, true, false, false, false, "%")
  };

  const ColorChannel lab_channels[3]{
    LinearChannel("lightness", 0, 100, false, true, true, false, "%"),
    LinearChannel("a", -125, 125, false, false, false),
    LinearChannel("b", -125, 125, false, false, false)
  };

  const ColorChannel lch_channels[3]{
    LinearChannel("lightness", 0, 100, false, true, true, false, "%"),
    LinearChannel("chroma", 0, 150, false, true, false),
    ColorChannel("hue", true, "deg"),
  };

  const ColorChannel lms_channels[3]{
    LinearChannel("long", 0, 1, false, false, false),
    LinearChannel("medium", 0, 1, false, false, false),
    LinearChannel("short", 0, 1, false, false, false)
  };

  const ColorChannel oklab_channels[3]{
    LinearChannel("lightness", 0, 1, false, true, true, true, "%"),
    LinearChannel("a", -0.4, 0.4, false, false, false),
    LinearChannel("b", -0.4, 0.4, false, false, false)
  };

  const ColorChannel oklch_channels[3]{
    LinearChannel("lightness", 0, 1, false, true, true, true, "%"),
    LinearChannel("chroma", 0, 0.4, false, true, false),
    ColorChannel("hue", true, "deg"),
  };

  const ColorChannel rgb_channels[3]{
    LinearChannel("red", 0, 1, false, false, false),
    LinearChannel("green", 0, 1, false, false, false),
    LinearChannel("blue", 0, 1, false, false, false)
  };

  const ColorChannel rgb255_channels[3]{
    LinearChannel("red", 0, 255, false, true, true),
    LinearChannel("green", 0, 255, false, true, true),
    LinearChannel("blue", 0, 255, false, true, true)
  };

  const ColorChannel xyz_channels[3]{
    LinearChannel("x", 0, 1, false, false, false),
    LinearChannel("y", 0, 1, false, false, false),
    LinearChannel("z", 0, 1, false, false, false)
  };

  const ColorChannel xyz255_channels[3]{
    LinearChannel("x", 0, 255, false, true, true),
    LinearChannel("y", 0, 255, false, true, true),
    LinearChannel("z", 0, 255, false, true, true)
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class A98RgbColorSpace : public ColorSpace {
  public:
    A98RgbColorSpace() : ColorSpace(
      "a98-rgb",
      SassColorSpace::A98RGB,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(std::abs(channel), 563.0 / 256.0);
    }
    double fromLinear(double channel) const override final {
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(std::abs(channel), 256.0 / 563.0);
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::linearA98RgbToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::linearA98RgbToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::linearA98RgbToLinearSrgb;
      if (dest.name() == str_display_p3) return ColorSpaces::linearA98RgbToLinearDisplayP3;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::linearA98RgbToLinearProphotoRgb;
      if (dest.name() == str_rec2020) return ColorSpaces::linearA98RgbToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearA98RgbToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearA98RgbToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearA98RgbToLms;
      return ColorSpace::transformationMatrix(dest);
    }
  };

  static double srgbAndDisplayP3ToLinear(double channel) {
    double abs = std::abs(channel);
    if (abs <= 0.04045) return channel / 12.92;
    return (std::signbit(channel) ? -1 : +1) *
      std::pow((abs + 0.055) / 1.055, 2.4);
  }

  static double srgbAndDisplayP3FromLinear(double channel) {
    double abs = std::abs(channel);
    if (abs <= 0.0031308) return channel * 12.92;
    return (std::signbit(channel) ? -1 : +1) *
      (1.055 * std::pow(abs, 1.0 / 2.4) - 0.055);
  }

  class DisplayP3ColorSpace : public ColorSpace {
  public:
    DisplayP3ColorSpace() : ColorSpace(
      "display-p3",
      SassColorSpace::DISPLAY_P3,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      return srgbAndDisplayP3ToLinear(channel);
    }
    double fromLinear(double channel) const override final {
      return srgbAndDisplayP3FromLinear(channel);
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::linearDisplayP3ToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::linearDisplayP3ToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::linearDisplayP3ToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::linearDisplayP3ToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::linearDisplayP3ToLinearProphotoRgb;
      if (dest.name() == str_rec2020) return ColorSpaces::linearDisplayP3ToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearDisplayP3ToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearDisplayP3ToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearDisplayP3ToLms;
      return ColorSpace::transformationMatrix(dest);
    }
  };

  class HslColorSpace : public ColorSpace {
  public:
    HslColorSpace() : ColorSpace(
      "hsl",
      SassColorSpace::HSL,
      hsl_channels, 3)
    {}

    bool isLegacy() const override final { return true; }
    bool isPolar() const override final { return true; }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> saturation,
      tl::optional<double> lightness,
      tl::optional<double> alpha)
        const override final;

  };

  class HwbColorSpace : public ColorSpace {
  public:
    HwbColorSpace() : ColorSpace(
      "hwb",
      SassColorSpace::HWB,
      hwb_channels, 3)
    {}

    bool isLegacy() const override final { return true; }
    bool isPolar() const override final { return true; }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha) const override final;

  };

  class LabColorSpace : public ColorSpace {
  public:
    LabColorSpace() : ColorSpace(
      "lab",
      SassColorSpace::LAB,
      lab_channels, 3)
    {}

    bool isBounded() const override final { return false; }

    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha,
      bool missingChroma = false,
      bool missingHue = false) const;

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
      const override final
    {
      return translate(dest, pstate,
        hue, whiteness, blackness, alpha);
    }

  };

  class LchColorSpace : public ColorSpace {
  public:
    LchColorSpace() : ColorSpace(
      "lch",
      SassColorSpace::LCH,
      lch_channels, 3)
    {}

    bool isPolar() const override final { return true; }
    bool isBounded() const override final { return false; }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha)
        const override final;

  };

  class LmsColorSpace : public ColorSpace {
  public:
    LmsColorSpace() : ColorSpace(
      "lms",
      SassColorSpace::LMS,
      lms_channels, 3)
    {}

    bool isBounded() const override final { return false; }

    double toLinear(double channel) const override final {
      return channel;
    }
    double fromLinear(double channel) const override final {
      return channel;
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::lmsToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::lmsToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::lmsToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::lmsToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::lmsToLinearProphotoRgb;
      if (dest.name() == str_display_p3) return ColorSpaces::lmsToLinearDisplayP3;
      if (dest.name() == str_rec2020) return ColorSpaces::lmsToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::lmsToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::lmsToXyzD50;
      return ColorSpace::transformationMatrix(dest);
    }

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
      const override final
    {
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  class OkLabColorSpace : public ColorSpace {
  public:
    OkLabColorSpace() : ColorSpace(
      "oklab",
      SassColorSpace::OKLAB,
      oklab_channels, 3)
    {}

    bool isBounded() const override final { return false; }

    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha,
      bool missingChroma = false,
      bool missingHue = false) const;

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha)
      const override final
    {
      return translate(dest, pstate,
        lightness, chroma, hue, alpha);
    }

  };

  class OkLchColorSpace : public ColorSpace {
  public:
    OkLchColorSpace() : ColorSpace(
      "oklch",
      SassColorSpace::OKLCH,
      oklch_channels, 3)
    {}

    bool isPolar() const override final { return true; }
    bool isBounded() const override final { return false; }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> lightness,
      tl::optional<double> chroma,
      tl::optional<double> hue,
      tl::optional<double> alpha) const override final;

  };

  class ProphotoRgbColorSpace : public ColorSpace {
  public:
    ProphotoRgbColorSpace() : ColorSpace(
      "prophoto-rgb",
      SassColorSpace::PROPHOTO_RGB,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      double abs = std::abs(channel);
      if (abs <= 16.0 / 512) return channel / 16.0;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(abs, 1.8 / 1.0);

    }
    double fromLinear(double channel) const override final {
      double abs = std::abs(channel);
      if (abs < 1.0 / 512) return 16.0 * channel;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow(abs, 1.0 / 1.8);
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::linearProphotoRgbToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::linearProphotoRgbToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::linearProphotoRgbToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::linearProphotoRgbToLinearA98Rgb;
      if (dest.name() == str_display_p3) return ColorSpaces::linearProphotoRgbToLinearDisplayP3;
      if (dest.name() == str_rec2020) return ColorSpaces::linearProphotoRgbToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearProphotoRgbToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearProphotoRgbToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearProphotoRgbToLms;
      return ColorSpace::transformationMatrix(dest);
    }

  };

  /// A constant used in the rec2020 gamma encoding/decoding functions.
  const double _alpha = 1.09929682680944;

  /// A constant used in the rec2020 gamma encoding/decoding functions.
  const double _beta = 0.018053968510807;


  class Rec2020ColorSpace : public ColorSpace {
  public:
    Rec2020ColorSpace() : ColorSpace(
      "rec2020",
      SassColorSpace::REC2020,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      double abs = std::abs(channel);
      if (abs < _beta * 4.5) return channel / 4.5;
      return (std::signbit(channel) ? -1 : +1) *
        std::pow((abs + _alpha -1.0) / _alpha, 1.0 / 0.45);

    }
    double fromLinear(double channel) const override final {
      double abs = std::abs(channel);
      if (abs < _beta) return 4.5 * channel;
      return (std::signbit(channel) ? -1 : +1) *
        (_alpha * std::pow(abs, 0.45) - (_alpha - 1.0));
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::linearRec2020ToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::linearRec2020ToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::linearRec2020ToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::linearRec2020ToLinearA98Rgb;
      if (dest.name() == str_display_p3) return ColorSpaces::linearRec2020ToLinearDisplayP3;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::linearRec2020ToLinearProphotoRgb;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearRec2020ToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearRec2020ToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearRec2020ToLms;
      return ColorSpace::transformationMatrix(dest);
    }

  };

  class RgbColorSpace : public ColorSpace {
  public:
    RgbColorSpace() : ColorSpace(
      "rgb",
      SassColorSpace::RGB,
      rgb255_channels, 3)
    {}

    bool isLegacy() const override final { return true; }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> channel0,
      tl::optional<double> channel1,
      tl::optional<double> channel2,
      tl::optional<double> alpha) const override final;

    double toLinear(double channel) const override final {
      return srgbAndDisplayP3ToLinear(channel / 255.0);
    }
    double fromLinear(double channel) const override final {
      double rv = srgbAndDisplayP3FromLinear(channel) * 255.0;
      // std::cerr << " from rgb linear " << channel << " => " << rv << "\n";
      return rv;
    }
  };

  class SrgbLinearColorSpace : public ColorSpace {
  public:
    SrgbLinearColorSpace() : ColorSpace(
      "srgb-linear",
      SassColorSpace::SRGB_LINEAR,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      return channel;
    }
    double fromLinear(double channel) const override final {
      return channel;
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_display_p3) return ColorSpaces::linearSrgbToLinearDisplayP3;
      if (dest.name() == str_a98_rgb) return ColorSpaces::linearSrgbToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::linearSrgbToLinearProphotoRgb;
      if (dest.name() == str_rec2020) return ColorSpaces::linearSrgbToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearSrgbToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearSrgbToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearSrgbToLms;
      return ColorSpace::transformationMatrix(dest);
    }

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> hue,
      tl::optional<double> whiteness,
      tl::optional<double> blackness,
      tl::optional<double> alpha) const override final;

  };

  class SrgbColorSpace : public ColorSpace {
  public:
    SrgbColorSpace() : ColorSpace(
      "srgb",
      SassColorSpace::SRGB,
      rgb_channels, 3)
    {}

    double toLinear(double channel) const override final {
      return srgbAndDisplayP3ToLinear(channel);
    }
    double fromLinear(double channel) const override final {
      return srgbAndDisplayP3FromLinear(channel);
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_display_p3) return ColorSpaces::linearSrgbToLinearDisplayP3;
      if (dest.name() == str_a98_rgb) return ColorSpaces::linearSrgbToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::linearSrgbToLinearProphotoRgb;
      if (dest.name() == str_rec2020) return ColorSpaces::linearSrgbToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::linearSrgbToXyzD65;
      if (dest.name() == str_xyz_d50) return ColorSpaces::linearSrgbToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::linearSrgbToLms;
      return ColorSpace::transformationMatrix(dest);
    }

    Color* translate(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha,
      bool missingLightness = false,
      bool missingChroma = false,
      bool missingHue = false) const;

    Color* convert(
      const ColorSpace& dest,
      const SourceSpan& pstate,
      tl::optional<double> red,
      tl::optional<double> green,
      tl::optional<double> blue,
      tl::optional<double> alpha)
      const override final
    {
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  class XyzD50ColorSpace : public ColorSpace {
  public:
    XyzD50ColorSpace() : ColorSpace(
      "xyz-d50",
      SassColorSpace::XYZ_D50,
      xyz_channels, 3)
    {}

    bool isBounded() const override final { return false; }

    double toLinear(double channel) const override final {
      return channel;
    }
    double fromLinear(double channel) const override final {
      return channel;
    }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::xyzD50ToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::xyzD50ToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::xyzD50ToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::xyzD50ToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::xyzD50ToLinearProphotoRgb;
      if (dest.name() == str_display_p3) return ColorSpaces::xyzD50ToLinearDisplayP3;
      if (dest.name() == str_rec2020) return ColorSpaces::xyzD50ToLinearRec2020;
      if (dest.name() == str_xyz_d65) return ColorSpaces::xyzD50ToXyzD65;
      if (dest.name() == str_lms) return ColorSpaces::xyzD50ToLms;
      return ColorSpace::transformationMatrix(dest);
    }

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
      const override final
    {
      return translate(dest, pstate,
        red, green, blue, alpha);
    }

  };

  class XyzD65ColorSpace : public ColorSpace {
  public:
    XyzD65ColorSpace() : ColorSpace(
      "xyz",
      SassColorSpace::XYZ_D65,
      xyz_channels, 3)
    {}

    double toLinear(double channel) const override final {
      return channel;
    }
    double fromLinear(double channel) const override final {
      return channel;
    }

    bool isBounded() const override final { return false; }

    const double* transformationMatrix(ColorSpace dest) const override final {
      if (dest.name() == str_rgb) return ColorSpaces::xyzD65ToLinearSrgb;
      if (dest.name() == str_srgb) return ColorSpaces::xyzD65ToLinearSrgb;
      if (dest.name() == str_srgb_linear) return ColorSpaces::xyzD65ToLinearSrgb;
      if (dest.name() == str_a98_rgb) return ColorSpaces::xyzD65ToLinearA98Rgb;
      if (dest.name() == str_prophoto_rgb) return ColorSpaces::xyzD65ToLinearProphotoRgb;
      if (dest.name() == str_display_p3) return ColorSpaces::xyzD65ToLinearDisplayP3;
      if (dest.name() == str_rec2020) return ColorSpaces::xyzD65ToLinearRec2020;
      if (dest.name() == str_xyz_d50) return ColorSpaces::xyzD65ToXyzD50;
      if (dest.name() == str_lms) return ColorSpaces::xyzD65ToLms;
      return ColorSpace::transformationMatrix(dest);
    }

  };

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
    size_t hash() const override final;
    SassValueType getTag() const override final { return SASS_COLOR; }
    const sass::string& type() const override final { return Strings::color; }

    // Main entry point for Value Visitor pattern
    void accept(ValueVisitor<void>* visitor) override final {
      return visitor->visitColor(this);
    }
    Value* accept(ValueVisitor<Value*>* visitor) override final {
      return visitor->visitColor(this);
    }

    // Implement some operations for base value class
    Value* plus(const Value* other, Logger& logger, const SourceSpan& pstate) const override final;
    Value* minus(const Value* other, Logger& logger, const SourceSpan& pstate) const override final;
    Value* dividedBy(const Value* other, Logger& logger, const SourceSpan& pstate) const override final;
    Value* modulo(const Value* other, Logger& logger, const SourceSpan& pstate) const override final;
    Value* remainder(const Value* other, Logger& logger, const SourceSpan& pstate) const override final;

    // Im ement equality comparators for base value class
    bool operator==(const Value& rhs) const override final;
    // Implement same class compare operator
    bool operator==(const Color& rhs) const;

    const Color* assertColor(Logger& logger, const sass::string& name = Strings::empty) const override final { return this; }
    Color* assertColor2(Logger& logger, const sass::string& name = Strings::empty) override final { return this; }

    // Copy operations for childless items
    Color* copy(SASS_MEMORY_ARGS bool childless) const override final {
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
    Value* accept(ExpressionVisitor<Value*>* visitor) override final {
      return visitor->visitColorExpression(this);
    }
    Expression* accept(ExpressionVisitor<Expression*>* visitor) override final {
      return visitor->visitColorExpression(this);
    }

    // Return if expression can be used in calculations
    bool isCalcSafe() override final { return false; }

    // Convert to string (only for debugging)
    sass::string toString() const override final;

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
