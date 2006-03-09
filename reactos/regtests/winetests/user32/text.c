/*
 * DrawText tests
 *
 * Copyright (c) 2004 Zach Gorman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <assert.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"


static void test_DrawTextCalcRect(void)
{
    HWND hwnd;
    HDC hdc;
    HFONT hFont, hOldFont;
    LOGFONTA lf;
    const char text[] = "Example text for testing DrawText in "
      "MM_HIENGLISH mode";
    INT textlen,textheight;
    RECT rect = { 0, 0, 100, 0 };
    BOOL ret;

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
    todo_wine ok(rect.bottom < 0, "In MM_HIENGLISH, DrawText with "
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

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %lu\n", GetLastError());

    /* Clean up */
    ret = ReleaseDC(hwnd, hdc);
    ok( ret, "ReleaseDC error %lu\n", GetLastError());
    ret = DestroyWindow(hwnd);
    ok( ret, "DestroyWindow error %lu\n", GetLastError());
}

START_TEST(text)
{
    test_DrawTextCalcRect();
}
