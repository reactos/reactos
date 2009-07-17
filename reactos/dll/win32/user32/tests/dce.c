/*
 * Unit tests for DCE support
 *
 * Copyright 2005 Alexandre Julliard
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
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#ifndef DCX_USESTYLE
#define DCX_USESTYLE         0x00010000
#endif

static HWND hwnd_cache, hwnd_owndc, hwnd_classdc, hwnd_classdc2;

/* test behavior of DC attributes with various GetDC/ReleaseDC combinations */
static void test_dc_attributes(void)
{
    HDC hdc, old_hdc;
    INT rop, def_rop;

    /* test cache DC */

    hdc = GetDC( hwnd_cache );
    def_rop = GetROP2( hdc );

    SetROP2( hdc, R2_WHITE );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d\n", rop );

    ReleaseDC( hwnd_cache, hdc );
    hdc = GetDC( hwnd_cache );
    rop = GetROP2( hdc );
    ok( rop == def_rop, "wrong ROP2 %d after release\n", rop );
    SetROP2( hdc, R2_WHITE );
    ReleaseDC( hwnd_cache, hdc );

    hdc = GetDCEx( hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS );
    rop = GetROP2( hdc );
    /* Win9x seems to silently ignore DCX_NORESETATTRS */
    ok( rop == def_rop || rop == R2_WHITE, "wrong ROP2 %d\n", rop );

    SetROP2( hdc, R2_WHITE );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d\n", rop );

    ReleaseDC( hwnd_cache, hdc );
    hdc = GetDCEx( hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS );
    rop = GetROP2( hdc );
    ok( rop == def_rop || rop == R2_WHITE, "wrong ROP2 %d after release\n", rop );
    ReleaseDC( hwnd_cache, hdc );

    hdc = GetDCEx( hwnd_cache, 0, DCX_USESTYLE );
    rop = GetROP2( hdc );
    ok( rop == def_rop, "wrong ROP2 %d after release\n", rop );
    ReleaseDC( hwnd_cache, hdc );

    /* test own DC */

    hdc = GetDC( hwnd_owndc );
    SetROP2( hdc, R2_WHITE );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d\n", rop );

    old_hdc = hdc;
    ReleaseDC( hwnd_owndc, hdc );
    hdc = GetDC( hwnd_owndc );
    ok( old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d after release\n", rop );
    ReleaseDC( hwnd_owndc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d after second release\n", rop );

    /* test class DC */

    hdc = GetDC( hwnd_classdc );
    SetROP2( hdc, R2_WHITE );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d\n", rop );

    old_hdc = hdc;
    ReleaseDC( hwnd_classdc, hdc );
    hdc = GetDC( hwnd_classdc );
    ok( old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d after release\n", rop );
    ReleaseDC( hwnd_classdc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_WHITE, "wrong ROP2 %d after second release\n", rop );

    /* test class DC with 2 windows */

    old_hdc = GetDC( hwnd_classdc );
    SetROP2( old_hdc, R2_BLACK );
    hdc = GetDC( hwnd_classdc2 );
    ok( old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_BLACK, "wrong ROP2 %d for other window\n", rop );
    ReleaseDC( hwnd_classdc, old_hdc );
    ReleaseDC( hwnd_classdc, hdc );
    rop = GetROP2( hdc );
    ok( rop == R2_BLACK, "wrong ROP2 %d after release\n", rop );
}


/* test behavior with various invalid parameters */
static void test_parameters(void)
{
    HDC hdc;

    hdc = GetDC( hwnd_cache );
    ok( ReleaseDC( hwnd_owndc, hdc ), "ReleaseDC with wrong window should succeed\n" );

    hdc = GetDC( hwnd_cache );
    ok( !ReleaseDC( hwnd_cache, 0 ), "ReleaseDC with wrong HDC should fail\n" );
    ok( ReleaseDC( hwnd_cache, hdc ), "correct ReleaseDC should succeed\n" );
    ok( !ReleaseDC( hwnd_cache, hdc ), "second ReleaseDC should fail\n" );

    hdc = GetDC( hwnd_owndc );
    ok( ReleaseDC( hwnd_cache, hdc ), "ReleaseDC with wrong window should succeed\n" );
    hdc = GetDC( hwnd_owndc );
    ok( ReleaseDC( hwnd_owndc, hdc ), "correct ReleaseDC should succeed\n" );
    ok( ReleaseDC( hwnd_owndc, hdc ), "second ReleaseDC should succeed\n" );

    hdc = GetDC( hwnd_classdc );
    ok( ReleaseDC( hwnd_cache, hdc ), "ReleaseDC with wrong window should succeed\n" );
    hdc = GetDC( hwnd_classdc );
    ok( ReleaseDC( hwnd_classdc, hdc ), "correct ReleaseDC should succeed\n" );
    ok( ReleaseDC( hwnd_classdc, hdc ), "second ReleaseDC should succeed\n" );
}


static void test_dc_visrgn(void)
{
    HDC old_hdc, hdc;
    HRGN hrgn, hrgn2;
    RECT rect;

    /* cache DC */

    SetRect( &rect, 10, 10, 20, 20 );
    MapWindowPoints( hwnd_cache, 0, (POINT *)&rect, 2 );
    hrgn = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_cache, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_cache, hdc );
    ok( GetRgnBox( hrgn, &rect ) == ERROR, "region must no longer be valid\n" );

    /* cache DC with NORESETATTRS */

    SetRect( &rect, 10, 10, 20, 20 );
    MapWindowPoints( hwnd_cache, 0, (POINT *)&rect, 2 );
    hrgn = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_cache, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE | DCX_NORESETATTRS );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_cache, hdc );
    ok( GetRgnBox( hrgn, &rect ) == ERROR, "region must no longer be valid\n" );
    hdc = GetDCEx( hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
        "clip box sould have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ReleaseDC( hwnd_cache, hdc );

    /* window DC */

    SetRect( &rect, 10, 10, 20, 20 );
    MapWindowPoints( hwnd_owndc, 0, (POINT *)&rect, 2 );
    hrgn = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_owndc, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_owndc, hdc );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    hdc = GetDCEx( hwnd_owndc, 0, DCX_USESTYLE );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_owndc, hdc );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );

    SetRect( &rect, 20, 20, 30, 30 );
    MapWindowPoints( hwnd_owndc, 0, (POINT *)&rect, 2 );
    hrgn2 = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_owndc, hrgn2, DCX_INTERSECTRGN | DCX_USESTYLE );
    ok( GetRgnBox( hrgn, &rect ) == ERROR, "region must no longer be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );
    ReleaseDC( hwnd_owndc, hdc );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );
    hdc = GetDCEx( hwnd_owndc, 0, DCX_EXCLUDERGN | DCX_USESTYLE );
    ok( GetRgnBox( hrgn2, &rect ) == ERROR, "region must no longer be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
        "clip box should have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ReleaseDC( hwnd_owndc, hdc );

    /* class DC */

    SetRect( &rect, 10, 10, 20, 20 );
    MapWindowPoints( hwnd_classdc, 0, (POINT *)&rect, 2 );
    hrgn = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_classdc, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_classdc, hdc );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    hdc = GetDCEx( hwnd_classdc, 0, DCX_USESTYLE );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );
    ReleaseDC( hwnd_classdc, hdc );
    ok( GetRgnBox( hrgn, &rect ) != ERROR, "region must still be valid\n" );

    SetRect( &rect, 20, 20, 30, 30 );
    MapWindowPoints( hwnd_classdc, 0, (POINT *)&rect, 2 );
    hrgn2 = CreateRectRgnIndirect( &rect );
    hdc = GetDCEx( hwnd_classdc, hrgn2, DCX_INTERSECTRGN | DCX_USESTYLE );
    ok( GetRgnBox( hrgn, &rect ) == ERROR, "region must no longer be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );

    old_hdc = hdc;
    hdc = GetDCEx( hwnd_classdc2, 0, DCX_USESTYLE );
    ok( old_hdc == hdc, "did not get the same hdc %p/%p\n", old_hdc, hdc );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
        "clip box should have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ReleaseDC( hwnd_classdc2, hdc );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );
    hdc = GetDCEx( hwnd_classdc2, 0, DCX_EXCLUDERGN | DCX_USESTYLE );
    ok( GetRgnBox( hrgn2, &rect ) != ERROR, "region2 must still be valid\n" );
    ok( !(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
        "clip box must have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ReleaseDC( hwnd_classdc2, hdc );
}


/* test various BeginPaint/EndPaint behaviors */
static void test_begin_paint(void)
{
    HDC old_hdc, hdc;
    RECT rect;
    PAINTSTRUCT ps;

    /* cache DC */

    /* clear update region */
    RedrawWindow( hwnd_cache, NULL, 0, RDW_VALIDATE|RDW_NOFRAME|RDW_NOERASE );
    SetRect( &rect, 10, 10, 20, 20 );
    RedrawWindow( hwnd_cache, &rect, 0, RDW_INVALIDATE );
    hdc = BeginPaint( hwnd_cache, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_cache, &ps );

    /* window DC */

    RedrawWindow( hwnd_owndc, NULL, 0, RDW_VALIDATE|RDW_NOFRAME|RDW_NOERASE );
    SetRect( &rect, 10, 10, 20, 20 );
    RedrawWindow( hwnd_owndc, &rect, 0, RDW_INVALIDATE );
    hdc = BeginPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ReleaseDC( hwnd_owndc, hdc );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    ok( GetDC( hwnd_owndc ) == hdc, "got different hdc\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
        "clip box should have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    RedrawWindow( hwnd_owndc, NULL, 0, RDW_VALIDATE|RDW_NOFRAME|RDW_NOERASE );
    SetRect( &rect, 10, 10, 20, 20 );
    RedrawWindow( hwnd_owndc, &rect, 0, RDW_INVALIDATE|RDW_ERASE );
    ok( GetDC( hwnd_owndc ) == hdc, "got different hdc\n" );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
        "clip box should be the whole window %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    RedrawWindow( hwnd_owndc, NULL, 0, RDW_ERASENOW );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
        "clip box should still be the whole window %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    /* class DC */

    RedrawWindow( hwnd_classdc, NULL, 0, RDW_VALIDATE|RDW_NOFRAME|RDW_NOERASE );
    SetRect( &rect, 10, 10, 20, 20 );
    RedrawWindow( hwnd_classdc, &rect, 0, RDW_INVALIDATE );
    hdc = BeginPaint( hwnd_classdc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    old_hdc = hdc;
    hdc = GetDC( hwnd_classdc2 );
    ok( old_hdc == hdc, "did not get the same hdc %p/%p\n", old_hdc, hdc );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( !(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
        "clip box should have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
}

static void test_invisible_create(void)
{
    HWND hwnd_owndc = CreateWindowA("owndc_class", NULL, WS_OVERLAPPED,
                                    0, 200, 100, 100,
                                    0, 0, GetModuleHandleA(0), NULL );
    HDC dc1, dc2;

    dc1 = GetDC(hwnd_owndc);
    dc2 = GetDC(hwnd_owndc);

    ok(dc1 == dc2, "expected owndc dcs to match\n");

    ReleaseDC(hwnd_owndc, dc2);
    ReleaseDC(hwnd_owndc, dc1);
    DestroyWindow(hwnd_owndc);
}

START_TEST(dce)
{
    WNDCLASSA cls;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "cache_class";
    RegisterClassA(&cls);
    cls.style = CS_DBLCLKS | CS_OWNDC;
    cls.lpszClassName = "owndc_class";
    RegisterClassA(&cls);
    cls.style = CS_DBLCLKS | CS_CLASSDC;
    cls.lpszClassName = "classdc_class";
    RegisterClassA(&cls);

    hwnd_cache = CreateWindowA("cache_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                               0, 0, 100, 100,
                               0, 0, GetModuleHandleA(0), NULL );
    hwnd_owndc = CreateWindowA("owndc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                               0, 200, 100, 100,
                               0, 0, GetModuleHandleA(0), NULL );
    hwnd_classdc = CreateWindowA("classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                 200, 0, 100, 100,
                                 0, 0, GetModuleHandleA(0), NULL );
    hwnd_classdc2 = CreateWindowA("classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                  200, 200, 100, 100,
                                  0, 0, GetModuleHandleA(0), NULL );
    test_dc_attributes();
    test_parameters();
    test_dc_visrgn();
    test_begin_paint();
    test_invisible_create();
}
