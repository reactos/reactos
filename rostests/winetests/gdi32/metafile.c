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

    trace("hdc %p, emr->iType %d, emr->nSize %d, param %p\n",
           hdc, emr->iType, emr->nSize, (void *)param);

    if(!hdc) return 1;

    PlayEnhMetaFileRecord(hdc, handle_table, emr, n_objs);

    switch (emr->iType)
    {
    case EMR_HEADER:
        ok(GetTextAlign(hdc) == 0, "text align %08x\n", GetTextAlign(hdc));
        ok(GetBkColor(hdc) == RGB(0xff, 0xff, 0xff), "bk color %08x\n", GetBkColor(hdc));
        ok(GetTextColor(hdc) == RGB(0x0, 0x0, 0x0), "text color %08x\n", GetTextColor(hdc));
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
        ok( ret == sizeof(device_lf), "GetObjectA error %d\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutA->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%d] (%d) didn't match %d\n",
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
        ok( ret == sizeof(device_lf), "GetObjectA error %d\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutW->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%d] (%d) didn't match %d\n",
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
    ok(hwnd != 0, "CreateWindowExA error %d\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %d\n", GetLastError());

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
    ok(hFont != 0, "CreateFontIndirectA error %d\n", GetLastError());

    hFont = SelectObject(hdcDisplay, hFont);

    len = lstrlenA(text);
    for (i = 0; i < len; i++)
    {
        ret = GetCharWidthA(hdcDisplay, text[i], text[i], &dx[i]);
        ok( ret, "GetCharWidthA error %d\n", GetLastError());
    }
    hFont = SelectObject(hdcDisplay, hFont);

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %d\n", GetLastError());

    trace("hdcMetafile %p\n", hdcMetafile);

    ok(GetDeviceCaps(hdcMetafile, TECHNOLOGY) == DT_RASDISPLAY,
       "GetDeviceCaps(TECHNOLOGY) has to return DT_RASDISPLAY for a display based EMF\n");

    hFont = SelectObject(hdcMetafile, hFont);

    /* 1. pass NULL lpDx */
    ret = ExtTextOutA(hdcMetafile, 0, 0, 0, &rc, text, lstrlenA(text), NULL);
    ok( ret, "ExtTextOutA error %d\n", GetLastError());

    /* 2. pass custom lpDx */
    ret = ExtTextOutA(hdcMetafile, 0, 20, 0, &rc, text, lstrlenA(text), dx);
    ok( ret, "ExtTextOutA error %d\n", GetLastError());

    hFont = SelectObject(hdcMetafile, hFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %d\n", GetLastError());

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %d\n", GetLastError());

    ok(!GetObjectType(hdcMetafile), "CloseEnhMetaFile has to destroy metafile hdc\n");

    ret = PlayEnhMetaFile(hdcDisplay, hMetafile, &rc);
    ok( ret, "PlayEnhMetaFile error %d\n", GetLastError());

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
    ok( ret, "EnumEnhMetaFile error %d\n", GetLastError());

    ok( GetTextAlign(hdcDisplay) == (TA_UPDATECP | TA_CENTER | TA_BASELINE | TA_RTLREADING),
        "text align %08x\n", GetTextAlign(hdcDisplay));
    ok( GetBkColor(hdcDisplay) == RGB(0xff, 0, 0), "bk color %08x\n", GetBkColor(hdcDisplay));
    ok( GetTextColor(hdcDisplay) == RGB(0, 0xff, 0), "text color %08x\n", GetTextColor(hdcDisplay));
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
    ok( ret, "DeleteEnhMetaFile error %d\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok( ret, "ReleaseDC error %d\n", GetLastError());
    DestroyWindow(hwnd);
}

static void check_dc_state(HDC hdc, int restore_no,
                           int wnd_org_x, int wnd_org_y, int wnd_ext_x, int wnd_ext_y,
                           int vp_org_x, int vp_org_y, int vp_ext_x, int vp_ext_y)
{
    BOOL ret;
    XFORM xform;
    POINT vp_org, win_org;
    SIZE vp_size, win_size;
    FLOAT xscale, yscale, edx, edy;

    SetLastError(0xdeadbeef);
    ret = GetWorldTransform(hdc, &xform);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) goto win9x_here;
    ok(ret, "GetWorldTransform error %u\n", GetLastError());

    trace("%d: eM11 %f, eM22 %f, eDx %f, eDy %f\n", restore_no, xform.eM11, xform.eM22, xform.eDx, xform.eDy);

    ok(xform.eM12 == 0.0, "%d: expected eM12 0.0, got %f\n", restore_no, xform.eM12);
    ok(xform.eM21 == 0.0, "%d: expected eM21 0.0, got %f\n", restore_no, xform.eM21);

    xscale = (FLOAT)vp_ext_x / (FLOAT)wnd_ext_x;
    trace("x scale %f\n", xscale);
    ok(fabs(xscale - xform.eM11) < 0.01, "%d: vp_ext_x %d, wnd_ext_cx %d, eM11 %f\n",
       restore_no, vp_ext_x, wnd_ext_x, xform.eM11);

    yscale = (FLOAT)vp_ext_y / (FLOAT)wnd_ext_y;
    trace("y scale %f\n", yscale);
    ok(fabs(yscale - xform.eM22) < 0.01, "%d: vp_ext_y %d, wnd_ext_y %d, eM22 %f\n",
       restore_no, vp_ext_y, wnd_ext_y, xform.eM22);

    edx = (FLOAT)vp_org_x - xform.eM11 * (FLOAT)wnd_org_x;
    ok(fabs(edx - xform.eDx) < 0.01, "%d: edx %f != eDx %f\n", restore_no, edx, xform.eDx);
    edy = (FLOAT)vp_org_y - xform.eM22 * (FLOAT)wnd_org_y;
    ok(fabs(edy - xform.eDy) < 0.01, "%d: edy %f != eDy %f\n", restore_no, edy, xform.eDy);

    return;

win9x_here:

    GetWindowOrgEx(hdc, &win_org);
    GetViewportOrgEx(hdc, &vp_org);
    GetWindowExtEx(hdc, &win_size);
    GetViewportExtEx(hdc, &vp_size);

    ok(wnd_org_x == win_org.x, "%d: wnd_org_x: %d != %d\n", restore_no, wnd_org_x, win_org.x);
    ok(wnd_org_y == win_org.y, "%d: wnd_org_y: %d != %d\n", restore_no, wnd_org_y, win_org.y);

    ok(vp_org_x == vp_org.x, "%d: vport_org_x: %d != %d\n", restore_no, vp_org_x, vp_org.x);
    ok(vp_org_y == vp_org.y, "%d: vport_org_y: %d != %d\n", restore_no, vp_org_y, vp_org.y);

    ok(wnd_ext_x == win_size.cx, "%d: wnd_ext_x: %d != %d\n", restore_no, wnd_ext_x, win_size.cx);
    ok(wnd_ext_y == win_size.cy, "%d: wnd_ext_y: %d != %d\n", restore_no, wnd_ext_y, win_size.cy);

    ok(vp_ext_x == vp_size.cx, "%d: vport_ext_x: %d != %d\n", restore_no, vp_ext_x, vp_size.cx);
    ok(vp_ext_y == vp_size.cy, "%d: vport_ext_y: %d != %d\n", restore_no, vp_ext_y, vp_size.cy);
}

static int CALLBACK savedc_emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
                                         const ENHMETARECORD *emr, int n_objs, LPARAM param)
{
    BOOL ret;
    XFORM xform;
    POINT pt;
    SIZE size;
    static int save_state;
    static int restore_no;

    trace("hdc %p, emr->iType %d, emr->nSize %d, param %p\n",
           hdc, emr->iType, emr->nSize, (void *)param);

    trace("BEFORE:\n");
    SetLastError(0xdeadbeef);
    ret = GetWorldTransform(hdc, &xform);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(GetWindowOrgEx(hdc, &pt), "GetWindowOrgEx error %u\n", GetLastError());
        trace("window org (%d,%d)\n", pt.x, pt.y);
        ok(GetViewportOrgEx(hdc, &pt), "GetViewportOrgEx error %u\n", GetLastError());
        trace("vport org (%d,%d)\n", pt.x, pt.y);
        ok(GetWindowExtEx(hdc, &size), "GetWindowExtEx error %u\n", GetLastError());
        trace("window ext (%d,%d)\n", size.cx, size.cy);
        ok(GetViewportExtEx(hdc, &size), "GetViewportExtEx error %u\n", GetLastError());
        trace("vport ext (%d,%d)\n", size.cx, size.cy);
    }
    else
    {
        ok(ret, "GetWorldTransform error %u\n", GetLastError());
        trace("eM11 %f, eM22 %f, eDx %f, eDy %f\n", xform.eM11, xform.eM22, xform.eDx, xform.eDy);
    }

    PlayEnhMetaFileRecord(hdc, handle_table, emr, n_objs);

    switch (emr->iType)
    {
    case EMR_HEADER:
    {
        static RECT exp_bounds = { 0, 0, 150, 150 };
        RECT bounds;
        const ENHMETAHEADER *emf = (const ENHMETAHEADER *)emr;

        trace("bounds %d,%d-%d,%d, frame %d,%d-%d,%d\n",
               emf->rclBounds.left, emf->rclBounds.top, emf->rclBounds.right, emf->rclBounds.bottom,
               emf->rclFrame.left, emf->rclFrame.top, emf->rclFrame.right, emf->rclFrame.bottom);
        trace("mm %d x %d, device %d x %d\n", emf->szlMillimeters.cx, emf->szlMillimeters.cy,
               emf->szlDevice.cx, emf->szlDevice.cy);

        SetRect(&bounds, emf->rclBounds.left, emf->rclBounds.top, emf->rclBounds.right, emf->rclBounds.bottom);
        ok(EqualRect(&bounds, &exp_bounds), "wrong bounds\n");

        save_state = 0;
        restore_no = 0;
        check_dc_state(hdc, restore_no, 0, 0, 1, 1, 0, 0, 1, 1);
        break;
    }

    case EMR_LINETO:
        {
            const EMRLINETO *line = (const EMRLINETO *)emr;
            trace("EMR_LINETO %d,%d\n", line->ptl.x, line->ptl.x);
            break;
        }
    case EMR_SETWINDOWORGEX:
        {
            const EMRSETWINDOWORGEX *org = (const EMRSETWINDOWORGEX *)emr;
            trace("EMR_SETWINDOWORGEX: %d,%d\n", org->ptlOrigin.x, org->ptlOrigin.y);
            break;
        }
    case EMR_SETWINDOWEXTEX:
        {
            const EMRSETWINDOWEXTEX *ext = (const EMRSETWINDOWEXTEX *)emr;
            trace("EMR_SETWINDOWEXTEX: %d,%d\n", ext->szlExtent.cx, ext->szlExtent.cy);
            break;
        }
    case EMR_SETVIEWPORTORGEX:
        {
            const EMRSETVIEWPORTORGEX *org = (const EMRSETVIEWPORTORGEX *)emr;
            trace("EMR_SETVIEWPORTORGEX: %d,%d\n", org->ptlOrigin.x, org->ptlOrigin.y);
            break;
        }
    case EMR_SETVIEWPORTEXTEX:
        {
            const EMRSETVIEWPORTEXTEX *ext = (const EMRSETVIEWPORTEXTEX *)emr;
            trace("EMR_SETVIEWPORTEXTEX: %d,%d\n", ext->szlExtent.cx, ext->szlExtent.cy);
            break;
        }
    case EMR_SAVEDC:
        save_state++;
        trace("EMR_SAVEDC\n");
        break;

    case EMR_RESTOREDC:
        {
            const EMRRESTOREDC *restoredc = (const EMRRESTOREDC *)emr;
            trace("EMR_RESTOREDC: %d\n", restoredc->iRelative);

            switch(++restore_no)
            {
            case 1:
                ok(restoredc->iRelative == -1, "first restore %d\n", restoredc->iRelative);
                check_dc_state(hdc, restore_no, -2, -2, 8192, 8192, 20, 20, 20479, 20478);
                break;
            case 2:
                ok(restoredc->iRelative == -3, "second restore %d\n", restoredc->iRelative);
                check_dc_state(hdc, restore_no, 0, 0, 16384, 16384, 0, 0, 17873, 17872);
                break;
            case 3:
                ok(restoredc->iRelative == -2, "third restore %d\n", restoredc->iRelative);
                check_dc_state(hdc, restore_no, -4, -4, 32767, 32767, 40, 40, 3276, 3276);
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

    trace("AFTER:\n");
    SetLastError(0xdeadbeef);
    ret = GetWorldTransform(hdc, &xform);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(GetWindowOrgEx(hdc, &pt), "GetWindowOrgEx error %u\n", GetLastError());
        trace("window org (%d,%d)\n", pt.x, pt.y);
        ok(GetViewportOrgEx(hdc, &pt), "GetViewportOrgEx error %u\n", GetLastError());
        trace("vport org (%d,%d)\n", pt.x, pt.y);
        ok(GetWindowExtEx(hdc, &size), "GetWindowExtEx error %u\n", GetLastError());
        trace("window ext (%d,%d)\n", size.cx, size.cy);
        ok(GetViewportExtEx(hdc, &size), "GetViewportExtEx error %u\n", GetLastError());
        trace("vport ext (%d,%d)\n", size.cx, size.cy);
    }
    else
    {
        ok(ret, "GetWorldTransform error %u\n", GetLastError());
        trace("eM11 %f, eM22 %f, eDx %f, eDy %f\n", xform.eM11, xform.eM22, xform.eDx, xform.eDy);
    }

    return 1;
}

static void test_SaveDC(void)
{
    HDC hdcMetafile, hdcDisplay;
    HENHMETAFILE hMetafile;
    HWND hwnd;
    int ret;
    static const RECT rc = { 0, 0, 150, 150 };

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %d\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %d\n", GetLastError());

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %d\n", GetLastError());

    SetMapMode(hdcMetafile, MM_ANISOTROPIC);

    /* Need to write something to the emf, otherwise Windows won't play it back */
    LineTo(hdcMetafile, 150, 150);

    SetWindowOrgEx(hdcMetafile, 0, 0, NULL);
    SetViewportOrgEx(hdcMetafile, 0, 0, NULL);
    SetWindowExtEx(hdcMetafile, 110, 110, NULL );
    SetViewportExtEx(hdcMetafile, 120, 120, NULL );

    /* Force Win9x to update DC state */
    SetPixelV(hdcMetafile, 50, 50, 0);

    ret = SaveDC(hdcMetafile);
    ok(ret == 1, "ret = %d\n", ret);

    SetWindowOrgEx(hdcMetafile, -1, -1, NULL);
    SetViewportOrgEx(hdcMetafile, 10, 10, NULL);
    SetWindowExtEx(hdcMetafile, 150, 150, NULL );
    SetViewportExtEx(hdcMetafile, 200, 200, NULL );

    /* Force Win9x to update DC state */
    SetPixelV(hdcMetafile, 50, 50, 0);

    ret = SaveDC(hdcMetafile);
    ok(ret == 2, "ret = %d\n", ret);

    SetWindowOrgEx(hdcMetafile, -2, -2, NULL);
    SetViewportOrgEx(hdcMetafile, 20, 20, NULL);
    SetWindowExtEx(hdcMetafile, 120, 120, NULL );
    SetViewportExtEx(hdcMetafile, 300, 300, NULL );

    /* Force Win9x to update DC state */
    SetPixelV(hdcMetafile, 50, 50, 0);

    ret = SaveDC(hdcMetafile);
    ok(ret == 3, "ret = %d\n", ret);

    SetWindowOrgEx(hdcMetafile, -3, -3, NULL);
    SetViewportOrgEx(hdcMetafile, 30, 30, NULL);
    SetWindowExtEx(hdcMetafile, 200, 200, NULL );
    SetViewportExtEx(hdcMetafile, 400, 400, NULL );

    /* Force Win9x to update DC state */
    SetPixelV(hdcMetafile, 50, 50, 0);

    ret = RestoreDC(hdcMetafile, -1);
    ok(ret, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 3, "ret = %d\n", ret);

    ret = RestoreDC(hdcMetafile, 1);
    ok(ret, "ret = %d\n", ret);

    SetWindowOrgEx(hdcMetafile, -4, -4, NULL);
    SetViewportOrgEx(hdcMetafile, 40, 40, NULL);
    SetWindowExtEx(hdcMetafile, 500, 500, NULL );
    SetViewportExtEx(hdcMetafile, 50, 50, NULL );

    /* Force Win9x to update DC state */
    SetPixelV(hdcMetafile, 50, 50, 0);

    ret = SaveDC(hdcMetafile);
    ok(ret == 1, "ret = %d\n", ret);

    ret = SaveDC(hdcMetafile);
    ok(ret == 2, "ret = %d\n", ret);

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %d\n", GetLastError());

    ret = EnumEnhMetaFile(hdcDisplay, hMetafile, savedc_emf_enum_proc, 0, &rc);
    ok( ret == 1, "EnumEnhMetaFile rets %d\n", ret);

    ret = DeleteEnhMetaFile(hMetafile);
    ok( ret, "DeleteEnhMetaFile error %d\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok( ret, "ReleaseDC error %d\n", GetLastError());
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
    trace("hdc %p, mr->rdFunction %04x, mr->rdSize %u, param %p\n",
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
    ok( ret && rd_size == mfsize, "ReadFile: error %d\n", GetLastError());

    CloseHandle(hfile);

    ok(mfsize == bsize, "%s: mfsize=%d, bsize=%d.\n", desc, mfsize, bsize);

    if (mfsize != bsize)
        return -1;

    diff = 0;
    for (i=0; i<bsize; i++)
    {
        if (buf[i] != bits[i])
            diff++;
    }
    ok(diff == 0, "%s: mfsize=%d, bsize=%d, diff=%d\n",
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
    ok (mfsize > 0, "%s: GetEnhMetaFileBits error %d\n", desc, GetLastError());

    printf("EMF %s has records:\n", desc);

    emf = buf;
    offset = 0;
    while(offset < mfsize)
    {
        EMR *emr = (EMR *)(emf + offset);
        printf("emr->iType %d, emr->nSize %u\n", emr->iType, emr->nSize);
        /*trace("emr->iType 0x%04lx, emr->nSize 0x%04lx\n", emr->iType, emr->nSize);*/
        offset += emr->nSize;
    }
}

static void dump_emf_record(const ENHMETARECORD *emr, const char *desc)
{
    const BYTE *buf;
    DWORD i;

    if (!winetest_debug) return;

    printf ("%s: EMF record %u has bits:\n{\n", desc, emr->iType);
    buf = (const BYTE *)emr;
    for (i = 0; i < emr->nSize; i++)
    {
        printf ("0x%02x", buf[i]);
        if (i == emr->nSize - 1)
            printf ("\n");
        else if (i % 8 == 7)
            printf (",\n");
        else
            printf (", ");
    }
    printf ("};\n");
}

static void dump_EMREXTTEXTOUT(const EMREXTTEXTOUTW *eto)
{
    trace("rclBounds %d,%d - %d,%d\n", eto->rclBounds.left, eto->rclBounds.top,
          eto->rclBounds.right, eto->rclBounds.bottom);
    trace("iGraphicsMode %u\n", eto->iGraphicsMode);
    trace("exScale: %f\n", eto->exScale);
    trace("eyScale: %f\n", eto->eyScale);
    trace("emrtext.ptlReference %d,%d\n", eto->emrtext.ptlReference.x, eto->emrtext.ptlReference.y);
    trace("emrtext.nChars %u\n", eto->emrtext.nChars);
    trace("emrtext.offString %#x\n", eto->emrtext.offString);
    trace("emrtext.fOptions %#x\n", eto->emrtext.fOptions);
    trace("emrtext.rcl %d,%d - %d,%d\n", eto->emrtext.rcl.left, eto->emrtext.rcl.top,
          eto->emrtext.rcl.right, eto->emrtext.rcl.bottom);
    trace("emrtext.offDx %#x\n", eto->emrtext.offDx);
}

static BOOL match_emf_record(const ENHMETARECORD *emr1, const ENHMETARECORD *emr2,
                             const char *desc, BOOL ignore_scaling, BOOL todo)
{
    int diff;

    if (emr1->iType != emr2->iType && todo)
    {
        todo_wine
        {
            ok(emr1->iType == emr2->iType, "%s: emr->iType %u != %u\n",
               desc, emr1->iType, emr2->iType);
        }
    }
    else
        ok(emr1->iType == emr2->iType, "%s: emr->iType %u != %u\n",
           desc, emr1->iType, emr2->iType);

    if (emr1->nSize != emr2->nSize && todo)
    {
        todo_wine
        {
            ok(emr1->nSize == emr2->nSize, "%s: emr->nSize %u != %u\n",
               desc, emr1->nSize, emr2->nSize);
        }
    }
    else
        ok(emr1->nSize == emr2->nSize, "%s: emr->nSize %u != %u\n",
           desc, emr1->nSize, emr2->nSize);

    /* iType and nSize mismatches are fatal */
    if (emr1->iType != emr2->iType || emr1->nSize != emr2->nSize) return FALSE;

    /* contents of EMR_GDICOMMENT are not interesting */
    if (emr1->iType == EMR_GDICOMMENT) return TRUE;

    /* different Windows versions setup DC scaling differently when
     * converting an old style metafile to an EMF.
     */
    if (ignore_scaling && (emr1->iType == EMR_SETWINDOWEXTEX ||
                           emr1->iType == EMR_SETVIEWPORTEXTEX))
        return TRUE;

    if (emr1->iType == EMR_EXTTEXTOUTW || emr1->iType == EMR_EXTTEXTOUTA)
    {
        EMREXTTEXTOUTW *eto1, *eto2;

        eto1 = HeapAlloc(GetProcessHeap(), 0, emr1->nSize);
        memcpy(eto1, emr1, emr1->nSize);
        eto2 = HeapAlloc(GetProcessHeap(), 0, emr2->nSize);
        memcpy(eto2, emr2, emr2->nSize);

        /* different Windows versions setup DC scaling differently */
        eto1->exScale = eto1->eyScale = 0.0;
        eto2->exScale = eto2->eyScale = 0.0;

        diff = memcmp(eto1, eto2, emr1->nSize);
        if (diff)
        {
            dump_EMREXTTEXTOUT(eto1);
            dump_EMREXTTEXTOUT(eto2);
        }
        HeapFree(GetProcessHeap(), 0, eto1);
        HeapFree(GetProcessHeap(), 0, eto2);
    }
    else
        diff = memcmp(emr1, emr2, emr1->nSize);
    if (diff && todo)
    {
        todo_wine
            ok(diff == 0, "%s: contents of record %u don't match\n", desc, emr1->iType);
    }
    else
        ok(diff == 0, "%s: contents of record %u don't match\n", desc, emr1->iType);

    if (diff)
    {
        dump_emf_record(emr1, "expected bits");
        dump_emf_record(emr2, "actual bits");
    }

    return diff == 0 || todo; /* report all non-fatal record mismatches */
}

/* Compare the EMF produced by a test function with the
 * expected raw EMF data in "bits".
 * Return value is 0 for a perfect match,
 * -1 if lengths aren't equal,
 * otherwise returns the number of non-matching bytes.
 */
static int compare_emf_bits(const HENHMETAFILE mf, const unsigned char *bits,
                            UINT bsize, const char *desc,
                            BOOL ignore_scaling, BOOL todo)
{
    unsigned char buf[MF_BUFSIZE];
    UINT mfsize, offset;
    const ENHMETAHEADER *emh1, *emh2;

    mfsize = GetEnhMetaFileBits(mf, MF_BUFSIZE, buf);
    ok (mfsize > 0, "%s: GetEnhMetaFileBits error %d\n", desc, GetLastError());

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

    /* basic things must match */
    emh1 = (const ENHMETAHEADER *)bits;
    emh2 = (const ENHMETAHEADER *)buf;
    ok(emh1->iType == EMR_HEADER, "expected EMR_HEADER, got %u\n", emh1->iType);
    ok(emh1->nSize == sizeof(ENHMETAHEADER), "expected sizeof(ENHMETAHEADER), got %u\n", emh1->nSize);
    ok(emh2->nBytes == mfsize, "expected emh->nBytes %u, got %u\n", mfsize, emh2->nBytes);
    ok(emh1->dSignature == ENHMETA_SIGNATURE, "expected ENHMETA_SIGNATURE, got %u\n", emh1->dSignature);

    ok(emh1->iType == emh2->iType, "expected EMR_HEADER, got %u\n", emh2->iType);
    ok(emh1->nSize == emh2->nSize, "expected nSize %u, got %u\n", emh1->nSize, emh2->nSize);
    ok(emh1->dSignature == emh2->dSignature, "expected dSignature %u, got %u\n", emh1->dSignature, emh2->dSignature);
    if (todo && emh1->nBytes != emh2->nBytes)
    {
        todo_wine
            ok(emh1->nBytes == emh2->nBytes, "expected nBytes %u, got %u\n", emh1->nBytes, emh2->nBytes);
    }
    else
        ok(emh1->nBytes == emh2->nBytes, "expected nBytes %u, got %u\n", emh1->nBytes, emh2->nBytes);
    if (todo && emh1->nRecords != emh2->nRecords)
    {
        todo_wine
            ok(emh1->nRecords == emh2->nRecords, "expected nBytes %u, got %u\n", emh1->nRecords, emh2->nRecords);
    }
    else
        ok(emh1->nRecords == emh2->nRecords, "expected nBytes %u, got %u\n", emh1->nRecords, emh2->nRecords);

    offset = emh1->nSize;
    while (offset < emh1->nBytes)
    {
	const ENHMETARECORD *emr1 = (const ENHMETARECORD *)(bits + offset);
	const ENHMETARECORD *emr2 = (const ENHMETARECORD *)(buf + offset);

	trace("EMF record %u, size %u/record %u, size %u\n",
              emr1->iType, emr1->nSize, emr2->iType, emr2->nSize);

        if (!match_emf_record(emr1, emr2, desc, ignore_scaling, todo)) return -1;

	offset += emr1->nSize;
    }
    return 0;
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
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %d\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

/* Tests on metafile initialization */
    caps = GetDeviceCaps (hdcMetafile, TECHNOLOGY);
    ok (caps == DT_METAFILE,
        "GetDeviceCaps: TECHNOLOGY=%d != DT_METAFILE.\n", caps);

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %d\n", GetLastError());
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
    ok( ret, "DeleteMetaFile(%p) error %d\n", hMetafile, GetLastError());
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
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %d\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %d\n", GetLastError());
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
    ok(hmf_copy != 0, "CopyMetaFile error %d\n", GetLastError());

    type = GetObjectType(hmf_copy);
    ok(type == OBJ_METAFILE, "CopyMetaFile created object with type %d\n", type);

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %d\n", hMetafile, GetLastError());

    if (compare_mf_disk_bits(mf_name, MF_BLANK_BITS, sizeof(MF_BLANK_BITS), "mf_blank") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Blank");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf_copy);
    ok( ret, "DeleteMetaFile(%p) error %d\n", hmf_copy, GetLastError());

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
    ok(hmf != 0, "SetMetaFileBitsEx error %d\n", GetLastError());
    type = GetObjectType(hmf);
    ok(type == OBJ_METAFILE, "SetMetaFileBitsEx created object with type %d\n", type);

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %d\n", hmf, GetLastError());

    /* NULL data crashes XP SP1 */
    /*hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), NULL);*/

    /* Now with not zero size */
    SetLastError(0xdeadbeef);
    hmf = SetMetaFileBitsEx(0, MF_GRAPHICS_BITS);
    ok(!hmf, "SetMetaFileBitsEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "wrong error %d\n", GetLastError());

    /* Now with not even size */
    SetLastError(0xdeadbeef);
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS) - 1, MF_GRAPHICS_BITS);
    ok(!hmf, "SetMetaFileBitsEx should fail\n");
    ok(GetLastError() == 0xdeadbeef /* XP SP1 */, "wrong error %d\n", GetLastError());

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
    ok(GetLastError() == ERROR_INVALID_DATA, "wrong error %d\n", GetLastError());

    /* Now with corrupted mtSize field */
    memcpy(buf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS));
    mh = (METAHEADER *)buf;
    /* corruption of mtSize doesn't lead to a failure */
    mh->mtSize *= 2;
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), buf);
    ok(hmf != 0, "SetMetaFileBitsEx error %d\n", GetLastError());

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %d\n", hmf, GetLastError());

    /* Now with zeroed out mtSize field */
    memcpy(buf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS));
    mh = (METAHEADER *)buf;
    /* zeroing mtSize doesn't lead to a failure */
    mh->mtSize = 0;
    hmf = SetMetaFileBitsEx(sizeof(MF_GRAPHICS_BITS), buf);
    ok(hmf != 0, "SetMetaFileBitsEx error %d\n", GetLastError());

    if (compare_mf_bits(hmf, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS), "mf_Graphics") != 0)
    {
        dump_mf_bits(hmf, "mf_Graphics");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hmf);
    ok(ret, "DeleteMetaFile(%p) error %d\n", hmf, GetLastError());
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
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %d\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    ret = MoveToEx(hdcMetafile, 1, 1, NULL);
    ok( ret, "MoveToEx error %d.\n", GetLastError());
    ret = LineTo(hdcMetafile, 2, 2);
    ok( ret, "LineTo error %d.\n", GetLastError());
    ret = MoveToEx(hdcMetafile, 1, 1, &oldpoint);
    ok( ret, "MoveToEx error %d.\n", GetLastError());

/* oldpoint gets garbage under Win XP, so the following test would
 * work under Wine but fails under Windows:
 *
 *   ok((oldpoint.x == 2) && (oldpoint.y == 2),
 *       "MoveToEx: (x, y) = (%ld, %ld), should be (2, 2).\n",
 *       oldpoint.x, oldpoint.y);
 */

    ret = Ellipse(hdcMetafile, 0, 0, 2, 2);
    ok( ret, "Ellipse error %d.\n", GetLastError());

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %d\n", GetLastError());
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_GRAPHICS_BITS, sizeof(MF_GRAPHICS_BITS),
        "mf_Graphics") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Graphics");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile(%p) error %d\n",
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
    ok((HBITMAP)orig_lb->lbHatch != NULL, "CreateBitmap error %d.\n", GetLastError());

    hBrush = CreateBrushIndirect (orig_lb);
    ok(hBrush != 0, "CreateBrushIndirect error %d\n", GetLastError());

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateMetaFileA error %d\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    hBrush = SelectObject(hdcMetafile, hBrush);
    ok(hBrush != 0, "SelectObject error %d.\n", GetLastError());

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %d\n", GetLastError());
    ok(!GetObjectType(hdcMetafile), "CloseMetaFile has to destroy metafile hdc\n");

    if (compare_mf_bits (hMetafile, MF_PATTERN_BRUSH_BITS, sizeof(MF_PATTERN_BRUSH_BITS),
        "mf_Pattern_Brush") != 0)
    {
        dump_mf_bits(hMetafile, "mf_Pattern_Brush");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok( ret, "DeleteMetaFile error %d\n", GetLastError());
    ret = DeleteObject(hBrush);
    ok( ret, "DeleteObject(HBRUSH) error %d\n", GetLastError());
    ret = DeleteObject((HBITMAP)orig_lb->lbHatch);
    ok( ret, "DeleteObject(HBITMAP) error %d\n",
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
    ok(hdcMetafile != 0, "CreateMetaFileA(NULL) error %d\n", GetLastError());
    trace("hdcMetafile %p\n", hdcMetafile);

    ret = BeginPath(hdcMetafile);
    ok(!ret, "BeginPath on metafile DC should fail\n");

    ret = ExtTextOutA(hdcMetafile, 11, 22, 0, NULL, "Test", 4, dx);
    ok(ret, "ExtTextOut error %d\n", GetLastError());

    ret = EndPath(hdcMetafile);
    ok(!ret, "EndPath on metafile DC should fail\n");

    hMetafile = CloseMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseMetaFile error %d\n", GetLastError());

    if (compare_mf_bits(hMetafile, MF_TEXTOUT_ON_PATH_BITS, sizeof(MF_TEXTOUT_ON_PATH_BITS),
        "mf_TextOut_on_path") != 0)
    {
        dump_mf_bits(hMetafile, "mf_TextOut_on_path");
        EnumMetaFile(0, hMetafile, mf_enum_proc, 0);
    }

    ret = DeleteMetaFile(hMetafile);
    ok(ret, "DeleteMetaFile(%p) error %d\n", hMetafile, GetLastError());
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
    ok(hwnd != 0, "CreateWindowExA error %d\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %d\n", GetLastError());

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %d\n", GetLastError());

    ret = BeginPath(hdcMetafile);
    ok(ret, "BeginPath error %d\n", GetLastError());

    ret = ExtTextOutA(hdcMetafile, 11, 22, 0, NULL, "Test", 4, dx);
    ok(ret, "ExtTextOut error %d\n", GetLastError());

    ret = EndPath(hdcMetafile);
    ok(ret, "EndPath error %d\n", GetLastError());

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %d\n", GetLastError());

    /* this doesn't succeed yet: EMF has correct size, all EMF records
     * are there, but their contents don't match for different reasons.
     */
    if (compare_emf_bits(hMetafile, EMF_TEXTOUT_ON_PATH_BITS, sizeof(EMF_TEXTOUT_ON_PATH_BITS),
        "emf_TextOut_on_path", FALSE, FALSE) != 0)
    {
        dump_emf_bits(hMetafile, "emf_TextOut_on_path");
        dump_emf_records(hMetafile, "emf_TextOut_on_path");
    }

    ret = DeleteEnhMetaFile(hMetafile);
    ok(ret, "DeleteEnhMetaFile error %d\n", GetLastError());
    ret = ReleaseDC(hwnd, hdcDisplay);
    ok(ret, "ReleaseDC error %d\n", GetLastError());
    DestroyWindow(hwnd);
}

static const unsigned char EMF_CLIPPING[] =
{
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0xd0, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x7c, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x60, 0xcc, 0x05, 0x00,
    0xe0, 0x93, 0x04, 0x00, 0x36, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00
};

static void translate( POINT *pt, UINT count, const XFORM *xform )
{
    while (count--)
    {
        FLOAT x = (FLOAT)pt->x;
        FLOAT y = (FLOAT)pt->y;
        pt->x = (LONG)floor( x * xform->eM11 + y * xform->eM21 + xform->eDx + 0.5 );
        pt->y = (LONG)floor( x * xform->eM12 + y * xform->eM22 + xform->eDy + 0.5 );
        pt++;
    }
}

/* Compare rectangles allowing rounding errors */
static BOOL is_equal_rect(const RECT *rc1, const RECT *rc2)
{
    return abs(rc1->left - rc2->left) <= 1 &&
           abs(rc1->top - rc2->top) <= 1 &&
           abs(rc1->right - rc2->right) <= 1 &&
           abs(rc1->bottom - rc2->bottom) <= 1;
}

static int CALLBACK clip_emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
                                       const ENHMETARECORD *emr, int n_objs, LPARAM param)
{
    if (emr->iType == EMR_EXTSELECTCLIPRGN)
    {
	const EMREXTSELECTCLIPRGN *clip = (const EMREXTSELECTCLIPRGN *)emr;
        union _rgn
        {
            RGNDATA data;
            char buf[sizeof(RGNDATAHEADER) + sizeof(RECT)];
        };
        const union _rgn *rgn1;
        union _rgn rgn2;
        RECT rect, rc_transformed;
        const RECT *rc = (const RECT *)param;
        HRGN hrgn;
        XFORM xform;
        INT ret;
        BOOL is_win9x;

        trace("EMR_EXTSELECTCLIPRGN: cbRgnData %#x, iMode %u\n",
               clip->cbRgnData, clip->iMode);

        ok(clip->iMode == RGN_COPY, "expected RGN_COPY, got %u\n", clip->iMode);
        ok(clip->cbRgnData >= sizeof(RGNDATAHEADER) + sizeof(RECT),
           "too small data block: %u bytes\n", clip->cbRgnData);
        if (clip->cbRgnData < sizeof(RGNDATAHEADER) + sizeof(RECT))
            return 0;

        rgn1 = (const union _rgn *)clip->RgnData;

        trace("size %u, type %u, count %u, rgn size %u, bound (%d,%d-%d,%d)\n",
              rgn1->data.rdh.dwSize, rgn1->data.rdh.iType,
              rgn1->data.rdh.nCount, rgn1->data.rdh.nRgnSize,
              rgn1->data.rdh.rcBound.left, rgn1->data.rdh.rcBound.top,
              rgn1->data.rdh.rcBound.right, rgn1->data.rdh.rcBound.bottom);

        ok(EqualRect(&rgn1->data.rdh.rcBound, rc), "rects don't match\n");

        rect = *(const RECT *)rgn1->data.Buffer;
        trace("rect (%d,%d-%d,%d)\n", rect.left, rect.top, rect.right, rect.bottom);
        ok(EqualRect(&rect, rc), "rects don't match\n");

        ok(rgn1->data.rdh.dwSize == sizeof(rgn1->data.rdh), "expected sizeof(rdh), got %u\n", rgn1->data.rdh.dwSize);
        ok(rgn1->data.rdh.iType == RDH_RECTANGLES, "expected RDH_RECTANGLES, got %u\n", rgn1->data.rdh.iType);
        ok(rgn1->data.rdh.nCount == 1, "expected 1, got %u\n", rgn1->data.rdh.nCount);
        ok(rgn1->data.rdh.nRgnSize == sizeof(RECT),  "expected sizeof(RECT), got %u\n", rgn1->data.rdh.nRgnSize);

        hrgn = CreateRectRgn(0, 0, 0, 0);

        memset(&xform, 0, sizeof(xform));
        SetLastError(0xdeadbeef);
        ret = GetWorldTransform(hdc, &xform);
        is_win9x = !ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED;
        if (!is_win9x)
            ok(ret, "GetWorldTransform error %u\n", GetLastError());

        trace("xform.eM11 %f, xform.eM22 %f\n", xform.eM11, xform.eM22);

        ret = GetClipRgn(hdc, hrgn);
        ok(ret == 0, "GetClipRgn returned %d, expected 0\n", ret);

        PlayEnhMetaFileRecord(hdc, handle_table, emr, n_objs);

        ret = GetClipRgn(hdc, hrgn);
        ok(ret == 1, "GetClipRgn returned %d, expected 0\n", ret);

        /* Win9x returns empty clipping region */
        if (is_win9x) return 1;

        ret = GetRegionData(hrgn, 0, NULL);
        ok(ret == sizeof(rgn2.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %u\n", ret);

        ret = GetRegionData(hrgn, sizeof(rgn2), &rgn2.data);

        trace("size %u, type %u, count %u, rgn size %u, bound (%d,%d-%d,%d)\n",
              rgn2.data.rdh.dwSize, rgn2.data.rdh.iType,
              rgn2.data.rdh.nCount, rgn2.data.rdh.nRgnSize,
              rgn2.data.rdh.rcBound.left, rgn2.data.rdh.rcBound.top,
              rgn2.data.rdh.rcBound.right, rgn2.data.rdh.rcBound.bottom);

        rect = rgn2.data.rdh.rcBound;
        rc_transformed = *rc;
        translate((POINT *)&rc_transformed, 2, &xform);
        trace("transformed (%d,%d-%d,%d)\n", rc_transformed.left, rc_transformed.top,
              rc_transformed.right, rc_transformed.bottom);
        ok(is_equal_rect(&rect, &rc_transformed), "rects don't match\n");

        rect = *(const RECT *)rgn2.data.Buffer;
        trace("rect (%d,%d-%d,%d)\n", rect.left, rect.top, rect.right, rect.bottom);
        rc_transformed = *rc;
        translate((POINT *)&rc_transformed, 2, &xform);
        trace("transformed (%d,%d-%d,%d)\n", rc_transformed.left, rc_transformed.top,
              rc_transformed.right, rc_transformed.bottom);
        ok(is_equal_rect(&rect, &rc_transformed), "rects don't match\n");

        ok(rgn2.data.rdh.dwSize == sizeof(rgn1->data.rdh), "expected sizeof(rdh), got %u\n", rgn2.data.rdh.dwSize);
        ok(rgn2.data.rdh.iType == RDH_RECTANGLES, "expected RDH_RECTANGLES, got %u\n", rgn2.data.rdh.iType);
        ok(rgn2.data.rdh.nCount == 1, "expected 1, got %u\n", rgn2.data.rdh.nCount);
        ok(rgn2.data.rdh.nRgnSize == sizeof(RECT),  "expected sizeof(RECT), got %u\n", rgn2.data.rdh.nRgnSize);

        DeleteObject(hrgn);
    }
    return 1;
}

static void test_emf_clipping(void)
{
    static const RECT rc = { 0, 0, 100, 100 };
    RECT rc_clip = { 100, 100, 1024, 1024 };
    HWND hwnd;
    HDC hdc;
    HENHMETAFILE hemf;
    HRGN hrgn;
    INT ret;

    SetLastError(0xdeadbeef);
    hdc = CreateEnhMetaFileA(0, NULL, NULL, NULL);
    ok(hdc != 0, "CreateEnhMetaFileA error %d\n", GetLastError());

    /* Need to write something to the emf, otherwise Windows won't play it back */
    LineTo(hdc, 1, 1);

    hrgn = CreateRectRgn(rc_clip.left, rc_clip.top, rc_clip.right, rc_clip.bottom);
    ret = SelectClipRgn(hdc, hrgn);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);

    SetLastError(0xdeadbeef);
    hemf = CloseEnhMetaFile(hdc);
    ok(hemf != 0, "CloseEnhMetaFile error %d\n", GetLastError());

    if (compare_emf_bits(hemf, EMF_CLIPPING, sizeof(EMF_CLIPPING),
        "emf_clipping", FALSE, FALSE) != 0)
    {
        dump_emf_bits(hemf, "emf_clipping");
        dump_emf_records(hemf, "emf_clipping");
    }

    DeleteObject(hrgn);

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %d\n", GetLastError());

    hdc = GetDC(hwnd);

    ret = EnumEnhMetaFile(hdc, hemf, clip_emf_enum_proc, &rc_clip, &rc);
    ok(ret, "EnumEnhMetaFile error %d\n", GetLastError());

    DeleteEnhMetaFile(hemf);
    ReleaseDC(hwnd, hdc);
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
    trace("EMF record: iType %d, nSize %d, (%d,%d)-(%d,%d)\n",
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
            ok(lpMFP->mm == MM_ANISOTROPIC, "mm=%d\n", lpMFP->mm);
            
            x0 = MulDiv(0, GetDeviceCaps(hdc, HORZSIZE) * 100, GetDeviceCaps(hdc, HORZRES));
            y0 = MulDiv(0, GetDeviceCaps(hdc, VERTSIZE) * 100, GetDeviceCaps(hdc, VERTRES));
            x1 = MulDiv(10, GetDeviceCaps(hdc, HORZSIZE) * 100, GetDeviceCaps(hdc, HORZRES));
            y1 = MulDiv(10, GetDeviceCaps(hdc, VERTSIZE) * 100, GetDeviceCaps(hdc, VERTRES));
        }
        ok(mapping[0].x == x0 && mapping[0].y == y0 && mapping[1].x == x1 && mapping[1].y == y1,
            "(%d,%d)->(%d,%d), expected (%d,%d)->(%d,%d)\n",
            mapping[0].x, mapping[0].y, mapping[1].x, mapping[1].y,
            x0, y0, x1, y1);
    }
    return TRUE;
}

static HENHMETAFILE create_converted_emf(const METAFILEPICT *mfp)
{
    HDC hdcMf;
    HMETAFILE hmf;
    HENHMETAFILE hemf;
    BOOL ret;
    UINT size;
    LPBYTE pBits;

    hdcMf = CreateMetaFile(NULL);
    ok(hdcMf != NULL, "CreateMetaFile failed with error %d\n", GetLastError());
    ret = LineTo(hdcMf, (INT)LINE_X, (INT)LINE_Y);
    ok(ret, "LineTo failed with error %d\n", GetLastError());
    hmf = CloseMetaFile(hdcMf);
    ok(hmf != NULL, "CloseMetaFile failed with error %d\n", GetLastError());

    if (compare_mf_bits (hmf, MF_LINETO_BITS, sizeof(MF_LINETO_BITS), "mf_LineTo") != 0)
    {
        dump_mf_bits(hmf, "mf_LineTo");
        EnumMetaFile(0, hmf, mf_enum_proc, 0);
    }

    size = GetMetaFileBitsEx(hmf, 0, NULL);
    ok(size, "GetMetaFileBitsEx failed with error %d\n", GetLastError());
    pBits = HeapAlloc(GetProcessHeap(), 0, size);
    GetMetaFileBitsEx(hmf, size, pBits);
    DeleteMetaFile(hmf);
    hemf = SetWinMetaFileBits(size, pBits, NULL, mfp);
    HeapFree(GetProcessHeap(), 0, pBits);
    return hemf;
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
                             "emf_LineTo MM_ANISOTROPIC", TRUE, FALSE) != 0)
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
                             "emf_LineTo MM_TEXT", TRUE, FALSE) != 0)
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
                             "emf_LineTo NULL", TRUE, FALSE) != 0)
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
      sprintf(buf, "mm=%s, xExt=%d, yExt=%d", mm_str, xExt, yExt);
      msg = buf;
    }

    ok(rclBounds.left == rclBoundsExpected->left, "rclBounds.left: Expected %d, got %d (%s)\n", rclBoundsExpected->left, rclBounds.left, msg);
    ok(rclBounds.top == rclBoundsExpected->top, "rclBounds.top: Expected %d, got %d (%s)\n", rclBoundsExpected->top, rclBounds.top, msg);
    ok(rclBounds.right == rclBoundsExpected->right, "rclBounds.right: Expected %d, got %d (%s)\n", rclBoundsExpected->right, rclBounds.right, msg);
    ok(rclBounds.bottom == rclBoundsExpected->bottom, "rclBounds.bottom: Expected %d, got %d (%s)\n", rclBoundsExpected->bottom, rclBounds.bottom, msg);
    ok(rclFrame.left == rclFrameExpected->left, "rclFrame.left: Expected %d, got %d (%s)\n", rclFrameExpected->left, rclFrame.left, msg);
    ok(rclFrame.top == rclFrameExpected->top, "rclFrame.top: Expected %d, got %d (%s)\n", rclFrameExpected->top, rclFrame.top, msg);
    ok(rclFrame.right == rclFrameExpected->right, "rclFrame.right: Expected %d, got %d (%s)\n", rclFrameExpected->right, rclFrame.right, msg);
    ok(rclFrame.bottom == rclFrameExpected->bottom, "rclFrame.bottom: Expected %d, got %d (%s)\n", rclFrameExpected->bottom, rclFrame.bottom, msg);
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

  buffer = HeapAlloc(GetProcessHeap(), 0, buffer_size);
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

  /* Allow 1 mm difference (rounding errors) */
  diffx = rclBoundsAnisotropic.right - GetDeviceCaps(dc, HORZRES) / 2;
  diffy = rclBoundsAnisotropic.bottom - GetDeviceCaps(dc, VERTRES) / 2;
  if (diffx < 0) diffx = -diffx;
  if (diffy < 0) diffy = -diffy;
  todo_wine
  {
  ok(diffx <= 1 && diffy <= 1,
     "SetWinMetaFileBits (MM_ANISOTROPIC): Reference bounds: The whole device surface must be used (%dx%d), but got (%dx%d)\n",
     GetDeviceCaps(dc, HORZRES) / 2, GetDeviceCaps(dc, VERTRES) / 2, rclBoundsAnisotropic.right, rclBoundsAnisotropic.bottom);
  }

  /* Allow 1 mm difference (rounding errors) */
  diffx = rclFrameAnisotropic.right / 100 - GetDeviceCaps(dc, HORZSIZE) / 2;
  diffy = rclFrameAnisotropic.bottom / 100 - GetDeviceCaps(dc, VERTSIZE) / 2;
  if (diffx < 0) diffx = -diffx;
  if (diffy < 0) diffy = -diffy;
  todo_wine
  {
  ok(diffx <= 1 && diffy <= 1,
     "SetWinMetaFileBits (MM_ANISOTROPIC): Reference frame: The whole device surface must be used (%dx%d), but got (%dx%d)\n",
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

    if(!pGdiIsMetaPrintDC || !pGdiIsMetaFileDC || !pGdiIsPlayMetafileDC)
    {
        win_skip("Needed GdiIs* functions are not available\n");
        return;
    }

    /* try with nothing */
    ok(!pGdiIsMetaPrintDC(NULL), "ismetaprint with NULL parameter\n");
    ok(!pGdiIsMetaFileDC(NULL), "ismetafile with NULL parameter\n");
    ok(!pGdiIsPlayMetafileDC(NULL), "isplaymetafile with NULL parameter\n");

    /* try with a metafile */
    hmfDC = CreateMetaFile(NULL);
    ok(!pGdiIsMetaPrintDC(hmfDC), "ismetaprint on metafile\n");
    ok(pGdiIsMetaFileDC(hmfDC), "ismetafile on metafile\n");
    ok(!pGdiIsPlayMetafileDC(hmfDC), "isplaymetafile on metafile\n");
    DeleteMetaFile(CloseMetaFile(hmfDC));

    /* try with an enhanced metafile */
    hdc = GetDC(NULL);
    hemfDC = CreateEnhMetaFileW(hdc, NULL, &rect, NULL);
    ok(hemfDC != NULL, "failed to create emf\n");

    ok(!pGdiIsMetaPrintDC(hemfDC), "ismetaprint on emf\n");
    ok(pGdiIsMetaFileDC(hemfDC), "ismetafile on emf\n");
    ok(!pGdiIsPlayMetafileDC(hemfDC), "isplaymetafile on emf\n");

    hemf = CloseEnhMetaFile(hemfDC);
    ok(hemf != NULL, "failed to close EMF\n");
    DeleteEnhMetaFile(hemf);
    ReleaseDC(NULL,hdc);
}

static void test_SetEnhMetaFileBits(void)
{
    BYTE data[256];
    HENHMETAFILE hemf;
    ENHMETAHEADER *emh;

    memset(data, 0xAA, sizeof(data));
    SetLastError(0xdeadbeef);
    hemf = SetEnhMetaFileBits(sizeof(data), data);
    ok(!hemf, "SetEnhMetaFileBits should fail\n");
    ok(GetLastError() == ERROR_INVALID_DATA ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x, WinMe */
       "expected ERROR_INVALID_DATA or ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    emh = (ENHMETAHEADER *)data;
    memset(emh, 0, sizeof(*emh));

    emh->iType = EMR_HEADER;
    emh->nSize = sizeof(*emh);
    emh->dSignature = ENHMETA_SIGNATURE;
    /* emh->nVersion  = 0x10000; XP doesn't care about version */
    emh->nBytes = sizeof(*emh);
    /* emh->nRecords = 1; XP doesn't care about records */
    emh->nHandles = 1; /* XP refuses to load a EMF if nHandles == 0 */

    SetLastError(0xdeadbeef);
    hemf = SetEnhMetaFileBits(emh->nBytes, data);
    ok(hemf != 0, "SetEnhMetaFileBits error %u\n", GetLastError());
    DeleteEnhMetaFile(hemf);

    /* XP refuses to load unaligned EMF */
    emh->nBytes++;
    SetLastError(0xdeadbeef);
    hemf = SetEnhMetaFileBits(emh->nBytes, data);
    ok(!hemf ||
       broken(hemf != NULL), /* Win9x, WinMe */
       "SetEnhMetaFileBits should fail\n");
    todo_wine
    ok(GetLastError() == 0xdeadbeef, "Expected deadbeef, got %u\n", GetLastError());
    DeleteEnhMetaFile(hemf);

    emh->dSignature = 0;
    emh->nBytes--;
    SetLastError(0xdeadbeef);
    hemf = SetEnhMetaFileBits(emh->nBytes, data);
    ok(!hemf ||
       broken(hemf != NULL), /* Win9x, WinMe */
       "SetEnhMetaFileBits should fail\n");
    todo_wine
    ok(GetLastError() == 0xdeadbeef, "Expected deadbeef, got %u\n", GetLastError());
    DeleteEnhMetaFile(hemf);
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
    test_emf_clipping();

    /* For metafile conversions */
    test_mf_conversions();
    test_SetWinMetaFileBits();

    test_gdiis();
    test_SetEnhMetaFileBits();
}
