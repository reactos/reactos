/*
 * Tests for usp10 dll
 *
 * Copyright 2006 Jeff Latimer
 * Copyright 2006 Hans Leidekker
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
 * Notes:
 * Uniscribe allows for processing of complex scripts such as joining
 * and filtering characters and bi-directional text with custom line breaks.
 */

#include <assert.h>
#include <stdio.h>

#include <wine/test.h>
#include <windows.h>
#include <usp10.h>

typedef struct _itemTest {
    char todo_flag[6];
    int iCharPos;
    int fRTL;
    int fLayoutRTL;
    int uBidiLevel;
    int fOverrideDirection;
    ULONG scriptTag;
    BOOL isBroken;
    int broken_value[6];
} itemTest;

typedef struct _shapeTest_char {
    WORD wLogClust;
    SCRIPT_CHARPROP CharProp;
} shapeTest_char;

typedef struct _shapeTest_glyph {
    int Glyph;
    SCRIPT_GLYPHPROP GlyphProp;
} shapeTest_glyph;

typedef struct _font_fingerprint {
    WCHAR check[10];
    WORD result[10];
} font_fingerprint;

/* Uniscribe 1.6 calls */
static HRESULT (WINAPI *pScriptItemizeOpenType)( const WCHAR *pwcInChars, int cInChars, int cMaxItems, const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState, SCRIPT_ITEM *pItems, ULONG *pScriptTags, int *pcItems);

static HRESULT (WINAPI *pScriptShapeOpenType)( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, OPENTYPE_TAG tagLangSys, int *rcRangeChars, TEXTRANGE_PROPERTIES **rpRangeProperties, int cRanges, const WCHAR *pwcChars, int cChars, int cMaxGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProps, WORD *pwOutGlyphs, SCRIPT_GLYPHPROP *pOutGlyphProps, int *pcGlyphs);

static HRESULT (WINAPI *pScriptGetFontScriptTags)( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa, int cMaxTags, OPENTYPE_TAG *pScriptTags, int *pcTags);
static HRESULT (WINAPI *pScriptGetFontLanguageTags)( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, int cMaxTags, OPENTYPE_TAG *pLangSysTags, int *pcTags);
static HRESULT (WINAPI *pScriptGetFontFeatureTags)( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, OPENTYPE_TAG tagLangSys, int cMaxTags, OPENTYPE_TAG *pFeatureTags, int *pcTags);

static inline void _test_items_ok(LPCWSTR string, DWORD cchString,
                         SCRIPT_CONTROL *Control, SCRIPT_STATE *State,
                         DWORD nItems, const itemTest* items, BOOL nItemsToDo,
                         const INT nItemsBroken[2])
{
    HRESULT hr;
    int x, outnItems;
    SCRIPT_ITEM outpItems[15];
    ULONG tags[15] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

    if (pScriptItemizeOpenType)
        hr = pScriptItemizeOpenType(string, cchString, 15, Control, State, outpItems, tags, &outnItems);
    else
        hr = ScriptItemize(string, cchString, 15, Control, State, outpItems, &outnItems);

    winetest_ok(hr == S_OK, "ScriptItemize should return S_OK not %08x\n", hr);
    if (nItemsBroken && (broken(nItemsBroken[0] == outnItems) || broken(nItemsBroken[1] == outnItems)))
    {
        winetest_win_skip("This test broken on this platform: nitems %d\n", outnItems);
        return;
    }
    todo_wine_if (nItemsToDo)
        winetest_ok(outnItems == nItems, "Wrong number of items (%u)\n", outnItems);
    outnItems = min(outnItems, nItems);
    for (x = 0; x <= outnItems; x++)
    {
        if (items[x].isBroken && broken(outpItems[x].iCharPos == items[x].broken_value[0]))
            winetest_win_skip("This test broken on this platform: item %d CharPos %d\n", x, outpItems[x].iCharPos);
        else todo_wine_if (items[x].todo_flag[0])
            winetest_ok(outpItems[x].iCharPos == items[x].iCharPos, "%i:Wrong CharPos (%i)\n",x,outpItems[x].iCharPos);

        if (items[x].isBroken && broken(outpItems[x].a.fRTL== items[x].broken_value[1]))
            winetest_win_skip("This test broken on this platform: item %d fRTL %d\n", x, outpItems[x].a.fRTL);
        else todo_wine_if (items[x].todo_flag[1])
            winetest_ok(outpItems[x].a.fRTL == items[x].fRTL, "%i:Wrong fRTL(%i)\n",x,outpItems[x].a.fRTL);

        if (items[x].isBroken && broken(outpItems[x].a.fLayoutRTL == items[x].broken_value[2]))
            winetest_win_skip("This test broken on this platform: item %d fLayoutRTL %d\n", x, outpItems[x].a.fLayoutRTL);
        else todo_wine_if (items[x].todo_flag[2])
            winetest_ok(outpItems[x].a.fLayoutRTL == items[x].fLayoutRTL, "%i:Wrong fLayoutRTL(%i)\n",x,outpItems[x].a.fLayoutRTL);

        if (items[x].isBroken && broken(outpItems[x].a.s.uBidiLevel == items[x].broken_value[3]))
            winetest_win_skip("This test broken on this platform: item %d BidiLevel %d\n", x, outpItems[x].a.s.uBidiLevel);
        else todo_wine_if (items[x].todo_flag[3])
            winetest_ok(outpItems[x].a.s.uBidiLevel == items[x].uBidiLevel, "%i:Wrong BidiLevel(%i)\n",x,outpItems[x].a.s.uBidiLevel);

        if (items[x].isBroken && broken(outpItems[x].a.s.fOverrideDirection == items[x].broken_value[4]))
            winetest_win_skip("This test broken on this platform: item %d fOverrideDirection %d\n", x, outpItems[x].a.s.fOverrideDirection);
        else todo_wine_if (items[x].todo_flag[4])
            winetest_ok(outpItems[x].a.s.fOverrideDirection == items[x].fOverrideDirection, "%i:Wrong fOverrideDirection(%i)\n",x,outpItems[x].a.s.fOverrideDirection);

        if (x != outnItems)
            winetest_ok(outpItems[x].a.eScript != SCRIPT_UNDEFINED, "%i: Undefined script\n",x);
        if (pScriptItemizeOpenType)
        {
            if (items[x].isBroken && broken(tags[x] == items[x].broken_value[5]))
                winetest_win_skip("This test broken on this platform: item %d Script Tag %x\n", x, tags[x]);
            else todo_wine_if (items[x].todo_flag[5])
                winetest_ok(tags[x] == items[x].scriptTag,"%i:Incorrect Script Tag %x != %x\n",x,tags[x],items[x].scriptTag);
        }

    }
}

#define test_items_ok(a,b,c,d,e,f,g,h) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_items_ok(a,b,c,d,e,f,g,h)

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

#define latn_tag MS_MAKE_TAG('l','a','t','n')
#define arab_tag MS_MAKE_TAG('a','r','a','b')
#define thai_tag MS_MAKE_TAG('t','h','a','i')
#define hebr_tag MS_MAKE_TAG('h','e','b','r')
#define syrc_tag MS_MAKE_TAG('s','y','r','c')
#define thaa_tag MS_MAKE_TAG('t','h','a','a')
#define deva_tag MS_MAKE_TAG('d','e','v','a')
#define beng_tag MS_MAKE_TAG('b','e','n','g')
#define guru_tag MS_MAKE_TAG('g','u','r','u')
#define gujr_tag MS_MAKE_TAG('g','u','j','r')
#define orya_tag MS_MAKE_TAG('o','r','y','a')
#define taml_tag MS_MAKE_TAG('t','a','m','l')
#define telu_tag MS_MAKE_TAG('t','e','l','u')
#define knda_tag MS_MAKE_TAG('k','n','d','a')
#define mlym_tag MS_MAKE_TAG('m','l','y','m')
#define mymr_tag MS_MAKE_TAG('m','y','m','r')
#define tale_tag MS_MAKE_TAG('t','a','l','e')
#define talu_tag MS_MAKE_TAG('t','a','l','u')
#define khmr_tag MS_MAKE_TAG('k','h','m','r')
#define hani_tag MS_MAKE_TAG('h','a','n','i')
#define bopo_tag MS_MAKE_TAG('b','o','p','o')
#define kana_tag MS_MAKE_TAG('k','a','n','a')
#define hang_tag MS_MAKE_TAG('h','a','n','g')
#define yi_tag MS_MAKE_TAG('y','i',' ',' ')
#define ethi_tag MS_MAKE_TAG('e','t','h','i')
#define mong_tag MS_MAKE_TAG('m','o','n','g')
#define tfng_tag MS_MAKE_TAG('t','f','n','g')
#define nko_tag MS_MAKE_TAG('n','k','o',' ')
#define vai_tag MS_MAKE_TAG('v','a','i',' ')
#define cher_tag MS_MAKE_TAG('c','h','e','r')
#define cans_tag MS_MAKE_TAG('c','a','n','s')
#define ogam_tag MS_MAKE_TAG('o','g','a','m')
#define runr_tag MS_MAKE_TAG('r','u','n','r')
#define brai_tag MS_MAKE_TAG('b','r','a','i')
#define dsrt_tag MS_MAKE_TAG('d','s','r','t')
#define osma_tag MS_MAKE_TAG('o','s','m','a')
#define math_tag MS_MAKE_TAG('m','a','t','h')

static void test_ScriptItemize( void )
{
    static const WCHAR test1[] = {'t', 'e', 's', 't',0};
    static const itemTest t11[2] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1}};
    static const itemTest t12[2] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    static const WCHAR test1b[] = {' ', ' ', ' ', ' ',0};
    static const itemTest t1b1[2] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t1b2[2] = {{{0,0,0,0,0,0},0,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    static const WCHAR test1c[] = {' ', ' ', ' ', '1', '2', ' ',0};
    static const itemTest t1c1[2] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t1c2[4] = {{{0,0,0,0,0,0},0,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},3,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},5,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Arabic, English*/
    static const WCHAR test2[] = {'1','2','3','-','5','2',0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,'7','1','.',0};
    static const itemTest t21[7] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},6,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},15,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};
    static const itemTest t22[5] = {{{0,0,0,0,0,0},0,0,0,2,0,0,FALSE},{{0,0,0,0,0,0},6,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},15,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};
    static const itemTest t23[5] = {{{0,0,0,0,0,0},0,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},6,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},15,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};
    static const itemTest t24[5] = {{{0,0,0,0,0,0},0,0,0,0,1,0,FALSE},
                                {{0,0,0,0,0,0},6,0,0,0,1,arab_tag,FALSE},
                                {{0,0,0,0,0,0},13,0,1,0,1,0,FALSE},
                                {{0,0,0,0,0,0},15,0,0,0,1,0,FALSE},
                                {{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};

    static const WCHAR test2b[] = {'A','B','C','-','D','E','F',' ',0x0621,0x0623,0x0624,0};
    static const itemTest t2b1[5] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},8,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2b2[5] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,2,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2b3[3] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2b4[5] = {{{0,0,0,0,0,0},0,0,0,0,1,latn_tag,FALSE},
                                    {{0,0,0,0,0,0},3,0,0,0,1,0,FALSE},
                                    {{0,0,0,0,0,0},4,0,0,0,1,latn_tag,FALSE},
                                    {{0,0,0,0,0,0},8,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const int b2[2] = {4,4};

    /* leading space */
    static const WCHAR test2c[] = {' ',0x0621,0x0623,0x0624,'A','B','C','-','D','E','F',0};
    static const itemTest t2c1[5] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2c2[6] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},1,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2c3[5] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},4,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,0,0,2,0,0,FALSE},{{0,0,0,0,0,0},8,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2c4[3] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},4,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2c5[5] = {{{0,0,0,0,0,0},0,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},4,0,0,0,1,latn_tag,FALSE},
                                    {{0,0,0,0,0,0},7,0,0,0,1,0,FALSE},
                                    {{0,0,0,0,0,0},8,0,0,0,1,latn_tag,FALSE},
                                    {{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};

    /* trailing space */
    static const WCHAR test2d[] = {'A','B','C','-','D','E','F',0x0621,0x0623,0x0624,' ',0};
    static const itemTest t2d1[5] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2d2[6] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2d3[5] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,2,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2d4[3] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},7,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};
    static const itemTest t2d5[5] = {{{0,0,0,0,0,0},0,0,0,0,1,latn_tag,FALSE},
                                {{0,0,0,0,0,0},3,0,0,0,1,0,FALSE},
                                {{0,0,0,0,0,0},4,0,0,0,1,latn_tag,FALSE},
                                {{0,0,0,0,0,0},7,0,0,0,1,arab_tag,FALSE},
                                {{0,0,0,0,0,0},11,0,0,0,0,-1,FALSE}};

    /* Thai */
    static const WCHAR test3[] =
{0x0e04,0x0e27,0x0e32,0x0e21,0x0e1e,0x0e22,0x0e32,0x0e22,0x0e32, 0x0e21
,0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e44,0x0e2b,0x0e19
,0x0e04,0x0e27,0x0e32,0x0e21,0x0e2a, 0x0e33,0x0e40,0x0e23,0x0e47,0x0e08,
 0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e19,0x0e31,0x0e48,0x0e19,0};

    static const itemTest t31[2] = {{{0,0,0,0,0,0},0,0,0,0,0,thai_tag,FALSE},{{0,0,0,0,0,0},41,0,0,0,0,-1,FALSE}};
    static const itemTest t32[2] = {{{0,0,0,0,0,0},0,0,0,2,0,thai_tag,FALSE},{{0,0,0,0,0,0},41,0,0,0,0,-1,FALSE}};

    static const WCHAR test4[]  = {'1','2','3','-','5','2',' ','i','s',' ','7','1','.',0};

    static const itemTest t41[6] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},12,0,0,0,0,-1,FALSE}};
    static const itemTest t42[5] = {{{0,0,0,0,0,0},0,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},6,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},7,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},10,0,0,2,0,0,FALSE},{{0,0,0,0,0,0},12,0,0,0,0,-1,FALSE}};
    static const itemTest t43[4] = {{{0,0,0,0,0,0},0,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},6,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},7,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},12,0,0,0,0,-1,FALSE}};
    static const int b43[2] = {4,4};

    /* Arabic */
    static const WCHAR test5[] = {0x0627,0x0644,0x0635,0x0651,0x0650,0x062d,0x0629,0x064f,' ',0x062a,0x064e,
                                  0x0627,0x062c,0x064c,' ',0x0639,0x064e,0x0644,0x0649,' ',
                                  0x0631,0x064f,0x0624,0x0648,0x0633,0x0650,' ',0x0627,0x0644,
                                  0x0623,0x0635,0x0650,0x062d,0x0651,0x064e,0x0627,0x0621,0x0650,0};
    static const itemTest t51[2] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},38,0,0,0,0,-1,FALSE}};
    static const itemTest t52[2] = {{{0,0,0,0,0,0},0,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},38,0,0,0,0,-1,FALSE}};


    /* Hebrew */
    static const WCHAR test6[]  = {0x05e9, 0x05dc, 0x05d5, 0x05dd, '.',0};
    static const itemTest t61[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,TRUE,{-1,0,0,0,-1,-1}},{{0,0,0,0,0,0},4,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t62[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},{{0,0,0,0,0,0},4,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t63[2] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t64[3] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},4,0,0,0,1,0,FALSE},
                                {{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    static const int b63[2] = {2,2};
    static const WCHAR test7[]  = {'p','a','r','t',' ','o','n','e',' ',0x05d7, 0x05dc, 0x05e7, ' ', 0x05e9, 0x05ea, 0x05d9, 0x05d9, 0x05dd, ' ','p','a','r','t',' ','t','h','r','e','e', 0};
    static const itemTest t71[4] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},9,1,1,1,0,hebr_tag,TRUE,{-1,0,0,0,-1,-1}},{{0,0,0,0,0,0},19,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},29,0,0,0,0,-1,FALSE}};
    static const itemTest t72[4] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},9,1,1,1,0,hebr_tag,FALSE},{{0,0,0,0,0,0},18,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},29,0,0,0,0,-1,FALSE}};
    static const itemTest t73[4] = {{{0,0,0,0,0,0},0,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},8,1,1,1,0,hebr_tag,FALSE},{{0,0,0,0,0,0},19,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},29,0,0,0,0,-1,FALSE}};
    static const itemTest t74[4] = {{{0,0,0,0,0,0},0,0,0,0,1,latn_tag,FALSE},
                                {{0,0,0,0,0,0},9,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},19,0,0,0,1,latn_tag,FALSE},
                                {{0,0,0,0,0,0},29,0,0,0,0,-1,FALSE}};

    static const WCHAR test8[] = {0x0633, 0x0644, 0x0627, 0x0645,0};
    static const itemTest t81[2] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t82[2] = {{{0,0,0,0,0,0},0,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    /* Syriac  (Like Arabic )*/
    static const WCHAR test9[] = {0x0710, 0x0712, 0x0712, 0x0714, '.',0};
    static const itemTest t91[3] = {{{0,0,0,0,0,0},0,1,1,1,0,syrc_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t92[3] = {{{0,0,0,0,0,0},0,1,1,1,0,syrc_tag},{{0,0,0,0,0,0},4,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t93[2] = {{{0,0,0,0,0,0},0,1,1,1,0,syrc_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t94[3] = {{{0,0,0,0,0,0},0,0,0,0,1,syrc_tag,FALSE},
                                    {{0,0,0,0,0,0},4,0,0,0,1,0,FALSE},
                                    {{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const int b93[2] = {2,2};

    static const WCHAR test10[] = {0x0717, 0x0718, 0x071a, 0x071b,0};
    static const itemTest t101[2] = {{{0,0,0,0,0,0},0,1,1,1,0,syrc_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t102[2] = {{{0,0,0,0,0,0},0,0,0,0,1,syrc_tag,FALSE},
                                     {{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    /* Devanagari */
    static const WCHAR test11[] = {0x0926, 0x0947, 0x0935, 0x0928, 0x093e, 0x0917, 0x0930, 0x0940};
    static const itemTest t111[2] = {{{0,0,0,0,0,0},0,0,0,0,0,deva_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t112[2] = {{{0,0,0,0,0,0},0,0,0,2,0,deva_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};

    /* Bengali */
    static const WCHAR test12[] = {0x09ac, 0x09be, 0x0982, 0x09b2, 0x09be};
    static const itemTest t121[2] = {{{0,0,0,0,0,0},0,0,0,0,0,beng_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t122[2] = {{{0,0,0,0,0,0},0,0,0,2,0,beng_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    /* Gurmukhi */
    static const WCHAR test13[] = {0x0a17, 0x0a41, 0x0a30, 0x0a2e, 0x0a41, 0x0a16, 0x0a40};
    static const itemTest t131[2] = {{{0,0,0,0,0,0},0,0,0,0,0,guru_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t132[2] = {{{0,0,0,0,0,0},0,0,0,2,0,guru_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    /* Gujarati */
    static const WCHAR test14[] = {0x0a97, 0x0ac1, 0x0a9c, 0x0ab0, 0x0abe, 0x0aa4, 0x0ac0};
    static const itemTest t141[2] = {{{0,0,0,0,0,0},0,0,0,0,0,gujr_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t142[2] = {{{0,0,0,0,0,0},0,0,0,2,0,gujr_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    /* Oriya */
    static const WCHAR test15[] = {0x0b13, 0x0b21, 0x0b3c, 0x0b3f, 0x0b06};
    static const itemTest t151[2] = {{{0,0,0,0,0,0},0,0,0,0,0,orya_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t152[2] = {{{0,0,0,0,0,0},0,0,0,2,0,orya_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    /* Tamil */
    static const WCHAR test16[] = {0x0ba4, 0x0bae, 0x0bbf, 0x0bb4, 0x0bcd};
    static const itemTest t161[2] = {{{0,0,0,0,0,0},0,0,0,0,0,taml_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t162[2] = {{{0,0,0,0,0,0},0,0,0,2,0,taml_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    /* Telugu */
    static const WCHAR test17[] = {0x0c24, 0x0c46, 0x0c32, 0x0c41, 0x0c17, 0x0c41};
    static const itemTest t171[2] = {{{0,0,0,0,0,0},0,0,0,0,0,telu_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t172[2] = {{{0,0,0,0,0,0},0,0,0,2,0,telu_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Kannada */
    static const WCHAR test18[] = {0x0c95, 0x0ca8, 0x0ccd, 0x0ca8, 0x0ca1};
    static const itemTest t181[2] = {{{0,0,0,0,0,0},0,0,0,0,0,knda_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t182[2] = {{{0,0,0,0,0,0},0,0,0,2,0,knda_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    /* Malayalam */
    static const WCHAR test19[] = {0x0d2e, 0x0d32, 0x0d2f, 0x0d3e, 0x0d33, 0x0d02};
    static const itemTest t191[2] = {{{0,0,0,0,0,0},0,0,0,0,0,mlym_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t192[2] = {{{0,0,0,0,0,0},0,0,0,2,0,mlym_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Diacritical */
    static const WCHAR test20[] = {0x0309,'a','b','c','d',0};
    static const itemTest t201[3] = {{{0,0,0,0,0,0},0,0,0,0,0x0,0,FALSE},{{0,0,0,0,0,0},1,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t202[3] = {{{0,0,0,0,0,0},0,0,0,2,0,0,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},1,0,0,2,0,latn_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    static const WCHAR test21[] = {0x0710, 0x0712, 0x0308, 0x0712, 0x0714,0};
    static const itemTest t211[2] = {{{0,0,0,0,0,0},0,1,1,1,0,syrc_tag,FALSE},{{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};
    static const itemTest t212[2] = {{{0,0,0,0,0,0},0,0,0,0,1,syrc_tag,FALSE},
                                     {{0,0,0,0,0,0},5,0,0,0,0,-1,FALSE}};

    /* Latin Punctuation */
    static const WCHAR test22[] = {'#','$',',','!','\"','*',0};
    static const itemTest t221[3] = {{{0,0,0,0,0,0},0,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t222[3] = {{{0,0,0,0,0,0},0,1,1,1,0,latn_tag,FALSE},{{0,0,0,0,0,0},3,1,1,1,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t223[2] = {{{0,0,0,0,0,0},0,1,1,1,0,latn_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const int b222[2] = {1,1};
    static const int b223[2] = {2,2};

    /* Number 2*/
    static const WCHAR test23[] = {'1','2','3',0x00b2,0x00b3,0x2070,0};
    static const itemTest t231[3] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t232[3] = {{{0,0,0,0,0,0},0,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},3,0,1,2,0,0,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Myanmar */
    static const WCHAR test24[] = {0x1019,0x103c,0x1014,0x103a,0x1019,0x102c,0x1021,0x1000,0x1039,0x1001,0x101b,0x102c};
    static const itemTest t241[2] = {{{0,0,0,0,0,0},0,0,0,0,0,mymr_tag,FALSE},{{0,0,0,0,0,0},12,0,0,0,0,-1,FALSE}};
    static const itemTest t242[2] = {{{0,0,0,0,0,0},0,0,0,2,0,mymr_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},12,0,0,0,0,-1,FALSE}};

    /* Tai Le */
    static const WCHAR test25[] = {0x1956,0x196d,0x1970,0x1956,0x196c,0x1973,0x1951,0x1968,0x1952,0x1970};
    static const itemTest t251[2] = {{{0,0,0,0,0,0},0,0,0,0,0,tale_tag,TRUE,{-1,-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};
    static const itemTest t252[2] = {{{0,0,0,0,0,0},0,0,0,2,0,tale_tag,TRUE,{-1,1,1,1,-1,latn_tag}},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};

    /* New Tai Lue */
    static const WCHAR test26[] = {0x1992,0x19c4};
    static const itemTest t261[2] = {{{0,0,0,0,0,0},0,0,0,0,0,talu_tag,TRUE,{-1,-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0,0},2,0,0,0,0,-1,FALSE}};
    static const itemTest t262[2] = {{{0,0,0,0,0,0},0,0,0,2,0,talu_tag,TRUE,{-1,1,1,1,-1,latn_tag}},{{0,0,0,0,0,0},2,0,0,0,0,-1,FALSE}};

    /* Khmer */
    static const WCHAR test27[] = {0x1781,0x17c1,0x1798,0x179a,0x1797,0x17b6,0x179f,0x17b6};
    static const itemTest t271[2] = {{{0,0,0,0,0,0},0,0,0,0,0,khmr_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t272[2] = {{{0,0,0,0,0,0},0,0,0,2,0,khmr_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};

    /* CJK Han */
    static const WCHAR test28[] = {0x8bed,0x7d20,0x6587,0x5b57};
    static const itemTest t281[2] = {{{0,0,0,0,0,0},0,0,0,0,0,hani_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t282[2] = {{{0,0,0,0,0,0},0,0,0,2,0,hani_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    /* Ideographic */
    static const WCHAR test29[] = {0x2ff0,0x2ff3,0x2ffb,0x2ff0,0x65e5,0x65e5,0x5de5,0x7f51,0x4e02,0x4e5e};
    static const itemTest t291[3] = {{{0,0,0,0,0,0},0,0,0,0,0,hani_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,hani_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};
    static const itemTest t292[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hani_tag,FALSE},{{0,0,0,0,0,0},4,0,0,2,0,hani_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};

    /* Bopomofo */
    static const WCHAR test30[] = {0x3113,0x3128,0x3127,0x3123,0x3108,0x3128,0x310f,0x3120};
    static const itemTest t301[2] = {{{0,0,0,0,0,0},0,0,0,0,0,bopo_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t302[2] = {{{0,0,0,0,0,0},0,0,0,2,0,bopo_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};

    /* Kana */
    static const WCHAR test31[] = {0x3072,0x3089,0x304b,0x306a,0x30ab,0x30bf,0x30ab,0x30ca};
    static const itemTest t311[2] = {{{0,0,0,0,0,0},0,0,0,0,0,kana_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t312[2] = {{{0,0,0,0,0,0},0,0,0,2,0,kana_tag,FALSE},{{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const int b311[2] = {2,2};
    static const int b312[2] = {2,2};

    /* Hangul */
    static const WCHAR test32[] = {0xd55c,0xad6d,0xc5b4};
    static const itemTest t321[2] = {{{0,0,0,0,0,0},0,0,0,0,0,hang_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};
    static const itemTest t322[2] = {{{0,0,0,0,0,0},0,0,0,2,0,hang_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};

    /* Yi */
    static const WCHAR test33[] = {0xa188,0xa320,0xa071,0xa0b7};
    static const itemTest t331[2] = {{{0,0,0,0,0,0},0,0,0,0,0,yi_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t332[2] = {{{0,0,0,0,0,0},0,0,0,2,0,yi_tag,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    /* Ethiopic */
    static const WCHAR test34[] = {0x130d,0x12d5,0x12dd};
    static const itemTest t341[2] = {{{0,0,0,0,0,0},0,0,0,0,0,ethi_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};
    static const itemTest t342[2] = {{{0,0,0,0,0,0},0,0,0,2,0,ethi_tag,FALSE},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};
    static const int b342[2] = {2,2};

    /* Mongolian */
    static const WCHAR test35[] = {0x182e,0x1823,0x1829,0x182d,0x1823,0x182f,0x0020,0x182a,0x1822,0x1834,0x1822,0x182d,0x180c};
    static const itemTest t351[2] = {{{0,0,0,0,0,0},0,0,0,0,0,mong_tag,FALSE},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    static const int b351[2] = {2,2};
    static const itemTest t352[2] = {{{0,0,0,0,0,0},0,0,0,2,0,mong_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    static const int b352[2] = {2,3};
    static const itemTest t353[2] = {{{0,0,0,0,1,0},0,0,0,0,1,mong_tag,TRUE,{-1,-1,-1,-1,0,0}},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};

    /* Tifinagh */
    static const WCHAR test36[] = {0x2d5c,0x2d49,0x2d3c,0x2d49,0x2d4f,0x2d30,0x2d56};
    static const itemTest t361[2] = {{{0,0,0,0,0,0},0,0,0,0,0,tfng_tag,TRUE,{-1,-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t362[2] = {{{0,0,0,0,0,0},0,0,0,2,0,tfng_tag,TRUE,{-1,1,1,1,-1,latn_tag}},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    /* N'Ko */
    static const WCHAR test37[] = {0x07d2,0x07de,0x07cf};
    static const itemTest t371[2] = {{{0,0,0,0,0,0},0,1,1,1,0,nko_tag,TRUE,{-1,0,0,0,-1,arab_tag}},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};
    static const itemTest t372[2] = {{{0,0,0,0,0,0},0,1,1,1,0,nko_tag,TRUE,{-1,0,0,2,-1,arab_tag}},{{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};
    static const itemTest t373[2] = {{{0,0,0,0,0,0},0,0,0,0,1,nko_tag,TRUE,{-1,-1,-1,2,0,arab_tag}}, {{0,0,0,0,0,0},3,0,0,0,0,-1,FALSE}};

    /* Vai */
    static const WCHAR test38[] = {0xa559,0xa524};
    static const itemTest t381[2] = {{{0,0,0,0,0,0},0,0,0,0,0,vai_tag,TRUE,{-1,-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0,0},2,0,0,0,0,-1,FALSE}};
    static const itemTest t382[2] = {{{0,0,0,0,0,0},0,0,0,2,0,vai_tag,TRUE,{-1,1,1,1,-1,latn_tag}},{{0,0,0,0,0,0},2,0,0,0,0,-1,FALSE}};

    /* Cherokee */
    static const WCHAR test39[] = {0x13e3,0x13b3,0x13a9,0x0020,0x13a6,0x13ec,0x13c2,0x13af,0x13cd,0x13d7};
    static const itemTest t391[2] = {{{0,0,0,0,0,0},0,0,0,0,0,cher_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};
    static const itemTest t392[2] = {{{0,0,0,0,0,0},0,0,0,2,0,cher_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},10,0,0,0,0,-1,FALSE}};

    /* Canadian Aboriginal Syllabics */
    static const WCHAR test40[] = {0x1403,0x14c4,0x1483,0x144e,0x1450,0x1466};
    static const itemTest t401[2] = {{{0,0,0,0,0,0},0,0,0,0,0,cans_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t402[2] = {{{0,0,0,0,0,0},0,0,0,2,0,cans_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Ogham */
    static const WCHAR test41[] = {0x169b,0x1691,0x168c,0x1690,0x168b,0x169c};
    static const itemTest t411[2] = {{{0,0,0,0,0,0},0,0,0,0,0,ogam_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t412[4] = {{{0,0,0,0,0,0},0,1,1,1,0,ogam_tag,FALSE},{{0,0,0,0,0,0},1,0,0,2,0,ogam_tag,FALSE},{{0,0,0,0,0,0},5,1,1,1,0,ogam_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const int b412[2] = {1,1};

    /* Runic */
    static const WCHAR test42[] = {0x16a0,0x16a1,0x16a2,0x16a3,0x16a4,0x16a5};
    static const itemTest t421[2] = {{{0,0,0,0,0,0},0,0,0,0,0,runr_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t422[4] = {{{0,0,0,0,0,0},0,0,0,2,0,runr_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Braille */
    static const WCHAR test43[] = {0x280f,0x2817,0x2811,0x280d,0x280a,0x2811,0x2817};
    static const itemTest t431[2] = {{{0,0,0,0,0,0},0,0,0,0,0,brai_tag,FALSE},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t432[4] = {{{0,0,0,0,0,0},0,0,0,2,0,brai_tag,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    /* Private and Surrogates Area */
    static const WCHAR test44[] = {0xe000, 0xe001, 0xd800, 0xd801};
    static const itemTest t441[3] = {{{0,0,0,0,0,0},0,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},2,0,0,0,0,0,FALSE},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};
    static const itemTest t442[4] = {{{0,0,0,0,0,0},0,0,0,2,0,0,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},2,0,0,2,0,0,TRUE,{-1,1,1,1,-1,-1}},{{0,0,0,0,0,0},4,0,0,0,0,-1,FALSE}};

    /* Deseret */
    static const WCHAR test45[] = {0xd801,0xdc19,0xd801,0xdc32,0xd801,0xdc4c,0xd801,0xdc3c,0xd801,0xdc32,0xd801,0xdc4b,0xd801,0xdc2f,0xd801,0xdc4c,0xd801,0xdc3b,0xd801,0xdc32,0xd801,0xdc4a,0xd801,0xdc28};
    static const itemTest t451[2] = {{{0,0,0,0,0,0},0,0,0,0,0,dsrt_tag,TRUE,{-1,-1,-1,-1,-1,0x0}},{{0,0,0,0,0,0},24,0,0,0,0,-1,FALSE}};
    static const itemTest t452[2] = {{{0,0,0,0,0,0},0,0,0,2,0,dsrt_tag,TRUE,{-1,1,1,1,-1,0x0}},{{0,0,0,0,0,0},24,0,0,0,0,-1,FALSE}};

    /* Osmanya */
    static const WCHAR test46[] = {0xd801,0xdc8b,0xd801,0xdc98,0xd801,0xdc88,0xd801,0xdc91,0xd801,0xdc9b,0xd801,0xdc92,0xd801,0xdc95,0xd801,0xdc80};
    static const itemTest t461[2] = {{{0,0,0,0,0,0},0,0,0,0,0,osma_tag,TRUE,{-1,-1,-1,-1,-1,0x0}},{{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};
    static const itemTest t462[2] = {{{0,0,0,0,0,0},0,0,0,2,0,osma_tag,TRUE,{-1,1,1,1,-1,0x0}},{{0,0,0,0,0,0},16,0,0,0,0,-1,FALSE}};

    /* Mathematical Alphanumeric Symbols */
    static const WCHAR test47[] = {0xd835,0xdc00,0xd835,0xdc35,0xd835,0xdc6a,0xd835,0xdc9f,0xd835,0xdcd4,0xd835,0xdd09,0xd835,0xdd3e,0xd835,0xdd73,0xd835,0xdda8,0xd835,0xdddd,0xd835,0xde12,0xd835,0xde47,0xd835,0xde7c};
    static const itemTest t471[2] = {{{0,0,0,0,0,0},0,0,0,0,0,math_tag,TRUE,{-1,-1,-1,-1,-1,0x0}},{{0,0,0,0,0,0},26,0,0,0,0,-1,FALSE}};
    static const itemTest t472[2] = {{{0,0,0,0,0,0},0,0,0,2,0,math_tag,TRUE,{-1,1,1,1,-1,0x0}},{{0,0,0,0,0,0},26,0,0,0,0,-1,FALSE}};

    /* Mathematical and Numeric combinations */
    /* These have a leading hebrew character to force complicated itemization */
    static const WCHAR test48[] = {0x05e9,' ','1','2','3','.'};
    static const itemTest t481[4] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},5,0,0,0,0,0,FALSE},
        {{0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t482[4] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                    {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                                    {{0,0,0,0,0,0},5,0,0,0,1,0,FALSE},
                                    {{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    static const WCHAR test49[] = {0x05e9,' ','1','2','.','1','2'};
    static const itemTest t491[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t492[3] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                                {{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    static const WCHAR test50[] = {0x05e9,' ','.','1','2','3'};
    static const itemTest t501[4] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,1,1,1,0,0,FALSE},{{0,0,0,0,0},3,0,1,2,0,0,FALSE},
        {{0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t502[4] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},2,0,0,0,1,0,FALSE},
                                {{0,0,0,0,0,0},3,0,1,0,1,0,FALSE},
                                {{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    static const WCHAR test51[] = {0x05e9,' ','a','b','.','1','2'};
    static const itemTest t511[5] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},1,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},4,0,0,0,0,0,FALSE},
        {{0,0,0,0,0},5,0,0,2,0,0,FALSE},{{0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t512[5] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                            {{0,0,0,0,0,0},2,0,0,0,1,latn_tag,FALSE},
                            {{0,0,0,0,0,0},4,0,0,0,1,0,FALSE},
                            {{0,0,0,0,0,0},5,0,0,0,1,0,FALSE},
                            {{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    static const WCHAR test52[] = {0x05e9,' ','1','2','.','a','b'};
    static const itemTest t521[5] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,0,0,FALSE},
        {{0,0,0,0,0},5,0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t522[5] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                        {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                        {{0,0,0,0,0,0},4,0,0,0,1,0,FALSE},
                        {{0,0,0,0,0,0},5,0,0,0,1,latn_tag,FALSE},
                        {{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};

    static const WCHAR test53[] = {0x05e9,' ','1','2','.','.','1','2'};
    static const itemTest t531[5] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},4,1,1,1,0,0,FALSE},
        {{0,0,0,0,0},6,0,1,2,0,0,FALSE},{{0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t532[5] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                            {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                            {{0,0,0,0,0,0},4,0,0,0,1,0,FALSE},
                            {{0,0,0,0,0,0},6,0,1,0,1,0,FALSE},
                            {{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};

    static const WCHAR test54[] = {0x05e9,' ','1','2','+','1','2'};
    static const itemTest t541[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const itemTest t542[3] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                                {{0,0,0,0,0,0},7,0,0,0,0,-1,FALSE}};
    static const WCHAR test55[] = {0x05e9,' ','1','2','+','+','1','2'};
    static const itemTest t551[3] = {{{0,0,0,0,0,0},0,1,1,1,0,hebr_tag,FALSE},
        {{0,0,0,0,0},2,0,1,2,0,0,FALSE},{{0,0,0,0,0},8,0,0,0,0,-1,FALSE}};
    static const itemTest t552[3] = {{{0,0,0,0,0,0},0,0,0,0,1,hebr_tag,FALSE},
                                {{0,0,0,0,0,0},2,0,1,0,1,0,FALSE},
                                {{0,0,0,0,0,0},8,0,0,0,0,-1,FALSE}};

    /* ZWNJ */
    static const WCHAR test56[] = {0x0645, 0x06cc, 0x200c, 0x06a9, 0x0646, 0x0645}; /* می‌کنم */
    static const itemTest t561[] = {{{0,0,0,0,0,0},0,1,1,1,0,arab_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};
    static const itemTest t562[] = {{{0,0,0,0,0,0},0,0,0,0,1,arab_tag,FALSE},{{0,0,0,0,0,0},6,0,0,0,0,-1,FALSE}};

    /* Persian numerals and punctuation. */
    static const WCHAR test57[] = {0x06f1, 0x06f2, 0x066c, 0x06f3, 0x06f4, 0x06f5, 0x066c,  /* ۱۲٬۳۴۵٬ */
                                   0x06f6, 0x06f7, 0x06f8, 0x066b, 0x06f9, 0x06f0};         /* ۶۷۸٫۹۰ */
    static const itemTest t571[] = {{{0,0,0,0,0,0}, 0,0,1,2,0,arab_tag,FALSE},{{0,0,0,0,0,0}, 2,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 3,0,1,2,0,arab_tag,FALSE},{{0,0,0,0,0,0}, 6,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 7,0,1,2,0,arab_tag,FALSE},{{0,0,0,0,0,0},10,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},11,0,1,2,0,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    static const itemTest t572[] = {{{0,0,0,0,0,0}, 0,0,0,2,0,arab_tag,FALSE},{{0,0,1,0,0,0}, 2,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 3,0,0,2,0,arab_tag,FALSE},{{0,0,1,0,0,0}, 6,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 7,0,0,2,0,arab_tag,FALSE},{{0,0,1,0,0,0},10,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},11,0,0,2,0,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    static const itemTest t573[] = {{{0,0,0,0,0,0}, 0,0,0,0,1,arab_tag,FALSE},{{0,0,0,0,0,0}, 2,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 3,0,0,0,1,arab_tag,FALSE},{{0,0,0,0,0,0}, 6,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0}, 7,0,0,0,1,arab_tag,FALSE},{{0,0,0,0,0,0},10,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},11,0,0,0,1,arab_tag,FALSE},{{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    /* Arabic numerals and punctuation. */
    static const WCHAR test58[] = {0x0661, 0x0662, 0x066c, 0x0663, 0x0664, 0x0665, 0x066c,  /* ١٢٬٣٤٥٬ */
                                   0x0666, 0x0667, 0x0668, 0x066b, 0x0669, 0x0660};         /* ٦٧٨٫٩٠ */
    static const itemTest t581[] = {{{0,0,0,0,0,0}, 0,0,1,2,0,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};
    static const itemTest t582[] = {{{0,0,1,1,1,0}, 0,0,0,0,1,arab_tag,FALSE},
                                    {{0,0,0,0,0,0},13,0,0,0,0,-1,FALSE}};

    SCRIPT_ITEM items[15];
    SCRIPT_CONTROL  Control;
    SCRIPT_STATE    State;
    HRESULT hr;
    int nItems;

    memset(&Control, 0, sizeof(Control));
    memset(&State, 0, sizeof(State));

    hr = ScriptItemize(NULL, 4, 10, &Control, &State, items, NULL);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pwcInChars is NULL\n");

    hr = ScriptItemize(test1, 4, 10, &Control, &State, NULL, NULL);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pItems is NULL\n");

    hr = ScriptItemize(test1, 4, 1, &Control, &State, items, NULL);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cMaxItems < 2.\n");

    hr = ScriptItemize(test1, 0, 10, NULL, NULL, items, &nItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cInChars is 0\n");

    test_items_ok(test1,4,NULL,NULL,1,t11,FALSE,0);
    test_items_ok(test1b,4,NULL,NULL,1,t1b1,FALSE,0);
    test_items_ok(test1c,6,NULL,NULL,1,t1c1,FALSE,0);
    test_items_ok(test2,16,NULL,NULL,6,t21,FALSE,0);
    test_items_ok(test2b,11,NULL,NULL,4,t2b1,FALSE,0);
    test_items_ok(test2c,11,NULL,NULL,4,t2c1,FALSE,0);
    test_items_ok(test2d,11,NULL,NULL,4,t2d1,FALSE,0);
    test_items_ok(test3,41,NULL,NULL,1,t31,FALSE,0);
    test_items_ok(test4,12,NULL,NULL,5,t41,FALSE,0);
    test_items_ok(test5,38,NULL,NULL,1,t51,FALSE,0);
    test_items_ok(test6,5,NULL,NULL,2,t61,FALSE,0);
    test_items_ok(test7,29,NULL,NULL,3,t71,FALSE,0);
    test_items_ok(test8,4,NULL,NULL,1,t81,FALSE,0);
    test_items_ok(test9,5,NULL,NULL,2,t91,FALSE,0);
    test_items_ok(test10,4,NULL,NULL,1,t101,FALSE,0);
    test_items_ok(test11,8,NULL,NULL,1,t111,FALSE,0);
    test_items_ok(test12,5,NULL,NULL,1,t121,FALSE,0);
    test_items_ok(test13,7,NULL,NULL,1,t131,FALSE,0);
    test_items_ok(test14,7,NULL,NULL,1,t141,FALSE,0);
    test_items_ok(test15,5,NULL,NULL,1,t151,FALSE,0);
    test_items_ok(test16,5,NULL,NULL,1,t161,FALSE,0);
    test_items_ok(test17,6,NULL,NULL,1,t171,FALSE,0);
    test_items_ok(test18,5,NULL,NULL,1,t181,FALSE,0);
    test_items_ok(test19,6,NULL,NULL,1,t191,FALSE,0);
    test_items_ok(test20,5,NULL,NULL,2,t201,FALSE,0);
    test_items_ok(test21,5,NULL,NULL,1,t211,FALSE,0);
    test_items_ok(test22,6,NULL,NULL,2,t221,FALSE,0);
    test_items_ok(test23,6,NULL,NULL,2,t231,FALSE,0);
    test_items_ok(test24,12,NULL,NULL,1,t241,FALSE,0);
    test_items_ok(test25,10,NULL,NULL,1,t251,FALSE,0);
    test_items_ok(test26,2,NULL,NULL,1,t261,FALSE,0);
    test_items_ok(test27,8,NULL,NULL,1,t271,FALSE,0);
    test_items_ok(test28,4,NULL,NULL,1,t281,FALSE,0);
    test_items_ok(test29,10,NULL,NULL,2,t291,FALSE,0);
    test_items_ok(test30,8,NULL,NULL,1,t301,FALSE,0);
    test_items_ok(test31,8,NULL,NULL,1,t311,FALSE,b311);
    test_items_ok(test32,3,NULL,NULL,1,t321,FALSE,0);
    test_items_ok(test33,4,NULL,NULL,1,t331,FALSE,0);
    test_items_ok(test34,3,NULL,NULL,1,t341,FALSE,0);
    test_items_ok(test35,13,NULL,NULL,1,t351,FALSE,b351);
    test_items_ok(test36,7,NULL,NULL,1,t361,FALSE,0);
    test_items_ok(test37,3,NULL,NULL,1,t371,FALSE,0);
    test_items_ok(test38,2,NULL,NULL,1,t381,FALSE,0);
    test_items_ok(test39,10,NULL,NULL,1,t391,FALSE,0);
    test_items_ok(test40,6,NULL,NULL,1,t401,FALSE,0);
    test_items_ok(test41,6,NULL,NULL,1,t411,FALSE,0);
    test_items_ok(test42,6,NULL,NULL,1,t421,FALSE,0);
    test_items_ok(test43,7,NULL,NULL,1,t431,FALSE,0);
    test_items_ok(test44,4,NULL,NULL,2,t441,FALSE,0);
    test_items_ok(test45,24,NULL,NULL,1,t451,FALSE,0);
    test_items_ok(test46,16,NULL,NULL,1,t461,FALSE,0);
    test_items_ok(test47,26,NULL,NULL,1,t471,FALSE,0);
    test_items_ok(test56,6,NULL,NULL,1,t561,FALSE,0);
    test_items_ok(test57,13,NULL,NULL,7,t571,FALSE,0);
    test_items_ok(test58,13,NULL,NULL,1,t581,FALSE,0);

    State.uBidiLevel = 0;
    test_items_ok(test1,4,&Control,&State,1,t11,FALSE,0);
    test_items_ok(test1b,4,&Control,&State,1,t1b1,FALSE,0);
    test_items_ok(test1c,6,&Control,&State,1,t1c1,FALSE,0);
    test_items_ok(test2,16,&Control,&State,4,t22,FALSE,0);
    test_items_ok(test2b,11,&Control,&State,4,t2b1,FALSE,0);
    test_items_ok(test2c,11,&Control,&State,5,t2c2,FALSE,0);
    test_items_ok(test2d,11,&Control,&State,5,t2d2,FALSE,0);
    test_items_ok(test3,41,&Control,&State,1,t31,FALSE,0);
    test_items_ok(test4,12,&Control,&State,5,t41,FALSE,0);
    test_items_ok(test5,38,&Control,&State,1,t51,FALSE,0);
    test_items_ok(test6,5,&Control,&State,2,t61,FALSE,0);
    test_items_ok(test7,29,&Control,&State,3,t72,FALSE,0);
    test_items_ok(test8,4,&Control,&State,1,t81,FALSE,0);
    test_items_ok(test9,5,&Control,&State,2,t91,FALSE,0);
    test_items_ok(test10,4,&Control,&State,1,t101,FALSE,0);
    test_items_ok(test11,8,&Control,&State,1,t111,FALSE,0);
    test_items_ok(test12,5,&Control,&State,1,t121,FALSE,0);
    test_items_ok(test13,7,&Control,&State,1,t131,FALSE,0);
    test_items_ok(test14,7,&Control,&State,1,t141,FALSE,0);
    test_items_ok(test15,5,&Control,&State,1,t151,FALSE,0);
    test_items_ok(test16,5,&Control,&State,1,t161,FALSE,0);
    test_items_ok(test17,6,&Control,&State,1,t171,FALSE,0);
    test_items_ok(test18,5,&Control,&State,1,t181,FALSE,0);
    test_items_ok(test19,6,&Control,&State,1,t191,FALSE,0);
    test_items_ok(test20,5,&Control,&State,2,t201,FALSE,0);
    test_items_ok(test21,5,&Control,&State,1,t211,FALSE,0);
    test_items_ok(test22,6,&Control,&State,2,t221,FALSE,0);
    test_items_ok(test23,6,&Control,&State,2,t231,FALSE,0);
    test_items_ok(test24,12,&Control,&State,1,t241,FALSE,0);
    test_items_ok(test25,10,&Control,&State,1,t251,FALSE,0);
    test_items_ok(test26,2,&Control,&State,1,t261,FALSE,0);
    test_items_ok(test27,8,&Control,&State,1,t271,FALSE,0);
    test_items_ok(test28,4,&Control,&State,1,t281,FALSE,0);
    test_items_ok(test29,10,&Control,&State,2,t291,FALSE,0);
    test_items_ok(test30,8,&Control,&State,1,t301,FALSE,0);
    test_items_ok(test31,8,&Control,&State,1,t311,FALSE,b311);
    test_items_ok(test32,3,&Control,&State,1,t321,FALSE,0);
    test_items_ok(test33,4,&Control,&State,1,t331,FALSE,0);
    test_items_ok(test34,3,&Control,&State,1,t341,FALSE,0);
    test_items_ok(test35,13,&Control,&State,1,t351,FALSE,b351);
    test_items_ok(test36,7,&Control,&State,1,t361,FALSE,0);
    test_items_ok(test37,3,&Control,&State,1,t371,FALSE,0);
    test_items_ok(test38,2,&Control,&State,1,t381,FALSE,0);
    test_items_ok(test39,10,&Control,&State,1,t391,FALSE,0);
    test_items_ok(test40,6,&Control,&State,1,t401,FALSE,0);
    test_items_ok(test41,6,&Control,&State,1,t411,FALSE,0);
    test_items_ok(test42,6,&Control,&State,1,t421,FALSE,0);
    test_items_ok(test43,7,&Control,&State,1,t431,FALSE,0);
    test_items_ok(test44,4,&Control,&State,2,t441,FALSE,0);
    test_items_ok(test45,24,&Control,&State,1,t451,FALSE,0);
    test_items_ok(test46,16,&Control,&State,1,t461,FALSE,0);
    test_items_ok(test47,26,&Control,&State,1,t471,FALSE,0);
    test_items_ok(test48,6,&Control,&State,3,t481,FALSE,0);
    test_items_ok(test49,7,&Control,&State,2,t491,FALSE,0);
    test_items_ok(test50,6,&Control,&State,3,t501,FALSE,0);
    test_items_ok(test51,7,&Control,&State,4,t511,FALSE,0);
    test_items_ok(test52,7,&Control,&State,4,t521,FALSE,0);
    test_items_ok(test53,8,&Control,&State,4,t531,FALSE,0);
    test_items_ok(test54,7,&Control,&State,2,t541,FALSE,0);
    test_items_ok(test55,8,&Control,&State,2,t551,FALSE,0);
    test_items_ok(test56,6,&Control,&State,1,t561,FALSE,0);
    test_items_ok(test57,13,&Control,&State,7,t572,FALSE,0);
    test_items_ok(test58,13,&Control,&State,1,t581,FALSE,0);

    State.uBidiLevel = 1;
    test_items_ok(test1,4,&Control,&State,1,t12,FALSE,0);
    test_items_ok(test1b,4,&Control,&State,1,t1b2,FALSE,0);
    test_items_ok(test1c,6,&Control,&State,3,t1c2,FALSE,0);
    test_items_ok(test2,16,&Control,&State,4,t23,FALSE,0);
    test_items_ok(test2b,11,&Control,&State,4,t2b2,FALSE,0);
    test_items_ok(test2c,11,&Control,&State,4,t2c3,FALSE,0);
    test_items_ok(test2d,11,&Control,&State,4,t2d3,FALSE,0);
    test_items_ok(test3,41,&Control,&State,1,t32,FALSE,0);
    test_items_ok(test4,12,&Control,&State,4,t42,FALSE,0);
    test_items_ok(test5,38,&Control,&State,1,t51,FALSE,0);
    test_items_ok(test6,5,&Control,&State,2,t62,FALSE,0);
    test_items_ok(test7,29,&Control,&State,3,t73,FALSE,0);
    test_items_ok(test8,4,&Control,&State,1,t81,FALSE,0);
    test_items_ok(test9,5,&Control,&State,2,t92,FALSE,0);
    test_items_ok(test10,4,&Control,&State,1,t101,FALSE,0);
    test_items_ok(test11,8,&Control,&State,1,t112,FALSE,0);
    test_items_ok(test12,5,&Control,&State,1,t122,FALSE,0);
    test_items_ok(test13,7,&Control,&State,1,t132,FALSE,0);
    test_items_ok(test14,7,&Control,&State,1,t142,FALSE,0);
    test_items_ok(test15,5,&Control,&State,1,t152,FALSE,0);
    test_items_ok(test16,5,&Control,&State,1,t162,FALSE,0);
    test_items_ok(test17,6,&Control,&State,1,t172,FALSE,0);
    test_items_ok(test18,5,&Control,&State,1,t182,FALSE,0);
    test_items_ok(test19,6,&Control,&State,1,t192,FALSE,0);
    test_items_ok(test20,5,&Control,&State,2,t202,FALSE,0);
    test_items_ok(test21,5,&Control,&State,1,t211,FALSE,0);
    test_items_ok(test22,6,&Control,&State,2,t222,FALSE,b222);
    test_items_ok(test23,6,&Control,&State,2,t232,FALSE,0);
    test_items_ok(test24,12,&Control,&State,1,t242,FALSE,0);
    test_items_ok(test25,10,&Control,&State,1,t252,FALSE,0);
    test_items_ok(test26,2,&Control,&State,1,t262,FALSE,0);
    test_items_ok(test27,8,&Control,&State,1,t272,FALSE,0);
    test_items_ok(test28,4,&Control,&State,1,t282,FALSE,0);
    test_items_ok(test29,10,&Control,&State,2,t292,FALSE,0);
    test_items_ok(test30,8,&Control,&State,1,t302,FALSE,0);
    test_items_ok(test31,8,&Control,&State,1,t312,FALSE,b312);
    test_items_ok(test32,3,&Control,&State,1,t322,FALSE,0);
    test_items_ok(test33,4,&Control,&State,1,t332,FALSE,0);
    test_items_ok(test34,3,&Control,&State,1,t342,FALSE,b342);
    test_items_ok(test35,13,&Control,&State,1,t352,FALSE,b352);
    test_items_ok(test36,7,&Control,&State,1,t362,FALSE,0);
    test_items_ok(test37,3,&Control,&State,1,t372,FALSE,0);
    test_items_ok(test38,2,&Control,&State,1,t382,FALSE,0);
    test_items_ok(test39,10,&Control,&State,1,t392,FALSE,0);
    test_items_ok(test40,6,&Control,&State,1,t402,FALSE,0);
    test_items_ok(test41,6,&Control,&State,3,t412,FALSE,b412);
    test_items_ok(test42,6,&Control,&State,1,t422,FALSE,0);
    test_items_ok(test43,7,&Control,&State,1,t432,FALSE,0);
    test_items_ok(test44,4,&Control,&State,2,t442,FALSE,0);
    test_items_ok(test45,24,&Control,&State,1,t452,FALSE,0);
    test_items_ok(test46,16,&Control,&State,1,t462,FALSE,0);
    test_items_ok(test47,26,&Control,&State,1,t472,FALSE,0);
    test_items_ok(test56,6,&Control,&State,1,t561,FALSE,0);
    test_items_ok(test57,13,&Control,&State,7,t571,FALSE,0);
    test_items_ok(test58,13,&Control,&State,1,t581,FALSE,0);

    State.uBidiLevel = 1;
    Control.fMergeNeutralItems = TRUE;
    test_items_ok(test1,4,&Control,&State,1,t12,FALSE,0);
    test_items_ok(test1b,4,&Control,&State,1,t1b2,FALSE,0);
    test_items_ok(test1c,6,&Control,&State,3,t1c2,FALSE,0);
    test_items_ok(test2,16,&Control,&State,4,t23,FALSE,0);
    test_items_ok(test2b,11,&Control,&State,2,t2b3,FALSE,b2);
    test_items_ok(test2c,11,&Control,&State,2,t2c4,FALSE,b2);
    test_items_ok(test2d,11,&Control,&State,2,t2d4,FALSE,b2);
    test_items_ok(test3,41,&Control,&State,1,t32,FALSE,0);
    test_items_ok(test4,12,&Control,&State,3,t43,FALSE,b43);
    test_items_ok(test5,38,&Control,&State,1,t51,FALSE,0);
    test_items_ok(test6,5,&Control,&State,1,t63,FALSE,b63);
    test_items_ok(test7,29,&Control,&State,3,t73,FALSE,0);
    test_items_ok(test8,4,&Control,&State,1,t81,FALSE,0);
    test_items_ok(test9,5,&Control,&State,1,t93,FALSE,b93);
    test_items_ok(test10,4,&Control,&State,1,t101,FALSE,0);
    test_items_ok(test11,8,&Control,&State,1,t112,FALSE,0);
    test_items_ok(test12,5,&Control,&State,1,t122,FALSE,0);
    test_items_ok(test13,7,&Control,&State,1,t132,FALSE,0);
    test_items_ok(test14,7,&Control,&State,1,t142,FALSE,0);
    test_items_ok(test15,5,&Control,&State,1,t152,FALSE,0);
    test_items_ok(test16,5,&Control,&State,1,t162,FALSE,0);
    test_items_ok(test17,6,&Control,&State,1,t172,FALSE,0);
    test_items_ok(test18,5,&Control,&State,1,t182,FALSE,0);
    test_items_ok(test19,6,&Control,&State,1,t192,FALSE,0);
    test_items_ok(test20,5,&Control,&State,2,t202,FALSE,0);
    test_items_ok(test21,5,&Control,&State,1,t211,FALSE,0);
    test_items_ok(test22,6,&Control,&State,1,t223,FALSE,b223);
    test_items_ok(test23,6,&Control,&State,2,t232,FALSE,0);
    test_items_ok(test24,12,&Control,&State,1,t242,FALSE,0);
    test_items_ok(test25,10,&Control,&State,1,t252,FALSE,0);
    test_items_ok(test26,2,&Control,&State,1,t262,FALSE,0);
    test_items_ok(test27,8,&Control,&State,1,t272,FALSE,0);
    test_items_ok(test28,4,&Control,&State,1,t282,FALSE,0);
    test_items_ok(test29,10,&Control,&State,2,t292,FALSE,0);
    test_items_ok(test30,8,&Control,&State,1,t302,FALSE,0);
    test_items_ok(test31,8,&Control,&State,1,t312,FALSE,b312);
    test_items_ok(test32,3,&Control,&State,1,t322,FALSE,0);
    test_items_ok(test33,4,&Control,&State,1,t332,FALSE,0);
    test_items_ok(test34,3,&Control,&State,1,t342,FALSE,b342);
    test_items_ok(test35,13,&Control,&State,1,t352,FALSE,b352);
    test_items_ok(test36,7,&Control,&State,1,t362,FALSE,0);
    test_items_ok(test37,3,&Control,&State,1,t372,FALSE,0);
    test_items_ok(test38,2,&Control,&State,1,t382,FALSE,0);
    test_items_ok(test39,10,&Control,&State,1,t392,FALSE,0);
    test_items_ok(test40,6,&Control,&State,1,t402,FALSE,0);
    test_items_ok(test41,6,&Control,&State,3,t412,FALSE,b412);
    test_items_ok(test42,6,&Control,&State,1,t422,FALSE,0);
    test_items_ok(test43,7,&Control,&State,1,t432,FALSE,0);
    test_items_ok(test44,4,&Control,&State,2,t442,FALSE,0);
    test_items_ok(test45,24,&Control,&State,1,t452,FALSE,0);
    test_items_ok(test46,16,&Control,&State,1,t462,FALSE,0);
    test_items_ok(test47,26,&Control,&State,1,t472,FALSE,0);
    test_items_ok(test56,6,&Control,&State,1,t561,FALSE,0);
    test_items_ok(test57,13,&Control,&State,7,t571,FALSE,0);
    test_items_ok(test58,13,&Control,&State,1,t581,FALSE,0);

    State.uBidiLevel = 0;
    Control.fMergeNeutralItems = FALSE;
    State.fOverrideDirection = 1;
    test_items_ok(test1,4,&Control,&State,1,t11,FALSE,0);
    test_items_ok(test1b,4,&Control,&State,1,t1b1,FALSE,0);
    test_items_ok(test1c,6,&Control,&State,1,t1c1,FALSE,0);
    test_items_ok(test2,16,&Control,&State,4,t24,FALSE,0);
    test_items_ok(test2b,11,&Control,&State,4,t2b4,FALSE,0);
    test_items_ok(test2c,11,&Control,&State,4,t2c5,FALSE,0);
    test_items_ok(test2d,11,&Control,&State,4,t2d5,FALSE,0);
    test_items_ok(test3,41,&Control,&State,1,t31,FALSE,0);
    test_items_ok(test4,12,&Control,&State,5,t41,FALSE,0);
    test_items_ok(test5,38,&Control,&State,1,t52,FALSE,0);
    test_items_ok(test6,5,&Control,&State,2,t64,FALSE,0);
    test_items_ok(test7,29,&Control,&State,3,t74,FALSE,0);
    test_items_ok(test8,4,&Control,&State,1,t82,FALSE,0);
    test_items_ok(test9,5,&Control,&State,2,t94,FALSE,0);
    test_items_ok(test10,4,&Control,&State,1,t102,FALSE,0);
    test_items_ok(test11,8,&Control,&State,1,t111,FALSE,0);
    test_items_ok(test12,5,&Control,&State,1,t121,FALSE,0);
    test_items_ok(test13,7,&Control,&State,1,t131,FALSE,0);
    test_items_ok(test14,7,&Control,&State,1,t141,FALSE,0);
    test_items_ok(test15,5,&Control,&State,1,t151,FALSE,0);
    test_items_ok(test16,5,&Control,&State,1,t161,FALSE,0);
    test_items_ok(test17,6,&Control,&State,1,t171,FALSE,0);
    test_items_ok(test18,5,&Control,&State,1,t181,FALSE,0);
    test_items_ok(test19,6,&Control,&State,1,t191,FALSE,0);
    test_items_ok(test20,5,&Control,&State,2,t201,FALSE,0);
    test_items_ok(test21,5,&Control,&State,1,t212,FALSE,0);
    test_items_ok(test22,6,&Control,&State,2,t221,FALSE,0);
    test_items_ok(test23,6,&Control,&State,2,t231,FALSE,0);
    test_items_ok(test24,12,&Control,&State,1,t241,FALSE,0);
    test_items_ok(test25,10,&Control,&State,1,t251,FALSE,0);
    test_items_ok(test26,2,&Control,&State,1,t261,FALSE,0);
    test_items_ok(test27,8,&Control,&State,1,t271,FALSE,0);
    test_items_ok(test28,4,&Control,&State,1,t281,FALSE,0);
    test_items_ok(test29,10,&Control,&State,2,t291,FALSE,0);
    test_items_ok(test30,8,&Control,&State,1,t301,FALSE,0);
    test_items_ok(test31,8,&Control,&State,1,t311,FALSE,b311);
    test_items_ok(test32,3,&Control,&State,1,t321,FALSE,0);
    test_items_ok(test33,4,&Control,&State,1,t331,FALSE,0);
    test_items_ok(test34,3,&Control,&State,1,t341,FALSE,0);
    test_items_ok(test35,13,&Control,&State,1,t353,FALSE,b351);
    test_items_ok(test36,7,&Control,&State,1,t361,FALSE,0);
    test_items_ok(test37,3,&Control,&State,1,t373,FALSE,0);
    test_items_ok(test38,2,&Control,&State,1,t381,FALSE,0);
    test_items_ok(test39,10,&Control,&State,1,t391,FALSE,0);
    test_items_ok(test40,6,&Control,&State,1,t401,FALSE,0);
    test_items_ok(test41,6,&Control,&State,1,t411,FALSE,0);
    test_items_ok(test42,6,&Control,&State,1,t421,FALSE,0);
    test_items_ok(test43,7,&Control,&State,1,t431,FALSE,0);
    test_items_ok(test44,4,&Control,&State,2,t441,FALSE,0);
    test_items_ok(test45,24,&Control,&State,1,t451,FALSE,0);
    test_items_ok(test46,16,&Control,&State,1,t461,FALSE,0);
    test_items_ok(test47,26,&Control,&State,1,t471,FALSE,0);
    test_items_ok(test48,6,&Control,&State,3,t482,FALSE,0);
    test_items_ok(test49,7,&Control,&State,2,t492,FALSE,0);
    test_items_ok(test50,6,&Control,&State,3,t502,FALSE,0);
    test_items_ok(test51,7,&Control,&State,4,t512,FALSE,0);
    test_items_ok(test52,7,&Control,&State,4,t522,FALSE,0);
    test_items_ok(test53,8,&Control,&State,4,t532,FALSE,0);
    test_items_ok(test54,7,&Control,&State,2,t542,FALSE,0);
    test_items_ok(test55,8,&Control,&State,2,t552,FALSE,0);
    test_items_ok(test56,6,&Control,&State,1,t562,FALSE,0);
    test_items_ok(test57,13,&Control,&State,7,t573,FALSE,0);
    test_items_ok(test58,13,&Control,&State,1,t582,FALSE,0);
}

static void make_surrogate(DWORD i, WORD out[2])
{
    static const DWORD mask = (1 << 10) - 1;

    if (i <= 0xffff)
    {
        out[0] = i;
        out[1] = 0;
    }
    else
    {
        i -= 0x010000;
        out[0] = ((i >> 10) & mask) + 0xd800;
        out[1] = (i & mask) + 0xdc00;
    }
}

static void test_ScriptItemize_surrogates(void)
{
    HRESULT hr;
    WCHAR surrogate[2];
    WORD Script_Surrogates;
    SCRIPT_ITEM items[2];
    int num;

    /* Find Script_Surrogates */
    surrogate[0] = 0xd800;
    hr = ScriptItemize( surrogate, 1, 2, NULL, NULL, items, &num );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( num == 1, "got %d\n", num );
    ok( items[0].a.eScript != SCRIPT_UNDEFINED, "got script %x\n", items[0].a.eScript );
    Script_Surrogates = items[0].a.eScript;

    /* Show that an invalid character has script Script_Surrogates */
    make_surrogate( 0x01ffff, surrogate );
    hr = ScriptItemize( surrogate, 2, 2, NULL, NULL, items, &num );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( num == 1, "got %d\n", num );
    ok( items[0].a.eScript == Script_Surrogates, "got script %x\n", items[0].a.eScript );
}

static inline void _test_shape_ok(int valid, HDC hdc, LPCWSTR string,
                         DWORD cchString, SCRIPT_CONTROL *Control,
                         SCRIPT_STATE *State, DWORD item, DWORD nGlyphs,
                         const shapeTest_char *charItems,
                         const shapeTest_glyph *glyphItems,
                         const SCRIPT_GLYPHPROP *props2)
{
    HRESULT hr;
    int x, outnItems = 0, outnGlyphs = 0, outnGlyphs2 = 0;
    const SCRIPT_PROPERTIES **script_properties;
    SCRIPT_ITEM outpItems[15];
    SCRIPT_CACHE sc = NULL;
    WORD *glyphs, *glyphs2;
    WORD *logclust, *logclust2;
    int maxGlyphs = cchString * 1.5;
    SCRIPT_GLYPHPROP *glyphProp, *glyphProp2;
    SCRIPT_CHARPROP  *charProp, *charProp2;
    int script_count;
    WCHAR *string2;
    ULONG tags[15];

    hr = ScriptGetProperties(&script_properties, &script_count);
    winetest_ok(SUCCEEDED(hr), "Failed to get script properties, hr %#x.\n", hr);

    hr = pScriptItemizeOpenType(string, cchString, 15, Control, State, outpItems, tags, &outnItems);
    if (valid > 0)
        winetest_ok(hr == S_OK, "ScriptItemizeOpenType should return S_OK not %08x\n", hr);
    else if (hr != S_OK)
        winetest_trace("ScriptItemizeOpenType should return S_OK not %08x\n", hr);

    if (outnItems <= item)
    {
        if (valid > 0)
            winetest_win_skip("Did not get enough items\n");
        else
            winetest_trace("Did not get enough items\n");
        return;
    }

    logclust = HeapAlloc(GetProcessHeap(), 0, sizeof(WORD) * cchString);
    memset(logclust,'a',sizeof(WORD) * cchString);
    charProp = HeapAlloc(GetProcessHeap(), 0, sizeof(SCRIPT_CHARPROP) * cchString);
    memset(charProp,'a',sizeof(SCRIPT_CHARPROP) * cchString);
    glyphs = HeapAlloc(GetProcessHeap(), 0, sizeof(WORD) * maxGlyphs);
    memset(glyphs,'a',sizeof(WORD) * cchString);
    glyphProp = HeapAlloc(GetProcessHeap(), 0, sizeof(SCRIPT_GLYPHPROP) * maxGlyphs);
    memset(glyphProp,'a',sizeof(SCRIPT_GLYPHPROP) * cchString);

    string2 = HeapAlloc(GetProcessHeap(), 0, cchString * sizeof(*string2));
    logclust2 = HeapAlloc(GetProcessHeap(), 0, cchString * sizeof(*logclust2));
    memset(logclust2, 'a', cchString * sizeof(*logclust2));
    charProp2 = HeapAlloc(GetProcessHeap(), 0, cchString * sizeof(*charProp2));
    memset(charProp2, 'a', cchString * sizeof(*charProp2));
    glyphs2 = HeapAlloc(GetProcessHeap(), 0, maxGlyphs * sizeof(*glyphs2));
    memset(glyphs2, 'a', maxGlyphs * sizeof(*glyphs2));
    glyphProp2 = HeapAlloc(GetProcessHeap(), 0, maxGlyphs * sizeof(*glyphProp2));
    memset(glyphProp2, 'a', maxGlyphs * sizeof(*glyphProp2));

    winetest_ok(!outpItems[item].a.fLogicalOrder, "Got unexpected fLogicalOrder %#x.\n",
            outpItems[item].a.fLogicalOrder);
    hr = pScriptShapeOpenType(hdc, &sc, &outpItems[item].a, tags[item], 0x00000000, NULL, NULL, 0, string, cchString, maxGlyphs, logclust, charProp, glyphs, glyphProp, &outnGlyphs);
    if (valid > 0)
        winetest_ok(hr == S_OK, "ScriptShapeOpenType failed (%x)\n",hr);
    else if (hr != S_OK)
        winetest_trace("ScriptShapeOpenType failed (%x)\n",hr);
    if (FAILED(hr))
        goto cleanup;

    for (x = 0; x < cchString; x++)
    {
        if (valid > 0)
            winetest_ok(logclust[x] == charItems[x].wLogClust, "%i: invalid LogClust(%i)\n",x,logclust[x]);
        else if (logclust[x] != charItems[x].wLogClust)
            winetest_trace("%i: invalid LogClust(%i)\n",x,logclust[x]);
        if (valid > 0)
            winetest_ok(charProp[x].fCanGlyphAlone == charItems[x].CharProp.fCanGlyphAlone, "%i: invalid fCanGlyphAlone\n",x);
        else if (charProp[x].fCanGlyphAlone != charItems[x].CharProp.fCanGlyphAlone)
            winetest_trace("%i: invalid fCanGlyphAlone\n",x);
    }

    if (valid > 0)
        winetest_ok(nGlyphs == outnGlyphs, "got incorrect number of glyphs (%i)\n",outnGlyphs);
    else if (nGlyphs != outnGlyphs)
        winetest_trace("got incorrect number of glyphs (%i)\n",outnGlyphs);
    for (x = 0; x < outnGlyphs; x++)
    {
        if (glyphItems[x].Glyph)
        {
            if (valid > 0)
                winetest_ok(glyphs[x]!=0, "%i: Glyph not present when it should be\n",x);
            else if (glyphs[x]==0)
                winetest_trace("%i: Glyph not present when it should be\n",x);
        }
        else
        {
            if (valid > 0)
                winetest_ok(glyphs[x]==0, "%i: Glyph present when it should not be\n",x);
            else if (glyphs[x]!=0)
                winetest_trace("%i: Glyph present when it should not be\n",x);
        }
        if (valid > 0)
        {
            todo_wine_if(tags[item] == syrc_tag && !x)
                winetest_ok(glyphProp[x].sva.uJustification == glyphItems[x].GlyphProp.sva.uJustification ||
                            (props2 && glyphProp[x].sva.uJustification == props2[x].sva.uJustification),
                             "%i: uJustification incorrect (%i)\n",x,glyphProp[x].sva.uJustification);
        }
        else if (glyphProp[x].sva.uJustification != glyphItems[x].GlyphProp.sva.uJustification)
        {
            winetest_trace("%i: uJustification incorrect (%i)\n",x,glyphProp[x].sva.uJustification);
        }
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fClusterStart == glyphItems[x].GlyphProp.sva.fClusterStart ||
                        (props2 && glyphProp[x].sva.fClusterStart == props2[x].sva.fClusterStart),
                        "%i: fClusterStart incorrect (%i)\n",x,glyphProp[x].sva.fClusterStart);
        else if (glyphProp[x].sva.fClusterStart != glyphItems[x].GlyphProp.sva.fClusterStart)
            winetest_trace("%i: fClusterStart incorrect (%i)\n",x,glyphProp[x].sva.fClusterStart);
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fDiacritic == glyphItems[x].GlyphProp.sva.fDiacritic ||
                        (props2 && glyphProp[x].sva.fDiacritic == props2[x].sva.fDiacritic),
                        "%i: fDiacritic incorrect (%i)\n",x,glyphProp[x].sva.fDiacritic);
        else if (glyphProp[x].sva.fDiacritic != glyphItems[x].GlyphProp.sva.fDiacritic)
            winetest_trace("%i: fDiacritic incorrect (%i)\n",x,glyphProp[x].sva.fDiacritic);
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fZeroWidth == glyphItems[x].GlyphProp.sva.fZeroWidth ||
                        (props2 && glyphProp[x].sva.fZeroWidth == props2[x].sva.fZeroWidth),
                        "%i: fZeroWidth incorrect (%i)\n",x,glyphProp[x].sva.fZeroWidth);
        else if (glyphProp[x].sva.fZeroWidth != glyphItems[x].GlyphProp.sva.fZeroWidth)
            winetest_trace("%i: fZeroWidth incorrect (%i)\n",x,glyphProp[x].sva.fZeroWidth);
    }

    outpItems[item].a.fLogicalOrder = 1;
    hr = pScriptShapeOpenType(hdc, &sc, &outpItems[item].a, tags[item], 0x00000000, NULL, NULL, 0,
            string, cchString, maxGlyphs, logclust2, charProp2, glyphs2, glyphProp2, &outnGlyphs2);
    winetest_ok(hr == S_OK, "ScriptShapeOpenType failed (%x)\n",hr);
    /* Cluster maps are hard. */
    if (tags[item] != thaa_tag && tags[item] != syrc_tag)
    {
        for (x = 0; x < cchString; ++x)
        {
            unsigned int compare_idx = outpItems[item].a.fRTL ? cchString - x - 1 : x;
            winetest_ok(logclust2[x] == logclust[compare_idx],
                    "Got unexpected logclust2[%u] %#x, expected %#x.\n",
                    x, logclust2[x], logclust[compare_idx]);
            winetest_ok(charProp2[x].fCanGlyphAlone == charProp[compare_idx].fCanGlyphAlone,
                    "Got unexpected charProp2[%u].fCanGlyphAlone %#x, expected %#x.\n",
                    x, charProp2[x].fCanGlyphAlone, charProp[compare_idx].fCanGlyphAlone);
        }
    }
    winetest_ok(outnGlyphs2 == outnGlyphs, "Got unexpected glyph count %u.\n", outnGlyphs2);
    for (x = 0; x < outnGlyphs2; ++x)
    {
        unsigned int compare_idx = outpItems[item].a.fRTL ? outnGlyphs2 - x - 1 : x;
        winetest_ok(glyphs2[x] == glyphs[compare_idx], "Got unexpected glyphs2[%u] %#x, expected %#x.\n",
                x, glyphs2[x], glyphs[compare_idx]);
        winetest_ok(glyphProp2[x].sva.uJustification == glyphProp[compare_idx].sva.uJustification,
                "Got unexpected glyphProp2[%u].sva.uJustification %#x, expected %#x.\n",
                x, glyphProp2[x].sva.uJustification, glyphProp[compare_idx].sva.uJustification);
        winetest_ok(glyphProp2[x].sva.fClusterStart == glyphProp[compare_idx].sva.fClusterStart,
                "Got unexpected glyphProp2[%u].sva.fClusterStart %#x, expected %#x.\n",
                x, glyphProp2[x].sva.fClusterStart, glyphProp[compare_idx].sva.fClusterStart);
        winetest_ok(glyphProp2[x].sva.fDiacritic == glyphProp[compare_idx].sva.fDiacritic,
                "Got unexpected glyphProp2[%u].sva.fDiacritic %#x, expected %#x.\n",
                x, glyphProp2[x].sva.fDiacritic, glyphProp[compare_idx].sva.fDiacritic);
        winetest_ok(glyphProp2[x].sva.fZeroWidth == glyphProp[compare_idx].sva.fZeroWidth,
                "Got unexpected glyphProp2[%u].sva.fZeroWidth %#x, expected %#x.\n",
                x, glyphProp2[x].sva.fZeroWidth, glyphProp[compare_idx].sva.fZeroWidth);
    }

    /* Most scripts get this wrong. For example, when the font has the
     * appropriate ligatures, "ttfffi" get rendered as "<ttf><ffi>", but
     * "<RLO>iffftt" gets rendered as "t<ft><ff>i". Arabic gets it right,
     * and there exist applications that depend on that. */
    if (tags[item] == arab_tag && broken(script_count <= 75))
    {
        winetest_win_skip("Test broken on this platform, skipping.\n");
    }
    else if (tags[item] == arab_tag)
    {
        for (x = 0; x < cchString; ++x)
        {
            string2[x] = string[cchString - x - 1];
        }
        outpItems[item].a.fLogicalOrder = 0;
        outpItems[item].a.fRTL = !outpItems[item].a.fRTL;
        hr = pScriptShapeOpenType(hdc, &sc, &outpItems[item].a, tags[item], 0x00000000, NULL, NULL, 0,
                string2, cchString, maxGlyphs, logclust2, charProp2, glyphs2, glyphProp2, &outnGlyphs2);
        winetest_ok(hr == S_OK, "ScriptShapeOpenType failed (%x)\n",hr);
        for (x = 0; x < cchString; ++x)
        {
            unsigned int compare_idx = cchString - x - 1;
            winetest_ok(logclust2[x] == logclust[compare_idx],
                    "Got unexpected logclust2[%u] %#x, expected %#x.\n",
                    x, logclust2[x], logclust[compare_idx]);
            winetest_ok(charProp2[x].fCanGlyphAlone == charProp[compare_idx].fCanGlyphAlone,
                    "Got unexpected charProp2[%u].fCanGlyphAlone %#x, expected %#x.\n",
                    x, charProp2[x].fCanGlyphAlone, charProp[compare_idx].fCanGlyphAlone);
        }
        winetest_ok(outnGlyphs2 == outnGlyphs, "Got unexpected glyph count %u.\n", outnGlyphs2);
        for (x = 0; x < outnGlyphs2; ++x)
        {
            winetest_ok(glyphs2[x] == glyphs[x], "Got unexpected glyphs2[%u] %#x, expected %#x.\n",
                    x, glyphs2[x], glyphs[x]);
            winetest_ok(glyphProp2[x].sva.uJustification == glyphProp[x].sva.uJustification,
                    "Got unexpected glyphProp2[%u].sva.uJustification %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.uJustification, glyphProp[x].sva.uJustification);
            winetest_ok(glyphProp2[x].sva.fClusterStart == glyphProp[x].sva.fClusterStart,
                    "Got unexpected glyphProp2[%u].sva.fClusterStart %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fClusterStart, glyphProp[x].sva.fClusterStart);
            winetest_ok(glyphProp2[x].sva.fDiacritic == glyphProp[x].sva.fDiacritic,
                    "Got unexpected glyphProp2[%u].sva.fDiacritic %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fDiacritic, glyphProp[x].sva.fDiacritic);
            winetest_ok(glyphProp2[x].sva.fZeroWidth == glyphProp[x].sva.fZeroWidth,
                    "Got unexpected glyphProp2[%u].sva.fZeroWidth %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fZeroWidth, glyphProp[x].sva.fZeroWidth);
        }
        outpItems[item].a.fLogicalOrder = 1;
        hr = pScriptShapeOpenType(hdc, &sc, &outpItems[item].a, tags[item], 0x00000000, NULL, NULL, 0,
                string2, cchString, maxGlyphs, logclust2, charProp2, glyphs2, glyphProp2, &outnGlyphs2);
        winetest_ok(hr == S_OK, "ScriptShapeOpenType failed (%x)\n",hr);
        for (x = 0; x < cchString; ++x)
        {
            unsigned int compare_idx = outpItems[item].a.fRTL ? x : cchString - x - 1;
            winetest_ok(logclust2[x] == logclust[compare_idx], "Got unexpected logclust2[%u] %#x, expected %#x.\n",
                    x, logclust2[x], logclust[compare_idx]);
            winetest_ok(charProp2[x].fCanGlyphAlone == charProp[compare_idx].fCanGlyphAlone,
                    "Got unexpected charProp2[%u].fCanGlyphAlone %#x, expected %#x.\n",
                    x, charProp2[x].fCanGlyphAlone, charProp[compare_idx].fCanGlyphAlone);
        }
        winetest_ok(outnGlyphs2 == outnGlyphs, "Got unexpected glyph count %u.\n", outnGlyphs2);
        for (x = 0; x < outnGlyphs2; ++x)
        {
            unsigned int compare_idx = outpItems[item].a.fRTL ? outnGlyphs2 - x - 1 : x;
            winetest_ok(glyphs2[x] == glyphs[compare_idx], "Got unexpected glyphs2[%u] %#x, expected %#x.\n",
                    x, glyphs2[x], glyphs[compare_idx]);
            winetest_ok(glyphProp2[x].sva.uJustification == glyphProp[compare_idx].sva.uJustification,
                    "Got unexpected glyphProp2[%u].sva.uJustification %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.uJustification, glyphProp[compare_idx].sva.uJustification);
            winetest_ok(glyphProp2[x].sva.fClusterStart == glyphProp[compare_idx].sva.fClusterStart,
                    "Got unexpected glyphProp2[%u].sva.fClusterStart %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fClusterStart, glyphProp[compare_idx].sva.fClusterStart);
            winetest_ok(glyphProp2[x].sva.fDiacritic == glyphProp[compare_idx].sva.fDiacritic,
                    "Got unexpected glyphProp2[%u].sva.fDiacritic %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fDiacritic, glyphProp[compare_idx].sva.fDiacritic);
            winetest_ok(glyphProp2[x].sva.fZeroWidth == glyphProp[compare_idx].sva.fZeroWidth,
                    "Got unexpected glyphProp2[%u].sva.fZeroWidth %#x, expected %#x.\n",
                    x, glyphProp2[x].sva.fZeroWidth, glyphProp[compare_idx].sva.fZeroWidth);
        }
    }

cleanup:
    HeapFree(GetProcessHeap(),0,string2);
    HeapFree(GetProcessHeap(),0,logclust2);
    HeapFree(GetProcessHeap(),0,charProp2);
    HeapFree(GetProcessHeap(),0,glyphs2);
    HeapFree(GetProcessHeap(),0,glyphProp2);

    HeapFree(GetProcessHeap(),0,logclust);
    HeapFree(GetProcessHeap(),0,charProp);
    HeapFree(GetProcessHeap(),0,glyphs);
    HeapFree(GetProcessHeap(),0,glyphProp);
    ScriptFreeCache(&sc);
}

#define test_shape_ok(a,b,c,d,e,f,g,h,i) \
    (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_shape_ok(1,a,b,c,d,e,f,g,h,i,NULL)

#define test_shape_ok_valid(v,a,b,c,d,e,f,g,h,i) \
    (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_shape_ok(v,a,b,c,d,e,f,g,h,i,NULL)

#define test_shape_ok_valid_props2(v,a,b,c,d,e,f,g,h,i,j) \
    (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_shape_ok(v,a,b,c,d,e,f,g,h,i,j)

typedef struct tagRangeP {
    BYTE range;
    LOGFONTA lf;
} fontEnumParam;

static int CALLBACK enumFontProc( const LOGFONTA *lpelfe, const TEXTMETRICA *lpntme, DWORD FontType, LPARAM lParam )
{
    NEWTEXTMETRICEXA *ntme = (NEWTEXTMETRICEXA*)lpntme;
    fontEnumParam *rp = (fontEnumParam*) lParam;
    int idx = 0;
    DWORD i;
    DWORD mask = 0;

    if (FontType != TRUETYPE_FONTTYPE)
        return 1;

    i = rp->range;
    while (i >= sizeof(DWORD)*8)
    {
        idx++;
        i -= (sizeof(DWORD)*8);
    }
    if (idx > 3)
        return 0;

    mask = 1 << i;

    if (ntme->ntmFontSig.fsUsb[idx] & mask)
    {
        memcpy(&(rp->lf),lpelfe,sizeof(LOGFONTA));
        return 0;
    }
    return 1;
}

static int _find_font_for_range(HDC hdc, const CHAR *recommended, BYTE range, const WCHAR check, HFONT *hfont, HFONT *origFont, const font_fingerprint *fingerprint)
{
    int rc = 0;
    fontEnumParam lParam;

    lParam.range = range;
    memset(&lParam.lf,0,sizeof(LOGFONTA));
    *hfont = NULL;

    if (recommended)
    {
        lstrcpyA(lParam.lf.lfFaceName, recommended);
        if (!EnumFontFamiliesExA(hdc, &lParam.lf, enumFontProc, (LPARAM)&lParam, 0))
        {
            *hfont = CreateFontIndirectA(&lParam.lf);
            if (*hfont)
            {
                winetest_trace("using font %s\n",lParam.lf.lfFaceName);
                if (fingerprint)
                {
                    WORD output[10];
                    int i;

                    *origFont = SelectObject(hdc,*hfont);
                    if (GetGlyphIndicesW(hdc, fingerprint->check, 10, output, 0) != GDI_ERROR)
                    {
                        for (i=0; i < 10; i++)
                            if (output[i] != fingerprint->result[i])
                            {
                                winetest_trace("found font does not match fingerprint\n");
                                SelectObject(hdc,*origFont);
                                DeleteObject(*hfont);
                                *hfont = NULL;
                                break;
                            }
                        if (i == 10) rc = 1;
                    }
                    SelectObject(hdc, *origFont);
                }
                else rc = 1;
            }
        }
        if (!rc)
            winetest_skip("Font %s is not available.\n", recommended);
    }

    if (!*hfont)
    {
        memset(&lParam.lf,0,sizeof(LOGFONTA));
        lParam.lf.lfCharSet = DEFAULT_CHARSET;

        if (!EnumFontFamiliesExA(hdc, &lParam.lf, enumFontProc, (LPARAM)&lParam, 0) && lParam.lf.lfFaceName[0])
        {
            *hfont = CreateFontIndirectA(&lParam.lf);
            if (*hfont)
                winetest_trace("trying font %s: failures will only be warnings\n",lParam.lf.lfFaceName);
        }
    }

    if (*hfont)
    {
        WORD glyph = 0;

        *origFont = SelectObject(hdc,*hfont);
        if (GetGlyphIndicesW(hdc, &check, 1, &glyph, 0) == GDI_ERROR || glyph == 0)
        {
            winetest_trace("    Font fails to contain required glyphs\n");
            SelectObject(hdc,*origFont);
            DeleteObject(*hfont);
            *hfont=NULL;
            rc = 0;
        }
        else if (!rc)
            rc = -1;
    }
    else
        winetest_trace("Failed to find usable font\n");

    return rc;
}

#define find_font_for_range(a,b,c,d,e,f,g) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _find_font_for_range(a,b,c,d,e,f,g)

static void test_ScriptShapeOpenType(HDC hdc)
{
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    WORD glyphs[4], logclust[4];
    SCRIPT_GLYPHPROP glyphProp[4];
    SCRIPT_ITEM items[2];
    ULONG tags[2];
    SCRIPT_CONTROL  Control;
    SCRIPT_STATE    State;
    int nb, outnItems;
    HFONT hfont, hfont_orig;
    int test_valid;
    shapeTest_glyph glyph_test[4];

    static const WCHAR test1[] = {'w', 'i', 'n', 'e',0};
    static const shapeTest_char t1_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph t1_g[] = {
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}} };

    static const WCHAR test2[] = {0x202B, 'i', 'n', 0x202C,0};
    static const shapeTest_char t2_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph t2_g[] = {
                            {0,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {0,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}} };

    static const WCHAR test3[] = {'t', 't', 'f', 'f', 'f', 'i', 0};
    static const shapeTest_char t3_c[] = {{0, {0, 0}}, {0, {0, 0}}, {0, {0, 0}},
            {1, {0, 0}}, {1, {0, 0}}, {1, {0, 0}}};
    static const shapeTest_glyph t3_g[] = {
                            {1, {{SCRIPT_JUSTIFY_CHARACTER, 1, 0, 0, 0, 0}, 0}},
                            {1, {{SCRIPT_JUSTIFY_CHARACTER, 1, 0, 0, 0, 0}, 0}}};

    /* Hebrew */
    static const WCHAR test_hebrew[]  = {0x05e9, 0x05dc, 0x05d5, 0x05dd,0};
    static const shapeTest_char hebrew_c[] = {{3,{0,0}},{2,{0,0}},{1,{0,0}},{0,{0,0}}};
    static const shapeTest_glyph hebrew_g[] = {
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}} };

    /* Arabic */
    static const WCHAR test_arabic[] = {0x0633,0x0644,0x0627,0x0645,0};
    static const shapeTest_char arabic_c[] = {{2,{0,0}},{1,{0,0}},{1,{0,0}},{0,{0,0}}};
    static const shapeTest_glyph arabic_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_ARABIC_NORMAL,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_ARABIC_SEEN,1,0,0,0,0},0}} };

    /* Thai */
    static const WCHAR test_thai[] = {0x0e2a, 0x0e04, 0x0e23, 0x0e34, 0x0e1b, 0x0e15, 0x0e4c, 0x0e44, 0x0e17, 0x0e22,};
    static const shapeTest_char thai_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{2,{0,0}},{4,{0,0}},{5,{0,0}},{5,{0,0}},{7,{0,0}},{8,{0,0}},{9,{0,0}}};
    static const shapeTest_glyph thai_g[] = {
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}}};

    /* Syriac */
    static const WCHAR test_syriac[] = {0x0710, 0x072c, 0x0728, 0x0742, 0x0718, 0x0723, 0x0720, 0x0710, 0};
    static const shapeTest_char syriac_c[] = {{6, {0, 0}}, {5, {0, 0}}, {4, {0, 0}},
            {4, {0, 0}}, {2, {0, 0}}, {1, {0, 0}}, {0, {0, 0}}, {0, {0, 0}}};
    static const shapeTest_glyph syriac_g[] = {
                            {1,{{SCRIPT_JUSTIFY_ARABIC_NORMAL,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };

    /* Thaana */
    static const WCHAR test_thaana[] = {0x078a, 0x07ae, 0x0792, 0x07b0, 0x0020, 0x0796, 0x07aa, 0x0789, 0x07b0, 0x0795, 0x07ac, 0x0791, 0x07b0};
    static const shapeTest_char thaana_c[] = {{12,{0,0}},{12,{0,0}},{10,{0,0}},{10,{0,0}},{8,{1,0}},{7,{0,0}},{7,{0,0}},{5,{0,0}},{5,{0,0}},{3,{0,0}},{3,{0,0}},{1,{0,0}},{1,{0,0}}};
    static const shapeTest_glyph thaana_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };

    /* Phags-pa */
    static const WCHAR test_phagspa[] = {0xa84f, 0xa861, 0xa843, 0x0020, 0xa863, 0xa861, 0xa859, 0x0020, 0xa850, 0xa85c, 0xa85e};
    static const shapeTest_char phagspa_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{3,{1,0}},{4,{0,0}},{5,{0,0}},{6,{0,0}},{7,{1,0}},{8,{0,0}},{9,{0,0}},{10,{0,0}}};
    static const shapeTest_glyph phagspa_g[] = {
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };
    static const SCRIPT_GLYPHPROP phagspa_win10_props[] = {
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0} };

    /* Lao */
    static const WCHAR test_lao[] = {0x0ead, 0x0eb1, 0x0e81, 0x0eaa, 0x0ead, 0x0e99, 0x0ea5, 0x0eb2, 0x0ea7, 0};
    static const shapeTest_char lao_c[] = {{0,{0,0}},{0,{0,0}},{2,{0,0}},{3,{0,0}},{4,{0,0}},{5,{0,0}},{6,{0,0}},{7,{0,0}},{8,{0,0}}};
    static const shapeTest_glyph lao_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,0,1,1,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_CHARACTER,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };

    /* Tibetan */
    static const WCHAR test_tibetan[] = {0x0f04, 0x0f05, 0x0f0e, 0x0020, 0x0f51, 0x0f7c, 0x0f53, 0x0f0b, 0x0f5a, 0x0f53, 0x0f0b, 0x0f51, 0x0f44, 0x0f0b, 0x0f54, 0x0f7c, 0x0f0d};
    static const shapeTest_char tibetan_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{3,{1,0}},{4,{0,0}},{4,{0,0}},{6,{0,0}},{7,{0,0}},{8,{0,0}},{9,{0,0}},{10,{0,0}},{11,{0,0}},{12,{0,0}},{13,{0,0}},{14,{0,0}},{14,{0,0}},{16,{0,0}}};
    static const shapeTest_glyph tibetan_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_BLANK,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };
    static const SCRIPT_GLYPHPROP tibetan_win10_props[] = {
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,0,1,1,0,0},0},
                            {{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0} };

    /* Devanagari */
    static const WCHAR test_devanagari[] = {0x0926, 0x0947, 0x0935, 0x0928, 0x093e, 0x0917, 0x0930, 0x0940};
    static const shapeTest_char devanagari_c[] = {{0,{0,0}},{0,{0,0}},{2,{0,0}},{3,{0,0}},{3,{0,0}},{5,{0,0}},{6,{0,0}},{6,{0,0}}};
    static const shapeTest_glyph devanagari_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Bengali */
    static const WCHAR test_bengali[] = {0x09ac, 0x09be, 0x0982, 0x09b2, 0x09be};
    static const shapeTest_char bengali_c[] = {{0,{0,0}},{0,{0,0}},{0,{0,0}},{3,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph bengali_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Gurmukhi */
    static const WCHAR test_gurmukhi[] = {0x0a17, 0x0a41, 0x0a30, 0x0a2e, 0x0a41, 0x0a16, 0x0a40};
    static const shapeTest_char gurmukhi_c[] = {{0,{0,0}},{0,{0,0}},{2,{0,0}},{3,{0,0}},{3,{0,0}},{5,{0,0}},{5,{0,0}}};
    static const shapeTest_glyph gurmukhi_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Gujarati */
    static const WCHAR test_gujarati[] = {0x0a97, 0x0ac1, 0x0a9c, 0x0ab0, 0x0abe, 0x0aa4, 0x0ac0};
    static const shapeTest_char gujarati_c[] = {{0,{0,0}},{0,{0,0}},{2,{0,0}},{3,{0,0}},{3,{0,0}},{5,{0,0}},{5,{0,0}}};
    static const shapeTest_glyph gujarati_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Oriya */
    static const WCHAR test_oriya[] = {0x0b13, 0x0b21, 0x0b3c, 0x0b3f, 0x0b06};
    static const shapeTest_char oriya_c[] = {{0,{0,0}},{1,{0,0}},{1,{0,0}},{1,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph oriya_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };

    /* Tamil */
    static const WCHAR test_tamil[] = {0x0ba4, 0x0bae, 0x0bbf, 0x0bb4, 0x0bcd};
    static const shapeTest_char tamil_c[] = {{0,{0,0}},{1,{0,0}},{1,{0,0}},{3,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph tamil_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}} };

    /* Telugu */
    static const WCHAR test_telugu[] = {0x0c24, 0x0c46, 0x0c32, 0x0c41, 0x0c17, 0x0c41};
    static const shapeTest_char telugu_c[] = {{0,{0,0}},{0,{0,0}},{2,{0,0}},{2,{0,0}},{4,{0,0}},{4,{0,0}}};
    static const shapeTest_glyph telugu_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Malayalam */
    static const WCHAR test_malayalam[] = {0x0d2e, 0x0d32, 0x0d2f, 0x0d3e, 0x0d33, 0x0d02};
    static const shapeTest_char malayalam_c[] = {{0,{0,0}},{1,{0,0}},{2,{0,0}},{2,{0,0}},{4,{0,0}},{4,{0,0}}};
    static const shapeTest_glyph malayalam_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    /* Kannada */
    static const WCHAR test_kannada[] = {0x0c95, 0x0ca8, 0x0ccd, 0x0ca8, 0x0ca1};
    static const shapeTest_char kannada_c[] = {{0,{0,0}},{1,{0,0}},{1,{0,0}},{1,{0,0}},{3,{0,0}}};
    static const shapeTest_glyph kannada_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,0,0,0,0,0},0}} };

    static const font_fingerprint fingerprint_estrangelo = {
        {'A','a','B','b','C','c','D','d',0,0},
        {284,310,285,311,286,312,287,313,0,0}};


    if (!pScriptItemizeOpenType || !pScriptShapeOpenType)
    {
        win_skip("ScriptShapeOpenType not available on this platform\n");
        return;
    }

    memset(&Control, 0 , sizeof(Control));
    memset(&State, 0 , sizeof(State));

    hr = pScriptItemizeOpenType(test1, 4, 2, &Control, &State, items, tags, &outnItems);
    ok(hr == S_OK, "ScriptItemizeOpenType should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = pScriptShapeOpenType(hdc, &sc, &items[0].a, tags[0], 0x00000000, NULL, NULL, 0, test1, 4, 4, NULL, NULL, glyphs, NULL, &nb);
    ok(hr == E_INVALIDARG, "ScriptShapeOpenType should return E_INVALIDARG not %08x\n", hr);

    hr = pScriptShapeOpenType(hdc, &sc, &items[0].a, tags[0], 0x00000000, NULL, NULL, 0, test1, 4, 4, NULL, NULL, glyphs, glyphProp, NULL);
    ok(hr == E_INVALIDARG, "ScriptShapeOpenType should return E_INVALIDARG not %08x\n", hr);

    hr = pScriptShapeOpenType(NULL, &sc, &items[0].a, tags[0], 0x00000000, NULL, NULL, 0, test1, 4, 4, NULL, NULL, glyphs, glyphProp, &nb);
    ok(hr == E_INVALIDARG, "ScriptShapeOpenType should return E_PENDING not %08x\n", hr);

    hr = pScriptShapeOpenType(hdc, &sc, &items[0].a, tags[0], 0x00000000, NULL, NULL, 0, test1, 4, 4, NULL, NULL, glyphs, glyphProp, &nb);
    ok( hr == E_INVALIDARG,
       "ScriptShapeOpenType should return E_FAIL or E_INVALIDARG, not %08x\n", hr);
    hr = pScriptShapeOpenType(hdc, &sc, &items[0].a, tags[0], 0x00000000, NULL, NULL, 0, test1, 4, 4, logclust, NULL, glyphs, glyphProp, &nb);
    ok(hr == E_INVALIDARG, "ScriptShapeOpenType should return E_INVALIDARG not %08x\n", hr);

    ScriptFreeCache(&sc);

    test_shape_ok(hdc, test1, 4, &Control, &State, 0, 4, t1_c, t1_g);

    /* newer Tahoma has zerowidth space glyphs for 0x202b and 0x202c */
    memcpy(glyph_test, t2_g, sizeof(glyph_test));
    GetGlyphIndicesW(hdc, test2, 4, glyphs, 0);
    if (glyphs[0] != 0)
        glyph_test[0].Glyph = 1;
    if (glyphs[3] != 0)
        glyph_test[3].Glyph = 1;

    test_shape_ok(hdc, test2, 4, &Control, &State, 1, 4, t2_c, glyph_test);

    test_valid = find_font_for_range(hdc, "Calibri", 0, test3[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test3, 6, &Control, &State, 0, 2, t3_c, t3_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 11, test_hebrew[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_hebrew, 4, &Control, &State, 0, 4, hebrew_c, hebrew_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 13, test_arabic[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_arabic, 4, &Control, &State, 0, 3, arabic_c, arabic_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 24, test_thai[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_thai, 10, &Control, &State, 0, 10, thai_c, thai_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Estrangelo Edessa", 71, test_syriac[0], &hfont, &hfont_orig, &fingerprint_estrangelo);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_syriac, 8, &Control, &State, 0, 7, syriac_c, syriac_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "MV Boli", 72, test_thaana[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_thaana, 13, &Control, &State, 0, 13, thaana_c, thaana_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft PhagsPa", 53, test_phagspa[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid_props2(test_valid, hdc, test_phagspa, 11, &Control, &State, 0, 11,
                                   phagspa_c, phagspa_g, phagspa_win10_props);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "DokChampa", 25, test_lao[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_lao, 9, &Control, &State, 0, 9, lao_c, lao_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Himalaya", 70, test_tibetan[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid_props2(test_valid, hdc, test_tibetan, 17, &Control, &State, 0, 17,
                                   tibetan_c, tibetan_g, tibetan_win10_props);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Mangal", 15, test_devanagari[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_devanagari, 8, &Control, &State, 0, 8, devanagari_c, devanagari_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Vrinda", 16, test_bengali[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_bengali, 5, &Control, &State, 0, 5, bengali_c, bengali_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Raavi", 17, test_gurmukhi[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_gurmukhi, 7, &Control, &State, 0, 7, gurmukhi_c, gurmukhi_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Shruti", 18, test_gujarati[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_gujarati, 7, &Control, &State, 0, 7, gujarati_c, gujarati_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Kalinga", 19, test_oriya[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_oriya, 5, &Control, &State, 0, 4, oriya_c, oriya_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Latha", 20, test_tamil[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_tamil, 5, &Control, &State, 0, 4, tamil_c, tamil_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Gautami", 21, test_telugu[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_telugu, 6, &Control, &State, 0, 6, telugu_c, telugu_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Kartika", 23, test_malayalam[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_malayalam, 6, &Control, &State, 0, 6, malayalam_c, malayalam_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Tunga", 22, test_kannada[0], &hfont, &hfont_orig, NULL);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_kannada, 5, &Control, &State, 0, 4, kannada_c, kannada_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }
}

static void test_ScriptShape(HDC hdc)
{
    static const WCHAR test1[] = {'w', 'i', 'n', 'e',0};
    static const WCHAR test2[] = {0x202B, 'i', 'n', 0x202C,0};
    static const WCHAR test3[] = {0x30b7};
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    SCRIPT_CACHE sc2 = NULL;
    WORD glyphs[4], glyphs2[4], logclust[4], glyphs3[4];
    SCRIPT_VISATTR attrs[4];
    SCRIPT_ITEM items[4];
    int nb, i, j;

    hr = ScriptItemize(test1, 4, 2, NULL, NULL, items, NULL);
    ok(hr == S_OK, "ScriptItemize should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, NULL, NULL, &nb);
    ok(hr == E_INVALIDARG, "ScriptShape should return E_INVALIDARG not %08x\n", hr);

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, NULL, attrs, NULL);
    ok(hr == E_INVALIDARG, "ScriptShape should return E_INVALIDARG not %08x\n", hr);

    hr = ScriptShape(NULL, &sc, test1, 4, 4, &items[0].a, glyphs, NULL, attrs, &nb);
    ok(hr == E_PENDING, "ScriptShape should return E_PENDING not %08x\n", hr);

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, NULL, attrs, &nb);
    ok(broken(hr == S_OK) ||
       hr == E_INVALIDARG || /* Vista, W2K8 */
       hr == E_FAIL, /* WIN7 */
       "ScriptShape should return E_FAIL or E_INVALIDARG, not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(hdc, &sc2, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(sc2 == sc, "caches %p, %p not identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    memset(glyphs,-1,sizeof(glyphs));
    memset(logclust,-1,sizeof(logclust));
    memset(attrs,-1,sizeof(attrs));
    hr = ScriptShape(NULL, &sc, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(nb == 4, "Wrong number of items\n");
    ok(logclust[0] == 0, "clusters out of order\n");
    ok(logclust[1] == 1, "clusters out of order\n");
    ok(logclust[2] == 2, "clusters out of order\n");
    ok(logclust[3] == 3, "clusters out of order\n");
    ok(attrs[0].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[1].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[2].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[3].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[0].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[1].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[2].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[3].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[0].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[1].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[2].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[3].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[0].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[1].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[2].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[3].fZeroWidth == 0, "fZeroWidth incorrect\n");

    ScriptFreeCache(&sc);
    sc = NULL;

    memset(glyphs2,-1,sizeof(glyphs2));
    memset(glyphs3,-1,sizeof(glyphs3));
    memset(logclust,-1,sizeof(logclust));
    memset(attrs,-1,sizeof(attrs));

    GetGlyphIndicesW(hdc, test2, 4, glyphs3, 0);

    hr = ScriptShape(hdc, &sc, test2, 4, 4, &items[0].a, glyphs2, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(nb == 4, "Wrong number of items\n");
    ok(glyphs2[0] == glyphs3[0], "Incorrect glyph for 0x202B\n");
    ok(glyphs2[3] == glyphs3[3], "Incorrect glyph for 0x202C\n");
    ok(logclust[0] == 0, "clusters out of order\n");
    ok(logclust[1] == 1, "clusters out of order\n");
    ok(logclust[2] == 2, "clusters out of order\n");
    ok(logclust[3] == 3, "clusters out of order\n");
    ok(attrs[0].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[1].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[2].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[3].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[0].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[1].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[2].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[3].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[0].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[1].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[2].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[3].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[0].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[1].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[2].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[3].fZeroWidth == 0, "fZeroWidth incorrect\n");

    /* modify LTR to RTL */
    items[0].a.fRTL = 1;
    memset(glyphs2,-1,sizeof(glyphs2));
    memset(logclust,-1,sizeof(logclust));
    memset(attrs,-1,sizeof(attrs));
    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs2, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(nb == 4, "Wrong number of items\n");
    ok(glyphs2[0] == glyphs[3], "Glyphs not reordered properly\n");
    ok(glyphs2[1] == glyphs[2], "Glyphs not reordered properly\n");
    ok(glyphs2[2] == glyphs[1], "Glyphs not reordered properly\n");
    ok(glyphs2[3] == glyphs[0], "Glyphs not reordered properly\n");
    ok(logclust[0] == 3, "clusters out of order\n");
    ok(logclust[1] == 2, "clusters out of order\n");
    ok(logclust[2] == 1, "clusters out of order\n");
    ok(logclust[3] == 0, "clusters out of order\n");
    ok(attrs[0].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[1].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[2].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[3].uJustification == SCRIPT_JUSTIFY_CHARACTER, "uJustification incorrect\n");
    ok(attrs[0].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[1].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[2].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[3].fClusterStart == 1, "fClusterStart incorrect\n");
    ok(attrs[0].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[1].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[2].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[3].fDiacritic == 0, "fDiacritic incorrect\n");
    ok(attrs[0].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[1].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[2].fZeroWidth == 0, "fZeroWidth incorrect\n");
    ok(attrs[3].fZeroWidth == 0, "fZeroWidth incorrect\n");

    ScriptFreeCache(&sc);

    /* some control characters are shown as blank */
    for (i = 0; i < 2; i++)
    {
        static const WCHAR space[]  = {' ', 0};
        static const struct
        {
            WCHAR c;
            unsigned int item_count;
            unsigned int item;
        }
        test_data[] =
        {
            {0x0009, 3, 1}, /* \t   */
            {0x000a, 3, 1}, /* \n   */
            {0x000d, 3, 1}, /* \r   */
            {0x001c, 3, 1}, /* FS   */
            {0x001d, 3, 1}, /* GS   */
            {0x001e, 3, 1}, /* RS   */
            {0x001f, 3, 1}, /* US   */
            {0x200b, 1, 0}, /* ZWSP */
            {0x200c, 1, 0}, /* ZWNJ */
            {0x200d, 1, 0}, /* ZWJ  */
            {0x200e, 3, 1}, /* LRM  */
            {0x200f, 3, 1}, /* RLM  */
            {0x202a, 3, 1}, /* LRE  */
            {0x202b, 3, 1}, /* RLE  */
            {0x202c, 3, 1}, /* PDF  */
            {0x202d, 3, 1}, /* LRO  */
            {0x202e, 3, 1}, /* RLO  */
        };
        WCHAR chars[3];
        HFONT font, oldfont = NULL;
        LOGFONTA lf;

        font = GetCurrentObject(hdc, OBJ_FONT);
        GetObjectA(font, sizeof(lf), &lf);
        if (i == 1) {
            lstrcpyA(lf.lfFaceName, "MS Sans Serif");
            font = CreateFontIndirectA(&lf);
            oldfont = SelectObject(hdc, font);
        }

        hr = ScriptItemize(space, 1, 2, NULL, NULL, items, NULL);
        ok(hr == S_OK, "%s: expected S_OK, got %08x\n", lf.lfFaceName, hr);

        hr = ScriptShape(hdc, &sc, space, 1, 1, &items[0].a, glyphs, logclust, attrs, &nb);
        ok(hr == S_OK, "%s: expected S_OK, got %08x\n", lf.lfFaceName, hr);
        ok(nb == 1, "%s: expected 1, got %d\n", lf.lfFaceName, nb);

        chars[0] = 'A';
        chars[2] = 'A';
        for (j = 0; j < ARRAY_SIZE(test_data); ++j)
        {
            WCHAR c = test_data[j].c;
            SCRIPT_ITEM *item;

            chars[1] = c;
            hr = ScriptItemize(chars, 3, 4, NULL, NULL, items, &nb);
            ok(hr == S_OK, "%s: [%02x] expected S_OK, got %08x\n", lf.lfFaceName, c, hr);
            ok(nb == test_data[j].item_count, "%s: [%02x] Got unexpected item count %d.\n",
               lf.lfFaceName, c, nb);
            item = &items[test_data[j].item];

            ok(!item->a.fNoGlyphIndex, "%s: [%02x] got unexpected fNoGlyphIndex %#x.\n",
               lf.lfFaceName, c, item->a.fNoGlyphIndex);
            hr = ScriptShape(hdc, &sc, chars, 3, 3, &item->a, glyphs2, logclust, attrs, &nb);
            ok(hr == S_OK, "%s: [%02x] expected S_OK, got %08x\n", lf.lfFaceName, c, hr);
            ok(nb == 3, "%s: [%02x] expected 3, got %d\n", lf.lfFaceName, c, nb);
            ok(!item->a.fNoGlyphIndex, "%s: [%02x] got unexpected fNoGlyphIndex %#x.\n",
               lf.lfFaceName, c, item->a.fNoGlyphIndex);

            ok(glyphs[0] == glyphs2[1] ||
               broken(glyphs2[1] == c && (c < 0x10)),
               "%s: [%02x] expected %04x, got %04x\n", lf.lfFaceName, c, glyphs[0], glyphs2[1]);
            ok(attrs[1].fZeroWidth || broken(!attrs[1].fZeroWidth && (c < 0x10) /* Vista */),
               "%s: [%02x] got unexpected fZeroWidth %#x.\n", lf.lfFaceName, c, attrs[1].fZeroWidth);

            item->a.fNoGlyphIndex = 1;
            hr = ScriptShape(hdc, &sc, chars, 3, 3, &item->a, glyphs2, logclust, attrs, &nb);
            ok(hr == S_OK, "%s: [%02x] expected S_OK, got %08x\n", lf.lfFaceName, c, hr);
            ok(nb == 3, "%s: [%02x] expected 1, got %d\n", lf.lfFaceName, c, nb);

            if (c == 0x200b || c == 0x200c || c == 0x200d)
            {
                ok(glyphs2[1] == 0x0020,
                   "%s: [%02x] got unexpected %04x.\n", lf.lfFaceName, c, glyphs2[1]);
                ok(attrs[1].fZeroWidth, "%s: [%02x] got unexpected fZeroWidth %#x.\n",
                   lf.lfFaceName, c, attrs[1].fZeroWidth);
            }
            else
            {
                ok(glyphs2[1] == c,
                   "%s: [%02x] got unexpected %04x.\n", lf.lfFaceName, c, glyphs2[1]);
                ok(!attrs[1].fZeroWidth, "%s: [%02x] got unexpected fZeroWidth %#x.\n",
                   lf.lfFaceName, c, attrs[1].fZeroWidth);
            }
        }
        if (oldfont)
            DeleteObject(SelectObject(hdc, oldfont));
        ScriptFreeCache(&sc);
    }

    /* Text does not support this range. */
    memset(items, 0, sizeof(items));
    nb = 0;
    hr = ScriptItemize(test3, ARRAY_SIZE(test3), ARRAY_SIZE(items), NULL, NULL, items, &nb);
    ok(hr == S_OK, "ScriptItemize failed, hr %#x.\n", hr);
    ok(items[0].a.eScript > 0, "Expected script id.\n");
    ok(nb == 1, "Unexpected number of items.\n");

    memset(glyphs, 0xff, sizeof(glyphs));
    nb = 0;
    hr = ScriptShape(hdc, &sc, test3, ARRAY_SIZE(test3), ARRAY_SIZE(glyphs),
            &items[0].a, glyphs, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape failed, hr %#x.\n", hr);
    ok(nb == 1, "Unexpected glyph count %u\n", nb);
    ok(glyphs[0] == 0, "Unexpected glyph id\n");
    ScriptFreeCache(&sc);
}

static void test_ScriptPlace(HDC hdc)
{
    static const WCHAR test1[] = {'t', 'e', 's', 't',0};
    static const WCHAR test2[] = {0x3044, 0x308d, 0x306f,0}; /* Hiragana, Iroha */
    BOOL ret;
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    SCRIPT_CACHE sc2 = NULL;
    WORD glyphs[4], logclust[4];
    SCRIPT_VISATTR attrs[4];
    SCRIPT_ITEM items[2];
    int nb, widths[4];
    GOFFSET offset[4];
    ABC abc[4];
    HFONT hfont, prev_hfont;
    LOGFONTA lf;
    TEXTMETRICW tm;

    hr = ScriptItemize(test1, 4, 2, NULL, NULL, items, NULL);
    ok(hr == S_OK, "ScriptItemize should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptPlace(hdc, &sc, glyphs, 4, NULL, &items[0].a, widths, NULL, NULL);
    ok(hr == E_INVALIDARG, "ScriptPlace should return E_INVALIDARG not %08x\n", hr);

    hr = ScriptPlace(NULL, &sc, glyphs, 4, attrs, &items[0].a, widths, NULL, NULL);
    ok(broken(hr == E_PENDING) ||
       hr == E_INVALIDARG || /* Vista, W2K8 */
       hr == E_FAIL, /* WIN7 */
       "ScriptPlace should return E_FAIL or E_INVALIDARG, not %08x\n", hr);

    hr = ScriptPlace(NULL, &sc, glyphs, 4, attrs, &items[0].a, widths, offset, NULL);
    ok(hr == E_PENDING, "ScriptPlace should return E_PENDING not %08x\n", hr);

    hr = ScriptPlace(NULL, &sc, glyphs, 4, attrs, &items[0].a, widths, NULL, abc);
    ok(broken(hr == E_PENDING) ||
       hr == E_INVALIDARG || /* Vista, W2K8 */
       hr == E_FAIL, /* WIN7 */
       "ScriptPlace should return E_FAIL or E_INVALIDARG, not %08x\n", hr);

    hr = ScriptPlace(hdc, &sc, glyphs, 4, attrs, &items[0].a, widths, offset, NULL);
    ok(hr == S_OK, "ScriptPlace should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptPlace(hdc, &sc2, glyphs, 4, attrs, &items[0].a, widths, offset, NULL);
    ok(hr == S_OK, "ScriptPlace should return S_OK not %08x\n", hr);
    ok(sc2 == sc, "caches %p, %p not identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    if (widths[0] != 0)
    {
        int old_width = widths[0];
        attrs[0].fZeroWidth = 1;

        hr = ScriptPlace(hdc, &sc, glyphs, 4, attrs, &items[0].a, widths, offset, NULL);
        ok(hr == S_OK, "ScriptPlace should return S_OK not %08x\n", hr);
        ok(widths[0] == 0, "got width %d\n", widths[0]);
        widths[0] = old_width;
    }
    else
        skip("Glyph already has zero-width - skipping fZeroWidth test\n");

    ret = ExtTextOutW(hdc, 1, 1, 0, NULL, glyphs, 4, widths);
    ok(ret, "ExtTextOutW should return TRUE\n");

    ScriptFreeCache(&sc);

    /* test CJK bitmap font which has associated font */
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Fixedsys");
    lf.lfCharSet = DEFAULT_CHARSET;
    hfont = CreateFontIndirectA(&lf);
    prev_hfont = SelectObject(hdc, hfont);
    ret = GetTextMetricsW(hdc, &tm);
    ok(ret, "GetTextMetrics failed\n");

    switch(tm.tmCharSet) {
    case SHIFTJIS_CHARSET:
    case HANGUL_CHARSET:
    case GB2312_CHARSET:
    case CHINESEBIG5_CHARSET:
    {
        SIZE sz;
        DWORD len = lstrlenW(test2), i, total;
        ret = GetTextExtentExPointW(hdc, test2, len, 0, NULL, NULL, &sz);
        ok(ret, "GetTextExtentExPoint failed\n");

        if (sz.cx > len * tm.tmAveCharWidth)
        {
            hr = ScriptItemize(test2, len, 2, NULL, NULL, items, NULL);
            ok(hr == S_OK, "ScriptItemize should return S_OK not %08x\n", hr);
            ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

            items[0].a.fNoGlyphIndex = TRUE;
            memset(glyphs, 'a', sizeof(glyphs));
            hr = ScriptShape(hdc, &sc, test2, len, ARRAY_SIZE(glyphs), &items[0].a, glyphs, logclust, attrs, &nb);
            ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);

            memset(offset, 'a', sizeof(offset));
            memset(widths, 'a', sizeof(widths));
            hr = ScriptPlace(hdc, &sc, glyphs, ARRAY_SIZE(widths), attrs, &items[0].a, widths, offset, NULL);
            ok(hr == S_OK, "ScriptPlace should return S_OK not %08x\n", hr);

            for (total = 0, i = 0; i < nb; i++)
            {
                ok(offset[i].du == 0, "[%d] expected 0, got %d\n", i, offset[i].du);
                ok(offset[i].dv == 0, "[%d] expected 0, got %d\n", i, offset[i].dv);
                ok(widths[i] > tm.tmAveCharWidth, "[%d] expected greater than %d, got %d\n",
                   i, tm.tmAveCharWidth, widths[i]);
                total += widths[i];
            }
            ok(total == sz.cx, "expected %d, got %d\n", sz.cx, total);
        }
        else
            skip("Associated font is unavailable\n");

        break;
    }
    default:
        skip("Non-CJK locale\n");
    }
    SelectObject(hdc, prev_hfont);
}

static void test_ScriptItemIzeShapePlace(HDC hdc, unsigned short pwOutGlyphs[256])
{
    HRESULT         hr;
    int             iMaxProps;
    const SCRIPT_PROPERTIES **ppSp;

    int             cInChars;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0}; 
    WCHAR           TestItem2[] = {'T', 'e', 's', 't', 'b', 0}; 
    WCHAR           TestItem3[] = {'T', 'e', 's', 't', 'c',' ','1','2','3',' ',' ','e','n','d',0};
    WCHAR           TestItem4[] = {'T', 'e', 's', 't', 'd',' ',0x0684,0x0694,0x06a4,' ',' ','\r','\n','e','n','d',0};
    WCHAR           TestItem5[] = {0x0684,'T','e','s','t','e',' ',0x0684,0x0694,0x06a4,' ',' ','e','n','d',0};
    WCHAR           TestItem6[] = {'T', 'e', 's', 't', 'f',' ',' ',' ','\r','\n','e','n','d',0};

    SCRIPT_CACHE    psc;
    unsigned short  pwOutGlyphs1[256];
    unsigned short  pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    unsigned int i;

    /* Verify we get a valid pointer from ScriptGetProperties(). */
    hr = ScriptGetProperties(&ppSp, &iMaxProps);
    ok(hr == S_OK, "ScriptGetProperties failed: 0x%08x\n", hr);
    trace("number of script properties %d\n", iMaxProps);
    ok(iMaxProps > 0, "Got unexpected script count %d.\n", iMaxProps);
    ok(ppSp[0]->langid == 0, "Got unexpected langid %#x.\n", ppSp[0]->langid);

    /* This is a valid test that will cause parsing to take place. */
    cInChars = lstrlenW(TestItem1);
    hr = ScriptItemize(TestItem1, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* This test is for the interim operation of ScriptItemize() where only
     * one SCRIPT_ITEM is returned. */
    ok(pcItems == 1, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[1].iCharPos, cInChars);

    psc = NULL;
    hr = ScriptShape(NULL, &psc, TestItem1, cInChars, cInChars,
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == E_PENDING, "Got unexpected hr %#x.\n", hr);

    hr = ScriptShape(hdc, &psc, TestItem1, cInChars, cInChars - 1,
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == E_OUTOFMEMORY, "Got unexpected hr %#x.\n", hr);

    hr = ScriptShape(hdc, &psc, TestItem1, cInChars, ARRAY_SIZE(pwOutGlyphs1),
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);
    ok(pcGlyphs == cInChars, "Got unexpected glyph count %d, expected %d.\n", pcGlyphs, cInChars);

    /* Send to next test. */
    memcpy(pwOutGlyphs, pwOutGlyphs1, pcGlyphs * sizeof(*pwOutGlyphs));

    hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ScriptPlace(NULL, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* This test verifies that SCRIPT_CACHE is reused and that no translation
     * takes place if fNoGlyphIndex is set. */
    cInChars = lstrlenW(TestItem2);
    hr = ScriptItemize(TestItem2, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* This test is for the interim operation of ScriptItemize() where only
     * one SCRIPT_ITEM is returned. */
    ok(pcItems == 1, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[1].iCharPos, cInChars);

    pItem[0].a.fNoGlyphIndex = 1; /* No translation. */
    hr = ScriptShape(NULL, &psc, TestItem2, cInChars, ARRAY_SIZE(pwOutGlyphs1),
           &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);
    ok(pcGlyphs == cInChars, "Got unexpected glyph count %d, expected %d.\n", pcGlyphs, cInChars);

    for (i = 0; i < cInChars; ++i)
    {
        ok(pwOutGlyphs1[i] == TestItem2[i],
                "Got unexpected pwOutGlyphs1[%u] %#x, expected %#x.\n",
                i, pwOutGlyphs1[i], TestItem2[i]);
    }

    hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ScriptFreeCache(&psc);
    ok(!psc, "Got unexpected psc %p.\n", psc);

    /* This is a valid test that will cause parsing to take place and create 3
     * script_items. */
    cInChars = lstrlenW(TestItem3);
    hr = ScriptItemize(TestItem3, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(pcItems == 3, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == 6, "Got unexpected character position %d.\n", pItem[1].iCharPos);
    ok(pItem[2].iCharPos == 11, "Got unexpected character position %d.\n", pItem[2].iCharPos);
    ok(pItem[3].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[3].iCharPos, cInChars);

    /* This is a valid test that will cause parsing to take place and create 5
     * script_items. */
    cInChars = lstrlenW(TestItem4);
    hr = ScriptItemize(TestItem4, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(pcItems == 5, "Got unexpected item count %d.\n", pcItems);

    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == 6, "Got unexpected character position %d.\n", pItem[1].iCharPos);
    ok(pItem[2].iCharPos == 11, "Got unexpected character position %d.\n", pItem[2].iCharPos);
    ok(pItem[3].iCharPos == 12, "Got unexpected character position %d.\n", pItem[3].iCharPos);
    ok(pItem[4].iCharPos == 13, "Got unexpected character position %d.\n", pItem[4].iCharPos);
    ok(pItem[5].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[5].iCharPos, cInChars);

    ok(pItem[0].a.s.uBidiLevel == 0, "Got unexpected bidi level %u.\n", pItem[0].a.s.uBidiLevel);
    ok(pItem[1].a.s.uBidiLevel == 1, "Got unexpected bidi level %u.\n", pItem[1].a.s.uBidiLevel);
    ok(pItem[2].a.s.uBidiLevel == 0, "Got unexpected bidi level %u.\n", pItem[2].a.s.uBidiLevel);
    ok(pItem[3].a.s.uBidiLevel == 0, "Got unexpected bidi level %u.\n", pItem[3].a.s.uBidiLevel);
    ok(pItem[4].a.s.uBidiLevel == 0, "Got unexpected bidi level %u.\n", pItem[3].a.s.uBidiLevel);

    /* This test is for when the first Unicode character requires BiDi support. */
    hr = ScriptItemize(TestItem5, lstrlenW(TestItem5), ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(pcItems == 4, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].a.s.uBidiLevel == 1, "Got unexpected bidi level %u.\n", pItem[0].a.s.uBidiLevel);

    /* This test verifies that the test to see if there are sufficient buffers
     * to store the pointer to the last character works. Note that Windows
     * often needs a greater number of SCRIPT_ITEMS to process a string than
     * is returned in pcItems. */
    hr = ScriptItemize(TestItem6, lstrlenW(TestItem6), 4, NULL, NULL, pItem, &pcItems);
    ok(hr == E_OUTOFMEMORY, "Got unexpected hr %#x.\n", hr);
}

static void test_ScriptGetCMap(HDC hdc, unsigned short pwOutGlyphs[256])
{
    HRESULT         hr;
    SCRIPT_CACHE    psc = NULL;
    int             cInChars;
    int             cChars;
    unsigned short  pwOutGlyphs2[256];
    unsigned short  pwOutGlyphs3[256];
    DWORD           dwFlags;
    int             cnt;

    static const WCHAR TestItem1[] = {'T', 'e', 's', 't', 'a', 0};
    static const WCHAR TestItem2[] = {0x202B, 'i', 'n', 0x202C,0};
    static const WCHAR TestItem3[] = {'a','b','c','d','(','<','{','[',0x2039,0};
    static const WCHAR TestItem3b[] = {'a','b','c','d',')','>','}',']',0x203A,0};

    /*  Check to make sure that SCRIPT_CACHE gets allocated ok                     */
    dwFlags = 0;
    cInChars = cChars = 5;
    /* Some sanity checks for ScriptGetCMap */

    hr = ScriptGetCMap(NULL, NULL, NULL, 0, 0, NULL);
    ok( hr == E_INVALIDARG, "(NULL,NULL,NULL,0,0,NULL), "
                            "expected E_INVALIDARG, got %08x\n", hr);

    hr = ScriptGetCMap(NULL, NULL, TestItem1, cInChars, dwFlags, pwOutGlyphs3);
    ok( hr == E_INVALIDARG, "(NULL,NULL,TestItem1, cInChars, dwFlags, pwOutGlyphs3), "
                            "expected E_INVALIDARG, got %08x\n", hr);

    /* Set psc to NULL, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetCMap(NULL, &psc, TestItem1, cInChars, 0, pwOutGlyphs3);
    ok( hr == E_PENDING, "(NULL,&psc,NULL,0,0,NULL), expected E_PENDING, "
                         "got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    /* Set psc to NULL but add hdc, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetCMap(hdc, &psc, TestItem1, cInChars, 0, pwOutGlyphs3);
    ok( hr == S_OK, "ScriptGetCMap(NULL,&psc,NULL,0,0,NULL), expected S_OK, "
                    "got %08x\n", hr);
    ok( psc != NULL, "ScriptGetCMap expected psc to be not NULL\n");
    ScriptFreeCache( &psc);

    /* Set psc to NULL, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetCMap(NULL, &psc, TestItem1, cInChars, dwFlags, pwOutGlyphs3);
    ok( hr == E_PENDING, "(NULL,&psc,), expected E_PENDING, got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);
    /*  Check to see if the results are the same as those returned by ScriptShape  */
    hr = ScriptGetCMap(hdc, &psc, TestItem1, cInChars, dwFlags, pwOutGlyphs3);
    ok (hr == S_OK, "ScriptGetCMap should return S_OK not (%08x)\n", hr);
    ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
    for (cnt=0; cnt < cChars && pwOutGlyphs[cnt] == pwOutGlyphs3[cnt]; cnt++) {}
    ok (cnt == cInChars, "Translation not correct. WCHAR %d - %04x != %04x\n",
                         cnt, pwOutGlyphs[cnt], pwOutGlyphs3[cnt]);

    ScriptFreeCache( &psc);
    ok (!psc, "psc is not null after ScriptFreeCache\n");

    /* ScriptGetCMap returns whatever font defines, no special treatment for control chars */
    cInChars = cChars = 4;
    GetGlyphIndicesW(hdc, TestItem2, cInChars, pwOutGlyphs2, 0);

    hr = ScriptGetCMap(hdc, &psc, TestItem2, cInChars, dwFlags, pwOutGlyphs3);
    if (pwOutGlyphs3[0] == 0 || pwOutGlyphs3[3] == 0)
        ok(hr == S_FALSE, "ScriptGetCMap should return S_FALSE not (%08x)\n", hr);
    else
        ok(hr == S_OK, "ScriptGetCMap should return S_OK not (%08x)\n", hr);

    ok(psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
    ok(pwOutGlyphs3[0] == pwOutGlyphs2[0], "expected glyph %d, got %d\n", pwOutGlyphs2[0], pwOutGlyphs3[0]);
    ok(pwOutGlyphs3[3] == pwOutGlyphs2[3], "expected glyph %d, got %d\n", pwOutGlyphs2[3], pwOutGlyphs3[3]);

    cInChars = cChars = 9;
    hr = ScriptGetCMap(hdc, &psc, TestItem3b, cInChars, dwFlags, pwOutGlyphs2);
    ok (hr == S_OK, "ScriptGetCMap should return S_OK not (%08x)\n", hr);
    ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");

    cInChars = cChars = 9;
    dwFlags = SGCM_RTL;
    hr = ScriptGetCMap(hdc, &psc, TestItem3, cInChars, dwFlags, pwOutGlyphs3);
    ok (hr == S_OK, "ScriptGetCMap should return S_OK not (%08x)\n", hr);
    ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
    ok(pwOutGlyphs3[0] == pwOutGlyphs2[0], "glyph incorrectly altered\n");
    ok(pwOutGlyphs3[1] == pwOutGlyphs2[1], "glyph incorrectly altered\n");
    ok(pwOutGlyphs3[2] == pwOutGlyphs2[2], "glyph incorrectly altered\n");
    ok(pwOutGlyphs3[3] == pwOutGlyphs2[3], "glyph incorrectly altered\n");
    ok(pwOutGlyphs3[4] == pwOutGlyphs2[4], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[5] == pwOutGlyphs2[5], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[6] == pwOutGlyphs2[6], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[7] == pwOutGlyphs2[7], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[8] == pwOutGlyphs2[8], "glyph not mirrored correctly\n");

    ScriptFreeCache( &psc);
    ok (!psc, "psc is not null after ScriptFreeCache\n");
}

#define MAX_ENUM_FONTS 4096

struct enum_font_data
{
    int total;
    ENUMLOGFONTA elf[MAX_ENUM_FONTS];
};

static INT CALLBACK enum_bitmap_font_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    struct enum_font_data *efnd = (struct enum_font_data *)lParam;

    if (type & (TRUETYPE_FONTTYPE | DEVICE_FONTTYPE)) return 1;

    if (efnd->total < MAX_ENUM_FONTS)
    {
        efnd->elf[efnd->total++] = *(ENUMLOGFONTA*)lf;
    }
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static INT CALLBACK enum_truetype_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    struct enum_font_data *efnd = (struct enum_font_data *)lParam;

    if (!(type & (TRUETYPE_FONTTYPE | DEVICE_FONTTYPE))) return 1;

    if (efnd->total < MAX_ENUM_FONTS)
    {
        efnd->elf[efnd->total++] = *(ENUMLOGFONTA*)lf;
    }
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static void test_ScriptGetFontProperties(HDC hdc)
{
    HRESULT         hr;
    SCRIPT_CACHE    psc,old_psc;
    SCRIPT_FONTPROPERTIES sfp;
    HFONT font, oldfont;
    LOGFONTA lf;
    struct enum_font_data efnd;
    TEXTMETRICA tmA;
    WORD gi[3];
    WCHAR str[3];
    DWORD  i, ret;
    WORD system_lang_id = PRIMARYLANGID(GetSystemDefaultLangID());
    static const WCHAR invalids[] = {0x0020, 0x200B, 0xF71B};
    /* U+0020: numeric space
       U+200B: zero width space
       U+F71B: unknown, found by black box testing */
    BOOL is_arial, is_times_new_roman, is_arabic = (system_lang_id == LANG_ARABIC);

    /* Some sanity checks for ScriptGetFontProperties */

    hr = ScriptGetFontProperties(NULL,NULL,NULL);
    ok( hr == E_INVALIDARG, "(NULL,NULL,NULL), expected E_INVALIDARG, got %08x\n", hr);

    hr = ScriptGetFontProperties(NULL,NULL,&sfp);
    ok( hr == E_INVALIDARG, "(NULL,NULL,&sfp), expected E_INVALIDARG, got %08x\n", hr);

    /* Set psc to NULL, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetFontProperties(NULL,&psc,NULL);
    ok( hr == E_INVALIDARG, "(NULL,&psc,NULL), expected E_INVALIDARG, got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    /* Set psc to NULL, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetFontProperties(NULL,&psc,&sfp);
    ok( hr == E_PENDING, "(NULL,&psc,&sfp), expected E_PENDING, got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    hr = ScriptGetFontProperties(hdc,NULL,NULL);
    ok( hr == E_INVALIDARG, "(hdc,NULL,NULL), expected E_INVALIDARG, got %08x\n", hr);

    hr = ScriptGetFontProperties(hdc,NULL,&sfp);
    ok( hr == E_INVALIDARG, "(hdc,NULL,&sfp), expected E_INVALIDARG, got %08x\n", hr);

    /* Set psc to NULL, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetFontProperties(hdc,&psc,NULL);
    ok( hr == E_INVALIDARG, "(hdc,&psc,NULL), expected E_INVALIDARG, got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    /* Pass an invalid sfp */
    psc = NULL;
    sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES) - 1;
    hr = ScriptGetFontProperties(hdc,&psc,&sfp);
    ok( hr == E_INVALIDARG, "(hdc,&psc,&sfp) invalid, expected E_INVALIDARG, got %08x\n", hr);
    ok( psc != NULL, "Expected a pointer in psc, got NULL\n");
    ok( sfp.cBytes == sizeof(SCRIPT_FONTPROPERTIES) - 1, "Unexpected cBytes.\n");
    ScriptFreeCache(&psc);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    /* Give it the correct cBytes, we don't care about what's coming back */
    sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
    psc = NULL;
    hr = ScriptGetFontProperties(hdc,&psc,&sfp);
    ok( hr == S_OK, "(hdc,&psc,&sfp) partly initialized, expected S_OK, got %08x\n", hr);
    ok( psc != NULL, "Expected a pointer in psc, got NULL\n");

    /* Save the psc pointer */
    old_psc = psc;
    /* Now a NULL hdc again */
    hr = ScriptGetFontProperties(NULL,&psc,&sfp);
    ok( hr == S_OK, "(NULL,&psc,&sfp), expected S_OK, got %08x\n", hr);
    ok( psc == old_psc, "Expected psc not to be changed, was %p is now %p\n", old_psc, psc);
    ScriptFreeCache(&psc);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    efnd.total = 0;
    EnumFontFamiliesA(hdc, NULL, enum_bitmap_font_proc, (LPARAM)&efnd);

    for (i = 0; i < efnd.total; i++)
    {
        if (strlen((char *)efnd.elf[i].elfFullName) >= LF_FACESIZE)
        {
            trace("Font name to long to test: %s\n",(char *)efnd.elf[i].elfFullName);
            continue;
        }
        lstrcpyA(lf.lfFaceName, (char *)efnd.elf[i].elfFullName);
        font = CreateFontIndirectA(&lf);
        oldfont = SelectObject(hdc, font);

        sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
        psc = NULL;
        hr = ScriptGetFontProperties(hdc, &psc, &sfp);
        ok(hr == S_OK, "ScriptGetFontProperties expected S_OK, got %08x\n", hr);
        if (winetest_interactive)
        {
            trace("bitmap font %s\n", lf.lfFaceName);
            trace("wgBlank %04x\n", sfp.wgBlank);
            trace("wgDefault %04x\n", sfp.wgDefault);
            trace("wgInvalid %04x\n", sfp.wgInvalid);
            trace("wgKashida %04x\n", sfp.wgKashida);
            trace("iKashidaWidth %d\n", sfp.iKashidaWidth);
        }

        ret = GetTextMetricsA(hdc, &tmA);
        ok(ret != 0, "GetTextMetricsA failed!\n");

        ret = GetGlyphIndicesW(hdc, invalids, 1, gi, GGI_MARK_NONEXISTING_GLYPHS);
        ok(ret != GDI_ERROR, "GetGlyphIndicesW failed!\n");

        ok(sfp.wgBlank == tmA.tmBreakChar || sfp.wgBlank == gi[0], "bitmap font %s wgBlank %04x tmBreakChar %04x Space %04x\n", lf.lfFaceName, sfp.wgBlank, tmA.tmBreakChar, gi[0]);

        ok(sfp.wgDefault == 0 || sfp.wgDefault == tmA.tmDefaultChar || broken(sfp.wgDefault == (0x100 | tmA.tmDefaultChar)), "bitmap font %s wgDefault %04x, tmDefaultChar %04x\n", lf.lfFaceName, sfp.wgDefault, tmA.tmDefaultChar);

        ok(sfp.wgInvalid == sfp.wgBlank || broken(is_arabic), "bitmap font %s wgInvalid %02x wgBlank %02x\n", lf.lfFaceName, sfp.wgInvalid, sfp.wgBlank);

        ok(sfp.wgKashida == 0xFFFF || broken(is_arabic), "bitmap font %s wgKashida %02x\n", lf.lfFaceName, sfp.wgKashida);

        ScriptFreeCache(&psc);

        SelectObject(hdc, oldfont);
        DeleteObject(font);
    }

    efnd.total = 0;
    EnumFontFamiliesA(hdc, NULL, enum_truetype_proc, (LPARAM)&efnd);

    for (i = 0; i < efnd.total; i++)
    {
        if (strlen((char *)efnd.elf[i].elfFullName) >= LF_FACESIZE)
        {
            trace("Font name to long to test: %s\n",(char *)efnd.elf[i].elfFullName);
            continue;
        }
        lstrcpyA(lf.lfFaceName, (char *)efnd.elf[i].elfFullName);
        font = CreateFontIndirectA(&lf);
        oldfont = SelectObject(hdc, font);

        sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
        psc = NULL;
        hr = ScriptGetFontProperties(hdc, &psc, &sfp);
        ok(hr == S_OK, "ScriptGetFontProperties expected S_OK, got %08x\n", hr);
        if (winetest_interactive)
        {
            trace("truetype font %s\n", lf.lfFaceName);
            trace("wgBlank %04x\n", sfp.wgBlank);
            trace("wgDefault %04x\n", sfp.wgDefault);
            trace("wgInvalid %04x\n", sfp.wgInvalid);
            trace("wgKashida %04x\n", sfp.wgKashida);
            trace("iKashidaWidth %d\n", sfp.iKashidaWidth);
        }

        str[0] = 0x0020; /* U+0020: numeric space */
        ret = GetGlyphIndicesW(hdc, str, 1, gi, 0);
        ok(ret != GDI_ERROR, "GetGlyphIndicesW failed!\n");
        ok(sfp.wgBlank == gi[0], "truetype font %s wgBlank %04x gi[0] %04x\n", lf.lfFaceName, sfp.wgBlank, gi[0]);

        ok(sfp.wgDefault == 0 || broken(is_arabic), "truetype font %s wgDefault %04x\n", lf.lfFaceName, sfp.wgDefault);

        ret = GetGlyphIndicesW(hdc, invalids, 3, gi, GGI_MARK_NONEXISTING_GLYPHS);
        ok(ret != GDI_ERROR, "GetGlyphIndicesW failed!\n");
        if (gi[2] != 0xFFFF) /* index of default non exist char */
            ok(sfp.wgInvalid == gi[2], "truetype font %s wgInvalid %04x gi[2] %04x\n", lf.lfFaceName, sfp.wgInvalid, gi[2]);
        else if (gi[1] != 0xFFFF)
            ok(sfp.wgInvalid == gi[1], "truetype font %s wgInvalid %04x gi[1] %04x\n", lf.lfFaceName, sfp.wgInvalid, gi[1]);
        else if (gi[0] != 0xFFFF)
            ok(sfp.wgInvalid == gi[0], "truetype font %s wgInvalid %04x gi[0] %04x\n", lf.lfFaceName, sfp.wgInvalid, gi[0]);
        else
            ok(sfp.wgInvalid == 0, "truetype font %s wgInvalid %04x expect 0\n", lf.lfFaceName, sfp.wgInvalid);

        str[0] = 0x0640; /* U+0640: kashida */
        ret = GetGlyphIndicesW(hdc, str, 1, gi, GGI_MARK_NONEXISTING_GLYPHS);
        ok(ret != GDI_ERROR, "GetGlyphIndicesW failed!\n");
        is_arial = !lstrcmpA(lf.lfFaceName, "Arial");
        is_times_new_roman= !lstrcmpA(lf.lfFaceName, "Times New Roman");
        ok(sfp.wgKashida == gi[0] || broken(is_arial || is_times_new_roman) || broken(is_arabic), "truetype font %s wgKashida %04x gi[0] %04x\n", lf.lfFaceName, sfp.wgKashida, gi[0]);

        ScriptFreeCache(&psc);

        SelectObject(hdc, oldfont);
        DeleteObject(font);
    }
}

static void test_ScriptTextOut(HDC hdc)
{
    HRESULT         hr;

    int             cInChars;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0}; 

    SCRIPT_CACHE    psc;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    RECT            rect;
    int             piX;
    SCRIPT_LOGATTR  sla[256];

    /* This is a valid test that will cause parsing to take place. */
    cInChars = lstrlenW(TestItem1);
    hr = ScriptItemize(TestItem1, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* This test is for the interim operation of ScriptItemize() where only
     * one SCRIPT_ITEM is returned. */
    ok(pcItems == 1, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[1].iCharPos, cInChars);

    psc = NULL;
    cInChars = 5;
    hr = ScriptShape(hdc, &psc, TestItem1, cInChars, ARRAY_SIZE(pwOutGlyphs1),
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);
    ok(pcGlyphs == cInChars, "Got unexpected glyph count %d, expected %d.\n", pcGlyphs, cInChars);

    /* Note hdc is needed as glyph info is not yet in psc. */
    hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* Get rid of psc for next test set. */
    ScriptFreeCache(&psc);
    ok(!psc, "Got unexpected psc %p.\n", psc);

    hr = ScriptTextOut(NULL, NULL, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    hr = ScriptTextOut(NULL, NULL, 0, 0, 0, NULL, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    hr = ScriptTextOut(NULL, &psc, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!psc, "Got unexpected psc %p.\n", psc);

    /* hdc is required. */
    hr = ScriptTextOut(NULL, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!psc, "Got unexpected psc %p.\n", psc);
    hr = ScriptTextOut(hdc, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Test Rect Rgn is acceptable. */
    SetRect(&rect, 10, 10, 40, 20);
    hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ScriptCPtoX(1, FALSE, cInChars, pcGlyphs, pwLogClust, psva, piAdvance, &pItem[0].a, &piX);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ScriptBreak(TestItem1, cInChars, &pItem[0].a, sla);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ScriptFreeCache(&psc);
    ok(!psc, "Got unexpected psc %p.\n", psc);
}

/* The intent is to validate that the DC passed into ScriptTextOut() is used
 * instead of the (possibly) invalid cached one. */
static void test_ScriptTextOut2(HDC hdc)
{
    HRESULT         hr;

    HDC             hdc1, hdc2;
    int             cInChars;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0};

    SCRIPT_CACHE    psc;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    BOOL ret;

    /* Create an extra DC that will be used until the ScriptTextOut() call. */
    hdc1 = CreateCompatibleDC(hdc);
    ok(!!hdc1, "Failed to create a DC.\n");
    hdc2 = CreateCompatibleDC(hdc);
    ok(!!hdc2, "Failed to create a DC.\n");

    /* This is a valid test that will cause parsing to take place. */
    cInChars = lstrlenW(TestItem1);
    hr = ScriptItemize(TestItem1, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* This test is for the interim operation of ScriptItemize() where only
     * one SCRIPT_ITEM is returned. */
    ok(pcItems == 1, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[1].iCharPos, cInChars);

    psc = NULL;
    hr = ScriptShape(hdc2, &psc, TestItem1, cInChars, ARRAY_SIZE(pwOutGlyphs1),
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);
    ok(pcGlyphs == cInChars, "Got unexpected glyph count %d, expected %d.\n", pcGlyphs, cInChars);

    /* Note hdc is needed as glyph info is not yet in psc. */
    hr = ScriptPlace(hdc2, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Key part! Cached DC is being deleted. */
    ret = DeleteDC(hdc2);
    ok(ret, "Got unexpected ret %#x.\n", ret);

    /* At this point the cached DC (hdc2) has been destroyed. However, we are
     * passing in a *real* DC (the original DC). The text should be written to
     * that DC. */
    hr = ScriptTextOut(hdc1, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);

    DeleteDC(hdc1);

    ScriptFreeCache(&psc);
    ok(!psc, "Got unexpected psc %p.\n", psc);
}

static void test_ScriptTextOut3(HDC hdc)
{
    HRESULT         hr;

    int             cInChars;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {' ','\r', 0};

    SCRIPT_CACHE    psc;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    RECT            rect;

    /* This is to ensure that non-existent glyphs are translated into a valid
     * glyph number. */
    cInChars = lstrlenW(TestItem1);
    hr = ScriptItemize(TestItem1, cInChars, ARRAY_SIZE(pItem), NULL, NULL, pItem, &pcItems);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* This test is for the interim operation of ScriptItemize() where only
     * one SCRIPT_ITEM is returned. */
    ok(pcItems == 2, "Got unexpected item count %d.\n", pcItems);
    ok(pItem[0].iCharPos == 0, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[1].iCharPos == 1, "Got unexpected character position %d.\n", pItem[0].iCharPos);
    ok(pItem[2].iCharPos == cInChars, "Got unexpected character position %d, expected %d.\n",
            pItem[2].iCharPos, cInChars);

    psc = NULL;
    hr = ScriptShape(hdc, &psc, TestItem1, cInChars, ARRAY_SIZE(pwOutGlyphs1),
            &pItem[0].a, pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!psc, "Got unexpected psc %p.\n", psc);
    ok(pcGlyphs == cInChars, "Got unexpected glyph count %d, expected %d.\n", pcGlyphs, cInChars);

    /* Note hdc is needed as glyph info is not yet in psc. */
    hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs,
            psva, &pItem[0].a, piAdvance, pGoffset, pABC);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Test Rect Rgn is acceptable. */
    SetRect(&rect, 10, 10, 40, 20);
    hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0,
            pwOutGlyphs1, pcGlyphs, piAdvance, NULL, pGoffset);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ScriptFreeCache(&psc);
    ok(!psc, "Got unexpected psc %p.\n", psc);
}

#define test_item_ScriptXtoX(a,b,c,d,e,f) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_item_ScriptXtoX(a,b,c,d,e,f)

static void _test_item_ScriptXtoX(SCRIPT_ANALYSIS *psa, int cChars, int cGlyphs, const int* offsets, const WORD *pwLogClust, const int* piAdvance )
{
    int iX, iCP;
    int icChars, icGlyphs;
    int piCP, piX;
    HRESULT hr;
    SCRIPT_VISATTR psva[10];
    int piTrailing;
    BOOL fTrailing;
    int direction;

    memset(psva,0,sizeof(psva));
    direction = (psa->fRTL)?-1:+1;

    for(iCP = 0; iCP < cChars; iCP++)
    {
        iX = offsets[iCP];
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
        winetest_ok(hr == S_OK, "ScriptXtoCP: should return S_OK not %08x\n", hr);
        winetest_ok(piCP == iCP, "ScriptXtoCP: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
        winetest_ok(piTrailing == 0, "ScriptXtoCP: iX=%d should return piTrailing=0 not %d\n", iX, piTrailing);
    }

    for(iCP = 0; iCP < cChars; iCP++)
    {
        iX = offsets[iCP]+direction;
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
        winetest_ok(hr == S_OK, "ScriptXtoCP leading: should return S_OK not %08x\n", hr);
        winetest_ok(piCP == iCP, "ScriptXtoCP leading: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
        winetest_ok(piTrailing == 0, "ScriptXtoCP leading: iX=%d should return piTrailing=0 not %d\n", iX, piTrailing);
    }

    for(iCP = 0; iCP < cChars; iCP++)
    {
        iX = offsets[iCP+1]-direction;
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
        winetest_ok(hr == S_OK, "ScriptXtoCP trailing: should return S_OK not %08x\n", hr);
        winetest_ok(piCP == iCP, "ScriptXtoCP trailing: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
        winetest_ok(piTrailing == 1, "ScriptXtoCP trailing: iX=%d should return piTrailing=1 not %d\n", iX, piTrailing);
    }

    for(iCP = 0; iCP <= cChars+1; iCP++)
    {
        fTrailing = FALSE;
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptCPtoX(iCP, fTrailing, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piX);
        winetest_ok(hr == S_OK, "ScriptCPtoX: should return S_OK not %08x\n", hr);
        winetest_ok(piX == offsets[iCP],
           "ScriptCPtoX: iCP=%d should return piX=%d not %d\n", iCP, offsets[iCP], piX);
    }

    for(iCP = 0; iCP <= cChars+1; iCP++)
    {
        fTrailing = TRUE;
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptCPtoX(iCP, fTrailing, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piX);
        winetest_ok(hr == S_OK, "ScriptCPtoX trailing: should return S_OK not %08x\n", hr);
        winetest_ok(piX == offsets[iCP+1],
           "ScriptCPtoX trailing: iCP=%d should return piX=%d not %d\n", iCP, offsets[iCP+1], piX);
    }
}

#define test_caret_item_ScriptXtoCP(a,b,c,d,e,f) _test_caret_item_ScriptXtoCP(__LINE__,a,b,c,d,e,f)

static void _test_caret_item_ScriptXtoCP(int line, SCRIPT_ANALYSIS *psa, int cChars, int cGlyphs, const int* offsets, const WORD *pwLogClust, const int* piAdvance )
{
    int iX, iCP, i;
    int icChars, icGlyphs;
    int piCP;
    int clusterSize;
    HRESULT hr;
    SCRIPT_VISATTR psva[10];
    int piTrailing;
    int direction;

    memset(psva,0,sizeof(psva));
    direction = (psa->fRTL)?-1:+1;

    for(iX = -1, i = iCP = 0; i < cChars; i++)
    {
        if (offsets[i] != iX)
        {
            iX = offsets[i];
            iCP = i;
        }
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
        ok_(__FILE__,line)(hr == S_OK, "ScriptXtoCP: should return S_OK not %08x\n", hr);
        ok_(__FILE__,line)(piCP == iCP, "ScriptXtoCP: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
        ok_(__FILE__,line)(piTrailing == 0, "ScriptXtoCP: iX=%d should return piTrailing=0 not %d\n", iX, piTrailing);
    }

    for(iX = -2, i = 0; i < cChars; i++)
    {
        if (offsets[i]+direction != iX)
        {
            iX = offsets[i] + direction;
            iCP = i;
        }
        icChars = cChars;
        icGlyphs = cGlyphs;
        hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
        ok_(__FILE__,line)(hr == S_OK, "ScriptXtoCP leading: should return S_OK not %08x\n", hr);
        ok_(__FILE__,line)(piCP == iCP, "ScriptXtoCP leading: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
        ok_(__FILE__,line)(piTrailing == 0, "ScriptXtoCP leading: iX=%d should return piTrailing=0 not %d\n", iX, piTrailing);
    }

    for(clusterSize = 0, iCP = 0, iX = -2, i = 0; i < cChars; i++)
    {
        clusterSize++;
        if (offsets[i] != offsets[i+1])
        {
            iX = offsets[i+1]-direction;
            icChars = cChars;
            icGlyphs = cGlyphs;
            hr = ScriptXtoCP(iX, icChars, icGlyphs, pwLogClust, psva, piAdvance, psa, &piCP, &piTrailing);
            ok_(__FILE__,line)(hr == S_OK, "ScriptXtoCP trailing: should return S_OK not %08x\n", hr);
            ok_(__FILE__,line)(piCP == iCP, "ScriptXtoCP trailing: iX=%d should return piCP=%d not %d\n", iX, iCP, piCP);
            ok_(__FILE__,line)(piTrailing == clusterSize, "ScriptXtoCP trailing: iX=%d should return piTrailing=%d not %d\n", iX, clusterSize, piTrailing);
            iCP = i+1;
            clusterSize = 0;
        }
    }
}

static void test_ScriptXtoX(void)
/****************************************************************************************
 *  This routine tests the ScriptXtoCP and ScriptCPtoX functions using static variables *
 ****************************************************************************************/
{
    WORD pwLogClust[10] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 3};
    WORD pwLogClust_RTL[10] = {3, 3, 3, 2, 2, 1, 1, 0, 0, 0};
    WORD pwLogClust_2[7] = {4, 3, 3, 2, 1, 0 ,0};
    WORD pwLogClust_3[17] = {0, 1, 1, 1, 1, 4, 5, 6, 6, 8, 8, 8, 8, 11, 11, 13, 13};
    WORD pwLogClust_3_RTL[17] = {13, 13, 11, 11, 8, 8, 8, 8, 6, 6, 5, 4, 1, 1, 1, 1, 0};
    int piAdvance[10] = {201, 190, 210, 180, 170, 204, 189, 195, 212, 203};
    int piAdvance_2[5] = {39, 26, 19, 17, 11};
    int piAdvance_3[15] = {6, 6, 0, 0, 10, 5, 10, 0, 12, 0, 0, 9, 0, 10, 0};
    static const int offsets[13] = {0, 67, 134, 201, 296, 391, 496, 601, 1052, 1503, 1954, 1954, 1954};
    static const int offsets_RTL[13] = {781, 721, 661, 601, 496, 391, 296, 201, 134, 67, 0, 0, 0};
    static const int offsets_2[10] = {112, 101, 92, 84, 65, 39, 19, 0, 0, 0};

    static const int offsets_3[19] = {0, 6, 6, 6, 6, 12, 22, 27, 27, 37, 37, 37, 37, 49, 49, 58, 58, 68, 68};
    static const int offsets_3_RTL[19] = {68, 68, 58, 58, 49, 49, 49, 49, 37, 37, 27, 22, 12, 12, 12, 12, 6, 6};

    SCRIPT_VISATTR psva[15];
    SCRIPT_ANALYSIS sa;
    SCRIPT_ITEM items[2];
    int iX, i;
    int piCP;
    int piTrailing;
    HRESULT hr;
    static const WCHAR hebrW[] = { 0x5be, 0};
    static const WCHAR thaiW[] = { 0xe2a, 0};
    const SCRIPT_PROPERTIES **ppScriptProperties;

    memset(&sa, 0 , sizeof(SCRIPT_ANALYSIS));
    memset(psva, 0, sizeof(psva));

    sa.fRTL = FALSE;
    hr = ScriptXtoCP(-1, 10, 10, pwLogClust, psva, piAdvance, &sa, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    if (piTrailing)
        ok(piCP == -1, "Negative iX should return piCP=-1 not %d\n", piCP);
    else /* win2k3 */
        ok(piCP == 10, "Negative iX should return piCP=10 not %d\n", piCP);

    for (iX = 0; iX <= 7; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
        ok(hr == S_OK, "ScriptXtoCP failed, hr %#x.\n", hr);
        ok(piCP==0 && piTrailing==0,"%i should return 0(%i) and 0(%i)\n",iX, piCP,piTrailing);
    }
    for (iX = 8; iX < 16; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
        ok(hr == S_OK, "ScriptXtoCP failed, hr %#x.\n", hr);
        ok(piCP==0 && piTrailing==1,"%i should return 0(%i) and 1(%i)\n",iX, piCP,piTrailing);
    }

    sa.fRTL = TRUE;
    hr = ScriptXtoCP(-1, 10, 10, pwLogClust_RTL, psva, piAdvance, &sa, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    if (piTrailing)
        ok(piCP == -1, "Negative iX should return piCP=-1 not %d\n", piCP);
    else /* win2k3 */
        ok(piCP == 10, "Negative iX should return piCP=10 not %d\n", piCP);

    iX = 1954;
    hr = ScriptXtoCP(1954, 10, 10, pwLogClust_RTL, psva, piAdvance, &sa, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == -1, "iX=%d should return piCP=-1 not %d\n", iX, piCP);
    ok(piTrailing == 1, "iX=%d should return piTrailing=1 not %d\n", iX, piTrailing);

    for (iX = 1; iX <= 8; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
        ok(hr == S_OK, "ScriptXtoCP() failed, hr %#x.\n", hr);
        ok(piCP==0 && piTrailing==1,"%i should return 0(%i) and 1(%i)\n",iX,piCP,piTrailing);
    }
    for (iX = 9; iX < 16; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
        ok(hr == S_OK, "ScriptXtoCP() failed, hr %#x.\n", hr);
        ok(piCP==0 && piTrailing==0,"%i should return 0(%i) and 0(%i)\n",iX,piCP,piTrailing);
    }

    sa.fRTL = FALSE;
    test_item_ScriptXtoX(&sa, 10, 10, offsets, pwLogClust, piAdvance);
    sa.fRTL = TRUE;
    test_item_ScriptXtoX(&sa, 10, 10, offsets_RTL, pwLogClust_RTL, piAdvance);
    test_item_ScriptXtoX(&sa, 7, 5, offsets_2, pwLogClust_2, piAdvance_2);

    /* Get thai eScript, This will do LTR and fNeedsCaretInfo */
    hr = ScriptItemize(thaiW, 1, 2, NULL, NULL, items, &i);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(i == 1, "got %d\n", i);
    sa = items[0].a;

    test_caret_item_ScriptXtoCP(&sa, 17, 15, offsets_3, pwLogClust_3, piAdvance_3);

    /* Get hebrew eScript, This will do RTL and fNeedsCaretInfo */
    hr = ScriptItemize(hebrW, 1, 2, NULL, NULL, items, &i);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(i == 1, "got %d\n", i);
    sa = items[0].a;

    /* Note: This behavior CHANGED in uniscribe versions...
     *       so we only want to test if fNeedsCaretInfo is set */
    hr = ScriptGetProperties(&ppScriptProperties, &i);
    if (ppScriptProperties[sa.eScript]->fNeedsCaretInfo)
    {
        test_caret_item_ScriptXtoCP(&sa, 17, 15, offsets_3_RTL, pwLogClust_3_RTL, piAdvance_3);
        hr = ScriptXtoCP(0, 17, 15, pwLogClust_3_RTL, psva, piAdvance_3, &sa, &piCP, &piTrailing);
        ok(hr == S_OK, "ScriptXtoCP: should return S_OK not %08x\n", hr);
        ok(piCP == 16, "ScriptXtoCP: iX=0 should return piCP=16 not %d\n", piCP);
        ok(piTrailing == 1, "ScriptXtoCP: iX=0 should return piTrailing=1 not %d\n", piTrailing);
    }
    else
        win_skip("Uniscribe version too old to test Hebrew clusters\n");
}

/* This set of tests is for the string functions of Uniscribe. The
 * ScriptStringAnalyse() function allocates memory pointed to by the
 * SCRIPT_STRING_ANALYSIS ssa pointer. This memory is freed by
 * ScriptStringFree(). There needs to be a valid hdc for this as
 * ScriptStringAnalyse() calls ScriptItemize(), ScriptShape() and
 * ScriptPlace() which require it. */
static void test_ScriptString(HDC hdc)
{

    HRESULT         hr;
    WCHAR           teststr[] = {'T','e','s','t','1',' ','a','2','b','3', '\0'};
    int             len = ARRAY_SIZE(teststr) - 1;
    int             Glyphs = len * 2 + 16;
    DWORD           Flags = SSA_GLYPHS;
    int             ReqWidth = 100;
    static const int Dx[ARRAY_SIZE(teststr) - 1];
    static const BYTE InClass[ARRAY_SIZE(teststr) - 1];
    SCRIPT_STRING_ANALYSIS ssa = NULL;

    int             X = 10; 
    int             Y = 100;
    UINT            Options = 0; 
    const RECT      rc = {0, 50, 100, 100}; 
    int             MinSel = 0;
    int             MaxSel = 0;
    BOOL            Disabled = FALSE;
    const int      *clip_len;
    UINT           *order;
    unsigned int i;

    /* Test without hdc to get E_PENDING. */
    hr = ScriptStringAnalyse(NULL, teststr, len, Glyphs, -1,
            Flags, ReqWidth, NULL, NULL, Dx, NULL, InClass, &ssa);
    ok(hr == E_PENDING, "Got unexpected hr %#x.\n", hr);

    /* Test that 0 length string returns E_INVALIDARG. */
    hr = ScriptStringAnalyse(hdc, teststr, 0, Glyphs, -1,
            Flags, ReqWidth, NULL, NULL, Dx, NULL, InClass, &ssa);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    /* Test with hdc, this should be a valid test. */
    hr = ScriptStringAnalyse(hdc, teststr, len, Glyphs, -1,
            Flags, ReqWidth, NULL, NULL, Dx, NULL, InClass, &ssa);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ScriptStringFree(&ssa);

    /* Test makes sure that a call with a valid pssa still works. */
    hr = ScriptStringAnalyse(hdc, teststr, len, Glyphs, -1,
            Flags, ReqWidth, NULL, NULL, Dx, NULL, InClass, &ssa);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!ssa, "Got unexpected ssa %p.\n", ssa);

    hr = ScriptStringOut(ssa, X, Y, Options, &rc, MinSel, MaxSel, Disabled);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    clip_len = ScriptString_pcOutChars(ssa);
    ok(*clip_len == len, "Got unexpected *clip_len %d, expected %d.\n", *clip_len, len);

    order = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *clip_len * sizeof(*order));
    hr = ScriptStringGetOrder(ssa, order);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < *clip_len; ++i)
    {
        ok(order[i] == i, "Got unexpected order[%u] %u.\n", i, order[i]);
    }
    HeapFree(GetProcessHeap(), 0, order);

    hr = ScriptStringFree(&ssa);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

/* Test ScriptStringXtoCP() and ScriptStringCPtoX(). Since fonts may differ
 * between Windows and Wine, the test generates values using one function, and
 * then verifies the output is consistent with the output of the other. */
static void test_ScriptStringXtoCP_CPtoX(HDC hdc)
{
    HRESULT         hr;
    static const WCHAR teststr1[]  = {0x05e9, 'i', 0x05dc, 'n', 0x05d5, 'e', 0x05dd, '.',0};
    static const BOOL rtl[] = {1, 0, 1, 0, 1, 0, 1, 0};
    unsigned int String_len = ARRAY_SIZE(teststr1) - 1;
    int             Glyphs = String_len * 2 + 16;       /* size of buffer as recommended  */
    static const BYTE InClass[ARRAY_SIZE(teststr1) - 1];
    SCRIPT_STRING_ANALYSIS ssa = NULL;

    int             Ch;                                  /* Character position in string */
    int             iTrailing;
    int             Cp;                                  /* Character position in string */
    int             X;
    int             trail,lead;

    /* Test with hdc, this should be a valid test. Here we generate a
     * SCRIPT_STRING_ANALYSIS that will be used as input to the following
     * character-positions-to-X and X-to-character-position functions. */
    hr = ScriptStringAnalyse(hdc, &teststr1, String_len, Glyphs, -1,
            SSA_GLYPHS, 100, NULL, NULL, NULL, NULL, InClass, &ssa);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* NT */,
            "Got unexpected hr %08x.\n", hr);
    if (hr != S_OK)
        return;
    ok(!!ssa, "Got unexpected ssa %p.\n", ssa);

    /* Loop to generate character positions to provide starting positions for
     * the ScriptStringCPtoX() and ScriptStringXtoCP() functions. */
    for (Cp = 0; Cp < String_len; ++Cp)
    {
        /* The fTrailing flag is used to indicate whether the X being returned
         * is at the beginning or the end of the character. What happens here
         * is that if fTrailing indicates the end of the character, i.e. FALSE,
         * then ScriptStringXtoCP() returns the beginning of the next
         * character and iTrailing is FALSE. So for this loop iTrailing will
         * be FALSE in both cases. */
        hr = ScriptStringCPtoX(ssa, Cp, TRUE, &trail);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        hr = ScriptStringCPtoX(ssa, Cp, FALSE, &lead);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(rtl[Cp] ? lead > trail : lead < trail,
                "Got unexpected lead %d, trail %d, for rtl[%u] %u.\n",
                lead, trail, Cp, rtl[Cp]);

        /* Move by 1 pixel so that we are not between 2 characters. That could
         * result in being the lead of a RTL and at the same time the trail of
         * an LTR. */

        /* Inside the leading edge. */
        X = rtl[Cp] ? lead - 1 : lead + 1;
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(Ch == Cp, "Got unexpected Ch %d for X %d, expected %d.\n", Ch, X, Cp);
        ok(!iTrailing, "Got unexpected iTrailing %#x for X %d.\n", iTrailing, X);

        /* Inside the trailing edge. */
        X = rtl[Cp] ? trail + 1 : trail - 1;
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(Ch == Cp, "Got unexpected Ch %d for X %d, expected %d.\n", Ch, X, Cp);
        ok(iTrailing, "Got unexpected iTrailing %#x for X %d.\n", iTrailing, X);

        /* Outside the trailing edge. */
        if (Cp < String_len - 1)
        {
            X = rtl[Cp] ? lead + 1 : trail + 1;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
            ok(Ch == Cp + 1, "Got unexpected Ch %d for X %d, expected %d.\n", Ch, X, Cp + 1);
            ok(iTrailing == !!rtl[Cp + 1], "Got unexpected iTrailing %#x for X %d, expected %#x.\n",
                    iTrailing, X, !!rtl[Cp + 1]);
        }

        /* Outside the leading edge. */
        if (Cp)
        {
            X = rtl[Cp] ? trail - 1 : lead - 1;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
            ok(Ch == Cp - 1, "Got unexpected Ch %d for X %d, expected %d.\n", Ch, X, Cp - 1);
            ok(iTrailing == !rtl[Cp - 1], "Got unexpected iTrailing %#x for X %d, expected %#x.\n",
                    iTrailing, X, !rtl[Cp - 1]);
        }
    }

    /* Check beyond the leading boundary of the whole string. */
    if (rtl[0])
    {
        /* Having a leading RTL character seems to confuse usp. This looks to
         * be a Windows bug we should emulate. */
        hr = ScriptStringCPtoX(ssa, 0, TRUE, &X);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        --X;
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(Ch == 1, "Got unexpected Ch %d.\n", Ch);
        ok(!iTrailing, "Got unexpected iTrailing %#x.\n", iTrailing);
    }
    else
    {
        hr = ScriptStringCPtoX(ssa, 0, FALSE, &X);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        --X;
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(Ch == -1, "Got unexpected Ch %d.\n", Ch);
        ok(iTrailing, "Got unexpected iTrailing %#x.\n", iTrailing);
    }

    /* Check beyond the end boundary of the whole string. */
    if (rtl[String_len - 1])
    {
        hr = ScriptStringCPtoX(ssa, String_len - 1, FALSE, &X);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        hr = ScriptStringCPtoX(ssa, String_len - 1, TRUE, &X);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }
    ++X;
    hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(Ch == String_len, "Got unexpected Ch %d, expected %d.\n", Ch, String_len);
    ok(!iTrailing, "Got unexpected iTrailing %#x.\n", iTrailing);

    /* Cleanup the SSA for the next round of tests. */
    hr = ScriptStringFree(&ssa);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Test to see that exceeding the number of characters returns
     * E_INVALIDARG. First generate an SSA for the subsequent tests. */
    hr = ScriptStringAnalyse(hdc, &teststr1, String_len, Glyphs, -1,
            SSA_GLYPHS, 100, NULL, NULL, NULL, NULL, InClass, &ssa);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* When ScriptStringCPtoX() is called with a character position that
     * exceeds the string length, return E_INVALIDARG. This also invalidates
     * the ssa so a ScriptStringFree() should also fail. */
    hr = ScriptStringCPtoX(ssa, String_len + 1, FALSE, &X);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    ScriptStringFree(&ssa);
}

static HWND create_test_window(void)
{
    HWND hwnd = CreateWindowExA(0, "Static", "", WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create test window.\n");

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

static void test_ScriptCacheGetHeight(HDC hdc)
{
    HFONT hfont, prev_hfont;
    SCRIPT_CACHE sc = NULL;
    LONG height, height2;
    TEXTMETRICW tm;
    LOGFONTA lf;
    HRESULT hr;
    HWND hwnd;
    HDC hdc2;

    hr = ScriptCacheGetHeight(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptCacheGetHeight(NULL, &sc, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptCacheGetHeight(NULL, &sc, &height);
    ok(hr == E_PENDING, "expected E_PENDING, got 0x%08x\n", hr);

    height = 123;
    hr = ScriptCacheGetHeight(hdc, NULL, &height);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    ok(height == 123, "Unexpected height.\n");

    memset(&tm, 0, sizeof(tm));
    GetTextMetricsW(hdc, &tm);
    ok(tm.tmHeight > 0, "Unexpected tmHeight %u.\n", tm.tmHeight);

    height = 0;
    hr = ScriptCacheGetHeight(hdc, &sc, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(height == tm.tmHeight, "expected height > 0\n");

    /* Try again with NULL dc. */
    height2 = 0;
    hr = ScriptCacheGetHeight(NULL, &sc, &height2);
    ok(hr == S_OK, "Failed to get cached height, hr %#x.\n", hr);
    ok(height2 == height, "Unexpected height %u.\n", height2);

    hwnd = create_test_window();

    hdc2 = GetDC(hwnd);
    ok(hdc2 != NULL, "Failed to get window dc.\n");

    memset(&lf, 0, sizeof(LOGFONTA));
    lstrcpyA(lf.lfFaceName, "Tahoma");
    lf.lfHeight = -32;

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != NULL, "Failed to create font.\n");

    prev_hfont = SelectObject(hdc2, hfont);

    memset(&tm, 0, sizeof(tm));
    GetTextMetricsW(hdc2, &tm);
    ok(tm.tmHeight > height, "Unexpected tmHeight %u.\n", tm.tmHeight);

    height2 = 0;
    hr = ScriptCacheGetHeight(hdc2, &sc, &height2);
    ok(hr == S_OK, "Failed to get cached height, hr %#x.\n", hr);
    ok(height2 == height, "Unexpected height.\n");

    SelectObject(hdc2, prev_hfont);
    DeleteObject(hfont);

    ReleaseDC(hwnd, hdc2);
    DestroyWindow(hwnd);

    ScriptFreeCache(&sc);
}

static void test_ScriptGetGlyphABCWidth(HDC hdc)
{
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    HFONT hfont, prev_hfont;
    TEXTMETRICA tm;
    ABC abc, abc2;
    LOGFONTA lf;
    WORD glyph;
    INT width;
    DWORD ret;

    glyph = 0;
    ret = GetGlyphIndicesA(hdc, "a", 1, &glyph, 0);
    ok(ret == 1, "Failed to get glyph index.\n");
    ok(glyph != 0, "Unexpected glyph index.\n");

    hr = ScriptGetGlyphABCWidth(NULL, NULL, glyph, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptGetGlyphABCWidth(NULL, &sc, glyph, NULL);
    ok(broken(hr == E_PENDING) ||
       hr == E_INVALIDARG, /* WIN7 */
       "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptGetGlyphABCWidth(NULL, &sc, glyph, &abc);
    ok(hr == E_PENDING, "expected E_PENDING, got 0x%08x\n", hr);

    if (0) {    /* crashes on WinXP */
    hr = ScriptGetGlyphABCWidth(hdc, &sc, glyph, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);
    }

    hr = ScriptGetGlyphABCWidth(hdc, &sc, glyph, &abc);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(abc.abcB != 0, "Unexpected width.\n");

    ret = GetCharABCWidthsI(hdc, glyph, 1, NULL, &abc2);
    ok(ret, "Failed to get char width.\n");
    ok(!memcmp(&abc, &abc2, sizeof(abc)), "Unexpected width.\n");

    ScriptFreeCache(&sc);

    /* Bitmap font */
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "System");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    prev_hfont = SelectObject(hdc, hfont);

    ret = GetTextMetricsA(hdc, &tm);
    ok(ret, "Failed to get text metrics.\n");
    ok(!(tm.tmPitchAndFamily & TMPF_TRUETYPE), "Unexpected TrueType font.\n");
    ok(tm.tmPitchAndFamily & TMPF_FIXED_PITCH, "Unexpected fixed pitch font.\n");

    glyph = 0;
    ret = GetGlyphIndicesA(hdc, "i", 1, &glyph, 0);
    ok(ret == 1, "Failed to get glyph index.\n");
    ok(glyph != 0, "Unexpected glyph index.\n");

    sc = NULL;
    hr = ScriptGetGlyphABCWidth(hdc, &sc, glyph, &abc);
    ok(hr == S_OK, "Failed to get glyph width, hr %#x.\n", hr);
    ok(abc.abcB != 0, "Unexpected width.\n");

    ret = GetCharWidthI(hdc, glyph, 1, NULL, &width);
    ok(ret, "Failed to get char width.\n");
    abc2.abcA = abc2.abcC = 0;
    abc2.abcB = width;
    ok(!memcmp(&abc, &abc2, sizeof(abc)), "Unexpected width.\n");

    SelectObject(hdc, prev_hfont);
    DeleteObject(hfont);

    ScriptFreeCache(&sc);
}

static void test_ScriptLayout(void)
{
    HRESULT hr;
    static const BYTE levels[][10] =
    {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },

        { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1},
        { 1, 1, 1, 2, 2, 2, 1, 1, 1, 1 },
        { 2, 2, 2, 1, 1, 1, 2, 2, 2, 2 },
        { 0, 0, 1, 1, 2, 2, 1, 1, 0, 0 },
        { 1, 1, 2, 2, 3, 3, 2, 2, 1, 1 },

        { 0, 0, 1, 1, 2, 2, 1, 1, 0, 1 },
        { 1, 0, 1, 2, 2, 1, 2, 1, 0, 1 },

        { 1, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 2, 2, 2, 0, 0, 0, 0, 0, 0, 0 },
        { 2, 2, 2, 4, 4, 4, 1, 1, 0, 0 },
        { 1, 2, 3, 0, 3, 2, 1, 0, 0, 0 }
    };
    static const int expect_l2v[][10] =
    {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },

        { 0, 1, 2, 3, 4, 9, 8 ,7 ,6, 5},
/**/    { 9, 8, 7, 4, 5, 6, 3 ,2 ,1, 0},
/**/    { 7, 8, 9, 6, 5, 4, 0 ,1 ,2, 3},
        { 0, 1, 7, 6, 4, 5, 3 ,2 ,8, 9},
        { 9, 8, 2, 3, 5, 4, 6 ,7 ,1, 0},

        { 0, 1, 7, 6, 4, 5, 3 ,2 ,8, 9},
/**/    { 0, 1, 7, 5, 6, 4, 3 ,2 ,8, 9},

        { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9},
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        { 2, 3, 4, 5, 6, 7, 1, 0, 8, 9},
        { 2, 0, 1, 3, 5, 6, 4, 7, 8, 9}
    };
    static const int expect_v2l[][10] =
    {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },

        { 0, 1, 2, 3, 4, 9, 8 ,7 ,6, 5},
        { 9, 8, 7, 6, 3, 4, 5 ,2 ,1, 0},
        { 6, 7, 8, 9, 5, 4, 3 ,0 ,1, 2},
        { 0, 1, 7, 6, 4, 5, 3 ,2 ,8, 9},
        { 9, 8, 2, 3, 5, 4, 6 ,7 ,1, 0},

        { 0, 1, 7, 6, 4, 5, 3 ,2 ,8, 9},
        { 0, 1, 7, 6, 5, 3, 4 ,2 ,8, 9},

        { 1, 0, 2, 3, 4, 5, 6, 7, 8, 9},
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        { 7, 6, 0, 1, 2, 3, 4, 5, 8, 9},
        { 1, 2, 0, 3, 6, 4, 5, 7, 8, 9}
    };

    int i, j, vistolog[sizeof(levels[0])], logtovis[sizeof(levels[0])];

    hr = ScriptLayout(sizeof(levels[0]), NULL, vistolog, logtovis);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptLayout(sizeof(levels[0]), levels[0], NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(levels); ++i)
    {
        hr = ScriptLayout(sizeof(levels[0]), levels[i], vistolog, logtovis);
        ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);

        for (j = 0; j < sizeof(levels[i]); j++)
        {
            ok(expect_v2l[i][j] == vistolog[j],
               "failure: levels[%d][%d] = %d, vistolog[%d] = %d\n",
               i, j, levels[i][j], j, vistolog[j] );
        }

        for (j = 0; j < sizeof(levels[i]); j++)
        {
            ok(expect_l2v[i][j] == logtovis[j],
               "failure: levels[%d][%d] = %d, logtovis[%d] = %d\n",
               i, j, levels[i][j], j, logtovis[j] );
        }
    }
}

static BOOL CALLBACK enum_proc(LGRPID group, LCID lcid, LPSTR locale, LONG_PTR lparam)
{
    HRESULT hr;
    SCRIPT_DIGITSUBSTITUTE sds;
    SCRIPT_CONTROL sc;
    SCRIPT_STATE ss;
    LCID lcid_old;

    if (!IsValidLocale(lcid, LCID_INSTALLED)) return TRUE;

    memset(&sds, 0, sizeof(sds));
    memset(&sc, 0, sizeof(sc));
    memset(&ss, 0, sizeof(ss));

    lcid_old = GetThreadLocale();
    if (!SetThreadLocale(lcid)) return TRUE;

    hr = ScriptRecordDigitSubstitution(lcid, &sds);
    ok(hr == S_OK, "ScriptRecordDigitSubstitution failed: 0x%08x\n", hr);

    hr = ScriptApplyDigitSubstitution(&sds, &sc, &ss);
    ok(hr == S_OK, "ScriptApplyDigitSubstitution failed: 0x%08x\n", hr);

    SetThreadLocale(lcid_old);
    return TRUE;
}

static void test_digit_substitution(void)
{
    BOOL ret;
    unsigned int i;
    static const LGRPID groups[] =
    {
        LGRPID_WESTERN_EUROPE,
        LGRPID_CENTRAL_EUROPE,
        LGRPID_BALTIC,
        LGRPID_GREEK,
        LGRPID_CYRILLIC,
        LGRPID_TURKISH,
        LGRPID_JAPANESE,
        LGRPID_KOREAN,
        LGRPID_TRADITIONAL_CHINESE,
        LGRPID_SIMPLIFIED_CHINESE,
        LGRPID_THAI,
        LGRPID_HEBREW,
        LGRPID_ARABIC,
        LGRPID_VIETNAMESE,
        LGRPID_INDIC,
        LGRPID_GEORGIAN,
        LGRPID_ARMENIAN
    };

    for (i = 0; i < ARRAY_SIZE(groups); ++i)
    {
        ret = EnumLanguageGroupLocalesA(enum_proc, groups[i], 0, 0);
        ok(ret, "EnumLanguageGroupLocalesA failed unexpectedly: %u\n", GetLastError());
    }
}

static void test_ScriptGetProperties(void)
{
    const SCRIPT_PROPERTIES **props;
    HRESULT hr;
    int num;

    hr = ScriptGetProperties(NULL, NULL);
    ok(hr == E_INVALIDARG, "ScriptGetProperties succeeded\n");

    hr = ScriptGetProperties(NULL, &num);
    ok(hr == S_OK, "ScriptGetProperties failed: 0x%08x\n", hr);

    hr = ScriptGetProperties(&props, NULL);
    ok(hr == S_OK, "ScriptGetProperties failed: 0x%08x\n", hr);

    hr = ScriptGetProperties(&props, &num);
    ok(hr == S_OK, "ScriptGetProperties failed: 0x%08x\n", hr);
}

static void test_ScriptBreak(void)
{
    static const WCHAR test[] = {' ','\r','\n',0};
    SCRIPT_ITEM items[4];
    SCRIPT_LOGATTR la;
    HRESULT hr;

    hr = ScriptItemize(test, 3, 4, NULL, NULL, items, NULL);
    ok(hr == S_OK, "ScriptItemize should return S_OK not %08x\n", hr);

    /*
     * This Test crashes pre Vista.

    hr = ScriptBreak(test, 1, &items[0].a, NULL);
    ok(hr == E_INVALIDARG, "ScriptBreak should return E_INVALIDARG not %08x\n", hr);
    */

    hr = ScriptBreak(test, 0, &items[0].a, &la);
    ok(hr == E_FAIL || broken(hr == S_OK), "ScriptBreak should return E_FAIL not %08x\n", hr);

    hr = ScriptBreak(test, -1, &items[0].a, &la);
    ok(hr == E_INVALIDARG || broken(hr == S_OK), "ScriptBreak should return E_INVALIDARG not %08x\n", hr);

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test, 1, &items[0].a, &la);
    ok(hr == S_OK, "ScriptBreak should return S_OK not %08x\n", hr);

    ok(!la.fSoftBreak, "fSoftBreak set\n");
    ok(la.fWhiteSpace, "fWhiteSpace not set\n");
    ok(la.fCharStop, "fCharStop not set\n");
    ok(!la.fWordStop, "fWordStop set\n");
    ok(!la.fInvalid, "fInvalid set\n");
    ok(!la.fReserved, "fReserved set\n");

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test + 1, 1, &items[1].a, &la);
    ok(hr == S_OK, "ScriptBreak should return S_OK not %08x\n", hr);

    ok(!la.fSoftBreak, "fSoftBreak set\n");
    ok(!la.fWhiteSpace, "fWhiteSpace set\n");
    ok(la.fCharStop, "fCharStop not set\n");
    ok(!la.fWordStop, "fWordStop set\n");
    ok(!la.fInvalid, "fInvalid set\n");
    ok(!la.fReserved, "fReserved set\n");

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test + 2, 1, &items[2].a, &la);
    ok(hr == S_OK, "ScriptBreak should return S_OK not %08x\n", hr);

    ok(!la.fSoftBreak, "fSoftBreak set\n");
    ok(!la.fWhiteSpace, "fWhiteSpace set\n");
    ok(la.fCharStop, "fCharStop not set\n");
    ok(!la.fWordStop, "fWordStop set\n");
    ok(!la.fInvalid, "fInvalid set\n");
    ok(!la.fReserved, "fReserved set\n");
}

static void test_newlines(void)
{
    static const WCHAR test1[] = {'t','e','x','t','\r','t','e','x','t',0};
    static const WCHAR test2[] = {'t','e','x','t','\n','t','e','x','t',0};
    static const WCHAR test3[] = {'t','e','x','t','\r','\n','t','e','x','t',0};
    static const WCHAR test4[] = {'t','e','x','t','\n','\r','t','e','x','t',0};
    static const WCHAR test5[] = {'1','2','3','4','\n','\r','1','2','3','4',0};
    SCRIPT_ITEM items[5];
    HRESULT hr;
    int count;

    count = 0;
    hr = ScriptItemize(test1, lstrlenW(test1), 5, NULL, NULL, items, &count);
    ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
    ok(count == 3, "got %d expected 3\n", count);

    count = 0;
    hr = ScriptItemize(test2, lstrlenW(test2), 5, NULL, NULL, items, &count);
    ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
    ok(count == 3, "got %d expected 3\n", count);

    count = 0;
    hr = ScriptItemize(test3, lstrlenW(test3), 5, NULL, NULL, items, &count);
    ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
    ok(count == 4, "got %d expected 4\n", count);

    count = 0;
    hr = ScriptItemize(test4, lstrlenW(test4), 5, NULL, NULL, items, &count);
    ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
    ok(count == 4, "got %d expected 4\n", count);

    count = 0;
    hr = ScriptItemize(test5, lstrlenW(test5), 5, NULL, NULL, items, &count);
    ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
    ok(count == 4, "got %d expected 4\n", count);
}

static void test_ScriptGetFontFunctions(HDC hdc)
{
    static const WCHAR test_phagspa[] = {0xa84f, 0xa861, 0xa843, 0x0020, 0xa863, 0xa861, 0xa859,
            0x0020, 0xa850, 0xa85c, 0xa85e};
    SCRIPT_CONTROL control;
    SCRIPT_CACHE sc = NULL;
    SCRIPT_ITEM items[15];
    OPENTYPE_TAG tags[5];
    SCRIPT_STATE state;
    int count = 0;
    HRESULT hr;

    if (!pScriptGetFontScriptTags || !pScriptGetFontLanguageTags || !pScriptGetFontFeatureTags)
    {
        win_skip("ScriptGetFontScriptTags, ScriptGetFontLanguageTags or "
                "ScriptGetFontFeatureTags not available on this platform.\n");
        return;
    }

    hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, NULL, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontScriptTags(hdc, &sc, NULL, ARRAY_SIZE(tags), tags, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, tags, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontScriptTags(NULL, &sc, NULL, ARRAY_SIZE(tags), tags, &count);
    ok(hr == E_PENDING, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontScriptTags(hdc, &sc, NULL, ARRAY_SIZE(tags), tags, &count);
    ok(hr == S_OK || hr == E_OUTOFMEMORY, "Got unexpected hr %#x.\n", hr);
    if (hr == S_OK)
        ok(count <= 5, "Got unexpected count %d.\n", count);
    else
        ok(!count, "Got unexpected count %d.\n", count);
    ok(!!sc, "Got unexpected script cache %p.\n", sc);

    ScriptFreeCache(&sc);
    sc = NULL;

    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, NULL, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, ARRAY_SIZE(tags), tags, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, tags, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontLanguageTags(NULL, &sc, NULL, latn_tag, ARRAY_SIZE(tags), tags, &count);
    ok(hr == E_PENDING, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, ARRAY_SIZE(tags), tags, &count);
    ok(hr == S_OK || hr == E_OUTOFMEMORY, "Got unexpected hr %#x.\n", hr);
    if (hr == S_OK)
        ok(count <= 5, "Got unexpected count %d.\n", count);
    else
        ok(!count, "Got unexpected count %d.\n", count);

    ScriptFreeCache(&sc);
    sc = NULL;

    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, NULL, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, ARRAY_SIZE(tags), tags, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, tags, &count);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontFeatureTags(NULL, &sc, NULL, latn_tag, 0x0, ARRAY_SIZE(tags), tags, &count);
    ok(hr == E_PENDING, "Got unexpected hr %#x.\n", hr);
    ok(!sc, "Got unexpected script cache %p.\n", sc);
    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, ARRAY_SIZE(tags), tags, &count);
    ok(hr == S_OK || hr == E_OUTOFMEMORY, "Got unexpected hr %#x.\n", hr);
    if (hr == S_OK)
        ok(count <= 5, "Got unexpected count %d.\n", count);
    else
        ok(!count, "Got unexpected count %d.\n", count);

    memset(&control, 0, sizeof(control));
    memset(&state, 0, sizeof(state));

    hr = ScriptItemize(test_phagspa, ARRAY_SIZE(test_phagspa), ARRAY_SIZE(items),
            &control, &state, items, &count);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    memset(tags, 0, sizeof(tags));
    hr = pScriptGetFontScriptTags(hdc, &sc, &items[0].a, ARRAY_SIZE(tags), tags, &count);
    ok(hr == USP_E_SCRIPT_NOT_IN_FONT || broken(hr == S_OK), "Got unexpected hr %#x.\n", hr);

    hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, dsrt_tag, ARRAY_SIZE(tags), tags, &count);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = pScriptGetFontLanguageTags(hdc, &sc, &items[0].a, dsrt_tag, ARRAY_SIZE(tags), tags, &count);
    ok(hr == E_INVALIDARG || broken(hr == S_OK), "Got unexpected hr %#x.\n", hr);

    hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, dsrt_tag, 0x0, ARRAY_SIZE(tags), tags, &count);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = pScriptGetFontFeatureTags(hdc, &sc, &items[0].a, dsrt_tag, 0x0, ARRAY_SIZE(tags), tags, &count);
    ok(hr == E_INVALIDARG || broken(hr == S_OK), "Got unexpected hr %#x.\n", hr);

    ScriptFreeCache(&sc);
}

struct logical_width_test
{
    int char_count;
    int glyph_count;
    int advances[3];
    WORD map[3];
    int widths[3];
    BOOL clusterstart[3];
    BOOL diacritic[3];
    BOOL zerowidth[3];
    BOOL todo;
};

static const struct logical_width_test logical_width_tests[] =
{
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, {  6,  9, 12 }, { 1, 1, 1 } },
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, {  6,  9, 12 }, { 1, 1, 1 }, { 1, 0, 0 } },
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, {  6,  9, 12 }, { 1, 1, 1 }, { 0 }, { 1, 1, 1 } },
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, { 27, 21, 12 }, { 0, 0, 0 }, { 0 }, { 0 }, TRUE },
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, {  6, 21, 12 }, { 0, 1, 0 }, { 0 }, { 0 }, TRUE },
    { 3, 3, { 6, 9, 12 }, { 0, 1, 2 }, {  6, 21, 12 }, { 1, 1, 0 }, { 0 }, { 0 }, TRUE },
    { 3, 3, { 6, 9, 12 }, { 0, 2, 2 }, { 15,  6,  6 }, { 1, 0, 1 } },
};

static void test_ScriptGetLogicalWidths(void)
{
    SCRIPT_ANALYSIS sa = { 0 };
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(logical_width_tests); ++i)
    {
        const struct logical_width_test *ptr = logical_width_tests + i;
        SCRIPT_VISATTR attrs[3];
        int widths[3];
        HRESULT hr;

        memset(attrs, 0, sizeof(attrs));
        for (j = 0; j < ptr->glyph_count; j++)
        {
            attrs[j].fClusterStart = ptr->clusterstart[j];
            attrs[j].fDiacritic = ptr->diacritic[j];
            attrs[j].fZeroWidth = ptr->zerowidth[j];
        }

        hr = ScriptGetLogicalWidths(&sa, ptr->char_count, ptr->glyph_count, ptr->advances, ptr->map, attrs, widths);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        todo_wine_if(ptr->todo)
            ok(!memcmp(ptr->widths, widths, sizeof(widths)), "test %u: got wrong widths\n", i);
    }
}

static void test_ScriptIsComplex(void)
{
    static const WCHAR testW[] = {0x202a,'1',0x202c,0};
    static const WCHAR test2W[] = {'1',0};
    static const struct complex_test
    {
        const WCHAR *text;
        DWORD flags;
        HRESULT hr;
        BOOL todo;
    } complex_tests[] =
    {
        { test2W, SIC_ASCIIDIGIT, S_OK },
        { test2W, SIC_COMPLEX, S_FALSE },
        { test2W, SIC_COMPLEX | SIC_ASCIIDIGIT, S_OK },
        { testW, SIC_NEUTRAL | SIC_COMPLEX, S_OK },
        { testW, SIC_NEUTRAL, S_FALSE, TRUE },
        { testW, SIC_COMPLEX, S_OK },
        { testW, 0, S_FALSE },
    };
    unsigned int i;
    HRESULT hr;

    hr = ScriptIsComplex(NULL, 0, 0);
    ok(hr == E_INVALIDARG || broken(hr == S_FALSE) /* winxp/vista */, "got 0x%08x\n", hr);

    if (hr == E_INVALIDARG)
    {
        hr = ScriptIsComplex(NULL, 1, 0);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    }

    hr = ScriptIsComplex(test2W, -1, SIC_ASCIIDIGIT);
    ok(hr == E_INVALIDARG || broken(hr == S_FALSE) /* winxp/vista */, "got 0x%08x\n", hr);

    hr = ScriptIsComplex(test2W, 0, SIC_ASCIIDIGIT);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(complex_tests); ++i)
    {
        hr = ScriptIsComplex(complex_tests[i].text, lstrlenW(complex_tests[i].text), complex_tests[i].flags);
    todo_wine_if(complex_tests[i].todo)
        ok(hr == complex_tests[i].hr, "%u: got %#x, expected %#x, flags %#x\n", i, hr, complex_tests[i].hr,
            complex_tests[i].flags);
    }

    hr = ScriptIsComplex(test2W, 1, ~0u);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ScriptIsComplex(testW, 3, 0);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);

    hr = ScriptIsComplex(testW, 3, SIC_NEUTRAL | SIC_COMPLEX);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ScriptIsComplex(testW, 3, SIC_COMPLEX);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ScriptIsComplex(test2W, 1, SIC_COMPLEX);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
}

static void test_ScriptString_pSize(HDC hdc)
{
    static const WCHAR textW[] = {'A',0};
    SCRIPT_STRING_ANALYSIS ssa;
    const SIZE *size;
    TEXTMETRICW tm;
    HRESULT hr;
    ABC abc;

    hr = ScriptStringAnalyse(hdc, textW, 1, 16, -1, SSA_GLYPHS, 0, NULL, NULL, NULL, NULL, NULL, &ssa);
    ok(hr == S_OK, "ScriptStringAnalyse failed, hr %#x.\n", hr);

    size = ScriptString_pSize(NULL);
    ok(size == NULL || broken(size != NULL) /* <win7 */, "Unexpected size pointer.\n");

    GetCharABCWidthsW(hdc, textW[0], textW[0], &abc);

    memset(&tm, 0, sizeof(tm));
    GetTextMetricsW(hdc, &tm);
    ok(tm.tmHeight > 0, "Unexpected tmHeight.\n");

    size = ScriptString_pSize(ssa);
    ok(size != NULL, "Unexpected size pointer.\n");
    ok(size->cx == abc.abcA + abc.abcB + abc.abcC, "Unexpected cx size %d.\n", size->cx);
    ok(size->cy == tm.tmHeight, "Unexpected cy size %d.\n", size->cy);

    hr = ScriptStringFree(&ssa);
    ok(hr == S_OK, "Failed to free ssa, hr %#x.\n", hr);
}

static void test_script_cache_reuse(void)
{
    HRESULT hr;
    HWND hwnd1, hwnd2;
    HDC hdc1, hdc2;
    LOGFONTA lf;
    HFONT hfont1, hfont2;
    HFONT prev_hfont1, prev_hfont2;
    SCRIPT_CACHE sc = NULL;
    SCRIPT_CACHE sc2;
    LONG height;

    hwnd1 = create_test_window();
    hwnd2 = create_test_window();

    hdc1 = GetDC(hwnd1);
    hdc2 = GetDC(hwnd2);
    ok(hdc1 != NULL && hdc2 != NULL, "Failed to get window dc.\n");

    memset(&lf, 0, sizeof(LOGFONTA));
    lstrcpyA(lf.lfFaceName, "Tahoma");

    lf.lfHeight = 10;
    hfont1 = CreateFontIndirectA(&lf);
    ok(hfont1 != NULL, "CreateFontIndirectA failed\n");
    hfont2 = CreateFontIndirectA(&lf);
    ok(hfont2 != NULL, "CreateFontIndirectA failed\n");
    ok(hfont1 != hfont2, "Expected fonts %p and %p to differ\n", hfont1, hfont2);

    prev_hfont1 = SelectObject(hdc1, hfont1);
    ok(prev_hfont1 != NULL, "SelectObject failed: %p\n", prev_hfont1);
    prev_hfont2 = SelectObject(hdc2, hfont1);
    ok(prev_hfont2 != NULL, "SelectObject failed: %p\n", prev_hfont2);

    /* Get a script cache */
    hr = ScriptCacheGetHeight(hdc1, &sc, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc != NULL, "Script cache is NULL\n");

    /* Same font, same DC -> same SCRIPT_CACHE */
    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc1, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc == sc2, "Expected caches %p, %p to be identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    /* Same font in different DC -> same SCRIPT_CACHE */
    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc2, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc == sc2, "Expected caches %p, %p to be identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    /* Same font face & size, but different font handle */
    ok(SelectObject(hdc1, hfont2) != NULL, "SelectObject failed\n");
    ok(SelectObject(hdc2, hfont2) != NULL, "SelectObject failed\n");

    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc1, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc == sc2, "Expected caches %p, %p to be identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc2, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc == sc2, "Expected caches %p, %p to be identical\n", sc, sc2);
    ScriptFreeCache(&sc2);

    /* Different font size -- now we get a different SCRIPT_CACHE */
    SelectObject(hdc1, prev_hfont1);
    SelectObject(hdc2, prev_hfont2);
    DeleteObject(hfont2);
    lf.lfHeight = 20;
    hfont2 = CreateFontIndirectA(&lf);
    ok(hfont2 != NULL, "CreateFontIndirectA failed\n");
    ok(SelectObject(hdc1, hfont2) != NULL, "SelectObject failed\n");
    ok(SelectObject(hdc2, hfont2) != NULL, "SelectObject failed\n");

    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc1, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc != sc2, "Expected caches %p, %p to be different\n", sc, sc2);
    ScriptFreeCache(&sc2);

    sc2 = NULL;
    hr = ScriptCacheGetHeight(hdc2, &sc2, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(sc2 != NULL, "Script cache is NULL\n");
    ok(sc != sc2, "Expected caches %p, %p to be different\n", sc, sc2);
    ScriptFreeCache(&sc2);

    ScriptFreeCache(&sc);
    SelectObject(hdc1, prev_hfont1);
    SelectObject(hdc2, prev_hfont2);
    DeleteObject(hfont1);
    DeleteObject(hfont2);
    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
}

static void init_tests(void)
{
    HMODULE module = GetModuleHandleA("usp10.dll");

    ok(module != 0, "Expected usp10.dll to be loaded.\n");

    pScriptItemizeOpenType = (void *)GetProcAddress(module, "ScriptItemizeOpenType");
    pScriptShapeOpenType = (void *)GetProcAddress(module, "ScriptShapeOpenType");
    pScriptGetFontScriptTags = (void *)GetProcAddress(module, "ScriptGetFontScriptTags");
    pScriptGetFontLanguageTags = (void *)GetProcAddress(module, "ScriptGetFontLanguageTags");
    pScriptGetFontFeatureTags = (void *)GetProcAddress(module, "ScriptGetFontFeatureTags");
}

START_TEST(usp10)
{
    HWND            hwnd;
    HDC             hdc;
    LOGFONTA        lf;
    HFONT           hfont;

    unsigned short  pwOutGlyphs[256];

    /* We need a valid HDC to drive a lot of Script functions which requires the following    *
     * to set up for the tests.                                                               */
    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    assert(hwnd != 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hdc = GetDC(hwnd);                                      /* We now have a hdc             */
    ok( hdc != NULL, "HDC failed to be created %p\n", hdc);

    memset(&lf, 0, sizeof(LOGFONTA));
    lstrcpyA(lf.lfFaceName, "Tahoma");
    lf.lfHeight = 10;
    lf.lfWeight = 3;
    lf.lfWidth = 10;

    hfont = SelectObject(hdc, CreateFontIndirectA(&lf));
    ok(hfont != NULL, "SelectObject failed: %p\n", hfont);

    init_tests();

    test_ScriptItemize();
    test_ScriptItemize_surrogates();
    test_ScriptItemIzeShapePlace(hdc,pwOutGlyphs);
    test_ScriptGetCMap(hdc, pwOutGlyphs);
    test_ScriptCacheGetHeight(hdc);
    test_ScriptGetGlyphABCWidth(hdc);
    test_ScriptShape(hdc);
    test_ScriptShapeOpenType(hdc);
    test_ScriptPlace(hdc);

    test_ScriptGetFontProperties(hdc);
    test_ScriptTextOut(hdc);
    test_ScriptTextOut2(hdc);
    test_ScriptTextOut3(hdc);
    test_ScriptXtoX();
    test_ScriptString(hdc);
    test_ScriptStringXtoCP_CPtoX(hdc);
    test_ScriptString_pSize(hdc);

    test_ScriptLayout();
    test_digit_substitution();
    test_ScriptGetProperties();
    test_ScriptBreak();
    test_newlines();

    test_ScriptGetFontFunctions(hdc);
    test_ScriptGetLogicalWidths();

    test_ScriptIsComplex();
    test_script_cache_reuse();

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}
