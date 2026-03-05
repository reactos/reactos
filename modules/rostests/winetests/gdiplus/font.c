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

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected,got) expect_inline(__LINE__, expected, got)
static inline void expect_inline(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}

#define expect_(expected, got, precision) ok(abs((expected) - (got)) <= (precision), "Expected %d, got %ld\n", (expected), (got))
#define expectf_(expected, got, precision) ok(fabs((expected) - (got)) <= (precision), "Expected %f, got %f\n", (expected), (got))
#define expectf(expected, got) expectf_((expected), (got), 0.001)

static void set_rect_empty(RectF *rc)
{
    rc->X = 0.0;
    rc->Y = 0.0;
    rc->Width = 0.0;
    rc->Height = 0.0;
}

#define load_resource(a, b, c) _load_resource(__LINE__, a, b, c)
static void _load_resource(int line, const WCHAR *filename, BYTE **data, DWORD *size)
{
    HRSRC resource = FindResourceW(NULL, filename, (const WCHAR *)RT_RCDATA);
    ok_(__FILE__, line)(!!resource, "FindResourceW failed, error %lu\n", GetLastError());
    *data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    ok_(__FILE__, line)(!!*data, "LockResource failed, error %lu\n", GetLastError());
    *size = SizeofResource(GetModuleHandleW(NULL), resource);
    ok_(__FILE__, line)(*size > 0, "SizeofResource failed, error %lu\n", GetLastError());
}

static void create_testfontfile(const WCHAR *filename, int resource, WCHAR pathW[MAX_PATH])
{
    DWORD written, length;
    HANDLE file;
    void *ptr;

    GetTempPathW(MAX_PATH, pathW);
    lstrcatW(pathW, filename);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", wine_dbgstr_w(pathW), GetLastError());

    load_resource(MAKEINTRESOURCEW(resource), (BYTE **)&ptr, &length);
    WriteFile(file, ptr, length, &written, NULL);
    ok(written == length, "couldn't write resource\n");
    CloseHandle(file);
}

#define DELETE_FONTFILE(filename) _delete_testfontfile(filename, __LINE__)
static void _delete_testfontfile(const WCHAR *filename, int line)
{
    BOOL ret = DeleteFileW(filename);
    ok_(__FILE__,line)(ret, "failed to delete file %s, error %ld\n", wine_dbgstr_w(filename), GetLastError());
}

static void test_long_name(void)
{
    WCHAR path[MAX_PATH];
    GpStatus stat;
    GpFontCollection *fonts;
    HANDLE file;
    INT num_families;
    GpFontFamily *family, *cloned_family;
    WCHAR family_name[LF_FACESIZE];
    GpFont *font;

    stat = GdipNewPrivateFontCollection(&fonts);
    ok(stat == Ok, "GdipNewPrivateFontCollection failed: %d\n", stat);

    create_testfontfile(L"wine_longname.ttf", 1, path);

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileW failed: %ld\n", GetLastError());

    stat = GdipPrivateAddFontFile(fonts, path);
    ok(stat == Ok, "GdipPrivateAddFontFile failed with open file handle: %d\n", stat);

    CloseHandle(file);

    if (stat != Ok) {
        /* try again without opened file handle */
        stat = GdipPrivateAddFontFile(fonts, path);
        ok(stat == Ok, "GdipPrivateAddFontFile failed: %d\n", stat);
    }

    stat = GdipGetFontCollectionFamilyCount(fonts, &num_families);
    ok(stat == Ok, "GdipGetFontCollectionFamilyCount failed: %d\n", stat);

    ok(num_families == 1, "expected num_families to be 1, got %d\n", num_families);

    stat = GdipGetFontCollectionFamilyList(fonts, num_families, &family, &num_families);
    ok(stat == Ok, "GdipGetFontCollectionFamilyList failed: %d\n", stat);

    stat = GdipGetFamilyName(family, family_name, LANG_NEUTRAL);
    ok(stat == Ok, "GdipGetFamilyName failed: %d\n", stat);

    stat = GdipCreateFont(family, 256.0, FontStyleRegular, UnitPixel, &font);
    ok(stat == Ok, "GdipCreateFont failed: %d\n", stat);

    stat = GdipCloneFontFamily(family, &cloned_family);
    ok(stat == Ok, "GdipCloneFontFamily failed: %d\n", stat);
    ok(family == cloned_family, "GdipCloneFontFamily returned new object\n");

    /* Cleanup */

    stat = GdipDeleteFont(font);
    ok(stat == Ok, "GdipDeleteFont failed: %d\n", stat);

    stat = GdipDeletePrivateFontCollection(&fonts);
    ok(stat == Ok, "GdipDeletePrivateFontCollection failed: %d\n", stat);

    /* Cloned family survives after collection is deleted */
    stat = GdipGetFamilyName(cloned_family, family_name, LANG_NEUTRAL);
    ok(stat == Ok, "GdipGetFamilyName failed: %d\n", stat);

    stat = GdipDeleteFontFamily(cloned_family);
    ok(stat == Ok, "GdipDeleteFontFamily failed: %d\n", stat);

    DELETE_FONTFILE(path);
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

    stat = GdipCreateFontFamilyFromName(L"ThisFontShouldNotExist", NULL, &fontfamily);
    expect (FontFamilyNotFound, stat);
    stat = GdipDeleteFont(font);
    expect (InvalidParameter, stat);
    stat = GdipCreateFontFamilyFromName(L"Tahoma", NULL, &fontfamily);
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
    ok (fontfamily == fontfamily2, "Unexpected family instance.\n");
    ok (lstrcmpiW(L"Tahoma", familyname) == 0, "Expected Tahoma, got %s\n",
            wine_dbgstr_w(familyname));
    stat = GdipDeleteFontFamily(fontfamily2);
    expect(Ok, stat);

    /* Test to see if returned size is based on unit (it's not) */
    GdipGetFontSize(font, &size);
    ok (size == 12, "Expected 12, got %f\n", size);
    GdipDeleteFont(font);

    /* Make sure everything is converted correctly for all Units */
    for (i = UnitWorld; i <=UnitMillimeter; i++)
    {
        if (i == UnitDisplay) continue; /* Crashes WindowsXP, wtf? */
        stat = GdipCreateFont(fontfamily, 24, FontStyleRegular, i, &font);
        expect(Ok, stat);
        GdipGetFontSize (font, &size);
        ok (size == 24, "Expected 24, got %f (with unit: %d)\n", size, i);
        stat = GdipGetFontUnit (font, &unit);
        ok (stat == Ok, "Failed to get font unit, %d.\n", stat);
        expect (i, unit);
        GdipDeleteFont(font);
    }

    GdipDeleteFontFamily(fontfamily);
}

static void test_createfont_charset(void)
{
    GpFontFamily* fontfamily = NULL;
    GpGraphics *graphics;
    GpFont* font = NULL;
    GpStatus stat;
    LOGFONTW lf;
    HDC hdc;
    UINT i;

    static const struct {
        LPCWSTR family_name;
        BYTE char_set;
    } td[] =
    {
        {L"Tahoma", ANSI_CHARSET},
        {L"Symbol", SYMBOL_CHARSET},
        {L"Marlett", SYMBOL_CHARSET},
        {L"Wingdings", SYMBOL_CHARSET},
    };

    hdc = CreateCompatibleDC(0);
    stat = GdipCreateFromHDC(hdc, &graphics);
    expect (Ok, stat);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        winetest_push_context("%u", i);

        stat = GdipCreateFontFamilyFromName(td[i].family_name, NULL, &fontfamily);
        expect (Ok, stat);
        stat = GdipCreateFont(fontfamily, 30, FontStyleRegular, UnitPoint, &font);
        expect (Ok, stat);

        stat = GdipGetLogFontW(font, graphics, &lf);
        expect(Ok, stat);

        if (lstrcmpiW(lf.lfFaceName, td[i].family_name) != 0)
        {
            skip("%s not installed\n", wine_dbgstr_w(td[i].family_name));
        }
        else
        {
            ok(lf.lfHeight < 0, "Expected negative height, got %ld\n", lf.lfHeight);
            expect(0, lf.lfWidth);
            expect(0, lf.lfEscapement);
            expect(0, lf.lfOrientation);
            ok((lf.lfWeight >= 100) && (lf.lfWeight <= 900), "Expected weight to be set\n");
            expect(0, lf.lfItalic);
            expect(0, lf.lfUnderline);
            expect(0, lf.lfStrikeOut);
            ok(td[i].char_set == lf.lfCharSet ||
                (td[i].char_set == ANSI_CHARSET && lf.lfCharSet == GetTextCharset(hdc)),
                "got %#x\n", lf.lfCharSet);
            expect(0, lf.lfOutPrecision);
            expect(0, lf.lfClipPrecision);
            expect(0, lf.lfQuality);
            expect(0, lf.lfPitchAndFamily);
        }

        GdipDeleteFont(font);
        GdipDeleteFontFamily(fontfamily);

        winetest_pop_context();
    }

    GdipDeleteGraphics(graphics);
    DeleteDC(hdc);
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

    stat = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, stat);

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
    font = NULL;

    /* The next test must be done with a font where tmHeight -
       tmInternalLeading != tmAscent. Times New Roman is such a font,
       so make sure we really have it before continuing. */
    memset(&lfa, 0, sizeof(lfa));
    lstrcpyA(lfa.lfFaceName, "Times New Roman");

    stat = GdipCreateFontFromLogfontA(hdc, &lfa, &font);
    expect(Ok, stat);

    memset(&lfa2, 0, sizeof(lfa2));
    stat = GdipGetLogFontA(font, graphics, &lfa2);
    expect(Ok, stat);

    GdipDeleteFont(font);
    font = NULL;

    if (!lstrlenA(lfa.lfFaceName) || lstrcmpA(lfa.lfFaceName, lfa2.lfFaceName))
    {
        skip("Times New Roman not installed\n");
    }
    else
    {
        static const struct
        {
            INT input;
            REAL expected;
        } test_sizes[] = {{12, 9.0}, {36, 32.0}, {48, 42.0}, {72, 63.0}, {144, 127.0}};

        UINT i;

        memset(&lfa, 0, sizeof(lfa));
        lstrcpyA(lfa.lfFaceName, "Times New Roman");

        for (i = 0; i < ARRAY_SIZE(test_sizes); ++i)
        {
            lfa.lfHeight = test_sizes[i].input;

            stat = GdipCreateFontFromLogfontA(hdc, &lfa, &font);
            expect(Ok, stat);

            stat = GdipGetFontSize(font, &rval);
            expect(Ok, stat);
            expectf(test_sizes[i].expected, rval);

            GdipDeleteFont(font);
            font = NULL;
        }
    }

    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_fontfamily (void)
{
    GpFontFamily *family, *clonedFontFamily;
    WCHAR itsName[LF_FACESIZE];
    GpStatus stat;

    /* FontFamily cannot be NULL */
    stat = GdipCreateFontFamilyFromName (L"Tahoma" , NULL, NULL);
    expect (InvalidParameter, stat);

    /* FontFamily must be able to actually find the family.
     * If it can't, any subsequent calls should fail.
     */
    stat = GdipCreateFontFamilyFromName (L"ThisFontShouldNotExist", NULL, &family);
    expect (FontFamilyNotFound, stat);

    /* Bitmap fonts are not found */
    stat = GdipCreateFontFamilyFromName (L"MS Sans Serif", NULL, &family);
    expect (FontFamilyNotFound, stat);
    if(stat == Ok) GdipDeleteFontFamily(family);

    stat = GdipCreateFontFamilyFromName (L"Tahoma", NULL, &family);
    expect (Ok, stat);

    stat = GdipGetFamilyName (family, itsName, LANG_NEUTRAL);
    expect (Ok, stat);
    expect (0, lstrcmpiW(itsName, L"Tahoma"));

    /* Crashes on Windows XP SP2 and Vista */
    stat = GdipGetFamilyName (family, NULL, LANG_NEUTRAL);
    expect (Ok, stat);

    /* Make sure we don't read old data */
    ZeroMemory (itsName, sizeof(itsName));
    stat = GdipCloneFontFamily(family, &clonedFontFamily);
    expect (Ok, stat);
    ok (family == clonedFontFamily, "Unexpected family instance.\n");
    GdipDeleteFontFamily(family);
    stat = GdipGetFamilyName(clonedFontFamily, itsName, LANG_NEUTRAL);
    expect(Ok, stat);
    expect(0, lstrcmpiW(itsName, L"Tahoma"));

    GdipDeleteFontFamily(clonedFontFamily);
}

static void test_fontfamily_properties (void)
{
    GpFontFamily* FontFamily = NULL;
    GpStatus stat;
    UINT16 result = 0;

    stat = GdipCreateFontFamilyFromName(L"Tahoma", NULL, &FontFamily);
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

    stat = GdipCreateFontFamilyFromName(L"Times New Roman", NULL, &FontFamily);
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

    stat = GdipCreateFontFamilyFromName(L"Tahoma", NULL, &fontfamily);
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

    stat = GdipGetFontHeight(NULL, NULL, &fm->font_height);
    expect(InvalidParameter, stat);

    stat = GdipGetFontHeight(font, NULL, NULL);
    expect(InvalidParameter, stat);

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
    lstrcpyW(lf.lfFaceName, L"Tahoma");
    lf.lfHeight = -13;
    stat = GdipCreateFontFromLogfontW(hdc, &lf, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %ld\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);
    ok((REAL)lf.lfHeight * -1.0 == fm_gdi.font_size, "expected %f, got %f\n", (REAL)lf.lfHeight * -1.0, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    GdipDeleteFont(font);

    /* Tahoma,13 */
    lstrcpyW(lf.lfFaceName, L"Tahoma");
    lf.lfHeight = 13;
    stat = GdipCreateFontFromLogfontW(hdc, &lf, &font);
    expect(Ok, stat);

    stat = GdipGetFontUnit(font, &unit);
    expect(Ok, stat);
    expect(UnitWorld, unit);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %ld\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdi.em_height, fm_gdi.line_spacing, fm_gdi.ascent, fm_gdi.descent,
          fm_gdi.font_height, fm_gdi.font_size);
    ok((REAL)lf.lfHeight * -1.0 == fm_gdi.font_size, "expected %f, got %f\n", (REAL)lf.lfHeight * -1.0, fm_gdi.font_size);

    cmp_font_metrics(&fm_gdip, &fm_gdi, __LINE__);

    GdipDeleteFont(font);

    stat = GdipCreateFontFamilyFromName(L"Tahoma", NULL, &family);
    expect(Ok, stat);

    /* Tahoma,13 */
    stat = GdipCreateFont(family, 13.0, FontStyleRegular, UnitPixel, &font);
    expect(Ok, stat);

    gdip_get_font_metrics(font, &fm_gdip);
    trace("gdiplus:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
          wine_dbgstr_w(lf.lfFaceName), lf.lfHeight,
          fm_gdip.em_height, fm_gdip.line_spacing, fm_gdip.ascent, fm_gdip.descent,
          fm_gdip.font_height, fm_gdip.font_size);

    stat = GdipGetLogFontW(font, graphics, &lf);
    expect(Ok, stat);
    ok(lf.lfHeight < 0, "lf.lfHeight should be negative, got %ld\n", lf.lfHeight);
    gdi_get_font_metrics(&lf, &fm_gdi);
    trace("gdi:\n");
    trace("%s,%ld: EmHeight %u, LineSpacing %u, CellAscent %u, CellDescent %u, FontHeight %f, FontSize %f\n",
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
    char fallback_font[LF_FACESIZE];
    HDC hdc;
    LOGFONTA lf;
    GpStatus status;
    GpGraphics *graphics;
    GpFont *font;
    GpFontFamily *family;

    hdc = CreateCompatibleDC(0);
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);

    memset(&lf, 0, sizeof(lf));
    lstrcpyA(lf.lfFaceName, "MS Shell Dlg");

    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(lstrcmpA(lf.lfFaceName, "MS Shell Dlg") != 0, "expected substitution of MS Shell Dlg\n");
    GdipDeleteFont(font);

    family = NULL;
    status = GdipCreateFontFamilyFromName(L"MS Shell Dlg", NULL, &family);
    expect(Ok, status);
    font = NULL;
    status = GdipCreateFont(family, 12, FontStyleRegular, UnitPoint, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(lstrcmpA(lf.lfFaceName, "MS Shell Dlg") != 0, "expected substitution of MS Shell Dlg\n");
    GdipDeleteFont(font);
    GdipDeleteFontFamily(family);

    status = GdipCreateFontFamilyFromName(L"ThisFontShouldNotExist", NULL, &family);
    ok(status == FontFamilyNotFound, "expected FontFamilyNotFound, got %d\n", status);

    /* nonexistent fonts fallback to Arial, or something else if it's missing */
    strcpy(lf.lfFaceName,"Arial");
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    strcpy(fallback_font,lf.lfFaceName);
    trace("fallback font %s\n", fallback_font);
    GdipDeleteFont(font);

    lstrcpyA(lf.lfFaceName, "ThisFontShouldNotExist");
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, fallback_font), "wrong face name %s / %s\n", lf.lfFaceName, fallback_font);
    GdipDeleteFont(font);

    /* empty FaceName */
    lf.lfFaceName[0] = 0;
    status = GdipCreateFontFromLogfontA(hdc, &lf, &font);
    expect(Ok, status);
    memset(&lf, 0xfe, sizeof(lf));
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    ok(!lstrcmpA(lf.lfFaceName, fallback_font), "wrong face name %s / %s\n", lf.lfFaceName, fallback_font);
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
    static const WCHAR string[] = L"A";
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

    GdipDeleteGraphics(graphics);

    SetMapMode( hdc, MM_ISOTROPIC);
    SetWindowExtEx(hdc, 200, 200, NULL);
    SetViewportExtEx(hdc, 100, 100, NULL);
    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    status = GdipGetLogFontA(font, graphics, &lf);
    expect(Ok, status);
    expect(-50, lf.lfHeight);
    expect(0, lf.lfWidth);
    expect(0, lf.lfEscapement);
    expect(0, lf.lfOrientation);

    GdipDeleteMatrix(matrix);
    GdipDeleteFont(font);
    GdipDeleteGraphics(graphics);
    GdipDeleteStringFormat(typographic);
    GdipDeleteStringFormat(format);
    DeleteDC(hdc);
}

static void test_GdipGetFontCollectionFamilyList(void)
{
    GpFontFamily *family, *family2, **families;
    GpFontCollection *collection;
    UINT i;
    INT found, count;
    GpStatus status;

    status = GdipNewInstalledFontCollection(&collection);
    ok(status == Ok, "Failed to get system collection, status %d.\n", status);

    count = 0;
    status = GdipGetFontCollectionFamilyCount(collection, &count);
    ok(status == Ok, "Failed to get family count, status %d.\n", status);
    ok(count > 0, "Unexpected empty collection.\n");

    status = GdipGetFontCollectionFamilyList(NULL, 0, NULL, NULL);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);

    found = 123;
    status = GdipGetFontCollectionFamilyList(NULL, 0, NULL, &found);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);
    ok(found == 123, "Unexpected list count %d.\n", found);

    status = GdipGetFontCollectionFamilyList(collection, 0, NULL, NULL);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);

    found = 123;
    status = GdipGetFontCollectionFamilyList(collection, 0, NULL, &found);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);
    ok(found == 123, "Unexpected list count %d.\n", found);

    found = 123;
    status = GdipGetFontCollectionFamilyList(collection, 1, NULL, &found);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);
    ok(found == 123, "Unexpected list count %d.\n", found);

    family = NULL;
    found = 0;
    status = GdipGetFontCollectionFamilyList(collection, 1, &family, &found);
    ok(status == Ok, "Failed to get family list, status %d.\n", status);
    ok(found == 1, "Unexpected list count %d.\n", found);
    ok(family != NULL, "Expected family instance.\n");

    family2 = NULL;
    found = 0;
    status = GdipGetFontCollectionFamilyList(collection, 1, &family2, &found);
    ok(status == Ok, "Failed to get family list, status %d.\n", status);
    ok(found == 1, "Unexpected list count %d.\n", found);
    ok(family2 == family, "Unexpected family instance.\n");

    status = GdipDeleteFontFamily(family);
    expect(Ok, status);

    status = GdipDeleteFontFamily(family2);
    expect(Ok, status);

    families = GdipAlloc((count + 1) * sizeof(*families));
    found = 0;
    status = GdipGetFontCollectionFamilyList(collection, count + 1, families, &found);
    ok(status == Ok, "Failed to get family list, status %d.\n", status);
    ok(found == count, "Unexpected list count %d, extected %d.\n", found, count);

    for (i = 0; i < found; i++)
    {
        status = GdipDeleteFontFamily(families[i]);
        expect(Ok, status);
    }
    GdipFree(families);
}

static void test_GdipGetFontCollectionFamilyCount(void)
{
    GpFontCollection *collection;
    GpStatus status;
    INT count;

    status = GdipGetFontCollectionFamilyCount(NULL, NULL);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);

    count = 123;
    status = GdipGetFontCollectionFamilyCount(NULL, &count);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);
    ok(count == 123, "Unexpected family count %d.\n", count);

    status = GdipNewInstalledFontCollection(&collection);
    ok(status == Ok, "Failed to get system collection, status %d.\n", status);

    status = GdipGetFontCollectionFamilyCount(collection, NULL);
    ok(status == InvalidParameter, "Unexpected status %d.\n", status);
}

static BOOL is_family_in_collection(GpFontCollection *collection, GpFontFamily *family)
{
    GpStatus status;
    GpFontFamily **list;
    int count, i;
    BOOL found = FALSE;

    status = GdipGetFontCollectionFamilyCount(collection, &count);
    expect(Ok, status);

    list = GdipAlloc(count * sizeof(GpFontFamily *));
    status = GdipGetFontCollectionFamilyList(collection, count, list, &count);
    expect(Ok, status);

    for (i = 0; i < count; i++)
    {
        if (list[i] == family)
        {
            found = TRUE;
            break;
        }
    }

    GdipFree(list);

    return found;
}

static void test_CloneFont(void)
{
    GpStatus status;
    GpFontCollection *collection, *collection2;
    GpFont *font, *font2;
    GpFontFamily *family, *family2;
    REAL height;
    Unit unit;
    int style;
    BOOL ret;

    status = GdipNewInstalledFontCollection(&collection);
    expect(Ok, status);

    status = GdipNewInstalledFontCollection(&collection2);
    expect(Ok, status);
    ok(collection == collection2, "got %p\n", collection2);

    status = GdipCreateFontFamilyFromName(L"ThisFontShouldNotExist", NULL, &family);
    expect(FontFamilyNotFound, status);

    status = GdipCreateFontFamilyFromName(L"ThisFontShouldNotExist", collection, &family);
    expect(FontFamilyNotFound, status);

    status = GdipCreateFontFamilyFromName(L"Tahoma", NULL, &family);
    expect(Ok, status);

    ret = is_family_in_collection(collection, family);
    ok(ret, "family is not in collection\n");

    status = GdipCreateFont(family, 30.0f, FontStyleRegular, UnitPixel, &font);
    expect(Ok, status);

    status = GdipGetFontUnit(font, &unit);
    expect(Ok, status);
    ok(unit == UnitPixel, "got %u\n", unit);

    status = GdipGetFontSize(font, &height);
    expect(Ok, status);
    ok(height == 30.0f, "got %f\n", height);

    status = GdipGetFontStyle(font, &style);
    expect(Ok, status);
    ok(style == FontStyleRegular, "got %d\n", style);

    status = GdipGetFamily(font, &family2);
    expect(Ok, status);
    ok(family == family2, "got %p\n", family2);

    status = GdipCloneFont(font, &font2);
    expect(Ok, status);

    status = GdipGetFontUnit(font2, &unit);
    expect(Ok, status);
    ok(unit == UnitPixel, "got %u\n", unit);

    status = GdipGetFontSize(font2, &height);
    expect(Ok, status);
    ok(height == 30.0f, "got %f\n", height);

    status = GdipGetFontStyle(font2, &style);
    expect(Ok, status);
    ok(style == FontStyleRegular, "got %d\n", style);

    status = GdipGetFamily(font2, &family2);
    expect(Ok, status);
    ok(family == family2, "got %p\n", family2);

    GdipDeleteFont(font2);
    GdipDeleteFont(font);
    GdipDeleteFontFamily(family);
}

static void test_GdipPrivateAddMemoryFont(void)
{
    static const WORD resource_ids[] =
    {
        3, /* A font that has an invalid full name on Mac platform and a valid full name on Microsoft platform */
        4, /* A font that has an invalid full name on Unicode platform and a valid full name on Mac platform */
    };
    GpFontCollection *fonts;
    GpStatus stat;
    int count, i;
    void *buffer;
    DWORD size;

    for (i = 0; i < ARRAY_SIZE(resource_ids); i++)
    {
        winetest_push_context("test %d", i);

        stat = GdipNewPrivateFontCollection(&fonts);
        ok(stat == Ok, "GdipNewPrivateFontCollection failed, error %d\n", stat);

        load_resource(MAKEINTRESOURCEW(resource_ids[i]), (BYTE **)&buffer, &size);
        stat = GdipPrivateAddMemoryFont(fonts, buffer, size);
        if (stat == Ok)
        {
            stat = GdipGetFontCollectionFamilyCount(fonts, &count);
            ok(stat == Ok, "GdipGetFontCollectionFamilyCount failed, error %d\n", stat);
            ok(count == 1, "Expected count 1, got %d\n", count);
        }
        else if (i == 1 && stat == FileNotFound)
            win_skip("Fonts without Microsoft platform names are unsupported on win7.\n");
        else
            ok(0, "GdipPrivateAddMemoryFont failed, error %d\n", stat);

        stat = GdipDeletePrivateFontCollection(&fonts);
        ok(stat == Ok, "GdipDeletePrivateFontCollection failed, error %d\n", stat);

        winetest_pop_context();
    }
}

START_TEST(font)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_CloneFont();
    test_long_name();
    test_font_transform();
    test_font_substitution();
    test_font_metrics();
    test_createfont();
    test_createfont_charset();
    test_logfont();
    test_fontfamily();
    test_fontfamily_properties();
    test_getgenerics();
    test_installedfonts();
    test_heightgivendpi();
    test_GdipGetFontCollectionFamilyList();
    test_GdipGetFontCollectionFamilyCount();
    test_GdipPrivateAddMemoryFont();

    GdiplusShutdown(gdiplusToken);
}
