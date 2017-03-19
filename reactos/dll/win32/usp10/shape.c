/*
 * Implementation of Shaping for the Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2010 CodeWeavers, Aric Stewart
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
 *
 */

#include "usp10_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

#define FIRST_ARABIC_CHAR 0x0600
#define LAST_ARABIC_CHAR  0x06ff

typedef VOID (*ContextualShapingProc)(HDC, ScriptCache*, SCRIPT_ANALYSIS*,
                                      WCHAR*, INT, WORD*, INT*, INT, WORD*);

static void ContextualShape_Control(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Arabic(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Hebrew(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Syriac(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Thaana(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Phags_pa(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Thai(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Lao(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Sinhala(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Devanagari(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Bengali(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Gurmukhi(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Gujarati(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Oriya(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Tamil(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Telugu(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Kannada(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Malayalam(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Khmer(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
static void ContextualShape_Mongolian(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);

typedef VOID (*ShapeCharGlyphPropProc)( HDC , ScriptCache*, SCRIPT_ANALYSIS*, const WCHAR*, const INT, const WORD*, const INT, WORD*, SCRIPT_CHARPROP*, SCRIPT_GLYPHPROP*);

static void ShapeCharGlyphProp_Default( ScriptCache* psc, SCRIPT_ANALYSIS* psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD* pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP* pGlyphProp);
static void ShapeCharGlyphProp_Control( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Latin( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Arabic( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Hebrew( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Thai( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_None( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Tibet( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Sinhala( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Devanagari( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Bengali( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Gurmukhi( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Gujarati( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Oriya( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Tamil( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Telugu( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Kannada( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Malayalam( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );
static void ShapeCharGlyphProp_Khmer( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp );

extern const unsigned short indic_syllabic_table[] DECLSPEC_HIDDEN;
extern const unsigned short wine_shaping_table[] DECLSPEC_HIDDEN;
extern const unsigned short wine_shaping_forms[LAST_ARABIC_CHAR - FIRST_ARABIC_CHAR + 1][4] DECLSPEC_HIDDEN;

enum joining_types {
    jtU,
    jtT,
    jtR,
    jtL,
    jtD,
    jtC
};

enum joined_forms {
    Xn=0,
    Xr,
    Xl,
    Xm,
    /* Syriac Alaph */
    Afj,
    Afn,
    Afx
};

typedef struct tagVowelComponents
{
    WCHAR base;
    WCHAR parts[3];
} VowelComponents;

typedef struct tagConsonantComponents
{
    WCHAR parts[3];
    WCHAR output;
} ConsonantComponents;

typedef void (*second_reorder_function)(LPWSTR pwChar, IndicSyllable *syllable,WORD* pwGlyphs, IndicSyllable* glyph_index, lexical_function lex);

typedef int (*combining_lexical_function)(WCHAR c);

/* the orders of joined_forms and contextual_features need to line up */
static const char* contextual_features[] =
{
    "isol",
    "fina",
    "init",
    "medi",
    /* Syriac Alaph */
    "med2",
    "fin2",
    "fin3"
};

static OPENTYPE_FEATURE_RECORD standard_features[] =
{
    { MS_MAKE_TAG('c','c','m','p'), 1},
    { MS_MAKE_TAG('l','o','c','l'), 1},
};

static OPENTYPE_FEATURE_RECORD latin_features[] =
{
    { MS_MAKE_TAG('l','o','c','l'), 1},
    { MS_MAKE_TAG('c','c','m','p'), 1},
    { MS_MAKE_TAG('l','i','g','a'), 1},
    { MS_MAKE_TAG('c','l','i','g'), 1},
};

static OPENTYPE_FEATURE_RECORD latin_gpos_features[] =
{
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('m','a','r','k'), 1},
    { MS_MAKE_TAG('m','k','m','k'), 1},
};

static OPENTYPE_FEATURE_RECORD arabic_features[] =
{
    { MS_MAKE_TAG('r','l','i','g'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
    { MS_MAKE_TAG('l','i','g','a'), 1},
    { MS_MAKE_TAG('d','l','i','g'), 1},
    { MS_MAKE_TAG('c','s','w','h'), 1},
    { MS_MAKE_TAG('m','s','e','t'), 1},
};

static const char* required_arabic_features[] =
{
    "fina",
    "init",
    "medi",
    "rlig",
    NULL
};

static OPENTYPE_FEATURE_RECORD arabic_gpos_features[] =
{
    { MS_MAKE_TAG('c','u','r','s'), 1},
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('m','a','r','k'), 1},
    { MS_MAKE_TAG('m','k','m','k'), 1},
};

static OPENTYPE_FEATURE_RECORD hebrew_features[] =
{
    { MS_MAKE_TAG('c','c','m','p'), 1},
    { MS_MAKE_TAG('d','l','i','g'), 0},
};

static OPENTYPE_FEATURE_RECORD hebrew_gpos_features[] =
{
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('m','a','r','k'), 1},
};

static OPENTYPE_FEATURE_RECORD syriac_features[] =
{
    { MS_MAKE_TAG('r','l','i','g'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
    { MS_MAKE_TAG('l','i','g','a'), 1},
    { MS_MAKE_TAG('d','l','i','g'), 1},
};

static const char* required_syriac_features[] =
{
    "fina",
    "fin2",
    "fin3",
    "init",
    "medi",
    "med2",
    "rlig",
    NULL
};

static OPENTYPE_FEATURE_RECORD syriac_gpos_features[] =
{
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('m','a','r','k'), 1},
    { MS_MAKE_TAG('m','k','m','k'), 1},
};

static OPENTYPE_FEATURE_RECORD sinhala_features[] =
{
    /* Presentation forms */
    { MS_MAKE_TAG('b','l','w','s'), 1},
    { MS_MAKE_TAG('a','b','v','s'), 1},
    { MS_MAKE_TAG('p','s','t','s'), 1},
};

static OPENTYPE_FEATURE_RECORD tibetan_features[] =
{
    { MS_MAKE_TAG('a','b','v','s'), 1},
    { MS_MAKE_TAG('b','l','w','s'), 1},
};

static OPENTYPE_FEATURE_RECORD tibetan_gpos_features[] =
{
    { MS_MAKE_TAG('a','b','v','m'), 1},
    { MS_MAKE_TAG('b','l','w','m'), 1},
};

static OPENTYPE_FEATURE_RECORD phags_features[] =
{
    { MS_MAKE_TAG('a','b','v','s'), 1},
    { MS_MAKE_TAG('b','l','w','s'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
};

static OPENTYPE_FEATURE_RECORD thai_features[] =
{
    { MS_MAKE_TAG('c','c','m','p'), 1},
};

static OPENTYPE_FEATURE_RECORD thai_gpos_features[] =
{
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('m','a','r','k'), 1},
    { MS_MAKE_TAG('m','k','m','k'), 1},
};

static const char* required_lao_features[] =
{
    "ccmp",
    NULL
};

static const char* required_devanagari_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "blwf",
    "half",
    "vatu",
    "pres",
    "abvs",
    "blws",
    "psts",
    "haln",
    NULL
};

static OPENTYPE_FEATURE_RECORD devanagari_features[] =
{
    { MS_MAKE_TAG('p','r','e','s'), 1},
    { MS_MAKE_TAG('a','b','v','s'), 1},
    { MS_MAKE_TAG('b','l','w','s'), 1},
    { MS_MAKE_TAG('p','s','t','s'), 1},
    { MS_MAKE_TAG('h','a','l','n'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
};

static OPENTYPE_FEATURE_RECORD devanagari_gpos_features[] =
{
    { MS_MAKE_TAG('k','e','r','n'), 1},
    { MS_MAKE_TAG('d','i','s','t'), 1},
    { MS_MAKE_TAG('a','b','v','m'), 1},
    { MS_MAKE_TAG('b','l','w','m'), 1},
};

static OPENTYPE_FEATURE_RECORD myanmar_features[] =
{
    { MS_MAKE_TAG('l','i','g','a'), 1},
    { MS_MAKE_TAG('c','l','i','g'), 1},
};

static const char* required_bengali_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "blwf",
    "half",
    "vatu",
    "pstf",
    "init",
    "abvs",
    "blws",
    "psts",
    "haln",
    NULL
};

static const char* required_gurmukhi_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "blwf",
    "half",
    "pstf",
    "vatu",
    "cjct",
    "pres",
    "abvs",
    "blws",
    "psts",
    "haln",
    "calt",
    NULL
};

static const char* required_oriya_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "blwf",
    "pstf",
    "cjct",
    "pres",
    "abvs",
    "blws",
    "psts",
    "haln",
    "calt",
    NULL
};

static const char* required_tamil_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "pref",
    "half",
    "pres",
    "abvs",
    "blws",
    "psts",
    "haln",
    "calt",
    NULL
};

static const char* required_telugu_features[] =
{
    "nukt",
    "akhn",
    "rphf",
    "pref",
    "half",
    "pstf",
    "cjct",
    "pres",
    "abvs",
    "blws",
    "psts",
    "haln",
    "calt",
    NULL
};

static OPENTYPE_FEATURE_RECORD khmer_features[] =
{
    { MS_MAKE_TAG('p','r','e','s'), 1},
    { MS_MAKE_TAG('b','l','w','s'), 1},
    { MS_MAKE_TAG('a','b','v','s'), 1},
    { MS_MAKE_TAG('p','s','t','s'), 1},
    { MS_MAKE_TAG('c','l','i','g'), 1},
};

static const char* required_khmer_features[] =
{
    "pref",
    "blwf",
    "abvf",
    "pstf",
    "pres",
    "blws",
    "abvs",
    "psts",
    "clig",
    NULL
};

static OPENTYPE_FEATURE_RECORD khmer_gpos_features[] =
{
    { MS_MAKE_TAG('d','i','s','t'), 1},
    { MS_MAKE_TAG('b','l','w','m'), 1},
    { MS_MAKE_TAG('a','b','v','m'), 1},
    { MS_MAKE_TAG('m','k','m','k'), 1},
};

static OPENTYPE_FEATURE_RECORD ethiopic_features[] =
{
    { MS_MAKE_TAG('c','c','m','p'), 1},
    { MS_MAKE_TAG('l','o','c','l'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
    { MS_MAKE_TAG('l','i','g','a'), 1},
};

static OPENTYPE_FEATURE_RECORD mongolian_features[] =
{
    { MS_MAKE_TAG('c','c','m','p'), 1},
    { MS_MAKE_TAG('l','o','c','l'), 1},
    { MS_MAKE_TAG('c','a','l','t'), 1},
    { MS_MAKE_TAG('r','l','i','g'), 1},
};

typedef struct ScriptShapeDataTag {
    TEXTRANGE_PROPERTIES   defaultTextRange;
    TEXTRANGE_PROPERTIES   defaultGPOSTextRange;
    const char**           requiredFeatures;
    OPENTYPE_TAG           newOtTag;
    ContextualShapingProc  contextProc;
    ShapeCharGlyphPropProc charGlyphPropProc;
} ScriptShapeData;

/* in order of scripts */
static const ScriptShapeData ShapingData[] =
{
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, ContextualShape_Control, ShapeCharGlyphProp_Control},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ arabic_features, 6}, {arabic_gpos_features, 4}, required_arabic_features, 0, ContextualShape_Arabic, ShapeCharGlyphProp_Arabic},
    {{ arabic_features, 6}, {arabic_gpos_features, 4}, required_arabic_features, 0, ContextualShape_Arabic, ShapeCharGlyphProp_Arabic},
    {{ hebrew_features, 2}, {hebrew_gpos_features, 2}, NULL, 0, ContextualShape_Hebrew, ShapeCharGlyphProp_Hebrew},
    {{ syriac_features, 4}, {syriac_gpos_features, 3}, required_syriac_features, 0, ContextualShape_Syriac, ShapeCharGlyphProp_None},
    {{ arabic_features, 6}, {arabic_gpos_features, 4}, required_arabic_features, 0, ContextualShape_Arabic, ShapeCharGlyphProp_Arabic},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, ContextualShape_Thaana, ShapeCharGlyphProp_None},
    {{ standard_features, 2}, {latin_gpos_features, 3}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {latin_gpos_features, 3}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {latin_gpos_features, 3}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {latin_gpos_features, 3}, NULL, 0, NULL, NULL},
    {{ sinhala_features, 3}, {NULL, 0}, NULL, 0, ContextualShape_Sinhala, ShapeCharGlyphProp_Sinhala},
    {{ tibetan_features, 2}, {tibetan_gpos_features, 2}, NULL, 0, NULL, ShapeCharGlyphProp_Tibet},
    {{ tibetan_features, 2}, {tibetan_gpos_features, 2}, NULL, 0, NULL, ShapeCharGlyphProp_Tibet},
    {{ phags_features, 3}, {NULL, 0}, NULL, 0, ContextualShape_Phags_pa, ShapeCharGlyphProp_Thai},
    {{ thai_features, 1}, {thai_gpos_features, 3}, NULL, 0, ContextualShape_Thai, ShapeCharGlyphProp_Thai},
    {{ thai_features, 1}, {thai_gpos_features, 3}, NULL, 0, ContextualShape_Thai, NULL},
    {{ thai_features, 1}, {thai_gpos_features, 3}, required_lao_features, 0, ContextualShape_Lao, ShapeCharGlyphProp_Thai},
    {{ thai_features, 1}, {thai_gpos_features, 3}, required_lao_features, 0, ContextualShape_Lao, ShapeCharGlyphProp_Thai},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_devanagari_features, MS_MAKE_TAG('d','e','v','2'), ContextualShape_Devanagari, ShapeCharGlyphProp_Devanagari},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_devanagari_features, MS_MAKE_TAG('d','e','v','2'), ContextualShape_Devanagari, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_bengali_features, MS_MAKE_TAG('b','n','g','2'), ContextualShape_Bengali, ShapeCharGlyphProp_Bengali},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_bengali_features, MS_MAKE_TAG('b','n','g','2'), ContextualShape_Bengali, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_bengali_features, MS_MAKE_TAG('b','n','g','2'), ContextualShape_Bengali, ShapeCharGlyphProp_Bengali},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_gurmukhi_features, MS_MAKE_TAG('g','u','r','2'), ContextualShape_Gurmukhi, ShapeCharGlyphProp_Gurmukhi},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_gurmukhi_features, MS_MAKE_TAG('g','u','r','2'), ContextualShape_Gurmukhi, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_devanagari_features, MS_MAKE_TAG('g','j','r','2'), ContextualShape_Gujarati, ShapeCharGlyphProp_Gujarati},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_devanagari_features, MS_MAKE_TAG('g','j','r','2'), ContextualShape_Gujarati, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_devanagari_features, MS_MAKE_TAG('g','j','r','2'), ContextualShape_Gujarati, ShapeCharGlyphProp_Gujarati},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_oriya_features, MS_MAKE_TAG('o','r','y','2'), ContextualShape_Oriya, ShapeCharGlyphProp_Oriya},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_oriya_features, MS_MAKE_TAG('o','r','y','2'), ContextualShape_Oriya, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_tamil_features, MS_MAKE_TAG('t','a','m','2'), ContextualShape_Tamil, ShapeCharGlyphProp_Tamil},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_tamil_features, MS_MAKE_TAG('t','a','m','2'), ContextualShape_Tamil, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('t','e','l','2'), ContextualShape_Telugu, ShapeCharGlyphProp_Telugu},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('t','e','l','2'), ContextualShape_Telugu, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('k','n','d','2'), ContextualShape_Kannada, ShapeCharGlyphProp_Kannada},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('k','n','d','2'), ContextualShape_Kannada, NULL},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('m','l','m','2'), ContextualShape_Malayalam, ShapeCharGlyphProp_Malayalam},
    {{ devanagari_features, 6}, {devanagari_gpos_features, 4}, required_telugu_features, MS_MAKE_TAG('m','l','m','2'), ContextualShape_Malayalam, NULL},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ myanmar_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ myanmar_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ standard_features, 2}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ khmer_features, 5}, {khmer_gpos_features, 4}, required_khmer_features, 0, ContextualShape_Khmer, ShapeCharGlyphProp_Khmer},
    {{ khmer_features, 5}, {khmer_gpos_features, 4}, required_khmer_features, 0, ContextualShape_Khmer, ShapeCharGlyphProp_Khmer},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ ethiopic_features, 4}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ ethiopic_features, 4}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ mongolian_features, 4}, {NULL, 0}, NULL, 0, ContextualShape_Mongolian, NULL},
    {{ mongolian_features, 4}, {NULL, 0}, NULL, 0, ContextualShape_Mongolian, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ NULL, 0}, {NULL, 0}, NULL, 0, NULL, NULL},
    {{ hebrew_features, 2}, {hebrew_gpos_features, 2}, NULL, 0, ContextualShape_Hebrew, NULL},
    {{ latin_features, 4}, {latin_gpos_features, 3}, NULL, 0, NULL, ShapeCharGlyphProp_Latin},
    {{ thai_features, 1}, {thai_gpos_features, 3}, NULL, 0, ContextualShape_Thai, ShapeCharGlyphProp_Thai},
};

extern scriptData scriptInformation[];

static INT GSUB_apply_feature_all_lookups(LPCVOID header, LoadedFeature *feature, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count)
{
    int i;
    int out_index = GSUB_E_NOGLYPH;

    TRACE("%i lookups\n", feature->lookup_count);
    for (i = 0; i < feature->lookup_count; i++)
    {
        out_index = OpenType_apply_GSUB_lookup(header, feature->lookups[i], glyphs, glyph_index, write_dir, glyph_count);
        if (out_index != GSUB_E_NOGLYPH)
            break;
    }
    if (out_index == GSUB_E_NOGLYPH)
        TRACE("lookups found no glyphs\n");
    else
    {
        int out2;
        out2 = GSUB_apply_feature_all_lookups(header, feature, glyphs, glyph_index, write_dir, glyph_count);
        if (out2!=GSUB_E_NOGLYPH)
            out_index = out2;
    }
    return out_index;
}

static OPENTYPE_TAG get_opentype_script(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc, BOOL tryNew)
{
    UINT charset;

    if (psc->userScript != 0)
    {
        if (tryNew && ShapingData[psa->eScript].newOtTag != 0 && psc->userScript == scriptInformation[psa->eScript].scriptTag)
            return ShapingData[psa->eScript].newOtTag;
        else
            return psc->userScript;
    }

    if (tryNew && ShapingData[psa->eScript].newOtTag != 0)
        return ShapingData[psa->eScript].newOtTag;

    if (scriptInformation[psa->eScript].scriptTag)
        return scriptInformation[psa->eScript].scriptTag;

    /*
     * fall back to the font charset
     */
    charset = GetTextCharsetInfo(hdc, NULL, 0x0);
    switch (charset)
    {
        case ANSI_CHARSET:
        case BALTIC_CHARSET: return MS_MAKE_TAG('l','a','t','n');
        case CHINESEBIG5_CHARSET: return MS_MAKE_TAG('h','a','n','i');
        case EASTEUROPE_CHARSET: return MS_MAKE_TAG('l','a','t','n'); /* ?? */
        case GB2312_CHARSET: return MS_MAKE_TAG('h','a','n','i');
        case GREEK_CHARSET: return MS_MAKE_TAG('g','r','e','k');
        case HANGUL_CHARSET: return MS_MAKE_TAG('h','a','n','g');
        case RUSSIAN_CHARSET: return MS_MAKE_TAG('c','y','r','l');
        case SHIFTJIS_CHARSET: return MS_MAKE_TAG('k','a','n','a');
        case TURKISH_CHARSET: return MS_MAKE_TAG('l','a','t','n'); /* ?? */
        case VIETNAMESE_CHARSET: return MS_MAKE_TAG('l','a','t','n');
        case JOHAB_CHARSET: return MS_MAKE_TAG('l','a','t','n'); /* ?? */
        case ARABIC_CHARSET: return MS_MAKE_TAG('a','r','a','b');
        case HEBREW_CHARSET: return MS_MAKE_TAG('h','e','b','r');
        case THAI_CHARSET: return MS_MAKE_TAG('t','h','a','i');
        default: return MS_MAKE_TAG('l','a','t','n');
    }
}

static LoadedFeature* load_OT_feature(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc, char tableType, const char* feat)
{
    LoadedFeature *feature = NULL;

    if (psc->GSUB_Table || psc->GPOS_Table)
    {
        int attempt = 2;
        OPENTYPE_TAG tags;
        OPENTYPE_TAG language;
        OPENTYPE_TAG script = 0x00000000;
        int cTags;

        do
        {
            script = get_opentype_script(hdc,psa,psc,(attempt==2));
            if (psc->userLang != 0)
                language = psc->userLang;
            else
                language = MS_MAKE_TAG('d','f','l','t');
            attempt--;

            OpenType_GetFontFeatureTags(psc, script, language, FALSE, MS_MAKE_TAG(feat[0],feat[1],feat[2],feat[3]), tableType, 1, &tags, &cTags, &feature);

        } while(attempt && !feature);

        /* try in the default (latin) table */
        if (!feature && !script)
            OpenType_GetFontFeatureTags(psc, MS_MAKE_TAG('l','a','t','n'), MS_MAKE_TAG('d','f','l','t'), FALSE, MS_MAKE_TAG(feat[0],feat[1],feat[2],feat[3]), tableType, 1, &tags, &cTags, &feature);
    }

    TRACE("Feature %s located at %p\n",debugstr_an(feat,4),feature);
    return feature;
}

static INT apply_GSUB_feature_to_glyph(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, WORD *glyphs, INT index, INT write_dir, INT* glyph_count, const char* feat)
{
    LoadedFeature *feature;

    feature = load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, feat);
    if (!feature)
        return GSUB_E_NOFEATURE;

    TRACE("applying feature %s\n",feat);
    return GSUB_apply_feature_all_lookups(psc->GSUB_Table, feature, glyphs, index, write_dir, glyph_count);
}

static VOID *load_gsub_table(HDC hdc)
{
    VOID* GSUB_Table = NULL;
    int length = GetFontData(hdc, MS_MAKE_TAG('G', 'S', 'U', 'B'), 0, NULL, 0);
    if (length != GDI_ERROR)
    {
        GSUB_Table = HeapAlloc(GetProcessHeap(),0,length);
        GetFontData(hdc, MS_MAKE_TAG('G', 'S', 'U', 'B'), 0, GSUB_Table, length);
        TRACE("Loaded GSUB table of %i bytes\n",length);
    }
    return GSUB_Table;
}

static VOID *load_gpos_table(HDC hdc)
{
    VOID* GPOS_Table = NULL;
    int length = GetFontData(hdc, MS_MAKE_TAG('G', 'P', 'O', 'S'), 0, NULL, 0);
    if (length != GDI_ERROR)
    {
        GPOS_Table = HeapAlloc(GetProcessHeap(),0,length);
        GetFontData(hdc, MS_MAKE_TAG('G', 'P', 'O', 'S'), 0, GPOS_Table, length);
        TRACE("Loaded GPOS table of %i bytes\n",length);
    }
    return GPOS_Table;
}

static VOID *load_gdef_table(HDC hdc)
{
    VOID* GDEF_Table = NULL;
    int length = GetFontData(hdc, MS_MAKE_TAG('G', 'D', 'E', 'F'), 0, NULL, 0);
    if (length != GDI_ERROR)
    {
        GDEF_Table = HeapAlloc(GetProcessHeap(),0,length);
        GetFontData(hdc, MS_MAKE_TAG('G', 'D', 'E', 'F'), 0, GDEF_Table, length);
        TRACE("Loaded GDEF table of %i bytes\n",length);
    }
    return GDEF_Table;
}

static VOID load_ot_tables(HDC hdc, ScriptCache *psc)
{
    if (!psc->GSUB_Table)
        psc->GSUB_Table = load_gsub_table(hdc);
    if (!psc->GPOS_Table)
        psc->GPOS_Table = load_gpos_table(hdc);
    if (!psc->GDEF_Table)
        psc->GDEF_Table = load_gdef_table(hdc);
}

INT SHAPE_does_GSUB_feature_apply_to_chars(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, const WCHAR *chars, INT write_dir, INT count, const char* feature)
{
    WORD *glyphs;
    INT glyph_count = count;
    INT rc;

    glyphs = HeapAlloc(GetProcessHeap(),0,sizeof(WORD)*(count*2));
    GetGlyphIndicesW(hdc, chars, count, glyphs, 0);
    rc = apply_GSUB_feature_to_glyph(hdc, psa, psc, glyphs, 0, write_dir, &glyph_count, feature);
    if (rc > GSUB_E_NOGLYPH)
        rc = count - glyph_count;
    else
        rc = 0;

    HeapFree(GetProcessHeap(),0,glyphs);
    return rc;
}

static void UpdateClustersFromGlyphProp(const int cGlyphs, const int cChars, WORD* pwLogClust, SCRIPT_GLYPHPROP *pGlyphProp)
{
    int i;

    for (i = 0; i < cGlyphs; i++)
    {
        if (!pGlyphProp[i].sva.fClusterStart)
        {
            int j;
            for (j = 0; j < cChars; j++)
            {
                if (pwLogClust[j] == i)
                {
                    int k = j;
                    while (k >= 0 && k <cChars && !pGlyphProp[pwLogClust[k]].sva.fClusterStart)
                        k-=1;
                    if (k >= 0 && k <cChars && pGlyphProp[pwLogClust[k]].sva.fClusterStart)
                        pwLogClust[j] = pwLogClust[k];
                }
            }
        }
    }
}

static void UpdateClusters(int nextIndex, int changeCount, int write_dir, int chars, WORD* pwLogClust )
{
    if (changeCount == 0)
        return;
    else
    {
        int i;
        int target_glyph = nextIndex - write_dir;
        int seeking_glyph;
        int target_index = -1;
        int replacing_glyph = -1;
        int changed = 0;
        int top_logclust = 0;

        if (changeCount > 0)
        {
            if (write_dir > 0)
                target_glyph = nextIndex - changeCount;
            else
                target_glyph = nextIndex + (changeCount + 1);
        }

        seeking_glyph = target_glyph;
        for (i = 0; i < chars; i++)
            if (pwLogClust[i] > top_logclust)
                top_logclust = pwLogClust[i];

        do {
            if (write_dir > 0)
                for (i = 0; i < chars; i++)
                {
                    if (pwLogClust[i] == seeking_glyph)
                    {
                        target_index = i;
                        break;
                    }
                }
            else
                for (i = chars - 1; i >= 0; i--)
                {
                    if (pwLogClust[i] == seeking_glyph)
                    {
                        target_index = i;
                        break;
                    }
                }
            if (target_index == -1)
                seeking_glyph ++;
        }
        while (target_index == -1 && seeking_glyph <= top_logclust);

        if (target_index == -1)
        {
            ERR("Unable to find target glyph\n");
            return;
        }

        if (changeCount < 0)
        {
            /* merge glyphs */
            for(i = target_index; i < chars && i >= 0; i+=write_dir)
            {
                if (pwLogClust[i] == target_glyph)
                    continue;
                if(pwLogClust[i] == replacing_glyph)
                    pwLogClust[i] = target_glyph;
                else
                {
                    changed--;
                    if (changed >= changeCount)
                    {
                        replacing_glyph = pwLogClust[i];
                        pwLogClust[i] = target_glyph;
                    }
                    else
                        break;
                }
            }

            /* renumber trailing indexes*/
            for(i = target_index; i < chars && i >= 0; i+=write_dir)
            {
                if (pwLogClust[i] != target_glyph)
                    pwLogClust[i] += changeCount;
            }
        }
        else
        {
            for(i = target_index; i < chars && i >= 0; i+=write_dir)
                    pwLogClust[i] += changeCount;
        }
    }
}

static int apply_GSUB_feature(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, WORD *pwOutGlyphs, int write_dir, INT* pcGlyphs, INT cChars, const char* feat, WORD *pwLogClust )
{
    if (psc->GSUB_Table)
    {
        LoadedFeature *feature;
        int lookup_index;

        feature = load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, feat);
        if (!feature)
            return GSUB_E_NOFEATURE;

        TRACE("applying feature %s: %i lookups\n",debugstr_an(feat,4),feature->lookup_count);
        for (lookup_index = 0; lookup_index < feature->lookup_count; lookup_index++)
        {
            int i;

            if (write_dir > 0)
                i = 0;
            else
                i = *pcGlyphs-1;
            TRACE("applying lookup (%i/%i)\n",lookup_index,feature->lookup_count);
            while(i < *pcGlyphs && i >= 0)
            {
                INT nextIndex;
                INT prevCount = *pcGlyphs;

                nextIndex = OpenType_apply_GSUB_lookup(psc->GSUB_Table, feature->lookups[lookup_index], pwOutGlyphs, i, write_dir, pcGlyphs);
                if (*pcGlyphs != prevCount)
                {
                    UpdateClusters(nextIndex, *pcGlyphs - prevCount, write_dir, cChars, pwLogClust);
                    i = nextIndex;
                }
                else
                    i+=write_dir;
            }
        }
        return *pcGlyphs;
    }
    return GSUB_E_NOFEATURE;
}

static VOID GPOS_apply_feature(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance, LoadedFeature *feature, const WORD *glyphs, INT glyph_count, GOFFSET *pGoffset)
{
    int i;

    TRACE("%i lookups\n", feature->lookup_count);
    for (i = 0; i < feature->lookup_count; i++)
    {
        int j;
        for (j = 0; j < glyph_count; )
            j = OpenType_apply_GPOS_lookup(psc, lpotm, lplogfont, analysis, piAdvance, feature->lookups[i], glyphs, j, glyph_count, pGoffset);
    }
}

static inline BOOL get_GSUB_Indic2(SCRIPT_ANALYSIS *psa, ScriptCache *psc)
{
    OPENTYPE_TAG tag;
    HRESULT hr;
    int count = 0;

    hr = OpenType_GetFontScriptTags(psc, ShapingData[psa->eScript].newOtTag, 1, &tag, &count);

    return(SUCCEEDED(hr));
}

static void insert_glyph(WORD *pwGlyphs, INT *pcGlyphs, INT cChars, INT write_dir, WORD glyph, INT index, WORD *pwLogClust)
{
    int i;
    for (i = *pcGlyphs; i>=index; i--)
        pwGlyphs[i+1] = pwGlyphs[i];
    pwGlyphs[index] = glyph;
    *pcGlyphs = *pcGlyphs+1;
    if (write_dir < 0)
        UpdateClusters(index-3, 1, write_dir, cChars, pwLogClust);
    else
        UpdateClusters(index, 1, write_dir, cChars, pwLogClust);
}

static void mark_invalid_combinations(HDC hdc, const WCHAR* pwcChars, INT cChars, WORD *pwGlyphs, INT *pcGlyphs, INT write_dir, WORD *pwLogClust, combining_lexical_function lex)
{
    CHAR *context_type;
    int i,g;
    WCHAR invalid = 0x25cc;
    WORD invalid_glyph;

    context_type = HeapAlloc(GetProcessHeap(),0,cChars);

    /* Mark invalid combinations */
    for (i = 0; i < cChars; i++)
       context_type[i] = lex(pwcChars[i]);

    GetGlyphIndicesW(hdc, &invalid, 1, &invalid_glyph, 0);
    for (i = 1, g=1; i < cChars - 1; i++, g++)
    {
        if (context_type[i] != 0 && context_type[i+write_dir]==context_type[i])
        {
            insert_glyph(pwGlyphs, pcGlyphs, cChars, write_dir, invalid_glyph, g, pwLogClust);
            g++;
        }
    }

    HeapFree(GetProcessHeap(),0,context_type);
}

static void ContextualShape_Control(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int i;
    for (i=0; i < cChars; i++)
    {
        switch (pwcChars[i])
        {
            case 0x000A:
            case 0x000D:
                pwOutGlyphs[i] = psc->sfp.wgBlank;
                break;
            default:
                if (pwcChars[i] < 0x1C)
                    pwOutGlyphs[i] = psc->sfp.wgDefault;
                else
                    pwOutGlyphs[i] = psc->sfp.wgBlank;
        }
    }
}

static WCHAR neighbour_char(int i, int delta, const WCHAR* chars, INT cchLen)
{
    if (i + delta < 0)
        return 0;
    if ( i+ delta >= cchLen)
        return 0;

    i += delta;

    return chars[i];
}

static CHAR neighbour_joining_type(int i, int delta, const CHAR* context_type, INT cchLen, SCRIPT_ANALYSIS *psa)
{
    if (i + delta < 0)
    {
        if (psa->fLinkBefore)
            return jtR;
        else
            return jtU;
    }
    if ( i+ delta >= cchLen)
    {
        if (psa->fLinkAfter)
            return jtL;
        else
            return jtU;
    }

    i += delta;

    if (context_type[i] == jtT)
        return neighbour_joining_type(i,delta,context_type,cchLen,psa);
    else
        return context_type[i];
}

static inline BOOL right_join_causing(CHAR joining_type)
{
    return (joining_type == jtL || joining_type == jtD || joining_type == jtC);
}

static inline BOOL left_join_causing(CHAR joining_type)
{
    return (joining_type == jtR || joining_type == jtD || joining_type == jtC);
}

static inline BOOL word_break_causing(WCHAR chr)
{
    /* we are working within a string of characters already guareented to
       be within one script, Syriac, so we do not worry about any character
       other than the space character outside of that range */
    return (chr == 0 || chr == 0x20 );
}

static int combining_lexical_Arabic(WCHAR c)
{
    enum {Arab_Norm = 0, Arab_DIAC1, Arab_DIAC2, Arab_DIAC3, Arab_DIAC4, Arab_DIAC5, Arab_DIAC6, Arab_DIAC7, Arab_DIAC8};

   switch(c)
    {
        case 0x064B:
        case 0x064C:
        case 0x064E:
        case 0x064F:
        case 0x0652:
        case 0x0657:
        case 0x0658:
        case 0x06E1: return Arab_DIAC1;
        case 0x064D:
        case 0x0650:
        case 0x0656: return Arab_DIAC2;
        case 0x0651: return Arab_DIAC3;
        case 0x0610:
        case 0x0611:
        case 0x0612:
        case 0x0613:
        case 0x0614:
        case 0x0659:
        case 0x06D6:
        case 0x06DC:
        case 0x06DF:
        case 0x06E0:
        case 0x06E2:
        case 0x06E4:
        case 0x06E7:
        case 0x06E8:
        case 0x06EB:
        case 0x06EC: return Arab_DIAC4;
        case 0x06E3:
        case 0x06EA:
        case 0x06ED: return Arab_DIAC5;
        case 0x0670: return Arab_DIAC6;
        case 0x0653: return Arab_DIAC7;
        case 0x0655:
        case 0x0654: return Arab_DIAC8;
        default: return Arab_Norm;
    }
}

/*
 * ContextualShape_Arabic
 */
static void ContextualShape_Arabic(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    CHAR *context_type;
    INT *context_shape;
    INT dirR, dirL;
    int i;
    int char_index;
    int glyph_index;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        dirR = 1;
        dirL = -1;
    }
    else
    {
        dirR = -1;
        dirL = 1;
    }

    load_ot_tables(hdc, psc);

    context_type = HeapAlloc(GetProcessHeap(),0,cChars);
    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
        context_type[i] = get_table_entry( wine_shaping_table, pwcChars[i] );

    for (i = 0; i < cChars; i++)
    {
        if (context_type[i] == jtR && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtL && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)) && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xm;
        else if (context_type[i] == jtD && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else
            context_shape[i] = Xn;
    }

    /* Contextual Shaping */
    if (dirL > 0)
        char_index = glyph_index = 0;
    else
        char_index = glyph_index = cChars-1;

    while(char_index < cChars && char_index >= 0)
    {
        BOOL shaped = FALSE;

        if (psc->GSUB_Table)
        {
            INT nextIndex, offset = 0;
            INT prevCount = *pcGlyphs;

            /* Apply CCMP first */
            apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, "ccmp");

            if (prevCount != *pcGlyphs)
            {
                offset = *pcGlyphs - prevCount;
                if (dirL < 0)
                    glyph_index -= offset * dirL;
            }

            /* Apply the contextual feature */
            nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, contextual_features[context_shape[char_index]]);

            if (nextIndex > GSUB_E_NOGLYPH)
            {
                UpdateClusters(glyph_index, *pcGlyphs - prevCount, dirL, cChars, pwLogClust);
                char_index += dirL;
                if (!offset)
                    glyph_index = nextIndex;
                else
                {
                    offset = *pcGlyphs - prevCount;
                    glyph_index += dirL * (offset + 1);
                }
            }
            shaped = (nextIndex > GSUB_E_NOGLYPH);
        }

        if (!shaped)
        {
            if (context_shape[char_index] == Xn)
            {
                WORD newGlyph = pwOutGlyphs[glyph_index];
                if (pwcChars[char_index] >= FIRST_ARABIC_CHAR && pwcChars[char_index] <= LAST_ARABIC_CHAR)
                {
                    /* fall back to presentation form B */
                    WCHAR context_char = wine_shaping_forms[pwcChars[char_index] - FIRST_ARABIC_CHAR][context_shape[char_index]];
                    if (context_char != pwcChars[char_index] && GetGlyphIndicesW(hdc, &context_char, 1, &newGlyph, 0) != GDI_ERROR && newGlyph != 0x0000)
                        pwOutGlyphs[glyph_index] = newGlyph;
                }
            }
            char_index += dirL;
            glyph_index += dirL;
        }
    }

    HeapFree(GetProcessHeap(),0,context_shape);
    HeapFree(GetProcessHeap(),0,context_type);

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Arabic);
}

static int combining_lexical_Hebrew(WCHAR c)
{
    enum {Hebr_Norm=0, Hebr_DIAC, Hebr_CANT1, Hebr_CANT2, Hebr_CANT3, Hebr_CANT4, Hebr_CANT5, Hebr_CANT6, Hebr_CANT7, Hebr_CANT8, Hebr_CANT9, Hebr_CANT10, Hebr_DAGESH, Hebr_DOTABV, Hebr_HOLAM, Hebr_METEG, Hebr_PATAH, Hebr_QAMATS, Hebr_RAFE, Hebr_SHINSIN};

   switch(c)
    {
        case 0x05B0:
        case 0x05B1:
        case 0x05B2:
        case 0x05B3:
        case 0x05B4:
        case 0x05B5:
        case 0x05B6:
        case 0x05BB: return Hebr_DIAC;
        case 0x0599:
        case 0x05A1:
        case 0x05A9:
        case 0x05AE: return Hebr_CANT1;
        case 0x0597:
        case 0x05A8:
        case 0x05AC: return Hebr_CANT2;
        case 0x0592:
        case 0x0593:
        case 0x0594:
        case 0x0595:
        case 0x05A7:
        case 0x05AB: return Hebr_CANT3;
        case 0x0598:
        case 0x059C:
        case 0x059E:
        case 0x059F: return Hebr_CANT4;
        case 0x059D:
        case 0x05A0: return Hebr_CANT5;
        case 0x059B:
        case 0x05A5: return Hebr_CANT6;
        case 0x0591:
        case 0x05A3:
        case 0x05A6: return Hebr_CANT7;
        case 0x0596:
        case 0x05A4:
        case 0x05AA: return Hebr_CANT8;
        case 0x059A:
        case 0x05AD: return Hebr_CANT9;
        case 0x05AF: return Hebr_CANT10;
        case 0x05BC: return Hebr_DAGESH;
        case 0x05C4: return Hebr_DOTABV;
        case 0x05B9: return Hebr_HOLAM;
        case 0x05BD: return Hebr_METEG;
        case 0x05B7: return Hebr_PATAH;
        case 0x05B8: return Hebr_QAMATS;
        case 0x05BF: return Hebr_RAFE;
        case 0x05C1:
        case 0x05C2: return Hebr_SHINSIN;
        default: return Hebr_Norm;
    }
}

static void ContextualShape_Hebrew(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT dirL;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Hebrew);
}

/*
 * ContextualShape_Syriac
 */

static int combining_lexical_Syriac(WCHAR c)
{
    enum {Syriac_Norm=0, Syriac_DIAC1, Syriac_DIAC2, Syriac_DIAC3, Syriac_DIAC4, Syriac_DIAC5, Syriac_DIAC6, Syriac_DIAC7, Syriac_DIAC8, Syriac_DIAC9, Syriac_DIAC10, Syriac_DIAC11, Syriac_DIAC12, Syriac_DIAC13, Syriac_DIAC14, Syriac_DIAC15, Syriac_DIAC16, Syriac_DIAC17};

   switch(c)
    {
        case 0x730:
        case 0x733:
        case 0x736:
        case 0x73A:
        case 0x73D: return Syriac_DIAC1;
        case 0x731:
        case 0x734:
        case 0x737:
        case 0x73B:
        case 0x73E: return Syriac_DIAC2;
        case 0x740:
        case 0x749:
        case 0x74A: return Syriac_DIAC3;
        case 0x732:
        case 0x735:
        case 0x73F: return Syriac_DIAC4;
        case 0x738:
        case 0x739:
        case 0x73C: return Syriac_DIAC5;
        case 0x741:
        case 0x30A: return Syriac_DIAC6;
        case 0x742:
        case 0x325: return Syriac_DIAC7;
        case 0x747:
        case 0x303: return Syriac_DIAC8;
        case 0x748:
        case 0x32D:
        case 0x32E:
        case 0x330:
        case 0x331: return Syriac_DIAC9;
        case 0x308: return Syriac_DIAC10;
        case 0x304: return Syriac_DIAC11;
        case 0x307: return Syriac_DIAC12;
        case 0x323: return Syriac_DIAC13;
        case 0x743: return Syriac_DIAC14;
        case 0x744: return Syriac_DIAC15;
        case 0x745: return Syriac_DIAC16;
        case 0x746: return Syriac_DIAC17;
        default: return Syriac_Norm;
    }
}

#define ALAPH 0x710
#define DALATH 0x715
#define RISH 0x72A

static void ContextualShape_Syriac(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    CHAR *context_type;
    INT *context_shape;
    INT dirR, dirL;
    int i;
    int char_index;
    int glyph_index;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        dirR = 1;
        dirL = -1;
    }
    else
    {
        dirR = -1;
        dirL = 1;
    }

    load_ot_tables(hdc, psc);

    if (!psc->GSUB_Table)
        return;

    context_type = HeapAlloc(GetProcessHeap(),0,cChars);
    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
        context_type[i] = get_table_entry( wine_shaping_table, pwcChars[i] );

    for (i = 0; i < cChars; i++)
    {
        if (pwcChars[i] == ALAPH)
        {
            WCHAR rchar = neighbour_char(i,dirR,pwcChars,cChars);

            if (left_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)) && word_break_causing(neighbour_char(i,dirL,pwcChars,cChars)))
            context_shape[i] = Afj;
            else if ( rchar != DALATH && rchar != RISH &&
!left_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)) &&
word_break_causing(neighbour_char(i,dirL,pwcChars,cChars)))
            context_shape[i] = Afn;
            else if ( (rchar == DALATH || rchar == RISH) && word_break_causing(neighbour_char(i,dirL,pwcChars,cChars)))
            context_shape[i] = Afx;
            else
            context_shape[i] = Xn;
        }
        else if (context_type[i] == jtR &&
right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtL && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)) && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xm;
        else if (context_type[i] == jtD && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else
            context_shape[i] = Xn;
    }

    /* Contextual Shaping */
    if (dirL > 0)
        char_index = glyph_index = 0;
    else
        char_index = glyph_index = cChars-1;

    while(char_index < cChars && char_index >= 0)
    {
        INT nextIndex, offset = 0;
        INT prevCount = *pcGlyphs;

        /* Apply CCMP first */
        apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, "ccmp");

        if (prevCount != *pcGlyphs)
        {
            offset = *pcGlyphs - prevCount;
            if (dirL < 0)
                glyph_index -= offset * dirL;
        }

        /* Apply the contextual feature */
        nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, contextual_features[context_shape[char_index]]);
        if (nextIndex > GSUB_E_NOGLYPH)
        {
            UpdateClusters(nextIndex, *pcGlyphs - prevCount, dirL, cChars, pwLogClust);
            char_index += dirL;
            if (!offset)
                glyph_index = nextIndex;
            else
            {
                offset = *pcGlyphs - prevCount;
                glyph_index += dirL * (offset + 1);
            }
        }
        else
        {
            char_index += dirL;
            glyph_index += dirL;
        }
    }

    HeapFree(GetProcessHeap(),0,context_shape);
    HeapFree(GetProcessHeap(),0,context_type);

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Syriac);
}

static int combining_lexical_Thaana(WCHAR c)
{
    enum {Thaana_Norm=0, Thaana_FILI};

   switch(c)
    {
        case 0x7A6:
        case 0x7A7:
        case 0x7A8:
        case 0x7A9:
        case 0x7AA:
        case 0x7AB:
        case 0x7AC:
        case 0x7AD:
        case 0x7AE:
        case 0x7AF: return Thaana_FILI;
        default: return Thaana_Norm;
    }
}

static void ContextualShape_Thaana(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT dirL;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Thaana);
}

/*
 * ContextualShape_Phags_pa
 */

#define phags_pa_CANDRABINDU  0xA873
#define phags_pa_START 0xA840
#define phags_pa_END  0xA87F

static void ContextualShape_Phags_pa(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT *context_shape;
    INT dirR, dirL;
    int i;
    int char_index;
    int glyph_index;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        dirR = 1;
        dirL = -1;
    }
    else
    {
        dirR = -1;
        dirL = 1;
    }

    load_ot_tables(hdc, psc);

    if (!psc->GSUB_Table)
        return;

    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
    {
        if (pwcChars[i] >= phags_pa_START && pwcChars[i] <=  phags_pa_END)
        {
            WCHAR rchar = neighbour_char(i,dirR,pwcChars,cChars);
            WCHAR lchar = neighbour_char(i,dirL,pwcChars,cChars);
            BOOL jrchar = (rchar != phags_pa_CANDRABINDU && rchar >= phags_pa_START && rchar <=  phags_pa_END);
            BOOL jlchar = (lchar != phags_pa_CANDRABINDU && lchar >= phags_pa_START && lchar <=  phags_pa_END);

            if (jrchar && jlchar)
                context_shape[i] = Xm;
            else if (jrchar)
                context_shape[i] = Xr;
            else if (jlchar)
                context_shape[i] = Xl;
            else
                context_shape[i] = Xn;
        }
        else
            context_shape[i] = -1;
    }

    /* Contextual Shaping */
    if (dirL > 0)
        char_index = glyph_index = 0;
    else
        char_index = glyph_index = cChars-1;

    while(char_index < cChars && char_index >= 0)
    {
        if (context_shape[char_index] >= 0)
        {
            INT nextIndex;
            INT prevCount = *pcGlyphs;
            nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, contextual_features[context_shape[char_index]]);

            if (nextIndex > GSUB_E_NOGLYPH)
            {
                UpdateClusters(nextIndex, *pcGlyphs - prevCount, dirL, cChars, pwLogClust);
                glyph_index = nextIndex;
                char_index += dirL;
            }
            else
            {
                char_index += dirL;
                glyph_index += dirL;
            }
        }
        else
        {
            char_index += dirL;
            glyph_index += dirL;
        }
    }

    HeapFree(GetProcessHeap(),0,context_shape);
}

static int combining_lexical_Thai(WCHAR c)
{
    enum {Thai_Norm=0, Thai_ABOVE1, Thai_ABOVE2, Thai_ABOVE3, Thai_ABOVE4, Thai_BELOW1, Thai_BELOW2, Thai_AM};

   switch(c)
    {
        case 0xE31:
        case 0xE34:
        case 0xE35:
        case 0xE36:
        case 0xE37: return Thai_ABOVE1;
        case 0xE47:
        case 0xE4D: return Thai_ABOVE2;
        case 0xE48:
        case 0xE49:
        case 0xE4A:
        case 0xE4B: return Thai_ABOVE3;
        case 0xE4C:
        case 0xE4E: return Thai_ABOVE4;
        case 0xE38:
        case 0xE39: return Thai_BELOW1;
        case 0xE3A: return Thai_BELOW2;
        case 0xE33: return Thai_AM;
        default: return Thai_Norm;
    }
}

static void ContextualShape_Thai(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT dirL;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Thai);
}

static int combining_lexical_Lao(WCHAR c)
{
    enum {Lao_Norm=0, Lao_ABOVE1, Lao_ABOVE2, Lao_BELOW1, Lao_BELOW2, Lao_AM};

   switch(c)
    {
        case 0xEB1:
        case 0xEB4:
        case 0xEB5:
        case 0xEB6:
        case 0xEB7:
        case 0xEBB:
        case 0xECD: return Lao_ABOVE1;
        case 0xEC8:
        case 0xEC9:
        case 0xECA:
        case 0xECB:
        case 0xECC: return Lao_ABOVE2;
        case 0xEBC: return Lao_BELOW1;
        case 0xEB8:
        case 0xEB9: return Lao_BELOW2;
        case 0xEB3: return Lao_AM;
        default: return Lao_Norm;
    }
}

static void ContextualShape_Lao(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT dirL;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    mark_invalid_combinations(hdc, pwcChars, cChars, pwOutGlyphs, pcGlyphs, dirL, pwLogClust, combining_lexical_Lao);
}

static void ReplaceInsertChars(HDC hdc, INT cWalk, INT* pcChars, WCHAR *pwOutChars, const WCHAR *replacements)
{
    int i;

    /* Replace */
    pwOutChars[cWalk] = replacements[0];
    cWalk=cWalk+1;

    /* Insert */
    for (i = 1; i < 3 && replacements[i] != 0x0000; i++)
    {
        int j;
        for (j = *pcChars; j > cWalk; j--)
            pwOutChars[j] = pwOutChars[j-1];
        *pcChars= *pcChars+1;
        pwOutChars[cWalk] = replacements[i];
        cWalk = cWalk+1;
    }
}

static void DecomposeVowels(HDC hdc, WCHAR *pwOutChars, INT *pcChars, const VowelComponents vowels[], WORD* pwLogClust, INT cChars)
{
    int i;
    int cWalk;

    for (cWalk = 0; cWalk < *pcChars; cWalk++)
    {
        for (i = 0; vowels[i].base != 0x0; i++)
        {
            if (pwOutChars[cWalk] == vowels[i].base)
            {
                int o = 0;
                ReplaceInsertChars(hdc, cWalk, pcChars, pwOutChars, vowels[i].parts);
                if (vowels[i].parts[1]) { cWalk++; o++; }
                if (vowels[i].parts[2]) { cWalk++; o++; }
                UpdateClusters(cWalk, o, 1,  cChars,  pwLogClust);
                break;
            }
        }
    }
}

static void ComposeConsonants(HDC hdc, WCHAR *pwOutChars, INT *pcChars, const ConsonantComponents consonants[], WORD* pwLogClust)
{
    int i;
    int offset = 0;
    int cWalk;

    for (cWalk = 0; cWalk < *pcChars; cWalk++)
    {
        for (i = 0; consonants[i].output!= 0x0; i++)
        {
            int j;
            for (j = 0; j + cWalk < *pcChars && consonants[i].parts[j]!=0x0; j++)
                if (pwOutChars[cWalk+j] != consonants[i].parts[j])
                    break;

            if (consonants[i].parts[j]==0x0) /* matched all */
            {
                int k;
                j--;
                pwOutChars[cWalk] = consonants[i].output;
                for(k = cWalk+1; k < *pcChars - j; k++)
                    pwOutChars[k] = pwOutChars[k+j];
                *pcChars = *pcChars - j;
                for (k = j ; k > 0; k--)
                    pwLogClust[cWalk + k + offset] = pwLogClust[cWalk + offset];
                offset += j;
                for (k = cWalk + j + offset; k < *pcChars + offset; k++)
                    pwLogClust[k]--;
                break;
            }
        }
        cWalk++;
    }
}

static void Reorder_Ra_follows_base(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    if (s->ralf >= 0)
    {
        int j;
        WORD Ra = pwChar[s->start];
        WORD H = pwChar[s->start+1];

        TRACE("Doing reorder of Ra to %i\n",s->base);
        for (j = s->start; j < s->base-1; j++)
            pwChar[j] = pwChar[j+2];
        pwChar[s->base-1] = Ra;
        pwChar[s->base] = H;

        s->ralf = s->base-1;
        s->base -= 2;
    }
}

static void Reorder_Ra_follows_matra(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    if (s->ralf >= 0)
    {
        int j,loc;
        int stop = (s->blwf >=0)? s->blwf+1 : s->base;
        WORD Ra = pwChar[s->start];
        WORD H = pwChar[s->start+1];
        for (loc = s->end; loc > stop; loc--)
            if (lexical(pwChar[loc]) == lex_Matra_post || lexical(pwChar[loc]) == lex_Matra_below)
                break;

        TRACE("Doing reorder of Ra to %i\n",loc);
        for (j = s->start; j < loc-1; j++)
            pwChar[j] = pwChar[j+2];
        pwChar[loc-1] = Ra;
        pwChar[loc] = H;

        s->ralf = loc-1;
        s->base -= 2;
        if (s->blwf >= 0) s->blwf -= 2;
        if (s->pref >= 0) s->pref -= 2;
    }
}

static void Reorder_Ra_follows_syllable(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    if (s->ralf >= 0)
    {
        int j;
        WORD Ra = pwChar[s->start];
        WORD H = pwChar[s->start+1];

        TRACE("Doing reorder of Ra to %i\n",s->end-1);
        for (j = s->start; j < s->end-1; j++)
            pwChar[j] = pwChar[j+2];
        pwChar[s->end-1] = Ra;
        pwChar[s->end] = H;

        s->ralf = s->end-1;
        s->base -= 2;
        if (s->blwf >= 0) s->blwf -= 2;
        if (s->pref >= 0) s->pref -= 2;
    }
}

static void Reorder_Matra_precede_base(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    int i;

    /* reorder Matras */
    if (s->end > s->base)
    {
        for (i = 1; i <= s->end-s->base; i++)
        {
            if (lexical(pwChar[s->base+i]) == lex_Matra_pre)
            {
                int j;
                WCHAR c = pwChar[s->base+i];
                TRACE("Doing reorder of %x %x\n",c,pwChar[s->base]);
                for (j = s->base+i; j > s->base; j--)
                    pwChar[j] = pwChar[j-1];
                pwChar[s->base] = c;

                if (s->ralf >= s->base) s->ralf++;
                if (s->blwf >= s->base) s->blwf++;
                if (s->pref >= s->base) s->pref++;
                s->base ++;
            }
        }
    }
}

static void Reorder_Matra_precede_syllable(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    int i;

    /* reorder Matras */
    if (s->end > s->base)
    {
        for (i = 1; i <= s->end-s->base; i++)
        {
            if (lexical(pwChar[s->base+i]) == lex_Matra_pre)
            {
                int j;
                WCHAR c = pwChar[s->base+i];
                TRACE("Doing reorder of %x to %i\n",c,s->start);
                for (j = s->base+i; j > s->start; j--)
                    pwChar[j] = pwChar[j-1];
                pwChar[s->start] = c;

                if (s->ralf >= 0) s->ralf++;
                if (s->blwf >= 0) s->blwf++;
                if (s->pref >= 0) s->pref++;
                s->base ++;
            }
        }
    }
}

static void SecondReorder_Blwf_follows_matra(LPWSTR pwChar, IndicSyllable *s, WORD *glyphs, IndicSyllable *g, lexical_function lexical)
{
    if (s->blwf >= 0 && g->blwf > g->base)
    {
        int j,loc;
        int g_offset;
        for (loc = s->end; loc > s->blwf; loc--)
            if (lexical(pwChar[loc]) == lex_Matra_below || lexical(pwChar[loc]) == lex_Matra_above || lexical(pwChar[loc]) == lex_Matra_post)
                break;

        g_offset = (loc - s->blwf) - 1;

        if (loc != s->blwf)
        {
            WORD blwf = glyphs[g->blwf];
            TRACE("Doing reorder of Below-base to %i (glyph offset %i)\n",loc,g_offset);
            /* do not care about the pwChar array anymore, just the glyphs */
            for (j = 0; j < g_offset; j++)
                glyphs[g->blwf + j] = glyphs[g->blwf + j + 1];
            glyphs[g->blwf + g_offset] = blwf;
        }
    }
}

static void SecondReorder_Matra_precede_base(LPWSTR pwChar, IndicSyllable *s, WORD *glyphs, IndicSyllable *g, lexical_function lexical)
{
    int i;

    /* reorder previously moved Matras to correct position*/
    for (i = s->start; i < s->base; i++)
    {
        if (lexical(pwChar[i]) == lex_Matra_pre)
        {
            int j;
            int g_start = g->start + i - s->start;
            if (g_start < g->base -1 )
            {
                WCHAR og = glyphs[g_start];
                TRACE("Doing reorder of matra from %i to %i\n",g_start,g->base);
                for (j = g_start; j < g->base-1; j++)
                    glyphs[j] = glyphs[j+1];
                glyphs[g->base-1] = og;
            }
        }
    }
}

static void SecondReorder_Pref_precede_base(LPWSTR pwChar, IndicSyllable *s, WORD *glyphs, IndicSyllable *g, lexical_function lexical)
{
    if (s->pref >= 0 && g->pref > g->base)
    {
        int j;
        WCHAR og = glyphs[g->pref];
        TRACE("Doing reorder of pref from %i to %i\n",g->pref,g->base);
        for (j = g->pref; j > g->base; j--)
            glyphs[j] = glyphs[j-1];
        glyphs[g->base] = og;
    }
}

static void Reorder_Like_Sinhala(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    Reorder_Ra_follows_base(pwChar, s, lexical);
    Reorder_Matra_precede_base(pwChar, s, lexical);
}

static void Reorder_Like_Devanagari(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    Reorder_Ra_follows_matra(pwChar, s, lexical);
    Reorder_Matra_precede_syllable(pwChar, s, lexical);
}

static void Reorder_Like_Bengali(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    Reorder_Ra_follows_base(pwChar, s, lexical);
    Reorder_Matra_precede_syllable(pwChar, s, lexical);
}

static void Reorder_Like_Kannada(LPWSTR pwChar, IndicSyllable *s, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    Reorder_Ra_follows_syllable(pwChar, s, lexical);
    Reorder_Matra_precede_syllable(pwChar, s, lexical);
}

static void SecondReorder_Like_Telugu(LPWSTR pwChar, IndicSyllable *s, WORD* pwGlyphs, IndicSyllable *g, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    TRACE("Glyphs (%i..%i..%i)\n",g->start,g->base,g->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    SecondReorder_Blwf_follows_matra(pwChar, s, pwGlyphs, g, lexical);
}

static void SecondReorder_Like_Tamil(LPWSTR pwChar, IndicSyllable *s, WORD* pwGlyphs, IndicSyllable *g, lexical_function lexical)
{
    TRACE("Syllable (%i..%i..%i)\n",s->start,s->base,s->end);
    TRACE("Glyphs (%i..%i..%i)\n",g->start,g->base,g->end);
    if (s->start == s->base && s->base == s->end)  return;
    if (lexical(pwChar[s->base]) == lex_Vowel) return;

    SecondReorder_Matra_precede_base(pwChar, s, pwGlyphs, g, lexical);
    SecondReorder_Pref_precede_base(pwChar, s, pwGlyphs, g, lexical);
}


static inline void shift_syllable_glyph_indexs(IndicSyllable *glyph_index, INT index, INT shift)
{
    if (shift == 0)
        return;

    if (glyph_index->start > index)
        glyph_index->start += shift;
    if (glyph_index->base > index)
        glyph_index->base+= shift;
    if (glyph_index->end > index)
        glyph_index->end+= shift;
    if (glyph_index->ralf > index)
        glyph_index->ralf+= shift;
    if (glyph_index->blwf > index)
        glyph_index->blwf+= shift;
    if (glyph_index->pref > index)
        glyph_index->pref+= shift;
}

static void Apply_Indic_BasicForm(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwChars, INT cChars, IndicSyllable *syllable, WORD *pwOutGlyphs, INT* pcGlyphs, WORD *pwLogClust, lexical_function lexical, IndicSyllable *glyph_index, LoadedFeature *feature )
{
    int index = glyph_index->start;

    if (!feature)
        return;

    while(index <= glyph_index->end)
    {
            INT nextIndex;
            INT prevCount = *pcGlyphs;
            nextIndex = GSUB_apply_feature_all_lookups(psc->GSUB_Table, feature, pwOutGlyphs, index, 1, pcGlyphs);
            if (nextIndex > GSUB_E_NOGLYPH)
            {
                UpdateClusters(nextIndex, *pcGlyphs - prevCount, 1, cChars, pwLogClust);
                shift_syllable_glyph_indexs(glyph_index,index,*pcGlyphs - prevCount);
                index = nextIndex;
            }
            else
                index++;
    }
}

static inline INT find_consonant_halant(WCHAR* pwChars, INT index, INT end, lexical_function lexical)
{
    int i = 0;
    while (i + index < end - 1 && !(is_consonant(lexical(pwChars[index+i])) && (lexical(pwChars[index+i+1]) == lex_Halant || (index + i < end - 2 && lexical(pwChars[index+i+1]) == lex_Nukta && lexical(pwChars[index+i+2] == lex_Halant)))))
        i++;
    if (index + i <= end-1)
        return index + i;
    else
        return -1;
}

static void Apply_Indic_PreBase(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwChars, INT cChars, IndicSyllable *syllable, WORD *pwOutGlyphs, INT* pcGlyphs, WORD *pwLogClust, lexical_function lexical, IndicSyllable *glyph_index, const char* feature)
{
    INT index, nextIndex;
    INT count,g_offset;

    count = syllable->base - syllable->start;

    g_offset = 0;
    index = find_consonant_halant(&pwChars[syllable->start], 0, count, lexical);
    while (index >= 0 && index + g_offset < (glyph_index->base - glyph_index->start))
    {
        INT prevCount = *pcGlyphs;
        nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, index+glyph_index->start+g_offset, 1, pcGlyphs, feature);
        if (nextIndex > GSUB_E_NOGLYPH)
        {
            UpdateClusters(nextIndex, *pcGlyphs - prevCount, 1, cChars, pwLogClust);
            shift_syllable_glyph_indexs(glyph_index, index + glyph_index->start + g_offset, (*pcGlyphs - prevCount));
            g_offset += (*pcGlyphs - prevCount);
        }

        index+=2;
        index = find_consonant_halant(&pwChars[syllable->start], index, count, lexical);
    }
}

static void Apply_Indic_Rphf(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwChars, INT cChars, IndicSyllable *syllable, WORD *pwOutGlyphs, INT* pcGlyphs, WORD *pwLogClust, lexical_function lexical, IndicSyllable *glyph_index)
{
    INT nextIndex;
    INT prevCount = *pcGlyphs;

    if (syllable->ralf >= 0)
    {
        nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index->ralf, 1, pcGlyphs, "rphf");
        if (nextIndex > GSUB_E_NOGLYPH)
        {
            UpdateClusters(nextIndex, *pcGlyphs - prevCount, 1, cChars, pwLogClust);
            shift_syllable_glyph_indexs(glyph_index,glyph_index->ralf,*pcGlyphs - prevCount);
        }
    }
}

static inline INT find_halant_consonant(WCHAR* pwChars, INT index, INT end, lexical_function lexical)
{
    int i = 0;
    while (index + i < end-1 && !(lexical(pwChars[index+i]) == lex_Halant &&
             ((index + i < end-2 && lexical(pwChars[index+i]) == lex_Nukta && is_consonant(lexical(pwChars[index+i+1]))) ||
              is_consonant(lexical(pwChars[index+i+1])))))
        i++;
    if (index + i <= end-1)
        return index+i;
    else
        return -1;
}

static void Apply_Indic_PostBase(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwChars, INT cChars, IndicSyllable *syllable, WORD *pwOutGlyphs, INT* pcGlyphs, WORD *pwLogClust, lexical_function lexical, IndicSyllable *glyph_index, BOOL modern, const char* feat)
{
    INT index, nextIndex;
    INT count, g_offset=0;
    INT ralf = syllable->ralf;

    count = syllable->end - syllable->base;

    index = find_halant_consonant(&pwChars[syllable->base], 0, count, lexical);

    while (index >= 0)
    {
        INT prevCount = *pcGlyphs;
        if (ralf >=0 && ralf < index)
        {
            g_offset--;
            ralf = -1;
        }

        if (!modern)
        {
            WORD g = pwOutGlyphs[index+glyph_index->base+g_offset];
            pwOutGlyphs[index+glyph_index->base+g_offset] = pwOutGlyphs[index+glyph_index->base+g_offset+1];
            pwOutGlyphs[index+glyph_index->base+g_offset+1] = g;
        }

        nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, index+glyph_index->base+g_offset, 1, pcGlyphs, feat);
        if (nextIndex > GSUB_E_NOGLYPH)
        {
            UpdateClusters(nextIndex, *pcGlyphs - prevCount, 1, cChars, pwLogClust);
            shift_syllable_glyph_indexs(glyph_index,index+glyph_index->start+g_offset, (*pcGlyphs - prevCount));
            g_offset += (*pcGlyphs - prevCount);
        }
        else if (!modern)
        {
            WORD g = pwOutGlyphs[index+glyph_index->base+g_offset];
            pwOutGlyphs[index+glyph_index->base+g_offset] = pwOutGlyphs[index+glyph_index->base+g_offset+1];
            pwOutGlyphs[index+glyph_index->base+g_offset+1] = g;
        }

        index+=2;
        index = find_halant_consonant(&pwChars[syllable->base], index, count, lexical);
    }
}

static void ShapeIndicSyllables(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwChars, INT cChars, IndicSyllable *syllables, INT syllable_count, WORD *pwOutGlyphs, INT* pcGlyphs, WORD *pwLogClust, lexical_function lexical, second_reorder_function second_reorder, BOOL modern)
{
    int c;
    int overall_shift = 0;
    LoadedFeature *locl = (modern)?load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "locl"):NULL;
    LoadedFeature *nukt = load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "nukt");
    LoadedFeature *akhn = load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "akhn");
    LoadedFeature *rkrf = (modern)?load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "rkrf"):NULL;
    LoadedFeature *pstf = load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "pstf");
    LoadedFeature *vatu = (!rkrf)?load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "vatu"):NULL;
    LoadedFeature *cjct = (modern)?load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "cjct"):NULL;
    BOOL rphf = (load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "rphf") != NULL);
    BOOL pref = (load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "pref") != NULL);
    BOOL blwf = (load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "blwf") != NULL);
    BOOL half = (load_OT_feature(hdc, psa, psc, FEATURE_GSUB_TABLE, "half") != NULL);
    IndicSyllable glyph_indexs;

    for (c = 0; c < syllable_count; c++)
    {
        int old_end;
        memcpy(&glyph_indexs, &syllables[c], sizeof(IndicSyllable));
        shift_syllable_glyph_indexs(&glyph_indexs, -1, overall_shift);
        old_end = glyph_indexs.end;

        if (locl)
        {
            TRACE("applying feature locl\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, locl);
        }
        if (nukt)
        {
            TRACE("applying feature nukt\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, nukt);
        }
        if (akhn)
        {
            TRACE("applying feature akhn\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, akhn);
        }

        if (rphf)
            Apply_Indic_Rphf(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs);
        if (rkrf)
        {
            TRACE("applying feature rkrf\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, rkrf);
        }
        if (pref)
            Apply_Indic_PostBase(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, modern, "pref");
        if (blwf)
        {
            if (!modern)
                Apply_Indic_PreBase(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, "blwf");

            Apply_Indic_PostBase(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, modern, "blwf");

        }
        if (half)
            Apply_Indic_PreBase(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, "half");
        if (pstf)
        {
            TRACE("applying feature pstf\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, pstf);
        }
        if (vatu)
        {
            TRACE("applying feature vatu\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, vatu);
        }
        if (cjct)
        {
            TRACE("applying feature cjct\n");
            Apply_Indic_BasicForm(hdc, psc, psa, pwChars, cChars, &syllables[c], pwOutGlyphs, pcGlyphs, pwLogClust, lexical, &glyph_indexs, cjct);
        }

        if (second_reorder)
            second_reorder(pwChars, &syllables[c], pwOutGlyphs, &glyph_indexs, lexical);

        overall_shift += glyph_indexs.end - old_end;
    }
}

static inline int unicode_lex(WCHAR c)
{
    int type;

    if (!c) return lex_Generic;
    if (c == 0x200D) return lex_ZWJ;
    if (c == 0x200C) return lex_ZWNJ;
    if (c == 0x00A0) return lex_NBSP;

    type = get_table_entry( indic_syllabic_table, c );

    if ((type & 0x00ff) != 0x0007)  type = type & 0x00ff;

    switch( type )
    {
        case 0x0d07: /* Unknown */
        case 0x0e07: /* Unknown */
        default: return lex_Generic;
        case 0x0001:
        case 0x0002:
        case 0x0011:
        case 0x0012:
        case 0x0013:
        case 0x0014: return lex_Modifier;
        case 0x0003:
        case 0x0009:
        case 0x000a:
        case 0x000b:
        case 0x000d:
        case 0x000e:
        case 0x000f:
        case 0x0010: return lex_Consonant;
        case 0x0004: return lex_Nukta;
        case 0x0005: return lex_Halant;
        case 0x0006:
        case 0x0008: return lex_Vowel;
        case 0x0007:
        case 0x0107: return lex_Matra_post;
        case 0x0207:
        case 0x0307: return lex_Matra_pre;
        case 0x0807:
        case 0x0907:
        case 0x0a07:
        case 0x0b07:
        case 0x0c07:
        case 0x0407: return lex_Composed_Vowel;
        case 0x0507: return lex_Matra_above;
        case 0x0607: return lex_Matra_below;
        case 0x000c:
        case 0x0015: return lex_Ra;
    };
}

static int sinhala_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0DDA:
        case 0x0DDD:
        case 0x0DDC:
        case 0x0DDE: return lex_Matra_post;
        default:
            return unicode_lex(c);
    }
}

static const VowelComponents Sinhala_vowels[] = {
            {0x0DDA, {0x0DD9,0x0DDA,0x0}},
            {0x0DDC, {0x0DD9,0x0DDC,0x0}},
            {0x0DDD, {0x0DD9,0x0DDD,0x0}},
            {0x0DDE, {0x0DD9,0x0DDE,0x0}},
            {0x0000, {0x0000,0x0000,0x0}}};

static void ContextualShape_Sinhala(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    int i;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR) * (cChars * 3));

    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1:  Decompose multi part vowels */
    DecomposeVowels(hdc, input,  &cCount, Sinhala_vowels, pwLogClust, cChars);

    TRACE("New double vowel expanded string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2:  Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, sinhala_lex, Reorder_Like_Sinhala, TRUE);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));

    /* Step 3:  Strip dangling joiners */
    for (i = 0; i < cCount; i++)
    {
        if ((input[i] == 0x200D || input[i] == 0x200C) &&
            (i == 0 || input[i-1] == 0x0020 || i == cCount-1 || input[i+1] == 0x0020))
            input[i] = 0x0020;
    }

    /* Step 4: Base Form application to syllables */
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, sinhala_lex, NULL, TRUE);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int devanagari_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0930: return lex_Ra;
        default:
            return unicode_lex(c);
    }
}

static const ConsonantComponents Devanagari_consonants[] ={
    {{0x0928, 0x093C, 0x00000}, 0x0929},
    {{0x0930, 0x093C, 0x00000}, 0x0931},
    {{0x0933, 0x093C, 0x00000}, 0x0934},
    {{0x0915, 0x093C, 0x00000}, 0x0958},
    {{0x0916, 0x093C, 0x00000}, 0x0959},
    {{0x0917, 0x093C, 0x00000}, 0x095A},
    {{0x091C, 0x093C, 0x00000}, 0x095B},
    {{0x0921, 0x093C, 0x00000}, 0x095C},
    {{0x0922, 0x093C, 0x00000}, 0x095D},
    {{0x092B, 0x093C, 0x00000}, 0x095E},
    {{0x092F, 0x093C, 0x00000}, 0x095F}};

static void ContextualShape_Devanagari(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, cChars * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Compose Consonant and Nukta */
    ComposeConsonants(hdc, input, &cCount, Devanagari_consonants, pwLogClust);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, devanagari_lex, Reorder_Like_Devanagari, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, devanagari_lex, NULL, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int bengali_lex(WCHAR c)
{
    switch (c)
    {
        case 0x09B0: return lex_Ra;
        default:
            return unicode_lex(c);
    }
}

static const VowelComponents Bengali_vowels[] = {
            {0x09CB, {0x09C7,0x09BE,0x0000}},
            {0x09CC, {0x09C7,0x09D7,0x0000}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static const ConsonantComponents Bengali_consonants[] = {
            {{0x09A4,0x09CD,0x200D}, 0x09CE},
            {{0x09A1,0x09BC,0x0000}, 0x09DC},
            {{0x09A2,0x09BC,0x0000}, 0x09DD},
            {{0x09AF,0x09BC,0x0000}, 0x09DF},
            {{0x0000,0x0000,0x0000}, 0x0000}};

static void ContextualShape_Bengali(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars * 2) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels and Compose Consonants */
    DecomposeVowels(hdc, input,  &cCount, Bengali_vowels, pwLogClust, cChars);
    ComposeConsonants(hdc, input, &cCount, Bengali_consonants, pwLogClust);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, bengali_lex, Reorder_Like_Bengali, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Initial form is only applied to the beginning of words */
    for (cCount = cCount - 1 ; cCount >= 0; cCount --)
    {
        if (cCount == 0 || input[cCount] == 0x0020) /* space */
        {
            int index = cCount;
            int gCount = 1;
            if (index > 0) index++;

            apply_GSUB_feature_to_glyph(hdc, psa, psc, &pwOutGlyphs[index], 0, 1, &gCount, "init");
        }
    }

    /* Step 4: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, bengali_lex, NULL, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int gurmukhi_lex(WCHAR c)
{
    if (c == 0x0A71)
        return lex_Modifier;
    else
        return unicode_lex(c);
}

static const ConsonantComponents Gurmukhi_consonants[] = {
            {{0x0A32,0x0A3C,0x0000}, 0x0A33},
            {{0x0A16,0x0A3C,0x0000}, 0x0A59},
            {{0x0A17,0x0A3C,0x0000}, 0x0A5A},
            {{0x0A1C,0x0A3C,0x0000}, 0x0A5B},
            {{0x0A2B,0x0A3C,0x0000}, 0x0A5E},
            {{0x0000,0x0000,0x0000}, 0x0000}};

static void ContextualShape_Gurmukhi(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, cChars * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Compose Consonants */
    ComposeConsonants(hdc, input, &cCount, Gurmukhi_consonants, pwLogClust);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, gurmukhi_lex, Reorder_Like_Bengali, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, gurmukhi_lex, NULL, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int gujarati_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0AB0: return lex_Ra;
        default:
            return unicode_lex(c);
    }
}

static void ContextualShape_Gujarati(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, cChars * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, gujarati_lex, Reorder_Like_Devanagari, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 2: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, gujarati_lex, NULL, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int oriya_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0B30: return lex_Ra;
        default:
            return unicode_lex(c);
    }
}

static const VowelComponents Oriya_vowels[] = {
            {0x0B48, {0x0B47,0x0B56,0x0000}},
            {0x0B4B, {0x0B47,0x0B3E,0x0000}},
            {0x0B4C, {0x0B47,0x0B57,0x0000}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static const ConsonantComponents Oriya_consonants[] = {
            {{0x0B21,0x0B3C,0x0000}, 0x0B5C},
            {{0x0B22,0x0B3C,0x0000}, 0x0B5D},
            {{0x0000,0x0000,0x0000}, 0x0000}};

static void ContextualShape_Oriya(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars*2) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels and Compose Consonants */
    DecomposeVowels(hdc, input,  &cCount, Oriya_vowels, pwLogClust, cChars);
    ComposeConsonants(hdc, input, &cCount, Oriya_consonants, pwLogClust);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, oriya_lex, Reorder_Like_Bengali, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, oriya_lex, NULL, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int tamil_lex(WCHAR c)
{
    return unicode_lex(c);
}

static const VowelComponents Tamil_vowels[] = {
            {0x0BCA, {0x0BC6,0x0BBE,0x0000}},
            {0x0BCB, {0x0BC7,0x0BBE,0x0000}},
            {0x0BCB, {0x0BC6,0x0BD7,0x0000}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static const ConsonantComponents Tamil_consonants[] = {
            {{0x0B92,0x0BD7,0x0000}, 0x0B94},
            {{0x0000,0x0000,0x0000}, 0x0000}};

static void ContextualShape_Tamil(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars*2) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels and Compose Consonants */
    DecomposeVowels(hdc, input,  &cCount, Tamil_vowels, pwLogClust, cChars);
    ComposeConsonants(hdc, input, &cCount, Tamil_consonants, pwLogClust);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, tamil_lex, Reorder_Like_Bengali, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, tamil_lex, SecondReorder_Like_Tamil, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int telugu_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0C43:
        case 0x0C44: return lex_Modifier;
        default:
            return unicode_lex(c);
    }
}

static const VowelComponents Telugu_vowels[] = {
            {0x0C48, {0x0C46,0x0C56,0x0000}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static void ContextualShape_Telugu(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars*2) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels */
    DecomposeVowels(hdc, input,  &cCount, Telugu_vowels, pwLogClust, cChars);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, telugu_lex, Reorder_Like_Bengali, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, telugu_lex, SecondReorder_Like_Telugu, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int kannada_lex(WCHAR c)
{
    switch (c)
    {
        case 0x0CB0: return lex_Ra;
        default:
            return unicode_lex(c);
    }
}

static const VowelComponents Kannada_vowels[] = {
            {0x0CC0, {0x0CBF,0x0CD5,0x0000}},
            {0x0CC7, {0x0CC6,0x0CD5,0x0000}},
            {0x0CC8, {0x0CC6,0x0CD6,0x0000}},
            {0x0CCA, {0x0CC6,0x0CC2,0x0000}},
            {0x0CCB, {0x0CC6,0x0CC2,0x0CD5}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static void ContextualShape_Kannada(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars*3) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels */
    DecomposeVowels(hdc, input,  &cCount, Kannada_vowels, pwLogClust, cChars);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, kannada_lex, Reorder_Like_Kannada, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, kannada_lex, SecondReorder_Like_Telugu, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int malayalam_lex(WCHAR c)
{
    return unicode_lex(c);
}

static const VowelComponents Malayalam_vowels[] = {
            {0x0D4A, {0x0D46,0x0D3E,0x0000}},
            {0x0D4B, {0x0D47,0x0D3E,0x0000}},
            {0x0D4C, {0x0D46,0x0D57,0x0000}},
            {0x0000, {0x0000,0x0000,0x0000}}};

static void ContextualShape_Malayalam(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;
    BOOL modern = get_GSUB_Indic2(psa, psc);

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, (cChars*2) * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Decompose Vowels */
    DecomposeVowels(hdc, input,  &cCount, Malayalam_vowels, pwLogClust, cChars);
    TRACE("New composed string %s (%i)\n",debugstr_wn(input,cCount),cCount);

    /* Step 2: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, malayalam_lex, Reorder_Like_Devanagari, modern);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 3: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, malayalam_lex, SecondReorder_Like_Tamil, modern);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static int khmer_lex(WCHAR c)
{
    return unicode_lex(c);
}

static void ContextualShape_Khmer(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    int cCount = cChars;
    WCHAR *input;
    IndicSyllable *syllables = NULL;
    int syllable_count = 0;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    input = HeapAlloc(GetProcessHeap(), 0, cChars * sizeof(WCHAR));
    memcpy(input, pwcChars, cChars * sizeof(WCHAR));

    /* Step 1: Reorder within Syllables */
    Indic_ReorderCharacters( hdc, psa, psc, input, cCount, &syllables, &syllable_count, khmer_lex, Reorder_Like_Devanagari, FALSE);
    TRACE("reordered string %s\n",debugstr_wn(input,cCount));
    GetGlyphIndicesW(hdc, input, cCount, pwOutGlyphs, 0);
    *pcGlyphs = cCount;

    /* Step 2: Base Form application to syllables */
    ShapeIndicSyllables(hdc, psc, psa, input, cChars, syllables, syllable_count, pwOutGlyphs, pcGlyphs, pwLogClust, khmer_lex, NULL, FALSE);

    HeapFree(GetProcessHeap(),0,input);
    HeapFree(GetProcessHeap(),0,syllables);
}

static inline BOOL mongolian_wordbreak(WCHAR chr)
{
    return ((chr == 0x0020) || (chr == 0x200C) || (chr == 0x202F) || (chr == 0x180E) || (chr == 0x1800) || (chr == 0x1802) || (chr == 0x1803) || (chr == 0x1805) || (chr == 0x1808) || (chr == 0x1809) || (chr == 0x1807));
}

static void ContextualShape_Mongolian(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    INT *context_shape;
    INT dirL;
    int i;
    int char_index;
    int glyph_index;

    if (*pcGlyphs != cChars)
    {
        ERR("Number of Glyphs and Chars need to match at the beginning\n");
        return;
    }

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    if (!psc->GSUB_Table)
        return;

    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
    {
        if (i == 0 || mongolian_wordbreak(pwcChars[i-1]))
        {
            if ((i == cChars-1) || mongolian_wordbreak(pwcChars[i+1]))
                context_shape[i] = Xn;
            else
                context_shape[i] = Xl;
        }
        else
        {
            if ((i == cChars-1) || mongolian_wordbreak(pwcChars[i+1]))
                context_shape[i] = Xr;
            else
                context_shape[i] = Xm;
        }
    }

    /* Contextual Shaping */
    if (dirL > 0)
        char_index = glyph_index = 0;
    else
        char_index = glyph_index = cChars-1;

    while(char_index < cChars && char_index >= 0)
    {
        INT nextIndex;
        INT prevCount = *pcGlyphs;
        nextIndex = apply_GSUB_feature_to_glyph(hdc, psa, psc, pwOutGlyphs, glyph_index, dirL, pcGlyphs, contextual_features[context_shape[char_index]]);

        if (nextIndex > GSUB_E_NOGLYPH)
        {
            UpdateClusters(nextIndex, *pcGlyphs - prevCount, dirL, cChars, pwLogClust);
            glyph_index = nextIndex;
            char_index += dirL;
        }
        else
        {
            char_index += dirL;
            glyph_index += dirL;
        }
    }

    HeapFree(GetProcessHeap(),0,context_shape);
}

static void ShapeCharGlyphProp_Default( ScriptCache* psc, SCRIPT_ANALYSIS* psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD* pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP* pGlyphProp)
{
    int i,k;

    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            continue;

        if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_BLANK;
            pCharProp[char_index[0]].fCanGlyphAlone = 1;
        }
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_CHARACTER;
    }

    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);
}

static void ShapeCharGlyphProp_Latin( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    int i;

    ShapeCharGlyphProp_Default( psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp);

    for (i = 0; i < cGlyphs; i++)
        if (pGlyphProp[i].sva.fZeroWidth)
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
}

static void ShapeCharGlyphProp_Control( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    int i;
    for (i = 0; i < cGlyphs; i++)
    {
        pGlyphProp[i].sva.fClusterStart = 1;
        pGlyphProp[i].sva.fDiacritic = 0;
        pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_BLANK;

        if (pwGlyphs[i] == psc->sfp.wgDefault)
            pGlyphProp[i].sva.fZeroWidth = 0;
        else
            pGlyphProp[i].sva.fZeroWidth = 1;
    }
}

static void ShapeCharGlyphProp_Arabic( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    int i,k;
    int initGlyph, finaGlyph;
    INT dirR, dirL;
    BYTE *spaces;

    spaces = HeapAlloc(GetProcessHeap(),0,cGlyphs);
    memset(spaces,0,cGlyphs);

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        initGlyph = cGlyphs-1;
        finaGlyph = 0;
        dirR = 1;
        dirL = -1;
    }
    else
    {
        initGlyph = 0;
        finaGlyph = cGlyphs-1;
        dirR = -1;
        dirL = 1;
    }

    for (i = 0; i < cGlyphs; i++)
    {
        for (k = 0; k < cChars; k++)
            if (pwLogClust[k] == i)
            {
                if (pwcChars[k] == 0x0020)
                    spaces[i] = 1;
            }
    }

    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;
        BOOL isInit, isFinal;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        isInit = (i == initGlyph || (i+dirR > 0 && i+dirR < cGlyphs && spaces[i+dirR]));
        isFinal = (i == finaGlyph || (i+dirL > 0 && i+dirL < cGlyphs && spaces[i+dirL]));

        if (char_count == 0)
            continue;

        if (char_count == 1)
        {
            if (pwcChars[char_index[0]] == 0x0020)  /* space */
            {
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_BLANK;
                pCharProp[char_index[0]].fCanGlyphAlone = 1;
            }
            else if (pwcChars[char_index[0]] == 0x0640)  /* kashida */
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_KASHIDA;
            else if (pwcChars[char_index[0]] == 0x0633)  /* SEEN */
            {
                if (!isInit && !isFinal)
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_SEEN_M;
                else if (isInit)
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_SEEN;
                else
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
            }
            else if (!isInit)
            {
                if (pwcChars[char_index[0]] == 0x0628 ) /* BA */
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_BA;
                else if (pwcChars[char_index[0]] == 0x0631 ) /* RA */
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_RA;
                else if (pwcChars[char_index[0]] == 0x0647 ) /* HA */
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_HA;
                else if ((pwcChars[char_index[0]] == 0x0627 || pwcChars[char_index[0]] == 0x0625 || pwcChars[char_index[0]] == 0x0623 || pwcChars[char_index[0]] == 0x0622) ) /* alef-like */
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_ALEF;
                else
                    pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
            }
            else if (!isInit && !isFinal)
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_NORMAL;
            else
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
        }
        else if (char_count == 2)
        {
            if ((pwcChars[char_index[0]] == 0x0628 && pwcChars[char_index[1]]== 0x0631) ||  (pwcChars[char_index[0]] == 0x0631 && pwcChars[char_index[1]]== 0x0628)) /* BA+RA */
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_BARA;
            else if (!isInit)
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_NORMAL;
            else
                pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
        }
        else if (!isInit && !isFinal)
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_ARABIC_NORMAL;
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
    }

    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);
    HeapFree(GetProcessHeap(),0,spaces);
}

static void ShapeCharGlyphProp_Hebrew( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    int i,k;

    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
        else
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_CHARACTER;
            if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
                pCharProp[char_index[0]].fCanGlyphAlone = 1;
        }
    }

    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);
}

static void ShapeCharGlyphProp_Thai( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    int i;
    int finaGlyph;
    INT dirL;

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        finaGlyph = 0;
        dirL = -1;
    }
    else
    {
        finaGlyph = cGlyphs-1;
        dirL = 1;
    }

    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);

    for (i = 0; i < cGlyphs; i++)
    {
        int k;
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (i == finaGlyph)
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_CHARACTER;

        if (char_count == 0)
            continue;

        if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
            pCharProp[char_index[0]].fCanGlyphAlone = 1;

        /* handle Thai SARA AM (U+0E33) differently than GDEF */
        if (char_count == 1 && pwcChars[char_index[0]] == 0x0e33)
            pGlyphProp[i].sva.fClusterStart = 0;
    }

    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);

    /* Do not allow justification between marks and their base */
    for (i = 0; i < cGlyphs; i++)
    {
        if (!pGlyphProp[i].sva.fClusterStart)
            pGlyphProp[i-dirL].sva.uJustification = SCRIPT_JUSTIFY_NONE;
    }
}

static void ShapeCharGlyphProp_None( HDC hdc, ScriptCache* psc, SCRIPT_ANALYSIS* psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD* pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP* pGlyphProp)
{
    int i,k;

    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            continue;

        if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_CHARACTER;
            pCharProp[char_index[0]].fCanGlyphAlone = 1;
        }
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
    }
    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);
}

static void ShapeCharGlyphProp_Tibet( HDC hdc, ScriptCache* psc, SCRIPT_ANALYSIS* psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD* pwLogClust, SCRIPT_CHARPROP* pCharProp, SCRIPT_GLYPHPROP* pGlyphProp)
{
    int i,k;

    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (char_count == 0)
            continue;

        if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_BLANK;
            pCharProp[char_index[0]].fCanGlyphAlone = 1;
        }
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
    }
    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);

    /* Tibeten script does not set sva.fDiacritic or sva.fZeroWidth */
    for (i = 0; i < cGlyphs; i++)
    {
        if (!pGlyphProp[i].sva.fClusterStart)
        {
            pGlyphProp[i].sva.fDiacritic = 0;
            pGlyphProp[i].sva.fZeroWidth = 0;
        }
    }
}

static void ShapeCharGlyphProp_BaseIndic( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp, lexical_function lexical, BOOL use_syllables, BOOL override_gsub)
{
    int i,k;

    OpenType_GDEF_UpdateGlyphProps(psc, pwGlyphs, cGlyphs, pwLogClust, cChars, pGlyphProp);
    for (i = 0; i < cGlyphs; i++)
    {
        int char_index[20];
        int char_count = 0;

        k = USP10_FindGlyphInLogClust(pwLogClust, cChars, i);
        if (k>=0)
        {
            for (; k < cChars && pwLogClust[k] == i; k++)
                char_index[char_count++] = k;
        }

        if (override_gsub)
        {
            /* Most indic scripts do not set fDiacritic or fZeroWidth */
            pGlyphProp[i].sva.fDiacritic = FALSE;
            pGlyphProp[i].sva.fZeroWidth = FALSE;
        }

        if (char_count == 0)
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;
            continue;
        }

        if (char_count ==1 && pwcChars[char_index[0]] == 0x0020)  /* space */
        {
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_BLANK;
            pCharProp[char_index[0]].fCanGlyphAlone = 1;
        }
        else
            pGlyphProp[i].sva.uJustification = SCRIPT_JUSTIFY_NONE;

        pGlyphProp[i].sva.fClusterStart = 0;
        for (k = 0; k < char_count && !pGlyphProp[i].sva.fClusterStart; k++)
            switch (lexical(pwcChars[char_index[k]]))
            {
                case lex_Matra_pre:
                case lex_Matra_post:
                case lex_Matra_above:
                case lex_Matra_below:
                case lex_Modifier:
                case lex_Halant:
                    break;
                case lex_ZWJ:
                case lex_ZWNJ:
                    /* check for dangling joiners */
                    if (pwcChars[char_index[k]-1] == 0x0020 || pwcChars[char_index[k]+1] == 0x0020)
                        pGlyphProp[i].sva.fClusterStart = 1;
                    else
                        k = char_count;
                    break;
                default:
                    pGlyphProp[i].sva.fClusterStart = 1;
                    break;
            }
    }

    if (use_syllables)
    {
        IndicSyllable *syllables = NULL;
        int syllable_count = 0;
        BOOL modern = get_GSUB_Indic2(psa, psc);

        Indic_ParseSyllables( hdc, psa, psc, pwcChars, cChars, &syllables, &syllable_count, lexical, modern);

        for (i = 0; i < syllable_count; i++)
        {
            int j;
            WORD g = pwLogClust[syllables[i].start];
            for (j = syllables[i].start+1; j <= syllables[i].end; j++)
            {
                if (pwLogClust[j] != g)
                {
                    pGlyphProp[pwLogClust[j]].sva.fClusterStart = 0;
                    pwLogClust[j] = g;
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, syllables);
    }

    UpdateClustersFromGlyphProp(cGlyphs, cChars, pwLogClust, pGlyphProp);
}

static void ShapeCharGlyphProp_Sinhala( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, sinhala_lex, FALSE, FALSE);
}

static void ShapeCharGlyphProp_Devanagari( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, devanagari_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Bengali( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, bengali_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Gurmukhi( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, gurmukhi_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Gujarati( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, gujarati_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Oriya( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, oriya_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Tamil( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, tamil_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Telugu( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, telugu_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Kannada( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, kannada_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Malayalam( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, malayalam_lex, TRUE, TRUE);
}

static void ShapeCharGlyphProp_Khmer( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp )
{
    ShapeCharGlyphProp_BaseIndic(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp, khmer_lex, TRUE, TRUE);
}

void SHAPE_CharGlyphProp(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp)
{
    load_ot_tables(hdc, psc);

    if (ShapingData[psa->eScript].charGlyphPropProc)
        ShapingData[psa->eScript].charGlyphPropProc(hdc, psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp);
    else
        ShapeCharGlyphProp_Default(psc, psa, pwcChars, cChars, pwGlyphs, cGlyphs, pwLogClust, pCharProp, pGlyphProp);
}

void SHAPE_ContextualShaping(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust)
{
    load_ot_tables(hdc, psc);

    if (ShapingData[psa->eScript].contextProc)
        ShapingData[psa->eScript].contextProc(hdc, psc, psa, pwcChars, cChars, pwOutGlyphs, pcGlyphs, cMaxGlyphs, pwLogClust);
}

static void SHAPE_ApplyOpenTypeFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, INT cChars, const TEXTRANGE_PROPERTIES *rpRangeProperties, WORD *pwLogClust)
{
    int i;
    INT dirL;

    if (!rpRangeProperties)
        return;

    load_ot_tables(hdc, psc);

    if (!psc->GSUB_Table)
        return;

    if (!psa->fLogicalOrder && psa->fRTL)
        dirL = -1;
    else
        dirL = 1;

    for (i = 0; i < rpRangeProperties->cotfRecords; i++)
    {
        if (rpRangeProperties->potfRecords[i].lParameter > 0)
        apply_GSUB_feature(hdc, psa, psc, pwOutGlyphs, dirL, pcGlyphs, cChars, (const char*)&rpRangeProperties->potfRecords[i].tagFeature, pwLogClust);
    }
}

void SHAPE_ApplyDefaultOpentypeFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, INT cChars, WORD *pwLogClust)
{
const TEXTRANGE_PROPERTIES *rpRangeProperties;
rpRangeProperties = &ShapingData[psa->eScript].defaultTextRange;

    SHAPE_ApplyOpenTypeFeatures(hdc, psc, psa, pwOutGlyphs, pcGlyphs, cMaxGlyphs, cChars, rpRangeProperties, pwLogClust);
}

void SHAPE_ApplyOpenTypePositions(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WORD* pwGlyphs, INT cGlyphs, int *piAdvance, GOFFSET *pGoffset )
{
    const TEXTRANGE_PROPERTIES *rpRangeProperties = &ShapingData[psa->eScript].defaultGPOSTextRange;
    int i;

    load_ot_tables(hdc, psc);

    if (!psc->GPOS_Table || !psc->otm)
        return;

    for (i = 0; i < rpRangeProperties->cotfRecords; i++)
    {
        if (rpRangeProperties->potfRecords[i].lParameter > 0)
        {
            LoadedFeature *feature;

            feature = load_OT_feature(hdc, psa, psc, FEATURE_GPOS_TABLE, (const char*)&rpRangeProperties->potfRecords[i].tagFeature);
            if (!feature)
                continue;

            GPOS_apply_feature(psc, psc->otm, &psc->lf, psa, piAdvance, feature, pwGlyphs, cGlyphs, pGoffset);
        }
    }
}

HRESULT SHAPE_CheckFontForRequiredFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa)
{
    LoadedFeature *feature;
    int i;

    if (!ShapingData[psa->eScript].requiredFeatures)
        return S_OK;

    load_ot_tables(hdc, psc);

    /* we need to have at least one of the required features */
    i = 0;
    while (ShapingData[psa->eScript].requiredFeatures[i])
    {
        feature = load_OT_feature(hdc, psa, psc, FEATURE_ALL_TABLES, ShapingData[psa->eScript].requiredFeatures[i]);
        if (feature)
            return S_OK;
        i++;
    }

    return USP_E_SCRIPT_NOT_IN_FONT;
}

HRESULT SHAPE_GetFontScriptTags( HDC hdc, ScriptCache *psc,
                                 SCRIPT_ANALYSIS *psa, int cMaxTags,
                                 OPENTYPE_TAG *pScriptTags, int *pcTags)
{
    HRESULT hr;
    OPENTYPE_TAG searching = 0x00000000;

    load_ot_tables(hdc, psc);

    if (psa && scriptInformation[psa->eScript].scriptTag)
        searching = scriptInformation[psa->eScript].scriptTag;

    hr = OpenType_GetFontScriptTags(psc, searching, cMaxTags, pScriptTags, pcTags);
    if (FAILED(hr))
        *pcTags = 0;
    return hr;
}

HRESULT SHAPE_GetFontLanguageTags( HDC hdc, ScriptCache *psc,
                                   SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript,
                                   int cMaxTags, OPENTYPE_TAG *pLangSysTags,
                                   int *pcTags)
{
    HRESULT hr;
    OPENTYPE_TAG searching = 0x00000000;
    BOOL fellback = FALSE;

    load_ot_tables(hdc, psc);

    if (psa && psc->userLang != 0)
        searching = psc->userLang;

    hr = OpenType_GetFontLanguageTags(psc, tagScript, searching, cMaxTags, pLangSysTags, pcTags);
    if (FAILED(hr))
    {
        fellback = TRUE;
        hr = OpenType_GetFontLanguageTags(psc, MS_MAKE_TAG('l','a','t','n'), searching, cMaxTags, pLangSysTags, pcTags);
    }

    if (FAILED(hr) || fellback)
        *pcTags = 0;
    if (SUCCEEDED(hr) && fellback && psa)
        hr = E_INVALIDARG;
    return hr;
}

HRESULT SHAPE_GetFontFeatureTags( HDC hdc, ScriptCache *psc,
                                  SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript,
                                  OPENTYPE_TAG tagLangSys, int cMaxTags,
                                  OPENTYPE_TAG *pFeatureTags, int *pcTags)
{
    HRESULT hr;
    BOOL filter = FALSE;

    load_ot_tables(hdc, psc);

    if (psa && scriptInformation[psa->eScript].scriptTag)
    {
        FIXME("Filtering not implemented\n");
        filter = TRUE;
    }

    hr = OpenType_GetFontFeatureTags(psc, tagScript, tagLangSys, filter, 0x00000000, FEATURE_ALL_TABLES, cMaxTags, pFeatureTags, pcTags, NULL);

    if (FAILED(hr))
        *pcTags = 0;
    return hr;
}
