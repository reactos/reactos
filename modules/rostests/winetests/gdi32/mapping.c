/*
 * Unit tests for mapping functions
 *
 * Copyright (c) 2005 Huw Davies
 * Copyright (c) 2008 Dmitry  Timoshkov
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

#include <stdio.h>
#include <math.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

static DWORD (WINAPI *pGetLayout)(HDC hdc);
static INT (WINAPI *pGetRandomRgn)(HDC hDC, HRGN hRgn, INT iCode);
static BOOL (WINAPI *pGetTransform)(HDC, DWORD, XFORM *);
static BOOL (WINAPI *pSetVirtualResolution)(HDC, DWORD, DWORD, DWORD, DWORD);

#define rough_match(got, expected) (abs( MulDiv( (got) - (expected), 1000, (expected) )) <= 5)

#define expect_LPtoDP(_hdc, _x, _y) \
{ \
    POINT _pt = { 1000, 1000 }; \
    LPtoDP(_hdc, &_pt, 1); \
    ok(rough_match(_pt.x, _x), "expected x %d, got %ld\n", (_x), _pt.x); \
    ok(rough_match(_pt.y, _y), "expected y %d, got %ld\n", (_y), _pt.y); \
}

#define expect_world_transform(_hdc, _em11, _em22) \
{ \
    BOOL _ret; \
    XFORM _xform; \
    SetLastError(0xdeadbeef); \
    _ret = GetWorldTransform(_hdc, &_xform); \
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) \
    { \
        ok(_ret, "GetWorldTransform error %lu\n", GetLastError()); \
        ok(_xform.eM11 == (_em11), "expected %f, got %f\n", (_em11), _xform.eM11); \
        ok(_xform.eM12 == 0.0, "expected 0.0, got %f\n", _xform.eM12); \
        ok(_xform.eM21 == 0.0, "expected 0.0, got %f\n", _xform.eM21); \
        ok(_xform.eM22 == (_em22), "expected %f, got %f\n", (_em22), _xform.eM22); \
        ok(_xform.eDx == 0.0, "expected 0.0, got %f\n", _xform.eDx); \
        ok(_xform.eDy == 0.0, "expected 0.0, got %f\n", _xform.eDy); \
    } \
}

#define expect_dc_ext(_func, _hdc, _cx, _cy) \
{ \
    BOOL _ret; \
    SIZE _size; \
    SetLastError(0xdeadbeef); \
    _ret = _func(_hdc, &_size); \
    ok(_ret, #_func " error %lu\n", GetLastError()); \
    ok(_size.cx == (_cx), "expected cx %ld, got %ld\n", (_cx), _size.cx); \
    ok(_size.cy == (_cy), "expected cy %ld, got %ld\n", (_cy), _size.cy); \
}

#define expect_viewport_ext(_hdc, _cx, _cy) expect_dc_ext(GetViewportExtEx, _hdc, _cx, _cy)
#define expect_window_ext(_hdc, _cx, _cy)  expect_dc_ext(GetWindowExtEx, _hdc, _cx, _cy)

static void test_world_transform(void)
{
    HDC hdc;
    int ret;
    LONG size_cx, size_cy, res_x, res_y, dpi_x, dpi_y;
    XFORM xform;
    SIZE size;

    hdc = CreateCompatibleDC(0);

    xform.eM11 = 1.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 1.0f;
    xform.eDx = 0.0f;
    xform.eDy = 0.0f;
    ret = SetWorldTransform(hdc, &xform);
    ok(!ret, "SetWorldTransform should fail in GM_COMPATIBLE mode\n");

    size_cx = GetDeviceCaps(hdc, HORZSIZE);
    size_cy = GetDeviceCaps(hdc, VERTSIZE);
    res_x = GetDeviceCaps(hdc, HORZRES);
    res_y = GetDeviceCaps(hdc, VERTRES);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    trace("dc size %ld x %ld, resolution %ld x %ld dpi %ld x %ld\n",
          size_cx, size_cy, res_x, res_y, dpi_x, dpi_y );

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_LOMETRIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    expect_viewport_ext(hdc, res_x, -res_y);
    ok( GetWindowExtEx( hdc, &size ), "GetWindowExtEx failed\n" );
    ok( rough_match( size.cx, size_cx * 10 ) ||
        rough_match( size.cx, MulDiv( res_x, 254, dpi_x )),  /* Vista uses a more precise method */
        "expected cx %ld or %d, got %ld\n", size_cx * 10, MulDiv( res_x, 254, dpi_x ), size.cx );
    ok( rough_match( size.cy, size_cy * 10 ) ||
        rough_match( size.cy, MulDiv( res_y, 254, dpi_y )),  /* Vista uses a more precise method */
        "expected cy %ld or %d, got %ld\n", size_cy * 10, MulDiv( res_y, 254, dpi_y ), size.cy );
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, MulDiv(1000 / 10, res_x, size_cx), -MulDiv(1000 / 10, res_y, size_cy));

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_TEXT);
    ok(ret == MM_LOMETRIC, "expected MM_LOMETRIC, got %d\n", ret);

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    if (!ret)
    {
        DeleteDC(hdc);
        skip("GM_ADVANCED is not supported on this platform\n");
        return;
    }

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    /* The transform must conform to (eM11 * eM22 != eM12 * eM21) requirement */
    xform.eM11 = 1.0f;
    xform.eM12 = 2.0f;
    xform.eM21 = 1.0f;
    xform.eM22 = 2.0f;
    xform.eDx = 0.0f;
    xform.eDy = 0.0f;
    ret = SetWorldTransform(hdc, &xform);
    ok(!ret ||
       broken(ret), /* NT4 */
       "SetWorldTransform should fail with an invalid xform\n");

    xform.eM11 = 20.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 20.0f;
    xform.eDx = 0.0f;
    xform.eDy = 0.0f;
    SetLastError(0xdeadbeef);
    ret = SetWorldTransform(hdc, &xform);
    ok(ret, "SetWorldTransform error %lu\n", GetLastError());

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_LOMETRIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    expect_viewport_ext(hdc, res_x, -res_y);
    ok( GetWindowExtEx( hdc, &size ), "GetWindowExtEx failed\n" );
    ok( rough_match( size.cx, size_cx * 10 ) ||
        rough_match( size.cx, MulDiv( res_x, 254, dpi_x )),  /* Vista uses a more precise method */
        "expected cx %ld or %d, got %ld\n", size_cx * 10, MulDiv( res_x, 254, dpi_x ), size.cx );
    ok( rough_match( size.cy, size_cy * 10 ) ||
        rough_match( size.cy, MulDiv( res_y, 254, dpi_y )),  /* Vista uses a more precise method */
        "expected cy %ld or %d, got %ld\n", size_cy * 10, MulDiv( res_y, 254, dpi_y ), size.cy );
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, MulDiv(20000, res_x, size.cx), -MulDiv(20000, res_y, size.cy));

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_TEXT);
    ok(ret == MM_LOMETRIC, "expected MM_LOMETRIC, got %d\n", ret);

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    size.cx = 0xdeadbeef;
    size.cy = 0xdeadbeef;
    ret = SetViewportExtEx(hdc, -1, -1, &size);
    ok(ret, "SetViewportExtEx(-1, -1) failed\n");
    ok(size.cx == 1 && size.cy == 1, "expected 1,1 got %ld,%ld\n", size.cx, size.cy);
    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    ret = SetMapMode(hdc, MM_ANISOTROPIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    size.cx = 0xdeadbeef;
    size.cy = 0xdeadbeef;
    ret = SetViewportExtEx(hdc, -1, -1, &size);
    ok(ret, "SetViewportExtEx(-1, -1) failed\n");
    ok(size.cx == 1 && size.cy == 1, "expected 1,1 got %ld,%ld\n", size.cx, size.cy);
    expect_viewport_ext(hdc, -1l, -1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, -20000, -20000);

    ret = SetGraphicsMode(hdc, GM_COMPATIBLE);
    ok(ret, "SetGraphicsMode(GM_COMPATIBLE) should not fail if DC has't an identity transform\n");
    ret = GetGraphicsMode(hdc);
    ok(ret == GM_COMPATIBLE, "expected GM_COMPATIBLE, got %d\n", ret);

    expect_viewport_ext(hdc, -1l, -1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, -20000, -20000);

    DeleteDC(hdc);
}

static void test_dc_layout(void)
{
    INT ret;
    LONG size_cx, size_cy, res_x, res_y, dpi_x, dpi_y;
    SIZE size;
    POINT pt;
    HBITMAP bitmap;
    RECT rc, ret_rc;
    HDC hdc;
    HRGN hrgn;

    if (!pGetLayout)
    {
        win_skip( "Don't have GetLayout\n" );
        return;
    }

    hdc = CreateCompatibleDC(0);
    bitmap = CreateCompatibleBitmap( hdc, 100, 100 );
    SelectObject( hdc, bitmap );

    size_cx = GetDeviceCaps(hdc, HORZSIZE);
    size_cy = GetDeviceCaps(hdc, VERTSIZE);
    res_x = GetDeviceCaps(hdc, HORZRES);
    res_y = GetDeviceCaps(hdc, VERTRES);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);

    ret = GetMapMode( hdc );
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);
    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    SetLayout( hdc, LAYOUT_RTL );
    if (!pGetLayout( hdc ))
    {
        win_skip( "SetLayout not supported\n" );
        DeleteDC(hdc);
        return;
    }

    ret = GetMapMode( hdc );
    ok(ret == MM_ANISOTROPIC, "expected MM_ANISOTROPIC, got %d\n", ret);
    ret = pGetLayout( hdc );
    ok(ret == LAYOUT_RTL, "got %x\n", ret);
    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, -1000 + 99, 1000);
    GetViewportOrgEx( hdc, &pt );
    ok( pt.x == 0 && pt.y == 0, "wrong origin %ld,%ld\n", pt.x, pt.y );
    GetWindowOrgEx( hdc, &pt );
    ok( pt.x == 0 && pt.y == 0, "wrong origin %ld,%ld\n", pt.x, pt.y );
    GetDCOrgEx( hdc, &pt );
    ok( pt.x == 0 && pt.y == 0, "wrong origin %ld,%ld\n", pt.x, pt.y );
    if (pGetTransform)
    {
        XFORM xform;
        BOOL ret = pGetTransform( hdc, 0x204, &xform ); /* World -> Device */
        ok( ret, "got %d\n", ret );
        ok( xform.eM11 == -1.0, "got %f\n", xform.eM11 );
        ok( xform.eM12 == 0.0, "got %f\n", xform.eM12 );
        ok( xform.eM21 == 0.0, "got %f\n", xform.eM21 );
        ok( xform.eM22 == 1.0, "got %f\n", xform.eM22 );
        ok( xform.eDx == 99.0, "got %f\n", xform.eDx );
        ok( xform.eDy == 0.0, "got %f\n", xform.eDy );
    }

    SetRect( &rc, 10, 10, 20, 20 );
    IntersectClipRect( hdc, 10, 10, 20, 20 );
    hrgn = CreateRectRgn( 0, 0, 0, 0 );
    GetClipRgn( hdc, hrgn );
    GetRgnBox( hrgn, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    SetLayout( hdc, LAYOUT_LTR );
    SetRect( &rc, 80, 10, 90, 20 );
    GetClipRgn( hdc, hrgn );
    GetRgnBox( hrgn, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    GetClipBox( hdc, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    IntersectClipRect( hdc, 80, 10, 85, 20 );
    SetLayout( hdc, LAYOUT_RTL );
    SetRect( &rc, 15, 10, 20, 20 );
    GetClipRgn( hdc, hrgn );
    GetRgnBox( hrgn, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    GetClipBox( hdc, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    SetRectRgn( hrgn, 60, 10, 80, 20 );
    SetLayout( hdc, LAYOUT_LTR );
    ExtSelectClipRgn( hdc, hrgn, RGN_OR );
    SetLayout( hdc, LAYOUT_RTL );
    SetRect( &rc, 15, 10, 40, 20 );
    GetClipRgn( hdc, hrgn );
    GetRgnBox( hrgn, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    GetClipBox( hdc, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));

    /* OffsetClipRgn mirrors too */
    OffsetClipRgn( hdc, 5, 5 );
    OffsetRect( &rc, 5, 5 );
    GetClipRgn( hdc, hrgn );
    GetRgnBox( hrgn, &ret_rc );
    ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));

    /* GetRandomRgn returns the raw region */
    if (pGetRandomRgn)
    {
        SetRect( &rc, 55, 15, 80, 25 );
        pGetRandomRgn( hdc, hrgn, 1 );
        GetRgnBox( hrgn, &ret_rc );
        ok( EqualRect( &rc, &ret_rc ), "wrong clip box %s\n", wine_dbgstr_rect( &ret_rc ));
    }

    SetMapMode(hdc, MM_LOMETRIC);
    ret = GetMapMode( hdc );
    ok(ret == MM_ANISOTROPIC, "expected MM_ANISOTROPIC, got %d\n", ret);

    expect_viewport_ext(hdc, res_x, -res_y);
    ok( GetWindowExtEx( hdc, &size ), "GetWindowExtEx failed\n" );
    ok( rough_match( size.cx, size_cx * 10 ) ||
        rough_match( size.cx, MulDiv( res_x, 254, dpi_x )),  /* Vista uses a more precise method */
        "expected cx %ld or %d, got %ld\n", size_cx * 10, MulDiv( res_x, 254, dpi_x ), size.cx );
    ok( rough_match( size.cy, size_cy * 10 ) ||
        rough_match( size.cy, MulDiv( res_y, 254, dpi_y )),  /* Vista uses a more precise method */
        "expected cy %lxd or %d, got %ld\n", size_cy * 10, MulDiv( res_y, 254, dpi_y ), size.cy );
    expect_world_transform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, -MulDiv(1000 / 10, res_x, size_cx) + 99, -MulDiv(1000 / 10, res_y, size_cy));

    SetMapMode(hdc, MM_TEXT);
    ret = GetMapMode( hdc );
    ok(ret == MM_ANISOTROPIC, "expected MM_ANISOTROPIC, got %d\n", ret);
    SetLayout( hdc, LAYOUT_LTR );
    ret = GetMapMode( hdc );
    ok(ret == MM_ANISOTROPIC, "expected MM_ANISOTROPIC, got %d\n", ret);
    SetMapMode(hdc, MM_TEXT);
    ret = GetMapMode( hdc );
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    DeleteDC(hdc);
    DeleteObject( bitmap );
}

static void test_modify_world_transform(void)
{
    HDC hdc = GetDC(0);
    XFORM xform, xform2;
    int ret;

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    ok(ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    ok(ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_LEFTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_RIGHTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

    xform.eM11 = 2;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = 1;
    xform.eDx = xform.eDy = 0;
    ret = ModifyWorldTransform(hdc, &xform, 4);
    ok(ret, "ModifyWorldTransform failed\n");

    memset(&xform2, 0xcc, sizeof(xform2));
    ret = GetWorldTransform(hdc, &xform2);
    ok(ret, "GetWorldTransform failed\n");
    ok(!memcmp(&xform, &xform2, sizeof(xform)), "unexpected xform\n");

    xform.eM11 = 1;
    xform.eM12 = 1;
    xform.eM21 = 1;
    xform.eM22 = 1;
    xform.eDx = xform.eDy = 0;
    ret = ModifyWorldTransform(hdc, &xform, 4);
    ok(!ret, "ModifyWorldTransform succeeded\n");

    ReleaseDC(0, hdc);
}

static void test_SetWindowExt(HDC hdc, LONG cx, LONG cy, LONG expected_vp_cx, LONG expected_vp_cy)
{
    SIZE windowExt, viewportExt;
    POINT windowOrg, windowOrgAfter, viewportOrg, viewportOrgAfter;

    GetWindowOrgEx(hdc, &windowOrg);
    GetViewportOrgEx(hdc, &viewportOrg);

    SetWindowExtEx(hdc, cx, cy, NULL);
    GetWindowExtEx(hdc, &windowExt);
    ok(windowExt.cx == cx && windowExt.cy == cy,
       "Window extension: Expected %ldx%ld, got %ldx%ld\n",
       cx, cy, windowExt.cx, windowExt.cy);

    GetViewportExtEx(hdc, &viewportExt);
    ok(rough_match(viewportExt.cx, expected_vp_cx) && rough_match(viewportExt.cy, expected_vp_cy),
        "Viewport extents have not been properly adjusted: Expected %ldx%ld, got %ldx%ld\n",
        expected_vp_cx, expected_vp_cy, viewportExt.cx, viewportExt.cy);

    GetWindowOrgEx(hdc, &windowOrgAfter);
    ok(windowOrg.x == windowOrgAfter.x && windowOrg.y == windowOrgAfter.y,
        "Window origin changed from (%ld,%ld) to (%ld,%ld)\n",
        windowOrg.x, windowOrg.y, windowOrgAfter.x, windowOrgAfter.y);

    GetViewportOrgEx(hdc, &viewportOrgAfter);
    ok(viewportOrg.x == viewportOrgAfter.x && viewportOrg.y == viewportOrgAfter.y,
        "Viewport origin changed from (%ld,%ld) to (%ld,%ld)\n",
        viewportOrg.x, viewportOrg.y, viewportOrgAfter.x, viewportOrgAfter.y);
}

static void test_SetViewportExt(HDC hdc, LONG cx, LONG cy, LONG expected_vp_cx, LONG expected_vp_cy)
{
    SIZE windowExt, windowExtAfter, viewportExt;
    POINT windowOrg, windowOrgAfter, viewportOrg, viewportOrgAfter;

    GetWindowOrgEx(hdc, &windowOrg);
    GetViewportOrgEx(hdc, &viewportOrg);
    GetWindowExtEx(hdc, &windowExt);

    SetViewportExtEx(hdc, cx, cy, NULL);
    GetViewportExtEx(hdc, &viewportExt);
    ok(rough_match(viewportExt.cx, expected_vp_cx) && rough_match(viewportExt.cy, expected_vp_cy),
        "Viewport extents have not been properly adjusted: Expected %ldx%ld, got %ldx%ld\n",
        expected_vp_cx, expected_vp_cy, viewportExt.cx, viewportExt.cy);

    GetWindowExtEx(hdc, &windowExtAfter);
    ok(windowExt.cx == windowExtAfter.cx && windowExt.cy == windowExtAfter.cy,
       "Window extension changed from %ldx%ld to %ldx%ld\n",
       windowExt.cx, windowExt.cy, windowExtAfter.cx, windowExtAfter.cy);

    GetWindowOrgEx(hdc, &windowOrgAfter);
    ok(windowOrg.x == windowOrgAfter.x && windowOrg.y == windowOrgAfter.y,
        "Window origin changed from (%ld,%ld) to (%ld,%ld)\n",
        windowOrg.x, windowOrg.y, windowOrgAfter.x, windowOrgAfter.y);

    GetViewportOrgEx(hdc, &viewportOrgAfter);
    ok(viewportOrg.x == viewportOrgAfter.x && viewportOrg.y == viewportOrgAfter.y,
        "Viewport origin changed from (%ld,%ld) to (%ld,%ld)\n",
        viewportOrg.x, viewportOrg.y, viewportOrgAfter.x, viewportOrgAfter.y);
}

static void test_isotropic_mapping(void)
{
    SIZE win, vp;
    HDC hdc = GetDC(0);
    
    SetMapMode(hdc, MM_ISOTROPIC);
    
    /* MM_ISOTROPIC is set up like MM_LOMETRIC.
       Initial values after SetMapMode():
       (1 inch = 25.4 mm)
       
                       Windows 9x:               Windows NT:
       Window Ext:     254 x -254                HORZSIZE*10 x VERTSIZE*10
       Viewport Ext:   LOGPIXELSX x LOGPIXELSY   HORZRES x -VERTRES
       
       To test without rounding errors, we have to use multiples of
       these values!
     */
    
    GetWindowExtEx(hdc, &win);
    GetViewportExtEx(hdc, &vp);
    
    test_SetViewportExt(hdc, 10 * vp.cx, 10 * vp.cy, 10 * vp.cx, 10 * vp.cy);
    test_SetWindowExt(hdc, win.cx, win.cy, 10 * vp.cx, 10 * vp.cy);
    test_SetWindowExt(hdc, 2 * win.cx, win.cy, 10 * vp.cx, 5 * vp.cy);
    test_SetWindowExt(hdc, win.cx, win.cy, 5 * vp.cx, 5 * vp.cy);
    test_SetViewportExt(hdc, 4 * vp.cx, 2 * vp.cy, 2 * vp.cx, 2 * vp.cy);
    test_SetViewportExt(hdc, vp.cx, 2 * vp.cy, vp.cx, vp.cy);
    test_SetViewportExt(hdc, 2 * vp.cx, 2 * vp.cy, 2 * vp.cx, 2 * vp.cy);
    test_SetViewportExt(hdc, 4 * vp.cx, 2 * vp.cy, 2 * vp.cx, 2 * vp.cy);
    test_SetWindowExt(hdc, 4 * win.cx, 2 * win.cy, 2 * vp.cx, vp.cy);
    test_SetViewportExt(hdc, -2 * vp.cx, -4 * vp.cy, -2 * vp.cx, -vp.cy);
    test_SetViewportExt(hdc, -2 * vp.cx, -1 * vp.cy, -2 * vp.cx, -vp.cy);    
    test_SetWindowExt(hdc, -4 * win.cx, -2 * win.cy, -2 * vp.cx, -vp.cy);
    test_SetWindowExt(hdc, 4 * win.cx, -4 * win.cy, -vp.cx, -vp.cy);
    
    ReleaseDC(0, hdc);
}

static void test_setvirtualresolution(void)
{
    HDC hdc = CreateICA("DISPLAY", NULL, NULL, NULL);
    BOOL r;
    INT horz_res = GetDeviceCaps(hdc, HORZRES);
    INT horz_size = GetDeviceCaps(hdc, HORZSIZE);
    INT log_pixels_x = GetDeviceCaps(hdc, LOGPIXELSX);
    SIZE orig_lometric_vp, orig_lometric_wnd;

    if(!pSetVirtualResolution)
    {
        win_skip("Don't have SetVirtualResolution\n");
        return;
    }

    /* Get the true resolution limits */
    SetMapMode(hdc, MM_LOMETRIC);
    GetViewportExtEx(hdc, &orig_lometric_vp);
    GetWindowExtEx(hdc, &orig_lometric_wnd);
    SetMapMode(hdc, MM_TEXT);

    r = pSetVirtualResolution(hdc, 4000, 1000, 400, 200); /* 10 pix/mm x 5 pix/mm */
    ok(r == TRUE, "got %d\n", r);
    expect_LPtoDP(hdc, 1000, 1000);
    expect_viewport_ext(hdc, 1l, 1l);
    expect_window_ext(hdc, 1l, 1l);

    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 1000, -500);
    expect_viewport_ext(hdc, 4000l, -1000l);
    expect_window_ext(hdc, 4000l, 2000l);

    /* Doesn't change the device caps */
    ok(horz_res == GetDeviceCaps(hdc, HORZRES), "horz_res changed\n");
    ok(horz_size == GetDeviceCaps(hdc, HORZSIZE), "horz_size changed\n");
    ok(log_pixels_x == GetDeviceCaps(hdc, LOGPIXELSX), "log_pixels_x changed\n");

    r = pSetVirtualResolution(hdc, 8000, 1000, 400, 200); /* 20 pix/mm x 5 pix/mm */
    ok(r == TRUE, "got %d\n", r);
    expect_LPtoDP(hdc, 1000, -500); /* No change, need to re-set the mapping mode */
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 2000, -500);
    expect_viewport_ext(hdc, 8000l, -1000l);
    expect_window_ext(hdc, 4000l, 2000l);

    r = pSetVirtualResolution(hdc, 8000, 1000, 200, 200); /* 40 pix/mm x 5 pix/mm */
    ok(r == TRUE, "got %d\n", r);
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 4000, -500);
    expect_viewport_ext(hdc, 8000l, -1000l);
    expect_window_ext(hdc, 2000l, 2000l);

    r = pSetVirtualResolution(hdc, 8000, 1000, 200, 200); /* 40 pix/mm x 5 pix/mm */
    ok(r == TRUE, "got %d\n", r);
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 4000, -500);
    expect_viewport_ext(hdc, 8000l, -1000l);
    expect_window_ext(hdc, 2000l, 2000l);

    r = pSetVirtualResolution(hdc, 8000, 2000, 200, 200); /* 40 pix/mm x 10 pix/mm */
    ok(r == TRUE, "got %d\n", r);
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 4000, -1000);
    expect_viewport_ext(hdc, 8000l, -2000l);
    expect_window_ext(hdc, 2000l, 2000l);

    r = pSetVirtualResolution(hdc, 0, 0, 10, 0); /* Error */
    ok(r == FALSE, "got %d\n", r);
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_LPtoDP(hdc, 4000, -1000);
    expect_viewport_ext(hdc, 8000l, -2000l);
    expect_window_ext(hdc, 2000l, 2000l);

    r = pSetVirtualResolution(hdc, 0, 0, 0, 0); /* Reset to true resolution */
    ok(r == TRUE, "got %d\n", r);
    SetMapMode(hdc, MM_TEXT);
    SetMapMode(hdc, MM_LOMETRIC);
    expect_viewport_ext(hdc, orig_lometric_vp.cx, orig_lometric_vp.cy);
    expect_window_ext(hdc, orig_lometric_wnd.cx, orig_lometric_wnd.cy);

    DeleteDC(hdc);
}


static inline void expect_identity(int line, XFORM *xf)
{
    ok(xf->eM11 == 1.0, "%d: got %f\n", line, xf->eM11);
    ok(xf->eM12 == 0.0, "%d: got %f\n", line, xf->eM12);
    ok(xf->eM21 == 0.0, "%d: got %f\n", line, xf->eM21);
    ok(xf->eM22 == 1.0, "%d: got %f\n", line, xf->eM22);
    ok(xf->eDx == 0.0, "%d: got %f\n", line, xf->eDx);
    ok(xf->eDy == 0.0, "%d: got %f\n", line, xf->eDy);
}

static inline void xform_near_match(int line, XFORM *got, XFORM *expect)
{
    ok(fabs(got->eM11 - expect->eM11) < 0.001, "%d: got %f expect %f\n", line, got->eM11, expect->eM11);
    ok(fabs(got->eM12 - expect->eM12) < 0.001, "%d: got %f expect %f\n", line, got->eM12, expect->eM12);
    ok(fabs(got->eM21 - expect->eM21) < 0.001, "%d: got %f expect %f\n", line, got->eM21, expect->eM21);
    ok(fabs(got->eM22 - expect->eM22) < 0.001, "%d: got %f expect %f\n", line, got->eM22, expect->eM22);
    ok(fabs(got->eDx - expect->eDx) < 0.001, "%d: got %f expect %f\n", line, got->eDx, expect->eDx);
    ok(fabs(got->eDy - expect->eDy) < 0.001, "%d: got %f expect %f\n", line, got->eDy, expect->eDy);
}


static void test_gettransform(void)
{
    HDC hdc = CreateICA("DISPLAY", NULL, NULL, NULL);
    XFORM xform, expect;
    BOOL r;
    SIZE lometric_vp, lometric_wnd;

    if(!pGetTransform)
    {
        win_skip("Don't have GetTransform\n");
        return;
    }

    r = pGetTransform(hdc, 0x203, &xform); /* World -> Page */
    ok(r == TRUE, "got %d\n", r);
    expect_identity(__LINE__, &xform);
    r = pGetTransform(hdc, 0x304, &xform); /* Page -> Device */
    ok(r == TRUE, "got %d\n", r);
    expect_identity(__LINE__, &xform);
    r = pGetTransform(hdc, 0x204, &xform); /* World -> Device */
    ok(r == TRUE, "got %d\n", r);
    expect_identity(__LINE__, &xform);
    r = pGetTransform(hdc, 0x402, &xform); /* Device -> World */
    ok(r == TRUE, "got %d\n", r);
    expect_identity(__LINE__, &xform);

    SetMapMode(hdc, MM_LOMETRIC);
    GetViewportExtEx(hdc, &lometric_vp);
    GetWindowExtEx(hdc, &lometric_wnd);

    r = pGetTransform(hdc, 0x203, &xform); /* World -> Page */
    ok(r == TRUE, "got %d\n", r);
    expect_identity(__LINE__, &xform);

    r = pGetTransform(hdc, 0x304, &xform); /* Page -> Device */
    ok(r == TRUE, "got %d\n", r);
    expect.eM11 = (FLOAT) lometric_vp.cx / lometric_wnd.cx;
    expect.eM12 = expect.eM21 = 0.0;
    expect.eM22 = (FLOAT) lometric_vp.cy / lometric_wnd.cy;
    expect.eDx = expect.eDy = 0.0;
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x204, &xform);  /* World -> Device */
    ok(r == TRUE, "got %d\n", r);
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x402, &xform); /* Device -> World */
    ok(r == TRUE, "got %d\n", r);
    expect.eM11 = (FLOAT) lometric_wnd.cx / lometric_vp.cx;
    expect.eM22 = (FLOAT) lometric_wnd.cy / lometric_vp.cy;
    xform_near_match(__LINE__, &xform, &expect);


    SetGraphicsMode(hdc, GM_ADVANCED);

    expect.eM11 = 10.0;
    expect.eM22 = 20.0;
    SetWorldTransform(hdc, &expect);
    r = pGetTransform(hdc, 0x203, &xform);  /* World -> Page */
    ok(r == TRUE, "got %d\n", r);
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x304, &xform); /* Page -> Device */
    ok(r == TRUE, "got %d\n", r);
    expect.eM11 = (FLOAT) lometric_vp.cx / lometric_wnd.cx;
    expect.eM22 = (FLOAT) lometric_vp.cy / lometric_wnd.cy;
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x204, &xform); /* World -> Device */
    ok(r == TRUE, "got %d\n", r);
    expect.eM11 *= 10.0;
    expect.eM22 *= 20.0;
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x402, &xform); /* Device -> World */
    ok(r == TRUE, "got %d\n", r);
    expect.eM11 = 1 / expect.eM11;
    expect.eM22 = 1 / expect.eM22;
    xform_near_match(__LINE__, &xform, &expect);

    r = pGetTransform(hdc, 0x102, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x103, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x104, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x202, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x302, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x303, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x403, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0x404, &xform);
    ok(r == FALSE, "got %d\n", r);
    r = pGetTransform(hdc, 0xffff, &xform);
    ok(r == FALSE, "got %d\n", r);
}

START_TEST(mapping)
{
    HMODULE mod = GetModuleHandleA("gdi32.dll");
    pGetLayout = (void *)GetProcAddress( mod, "GetLayout" );
    pGetRandomRgn = (void *)GetProcAddress( mod, "GetRandomRgn" );
    pGetTransform = (void *)GetProcAddress( mod, "GetTransform" );
    pSetVirtualResolution = (void *)GetProcAddress( mod, "SetVirtualResolution" );

    test_modify_world_transform();
    test_world_transform();
    test_dc_layout();
    test_isotropic_mapping();
    test_setvirtualresolution();
    test_gettransform();
}
