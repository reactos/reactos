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

#include "wine/test.h"

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
    ok(!memcmp(&lf, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "%s: fonts don't match\n", test);
    ok(!lstrcmpA(lf->lfFaceName, getobj_lf.lfFaceName),
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

static void test_font_metrics(HDC hdc, HFONT hfont, const char *test_str,
			      INT test_str_len, const TEXTMETRICA *tm_orig,
			      const SIZE *size_orig, INT width_orig,
			      INT scale_x, INT scale_y)
{
    HFONT old_hfont;
    TEXTMETRICA tm;
    SIZE size;
    INT width;

    if (!hfont)
        return;

    old_hfont = SelectObject(hdc, hfont);

    GetTextMetricsA(hdc, &tm);

    ok(tm.tmHeight == tm_orig->tmHeight * scale_y, "%ld != %ld\n", tm.tmHeight, tm_orig->tmHeight * scale_y);
    ok(tm.tmAscent == tm_orig->tmAscent * scale_y, "%ld != %ld\n", tm.tmAscent, tm_orig->tmAscent * scale_y);
    ok(tm.tmDescent == tm_orig->tmDescent * scale_y, "%ld != %ld\n", tm.tmDescent, tm_orig->tmDescent * scale_y);
    ok(tm.tmAveCharWidth == tm_orig->tmAveCharWidth * scale_x, "%ld != %ld\n", tm.tmAveCharWidth, tm_orig->tmAveCharWidth * scale_x);

    GetTextExtentPoint32A(hdc, test_str, test_str_len, &size);

    ok(size.cx == size_orig->cx * scale_x, "%ld != %ld\n", size.cx, size_orig->cx * scale_x);
    ok(size.cy == size_orig->cy * scale_y, "%ld != %ld\n", size.cy, size_orig->cy * scale_y);

    GetCharWidthA(hdc, 'A', 'A', &width);

    ok(width == width_orig * scale_x, "%d != %d\n", width, width_orig * scale_x);

    SelectObject(hdc, old_hfont);
}

/* see whether GDI scales bitmap font metrics */
static void test_bitmap_font(void)
{
    static const char test_str[11] = "Test String";
    HDC hdc;
    LOGFONTA bitmap_lf;
    HFONT hfont, old_hfont;
    TEXTMETRICA tm_orig;
    SIZE size_orig;
    INT ret, i, width_orig, height_orig;

    hdc = GetDC(0);

    /* "System" has only 1 pixel size defined, otherwise the test breaks */
    ret = EnumFontFamiliesA(hdc, "System", font_enum_proc, (LPARAM)&bitmap_lf);
    if (ret)
    {
	ReleaseDC(0, hdc);
	trace("no bitmap fonts were found, skipping the test\n");
	return;
    }

    trace("found bitmap font %s, height %ld\n", bitmap_lf.lfFaceName, bitmap_lf.lfHeight);

    height_orig = bitmap_lf.lfHeight;
    hfont = create_font("bitmap", &bitmap_lf);

    old_hfont = SelectObject(hdc, hfont);
    ok(GetTextMetricsA(hdc, &tm_orig), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test fractional scaling */
    for (i = 1; i < height_orig; i++)
    {
	hfont = create_font("fractional", &bitmap_lf);
	test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 1, 1);
	DeleteObject(hfont);
    }

    /* test integer scaling 3x2 */
    bitmap_lf.lfHeight = height_orig * 2;
    bitmap_lf.lfWidth *= 3;
    hfont = create_font("3x2", &bitmap_lf);
todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 2);
}
    DeleteObject(hfont);

    /* test integer scaling 3x3 */
    bitmap_lf.lfHeight = height_orig * 3;
    bitmap_lf.lfWidth = 0;
    hfont = create_font("3x3", &bitmap_lf);

todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 3);
}
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
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
        int ave_char_width, max_char_width;
    } fd[] =
    {
        { "MS Sans Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11 },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 16 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 20 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 25 },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32 },
        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8 },
        { "MS Serif", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9 },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 12 },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 16 },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 19 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 23 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 27 },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 34 },
        { "Courier", FW_NORMAL, 13, 11, 2, 0, 0, 8, 8 },
        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9 },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12 },
        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 15 },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 2 },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 1, 0, 3, 4 },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 13 },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 7 },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 4, 8 },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9 }
        /* FIXME: add "Fixedsys", "Terminal" */
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
        memset(&lf, 0, sizeof(lf));

        lf.lfHeight = fd[i].height;
        strcpy(lf.lfFaceName, fd[i].face_name);
        ret = EnumFontFamilies(hdc, fd[i].face_name, find_font_proc, (LPARAM)&lf);
        if (ret)
        {
            trace("font %s height %d not found\n", fd[i].face_name, fd[i].height);
            continue;
        }

        trace("found font %s, height %ld\n", lf.lfFaceName, lf.lfHeight);

        hfont = create_font(lf.lfFaceName, &lf);
        old_hfont = SelectObject(hdc, hfont);
        ok(GetTextMetrics(hdc, &tm), "GetTextMetrics error %ld\n", GetLastError());

        ok(tm.tmWeight == fd[i].weight, "%s(%d): tm.tmWeight %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmWeight, fd[i].weight);
        ok(tm.tmHeight == fd[i].height, "%s(%d): tm.tmHeight %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmHeight, fd[i].height);
        ok(tm.tmAscent == fd[i].ascent, "%s(%d): tm.tmAscent %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmAscent, fd[i].ascent);
        ok(tm.tmDescent == fd[i].descent, "%s(%d): tm.tmDescent %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmDescent, fd[i].descent);
        ok(tm.tmInternalLeading == fd[i].int_leading, "%s(%d): tm.tmInternalLeading %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmInternalLeading, fd[i].int_leading);
        ok(tm.tmExternalLeading == fd[i].ext_leading, "%s(%d): tm.tmExternalLeading %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmExternalLeading, fd[i].ext_leading);
        ok(tm.tmAveCharWidth == fd[i].ave_char_width, "%s(%d): tm.tmAveCharWidth %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmAveCharWidth, fd[i].ave_char_width);
        ok(tm.tmMaxCharWidth == fd[i].max_char_width, "%s(%d): tm.tmMaxCharWidth %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmMaxCharWidth, fd[i].max_char_width);

        SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
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
    typedef LONG (WINAPI *fnGdiGetCharDimensions)(HDC hdc, LPTEXTMETRICW lptm, LONG *height);
    fnGdiGetCharDimensions GdiGetCharDimensions = (fnGdiGetCharDimensions)GetProcAddress(LoadLibrary("gdi32"), "GdiGetCharDimensions");
    if (!GdiGetCharDimensions) return;

    hdc = CreateCompatibleDC(NULL);

    GetTextExtentPoint(hdc, szAlphabet, strlen(szAlphabet), &size);
    avgwidth = ((size.cx / 26) + 1) / 2;

    ret = GdiGetCharDimensions(hdc, &tm, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);
    ok(height == tm.tmHeight, "GdiGetCharDimensions should have set height to %ld instead of %ld\n", tm.tmHeight, height);

    ret = GdiGetCharDimensions(hdc, &tm, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);

    ret = GdiGetCharDimensions(hdc, NULL, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);

    height = 0;
    ret = GdiGetCharDimensions(hdc, NULL, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);
    ok(height == size.cy, "GdiGetCharDimensions should have set height to %ld instead of %ld\n", size.cy, height);

    DeleteDC(hdc);
}

static void test_GetCharABCWidthsW(void)
{
    BOOL ret;
    ABC abc[1];
    typedef BOOL (WINAPI *fnGetCharABCWidthsW)(HDC hdc, UINT first, UINT last, LPABC abc);
    fnGetCharABCWidthsW GetCharABCWidthsW = (fnGetCharABCWidthsW)GetProcAddress(LoadLibrary("gdi32"), "GetCharABCWidthsW");
    if (!GetCharABCWidthsW) return;

    ret = GetCharABCWidthsW(NULL, 'a', 'a', abc);
    ok(!ret, "GetCharABCWidthsW should have returned FALSE\n");
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
    ok(sz.cy == tm.tmHeight, "cy %ld tmHeight %ld\n", sz.cy, tm.tmHeight);

    len = lstrlenW(wt);
    extents = HeapAlloc(GetProcessHeap(), 0, len * sizeof extents[0]);
    memset(extents, 0, len * sizeof extents[0]);
    extents[0] = 1;         /* So that the increasing sequence test will fail
                               if the extents array is untouched.  */
    GetTextExtentExPointW(hdc, wt, len, 32767, &fit1, extents, &sz1);
    GetTextExtentPointW(hdc, wt, len, &sz2);
    ok(sz1.cx == sz2.cx && sz1.cy == sz2.cy,
       "results from GetTextExtentExPointW and GetTextExtentPointW differ\n");
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

    SelectObject(hdc, hfont);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void test_GetGlyphIndices()
{
    HDC      hdc;
    HFONT    hfont;
    DWORD    charcount;
    LOGFONTA lf;
    DWORD    flags = 0;
    WCHAR    testtext[] = {'T','e','s','t',0xffff,0};
    WORD     glyphs[(sizeof(testtext)/2)-1];
    TEXTMETRIC textm;

    typedef BOOL (WINAPI *fnGetGlyphIndicesW)(HDC hdc, LPCWSTR lpstr, INT count, LPWORD pgi, DWORD flags);
    fnGetGlyphIndicesW GetGlyphIndicesW = (fnGetGlyphIndicesW)GetProcAddress(LoadLibrary("gdi32"),
                                           "GetGlyphIndicesW");
    if (!GetGlyphIndicesW) {
        trace("GetGlyphIndices not available on platform\n");
        return;
    }

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Symbol");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hdc = GetDC(0);

    ok(GetTextMetrics(hdc, &textm), "GetTextMetric failed\n");
    flags |= GGI_MARK_NONEXISTING_GLYPHS;
    charcount = GetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndices count of glyphs should = 5 not %ld\n", charcount);
    ok(glyphs[4] == 0x001f, "GetGlyphIndices should have returned a nonexistent char not %04x\n", glyphs[4]);
    flags = 0;
    charcount = GetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
    ok(charcount == 5, "GetGlyphIndices count of glyphs should = 5 not %ld\n", charcount);
    ok(glyphs[4] == textm.tmDefaultChar, "GetGlyphIndices should have returned a %04x not %04x\n",
                    textm.tmDefaultChar, glyphs[4]);
}

START_TEST(font)
{
    test_logfont();
    test_bitmap_font();
    test_bitmap_font_metrics();
    test_GdiGetCharDimensions();
    test_GetCharABCWidthsW();
    test_text_extents();
    test_GetGlyphIndices();
}
