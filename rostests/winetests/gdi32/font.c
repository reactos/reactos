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

/* Do not allow more than 1 deviation here */
#define match_off_by_1(a, b, exact) (abs((a) - (b)) <= ((exact) ? 0 : 1))

#define near_match(a, b) (abs((a) - (b)) <= 6)
#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static LONG  (WINAPI *pGdiGetCharDimensions)(HDC hdc, LPTEXTMETRICW lptm, LONG *height);
static DWORD (WINAPI *pGdiGetCodePage)(HDC hdc);
static BOOL  (WINAPI *pGetCharABCWidthsI)(HDC hdc, UINT first, UINT count, LPWORD glyphs, LPABC abc);
static BOOL  (WINAPI *pGetCharABCWidthsA)(HDC hdc, UINT first, UINT last, LPABC abc);
static BOOL  (WINAPI *pGetCharABCWidthsW)(HDC hdc, UINT first, UINT last, LPABC abc);
static BOOL  (WINAPI *pGetCharABCWidthsFloatW)(HDC hdc, UINT first, UINT last, LPABCFLOAT abc);
static BOOL  (WINAPI *pGetCharWidth32A)(HDC hdc, UINT first, UINT last, LPINT buffer);
static BOOL  (WINAPI *pGetCharWidth32W)(HDC hdc, UINT first, UINT last, LPINT buffer);
static DWORD (WINAPI *pGetFontUnicodeRanges)(HDC hdc, LPGLYPHSET lpgs);
static DWORD (WINAPI *pGetGlyphIndicesA)(HDC hdc, LPCSTR lpstr, INT count, LPWORD pgi, DWORD flags);
static DWORD (WINAPI *pGetGlyphIndicesW)(HDC hdc, LPCWSTR lpstr, INT count, LPWORD pgi, DWORD flags);
static BOOL  (WINAPI *pGetTextExtentExPointI)(HDC hdc, const WORD *indices, INT count, INT max_ext,
                                              LPINT nfit, LPINT dxs, LPSIZE size );
static BOOL  (WINAPI *pGdiRealizationInfo)(HDC hdc, DWORD *);
static HFONT (WINAPI *pCreateFontIndirectExA)(const ENUMLOGFONTEXDVA *);
static HANDLE (WINAPI *pAddFontMemResourceEx)(PVOID, DWORD, PVOID, DWORD *);
static BOOL  (WINAPI *pRemoveFontMemResourceEx)(HANDLE);
static INT   (WINAPI *pAddFontResourceExA)(LPCSTR, DWORD, PVOID);
static BOOL  (WINAPI *pRemoveFontResourceExA)(LPCSTR, DWORD, PVOID);

static HMODULE hgdi32 = 0;
static const MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
static WORD system_lang_id;

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
#define MS_NAME_TAG MS_MAKE_TAG('n','a','m','e')

static void init(void)
{
    hgdi32 = GetModuleHandleA("gdi32.dll");

    pGdiGetCharDimensions = (void *)GetProcAddress(hgdi32, "GdiGetCharDimensions");
    pGdiGetCodePage = (void *) GetProcAddress(hgdi32,"GdiGetCodePage");
    pGetCharABCWidthsI = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsI");
    pGetCharABCWidthsA = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsA");
    pGetCharABCWidthsW = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsW");
    pGetCharABCWidthsFloatW = (void *)GetProcAddress(hgdi32, "GetCharABCWidthsFloatW");
    pGetCharWidth32A = (void *)GetProcAddress(hgdi32, "GetCharWidth32A");
    pGetCharWidth32W = (void *)GetProcAddress(hgdi32, "GetCharWidth32W");
    pGetFontUnicodeRanges = (void *)GetProcAddress(hgdi32, "GetFontUnicodeRanges");
    pGetGlyphIndicesA = (void *)GetProcAddress(hgdi32, "GetGlyphIndicesA");
    pGetGlyphIndicesW = (void *)GetProcAddress(hgdi32, "GetGlyphIndicesW");
    pGetTextExtentExPointI = (void *)GetProcAddress(hgdi32, "GetTextExtentExPointI");
    pGdiRealizationInfo = (void *)GetProcAddress(hgdi32, "GdiRealizationInfo");
    pCreateFontIndirectExA = (void *)GetProcAddress(hgdi32, "CreateFontIndirectExA");
    pAddFontMemResourceEx = (void *)GetProcAddress(hgdi32, "AddFontMemResourceEx");
    pRemoveFontMemResourceEx = (void *)GetProcAddress(hgdi32, "RemoveFontMemResourceEx");
    pAddFontResourceExA = (void *)GetProcAddress(hgdi32, "AddFontResourceExA");
    pRemoveFontResourceExA = (void *)GetProcAddress(hgdi32, "RemoveFontResourceExA");

    system_lang_id = PRIMARYLANGID(GetSystemDefaultLangID());
}

static INT CALLBACK is_truetype_font_installed_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
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

static INT CALLBACK is_font_installed_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
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

static void *get_res_data(const char *fontname, DWORD *rsrc_size)
{
    HRSRC rsrc;
    void *rsrc_data;

    rsrc = FindResourceA(GetModuleHandleA(NULL), fontname, (LPCSTR)RT_RCDATA);
    if (!rsrc) return NULL;

    rsrc_data = LockResource(LoadResource(GetModuleHandleA(NULL), rsrc));
    if (!rsrc_data) return NULL;

    *rsrc_size = SizeofResource(GetModuleHandleA(NULL), rsrc);
    if (!*rsrc_size) return NULL;

    return rsrc_data;
}

static BOOL write_tmp_file( const void *data, DWORD *size, char *tmp_name )
{
    char tmp_path[MAX_PATH];
    HANDLE hfile;
    BOOL ret;

    GetTempPathA(MAX_PATH, tmp_path);
    GetTempFileNameA(tmp_path, "ttf", 0, tmp_name);

    hfile = CreateFileA(tmp_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    ret = WriteFile(hfile, data, *size, size, NULL);

    CloseHandle(hfile);
    return ret;
}

static BOOL write_ttf_file(const char *fontname, char *tmp_name)
{
    void *rsrc_data;
    DWORD rsrc_size;

    rsrc_data = get_res_data( fontname, &rsrc_size );
    if (!rsrc_data) return FALSE;

    return write_tmp_file( rsrc_data, &rsrc_size, tmp_name );
}

static void check_font(const char* test, const LOGFONTA* lf, HFONT hfont)
{
    LOGFONTA getobj_lf;
    int ret, minlen = 0;

    if (!hfont)
        return;

    ret = GetObjectA(hfont, sizeof(getobj_lf), &getobj_lf);
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

static INT CALLBACK font_enum_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    if (type & RASTER_FONTTYPE)
    {
	LOGFONTA *lf = (LOGFONTA *)lParam;
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
    OUTLINETEXTMETRICA otm;
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

    hdc = CreateCompatibleDC(0);

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

    DeleteDC(hdc);
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

static INT CALLBACK find_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    LOGFONTA *lf = (LOGFONTA *)lParam;

    if (elf->lfHeight == lf->lfHeight && !strcmp(elf->lfFaceName, lf->lfFaceName))
    {
        *lf = *elf;
        return 0; /* stop enumeration */
    }
    return 1; /* continue enumeration */
}

static BOOL is_CJK(void)
{
    return (system_lang_id == LANG_CHINESE || system_lang_id == LANG_JAPANESE || system_lang_id == LANG_KOREAN);
}

#define FH_SCALE 0x80000000
static void test_bitmap_font_metrics(void)
{
    static const struct font_data
    {
        const char face_name[LF_FACESIZE];
        int weight, height, ascent, descent, int_leading, ext_leading;
        int ave_char_width, max_char_width, dpi;
        BYTE first_char, last_char, def_char, break_char;
        DWORD ansi_bitfield;
        WORD skip_lang_id;
        int scaled_height;
    } fd[] =
    {
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 6, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 6, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 8, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 8, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 10, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 10, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 14, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 14, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 13 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 18, 13, 3, 3, 0, 7, 14, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 18, 13, 3, 3, 0, 7, 14, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },

        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 6, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 6, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 8, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 8, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 10, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 10, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 14, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 14, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 18, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2, 0, 16 },
        { "MS Sans Serif", FW_NORMAL, FH_SCALE | 18, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC, 0, 16 },

        { "MS Sans Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 16, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 16, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 19, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 24, 96, 0x20, 0xff, 0x81, 0x40, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 20, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 24, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 6, 0, 12, 24, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 25, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32, 96, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32, 96, 0x20, 0xff, 0x81, 0x40, FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32, 96, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },

        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 17, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 25, 20, 5, 5, 0, 10, 21, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 25, 20, 5, 5, 0, 10, 21, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 6, 0, 12, 24, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 24, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 36, 29, 7, 6, 0, 15, 30, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 36, 29, 7, 6, 0, 15, 30, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },
        { "MS Sans Serif", FW_NORMAL, 46, 37, 9, 6, 0, 20, 40, 120, 0x20, 0xff, 0x81, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Sans Serif", FW_NORMAL, 46, 37, 9, 6, 0, 20, 40, 120, 0x20, 0xff, 0x7f, 0x20, FS_CYRILLIC },

        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 4, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 12, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 14, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 16, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 18, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 19, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 17, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 22, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 23, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 23, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 26, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 27, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 33, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 34, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },

        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 14, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 13, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 18, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 15, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 23, 18, 5, 3, 0, 10, 21, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 23, 18, 5, 3, 0, 10, 19, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 4, 0, 12, 23, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_MEDIUM, 27, 22, 5, 2, 0, 12, 30, 120, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 33, 26, 7, 3, 0, 14, 30, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "MS Serif", FW_MEDIUM, 32, 25, 7, 2, 0, 14, 32, 120, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "MS Serif", FW_NORMAL, 43, 34, 9, 3, 0, 19, 39, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "Courier", FW_NORMAL, 13, 11, 2, 0, 0, 8, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },
        { "Courier", FW_NORMAL, 25, 20, 5, 0, 0, 15, 15, 120, 0x20, 0xff, 0x40, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC },

        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 14, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 15, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "System", FW_NORMAL, 18, 16, 2, 0, 2, 8, 16, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },

        { "System", FW_BOLD, 20, 16, 4, 4, 0, 9, 14, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "System", FW_BOLD, 20, 16, 4, 4, 0, 9, 17, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },

        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 2, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 2, 4, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 1, 0, 3, 4, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1, LANG_ARABIC },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 1, 0, 2, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 5, 4, 1, 0, 0, 3, 6, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 13, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1, LANG_ARABIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 8, 96, 0x00, 0xff, 0x60, 0x00, FS_ARABIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 0, 0, 4, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 7, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1, LANG_ARABIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 8, 96, 0x00, 0xff, 0x60, 0x00, FS_ARABIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 0, 0, 5, 10, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 4, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2, LANG_ARABIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 4, 9, 96, 0x00, 0xff, 0x60, 0x00, FS_ARABIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 0, 0, 6, 12, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC, LANG_ARABIC },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 2, 0, 4, 10, 96, 0x00, 0xff, 0x60, 0x00, FS_ARABIC },
        { "Small Fonts", FW_NORMAL, 11, 9, 2, 0, 0, 7, 14, 96, 0x20, 0xff, 0x80, 0x20, FS_JISJAPAN },

        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 2, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 3, 2, 1, 0, 0, 1, 8, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 5, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 6, 5, 1, 1, 0, 3, 8, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 7, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 8, 7, 1, 1, 0, 4, 8, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN2 | FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 9, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8, 120, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 12, 10, 2, 2, 0, 5, 10, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 12, 10, 2, 2, 0, 6, 10, 120, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "Small Fonts", FW_NORMAL, 13, 11, 2, 2, 0, 6, 12, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_JISJAPAN },
        { "Small Fonts", FW_NORMAL, 13, 11, 2, 2, 0, 6, 11, 120, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },

        { "Fixedsys", FW_NORMAL, 15, 12, 3, 3, 0, 8, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 },
        { "Fixedsys", FW_NORMAL, 16, 12, 4, 3, 0, 8, 8, 96, 0x20, 0xff, 0x80, 0x20, FS_CYRILLIC },
        { "FixedSys", FW_NORMAL, 18, 16, 2, 0, 0, 8, 16, 96, 0x20, 0xff, 0xa0, 0x20, FS_JISJAPAN },

        { "Fixedsys", FW_NORMAL, 20, 16, 4, 2, 0, 10, 10, 120, 0x20, 0xff, 0x80, 0x20, FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC }

        /* FIXME: add "Terminal" */
    };
    static const int font_log_pixels[] = { 96, 120 };
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, old_hfont;
    TEXTMETRICA tm;
    INT ret, i, expected_cs, screen_log_pixels, diff, font_res;
    char face_name[LF_FACESIZE];
    CHARSETINFO csi;

    trace("system language id %04x\n", system_lang_id);

    expected_cs = GetACP();
    if (!TranslateCharsetInfo(ULongToPtr(expected_cs), &csi, TCI_SRCCODEPAGE))
    {
        skip("TranslateCharsetInfo failed for code page %d\n", expected_cs);
        return;
    }
    expected_cs = csi.ciCharset;
    trace("ACP %d -> charset %d\n", GetACP(), expected_cs);

    hdc = CreateCompatibleDC(0);
    assert(hdc);

    trace("logpixelsX %d, logpixelsY %d\n", GetDeviceCaps(hdc, LOGPIXELSX),
          GetDeviceCaps(hdc, LOGPIXELSY));

    screen_log_pixels = GetDeviceCaps(hdc, LOGPIXELSY);
    diff = 32768;
    font_res = 0;
    for (i = 0; i < sizeof(font_log_pixels)/sizeof(font_log_pixels[0]); i++)
    {
        int new_diff = abs(font_log_pixels[i] - screen_log_pixels);
        if (new_diff < diff)
        {
            diff = new_diff;
            font_res = font_log_pixels[i];
        }
    }
    trace("best font resolution is %d\n", font_res);

    for (i = 0; i < sizeof(fd)/sizeof(fd[0]); i++)
    {
        int bit, height;

        memset(&lf, 0, sizeof(lf));

        height = fd[i].height & ~FH_SCALE;
        lf.lfHeight = height;
        strcpy(lf.lfFaceName, fd[i].face_name);

        for(bit = 0; bit < 32; bit++)
        {
            GLYPHMETRICS gm;
            DWORD fs[2];
            BOOL bRet;

            fs[0] = 1L << bit;
            fs[1] = 0;
            if((fd[i].ansi_bitfield & fs[0]) == 0) continue;
            if(!TranslateCharsetInfo( fs, &csi, TCI_SRCFONTSIG )) continue;

            lf.lfCharSet = csi.ciCharset;
            ret = EnumFontFamiliesExA(hdc, &lf, find_font_proc, (LPARAM)&lf, 0);
            if (fd[i].height & FH_SCALE)
                ok(ret, "scaled font height %d should not be enumerated\n", height);
            else
            {
                if (font_res == fd[i].dpi && lf.lfCharSet == expected_cs)
                {
                    if (ret) /* FIXME: Remove once Wine is fixed */
                        todo_wine ok(!ret, "%s height %d charset %d dpi %d should be enumerated\n", lf.lfFaceName, lf.lfHeight, lf.lfCharSet, fd[i].dpi);
                    else
                        ok(!ret, "%s height %d charset %d dpi %d should be enumerated\n", lf.lfFaceName, lf.lfHeight, lf.lfCharSet, fd[i].dpi);
                }
            }
            if (ret && !(fd[i].height & FH_SCALE))
                continue;

            hfont = create_font(lf.lfFaceName, &lf);
            old_hfont = SelectObject(hdc, hfont);

            SetLastError(0xdeadbeef);
            ret = GetTextFaceA(hdc, sizeof(face_name), face_name);
            ok(ret, "GetTextFace error %u\n", GetLastError());

            if (strcmp(face_name, fd[i].face_name) != 0)
            {
                ok(ret != ANSI_CHARSET, "font charset should not be ANSI_CHARSET\n");
                ok(ret != expected_cs, "font charset %d should not be %d\n", ret, expected_cs);
                SelectObject(hdc, old_hfont);
                DeleteObject(hfont);
                continue;
            }

            memset(&gm, 0, sizeof(gm));
            SetLastError(0xdeadbeef);
            ret = GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
            todo_wine {
            ok(ret == GDI_ERROR, "GetGlyphOutline should fail for a bitmap font\n");
            ok(GetLastError() == ERROR_CAN_NOT_COMPLETE, "expected ERROR_CAN_NOT_COMPLETE, got %u\n", GetLastError());
            }

            bRet = GetTextMetricsA(hdc, &tm);
            ok(bRet, "GetTextMetrics error %d\n", GetLastError());

            SetLastError(0xdeadbeef);
            ret = GetTextCharset(hdc);
            if (is_CJK() && lf.lfCharSet == ANSI_CHARSET)
                ok(ret == ANSI_CHARSET, "got charset %d, expected ANSI_CHARSETd\n", ret);
            else
                ok(ret == expected_cs, "got charset %d, expected %d\n", ret, expected_cs);

            trace("created %s, height %d charset %x dpi %d\n", face_name, tm.tmHeight, tm.tmCharSet, tm.tmDigitizedAspectX);
            trace("expected %s, height %d scaled_hight %d, dpi %d\n", fd[i].face_name, height, fd[i].scaled_height, fd[i].dpi);

            if(fd[i].dpi == tm.tmDigitizedAspectX)
            {
                trace("matched %s, height %d charset %x dpi %d\n", lf.lfFaceName, lf.lfHeight, lf.lfCharSet, fd[i].dpi);
                if (fd[i].skip_lang_id == 0 || system_lang_id != fd[i].skip_lang_id)
                {
                    ok(tm.tmWeight == fd[i].weight, "%s(%d): tm.tmWeight %d != %d\n", fd[i].face_name, height, tm.tmWeight, fd[i].weight);
                    if (fd[i].height & FH_SCALE)
                        ok(tm.tmHeight == fd[i].scaled_height, "%s(%d): tm.tmHeight %d != %d\n", fd[i].face_name, height, tm.tmHeight, fd[i].scaled_height);
                    else
                        ok(tm.tmHeight == fd[i].height, "%s(%d): tm.tmHeight %d != %d\n", fd[i].face_name, fd[i].height, tm.tmHeight, fd[i].height);
                    ok(tm.tmAscent == fd[i].ascent, "%s(%d): tm.tmAscent %d != %d\n", fd[i].face_name, height, tm.tmAscent, fd[i].ascent);
                    ok(tm.tmDescent == fd[i].descent, "%s(%d): tm.tmDescent %d != %d\n", fd[i].face_name, height, tm.tmDescent, fd[i].descent);
                    ok(tm.tmInternalLeading == fd[i].int_leading, "%s(%d): tm.tmInternalLeading %d != %d\n", fd[i].face_name, height, tm.tmInternalLeading, fd[i].int_leading);
                    ok(tm.tmExternalLeading == fd[i].ext_leading, "%s(%d): tm.tmExternalLeading %d != %d\n", fd[i].face_name, height, tm.tmExternalLeading, fd[i].ext_leading);
                    ok(tm.tmAveCharWidth == fd[i].ave_char_width, "%s(%d): tm.tmAveCharWidth %d != %d\n", fd[i].face_name, height, tm.tmAveCharWidth, fd[i].ave_char_width);
                    ok(tm.tmFirstChar == fd[i].first_char, "%s(%d): tm.tmFirstChar = %02x\n", fd[i].face_name, height, tm.tmFirstChar);
                    ok(tm.tmLastChar == fd[i].last_char, "%s(%d): tm.tmLastChar = %02x\n", fd[i].face_name, height, tm.tmLastChar);
                    /* Substitutions like MS Sans Serif,0=MS Sans Serif,204
                       make default char test fail */
                    if (tm.tmCharSet == lf.lfCharSet)
                        ok(tm.tmDefaultChar == fd[i].def_char, "%s(%d): tm.tmDefaultChar = %02x\n", fd[i].face_name, height, tm.tmDefaultChar);
                    ok(tm.tmBreakChar == fd[i].break_char, "%s(%d): tm.tmBreakChar = %02x\n", fd[i].face_name, height, tm.tmBreakChar);
                    ok(tm.tmCharSet == expected_cs || tm.tmCharSet == ANSI_CHARSET, "%s(%d): tm.tmCharSet %d != %d\n", fd[i].face_name, height, tm.tmCharSet, expected_cs);

                    /* Don't run the max char width test on System/ANSI_CHARSET.  We have extra characters in our font
                       that make the max width bigger */
                    if ((strcmp(lf.lfFaceName, "System") || lf.lfCharSet != ANSI_CHARSET) && tm.tmDigitizedAspectX == 96)
                        ok(tm.tmMaxCharWidth == fd[i].max_char_width, "%s(%d): tm.tmMaxCharWidth %d != %d\n", fd[i].face_name, height, tm.tmMaxCharWidth, fd[i].max_char_width);
                }
                else
                    skip("Skipping font metrics test for system langid 0x%x\n",
                         system_lang_id);
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

    GetTextExtentPointA(hdc, szAlphabet, strlen(szAlphabet), &size);
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

static int CALLBACK create_font_proc(const LOGFONTA *lpelfe,
                                     const TEXTMETRICA *lpntme,
                                     DWORD FontType, LPARAM lParam)
{
    if (FontType & TRUETYPE_FONTTYPE)
    {
        HFONT hfont;

        hfont = CreateFontIndirectA(lpelfe);
        if (hfont)
        {
            *(HFONT *)lParam = hfont;
            return 0;
        }
    }

    return 1;
}

static void ABCWidths_helper(const char* description, HDC hdc, WORD *glyphs, ABC *base_abci, ABC *base_abcw, ABCFLOAT *base_abcf, INT todo)
{
    ABC abc[1];
    ABCFLOAT abcf[1];
    BOOL ret = FALSE;

    ret = pGetCharABCWidthsI(hdc, 0, 1, glyphs, abc);
    ok(ret, "%s: GetCharABCWidthsI should have succeeded\n", description);
    ok ((INT)abc->abcB > 0, "%s: abcB should be positive\n", description);
    if (todo) todo_wine ok(abc->abcA * base_abci->abcA >= 0, "%s: abcA's sign should be unchanged\n", description);
    else ok(abc->abcA * base_abci->abcA >= 0, "%s: abcA's sign should be unchanged\n", description);
    if (todo) todo_wine ok(abc->abcC * base_abci->abcC >= 0, "%s: abcC's sign should be unchanged\n", description);
    else ok(abc->abcC * base_abci->abcC >= 0, "%s: abcC's sign should be unchanged\n", description);

    ret = pGetCharABCWidthsW(hdc, 'i', 'i', abc);
    ok(ret, "%s: GetCharABCWidthsW should have succeeded\n", description);
    ok ((INT)abc->abcB > 0, "%s: abcB should be positive\n", description);
    if (todo) todo_wine ok(abc->abcA * base_abcw->abcA >= 0, "%s: abcA's sign should be unchanged\n", description);
    else ok(abc->abcA * base_abcw->abcA >= 0, "%s: abcA's sign should be unchanged\n", description);
    if (todo) todo_wine ok(abc->abcC * base_abcw->abcC >= 0, "%s: abcC's sign should be unchanged\n", description);
    else ok(abc->abcC * base_abcw->abcC >= 0, "%s: abcC's should be unchanged\n", description);

    ret = pGetCharABCWidthsFloatW(hdc, 'i', 'i', abcf);
    ok(ret, "%s: GetCharABCWidthsFloatW should have succeeded\n", description);
    ok (abcf->abcfB > 0.0, "%s: abcfB should be positive\n", description);
    if (todo) todo_wine ok(abcf->abcfA * base_abcf->abcfA >= 0.0, "%s: abcfA's sign should be unchanged\n", description);
    else ok(abcf->abcfA * base_abcf->abcfA >= 0.0, "%s: abcfA's should be unchanged\n", description);
    if (todo) todo_wine ok(abcf->abcfC * base_abcf->abcfC >= 0.0, "%s: abcfC's sign should be unchanged\n", description);
    else ok(abcf->abcfC * base_abcf->abcfC >= 0.0, "%s: abcfC's sign should be unchanged\n", description);
}

static void test_GetCharABCWidths(void)
{
    static const WCHAR str[] = {'i',0};
    BOOL ret;
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont;
    ABC abc[1];
    ABC abcw[1];
    ABCFLOAT abcf[1];
    WORD glyphs[1];
    DWORD nb;
    HWND hwnd;
    static const struct
    {
        UINT first;
        UINT last;
    } range[] =
    {
        {0xff, 0xff},
        {0x100, 0x100},
        {0xff, 0x100},
        {0x1ff, 0xff00},
        {0xffff, 0xffff},
        {0x10000, 0x10000},
        {0xffff, 0x10000},
        {0xffffff, 0xffffff},
        {0x1000000, 0x1000000},
        {0xffffff, 0x1000000},
        {0xffffffff, 0xffffffff},
        {0x00, 0xff}
    };
    static const struct
    {
        UINT cs;
        UINT a;
        UINT w;
        BOOL r[sizeof range / sizeof range[0]];
    } c[] =
    {
        {ANSI_CHARSET, 0x30, 0x30,
         {TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}},
        {SHIFTJIS_CHARSET, 0x82a0, 0x3042,
         {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}},
        {HANGEUL_CHARSET, 0x8141, 0xac02,
         {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}},
        {GB2312_CHARSET, 0x8141, 0x4e04,
         {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}},
        {CHINESEBIG5_CHARSET, 0xa142, 0x3001,
         {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE}}
    };
    UINT i;

    if (!pGetCharABCWidthsA || !pGetCharABCWidthsW || !pGetCharABCWidthsFloatW || !pGetCharABCWidthsI)
    {
        win_skip("GetCharABCWidthsA/W/I not available on this platform\n");
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

    ret = pGetCharABCWidthsFloatW(NULL, 'a', 'a', abcf);
    ok(!ret, "GetCharABCWidthsFloatW should have failed\n");

    ret = pGetCharABCWidthsFloatW(hdc, 'a', 'a', NULL);
    ok(!ret, "GetCharABCWidthsFloatW should have failed\n");

    ret = pGetCharABCWidthsFloatW(hdc, 'a', 'a', abcf);
    ok(ret, "GetCharABCWidthsFloatW should have succeeded\n");

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);

    for (i = 0; i < sizeof c / sizeof c[0]; ++i)
    {
        ABC a[2], w[2];
        ABC full[256];
        UINT code = 0x41, j;

        lf.lfFaceName[0] = '\0';
        lf.lfCharSet = c[i].cs;
        lf.lfPitchAndFamily = 0;
        if (EnumFontFamiliesExA(hdc, &lf, create_font_proc, (LPARAM)&hfont, 0))
        {
            skip("TrueType font for charset %u is not installed\n", c[i].cs);
            continue;
        }

        memset(a, 0, sizeof a);
        memset(w, 0, sizeof w);
        hfont = SelectObject(hdc, hfont);
        ok(pGetCharABCWidthsA(hdc, c[i].a, c[i].a + 1, a) &&
           pGetCharABCWidthsW(hdc, c[i].w, c[i].w + 1, w) &&
           memcmp(a, w, sizeof a) == 0,
           "GetCharABCWidthsA and GetCharABCWidthsW should return same widths. charset = %u\n", c[i].cs);

        memset(a, 0xbb, sizeof a);
        ret = pGetCharABCWidthsA(hdc, code, code, a);
        ok(ret, "GetCharABCWidthsA should have succeeded\n");
        memset(full, 0xcc, sizeof full);
        ret = pGetCharABCWidthsA(hdc, 0x00, code, full);
        ok(ret, "GetCharABCWidthsA should have succeeded\n");
        ok(memcmp(&a[0], &full[code], sizeof(ABC)) == 0,
           "GetCharABCWidthsA info should match. codepage = %u\n", c[i].cs);

        for (j = 0; j < sizeof range / sizeof range[0]; ++j)
        {
            memset(full, 0xdd, sizeof full);
            ret = pGetCharABCWidthsA(hdc, range[j].first, range[j].last, full);
            ok(ret == c[i].r[j], "GetCharABCWidthsA %x - %x should have %s\n",
               range[j].first, range[j].last, c[i].r[j] ? "succeeded" : "failed");
            if (ret)
            {
                UINT last = range[j].last - range[j].first;
                ret = pGetCharABCWidthsA(hdc, range[j].last, range[j].last, a);
                ok(ret && memcmp(&full[last], &a[0], sizeof(ABC)) == 0,
                   "GetCharABCWidthsA %x should match. codepage = %u\n",
                   range[j].last, c[i].cs);
            }
        }

        hfont = SelectObject(hdc, hfont);
        DeleteObject(hfont);
    }

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Tahoma");
    lf.lfHeight = 200;
    hfont = CreateFontIndirectA(&lf);

    /* test empty glyph's metrics */
    hfont = SelectObject(hdc, hfont);
    ret = pGetCharABCWidthsFloatW(hdc, ' ', ' ', abcf);
    ok(ret, "GetCharABCWidthsFloatW should have succeeded\n");
    ok(abcf[0].abcfB == 1.0, "got %f\n", abcf[0].abcfB);
    ret = pGetCharABCWidthsW(hdc, ' ', ' ', abcw);
    ok(ret, "GetCharABCWidthsW should have succeeded\n");
    ok(abcw[0].abcB == 1, "got %u\n", abcw[0].abcB);

    /* 1) prepare unrotated font metrics */
    ret = pGetCharABCWidthsW(hdc, 'a', 'a', abcw);
    ok(ret, "GetCharABCWidthsW should have succeeded\n");
    DeleteObject(SelectObject(hdc, hfont));

    /* 2) get rotated font metrics */
    lf.lfEscapement = lf.lfOrientation = 900;
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);
    ret = pGetCharABCWidthsW(hdc, 'a', 'a', abc);
    ok(ret, "GetCharABCWidthsW should have succeeded\n");

    /* 3) compare ABC results */
    ok(match_off_by_1(abcw[0].abcA, abc[0].abcA, FALSE),
       "got %d, expected %d (A)\n", abc[0].abcA, abcw[0].abcA);
    ok(match_off_by_1(abcw[0].abcB, abc[0].abcB, FALSE),
       "got %d, expected %d (B)\n", abc[0].abcB, abcw[0].abcB);
    ok(match_off_by_1(abcw[0].abcC, abc[0].abcC, FALSE),
       "got %d, expected %d (C)\n", abc[0].abcC, abcw[0].abcC);

    DeleteObject(SelectObject(hdc, hfont));
    ReleaseDC(NULL, hdc);

    trace("ABC sign test for a variety of transforms:\n");
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Tahoma");
    lf.lfHeight = 20;
    hfont = CreateFontIndirectA(&lf);
    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    hdc = GetDC(hwnd);
    SetMapMode(hdc, MM_ANISOTROPIC);
    SelectObject(hdc, hfont);

    nb = pGetGlyphIndicesW(hdc, str, 1, glyphs, 0);
    ok(nb == 1, "GetGlyphIndicesW should have returned 1\n");

    ret = pGetCharABCWidthsI(hdc, 0, 1, glyphs, abc);
    ok(ret, "GetCharABCWidthsI should have succeeded\n");
    ret = pGetCharABCWidthsW(hdc, 'i', 'i', abcw);
    ok(ret, "GetCharABCWidthsW should have succeeded\n");
    ret = pGetCharABCWidthsFloatW(hdc, 'i', 'i', abcf);
    ok(ret, "GetCharABCWidthsFloatW should have succeeded\n");

    ABCWidths_helper("LTR", hdc, glyphs, abc, abcw, abcf, 0);
    SetWindowExtEx(hdc, -1, -1, NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ABCWidths_helper("LTR -1 compatible", hdc, glyphs, abc, abcw, abcf, 0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ABCWidths_helper("LTR -1 advanced", hdc, glyphs, abc, abcw, abcf, 1);
    SetWindowExtEx(hdc, 1, 1, NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ABCWidths_helper("LTR 1 compatible", hdc, glyphs, abc, abcw, abcf, 0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ABCWidths_helper("LTR 1 advanced", hdc, glyphs, abc, abcw, abcf, 0);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    trace("RTL layout\n");
    hwnd = CreateWindowExA(WS_EX_LAYOUTRTL, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    hdc = GetDC(hwnd);
    SetMapMode(hdc, MM_ANISOTROPIC);
    SelectObject(hdc, hfont);

    ABCWidths_helper("RTL", hdc, glyphs, abc, abcw, abcf, 0);
    SetWindowExtEx(hdc, -1, -1, NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ABCWidths_helper("RTL -1 compatible", hdc, glyphs, abc, abcw, abcf, 0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ABCWidths_helper("RTL -1 advanced", hdc, glyphs, abc, abcw, abcf, 0);
    SetWindowExtEx(hdc, 1, 1, NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ABCWidths_helper("RTL 1 compatible", hdc, glyphs, abc, abcw, abcf, 0);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ABCWidths_helper("RTL 1 advanced", hdc, glyphs, abc, abcw, abcf, 1);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    DeleteObject(hfont);
}

static void test_text_extents(void)
{
    static const WCHAR wt[] = {'O','n','e','\n','t','w','o',' ','3',0};
    LPINT extents;
    INT i, len, fit1, fit2, extents2[3];
    LOGFONTA lf;
    TEXTMETRICA tm;
    HDC hdc;
    HFONT hfont;
    SIZE sz;
    SIZE sz1, sz2;
    BOOL ret;

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

    /* extents functions fail with -ve counts (the interesting case being -1) */
    ret = GetTextExtentPointA(hdc, "o", -1, &sz);
    ok(ret == FALSE, "got %d\n", ret);
    ret = GetTextExtentExPointA(hdc, "o", -1, 0, NULL, NULL, &sz);
    ok(ret == FALSE, "got %d\n", ret);
    ret = GetTextExtentExPointW(hdc, wt, -1, 0, NULL, NULL, &sz1);
    ok(ret == FALSE, "got %d\n", ret);

    /* max_extent = 0 succeeds and returns zero */
    fit1 = fit2 = -215;
    ret = GetTextExtentExPointA(hdc, NULL, 0, 0, &fit1, NULL, &sz);
    ok(ret == TRUE ||
       broken(ret == FALSE), /* NT4, 2k */
       "got %d\n", ret);
    ok(fit1 == 0 ||
       broken(fit1 == -215), /* NT4, 2k */
       "fit = %d\n", fit1);
    ret = GetTextExtentExPointW(hdc, NULL, 0, 0, &fit2, NULL, &sz1);
    ok(ret == TRUE, "got %d\n", ret);
    ok(fit2 == 0, "fit = %d\n", fit2);

    /* max_extent = -1 is interpreted as a very large width that will
     * definitely fit our three characters */
    fit1 = fit2 = -215;
    ret = GetTextExtentExPointA(hdc, "One", 3, -1, &fit1, NULL, &sz);
    ok(ret == TRUE, "got %d\n", ret);
    ok(fit1 == 3, "fit = %d\n", fit1);
    ret = GetTextExtentExPointW(hdc, wt, 3, -1, &fit2, NULL, &sz);
    ok(ret == TRUE, "got %d\n", ret);
    ok(fit2 == 3, "fit = %d\n", fit2);

    /* max_extent = -2 is interpreted similarly, but the Ansi version
     * rejects it while the Unicode one accepts it */
    fit1 = fit2 = -215;
    ret = GetTextExtentExPointA(hdc, "One", 3, -2, &fit1, NULL, &sz);
    ok(ret == FALSE, "got %d\n", ret);
    ok(fit1 == -215, "fit = %d\n", fit1);
    ret = GetTextExtentExPointW(hdc, wt, 3, -2, &fit2, NULL, &sz);
    ok(ret == TRUE, "got %d\n", ret);
    ok(fit2 == 3, "fit = %d\n", fit2);

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);

    /* non-MM_TEXT mapping mode */
    lf.lfHeight = 2000;
    hfont = CreateFontIndirectA(&lf);
    hfont = SelectObject(hdc, hfont);

    SetMapMode( hdc, MM_HIMETRIC );
    ret = GetTextExtentExPointW(hdc, wt, 3, 0, NULL, extents, &sz);
    ok(ret, "got %d\n", ret);
    ok(sz.cx == extents[2], "got %d vs %d\n", sz.cx, extents[2]);

    ret = GetTextExtentExPointW(hdc, wt, 3, extents[1], &fit1, extents2, &sz2);
    ok(ret, "got %d\n", ret);
    ok(fit1 == 2, "got %d\n", fit1);
    ok(sz2.cx == sz.cx, "got %d vs %d\n", sz2.cx, sz.cx);
    for(i = 0; i < 2; i++)
        ok(extents2[i] == extents[i], "%d: %d, %d\n", i, extents2[i], extents[i]);

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);
    HeapFree(GetProcessHeap(), 0, extents);
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
    TEXTMETRICA textm;
    HFONT hOldFont;

    if (!pGetGlyphIndicesW) {
        win_skip("GetGlyphIndicesW not available on platform\n");
        return;
    }

    hdc = GetDC(0);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "System");
    lf.lfHeight = 16;
    lf.lfCharSet = ANSI_CHARSET;

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectEx failed\n");
    ok(GetTextMetricsA(hdc, &textm), "GetTextMetric failed\n");
    if (textm.tmCharSet == ANSI_CHARSET)
    {
        flags |= GGI_MARK_NONEXISTING_GLYPHS;
        charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
        ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
        ok((glyphs[4] == 0x001f || glyphs[4] == 0xffff /* Vista */), "GetGlyphIndicesW should have returned a nonexistent char not %04x\n", glyphs[4]);
        flags = 0;
        charcount = pGetGlyphIndicesW(hdc, testtext, (sizeof(testtext)/2)-1, glyphs, flags);
        ok(charcount == 5, "GetGlyphIndicesW count of glyphs should = 5 not %d\n", charcount);
        ok(glyphs[4] == textm.tmDefaultChar, "GetGlyphIndicesW should have returned a %04x not %04x\n",
                        textm.tmDefaultChar, glyphs[4]);
    }
    else
        /* FIXME: Write tests for non-ANSI charsets. */
        skip("GetGlyphIndices System font tests only for ANSI_CHARSET\n");

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
    ok(GetTextMetricsA(hdc, &textm), "GetTextMetric failed\n");
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
    LOGFONTA lf;
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
        UINT uiRet;

        if (!is_font_installed(kd[i].face_name))
        {
            trace("%s is not installed so skipping this test\n", kd[i].face_name);
            continue;
        }

        trace("testing font %s, height %d\n", kd[i].face_name, kd[i].height);

        memset(&lf, 0, sizeof(lf));
        strcpy(lf.lfFaceName, kd[i].face_name);
        lf.lfHeight = kd[i].height;
        hfont = CreateFontIndirectA(&lf);
        assert(hfont != 0);

        hfont_old = SelectObject(hdc, hfont);

        SetLastError(0xdeadbeef);
        otm.otmSize = sizeof(otm); /* just in case for Win9x compatibility */
        uiRet = GetOutlineTextMetricsW(hdc, sizeof(otm), &otm);
        ok(uiRet == sizeof(otm), "GetOutlineTextMetricsW error %d\n", GetLastError());

        ok(match_off_by_1(kd[i].tmHeight, otm.otmTextMetrics.tmHeight, FALSE), "expected %d, got %d\n",
           kd[i].tmHeight, otm.otmTextMetrics.tmHeight);
        ok(match_off_by_1(kd[i].tmAscent, otm.otmTextMetrics.tmAscent, FALSE), "expected %d, got %d\n",
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
        ok(near_match(kd[i].otmMacAscent, otm.otmMacAscent), "expected %d, got %d\n",
           kd[i].otmMacAscent, otm.otmMacAscent);
todo_wine {
        ok(kd[i].otmsCapEmHeight == otm.otmsCapEmHeight, "expected %u, got %u\n",
           kd[i].otmsCapEmHeight, otm.otmsCapEmHeight);
        ok(kd[i].otmsXHeight == otm.otmsXHeight, "expected %u, got %u\n",
           kd[i].otmsXHeight, otm.otmsXHeight);
        /* FIXME: this one sometimes succeeds due to expected 0, enable it when removing todo */
        if (0) ok(kd[i].otmMacLineGap == otm.otmMacLineGap, "expected %u, got %u\n",
           kd[i].otmMacLineGap, otm.otmMacLineGap);
        ok(kd[i].otmusMinimumPPEM == otm.otmusMinimumPPEM, "expected %u, got %u\n",
           kd[i].otmusMinimumPPEM, otm.otmusMinimumPPEM);
}

        total_kern_pairs = GetKerningPairsW(hdc, 0, NULL);
        trace("total_kern_pairs %u\n", total_kern_pairs);
        kern_pair = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pair));

        /* Win98 (GetKerningPairsA) and XP behave differently here, the test
         * passes on XP.
         */
        SetLastError(0xdeadbeef);
        ret = GetKerningPairsW(hdc, 0, kern_pair);
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "got error %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());
        ok(ret == 0, "got %u, expected 0\n", ret);

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
            /* Disabled to limit console spam */
            if (0 && kern_pair[n].wFirst < 127 && kern_pair[n].wSecond < 127)
                trace("{'%c','%c',%d},\n",
                      kern_pair[n].wFirst, kern_pair[n].wSecond, kern_pair[n].iKernAmount);
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

struct font_data
{
    const char face_name[LF_FACESIZE];
    int requested_height;
    int weight, height, ascent, descent, int_leading, ext_leading, dpi;
    BOOL exact;
};

static void test_height( HDC hdc, const struct font_data *fd )
{
    LOGFONTA lf;
    HFONT hfont, old_hfont;
    TEXTMETRICA tm;
    INT ret, i;

    for (i = 0; fd[i].face_name[0]; i++)
    {
        if (!is_truetype_font_installed(fd[i].face_name))
        {
            skip("%s is not installed\n", fd[i].face_name);
            continue;
        }

        memset(&lf, 0, sizeof(lf));
        lf.lfHeight = fd[i].requested_height;
        lf.lfWeight = fd[i].weight;
        strcpy(lf.lfFaceName, fd[i].face_name);

        hfont = CreateFontIndirectA(&lf);
        assert(hfont);

        old_hfont = SelectObject(hdc, hfont);
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetrics error %d\n", GetLastError());
        if(fd[i].dpi == tm.tmDigitizedAspectX)
        {
            ok(tm.tmWeight == fd[i].weight, "%s(%d): tm.tmWeight %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmWeight, fd[i].weight);
            ok(match_off_by_1(tm.tmHeight, fd[i].height, fd[i].exact), "%s(%d): tm.tmHeight %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmHeight, fd[i].height);
            ok(match_off_by_1(tm.tmAscent, fd[i].ascent, fd[i].exact), "%s(%d): tm.tmAscent %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmAscent, fd[i].ascent);
            ok(match_off_by_1(tm.tmDescent, fd[i].descent, fd[i].exact), "%s(%d): tm.tmDescent %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmDescent, fd[i].descent);
            ok(match_off_by_1(tm.tmInternalLeading, fd[i].int_leading, fd[i].exact), "%s(%d): tm.tmInternalLeading %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmInternalLeading, fd[i].int_leading);
            ok(tm.tmExternalLeading == fd[i].ext_leading, "%s(%d): tm.tmExternalLeading %d != %d\n", fd[i].face_name, fd[i].requested_height, tm.tmExternalLeading, fd[i].ext_leading);
        }

        SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
    }
}

static void *find_ttf_table( void *ttf, DWORD size, DWORD tag )
{
    WORD i, num_tables = GET_BE_WORD(*((WORD *)ttf + 2));
    DWORD *table = (DWORD *)ttf + 3;

    for (i = 0; i < num_tables; i++)
    {
        if (table[0] == tag)
            return (BYTE *)ttf + GET_BE_DWORD(table[2]);
        table += 4;
    }
    return NULL;
}

static void test_height_selection_vdmx( HDC hdc )
{
    static const struct font_data charset_0[] = /* doesn't use VDMX */
    {
        { "wine_vdmx", 10, FW_NORMAL, 10, 8, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 11, FW_NORMAL, 11, 9, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 12, FW_NORMAL, 12, 10, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 13, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 14, FW_NORMAL, 14, 12, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 15, FW_NORMAL, 15, 12, 3, 3, 0, 96, FALSE },
        { "wine_vdmx", 16, FW_NORMAL, 16, 13, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 17, FW_NORMAL, 17, 14, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 18, FW_NORMAL, 18, 15, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 19, FW_NORMAL, 19, 16, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 20, FW_NORMAL, 20, 17, 3, 4, 0, 96, FALSE },
        { "wine_vdmx", 21, FW_NORMAL, 21, 17, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", 22, FW_NORMAL, 22, 18, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", 23, FW_NORMAL, 23, 19, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", 24, FW_NORMAL, 24, 20, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", 25, FW_NORMAL, 25, 21, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", 26, FW_NORMAL, 26, 22, 4, 5, 0, 96, FALSE },
        { "wine_vdmx", 27, FW_NORMAL, 27, 22, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", 28, FW_NORMAL, 28, 23, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", 29, FW_NORMAL, 29, 24, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", 30, FW_NORMAL, 30, 25, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", 31, FW_NORMAL, 31, 26, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", 32, FW_NORMAL, 32, 27, 5, 6, 0, 96, FALSE },
        { "wine_vdmx", 48, FW_NORMAL, 48, 40, 8, 8, 0, 96, TRUE },
        { "wine_vdmx", 64, FW_NORMAL, 64, 53, 11, 11, 0, 96, TRUE },
        { "wine_vdmx", 96, FW_NORMAL, 96, 80, 16, 17, 0, 96, FALSE },
        { "wine_vdmx", -10, FW_NORMAL, 12, 10, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", -11, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", -12, FW_NORMAL, 14, 12, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", -13, FW_NORMAL, 16, 13, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", -14, FW_NORMAL, 17, 14, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", -15, FW_NORMAL, 18, 15, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", -16, FW_NORMAL, 19, 16, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", -17, FW_NORMAL, 21, 17, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", -18, FW_NORMAL, 22, 18, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", -19, FW_NORMAL, 23, 19, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", -20, FW_NORMAL, 24, 20, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", -21, FW_NORMAL, 25, 21, 4, 4, 0, 96, TRUE },
        { "wine_vdmx", -22, FW_NORMAL, 27, 22, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", -23, FW_NORMAL, 28, 23, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", -24, FW_NORMAL, 29, 24, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", -25, FW_NORMAL, 30, 25, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", -26, FW_NORMAL, 31, 26, 5, 5, 0, 96, TRUE },
        { "wine_vdmx", -27, FW_NORMAL, 33, 27, 6, 6, 0, 96, TRUE },
        { "wine_vdmx", -28, FW_NORMAL, 34, 28, 6, 6, 0, 96, TRUE },
        { "wine_vdmx", -29, FW_NORMAL, 35, 29, 6, 6, 0, 96, TRUE },
        { "wine_vdmx", -30, FW_NORMAL, 36, 30, 6, 6, 0, 96, TRUE },
        { "wine_vdmx", -31, FW_NORMAL, 37, 31, 6, 6, 0, 96, TRUE },
        { "wine_vdmx", -32, FW_NORMAL, 39, 32, 7, 7, 0, 96, TRUE },
        { "wine_vdmx", -48, FW_NORMAL, 58, 48, 10, 10, 0, 96, TRUE },
        { "wine_vdmx", -64, FW_NORMAL, 77, 64, 13, 13, 0, 96, TRUE },
        { "wine_vdmx", -96, FW_NORMAL, 116, 96, 20, 20, 0, 96, TRUE },
        { "", 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static const struct font_data charset_1[] = /* Uses VDMX */
    {
        { "wine_vdmx", 10, FW_NORMAL, 10, 8, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 11, FW_NORMAL, 11, 9, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 12, FW_NORMAL, 12, 10, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 13, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 14, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 15, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", 16, FW_NORMAL, 16, 13, 3, 4, 0, 96, TRUE },
        { "wine_vdmx", 17, FW_NORMAL, 16, 13, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 18, FW_NORMAL, 16, 13, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", 19, FW_NORMAL, 19, 15, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 20, FW_NORMAL, 20, 16, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 21, FW_NORMAL, 21, 17, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 22, FW_NORMAL, 22, 18, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 23, FW_NORMAL, 23, 19, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 24, FW_NORMAL, 23, 19, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 25, FW_NORMAL, 25, 21, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", 26, FW_NORMAL, 26, 22, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", 27, FW_NORMAL, 27, 23, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", 28, FW_NORMAL, 27, 23, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", 29, FW_NORMAL, 29, 24, 5, 6, 0, 96, TRUE },
        { "wine_vdmx", 30, FW_NORMAL, 29, 24, 5, 6, 0, 96, TRUE },
        { "wine_vdmx", 31, FW_NORMAL, 29, 24, 5, 6, 0, 96, TRUE },
        { "wine_vdmx", 32, FW_NORMAL, 32, 26, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", 48, FW_NORMAL, 48, 40, 8, 10, 0, 96, TRUE },
        { "wine_vdmx", 64, FW_NORMAL, 64, 54, 10, 13, 0, 96, TRUE },
        { "wine_vdmx", 96, FW_NORMAL, 95, 79, 16, 18, 0, 96, TRUE },
        { "wine_vdmx", -10, FW_NORMAL, 12, 10, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", -11, FW_NORMAL, 13, 11, 2, 2, 0, 96, TRUE },
        { "wine_vdmx", -12, FW_NORMAL, 16, 13, 3, 4, 0, 96, TRUE },
        { "wine_vdmx", -13, FW_NORMAL, 16, 13, 3, 3, 0, 96, TRUE },
        { "wine_vdmx", -14, FW_NORMAL, 19, 15, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -15, FW_NORMAL, 20, 16, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -16, FW_NORMAL, 21, 17, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -17, FW_NORMAL, 22, 18, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -18, FW_NORMAL, 23, 19, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -19, FW_NORMAL, 25, 21, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", -20, FW_NORMAL, 26, 22, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", -21, FW_NORMAL, 27, 23, 4, 6, 0, 96, TRUE },
        { "wine_vdmx", -22, FW_NORMAL, 27, 23, 4, 5, 0, 96, TRUE },
        { "wine_vdmx", -23, FW_NORMAL, 29, 24, 5, 6, 0, 96, TRUE },
        { "wine_vdmx", -24, FW_NORMAL, 32, 26, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", -25, FW_NORMAL, 32, 26, 6, 7, 0, 96, TRUE },
        { "wine_vdmx", -26, FW_NORMAL, 33, 27, 6, 7, 0, 96, TRUE },
        { "wine_vdmx", -27, FW_NORMAL, 35, 29, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", -28, FW_NORMAL, 36, 30, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", -29, FW_NORMAL, 36, 30, 6, 7, 0, 96, TRUE },
        { "wine_vdmx", -30, FW_NORMAL, 38, 32, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", -31, FW_NORMAL, 39, 33, 6, 8, 0, 96, TRUE },
        { "wine_vdmx", -32, FW_NORMAL, 40, 33, 7, 8, 0, 96, TRUE },
        { "wine_vdmx", -48, FW_NORMAL, 60, 50, 10, 12, 0, 96, TRUE },
        { "wine_vdmx", -64, FW_NORMAL, 81, 67, 14, 17, 0, 96, TRUE },
        { "wine_vdmx", -96, FW_NORMAL, 119, 99, 20, 23, 0, 96, TRUE },
        { "", 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static const struct vdmx_data
    {
        WORD version;
        BYTE bCharSet;
        const struct font_data *fd;
    } data[] =
    {
        { 0, 0, charset_0 },
        { 0, 1, charset_1 },
        { 1, 0, charset_0 },
        { 1, 1, charset_1 }
    };
    int i;
    DWORD size, num;
    WORD *vdmx_header;
    BYTE *ratio_rec;
    char ttf_name[MAX_PATH];
    void *res, *copy;

    if (!pAddFontResourceExA)
    {
        win_skip("AddFontResourceExA unavailable\n");
        return;
    }

    for (i = 0; i < sizeof(data) / sizeof(data[0]); i++)
    {
        res = get_res_data( "wine_vdmx.ttf", &size );

        copy = HeapAlloc( GetProcessHeap(), 0, size );
        memcpy( copy, res, size );
        vdmx_header = find_ttf_table( copy, size, MS_MAKE_TAG('V','D','M','X') );
        vdmx_header[0] = GET_BE_WORD( data[i].version );
        ok( GET_BE_WORD( vdmx_header[1] ) == 1, "got %04x\n", GET_BE_WORD( vdmx_header[1] ) );
        ok( GET_BE_WORD( vdmx_header[2] ) == 1, "got %04x\n", GET_BE_WORD( vdmx_header[2] ) );
        ratio_rec = (BYTE *)&vdmx_header[3];
        ratio_rec[0] = data[i].bCharSet;

        write_tmp_file( copy, &size, ttf_name );
        HeapFree( GetProcessHeap(), 0, copy );

        ok( !is_truetype_font_installed("wine_vdmx"), "Already installed\n" );
        num = pAddFontResourceExA( ttf_name, FR_PRIVATE, 0 );
        if (!num) win_skip("Unable to add ttf font resource\n");
        else
        {
            ok( is_truetype_font_installed("wine_vdmx"), "Not installed\n" );
            test_height( hdc, data[i].fd );
            pRemoveFontResourceExA( ttf_name, FR_PRIVATE, 0 );
        }
        DeleteFileA( ttf_name );
    }
}

static void test_height_selection(void)
{
    static const struct font_data tahoma[] =
    {
        {"Tahoma", -12, FW_NORMAL, 14, 12, 2, 2, 0, 96, TRUE },
        {"Tahoma", -24, FW_NORMAL, 29, 24, 5, 5, 0, 96, TRUE },
        {"Tahoma", -48, FW_NORMAL, 58, 48, 10, 10, 0, 96, TRUE },
        {"Tahoma", -96, FW_NORMAL, 116, 96, 20, 20, 0, 96, TRUE },
        {"Tahoma", -192, FW_NORMAL, 232, 192, 40, 40, 0, 96, TRUE },
        {"Tahoma", 12, FW_NORMAL, 12, 10, 2, 2, 0, 96, TRUE },
        {"Tahoma", 24, FW_NORMAL, 24, 20, 4, 4, 0, 96, TRUE },
        {"Tahoma", 48, FW_NORMAL, 48, 40, 8, 8, 0, 96, TRUE },
        {"Tahoma", 96, FW_NORMAL, 96, 80, 16, 17, 0, 96, FALSE },
        {"Tahoma", 192, FW_NORMAL, 192, 159, 33, 33, 0, 96, TRUE },
        {"", 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    HDC hdc = CreateCompatibleDC(0);
    assert(hdc);

    test_height( hdc, tahoma );
    test_height_selection_vdmx( hdc );

    DeleteDC(hdc);
}

static void test_GetOutlineTextMetrics(void)
{
    OUTLINETEXTMETRICA *otm;
    LOGFONTA lf;
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
    hfont = CreateFontIndirectA(&lf);
    assert(hfont != 0);

    hfont_old = SelectObject(hdc, hfont);
    otm_size = GetOutlineTextMetricsA(hdc, 0, NULL);
    trace("otm buffer size %u (0x%x)\n", otm_size, otm_size);

    otm = HeapAlloc(GetProcessHeap(), 0, otm_size);

    memset(otm, 0xAA, otm_size);
    SetLastError(0xdeadbeef);
    otm->otmSize = sizeof(*otm); /* just in case for Win9x compatibility */
    ret = GetOutlineTextMetricsA(hdc, otm->otmSize, otm);
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
    ret = GetOutlineTextMetricsA(hdc, otm->otmSize, otm);
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
    ret = GetOutlineTextMetricsA(hdc, otm->otmSize, otm);
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

static void testJustification(HDC hdc, PCSTR str, RECT *clientArea)
{
    INT         y,
                breakCount,
                areaWidth = clientArea->right - clientArea->left,
                nErrors = 0, e;
    const char *pFirstChar, *pLastChar;
    SIZE        size;
    TEXTMETRICA tm;
    struct err
    {
        const char *start;
        int  len;
        int  GetTextExtentExPointWWidth;
    } error[20];

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
            GetTextExtentPoint32A(hdc, pFirstChar, str - pFirstChar - 1, &size);
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
        GetTextExtentPoint32A(hdc, pFirstChar, pLastChar - pFirstChar, &size);

        /* do not justify the last extent */
        if (*str != '\0' && breakCount > 0)
        {
            SetTextJustification(hdc, areaWidth - size.cx, breakCount);
            GetTextExtentPoint32A(hdc, pFirstChar, pLastChar - pFirstChar, &size);
            if (size.cx != areaWidth && nErrors < sizeof(error)/sizeof(error[0]) - 1)
            {
                error[nErrors].start = pFirstChar;
                error[nErrors].len = pLastChar - pFirstChar;
                error[nErrors].GetTextExtentExPointWWidth = size.cx;
                nErrors++;
            }
        }

        y += size.cy;
        str = pLastChar;
    } while (*str && y < clientArea->bottom);

    for (e = 0; e < nErrors; e++)
    {
        /* The width returned by GetTextExtentPoint32() is exactly the same
           returned by GetTextExtentExPointW() - see dlls/gdi32/font.c */
        ok(error[e].GetTextExtentExPointWWidth == areaWidth,
            "GetTextExtentPointW() for \"%.*s\" should have returned a width of %d, not %d.\n",
           error[e].len, error[e].start, areaWidth, error[e].GetTextExtentExPointWWidth);
    }
}

static void test_SetTextJustification(void)
{
    HDC hdc;
    RECT clientArea;
    LOGFONTA lf;
    HFONT hfont;
    HWND hwnd;
    SIZE size, expect;
    int i;
    WORD indices[2];
    static const char testText[] =
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

    if (!is_font_installed("Times New Roman"))
    {
        skip("Times New Roman is not installed\n");
        return;
    }

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

    if (!pGetGlyphIndicesA || !pGetTextExtentExPointI) goto done;
    pGetGlyphIndicesA( hdc, "A ", 2, indices, 0 );

    SetTextJustification(hdc, 0, 0);
    GetTextExtentPoint32A(hdc, " ", 1, &expect);
    GetTextExtentPoint32A(hdc, "   ", 3, &size);
    ok( size.cx == 3 * expect.cx, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 4, 1);
    GetTextExtentPoint32A(hdc, " ", 1, &size);
    ok( size.cx == expect.cx + 4, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 9, 2);
    GetTextExtentPoint32A(hdc, "  ", 2, &size);
    ok( size.cx == 2 * expect.cx + 9, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 7, 3);
    GetTextExtentPoint32A(hdc, "   ", 3, &size);
    ok( size.cx == 3 * expect.cx + 7, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 7, 3);
    SetTextCharacterExtra(hdc, 2 );
    GetTextExtentPoint32A(hdc, "   ", 3, &size);
    ok( size.cx == 3 * (expect.cx + 2) + 7, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 0, 0);
    SetTextCharacterExtra(hdc, 0);
    size.cx = size.cy = 1234;
    GetTextExtentPoint32A(hdc, " ", 0, &size);
    ok( size.cx == 0 && size.cy == 0, "wrong size %d,%d\n", size.cx, size.cy );
    pGetTextExtentExPointI(hdc, indices, 2, -1, NULL, NULL, &expect);
    SetTextJustification(hdc, 5, 1);
    pGetTextExtentExPointI(hdc, indices, 2, -1, NULL, NULL, &size);
    ok( size.cx == expect.cx + 5, "wrong size %d/%d\n", size.cx, expect.cx );
    SetTextJustification(hdc, 0, 0);

    SetMapMode( hdc, MM_ANISOTROPIC );
    SetWindowExtEx( hdc, 2, 2, NULL );
    GetClientRect( hwnd, &clientArea );
    DPtoLP( hdc, (POINT *)&clientArea, 2 );
    testJustification(hdc, testText, &clientArea);

    GetTextExtentPoint32A(hdc, "A", 1, &expect);
    for (i = 0; i < 10; i++)
    {
        SetTextCharacterExtra(hdc, i);
        GetTextExtentPoint32A(hdc, "A", 1, &size);
        ok( size.cx == expect.cx + i, "wrong size %d/%d+%d\n", size.cx, expect.cx, i );
    }
    SetTextCharacterExtra(hdc, 0);
    pGetTextExtentExPointI(hdc, indices, 1, -1, NULL, NULL, &expect);
    for (i = 0; i < 10; i++)
    {
        SetTextCharacterExtra(hdc, i);
        pGetTextExtentExPointI(hdc, indices, 1, -1, NULL, NULL, &size);
        ok( size.cx == expect.cx + i, "wrong size %d/%d+%d\n", size.cx, expect.cx, i );
    }
    SetTextCharacterExtra(hdc, 0);

    SetViewportExtEx( hdc, 3, 3, NULL );
    GetClientRect( hwnd, &clientArea );
    DPtoLP( hdc, (POINT *)&clientArea, 2 );
    testJustification(hdc, testText, &clientArea);

    GetTextExtentPoint32A(hdc, "A", 1, &expect);
    for (i = 0; i < 10; i++)
    {
        SetTextCharacterExtra(hdc, i);
        GetTextExtentPoint32A(hdc, "A", 1, &size);
        ok( size.cx == expect.cx + i, "wrong size %d/%d+%d\n", size.cx, expect.cx, i );
    }

done:
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

    if (pGdiGetCodePage != NULL && pGdiGetCodePage(hdc) != code_page)
    {
        skip("Font code page %d, looking for code page %d\n",
             pGdiGetCodePage(hdc), code_page);
        ReleaseDC(0, hdc);
        return FALSE;
    }

    if (unicode)
    {
        char ansi_buf[128];
        WCHAR unicode_buf[128];

        for (i = 0; i < count; i++) ansi_buf[i] = (BYTE)(i + 128);

        MultiByteToWideChar(code_page, 0, ansi_buf, count, unicode_buf, count);

        SetLastError(0xdeadbeef);
        ret = pGetGlyphIndicesW(hdc, unicode_buf, count, idx, 0);
        ok(ret == count, "GetGlyphIndicesW expected %d got %d, error %u\n",
           count, ret, GetLastError());
    }
    else
    {
        char ansi_buf[128];

        for (i = 0; i < count; i++) ansi_buf[i] = (BYTE)(i + 128);

        SetLastError(0xdeadbeef);
        ret = pGetGlyphIndicesA(hdc, ansi_buf, count, idx, 0);
        ok(ret == count, "GetGlyphIndicesA expected %d got %d, error %u\n",
           count, ret, GetLastError());
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
        if (get_glyph_indices(cd[i].charset, cd[i].code_page, cd[i].font_idxA, 128, FALSE) &&
            get_glyph_indices(cd[i].charset, cd[i].code_page, cd[i].font_idxW, 128, TRUE))
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

static void test_GdiGetCodePage(void)
{
    static const struct _matching_data
    {
        UINT   current_codepage;
        LPCSTR lfFaceName;
        UCHAR  lfCharSet;
        UINT   expected_codepage;
    } matching_data[] = {
        {1251, "Arial", ANSI_CHARSET, 1252},
        {1251, "Tahoma", ANSI_CHARSET, 1252},

        {1252, "Arial", ANSI_CHARSET, 1252},
        {1252, "Tahoma", ANSI_CHARSET, 1252},

        {1253, "Arial", ANSI_CHARSET, 1252},
        {1253, "Tahoma", ANSI_CHARSET, 1252},

        { 932, "Arial", ANSI_CHARSET, 1252}, /* Japanese Windows returns 1252, not 932 */
        { 932, "Tahoma", ANSI_CHARSET, 1252},
        { 932, "MS UI Gothic", ANSI_CHARSET, 1252},

        { 936, "Arial", ANSI_CHARSET, 936},
        { 936, "Tahoma", ANSI_CHARSET, 936},
        { 936, "Simsun", ANSI_CHARSET, 936},

        { 949, "Arial", ANSI_CHARSET, 949},
        { 949, "Tahoma", ANSI_CHARSET, 949},
        { 949, "Gulim",  ANSI_CHARSET, 949},

        { 950, "Arial", ANSI_CHARSET, 950},
        { 950, "Tahoma", ANSI_CHARSET, 950},
        { 950, "PMingLiU", ANSI_CHARSET, 950},
    };
    HDC         hdc;
    LOGFONTA    lf;
    HFONT       hfont;
    UINT        charset, acp;
    DWORD       codepage;
    int         i;

    if (!pGdiGetCodePage)
    {
        skip("GdiGetCodePage not available on this platform\n");
        return;
    }

    acp = GetACP();

    for (i = 0; i < sizeof(matching_data) / sizeof(struct _matching_data); i++)
    {
        /* only test data matched current locale codepage */
        if (matching_data[i].current_codepage != acp)
            continue;

        if (!is_font_installed(matching_data[i].lfFaceName))
        {
            skip("%s is not installed\n", matching_data[i].lfFaceName);
            continue;
        }

        hdc = GetDC(0);

        memset(&lf, 0, sizeof(lf));
        lf.lfHeight = -16;
        lf.lfCharSet = matching_data[i].lfCharSet;
        lstrcpyA(lf.lfFaceName, matching_data[i].lfFaceName);
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());

        hfont = SelectObject(hdc, hfont);
        charset = GetTextCharset(hdc);
        codepage = pGdiGetCodePage(hdc);
        trace("acp=%d, lfFaceName=%s, lfCharSet=%d, GetTextCharset=%d, GdiGetCodePage=%d, expected codepage=%d\n",
              acp, lf.lfFaceName, lf.lfCharSet, charset, codepage, matching_data[i].expected_codepage);
        ok(codepage == matching_data[i].expected_codepage,
           "GdiGetCodePage should have returned %d, got %d\n", matching_data[i].expected_codepage, codepage);

        hfont = SelectObject(hdc, hfont);
        DeleteObject(hfont);

        /* CLIP_DFA_DISABLE turns off the font association */
        lf.lfClipPrecision = CLIP_DFA_DISABLE;
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());

        hfont = SelectObject(hdc, hfont);
        charset = GetTextCharset(hdc);
        codepage = pGdiGetCodePage(hdc);
        trace("acp=%d, lfFaceName=%s, lfCharSet=%d, GetTextCharset=%d, GdiGetCodePage=%d\n",
              acp, lf.lfFaceName, lf.lfCharSet, charset, codepage);
        ok(codepage == 1252, "GdiGetCodePage returned %d\n", codepage);

        hfont = SelectObject(hdc, hfont);
        DeleteObject(hfont);

        ReleaseDC(NULL, hdc);
    }
}

static void test_GetFontUnicodeRanges(void)
{
    LOGFONTA lf;
    HDC hdc;
    HFONT hfont, hfont_old;
    DWORD size;
    GLYPHSET *gs;
    DWORD i;

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

    if (0) /* Disabled to limit console spam */
        for (i = 0; i < gs->cRanges; i++)
            trace("%03d wcLow %04x cGlyphs %u\n", i, gs->ranges[i].wcLow, gs->ranges[i].cGlyphs);
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
    LOGFONTA lf[MAX_ENUM_FONTS];
};

struct enum_fullname_data
{
    int total;
    ENUMLOGFONTA elf[MAX_ENUM_FONTS];
};

struct enum_font_dataW
{
    int total;
    LOGFONTW lf[MAX_ENUM_FONTS];
};

static INT CALLBACK arial_enum_proc(const LOGFONTA *lf, const TEXTMETRICA *tm, DWORD type, LPARAM lParam)
{
    struct enum_font_data *efd = (struct enum_font_data *)lParam;
    const NEWTEXTMETRICA *ntm = (const NEWTEXTMETRICA *)tm;

    ok(lf->lfHeight == tm->tmHeight, "lfHeight %d != tmHeight %d\n", lf->lfHeight, tm->tmHeight);
    ok(lf->lfHeight > 0 && lf->lfHeight < 200, "enumerated font height %d\n", lf->lfHeight);

    if (type != TRUETYPE_FONTTYPE) return 1;

    ok(ntm->ntmCellHeight + ntm->ntmCellHeight/5 >= ntm->ntmSizeEM, "ntmCellHeight %d should be close to ntmSizeEM %d\n", ntm->ntmCellHeight, ntm->ntmSizeEM);

    if (0) /* Disabled to limit console spam */
        trace("enumed font \"%s\", charset %d, height %d, weight %d, italic %d\n",
              lf->lfFaceName, lf->lfCharSet, lf->lfHeight, lf->lfWeight, lf->lfItalic);
    if (efd->total < MAX_ENUM_FONTS)
        efd->lf[efd->total++] = *lf;
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static INT CALLBACK arial_enum_procw(const LOGFONTW *lf, const TEXTMETRICW *tm, DWORD type, LPARAM lParam)
{
    struct enum_font_dataW *efd = (struct enum_font_dataW *)lParam;
    const NEWTEXTMETRICW *ntm = (const NEWTEXTMETRICW *)tm;

    ok(lf->lfHeight == tm->tmHeight, "lfHeight %d != tmHeight %d\n", lf->lfHeight, tm->tmHeight);
    ok(lf->lfHeight > 0 && lf->lfHeight < 200, "enumerated font height %d\n", lf->lfHeight);

    if (type != TRUETYPE_FONTTYPE) return 1;

    ok(ntm->ntmCellHeight + ntm->ntmCellHeight/5 >= ntm->ntmSizeEM, "ntmCellHeight %d should be close to ntmSizeEM %d\n", ntm->ntmCellHeight, ntm->ntmSizeEM);

    if (0) /* Disabled to limit console spam */
        trace("enumed font %s, charset %d, height %d, weight %d, italic %d\n",
              wine_dbgstr_w(lf->lfFaceName), lf->lfCharSet, lf->lfHeight, lf->lfWeight, lf->lfItalic);
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
    LOGFONTA lf;
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
    ret = EnumFontFamiliesA(hdc, font_name, arial_enum_proc, (LPARAM)&efd);
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
        ok(!strcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
           font_name, efd.lf[i].lfFaceName);
    }

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    strcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    ret = EnumFontFamiliesExA(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
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
                ok(!strcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
                   font_name, efd.lf[i].lfFaceName);
        }
    }

    /* DEFAULT_CHARSET should enumerate all available charsets */
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    strcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    EnumFontFamiliesExA(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
    ok(ret, "EnumFontFamiliesEx error %u\n", GetLastError());
    get_charset_stats(&efd, &ansi_charset, &symbol_charset, &russian_charset);
    trace("enumerated ansi %d, symbol %d, russian %d fonts for %s DEFAULT_CHARSET\n",
          ansi_charset, symbol_charset, russian_charset,
          *font_name ? font_name : "<empty>");
    ok(efd.total > 0, "no fonts enumerated: %s DEFAULT_CHARSET\n", font_name);
    for (i = 0; i < efd.total; i++)
    {
        if (*font_name)
            ok(!strcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
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
    strcpy(lf.lfFaceName, font_name);
    efd.total = 0;
    SetLastError(0xdeadbeef);
    EnumFontFamiliesExA(hdc, &lf, arial_enum_proc, (LPARAM)&efd, 0);
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
                ok(!strcmp(efd.lf[i].lfFaceName, font_name), "expected %s, got %s\n",
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

static INT CALLBACK enum_multi_charset_font_proc(const LOGFONTA *lf, const TEXTMETRICA *tm, DWORD type, LPARAM lParam)
{
    const NEWTEXTMETRICEXA *ntm = (const NEWTEXTMETRICEXA *)tm;
    LOGFONTA *target = (LOGFONTA *)lParam;
    const DWORD valid_bits = 0x003f01ff;
    CHARSETINFO csi;
    DWORD fs;

    if (type != TRUETYPE_FONTTYPE) return TRUE;

    if (TranslateCharsetInfo(ULongToPtr(target->lfCharSet), &csi, TCI_SRCCHARSET)) {
        fs = ntm->ntmFontSig.fsCsb[0] & valid_bits;
        if ((fs & csi.fs.fsCsb[0]) && (fs & ~csi.fs.fsCsb[0]) && (fs & FS_LATIN1)) {
            *target = *lf;
            return FALSE;
        }
    }

    return TRUE;
}

static INT CALLBACK enum_font_data_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    struct enum_font_data *efd = (struct enum_font_data *)lParam;

    if (type != TRUETYPE_FONTTYPE) return 1;

    if (efd->total < MAX_ENUM_FONTS)
        efd->lf[efd->total++] = *lf;
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static INT CALLBACK enum_fullname_data_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    struct enum_fullname_data *efnd = (struct enum_fullname_data *)lParam;

    if (type != TRUETYPE_FONTTYPE) return 1;

    if (efnd->total < MAX_ENUM_FONTS)
        efnd->elf[efnd->total++] = *(ENUMLOGFONTA *)lf;
    else
        trace("enum tests invalid; you have more than %d fonts\n", MAX_ENUM_FONTS);

    return 1;
}

static void test_EnumFontFamiliesEx_default_charset(void)
{
    struct enum_font_data efd;
    LOGFONTA target, enum_font;
    UINT acp;
    HDC hdc;
    CHARSETINFO csi;

    acp = GetACP();
    if (!TranslateCharsetInfo(ULongToPtr(acp), &csi, TCI_SRCCODEPAGE)) {
        skip("TranslateCharsetInfo failed for code page %u.\n", acp);
        return;
    }

    hdc = GetDC(0);
    memset(&enum_font, 0, sizeof(enum_font));
    enum_font.lfCharSet = csi.ciCharset;
    target.lfFaceName[0] = '\0';
    target.lfCharSet = csi.ciCharset;
    EnumFontFamiliesExA(hdc, &enum_font, enum_multi_charset_font_proc, (LPARAM)&target, 0);
    if (target.lfFaceName[0] == '\0') {
        skip("suitable font isn't found for charset %d.\n", enum_font.lfCharSet);
        return;
    }
    if (acp == 874 || acp == 1255 || acp == 1256) {
        /* these codepage use complex script, expecting ANSI_CHARSET here. */
        target.lfCharSet = ANSI_CHARSET;
    }

    efd.total = 0;
    memset(&enum_font, 0, sizeof(enum_font));
    strcpy(enum_font.lfFaceName, target.lfFaceName);
    enum_font.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExA(hdc, &enum_font, enum_font_data_proc, (LPARAM)&efd, 0);
    ReleaseDC(0, hdc);

    trace("'%s' has %d charsets.\n", target.lfFaceName, efd.total);
    if (efd.total < 2) {
        ok(0, "EnumFontFamilies is broken. Expected >= 2, got %d.\n", efd.total);
        return;
    }

    ok(efd.lf[0].lfCharSet == target.lfCharSet,
       "(%s) got charset %d expected %d\n",
       efd.lf[0].lfFaceName, efd.lf[0].lfCharSet, target.lfCharSet);

    return;
}

static void test_negative_width(HDC hdc, const LOGFONTA *lf)
{
    HFONT hfont, hfont_prev;
    DWORD ret;
    GLYPHMETRICS gm1, gm2;
    LOGFONTA lf2 = *lf;
    WORD idx;

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
     * version numbers were taken from the OpenType spec.
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
    ok((tmA->tmPitchAndFamily & 0xf0) == family ||
       broken(PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH),
       "%s: expected family %02x got %02x. panose %d-%d-%d-%d-...\n",
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

static BOOL get_first_last_from_cmap4(void *ptr, DWORD *first, DWORD *last, DWORD limit)
{
    int i;
    cmap_format_4 *cmap = (cmap_format_4*)ptr;
    USHORT seg_count = GET_BE_WORD(cmap->seg_countx2) / 2;

    *first = 0x10000;

    for(i = 0; i < seg_count; i++)
    {
        cmap_format_4_seg seg;

        get_seg4(cmap, i, &seg);

        if(seg.start_count > 0xfffe) break;

        if(*first == 0x10000) *first = seg.start_count;

        *last = min(seg.end_count, 0xfffe);
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
        r = get_first_last_from_cmap4(cmap, first, last, size);
        break;
    default:
        trace("unhandled cmap format %d\n", format);
        break;
    }

end:
    HeapFree(GetProcessHeap(), 0, header);
    return r;
}

#define TT_PLATFORM_APPLE_UNICODE 0
#define TT_PLATFORM_MACINTOSH 1
#define TT_PLATFORM_MICROSOFT 3
#define TT_APPLE_ID_DEFAULT 0
#define TT_APPLE_ID_ISO_10646 2
#define TT_APPLE_ID_UNICODE_2_0 3
#define TT_MS_ID_SYMBOL_CS 0
#define TT_MS_ID_UNICODE_CS 1
#define TT_MS_LANGID_ENGLISH_UNITED_STATES 0x0409
#define TT_NAME_ID_FONT_FAMILY 1
#define TT_NAME_ID_FONT_SUBFAMILY 2
#define TT_NAME_ID_UNIQUE_ID 3
#define TT_NAME_ID_FULL_NAME 4
#define TT_MAC_ID_SIMPLIFIED_CHINESE    25

typedef struct sfnt_name
{
    USHORT platform_id;
    USHORT encoding_id;
    USHORT language_id;
    USHORT name_id;
    USHORT length;
    USHORT offset;
} sfnt_name;

static const LANGID mac_langid_table[] =
{
    MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ENGLISH */
    MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_FRENCH */
    MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_GERMAN */
    MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ITALIAN */
    MAKELANGID(LANG_DUTCH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_DUTCH */
    MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWEDISH */
    MAKELANGID(LANG_SPANISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SPANISH */
    MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_DANISH */
    MAKELANGID(LANG_PORTUGUESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_PORTUGUESE */
    MAKELANGID(LANG_NORWEGIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_NORWEGIAN */
    MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_HEBREW */
    MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_JAPANESE */
    MAKELANGID(LANG_ARABIC,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_ARABIC */
    MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_FINNISH */
    MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_GREEK */
    MAKELANGID(LANG_ICELANDIC,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ICELANDIC */
    MAKELANGID(LANG_MALTESE,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MALTESE */
    MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKISH */
    MAKELANGID(LANG_CROATIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_CROATIAN */
    MAKELANGID(LANG_CHINESE_TRADITIONAL,SUBLANG_DEFAULT),    /* TT_MAC_LANGID_CHINESE_TRADITIONAL */
    MAKELANGID(LANG_URDU,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_URDU */
    MAKELANGID(LANG_HINDI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_HINDI */
    MAKELANGID(LANG_THAI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_THAI */
    MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KOREAN */
    MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_LITHUANIAN */
    MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_POLISH */
    MAKELANGID(LANG_HUNGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_HUNGARIAN */
    MAKELANGID(LANG_ESTONIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ESTONIAN */
    MAKELANGID(LANG_LATVIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_LETTISH */
    MAKELANGID(LANG_SAMI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_SAAMISK */
    MAKELANGID(LANG_FAEROESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_FAEROESE */
    MAKELANGID(LANG_FARSI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_FARSI */
    MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_RUSSIAN */
    MAKELANGID(LANG_CHINESE_SIMPLIFIED,SUBLANG_DEFAULT),     /* TT_MAC_LANGID_CHINESE_SIMPLIFIED */
    MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN),            /* TT_MAC_LANGID_FLEMISH */
    MAKELANGID(LANG_IRISH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_IRISH */
    MAKELANGID(LANG_ALBANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ALBANIAN */
    MAKELANGID(LANG_ROMANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ROMANIAN */
    MAKELANGID(LANG_CZECH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_CZECH */
    MAKELANGID(LANG_SLOVAK,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SLOVAK */
    MAKELANGID(LANG_SLOVENIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SLOVENIAN */
    0,                                                       /* TT_MAC_LANGID_YIDDISH */
    MAKELANGID(LANG_SERBIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SERBIAN */
    MAKELANGID(LANG_MACEDONIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_MACEDONIAN */
    MAKELANGID(LANG_BULGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_BULGARIAN */
    MAKELANGID(LANG_UKRAINIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_UKRAINIAN */
    MAKELANGID(LANG_BELARUSIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_BYELORUSSIAN */
    MAKELANGID(LANG_UZBEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_UZBEK */
    MAKELANGID(LANG_KAZAK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KAZAKH */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_CYRILLIC),           /* TT_MAC_LANGID_AZERBAIJANI */
    0,                                                       /* TT_MAC_LANGID_AZERBAIJANI_ARABIC_SCRIPT */
    MAKELANGID(LANG_ARMENIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ARMENIAN */
    MAKELANGID(LANG_GEORGIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GEORGIAN */
    0,                                                       /* TT_MAC_LANGID_MOLDAVIAN */
    MAKELANGID(LANG_KYRGYZ,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KIRGHIZ */
    MAKELANGID(LANG_TAJIK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAJIKI */
    MAKELANGID(LANG_TURKMEN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKMEN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MONGOLIAN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA), /* TT_MAC_LANGID_MONGOLIAN_CYRILLIC_SCRIPT */
    MAKELANGID(LANG_PASHTO,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_PASHTO */
    0,                                                       /* TT_MAC_LANGID_KURDISH */
    MAKELANGID(LANG_KASHMIRI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_KASHMIRI */
    MAKELANGID(LANG_SINDHI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SINDHI */
    MAKELANGID(LANG_TIBETAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TIBETAN */
    MAKELANGID(LANG_NEPALI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_NEPALI */
    MAKELANGID(LANG_SANSKRIT,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_SANSKRIT */
    MAKELANGID(LANG_MARATHI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MARATHI */
    MAKELANGID(LANG_BENGALI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_BENGALI */
    MAKELANGID(LANG_ASSAMESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ASSAMESE */
    MAKELANGID(LANG_GUJARATI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GUJARATI */
    MAKELANGID(LANG_PUNJABI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_PUNJABI */
    MAKELANGID(LANG_ORIYA,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_ORIYA */
    MAKELANGID(LANG_MALAYALAM,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MALAYALAM */
    MAKELANGID(LANG_KANNADA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_KANNADA */
    MAKELANGID(LANG_TAMIL,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAMIL */
    MAKELANGID(LANG_TELUGU,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_TELUGU */
    MAKELANGID(LANG_SINHALESE,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SINHALESE */
    0,                                                       /* TT_MAC_LANGID_BURMESE */
    MAKELANGID(LANG_KHMER,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KHMER */
    MAKELANGID(LANG_LAO,SUBLANG_DEFAULT),                    /* TT_MAC_LANGID_LAO */
    MAKELANGID(LANG_VIETNAMESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_VIETNAMESE */
    MAKELANGID(LANG_INDONESIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_INDONESIAN */
    0,                                                       /* TT_MAC_LANGID_TAGALOG */
    MAKELANGID(LANG_MALAY,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_MALAY_ROMAN_SCRIPT */
    0,                                                       /* TT_MAC_LANGID_MALAY_ARABIC_SCRIPT */
    MAKELANGID(LANG_AMHARIC,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_AMHARIC */
    MAKELANGID(LANG_TIGRIGNA,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_TIGRINYA */
    0,                                                       /* TT_MAC_LANGID_GALLA */
    0,                                                       /* TT_MAC_LANGID_SOMALI */
    MAKELANGID(LANG_SWAHILI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWAHILI */
    0,                                                       /* TT_MAC_LANGID_RUANDA */
    0,                                                       /* TT_MAC_LANGID_RUNDI */
    0,                                                       /* TT_MAC_LANGID_CHEWA */
    MAKELANGID(LANG_MALAGASY,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_MALAGASY */
    MAKELANGID(LANG_ESPERANTO,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ESPERANTO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* 95-111 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 112-127 */
    MAKELANGID(LANG_WELSH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_WELSH */
    MAKELANGID(LANG_BASQUE,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BASQUE */
    MAKELANGID(LANG_CATALAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_CATALAN */
    0,                                                       /* TT_MAC_LANGID_LATIN */
    MAKELANGID(LANG_QUECHUA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_QUECHUA */
    0,                                                       /* TT_MAC_LANGID_GUARANI */
    0,                                                       /* TT_MAC_LANGID_AYMARA */
    MAKELANGID(LANG_TATAR,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TATAR */
    MAKELANGID(LANG_UIGHUR,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_UIGHUR */
    0,                                                       /* TT_MAC_LANGID_DZONGKHA */
    0,                                                       /* TT_MAC_LANGID_JAVANESE */
    0,                                                       /* TT_MAC_LANGID_SUNDANESE */
    MAKELANGID(LANG_GALICIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GALICIAN */
    MAKELANGID(LANG_AFRIKAANS,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_AFRIKAANS */
    MAKELANGID(LANG_BRETON,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BRETON */
    MAKELANGID(LANG_INUKTITUT,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_INUKTITUT */
    MAKELANGID(LANG_SCOTTISH_GAELIC,SUBLANG_DEFAULT),        /* TT_MAC_LANGID_SCOTTISH_GAELIC */
    MAKELANGID(LANG_MANX_GAELIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_MANX_GAELIC */
    MAKELANGID(LANG_IRISH,SUBLANG_IRISH_IRELAND),            /* TT_MAC_LANGID_IRISH_GAELIC */
    0,                                                       /* TT_MAC_LANGID_TONGAN */
    0,                                                       /* TT_MAC_LANGID_GREEK_POLYTONIC */
    MAKELANGID(LANG_GREENLANDIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_GREELANDIC */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN),              /* TT_MAC_LANGID_AZERBAIJANI_ROMAN_SCRIPT */
};

static inline WORD get_mac_code_page( const sfnt_name *name )
{
    if (GET_BE_WORD(name->encoding_id) == TT_MAC_ID_SIMPLIFIED_CHINESE) return 10008;  /* special case */
    return 10000 + GET_BE_WORD(name->encoding_id);
}

static int match_name_table_language( const sfnt_name *name, LANGID lang )
{
    LANGID name_lang;
    int res = 0;

    switch (GET_BE_WORD(name->platform_id))
    {
    case TT_PLATFORM_MICROSOFT:
        res += 5;  /* prefer the Microsoft name */
        switch (GET_BE_WORD(name->encoding_id))
        {
        case TT_MS_ID_UNICODE_CS:
        case TT_MS_ID_SYMBOL_CS:
            name_lang = GET_BE_WORD(name->language_id);
            break;
        default:
            return 0;
        }
        break;
    case TT_PLATFORM_MACINTOSH:
        if (!IsValidCodePage( get_mac_code_page( name ))) return 0;
        if (GET_BE_WORD(name->language_id) >= sizeof(mac_langid_table)/sizeof(mac_langid_table[0])) return 0;
        name_lang = mac_langid_table[GET_BE_WORD(name->language_id)];
        break;
    case TT_PLATFORM_APPLE_UNICODE:
        res += 2;  /* prefer Unicode encodings */
        switch (GET_BE_WORD(name->encoding_id))
        {
        case TT_APPLE_ID_DEFAULT:
        case TT_APPLE_ID_ISO_10646:
        case TT_APPLE_ID_UNICODE_2_0:
            if (GET_BE_WORD(name->language_id) >= sizeof(mac_langid_table)/sizeof(mac_langid_table[0])) return 0;
            name_lang = mac_langid_table[GET_BE_WORD(name->language_id)];
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    if (name_lang == lang) res += 30;
    else if (PRIMARYLANGID( name_lang ) == PRIMARYLANGID( lang )) res += 20;
    else if (name_lang == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )) res += 10;
    return res;
}

static BOOL get_ttf_nametable_entry(HDC hdc, WORD name_id, WCHAR *out_buf, SIZE_T out_size, LCID language_id)
{
    struct sfnt_name_header
    {
        USHORT format;
        USHORT number_of_record;
        USHORT storage_offset;
    } *header;
    sfnt_name *entry;
    BOOL r = FALSE;
    LONG size, offset, length;
    LONG c, ret;
    WCHAR *name;
    BYTE *data;
    USHORT i;
    int res, best_lang = 0, best_index = -1;

    size = GetFontData(hdc, MS_NAME_TAG, 0, NULL, 0);
    ok(size != GDI_ERROR, "no name table found\n");
    if(size == GDI_ERROR) return FALSE;

    data = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetFontData(hdc, MS_NAME_TAG, 0, data, size);
    ok(ret == size, "GetFontData should return %u not %u\n", size, ret);

    header = (void *)data;
    header->format = GET_BE_WORD(header->format);
    header->number_of_record = GET_BE_WORD(header->number_of_record);
    header->storage_offset = GET_BE_WORD(header->storage_offset);
    if (header->format != 0)
    {
        trace("got format %u\n", header->format);
        goto out;
    }
    if (header->number_of_record == 0 || sizeof(*header) + header->number_of_record * sizeof(*entry) > size)
    {
        trace("number records out of range: %d\n", header->number_of_record);
        goto out;
    }
    if (header->storage_offset >= size)
    {
        trace("storage_offset %u > size %u\n", header->storage_offset, size);
        goto out;
    }

    entry = (void *)&header[1];
    for (i = 0; i < header->number_of_record; i++)
    {
        if (GET_BE_WORD(entry[i].name_id) != name_id) continue;
        res = match_name_table_language( &entry[i], language_id);
        if (res > best_lang)
        {
            best_lang = res;
            best_index = i;
        }
    }

    offset = header->storage_offset + GET_BE_WORD(entry[best_index].offset);
    length = GET_BE_WORD(entry[best_index].length);
    if (offset + length > size)
    {
        trace("entry %d is out of range\n", best_index);
        goto out;
    }
    if (length >= out_size)
    {
        trace("buffer too small for entry %d\n", best_index);
        goto out;
    }

    name = (WCHAR *)(data + offset);
    for (c = 0; c < length / 2; c++)
        out_buf[c] = GET_BE_WORD(name[c]);
    out_buf[c] = 0;

    r = TRUE;

out:
    HeapFree(GetProcessHeap(), 0, data);
    return r;
}

static void test_text_metrics(const LOGFONTA *lf, const NEWTEXTMETRICA *ntm)
{
    HDC hdc;
    HFONT hfont, hfont_old;
    TEXTMETRICA tmA;
    TT_OS2_V2 tt_os2;
    LONG size, ret;
    const char *font_name = lf->lfFaceName;
    DWORD cmap_first = 0, cmap_last = 0;
    UINT ascent, descent, cell_height;
    cmap_type cmap_type;
    BOOL sys_lang_non_english;

    sys_lang_non_english = PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH;
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
        skip("%s is not a Windows font, OS/2 metrics may be invalid.\n",font_name);
    }
    else
    {
        USHORT expect_first_A, expect_last_A, expect_break_A, expect_default_A;
        USHORT expect_first_W, expect_last_W, expect_break_W, expect_default_W;
        UINT os2_first_char, os2_last_char, default_char, break_char;
        USHORT version;
        TEXTMETRICW tmW;

        ascent = GET_BE_WORD(tt_os2.usWinAscent);
        descent = GET_BE_WORD(tt_os2.usWinDescent);
        cell_height = ascent + descent;
        ok(ntm->ntmCellHeight == cell_height, "%s: ntmCellHeight %u != %u, os2.usWinAscent/os2.usWinDescent %u/%u\n",
           font_name, ntm->ntmCellHeight, cell_height, ascent, descent);

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
            case 1255:  /* Hebrew */
                expect_last_W = 0xf896;
                break;
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
            expect_last_W     = cmap_last;
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
        if (pGdiGetCodePage == NULL || ! IsDBCSLeadByteEx(pGdiGetCodePage(hdc), tmA.tmLastChar))
            ok(tmA.tmLastChar == expect_last_A ||
               tmA.tmLastChar == 0xff /* win9x */,
               "A: tmLastChar for %s got %02x expected %02x\n", font_name, tmA.tmLastChar, expect_last_A);
        else
           skip("tmLastChar is DBCS lead byte\n");
        ok(tmA.tmBreakChar == expect_break_A, "A: tmBreakChar for %s got %02x expected %02x\n",
           font_name, tmA.tmBreakChar, expect_break_A);
        ok(tmA.tmDefaultChar == expect_default_A || broken(sys_lang_non_english),
           "A: tmDefaultChar for %s got %02x expected %02x\n",
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
            ok(tmW.tmDefaultChar == expect_default_W || broken(sys_lang_non_english),
               "W: tmDefaultChar for %s got %02x expected %02x\n",
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

static INT CALLBACK enum_truetype_font_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    INT *enumed = (INT *)lParam;

    if (type == TRUETYPE_FONTTYPE)
    {
        (*enumed)++;
        test_text_metrics(lf, (const NEWTEXTMETRICA *)ntm);
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
    ok(cs == expected_cs || cs == ANSI_CHARSET, "expected %d, got %d\n", expected_cs, cs);
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
        ok(cs == expected_cs || cs == ANSI_CHARSET, "expected %d, got %d for font %s\n", expected_cs, cs, font_subst[i].name);
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

static void test_oemcharset(void)
{
    HDC hdc;
    LOGFONTA lf, clf;
    HFONT hfont, old_hfont;
    int charset;

    hdc = CreateCompatibleDC(0);
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = 12;
    lf.lfCharSet = OEM_CHARSET;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lstrcpyA(lf.lfFaceName, "Terminal");
    hfont = CreateFontIndirectA(&lf);
    old_hfont = SelectObject(hdc, hfont);
    charset = GetTextCharset(hdc);
todo_wine
    ok(charset == OEM_CHARSET, "expected %d charset, got %d\n", OEM_CHARSET, charset);
    hfont = SelectObject(hdc, old_hfont);
    GetObjectA(hfont, sizeof(clf), &clf);
    ok(!lstrcmpA(clf.lfFaceName, lf.lfFaceName), "expected %s face name, got %s\n", lf.lfFaceName, clf.lfFaceName);
    ok(clf.lfPitchAndFamily == lf.lfPitchAndFamily, "expected %x family, got %x\n", lf.lfPitchAndFamily, clf.lfPitchAndFamily);
    ok(clf.lfCharSet == lf.lfCharSet, "expected %d charset, got %d\n", lf.lfCharSet, clf.lfCharSet);
    ok(clf.lfHeight == lf.lfHeight, "expected %d height, got %d\n", lf.lfHeight, clf.lfHeight);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

static int CALLBACK create_fixed_pitch_font_proc(const LOGFONTA *lpelfe,
                                                 const TEXTMETRICA *lpntme,
                                                 DWORD FontType, LPARAM lParam)
{
    const NEWTEXTMETRICEXA *lpntmex = (const NEWTEXTMETRICEXA *)lpntme;
    CHARSETINFO csi;
    LOGFONTA lf = *lpelfe;
    HFONT hfont;
    DWORD found_subset;

    /* skip bitmap, proportional or vertical font */
    if ((FontType & TRUETYPE_FONTTYPE) == 0 ||
        (lf.lfPitchAndFamily & 0xf) != FIXED_PITCH ||
        lf.lfFaceName[0] == '@')
        return 1;

    /* skip linked font */
    if (!TranslateCharsetInfo((DWORD*)(INT_PTR)lpelfe->lfCharSet, &csi, TCI_SRCCHARSET) ||
        (lpntmex->ntmFontSig.fsCsb[0] & csi.fs.fsCsb[0]) == 0)
        return 1;

    /* skip linked font, like SimSun-ExtB */
    switch (lpelfe->lfCharSet) {
    case SHIFTJIS_CHARSET:
        found_subset = lpntmex->ntmFontSig.fsUsb[1] & (1 << 17); /* Hiragana */
        break;
    case GB2312_CHARSET:
    case CHINESEBIG5_CHARSET:
        found_subset = lpntmex->ntmFontSig.fsUsb[1] & (1 << 16); /* CJK Symbols And Punctuation */
        break;
    case HANGEUL_CHARSET:
        found_subset = lpntmex->ntmFontSig.fsUsb[1] & (1 << 24); /* Hangul Syllables */
        break;
    default:
        found_subset = lpntmex->ntmFontSig.fsUsb[0] & (1 <<  0); /* Basic Latin */
        break;
    }
    if (!found_subset)
        return 1;

    /* test with an odd height */
    lf.lfHeight = -19;
    lf.lfWidth = 0;
    hfont = CreateFontIndirectA(&lf);
    if (hfont)
    {
        *(HFONT *)lParam = hfont;
        return 0;
    }
    return 1;
}

static void test_GetGlyphOutline(void)
{
    HDC hdc;
    GLYPHMETRICS gm, gm2;
    LOGFONTA lf;
    HFONT hfont, old_hfont;
    INT ret, ret2;
    const UINT fmt[] = { GGO_METRICS, GGO_BITMAP, GGO_GRAY2_BITMAP,
                         GGO_GRAY4_BITMAP, GGO_GRAY8_BITMAP };
    static const struct
    {
        UINT cs;
        UINT a;
        UINT w;
    } c[] =
    {
        {ANSI_CHARSET, 0x30, 0x30},
        {SHIFTJIS_CHARSET, 0x82a0, 0x3042},
        {HANGEUL_CHARSET, 0x8141, 0xac02},
        {GB2312_CHARSET, 0x8141, 0x4e04},
        {CHINESEBIG5_CHARSET, 0xa142, 0x3001}
    };
    UINT i;

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
    {
        ok(ret == 0, "GetGlyphOutlineW should return 0 buffer size for space char\n");
        ok(gm.gmBlackBoxX == 1, "Expected 1, got %u\n", gm.gmBlackBoxX);
        ok(gm.gmBlackBoxY == 1, "Expected 1, got %u\n", gm.gmBlackBoxY);
    }

    /* requesting buffer size for space char + error */
    memset(&gm, 0, sizeof(gm));
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineW(0, ' ', GGO_NATIVE, &gm, 0, NULL, NULL);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
       ok(ret == GDI_ERROR, "GetGlyphOutlineW should return GDI_ERROR\n");
       ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %u\n", GetLastError());
       ok(gm.gmBlackBoxX == 0, "Expected 0, got %u\n", gm.gmBlackBoxX);
       ok(gm.gmBlackBoxY == 0, "Expected 0, got %u\n", gm.gmBlackBoxY);
    }

    /* test GetGlyphOutline with a buffer too small */
    SetLastError(0xdeadbeef);
    ret = GetGlyphOutlineA(hdc, 'A', GGO_NATIVE, &gm, sizeof(i), &i, &mat);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        ok(ret == GDI_ERROR, "GetGlyphOutlineW should return an error when the buffer size is too small.\n");

    for (i = 0; i < sizeof(fmt) / sizeof(fmt[0]); ++i)
    {
        DWORD dummy;

        memset(&gm, 0xab, sizeof(gm));
        SetLastError(0xdeadbeef);
        ret = GetGlyphOutlineW(hdc, ' ', fmt[i], &gm, 0, NULL, &mat);
        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        {
            if (fmt[i] == GGO_METRICS)
                ok(ret != GDI_ERROR, "%2d:GetGlyphOutlineW should succeed, got %d\n", fmt[i], ret);
            else
                ok(ret == 0, "%2d:GetGlyphOutlineW should return 0, got %d\n", fmt[i], ret);
            ok(gm.gmBlackBoxX == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxX);
            ok(gm.gmBlackBoxY == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxY);
        }

        memset(&gm, 0xab, sizeof(gm));
        SetLastError(0xdeadbeef);
        ret = GetGlyphOutlineW(hdc, ' ', fmt[i], &gm, 0, &dummy, &mat);
        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        {
            if (fmt[i] == GGO_METRICS)
                ok(ret != GDI_ERROR, "%2d:GetGlyphOutlineW should succeed, got %d\n", fmt[i], ret);
            else
                ok(ret == 0, "%2d:GetGlyphOutlineW should return 0, got %d\n", fmt[i], ret);
            ok(gm.gmBlackBoxX == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxX);
            ok(gm.gmBlackBoxY == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxY);
        }

        memset(&gm, 0xab, sizeof(gm));
        SetLastError(0xdeadbeef);
        ret = GetGlyphOutlineW(hdc, ' ', fmt[i], &gm, sizeof(dummy), NULL, &mat);
        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        {
            if (fmt[i] == GGO_METRICS)
                ok(ret != GDI_ERROR, "%2d:GetGlyphOutlineW should succeed, got %d\n", fmt[i], ret);
            else
                ok(ret == 0, "%2d:GetGlyphOutlineW should return 0, got %d\n", fmt[i], ret);
            ok(gm.gmBlackBoxX == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxX);
            ok(gm.gmBlackBoxY == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxY);
        }

        memset(&gm, 0xab, sizeof(gm));
        SetLastError(0xdeadbeef);
        ret = GetGlyphOutlineW(hdc, ' ', fmt[i], &gm, sizeof(dummy), &dummy, &mat);
        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        {
            if (fmt[i] == GGO_METRICS) {
                ok(ret != GDI_ERROR, "%2d:GetGlyphOutlineW should succeed, got %d\n", fmt[i], ret);
                ok(gm.gmBlackBoxX == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxX);
                ok(gm.gmBlackBoxY == 1, "%2d:expected 1, got %u\n", fmt[i], gm.gmBlackBoxY);
            }
            else
            {
                ok(ret == GDI_ERROR, "%2d:GetGlyphOutlineW should return GDI_ERROR, got %d\n", fmt[i], ret);
                memset(&gm2, 0xab, sizeof(gm2));
                ok(memcmp(&gm, &gm2, sizeof(GLYPHMETRICS)) == 0,
                   "%2d:GLYPHMETRICS shouldn't be touched on error\n", fmt[i]);
            }
        }
    }

    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    for (i = 0; i < sizeof c / sizeof c[0]; ++i)
    {
        static const MAT2 rotate_mat = {{0, 0}, {0, -1}, {0, 1}, {0, 0}};
        TEXTMETRICA tm;

        lf.lfFaceName[0] = '\0';
        lf.lfCharSet = c[i].cs;
        lf.lfPitchAndFamily = 0;
        if (EnumFontFamiliesExA(hdc, &lf, create_font_proc, (LPARAM)&hfont, 0))
        {
            skip("TrueType font for charset %u is not installed\n", c[i].cs);
            continue;
        }

        old_hfont = SelectObject(hdc, hfont);

        /* expected to ignore superfluous bytes (sigle-byte character) */
        ret = GetGlyphOutlineA(hdc, 0x8041, GGO_BITMAP, &gm, 0, NULL, &mat);
        ret2 = GetGlyphOutlineA(hdc, 0x41, GGO_BITMAP, &gm2, 0, NULL, &mat);
        ok(ret == ret2 && memcmp(&gm, &gm2, sizeof gm) == 0, "%d %d\n", ret, ret2);

        ret = GetGlyphOutlineA(hdc, 0xcc8041, GGO_BITMAP, &gm, 0, NULL, &mat);
        ok(ret == ret2 && memcmp(&gm, &gm2, sizeof gm) == 0,
           "Expected to ignore superfluous bytes, got %d %d\n", ret, ret2);

        /* expected to ignore superfluous bytes (double-byte character) */
        ret = GetGlyphOutlineA(hdc, c[i].a, GGO_BITMAP, &gm, 0, NULL, &mat);
        ret2 = GetGlyphOutlineA(hdc, c[i].a | 0xdead0000, GGO_BITMAP, &gm2, 0, NULL, &mat);
        ok(ret == ret2 && memcmp(&gm, &gm2, sizeof gm) == 0,
           "Expected to ignore superfluous bytes, got %d %d\n", ret, ret2);

        /* expected to match wide-char version results */
        ret2 = GetGlyphOutlineW(hdc, c[i].w, GGO_BITMAP, &gm2, 0, NULL, &mat);
        ok(ret == ret2 && memcmp(&gm, &gm2, sizeof gm) == 0, "%d %d\n", ret, ret2);

        if (EnumFontFamiliesExA(hdc, &lf, create_fixed_pitch_font_proc, (LPARAM)&hfont, 0))
        {
            skip("Fixed-pitch TrueType font for charset %u is not available\n", c[i].cs);
            continue;
        }
        DeleteObject(SelectObject(hdc, hfont));
        if (c[i].a <= 0xff)
        {
            DeleteObject(SelectObject(hdc, old_hfont));
            continue;
        }

        ret = GetObjectA(hfont, sizeof lf, &lf);
        ok(ret > 0, "GetObject error %u\n", GetLastError());

        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetrics error %u\n", GetLastError());
        ret = GetGlyphOutlineA(hdc, c[i].a, GGO_METRICS, &gm2, 0, NULL, &mat);
        ok(ret != GDI_ERROR, "GetGlyphOutlineA error %u\n", GetLastError());
        trace("Tests with height=%d,avg=%d,full=%d,face=%s,charset=%d\n",
              -lf.lfHeight, tm.tmAveCharWidth, gm2.gmCellIncX, lf.lfFaceName, lf.lfCharSet);
        ok(gm2.gmCellIncX == tm.tmAveCharWidth * 2 || broken(gm2.gmCellIncX == -lf.lfHeight),
           "expected %d, got %d (%s:%d)\n",
           tm.tmAveCharWidth * 2, gm2.gmCellIncX, lf.lfFaceName, lf.lfCharSet);

        ret = GetGlyphOutlineA(hdc, c[i].a, GGO_METRICS, &gm2, 0, NULL, &rotate_mat);
        ok(ret != GDI_ERROR, "GetGlyphOutlineA error %u\n", GetLastError());
        ok(gm2.gmCellIncY == -lf.lfHeight,
           "expected %d, got %d (%s:%d)\n",
           -lf.lfHeight, gm2.gmCellIncY, lf.lfFaceName, lf.lfCharSet);

        lf.lfItalic = TRUE;
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != NULL, "CreateFontIndirect error %u\n", GetLastError());
        DeleteObject(SelectObject(hdc, hfont));
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetrics error %u\n", GetLastError());
        ret = GetGlyphOutlineA(hdc, c[i].a, GGO_METRICS, &gm2, 0, NULL, &mat);
        ok(ret != GDI_ERROR, "GetGlyphOutlineA error %u\n", GetLastError());
        ok(gm2.gmCellIncX == tm.tmAveCharWidth * 2 || broken(gm2.gmCellIncX == -lf.lfHeight),
           "expected %d, got %d (%s:%d)\n",
           tm.tmAveCharWidth * 2, gm2.gmCellIncX, lf.lfFaceName, lf.lfCharSet);

        lf.lfItalic = FALSE;
        lf.lfEscapement = lf.lfOrientation = 2700;
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != NULL, "CreateFontIndirect error %u\n", GetLastError());
        DeleteObject(SelectObject(hdc, hfont));
        ret = GetGlyphOutlineA(hdc, c[i].a, GGO_METRICS, &gm2, 0, NULL, &mat);
        ok(ret != GDI_ERROR, "GetGlyphOutlineA error %u\n", GetLastError());
        ok(gm2.gmCellIncY == -lf.lfHeight,
           "expected %d, got %d (%s:%d)\n",
           -lf.lfHeight, gm2.gmCellIncY, lf.lfFaceName, lf.lfCharSet);

        hfont = SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
    }

    DeleteDC(hdc);
}

/* bug #9995: there is a limit to the character width that can be specified */
static void test_GetTextMetrics2(const char *fontname, int font_height)
{
    HFONT of, hf;
    HDC hdc;
    TEXTMETRICA tm;
    BOOL ret;
    int ave_width, height, width, ratio, scale;

    if (!is_truetype_font_installed( fontname)) {
        skip("%s is not installed\n", fontname);
        return;
    }
    hdc = CreateCompatibleDC(0);
    ok( hdc != NULL, "CreateCompatibleDC failed\n");
    /* select width = 0 */
    hf = CreateFontA(font_height, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_LH_ANGLES,
            DEFAULT_QUALITY, VARIABLE_PITCH,
            fontname);
    ok( hf != NULL, "CreateFontA(%s, %d) failed\n", fontname, font_height);
    of = SelectObject( hdc, hf);
    ret = GetTextMetricsA( hdc, &tm);
    ok(ret, "GetTextMetricsA error %u\n", GetLastError());
    height = tm.tmHeight;
    ave_width = tm.tmAveCharWidth;
    SelectObject( hdc, of);
    DeleteObject( hf);

    trace("height %d, ave width %d\n", height, ave_width);

    for (width = ave_width * 2; /* nothing*/; width += ave_width)
    {
        hf = CreateFontA(height, width, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_LH_ANGLES,
                        DEFAULT_QUALITY, VARIABLE_PITCH, fontname);
        ok(hf != 0, "CreateFont failed\n");
        of = SelectObject(hdc, hf);
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetrics error %u\n", GetLastError());
        SelectObject(hdc, of);
        DeleteObject(hf);

        if (match_off_by_1(tm.tmAveCharWidth, ave_width, FALSE) || width / height > 200)
            break;
    }

    DeleteDC(hdc);

    ratio = width / height;
    scale = width / ave_width;

    trace("max width/height ratio (%d / %d) %d, max width scale (%d / %d) %d\n",
          width, height, ratio, width, ave_width, scale);

    ok(ratio >= 90 && ratio <= 110, "expected width/height ratio 90-110, got %d\n", ratio);
}

static void test_CreateFontIndirect(void)
{
    LOGFONTA lf, getobj_lf;
    int ret, i;
    HFONT hfont;
    char TestName[][16] = {"Arial", "Arial Bold", "Arial Italic", "Arial Baltic"};

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfItalic = FALSE;
    lf.lfWeight = FW_DONTCARE;

    for (i = 0; i < sizeof(TestName)/sizeof(TestName[0]); i++)
    {
        lstrcpyA(lf.lfFaceName, TestName[i]);
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != 0, "CreateFontIndirectA failed\n");
        SetLastError(0xdeadbeef);
        ret = GetObjectA(hfont, sizeof(getobj_lf), &getobj_lf);
        ok(ret, "GetObject failed: %d\n", GetLastError());
        ok(lf.lfItalic == getobj_lf.lfItalic, "lfItalic: expect %02x got %02x\n", lf.lfItalic, getobj_lf.lfItalic);
        ok(lf.lfWeight == getobj_lf.lfWeight ||
           broken((SHORT)lf.lfWeight == getobj_lf.lfWeight), /* win9x */
           "lfWeight: expect %08x got %08x\n", lf.lfWeight, getobj_lf.lfWeight);
        ok(!lstrcmpA(lf.lfFaceName, getobj_lf.lfFaceName) ||
           broken(!memcmp(lf.lfFaceName, getobj_lf.lfFaceName, LF_FACESIZE-1)), /* win9x doesn't ensure '\0' termination */
           "font names don't match: %s != %s\n", lf.lfFaceName, getobj_lf.lfFaceName);
        DeleteObject(hfont);
    }
}

static void test_CreateFontIndirectEx(void)
{
    ENUMLOGFONTEXDVA lfex;
    HFONT hfont;

    if (!pCreateFontIndirectExA)
    {
        win_skip("CreateFontIndirectExA is not available\n");
        return;
    }

    if (!is_truetype_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    SetLastError(0xdeadbeef);
    hfont = pCreateFontIndirectExA(NULL);
    ok(hfont == NULL, "got %p\n", hfont);
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

    memset(&lfex, 0, sizeof(lfex));
    lstrcpyA(lfex.elfEnumLogfontEx.elfLogFont.lfFaceName, "Arial");
    hfont = pCreateFontIndirectExA(&lfex);
    ok(hfont != 0, "CreateFontIndirectEx failed\n");
    if (hfont)
        check_font("Arial", &lfex.elfEnumLogfontEx.elfLogFont, hfont);
    DeleteObject(hfont);
}

static void free_font(void *font)
{
    UnmapViewOfFile(font);
}

static void *load_font(const char *font_name, DWORD *font_size)
{
    char file_name[MAX_PATH];
    HANDLE file, mapping;
    void *font;

    if (!GetWindowsDirectoryA(file_name, sizeof(file_name))) return NULL;
    strcat(file_name, "\\fonts\\");
    strcat(file_name, font_name);

    file = CreateFileA(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return NULL;

    *font_size = GetFileSize(file, NULL);

    mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mapping)
    {
        CloseHandle(file);
        return NULL;
    }

    font = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(file);
    CloseHandle(mapping);
    return font;
}

static void test_AddFontMemResource(void)
{
    void *font;
    DWORD font_size, num_fonts;
    HANDLE ret;
    BOOL bRet;

    if (!pAddFontMemResourceEx || !pRemoveFontMemResourceEx)
    {
        win_skip("AddFontMemResourceEx is not available on this platform\n");
        return;
    }

    font = load_font("sserife.fon", &font_size);
    if (!font)
    {
        skip("Unable to locate and load font sserife.fon\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(NULL, 0, NULL, NULL);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(NULL, 10, NULL, NULL);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(NULL, 0, NULL, &num_fonts);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(NULL, 10, NULL, &num_fonts);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, 0, NULL, NULL);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, 10, NULL, NULL);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    num_fonts = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, 0, NULL, &num_fonts);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());
    ok(num_fonts == 0xdeadbeef, "number of loaded fonts should be 0xdeadbeef\n");

    if (0) /* hangs under windows 2000 */
    {
        num_fonts = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = pAddFontMemResourceEx(font, 10, NULL, &num_fonts);
        ok(!ret, "AddFontMemResourceEx should fail\n");
        ok(GetLastError() == 0xdeadbeef,
           "Expected GetLastError() to return 0xdeadbeef, got %u\n",
           GetLastError());
        ok(num_fonts == 0xdeadbeef, "number of loaded fonts should be 0xdeadbeef\n");
    }

    num_fonts = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, font_size, NULL, &num_fonts);
    ok(ret != 0, "AddFontMemResourceEx error %d\n", GetLastError());
    ok(num_fonts != 0xdeadbeef, "number of loaded fonts should not be 0xdeadbeef\n");
    ok(num_fonts != 0, "number of loaded fonts should not be 0\n");

    free_font(font);

    SetLastError(0xdeadbeef);
    bRet = pRemoveFontMemResourceEx(ret);
    ok(bRet, "RemoveFontMemResourceEx error %d\n", GetLastError());

    /* test invalid pointer to number of loaded fonts */
    font = load_font("sserife.fon", &font_size);
    ok(font != NULL, "Unable to locate and load font sserife.fon\n");

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, font_size, NULL, (void *)0xdeadbeef);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected GetLastError() to return 0xdeadbeef, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pAddFontMemResourceEx(font, font_size, NULL, NULL);
    ok(!ret, "AddFontMemResourceEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    free_font(font);
}

static INT CALLBACK enum_fonts_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lparam)
{
    LOGFONTA *lf;

    if (type != TRUETYPE_FONTTYPE) return 1;

    ok(ntm->tmWeight == elf->lfWeight, "expected %d got %d\n", ntm->tmWeight, elf->lfWeight);

    lf = (LOGFONTA *)lparam;
    *lf = *elf;
    return 0;
}

static INT CALLBACK enum_all_fonts_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lparam)
{
    int ret;
    LOGFONTA *lf;

    if (type != TRUETYPE_FONTTYPE) return 1;

    lf = (LOGFONTA *)lparam;
    ret = strcmp(lf->lfFaceName, elf->lfFaceName);
    if(ret == 0)
    {
        ok(ntm->tmWeight == elf->lfWeight, "expected %d got %d\n", ntm->tmWeight, elf->lfWeight);
        *lf = *elf;
        return 0;
    }
    return 1;
}

static INT CALLBACK enum_with_magic_retval_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lparam)
{
    return lparam;
}

static void test_EnumFonts(void)
{
    int ret;
    LOGFONTA lf;
    HDC hdc;
    struct enum_fullname_data efnd;

    if (!is_truetype_font_installed("Arial"))
    {
        skip("Arial is not installed\n");
        return;
    }

    /* Windows uses localized font face names, so Arial Bold won't be found */
    if (PRIMARYLANGID(GetUserDefaultLangID()) != LANG_ENGLISH)
    {
        skip("User locale is not English, skipping the test\n");
        return;
    }

    hdc = CreateCompatibleDC(0);

    /* check that the enumproc's retval is returned */
    ret = EnumFontFamiliesA(hdc, NULL, enum_with_magic_retval_proc, 0xcafe);
    ok(ret == 0xcafe, "got %08x\n", ret);

    ret = EnumFontFamiliesA(hdc, "Arial", enum_fonts_proc, (LPARAM)&lf);
    ok(!ret, "font Arial is not enumerated\n");
    ret = strcmp(lf.lfFaceName, "Arial");
    ok(!ret, "expected Arial got %s\n", lf.lfFaceName);
    ok(lf.lfWeight == FW_NORMAL, "expected FW_NORMAL got %d\n", lf.lfWeight);

    strcpy(lf.lfFaceName, "Arial");
    ret = EnumFontFamiliesA(hdc, NULL, enum_all_fonts_proc, (LPARAM)&lf);
    ok(!ret, "font Arial is not enumerated\n");
    ret = strcmp(lf.lfFaceName, "Arial");
    ok(!ret, "expected Arial got %s\n", lf.lfFaceName);
    ok(lf.lfWeight == FW_NORMAL, "expected FW_NORMAL got %d\n", lf.lfWeight);

    ret = EnumFontFamiliesA(hdc, "Arial Bold", enum_fonts_proc, (LPARAM)&lf);
    ok(!ret, "font Arial Bold is not enumerated\n");
    ret = strcmp(lf.lfFaceName, "Arial");
    ok(!ret, "expected Arial got %s\n", lf.lfFaceName);
    ok(lf.lfWeight == FW_BOLD, "expected FW_BOLD got %d\n", lf.lfWeight);

    strcpy(lf.lfFaceName, "Arial Bold");
    ret = EnumFontFamiliesA(hdc, NULL, enum_all_fonts_proc, (LPARAM)&lf);
    ok(ret, "font Arial Bold should not be enumerated\n");

    ret = EnumFontFamiliesA(hdc, "Arial Bold Italic", enum_fonts_proc, (LPARAM)&lf);
    ok(!ret, "font Arial Bold Italic is not enumerated\n");
    ret = strcmp(lf.lfFaceName, "Arial");
    ok(!ret, "expected Arial got %s\n", lf.lfFaceName);
    ok(lf.lfWeight == FW_BOLD, "expected FW_BOLD got %d\n", lf.lfWeight);

    strcpy(lf.lfFaceName, "Arial Bold Italic");
    ret = EnumFontFamiliesA(hdc, NULL, enum_all_fonts_proc, (LPARAM)&lf);
    ok(ret, "font Arial Bold Italic should not be enumerated\n");

    ret = EnumFontFamiliesA(hdc, "Arial Italic Bold", enum_fonts_proc, (LPARAM)&lf);
    ok(ret, "font Arial Italic Bold  should not be enumerated\n");

    strcpy(lf.lfFaceName, "Arial Italic Bold");
    ret = EnumFontFamiliesA(hdc, NULL, enum_all_fonts_proc, (LPARAM)&lf);
    ok(ret, "font Arial Italic Bold should not be enumerated\n");

    /* MS Shell Dlg and MS Shell Dlg 2 must exist */
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;

    memset(&efnd, 0, sizeof(efnd));
    strcpy(lf.lfFaceName, "MS Shell Dlg");
    ret = EnumFontFamiliesExA(hdc, &lf, enum_fullname_data_proc, (LPARAM)&efnd, 0);
    ok(ret, "font MS Shell Dlg is not enumerated\n");
    ret = strcmp((char*)efnd.elf[0].elfLogFont.lfFaceName, "MS Shell Dlg");
    todo_wine ok(!ret, "expected MS Shell Dlg got %s\n", efnd.elf[0].elfLogFont.lfFaceName);
    ret = strcmp((char*)efnd.elf[0].elfFullName, "MS Shell Dlg");
    ok(ret, "did not expect MS Shell Dlg\n");

    memset(&efnd, 0, sizeof(efnd));
    strcpy(lf.lfFaceName, "MS Shell Dlg 2");
    ret = EnumFontFamiliesExA(hdc, &lf, enum_fullname_data_proc, (LPARAM)&efnd, 0);
    ok(ret, "font MS Shell Dlg 2 is not enumerated\n");
    ret = strcmp((char*)efnd.elf[0].elfLogFont.lfFaceName, "MS Shell Dlg 2");
    todo_wine ok(!ret, "expected MS Shell Dlg 2 got %s\n", efnd.elf[0].elfLogFont.lfFaceName);
    ret = strcmp((char*)efnd.elf[0].elfFullName, "MS Shell Dlg 2");
    ok(ret, "did not expect MS Shell Dlg 2\n");

    DeleteDC(hdc);
}

static INT CALLBACK is_font_installed_fullname_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    const ENUMLOGFONTA *elf = (const ENUMLOGFONTA *)lf;
    const char *fullname = (const char *)lParam;

    if (!strcmp((const char *)elf->elfFullName, fullname)) return 0;

    return 1;
}

static BOOL is_font_installed_fullname(const char *family, const char *fullname)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if(!EnumFontFamiliesA(hdc, family, is_font_installed_fullname_proc, (LPARAM)fullname))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

static void test_fullname(void)
{
    static const char *TestName[] = {"Lucida Sans Demibold Roman", "Lucida Sans Italic", "Lucida Sans Regular"};
    WCHAR bufW[LF_FULLFACESIZE];
    char bufA[LF_FULLFACESIZE];
    HFONT hfont, of;
    LOGFONTA lf;
    HDC hdc;
    int i;
    DWORD ret;

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfItalic = FALSE;
    lf.lfWeight = FW_DONTCARE;

    for (i = 0; i < sizeof(TestName) / sizeof(TestName[0]); i++)
    {
        if (!is_font_installed_fullname("Lucida Sans", TestName[i]))
        {
            skip("%s is not installed\n", TestName[i]);
            continue;
        }

        lstrcpyA(lf.lfFaceName, TestName[i]);
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != 0, "CreateFontIndirectA failed\n");

        of = SelectObject(hdc, hfont);
        bufW[0] = 0;
        bufA[0] = 0;
        ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FULL_NAME, bufW, sizeof(bufW), TT_MS_LANGID_ENGLISH_UNITED_STATES);
        ok(ret, "face full name could not be read\n");
        WideCharToMultiByte(CP_ACP, 0, bufW, -1, bufA, sizeof(bufA), NULL, FALSE);
        ok(!lstrcmpA(bufA, TestName[i]), "font full names don't match: %s != %s\n", TestName[i], bufA);
        SelectObject(hdc, of);
        DeleteObject(hfont);
    }
    DeleteDC(hdc);
}

static WCHAR *prepend_at(WCHAR *family)
{
    if (!family)
        return NULL;

    memmove(family + 1, family, (lstrlenW(family) + 1) * sizeof(WCHAR));
    family[0] = '@';
    return family;
}

static void test_fullname2_helper(const char *Family)
{
    char *FamilyName, *FaceName, *StyleName, *otmStr;
    struct enum_fullname_data efnd;
    WCHAR *bufW;
    char *bufA;
    HFONT hfont, of;
    LOGFONTA lf;
    HDC hdc;
    int i;
    DWORD otm_size, ret, buf_size;
    OUTLINETEXTMETRICA *otm;
    BOOL want_vertical, get_vertical;
    want_vertical = ( Family[0] == '@' );

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfItalic = FALSE;
    lf.lfWeight = FW_DONTCARE;
    strcpy(lf.lfFaceName, Family);
    efnd.total = 0;
    EnumFontFamiliesExA(hdc, &lf, enum_fullname_data_proc, (LPARAM)&efnd, 0);
    if (efnd.total == 0)
        skip("%s is not installed\n", lf.lfFaceName);

    for (i = 0; i < efnd.total; i++)
    {
        FamilyName = (char *)efnd.elf[i].elfLogFont.lfFaceName;
        FaceName = (char *)efnd.elf[i].elfFullName;
        StyleName = (char *)efnd.elf[i].elfStyle;

        get_vertical = ( FamilyName[0] == '@' );
        ok(get_vertical == want_vertical, "Vertical flags don't match: %s %s\n", Family, FamilyName);

        lstrcpyA(lf.lfFaceName, FaceName);
        hfont = CreateFontIndirectA(&lf);
        ok(hfont != 0, "CreateFontIndirectA failed\n");

        of = SelectObject(hdc, hfont);
        buf_size = GetFontData(hdc, MS_NAME_TAG, 0, NULL, 0);
        ok(buf_size != GDI_ERROR, "no name table found\n");
        if (buf_size == GDI_ERROR) continue;

        bufW = HeapAlloc(GetProcessHeap(), 0, buf_size);
        bufA = HeapAlloc(GetProcessHeap(), 0, buf_size);

        otm_size = GetOutlineTextMetricsA(hdc, 0, NULL);
        otm = HeapAlloc(GetProcessHeap(), 0, otm_size);
        memset(otm, 0, otm_size);
        ret = GetOutlineTextMetricsA(hdc, otm_size, otm);
        ok(ret != 0, "GetOutlineTextMetrics fails!\n");
        if (ret == 0) continue;

        bufW[0] = 0;
        bufA[0] = 0;
        ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FONT_FAMILY, bufW, buf_size, GetSystemDefaultLangID());
        if (!ret) ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FONT_FAMILY, bufW, buf_size, TT_MS_LANGID_ENGLISH_UNITED_STATES);
        ok(ret, "%s: FAMILY (family name) could not be read\n", FamilyName);
        if (want_vertical) bufW = prepend_at(bufW);
        WideCharToMultiByte(CP_ACP, 0, bufW, -1, bufA, buf_size, NULL, FALSE);
        ok(!lstrcmpA(FamilyName, bufA), "font family names don't match: returned %s, expect %s\n", FamilyName, bufA);
        otmStr = (LPSTR)otm + (UINT_PTR)otm->otmpFamilyName;
        ok(!lstrcmpA(FamilyName, otmStr), "FamilyName %s doesn't match otmpFamilyName %s\n", FamilyName, otmStr);

        bufW[0] = 0;
        bufA[0] = 0;
        ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FULL_NAME, bufW, buf_size, GetSystemDefaultLangID());
        if (!ret) ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FULL_NAME, bufW, buf_size, TT_MS_LANGID_ENGLISH_UNITED_STATES);
        ok(ret, "FULL_NAME (face name) could not be read\n");
        if (want_vertical) bufW = prepend_at(bufW);
        WideCharToMultiByte(CP_ACP, 0, bufW, -1, bufA, buf_size, NULL, FALSE);
        ok(!lstrcmpA(FaceName, bufA), "%s: font face names don't match: returned %s, expect %s\n", FamilyName, FaceName, bufA);
        otmStr = (LPSTR)otm + (UINT_PTR)otm->otmpFaceName;
        ok(!lstrcmpA(FaceName, otmStr), "%s: FaceName %s doesn't match otmpFaceName %s\n", FamilyName, FaceName, otmStr);

        bufW[0] = 0;
        bufA[0] = 0;
        ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FONT_SUBFAMILY, bufW, buf_size, GetSystemDefaultLangID());
        if (!ret) ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_FONT_SUBFAMILY, bufW, buf_size, TT_MS_LANGID_ENGLISH_UNITED_STATES);
        ok(ret, "%s: SUBFAMILY (style name) could not be read\n", FamilyName);
        WideCharToMultiByte(CP_ACP, 0, bufW, -1, bufA, buf_size, NULL, FALSE);
        ok(!lstrcmpA(StyleName, bufA), "%s: style names don't match: returned %s, expect %s\n", FamilyName, StyleName, bufA);
        otmStr = (LPSTR)otm + (UINT_PTR)otm->otmpStyleName;
        ok(!lstrcmpA(StyleName, otmStr), "%s: StyleName %s doesn't match otmpStyleName %s\n", FamilyName, StyleName, otmStr);

        bufW[0] = 0;
        bufA[0] = 0;
        ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_UNIQUE_ID, bufW, buf_size, GetSystemDefaultLangID());
        if (!ret) ret = get_ttf_nametable_entry(hdc, TT_NAME_ID_UNIQUE_ID, bufW, buf_size, TT_MS_LANGID_ENGLISH_UNITED_STATES);
        ok(ret, "%s: UNIQUE_ID (full name) could not be read\n", FamilyName);
        WideCharToMultiByte(CP_ACP, 0, bufW, -1, bufA, buf_size, NULL, FALSE);
        otmStr = (LPSTR)otm + (UINT_PTR)otm->otmpFullName;
        ok(!lstrcmpA(otmStr, bufA), "%s: UNIQUE ID (full name) doesn't match: returned %s, expect %s\n", FamilyName, otmStr, bufA);

        SelectObject(hdc, of);
        DeleteObject(hfont);

        HeapFree(GetProcessHeap(), 0, otm);
        HeapFree(GetProcessHeap(), 0, bufW);
        HeapFree(GetProcessHeap(), 0, bufA);
    }
    DeleteDC(hdc);
}

static void test_fullname2(void)
{
    test_fullname2_helper("Arial");
    test_fullname2_helper("DejaVu Sans");
    test_fullname2_helper("Lucida Sans");
    test_fullname2_helper("Tahoma");
    test_fullname2_helper("Webdings");
    test_fullname2_helper("Wingdings");
    test_fullname2_helper("SimSun");
    test_fullname2_helper("NSimSun");
    test_fullname2_helper("MingLiu");
    test_fullname2_helper("PMingLiu");
    test_fullname2_helper("WenQuanYi Micro Hei");
    test_fullname2_helper("MS UI Gothic");
    test_fullname2_helper("Ume UI Gothic");
    test_fullname2_helper("MS Gothic");
    test_fullname2_helper("Ume Gothic");
    test_fullname2_helper("MS PGothic");
    test_fullname2_helper("Ume P Gothic");
    test_fullname2_helper("Gulim");
    test_fullname2_helper("Batang");
    test_fullname2_helper("UnBatang");
    test_fullname2_helper("UnDotum");
    test_fullname2_helper("@SimSun");
    test_fullname2_helper("@NSimSun");
    test_fullname2_helper("@MingLiu");
    test_fullname2_helper("@PMingLiu");
    test_fullname2_helper("@WenQuanYi Micro Hei");
    test_fullname2_helper("@MS UI Gothic");
    test_fullname2_helper("@Ume UI Gothic");
    test_fullname2_helper("@MS Gothic");
    test_fullname2_helper("@Ume Gothic");
    test_fullname2_helper("@MS PGothic");
    test_fullname2_helper("@Ume P Gothic");
    test_fullname2_helper("@Gulim");
    test_fullname2_helper("@Batang");
    test_fullname2_helper("@UnBatang");
    test_fullname2_helper("@UnDotum");

}

static void test_GetGlyphOutline_empty_contour(void)
{
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, hfont_prev;
    TTPOLYGONHEADER *header;
    GLYPHMETRICS gm;
    char buf[1024];
    DWORD ret;

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 72;
    lstrcpyA(lf.lfFaceName, "wine_test");

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());

    hdc = GetDC(NULL);

    hfont_prev = SelectObject(hdc, hfont);
    ok(hfont_prev != NULL, "SelectObject failed\n");

    ret = GetGlyphOutlineW(hdc, 0xa8, GGO_NATIVE, &gm, 0, NULL, &mat);
    ok(ret == 228, "GetGlyphOutline returned %d, expected 228\n", ret);

    header = (TTPOLYGONHEADER*)buf;
    ret = GetGlyphOutlineW(hdc, 0xa8, GGO_NATIVE, &gm, sizeof(buf), buf, &mat);
    ok(ret == 228, "GetGlyphOutline returned %d, expected 228\n", ret);
    ok(header->cb == 36, "header->cb = %d, expected 36\n", header->cb);
    ok(header->dwType == TT_POLYGON_TYPE, "header->dwType = %d, expected TT_POLYGON_TYPE\n", header->dwType);
    header = (TTPOLYGONHEADER*)((char*)header+header->cb);
    ok(header->cb == 96, "header->cb = %d, expected 96\n", header->cb);
    header = (TTPOLYGONHEADER*)((char*)header+header->cb);
    ok(header->cb == 96, "header->cb = %d, expected 96\n", header->cb);

    SelectObject(hdc, hfont_prev);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void test_GetGlyphOutline_metric_clipping(void)
{
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, hfont_prev;
    GLYPHMETRICS gm;
    TEXTMETRICA tm;
    TEXTMETRICW tmW;
    DWORD ret;

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 72;
    lstrcpyA(lf.lfFaceName, "wine_test");

    SetLastError(0xdeadbeef);
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA error %u\n", GetLastError());

    hdc = GetDC(NULL);

    hfont_prev = SelectObject(hdc, hfont);
    ok(hfont_prev != NULL, "SelectObject failed\n");

    SetLastError(0xdeadbeef);
    ret = GetTextMetricsA(hdc, &tm);
    ok(ret, "GetTextMetrics error %u\n", GetLastError());

    GetGlyphOutlineA(hdc, 'A', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(gm.gmptGlyphOrigin.y <= tm.tmAscent,
        "Glyph top(%d) exceeds ascent(%d)\n",
        gm.gmptGlyphOrigin.y, tm.tmAscent);
    GetGlyphOutlineA(hdc, 'D', GGO_METRICS, &gm, 0, NULL, &mat);
    ok(gm.gmptGlyphOrigin.y - gm.gmBlackBoxY >= -tm.tmDescent,
        "Glyph bottom(%d) exceeds descent(%d)\n",
        gm.gmptGlyphOrigin.y - gm.gmBlackBoxY, -tm.tmDescent);

    /* Test tmLastChar - wine_test has code points fffb-fffe mapped to glyph 0 */
    GetTextMetricsW(hdc, &tmW);
todo_wine
    ok( tmW.tmLastChar == 0xfffe, "got %04x\n", tmW.tmLastChar);

    SelectObject(hdc, hfont_prev);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void test_CreateScalableFontResource(void)
{
    char ttf_name[MAX_PATH];
    char tmp_path[MAX_PATH];
    char fot_name[MAX_PATH];
    char *file_part;
    DWORD ret;
    int i;

    if (!pAddFontResourceExA || !pRemoveFontResourceExA)
    {
        win_skip("AddFontResourceExA is not available on this platform\n");
        return;
    }

    if (!write_ttf_file("wine_test.ttf", ttf_name))
    {
        skip("Failed to create ttf file for testing\n");
        return;
    }

    trace("created %s\n", ttf_name);

    ret = is_truetype_font_installed("wine_test");
    ok(!ret, "font wine_test should not be enumerated\n");

    ret = GetTempPathA(MAX_PATH, tmp_path);
    ok(ret, "GetTempPath() error %d\n", GetLastError());
    ret = GetTempFileNameA(tmp_path, "fot", 0, fot_name);
    ok(ret, "GetTempFileName() error %d\n", GetLastError());

    ret = GetFileAttributesA(fot_name);
    ok(ret != INVALID_FILE_ATTRIBUTES, "file %s does not exist\n", fot_name);

    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, ttf_name, NULL);
    ok(!ret, "CreateScalableFontResource() should fail\n");
    ok(GetLastError() == ERROR_FILE_EXISTS, "not expected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, ttf_name, "");
    ok(!ret, "CreateScalableFontResource() should fail\n");
    ok(GetLastError() == ERROR_FILE_EXISTS, "not expected error %d\n", GetLastError());

    file_part = strrchr(ttf_name, '\\');
    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, file_part, tmp_path);
    ok(!ret, "CreateScalableFontResource() should fail\n");
    ok(GetLastError() == ERROR_FILE_EXISTS, "not expected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, "random file name", tmp_path);
    ok(!ret, "CreateScalableFontResource() should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "not expected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, NULL, ttf_name);
    ok(!ret, "CreateScalableFontResource() should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "not expected error %d\n", GetLastError());

    ret = DeleteFileA(fot_name);
    ok(ret, "DeleteFile() error %d\n", GetLastError());

    ret = pRemoveFontResourceExA(fot_name, 0, 0);
    ok(!ret, "RemoveFontResourceEx() should fail\n");

    /* test public font resource */
    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(0, fot_name, ttf_name, NULL);
    ok(ret, "CreateScalableFontResource() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    ok(!ret, "font wine_test should not be enumerated\n");

    SetLastError(0xdeadbeef);
    ret = pAddFontResourceExA(fot_name, 0, 0);
    ok(ret, "AddFontResourceEx() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    ok(ret, "font wine_test should be enumerated\n");

    test_GetGlyphOutline_empty_contour();
    test_GetGlyphOutline_metric_clipping();

    ret = pRemoveFontResourceExA(fot_name, FR_PRIVATE, 0);
    ok(!ret, "RemoveFontResourceEx() with not matching flags should fail\n");

    SetLastError(0xdeadbeef);
    ret = pRemoveFontResourceExA(fot_name, 0, 0);
    ok(ret, "RemoveFontResourceEx() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    ok(!ret, "font wine_test should not be enumerated\n");

    ret = pRemoveFontResourceExA(fot_name, 0, 0);
    ok(!ret, "RemoveFontResourceEx() should fail\n");

    /* test refcounting */
    for (i = 0; i < 5; i++)
    {
        SetLastError(0xdeadbeef);
        ret = pAddFontResourceExA(fot_name, 0, 0);
        ok(ret, "AddFontResourceEx() error %d\n", GetLastError());
    }
    for (i = 0; i < 5; i++)
    {
        SetLastError(0xdeadbeef);
        ret = pRemoveFontResourceExA(fot_name, 0, 0);
        ok(ret, "RemoveFontResourceEx() error %d\n", GetLastError());
    }
    ret = pRemoveFontResourceExA(fot_name, 0, 0);
    ok(!ret, "RemoveFontResourceEx() should fail\n");

    DeleteFileA(fot_name);

    /* test hidden font resource */
    SetLastError(0xdeadbeef);
    ret = CreateScalableFontResourceA(1, fot_name, ttf_name, NULL);
    ok(ret, "CreateScalableFontResource() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    ok(!ret, "font wine_test should not be enumerated\n");

    SetLastError(0xdeadbeef);
    ret = pAddFontResourceExA(fot_name, 0, 0);
    ok(ret, "AddFontResourceEx() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    todo_wine
    ok(!ret, "font wine_test should not be enumerated\n");

    /* XP allows removing a private font added with 0 flags */
    SetLastError(0xdeadbeef);
    ret = pRemoveFontResourceExA(fot_name, FR_PRIVATE, 0);
    ok(ret, "RemoveFontResourceEx() error %d\n", GetLastError());

    ret = is_truetype_font_installed("wine_test");
    ok(!ret, "font wine_test should not be enumerated\n");

    ret = pRemoveFontResourceExA(fot_name, 0, 0);
    ok(!ret, "RemoveFontResourceEx() should fail\n");

    DeleteFileA(fot_name);
    DeleteFileA(ttf_name);
}

static void check_vertical_font(const char *name, BOOL *installed, BOOL *selected, GLYPHMETRICS *gm, WORD *gi)
{
    LOGFONTA lf;
    HFONT hfont, hfont_prev;
    HDC hdc;
    char facename[100];
    DWORD ret;
    static const WCHAR str[] = { 0x2025 };

    *installed = is_truetype_font_installed(name);

    lf.lfHeight = -18;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_DONTCARE;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy(lf.lfFaceName, name);

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != NULL, "CreateFontIndirectA failed\n");

    hdc = GetDC(NULL);

    hfont_prev = SelectObject(hdc, hfont);
    ok(hfont_prev != NULL, "SelectObject failed\n");

    ret = GetTextFaceA(hdc, sizeof facename, facename);
    ok(ret, "GetTextFaceA failed\n");
    *selected = !strcmp(facename, name);

    ret = GetGlyphOutlineW(hdc, 0x2025, GGO_METRICS, gm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineW failed\n");
    if (!*selected)
        memset(gm, 0, sizeof *gm);

    ret = pGetGlyphIndicesW(hdc, str, 1, gi, 0);
    ok(ret != GDI_ERROR, "GetGlyphIndicesW failed\n");

    SelectObject(hdc, hfont_prev);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

static void check_vertical_metrics(const char *face)
{
    LOGFONTA lf;
    HFONT hfont, hfont_prev;
    HDC hdc;
    DWORD ret;
    GLYPHMETRICS rgm, vgm;
    const UINT code = 0x5EAD, height = 1000;
    WORD idx;
    ABC abc;
    OUTLINETEXTMETRICA otm;
    USHORT numOfLongVerMetrics;

    hdc = GetDC(NULL);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, face);
    lf.lfHeight = -height;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfEscapement = lf.lfOrientation = 900;
    hfont = CreateFontIndirectA(&lf);
    hfont_prev = SelectObject(hdc, hfont);
    ret = GetGlyphOutlineW(hdc, code, GGO_METRICS, &rgm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineW failed\n");
    ret = GetCharABCWidthsW(hdc, code, code, &abc);
    ok(ret, "GetCharABCWidthsW failed\n");
    DeleteObject(SelectObject(hdc, hfont_prev));

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "@");
    strcat(lf.lfFaceName, face);
    lf.lfHeight = -height;
    lf.lfCharSet = DEFAULT_CHARSET;
    hfont = CreateFontIndirectA(&lf);
    hfont_prev = SelectObject(hdc, hfont);
    ret = GetGlyphOutlineW(hdc, code, GGO_METRICS, &vgm, 0, NULL, &mat);
    ok(ret != GDI_ERROR, "GetGlyphOutlineW failed\n");

    memset(&otm, 0, sizeof(otm));
    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsA(hdc, sizeof(otm), &otm);
    ok(ret != 0, "GetOutlineTextMetricsA failed\n");

    if (GetFontData(hdc, MS_MAKE_TAG('v','h','e','a'), sizeof(SHORT) * 17,
                    &numOfLongVerMetrics, sizeof(numOfLongVerMetrics)) != GDI_ERROR) {
        int offset;
        SHORT topSideBearing;

        if (!pGetGlyphIndicesW) {
            win_skip("GetGlyphIndices is not available on this platform\n");
        }
        else {
            ret = pGetGlyphIndicesW(hdc, (LPCWSTR)&code, 1, &idx, 0);
            ok(ret != 0, "GetGlyphIndicesW failed\n");
            numOfLongVerMetrics = GET_BE_WORD(numOfLongVerMetrics);
            if (numOfLongVerMetrics > idx)
                offset = idx * 2 + 1;
            else
                offset = numOfLongVerMetrics * 2 + (idx - numOfLongVerMetrics);
            ret = GetFontData(hdc, MS_MAKE_TAG('v','m','t','x'), offset * sizeof(SHORT),
                              &topSideBearing, sizeof(SHORT));
            ok(ret != GDI_ERROR, "GetFontData(vmtx) failed\n");
            topSideBearing = GET_BE_WORD(topSideBearing);
            ok(match_off_by_1(vgm.gmptGlyphOrigin.x,
                              MulDiv(topSideBearing, height, otm.otmEMSquare), FALSE),
               "expected %d, got %d\n",
               MulDiv(topSideBearing, height, otm.otmEMSquare), vgm.gmptGlyphOrigin.x);
        }
    }
    else
    {
        ok(vgm.gmptGlyphOrigin.x == rgm.gmptGlyphOrigin.x + vgm.gmCellIncX + otm.otmDescent,
           "got %d, expected rgm.origin.x(%d) + vgm.cellIncX(%d) + descent(%d)\n",
           vgm.gmptGlyphOrigin.x, rgm.gmptGlyphOrigin.x, vgm.gmCellIncX, otm.otmDescent);
    }

    ok(vgm.gmptGlyphOrigin.y == abc.abcA + abc.abcB + otm.otmDescent ||
       broken(vgm.gmptGlyphOrigin.y == abc.abcA + abc.abcB - otm.otmTextMetrics.tmDescent) /* win2k */,
       "got %d, expected abcA(%d) + abcB(%u) + descent(%d)\n",
       (INT)vgm.gmptGlyphOrigin.y, abc.abcA, abc.abcB, otm.otmDescent);

    DeleteObject(SelectObject(hdc, hfont_prev));
    ReleaseDC(NULL, hdc);
}

static void test_vertical_font(void)
{
    char ttf_name[MAX_PATH];
    int num, i;
    BOOL ret, installed, selected;
    GLYPHMETRICS gm;
    WORD hgi, vgi;
    const char* face_list[] = {
        "@WineTestVertical", /* has vmtx table */
        "@Ume Gothic",       /* doesn't have vmtx table */
        "@MS UI Gothic",     /* has vmtx table, available on native */
    };

    if (!pAddFontResourceExA || !pRemoveFontResourceExA || !pGetGlyphIndicesW)
    {
        win_skip("AddFontResourceExA or GetGlyphIndicesW is not available on this platform\n");
        return;
    }

    if (!write_ttf_file("vertical.ttf", ttf_name))
    {
        skip("Failed to create ttf file for testing\n");
        return;
    }

    num = pAddFontResourceExA(ttf_name, FR_PRIVATE, 0);
    ok(num == 2, "AddFontResourceExA should add 2 fonts from vertical.ttf\n");

    check_vertical_font("WineTestVertical", &installed, &selected, &gm, &hgi);
    ok(installed, "WineTestVertical is not installed\n");
    ok(selected, "WineTestVertical is not selected\n");
    ok(gm.gmBlackBoxX > gm.gmBlackBoxY,
       "gmBlackBoxX(%u) should be greater than gmBlackBoxY(%u) if horizontal\n",
       gm.gmBlackBoxX, gm.gmBlackBoxY);

    check_vertical_font("@WineTestVertical", &installed, &selected, &gm, &vgi);
    ok(installed, "@WineTestVertical is not installed\n");
    ok(selected, "@WineTestVertical is not selected\n");
    ok(gm.gmBlackBoxX > gm.gmBlackBoxY,
       "gmBlackBoxX(%u) should be less than gmBlackBoxY(%u) if vertical\n",
       gm.gmBlackBoxX, gm.gmBlackBoxY);

    ok(hgi != vgi, "same glyph h:%u v:%u\n", hgi, vgi);

    for (i = 0; i < sizeof(face_list)/sizeof(face_list[0]); i++) {
        const char* face = face_list[i];
        if (!is_truetype_font_installed(face)) {
            skip("%s is not installed\n", face);
            continue;
        }
        trace("Testing %s...\n", face);
        check_vertical_metrics(&face[1]);
    }

    ret = pRemoveFontResourceExA(ttf_name, FR_PRIVATE, 0);
    ok(ret, "RemoveFontResourceEx() error %d\n", GetLastError());

    DeleteFileA(ttf_name);
}

static INT CALLBACK has_vertical_font_proc(const LOGFONTA *lf, const TEXTMETRICA *ntm,
                                           DWORD type, LPARAM lParam)
{
    if (lf->lfFaceName[0] == '@') {
        return 0;
    }
    return 1;
}

static void test_east_asian_font_selection(void)
{
    HDC hdc;
    UINT charset[] = { SHIFTJIS_CHARSET, HANGEUL_CHARSET, JOHAB_CHARSET,
                       GB2312_CHARSET, CHINESEBIG5_CHARSET };
    size_t i;

    hdc = GetDC(NULL);

    for (i = 0; i < sizeof(charset)/sizeof(charset[0]); i++)
    {
        LOGFONTA lf;
        HFONT hfont;
        char face_name[LF_FACESIZE];
        int ret;

        memset(&lf, 0, sizeof lf);
        lf.lfFaceName[0] = '\0';
        lf.lfCharSet = charset[i];

        if (EnumFontFamiliesExA(hdc, &lf, has_vertical_font_proc, 0, 0))
        {
            skip("Vertical font for charset %u is not installed\n", charset[i]);
            continue;
        }

        hfont = CreateFontIndirectA(&lf);
        hfont = SelectObject(hdc, hfont);
        memset(face_name, 0, sizeof face_name);
        ret = GetTextFaceA(hdc, sizeof face_name, face_name);
        ok(ret && face_name[0] != '@',
           "expected non-vertical face for charset %u, got %s\n", charset[i], face_name);
        DeleteObject(SelectObject(hdc, hfont));

        memset(&lf, 0, sizeof lf);
        strcpy(lf.lfFaceName, "@");
        lf.lfCharSet = charset[i];
        hfont = CreateFontIndirectA(&lf);
        hfont = SelectObject(hdc, hfont);
        memset(face_name, 0, sizeof face_name);
        ret = GetTextFaceA(hdc, sizeof face_name, face_name);
        ok(ret && face_name[0] == '@',
           "expected vertical face for charset %u, got %s\n", charset[i], face_name);
        DeleteObject(SelectObject(hdc, hfont));
    }
    ReleaseDC(NULL, hdc);
}

static int get_font_dpi(const LOGFONTA *lf, int *height)
{
    HDC hdc = CreateCompatibleDC(0);
    HFONT hfont;
    TEXTMETRICA tm;
    int ret;

    hfont = CreateFontIndirectA(lf);
    ok(hfont != 0, "CreateFontIndirect failed\n");

    SelectObject(hdc, hfont);
    ret = GetTextMetricsA(hdc, &tm);
    ok(ret, "GetTextMetrics failed\n");
    ret = tm.tmDigitizedAspectX;
    if (height) *height = tm.tmHeight;

    DeleteDC(hdc);
    DeleteObject(hfont);

    return ret;
}

static void test_stock_fonts(void)
{
    static const int font[] =
    {
        ANSI_FIXED_FONT, ANSI_VAR_FONT, SYSTEM_FONT, DEVICE_DEFAULT_FONT, DEFAULT_GUI_FONT
        /* SYSTEM_FIXED_FONT, OEM_FIXED_FONT */
    };
    static const struct test_data
    {
        int charset, weight, height, height_pixels, dpi;
        const char face_name[LF_FACESIZE];
    } td[][11] =
    {
        { /* ANSI_FIXED_FONT */
            { DEFAULT_CHARSET, FW_NORMAL, 12, 13, 96, "Courier" },
            { DEFAULT_CHARSET, FW_NORMAL, 12, 13, 120, "Courier" },
            { 0 }
        },
        { /* ANSI_VAR_FONT */
            { DEFAULT_CHARSET, FW_NORMAL, 12, 13, 96, "MS Sans Serif" },
            { DEFAULT_CHARSET, FW_NORMAL, 12, 13, 120, "MS Sans Serif" },
            { 0 }
        },
        { /* SYSTEM_FONT */
            { SHIFTJIS_CHARSET, FW_NORMAL, 18, 18, 96, "System" },
            { SHIFTJIS_CHARSET, FW_NORMAL, 22, 22, 120, "System" },
            { HANGEUL_CHARSET, FW_NORMAL, 16, 16, 96, "System" },
            { HANGEUL_CHARSET, FW_NORMAL, 20, 20, 120, "System" },
            { DEFAULT_CHARSET, FW_BOLD, 16, 16, 96, "System" },
            { DEFAULT_CHARSET, FW_BOLD, 20, 20, 120, "System" },
            { 0 }
        },
        { /* DEVICE_DEFAULT_FONT */
            { SHIFTJIS_CHARSET, FW_NORMAL, 18, 18, 96, "System" },
            { SHIFTJIS_CHARSET, FW_NORMAL, 22, 22, 120, "System" },
            { HANGEUL_CHARSET, FW_NORMAL, 16, 16, 96, "System" },
            { HANGEUL_CHARSET, FW_NORMAL, 20, 20, 120, "System" },
            { DEFAULT_CHARSET, FW_BOLD, 16, 16, 96, "System" },
            { DEFAULT_CHARSET, FW_BOLD, 20, 20, 120, "System" },
            { 0 }
        },
        { /* DEFAULT_GUI_FONT */
            { SHIFTJIS_CHARSET, FW_NORMAL, -12, 15, 96, "?MS UI Gothic" },
            { SHIFTJIS_CHARSET, FW_NORMAL, -15, 18, 120, "?MS UI Gothic" },
            { HANGEUL_CHARSET, FW_NORMAL, -12, 15, 96, "?Gulim" },
            { HANGEUL_CHARSET, FW_NORMAL, -15, 18, 120, "?Gulim" },
            { GB2312_CHARSET, FW_NORMAL, -12, 15, 96, "?SimHei" },
            { GB2312_CHARSET, FW_NORMAL, -15, 18, 120, "?SimHei" },
            { CHINESEBIG5_CHARSET, FW_NORMAL, -12, 15, 96, "?MingLiU" },
            { CHINESEBIG5_CHARSET, FW_NORMAL, -15, 18, 120, "?MingLiU" },
            { DEFAULT_CHARSET, FW_NORMAL, -11, 13, 96, "MS Shell Dlg" },
            { DEFAULT_CHARSET, FW_NORMAL, -13, 16, 120, "MS Shell Dlg" },
            { 0 }
        }
    };
    int i, j;

    for (i = 0; i < sizeof(font)/sizeof(font[0]); i++)
    {
        HFONT hfont;
        LOGFONTA lf;
        int ret, height;

        hfont = GetStockObject(font[i]);
        ok(hfont != 0, "%d: GetStockObject(%d) failed\n", i, font[i]);

        ret = GetObjectA(hfont, sizeof(lf), &lf);
        if (ret != sizeof(lf))
        {
            /* NT4 */
            win_skip("%d: GetObject returned %d instead of sizeof(LOGFONT)\n", i, ret);
            continue;
        }

        for (j = 0; td[i][j].face_name[0] != 0; j++)
        {
            if (lf.lfCharSet != td[i][j].charset && td[i][j].charset != DEFAULT_CHARSET)
            {
                continue;
            }

            ret = get_font_dpi(&lf, &height);
            if (ret != td[i][j].dpi)
            {
                trace("%d(%d): font %s %d dpi doesn't match test data %d\n",
                      i, j, lf.lfFaceName, ret, td[i][j].dpi);
                continue;
            }

            /* FIXME: Remove once Wine is fixed */
            if (td[i][j].dpi != 96 &&
                /* MS Sans Serif for 120 dpi and higher should include 12 pixel bitmap set */
                ((!strcmp(td[i][j].face_name, "MS Sans Serif") && td[i][j].height == 12) ||
                /* System for 120 dpi and higher should include 20 pixel bitmap set */
                (!strcmp(td[i][j].face_name, "System") && td[i][j].height > 16)))
            todo_wine
            ok(height == td[i][j].height_pixels, "%d(%d): expected height %d, got %d\n", i, j, td[i][j].height_pixels, height);
            else
            ok(height == td[i][j].height_pixels, "%d(%d): expected height %d, got %d\n", i, j, td[i][j].height_pixels, height);

            ok(td[i][j].weight == lf.lfWeight, "%d(%d): expected lfWeight %d, got %d\n", i, j, td[i][j].weight, lf.lfWeight);
            ok(td[i][j].height == lf.lfHeight, "%d(%d): expected lfHeight %d, got %d\n", i, j, td[i][j].height, lf.lfHeight);
            if (td[i][j].face_name[0] == '?')
            {
                /* Wine doesn't have this font, skip this case for now.
                   Actually, the face name is localized on Windows and varies
                   dpending on Windows versions (e.g. Japanese NT4 vs win2k). */
                trace("%d(%d): default gui font is %s\n", i, j, lf.lfFaceName);
            }
            else
            {
                ok(!strcmp(td[i][j].face_name, lf.lfFaceName), "%d(%d): expected lfFaceName %s, got %s\n", i, j, td[i][j].face_name, lf.lfFaceName);
            }
            break;
        }
    }
}

static void test_max_height(void)
{
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont, hfont_old;
    TEXTMETRICA tm1, tm;
    BOOL r;
    LONG invalid_height[] = { -65536, -123456, 123456 };
    size_t i;

    memset(&tm1, 0, sizeof(tm1));
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Tahoma");
    lf.lfHeight = -1;

    hdc = GetDC(NULL);

    /* get 1 ppem value */
    hfont = CreateFontIndirectA(&lf);
    hfont_old = SelectObject(hdc, hfont);
    r = GetTextMetricsA(hdc, &tm1);
    ok(r, "GetTextMetrics failed\n");
    ok(tm1.tmHeight > 0, "expected a positive value, got %d\n", tm1.tmHeight);
    ok(tm1.tmAveCharWidth > 0, "expected a positive value, got %d\n", tm1.tmHeight);
    DeleteObject(SelectObject(hdc, hfont_old));

    /* test the largest value */
    lf.lfHeight = -((1 << 16) - 1);
    hfont = CreateFontIndirectA(&lf);
    hfont_old = SelectObject(hdc, hfont);
    memset(&tm, 0, sizeof(tm));
    r = GetTextMetricsA(hdc, &tm);
    ok(r, "GetTextMetrics failed\n");
    ok(tm.tmHeight > tm1.tmHeight,
       "expected greater than 1 ppem value (%d), got %d\n", tm1.tmHeight, tm.tmHeight);
    ok(tm.tmAveCharWidth > tm1.tmAveCharWidth,
       "expected greater than 1 ppem value (%d), got %d\n", tm1.tmAveCharWidth, tm.tmAveCharWidth);
    DeleteObject(SelectObject(hdc, hfont_old));

    /* test an invalid value */
    for (i = 0; i < sizeof(invalid_height)/sizeof(invalid_height[0]); i++) {
        lf.lfHeight = invalid_height[i];
        hfont = CreateFontIndirectA(&lf);
        hfont_old = SelectObject(hdc, hfont);
        memset(&tm, 0, sizeof(tm));
        r = GetTextMetricsA(hdc, &tm);
        ok(r, "GetTextMetrics failed\n");
        ok(tm.tmHeight == tm1.tmHeight,
           "expected 1 ppem value (%d), got %d\n", tm1.tmHeight, tm.tmHeight);
        ok(tm.tmAveCharWidth == tm1.tmAveCharWidth,
           "expected 1 ppem value (%d), got %d\n", tm1.tmAveCharWidth, tm.tmAveCharWidth);
        DeleteObject(SelectObject(hdc, hfont_old));
    }

    ReleaseDC(NULL, hdc);
    return;
}

static void test_vertical_order(void)
{
    struct enum_font_data efd;
    LOGFONTA lf;
    HDC hdc;
    int i, j;

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");

    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfItalic = FALSE;
    lf.lfWeight = FW_DONTCARE;
    efd.total = 0;
    EnumFontFamiliesExA(hdc, &lf, enum_font_data_proc, (LPARAM)&efd, 0);
    for (i = 0; i < efd.total; i++)
    {
        if (efd.lf[i].lfFaceName[0] != '@') continue;
        for (j = 0; j < efd.total; j++)
        {
            if (!strcmp(efd.lf[i].lfFaceName + 1, efd.lf[j].lfFaceName))
            {
                ok(i > j,"Found vertical font %s before its horizontal version\n", efd.lf[i].lfFaceName);
                break;
            }
        }
    }
    DeleteDC( hdc );
}

static void test_GetCharWidth32(void)
{
    BOOL ret;
    HDC hdc;
    LOGFONTA lf;
    HFONT hfont;
    INT bufferA;
    INT bufferW;
    HWND hwnd;

    if (!pGetCharWidth32A || !pGetCharWidth32W)
    {
        win_skip("GetCharWidth32A/W not available on this platform\n");
        return;
    }

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "System");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hdc = GetDC(0);
    hfont = SelectObject(hdc, hfont);

    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ret = pGetCharWidth32A(hdc, 'a', 'a', &bufferA);
    ok(ret, "GetCharWidth32A should have succeeded\n");
    ok (bufferA == bufferW, "Widths should be the same\n");
    ok (bufferA > 0," Width should be greater than zero\n");

    hfont = SelectObject(hdc, hfont);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Tahoma");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    hdc = GetDC(hwnd);
    SetMapMode( hdc, MM_ANISOTROPIC );
    SelectObject(hdc, hfont);

    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetWindowExtEx(hdc, -1,-1,NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetGraphicsMode(hdc, GM_ADVANCED);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    todo_wine ok (bufferW > 0," Width should be greater than zero\n");
    SetWindowExtEx(hdc, 1,1,NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetGraphicsMode(hdc, GM_ADVANCED);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(WS_EX_LAYOUTRTL, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    hdc = GetDC(hwnd);
    SetMapMode( hdc, MM_ANISOTROPIC );
    SelectObject(hdc, hfont);

    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetWindowExtEx(hdc, -1,-1,NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetGraphicsMode(hdc, GM_ADVANCED);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetWindowExtEx(hdc, 1,1,NULL);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    ok (bufferW > 0," Width should be greater than zero\n");
    SetGraphicsMode(hdc, GM_ADVANCED);
    ret = pGetCharWidth32W(hdc, 'a', 'a', &bufferW);
    ok(ret, "GetCharWidth32W should have succeeded\n");
    todo_wine ok (bufferW > 0," Width should be greater than zero\n");

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    DeleteObject(hfont);
}

static void test_fake_bold_font(void)
{
    HDC hdc;
    HFONT hfont, hfont_old;
    LOGFONTA lf;
    BOOL ret;
    TEXTMETRICA tm[2];
    ABC abc[2];
    INT w[2];

    if (!pGetCharWidth32A || !pGetCharABCWidthsA) {
        win_skip("GetCharWidth32A/GetCharABCWidthA is not available on this platform\n");
        return;
    }

    /* Test outline font */
    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Wingdings");
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = SYMBOL_CHARSET;
    hfont = CreateFontIndirectA(&lf);

    hdc = GetDC(NULL);
    hfont_old = SelectObject(hdc, hfont);

    /* base metrics */
    ret = GetTextMetricsA(hdc, &tm[0]);
    ok(ret, "got %d\n", ret);
    ret = pGetCharABCWidthsA(hdc, 0x76, 0x76, &abc[0]);
    ok(ret, "got %d\n", ret);

    lf.lfWeight = FW_BOLD;
    hfont = CreateFontIndirectA(&lf);
    DeleteObject(SelectObject(hdc, hfont));

    /* bold metrics */
    ret = GetTextMetricsA(hdc, &tm[1]);
    ok(ret, "got %d\n", ret);
    ret = pGetCharABCWidthsA(hdc, 0x76, 0x76, &abc[1]);
    ok(ret, "got %d\n", ret);

    DeleteObject(SelectObject(hdc, hfont_old));
    ReleaseDC(NULL, hdc);

    /* compare results (outline) */
    ok(tm[0].tmHeight == tm[1].tmHeight, "expected %d, got %d\n", tm[0].tmHeight, tm[1].tmHeight);
    ok(tm[0].tmAscent == tm[1].tmAscent, "expected %d, got %d\n", tm[0].tmAscent, tm[1].tmAscent);
    ok(tm[0].tmDescent == tm[1].tmDescent, "expected %d, got %d\n", tm[0].tmDescent, tm[1].tmDescent);
    ok((tm[0].tmAveCharWidth + 1) == tm[1].tmAveCharWidth,
       "expected %d, got %d\n", tm[0].tmAveCharWidth + 1, tm[1].tmAveCharWidth);
    ok((tm[0].tmMaxCharWidth + 1) == tm[1].tmMaxCharWidth,
       "expected %d, got %d\n", tm[0].tmMaxCharWidth + 1, tm[1].tmMaxCharWidth);
    ok(tm[0].tmOverhang == tm[1].tmOverhang, "expected %d, got %d\n", tm[0].tmOverhang, tm[1].tmOverhang);
    w[0] = abc[0].abcA + abc[0].abcB + abc[0].abcC;
    w[1] = abc[1].abcA + abc[1].abcB + abc[1].abcC;
    ok((w[0] + 1) == w[1], "expected %d, got %d\n", w[0] + 1, w[1]);

}

static void test_bitmap_font_glyph_index(void)
{
    const WCHAR text[] = {'#','!','/','b','i','n','/','s','h',0};
    const struct {
        LPCSTR face;
        BYTE charset;
    } bitmap_font_list[] = {
        { "Courier", ANSI_CHARSET },
        { "Small Fonts", ANSI_CHARSET },
        { "Fixedsys", DEFAULT_CHARSET },
        { "System", DEFAULT_CHARSET }
    };
    HDC hdc;
    LOGFONTA lf;
    HFONT hFont;
    CHAR facename[LF_FACESIZE];
    BITMAPINFO bmi;
    HBITMAP hBmp[2];
    void *pixels[2];
    int i, j;
    DWORD ret;
    BITMAP bmp;
    TEXTMETRICA tm;
    CHARSETINFO ci;
    BYTE chr = '\xA9';

    if (!pGetGlyphIndicesW || !pGetGlyphIndicesA) {
        win_skip("GetGlyphIndices is unavailable\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biWidth = 128;
    bmi.bmiHeader.biHeight = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    for (i = 0; i < sizeof(bitmap_font_list)/sizeof(bitmap_font_list[0]); i++) {
        memset(&lf, 0, sizeof(lf));
        lf.lfCharSet = bitmap_font_list[i].charset;
        strcpy(lf.lfFaceName, bitmap_font_list[i].face);
        hFont = CreateFontIndirectA(&lf);
        ok(hFont != NULL, "Can't create font (%s:%d)\n", lf.lfFaceName, lf.lfCharSet);
        hFont = SelectObject(hdc, hFont);
        ret = GetTextMetricsA(hdc, &tm);
        ok(ret, "GetTextMetric failed\n");
        ret = GetTextFaceA(hdc, sizeof(facename), facename);
        ok(ret, "GetTextFace failed\n");
        if (tm.tmPitchAndFamily & TMPF_TRUETYPE) {
            skip("TrueType font (%s) was selected for \"%s\"\n", facename, bitmap_font_list[i].face);
            continue;
        }
        if (lstrcmpiA(facename, lf.lfFaceName) != 0) {
            skip("expected %s, got %s\n", lf.lfFaceName, facename);
            continue;
        }

        for (j = 0; j < 2; j++) {
            HBITMAP hBmpPrev;
            hBmp[j] = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pixels[j], NULL, 0);
            ok(hBmp[j] != NULL, "Can't create DIB\n");
            hBmpPrev = SelectObject(hdc, hBmp[j]);
            switch (j) {
            case 0:
                ret = ExtTextOutW(hdc, 0, 0, 0, NULL, text, lstrlenW(text), NULL);
                break;
            case 1:
            {
                int len = lstrlenW(text);
                LPWORD indices = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WORD));
                ret = pGetGlyphIndicesW(hdc, text, len, indices, 0);
                ok(ret, "GetGlyphIndices failed\n");
                ok(memcmp(indices, text, sizeof(WORD) * len) == 0,
                   "Glyph indices and text are different for %s:%d\n", lf.lfFaceName, tm.tmCharSet);
                ret = ExtTextOutW(hdc, 0, 0, ETO_GLYPH_INDEX, NULL, indices, len, NULL);
                HeapFree(GetProcessHeap(), 0, indices);
                break;
            }
            }
            ok(ret, "ExtTextOutW failed\n");
            SelectObject(hdc, hBmpPrev);
        }

        GetObjectA(hBmp[0], sizeof(bmp), &bmp);
        ok(memcmp(pixels[0], pixels[1], bmp.bmHeight * bmp.bmWidthBytes) == 0,
           "Images are different (%s:%d)\n", lf.lfFaceName, tm.tmCharSet);

        ret = TranslateCharsetInfo((LPDWORD)(DWORD_PTR)tm.tmCharSet, &ci, TCI_SRCCHARSET);
        if (!ret) {
            skip("Can't get charset info for (%s:%d)\n", lf.lfFaceName, tm.tmCharSet);
            goto next;
        }
        if (IsDBCSLeadByteEx(ci.ciACP, chr)) {
            skip("High-ascii character is not defined in codepage %d\n", ci.ciACP);
            goto next;
        }

        for (j = 0; j < 2; j++) {
            HBITMAP hBmpPrev;
            WORD code;
            hBmpPrev = SelectObject(hdc, hBmp[j]);
            switch (j) {
            case 0:
                ret = ExtTextOutA(hdc, 100, 0, 0, NULL, (LPCSTR)&chr, 1, NULL);
                break;
            case 1:
                ret = pGetGlyphIndicesA(hdc, (LPCSTR)&chr, 1, &code, 0);
                ok(ret, "GetGlyphIndices failed\n");
                ok(code == chr, "expected %02x, got %02x (%s:%d)\n", chr, code, lf.lfFaceName, tm.tmCharSet);
                ret = ExtTextOutA(hdc, 100, 0, ETO_GLYPH_INDEX, NULL, (LPCSTR)&code, 1, NULL);
                break;
            }
            ok(ret, "ExtTextOutA failed\n");
            SelectObject(hdc, hBmpPrev);
        }

        ok(memcmp(pixels[0], pixels[1], bmp.bmHeight * bmp.bmWidthBytes) == 0,
           "Images are different (%s:%d)\n", lf.lfFaceName, tm.tmCharSet);
    next:
        for (j = 0; j < 2; j++)
            DeleteObject(hBmp[j]);
        hFont = SelectObject(hdc, hFont);
        DeleteObject(hFont);
    }

    DeleteDC(hdc);
}

START_TEST(font)
{
    init();

    test_stock_fonts();
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
    test_GdiGetCodePage();
    test_GetFontUnicodeRanges();
    test_nonexistent_font();
    test_orientation();
    test_height_selection();
    test_AddFontMemResource();
    test_EnumFonts();

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
    test_EnumFontFamiliesEx_default_charset();
    test_GetTextMetrics();
    test_GdiRealizationInfo();
    test_GetTextFace();
    test_GetGlyphOutline();
    test_GetTextMetrics2("Tahoma", -11);
    test_GetTextMetrics2("Tahoma", -55);
    test_GetTextMetrics2("Tahoma", -110);
    test_GetTextMetrics2("Arial", -11);
    test_GetTextMetrics2("Arial", -55);
    test_GetTextMetrics2("Arial", -110);
    test_CreateFontIndirect();
    test_CreateFontIndirectEx();
    test_oemcharset();
    test_fullname();
    test_fullname2();
    test_east_asian_font_selection();
    test_max_height();
    test_vertical_order();
    test_GetCharWidth32();
    test_fake_bold_font();
    test_bitmap_font_glyph_index();

    /* These tests should be last test until RemoveFontResource
     * is properly implemented.
     */
    test_vertical_font();
    test_CreateScalableFontResource();
}
