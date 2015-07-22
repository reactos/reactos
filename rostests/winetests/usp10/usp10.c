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
#include <winnls.h>
#include <wingdi.h>
#include <winuser.h>
//#include <windows.h>
#include <usp10.h>

typedef struct _itemTest {
    char todo_flag[5];
    int iCharPos;
    int fRTL;
    int fLayoutRTL;
    int uBidiLevel;
    ULONG scriptTag;
    BOOL isBroken;
    int broken_value[5];
} itemTest;

typedef struct _shapeTest_char {
    WORD wLogClust;
    SCRIPT_CHARPROP CharProp;
} shapeTest_char;

typedef struct _shapeTest_glyph {
    int Glyph;
    SCRIPT_GLYPHPROP GlyphProp;
} shapeTest_glyph;

/* Uniscribe 1.6 calls */
static HRESULT (WINAPI *pScriptItemizeOpenType)( const WCHAR *pwcInChars, int cInChars, int cMaxItems, const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState, SCRIPT_ITEM *pItems, ULONG *pScriptTags, int *pcItems);

static HRESULT (WINAPI *pScriptShapeOpenType)( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript, OPENTYPE_TAG tagLangSys, int *rcRangeChars, TEXTRANGE_PROPERTIES **rpRangeProperties, int cRanges, const WCHAR *pwcChars, int cChars, int cMaxGlyphs, WORD *pwLogClust, SCRIPT_CHARPROP *pCharProps, WORD *pwOutGlyphs, SCRIPT_GLYPHPROP *pOutGlyphProps, int *pcGlyphs);

static DWORD (WINAPI *pGetGlyphIndicesW)(HDC hdc, LPCWSTR lpstr, INT count, LPWORD pgi, DWORD flags);

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
        winetest_win_skip("This test broken on this platform\n");
        return;
    }
    if (nItemsToDo)
        todo_wine winetest_ok(outnItems == nItems, "Wrong number of items\n");
    else
        winetest_ok(outnItems == nItems, "Wrong number of items\n");
    for (x = 0; x <= outnItems; x++)
    {
        if (items[x].isBroken && broken(outpItems[x].iCharPos == items[x].broken_value[0]))
            winetest_win_skip("This test broken on this platform\n");
        else if (items[x].todo_flag[0])
            todo_wine winetest_ok(outpItems[x].iCharPos == items[x].iCharPos, "%i:Wrong CharPos\n",x);
        else
            winetest_ok(outpItems[x].iCharPos == items[x].iCharPos, "%i:Wrong CharPos (%i)\n",x,outpItems[x].iCharPos);

        if (items[x].isBroken && broken(outpItems[x].a.fRTL== items[x].broken_value[1]))
            winetest_win_skip("This test broken on this platform\n");
        else if (items[x].todo_flag[1])
            todo_wine winetest_ok(outpItems[x].a.fRTL == items[x].fRTL, "%i:Wrong fRTL\n",x);
        else
            winetest_ok(outpItems[x].a.fRTL == items[x].fRTL, "%i:Wrong fRTL(%i)\n",x,outpItems[x].a.fRTL);

        if (items[x].isBroken && broken(outpItems[x].a.fLayoutRTL == items[x].broken_value[2]))
            winetest_win_skip("This test broken on this platform\n");
        else if (items[x].todo_flag[2])
            todo_wine winetest_ok(outpItems[x].a.fLayoutRTL == items[x].fLayoutRTL, "%i:Wrong fLayoutRTL\n",x);
        else
            winetest_ok(outpItems[x].a.fLayoutRTL == items[x].fLayoutRTL, "%i:Wrong fLayoutRTL(%i)\n",x,outpItems[x].a.fLayoutRTL);

        if (items[x].isBroken && broken(outpItems[x].a.s.uBidiLevel == items[x].broken_value[3]))
            winetest_win_skip("This test broken on this platform\n");
        else if (items[x].todo_flag[3])
            todo_wine winetest_ok(outpItems[x].a.s.uBidiLevel == items[x].uBidiLevel, "%i:Wrong BidiLevel\n",x);
        else
            winetest_ok(outpItems[x].a.s.uBidiLevel == items[x].uBidiLevel, "%i:Wrong BidiLevel(%i)\n",x,outpItems[x].a.s.uBidiLevel);
        if (x != outnItems)
            winetest_ok(outpItems[x].a.eScript != SCRIPT_UNDEFINED, "%i: Undefined script\n",x);
        if (pScriptItemizeOpenType)
        {
            if (items[x].isBroken && broken(tags[x] == items[x].broken_value[4]))
                winetest_win_skip("This test broken on this platform\n");
            else if (items[x].todo_flag[4])
                todo_wine winetest_ok(tags[x] == items[x].scriptTag,"%i:Incorrect Script Tag %x != %x\n",x,tags[x],items[x].scriptTag);
            else
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
    static const itemTest t11[2] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1}};
    static const itemTest t12[2] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    static const WCHAR test1b[] = {' ', ' ', ' ', ' ',0};
    static const itemTest t1b1[2] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};
    static const itemTest t1b2[2] = {{{0,0,0,0,0},0,1,1,1,0,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    static const WCHAR test1c[] = {' ', ' ', ' ', '1', '2', ' ',0};
    static const itemTest t1c1[2] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t1c2[4] = {{{0,0,0,0,0},0,1,1,1,0,FALSE},{{0,0,0,0,0},3,0,1,2,0,FALSE},{{0,0,0,0,0},5,1,1,1,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Arabic, English*/
    static const WCHAR test2[] = {'1','2','3','-','5','2',0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,'7','1','.',0};
    static const itemTest t21[7] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,0,FALSE},{{0,0,0,0,0},6,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},13,0,0,0,0,FALSE},{{0,0,0,0,0},15,0,0,0,0,FALSE},{{0,0,0,0,0},16,0,0,0,-1,FALSE}};
    static const itemTest t22[5] = {{{0,0,0,0,0},0,0,0,2,0,FALSE},{{0,0,0,0,0},6,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},13,0,1,2,0,FALSE},{{0,0,0,0,0},15,0,0,0,0,FALSE},{{0,0,0,0,0},16,0,0,0,-1,FALSE}};
    static const itemTest t23[5] = {{{0,0,0,0,0},0,0,1,2,0,FALSE},{{0,0,0,0,0},6,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},13,0,1,2,0,FALSE},{{0,0,0,0,0},15,1,1,1,0,FALSE},{{0,0,0,0,0},16,0,0,0,-1,FALSE}};

    static const WCHAR test2b[] = {'A','B','C','-','D','E','F',' ',0x0621,0x0623,0x0624,0};
    static const itemTest t2b1[5] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},8,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2b2[5] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,2,0,FALSE},{{0,0,0,0,0},4,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2b3[3] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const int b2[2] = {4,4};

    /* leading space */
    static const WCHAR test2c[] = {' ',0x0621,0x0623,0x0624,'A','B','C','-','D','E','F',0};
    static const itemTest t2c1[5] = {{{0,0,0,0,0},0,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},4,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},7,0,0,0,0,FALSE},{{0,0,0,0,0},8,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2c2[6] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},1,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},4,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},7,0,0,0,0,FALSE},{{0,0,0,0,0},8,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2c3[5] = {{{0,0,0,0,0},0,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},4,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},7,0,0,2,0,FALSE},{{0,0,0,0,0},8,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2c4[3] = {{{0,0,0,0,0},0,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},4,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};

    /* trailing space */
    static const WCHAR test2d[] = {'A','B','C','-','D','E','F',0x0621,0x0623,0x0624,' ',0};
    static const itemTest t2d1[5] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2d2[6] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},10,0,0,0,0,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2d3[5] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,2,0,FALSE},{{0,0,0,0,0},4,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};
    static const itemTest t2d4[3] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},7,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},11,0,0,0,-1,FALSE}};

    /* Thai */
    static const WCHAR test3[] =
{0x0e04,0x0e27,0x0e32,0x0e21,0x0e1e,0x0e22,0x0e32,0x0e22,0x0e32, 0x0e21
,0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e44,0x0e2b,0x0e19
,0x0e04,0x0e27,0x0e32,0x0e21,0x0e2a, 0x0e33,0x0e40,0x0e23,0x0e47,0x0e08,
 0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e19,0x0e31,0x0e48,0x0e19,0};

    static const itemTest t31[2] = {{{0,0,0,0,0},0,0,0,0,thai_tag,FALSE},{{0,0,0,0,0},41,0,0,0,-1,FALSE}};
    static const itemTest t32[2] = {{{0,0,0,0,0},0,0,0,2,thai_tag,FALSE},{{0,0,0,0,0},41,0,0,0,-1,FALSE}};

    static const WCHAR test4[]  = {'1','2','3','-','5','2',' ','i','s',' ','7','1','.',0};

    static const itemTest t41[6] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,0,FALSE},{{0,0,0,0,0},7,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},10,0,0,0,0,FALSE},{{0,0,0,0,0},12,0,0,0,-1,FALSE}};
    static const itemTest t42[5] = {{{0,0,0,0,0},0,0,1,2,0,FALSE},{{0,0,0,0,0},6,1,1,1,0,FALSE},{{0,0,0,0,0},7,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},10,0,0,2,0,FALSE},{{0,0,0,0,0},12,0,0,0,-1,FALSE}};
    static const itemTest t43[4] = {{{0,0,0,0,0},0,0,1,2,0,FALSE},{{0,0,0,0,0},6,1,1,1,0,FALSE},{{0,0,0,0,0},7,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},12,0,0,0,-1,FALSE}};
    static const int b43[2] = {4,4};

    /* Arabic */
    static const WCHAR test5[]  =
{0x0627,0x0644,0x0635,0x0651,0x0650,0x062d,0x0629,0x064f,' ',0x062a,0x064e,
0x0627,0x062c,0x064c,' ',0x0639,0x064e,0x0644,0x0649,' ',
0x0631,0x064f,0x0624,0x0648,0x0633,0x0650,' ',0x0627,0x0644
,0x0623,0x0635,0x0650,0x062d,0x0651,0x064e,0x0627,0x0621,0x0650,0};
    static const itemTest t51[2] = {{{0,0,0,0,0},0,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},38,0,0,0,-1,FALSE}};

    /* Hebrew */
    static const WCHAR test6[]  = {0x05e9, 0x05dc, 0x05d5, 0x05dd, '.',0};
    static const itemTest t61[3] = {{{0,0,0,0,0},0,1,1,1,hebr_tag,TRUE,{-1,0,0,0,-1}},{{0,0,0,0,0},4,0,0,0,0,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t62[3] = {{{0,0,0,0,0},0,1,1,1,hebr_tag,FALSE},{{0,0,0,0,0},4,1,1,1,0,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t63[2] = {{{0,0,0,0,0},0,1,1,1,hebr_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const int b63[2] = {2,2};
    static const WCHAR test7[]  = {'p','a','r','t',' ','o','n','e',' ',0x05d7, 0x05dc, 0x05e7, ' ', 0x05e9, 0x05ea, 0x05d9, 0x05d9, 0x05dd, ' ','p','a','r','t',' ','t','h','r','e','e', 0};
    static const itemTest t71[4] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},9,1,1,1,hebr_tag,TRUE,{-1,0,0,0,-1}},{{0,0,0,0,0},19,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},29,0,0,0,-1,FALSE}};
    static const itemTest t72[4] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},9,1,1,1,hebr_tag,FALSE},{{0,0,0,0,0},18,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},29,0,0,0,-1,FALSE}};
    static const itemTest t73[4] = {{{0,0,0,0,0},0,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},8,1,1,1,hebr_tag,FALSE},{{0,0,0,0,0},19,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},29,0,0,0,-1,FALSE}};
    static const WCHAR test8[] = {0x0633, 0x0644, 0x0627, 0x0645,0};
    static const itemTest t81[2] = {{{0,0,0,0,0},0,1,1,1,arab_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    /* Syriac  (Like Arabic )*/
    static const WCHAR test9[] = {0x0710, 0x0712, 0x0712, 0x0714, '.',0};
    static const itemTest t91[3] = {{{0,0,0,0,0},0,1,1,1,syrc_tag,FALSE},{{0,0,0,0,0},4,0,0,0,0,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t92[3] = {{{0,0,0,0,0},0,1,1,1,syrc_tag},{{0,0,0,0,0},4,1,1,1,0,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t93[2] = {{{0,0,0,0,0},0,1,1,1,syrc_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const int b93[2] = {2,2};

    static const WCHAR test10[] = {0x0717, 0x0718, 0x071a, 0x071b,0};
    static const itemTest t101[2] = {{{0,0,0,0,0},0,1,1,1,syrc_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    /* Devanagari */
    static const WCHAR test11[] = {0x0926, 0x0947, 0x0935, 0x0928, 0x093e, 0x0917, 0x0930, 0x0940};
    static const itemTest t111[2] = {{{0,0,0,0,0},0,0,0,0,deva_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};
    static const itemTest t112[2] = {{{0,0,0,0,0},0,0,0,2,deva_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};

    /* Bengali */
    static const WCHAR test12[] = {0x09ac, 0x09be, 0x0982, 0x09b2, 0x09be};
    static const itemTest t121[2] = {{{0,0,0,0,0},0,0,0,0,beng_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t122[2] = {{{0,0,0,0,0},0,0,0,2,beng_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    /* Gurmukhi */
    static const WCHAR test13[] = {0x0a17, 0x0a41, 0x0a30, 0x0a2e, 0x0a41, 0x0a16, 0x0a40};
    static const itemTest t131[2] = {{{0,0,0,0,0},0,0,0,0,guru_tag,FALSE},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};
    static const itemTest t132[2] = {{{0,0,0,0,0},0,0,0,2,guru_tag,FALSE},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};

    /* Gujarati */
    static const WCHAR test14[] = {0x0a97, 0x0ac1, 0x0a9c, 0x0ab0, 0x0abe, 0x0aa4, 0x0ac0};
    static const itemTest t141[2] = {{{0,0,0,0,0},0,0,0,0,gujr_tag,FALSE},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};
    static const itemTest t142[2] = {{{0,0,0,0,0},0,0,0,2,gujr_tag,FALSE},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};

    /* Oriya */
    static const WCHAR test15[] = {0x0b13, 0x0b21, 0x0b3c, 0x0b3f, 0x0b06};
    static const itemTest t151[2] = {{{0,0,0,0,0},0,0,0,0,orya_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t152[2] = {{{0,0,0,0,0},0,0,0,2,orya_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    /* Tamil */
    static const WCHAR test16[] = {0x0ba4, 0x0bae, 0x0bbf, 0x0bb4, 0x0bcd};
    static const itemTest t161[2] = {{{0,0,0,0,0},0,0,0,0,taml_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t162[2] = {{{0,0,0,0,0},0,0,0,2,taml_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    /* Telugu */
    static const WCHAR test17[] = {0x0c24, 0x0c46, 0x0c32, 0x0c41, 0x0c17, 0x0c41};
    static const itemTest t171[2] = {{{0,0,0,0,0},0,0,0,0,telu_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t172[2] = {{{0,0,0,0,0},0,0,0,2,telu_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Kannada */
    static const WCHAR test18[] = {0x0c95, 0x0ca8, 0x0ccd, 0x0ca8, 0x0ca1};
    static const itemTest t181[2] = {{{0,0,0,0,0},0,0,0,0,knda_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t182[2] = {{{0,0,0,0,0},0,0,0,2,knda_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    /* Malayalam */
    static const WCHAR test19[] = {0x0d2e, 0x0d32, 0x0d2f, 0x0d3e, 0x0d33, 0x0d02};
    static const itemTest t191[2] = {{{0,0,0,0,0},0,0,0,0,mlym_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t192[2] = {{{0,0,0,0,0},0,0,0,2,mlym_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Diacritical */
    static const WCHAR test20[] = {0x0309,'a','b','c','d',0};
    static const itemTest t201[3] = {{{0,0,0,0,0},0,0,0,0,0x0,FALSE},{{0,0,0,0,0},1,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};
    static const itemTest t202[3] = {{{0,0,0,0,0},0,0,0,2,0,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},1,0,0,2,latn_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    static const WCHAR test21[] = {0x0710, 0x0712, 0x0308, 0x0712, 0x0714,0};
    static const itemTest t211[2] = {{{0,0,0,0,0},0,1,1,1,syrc_tag,FALSE},{{0,0,0,0,0},5,0,0,0,-1,FALSE}};

    /* Latin Punctuation */
    static const WCHAR test22[] = {'#','$',',','!','\"','*',0};
    static const itemTest t221[3] = {{{0,0,0,0,0},0,0,0,0,latn_tag,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t222[3] = {{{0,0,0,0,0},0,1,1,1,latn_tag,FALSE},{{0,0,0,0,0},3,1,1,1,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t223[2] = {{{0,0,0,0,0},0,1,1,1,latn_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const int b222[2] = {1,1};
    static const int b223[2] = {2,2};

    /* Number 2*/
    static const WCHAR test23[] = {'1','2','3',0x00b2,0x00b3,0x2070,0};
    static const itemTest t231[3] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},3,0,0,0,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t232[3] = {{{0,0,0,0,0},0,0,1,2,0,FALSE},{{0,0,0,0,0},3,0,1,2,0,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Myanmar */
    static const WCHAR test24[] = {0x1019,0x103c,0x1014,0x103a,0x1019,0x102c,0x1021,0x1000,0x1039,0x1001,0x101b,0x102c};
    static const itemTest t241[2] = {{{0,0,0,0,0},0,0,0,0,mymr_tag,FALSE},{{0,0,0,0,0},12,0,0,0,-1,FALSE}};
    static const itemTest t242[2] = {{{0,0,0,0,0},0,0,0,2,mymr_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},12,0,0,0,-1,FALSE}};

    /* Tai Le */
    static const WCHAR test25[] = {0x1956,0x196d,0x1970,0x1956,0x196c,0x1973,0x1951,0x1968,0x1952,0x1970};
    static const itemTest t251[2] = {{{0,0,0,0,0},0,0,0,0,tale_tag,TRUE,{-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};
    static const itemTest t252[2] = {{{0,0,0,0,0},0,0,0,2,tale_tag,TRUE,{-1,1,1,1,latn_tag}},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};

    /* New Tai Lue */
    static const WCHAR test26[] = {0x1992,0x19c4};
    static const itemTest t261[2] = {{{0,0,0,0,0},0,0,0,0,talu_tag,TRUE,{-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0},2,0,0,0,-1,FALSE}};
    static const itemTest t262[2] = {{{0,0,0,0,0},0,0,0,2,talu_tag,TRUE,{-1,1,1,1,latn_tag}},{{0,0,0,0,0},2,0,0,0,-1,FALSE}};

    /* Khmer */
    static const WCHAR test27[] = {0x1781,0x17c1,0x1798,0x179a,0x1797,0x17b6,0x179f,0x17b6};
    static const itemTest t271[2] = {{{0,0,0,0,0},0,0,0,0,khmr_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};
    static const itemTest t272[2] = {{{0,0,0,0,0},0,0,0,2,khmr_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};

    /* CJK Han */
    static const WCHAR test28[] = {0x8bed,0x7d20,0x6587,0x5b57};
    static const itemTest t281[2] = {{{0,0,0,0,0},0,0,0,0,hani_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};
    static const itemTest t282[2] = {{{0,0,0,0,0},0,0,0,2,hani_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    /* Ideographic */
    static const WCHAR test29[] = {0x2ff0,0x2ff3,0x2ffb,0x2ff0,0x65e5,0x65e5,0x5de5,0x7f51,0x4e02,0x4e5e};
    static const itemTest t291[3] = {{{0,0,0,0,0},0,0,0,0,hani_tag,FALSE},{{0,0,0,0,0},4,0,0,0,hani_tag,FALSE},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};
    static const itemTest t292[3] = {{{0,0,0,0,0},0,1,1,1,hani_tag,FALSE},{{0,0,0,0,0},4,0,0,2,hani_tag,FALSE},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};

    /* Bopomofo */
    static const WCHAR test30[] = {0x3113,0x3128,0x3127,0x3123,0x3108,0x3128,0x310f,0x3120};
    static const itemTest t301[2] = {{{0,0,0,0,0},0,0,0,0,bopo_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};
    static const itemTest t302[2] = {{{0,0,0,0,0},0,0,0,2,bopo_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};

    /* Kana */
    static const WCHAR test31[] = {0x3072,0x3089,0x304b,0x306a,0x30ab,0x30bf,0x30ab,0x30ca};
    static const itemTest t311[2] = {{{0,0,0,0,0},0,0,0,0,kana_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};
    static const itemTest t312[2] = {{{0,0,0,0,0},0,0,0,2,kana_tag,FALSE},{{0,0,0,0,0},8,0,0,0,-1,FALSE}};
    static const int b311[2] = {2,2};
    static const int b312[2] = {2,2};

    /* Hangul */
    static const WCHAR test32[] = {0xd55c,0xad6d,0xc5b4};
    static const itemTest t321[2] = {{{0,0,0,0,0},0,0,0,0,hang_tag,FALSE},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};
    static const itemTest t322[2] = {{{0,0,0,0,0},0,0,0,2,hang_tag,FALSE},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};

    /* Yi */
    static const WCHAR test33[] = {0xa188,0xa320,0xa071,0xa0b7};
    static const itemTest t331[2] = {{{0,0,0,0,0},0,0,0,0,yi_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};
    static const itemTest t332[2] = {{{0,0,0,0,0},0,0,0,2,yi_tag,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    /* Ethiopic */
    static const WCHAR test34[] = {0x130d,0x12d5,0x12dd};
    static const itemTest t341[2] = {{{0,0,0,0,0},0,0,0,0,ethi_tag,FALSE},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};
    static const itemTest t342[2] = {{{0,0,0,0,0},0,0,0,2,ethi_tag,FALSE},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};
    static const int b342[2] = {2,2};

    /* Mongolian */
    static const WCHAR test35[] = {0x182e,0x1823,0x1829,0x182d,0x1823,0x182f,0x0020,0x182a,0x1822,0x1834,0x1822,0x182d,0x180c};
    static const itemTest t351[2] = {{{0,0,0,0,0},0,0,0,0,mong_tag,FALSE},{{0,0,0,0,0},13,0,0,0,-1,FALSE}};
    static const itemTest t352[2] = {{{0,0,0,0,0},0,0,0,2,mong_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},13,0,0,0,-1,FALSE}};
    static const int b351[2] = {2,2};
    static const int b352[2] = {2,3};

    /* Tifinagh */
    static const WCHAR test36[] = {0x2d5c,0x2d49,0x2d3c,0x2d49,0x2d4f,0x2d30,0x2d56};
    static const itemTest t361[2] = {{{0,0,0,0,0},0,0,0,0,tfng_tag,TRUE,{-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};
    static const itemTest t362[2] = {{{0,0,0,0,0},0,0,0,2,tfng_tag,TRUE,{-1,1,1,1,latn_tag}},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};

    /* N'Ko */
    static const WCHAR test37[] = {0x07d2,0x07de,0x07cf};
    static const itemTest t371[2] = {{{0,0,0,0,0},0,1,1,1,nko_tag,TRUE,{-1,0,0,0,arab_tag}},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};
    static const itemTest t372[2] = {{{0,0,0,0,0},0,1,1,1,nko_tag,TRUE,{-1,0,0,2,arab_tag}},{{0,0,0,0,0},3,0,0,0,-1,FALSE}};

    /* Vai */
    static const WCHAR test38[] = {0xa559,0xa524};
    static const itemTest t381[2] = {{{0,0,0,0,0},0,0,0,0,vai_tag,TRUE,{-1,-1,-1,-1,latn_tag}},{{0,0,0,0,0},2,0,0,0,-1,FALSE}};
    static const itemTest t382[2] = {{{0,0,0,0,0},0,0,0,2,vai_tag,TRUE,{-1,1,1,1,latn_tag}},{{0,0,0,0,0},2,0,0,0,-1,FALSE}};

    /* Cherokee */
    static const WCHAR test39[] = {0x13e3,0x13b3,0x13a9,0x0020,0x13a6,0x13ec,0x13c2,0x13af,0x13cd,0x13d7};
    static const itemTest t391[2] = {{{0,0,0,0,0},0,0,0,0,cher_tag,FALSE},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};
    static const itemTest t392[2] = {{{0,0,0,0,0},0,0,0,2,cher_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},10,0,0,0,-1,FALSE}};

    /* Canadian Aboriginal Syllabics */
    static const WCHAR test40[] = {0x1403,0x14c4,0x1483,0x144e,0x1450,0x1466};
    static const itemTest t401[2] = {{{0,0,0,0,0},0,0,0,0,cans_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t402[2] = {{{0,0,0,0,0},0,0,0,2,cans_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Ogham */
    static const WCHAR test41[] = {0x169b,0x1691,0x168c,0x1690,0x168b,0x169c};
    static const itemTest t411[2] = {{{0,0,0,0,0},0,0,0,0,ogam_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t412[4] = {{{0,0,0,0,0},0,1,1,1,ogam_tag,FALSE},{{0,0,0,0,0},1,0,0,2,ogam_tag,FALSE},{{0,0,0,0,0},5,1,1,1,ogam_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const int b412[2] = {1,1};

    /* Runic */
    static const WCHAR test42[] = {0x16a0,0x16a1,0x16a2,0x16a3,0x16a4,0x16a5};
    static const itemTest t421[2] = {{{0,0,0,0,0},0,0,0,0,runr_tag,FALSE},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};
    static const itemTest t422[4] = {{{0,0,0,0,0},0,0,0,2,runr_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},6,0,0,0,-1,FALSE}};

    /* Braille */
    static const WCHAR test43[] = {0x280f,0x2817,0x2811,0x280d,0x280a,0x2811,0x2817};
    static const itemTest t431[2] = {{{0,0,0,0,0},0,0,0,0,brai_tag,FALSE},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};
    static const itemTest t432[4] = {{{0,0,0,0,0},0,0,0,2,brai_tag,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},7,0,0,0,-1,FALSE}};

    /* Private and Surrogates Area */
    static const WCHAR test44[] = {0xe000, 0xe001, 0xd800, 0xd801};
    static const itemTest t441[3] = {{{0,0,0,0,0},0,0,0,0,0,FALSE},{{0,0,0,0,0},2,0,0,0,0,FALSE},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};
    static const itemTest t442[4] = {{{0,0,0,0,0},0,0,0,2,0,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},2,0,0,2,0,TRUE,{-1,1,1,1,-1}},{{0,0,0,0,0},4,0,0,0,-1,FALSE}};

    /* Deseret */
    static const WCHAR test45[] = {0xd801,0xdc19,0xd801,0xdc32,0xd801,0xdc4c,0xd801,0xdc3c,0xd801,0xdc32,0xd801,0xdc4b,0xd801,0xdc2f,0xd801,0xdc4c,0xd801,0xdc3b,0xd801,0xdc32,0xd801,0xdc4a,0xd801,0xdc28};
    static const itemTest t451[2] = {{{0,0,0,0,0},0,0,0,0,dsrt_tag,TRUE,{-1,-1,-1,-1,0x0}},{{0,0,0,0,0},24,0,0,0,-1,FALSE}};
    static const itemTest t452[2] = {{{0,0,0,0,0},0,0,0,2,dsrt_tag,TRUE,{-1,1,1,1,0x0}},{{0,0,0,0,0},24,0,0,0,-1,FALSE}};

    /* Osmanya */
    static const WCHAR test46[] = {0xd801,0xdc8b,0xd801,0xdc98,0xd801,0xdc88,0xd801,0xdc91,0xd801,0xdc9b,0xd801,0xdc92,0xd801,0xdc95,0xd801,0xdc80};
    static const itemTest t461[2] = {{{0,0,0,0,0},0,0,0,0,osma_tag,TRUE,{-1,-1,-1,-1,0x0}},{{0,0,0,0,0},16,0,0,0,-1,FALSE}};
    static const itemTest t462[2] = {{{0,0,0,0,0},0,0,0,2,osma_tag,TRUE,{-1,1,1,1,0x0}},{{0,0,0,0,0},16,0,0,0,-1,FALSE}};

    /* Mathematical Alphanumeric Symbols */
    static const WCHAR test47[] = {0xd835,0xdc00,0xd835,0xdc35,0xd835,0xdc6a,0xd835,0xdc9f,0xd835,0xdcd4,0xd835,0xdd09,0xd835,0xdd3e,0xd835,0xdd73,0xd835,0xdda8,0xd835,0xdddd,0xd835,0xde12,0xd835,0xde47,0xd835,0xde7c};
    static const itemTest t471[2] = {{{0,0,0,0,0},0,0,0,0,math_tag,TRUE,{-1,-1,-1,-1,0x0}},{{0,0,0,0,0},26,0,0,0,-1,FALSE}};
    static const itemTest t472[2] = {{{0,0,0,0,0},0,0,0,2,math_tag,TRUE,{-1,1,1,1,0x0}},{{0,0,0,0,0},26,0,0,0,-1,FALSE}};

    SCRIPT_ITEM items[15];
    SCRIPT_CONTROL  Control;
    SCRIPT_STATE    State;
    HRESULT hr;
    HMODULE usp10;
    int nItems;

    usp10 = LoadLibraryA("usp10.dll");
    ok (usp10 != 0,"Unable to LoadLibrary on usp10.dll\n");
    pScriptItemizeOpenType = (void*)GetProcAddress(usp10, "ScriptItemizeOpenType");
    pScriptShapeOpenType = (void*)GetProcAddress(usp10, "ScriptShapeOpenType");
    pGetGlyphIndicesW = (void*)GetProcAddress(GetModuleHandleA("gdi32.dll"), "GetGlyphIndicesW");

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
}

static inline void _test_shape_ok(int valid, HDC hdc, LPCWSTR string,
                         DWORD cchString, SCRIPT_CONTROL *Control,
                         SCRIPT_STATE *State, DWORD item, DWORD nGlyphs,
                         const shapeTest_char* charItems,
                         const shapeTest_glyph* glyphItems)
{
    HRESULT hr;
    int x, outnItems=0, outnGlyphs=0;
    SCRIPT_ITEM outpItems[15];
    SCRIPT_CACHE sc = NULL;
    WORD *glyphs;
    WORD *logclust;
    int maxGlyphs = cchString * 1.5;
    SCRIPT_GLYPHPROP *glyphProp;
    SCRIPT_CHARPROP  *charProp;
    ULONG tags[15];

    hr = pScriptItemizeOpenType(string, cchString, 15, Control, State, outpItems, tags, &outnItems);
    if (hr == USP_E_SCRIPT_NOT_IN_FONT)
    {
        if (valid > 0)
            winetest_win_skip("Select font does not support script\n");
        else
            winetest_trace("Select font does not support script\n");
        return;
    }
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
            winetest_ok(glyphProp[x].sva.uJustification == glyphItems[x].GlyphProp.sva.uJustification, "%i: uJustification incorrect (%i)\n",x,glyphProp[x].sva.uJustification);
        else if (glyphProp[x].sva.uJustification != glyphItems[x].GlyphProp.sva.uJustification)
            winetest_trace("%i: uJustification incorrect (%i)\n",x,glyphProp[x].sva.uJustification);
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fClusterStart == glyphItems[x].GlyphProp.sva.fClusterStart, "%i: fClusterStart incorrect (%i)\n",x,glyphProp[x].sva.fClusterStart);
        else if (glyphProp[x].sva.fClusterStart != glyphItems[x].GlyphProp.sva.fClusterStart)
            winetest_trace("%i: fClusterStart incorrect (%i)\n",x,glyphProp[x].sva.fClusterStart);
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fDiacritic == glyphItems[x].GlyphProp.sva.fDiacritic, "%i: fDiacritic incorrect (%i)\n",x,glyphProp[x].sva.fDiacritic);
        else if (glyphProp[x].sva.fDiacritic != glyphItems[x].GlyphProp.sva.fDiacritic)
            winetest_trace("%i: fDiacritic incorrect (%i)\n",x,glyphProp[x].sva.fDiacritic);
        if (valid > 0)
            winetest_ok(glyphProp[x].sva.fZeroWidth == glyphItems[x].GlyphProp.sva.fZeroWidth, "%i: fZeroWidth incorrect (%i)\n",x,glyphProp[x].sva.fZeroWidth);
        else if (glyphProp[x].sva.fZeroWidth != glyphItems[x].GlyphProp.sva.fZeroWidth)
            winetest_trace("%i: fZeroWidth incorrect (%i)\n",x,glyphProp[x].sva.fZeroWidth);
    }

cleanup:
    HeapFree(GetProcessHeap(),0,logclust);
    HeapFree(GetProcessHeap(),0,charProp);
    HeapFree(GetProcessHeap(),0,glyphs);
    HeapFree(GetProcessHeap(),0,glyphProp);
    ScriptFreeCache(&sc);
}

#define test_shape_ok(a,b,c,d,e,f,g,h,i) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_shape_ok(1,a,b,c,d,e,f,g,h,i)

#define test_shape_ok_valid(v,a,b,c,d,e,f,g,h,i) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_shape_ok(v,a,b,c,d,e,f,g,h,i)

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

static int _find_font_for_range(HDC hdc, const CHAR *recommended, BYTE range, const WCHAR check, HFONT *hfont, HFONT *origFont)
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
                rc = 1;
            }
        }
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
        if (pGetGlyphIndicesW && (pGetGlyphIndicesW(hdc, &check, 1, &glyph, 0) == GDI_ERROR || glyph ==0))
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

#define find_font_for_range(a,b,c,d,e,f) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _find_font_for_range(a,b,c,d,e,f)

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
    static const WCHAR test_syriac[] = {0x0710, 0x0710, 0x0710, 0x0728, 0x0718, 0x0723,0};
    static const shapeTest_char syriac_c[] = {{5,{0,0}},{4,{0,0}},{3,{0,0}},{2,{0,0}},{1,{0,0}},{0,{0,0}}};
    static const shapeTest_glyph syriac_g[] = {
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
                            {1,{{SCRIPT_JUSTIFY_NONE,1,0,0,0,0},0}},
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
    test_shape_ok(hdc, test2, 4, &Control, &State, 1, 4, t2_c, t2_g);

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 11, test_hebrew[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_hebrew, 4, &Control, &State, 0, 4, hebrew_c, hebrew_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 13, test_arabic[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_arabic, 4, &Control, &State, 0, 3, arabic_c, arabic_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Sans Serif", 24, test_thai[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_thai, 10, &Control, &State, 0, 10, thai_c, thai_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Estrangelo Edessa", 71, test_syriac[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_syriac, 6, &Control, &State, 0, 6, syriac_c, syriac_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "MV Boli", 72, test_thaana[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_thaana, 13, &Control, &State, 0, 13, thaana_c, thaana_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft PhagsPa", 53, test_phagspa[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_phagspa, 11, &Control, &State, 0, 11, phagspa_c, phagspa_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "DokChampa", 25, test_lao[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_lao, 9, &Control, &State, 0, 9, lao_c, lao_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Microsoft Himalaya", 70, test_tibetan[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_tibetan, 17, &Control, &State, 0, 17, tibetan_c, tibetan_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Mangal", 15, test_devanagari[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_devanagari, 8, &Control, &State, 0, 8, devanagari_c, devanagari_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Vrinda", 16, test_bengali[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_bengali, 5, &Control, &State, 0, 5, bengali_c, bengali_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Raavi", 17, test_gurmukhi[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_gurmukhi, 7, &Control, &State, 0, 7, gurmukhi_c, gurmukhi_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Shruti", 18, test_gujarati[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_gujarati, 7, &Control, &State, 0, 7, gujarati_c, gujarati_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Kalinga", 19, test_oriya[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_oriya, 5, &Control, &State, 0, 4, oriya_c, oriya_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Latha", 20, test_tamil[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_tamil, 5, &Control, &State, 0, 4, tamil_c, tamil_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Gautami", 21, test_telugu[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_telugu, 6, &Control, &State, 0, 6, telugu_c, telugu_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Kartika", 23, test_malayalam[0], &hfont, &hfont_orig);
    if (hfont != NULL)
    {
        test_shape_ok_valid(test_valid, hdc, test_malayalam, 6, &Control, &State, 0, 6, malayalam_c, malayalam_g);
        SelectObject(hdc, hfont_orig);
        DeleteObject(hfont);
    }

    test_valid = find_font_for_range(hdc, "Tunga", 22, test_kannada[0], &hfont, &hfont_orig);
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
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    WORD glyphs[4], glyphs2[4], logclust[4];
    SCRIPT_VISATTR attrs[4];
    SCRIPT_ITEM items[2];
    int nb;

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
    memset(logclust,-1,sizeof(logclust));
    memset(attrs,-1,sizeof(attrs));
    hr = ScriptShape(hdc, &sc, test2, 4, 4, &items[0].a, glyphs2, logclust, attrs, &nb);
    ok(hr == S_OK, "ScriptShape should return S_OK not %08x\n", hr);
    ok(nb == 4, "Wrong number of items\n");
    ok(glyphs2[0] == 0 || broken(glyphs2[0] == 0x80), "Incorrect glyph for 0x202B\n");
    ok(glyphs2[3] == 0 || broken(glyphs2[3] == 0x80), "Incorrect glyph for 0x202C\n");
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
}

static void test_ScriptPlace(HDC hdc)
{
    static const WCHAR test1[] = {'t', 'e', 's', 't',0};
    BOOL ret;
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    WORD glyphs[4], logclust[4];
    SCRIPT_VISATTR attrs[4];
    SCRIPT_ITEM items[2];
    int nb, widths[4];
    GOFFSET offset[4];
    ABC abc[4];

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

    ret = ExtTextOutW(hdc, 1, 1, 0, NULL, glyphs, 4, widths);
    ok(ret, "ExtTextOutW should return TRUE\n");

    ScriptFreeCache(&sc);
}

static void test_ScriptItemIzeShapePlace(HDC hdc, unsigned short pwOutGlyphs[256])
{
    HRESULT         hr;
    int             iMaxProps;
    const SCRIPT_PROPERTIES **ppSp;

    int             cInChars;
    int             cMaxItems;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0}; 
    WCHAR           TestItem2[] = {'T', 'e', 's', 't', 'b', 0}; 
    WCHAR           TestItem3[] = {'T', 'e', 's', 't', 'c',' ','1','2','3',' ',' ','e','n','d',0};
    WCHAR           TestItem4[] = {'T', 'e', 's', 't', 'd',' ',0x0684,0x0694,0x06a4,' ',' ','\r','\n','e','n','d',0};
    WCHAR           TestItem5[] = {0x0684,'T','e','s','t','e',' ',0x0684,0x0694,0x06a4,' ',' ','e','n','d',0};
    WCHAR           TestItem6[] = {'T', 'e', 's', 't', 'f',' ',' ',' ','\r','\n','e','n','d',0};

    SCRIPT_CACHE    psc;
    int             cChars;
    int             cMaxGlyphs;
    unsigned short  pwOutGlyphs1[256];
    unsigned short  pwOutGlyphs2[256];
    unsigned short  pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    int             cnt;

    /* Start testing usp10 functions                                                         */
    /* This test determines that the pointer returned by ScriptGetProperties is valid
     * by checking a known value in the table                                                */
    hr = ScriptGetProperties(&ppSp, &iMaxProps);
    ok(hr == S_OK, "ScriptGetProperties failed: 0x%08x\n", hr);
    trace("number of script properties %d\n", iMaxProps);
    ok (iMaxProps > 0, "Number of scripts returned should not be 0\n");
    if  (iMaxProps > 0)
         ok( ppSp[0]->langid == 0, "Langid[0] not = to 0\n"); /* Check a known value to ensure   */
                                                              /* ptrs work                       */

    /* This is a valid test that will cause parsing to take place                             */
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
     *  returned.                                                                             */
    ok (pcItems > 0, "The number of SCRIPT_ITEMS should be greater than 0\n");
    if (pcItems > 0)
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
            pItem[0].iCharPos, cInChars, pItem[1].iCharPos);

    /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue
     * ie. ScriptItemize has succeeded and that pItem has been set                            */
    cInChars = 5;
    if (hr == S_OK) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = cInChars;
        hr = ScriptShape(NULL, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == E_PENDING, "If psc is NULL (%08x) the E_PENDING should be returned\n", hr);
        cMaxGlyphs = 4;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == E_OUTOFMEMORY, "If not enough output area cChars (%d) is > than CMaxGlyphs "
                                 "(%d) but not E_OUTOFMEMORY\n",
                                 cChars, cMaxGlyphs);
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == S_OK, "ScriptShape should return S_OK not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == S_OK, "ScriptPlace should return S_OK not (%08x)\n", hr);
            hr = ScriptPlace(NULL, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == S_OK, "ScriptPlace should return S_OK not (%08x)\n", hr);
            for (cnt=0; cnt < pcGlyphs; cnt++)
                pwOutGlyphs[cnt] = pwOutGlyphs1[cnt];                 /* Send to next function */
        }

        /* This test will check to make sure that SCRIPT_CACHE is reused and that not translation   *
         * takes place if fNoGlyphIndex is set.                                                     */

        cInChars = 5;
        cMaxItems = 255;
        hr = ScriptItemize(TestItem2, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
        ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
        /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is   *
         *  returned.                                                                               */
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
                            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
                             pItem[0].iCharPos, cInChars, pItem[1].iCharPos);
        /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue                    */
        if (hr == S_OK) {
             cChars = cInChars;
             cMaxGlyphs = 256;
             pItem[0].a.fNoGlyphIndex = 1;                /* say no translate                     */
             hr = ScriptShape(NULL, &psc, TestItem2, cChars,
                              cMaxGlyphs, &pItem[0].a,
                              pwOutGlyphs2, pwLogClust, psva, &pcGlyphs);
             ok (hr != E_PENDING, "If psc should not be NULL (%08x) and the E_PENDING should be returned\n", hr);
             ok (hr == S_OK, "ScriptShape should return S_OK not (%08x)\n", hr);
             ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
             ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
             for (cnt=0; cnt < cChars && TestItem2[cnt] == pwOutGlyphs2[cnt]; cnt++) {}
             ok (cnt == cChars, "Translation to place when told not to. WCHAR %d - %04x != %04x\n",
                           cnt, TestItem2[cnt], pwOutGlyphs2[cnt]);
             if (hr == S_OK) {
                 hr = ScriptPlace(hdc, &psc, pwOutGlyphs2, pcGlyphs, psva, &pItem[0].a, piAdvance,
                                  pGoffset, pABC);
                 ok (hr == S_OK, "ScriptPlace should return S_OK not (%08x)\n", hr);
             }
        }
        ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");

    }

    /* This is a valid test that will cause parsing to take place and create 3 script_items   */
    cInChars = (sizeof(TestItem3)/2)-1;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem3, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    if  (hr == S_OK)
	{
        ok (pcItems == 3, "The number of SCRIPT_ITEMS should be 3 not %d\n", pcItems);
        if (pcItems > 2)
        {
            ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == 6,
                "Start pos [0] not = 0 (%d) or end pos [1] not = %d\n",
                pItem[0].iCharPos, pItem[1].iCharPos);
            ok (pItem[1].iCharPos == 6 && pItem[2].iCharPos == 11,
                "Start pos [1] not = 6 (%d) or end pos [2] not = 11 (%d)\n",
                pItem[1].iCharPos, pItem[2].iCharPos);
            ok (pItem[2].iCharPos == 11 && pItem[3].iCharPos == cInChars,
                "Start pos [2] not = 11 (%d) or end [3] pos not = 14 (%d), cInChars = %d\n",
                pItem[2].iCharPos, pItem[3].iCharPos, cInChars);
        }
    }

    /* This is a valid test that will cause parsing to take place and create 5 script_items   */
    cInChars = (sizeof(TestItem4)/2)-1;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem4, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    if  (hr == S_OK)
	{
        ok (pcItems == 5, "The number of SCRIPT_ITEMS should be 5 not %d\n", pcItems);
        if (pcItems > 4)
        {
            ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == 6,
                "Start pos [0] not = 0 (%d) or end pos [1] not = %d\n",
                pItem[0].iCharPos, pItem[1].iCharPos);
            ok (pItem[0].a.s.uBidiLevel == 0, "Should have been bidi=0 not %d\n",
                                               pItem[0].a.s.uBidiLevel);
            ok (pItem[1].iCharPos == 6 && pItem[2].iCharPos == 11,
                "Start pos [1] not = 6 (%d) or end pos [2] not = 11 (%d)\n",
                pItem[1].iCharPos, pItem[2].iCharPos);
            ok (pItem[1].a.s.uBidiLevel == 1, "Should have been bidi=1 not %d\n",
                                              pItem[1].a.s.uBidiLevel);
            ok (pItem[2].iCharPos == 11 && pItem[3].iCharPos == 12,
                "Start pos [2] not = 11 (%d) or end [3] pos not = 12 (%d)\n",
                pItem[2].iCharPos, pItem[3].iCharPos);
            ok (pItem[2].a.s.uBidiLevel == 0, "Should have been bidi=0 not %d\n",
                                               pItem[2].a.s.uBidiLevel);
            ok (pItem[3].iCharPos == 12 && pItem[4].iCharPos == 13,
                "Start pos [3] not = 12 (%d) or end [4] pos not = 13 (%d)\n",
                pItem[3].iCharPos, pItem[4].iCharPos);
            ok (pItem[3].a.s.uBidiLevel == 0, "Should have been bidi=0 not %d\n",
                                               pItem[3].a.s.uBidiLevel);
            ok (pItem[4].iCharPos == 13 && pItem[5].iCharPos == cInChars,
                "Start pos [4] not = 13 (%d) or end [5] pos not = 16 (%d), cInChars = %d\n",
                pItem[4].iCharPos, pItem[5].iCharPos, cInChars);
        }
    }

    /*
     * This test is for when the first unicode character requires bidi support
     */
    cInChars = (sizeof(TestItem5)-1)/sizeof(WCHAR);
    hr = ScriptItemize(TestItem5, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    ok (pcItems == 4, "There should have been 4 items, found %d\n", pcItems);
    ok (pItem[0].a.s.uBidiLevel == 1, "The first character should have been bidi=1 not %d\n",
                                       pItem[0].a.s.uBidiLevel);

    /* This test checks to make sure that the test to see if there are sufficient buffers to store  *
     * the pointer to the last char works.  Note that windows often needs a greater number of       *
     * SCRIPT_ITEMS to process a string than is returned in pcItems.                                */
    cInChars = (sizeof(TestItem6)/2)-1;
    cMaxItems = 4;
    hr = ScriptItemize(TestItem6, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_OUTOFMEMORY, "ScriptItemize should return E_OUTOFMEMORY, returned %08x\n", hr);

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
    ok( psc != NULL, "ScritpGetCMap expected psc to be not NULL\n");
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

    cInChars = cChars = 4;
    hr = ScriptGetCMap(hdc, &psc, TestItem2, cInChars, dwFlags, pwOutGlyphs3);
    ok (hr == S_FALSE, "ScriptGetCMap should return S_FALSE not (%08x)\n", hr);
    ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
    ok(pwOutGlyphs3[0] == 0 || broken(pwOutGlyphs3[0] == 0x80), "Glyph 0 should be default glyph\n");
    ok(pwOutGlyphs3[3] == 0 || broken(pwOutGlyphs3[0] == 0x80), "Glyph 0 should be default glyph\n");


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
    BOOL is_terminal, is_arial, is_times_new_roman, is_arabic = (system_lang_id == LANG_ARABIC);

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

    pGetGlyphIndicesW = (void*)GetProcAddress(GetModuleHandleA("gdi32.dll"), "GetGlyphIndicesW");
    if (!pGetGlyphIndicesW)
    {
        win_skip("Skip on WINNT4\n");
        return;
    }
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    efnd.total = 0;
    EnumFontFamiliesA(hdc, NULL, enum_bitmap_font_proc, (LPARAM)&efnd);

    for (i = 0; i < efnd.total; i++)
    {
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

        is_terminal = !(lstrcmpA(lf.lfFaceName, "Terminal") && lstrcmpA(lf.lfFaceName, "@Terminal"));
        ok(sfp.wgBlank == tmA.tmBreakChar || broken(is_terminal) || broken(is_arabic), "bitmap font %s wgBlank %04x tmBreakChar %04x\n", lf.lfFaceName, sfp.wgBlank, tmA.tmBreakChar);

        ok(sfp.wgDefault == tmA.tmDefaultChar || broken(is_arabic), "bitmap font %s wgDefault %04x, tmDefaultChar %04x\n", lf.lfFaceName, sfp.wgDefault, tmA.tmDefaultChar);

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
        ret = pGetGlyphIndicesW(hdc, str, 1, gi, 0);
        ok(ret != GDI_ERROR, "GetGlyphIndicesW failed!\n");
        ok(sfp.wgBlank == gi[0], "truetype font %s wgBlank %04x gi[0] %04x\n", lf.lfFaceName, sfp.wgBlank, gi[0]);

        ok(sfp.wgDefault == 0 || broken(is_arabic), "truetype font %s wgDefault %04x\n", lf.lfFaceName, sfp.wgDefault);

        ret = pGetGlyphIndicesW(hdc, invalids, 3, gi, GGI_MARK_NONEXISTING_GLYPHS);
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
        ret = pGetGlyphIndicesW(hdc, str, 1, gi, GGI_MARK_NONEXISTING_GLYPHS);
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
    int             cMaxItems;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0}; 

    SCRIPT_CACHE    psc;
    int             cChars;
    int             cMaxGlyphs;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    RECT            rect;
    int             piX;
    int             iCP = 1;
    BOOL            fTrailing = FALSE;
    SCRIPT_LOGATTR  *psla;
    SCRIPT_LOGATTR  sla[256];

    /* This is a valid test that will cause parsing to take place                             */
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
     *  returned.                                                                             */
    ok (pcItems > 0, "The number of SCRIPT_ITEMS should be greater than 0\n");
    if (pcItems > 0)
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
            pItem[0].iCharPos, cInChars, pItem[1].iCharPos);

    /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue
     * ie. ScriptItemize has succeeded and that pItem has been set                            */
    cInChars = 5;
    if (hr == S_OK) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == S_OK, "ScriptShape should return S_OK not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr == S_OK) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == S_OK, "Should return S_OK not (%08x)\n", hr);
            ScriptFreeCache(&psc);              /* Get rid of psc for next test set */
            ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

            hr = ScriptTextOut(NULL, NULL, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL);
            ok (hr == E_INVALIDARG, "Should return 0 not (%08x)\n", hr);

            hr = ScriptTextOut(NULL, NULL, 0, 0, 0, NULL, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok( hr == E_INVALIDARG, "(NULL,NULL,TestItem1, cInChars, dwFlags, pwOutGlyphs3), "
                                    "expected E_INVALIDARG, got %08x\n", hr);

            /* Set psc to NULL, to be able to check if a pointer is returned in psc */
            psc = NULL;
            hr = ScriptTextOut(NULL, &psc, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0,
                               NULL, NULL, NULL);
            ok( hr == E_INVALIDARG, "(NULL,&psc,NULL,0,0,0,NULL,), expected E_INVALIDARG, "
                                    "got %08x\n", hr);
            ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

            /* hdc is required for this one rather than the usual optional          */
            psc = NULL;
            hr = ScriptTextOut(NULL, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok( hr == E_INVALIDARG, "(NULL,&psc,), expected E_INVALIDARG, got %08x\n", hr);
            ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

            /* Set that it returns 0 status */
            hr = ScriptTextOut(hdc, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == S_OK, "ScriptTextOut should return S_OK not (%08x)\n", hr);

            /* Test Rect Rgn is acceptable */
            rect.top = 10;
            rect.bottom = 20;
            rect.left = 10;
            rect.right = 40;
            hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == S_OK, "ScriptTextOut should return S_OK not (%08x)\n", hr);

            iCP = 1;
            hr = ScriptCPtoX(iCP, fTrailing, cChars, pcGlyphs, (const WORD *) &pwLogClust,
                            (const SCRIPT_VISATTR *) &psva, (const int *)&piAdvance, &pItem[0].a, &piX);
            ok(hr == S_OK, "ScriptCPtoX Stub should return S_OK not %08x\n", hr);

            psla = (SCRIPT_LOGATTR *)&sla;
            hr = ScriptBreak(TestItem1, cChars, &pItem[0].a, psla);
            ok(hr == S_OK, "ScriptBreak Stub should return S_OK not %08x\n", hr);

            /* Clean up and go   */
            ScriptFreeCache(&psc);
            ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);
        }
    }
}

static void test_ScriptTextOut2(HDC hdc)
{
/*  Intent is to validate that the HDC passed into ScriptTextOut is
 *  used instead of the (possibly) invalid cached one
 */
    HRESULT         hr;

    HDC             hdc1, hdc2;
    int             cInChars;
    int             cMaxItems;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0};

    SCRIPT_CACHE    psc;
    int             cChars;
    int             cMaxGlyphs;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];

    /* Create an extra DC that will be used until the ScriptTextOut */
    hdc1 = CreateCompatibleDC(hdc);
    ok (hdc1 != 0, "CreateCompatibleDC failed to create a DC\n");
    hdc2 = CreateCompatibleDC(hdc);
    ok (hdc2 != 0, "CreateCompatibleDC failed to create a DC\n");

    /* This is a valid test that will cause parsing to take place                             */
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
     *  returned.                                                                             */
    ok (pcItems > 0, "The number of SCRIPT_ITEMS should be greater than 0\n");
    if (pcItems > 0)
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
            pItem[0].iCharPos, cInChars, pItem[1].iCharPos);

    /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue
     * ie. ScriptItemize has succeeded and that pItem has been set                            */
    cInChars = 5;
    if (hr == S_OK) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc2, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == S_OK, "ScriptShape should return S_OK not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc2, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == S_OK, "Should return S_OK not (%08x)\n", hr);

            /*   key part!!!   cached dc is being deleted  */
            hr = DeleteDC(hdc2);
            ok(hr == 1, "DeleteDC should return 1 not %08x\n", hr);

            /* At this point the cached hdc (hdc2) has been destroyed,
             * however, we are passing in a *real* hdc (the original hdc).
             * The text should be written to that DC
             */
            hr = ScriptTextOut(hdc1, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == S_OK, "ScriptTextOut should return S_OK not (%08x)\n", hr);
            ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");

            DeleteDC(hdc1);

            /* Clean up and go   */
            ScriptFreeCache(&psc);
            ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);
        }
    }
}

static void test_ScriptTextOut3(HDC hdc)
{
    HRESULT         hr;

    int             cInChars;
    int             cMaxItems;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[] = {' ','\r', 0};

    SCRIPT_CACHE    psc;
    int             cChars;
    int             cMaxGlyphs;
    unsigned short  pwOutGlyphs1[256];
    WORD            pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    RECT            rect;

    /* This is to ensure that nonexistent glyphs are translated into a valid glyph number */
    cInChars = 2;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == S_OK, "ScriptItemize should return S_OK, returned %08x\n", hr);
    /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
     *  returned.                                                                             */
    ok (pcItems > 0, "The number of SCRIPT_ITEMS should be greater than 0\n");
    if (pcItems > 0)
        ok (pItem[0].iCharPos == 0 && pItem[2].iCharPos == cInChars,
            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
            pItem[0].iCharPos, cInChars, pItem[2].iCharPos);

    /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue
     * ie. ScriptItemize has succeeded and that pItem has been set                            */
    cInChars = 2;
    if (hr == S_OK) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == S_OK, "ScriptShape should return S_OK not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == S_OK, "Should return S_OK not (%08x)\n", hr);

            /* Test Rect Rgn is acceptable */
            rect.top = 10;
            rect.bottom = 20;
            rect.left = 10;
            rect.right = 40;
            hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == S_OK, "ScriptTextOut should return S_OK not (%08x)\n", hr);
        }
        /* Clean up and go   */
        ScriptFreeCache(&psc);
        ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);
    }
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

static void test_ScriptXtoX(void)
/****************************************************************************************
 *  This routine tests the ScriptXtoCP and ScriptCPtoX functions using static variables *
 ****************************************************************************************/
{
    WORD pwLogClust[10] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 3};
    WORD pwLogClust_RTL[10] = {3, 3, 3, 2, 2, 1, 1, 0, 0, 0};
    WORD pwLogClust_2[7] = {4, 3, 3, 2, 1, 0 ,0};
    int piAdvance[10] = {201, 190, 210, 180, 170, 204, 189, 195, 212, 203};
    int piAdvance_2[5] = {39, 26, 19, 17, 11};
    static const int offsets[13] = {0, 67, 134, 201, 296, 391, 496, 601, 1052, 1503, 1954, 1954, 1954};
    static const int offsets_RTL[13] = {781, 721, 661, 601, 496, 391, 296, 201, 134, 67, 0, 0, 0};
    static const int offsets_2[10] = {112, 101, 92, 84, 65, 39, 19, 0, 0, 0};
    SCRIPT_VISATTR psva[10];
    SCRIPT_ANALYSIS sa;
    int iX;
    int piCP;
    int piTrailing;
    HRESULT hr;

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
        ok(piCP==0 && piTrailing==0,"%i should return 0(%i) and 0(%i)\n",iX, piCP,piTrailing);
    }
    for (iX = 8; iX < 16; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
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
        ok(piCP==0 && piTrailing==1,"%i should return 0(%i) and 1(%i)\n",iX,piCP,piTrailing);
    }
    for (iX = 9; iX < 16; iX++)
    {
        WORD clust = 0;
        INT advance = 16;
        hr = ScriptXtoCP(iX, 1, 1, &clust, psva, &advance, &sa, &piCP, &piTrailing);
        ok(piCP==0 && piTrailing==0,"%i should return 0(%i) and 0(%i)\n",iX,piCP,piTrailing);
    }

    sa.fRTL = FALSE;
    test_item_ScriptXtoX(&sa, 10, 10, offsets, pwLogClust, piAdvance);
    sa.fRTL = TRUE;
    test_item_ScriptXtoX(&sa, 10, 10, offsets_RTL, pwLogClust_RTL, piAdvance);
    test_item_ScriptXtoX(&sa, 7, 5, offsets_2, pwLogClust_2, piAdvance_2);
}

static void test_ScriptString(HDC hdc)
{
/*******************************************************************************************
 *
 * This set of tests are for the string functions of uniscribe.  The ScriptStringAnalyse
 * function allocates memory pointed to by the SCRIPT_STRING_ANALYSIS ssa pointer.  This
 * memory is freed by ScriptStringFree.  There needs to be a valid hdc for this as
 * ScriptStringAnalyse calls ScriptSItemize, ScriptShape and ScriptPlace which require it.
 *
 */

    HRESULT         hr;
    WCHAR           teststr[] = {'T','e','s','t','1',' ','a','2','b','3', '\0'};
    int             len = (sizeof(teststr) / sizeof(WCHAR)) - 1;
    int             Glyphs = len * 2 + 16;
    int             Charset;
    DWORD           Flags = SSA_GLYPHS;
    int             ReqWidth = 100;
    const int       Dx[5] = {10, 10, 10, 10, 10};
    const BYTE      InClass = 0;
    SCRIPT_STRING_ANALYSIS ssa = NULL;

    int             X = 10; 
    int             Y = 100;
    UINT            Options = 0; 
    const RECT      rc = {0, 50, 100, 100}; 
    int             MinSel = 0;
    int             MaxSel = 0;
    BOOL            Disabled = FALSE;
    const int      *clip_len;
    int            i;
    UINT           *order;


    Charset = -1;     /* this flag indicates unicode input */
    /* Test without hdc to get E_PENDING */
    hr = ScriptStringAnalyse( NULL, teststr, len, Glyphs, Charset, Flags,
                              ReqWidth, NULL, NULL, Dx, NULL,
                              &InClass, &ssa);
    ok(hr == E_PENDING, "ScriptStringAnalyse Stub should return E_PENDING not %08x\n", hr);

    /* test with hdc, this should be a valid test  */
    hr = ScriptStringAnalyse( hdc, teststr, len, Glyphs, Charset, Flags,
                              ReqWidth, NULL, NULL, Dx, NULL,
                              &InClass, &ssa);
    ok(hr == S_OK, "ScriptStringAnalyse should return S_OK not %08x\n", hr);
    ScriptStringFree(&ssa);

    /* test makes sure that a call with a valid pssa still works */
    hr = ScriptStringAnalyse( hdc, teststr, len, Glyphs, Charset, Flags,
                              ReqWidth, NULL, NULL, Dx, NULL,
                              &InClass, &ssa);
    ok(hr == S_OK, "ScriptStringAnalyse should return S_OK not %08x\n", hr);
    ok(ssa != NULL, "ScriptStringAnalyse pssa should not be NULL\n");

    if (hr == S_OK)
    {
        hr = ScriptStringOut(ssa, X, Y, Options, &rc, MinSel, MaxSel, Disabled);
        ok(hr == S_OK, "ScriptStringOut should return S_OK not %08x\n", hr);
    }

     clip_len = ScriptString_pcOutChars(ssa);
     ok(*clip_len == len, "ScriptString_pcOutChars failed, got %d, expected %d\n", *clip_len, len);

     order = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *clip_len * sizeof(UINT));
     hr = ScriptStringGetOrder(ssa, order);
     ok(hr == S_OK, "ScriptStringGetOrder failed, got %08x, expected S_OK\n", hr);

     for (i = 0; i < *clip_len; i++) ok(order[i] == i, "%d: got %d expected %d\n", i, order[i], i);
     HeapFree(GetProcessHeap(), 0, order);

     hr = ScriptStringFree(&ssa);
     ok(hr == S_OK, "ScriptStringFree should return S_OK not %08x\n", hr);
}

static void test_ScriptStringXtoCP_CPtoX(HDC hdc)
{
/*****************************************************************************************
 *
 * This test is for the ScriptStringXtoCP and ScriptStringXtoCP functions.  Due to the
 * nature of the fonts between Windows and Wine, the test is implemented by generating
 * values using one one function then checking the output of the second.  In this way
 * the validity of the functions is established using Windows as a base and confirming
 * similar behaviour in wine.
 */

    HRESULT         hr;
    static const WCHAR teststr1[]  = {0x05e9, 'i', 0x05dc, 'n', 0x05d5, 'e', 0x05dd, '.',0};
    static const BOOL rtl[] = {1, 0, 1, 0, 1, 0, 1, 0};
    void            *String = (WCHAR *) &teststr1;      /* ScriptStringAnalysis needs void */
    int             String_len = (sizeof(teststr1)/sizeof(WCHAR))-1;
    int             Glyphs = String_len * 2 + 16;       /* size of buffer as recommended  */
    int             Charset = -1;                       /* unicode                        */
    DWORD           Flags = SSA_GLYPHS;
    int             ReqWidth = 100;
    const BYTE      InClass = 0;
    SCRIPT_STRING_ANALYSIS ssa = NULL;

    int             Ch;                                  /* Character position in string */
    int             iTrailing;
    int             Cp;                                  /* Character position in string */
    int             X;
    int             trail,lead;
    BOOL            fTrailing;

    /* Test with hdc, this should be a valid test
     * Here we generate an SCRIPT_STRING_ANALYSIS that will be used as input to the
     * following character positions to X and X to character position functions.
     */

    hr = ScriptStringAnalyse( hdc, String, String_len, Glyphs, Charset, Flags,
                              ReqWidth, NULL, NULL, NULL, NULL,
                              &InClass, &ssa);
    ok(hr == S_OK ||
       hr == E_INVALIDARG, /* NT */
       "ScriptStringAnalyse should return S_OK or E_INVALIDARG not %08x\n", hr);

    if  (hr == S_OK)
    {
        ok(ssa != NULL, "ScriptStringAnalyse ssa should not be NULL\n");

        /*
         * Loop to generate character positions to provide starting positions for the
         * ScriptStringCPtoX and ScriptStringXtoCP functions
         */
        for (Cp = 0; Cp < String_len; Cp++)
        {
            /* The fTrailing flag is used to indicate whether the X being returned is at
             * the beginning or the end of the character. What happens here is that if
             * fTrailing indicates the end of the character, ie. FALSE, then ScriptStringXtoCP
             * returns the beginning of the next character and iTrailing is FALSE.  So for this
             * loop iTrailing will be FALSE in both cases.
             */
            hr = ScriptStringCPtoX(ssa, Cp, TRUE, &trail);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            hr = ScriptStringCPtoX(ssa, Cp, FALSE, &lead);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            if (rtl[Cp])
                ok(lead > trail, "Leading values should be after trailing for rtl characters(%i)\n",Cp);
            else
                ok(lead < trail, "Trailing values should be after leading for ltr characters(%i)\n",Cp);

            /* move by 1 pixel so that we are not between 2 characters.  That could result in being the lead of a rtl and
               at the same time the trail of an ltr */

            /* inside the leading edge */
            X = lead;
            if (rtl[Cp]) X--; else X++;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
            ok(Cp == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp, Ch, trail);
            ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n",
                                  iTrailing, X);

            /* inside the trailing edge */
            X = trail;
            if (rtl[Cp]) X++; else X--;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
            ok(Cp == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp, Ch, trail);
            ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = %d\n",
                                  iTrailing, X);

            /* outside the "trailing" edge */
            if (Cp < String_len-1)
            {
                if (rtl[Cp]) X = lead; else X = trail;
                X++;
                hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
                ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
                ok(Cp + 1 == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp + 1, Ch, trail);
                if (rtl[Cp+1])
                    ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = %d\n",
                                          iTrailing, X);
                else
                    ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n",
                                          iTrailing, X);
            }

            /* outside the "leading" edge */
            if (Cp != 0)
            {
                if (rtl[Cp]) X = trail; else X = lead;
                X--;
                hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
                ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
                ok(Cp - 1 == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp - 1, Ch, trail);
                if (Cp != 0  && rtl[Cp-1])
                    ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n",
                                          iTrailing, X);
                else
                    ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = %d\n",
                                          iTrailing, X);
            }
        }

        /* Check beyond the leading boundary of the whole string */
        if (rtl[0])
        {
            /* having a leading rtl character seems to confuse usp */
            /* this looks to be a windows bug we should emulate */
            hr = ScriptStringCPtoX(ssa, 0, TRUE, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            X--;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
            ok(Ch == 1, "ScriptStringXtoCP should return Ch = 1 not %d for X outside leading edge when rtl\n", Ch);
            ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = outside leading edge when rtl\n",
                                       iTrailing);
        }
        else
        {
            hr = ScriptStringCPtoX(ssa, 0, FALSE, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            X--;
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
            ok(Ch == -1, "ScriptStringXtoCP should return Ch = -1 not %d for X outside leading edge\n", Ch);
            ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = outside leading edge\n",
                                       iTrailing);
        }

        /* Check beyond the end boundary of the whole string */
        if (rtl[String_len-1])
        {
            hr = ScriptStringCPtoX(ssa, String_len-1, FALSE, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
        }
        else
        {
            hr = ScriptStringCPtoX(ssa, String_len-1, TRUE, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
        }
        X++;
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
        ok(Ch == String_len, "ScriptStringXtoCP should return Ch = %i not %d for X outside trailing edge\n", String_len, Ch);
        ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = outside trailing edge\n",
                                   iTrailing);

        /*
         * Cleanup the SSA for the next round of tests
         */
        hr = ScriptStringFree(&ssa);
        ok(hr == S_OK, "ScriptStringFree should return S_OK not %08x\n", hr);

        /*
         * Test to see that exceeding the number of chars returns E_INVALIDARG.  First
         * generate an SSA for the subsequent tests.
         */
        hr = ScriptStringAnalyse( hdc, String, String_len, Glyphs, Charset, Flags,
                                  ReqWidth, NULL, NULL, NULL, NULL,
                                  &InClass, &ssa);
        ok(hr == S_OK, "ScriptStringAnalyse should return S_OK not %08x\n", hr);

        /*
         * When ScriptStringCPtoX is called with a character position Cp that exceeds the
         * string length, return E_INVALIDARG.  This also invalidates the ssa so a
         * ScriptStringFree should also fail.
         */
        fTrailing = FALSE;
        Cp = String_len + 1;
        hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
        ok(hr == E_INVALIDARG, "ScriptStringCPtoX should return E_INVALIDARG not %08x\n", hr);

        ScriptStringFree(&ssa);
    }
}

static void test_ScriptCacheGetHeight(HDC hdc)
{
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    LONG height;

    hr = ScriptCacheGetHeight(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptCacheGetHeight(NULL, &sc, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptCacheGetHeight(NULL, &sc, &height);
    ok(hr == E_PENDING, "expected E_PENDING, got 0x%08x\n", hr);

    height = 0;

    hr = ScriptCacheGetHeight(hdc, &sc, &height);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(height > 0, "expected height > 0\n");

    ScriptFreeCache(&sc);
}

static void test_ScriptGetGlyphABCWidth(HDC hdc)
{
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    ABC abc;

    hr = ScriptGetGlyphABCWidth(NULL, NULL, 'a', NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptGetGlyphABCWidth(NULL, &sc, 'a', NULL);
    ok(broken(hr == E_PENDING) ||
       hr == E_INVALIDARG, /* WIN7 */
       "expected E_INVALIDARG, got 0x%08x\n", hr);

    hr = ScriptGetGlyphABCWidth(NULL, &sc, 'a', &abc);
    ok(hr == E_PENDING, "expected E_PENDING, got 0x%08x\n", hr);

    if (0) {    /* crashes on WinXP */
    hr = ScriptGetGlyphABCWidth(hdc, &sc, 'a', NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);
    }

    hr = ScriptGetGlyphABCWidth(hdc, &sc, 'a', &abc);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);

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

    for (i = 0; i < sizeof(levels)/sizeof(levels[0]); i++)
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
    HMODULE hKernel32;
    static BOOL (WINAPI * pEnumLanguageGroupLocalesA)(LANGGROUPLOCALE_ENUMPROCA,LGRPID,DWORD,LONG_PTR);

    hKernel32 = GetModuleHandleA("kernel32.dll");
    pEnumLanguageGroupLocalesA = (void*)GetProcAddress(hKernel32, "EnumLanguageGroupLocalesA");

    if (!pEnumLanguageGroupLocalesA)
    {
        win_skip("EnumLanguageGroupLocalesA not available on this platform\n");
        return;
    }

    for (i = 0; i < sizeof(groups)/sizeof(groups[0]); i++)
    {
        ret = pEnumLanguageGroupLocalesA(enum_proc, groups[i], 0, 0);
        if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            win_skip("EnumLanguageGroupLocalesA not implemented on this platform\n");
            break;
        }
        
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
    HRESULT hr;
    pScriptGetFontScriptTags = (void*)GetProcAddress(GetModuleHandleA("usp10.dll"), "ScriptGetFontScriptTags");
    pScriptGetFontLanguageTags = (void*)GetProcAddress(GetModuleHandleA("usp10.dll"), "ScriptGetFontLanguageTags");
    pScriptGetFontFeatureTags = (void*)GetProcAddress(GetModuleHandleA("usp10.dll"), "ScriptGetFontFeatureTags");
    if (!pScriptGetFontScriptTags || !pScriptGetFontLanguageTags || !pScriptGetFontFeatureTags)
    {
        win_skip("ScriptGetFontScriptTags,ScriptGetFontLanguageTags or ScriptGetFontFeatureTags not available on this platform\n");
    }
    else
    {
        SCRIPT_CACHE sc = NULL;
        OPENTYPE_TAG tags[5];
        int count = 0;
        int outnItems=0;
        SCRIPT_ITEM outpItems[15];
        SCRIPT_CONTROL Control;
        SCRIPT_STATE State;
        static const WCHAR test_phagspa[] = {0xa84f, 0xa861, 0xa843, 0x0020, 0xa863, 0xa861, 0xa859, 0x0020, 0xa850, 0xa85c, 0xa85e};

        hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, NULL, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, NULL, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 5, tags, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 0, tags, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontScriptTags(NULL, &sc, NULL, 5, tags, &count);
        ok(hr == E_PENDING,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontScriptTags(hdc, &sc, NULL, 5, tags, &count);
        ok((hr == S_OK || hr == E_OUTOFMEMORY),"Incorrect return code\n");
        if (hr == S_OK)
            ok(count <= 5, "Count should be less or equal to 5 with S_OK return\n");
        else if (hr == E_OUTOFMEMORY)
            ok(count == 0, "Count should be 0 with E_OUTOFMEMORY return\n");
        ok(sc != NULL, "ScriptCache should be initialized\n");

        ScriptFreeCache(&sc);
        sc = NULL;

        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, NULL, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, NULL, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 5, tags, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 0, tags, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontLanguageTags(NULL, &sc, NULL, latn_tag, 5, tags, &count);
        ok(hr == E_PENDING,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, latn_tag, 5, tags, &count);
        ok((hr == S_OK || hr == E_OUTOFMEMORY),"Incorrect return code\n");
        if (hr == S_OK)
            ok(count <= 5, "Count should be less or equal to 5 with S_OK return\n");
        else if (hr == E_OUTOFMEMORY)
            ok(count == 0, "Count should be 0 with E_OUTOFMEMORY return\n");

        ScriptFreeCache(&sc);
        sc = NULL;

        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, NULL, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, NULL, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 5, tags, NULL);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 0, tags, &count);
        ok(hr == E_INVALIDARG,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontFeatureTags(NULL, &sc, NULL, latn_tag, 0x0, 5, tags, &count);
        ok(hr == E_PENDING,"Incorrect return code\n");
        ok(sc == NULL, "ScriptCache should remain uninitialized\n");
        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, latn_tag, 0x0, 5, tags, &count);
        ok((hr == S_OK || hr == E_OUTOFMEMORY),"Incorrect return code\n");
        if (hr == S_OK)
            ok(count <= 5, "Count should be less or equal to 5 with S_OK return\n");
        else if (hr == E_OUTOFMEMORY)
            ok(count == 0, "Count should be 0 with E_OUTOFMEMORY return\n");

        memset(&Control, 0, sizeof(Control));
        memset(&State, 0, sizeof(State));

        hr = ScriptItemize(test_phagspa, 10, 15, &Control, &State, outpItems, &outnItems);
        ok(hr == S_OK, "ScriptItemize failed: 0x%08x\n", hr);
        memset(tags,0,sizeof(tags));
        hr = pScriptGetFontScriptTags(hdc, &sc, &outpItems[0].a, 5, tags, &count);
        ok( hr == USP_E_SCRIPT_NOT_IN_FONT || broken(hr == S_OK), "wrong return code\n");

        hr = pScriptGetFontLanguageTags(hdc, &sc, NULL, dsrt_tag, 5, tags, &count);
        ok( hr == S_OK, "wrong return code\n");
        hr = pScriptGetFontLanguageTags(hdc, &sc, &outpItems[0].a, dsrt_tag, 5, tags, &count);
        ok( hr == E_INVALIDARG || broken(hr == S_OK), "wrong return code\n");

        hr = pScriptGetFontFeatureTags(hdc, &sc, NULL, dsrt_tag, 0x0, 5, tags, &count);
        ok( hr == S_OK, "wrong return code\n");
        hr = pScriptGetFontFeatureTags(hdc, &sc, &outpItems[0].a, dsrt_tag, 0x0, 5, tags, &count);
        ok( hr == E_INVALIDARG || broken(hr == S_OK), "wrong return code\n");

        ScriptFreeCache(&sc);
    }
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

    test_ScriptItemize();
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

    test_ScriptLayout();
    test_digit_substitution();
    test_ScriptGetProperties();
    test_ScriptBreak();
    test_newlines();

    test_ScriptGetFontFunctions(hdc);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}
