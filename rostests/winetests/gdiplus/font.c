/*
 * Unit test suite for fonts
 *
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2012 Dmitry Timoshkov
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

#include <math.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

//#include "windows.h"
#include <wine/test.h>
#include <wingdi.h>
#include <winnls.h>
#include <objbase.h>
#include <gdiplus.h>

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define expect_(expected, got, precision) ok(abs((expected) - (got)) <= (precision), "Expected %d, got %d\n", (expected), (got))
#define expectf_(expected, got, precision) ok(fabs((expected) - (got)) <= (precision), "Expected %f, got %f\n", (expected), (got))
#define expectf(expected, got) expectf_((expected), (got), 0.001)

static const WCHAR nonexistent[] = {'T','h','i','s','F','o','n','t','s','h','o','u','l','d','N','o','t','E','x','i','s','t','\0'};
static const WCHAR MSSansSerif[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'};
static const WCHAR TimesNewRoman[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n','\0'};
static const WCHAR Tahoma[] = {'T','a','h','o','m','a',0};

static void set_rect_empty(RectF *rc)
{
    rc->X = 0.0;
    rc->Y = 0.0;
    rc->Width = 0.0;
    rc->Height = 0.0;
}

static void test_createfont(void)
{
    GpFontFamily* fontfamily = NULL, *fontfamily2;
    GpFont* font = NULL;
    GpStatus stat;
    Unit unit;
    UINT i;
    REAL size;
    WCHAR familyname[LF_FACESIZE];

    stat = GdipCreateFontFamilyFromName(nonexistent, NULL, &fontfamily);
    expect (FontFamilyNotFound, stat);
    stat = GdipDeleteFont(font);
    expect (InvalidParameter, stat);
    stat = GdipCreateFontFamilyFromName(Tahoma, NULL, &fontfamily);
    expect (Ok, stat);
    stat = GdipCreateFont(fontfamily, 12, FontStyleRegular, UnitPoint, &font);
    expect (Ok, stat);
    stat = GdipGetFontUnit (font, &unit);
    expect (Ok, stat);
    expect (UnitPoint, unit);

    stat = GdipGetFamily(font, &fontfamily2);
    expect(Ok, stat);
    stat = GdipGetFamilyName(fontfamily2, familyname, 0);
    expect(Ok, stat);
    ok (lstrcmpiW(Tahoma, familyname) == 0, "Expected Tahoma, got %s\n",
            wine_dbgstr_w(familyname));
    stat = GdipDeleteFontFamily(fontfamily2);
    expect(Ok, stat);

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

    GdipDeleteFontFamily(fontfamily);
}

static void test_logfont(void)
{
    LOGFONTA lfa, lfa2;
    GpFont *font;
    GpFontFamily *family;
    GpStatus stat;
    GpGraphics *graphics;
    HDC hdc = GetDC(0);
    INT style;
    REAL rval;
    UINT16 em_height, line_spacing;
    Unit unit;

    GdipCreateFromHDC(hdc, &graphics);

    memset(&lfa, 0, sizeof(LOGFONTA));
    memset(&lfa2, 0xff, sizeof(LOGFONTA));
    lstrcpyA(lfa.lfFaceName, "Tahoma");

    stat = GdipCreateFontFromLogfontA(hdc, &lfa, &font);
    expect(Ok, stat);
    stat = GdipGetLogFontA(font, graphics, &lfa2);
    expect(Ok, stat);

    ok(lfa2.lfHeight < 0, "Expected negative height\n");
    expect(0, lfa2.lfWidth);
    expect(0, lfa2.lfEscapement);
    expect(0, lfa2.lfOrientation);
    ok((lfa2.lfWeight >= 100) && (lfa2.lfWeight <= 900), "Expected weight to be set\n");
    expect(0, lfa2.lfItalic);
    expect(0, lfa2.lfUnderline);
    expect(0, lfa2.lfStrikeOut);
    ok(lfa2.lfCharSet == GetTextCharset(hdc) || lfa2.lfCharSet == ANSI_CHARSET,
        "Expected %x or %x, got %x\n", GetTextCharset(hdc), ANSI_CHARSET, lfa2.lfCharSet);
    expect(0, lfa2.lfOutPrecision);
    expect(0, lfa2.lfClipPrecision);
    expect(0, lfa2.lfQuality);
    expect(0, lfa2.lfPitchAndFamily);

    GdipDeleteFont(font);

    memset(&lfa, 0, sizeof(LOGFONTA));
    lfa.lfHeight = 25;
    lfa.lfWidth = 25;
    lfa.lfEscapement = lfa.lfOrientation = 50;
    lfa.lfItalic = lfa.lfUnderline = lfa.lfStrikeOut = TRUE;

    memset(&lfa2, 0xff, sizeof(LOGFONTA));
    lstrcpyA(lfa.lfFaceName, "Tahoma");

    stat = GdipCreateFontFromLogfontA(hdc, &lfa, &font);
    expect(Ok, stat);
    stat = GdipGetLogFontA(font, graphics, &lfa2);
    expect(Ok, stat);

    ok(lfa2.lfHeight < 0, "Expected negative height\n");
    expect(0, lfa2.lfWidth);
    expect(0, lfa2.lfEscapement);
    expect(0, lfa2.lfOrientation);
    ok((lfa2.lfWeight >= 100) && (lfa2.lfWeight <= 900), "Expected weight to be set\n");
    expect(TRUE, lfa2.lfItalic);
    expect(TRUE, lfa2.lfUnderline);
    expect(TRUE, lfa2.lfStrikeOut);
    ok(lfa2.lfCharSet == GetTextCharset(hdc) || lfa2.lfCharSet == ANSI_CHARSET,
        "Expected %x or %x, got %x\n", GetTextCharset(hdc), ANSI_CHARSET, lfa2.lfCharSet);
    expect(0, lfa2.lfOutPrecision);
    expect(0, lfa2.lfClipPrecision);
    expect(0, lfa2.lfQuality);
    expect(0, lfa2.lfPitchAndFamily);

    stat = GdipGetFontStyle(font, &style);
    expect(Ok, stat);
    ok (style == (FontStyleItalic | FontStyleUnderline | FontStyleStrikeout),
            "Expected , got %d\n", style);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    stat = GdipGetFontHeight(font, graphics, &rval);
    expect(Ok, stat);
    expectf(25.347656, rval);
    stat = GdipGetFontSize(font, &rval);
    expect(Ok, stat);
    expectf(21.0, rval);

    stat = GdipGetFamily(font, &family);
    expect(Ok, stat);
    stat = GdipGetEmHeight(family, FontStyleRegular, &em_height);
    expect(Ok, stat);
    expect(2048, em_height);
    stat = GdipGetLineSpacing(family, FontStyleRegular, &line_spacing);
    expect(Ok, stat);
    expect(2472, line_spacing);
    GdipDeleteFontFamily(family);

    GdipDeleteFont(font);

    memset(&lfa, 0, sizeof(lfa));
    lfa.lfHeight = -25;
    lstrcpyA(lfa.lfFaceName, "Tahoma");
    stat = GdipCreateFontFromLogfontA(hdc, &lfa, &font);
    expect(Ok, stat);
    memset(&lfa2, 0xff, sizeof(lfa2));
    stat = GdipGetLogFontA(font, graphics, &lfa2);
    expect(Ok, stat);
    expect(lfa.lfHeight, lfa2.lfHeight);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    stat = GdipGetFontHeight(font, graphics, &rval);
    expect(Ok, stat);
    expectf(30.175781, rval);
    stat = GdipGetFontSize(font, &rval);
    expect(Ok, stat);
    expectf(25.0, rval);

    stat = GdipGetFamily(font, &family);
    expect(Ok, stat);
    stat = GdipGetEmHeight(family, FontStyleRegular, &em_height);
    expect(Ok, stat);
    expect(2048, em_height);
    stat = GdipGetLineSpacing(family, FontStyleRegular, &line_spacing);
    expect(Ok, stat);
    expect(2472, line_spacing);
    GdipDeleteFontFamily(family);

    GdipDeleteFont(font);

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_fontfamily (void)
{
    GpFontFamily *family, *clonedFontFamily;
    WCHAR itsName[LF_FACESIZE];
    GpStatus stat;

    /* FontFamily cannot be NULL */
    stat = GdipCreateFontFamilyFromName (Tahoma , NULL, NULL);
    expect (InvalidParameter, stat);

    /* FontFamily must be able to actually find the family.
     * If it can't, any subsequent calls should fail.
     */
    stat = GdipCreateFontFamilyFromName (nonexistent, NULL, &family);
    expect (FontFamilyNotFound, stat);

    /* Bitmap fonts are not found */
    stat = GdipCreateFontFamilyFromName (MSSansSerif, NULL, &family);
    expect (FontFamilyNotFound, stat);
    if(stat == Ok) GdipDeleteFontFamily(family);

    stat = GdipCreateFontFamilyFromName (Tahoma, NULL, &family);
    expect (Ok, stat);

    stat = GdipGetFamilyName (family, itsName, LANG_NEUTRAL);
    expect (Ok, stat);
    expect (0, lstrcmpiW(itsName, Tahoma));

    if (0)
    {
        /* Crashes on Windows XP SP2, Vista, and so Wine as well */
        stat = GdipGetFamilyName (family, NULL, LANG_NEUTRAL);
        expect (Ok, stat);
    }

    /* Make sure we don't read old data */
    ZeroMemory (itsName, sizeof(itsName));
    stat = GdipCloneFontFamily(family, &clonedFontFamily);
    expect (Ok, stat);
    GdipDeleteFontFamily(family);
    stat = GdipGetFamilyName(clonedFontFamily, itsName, LANG_NEUTRAL);
    expect(Ok, stat);
    expect(0, lstrcmpiW(itsName, Tahoma));

    GdipDeleteFontFamily(clonedFontFamily);
}

static void test_fontfamily_properties (void)
{
    GpFontFamily* FontFamily = NULL;
    GpStatus stat;
    UINT16 result = 0;

    stat = GdipCreateFontFamilyFromName(Tahoma, NULL, &FontFamily);
    expect(Ok, stat);

    stat = GdipGetLineSpacing(FontFamily, FontStyleRegular, &result);
    expect(Ok, stat);
    ok (result == 2472, "Expected 2472, got %d\n", result);
    result = 0;
    stat = GdipGetEmHeight(FontFamily, FontStyleRegular, &result);
    expect(Ok, stat);
    ok(result == 2048, "Expected 2048, got %d\n", result);
    result = 0;
    stat = GdipGetCellAscent(FontFamily, FontStyleRegular, &result);
    expect(Ok, stat);
    ok(result == 2049, "Expected 2049, got %d\n", result);
    result = 0;
    stat = GdipGetCellDescent(FontFamily, FontStyleRegular, &result);
    expect(Ok, stat);
    ok(result == 423, "Expected 423, got %d\n", result);
    GdipDeleteFontFamily(FontFamily);

    stat = GdipCreateFontFamilyFromName(TimesNewRoman, NULL, &FontFamily);
    if(stat == FontFamilyNotFound)
        skip("Times New Roman not installed\n");
    else
    {
        result = 0;
        stat = GdipGetLineSpacing(FontFamily, FontStyleRegular, &result);
        expect(Ok, stat);
        ok(result == 2355, "Expected 2355, got %d\n", result);
        result = 0;
        stat = GdipGetEmHeight(FontFamily, FontStyleRegular, &result);
        expect(Ok, stat);
        ok(result == 2048, "Expected 2048, got %d\n", result);
        result = 0;
        stat = GdipGetCellAscent(FontFamily, FontStyleRegular, &result);
        expect(Ok, stat);
        ok(result == 1825, "Expected 1825, got %d\n", result);
        result = 0;
        stat = GdipGetCellDescent(FontFamily, FontStyleRegular, &result);
        expect(Ok, stat);
        ok(result == 443, "Expected 443 got %d\n", result);
        GdipDeleteFontFamily(FontFamily);
    }
}

static void check_family(const char* context, GpFontFamily *family, WCHAR *name)
{
    GpStatus stat;
    GpFont* font;

    *name = 0;
    stat = GdipGetFamilyName(family, name, LANG_NEUTRAL);
    ok(stat == Ok, "could not get the %s family name: %.8x\n", context, stat);

    stat = GdipCreateFont(family, 12, FontStyleRegular, UnitPixel, &font);
    ok(stat == Ok, "could not create a font for the %s family: %.8x\n", context, stat);
    if (stat == Ok)
    {
        stat = GdipDeleteFont(font);
        ok(stat == Ok, "could not delete the %s family font: %.8x\n", context, stat);
    }

    stat = GdipDeleteFontFamily(family);
    ok(stat == Ok, "could not delete the %s family: %.8x\n", context, stat);
}

static void test_getgenerics (void)
{
    GpStatus stat;
    GpFontFamily *family;
    WCHAR sansname[LF_FACESIZE], serifname[LF_FACESIZE], mononame[LF_FACESIZE];
    int missingfonts = 0;

    stat = GdipGetGenericFontFamilySansSerif(&family);
    expect (Ok, stat);
    if (stat == FontFamilyNotFound)
        missingfonts = 1;
    else
        check_family("Sans Serif", family, sansname);

    stat = GdipGetGenericFontFamilySerif(&family);
    expect (Ok, stat);
    if (stat == FontFamilyNotFound)
        missingfonts = 1;
    else
        check_family("Serif", family, serifname);

    stat = GdipGetGenericFontFamilyMonospace(&family);
    expect (Ok, stat);
    if (stat == FontFamilyNotFound)
        missingfonts = 1;
    else
        check_family("Monospace", family, mononame);

    if (missingfonts && strcmp(winetest_platform, "wine") == 0)
        trace("You may need to install either the Microsoft Web Fonts or the Liberation Fonts\n");

    /* Check that the family names are all different */
    ok(lstrcmpiW(sansname, serifname) != 0, "Sans Serif and Serif families should be different: %s\n", wine_dbgstr_w(sansname));
    ok(lstrcmpiW(sansname, mononame) != 0, "Sans Serif and Monospace families should be different: %s\n", wine_dbgstr_w(sansname));
    ok(lstrcmpiW(serifname, mononame) != 0, "Serif and Monospace families should be different: %s\n", wine_dbgstr_w(serifname));
}

static void test_installedfonts (void)
{
    GpStatus stat;
    GpFontCollection* collection=NULL;

    stat = GdipNewInstalledFontCollection(NULL);
    expect (InvalidParameter, stat);

    stat = GdipNewInstalledFontCollection(&collection);
    expect (Ok, stat);
    ok (collection != NULL, "got NULL font collection\n");
}

static void test_heightgivendpi(void)
{
    GpStatus stat;
    GpFont* font = NULL;
    GpFontFamily* fontfamily = NULL;
    REAL height;
    Unit unit;

    stat = GdipCreateFontFamilyFromName(Tahoma, NULL, &fontfamily);
    expect(Ok, stat);

    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitPixel, &font);
    expect(Ok, stat);

    stat = GdipGetFontHeightGivenDPI(NULL, 96, &height);
    expect(InvalidParameter, stat);

    stat = GdipGetFontHeightGivenDPI(font, 96, NULL);
    expect(InvalidParameter, stat);

    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(36.210938, height);
    GdipDeleteFont(font);

    height = 12345;
    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitWorld, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(36.210938, height);
    GdipDeleteFont(font);

    height = 12345;
    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitPoint, &font);
    expect(Ok, stat);
    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(48.281250, height);
    GdipDeleteFont(font);

    height = 12345;
    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitInch, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitInch, unit);

    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(3476.250000, height);
    GdipDeleteFont(font);

    height = 12345;
    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitDocument, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitDocument, unit);

    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(11.587500, height);
    GdipDeleteFont(font);

    height = 12345;
    stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitMillimeter, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitMillimeter, unit);

    stat = GdipGetFontHeightGivenDPI(font, 96, &height);
    expect(Ok, stat);
    expectf(136.860245, height);
    GdipDeleteFont(font);

    GdipDeleteFontFamily(fontfamily);
}

static int CALLBACK font_enum_proc(const LOGFONTW *lfe, const TEXTMETRICW *ntme,
                                   DWORD type, LPARAM lparam)
{
    NEWTEXTMETRICW *ntm = (NEWTEXTMETRICW *)lparam;

    if (type != TRUETYPE_FONTTYPE) return 1;

    *ntm = *(NEWTEXTMETRICW *)ntme;
    return 0;
}

struct font_metrics
{
    UINT16 em_height, line_spacing, ascent, descent;
    REAL font_height, font_size;
    INT lfHeight;
};

static void gdi_get_font_metrics(LOGFONTW *lf, struct font_metrics *fm)
{
    HDC hdc;
    HFONT hfont;
    NEWTEXTMETRICW ntm;
    OUTLINETEXTMETRICW otm;
    int ret;

    hdc = CreateCompatibleDC(0);

    /* it's the only way to get extended NEWTEXTMETRIC fields */
    ret = EnumFontFamiliesExW(hdc, lf, font_enum_proc, (LPARAM)&ntm, 0);
    ok(!ret, "EnumFontFamiliesExW failed to find %s\n", wine_dbgstr_w(lf->lfFaceName));

    hfont = CreateFontIndirectW(lf);
    SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    ok(ret, "GetOutlineTextMetrics failed\n");

    DeleteDC(hdc);
    DeleteObject(hfont);

    fm->lfHeight = -otm.otmTextMetrics.tmAscent;
    fm->line_spacing = ntm.ntmCellHeight;
    fm->font_size = (REAL)otm.otmTextMetrics.tmAscent;
    fm->font_height = (REAL)fm->line_spacing * fm->font_size / (REAL)ntm.ntmSizeEM;
    fm->em_height = ntm.ntmSizeEM;
    fm->ascent = ntm.ntmSizeEM;
    fm->descent = ntm.ntmCellHeight - ntm.ntmSizeEM;
}

static void gdip_get_font_metrics(GpFont *font, struct font_metrics *fm)
{
    INT style;
    GpFontFamily *family;
    GpStatus stat;

    stat = GdipGetFontStyle(font, &style);
    expect(Ok, stat);

    stat = GdipGetFontHeight(font, NULL, &fm->font_height);
    expect(Ok, stat);
    stat = GdipGetFontSize(font, &fm->font_size);
    expect(Ok, stat);

    fm->lfHeight = (INT)(fm->font_size * -1.0);

    stat = GdipGetFamily(font, &family);
    expect(Ok, stat);

    stat = GdipGetEmHeight(family, style, &fm->em_height);
    expect(Ok, stat);
    stat = GdipGetLineSpacing(family, style, &fm->line_spacing);
    expect(Ok, stat);
    stat = GdipGetCellAscent(family, style, &fm->ascent);
    expect(Ok, stat);
    stat = GdipGetCellDescent(family, style, &fm->descent);
    expect(Ok, stat);

    GdipDeleteFontFamily(family);
}

static void cmp_font_metrics(struct font_metrics *fm1, struct font_metrics *fm2, int line)
{
    ok_(__FILE__, line)(fm1->lfHeight == fm2->lfHeight, "lfHeight %d != %d\n", fm1->lfHeight, fm2->lfHeight);
    ok_(__FILE__, line)(fm1->em_height == fm2->em_height, "em_height %u != %u\n", fm1->em_height, fm2->em_height);
    ok_(__FILE__, line)(fm1->line_spacing == fm2->line_spacing, "line_spacing %u != %u\n", fm1->line_spacing, fm2->line_spacing);
    ok_(__FILE__, line)(abs(fm1->ascent - fm2->ascent) <= 1, "ascent %u != %u\n", fm1->ascent, fm2->ascent);
    ok_(__FILE__, line)(abs(fm1->descent - fm2->descent) <= 1, "descent %u != %u\n", fm1->descent, fm2->descent);
    ok(fm1->font_height > 0.0, "fm1->font_height should be positive, got %f\n", fm1->font_height);
    ok(fm2->font_height > 0.0, "fm2->font_height should be positive, got %f\n", fm2->font_height);
    ok_(__FILE__, line)(fm1->font_height == fm2->font_height, "font_height %f != %f\n", fm1->font_height, fm2->font_height);
    ok(fm1->font_size > 0.0, "fm1->font_size should be positive, got %f\n", fm1->font_size);
    ok(fm2->font_size > 0.0, "fm2->font_size should be positive, got %f\n", fm2->font_size);
    ok_(__FILE__, line)(fm1->font_size == fm2->font_size, "font_size %f != %f\n", fm1->font_size, fm2->font_size);
}

static void test_font_metrics(void)
{
    LOGFONTW lf;
    GpFont *font;
    GpFontFamily *family;
    GpGraphics *graphics;
    GpStatus stat;
    Unit unit;
    struct font_metrics fm_gdi, fm_gdip;
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    stat = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, stat);

    memset(&lf, 0, sizeof(lf));

    /* Tahoma,-13 */
    lstrcpyW(lf.lfFaceName, Tahoma);
    lf.lfHeight = -13;
    stat = GdipCreateFontFromLogfontW(hdc, &lf, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %d\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);
    ok((REAL)lf.lfHeight * -1.0 == fm_gdi.font_size, "expected %f, got %f\n", (REAL)lf.lfHeight * -1.0, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    GdipDeleteFont(font);

    /* Tahoma,13 */
    lstrcpyW(lf.lfFaceName, Tahoma);
    lf.lfHeight = 13;
    stat = GdipCreateFontFromLogfontW(hdc, &lf, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %d\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);
    ok((REAL)lf.lfHeight * -1.0 == fm_gdi.font_size, "expected %f, got %f\n", (REAL)lf.lfHeight * -1.0, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    GdipDeleteFont(font);

    stat = GdipCreateFontFamilyFromName(Tahoma, NULL, &family);
    expect(Ok, stat);

    /* Tahoma,13 */
    stat = GdipCreateFont(family, 13.0, FontStyleRegular, UnitPixel, &font);
    expect(Ok, stat);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %d\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%d: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);
    ok((REAL)lf.lfHeight * -1.0 == fm_gdi.font_size, "expected %f, got %f\n", (REAL)lf.lfHeight * -1.0, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    stat = GdipGetLogFontW(font, NULL, &lf);
    expect(InvalidParameter, stat);

    GdipDeleteFont(font);

    stat = GdipCreateFont(family, -13.0, FontStyleRegular, UnitPixel, &font);
    expect(InvalidParameter, stat);

    GdipDeleteFontFamily(family);

    GdipDeleteGraphics(graphics);
    DeleteDC(hdc);
}

static void test_font_substitution(void)
{
    WCHAR ms_shell_dlg[LF_FACESIZE];
    HDC hdc;
    HFONT hfont;
    LOGFONTA lf;
    GpStatus status;
    GpGraphics *graphics;
    GpFont *font;
    GpFontFamily *family;
    int ret;

    hdc = CreateCompatibleDC(0);
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    hfont = GetStockObject(DEFAULT_GUI_FONT);
    ok(hfont != 0, "GetStockObject(DEFAULT_GUI_FONT) failed\n");

    memset(&lf, 0xfe, sizeof(lf));
    ret = GetObjectA(hfont, sizeof(lf), &lf);
    ok(ret == sizeof(lf), "GetObject failed\n");
    ok(!lstrcmpA(lf.lfFaceName, "MS Shell Dlg"), "wrong face name %s\n", lf.lfFaceName);
    MultiByteToWideChar(CP_ACP, 0, lf.lfFaceName, -1, ms_shell_dlg, LF_FACESIZE);

    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, "Microsoft Sans Serif") ||
       !lstrcmpA(lf.lfFaceName, "Tahoma"), "wrong face name %s\n", lf.lfFaceName);
    GdipDeleteFont(font);

    status = GdipCreateFontFamilyFromName(ms_shell_dlg, NULL, &family);
    expect(Ok, status);
    status = GdipCreateFont(family, 12, FontStyleRegular, UnitPoint, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, "Microsoft Sans Serif") ||
       !lstrcmpA(lf.lfFaceName, "Tahoma"), "wrong face name %s\n", lf.lfFaceName);
    GdipDeleteFont(font);
    GdipDeleteFontFamily(family);

    status = GdipCreateFontFamilyFromName(nonexistent, NULL, &family);
    ok(status == FontFamilyNotFound, "expected FontFamilyNotFound, got %d\n", status);

    lstrcpyA(lf.lfFaceName, "ThisFontShouldNotExist");
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, "Arial"), "wrong face name %s\n", lf.lfFaceName);
    GdipDeleteFont(font);

    /* empty FaceName */
    lf.lfFaceName[0] = 0;
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, "Arial"), "wrong face name %s\n", lf.lfFaceName);
    GdipDeleteFont(font);

    /* zeroing out lfWeight and lfCharSet leads to font creation failure */
    lf.lfWeight = 0;
    lf.lfCharSet = 0;
    lstrcpyA(lf.lfFaceName, "ThisFontShouldNotExist");
    font = NULL;
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
todo_wine
    ok(status == NotTrueTypeFont || broken(status == FileNotFound), /* before XP */
       "expected NotTrueTypeFont, got %d\n", status);
    /* FIXME: remove when wine is fixed */
    if (font) GdipDeleteFont(font);

    /* empty FaceName */
    lf.lfFaceName[0] = 0;
    font = NULL;
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
todo_wine
    ok(status == NotTrueTypeFont || broken(status == FileNotFound), /* before XP */
       "expected NotTrueTypeFont, got %d\n", status);
    /* FIXME: remove when wine is fixed */
    if (font) GdipDeleteFont(font);

    GdipDeleteGraphics(graphics);
    DeleteDC(hdc);
}

static void test_font_transform(void)
{
    static const WCHAR string[] = { 'A',0 };
    GpStatus status;
    HDC hdc;
    LOGFONTA lf;
    GpFont *font;
    GpGraphics *graphics;
    GpMatrix *matrix;
    GpStringFormat *format, *typographic;
    PointF pos[1] = { { 0,0 } };
    REAL height, margin_y;
    RectF bounds, rect;

    hdc = CreateCompatibleDC(0);
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    status = GdipSetPageUnit(graphics, UnitPixel);
    expect(Ok, status);

    status = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, status);
    status = GdipStringFormatGetGenericTypographic(&typographic);
    expect(Ok, status);

    memset(&lf, 0, sizeof(lf));
    lstrcpyA(lf.lfFaceName, "Tahoma");
    lf.lfHeight = -100;
    lf.lfWidth = 100;
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);

    margin_y = 100.0 / 8.0;

    /* identity matrix */
    status = GdipCreateMatrix(&matrix);
    expect(Ok, status);
    status = GdipSetWorldTransform(graphics, matrix);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    expect(-100, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect(0, lf.lfEscapement);
    expect(0, lf.lfOrientation);
    status = GdipGetFontHeight(font, graphics, &height);
    expect(Ok, status);
    expectf(120.703125, height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, format, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
todo_wine
    expectf(height + margin_y, bounds.Height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, typographic, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(height, bounds.Height, 1.0);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, NULL, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.05);
    expectf_(height, bounds.Height, 0.5);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, matrix, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.05);
    expectf_(height, bounds.Height, 0.5);

    /* scale matrix */
    status = GdipScaleMatrix(matrix, 2.0, 3.0, MatrixOrderAppend);
    expect(Ok, status);
    status = GdipSetWorldTransform(graphics, matrix);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    expect(-300, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect(0, lf.lfEscapement);
    expect(0, lf.lfOrientation);
    status = GdipGetFontHeight(font, graphics, &height);
    expect(Ok, status);
    expectf(120.703125, height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, format, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
todo_wine
    expectf(height + margin_y, bounds.Height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, typographic, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(height, bounds.Height, 0.05);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, NULL, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.05);
    expectf_(height, bounds.Height, 0.2);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, matrix, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
todo_wine
    expectf_(-300.0, bounds.Y, 0.15);
todo_wine
    expectf(height * 3.0, bounds.Height);

    /* scale + ratate matrix */
    status = GdipRotateMatrix(matrix, 45.0, MatrixOrderAppend);
    expect(Ok, status);
    status = GdipSetWorldTransform(graphics, matrix);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    expect(-300, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect_(3151, lf.lfEscapement, 1);
    expect_(3151, lf.lfOrientation, 1);
    status = GdipGetFontHeight(font, graphics, &height);
    expect(Ok, status);
    expectf(120.703125, height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, format, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
todo_wine
    expectf(height + margin_y, bounds.Height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, typographic, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(height, bounds.Height, 0.05);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, NULL, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.05);
    expectf_(height, bounds.Height, 0.2);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, matrix, &bounds);
    expect(Ok, status);
todo_wine
    expectf_(-43.814377, bounds.X, 0.05);
todo_wine
    expectf_(-212.235611, bounds.Y, 0.05);
todo_wine
    expectf_(340.847534, bounds.Height, 0.05);

    /* scale + ratate + shear matrix */
    status = GdipShearMatrix(matrix, 4.0, 5.0, MatrixOrderAppend);
    expect(Ok, status);
    status = GdipSetWorldTransform(graphics, matrix);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
todo_wine
    expect(1032, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect_(3099, lf.lfEscapement, 1);
    expect_(3099, lf.lfOrientation, 1);
    status = GdipGetFontHeight(font, graphics, &height);
    expect(Ok, status);
    expectf(120.703125, height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, format, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
todo_wine
    expectf(height + margin_y, bounds.Height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, typographic, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(height, bounds.Height, 0.2);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, NULL, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.2);
    expectf_(height, bounds.Height, 0.2);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, matrix, &bounds);
    expect(Ok, status);
todo_wine
    expectf_(-636.706848, bounds.X, 0.05);
todo_wine
    expectf_(-175.257523, bounds.Y, 0.05);
todo_wine
    expectf_(1532.984985, bounds.Height, 0.05);

    /* scale + ratate + shear + translate matrix */
    status = GdipTranslateMatrix(matrix, 10.0, 20.0, MatrixOrderAppend);
    expect(Ok, status);
    status = GdipSetWorldTransform(graphics, matrix);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
todo_wine
    expect(1032, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect_(3099, lf.lfEscapement, 1);
    expect_(3099, lf.lfOrientation, 1);
    status = GdipGetFontHeight(font, graphics, &height);
    expect(Ok, status);
    expectf(120.703125, height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, format, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
todo_wine
    expectf(height + margin_y, bounds.Height);
    set_rect_empty(&rect);
    set_rect_empty(&bounds);
    status = GdipMeasureString(graphics, string, -1, font, &rect, typographic, &bounds, NULL, NULL);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf_(height, bounds.Height, 0.1);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, NULL, &bounds);
    expect(Ok, status);
    expectf(0.0, bounds.X);
    expectf_(-100.0, bounds.Y, 0.2);
    expectf_(height, bounds.Height, 0.2);
    set_rect_empty(&bounds);
    status = GdipMeasureDriverString(graphics, (const UINT16 *)string, -1, font, pos,
                                     DriverStringOptionsCmapLookup, matrix, &bounds);
    expect(Ok, status);
todo_wine
    expectf_(-626.706848, bounds.X, 0.05);
todo_wine
    expectf_(-155.257523, bounds.Y, 0.05);
todo_wine
    expectf_(1532.984985, bounds.Height, 0.05);

    GdipDeleteMatrix(matrix);
    GdipDeleteFont(font);
    GdipDeleteGraphics(graphics);
    GdipDeleteStringFormat(typographic);
    GdipDeleteStringFormat(format);
    DeleteDC(hdc);
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

    test_font_transform();
    if (!winetest_interactive)
        skip("ROSTESTS-154: Skipping test_font_substitution because of improper error handling\n");
    else
        test_font_substitution();
    test_font_metrics();
    test_createfont();
    test_logfont();
    test_fontfamily();
    test_fontfamily_properties();
    test_getgenerics();
    test_installedfonts();
    test_heightgivendpi();

    GdiplusShutdown(gdiplusToken);
}
