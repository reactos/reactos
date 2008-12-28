/*
 * Tests for usp10 dll
 *
 * Copyright 2006 Jeff Latimer
 * Copyright 2006 Hans Leidekker
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
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winerror.h>
#include <winnls.h>
#include <usp10.h>

static void test_ScriptShape(HDC hdc)
{
    static const WCHAR test1[] = {'t', 'e', 's', 't',0};
    BOOL ret;
    HRESULT hr;
    SCRIPT_CACHE sc = NULL;
    WORD glyphs[4];
    SCRIPT_VISATTR attrs[4];
    SCRIPT_ITEM items[2];
    int nb, widths[4];

    hr = ScriptItemize(NULL, 4, 2, NULL, NULL, items, NULL);
    ok(hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG not %08x\n", hr);

    hr = ScriptItemize(test1, 4, 2, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG not %08x\n", hr);

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
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);
    ok(items[0].a.fNoGlyphIndex == FALSE, "fNoGlyphIndex TRUE\n");

    hr = ScriptShape(NULL, &sc, test1, 4, 4, &items[0].a, glyphs, NULL, attrs, &nb);
    ok(!hr, "ScriptShape should return S_OK not %08x\n", hr);

    hr = ScriptPlace(hdc, &sc, glyphs, 4, NULL, &items[0].a, widths, NULL, NULL);
    ok(hr == E_INVALIDARG, "ScriptPlace should return E_INVALIDARG not %08x\n", hr);

    hr = ScriptPlace(NULL, &sc, glyphs, 4, attrs, &items[0].a, widths, NULL, NULL);
    ok(hr == E_PENDING, "ScriptPlace should return E_PENDING not %08x\n", hr);

    hr = ScriptPlace(hdc, &sc, glyphs, 4, attrs, &items[0].a, widths, NULL, NULL);
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
    WCHAR           TestItem4[] = {'T', 'e', 's', 't', 'c',' ',0x0684,0x0694,0x06a4,' ',' ','e','n','d',0};
    WCHAR           TestItem5[] = {0x0684,'T','e','s','t','c',' ',0x0684,0x0694,0x06a4,' ',' ','e','n','d',0}; 

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


    /* This set of tests are to check that the various edits in ScriptIemize work           */
    cInChars = 5;                                        /* Length of test without NULL     */
    cMaxItems = 1;                                       /* Check threshold value           */
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cMaxItems < 2.  Was %d\n",
        cMaxItems);
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(NULL, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pwcInChars is NULL\n");

    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, 0, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cInChars is 0\n");

    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, NULL, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pItems is NULL\n");

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
        /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
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
        hr = ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");
    }

    /* This is a valid test that will cause parsing to take place and create 3 script_items   */
    cInChars = (sizeof(TestItem4)/2)-1;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem4, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
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
        hr = ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");
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
}

static void test_ScriptGetCMap(HDC hdc, unsigned short pwOutGlyphs[256])
{
    HRESULT         hr;
    SCRIPT_CACHE    psc = NULL;
    int             cInChars;
    int             cChars;
    unsigned short  pwOutGlyphs3[256];
    WCHAR           TestItem1[] = {'T', 'e', 's', 't', 'a', 0}; 
    DWORD           dwFlags;
    int             cnt;

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
    hr = ScriptGetCMap(NULL, &psc, NULL, 0, 0, NULL);
    ok( hr == E_PENDING, "(NULL,&psc,NULL,0,0NULL), expected E_PENDING, "
                         "got %08x\n", hr);
    ok( psc == NULL, "Expected psc to be NULL, got %p\n", psc);

    /* Set psc to NULL but add hdc, to be able to check if a pointer is returned in psc */
    psc = NULL;
    hr = ScriptGetCMap(hdc, &psc, NULL, 0, 0, NULL);
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
    WCHAR           teststr1[] = {'T', 'e', 's', 't', 'e', '1', '2', ' ', 'a', '\0'};
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

        hr = ScriptStringFree(&ssa);
        /*
         * ScriptStringCPtoX should free ssa, hence ScriptStringFree should fail
         */
        ok(hr == E_INVALIDARG ||
           hr == E_FAIL, /* win2k3 */
           "ScriptStringFree should return E_INVALIDARG or E_FAIL not %08x\n", hr);
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
    static const BYTE levels[][5] =
    {
        { 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1 },
        { 2, 2, 2, 2, 2 },
        { 3, 3, 3, 3, 3 },
    };
    static const int expect[][5] =
    {
        { 0, 1, 2, 3, 4 },
        { 4, 3, 2, 1, 0 },
        { 0, 1, 2, 3, 4 },
        { 4, 3, 2, 1, 0 }
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
            ok(expect[i][j] == vistolog[j],
               "failure: levels[%d][%d] = %d, vistolog[%d] = %d\n",
               i, j, levels[i][j], j, vistolog[j] );
        }

        for (j = 0; j < sizeof(levels[i]); j++)
        {
            ok(expect[i][j] == logtovis[j],
               "failure: levels[%d][%d] = %d, logtovis[%d] = %d\n",
               i, j, levels[i][j], j, logtovis[j] );
        }
    }
}

static const struct
{
    LGRPID group;
    LCID lcid;
    SCRIPT_DIGITSUBSTITUTE sds;
    DWORD uDefaultLanguage;
    DWORD fContextDigits;
    WORD fDigitSubstitute;
}
subst_data[] =
{
    { 0x01, 0x00403, { 9, 3, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00406, { 9, 6, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00407, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00409, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0040a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0040b, { 9, 11, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0040c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0040f, { 9, 15, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00410, { 9, 16, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00413, { 9, 19, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00414, { 9, 20, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00416, { 9, 22, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0041d, { 9, 29, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00421, { 9, 33, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0042d, { 9, 45, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00432, { 9, 50, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00434, { 9, 52, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00435, { 9, 53, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00436, { 9, 54, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00438, { 9, 56, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0043a, { 9, 58, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0043b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0043e, { 9, 62, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00441, { 9, 65, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00452, { 9, 82, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00456, { 9, 86, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0046b, { 9, 107, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0046c, { 9, 108, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00481, { 9, 129, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00807, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00809, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0080a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0080c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00810, { 9, 16, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00813, { 9, 19, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00814, { 9, 20, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00816, { 9, 22, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0081d, { 9, 29, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0083b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0083e, { 9, 62, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0086b, { 9, 107, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c07, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c09, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c0a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c0c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c3b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x00c6b, { 9, 107, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01007, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01009, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0100a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0100c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0103b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01407, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01409, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0140a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0140c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0143b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01809, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0180a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0180c, { 9, 12, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0183b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01c09, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01c0a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x01c3b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x02009, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0200a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0203b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x02409, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0240a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0243b, { 9, 59, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x02809, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0280a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x02c09, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x02c0a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x03009, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0300a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x03409, { 9, 9, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0340a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0380a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x03c0a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0400a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0440a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0480a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x04c0a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x0500a, { 9, 10, 1, 0 }, 9, 0, 0 },
    { 0x01, 0x10407, { 9, 7, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x00405, { 9, 5, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0040e, { 9, 14, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x00415, { 9, 21, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x00418, { 9, 24, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0041a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0041b, { 9, 27, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0041c, { 9, 28, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x00424, { 9, 36, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0081a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0101a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0141a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x0181a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x02, 0x1040e, { 9, 14, 1, 0 }, 9, 0, 0 },
    { 0x03, 0x00425, { 9, 37, 1, 0 }, 9, 0, 0 },
    { 0x03, 0x00426, { 9, 38, 1, 0 }, 9, 0, 0 },
    { 0x03, 0x00427, { 9, 39, 1, 0 }, 9, 0, 0 },
    { 0x04, 0x00408, { 9, 8, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00402, { 9, 2, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00419, { 9, 25, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00422, { 9, 34, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00423, { 9, 35, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x0042f, { 9, 47, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x0043f, { 9, 63, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00440, { 9, 64, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00444, { 9, 68, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00450, { 9, 80, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x0082c, { 9, 44, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00843, { 9, 67, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x00c1a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x05, 0x01c1a, { 9, 26, 1, 0 }, 9, 0, 0 },
    { 0x06, 0x0041f, { 9, 31, 1, 0 }, 9, 0, 0 },
    { 0x06, 0x0042c, { 9, 44, 1, 0 }, 9, 0, 0 },
    { 0x06, 0x00443, { 9, 67, 1, 0 }, 9, 0, 0 },
    { 0x07, 0x00411, { 9, 17, 1, 0 }, 9, 0, 0 },
    { 0x08, 0x00412, { 9, 18, 1, 0 }, 9, 0, 0 },
    { 0x09, 0x00404, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x09, 0x00c04, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x09, 0x01404, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x09, 0x21404, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x09, 0x30404, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x0a, 0x00804, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x0a, 0x01004, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x0a, 0x20804, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x0a, 0x21004, { 9, 4, 1, 0 }, 9, 0, 0 },
    { 0x0b, 0x0041e, { 9, 30, 1, 0 }, 9, 0, 0 },
    { 0x0c, 0x0040d, { 9, 13, 1, 0 }, 9, 0, 0 },
    { 0x0d, 0x00401, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x00420, { 9, 32, 1, 0 }, 9, 0, 0 },
    { 0x0d, 0x00429, { 41, 41, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x0045a, { 9, 90, 1, 0 }, 9, 0, 0 },
    { 0x0d, 0x00465, { 9, 101, 1, 0 }, 9, 0, 0 },
    { 0x0d, 0x00801, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x00c01, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x01001, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x01401, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x01801, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x01c01, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x02001, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x02401, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x02801, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x02c01, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x03001, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x03401, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x03801, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x03c01, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0d, 0x04001, { 1, 1, 0, 0 }, 9, 0, 0 },
    { 0x0e, 0x0042a, { 9, 42, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x00439, { 9, 57, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x00446, { 9, 70, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x00447, { 9, 71, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x00449, { 9, 73, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x0044a, { 9, 74, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x0044b, { 9, 75, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x0044e, { 9, 78, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x0044f, { 9, 79, 1, 0 }, 9, 0, 0 },
    { 0x0f, 0x00457, { 9, 87, 1, 0 }, 9, 0, 0 },
    { 0x10, 0x00437, { 9, 55, 1, 0 }, 9, 0, 0 },
    { 0x10, 0x10437, { 9, 55, 1, 0 }, 9, 0, 0 },
    { 0x11, 0x0042b, { 9, 43, 1, 0 }, 9, 0, 0 }
};

static BOOL CALLBACK enum_proc(LGRPID group, LCID lcid, LPSTR locale, LONG_PTR lparam)
{
    HRESULT hr;
    SCRIPT_DIGITSUBSTITUTE sds;
    SCRIPT_CONTROL sc;
    SCRIPT_STATE ss;
    LCID lcid_old;
    unsigned int i;

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

    for (i = 0; i < sizeof(subst_data)/sizeof(subst_data[0]); i++)
    {
        if (group == subst_data[i].group && lcid == subst_data[i].lcid)
        {
            ok(!memcmp(&sds, &subst_data[i].sds, sizeof(sds)),
               "substitution data does not match\n");

            ok(sc.uDefaultLanguage == subst_data[i].uDefaultLanguage,
               "sc.uDefaultLanguage does not match\n");
            ok(sc.fContextDigits == subst_data[i].fContextDigits,
               "sc.fContextDigits does not match\n");
            ok(ss.fDigitSubstitute == subst_data[i].fDigitSubstitute,
               "ss.fDigitSubstitute does not match\n");
        }
    }
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
    static BOOL (WINAPI * pEnumLanguageGroupLocalesA)(LANGGROUPLOCALE_ENUMPROC,LGRPID,DWORD,LONG_PTR);

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

    test_ScriptItemIzeShapePlace(hdc,pwOutGlyphs);
    test_ScriptGetCMap(hdc, pwOutGlyphs);
    test_ScriptCacheGetHeight(hdc);
    test_ScriptGetGlyphABCWidth(hdc);
    test_ScriptShape(hdc);

    test_ScriptGetFontProperties(hdc);
    test_ScriptTextOut(hdc);
    test_ScriptTextOut2(hdc);
    test_ScriptXtoX();
    test_ScriptString(hdc);
    test_ScriptStringXtoCP_CPtoX(hdc);

    test_ScriptLayout();
    test_digit_substitution();
    test_ScriptGetProperties();

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}
