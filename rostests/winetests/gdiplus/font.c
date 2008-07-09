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

static const WCHAR arial[] = {'A','r','i','a','l','\0'};
static const WCHAR nonexistant[] = {'T','h','i','s','F','o','n','t','s','h','o','u','l','d','N','o','t','E','x','i','s','t','\0'};
static const WCHAR MSSansSerif[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'};
static const WCHAR MicrosoftSansSerif[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f','\0'};
static const WCHAR TimesNewRoman[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n','\0'};
static const WCHAR CourierNew[] = {'C','o','u','r','i','e','r',' ','N','e','w','\0'};

static const char *debugstr_w(LPCWSTR str)
{
   static char buf[1024];
   WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
   return buf;
}


static void test_createfont(void)
{
    GpFontFamily* fontfamily = NULL;
    GpFont* font = NULL;
    GpStatus stat;
    Unit unit;
    UINT i;
    REAL size;

    stat = GdipCreateFontFamilyFromName(nonexistant, NULL, &fontfamily);
    expect (FontFamilyNotFound, stat);
    stat = GdipDeleteFont(font);
    expect (InvalidParameter, stat);
    stat = GdipCreateFontFamilyFromName(arial, NULL, &fontfamily);
    expect (Ok, stat);
    stat = GdipCreateFont(fontfamily, 12, FontStyleRegular, UnitPoint, &font);
    expect (Ok, stat);
    stat = GdipGetFontUnit (font, &unit);
    expect (Ok, stat);
    expect (UnitPoint, unit);

    /* Test to see if returned size is based on unit (its not) */
    GdipGetFontSize(font, &size);
    ok (size == 12, "Expected 12, got %f\n", size);
    GdipDeleteFont(font);

    /* Make sure everything is converted correctly for all Units */
    for (i = UnitWorld; i <=UnitMillimeter; i++)
    {
        if (i == UnitDisplay) continue; /* Crashes WindowsXP, wtf? */
        GdipCreateFont(fontfamily, 24, FontStyleRegular, i, &font);
        GdipGetFontSize (font, &size);
        ok (size == 24, "Expected 24, got %f (with unit: %d)\n", size, i);
        GdipGetFontUnit (font, &unit);
        expect (i, unit);
        GdipDeleteFont(font);
    }
}

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

static void test_fontfamily (void)
{
    GpFontFamily** family = NULL;
    WCHAR itsName[LF_FACESIZE];
    GpStatus stat;

    /* FontFamily can not be NULL */
    stat = GdipCreateFontFamilyFromName (arial , NULL, family);
    expect (InvalidParameter, stat);

    family = GdipAlloc (sizeof (GpFontFamily*));

    /* FontFamily must be able to actually find the family.
     * If it can't, any subsequent calls should fail
     *
     * We currently fail (meaning we don't) because we don't actually
     * test to see if we can successfully get a family
     */
    stat = GdipCreateFontFamilyFromName (nonexistant, NULL, family);
    expect (FontFamilyNotFound, stat);
    stat = GdipGetFamilyName (*family,itsName, LANG_NEUTRAL);
    expect (InvalidParameter, stat);
    ok ((lstrcmpiW(itsName,nonexistant) != 0),
        "Expected a non-zero value for nonexistant font!\n");
    stat = GdipDeleteFontFamily(*family);
    expect (InvalidParameter, stat);

    stat = GdipCreateFontFamilyFromName (arial, NULL, family);
    expect (Ok, stat);

    stat = GdipGetFamilyName (*family, itsName, LANG_NEUTRAL);
    expect (Ok, stat);
    expect (0, lstrcmpiW(itsName,arial));

    if (0)
    {
        /* Crashes on Windows XP SP2, Vista, and so Wine as well */
        stat = GdipGetFamilyName (*family, NULL, LANG_NEUTRAL);
        expect (Ok, stat);
    }

    stat = GdipDeleteFontFamily(*family);
    expect (Ok, stat);

    GdipFree (family);
}


static void test_getgenerics (void)
{
    GpStatus stat;
    GpFontFamily** family;
    WCHAR familyName[LF_FACESIZE];
    ZeroMemory(familyName, sizeof(familyName)/sizeof(WCHAR));

    family = GdipAlloc (sizeof (GpFontFamily*));

    stat = GdipGetGenericFontFamilySansSerif (family);
    expect (Ok, stat);
    stat = GdipGetFamilyName (*family, familyName, LANG_NEUTRAL);
    expect (Ok, stat);
    ok ((lstrcmpiW(familyName, MicrosoftSansSerif) == 0) ||
        (lstrcmpiW(familyName,MSSansSerif) == 0),
        "Expected Microsoft Sans Serif or MS Sans Serif, got %s\n",
        debugstr_w(familyName));
    stat = GdipDeleteFontFamily (*family);
    expect (Ok, stat);

    stat = GdipGetGenericFontFamilySerif (family);
    expect (Ok, stat);
    stat = GdipGetFamilyName (*family, familyName, LANG_NEUTRAL);
    expect (Ok, stat);
    ok (lstrcmpiW(familyName, TimesNewRoman) == 0,
        "Expected Times New Roman, got %s\n", debugstr_w(familyName));
    stat = GdipDeleteFontFamily (*family);
    expect (Ok, stat);

    stat = GdipGetGenericFontFamilyMonospace (family);
    expect (Ok, stat);
    stat = GdipGetFamilyName (*family, familyName, LANG_NEUTRAL);
    expect (Ok, stat);
    ok (lstrcmpiW(familyName, CourierNew) == 0,
        "Expected Courier New, got %s\n", debugstr_w(familyName));
    stat = GdipDeleteFontFamily (*family);
    expect (Ok, stat);

    GdipFree (family);
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

    test_createfont();
    test_logfont();
    test_fontfamily();
    test_getgenerics();

    GdiplusShutdown(gdiplusToken);
}
