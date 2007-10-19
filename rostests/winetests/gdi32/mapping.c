/*
 * Unit tests for mapping functions
 *
 * Copyright (c) 2005 Huw Davies
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


void test_modify_world_transform(void)
{
    HDC hdc = GetDC(0);
    int ret;

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    if(!ret) /* running in win9x so quit */
    {
        ReleaseDC(0, hdc);
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

void test_SetWindowExt(HDC hdc, LONG cx, LONG cy, LONG expected_vp_cx, LONG expected_vp_cy)
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
    ok(viewportExt.cx == expected_vp_cx && viewportExt.cy == expected_vp_cy,
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

void test_SetViewportExt(HDC hdc, LONG cx, LONG cy, LONG expected_vp_cx, LONG expected_vp_cy)
{
    SIZE windowExt, windowExtAfter, viewportExt;
    POINT windowOrg, windowOrgAfter, viewportOrg, viewportOrgAfter;

    GetWindowOrgEx(hdc, &windowOrg);
    GetViewportOrgEx(hdc, &viewportOrg);
    GetWindowExtEx(hdc, &windowExt);

    SetViewportExtEx(hdc, cx, cy, NULL);
    GetViewportExtEx(hdc, &viewportExt);
    ok(viewportExt.cx == expected_vp_cx && viewportExt.cy == expected_vp_cy,
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

void test_isotropic_mapping(void)
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
    test_modify_world_transform();
    test_isotropic_mapping();
}
