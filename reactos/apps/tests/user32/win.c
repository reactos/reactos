/*
 * Unit tests for window handling
 *
 * Copyright 2002 Bill Medland
 * Copyright 2002 Alexandre Julliard
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

HWND      WINAPI GetShellWindow(void);

#ifndef SPI_GETDESKWALLPAPER
#define SPI_GETDESKWALLPAPER 0x0073
#endif

#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

static HWND (WINAPI *pGetAncestor)(HWND,UINT);

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
        ok( res == ga_parent, "Wrong result for GA_PARENT %p expected %p", res, ga_parent );
    }
    res = (HWND)GetWindowLongA( hwnd, GWL_HWNDPARENT );
    ok( res == gwl_parent, "Wrong result for GWL_HWNDPARENT %p expected %p", res, gwl_parent );
    res = GetParent( hwnd );
    ok( res == get_parent, "Wrong result for GetParent %p expected %p", res, get_parent );
    res = GetWindow( hwnd, GW_OWNER );
    ok( res == gw_owner, "Wrong result for GW_OWNER %p expected %p", res, gw_owner );
    if (pGetAncestor)
    {
        res = pGetAncestor( hwnd, GA_ROOT );
        ok( res == ga_root, "Wrong result for GA_ROOT %p expected %p", res, ga_root );
        res = pGetAncestor( hwnd, GA_ROOTOWNER );
        ok( res == ga_root_owner, "Wrong result for GA_ROOTOWNER %p expected %p", res, ga_root_owner );
    }
}


static HWND create_tool_window( LONG style, HWND parent )
{
    HWND ret = CreateWindowExA(0, "ToolWindowClass", "Tool window 1", style,
                               0, 0, 100, 100, parent, 0, 0, NULL );
    ok( ret != 0, "Creation failed" );
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
    ok( !test, "WS_CHILD without parent created" );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    style = GetWindowLongA( desktop, GWL_STYLE );
    ok( !SetWindowLongA( desktop, GWL_STYLE, WS_POPUP ), "Set GWL_STYLE on desktop succeeded" );
    ok( !SetWindowLongA( desktop, GWL_STYLE, 0 ), "Set GWL_STYLE on desktop succeeded" );
    ok( GetWindowLongA( desktop, GWL_STYLE ) == style, "Desktop style changed" );

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
    ok( !ret, "Set GWL_HWNDPARENT succeeded on desktop" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ok( !SetParent( desktop, hwndMain ), "SetParent succeeded on desktop" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
#endif
    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == hwndMain, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)desktop );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    /* window is now child of desktop so GWL_HWNDPARENT changes owner from now on */
    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, child, desktop, child, test, desktop );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, hwndMain2, 0, hwndMain2, test, test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, desktop, child, 0, child, test, test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, hwndMain2, hwndMain2, hwndMain2, test, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p", ret, hwndMain2 );
    check_parents( test, desktop, child, child, child, test, hwndMain );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = SetParent( test, desktop );
    ok( ret == hwndMain, "SetParent return value %p expected %p", ret, hwndMain );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = SetParent( test, hwndMain2 );
    ok( ret == child, "SetParent return value %p expected %p", ret, child );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain2 );
    trace( "created owned popup %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, hwndMain2, hwndMain2, hwndMain, hwndMain2 );

    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (ULONG_PTR)hwndMain );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p", ret, child );
    check_parents( test, hwndMain, hwndMain, hwndMain2, hwndMain2, hwndMain, hwndMain2 );
    DestroyWindow( test );

    /**************** test owner destruction *******************/

    /* owned child popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p", ret, desktop );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    /* window is now child of 'child' but owned by 'owner' */
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction", test );
    check_parents( test, child, child, owner, owner, hwndMain, owner );
    ok( !IsWindow(owner), "Owner %p not destroyed", owner );
    DestroyWindow(test);

    /* owned top-level popup */
    owner = create_tool_window( 0, 0 );
    test = create_tool_window( WS_POPUP, owner );
    trace( "created owner %p and popup %p\n", owner, test );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow( owner );
    ok( !IsWindow(test), "Window %p not destroyed by owner destruction", test );

    /* top-level popup owned by child */
    owner = create_tool_window( WS_CHILD, hwndMain2 );
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created owner %p and popup %p\n", owner, test );
    ret = (HWND)SetWindowLongA( test, GWL_HWNDPARENT, (ULONG_PTR)owner );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0", ret );
    check_parents( test, desktop, owner, owner, owner, test, hwndMain2 );
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction", test );
    ok( !IsWindow(owner), "Owner %p not destroyed", owner );
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
	    CBT_CREATEWNDA *createwnd = (CBT_CREATEWNDA *)lParam;
	    trace("HCBT_CREATEWND: hwnd %p, parent %p, style %08lx\n",
		  (HWND)wParam, createwnd->lpcs->hwndParent, createwnd->lpcs->style);
	    ok(createwnd->hwndInsertAfter == HWND_TOP, "hwndInsertAfter should be always HWND_TOP\n");
	    break;
	}
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
        ok(ret, "termination of previous shell process failed: GetLastError()=%ld", GetLastError());
        WaitForSingleObject(hProcess, INFINITE);    /* wait for termination */
        CloseHandle(hProcess);
    }

    hwnd1 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST1"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 100, 100, 300, 200, 0, 0, hinst, 0);
    trace("created window 1: %p\n", hwnd1);

    ret = SetShellWindow(hwnd1);
    ok(ret, "first call to SetShellWindow(hwnd1)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd1, "wrong shell window: %p", shellWindow);

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
    ok(!ret, "SetShellWindow(hwnd2) with WS_EX_TOPMOST");

    hwnd3 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST3"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 200, 400, 300, 200, 0, 0, hinst, 0);
    trace("created window 3: %p\n", hwnd3);

    hwnd4 = CreateWindowEx(0, TEXT("#32770"), TEXT("TEST4"), WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 250, 500, 300, 200, 0, 0, hinst, 0);
    trace("created window 4: %p\n", hwnd4);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==hwnd3, "wrong next window for hwnd4: %p - expected hwnd3\n", nextWnd);

    ret = SetShellWindow(hwnd4);
    ok(ret, "SetShellWindow(hwnd4)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected hwnd4", shellWindow);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==0, "wrong next window for hwnd4: %p - expected 0\n", nextWnd);

    ret = SetWindowPos(hwnd4, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, HWND_TOPMOST)\n");

    ret = SetWindowPos(hwnd4, hwnd3, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, hwnd3");

    ret = SetShellWindow(hwnd3);
    ok(!ret, "SetShellWindow(hwnd3)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected hwnd4", shellWindow);

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


START_TEST(win)
{
    pGetAncestor = (void *)GetProcAddress( GetModuleHandleA("user32.dll"), "GetAncestor" );

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
    UnhookWindowsHookEx(hhook);
}
