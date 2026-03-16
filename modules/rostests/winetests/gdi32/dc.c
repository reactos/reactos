/*
 * Unit tests for dc functions
 *
 * Copyright (c) 2005 Huw Davies
 * Copyright (c) 2005,2016 Dmitry Timoshkov
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

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winspool.h"
#include "winerror.h"

#ifndef LAYOUT_LTR
#define LAYOUT_LTR 0
#endif

static void test_dc_values(void)
{
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    COLORREF color;
    int extra, attr;
    float limit;
    BOOL ret;

    ok( hdc != NULL, "CreateDC failed\n" );
    color = SetBkColor( hdc, 0x12345678 );
    ok( color == 0xffffff, "initial color %08lx\n", color );
    color = GetBkColor( hdc );
    ok( color == 0x12345678, "wrong color %08lx\n", color );
    color = SetBkColor( hdc, 0xffffffff );
    ok( color == 0x12345678, "wrong color %08lx\n", color );
    color = GetBkColor( hdc );
    ok( color == 0xffffffff, "wrong color %08lx\n", color );
    color = SetBkColor( hdc, 0 );
    ok( color == 0xffffffff, "wrong color %08lx\n", color );
    color = GetBkColor( hdc );
    ok( color == 0, "wrong color %08lx\n", color );

    color = SetTextColor( hdc, 0xffeeddcc );
    ok( color == 0, "initial color %08lx\n", color );
    color = GetTextColor( hdc );
    ok( color == 0xffeeddcc, "wrong color %08lx\n", color );
    color = SetTextColor( hdc, 0xffffffff );
    ok( color == 0xffeeddcc, "wrong color %08lx\n", color );
    color = GetTextColor( hdc );
    ok( color == 0xffffffff, "wrong color %08lx\n", color );
    color = SetTextColor( hdc, 0 );
    ok( color == 0xffffffff, "wrong color %08lx\n", color );
    color = GetTextColor( hdc );
    ok( color == 0, "wrong color %08lx\n", color );

    extra = GetTextCharacterExtra( hdc );
    ok( extra == 0, "initial extra %d\n", extra );
    SetTextCharacterExtra( hdc, 123 );
    extra = GetTextCharacterExtra( hdc );
    ok( extra == 123, "initial extra %d\n", extra );
    SetMapMode( hdc, MM_LOMETRIC );
    extra = GetTextCharacterExtra( hdc );
    ok( extra == 123, "initial extra %d\n", extra );
    SetMapMode( hdc, MM_TEXT );
    extra = GetTextCharacterExtra( hdc );
    ok( extra == 123, "initial extra %d\n", extra );

    SetLastError(0xdeadbeef);
    attr = SetBkMode(ULongToHandle(0xdeadbeef), OPAQUE);
    ok(!attr, "attr = %x\n", attr);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() = %lu\n", GetLastError());

    attr = GetBkColor(ULongToHandle(0xdeadbeef));
    ok(attr == CLR_INVALID, "attr = %x\n", attr);

    SetLastError(0xdeadbeef);
    attr = GetDeviceCaps(ULongToHandle(0xdeadbeef), TECHNOLOGY);
    ok(!attr, "GetDeviceCaps rets %d\n", attr);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() = %lu\n", GetLastError());

    /* Miter limit */
    limit = 123.0f;
    ret = GetMiterLimit(hdc, &limit);
    ok(ret, "Unexpected return value.\n");
    ok(limit == 10.0f, "Unexpected default miter limit %f.\n", limit);

    limit = 456.0;
    ret = SetMiterLimit(hdc, 0.9f, &limit);
    ok(!ret, "Unexpected return value.\n");
    ok(limit == 456.0f, "Unexpected default miter limit %f.\n", limit);

    limit = 0.0;
    ret = SetMiterLimit(hdc, 1.0f, &limit);
    ok(ret, "Unexpected return value.\n");
    ok(limit == 10.0f, "Unexpected default miter limit %f.\n", limit);

    DeleteDC( hdc );
}

static void test_savedc_2(void)
{
    char buffer[100];
    RGNDATA *rgndata = (RGNDATA *)buffer;
    HWND hwnd;
    HDC hdc;
    HRGN hrgn;
    RECT rc, rc_clip;
    int ret;

    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    assert(hwnd != 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hrgn = CreateRectRgn(0, 0, 0, 0);
    assert(hrgn != 0);

    hdc = GetDC(hwnd);
    ok(hdc != NULL, "GetDC failed\n");

    ret = GetClipBox(hdc, &rc_clip);
    /* all versions of Windows return SIMPLEREGION despite returning an empty region */
    todo_wine ok(ret == NULLREGION || broken(ret == SIMPLEREGION), "wrong region type %d\n", ret);
    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "GetClipRgn returned %d instead of 0\n", ret);
    ret = GetRgnBox(hrgn, &rc);
    ok(ret == NULLREGION, "GetRgnBox returned %d %s instead of NULLREGION\n",
       ret, wine_dbgstr_rect(&rc));
    ret = GetRegionData(hrgn, sizeof(buffer), rgndata);
    ok(ret == sizeof(RGNDATAHEADER), "got %u\n", ret);
    ok(!rgndata->rdh.nCount, "got %lu rectangles\n", rgndata->rdh.nCount);
    SetRect(&rc, 0, 0, 100, 100);
    ok(EqualRect(&rc, &rc_clip), "rects are not equal: %s - %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&rc_clip));

    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);

    ret = IntersectClipRect(hdc, 0, 0, 50, 50);
    /* all versions of Windows return COMPLEXREGION despite the region comprising one rectangle */
    ok(ret == SIMPLEREGION || broken(ret == COMPLEXREGION), "wrong region type %d\n", ret);
    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "GetClipRgn returned %d instead of 1\n", ret);
    ret = GetRegionData(hrgn, sizeof(buffer), rgndata);
    ok(ret == sizeof(RGNDATAHEADER) + sizeof(RECT), "got %u\n", ret);
    ok(rgndata->rdh.nCount == 1, "got %lu rectangles\n", rgndata->rdh.nCount);
    SetRect(&rc, 0, 0, 50, 50);
    ok(EqualRect((RECT *)rgndata->Buffer, &rc), "got rect %s\n", wine_dbgstr_rect((RECT *)rgndata->Buffer));

    ret = GetClipBox(hdc, &rc_clip);
    ok(ret == SIMPLEREGION, "wrong region type %d\n", ret);
    SetRect(&rc, 0, 0, 50, 50);
    ok(EqualRect(&rc, &rc_clip), "rects are not equal: %s - %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&rc_clip));

    ret = RestoreDC(hdc, 1);
    ok(ret, "ret = %d\n", ret);

    ret = GetClipBox(hdc, &rc_clip);
    ok(ret == SIMPLEREGION, "wrong region type %d\n", ret);
    SetRect(&rc, 0, 0, 100, 100);
    ok(EqualRect(&rc, &rc_clip), "rects are not equal: %s - %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&rc_clip));

    DeleteObject(hrgn);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void test_savedc(void)
{
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    int ret;

    ok(hdc != NULL, "CreateDC rets %p\n", hdc);

    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -1);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, 1);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 3, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -2);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret);
    ret = RestoreDC(hdc, -2);
    ok(ret, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 1, "ret = %d\n", ret);
    ret = SaveDC(hdc);
    ok(ret == 2, "ret = %d\n", ret); 
    ret = RestoreDC(hdc, -4);
    ok(!ret, "ret = %d\n", ret);
    ret = RestoreDC(hdc, 3);
    ok(!ret, "ret = %d\n", ret);

    /* Under Win9x the following RestoreDC call succeeds and clears the save stack. */
    ret = RestoreDC(hdc, -3);
    ok(!ret ||
       broken(ret), /* Win9x */
       "ret = %d\n", ret);

    /* Trying to clear an empty save stack fails. */
    ret = RestoreDC(hdc, -3);
    ok(!ret, "ret = %d\n", ret);

    ret = SaveDC(hdc);
    ok(ret == 3 ||
       broken(ret == 1), /* Win9x */
       "ret = %d\n", ret);

    /* Under Win9x the following RestoreDC call succeeds and clears the save stack. */
    ret = RestoreDC(hdc, 0);
    ok(!ret ||
       broken(ret), /* Win9x */
       "ret = %d\n", ret);

    /* Trying to clear an empty save stack fails. */
    ret = RestoreDC(hdc, 0);
    ok(!ret, "ret = %d\n", ret);

    ret = RestoreDC(hdc, 1);
    ok(ret ||
       broken(!ret), /* Win9x */
       "ret = %d\n", ret);

    DeleteDC(hdc);

    SetLastError(0xdeadbeef);
    ret = SaveDC(ULongToHandle(0xdeadbeef));
    ok(!ret, "SaveDC returned %u\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() = %lu\n", GetLastError());
}

static void test_GdiConvertToDevmodeW(void)
{
    DEVMODEW * (WINAPI *pGdiConvertToDevmodeW)(const DEVMODEA *);
    DEVMODEA dmA;
    DEVMODEW *dmW;
    BOOL ret;

    pGdiConvertToDevmodeW = (void *)GetProcAddress(GetModuleHandleA("gdi32.dll"), "GdiConvertToDevmodeW");
    if (!pGdiConvertToDevmodeW)
    {
        win_skip("GdiConvertToDevmodeW is not available on this platform\n");
        return;
    }

    memset(&dmA, 0, sizeof(dmA));
    dmA.dmSize = sizeof(dmA);
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dmA);
    ok(ret, "EnumDisplaySettingsExA error %lu\n", GetLastError());
    ok(dmA.dmSize >= FIELD_OFFSET(DEVMODEA, dmICMMethod), "dmSize is too small: %04x\n", dmA.dmSize);
    ok(dmA.dmSize <= sizeof(DEVMODEA), "dmSize is too large: %04x\n", dmA.dmSize);

    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize >= FIELD_OFFSET(DEVMODEW, dmICMMethod), "dmSize is too small: %04x\n", dmW->dmSize);
    ok(dmW->dmSize <= sizeof(DEVMODEW), "dmSize is too large: %04x\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmFields) + sizeof(dmA.dmFields);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmFields) + sizeof(dmW->dmFields),
       "wrong size %u\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmICMMethod = DMICMMETHOD_NONE;
    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmICMMethod) + sizeof(dmA.dmICMMethod);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmICMMethod) + sizeof(dmW->dmICMMethod),
       "wrong size %u\n", dmW->dmSize);
    ok(dmW->dmICMMethod == DMICMMETHOD_NONE,
       "expected DMICMMETHOD_NONE, got %lu\n", dmW->dmICMMethod);
    HeapFree(GetProcessHeap(), 0, dmW);

    dmA.dmSize = 1024;
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmPanningHeight) + sizeof(dmW->dmPanningHeight),
       "wrong size %u\n", dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);

    SetLastError(0xdeadbeef);
    dmA.dmSize = 0;
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(!dmW, "GdiConvertToDevmodeW should fail\n");
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", GetLastError());

    /* this is the minimal dmSize that XP accepts */
    dmA.dmSize = FIELD_OFFSET(DEVMODEA, dmFields);
    dmW = pGdiConvertToDevmodeW(&dmA);
    ok(dmW->dmSize == FIELD_OFFSET(DEVMODEW, dmFields),
       "expected %04lx, got %04x\n", FIELD_OFFSET(DEVMODEW, dmFields), dmW->dmSize);
    HeapFree(GetProcessHeap(), 0, dmW);
}

static void test_device_caps( HDC hdc, HDC ref_dc, const char *descr, int scale )
{
    static const int caps[] =
    {
        DRIVERVERSION,
        TECHNOLOGY,
        HORZSIZE,
        VERTSIZE,
        HORZRES,
        VERTRES,
        BITSPIXEL,
        PLANES,
        NUMBRUSHES,
        NUMPENS,
        NUMMARKERS,
        NUMFONTS,
        NUMCOLORS,
        PDEVICESIZE,
        CURVECAPS,
        LINECAPS,
        POLYGONALCAPS,
        /* TEXTCAPS broken on printer DC on winxp */
        CLIPCAPS,
        RASTERCAPS,
        ASPECTX,
        ASPECTY,
        ASPECTXY,
        LOGPIXELSX,
        LOGPIXELSY,
        SIZEPALETTE,
        NUMRESERVED,
        COLORRES,
        PHYSICALWIDTH,
        PHYSICALHEIGHT,
        PHYSICALOFFSETX,
        PHYSICALOFFSETY,
        SCALINGFACTORX,
        SCALINGFACTORY,
        VREFRESH,
        DESKTOPVERTRES,
        DESKTOPHORZRES,
        BLTALIGNMENT,
        SHADEBLENDCAPS
    };
    unsigned int i;
    WORD ramp[3][256];
    BOOL ret;
    RECT rect;
    UINT type;

    if (GetObjectType( hdc ) == OBJ_METADC)
    {
        for (i = 0; i < ARRAY_SIZE(caps); i++)
            ok( GetDeviceCaps( hdc, caps[i] ) == (caps[i] == TECHNOLOGY ? DT_METAFILE : 0),
                "wrong caps on %s for %u: %u\n", descr, caps[i],
                GetDeviceCaps( hdc, caps[i] ) );

        type = GetClipBox( hdc, &rect );
        ok( type == ERROR, "GetClipBox returned %d on %s\n", type, descr );

        SetBoundsRect( hdc, NULL, DCB_RESET | DCB_ENABLE );
        SetMapMode( hdc, MM_TEXT );
        Rectangle( hdc, 2, 2, 5, 5 );
        type = GetBoundsRect( hdc, &rect, DCB_RESET );
        ok( !type, "GetBoundsRect succeeded on %s\n", descr );
        type = SetBoundsRect( hdc, &rect, DCB_RESET | DCB_ENABLE );
        ok( !type, "SetBoundsRect succeeded on %s\n", descr );
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE(caps); i++)
        {
            INT precision = 0;
            INT hdc_caps = GetDeviceCaps( hdc, caps[i] );

            switch (caps[i])
            {
            case HORZSIZE:
            case VERTSIZE:
                hdc_caps /= scale;
                precision = 1;
                break;
            case LOGPIXELSX:
            case LOGPIXELSY:
                hdc_caps *= scale;
                break;
            case VREFRESH:
                if (GetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASDISPLAY)
                    ok( hdc_caps > 0, "expected a positive value on %s, got %d\n", descr, hdc_caps );
                else
                    ok( hdc_caps == 0, "expected 0 on %s, got %d\n", descr, hdc_caps );
                break;
            }

            ok( abs(hdc_caps - GetDeviceCaps( ref_dc, caps[i] )) <= precision,
                "mismatched caps on %s for %u: %u/%u (scale %d)\n", descr, caps[i],
                hdc_caps, GetDeviceCaps( ref_dc, caps[i] ), scale );
        }

        type = GetBoundsRect( hdc, &rect, 0 );
        ok( type == DCB_RESET || broken(type == DCB_SET) /* XP */,
            "GetBoundsRect returned type %x for %s\n", type, descr );
        if (type == DCB_RESET)
            ok( rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0,
                "GetBoundsRect returned %s type %x for %s\n", wine_dbgstr_rect( &rect ),
                type, descr );
        type = SetBoundsRect( hdc, NULL, DCB_RESET | DCB_ENABLE );
        ok( type == (DCB_RESET | DCB_DISABLE) || broken(type == (DCB_SET | DCB_ENABLE)) /* XP */,
            "SetBoundsRect returned %x for %s (hdc type %ld)\n", type, descr, GetObjectType( hdc ) );

        SetMapMode( hdc, MM_TEXT );
        Rectangle( hdc, 2, 2, 4, 4 );
        type = GetBoundsRect( hdc, &rect, DCB_RESET );
        todo_wine_if (GetObjectType( hdc ) == OBJ_ENHMETADC || (GetObjectType( hdc ) == OBJ_DC && GetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASPRINTER))
            ok( rect.left == 2 && rect.top == 2 && rect.right == 4 && rect.bottom == 4 && type == DCB_SET,
                "GetBoundsRect returned %s type %x for %s\n", wine_dbgstr_rect( &rect ),
                type, descr );
    }

    type = GetClipBox( ref_dc, &rect );
    if (type != COMPLEXREGION && type != ERROR)  /* region can be complex on multi-monitor setups */
    {
        RECT ref_rect;

        ok( type == SIMPLEREGION, "GetClipBox returned %d on %s\n", type, descr );
        if (GetDeviceCaps( ref_dc, TECHNOLOGY ) == DT_RASDISPLAY)
        {
            todo_wine_if (GetSystemMetrics( SM_CXSCREEN ) != GetSystemMetrics( SM_CXVIRTUALSCREEN ))
                ok( GetDeviceCaps( ref_dc, DESKTOPHORZRES ) == GetSystemMetrics( SM_CXSCREEN ),
                    "Got DESKTOPHORZRES %d on %s, expected %d\n",
                    GetDeviceCaps( ref_dc, DESKTOPHORZRES ), descr, GetSystemMetrics( SM_CXSCREEN ) );

            todo_wine_if (GetSystemMetrics( SM_CYSCREEN ) != GetSystemMetrics( SM_CYVIRTUALSCREEN ))
                ok( GetDeviceCaps( ref_dc, DESKTOPVERTRES ) == GetSystemMetrics( SM_CYSCREEN ),
                    "Got DESKTOPVERTRES %d on %s, expected %d\n",
                    GetDeviceCaps( ref_dc, DESKTOPVERTRES ), descr, GetSystemMetrics( SM_CYSCREEN ) );

            SetRect( &ref_rect, GetSystemMetrics( SM_XVIRTUALSCREEN ), GetSystemMetrics( SM_YVIRTUALSCREEN ),
                     GetSystemMetrics( SM_XVIRTUALSCREEN ) + GetSystemMetrics( SM_CXVIRTUALSCREEN ),
                     GetSystemMetrics( SM_YVIRTUALSCREEN ) + GetSystemMetrics( SM_CYVIRTUALSCREEN ) );
        }
        else
        {
            ok( GetDeviceCaps( ref_dc, DESKTOPHORZRES ) == GetDeviceCaps( ref_dc, HORZRES ),
                "Got DESKTOPHORZRES %d on %s, expected %d\n",
                GetDeviceCaps( ref_dc, DESKTOPHORZRES ), descr, GetDeviceCaps( ref_dc, HORZRES ));
            ok( GetDeviceCaps( ref_dc, DESKTOPVERTRES ) == GetDeviceCaps( ref_dc, VERTRES ),
                "Got DESKTOPVERTRES %d on %s, expected %d\n",
                GetDeviceCaps( ref_dc, DESKTOPVERTRES ), descr, GetDeviceCaps( ref_dc, VERTRES ));
            SetRect( &ref_rect, 0, 0, GetDeviceCaps( ref_dc, DESKTOPHORZRES ),
                     GetDeviceCaps( ref_dc, DESKTOPVERTRES ) );
        }

        todo_wine_if (GetDeviceCaps( ref_dc, TECHNOLOGY ) == DT_RASDISPLAY && GetObjectType( hdc ) != OBJ_ENHMETADC &&
            (GetSystemMetrics( SM_XVIRTUALSCREEN ) || GetSystemMetrics( SM_YVIRTUALSCREEN )))
            ok( EqualRect( &rect, &ref_rect ), "GetClipBox returned %s on %s\n",
                wine_dbgstr_rect( &rect ), descr );
    }

    SetBoundsRect( ref_dc, NULL, DCB_RESET | DCB_ACCUMULATE );
    SetMapMode( ref_dc, MM_TEXT );
    Rectangle( ref_dc, 3, 3, 5, 5 );
    type = GetBoundsRect( ref_dc, &rect, DCB_RESET );
    /* it may or may not work on non-memory DCs */
    ok( (rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0 && type == DCB_RESET) ||
        (rect.left == 3 && rect.top == 3 && rect.right == 5 && rect.bottom == 5 && type == DCB_SET),
        "GetBoundsRect returned %s type %x on %s\n", wine_dbgstr_rect( &rect ), type, descr );

    if (GetObjectType( hdc ) == OBJ_MEMDC)
    {
        char buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        HBITMAP dib, old;

        memset( buffer, 0, sizeof(buffer) );
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = 16;
        info->bmiHeader.biHeight = 16;
        info->bmiHeader.biPlanes = 1;
        info->bmiHeader.biBitCount = 8;
        info->bmiHeader.biCompression = BI_RGB;
        dib = CreateDIBSection( ref_dc, info, DIB_RGB_COLORS, NULL, NULL, 0 );
        old = SelectObject( hdc, dib );

        for (i = 0; i < ARRAY_SIZE(caps); i++)
            ok( GetDeviceCaps( hdc, caps[i] ) == GetDeviceCaps( ref_dc, caps[i] ),
                "mismatched caps on %s and DIB for %u: %u/%u\n", descr, caps[i],
                GetDeviceCaps( hdc, caps[i] ), GetDeviceCaps( ref_dc, caps[i] ) );

        type = GetClipBox( hdc, &rect );
        ok( type == SIMPLEREGION, "GetClipBox returned %d on memdc for %s\n", type, descr );
        ok( rect.left == 0 && rect.top == 0 && rect.right == 16 && rect.bottom == 16,
            "GetClipBox returned %s on memdc for %s\n", wine_dbgstr_rect( &rect ), descr );

        SetBoundsRect( hdc, NULL, DCB_RESET | DCB_ENABLE );
        SetMapMode( hdc, MM_TEXT );
        Rectangle( hdc, 5, 5, 12, 14 );
        type = GetBoundsRect( hdc, &rect, DCB_RESET );
        ok( rect.left == 5 && rect.top == 5 && rect.right == 12 && rect.bottom == 14 && type == DCB_SET,
            "GetBoundsRect returned %s type %x on memdc for %s\n", wine_dbgstr_rect( &rect ),
            type, descr );

        SelectObject( hdc, old );
        DeleteObject( dib );
    }

    /* Memory DC, metafile DC and enhanced metafile DC support gamma ramp on Win10 1909+. Exclude
     * these types from tests as they return different results depending on Windows versions */
    if (GetObjectType( hdc ) != OBJ_MEMDC
        && GetObjectType( hdc ) != OBJ_METADC
        && GetObjectType( hdc ) != OBJ_ENHMETADC)
    {
        SetLastError( 0xdeadbeef );
        ret = GetDeviceGammaRamp( hdc, &ramp );
        if (GetObjectType( hdc ) != OBJ_DC || GetDeviceCaps( hdc, TECHNOLOGY ) == DT_RASPRINTER)
        {
            ok( !ret, "GetDeviceGammaRamp succeeded on %s (type %ld)\n", descr, GetObjectType( hdc ) );
            ok( GetLastError() == ERROR_INVALID_PARAMETER
                || broken(GetLastError() == 0xdeadbeef) /* nt4 */
                || broken(GetLastError() == NO_ERROR), /* Printer DC on Win10 1909+ */
                "wrong error %lu on %s\n", GetLastError(), descr );
        }
        else
        {
            ok( ret || broken(!ret) /* NT4 */, "GetDeviceGammaRamp failed on %s (type %ld), error %lu\n",
                descr, GetObjectType( hdc ), GetLastError() );
        }
    }

    /* restore hdc state */
    SetBoundsRect( hdc, NULL, DCB_RESET | DCB_DISABLE );
    SetBoundsRect( ref_dc, NULL, DCB_RESET | DCB_DISABLE );
}

static void test_CreateDC(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    WCHAR adapter_name[CCHDEVICENAME];
    DWORD i, j;
    HDC hdc;

    hdc = CreateDCW( NULL, NULL, NULL, NULL );
    ok( !hdc, "CreateDC succeeded\n" );

    hdc = CreateDCW( NULL, L"display", NULL, NULL );
    todo_wine ok( !hdc, "CreateDC succeeded\n" );

    hdc = CreateDCW( L"display", NULL, NULL, NULL );
    ok( hdc != NULL, "CreateDC failed\n" );
    DeleteDC( hdc );

    hdc = CreateDCW( L"display", L"deadbeef", NULL, NULL );
    ok( hdc != NULL, "CreateDC failed\n" );
    DeleteDC( hdc );

    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
    {
        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            hdc = CreateDCW( display_device.DeviceName, NULL, NULL, NULL );
            todo_wine ok( !hdc, "CreateDC succeeded\n" );

            hdc = CreateDCW( NULL, display_device.DeviceName, NULL, NULL );
            todo_wine ok( !hdc, "CreateDC succeeded\n" );
            continue;
        }

        hdc = CreateDCW( display_device.DeviceName, NULL, NULL, NULL );
        ok( hdc != NULL, "CreateDC failed %s\n", wine_dbgstr_w( display_device.DeviceName ) );
        DeleteDC( hdc );

        hdc = CreateDCW( NULL, display_device.DeviceName, NULL, NULL );
        ok( hdc != NULL, "CreateDC failed\n" );
        DeleteDC( hdc );

        hdc = CreateDCW( display_device.DeviceName, display_device.DeviceName, NULL, NULL );
        ok( hdc != NULL, "CreateDC failed\n" );
        DeleteDC( hdc );

        hdc = CreateDCW( display_device.DeviceName, L"deadbeef", NULL, NULL );
        ok( hdc != NULL, "CreateDC failed\n" );
        DeleteDC( hdc );

        lstrcpyW( adapter_name, display_device.DeviceName );
        for (j = 0; EnumDisplayDevicesW( adapter_name, j, &display_device, 0 ); ++j)
        {
            hdc = CreateDCW( display_device.DeviceName, NULL, NULL, NULL );
            ok( !hdc, "CreateDC succeeded\n" );

            hdc = CreateDCW( NULL, display_device.DeviceName, NULL, NULL );
            ok( !hdc, "CreateDC succeeded\n" );
        }
    }
}

static void test_CreateCompatibleDC(void)
{
    BOOL bRet;
    HDC hdc, hNewDC, hdcMetafile, screen_dc;
    HBITMAP bitmap;
    INT caps;
    DEVMODEA dm;

    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    bRet = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(bRet, "EnumDisplaySettingsEx failed\n");
    dm.dmScale = 200;
    dm.dmFields |= DM_SCALE;
    hdc = CreateDCA( "DISPLAY", NULL, NULL, &dm );

    screen_dc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
    test_device_caps( hdc, screen_dc, "display dc", 1 );
    ResetDCA( hdc, &dm );
    test_device_caps( hdc, screen_dc, "display dc", 1 );
    DeleteDC( hdc );

    /* Create a DC compatible with the screen */
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "CreateCompatibleDC returned %p\n", hdc);
    ok( SelectObject( hdc, bitmap ) != 0, "SelectObject failed\n" );
    caps = GetDeviceCaps( hdc, TECHNOLOGY );
    ok( caps == DT_RASDISPLAY, "wrong caps %u\n", caps );

    test_device_caps( hdc, screen_dc, "display dc", 1 );

    /* Delete this DC, this should succeed */
    bRet = DeleteDC(hdc);
    ok(bRet == TRUE, "DeleteDC returned %u\n", bRet);

    /* Try to create a DC compatible to the deleted DC. This has to fail */
    hNewDC = CreateCompatibleDC(hdc);
    ok(hNewDC == NULL, "CreateCompatibleDC returned %p\n", hNewDC);

    hdc = GetDC( 0 );
    hdcMetafile = CreateEnhMetaFileA(hdc, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA failed\n");
    hNewDC = CreateCompatibleDC( hdcMetafile );
    ok(hNewDC != NULL, "CreateCompatibleDC failed\n");
    ok( SelectObject( hNewDC, bitmap ) != 0, "SelectObject failed\n" );
    caps = GetDeviceCaps( hdcMetafile, TECHNOLOGY );
    ok( caps == DT_RASDISPLAY, "wrong caps %u\n", caps );
    test_device_caps( hdcMetafile, hdc, "enhmetafile dc", 1 );
    ResetDCA( hdcMetafile, &dm );
    test_device_caps( hdcMetafile, hdc, "enhmetafile dc", 1 );
    DeleteDC( hNewDC );
    DeleteEnhMetaFile( CloseEnhMetaFile( hdcMetafile ));
    ReleaseDC( 0, hdc );

    hdcMetafile = CreateMetaFileA(NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA failed\n");
    hNewDC = CreateCompatibleDC( hdcMetafile );
    ok(hNewDC == NULL, "CreateCompatibleDC succeeded\n");
    caps = GetDeviceCaps( hdcMetafile, TECHNOLOGY );
    ok( caps == DT_METAFILE, "wrong caps %u\n", caps );
    test_device_caps( hdcMetafile, screen_dc, "metafile dc", 1 );
    ResetDCA( hdcMetafile, &dm );
    test_device_caps( hdcMetafile, screen_dc, "metafile dc", 1 );
    DeleteMetaFile( CloseMetaFile( hdcMetafile ));

    DeleteObject( bitmap );
    DeleteDC( screen_dc );
}

static void test_DC_bitmap(void)
{
    PIXELFORMATDESCRIPTOR descr;
    HDC hdc, hdcmem;
    DWORD bits[64];
    HBITMAP hbmp, oldhbmp;
    COLORREF col;
    int i, bitspixel;
    int ret, ret2;

    /* fill bitmap data with b&w pattern */
    for( i = 0; i < 64; i++) bits[i] = i & 1 ? 0 : 0xffffff;

    hdc = GetDC(0);
    ok( hdc != NULL, "CreateDC rets %p\n", hdc);
    bitspixel = GetDeviceCaps( hdc, BITSPIXEL);
    /* create a memory dc */
    hdcmem = CreateCompatibleDC( hdc);
    ok( hdcmem != NULL, "CreateCompatibleDC rets %p\n", hdcmem);

    /* test DescribePixelFormat with descr == NULL */
    ret2 = DescribePixelFormat(hdcmem, 0, sizeof(descr), NULL);
    ok(ret2 > 0, "expected ret2 > 0, got %d\n", ret2);
    ret = DescribePixelFormat(hdcmem, 1, sizeof(descr), NULL);
    ok(ret == ret2, "expected ret == %d, got %d\n", ret2, ret);
    ret = DescribePixelFormat(hdcmem, 0x10000, sizeof(descr), NULL);
    ok(ret == ret2, "expected ret == %d, got %d\n", ret2, ret);

    /* test DescribePixelFormat with descr != NULL */
    memset(&descr, 0, sizeof(descr));
    ret = DescribePixelFormat(hdcmem, 0, sizeof(descr), &descr);
    ok(ret == 0, "expected ret == 0, got %d\n", ret);
    ok(descr.nSize == 0, "expected descr.nSize == 0, got %d\n", descr.nSize);

    memset(&descr, 0, sizeof(descr));
    ret = DescribePixelFormat(hdcmem, 1, sizeof(descr), &descr);
    ok(ret == ret2, "expected ret == %d, got %d\n", ret2, ret);
    ok(descr.nSize == sizeof(descr), "expected desc.nSize == sizeof(descr), got %d\n", descr.nSize);

    memset(&descr, 0, sizeof(descr));
    ret = DescribePixelFormat(hdcmem, 0x10000, sizeof(descr), &descr);
    ok(ret == 0, "expected ret == 0, got %d\n", ret);
    ok(descr.nSize == 0, "expected descr.nSize == 0, got %d\n", descr.nSize);

    /* test monochrome bitmap: should always work */
    hbmp = CreateBitmap(32, 32, 1, 1, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    ok( oldhbmp != NULL, "SelectObject returned NULL\n" ); /* a memdc always has a bitmap selected */
    col = GetPixel( hdcmem, 0, 0);
    ok( col == 0xffffff, "GetPixel returned %08lx, expected 00ffffff\n", col);
    col = GetPixel( hdcmem, 1, 1);
    ok( col == 0x000000, "GetPixel returned %08lx, expected 00000000\n", col);
    col = GetPixel( hdcmem, 100, 1);
    ok( col == CLR_INVALID, "GetPixel returned %08lx, expected ffffffff\n", col);
    SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 2 bits color depth, not likely to succeed */
    hbmp = CreateBitmap(16, 16, 1, 2, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel != 2)
        ok( !oldhbmp, "SelectObject of a bitmap with 2 bits/pixel should return  NULL\n");
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 16 bits color depth, might succeed */
    hbmp = CreateBitmap(6, 6, 1, 16, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel == 16) {
        ok( oldhbmp != NULL, "SelectObject returned NULL\n" );
        col = GetPixel( hdcmem, 0, 0);
        ok( col == 0xffffff,
            "GetPixel of a bitmap with 16 bits/pixel returned %08lx, expected 00ffffff\n", col);
        col = GetPixel( hdcmem, 1, 1);
        ok( col == 0x000000,
            "GetPixel of a bitmap with 16 bits/pixel returned returned %08lx, expected 00000000\n", col);
    }
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);

    /* test with 32 bits color depth, probably succeed */
    hbmp = CreateBitmap(4, 4, 1, 32, bits);
    ok( hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject( hdcmem, hbmp);
    if( bitspixel == 32) {
        ok( oldhbmp != NULL, "SelectObject returned NULL\n" );
        col = GetPixel( hdcmem, 0, 0);
        ok( col == 0xffffff,
            "GetPixel of a bitmap with 32 bits/pixel returned %08lx, expected 00ffffff\n", col);
        col = GetPixel( hdcmem, 1, 1);
        ok( col == 0x000000,
            "GetPixel of a bitmap with 32 bits/pixel returned returned %08lx, expected 00000000\n", col);
    }
    if( oldhbmp) SelectObject( hdcmem, oldhbmp);
    DeleteObject( hbmp);
    ReleaseDC( 0, hdc );
}

static void test_DeleteDC(void)
{
    HWND hwnd;
    HDC hdc, hdc_test;
    WNDCLASSEXA cls;
    int ret;

    /* window DC */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0,0,100,100,
                           0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA failed\n");

    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    hdc = GetWindowDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    DestroyWindow(hwnd);

    /* desktop window DC */
    hwnd = GetDesktopWindow();
    ok(hwnd != 0, "GetDesktopWindow failed\n");

    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    hdc = GetWindowDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    /* CS_CLASSDC */
    memset(&cls, 0, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = CS_CLASSDC;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "Wine class DC";
    cls.lpfnWndProc = DefWindowProcA;
    ret = RegisterClassExA(&cls);
    ok(ret, "RegisterClassExA failed\n");

    hwnd = CreateWindowExA(0, "Wine class DC", NULL, WS_POPUP|WS_VISIBLE, 0,0,100,100,
                           0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA failed\n");

    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = ReleaseDC(hwnd, hdc);
    ok(ret, "ReleaseDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);

    hdc_test = hdc;

    hdc = GetWindowDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    DestroyWindow(hwnd);

    ret = GetObjectType(hdc_test);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc_test, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);

    ret = UnregisterClassA("Wine class DC", GetModuleHandleA(NULL));
    ok(ret, "UnregisterClassA failed\n");

    ret = GetDeviceCaps(hdc_test, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    /* CS_OWNDC */
    memset(&cls, 0, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = CS_OWNDC;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "Wine own DC";
    cls.lpfnWndProc = DefWindowProcA;
    ret = RegisterClassExA(&cls);
    ok(ret, "RegisterClassExA failed\n");

    hwnd = CreateWindowExA(0, "Wine own DC", NULL, WS_POPUP|WS_VISIBLE, 0,0,100,100,
                           0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA failed\n");

    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = ReleaseDC(hwnd, hdc);
    ok(ret, "ReleaseDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);

    hdc = GetWindowDC(hwnd);
    ok(hdc != 0, "GetDC failed\n");
    ret = GetObjectType(hdc);
    ok(ret == OBJ_DC, "expected OBJ_DC, got %d\n", ret);
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps rets %d\n", ret);
    ret = DeleteDC(hdc);
    ok(ret, "DeleteDC failed\n");
    ret = GetDeviceCaps(hdc, TECHNOLOGY);
    ok(!ret, "GetDeviceCaps should fail for a deleted DC\n");

    DestroyWindow(hwnd);

    ret = UnregisterClassA("Wine own DC", GetModuleHandleA(NULL));
    ok(ret, "UnregisterClassA failed\n");
}

static void test_boundsrect(void)
{
    char buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    HDC hdc;
    HBITMAP bitmap, dib, old;
    RECT rect, expect, set_rect;
    UINT ret;
    int i, level;

    hdc = CreateCompatibleDC(0);
    ok(hdc != NULL, "CreateCompatibleDC failed\n");
    bitmap = CreateCompatibleBitmap( hdc, 200, 200 );
    old = SelectObject( hdc, bitmap );

    ret = GetBoundsRect(hdc, NULL, 0);
    ok(ret == 0, "Expected GetBoundsRect to return 0, got %u\n", ret);

    ret = GetBoundsRect(hdc, NULL, ~0U);
    ok(ret == 0, "Expected GetBoundsRect to return 0, got %u\n", ret);

    /* Test parameter handling order. */
    SetRect(&set_rect, 10, 20, 40, 50);
    ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
    ok(ret & DCB_RESET,
       "Expected return flag DCB_RESET to be set, got %u\n", ret);

    ret = GetBoundsRect(hdc, NULL, DCB_RESET);
    ok(ret == 0,
       "Expected GetBoundsRect to return 0, got %u\n", ret);

    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_RESET,
       "Expected GetBoundsRect to return DCB_RESET, got %u\n", ret);
    SetRectEmpty(&expect);
    ok(EqualRect(&rect, &expect), "Expected output rectangle (0,0)-(0,0), got %s\n",
       wine_dbgstr_rect(&rect));

    ret = GetBoundsRect(NULL, NULL, 0);
    ok(ret == 0, "Expected GetBoundsRect to return 0, got %u\n", ret);

    ret = GetBoundsRect(NULL, NULL, ~0U);
    ok(ret == 0, "Expected GetBoundsRect to return 0, got %u\n", ret);

    ret = SetBoundsRect(NULL, NULL, 0);
    ok(ret == 0, "Expected SetBoundsRect to return 0, got %u\n", ret);

    ret = SetBoundsRect(NULL, NULL, ~0U);
    ok(ret == 0, "Expected SetBoundsRect to return 0, got %u\n", ret);

    SetRect(&set_rect, 10, 20, 40, 50);
    ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
    ok(ret == (DCB_RESET | DCB_DISABLE), "SetBoundsRect returned %x\n", ret);

    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 10, 20, 40, 50);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, 2, 2, NULL );
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 5, 10, 20, 25);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetViewportOrgEx( hdc, 20, 30, NULL );
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, -5, -5, 10, 10);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetRect(&set_rect, 10, 20, 40, 50);
    ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
    ok(ret == (DCB_SET | DCB_DISABLE), "SetBoundsRect returned %x\n", ret);

    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 10, 20, 40, 50);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetMapMode( hdc, MM_TEXT );
    SetViewportOrgEx( hdc, 0, 0, NULL );
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 40, 70, 100, 130);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetLayout( hdc, LAYOUT_RTL );
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 159, 70, 99, 130);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SetRect(&set_rect, 50, 25, 30, 35);
    ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
    ok(ret == (DCB_SET | DCB_DISABLE), "SetBoundsRect returned %x\n", ret);
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 50, 25, 30, 35);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SetLayout( hdc, LAYOUT_LTR );
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_SET, "GetBoundsRect returned %x\n", ret);
    SetRect(&expect, 149, 25, 169, 35);
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    /* empty rect resets, except on nt4 */
    SetRect(&expect, 20, 20, 10, 10);
    ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
    ok(ret == (DCB_SET | DCB_DISABLE), "SetBoundsRect returned %x\n", ret);
    ret = GetBoundsRect(hdc, &rect, 0);
    ok(ret == DCB_RESET || broken(ret == DCB_SET)  /* nt4 */,
       "GetBoundsRect returned %x\n", ret);
    if (ret == DCB_RESET)
    {
        SetRectEmpty(&expect);
        ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

        SetRect(&expect, 20, 20, 20, 20);
        ret = SetBoundsRect(hdc, &set_rect, DCB_SET);
        ok(ret == (DCB_RESET | DCB_DISABLE), "SetBoundsRect returned %x\n", ret);
        ret = GetBoundsRect(hdc, &rect, 0);
        ok(ret == DCB_RESET, "GetBoundsRect returned %x\n", ret);
        SetRectEmpty(&expect);
        ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    }

    SetBoundsRect( hdc, NULL, DCB_RESET | DCB_ENABLE );
    MoveToEx( hdc, 10, 10, NULL );
    LineTo( hdc, 20, 20 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 10, 10, 21, 21 );
    ok( EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SetRect( &rect, 8, 8, 23, 23 );
    expect = rect;
    SetBoundsRect( hdc, &rect, DCB_ACCUMULATE );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    level = SaveDC( hdc );
    LineTo( hdc, 30, 25 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 8, 8, 31, 26 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SetBoundsRect( hdc, NULL, DCB_DISABLE );
    LineTo( hdc, 40, 40 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 8, 8, 31, 26 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SetRect( &rect, 6, 6, 30, 30 );
    SetBoundsRect( hdc, &rect, DCB_ACCUMULATE );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 31, 30 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    RestoreDC( hdc, level );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    LineTo( hdc, 40, 40 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    SelectObject( hdc, old );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 1, 1 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SetBoundsRect( hdc, NULL, DCB_ENABLE );
    LineTo( hdc, 50, 40 );

    SelectObject( hdc, bitmap );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 51, 41 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    SelectObject( hdc, GetStockObject( NULL_PEN ));
    LineTo( hdc, 50, 50 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 51, 51 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    memset( buffer, 0, sizeof(buffer) );
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = 256;
    info->bmiHeader.biHeight = 256;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 8;
    info->bmiHeader.biCompression = BI_RGB;
    dib = CreateDIBSection( 0, info, DIB_RGB_COLORS, NULL, NULL, 0 );
    ok( dib != 0, "failed to create DIB\n" );
    SelectObject( hdc, dib );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 51, 51 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    LineTo( hdc, 55, 30 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 56, 51 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    LineTo( hdc, 300, 30 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 6, 6, 256, 51 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));
    LineTo( hdc, -300, -300 );
    ret = GetBoundsRect( hdc, &rect, 0 );
    ok( ret == DCB_SET, "GetBoundsRect returned %x\n", ret );
    SetRect( &expect, 0, 0, 256, 51 );
    ok(EqualRect(&rect, &expect), "Got %s\n", wine_dbgstr_rect(&rect));

    /* test the wide pen heuristics */
    SetBoundsRect( hdc, NULL, DCB_ENABLE | DCB_RESET );
    for (i = 0; i < 1000; i++)
    {
        static const UINT endcaps[3] = { PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE, PS_ENDCAP_FLAT };
        static const UINT joins[3] = { PS_JOIN_ROUND, PS_JOIN_BEVEL, PS_JOIN_MITER };
        LOGBRUSH brush = { BS_SOLID, RGB(0,0,0), 0 };
        UINT join = joins[i % 3];
        UINT endcap = endcaps[(i / 3) % 3];
        INT inflate, width = 1 + i / 9;
        HPEN pen = ExtCreatePen( PS_GEOMETRIC | join | endcap | PS_SOLID, width, &brush, 0, NULL );
        HPEN old = SelectObject( hdc, pen );
        MoveToEx( hdc, 100, 100, NULL );
        LineTo( hdc, 160, 100 );
        LineTo( hdc, 100, 160 );
        LineTo( hdc, 160, 160 );
        GetBoundsRect( hdc, &rect, DCB_RESET );
        SetRect( &expect, 100, 100, 161, 161 );

        inflate = width + 2;
        if (join == PS_JOIN_MITER)
        {
            inflate *= 5;
            if (endcap == PS_ENDCAP_SQUARE)
                InflateRect( &expect, (inflate * 3 + 1) / 2, (inflate * 3 + 1) / 2 );
            else
                InflateRect( &expect, inflate, inflate );
        }
        else
        {
            if (endcap == PS_ENDCAP_SQUARE)
                InflateRect( &expect, inflate - inflate / 4, inflate - inflate / 4 );
            else
                InflateRect( &expect, (inflate + 1) / 2, (inflate + 1) / 2 );
        }
        expect.left   = max( expect.left, 0 );
        expect.top    = max( expect.top, 0 );
        expect.right  = min( expect.right, 256 );
        expect.bottom = min( expect.bottom, 256 );
        ok(EqualRect(&rect, &expect), "Got %s expected %s %u/%x/%x\n", wine_dbgstr_rect(&rect),
           wine_dbgstr_rect(&expect), width, endcap, join);
        DeleteObject( SelectObject( hdc, old ));
    }

    DeleteDC( hdc );
    DeleteObject( bitmap );
    DeleteObject( dib );
}

static void test_desktop_colorres(void)
{
    HDC hdc = GetDC(NULL);
    int bitspixel, colorres;

    bitspixel = GetDeviceCaps(hdc, BITSPIXEL);
    ok(bitspixel != 0, "Expected to get valid BITSPIXEL capability value\n");

    colorres = GetDeviceCaps(hdc, COLORRES);
    ok(colorres != 0 ||
       broken(colorres == 0), /* Win9x */
       "Expected to get valid COLORRES capability value\n");

    if (colorres)
    {
        switch (bitspixel)
        {
        case 8:
            ok(colorres == 18,
               "Expected COLORRES to be 18, got %d\n", colorres);
            break;
        case 16:
            ok(colorres == 16,
               "Expected COLORRES to be 16, got %d\n", colorres);
            break;
        case 24:
        case 32:
            ok(colorres == 24,
               "Expected COLORRES to be 24, got %d\n", bitspixel);
            break;
        default:
            ok(0, "Got unknown BITSPIXEL %d with COLORRES %d\n", bitspixel, colorres);
            break;
        }
    }

    ReleaseDC(NULL, hdc);
}

static void test_gamma(void)
{
    BOOL ret;
    HDC hdc = GetDC(NULL);
    WORD oldramp[3][256], ramp[3][256];
    INT i;

    ret = GetDeviceGammaRamp(hdc, &oldramp);
    if (!ret)
    {
        win_skip("GetDeviceGammaRamp failed, skipping tests\n");
        goto done;
    }

    /* try to set back old ramp */
    ret = SetDeviceGammaRamp(hdc, &oldramp);
    if (!ret)
    {
        win_skip("SetDeviceGammaRamp failed, skipping tests\n");
        goto done;
    }

    memcpy(ramp, oldramp, sizeof(ramp));

    /* set one color ramp to zeros */
    memset(ramp[0], 0, sizeof(ramp[0]));
    ret = SetDeviceGammaRamp(hdc, &ramp);
    ok(!ret || broken(ret /* win1909 */),
       "SetDeviceGammaRamp(zeroes) succeeded\n");

    /* set one color ramp to a flat straight rising line */
    for (i = 0; i < 256; i++) ramp[0][i] = i;
    ret = SetDeviceGammaRamp(hdc, &ramp);
    todo_wine ok(!ret || broken(ret /* win1909 */),
       "SetDeviceGammaRamp(low) succeeded\n");

    /* set one color ramp to a steep straight rising line */
    for (i = 0; i < 256; i++) ramp[0][i] = i * 256;
    ret = SetDeviceGammaRamp(hdc, &ramp);
    ok(ret, "SetDeviceGammaRamp(steep) failed\n");

    /* try a bright gamma ramp */
    ramp[0][0] = 0;
    ramp[0][1] = 0x7FFF;
    for (i = 2; i < 256; i++) ramp[0][i] = 0xFFFF;
    ret = SetDeviceGammaRamp(hdc, &ramp);
    ok(!ret || broken(ret /* win1909 */),
       "SetDeviceGammaRam(bright) succeeded\n");

    /* try ramps which are not uniform */
    for (i = 0; i < 256; i++) ramp[0][i] = 512 * i; /* wraps midway */
    ret = SetDeviceGammaRamp(hdc, &ramp);
    ok(ret, "SetDeviceGammaRamp(wrap) failed\n");
    for (i = 0; i < 256; i += 2)
         ramp[0][i] = ramp[0][i + 1] = 256 * i; /* stairs */
    ret = SetDeviceGammaRamp(hdc, &ramp);
    ok(ret, "SetDeviceGammaRamp(stairs) failed\n");

    /* cleanup: set old ramp again */
    ret = SetDeviceGammaRamp(hdc, &oldramp);
    ok(ret, "SetDeviceGammaRamp(old) failed\n");

done:
    ReleaseDC(NULL, hdc);
}

static BOOL is_postscript_printer(HDC hdc)
{
    char tech[256];

    if (ExtEscape(hdc, GETTECHNOLOGY, 0, NULL, sizeof(tech), tech) > 0)
        return strcmp(tech, "PostScript") == 0;

    return FALSE;
}

static HDC create_printer_dc(int scale, BOOL reset)
{
    char buffer[260];
    DWORD len;
    PRINTER_INFO_2A *pbuf = NULL;
    DRIVER_INFO_3A *dbuf = NULL;
    HANDLE hprn = 0;
    HDC hdc = 0;
    HMODULE winspool = LoadLibraryA( "winspool.drv" );
    BOOL (WINAPI *pOpenPrinterA)(LPSTR, HANDLE *, LPPRINTER_DEFAULTSA);
    BOOL (WINAPI *pGetDefaultPrinterA)(LPSTR, LPDWORD);
    BOOL (WINAPI *pGetPrinterA)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
    BOOL (WINAPI *pGetPrinterDriverA)(HANDLE, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD);
    BOOL (WINAPI *pClosePrinter)(HANDLE);

    pGetDefaultPrinterA = (void *)GetProcAddress( winspool, "GetDefaultPrinterA" );
    pOpenPrinterA = (void *)GetProcAddress( winspool, "OpenPrinterA" );
    pGetPrinterA = (void *)GetProcAddress( winspool, "GetPrinterA" );
    pGetPrinterDriverA = (void *)GetProcAddress( winspool, "GetPrinterDriverA" );
    pClosePrinter = (void *)GetProcAddress( winspool, "ClosePrinter" );

    if (!pGetDefaultPrinterA || !pOpenPrinterA || !pGetPrinterA || !pGetPrinterDriverA || !pClosePrinter)
        goto done;

    len = sizeof(buffer);
    if (!pGetDefaultPrinterA( buffer, &len )) goto done;
    if (!pOpenPrinterA( buffer, &hprn, NULL )) goto done;

    pGetPrinterA( hprn, 2, NULL, 0, &len );
    pbuf = malloc( len );
    if (!pGetPrinterA( hprn, 2, (LPBYTE)pbuf, len, &len )) goto done;

    pGetPrinterDriverA( hprn, NULL, 3, NULL, 0, &len );
    dbuf = malloc( len );
    if (!pGetPrinterDriverA( hprn, NULL, 3, (LPBYTE)dbuf, len, &len )) goto done;

    pbuf->pDevMode->dmScale = scale;
    pbuf->pDevMode->dmFields |= DM_SCALE;

    hdc = CreateDCA( dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName, pbuf->pDevMode );
    trace( "hdc %p for driver '%s' printer '%s' port '%s' is %sPostScript\n", hdc,
           dbuf->pDriverPath, pbuf->pPrinterName, pbuf->pPortName,
           is_postscript_printer(hdc) ? "" : "NOT " );

    if (reset) ResetDCA( hdc, pbuf->pDevMode );
done:
    free( dbuf );
    free( pbuf );
    if (hprn) pClosePrinter( hprn );
    if (winspool) FreeLibrary( winspool );
    if (!hdc) skip( "could not create a DC for the default printer\n" );
    return hdc;
}

static void test_printer_dc(void)
{
    HDC memdc, display_memdc, enhmf_dc;
    HBITMAP orig, bmp;
    DWORD ret;
    HDC hdc, hdc_200;

    hdc = create_printer_dc(100, FALSE);
    hdc_200 = create_printer_dc(200, FALSE);

    if (!hdc || !hdc_200) return;

    test_device_caps( hdc, hdc_200, "printer dc", is_postscript_printer(hdc) ? 2 : 1 );
    DeleteDC( hdc_200 );

    hdc_200 = create_printer_dc(200, TRUE);
    test_device_caps( hdc, hdc_200, "printer dc", is_postscript_printer(hdc) ? 2 : 1 );
    DeleteDC( hdc_200 );

    memdc = CreateCompatibleDC( hdc );
    display_memdc = CreateCompatibleDC( 0 );

    ok( memdc != NULL, "CreateCompatibleDC failed for printer\n" );
    ok( display_memdc != NULL, "CreateCompatibleDC failed for screen\n" );

    ret = GetDeviceCaps( hdc, TECHNOLOGY );
    ok( ret == DT_RASPRINTER, "wrong type %lu\n", ret );

    ret = GetDeviceCaps( memdc, TECHNOLOGY );
    ok( ret == DT_RASPRINTER, "wrong type %lu\n", ret );

    ret = GetDeviceCaps( display_memdc, TECHNOLOGY );
    ok( ret == DT_RASDISPLAY, "wrong type %lu\n", ret );

    bmp = CreateBitmap( 100, 100, 1, GetDeviceCaps( hdc, BITSPIXEL ), NULL );
    orig = SelectObject( memdc, bmp );
    ok( orig != NULL, "SelectObject failed\n" );
    ok( BitBlt( hdc, 10, 10, 20, 20, memdc, 0, 0, SRCCOPY ), "BitBlt failed\n" );

    test_device_caps( memdc, hdc, "printer dc", 1 );

    ok( !SelectObject( display_memdc, bmp ), "SelectObject succeeded\n" );
    SelectObject( memdc, orig );
    DeleteObject( bmp );

    bmp = CreateBitmap( 100, 100, 1, 1, NULL );
    orig = SelectObject( display_memdc, bmp );
    ok( orig != NULL, "SelectObject failed\n" );
    ok( !SelectObject( memdc, bmp ), "SelectObject succeeded\n" );
    ok( BitBlt( hdc, 10, 10, 20, 20, display_memdc, 0, 0, SRCCOPY ), "BitBlt failed\n" );
    ok( BitBlt( memdc, 10, 10, 20, 20, display_memdc, 0, 0, SRCCOPY ), "BitBlt failed\n" );
    ok( BitBlt( display_memdc, 10, 10, 20, 20, memdc, 0, 0, SRCCOPY ), "BitBlt failed\n" );

    ret = GetPixel( hdc, 0, 0 );
    ok( ret == CLR_INVALID, "wrong pixel value %lx\n", ret );

    enhmf_dc = CreateEnhMetaFileA( hdc, NULL, NULL, NULL );
    ok(enhmf_dc != 0, "CreateEnhMetaFileA failed\n");
    test_device_caps( enhmf_dc, hdc, "enhmetafile printer dc", 1 );
    DeleteEnhMetaFile( CloseEnhMetaFile( enhmf_dc ));

    enhmf_dc = CreateEnhMetaFileA( hdc, NULL, NULL, NULL );
    ok(enhmf_dc != 0, "CreateEnhMetaFileA failed\n");
    test_device_caps( enhmf_dc, hdc, "enhmetafile printer dc", 1 );
    DeleteEnhMetaFile( CloseEnhMetaFile( enhmf_dc ));

    DeleteDC( memdc );
    DeleteDC( display_memdc );
    DeleteDC( hdc );
    DeleteObject( bmp );
}

static void print_something(HDC hdc)
{
    static const char psadobe[10] = "%!PS-Adobe";
    char buf[1024], *p;
    char temp_path[MAX_PATH], file_name[MAX_PATH];
    DOCINFOA di;
    DWORD ret;
    HANDLE hfile;

    GetTempPathA(sizeof(temp_path), temp_path);
    GetTempFileNameA(temp_path, "ps", 0, file_name);

    di.cbSize = sizeof(di);
    di.lpszDocName = "Let's dance";
    di.lpszOutput = file_name;
    di.lpszDatatype = NULL;
    di.fwType = 0;
    ret = StartDocA(hdc, &di);
    ok(ret > 0, "StartDoc failed: %ld\n", ret);

    strcpy(buf + 2, "\n% ===> before DOWNLOADHEADER <===\n");
    *(WORD *)buf = strlen(buf + 2);
    ret = Escape(hdc, POSTSCRIPT_PASSTHROUGH, 0, buf, NULL);
    ok(ret == *(WORD *)buf, "POSTSCRIPT_PASSTHROUGH failed: %ld\n", ret);

    strcpy(buf, "deadbeef");
    ret = ExtEscape(hdc, DOWNLOADHEADER, 0, NULL, sizeof(buf), buf );
    ok(ret == 1, "DOWNLOADHEADER failed\n");
    ok(strcmp(buf, "deadbeef") != 0, "DOWNLOADHEADER failed\n");

    strcpy(buf + 2, "\n% ===> after DOWNLOADHEADER <===\n");
    *(WORD *)buf = strlen(buf + 2);
    ret = Escape(hdc, POSTSCRIPT_PASSTHROUGH, 0, buf, NULL);
    ok(ret == *(WORD *)buf, "POSTSCRIPT_PASSTHROUGH failed: %ld\n", ret);

    ret = EndDoc(hdc);
    ok(ret == 1, "EndDoc failed\n");

    hfile = CreateFileA(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    memset(buf, 0, sizeof(buf));
    ret = ReadFile(hfile, buf, sizeof(buf), &ret, NULL);
    ok(ret, "ReadFile failed\n");
    CloseHandle(hfile);

    /* skip the HP PCL language selector */
    buf[sizeof(buf) - 1] = 0;
    p = buf;
    while (*p)
    {
        if (!(p[0] == 0x1b && p[1] == '%') && memcmp(p, "@PJL", 4) != 0)
            break;

        p = strchr(p, '\n');
        if (!p) break;

        while (*p == '\r' || *p == '\n') p++;
    }
    ok(p && !memcmp(p, psadobe, sizeof(psadobe)), "wrong signature: %.14s\n", p ? p : buf);

    DeleteFileA(file_name);
}

static void test_pscript_printer_dc(void)
{
    HDC hdc;
    char buf[256];
    DWORD query, ret;

    hdc = create_printer_dc(100, FALSE);

    if (!hdc) return;

    if (!is_postscript_printer(hdc))
    {
        skip("Default printer is not a PostScript device\n");
        DeleteDC( hdc );
        return;
    }

    query = GETFACENAME;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(!ret, "GETFACENAME is supported\n");

    query = DOWNLOADFACE;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(ret == 1, "DOWNLOADFACE is not supported\n");

    query = OPENCHANNEL;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(ret == 1, "OPENCHANNEL is not supported\n");

    query = DOWNLOADHEADER;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(ret == 1, "DOWNLOADHEADER is not supported\n");

    query = CLOSECHANNEL;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(ret == 1, "CLOSECHANNEL is not supported\n");

    query = POSTSCRIPT_PASSTHROUGH;
    ret = Escape(hdc, QUERYESCSUPPORT, sizeof(query), (LPCSTR)&query, NULL);
    ok(ret == 1, "POSTSCRIPT_PASSTHROUGH is not supported\n");

    ret = ExtEscape(hdc, GETFACENAME, 0, NULL, sizeof(buf), buf);
    ok(ret == 1, "GETFACENAME failed\n");
    trace("face name: %s\n", buf);

    print_something(hdc);

    DeleteDC(hdc);
}

struct screen_region_info
{
    HRGN region;
    INT type;
};

static BOOL CALLBACK enum_monitor_proc(HMONITOR monitor, HDC hdc, RECT *rect, LPARAM lparam)
{
    struct screen_region_info *info = (struct screen_region_info *)lparam;
    HRGN region;

    if (!info->region)
    {
        info->region = CreateRectRgnIndirect(rect);
        info->type = SIMPLEREGION;
    }
    else
    {
        region = CreateRectRgnIndirect(rect);
        info->type = CombineRgn(info->region, info->region, region, RGN_OR);
        DeleteObject(region);
    }
    return TRUE;
}

static INT get_screen_region_type(void)
{
    struct screen_region_info info = {NULL, NULLREGION};

    EnumDisplayMonitors(NULL, NULL, enum_monitor_proc, (LPARAM)&info);
    DeleteObject(info.region);
    return info.type;
}

static void test_clip_box(void)
{
    DEVMODEA scale_mode = {.dmSize = sizeof(DEVMODEA)};
    HBITMAP bitmap = CreateBitmap(10, 10, 1, 1, NULL);
    HDC dc, desktop_dc, printer_dc;
    RECT rect, expect, screen_rect;
    int type, screen_type;

    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &scale_mode);
    scale_mode.dmFields |= DM_SCALE;
    scale_mode.dmScale = 200;

    SetRect(&screen_rect, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN));
    screen_type = get_screen_region_type();

    dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    type = GetClipBox(dc, &rect);
    todo_wine_if(screen_type == COMPLEXREGION)
        ok(type == screen_type, "wrong region type %d\n", type);
    ok(EqualRect(&rect, &screen_rect), "expected %s, got %s\n",
            wine_dbgstr_rect(&screen_rect), wine_dbgstr_rect(&rect));
    DeleteDC(dc);

    dc = CreateDCA("DISPLAY", NULL, NULL, &scale_mode);
    type = GetClipBox(dc, &rect);
    todo_wine_if(screen_type == COMPLEXREGION)
        ok(type == screen_type, "wrong region type %d\n", type);
    ok(EqualRect(&rect, &screen_rect), "expected %s, got %s\n",
            wine_dbgstr_rect(&screen_rect), wine_dbgstr_rect(&rect));
    ResetDCA(dc, &scale_mode);
    type = GetClipBox(dc, &rect);
    todo_wine_if(screen_type == COMPLEXREGION)
        ok(type == screen_type, "wrong region type %d\n", type);
    ok(EqualRect(&rect, &screen_rect), "expected %s, got %s\n",
            wine_dbgstr_rect(&screen_rect), wine_dbgstr_rect(&rect));
    DeleteDC(dc);

    dc = CreateCompatibleDC(NULL);
    type = GetClipBox(dc, &rect);
    ok(type == SIMPLEREGION, "wrong region type %d\n", type);
    SetRect(&expect, 0, 0, 1, 1);
    ok(EqualRect(&rect, &expect), "got %s\n", wine_dbgstr_rect(&rect));
    SelectObject(dc, bitmap);
    type = GetClipBox(dc, &rect);
    ok(type == SIMPLEREGION, "wrong region type %d\n", type);
    SetRect(&expect, 0, 0, 10, 10);
    ok(EqualRect(&rect, &expect), "got %s\n", wine_dbgstr_rect(&rect));
    DeleteDC(dc);

    desktop_dc = GetDC(0);
    type = GetClipBox(desktop_dc, &rect);
    todo_wine_if(screen_type == COMPLEXREGION)
        ok(type == screen_type, "wrong region type %d\n", type);
    ok(EqualRect(&rect, &screen_rect), "expected %s, got %s\n",
            wine_dbgstr_rect(&screen_rect), wine_dbgstr_rect(&rect));

    dc = CreateEnhMetaFileA(desktop_dc, NULL, NULL, NULL);
    ok(!!dc, "CreateEnhMetaFile() failed\n");
    type = GetClipBox(dc, &rect);
    todo_wine ok(type == SIMPLEREGION, "wrong region type %d\n", type);
    if (type != ERROR)
        ok(EqualRect(&rect, &screen_rect), "expected %s, got %s\n",
                wine_dbgstr_rect(&screen_rect), wine_dbgstr_rect(&rect));
    DeleteEnhMetaFile(CloseEnhMetaFile(dc));

    ReleaseDC(0, desktop_dc);

    dc = CreateMetaFileA(NULL);
    ok(!!dc, "CreateEnhMetaFile() failed\n");
    type = GetClipBox(dc, &rect);
    ok(type == ERROR, "wrong region type %d\n", type);
    DeleteMetaFile(CloseMetaFile(dc));

    if ((printer_dc = create_printer_dc(100, FALSE)))
    {
        type = GetClipBox(printer_dc, &rect);
        ok(type == SIMPLEREGION, "wrong region type %d\n", type);

        dc = CreateCompatibleDC(printer_dc);
        type = GetClipBox(dc, &rect);
        ok(type == SIMPLEREGION, "wrong region type %d\n", type);
        SetRect(&expect, 0, 0, 1, 1);
        ok(EqualRect(&rect, &expect), "got %s\n", wine_dbgstr_rect(&rect));
        DeleteDC(dc);

        dc = CreateEnhMetaFileA(printer_dc, NULL, NULL, NULL);
        type = GetClipBox(dc, &rect);
        todo_wine ok(type == SIMPLEREGION, "wrong region type %d\n", type);
        DeleteEnhMetaFile(CloseEnhMetaFile(dc));

        DeleteDC(printer_dc);
    }

    DeleteObject(bitmap);
}

static void test_SetPixel(void)
{
    COLORREF c;

    c = SetPixel((HDC)0xdeadbeef, 0, 0, 0);
    ok(c == ~0, "SetPixel returned: %lx\n", c);
}


START_TEST(dc)
{
    test_dc_values();
    test_savedc();
    test_savedc_2();
    test_GdiConvertToDevmodeW();
    test_CreateDC();
    test_CreateCompatibleDC();
    test_DC_bitmap();
    test_DeleteDC();
    test_boundsrect();
    test_desktop_colorres();
    test_gamma();
    test_printer_dc();
    test_pscript_printer_dc();
    test_clip_box();
    test_SetPixel();
}
