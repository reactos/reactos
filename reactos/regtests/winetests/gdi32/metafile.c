/*
 * Unit tests for metafile functions
 *
 * Copyright (c) 2002 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

static LOGFONTA orig_lf;
static BOOL emr_processed = FALSE;

/* Arbitrarily chosen values for the second co-ordinate of a metafile line */
#define LINE_X 55.0f
#define LINE_Y 15.0f

static int CALLBACK emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
    const ENHMETARECORD *emr, int n_objs, LPARAM param)
{
    static int n_record;
    DWORD i;
    const INT *dx;
    INT *orig_dx = (INT *)param;
    LOGFONTA device_lf;
    INT ret;

    trace("hdc %p, emr->iType %ld, emr->nSize %ld, param %p\n",
           hdc, emr->iType, emr->nSize, (void *)param);

    if(!hdc) return 1;

    PlayEnhMetaFileRecord(hdc, handle_table, emr, n_objs);

    switch (emr->iType)
    {
    case EMR_HEADER:
        n_record = 0;
        break;

    case EMR_EXTTEXTOUTA:
    {
        const EMREXTTEXTOUTA *emr_ExtTextOutA = (const EMREXTTEXTOUTA *)emr;
        dx = (const INT *)((const char *)emr + emr_ExtTextOutA->emrtext.offDx);

        ret = GetObjectA(GetCurrentObject(hdc, OBJ_FONT), sizeof(device_lf), &device_lf);
        ok( ret == sizeof(device_lf), "GetObjectA error %ld\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutA->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%ld] (%d) didn't match %d\n",
                                     n_record, i, dx[i], orig_dx[i]);
        }
        n_record++;
        emr_processed = TRUE;
        break;
    }

    case EMR_EXTTEXTOUTW:
    {
        const EMREXTTEXTOUTW *emr_ExtTextOutW = (const EMREXTTEXTOUTW *)emr;
        dx = (const INT *)((const char *)emr + emr_ExtTextOutW->emrtext.offDx);

        ret = GetObjectA(GetCurrentObject(hdc, OBJ_FONT), sizeof(device_lf), &device_lf);
        ok( ret == sizeof(device_lf), "GetObjectA error %ld\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutW->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%ld] (%d) didn't match %d\n",
                                     n_record, i, dx[i], orig_dx[i]);
        }
        n_record++;
        emr_processed = TRUE;
        break;
    }

    default:
        break;
    }

    return 1;
}

static void test_ExtTextOut(void)
{
    HWND hwnd;
    HDC hdcDisplay, hdcMetafile;
    HENHMETAFILE hMetafile;
    HFONT hFont;
    static const char text[] = "Simple text to test ExtTextOut on metafiles";
    INT i, len, dx[256];
    static const RECT rc = { 0, 0, 100, 100 };
    BOOL ret;

    assert(sizeof(dx)/sizeof(dx[0]) >= lstrlenA(text));

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %ld\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %ld\n", GetLastError());

    trace("hdcDisplay %p\n", hdcDisplay);

    SetMapMode(hdcDisplay, MM_TEXT);

    memset(&orig_lf, 0, sizeof(orig_lf));

    orig_lf.lfCharSet = ANSI_CHARSET;
    orig_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    orig_lf.lfWeight = FW_DONTCARE;
    orig_lf.lfHeight = 7;
    orig_lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(orig_lf.lfFaceName, "Arial");
    hFont = CreateFontIndirectA(&orig_lf);
    ok(hFont != 0, "CreateFontIndirectA error %ld\n", GetLastError());

    hFont = SelectObject(hdcDisplay, hFont);

    len = lstrlenA(text);
    for (i = 0; i < len; i++)
    {
        ret = GetCharWidthA(hdcDisplay, text[i], text[i], &dx[i]);
        ok( ret, "GetCharWidthA error %ld\n", GetLastError());
    }
    hFont = SelectObject(hdcDisplay, hFont);

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %ld\n", GetLastError());

    trace("hdcMetafile %p\n", hdcMetafile);

    ok(GetDeviceCaps(hdcMetafile, TECHNOLOGY) == DT_RASDISPLAY,
       "GetDeviceCaps(TECHNOLOGY) has to return DT_RASDISPLAY for a display based EMF\n");

    hFont = SelectObject(hdcMetafile, hFont);

    /* 1. pass NULL lpDx */
    ret = ExtTextOutA(hdcMetafile, 0, 0, 0, &rc, text, lstrlenA(text), NULL);
    ok( ret, "ExtTextOutA error %ld\n", GetLastError());

    /* 2. pass custom lpDx */
    ret = ExtTextOutA(hdcMetafile, 0, 20, 0, &rc, text, lstrlenA(text), dx);
    ok( ret, "ExtTextOutA error %ld\n", GetLastError());

    hFont = SelectObject(hdcMetafile, hFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %ld\n", GetLastError());

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %ld\n", GetLastError());

    ok(!GetObjectType(hdcMetafile), "CloseEnhMetaFile has to destroy metafile hdc\n");

    ret = PlayEnhMetaFile(hdcDisplay, hMetafile, &rc);
    ok( ret, "PlayEnhMetaFile error %ld\n", GetLastError());

    ret = EnumEnhMetaFile(hdcDisplay, hMetafile, emf_enum_proc, dx, &rc);
    ok( ret, "EnumEnhMetaFile error %ld\n", GetLastError());

    ok(emr_processed, "EnumEnhMetaFile couldn't find EMR_EXTTEXTOUTA or EMR_EXTTEXTOUTW record\n");

    ok(!EnumEnhMetaFile(hdcDisplay, hMetafile, emf_enum_proc, dx, NULL),
       "A valid hdc has to require a valid rc\n");

    ok(EnumEnhMetaFile(NULL, hMetafile, emf_enum_proc, dx, NULL),
       "A null hdc does not require a valid rc\n");

    ret = DeleteEnhMetaFile(hMetafile);
    ok( ret, "DeleteEnhMetaFile error %ld\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok( ret, "ReleaseDC error %ld\n", GetLastError());
}

/* Win-format metafile (mfdrv) tests */
/* These tests compare the generated metafiles byte-by-byte */
/* with the nominal results. */

/* Maximum size of sample metafiles in bytes. */
#define MF_BUFSIZE 256

/* 8x8 bitmap data for a pattern brush */
static const unsigned char SAMPLE_PATTERN_BRUSH[] = {
    0x01, 0x00, 0x02, 0x00,
    0x03, 0x00, 0x04, 0x00,
    0x05, 0x00, 0x06, 0x00,
    0x07, 0x00, 0x08, 0x00
};

/* Sample metafiles to be compared to the outputs of the
 * test functions.
 */

static const unsigned char MF_BLANK_BITS[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x0c, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char MF_GRAPHICS_BITS[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x22, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x14, 0x02,
    0x01, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x13, 0x02, 0x02, 0x00, 0x02, 0x00, 0x05, 0x00,
    0x00, 0x00, 0x14, 0x02, 0x01, 0x00, 0x01, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x18, 0x04, 0x02, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const unsigned char MF_PATTERN_BRUSH_BITS[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x3d, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x2d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x42, 0x01,
    0x03, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x2d, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00
};

/* For debugging or dumping the raw metafiles produced by
 * new test functions.
 */

static void dump_mf_bits (const HMETAFILE mf, const char *desc)
{
    char buf[MF_BUFSIZE];
    UINT mfsize, i;

    mfsize = GetMetaFileBitsEx (mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetMetaFileBitsEx failed.\n", desc);

    printf ("MetaFile %s has bits:\n{\n    ", desc);
    for (i=0; i<mfsize; i++)
    {
        printf ("0x%.2hhx", buf[i]);
        if (i == mfsize-1)
            printf ("\n");
        else if (i % 8 == 7)
            printf (",\n    ");
        else
            printf (", ");
    }
    printf ("};\n");
}

/* Compare the metafile produced by a test function with the
 * expected raw metafile data in "bits".
 * Return value is 0 for a perfect match,
 * -1 if lengths aren't equal,
 * otherwise returns the number of non-matching bytes.
 */

static int compare_mf_bits (const HMETAFILE mf, const char *bits, UINT bsize,
    const char *desc)
{
    char buf[MF_BUFSIZE];
    UINT mfsize, i;
    int diff;

    mfsize = GetMetaFileBitsEx (mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetMetaFileBitsEx failed.\n", desc);
    if (mfsize < MF_BUFSIZE)
        ok (mfsize == bsize, "%s: mfsize=%d, bsize=%d.\n",
            desc, mfsize, bsize);
    else
        ok (bsize >= MF_BUFSIZE, "%s: mfsize > bufsize (%d bytes), bsize=%d.\n",
            desc, mfsize, bsize);
    if (mfsize != bsize)
        return -1;

    diff = 0;
    for (i=0; i<bsize; i++)
    {
       if (buf[i] !=  bits[i])
           diff++;
    }
    ok (diff == 0, "%s: mfsize=%d, bsize=%d, diff=%d\n",
        desc, mfsize, bsize, diff);

    return diff; 
}

/* Test a blank metafile.  May be used as a template for new tests. */

static void test_mf_Blank(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile;
    INT caps;
    BOOL ret;

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

/* Tests on metafile initialization */
    caps = GetDeviceCaps (hdcMetafile, TECHNOLOGY);
    ok (caps == DT_METAFILE,
        "GetDeviceCaps: TECHNOLOGY=%d != DT_METAFILE.\n", caps);

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_BLANK_BITS, sizeof(MF_BLANK_BITS),
        "mf_blank") != 0)
            dump_mf_bits (hMetafile, "mf_Blank");

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %ld\n", hMetafile, GetLastError());
}

/* Simple APIs from mfdrv/graphics.c
 */

static void test_mf_Graphics(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile;
    POINT oldpoint;
    BOOL ret;

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    ret = MoveToEx(hdcMetafile, 1, 1, NULL);
    ok( ret, "MoveToEx error %ld.\n", GetLastError());
    ret = LineTo(hdcMetafile, 2, 2);
    ok( ret, "LineTo error %ld.\n", GetLastError());
    ret = MoveToEx(hdcMetafile, 1, 1, &oldpoint);
    ok( ret, "MoveToEx error %ld.\n", GetLastError());

/* oldpoint gets garbage under Win XP, so the following test would
 * work under Wine but fails under Windows:
 *
 *   ok((oldpoint.x == 2) && (oldpoint.y == 2),
 *       "MoveToEx: (x, y) = (%ld, %ld), should be (2, 2).\n",
 *       oldpoint.x, oldpoint.y);
 */

    ret = Ellipse(hdcMetafile, 0, 0, 2, 2);
    ok( ret, "Ellipse error %ld.\n", GetLastError());

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS),
        "mf_Graphics") != 0)
            dump_mf_bits (hMetafile, "mf_Graphics");

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %ld\n",
        hMetafile, GetLastError());
}

static void test_mf_PatternBrush(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile;
    LOGBRUSH *orig_lb;
    HBRUSH hBrush;
    BOOL ret;

    orig_lb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOGBRUSH));

    orig_lb->lbStyle = BS_PATTERN;
    orig_lb->lbColor = RGB(0, 0, 0);
    orig_lb->lbHatch = (INT) CreateBitmap (8, 8, 1, 1, SAMPLE_PATTERN_BRUSH);
    ok((HBITMAP *)orig_lb->lbHatch != NULL, "CreateBitmap error %ld.\n", GetLastError());

    hBrush = CreateBrushIndirect (orig_lb);
    ok(hBrush != 0, "CreateBrushIndirect error %ld\n", GetLastError());

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    hBrush = SelectObject(hdcMetafile, hBrush);
    ok(hBrush != 0, "SelectObject error %ld.\n", GetLastError());

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_PATTERN_BRUSH_BITS, sizeof(MF_PATTERN_BRUSH_BITS),
        "mf_Pattern_Brush") != 0)
            dump_mf_bits (hMetafile, "mf_Pattern_Brush");

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile error %ld\n", GetLastError());
    ret = DeleteObject(hBrush);
    ok( ret, "DeleteObject(HBRUSH) error %ld\n", GetLastError());
    ret = DeleteObject((HBITMAP *)orig_lb->lbHatch);
    ok( ret, "DeleteObject(HBITMAP) error %ld\n",
        GetLastError());
    HeapFree (GetProcessHeap(), 0, orig_lb);
}

static INT CALLBACK EmfEnumProc(HDC hdc, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR, INT nObj, LPARAM lpData)
{
    LPMETAFILEPICT lpMFP = (LPMETAFILEPICT)lpData;
    POINT mapping[2] = { { 0, 0 }, { 10, 10 } };
    /* When using MM_TEXT Win9x does not update the mapping mode 
     * until a record is played which actually outputs something */
    PlayEnhMetaFileRecord(hdc, lpHTable, lpEMFR, nObj);
    LPtoDP(hdc, mapping, 2);
    trace("Meta record: iType = %ld, (%ld,%ld)-(%ld,%ld)\n", lpEMFR->iType, mapping[0].x, mapping[0].y, mapping[1].x, mapping[1].y);
    if (lpEMFR->iType == EMR_LINETO)
    {
        INT x0, y0, x1, y1;
        if (!lpMFP || lpMFP->mm == MM_TEXT)
        {
            x0 = 0;
            y0 = 0;
            x1 = (INT)floor(10 * 100.0 / LINE_X + 0.5);
            y1 = (INT)floor(10 * 100.0 / LINE_Y + 0.5);
        }
        else
        {
            ok(lpMFP->mm == MM_ANISOTROPIC, "mm=%ld\n", lpMFP->mm);
            
            x0 = MulDiv(0, GetDeviceCaps(hdc, HORZSIZE) * 100, GetDeviceCaps(hdc, HORZRES));
            y0 = MulDiv(0, GetDeviceCaps(hdc, VERTSIZE) * 100, GetDeviceCaps(hdc, VERTRES));
            x1 = MulDiv(10, GetDeviceCaps(hdc, HORZSIZE) * 100, GetDeviceCaps(hdc, HORZRES));
            y1 = MulDiv(10, GetDeviceCaps(hdc, VERTSIZE) * 100, GetDeviceCaps(hdc, VERTRES));
        }
        ok(mapping[0].x == x0 && mapping[0].y == y0 && mapping[1].x == x1 && mapping[1].y == y1,
            "(%ld,%ld)->(%ld,%ld), expected (%d,%d)->(%d,%d)\n",
            mapping[0].x, mapping[0].y, mapping[1].x, mapping[1].y,
            x0, y0, x1, y1);
    }
    return TRUE;
}

static HENHMETAFILE create_converted_emf(const METAFILEPICT *mfp)
{
    HDC hdcMf;
    HMETAFILE hmf;
    BOOL ret;
    UINT size;
    LPBYTE pBits;

    hdcMf = CreateMetaFile(NULL);
    ok(hdcMf != NULL, "CreateMetaFile failed with error %ld\n", GetLastError());
    ret = LineTo(hdcMf, (INT)LINE_X, (INT)LINE_Y);
    ok(ret, "LineTo failed with error %ld\n", GetLastError());
    hmf = CloseMetaFile(hdcMf);
    ok(hmf != NULL, "CloseMetaFile failed with error %ld\n", GetLastError());
    size = GetMetaFileBitsEx(hmf, 0, NULL);
    ok(size, "GetMetaFileBitsEx failed with error %ld\n", GetLastError());
    pBits = HeapAlloc(GetProcessHeap(), 0, size);
    GetMetaFileBitsEx(hmf, size, pBits);
    DeleteMetaFile(hmf);
    return SetWinMetaFileBits(size, pBits, NULL, mfp);
}

static void test_mf_conversions(void)
{
    trace("Testing MF->EMF conversion (MM_ANISOTROPIC)\n");
    {
        HDC hdcOffscreen = CreateCompatibleDC(NULL);
        HENHMETAFILE hemf;
        METAFILEPICT mfp;
        RECT rect = { 0, 0, 100, 100 };
        mfp.mm = MM_ANISOTROPIC;
        mfp.xExt = 100;
        mfp.yExt = 100;
        mfp.hMF = NULL;
        hemf = create_converted_emf(&mfp);
        EnumEnhMetaFile(hdcOffscreen, hemf, EmfEnumProc, &mfp, &rect);
        DeleteEnhMetaFile(hemf);
        DeleteDC(hdcOffscreen);
    }

    trace("Testing MF->EMF conversion (MM_TEXT)\n");
    {
        HDC hdcOffscreen = CreateCompatibleDC(NULL);
        HENHMETAFILE hemf;
        METAFILEPICT mfp;
        RECT rect = { 0, 0, 100, 100 };
        mfp.mm = MM_TEXT;
        mfp.xExt = 0;
        mfp.yExt = 0;
        mfp.hMF = NULL;
        hemf = create_converted_emf(&mfp);
        EnumEnhMetaFile(hdcOffscreen, hemf, EmfEnumProc, &mfp, &rect);
        DeleteEnhMetaFile(hemf);
        DeleteDC(hdcOffscreen);
    }

    trace("Testing MF->EMF conversion (NULL mfp)\n");
    {
        HDC hdcOffscreen = CreateCompatibleDC(NULL);
        HENHMETAFILE hemf;
        RECT rect = { 0, 0, 100, 100 };
        hemf = create_converted_emf(NULL);
        EnumEnhMetaFile(hdcOffscreen, hemf, EmfEnumProc, NULL, &rect);
        DeleteEnhMetaFile(hemf);
        DeleteDC(hdcOffscreen);
    }
}

static BOOL (WINAPI *pGdiIsMetaPrintDC)(HDC);
static BOOL (WINAPI *pGdiIsMetaFileDC)(HDC);
static BOOL (WINAPI *pGdiIsPlayMetafileDC)(HDC);

static void test_gdiis(void)
{
    RECT rect = {0,0,100,100};
    HDC hdc, hemfDC, hmfDC;
    HENHMETAFILE hemf;
    HMODULE hgdi32;

    /* resolve all the functions */
    hgdi32 = GetModuleHandle("gdi32");
    pGdiIsMetaPrintDC = (void*) GetProcAddress(hgdi32, "GdiIsMetaPrintDC");
    pGdiIsMetaFileDC = (void*) GetProcAddress(hgdi32, "GdiIsMetaFileDC");
    pGdiIsPlayMetafileDC = (void*) GetProcAddress(hgdi32, "GdiIsPlayMetafileDC");

    /* they should all exist or none should exist */
    if(!pGdiIsMetaPrintDC)
        return;

    /* try with nothing */
    ok(!pGdiIsMetaPrintDC(NULL), "ismetaprint with NULL parameter\n");
    ok(!pGdiIsMetaFileDC(NULL), "ismetafile with NULL parameter\n");
    ok(!pGdiIsPlayMetafileDC(NULL), "isplaymetafile with NULL parameter\n");

    /* try with a metafile */
    hmfDC = CreateMetaFile(NULL);
    ok(!pGdiIsMetaPrintDC(hmfDC), "ismetaprint on metafile\n");
    ok(pGdiIsMetaFileDC(hmfDC), "ismetafile on metafile\n");
    ok(!pGdiIsPlayMetafileDC(hmfDC), "isplaymetafile on metafile\n");
    DeleteObject(CloseMetaFile(hmfDC));

    /* try with an enhanced metafile */
    hdc = GetDC(NULL);
    hemfDC = CreateEnhMetaFileW(hdc, NULL, &rect, NULL);
    ok(hemfDC != NULL, "failed to create emf\n");

    ok(!pGdiIsMetaPrintDC(hemfDC), "ismetaprint on emf\n");
    ok(pGdiIsMetaFileDC(hemfDC), "ismetafile on emf\n");
    ok(!pGdiIsPlayMetafileDC(hemfDC), "isplaymetafile on emf\n");

    hemf = CloseEnhMetaFile(hemfDC);
    ok(hemf != NULL, "failed to close EMF\n");
    DeleteObject(hemf);
    ReleaseDC(NULL,hdc);
}

START_TEST(metafile)
{
    /* For enhanced metafiles (enhmfdrv) */
    test_ExtTextOut();

    /* For win-format metafiles (mfdrv) */
    test_mf_Blank();
    test_mf_Graphics();
    test_mf_PatternBrush();

    /* For metafile conversions */
    test_mf_conversions();

    test_gdiis();
}
