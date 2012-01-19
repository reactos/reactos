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

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

typedef struct {
    char    tag[5];
    char    script[5];
    LPCVOID  feature;
} LoadedFeature;

typedef struct {
    LOGFONTW lf;
    TEXTMETRICW tm;
    BOOL sfnt;
    WORD *glyphs[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    ABC *widths[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    LPVOID GSUB_Table;
    LPVOID GDEF_Table;
    INT feature_count;
    LoadedFeature *features;

    OPENTYPE_TAG userScript;
    OPENTYPE_TAG userLang;
} ScriptCache;

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

static inline WCHAR get_table_entry( const unsigned short *table, WCHAR ch )
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

typedef int (*lexical_function)(WCHAR c);
typedef void (*reorder_function)(LPWSTR pwChar, IndicSyllable *syllable, lexical_function lex);

#define odd(x) ((x) & 1)
#define BIDI_STRONG  1
#define BIDI_WEAK    2
#define BIDI_NEUTRAL 0

BOOL BIDI_DetermineLevels( LPCWSTR lpString, INT uCount, const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c, WORD *lpOutLevels ) DECLSPEC_HIDDEN;
BOOL BIDI_GetStrengths(LPCWSTR lpString, INT uCount, const SCRIPT_CONTROL *c,
                      WORD* lpStrength) DECLSPEC_HIDDEN;
INT BIDI_ReorderV2lLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse) DECLSPEC_HIDDEN;
INT BIDI_ReorderL2vLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse) DECLSPEC_HIDDEN;
void SHAPE_ContextualShaping(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust) DECLSPEC_HIDDEN;
void SHAPE_ApplyDefaultOpentypeFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, INT cChars, WORD *pwLogClust) DECLSPEC_HIDDEN;
HRESULT SHAPE_CheckFontForRequiredFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa) DECLSPEC_HIDDEN;
void SHAPE_CharGlyphProp(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, const WCHAR* pwcChars, const INT cChars, const WORD* pwGlyphs, const INT cGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProp, SCRIPT_GLYPHPROP *pGlyphProp) DECLSPEC_HIDDEN;
INT SHAPE_does_GSUB_feature_apply_to_chars(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, const WCHAR *chars, INT write_dir, INT count, const char* feature) DECLSPEC_HIDDEN;

void Indic_ReorderCharacters( HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, LPWSTR input, int cChars, IndicSyllable **syllables, int *syllable_count, lexical_function lexical_f, reorder_function reorder_f, BOOL modern) DECLSPEC_HIDDEN;
void Indic_ParseSyllables( HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache* psc, LPCWSTR input, const int cChar, IndicSyllable **syllables, int *syllable_count, lexical_function lex, BOOL modern);

void BREAK_line(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la) DECLSPEC_HIDDEN;
