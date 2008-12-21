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

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#define rough_match(got, expected) (abs((got) - (expected)) <= 5)

#define expect_LPtoDP(_hdc, _x, _y) \
{ \
    POINT _pt = { 1000, 1000 }; \
    LPtoDP(_hdc, &_pt, 1); \
    ok(rough_match(_pt.x, _x), "expected x %d, got %d\n", (_x), _pt.x); \
    ok(rough_match(_pt.y, _y), "expected y %d, got %d\n", (_y), _pt.y); \
}

#define expect_world_trasform(_hdc, _em11, _em22) \
{ \
    BOOL _ret; \
    XFORM _xform; \
    SetLastError(0xdeadbeef); \
    _ret = GetWorldTransform(_hdc, &_xform); \
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) \
    { \
        ok(_ret, "GetWorldTransform error %u\n", GetLastError()); \
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
    ok(_ret, #_func " error %u\n", GetLastError()); \
    ok(_size.cx == (_cx), "expected cx %d, got %d\n", (_cx), _size.cx); \
    ok(_size.cy == (_cy), "expected cy %d, got %d\n", (_cy), _size.cy); \
}

#define expect_viewport_ext(_hdc, _cx, _cy) expect_dc_ext(GetViewportExtEx, _hdc, _cx, _cy)
#define expect_window_ext(_hdc, _cx, _cy)  expect_dc_ext(GetWindowExtEx, _hdc, _cx, _cy)

static void test_world_transform(void)
{
    BOOL is_win9x;
    HDC hdc;
    INT ret, size_cx, size_cy, res_x, res_y;
    XFORM xform;

    SetLastError(0xdeadbeef);
    GetWorldTransform(0, NULL);
    is_win9x = GetLastError() == ERROR_CALL_NOT_IMPLEMENTED;

    hdc = CreateCompatibleDC(0);

    size_cx = GetDeviceCaps(hdc, HORZSIZE);
    size_cy = GetDeviceCaps(hdc, VERTSIZE);
    res_x = GetDeviceCaps(hdc, HORZRES);
    res_y = GetDeviceCaps(hdc, VERTRES);
    trace("dc size %d x %d, resolution %d x %d\n", size_cx, size_cy, res_x, res_y);

    expect_viewport_ext(hdc, 1, 1);
    expect_window_ext(hdc, 1, 1);
    expect_world_trasform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_LOMETRIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    if (is_win9x)
    {
        expect_viewport_ext(hdc, GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));
        expect_window_ext(hdc, 254, -254);
    }
    else
    {
        expect_viewport_ext(hdc, res_x, -res_y);
        expect_window_ext(hdc, size_cx * 10, size_cy * 10);
    }
    expect_world_trasform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, MulDiv(1000 / 10, res_x, size_cx), -MulDiv(1000 / 10, res_y, size_cy));

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_TEXT);
    ok(ret == MM_LOMETRIC, "expected MM_LOMETRIC, got %d\n", ret);

    expect_viewport_ext(hdc, 1, 1);
    expect_window_ext(hdc, 1, 1);
    expect_world_trasform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    if (!ret)
    {
        DeleteDC(hdc);
        skip("GM_ADVANCED is not supported on this platform\n");
        return;
    }

    expect_viewport_ext(hdc, 1, 1);
    expect_window_ext(hdc, 1, 1);
    expect_world_trasform(hdc, 1.0, 1.0);
    expect_LPtoDP(hdc, 1000, 1000);

    xform.eM11 = 20.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 20.0f;
    xform.eDx = 0.0f;
    xform.eDy = 0.0f;
    SetLastError(0xdeadbeef);
    ret = SetWorldTransform(hdc, &xform);
    ok(ret, "SetWorldTransform error %u\n", GetLastError());

    expect_viewport_ext(hdc, 1, 1);
    expect_window_ext(hdc, 1, 1);
    expect_world_trasform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_LOMETRIC);
    ok(ret == MM_TEXT, "expected MM_TEXT, got %d\n", ret);

    expect_viewport_ext(hdc, res_x, -res_y);
    expect_window_ext(hdc, size_cx * 10, size_cy * 10);
    expect_world_trasform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, MulDiv(1000 * 2, res_x, size_cx), -MulDiv(1000 * 2, res_y, size_cy));

    SetLastError(0xdeadbeef);
    ret = SetMapMode(hdc, MM_TEXT);
    ok(ret == MM_LOMETRIC, "expected MM_LOMETRIC, got %d\n", ret);

    expect_viewport_ext(hdc, 1, 1);
    expect_window_ext(hdc, 1, 1);
    expect_world_trasform(hdc, 20.0, 20.0);
    expect_LPtoDP(hdc, 20000, 20000);

    DeleteDC(hdc);
}

static void test_modify_world_transform(void)
{
    HDC hdc = GetDC(0);
    int ret;

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    if(!ret) /* running in win9x so quit */
    {
        ReleaseDC(0, hdc);
        skip("GM_ADVANCED is not supported on this platform\n");
        return;
    }

    ret = ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    ok(ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_LEFTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_RIGHTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

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
       "Window extension: Expected %dx%d, got %dx%d\n",
       cx, cy, windowExt.cx, windowExt.cy);

    GetViewportExtEx(hdc, &viewportExt);
    ok(rough_match(viewportExt.cx, expected_vp_cx) && rough_match(viewportExt.cy, expected_vp_cy),
        "Viewport extents have not been properly adjusted: Expected %dx%d, got %dx%d\n",
        expected_vp_cx, expected_vp_cy, viewportExt.cx, viewportExt.cy);

    GetWindowOrgEx(hdc, &windowOrgAfter);
    ok(windowOrg.x == windowOrgAfter.x && windowOrg.y == windowOrgAfter.y,
        "Window origin changed from (%d,%d) to (%d,%d)\n",
        windowOrg.x, windowOrg.y, windowOrgAfter.x, windowOrgAfter.y);

    GetViewportOrgEx(hdc, &viewportOrgAfter);
    ok(viewportOrg.x == viewportOrgAfter.x && viewportOrg.y == viewportOrgAfter.y,
        "Viewport origin changed from (%d,%d) to (%d,%d)\n",
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
        "Viewport extents have not been properly adjusted: Expected %dx%d, got %dx%d\n",
        expected_vp_cx, expected_vp_cy, viewportExt.cx, viewportExt.cy);

    GetWindowExtEx(hdc, &windowExtAfter);
    ok(windowExt.cx == windowExtAfter.cx && windowExt.cy == windowExtAfter.cy,
       "Window extension changed from %dx%d to %dx%d\n",
       windowExt.cx, windowExt.cy, windowExtAfter.cx, windowExtAfter.cy);

    GetWindowOrgEx(hdc, &windowOrgAfter);
    ok(windowOrg.x == windowOrgAfter.x && windowOrg.y == windowOrgAfter.y,
        "Window origin changed from (%d,%d) to (%d,%d)\n",
        windowOrg.x, windowOrg.y, windowOrgAfter.x, windowOrgAfter.y);

    GetViewportOrgEx(hdc, &viewportOrgAfter);
    ok(viewportOrg.x == viewportOrgAfter.x && viewportOrg.y == viewportOrgAfter.y,
        "Viewport origin changed from (%d,%d) to (%d,%d)\n",
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

START_TEST(mapping)
{
    skip("ROS-HACK: Skipping mapping tests!\n");
    return;
    test_modify_world_transform();
    test_world_transform();
    test_isotropic_mapping();
}
