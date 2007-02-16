/*
 * Unit test suite for clipping
 *
 * Copyright 2005 Huw Davies
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
    MoveWindow(hwnd, window_rc.left, window_rc.top, window_rc.right - window_rc.left, window_rc.bottom - window_rc.top, FALSE);
    hdc = GetDC(hwnd);

    ret = GetRandomRgn(hdc, hrgn, 1);
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
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);
 
    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);

    /* Move the clip to the meta and clear the clip */
    SetMetaRgn(hdc);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret == 0, "GetRandomRgn rets %d\n", ret);
    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);

    /* Set a new clip (still got the meta) */
    SetRect(&rc2, 10, 30, 70, 90);
    IntersectClipRect(hdc, rc2.left, rc2.top, rc2.right, rc2.bottom);

    ret = GetRandomRgn(hdc, hrgn, 1);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc2, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);

    ret = GetRandomRgn(hdc, hrgn, 2);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);
 
    IntersectRect(&rc2, &rc, &rc2);

    ret = GetRandomRgn(hdc, hrgn, 3);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    ok(EqualRect(&rc2, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);


    ret = GetRandomRgn(hdc, hrgn, SYSRGN);
    ok(ret != 0, "GetRandomRgn rets %d\n", ret);
    GetRgnBox(hrgn, &ret_rc);
    if(GetVersion() & 0x80000000)
        OffsetRect(&window_rc, -window_rc.left, -window_rc.top);
    ok(EqualRect(&window_rc, &ret_rc), "GetRandomRgn %ld,%ld - %ld,%ld\n",
       ret_rc.left, ret_rc.top, ret_rc.right, ret_rc.bottom);

    DeleteObject(hrgn);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

START_TEST(clipping)
{
    test_GetRandomRgn();
}
