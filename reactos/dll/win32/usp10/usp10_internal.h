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

#define Script_Syriac  8
#define Script_Hebrew  7
#define Script_Arabic  6
#define Script_Latin   1
#define Script_Numeric 5
#define Script_CR      22
#define Script_LF      23

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

typedef struct {
    LOGFONTW lf;
    TEXTMETRICW tm;
    WORD *glyphs[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    ABC *widths[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    LPVOID *GSUB_Table;
} ScriptCache;

#define odd(x) ((x) & 1)

BOOL BIDI_DetermineLevels( LPCWSTR lpString, INT uCount, const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c, WORD *lpOutLevels );

INT BIDI_ReorderV2lLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse);
INT BIDI_ReorderL2vLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse);
void SHAPE_ShapeArabicGlyphs(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs);
