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

#ifndef _USP10_INTERNAL_H_
#define _USP10_INTERNAL_H_

#include <config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <usp10.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )


#define Script_Latin   1
#define Script_CR      2
#define Script_Numeric 3
#define Script_Control 4
#define Script_Punctuation 5
#define Script_Arabic  6
#define Script_Arabic_Numeric  7
#define Script_Hebrew  8
#define Script_Syriac  9
#define Script_Persian 10
#define Script_Thaana  11
#define Script_Greek   12
#define Script_Cyrillic   13
#define Script_Armenian 14
#define Script_Georgian 15
/* Unicode Chapter 10 */
#define Script_Sinhala 16
#define Script_Tibetan 17
#define Script_Tibetan_Numeric 18
#define Script_Phags_pa 19
/* Unicode Chapter 11 */
#define Script_Thai 20
#define Script_Thai_Numeric 21
#define Script_Lao 22
#define Script_Lao_Numeric 23
/* Unicode Chapter 9 */
#define Script_Devanagari 24
#define Script_Devanagari_Numeric 25
#define Script_Bengali 26
#define Script_Bengali_Numeric 27
#define Script_Bengali_Currency 28
#define Script_Gurmukhi 29
#define Script_Gurmukhi_Numeric 30
#define Script_Gujarati 31
#define Script_Gujarati_Numeric 32
#define Script_Gujarati_Currency 33
#define Script_Oriya 34
#define Script_Oriya_Numeric 35
#define Script_Tamil 36
#define Script_Tamil_Numeric 37
#define Script_Telugu 38
#define Script_Telugu_Numeric 39
#define Script_Kannada 40
#define Script_Kannada_Numeric 41
#define Script_Malayalam 42
#define Script_Malayalam_Numeric 43
/* More supplemental */
#define Script_Diacritical 44
#define Script_Punctuation2 45
#define Script_Numeric2 46
/* Unicode Chapter 11 continued */
#define Script_Myanmar 47
#define Script_Myanmar_Numeric 48
#define Script_Tai_Le 49
#define Script_New_Tai_Lue 50
#define Script_New_Tai_Lue_Numeric 51
#define Script_Khmer 52
#define Script_Khmer_Numeric 53
/* Unicode Chapter 12 */
#define Script_CJK_Han  54
#define Script_Ideograph  55
#define Script_Bopomofo 56
#define Script_Kana 57
#define Script_Hangul 58
#define Script_Yi 59
/* Unicode Chapter 13 */
#define Script_Ethiopic 60
#define Script_Ethiopic_Numeric 61
#define Script_Mongolian 62
#define Script_Mongolian_Numeric 63
#define Script_Tifinagh 64
#define Script_NKo 65
#define Script_Vai 66
#define Script_Vai_Numeric 67
#define Script_Cherokee 68
#define Script_Canadian 69
/* Unicode Chapter 14 */
#define Script_Ogham 70
#define Script_Runic 71
/* Unicode Chapter 15 */
#define Script_Braille 72
/* Unicode Chapter 16 */
#define Script_Surrogates 73
#define Script_Private 74
/* Unicode Chapter 13 : Plane 1 */
#define Script_Deseret 75
#define Script_Osmanya 76
#define Script_Osmanya_Numeric 77
/* Unicode Chapter 15 : Plane 1 */
#define Script_MathAlpha 78
/* Additional Currency Scripts */
#define Script_Hebrew_Currency 79
#define Script_Vietnamese_Currency 80
#define Script_Thai_Currency 81

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
    LPCVOID  feature;
    INT lookup_count;
    WORD *lookups;
} LoadedFeature;

typedef struct {
    OPENTYPE_TAG tag;
    LPCVOID gsub_table;
    LPCVOID gpos_table;
    BOOL features_initialized;
    INT feature_count;
    LoadedFeature *features;
} LoadedLanguage;

typedef struct {
    OPENTYPE_TAG tag;
    LPCVOID gsub_table;
    LPCVOID gpos_table;
    LoadedLanguage default_language;
    BOOL languages_initialized;
    INT language_count;
    LoadedLanguage *languages;
} LoadedScript;

typedef struct {
    WORD *glyphs[GLYPH_MAX / GLYPH_BLOCK_SIZE];
} CacheGlyphPage;

typedef struct {
    LOGFONTW lf;
    TEXTMETRICW tm;
    OUTLINETEXTMETRICW *otm;
    SCRIPT_FONTPROPERTIES sfp;
    BOOL sfnt;
    CacheGlyphPage *page[NUM_PAGES];
    ABC *widths[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    LPVOID GSUB_Table;
    LPVOID GDEF_Table;
    LPVOID CMAP_Table;
    LPVOID CMAP_format12_Table;
    LPVOID GPOS_Table;
    BOOL scripts_initialized;
    INT script_count;
    LoadedScript *scripts;

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
typedef void (*reorder_function)(LPWSTR pwChar, IndicSyllable *syllable, lexical_function lex);

#define odd(x) ((x) & 1)
#define BIDI_STRONG  1
#define BIDI_WEAK    2
#define BIDI_NEUTRAL 0

int USP10_FindGlyphInLogClust(const WORD* pwLogClust, int cChars, WORD target) DECLSPEC_HIDDEN;

BOOL BIDI_DetermineLevels( LPCWSTR lpString, INT uCount, const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c, WORD *lpOutLevels, WORD *lpOutOverrides ) DECLSPEC_HIDDEN;
BOOL BIDI_GetStrengths(LPCWSTR lpString, INT uCount, const SCRIPT_CONTROL *c,
                      WORD* lpStrength) DECLSPEC_HIDDEN;
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

void Indic_ReorderCharacters( HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, LPWSTR input, int cChars, IndicSyllable **syllables, int *syllable_count, lexical_function lexical_f, reorder_function reorder_f, BOOL modern) DECLSPEC_HIDDEN;
void Indic_ParseSyllables( HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, LPCWSTR input, const int cChar, IndicSyllable **syllables, int *syllable_count, lexical_function lex, BOOL modern) DECLSPEC_HIDDEN;

void BREAK_line(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la) DECLSPEC_HIDDEN;

DWORD OpenType_CMAP_GetGlyphIndex(HDC hdc, ScriptCache *psc, DWORD utf32c, LPWORD pgi, DWORD flags) DECLSPEC_HIDDEN;
void OpenType_GDEF_UpdateGlyphProps(ScriptCache *psc, const WORD *pwGlyphs, const WORD cGlyphs, WORD* pwLogClust, const WORD cChars, SCRIPT_GLYPHPROP *pGlyphProp) DECLSPEC_HIDDEN;
INT OpenType_apply_GSUB_lookup(LPCVOID table, INT lookup_index, WORD *glyphs, INT glyph_index, INT write_dir, INT *glyph_count) DECLSPEC_HIDDEN;
INT OpenType_apply_GPOS_lookup(ScriptCache *psc, LPOUTLINETEXTMETRICW lpotm, LPLOGFONTW lplogfont, const SCRIPT_ANALYSIS *analysis, INT* piAdvance, INT lookup_index, const WORD *glyphs, INT glyph_index, INT glyph_count, GOFFSET *pGoffset) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontScriptTags(ScriptCache *psc, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontLanguageTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG searchingFor, int cMaxTags, OPENTYPE_TAG *pLanguageTags, int *pcTags) DECLSPEC_HIDDEN;
HRESULT OpenType_GetFontFeatureTags(ScriptCache *psc, OPENTYPE_TAG script_tag, OPENTYPE_TAG language_tag, BOOL filtered, OPENTYPE_TAG searchingFor, char tableType, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags, LoadedFeature** feature) DECLSPEC_HIDDEN;

#endif /* _USP10_INTERNAL_H_ */
