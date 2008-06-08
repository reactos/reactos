/*
 * Unit test suite for fonts
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static WCHAR arial[] = {'A','r','i','a','l','\0'};

static void test_logfont(void)
{
    LOGFONTW lfw, lfw2;
    GpFont *font;
    GpStatus stat;
    GpGraphics *graphics;
    HDC hdc = GetDC(0);

    GdipCreateFromHDC(hdc, &graphics);
    memset(&lfw, 0, sizeof(LOGFONTW));
    memset(&lfw2, 0xff, sizeof(LOGFONTW));
    memcpy(&lfw.lfFaceName, arial, 6 * sizeof(WCHAR));

    stat = GdipCreateFontFromLogfontW(hdc, &lfw, &font);
    expect(Ok, stat);
    stat = GdipGetLogFontW(font, graphics, &lfw2);
    expect(Ok, stat);

    ok(lfw2.lfHeight < 0, "Expected negative height\n");
    expect(0, lfw2.lfWidth);
    expect(0, lfw2.lfEscapement);
    expect(0, lfw2.lfOrientation);
    ok((lfw2.lfWeight >= 100) && (lfw2.lfWeight <= 900), "Expected weight to be set\n");
    expect(0, lfw2.lfItalic);
    expect(0, lfw2.lfUnderline);
    expect(0, lfw2.lfStrikeOut);
    expect(0, lfw2.lfCharSet);
    expect(0, lfw2.lfOutPrecision);
    expect(0, lfw2.lfClipPrecision);
    expect(0, lfw2.lfQuality);
    expect(0, lfw2.lfPitchAndFamily);

    GdipDeleteFont(font);

    memset(&lfw, 0, sizeof(LOGFONTW));
    lfw.lfHeight = 25;
    lfw.lfWidth = 25;
    lfw.lfEscapement = lfw.lfOrientation = 50;
    lfw.lfItalic = lfw.lfUnderline = lfw.lfStrikeOut = TRUE;

    memset(&lfw2, 0xff, sizeof(LOGFONTW));
    memcpy(&lfw.lfFaceName, arial, 6 * sizeof(WCHAR));

    stat = GdipCreateFontFromLogfontW(hdc, &lfw, &font);
    expect(Ok, stat);
    stat = GdipGetLogFontW(font, graphics, &lfw2);
    expect(Ok, stat);

    ok(lfw2.lfHeight < 0, "Expected negative height\n");
    expect(0, lfw2.lfWidth);
    expect(0, lfw2.lfEscapement);
    expect(0, lfw2.lfOrientation);
    ok((lfw2.lfWeight >= 100) && (lfw2.lfWeight <= 900), "Expected weight to be set\n");
    expect(TRUE, lfw2.lfItalic);
    expect(TRUE, lfw2.lfUnderline);
    expect(TRUE, lfw2.lfStrikeOut);
    expect(0, lfw2.lfCharSet);
    expect(0, lfw2.lfOutPrecision);
    expect(0, lfw2.lfClipPrecision);
    expect(0, lfw2.lfQuality);
    expect(0, lfw2.lfPitchAndFamily);

    GdipDeleteFont(font);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

START_TEST(font)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_logfont();

    GdiplusShutdown(gdiplusToken);
}
