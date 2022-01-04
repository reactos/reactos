/*
 * ReactOS Calc (Unit conversion engine)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

/*
    The conversion is made by converting the "from" unit
    into the I.S. unit and then the I.S. unit is converted
    into the "from" one.

    The base units for all categories are:

    ANGLE..........degrees
    AREA...........square meters
    CONSUMPTION....kilometers per liter
    CURRENCY.......Euro
    ENERGY.........joule
    LENGTHS........meters
    POWER..........Watt
    PRESSURE.......Pascal
    TEMPERATURE....kelvin
    TIME...........seconds
    VELOCITY.......meters per second
    VOLUME.........liter
    WEIGHT.........gram

    The '$' character is used as "what you read into the display".
*/
typedef struct {
    DWORD unit;
    const char *formula_from;
    const char *formula_to;
} conv_t;

typedef struct {
    const DWORD   category;
    const conv_t *items;
} conv_category_t;

#define DECLARE_CONV_CAT(_category) \
    { IDS_CONV_##_category, conv_##_category },

#define DECLARE_CONV_UNIT(_category, _unit, _from, _to ) \
    { IDS_##_category##_##_unit, _from, _to },

#define DECLARE_CONV_END \
    { 0, NULL, NULL },

/*
    1 arcminute ....... = 1/60 deg
    1 arcsecond ....... = 1/3600 deg
    1 gradian ......... = 0.9 deg
    1 radian .......... = 57.29577951308233 deg
*/
static const conv_t conv_ANGLE[] = {
    DECLARE_CONV_UNIT(ANGLE, ARCMINUTES, "$/60",    "$*60")
    DECLARE_CONV_UNIT(ANGLE, ARCSECONDS, "$/3600",  "$*3600")
    DECLARE_CONV_UNIT(ANGLE, DEGREES,    "$",       "$")
    DECLARE_CONV_UNIT(ANGLE, GRADIANS,   "$*0.9",   "$/0.9")
    DECLARE_CONV_UNIT(ANGLE, RADIANS,    "$/P*180", "$*P/180")
    DECLARE_CONV_END
};

/*
    1 acre ................ = 4840 square yd = 4046,8564224 mq
    1 acre brazil ......... =
    1 acre france ......... =
    1 acre scots .......... = 5000 mq
    1 acre us ............. = 4840*(36/39.37)^2 m = 6272640/1549.9969 m
    1 are ................. = 100 mq
    1 chou ................ = 108000*(10/33)^2 mq
    1 danbo ............... = 991.74 mq
    1 ha .................. = 10000 mq
    1 jeongbo ............. = 9917.4 mq
    1 morgen hungary ...... = 5754.642 mq
    1 mu .................. = 2000/3 mq
    1 ping ................ =
    1 pyeong .............. = 400/121 mq
    1 pyeongbangja ........ =
    1 rai ................. = 1600 mq
    1 se .................. = 1080*(10/33)^2 mq
    1 square cm ........... = 0.0001 mq
    1 square chr .......... =
    1 square fathom ....... = 1.8288^2 = 3.34450944 mq
    1 square fathom hungary = 1.8964838^2 = 3.59665080366244 mq
    1 square ft ........... = 0,09290304 mq
    1 square in ........... = 0,00064516 mq
    1 square km ........... = 1000000 mq
    1 square lar .......... =
    1 square mile ......... = 1609.344^2 = 2589988.110336 mq
    1 square mm ........... = 0,000001 mq
    1 square shaku ........ = (10/33)^2 mq
    1 square tsuen ........ =
    1 square va ........... = 4 mq
    1 square yard ......... = 0,83612736 mq
    1 tan ................. = 10800*(10/33)^2 mq
    1 tsubo ............... = 36*(10/33)^2 mq
*/
static const conv_t conv_AREA[] = {
    DECLARE_CONV_UNIT(AREA, ACRES,                  "$*4046,8564224",    "$/4046,8564224")
//    DECLARE_CONV_UNIT(AREA, ACRES_BRAZIL,           "$", "$")
//    DECLARE_CONV_UNIT(AREA, ACRES_FRANCE,           "$", "$")
    DECLARE_CONV_UNIT(AREA, ACRES_US,               "$*627264/154.99969", "$/627264*154.99969")
    DECLARE_CONV_UNIT(AREA, ACRES_SCOTS,            "$*5000",            "$/5000")
    DECLARE_CONV_UNIT(AREA, ARES,                   "$*100",             "$/100")
    DECLARE_CONV_UNIT(AREA, CHOU,                   "$*10800000/1089",   "$*1089/10800000")
    DECLARE_CONV_UNIT(AREA, DANBO,                  "$*991.74",          "$/991.74")
    DECLARE_CONV_UNIT(AREA, HECTARES,               "$*10000",           "$/10000")
    DECLARE_CONV_UNIT(AREA, JEONGBO,                "$*9917.4",          "$/9917.4")
//    DECLARE_CONV_UNIT(AREA, MORGEN_HUNGARY,         "$", "$")
    DECLARE_CONV_UNIT(AREA, MU,                     "$*(2000/3)",        "$/(2000/3)")
//    DECLARE_CONV_UNIT(AREA, PING,                   "$", "$")
    DECLARE_CONV_UNIT(AREA, PYEONG,                 "$*400/121",         "$*121/400")
//    DECLARE_CONV_UNIT(AREA, PYEONGBANGJA,           "$", "$")
    DECLARE_CONV_UNIT(AREA, RAI,                    "$*1600",            "$/1600")
    DECLARE_CONV_UNIT(AREA, SE,                     "$*108000/1089",     "$*1089/108000")
    DECLARE_CONV_UNIT(AREA, SQUARE_CENTIMETERS,     "$*0,0001",          "$/0,0001")
//    DECLARE_CONV_UNIT(AREA, SQUARE_CHR,             "$", "$")
    DECLARE_CONV_UNIT(AREA, SQUARE_FATHOMS,         "$*3.34450944",      "$/3.34450944")
    DECLARE_CONV_UNIT(AREA, SQUARE_FATHOMS_HUNGARY, "$*3.59665080366244", "$/3.59665080366244")
    DECLARE_CONV_UNIT(AREA, SQUARE_FEET,            "$*0,09290304",      "$/0,09290304")
    DECLARE_CONV_UNIT(AREA, SQUARE_INCHES,          "$*0,00064516",      "$/0.00064516")
    DECLARE_CONV_UNIT(AREA, SQUARE_KILOMETERS,      "$*1000000",         "$/1000000")
//    DECLARE_CONV_UNIT(AREA, SQUARE_LAR,             "$", "$")
    DECLARE_CONV_UNIT(AREA, SQUARE_METER,           "$",                 "$")
    DECLARE_CONV_UNIT(AREA, SQUARE_MILES,           "$*2589988.110336",  "$/2589988.110336")
    DECLARE_CONV_UNIT(AREA, SQUARE_MILLIMETERS,     "$*1000000",         "$/1000000")
    DECLARE_CONV_UNIT(AREA, SQUARE_SHAKU,           "$*100/1089",        "$/1089*100")
//    DECLARE_CONV_UNIT(AREA, SQUARE_TSUEN,           "$", "$")
    DECLARE_CONV_UNIT(AREA, SQUARE_VA,              "$*4",               "$/4")
    DECLARE_CONV_UNIT(AREA, SQUARE_YARD,            "$*0,83612736",      "$/0,83612736")
    DECLARE_CONV_UNIT(AREA, TAN,                    "$*1080000/1089",    "$*1089/1080000")
    DECLARE_CONV_UNIT(AREA, TSUBO,                  "$*1188/1089",       "$*1089/1188")
    DECLARE_CONV_END
};

/*
    1 l/100Km = 100 km/l -> y = 100/x
    1 miles/gal uk = 1.609344/4.54609 km/l
    1 miles/gal us = 1.609344/3.785411784 km/l
*/
static const conv_t conv_CONSUMPTION[] = {
    DECLARE_CONV_UNIT(CONSUMPTION, KM_PER_L,        "$", "$")
    DECLARE_CONV_UNIT(CONSUMPTION, L_PER_100_KM,    "100/$", "100/$")
    DECLARE_CONV_UNIT(CONSUMPTION, MILES_GALLON_UK, "$*1.609344/4.54609", "$/1.609344*4.54609")
    DECLARE_CONV_UNIT(CONSUMPTION, MILES_GALLON_US, "$*1.609344/3.785411784", "$/1.609344*3.785411784")
    DECLARE_CONV_END
};

static const conv_t conv_CURRENCY[] = {
    DECLARE_CONV_UNIT(CURRENCY, AUSTRIAN_SCHILLING, "$/13,7603", "$*13,7603")
    DECLARE_CONV_UNIT(CURRENCY, BELGIAN_FRANC,      "$/40,3399", "$*40,3399")
    DECLARE_CONV_UNIT(CURRENCY, CYPRIOT_POUND,      "$/0,585274","$*0,585274")
    DECLARE_CONV_UNIT(CURRENCY, DEUTSCHE_MARK,      "$/1,95583", "$*1,95583")
    DECLARE_CONV_UNIT(CURRENCY, DUTCH_GUILDER,      "$/2,20371", "$*2,20371")
    DECLARE_CONV_UNIT(CURRENCY, ESTONIAN_KROON,     "$/15,6466", "$*15,6466")
    DECLARE_CONV_UNIT(CURRENCY, EURO,               "$",         "$")
    DECLARE_CONV_UNIT(CURRENCY, FINNISH_MARKKA,     "$/5,94573", "$*5,94573")
    DECLARE_CONV_UNIT(CURRENCY, FRENCH_FRANC,       "$/6,55957", "$*6,55957")
    DECLARE_CONV_UNIT(CURRENCY, GREEK_DRACHMA,      "$/340,751", "$*340,751")
    DECLARE_CONV_UNIT(CURRENCY, IRISH_POUND,        "$/0,787564","$*0,787564")
    DECLARE_CONV_UNIT(CURRENCY, ITALIAN_LIRA,       "$/1936.27", "$*1936.27")
    DECLARE_CONV_UNIT(CURRENCY, LATVIAN_LATS,       "$/0,7028",  "$*0,7028")
    DECLARE_CONV_UNIT(CURRENCY, LITHUANIAN_LITAS,   "$/3,45280", "$*3,45280")
    DECLARE_CONV_UNIT(CURRENCY, LUXEMBOURG_FRANC,   "$/40,3399", "$*40,3399")
    DECLARE_CONV_UNIT(CURRENCY, MALTESE_LIRA,       "$/0.42930", "$*0.42930")
    DECLARE_CONV_UNIT(CURRENCY, PORTOGUESE_ESCUDO,  "$/200,482", "$*200,482")
    DECLARE_CONV_UNIT(CURRENCY, SLOVAK_KORUNA,      "$/30,126",  "$*30,126")
    DECLARE_CONV_UNIT(CURRENCY, SLOVENIAN_TOLAR,    "$/239,640", "$*239,640")
    DECLARE_CONV_UNIT(CURRENCY, SPANISH_PESETA,     "$/166,386", "$*166,386")
    DECLARE_CONV_END
};

/*
    1 cal15 .... = 4.1855 J
    1 BTU ...... = 1055.056 J
    1 ERG ...... = 0.0000001 J
    1 eV ....... = 0.000000000000000000160217653 J
    1 Foot-Pound = 1.3558179483314004 J
    1 IT calorie = 4.1868 J
    1 KCa ...... = 4186.8 J
    1 KJ ....... = 1000 J
    1 KWh ...... = 3600 J
    1 IUNS ..... = 4.182 J
    1 calth .... = 4.184 J
*/
static const conv_t conv_ENERGY[] = {
    DECLARE_CONV_UNIT(ENERGY, 15_C_CALORIES,        "$*4.1855",   "$/4.1855")
    DECLARE_CONV_UNIT(ENERGY, BTUS,                 "$*1055.056", "$/1055.056")
    DECLARE_CONV_UNIT(ENERGY, ERGS,                 "$*.0000001", "$/.0000001")
    DECLARE_CONV_UNIT(ENERGY, EVS,                  "$*1.60217653/1X19", "$/1.60217653*1X19")
    DECLARE_CONV_UNIT(ENERGY, FOOT_POUNDS,          "$*1.3558179483314004",   "$/1.3558179483314004")
    DECLARE_CONV_UNIT(ENERGY, IT_CALORIES,          "$*4.1868",   "$/4.1868")
    DECLARE_CONV_UNIT(ENERGY, IT_KILOCALORIES,      "$*4186.8",   "$/4186.8")
    DECLARE_CONV_UNIT(ENERGY, JOULES,               "$",          "$")
    DECLARE_CONV_UNIT(ENERGY, KILOJOULES,           "$*1000",     "$/1000")
    DECLARE_CONV_UNIT(ENERGY, KILOWATT_HOURS,       "$*3600",     "$/3600")
    DECLARE_CONV_UNIT(ENERGY, NUTRITION_CALORIES,   "$*4.182",    "$/4.182")
    DECLARE_CONV_UNIT(ENERGY, TH_CALORIES,          "$*4.184",    "$/4.184")
    DECLARE_CONV_END
};

/*
    1 angstrom ....... = 0.0000000001 m
    1 astronomical unit= 149598000000 m
    1 barleycorn ..... = 1/3 inch = 0.9144/108 m
    1 cm ............. = 1/100 m
    1 chain uk ....... = 22 yards = 22*0.9144 m
    1 chi ............ = 1/3 m
    1 chou ........... = 3600/33 m
    1 chr ............ =
    1 cun ............ = 1/10 chi = 1/30 m
    1 fathom ......... = 2 yard = 2*0.9144 m
    1 fathom ungary .. = 1.8964838 m (fixed)
    1 feet ........... = 12 inch = 0.9144/3 m
    1 furlong ........ = 10 chains = 220*0.9144 m
    1 gan ............ =
    1 hand ........... = 4 inches = 0.9144/9 m
    1 hunh ........... = 1/8 nieu = 9.144/3456 m
    1 inch ........... = yard/36 = 0.9144/36 m
    1 ja ............. =
    1 jeong .......... =
    1 kabiet ......... = 1/4 nieu = 9.144/1728 m
    1 ken ............ = 60/33 m
    1 keub ........... = 12 nieu = 10 inch = 9.144/36 m
    1 km ............. = 1000 m
    1 lar ............ =
    1 light year ..... = 9460730472580800 m
    1 link uk ........ = 0.01 chains = 0.22*0.9144 m
    1 micron ......... = 0.000001 m
    1 mile ........... = 1760 yards = 1609.344 m
    1 millimeter ..... = 1/1000 m
    1 nautical mile .. = 1852 m
    1 nieu ........... = 10/12 inch = 9.144/432 m
    1 parsec ......... = 30856800000000000 m
    1 pica ........... = yard/216 = 0.9144/216 m
    1 ri japan ....... = 129600/33 m
    1 ri korea ....... = 12960/33 m
    1 rod ............ = 5.0292 m
    1 sawk ........... = 2 keub = 20 inch = 9.144/18 m
    1 sen ............ = 20 va = 40.64 m
    1 shaku .......... = 10/33 m
    1 span ........... = 9 inches = 0.9144/4 m
    1 sun ............ = 1/33 m
    1 tsuen .......... =
    1 va ............. = 4 sawk = 80 inch = 2.032 m
    1 yard ........... = 0.9144 m
    1 yote ........... = 400 sen = 16256 m
    1 zhang .......... = 3+1/3 m = 10/3 m
*/
static const conv_t conv_LENGTH[] = {
    DECLARE_CONV_UNIT(LENGTH, ANGSTROMS,            "$/1X10",           "$*1X10")
    DECLARE_CONV_UNIT(LENGTH, ASTRONOMICAL_UNITS,   "$*149598000000",   "$/149598000000")
    DECLARE_CONV_UNIT(LENGTH, BARLEYCORNS,          "$*0.9144/108",     "$/0.9144*108")
    DECLARE_CONV_UNIT(LENGTH, CENTIMETERS,          "$/100",            "$*100")
    DECLARE_CONV_UNIT(LENGTH, CHAINS_UK,            "$*20.1168",        "$/20.1168")
    DECLARE_CONV_UNIT(LENGTH, CHI,                  "$/3",              "$*3")
    DECLARE_CONV_UNIT(LENGTH, CHOU,                 "$*3600/33",        "$*33/3600")
//    DECLARE_CONV_UNIT(LENGTH, CHR,                  "$", "$")
    DECLARE_CONV_UNIT(LENGTH, CUN,                  "$/30",             "$*30")
    DECLARE_CONV_UNIT(LENGTH, FATHOMS,              "$*1.8288",         "$/1.8288")
    DECLARE_CONV_UNIT(LENGTH, FATHOMS_HUNGARY,      "$*1.8964838",      "$/1.8964838")
    DECLARE_CONV_UNIT(LENGTH, FEET,                 "$*0.3048",         "$/0.3048")
    DECLARE_CONV_UNIT(LENGTH, FURLONGS,             "$*201.168",        "$/201.168")
//    DECLARE_CONV_UNIT(LENGTH, GAN,                  "$", "$")
    DECLARE_CONV_UNIT(LENGTH, HANDS,                "$*0,1016",         "$/0,1016")
    DECLARE_CONV_UNIT(LENGTH, HUNH,                 "$*9.144/3456",     "$/9.144*3456")
    DECLARE_CONV_UNIT(LENGTH, INCHES,               "$*0.0254",         "$/0.0254")
//    DECLARE_CONV_UNIT(LENGTH, JA,                   "$", "$")
//    DECLARE_CONV_UNIT(LENGTH, JEONG,                "$", "$")
    DECLARE_CONV_UNIT(LENGTH, KABIET,               "$*9.144/1728",     "$/9.144*1728")
    DECLARE_CONV_UNIT(LENGTH, KEN,                  "$*60/33",          "$*33/60")
    DECLARE_CONV_UNIT(LENGTH, KEUB,                 "$*9.144/36",       "$/9.144*36")
    DECLARE_CONV_UNIT(LENGTH, KILOMETERS,           "$*1000",           "$/1000")
//    DECLARE_CONV_UNIT(LENGTH, LAR,                  "$", "$")
    DECLARE_CONV_UNIT(LENGTH, LIGHT_YEARS,          "$*9460730472580800", "$/9460730472580800")
    DECLARE_CONV_UNIT(LENGTH, LINKS_UK,             "$*0.201168",       "$/0.201168")
    DECLARE_CONV_UNIT(LENGTH, METERS,               "$",                "$")
    DECLARE_CONV_UNIT(LENGTH, MICRONS,              "$*0.000001",       "$/0.000001")
    DECLARE_CONV_UNIT(LENGTH, MILES,                "$*1609.344",       "$/1609.344")
    DECLARE_CONV_UNIT(LENGTH, MILLIMETERS,          "$/1000",           "$*1000")
    DECLARE_CONV_UNIT(LENGTH, NAUTICAL_MILES,       "$*1852",           "$/1852")
    DECLARE_CONV_UNIT(LENGTH, NIEU,                 "$*9.144/432",      "$/9.144*432")
    DECLARE_CONV_UNIT(LENGTH, PARSECS,              "$*3.085678X16",    "$/3.085678X16")
    DECLARE_CONV_UNIT(LENGTH, PICAS,                "$*0.9144/216",     "$/0.9144*216")
    DECLARE_CONV_UNIT(LENGTH, RODS,                 "$*5.0292",         "$/5.0292")
    DECLARE_CONV_UNIT(LENGTH, RI_JAPAN,             "$*129600/33",      "$*33/129600")
    DECLARE_CONV_UNIT(LENGTH, RI_KOREA,             "$*12960/33",       "$*33/12960")
    DECLARE_CONV_UNIT(LENGTH, SAWK,                 "$*9.144/18",       "$/9.144*18")
    DECLARE_CONV_UNIT(LENGTH, SEN,                  "$*40.64",          "$/40.64")
    DECLARE_CONV_UNIT(LENGTH, SHAKU,                "$*10/33",          "$*33/10")
    DECLARE_CONV_UNIT(LENGTH, SPAN,                 "$*0.9144/4",       "$*4/0.9144")
    DECLARE_CONV_UNIT(LENGTH, SUN,                  "$*1/33",            "$*33")
//    DECLARE_CONV_UNIT(LENGTH, TSUEN,                "$", "$")
    DECLARE_CONV_UNIT(LENGTH, VA,                   "$*2.032",          "$/2.032")
    DECLARE_CONV_UNIT(LENGTH, YARDS,                "$*0.9144",         "$/0.9144")
    DECLARE_CONV_UNIT(LENGTH, YOTE,                 "$*16256",          "$/16256")
    DECLARE_CONV_UNIT(LENGTH, ZHANG,                "$/0.3",            "$*0.3")
    DECLARE_CONV_END
};

/*
    1 BTU/min = 17.5842642 W
    1 Foot-pound/minute = 0.02259696580552333 W
    1 hp = 745.69987158227022 W
    1 KW = 1000 W
    1 MW = 1000000 W
*/
static const conv_t conv_POWER[] = {
    DECLARE_CONV_UNIT(POWER, BTUS_PER_MINUTE, "$*17.5842642", "$/17.5842642")
    DECLARE_CONV_UNIT(POWER, FPS_PER_MINUTE,  "$*0.02259696580552333", "$/0.02259696580552333")
    DECLARE_CONV_UNIT(POWER, HORSEPOWER,      "$*745.69987158227022", "$/745.69987158227022")
    DECLARE_CONV_UNIT(POWER, KILOWATTS,       "$*1000", "$/1000")
    DECLARE_CONV_UNIT(POWER, MEGAWATTS,       "$*1000000", "$/1000000")
    DECLARE_CONV_UNIT(POWER, WATTS,           "$", "$")
    DECLARE_CONV_END
};

/*
    1 ATM   = 101325 Pa
    1 BAR   = 100000 Pa
    1 hPa   = 100 Pa
    1 kPa   = 1000 Pa
    1 mm HG = 133.322 Pa
    1 psi   = 6894.757 Pa
*/
static const conv_t conv_PRESSURE[] = {
    DECLARE_CONV_UNIT(PRESSURE, ATMOSPHERES,   "$*101325",   "$/101325")
    DECLARE_CONV_UNIT(PRESSURE, BARS,          "$*100000",   "$/100000")
    DECLARE_CONV_UNIT(PRESSURE, HECTOPASCALS,  "$*100",      "$/100")
    DECLARE_CONV_UNIT(PRESSURE, KILOPASCALS,   "$*1000",     "$/1000")
    DECLARE_CONV_UNIT(PRESSURE, MM_OF_MERCURY, "$*133.322",  "$/133.322")
    DECLARE_CONV_UNIT(PRESSURE, PASCALS,       "$",          "$")
    DECLARE_CONV_UNIT(PRESSURE, PSI,           "$*6894.757", "$/6894.757")
    DECLARE_CONV_END
};

/*
    1 day ....... = 86400 s
    1 hour ...... = 3600 s
    1 microsecond = 0.000001 s
    1 millisecond = 0.001 s
    1 nanosecond. = 0.000000001 s
    1 minute .... = 60 s
    1 week ...... = 669600 s
*/
static const conv_t conv_TIME[] = {
    DECLARE_CONV_UNIT(TIME, MINUTES,      "$*60",          "$/60")
    DECLARE_CONV_UNIT(TIME, DAYS,         "$*86400",       "$/86400")
    DECLARE_CONV_UNIT(TIME, HOURS,        "$*3600",        "$/3600")
    DECLARE_CONV_UNIT(TIME, MILLISECONDS, "$*0.001",       "$/0.001")
    DECLARE_CONV_UNIT(TIME, MICROSECONDS, "$*0.000001",    "$/0.000001")
    DECLARE_CONV_UNIT(TIME, NANOSECONDS,  "$*0.000000001", "$/0.000000001")
    DECLARE_CONV_UNIT(TIME, SECONDS,      "$",             "$")
    DECLARE_CONV_UNIT(TIME, WEEKS,        "$*604800",      "$/604800")
    DECLARE_CONV_UNIT(TIME, YEARS,        "$*31556952",    "$/31556952")
    DECLARE_CONV_END
};

/*
    C = K - 273.15
    F = K * 9/5 - 459.67
    R = K * 9/5
 */
static const conv_t conv_TEMPERATURE[] = {
    DECLARE_CONV_UNIT(TEMPERATURE, CELSIUS,     "$+273.15",       "$-273.15")
    DECLARE_CONV_UNIT(TEMPERATURE, FAHRENHEIT,  "($+459.67)*5/9", "$*9/5-459.67")
    DECLARE_CONV_UNIT(TEMPERATURE, KELVIN,      "$",              "$")
    DECLARE_CONV_UNIT(TEMPERATURE, RANKINE,     "$*5/9",          "$*9/5")
    DECLARE_CONV_END
};

/*
    1 cm/s = 0.01 m/s
    1 f/h  = 0.0000846666667 m/s
    1 f/s  = 0.3048 m/s
    1 Km/h = 10/36 m/s -> 0.27778 m/s
    1 knot = 18.52/36 m/s -> 0.51444444 m/s
    1 mach = 340.3 m/s
    1 mph  = 0.44704 m/s
*/
static const conv_t conv_VELOCITY[] = {
    DECLARE_CONV_UNIT(VELOCITY, CMS_SECOND,         "$*.01",      "$/.01")
    DECLARE_CONV_UNIT(VELOCITY, FEET_SECOND,        "$*.3048",    "$/.3048")
    DECLARE_CONV_UNIT(VELOCITY, FEET_HOUR,          "$*.0000846666667", "$/.0000846666667")
    DECLARE_CONV_UNIT(VELOCITY, KILOMETERS_HOUR,    "$*10/36",    "$*36/10")
    DECLARE_CONV_UNIT(VELOCITY, KNOTS,              "$*18.52/36", "$*36/18.52")
    DECLARE_CONV_UNIT(VELOCITY, MACH,               "$*340.3",    "$/340.3")
    DECLARE_CONV_UNIT(VELOCITY, METERS_SECOND,      "$",          "$")
    DECLARE_CONV_UNIT(VELOCITY, MILES_HOUR,         "$*.44704",   "$/.44704")
    DECLARE_CONV_END
};

/*
    1 barrel uk ...... = 163.65924 l
    1 barrel oil ..... = 158.987295 l
    1 bun ............ = 1000 l
    1 bushel uk ...... = 36.36872 l
    1 bushel us ...... = 35.23907017 l
    1 cubic cm  ...... = 0.001 l
    1 cubic feet ..... = 28.316846 l
    1 cubic inch ..... = 0.016387064 l
    1 cubic meter .... = 1000 l
    1 cubic yard ..... = 764.554857 l
    1 doe ............ =
    1 fluid ounce uk   = 0.0284130625 l
    1 fluid ounce us   = 0.0295735295625 l
    1 gallon uk ...... = 4.54609 l
    1 gallon dry us .. = 4.40488377086 l
    1 gallon liquid us = 3.785411784 l
    1 gou ............ = 0.1809 l
    1 hop ............ =
    1 icce ........... =
    1 kwian .......... = 2000 l
    1 mal ............ =
    1 milliliter ..... = 0.001 l
    1 pint uk ........ = 0.56826125 l
    1 pint dry us .... = 0.5506104713575 l
    1 pint liquid us   = 0.473176473 l
    1 quart uk ....... = 1.1365225 l
    1 quart dry us ... = 1.101220942715 l
    1 quart liquid us  = 0.946352946 l
    1 seki ........... =
    1 syou ........... =
    1 tananloung ..... = 1 l
    1 tang ........... = 20 l
    1 to ............. = 18040 l
*/
static const conv_t conv_VOLUME[] = {
    DECLARE_CONV_UNIT(VOLUME, BARRELS_UK,           "$*163.65924",       "$/163.65924")
    DECLARE_CONV_UNIT(VOLUME, BARRELS_OIL,          "$*158.987295",      "$/158.987295")
    DECLARE_CONV_UNIT(VOLUME, BUN,                  "$*1000",            "$/1000")
    DECLARE_CONV_UNIT(VOLUME, BUSHELS_UK,           "$*36.36872",        "$/36.36872")
    DECLARE_CONV_UNIT(VOLUME, BUSHELS_US,           "$*35.23907017",     "$/35.23907017")
    DECLARE_CONV_UNIT(VOLUME, CUBIC_CENTIMETERS,    "$*0.001",           "$/0.001")
    DECLARE_CONV_UNIT(VOLUME, CUBIC_FEET,           "$*28.316846",       "$/28.316846")
    DECLARE_CONV_UNIT(VOLUME, CUBIC_INCHES,         "$*0.016387064",     "$/0.016387064")
    DECLARE_CONV_UNIT(VOLUME, CUBIC_METERS,         "$*1000",            "$/1000")
    DECLARE_CONV_UNIT(VOLUME, CUBIC_YARDS,          "$*764.554857",      "$/764.554857")
//    DECLARE_CONV_UNIT(VOLUME, DOE,                  "$", "$")
    DECLARE_CONV_UNIT(VOLUME, FLUID_OUNCES_UK,      "$*0.0284130625",    "$/0.0284130625")
    DECLARE_CONV_UNIT(VOLUME, FLUID_OUNCES_US,      "$*0.0295735295625", "$/0.0295735295625")
    DECLARE_CONV_UNIT(VOLUME, GALLONS_UK,           "$*4.54609",         "$/4.54609")
    DECLARE_CONV_UNIT(VOLUME, GALLONS_DRY_US,       "$*4.40488377086",   "$/4.40488377086")
    DECLARE_CONV_UNIT(VOLUME, GALLONS_LIQUID_US,    "$*3.785411784",     "$/3.785411784")
    DECLARE_CONV_UNIT(VOLUME, GOU,                  "$*0.1809",          "$/0.1809")
//    DECLARE_CONV_UNIT(VOLUME, HOP,                  "$", "$")
//    DECLARE_CONV_UNIT(VOLUME, ICCE,                 "$", "$")
    DECLARE_CONV_UNIT(VOLUME, KWIAN,                "$*2000",            "$/2000")
    DECLARE_CONV_UNIT(VOLUME, LITERS,               "$",                 "$")
//    DECLARE_CONV_UNIT(VOLUME, MAL,                  "$", "$")
    DECLARE_CONV_UNIT(VOLUME, MILLILITERS,          "$*0.001",           "$/0.001")
    DECLARE_CONV_UNIT(VOLUME, PINTS_UK,             "$*0.56826125",      "$/0.56826125")
    DECLARE_CONV_UNIT(VOLUME, PINTS_DRY_US,         "$*0.5506104713575", "$/0.5506104713575")
    DECLARE_CONV_UNIT(VOLUME, PINTS_LIQUID_US,      "$*0.473176473",     "$/0.473176473")
    DECLARE_CONV_UNIT(VOLUME, QUARTS_UK,            "$*1.1365225",       "$/1.1365225")
    DECLARE_CONV_UNIT(VOLUME, QUARTS_DRY_US,        "$*1.101220942715",  "$/1.101220942715")
    DECLARE_CONV_UNIT(VOLUME, QUARTS_LIQUID_US,     "$*0.946352946",     "$/0.946352946")
//    DECLARE_CONV_UNIT(VOLUME, SEKI,                 "$", "$")
//    DECLARE_CONV_UNIT(VOLUME, SYOU,                 "$", "$")
    DECLARE_CONV_UNIT(VOLUME, TANANLOUNG,           "$",                 "$")
    DECLARE_CONV_UNIT(VOLUME, TANG,                 "$*20",              "$/20")
    DECLARE_CONV_UNIT(VOLUME, TO,                   "$*18040",           "$/18040")
    DECLARE_CONV_END
};

/*
    1 baht ............ = 12.244 g
    1 carat ........... = 0.2 g
    1 chung ........... =
    1 don ............. =
    1 geun ............ =
    1 gwan ............ =
    1 harb ............ =
    1 jin china ....... =
    1 jin taiwan ...... =
    1 Kan ............. = 3750 g
    1 Kilograms ....... = 1000 g
    1 Kin ............. = 600 g
    1 Liang China ..... =
    1 Liang Taiwan .... =
    1 monme ........... = 3.75 g
    1 ounce avoirdupois = 28.349523125 g
    1 ounce troy ...... = 31.1034768 g
    1 pound ........... = 453.59237 g
    1 quintal metric .. = 100000 g
    1 saloung ......... =
    1 stone ........... = 6350.29318 g
    1 tamlung ......... =
    1 ton ............. = 1000000 g
    1 ton uk .......... = 1016046.9088 g  // long ton
    1 ton us .......... = 907184.74 g     // short ton
*/
/*
1 chang = 20 tamlung
1 tamlung = 4 baht
1 baht = 4 saloung
1 saloung =

1 chang = 6/5 kg = 1200 g
1 tamlung = 1/20 chang = 60 g
1 baht = 1/4 tamlung = 15 g
1 saloung = 1/4 bath = 15/4 g
*/
static const conv_t conv_WEIGHT[] = {
    DECLARE_CONV_UNIT(WEIGHT, BAHT,                 "$*12.244",       "$/12.244")
    DECLARE_CONV_UNIT(WEIGHT, CARATS,               "$*0.2",          "$/0.2")
//    DECLARE_CONV_UNIT(WEIGHT, CHUNG,                "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, DON,                  "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, GEUN,                 "$", "$")
    DECLARE_CONV_UNIT(WEIGHT, GRAMS,                "$",              "$")
//    DECLARE_CONV_UNIT(WEIGHT, GWAN,                 "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, HARB,                 "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, JIN_CHINA,            "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, JIN_TAIWAN,           "$", "$")
    DECLARE_CONV_UNIT(WEIGHT, KAN,                  "$*3750",         "$/3750")
    DECLARE_CONV_UNIT(WEIGHT, KILOGRAMS,            "$*1000",         "$/1000")
    DECLARE_CONV_UNIT(WEIGHT, KIN,                  "$*600",          "$/600")
//    DECLARE_CONV_UNIT(WEIGHT, LIANG_CHINA,          "$", "$")
//    DECLARE_CONV_UNIT(WEIGHT, LIANG_TAIWAN,         "$", "$")
    DECLARE_CONV_UNIT(WEIGHT, MONME,                "$*3.75",         "$/3.75")
    DECLARE_CONV_UNIT(WEIGHT, OUNCES_AVOIRDUPOIS,   "$*28.349523125", "$/28.349523125")
    DECLARE_CONV_UNIT(WEIGHT, OUNCES_TROY,          "$*31.1034768",   "$/31.1034768")
    DECLARE_CONV_UNIT(WEIGHT, POUNDS,               "$*453.59237",    "$/453.59237")
    DECLARE_CONV_UNIT(WEIGHT, QUINTAL_METRIC,       "$*100000",       "$/100000")
//    DECLARE_CONV_UNIT(WEIGHT, SALOUNG,              "$", "$")
    DECLARE_CONV_UNIT(WEIGHT, STONES,               "$*6350.29318",    "$/6350.29318")
//    DECLARE_CONV_UNIT(WEIGHT, TAMLUNG,              "$", "$")
    DECLARE_CONV_UNIT(WEIGHT, TONNES,               "$*1000000",       "$/1000000")
    DECLARE_CONV_UNIT(WEIGHT, TONS_UK,              "$*1016046.9088",  "$/1016046.9088")
    DECLARE_CONV_UNIT(WEIGHT, TONS_US,              "$*907184.74",     "$/907184.74")
    DECLARE_CONV_END
};

static const conv_category_t conv_table[] = {
    DECLARE_CONV_CAT(ANGLE)
    DECLARE_CONV_CAT(AREA)
    DECLARE_CONV_CAT(CONSUMPTION)
    DECLARE_CONV_CAT(CURRENCY)
    DECLARE_CONV_CAT(ENERGY)
    DECLARE_CONV_CAT(LENGTH)
    DECLARE_CONV_CAT(POWER)
    DECLARE_CONV_CAT(PRESSURE)
    DECLARE_CONV_CAT(TIME)
    DECLARE_CONV_CAT(TEMPERATURE)
    DECLARE_CONV_CAT(VELOCITY)
    DECLARE_CONV_CAT(VOLUME)
    DECLARE_CONV_CAT(WEIGHT)
};

void ConvExecute(HWND hWnd)
{
    DWORD         c_cat = (DWORD)SendDlgItemMessage(hWnd, IDC_COMBO_CATEGORY, CB_GETCURSEL, 0, 0);
    const conv_t *items = NULL;
    DWORD         from  = SendDlgItemMessage(hWnd, IDC_COMBO_FROM, CB_GETCURSEL, 0, 0);
    DWORD         to    = SendDlgItemMessage(hWnd, IDC_COMBO_TO,   CB_GETCURSEL, 0, 0);
    TCHAR         txt_cb[128];
    TCHAR         txt[128];
    const conv_t *item;

    /* do nothing if the indexes point to the same unit */
    if (from == to)
        return;

    /* Search correct category, since it can be sorted too */
    SendDlgItemMessage(hWnd, IDC_COMBO_CATEGORY, CB_GETLBTEXT, c_cat, (LPARAM)txt_cb);
    for (c_cat=0; c_cat < SIZEOF(conv_table); c_cat++) {
        LoadString(calc.hInstance, conv_table[c_cat].category, txt, SIZEOF(txt));
        if (!_tcscmp(txt_cb, txt)) {
            items = conv_table[c_cat].items;
            break;
        }
    }

    /* The units can be sorted, so I must search the exact match */
    item = items;
    SendDlgItemMessage(hWnd, IDC_COMBO_FROM, CB_GETLBTEXT, from, (LPARAM)txt_cb);
    while (item->unit) {
        LoadString(calc.hInstance, item->unit, txt, SIZEOF(txt));
        if (!_tcscmp(txt_cb, txt)) {
            from = item-items;
            break;
        }
        item++;
    }
    SendDlgItemMessage(hWnd, IDC_COMBO_TO, CB_GETLBTEXT, to, (LPARAM)txt_cb);
    item = items;
    while (item->unit) {
        LoadString(calc.hInstance, item->unit, txt, SIZEOF(txt));
        if (!_tcscmp(txt_cb, txt)) {
            to = item-items;
            break;
        }
        item++;
    }

    calc.Convert[0].data = (char *)items[from].formula_from;
    calc.Convert[1].data = (char *)items[to].formula_to;
    calc.Convert[0].wm_msg = WM_HANDLE_FROM;
    calc.Convert[1].wm_msg = WM_HANDLE_TO;
    PostMessage(hWnd, WM_START_CONV, 0, MAKELPARAM(0, WM_HANDLE_FROM));
}

void ConvAdjust(HWND hWnd, int n_cat)
{
    TCHAR         txt[128];
    TCHAR         txt_cat[128];
    HWND          hFromWnd = GetDlgItem(hWnd, IDC_COMBO_FROM);
    HWND          hToWnd   = GetDlgItem(hWnd, IDC_COMBO_TO);
    const conv_t *item;
    unsigned int  n;

    SendDlgItemMessage(hWnd, IDC_COMBO_CATEGORY, CB_GETLBTEXT, n_cat, (LPARAM)txt_cat);
    for (n=0; n<SIZEOF(conv_table); n++) {
        item = conv_table[n].items;
        LoadString(calc.hInstance, conv_table[n].category, txt, SIZEOF(txt));
        if (!_tcscmp(txt_cat, txt))
            break;
    }

    SendMessage(hFromWnd, CB_RESETCONTENT, 0, 0);
    SendMessage(hToWnd,   CB_RESETCONTENT, 0, 0);
    while (item->unit) {
        LoadString(calc.hInstance, item->unit, txt, SIZEOF(txt));
        SendMessage(hFromWnd, CB_ADDSTRING, 0, (LPARAM)txt);
        SendMessage(hToWnd,   CB_ADDSTRING, 0, (LPARAM)txt);
        item++;
    }
    SendMessage(hFromWnd, CB_SETCURSEL, 0, 0);
    SendMessage(hToWnd,   CB_SETCURSEL, 0, 0);
}

void ConvInit(HWND hWnd)
{
    HWND         hCatWnd = GetDlgItem(hWnd, IDC_COMBO_CATEGORY);
    TCHAR        txt[128];
    unsigned int n;

    /* Fill category combo */
    for (n=0; n<SIZEOF(conv_table); n++) {
        LoadString(calc.hInstance, conv_table[n].category, txt, SIZEOF(txt));
        SendMessage(hCatWnd, CB_ADDSTRING, 0, (LPARAM)txt);
    }
    SendMessage(hCatWnd, CB_SETCURSEL, 0, 0);
    ConvAdjust(hWnd, 0);
}

