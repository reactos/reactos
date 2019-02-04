/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
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

#pragma once

#include "wine/list.h"

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

enum usp10_script
{
    Script_Undefined = 0x00,
    Script_Latin = 0x01,
    Script_CR = 0x02,
    Script_Numeric = 0x03,
    Script_Control = 0x04,
    Script_Punctuation = 0x05,
    Script_Arabic = 0x06,
    Script_Arabic_Numeric = 0x07,
    Script_Hebrew = 0x08,
    Script_Syriac = 0x09,
    Script_Persian = 0x0a,
    Script_Thaana = 0x0b,
    Script_Greek = 0x0c,
    Script_Cyrillic = 0x0d,
    Script_Armenian = 0x0e,
    Script_Georgian = 0x0f,
    /* Unicode Chapter 10 */
    Script_Sinhala = 0x10,
    Script_Tibetan = 0x11,
    Script_Tibetan_Numeric = 0x12,
    Script_Phags_pa = 0x13,
    /* Unicode Chapter 11 */
    Script_Thai = 0x14,
    Script_Thai_Numeric = 0x15,
    Script_Lao = 0x16,
    Script_Lao_Numeric = 0x17,
    /* Unicode Chapter 9 */
    Script_Devanagari = 0x18,
    Script_Devanagari_Numeric = 0x19,
    Script_Bengali = 0x1a,
    Script_Bengali_Numeric = 0x1b,
    Script_Bengali_Currency = 0x1c,
    Script_Gurmukhi = 0x1d,
    Script_Gurmukhi_Numeric = 0x1e,
    Script_Gujarati = 0x1f,
    Script_Gujarati_Numeric = 0x20,
    Script_Gujarati_Currency = 0x21,
    Script_Oriya = 0x22,
    Script_Oriya_Numeric = 0x23,
    Script_Tamil = 0x24,
    Script_Tamil_Numeric = 0x25,
    Script_Telugu = 0x26,
    Script_Telugu_Numeric = 0x27,
    Script_Kannada = 0x28,
    Script_Kannada_Numeric = 0x29,
    Script_Malayalam = 0x2a,
    Script_Malayalam_Numeric = 0x2b,
    /* More supplemental */
    Script_Diacritical = 0x2c,
    Script_Punctuation2 = 0x2d,
    Script_Numeric2 = 0x2e,
    /* Unicode Chapter 11 continued */
    Script_Myanmar = 0x2f,
    Script_Myanmar_Numeric = 0x30,
    Script_Tai_Le = 0x31,
    Script_New_Tai_Lue = 0x32,
    Script_New_Tai_Lue_Numeric = 0x33,
    Script_Khmer = 0x34,
    Script_Khmer_Numeric = 0x35,
    /* Unicode Chapter 12 */
    Script_CJK_Han = 0x36,
    Script_Ideograph = 0x37,
    Script_Bopomofo = 0x38,
    Script_Kana = 0x39,
    Script_Hangul = 0x3a,
    Script_Yi = 0x3b,
    /* Unicode Chapter 13 */
    Script_Ethiopic = 0x3c,
    Script_Ethiopic_Numeric = 0x3d,
    Script_Mongolian = 0x3e,
    Script_Mongolian_Numeric = 0x3f,
    Script_Tifinagh = 0x40,
    Script_NKo = 0x41,
    Script_Vai = 0x42,
    Script_Vai_Numeric = 0x43,
    Script_Cherokee = 0x44,
    Script_Canadian = 0x45,
    /* Unicode Chapter 14 */
    Script_Ogham = 0x46,
    Script_Runic = 0x47,
    /* Unicode Chapter 15 */
    Script_Braille = 0x48,
    /* Unicode Chapter 16 */
    Script_Surrogates = 0x49,
    Script_Private = 0x4a,
    /* Unicode Chapter 13 : Plane 1 */
    Script_Deseret = 0x4b,
    Script_Osmanya = 0x4c,
    Script_Osmanya_Numeric = 0x4d,
    /* Unicode Chapter 15 : Plane 1 */
    Script_MathAlpha = 0x4e,
    /* Additional Currency Scripts */
    Script_Hebrew_Currency = 0x4f,
    Script_Vietnamese_Currency = 0x50,
    Script_Thai_Currency = 0x51,
};

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

#define NUM_PAGES         17

#define GSUB_E_NOFEATURE -20
#define GSUB_E_NOGLYPH -10

#define FEATURE_ALL_TABLES 0
#define FEATURE_GSUB_TABLE 1
#define FEATURE_GPOS_TABLE 2

typedef struct {
    OPENTYPE_TAG tag;
    CHAR tableType;
    const void *feature;
    INT lookup_count;
    WORD *lookups;
} LoadedFeature;

enum usp10_language_table
{
    USP10_LANGUAGE_TABLE_GSUB = 0,
    USP10_LANGUAGE_TABLE_GPOS,
    USP10_LANGUAGE_TABLE_COUNT
};

typedef struct {
    OPENTYPE_TAG tag;
    const void *table[USP10_LANGUAGE_TABLE_COUNT];
    BOOL features_initialized;
    LoadedFeature *features;
    SIZE_T features_size;
    SIZE_T feature_count;
} LoadedLanguage;

enum usp10_script_table
{
    USP10_SCRIPT_TABLE_GSUB = 0,
    USP10_SCRIPT_TABLE_GPOS,
    USP10_SCRIPT_TABLE_COUNT
};

typedef struct {
    OPENTYPE_TAG tag;
    const void *table[USP10_SCRIPT_TABLE_COUNT];
    LoadedLanguage default_language;
    BOOL languages_initialized;
    LoadedLanguage *languages;
    SIZE_T languages_size;
    SIZE_T language_count;
} LoadedScript;

typedef struct {
    WORD *glyphs[GLYPH_MAX / GLYPH_BLOCK_SIZE];
} CacheGlyphPage;

typedef struct {
    struct list entry;
    DWORD refcount;
    LOGFONTW lf;
    TEXTMETRICW tm;
    OUTLINETEXTMETRICW *otm;
    SCRIPT_FONTPROPERTIES sfp;
    BOOL sfnt;
    CacheGlyphPage *page[NUM_PAGES];
    ABC *widths[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    void *GSUB_Table;
    void *GDEF_Table;
    void *CMAP_Table;
    void *CMAP_format12_Table;
    void *GPOS_Table;
    BOOL scripts_initialized;
    LoadedScript *scripts;
    SIZE_T scripts_size;
    SIZE_T script_count;

    OPENTYPE_TAG userScript;
    OPENTYPE_TAG userLang;
} ScriptCache;

typedef struct _scriptData
{
    SCRIPT_ANALYSIS a;
    SCRIPT_PROPERTIES props;
    OPENTYPE_TAG scriptTag;
    WCHAR fallbackFont[LF_FACESIZE];
} scriptData;

typedef struct {
    INT start;
    INT base;
    INT ralf;
    INT blwf;
    INT pref;
    INT end;
} IndicSyllable;

enum {lex_Halant, lex_Composed_Vowel, lex_Matra_post, lex_Matra_pre, lex_Matra_above, lex_Matra_below, lex_ZWJ, lex_ZWNJ, lex_NBSP, lex_Modifier, lex_Vowel, lex_Consonant, lex_Generic, lex_Ra, lex_Vedic, lex_Anudatta, lex_Nukta};

static inline BOOL is_consonant( int type )
{
    return (type == lex_Ra || type == lex_Consonant);
}

static inline unsigned short get_table_entry( const unsigned short *table, WCHAR ch )
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

typedef int (*lexical_function)(WCHAR c);
typedef void (*reorder_function)(WCHAR *chars, IndicSyllable *syllable, lexical_function lex);

#define odd(x) ((x) & 1)
#define BIDI_STRONG  1
#define BIDI_WEAK    2
#define BIDI_NEUTRAL 0

BOOL usp10_array_reserve(void **elements, SIZE_T *capacity, SIZE_T count, SIZE_T size) DECLSPEC_HIDDEN;
int USP10_FindGlyphInLogClust(const WORD* pwLogClust, int cChars, WORD target) DECLSPEC_HIDDEN;

BOOL BIDI_DetermineLevels(const WCHAR *string, unsigned int count, const SCRIPT_STATE *s,
        const SCRIPT_CONTROL *c, WORD *levels, WORD *overrides) DECLSPEC_HIDDEN;
BOOL BIDI_GetStrengths(const WCHAR *string, unsigned int count,
        const SCRIPT_CONTROL *c, WORD *strength) DECLSPEC_HIDDEN;
INT BIDI_ReorderV2lLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse) DECLSPEC_HIDDEN;
INT BIDI_ReorderL2vLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse) DECLSPEC_HIDDEN;
void SHAPE_ContextualShaping(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust) DECLSPEC_HIDDEN;
void SHAPE_ApplyDefaultOpentypeFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, INT cChars, WORD *pwLogClust) DECLSPEC_HIDDEN;
void SHAPE_ApplyOpenTypePositions(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WORD* pwGlyphs, INT cGlyphs, int *piAdvance, GOFFSET *pGoffset ) DECLSPEC_HIDDEN;
HRESULT SHAPE_CheckFontForRequiredFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa) DECLSPEC_HIDDEN;
void SHAPE_CharGlyphProp(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp) DECLSPEC_HIDDEN;
INT SHAPE_does_GSUB_feature_apply_to_chars(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, const WCHAR *chars, INT write_dir, INT count, const char* feature) DECLSPEC_HIDDEN;
HRESULT SHAPE_GetFontScriptTags( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT SHAPE_GetFontLanguageTags( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, int cMaxTags, OPENTYPE_TAG *pLangSysTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT SHAPE_GetFontFeatureTags( HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, OPENTYPE_TAG tagLangSys, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags) DECLSPEC_HIDDEN;

void Indic_ReorderCharacters(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc, WCHAR *input, unsigned int cChars,
        IndicSyllable **syllables, int *syllable_count, lexical_function lexical_f,
        reorder_function reorder_f, BOOL modern) DECLSPEC_HIDDEN;
void Indic_ParseSyllables(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, const WCHAR *input, unsigned int cChar,
        IndicSyllable **syllables, int *syllable_count, lexical_function lex, BOOL modern) DECLSPEC_HIDDEN;

void BREAK_line(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la) DECLSPEC_HIDDEN;

DWORD OpenType_CMAP_GetGlyphIndex(HDC hdc, ScriptCache *psc, DWORD utf32c, LPWORD pgi, DWORD flags) DECLSPEC_HIDDEN;
void OpenType_GDEF_UpdateGlyphProps(ScriptCache *psc, const WORD *pwGlyphs, const WORD cGlyphs, WORD* pwLogClust, const WORD cChars, SCRIPT_GLYPHPROP *pGlyphProp) DECLSPEC_HIDDEN;
int OpenType_apply_GSUB_lookup(const void *table, unsigned int lookup_index, WORD *glyphs,
        unsigned int glyph_index, int write_dir, int *glyph_count) DECLSPEC_HIDDEN;
unsigned int OpenType_apply_GPOS_lookup(const ScriptCache *psc, const OUTLINETEXTMETRICW *otm,
        const LOGFONTW *logfont, const SCRIPT_ANALYSIS *analysis, int *advance, unsigned int lookup_index,
        const WORD *glyphs, unsigned int glyph_index, unsigned int glyph_count, GOFFSET *goffset) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontScriptTags(ScriptCache *psc, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontLanguageTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pLanguageTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontFeatureTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG language_tag, BOOL filtered, OPENTYPE_TAG searchingFor, char tableType, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags, LoadedFeature** feature) DECLSPEC_HIDDEN;
