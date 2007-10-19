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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

static INT (WINAPI * pGetRelAbs)(HDC, DWORD);
static INT (WINAPI * pSetRelAbs)(HDC, INT);

#define GDI_GET_PROC(func)                                     \
    p ## func = (void *)GetProcAddress(hGDI, #func);           \
    if(!p ## func)                                             \
        trace("GetProcAddress(hGDI, \"%s\") failed\n", #func); \

static void init_function_pointers(void)
{
    HMODULE hGDI;

    pGetRelAbs = NULL;
    pSetRelAbs = NULL;

    hGDI = GetModuleHandleA("gdi32.dll");
    assert(hGDI);
    GDI_GET_PROC(GetRelAbs);
    GDI_GET_PROC(SetRelAbs);
}

static int CALLBACK eto_emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
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
        ok(GetTextAlign(hdc) == 0, "text align %08x\n", GetTextAlign(hdc));
        ok(GetBkColor(hdc) == RGB(0xff, 0xff, 0xff), "bk color %08lx\n", GetBkColor(hdc));
        ok(GetTextColor(hdc) == RGB(0x0, 0x0, 0x0), "text color %08lx\n", GetTextColor(hdc));
        ok(GetROP2(hdc) == R2_COPYPEN, "rop %d\n", GetROP2(hdc));
        ok(GetArcDirection(hdc) == AD_COUNTERCLOCKWISE, "arc dir %d\n", GetArcDirection(hdc));
        ok(GetPolyFillMode(hdc) == ALTERNATE, "poly fill %d\n", GetPolyFillMode(hdc));
        ok(GetStretchBltMode(hdc) == BLACKONWHITE, "stretchblt mode %d\n", GetStretchBltMode(hdc));

        /* GetBkMode, GetRelAbs do not get reset to the default value */
        ok(GetBkMode(hdc) == OPAQUE, "bk mode %d\n", GetBkMode(hdc));
        if(pSetRelAbs && pGetRelAbs)
            ok(pGetRelAbs(hdc, 0) == RELATIVE, "relabs %d\n", pGetRelAbs(hdc, 0));

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

    SetTextAlign(hdcDisplay, TA_UPDATECP | TA_CENTER | TA_BASELINE | TA_RTLREADING );
    SetBkColor(hdcDisplay, RGB(0xff, 0, 0));
    SetTextColor(hdcDisplay, RGB(0, 0xff, 0));
    SetROP2(hdcDisplay, R2_NOT);
    SetArcDirection(hdcDisplay, AD_CLOCKWISE);
    SetPolyFillMode(hdcDisplay, WINDING);
    SetStretchBltMode(hdcDisplay, HALFTONE);

    if(pSetRelAbs) pSetRelAbs(hdcDisplay, RELATIVE);
    SetBkMode(hdcDisplay, OPAQUE);

    ret = EnumEnhMetaFile(hdcDisplay, hMetafile, eto_emf_enum_proc, dx, &rc);
    ok( ret, "EnumEnhMetaFile error %ld\n", GetLastError());

    ok( GetTextAlign(hdcDisplay) == (TA_UPDATECP | TA_CENTER | TA_BASELINE | TA_RTLREADING),
        "text align %08x\n", GetTextAlign(hdcDisplay));
    ok( GetBkColor(hdcDisplay) == RGB(0xff, 0, 0), "bk color %08lx\n", GetBkColor(hdcDisplay));
    ok( GetTextColor(hdcDisplay) == RGB(0, 0xff, 0), "text color %08lx\n", GetTextColor(hdcDisplay));
    ok( GetROP2(hdcDisplay) == R2_NOT, "rop2 %d\n", GetROP2(hdcDisplay));
    ok( GetArcDirection(hdcDisplay) == AD_CLOCKWISE, "arc dir  %d\n", GetArcDirection(hdcDisplay));
    ok( GetPolyFillMode(hdcDisplay) == WINDING, "poly fill %d\n", GetPolyFillMode(hdcDisplay));
    ok( GetStretchBltMode(hdcDisplay) == HALFTONE, "stretchblt mode %d\n", GetStretchBltMode(hdcDisplay));

    ok(emr_processed, "EnumEnhMetaFile couldn't find EMR_EXTTEXTOUTA or EMR_EXTTEXTOUTW record\n");

    ok(!EnumEnhMetaFile(hdcDisplay, hMetafile, eto_emf_enum_proc, dx, NULL),
       "A valid hdc has to require a valid rc\n");

    ok(EnumEnhMetaFile(NULL, hMetafile, eto_emf_enum_proc, dx, NULL),
       "A null hdc does not require a valid rc\n");

    ret = DeleteEnhMetaFile(hMetafile);
    ok( ret, "DeleteEnhMetaFile error %ld\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok( ret, "ReleaseDC error %ld\n", GetLastError());
    DestroyWindow(hwnd);
}

static int CALLBACK savedc_emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
                                         const ENHMETARECORD *emr, int n_objs, LPARAM param)
{
    static int save_state;
    static int restore_no;

    switch (emr->iType)
    {
    case EMR_HEADER:
        save_state = 0;
        restore_no = 0;
        break;

    case EMR_SAVEDC:
        save_state++;
        break;

    case EMR_RESTOREDC:
        {
            EMRRESTOREDC *restoredc = (EMRRESTOREDC *)emr;
            switch(++restore_no)
            {
            case 1:
                ok(restoredc->iRelative == -1, "first restore %ld\n", restoredc->iRelative);
                break;

            case 2:
                ok(restoredc->iRelative == -3, "second restore %ld\n", restoredc->iRelative);
                break;
            case 3:
                ok(restoredc->iRelative == -2, "third restore %ld\n", restoredc->iRelative);
                break;
            }
            ok(restore_no <= 3, "restore_no %d\n", restore_no);
            save_state += restoredc->iRelative;
            break;
        }
    case EMR_EOF:
        ok(save_state == 0, "EOF save_state %d\n", save_state);
        break;
    }


    return 1;
}

void test_SaveDC(void)
{
    HDC hdcMetafile, hdcDisplay;
    HENHMETAFILE hMetafile;
    HWND hwnd;
    int ret;
    static const RECT rc = { 0, 0, 100, 100 };

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %ld\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %ld\n", GetLastError());

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %ld\n", GetLastError());

    /* Need to write something to the emf, otherwise Windows won't play it back */
    LineTo(hdcMetafile, 100, 100);

    ret = SaveDC(hdcMetafile);
    ok(ret == 1, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 2, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 3, "ret = %d\n", ret);

    ret = RestoreDC(hdcMetafile, -1);
    ok(ret, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 3, "ret = %d\n", ret);

    ret = RestoreDC(hdcMetafile, 1);
    ok(ret, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 1, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 2, "ret = %d\n", ret);

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %ld\n", GetLastError());

    ret = EnumEnhMetaFile(hdcDisplay, hMetafile, savedc_emf_enum_proc, 0, &rc);
    ok( ret == 1, "EnumEnhMetaFile rets %d\n", ret);

    ret = DeleteEnhMetaFile(hMetafile);
    ok( ret, "DeleteEnhMetaFile error %ld\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok( ret, "ReleaseDC error %ld\n", GetLastError());
    DestroyWindow(hwnd);
}

/* Win-format metafile (mfdrv) tests */
/* These tests compare the generated metafiles byte-by-byte */
/* with the nominal results. */

/* Maximum size of sample metafiles in bytes. */
#define MF_BUFSIZE 512

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

static const unsigned char MF_TEXTOUT_ON_PATH_BITS[] =
{
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x19, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x32, 0x0a,
    0x16, 0x00, 0x0b, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x54, 0x65, 0x73, 0x74, 0x03, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static const unsigned char EMF_TEXTOUT_ON_PATH_BITS[] =
{
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe7, 0xff, 0xff, 0xff, 0xe9, 0xff, 0xff, 0xff,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0xf4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x40, 0x01, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0x04, 0x00,
    0x80, 0xa9, 0x03, 0x00, 0x3b, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc8, 0x41, 0x00, 0x80, 0xbb, 0x41,
    0x0b, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x54, 0x00, 0x00, 0x00,
    0x54, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x3c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00
};

static const unsigned char MF_LINETO_BITS[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x13, 0x02,
    0x0f, 0x00, 0x37, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static const unsigned char EMF_LINETO_BITS[] = {
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x61, 0x06, 0x00, 0x00, 0xb7, 0x01, 0x00, 0x00,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0x38, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x7c, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x60, 0xcc, 0x05, 0x00,
    0xe0, 0x93, 0x04, 0x00, 0x46, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00,
    0x47, 0x44, 0x49, 0x43, 0x01, 0x00, 0x00, 0x80,
    0x00, 0x03, 0x00, 0x00, 0x60, 0xe5, 0xf4, 0x73,
    0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x13, 0x02,
    0x0f, 0x00, 0x37, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x36, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x25, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x30, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x80, 0x4b, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00
};

static const unsigned char EMF_LINETO_MM_ANISOTROPIC_BITS[] = {
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0x38, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x7c, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x60, 0xcc, 0x05, 0x00,
    0xe0, 0x93, 0x04, 0x00, 0x46, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00,
    0x47, 0x44, 0x49, 0x43, 0x01, 0x00, 0x00, 0x80,
    0x00, 0x03, 0x00, 0x00, 0xa4, 0xfe, 0xf4, 0x73,
    0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x13, 0x02,
    0x0f, 0x00, 0x37, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x36, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x25, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x30, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x80, 0x4b, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00
};

static const unsigned char EMF_LINETO_MM_TEXT_BITS[] = {
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x37, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x61, 0x06, 0x00, 0x00, 0xb7, 0x01, 0x00, 0x00,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0xe4, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x7c, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x60, 0xcc, 0x05, 0x00,
    0xe0, 0x93, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x80,
    0x25, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x30, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x80,
    0x4b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00
};

/* For debugging or dumping the raw metafiles produced by
 * new test functions.
 */
static INT CALLBACK mf_enum_proc(HDC hdc, HANDLETABLE *ht, METARECORD *mr,
                                 INT nobj, LPARAM param)
{
    trace("hdc %p, mr->rdFunction %04x, mr->rdSize %lu, param %p\n",
           hdc, mr->rdFunction, mr->rdSize, (void *)param);
    return TRUE;
}

/* For debugging or dumping the raw metafiles produced by
 * new test functions.
 */

static void dump_mf_bits (const HMETAFILE mf, const char *desc)
{
    BYTE buf[MF_BUFSIZE];
    UINT mfsize, i;

    if (!winetest_debug) return;

    mfsize = GetMetaFileBitsEx (mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetMetaFileBitsEx failed.\n", desc);

    printf ("MetaFile %s has bits:\n{\n    ", desc);
    for (i=0; i<mfsize; i++)
    {
        printf ("0x%02x", buf[i]);
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

static int compare_mf_bits (const HMETAFILE mf, const unsigned char *bits, UINT bsize,
    const char *desc)
{
    unsigned char buf[MF_BUFSIZE];
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

static int compare_mf_disk_bits(LPCSTR name, const BYTE *bits, UINT bsize, const char *desc)
{
    unsigned char buf[MF_BUFSIZE];
    DWORD mfsize, rd_size, i;
    int diff;
    HANDLE hfile;
    BOOL ret;

    hfile = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    assert(hfile != INVALID_HANDLE_VALUE);

    mfsize = GetFileSize(hfile, NULL);
    assert(mfsize <= MF_BUFSIZE);

    ret = ReadFile(hfile, buf, sizeof(buf), &rd_size, NULL);
    ok( ret && rd_size == mfsize, "ReadFile: error %ld\n", GetLastError());

    CloseHandle(hfile);

    ok(mfsize == bsize, "%s: mfsize=%ld, bsize=%d.\n", desc, mfsize, bsize);

    if (mfsize != bsize)
        return -1;

    diff = 0;
    for (i=0; i<bsize; i++)
    {
        if (buf[i] != bits[i])
            diff++;
    }
    ok(diff == 0, "%s: mfsize=%ld, bsize=%d, diff=%d\n",
        desc, mfsize, bsize, diff);

    return diff;
}

/* For debugging or dumping the raw EMFs produced by
 * new test functions.
 */
static void dump_emf_bits(const HENHMETAFILE mf, const char *desc)
{
    BYTE buf[MF_BUFSIZE];
    UINT mfsize, i;

    if (!winetest_debug) return;

    mfsize = GetEnhMetaFileBits(mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetEnhMetaFileBits failed\n", desc);

    printf("EMF %s has bits:\n{\n    ", desc);
    for (i = 0; i < mfsize; i++)
    {
        printf ("0x%02x", buf[i]);
        if (i == mfsize-1)
            printf ("\n");
        else if (i % 8 == 7)
            printf (",\n    ");
        else
            printf (", ");
    }
    printf ("};\n");
}

static void dump_emf_records(const HENHMETAFILE mf, const char *desc)
{
    BYTE *emf;
    BYTE buf[MF_BUFSIZE];
    UINT mfsize, offset;

    if (!winetest_debug) return;

    mfsize = GetEnhMetaFileBits(mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetEnhMetaFileBits error %ld\n", desc, GetLastError());

    printf("EMF %s has records:\n", desc);

    emf = buf;
    offset = 0;
    while(offset < mfsize)
    {
        EMR *emr = (EMR *)(emf + offset);
        printf("emr->iType %ld, emr->nSize %lu\n", emr->iType, emr->nSize);
        /*trace("emr->iType 0x%04lx, emr->nSize 0x%04lx\n", emr->iType, emr->nSize);*/
        offset += emr->nSize;
    }
}

/* Compare the EMF produced by a test function with the
 * expected raw EMF data in "bits".
 * Return value is 0 for a perfect match,
 * -1 if lengths aren't equal,
 * otherwise returns the number of non-matching bytes.
 */
static int compare_emf_bits(const HENHMETAFILE mf, const unsigned char *bits,
                            UINT bsize, const char *desc, BOOL todo)
{
    unsigned char buf[MF_BUFSIZE];
    UINT mfsize, i;
    int diff;

    mfsize = GetEnhMetaFileBits(mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetEnhMetaFileBits error %ld\n", desc, GetLastError());

    if (mfsize < MF_BUFSIZE)
    {
        if (mfsize != bsize && todo)
        {
        todo_wine
        ok(mfsize == bsize, "%s: mfsize=%d, bsize=%d\n", desc, mfsize, bsize);
        }
        else
        ok(mfsize == bsize, "%s: mfsize=%d, bsize=%d\n", desc, mfsize, bsize);
    }
    else
        ok(bsize >= MF_BUFSIZE, "%s: mfsize > bufsize (%d bytes), bsize=%d\n",
           desc, mfsize, bsize);

    if (mfsize != bsize)
        return -1;

    diff = 0;
    for (i = 0; i < bsize; i++)
    {
       if (buf[i] != bits[i])
           diff++;
    }
    if (diff != 0 && todo)
    {
        todo_wine
        {
            ok(diff == 0, "%s: mfsize=%d, bsize=%d, diff=%d\n",
               desc, mfsize, bsize, diff);
        }
        return diff;
    }
    else
    {
        ok(diff == 0, "%s: mfsize=%d, bsize=%d, diff=%d\n",
           desc, mfsize, bsize, diff);

        return diff;
    }
}

/* Test a blank metafile.  May be used as a template for new tests. */

static void test_mf_Blank(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile;
    INT caps;
    BOOL ret;
    INT type;

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

/* Tests on metafile initialization */
    caps = GetDeviceCaps (hdcMetafile, TECHNOLOGY);
    ok (caps == DT_METAFILE,
        "GetDeviceCaps: TECHNOLOGY=%d != DT_METAFILE.\n", caps);

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());
    type = GetObjectType(hMetafile);
    ok(type == OBJ_METAFILE, "CloseMetaFile created object with type %d\n", type);
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_BLANK_BITS, sizeof(MF_BLANK_BITS),
        "mf_blank") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Blank");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %ld\n", hMetafile, GetLastError());
}

static void test_CopyMetaFile(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile, hmf_copy;
    BOOL ret;
    char temp_path[MAX_PATH];
    char mf_name[MAX_PATH];
    INT type;

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());
    type = GetObjectType(hMetafile);
    ok(type == OBJ_METAFILE, "CloseMetaFile created object with type %d\n", type);

    if (compare_mf_bits (hMetafile, MF_BLANK_BITS, sizeof(MF_BLANK_BITS),
        "mf_blank") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Blank");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "wmf", 0, mf_name);

    hmf_copy = CopyMetaFileA(hMetafile, mf_name);
    ok(hmf_copy != 0, "CopyMetaFile error %ld\n", GetLastError());

    type = GetObjectType(hmf_copy);
    ok(type == OBJ_METAFILE, "CopyMetaFile created object with type %d\n", type);

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %ld\n", hMetafile, GetLastError());

    if (compare_mf_disk_bits(mf_name, MF_BLANK_BITS, sizeof(MF_BLANK_BITS), "mf_blank") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Blank");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf_copy);
    ok( ret, "DeleteMetaFile(%p) error %ld\n", hmf_copy, GetLastError());

    DeleteFileA(mf_name);
}

static void test_SetMetaFileBits(void)
{
    HMETAFILE hmf;
    INT type;
    BOOL ret;
    BYTE buf[256];
    METAHEADER *mh;

    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), MF_GRAPHICS_BITS);
    ok(hmf != 0, "SetMetaFileBitsEx error %ld\n", GetLastError());
    type = GetObjectType(hmf);
    ok(type == OBJ_METAFILE, "SetMetaFileBitsEx created object with type %d\n", type);

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %ld\n", hmf, GetLastError());

    /* NULL data crashes XP SP1 */
    /*hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), NULL);*/

    /* Now with not zero size */
    SetLastError(0xdeadbeef);
    hmf = SetMetaFileBitsEx(0, MF_GRAPHICS_BITS);
    ok(!hmf, "SetMetaFileBitsEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "wrong error %ld\n", GetLastError());

    /* Now with not even size */
    SetLastError(0xdeadbeef);
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS) - 1, MF_GRAPHICS_BITS);
    ok(!hmf, "SetMetaFileBitsEx should fail\n");
    ok(GetLastError() == 0xdeadbeef /* XP SP1 */, "wrong error %ld\n", GetLastError());

    /* Now with zeroed out or faked some header fields */
    assert(sizeof(buf) >= sizeof(MF_GRAPHICS_BITS));
    memcpy(buf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS));
    mh = (METAHEADER *)buf;
    /* corruption of any of the below fields leads to a failure */
    mh->mtType = 0;
    mh->mtVersion = 0;
    mh->mtHeaderSize = 0;
    SetLastError(0xdeadbeef);
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), buf);
    ok(!hmf, "SetMetaFileBitsEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "wrong error %ld\n", GetLastError());

    /* Now with corrupted mtSize field */
    memcpy(buf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS));
    mh = (METAHEADER *)buf;
    /* corruption of mtSize doesn't lead to a failure */
    mh->mtSize *= 2;
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), buf);
    ok(hmf != 0, "SetMetaFileBitsEx error %ld\n", GetLastError());

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %ld\n", hmf, GetLastError());

    /* Now with zeroed out mtSize field */
    memcpy(buf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS));
    mh = (METAHEADER *)buf;
    /* zeroing mtSize doesn't lead to a failure */
    mh->mtSize = 0;
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), buf);
    ok(hmf != 0, "SetMetaFileBitsEx error %ld\n", GetLastError());

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %ld\n", hmf, GetLastError());
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
    {
        dump_mf_bits(hMetafile, "mf_Graphics");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

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
    orig_lb->lbHatch = (ULONG_PTR)CreateBitmap (8, 8, 1, 1, SAMPLE_PATTERN_BRUSH);
    ok((HBITMAP)orig_lb->lbHatch != NULL, "CreateBitmap error %ld.\n", GetLastError());

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
    {
        dump_mf_bits(hMetafile, "mf_Pattern_Brush");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile error %ld\n", GetLastError());
    ret = DeleteObject(hBrush);
    ok( ret, "DeleteObject(HBRUSH) error %ld\n", GetLastError());
    ret = DeleteObject((HBITMAP)orig_lb->lbHatch);
    ok( ret, "DeleteObject(HBITMAP) error %ld\n",
        GetLastError());
    HeapFree (GetProcessHeap(), 0, orig_lb);
}

static void test_mf_ExtTextOut_on_path(void)
{
    HDC hdcMetafile;
    HMETAFILE hMetafile;
    BOOL ret;
    static const INT dx[4] = { 3, 5, 8, 12 };

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %ld\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    ret = BeginPath(hdcMetafile);
    ok(!ret, "BeginPath on metafile DC should fail\n");

    ret = ExtTextOutA(hdcMetafile, 11, 22, 0, NULL, "Test", 4, dx);
    ok(ret, "ExtTextOut error %ld\n", GetLastError());

    ret = EndPath(hdcMetafile);
    ok(!ret, "EndPath on metafile DC should fail\n");

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %ld\n", GetLastError());

    if (compare_mf_bits(hMetafile, MF_TEXTOUT_ON_PATH_BITS, sizeof(MF_TEXTOUT_ON_PATH_BITS),
        "mf_TextOut_on_path") != 0)
    {
        dump_mf_bits(hMetafile, "mf_TextOut_on_path");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok(ret, "DeleteMetaFile(%p) error %ld\n", hMetafile, GetLastError());
}

static void test_emf_ExtTextOut_on_path(void)
{
    HWND hwnd;
    HDC hdcDisplay, hdcMetafile;
    HENHMETAFILE hMetafile;
    BOOL ret;
    static const INT dx[4] = { 3, 5, 8, 12 };

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %ld\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %ld\n", GetLastError());

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %ld\n", GetLastError());

    ret = BeginPath(hdcMetafile);
    ok(ret, "BeginPath error %ld\n", GetLastError());

    ret = ExtTextOutA(hdcMetafile, 11, 22, 0, NULL, "Test", 4, dx);
    ok(ret, "ExtTextOut error %ld\n", GetLastError());

    ret = EndPath(hdcMetafile);
    ok(ret, "EndPath error %ld\n", GetLastError());

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %ld\n", GetLastError());

    /* this doesn't succeed yet: EMF has correct size, all EMF records
     * are there, but their contents don't match for different reasons.
     */
    if (compare_emf_bits(hMetafile, EMF_TEXTOUT_ON_PATH_BITS, sizeof(EMF_TEXTOUT_ON_PATH_BITS),
        "emf_TextOut_on_path", TRUE) != 0)
    {
        dump_emf_bits(hMetafile, "emf_TextOut_on_path");
        dump_emf_records(hMetafile, "emf_TextOut_on_path");
    }

    ret = DeleteEnhMetaFile(hMetafile);
    ok(ret, "DeleteEnhMetaFile error %ld\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok(ret, "ReleaseDC error %ld\n", GetLastError());
    DestroyWindow(hwnd);
}

static INT CALLBACK EmfEnumProc(HDC hdc, HANDLETABLE *lpHTable, const ENHMETARECORD *lpEMFR, INT nObj, LPARAM lpData)
{
    LPMETAFILEPICT lpMFP = (LPMETAFILEPICT)lpData;
    POINT mapping[2] = { { 0, 0 }, { 10, 10 } };
    /* When using MM_TEXT Win9x does not update the mapping mode
     * until a record is played which actually outputs something */
    PlayEnhMetaFileRecord(hdc, lpHTable, lpEMFR, nObj);
    LPtoDP(hdc, mapping, 2);
    trace("Meta record: iType %ld, nSize %ld, (%ld,%ld)-(%ld,%ld)\n",
           lpEMFR->iType, lpEMFR->nSize,
           mapping[0].x, mapping[0].y, mapping[1].x, mapping[1].y);

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

    if (compare_mf_bits (hmf, MF_LINETO_BITS, sizeof(MF_LINETO_BITS), "mf_LineTo") != 0)
    {
        dump_mf_bits(hmf, "mf_LineTo");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

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

        if (compare_emf_bits(hemf, EMF_LINETO_MM_ANISOTROPIC_BITS, sizeof(EMF_LINETO_MM_ANISOTROPIC_BITS),
                             "emf_LineTo MM_ANISOTROPIC", TRUE) != 0)
        {
            dump_emf_bits(hemf, "emf_LineTo MM_ANISOTROPIC");
            dump_emf_records(hemf, "emf_LineTo MM_ANISOTROPIC");
        }

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

        if (compare_emf_bits(hemf, EMF_LINETO_MM_TEXT_BITS, sizeof(EMF_LINETO_MM_TEXT_BITS),
                             "emf_LineTo MM_TEXT", TRUE) != 0)
        {
            dump_emf_bits(hemf, "emf_LineTo MM_TEXT");
            dump_emf_records(hemf, "emf_LineTo MM_TEXT");
        }

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

        if (compare_emf_bits(hemf, EMF_LINETO_BITS, sizeof(EMF_LINETO_BITS),
                             "emf_LineTo NULL", TRUE) != 0)
        {
            dump_emf_bits(hemf, "emf_LineTo NULL");
            dump_emf_records(hemf, "emf_LineTo NULL");
        }

        EnumEnhMetaFile(hdcOffscreen, hemf, EmfEnumProc, NULL, &rect);

        DeleteEnhMetaFile(hemf);
        DeleteDC(hdcOffscreen);
    }
}

static BOOL getConvertedFrameAndBounds(UINT buffer_size, BYTE * buffer, BOOL mfpIsNull,
                                       LONG mm, LONG xExt, LONG yExt,
                                       RECTL * rclBounds, RECTL * rclFrame)
{
  METAFILEPICT mfp;
  METAFILEPICT * mfpPtr = NULL;
  HENHMETAFILE emf;
  ENHMETAHEADER header;
  UINT res;

  if (!mfpIsNull)
  {
    mfp.mm = mm;
    mfp.xExt = xExt;
    mfp.yExt = yExt;
    mfpPtr = &mfp;
  }

  emf = SetWinMetaFileBits(buffer_size, buffer, NULL, mfpPtr);
  ok(emf != NULL, "SetWinMetaFileBits failed\n");
  if (!emf) return FALSE;
  res = GetEnhMetaFileHeader(emf, sizeof(header), &header);
  ok(res != 0, "GetEnhMetaHeader failed\n");
  DeleteEnhMetaFile(emf);
  if (!res) return FALSE;

  *rclBounds = header.rclBounds;
  *rclFrame = header.rclFrame;
  return TRUE;
}

static void checkConvertedFrameAndBounds(UINT buffer_size, BYTE * buffer, BOOL mfpIsNull,
                                         LONG mm, LONG xExt, LONG yExt,
                                         RECTL * rclBoundsExpected, RECTL * rclFrameExpected)
{
  RECTL rclBounds, rclFrame;

  if (getConvertedFrameAndBounds(buffer_size, buffer, mfpIsNull, mm, xExt, yExt, &rclBounds, &rclFrame))
  {
    const char * msg;
    char buf[64];

    if (mfpIsNull)
    {
       msg = "mfp == NULL";
    }
    else
    {
      const char * mm_str;
      switch (mm)
      {
         case MM_ANISOTROPIC: mm_str = "MM_ANISOTROPIC"; break;
         case MM_ISOTROPIC:   mm_str = "MM_ISOTROPIC"; break;
         default:             mm_str = "Unexpected";
      }
      sprintf(buf, "mm=%s, xExt=%ld, yExt=%ld", mm_str, xExt, yExt);
      msg = buf;
    }

    ok(rclBounds.left == rclBoundsExpected->left, "rclBounds.left: Expected %ld, got %ld (%s)\n", rclBoundsExpected->left, rclBounds.left, msg);
    ok(rclBounds.top == rclBoundsExpected->top, "rclBounds.top: Expected %ld, got %ld (%s)\n", rclBoundsExpected->top, rclBounds.top, msg);
    ok(rclBounds.right == rclBoundsExpected->right, "rclBounds.right: Expected %ld, got %ld (%s)\n", rclBoundsExpected->right, rclBounds.right, msg);
    ok(rclBounds.bottom == rclBoundsExpected->bottom, "rclBounds.bottom: Expected %ld, got %ld (%s)\n", rclBoundsExpected->bottom, rclBounds.bottom, msg);
    ok(rclFrame.left == rclFrameExpected->left, "rclFrame.left: Expected %ld, got %ld (%s)\n", rclFrameExpected->left, rclFrame.left, msg);
    ok(rclFrame.top == rclFrameExpected->top, "rclFrame.top: Expected %ld, got %ld (%s)\n", rclFrameExpected->top, rclFrame.top, msg);
    ok(rclFrame.right == rclFrameExpected->right, "rclFrame.right: Expected %ld, got %ld (%s)\n", rclFrameExpected->right, rclFrame.right, msg);
    ok(rclFrame.bottom == rclFrameExpected->bottom, "rclFrame.bottom: Expected %ld, got %ld (%s)\n", rclFrameExpected->bottom, rclFrame.bottom, msg);
  }
}

static void test_SetWinMetaFileBits(void)
{
  HMETAFILE wmf;
  HDC wmfDC;
  BYTE * buffer;
  UINT buffer_size;
  RECT rect;
  UINT res;
  RECTL rclBoundsAnisotropic, rclFrameAnisotropic;
  RECTL rclBoundsIsotropic, rclFrameIsotropic;
  RECTL rclBounds, rclFrame;
  HDC dc;
  LONG diffx, diffy;

  wmfDC = CreateMetaFile(NULL);
  ok(wmfDC != NULL, "CreateMetaFile failed\n");
  if (!wmfDC) return;

  SetWindowExtEx(wmfDC, 100, 100, NULL);
  rect.left = rect.top = 0;
  rect.right = rect.bottom = 50;
  FillRect(wmfDC, &rect, GetStockObject(BLACK_BRUSH));
  wmf = CloseMetaFile(wmfDC);
  ok(wmf != NULL, "Metafile creation failed\n");
  if (!wmf) return;

  buffer_size = GetMetaFileBitsEx(wmf, 0, NULL);
  ok(buffer_size != 0, "GetMetaFileBitsEx failed\n");
  if (buffer_size == 0)
  {
    DeleteMetaFile(wmf);
    return;
  }

  buffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, buffer_size);
  ok(buffer != NULL, "HeapAlloc failed\n");
  if (!buffer)
  {
    DeleteMetaFile(wmf);
    return;
  }

  res = GetMetaFileBitsEx(wmf, buffer_size, buffer);
  ok(res == buffer_size, "GetMetaFileBitsEx failed\n");
  DeleteMetaFile(wmf);
  if (res != buffer_size)
  {
     HeapFree(GetProcessHeap(), 0, buffer);
     return;
  }

  /* Get the reference bounds and frame */
  getConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 0, 0, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  getConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 0, 0,  &rclBoundsIsotropic, &rclFrameIsotropic);

  ok(rclBoundsAnisotropic.left == 0 && rclBoundsAnisotropic.top == 0 &&
     rclBoundsIsotropic.left == 0 && rclBoundsIsotropic.top == 0,
     "SetWinMetaFileBits: Reference bounds: Left and top bound must be zero\n");

  ok(rclBoundsAnisotropic.right >= rclBoundsIsotropic.right, "SetWinMetaFileBits: Reference bounds: Invalid right bound\n");
  ok(rclBoundsAnisotropic.bottom >= rclBoundsIsotropic.bottom, "SetWinMetaFileBits: Reference bounds: Invalid bottom bound\n");
  diffx = rclBoundsIsotropic.right - rclBoundsIsotropic.bottom;
  if (diffx < 0) diffx = -diffx;
  ok(diffx <= 1, "SetWinMetaFileBits (MM_ISOTROPIC): Reference bounds are not isotropic\n");

  dc = CreateCompatibleDC(NULL);
  todo_wine
  {
  ok(rclBoundsAnisotropic.right == GetDeviceCaps(dc, HORZRES) / 2 - 1 &&
     rclBoundsAnisotropic.bottom == GetDeviceCaps(dc, VERTRES) / 2 - 1,
     "SetWinMetaFileBits (MM_ANISOTROPIC): Reference bounds: The whole device surface must be used (%dx%d), but got (%ldx%ld)\n",
     GetDeviceCaps(dc, HORZRES) / 2 - 1, GetDeviceCaps(dc, VERTRES) / 2 - 1, rclBoundsAnisotropic.right, rclBoundsAnisotropic.bottom);
  }

  /* Allow 1 mm difference (rounding errors) */
  diffx = rclFrameAnisotropic.right / 100 - GetDeviceCaps(dc, HORZSIZE) / 2;
  diffy = rclFrameAnisotropic.bottom / 100 - GetDeviceCaps(dc, VERTSIZE) / 2;
  if (diffx < 0) diffx = -diffx;
  if (diffy < 0) diffy = -diffy;
  todo_wine
  {
  ok(diffx <= 1 && diffy <= 1,
     "SetWinMetaFileBits (MM_ANISOTROPIC): Reference frame: The whole device surface must be used (%dx%d), but got (%ldx%ld)\n",
     GetDeviceCaps(dc, HORZSIZE) / 2, GetDeviceCaps(dc, VERTSIZE) / 2, rclFrameAnisotropic.right / 100, rclFrameAnisotropic.bottom / 100);
  }
  DeleteDC(dc);

  /* If the METAFILEPICT pointer is NULL, the MM_ANISOTROPIC mapping mode and the whole device surface are used */
  checkConvertedFrameAndBounds(buffer_size, buffer, TRUE, 0, 0, 0, &rclBoundsAnisotropic, &rclFrameAnisotropic);

  /* If xExt or yExt is zero or negative, the whole device surface is used */
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 10000, 0, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 10000, 0, &rclBoundsIsotropic, &rclFrameIsotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 0, 10000, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 0, 10000, &rclBoundsIsotropic, &rclFrameIsotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, -10000, 0, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, -10000, 0, &rclBoundsIsotropic, &rclFrameIsotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 0, -10000, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 0, -10000, &rclBoundsIsotropic, &rclFrameIsotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, -10000, 10000, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, -10000, 10000, &rclBoundsIsotropic, &rclFrameIsotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 10000, -10000, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 10000, -10000, &rclBoundsIsotropic, &rclFrameIsotropic);

  /* MSDN says that negative xExt and yExt values specify a ratio.
     Check that this is wrong and the whole device surface is used */
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, -1000, -100, &rclBoundsAnisotropic, &rclFrameAnisotropic);
  checkConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, -1000, -100, &rclBoundsIsotropic, &rclFrameIsotropic);

  /* Ordinary conversions */

  if (getConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ANISOTROPIC, 30000, 20000, &rclBounds, &rclFrame))
  {
    ok(rclFrame.left == 0 && rclFrame.top == 0 && rclFrame.right == 30000 && rclFrame.bottom == 20000,
       "SetWinMetaFileBits (MM_ANISOTROPIC): rclFrame contains invalid values\n");
    ok(rclBounds.left == 0 && rclBounds.top == 0 && rclBounds.right > rclBounds.bottom,
       "SetWinMetaFileBits (MM_ANISOTROPIC): rclBounds contains invalid values\n");
  }

  if (getConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_ISOTROPIC, 30000, 20000, &rclBounds, &rclFrame))
  {
    ok(rclFrame.left == 0 && rclFrame.top == 0 && rclFrame.right == 30000 && rclFrame.bottom == 20000,
       "SetWinMetaFileBits (MM_ISOTROPIC): rclFrame contains invalid values\n");
    ok(rclBounds.left == 0 && rclBounds.top == 0,
       "SetWinMetaFileBits (MM_ISOTROPIC): rclBounds contains invalid values\n");

    /* Wine has a rounding error */
    diffx = rclBounds.right - rclBounds.bottom;
    if (diffx < 0) diffx = -diffx;
    ok(diffx <= 1, "SetWinMetaFileBits (MM_ISOTROPIC): rclBounds is not isotropic\n");
  }

  if (getConvertedFrameAndBounds(buffer_size, buffer, FALSE, MM_HIMETRIC, 30000, 20000, &rclBounds, &rclFrame))
  {
    ok(rclFrame.right - rclFrame.left != 30000 && rclFrame.bottom - rclFrame.top != 20000,
       "SetWinMetaFileBits: xExt and yExt must be ignored for mapping modes other than MM_ANISOTROPIC and MM_ISOTROPIC\n");
  }

  HeapFree(GetProcessHeap(), 0, buffer);
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
    init_function_pointers();

    /* For enhanced metafiles (enhmfdrv) */
    test_ExtTextOut();
    test_SaveDC();

    /* For win-format metafiles (mfdrv) */
    test_mf_Blank();
    test_mf_Graphics();
    test_mf_PatternBrush();
    test_CopyMetaFile();
    test_SetMetaFileBits();
    test_mf_ExtTextOut_on_path();
    test_emf_ExtTextOut_on_path();

    /* For metafile conversions */
    test_mf_conversions();
    test_SetWinMetaFileBits();

    test_gdiis();
}
