/*
 * Unit tests for window handling
 *
 * Copyright 2002 Bill Medland
 * Copyright 2002 Alexandre Julliard
 * Copyright 2003 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* To get ICON_SMALL2 with the MSVC headers */
#define _WIN32_WINNT 0x0501

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#ifndef SPI_GETDESKWALLPAPER
#define SPI_GETDESKWALLPAPER 0x0073
#endif

#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

static HWND (WINAPI *pGetAncestor)(HWND,UINT);
static BOOL (WINAPI *pGetWindowInfo)(HWND,WINDOWINFO*);

static HWND hwndMessage;
static HWND hwndMain, hwndMain2;
static HHOOK hhook;

/* check the values returned by the various parent/owner functions on a given window */
static void check_parents( HWND hwnd, HWND ga_parent, HWND gwl_parent, HWND get_parent,
                           HWND gw_owner, HWND ga_root, HWND ga_root_owner )
{
    HWND res;

    if (pGetAncestor)
    {
        res = pGetAncestor( hwnd, GA_PARENT );
        ok( res == ga_parent, "Wrong result for GA_PARENT %p expected %p\n", res, ga_parent );
    }
    res = (HWND)GetWindowLongA( hwnd, GWL_HWNDPARENT );
    ok( res == gwl_parent, "Wrong result for GWL_HWNDPARENT %p expected %p\n", res, gwl_parent );
    res = GetParent( hwnd );
    ok( res == get_parent, "Wrong result for GetParent %p expected %p\n", res, get_parent );
    res = GetWindow( hwnd, GW_OWNER );
    ok( res == gw_owner, "Wrong result for GW_OWNER %p expected %p\n", res, gw_owner );
    if (pGetAncestor)
    {
        res = pGetAncestor( hwnd, GA_ROOT );
        ok( res == ga_root, "Wrong result for GA_ROOT %p expected %p\n", res, ga_root );
        res = pGetAncestor( hwnd, GA_ROOTOWNER );
        ok( res == ga_root_owner, "Wrong result for GA_ROOTOWNER %p expected %p\n", res, ga_root_owner );
    }
}


static HWND create_tool_window( LONG style, HWND parent )
{
    HWND ret = CreateWindowExA(0, "ToolWindowClass", "Tool window 1", style,
                               0, 0, 100, 100, parent, 0, 0, NULL );
    ok( ret != 0, "Creation failed\n" );
    return ret;
}

/* test parent and owner values for various combinations */
static void test_parent_owner(void)
{
    LONG style;
    HWND test, owner, ret;
    HWND desktop = GetDesktopWindow();
    HWND child = create_tool_window( WS_CHILD, hwndMain );

    trace( "main window %p main2 %p desktop %p child %p\n", hwndMain, hwndMain2, desktop, child );

    /* child without parent, should fail */
    test = CreateWindowExA(0, "ToolWindowClass", "Tool window 1",
                           WS_CHILD, 0, 0, 100, 100, 0, 0, 0, NULL );
    ok( !test, "WS_CHILD without parent created\n" );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    style = GetWindowLongA( desktop, GWL_STYLE );
    ok( !SetWindowLongA( desktop, GWL_STYLE, WS_POPUP ), "Set GWL_STYLE on desktop succeeded\n" );
    ok( !SetWindowLongA( desktop, GWL_STYLE, 0 ), "Set GWL_STYLE on desktop succeeded\n" );
    ok( GetWindowLongA( desktop, GWL_STYLE ) == style, "Desktop style changed\n" );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );
    check_parents( test, hwndMain, hwndMain, hwndMain, 0, hwndMain, hwndMain );
    SetWindowLongA( test, GWL_STYLE, 0 );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP|WS_CHILD );
    check_parents( test, hwndMain, hwndMain, 0, 0, hwndMain, test );
    SetWindowLongA( test, GWL_STYLE, WS_CHILD );
    DestroyWindow( test );

    /* normal child window with WS_MAXIMIZE */
    test = create_tool_window( WS_CHILD | WS_MAXIMIZE, hwndMain );
    DestroyWindow( test );

    /* normal child window with WS_THICKFRAME */
    test = create_tool_window( WS_CHILD | WS_THICKFRAME, hwndMain );
    DestroyWindow( test );

    /* popup window with WS_THICKFRAME */
    test = create_tool_window( WS_POPUP | WS_THICKFRAME, hwndMain );
    DestroyWindow( test );

    /* child of desktop */
    test = create_tool_window( WS_CHILD, desktop );
    trace( "created child of desktop %p\n", test );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongA( test, GWL_STYLE, 0 );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* child of desktop with WS_MAXIMIZE */
    test = create_tool_window( WS_CHILD | WS_MAXIMIZE, desktop );
    DestroyWindow( test );

    /* child of desktop with WS_MINIMIZE */
    test = create_tool_window( WS_CHILD | WS_MINIMIZE, desktop );
    DestroyWindow( test );

    /* child of child */
    test = create_tool_window( WS_CHILD, child );
    trace( "created child of child %p\n", test );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );
    SetWindowLongA( test, GWL_STYLE, 0 );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* child of child with WS_MAXIMIZE */
    test = create_tool_window( WS_CHILD | WS_MAXIMIZE, child );
    DestroyWindow( test );

    /* child of child with WS_MINIMIZE */
    test = create_tool_window( WS_CHILD | WS_MINIMIZE, child );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongA( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window with WS_MAXIMIZE */
    test = create_tool_window( WS_MAXIMIZE, 0 );
    DestroyWindow( test );

    /* owned top-level window */
    test = create_tool_window( 0, hwndMain );
    trace( "created owned top-level %p\n", test );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    SetWindowLongA( test, GWL_STYLE, WS_POPUP );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    SetWindowLongA( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, hwndMain, desktop, hwndMain, test, desktop );
    DestroyWindow( test );

    /* owned top-level window with WS_MAXIMIZE */
    test = create_tool_window( WS_MAXIMIZE, hwndMain );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    SetWindowLongA( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    SetWindowLongA( test, GWL_STYLE, 0 );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, 0 );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain );
    trace( "created owned popup %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    SetWindowLongA( test, GWL_STYLE, WS_CHILD );
    check_parents( test, desktop, hwndMain, desktop, hwndMain, test, desktop );
    SetWindowLongA( test, GWL_STYLE, 0 );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    DestroyWindow( test );

    /* owned popup with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, hwndMain );
    DestroyWindow( test );

    /* top-level window owned by child (same as owned by top-level) */
    test = create_tool_window( 0, child );
    trace( "created top-level owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    DestroyWindow( test );

    /* top-level window owned by child (same as owned by top-level) with WS_MAXIMIZE */
    test = create_tool_window( WS_MAXIMIZE, child );
    DestroyWindow( test );

    /* popup owned by desktop (same as not owned) */
    test = create_tool_window( WS_POPUP, desktop );
    trace( "created popup owned by desktop %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* popup owned by desktop (same as not owned) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, desktop );
    DestroyWindow( test );

    /* popup owned by child (same as owned by top-level) */
    test = create_tool_window( WS_POPUP, child );
    trace( "created popup owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /* popup owned by child (same as owned by top-level) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, child );
    DestroyWindow( test );

    /* not owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, 0 );
    trace( "created WS_CHILD popup %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup with WS_CHILD | WS_MAXIMIZE (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD | WS_MAXIMIZE, 0 );
    DestroyWindow( test );

    /* owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, hwndMain );
    trace( "created owned WS_CHILD popup %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /* owned popup with WS_CHILD (same as WS_POPUP only) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_CHILD | WS_MAXIMIZE, hwndMain );
    DestroyWindow( test );

    /******************** parent changes *************************/
    trace( "testing parent changes\n" );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
#if 0 /* this test succeeds on NT but crashes on win9x systems */
    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( !ret, "Set GWL_HWNDPARENT succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ok( !SetParent( desktop, hwndMain ), "SetParent succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
#endif
    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == hwndMain, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)desktop );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    /* window is now child of desktop so GWL_HWNDPARENT changes owner from now on */
    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, child, desktop, child, test, desktop );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, hwndMain2, 0, hwndMain2, test, test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, desktop, child, 0, child, test, test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, hwndMain2, hwndMain2, hwndMain2, test, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, desktop, child, child, child, test, hwndMain );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = SetParent( test, desktop );
    ok( ret == hwndMain, "SetParent return value %p expected %p\n", ret, hwndMain );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = SetParent( test, hwndMain2 );
    ok( ret == child, "SetParent return value %p expected %p\n", ret, child );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain2 );
    trace( "created owned popup %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    check_parents( test, child, child, hwndMain2, hwndMain2, hwndMain, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (ULONG_PTR)hwndMain );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, hwndMain, hwndMain, hwndMain2, hwndMain2, hwndMain, hwndMain2 );
    DestroyWindow( test );

    /**************** test owner destruction *******************/

    /* owned child popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    /* window is now child of 'child' but owned by 'owner' */
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction\n", test );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    ok( !IsWindow(owner), "Owner %p not destroyed\n", owner );
    DestroyWindow(test);

    /* owned top-level popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow( owner );
    ok( !IsWindow(test), "Window %p not destroyed by owner destruction\n", test );

    /* top-level popup owned by child */
    owner = create_tool_window( WS_CHILD, hwndMain2 );
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (ULONG_PTR)owner );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, owner, owner, owner, test, hwndMain2 );
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction\n", test );
    ok( !IsWindow(owner), "Owner %p not destroyed\n", owner );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow(test);

    /* final cleanup */
    DestroyWindow(child);
}


static LRESULT WINAPI main_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
	case WM_GETMINMAXINFO:
	{
	    MINMAXINFO* minmax = (MINMAXINFO *)lparam;

	    trace("hwnd %p, WM_GETMINMAXINFO, %08x, %08lx\n", hwnd, wparam, lparam);
	    trace("ptReserved (%ld,%ld), ptMaxSize (%ld,%ld), ptMaxPosition (%ld,%ld)\n"
		  "	  ptMinTrackSize (%ld,%ld), ptMaxTrackSize (%ld,%ld)\n",
		  minmax->ptReserved.x, minmax->ptReserved.y,
		  minmax->ptMaxSize.x, minmax->ptMaxSize.y,
		  minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
		  minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
		  minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);
	    SetWindowLongA(hwnd, GWL_USERDATA, 0x20031021);
	    break;
	}
	case WM_WINDOWPOSCHANGING:
	{
	    BOOL is_win9x = GetWindowLongW(hwnd, GWL_WNDPROC) == 0;
	    WINDOWPOS *winpos = (WINDOWPOS *)lparam;
	    trace("main: WM_WINDOWPOSCHANGING\n");
	    trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
		   winpos->hwnd, winpos->hwndInsertAfter,
		   winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);
	    if (!(winpos->flags & SWP_NOMOVE))
	    {
		ok(winpos->x >= -32768 && winpos->x <= 32767, "bad winpos->x %d\n", winpos->x);
		ok(winpos->y >= -32768 && winpos->y <= 32767, "bad winpos->y %d\n", winpos->y);
	    }
	    /* Win9x does not fixup cx/xy for WM_WINDOWPOSCHANGING */
	    if (!(winpos->flags & SWP_NOSIZE) && !is_win9x)
	    {
		ok(winpos->cx >= 0 && winpos->cx <= 32767, "bad winpos->cx %d\n", winpos->cx);
		ok(winpos->cy >= 0 && winpos->cy <= 32767, "bad winpos->cy %d\n", winpos->cy);
	    }
	    break;
	}
	case WM_WINDOWPOSCHANGED:
	{
	    WINDOWPOS *winpos = (WINDOWPOS *)lparam;
	    trace("main: WM_WINDOWPOSCHANGED\n");
	    trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
		   winpos->hwnd, winpos->hwndInsertAfter,
		   winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);
	    ok(winpos->x >= -32768 && winpos->x <= 32767, "bad winpos->x %d\n", winpos->x);
	    ok(winpos->y >= -32768 && winpos->y <= 32767, "bad winpos->y %d\n", winpos->y);

	    ok(winpos->cx >= 0 && winpos->cx <= 32767, "bad winpos->cx %d\n", winpos->cx);
	    ok(winpos->cy >= 0 && winpos->cy <= 32767, "bad winpos->cy %d\n", winpos->cy);
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongA(hwnd, GWL_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

	    trace("WM_NCCREATE: hwnd %p, parent %p, style %08lx\n", hwnd, cs->hwndParent, cs->style);
	    if (got_getminmaxinfo)
		trace("%p got WM_GETMINMAXINFO\n", hwnd);

	    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
		ok(got_getminmaxinfo, "main: WM_GETMINMAXINFO should have been received before WM_NCCREATE\n");
	    else
		ok(!got_getminmaxinfo, "main: WM_GETMINMAXINFO should NOT have been received before WM_NCCREATE\n");
	    break;
	}
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI tool_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
	case WM_GETMINMAXINFO:
	{
	    MINMAXINFO* minmax = (MINMAXINFO *)lparam;

	    trace("hwnd %p, WM_GETMINMAXINFO, %08x, %08lx\n", hwnd, wparam, lparam);
	    trace("ptReserved (%ld,%ld), ptMaxSize (%ld,%ld), ptMaxPosition (%ld,%ld)\n"
		  "	  ptMinTrackSize (%ld,%ld), ptMaxTrackSize (%ld,%ld)\n",
		  minmax->ptReserved.x, minmax->ptReserved.y,
		  minmax->ptMaxSize.x, minmax->ptMaxSize.y,
		  minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
		  minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
		  minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);
	    SetWindowLongA(hwnd, GWL_USERDATA, 0x20031021);
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongA(hwnd, GWL_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

	    trace("WM_NCCREATE: hwnd %p, parent %p, style %08lx\n", hwnd, cs->hwndParent, cs->style);
	    if (got_getminmaxinfo)
		trace("%p got WM_GETMINMAXINFO\n", hwnd);

	    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
		ok(got_getminmaxinfo, "tool: WM_GETMINMAXINFO should have been received before WM_NCCREATE\n");
	    else
		ok(!got_getminmaxinfo, "tool: WM_GETMINMAXINFO should NOT have been received before WM_NCCREATE\n");
	    break;
	}
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = main_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MainWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    cls.style = 0;
    cls.lpfnWndProc = tool_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "ToolWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

static void verify_window_info(HWND hwnd, const WINDOWINFO *info, BOOL test_borders)
{
    RECT rcWindow, rcClient;
    INT border;
    DWORD status;

    ok(IsWindow(hwnd), "bad window handle\n");

    GetWindowRect(hwnd, &rcWindow);
    ok(EqualRect(&rcWindow, &info->rcWindow), "wrong rcWindow\n");

    GetClientRect(hwnd, &rcClient);
    /* translate to screen coordinates */
    MapWindowPoints(hwnd, 0, (LPPOINT)&rcClient, 2);
    ok(EqualRect(&rcClient, &info->rcClient), "wrong rcClient\n");

    ok(info->dwStyle == GetWindowLongA(hwnd, GWL_STYLE), "wrong dwStyle\n");
    ok(info->dwExStyle == GetWindowLongA(hwnd, GWL_EXSTYLE), "wrong dwExStyle\n");
    status = (GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0;
    ok(info->dwWindowStatus == status, "wrong dwWindowStatus\n");

    if (test_borders && !IsRectEmpty(&rcWindow))
    {
	trace("rcWindow: %ld,%ld - %ld,%ld\n", rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
	trace("rcClient: %ld,%ld - %ld,%ld\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	ok(info->cxWindowBorders == rcClient.left - rcWindow.left,
	    "wrong cxWindowBorders %d != %ld\n", info->cxWindowBorders, rcClient.left - rcWindow.left);
	border = min(rcWindow.bottom - rcClient.bottom, rcClient.top - rcWindow.top);
	ok(info->cyWindowBorders == border,
	   "wrong cyWindowBorders %d != %d\n", info->cyWindowBorders, border);
    }

    ok(info->atomWindowType == GetClassLongA(hwnd, GCW_ATOM), "wrong atomWindowType\n");
    ok(info->wCreatorVersion == 0x0400, "wrong wCreatorVersion %04x\n", info->wCreatorVersion);
}

static LRESULT CALLBACK cbt_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    static const char *CBT_code_name[10] = {
	"HCBT_MOVESIZE",
	"HCBT_MINMAX",
	"HCBT_QS",
	"HCBT_CREATEWND",
	"HCBT_DESTROYWND",
	"HCBT_ACTIVATE",
	"HCBT_CLICKSKIPPED",
	"HCBT_KEYSKIPPED",
	"HCBT_SYSCOMMAND",
	"HCBT_SETFOCUS" };
    const char *code_name = (nCode >= 0 && nCode <= HCBT_SETFOCUS) ? CBT_code_name[nCode] : "Unknown";

    trace("CBT: %d (%s), %08x, %08lx\n", nCode, code_name, wParam, lParam);

    switch (nCode)
    {
	case HCBT_CREATEWND:
	{
#if 0 /* Uncomment this once the test succeeds in all cases */
	    static const RECT rc_null;
	    RECT rc;
#endif
	    LONG style;
	    CBT_CREATEWNDA *createwnd = (CBT_CREATEWNDA *)lParam;
	    trace("HCBT_CREATEWND: hwnd %p, parent %p, style %08lx\n",
		  (HWND)wParam, createwnd->lpcs->hwndParent, createwnd->lpcs->style);
	    ok(createwnd->hwndInsertAfter == HWND_TOP, "hwndInsertAfter should be always HWND_TOP\n");

	    /* WS_VISIBLE should be turned off yet */
	    style = createwnd->lpcs->style & ~WS_VISIBLE;
	    ok(style == GetWindowLongA((HWND)wParam, GWL_STYLE),
		"style of hwnd and style in the CREATESTRUCT do not match: %08lx != %08lx\n",
		GetWindowLongA((HWND)wParam, GWL_STYLE), style);

#if 0 /* Uncomment this once the test succeeds in all cases */
	    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
	    {
		ok(GetParent((HWND)wParam) == hwndMessage,
		   "wrong result from GetParent %p: message window %p\n",
		   GetParent((HWND)wParam), hwndMessage);
	    }
	    else
		ok(!GetParent((HWND)wParam), "GetParent should return 0 at this point\n");

	    ok(!GetWindow((HWND)wParam, GW_OWNER), "GW_OWNER should be set to 0 at this point\n");
#endif
#if 0	    /* while NT assigns GW_HWNDFIRST/LAST some values at this point,
	     * Win9x still has them set to 0.
	     */
	    ok(GetWindow((HWND)wParam, GW_HWNDFIRST) != 0, "GW_HWNDFIRST should not be set to 0 at this point\n");
	    ok(GetWindow((HWND)wParam, GW_HWNDLAST) != 0, "GW_HWNDLAST should not be set to 0 at this point\n");
#endif
	    ok(!GetWindow((HWND)wParam, GW_HWNDPREV), "GW_HWNDPREV should be set to 0 at this point\n");
	    ok(!GetWindow((HWND)wParam, GW_HWNDNEXT), "GW_HWNDNEXT should be set to 0 at this point\n");

#if 0 /* Uncomment this once the test succeeds in all cases */
	    if (pGetAncestor)
	    {
		ok(pGetAncestor((HWND)wParam, GA_PARENT) == hwndMessage, "GA_PARENT should be set to hwndMessage at this point\n");
		ok(pGetAncestor((HWND)wParam, GA_ROOT) == (HWND)wParam,
		   "GA_ROOT is set to %p, expected %p\n", pGetAncestor((HWND)wParam, GA_ROOT), (HWND)wParam);

		if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
		    ok(pGetAncestor((HWND)wParam, GA_ROOTOWNER) == hwndMessage,
		       "GA_ROOTOWNER should be set to hwndMessage at this point\n");
		else
		    ok(pGetAncestor((HWND)wParam, GA_ROOTOWNER) == (HWND)wParam,
		       "GA_ROOTOWNER is set to %p, expected %p\n", pGetAncestor((HWND)wParam, GA_ROOTOWNER), (HWND)wParam);
            }

	    ok(GetWindowRect((HWND)wParam, &rc), "GetWindowRect failed\n");
	    ok(EqualRect(&rc, &rc_null), "window rect should be set to 0 HCBT_CREATEWND\n");
	    ok(GetClientRect((HWND)wParam, &rc), "GetClientRect failed\n");
	    ok(EqualRect(&rc, &rc_null), "client rect should be set to 0 on HCBT_CREATEWND\n");
#endif
	    break;
	}

	case HCBT_DESTROYWND:
	case HCBT_SETFOCUS:
	    if (wParam && pGetWindowInfo)
	    {
		WINDOWINFO info;

		info.cbSize = 0;
		ok(pGetWindowInfo((HWND)wParam, &info), "GetWindowInfo should not fail\n");
		/* win2k SP4 returns broken border info if GetWindowInfo
		 * is being called from HCBT_DESTROYWND hook proc.
		 */
		verify_window_info((HWND)wParam, &info, nCode != HCBT_DESTROYWND);

		info.cbSize = sizeof(WINDOWINFO) + 1;
		ok(pGetWindowInfo((HWND)wParam, &info), "GetWindowInfo should not fail\n");
		verify_window_info((HWND)wParam, &info, nCode != HCBT_DESTROYWND);
	    }
	    break;
    }

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

static void test_shell_window()
{
    BOOL ret;
    DWORD error;
    HMODULE hinst, hUser32;
    BOOL (WINAPI*SetShellWindow)(HWND);
    BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);
    HWND hwnd1, hwnd2, hwnd3, hwnd4, hwnd5;
    HWND shellWindow, nextWnd;

    shellWindow = GetShellWindow();
    hinst = GetModuleHandle(0);
    hUser32 = GetModuleHandleA("user32");

    SetShellWindow = (void *)GetProcAddress(hUser32, "SetShellWindow");
    SetShellWindowEx = (void *)GetProcAddress(hUser32, "SetShellWindowEx");

    trace("previous shell window: %p\n", shellWindow);

    if (shellWindow) {
        DWORD pid;
        HANDLE hProcess;

        ret = DestroyWindow(shellWindow);
        error = GetLastError();

        ok(!ret, "DestroyWindow(shellWindow)\n");
        /* passes on Win XP, but not on Win98
        ok(error==ERROR_ACCESS_DENIED, "ERROR_ACCESS_DENIED after DestroyWindow(shellWindow)\n"); */

        /* close old shell instance */
        GetWindowThreadProcessId(shellWindow, &pid);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        ret = TerminateProcess(hProcess, 0);
        ok(ret, "termination of previous shell process failed: GetLastError()=%ld\n", GetLastError());
        WaitForSingleObject(hProcess, INFINITE);    /* wait for termination */
        CloseHandle(hProcess);
    }

    hwnd1 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST1"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 100, 100, 300, 200, 0, 0, hinst, 0);
    trace("created window 1: %p\n", hwnd1);

    ret = SetShellWindow(hwnd1);
    ok(ret, "first call to SetShellWindow(hwnd1)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd1, "wrong shell window: %p\n", shellWindow);

    ret = SetShellWindow(hwnd1);
    ok(!ret, "second call to SetShellWindow(hwnd1)\n");

    ret = SetShellWindow(0);
    error = GetLastError();
    /* passes on Win XP, but not on Win98
    ok(!ret, "reset shell window by SetShellWindow(0)\n");
    ok(error==ERROR_INVALID_WINDOW_HANDLE, "ERROR_INVALID_WINDOW_HANDLE after SetShellWindow(0)\n"); */

    ret = SetShellWindow(hwnd1);
    /* passes on Win XP, but not on Win98
    ok(!ret, "third call to SetShellWindow(hwnd1)\n"); */

    todo_wine
    {
        SetWindowLong(hwnd1, GWL_EXSTYLE, GetWindowLong(hwnd1,GWL_EXSTYLE)|WS_EX_TOPMOST);
        ret = GetWindowLong(hwnd1,GWL_EXSTYLE)&WS_EX_TOPMOST? TRUE: FALSE;
        ok(!ret, "SetWindowExStyle(hwnd1, WS_EX_TOPMOST)\n");
    }

    ret = DestroyWindow(hwnd1);
    ok(ret, "DestroyWindow(hwnd1)\n");

    hwnd2 = CreateWindowEx(WS_EX_TOPMOST, TEXT("#32770"), TEXT("TEST2"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 150, 250, 300, 200, 0, 0, hinst, 0);
    trace("created window 2: %p\n", hwnd2);
    ret = SetShellWindow(hwnd2);
    ok(!ret, "SetShellWindow(hwnd2) with WS_EX_TOPMOST\n");

    hwnd3 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST3"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 200, 400, 300, 200, 0, 0, hinst, 0);
    trace("created window 3: %p\n", hwnd3);

    hwnd4 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST4"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 250, 500, 300, 200, 0, 0, hinst, 0);
    trace("created window 4: %p\n", hwnd4);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==hwnd3, "wrong next window for hwnd4: %p - expected hwnd3\n", nextWnd);

    ret = SetShellWindow(hwnd4);
    ok(ret, "SetShellWindow(hwnd4)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected hwnd4\n", shellWindow);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==0, "wrong next window for hwnd4: %p - expected 0\n", nextWnd);

    ret = SetWindowPos(hwnd4, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, HWND_TOPMOST)\n");

    ret = SetWindowPos(hwnd4, hwnd3, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, hwnd3\n");

    ret = SetShellWindow(hwnd3);
    ok(!ret, "SetShellWindow(hwnd3)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected hwnd4\n", shellWindow);

    hwnd5 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST5"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 300, 600, 300, 200, 0, 0, hinst, 0);
    trace("created window 5: %p\n", hwnd5);
    ret = SetWindowPos(hwnd4, hwnd5, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, hwnd5)\n");

    todo_wine
    {
        nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
        ok(nextWnd==0, "wrong next window for hwnd4 after SetWindowPos(): %p - expected 0\n", nextWnd);
    }

    /* destroy test windows */
    DestroyWindow(hwnd2);
    DestroyWindow(hwnd3);
    DestroyWindow(hwnd4);
    DestroyWindow(hwnd5);
}

/************** MDI test ****************/

static const char mdi_lParam_test_message[] = "just a test string";

static void test_MDI_create(HWND parent, HWND mdi_client)
{
    MDICREATESTRUCTA mdi_cs;
    HWND mdi_child;
    static const WCHAR classW[] = {'M','D','I','_','c','h','i','l','d','_','C','l','a','s','s','_','1',0};
    static const WCHAR titleW[] = {'M','D','I',' ','c','h','i','l','d',0};
    BOOL isWin9x = FALSE;

    mdi_cs.szClass = "MDI_child_Class_1";
    mdi_cs.szTitle = "MDI child";
    mdi_cs.hOwner = GetModuleHandle(0);
    mdi_cs.x = CW_USEDEFAULT;
    mdi_cs.y = CW_USEDEFAULT;
    mdi_cs.cx = CW_USEDEFAULT;
    mdi_cs.cy = CW_USEDEFAULT;
    mdi_cs.style = 0;
    mdi_cs.lParam = (LPARAM)mdi_lParam_test_message;
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_cs.style = 0x7fffffff; /* without WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_cs.style = 0xffffffff; /* with WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
        DestroyWindow(mdi_child);
    }
    else
        ok(mdi_child != 0, "MDI child creation failed\n");

    /* test MDICREATESTRUCT A<->W mapping */
    /* MDICREATESTRUCTA and MDICREATESTRUCTW have the same layout */
    mdi_cs.style = 0;
    mdi_cs.szClass = (LPCSTR)classW;
    mdi_cs.szTitle = (LPCSTR)titleW;
    SetLastError(0xdeadbeef);
    mdi_child = (HWND)SendMessageW(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    if (!mdi_child)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            isWin9x = TRUE;
        else
            ok(mdi_child != 0, "MDI child creation failed\n");
    }
    DestroyWindow(mdi_child);

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0x7fffffff, /* without WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0xffffffff, /* with WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
        DestroyWindow(mdi_child);
    }
    else
        ok(mdi_child != 0, "MDI child creation failed\n");

    /* test MDICREATESTRUCT A<->W mapping */
    SetLastError(0xdeadbeef);
    mdi_child = CreateMDIWindowW(classW, titleW,
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    if (!mdi_child)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            isWin9x = TRUE;
        else
            ok(mdi_child != 0, "MDI child creation failed\n");
    }
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0x7fffffff, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0xffffffff, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
        DestroyWindow(mdi_child);
    }
    else
        ok(mdi_child != 0, "MDI child creation failed\n");

    /* test MDICREATESTRUCT A<->W mapping */
    SetLastError(0xdeadbeef);
    mdi_child = CreateWindowExW(WS_EX_MDICHILD, classW, titleW,
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    if (!mdi_child)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            isWin9x = TRUE;
        else
            ok(mdi_child != 0, "MDI child creation failed\n");
    }
    DestroyWindow(mdi_child);

    /* This test fails on Win9x */
    if (!isWin9x)
    {
        mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_2", "MDI child",
                                WS_CHILD,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parent, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
        ok(!mdi_child, "WS_EX_MDICHILD with a not MDIClient parent should fail\n");
    }

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_POPUP, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    /* maximized child */
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption and a thick frame\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION | WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    DestroyWindow(mdi_child);
}

/**********************************************************************
 * MDI_ChildGetMinMaxInfo (copied from windows/mdi.c)
 *
 * Note: The rule here is that client rect of the maximized MDI child
 *	 is equal to the client rect of the MDI client window.
 */
static void MDI_ChildGetMinMaxInfo( HWND client, HWND hwnd, MINMAXINFO* lpMinMax )
{
    RECT rect;

    GetClientRect( client, &rect );
    AdjustWindowRectEx( &rect, GetWindowLongA( hwnd, GWL_STYLE ),
                        0, GetWindowLongA( hwnd, GWL_EXSTYLE ));

    rect.right -= rect.left;
    rect.bottom -= rect.top;
    lpMinMax->ptMaxSize.x = rect.right;
    lpMinMax->ptMaxSize.y = rect.bottom;

    lpMinMax->ptMaxPosition.x = rect.left;
    lpMinMax->ptMaxPosition.y = rect.top;

    trace("max rect (%ld,%ld - %ld, %ld)\n",
           rect.left, rect.top, rect.right, rect.bottom);
}

static LRESULT WINAPI mdi_child_wnd_proc_1(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_NCCREATE:
        case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;
            MDICREATESTRUCTA *mdi_cs = (MDICREATESTRUCTA *)cs->lpCreateParams;

            ok(cs->dwExStyle & WS_EX_MDICHILD, "WS_EX_MDICHILD should be set\n");
            ok(mdi_cs->lParam == (LPARAM)mdi_lParam_test_message, "wrong mdi_cs->lParam\n");

            ok(!lstrcmpA(cs->lpszClass, "MDI_child_Class_1"), "wrong class name\n");
            ok(!lstrcmpA(cs->lpszClass, mdi_cs->szClass), "class name does not match\n");
            ok(!lstrcmpA(cs->lpszName, "MDI child"), "wrong title\n");
            ok(!lstrcmpA(cs->lpszName, mdi_cs->szTitle), "title does not match\n");
            ok(cs->hInstance == mdi_cs->hOwner, "%p != %p\n", cs->hInstance, mdi_cs->hOwner);

            /* MDICREATESTRUCT should have original values */
            ok(mdi_cs->style == 0 || mdi_cs->style == 0x7fffffff || mdi_cs->style == 0xffffffff,
                "mdi_cs->style does not match (%08lx)\n", mdi_cs->style);
            ok(mdi_cs->x == CW_USEDEFAULT, "%d != CW_USEDEFAULT\n", mdi_cs->x);
            ok(mdi_cs->y == CW_USEDEFAULT, "%d != CW_USEDEFAULT\n", mdi_cs->y);
            ok(mdi_cs->cx == CW_USEDEFAULT, "%d != CW_USEDEFAULT\n", mdi_cs->cx);
            ok(mdi_cs->cy == CW_USEDEFAULT, "%d != CW_USEDEFAULT\n", mdi_cs->cy);

            /* CREATESTRUCT should have fixed values */
            ok(cs->x != CW_USEDEFAULT, "%d == CW_USEDEFAULT\n", cs->x);
            ok(cs->y != CW_USEDEFAULT, "%d == CW_USEDEFAULT\n", cs->y);

            /* cx/cy == CW_USEDEFAULT are translated to NOT zero values */
            ok(cs->cx != CW_USEDEFAULT && cs->cx != 0, "%d == CW_USEDEFAULT\n", cs->cx);
            ok(cs->cy != CW_USEDEFAULT && cs->cy != 0, "%d == CW_USEDEFAULT\n", cs->cy);

            ok(!(cs->style & WS_POPUP), "WS_POPUP is not allowed\n");

            if (GetWindowLongA(cs->hwndParent, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
            {
                LONG style = mdi_cs->style | WS_CHILD | WS_CLIPSIBLINGS;
                ok(cs->style == style,
                   "cs->style does not match (%08lx)\n", cs->style);
            }
            else
            {
                LONG style = mdi_cs->style;
                style &= ~WS_POPUP;
                style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                    WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
                ok(cs->style == style,
                   "cs->style does not match (%08lx)\n", cs->style);
            }
            break;
        }

        case WM_GETMINMAXINFO:
        {
            HWND client = GetParent(hwnd);
            RECT rc;
            MINMAXINFO *minmax = (MINMAXINFO *)lparam;
            MINMAXINFO my_minmax;
            LONG style, exstyle;

            style = GetWindowLongA(hwnd, GWL_STYLE);
            exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

            GetWindowRect(client, &rc);
            trace("MDI client %p window size = (%ld x %ld)\n", client, rc.right-rc.left, rc.bottom-rc.top);
            GetClientRect(client, &rc);
            trace("MDI client %p client size = (%ld x %ld)\n", client, rc.right, rc.bottom);
            trace("screen size: %d x %d\n", GetSystemMetrics(SM_CXSCREEN),
                                            GetSystemMetrics(SM_CYSCREEN));

            GetClientRect(client, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
            trace("MDI child: calculated max window size = (%ld x %ld)\n", rc.right-rc.left, rc.bottom-rc.top);

            trace("ptReserved = (%ld,%ld)\n"
                  "ptMaxSize = (%ld,%ld)\n"
                  "ptMaxPosition = (%ld,%ld)\n"
                  "ptMinTrackSize = (%ld,%ld)\n"
                  "ptMaxTrackSize = (%ld,%ld)\n",
                  minmax->ptReserved.x, minmax->ptReserved.y,
                  minmax->ptMaxSize.x, minmax->ptMaxSize.y,
                  minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
                  minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
                  minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %ld != %ld\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);

            DefMDIChildProcA(hwnd, msg, wparam, lparam);

            trace("DefMDIChildProc returned:\n"
                  "ptReserved = (%ld,%ld)\n"
                  "ptMaxSize = (%ld,%ld)\n"
                  "ptMaxPosition = (%ld,%ld)\n"
                  "ptMinTrackSize = (%ld,%ld)\n"
                  "ptMaxTrackSize = (%ld,%ld)\n",
                  minmax->ptReserved.x, minmax->ptReserved.y,
                  minmax->ptMaxSize.x, minmax->ptMaxSize.y,
                  minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
                  minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
                  minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);

            MDI_ChildGetMinMaxInfo(client, hwnd, &my_minmax);
            ok(minmax->ptMaxSize.x == my_minmax.ptMaxSize.x, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, my_minmax.ptMaxSize.x);
            ok(minmax->ptMaxSize.y == my_minmax.ptMaxSize.y, "default height of maximized child %ld != %ld\n",
               minmax->ptMaxSize.y, my_minmax.ptMaxSize.y);

            return 1;
        }
    }
    return DefMDIChildProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI mdi_child_wnd_proc_2(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_NCCREATE:
        case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

            trace("%s\n", (msg == WM_NCCREATE) ? "WM_NCCREATE" : "WM_CREATE");
            trace("x %d, y %d, cx %d, cy %d\n", cs->x, cs->y, cs->cx, cs->cy);

            ok(!(cs->dwExStyle & WS_EX_MDICHILD), "WS_EX_MDICHILD should not be set\n");
            ok(cs->lpCreateParams == mdi_lParam_test_message, "wrong cs->lpCreateParams\n");

            ok(!lstrcmpA(cs->lpszClass, "MDI_child_Class_2"), "wrong class name\n");
            ok(!lstrcmpA(cs->lpszName, "MDI child"), "wrong title\n");

            /* CREATESTRUCT should have fixed values */
            /* For some reason Win9x doesn't translate cs->x from CW_USEDEFAULT,
               while NT does. */
            /*ok(cs->x != CW_USEDEFAULT, "%d == CW_USEDEFAULT\n", cs->x);*/
            ok(cs->y != CW_USEDEFAULT, "%d == CW_USEDEFAULT\n", cs->y);

            /* cx/cy == CW_USEDEFAULT are translated to 0 */
            ok(cs->cx == 0, "%d != 0\n", cs->cx);
            ok(cs->cy == 0, "%d != 0\n", cs->cy);
            break;
        }

        case WM_GETMINMAXINFO:
        {
            HWND parent = GetParent(hwnd);
            RECT rc;
            MINMAXINFO *minmax = (MINMAXINFO *)lparam;
            LONG style, exstyle;

            trace("WM_GETMINMAXINFO\n");

            style = GetWindowLongA(hwnd, GWL_STYLE);
            exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

            GetClientRect(parent, &rc);
            trace("parent %p client size = (%ld x %ld)\n", parent, rc.right, rc.bottom);

            GetClientRect(parent, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
            trace("calculated max child window size = (%ld x %ld)\n", rc.right-rc.left, rc.bottom-rc.top);

            trace("ptReserved = (%ld,%ld)\n"
                  "ptMaxSize = (%ld,%ld)\n"
                  "ptMaxPosition = (%ld,%ld)\n"
                  "ptMinTrackSize = (%ld,%ld)\n"
                  "ptMaxTrackSize = (%ld,%ld)\n",
                  minmax->ptReserved.x, minmax->ptReserved.y,
                  minmax->ptMaxSize.x, minmax->ptMaxSize.y,
                  minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
                  minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
                  minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %ld != %ld\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);
            break;
        }

        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            WINDOWPOS my_winpos = *winpos;

            trace("%s\n", (msg == WM_WINDOWPOSCHANGING) ? "WM_WINDOWPOSCHANGING" : "WM_WINDOWPOSCHANGED");
            trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                  winpos->hwnd, winpos->hwndInsertAfter,
                  winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

            DefWindowProcA(hwnd, msg, wparam, lparam);

            trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                  winpos->hwnd, winpos->hwndInsertAfter,
                  winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

            ok(!memcmp(&my_winpos, winpos, sizeof(WINDOWPOS)),
               "DefWindowProc should not change WINDOWPOS values\n");

            return 1;
        }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI mdi_main_wnd_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static HWND mdi_client;

    switch (msg)
    {
        case WM_CREATE:
        {
            CLIENTCREATESTRUCT client_cs;
            RECT rc;

            GetClientRect(hwnd, &rc);

            client_cs.hWindowMenu = 0;
            client_cs.idFirstChild = 1;

            /* MDIClient without MDIS_ALLCHILDSTYLES */
            mdi_client = CreateWindowExA(0, "mdiclient",
                                         NULL,
                                         WS_CHILD /*| WS_VISIBLE*/,
                                          /* tests depend on a not zero MDIClient size */
                                         0, 0, rc.right, rc.bottom,
                                         hwnd, 0, GetModuleHandle(0),
                                         (LPVOID)&client_cs);
            assert(mdi_client);
            test_MDI_create(hwnd, mdi_client);
            DestroyWindow(mdi_client);

            /* MDIClient with MDIS_ALLCHILDSTYLES */
            mdi_client = CreateWindowExA(0, "mdiclient",
                                         NULL,
                                         WS_CHILD | MDIS_ALLCHILDSTYLES /*| WS_VISIBLE*/,
                                          /* tests depend on a not zero MDIClient size */
                                         0, 0, rc.right, rc.bottom,
                                         hwnd, 0, GetModuleHandle(0),
                                         (LPVOID)&client_cs);
            assert(mdi_client);
            test_MDI_create(hwnd, mdi_client);
            DestroyWindow(mdi_client);
            break;
        }

        case WM_CLOSE:
            PostQuitMessage(0);
            break;
    }
    return DefFrameProcA(hwnd, mdi_client, msg, wparam, lparam);
}

static BOOL mdi_RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = mdi_main_wnd_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MDI_parent_Class";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = mdi_child_wnd_proc_1;
    cls.lpszClassName = "MDI_child_Class_1";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = mdi_child_wnd_proc_2;
    cls.lpszClassName = "MDI_child_Class_2";
    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

static void test_mdi(void)
{
    HWND mdi_hwndMain;
    /*MSG msg;*/

    if (!mdi_RegisterWindowClasses()) assert(0);

    mdi_hwndMain = CreateWindowExA(0, "MDI_parent_Class", "MDI parent window",
                                   WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                   WS_MAXIMIZEBOX /*| WS_VISIBLE*/,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandle(0), NULL);
    assert(mdi_hwndMain);
/*
    while(GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
*/
}

static void test_icons(void)
{
    WNDCLASSEXA cls;
    HWND hwnd;
    HICON icon = LoadIconA(0, (LPSTR)IDI_APPLICATION);
    HICON icon2 = LoadIconA(0, (LPSTR)IDI_QUESTION);
    HICON small_icon = LoadImageA(0, (LPSTR)IDI_APPLICATION, IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED );
    HICON res;

    cls.cbSize = sizeof(cls);
    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = 0;
    cls.hIcon = LoadIconA(0, (LPSTR)IDI_HAND);
    cls.hIconSm = small_icon;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "IconWindowClass";

    RegisterClassExA(&cls);

    hwnd = CreateWindowExA(0, "IconWindowClass", "icon test", 0,
                           100, 100, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL);
    assert( hwnd );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == 0, "wrong big icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon );
    ok( res == 0, "wrong previous big icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon, "wrong big icon after set %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon2 );
    ok( res == icon, "wrong previous big icon %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon2, "wrong big icon after set %p/%p\n", res, icon2 );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == 0, "wrong small icon %p/0\n", res );
    /* this test is XP specific */
    /*res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res != 0, "wrong small icon %p\n", res );*/
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon );
    ok( res == 0, "wrong previous small icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == icon, "wrong small icon after set %p/%p\n", res, icon );
    /* this test is XP specific */
    /*res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == icon, "wrong small icon after set %p/%p\n", res, icon );*/
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small_icon );
    ok( res == icon, "wrong previous small icon %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == small_icon, "wrong small icon after set %p/%p\n", res, small_icon );
    /* this test is XP specific */
    /*res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == small_icon, "wrong small icon after set %p/%p\n", res, small_icon );*/

    /* make sure the big icon hasn't changed */
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon2, "wrong big icon after set %p/%p\n", res, icon2 );
}

static void test_SetWindowPos(HWND hwnd)
{
    SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOMOVE);
    SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOMOVE);

    SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOSIZE);
    SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOSIZE);
}

static void test_SetMenu(HWND parent)
{
    HWND child;
    HMENU hMenu, ret;

    hMenu = CreateMenu();
    assert(hMenu);

    /* parent */
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);

    ok(!SetMenu(parent, (HMENU)20), "SetMenu with invalid menu handle should fail\n");
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);

    ok(SetMenu(parent, hMenu), "SetMenu on a top level window should not fail\n");
    ret = GetMenu(parent);
    ok(ret == (HMENU)hMenu, "unexpected menu id %p\n", ret);

    ok(SetMenu(parent, 0), "SetMenu(0) on a top level window should not fail\n");
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);
 
    /* child */
    child = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, parent, (HMENU)10, 0, NULL);
    assert(child);

    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, (HMENU)20), "SetMenu with invalid menu handle should fail\n");
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, hMenu), "SetMenu on a child window should fail\n");
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, 0), "SetMenu(0) on a child window should fail\n");
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    DestroyWindow(child);
    DestroyMenu(hMenu);
}

static void test_window_tree(HWND parent, const DWORD *style, const int *order, int total)
{
    HWND child[5], hwnd;
    int i;

    assert(total <= 5);

    hwnd = GetWindow(parent, GW_CHILD);
    ok(!hwnd, "have to start without children to perform the test\n");

    for (i = 0; i < total; i++)
    {
        child[i] = CreateWindowExA(0, "static", "", style[i], 0,0,10,10,
                                   parent, 0, 0, NULL);
        trace("child[%d] = %p\n", i, child[i]);
        ok(child[i] != 0, "CreateWindowEx failed to create child window\n");
    }

    hwnd = GetWindow(parent, GW_CHILD);
    ok(hwnd != 0, "GetWindow(GW_CHILD) failed\n");
    ok(hwnd == GetWindow(child[total - 1], GW_HWNDFIRST), "GW_HWNDFIRST is wrong\n");
    ok(child[order[total - 1]] == GetWindow(child[0], GW_HWNDLAST), "GW_HWNDLAST is wrong\n");

    for (i = 0; i < total; i++)
    {
        trace("hwnd[%d] = %p\n", i, hwnd);
        ok(child[order[i]] == hwnd, "Z order of child #%d is wrong\n", i);

        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    for (i = 0; i < total; i++)
        ok(DestroyWindow(child[i]), "DestroyWindow failed\n");
}

static void test_children_zorder(HWND parent)
{
    const DWORD simple_style[5] = { WS_CHILD, WS_CHILD, WS_CHILD, WS_CHILD,
                                    WS_CHILD };
    const int simple_order[5] = { 0, 1, 2, 3, 4 };

    const DWORD complex_style[5] = { WS_CHILD, WS_CHILD | WS_MAXIMIZE,
                             WS_CHILD | WS_VISIBLE, WS_CHILD,
                             WS_CHILD | WS_MAXIMIZE | WS_VISIBLE };
    const int complex_order_1[1] = { 0 };
    const int complex_order_2[2] = { 1, 0 };
    const int complex_order_3[3] = { 1, 0, 2 };
    const int complex_order_4[4] = { 1, 0, 2, 3 };
    const int complex_order_5[5] = { 4, 1, 0, 2, 3 };

    /* simple WS_CHILD */
    test_window_tree(parent, simple_style, simple_order, 5);

    /* complex children styles */
    test_window_tree(parent, complex_style, complex_order_1, 1);
    test_window_tree(parent, complex_style, complex_order_2, 2);
    test_window_tree(parent, complex_style, complex_order_3, 3);
    test_window_tree(parent, complex_style, complex_order_4, 4);
    test_window_tree(parent, complex_style, complex_order_5, 5);
}

static void test_SetFocus(HWND hwnd)
{
    HWND child;

    /* check if we can set focus to non-visible windows */

    ShowWindow(hwnd, SW_SHOW);
    SetFocus(0);
    SetFocus(hwnd);
    ok( GetFocus() == hwnd, "Failed to set focus to visible window %p\n", hwnd );
    ok( GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE, "Window %p not visible\n", hwnd );
    ShowWindow(hwnd, SW_HIDE);
    SetFocus(0);
    SetFocus(hwnd);
    ok( GetFocus() == hwnd, "Failed to set focus to invisible window %p\n", hwnd );
    ok( !(GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE), "Window %p still visible\n", hwnd );
    child = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    assert(child);
    SetFocus(child);
    ok( GetFocus() == child, "Failed to set focus to invisible child %p\n", child );
    ok( !(GetWindowLong(child,GWL_STYLE) & WS_VISIBLE), "Child %p is visible\n", child );
    ShowWindow(child, SW_SHOW);
    ok( GetWindowLong(child,GWL_STYLE) & WS_VISIBLE, "Child %p is not visible\n", child );
    ok( GetFocus() == child, "Focus no longer on child %p\n", child );
    ShowWindow(child, SW_HIDE);
    ok( !(GetWindowLong(child,GWL_STYLE) & WS_VISIBLE), "Child %p is visible\n", child );
    ok( GetFocus() == hwnd, "Focus should be on parent %p, not %p\n", hwnd, GetFocus() );
    ShowWindow(child, SW_SHOW);
    SetFocus(child);
    ok( GetFocus() == child, "Focus should be on child %p\n", child );
    SetWindowPos(child,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);
    ok( GetFocus() == child, "Focus should still be on child %p\n", child );

    ShowWindow(child, SW_HIDE);
    SetFocus(hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p, not %p\n", hwnd, GetFocus() );
    SetWindowPos(child,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    ok( GetFocus() == hwnd, "Focus should still be on parent %p, not %p\n", hwnd, GetFocus() );
    ShowWindow(child, SW_HIDE);
    ok( GetFocus() == hwnd, "Focus should still be on parent %p, not %p\n", hwnd, GetFocus() );

    DestroyWindow( child );
}

static void test_SetActiveWindow(HWND hwnd)
{
    HWND hwnd2;

    ShowWindow(hwnd, SW_SHOW);
    SetActiveWindow(0);
    SetActiveWindow(hwnd);
    ok( GetActiveWindow() == hwnd, "Failed to set focus to visible window %p\n", hwnd );
    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    ok( GetActiveWindow() == hwnd, "Window %p no longer active\n", hwnd );
    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    ShowWindow(hwnd, SW_HIDE);
    ok( GetActiveWindow() != hwnd, "Window %p is still active\n", hwnd );
    ShowWindow(hwnd, SW_SHOW);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    ok( GetActiveWindow() == hwnd2, "Window %p is not active\n", hwnd2 );
    DestroyWindow(hwnd2);
    ok( GetActiveWindow() != hwnd2, "Window %p is still active\n", hwnd2 );

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    ok( GetActiveWindow() == hwnd2, "Window %p is not active\n", hwnd2 );
    SetWindowPos(hwnd2,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    ok( GetActiveWindow() == hwnd2, "Window %p no longer active (%p)\n", hwnd2, GetActiveWindow() );
    DestroyWindow(hwnd2);
    ok( GetActiveWindow() != hwnd2, "Window %p is still active\n", hwnd2 );
}

START_TEST(win)
{
    pGetAncestor = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetAncestor" );
    pGetWindowInfo = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetWindowInfo" );

    hwndMain = CreateWindowExA(0, "static", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, NULL);
    if (hwndMain)
    {
        ok(!GetParent(hwndMain), "GetParent should return 0 for message only windows\n");
        if (pGetAncestor)
        {
            hwndMessage = pGetAncestor(hwndMain, GA_PARENT);
            ok(hwndMessage != 0, "GetAncestor(GA_PARENT) should not return 0 for message only windows\n");
            trace("hwndMessage %p\n", hwndMessage);
        }
        DestroyWindow(hwndMain);
    }
    else
        trace("CreateWindowExA with parent HWND_MESSAGE failed\n");

    if (!RegisterWindowClasses()) assert(0);

    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    assert(hhook);

    hwndMain = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window",
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_POPUP,
                               100, 100, 200, 200,
                               0, 0, 0, NULL);
    hwndMain2 = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window 2",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_POPUP,
                                100, 100, 200, 200,
                                0, 0, 0, NULL);
    assert( hwndMain );
    assert( hwndMain2 );

    test_parent_owner();
    test_shell_window();

    test_mdi();
    test_icons();
    test_SetWindowPos(hwndMain);
    test_SetMenu(hwndMain);
    test_SetFocus(hwndMain);
    test_SetActiveWindow(hwndMain);

    test_children_zorder(hwndMain);

    UnhookWindowsHookEx(hhook);
}
