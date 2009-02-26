/*
 * DrawText tests
 *
 * Copyright (c) 2004 Zach Gorman
 * Copyright 2007 Dmitry Timoshkov
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

#include <assert.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#define MODIFIED(rect) (rect.left = 10 && rect.right != 100 && rect.top == 10 && rect.bottom != 100)
#define SAME(rect) (rect.left = 10 && rect.right == 100 && rect.top == 10 && rect.bottom == 100)
#define EMPTY(rect) (rect.left == rect.right && rect.bottom == rect.top)

static void test_DrawTextCalcRect(void)
{
    HWND hwnd;
    HDC hdc;
    HFONT hFont, hOldFont;
    LOGFONTA lf;
    static CHAR text[] = "Example text for testing DrawText in "
      "MM_HIENGLISH mode";
    static WCHAR textW[] = {'W','i','d','e',' ','c','h','a','r',' ',
        's','t','r','i','n','g','\0'};
    static CHAR emptystring[] = "";
    static WCHAR emptystringW[] = { 0 };
    INT textlen, textheight, heightcheck;
    RECT rect = { 0, 0, 100, 0 };
    BOOL ret;
    DRAWTEXTPARAMS dtp;
    BOOL conform_xp = TRUE;

    /* Initialization */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %u\n", GetLastError());
    trace("hdc %p\n", hdc);
    textlen = lstrlenA(text);

    /* LOGFONT initialization */
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 0; /* mapping mode dependent */
    lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(lf.lfFaceName, "Arial");

    /* DrawText in MM_HIENGLISH with DT_CALCRECT */
    SetMapMode(hdc, MM_HIENGLISH);
    lf.lfHeight = 100 * 9 / 72; /* 9 point */
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != 0, "CreateFontIndirectA error %u\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %u\n", GetLastError());

    trace("MM_HIENGLISH rect.bottom %d\n", rect.bottom);
    todo_wine ok(rect.bottom < 0, "In MM_HIENGLISH, DrawText with "
       "DT_CALCRECT should return a negative rectangle bottom. "
       "(bot=%d)\n", rect.bottom);

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %u\n", GetLastError());


    /* DrawText in MM_TEXT with DT_CALCRECT */
    SetMapMode(hdc, MM_TEXT);
    lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc,
       LOGPIXELSY), 72); /* 9 point */
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != 0, "CreateFontIndirectA error %u\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %u\n", GetLastError());

    trace("MM_TEXT rect.bottom %d\n", rect.bottom);
    ok(rect.bottom > 0, "In MM_TEXT, DrawText with DT_CALCRECT "
       "should return a positive rectangle bottom. (bot=%d)\n",
       rect.bottom);

    /* empty or null text should in some cases calc an empty rectangle */

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, text, 0, &rect, DT_CALCRECT, NULL );
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (textheight != 0)  /* Windows 98 */
    {
        win_skip("XP conformity failed, skipping XP tests. Probably win9x\n");
        conform_xp = FALSE;
    }
    else
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, text, 0, &rect, DT_CALCRECT);
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, emptystring, -1, &rect, DT_CALCRECT, NULL );
    ok( EMPTY(rect),
        "rectangle should be empty got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT);
    ok( EMPTY(rect),
        "rectangle should be empty got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
    ok( EMPTY(rect) || !MODIFIED(rect),
        "rectangle should be empty or not modified got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (!textheight) /* Windows NT 4 */
    {
        if (conform_xp)
            win_skip("XP conformity failed, skipping XP tests. Probably winNT\n");
        conform_xp = FALSE;
    }
    else
        ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, -1, &rect, DT_CALCRECT);
    ok( EMPTY(rect) || !MODIFIED(rect),
        "rectangle should be empty or NOT modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, 0, &rect, DT_CALCRECT);
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    /* DT_SINGLELINE tests */

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, text, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, text, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, emptystring, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok( !EMPTY(rect) && MODIFIED(rect),
        "rectangle should be modified got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok( !EMPTY(rect) && MODIFIED (rect),
        "rectangle should be modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok( (!EMPTY(rect) && MODIFIED(rect)) || !MODIFIED(rect),
        "rectangle should be modified got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok( (!EMPTY(rect) && MODIFIED(rect)) || !MODIFIED(rect),
        "rectangle should be modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok( !EMPTY(rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
        rect.left, rect.top, rect.right, rect.bottom );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    /* further tests with  0 count, NULL and empty strings */
    heightcheck = textheight = DrawTextA(hdc, text, 0, &rect, 0);
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, text, 0, &rect, 0, NULL );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
    heightcheck = textheight = DrawTextA(hdc, emptystring, 0, &rect, 0);
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, emptystring, 0, &rect, 0, NULL );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
    heightcheck = textheight = DrawTextA(hdc, NULL, 0, &rect, 0);
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, NULL, 0, &rect, 0, NULL );
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
    heightcheck = textheight = DrawTextA(hdc, emptystring, -1, &rect, 0);
    ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, emptystring, -1, &rect, 0, NULL );
    ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
    heightcheck = textheight = DrawTextA(hdc, NULL, -1, &rect, 0);
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, NULL, -1, &rect, 0, NULL );
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
    heightcheck = textheight = DrawTextA(hdc, NULL, 10, &rect, 0);
    ok(textheight==0,"Got textheight from DrawTextA\n");
    textheight = DrawTextExA(hdc, NULL, 10, &rect, 0, NULL );
    ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");


    /* invalid dtp size test */
    dtp.cbSize = -1; /* Invalid */
    dtp.uiLengthDrawn = 1337;
    textheight = DrawTextExA(hdc, text, 0, &rect, 0, &dtp);
    ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
    dtp.uiLengthDrawn = 1337;
    textheight = DrawTextExA(hdc, emptystring, 0, &rect, 0, &dtp);
    ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
    dtp.uiLengthDrawn = 1337;
    textheight = DrawTextExA(hdc, NULL, 0, &rect, 0, &dtp);
    ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
    dtp.uiLengthDrawn = 1337;
    textheight = DrawTextExA(hdc, emptystring, -1, &rect, 0, &dtp);
    ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
    dtp.uiLengthDrawn = 1337;
    textheight = DrawTextExA(hdc, NULL, -1, &rect, 0, &dtp);
    ok(textheight==0,"Got textheight from DrawTextExA\n");
    ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);

    /* Margin calculations */
    dtp.cbSize = sizeof(dtp);
    dtp.iLeftMargin = 0;
    dtp.iRightMargin = 0;
    SetRect( &rect, 0, 0, 0, 0);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    textlen = rect.right; /* Width without margin */
    dtp.iLeftMargin = 8;
    SetRect( &rect, 0, 0, 0, 0);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    ok(rect.right==dtp.iLeftMargin+textlen  ,"Incorrect left margin calculated  rc(%d,%d)\n", rect.left, rect.right);
    dtp.iLeftMargin = 0;
    dtp.iRightMargin = 8;
    SetRect( &rect, 0, 0, 0, 0);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    ok(rect.right==dtp.iRightMargin+textlen  ,"Incorrect right margin calculated rc(%d,%d)\n", rect.left, rect.right);

    /* Wide char versions */
    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExW(hdc, textW, 0, &rect, DT_CALCRECT, NULL );
    if( GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) {
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, textW, 0, &rect, DT_CALCRECT);
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, emptystringW, -1, &rect, DT_CALCRECT, NULL );
        ok( EMPTY(rect),
            "rectangle should be empty got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT);
        ok( EMPTY(rect),
            "rectangle should be empty got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        if (textheight) /* windows 2000 */
        {
            if (conform_xp)
                win_skip("XP conformity failed, skipping XP tests. Probably win 2000\n");
            conform_xp = FALSE;
        }
        else
            ok(textheight==0,"Got textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, NULL, 0, &rect, DT_CALCRECT);
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        if (conform_xp) {
            /* Crashes on NT4 */
            SetRect( &rect, 10,10, 100, 100);
            heightcheck = textheight = DrawTextExW(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
            ok( !EMPTY(rect) && !MODIFIED(rect),
                "rectangle should NOT be empty  and NOT modified got %d,%d-%d,%d\n",
                rect.left, rect.top, rect.right, rect.bottom );
            ok(textheight==0,"Got textheight from DrawTextExW\n");

            SetRect( &rect, 10,10, 100, 100);
            textheight = DrawTextW(hdc, NULL, -1, &rect, DT_CALCRECT);
            ok( !EMPTY(rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
                rect.left, rect.top, rect.right, rect.bottom );
            ok(textheight==0,"Got textheight from DrawTextW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        }


        /* DT_SINGLELINE tests */

        heightcheck = textheight = DrawTextExW(hdc, textW, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, textW, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, emptystringW, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok( !EMPTY(rect) && MODIFIED(rect),
            "rectangle should be modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok( !EMPTY(rect) && MODIFIED(rect),
            "rectangle should be modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        if (conform_xp) {
            /* Crashes on NT4 */
            SetRect( &rect, 10,10, 100, 100);
            heightcheck = textheight = DrawTextExW(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
            ok( !EMPTY(rect) && !MODIFIED(rect),
                "rectangle should NOT be empty  and NOT modified got %d,%d-%d,%d\n",
                rect.left, rect.top, rect.right, rect.bottom );
            ok(textheight==0,"Got textheight from DrawTextExW\n");

            SetRect( &rect, 10,10, 100, 100);
            textheight = DrawTextW(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
            ok( !EMPTY(rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
                rect.left, rect.top, rect.right, rect.bottom );
            ok(textheight==0,"Got textheight from DrawTextW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        }

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok( !EMPTY(rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom );
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        /* further tests with NULL and empty strings */
        heightcheck = textheight = DrawTextW(hdc, textW, 0, &rect, 0);
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        textheight = DrawTextExW(hdc, textW, 0, &rect, 0, NULL );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        heightcheck = textheight = DrawTextW(hdc, emptystringW, 0, &rect, 0);
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        textheight = DrawTextExW(hdc, emptystringW, 0, &rect, 0, NULL );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        heightcheck = textheight = DrawTextW(hdc, NULL, 0, &rect, 0);
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextW\n");
        textheight = DrawTextExW(hdc, NULL, 0, &rect, 0, NULL );
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextExW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        heightcheck = textheight = DrawTextW(hdc, emptystringW, -1, &rect, 0);
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        textheight = DrawTextExW(hdc, emptystringW, -1, &rect, 0, NULL );
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        if (conform_xp) {
            /* Crashes on NT4 */
            heightcheck = textheight = DrawTextW(hdc, NULL, -1, &rect, 0);
            ok(textheight==0,"Got textheight from DrawTextW\n");
            textheight = DrawTextExW(hdc, NULL, -1, &rect, 0, NULL );
            ok(textheight==0,"Got textheight from DrawTextExW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
            heightcheck = textheight = DrawTextW(hdc, NULL, 10, &rect, 0);
            ok(textheight==0,"Got textheight from DrawTextW\n");
            textheight = DrawTextExW(hdc, NULL, 10, &rect, 0, NULL );
            ok(textheight==0,"Got textheight from DrawTextW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        }

        dtp.cbSize = -1; /* Invalid */
        dtp.uiLengthDrawn = 1337;
        textheight = DrawTextExW(hdc, textW, 0, &rect, 0, &dtp);
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");
        ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
        dtp.uiLengthDrawn = 1337;
        textheight = DrawTextExW(hdc, emptystringW, 0, &rect, 0, &dtp);
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextExW\n");
        ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
        dtp.uiLengthDrawn = 1337;
        textheight = DrawTextExW(hdc, NULL, 0, &rect, 0, &dtp);
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextExW\n");
        ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
        dtp.uiLengthDrawn = 1337;
        textheight = DrawTextExW(hdc, emptystringW, -1, &rect, 0, &dtp);
        ok(textheight==0,"Got textheight from DrawTextExW\n");
        ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
        if (conform_xp) {
            /* Crashes on NT4 */
            dtp.uiLengthDrawn = 1337;
            textheight = DrawTextExW(hdc, NULL, -1, &rect, 0, &dtp);
            ok(textheight==0,"Got textheight from DrawTextExW\n");
            ok(dtp.uiLengthDrawn==1337, "invalid dtp.uiLengthDrawn = %i\n",dtp.uiLengthDrawn);
        }
    }

    /* More test cases from bug 12226 */
    SetRect(&rect, 0, 0, 0, 0);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
    ok(textheight, "DrawTextA error %u\n", GetLastError());
    ok(0 == rect.left, "expected 0, got %d\n", rect.left);
    ok(0 == rect.right, "expected 0, got %d\n", rect.right);
    ok(0 == rect.top, "expected 0, got %d\n", rect.top);
    ok(rect.bottom, "rect.bottom should not be 0\n");

    SetRect(&rect, 0, 0, 0, 0);
    textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
    if (!textheight && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip( "DrawTextW not implemented\n" );
    }
    else
    {
        ok(textheight, "DrawTextW error %u\n", GetLastError());
        ok(0 == rect.left, "expected 0, got %d\n", rect.left);
        ok(0 == rect.right, "expected 0, got %d\n", rect.right);
        ok(0 == rect.top, "expected 0, got %d\n", rect.top);
        ok(rect.bottom, "rect.bottom should not be 0\n");
    }

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %u\n", GetLastError());

    /* Clean up */
    ret = ReleaseDC(hwnd, hdc);
    ok( ret, "ReleaseDC error %u\n", GetLastError());
    ret = DestroyWindow(hwnd);
    ok( ret, "DestroyWindow error %u\n", GetLastError());
}

/* replace tabs by \t */
static void strfmt( const char *str, char *strout)
{
    unsigned int i,j ;
    for(i=0,j=0;i<=strlen(str);i++,j++)
        if((strout[j]=str[i])=='\t') {
            strout[j++]='\\';
            strout[j]='t';
        }
}


#define TABTEST( tabval, tabcount, string, _exp) \
{ int i,x_act, x_exp; char strdisp[64];\
    for(i=0;i<8;i++) tabs[i]=(i+1)*(tabval); \
    extent = GetTabbedTextExtentA( hdc, string, strlen( string), (tabcount), tabs); \
    strfmt( string, strdisp); \
 /*   trace( "Extent is %08lx\n", extent); */\
    x_act = LOWORD( extent); \
    x_exp = (_exp); \
    ok( x_act == x_exp, "Test case \"%s\". Text extent is %d, expected %d tab %d tabcount %d\n", \
        strdisp, x_act, x_exp, tabval, tabcount); \
} \


static void test_TabbedText(void)
{
    HWND hwnd;
    HDC hdc;
    BOOL ret;
    TEXTMETRICA tm;
    DWORD extent;
    INT tabs[8], cx, cy, tab, tabcount,t,align;

    /* Initialization */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %u\n", GetLastError());

    ret = GetTextMetricsA( hdc, &tm);
    ok( ret, "GetTextMetrics error %u\n", GetLastError());

    extent = GetTabbedTextExtentA( hdc, "x", 1, 1, tabs);
    cx = LOWORD( extent);
    cy = HIWORD( extent);
    trace( "cx is %d cy is %d\n", cx, cy);

    align=1;
    for( t=-1; t<=1; t++) { /* slightly adjust the 4 char tabstop, to 
                               catch the one off errors */
        tab =  (cx *4 + t);
        /* test the special case tabcount =1 and the general array (80 of tabs */
        for( tabcount = 1; tabcount <= 8; tabcount +=7) { 
            TABTEST( align * tab, tabcount, "\t", tab)
            TABTEST( align * tab, tabcount, "xxx\t", tab)
            TABTEST( align * tab, tabcount, "\tx", tab+cx)
            TABTEST( align * tab, tabcount, "\t\t", tab*2)
            TABTEST( align * tab, tabcount, "\tx\t", tab*2)
            TABTEST( align * tab, tabcount, "x\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xx\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xxx\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xxxx\tx", t>0 ? tab + cx : 2*tab+cx)
            TABTEST( align * tab, tabcount, "xxxxx\tx", 2*tab+cx)
        }
    }
    align=-1;
    for( t=-1; t<=1; t++) { /* slightly adjust the 4 char tabstop, to 
                               catch the one off errors */
        tab =  (cx *4 + t);
        /* test the special case tabcount =1 and the general array (8) of tabs */
        for( tabcount = 1; tabcount <= 8; tabcount +=7) { 
            TABTEST( align * tab, tabcount, "\t", tab)
            TABTEST( align * tab, tabcount, "xxx\t", tab)
            TABTEST( align * tab, tabcount, "\tx", tab)
            TABTEST( align * tab, tabcount, "\t\t", tab*2)
            TABTEST( align * tab, tabcount, "\tx\t", tab*2)
            TABTEST( align * tab, tabcount, "x\tx", tab)
            TABTEST( align * tab, tabcount, "xx\tx", tab)
            TABTEST( align * tab, tabcount, "xxx\tx", 4 * cx >= tab ? 2*tab :tab)
            TABTEST( align * tab, tabcount, "xxxx\tx", 2*tab)
            TABTEST( align * tab, tabcount, "xxxxx\tx", 2*tab)
        }
    }

    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );
}

static void test_DrawState(void)
{
    static const char text[] = "Sample text string";
    HWND hwnd;
    HDC hdc;
    BOOL ret;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    assert(hwnd);

    hdc = GetDC(hwnd);
    assert(hdc);

    SetLastError(0xdeadbeef);
    ret = DrawState(hdc, GetStockObject(DKGRAY_BRUSH), NULL, (LPARAM)text, strlen(text),
                    0, 0, 10, 10, DST_TEXT);
    ok(ret, "DrawState error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawState(hdc, GetStockObject(DKGRAY_BRUSH), NULL, (LPARAM)text, 0,
                    0, 0, 10, 10, DST_TEXT);
    ok(ret, "DrawState error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawState(hdc, GetStockObject(DKGRAY_BRUSH), NULL, 0, strlen(text),
                    0, 0, 10, 10, DST_TEXT);
    ok(!ret || broken(ret) /* win98 */, "DrawState succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "not expected error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawState(hdc, GetStockObject(DKGRAY_BRUSH), NULL, 0, 0,
                    0, 0, 10, 10, DST_TEXT);
    ok(!ret || broken(ret) /* win98 */, "DrawState succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "not expected error %u\n", GetLastError());

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

START_TEST(text)
{
    test_TabbedText();
    test_DrawTextCalcRect();
    test_DrawState();
}
