/*
 * Unit test suite for clipping
 *
 * Copyright 2005 Huw Davies
 * Copyright 2008,2011,2013 Dmitry Timoshkov
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

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

static void test_GetRandomRgn(void)
{
    HWND hwnd = CreateWindowExA(0,"BUTTON","test",WS_VISIBLE|WS_POPUP,0,0,100,100,GetDesktopWindow(),0,0,0);
    HDC hdc;
    HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
    int ret;
    RECT rc, rc2;
    RECT ret_rc, window_rc;

    ok( hwnd != 0, "CreateWindow failed\n" );

    SetRect(&window_rc, 400, 300, 500, 400);
    SetWindowPos(hwnd, HWND_TOPMOST, window_rc.left, window_rc.top,
                 window_rc.right - window_rc.left, window_rc.bottom - window_rc.top, 0 );
    hdc = GetDC(hwnd);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);
    ret = GetRandomRgn(hdc, NULL, 1);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);
    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);
    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);

    /* Set a clip region */
    SetRect(&rc, 20, 20, 80, 80);
    IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));
 
    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));

    /* Move the clip to the meta and clear the clip */
    SetMetaRgn(hdc);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);
    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));

    /* Set a new clip (still got the meta) */
    SetRect(&rc2, 10, 30, 70, 90);
    IntersectClipRect(hdc, rc2.left, rc2.top, rc2.right, rc2.bottom);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret > 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc2, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));

    ret = GetRandomRgn(hdc, NULL, 1);
    ok(ret == -1, "GetRandomRgn rets %d\n", ret);

    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));
 
    IntersectRect(&rc2, &rc, &rc2);

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc2, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));


    ret = GetRandomRgn(hdc, hrgn, SYSRGN);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    if(GetVersion() & 0x80000000)
        OffsetRect(&window_rc, -window_rc.left, -window_rc.top);
    /* the window may be partially obscured so the region may be smaller */
    IntersectRect( &window_rc, &ret_rc, &ret_rc );
    ok(EqualRect(&window_rc, &ret_rc), "GetRandomRgn %s\n", wine_dbgstr_rect(&ret_rc));

    DeleteObject(hrgn);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void verify_region(HRGN hrgn, const RECT *rc)
{
    union
    {
        RGNDATA data;
        char buf[sizeof(RGNDATAHEADER) + sizeof(RECT)];
    } rgn;
    const RECT *rect;
    DWORD ret;

    ret = GetRegionData(hrgn, 0, NULL);
    if (IsRectEmpty(rc))
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %lu\n", ret);

    if (!ret) return;

    ret = GetRegionData(hrgn, sizeof(rgn), &rgn.data);
    if (IsRectEmpty(rc))
        ok(ret == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", ret);
    else
        ok(ret == sizeof(rgn.data.rdh) + sizeof(RECT), "expected sizeof(rgn), got %lu\n", ret);

    trace("size %lu, type %lu, count %lu, rgn size %lu, bound %s\n",
          rgn.data.rdh.dwSize, rgn.data.rdh.iType, rgn.data.rdh.nCount, rgn.data.rdh.nRgnSize,
          wine_dbgstr_rect(&rgn.data.rdh.rcBound));
    if (rgn.data.rdh.nCount != 0)
    {
        rect = (const RECT *)rgn.data.Buffer;
        trace("rect %s\n", wine_dbgstr_rect(rect));
        ok(EqualRect(rect, rc), "rects don't match\n");
    }

    ok(rgn.data.rdh.dwSize == sizeof(rgn.data.rdh), "expected sizeof(rdh), got %lu\n", rgn.data.rdh.dwSize);
    ok(rgn.data.rdh.iType == RDH_RECTANGLES, "expected RDH_RECTANGLES, got %lu\n", rgn.data.rdh.iType);
    if (IsRectEmpty(rc))
    {
        ok(rgn.data.rdh.nCount == 0, "expected 0, got %lu\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == 0 ||
           broken(rgn.data.rdh.nRgnSize == 168), /* NT4 */
           "expected 0, got %lu\n", rgn.data.rdh.nRgnSize);
    }
    else
    {
        ok(rgn.data.rdh.nCount == 1, "expected 1, got %lu\n", rgn.data.rdh.nCount);
        ok(rgn.data.rdh.nRgnSize == sizeof(RECT) ||
           broken(rgn.data.rdh.nRgnSize == 168), /* NT4 */
           "expected sizeof(RECT), got %lu\n", rgn.data.rdh.nRgnSize);
    }
    ok(EqualRect(&rgn.data.rdh.rcBound, rc), "rects don't match\n");
}

static void test_ExtCreateRegion(void)
{
    static const RECT empty_rect;
    static const RECT rc = { 111, 222, 333, 444 };
    static const RECT arc[2] = { {0, 0, 10, 10}, {10, 10, 20, 20}};
    static const RECT rc_xformed = { 76, 151, 187, 262 };
    union
    {
        RGNDATA data;
        char buf[sizeof(RGNDATAHEADER) + 2 * sizeof(RECT)];
    } rgn;
    HRGN hrgn;
    XFORM xform;

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, 0, NULL);
    ok(!hrgn, "ExtCreateRegion should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    rgn.data.rdh.dwSize = 0;
    rgn.data.rdh.iType = 0;
    rgn.data.rdh.nCount = 0;
    rgn.data.rdh.nRgnSize = 0;
    SetRectEmpty(&rgn.data.rdh.rcBound);
    memcpy(rgn.data.Buffer, &rc, sizeof(rc));

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(rgn), &rgn.data);
    ok(!hrgn, "ExtCreateRegion should fail\n");
    ok(GetLastError() == 0xdeadbeef, "0xdeadbeef, got %lu\n", GetLastError());

    rgn.data.rdh.dwSize = sizeof(rgn.data.rdh) - 1;

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(rgn), &rgn.data);
    ok(!hrgn, "ExtCreateRegion should fail\n");
    ok(GetLastError() == 0xdeadbeef, "0xdeadbeef, got %lu\n", GetLastError());

    /* although XP doesn't care about the type Win9x does */
    rgn.data.rdh.iType = RDH_RECTANGLES;
    rgn.data.rdh.dwSize = sizeof(rgn.data.rdh);

    /* sizeof(RGNDATAHEADER) is large enough */
    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER), &rgn.data);
    ok(hrgn != 0, "ExtCreateRegion error %lu\n", GetLastError());
    verify_region(hrgn, &empty_rect);
    DeleteObject(hrgn);

    /* Cannot be smaller than sizeof(RGNDATAHEADER) */
    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) - 1, &rgn.data);
    todo_wine
    ok(!hrgn, "ExtCreateRegion should fail\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), "0xdeadbeef, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(rgn), &rgn.data);
    ok(hrgn != 0, "ExtCreateRegion error %lu\n", GetLastError());
    verify_region(hrgn, &empty_rect);
    DeleteObject(hrgn);

    rgn.data.rdh.nCount = 1;
    SetRectEmpty(&rgn.data.rdh.rcBound);
    memcpy(rgn.data.Buffer, &rc, sizeof(rc));

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(rgn), &rgn.data);
    ok(hrgn != 0, "ExtCreateRegion error %lu\n", GetLastError());
    verify_region(hrgn, &rc);
    DeleteObject(hrgn);

    xform.eM11 = 0.5; /* 50% width */
    xform.eM12 = 0.0;
    xform.eM21 = 0.0;
    xform.eM22 = 0.5; /* 50% height */
    xform.eDx = 20.0;
    xform.eDy = 40.0;

    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(&xform, sizeof(rgn), &rgn.data);
    ok(hrgn != 0, "ExtCreateRegion error %lu/%lx\n", GetLastError(), GetLastError());
    verify_region(hrgn, &rc_xformed);
    DeleteObject(hrgn);

    rgn.data.rdh.nCount = 2;
    SetRectEmpty(&rgn.data.rdh.rcBound);
    memcpy(rgn.data.Buffer, arc, sizeof(arc));

    /* Buffer cannot be smaller than sizeof(RGNDATAHEADER) + 2 * sizeof(RECT) */
    SetLastError(0xdeadbeef);
    hrgn = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + 2 * sizeof(RECT) - 1, &rgn.data);
    todo_wine
    ok(!hrgn, "ExtCreateRegion should fail\n");
    ok(GetLastError() == 0xdeadbeef, "0xdeadbeef, got %lu\n", GetLastError());

}

static void test_GetClipRgn(void)
{
    HDC hdc;
    HRGN hrgn, hrgn2, hrgn3, hrgn4;
    int ret;

    /* Test calling GetClipRgn with NULL device context and region handles. */
    ret = GetClipRgn(NULL, NULL);
    ok(ret == -1, "Expected GetClipRgn to return -1, got %d\n", ret);

    hdc = GetDC(NULL);
    ok(hdc != NULL, "Expected GetDC to return a valid device context handle\n");

    /* Test calling GetClipRgn with a valid device context and NULL region. */
    ret = GetClipRgn(hdc, NULL);
    ok(ret == 0, "Expected GetClipRgn to return 0, got %d\n", ret);

    /* Initialize the test regions. */
    hrgn = CreateRectRgn(100, 100, 100, 100);
    ok(hrgn != NULL,
       "Expected CreateRectRgn to return a handle to a new rectangular region\n");

    hrgn2 = CreateRectRgn(1, 2, 3, 4);
    ok(hrgn2 != NULL,
       "Expected CreateRectRgn to return a handle to a new rectangular region\n");

    hrgn3 = CreateRectRgn(1, 2, 3, 4);
    ok(hrgn3 != NULL,
       "Expected CreateRectRgn to return a handle to a new rectangular region\n");

    hrgn4 = CreateRectRgn(1, 2, 3, 4);
    ok(hrgn4 != NULL,
       "Expected CreateRectRgn to return a handle to a new rectangular region\n");

    /* Try getting a clipping region from the device context
     * when the device context's clipping region isn't set. */
    ret = GetClipRgn(hdc, hrgn2);
    ok(ret == 0, "Expected GetClipRgn to return 0, got %d\n", ret);

    /* The region passed to GetClipRgn should be unchanged. */
    ret = EqualRgn(hrgn2, hrgn3);
    ok(ret == 1,
       "Expected EqualRgn to compare the two regions as equal, got %d\n", ret);

    /* Try setting and getting back a clipping region. */
    ret = SelectClipRgn(hdc, hrgn);
    ok(ret == NULLREGION,
       "Expected SelectClipRgn to return NULLREGION, got %d\n", ret);

    /* Passing a NULL region handle when the device context
     * has a clipping region results in an error. */
    ret = GetClipRgn(hdc, NULL);
    ok(ret == -1, "Expected GetClipRgn to return -1, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn2);
    ok(ret == 1, "Expected GetClipRgn to return 1, got %d\n", ret);

    ret = EqualRgn(hrgn, hrgn2);
    ok(ret == 1,
       "Expected EqualRgn to compare the two regions as equal, got %d\n", ret);

    /* Try unsetting and then query the clipping region. */
    ret = SelectClipRgn(hdc, NULL);
    ok(ret == SIMPLEREGION || (ret == COMPLEXREGION && GetSystemMetrics(SM_CMONITORS) > 1),
       "Expected SelectClipRgn to return SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, NULL);
    ok(ret == 0, "Expected GetClipRgn to return 0, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn3);
    ok(ret == 0, "Expected GetClipRgn to return 0, got %d\n", ret);

    ret = EqualRgn(hrgn3, hrgn4);
    ok(ret == 1,
       "Expected EqualRgn to compare the two regions as equal, got %d\n", ret);

    DeleteObject(hrgn4);
    DeleteObject(hrgn3);
    DeleteObject(hrgn2);
    DeleteObject(hrgn);
    ReleaseDC(NULL, hdc);
}

static void test_memory_dc_clipping(void)
{
    HDC hdc;
    HRGN hrgn, hrgn_empty;
    HBITMAP hbmp;
    RECT rc;
    int ret;

    hdc = CreateCompatibleDC(0);
    hrgn_empty = CreateRectRgn(0, 0, 0, 0);
    hrgn = CreateRectRgn(0, 0, 0, 0);
    hbmp = CreateCompatibleBitmap(hdc, 100, 100);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = ExtSelectClipRgn(hdc, hrgn_empty, RGN_DIFF);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "expected 1, got %d\n", ret);

    ret = GetRgnBox(hrgn, &rc);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);
    ok(rc.left == 0 && rc.top == 0 && rc.right == 1 && rc.bottom == 1,
       "expected 0,0-1,1, got %s\n", wine_dbgstr_rect(&rc));

    ret = ExtSelectClipRgn(hdc, 0, RGN_COPY);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = ExtSelectClipRgn(hdc, 0, RGN_DIFF);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    SelectObject(hdc, hbmp);

    ret = ExtSelectClipRgn(hdc, hrgn_empty, RGN_DIFF);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "expected 1, got %d\n", ret);

    ret = GetRgnBox(hrgn, &rc);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);
    ok(rc.left == 0 && rc.top == 0 && rc.right == 100 && rc.bottom == 100,
       "expected 0,0-100,100, got %s\n", wine_dbgstr_rect(&rc));

    SetRect( &rc, 10, 10, 20, 20 );
    ret = RectVisible( hdc, &rc );
    ok(ret, "RectVisible failed for %s\n", wine_dbgstr_rect(&rc));

    SetRect( &rc, 20, 20, 10, 10 );
    ret = RectVisible( hdc, &rc );
    ok(ret, "RectVisible failed for %s\n", wine_dbgstr_rect(&rc));

    ret = ExtSelectClipRgn(hdc, 0, RGN_DIFF);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "expected 1, got %d\n", ret);

    ret = GetRgnBox(hrgn, &rc);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);
    ok(rc.left == 0 && rc.top == 0 && rc.right == 100 && rc.bottom == 100,
       "expected 0,0-100,100, got %s\n", wine_dbgstr_rect(&rc));

    DeleteDC(hdc);
    DeleteObject(hrgn);
    DeleteObject(hrgn_empty);
    DeleteObject(hbmp);
}

static void test_window_dc_clipping(void)
{
    HDC hdc;
    HRGN hrgn, hrgn_empty;
    HWND hwnd;
    RECT rc, virtual_rect;
    int ret, screen_width, screen_height;

    /* Windows versions earlier than Win2k do not support the virtual screen metrics,
     * so we fall back to the primary screen metrics. */
    screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    if(!screen_width) screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if(!screen_height) screen_height = GetSystemMetrics(SM_CYSCREEN);
    SetRect(&virtual_rect, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_XVIRTUALSCREEN) + screen_width, GetSystemMetrics(SM_YVIRTUALSCREEN) + screen_height);

    trace("screen resolution %d x %d\n", screen_width, screen_height);

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           -100, -100, screen_width * 2, screen_height * 2, 0, 0, 0, NULL);
    hdc = GetWindowDC(0);
    hrgn_empty = CreateRectRgn(0, 0, 0, 0);
    hrgn = CreateRectRgn(0, 0, 0, 0);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = ExtSelectClipRgn(hdc, 0, RGN_DIFF);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = ExtSelectClipRgn(hdc, hrgn_empty, RGN_DIFF);
    ok(ret == SIMPLEREGION || (ret == COMPLEXREGION && GetSystemMetrics(SM_CMONITORS) > 1),
       "expected SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "expected 1, got %d\n", ret);

    ret = GetRgnBox(hrgn, &rc);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);
    ok(EqualRect(&rc, &virtual_rect), "expected %s, got %s\n", wine_dbgstr_rect(&virtual_rect),
       wine_dbgstr_rect(&rc));

    SetRect( &rc, 10, 10, 20, 20 );
    ret = RectVisible( hdc, &rc );
    ok( ret, "RectVisible failed for %s\n", wine_dbgstr_rect(&rc));

    SetRect( &rc, 20, 20, 10, 10 );
    ret = RectVisible( hdc, &rc );
    ok( ret, "RectVisible failed for %s\n", wine_dbgstr_rect(&rc));

    ret = ExtSelectClipRgn(hdc, 0, RGN_DIFF);
    ok(ret == 0, "expected 0, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 1, "expected 1, got %d\n", ret);

    ret = GetRgnBox(hrgn, &rc);
    ok(ret == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", ret);
    ok(EqualRect(&rc, &virtual_rect), "expected %s, got %s\n", wine_dbgstr_rect(&virtual_rect),
       wine_dbgstr_rect(&rc));

    ret = ExtSelectClipRgn(hdc, 0, RGN_COPY);
    ok(ret == SIMPLEREGION || (ret == COMPLEXREGION && GetSystemMetrics(SM_CMONITORS) > 1),
       "expected SIMPLEREGION, got %d\n", ret);

    ret = GetClipRgn(hdc, hrgn);
    ok(ret == 0, "expected 0, got %d\n", ret);

    DeleteDC(hdc);
    DeleteObject(hrgn);
    DeleteObject(hrgn_empty);
    DestroyWindow(hwnd);
}

static void test_CreatePolyPolygonRgn(void)
{
    HRGN region;
    POINT points_zero[] = { {0, 0}, {0, 0}, {0, 0} };
    POINT points_mixed[] = { {0, 0}, {0, 0}, {0, 0}, {6, 6}, {6, 6}, {6, 6} };
    POINT points_six[] = { {6, 6}, {6, 6}, {6, 6} };
    POINT points_line[] = { {1, 0}, {11, 0}, {21, 0}};
    POINT points_overflow[] = { {0, 0}, {1, 0}, {0, 0x80000000} };
    INT counts_single_poly[] = { 3 };
    INT counts_two_poly[] = { 3, 3 };
    INT counts_overflow[] = { ARRAY_SIZE(points_overflow) };
    int ret;
    RECT rect;

    /* When all polygons are just one point or line, return must not be NULL */

    region = CreatePolyPolygonRgn(points_zero, counts_single_poly, ARRAY_SIZE(counts_single_poly), ALTERNATE);
    ok (region != NULL, "region must not be NULL\n");
    ret = GetRgnBox(region, &rect);
    ok (ret == NULLREGION, "Expected NULLREGION, got %d\n", ret);
    DeleteObject(region);

    region = CreatePolyPolygonRgn(points_six, counts_single_poly, ARRAY_SIZE(counts_single_poly), ALTERNATE);
    ok (region != NULL, "region must not be NULL\n");
    ret = GetRgnBox(region, &rect);
    ok (ret == NULLREGION, "Expected NULLREGION, got %d\n", ret);
    DeleteObject(region);

    region = CreatePolyPolygonRgn(points_line, counts_single_poly, ARRAY_SIZE(counts_single_poly), ALTERNATE);
    ok (region != NULL, "region must not be NULL\n");
    ret = GetRgnBox(region, &rect);
    ok (ret == NULLREGION, "Expected NULLREGION, got %d\n", ret);
    DeleteObject(region);

    region = CreatePolyPolygonRgn(points_mixed, counts_two_poly, ARRAY_SIZE(counts_two_poly), ALTERNATE);
    ok (region != NULL, "region must not be NULL\n");
    ret = GetRgnBox(region, &rect);
    ok (ret == NULLREGION, "Expected NULLREGION, got %d\n", ret);
    DeleteObject(region);

    /* Test with points that overflow the edge table */
    region = CreatePolyPolygonRgn(points_overflow, counts_overflow, ARRAY_SIZE(counts_overflow), ALTERNATE);
    ok (region != NULL, "region must not be NULL\n");
    ret = GetRgnBox(region, &rect);
    ok (ret == NULLREGION, "Expected NULLREGION, got %d\n", ret);
    DeleteObject(region);
}

START_TEST(clipping)
{
    test_GetRandomRgn();
    test_ExtCreateRegion();
    test_GetClipRgn();
    test_memory_dc_clipping();
    test_window_dc_clipping();
    test_CreatePolyPolygonRgn();
}
