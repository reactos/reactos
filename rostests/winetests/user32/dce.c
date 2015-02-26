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

static HWND hwnd_cache, hwnd_owndc, hwnd_classdc, hwnd_classdc2, hwnd_parent, hwnd_parentdc;

/* test behavior of DC attributes with various GetDC/ReleaseDC combinations */
static void test_dc_attributes(void)
{
    HDC hdc, old_hdc;
    HDC hdcs[20];
    INT i, rop, def_rop;
    BOOL found_dc;

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
    old_hdc = hdc;

    found_dc = FALSE;
    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx( hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS );
        if (!hdc) break;
        rop = GetROP2( hdc );
        ok( rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc );
        if (hdc == old_hdc)
        {
            found_dc = TRUE;
            SetROP2( hdc, R2_WHITE );
        }
    }
    if (!found_dc)
    {
        trace( "hdc %p not found in cache using %p\n", old_hdc, hdcs[0] );
        old_hdc = hdcs[0];
        SetROP2( old_hdc, R2_WHITE );
    }
    while (i > 0) ReleaseDC( hwnd_cache, hdcs[--i] );

    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx( hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS );
        if (!hdc) break;
        rop = GetROP2( hdc );
        if (hdc == old_hdc)
            ok( rop == R2_WHITE || broken( rop == def_rop),  /* win9x doesn't support DCX_NORESETATTRS */
                "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc );
        else
            ok( rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc );
    }
    while (i > 0) ReleaseDC( hwnd_cache, hdcs[--i] );

    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx( hwnd_cache, 0, DCX_USESTYLE );
        if (!hdc) break;
        rop = GetROP2( hdc );
        if (hdc == old_hdc)
        {
            ok( rop == R2_WHITE || broken( rop == def_rop),
                "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc );
            SetROP2( old_hdc, def_rop );
        }
        else
            ok( rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc );
    }
    while (i > 0) ReleaseDC( hwnd_cache, hdcs[--i] );

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
    RECT rect, parent_rect;

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
        "clip box should have been reset %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
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

    /* parent DC */
    hdc = GetDC( hwnd_parentdc );
    GetClipBox( hdc, &rect );
    ReleaseDC( hwnd_parentdc, hdc );

    hdc = GetDC( hwnd_parent );
    GetClipBox( hdc, &parent_rect );
    ReleaseDC( hwnd_parent, hdc );

    ok( rect.left == parent_rect.left, "rect.left = %d, expected %d\n", rect.left, parent_rect.left );
    ok( rect.top == parent_rect.top, "rect.top = %d, expected %d\n", rect.top, parent_rect.top );
    ok( rect.right == parent_rect.right, "rect.right = %d, expected %d\n", rect.right, parent_rect.right );
    ok( rect.bottom == parent_rect.bottom, "rect.bottom = %d, expected %d\n", rect.bottom, parent_rect.bottom );
}


/* test various BeginPaint/EndPaint behaviors */
static void test_begin_paint(void)
{
    HDC old_hdc, hdc;
    RECT rect, parent_rect;
    PAINTSTRUCT ps;
    COLORREF cr;

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
    ReleaseDC( hwnd_classdc2, hdc );
    EndPaint( hwnd_classdc, &ps );

    /* parent DC */
    RedrawWindow( hwnd_parent, NULL, 0, RDW_VALIDATE|RDW_NOFRAME|RDW_NOERASE );
    RedrawWindow( hwnd_parentdc, NULL, 0, RDW_INVALIDATE );
    hdc = BeginPaint( hwnd_parentdc, &ps );
    GetClipBox( hdc, &rect );
    cr = SetPixel( hdc, 10, 10, RGB(255, 0, 0) );
    ok( cr != -1, "error drawing outside of window client area\n" );
    EndPaint( hwnd_parentdc, &ps );
    GetClientRect( hwnd_parent, &parent_rect );

    ok( rect.left == parent_rect.left, "rect.left = %d, expected %d\n", rect.left, parent_rect.left );
    ok( rect.top == parent_rect.top, "rect.top = %d, expected %d\n", rect.top, parent_rect.top );
    todo_wine ok( rect.right == parent_rect.right, "rect.right = %d, expected %d\n", rect.right, parent_rect.right );
    todo_wine ok( rect.bottom == parent_rect.bottom, "rect.bottom = %d, expected %d\n", rect.bottom, parent_rect.bottom );

    hdc = GetDC( hwnd_parent );
    todo_wine ok( GetPixel( hdc, 10, 10 ) == cr, "error drawing outside of window client area\n" );
    ReleaseDC( hwnd_parent, hdc );
}

/* test ScrollWindow with window DCs */
static void test_scroll_window(void)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT clip, rect;

    /* ScrollWindow uses the window DC, ScrollWindowEx doesn't */

    UpdateWindow( hwnd_owndc );
    SetRect( &clip, 25, 25, 50, 50 );
    ScrollWindow( hwnd_owndc, -5, -10, NULL, &clip );
    hdc = BeginPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_owndc, &ps );

    SetViewportExtEx( hdc, 2, 3, NULL );
    SetViewportOrgEx( hdc, 30, 20, NULL );

    ScrollWindow( hwnd_owndc, -5, -10, NULL, &clip );
    hdc = BeginPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_owndc, &ps );

    ScrollWindowEx( hwnd_owndc, -5, -10, NULL, &clip, 0, NULL, SW_INVALIDATE | SW_ERASE );
    hdc = BeginPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= -5 && rect.top >= 5 && rect.right <= 20 && rect.bottom <= 30,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_owndc, &ps );

    SetViewportExtEx( hdc, 1, 1, NULL );
    SetViewportOrgEx( hdc, 0, 0, NULL );

    ScrollWindowEx( hwnd_owndc, -5, -10, NULL, &clip, 0, NULL, SW_INVALIDATE | SW_ERASE );
    hdc = BeginPaint( hwnd_owndc, &ps );
    SetRectEmpty( &rect );
    GetClipBox( hdc, &rect );
    ok( rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50,
        "invalid clip box %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    EndPaint( hwnd_owndc, &ps );
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

static void test_dc_layout(void)
{
    DWORD (WINAPI *pSetLayout)(HDC hdc, DWORD layout);
    DWORD (WINAPI *pGetLayout)(HDC hdc);
    HWND hwnd_cache_rtl, hwnd_owndc_rtl, hwnd_classdc_rtl, hwnd_classdc2_rtl;
    HDC hdc;
    DWORD layout;
    HMODULE mod = GetModuleHandleA("gdi32.dll");

    pGetLayout = (void *)GetProcAddress( mod, "GetLayout" );
    pSetLayout = (void *)GetProcAddress( mod, "SetLayout" );
    if (!pGetLayout || !pSetLayout)
    {
        win_skip( "Don't have SetLayout\n" );
        return;
    }

    hdc = GetDC( hwnd_cache );
    pSetLayout( hdc, LAYOUT_RTL );
    layout = pGetLayout( hdc );
    ReleaseDC( hwnd_cache, hdc );
    if (!layout)
    {
        win_skip( "SetLayout not supported\n" );
        return;
    }

    hwnd_cache_rtl = CreateWindowExA(WS_EX_LAYOUTRTL, "cache_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                     0, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
    hwnd_owndc_rtl = CreateWindowExA(WS_EX_LAYOUTRTL, "owndc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                     0, 200, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
    hwnd_classdc_rtl = CreateWindowExA(WS_EX_LAYOUTRTL, "classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                       200, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
    hwnd_classdc2_rtl = CreateWindowExA(WS_EX_LAYOUTRTL, "classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                        200, 200, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
    hdc = GetDC( hwnd_cache_rtl );
    layout = pGetLayout( hdc );

    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    pSetLayout( hdc, 0 );
    ReleaseDC( hwnd_cache_rtl, hdc );
    hdc = GetDC( hwnd_owndc_rtl );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    ReleaseDC( hwnd_cache_rtl, hdc );

    hdc = GetDC( hwnd_cache );
    layout = pGetLayout( hdc );
    ok( layout == 0, "wrong layout %x\n", layout );
    ReleaseDC( hwnd_cache, hdc );

    hdc = GetDC( hwnd_owndc_rtl );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    pSetLayout( hdc, 0 );
    ReleaseDC( hwnd_owndc_rtl, hdc );
    hdc = GetDC( hwnd_owndc_rtl );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    ReleaseDC( hwnd_owndc_rtl, hdc );

    hdc = GetDC( hwnd_classdc_rtl );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    pSetLayout( hdc, 0 );
    ReleaseDC( hwnd_classdc_rtl, hdc );
    hdc = GetDC( hwnd_classdc2_rtl );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    ReleaseDC( hwnd_classdc2_rtl, hdc );
    hdc = GetDC( hwnd_classdc );
    layout = pGetLayout( hdc );
    ok( layout == LAYOUT_RTL, "wrong layout %x\n", layout );
    ReleaseDC( hwnd_classdc_rtl, hdc );

    DestroyWindow(hwnd_classdc2_rtl);
    DestroyWindow(hwnd_classdc_rtl);
    DestroyWindow(hwnd_owndc_rtl);
    DestroyWindow(hwnd_cache_rtl);
}

static void test_destroyed_window(void)
{
    HDC dc;

    dc = GetDCEx(hwnd_cache, 0, DCX_USESTYLE);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window.\n", dc);

    dc = GetDCEx(hwnd_owndc, 0, DCX_USESTYLE);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window.\n", dc);

    dc = GetDCEx(hwnd_classdc, 0, DCX_USESTYLE);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window.\n", dc);

    dc = GetDCEx(hwnd_classdc2, 0, DCX_USESTYLE);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window.\n", dc);
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
    cls.style = CS_PARENTDC;
    cls.lpszClassName = "parentdc_class";
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
    hwnd_parent = CreateWindowA("static", NULL, WS_OVERLAPPED | WS_VISIBLE,
                                400, 0, 100, 100, 0, 0, 0, NULL );
    hwnd_parentdc = CreateWindowA("parentdc_class", NULL, WS_CHILD | WS_VISIBLE,
                                  0, 0, 1, 1, hwnd_parent, 0, 0, NULL );

    test_dc_attributes();
    test_parameters();
    test_dc_visrgn();
    test_begin_paint();
    test_scroll_window();
    test_invisible_create();
    test_dc_layout();

    DestroyWindow(hwnd_parent);
    DestroyWindow(hwnd_classdc2);
    DestroyWindow(hwnd_classdc);
    DestroyWindow(hwnd_owndc);
    DestroyWindow(hwnd_cache);

    test_destroyed_window();
}
