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
    char todo_flag[4];
    int iCharPos;
    int fRTL;
    int fLayoutRTL;
    int uBidiLevel;
} itemTest;

static inline void _test_items_ok(LPCWSTR string, DWORD cchString,
                         SCRIPT_CONTROL *Control, SCRIPT_STATE *State,
                         DWORD nItems, const itemTest* items, BOOL nItemsToDo)
{
    HRESULT hr;
    int x, outnItems;
    SCRIPT_ITEM outpItems[15];

    hr = ScriptItemize(string, cchString, 15, Control, State, outpItems, &outnItems);
    winetest_ok(!hr, "ScriptItemize should return S_OK not %08x\n", hr);
    if (nItemsToDo)
        todo_wine winetest_ok(outnItems == nItems, "Wrong number of items\n");
    else
        winetest_ok(outnItems == nItems, "Wrong number of items\n");
    for (x = 0; x <= outnItems; x++)
    {
        if (items[x].todo_flag[0])
            todo_wine winetest_ok(outpItems[x].iCharPos == items[x].iCharPos, "%i:Wrong CharPos\n",x);
        else
            winetest_ok(outpItems[x].iCharPos == items[x].iCharPos, "%i:Wrong CharPos (%i)\n",x,outpItems[x].iCharPos);

        if (items[x].todo_flag[1])
            todo_wine winetest_ok(outpItems[x].a.fRTL == items[x].fRTL, "%i:Wrong fRTL\n",x);
        else
            winetest_ok(outpItems[x].a.fRTL == items[x].fRTL, "%i:Wrong fRTL(%i)\n",x,outpItems[x].a.fRTL);
        if (items[x].todo_flag[2])
            todo_wine winetest_ok(outpItems[x].a.fLayoutRTL == items[x].fLayoutRTL, "%i:Wrong fLayoutRTL\n",x);
        else
            winetest_ok(outpItems[x].a.fLayoutRTL == items[x].fLayoutRTL, "%i:Wrong fLayoutRTL(%i)\n",x,outpItems[x].a.fLayoutRTL);
        if (items[x].todo_flag[3])
            todo_wine winetest_ok(outpItems[x].a.s.uBidiLevel == items[x].uBidiLevel, "%i:Wrong BidiLevel\n",x);
        else
            winetest_ok(outpItems[x].a.s.uBidiLevel == items[x].uBidiLevel, "%i:Wrong BidiLevel(%i)\n",x,outpItems[x].a.s.uBidiLevel);
    }
}

#define test_items_ok(a,b,c,d,e,f,g) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _test_items_ok(a,b,c,d,e,f,g)


static void test_ScriptItemize( void )
{
    static const WCHAR test1[] = {'t', 'e', 's', 't',0};
    static const itemTest t11[2] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},4,0,0,0}};
    static const itemTest t12[2] = {{{0,0,0,0},0,0,0,2},{{0,0,0,0},4,0,0,0}};

    /* Arabic, English*/
    static const WCHAR test2[] = {'1','2','3','-','5','2',0x064a,0x064f,0x0633,0x0627,0x0648,0x0650,0x064a,'7','1','.',0};
    static const itemTest t21[7] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},3,0,0,0},{{0,0,0,0},4,0,0,0},{{0,0,0,0},6,1,1,1},{{0,0,0,0},13,0,0,0},{{0,0,0,0},15,0,0,0},{{0,0,0,0},16,0,0,0}};
    static const itemTest t22[5] = {{{0,0,0,1},0,0,0,2},{{0,0,0,0},6,1,1,1},{{0,0,1,0},13,0,1,2},{{0,0,0,0},15,0,0,0},{{0,0,0,0},16,0,0,0}};
    static const itemTest t23[5] = {{{0,0,1,0},0,0,1,2},{{0,0,0,0},6,1,1,1},{{0,0,1,0},13,0,1,2},{{0,0,0,0},15,1,1,1},{{0,0,0,0},16,0,0,0}};

    /* Thai */
    static const WCHAR test3[] =
{0x0e04,0x0e27,0x0e32,0x0e21,0x0e1e,0x0e22,0x0e32,0x0e22,0x0e32, 0x0e21
,0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e44,0x0e2b,0x0e19
,0x0e04,0x0e27,0x0e32,0x0e21,0x0e2a, 0x0e33,0x0e40,0x0e23,0x0e47,0x0e08,
 0x0e2d,0x0e22,0x0e39,0x0e48,0x0e17,0x0e35,0x0e48,0x0e19,0x0e31,0x0e48,0x0e19,0};

    static const itemTest t31[2] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},41,0,0,0}};
    static const itemTest t32[2] = {{{0,0,0,0},0,0,0,2},{{0,0,0,0},41,0,0,0}};

    static const WCHAR test4[]  = {'1','2','3','-','5','2',' ','i','s',' ','7','1','.',0};

    static const itemTest t41[6] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},3,0,0,0},{{0,0,0,0},4,0,0,0},{{0,0,0,0},7,0,0,0},{{0,0,0,0},10,0,0,0},{{0,0,0,0},12,0,0,0}};
    static const itemTest t42[5] = {{{0,0,1,0},0,0,1,2},{{0,0,0,0},6,1,1,1},{{0,0,0,0},7,0,0,2},{{1,0,0,1},10,0,0,2},{{1,0,0,0},12,0,0,0}};

    /* Arabic */
    static const WCHAR test5[]  =
{0x0627,0x0644,0x0635,0x0651,0x0650,0x062d,0x0629,0x064f,' ',0x062a,0x064e,
0x0627,0x062c,0x064c,' ',0x0639,0x064e,0x0644,0x0649,' ',
0x0631,0x064f,0x0624,0x0648,0x0633,0x0650,' ',0x0627,0x0644
,0x0623,0x0635,0x0650,0x062d,0x0651,0x064e,0x0627,0x0621,0x0650,0};
    static const itemTest t51[2] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},38,0,0,0}};

    /* Hebrew */
    static const WCHAR test6[]  = {0x05e9, 0x05dc, 0x05d5, 0x05dd, '.',0};
    static const itemTest t61[3] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,0,0,0},{{0,0,0,0},5,0,0,0}};
    static const itemTest t62[3] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,1,1,1},{{0,0,0,0},5,0,0,0}};
    static const WCHAR test7[]  = {'p','a','r','t',' ','o','n','e',' ',0x05d7, 0x05dc, 0x05e7, ' ', 0x05e9, 0x05ea, 0x05d9, 0x05d9, 0x05dd, ' ','p','a','r','t',' ','t','h','r','e','e', 0};
    static const itemTest t71[4] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},9,1,1,1},{{0,0,0,0},19,0,0,0},{{0,0,0,0},29,0,0,0}};
    static const itemTest t72[4] = {{{0,0,0,0},0,0,0,0},{{0,0,0,0},9,1,1,1},{{0,0,0,0},18,0,0,0},{{0,0,0,0},29,0,0,0}};
    static const itemTest t73[4] = {{{0,0,0,0},0,0,0,2},{{0,0,0,0},8,1,1,1},{{0,0,0,0},19,0,0,2},{{0,0,0,0},29,0,0,0}};
    static const WCHAR test8[] = {0x0633, 0x0644, 0x0627, 0x0645,0};
    static const itemTest t81[2] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,0,0,0}};

    /* Syriac  (Like Arabic )*/
    static const WCHAR test9[] = {0x0710, 0x0712, 0x0712, 0x0714, '.',0};
    static const itemTest t91[3] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,0,0,0},{{0,0,0,0},5,0,0,0}};
    static const itemTest t92[3] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,1,1,1},{{0,0,0,0},5,0,0,0}};

    static const WCHAR test10[] = {0x0717, 0x0718, 0x071a, 0x071b,0};
    static const itemTest t101[2] = {{{0,0,0,0},0,1,1,1},{{0,0,0,0},4,0,0,0}};

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

    test_items_ok(test1,4,NULL,NULL,1,t11,FALSE);
    test_items_ok(test2,16,NULL,NULL,6,t21,FALSE);
    test_items_ok(test3,41,NULL,NULL,1,t31,FALSE);
    test_items_ok(test4,12,NULL,NULL,5,t41,FALSE);
    test_items_ok(test5,38,NULL,NULL,1,t51,FALSE);
    test_items_ok(test6,5,NULL,NULL,2,t61,FALSE);
    test_items_ok(test7,29,NULL,NULL,3,t71,FALSE);
    test_items_ok(test8,4,NULL,NULL,1,t81,FALSE);
    test_items_ok(test9,5,NULL,NULL,2,t91,FALSE);
    test_items_ok(test10,4,NULL,NULL,1,t101,FALSE);

    State.uBidiLevel = 0;
    test_items_ok(test1,4,&Control,&State,1,t11,FALSE);
    test_items_ok(test2,16,&Control,&State,4,t22,FALSE);
    test_items_ok(test3,41,&Control,&State,1,t31,FALSE);
    test_items_ok(test4,12,&Control,&State,5,t41,FALSE);
    test_items_ok(test5,38,&Control,&State,1,t51,FALSE);
    test_items_ok(test6,5,&Control,&State,2,t61,FALSE);
    test_items_ok(test7,29,&Control,&State,3,t72,FALSE);
    test_items_ok(test8,4,&Control,&State,1,t81,FALSE);
    test_items_ok(test9,5,&Control,&State,2,t91,FALSE);
    test_items_ok(test10,4,&Control,&State,1,t101,FALSE);

    State.uBidiLevel = 1;
    test_items_ok(test1,4,&Control,&State,1,t12,FALSE);
    test_items_ok(test2,16,&Control,&State,4,t23,FALSE);
    test_items_ok(test3,41,&Control,&State,1,t32,FALSE);
    test_items_ok(test4,12,&Control,&State,4,t42,TRUE);
    test_items_ok(test5,38,&Control,&State,1,t51,FALSE);
    test_items_ok(test6,5,&Control,&State,2,t62,FALSE);
    test_items_ok(test7,29,&Control,&State,3,t73,FALSE);
    test_items_ok(test8,4,&Control,&State,1,t81,FALSE);
    test_items_ok(test9,5,&Control,&State,2,t92,FALSE);
    test_items_ok(test10,4,&Control,&State,1,t101,FALSE);
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
    ok(!hr, "ScriptItemize should return S_OK not %08x\n", hr);
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
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");


    memset(glyphs,-1,sizeof(glyphs));
    memset(logclust,-1,sizeof(logclust));
    memset(attrs,-1,sizeof(attrs));
    hr = ScriptShape(NULL, &sc, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);
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
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);
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
    ok(!hr, "ScriptItemize should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(hdc, &sc, test1, 4, 4, &items[0].a, glyphs, logclust, attrs, &nb);
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);
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
    ok(!hr, "ScriptPlace should return S_OK not %08x\n", hr);
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
    trace("number of script properties %d\n", iMaxProps);
    ok (iMaxProps > 0, "Number of scripts returned should not be 0\n");
    if  (iMaxProps > 0)
         ok( ppSp[5]->langid == 9, "Langid[5] not = to 9\n"); /* Check a known value to ensure   */
                                                              /* ptrs work                       */

    /* This is a valid test that will cause parsing to take place                             */
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
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
    cMaxItems = 255;
    if (hr == 0) {
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
        ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "ScriptPlace should return 0 not (%08x)\n", hr);
            hr = ScriptPlace(NULL, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "ScriptPlace should return 0 not (%08x)\n", hr);
            for (cnt=0; cnt < pcGlyphs; cnt++)
                pwOutGlyphs[cnt] = pwOutGlyphs1[cnt];                 /* Send to next function */
        }

        /* This test will check to make sure that SCRIPT_CACHE is reused and that not translation   *
         * takes place if fNoGlyphIndex is set.                                                     */

        cInChars = 5;
        cMaxItems = 255;
        hr = ScriptItemize(TestItem2, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
        ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
        /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is   *
         *  returned.                                                                               */
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
                            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
                             pItem[0].iCharPos, cInChars, pItem[1].iCharPos);
        /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue                    */
        if (hr == 0) {
             cChars = cInChars;
             cMaxGlyphs = 256;
             pItem[0].a.fNoGlyphIndex = 1;                /* say no translate                     */
             hr = ScriptShape(NULL, &psc, TestItem2, cChars,
                              cMaxGlyphs, &pItem[0].a,
                              pwOutGlyphs2, pwLogClust, psva, &pcGlyphs);
             ok (hr != E_PENDING, "If psc should not be NULL (%08x) and the E_PENDING should be returned\n", hr);
             ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", hr);
             ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
             ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
             for (cnt=0; cnt < cChars && TestItem2[cnt] == pwOutGlyphs2[cnt]; cnt++) {}
             ok (cnt == cChars, "Translation to place when told not to. WCHAR %d - %04x != %04x\n",
                           cnt, TestItem2[cnt], pwOutGlyphs2[cnt]);
             if (hr ==0) {
                 hr = ScriptPlace(hdc, &psc, pwOutGlyphs2, pcGlyphs, psva, &pItem[0].a, piAdvance,
                                  pGoffset, pABC);
                 ok (hr == 0, "ScriptPlace should return 0 not (%08x)\n", hr);
             }
        }
        hr = ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");

    }

    /* This is a valid test that will cause parsing to take place and create 3 script_items   */
    cInChars = (sizeof(TestItem3)/2)-1;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem3, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
    if  (hr == 0)
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
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
    if  (hr == 0)
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
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
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
    ok (hr == 0, "ScriptGetCMap should return 0 not (%08x)\n", hr);
    ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
    for (cnt=0; cnt < cChars && pwOutGlyphs[cnt] == pwOutGlyphs3[cnt]; cnt++) {}
    ok (cnt == cInChars, "Translation not correct. WCHAR %d - %04x != %04x\n",
                         cnt, pwOutGlyphs[cnt], pwOutGlyphs3[cnt]);

    hr = ScriptFreeCache( &psc);
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
    ok(pwOutGlyphs3[1] == pwOutGlyphs2[1], "glyph incorreclty altered\n");
    ok(pwOutGlyphs3[2] == pwOutGlyphs2[2], "glyph incorreclty altered\n");
    ok(pwOutGlyphs3[3] == pwOutGlyphs2[3], "glyph incorreclty altered\n");
    ok(pwOutGlyphs3[4] == pwOutGlyphs2[4], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[5] == pwOutGlyphs2[5], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[6] == pwOutGlyphs2[6], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[7] == pwOutGlyphs2[7], "glyph not mirrored correctly\n");
    ok(pwOutGlyphs3[8] == pwOutGlyphs2[8], "glyph not mirrored correctly\n");

    hr = ScriptFreeCache( &psc);
    ok (!psc, "psc is not null after ScriptFreeCache\n");
}

static void test_ScriptGetFontProperties(HDC hdc)
{
    HRESULT         hr;
    SCRIPT_CACHE    psc,old_psc;
    SCRIPT_FONTPROPERTIES sfp;

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
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
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
    cMaxItems = 255;
    if (hr == 0) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "Should return 0 not (%08x)\n", hr);
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
            ok (hr == 0, "ScriptTextOut should return 0 not (%08x)\n", hr);

            /* Test Rect Rgn is acceptable */
            rect.top = 10;
            rect.bottom = 20;
            rect.left = 10;
            rect.right = 40;
            hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == 0, "ScriptTextOut should return 0 not (%08x)\n", hr);

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
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
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
    cMaxItems = 255;
    if (hr == 0) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc2, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc2, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "Should return 0 not (%08x)\n", hr);

            /*   key part!!!   cached dc is being deleted  */
            hr = DeleteDC(hdc2);
            ok(hr == 1, "DeleteDC should return 1 not %08x\n", hr);

            /* At this point the cached hdc (hdc2) has been destroyed,
             * however, we are passing in a *real* hdc (the original hdc).
             * The text should be written to that DC
             */
            hr = ScriptTextOut(hdc1, &psc, 0, 0, 0, NULL, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == 0, "ScriptTextOut should return 0 not (%08x)\n", hr);
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

    /* This is to ensure that non exisiting glyphs are translated into a valid glyph number */
    cInChars = 2;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", hr);
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
    cMaxItems = 255;
    if (hr == 0) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = cInChars;
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs1, pwLogClust, psva, &pcGlyphs);
        ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            /* Note hdc is needed as glyph info is not yet in psc                  */
            hr = ScriptPlace(hdc, &psc, pwOutGlyphs1, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "Should return 0 not (%08x)\n", hr);

            /* Test Rect Rgn is acceptable */
            rect.top = 10;
            rect.bottom = 20;
            rect.left = 10;
            rect.right = 40;
            hr = ScriptTextOut(hdc, &psc, 0, 0, 0, &rect, &pItem[0].a, NULL, 0, pwOutGlyphs1, pcGlyphs,
                               piAdvance, NULL, pGoffset);
            ok (hr == 0, "ScriptTextOut should return 0 not (%08x)\n", hr);

        }
        /* Clean up and go   */
        ScriptFreeCache(&psc);
        ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);
    }
}

static void test_ScriptXtoX(void)
/****************************************************************************************
 *  This routine tests the ScriptXtoCP and ScriptCPtoX functions using static variables *
 ****************************************************************************************/
{
    static const WCHAR test[] = {'t', 'e', 's', 't',0};
    SCRIPT_ITEM items[2];
    int iX, iCP;
    int cChars;
    int cGlyphs;
    WORD pwLogClust[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    SCRIPT_VISATTR psva[10];
    int piAdvance[10] = {200, 190, 210, 180, 170, 204, 189, 195, 212, 203};
    int piCP, piX;
    int piTrailing;
    BOOL fTrailing;
    HRESULT hr;

    hr = ScriptItemize(test, lstrlenW(test), sizeof(items)/sizeof(items[0]), NULL, NULL, items, NULL);
    ok(!hr, "ScriptItemize should return S_OK not %08x\n", hr);

    iX = -1;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    if (piTrailing)
        ok(piCP == -1, "Negative iX should return piCP=-1 not %d\n", piCP);
    else /* win2k3 */
        ok(piCP == 10, "Negative iX should return piCP=10 not %d\n", piCP);

    iX = 1954;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    if (piTrailing) /* win2k3 */
        ok(piCP == -1, "Negative iX should return piCP=-1 not %d\n", piCP);
    else
        ok(piCP == 10, "Negative iX should return piCP=10 not %d\n", piCP);

    iX = 779;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 3 ||
       piCP == -1, /* win2k3 */
       "iX=%d should return piCP=3 or piCP=-1 not %d\n", iX, piCP);
    ok(piTrailing == 1, "iX=%d should return piTrailing=1 not %d\n", iX, piTrailing);

    iX = 780;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 3 ||
       piCP == -1, /* win2k3 */
       "iX=%d should return piCP=3 or piCP=-1 not %d\n", iX, piCP);
    ok(piTrailing == 1, "iX=%d should return piTrailing=1 not %d\n", iX, piTrailing);

    iX = 868;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 4 ||
       piCP == -1, /* win2k3 */
       "iX=%d should return piCP=4 or piCP=-1 not %d\n", iX, piCP);

    iX = 0;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 0 ||
       piCP == 10, /* win2k3 */
       "iX=%d should return piCP=0 piCP=10 not %d\n", iX, piCP);

    iX = 195;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 0, "iX=%d should return piCP=0 not %d\n", iX, piCP);

    iX = 196;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptXtoCP(iX, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piCP, &piTrailing);
    ok(hr == S_OK, "ScriptXtoCP should return S_OK not %08x\n", hr);
    ok(piCP == 1 ||
       piCP == 0, /* win2k3 */
       "iX=%d should return piCP=1 or piCP=0 not %d\n", iX, piCP);

    iCP=5;
    fTrailing = FALSE;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptCPtoX(iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piX);
    ok(hr == S_OK, "ScriptCPtoX should return S_OK not %08x\n", hr);
    ok(piX == 976 ||
       piX == 100, /* win2k3 */
       "iCP=%d should return piX=976 or piX=100 not %d\n", iCP, piX);

    iCP=5;
    fTrailing = TRUE;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptCPtoX(iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piX);
    ok(hr == S_OK, "ScriptCPtoX should return S_OK not %08x\n", hr);
    ok(piX == 1171 ||
       piX == 80, /* win2k3 */
       "iCP=%d should return piX=1171 or piX=80 not %d\n", iCP, piX);

    iCP=6;
    fTrailing = FALSE;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptCPtoX(iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piX);
    ok(hr == S_OK, "ScriptCPtoX should return S_OK not %08x\n", hr);
    ok(piX == 1171 ||
       piX == 80, /* win2k3 */
       "iCP=%d should return piX=1171 or piX=80 not %d\n", iCP, piX);

    iCP=11;
    fTrailing = FALSE;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptCPtoX(iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piX);
    ok(hr == S_OK, "ScriptCPtoX should return S_OK not %08x\n", hr);
    ok(piX == 1953 ||
       piX == 0, /* win2k3 */
       "iCP=%d should return piX=1953 or piX=0 not %d\n", iCP, piX);

    iCP=11;
    fTrailing = TRUE;
    cChars = 10;
    cGlyphs = 10;
    hr = ScriptCPtoX(iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance, &items[0].a, &piX);
    ok(hr == S_OK, "ScriptCPtoX should return S_OK not %08x\n", hr);
    ok(piX == 1953 ||
       piX == 0, /* win2k3 */
       "iCP=%d should return piX=1953 or piX=0 not %d\n", iCP, piX);
}

static void test_ScriptString(HDC hdc)
{
/*******************************************************************************************
 *
 * This set of tests are for the string functions of uniscribe.  The ScriptStringAnalyse
 * function allocates memory pointed to by the SCRIPT_STRING_ANALYSIS ssa pointer.  This
 * memory if freed by ScriptStringFree.  There needs to be a valid hdc for this as
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
    SCRIPT_CONTROL  Control;
    SCRIPT_STATE    State;
    const int       Dx[5] = {10, 10, 10, 10, 10};
    SCRIPT_TABDEF   Tabdef;
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
                             ReqWidth, &Control, &State, Dx, &Tabdef,
                             &InClass, &ssa);
    ok(hr == E_PENDING, "ScriptStringAnalyse Stub should return E_PENDING not %08x\n", hr);

    /* test with hdc, this should be a valid test  */
    hr = ScriptStringAnalyse( hdc, teststr, len, Glyphs, Charset, Flags,
                              ReqWidth, &Control, &State, Dx, &Tabdef,
                              &InClass, &ssa);
    ok(hr == S_OK, "ScriptStringAnalyse should return S_OK not %08x\n", hr);
    ScriptStringFree(&ssa);

    /* test makes sure that a call with a valid pssa still works */
    hr = ScriptStringAnalyse( hdc, teststr, len, Glyphs, Charset, Flags,
                              ReqWidth, &Control, &State, Dx, &Tabdef,
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
    WCHAR           teststr1[] = {'T', 'e', 's', 't', 'e', 'a', 'b', ' ', 'a', '\0'};
    void            *String = (WCHAR *) &teststr1;      /* ScriptStringAnalysis needs void */
    int             String_len = (sizeof(teststr1)/sizeof(WCHAR))-1;
    int             Glyphs = String_len * 2 + 16;       /* size of buffer as recommended  */
    int             Charset = -1;                       /* unicode                        */
    DWORD           Flags = SSA_GLYPHS;
    int             ReqWidth = 100;
    SCRIPT_CONTROL  Control;
    SCRIPT_STATE    State;
    SCRIPT_TABDEF   Tabdef;
    const BYTE      InClass = 0;
    SCRIPT_STRING_ANALYSIS ssa = NULL;

    int             Ch;                                  /* Character position in string */
    int             iTrailing;
    int             Cp;                                  /* Character position in string */
    int             X;
    BOOL            fTrailing;

    /* Test with hdc, this should be a valid test
     * Here we generate an SCRIPT_STRING_ANALYSIS that will be used as input to the
     * following character positions to X and X to character position functions.
     */
    memset(&Control, 0, sizeof(SCRIPT_CONTROL));
    memset(&State, 0, sizeof(SCRIPT_STATE));
    memset(&Tabdef, 0, sizeof(SCRIPT_TABDEF));

    hr = ScriptStringAnalyse( hdc, String, String_len, Glyphs, Charset, Flags,
                              ReqWidth, &Control, &State, NULL, &Tabdef,
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
            fTrailing = FALSE;
            hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
            ok(Cp == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp, Ch, X);
            ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n", 
                                  iTrailing, X);
            fTrailing = TRUE;
            hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
            ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
            hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
            ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);

            /*
             * Check that character position returned by ScriptStringXtoCP in Ch matches the
             * one input to ScriptStringCPtoX.  This means that the Cp to X position and back
             * again works
             */
            ok(Cp + 1 == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp + 1, Ch, X);
            ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n", 
                                   iTrailing, X);
        }
        /*
         * This test is to check that if the X position is just inside the trailing edge of the
         * character then iTrailing will indicate the trailing edge, ie. TRUE
         */
        fTrailing = TRUE;
        Cp = 3;
        hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
        ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
        X--;                                /* put X just inside the trailing edge */
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
        ok(Cp == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp, Ch, X);
        ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = %d\n", 
                                  iTrailing, X);

        /*
         * This test is to check that if the X position is just outside the trailing edge of the
         * character then iTrailing will indicate the leading edge, ie. FALSE, and Ch will indicate
         * the next character, ie. Cp + 1 
         */
        fTrailing = TRUE;
        Cp = 3;
        hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
        ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
        X++;                                /* put X just outside the trailing edge */
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
        ok(Cp + 1 == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp + 1, Ch, X);
        ok(iTrailing == FALSE, "ScriptStringXtoCP should return iTrailing = 0 not %d for X = %d\n", 
                                  iTrailing, X);

        /*
         * This test is to check that if the X position is just outside the leading edge of the
         * character then iTrailing will indicate the trailing edge, ie. TRUE, and Ch will indicate
         * the next character down , ie. Cp - 1 
         */
        fTrailing = FALSE;
        Cp = 3;
        hr = ScriptStringCPtoX(ssa, Cp, fTrailing, &X);
        ok(hr == S_OK, "ScriptStringCPtoX should return S_OK not %08x\n", hr);
        X--;                                /* put X just outside the leading edge */
        hr = ScriptStringXtoCP(ssa, X, &Ch, &iTrailing);
        ok(hr == S_OK, "ScriptStringXtoCP should return S_OK not %08x\n", hr);
        ok(Cp - 1 == Ch, "ScriptStringXtoCP should return Ch = %d not %d for X = %d\n", Cp - 1, Ch, X);
        ok(iTrailing == TRUE, "ScriptStringXtoCP should return iTrailing = 1 not %d for X = %d\n", 
                                  iTrailing, X);

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
                                  ReqWidth, &Control, &State, NULL, &Tabdef,
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
    ok(!hr, "ScriptItemize should return S_OK not %08x\n", hr);

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test, 1, &items[0].a, &la);
    ok(!hr, "ScriptBreak should return S_OK not %08x\n", hr);

    ok(!la.fSoftBreak, "fSoftBreak set\n");
    ok(la.fWhiteSpace, "fWhiteSpace not set\n");
    ok(la.fCharStop, "fCharStop not set\n");
    ok(!la.fWordStop, "fWordStop set\n");
    ok(!la.fInvalid, "fInvalid set\n");
    ok(!la.fReserved, "fReserved set\n");

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test + 1, 1, &items[1].a, &la);
    ok(!hr, "ScriptBreak should return S_OK not %08x\n", hr);

    ok(!la.fSoftBreak, "fSoftBreak set\n");
    ok(!la.fWhiteSpace, "fWhiteSpace set\n");
    ok(la.fCharStop, "fCharStop not set\n");
    ok(!la.fWordStop, "fWordStop set\n");
    ok(!la.fInvalid, "fInvalid set\n");
    ok(!la.fReserved, "fReserved set\n");

    memset(&la, 0, sizeof(la));
    hr = ScriptBreak(test + 2, 1, &items[2].a, &la);
    ok(!hr, "ScriptBreak should return S_OK not %08x\n", hr);

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

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}
