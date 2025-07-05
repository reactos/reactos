/*
 * DrawText tests
 *
 * Copyright (c) 2004 Zach Gorman
 * Copyright 2007,2016 Dmitry Timoshkov
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
#include "winnls.h"

#define MODIFIED(rect) (rect.left == 10 && rect.right != 100 && rect.top == 10 && rect.bottom != 100)
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
    static CHAR wordbreak_text[] = "line1 line2";
    static WCHAR wordbreak_textW[] = {'l','i','n','e','1',' ','l','i','n','e','2',0};
    static WCHAR wordbreak_text_colonW[] = {'l','i','n','e','1',' ','l','i','n','e','2',' ',':',0};
    static WCHAR wordbreak_text_csbW[] = {'l','i','n','e','1',' ','l','i','n','e','2',' ',']',0};
    static char tabstring[] = "one\ttwo";
    INT textlen, textheight, heightcheck;
    RECT rect = { 0, 0, 100, 0 }, rect2;
    BOOL ret;
    DRAWTEXTPARAMS dtp;
    BOOL conform_xp = TRUE;

    /* Initialization */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %lu\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %lu\n", GetLastError());
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
    ok(hFont != 0, "CreateFontIndirectA error %lu\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %lu\n", GetLastError());

    trace("MM_HIENGLISH rect.bottom %ld\n", rect.bottom);
    ok(rect.bottom < 0, "In MM_HIENGLISH, DrawText with "
       "DT_CALCRECT should return a negative rectangle bottom. "
       "(bot=%ld)\n", rect.bottom);

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %lu\n", GetLastError());


    /* DrawText in MM_TEXT with DT_CALCRECT */
    SetMapMode(hdc, MM_TEXT);
    lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc,
       LOGPIXELSY), 72); /* 9 point */
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != 0, "CreateFontIndirectA error %lu\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %lu\n", GetLastError());

    trace("MM_TEXT rect.bottom %ld\n", rect.bottom);
    ok(rect.bottom > 0, "In MM_TEXT, DrawText with DT_CALCRECT "
       "should return a positive rectangle bottom. (bot=%ld)\n",
       rect.bottom);

    /* empty or null text should in some cases calc an empty rectangle */

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, text, 0, &rect, DT_CALCRECT, NULL );
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, text, 0, &rect, DT_CALCRECT);
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, emptystring, -1, &rect, DT_CALCRECT, NULL );
    ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
    ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT);
    ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
    ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
    ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
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
    ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, 0, &rect, DT_CALCRECT);
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    /* DT_SINGLELINE tests */

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, text, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, text, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, emptystring, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok(!EMPTY(rect) && MODIFIED(rect), "rectangle should be modified got %s\n",
       wine_dbgstr_rect(&rect));
    ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok(!EMPTY(rect) && MODIFIED (rect), "rectangle should be modified got %s\n",
       wine_dbgstr_rect(&rect));
    ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExA(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok(!EMPTY(rect) && MODIFIED(rect), "rectangle should be modified got %s\n",
       wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok(!EMPTY(rect) && MODIFIED(rect), "rectangle should be modified got %s\n",
       wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight!=0,"Failed to get textheight from DrawTextA\n");
    ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

    SetRect( &rect, 10,10, 100, 100);
    heightcheck = textheight = DrawTextExA(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
    if (conform_xp)
        ok(textheight==0,"Got textheight from DrawTextExA\n");

    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextA(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
    ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
        "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
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

    /* When offset to top is zero, return 1 */
    SetRectEmpty(&rect);
    textheight = DrawTextExW(hdc, textW, -1, &rect, DT_SINGLELINE | DT_CALCRECT | DT_BOTTOM, NULL);
    ok(textheight == 1, "Expect returned height:1 got:%d\n", textheight);

    SetRect(&rect, 0, 100, 0, 100);
    textheight = DrawTextExW(hdc, textW, -1, &rect, DT_SINGLELINE | DT_CALCRECT | DT_BOTTOM, NULL);
    ok(textheight == 1, "Expect returned height:1 got:%d\n", textheight);

    SetRectEmpty(&rect);
    textheight = DrawTextExW(hdc, textW, -1, &rect, DT_SINGLELINE | DT_CALCRECT | DT_TOP, NULL);
    /* Set top to text height and bottom zero, so bottom of drawn text to top is zero when DT_VCENTER is used */
    SetRect(&rect, 0, textheight, 0, 0);
    textheight = DrawTextExW(hdc, textW, -1, &rect, DT_SINGLELINE | DT_CALCRECT | DT_VCENTER, NULL);
    ok(textheight == 1, "Expect returned height:1 got:%d\n", textheight);

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
    SetRectEmpty(&rect);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    textlen = rect.right; /* Width without margin */
    dtp.iLeftMargin = 8;
    SetRectEmpty(&rect);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    ok(rect.right==dtp.iLeftMargin+textlen  ,"Incorrect left margin calculated  rc(%ld,%ld)\n", rect.left, rect.right);
    dtp.iLeftMargin = 0;
    dtp.iRightMargin = 8;
    SetRectEmpty(&rect);
    DrawTextExA(hdc, text, -1, &rect, DT_CALCRECT, &dtp);
    ok(rect.right==dtp.iRightMargin+textlen  ,"Incorrect right margin calculated rc(%ld,%ld)\n", rect.left, rect.right);

    /* Wide char versions */
    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    heightcheck = textheight = DrawTextExW(hdc, textW, 0, &rect, DT_CALCRECT, NULL );
    if( GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) {
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, textW, 0, &rect, DT_CALCRECT);
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, emptystringW, -1, &rect, DT_CALCRECT, NULL );
        ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT);
        ok(EMPTY(rect), "rectangle should be empty got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
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
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        if (conform_xp) {
            /* Crashes on NT4 */
            SetRect( &rect, 10,10, 100, 100);
            heightcheck = textheight = DrawTextExW(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
            ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
            ok(textheight==0,"Got textheight from DrawTextExW\n");

            SetRect( &rect, 10,10, 100, 100);
            textheight = DrawTextW(hdc, NULL, -1, &rect, DT_CALCRECT);
            ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
            ok(textheight==0,"Got textheight from DrawTextW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        }


        /* DT_SINGLELINE tests */

        heightcheck = textheight = DrawTextExW(hdc, textW, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, textW, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, emptystringW, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok(!EMPTY(rect) && MODIFIED(rect), "rectangle should be modified got %s\n",
           wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok(!EMPTY(rect) && MODIFIED(rect), "rectangle should be modified got %s\n",
           wine_dbgstr_rect(&rect));
        ok(textheight!=0,"Failed to get textheight from DrawTextW\n");
        ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");

        if (conform_xp) {
            /* Crashes on NT4 */
            SetRect( &rect, 10,10, 100, 100);
            heightcheck = textheight = DrawTextExW(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
            ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
            ok(textheight==0,"Got textheight from DrawTextExW\n");

            SetRect( &rect, 10,10, 100, 100);
            textheight = DrawTextW(hdc, NULL, -1, &rect, DT_CALCRECT|DT_SINGLELINE);
            ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
                "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
            ok(textheight==0,"Got textheight from DrawTextW\n");
            ok(textheight == heightcheck,"DrawTextEx and DrawText differ in return\n");
        }

        SetRect( &rect, 10,10, 100, 100);
        heightcheck = textheight = DrawTextExW(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE, NULL );
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
        if (conform_xp)
            ok(textheight==0,"Got textheight from DrawTextExW\n");

        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextW(hdc, NULL, 0, &rect, DT_CALCRECT|DT_SINGLELINE);
        ok(!IsRectEmpty(&rect) && !MODIFIED(rect),
            "rectangle should NOT be empty and NOT modified got %s\n", wine_dbgstr_rect(&rect));
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

        /* When passing invalid DC, other parameters must be ignored - no crashes on invalid pointers */

        SetLastError(0xdeadbeef);
        textheight = DrawTextExW((HDC)0xdeadbeef, (LPWSTR)0xdeadbeef, 100000, &rect, 0, 0);
        ok(textheight == 0, "Got textheight from DrawTextExW\n");
        ok(GetLastError() == 0xdeadbeef,"Got error %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        textheight = DrawTextExW((HDC)0xdeadbeef, 0, -1, (LPRECT)0xdeadbeef, DT_CALCRECT, 0);
        ok(textheight == 0, "Got textheight from DrawTextExW\n");
        ok(GetLastError() == 0xdeadbeef,"Got error %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        textheight = DrawTextExA((HDC)0xdeadbeef, 0, -1, (LPRECT)0xdeadbeef, DT_CALCRECT, 0);
        ok(textheight == 0, "Got textheight from DrawTextExA\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_HANDLE,"Got error %lu\n", GetLastError());

        if (0)
        {
            /* Crashes */
            textheight = DrawTextExA((HDC)0xdeadbeef, (LPSTR)0xdeadbeef, 100, &rect, 0, 0);
        }
    }

    /* More test cases from bug 12226 */
    SetRectEmpty(&rect);
    textheight = DrawTextA(hdc, emptystring, -1, &rect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
    ok(textheight, "DrawTextA error %lu\n", GetLastError());
    ok(0 == rect.left, "expected 0, got %ld\n", rect.left);
    ok(0 == rect.right, "expected 0, got %ld\n", rect.right);
    ok(0 == rect.top, "expected 0, got %ld\n", rect.top);
    ok(rect.bottom, "rect.bottom should not be 0\n");

    SetRectEmpty(&rect);
    textheight = DrawTextW(hdc, emptystringW, -1, &rect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
    if (!textheight && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip( "DrawTextW not implemented\n" );
    }
    else
    {
        ok(textheight, "DrawTextW error %lu\n", GetLastError());
        ok(0 == rect.left, "expected 0, got %ld\n", rect.left);
        ok(0 == rect.right, "expected 0, got %ld\n", rect.right);
        ok(0 == rect.top, "expected 0, got %ld\n", rect.top);
        ok(rect.bottom, "rect.bottom should not be 0\n");
    }

    SetRect(&rect, 0, 0, 1, 1);
    heightcheck = DrawTextA(hdc, wordbreak_text, -1, &rect, DT_CALCRECT);
    SetRect(&rect, 0, 0, 1, 1);
    textheight = DrawTextA(hdc, wordbreak_text, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck * 2, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck * 2);
    SetRect(&rect, 0, 0, 1, 1);
    textheight = DrawTextA(hdc, wordbreak_text, -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL);
    ok(textheight >= heightcheck * 6, "Got unexpected textheight %d, expected at least %d.\n",
       textheight, heightcheck * 6);

    SetRect(&rect, 0, 0, 1, 1);
    heightcheck = DrawTextW(hdc, wordbreak_textW, -1, &rect, DT_CALCRECT);
    SetRect(&rect, 0, 0, 1, 1);
    textheight = DrawTextW(hdc, wordbreak_textW, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck * 2, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck * 2);
    SetRect(&rect, 0, 0, 1, 1);
    textheight = DrawTextW(hdc, wordbreak_textW, -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL);
    ok(textheight >= heightcheck * 6, "Got unexpected textheight %d, expected at least %d.\n",
       textheight, heightcheck * 6);

    /* Word break tests with space before punctuation */
    SetRect(&rect, 0, 0, 200, 1);
    textheight = DrawTextW(hdc, wordbreak_text_colonW, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck);

    rect2 = rect;
    rect.right--;

    textheight = DrawTextW(hdc, wordbreak_text_colonW, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck * 2, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck * 2);
    ok(rect.right > rect2.right - 10, "Got unexpected textwdith %ld, expected larger than %ld.\n",
       rect.right, rect2.right - 10);

    SetRect(&rect, 0, 0, 200, 1);
    textheight = DrawTextW(hdc, wordbreak_text_csbW, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck);

    rect2 = rect;
    rect.right--;

    textheight = DrawTextW(hdc, wordbreak_text_csbW, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ok(textheight == heightcheck * 2, "Got unexpected textheight %d, expected %d.\n",
       textheight, heightcheck * 2);
    ok(rect.right > rect2.right - 10, "Got unexpected textwdith %ld, expected larger than %ld.\n",
       rect.right, rect2.right - 10);


    /* DT_TABSTOP | DT_EXPANDTABS tests */
    SetRect( &rect, 0,0, 10, 10);
    textheight = DrawTextA(hdc, tabstring, -1, &rect, DT_TABSTOP | DT_EXPANDTABS );
    ok(textheight >= heightcheck, "Got unexpected textheight %d\n", textheight);

    SetRect( &rect, 0,0, 10, 10);
    memset(&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    textheight = DrawTextExA(hdc, tabstring, -1, &rect, DT_CALCRECT, &dtp);
    ok(textheight >= heightcheck, "Got unexpected textheight %d\n", textheight);
    ok(dtp.iTabLength == 0, "invalid dtp.iTabLength = %i\n",dtp.iTabLength);

    SetRect( &rect2, 0,0, 10, 10);
    memset(&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    textheight = DrawTextExA(hdc, tabstring, -1, &rect2, DT_CALCRECT | DT_TABSTOP | DT_EXPANDTABS, &dtp);
    ok(textheight >= heightcheck, "Got unexpected textheight %d\n", textheight);
    ok(dtp.iTabLength == 0, "invalid dtp.iTabLength = %i\n",dtp.iTabLength);
    ok(rect.left == rect2.left && rect.right != rect2.right && rect.top == rect2.top && rect.bottom == rect2.bottom,
       "incorrect rect %s rect2 %s\n", wine_dbgstr_rect(&rect), wine_dbgstr_rect(&rect2));

    SetRect( &rect, 0,0, 10, 10);
    memset(&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    dtp.iTabLength = 8;
    textheight = DrawTextExA(hdc, tabstring, -1, &rect, DT_CALCRECT | DT_TABSTOP | DT_EXPANDTABS, &dtp);
    ok(textheight >= heightcheck, "Got unexpected textheight %d\n", textheight);
    ok(dtp.iTabLength == 8, "invalid dtp.iTabLength = %i\n",dtp.iTabLength);
    ok(rect.left == rect2.left, "unexpected value %ld, got %ld\n", rect.left, rect2.left);
    /* XP, 2003 appear to not give the same values. */
    ok(rect.right == rect2.right || broken(rect.right > rect2.right), "unexpected value %ld, got %ld\n",rect.right, rect2.right);
    ok(rect.top == rect2.top, "unexpected value %ld, got %ld\n", rect.top, rect2.top);
    ok(rect.bottom == rect2.bottom , "unexpected value %ld, got %ld\n", rect.bottom, rect2.bottom);


    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %lu\n", GetLastError());

    /* Clean up */
    ret = ReleaseDC(hwnd, hdc);
    ok( ret, "ReleaseDC error %lu\n", GetLastError());
    ret = DestroyWindow(hwnd);
    ok( ret, "DestroyWindow error %lu\n", GetLastError());
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
{ int i; char strdisp[64];\
    for(i=0;i<8;i++) tabs[i]=(i+1)*(tabval); \
    extent = GetTabbedTextExtentA( hdc, string, strlen( string), (tabcount), tabs); \
    strfmt( string, strdisp); \
 /*   trace( "Extent is %08lx\n", extent); */\
    ok( extent == _exp, "Test case \"%s\". Text extent is 0x%lx, expected 0x%lx tab %d tabcount %d\n", \
        strdisp, extent, _exp, tabval, tabcount); \
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
    ok(hwnd != 0, "CreateWindowExA error %lu\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %lu\n", GetLastError());

    ret = GetTextMetricsA( hdc, &tm);
    ok( ret, "GetTextMetrics error %lu\n", GetLastError());

    extent = GetTabbedTextExtentA( hdc, "x", 0, 1, tabs);
    ok( extent == 0, "GetTabbedTextExtentA returned non-zero on nCount == 0\n");

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
            TABTEST( align * tab, tabcount, "\t", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "xxx\t", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "\tx", MAKELONG(tab+cx, cy))
            TABTEST( align * tab, tabcount, "\t\t", MAKELONG(tab*2, cy))
            TABTEST( align * tab, tabcount, "\tx\t", MAKELONG(tab*2, cy))
            TABTEST( align * tab, tabcount, "x\tx", MAKELONG(tab+cx, cy))
            TABTEST( align * tab, tabcount, "xx\tx", MAKELONG(tab+cx, cy))
            TABTEST( align * tab, tabcount, "xxx\tx", MAKELONG(tab+cx, cy))
            TABTEST( align * tab, tabcount, "xxxx\tx", MAKELONG(t>0 ? tab + cx : 2*tab+cx, cy))
            TABTEST( align * tab, tabcount, "xxxxx\tx", MAKELONG(2*tab+cx, cy))
        }
    }
    align=-1;
    for( t=-1; t<=1; t++) { /* slightly adjust the 4 char tabstop, to 
                               catch the one off errors */
        tab =  (cx *4 + t);
        /* test the special case tabcount =1 and the general array (8) of tabs */
        for( tabcount = 1; tabcount <= 8; tabcount +=7) { 
            TABTEST( align * tab, tabcount, "\t", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "xxx\t", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "\tx", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "\t\t", MAKELONG(tab*2, cy))
            TABTEST( align * tab, tabcount, "\tx\t", MAKELONG(tab*2, cy))
            TABTEST( align * tab, tabcount, "x\tx", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "xx\tx", MAKELONG(tab, cy))
            TABTEST( align * tab, tabcount, "xxx\tx", MAKELONG(4 * cx >= tab ? 2*tab :tab, cy))
            TABTEST( align * tab, tabcount, "xxxx\tx", MAKELONG(2*tab, cy))
            TABTEST( align * tab, tabcount, "xxxxx\tx", MAKELONG(2*tab, cy))
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
    ret = DrawStateA(hdc, GetStockObject(DKGRAY_BRUSH), NULL, (LPARAM)text, strlen(text),
                    0, 0, 10, 10, DST_TEXT);
    ok(ret, "DrawState error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawStateA(hdc, GetStockObject(DKGRAY_BRUSH), NULL, (LPARAM)text, 0,
                    0, 0, 10, 10, DST_TEXT);
    ok(ret, "DrawState error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawStateA(hdc, GetStockObject(DKGRAY_BRUSH), NULL, 0, strlen(text),
                    0, 0, 10, 10, DST_TEXT);
    ok(!ret || broken(ret) /* win98 */, "DrawState succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "not expected error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DrawStateA(hdc, GetStockObject(DKGRAY_BRUSH), NULL, 0, 0,
                    0, 0, 10, 10, DST_TEXT);
    ok(!ret || broken(ret) /* win98 */, "DrawState succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "not expected error %lu\n", GetLastError());

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void test_CharToOem_OemToChar(void)
{
    static const WCHAR helloWorldW[] = {'H','e','l','l','o',' ','W','o','r','l','d',0};
    static const WCHAR emptyW[] = {0};
    static const char helloWorld[] = "Hello World";
    static const struct
    {
        BOOL src, dst, ret;
    }
    tests[] =
    {
        { FALSE, FALSE, FALSE },
        { TRUE,  FALSE, FALSE },
        { FALSE, TRUE,  FALSE },
        { TRUE,  TRUE,  TRUE  },
    };
    BOOL ret;
    int i;
    char oem;
    WCHAR uni, expect;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const char *expected = tests[i].ret ? helloWorld : "";
        const char *src = tests[i].src ? helloWorld : NULL;
        char buf[64], *dst = tests[i].dst ? buf : NULL;

        memset(buf, 0, sizeof(buf));
        ret = CharToOemA(src, dst);
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);

        memset(buf, 0, sizeof(buf));
        ret = CharToOemBuffA(src, dst, sizeof(helloWorld));
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);

        memset(buf, 0, sizeof(buf));
        ret = OemToCharA(src, dst);
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);

        memset(buf, 0, sizeof(buf));
        ret = OemToCharBuffA(src, dst, sizeof(helloWorld));
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const char *expected = tests[i].ret ? helloWorld : "";
        const WCHAR *src = tests[i].src ? helloWorldW : NULL;
        char buf[64], *dst = tests[i].dst ? buf : NULL;

        memset(buf, 0, sizeof(buf));
        ret = CharToOemW(src, dst);
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);

        memset(buf, 0, sizeof(buf));
        ret = CharToOemBuffW(src, dst, ARRAY_SIZE(helloWorldW));
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!strcmp(buf, expected), "test %d: got '%s'\n", i, buf);
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const WCHAR *expected = tests[i].ret ? helloWorldW : emptyW;
        const char *src = tests[i].src ? helloWorld : NULL;
        WCHAR buf[64], *dst = tests[i].dst ? buf : NULL;

        memset(buf, 0, sizeof(buf));
        ret = OemToCharW(src, dst);
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!lstrcmpW(buf, expected), "test %d: got '%s'\n", i, wine_dbgstr_w(buf));

        memset(buf, 0, sizeof(buf));
        ret = OemToCharBuffW(src, dst, sizeof(helloWorld));
        ok(ret == tests[i].ret, "test %d: expected %d, got %d\n", i, tests[i].ret, ret);
        ok(!lstrcmpW(buf, expected), "test %d: got '%s'\n", i, wine_dbgstr_w(buf));
    }

    for (i = 0; i < 0x100; i++)
    {
        oem = i;
        ret = OemToCharBuffW( &oem, &uni, 1 );
        ok( ret, "%02x: returns FALSE\n", i );
        MultiByteToWideChar( CP_OEMCP, MB_PRECOMPOSED | MB_USEGLYPHCHARS, &oem, 1, &expect, 1 );
        ok( uni == expect, "%02x: got %04x expected %04x\n", i, uni, expect );
    }
}

START_TEST(text)
{
    test_TabbedText();
    test_DrawTextCalcRect();
    test_DrawState();
    test_CharToOem_OemToChar();
}
