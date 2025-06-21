/*
 *    Text analyzer
 *
 * Copyright 2011 Aric Stewart for CodeWeavers
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <math.h>

#include "dwrite_private.h"
#include "scripts.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

extern const unsigned short wine_linebreak_table[];
extern const unsigned short wine_scripts_table[];
extern const unsigned short bidi_direction_table[];

/* Number of characters needed for LOCALE_SNATIVEDIGITS */
#define NATIVE_DIGITS_LEN 11

struct dwritescript_properties
{
    DWRITE_SCRIPT_PROPERTIES props;
    UINT32 scripttags[3]; /* Maximum 2 script tags, 0-terminated. */
    BOOL is_complex;
};

#define _OT(a,b,c,d) DWRITE_MAKE_OPENTYPE_TAG(a,b,c,d)

/* NOTE: keep this array synced with script ids from scripts.h */
static const struct dwritescript_properties dwritescripts_properties[Script_LastId+1] = {
    { /* Zzzz */ { 0x7a7a7a5a, 999, 15, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Zyyy */ { 0x7979795a, 998,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Zinh */ { 0x686e695a, 994, 15, 0x0020, 1, 0, 0, 0, 0, 0, 0 } },
    { /* Arab */ { 0x62617241, 160,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, { _OT('a','r','a','b') }, TRUE },
    { /* Armn */ { 0x6e6d7241, 230,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','r','m','n') } },
    { /* Avst */ { 0x74737641, 134,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','v','s','t') } },
    { /* Bali */ { 0x696c6142, 360, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('b','a','l','i') } },
    { /* Bamu */ { 0x756d6142, 435,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('b','a','m','u') } },
    { /* Batk */ { 0x6b746142, 365,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','a','t','k') } },
    { /* Beng */ { 0x676e6542, 325, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('b','n','g','2'), _OT('b','e','n','g') }, TRUE },
    { /* Bopo */ { 0x6f706f42, 285,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('b','o','p','o') } },
    { /* Brah */ { 0x68617242, 300,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','r','a','h') } },
    { /* Brai */ { 0x69617242, 570,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','r','a','i') }, TRUE },
    { /* Bugi */ { 0x69677542, 367,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','u','g','i') } },
    { /* Buhd */ { 0x64687542, 372,  8, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('b','u','h','d') } },
    { /* Cans */ { 0x736e6143, 440,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('c','a','n','s') }, TRUE },
    { /* Cari */ { 0x69726143, 201,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('c','a','r','i') } },
    { /* Cham */ { 0x6d616843, 358,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('c','h','a','m') } },
    { /* Cher */ { 0x72656843, 445,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('c','h','e','r') }, TRUE },
    { /* Copt */ { 0x74706f43, 204,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('c','o','p','t') } },
    { /* Xsux */ { 0x78757358,  20,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('x','s','u','x') } },
    { /* Cprt */ { 0x74727043, 403,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('c','p','r','t') } },
    { /* Cyrl */ { 0x6c727943, 220,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('c','y','r','l') } },
    { /* Dsrt */ { 0x74727344, 250,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('d','s','r','t') }, TRUE },
    { /* Deva */ { 0x61766544, 315, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('d','e','v','2'), _OT('d','e','v','a') }, TRUE },
    { /* Egyp */ { 0x70796745,  50,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('e','g','y','p') } },
    { /* Ethi */ { 0x69687445, 430,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('e','t','h','i') }, TRUE },
    { /* Geor */ { 0x726f6547, 240,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('g','e','o','r') } },
    { /* Glag */ { 0x67616c47, 225,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('g','l','a','g') } },
    { /* Goth */ { 0x68746f47, 206,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('g','o','t','h') } },
    { /* Grek */ { 0x6b657247, 200,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('g','r','e','k') } },
    { /* Gujr */ { 0x726a7547, 320, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('g','j','r','2'), _OT('g','u','j','r') }, TRUE },
    { /* Guru */ { 0x75727547, 310, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('g','u','r','2'), _OT('g','u','r','u') }, TRUE },
    { /* Hani */ { 0x696e6148, 500,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('h','a','n','i') } },
    { /* Hang */ { 0x676e6148, 286,  8, 0x0020, 1, 1, 1, 1, 0, 0, 0 }, { _OT('h','a','n','g') }, TRUE },
    { /* Hano */ { 0x6f6e6148, 371,  8, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('h','a','n','o') } },
    { /* Hebr */ { 0x72626548, 125,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('h','e','b','r') }, TRUE },
    { /* Hira */ { 0x61726948, 410,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('k','a','n','a') } },
    { /* Armi */ { 0x696d7241, 124,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','r','m','i') } },
    { /* Phli */ { 0x696c6850, 131,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('p','h','l','i') } },
    { /* Prti */ { 0x69747250, 130,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('p','r','t','i') } },
    { /* Java */ { 0x6176614a, 361, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('j','a','v','a') } },
    { /* Kthi */ { 0x6968744b, 317, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('k','t','h','i') } },
    { /* Knda */ { 0x61646e4b, 345, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('k','n','d','2'), _OT('k','n','d','a') }, TRUE },
    { /* Kana */ { 0x616e614b, 411,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('k','a','n','a') } },
    { /* Kali */ { 0x696c614b, 357,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('k','a','l','i') } },
    { /* Khar */ { 0x7261684b, 305, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('k','h','a','r') } },
    { /* Khmr */ { 0x726d684b, 355,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, { _OT('k','h','m','r') }, TRUE },
    { /* Laoo */ { 0x6f6f614c, 356,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, { _OT('l','a','o',' ') }, TRUE },
    { /* Latn */ { 0x6e74614c, 215,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('l','a','t','n') } },
    { /* Lepc */ { 0x6370654c, 335,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('l','e','p','c') } },
    { /* Limb */ { 0x626d694c, 336,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('l','i','m','b') } },
    { /* Linb */ { 0x626e694c, 401,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('l','i','n','b') } },
    { /* Lisu */ { 0x7573694c, 399,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('l','i','s','u') } },
    { /* Lyci */ { 0x6963794c, 202,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('l','y','c','i') } },
    { /* Lydi */ { 0x6964794c, 116,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('l','y','d','i') } },
    { /* Mlym */ { 0x6d796c4d, 347, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('m','l','m','2'), _OT('m','l','y','m') }, TRUE },
    { /* Mand */ { 0x646e614d, 140,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, { _OT('m','a','n','d') } },
    { /* Mtei */ { 0x6965744d, 337,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('m','t','e','i') } },
    { /* Mong */ { 0x676e6f4d, 145,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('m','o','n','g') }, TRUE },
    { /* Mymr */ { 0x726d794d, 350, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('m','y','m','r') }, TRUE },
    { /* Talu */ { 0x756c6154, 354,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','a','l','u') }, TRUE },
    { /* Nkoo */ { 0x6f6f6b4e, 165,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('n','k','o',' ') }, TRUE },
    { /* Ogam */ { 0x6d61674f, 212,  1, 0x1680, 0, 1, 0, 0, 0, 1, 0 }, { _OT('o','g','a','m') }, TRUE },
    { /* Olck */ { 0x6b636c4f, 261,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('o','l','c','k') } },
    { /* Ital */ { 0x6c617449, 210,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('i','t','a','l') } },
    { /* Xpeo */ { 0x6f657058,  30,  1, 0x0020, 0, 1, 1, 1, 0, 0, 0 }, { _OT('x','p','e','o') }, TRUE },
    { /* Sarb */ { 0x62726153, 105,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('s','a','r','b') } },
    { /* Orkh */ { 0x686b724f, 175,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('o','r','k','h') } },
    { /* Orya */ { 0x6179724f, 327, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('o','r','y','2'), _OT('o','r','y','a') }, TRUE },
    { /* Osma */ { 0x616d734f, 260,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('o','s','m','a') }, TRUE },
    { /* Phag */ { 0x67616850, 331,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('p','h','a','g') }, TRUE },
    { /* Phnx */ { 0x786e6850, 115,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('p','h','n','x') } },
    { /* Rjng */ { 0x676e6a52, 363,  8, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('r','j','n','g') } },
    { /* Runr */ { 0x726e7552, 211,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('r','u','n','r') }, TRUE },
    { /* Samr */ { 0x726d6153, 123,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('s','a','m','r') } },
    { /* Saur */ { 0x72756153, 344,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','a','u','r') } },
    { /* Shaw */ { 0x77616853, 281,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('s','h','a','w') } },
    { /* Sinh */ { 0x686e6953, 348,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','i','n','h') }, TRUE },
    { /* Sund */ { 0x646e7553, 362,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','u','n','d') } },
    { /* Sylo */ { 0x6f6c7953, 316,  8, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('s','y','l','o') } },
    { /* Syrc */ { 0x63727953, 135,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, { _OT('s','y','r','c') }, TRUE },
    { /* Tglg */ { 0x676c6754, 370,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('t','g','l','g') } },
    { /* Tagb */ { 0x62676154, 373,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('t','a','g','b') } },
    { /* Tale */ { 0x656c6154, 353,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('t','a','l','e') }, TRUE },
    { /* Lana */ { 0x616e614c, 351,  8, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('l','a','n','a') } },
    { /* Tavt */ { 0x74766154, 359,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, { _OT('t','a','v','t') } },
    { /* Taml */ { 0x6c6d6154, 346, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','m','l','2'), _OT('t','a','m','l') }, TRUE },
    { /* Telu */ { 0x756c6554, 340, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','e','l','2'), _OT('t','e','l','u') }, TRUE },
    { /* Thaa */ { 0x61616854, 170,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','h','a','a') }, TRUE },
    { /* Thai */ { 0x69616854, 352,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, { _OT('t','h','a','i') }, TRUE },
    { /* Tibt */ { 0x74626954, 330,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','i','b','t') }, TRUE },
    { /* Tfng */ { 0x676e6654, 120,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','f','n','g') }, TRUE },
    { /* Ugar */ { 0x72616755,  40,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('u','g','a','r') } },
    { /* Vaii */ { 0x69696156, 470,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('v','a','i',' ') }, TRUE },
    { /* Yiii */ { 0x69696959, 460,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('y','i',' ',' ') }, TRUE },
    { /* Cakm */ { 0x6d6b6143, 349,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('c','a','k','m') } },
    { /* Merc */ { 0x6372654d, 101,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','e','r','c') } },
    { /* Mero */ { 0x6f72654d, 100,  1, 0x0020, 0, 1, 1, 1, 0, 0, 0 }, { _OT('m','e','r','o') } },
    { /* Plrd */ { 0x64726c50, 282,  8, 0x0020, 1, 0, 1, 0, 0, 0, 0 }, { _OT('p','l','r','d') } },
    { /* Shrd */ { 0x64726853, 319,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','h','r','d') } },
    { /* Sora */ { 0x61726f53, 398,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','o','r','a') } },
    { /* Takr */ { 0x726b6154, 321,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('t','a','k','r') } },
    { /* Bass */ { 0x73736142, 259,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','a','s','s') } },
    { /* Aghb */ { 0x62686741, 239,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','g','h','b') } },
    { /* Dupl */ { 0x6c707544, 755,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('d','u','p','l') } },
    { /* Elba */ { 0x61626c45, 226,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('e','l','b','a') } },
    { /* Gran */ { 0x6e617247, 343, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('g','r','a','n') } },
    { /* Khoj */ { 0x6a6f684b, 322, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('k','h','o','j') } },
    { /* Sind */ { 0x646e6953, 318,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','i','n','d') } },
    { /* Lina */ { 0x616e694c, 400,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('l','i','n','a') } },
    { /* Mahj */ { 0x6a68614d, 314,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','a','h','j') } },
    { /* Mani */ { 0x696e614d, 139,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, { _OT('m','a','n','i') } },
    { /* Mend */ { 0x646e654d, 438,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','e','n','d') } },
    { /* Modi */ { 0x69646f4d, 324, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('m','o','d','i') } },
    { /* Mroo */ { 0x6f6f724d, 199,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','r','o','o') } },
    { /* Nbat */ { 0x7461624e, 159,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('n','b','a','t') } },
    { /* Narb */ { 0x6272614e, 106,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('n','a','r','b') } },
    { /* Perm */ { 0x6d726550, 227,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('p','e','r','m') } },
    { /* Hmng */ { 0x676e6d48, 450,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('h','m','n','g') } },
    { /* Palm */ { 0x6d6c6150, 126,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('p','a','l','m') } },
    { /* Pauc */ { 0x63756150, 263,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('p','a','u','c') } },
    { /* Phlp */ { 0x706c6850, 132,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, { _OT('p','h','l','p') } },
    { /* Sidd */ { 0x64646953, 302,  8, 0x0020, 1, 0, 1, 1, 0, 0, 0 }, { _OT('s','i','d','d') } },
    { /* Tirh */ { 0x68726954, 326, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('t','i','r','h') } },
    { /* Wara */ { 0x61726157, 262,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('w','a','r','a') } },
    { /* Adlm */ { 0x6d6c6441, 166,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','d','l','m') } },
    { /* Ahom */ { 0x6d6f6841, 338,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('a','h','o','m') } },
    { /* Hluw */ { 0x77756c48,  80,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('h','l','u','w') } },
    { /* Bhks */ { 0x736b6842, 334,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('b','h','k','s') } },
    { /* Hatr */ { 0x72746148, 127,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('h','a','t','r') } },
    { /* Marc */ { 0x6372614d, 332,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','a','r','c') } },
    { /* Mult */ { 0x746c754d, 323,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','u','l','t') } },
    { /* Newa */ { 0x6177654e, 333,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('n','e','w','a') } },
    { /* Hung */ { 0x676e7548, 176,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('h','u','n','g') } },
    { /* Osge */ { 0x6567734f, 219,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('o','s','g','e') } },
    { /* Sgnw */ { 0x776e6753,  95,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('s','g','n','w') } },
    { /* Tang */ { 0x676e6154, 520,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('t','a','n','g') } },
    { /* Gonm */ { 0x6d6e6f47, 313,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('g','o','n','m') } },
    { /* Nshu */ { 0x7568734e, 499,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, { _OT('n','s','h','u') } },
    { /* Soyo */ { 0x6f796f53, 329,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('s','o','y','o') } },
    { /* Zanb */ { 0x626e615a, 339,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('z','a','n','b') } },
    { /* Dogr */ { 0x72676f44, 328,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('d','o','g','r') } },
    { /* Gong */ { 0x676e6f47, 312,  8, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('g','o','n','g') } },
    { /* Rohg */ { 0x67686f52, 167,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('r','o','h','g') } },
    { /* Maka */ { 0x616b614d, 366,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','a','k','a') } },
    { /* Medf */ { 0x6664654d, 265,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('m','e','d','f') } },
    { /* Sogo */ { 0x6f676f53, 142,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, { _OT('s','o','g','o') } },
    { /* Sogd */ { 0x64676f53, 141,  8, 0x0020, 1, 1, 0, 0, 0, 1, 1 }, { _OT('s','o','g','d') } },
    { /* Elym */ { 0x6d796c45, 128,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, { _OT('e','l','y','m') } },
    { /* Hmnp */ { 0x706e6d48, 451,  8, 0x0020, 1, 1, 0, 0, 0, 0, 0 }, { _OT('h','m','n','p') } },
    { /* Nand */ { 0x646e614e, 311,  8, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, { _OT('n','a','n','d') } },
    { /* Wcho */ { 0x6f686357, 283,  8, 0x0020, 1, 1, 0, 0, 0, 0, 0 }, { _OT('w','c','h','o') } },
    { /* Chrs */ { 0x73726843, 109,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, { _OT('c','h','r','s') } },
    { /* Diak */ { 0x6b616944, 342,  8, 0x0020, 1, 1, 0, 0, 0, 0, 0 }, { _OT('d','i','a','k') } },
    { /* Kits */ { 0x7374694b, 288,  8, 0x0020, 1, 0, 1, 1, 0, 0, 0 }, { _OT('k','i','t','s') } },
    { /* Yezi */ { 0x697a6559, 192,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, { _OT('y','e','z','i') } },
};
#undef _OT

const char *debugstr_sa_script(UINT16 script)
{
    return script < Script_LastId ? debugstr_fourcc(dwritescripts_properties[script].props.isoScriptCode) : "undefined";
}

static const struct fallback_description
{
    const char *ranges;
    const WCHAR *families;
    const WCHAR *locale;
}
system_fallback_config[] =
{
    /* Latin, Combining Diacritical Marks */
    { "0000-007F, 0080-00FF, 0100-017F, 0180-024F, "
      "0250-02AF, 02B0-02FF, 0300-036F", L"Tahoma" },

    { "0530-058F, FB10-FB1C",   L"Noto Sans Armenian" },

    { "0590-05FF, FB1D-FB4F",   L"Noto Sans Hebrew" },

    { "0600-06FF, 0750-077F, "
      "08A0-08FF, FB50-FDCF, "
      "FDF0-FDFF, FE70-FEFE",   L"Noto Sans Arabic" },

    { "0700-074F",              L"Noto Sans Syriac" },
    { "0780-07BF",              L"Noto Sans Thaana" },
    { "07C0-07FF",              L"Noto Sans NKo" },
    { "0800-083F",              L"Noto Sans Samaritan" },
    { "0840-085F",              L"Noto Sans Mandaic" },

    { "0900-097F",              L"Noto Sans Devanagari" },
    { "0980-09FF",              L"Noto Sans Bengali" },
    { "0A00-0A7F",              L"Noto Sans Gurmukhi" },
    { "0A80-0AFF",              L"Noto Sans Gujarati" },
    { "0B00-0B7F",              L"Noto Sans Oriya" },
    { "0B80-0BFF",              L"Noto Sans Tamil" },
    { "0C00-0C7F",              L"Noto Sans Telugu" },
    { "0C80-0CFF",              L"Noto Sans Kannada" },
    { "0D00-0D7F",              L"Noto Sans Malayalam" },
    { "0D80-0DFF",              L"Noto Sans Sinhala" },

    { "0E00-0E7F",              L"Noto Sans Thai" },
    { "0E80-0EFF",              L"Noto Sans Lao" },

    { "0F00-0FFF",              L"Noto Serif Tibetan" },

    { "1000-109F, A9E0-A9FF, AA60-AA7F", L"Noto Sans Myanmar" },

    { "10A0-10FF, 2D00-2D2F",   L"Noto Sans Georgian" },

    /* Hangul Jamo               - 1100-11FF
       Hangul Compatibility Jamo - 3130-318F
       Enc. CJK (Paren Hangul)   - 3200-321F
       Enc. CJK (Circled Hangul) - 3260-327F
       Hangul Jamo Extended-A    - A960-A97F
       Hangul Syllables          - AC00-D7AF
       Hangul Jamo Extended-B    - D7B0-D7FF */

    { "1100-11FF, 3130-318F, "
      "3200-321F, 3260-327F, "
      "A960-A97F, AC00-D7FF, "
      "D7B0-D7FF",              L"Noto Sans CJK KR" },

    { "1680-169F",              L"Noto Sans Ogham" },

    { "16A0-16FF",              L"Noto Sans Runic" },

    { "1700-171F",              L"Noto Sans Tagalog" },
    { "1720-173F",              L"Noto Sans Hanunoo" },
    { "1740-175F",              L"Noto Sans Buhid" },
    { "1760-177F",              L"Noto Sans Tagbanwa" },

    { "1800-18AF, 202F, 11660-1167F", L"Noto Sans Mongolian" },

    { "1900-194F",              L"Noto Sans Limbu" },
    { "1950-197F",              L"Noto Sans Tai Le" },
    { "1980-19DF",              L"Noto Sans New Tai Lue" },
    { "1A00-1A1F",              L"Noto Sans Buginese" },
    { "1A20-1AAF",              L"Noto Sans Tai Tham" },
    { "1B00-1B7F",              L"Noto Sans Balinese" },
    { "1B80-1BBF, 1CC0-1CCF",   L"Noto Sans Sundanes" },
    { "1BC0-1BFF",              L"Noto Sans Batak" },
    { "1C00-1C4F",              L"Noto Sans Lepcha" },
    { "1C50-1C7F",              L"Noto Sans Ol Chiki" },

    { "2C80-2CFF",              L"Noto Sans Coptic" },
    { "2D30-2D7F",              L"Noto Sans Tifinagh" },

    /* CJK Radicals Supplement - 2E80-2EFF */

    { "2E80-2EFF",              L"Noto Sans CJK SC", L"zh-Hans" },
    { "2E80-2EFF",              L"Noto Sans CJK TC", L"zh-Hant" },
    { "2E80-2EFF",              L"Noto Sans CJK KR", L"ko" },

    /* CJK Symbols and Punctuation - 3000-303F
       Hiragana                    - 3040-309F
       Katakana                    - 30A0-30FF
       Katakana Phonetic Ext.      - 31F0-31FF */

    { "3000-30FF, 31F0-31FF",   L"Noto Sans CJK SC", L"zh-Hans" },
    { "3000-30FF, 31F0-31FF",   L"Noto Sans CJK TC", L"zh-Hant" },
    { "3000-30FF, 31F0-31FF",   L"Noto Sans CJK KR", L"ko" },
    { "3000-30FF, 31F0-31FF",   L"Noto Sans CJK JP" },

    /* CJK Unified Ext A - 3400-4DBF
       CJK Unified       - 4E00-9FFF */

    { "3400-4DBF, 4E00-9FFF",   L"Noto Sans CJK SC", L"zh-Hans" },
    { "3400-4DBF, 4E00-9FFF",   L"Noto Sans CJK TC", L"zh-Hant" },
    { "3400-4DBF, 4E00-9FFF",   L"Noto Sans CJK KR", L"ko" },
    { "3400-4DBF, 4E00-9FFF",   L"Noto Sans CJK JP" },

    { "A000-A4CF",              L"Noto Sans Yi" },
    { "A4D0-A4FF",              L"Noto Sans Lisu" },
    { "A500-A63F",              L"Noto Sans Vai" },
    { "A6A0-A6FF",              L"Noto Sans Bamum" },
    { "A800-A82F",              L"Noto Sans Syloti Nagri" },
    { "A840-A87F",              L"Noto Sans PhagsPa" },
    { "A880-A8DF",              L"Noto Sans Saurashtra" },
    { "A900-A92F",              L"Noto Sans Kayah Li" },
    { "A930-A95F",              L"Noto Sans Rejang" },
    { "A980-A9DF",              L"Noto Sans Javanese" },
    { "AA00-AA5F",              L"Noto Sans Cham" },

    /* CJK Compatibility Ideographs - F900-FAFF */

    { "F900-FAFF",              L"Noto Sans CJK SC", L"zh-Hans" },
    { "F900-FAFF",              L"Noto Sans CJK TC", L"zh-Hant" },
    { "F900-FAFF",              L"Noto Sans CJK KR", L"ko" },
    { "F900-FAFF",              L"Noto Sans CJK JP" },

    /* Vertical Forms - FE10-FE1F */

    { "FE10-FE1F",              L"Noto Sans CJK SC", L"zh-Hans" },
    { "FE10-FE1F",              L"Noto Sans CJK KR", L"ko" },
    { "FE10-FE1F",              L"Noto Sans CJK TC" },

    /* CJK Compatibility Forms - FE30-FE4F
       Small Form Variants     - FE50-FE6F */

    { "FE30-FE6F",              L"Noto Sans CJK SC", L"zh-Hans" },
    { "FE30-FE6F",              L"Noto Sans CJK KR", L"ko" },
    { "FE30-FE6F",              L"Noto Sans CJK JP", L"ja" },
    { "FE30-FE6F",              L"Noto Sans CJK TC" },

    /* Halfwidth and Fullwidth Forms */
    { "FF00-FFEF",              L"Noto Sans CJK SC", L"zh-Hans" },
    { "FF00-FFEF",              L"Noto Sans CJK TC", L"zh-Hant" },
    { "FF00-FFEF",              L"Noto Sans CJK KR", L"ko" },
    { "FF00-FFEF",              L"Noto Sans CJK JP" },
};

struct text_source_context
{
    IDWriteTextAnalysisSource *source;

    UINT32 position;
    UINT32 length;
    UINT32 consumed;

    UINT32 chunk_length;
    const WCHAR *text;
    UINT32 cursor;
    HRESULT status;
    BOOL end;

    UINT32 ch;
};

static inline unsigned int text_source_get_char_length(const struct text_source_context *context)
{
    return context->ch > 0xffff ? 2 : 1;
}

static void text_source_read_more(struct text_source_context *context)
{
    if ((context->chunk_length - context->cursor) > 1) return;

    context->position += context->cursor;
    context->cursor = 0;
    if (FAILED(context->status = IDWriteTextAnalysisSource_GetTextAtPosition(context->source, context->position,
            &context->text, &context->chunk_length)))
    {
        context->end = TRUE;
    }
}

static void text_source_get_u32_char(struct text_source_context *context)
{
    const WCHAR *text;
    UINT32 available;

    /* Make sure to have full pair of surrogates */
    text_source_read_more(context);

    available = context->chunk_length - context->cursor;
    text = context->text + context->cursor;

    if (available > 1 && IS_HIGH_SURROGATE(*text) && IS_LOW_SURROGATE(*(text + 1)))
    {
        context->cursor += 2;
        context->consumed += 2;
        context->ch = 0x10000 + ((*text - 0xd800) << 10) + *(text + 1) - 0xdc00;
        return;
    }

    context->cursor++;
    context->consumed++;
    context->ch = *text;
}

static HRESULT text_source_context_init(struct text_source_context *context, IDWriteTextAnalysisSource *source,
        UINT32 position, UINT32 length)
{
    memset(context, 0, sizeof(*context));
    context->source = source;
    context->position = position;
    context->length = length;

    return IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &context->text, &context->chunk_length);
}

static BOOL text_source_get_next_u32_char(struct text_source_context *context)
{
    if (context->consumed == context->length)
    {
        context->ch = 0;
        context->end = TRUE;
    }
    else if (context->cursor < context->chunk_length)
    {
        text_source_get_u32_char(context);
    }
    else
    {
        text_source_read_more(context);
        /* Normal end-of-text condition. */
        if (!context->text || !context->chunk_length)
            context->end = TRUE;
        else
            text_source_get_u32_char(context);
    }

    return context->end;
}

struct fallback_mapping
{
    DWRITE_UNICODE_RANGE *ranges;
    UINT32 ranges_count;
    WCHAR **families;
    UINT32 families_count;
    IDWriteFontCollection *collection;
    float scale;
};

struct fallback_locale
{
    struct list entry;
    WCHAR name[LOCALE_NAME_MAX_LENGTH];
    struct
    {
        size_t *data;
        size_t count;
        size_t size;
    } ranges;
};

static void fallback_locale_list_destroy(struct list *locales)
{
    struct fallback_locale *cur, *cur2;

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, locales, struct fallback_locale, entry)
    {
        list_remove(&cur->entry);
        free(cur->ranges.data);
        free(cur);
    }
}

static HRESULT fallback_locale_add_mapping(struct fallback_locale *locale, size_t index)
{
    size_t count = locale->ranges.count;

    /* Append to last range, or start a new one. */
    if (count && locale->ranges.data[count - 1] == (index - 1))
    {
        locale->ranges.data[count - 1] = index;
        return S_OK;
    }

    if (!dwrite_array_reserve((void **)&locale->ranges.data, &locale->ranges.size, count + 2,
            sizeof(*locale->ranges.data)))
    {
        return E_OUTOFMEMORY;
    }

    locale->ranges.data[count] = locale->ranges.data[count + 1] = index;
    locale->ranges.count += 2;

    return S_OK;
}

/* TODO: potentially needs improvement to consider partially matching locale names. */
static struct fallback_locale * font_fallback_get_locale(const struct list *locales,
        const WCHAR *locale_name)
{
    struct fallback_locale *locale, *neutral = NULL;

    LIST_FOR_EACH_ENTRY(locale, locales, struct fallback_locale, entry)
    {
        if (!wcsicmp(locale->name, locale_name)) return locale;
        if (!*locale->name) neutral = locale;
    }

    return neutral;
}

struct fallback_data
{
    struct fallback_mapping *mappings;
    size_t count;
    struct list locales;
};

struct dwrite_fontfallback
{
    IDWriteFontFallback1 IDWriteFontFallback1_iface;
    LONG refcount;
    IDWriteFactory7 *factory;
    IDWriteFontCollection *systemcollection;
    struct fallback_data data;
    size_t mappings_size;
};

struct dwrite_fontfallback_builder
{
    IDWriteFontFallbackBuilder IDWriteFontFallbackBuilder_iface;
    LONG refcount;
    IDWriteFactory7 *factory;
    struct fallback_data data;
    size_t mappings_size;
};

static struct fallback_data system_fallback =
{
    .locales = LIST_INIT(system_fallback.locales),
};

static void release_fallback_mapping(struct fallback_mapping *mapping)
{
    unsigned int i;

    free(mapping->ranges);
    for (i = 0; i < mapping->families_count; ++i)
        free(mapping->families[i]);
    free(mapping->families);
    if (mapping->collection)
        IDWriteFontCollection_Release(mapping->collection);
}

static void release_fallback_data(struct fallback_data *data)
{
    size_t i;

    for (i = 0; i < data->count; ++i)
        release_fallback_mapping(&data->mappings[i]);
    free(data->mappings);
    fallback_locale_list_destroy(&data->locales);
}

static BOOL fallback_mapping_contains_character(const struct fallback_mapping *mapping, UINT32 ch)
{
    size_t i;

    for (i = 0; i < mapping->ranges_count; ++i)
    {
        const DWRITE_UNICODE_RANGE *range = &mapping->ranges[i];
        if (range->first <= ch && range->last >= ch) return TRUE;
    }

    return FALSE;
}

static const struct fallback_mapping * find_fallback_mapping(const struct fallback_data *fallback,
        const struct fallback_locale *locale, UINT32 ch)
{
    const struct fallback_mapping *mapping;
    size_t i, j;

    for (i = 0; i < locale->ranges.count; i += 2)
    {
        size_t start = locale->ranges.data[i], end = locale->ranges.data[i + 1];
        for (j = start; j <= end; ++j)
        {
            mapping = &fallback->mappings[j];
            if (fallback_mapping_contains_character(mapping, ch)) return mapping;
        }
    }

    mapping = NULL;

    /* Mapping wasn't found for specific locale, try with neutral one. This will only recurse once. */
    if (*locale->name)
    {
        locale = font_fallback_get_locale(&fallback->locales, L"");
        mapping = find_fallback_mapping(fallback, locale, ch);
    }

    return mapping;
}

struct dwrite_numbersubstitution
{
    IDWriteNumberSubstitution IDWriteNumberSubstitution_iface;
    LONG refcount;

    DWRITE_NUMBER_SUBSTITUTION_METHOD method;
    WCHAR *locale;
    BOOL ignore_user_override;
};

static inline struct dwrite_numbersubstitution *impl_from_IDWriteNumberSubstitution(IDWriteNumberSubstitution *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_numbersubstitution, IDWriteNumberSubstitution_iface);
}

static struct dwrite_numbersubstitution *unsafe_impl_from_IDWriteNumberSubstitution(IDWriteNumberSubstitution *iface);

static inline struct dwrite_fontfallback *impl_from_IDWriteFontFallback1(IDWriteFontFallback1 *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfallback, IDWriteFontFallback1_iface);
}

static inline struct dwrite_fontfallback_builder *impl_from_IDWriteFontFallbackBuilder(IDWriteFontFallbackBuilder *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_fontfallback_builder, IDWriteFontFallbackBuilder_iface);
}

static inline UINT16 get_char_script(UINT32 c)
{
    UINT16 script = get_table_entry_32(wine_scripts_table, c);
    return script == Script_Inherited ? Script_Unknown : script;
}

static DWRITE_SCRIPT_ANALYSIS get_char_sa(UINT32 c)
{
    DWRITE_SCRIPT_ANALYSIS sa;

    sa.script = get_char_script(c);
    sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    if ((c <= 0x001f)                          /* C0 controls */
            || (c >= 0x007f && c <= 0x009f)    /* DELETE, C1 controls */
            || (c == 0x00ad)                   /* SOFT HYPHEN */
            || (c >= 0x200b && c <= 0x200f)    /* ZWSP, ZWNJ, ZWJ, LRM, RLM */
            || (c >= 0x2028 && c <= 0x202e)    /* Line/paragraph separators, LRE, RLE, PDF, LRO, RLO */
            || (c >= 0x2060 && c <= 0x2064)    /* WJ, invisible operators */
            || (c >= 0x2066 && c <= 0x2069)    /* LRI, RLI, FSI, PDI */
            || (c >= 0x206a && c <= 0x206f)    /* Deprecated control characters */
            || (c == 0xfeff)                   /* ZWBNSP */
            || (c == 0xfff9)                   /* Interlinear annotation */
            || (c == 0xfffa)
            || (c == 0xfffb)
            || (c >= 0x1bca0 && c <= 0x1bca3)  /* Shorthand format controls */
            || (c >= 0x1d173 && c <= 0x1d17a)  /* Musical symbols: beams and slurs */
            || (c == 0xe0001)                  /* Language tag, deprecated */
            || (c >= 0xe0020 && c <= 0xe007f)) /* Tag components */
    {
        sa.shapes = DWRITE_SCRIPT_SHAPES_NO_VISUAL;
    }
    else
    {
        sa.shapes = DWRITE_SCRIPT_SHAPES_DEFAULT;
    }

    return sa;
}

static HRESULT analyze_script(struct text_source_context *context, IDWriteTextAnalysisSink *sink)
{
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT32 pos, length;
    HRESULT hr;

    text_source_get_next_u32_char(context);

    sa = get_char_sa(context->ch);

    pos = context->position;
    length = text_source_get_char_length(context);

    while (!text_source_get_next_u32_char(context))
    {
        DWRITE_SCRIPT_ANALYSIS cur_sa = get_char_sa(context->ch);

        /* Unknown type is ignored when preceded or followed by another script */
        switch (sa.script) {
        case Script_Unknown:
            sa.script = cur_sa.script;
            break;
        case Script_Common:
            if (cur_sa.script == Script_Unknown)
                cur_sa.script = sa.script;
            else if ((cur_sa.script != Script_Common) && sa.shapes == DWRITE_SCRIPT_SHAPES_DEFAULT)
                sa.script = cur_sa.script;
            break;
        default:
            if ((cur_sa.script == Script_Common && cur_sa.shapes == DWRITE_SCRIPT_SHAPES_DEFAULT) || cur_sa.script == Script_Unknown)
                cur_sa.script = sa.script;
        }

        /* this is a length of a sequence to be reported next */
        if (sa.script == cur_sa.script && sa.shapes == cur_sa.shapes)
            length += text_source_get_char_length(context);
        else
        {
            hr = IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
            if (FAILED(hr)) return hr;
            pos += length;
            length = text_source_get_char_length(context);
            sa = cur_sa;
        }
    }

    /* one char length case or normal completion call */
    return IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
}

struct break_index
{
    unsigned int index;
    UINT8 length;
};

struct linebreaking_state
{
    DWRITE_LINE_BREAKPOINT *breakpoints;
    struct break_index *breaks;
    UINT32 count;
};

enum BreakConditionLocation {
    BreakConditionBefore,
    BreakConditionAfter
};

enum linebreaking_classes {
    b_BK = 1,
    b_CR,
    b_LF,
    b_CM,
    b_SG,
    b_GL,
    b_CB,
    b_SP,
    b_ZW,
    b_NL,
    b_WJ,
    b_JL,
    b_JV,
    b_JT,
    b_H2,
    b_H3,
    b_XX,
    b_OP,
    b_CL,
    b_CP,
    b_QU,
    b_NS,
    b_EX,
    b_SY,
    b_IS,
    b_PR,
    b_PO,
    b_NU,
    b_AL,
    b_ID,
    b_IN,
    b_HY,
    b_BB,
    b_BA,
    b_SA,
    b_AI,
    b_B2,
    b_HL,
    b_CJ,
    b_RI,
    b_EB,
    b_EM,
    b_ZWJ,
};

static BOOL has_strong_condition(DWRITE_BREAK_CONDITION old_condition, DWRITE_BREAK_CONDITION new_condition)
{
    if (old_condition == DWRITE_BREAK_CONDITION_MAY_NOT_BREAK || old_condition == DWRITE_BREAK_CONDITION_MUST_BREAK)
        return TRUE;

    if (old_condition == DWRITE_BREAK_CONDITION_CAN_BREAK && new_condition != DWRITE_BREAK_CONDITION_MUST_BREAK)
        return TRUE;

    return FALSE;
}

/* "Can break" is a weak condition, stronger "may not break" and "must break" override it. Initially all conditions are
    set to "can break" and could only be changed once. */
static inline void set_break_condition(UINT32 pos, enum BreakConditionLocation location, DWRITE_BREAK_CONDITION condition,
    struct linebreaking_state *state)
{
    unsigned int index = state->breaks[pos].index;

    if (location == BreakConditionBefore)
    {
        if (has_strong_condition(state->breakpoints[index].breakConditionBefore, condition))
            return;
        state->breakpoints[index].breakConditionBefore = condition;
        if (pos)
        {
            --pos;

            index = state->breaks[pos].index;
            if (state->breaks[pos].length > 1) index++;

            state->breakpoints[index].breakConditionAfter = condition;
        }
    }
    else
    {
        if (state->breaks[pos].length > 1) index++;

        if (has_strong_condition(state->breakpoints[index].breakConditionAfter, condition))
            return;
        state->breakpoints[index].breakConditionAfter = condition;

        if (pos + 1 < state->count)
        {
            index = state->breaks[pos + 1].index;
            state->breakpoints[index].breakConditionBefore = condition;
        }
    }
}

BOOL lb_is_newline_char(WCHAR ch)
{
    short c = get_table_entry_32(wine_linebreak_table, ch);
    return c == b_LF || c == b_NL || c == b_CR || c == b_BK;
}

static HRESULT analyze_linebreaks(IDWriteTextAnalysisSource *source, UINT32 position,
        UINT32 length, DWRITE_LINE_BREAKPOINT *breakpoints)
{
    struct text_source_context context;
    struct linebreaking_state state;
    struct break_index *breaks;
    unsigned int count, index;
    short *break_class;
    int i = 0, j;
    HRESULT hr;

    if (FAILED(hr = text_source_context_init(&context, source, position, length))) return hr;

    if (!(breaks = calloc(length, sizeof(*breaks))))
        return E_OUTOFMEMORY;

    if (!(break_class = calloc(length, sizeof(*break_class))))
    {
        free(breaks);
        return E_OUTOFMEMORY;
    }

    count = index = 0;
    while (!text_source_get_next_u32_char(&context))
    {
        break_class[count] = get_table_entry_32(wine_linebreak_table, context.ch);
        breaks[count].length = text_source_get_char_length(&context);
        breaks[count].index = index;

        /* LB1 - resolve some classes. TODO: use external algorithms for these classes. */
        switch (break_class[count])
        {
            case b_AI:
            case b_SA:
            case b_SG:
            case b_XX:
                break_class[count] = b_AL;
                break;
            case b_CJ:
                break_class[count] = b_NS;
                break;
        }

        breakpoints[index].breakConditionBefore = DWRITE_BREAK_CONDITION_NEUTRAL;
        breakpoints[index].breakConditionAfter  = DWRITE_BREAK_CONDITION_NEUTRAL;
        breakpoints[index].isWhitespace = context.ch < 0xffff ? !!iswspace(context.ch) : 0;
        breakpoints[index].isSoftHyphen = context.ch == 0x00ad /* Unicode Soft Hyphen */;
        breakpoints[index].padding = 0;
        ++index;

        if (breaks[count].length > 1)
        {
            breakpoints[index] = breakpoints[index - 1];
            /* Never break in surrogate pairs. */
            breakpoints[index - 1].breakConditionAfter = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
            breakpoints[index].breakConditionBefore = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
            ++index;
        }

        ++count;
    }

    state.breakpoints = breakpoints;
    state.breaks = breaks;
    state.count = count;

    /* LB2 - never break at the start */
    set_break_condition(0, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
    /* LB3 - always break at the end. */
    set_break_condition(count - 1, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);

    /* LB4 - LB6 - mandatory breaks. */
    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB4 - LB6 */
            case b_CR:
                /* LB5 - don't break CR x LF */
                if (i < count-1 && break_class[i+1] == b_LF)
                {
                    set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    break;
                }
            case b_LF:
            case b_NL:
            case b_BK:
                /* LB4 - LB5 - always break after hard breaks */
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MUST_BREAK, &state);
                /* LB6 - do not break before hard breaks */
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }
    }

    /* LB7 - LB8 - explicit breaks and non-breaks */
    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB7 - do not break before spaces or zero-width space */
            case b_SP:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_ZW:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

                /* LB8 - break before character after zero-width space, skip spaces in-between */
                j = i;
                while (j < count-1 && break_class[j+1] == b_SP)
                    j++;
                if (j < count-1 && break_class[j+1] != b_ZW)
                    set_break_condition(j, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB8a - do not break after ZWJ */
            case b_ZWJ:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }
    }

    /* LB9 - LB10 - combining marks */
    for (i = 0; i < count; i++)
    {
        if (break_class[i] == b_CM || break_class[i] == b_ZWJ)
        {
            if (i > 0)
            {
                switch (break_class[i-1])
                {
                    case b_SP:
                    case b_BK:
                    case b_CR:
                    case b_LF:
                    case b_NL:
                    case b_ZW:
                        break_class[i] = b_AL;
                        break;
                    default:
                    {
                        break_class[i] = break_class[i-1];
                        set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                }
            }
            else break_class[i] = b_AL;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB11 - don't break before and after word joiner */
            case b_WJ:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB12 - don't break after glue */
            case b_GL:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB12a */
                if (i > 0)
                {
                    if (break_class[i-1] != b_SP && break_class[i-1] != b_BA && break_class[i-1] != b_HY)
                        set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
                break;
            /* LB13 */
            case b_CL:
            case b_CP:
            case b_EX:
            case b_IS:
            case b_SY:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB14 - do not break after OP, even after spaces */
            case b_OP:
                j = i;
                while (j < count-1 && break_class[j+1] == b_SP)
                    j++;
                set_break_condition(j, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB15 - do not break within QU-OP, even with intervening spaces */
            case b_QU:
                j = i;
                while (j < count-1 && break_class[j+1] == b_SP)
                    j++;
                if (j < count - 1 && break_class[j+1] == b_OP)
                    set_break_condition(j, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB16 */
            case b_NS:
                j = i-1;
                while(j > 0 && break_class[j] == b_SP)
                    j--;
                if (break_class[j] == b_CL || break_class[j] == b_CP)
                    for (j++; j <= i; j++)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB17 - do not break within B2, even with intervening spaces */
            case b_B2:
                j = i;
                while (j < count && break_class[j+1] == b_SP)
                    j++;
                if (j < count - 1 && break_class[j+1] == b_B2)
                    set_break_condition(j, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB18 - break is allowed after space */
            case b_SP:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB19 - don't break before or after quotation mark */
            case b_QU:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB20 */
            case b_CB:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                if (i < count - 1 && break_class[i+1] != b_QU)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB21 */
            case b_BA:
            case b_HY:
            case b_NS:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_BB:
                if (i < count - 1 && break_class[i+1] != b_CB)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB21a, LB21b */
            case b_HL:
                /* LB21a */
                if (i < count-1)
                    switch (break_class[i+1])
                    {
                    case b_HY:
                    case b_BA:
                        set_break_condition(i+1, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                /* LB21b */
                if (i > 0 && break_class[i-1] == b_SY)
                    set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB22 */
            case b_IN:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }

        if (i < count-1)
        {
            /* LB23 - do not break between digits and letters */
            if ((break_class[i] == b_AL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_AL) ||
                (break_class[i] == b_NU && break_class[i+1] == b_HL))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB23a - do not break between numeric prefixes and ideographs, or between ideographs and numeric postfixes */
            if ((break_class[i] == b_PR && break_class[i+1] == b_ID) ||
                (break_class[i] == b_PR && break_class[i+1] == b_EB) ||
                (break_class[i] == b_PR && break_class[i+1] == b_EM) ||
                (break_class[i] == b_ID && break_class[i+1] == b_PO) ||
                (break_class[i] == b_EM && break_class[i+1] == b_PO) ||
                (break_class[i] == b_EB && break_class[i+1] == b_PO))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB24 - do not break between numeric prefix/postfix and letters, or letters and prefix/postfix */
            if ((break_class[i] == b_PR && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PR && break_class[i+1] == b_HL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_HL) ||
                (break_class[i] == b_AL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_HL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_AL && break_class[i+1] == b_PO) ||
                (break_class[i] == b_HL && break_class[i+1] == b_PO))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB25 */
            if ((break_class[i] == b_CL && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PR) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PO) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PR) ||
                (break_class[i] == b_PO && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PO && break_class[i+1] == b_NU) ||
                (break_class[i] == b_PR && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PR && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HY && break_class[i+1] == b_NU) ||
                (break_class[i] == b_IS && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_NU) ||
                (break_class[i] == b_SY && break_class[i+1] == b_NU))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB26 */
            if (break_class[i] == b_JL)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }
            if ((break_class[i] == b_JV || break_class[i] == b_H2) &&
                (break_class[i+1] == b_JV || break_class[i+1] == b_JT))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if ((break_class[i] == b_JT || break_class[i] == b_H3) &&
                 break_class[i+1] == b_JT)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB27 */
            switch (break_class[i])
            {
                case b_JL:
                case b_JV:
                case b_JT:
                case b_H2:
                case b_H3:
                    if (break_class[i+1] == b_IN || break_class[i+1] == b_PO)
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            }
            if (break_class[i] == b_PR)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_JT:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }

            /* LB28 */
            if ((break_class[i] == b_AL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_AL && break_class[i+1] == b_HL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB29 */
            if ((break_class[i] == b_IS && break_class[i+1] == b_AL) ||
                (break_class[i] == b_IS && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30 */
            if ((break_class[i] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU) &&
                 break_class[i+1] == b_OP)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if (break_class[i] == b_CP &&
               (break_class[i+1] == b_AL || break_class[i+1] == b_HL || break_class[i+1] == b_NU))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30a - break between two RIs if and only if there are an even number of RIs preceding position of the break */
            if (break_class[i] == b_RI && break_class[i+1] == b_RI) {
                unsigned int c = 0;

                j = i + 1;
                while (j > 0 && break_class[--j] == b_RI)
                    c++;

                if ((c & 1) == 0)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            }

            /* LB30b - do not break between an emoji base and an emoji modifier */
            if (break_class[i] == b_EB && break_class[i+1] == b_EM)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
        }
    }

    /* LB31 - allow breaks everywhere else. */
    for (i = 0; i < count; i++)
    {
        set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
    }

    free(break_class);
    free(breaks);

    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer_QueryInterface(IDWriteTextAnalyzer2 *iface, REFIID riid, void **obj)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextAnalyzer2) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer1) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dwritetextanalyzer_AddRef(IDWriteTextAnalyzer2 *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextanalyzer_Release(IDWriteTextAnalyzer2 *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeScript(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    struct text_source_context context;
    HRESULT hr;

    TRACE("%p, %u, %u, %p.\n", source, position, length, sink);

    if (!length)
        return S_OK;

    if (FAILED(hr = text_source_context_init(&context, source, position, length))) return hr;

    return analyze_script(&context, sink);
}

static inline unsigned int get_bidi_char_length(const struct bidi_char *c)
{
    return c->ch > 0xffff ? 2 : 1;
}

static inline UINT8 get_char_bidi_class(UINT32 ch)
{
    return get_table_entry_32(bidi_direction_table, ch);
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeBidi(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    struct text_source_context context;
    UINT8 baselevel, resolved, explicit;
    unsigned int i, chars_count = 0;
    struct bidi_char *chars, *ptr;
    UINT32 pos, seq_length;
    HRESULT hr;

    TRACE("%p, %u, %u, %p.\n", source, position, length, sink);

    if (!length)
        return S_OK;

    if (!(chars = calloc(length, sizeof(*chars))))
        return E_OUTOFMEMORY;

    ptr = chars;
    text_source_context_init(&context, source, position, length);
    while (!text_source_get_next_u32_char(&context))
    {
        ptr->ch = context.ch;
        ptr->nominal_bidi_class = ptr->bidi_class = get_char_bidi_class(context.ch);
        ptr++;

        ++chars_count;
    }

    /* Resolve levels using utf-32 codepoints, size differences are accounted for
       when levels are reported with SetBidiLevel(). */

    baselevel = IDWriteTextAnalysisSource_GetParagraphReadingDirection(source);
    hr = bidi_computelevels(chars, chars_count, baselevel);
    if (FAILED(hr))
        goto done;

    pos = position;
    resolved = chars->resolved;
    explicit = chars->explicit;
    seq_length = get_bidi_char_length(chars);

    for (i = 1, ptr = chars + 1; i < chars_count; ++i, ++ptr)
    {
        if (ptr->resolved == resolved && ptr->explicit == explicit)
        {
            seq_length += get_bidi_char_length(ptr);
        }
        else
        {
            hr = IDWriteTextAnalysisSink_SetBidiLevel(sink, pos, seq_length, explicit, resolved);
            if (FAILED(hr))
                goto done;

            pos += seq_length;
            seq_length = get_bidi_char_length(ptr);
            resolved = ptr->resolved;
            explicit = ptr->explicit;
        }
    }

    /* one char length case or normal completion call */
    hr = IDWriteTextAnalysisSink_SetBidiLevel(sink, pos, seq_length, explicit, resolved);

done:
    free(chars);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeNumberSubstitution(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    static int once;

    if (!once++)
        FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeLineBreakpoints(IDWriteTextAnalyzer2 *iface,
        IDWriteTextAnalysisSource *source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink *sink)
{
    DWRITE_LINE_BREAKPOINT *breakpoints;
    HRESULT hr;

    TRACE("%p, %u, %u, %p.\n", source, position, length, sink);

    if (!length)
        return S_OK;

    if (!(breakpoints = calloc(length, sizeof(*breakpoints))))
        return E_OUTOFMEMORY;

    if (SUCCEEDED(hr = analyze_linebreaks(source, position, length, breakpoints)))
        hr = IDWriteTextAnalysisSink_SetLineBreakpoints(sink, position, length, breakpoints);

    free(breakpoints);

    return hr;
}

static UINT32 get_opentype_language(const WCHAR *locale)
{
    UINT32 language = DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t');

    if (locale) {
        WCHAR tag[5];
        if (GetLocaleInfoEx(locale, LOCALE_SOPENTYPELANGUAGETAG, tag, ARRAY_SIZE(tag)))
            language = DWRITE_MAKE_OPENTYPE_TAG(tag[0],tag[1],tag[2],tag[3]);
    }

    return language;
}

static void get_number_substitutes(IDWriteNumberSubstitution *substitution, BOOL is_rtl, WCHAR *digits)
{
    struct dwrite_numbersubstitution *numbersubst = unsafe_impl_from_IDWriteNumberSubstitution(substitution);
    DWRITE_NUMBER_SUBSTITUTION_METHOD method;
    WCHAR isolang[9];
    DWORD lctype;

    digits[0] = 0;

    if (!numbersubst)
        return;

    lctype = numbersubst->ignore_user_override ? LOCALE_NOUSEROVERRIDE : 0;

    if (numbersubst->method == DWRITE_NUMBER_SUBSTITUTION_METHOD_FROM_CULTURE) {
        DWORD value;

        method = DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE;
        if (GetLocaleInfoEx(numbersubst->locale, lctype | LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER, (WCHAR *)&value, 2)) {
            switch (value)
            {
            case 0:
                method = DWRITE_NUMBER_SUBSTITUTION_METHOD_CONTEXTUAL;
                break;
            case 2:
                method = DWRITE_NUMBER_SUBSTITUTION_METHOD_NATIONAL;
                break;
            case 1:
            default:
                if (value != 1)
                    WARN("Unknown IDIGITSUBSTITUTION value %lu, locale %s.\n", value, debugstr_w(numbersubst->locale));
            }
        }
        else
            WARN("Failed to get IDIGITSUBSTITUTION for locale %s\n", debugstr_w(numbersubst->locale));
    }
    else
        method = numbersubst->method;

    switch (method)
    {
    case DWRITE_NUMBER_SUBSTITUTION_METHOD_NATIONAL:
        GetLocaleInfoEx(numbersubst->locale, lctype | LOCALE_SNATIVEDIGITS, digits, NATIVE_DIGITS_LEN);
        break;
    case DWRITE_NUMBER_SUBSTITUTION_METHOD_CONTEXTUAL:
    case DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL:
        if (GetLocaleInfoEx(numbersubst->locale, LOCALE_SISO639LANGNAME, isolang, ARRAY_SIZE(isolang)))
        {
             static const WCHAR arabicW[] = {0x640,0x641,0x642,0x643,0x644,0x645,0x646,0x647,0x648,0x649,0};

             /* For some Arabic locales Latin digits are returned for SNATIVEDIGITS */
             if (!wcscmp(L"ar", isolang))
             {
                 wcscpy(digits, arabicW);
                 break;
             }
        }
        GetLocaleInfoEx(numbersubst->locale, lctype | LOCALE_SNATIVEDIGITS, digits, NATIVE_DIGITS_LEN);
        break;
    default:
        ;
    }

    if (method != DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE && !*digits) {
        WARN("Failed to get number substitutes for locale %s, method %d\n", debugstr_w(numbersubst->locale), method);
        method = DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE;
    }

    if ((method == DWRITE_NUMBER_SUBSTITUTION_METHOD_CONTEXTUAL && !is_rtl) || method == DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE)
        digits[0] = 0;
}

static void analyzer_dump_user_features(DWRITE_TYPOGRAPHIC_FEATURES const **features,
        UINT32 const *feature_range_lengths, UINT32 feature_ranges)
{
    UINT32 i, j, start;

    if (!TRACE_ON(dwrite) || !features)
        return;

    for (i = 0, start = 0; i < feature_ranges; start += feature_range_lengths[i++]) {
        TRACE("feature range [%u,%u)\n", start, start + feature_range_lengths[i]);
        for (j = 0; j < features[i]->featureCount; j++)
            TRACE("feature %s, parameter %u\n", debugstr_fourcc(features[i]->features[j].nameTag),
                    features[i]->features[j].parameter);
    }
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphs(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT32 length, IDWriteFontFace* fontface, BOOL is_sideways,
    BOOL is_rtl, DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale,
    IDWriteNumberSubstitution* substitution, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_lengths, UINT32 feature_ranges, UINT32 max_glyph_count,
    UINT16* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* text_props, UINT16 *glyphs,
    DWRITE_SHAPING_GLYPH_PROPERTIES* glyph_props, UINT32* actual_glyph_count)
{
    const struct dwritescript_properties *scriptprops;
    struct scriptshaping_context context = { 0 };
    struct dwrite_fontface *font_obj;
    WCHAR digits[NATIVE_DIGITS_LEN];
    unsigned int glyph_count;
    HRESULT hr;

    TRACE("%s:%u, %p, %d, %d, %s, %s, %p, %p, %p, %u, %u, %p, %p, %p, %p, %p.\n", debugstr_wn(text, length),
        length, fontface, is_sideways, is_rtl, debugstr_sa_script(analysis->script), debugstr_w(locale), substitution,
        features, feature_range_lengths, feature_ranges, max_glyph_count, clustermap, text_props, glyphs,
        glyph_props, actual_glyph_count);

    analyzer_dump_user_features(features, feature_range_lengths, feature_ranges);

    get_number_substitutes(substitution, is_rtl, digits);
    font_obj = unsafe_impl_from_IDWriteFontFace(fontface);
    glyph_count = max(max_glyph_count, length);

    context.cache = fontface_get_shaping_cache(font_obj);
    context.script = analysis->script > Script_LastId ? Script_Unknown : analysis->script;
    context.text = text;
    context.length = length;
    context.is_rtl = is_rtl;
    context.is_sideways = is_sideways;
    context.u.subst.glyphs = calloc(glyph_count, sizeof(*glyphs));
    context.u.subst.glyph_props = calloc(glyph_count, sizeof(*glyph_props));
    context.u.subst.text_props = text_props;
    context.u.subst.clustermap = clustermap;
    context.u.subst.max_glyph_count = max_glyph_count;
    context.u.subst.capacity = glyph_count;
    context.u.subst.digits = digits;
    context.language_tag = get_opentype_language(locale);
    context.user_features.features = features;
    context.user_features.range_lengths = feature_range_lengths;
    context.user_features.range_count = feature_ranges;
    context.glyph_infos = calloc(glyph_count, sizeof(*context.glyph_infos));
    context.table = &context.cache->gsub;

    *actual_glyph_count = 0;

    if (!context.u.subst.glyphs || !context.u.subst.glyph_props || !context.glyph_infos)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    scriptprops = &dwritescripts_properties[context.script];
    hr = shape_get_glyphs(&context, scriptprops->scripttags);
    if (SUCCEEDED(hr))
    {
        *actual_glyph_count = context.glyph_count;
        memcpy(glyphs, context.u.subst.glyphs, context.glyph_count * sizeof(*glyphs));
        memcpy(glyph_props, context.u.subst.glyph_props, context.glyph_count * sizeof(*glyph_props));
    }

failed:
    free(context.u.subst.glyph_props);
    free(context.u.subst.glyphs);
    free(context.glyph_infos);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES *text_props,
    UINT32 text_len, UINT16 const* glyphs, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace *fontface, float emSize, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_lengths, UINT32 feature_ranges, float *advances, DWRITE_GLYPH_OFFSET *offsets)
{
    const struct dwritescript_properties *scriptprops;
    struct scriptshaping_context context = { 0 };
    struct dwrite_fontface *font_obj;
    unsigned int i;
    HRESULT hr;

    TRACE("%s, %p, %p, %u, %p, %p, %u, %p, %.2f, %d, %d, %s, %s, %p, %p, %u, %p, %p.\n", debugstr_wn(text, text_len),
        clustermap, text_props, text_len, glyphs, glyph_props, glyph_count, fontface, emSize, is_sideways,
        is_rtl, debugstr_sa_script(analysis->script), debugstr_w(locale), features, feature_range_lengths,
        feature_ranges, advances, offsets);

    analyzer_dump_user_features(features, feature_range_lengths, feature_ranges);

    if (glyph_count == 0)
        return S_OK;

    font_obj = unsafe_impl_from_IDWriteFontFace(fontface);

    for (i = 0; i < glyph_count; ++i)
    {
        if (glyph_props[i].isZeroWidthSpace)
            advances[i] = 0.0f;
        else
            advances[i] = fontface_get_scaled_design_advance(font_obj, DWRITE_MEASURING_MODE_NATURAL, emSize, 1.0f,
                    NULL, glyphs[i], is_sideways);
        offsets[i].advanceOffset = 0.0f;
        offsets[i].ascenderOffset = 0.0f;
    }

    context.cache = fontface_get_shaping_cache(font_obj);
    context.script = analysis->script > Script_LastId ? Script_Unknown : analysis->script;
    context.text = text;
    context.length = text_len;
    context.is_rtl = is_rtl;
    context.is_sideways = is_sideways;
    context.u.pos.glyphs = glyphs;
    context.u.pos.glyph_props = glyph_props;
    context.u.pos.text_props = text_props;
    context.u.pos.clustermap = clustermap;
    context.glyph_count = glyph_count;
    context.emsize = emSize;
    context.measuring_mode = DWRITE_MEASURING_MODE_NATURAL;
    context.advances = advances;
    context.offsets = offsets;
    context.language_tag = get_opentype_language(locale);
    context.user_features.features = features;
    context.user_features.range_lengths = feature_range_lengths;
    context.user_features.range_count = feature_ranges;
    context.glyph_infos = calloc(glyph_count, sizeof(*context.glyph_infos));
    context.table = &context.cache->gpos;

    if (!context.glyph_infos)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    scriptprops = &dwritescripts_properties[context.script];
    hr = shape_get_positions(&context, scriptprops->scripttags);

failed:
    free(context.glyph_infos);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_GetGdiCompatibleGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES *text_props,
    UINT32 text_len, UINT16 const* glyphs, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace *fontface, float emSize, float ppdip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_lengths, UINT32 feature_ranges, float *advances, DWRITE_GLYPH_OFFSET *offsets)
{
    const struct dwritescript_properties *scriptprops;
    struct scriptshaping_context context = { 0 };
    DWRITE_MEASURING_MODE measuring_mode;
    struct dwrite_fontface *font_obj;
    unsigned int i;
    HRESULT hr;

    TRACE("%s, %p, %p, %u, %p, %p, %u, %p, %.2f, %.2f, %p, %d, %d, %d, %s, %s, %p, %p, %u, %p, %p.\n",
            debugstr_wn(text, text_len), clustermap, text_props, text_len, glyphs, glyph_props, glyph_count, fontface,
            emSize, ppdip, transform, use_gdi_natural, is_sideways, is_rtl, debugstr_sa_script(analysis->script),
            debugstr_w(locale), features, feature_range_lengths, feature_ranges, advances, offsets);

    analyzer_dump_user_features(features, feature_range_lengths, feature_ranges);

    if (glyph_count == 0)
        return S_OK;

    font_obj = unsafe_impl_from_IDWriteFontFace(fontface);

    measuring_mode = use_gdi_natural ? DWRITE_MEASURING_MODE_GDI_NATURAL : DWRITE_MEASURING_MODE_GDI_CLASSIC;

    for (i = 0; i < glyph_count; ++i)
    {
        if (glyph_props[i].isZeroWidthSpace)
            advances[i] = 0.0f;
        else
            advances[i] = fontface_get_scaled_design_advance(font_obj, measuring_mode, emSize, ppdip,
                    transform, glyphs[i], is_sideways);
        offsets[i].advanceOffset = 0.0f;
        offsets[i].ascenderOffset = 0.0f;
    }

    context.cache = fontface_get_shaping_cache(font_obj);
    context.script = analysis->script > Script_LastId ? Script_Unknown : analysis->script;
    context.text = text;
    context.length = text_len;
    context.is_rtl = is_rtl;
    context.is_sideways = is_sideways;
    context.u.pos.glyphs = glyphs;
    context.u.pos.glyph_props = glyph_props;
    context.u.pos.text_props = text_props;
    context.u.pos.clustermap = clustermap;
    context.glyph_count = glyph_count;
    context.emsize = emSize * ppdip;
    context.measuring_mode = measuring_mode;
    context.advances = advances;
    context.offsets = offsets;
    context.language_tag = get_opentype_language(locale);
    context.user_features.features = features;
    context.user_features.range_lengths = feature_range_lengths;
    context.user_features.range_count = feature_ranges;
    context.glyph_infos = calloc(glyph_count, sizeof(*context.glyph_infos));
    context.table = &context.cache->gpos;

    if (!context.glyph_infos)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }

    scriptprops = &dwritescripts_properties[context.script];
    hr = shape_get_positions(&context, scriptprops->scripttags);

failed:
    free(context.glyph_infos);

    return hr;
}

static HRESULT apply_cluster_spacing(float leading_spacing, float trailing_spacing, float min_advance_width,
        unsigned int start, unsigned int end, float const *advances, DWRITE_GLYPH_OFFSET const *offsets,
        DWRITE_SHAPING_GLYPH_PROPERTIES const *glyph_props, float *modified_advances,
        DWRITE_GLYPH_OFFSET *modified_offsets)
{
    BOOL reduced = leading_spacing < 0.0f || trailing_spacing < 0.0f;
    unsigned int first_spacing, last_spacing, i;
    float advance, origin = 0.0f, *deltas;
    BOOL is_spacing_cluster = FALSE;

    if (modified_advances != advances)
        memcpy(&modified_advances[start], &advances[start], (end - start + 1) * sizeof(*advances));
    if (modified_offsets != offsets)
        memcpy(&modified_offsets[start], &offsets[start], (end - start + 1) * sizeof(*offsets));

    for (first_spacing = start; first_spacing <= end; ++first_spacing)
    {
        if ((is_spacing_cluster = !glyph_props[first_spacing].isZeroWidthSpace))
            break;
    }

    /* Nothing to adjust if there is no spacing glyphs. */
    if (!is_spacing_cluster)
        return S_OK;

    for (last_spacing = end; last_spacing >= start; --last_spacing)
    {
        if (!glyph_props[last_spacing].isZeroWidthSpace)
            break;
    }

    if (!(deltas = calloc(end - start + 1, sizeof(*deltas))))
        return E_OUTOFMEMORY;

    /* Cluster advance, note that properties are ignored. */
    origin = offsets[start].advanceOffset;
    for (i = start, advance = 0.0f; i <= end; ++i)
    {
        float cur = advance + offsets[i].advanceOffset;

        deltas[i - start] = cur - origin;

        advance += advances[i];
        origin = cur;
    }

    /* Negative spacing. */
    if (leading_spacing < 0.0f)
    {
        advance += leading_spacing;
        modified_advances[first_spacing] += leading_spacing;
        modified_offsets[first_spacing].advanceOffset += leading_spacing;
    }

    if (trailing_spacing < 0.0f)
    {
        advance += trailing_spacing;
        modified_advances[last_spacing] += trailing_spacing;
    }

    /* Minimal advance. */
    advance = min_advance_width - advance;
    if (advance > 0.0f) {
        /* Additional spacing is only applied to leading and trailing spacing glyphs. */
        float half = advance / 2.0f;

        if (!reduced)
        {
            modified_advances[first_spacing] += half;
            modified_advances[last_spacing] += half;
            modified_offsets[first_spacing].advanceOffset += half;
        }
        else if (leading_spacing < 0.0f && trailing_spacing < 0.0f)
        {
            modified_advances[first_spacing] += half;
            modified_advances[last_spacing] += half;
            modified_offsets[first_spacing].advanceOffset += half;
        }
        else if (leading_spacing < 0.0f)
        {
            modified_advances[first_spacing] += advance;
            modified_offsets[first_spacing].advanceOffset += advance;
        }
        else
            modified_advances[last_spacing] += advance;
    }

    /* Positive spacing. */
    if (leading_spacing > 0.0f)
    {
        modified_advances[first_spacing] += leading_spacing;
        modified_offsets[first_spacing].advanceOffset += leading_spacing;
    }

    if (trailing_spacing > 0.0f)
        modified_advances[last_spacing] += trailing_spacing;

    /* Update offsets to preserve original relative positions within cluster. */
    for (i = first_spacing; i > start; --i)
    {
        unsigned int cur = i - 1;
        modified_offsets[cur].advanceOffset = modified_advances[cur] + modified_offsets[i].advanceOffset -
                deltas[i - start];
    }

    for (i = first_spacing + 1; i <= end; ++i)
    {
        modified_offsets[i].advanceOffset = deltas[i - start] + modified_offsets[i - 1].advanceOffset -
                modified_advances[i - 1];
    }

    free(deltas);

    return S_OK;
}

static inline UINT32 get_cluster_length(UINT16 const *clustermap, UINT32 start, UINT32 text_len)
{
    UINT16 g = clustermap[start];
    UINT32 length = 1;

    while (start < (text_len - 1) && clustermap[++start] == g)
        length++;
    return length;
}

/* Applies spacing adjustments to clusters.

   Adjustments are applied in the following order:

   1. Negative adjustments

      Leading and trailing spacing could be negative, at this step
      only negative ones are actually applied. Leading spacing is only
      applied to leading glyph, trailing - to trailing glyph.

   2. Minimum advance width

      Advances could only be reduced at this point or unchanged. In any
      case it's checked if cluster advance width is less than minimum width.
      If it's the case advance width is incremented up to minimum value.

      Important part is the direction in which this increment is applied;
      it depends on direction from which total cluster advance was trimmed
      at step 1. So it could be incremented from leading, trailing, or both
      sides. When applied to both sides, each side gets half of difference
      that brings advance to minimum width.

   3. Positive adjustments

      After minimum width rule was applied, positive spacing is applied in the same
      way as negative one on step 1.

   Glyph offset for leading glyph is adjusted too in a way that glyph origin
   keeps its position in coordinate system where initial advance width is counted
   from 0.

   Glyph properties

   It's known that isZeroWidthSpace property keeps initial advance from changing.

*/
static HRESULT WINAPI dwritetextanalyzer1_ApplyCharacterSpacing(IDWriteTextAnalyzer2 *iface,
    FLOAT leading_spacing, FLOAT trailing_spacing, FLOAT min_advance_width, UINT32 len,
    UINT32 glyph_count, UINT16 const *clustermap, FLOAT const *advances, DWRITE_GLYPH_OFFSET const *offsets,
    DWRITE_SHAPING_GLYPH_PROPERTIES const *props, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    unsigned int i;

    TRACE("%.2f, %.2f, %.2f, %u, %u, %p, %p, %p, %p, %p, %p.\n", leading_spacing, trailing_spacing, min_advance_width,
        len, glyph_count, clustermap, advances, offsets, props, modified_advances, modified_offsets);

    if (min_advance_width < 0.0f) {
        if (modified_advances != advances)
            memset(modified_advances, 0, glyph_count*sizeof(*modified_advances));
        return E_INVALIDARG;
    }

    for (i = 0; i < len;)
    {
        unsigned int length = get_cluster_length(clustermap, i, len);
        unsigned int start, end;

        start = clustermap[i];
        end = i + length < len ? clustermap[i + length] : glyph_count;

        apply_cluster_spacing(leading_spacing, trailing_spacing, min_advance_width, start, end - 1, advances,
                offsets, props, modified_advances, modified_offsets);

        i += length;
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer1_GetBaseline(IDWriteTextAnalyzer2 *iface, IDWriteFontFace *fontface,
    DWRITE_BASELINE baseline, BOOL vertical, BOOL is_simulation_allowed, DWRITE_SCRIPT_ANALYSIS sa,
    const WCHAR *localeName, INT32 *baseline_coord, BOOL *exists)
{
    struct dwrite_fontface *font_obj;
    const DWRITE_FONT_METRICS1 *metrics;

    TRACE("%p, %d, %d, %u, %s, %p, %p.\n", fontface, vertical, is_simulation_allowed, sa.script, debugstr_w(localeName),
            baseline_coord, exists);

    *exists = FALSE;
    *baseline_coord = 0;

    if (baseline == DWRITE_BASELINE_DEFAULT)
        baseline = vertical ? DWRITE_BASELINE_CENTRAL : DWRITE_BASELINE_ROMAN;

    if ((unsigned int)baseline > DWRITE_BASELINE_MAXIMUM)
        return E_INVALIDARG;

    /* TODO: fetch BASE table data if available. */

    if (!*exists && is_simulation_allowed)
    {
        font_obj = unsafe_impl_from_IDWriteFontFace(fontface);
        metrics = &font_obj->metrics;

        switch (baseline)
        {
            case DWRITE_BASELINE_ROMAN:
                *baseline_coord = vertical ? metrics->descent : 0;
                break;
            case DWRITE_BASELINE_CENTRAL:
                *baseline_coord = vertical ? (metrics->ascent + metrics->descent) / 2 :
                        -(metrics->ascent - metrics->descent) / 2;
                break;
            case DWRITE_BASELINE_MATH:
                *baseline_coord = vertical ? (metrics->ascent + metrics->descent) / 2 :
                        -(metrics->ascent + metrics->descent) / 2;
                break;
            case DWRITE_BASELINE_HANGING:
                /* FIXME: this one isn't accurate, but close. */
                *baseline_coord = vertical ? metrics->capHeight * 6 / 7 + metrics->descent : metrics->capHeight * 6 / 7;
                break;
            case DWRITE_BASELINE_IDEOGRAPHIC_BOTTOM:
            case DWRITE_BASELINE_MINIMUM:
                *baseline_coord = vertical ? 0 : metrics->descent;
                break;
            case DWRITE_BASELINE_IDEOGRAPHIC_TOP:
            case DWRITE_BASELINE_MAXIMUM:
                *baseline_coord = vertical ? metrics->ascent + metrics->descent : -metrics->ascent;
                break;
            default:
                ;
        }
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource1* source, UINT32 text_pos, UINT32 len, IDWriteTextAnalysisSink1 *sink)
{
    FIXME("(%p %u %u %p): stub\n", source, text_pos, len, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, DWRITE_MATRIX *transform)
{
    TRACE("%d, %d, %p.\n", angle, is_sideways, transform);

    return IDWriteTextAnalyzer2_GetGlyphOrientationTransform(iface, angle, is_sideways, 0.0, 0.0, transform);
}

static HRESULT WINAPI dwritetextanalyzer1_GetScriptProperties(IDWriteTextAnalyzer2 *iface, DWRITE_SCRIPT_ANALYSIS sa,
    DWRITE_SCRIPT_PROPERTIES *props)
{
    TRACE("%u, %p.\n", sa.script, props);

    if (sa.script > Script_LastId)
        return E_INVALIDARG;

    *props = dwritescripts_properties[sa.script].props;
    return S_OK;
}

static inline BOOL is_char_from_simple_script(WCHAR c)
{
    if (IS_HIGH_SURROGATE(c) || IS_LOW_SURROGATE(c) ||
            /* LRM, RLM, LRE, RLE, PDF, LRO, RLO */
            c == 0x200e || c == 0x200f || (c >= 0x202a && c <= 0x202e))
        return FALSE;
    else {
        UINT16 script = get_char_script(c);
        return !dwritescripts_properties[script].is_complex;
    }
}

static HRESULT WINAPI dwritetextanalyzer1_GetTextComplexity(IDWriteTextAnalyzer2 *iface, const WCHAR *text,
    UINT32 len, IDWriteFontFace *face, BOOL *is_simple, UINT32 *len_read, UINT16 *indices)
{
    HRESULT hr = S_OK;
    int i;

    TRACE("%s:%u, %p, %p, %p, %p.\n", debugstr_wn(text, len), len, face, is_simple, len_read, indices);

    *is_simple = FALSE;
    *len_read = 0;

    if (!face)
        return E_INVALIDARG;

    if (len == 0) {
        *is_simple = TRUE;
        return S_OK;
    }

    *is_simple = text[0] && is_char_from_simple_script(text[0]);
    for (i = 1; i < len && text[i]; i++) {
        if (is_char_from_simple_script(text[i])) {
            if (!*is_simple)
                break;
        }
        else
            *is_simple = FALSE;
    }

    *len_read = i;

    /* fetch indices */
    if (*is_simple && indices)
    {
        UINT32 *codepoints;

        if (!(codepoints = calloc(*len_read, sizeof(*codepoints))))
            return E_OUTOFMEMORY;

        for (i = 0; i < *len_read; i++)
            codepoints[i] = text[i];

        hr = IDWriteFontFace_GetGlyphIndices(face, codepoints, *len_read, indices);
        free(codepoints);
    }

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustificationOpportunities(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length, UINT32 glyph_count,
    const WCHAR *text, const UINT16 *clustermap, const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, DWRITE_JUSTIFICATION_OPPORTUNITY *jo)
{
    FIXME("(%p %.2f %u %u %u %s %p %p %p): stub\n", face, font_em_size, sa.script, length, glyph_count,
        debugstr_wn(text, length), clustermap, prop, jo);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_JustifyGlyphAdvances(IDWriteTextAnalyzer2 *iface,
    FLOAT width, UINT32 glyph_count, const DWRITE_JUSTIFICATION_OPPORTUNITY *jo, const FLOAT *advances,
    const DWRITE_GLYPH_OFFSET *offsets, FLOAT *justifiedadvances, DWRITE_GLYPH_OFFSET *justifiedoffsets)
{
    FIXME("(%.2f %u %p %p %p %p %p): stub\n", width, glyph_count, jo, advances, offsets, justifiedadvances,
        justifiedoffsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustifiedGlyphs(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length,
    UINT32 glyph_count, UINT32 max_glyphcount, const UINT16 *clustermap, const UINT16 *indices,
    const FLOAT *advances, const FLOAT *justifiedadvances, const DWRITE_GLYPH_OFFSET *justifiedoffsets,
    const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, UINT32 *actual_count, UINT16 *modified_clustermap,
    UINT16 *modified_indices, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    FIXME("(%p %.2f %u %u %u %u %p %p %p %p %p %p %p %p %p %p %p): stub\n", face, font_em_size, sa.script,
        length, glyph_count, max_glyphcount, clustermap, indices, advances, justifiedadvances, justifiedoffsets,
        prop, actual_count, modified_clustermap, modified_indices, modified_advances, modified_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, FLOAT originX, FLOAT originY, DWRITE_MATRIX *m)
{
    static const DWRITE_MATRIX transforms[] = {
        {  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f },
        {  0.0f,  1.0f, -1.0f,  0.0f, 0.0f, 0.0f },
        { -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f },
        {  0.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f }
    };

    TRACE("%d, %d, %.2f, %.2f, %p.\n", angle, is_sideways, originX, originY, m);

    if ((UINT32)angle > DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES) {
        memset(m, 0, sizeof(*m));
        return E_INVALIDARG;
    }

    /* for sideways case simply rotate 90 degrees more */
    if (is_sideways) {
        switch (angle) {
        case DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES:
            angle = DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES;
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_90_DEGREES:
            angle = DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES;
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_180_DEGREES:
            angle = DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES;
            break;
        case DWRITE_GLYPH_ORIENTATION_ANGLE_270_DEGREES:
            angle = DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES;
            break;
        default:
            ;
        }
    }

    *m = transforms[angle];

    /* shift components represent transform necessary to get from original point to
       rotated one in new coordinate system */
    if ((originX != 0.0f || originY != 0.0f) && angle != DWRITE_GLYPH_ORIENTATION_ANGLE_0_DEGREES) {
        m->dx = originX - (m->m11 * originX + m->m21 * originY);
        m->dy = originY - (m->m12 * originX + m->m22 * originY);
    }

    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer2_GetTypographicFeatures(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *fontface, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *locale,
    UINT32 max_tagcount, UINT32 *actual_tagcount, DWRITE_FONT_FEATURE_TAG *tags)
{
    struct scriptshaping_context context = { 0 };
    const struct dwritescript_properties *props;
    struct dwrite_fontface *font_obj;

    TRACE("%p, %p, %u, %s, %u, %p, %p.\n", iface, fontface, sa.script, debugstr_w(locale), max_tagcount,
            actual_tagcount, tags);

    if (sa.script > Script_LastId)
        return E_INVALIDARG;

    font_obj = unsafe_impl_from_IDWriteFontFace(fontface);

    context.cache = fontface_get_shaping_cache(font_obj);
    context.language_tag = get_opentype_language(locale);
    props = &dwritescripts_properties[sa.script];

    return shape_get_typographic_features(&context, props->scripttags, max_tagcount, actual_tagcount, tags);
};

static HRESULT WINAPI dwritetextanalyzer2_CheckTypographicFeature(IDWriteTextAnalyzer2 *iface,
        IDWriteFontFace *fontface, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *locale, DWRITE_FONT_FEATURE_TAG feature,
        UINT32 glyph_count, const UINT16 *glyphs, UINT8 *feature_applies)
{
    struct scriptshaping_context context = { 0 };
    const struct dwritescript_properties *props;
    struct dwrite_fontface *font_obj;
    HRESULT hr;

    TRACE("%p, %p, %u, %s, %s, %u, %p, %p.\n", iface, fontface, sa.script, debugstr_w(locale), debugstr_fourcc(feature),
            glyph_count, glyphs, feature_applies);

    if (sa.script > Script_LastId)
        return E_INVALIDARG;

    font_obj = unsafe_impl_from_IDWriteFontFace(fontface);

    context.cache = fontface_get_shaping_cache(font_obj);
    context.language_tag = get_opentype_language(locale);
    if (!(context.glyph_infos = calloc(glyph_count, sizeof(*context.glyph_infos))))
        return E_OUTOFMEMORY;

    props = &dwritescripts_properties[sa.script];

    hr = shape_check_typographic_feature(&context, props->scripttags, feature, glyph_count, glyphs, feature_applies);

    free(context.glyph_infos);

    return hr;
}

static const IDWriteTextAnalyzer2Vtbl textanalyzervtbl =
{
    dwritetextanalyzer_QueryInterface,
    dwritetextanalyzer_AddRef,
    dwritetextanalyzer_Release,
    dwritetextanalyzer_AnalyzeScript,
    dwritetextanalyzer_AnalyzeBidi,
    dwritetextanalyzer_AnalyzeNumberSubstitution,
    dwritetextanalyzer_AnalyzeLineBreakpoints,
    dwritetextanalyzer_GetGlyphs,
    dwritetextanalyzer_GetGlyphPlacements,
    dwritetextanalyzer_GetGdiCompatibleGlyphPlacements,
    dwritetextanalyzer1_ApplyCharacterSpacing,
    dwritetextanalyzer1_GetBaseline,
    dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation,
    dwritetextanalyzer1_GetGlyphOrientationTransform,
    dwritetextanalyzer1_GetScriptProperties,
    dwritetextanalyzer1_GetTextComplexity,
    dwritetextanalyzer1_GetJustificationOpportunities,
    dwritetextanalyzer1_JustifyGlyphAdvances,
    dwritetextanalyzer1_GetJustifiedGlyphs,
    dwritetextanalyzer2_GetGlyphOrientationTransform,
    dwritetextanalyzer2_GetTypographicFeatures,
    dwritetextanalyzer2_CheckTypographicFeature
};

static IDWriteTextAnalyzer2 textanalyzer = { &textanalyzervtbl };

IDWriteTextAnalyzer2 *get_text_analyzer(void)
{
    return &textanalyzer;
}

static HRESULT WINAPI dwritenumbersubstitution_QueryInterface(IDWriteNumberSubstitution *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteNumberSubstitution) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteNumberSubstitution_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritenumbersubstitution_AddRef(IDWriteNumberSubstitution *iface)
{
    struct dwrite_numbersubstitution *object = impl_from_IDWriteNumberSubstitution(iface);
    ULONG refcount = InterlockedIncrement(&object->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI dwritenumbersubstitution_Release(IDWriteNumberSubstitution *iface)
{
    struct dwrite_numbersubstitution *object = impl_from_IDWriteNumberSubstitution(iface);
    ULONG refcount = InterlockedDecrement(&object->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        free(object->locale);
        free(object);
    }

    return refcount;
}

static const IDWriteNumberSubstitutionVtbl numbersubstitutionvtbl =
{
    dwritenumbersubstitution_QueryInterface,
    dwritenumbersubstitution_AddRef,
    dwritenumbersubstitution_Release
};

struct dwrite_numbersubstitution *unsafe_impl_from_IDWriteNumberSubstitution(IDWriteNumberSubstitution *iface)
{
    if (!iface || iface->lpVtbl != &numbersubstitutionvtbl)
        return NULL;
    return CONTAINING_RECORD(iface, struct dwrite_numbersubstitution, IDWriteNumberSubstitution_iface);
}

HRESULT create_numbersubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD method, const WCHAR *locale,
    BOOL ignore_user_override, IDWriteNumberSubstitution **ret)
{
    struct dwrite_numbersubstitution *substitution;

    *ret = NULL;

    if ((UINT32)method > DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL)
        return E_INVALIDARG;

    if (method != DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE && !IsValidLocaleName(locale))
        return E_INVALIDARG;

    if (!(substitution = calloc(1, sizeof(*substitution))))
        return E_OUTOFMEMORY;

    substitution->IDWriteNumberSubstitution_iface.lpVtbl = &numbersubstitutionvtbl;
    substitution->refcount = 1;
    substitution->ignore_user_override = ignore_user_override;
    substitution->method = method;
    substitution->locale = wcsdup(locale);
    if (locale && !substitution->locale)
    {
        free(substitution);
        return E_OUTOFMEMORY;
    }

    *ret = &substitution->IDWriteNumberSubstitution_iface;
    return S_OK;
}

/* IDWriteFontFallback */
static HRESULT WINAPI fontfallback_QueryInterface(IDWriteFontFallback1 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFallback1) ||
            IsEqualIID(riid, &IID_IDWriteFontFallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteFontFallback1_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontfallback_AddRef(IDWriteFontFallback1 *iface)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);

    TRACE("%p.\n", iface);

    return IDWriteFactory7_AddRef(fallback->factory);
}

static ULONG WINAPI fontfallback_Release(IDWriteFontFallback1 *iface)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);

    TRACE("%p.\n", fallback);

    return IDWriteFactory7_Release(fallback->factory);
}

static inline BOOL fallback_is_uvs(const struct text_source_context *context)
{
    /* MONGOLIAN FREE VARIATION SELECTOR ONE..THREE */
    if (context->ch >= 0x180b && context->ch <= 0x180d) return TRUE;
    /* VARIATION SELECTOR-1..16 */
    if (context->ch >= 0xfe00 && context->ch <= 0xfe0f) return TRUE;
    /* VARIATION SELECTOR-17..256 */
    if (context->ch >= 0xe0100 && context->ch <= 0xe01ef) return TRUE;
    return FALSE;
}

static UINT32 fallback_font_get_supported_length(IDWriteFont3 *font, IDWriteTextAnalysisSource *source,
        UINT32 position, UINT32 length)
{
    struct text_source_context context;
    UINT32 mapped = 0;

    text_source_context_init(&context, source, position, length);
    while (!text_source_get_next_u32_char(&context))
    {
        /* Ignore selectors that are not leading. */
        if (!mapped || !fallback_is_uvs(&context))
        {
            if (!IDWriteFont3_HasCharacter(font, context.ch)) break;
        }
        mapped += text_source_get_char_length(&context);
    }

    return mapped;
}

static HRESULT fallback_map_characters(const struct dwrite_fontfallback *fallback, IDWriteTextAnalysisSource *source,
        UINT32 position, UINT32 text_length, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style,
        DWRITE_FONT_STRETCH stretch, IDWriteFont **ret_font, UINT32 *ret_length, float *scale)
{
    const struct fallback_mapping *mapping = NULL;
    struct text_source_context context;
    const struct fallback_data *data;
    const WCHAR *locale_name = NULL;
    struct fallback_locale *locale;
    UINT32 i, length = 0, mapped;
    IDWriteFont3 *font;
    HRESULT hr;

    /* ~0u is a marker for system fallback data */
    data = fallback->data.count == ~0u ? &system_fallback : &fallback->data;

    /* We will try to map as much of given input as GetLocaleName() says. It's assumed that returned length covers
       whole span of characters set with that locale, so callback is only used once. */
    if (FAILED(hr = IDWriteTextAnalysisSource_GetLocaleName(source, position, &length, &locale_name)))
        return hr;

    length = length ? min(length, text_length) : text_length;
    if (!locale_name) locale_name = L"";

    /* Lookup locale entry once, if specific locale is missing neutral one will be returned. */
    locale = font_fallback_get_locale(&data->locales, locale_name);

    if (FAILED(hr = text_source_context_init(&context, source, position, length))) return hr;

    /* Find a mapping for given locale. */
    text_source_get_next_u32_char(&context);
    mapping = find_fallback_mapping(data, locale, context.ch);
    mapped = text_source_get_char_length(&context);
    while (!text_source_get_next_u32_char(&context))
    {
        if (find_fallback_mapping(data, locale, context.ch) != mapping) break;
        mapped += text_source_get_char_length(&context);
    }

    if (!mapping)
    {
        *ret_font = NULL;
        *ret_length = mapped;

        return S_OK;
    }

    /* Go through families in the mapping, use first family that supports some of the input. */
    for (i = 0; i < mapping->families_count; ++i)
    {
        if (SUCCEEDED(create_matching_font(mapping->collection ? mapping->collection : fallback->systemcollection,
                mapping->families[i], weight, style, stretch, &IID_IDWriteFont3, (void **)&font)))
        {
            if (!(mapped = fallback_font_get_supported_length(font, source, position, mapped)))
            {
                IDWriteFont3_Release(font);
                continue;
            }

            *ret_font = (IDWriteFont *)font;
            *ret_length = mapped;
            *scale = mapping->scale;

            return S_OK;
        }
    }

    /* Mapping was found, but either font couldn't be created or there's no font that supports given input. */
    *ret_font = NULL;
    *ret_length = length;

    return S_OK;
}

HRESULT create_matching_font(IDWriteFontCollection *collection, const WCHAR *name, DWRITE_FONT_WEIGHT weight,
        DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, REFIID riid, void **obj)
{
    IDWriteFontFamily *family;
    BOOL exists = FALSE;
    IDWriteFont *font;
    HRESULT hr;
    UINT32 i;

    *obj = NULL;

    hr = IDWriteFontCollection_FindFamilyName(collection, name, &i, &exists);
    if (FAILED(hr))
        return hr;

    if (!exists)
        return E_FAIL;

    hr = IDWriteFontCollection_GetFontFamily(collection, i, &family);
    if (FAILED(hr))
        return hr;

    hr = IDWriteFontFamily_GetFirstMatchingFont(family, weight, stretch, style, &font);
    IDWriteFontFamily_Release(family);

    if (SUCCEEDED(hr))
    {
        hr = IDWriteFont_QueryInterface(font, riid, obj);
        IDWriteFont_Release(font);
    }

    return hr;
}

static HRESULT WINAPI fontfallback_MapCharacters(IDWriteFontFallback1 *iface, IDWriteTextAnalysisSource *source,
        UINT32 position, UINT32 length, IDWriteFontCollection *basecollection, const WCHAR *basefamily,
        DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, UINT32 *mapped_length,
        IDWriteFont **ret_font, float *scale)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);
    IDWriteFont3 *font;

    TRACE("%p, %p, %u, %u, %p, %s, %u, %u, %u, %p, %p, %p.\n", iface, source, position, length,
        basecollection, debugstr_w(basefamily), weight, style, stretch, mapped_length, ret_font, scale);

    *mapped_length = 0;
    *ret_font = NULL;
    *scale = 1.0f;

    if (!source)
        return E_INVALIDARG;

    if (!length)
        return S_OK;

    if (!basecollection)
        basecollection = fallback->systemcollection;

    if (basefamily && *basefamily)
    {
        if (SUCCEEDED(create_matching_font(basecollection, basefamily, weight, style, stretch,
                &IID_IDWriteFont, (void **)&font)))
        {
            if ((*mapped_length = fallback_font_get_supported_length(font, source, position, length)))
            {
                *ret_font = (IDWriteFont *)font;
                *scale = 1.0f;
                return S_OK;
            }
            IDWriteFont3_Release(font);
        }
    }

    return fallback_map_characters(fallback, source, position, length, weight, style, stretch, ret_font, mapped_length, scale);
}

static HRESULT WINAPI fontfallback1_MapCharacters(IDWriteFontFallback1 *iface, IDWriteTextAnalysisSource *source,
    UINT32 position, UINT32 length, IDWriteFontCollection *basecollection, const WCHAR *basefamily,
    DWRITE_FONT_AXIS_VALUE const *axis_values, UINT32 values_count, UINT32 *mapped_length, FLOAT *scale,
    IDWriteFontFace5 **ret_fontface)
{
    FIXME("%p, %p, %u, %u, %p, %s, %p, %u, %p, %p, %p.\n", iface, source, position, length, basecollection,
            debugstr_w(basefamily), axis_values, values_count, mapped_length, scale, ret_fontface);

    return E_NOTIMPL;
}

static const IDWriteFontFallback1Vtbl fontfallbackvtbl =
{
    fontfallback_QueryInterface,
    fontfallback_AddRef,
    fontfallback_Release,
    fontfallback_MapCharacters,
    fontfallback1_MapCharacters,
};

void release_system_fontfallback(IDWriteFontFallback1 *iface)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);
    IDWriteFontCollection_Release(fallback->systemcollection);
    free(fallback);
}

static ULONG WINAPI customfontfallback_AddRef(IDWriteFontFallback1 *iface)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);
    ULONG refcount = InterlockedIncrement(&fallback->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI customfontfallback_Release(IDWriteFontFallback1 *iface)
{
    struct dwrite_fontfallback *fallback = impl_from_IDWriteFontFallback1(iface);
    ULONG refcount = InterlockedDecrement(&fallback->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(fallback->factory);
        if (fallback->systemcollection)
            IDWriteFontCollection_Release(fallback->systemcollection);
        release_fallback_data(&fallback->data);
        free(fallback);
    }

    return refcount;
}

static const IDWriteFontFallback1Vtbl customfontfallbackvtbl =
{
    fontfallback_QueryInterface,
    customfontfallback_AddRef,
    customfontfallback_Release,
    fontfallback_MapCharacters,
    fontfallback1_MapCharacters,
};

static HRESULT WINAPI fontfallbackbuilder_QueryInterface(IDWriteFontFallbackBuilder *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteFontFallbackBuilder) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDWriteFontFallbackBuilder_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI fontfallbackbuilder_AddRef(IDWriteFontFallbackBuilder *iface)
{
    struct dwrite_fontfallback_builder *fallbackbuilder = impl_from_IDWriteFontFallbackBuilder(iface);
    ULONG refcount = InterlockedIncrement(&fallbackbuilder->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI fontfallbackbuilder_Release(IDWriteFontFallbackBuilder *iface)
{
    struct dwrite_fontfallback_builder *builder = impl_from_IDWriteFontFallbackBuilder(iface);
    ULONG refcount = InterlockedDecrement(&builder->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
    {
        IDWriteFactory7_Release(builder->factory);
        release_fallback_data(&builder->data);
        free(builder);
    }

    return refcount;
}

static struct fallback_locale * fallback_builder_add_locale(struct dwrite_fontfallback_builder *builder,
        const WCHAR *locale_name)
{
    struct fallback_locale *locale;

    if (!locale_name) locale_name = L"";
    if ((locale = font_fallback_get_locale(&builder->data.locales, locale_name))) return locale;
    if (!(locale = calloc(1, sizeof(*locale)))) return NULL;
    lstrcpynW(locale->name, locale_name, ARRAY_SIZE(locale->name));
    list_add_tail(&builder->data.locales, &locale->entry);
    return locale;
}

static HRESULT WINAPI fontfallbackbuilder_AddMapping(IDWriteFontFallbackBuilder *iface,
        const DWRITE_UNICODE_RANGE *ranges, UINT32 ranges_count, WCHAR const **families, UINT32 families_count,
        IDWriteFontCollection *collection, WCHAR const *locale_name, WCHAR const *base_family, float scale)
{
    struct dwrite_fontfallback_builder *builder = impl_from_IDWriteFontFallbackBuilder(iface);
    struct fallback_mapping *mapping;
    struct fallback_locale *locale;
    unsigned int i, j, m, count;

    TRACE("%p, %p, %u, %p, %u, %p, %s, %s, %f.\n", iface, ranges, ranges_count, families, families_count,
            collection, debugstr_w(locale_name), debugstr_w(base_family), scale);

    if (!ranges || !ranges_count || !families || !families_count || scale < 0.0f)
        return E_INVALIDARG;

    if (base_family)
        FIXME("base family ignored.\n");

    if (!dwrite_array_reserve((void **)&builder->data.mappings, &builder->mappings_size,
            builder->data.count + 1, sizeof(*builder->data.mappings)))
    {
        return E_OUTOFMEMORY;
    }

    mapping = &builder->data.mappings[builder->data.count];
    memset(mapping, 0, sizeof(*mapping));

    /* Append new mapping, link to its locale node. */

    if (!(locale = fallback_builder_add_locale(builder, locale_name)))
        return E_FAIL;

    if (!(mapping->ranges = calloc(ranges_count, sizeof(*mapping->ranges))))
        goto failed;

    /* Filter ranges that won't be usable. */
    for (i = 0, count = 0; i < ranges_count; ++i)
    {
        if (ranges[i].first > ranges[i].last) continue;
        if (ranges[i].first > 0x10ffff) continue;
        mapping->ranges[count].first = ranges[i].first;
        mapping->ranges[count].last = min(ranges[i].last, 0x10ffff);
        count++;
    }
    if (!count)
    {
        release_fallback_mapping(mapping);
        return S_OK;
    }

    mapping->ranges_count = count;

    if (!(mapping->families = calloc(families_count, sizeof(*mapping->families))))
        goto failed;
    mapping->families_count = families_count;
    for (i = 0; i < families_count; i++)
        if (!(mapping->families[i] = wcsdup(families[i]))) goto failed;
    mapping->scale = scale;

    if (FAILED(fallback_locale_add_mapping(locale, builder->data.count))) goto failed;

    /* Mappings with explicit collections take priority, for that reduce existing mappings ranges
       by newly added ranges. */

    mapping->collection = collection;
    if (mapping->collection)
    {
        IDWriteFontCollection_AddRef(mapping->collection);

        for (m = 0; m < builder->data.count; ++m)
        {
            struct fallback_mapping *c = &builder->data.mappings[m];
            if (c->collection) continue;
            for (i = 0; i < count; ++i)
            {
                const DWRITE_UNICODE_RANGE *new_range = &mapping->ranges[i];

                for (j = 0; j < c->ranges_count; ++j)
                {
                    DWRITE_UNICODE_RANGE *range = &c->ranges[j];

                    /* In case existing ranges intersect, disable or reduce them */
                    if (range->first >= new_range->first && range->last <= new_range->last)
                    {
                        range->first = range->last = ~0u;
                    }
                    else if (range->first >= new_range->first && range->first <= new_range->last)
                    {
                        range->first = new_range->last;
                    }
                    else if (range->last >= new_range->first && range->last <= new_range->last)
                    {
                        range->last = new_range->first;
                    }
                }
            }
        }
    }

    builder->data.count++;
    return S_OK;

failed:
    release_fallback_mapping(mapping);
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI fontfallbackbuilder_AddMappings(IDWriteFontFallbackBuilder *iface, IDWriteFontFallback *fallback)
{
    FIXME("%p, %p stub.\n", iface, fallback);

    return E_NOTIMPL;
}

static HRESULT fallbackbuilder_init_fallback_data(const struct dwrite_fontfallback_builder *builder,
        struct fallback_data *data)
{
    struct fallback_locale *iter, *locale;
    size_t i, j;

    /* Duplicate locales list. */
    list_init(&data->locales);
    LIST_FOR_EACH_ENTRY(iter, &builder->data.locales, struct fallback_locale, entry)
    {
        if (!(locale = calloc(1, sizeof(*locale)))) goto failed;
        wcscpy(locale->name, iter->name);
        locale->ranges.count = iter->ranges.count;
        locale->ranges.size = iter->ranges.count;
        if (!(locale->ranges.data = malloc(iter->ranges.count * sizeof(*iter->ranges.data))))
        {
            free(locale);
            goto failed;
        }
        memcpy(locale->ranges.data, iter->ranges.data, iter->ranges.count * sizeof(*iter->ranges.data));
        list_add_tail(&data->locales, &locale->entry);
    }

    /* Duplicate mappings. */
    if (!(data->mappings = calloc(builder->data.count, sizeof(*data->mappings))))
        goto failed;

    data->count = builder->data.count;
    for (i = 0; i < data->count; ++i)
    {
        struct fallback_mapping *src = &builder->data.mappings[i];
        struct fallback_mapping *dst = &data->mappings[i];

        if (!(dst->ranges = calloc(src->ranges_count, sizeof(*src->ranges)))) goto failed;
        memcpy(dst->ranges, src->ranges, src->ranges_count * sizeof(*src->ranges));
        dst->ranges_count = src->ranges_count;

        if (!(dst->families = calloc(src->families_count, sizeof(*src->families)))) goto failed;
        dst->families_count = src->families_count;
        for (j = 0; j < src->families_count; ++j)
        {
            if (!(dst->families[j] = wcsdup(src->families[j]))) goto failed;
        }

        dst->collection = src->collection;
        if (dst->collection)
            IDWriteFontCollection_AddRef(dst->collection);
        dst->scale = src->scale;
    }

    return S_OK;

failed:

    return E_OUTOFMEMORY;
}

static HRESULT fallbackbuilder_create_fallback(struct dwrite_fontfallback_builder *builder, struct dwrite_fontfallback **ret)
{
    struct dwrite_fontfallback *fallback;
    HRESULT hr;

    if (!(fallback = calloc(1, sizeof(*fallback))))
        return E_OUTOFMEMORY;

    fallback->IDWriteFontFallback1_iface.lpVtbl = &customfontfallbackvtbl;
    fallback->refcount = 1;
    fallback->factory = builder->factory;
    IDWriteFactory7_AddRef(fallback->factory);
    if (FAILED(hr = IDWriteFactory_GetSystemFontCollection((IDWriteFactory *)fallback->factory,
            &fallback->systemcollection, FALSE)))
    {
        goto done;
    }

    if (FAILED(hr = fallbackbuilder_init_fallback_data(builder, &fallback->data)))
    {
        goto done;
    }

    *ret = fallback;

    return S_OK;

done:

    IDWriteFontFallback1_Release(&fallback->IDWriteFontFallback1_iface);
    return hr;
}

static HRESULT WINAPI fontfallbackbuilder_CreateFontFallback(IDWriteFontFallbackBuilder *iface,
        IDWriteFontFallback **ret)
{
    struct dwrite_fontfallback_builder *builder = impl_from_IDWriteFontFallbackBuilder(iface);
    struct dwrite_fontfallback *fallback;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    *ret = NULL;

    if (SUCCEEDED(hr = fallbackbuilder_create_fallback(builder, &fallback)))
    {
        *ret = (IDWriteFontFallback *)&fallback->IDWriteFontFallback1_iface;
    }

    return hr;
}

static const IDWriteFontFallbackBuilderVtbl fontfallbackbuildervtbl =
{
    fontfallbackbuilder_QueryInterface,
    fontfallbackbuilder_AddRef,
    fontfallbackbuilder_Release,
    fontfallbackbuilder_AddMapping,
    fontfallbackbuilder_AddMappings,
    fontfallbackbuilder_CreateFontFallback,
};

static HRESULT create_fontfallback_builder_internal(IDWriteFactory7 *factory, struct dwrite_fontfallback_builder **ret)
{
    struct dwrite_fontfallback_builder *builder;

    *ret = NULL;

    if (!(builder = calloc(1, sizeof(*builder))))
        return E_OUTOFMEMORY;

    builder->IDWriteFontFallbackBuilder_iface.lpVtbl = &fontfallbackbuildervtbl;
    builder->refcount = 1;
    builder->factory = factory;
    IDWriteFactory7_AddRef(builder->factory);
    list_init(&builder->data.locales);

    *ret = builder;

    return S_OK;
}

HRESULT create_fontfallback_builder(IDWriteFactory7 *factory, IDWriteFontFallbackBuilder **ret)
{
    struct dwrite_fontfallback_builder *builder;
    HRESULT hr;

    *ret = NULL;

    if (SUCCEEDED(hr = create_fontfallback_builder_internal(factory, &builder)))
        *ret = &builder->IDWriteFontFallbackBuilder_iface;

    return hr;
}

static void system_fallback_parse_ranges(const char *str, DWRITE_UNICODE_RANGE *ranges,
        unsigned int max_count, unsigned int *ret)
{
    unsigned int count = 0;
    char *end;

    while (*str && count < max_count)
    {
        ranges[count].first = ranges[count].last = strtoul(str, &end, 16);
        if (*end == '-')
        {
            str = end + 1;
            ranges[count].last = strtoul(str, &end, 16);
        }
        str = end;
        if (*str == ',') str++;
        count++;
    }

    *ret = count;
}

static void system_fallback_parse_families(WCHAR *str, WCHAR **families, unsigned int max_count,
        unsigned int *ret)
{
    unsigned int count = 0;
    WCHAR *family, *ctx;

    family = wcstok_s(str, L",", &ctx);
    while (family && count < max_count)
    {
        while (*family == ' ') family++;
        families[count++] = family;
        family = wcstok_s(NULL, L",", &ctx);
    }

    *ret = count;
}

static INIT_ONCE init_system_fallback_once = INIT_ONCE_STATIC_INIT;

/* Particular factory instance used for initialization is not important, it won't be referenced by
   created fallback data. */
static BOOL WINAPI dwrite_system_fallback_initonce(INIT_ONCE *once, void *param, void **context)
{
    struct dwrite_fontfallback_builder *builder;
    IDWriteFontFallbackBuilder *builder_iface;
    unsigned int range_count, families_count;
    IDWriteFactory7 *factory = param;
    DWRITE_UNICODE_RANGE ranges[16];
    WCHAR *families[4], *str;
    HRESULT hr;
    size_t i;

    if (FAILED(create_fontfallback_builder_internal(factory, &builder))) return FALSE;
    builder_iface = &builder->IDWriteFontFallbackBuilder_iface;

    for (i = 0; i < ARRAY_SIZE(system_fallback_config); ++i)
    {
        const struct fallback_description *entry = &system_fallback_config[i];

        system_fallback_parse_ranges(entry->ranges, ranges, ARRAY_SIZE(ranges), &range_count);

        /* TODO: reuse the buffer */
        str = wcsdup(entry->families);
        system_fallback_parse_families(str, families, ARRAY_SIZE(families), &families_count);

        if (FAILED(hr = IDWriteFontFallbackBuilder_AddMapping(builder_iface, ranges, range_count,
                (const WCHAR **)families, families_count, NULL, entry->locale, NULL, 1.0f)))
        {
            WARN("Failed to add mapping, hr %#lx.\n", hr);
        }

        free(str);
    }

    hr = fallbackbuilder_init_fallback_data(builder, &system_fallback);
    IDWriteFontFallbackBuilder_Release(builder_iface);

    return hr == S_OK;
}

void release_system_fallback_data(void)
{
    release_fallback_data(&system_fallback);
}

HRESULT create_system_fontfallback(IDWriteFactory7 *factory, IDWriteFontFallback1 **ret)
{
    struct dwrite_fontfallback *fallback;

    *ret = NULL;

    if (!InitOnceExecuteOnce(&init_system_fallback_once, dwrite_system_fallback_initonce, factory, NULL))
    {
        WARN("Failed to initialize system fallback data.\n");
        return E_FAIL;
    }

    if (!(fallback = calloc(1, sizeof(*fallback))))
        return E_OUTOFMEMORY;

    fallback->IDWriteFontFallback1_iface.lpVtbl = &fontfallbackvtbl;
    fallback->factory = factory;
    fallback->data.count = ~0u;
    IDWriteFactory_GetSystemFontCollection((IDWriteFactory *)fallback->factory, &fallback->systemcollection, FALSE);

    *ret = &fallback->IDWriteFontFallback1_iface;

    return S_OK;
}
