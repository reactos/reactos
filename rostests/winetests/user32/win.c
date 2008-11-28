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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* To get ICON_SMALL2 with the MSVC headers */

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

void dump_region(HRGN hrgn);

static HWND (WINAPI *pGetAncestor)(HWND,UINT);
static BOOL (WINAPI *pGetWindowInfo)(HWND,WINDOWINFO*);
static UINT (WINAPI *pGetWindowModuleFileNameA)(HWND,LPSTR,UINT);
static BOOL (WINAPI *pGetLayeredWindowAttributes)(HWND,COLORREF*,BYTE*,DWORD*);
static BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
static HMONITOR (WINAPI *pMonitorFromPoint)(POINT,DWORD);

static BOOL test_lbuttondown_flag;
static HWND hwndMessage;
static HWND hwndMain, hwndMain2;
static HHOOK hhook;

static const char* szAWRClass = "Winsize";
static HMENU hmenu;
static DWORD our_pid;

#define COUNTOF(arr) (sizeof(arr)/sizeof(arr[0]))

static void dump_minmax_info( const MINMAXINFO *minmax )
{
    trace("Reserved=%d,%d MaxSize=%d,%d MaxPos=%d,%d MinTrack=%d,%d MaxTrack=%d,%d\n",
          minmax->ptReserved.x, minmax->ptReserved.y,
          minmax->ptMaxSize.x, minmax->ptMaxSize.y,
          minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
          minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
          minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events( BOOL remove_messages )
{
    MSG msg;
    int diff = 200;
    int min_timeout = 50;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        if (remove_messages)
            while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessage( &msg );
        diff = time - GetTickCount();
        min_timeout = 10;
    }
}

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
    res = (HWND)GetWindowLongPtrA( hwnd, GWLP_HWNDPARENT );
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

static BOOL CALLBACK EnumChildProc( HWND hwndChild, LPARAM lParam)
{
    (*(LPINT)lParam)++;
    trace("EnumChildProc on %p\n", hwndChild);
    if (*(LPINT)lParam > 1) return FALSE;
    return TRUE;
}

/* will search for the given window */
static BOOL CALLBACK EnumChildProc1( HWND hwndChild, LPARAM lParam)
{
    trace("EnumChildProc1 on %p\n", hwndChild);
    if ((HWND)lParam == hwndChild) return FALSE;
    return TRUE;
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
    INT  numChildren;

    trace( "main window %p main2 %p desktop %p child %p\n", hwndMain, hwndMain2, desktop, child );

    /* child without parent, should fail */
    SetLastError(0xdeadbeef);
    test = CreateWindowExA(0, "ToolWindowClass", "Tool window 1",
                           WS_CHILD, 0, 0, 100, 100, 0, 0, 0, NULL );
    ok( !test, "WS_CHILD without parent created\n" );
    ok( GetLastError() == ERROR_TLW_WITH_WSCHILD ||
        broken(GetLastError() == 0xdeadbeef), /* win9x */
        "CreateWindowExA error %u\n", GetLastError() );

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
    if (0)
    {
    /* this test succeeds on NT but crashes on win9x systems */
    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( !ret, "Set GWL_HWNDPARENT succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ok( !SetParent( desktop, hwndMain ), "SetParent succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    }
    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    trace( "created child %p\n", test );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == hwndMain, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain );
    check_parents( test, hwndMain2, hwndMain2, hwndMain2, 0, hwndMain2, hwndMain2 );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, child, child, child, 0, hwndMain, hwndMain );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)desktop );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    /* window is now child of desktop so GWLP_HWNDPARENT changes owner from now on */
    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)child );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, child, desktop, child, test, desktop );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    trace( "created top-level %p\n", test );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, hwndMain2, 0, hwndMain2, test, test );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, desktop, child, 0, child, test, test );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup */
    test = create_tool_window( WS_POPUP, 0 );
    trace( "created popup %p\n", test );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, hwndMain2, hwndMain2, hwndMain2, test, hwndMain2 );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)child );
    ok( ret == hwndMain2, "GWL_HWNDPARENT return value %p expected %p\n", ret, hwndMain2 );
    check_parents( test, desktop, child, child, child, test, hwndMain );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, 0 );
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

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (ULONG_PTR)hwndMain );
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
    /* Win98 doesn't pass this test. It doesn't allow a destroyed owner,
     * while Win95, Win2k, WinXP do.
     */
    /*check_parents( test, child, child, owner, owner, hwndMain, owner );*/
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
    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (ULONG_PTR)owner );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, owner, owner, owner, test, hwndMain2 );
    DestroyWindow( owner );
    ok( IsWindow(test), "Window %p destroyed by owner destruction\n", test );
    ok( !IsWindow(owner), "Owner %p not destroyed\n", owner );
    /* Win98 doesn't pass this test. It doesn't allow a destroyed owner,
     * while Win95, Win2k, WinXP do.
     */
    /*check_parents( test, desktop, owner, owner, owner, test, owner );*/
    DestroyWindow(test);

    /* final cleanup */
    DestroyWindow(child);


    owner = create_tool_window( WS_OVERLAPPED, 0 );
    test = create_tool_window( WS_POPUP, desktop );

    ok( !GetWindow( test, GW_OWNER ), "Wrong owner window\n" );
    numChildren = 0;
    ok( !EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned FALSE\n" );
    ok( numChildren == 0, "numChildren should be 0 got %d\n", numChildren );

    SetWindowLongA( test, GWL_STYLE, (GetWindowLongA( test, GWL_STYLE ) & ~WS_POPUP) | WS_CHILD );
    ret = SetParent( test, owner );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );

    numChildren = 0;
    ok( EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned TRUE\n" );
    ok( numChildren == 1, "numChildren should be 1 got %d\n", numChildren );

    child = create_tool_window( WS_CHILD, owner );
    numChildren = 0;
    ok( !EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned FALSE\n" );
    ok( numChildren == 2, "numChildren should be 2 got %d\n", numChildren );
    DestroyWindow( child );

    child = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, owner );
    ok( GetWindow( child, GW_OWNER ) == owner, "Wrong owner window\n" );
    numChildren = 0;
    ok( EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned TRUE\n" );
    ok( numChildren == 1, "numChildren should be 1 got %d\n", numChildren );

    ret = SetParent( child, owner );
    ok( GetWindow( child, GW_OWNER ) == owner, "Wrong owner window\n" );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    numChildren = 0;
    ok( !EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned FALSE\n" );
    ok( numChildren == 2, "numChildren should be 2 got %d\n", numChildren );

    ret = SetParent( child, NULL );
    ok( GetWindow( child, GW_OWNER ) == owner, "Wrong owner window\n" );
    ok( ret == owner, "SetParent return value %p expected %p\n", ret, owner );
    numChildren = 0;
    ok( EnumChildWindows( owner, EnumChildProc, (LPARAM)&numChildren ),
        "EnumChildWindows should have returned TRUE\n" );
    ok( numChildren == 1, "numChildren should be 1 got %d\n", numChildren );

    /* even GW_OWNER == owner it's still a desktop's child */
    ok( !EnumChildWindows( desktop, EnumChildProc1, (LPARAM)child ),
        "EnumChildWindows should have found %p and returned FALSE\n", child );

    DestroyWindow( child );
    child = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, NULL );

    ok( !EnumChildWindows( desktop, EnumChildProc1, (LPARAM)child ),
        "EnumChildWindows should have found %p and returned FALSE\n", child );

    DestroyWindow( child );
    DestroyWindow( test );
    DestroyWindow( owner );
}


static LRESULT WINAPI main_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
	case WM_GETMINMAXINFO:
	{
	    MINMAXINFO* minmax = (MINMAXINFO *)lparam;

	    trace("hwnd %p, WM_GETMINMAXINFO, %08lx, %08lx\n", hwnd, wparam, lparam);
            dump_minmax_info( minmax );
	    SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0x20031021);
	    break;
	}
	case WM_WINDOWPOSCHANGING:
	{
	    BOOL is_win9x = GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == 0;
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
            RECT rc1, rc2;
	    WINDOWPOS *winpos = (WINDOWPOS *)lparam;
	    trace("main: WM_WINDOWPOSCHANGED\n");
	    trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
		   winpos->hwnd, winpos->hwndInsertAfter,
		   winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);
	    ok(winpos->x >= -32768 && winpos->x <= 32767, "bad winpos->x %d\n", winpos->x);
	    ok(winpos->y >= -32768 && winpos->y <= 32767, "bad winpos->y %d\n", winpos->y);

	    ok(winpos->cx >= 0 && winpos->cx <= 32767, "bad winpos->cx %d\n", winpos->cx);
	    ok(winpos->cy >= 0 && winpos->cy <= 32767, "bad winpos->cy %d\n", winpos->cy);

            GetWindowRect(hwnd, &rc1);
            trace("window: (%d,%d)-(%d,%d)\n", rc1.left, rc1.top, rc1.right, rc1.bottom);
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            trace("pos: (%d,%d)-(%d,%d)\n", rc2.left, rc2.top, rc2.right, rc2.bottom);
            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
            ok(EqualRect(&rc1, &rc2), "rects do not match\n");
            }

            GetClientRect(hwnd, &rc2);
            DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match (%d,%d-%d,%d) / (%d,%d-%d,%d)\n",
               rc1.left, rc1.top, rc1.right, rc1.bottom, rc2.left, rc2.top, rc2.right, rc2.bottom );
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongPtrA(hwnd, GWLP_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

            trace("WM_NCCREATE: hwnd %p, parent %p, style %08x\n", hwnd, cs->hwndParent, cs->style);
	    if (got_getminmaxinfo)
		trace("%p got WM_GETMINMAXINFO\n", hwnd);

	    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
		ok(got_getminmaxinfo, "main: WM_GETMINMAXINFO should have been received before WM_NCCREATE\n");
	    else
		ok(!got_getminmaxinfo, "main: WM_GETMINMAXINFO should NOT have been received before WM_NCCREATE\n");
	    break;
	}
        case WM_COMMAND:
            if (test_lbuttondown_flag)
            {
                ShowWindow((HWND)wparam, SW_SHOW);
                flush_events( FALSE );
            }
            break;
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

	    trace("hwnd %p, WM_GETMINMAXINFO, %08lx, %08lx\n", hwnd, wparam, lparam);
            dump_minmax_info( minmax );
	    SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0x20031021);
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongPtrA(hwnd, GWLP_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

            trace("WM_NCCREATE: hwnd %p, parent %p, style %08x\n", hwnd, cs->hwndParent, cs->style);
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

    cls.style = CS_DBLCLKS;
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

static void verify_window_info(HWND hwnd, const WINDOWINFO *info)
{
    RECT rcWindow, rcClient;
    DWORD status;

    ok(IsWindow(hwnd), "bad window handle\n");

    GetWindowRect(hwnd, &rcWindow);
    ok(EqualRect(&rcWindow, &info->rcWindow), "wrong rcWindow\n");

    GetClientRect(hwnd, &rcClient);
    /* translate to screen coordinates */
    MapWindowPoints(hwnd, 0, (LPPOINT)&rcClient, 2);
    ok(EqualRect(&rcClient, &info->rcClient), "wrong rcClient\n");

    ok(info->dwStyle == (DWORD)GetWindowLongA(hwnd, GWL_STYLE),
       "wrong dwStyle: %08x != %08x\n", info->dwStyle, GetWindowLongA(hwnd, GWL_STYLE));
    /* Windows reports some undocumented exstyles in WINDOWINFO, but
     * doesn't return them in GetWindowLong(hwnd, GWL_EXSTYLE).
     */
    ok((info->dwExStyle & ~0xe0000800) == (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE),
       "wrong dwExStyle: %08x != %08x\n", info->dwExStyle, GetWindowLongA(hwnd, GWL_EXSTYLE));
    status = (GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0;
    if (GetForegroundWindow())
        ok(info->dwWindowStatus == status, "wrong dwWindowStatus: %04x != %04x active %p fg %p\n",
           info->dwWindowStatus, status, GetActiveWindow(), GetForegroundWindow());

    /* win2k and XP return broken border info in GetWindowInfo most of
     * the time, so there is no point in testing it.
     */
#if 0
    UINT border;
    ok(info->cxWindowBorders == (unsigned)(rcClient.left - rcWindow.left),
       "wrong cxWindowBorders %d != %d\n", info->cxWindowBorders, rcClient.left - rcWindow.left);
    border = min(rcWindow.bottom - rcClient.bottom, rcClient.top - rcWindow.top);
    ok(info->cyWindowBorders == border,
       "wrong cyWindowBorders %d != %d\n", info->cyWindowBorders, border);
#endif
    ok(info->atomWindowType == GetClassLongA(hwnd, GCW_ATOM), "wrong atomWindowType\n");
    ok(info->wCreatorVersion == 0x0400 /* NT4, Win2000, XP, Win2003 */ ||
       info->wCreatorVersion == 0x0500 /* Vista */,
       "wrong wCreatorVersion %04x\n", info->wCreatorVersion);
}

static void FixedAdjustWindowRectEx(RECT* rc, LONG style, BOOL menu, LONG exstyle)
{
    AdjustWindowRectEx(rc, style, menu, exstyle);
    /* AdjustWindowRectEx does not include scroll bars */
    if (style & WS_VSCROLL)
    {
        if(exstyle & WS_EX_LEFTSCROLLBAR)
            rc->left  -= GetSystemMetrics(SM_CXVSCROLL);
        else
            rc->right += GetSystemMetrics(SM_CXVSCROLL);
    }
    if (style & WS_HSCROLL)
	rc->bottom += GetSystemMetrics(SM_CYHSCROLL);
}

static void test_nonclient_area(HWND hwnd)
{
    DWORD style, exstyle;
    RECT rc_window, rc_client, rc;
    BOOL menu;
    BOOL is_win9x = GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == 0;
    LRESULT ret;

    style = GetWindowLongA(hwnd, GWL_STYLE);
    exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    menu = !(style & WS_CHILD) && GetMenu(hwnd) != 0;

    GetWindowRect(hwnd, &rc_window);
    trace("window: (%d,%d)-(%d,%d)\n", rc_window.left, rc_window.top, rc_window.right, rc_window.bottom);
    GetClientRect(hwnd, &rc_client);
    trace("client: (%d,%d)-(%d,%d)\n", rc_client.left, rc_client.top, rc_client.right, rc_client.bottom);

    /* avoid some cases when things go wrong */
    if (IsRectEmpty(&rc_window) || IsRectEmpty(&rc_client) ||
	rc_window.right > 32768 || rc_window.bottom > 32768) return;

    CopyRect(&rc, &rc_client);
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc, 2);
    FixedAdjustWindowRectEx(&rc, style, menu, exstyle);

    trace("calc window: (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    ok(EqualRect(&rc, &rc_window), "window rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d\n", style, exstyle, menu);


    CopyRect(&rc, &rc_window);
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    trace("calc client: (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    ok(EqualRect(&rc, &rc_client), "client rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d\n", style, exstyle, menu);

    /* NULL rectangle shouldn't crash */
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, 0);
    ok(ret == 0, "NULL rectangle returned %ld instead of 0\n", ret);

    /* Win9x doesn't like WM_NCCALCSIZE with synthetic data and crashes */;
    if (is_win9x)
	return;

    /* and now test AdjustWindowRectEx and WM_NCCALCSIZE on synthetic data */
    SetRect(&rc_client, 0, 0, 250, 150);
    CopyRect(&rc_window, &rc_client);
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc_window, 2);
    FixedAdjustWindowRectEx(&rc_window, style, menu, exstyle);
    trace("calc window: (%d,%d)-(%d,%d)\n",
	rc_window.left, rc_window.top, rc_window.right, rc_window.bottom);

    CopyRect(&rc, &rc_window);
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    trace("calc client: (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    ok(EqualRect(&rc, &rc_client), "synthetic rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d\n", style, exstyle, menu);
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

    trace("CBT: %d (%s), %08lx, %08lx\n", nCode, code_name, wParam, lParam);

    /* on HCBT_DESTROYWND window state is undefined */
    if (nCode != HCBT_DESTROYWND && IsWindow((HWND)wParam))
    {
	if (pGetWindowInfo)
	{
	    WINDOWINFO info;

	    /* Win98 actually does check the info.cbSize and doesn't allow
	     * it to be anything except sizeof(WINDOWINFO), while Win95, Win2k,
	     * WinXP do not check it at all.
	     */
	    info.cbSize = sizeof(WINDOWINFO);
	    ok(pGetWindowInfo((HWND)wParam, &info), "GetWindowInfo should not fail\n");
	    verify_window_info((HWND)wParam, &info);
	}
    }

    switch (nCode)
    {
	case HCBT_CREATEWND:
	{
	    static const RECT rc_null;
	    RECT rc;
	    LONG style;
	    CBT_CREATEWNDA *createwnd = (CBT_CREATEWNDA *)lParam;
            trace("HCBT_CREATEWND: hwnd %p, parent %p, style %08x\n",
		  (HWND)wParam, createwnd->lpcs->hwndParent, createwnd->lpcs->style);
	    ok(createwnd->hwndInsertAfter == HWND_TOP, "hwndInsertAfter should be always HWND_TOP\n");

	    /* WS_VISIBLE should be turned off yet */
	    style = createwnd->lpcs->style & ~WS_VISIBLE;
	    ok(style == GetWindowLongA((HWND)wParam, GWL_STYLE),
		"style of hwnd and style in the CREATESTRUCT do not match: %08x != %08x\n",
		GetWindowLongA((HWND)wParam, GWL_STYLE), style);

            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
	    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
	    {
		ok(GetParent((HWND)wParam) == hwndMessage,
		   "wrong result from GetParent %p: message window %p\n",
		   GetParent((HWND)wParam), hwndMessage);
	    }
	    else
		ok(!GetParent((HWND)wParam), "GetParent should return 0 at this point\n");

	    ok(!GetWindow((HWND)wParam, GW_OWNER), "GW_OWNER should be set to 0 at this point\n");
            }
            if (0)
            {
	    /* while NT assigns GW_HWNDFIRST/LAST some values at this point,
	     * Win9x still has them set to 0.
	     */
	    ok(GetWindow((HWND)wParam, GW_HWNDFIRST) != 0, "GW_HWNDFIRST should not be set to 0 at this point\n");
	    ok(GetWindow((HWND)wParam, GW_HWNDLAST) != 0, "GW_HWNDLAST should not be set to 0 at this point\n");
            }
	    ok(!GetWindow((HWND)wParam, GW_HWNDPREV), "GW_HWNDPREV should be set to 0 at this point\n");
	    ok(!GetWindow((HWND)wParam, GW_HWNDNEXT), "GW_HWNDNEXT should be set to 0 at this point\n");

            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
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
            }
	    break;
	}
    }

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

static void test_shell_window(void)
{
    BOOL ret;
    DWORD error;
    HMODULE hinst, hUser32;
    BOOL (WINAPI*SetShellWindow)(HWND);
    BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);
    HWND hwnd1, hwnd2, hwnd3, hwnd4, hwnd5;
    HWND shellWindow, nextWnd;

    if (!GetWindowLongW(GetDesktopWindow(), GWL_STYLE))
    {
        trace("Skipping shell window test on Win9x\n");
        return;
    }

    shellWindow = GetShellWindow();
    hinst = GetModuleHandle(0);
    hUser32 = GetModuleHandleA("user32");

    SetShellWindow = (void *)GetProcAddress(hUser32, "SetShellWindow");
    SetShellWindowEx = (void *)GetProcAddress(hUser32, "SetShellWindowEx");

    trace("previous shell window: %p\n", shellWindow);

    if (shellWindow) {
        DWORD pid;
        HANDLE hProcess;

        SetLastError(0xdeadbeef);
        ret = DestroyWindow(shellWindow);
        error = GetLastError();

        ok(!ret, "DestroyWindow(shellWindow)\n");
        /* passes on Win XP, but not on Win98 */
        ok(error==ERROR_ACCESS_DENIED || error == 0xdeadbeef,
           "got %u after DestroyWindow(shellWindow)\n", error);

        /* close old shell instance */
        GetWindowThreadProcessId(shellWindow, &pid);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        ret = TerminateProcess(hProcess, 0);
        ok(ret, "termination of previous shell process failed: GetLastError()=%d\n", GetLastError());
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

    SetWindowLong(hwnd1, GWL_EXSTYLE, GetWindowLong(hwnd1,GWL_EXSTYLE)|WS_EX_TOPMOST);
    ret = GetWindowLong(hwnd1,GWL_EXSTYLE)&WS_EX_TOPMOST? TRUE: FALSE;
    ok(!ret, "SetWindowExStyle(hwnd1, WS_EX_TOPMOST)\n");

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

static char mdi_lParam_test_message[] = "just a test string";

static void test_MDI_create(HWND parent, HWND mdi_client, INT first_id)
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
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_cs.style = 0x7fffffff; /* without WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_cs.style = 0xffffffff; /* with WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
    }
    else
    {
        ok(mdi_child != 0, "MDI child creation failed\n");
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

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
    else
    {
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0x7fffffff, /* without WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0xffffffff, /* with WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandle(0),
                                 (LPARAM)mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
    }
    else
    {
        ok(mdi_child != 0, "MDI child creation failed\n");
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

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
    else
    {
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0x7fffffff, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0xffffffff, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
    }
    else
    {
        ok(mdi_child != 0, "MDI child creation failed\n");
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

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
    else
    {
        ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == first_id, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

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
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == 0, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_POPUP, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == 0, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    DestroyWindow(mdi_child);

    /* maximized child */
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == 0, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == 0, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption and a thick frame\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION | WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandle(0),
                                (LPVOID)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok(GetWindowLongPtrA(mdi_child, GWLP_ID) == 0, "wrong child id %d\n", GetWindowLongPtrA(mdi_child, GWLP_ID));
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

    trace("max rect (%d,%d - %d, %d)\n",
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
                "mdi_cs->style does not match (%08x)\n", mdi_cs->style);
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
                   "cs->style does not match (%08x)\n", cs->style);
            }
            else
            {
                LONG style = mdi_cs->style;
                style &= ~WS_POPUP;
                style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                    WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
                ok(cs->style == style,
                   "cs->style does not match (%08x)\n", cs->style);
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
            trace("MDI client %p window size = (%d x %d)\n", client, rc.right-rc.left, rc.bottom-rc.top);
            GetClientRect(client, &rc);
            trace("MDI client %p client size = (%d x %d)\n", client, rc.right, rc.bottom);
            trace("screen size: %d x %d\n", GetSystemMetrics(SM_CXSCREEN),
                                            GetSystemMetrics(SM_CYSCREEN));

            GetClientRect(client, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
            trace("MDI child: calculated max window size = (%d x %d)\n", rc.right-rc.left, rc.bottom-rc.top);
            dump_minmax_info( minmax );

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %d != %d\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %d != %d\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);

            DefMDIChildProcA(hwnd, msg, wparam, lparam);

            trace("DefMDIChildProc returned:\n");
            dump_minmax_info( minmax );

            MDI_ChildGetMinMaxInfo(client, hwnd, &my_minmax);
            ok(minmax->ptMaxSize.x == my_minmax.ptMaxSize.x, "default width of maximized child %d != %d\n",
               minmax->ptMaxSize.x, my_minmax.ptMaxSize.x);
            ok(minmax->ptMaxSize.y == my_minmax.ptMaxSize.y, "default height of maximized child %d != %d\n",
               minmax->ptMaxSize.y, my_minmax.ptMaxSize.y);

            return 1;
        }

        case WM_MDIACTIVATE:
        {
            HWND active, client = GetParent(hwnd);
            /*trace("%p WM_MDIACTIVATE %08x %08lx\n", hwnd, wparam, lparam);*/
            active = (HWND)SendMessageA(client, WM_MDIGETACTIVE, 0, 0);
            if (hwnd == (HWND)lparam) /* if we are being activated */
                ok (active == (HWND)lparam, "new active %p != active %p\n", (HWND)lparam, active);
            else
                ok (active == (HWND)wparam, "old active %p != active %p\n", (HWND)wparam, active);
            break;
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
            /* For some reason Win98 doesn't translate cs->cx from CW_USEDEFAULT,
               while Win95, Win2k, WinXP do. */
            /*ok(cs->cx == 0, "%d != 0\n", cs->cx);*/
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
            trace("parent %p client size = (%d x %d)\n", parent, rc.right, rc.bottom);

            GetClientRect(parent, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
            trace("calculated max child window size = (%d x %d)\n", rc.right-rc.left, rc.bottom-rc.top);
            dump_minmax_info( minmax );

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %d != %d\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %d != %d\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);
            break;
        }

        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            RECT rc1, rc2;

            GetWindowRect(hwnd, &rc1);
            trace("window: (%d,%d)-(%d,%d)\n", rc1.left, rc1.top, rc1.right, rc1.bottom);
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            trace("pos: (%d,%d)-(%d,%d)\n", rc2.left, rc2.top, rc2.right, rc2.bottom);
            ok(EqualRect(&rc1, &rc2), "rects do not match\n");

            GetWindowRect(hwnd, &rc1);
            GetClientRect(hwnd, &rc2);
            DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match\n");
        }
        /* fall through */
        case WM_WINDOWPOSCHANGING:
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
            test_MDI_create(hwnd, mdi_client, client_cs.idFirstChild);
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
            test_MDI_create(hwnd, mdi_client, client_cs.idFirstChild);
            DestroyWindow(mdi_client);
            break;
        }

        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            RECT rc1, rc2;

            GetWindowRect(hwnd, &rc1);
            trace("window: (%d,%d)-(%d,%d)\n", rc1.left, rc1.top, rc1.right, rc1.bottom);
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            trace("pos: (%d,%d)-(%d,%d)\n", rc2.left, rc2.top, rc2.right, rc2.bottom);
            ok(EqualRect(&rc1, &rc2), "rects do not match\n");

            GetWindowRect(hwnd, &rc1);
            GetClientRect(hwnd, &rc2);
            DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match\n");
        }
        /* fall through */
        case WM_WINDOWPOSCHANGING:
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
    DestroyWindow(mdi_hwndMain);
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

    DestroyWindow( hwnd );
}

static LRESULT WINAPI nccalcsize_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NCCALCSIZE)
    {
        RECT *rect = (RECT *)lparam;
        /* first time around increase the rectangle, next time decrease it */
        if (rect->left == 100) InflateRect( rect, 10, 10 );
        else InflateRect( rect, -10, -10 );
        return 0;
    }
    return DefWindowProc( hwnd, msg, wparam, lparam );
}

static void test_SetWindowPos(HWND hwnd)
{
    RECT orig_win_rc, rect;
    LONG_PTR old_proc;
    BOOL is_win9x = GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == 0;

    SetRect(&rect, 111, 222, 333, 444);
    ok(!GetWindowRect(0, &rect), "GetWindowRect succeeded\n");
    ok(rect.left == 111 && rect.top == 222 && rect.right == 333 && rect.bottom == 444,
       "wrong window rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    SetRect(&rect, 111, 222, 333, 444);
    ok(!GetClientRect(0, &rect), "GetClientRect succeeded\n");
    ok(rect.left == 111 && rect.top == 222 && rect.right == 333 && rect.bottom == 444,
       "wrong window rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    GetWindowRect(hwnd, &orig_win_rc);

    old_proc = SetWindowLongPtr( hwnd, GWLP_WNDPROC, (ULONG_PTR)nccalcsize_proc );
    SetWindowPos(hwnd, 0, 100, 100, 0, 0, SWP_NOZORDER|SWP_FRAMECHANGED);
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 100 && rect.top == 100 && rect.right == 100 && rect.bottom == 100,
        "invalid window rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
    ok( rect.left == 90 && rect.top == 90 && rect.right == 110 && rect.bottom == 110,
        "invalid client rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    SetWindowPos(hwnd, 0, 200, 200, 0, 0, SWP_NOZORDER|SWP_FRAMECHANGED);
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 200 && rect.top == 200 && rect.right == 200 && rect.bottom == 200,
        "invalid window rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
    ok( rect.left == 210 && rect.top == 210 && rect.right == 190 && rect.bottom == 190,
        "invalid client rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    SetWindowPos(hwnd, 0, orig_win_rc.left, orig_win_rc.top,
                 orig_win_rc.right, orig_win_rc.bottom, 0);
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, old_proc );

    /* Win9x truncates coordinates to 16-bit irrespectively */
    if (!is_win9x)
    {
        SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOMOVE);
        SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOMOVE);

        SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOSIZE);
        SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOSIZE);
    }

    SetWindowPos(hwnd, 0, orig_win_rc.left, orig_win_rc.top,
                 orig_win_rc.right, orig_win_rc.bottom, 0);

    ok(!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST), "WS_EX_TOPMOST should not be set\n");
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST, "WS_EX_TOPMOST should be set\n");
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST, "WS_EX_TOPMOST should be set\n");
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST), "WS_EX_TOPMOST should not be set\n");
}

static void test_SetMenu(HWND parent)
{
    HWND child;
    HMENU hMenu, ret;
    BOOL is_win9x = GetWindowLongPtrW(parent, GWLP_WNDPROC) == 0;
    BOOL retok;
    DWORD style;

    hMenu = CreateMenu();
    assert(hMenu);

    ok(SetMenu(parent, hMenu), "SetMenu on a top level window should not fail\n");
    if (0)
    {
    /* fails on (at least) Wine, NT4, XP SP2 */
    test_nonclient_area(parent); 
    }
    ret = GetMenu(parent);
    ok(ret == hMenu, "unexpected menu id %p\n", ret);
    /* test whether we can destroy a menu assigned to a window */
    retok = DestroyMenu(hMenu);
    ok( retok, "DestroyMenu error %d\n", GetLastError());
    retok = IsMenu(hMenu);
    ok(!retok || broken(retok) /* nt4 */, "menu handle should be not valid after DestroyMenu\n");
    ret = GetMenu(parent);
    /* This test fails on Win9x */
    if (!is_win9x)
        ok(ret == hMenu, "unexpected menu id %p\n", ret);
    ok(SetMenu(parent, 0), "SetMenu(0) on a top level window should not fail\n");
    test_nonclient_area(parent);

    hMenu = CreateMenu();
    assert(hMenu);

    /* parent */
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);

    ok(!SetMenu(parent, (HMENU)20), "SetMenu with invalid menu handle should fail\n");
    test_nonclient_area(parent);
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);

    ok(SetMenu(parent, hMenu), "SetMenu on a top level window should not fail\n");
    if (0)
    {
    /* fails on (at least) Wine, NT4, XP SP2 */
    test_nonclient_area(parent);
    }
    ret = GetMenu(parent);
    ok(ret == hMenu, "unexpected menu id %p\n", ret);

    ok(SetMenu(parent, 0), "SetMenu(0) on a top level window should not fail\n");
    test_nonclient_area(parent);
    ret = GetMenu(parent);
    ok(ret == 0, "unexpected menu id %p\n", ret);
 
    /* child */
    child = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, parent, (HMENU)10, 0, NULL);
    assert(child);

    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, (HMENU)20), "SetMenu with invalid menu handle should fail\n");
    test_nonclient_area(child);
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, hMenu), "SetMenu on a child window should fail\n");
    test_nonclient_area(child);
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    ok(!SetMenu(child, 0), "SetMenu(0) on a child window should fail\n");
    test_nonclient_area(child);
    ret = GetMenu(child);
    ok(ret == (HMENU)10, "unexpected menu id %p\n", ret);

    style = GetWindowLong(child, GWL_STYLE);
    SetWindowLong(child, GWL_STYLE, style | WS_POPUP);
    ok(SetMenu(child, hMenu), "SetMenu on a popup child window should not fail\n");
    ok(SetMenu(child, 0), "SetMenu on a popup child window should not fail\n");
    SetWindowLong(child, GWL_STYLE, style);

    SetWindowLong(child, GWL_STYLE, style | WS_OVERLAPPED);
    ok(!SetMenu(child, hMenu), "SetMenu on a overlapped child window should fail\n");
    SetWindowLong(child, GWL_STYLE, style);

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
        if (style[i] & DS_CONTROL)
        {
            child[i] = CreateWindowExA(0, MAKEINTATOM(32770), "", style[i] & ~WS_VISIBLE,
                                       0,0,0,0, parent, (HMENU)i, 0, NULL);
            if (style[i] & WS_VISIBLE)
                ShowWindow(child[i], SW_SHOW);

            SetWindowPos(child[i], HWND_BOTTOM, 0,0,10,10, SWP_NOACTIVATE);
        }
        else
            child[i] = CreateWindowExA(0, "static", "", style[i], 0,0,10,10,
                                       parent, (HMENU)i, 0, NULL);
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
    const DWORD complex_style_6[3] = { WS_CHILD | WS_VISIBLE,
                                       WS_CHILD | WS_CLIPSIBLINGS | DS_CONTROL | WS_VISIBLE,
                                       WS_CHILD | WS_VISIBLE };
    const int complex_order_6[3] = { 0, 1, 2 };

    /* simple WS_CHILD */
    test_window_tree(parent, simple_style, simple_order, 5);

    /* complex children styles */
    test_window_tree(parent, complex_style, complex_order_1, 1);
    test_window_tree(parent, complex_style, complex_order_2, 2);
    test_window_tree(parent, complex_style, complex_order_3, 3);
    test_window_tree(parent, complex_style, complex_order_4, 4);
    test_window_tree(parent, complex_style, complex_order_5, 5);

    /* another set of complex children styles */
    test_window_tree(parent, complex_style_6, complex_order_6, 3);
}

#define check_z_order(hwnd, next, prev, owner, topmost) \
        check_z_order_debug((hwnd), (next), (prev), (owner), (topmost), \
                            __FILE__, __LINE__)

static void check_z_order_debug(HWND hwnd, HWND next, HWND prev, HWND owner,
                                BOOL topmost, const char *file, int line)
{
    HWND test;
    DWORD ex_style;

    test = GetWindow(hwnd, GW_HWNDNEXT);
    /* skip foreign windows */
    while (test && (GetWindowThreadProcessId(test, NULL) != our_pid ||
                    (void *)(ULONG_PTR)(GetWindowLongPtr(test, GWLP_HINSTANCE)) != GetModuleHandle(0)))
    {
        /*trace("skipping next %p (%p)\n", test, UlongToHandle(GetWindowLongPtr(test, GWLP_HINSTANCE)));*/
        test = GetWindow(test, GW_HWNDNEXT);
    }
    ok_(file, line)(next == test, "expected next %p, got %p\n", next, test);

    test = GetWindow(hwnd, GW_HWNDPREV);
    /* skip foreign windows */
    while (test && (GetWindowThreadProcessId(test, NULL) != our_pid ||
                    (void *)(ULONG_PTR)(GetWindowLongPtr(test, GWLP_HINSTANCE)) != GetModuleHandle(0)))
    {
        /*trace("skipping prev %p (%p)\n", test, UlongToHandle(GetWindowLongPtr(test, GWLP_HINSTANCE)));*/
        test = GetWindow(test, GW_HWNDPREV);
    }
    ok_(file, line)(prev == test, "expected prev %p, got %p\n", prev, test);

    test = GetWindow(hwnd, GW_OWNER);
    ok_(file, line)(owner == test, "expected owner %p, got %p\n", owner, test);

    ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
    ok_(file, line)(!(ex_style & WS_EX_TOPMOST) == !topmost, "expected %stopmost\n", topmost ? "" : "NOT ");
}

static void test_popup_zorder(HWND hwnd_D, HWND hwnd_E)
{
    HWND hwnd_A, hwnd_B, hwnd_C, hwnd_F;

    trace("hwnd_D %p, hwnd_E %p\n", hwnd_D, hwnd_E);

    SetWindowPos(hwnd_E, hwnd_D, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);
    check_z_order(hwnd_E, 0, hwnd_D, 0, FALSE);

    hwnd_F = CreateWindowEx(0, "MainWindowClass", "Owner window",
                            WS_OVERLAPPED | WS_CAPTION,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandle(0), NULL);
    trace("hwnd_F %p\n", hwnd_F);
    check_z_order(hwnd_F, hwnd_D, 0, 0, FALSE);

    SetWindowPos(hwnd_F, hwnd_E, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);

    hwnd_C = CreateWindowEx(0, "MainWindowClass", NULL,
                            WS_POPUP,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandle(0), NULL);
    trace("hwnd_C %p\n", hwnd_C);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, 0, hwnd_F, FALSE);

    hwnd_B = CreateWindowEx(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            WS_POPUP,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandle(0), NULL);
    trace("hwnd_B %p\n", hwnd_B);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, 0, hwnd_F, TRUE);

    hwnd_A = CreateWindowEx(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            WS_POPUP,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandle(0), NULL);
    trace("hwnd_A %p\n", hwnd_A);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, hwnd_A, hwnd_F, TRUE);
    check_z_order(hwnd_A, hwnd_B, 0, 0, TRUE);

    trace("A %p B %p C %p D %p E %p F %p\n", hwnd_A, hwnd_B, hwnd_C, hwnd_D, hwnd_E, hwnd_F);

    /* move hwnd_F and its popups up */
    SetWindowPos(hwnd_F, HWND_TOP, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    check_z_order(hwnd_E, 0, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_F, 0, FALSE);
    check_z_order(hwnd_F, hwnd_D, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_F, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, hwnd_A, hwnd_F, TRUE);
    check_z_order(hwnd_A, hwnd_B, 0, 0, TRUE);

    /* move hwnd_F and its popups down */
#if 0 /* enable once Wine is fixed to pass this test */
    SetWindowPos(hwnd_F, HWND_BOTTOM, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    check_z_order(hwnd_F, 0, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_F, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, hwnd_E, hwnd_F, FALSE);
    check_z_order(hwnd_E, hwnd_B, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_A, 0, FALSE);
    check_z_order(hwnd_A, hwnd_D, 0, 0, TRUE);
#endif

    DestroyWindow(hwnd_A);
    DestroyWindow(hwnd_B);
    DestroyWindow(hwnd_C);
    DestroyWindow(hwnd_F);
}

static void test_vis_rgn( HWND hwnd )
{
    RECT win_rect, rgn_rect;
    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
    HDC hdc;

    ShowWindow(hwnd,SW_SHOW);
    hdc = GetDC( hwnd );
    ok( GetRandomRgn( hdc, hrgn, SYSRGN ) != 0, "GetRandomRgn failed\n" );
    GetWindowRect( hwnd, &win_rect );
    GetRgnBox( hrgn, &rgn_rect );
    if (GetVersion() & 0x80000000)
    {
        trace("win9x, mapping to screen coords\n");
        MapWindowPoints( hwnd, 0, (POINT *)&rgn_rect, 2 );
    }
    trace("win: %d,%d-%d,%d\n", win_rect.left, win_rect.top, win_rect.right, win_rect.bottom );
    trace("rgn: %d,%d-%d,%d\n", rgn_rect.left, rgn_rect.top, rgn_rect.right, rgn_rect.bottom );
    ok( win_rect.left <= rgn_rect.left, "rgn left %d not inside win rect %d\n",
        rgn_rect.left, win_rect.left );
    ok( win_rect.top <= rgn_rect.top, "rgn top %d not inside win rect %d\n",
        rgn_rect.top, win_rect.top );
    ok( win_rect.right >= rgn_rect.right, "rgn right %d not inside win rect %d\n",
        rgn_rect.right, win_rect.right );
    ok( win_rect.bottom >= rgn_rect.bottom, "rgn bottom %d not inside win rect %d\n",
        rgn_rect.bottom, win_rect.bottom );
    ReleaseDC( hwnd, hdc );
}

static LRESULT WINAPI set_focus_on_activate_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_ACTIVATE && LOWORD(wp) == WA_ACTIVE)
    {
        HWND child = GetWindow(hwnd, GW_CHILD);
        ok(child != 0, "couldn't find child window\n");
        SetFocus(child);
        ok(GetFocus() == child, "Focus should be on child %p\n", child);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static void test_SetFocus(HWND hwnd)
{
    HWND child;
    WNDPROC old_wnd_proc;

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

    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(child, SW_SHOW);
    SetFocus(child);
    ok( GetFocus() == child, "Focus should be on child %p\n", child );
    EnableWindow(hwnd, FALSE);
    ok( GetFocus() == child, "Focus should still be on child %p\n", child );
    EnableWindow(hwnd, TRUE);

    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ShowWindow(hwnd, SW_SHOWMINIMIZED);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
todo_wine
    ok( GetFocus() != child, "Focus should not be on child %p\n", child );
    ok( GetFocus() != hwnd, "Focus should not be on parent %p\n", hwnd );
    ShowWindow(hwnd, SW_RESTORE);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p\n", hwnd );
    ShowWindow(hwnd, SW_SHOWMINIMIZED);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() != child, "Focus should not be on child %p\n", child );
todo_wine
    ok( GetFocus() != hwnd, "Focus should not be on parent %p\n", hwnd );
    old_wnd_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)set_focus_on_activate_proc);
    ShowWindow(hwnd, SW_RESTORE);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
todo_wine
    ok( GetFocus() == child, "Focus should be on child %p, not %p\n", child, GetFocus() );

    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)old_wnd_proc);

    DestroyWindow( child );
}

#define check_wnd_state(a,b,c,d) check_wnd_state_(__FILE__,__LINE__,a,b,c,d)
static void check_wnd_state_(const char *file, int line,
                             HWND active, HWND foreground, HWND focus, HWND capture)
{
    ok_(file, line)(active == GetActiveWindow(), "GetActiveWindow() = %p\n", GetActiveWindow());
    if (foreground && GetForegroundWindow())
        ok_(file, line)(foreground == GetForegroundWindow(), "GetForegroundWindow() = %p\n", GetForegroundWindow());
    ok_(file, line)(focus == GetFocus(), "GetFocus() = %p\n", GetFocus());
    ok_(file, line)(capture == GetCapture(), "GetCapture() = %p\n", GetCapture());
}

static void test_SetActiveWindow(HWND hwnd)
{
    HWND hwnd2;

    flush_events( TRUE );
    ShowWindow(hwnd, SW_HIDE);
    SetFocus(0);
    SetActiveWindow(0);
    check_wnd_state(0, 0, 0, 0);

    /*trace("testing SetActiveWindow %p\n", hwnd);*/

    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = SetActiveWindow(0);
    ok(hwnd2 == hwnd, "SetActiveWindow returned %p instead of %p\n", hwnd2, hwnd);
    if (!GetActiveWindow())  /* doesn't always work on vista */
    {
        check_wnd_state(0, 0, 0, 0);
        hwnd2 = SetActiveWindow(hwnd);
        ok(hwnd2 == 0, "SetActiveWindow returned %p instead of 0\n", hwnd2);
    }
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    /*trace("testing SetActiveWindow on an invisible window %p\n", hwnd);*/
    SetActiveWindow(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    
    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    SetWindowPos(hwnd2,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
}

static void test_SetForegroundWindow(HWND hwnd)
{
    BOOL ret;
    HWND hwnd2;

    flush_events( TRUE );
    ShowWindow(hwnd, SW_HIDE);
    SetFocus(0);
    SetActiveWindow(0);
    check_wnd_state(0, 0, 0, 0);

    /*trace("testing SetForegroundWindow %p\n", hwnd);*/

    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = SetActiveWindow(0);
    ok(hwnd2 == hwnd, "SetActiveWindow(0) returned %p instead of %p\n", hwnd2, hwnd);
    if (GetActiveWindow() == hwnd)  /* doesn't always work on vista */
        check_wnd_state(hwnd, hwnd, hwnd, 0);
    else
        check_wnd_state(0, 0, 0, 0);

    ret = SetForegroundWindow(hwnd);
    ok(ret, "SetForegroundWindow returned FALSE instead of TRUE\n");
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetLastError(0xdeadbeef);
    ret = SetForegroundWindow(0);
    ok(!ret, "SetForegroundWindow returned TRUE instead of FALSE\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE ||
       broken(GetLastError() == 0xdeadbeef),  /* win9x */
       "got error %d expected ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = GetForegroundWindow();
    ok(hwnd2 == hwnd, "Wrong foreground window %p\n", hwnd2);
    ok(SetForegroundWindow( GetDesktopWindow() ), "SetForegroundWindow(desktop) error: %d\n", GetLastError());
    hwnd2 = GetForegroundWindow();
    ok(hwnd2 != hwnd, "Wrong foreground window %p\n", hwnd2);

    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    /*trace("testing SetForegroundWindow on an invisible window %p\n", hwnd);*/
    ret = SetForegroundWindow(hwnd);
    ok(ret, "SetForegroundWindow returned FALSE instead of TRUE\n");
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    
    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    SetWindowPos(hwnd2,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
}

static WNDPROC old_button_proc;

static LRESULT WINAPI button_hook_proc(HWND button, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret;
    USHORT key_state;

    key_state = GetKeyState(VK_LBUTTON);
    ok(!(key_state & 0x8000), "VK_LBUTTON should not be pressed, state %04x\n", key_state);

    ret = CallWindowProcA(old_button_proc, button, msg, wparam, lparam);

    if (msg == WM_LBUTTONDOWN)
    {
	HWND hwnd, capture;

	check_wnd_state(button, button, button, button);

	hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
	assert(hwnd);
	trace("hwnd %p\n", hwnd);

	check_wnd_state(button, button, button, button);

	ShowWindow(hwnd, SW_SHOWNOACTIVATE);

	check_wnd_state(button, button, button, button);

	DestroyWindow(hwnd);

	hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
	assert(hwnd);
	trace("hwnd %p\n", hwnd);

	check_wnd_state(button, button, button, button);

	/* button wnd proc should release capture on WM_KILLFOCUS if it does
	 * match internal button state.
	 */
	SendMessage(button, WM_KILLFOCUS, 0, 0);
	check_wnd_state(button, button, button, 0);

	ShowWindow(hwnd, SW_SHOW);
	check_wnd_state(hwnd, hwnd, hwnd, 0);

	capture = SetCapture(hwnd);
	ok(capture == 0, "SetCapture() = %p\n", capture);

	check_wnd_state(hwnd, hwnd, hwnd, hwnd);

	DestroyWindow(hwnd);

	check_wnd_state(button, 0, button, 0);
    }

    return ret;
}

static void test_capture_1(void)
{
    HWND button, capture, oldFocus, oldActive;

    oldFocus = GetFocus();
    oldActive = GetActiveWindow();

    capture = GetCapture();
    ok(capture == 0, "GetCapture() = %p\n", capture);

    button = CreateWindowExA(0, "button", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
    assert(button);
    trace("button %p\n", button);

    old_button_proc = (WNDPROC)SetWindowLongPtrA(button, GWLP_WNDPROC, (LONG_PTR)button_hook_proc);

    SendMessageA(button, WM_LBUTTONDOWN, 0, 0);

    capture = SetCapture(button);
    ok(capture == 0, "SetCapture() = %p\n", capture);
    check_wnd_state(button, 0, button, button);

    DestroyWindow(button);
    check_wnd_state(oldActive, 0, oldFocus, 0);
}

static void test_capture_2(void)
{
    HWND button, hwnd, capture, oldFocus, oldActive;

    oldFocus = GetFocus();
    oldActive = GetActiveWindow();
    check_wnd_state(oldActive, 0, oldFocus, 0);

    button = CreateWindowExA(0, "button", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
    assert(button);
    trace("button %p\n", button);

    check_wnd_state(button, button, button, 0);

    capture = SetCapture(button);
    ok(capture == 0, "SetCapture() = %p\n", capture);

    check_wnd_state(button, button, button, button);

    /* button wnd proc should ignore WM_KILLFOCUS if it doesn't match
     * internal button state.
     */
    SendMessage(button, WM_KILLFOCUS, 0, 0);
    check_wnd_state(button, button, button, button);

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
    assert(hwnd);
    trace("hwnd %p\n", hwnd);

    check_wnd_state(button, button, button, button);

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    check_wnd_state(button, button, button, button);

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
    assert(hwnd);
    trace("hwnd %p\n", hwnd);

    check_wnd_state(button, button, button, button);

    ShowWindow(hwnd, SW_SHOW);

    check_wnd_state(hwnd, hwnd, hwnd, button);

    capture = SetCapture(hwnd);
    ok(capture == button, "SetCapture() = %p\n", capture);

    check_wnd_state(hwnd, hwnd, hwnd, hwnd);

    DestroyWindow(hwnd);
    check_wnd_state(button, button, button, 0);

    DestroyWindow(button);
    check_wnd_state(oldActive, 0, oldFocus, 0);
}

static void test_capture_3(HWND hwnd1, HWND hwnd2)
{
    BOOL ret;

    ShowWindow(hwnd1, SW_HIDE);
    ShowWindow(hwnd2, SW_HIDE);

    ok(!IsWindowVisible(hwnd1), "%p should be invisible\n", hwnd1);
    ok(!IsWindowVisible(hwnd2), "%p should be invisible\n", hwnd2);

    SetCapture(hwnd1);
    check_wnd_state(0, 0, 0, hwnd1);

    SetCapture(hwnd2);
    check_wnd_state(0, 0, 0, hwnd2);

    ShowWindow(hwnd1, SW_SHOW);
    check_wnd_state(hwnd1, hwnd1, hwnd1, hwnd2);

    ret = ReleaseCapture();
    ok (ret, "releasecapture did not return TRUE.\n");
    ret = ReleaseCapture();
    ok (ret, "releasecapture did not return TRUE after second try.\n");
}

static void test_keyboard_input(HWND hwnd)
{
    MSG msg;
    BOOL ret;

    flush_events( TRUE );
    SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
    UpdateWindow(hwnd);
    flush_events( TRUE );

    ok(GetActiveWindow() == hwnd, "wrong active window %p\n", GetActiveWindow());

    SetFocus(hwnd);
    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    flush_events( TRUE );

    PostMessageA(hwnd, WM_KEYDOWN, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    PostThreadMessageA(GetCurrentThreadId(), WM_KEYDOWN, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(!msg.hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    keybd_event(VK_SPACE, 0, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    SetFocus(0);
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    flush_events( TRUE );

    PostMessageA(hwnd, WM_KEYDOWN, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    PostThreadMessageA(GetCurrentThreadId(), WM_KEYDOWN, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(!msg.hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    keybd_event(VK_SPACE, 0, 0, 0);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_SYSKEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);
}

static void test_mouse_input(HWND hwnd)
{
    RECT rc;
    POINT pt;
    int x, y;
    HWND popup;
    MSG msg;
    BOOL ret;
    LRESULT res;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    GetWindowRect(hwnd, &rc);
    trace("main window %p: (%d,%d)-(%d,%d)\n", hwnd, rc.left, rc.top, rc.right, rc.bottom);

    popup = CreateWindowExA(0, "MainWindowClass", NULL, WS_POPUP,
                            rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                            hwnd, 0, 0, NULL);
    assert(popup != 0);
    ShowWindow(popup, SW_SHOW);
    UpdateWindow(popup);

    GetWindowRect(popup, &rc);
    trace("popup window %p: (%d,%d)-(%d,%d)\n", popup, rc.left, rc.top, rc.right, rc.bottom);

    x = rc.left + (rc.right - rc.left) / 2;
    y = rc.top + (rc.bottom - rc.top) / 2;
    trace("setting cursor to (%d,%d)\n", x, y);

    SetCursorPos(x, y);
    GetCursorPos(&pt);
    ok(x == pt.x && y == pt.y, "wrong cursor pos (%d,%d), expected (%d,%d)\n", pt.x, pt.y, x, y);

    flush_events( TRUE );

    /* Check that setting the same position may generate WM_MOUSEMOVE */
    SetCursorPos(x, y);
    msg.message = 0;
    do
        ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    while (ret && msg.message >= 0xc000);  /* skip registered messages */
    if (ret)
    {
        ok(msg.hwnd == popup && msg.message == WM_MOUSEMOVE, "hwnd %p message %04x\n",
           msg.hwnd, msg.message);
        ok(msg.pt.x == x && msg.pt.y == y, "wrong message coords (%d,%d)/(%d,%d)\n",
           x, y, msg.pt.x, msg.pt.y);
    }

    /* force the system to update its internal queue mouse position,
     * otherwise it won't generate relative mouse movements below.
     */
    mouse_event(MOUSEEVENTF_MOVE, -1, -1, 0, 0);
    flush_events( TRUE );

    msg.message = 0;
    mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
    /* FIXME: SetCursorPos in Wine generates additional WM_MOUSEMOVE message */
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message >= 0xc000) continue;  /* skip registered messages */
        ok(msg.hwnd == popup && msg.message == WM_MOUSEMOVE,
           "hwnd %p message %04x\n", msg.hwnd, msg.message);
    }
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok( !ret, "message %04x available\n", msg.message);

    mouse_event(MOUSEEVENTF_MOVE, -1, -1, 0, 0);
    ShowWindow(popup, SW_HIDE);
    do
        ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    while (msg.message >= 0xc000);  /* skip registered messages */
    ok(msg.hwnd == hwnd && msg.message == WM_MOUSEMOVE, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
    ShowWindow(hwnd, SW_HIDE);
    do
        ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    while (ret && msg.message >= 0xc000);  /* skip registered messages */
    ok( !ret, "message %04x available\n", msg.message);
    flush_events( TRUE );

    /* test mouse clicks */

    ShowWindow(hwnd, SW_SHOW);
    flush_events( TRUE );
    ShowWindow(popup, SW_SHOW);
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    ok(PeekMessageA(&msg, 0, 0, 0, 0), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ok(PeekMessageA(&msg, 0, 0, 0, 0), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ok(PeekMessageA(&msg, 0, 0, 0, 0), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDBLCLK, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDBLCLK, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ok(PeekMessageA(&msg, 0, 0, 0, 0), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(!ret, "message %04x available\n", msg.message);

    ShowWindow(popup, SW_HIDE);
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, hwnd, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, hwnd, msg.message);

    test_lbuttondown_flag = TRUE;
    SendMessageA(hwnd, WM_COMMAND, (WPARAM)popup, 0);
    test_lbuttondown_flag = FALSE;

    do
        ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    while (msg.message >= 0xc000);  /* skip registered messages */
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE), "no message available\n");

    /* Test WM_MOUSEACTIVATE */
#define TEST_MOUSEACTIVATE(A,B)                                                          \
       res = SendMessageA(hwnd, WM_MOUSEACTIVATE, (WPARAM)hwnd, (LPARAM)MAKELRESULT(A,0));   \
       ok(res == B, "WM_MOUSEACTIVATE for %s returned %ld\n", #A, res);
       
    TEST_MOUSEACTIVATE(HTERROR,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTTRANSPARENT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTNOWHERE,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTCLIENT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTCAPTION,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTSYSMENU,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTSIZE,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTMENU,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTHSCROLL,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTVSCROLL,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTMINBUTTON,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTMAXBUTTON,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTLEFT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTRIGHT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTTOP,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTTOPLEFT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTTOPRIGHT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTBOTTOM,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTBOTTOMLEFT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTBOTTOMRIGHT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTBORDER,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTOBJECT,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTCLOSE,MA_ACTIVATE);
    TEST_MOUSEACTIVATE(HTHELP,MA_ACTIVATE);

    /* Clear any messages left behind by WM_MOUSEACTIVATE tests */
    flush_events( TRUE );

    DestroyWindow(popup);
}

static void test_validatergn(HWND hwnd)
{
    HWND child;
    RECT rc, rc2;
    HRGN rgn;
    int ret;
    child = CreateWindowExA(0, "static", NULL, WS_CHILD| WS_VISIBLE, 10, 10, 10, 10, hwnd, 0, 0, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow( hwnd);
    /* test that ValidateRect validates children*/
    InvalidateRect( child, NULL, 1);
    GetWindowRect( child, &rc);
    MapWindowPoints( NULL, hwnd, (POINT*) &rc, 2);
    ret = GetUpdateRect( child, &rc2, 0);
    ok( rc2.right > rc2.left && rc2.bottom > rc2.top,
            "Update rectangle is empty!\n");
    ValidateRect( hwnd, &rc);
    ret = GetUpdateRect( child, &rc2, 0);
    ok( rc2.left == 0 && rc2.top == 0 && rc2.right == 0 && rc2.bottom == 0,
            "Update rectangle %d,%d-%d,%d is not empty!\n", rc2.left, rc2.top,
            rc2.right, rc2.bottom);

    /* now test ValidateRgn */
    InvalidateRect( child, NULL, 1);
    GetWindowRect( child, &rc);
    MapWindowPoints( NULL, hwnd, (POINT*) &rc, 2);
    rgn = CreateRectRgnIndirect( &rc);
    ValidateRgn( hwnd, rgn);
    ret = GetUpdateRect( child, &rc2, 0);
    ok( rc2.left == 0 && rc2.top == 0 && rc2.right == 0 && rc2.bottom == 0,
            "Update rectangle %d,%d-%d,%d is not empty!\n", rc2.left, rc2.top,
            rc2.right, rc2.bottom);

    DeleteObject( rgn);
    DestroyWindow( child );
}

static void nccalchelper(HWND hwnd, INT x, INT y, RECT *prc)
{
    MoveWindow( hwnd, 0, 0, x, y, 0);
    GetWindowRect( hwnd, prc);
    trace("window rect is %d,%d - %d,%d\n",
            prc->left,prc->top,prc->right,prc->bottom);
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)prc);
    trace("nccalc rect is %d,%d - %d,%d\n",
            prc->left,prc->top,prc->right,prc->bottom);
}

static void test_nccalcscroll(HWND parent)
{
    RECT rc1;
    INT sbheight = GetSystemMetrics( SM_CYHSCROLL);
    INT sbwidth = GetSystemMetrics( SM_CXVSCROLL);
    HWND hwnd = CreateWindowExA(0, "static", NULL, 
            WS_CHILD| WS_VISIBLE | WS_VSCROLL | WS_HSCROLL , 
            10, 10, 200, 200, parent, 0, 0, NULL); 
    ShowWindow( parent, SW_SHOW);
    UpdateWindow( parent);

    /* test window too low for a horizontal scroll bar */
    nccalchelper( hwnd, 100, sbheight, &rc1);
    ok( rc1.bottom - rc1.top == sbheight, "Height should be %d size is %d,%d - %d,%d\n",
            sbheight, rc1.left, rc1.top, rc1.right, rc1.bottom);

    /* test window just high enough for a horizontal scroll bar */
    nccalchelper( hwnd, 100, sbheight + 1, &rc1);
    ok( rc1.bottom - rc1.top == 1, "Height should be %d size is %d,%d - %d,%d\n",
            1, rc1.left, rc1.top, rc1.right, rc1.bottom);

    /* test window too narrow for a vertical scroll bar */
    nccalchelper( hwnd, sbwidth - 1, 100, &rc1);
    ok( rc1.right - rc1.left == sbwidth - 1 , "Width should be %d size is %d,%d - %d,%d\n",
            sbwidth - 1, rc1.left, rc1.top, rc1.right, rc1.bottom);

    /* test window just wide enough for a vertical scroll bar */
    nccalchelper( hwnd, sbwidth, 100, &rc1);
    ok( rc1.right - rc1.left == 0, "Width should be %d size is %d,%d - %d,%d\n",
            0, rc1.left, rc1.top, rc1.right, rc1.bottom);

    /* same test, but with client edge: not enough width */
    SetWindowLong( hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE | GetWindowLong( hwnd, GWL_EXSTYLE));
    nccalchelper( hwnd, sbwidth, 100, &rc1);
    ok( rc1.right - rc1.left == sbwidth - 2 * GetSystemMetrics(SM_CXEDGE),
            "Width should be %d size is %d,%d - %d,%d\n",
            sbwidth - 2 * GetSystemMetrics(SM_CXEDGE), rc1.left, rc1.top, rc1.right, rc1.bottom);

    DestroyWindow( hwnd);
}

static void test_SetParent(void)
{
    BOOL ret;
    HWND desktop = GetDesktopWindow();
    HMENU hMenu;
    BOOL is_win9x = GetWindowLongPtrW(desktop, GWLP_WNDPROC) == 0;
    HWND parent, child1, child2, child3, child4, sibling;

    parent = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
			     100, 100, 200, 200, 0, 0, 0, NULL);
    assert(parent != 0);
    child1 = CreateWindowExA(0, "static", NULL, WS_CHILD,
			     0, 0, 50, 50, parent, 0, 0, NULL);
    assert(child1 != 0);
    child2 = CreateWindowExA(0, "static", NULL, WS_POPUP,
			     0, 0, 50, 50, child1, 0, 0, NULL);
    assert(child2 != 0);
    child3 = CreateWindowExA(0, "static", NULL, WS_CHILD,
			     0, 0, 50, 50, child2, 0, 0, NULL);
    assert(child3 != 0);
    child4 = CreateWindowExA(0, "static", NULL, WS_POPUP,
			     0, 0, 50, 50, child3, 0, 0, NULL);
    assert(child4 != 0);

    trace("parent %p, child1 %p, child2 %p, child3 %p, child4 %p\n",
	   parent, child1, child2, child3, child4);

    check_parents(parent, desktop, 0, 0, 0, parent, parent);
    check_parents(child1, parent, parent, parent, 0, parent, parent);
    check_parents(child2, desktop, parent, parent, parent, child2, parent);
    check_parents(child3, child2, child2, child2, 0, child2, parent);
    check_parents(child4, desktop, child2, child2, child2, child4, parent);

    ok(!IsChild(desktop, parent), "wrong parent/child %p/%p\n", desktop, parent);
    ok(!IsChild(desktop, child1), "wrong parent/child %p/%p\n", desktop, child1);
    ok(!IsChild(desktop, child2), "wrong parent/child %p/%p\n", desktop, child2);
    ok(!IsChild(desktop, child3), "wrong parent/child %p/%p\n", desktop, child3);
    ok(!IsChild(desktop, child4), "wrong parent/child %p/%p\n", desktop, child4);

    ok(IsChild(parent, child1), "wrong parent/child %p/%p\n", parent, child1);
    ok(!IsChild(desktop, child2), "wrong parent/child %p/%p\n", desktop, child2);
    ok(!IsChild(parent, child2), "wrong parent/child %p/%p\n", parent, child2);
    ok(!IsChild(child1, child2), "wrong parent/child %p/%p\n", child1, child2);
    ok(!IsChild(parent, child3), "wrong parent/child %p/%p\n", parent, child3);
    ok(IsChild(child2, child3), "wrong parent/child %p/%p\n", child2, child3);
    ok(!IsChild(parent, child4), "wrong parent/child %p/%p\n", parent, child4);
    ok(!IsChild(child3, child4), "wrong parent/child %p/%p\n", child3, child4);
    ok(!IsChild(desktop, child4), "wrong parent/child %p/%p\n", desktop, child4);

    if (!is_win9x) /* Win9x doesn't survive this test */
    {
        ok(!SetParent(parent, child1), "SetParent should fail\n");
        ok(!SetParent(child2, child3), "SetParent should fail\n");
        ok(SetParent(child1, parent) != 0, "SetParent should not fail\n");
        ok(SetParent(parent, child2) != 0, "SetParent should not fail\n");
        ok(SetParent(parent, child3) != 0, "SetParent should not fail\n");
        ok(!SetParent(child2, parent), "SetParent should fail\n");
        ok(SetParent(parent, child4) != 0, "SetParent should not fail\n");

        check_parents(parent, child4, child4, 0, 0, child4, parent);
        check_parents(child1, parent, parent, parent, 0, child4, parent);
        check_parents(child2, desktop, parent, parent, parent, child2, parent);
        check_parents(child3, child2, child2, child2, 0, child2, parent);
        check_parents(child4, desktop, child2, child2, child2, child4, parent);
    }

    hMenu = CreateMenu();
    sibling = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
			     100, 100, 200, 200, 0, hMenu, 0, NULL);
    assert(sibling != 0);

    ok(SetParent(sibling, parent) != 0, "SetParent should not fail\n");
    ok(GetMenu(sibling) == hMenu, "SetParent should not remove menu\n");

    ret = DestroyWindow(parent);
    ok( ret, "DestroyWindow() error %d\n", GetLastError());

    ok(!IsWindow(parent), "parent still exists\n");
    ok(!IsWindow(sibling), "sibling still exists\n");
    ok(!IsWindow(child1), "child1 still exists\n");
    ok(!IsWindow(child2), "child2 still exists\n");
    ok(!IsWindow(child3), "child3 still exists\n");
    ok(!IsWindow(child4), "child4 still exists\n");
}

static LRESULT WINAPI StyleCheckProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LPCREATESTRUCT lpcs;
    LPSTYLESTRUCT lpss;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        lpcs = (LPCREATESTRUCT)lparam;
        lpss = (LPSTYLESTRUCT)lpcs->lpCreateParams;
        if (lpss)
        {
            if ((lpcs->dwExStyle & WS_EX_DLGMODALFRAME) ||
                ((!(lpcs->dwExStyle & WS_EX_STATICEDGE)) &&
                    (lpcs->style & (WS_DLGFRAME | WS_THICKFRAME))))
                ok(lpcs->dwExStyle & WS_EX_WINDOWEDGE, "Window should have WS_EX_WINDOWEDGE style\n");
            else
                ok(!(lpcs->dwExStyle & WS_EX_WINDOWEDGE), "Window shouldn't have WS_EX_WINDOWEDGE style\n");

            ok((lpss->styleOld & ~WS_EX_WINDOWEDGE) == (lpcs->dwExStyle & ~WS_EX_WINDOWEDGE),
                "Ex style (0x%08lx) should match what the caller passed to CreateWindowEx (0x%08lx)\n",
                (lpss->styleOld & ~WS_EX_WINDOWEDGE), (lpcs->dwExStyle & ~WS_EX_WINDOWEDGE));

            ok(lpss->styleNew == lpcs->style,
                "Style (0x%08x) should match what the caller passed to CreateWindowEx (0x%08x)\n",
                lpss->styleNew, lpcs->style);
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static ATOM atomStyleCheckClass;

static void register_style_check_class(void)
{
    WNDCLASS wc =
    {
        0,
        StyleCheckProc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineStyleCheck"),
    };
    
    atomStyleCheckClass = RegisterClass(&wc);
}

static void check_window_style(DWORD dwStyleIn, DWORD dwExStyleIn, DWORD dwStyleOut, DWORD dwExStyleOut)
{
    DWORD dwActualStyle;
    DWORD dwActualExStyle;
    STYLESTRUCT ss;
    HWND hwnd;
    HWND hwndParent = NULL;

    ss.styleNew = dwStyleIn;
    ss.styleOld = dwExStyleIn;

    if (dwStyleIn & WS_CHILD)
    {
        hwndParent = CreateWindowEx(0, MAKEINTATOM(atomStyleCheckClass), NULL,
            WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    }

    hwnd = CreateWindowEx(dwExStyleIn, MAKEINTATOM(atomStyleCheckClass), NULL,
                    dwStyleIn, 0, 0, 0, 0, hwndParent, NULL, NULL, &ss);
    assert(hwnd);

    flush_events( TRUE );

    dwActualStyle = GetWindowLong(hwnd, GWL_STYLE);
    dwActualExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    ok((dwActualStyle == dwStyleOut) && (dwActualExStyle == dwExStyleOut),
        "Style (0x%08x) should really be 0x%08x and/or Ex style (0x%08x) should really be 0x%08x\n",
        dwActualStyle, dwStyleOut, dwActualExStyle, dwExStyleOut);

    DestroyWindow(hwnd);
    if (hwndParent) DestroyWindow(hwndParent);
}

/* tests what window styles the window manager automatically adds */
static void test_window_styles(void)
{
    register_style_check_class();

    check_window_style(0, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_window_style(WS_OVERLAPPEDWINDOW, 0, WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD, 0, WS_CHILD, 0);
    check_window_style(WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0);
    check_window_style(0, WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW);
    check_window_style(WS_POPUP, 0, WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_POPUP, WS_EX_WINDOWEDGE, WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_CHILD, WS_EX_DLGMODALFRAME, WS_CHILD, WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_window_style(WS_CHILD, WS_EX_DLGMODALFRAME|WS_EX_STATICEDGE, WS_CHILD, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_window_style(WS_CAPTION, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE);
    check_window_style(0, WS_EX_APPWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE);
}

static void test_scrollvalidate( HWND parent)
{
    HDC hdc;
    HRGN hrgn=CreateRectRgn(0,0,0,0);
    HRGN exprgn, tmprgn, clipping;
    RECT rc, rcu, cliprc;
    /* create two overlapping child windows. The visual region
     * of hwnd1 is clipped by the overlapping part of
     * hwnd2 because of the WS_CLIPSIBLING style */
    HWND hwnd1, hwnd2;

    clipping = CreateRectRgn(0,0,0,0);
    tmprgn = CreateRectRgn(0,0,0,0);
    exprgn = CreateRectRgn(0,0,0,0);
    hwnd2 = CreateWindowExA(0, "static", NULL,
            WS_CHILD| WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER ,
            75, 30, 100, 100, parent, 0, 0, NULL);
    hwnd1 = CreateWindowExA(0, "static", NULL,
            WS_CHILD| WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER ,
            25, 50, 100, 100, parent, 0, 0, NULL);
    ShowWindow( parent, SW_SHOW);
    UpdateWindow( parent);
    GetClientRect( hwnd1, &rc);
    cliprc=rc; 
    SetRectRgn( clipping, 10, 10, 90, 90);
    hdc = GetDC( hwnd1);
    /* for a visual touch */
    TextOut( hdc, 0,10, "0123456789", 10);
    ScrollDC( hdc, -10, -5, &rc, &cliprc, hrgn, &rcu);
    if (winetest_debug > 0) dump_region(hrgn);
    /* create a region with what is expected */
    SetRectRgn( exprgn, 39,0,49,74);
    SetRectRgn( tmprgn, 88,79,98,93);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 0,93,98,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    trace("update rect is %d,%d - %d,%d\n",
            rcu.left,rcu.top,rcu.right,rcu.bottom);
    /* now with clipping region */
    SelectClipRgn( hdc, clipping);
    ScrollDC( hdc, -10, -5, &rc, &cliprc, hrgn, &rcu);
    if (winetest_debug > 0) dump_region(hrgn);
    /* create a region with what is expected */
    SetRectRgn( exprgn, 39,10,49,74);
    SetRectRgn( tmprgn, 80,79,90,85);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 10,85,90,90);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    trace("update rect is %d,%d - %d,%d\n",
            rcu.left,rcu.top,rcu.right,rcu.bottom);
    ReleaseDC( hwnd1, hdc);

    /* test scrolling a window with an update region */
    DestroyWindow( hwnd2);
    ValidateRect( hwnd1, NULL);
    SetRect( &rc, 40,40, 50,50);
    InvalidateRect( hwnd1, &rc, 1);
    GetClientRect( hwnd1, &rc);
    cliprc=rc;
    ScrollWindowEx( hwnd1, -10, 0, &rc, &cliprc, hrgn, &rcu,
      SW_SCROLLCHILDREN | SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    SetRectRgn( exprgn, 88,0,98,98);
    SetRectRgn( tmprgn, 30, 40, 50, 50);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* clear an update region */
    UpdateWindow( hwnd1 );

    SetRect( &rc, 0,40, 100,60);
    SetRect( &cliprc, 0,0, 100,100);
    ScrollWindowEx( hwnd1, 0, -25, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    if (winetest_debug > 0) dump_region( hrgn );
    SetRectRgn( exprgn, 0, 40, 98, 60 );
    ok( EqualRgn( exprgn, hrgn), "wrong update region in excessive scroll\n");

    /* now test ScrollWindowEx with a combination of
     * WS_CLIPCHILDREN style and SW_SCROLLCHILDREN flag */
    /* make hwnd2 the child of hwnd1 */
    hwnd2 = CreateWindowExA(0, "static", NULL,
            WS_CHILD| WS_VISIBLE | WS_BORDER ,
            50, 50, 100, 100, hwnd1, 0, 0, NULL);
    SetWindowLong( hwnd1, GWL_STYLE, GetWindowLong( hwnd1, GWL_STYLE) & ~WS_CLIPSIBLINGS);
    GetClientRect( hwnd1, &rc);
    cliprc=rc;

    /* WS_CLIPCHILDREN and SW_SCROLLCHILDREN */
    SetWindowLong( hwnd1, GWL_STYLE, GetWindowLong( hwnd1, GWL_STYLE) | WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu,
      SW_SCROLLCHILDREN | SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    SetRectRgn( exprgn, 88,0,98,88);
    SetRectRgn( tmprgn, 0,88,98,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* SW_SCROLLCHILDREN */
    SetWindowLong( hwnd1, GWL_STYLE, GetWindowLong( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_SCROLLCHILDREN | SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* no SW_SCROLLCHILDREN */
    SetWindowLong( hwnd1, GWL_STYLE, GetWindowLong( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* WS_CLIPCHILDREN and no SW_SCROLLCHILDREN */
    SetWindowLong( hwnd1, GWL_STYLE, GetWindowLong( hwnd1, GWL_STYLE) | WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    SetRectRgn( exprgn, 88,0,98,20);
    SetRectRgn( tmprgn, 20,20,98,30);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 20,30,30,88);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 0,88,30,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* clean up */
    DeleteObject( hrgn);
    DeleteObject( exprgn);
    DeleteObject( tmprgn);
    DestroyWindow( hwnd1);
    DestroyWindow( hwnd2);
}

/* couple of tests of return values of scrollbar functions
 * called on a scrollbarless window */ 
static void test_scroll(void)
{
    BOOL ret;
    INT min, max;
    SCROLLINFO si;
    HWND hwnd = CreateWindowExA(0, "Static", "Wine test window",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP,
        100, 100, 200, 200, 0, 0, 0, NULL);
    /* horizontal */
    ret = GetScrollRange( hwnd, SB_HORZ, &min, &max);
    if (!ret)  /* win9x */
    {
        win_skip( "GetScrollRange doesn't work\n" );
        DestroyWindow( hwnd);
        return;
    }
    ok( min == 0, "minimum scroll pos is %d (should be zero)\n", min);
    ok( max == 0, "maximum scroll pos is %d (should be zero)\n", min);
    si.cbSize = sizeof( si);
    si.fMask = SIF_PAGE;
    si.nPage = 0xdeadbeef;
    ret = GetScrollInfo( hwnd, SB_HORZ, &si);
    ok( !ret, "GetScrollInfo returns %d (should be zero)\n", ret);
    ok( si.nPage == 0xdeadbeef, "unexpected value for nPage is %d\n", si.nPage);
    /* vertical */
    ret = GetScrollRange( hwnd, SB_VERT, &min, &max);
    ok( ret, "GetScrollRange returns FALSE\n");
    ok( min == 0, "minimum scroll pos is %d (should be zero)\n", min);
    ok( max == 0, "maximum scroll pos is %d (should be zero)\n", min);
    si.cbSize = sizeof( si);
    si.fMask = SIF_PAGE;
    si.nPage = 0xdeadbeef;
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    ok( !ret, "GetScrollInfo returns %d (should be zero)\n", ret);
    ok( si.nPage == 0xdeadbeef, "unexpected value for nPage is %d\n", si.nPage);
    /* clean up */
    DestroyWindow( hwnd);
}

static void test_scrolldc( HWND parent)
{
    HDC hdc;
    HRGN exprgn, tmprgn, hrgn;
    RECT rc, rc2, rcu, cliprc;
    HWND hwnd1;
    COLORREF colr;

    hrgn = CreateRectRgn(0,0,0,0);
    tmprgn = CreateRectRgn(0,0,0,0);
    exprgn = CreateRectRgn(0,0,0,0);

    hwnd1 = CreateWindowExA(0, "static", NULL,
            WS_CHILD| WS_VISIBLE,
            25, 50, 100, 100, parent, 0, 0, NULL);
    ShowWindow( parent, SW_SHOW);
    UpdateWindow( parent);
    flush_events( TRUE );
    GetClientRect( hwnd1, &rc);
    hdc = GetDC( hwnd1);
    /* paint the upper half of the window black */
    rc2 = rc;
    rc2.bottom = ( rc.top + rc.bottom) /2;
    FillRect( hdc, &rc2, GetStockObject(BLACK_BRUSH));
    /* clip region is the lower half */
    cliprc=rc; 
    cliprc.top = (rc.top + rc.bottom) /2;
    /* test whether scrolled pixels are properly clipped */ 
    colr = GetPixel( hdc, (rc.left+rc.right)/2, ( rc.top + rc.bottom) /2 - 1);
    ok ( colr == 0, "pixel should be black, color is %08x\n", colr);
    /* this scroll should not cause any visible changes */ 
    ScrollDC( hdc, 5, -20, &rc, &cliprc, hrgn, &rcu);
    colr = GetPixel( hdc, (rc.left+rc.right)/2, ( rc.top + rc.bottom) /2 - 1);
    ok ( colr == 0, "pixel should be black, color is %08x\n", colr);
    /* test with NULL clip rect */
    ScrollDC( hdc, 20, -20, &rc, NULL, hrgn, &rcu);
    /*FillRgn(hdc, hrgn, GetStockObject(WHITE_BRUSH));*/
    trace("update rect: %d,%d - %d,%d\n",
           rcu.left, rcu.top, rcu.right, rcu.bottom);
    if (winetest_debug > 0) dump_region(hrgn);
    SetRect(&rc2, 0, 0, 100, 100);
    ok(EqualRect(&rcu, &rc2), "rects do not match (%d,%d-%d,%d) / (%d,%d-%d,%d)\n",
       rcu.left, rcu.top, rcu.right, rcu.bottom, rc2.left, rc2.top, rc2.right, rc2.bottom);

    SetRectRgn( exprgn, 0, 0, 20, 80);
    SetRectRgn( tmprgn, 0, 80, 100, 100);
    CombineRgn(exprgn, exprgn, tmprgn, RGN_OR);
    if (winetest_debug > 0) dump_region(exprgn);
    ok(EqualRgn(exprgn, hrgn), "wrong update region\n");
    /* test clip rect > scroll rect */ 
    FillRect( hdc, &rc, GetStockObject(WHITE_BRUSH));
    rc2=rc;
    InflateRect( &rc2, -(rc.right-rc.left)/4, -(rc.bottom-rc.top)/4);
    FillRect( hdc, &rc2, GetStockObject(BLACK_BRUSH));
    ScrollDC( hdc, 10, 10, &rc2, &rc, hrgn, &rcu);
    SetRectRgn( exprgn, 25, 25, 75, 35);
    SetRectRgn( tmprgn, 25, 35, 35, 75);
    CombineRgn(exprgn, exprgn, tmprgn, RGN_OR);
    ok(EqualRgn(exprgn, hrgn), "wrong update region\n");
    colr = GetPixel( hdc, 80, 80);
    ok ( colr == 0, "pixel should be black, color is %08x\n", colr);
    trace("update rect: %d,%d - %d,%d\n",
           rcu.left, rcu.top, rcu.right, rcu.bottom);
    if (winetest_debug > 0) dump_region(hrgn);

    /* clean up */
    DeleteObject(hrgn);
    DeleteObject(exprgn);
    DeleteObject(tmprgn);
    DestroyWindow(hwnd1);
}

static void test_params(void)
{
    HWND hwnd;
    INT rc;

    /* Just a param check */
    if (pGetMonitorInfoA)
    {
        SetLastError(0xdeadbeef);
        rc = GetWindowText(hwndMain2, NULL, 1024);
        ok( rc==0, "GetWindowText: rc=%d err=%d\n",rc,GetLastError());
    }
    else
    {
        /* Skips actually on Win95 and NT4 */
        win_skip("Test would crash on Win95\n");
    }

    SetLastError(0xdeadbeef);
    hwnd=CreateWindow("LISTBOX", "TestList",
                      (LBS_STANDARD & ~LBS_SORT),
                      0, 0, 100, 100,
                      NULL, (HMENU)1, NULL, 0);

    ok(!hwnd, "CreateWindow with invalid menu handle should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE || /* NT */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "wrong last error value %d\n", GetLastError());
}

static void test_AWRwindow(LPCSTR class, LONG style, LONG exStyle, BOOL menu)
{
    HWND hwnd = 0;

    hwnd = CreateWindowEx(exStyle, class, class, style,
			  110, 100,
			  225, 200,
			  0,
			  menu ? hmenu : 0,
			  0, 0);
    if (!hwnd) {
	trace("Failed to create window class=%s, style=0x%08x, exStyle=0x%08x\n", class, style, exStyle);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);

    test_nonclient_area(hwnd);

    SetMenu(hwnd, 0);
    DestroyWindow(hwnd);
}

static BOOL AWR_init(void)
{
    WNDCLASS class;

    class.style         = CS_HREDRAW | CS_VREDRAW;
    class.lpfnWndProc     = DefWindowProcA;
    class.cbClsExtra    = 0;
    class.cbWndExtra    = 0;
    class.hInstance     = 0;
    class.hIcon         = LoadIcon (0, IDI_APPLICATION);
    class.hCursor       = LoadCursor (0, IDC_ARROW);
    class.hbrBackground = 0;
    class.lpszMenuName  = 0;
    class.lpszClassName = szAWRClass;
    
    if (!RegisterClass (&class)) {
	ok(FALSE, "RegisterClass failed\n");
	return FALSE;
    }

    hmenu = CreateMenu();
    if (!hmenu)
	return FALSE;
    ok(hmenu != 0, "Failed to create menu\n");
    ok(AppendMenu(hmenu, MF_STRING, 1, "Test!"), "Failed to create menu item\n");
    
    return TRUE;
}


static void test_AWR_window_size(BOOL menu)
{
    LONG styles[] = {
	WS_POPUP,
	WS_MAXIMIZE, WS_BORDER, WS_DLGFRAME, 
	WS_SYSMENU, 
	WS_THICKFRAME,
	WS_MINIMIZEBOX, WS_MAXIMIZEBOX,
	WS_HSCROLL, WS_VSCROLL
    };
    LONG exStyles[] = {
	WS_EX_CLIENTEDGE,
	WS_EX_TOOLWINDOW, WS_EX_WINDOWEDGE,
	WS_EX_APPWINDOW,
#if 0
	/* These styles have problems on (at least) WinXP (SP2) and Wine */
	WS_EX_DLGMODALFRAME, 
	WS_EX_STATICEDGE, 
#endif
    };

    int i;    

    /* A exhaustive check of all the styles takes too long
     * so just do a (hopefully representative) sample
     */
    for (i = 0; i < COUNTOF(styles); ++i)
        test_AWRwindow(szAWRClass, styles[i], 0, menu);
    for (i = 0; i < COUNTOF(exStyles); ++i) {
        test_AWRwindow(szAWRClass, WS_POPUP, exStyles[i], menu);
        test_AWRwindow(szAWRClass, WS_THICKFRAME, exStyles[i], menu);
    }
}
#undef COUNTOF

#define SHOWSYSMETRIC(SM) trace(#SM "=%d\n", GetSystemMetrics(SM))

static void test_AdjustWindowRect(void)
{
    if (!AWR_init())
	return;
    
    SHOWSYSMETRIC(SM_CYCAPTION);
    SHOWSYSMETRIC(SM_CYSMCAPTION);
    SHOWSYSMETRIC(SM_CYMENU);
    SHOWSYSMETRIC(SM_CXEDGE);
    SHOWSYSMETRIC(SM_CYEDGE);
    SHOWSYSMETRIC(SM_CXVSCROLL);
    SHOWSYSMETRIC(SM_CYHSCROLL);
    SHOWSYSMETRIC(SM_CXFRAME);
    SHOWSYSMETRIC(SM_CYFRAME);
    SHOWSYSMETRIC(SM_CXDLGFRAME);
    SHOWSYSMETRIC(SM_CYDLGFRAME);
    SHOWSYSMETRIC(SM_CXBORDER);
    SHOWSYSMETRIC(SM_CYBORDER);  

    test_AWR_window_size(FALSE);
    test_AWR_window_size(TRUE);

    DestroyMenu(hmenu);
}
#undef SHOWSYSMETRIC


/* Global variables to trigger exit from loop */
static int redrawComplete, WMPAINT_count;

static LRESULT WINAPI redraw_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_PAINT:
        trace("doing WM_PAINT %d\n", WMPAINT_count);
        WMPAINT_count++;
        if (WMPAINT_count > 10 && redrawComplete == 0) {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 1;
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* Ensure we exit from RedrawNow regardless of invalidated area */
static void test_redrawnow(void)
{
   WNDCLASSA cls;
   HWND hwndMain;

   cls.style = CS_DBLCLKS;
   cls.lpfnWndProc = redraw_window_procA;
   cls.cbClsExtra = 0;
   cls.cbWndExtra = 0;
   cls.hInstance = GetModuleHandleA(0);
   cls.hIcon = 0;
   cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
   cls.hbrBackground = GetStockObject(WHITE_BRUSH);
   cls.lpszMenuName = NULL;
   cls.lpszClassName = "RedrawWindowClass";

   if(!RegisterClassA(&cls)) {
       trace("Register failed %d\n", GetLastError());
       return;
   }

   hwndMain = CreateWindowA("RedrawWindowClass", "Main Window", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, 100, 100, NULL, NULL, 0, NULL);

   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   ShowWindow(hwndMain, SW_SHOW);
   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   RedrawWindow(hwndMain, NULL,NULL,RDW_UPDATENOW | RDW_ALLCHILDREN);
   ok( WMPAINT_count == 1, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   redrawComplete = TRUE;
   ok( WMPAINT_count < 10, "RedrawWindow (RDW_UPDATENOW) never completed (%d)\n", WMPAINT_count);

   /* clean up */
   DestroyWindow( hwndMain);
}

struct parentdc_stat {
    RECT client;
    RECT clip;
    RECT paint;
};

struct parentdc_test {
   struct parentdc_stat main, main_todo;
   struct parentdc_stat child1, child1_todo;
   struct parentdc_stat child2, child2_todo;
};

static LRESULT WINAPI parentdc_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    RECT rc;
    PAINTSTRUCT ps;

    struct parentdc_stat *t = (struct parentdc_stat *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_PAINT:
        trace("doing WM_PAINT on %p\n", hwnd);
        GetClientRect(hwnd, &rc);
        CopyRect(&t->client, &rc);
        trace("client rect (%d, %d)-(%d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
        GetWindowRect(hwnd, &rc);
        trace("window rect (%d, %d)-(%d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
        BeginPaint(hwnd, &ps);
        CopyRect(&t->paint, &ps.rcPaint);
        GetClipBox(ps.hdc, &rc);
        CopyRect(&t->clip, &rc);
        trace("clip rect (%d, %d)-(%d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
        trace("paint rect (%d, %d)-(%d, %d)\n", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void zero_parentdc_stat(struct parentdc_stat *t)
{
    SetRectEmpty(&t->client);
    SetRectEmpty(&t->clip);
    SetRectEmpty(&t->paint);
}

static void zero_parentdc_test(struct parentdc_test *t)
{
    zero_parentdc_stat(&t->main);
    zero_parentdc_stat(&t->child1);
    zero_parentdc_stat(&t->child2);
}

#define parentdc_field_ok(t, w, r, f, got) \
  ok (t.w.r.f==got.w.r.f, "window " #w ", rect " #r ", field " #f \
      ": expected %d, got %d\n", \
      t.w.r.f, got.w.r.f)

#define parentdc_todo_field_ok(t, w, r, f, got) \
  if (t.w##_todo.r.f) todo_wine { parentdc_field_ok(t, w, r, f, got); } \
  else parentdc_field_ok(t, w, r, f, got)

#define parentdc_rect_ok(t, w, r, got) \
  parentdc_todo_field_ok(t, w, r, left, got); \
  parentdc_todo_field_ok(t, w, r, top, got); \
  parentdc_todo_field_ok(t, w, r, right, got); \
  parentdc_todo_field_ok(t, w, r, bottom, got);

#define parentdc_win_ok(t, w, got) \
  parentdc_rect_ok(t, w, client, got); \
  parentdc_rect_ok(t, w, clip, got); \
  parentdc_rect_ok(t, w, paint, got);

#define parentdc_ok(t, got) \
  parentdc_win_ok(t, main, got); \
  parentdc_win_ok(t, child1, got); \
  parentdc_win_ok(t, child2, got);

static void test_csparentdc(void)
{
   WNDCLASSA clsMain, cls;
   HWND hwndMain, hwnd1, hwnd2;
   RECT rc;

   struct parentdc_test test_answer;

#define nothing_todo {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
   const struct parentdc_test test1 = 
   {
        {{0, 0, 150, 150}, {0, 0, 150, 150}, {0, 0, 150, 150}}, nothing_todo,
        {{0, 0, 40, 40}, {-20, -20, 130, 130}, {0, 0, 40, 40}}, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
        {{0, 0, 40, 40}, {-40, -40, 110, 110}, {0, 0, 40, 40}}, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
   };

   const struct parentdc_test test2 = 
   {
        {{0, 0, 150, 150}, {0, 0, 50, 50}, {0, 0, 50, 50}}, nothing_todo,
        {{0, 0, 40, 40}, {-20, -20, 30, 30}, {0, 0, 30, 30}}, {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
        {{0, 0, 40, 40}, {-40, -40, 10, 10}, {0, 0, 10, 10}}, {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
   };

   const struct parentdc_test test3 = 
   {
        {{0, 0, 150, 150}, {0, 0, 10, 10}, {0, 0, 10, 10}}, nothing_todo,
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
   };

   const struct parentdc_test test4 = 
   {
        {{0, 0, 150, 150}, {40, 40, 50, 50}, {40, 40, 50, 50}}, nothing_todo,
        {{0, 0, 40, 40}, {20, 20, 30, 30}, {20, 20, 30, 30}}, nothing_todo,
        {{0, 0, 40, 40}, {0, 0, 10, 10}, {0, 0, 10, 10}}, nothing_todo,
   };

   const struct parentdc_test test5 = 
   {
        {{0, 0, 150, 150}, {20, 20, 60, 60}, {20, 20, 60, 60}}, nothing_todo,
        {{0, 0, 40, 40}, {-20, -20, 130, 130}, {0, 0, 40, 40}}, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
        {{0, 0, 40, 40}, {-20, -20, 20, 20}, {0, 0, 20, 20}}, {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
   };

   const struct parentdc_test test6 = 
   {
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
        {{0, 0, 40, 40}, {0, 0, 10, 10}, {0, 0, 10, 10}}, nothing_todo,
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
   };

   const struct parentdc_test test7 = 
   {
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
        {{0, 0, 40, 40}, {-20, -20, 130, 130}, {0, 0, 40, 40}}, {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
        {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, nothing_todo,
   };
#undef nothing_todo

   clsMain.style = CS_DBLCLKS;
   clsMain.lpfnWndProc = parentdc_window_procA;
   clsMain.cbClsExtra = 0;
   clsMain.cbWndExtra = 0;
   clsMain.hInstance = GetModuleHandleA(0);
   clsMain.hIcon = 0;
   clsMain.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
   clsMain.hbrBackground = GetStockObject(WHITE_BRUSH);
   clsMain.lpszMenuName = NULL;
   clsMain.lpszClassName = "ParentDcMainWindowClass";

   if(!RegisterClassA(&clsMain)) {
       trace("Register failed %d\n", GetLastError());
       return;
   }

   cls.style = CS_DBLCLKS | CS_PARENTDC;
   cls.lpfnWndProc = parentdc_window_procA;
   cls.cbClsExtra = 0;
   cls.cbWndExtra = 0;
   cls.hInstance = GetModuleHandleA(0);
   cls.hIcon = 0;
   cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
   cls.hbrBackground = GetStockObject(WHITE_BRUSH);
   cls.lpszMenuName = NULL;
   cls.lpszClassName = "ParentDcWindowClass";

   if(!RegisterClassA(&cls)) {
       trace("Register failed %d\n", GetLastError());
       return;
   }

   SetRect(&rc, 0, 0, 150, 150);
   AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
   hwndMain = CreateWindowA("ParentDcMainWindowClass", "Main Window", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, 0, NULL);
   SetWindowLongPtrA(hwndMain, GWLP_USERDATA, (DWORD_PTR)&test_answer.main);
   hwnd1 = CreateWindowA("ParentDcWindowClass", "Child Window 1", WS_CHILD,
                            20, 20, 40, 40, hwndMain, NULL, 0, NULL);
   SetWindowLongPtrA(hwnd1, GWLP_USERDATA, (DWORD_PTR)&test_answer.child1);
   hwnd2 = CreateWindowA("ParentDcWindowClass", "Child Window 2", WS_CHILD,
                            40, 40, 40, 40, hwndMain, NULL, 0, NULL);
   SetWindowLongPtrA(hwnd2, GWLP_USERDATA, (DWORD_PTR)&test_answer.child2);
   ShowWindow(hwndMain, SW_SHOW);
   ShowWindow(hwnd1, SW_SHOW);
   ShowWindow(hwnd2, SW_SHOW);
   flush_events( TRUE );

   zero_parentdc_test(&test_answer);
   InvalidateRect(hwndMain, NULL, TRUE);
   flush_events( TRUE );
   parentdc_ok(test1, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, 0, 0, 50, 50);
   InvalidateRect(hwndMain, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test2, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, 0, 0, 10, 10);
   InvalidateRect(hwndMain, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test3, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, 40, 40, 50, 50);
   InvalidateRect(hwndMain, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test4, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, 20, 20, 60, 60);
   InvalidateRect(hwndMain, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test5, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, 0, 0, 10, 10);
   InvalidateRect(hwnd1, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test6, test_answer);

   zero_parentdc_test(&test_answer);
   SetRect(&rc, -5, -5, 65, 65);
   InvalidateRect(hwnd1, &rc, TRUE);
   flush_events( TRUE );
   parentdc_ok(test7, test_answer);

   DestroyWindow(hwndMain);
   DestroyWindow(hwnd1);
   DestroyWindow(hwnd2);
}

static LRESULT WINAPI def_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI def_window_procW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static void test_IsWindowUnicode(void)
{
    static const char ansi_class_nameA[] = "ansi class name";
    static const WCHAR ansi_class_nameW[] = {'a','n','s','i',' ','c','l','a','s','s',' ','n','a','m','e',0};
    static const char unicode_class_nameA[] = "unicode class name";
    static const WCHAR unicode_class_nameW[] = {'u','n','i','c','o','d','e',' ','c','l','a','s','s',' ','n','a','m','e',0};
    WNDCLASSA classA;
    WNDCLASSW classW;
    HWND hwnd;

    memset(&classW, 0, sizeof(classW));
    classW.hInstance = GetModuleHandleA(0);
    classW.lpfnWndProc = def_window_procW;
    classW.lpszClassName = unicode_class_nameW;
    if (!RegisterClassW(&classW)) return; /* this catches Win9x as well */

    memset(&classA, 0, sizeof(classA));
    classA.hInstance = GetModuleHandleA(0);
    classA.lpfnWndProc = def_window_procA;
    classA.lpszClassName = ansi_class_nameA;
    assert(RegisterClassA(&classA));

    /* unicode class: window proc */
    hwnd = CreateWindowExW(0, unicode_class_nameW, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, unicode_class_nameA, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");

    DestroyWindow(hwnd);

    /* ansi class: window proc */
    hwnd = CreateWindowExW(0, ansi_class_nameW, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, ansi_class_nameA, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");

    DestroyWindow(hwnd);

    /* unicode class: class proc */
    hwnd = CreateWindowExW(0, unicode_class_nameW, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetClassLongPtrA(hwnd, GCLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    /* do not restore class window proc back to unicode */

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, unicode_class_nameA, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetClassLongPtrW(hwnd, GCLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");

    DestroyWindow(hwnd);

    /* ansi class: class proc */
    hwnd = CreateWindowExW(0, ansi_class_nameW, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    SetClassLongPtrW(hwnd, GCLP_WNDPROC, (ULONG_PTR)def_window_procW);
    ok(!IsWindowUnicode(hwnd), "IsWindowUnicode expected to return FALSE\n");
    /* do not restore class window proc back to ansi */

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, ansi_class_nameA, NULL, WS_POPUP,
                           0, 0, 100, 100, GetDesktopWindow(), 0, 0, NULL);
    assert(hwnd);

    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");
    SetClassLongPtrA(hwnd, GCLP_WNDPROC, (ULONG_PTR)def_window_procA);
    ok(IsWindowUnicode(hwnd), "IsWindowUnicode expected to return TRUE\n");

    DestroyWindow(hwnd);
}

static LRESULT CALLBACK minmax_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    MINMAXINFO *minmax;

    if (msg != WM_GETMINMAXINFO)
        return DefWindowProc(hwnd, msg, wp, lp);

    minmax = (MINMAXINFO *)lp;

    if ((GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD))
    {
        minmax->ptReserved.x = 0;
        minmax->ptReserved.y = 0;
        minmax->ptMaxSize.x = 400;
        minmax->ptMaxSize.y = 400;
        minmax->ptMaxPosition.x = 300;
        minmax->ptMaxPosition.y = 300;
        minmax->ptMaxTrackSize.x = 200;
        minmax->ptMaxTrackSize.y = 200;
        minmax->ptMinTrackSize.x = 100;
        minmax->ptMinTrackSize.y = 100;
    }
    else
        DefWindowProc(hwnd, msg, wp, lp);
    return 1;
}

static int expected_cx, expected_cy;
static RECT expected_rect, broken_rect;

static LRESULT CALLBACK winsizes_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
    case WM_GETMINMAXINFO:
    {
        RECT rect;
        GetWindowRect( hwnd, &rect );
        ok( !rect.left && !rect.top && !rect.right && !rect.bottom,
            "wrong rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
        RECT rect;
        GetWindowRect( hwnd, &rect );
        trace( "hwnd %p msg %x size %dx%d rect %d,%d-%d,%d\n",
               hwnd, msg, cs->cx, cs->cy, rect.left, rect.top, rect.right, rect.bottom );
        ok( cs->cx == expected_cx || broken(cs->cx == (short)expected_cx),
            "wrong x size %d/%d\n", cs->cx, expected_cx );
        ok( cs->cy == expected_cy || broken(cs->cy == (short)expected_cy),
            "wrong y size %d/%d\n", cs->cy, expected_cy );
        ok( (rect.right - rect.left == expected_rect.right - expected_rect.left &&
             rect.bottom - rect.top == expected_rect.bottom - expected_rect.top) ||
            broken( rect.right - rect.left == broken_rect.right - broken_rect.left &&
                    rect.bottom - rect.top == broken_rect.bottom - broken_rect.top) ||
            broken( rect.right - rect.left == (short)broken_rect.right - (short)broken_rect.left &&
                    rect.bottom - rect.top == (short)broken_rect.bottom - (short)broken_rect.top),
            "wrong rect %d,%d-%d,%d / %d,%d-%d,%d\n",
            rect.left, rect.top, rect.right, rect.bottom,
            expected_rect.left, expected_rect.top, expected_rect.right, expected_rect.bottom );
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    case WM_NCCALCSIZE:
    {
        RECT rect, *r = (RECT *)lp;
        GetWindowRect( hwnd, &rect );
        ok( !memcmp( &rect, r, sizeof(rect) ),
            "passed rect %d,%d-%d,%d doesn't match window rect %d,%d-%d,%d\n",
            r->left, r->top, r->right, r->bottom, rect.left, rect.top, rect.right, rect.bottom );
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
}

static void test_CreateWindow(void)
{
    WNDCLASS cls;
    HWND hwnd, parent;
    HMENU hmenu;
    RECT rc, rc_minmax;
    MINMAXINFO minmax;

#define expect_menu(window, menu) \
    SetLastError(0xdeadbeef); \
    ok(GetMenu(window) == (HMENU)menu, "GetMenu error %d\n", GetLastError())

#define expect_style(window, style)\
    ok(GetWindowLong(window, GWL_STYLE) == (style), "expected style %x != %x\n", (LONG)(style), GetWindowLong(window, GWL_STYLE))

#define expect_ex_style(window, ex_style)\
    ok(GetWindowLong(window, GWL_EXSTYLE) == (ex_style), "expected ex_style %x != %x\n", (LONG)(ex_style), GetWindowLong(window, GWL_EXSTYLE))

#define expect_gle_broken_9x(gle)\
    ok(GetLastError() == gle ||\
       broken(GetLastError() == 0xdeadbeef),\
       "IsMenu set error %d\n", GetLastError())

    hmenu = CreateMenu();
    assert(hmenu != 0);
    parent = GetDesktopWindow();
    assert(parent != 0);

    SetLastError(0xdeadbeef);
    ok(IsMenu(hmenu), "IsMenu error %d\n", GetLastError());

    /* WS_CHILD */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    /* WS_POPUP */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    /* WS_CHILD | WS_POPUP */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);

    /* test child window sizing */
    cls.style = 0;
    cls.lpfnWndProc = minmax_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandle(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MinMax_WndClass";
    RegisterClass(&cls);

    SetLastError(0xdeadbeef);
    parent = CreateWindowEx(0, "MinMax_WndClass", NULL, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                           0, 0, 100, 100, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(parent, 0);
    expect_style(parent, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPSIBLINGS);
    expect_ex_style(parent, WS_EX_WINDOWEDGE);

    memset(&minmax, 0, sizeof(minmax));
    SendMessage(parent, WM_GETMINMAXINFO, 0, (LPARAM)&minmax);
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxSize.x, minmax.ptMaxSize.y);
    ok(IsRectEmpty(&rc_minmax), "ptMaxSize is not empty\n");
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y);
    ok(IsRectEmpty(&rc_minmax), "ptMaxTrackSize is not empty\n");

    GetWindowRect(parent, &rc);
    ok(!IsRectEmpty(&rc), "parent window rect is empty\n");
    GetClientRect(parent, &rc);
    ok(!IsRectEmpty(&rc), "parent client rect is empty\n");

    InflateRect(&rc, 200, 200);
    trace("creating child with rect (%d,%d-%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowEx(0, "MinMax_WndClass", NULL, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                          rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                          parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);

    memset(&minmax, 0, sizeof(minmax));
    SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minmax);
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y);

    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);
    ok(EqualRect(&rc, &rc_minmax), "rects don't match: (%d,%d-%d,%d) and (%d,%d-%d,%d)\n",
       rc.left, rc.top, rc.right, rc.bottom,
       rc_minmax.left, rc_minmax.top, rc_minmax.right, rc_minmax.bottom);
    DestroyWindow(hwnd);

    cls.lpfnWndProc = winsizes_wnd_proc;
    cls.lpszClassName = "Sizes_WndClass";
    RegisterClass(&cls);

    expected_cx = expected_cy = 200000;
    SetRect( &expected_rect, 0, 0, 200000, 200000 );
    broken_rect = expected_rect;
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 300000, 300000, 200000, 200000, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 200000 || broken(rc.right == (short)200000), "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 200000 || broken(rc.bottom == (short)200000), "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -10;
    SetRect( &expected_rect, 0, 0, 0, 0 );
    SetRect( &broken_rect, 0, 0, -10, -10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -20, -20, -10, -10, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -200000;
    SetRect( &expected_rect, 0, 0, 0, 0 );
    SetRect( &broken_rect, 0, 0, -200000, -200000 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -300000, -300000, -200000, -200000, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    /* we need a parent at 0,0 so that child coordinates match */
    DestroyWindow(parent);
    parent = CreateWindowEx(0, "MinMax_WndClass", NULL, WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowEx error %d\n", GetLastError());

    expected_cx = 100;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 10, 10, 110, 0x7fffffff );
    SetRect( &broken_rect, 10, 10, 110, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 10, 10, 100, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 100, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || broken(rc.bottom == 0), "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = 0x7fffffff;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 20, 10, 0x7fffffff, 0x7fffffff );
    SetRect( &broken_rect, 20, 10, 0x7fffffffU + 20, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 20, 10, 0x7fffffff, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0x7fffffff - 20 || broken(rc.right == 0), "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || broken(rc.bottom == 0), "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    /* top level window */
    expected_cx = expected_cy = 200000;
    SetRect( &expected_rect, 0, 0, GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CYMAXTRACK) );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_OVERLAPPEDWINDOW, 300000, 300000, 200000, 200000, 0, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right <= expected_cx, "invalid rect right %u\n", rc.right );
    ok( rc.bottom <= expected_cy, "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    DestroyWindow(parent);

    UnregisterClass("MinMax_WndClass", GetModuleHandle(0));
    UnregisterClass("Sizes_WndClass", GetModuleHandle(0));

#undef expect_gle_broken_9x
#undef expect_menu
#undef expect_style
#undef expect_ex_style
}

/* function that remembers whether the system the test is running on sets the
 * last error for user32 functions to make the tests stricter */
static int check_error(DWORD actual, DWORD expected)
{
    static int sets_last_error = -1;
    if (sets_last_error == -1)
        sets_last_error = (actual != 0xdeadbeef);
    return (!sets_last_error && (actual == 0xdeadbeef)) || (actual == expected);
}

static void test_SetWindowLong(void)
{
    LONG_PTR retval;
    WNDPROC old_window_procW;

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtr(NULL, GWLP_WNDPROC, 0);
    ok(!retval, "SetWindowLongPtr on invalid window handle should have returned 0 instead of 0x%lx\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_WINDOW_HANDLE),
        "SetWindowLongPtr should have set error to ERROR_INVALID_WINDOW_HANDLE instad of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtr(hwndMain, 0xdeadbeef, 0);
    ok(!retval, "SetWindowLongPtr on invalid index should have returned 0 instead of 0x%lx\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_INDEX),
        "SetWindowLongPtr should have set error to ERROR_INVALID_INDEX instad of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtr(hwndMain, GWLP_WNDPROC, 0);
    ok((WNDPROC)retval == main_window_procA || broken(!retval), /* win9x */
        "SetWindowLongPtr on invalid window proc should have returned address of main_window_procA instead of 0x%lx\n", retval);
    ok(GetLastError() == 0xdeadbeef, "SetWindowLongPtr shouldn't have set the last error, instead of setting it to %d\n", GetLastError());
    retval = GetWindowLongPtr(hwndMain, GWLP_WNDPROC);
    ok((WNDPROC)retval == main_window_procA,
        "SetWindowLongPtr on invalid window proc shouldn't have changed the value returned by GetWindowLongPtr, instead of changing it to 0x%lx\n", retval);
    ok(!IsWindowUnicode(hwndMain), "hwndMain shouldn't be Unicode\n");

    old_window_procW = (WNDPROC)GetWindowLongPtrW(hwndMain, GWLP_WNDPROC);
    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrW(hwndMain, GWLP_WNDPROC, 0);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(GetLastError() == 0xdeadbeef, "SetWindowLongPtr shouldn't have set the last error, instead of setting it to %d\n", GetLastError());
        ok(retval != 0, "SetWindowLongPtr error %d\n", GetLastError());
        ok((WNDPROC)retval == old_window_procW,
            "SetWindowLongPtr on invalid window proc shouldn't have changed the value returned by GetWindowLongPtr, instead of changing it to 0x%lx\n", retval);
        ok(IsWindowUnicode(hwndMain), "hwndMain should now be Unicode\n");

        /* set it back to ANSI */
        SetWindowLongPtr(hwndMain, GWLP_WNDPROC, 0);
    }
}

static void test_ShowWindow(void)
{
    HWND hwnd;
    DWORD style;
    RECT rcMain, rc;
    LPARAM ret;

    SetRect(&rcMain, 120, 120, 210, 210);

    hwnd = CreateWindowEx(0, "MainWindowClass", NULL,
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_POPUP,
                          rcMain.left, rcMain.top,
                          rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                          0, 0, 0, NULL);
    assert(hwnd);

    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(!(style & WS_VISIBLE), "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(!EqualRect(&rcMain, &rc), "rects shouldn't match\n");

    ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = EnableWindow(hwnd, FALSE);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");

    ret = DefWindowProc(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = DefWindowProc(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(!EqualRect(&rcMain, &rc), "rects shouldn't match\n");

    ret = DefWindowProc(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(!EqualRect(&rcMain, &rc), "rects shouldn't match\n");

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLong(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected (%d,%d)-(%d,%d), got (%d,%d)-(%d,%d)\n",
       rcMain.left, rcMain.top, rcMain.right, rcMain.bottom,
       rc.left, rc.top, rc.right, rc.bottom);

    ret = DefWindowProc(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    ok(IsWindow(hwnd), "window should exist\n");

    ret = EnableWindow(hwnd, TRUE);
    ok(ret, "not expected ret: %lu\n", ret);

    ret = DefWindowProc(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    ok(!IsWindow(hwnd), "window should not exist\n");
}

static void test_gettext(void)
{
    WNDCLASS cls;
    LPCSTR clsname = "gettexttest";
    HWND hwnd;
    LRESULT r;

    memset( &cls, 0, sizeof cls );
    cls.lpfnWndProc = DefWindowProc;
    cls.lpszClassName = clsname;
    cls.hInstance = GetModuleHandle(NULL);

    if (!RegisterClass( &cls )) return;

    hwnd = CreateWindow( clsname, "test text", WS_OVERLAPPED, 0, 0, 10, 10, 0, NULL, NULL, NULL);
    ok( hwnd != NULL, "window was null\n");

    r = SendMessage( hwnd, WM_GETTEXT, 0x10, 0x1000);
    ok( r == 0, "settext should return zero\n");

    r = SendMessage( hwnd, WM_GETTEXT, 0x10000, 0);
    ok( r == 0, "settext should return zero (%ld)\n", r);

    r = SendMessage( hwnd, WM_GETTEXT, 0xff000000, 0x1000);
    ok( r == 0, "settext should return zero (%ld)\n", r);

    r = SendMessage( hwnd, WM_GETTEXT, 0x1000, 0xff000000);
    ok( r == 0, "settext should return zero (%ld)\n", r);

    DestroyWindow(hwnd);
    UnregisterClass( clsname, NULL );
}


static void test_GetUpdateRect(void)
{
    MSG msg;
    BOOL ret, parent_wm_paint, grandparent_wm_paint;
    RECT rc1, rc2;
    HWND hgrandparent, hparent, hchild;
    WNDCLASSA cls;
    static const char classNameA[] = "GetUpdateRectClass";

    hgrandparent = CreateWindowA("static", "grandparent", WS_OVERLAPPEDWINDOW,
                                 0, 0, 100, 100, NULL, NULL, 0, NULL);

    hparent = CreateWindowA("static", "parent", WS_CHILD|WS_VISIBLE,
                            0, 0, 100, 100, hgrandparent, NULL, 0, NULL);

    hchild = CreateWindowA("static", "child", WS_CHILD|WS_VISIBLE,
                            10, 10, 30, 30, hparent, NULL, 0, NULL);

    ShowWindow(hgrandparent, SW_SHOW);
    UpdateWindow(hgrandparent);
    flush_events( TRUE );

    ShowWindow(hchild, SW_HIDE);
    SetRect(&rc2, 0, 0, 0, 0);
    ret = GetUpdateRect(hgrandparent, &rc1, FALSE);
    ok(!ret, "GetUpdateRect returned not empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match (%d,%d,%d,%d) / (%d,%d,%d,%d)\n",
       rc1.left, rc1.top, rc1.right, rc1.bottom,
       rc2.left, rc2.top, rc2.right, rc2.bottom);

    SetRect(&rc2, 10, 10, 40, 40);
    ret = GetUpdateRect(hparent, &rc1, FALSE);
    ok(ret, "GetUpdateRect returned empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match (%d,%d,%d,%d) / (%d,%d,%d,%d)\n",
            rc1.left, rc1.top, rc1.right, rc1.bottom,
            rc2.left, rc2.top, rc2.right, rc2.bottom);

    parent_wm_paint = FALSE;
    grandparent_wm_paint = FALSE;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_PAINT)
        {
            if (msg.hwnd == hgrandparent) grandparent_wm_paint = TRUE;
            if (msg.hwnd == hparent) parent_wm_paint = TRUE;
        }
        DispatchMessage(&msg);
    }
    ok(parent_wm_paint, "WM_PAINT should have been received in parent\n");
    ok(!grandparent_wm_paint, "WM_PAINT should NOT have been received in grandparent\n");

    DestroyWindow(hgrandparent);

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = classNameA;

    if(!RegisterClassA(&cls)) {
       trace("Register failed %d\n", GetLastError());
       return;
    }

    hgrandparent = CreateWindowA(classNameA, "grandparent", WS_OVERLAPPEDWINDOW,
                                 0, 0, 100, 100, NULL, NULL, 0, NULL);

    hparent = CreateWindowA(classNameA, "parent", WS_CHILD|WS_VISIBLE,
                            0, 0, 100, 100, hgrandparent, NULL, 0, NULL);

    hchild = CreateWindowA(classNameA, "child", WS_CHILD|WS_VISIBLE,
                            10, 10, 30, 30, hparent, NULL, 0, NULL);

    ShowWindow(hgrandparent, SW_SHOW);
    UpdateWindow(hgrandparent);
    flush_events( TRUE );

    ret = GetUpdateRect(hgrandparent, &rc1, FALSE);
    ok(!ret, "GetUpdateRect returned not empty region\n");

    ShowWindow(hchild, SW_HIDE);

    SetRect(&rc2, 0, 0, 0, 0);
    ret = GetUpdateRect(hgrandparent, &rc1, FALSE);
    ok(!ret, "GetUpdateRect returned not empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match (%d,%d,%d,%d) / (%d,%d,%d,%d)\n",
       rc1.left, rc1.top, rc1.right, rc1.bottom,
       rc2.left, rc2.top, rc2.right, rc2.bottom);

    SetRect(&rc2, 10, 10, 40, 40);
    ret = GetUpdateRect(hparent, &rc1, FALSE);
    ok(ret, "GetUpdateRect returned empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match (%d,%d,%d,%d) / (%d,%d,%d,%d)\n",
            rc1.left, rc1.top, rc1.right, rc1.bottom,
            rc2.left, rc2.top, rc2.right, rc2.bottom);

    parent_wm_paint = FALSE;
    grandparent_wm_paint = FALSE;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_PAINT)
        {
            if (msg.hwnd == hgrandparent) grandparent_wm_paint = TRUE;
            if (msg.hwnd == hparent) parent_wm_paint = TRUE;
        }
        DispatchMessage(&msg);
    }
    ok(parent_wm_paint, "WM_PAINT should have been received in parent\n");
    ok(!grandparent_wm_paint, "WM_PAINT should NOT have been received in grandparent\n");

    DestroyWindow(hgrandparent);
}


static LRESULT CALLBACK TestExposedRegion_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        RECT updateRect;
        DWORD waitResult;
        HWND win;
        const int waitTime = 2000;

        BeginPaint(hwnd, &ps);

        /* create and destroy window to create an exposed region on this window */
        win = CreateWindowA("static", "win", WS_VISIBLE,
                             10,10,50,50, NULL, NULL, 0, NULL);
        DestroyWindow(win);

        waitResult = MsgWaitForMultipleObjects( 0, NULL, FALSE, waitTime, QS_PAINT );

        ValidateRect(hwnd, NULL);
        EndPaint(hwnd, &ps);

        if(waitResult != WAIT_TIMEOUT)
        {
            GetUpdateRect(hwnd, &updateRect, FALSE);
            ok(IsRectEmpty(&updateRect), "Exposed rect should be empty\n");
        }

        return 1;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void test_Expose(void)
{
    ATOM atom;
    WNDCLASSA cls;
    HWND mw;
    memset(&cls, 0, sizeof(WNDCLASSA));
    cls.lpfnWndProc = TestExposedRegion_WndProc;
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszClassName = "TestExposeClass";
    atom = RegisterClassA(&cls);

    mw = CreateWindowA("TestExposeClass", "MainWindow", WS_VISIBLE|WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 100, NULL, NULL, 0, NULL);

    UpdateWindow(mw);
    DestroyWindow(mw);
}

static void test_GetWindowModuleFileName(void)
{
    HWND hwnd;
    HINSTANCE hinst;
    UINT ret1, ret2;
    char buf1[MAX_PATH], buf2[MAX_PATH];

    if (!pGetWindowModuleFileNameA)
    {
        skip("GetWindowModuleFileNameA is not available\n");
        return;
    }

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0,0,0,0, 0, 0, 0, NULL);
    assert(hwnd);

    hinst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    ok(hinst == 0 || broken(hinst == GetModuleHandle(0)), /* win9x */ "expected 0, got %p\n", hinst);

    buf1[0] = 0;
    SetLastError(0xdeadbeef);
    ret1 = GetModuleFileName(hinst, buf1, sizeof(buf1));
    ok(ret1, "GetModuleFileName error %u\n", GetLastError());

    buf2[0] = 0;
    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, sizeof(buf2));
    ok(ret2, "GetWindowModuleFileNameA error %u\n", GetLastError());

    ok(ret1 == ret2 || broken(ret2 == ret1 + 1), /* win98 */ "%u != %u\n", ret1, ret2);
    ok(!strcmp(buf1, buf2), "%s != %s\n", buf1, buf2);

    hinst = GetModuleHandle(0);

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileName(hinst, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2 || broken(ret2 == ret1 - 3), /* win98 */
       "expected %u, got %u\n", ret1 - 2, ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileName(hinst, buf2, 0);
    ok(!ret2, "GetModuleFileName should return 0\n");
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2 || broken(ret2 == ret1 - 3), /* win98 */
       "expected %u, got %u\n", ret1 - 2, ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, 0);
    ok(!ret2, "expected 0, got %u\n", ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    DestroyWindow(hwnd);

    buf2[0] = 0;
    hwnd = (HWND)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret1 = pGetWindowModuleFileNameA(hwnd, buf1, sizeof(buf1));
    ok(!ret1, "expected 0, got %u\n", ret1);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || broken(GetLastError() == 0xdeadbeef), /* win9x */
       "expected ERROR_INVALID_WINDOW_HANDLE, got %u\n", GetLastError());

    hwnd = FindWindow("Shell_TrayWnd", NULL);
    ok(IsWindow(hwnd), "got invalid tray window %p\n", hwnd);
    SetLastError(0xdeadbeef);
    ret1 = pGetWindowModuleFileNameA(hwnd, buf1, sizeof(buf1));
    ok(!ret1 || broken(ret1), /* win98 */ "expected 0, got %u\n", ret1);

    if (!ret1)  /* inter-process GetWindowModuleFileName works on win9x, so don't test the desktop there */
    {
        ret1 = GetModuleFileName(0, buf1, sizeof(buf1));
        hwnd = GetDesktopWindow();
        ok(IsWindow(hwnd), "got invalid desktop window %p\n", hwnd);
        SetLastError(0xdeadbeef);
        ret2 = pGetWindowModuleFileNameA(hwnd, buf2, sizeof(buf2));
        ok(!ret2 || ret1 == ret2 /* vista */, "expected 0 or %u, got %u %s\n", ret1, ret2, buf2);
    }
}

static void test_hwnd_message(void)
{
    static const WCHAR mainwindowclassW[] = {'M','a','i','n','W','i','n','d','o','w','C','l','a','s','s',0};
    static const WCHAR message_windowW[] = {'m','e','s','s','a','g','e',' ','w','i','n','d','o','w',0};

    HWND parent = 0, hwnd, found;
    RECT rect;

    /* HWND_MESSAGE is not supported below w2k, but win9x return != 0
       on CreateWindowExA and crash later in the test.
       Use UNICODE here to fail on win9x */
    hwnd = CreateWindowExW(0, mainwindowclassW, message_windowW, WS_CAPTION | WS_VISIBLE,
                           100, 100, 200, 200, HWND_MESSAGE, 0, 0, NULL);
    if (!hwnd)
    {
        win_skip("CreateWindowExW with parent HWND_MESSAGE failed\n");
        return;
    }

    ok( !GetParent(hwnd), "GetParent should return 0 for message only windows\n" );
    if (pGetAncestor)
    {
        char buffer[100];
        HWND root, desktop = GetDesktopWindow();

        parent = pGetAncestor(hwnd, GA_PARENT);
        ok(parent != 0, "GetAncestor(GA_PARENT) should not return 0 for message windows\n");
        ok(parent != desktop, "GetAncestor(GA_PARENT) should not return desktop for message windows\n");
        root = pGetAncestor(hwnd, GA_ROOT);
        ok(root == hwnd, "GetAncestor(GA_ROOT) should return hwnd for message windows\n");
        ok( !pGetAncestor(parent, GA_PARENT), "parent shouldn't have parent %p\n",
            pGetAncestor(parent, GA_PARENT) );
        trace("parent %p root %p desktop %p\n", parent, root, desktop);
        if (!GetClassNameA( parent, buffer, sizeof(buffer) )) buffer[0] = 0;
        ok( !lstrcmpi( buffer, "Message" ), "wrong parent class '%s'\n", buffer );
        GetWindowRect( parent, &rect );
        ok( rect.left == 0 && rect.right == 100 && rect.top == 0 && rect.bottom == 100,
            "wrong parent rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    }
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 100 && rect.right == 300 && rect.top == 100 && rect.bottom == 300,
        "wrong window rect %d,%d-%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    /* test FindWindow behavior */

    found = FindWindowExA( 0, 0, 0, "message window" );
    ok( found == hwnd, "didn't find message window %p/%p\n", found, hwnd );
    SetLastError(0xdeadbeef);
    found = FindWindowExA( GetDesktopWindow(), 0, 0, "message window" );
    ok( found == 0, "found message window %p/%p\n", found, hwnd );
    ok( GetLastError() == 0xdeadbeef, "expected deadbeef, got %d\n", GetLastError() );
    if (parent)
    {
        found = FindWindowExA( parent, 0, 0, "message window" );
        ok( found == hwnd, "didn't find message window %p/%p\n", found, hwnd );
    }

    /* test IsChild behavior */

    if (parent) ok( !IsChild( parent, hwnd ), "HWND_MESSAGE is child of top window\n" );

    /* test IsWindowVisible behavior */

    ok( !IsWindowVisible( hwnd ), "HWND_MESSAGE window is visible\n" );
    if (parent) ok( !IsWindowVisible( parent ), "HWND_MESSAGE parent is visible\n" );

    DestroyWindow(hwnd);
}

static void test_layered_window(void)
{
    HWND hwnd;
    COLORREF key = 0;
    BYTE alpha = 0;
    DWORD flags = 0;
    BOOL ret;

    if (!pGetLayeredWindowAttributes || !pSetLayeredWindowAttributes)
    {
        win_skip( "layered windows not supported\n" );
        return;
    }
    hwnd = CreateWindowExA(0, "MainWindowClass", "message window", WS_CAPTION,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    assert( hwnd );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on non-layered window\n" );
    ret = pSetLayeredWindowAttributes( hwnd, 0, 0, LWA_ALPHA );
    ok( !ret, "SetLayeredWindowAttributes should fail on non-layered window\n" );
    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on layered but not initialized window\n" );
    ret = pSetLayeredWindowAttributes( hwnd, 0x123456, 44, LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x123456, "wrong color key %x\n", key );
    ok( alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == LWA_ALPHA, "wrong flags %x\n", flags );

    /* clearing WS_EX_LAYERED resets attributes */
    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on no longer layered window\n" );
    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on layered but not initialized window\n" );
    ret = pSetLayeredWindowAttributes( hwnd, 0x654321, 22, LWA_COLORKEY | LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x654321, "wrong color key %x\n", key );
    ok( alpha == 22, "wrong alpha %u\n", alpha );
    ok( flags == (LWA_COLORKEY | LWA_ALPHA), "wrong flags %x\n", flags );

    ret = pSetLayeredWindowAttributes( hwnd, 0x888888, 33, LWA_COLORKEY );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x888888, "wrong color key %x\n", key );
    /* alpha not changed on vista if LWA_ALPHA is not set */
    ok( alpha == 22 || alpha == 33, "wrong alpha %u\n", alpha );
    ok( flags == LWA_COLORKEY, "wrong flags %x\n", flags );

    /* color key always changed */
    ret = pSetLayeredWindowAttributes( hwnd, 0x999999, 44, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x999999, "wrong color key %x\n", key );
    ok( alpha == 22 || alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %x\n", flags );

    /* default alpha is 0 */
    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetWindowLong( hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pSetLayeredWindowAttributes( hwnd, 0x222222, 55, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x222222, "wrong color key %x\n", key );
    ok( alpha == 0 || alpha == 55, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %x\n", flags );

    DestroyWindow( hwnd );
}

static MONITORINFO mi;

static LRESULT CALLBACK fullscreen_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
            trace("WM_NCCREATE: rect %d,%d-%d,%d\n", cs->x, cs->y, cs->cx, cs->cy);
            ok(cs->x == mi.rcMonitor.left && cs->y == mi.rcMonitor.top &&
               cs->cx == mi.rcMonitor.right && cs->cy == mi.rcMonitor.bottom,
               "expected %d,%d-%d,%d, got %d,%d-%d,%d\n",
               mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
               cs->x, cs->y, cs->cx, cs->cy);
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minmax = (MINMAXINFO *)lp;
            dump_minmax_info(minmax);
            ok(minmax->ptMaxPosition.x <= mi.rcMonitor.left, "%d <= %d\n", minmax->ptMaxPosition.x, mi.rcMonitor.left);
            ok(minmax->ptMaxPosition.y <= mi.rcMonitor.top, "%d <= %d\n", minmax->ptMaxPosition.y, mi.rcMonitor.top);
            ok(minmax->ptMaxSize.x >= mi.rcMonitor.right, "%d >= %d\n", minmax->ptMaxSize.x, mi.rcMonitor.right);
            ok(minmax->ptMaxSize.y >= mi.rcMonitor.bottom, "%d >= %d\n", minmax->ptMaxSize.y, mi.rcMonitor.bottom);
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static void test_fullscreen(void)
{
    static const DWORD t_style[] = {
        WS_OVERLAPPED, WS_POPUP, WS_CHILD, WS_THICKFRAME, WS_DLGFRAME
    };
    static const DWORD t_ex_style[] = {
        0, WS_EX_APPWINDOW, WS_EX_TOOLWINDOW
    };
    WNDCLASS cls;
    HWND hwnd;
    int i, j;
    POINT pt;
    RECT rc;
    HMONITOR hmon;
    LRESULT ret;

    if (!pGetMonitorInfoA || !pMonitorFromPoint)
    {
        win_skip("GetMonitorInfoA or MonitorFromPoint are not available on this platform\n");
        return;
    }

    pt.x = pt.y = 0;
    SetLastError(0xdeadbeef);
    hmon = pMonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    ok(hmon != 0, "MonitorFromPoint error %u\n", GetLastError());

    mi.cbSize = sizeof(mi);
    SetLastError(0xdeadbeef);
    ret = pGetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo error %u\n", GetLastError());
    trace("monitor (%d,%d-%d,%d), work (%d,%d-%d,%d)\n",
        mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
        mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);

    cls.style = 0;
    cls.lpfnWndProc = fullscreen_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandle(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "fullscreen_class";
    RegisterClass(&cls);

    for (i = 0; i < sizeof(t_style)/sizeof(t_style[0]); i++)
    {
        DWORD style, ex_style;

        /* avoid a WM interaction */
        assert(!(t_style[i] & WS_VISIBLE));

        for (j = 0; j < sizeof(t_ex_style)/sizeof(t_ex_style[0]); j++)
        {
            int fixup;

            style = t_style[i];
            ex_style = t_ex_style[j];

            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);

            style = t_style[i] | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            /* FIXME: this doesn't work correctly in Wine for child windows yet */
            if (style & WS_CHILD)
            todo_wine
            ok(rc.left >= mi.rcWork.left && rc.top <= mi.rcWork.top &&
               rc.right <= mi.rcWork.right && rc.bottom <= mi.rcWork.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            else
            ok(rc.left >= mi.rcWork.left && rc.top <= mi.rcWork.top &&
               rc.right <= mi.rcWork.right && rc.bottom <= mi.rcWork.bottom,
               "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandle(0), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            if (style & (WS_CHILD | WS_POPUP))
                ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
                   rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
                   "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            else
                ok(rc.left >= mi.rcWork.left && rc.top <= mi.rcWork.top &&
                   rc.right <= mi.rcWork.right && rc.bottom <= mi.rcWork.bottom,
                   "%#x/%#x: window rect %d,%d-%d,%d\n", ex_style, style, rc.left, rc.top, rc.right, rc.bottom);
            DestroyWindow(hwnd);
        }
    }

    UnregisterClass("fullscreen_class", GetModuleHandle(0));
}

static BOOL test_thick_child_got_minmax;

static int getExpectedThickChildInc(void)
{
    const int outer = 2;
    int resizeBorder = GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME);
    return (outer + resizeBorder);
}

static LRESULT WINAPI test_thick_child_size_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    MINMAXINFO* minmax;
    int expectedMinTrack;
    int actualMinTrackX;
    int actualMinTrackY;
    int expectedMaxTrackX;
    int expectedMaxTrackY;
    int actualMaxTrackX;
    int actualMaxTrackY;
    int expectedMaxSizeX;
    int expectedMaxSizeY;
    int actualMaxSizeX;
    int actualMaxSizeY;
    int expectedPosX;
    int expectedPosY;
    int actualPosX;
    int actualPosY;
    RECT rect;
    switch (msg)
    {
        case WM_GETMINMAXINFO:
        {
            minmax = (MINMAXINFO *)lparam;
            trace("hwnd %p, WM_GETMINMAXINFO, %08lx, %08lx\n", hwnd, wparam, lparam);
            dump_minmax_info( minmax );

            test_thick_child_got_minmax = TRUE;

            expectedMinTrack = 2* getExpectedThickChildInc();
            actualMinTrackX =  minmax->ptMinTrackSize.x;
            actualMinTrackY =  minmax->ptMinTrackSize.y;
            todo_wine
                ok(actualMinTrackX == expectedMinTrack && actualMinTrackY == expectedMinTrack,
                    "expected minTrack %dx%d, actual minTrack %dx%d\n",
                    expectedMinTrack, expectedMinTrack, actualMinTrackX, actualMinTrackY);

            actualMaxTrackX = minmax->ptMaxTrackSize.x;
            actualMaxTrackY = minmax->ptMaxTrackSize.y;
            expectedMaxTrackX = GetSystemMetrics(SM_CXMAXTRACK);
            expectedMaxTrackY = GetSystemMetrics(SM_CYMAXTRACK);
            ok(actualMaxTrackX == expectedMaxTrackX &&  actualMaxTrackY == expectedMaxTrackY,
                "expected maxTrack %dx%d, actual maxTrack %dx%d\n",
                 expectedMaxTrackX, expectedMaxTrackY, actualMaxTrackX, actualMaxTrackY);

            GetClientRect(GetParent(hwnd), &rect);
            AdjustWindowRectEx(&rect, WS_CHILD | WS_VISIBLE | WS_THICKFRAME, FALSE, 0);
            expectedMaxSizeX = rect.right - rect.left;
            expectedMaxSizeY = rect.bottom - rect.top;
            actualMaxSizeX = minmax->ptMaxSize.x;
            actualMaxSizeY = minmax->ptMaxSize.y;
            ok(actualMaxSizeX == expectedMaxSizeX &&  actualMaxSizeY == expectedMaxSizeY,
                "expected maxTrack %dx%d, actual maxTrack %dx%d\n",
                expectedMaxSizeX, expectedMaxSizeY, actualMaxSizeX, actualMaxSizeY);

            expectedPosX = - getExpectedThickChildInc();
            expectedPosY = expectedPosX;
            actualPosX = minmax->ptMaxPosition.x;
            actualPosY = minmax->ptMaxPosition.y;
            todo_wine
                ok(actualPosX == expectedPosX && actualPosY == expectedPosY,
                    "expected maxPosition (%d/%d), actual maxPosition (%d/%d)\n",
                    expectedPosX, expectedPosY, actualPosX, actualPosY);
            break;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_thick_child_size(HWND parentWindow)
{
    BOOL success;
    RECT childRect;
    HWND childWindow;
    LONG childWidth;
    LONG childHeight;
    WNDCLASSA cls;
    LPCTSTR className = "THICK_CHILD_CLASS";
    LONG style = WS_CHILD | WS_VISIBLE | WS_THICKFRAME;
    LONG exStyle = 0;
    int expectedSize = 2*getExpectedThickChildInc();


    cls.style = 0;
    cls.lpfnWndProc = test_thick_child_size_winproc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = className;
    SetLastError(0xdeadbeef);
    ok(RegisterClassA(&cls),"RegisterClassA failed, error: %u\n", GetLastError());

    test_thick_child_got_minmax = FALSE;

    SetLastError(0xdeadbeef);
    childWindow = CreateWindowEx( exStyle, className, "", style,  0, 0, 0, 0, parentWindow, 0,  GetModuleHandleA(0),  NULL );
    ok(childWindow != NULL, "Failed to create child window, error: %u\n", GetLastError());

    ok(test_thick_child_got_minmax, "Got no WM_GETMINMAXINFO\n");

    SetLastError(0xdeadbeef);
    success = GetWindowRect(childWindow, &childRect);
    ok(success,"GetWindowRect call failed, error: %u\n", GetLastError());
    childWidth = childRect.right - childRect.left;
    childHeight = childRect.bottom - childRect.top;

    todo_wine
        ok( (childWidth == expectedSize) && (childHeight == expectedSize),
            "size of window with style WS_CHILD | WS_VISIBLE | WS_THICKFRAME  is wrong: expected size %dx%d != actual size %dx%d\n",
            expectedSize, expectedSize, childWidth, childHeight);


    SetLastError(0xdeadbeef);
    success = DestroyWindow(childWindow);
    ok(success,"DestroyWindow call failed, error: %u\n", GetLastError());
    ok(UnregisterClass(className, GetModuleHandleA(0)),"UnregisterClass call failed\n");
}


START_TEST(win)
{
    HMODULE user32 = GetModuleHandleA( "user32.dll" );
    pGetAncestor = (void *)GetProcAddress( user32, "GetAncestor" );
    pGetWindowInfo = (void *)GetProcAddress( user32, "GetWindowInfo" );
    pGetWindowModuleFileNameA = (void *)GetProcAddress( user32, "GetWindowModuleFileNameA" );
    pGetLayeredWindowAttributes = (void *)GetProcAddress( user32, "GetLayeredWindowAttributes" );
    pSetLayeredWindowAttributes = (void *)GetProcAddress( user32, "SetLayeredWindowAttributes" );
    pGetMonitorInfoA = (void *)GetProcAddress( user32,  "GetMonitorInfoA" );
    pMonitorFromPoint = (void *)GetProcAddress( user32,  "MonitorFromPoint" );

    if (!RegisterWindowClasses()) assert(0);

    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    assert(hhook);

    hwndMain = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window",
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_POPUP,
                               100, 100, 200, 200,
                               0, 0, GetModuleHandle(0), NULL);
    hwndMain2 = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window 2",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_POPUP,
                                100, 100, 200, 200,
                                0, 0, GetModuleHandle(0), NULL);
    assert( hwndMain );
    assert( hwndMain2 );

    our_pid = GetWindowThreadProcessId(hwndMain, NULL);

    /* Add the tests below this line */
    test_thick_child_size(hwndMain);
    test_fullscreen();
    test_hwnd_message();
    test_nonclient_area(hwndMain);
    test_params();
    test_GetWindowModuleFileName();
    test_capture_1();
    test_capture_2();
    test_capture_3(hwndMain, hwndMain2);

    test_CreateWindow();
    test_parent_owner();
    test_SetParent();

    test_mdi();
    test_icons();
    test_SetWindowPos(hwndMain);
    test_SetMenu(hwndMain);
    test_SetFocus(hwndMain);
    test_SetActiveWindow(hwndMain);

    test_children_zorder(hwndMain);
    test_popup_zorder(hwndMain2, hwndMain);
    test_keyboard_input(hwndMain);
    test_mouse_input(hwndMain);
    test_validatergn(hwndMain);
    test_nccalcscroll( hwndMain);
    test_scrollvalidate( hwndMain);
    test_scrolldc( hwndMain);
    test_scroll();
    test_IsWindowUnicode();
    test_vis_rgn(hwndMain);

    test_AdjustWindowRect();
    test_window_styles();
    test_redrawnow();
    test_csparentdc();
    test_SetWindowLong();
    test_ShowWindow();
    if (0) test_gettext(); /* crashes on NT4 */
    test_GetUpdateRect();
    test_Expose();
    test_layered_window();

    test_SetForegroundWindow(hwndMain);
    test_shell_window();

    /* add the tests above this line */
    UnhookWindowsHookEx(hhook);
}
