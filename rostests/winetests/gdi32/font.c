/*
 * Unit test suite for fonts
 *
 * Copyright 2002 Mike McCormack
 * Copyright 2004 Dmitry Timoshkov
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/test.h"

#define near_match(a, b) (abs((a) - (b)) <= 6)
#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

LONG  (WINAPI *pGdiGetCharDimensions)(HDC hdc, LPTEXTMETRICW lptm, LONG *height);
BOOL  (WINAPI *pGetCharABCWidthsI)(HDC hdc, UINT first, UINT count, LPWORD glyphs, LPABC abc);
BOOL  (WINAPI *pGetCharABCWidthsW)(HDC hdc, UINT first, UINT last, LPABC abc);
DWORD (WINAPI *pGetFontUnicodeRanges)(HDC hdc, LPGLYPHSET lpgs);
DWORD (WINAPI *pGetGlyphIndicesA)(HDC hdc, LPCSTR lpstr, INT count, LPWORD pgi, DWORD flags);
DWORD (WINAPI *pGetGlyphIndicesW)(HDC hdc, LPCWSTR lpstr, INT count, LPWORD pgi, DWORD flags);
BOOL  (WINAPI *pGdiRealizationInfo)(HDC hdc, DWORD *);

static HMODULE hgdi32 = 0;

static void init(void)
{
    hgdi32 = GetModuleHandleA("gdi32.dll");

    pGdiGetCharDimensions = (void *)GetProcAddress(hgdi32, "GdiGetCharDimensions");
    pGetCharABCWidthsI = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsI");
    pGetCharABCWidthsW = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsW");
    pGetFontUnicodeRanges = (void *)GetProcAddress(hgdi32, "GetFontUnicodeRanges");
    pGetGlyphIndicesA = (void *)GetProcAddress(hgdi32, "GetGlyphIndicesA");
    pGetGlyphIndicesW = (void *)GetProcAddress(hgdi32, "GetGlyphIndicesW");
    pGdiRealizationInfo = (void *)GetProcAddress(hgdi32, "GdiRealizationInfo");
}

static INT CALLBACK is_truetype_font_installed_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    if (type != TRUETYPE_FONTTYPE) return 1;

    return 0;
}

static BOOL is_truetype_font_installed(const char *name)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if (!EnumFontFamiliesA(hdc, name, is_truetype_font_installed_proc, 0))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

static INT CALLBACK is_font_installed_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const char *name)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if(!EnumFontFamiliesA(hdc, name, is_font_installed_proc, 0))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

static void check_font(const char* test, const LOGFONTA* lf, HFONT hfont)
{
    LOGFONTA getobj_lf;
    int ret, minlen = 0;

    if (!hfont)
        return;

    ret = GetObject(hfont, sizeof(getobj_lf), &getobj_lf);
    /* NT4 tries to be clever and only returns the minimum length */
    while (lf->lfFaceName[minlen] && minlen < LF_FACESIZE-1)
        minlen++;
    minlen += FIELD_OFFSET(LOGFONTA, lfFaceName) + 1;
    ok(ret == sizeof(LOGFONTA) || ret == minlen, "%s: GetObject returned %d\n", test, ret);
    ok(lf->lfHeight == getobj_lf.lfHeight ||
       broken((SHORT)lf->lfHeight == getobj_lf.lfHeight), /* win9x */
       "lfHeight: expect %08x got %08x\n", lf->lfHeight, getobj_lf.lfHeight);
    ok(lf->lfWidth == getobj_lf.lfWidth ||
       broken((SHORT)lf->lfWidth == getobj_lf.lfWidth), /* win9x */
       "lfWidth: expect %08x got %08x\n", lf->lfWidth, getobj_lf.lfWidth);
    ok(lf->lfEscapement == getobj_lf.lfEscapement ||
       broken((SHORT)lf->lfEscapement == getobj_lf.lfEscapement), /* win9x */
       "lfEscapement: expect %08x got %08x\n", lf->lfEscapement, getobj_lf.lfEscapement);
    ok(lf->lfOrientation == getobj_lf.lfOrientation ||
       broken((SHORT)lf->lfOrientation == getobj_lf.lfOrientation), /* win9x */
       "lfOrientation: expect %08x got %08x\n", lf->lfOrientation, getobj_lf.lfOrientation);
    ok(lf->lfWeight == getobj_lf.lfWeight ||
       broken((SHORT)lf->lfWeight == getobj_lf.lfWeight), /* win9x */
       "lfWeight: expect %08x got %08x\n", lf->lfWeight, getobj_lf.lfWeight);
    ok(lf->lfItalic == getobj_lf.lfItalic, "lfItalic: expect %02x got %02x\n", lf->lfItalic, getobj_lf.lfItalic);
    ok(lf->lfUnderline == getobj_lf.lfUnderline, "lfUnderline: expect %02x got %02x\n", lf->lfUnderline, getobj_lf.lfUnderline);
    ok(lf->lfStrikeOut == getobj_lf.lfStrikeOut, "lfStrikeOut: expect %02x got %02x\n", lf->lfStrikeOut, getobj_lf.lfStrikeOut);
    ok(lf->lfCharSet == getobj_lf.lfCharSet, "lfCharSet: expect %02x got %02x\n", lf->lfCharSet, getobj_lf.lfCharSet);
    ok(lf->lfOutPrecision == getobj_lf.lfOutPrecision, "lfOutPrecision: expect %02x got %02x\n", lf->lfOutPrecision, getobj_lf.lfOutPrecision);
    ok(lf->lfClipPrecision == getobj_lf.lfClipPrecision, "lfClipPrecision: expect %02x got %02x\n", lf->lfClipPrecision, getobj_lf.lfClipPrecision);
    ok(lf->lfQuality == getobj_lf.lfQuality, "lfQuality: expect %02x got %02x\n", lf->lfQuality, getobj_lf.lfQuality);
    ok(lf->lfPitchAndFamily == getobj_lf.lfPitchAndFamily, "lfPitchAndFamily: expect %02x got %02x\n", lf->lfPitchAndFamily, getobj_lf.lfPitchAndFamily);
    ok(!lstrcmpA(lf->lfFaceName, getobj_lf.lfFaceName) ||
       broken(!memcmp(lf->lfFaceName, getobj_lf.lfFaceName, LF_FACESIZE-1)), /* win9x doesn't ensure '\0' termination */
       "%s: font names don't match: %s != %s\n", test, lf->lfFaceName, getobj_lf.lfFaceName);
}

static HFONT create_font(const char* test, const LOGFONTA* lf)
{
    HFONT hfont = CreateFontIndirectA(lf);
    ok(hfont != 0, "%s: CreateFontIndirect failed\n", test);
    if (hfont)
        check_font(test, lf, hfont);
    return hfont;
}

static void test_logfont(void)
{
    LOGFONTA lf;
    HFONT hfont;

    memset(&lf, 0, sizeof lf);

    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;

    lstrcpyA(lf.lfFaceName, "Arial");
    hfont = create_font("Arial", &lf);
    DeleteObject(hfont);

    memset(&lf, 'A', sizeof(lf));
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA with strange LOGFONT failed\n");
    
    lf.lfFaceName[LF_FACESIZE - 1] = 0;
    check_font("AAA...", &lf, hfont);
    DeleteObject(hfont);
}

static INT CALLBACK font_enum_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    if (type & RASTER_FONTTYPE)
    {
	LOGFONT *lf = (LOGFONT *)lParam;
	*lf = *elf;
	return 0; /* stop enumeration */
    }

    return 1; /* continue enumeration */
}

static void compare_tm(const TEXTMETRICA *tm, const TEXTMETRICA *otm)
{
    ok(tm->tmHeight == otm->tmHeight, "tmHeight %d != %d\n", tm->tmHeight, otm->tmHeight);
    ok(tm->tmAscent == otm->tmAscent, "tmAscent %d != %d\n", tm->tmAscent, otm->tmAscent);
    ok(tm->tmDescent == otm->tmDescent, "tmDescent %d != %d\n", tm->tmDescent, otm->tmDescent);
    ok(tm->tmInternalLeading == otm->tmInternalLeading, "tmInternalLeading %d != %d\n", tm->tmInternalLeading, otm->tmInternalLeading);
    ok(tm->tmExternalLeading == otm->tmExternalLeading, "tmExternalLeading %d != %d\n", tm->tmExternalLeading, otm->tmExternalLeading);
    ok(tm->tmAveCharWidth == otm->tmAveCharWidth, "tmAveCharWidth %d != %d\n", tm->tmAveCharWidth, otm->tmAveCharWidth);
    ok(tm->tmMaxCharWidth == otm->tmMaxCharWidth, "tmMaxCharWidth %d != %d\n", tm->tmMaxCharWidth, otm->tmMaxCharWidth);
    ok(tm->tmWeight == otm->tmWeight, "tmWeight %d != %d\n", tm->tmWeight, otm->tmWeight);
    ok(tm->tmOverhang == otm->tmOverhang, "tmOverhang %d != %d\n", tm->tmOverhang, otm->tmOverhang);
    ok(tm->tmDigitizedAspectX == otm->tmDigitizedAspectX, "tmDigitizedAspectX %d != %d\n", tm->tmDigitizedAspectX, otm->tmDigitizedAspectX);
    ok(tm->tmDigitizedAspectY == otm->tmDigitizedAspectY, "tmDigitizedAspectY %d != %d\n", tm->tmDigitizedAspectY, otm->tmDigitizedAspectY);
    ok(tm->tmFirstChar == otm->tmFirstChar, "tmFirstChar %d != %d\n", tm->tmFirstChar, otm->tmFirstChar);
    ok(tm->tmLastChar == otm->tmLastChar, "tmLastChar %d != %d\n", tm->tmLastChar, otm->tmLastChar);
    ok(tm->tmDefaultChar == otm->tmDefaultChar, "tmDefaultChar %d != %d\n", tm->tmDefaultChar, otm->tmDefaultChar);
    ok(tm->tmBreakChar == otm->tmBreakChar, "tmBreakChar %d != %d\n", tm->tmBreakChar, otm->tmBreakChar);
    ok(tm->tmItalic == otm->tmItalic, "tmItalic %d != %d\n", tm->tmItalic, otm->tmItalic);
    ok(tm->tmUnderlined == otm->tmUnderlined, "tmUnderlined %d != %d\n", tm->tmUnderlined, otm->tmUnderlined);
    ok(tm->tmStruckOut == otm->tmStruckOut, "tmStruckOut %d != %d\n", tm->tmStruckOut, otm->tmStruckOut);
    ok(tm->tmPitchAndFamily == otm->tmPitchAndFamily, "tmPitchAndFamily %d != %d\n", tm->tmPitchAndFamily, otm->tmPitchAndFamily);
    ok(tm->tmCharSet == otm->tmCharSet, "tmCharSet %d != %d\n", tm->tmCharSet, otm->tmCharSet);
}

static void test_font_metrics(HDC hdc, HFONT hfont, LONG lfHeight,
                              LONG lfWidth, const char *test_str,
			      INT test_str_len, const TEXTMETRICA *tm_orig,
			      const SIZE *size_orig, INT width_of_A_orig,
			      INT scale_x, INT scale_y)
{
    LOGFONTA lf;
    OUTLINETEXTMETRIC otm;
    TEXTMETRICA tm;
    SIZE size;
    INT width_of_A, cx, cy;
    UINT ret;

    if (!hfont)
        return;

    ok(GetCurrentObject(hdc, OBJ_FONT) == hfont, "hfont should be selected\n");

    GetObjectA(hfont, sizeof(lf), &lf);

    if (GetOutlineTextMetricsA(hdc, 0, NULL))
    {
        otm.otmSize = sizeof(otm) / 2;
        ret = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
        ok(ret == sizeof(otm)/2 /* XP */ ||
           ret == 1 /* Win9x */, "expected sizeof(otm)/2, got %u\n", ret);

        memset(&otm, 0x1, sizeof(otm));
        otm.otmSize = sizeof(otm);
        ret = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
        ok(ret == sizeof(otm) /* XP */ ||
           ret == 1 /* Win9x */, "expected sizeof(otm), got %u\n", ret);

        memset(&tm, 0x2, sizeof(tm));
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetricsA failed\n");
        /* the structure size is aligned */
        if (memcmp(&tm, &otm.otmTextMetrics, FIELD_OFFSET(TEXTMETRICA, tmCharSet) + 1))
        {
            ok(0, "tm != otm\n");
            compare_tm(&tm, &otm.otmTextMetrics);
        }

        tm = otm.otmTextMetrics;
if (0) /* these metrics are scaled too, but with rounding errors */
{
        ok(otm.otmAscent == tm.tmAscent, "ascent %d != %d\n", otm.otmAscent, tm.tmAscent);
        ok(otm.otmDescent == -tm.tmDescent, "descent %d != %d\n", otm.otmDescent, -tm.tmDescent);
}
        ok(otm.otmMacAscent == tm.tmAscent, "ascent %d != %d\n", otm.otmMacAscent, tm.tmAscent);
        ok(otm.otmDescent < 0, "otm.otmDescent should be < 0\n");
        ok(otm.otmMacDescent < 0, "otm.otmMacDescent should be < 0\n");
        ok(tm.tmDescent > 0, "tm.tmDescent should be > 0\n");
        ok(otm.otmMacDescent == -tm.tmDescent, "descent %d != %d\n", otm.otmMacDescent, -tm.tmDescent);
        ok(otm.otmEMSquare == 2048, "expected 2048, got %d\n", otm.otmEMSquare);
    }
    else
    {
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetricsA failed\n");
    }

    cx = tm.tmAveCharWidth / tm_orig->tmAveCharWidth;
    cy = tm.tmHeight / tm_orig->tmHeight;
    ok(cx == scale_x && cy == scale_y, "height %d: expected scale_x %d, scale_y %d, got cx %d, cy %d\n",
       lfHeight, scale_x, scale_y, cx, cy);
    ok(tm.tmHeight == tm_orig->tmHeight * scale_y, "height %d != %d\n", tm.tmHeight, tm_orig->tmHeight * scale_y);
    ok(tm.tmAscent == tm_orig->tmAscent * scale_y, "ascent %d != %d\n", tm.tmAscent, tm_orig->tmAscent * scale_y);
    ok(tm.tmDescent == tm_orig->tmDescent * scale_y, "descent %d != %d\n", tm.tmDescent, tm_orig->tmDescent * scale_y);
    ok(near_match(tm.tmAveCharWidth, tm_orig->tmAveCharWidth * scale_x), "ave width %d != %d\n", tm.tmAveCharWidth, tm_orig->tmAveCharWidth * scale_x);
    ok(near_match(tm.tmMaxCharWidth, tm_orig->tmMaxCharWidth * scale_x), "max width %d != %d\n", tm.tmMaxCharWidth, tm_orig->tmMaxCharWidth * scale_x);

    ok(lf.lfHeight == lfHeight, "lfHeight %d != %d\n", lf.lfHeight, lfHeight);
    if (lf.lfHeight)
    {
        if (lf.lfWidth)
            ok(lf.lfWidth == tm.tmAveCharWidth, "lfWidth %d != tm %d\n", lf.lfWidth, tm.tmAveCharWidth);
    }
    else
        ok(lf.lfWidth == lfWidth, "lfWidth %d != %d\n", lf.lfWidth, lfWidth);

    GetTextExtentPoint32A(hdc, test_str, test_str_len, &size);

    ok(near_match(size.cx, size_orig->cx * scale_x), "cx %d != %d\n", size.cx, size_orig->cx * scale_x);
    ok(size.cy == size_orig->cy * scale_y, "cy %d != %d\n", size.cy, size_orig->cy * scale_y);

    GetCharWidthA(hdc, 'A', 'A', &width_of_A);

    ok(near_match(width_of_A, width_of_A_orig * scale_x), "width A %d != %d\n", width_of_A, width_of_A_orig * scale_x);
}

/* Test how GDI scales bitmap font metrics */
static void test_bitmap_font(void)
{
    static const char test_str[11] = "Test String";
    HDC hdc;
    LOGFONTA bitmap_lf;
    HFONT hfont, old_hfont;
    TEXTMETRICA tm_orig;
    SIZE size_orig;
    INT ret, i, width_orig, height_orig, scale, lfWidth;

    hdc = GetDC(0);

    /* "System" has only 1 pixel size defined, otherwise the test breaks */
    ret = EnumFontFamiliesA(hdc, "System", font_enum_proc, (LPARAM)&bitmap_lf);
    if (ret)
    {
	ReleaseDC(0, hdc);
	trace("no bitmap fonts were found, skipping the test\n");
	return;
    }

    trace("found bitmap font %s, height %d\n", bitmap_lf.lfFaceName, bitmap_lf.lfHeight);

    height_orig = bitmap_lf.lfHeight;
    lfWidth = bitmap_lf.lfWidth;

    hfont = create_font("bitmap", &bitmap_lf);
    old_hfont = SelectObject(hdc, hfont);
    ok(GetTextMetricsA(hdc, &tm_orig), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    bitmap_lf.lfHeight = 0;
    bitmap_lf.lfWidth = 4;
    hfont = create_font("bitmap", &bitmap_lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, 0, 4, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 1, 1);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    bitmap_lf.lfHeight = height_orig;
    bitmap_lf.lfWidth = lfWidth;

    /* test fractional scaling */
    for (i = 1; i <= height_orig * 6; i++)
    {
        INT nearest_height;

        bitmap_lf.lfHeight = i;
	hfont = create_font("fractional", &bitmap_lf);
        scale = (i + height_orig - 1) / height_orig;
        nearest_height = scale * height_orig;
        /* Only jump to the next height if the difference <= 25% original height */
        if (scale > 2 && nearest_height - i > height_orig / 4) scale--;
        /* The jump between unscaled and doubled is delayed by 1 in winnt+ but not in win9x,
           so we'll not test this particular height. */
        else if(scale == 2 && nearest_height - i == (height_orig / 4)) continue;
        else if(scale == 2 && nearest_height - i > (height_orig / 4 - 1)) scale--;
        old_hfont = SelectObject(hdc, hfont);
        test_font_metrics(hdc, hfont, bitmap_lf.lfHeight, 0, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 1, scale);
        SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
    }

    /* test integer scaling 3x2 */
    bitmap_lf.lfHeight = height_orig * 2;
    bitmap_lf.lfWidth *= 3;
    hfont = create_font("3x2", &bitmap_lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, bitmap_lf.lfHeight, 0, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 2);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test integer scaling 3x3 */
    bitmap_lf.lfHeight = height_orig * 3;
    bitmap_lf.lfWidth = 0;
    hfont = create_font("3x3", &bitmap_lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, bitmap_lf.lfHeight, 0, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 3);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
}

/* Test how GDI scales outline font metrics */
static void test_outline_font(void)
{
    static const char test_str[11] = "Test String";
    HDC hdc, hdc_2;
    LOGFONTA lf;
    HFONT hfont, old_hfont, old_hfont_2;
    OUTLINETEXTMETRICA otm;
    SIZE size_orig;
    INT width_orig, height_orig, lfWidth;
    XFORM xform;
    GLYPHMETRICS gm;
    MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
    MAT2 mat2 = { {0x8000,0}, {0,0}, {0,0}, {0x8000,0} };
    POINT pt;
    INT ret;

    if (!is_truetype_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    hdc = CreateCompatibleDC(0);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 72;
    hfont = create_font("outline", &lf);
    old_hfont = SelectObject(hdc, hfont);
    otm.otmSize = sizeof(otm);
    ok(GetOutlineTextMetricsA(hdc, sizeof(otm), &otm), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");

    test_font_metrics(hdc, hfont, lf.lfHeight, otm.otmTextMetrics.tmAveCharWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* font of otmEMSquare height helps to avoid a lot of rounding errors */
    lf.lfHeight = otm.otmEMSquare;
    lf.lfHeight = -lf.lfHeight;
    hfont = create_font("outline", &lf);
    old_hfont = SelectObject(hdc, hfont);
    otm.otmSize = sizeof(otm);
    ok(GetOutlineTextMetricsA(hdc, sizeof(otm), &otm), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    height_orig = otm.otmTextMetrics.tmHeight;
    lfWidth = otm.otmTextMetrics.tmAveCharWidth;

    /* test integer scaling 3x2 */
    lf.lfHeight = height_orig * 2;
    lf.lfWidth = lfWidth * 3;
    hfont = create_font("3x2", &lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 3, 2);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test integer scaling 3x3 */
    lf.lfHeight = height_orig * 3;
    lf.lfWidth = lfWidth * 3;
    hfont = create_font("3x3", &lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 3, 3);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test integer scaling 1x1 */
    lf.lfHeight = height_orig * 1;
    lf.lfWidth = lfWidth * 1;
    hfont = create_font("1x1", &lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test integer scaling 1x1 */
    lf.lfHeight = height_orig;
    lf.lfWidth = 0;
    hfont = create_font("1x1", &lf);
    old_hfont = SelectObject(hdc, hfont);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* with an identity matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    ok(gm.gmCellIncX == width_orig, "incX %d != %d\n", gm.gmCellIncX, width_orig);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);
    /* with a custom matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat2);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    ok(gm.gmCellIncX == width_orig/2, "incX %d != %d\n", gm.gmCellIncX, width_orig/2);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);

    /* Test that changing the DC transformation affects only the font
     * selected on this DC and doesn't affect the same font selected on
     * another DC.
     */
    hdc_2 = CreateCompatibleDC(0);
    old_hfont_2 = SelectObject(hdc_2, hfont);
    test_font_metrics(hdc_2, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    SetMapMode(hdc, MM_ANISOTROPIC);

    /* font metrics on another DC should be unchanged */
    test_font_metrics(hdc_2, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* test restrictions of compatibility mode GM_COMPATIBLE */
    /*  part 1: rescaling only X should not change font scaling on screen.
                So compressing the X axis by 2 is not done, and this
                appears as X scaling of 2 that no one requested. */
    SetWindowExtEx(hdc, 100, 100, NULL);
    SetViewportExtEx(hdc, 50, 100, NULL);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 2, 1);
    /* font metrics on another DC should be unchanged */
    test_font_metrics(hdc_2, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /*  part 2: rescaling only Y should change font scaling.
                As also X is scaled by a factor of 2, but this is not
                requested by the DC transformation, we get a scaling factor
                of 2 in the X coordinate. */
    SetViewportExtEx(hdc, 100, 200, NULL);
    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 2, 1);
    /* font metrics on another DC should be unchanged */
    test_font_metrics(hdc_2, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* restore scaling */
    SetMapMode(hdc, MM_TEXT);

    /* font metrics on another DC should be unchanged */
    test_font_metrics(hdc_2, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    SelectObject(hdc_2, old_hfont_2);
    DeleteDC(hdc_2);

    if (!SetGraphicsMode(hdc, GM_ADVANCED))
    {
        SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
        DeleteDC(hdc);
        skip("GM_ADVANCED is not supported on this platform\n");
        return;
    }

    xform.eM11 = 20.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 20.0f;
    xform.eDx = 0.0f;
    xform.eDy = 0.0f;

    SetLastError(0xdeadbeef);
    ret = SetWorldTransform(hdc, &xform);
    ok(ret, "SetWorldTransform error %u\n", GetLastError());

    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* with an identity matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(gm.gmCellIncX == pt.x, "incX %d != %d\n", gm.gmCellIncX, pt.x);
    ok(gm.gmCellIncX == 20 * width_orig, "incX %d != %d\n", gm.gmCellIncX, 20 * width_orig);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);
    /* with a custom matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat2);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(gm.gmCellIncX == pt.x/2, "incX %d != %d\n", gm.gmCellIncX, pt.x/2);
    ok(near_match(gm.gmCellIncX, 10 * width_orig), "incX %d != %d\n", gm.gmCellIncX, 10 * width_orig);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_LOMETRIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d, error %u\n", ret, GetLastError());

    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* with an identity matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(near_match(gm.gmCellIncX, pt.x), "incX %d != %d\n", gm.gmCellIncX, pt.x);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);
    /* with a custom matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat2);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(near_match(gm.gmCellIncX, (pt.x + 1)/2), "incX %d != %d\n", gm.gmCellIncX, (pt.x + 1)/2);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_TEXT);
    ok(ret == MM_LOMETRIC, "expected MM_LOMETRIC, got %d, error %u\n", ret, GetLastError());

    test_font_metrics(hdc, hfont, lf.lfHeight, lf.lfWidth, test_str, sizeof(test_str), &otm.otmTextMetrics, &size_orig, width_orig, 1, 1);

    /* with an identity matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(gm.gmCellIncX == pt.x, "incX %d != %d\n", gm.gmCellIncX, pt.x);
    ok(gm.gmCellIncX == 20 * width_orig, "incX %d != %d\n", gm.gmCellIncX, 20 * width_orig);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);
    /* with a custom matrix */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat2);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %d\n", GetLastError());
    trace("gm.gmCellIncX %d, width_orig %d\n", gm.gmCellIncX, width_orig);
    pt.x = width_orig; pt.y = 0;
    LPtoDP(hdc, &pt, 1);
    ok(gm.gmCellIncX == pt.x/2, "incX %d != %d\n", gm.gmCellIncX, pt.x/2);
    ok(gm.gmCellIncX == 10 * width_orig, "incX %d != %d\n", gm.gmCellIncX, 10 * width_orig);
    ok(gm.gmCellIncY == 0, "incY %d != 0\n", gm.gmCellIncY);

    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

static INT CALLBACK find_font_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    LOGFONT *lf = (LOGFONT *)lParam;

    if (elf->lfHeight == lf->lfHeight && !strcmp(elf->lfFaceName, lf->lfFaceName))
    {
        *lf = *elf;
        return 0; /* stop enumeration */
    }
    return 1; /* continue enumeration */
}

static void test_bitmap_font_metrics(void)
{
    static const struct font_data
    {
        const char face_name[LF_FACESIZE];
        int weight, height, ascent, descent, int_leading, ext_leading;
        int ave_char_width, max_char_width, dpi;
        DWORD ansi_bitfield;
    } fd[] =
    {
        { "MS Sans Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 16, 96, FS_LATIN1 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 96, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 19, 96, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 24, 96, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 20, 96, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 24, 96, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 6, 0, 12, 24, 96, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 25, 96, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 120, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 17, 120, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 25, 20, 5, 5, 0, 10, 21, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 6, 0, 12, 24, 120, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 24, 120, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 36, 29, 7, 6, 0, 15, 30, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 46, 37, 9, 6, 0, 20, 40, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 4, 8, 96, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 96, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11, 96, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 12, 96, FS_LATIN2 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 14, 96, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 16, 96, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 18, 96, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 19, 96, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 17, 96, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 22, 96, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 23, 96, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 23, 96, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 26, 96, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 27, 96, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 33, 96, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 34, 96, FS_CYRILLIC },

        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 14, 120, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 13, 120, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 120, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 15, 120, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 23, 18, 5, 3, 0, 10, 21, 120, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 23, 18, 5, 3, 0, 10, 19, 120, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 4, 0, 12, 23, 120, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_MEDIUM, 27, 22, 5, 2, 0, 12, 30, 120, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 33, 26, 7, 3, 0, 14, 30, 120, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_MEDIUM, 32, 25, 7, 2, 0, 14, 32, 120, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 43, 34, 9, 3, 0, 19, 39, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "Courier", FW_NORMAL, 13, 11, 2, 0, 0, 8, 8, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 25, 20, 5, 0, 0, 15, 15, 120, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 14, 96, FS_LATIN1 },
        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 15, 96, FS_LATIN2 | FS_CYRILLIC },
/*
 * TODO:  the system for CP932 should be NORMAL, not BOLD.  However that would
 *        require a new system.sfd for that font
 */
        { "System", FW_BOLD, 18, 16, 2, 0, 2, 8, 16, 96, FS_JISJAPAN },

        { "System", FW_BOLD, 20, 16, 4, 4, 0, 9, 14, 120, FS_LATIN1 },
        { "System", FW_BOLD, 20, 16, 4, 4, 0, 9, 17, 120, FS_LATIN2 | FS_CYRILLIC },

        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 2, 96, FS_LATIN1 },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 8, 96, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 2, 4, 96, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 1, 0, 3, 4, 96, FS_LATIN1 },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 1, 0, 2, 8, 96, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 0, 0, 3, 6, 96, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 13, 96, FS_LATIN1 },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 8, 96, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 0, 0, 4, 8, 96, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 7, 96, FS_LATIN1 },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 8, 96, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 0, 0, 5, 10, 96, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 4, 8, 96, FS_LATIN1 | FS_LATIN2 },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 96, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 0, 0, 6, 12, 96, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 0, 0, 7, 14, 96, FS_JISJAPAN },

        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 2, 120, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 8, 120, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 5, 120, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 8, 120, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 7, 120, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 8, 120, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 9, 120, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 120, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 12, 10, 2, 2, 0, 5, 10, 120, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 12, 10, 2, 2, 0, 6, 10, 120, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 13, 11, 2, 2, 0, 6, 12, 120, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 13, 11, 2, 2, 0, 6, 11, 120, FS_CYRILLIC },

        { "Fixedsys", FW_NORMAL, 15, 12, 3, 3, 0, 8, 8, 96, FS_LATIN1 | FS_LATIN2 },
        { "Fixedsys", FW_NORMAL, 16, 12, 4, 3, 0, 8, 8, 96, FS_CYRILLIC },
        { "FixedSys", FW_NORMAL, 18, 16, 2, 0, 0, 8, 16, 96, FS_JISJAPAN },

        /* The 120dpi version still has its dpi marked as 96 */
        { "Fixedsys", FW_NORMAL, 20, 16, 4, 2, 0, 10, 10, 96, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC }

        /* FIXME: add "Terminal" */
    };
    HDC hdc;
    LOGFONT lf;
    HFONT hfont, old_hfont;
    TEXTMETRIC tm;
    INT ret, i;

    hdc = CreateCompatibleDC(0);
    assert(hdc);

    for (i = 0; i < sizeof(fd)/sizeof(fd[0]); i++)
    {
        int bit;

        memset(&lf, 0, sizeof(lf));

        lf.lfHeight = fd[i].height;
        strcpy(lf.lfFaceName, fd[i].face_name);

        for(bit = 0; bit < 32; bit++)
        {
            DWORD fs[2];
            CHARSETINFO csi;

            fs[0] = 1L << bit;
            fs[1] = 0;
            if((fd[i].ansi_bitfield & fs[0]) == 0) continue;
            if(!TranslateCharsetInfo( fs, &csi, TCI_SRCFONTSIG )) continue;

            lf.lfCharSet = csi.ciCharset;
            ret = EnumFontFamiliesEx(hdc, &lf, find_font_proc, (LPARAM)&lf, 0);
            if (ret) continue;

            hfont = create_font(lf.lfFaceName, &lf);
            old_hfont = SelectObject(hdc, hfont);
            ok(GetTextMetrics(hdc, &tm), "GetTextMetrics error %d\n", GetLastError());
            if(fd[i].dpi == tm.tmDigitizedAspectX)
            {
                trace("found font %s, height %d charset %x dpi %d\n", lf.lfFaceName, lf.lfHeight, lf.lfCharSet, fd[i].dpi);
                ok(tm.tmWeight == fd[i].weight, "%s(%d): tm.tmWeight %d != %d\n", fd[i].face_name, fd[i].height, tm.tmWeight, fd[i].weight);
                ok(tm.tmHeight == fd[i].height, "%s(%d): tm.tmHeight %d != %d\n", fd[i].face_name, fd[i].height, tm.tmHeight, fd[i].height);
                ok(tm.tmAscent == fd[i].ascent, "%s(%d): tm.tmAscent %d != %d\n", fd[i].face_name, fd[i].height, tm.tmAscent, fd[i].ascent);
                ok(tm.tmDescent == fd[i].descent, "%s(%d): tm.tmDescent %d != %d\n", fd[i].face_name, fd[i].height, tm.tmDescent, fd[i].descent);
                ok(tm.tmInternalLeading == fd[i].int_leading, "%s(%d): tm.tmInternalLeading %d != %d\n", fd[i].face_name, fd[i].height, tm.tmInternalLeading, fd[i].int_leading);
                ok(tm.tmExternalLeading == fd[i].ext_leading, "%s(%d): tm.tmExternalLeading %d != %d\n", fd[i].face_name, fd[i].height, tm.tmExternalLeading, fd[i].ext_leading);
                ok(tm.tmAveCharWidth == fd[i].ave_char_width, "%s(%d): tm.tmAveCharWidth %d != %d\n", fd[i].face_name, fd[i].height, tm.tmAveCharWidth, fd[i].ave_char_width);

                /* Don't run the max char width test on System/ANSI_CHARSET.  We have extra characters in our font
                   that make the max width bigger */
                if(strcmp(lf.lfFaceName, "System") || lf.lfCharSet != ANSI_CHARSET)
                    ok(tm.tmMaxCharWidth == fd[i].max_char_width, "%s(%d): tm.tmMaxCharWidth %d != %d\n", fd[i].face_name, fd[i].height, tm.tmMaxCharWidth, fd[i].max_char_width);
            }
            SelectObject(hdc, old_hfont);
            DeleteObject(hfont);
        }
    }

    DeleteDC(hdc);
}

static void test_GdiGetCharDimensions(void)
{
    HDC hdc;
    TEXTMETRICW tm;
    LONG ret;
    SIZE size;
    LONG avgwidth, height;
    static const char szAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    if (!pGdiGetCharDimensions)
    {
        win_skip("GdiGetCharDimensions not available on this platform\n");
        return;
    }

    hdc = CreateCompatibleDC(NULL);

    GetTextExtentPoint(hdc, szAlphabet, strlen(szAlphabet), &size);
    avgwidth = ((size.cx / 26) + 1) / 2;

    ret = pGdiGetCharDimensions(hdc, &tm, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %d instead of %d\n", avgwidth, ret);
    ok(height == tm.tmHeight, "GdiGetCharDimensions should have set height to %d instead of %d\n", tm.tmHeight, height);

    ret = pGdiGetCharDimensions(hdc, &tm, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %d instead of %d\n", avgwidth, ret);

    ret = pGdiGetCharDimensions(hdc, NULL, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %d instead of %d\n", avgwidth, ret);

    height = 0;
    ret = pGdiGetCharDimensions(hdc, NULL, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %d instead of %d\n", avgwidth, ret);
    ok(height == size.cy, "GdiGetCharDimensions should have set height to %d instead of %d\n", size.cy, height);

    DeleteDC(hdc);
}

static void test_GetCharABCWidths(void)
{
    static const WCHAR str[] = {'a',0};
    BOOL ret;
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont;
    ABC abc[1];
    WORD glyphs[1];
    DWORD nb;

    if (!pGetCharABCWidthsW || !pGetCharABCWidthsI)
    {
        win_skip("GetCharABCWidthsW/I not available on this platform\n");
        return;
    }

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "System");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hdc = GetDC(0);
    hfont = SelectObject(hdc, hfont);

    nb = pGetGlyphIndicesW(hdc, str, 1, glyphs, 0);
    ok(nb == 1, "GetGlyphIndicesW should have returned 1\n");

    ret = pGetCharABCWidthsI(NULL, 0, 1, glyphs, abc);
    ok(!ret, "GetCharABCWidthsI should have failed\n");

    ret = pGetCharABCWidthsI(hdc, 0, 1, glyphs, NULL);
    ok(!ret, "GetCharABCWidthsI should have failed\n");

    ret = pGetCharABCWidthsI(hdc, 0, 1, glyphs, abc);
    ok(ret, "GetCharABCWidthsI should have succeeded\n");

    ret = pGetCharABCWidthsW(NULL, 'a', 'a', abc);
    ok(!ret, "GetCharABCWidthsW should have failed\n");

    ret = pGetCharABCWidthsW(hdc, 'a', 'a', NULL);
    ok(!ret, "GetCharABCWidthsW should have failed\n");

    ret = pGetCharABCWidthsW(hdc, 'a', 'a', abc);
    ok(!ret, "GetCharABCWidthsW should have failed\n");

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void test_text_extents(void)
{
    static const WCHAR wt[] = {'O','n','e','\n','t','w','o',' ','3',0};
    LPINT extents;
    INT i, len, fit1, fit2;
    LOGFONTA lf;
    TEXTMETRICA tm;
    HDC hdc;
    HFONT hfont;
    SIZE sz;
    SIZE sz1, sz2;

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hdc = GetDC(0);
    hfont = SelectObject(hdc, hfont);
    GetTextMetricsA(hdc, &tm);
    GetTextExtentPointA(hdc, "o", 1, &sz);
    ok(sz.cy == tm.tmHeight, "cy %d tmHeight %d\n", sz.cy, tm.tmHeight);

    SetLastError(0xdeadbeef);
    GetTextExtentExPointW(hdc, wt, 1, 1, &fit1, &fit2, &sz1);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Skipping remainder of text extents test on a Win9x platform\n");
        hfont = SelectObject(hdc, hfont);
        DeleteObject(hfont);
        ReleaseDC(0, hdc);
        return;
    }

    len = lstrlenW(wt);
    extents = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof extents[0]);
    extents[0] = 1;         /* So that the increasing sequence test will fail
                               if the extents array is untouched.  */
    GetTextExtentExPointW(hdc, wt, len, 32767, &fit1, extents, &sz1);
    GetTextExtentPointW(hdc, wt, len, &sz2);
    ok(sz1.cy == sz2.cy,
       "cy from GetTextExtentExPointW (%d) and GetTextExtentPointW (%d) differ\n", sz1.cy, sz2.cy);
    /* Because of the '\n' in the string GetTextExtentExPoint and
       GetTextExtentPoint return different widths under Win2k, but
       under WinXP they return the same width.  So we don't test that
       here. */

    for (i = 1; i < len; ++i)
        ok(extents[i-1] <= extents[i],
           "GetTextExtentExPointW generated a non-increasing sequence of partial extents (at position %d)\n",
           i);
    ok(extents[len-1] == sz1.cx, "GetTextExtentExPointW extents and size don't match\n");
    ok(0 <= fit1 && fit1 <= len, "GetTextExtentExPointW generated illegal value %d for fit\n", fit1);
    ok(0 < fit1, "GetTextExtentExPointW says we can't even fit one letter in 32767 logical units\n");
    GetTextExtentExPointW(hdc, wt, len, extents[2], &fit2, NULL, &sz2);
    ok(sz1.cx == sz2.cx && sz1.cy == sz2.cy, "GetTextExtentExPointW returned different sizes for the same string\n");
    ok(fit2 == 3, "GetTextExtentExPointW extents isn't consistent with fit\n");
    GetTextExtentExPointW(hdc, wt, len, extents[2]-1, &fit2, NULL, &sz2);
    ok(fit2 == 2, "GetTextExtentExPointW extents isn't consistent with fit\n");
    GetTextExtentExPointW(hdc, wt, 2, 0, NULL, extents + 2, &sz2);
    ok(extents[0] == extents[2] && extents[1] == extents[3],
       "GetTextExtentExPointW with lpnFit == NULL returns incorrect results\n");
    GetTextExtentExPointW(hdc, wt, 2, 0, NULL, NULL, &sz1);
    ok(sz1.cx == sz2.cx && sz1.cy == sz2.cy,
       "GetTextExtentExPointW with lpnFit and alpDx both NULL returns incorrect results\n");
    HeapFree(GetProcessHeap(), 0, extents);

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void test_GetGlyphIndices(void)
{
    HDC      hdc;
    HFONT    hfont;
    DWORD    charcount;
    LOGFONTA lf;
    DWORD    flags = 0;
    WCHAR    testtext[] = {'T','e','s','t',0xffff,0};
    WORD     glyphs[(sizeof(testtext)/2)-1];
    TEXTMETRIC textm;
    HFONT hOldFont;

    if (!pGetGlyphIndicesW) {
        win_skip("GetGlyphIndicesW not available on platform\n");
        return;
    }

    hdc = GetDC(0);

    ok(GetTextMetrics(hdc, &textm), "GetTextMetric failed\n");
    flags |= GGI_MARK_NONEXISTING_GLYPHS;
    charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
    ok((glyphs[4] == 0x001f || glyphs[4] == 0xffff /* Vista */), "GetGlyphIndicesW should have returned a nonexistent char not %04x\n", glyphs[4]);
    flags = 0;
    charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
    ok(glyphs[4] == textm.tmDefaultChar, "GetGlyphIndicesW should have returned a %04x not %04x\n",
                    textm.tmDefaultChar, glyphs[4]);

    if(!is_font_installed("Tahoma"))
    {
        skip("Tahoma is not installed so skipping this test\n");
        return;
    }
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Tahoma");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hOldFont = SelectObject(hdc, hfont);
    ok(GetTextMetrics(hdc, &textm), "GetTextMetric failed\n");
    flags |= GGI_MARK_NONEXISTING_GLYPHS;
    charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
    ok(glyphs[4] == 0xffff, "GetGlyphIndicesW should have returned 0xffff char not %04x\n", glyphs[4]);
    flags = 0;
    testtext[0] = textm.tmDefaultChar;
    charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
    ok(glyphs[0] == 0, "GetGlyphIndicesW for tmDefaultChar should be 0 not %04x\n", glyphs[0]);
    ok(glyphs[4] == 0, "GetGlyphIndicesW should have returned 0 not %04x\n", glyphs[4]);
    DeleteObject(SelectObject(hdc, hOldFont));
}

static void test_GetKerningPairs(void)
{
    static const struct kerning_data
    {
        const char face_name[LF_FACESIZE];
        LONG height;
        /* some interesting fields from OUTLINETEXTMETRIC */
        LONG tmHeight, tmAscent, tmDescent;
        UINT otmEMSquare;
        INT  otmAscent;
        INT  otmDescent;
        UINT otmLineGap;
        UINT otmsCapEmHeight;
        UINT otmsXHeight;
        INT  otmMacAscent;
        INT  otmMacDescent;
        UINT otmMacLineGap;
        UINT otmusMinimumPPEM;
        /* small subset of kerning pairs to test */
        DWORD total_kern_pairs;
        const KERNINGPAIR kern_pair[26];
    } kd[] =
    {
        {"Arial", 12, 12, 9, 3,
                  2048, 7, -2, 1, 5, 2, 8, -2, 0, 9,
                  26,
            {
                {' ','A',-1},{' ','T',0},{' ','Y',0},{'1','1',-1},
                {'A',' ',-1},{'A','T',-1},{'A','V',-1},{'A','W',0},
                {'A','Y',-1},{'A','v',0},{'A','w',0},{'A','y',0},
                {'F',',',-1},{'F','.',-1},{'F','A',-1},{'L',' ',0},
                {'L','T',-1},{'L','V',-1},{'L','W',-1},{'L','Y',-1},
                {915,912,+1},{915,913,-1},{910,912,+1},{910,913,-1},
                {933,970,+1},{933,972,-1}
                }
        },
        {"Arial", -34, 39, 32, 7,
                  2048, 25, -7, 5, 17, 9, 31, -7, 1, 9,
                  26,
            {
                {' ','A',-2},{' ','T',-1},{' ','Y',-1},{'1','1',-3},
                {'A',' ',-2},{'A','T',-3},{'A','V',-3},{'A','W',-1},
                {'A','Y',-3},{'A','v',-1},{'A','w',-1},{'A','y',-1},
                {'F',',',-4},{'F','.',-4},{'F','A',-2},{'L',' ',-1},
                {'L','T',-3},{'L','V',-3},{'L','W',-3},{'L','Y',-3},
                {915,912,+3},{915,913,-3},{910,912,+3},{910,913,-3},
                {933,970,+2},{933,972,-3}
            }
        },
        { "Arial", 120, 120, 97, 23,
                   2048, 79, -23, 16, 54, 27, 98, -23, 4, 9,
                   26,
            {
                {' ','A',-6},{' ','T',-2},{' ','Y',-2},{'1','1',-8},
                {'A',' ',-6},{'A','T',-8},{'A','V',-8},{'A','W',-4},
                {'A','Y',-8},{'A','v',-2},{'A','w',-2},{'A','y',-2},
                {'F',',',-12},{'F','.',-12},{'F','A',-6},{'L',' ',-4},
                {'L','T',-8},{'L','V',-8},{'L','W',-8},{'L','Y',-8},
                {915,912,+9},{915,913,-10},{910,912,+9},{910,913,-8},
                {933,970,+6},{933,972,-10}
            }
        },
#if 0 /* this set fails due to +1/-1 errors (rounding bug?), needs investigation. */
        { "Arial", 1024 /* usually 1/2 of EM Square */, 1024, 830, 194,
                   2048, 668, -193, 137, 459, 229, 830, -194, 30, 9,
                   26,
            {
                {' ','A',-51},{' ','T',-17},{' ','Y',-17},{'1','1',-68},
                {'A',' ',-51},{'A','T',-68},{'A','V',-68},{'A','W',-34},
                {'A','Y',-68},{'A','v',-17},{'A','w',-17},{'A','y',-17},
                {'F',',',-102},{'F','.',-102},{'F','A',-51},{'L',' ',-34},
                {'L','T',-68},{'L','V',-68},{'L','W',-68},{'L','Y',-68},
                {915,912,+73},{915,913,-84},{910,912,+76},{910,913,-68},
                {933,970,+54},{933,972,-83}
            }
        }
#endif
    };
    LOGFONT lf;
    HFONT hfont, hfont_old;
    KERNINGPAIR *kern_pair;
    HDC hdc;
    DWORD total_kern_pairs, ret, i, n, matches;

    hdc = GetDC(0);

    /* GetKerningPairsA maps unicode set of kerning pairs to current code page
     * which may render this test unusable, so we're trying to avoid that.
     */
    SetLastError(0xdeadbeef);
    GetKerningPairsW(hdc, 0, NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Skipping the GetKerningPairs test on a Win9x platform\n");
        ReleaseDC(0, hdc);
        return;
    }

    for (i = 0; i < sizeof(kd)/sizeof(kd[0]); i++)
    {
        OUTLINETEXTMETRICW otm;

        if (!is_font_installed(kd[i].face_name))
        {
            trace("%s is not installed so skipping this test\n", kd[i].face_name);
            continue;
        }

        trace("testing font %s, height %d\n", kd[i].face_name, kd[i].height);

        memset(&lf, 0, sizeof(lf));
        strcpy(lf.lfFaceName, kd[i].face_name);
        lf.lfHeight = kd[i].height;
        hfont = CreateFontIndirect(&lf);
        assert(hfont != 0);

        hfont_old = SelectObject(hdc, hfont);

        SetLastError(0xdeadbeef);
        otm.otmSize = sizeof(otm); /* just in case for Win9x compatibility */
        ok(GetOutlineTextMetricsW(hdc, sizeof(otm), &otm) == sizeof(otm), "GetOutlineTextMetricsW error %d\n", GetLastError());

        ok(kd[i].tmHeight == otm.otmTextMetrics.tmHeight, "expected %d, got %d\n",
           kd[i].tmHeight, otm.otmTextMetrics.tmHeight);
        ok(kd[i].tmAscent == otm.otmTextMetrics.tmAscent, "expected %d, got %d\n",
           kd[i].tmAscent, otm.otmTextMetrics.tmAscent);
        ok(kd[i].tmDescent == otm.otmTextMetrics.tmDescent, "expected %d, got %d\n",
           kd[i].tmDescent, otm.otmTextMetrics.tmDescent);

        ok(kd[i].otmEMSquare == otm.otmEMSquare, "expected %u, got %u\n",
           kd[i].otmEMSquare, otm.otmEMSquare);
        ok(kd[i].otmAscent == otm.otmAscent, "expected %d, got %d\n",
           kd[i].otmAscent, otm.otmAscent);
        ok(kd[i].otmDescent == otm.otmDescent, "expected %d, got %d\n",
           kd[i].otmDescent, otm.otmDescent);
        ok(kd[i].otmLineGap == otm.otmLineGap, "expected %u, got %u\n",
           kd[i].otmLineGap, otm.otmLineGap);
        ok(near_match(kd[i].otmMacDescent, otm.otmMacDescent), "expected %d, got %d\n",
           kd[i].otmMacDescent, otm.otmMacDescent);
todo_wine {
        ok(kd[i].otmsCapEmHeight == otm.otmsCapEmHeight, "expected %u, got %u\n",
           kd[i].otmsCapEmHeight, otm.otmsCapEmHeight);
        ok(kd[i].otmsXHeight == otm.otmsXHeight, "expected %u, got %u\n",
           kd[i].otmsXHeight, otm.otmsXHeight);
        ok(kd[i].otmMacAscent == otm.otmMacAscent, "expected %d, got %d\n",
           kd[i].otmMacAscent, otm.otmMacAscent);
        /* FIXME: this one sometimes succeeds due to expected 0, enable it when removing todo */
        if (0) ok(kd[i].otmMacLineGap == otm.otmMacLineGap, "expected %u, got %u\n",
           kd[i].otmMacLineGap, otm.otmMacLineGap);
        ok(kd[i].otmusMinimumPPEM == otm.otmusMinimumPPEM, "expected %u, got %u\n",
           kd[i].otmusMinimumPPEM, otm.otmusMinimumPPEM);
}

        total_kern_pairs = GetKerningPairsW(hdc, 0, NULL);
        trace("total_kern_pairs %u\n", total_kern_pairs);
        kern_pair = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pair));

#if 0 /* Win98 (GetKerningPairsA) and XP behave differently here, the test passes on XP */
        SetLastError(0xdeadbeef);
        ret = GetKerningPairsW(hdc, 0, kern_pair);
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "got error %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());
        ok(ret == 0, "got %lu, expected 0\n", ret);
#endif

        ret = GetKerningPairsW(hdc, 100, NULL);
        ok(ret == total_kern_pairs, "got %u, expected %u\n", ret, total_kern_pairs);

        ret = GetKerningPairsW(hdc, total_kern_pairs/2, kern_pair);
        ok(ret == total_kern_pairs/2, "got %u, expected %u\n", ret, total_kern_pairs/2);

        ret = GetKerningPairsW(hdc, total_kern_pairs, kern_pair);
        ok(ret == total_kern_pairs, "got %u, expected %u\n", ret, total_kern_pairs);

        matches = 0;

        for (n = 0; n < ret; n++)
        {
            DWORD j;
#if 0
            if (kern_pair[n].wFirst < 127 && kern_pair[n].wSecond < 127)
                trace("{'%c','%c',%d},\n",
                      kern_pair[n].wFirst, kern_pair[n].wSecond, kern_pair[n].iKernAmount);
#endif
            for (j = 0; j < kd[i].total_kern_pairs; j++)
            {
                if (kern_pair[n].wFirst == kd[i].kern_pair[j].wFirst &&
                    kern_pair[n].wSecond == kd[i].kern_pair[j].wSecond)
                {
                    ok(kern_pair[n].iKernAmount == kd[i].kern_pair[j].iKernAmount,
                       "pair %d:%d got %d, expected %d\n",
                       kern_pair[n].wFirst, kern_pair[n].wSecond,
                       kern_pair[n].iKernAmount, kd[i].kern_pair[j].iKernAmount);
                    matches++;
                }
            }
        }

        ok(matches == kd[i].total_kern_pairs, "got matches %u, expected %u\n",
           matches, kd[i].total_kern_pairs);

        HeapFree(GetProcessHeap(), 0, kern_pair);

        SelectObject(hdc, hfont_old);
        DeleteObject(hfont);
    }

    ReleaseDC(0, hdc);
}

static void test_GetOutlineTextMetrics(void)
{
    OUTLINETEXTMETRIC *otm;
    LOGFONT lf;
    HFONT hfont, hfont_old;
    HDC hdc;
    DWORD ret, otm_size;
    LPSTR unset_ptr;

    if (!is_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    hdc = GetDC(0);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = -13;
    lf.lfWeight = FW_NORMAL;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    lf.lfQuality = PROOF_QUALITY;
    hfont = CreateFontIndirect(&lf);
    assert(hfont != 0);

    hfont_old = SelectObject(hdc, hfont);
    otm_size = GetOutlineTextMetrics(hdc, 0, NULL);
    trace("otm buffer size %u (0x%x)\n", otm_size, otm_size);

    otm = HeapAlloc(GetProcessHeap(), 0, otm_size);

    memset(otm, 0xAA, otm_size);
    SetLastError(0xdeadbeef);
    otm->otmSize = sizeof(*otm); /* just in case for Win9x compatibility */
    ret = GetOutlineTextMetrics(hdc, otm->otmSize, otm);
    ok(ret == 1 /* Win9x */ ||
       ret == otm->otmSize /* XP*/,
       "expected %u, got %u, error %d\n", otm->otmSize, ret, GetLastError());
    if (ret != 1) /* Win9x doesn't care about pointing beyond of the buffer */
    {
        ok(otm->otmpFamilyName == NULL, "expected NULL got %p\n", otm->otmpFamilyName);
        ok(otm->otmpFaceName == NULL, "expected NULL got %p\n", otm->otmpFaceName);
        ok(otm->otmpStyleName == NULL, "expected NULL got %p\n", otm->otmpStyleName);
        ok(otm->otmpFullName == NULL, "expected NULL got %p\n", otm->otmpFullName);
    }

    memset(otm, 0xAA, otm_size);
    SetLastError(0xdeadbeef);
    otm->otmSize = otm_size; /* just in case for Win9x compatibility */
    ret = GetOutlineTextMetrics(hdc, otm->otmSize, otm);
    ok(ret == 1 /* Win9x */ ||
       ret == otm->otmSize /* XP*/,
       "expected %u, got %u, error %d\n", otm->otmSize, ret, GetLastError());
    if (ret != 1) /* Win9x doesn't care about pointing beyond of the buffer */
    {
        ok(otm->otmpFamilyName != NULL, "expected not NULL got %p\n", otm->otmpFamilyName);
        ok(otm->otmpFaceName != NULL, "expected not NULL got %p\n", otm->otmpFaceName);
        ok(otm->otmpStyleName != NULL, "expected not NULL got %p\n", otm->otmpStyleName);
        ok(otm->otmpFullName != NULL, "expected not NULL got %p\n", otm->otmpFullName);
    }

    /* ask about truncated data */
    memset(otm, 0xAA, otm_size);
    memset(&unset_ptr, 0xAA, sizeof(unset_ptr));
    SetLastError(0xdeadbeef);
    otm->otmSize = sizeof(*otm) - sizeof(LPSTR); /* just in case for Win9x compatibility */
    ret = GetOutlineTextMetrics(hdc, otm->otmSize, otm);
    ok(ret == 1 /* Win9x */ ||
       ret == otm->otmSize /* XP*/,
       "expected %u, got %u, error %d\n", otm->otmSize, ret, GetLastError());
    if (ret != 1) /* Win9x doesn't care about pointing beyond of the buffer */
    {
        ok(otm->otmpFamilyName == NULL, "expected NULL got %p\n", otm->otmpFamilyName);
        ok(otm->otmpFaceName == NULL, "expected NULL got %p\n", otm->otmpFaceName);
        ok(otm->otmpStyleName == NULL, "expected NULL got %p\n", otm->otmpStyleName);
    }
    ok(otm->otmpFullName == unset_ptr, "expected %p got %p\n", unset_ptr, otm->otmpFullName);

    HeapFree(GetProcessHeap(), 0, otm);

    SelectObject(hdc, hfont_old);
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
}

static void testJustification(HDC hdc, PSTR str, RECT *clientArea)
{
    INT         x, y,
                breakCount,
                justifiedWidth = 0, /* to test GetTextExtentExPointW() */
                areaWidth = clientArea->right - clientArea->left,
                nErrors = 0, e;
    BOOL        lastExtent = FALSE;
    PSTR        pFirstChar, pLastChar;
    SIZE        size;
    TEXTMETRICA tm;
    struct err
    {
        char extent[100];
        int  GetTextExtentExPointWWidth;
    } error[10];

    GetTextMetricsA(hdc, &tm);
    y = clientArea->top;
    do {
        breakCount = 0;
        while (*str == tm.tmBreakChar) str++; /* skip leading break chars */
        pFirstChar = str;

        do {
            pLastChar = str;

            /* if not at the end of the string, ... */
            if (*str == '\0') break;
            /* ... add the next word to the current extent */
            while (*str != '\0' && *str++ != tm.tmBreakChar);
            breakCount++;
            SetTextJustification(hdc, 0, 0);
            GetTextExtentPoint32(hdc, pFirstChar, str - pFirstChar - 1, &size);
        } while ((int) size.cx < areaWidth);

        /* ignore trailing break chars */
        breakCount--;
        while (*(pLastChar - 1) == tm.tmBreakChar)
        {
            pLastChar--;
            breakCount--;
        }

        if (*str == '\0' || breakCount <= 0) pLastChar = str;

        SetTextJustification(hdc, 0, 0);
        GetTextExtentPoint32(hdc, pFirstChar, pLastChar - pFirstChar, &size);

        /* do not justify the last extent */
        if (*str != '\0' && breakCount > 0)
        {
            SetTextJustification(hdc, areaWidth - size.cx, breakCount);
            GetTextExtentPoint32(hdc, pFirstChar, pLastChar - pFirstChar, &size);
            justifiedWidth = size.cx;
        }
        else lastExtent = TRUE;

        x = clientArea->left;

        /* catch errors and report them */
        if (!lastExtent && (justifiedWidth != areaWidth))
        {
            memset(error[nErrors].extent, 0, 100);
            memcpy(error[nErrors].extent, pFirstChar, pLastChar - pFirstChar);
            error[nErrors].GetTextExtentExPointWWidth = justifiedWidth;
            nErrors++;
        }

        y += size.cy;
        str = pLastChar;
    } while (*str && y < clientArea->bottom);

    for (e = 0; e < nErrors; e++)
    {
        /* The width returned by GetTextExtentPoint32() is exactly the same
           returned by GetTextExtentExPointW() - see dlls/gdi32/font.c */
        ok(error[e].GetTextExtentExPointWWidth == areaWidth,
            "GetTextExtentPointW() for \"%s\" should have returned a width of %d, not %d.\n",
            error[e].extent, areaWidth, error[e].GetTextExtentExPointWWidth);
    }
}

static void test_SetTextJustification(void)
{
    HDC hdc;
    RECT clientArea;
    LOGFONTA lf;
    HFONT hfont;
    HWND hwnd;
    static char testText[] =
            "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do "
            "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
            "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
            "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
            "reprehenderit in voluptate velit esse cillum dolore eu fugiat "
            "nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
            "sunt in culpa qui officia deserunt mollit anim id est laborum.";

    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0, 400,400, 0, 0, 0, NULL);
    GetClientRect( hwnd, &clientArea );
    hdc = GetDC( hwnd );

    memset(&lf, 0, sizeof lf);
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 20;
    lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(lf.lfFaceName, "Times New Roman");
    hfont = create_font("Times New Roman", &lf);
    SelectObject(hdc, hfont);

    testJustification(hdc, testText, &clientArea);

    DeleteObject(hfont);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static BOOL get_glyph_indices(INT charset, UINT code_page, WORD *idx, UINT count, BOOL unicode)
{
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, hfont_old;
    CHARSETINFO csi;
    FONTSIGNATURE fs;
    INT cs;
    DWORD i, ret;
    char name[64];

    assert(count <= 128);

    memset(&lf, 0, sizeof(lf));

    lf.lfCharSet = charset;
    lf.lfHeight = 10;
    lstrcpyA(lf.lfFaceName, "Arial");
    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());

    hdc = GetDC(0);
    hfont_old = SelectObject(hdc, hfont);

    cs = GetTextCharsetInfo(hdc, &fs, 0);
    ok(cs == charset, "expected %d, got %d\n", charset, cs);

    SetLastError(0xdeadbeef);
    ret = GetTextFaceA(hdc, sizeof(name), name);
    ok(ret, "GetTextFaceA error %u\n", GetLastError());

    if (charset == SYMBOL_CHARSET)
    {
        ok(strcmp("Arial", name), "face name should NOT be Arial\n");
        ok(fs.fsCsb[0] & (1 << 31), "symbol encoding should be available\n");
    }
    else
    {
        ok(!strcmp("Arial", name), "face name should be Arial, not %s\n", name);
        ok(!(fs.fsCsb[0] & (1 << 31)), "symbol encoding should NOT be available\n");
    }

    if (!TranslateCharsetInfo((DWORD *)(INT_PTR)cs, &csi, TCI_SRCCHARSET))
    {
        trace("Can't find codepage for charset %d\n", cs);
        ReleaseDC(0, hdc);
        return FALSE;
    }
    ok(csi.ciACP == code_page, "expected %d, got %d\n", code_page, csi.ciACP);

    if (unicode)
    {
        char ansi_buf[128];
        WCHAR unicode_buf[128];

        for (i = 0; i < count; i++) ansi_buf[i] = (BYTE)(i + 128);

        MultiByteToWideChar(code_page, 0, ansi_buf, count, unicode_buf, count);

        SetLastError(0xdeadbeef);
        ret = pGetGlyphIndicesW(hdc, unicode_buf, count, idx, 0);
        ok(ret == count, "GetGlyphIndicesW error %u\n", GetLastError());
    }
    else
    {
        char ansi_buf[128];

        for (i = 0; i < count; i++) ansi_buf[i] = (BYTE)(i + 128);

        SetLastError(0xdeadbeef);
        ret = pGetGlyphIndicesA(hdc, ansi_buf, count, idx, 0);
        ok(ret == count, "GetGlyphIndicesA error %u\n", GetLastError());
    }

    SelectObject(hdc, hfont_old);
    DeleteObject(hfont);

    ReleaseDC(0, hdc);

    return TRUE;
}

static void test_font_charset(void)
{
    static struct charset_data
    {
        INT charset;
        UINT code_page;
        WORD font_idxA[128], font_idxW[128];
    } cd[] =
    {
        { ANSI_CHARSET, 1252 },
        { RUSSIAN_CHARSET, 1251 },
        { SYMBOL_CHARSET, CP_SYMBOL } /* keep it as the last one */
    };
    int i;

    if (!pGetGlyphIndicesA || !pGetGlyphIndicesW)
    {
        win_skip("Skipping the font charset test on a Win9x platform\n");
        return;
    }

    if (!is_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    for (i = 0; i < sizeof(cd)/sizeof(cd[0]); i++)
    {
        if (cd[i].charset == SYMBOL_CHARSET)
        {
            if (!is_font_installed("Symbol") && !is_font_installed("Wingdings"))
            {
                skip("Symbol or Wingdings is not installed\n");
                break;
            }
        }
        get_glyph_indices(cd[i].charset, cd[i].code_page, cd[i].font_idxA, 128, FALSE);
        get_glyph_indices(cd[i].charset, cd[i].code_page, cd[i].font_idxW, 128, TRUE);
        ok(!memcmp(cd[i].font_idxA, cd[i].font_idxW, 128*sizeof(WORD)), "%d: indices don't match\n", i);
    }

    ok(memcmp(cd[0].font_idxW, cd[1].font_idxW, 128*sizeof(WORD)), "0 vs 1: indices shouldn't match\n");
    if (i > 2)
    {
        ok(memcmp(cd[0].font_idxW, cd[2].font_idxW, 128*sizeof(WORD)), "0 vs 2: indices shouldn't match\n");
        ok(memcmp(cd[1].font_idxW, cd[2].font_idxW, 128*sizeof(WORD)), "1 vs 2: indices shouldn't match\n");
    }
    else
        skip("Symbol or Wingdings is not installed\n");
}

static void test_GetFontUnicodeRanges(void)
{
    LOGFONTA lf;
    HDC hdc;
    HFONT hfont, hfont_old;
    DWORD size;
    GLYPHSET *gs;

    if (!pGetFontUnicodeRanges)
    {
        win_skip("GetFontUnicodeRanges not available before W2K\n");
        return;
    }

    memset(&lf, 0, sizeof(lf));
    lstrcpyA(lf.lfFaceName, "Arial");
    hfont = create_font("Arial", &lf);

    hdc = GetDC(0);
    hfont_old = SelectObject(hdc, hfont);

    size = pGetFontUnicodeRanges(NULL, NULL);
    ok(!size, "GetFontUnicodeRanges succeeded unexpectedly\n");

    size = pGetFontUnicodeRanges(hdc, NULL);
    ok(size, "GetFontUnicodeRanges failed unexpectedly\n");

    gs = HeapAlloc(GetProcessHeap(), 0, size);

    size = pGetFontUnicodeRanges(hdc, gs);
    ok(size, "GetFontUnicodeRanges failed\n");
#if 0
    for (i = 0; i < gs->cRanges; i++)
        trace("%03d wcLow %04x cGlyphs %u\n", i, gs->ranges[i].wcLow, gs->ranges[i].cGlyphs);
#endif
    trace("found %u ranges\n", gs->cRanges);

    HeapFree(GetProcessHeap(), 0, gs);

    SelectObject(hdc, hfont_old);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

#define MAX_ENUM_FONTS 4096

struct enum_font_data
{
    int total;
    LOGFONT lf[MAX_ENUM_FONTS];
};

struct enum_font_dataW
{
    int total;
    LOGFONTW lf[MAX_ENUM_FONTS];
};

static INT CALLBACK arial_enum_proc(const LOGFONT *lf, const TEXTMETRIC *tm, DWORD type, LPARAM lParam)
{
    struct enum_font_data *efd = (struct enum_font_data *)lParam;

    ok(lf->lfHeight == tm->tmHeight, "lfHeight %d != tmHeight %d\n", lf->lfHeight, tm->tmHeight);

    if (type != TRUETYPE_FONTTYPE) return 1;
#if 0
    trace("enumed font \"%s\", charset %d, height %d, weight %d, italic %d\n",
          lf->lfFaceName, lf->lfCharSet, lf->lfHeight, lf->lfWeight, lf->lfItalic);
#endif
    if (efd->total < MAX_ENUM_FONTS)
        efd->lf[efd->total++] = *lf;
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static INT CALLBACK arial_enum_procw(const LOGFONTW *lf, const TEXTMETRICW *tm, DWORD type, LPARAM lParam)
{
    struct enum_font_dataW *efd = (struct enum_font_dataW *)lParam;

    ok(lf->lfHeight == tm->tmHeight, "lfHeight %d != tmHeight %d\n", lf->lfHeight, tm->tmHeight);

    if (type != TRUETYPE_FONTTYPE) return 1;
#if 0
    trace("enumed font \"%s\", charset %d, height %d, weight %d, italic %d\n",
          lf->lfFaceName, lf->lfCharSet, lf->lfHeight, lf->lfWeight, lf->lfItalic);
#endif
    if (efd->total < MAX_ENUM_FONTS)
        efd->lf[efd->total++] = *lf;
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static void get_charset_stats(struct enum_font_data *efd,
                              int *ansi_charset, int *symbol_charset,
                              int *russian_charset)
{
    int i;

    *ansi_charset = 0;
    *symbol_charset = 0;
    *russian_charset = 0;

    for (i = 0; i < efd->total; i++)
    {
        switch (efd->lf[i].lfCharSet)
        {
        case ANSI_CHARSET:
            (*ansi_charset)++;
            break;
        case SYMBOL_CHARSET:
            (*symbol_charset)++;
            break;
        case RUSSIAN_CHARSET:
            (*russian_charset)++;
            break;
        }
    }
}

static void get_charset_statsW(struct enum_font_dataW *efd,
                              int *ansi_charset, int *symbol_charset,
                              int *russian_charset)
{
    int i;

    *ansi_charset = 0;
    *symbol_charset = 0;
    *russian_charset = 0;

    for (i = 0; i < efd->total; i++)
    {
        switch (efd->lf[i].lfCharSet)
        {
        case ANSI_CHARSET:
            (*ansi_charset)++;
            break;
        case SYMBOL_CHARSET:
            (*symbol_charset)++;
            break;
        case RUSSIAN_CHARSET:
            (*russian_charset)++;
            break;
        }
    }
}

static void test_EnumFontFamilies(const char *font_name, INT font_charset)
{
    struct enum_font_data efd;
    struct enum_font_dataW efdw;
    LOGFONT lf;
    HDC hdc;
    int i, ret, ansi_charset, symbol_charset, russian_charset;

    trace("Testing font %s, charset %d\n", *font_name ? font_name : "<empty>", font_charset);

    if (*font_name && !is_truetype_font_installed(font_name))
    {
        skip("%s is not installed\n", font_name);
        return;
    }

    hdc = GetDC(0);

    /* Observed behaviour: EnumFontFamilies enumerates aliases like "Arial Cyr"
     * while EnumFontFamiliesEx doesn't.
     */
    if (!*font_name && font_charset == DEFAULT_CHARSET) /* do it only once */
    {
        /*
         * Use EnumFontFamiliesW since win98 crashes when the
         *    second parameter is NULL using EnumFontFamilies
         */
        efdw.total = 0;
        SetLastError(0xdeadbeef);
        ret = EnumFontFamiliesW(hdc, NULL, arial_enum_procw, (LPARAM)&efdw);
        ok(ret || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED, "EnumFontFamiliesW error %u\n", GetLastError());
        if(ret)
        {
            get_charset_statsW(&efdw, &ansi_charset, &symbol_charset, &russian_charset);
            trace("enumerated ansi %d, symbol %d, russian %d fonts for NULL\n",
                  ansi_charset, symbol_charset, russian_charset);
            ok(efdw.total > 0, "fonts enumerated: NULL\n");
            ok(ansi_charset > 0, "NULL family should enumerate ANSI_CHARSET\n");
            ok(symbol_charset > 0, "NULL family should enumerate SYMBOL_CHARSET\n");
            ok(russian_charset > 0 ||
               broken(russian_charset == 0), /* NT4 */
               "NULL family should enumerate RUSSIAN_CHARSET\n");
        }

        efdw.total = 0;
        SetLastError(0xdeadbeef);
        ret = EnumFontFamiliesExW(hdc, NULL, arial_enum_procw, (LPARAM)&efdw, 0);
        ok(ret || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED, "EnumFontFamiliesExW error %u\n", GetLastError());
        if(ret)
        {
            get_charset_statsW(&efdw, &ansi_charset, &symbol_charset, &russian_charset);
            trace("enumerated ansi %d, symbol %d, russian %d fonts for NULL\n",
                  ansi_charset, symbol_charset, russian_charset);
            ok(efdw.total > 0, "fonts enumerated: NULL\n");
            ok(ansi_charset > 0, "NULL family should enumerate ANSI_CHARSET\n");
            ok(symbol_charset > 0, "NULL family should enumerate SYMBOL_CHARSET\n");
            ok(russian_charset > 0, "NULL family should enumerate RUSSIAN_CHARSET\n");
        }
    }

    efd.total = 0;
    SetLastError(0xdeadbeef);
    ret = EnumFontFamilies(hdc, font_name, arial_enum_proc, (LPARAM)&efd);
    ok(ret, "EnumFontFamilies error %u\n", GetLastError());
    get_charset_stats(&efd, &ansi_charset, &symbol_charset, &russian_charset);
    trace("enumerated ansi %d, symbol %d, russian %d fonts for %s\n",
          ansi_charset, symbol_charset, russian_charset,
          *font_name ? font_name : "<empty>");
    if (*font_name)
        ok(efd.total > 0, "no fonts enumerated: %s\n", font_name);
    else
        ok(!efd.total, "no fonts should be enumerated for empty font_name\n");
    for (i = 0; i < efd.total; i++)
    {
/* FIXME: remove completely once Wine is fixed */
if (efd.lf[i].lfCharSet != font_charset)
{
todo_wine
    ok(efd.lf[i].lfCharSet == font_charset, "%d: got charset %d\n", i, efd.lf[i].lfCharSet);
}
else
        ok(efd.lf[i].lfCharSet == font_charset, "%d: got charset %d\n", i, efd.lf[i].lfCharSet);
        ok(!lstrcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
           font_name, efd.lf[i].lfFaceName);
    }

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    lstrcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesEx(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
    ok(ret, "EnumFontFamiliesEx error %u\n", GetLastError());
    get_charset_stats(&efd, &ansi_charset, &symbol_charset, &russian_charset);
    trace("enumerated ansi %d, symbol %d, russian %d fonts for %s ANSI_CHARSET\n",
          ansi_charset, symbol_charset, russian_charset,
          *font_name ? font_name : "<empty>");
    if (font_charset == SYMBOL_CHARSET)
    {
        if (*font_name)
            ok(efd.total == 0, "no fonts should be enumerated: %s ANSI_CHARSET\n", font_name);
        else
            ok(efd.total > 0, "no fonts enumerated: %s\n", font_name);
    }
    else
    {
        ok(efd.total > 0, "no fonts enumerated: %s ANSI_CHARSET\n", font_name);
        for (i = 0; i < efd.total; i++)
        {
            ok(efd.lf[i].lfCharSet == ANSI_CHARSET, "%d: got charset %d\n", i, efd.lf[i].lfCharSet);
            if (*font_name)
                ok(!lstrcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
                   font_name, efd.lf[i].lfFaceName);
        }
    }

    /* DEFAULT_CHARSET should enumerate all available charsets */
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    EnumFontFamiliesEx(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
    ok(ret, "EnumFontFamiliesEx error %u\n", GetLastError());
    get_charset_stats(&efd, &ansi_charset, &symbol_charset, &russian_charset);
    trace("enumerated ansi %d, symbol %d, russian %d fonts for %s DEFAULT_CHARSET\n",
          ansi_charset, symbol_charset, russian_charset,
          *font_name ? font_name : "<empty>");
    ok(efd.total > 0, "no fonts enumerated: %s DEFAULT_CHARSET\n", font_name);
    for (i = 0; i < efd.total; i++)
    {
        if (*font_name)
            ok(!lstrcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
               font_name, efd.lf[i].lfFaceName);
    }
    if (*font_name)
    {
        switch (font_charset)
        {
        case ANSI_CHARSET:
            ok(ansi_charset > 0,
               "ANSI_CHARSET should enumerate ANSI_CHARSET for %s\n", font_name);
            ok(!symbol_charset,
               "ANSI_CHARSET should NOT enumerate SYMBOL_CHARSET for %s\n", font_name);
            ok(russian_charset > 0,
               "ANSI_CHARSET should enumerate RUSSIAN_CHARSET for %s\n", font_name);
            break;
        case SYMBOL_CHARSET:
            ok(!ansi_charset,
               "SYMBOL_CHARSET should NOT enumerate ANSI_CHARSET for %s\n", font_name);
            ok(symbol_charset,
               "SYMBOL_CHARSET should enumerate SYMBOL_CHARSET for %s\n", font_name);
            ok(!russian_charset,
               "SYMBOL_CHARSET should NOT enumerate RUSSIAN_CHARSET for %s\n", font_name);
            break;
        case DEFAULT_CHARSET:
            ok(ansi_charset > 0,
               "DEFAULT_CHARSET should enumerate ANSI_CHARSET for %s\n", font_name);
            ok(symbol_charset > 0,
               "DEFAULT_CHARSET should enumerate SYMBOL_CHARSET for %s\n", font_name);
            ok(russian_charset > 0,
               "DEFAULT_CHARSET should enumerate RUSSIAN_CHARSET for %s\n", font_name);
            break;
        }
    }
    else
    {
        ok(ansi_charset > 0,
           "DEFAULT_CHARSET should enumerate ANSI_CHARSET for %s\n", *font_name ? font_name : "<empty>");
        ok(symbol_charset > 0,
           "DEFAULT_CHARSET should enumerate SYMBOL_CHARSET for %s\n", *font_name ? font_name : "<empty>");
        ok(russian_charset > 0,
           "DEFAULT_CHARSET should enumerate RUSSIAN_CHARSET for %s\n", *font_name ? font_name : "<empty>");
    }

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = SYMBOL_CHARSET;
    lstrcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    EnumFontFamiliesEx(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
    ok(ret, "EnumFontFamiliesEx error %u\n", GetLastError());
    get_charset_stats(&efd, &ansi_charset, &symbol_charset, &russian_charset);
    trace("enumerated ansi %d, symbol %d, russian %d fonts for %s SYMBOL_CHARSET\n",
          ansi_charset, symbol_charset, russian_charset,
          *font_name ? font_name : "<empty>");
    if (*font_name && font_charset == ANSI_CHARSET)
        ok(efd.total == 0, "no fonts should be enumerated: %s SYMBOL_CHARSET\n", font_name);
    else
    {
        ok(efd.total > 0, "no fonts enumerated: %s SYMBOL_CHARSET\n", font_name);
        for (i = 0; i < efd.total; i++)
        {
            ok(efd.lf[i].lfCharSet == SYMBOL_CHARSET, "%d: got charset %d\n", i, efd.lf[i].lfCharSet);
            if (*font_name)
                ok(!lstrcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
                   font_name, efd.lf[i].lfFaceName);
        }

        ok(!ansi_charset,
           "SYMBOL_CHARSET should NOT enumerate ANSI_CHARSET for %s\n", *font_name ? font_name : "<empty>");
        ok(symbol_charset > 0,
           "SYMBOL_CHARSET should enumerate SYMBOL_CHARSET for %s\n", *font_name ? font_name : "<empty>");
        ok(!russian_charset,
           "SYMBOL_CHARSET should NOT enumerate RUSSIAN_CHARSET for %s\n", *font_name ? font_name : "<empty>");
    }

    ReleaseDC(0, hdc);
}

static void test_negative_width(HDC hdc, const LOGFONTA *lf)
{
    HFONT hfont, hfont_prev;
    DWORD ret;
    GLYPHMETRICS gm1, gm2;
    LOGFONTA lf2 = *lf;
    WORD idx;
    MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };

    if(!pGetGlyphIndicesA)
        return;

    /* negative widths are handled just as positive ones */
    lf2.lfWidth = -lf->lfWidth;

    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(lf);
    ok(hfont != 0, "CreateFontIndirect error %u\n", GetLastError());
    check_font("original", lf, hfont);

    hfont_prev = SelectObject(hdc, hfont);

    ret = pGetGlyphIndicesA(hdc, "x", 1, &idx, GGI_MARK_NONEXISTING_GLYPHS);
    if (ret == GDI_ERROR || idx == 0xffff)
    {
        SelectObject(hdc, hfont_prev);
        DeleteObject(hfont);
        skip("Font %s doesn't contain 'x', skipping the test\n", lf->lfFaceName);
        return;
    }

    /* filling with 0xaa causes false pass under WINEDEBUG=warn+heap */
    memset(&gm1, 0xab, sizeof(gm1));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'x', GGO_METRICS, &gm1, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutline error 0x%x\n", GetLastError());

    SelectObject(hdc, hfont_prev);
    DeleteObject(hfont);

    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(&lf2);
    ok(hfont != 0, "CreateFontIndirect error %u\n", GetLastError());
    check_font("negative width", &lf2, hfont);

    hfont_prev = SelectObject(hdc, hfont);

    memset(&gm2, 0xbb, sizeof(gm2));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'x', GGO_METRICS, &gm2, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutline error 0x%x\n", GetLastError());

    SelectObject(hdc, hfont_prev);
    DeleteObject(hfont);

    ok(gm1.gmBlackBoxX == gm2.gmBlackBoxX &&
       gm1.gmBlackBoxY == gm2.gmBlackBoxY &&
       gm1.gmptGlyphOrigin.x == gm2.gmptGlyphOrigin.x &&
       gm1.gmptGlyphOrigin.y == gm2.gmptGlyphOrigin.y &&
       gm1.gmCellIncX == gm2.gmCellIncX &&
       gm1.gmCellIncY == gm2.gmCellIncY,
       "gm1=%d,%d,%d,%d,%d,%d gm2=%d,%d,%d,%d,%d,%d\n",
       gm1.gmBlackBoxX, gm1.gmBlackBoxY, gm1.gmptGlyphOrigin.x,
       gm1.gmptGlyphOrigin.y, gm1.gmCellIncX, gm1.gmCellIncY,
       gm2.gmBlackBoxX, gm2.gmBlackBoxY, gm2.gmptGlyphOrigin.x,
       gm2.gmptGlyphOrigin.y, gm2.gmCellIncX, gm2.gmCellIncY);
}

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include "pshpack2.h"
typedef struct
{
    USHORT version;
    SHORT xAvgCharWidth;
    USHORT usWeightClass;
    USHORT usWidthClass;
    SHORT fsType;
    SHORT ySubscriptXSize;
    SHORT ySubscriptYSize;
    SHORT ySubscriptXOffset;
    SHORT ySubscriptYOffset;
    SHORT ySuperscriptXSize;
    SHORT ySuperscriptYSize;
    SHORT ySuperscriptXOffset;
    SHORT ySuperscriptYOffset;
    SHORT yStrikeoutSize;
    SHORT yStrikeoutPosition;
    SHORT sFamilyClass;
    PANOSE panose;
    ULONG ulUnicodeRange1;
    ULONG ulUnicodeRange2;
    ULONG ulUnicodeRange3;
    ULONG ulUnicodeRange4;
    CHAR achVendID[4];
    USHORT fsSelection;
    USHORT usFirstCharIndex;
    USHORT usLastCharIndex;
    /* According to the Apple spec, original version didn't have the below fields,
     * version numbers were taked from the OpenType spec.
     */
    /* version 0 (TrueType 1.5) */
    USHORT sTypoAscender;
    USHORT sTypoDescender;
    USHORT sTypoLineGap;
    USHORT usWinAscent;
    USHORT usWinDescent;
    /* version 1 (TrueType 1.66) */
    ULONG ulCodePageRange1;
    ULONG ulCodePageRange2;
    /* version 2 (OpenType 1.2) */
    SHORT sxHeight;
    SHORT sCapHeight;
    USHORT usDefaultChar;
    USHORT usBreakChar;
    USHORT usMaxContext;
} TT_OS2_V2;
#include "poppack.h"

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define GET_BE_DWORD(x) MAKELONG(GET_BE_WORD(HIWORD(x)), GET_BE_WORD(LOWORD(x)));
#endif

#define MS_MAKE_TAG(ch0, ch1, ch2, ch3) \
                    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
                    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#define MS_OS2_TAG MS_MAKE_TAG('O','S','/','2')
#define MS_CMAP_TAG MS_MAKE_TAG('c','m','a','p')

typedef struct
{
    USHORT version;
    USHORT num_tables;
} cmap_header;

typedef struct
{
    USHORT plat_id;
    USHORT enc_id;
    ULONG offset;
} cmap_encoding_record;

typedef struct
{
    USHORT format;
    USHORT length;
    USHORT language;

    BYTE glyph_ids[256];
} cmap_format_0;

typedef struct
{
    USHORT format;
    USHORT length;
    USHORT language;

    USHORT seg_countx2;
    USHORT search_range;
    USHORT entry_selector;
    USHORT range_shift;

    USHORT end_count[1]; /* this is a variable-sized array of length seg_countx2 / 2 */
/* Then follows:
    USHORT pad;
    USHORT start_count[seg_countx2 / 2];
    USHORT id_delta[seg_countx2 / 2];
    USHORT id_range_offset[seg_countx2 / 2];
    USHORT glyph_ids[];
*/
} cmap_format_4;

typedef struct
{
    USHORT end_count;
    USHORT start_count;
    USHORT id_delta;
    USHORT id_range_offset;
} cmap_format_4_seg;

static void expect_ff(const TEXTMETRICA *tmA, const TT_OS2_V2 *os2, WORD family, const char *name)
{
    ok((tmA->tmPitchAndFamily & 0xf0) == family, "%s: expected family %02x got %02x. panose %d-%d-%d-%d-...\n",
       name, family, tmA->tmPitchAndFamily, os2->panose.bFamilyType, os2->panose.bSerifStyle,
       os2->panose.bWeight, os2->panose.bProportion);
}

static BOOL get_first_last_from_cmap0(void *ptr, DWORD *first, DWORD *last)
{
    int i;
    cmap_format_0 *cmap = (cmap_format_0*)ptr;

    *first = 256;

    for(i = 0; i < 256; i++)
    {
        if(cmap->glyph_ids[i] == 0) continue;
        *last = i;
        if(*first == 256) *first = i;
    }
    if(*first == 256) return FALSE;
    return TRUE;
}

static void get_seg4(cmap_format_4 *cmap, USHORT seg_num, cmap_format_4_seg *seg)
{
    USHORT segs = GET_BE_WORD(cmap->seg_countx2) / 2;
    seg->end_count = GET_BE_WORD(cmap->end_count[seg_num]);
    seg->start_count = GET_BE_WORD(cmap->end_count[segs + 1 + seg_num]);
    seg->id_delta = GET_BE_WORD(cmap->end_count[2 * segs + 1 + seg_num]);
    seg->id_range_offset = GET_BE_WORD(cmap->end_count[3 * segs + 1 + seg_num]);
}

static BOOL get_first_last_from_cmap4(void *ptr, DWORD *first, DWORD *last)
{
    int i;
    cmap_format_4 *cmap = (cmap_format_4*)ptr;
    USHORT seg_count = GET_BE_WORD(cmap->seg_countx2) / 2;
    USHORT const *glyph_ids = cmap->end_count + 4 * seg_count + 1;

    *first = 0x10000;

    for(i = 0; i < seg_count; i++)
    {
        DWORD code, index;
        cmap_format_4_seg seg;

        get_seg4(cmap, i, &seg);
        for(code = seg.start_count; code <= seg.end_count; code++)
        {
            if(seg.id_range_offset == 0)
                index = (seg.id_delta + code) & 0xffff;
            else
            {
                index = seg.id_range_offset / 2
                    + code - seg.start_count
                    + i - seg_count;

                index = GET_BE_WORD(glyph_ids[index]);
                if(index) index += seg.id_delta;
            }
            if(*first == 0x10000)
                *last = *first = code;
            else if(index)
                *last = code;
        }
    }

    if(*first == 0x10000) return FALSE;
    return TRUE;
}

static void *get_cmap(cmap_header *header, USHORT plat_id, USHORT enc_id)
{
    USHORT i;
    cmap_encoding_record *record = (cmap_encoding_record *)(header + 1);

    for(i = 0; i < GET_BE_WORD(header->num_tables); i++)
    {
        if(GET_BE_WORD(record->plat_id) == plat_id && GET_BE_WORD(record->enc_id) == enc_id)
            return (BYTE *)header + GET_BE_DWORD(record->offset);
        record++;
    }
    return NULL;
}

typedef enum
{
    cmap_none,
    cmap_ms_unicode,
    cmap_ms_symbol
} cmap_type;

static BOOL get_first_last_from_cmap(HDC hdc, DWORD *first, DWORD *last, cmap_type *cmap_type)
{
    LONG size, ret;
    cmap_header *header;
    void *cmap;
    BOOL r = FALSE;
    WORD format;

    size = GetFontData(hdc, MS_CMAP_TAG, 0, NULL, 0);
    ok(size != GDI_ERROR, "no cmap table found\n");
    if(size == GDI_ERROR) return FALSE;

    header = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetFontData(hdc, MS_CMAP_TAG, 0, header, size);
    ok(ret == size, "GetFontData should return %u not %u\n", size, ret);
    ok(GET_BE_WORD(header->version) == 0, "got cmap version %d\n", GET_BE_WORD(header->version));

    cmap = get_cmap(header, 3, 1);
    if(cmap)
        *cmap_type = cmap_ms_unicode;
    else
    {
        cmap = get_cmap(header, 3, 0);
        if(cmap) *cmap_type = cmap_ms_symbol;
    }
    if(!cmap)
    {
        *cmap_type = cmap_none;
        goto end;
    }

    format = GET_BE_WORD(*(WORD *)cmap);
    switch(format)
    {
    case 0:
        r = get_first_last_from_cmap0(cmap, first, last);
        break;
    case 4:
        r = get_first_last_from_cmap4(cmap, first, last);
        break;
    default:
        trace("unhandled cmap format %d\n", format);
        break;
    }

end:
    HeapFree(GetProcessHeap(), 0, header);
    return r;
}

static void test_text_metrics(const LOGFONTA *lf)
{
    HDC hdc;
    HFONT hfont, hfont_old;
    TEXTMETRICA tmA;
    TT_OS2_V2 tt_os2;
    LONG size, ret;
    const char *font_name = lf->lfFaceName;
    DWORD cmap_first = 0, cmap_last = 0;
    cmap_type cmap_type;

    hdc = GetDC(0);

    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(lf);
    ok(hfont != 0, "CreateFontIndirect error %u\n", GetLastError());

    hfont_old = SelectObject(hdc, hfont);

    size = GetFontData(hdc, MS_OS2_TAG, 0, NULL, 0);
    if (size == GDI_ERROR)
    {
        trace("OS/2 chunk was not found\n");
        goto end_of_test;
    }
    if (size > sizeof(tt_os2))
    {
        trace("got too large OS/2 chunk of size %u\n", size);
        size = sizeof(tt_os2);
    }

    memset(&tt_os2, 0, sizeof(tt_os2));
    ret = GetFontData(hdc, MS_OS2_TAG, 0, &tt_os2, size);
    ok(ret == size, "GetFontData should return %u not %u\n", size, ret);

    SetLastError(0xdeadbeef);
    ret = GetTextMetricsA(hdc, &tmA);
    ok(ret, "GetTextMetricsA error %u\n", GetLastError());

    if(!get_first_last_from_cmap(hdc, &cmap_first, &cmap_last, &cmap_type))
    {
        skip("Unable to retrieve first and last glyphs from cmap\n");
    }
    else
    {
        USHORT expect_first_A, expect_last_A, expect_break_A, expect_default_A;
        USHORT expect_first_W, expect_last_W, expect_break_W, expect_default_W;
        UINT os2_first_char, os2_last_char, default_char, break_char;
        USHORT version;
        TEXTMETRICW tmW;

        version = GET_BE_WORD(tt_os2.version);

        os2_first_char = GET_BE_WORD(tt_os2.usFirstCharIndex);
        os2_last_char = GET_BE_WORD(tt_os2.usLastCharIndex);
        default_char = GET_BE_WORD(tt_os2.usDefaultChar);
        break_char = GET_BE_WORD(tt_os2.usBreakChar);

        trace("font %s charset %u: %x-%x (%x-%x) default %x break %x OS/2 version %u vendor %4.4s\n",
              font_name, lf->lfCharSet, os2_first_char, os2_last_char, cmap_first, cmap_last,
              default_char, break_char, version, (LPCSTR)&tt_os2.achVendID);

        if (cmap_type == cmap_ms_symbol || (cmap_first >= 0xf000 && cmap_first < 0xf100))
        {
            expect_first_W    = 0;
            switch(GetACP())
            {
            case 1257:  /* Baltic */
                expect_last_W = 0xf8fd;
                break;
            default:
                expect_last_W = 0xf0ff;
            }
            expect_break_W    = 0x20;
            expect_default_W  = expect_break_W - 1;
            expect_first_A    = 0x1e;
            expect_last_A     = min(os2_last_char - os2_first_char + 0x20, 0xff);
        }
        else
        {
            expect_first_W    = cmap_first;
            expect_last_W     = min(cmap_last, os2_last_char);
            if(os2_first_char <= 1)
                expect_break_W = os2_first_char + 2;
            else if(os2_first_char > 0xff)
                expect_break_W = 0x20;
            else
                expect_break_W = os2_first_char;
            expect_default_W  = expect_break_W - 1;
            expect_first_A    = expect_default_W - 1;
            expect_last_A     = min(expect_last_W, 0xff);
        }
        expect_break_A    = expect_break_W;
        expect_default_A  = expect_default_W;

        /* Wine currently uses SYMBOL_CHARSET to identify whether the ANSI metrics need special handling */
        if(cmap_type != cmap_ms_symbol && tmA.tmCharSet == SYMBOL_CHARSET && expect_first_A != 0x1e)
            todo_wine ok(tmA.tmFirstChar == expect_first_A ||
                         tmA.tmFirstChar == expect_first_A + 1 /* win9x */,
                         "A: tmFirstChar for %s got %02x expected %02x\n", font_name, tmA.tmFirstChar, expect_first_A);
        else
            ok(tmA.tmFirstChar == expect_first_A ||
               tmA.tmFirstChar == expect_first_A + 1 /* win9x */,
               "A: tmFirstChar for %s got %02x expected %02x\n", font_name, tmA.tmFirstChar, expect_first_A);
        ok(tmA.tmLastChar == expect_last_A ||
           tmA.tmLastChar == 0xff /* win9x */,
           "A: tmLastChar for %s got %02x expected %02x\n", font_name, tmA.tmLastChar, expect_last_A);
        ok(tmA.tmBreakChar == expect_break_A, "A: tmBreakChar for %s got %02x expected %02x\n",
           font_name, tmA.tmBreakChar, expect_break_A);
        ok(tmA.tmDefaultChar == expect_default_A, "A: tmDefaultChar for %s got %02x expected %02x\n",
           font_name, tmA.tmDefaultChar, expect_default_A);


        SetLastError(0xdeadbeef);
        ret = GetTextMetricsW(hdc, &tmW);
        ok(ret || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
           "GetTextMetricsW error %u\n", GetLastError());
        if (ret)
        {
            /* Wine uses the os2 first char */
            if(cmap_first != os2_first_char && cmap_type != cmap_ms_symbol)
                todo_wine ok(tmW.tmFirstChar == expect_first_W, "W: tmFirstChar for %s got %02x expected %02x\n",
                             font_name, tmW.tmFirstChar, expect_first_W);
            else
                ok(tmW.tmFirstChar == expect_first_W, "W: tmFirstChar for %s got %02x expected %02x\n",
                   font_name, tmW.tmFirstChar, expect_first_W);

            /* Wine uses the os2 last char */
            if(expect_last_W != os2_last_char && cmap_type != cmap_ms_symbol)
                todo_wine ok(tmW.tmLastChar == expect_last_W, "W: tmLastChar for %s got %02x expected %02x\n",
                             font_name, tmW.tmLastChar, expect_last_W);
            else
                ok(tmW.tmLastChar == expect_last_W, "W: tmLastChar for %s got %02x expected %02x\n",
                   font_name, tmW.tmLastChar, expect_last_W);
            ok(tmW.tmBreakChar == expect_break_W, "W: tmBreakChar for %s got %02x expected %02x\n",
               font_name, tmW.tmBreakChar, expect_break_W);
            ok(tmW.tmDefaultChar == expect_default_W, "W: tmDefaultChar for %s got %02x expected %02x\n",
               font_name, tmW.tmDefaultChar, expect_default_W);

            /* Test the aspect ratio while we have tmW */
            ret = GetDeviceCaps(hdc, LOGPIXELSX);
            ok(tmW.tmDigitizedAspectX == ret, "W: tmDigitizedAspectX %u != %u\n",
               tmW.tmDigitizedAspectX, ret);
            ret = GetDeviceCaps(hdc, LOGPIXELSY);
            ok(tmW.tmDigitizedAspectX == ret, "W: tmDigitizedAspectY %u != %u\n",
               tmW.tmDigitizedAspectX, ret);
        }
    }

    /* test FF_ values */
    switch(tt_os2.panose.bFamilyType)
    {
    case PAN_ANY:
    case PAN_NO_FIT:
    case PAN_FAMILY_TEXT_DISPLAY:
    case PAN_FAMILY_PICTORIAL:
    default:
        if((tmA.tmPitchAndFamily & 1) == 0 || /* fixed */
           tt_os2.panose.bProportion == PAN_PROP_MONOSPACED)
        {
            expect_ff(&tmA, &tt_os2, FF_MODERN, font_name);
            break;
        }
        switch(tt_os2.panose.bSerifStyle)
        {
        case PAN_ANY:
        case PAN_NO_FIT:
        default:
            expect_ff(&tmA, &tt_os2, FF_DONTCARE, font_name);
            break;

        case PAN_SERIF_COVE:
        case PAN_SERIF_OBTUSE_COVE:
        case PAN_SERIF_SQUARE_COVE:
        case PAN_SERIF_OBTUSE_SQUARE_COVE:
        case PAN_SERIF_SQUARE:
        case PAN_SERIF_THIN:
        case PAN_SERIF_BONE:
        case PAN_SERIF_EXAGGERATED:
        case PAN_SERIF_TRIANGLE:
            expect_ff(&tmA, &tt_os2, FF_ROMAN, font_name);
            break;

        case PAN_SERIF_NORMAL_SANS:
        case PAN_SERIF_OBTUSE_SANS:
        case PAN_SERIF_PERP_SANS:
        case PAN_SERIF_FLARED:
        case PAN_SERIF_ROUNDED:
            expect_ff(&tmA, &tt_os2, FF_SWISS, font_name);
            break;
        }
        break;

    case PAN_FAMILY_SCRIPT:
        expect_ff(&tmA, &tt_os2, FF_SCRIPT, font_name);
        break;

    case PAN_FAMILY_DECORATIVE:
        expect_ff(&tmA, &tt_os2, FF_DECORATIVE, font_name);
        break;
    }

    test_negative_width(hdc, lf);

end_of_test:
    SelectObject(hdc, hfont_old);
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
}

static INT CALLBACK enum_truetype_font_proc(const LOGFONT *lf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    INT *enumed = (INT *)lParam;

    if (type == TRUETYPE_FONTTYPE)
    {
        (*enumed)++;
        test_text_metrics(lf);
    }
    return 1;
}

static void test_GetTextMetrics(void)
{
    LOGFONTA lf;
    HDC hdc;
    INT enumed;

    /* Report only once */
    if(!pGetGlyphIndicesA)
        win_skip("GetGlyphIndicesA is unavailable, negative width will not be checked\n");

    hdc = GetDC(0);

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    enumed = 0;
    EnumFontFamiliesExA(hdc, &lf, enum_truetype_font_proc, (LPARAM)&enumed, 0);
    trace("Tested metrics of %d truetype fonts\n", enumed);

    ReleaseDC(0, hdc);
}

static void test_nonexistent_font(void)
{
    static const struct
    {
        const char *name;
        int charset;
    } font_subst[] =
    {
        { "Times New Roman Baltic", 186 },
        { "Times New Roman CE", 238 },
        { "Times New Roman CYR", 204 },
        { "Times New Roman Greek", 161 },
        { "Times New Roman TUR", 162 }
    };
    LOGFONTA lf;
    HDC hdc;
    HFONT hfont;
    CHARSETINFO csi;
    INT cs, expected_cs, i;
    char buf[LF_FACESIZE];

    if (!is_truetype_font_installed("Arial") ||
        !is_truetype_font_installed("Times New Roman"))
    {
        skip("Arial or Times New Roman not installed\n");
        return;
    }

    expected_cs = GetACP();
    if (!TranslateCharsetInfo(ULongToPtr(expected_cs), &csi, TCI_SRCCODEPAGE))
    {
        skip("TranslateCharsetInfo failed for code page %d\n", expected_cs);
        return;
    }
    expected_cs = csi.ciCharset;
    trace("ACP %d -> charset %d\n", GetACP(), expected_cs);

    hdc = GetDC(0);

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 100;
    lf.lfWeight = FW_REGULAR;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfPitchAndFamily = FF_SWISS;
    strcpy(lf.lfFaceName, "Nonexistent font");
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);
    GetTextFaceA(hdc, sizeof(buf), buf);
    ok(!lstrcmpiA(buf, "Arial"), "Got %s\n", buf);
    cs = GetTextCharset(hdc);
    ok(cs == ANSI_CHARSET, "expected ANSI_CHARSET, got %d\n", cs);
    DeleteObject(SelectObject(hdc, hfont));

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -13;
    lf.lfWeight = FW_DONTCARE;
    strcpy(lf.lfFaceName, "Nonexistent font");
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);
    GetTextFaceA(hdc, sizeof(buf), buf);
todo_wine /* Wine uses Arial for all substitutions */
    ok(!lstrcmpiA(buf, "Nonexistent font") /* XP, Vista */ ||
       !lstrcmpiA(buf, "MS Serif") || /* Win9x */
       !lstrcmpiA(buf, "MS Sans Serif"), /* win2k3 */
       "Got %s\n", buf);
    cs = GetTextCharset(hdc);
    ok(cs == expected_cs, "expected %d, got %d\n", expected_cs, cs);
    DeleteObject(SelectObject(hdc, hfont));

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -13;
    lf.lfWeight = FW_REGULAR;
    strcpy(lf.lfFaceName, "Nonexistent font");
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);
    GetTextFaceA(hdc, sizeof(buf), buf);
    ok(!lstrcmpiA(buf, "Arial") /* XP, Vista */ ||
       !lstrcmpiA(buf, "Times New Roman") /* Win9x */, "Got %s\n", buf);
    cs = GetTextCharset(hdc);
    ok(cs == ANSI_CHARSET, "expected ANSI_CHARSET, got %d\n", cs);
    DeleteObject(SelectObject(hdc, hfont));

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -13;
    lf.lfWeight = FW_DONTCARE;
    strcpy(lf.lfFaceName, "Times New Roman");
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);
    GetTextFaceA(hdc, sizeof(buf), buf);
    ok(!lstrcmpiA(buf, "Times New Roman"), "Got %s\n", buf);
    cs = GetTextCharset(hdc);
    ok(cs == ANSI_CHARSET, "expected ANSI_CHARSET, got %d\n", cs);
    DeleteObject(SelectObject(hdc, hfont));

    for (i = 0; i < sizeof(font_subst)/sizeof(font_subst[0]); i++)
    {
        memset(&lf, 0, sizeof(lf));
        lf.lfHeight = -13;
        lf.lfWeight = FW_REGULAR;
        strcpy(lf.lfFaceName, font_subst[i].name);
        hfont = CreateFontIndirectA(&lf);
        hfont = SelectObject(hdc, hfont);
        cs = GetTextCharset(hdc);
        if (font_subst[i].charset == expected_cs)
        {
            ok(cs == expected_cs, "expected %d, got %d for font %s\n", expected_cs, cs, font_subst[i].name);
            GetTextFaceA(hdc, sizeof(buf), buf);
            ok(!lstrcmpiA(buf, font_subst[i].name), "expected %s, got %s\n", font_subst[i].name, buf);
        }
        else
        {
            ok(cs == ANSI_CHARSET, "expected ANSI_CHARSET, got %d for font %s\n", cs, font_subst[i].name);
            GetTextFaceA(hdc, sizeof(buf), buf);
            ok(!lstrcmpiA(buf, "Arial") /* XP, Vista */ ||
               !lstrcmpiA(buf, "Times New Roman") /* Win9x */, "got %s for font %s\n", buf, font_subst[i].name);
        }
        DeleteObject(SelectObject(hdc, hfont));

        memset(&lf, 0, sizeof(lf));
        lf.lfHeight = -13;
        lf.lfWeight = FW_DONTCARE;
        strcpy(lf.lfFaceName, font_subst[i].name);
        hfont = CreateFontIndirectA(&lf);
        hfont = SelectObject(hdc, hfont);
        GetTextFaceA(hdc, sizeof(buf), buf);
        ok(!lstrcmpiA(buf, "Arial") /* Wine */ ||
           !lstrcmpiA(buf, font_subst[i].name) /* XP, Vista */ ||
           !lstrcmpiA(buf, "MS Serif") /* Win9x */ ||
           !lstrcmpiA(buf, "MS Sans Serif"), /* win2k3 */
           "got %s for font %s\n", buf, font_subst[i].name);
        cs = GetTextCharset(hdc);
        ok(cs == expected_cs, "expected %d, got %d for font %s\n", expected_cs, cs, font_subst[i].name);
        DeleteObject(SelectObject(hdc, hfont));
    }

    ReleaseDC(0, hdc);
}

static void test_GdiRealizationInfo(void)
{
    HDC hdc;
    DWORD info[4];
    BOOL r;
    HFONT hfont, hfont_old;
    LOGFONTA lf;

    if(!pGdiRealizationInfo)
    {
        win_skip("GdiRealizationInfo not available\n");
        return;
    }

    hdc = GetDC(0);

    memset(info, 0xcc, sizeof(info));
    r = pGdiRealizationInfo(hdc, info);
    ok(r != 0, "ret 0\n");
    ok((info[0] & 0xf) == 1, "info[0] = %x for the system font\n", info[0]);
    ok(info[3] == 0xcccccccc, "structure longer than 3 dwords\n");

    if (!is_truetype_font_installed("Arial"))
    {
        skip("skipping GdiRealizationInfo with truetype font\n");
        goto end;
    }

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 20;
    lf.lfWeight = FW_NORMAL;
    hfont = CreateFontIndirectA(&lf);
    hfont_old = SelectObject(hdc, hfont);

    memset(info, 0xcc, sizeof(info));
    r = pGdiRealizationInfo(hdc, info);
    ok(r != 0, "ret 0\n");
    ok((info[0] & 0xf) == 3, "info[0] = %x for arial\n", info[0]);
    ok(info[3] == 0xcccccccc, "structure longer than 3 dwords\n");

    DeleteObject(SelectObject(hdc, hfont_old));

 end:
    ReleaseDC(0, hdc);
}

/* Tests on XP SP2 show that the ANSI version of GetTextFace does NOT include
   the nul in the count of characters copied when the face name buffer is not
   NULL, whereas it does if the buffer is NULL.  Further, the Unicode version
   always includes it.  */
static void test_GetTextFace(void)
{
    static const char faceA[] = "Tahoma";
    static const WCHAR faceW[] = {'T','a','h','o','m','a', 0};
    LOGFONTA fA = {0};
    LOGFONTW fW = {0};
    char bufA[LF_FACESIZE];
    WCHAR bufW[LF_FACESIZE];
    HFONT f, g;
    HDC dc;
    int n;

    if(!is_font_installed("Tahoma"))
    {
        skip("Tahoma is not installed so skipping this test\n");
        return;
    }

    /* 'A' case.  */
    memcpy(fA.lfFaceName, faceA, sizeof faceA);
    f = CreateFontIndirectA(&fA);
    ok(f != NULL, "CreateFontIndirectA failed\n");

    dc = GetDC(NULL);
    g = SelectObject(dc, f);
    n = GetTextFaceA(dc, sizeof bufA, bufA);
    ok(n == sizeof faceA - 1, "GetTextFaceA returned %d\n", n);
    ok(lstrcmpA(faceA, bufA) == 0, "GetTextFaceA\n");

    /* Play with the count arg.  */
    bufA[0] = 'x';
    n = GetTextFaceA(dc, 0, bufA);
    ok(n == 0, "GetTextFaceA returned %d\n", n);
    ok(bufA[0] == 'x', "GetTextFaceA buf[0] == %d\n", bufA[0]);

    bufA[0] = 'x';
    n = GetTextFaceA(dc, 1, bufA);
    ok(n == 0, "GetTextFaceA returned %d\n", n);
    ok(bufA[0] == '\0', "GetTextFaceA buf[0] == %d\n", bufA[0]);

    bufA[0] = 'x'; bufA[1] = 'y';
    n = GetTextFaceA(dc, 2, bufA);
    ok(n == 1, "GetTextFaceA returned %d\n", n);
    ok(bufA[0] == faceA[0] && bufA[1] == '\0', "GetTextFaceA didn't copy\n");

    n = GetTextFaceA(dc, 0, NULL);
    ok(n == sizeof faceA ||
       broken(n == 0), /* win98, winMe */
       "GetTextFaceA returned %d\n", n);

    DeleteObject(SelectObject(dc, g));
    ReleaseDC(NULL, dc);

    /* 'W' case.  */
    memcpy(fW.lfFaceName, faceW, sizeof faceW);
    SetLastError(0xdeadbeef);
    f = CreateFontIndirectW(&fW);
    if (!f && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("CreateFontIndirectW is not implemented\n");
        return;
    }
    ok(f != NULL, "CreateFontIndirectW failed\n");

    dc = GetDC(NULL);
    g = SelectObject(dc, f);
    n = GetTextFaceW(dc, sizeof bufW / sizeof bufW[0], bufW);
    ok(n == sizeof faceW / sizeof faceW[0], "GetTextFaceW returned %d\n", n);
    ok(lstrcmpW(faceW, bufW) == 0, "GetTextFaceW\n");

    /* Play with the count arg.  */
    bufW[0] = 'x';
    n = GetTextFaceW(dc, 0, bufW);
    ok(n == 0, "GetTextFaceW returned %d\n", n);
    ok(bufW[0] == 'x', "GetTextFaceW buf[0] == %d\n", bufW[0]);

    bufW[0] = 'x';
    n = GetTextFaceW(dc, 1, bufW);
    ok(n == 1, "GetTextFaceW returned %d\n", n);
    ok(bufW[0] == '\0', "GetTextFaceW buf[0] == %d\n", bufW[0]);

    bufW[0] = 'x'; bufW[1] = 'y';
    n = GetTextFaceW(dc, 2, bufW);
    ok(n == 2, "GetTextFaceW returned %d\n", n);
    ok(bufW[0] == faceW[0] && bufW[1] == '\0', "GetTextFaceW didn't copy\n");

    n = GetTextFaceW(dc, 0, NULL);
    ok(n == sizeof faceW / sizeof faceW[0], "GetTextFaceW returned %d\n", n);

    DeleteObject(SelectObject(dc, g));
    ReleaseDC(NULL, dc);
}

static void test_orientation(void)
{
    static const char test_str[11] = "Test String";
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, old_hfont;
    SIZE size;

    if (!is_truetype_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    memset(&lf, 0, sizeof(lf));
    lstrcpyA(lf.lfFaceName, "Arial");
    lf.lfHeight = 72;
    lf.lfOrientation = lf.lfEscapement = 900;
    hfont = create_font("orientation", &lf);
    old_hfont = SelectObject(hdc, hfont);
    ok(GetTextExtentExPointA(hdc, test_str, sizeof(test_str), 32767, NULL, NULL, &size), "GetTextExtentExPointA failed\n");
    ok(near_match(311, size.cx), "cx should be about 311, got %d\n", size.cx);
    ok(near_match(75, size.cy), "cy should be about 75, got %d\n", size.cy);
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

static void test_GetGlyphOutline(void)
{
    MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
    HDC hdc;
    GLYPHMETRICS gm;
    LOGFONTA lf;
    HFONT hfont, old_hfont;
    INT ret;

    if (!is_truetype_font_installed("Tahoma"))
    {
        skip("Tahoma is not installed\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 72;
    lstrcpyA(lf.lfFaceName, "Tahoma");
    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());
    old_hfont = SelectObject(hdc, hfont);

    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineA error %u\n", GetLastError());

    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, NULL);
    ok(ret == GDI_ERROR, "GetGlyphOutlineA should fail\n");
    ok(GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* win98, winMe */
       "expected 0xdeadbeef, got %u\n", GetLastError());

    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineW(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        ok(ret != GDI_ERROR, "GetGlyphOutlineW error %u\n", GetLastError());

    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineW(hdc, 'A', GGO_METRICS, &gm, 0, NULL, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
       ok(ret == GDI_ERROR, "GetGlyphOutlineW should fail\n");
       ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %u\n", GetLastError());
    }

    /* test for needed buffer size request on space char */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineW(hdc, ' ', GGO_NATIVE, &gm, 0, NULL, &mat);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        ok(ret == 0, "GetGlyphOutlineW should return 0 buffer size for space char\n");

    /* requesting buffer size for space char + error */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineW(0, ' ', GGO_NATIVE, &gm, 0, NULL, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
       ok(ret == GDI_ERROR, "GetGlyphOutlineW should return GDI_ERROR\n");
       ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %u\n", GetLastError());
    }

    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

START_TEST(font)
{
    init();

    test_logfont();
    test_bitmap_font();
    test_outline_font();
    test_bitmap_font_metrics();
    test_GdiGetCharDimensions();
    test_GetCharABCWidths();
    test_text_extents();
    test_GetGlyphIndices();
    test_GetKerningPairs();
    test_GetOutlineTextMetrics();
    test_SetTextJustification();
    test_font_charset();
    test_GetFontUnicodeRanges();
    test_nonexistent_font();
    test_orientation();

    /* On Windows Arial has a lot of default charset aliases such as Arial Cyr,
     * I'd like to avoid them in this test.
     */
    test_EnumFontFamilies("Arial Black", ANSI_CHARSET);
    test_EnumFontFamilies("Symbol", SYMBOL_CHARSET);
    if (is_truetype_font_installed("Arial Black") &&
        (is_truetype_font_installed("Symbol") || is_truetype_font_installed("Wingdings")))
    {
        test_EnumFontFamilies("", ANSI_CHARSET);
        test_EnumFontFamilies("", SYMBOL_CHARSET);
        test_EnumFontFamilies("", DEFAULT_CHARSET);
    }
    else
        skip("Arial Black or Symbol/Wingdings is not installed\n");
    test_GetTextMetrics();
    test_GdiRealizationInfo();
    test_GetTextFace();
    test_GetGlyphOutline();
}
