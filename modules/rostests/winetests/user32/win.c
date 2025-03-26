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

#include "precomp.h"

#ifndef SPI_GETDESKWALLPAPER
#define SPI_GETDESKWALLPAPER 0x0073
#endif

#ifndef WM_SYSTIMER
#define WM_SYSTIMER 0x0118
#endif

#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

void dump_region(HRGN hrgn);

static HWND (WINAPI *pGetAncestor)(HWND,UINT);
static BOOL (WINAPI *pGetWindowInfo)(HWND,WINDOWINFO*);
static UINT (WINAPI *pGetWindowModuleFileNameA)(HWND,LPSTR,UINT);
static BOOL (WINAPI *pGetLayeredWindowAttributes)(HWND,COLORREF*,BYTE*,DWORD*);
static BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
static BOOL (WINAPI *pUpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
static BOOL (WINAPI *pUpdateLayeredWindowIndirect)(HWND,const UPDATELAYEREDWINDOWINFO*);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
static HMONITOR (WINAPI *pMonitorFromPoint)(POINT,DWORD);
static int  (WINAPI *pGetWindowRgnBox)(HWND,LPRECT);
static BOOL (WINAPI *pGetGUIThreadInfo)(DWORD, GUITHREADINFO*);
static BOOL (WINAPI *pGetProcessDefaultLayout)( DWORD *layout );
static BOOL (WINAPI *pSetProcessDefaultLayout)( DWORD layout );
static BOOL (WINAPI *pFlashWindow)( HWND hwnd, BOOL bInvert );
static BOOL (WINAPI *pFlashWindowEx)( PFLASHWINFO pfwi );
static DWORD (WINAPI *pSetLayout)(HDC hdc, DWORD layout);
static DWORD (WINAPI *pGetLayout)(HDC hdc);
static BOOL (WINAPI *pMirrorRgn)(HWND hwnd, HRGN hrgn);

static BOOL test_lbuttondown_flag;
static DWORD num_gettext_msgs;
static DWORD num_settext_msgs;
static HWND hwndMessage;
static HWND hwndMain, hwndMain2;
static HHOOK hhook;
static BOOL app_activated, app_deactivated;

static const char* szAWRClass = "Winsize";
static HMENU hmenu;
static DWORD our_pid;

static BOOL is_win9x = FALSE;

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
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        if (remove_messages)
            while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
        min_timeout = 50;
    }
}

static BOOL wait_for_event(HANDLE event, int timeout)
{
    DWORD end_time = GetTickCount() + timeout;
    MSG msg;

    do {
        if(MsgWaitForMultipleObjects(1, &event, FALSE, timeout, QS_ALLINPUT) == WAIT_OBJECT_0)
            return TRUE;
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        timeout = end_time - GetTickCount();
    }while(timeout > 0);

    return FALSE;
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

#define check_wnd_state(a,b,c,d) check_wnd_state_(__FILE__,__LINE__,a,b,c,d)
static void check_wnd_state_(const char *file, int line,
                             HWND active, HWND foreground, HWND focus, HWND capture)
{
    ok_(file, line)(active == GetActiveWindow(), "GetActiveWindow() = %p\n", GetActiveWindow());
    /* only check foreground if it belongs to the current thread */
    /* foreground can be moved to a different app pretty much at any time */
    if (foreground && GetForegroundWindow() &&
        GetWindowThreadProcessId(GetForegroundWindow(), NULL) == GetCurrentThreadId())
        disable_success_count
        ok_(file, line)(foreground == GetForegroundWindow(), "GetForegroundWindow() = %p\n", GetForegroundWindow());
    ok_(file, line)(focus == GetFocus(), "GetFocus() = %p\n", GetFocus());
    ok_(file, line)(capture == GetCapture(), "GetCapture() = %p\n", GetCapture());
}

/* same as above but without capture test */
#define check_active_state(a,b,c) check_active_state_(__FILE__,__LINE__,a,b,c)
static void check_active_state_(const char *file, int line,
                                HWND active, HWND foreground, HWND focus)
{
    ok_(file, line)(active == GetActiveWindow(), "GetActiveWindow() = %p\n", GetActiveWindow());
    /* only check foreground if it belongs to the current thread */
    /* foreground can be moved to a different app pretty much at any time */
    if (foreground && GetForegroundWindow() &&
        GetWindowThreadProcessId(GetForegroundWindow(), NULL) == GetCurrentThreadId())
        disable_success_count
        ok_(file, line)(foreground == GetForegroundWindow(), "GetForegroundWindow() = %p\n", GetForegroundWindow());
    ok_(file, line)(focus == GetFocus(), "GetFocus() = %p\n", GetFocus());
}

static BOOL ignore_message( UINT message )
{
    /* these are always ignored */
    return (message >= 0xc000 ||
            message == WM_GETICON ||
            message == WM_GETOBJECT ||
            message == WM_TIMER ||
            message == WM_SYSTIMER ||
            message == WM_TIMECHANGE ||
            message == WM_DEVICECHANGE);
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
    if (!is_win9x)
    {
        ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)test );
        ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
        check_parents( test, desktop, 0, desktop, 0, test, desktop );
    }
    else
        win_skip("Test creates circular window tree under Win9x/WinMe\n" );

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

    if (!is_win9x)
    {
        ShowWindow( test, SW_SHOW );
        ret = SetParent( test, test );
        ok( ret == NULL, "SetParent return value %p expected %p\n", ret, NULL );
        ok( GetWindowLongA( test, GWL_STYLE ) & WS_VISIBLE, "window is not visible after SetParent\n" );
        check_parents( test, child, child, 0, 0, hwndMain, test );
    }
    else
        win_skip( "Test crashes on Win9x/WinMe\n" );
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

    /* Test that owner window takes into account WS_CHILD flag even if parent is set by SetParent. */
    owner = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, desktop );
    SetParent(owner, hwndMain);
    check_parents( owner, hwndMain, hwndMain, NULL, NULL, hwndMain, owner );
    test = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, owner );
    check_parents( test, desktop, owner, NULL, owner, test, test );
    DestroyWindow( owner );
    DestroyWindow( test );

    owner = create_tool_window( WS_VISIBLE | WS_CHILD, desktop );
    SetParent(owner, hwndMain);
    check_parents( owner, hwndMain, hwndMain, hwndMain, NULL, hwndMain, hwndMain );
    test = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, owner );
    check_parents( test, desktop, hwndMain, NULL, hwndMain, test, test );
    DestroyWindow( owner );
    DestroyWindow( test );

    owner = create_tool_window( WS_VISIBLE | WS_POPUP | WS_CHILD, desktop );
    SetParent(owner, hwndMain);
    check_parents( owner, hwndMain, hwndMain, NULL, NULL, hwndMain, owner );
    test = create_tool_window( WS_VISIBLE | WS_OVERLAPPEDWINDOW, owner );
    check_parents( test, desktop, owner, NULL, owner, test, test );
    DestroyWindow( owner );
    DestroyWindow( test );
}

static BOOL CALLBACK enum_proc( HWND hwnd, LPARAM lParam)
{
    (*(LPINT)lParam)++;
    if (*(LPINT)lParam > 2) return FALSE;
    return TRUE;
}
static DWORD CALLBACK enum_thread( void *arg )
{
    INT count;
    HWND hwnd[3];
    BOOL ret;
    MSG msg;

    if (pGetGUIThreadInfo)
    {
        GUITHREADINFO info;
        info.cbSize = sizeof(info);
        ret = pGetGUIThreadInfo( GetCurrentThreadId(), &info );
        ok( ret || broken(!ret), /* win9x */
            "GetGUIThreadInfo failed without message queue\n" );
        SetLastError( 0xdeadbeef );
        info.cbSize = sizeof(info) + 1;
        ret = pGetGUIThreadInfo( GetCurrentThreadId(), &info );
        ok( !ret, "GetGUIThreadInfo succeeded with wrong size\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER ||
            broken(GetLastError() == 0xdeadbeef), /* win9x */
            "wrong error %u\n", GetLastError() );
    }

    PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );  /* make sure we have a message queue */

    count = 0;
    ret = EnumThreadWindows( GetCurrentThreadId(), enum_proc, (LPARAM)&count );
    ok( ret, "EnumThreadWindows should have returned TRUE\n" );
    ok( count == 0, "count should be 0 got %d\n", count );

    hwnd[0] = CreateWindowExA(0, "ToolWindowClass", "Tool window 1", WS_POPUP,
                              0, 0, 100, 100, 0, 0, 0, NULL );
    count = 0;
    ret = EnumThreadWindows( GetCurrentThreadId(), enum_proc, (LPARAM)&count );
    ok( ret, "EnumThreadWindows should have returned TRUE\n" );
    if (count != 2)  /* Vista gives us two windows for the price of one */
    {
        ok( count == 1, "count should be 1 got %d\n", count );
        hwnd[2] = CreateWindowExA(0, "ToolWindowClass", "Tool window 2", WS_POPUP,
                                  0, 0, 100, 100, 0, 0, 0, NULL );
    }
    else hwnd[2] = 0;

    hwnd[1] = CreateWindowExA(0, "ToolWindowClass", "Tool window 3", WS_POPUP,
                              0, 0, 100, 100, 0, 0, 0, NULL );
    count = 0;
    ret = EnumThreadWindows( GetCurrentThreadId(), enum_proc, (LPARAM)&count );
    ok( !ret, "EnumThreadWindows should have returned FALSE\n" );
    ok( count == 3, "count should be 3 got %d\n", count );

    if (hwnd[2]) DestroyWindow(hwnd[2]);
    DestroyWindow(hwnd[1]);
    DestroyWindow(hwnd[0]);
    return 0;
}

/* test EnumThreadWindows in a separate thread */
static void test_enum_thread_windows(void)
{
    DWORD id;
    HANDLE handle = CreateThread( NULL, 0, enum_thread, 0, 0, &id );
    ok( !WaitForSingleObject( handle, 10000 ), "wait failed\n" );
    CloseHandle( handle );
}

static struct wm_gettext_override_data
{
    BOOL   enabled; /* when 1 bypasses default procedure */
    char  *buff;    /* expected text buffer pointer */
    WCHAR *buffW;   /* same, for W test */
} g_wm_gettext_override;

static LRESULT WINAPI main_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
	case WM_GETMINMAXINFO:
	{
	    SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0x20031021);
	    break;
	}
	case WM_WINDOWPOSCHANGING:
	{
	    WINDOWPOS *winpos = (WINDOWPOS *)lparam;
	    if (!(winpos->flags & SWP_NOMOVE))
	    {
		ok(winpos->x >= -32768 && winpos->x <= 32767, "bad winpos->x %d\n", winpos->x);
		ok(winpos->y >= -32768 && winpos->y <= 32767, "bad winpos->y %d\n", winpos->y);
	    }
	    /* Win9x does not fixup cx/xy for WM_WINDOWPOSCHANGING */
	    if (!(winpos->flags & SWP_NOSIZE) && !is_win9x)
	    {
		ok((winpos->cx >= 0 && winpos->cx <= 32767) ||
                   winpos->cx == 32768, /* win7 doesn't truncate */
                   "bad winpos->cx %d\n", winpos->cx);
		ok((winpos->cy >= 0 && winpos->cy <= 32767) ||
                   winpos->cy == 40000, /* win7 doesn't truncate */
                   "bad winpos->cy %d\n", winpos->cy);
	    }
	    break;
	}
	case WM_WINDOWPOSCHANGED:
    disable_success_count
	{
            RECT rc1, rc2;
	    WINDOWPOS *winpos = (WINDOWPOS *)lparam;
	    ok(winpos->x >= -32768 && winpos->x <= 32767, "bad winpos->x %d\n", winpos->x);
	    ok(winpos->y >= -32768 && winpos->y <= 32767, "bad winpos->y %d\n", winpos->y);

            ok((winpos->cx >= 0 && winpos->cx <= 32767) ||
               winpos->cx == 32768, /* win7 doesn't truncate */
               "bad winpos->cx %d\n", winpos->cx);
            ok((winpos->cy >= 0 && winpos->cy <= 32767) ||
               winpos->cy == 40000, /* win7 doesn't truncate */
               "bad winpos->cy %d\n", winpos->cy);

            GetWindowRect(hwnd, &rc1);
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
            ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
               wine_dbgstr_rect(&rc2));

            GetClientRect(hwnd, &rc2);
            DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
               wine_dbgstr_rect(&rc2));
            }
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongPtrA(hwnd, GWLP_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

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
        case WM_GETTEXT:
            num_gettext_msgs++;
            if (g_wm_gettext_override.enabled)
            {
                char *text = (char*)lparam;
                ok(g_wm_gettext_override.buff == text, "expected buffer %p, got %p\n", g_wm_gettext_override.buff, text);
                ok(*text == 0, "expected empty string buffer %x\n", *text);
                return 0;
            }
            break;
        case WM_SETTEXT:
            num_settext_msgs++;
            break;
        case WM_ACTIVATEAPP:
            if (wparam) app_activated = TRUE;
            else app_deactivated = TRUE;
            break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI main_window_procW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_GETTEXT:
            num_gettext_msgs++;
            if (g_wm_gettext_override.enabled)
            {
                WCHAR *text = (WCHAR*)lparam;
                ok(g_wm_gettext_override.buffW == text, "expected buffer %p, got %p\n", g_wm_gettext_override.buffW, text);
                ok(*text == 0, "expected empty string buffer %x\n", *text);
                return 0;
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
	    SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0x20031021);
	    break;
	}
	case WM_NCCREATE:
	{
	    BOOL got_getminmaxinfo = GetWindowLongPtrA(hwnd, GWLP_USERDATA) == 0x20031021;
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;

	    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
		ok(got_getminmaxinfo, "tool: WM_GETMINMAXINFO should have been received before WM_NCCREATE\n");
	    else
		ok(!got_getminmaxinfo, "tool: WM_GETMINMAXINFO should NOT have been received before WM_NCCREATE\n");
	    break;
	}
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static const WCHAR mainclassW[] = {'M','a','i','n','W','i','n','d','o','w','C','l','a','s','s','W',0};

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSW clsW;
    WNDCLASSA cls;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = main_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MainWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    clsW.style = CS_DBLCLKS;
    clsW.lpfnWndProc = main_window_procW;
    clsW.cbClsExtra = 0;
    clsW.cbWndExtra = 0;
    clsW.hInstance = GetModuleHandleA(0);
    clsW.hIcon = 0;
    clsW.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    clsW.hbrBackground = GetStockObject(WHITE_BRUSH);
    clsW.lpszMenuName = NULL;
    clsW.lpszClassName = mainclassW;

    if(!RegisterClassW(&clsW)) return FALSE;

    cls.style = 0;
    cls.lpfnWndProc = tool_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "ToolWindowClass";

    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

static void verify_window_info(const char *hook, HWND hwnd, const WINDOWINFO *info)
{
    RECT rcWindow, rcClient;
    DWORD status;

    ok(IsWindow(hwnd), "bad window handle %p in hook %s\n", hwnd, hook);

    GetWindowRect(hwnd, &rcWindow);
    ok(EqualRect(&rcWindow, &info->rcWindow), "wrong rcWindow for %p in hook %s\n", hwnd, hook);

    GetClientRect(hwnd, &rcClient);
    /* translate to screen coordinates */
    MapWindowPoints(hwnd, 0, (LPPOINT)&rcClient, 2);
    ok(EqualRect(&rcClient, &info->rcClient), "wrong rcClient for %p in hook %s\n", hwnd, hook);

    ok(info->dwStyle == (DWORD)GetWindowLongA(hwnd, GWL_STYLE),
       "wrong dwStyle: %08x != %08x for %p in hook %s\n",
       info->dwStyle, GetWindowLongA(hwnd, GWL_STYLE), hwnd, hook);
    /* Windows reports some undocumented exstyles in WINDOWINFO, but
     * doesn't return them in GetWindowLong(hwnd, GWL_EXSTYLE).
     */
    ok((info->dwExStyle & ~0xe0000800) == (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE),
       "wrong dwExStyle: %08x != %08x for %p in hook %s\n",
       info->dwExStyle, GetWindowLongA(hwnd, GWL_EXSTYLE), hwnd, hook);
    status = (GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0;
    if (GetForegroundWindow())
        ok(info->dwWindowStatus == status, "wrong dwWindowStatus: %04x != %04x active %p fg %p in hook %s\n",
           info->dwWindowStatus, status, GetActiveWindow(), GetForegroundWindow(), hook);
    else
        ok(1, "Just counting");

    /* win2k and XP return broken border info in GetWindowInfo most of
     * the time, so there is no point in testing it.
     */
if (0)
{
    UINT border;
    ok(info->cxWindowBorders == (unsigned)(rcClient.left - rcWindow.left),
       "wrong cxWindowBorders %d != %d\n", info->cxWindowBorders, rcClient.left - rcWindow.left);
    border = min(rcWindow.bottom - rcClient.bottom, rcClient.top - rcWindow.top);
    ok(info->cyWindowBorders == border,
       "wrong cyWindowBorders %d != %d\n", info->cyWindowBorders, border);
}
    ok(info->atomWindowType == GetClassLongA(hwnd, GCW_ATOM), "wrong atomWindowType for %p in hook %s\n",
       hwnd, hook);
    ok(info->wCreatorVersion == 0x0400 /* NT4, Win2000, XP, Win2003 */ ||
       info->wCreatorVersion == 0x0500 /* Vista */,
       "wrong wCreatorVersion %04x for %p in hook %s\n", info->wCreatorVersion, hwnd, hook);
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

/* reimplement it to check that the Wine algorithm gives the correct result */
static void wine_AdjustWindowRectEx( RECT *rect, LONG style, BOOL menu, LONG exStyle )
{
    int adjust;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) ==
        WS_EX_STATICEDGE)
    {
        adjust = 1; /* for the outer frame always present */
    }
    else
    {
        adjust = 0;
        if ((exStyle & WS_EX_DLGMODALFRAME) ||
            (style & (WS_THICKFRAME|WS_DLGFRAME))) adjust = 2; /* outer */
    }
    if (style & WS_THICKFRAME)
        adjust += GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME); /* The resize border */
    if ((style & (WS_BORDER|WS_DLGFRAME)) ||
        (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top -= GetSystemMetrics(SM_CYCAPTION);
    }
    if (menu) rect->top -= GetSystemMetrics(SM_CYMENU);

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));

    if (style & WS_VSCROLL)
    {
        if((exStyle & WS_EX_LEFTSCROLLBAR) != 0)
            rect->left  -= GetSystemMetrics(SM_CXVSCROLL);
        else
            rect->right += GetSystemMetrics(SM_CXVSCROLL);
    }
    if (style & WS_HSCROLL) rect->bottom += GetSystemMetrics(SM_CYHSCROLL);
}

static void test_nonclient_area(HWND hwnd)
{
    DWORD style, exstyle;
    RECT rc_window, rc_client, rc;
    BOOL menu;
    LRESULT ret;

    style = GetWindowLongA(hwnd, GWL_STYLE);
    exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    menu = !(style & WS_CHILD) && GetMenu(hwnd) != 0;

    GetWindowRect(hwnd, &rc_window);
    GetClientRect(hwnd, &rc_client);

    /* avoid some cases when things go wrong */
    if (IsRectEmpty(&rc_window) || IsRectEmpty(&rc_client) ||
	rc_window.right > 32768 || rc_window.bottom > 32768) return;

    rc = rc_client;
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc, 2);
    FixedAdjustWindowRectEx(&rc, style, menu, exstyle);

    ok(EqualRect(&rc, &rc_window),
       "window rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d, win=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_window), wine_dbgstr_rect(&rc));

    rc = rc_client;
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc, 2);
    wine_AdjustWindowRectEx(&rc, style, menu, exstyle);
    ok(EqualRect(&rc, &rc_window),
       "window rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d, win=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_window), wine_dbgstr_rect(&rc));


    rc = rc_window;
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    ok(EqualRect(&rc, &rc_client),
       "client rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d client=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_client), wine_dbgstr_rect(&rc));

    /* NULL rectangle shouldn't crash */
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, 0);
    ok(ret == 0, "NULL rectangle returned %ld instead of 0\n", ret);

    /* Win9x doesn't like WM_NCCALCSIZE with synthetic data and crashes */;
    if (is_win9x)
	return;

    /* and now test AdjustWindowRectEx and WM_NCCALCSIZE on synthetic data */
    SetRect(&rc_client, 0, 0, 250, 150);
    rc_window = rc_client;
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc_window, 2);
    FixedAdjustWindowRectEx(&rc_window, style, menu, exstyle);

    rc = rc_window;
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    ok(EqualRect(&rc, &rc_client),
       "synthetic rect does not match: style:exstyle=0x%08x:0x%08x, menu=%d, client=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_client), wine_dbgstr_rect(&rc));
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
    HWND hwnd = (HWND)wParam;

    switch (nCode)
    {
    case HCBT_CREATEWND:
	{
	    static const RECT rc_null;
	    RECT rc;
	    LONG style;
	    CBT_CREATEWNDA *createwnd = (CBT_CREATEWNDA *)lParam;
	    ok(createwnd->hwndInsertAfter == HWND_TOP, "hwndInsertAfter should be always HWND_TOP\n");

            if (pGetWindowInfo)
            {
                WINDOWINFO info;
                info.cbSize = sizeof(WINDOWINFO);
                ok(pGetWindowInfo(hwnd, &info), "GetWindowInfo should not fail\n");
                verify_window_info(code_name, hwnd, &info);
            }

	    /* WS_VISIBLE should be turned off yet */
	    style = createwnd->lpcs->style & ~WS_VISIBLE;
	    ok(style == GetWindowLongA(hwnd, GWL_STYLE),
		"style of hwnd and style in the CREATESTRUCT do not match: %08x != %08x\n",
		GetWindowLongA(hwnd, GWL_STYLE), style);

            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
	    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
	    {
		ok(GetParent(hwnd) == hwndMessage,
		   "wrong result from GetParent %p: message window %p\n",
		   GetParent(hwnd), hwndMessage);
	    }
	    else
		ok(!GetParent(hwnd), "GetParent should return 0 at this point\n");

	    ok(!GetWindow(hwnd, GW_OWNER), "GW_OWNER should be set to 0 at this point\n");
            }
            if (0)
            {
	    /* while NT assigns GW_HWNDFIRST/LAST some values at this point,
	     * Win9x still has them set to 0.
	     */
	    ok(GetWindow(hwnd, GW_HWNDFIRST) != 0, "GW_HWNDFIRST should not be set to 0 at this point\n");
	    ok(GetWindow(hwnd, GW_HWNDLAST) != 0, "GW_HWNDLAST should not be set to 0 at this point\n");
            }
	    ok(!GetWindow(hwnd, GW_HWNDPREV), "GW_HWNDPREV should be set to 0 at this point\n");
	    ok(!GetWindow(hwnd, GW_HWNDNEXT), "GW_HWNDNEXT should be set to 0 at this point\n");

            if (0)
            {
            /* Uncomment this once the test succeeds in all cases */
	    if (pGetAncestor)
	    {
		ok(pGetAncestor(hwnd, GA_PARENT) == hwndMessage, "GA_PARENT should be set to hwndMessage at this point\n");
		ok(pGetAncestor(hwnd, GA_ROOT) == hwnd,
		   "GA_ROOT is set to %p, expected %p\n", pGetAncestor(hwnd, GA_ROOT), hwnd);

		if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
		    ok(pGetAncestor(hwnd, GA_ROOTOWNER) == hwndMessage,
		       "GA_ROOTOWNER should be set to hwndMessage at this point\n");
		else
		    ok(pGetAncestor(hwnd, GA_ROOTOWNER) == hwnd,
		       "GA_ROOTOWNER is set to %p, expected %p\n", pGetAncestor(hwnd, GA_ROOTOWNER), hwnd);
            }

	    ok(GetWindowRect(hwnd, &rc), "GetWindowRect failed\n");
	    ok(EqualRect(&rc, &rc_null), "window rect should be set to 0 HCBT_CREATEWND\n");
	    ok(GetClientRect(hwnd, &rc), "GetClientRect failed\n");
	    ok(EqualRect(&rc, &rc_null), "client rect should be set to 0 on HCBT_CREATEWND\n");
            }
	    break;
	}
    case HCBT_MOVESIZE:
    case HCBT_MINMAX:
    case HCBT_ACTIVATE:
	if (pGetWindowInfo && IsWindow(hwnd))
	{
	    WINDOWINFO info;

	    /* Win98 actually does check the info.cbSize and doesn't allow
	     * it to be anything except sizeof(WINDOWINFO), while Win95, Win2k,
	     * WinXP do not check it at all.
	     */
	    info.cbSize = sizeof(WINDOWINFO);
	    ok(pGetWindowInfo(hwnd, &info), "GetWindowInfo should not fail\n");
	    verify_window_info(code_name, hwnd, &info);
	}
        break;
    /* window state is undefined */
    case HCBT_SETFOCUS:
    case HCBT_DESTROYWND:
        break;
    default:
        break;
    }

    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

static void test_shell_window(void)
{
    BOOL ret;
    DWORD error;
    HMODULE hinst, hUser32;
    BOOL (WINAPI*SetShellWindow)(HWND);
    HWND hwnd1, hwnd2, hwnd3, hwnd4, hwnd5;
    HWND shellWindow, nextWnd;

    if (is_win9x)
    {
        win_skip("Skipping shell window test on Win9x\n");
        return;
    }

    shellWindow = GetShellWindow();
    hinst = GetModuleHandleA(NULL);
    hUser32 = GetModuleHandleA("user32");

    SetShellWindow = (void *)GetProcAddress(hUser32, "SetShellWindow");

    trace("previous shell window: %p\n", shellWindow);

    if (shellWindow) {
        DWORD pid;
        HANDLE hProcess;

        GetWindowThreadProcessId(shellWindow, &pid);
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProcess)
        {
            skip( "cannot get access to shell process\n" );
            return;
        }

        SetLastError(0xdeadbeef);
        ret = DestroyWindow(shellWindow);
        error = GetLastError();

        ok(!ret, "DestroyWindow(shellWindow)\n");
        /* passes on Win XP, but not on Win98 */
        ok(error==ERROR_ACCESS_DENIED || error == 0xdeadbeef,
           "got %u after DestroyWindow(shellWindow)\n", error);

        /* close old shell instance */
        ret = TerminateProcess(hProcess, 0);
        ok(ret, "termination of previous shell process failed: GetLastError()=%d\n", GetLastError());
        WaitForSingleObject(hProcess, INFINITE);    /* wait for termination */
        CloseHandle(hProcess);
    }

    hwnd1 = CreateWindowExA(0, "#32770", "TEST1", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 100, 100, 300, 200, 0, 0, hinst, 0);
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

    SetWindowLongA(hwnd1, GWL_EXSTYLE, GetWindowLongA(hwnd1,GWL_EXSTYLE)|WS_EX_TOPMOST);
    ret = (GetWindowLongA(hwnd1,GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    ok(!ret, "SetWindowExStyle(hwnd1, WS_EX_TOPMOST)\n");

    ret = DestroyWindow(hwnd1);
    ok(ret, "DestroyWindow(hwnd1)\n");

    hwnd2 = CreateWindowExA(WS_EX_TOPMOST, "#32770", "TEST2", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 150, 250, 300, 200, 0, 0, hinst, 0);
    trace("created window 2: %p\n", hwnd2);
    ret = SetShellWindow(hwnd2);
    ok(!ret, "SetShellWindow(hwnd2) with WS_EX_TOPMOST\n");

    hwnd3 = CreateWindowExA(0, "#32770", "TEST3", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 200, 400, 300, 200, 0, 0, hinst, 0);
    trace("created window 3: %p\n", hwnd3);

    hwnd4 = CreateWindowExA(0, "#32770", "TEST4", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 250, 500, 300, 200, 0, 0, hinst, 0);
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

    hwnd5 = CreateWindowExA(0, "#32770", "TEST5", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 300, 600, 300, 200, 0, 0, hinst, 0);
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

static void test_MDI_create(HWND parent, HWND mdi_client, INT_PTR first_id)
{
    MDICREATESTRUCTA mdi_cs;
    HWND mdi_child, hwnd, exp_hwnd;
    INT_PTR id;
    static const WCHAR classW[] = {'M','D','I','_','c','h','i','l','d','_','C','l','a','s','s','_','1',0};
    static const WCHAR titleW[] = {'M','D','I',' ','c','h','i','l','d',0};
    BOOL isWin9x = FALSE;
    HMENU frame_menu = GetMenu(parent);

    ok(frame_menu != NULL, "Frame window didn't have a menu\n");

    mdi_cs.szClass = "MDI_child_Class_1";
    mdi_cs.szTitle = "MDI child";
    mdi_cs.hOwner = GetModuleHandleA(NULL);
    mdi_cs.x = CW_USEDEFAULT;
    mdi_cs.y = CW_USEDEFAULT;
    mdi_cs.cx = CW_USEDEFAULT;
    mdi_cs.cy = CW_USEDEFAULT;
    mdi_cs.style = 0;
    mdi_cs.lParam = (LPARAM)mdi_lParam_test_message;
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_cs.style = 0x7fffffff; /* without WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
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
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
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
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
        ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandleA(NULL),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0x7fffffff, /* without WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandleA(NULL),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0xffffffff, /* with WS_POPUP */
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandleA(NULL),
                                 (LPARAM)mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
    }
    else
    {
        ok(mdi_child != 0, "MDI child creation failed\n");
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    /* test MDICREATESTRUCT A<->W mapping */
    SetLastError(0xdeadbeef);
    mdi_child = CreateMDIWindowW(classW, titleW,
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandleA(NULL),
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
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
        ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                WS_MAXIMIZE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    else
        ok(GetMenuItemCount(frame_menu) == 4, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0x7fffffff, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0xffffffff, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    if (GetWindowLongA(mdi_client, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
    {
        ok(!mdi_child, "MDI child with WS_POPUP and with MDIS_ALLCHILDSTYLES should fail\n");
    }
    else
    {
        ok(mdi_child != 0, "MDI child creation failed\n");
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
        SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
        ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");
    }

    /* test MDICREATESTRUCT A<->W mapping */
    SetLastError(0xdeadbeef);
    mdi_child = CreateWindowExW(WS_EX_MDICHILD, classW, titleW,
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    if (!mdi_child)
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            isWin9x = TRUE;
        else
            ok(mdi_child != 0, "MDI child creation failed\n");
    }
    else
    {
        id = GetWindowLongPtrA(mdi_child, GWLP_ID);
        ok(id == first_id, "wrong child id %ld\n", id);
        hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
        exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
        ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
        ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
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
                                parent, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
        ok(!mdi_child, "WS_EX_MDICHILD with a not MDIClient parent should fail\n");
    }

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %ld\n", id);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_POPUP, /* with WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);

    /* maximized child */
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);

    trace("Creating maximized child with a caption and a thick frame\n");
    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION | WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %ld\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);
}

static void test_MDI_child_stack(HWND mdi_client)
{
    HWND child_1, child_2, child_3, child_4;
    HWND stack[4];
    MDICREATESTRUCTA cs;

    cs.szClass = "MDI_child_Class_1";
    cs.szTitle = "MDI child";
    cs.hOwner  = GetModuleHandleA(0);
    cs.x       = CW_USEDEFAULT;
    cs.y       = CW_USEDEFAULT;
    cs.cx      = CW_USEDEFAULT;
    cs.cy      = CW_USEDEFAULT;
    cs.style   = 0;
    cs.lParam  = (LPARAM)mdi_lParam_test_message;

    child_1 = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&cs);
    ok(child_1 != 0, "expected child_1 to be non NULL\n");
    child_2 = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&cs);
    ok(child_2 != 0, "expected child_2 to be non NULL\n");
    child_3 = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&cs);
    ok(child_3 != 0, "expected child_3 to be non NULL\n");
    child_4 = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&cs);
    ok(child_4 != 0, "expected child_4 to be non NULL\n");

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    stack[2] = GetWindow(stack[1], GW_HWNDNEXT);
    stack[3] = GetWindow(stack[2], GW_HWNDNEXT);
    trace("Initial MDI child stack: %p->%p->%p->%p\n", stack[0], stack[1], stack[2], stack[3]);
    ok(stack[0] == child_4 && stack[1] == child_3 &&
        stack[2] == child_2 && stack[3] == child_1,
        "Unexpected initial order, should be: %p->%p->%p->%p\n",
            child_4, child_3, child_2, child_1);

    trace("Activate child next to %p\n", child_3);
    SendMessageA(mdi_client, WM_MDINEXT, (WPARAM)child_3, 0);

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    stack[2] = GetWindow(stack[1], GW_HWNDNEXT);
    stack[3] = GetWindow(stack[2], GW_HWNDNEXT);
    ok(stack[0] == child_2 && stack[1] == child_4 &&
        stack[2] == child_1 && stack[3] == child_3,
        "Broken MDI child stack:\nexpected: %p->%p->%p->%p, but got: %p->%p->%p->%p\n",
            child_2, child_4, child_1, child_3, stack[0], stack[1], stack[2], stack[3]);

    trace("Activate child previous to %p\n", child_1);
    SendMessageA(mdi_client, WM_MDINEXT, (WPARAM)child_1, 1);

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    stack[2] = GetWindow(stack[1], GW_HWNDNEXT);
    stack[3] = GetWindow(stack[2], GW_HWNDNEXT);
    ok(stack[0] == child_4 && stack[1] == child_2 &&
        stack[2] == child_1 && stack[3] == child_3,
        "Broken MDI child stack:\nexpected: %p->%p->%p->%p, but got: %p->%p->%p->%p\n",
            child_4, child_2, child_1, child_3, stack[0], stack[1], stack[2], stack[3]);

    DestroyWindow(child_1);
    DestroyWindow(child_2);
    DestroyWindow(child_3);
    DestroyWindow(child_4);
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

    trace("max rect %s\n", wine_dbgstr_rect(&rect));
}

static LRESULT WINAPI mdi_child_wnd_proc_1(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_NCCREATE:
        case WM_CREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;
            MDICREATESTRUCTA *mdi_cs = cs->lpCreateParams;

            ok(cs->dwExStyle & WS_EX_MDICHILD, "WS_EX_MDICHILD should be set\n");
            ok(mdi_cs->lParam == (LPARAM)mdi_lParam_test_message, "wrong mdi_cs->lParam\n");

            ok(!lstrcmpA(cs->lpszClass, "MDI_child_Class_1"), "wrong class name\n");
            ok(!lstrcmpA(cs->lpszClass, mdi_cs->szClass), "class name does not match\n");
            ok(!lstrcmpA(cs->lpszName, "MDI child"), "wrong title\n");
            ok(!lstrcmpA(cs->lpszName, mdi_cs->szTitle), "title does not match\n");
            ok(cs->hInstance == mdi_cs->hOwner, "%p != %p\n", cs->hInstance, mdi_cs->hOwner);

            /* MDICREATESTRUCT should have original values */
            ok(mdi_cs->style == 0 || mdi_cs->style == 0x7fffffff || mdi_cs->style == 0xffffffff || mdi_cs->style == WS_MAXIMIZE,
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

            GetClientRect(client, &rc);

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

            trace("%s: x %d, y %d, cx %d, cy %d\n", (msg == WM_NCCREATE) ? "WM_NCCREATE" : "WM_CREATE",
                cs->x, cs->y, cs->cx, cs->cy);

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

            style = GetWindowLongA(hwnd, GWL_STYLE);
            exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

            GetClientRect(parent, &rc);
            trace("WM_GETMINMAXINFO: parent %p client size = (%d x %d)\n", parent, rc.right, rc.bottom);

            GetClientRect(parent, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
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
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match, window=%s pos=%s\n",
               wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
            GetWindowRect(hwnd, &rc1);
            GetClientRect(hwnd, &rc2);
            DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match, window=%s client=%s\n",
               wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
        }
        /* fall through */
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            WINDOWPOS my_winpos = *winpos;

            trace("%s: %p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                  (msg == WM_WINDOWPOSCHANGING) ? "WM_WINDOWPOSCHANGING" : "WM_WINDOWPOSCHANGED",
                  winpos->hwnd, winpos->hwndInsertAfter,
                  winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

            DefWindowProcA(hwnd, msg, wparam, lparam);

            ok(!memcmp(&my_winpos, winpos, sizeof(WINDOWPOS)),
               "DefWindowProc should not change WINDOWPOS: %p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                  winpos->hwnd, winpos->hwndInsertAfter,
                  winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

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
            return 1;

        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            RECT rc1, rc2;

            GetWindowRect(hwnd, &rc1);
            trace("window: %s\n", wine_dbgstr_rect(&rc1));
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            trace("pos: %s\n", wine_dbgstr_rect(&rc2));
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
    static const DWORD style[] = { 0, WS_HSCROLL, WS_VSCROLL, WS_HSCROLL | WS_VSCROLL };
    HWND mdi_hwndMain, mdi_client, mdi_child;
    CLIENTCREATESTRUCT client_cs;
    RECT rc;
    DWORD i;
    MSG msg;
    HMENU frame_menu, child_menu;

    if (!mdi_RegisterWindowClasses()) assert(0);

    mdi_hwndMain = CreateWindowExA(0, "MDI_parent_Class", "MDI parent window",
                                   WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                   WS_MAXIMIZEBOX /*| WS_VISIBLE*/,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandleA(NULL), NULL);
    assert(mdi_hwndMain);

    frame_menu = CreateMenu();

    GetClientRect(mdi_hwndMain, &rc);

    client_cs.hWindowMenu = 0;
    client_cs.idFirstChild = 1;

    for (i = 0; i < sizeof(style)/sizeof(style[0]); i++)
    {
        SCROLLINFO si;
        BOOL ret, gotit;

        mdi_client = CreateWindowExA(0, "mdiclient", NULL,
                                 WS_CHILD | style[i],
                                 0, 0, rc.right, rc.bottom,
                                 mdi_hwndMain, 0, 0, &client_cs);
        ok(mdi_client != 0, "MDI client creation failed\n");

        mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, 0,
                                mdi_lParam_test_message);
        ok(mdi_child != 0, "MDI child creation failed\n");

        SendMessageW(mdi_child, WM_SIZE, SIZE_MAXIMIZED, 0);
        SetMenu(mdi_hwndMain, frame_menu);

        ok(GetMenuItemCount(frame_menu) == 0, "Frame menu should be empty after child maximize, but has %u\n",
                GetMenuItemCount(frame_menu));

        child_menu = CreateMenu();
        SendMessageW(mdi_client, WM_MDISETMENU, 0, (LPARAM)child_menu);

        ok(GetMenuItemCount(frame_menu) == 0, "Frame menu should be empty after WM_MDISETMENU, but has %u\n",
                GetMenuItemCount(frame_menu));

        SendMessageW(mdi_child, WM_SIZE, SIZE_RESTORED, 0);

        ok(GetMenuItemCount(frame_menu) == 0, "Frame menu should be empty after child restored, but has %u items\n",
                GetMenuItemCount(frame_menu));

        SetMenu(mdi_hwndMain, NULL);

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        ret = GetScrollInfo(mdi_client, SB_HORZ, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_HORZ) failed\n", style[i]);
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_HORZ) should fail\n", style[i]);

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_VERT) failed\n", style[i]);
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_VERT) should fail\n", style[i]);

        SetWindowPos(mdi_child, 0, -100, -100, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        ret = GetScrollInfo(mdi_client, SB_HORZ, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_HORZ) failed\n", style[i]);
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_HORZ) should fail\n", style[i]);

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_VERT) failed\n", style[i]);
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_VERT) should fail\n", style[i]);

        gotit = FALSE;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_MOUSEMOVE || msg.message == WM_PAINT)
            {
                DispatchMessageA(&msg);
                continue;
            }

            if (msg.message == 0x003f) /* WM_MDICALCCHILDSCROLL ??? */
            {
                ok(msg.hwnd == mdi_client, "message 0x003f should be posted to mdiclient\n");
                gotit = TRUE;
            }
            else
                ok(msg.hwnd != mdi_client, "message %04x should not be posted to mdiclient\n", msg.message);
            DispatchMessageA(&msg);
        }
        ok(gotit, "message 0x003f should appear after SetWindowPos\n");

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        ret = GetScrollInfo(mdi_client, SB_HORZ, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_HORZ) failed\n", style[i]);
todo_wine
            ok(si.nPage != 0, "expected !0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin != 0, "expected !0\n");
            ok(si.nMax != 100, "expected !100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_HORZ) should fail\n", style[i]);

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "style %#x: GetScrollInfo(SB_VERT) failed\n", style[i]);
todo_wine
            ok(si.nPage != 0, "expected !0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin != 0, "expected !0\n");
            ok(si.nMax != 100, "expected !100\n");
        }
        else
            ok(!ret, "style %#x: GetScrollInfo(SB_VERT) should fail\n", style[i]);

        DestroyMenu(child_menu);
        DestroyWindow(mdi_child);
        DestroyWindow(mdi_client);
    }

    SetMenu(mdi_hwndMain, frame_menu);

    mdi_client = CreateWindowExA(0, "mdiclient", NULL,
                             WS_CHILD,
                             0, 0, rc.right, rc.bottom,
                             mdi_hwndMain, 0, 0, &client_cs);
    ok(mdi_client != 0, "MDI client creation failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                            0,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            mdi_client, 0, 0,
                            mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");

    SendMessageW(mdi_child, WM_SIZE, SIZE_MAXIMIZED, 0);
    ok(GetMenuItemCount(frame_menu) == 4, "Frame menu should have 4 items after child maximize, but has %u\n",
            GetMenuItemCount(frame_menu));

    child_menu = CreateMenu();
    SendMessageW(mdi_client, WM_MDISETMENU, 0, (LPARAM)child_menu);

    ok(GetMenuItemCount(frame_menu) == 4, "Frame menu should have 4 items after WM_MDISETMENU, but has %u\n",
            GetMenuItemCount(frame_menu));

    SendMessageW(mdi_child, WM_SIZE, SIZE_RESTORED, 0);

    ok(GetMenuItemCount(frame_menu) == 0, "Frame menu should be empty after child restored, but has %u items\n",
            GetMenuItemCount(frame_menu));

    DestroyMenu(child_menu);
    DestroyWindow(mdi_child);
    DestroyWindow(mdi_client);

    /* MDIClient without MDIS_ALLCHILDSTYLES */
    mdi_client = CreateWindowExA(0, "mdiclient",
                                 NULL,
                                 WS_CHILD /*| WS_VISIBLE*/,
                                  /* tests depend on a not zero MDIClient size */
                                 0, 0, rc.right, rc.bottom,
                                 mdi_hwndMain, 0, GetModuleHandleA(NULL),
                                 &client_cs);
    assert(mdi_client);
    test_MDI_create(mdi_hwndMain, mdi_client, client_cs.idFirstChild);
    DestroyWindow(mdi_client);

    /* MDIClient with MDIS_ALLCHILDSTYLES */
    mdi_client = CreateWindowExA(0, "mdiclient",
                                 NULL,
                                 WS_CHILD | MDIS_ALLCHILDSTYLES /*| WS_VISIBLE*/,
                                  /* tests depend on a not zero MDIClient size */
                                 0, 0, rc.right, rc.bottom,
                                 mdi_hwndMain, 0, GetModuleHandleA(NULL),
                                 &client_cs);
    assert(mdi_client);
    test_MDI_create(mdi_hwndMain, mdi_client, client_cs.idFirstChild);
    DestroyWindow(mdi_client);

    /* Test child window stack management */
    mdi_client = CreateWindowExA(0, "mdiclient",
                                 NULL,
                                 WS_CHILD,
                                 0, 0, rc.right, rc.bottom,
                                 mdi_hwndMain, 0, GetModuleHandleA(NULL),
                                 &client_cs);
    assert(mdi_client);
    test_MDI_child_stack(mdi_client);
    DestroyWindow(mdi_client);
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
    HICON icon = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    HICON icon2 = LoadIconA(0, (LPCSTR)IDI_QUESTION);
    HICON small_icon = LoadImageA(0, (LPCSTR)IDI_APPLICATION, IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED );
    HICON res;

    cls.cbSize = sizeof(cls);
    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = 0;
    cls.hIcon = LoadIconA(0, (LPCSTR)IDI_HAND);
    cls.hIconSm = small_icon;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( (res && res != small_icon && res != icon2) || broken(!res), "wrong small2 icon %p\n", res );
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon );
    ok( res == 0, "wrong previous small icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == icon, "wrong small icon after set %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == icon || broken(!res), "wrong small2 icon after set %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small_icon );
    ok( res == icon, "wrong previous small icon %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == small_icon, "wrong small icon after set %p/%p\n", res, small_icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == small_icon || broken(!res), "wrong small2 icon after set %p/%p\n", res, small_icon );

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
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static void test_SetWindowPos(HWND hwnd, HWND hwnd2)
{
    RECT orig_win_rc, rect;
    LONG_PTR old_proc;
    HWND hwnd_grandchild, hwnd_child, hwnd_child2;
    HWND hwnd_desktop;
    RECT rc1, rc2;
    BOOL ret;

    SetRect(&rect, 111, 222, 333, 444);
    ok(!GetWindowRect(0, &rect), "GetWindowRect succeeded\n");
    ok(rect.left == 111 && rect.top == 222 && rect.right == 333 && rect.bottom == 444,
       "wrong window rect %s\n", wine_dbgstr_rect(&rect));

    SetRect(&rect, 111, 222, 333, 444);
    ok(!GetClientRect(0, &rect), "GetClientRect succeeded\n");
    ok(rect.left == 111 && rect.top == 222 && rect.right == 333 && rect.bottom == 444,
       "wrong window rect %s\n", wine_dbgstr_rect(&rect));

    GetWindowRect(hwnd, &orig_win_rc);

    old_proc = SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (ULONG_PTR)nccalcsize_proc );
    ret = SetWindowPos(hwnd, 0, 100, 100, 0, 0, SWP_NOZORDER|SWP_FRAMECHANGED);
    ok(ret, "Got %d\n", ret);
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 100 && rect.top == 100 && rect.right == 100 && rect.bottom == 100,
        "invalid window rect %s\n", wine_dbgstr_rect(&rect));
    GetClientRect( hwnd, &rect );
    MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
    ok( rect.left == 90 && rect.top == 90 && rect.right == 110 && rect.bottom == 110,
        "invalid client rect %s\n", wine_dbgstr_rect(&rect));

    ret = SetWindowPos(hwnd, 0, 200, 200, 0, 0, SWP_NOZORDER|SWP_FRAMECHANGED);
    ok(ret, "Got %d\n", ret);
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 200 && rect.top == 200 && rect.right == 200 && rect.bottom == 200,
        "invalid window rect %s\n", wine_dbgstr_rect(&rect));
    GetClientRect( hwnd, &rect );
    MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
    ok( rect.left == 210 && rect.top == 210 && rect.right == 190 && rect.bottom == 190,
        "invalid client rect %s\n", wine_dbgstr_rect(&rect));

    ret = SetWindowPos(hwnd, 0, orig_win_rc.left, orig_win_rc.top,
                      orig_win_rc.right, orig_win_rc.bottom, 0);
    ok(ret, "Got %d\n", ret);
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, old_proc );

    /* Win9x truncates coordinates to 16-bit irrespectively */
    if (!is_win9x)
    {
        ret = SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOMOVE);
        ok(ret, "Got %d\n", ret);
        ret = SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOMOVE);
        ok(ret, "Got %d\n", ret);

        ret = SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOSIZE);
        ok(ret, "Got %d\n", ret);
        ret = SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOSIZE);
        ok(ret, "Got %d\n", ret);
    }

    ret = SetWindowPos(hwnd, 0, orig_win_rc.left, orig_win_rc.top,
                       orig_win_rc.right, orig_win_rc.bottom, 0);
    ok(ret, "Got %d\n", ret);

    ok(!(GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST), "WS_EX_TOPMOST should not be set\n");
    ret = SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "Got %d\n", ret);
    ok(GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST, "WS_EX_TOPMOST should be set\n");
    ret = SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "Got %d\n", ret);
    ok(GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST, "WS_EX_TOPMOST should be set\n");
    ret = SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "Got %d\n", ret);
    ok(!(GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST), "WS_EX_TOPMOST should not be set\n");

    hwnd_desktop = GetDesktopWindow();
    ok(!!hwnd_desktop, "Failed to get hwnd_desktop window (%d).\n", GetLastError());
    hwnd_child = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd);
    ok(!!hwnd_child, "Failed to create child window (%d)\n", GetLastError());
    hwnd_grandchild = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd_child);
    ok(!!hwnd_child, "Failed to create child window (%d)\n", GetLastError());
    hwnd_child2 = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd);
    ok(!!hwnd_child2, "Failed to create second child window (%d)\n", GetLastError());

    ret = SetWindowPos(hwnd, hwnd2, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);
    check_active_state(hwnd, hwnd, hwnd);

    ret = SetWindowPos(hwnd2, hwnd, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Returns TRUE also for windows that are not siblings */
    ret = SetWindowPos(hwnd_child, hwnd2, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    ret = SetWindowPos(hwnd2, hwnd_child, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Does not seem to do anything even without passing flags, still returns TRUE */
    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, hwnd2 , 1, 2, 3, 4, 0);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc1, &rc2), "%s != %s\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Same thing the other way around. */
    GetWindowRect(hwnd2, &rc1);
    ret = SetWindowPos(hwnd2, hwnd_child, 1, 2, 3, 4, 0);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd2, &rc2);
    ok(EqualRect(&rc1, &rc2), "%s != %s\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* .. and with these windows. */
    GetWindowRect(hwnd_grandchild, &rc1);
    ret = SetWindowPos(hwnd_grandchild, hwnd_child2, 1, 2, 3, 4, 0);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd_grandchild, &rc2);
    ok(EqualRect(&rc1, &rc2), "%s != %s\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Add SWP_NOZORDER and it will be properly resized. */
    GetWindowRect(hwnd_grandchild, &rc1);
    ret = SetWindowPos(hwnd_grandchild, hwnd_child2, 1, 2, 3, 4, SWP_NOZORDER);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd_grandchild, &rc2);
    ok((rc1.left+1) == rc2.left && (rc1.top+2) == rc2.top &&
       (rc1.left+4) == rc2.right && (rc1.top+6) == rc2.bottom,
       "(%d,%d)-(%d,%d) != %s\n", rc1.left+1, rc1.top+2, rc1.left+4, rc1.top+6,
       wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Given a sibling window, the window is properly resized. */
    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, hwnd_child2, 1, 2, 3, 4, 0);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok((rc1.left+1) == rc2.left && (rc1.top+2) == rc2.top &&
       (rc1.left+4) == rc2.right && (rc1.top+6) == rc2.bottom,
       "(%d,%d)-(%d,%d) != %s\n", rc1.left+1, rc1.top+2, rc1.left+4, rc1.top+6,
       wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Involving the desktop window changes things. */
    ret = SetWindowPos(hwnd_child, hwnd_desktop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(!ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, hwnd_desktop, 0, 0, 0, 0, 0);
    ok(!ret, "Got %d\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc1, &rc2), "%s != %s\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    ret = SetWindowPos(hwnd_desktop, hwnd_child, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(!ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    ret = SetWindowPos(hwnd_desktop, hwnd, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(!ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    ret = SetWindowPos(hwnd, hwnd_desktop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    ok(!ret, "Got %d\n", ret);
    check_active_state(hwnd2, hwnd2, hwnd2);

    DestroyWindow(hwnd_grandchild);
    DestroyWindow(hwnd_child);
    DestroyWindow(hwnd_child2);

    hwnd_child = create_tool_window(WS_CHILD|WS_POPUP|WS_SYSMENU, hwnd2);
    ok(!!hwnd_child, "Failed to create child window (%d)\n", GetLastError());
    ret = SetWindowPos(hwnd_child, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    ok(ret, "Got %d\n", ret);
    flush_events( TRUE );
    todo_wine check_active_state(hwnd2, hwnd2, hwnd2);
    DestroyWindow(hwnd_child);
}

static void test_SetMenu(HWND parent)
{
    HWND child;
    HMENU hMenu, ret;
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

    style = GetWindowLongA(child, GWL_STYLE);
    SetWindowLongA(child, GWL_STYLE, style | WS_POPUP);
    ok(SetMenu(child, hMenu), "SetMenu on a popup child window should not fail\n");
    ok(SetMenu(child, 0), "SetMenu on a popup child window should not fail\n");
    SetWindowLongA(child, GWL_STYLE, style);

    SetWindowLongA(child, GWL_STYLE, style | WS_OVERLAPPED);
    ok(!SetMenu(child, hMenu), "SetMenu on an overlapped child window should fail\n");
    SetWindowLongA(child, GWL_STYLE, style);

    DestroyWindow(child);
    DestroyMenu(hMenu);
}

static void test_window_tree(HWND parent, const DWORD *style, const int *order, int total)
{
    HWND child[5], hwnd;
    INT_PTR i;

    assert(total <= 5);

    hwnd = GetWindow(parent, GW_CHILD);
    ok(!hwnd, "have to start without children to perform the test\n");

    for (i = 0; i < total; i++)
    {
        if (style[i] & DS_CONTROL)
        {
            child[i] = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(32770), "", style[i] & ~WS_VISIBLE,
                                       0,0,0,0, parent, (HMENU)i, 0, NULL);
            if (style[i] & WS_VISIBLE)
                ShowWindow(child[i], SW_SHOW);

            SetWindowPos(child[i], HWND_BOTTOM, 0,0,10,10, SWP_NOACTIVATE);
        }
        else
            child[i] = CreateWindowExA(0, "static", "", style[i], 0,0,10,10,
                                       parent, (HMENU)i, 0, NULL);
        trace("child[%ld] = %p\n", i, child[i]);
        ok(child[i] != 0, "CreateWindowEx failed to create child window\n");
    }

    hwnd = GetWindow(parent, GW_CHILD);
    ok(hwnd != 0, "GetWindow(GW_CHILD) failed\n");
    ok(hwnd == GetWindow(child[total - 1], GW_HWNDFIRST), "GW_HWNDFIRST is wrong\n");
    ok(child[order[total - 1]] == GetWindow(child[0], GW_HWNDLAST), "GW_HWNDLAST is wrong\n");

    for (i = 0; i < total; i++)
    {
        trace("hwnd[%ld] = %p\n", i, hwnd);
        ok(child[order[i]] == hwnd, "Z order of child #%ld is wrong\n", i);

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
    while (test && test != next &&
           (GetWindowThreadProcessId(test, NULL) != our_pid ||
            UlongToHandle(GetWindowLongPtrA(test, GWLP_HINSTANCE)) != GetModuleHandleA(NULL) ||
            GetWindow(test, GW_OWNER) == next))
    {
        /*trace("skipping next %p (%p)\n", test, UlongToHandle(GetWindowLongPtr(test, GWLP_HINSTANCE)));*/
        test = GetWindow(test, GW_HWNDNEXT);
    }
    ok_(file, line)(next == test, "%p: expected next %p, got %p\n", hwnd, next, test);

    test = GetWindow(hwnd, GW_HWNDPREV);
    /* skip foreign windows */
    while (test && test != prev &&
           (GetWindowThreadProcessId(test, NULL) != our_pid ||
            UlongToHandle(GetWindowLongPtrA(test, GWLP_HINSTANCE)) != GetModuleHandleA(NULL) ||
            GetWindow(test, GW_OWNER) == hwnd))
    {
        /*trace("skipping prev %p (%p)\n", test, UlongToHandle(GetWindowLongPtr(test, GWLP_HINSTANCE)));*/
        test = GetWindow(test, GW_HWNDPREV);
    }
    ok_(file, line)(prev == test, "%p: expected prev %p, got %p\n", hwnd, prev, test);

    test = GetWindow(hwnd, GW_OWNER);
    ok_(file, line)(owner == test, "%p: expected owner %p, got %p\n", hwnd, owner, test);

    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok_(file, line)(!(ex_style & WS_EX_TOPMOST) == !topmost, "%p: expected %stopmost\n",
                    hwnd, topmost ? "" : "NOT ");
}

static void test_popup_zorder(HWND hwnd_D, HWND hwnd_E, DWORD style)
{
    HWND hwnd_A, hwnd_B, hwnd_C, hwnd_F;

    trace("hwnd_D %p, hwnd_E %p\n", hwnd_D, hwnd_E);

    SetWindowPos(hwnd_E, hwnd_D, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);
    check_z_order(hwnd_E, 0, hwnd_D, 0, FALSE);

    hwnd_F = CreateWindowExA(0, "MainWindowClass", "Owner window",
                            WS_OVERLAPPED | WS_CAPTION,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandleA(NULL), NULL);
    trace("hwnd_F %p\n", hwnd_F);
    check_z_order(hwnd_F, hwnd_D, 0, 0, FALSE);

    SetWindowPos(hwnd_F, hwnd_E, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);

    hwnd_C = CreateWindowExA(0, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandleA(NULL), NULL);
    trace("hwnd_C %p\n", hwnd_C);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, 0, hwnd_F, FALSE);

    hwnd_B = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandleA(NULL), NULL);
    trace("hwnd_B %p\n", hwnd_B);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, 0, hwnd_F, TRUE);

    hwnd_A = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandleA(NULL), NULL);
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

    /* make hwnd_C owned by a topmost window */
    DestroyWindow( hwnd_C );
    hwnd_C = CreateWindowExA(0, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            hwnd_A, 0, GetModuleHandleA(NULL), NULL);
    trace("hwnd_C %p\n", hwnd_C);
    check_z_order(hwnd_E, 0, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_F, 0, FALSE);
    check_z_order(hwnd_F, hwnd_D, hwnd_B, 0, FALSE);
    check_z_order(hwnd_B, hwnd_F, hwnd_A, hwnd_F, TRUE);
    check_z_order(hwnd_A, hwnd_B, hwnd_C, 0, TRUE);
    check_z_order(hwnd_C, hwnd_A, 0, hwnd_A, TRUE);

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
    if (is_win9x)
    {
        trace("win9x, mapping to screen coords\n");
        MapWindowPoints( hwnd, 0, (POINT *)&rgn_rect, 2 );
    }
    trace("win: %s\n", wine_dbgstr_rect(&win_rect));
    trace("rgn: %s\n", wine_dbgstr_rect(&rgn_rect));
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
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void test_SetFocus(HWND hwnd)
{
    HWND child, child2, ret;
    WNDPROC old_wnd_proc;

    /* check if we can set focus to non-visible windows */

    ShowWindow(hwnd, SW_SHOW);
    SetFocus(0);
    SetFocus(hwnd);
    ok( GetFocus() == hwnd, "Failed to set focus to visible window %p\n", hwnd );
    ok( GetWindowLongA(hwnd,GWL_STYLE) & WS_VISIBLE, "Window %p not visible\n", hwnd );
    ShowWindow(hwnd, SW_HIDE);
    SetFocus(0);
    SetFocus(hwnd);
    ok( GetFocus() == hwnd, "Failed to set focus to invisible window %p\n", hwnd );
    ok( !(GetWindowLongA(hwnd,GWL_STYLE) & WS_VISIBLE), "Window %p still visible\n", hwnd );
    child = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    assert(child);
    SetFocus(child);
    ok( GetFocus() == child, "Failed to set focus to invisible child %p\n", child );
    ok( !(GetWindowLongA(child,GWL_STYLE) & WS_VISIBLE), "Child %p is visible\n", child );
    ShowWindow(child, SW_SHOW);
    ok( GetWindowLongA(child,GWL_STYLE) & WS_VISIBLE, "Child %p is not visible\n", child );
    ok( GetFocus() == child, "Focus no longer on child %p\n", child );
    ShowWindow(child, SW_HIDE);
    ok( !(GetWindowLongA(child,GWL_STYLE) & WS_VISIBLE), "Child %p is visible\n", child );
    ok( GetFocus() == hwnd, "Focus should be on parent %p, not %p\n", hwnd, GetFocus() );
    ShowWindow(child, SW_SHOW);
    child2 = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, child, 0, 0, NULL);
    assert(child2);
    ShowWindow(child2, SW_SHOW);
    SetFocus(child2);
    ShowWindow(child, SW_HIDE);
    ok( !(GetWindowLongA(child,GWL_STYLE) & WS_VISIBLE), "Child %p is visible\n", child );
    ok( GetFocus() == child2, "Focus should be on %p, not %p\n", child2, GetFocus() );
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
    old_wnd_proc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)set_focus_on_activate_proc);
    ShowWindow(hwnd, SW_RESTORE);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
todo_wine
    ok( GetFocus() == child, "Focus should be on child %p, not %p\n", child, GetFocus() );
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)old_wnd_proc);

    SetFocus( hwnd );
    SetParent( child, GetDesktopWindow());
    SetParent( child2, child );
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p\n", hwnd );
    ret = SetFocus( child2 );
    ok( ret == 0, "SetFocus %p should fail\n", child2);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p\n", hwnd );
    ret = SetFocus( child );
    ok( ret == 0, "SetFocus %p should fail\n", child);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p\n", hwnd );
    SetWindowLongW( child, GWL_STYLE, WS_POPUP|WS_CHILD );
    SetFocus( child2 );
    ok( GetActiveWindow() == child, "child window %p should be active\n", child);
    ok( GetFocus() == child2, "Focus should be on child2 %p\n", child2 );
    SetFocus( hwnd );
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
    ok( GetFocus() == hwnd, "Focus should be on parent %p\n", hwnd );
    SetFocus( child );
    ok( GetActiveWindow() == child, "child window %p should be active\n", child);
    ok( GetFocus() == child, "Focus should be on child %p\n", child );

    DestroyWindow( child2 );
    DestroyWindow( child );
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
        ros_skip_flaky
        check_wnd_state(0, 0, 0, 0);
        hwnd2 = SetActiveWindow(hwnd);
        ros_skip_flaky
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

struct create_window_thread_params
{
    HWND window;
    HANDLE window_created;
    HANDLE test_finished;
};

static DWORD WINAPI create_window_thread(void *param)
{
    struct create_window_thread_params *p = param;
    DWORD res;
    BOOL ret;

    p->window = CreateWindowA("static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, 0, 0, 0, 0);

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(p->test_finished, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    DestroyWindow(p->window);
    return 0;
}

static void test_SetForegroundWindow(HWND hwnd)
{
    struct create_window_thread_params thread_params;
    HANDLE thread;
    DWORD res, tid;
    BOOL ret;
    HWND hwnd2;
    MSG msg;
    LONG style;

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
    if (!ret)
    {
        skip( "SetForegroundWindow not working\n" );
        return;
    }
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
    ret = SetForegroundWindow( GetDesktopWindow() );
    ok(ret, "SetForegroundWindow(desktop) error: %d\n", GetLastError());
    hwnd2 = GetForegroundWindow();
    ok(hwnd2 != hwnd, "Wrong foreground window %p\n", hwnd2);

    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    /*trace("testing SetForegroundWindow on an invisible window %p\n", hwnd);*/
    ret = SetForegroundWindow(hwnd);
    ok(ret || broken(!ret), /* win98 */ "SetForegroundWindow returned FALSE instead of TRUE\n");
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

    hwnd2 = CreateWindowA("static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, 0, 0, 0, 0);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    thread_params.window_created = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread = CreateThread(NULL, 0, create_window_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());
    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
    check_wnd_state(hwnd2, thread_params.window, hwnd2, 0);

    SetForegroundWindow(hwnd2);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    if (0) check_wnd_state(hwnd2, hwnd2, hwnd2, 0);
    todo_wine ok(GetActiveWindow() == hwnd2, "Expected active window %p, got %p.\n", hwnd2, GetActiveWindow());
    todo_wine ok(GetFocus() == hwnd2, "Expected focus window %p, got %p.\n", hwnd2, GetFocus());

    SetForegroundWindow(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    style = GetWindowLongA(hwnd2, GWL_STYLE) | WS_CHILD;
    ok(SetWindowLongA(hwnd2, GWL_STYLE, style), "SetWindowLong failed\n");
    ok(SetForegroundWindow(hwnd2), "SetForegroundWindow failed\n");
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    SetForegroundWindow(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    ok(SetWindowLongA(hwnd2, GWL_STYLE, style & (~WS_POPUP)), "SetWindowLong failed\n");
    ok(!SetForegroundWindow(hwnd2), "SetForegroundWindow failed\n");
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);
    DestroyWindow(hwnd2);
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
	SendMessageA(button, WM_KILLFOCUS, 0, 0);
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
    HWND button, capture;

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
    /* old active window test depends on previously executed window
     * activation tests, and fails under NT4.
    check_wnd_state(oldActive, 0, oldFocus, 0);*/
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
    SendMessageA(button, WM_KILLFOCUS, 0, 0);
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

static LRESULT CALLBACK test_capture_4_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GUITHREADINFO gti;
    HWND cap_wnd, cap_wnd2, set_cap_wnd;
    BOOL status;
    switch (msg)
    {
        case WM_CAPTURECHANGED:

            /* now try to release capture from menu. this should fail */
            if (pGetGUIThreadInfo)
            {
                memset(&gti, 0, sizeof(GUITHREADINFO));
                gti.cbSize = sizeof(GUITHREADINFO);
                status = pGetGUIThreadInfo(GetCurrentThreadId(), &gti);
                ok(status, "GetGUIThreadInfo() failed!\n");
                ok(gti.flags & GUI_INMENUMODE, "Thread info incorrect (flags=%08X)!\n", gti.flags);
            }
            cap_wnd = GetCapture();

            ok(cap_wnd == (HWND)lParam, "capture window %p does not match lparam %lx\n", cap_wnd, lParam);
            todo_wine ok(cap_wnd == hWnd, "capture window %p does not match hwnd %p\n", cap_wnd, hWnd);

            /* check that re-setting the capture for the menu fails */
            set_cap_wnd = SetCapture(cap_wnd);
            ok(!set_cap_wnd || broken(set_cap_wnd == cap_wnd), /* nt4 */
               "SetCapture should have failed!\n");
            if (set_cap_wnd)
            {
                DestroyWindow(hWnd);
                break;
            }

            /* check that SetCapture fails for another window and that it does not touch the error code */
            set_cap_wnd = SetCapture(hWnd);
            ok(!set_cap_wnd, "SetCapture should have failed!\n");

            /* check that ReleaseCapture fails and does not touch the error code */
            status = ReleaseCapture();
            ok(!status, "ReleaseCapture should have failed!\n");

            /* check that thread info did not change */
            if (pGetGUIThreadInfo)
            {
                memset(&gti, 0, sizeof(GUITHREADINFO));
                gti.cbSize = sizeof(GUITHREADINFO);
                status = pGetGUIThreadInfo(GetCurrentThreadId(), &gti);
                ok(status, "GetGUIThreadInfo() failed!\n");
                ok(gti.flags & GUI_INMENUMODE, "Thread info incorrect (flags=%08X)!\n", gti.flags);
            }

            /* verify that no capture change took place */
            cap_wnd2 = GetCapture();
            ok(cap_wnd2 == cap_wnd, "Capture changed!\n");

            /* we are done. kill the window */
            DestroyWindow(hWnd);
            break;

        default:
            return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }
    return 0;
}

/* Test that no-one can mess around with the current capture while a menu is open */
static void test_capture_4(void)
{
    BOOL ret;
    HMENU hmenu;
    HWND hwnd;
    WNDCLASSA wclass;
    HINSTANCE hInstance = GetModuleHandleA( NULL );
    ATOM aclass;

    if (!pGetGUIThreadInfo)
    {
        win_skip("GetGUIThreadInfo is not available\n");
        return;
    }
    wclass.lpszClassName = "TestCapture4Class";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = test_capture_4_proc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPCSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( 0, (LPCSTR)IDC_ARROW );
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    aclass = RegisterClassA( &wclass );
    ok( aclass, "RegisterClassA failed with error %d\n", GetLastError());
    hwnd = CreateWindowA( wclass.lpszClassName, "MenuTest",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                          400, 200, NULL, NULL, hInstance, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());
    if (!hwnd) return;
    hmenu = CreatePopupMenu();

    ret = AppendMenuA( hmenu, MF_STRING, 1, "winetest2");
    ok( ret, "AppendMenuA has failed!\n");

    /* set main window to have initial capture */
    SetCapture(hwnd);

    if (is_win9x)
    {
        win_skip("TrackPopupMenu test crashes on Win9x/WinMe\n");
    }
    else
    {
        /* create popup (it will self-destruct) */
        ret = TrackPopupMenu(hmenu, TPM_RETURNCMD, 100, 100, 0, hwnd, NULL);
        ok( ret == 0, "TrackPopupMenu returned %d expected zero\n", ret);
    }

    /* clean up */
    DestroyMenu(hmenu);
    DestroyWindow(hwnd);
}

/* PeekMessage wrapper that ignores the messages we don't care about */
static BOOL peek_message( MSG *msg )
{
    BOOL ret;
    do
    {
        ret = PeekMessageA(msg, 0, 0, 0, PM_REMOVE);
    } while (ret && ignore_message(msg->message));
    return ret;
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
    ret = peek_message(&msg);
    ok( ret, "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    PostThreadMessageA(GetCurrentThreadId(), WM_KEYDOWN, 0, 0);
    ret = peek_message(&msg);
    ok(ret, "no message available\n");
    ok(!msg.hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    keybd_event(VK_SPACE, 0, 0, 0);
    if (!peek_message(&msg))
    {
        skip( "keybd_event didn't work, skipping keyboard test\n" );
        return;
    }
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    SetFocus(0);
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    flush_events( TRUE );

    PostMessageA(hwnd, WM_KEYDOWN, 0, 0);
    ret = peek_message(&msg);
    ok(ret, "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    PostThreadMessageA(GetCurrentThreadId(), WM_KEYDOWN, 0, 0);
    ret = peek_message(&msg);
    ok(ret, "no message available\n");
    ok(!msg.hwnd && msg.message == WM_KEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    keybd_event(VK_SPACE, 0, 0, 0);
    ret = peek_message(&msg);
    ok(ret, "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_SYSKEYDOWN, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);
}

static BOOL wait_for_message( MSG *msg )
{
    BOOL ret;

    for (;;)
    {
        ret = peek_message(msg);
        if (ret)
        {
            if (msg->message == WM_PAINT) DispatchMessageA(msg);
            else break;
        }
        else if (MsgWaitForMultipleObjects( 0, NULL, FALSE, 100, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
    }
    if (!ret) msg->message = 0;
    return ret;
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

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );

    GetWindowRect(hwnd, &rc);
    trace("main window %p: %s\n", hwnd, wine_dbgstr_rect(&rc));

    popup = CreateWindowExA(0, "MainWindowClass", NULL, WS_POPUP,
                            rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                            hwnd, 0, 0, NULL);
    assert(popup != 0);
    ShowWindow(popup, SW_SHOW);
    UpdateWindow(popup);
    SetWindowPos( popup, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );

    GetWindowRect(popup, &rc);
    trace("popup window %p: %s\n", popup, wine_dbgstr_rect(&rc));

    x = rc.left + (rc.right - rc.left) / 2;
    y = rc.top + (rc.bottom - rc.top) / 2;
    trace("setting cursor to (%d,%d)\n", x, y);

    SetCursorPos(x, y);
    GetCursorPos(&pt);
    if (x != pt.x || y != pt.y)
    {
        skip( "failed to set mouse position, skipping mouse input tests\n" );
        goto done;
    }

    flush_events( TRUE );

    /* Check that setting the same position may generate WM_MOUSEMOVE */
    SetCursorPos(x, y);
    msg.message = 0;
    ret = peek_message(&msg);
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
    flush_events( FALSE );
    /* FIXME: SetCursorPos in Wine generates additional WM_MOUSEMOVE message */
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (ignore_message(msg.message)) continue;
        ok(msg.hwnd == popup && msg.message == WM_MOUSEMOVE,
           "hwnd %p message %04x\n", msg.hwnd, msg.message);
        DispatchMessageA(&msg);
    }
    ret = peek_message(&msg);
    ok( !ret, "message %04x available\n", msg.message);

    mouse_event(MOUSEEVENTF_MOVE, -1, -1, 0, 0);
    ShowWindow(popup, SW_HIDE);
    ret = wait_for_message( &msg );
    if (ret)
        ok(msg.hwnd == hwnd && msg.message == WM_MOUSEMOVE, "hwnd %p message %04x\n", msg.hwnd, msg.message);
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
    ShowWindow(hwnd, SW_HIDE);
    ret = wait_for_message( &msg );
    ok( !ret, "message %04x available\n", msg.message);
    flush_events( TRUE );

    /* test mouse clicks */

    ShowWindow(hwnd, SW_SHOW);
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    flush_events( TRUE );
    ShowWindow(popup, SW_SHOW);
    SetWindowPos( popup, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    ret = wait_for_message( &msg );
    if (!ret)
    {
        skip( "simulating mouse click doesn't work, skipping mouse button tests\n" );
        goto done;
    }
    if (msg.message == WM_MOUSEMOVE)  /* win2k has an extra WM_MOUSEMOVE here */
    {
        ret = wait_for_message( &msg );
        ok(ret, "no message available\n");
    }

    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDBLCLK, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);

    ret = peek_message(&msg);
    ok(!ret, "message %04x available\n", msg.message);

    ShowWindow(popup, SW_HIDE);
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, hwnd, msg.message);
    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == hwnd && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, hwnd, msg.message);

    test_lbuttondown_flag = TRUE;
    SendMessageA(hwnd, WM_COMMAND, (WPARAM)popup, 0);
    test_lbuttondown_flag = FALSE;

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(peek_message(&msg), "no message available\n");
    ok(msg.hwnd == popup && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, popup, msg.message);
    ok(peek_message(&msg), "no message available\n");

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

done:
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
    ok( ret == 1, "Expected GetUpdateRect to return non-zero, got %d\n", ret);
    ok( rc2.right > rc2.left && rc2.bottom > rc2.top,
            "Update rectangle is empty!\n");
    ValidateRect( hwnd, &rc);
    ret = GetUpdateRect( child, &rc2, 0);
    ok( !ret, "Expected GetUpdateRect to return zero, got %d\n", ret);
    ok( rc2.left == 0 && rc2.top == 0 && rc2.right == 0 && rc2.bottom == 0,
            "Update rectangle %s is not empty!\n", wine_dbgstr_rect(&rc2));

    /* now test ValidateRgn */
    InvalidateRect( child, NULL, 1);
    GetWindowRect( child, &rc);
    MapWindowPoints( NULL, hwnd, (POINT*) &rc, 2);
    rgn = CreateRectRgnIndirect( &rc);
    ValidateRgn( hwnd, rgn);
    ret = GetUpdateRect( child, &rc2, 0);
    ok( !ret, "Expected GetUpdateRect to return zero, got %d\n", ret);
    ok( rc2.left == 0 && rc2.top == 0 && rc2.right == 0 && rc2.bottom == 0,
            "Update rectangle %s is not empty!\n", wine_dbgstr_rect(&rc2));

    DeleteObject( rgn);
    DestroyWindow( child );
}

static void nccalchelper(HWND hwnd, INT x, INT y, RECT *prc)
{
    RECT rc;
    MoveWindow( hwnd, 0, 0, x, y, 0);
    GetWindowRect( hwnd, prc);
    rc = *prc;
    DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)prc);
    trace("window rect is %s, nccalc rect is %s\n", wine_dbgstr_rect(&rc), wine_dbgstr_rect(prc));
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
    ok( rc1.bottom - rc1.top == sbheight, "Height should be %d size is %s\n", sbheight,
            wine_dbgstr_rect(&rc1));

    /* test window just high enough for a horizontal scroll bar */
    nccalchelper( hwnd, 100, sbheight + 1, &rc1);
    ok( rc1.bottom - rc1.top == 1, "Height should be 1 size is %s\n", wine_dbgstr_rect(&rc1));

    /* test window too narrow for a vertical scroll bar */
    nccalchelper( hwnd, sbwidth - 1, 100, &rc1);
    ok( rc1.right - rc1.left == sbwidth - 1 , "Width should be %d size is %s\n", sbwidth - 1,
            wine_dbgstr_rect(&rc1));

    /* test window just wide enough for a vertical scroll bar */
    nccalchelper( hwnd, sbwidth, 100, &rc1);
    ok( rc1.right - rc1.left == 0, "Width should be 0 size is %s\n", wine_dbgstr_rect(&rc1));

    /* same test, but with client edge: not enough width */
    SetWindowLongA( hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE | GetWindowLongA( hwnd, GWL_EXSTYLE));
    nccalchelper( hwnd, sbwidth, 100, &rc1);
    ok( rc1.right - rc1.left == sbwidth - 2 * GetSystemMetrics(SM_CXEDGE),
            "Width should be %d size is %s\n", sbwidth - 2 * GetSystemMetrics(SM_CXEDGE),
            wine_dbgstr_rect(&rc1));

    DestroyWindow( hwnd);
}

static void test_SetParent(void)
{
    HWND desktop = GetDesktopWindow();
    HMENU hMenu;
    HWND ret, parent, child1, child2, child3, child4, sibling, popup;
    BOOL bret;

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
        ret = SetParent(parent, child2);
        todo_wine ok( !ret || broken( ret != 0 ), "SetParent should fail\n");
        if (ret)  /* nt4, win2k */
        {
            ret = SetParent(parent, child3);
            ok(ret != 0, "SetParent should not fail\n");
            ret = SetParent(child2, parent);
            ok(!ret, "SetParent should fail\n");
            ret = SetParent(parent, child4);
            ok(ret != 0, "SetParent should not fail\n");
            check_parents(parent, child4, child4, 0, 0, child4, parent);
            check_parents(child1, parent, parent, parent, 0, child4, parent);
            check_parents(child2, desktop, parent, parent, parent, child2, parent);
            check_parents(child3, child2, child2, child2, 0, child2, parent);
            check_parents(child4, desktop, child2, child2, child2, child4, parent);
        }
        else
        {
            ret = SetParent(parent, child3);
            ok(ret != 0, "SetParent should not fail\n");
            ret = SetParent(child2, parent);
            ok(!ret, "SetParent should fail\n");
            ret = SetParent(parent, child4);
            ok(!ret, "SetParent should fail\n");
            check_parents(parent, child3, child3, 0, 0, child2, parent);
            check_parents(child1, parent, parent, parent, 0, child2, parent);
            check_parents(child2, desktop, parent, parent, parent, child2, parent);
            check_parents(child3, child2, child2, child2, 0, child2, parent);
            check_parents(child4, desktop, child2, child2, child2, child4, parent);
        }
    }
    else
        skip("Win9x/WinMe crash\n");

    hMenu = CreateMenu();
    sibling = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
			     100, 100, 200, 200, 0, hMenu, 0, NULL);
    assert(sibling != 0);

    ok(SetParent(sibling, parent) != 0, "SetParent should not fail\n");
    ok(GetMenu(sibling) == hMenu, "SetParent should not remove menu\n");

    ok(SetParent(parent, desktop) != 0, "SetParent should not fail\n");
    ok(SetParent(child4, child3) != 0, "SetParent should not fail\n");
    ok(SetParent(child3, child2) != 0, "SetParent should not fail\n");
    ok(SetParent(child2, child1) != 0, "SetParent should not fail\n");
    ok(!IsChild(child3, child4), "wrong parent/child %p/%p\n", child3, child4);
    SetWindowLongW(child4, GWL_STYLE, WS_CHILD);
    ok(IsChild(child3, child4), "wrong parent/child %p/%p\n", child3, child4);
    ok(IsChild(child2, child4), "wrong parent/child %p/%p\n", child2, child4);
    ok(!IsChild(child1, child4), "wrong parent/child %p/%p\n", child1, child4);
    SetWindowLongW(child2, GWL_STYLE, WS_CHILD);
    ok(IsChild(child1, child4), "wrong parent/child %p/%p\n", child1, child4);
    ok(IsChild(parent, child4), "wrong parent/child %p/%p\n", parent, child4);

    ok(DestroyWindow(parent), "DestroyWindow() failed\n");

    ok(!IsWindow(parent), "parent still exists\n");
    ok(!IsWindow(sibling), "sibling still exists\n");
    ok(!IsWindow(child1), "child1 still exists\n");
    ok(!IsWindow(child2), "child2 still exists\n");
    ok(!IsWindow(child3), "child3 still exists\n");
    ok(!IsWindow(child4), "child4 still exists\n");

    parent = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
			     100, 100, 200, 200, 0, 0, 0, NULL);
    assert(parent != 0);
    child1 = CreateWindowExA(0, "static", NULL, WS_CHILD,
			     0, 0, 50, 50, parent, 0, 0, NULL);
    assert(child1 != 0);
    popup = CreateWindowExA(0, "static", NULL, WS_POPUP,
			     0, 0, 50, 50, 0, 0, 0, NULL);
    assert(popup != 0);

    trace("parent %p, child %p, popup %p\n", parent, child1, popup);

    check_parents(parent, desktop, 0, 0, 0, parent, parent);
    check_parents(child1, parent, parent, parent, 0, parent, parent);
    check_parents(popup, desktop, 0, 0, 0, popup, popup);

    SetActiveWindow(parent);
    SetFocus(parent);
    check_active_state(parent, 0, parent);

    ret = SetParent(popup, child1);
    ok(ret == desktop, "expected %p, got %p\n", desktop, ret);
    check_parents(popup, child1, child1, 0, 0, parent, popup);
    check_active_state(popup, 0, popup);

    SetActiveWindow(parent);
    SetFocus(popup);
    check_active_state(popup, 0, popup);

    EnableWindow(child1, FALSE);
    check_active_state(popup, 0, popup);
    SetFocus(parent);
    check_active_state(parent, 0, parent);
    SetFocus(popup);
    check_active_state(popup, 0, popup);
    EnableWindow(child1, TRUE);

    ShowWindow(child1, SW_MINIMIZE);
    SetFocus(parent);
    check_active_state(parent, 0, parent);
    SetFocus(popup);
    check_active_state(popup, 0, popup);
    ShowWindow(child1, SW_HIDE);

    SetActiveWindow(parent);
    SetFocus(parent);
    check_active_state(parent, 0, parent);

    bret = SetForegroundWindow(popup);
    ok(bret, "SetForegroundWindow() failed\n");
    check_active_state(popup, popup, popup);

    ShowWindow(parent, SW_SHOW);
    SetActiveWindow(popup);
    ok(DestroyWindow(popup), "DestroyWindow() failed\n");
    check_active_state(parent, parent, parent);

    ok(DestroyWindow(parent), "DestroyWindow() failed\n");

    ok(!IsWindow(parent), "parent still exists\n");
    ok(!IsWindow(child1), "child1 still exists\n");
    ok(!IsWindow(popup), "popup still exists\n");
}

static LRESULT WINAPI StyleCheckProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LPCREATESTRUCTA lpcs;
    LPSTYLESTRUCT lpss;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        lpcs = (LPCREATESTRUCTA)lparam;
        lpss = lpcs->lpCreateParams;
        if (lpss)
        {
            if ((lpcs->dwExStyle & WS_EX_DLGMODALFRAME) ||
                ((!(lpcs->dwExStyle & WS_EX_STATICEDGE)) &&
                    (lpcs->style & (WS_DLGFRAME | WS_THICKFRAME))))
                ok(lpcs->dwExStyle & WS_EX_WINDOWEDGE, "Window should have WS_EX_WINDOWEDGE style\n");
            else
                ok(!(lpcs->dwExStyle & WS_EX_WINDOWEDGE), "Window shouldn't have WS_EX_WINDOWEDGE style\n");

            ok((lpss->styleOld & ~WS_EX_WINDOWEDGE) == (lpcs->dwExStyle & ~WS_EX_WINDOWEDGE),
                "Ex style (0x%08x) should match what the caller passed to CreateWindowEx (0x%08x)\n",
                lpss->styleOld, lpcs->dwExStyle);

            ok(lpss->styleNew == lpcs->style,
                "Style (0x%08x) should match what the caller passed to CreateWindowEx (0x%08x)\n",
                lpss->styleNew, lpcs->style);
        }
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static ATOM atomStyleCheckClass;

static void register_style_check_class(void)
{
    WNDCLASSA wc =
    {
        0,
        StyleCheckProc,
        0,
        0,
        GetModuleHandleA(NULL),
        NULL,
        LoadCursorA(0, (LPCSTR)IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        "WineStyleCheck",
    };

    atomStyleCheckClass = RegisterClassA(&wc);
    assert(atomStyleCheckClass);
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
        hwndParent = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atomStyleCheckClass), NULL,
            WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    }

    hwnd = CreateWindowExA(dwExStyleIn, (LPCSTR)MAKEINTATOM(atomStyleCheckClass), NULL,
                    dwStyleIn, 0, 0, 0, 0, hwndParent, NULL, NULL, &ss);
    assert(hwnd);

    flush_events( TRUE );

    dwActualStyle = GetWindowLongA(hwnd, GWL_STYLE);
    dwActualExStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(dwActualStyle == dwStyleOut, "expected style %#x, got %#x\n", dwStyleOut, dwActualStyle);
    ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#x, got %#x\n", dwExStyleOut, dwActualExStyle);

    /* try setting the styles explicitly */
    SetWindowLongA( hwnd, GWL_EXSTYLE, dwExStyleIn );
    dwActualStyle = GetWindowLongA(hwnd, GWL_STYLE);
    dwActualExStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (dwExStyleIn & WS_EX_DLGMODALFRAME)
        dwExStyleOut = dwExStyleIn | WS_EX_WINDOWEDGE;
    else if ((dwActualStyle & (WS_DLGFRAME | WS_THICKFRAME)) && !(dwExStyleIn & WS_EX_STATICEDGE))
        dwExStyleOut = dwExStyleIn | WS_EX_WINDOWEDGE;
    else
        dwExStyleOut = dwExStyleIn & ~WS_EX_WINDOWEDGE;
    ok(dwActualStyle == dwStyleOut, "expected style %#x, got %#x\n", dwStyleOut, dwActualStyle);
    ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#x, got %#x\n", dwExStyleOut, dwActualExStyle);

    SetWindowLongA( hwnd, GWL_STYLE, dwStyleIn );
    dwActualStyle = GetWindowLongA(hwnd, GWL_STYLE);
    dwActualExStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    /* WS_CLIPSIBLINGS can't be reset on top-level windows */
    if ((dwStyleIn & (WS_CHILD|WS_POPUP)) == WS_CHILD) dwStyleOut = dwStyleIn;
    else dwStyleOut = dwStyleIn | WS_CLIPSIBLINGS;
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (dwExStyleIn & WS_EX_DLGMODALFRAME)
        dwExStyleOut = dwExStyleIn | WS_EX_WINDOWEDGE;
    else if ((dwActualStyle & (WS_DLGFRAME | WS_THICKFRAME)) && !(dwExStyleIn & WS_EX_STATICEDGE))
        dwExStyleOut = dwExStyleIn | WS_EX_WINDOWEDGE;
    else
        dwExStyleOut = dwExStyleIn & ~WS_EX_WINDOWEDGE;
    ok(dwActualStyle == dwStyleOut, "expected style %#x, got %#x\n", dwStyleOut, dwActualStyle);
    /* FIXME: Remove the condition below once Wine is fixed */
    todo_wine_if (dwActualExStyle != dwExStyleOut)
        ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#x, got %#x\n", dwExStyleOut, dwActualExStyle);

    DestroyWindow(hwnd);
    if (hwndParent) DestroyWindow(hwndParent);
}

/* tests what window styles the window manager automatically adds */
static void test_window_styles(void)
{
    register_style_check_class();

    check_window_style(0, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_window_style(WS_DLGFRAME, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_window_style(WS_THICKFRAME, 0, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_window_style(WS_DLGFRAME, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE);
    check_window_style(WS_THICKFRAME, WS_EX_STATICEDGE, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE);
    check_window_style(WS_OVERLAPPEDWINDOW, 0, WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD, 0, WS_CHILD, 0);
    check_window_style(WS_CHILD|WS_DLGFRAME, 0, WS_CHILD|WS_DLGFRAME, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD|WS_THICKFRAME, 0, WS_CHILD|WS_THICKFRAME, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE);
    check_window_style(WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE);
    check_window_style(WS_CHILD|WS_CAPTION, 0, WS_CHILD|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD|WS_CAPTION|WS_SYSMENU, 0, WS_CHILD|WS_CAPTION|WS_SYSMENU, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0);
    check_window_style(WS_CHILD, WS_EX_DLGMODALFRAME, WS_CHILD, WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_window_style(WS_CHILD, WS_EX_DLGMODALFRAME|WS_EX_STATICEDGE, WS_CHILD, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_window_style(WS_CHILD|WS_POPUP, 0, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_CHILD|WS_POPUP|WS_DLGFRAME, 0, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD|WS_POPUP|WS_THICKFRAME, 0, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_window_style(WS_CHILD|WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_window_style(WS_CHILD|WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_window_style(WS_CHILD|WS_POPUP, WS_EX_APPWINDOW, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, WS_EX_APPWINDOW);
    check_window_style(WS_CHILD|WS_POPUP, WS_EX_WINDOWEDGE, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0);
    check_window_style(0, WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW);
    check_window_style(WS_POPUP, 0, WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_POPUP, WS_EX_WINDOWEDGE, WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_window_style(WS_POPUP|WS_DLGFRAME, 0, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_window_style(WS_POPUP|WS_THICKFRAME, 0, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_window_style(WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_window_style(WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_window_style(WS_CAPTION, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE);
    check_window_style(0, WS_EX_APPWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE);

    if (pGetLayeredWindowAttributes)
    {
        check_window_style(0, WS_EX_LAYERED, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_WINDOWEDGE);
        check_window_style(0, WS_EX_LAYERED|WS_EX_TRANSPARENT, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_WINDOWEDGE);
        check_window_style(0, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION,
                                                      WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_WINDOWEDGE);
    }
}

static HWND root_dialog(HWND hwnd)
{
    while ((GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_CONTROLPARENT) &&
           (GetWindowLongA(hwnd, GWL_STYLE) & (WS_CHILD|WS_POPUP)) == WS_CHILD)
    {
        HWND parent = GetParent(hwnd);

        /* simple detector for a window being a dialog */
        if (!DefDlgProcA(parent, DM_GETDEFID, 0, 0))
            break;

        hwnd = parent;

        if (!(GetWindowLongA(hwnd, GWL_STYLE) & DS_CONTROL))
            break;
    }

    return hwnd;
}

static INT_PTR WINAPI empty_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return 0;
}

static LRESULT expected_id;

static INT_PTR WINAPI empty_dlg_proc3(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        HWND parent = GetParent(hwnd);
        LRESULT id, ret;

        id = DefDlgProcA(parent, DM_GETDEFID, 0, 0);
        if (!id || root_dialog(hwnd) == hwnd)
            parent = 0;

        id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
        if (!parent)
            ok(id == MAKELONG(IDOK,DC_HASDEFID), "expected (IDOK,DC_HASDEFID), got %08lx\n", id);
        else
            ok(id == expected_id, "expected %08lx, got %08lx\n", expected_id, id);

        ret = DefDlgProcA(hwnd, DM_SETDEFID, 0x3333, 0);
        ok(ret, "DefDlgProc(DM_SETDEFID) failed\n");
        id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
        ok(id == MAKELONG(0x3333,DC_HASDEFID), "expected (0x3333,DC_HASDEFID), got %08lx\n", id);

        if (parent)
        {
            id = DefDlgProcA(parent, DM_GETDEFID, 0, 0);
            ok(id == MAKELONG(0x3333,DC_HASDEFID), "expected (0x3333,DC_HASDEFID), got %08lx\n", id);

            expected_id = MAKELONG(0x3333,DC_HASDEFID);
        }

        EndDialog(hwnd, 0);
    }

    return 0;
}

struct dialog_param
{
    HWND parent, grand_parent;
    DLGTEMPLATE *dlg_data;
};

static INT_PTR WINAPI empty_dlg_proc2(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        DWORD style = GetWindowLongA(hwnd, GWL_STYLE);
        struct dialog_param *param = (struct dialog_param *)lparam;
        BOOL parent_is_child;
        HWND disabled_hwnd;
        LRESULT id, ret;

        id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
        ok(id == MAKELONG(IDOK,DC_HASDEFID), "expected (IDOK,DC_HASDEFID), got %08lx\n", id);
        ret = DefDlgProcA(hwnd, DM_SETDEFID, 0x2222, 0);
        ok(ret, "DefDlgProc(DM_SETDEFID) failed\n");
        id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
        ok(id == MAKELONG(0x2222,DC_HASDEFID), "expected (0x2222,DC_HASDEFID), got %08lx\n", id);

        expected_id = MAKELONG(0x2222,DC_HASDEFID);

        parent_is_child = (GetWindowLongA(param->parent, GWL_STYLE) & (WS_POPUP | WS_CHILD)) == WS_CHILD;

        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        if (parent_is_child)
        {
            ok(IsWindowEnabled(param->parent), "wrong state for %08x\n", style);
            disabled_hwnd = param->grand_parent;
        }
        else
        {
            ok(!IsWindowEnabled(param->parent), "wrong state for %08x\n", style);
            disabled_hwnd = param->parent;
        }

        if (param->grand_parent)
        {
            if (parent_is_child)
                ok(!IsWindowEnabled(param->grand_parent), "wrong state for %08x\n", style);
            else
                ok(IsWindowEnabled(param->grand_parent), "wrong state for %08x\n", style);
        }

        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, disabled_hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(disabled_hwnd), "wrong state for %08x\n", style);

        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        ok(IsWindowEnabled(param->parent), "wrong state for %p\n", param->parent);
        if (param->grand_parent)
            ok(IsWindowEnabled(param->grand_parent), "wrong state for %p (%08x)\n", param->grand_parent, style);

        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        ok(IsWindowEnabled(param->parent), "wrong state for %p\n", param->parent);
        if (param->grand_parent)
            ok(IsWindowEnabled(param->grand_parent), "wrong state for %p (%08x)\n", param->grand_parent, style);

        param->dlg_data->style |= WS_CHILD;
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08x)\n", hwnd, style);

        param->dlg_data->style |= DS_CONTROL;
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08x)\n", hwnd, style);

        param->dlg_data->dwExtendedStyle |= WS_EX_CONTROLPARENT;
        SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
        SetWindowLongA(hwnd, GWL_STYLE, style & ~DS_CONTROL);
        param->dlg_data->style &= ~DS_CONTROL;
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08x)\n", hwnd, style);

        SetWindowLongA(hwnd, GWL_STYLE, style | DS_CONTROL);
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08x)\n", hwnd, style);

        param->dlg_data->style |= DS_CONTROL;
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08x)\n", hwnd, style);

        EndDialog(hwnd, 0);
    }
    return 0;
}

static void check_dialog_style(DWORD style_in, DWORD ex_style_in, DWORD style_out, DWORD ex_style_out)
{
    struct
    {
        DLGTEMPLATE dt;
        WORD menu_name;
        WORD class_id;
        WORD class_atom;
        WCHAR caption[1];
    } dlg_data;
    DWORD style, ex_style;
    HWND hwnd, grand_parent = 0, parent = 0;
    struct dialog_param param;
    LRESULT id, ret;

    if (style_in & WS_CHILD)
    {
        grand_parent = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
                                0, 0, 0, 0, NULL, NULL, NULL, NULL);
        ok(grand_parent != 0, "grand_parent creation failed\n");
    }

    parent = CreateWindowExA(0, "static", NULL, style_in,
                             0, 0, 0, 0, grand_parent, NULL, NULL, NULL);
    ok(parent != 0, "parent creation failed, style %#x\n", style_in);

    dlg_data.dt.style = style_in;
    dlg_data.dt.dwExtendedStyle = ex_style_in;
    dlg_data.dt.cdit = 0;
    dlg_data.dt.x = 0;
    dlg_data.dt.y = 0;
    dlg_data.dt.cx = 100;
    dlg_data.dt.cy = 100;
    dlg_data.menu_name = 0;
    dlg_data.class_id = 0;
    dlg_data.class_atom = 0;
    dlg_data.caption[0] = 0;

    hwnd = CreateDialogIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, parent, empty_dlg_proc, 0);
    ok(hwnd != 0, "dialog creation failed, style %#x, exstyle %#x\n", style_in, ex_style_in);

    id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
    ok(id == MAKELONG(IDOK,DC_HASDEFID), "expected (IDOK,DC_HASDEFID), got %08lx\n", id);
    ret = DefDlgProcA(hwnd, DM_SETDEFID, 0x1111, 0);
    ok(ret, "DefDlgProc(DM_SETDEFID) failed\n");
    id = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
    ok(id == MAKELONG(0x1111,DC_HASDEFID), "expected (0x1111,DC_HASDEFID), got %08lx\n", id);

    flush_events( TRUE );

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (style_out | DS_3DLOOK), "got %#x\n", style);
    ok(ex_style == ex_style_out, "expected ex_style %#x, got %#x\n", ex_style_out, ex_style);

    ok(IsWindowEnabled(parent), "wrong parent state (dialog style %#x)\n", style_in);

    /* try setting the styles explicitly */
    SetWindowLongA(hwnd, GWL_EXSTYLE, ex_style_in);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (style_out | DS_3DLOOK), "got %#x\n", style);
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (ex_style_in & WS_EX_DLGMODALFRAME)
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else if ((style & (WS_DLGFRAME | WS_THICKFRAME)) && !(ex_style_in & WS_EX_STATICEDGE))
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else
        ex_style_out = ex_style_in & ~WS_EX_WINDOWEDGE;
    ok(ex_style == ex_style_out, "expected ex_style %#x, got %#x\n", ex_style_out, ex_style);

    SetWindowLongA(hwnd, GWL_STYLE, style_in);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    /* WS_CLIPSIBLINGS can't be reset on top-level windows */
    if ((style_in & (WS_CHILD | WS_POPUP)) == WS_CHILD) style_out = style_in;
    else style_out = style_in | WS_CLIPSIBLINGS;
    ok(style == style_out, "expected style %#x, got %#x\n", style_out, style);
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (ex_style_in & WS_EX_DLGMODALFRAME)
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else if ((style & (WS_DLGFRAME | WS_THICKFRAME)) && !(ex_style_in & WS_EX_STATICEDGE))
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else
        ex_style_out = ex_style_in & ~WS_EX_WINDOWEDGE;
    /* FIXME: Remove the condition below once Wine is fixed */
    todo_wine_if (ex_style != ex_style_out)
        ok(ex_style == ex_style_out, "expected ex_style %#x, got %#x\n", ex_style_out, ex_style);

    DestroyWindow(hwnd);

    param.parent = parent;
    param.grand_parent = grand_parent;
    param.dlg_data = &dlg_data.dt;
    DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, parent, empty_dlg_proc2, (LPARAM)&param);

    ok(IsWindowEnabled(parent), "wrong parent state (dialog style %#x)\n", style_in);
    if (grand_parent)
        ok(IsWindowEnabled(grand_parent), "wrong grand parent state (dialog style %#x)\n", style_in);

    DestroyWindow(parent);
    DestroyWindow(grand_parent);
}

static void test_dialog_styles(void)
{
    check_dialog_style(0, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_DLGFRAME, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_THICKFRAME, 0, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_DLGFRAME, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_THICKFRAME, WS_EX_STATICEDGE, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(DS_CONTROL, 0, WS_CLIPSIBLINGS|WS_CAPTION|DS_CONTROL, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CAPTION, 0, WS_CAPTION|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_BORDER, 0, WS_CAPTION|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_DLGFRAME, 0, WS_CAPTION|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_BORDER|DS_CONTROL, 0, WS_CAPTION|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_DLGFRAME|DS_CONTROL, 0, WS_CAPTION|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CAPTION|WS_SYSMENU, 0, WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_SYSMENU, 0, WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CAPTION|DS_CONTROL, 0, WS_CAPTION|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_SYSMENU|DS_CONTROL, 0, WS_CAPTION|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CAPTION|WS_SYSMENU|DS_CONTROL, 0, WS_CAPTION|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_OVERLAPPEDWINDOW, 0, WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD, 0, WS_CHILD, 0);
    check_dialog_style(WS_CHILD|WS_DLGFRAME, 0, WS_CHILD|WS_DLGFRAME, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_THICKFRAME, 0, WS_CHILD|WS_THICKFRAME, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE);
    check_dialog_style(WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE);
    check_dialog_style(WS_CHILD|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_CAPTION, 0, WS_CHILD|WS_CAPTION, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_BORDER, 0, WS_CHILD|WS_BORDER, 0);
    check_dialog_style(WS_CHILD|WS_DLGFRAME, 0, WS_CHILD|WS_DLGFRAME, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_BORDER|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_DLGFRAME|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_CAPTION|WS_SYSMENU, 0, WS_CHILD|WS_CAPTION|WS_SYSMENU, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_SYSMENU, 0, WS_CHILD|WS_SYSMENU, 0);
    check_dialog_style(WS_CHILD|WS_CAPTION|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_SYSMENU|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_CAPTION|WS_SYSMENU|DS_CONTROL, 0, WS_CHILD|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0);
    check_dialog_style(WS_CHILD, WS_EX_DLGMODALFRAME, WS_CHILD, WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_dialog_style(WS_CHILD, WS_EX_DLGMODALFRAME|WS_EX_STATICEDGE, WS_CHILD, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME);
    check_dialog_style(WS_CHILD|WS_POPUP, 0, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_DLGFRAME, 0, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_THICKFRAME, 0, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|DS_CONTROL, 0, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_CAPTION, 0, WS_CHILD|WS_POPUP|WS_CAPTION|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_BORDER, 0, WS_CHILD|WS_POPUP|WS_BORDER|WS_CLIPSIBLINGS, 0);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_DLGFRAME, 0, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_BORDER|DS_CONTROL, 0, WS_CHILD|WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_DLGFRAME|DS_CONTROL, 0, WS_CHILD|WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_CAPTION|WS_SYSMENU, 0, WS_CHILD|WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_SYSMENU, 0, WS_CHILD|WS_POPUP|WS_SYSMENU|WS_CLIPSIBLINGS, 0);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_CAPTION|DS_CONTROL, 0, WS_CHILD|WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_SYSMENU|DS_CONTROL, 0, WS_CHILD|WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_CONTROL, 0, WS_CHILD|WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CHILD|WS_POPUP, WS_EX_APPWINDOW, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, WS_EX_APPWINDOW);
    check_dialog_style(WS_CHILD|WS_POPUP, WS_EX_WINDOWEDGE, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0);
    check_dialog_style(WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0);
    check_dialog_style(0, WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP, 0, WS_POPUP|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP, WS_EX_WINDOWEDGE, WS_POPUP|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_DLGFRAME, 0, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_THICKFRAME, 0, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|DS_CONTROL, 0, WS_POPUP|WS_CLIPSIBLINGS|DS_CONTROL, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_CAPTION, 0, WS_POPUP|WS_CAPTION|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_BORDER, 0, WS_POPUP|WS_BORDER|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_DLGFRAME, 0, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_BORDER|DS_CONTROL, 0, WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_DLGFRAME|DS_CONTROL, 0, WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_CAPTION|WS_SYSMENU, 0, WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_SYSMENU, 0, WS_POPUP|WS_SYSMENU|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_CAPTION|DS_CONTROL, 0, WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_SYSMENU|DS_CONTROL, 0, WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_CONTROL, 0, WS_POPUP|DS_CONTROL|WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT);
    check_dialog_style(WS_CAPTION, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    check_dialog_style(0, WS_EX_APPWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);

    if (pGetLayeredWindowAttributes)
    {
        check_dialog_style(0, WS_EX_LAYERED, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
        check_dialog_style(0, WS_EX_LAYERED|WS_EX_TRANSPARENT, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
        check_dialog_style(0, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION,
                              WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT);
    }
}

struct dlg_parent_param
{
    HWND ga_parent;
    HWND gwl_parent;
    HWND get_parent;
    HWND owner;
    HWND root;
    HWND ga_root_owner;
};

static INT_PTR WINAPI parent_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG) {
        struct dlg_parent_param *param = (void*)lparam;
        check_parents(hwnd, param->ga_parent, param->gwl_parent, param->get_parent, param->owner,
                      param->root ? param->root : hwnd, param->ga_root_owner ? param->ga_root_owner : hwnd);

        ok(!IsWindowEnabled(param->gwl_parent), "parent is not disabled\n");
        EndDialog(hwnd, 2);
        ok(IsWindowEnabled(param->gwl_parent), "parent is not enabled\n");
    }

    return 0;
}

static INT_PTR WINAPI reparent_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG) {
        ok(!IsWindowEnabled(GetParent(hwnd)), "parent is not disabled\n");
        SetParent(hwnd, (HWND)lparam);
    }

    return 0;
}

static INT_PTR WINAPI reparent_owned_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG) {
        HWND new_parent = (HWND)lparam;
        HWND owner = GetWindow(hwnd, GW_OWNER);
        ok(!IsWindowEnabled(owner), "owner is not disabled\n");
        SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) | WS_CHILD);
        SetParent(hwnd, new_parent);
        ok(GetParent(hwnd) == new_parent, "GetParent(hwnd) = %p, expected %p\n", GetParent(hwnd), new_parent);
        PostMessageA(hwnd, WM_QUIT, 0, 0);
    }

    return 0;
}

static LRESULT WINAPI reparent_dialog_owner_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ENTERIDLE) {
        HWND dialog = (HWND)lparam;
        HWND owner = GetParent(dialog);
        /* EndDialog will enable owner */
        EnableWindow(owner, FALSE);
        EndDialog(dialog, 2);
        ok(IsWindowEnabled(owner), "owner is not enabled\n");
        /* ...but it won't be enabled on dialog exit */
        EnableWindow(owner, FALSE);
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static LRESULT WINAPI post_quit_dialog_owner_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ENTERIDLE) {
        HWND dialog = (HWND)lparam;
        PostMessageA(dialog, WM_QUIT, 0, 0);
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static LRESULT WINAPI destroy_dialog_owner_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ENTERIDLE) {
        HWND dialog = (HWND)lparam;
        DestroyWindow(dialog);
    }
    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

static void test_dialog_parent(void)
{
    HWND dialog, parent, child, child2, other, desktop = GetDesktopWindow();
    struct dlg_parent_param param;
    INT_PTR ret;
    struct
    {
        DLGTEMPLATE dt;
        WORD menu_name;
        WORD class_id;
        WORD class_atom;
        WCHAR caption[1];
    } dlg_data;

    dlg_data.dt.dwExtendedStyle = 0;
    dlg_data.dt.cdit = 0;
    dlg_data.dt.x = 0;
    dlg_data.dt.y = 0;
    dlg_data.dt.cx = 100;
    dlg_data.dt.cy = 100;
    dlg_data.menu_name = 0;
    dlg_data.class_id = 0;
    dlg_data.class_atom = 0;
    dlg_data.caption[0] = 0;

    parent = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    /* Create a child without WS_CHILD flag. It's a valid owner window. */
    child = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetParent(child, parent);
    /* Regular child. If passed as an owner, its parent will be true owner window. */
    child2 = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 0, 0, child, NULL, NULL, NULL);

    trace("parent %p child %p child2 %p desktop %p\n", parent, child, child2, desktop);

    /* When dialog is created with WS_CHILD style, its parent depends on function used to create it. */
    dlg_data.dt.style = WS_CHILD;

    /* CreateDialogIndirectParam uses passed parent as dialog parent. */
    dialog = CreateDialogIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, empty_dlg_proc, 0);
    ok(dialog != 0, "dialog creation failed\n");
    check_parents(dialog, child2, child2, child2, NULL, parent, child);

    ok(IsWindowEnabled(child2), "child2 is disabled\n");
    EnableWindow(child2, FALSE);
    EndDialog(dialog, 0);
    ok(IsWindowEnabled(child2), "child2 is not enabled\n");
    DestroyWindow(dialog);

    /* DialogBoxIndirectParam uses the first parent of passed owner that's not a child window as dialog
     * parent (like in case of dialog with owner). */
    param.ga_parent = param.gwl_parent = param.get_parent = child;
    param.owner = NULL;
    param.root = parent;
    param.ga_root_owner = child;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, parent_dlg_proc, (LPARAM)&param);
    ok(ret == 2, "DialogBoxIndirectParam returned %ld\n", ret);

    /* Dialogs without WS_CHILD behave as expected, they use passed owner just like CreateWindow does. */
    dlg_data.dt.style = WS_OVERLAPPEDWINDOW;

    dialog = CreateDialogIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, empty_dlg_proc, 0);
    ok(dialog != 0, "dialog creation failed\n");
    check_parents(dialog, desktop, child, NULL, child, dialog, dialog);

    ok(IsWindowEnabled(child), "child is disabled\n");
    EnableWindow(child, FALSE);
    EndDialog(dialog, 0);
    ok(IsWindowEnabled(child), "child is not enabled\n");
    DestroyWindow(dialog);

    param.ga_parent = desktop;
    param.gwl_parent = child;
    param.get_parent = NULL;
    param.owner = child;
    param.root = param.ga_root_owner = NULL;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, parent_dlg_proc, (LPARAM)&param);
    ok(ret == 2, "DialogBoxIndirectParam returned %ld\n", ret);

    other = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetWindowLongPtrA(child, GWLP_WNDPROC, (ULONG_PTR)reparent_dialog_owner_proc);

    /* When dialog is created with WS_CHILD|WS_POPUP style, we have an owner. */
    dlg_data.dt.style = WS_CHILD|WS_POPUP;

    dialog = CreateDialogIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, empty_dlg_proc, 0);
    ok(dialog != 0, "dialog creation failed\n");
    check_parents(dialog, desktop, child, child, child, dialog, child);

    ok(IsWindowEnabled(child), "child is disabled\n");
    EnableWindow(child, FALSE);
    EndDialog(dialog, 0);
    ok(IsWindowEnabled(child), "child is not enabled\n");
    DestroyWindow(dialog);

    param.ga_parent = desktop;
    param.gwl_parent = param.get_parent = child;
    param.owner = child;
    param.root = NULL;
    param.ga_root_owner = child;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, parent_dlg_proc, (LPARAM)&param);
    ok(ret == 2, "DialogBoxIndirectParam returned %ld\n", ret);

    /* If we change parent in WM_INITDIALOG for WS_CHILD dialog WM_ENTERIDLE is still sent to the original
     * parent. EndDialog will enable the new parent. */
    EnableWindow(child, TRUE);
    EnableWindow(other, FALSE);
    dlg_data.dt.style = WS_CHILD;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, reparent_dlg_proc, (LPARAM)other);
    ok(ret == 2, "DialogBoxIndirectParam returned %ld\n", ret);
    ok(!IsWindowEnabled(other), "other is not disabled\n");
    ok(!IsWindowEnabled(child), "child is not disabled\n");
    ok(IsWindowEnabled(child2), "child2 is not enabled\n");
    EnableWindow(child, TRUE);

    /* If we change parent and style in WM_INITDIALOG for dialog with an owner to make it true child
     * (thus GetParent() will return the new parent instead of an owner), WM_ENTERIDLE is still sent
     * to the original parent. EndDialog will enable the new parent. */
    EnableWindow(other, FALSE);
    dlg_data.dt.style = WS_OVERLAPPED;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, reparent_owned_dlg_proc, (LPARAM)other);
    ok(ret == 1, "DialogBoxIndirectParam returned %ld\n", ret);
    ok(!IsWindowEnabled(other), "other is not disabled\n");
    ok(!IsWindowEnabled(child), "child is not disabled\n");
    ok(IsWindowEnabled(child2), "child2 is not enabled\n");
    EnableWindow(child, TRUE);
    EnableWindow(other, TRUE);

    /* Quit dialog message loop by sending WM_QUIT message. Dialog owner is not enabled. */
    SetWindowLongPtrA(child, GWLP_WNDPROC, (ULONG_PTR)post_quit_dialog_owner_proc);
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, other, empty_dlg_proc, 0);
    ok(ret == 1, "DialogBoxIndirectParam returned %ld\n", ret);
    ok(!IsWindowEnabled(other), "other is enabled\n");
    EnableWindow(other, TRUE);

    /* Quit dialog message loop by destroying the window. Dialog owner is not enabled. */
    SetWindowLongPtrA(child, GWLP_WNDPROC, (ULONG_PTR)destroy_dialog_owner_proc);
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, other, empty_dlg_proc, 0);
    ok(ret == 1, "DialogBoxIndirectParam returned %ld\n", ret);
    ok(!IsWindowEnabled(other), "other is enabled\n");
    EnableWindow(other, TRUE);

    DestroyWindow(parent);
}

static void test_scrollwindow( HWND hwnd)
{
    HDC hdc;
    RECT rc, rc2, rc3;
    COLORREF colr;

    ShowWindow( hwnd, SW_SHOW);
    UpdateWindow( hwnd);
    flush_events( TRUE );
    GetClientRect( hwnd, &rc);
    hdc = GetDC( hwnd);
    /* test ScrollWindow(Ex) with no clip rectangle */
    /* paint the lower half of the window black */
    rc2 = rc;
    rc2.top = ( rc2.top + rc2.bottom) / 2;
    FillRect( hdc, &rc2, GetStockObject(BLACK_BRUSH));
    /* paint the upper half of the window white */
    rc2.bottom = rc2.top;
    rc2.top =0;
    FillRect( hdc, &rc2, GetStockObject(WHITE_BRUSH));
    /* scroll lower half up */
    rc2 = rc;
    rc2.top = ( rc2.top + rc2.bottom) / 2;
    ScrollWindowEx( hwnd, 0, - rc2.top, &rc2, NULL, NULL, NULL, SW_ERASE);
    flush_events(FALSE);
    /* expected: black should have scrolled to the upper half */
    colr = GetPixel( hdc, (rc2.left+rc2.right)/ 2,  rc2.bottom / 4 );
    ok ( colr == 0, "pixel should be black, color is %08x\n", colr);
    /* Repeat that test of ScrollWindow(Ex) now with clip rectangle */
    /* paint the lower half of the window black */
    rc2 = rc;
    rc2.top = ( rc2.top + rc2.bottom) / 2;
    FillRect( hdc, &rc2, GetStockObject(BLACK_BRUSH));
    /* paint the upper half of the window white */
    rc2.bottom = rc2.top;
    rc2.top =0;
    FillRect( hdc, &rc2, GetStockObject(WHITE_BRUSH));
    /* scroll lower half up */
    rc2 = rc;
    rc2.top = ( rc2.top + rc2.bottom) / 2;
    rc3 = rc;
    rc3.left = rc3.right / 4;
    rc3.right -= rc3.right / 4;
    ScrollWindowEx( hwnd, 0, - rc2.top, &rc2, &rc3, NULL, NULL, SW_ERASE);
    flush_events(FALSE);
    /* expected: black should have scrolled to the upper half */
    colr = GetPixel( hdc, (rc2.left+rc2.right)/ 2,  rc2.bottom / 4 );
    ok ( colr == 0, "pixel should be black, color is %08x\n", colr);

    /* clean up */
    ReleaseDC( hwnd, hdc);
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
    TextOutA( hdc, 0,10, "0123456789", 10);
    ScrollDC( hdc, -10, -5, &rc, &cliprc, hrgn, &rcu);
    if (winetest_debug > 0) dump_region(hrgn);
    /* create a region with what is expected */
    SetRectRgn( exprgn, 39,0,49,74);
    SetRectRgn( tmprgn, 88,79,98,93);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 0,93,98,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    trace("update rect is %s\n", wine_dbgstr_rect(&rcu));
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
    trace("update rect is %s\n", wine_dbgstr_rect(&rcu));
    ReleaseDC( hwnd1, hdc);

    /* test scrolling a rect by more than its size */
    DestroyWindow( hwnd2);
    ValidateRect( hwnd1, NULL);
    SetRect( &rc, 40,40, 50,50);
    InvalidateRect( hwnd1, &rc, 1);
    ScrollWindowEx( hwnd1, -20, 0, &rc, NULL, hrgn, &rcu,
      SW_SCROLLCHILDREN | SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    SetRectRgn( exprgn, 20, 40, 30, 50);
    SetRectRgn( tmprgn, 40, 40, 50, 50);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    ok( rcu.left == 20 && rcu.top == 40 && rcu.right == 50 && rcu.bottom == 50,
        "unexpected update rect: %s\n", wine_dbgstr_rect(&rcu));

    /* test scrolling a window with an update region */
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
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) & ~WS_CLIPSIBLINGS);
    GetClientRect( hwnd1, &rc);
    cliprc=rc;

    /* WS_CLIPCHILDREN and SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) | WS_CLIPCHILDREN );
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
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_SCROLLCHILDREN | SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* no SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    if (winetest_debug > 0) dump_region(hrgn);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");

    /* WS_CLIPCHILDREN and no SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) | WS_CLIPCHILDREN );
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
    trace("update rect: %s\n", wine_dbgstr_rect(&rcu));
    if (winetest_debug > 0) dump_region(hrgn);
    SetRect(&rc2, 0, 0, 100, 100);
    ok(EqualRect(&rcu, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rcu),
       wine_dbgstr_rect(&rc2));

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
    trace("update rect: %s\n", wine_dbgstr_rect(&rcu));
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

    ok(!IsWindow(0), "IsWindow(0)\n");
    ok(!IsWindow(HWND_BROADCAST), "IsWindow(HWND_BROADCAST)\n");
    ok(!IsWindow(HWND_TOPMOST), "IsWindow(HWND_TOPMOST)\n");

    /* Just a param check */
    if (pGetMonitorInfoA)
    {
        SetLastError(0xdeadbeef);
        rc = GetWindowTextA(hwndMain2, NULL, 1024);
        ok( rc==0, "GetWindowText: rc=%d err=%d\n",rc,GetLastError());
    }
    else
    {
        /* Skips actually on Win95 and NT4 */
        win_skip("Test would crash on Win95\n");
    }

    SetLastError(0xdeadbeef);
    hwnd=CreateWindowA("LISTBOX", "TestList",
                      (LBS_STANDARD & ~LBS_SORT),
                      0, 0, 100, 100,
                      NULL, (HMENU)1, NULL, 0);

    ok(!hwnd || broken(hwnd != NULL), /* w2k3 sp2 */
       "CreateWindow with invalid menu handle should fail\n");
    if (!hwnd)
        ok(GetLastError() == ERROR_INVALID_MENU_HANDLE || /* NT */
           GetLastError() == 0xdeadbeef, /* Win9x */
           "wrong last error value %d\n", GetLastError());
}

static void test_AWRwindow(LPCSTR class, LONG style, LONG exStyle, BOOL menu)
{
    HWND hwnd = 0;

    hwnd = CreateWindowExA(exStyle, class, class, style,
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
    WNDCLASSA class;

    class.style         = CS_HREDRAW | CS_VREDRAW;
    class.lpfnWndProc     = DefWindowProcA;
    class.cbClsExtra    = 0;
    class.cbWndExtra    = 0;
    class.hInstance     = 0;
    class.hIcon         = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    class.hCursor       = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    class.hbrBackground = 0;
    class.lpszMenuName  = 0;
    class.lpszClassName = szAWRClass;

    if (!RegisterClassA(&class)) {
	ok(FALSE, "RegisterClass failed\n");
	return FALSE;
    }

    hmenu = CreateMenu();
    if (!hmenu)
	return FALSE;
    ok(hmenu != 0, "Failed to create menu\n");
    ok(AppendMenuA(hmenu, MF_STRING, 1, "Test!"), "Failed to create menu item\n");

    return TRUE;
}


static void test_AWR_window_size(BOOL menu)
{
    static const DWORD styles[] = {
        WS_POPUP, WS_MAXIMIZE, WS_BORDER, WS_DLGFRAME, WS_CAPTION, WS_SYSMENU,
        WS_THICKFRAME, WS_MINIMIZEBOX, WS_MAXIMIZEBOX, WS_HSCROLL, WS_VSCROLL
    };
    static const DWORD exStyles[] = {
	WS_EX_CLIENTEDGE, WS_EX_TOOLWINDOW, WS_EX_WINDOWEDGE, WS_EX_APPWINDOW,
	WS_EX_DLGMODALFRAME, WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE
    };

    unsigned int i;

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

static void test_AWR_flags(void)
{
    static const DWORD styles[] = { WS_POPUP, WS_BORDER, WS_DLGFRAME, WS_THICKFRAME };
    static const DWORD exStyles[] = { WS_EX_CLIENTEDGE, WS_EX_TOOLWINDOW, WS_EX_WINDOWEDGE,
                                      WS_EX_APPWINDOW, WS_EX_DLGMODALFRAME, WS_EX_STATICEDGE };

    DWORD i, j, k, style, exstyle;
    RECT rect, rect2;

    for (i = 0; i < (1 << COUNTOF(styles)); i++)
    {
        for (k = style = 0; k < COUNTOF(styles); k++) if (i & (1 << k)) style |= styles[k];

        for (j = 0; j < (1 << COUNTOF(exStyles)); j++)
        {
            for (k = exstyle = 0; k < COUNTOF(exStyles); k++) if (j & (1 << k)) exstyle |= exStyles[k];
            SetRect( &rect, 100, 100, 200, 200 );
            rect2 = rect;
            AdjustWindowRectEx( &rect, style, FALSE, exstyle );
            wine_AdjustWindowRectEx( &rect2, style, FALSE, exstyle );
            ok( EqualRect( &rect, &rect2 ), "rects do not match: win %s wine %s\n",
                wine_dbgstr_rect( &rect ), wine_dbgstr_rect( &rect2 ));
        }
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
    test_AWR_flags();

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
    return DefWindowProcA(hwnd, msg, wparam, lparam);
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
   cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
   ok( WMPAINT_count == 1 || broken(WMPAINT_count == 0), /* sometimes on win9x */
       "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
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
        GetClientRect(hwnd, &rc);
        t->client = rc;
        GetWindowRect(hwnd, &rc);
        trace("WM_PAINT: hwnd %p, client rect %s, window rect %s\n", hwnd,
              wine_dbgstr_rect(&t->client), wine_dbgstr_rect(&rc));
        BeginPaint(hwnd, &ps);
        t->paint = ps.rcPaint;
        GetClipBox(ps.hdc, &rc);
        t->clip = rc;
        trace("clip rect %s, paint rect %s\n", wine_dbgstr_rect(&rc),
              wine_dbgstr_rect(&ps.rcPaint));
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
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
  todo_wine_if (t.w##_todo.r.f) \
      parentdc_field_ok(t, w, r, f, got);

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
   clsMain.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
   cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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
   SetWindowPos(hwndMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
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
    ATOM atom;

    memset(&classW, 0, sizeof(classW));
    classW.hInstance = GetModuleHandleA(0);
    classW.lpfnWndProc = def_window_procW;
    classW.lpszClassName = unicode_class_nameW;
    if (!RegisterClassW(&classW)) return; /* this catches Win9x as well */

    memset(&classA, 0, sizeof(classA));
    classA.hInstance = GetModuleHandleA(0);
    classA.lpfnWndProc = def_window_procA;
    classA.lpszClassName = ansi_class_nameA;
    atom = RegisterClassA(&classA);
    assert(atom);

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
        return DefWindowProcA(hwnd, msg, wp, lp);

    minmax = (MINMAXINFO *)lp;

    if ((GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD))
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
        DefWindowProcA(hwnd, msg, wp, lp);
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
        ok( !rect.left && !rect.top && !rect.right && !rect.bottom, "wrong rect %s\n",
            wine_dbgstr_rect( &rect ));
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
        RECT rect;
        GetWindowRect( hwnd, &rect );
        trace( "hwnd %p msg %x size %dx%d rect %s\n", hwnd, msg, cs->cx, cs->cy,
               wine_dbgstr_rect( &rect ));
        ok( cs->cx == expected_cx || broken(cs->cx == (short)expected_cx),
            "wrong x size %d/%d\n", cs->cx, expected_cx );
        ok( cs->cy == expected_cy || broken(cs->cy == (short)expected_cy),
            "wrong y size %d/%d\n", cs->cy, expected_cy );
        ok( (rect.right - rect.left == expected_rect.right - expected_rect.left &&
             rect.bottom - rect.top == expected_rect.bottom - expected_rect.top) ||
            (rect.right - rect.left == min( 65535, expected_rect.right - expected_rect.left ) &&
             rect.bottom - rect.top == min( 65535, expected_rect.bottom - expected_rect.top )) ||
            broken( rect.right - rect.left == broken_rect.right - broken_rect.left &&
                    rect.bottom - rect.top == broken_rect.bottom - broken_rect.top) ||
            broken( rect.right - rect.left == (short)broken_rect.right - (short)broken_rect.left &&
                    rect.bottom - rect.top == (short)broken_rect.bottom - (short)broken_rect.top),
            "wrong rect %s / %s\n", wine_dbgstr_rect( &rect ), wine_dbgstr_rect( &expected_rect ));
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    case WM_NCCALCSIZE:
    {
        RECT rect, *r = (RECT *)lp;
        GetWindowRect( hwnd, &rect );
        ok( EqualRect( &rect, r ), "passed rect %s doesn't match window rect %s\n",
            wine_dbgstr_rect( r ), wine_dbgstr_rect( &rect ));
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

static void test_CreateWindow(void)
{
    WNDCLASSA cls;
    HWND hwnd, parent;
    HMENU hmenu;
    RECT rc, rc_minmax;
    MINMAXINFO minmax;
    BOOL res;

#define expect_menu(window, menu) \
    SetLastError(0xdeadbeef); \
    res = (GetMenu(window) == (HMENU)menu); \
    ok(res, "GetMenu error %d\n", GetLastError())

#define expect_style(window, style)\
    ok((ULONG)GetWindowLongA(window, GWL_STYLE) == (style), "expected style %x != %x\n", (LONG)(style), GetWindowLongA(window, GWL_STYLE))

#define expect_ex_style(window, ex_style)\
    ok((ULONG)GetWindowLongA(window, GWL_EXSTYLE) == (ex_style), "expected ex_style %x != %x\n", (LONG)(ex_style), GetWindowLongA(window, GWL_EXSTYLE))

#define expect_gle_broken_9x(gle)\
    ok(GetLastError() == gle ||\
       broken(GetLastError() == 0xdeadbeef),\
       "IsMenu set error %d\n", GetLastError())

    hmenu = CreateMenu();
    assert(hmenu != 0);
    parent = GetDesktopWindow();
    assert(parent != 0);

    SetLastError(0xdeadbeef);
    res = IsMenu(hmenu);
    ok(res, "IsMenu error %d\n", GetLastError());

    /* WS_CHILD */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    /* WS_POPUP */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
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
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP | WS_CAPTION,
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
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
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
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_CAPTION,
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
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
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
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
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
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP,
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
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd || broken(hwnd != 0 /* Win9x */), "CreateWindowEx should fail\n");
    expect_gle_broken_9x(ERROR_INVALID_MENU_HANDLE);
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
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
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MinMax_WndClass";
    RegisterClassA(&cls);

    SetLastError(0xdeadbeef);
    parent = CreateWindowExA(0, "MinMax_WndClass", NULL, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                           0, 0, 100, 100, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(parent, 0);
    expect_style(parent, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPSIBLINGS);
    expect_ex_style(parent, WS_EX_WINDOWEDGE);

    memset(&minmax, 0, sizeof(minmax));
    SendMessageA(parent, WM_GETMINMAXINFO, 0, (LPARAM)&minmax);
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxSize.x, minmax.ptMaxSize.y);
    ok(IsRectEmpty(&rc_minmax), "ptMaxSize is not empty\n");
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y);
    ok(IsRectEmpty(&rc_minmax), "ptMaxTrackSize is not empty\n");

    GetWindowRect(parent, &rc);
    ok(!IsRectEmpty(&rc), "parent window rect is empty\n");
    GetClientRect(parent, &rc);
    ok(!IsRectEmpty(&rc), "parent client rect is empty\n");

    InflateRect(&rc, 200, 200);
    trace("creating child with rect %s\n", wine_dbgstr_rect(&rc));

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "MinMax_WndClass", NULL, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                          rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                          parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %d\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);

    memset(&minmax, 0, sizeof(minmax));
    SendMessageA(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minmax);
    SetRect(&rc_minmax, 0, 0, minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y);

    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);
    ok(EqualRect(&rc, &rc_minmax), "rects don't match: %s and %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&rc_minmax));
    DestroyWindow(hwnd);

    cls.lpfnWndProc = winsizes_wnd_proc;
    cls.lpszClassName = "Sizes_WndClass";
    RegisterClassA(&cls);

    expected_cx = expected_cy = 200000;
    SetRect( &expected_rect, 0, 0, 200000, 200000 );
    broken_rect = expected_rect;
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 300000, 300000, 200000, 200000, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 200000 || rc.right == 65535 || broken(rc.right == (short)200000),
        "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 200000 || rc.bottom == 65535 || broken(rc.bottom == (short)200000),
        "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -10;
    SetRectEmpty(&expected_rect);
    SetRect( &broken_rect, 0, 0, -10, -10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -20, -20, -10, -10, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -200000;
    SetRectEmpty(&expected_rect);
    SetRect( &broken_rect, 0, 0, -200000, -200000 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -300000, -300000, -200000, -200000, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    /* we need a parent at 0,0 so that child coordinates match */
    DestroyWindow(parent);
    parent = CreateWindowExA(0, "MinMax_WndClass", NULL, WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowEx error %d\n", GetLastError());

    expected_cx = 100;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 10, 10, 110, 0x7fffffff );
    SetRect( &broken_rect, 10, 10, 110, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 10, 10, 100, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 100, "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || rc.bottom ==65535 || broken(rc.bottom == 0),
        "invalid rect bottom %u\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = 0x7fffffff;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 20, 10, 0x7fffffff, 0x7fffffff );
    SetRect( &broken_rect, 20, 10, 0x7fffffffU + 20, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 20, 10, 0x7fffffff, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %u\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0x7fffffff - 20 || rc.right == 65535 || broken(rc.right == 0),
        "invalid rect right %u\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || rc.right == 65535 || broken(rc.bottom == 0),
        "invalid rect bottom %u\n", rc.bottom );
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

    if (pGetLayout && pSetLayout)
    {
        HDC hdc = GetDC( parent );
        pSetLayout( hdc, LAYOUT_RTL );
        if (pGetLayout( hdc ))
        {
            ReleaseDC( parent, hdc );
            DestroyWindow( parent );
            SetLastError( 0xdeadbeef );
            parent = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_LAYOUTRTL, "static", NULL, WS_POPUP,
                                    0, 0, 100, 100, 0, 0, 0, NULL);
            ok( parent != 0, "creation failed err %u\n", GetLastError());
            expect_ex_style( parent, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL );
            hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 20, 20, parent, 0, 0, NULL);
            ok( hwnd != 0, "creation failed err %u\n", GetLastError());
            expect_ex_style( hwnd, WS_EX_LAYOUTRTL );
            DestroyWindow( hwnd );
            hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 20, 20, parent, 0, 0, NULL);
            ok( hwnd != 0, "creation failed err %u\n", GetLastError());
            expect_ex_style( hwnd, 0 );
            DestroyWindow( hwnd );
            SetWindowLongW( parent, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT );
            hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 20, 20, parent, 0, 0, NULL);
            ok( hwnd != 0, "creation failed err %u\n", GetLastError());
            expect_ex_style( hwnd, 0 );
            DestroyWindow( hwnd );

            if (pGetProcessDefaultLayout && pSetProcessDefaultLayout)
            {
                DWORD layout;

                SetLastError( 0xdeadbeef );
                ok( !pGetProcessDefaultLayout( NULL ), "GetProcessDefaultLayout succeeded\n" );
                ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );
                SetLastError( 0xdeadbeef );
                res = pGetProcessDefaultLayout( &layout );
                ok( res, "GetProcessDefaultLayout failed err %u\n", GetLastError ());
                ok( layout == 0, "GetProcessDefaultLayout wrong layout %x\n", layout );
                SetLastError( 0xdeadbeef );
                res = pSetProcessDefaultLayout( 7 );
                ok( res, "SetProcessDefaultLayout failed err %u\n", GetLastError ());
                res = pGetProcessDefaultLayout( &layout );
                ok( res, "GetProcessDefaultLayout failed err %u\n", GetLastError ());
                ok( layout == 7, "GetProcessDefaultLayout wrong layout %x\n", layout );
                SetLastError( 0xdeadbeef );
                res = pSetProcessDefaultLayout( LAYOUT_RTL );
                ok( res, "SetProcessDefaultLayout failed err %u\n", GetLastError ());
                res = pGetProcessDefaultLayout( &layout );
                ok( res, "GetProcessDefaultLayout failed err %u\n", GetLastError ());
                ok( layout == LAYOUT_RTL, "GetProcessDefaultLayout wrong layout %x\n", layout );
                hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                                      0, 0, 100, 100, 0, 0, 0, NULL);
                ok( hwnd != 0, "creation failed err %u\n", GetLastError());
                expect_ex_style( hwnd, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL );
                DestroyWindow( hwnd );
                hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                                      0, 0, 100, 100, parent, 0, 0, NULL);
                ok( hwnd != 0, "creation failed err %u\n", GetLastError());
                expect_ex_style( hwnd, WS_EX_APPWINDOW );
                DestroyWindow( hwnd );
                pSetProcessDefaultLayout( 0 );
            }
            else win_skip( "SetProcessDefaultLayout not supported\n" );
        }
        else win_skip( "SetLayout not supported\n" );
    }
    else win_skip( "SetLayout not available\n" );

    DestroyWindow(parent);

    UnregisterClassA("MinMax_WndClass", GetModuleHandleA(NULL));
    UnregisterClassA("Sizes_WndClass", GetModuleHandleA(NULL));

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
    retval = SetWindowLongPtrA(NULL, GWLP_WNDPROC, 0);
    ok(!retval, "SetWindowLongPtr on invalid window handle should have returned 0 instead of 0x%lx\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_WINDOW_HANDLE),
        "SetWindowLongPtr should have set error to ERROR_INVALID_WINDOW_HANDLE instead of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrA(hwndMain, 0xdeadbeef, 0);
    ok(!retval, "SetWindowLongPtr on invalid index should have returned 0 instead of 0x%lx\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_INDEX),
        "SetWindowLongPtr should have set error to ERROR_INVALID_INDEX instead of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrA(hwndMain, GWLP_WNDPROC, 0);
    ok((WNDPROC)retval == main_window_procA || broken(!retval), /* win9x */
        "SetWindowLongPtr on invalid window proc should have returned address of main_window_procA instead of 0x%lx\n", retval);
    ok(GetLastError() == 0xdeadbeef, "SetWindowLongPtr shouldn't have set the last error, instead of setting it to %d\n", GetLastError());
    retval = GetWindowLongPtrA(hwndMain, GWLP_WNDPROC);
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
        SetWindowLongPtrA(hwndMain, GWLP_WNDPROC, 0);
    }
}

static LRESULT WINAPI check_style_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const STYLESTRUCT *expected = (STYLESTRUCT *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    const STYLESTRUCT *got = (STYLESTRUCT *)lParam;

    if (message == WM_STYLECHANGING && wParam == GWL_STYLE)
    {
        ok(got->styleOld == expected[0].styleOld, "expected old style %#x, got %#x\n",
            expected[0].styleOld, got->styleOld);
        ok(got->styleNew == expected[0].styleNew, "expected new style %#x, got %#x\n",
            expected[0].styleNew, got->styleNew);
    }
    else if (message == WM_STYLECHANGED && wParam == GWL_STYLE)
    {
        ok(got->styleOld == expected[1].styleOld, "expected old style %#x, got %#x\n",
            expected[1].styleOld, got->styleOld);
        ok(got->styleNew == expected[1].styleNew, "expected new style %#x, got %#x\n",
            expected[1].styleNew, got->styleNew);
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_set_window_style(void)
{
    LONG expected_style, new_style, old_style;
    STYLESTRUCT expected_stylestruct[2];
    unsigned int i;
    WNDCLASSA cls;
    HWND hwnd;

    static const struct
    {
        LONG creation_style;
        LONG style;
    }
    tests[] =
    {
        { WS_MINIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
          WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX },
        { WS_MINIMIZE | WS_CLIPSIBLINGS | WS_CAPTION,
          WS_CLIPSIBLINGS | WS_CAPTION },
        { WS_MAXIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
          WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX },
        { WS_MAXIMIZE | WS_CLIPSIBLINGS | WS_CAPTION,
          WS_CLIPSIBLINGS | WS_CAPTION },
        { WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
          WS_MINIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX },
        { WS_CLIPSIBLINGS | WS_CAPTION,
          WS_MINIMIZE | WS_CLIPSIBLINGS | WS_CAPTION },
        { WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
          WS_MAXIMIZE | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX },
        { WS_CLIPSIBLINGS | WS_CAPTION,
          WS_MAXIMIZE | WS_CLIPSIBLINGS | WS_CAPTION },
    };

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = check_style_wnd_proc;
    cls.hInstance = GetModuleHandleA(0);
    cls.lpszClassName = "TestSetWindowStylesClass";
    ok(RegisterClassA(&cls), "RegisterClass failed\n");

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        expected_style = tests[i].style;
        if (tests[i].creation_style & WS_MINIMIZE)
            expected_style |= WS_MINIMIZE;

        expected_stylestruct[0].styleOld = tests[i].creation_style;
        expected_stylestruct[0].styleNew = tests[i].style;
        expected_stylestruct[1].styleOld = tests[i].creation_style;
        expected_stylestruct[1].styleNew = expected_style;

        hwnd = CreateWindowA(cls.lpszClassName, "Test set styles",
                             tests[i].creation_style, 100, 100, 200, 200, 0, 0, 0, NULL);
        ok(hwnd != 0, "CreateWindow failed\n");
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)&expected_stylestruct);

        old_style = SetWindowLongA(hwnd, GWL_STYLE, tests[i].style);
        ok(old_style == tests[i].creation_style, "expected old style %#x, got %#x\n",
            tests[i].creation_style, old_style);
        new_style = GetWindowLongA(hwnd, GWL_STYLE);
        ok(new_style == expected_style, "expected new style %#x, got %#x\n",
            expected_style, new_style);

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0);
        DestroyWindow(hwnd);
    }

    UnregisterClassA(cls.lpszClassName, cls.hInstance);
}

static void test_ShowWindow(void)
{
    HWND hwnd;
    DWORD style;
    RECT rcMain, rc, rcMinimized;
    LPARAM ret;

    SetRect(&rcMain, 120, 120, 210, 210);

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL,
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_POPUP,
                          rcMain.left, rcMain.top,
                          rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                          0, 0, 0, NULL);
    assert(hwnd);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(!(style & WS_VISIBLE), "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rcMinimized);
    ok(!EqualRect(&rcMain, &rcMinimized), "rects shouldn't match\n");
    /* shouldn't be able to resize minimized windows */
    ret = SetWindowPos(hwnd, 0, 0, 0,
                       (rcMinimized.right - rcMinimized.left) * 2,
                       (rcMinimized.bottom - rcMinimized.top) * 2,
                       SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "not expected ret: %lu\n", ret);
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rc, &rcMinimized), "rects should match\n");

    ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = EnableWindow(hwnd, FALSE);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(!EqualRect(&rcMain, &rc), "rects shouldn't match\n");

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(!EqualRect(&rcMain, &rc), "rects shouldn't match\n");

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %lu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    ok(IsWindow(hwnd), "window should exist\n");

    ret = EnableWindow(hwnd, TRUE);
    ok(ret, "not expected ret: %lu\n", ret);

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %lu\n", ret);
    ok(!IsWindow(hwnd), "window should not exist\n");
}

static DWORD CALLBACK gettext_msg_thread( LPVOID arg )
{
    HWND hwnd = arg;
    char buf[32];
    INT buf_len;

    /* test GetWindowTextA */
    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "another_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    return 0;
}

static DWORD CALLBACK settext_msg_thread( LPVOID arg )
{
    HWND hwnd = arg;
    BOOL success;

    /* test SetWindowTextA */
    num_settext_msgs = 0;
    success = SetWindowTextA( hwnd, "thread_caption" );
    ok( success, "SetWindowTextA failed\n" );
    ok( num_settext_msgs == 1, "got %u WM_SETTEXT messages\n", num_settext_msgs );

    return 0;
}

static void test_gettext(void)
{
    DWORD tid, num_msgs;
    WCHAR bufW[32];
    HANDLE thread;
    BOOL success;
    char buf[32];
    INT buf_len;
    HWND hwnd, hwnd2;
    LRESULT r;
    MSG msg;

    hwnd = CreateWindowExA( 0, "MainWindowClass", "caption", WS_POPUP, 0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );

    /* test GetWindowTextA */
    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    /* other process window */
    strcpy( buf, "a" );
    buf_len = GetWindowTextA( GetDesktopWindow(), buf, sizeof(buf) );
    ok( buf_len == 0, "expected a nonempty window text\n" );
    ok( *buf == 0, "got wrong window text '%s'\n", buf );

    strcpy( buf, "blah" );
    buf_len = GetWindowTextA( GetDesktopWindow(), buf, 0 );
    ok( buf_len == 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "blah"), "got wrong window text '%s'\n", buf );

    bufW[0] = 0xcc;
    buf_len = GetWindowTextW( GetDesktopWindow(), bufW, 0 );
    ok( buf_len == 0, "expected a nonempty window text\n" );
    ok( bufW[0] == 0xcc, "got %x\n", bufW[0] );

    g_wm_gettext_override.enabled = TRUE;

    num_gettext_msgs = 0;
    memset( buf, 0xcc, sizeof(buf) );
    g_wm_gettext_override.buff = buf;
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( *buf == 0, "got %x\n", *buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    strcpy( buf, "blah" );
    g_wm_gettext_override.buff = buf;
    buf_len = GetWindowTextA( hwnd, buf, 0 );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( !strcmp(buf, "blah"), "got %s\n", buf );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    g_wm_gettext_override.enabled = FALSE;

    /* same for W window */
    hwnd2 = CreateWindowExW( 0, mainclassW, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd2 != 0, "CreateWindowExA error %d\n", GetLastError() );

    g_wm_gettext_override.enabled = TRUE;

    num_gettext_msgs = 0;
    memset( bufW, 0xcc, sizeof(bufW) );
    g_wm_gettext_override.buffW = bufW;
    buf_len = GetWindowTextW( hwnd2, bufW, sizeof(bufW)/sizeof(WCHAR) );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( *bufW == 0, "got %x\n", *bufW );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    memset( bufW, 0xcc, sizeof(bufW) );
    g_wm_gettext_override.buffW = bufW;
    buf_len = GetWindowTextW( hwnd2, bufW, 0 );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( *bufW == 0xcccc, "got %x\n", *bufW );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    g_wm_gettext_override.enabled = FALSE;

    DestroyWindow( hwnd2 );

    /* test WM_GETTEXT */
    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    r = SendMessageA( hwnd, WM_GETTEXT, sizeof(buf), (LONG_PTR)buf );
    ok( r != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    /* test SetWindowTextA */
    num_settext_msgs = 0;
    success = SetWindowTextA( hwnd, "new_caption" );
    ok( success, "SetWindowTextA failed\n" );
    ok( num_settext_msgs == 1, "got %u WM_SETTEXT messages\n", num_settext_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "new_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    /* test WM_SETTEXT */
    num_settext_msgs = 0;
    r = SendMessageA( hwnd, WM_SETTEXT, 0, (ULONG_PTR)"another_caption" );
    ok( r != 0, "WM_SETTEXT failed\n" );
    ok( num_settext_msgs == 1, "got %u WM_SETTEXT messages\n", num_settext_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "another_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
        DispatchMessageA( &msg );

    /* test interthread GetWindowTextA */
    num_msgs = 0;
    thread = CreateThread( NULL, 0, gettext_msg_thread, hwnd, 0, &tid );
    ok(thread != NULL, "CreateThread failed, error %d\n", GetLastError());
    while (MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, QS_SENDMESSAGE ) != WAIT_OBJECT_0)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
            DispatchMessageA( &msg );
        num_msgs++;
    }
    CloseHandle( thread );
    ok( num_msgs >= 1, "got %u wakeups from MsgWaitForMultipleObjects\n", num_msgs );

    /* test interthread SetWindowText */
    num_msgs = 0;
    thread = CreateThread( NULL, 0, settext_msg_thread, hwnd, 0, &tid );
    ok(thread != NULL, "CreateThread failed, error %d\n", GetLastError());
    while (MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, QS_SENDMESSAGE ) != WAIT_OBJECT_0)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
            DispatchMessageA( &msg );
        num_msgs++;
    }
    CloseHandle( thread );
    ok( num_msgs >= 1, "got %u wakeups from MsgWaitForMultipleObjects\n", num_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "thread_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    /* seems to crash on every modern Windows version */
    if (0)
    {
    r = SendMessageA( hwnd, WM_GETTEXT, 0x10, 0x1000);
    ok( r == 0, "settext should return zero\n");

    r = SendMessageA( hwnd, WM_GETTEXT, 0x10000, 0);
    ok( r == 0, "settext should return zero (%ld)\n", r);

    r = SendMessageA( hwnd, WM_GETTEXT, 0xff000000, 0x1000);
    ok( r == 0, "settext should return zero (%ld)\n", r);

    r = SendMessageA( hwnd, WM_GETTEXT, 0x1000, 0xff000000);
    ok( r == 0, "settext should return zero (%ld)\n", r);
    }

    DestroyWindow(hwnd);
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
    SetRectEmpty(&rc2);
    ret = GetUpdateRect(hgrandparent, &rc1, FALSE);
    ok(!ret, "GetUpdateRect returned not empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
       wine_dbgstr_rect(&rc2));

    SetRect(&rc2, 10, 10, 40, 40);
    ret = GetUpdateRect(hparent, &rc1, FALSE);
    ok(ret, "GetUpdateRect returned empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
       wine_dbgstr_rect(&rc2));

    parent_wm_paint = FALSE;
    grandparent_wm_paint = FALSE;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_PAINT)
        {
            if (msg.hwnd == hgrandparent) grandparent_wm_paint = TRUE;
            if (msg.hwnd == hparent) parent_wm_paint = TRUE;
        }
        DispatchMessageA(&msg);
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
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

    SetRectEmpty(&rc2);
    ret = GetUpdateRect(hgrandparent, &rc1, FALSE);
    ok(!ret, "GetUpdateRect returned not empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
       wine_dbgstr_rect(&rc2));

    SetRect(&rc2, 10, 10, 40, 40);
    ret = GetUpdateRect(hparent, &rc1, FALSE);
    ok(ret, "GetUpdateRect returned empty region\n");
    ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
       wine_dbgstr_rect(&rc2));

    parent_wm_paint = FALSE;
    grandparent_wm_paint = FALSE;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_PAINT)
        {
            if (msg.hwnd == hgrandparent) grandparent_wm_paint = TRUE;
            if (msg.hwnd == hparent) parent_wm_paint = TRUE;
        }
        DispatchMessageA(&msg);
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
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void test_Expose(void)
{
    WNDCLASSA cls;
    HWND mw;

    memset(&cls, 0, sizeof(WNDCLASSA));
    cls.lpfnWndProc = TestExposedRegion_WndProc;
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszClassName = "TestExposeClass";
    RegisterClassA(&cls);

    mw = CreateWindowA("TestExposeClass", "MainWindow", WS_VISIBLE|WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 100, NULL, NULL, 0, NULL);

    UpdateWindow(mw);
    DestroyWindow(mw);
}

static LRESULT CALLBACK TestNCRedraw_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static UINT ncredrawflags;
    PAINTSTRUCT ps;

    switch(msg)
    {
    case WM_CREATE:
        ncredrawflags = *(UINT *) (((CREATESTRUCTA *)lParam)->lpCreateParams);
        return 0;
    case WM_NCPAINT:
        RedrawWindow(hwnd, NULL, NULL, ncredrawflags);
        break;
    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void run_NCRedrawLoop(UINT flags)
{
    HWND hwnd;
    MSG msg;

    UINT loopcount = 0;

    hwnd = CreateWindowA("TestNCRedrawClass", "MainWindow",
                         WS_OVERLAPPEDWINDOW, 0, 0, 200, 100,
                         NULL, NULL, 0, &flags);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    flush_events( FALSE );
    while (PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_PAINT) loopcount++;
        if (loopcount >= 100) break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);
    }
    todo_wine_if (flags == (RDW_INVALIDATE | RDW_FRAME))
        ok(loopcount < 100, "Detected infinite WM_PAINT loop (%x).\n", flags);
    DestroyWindow(hwnd);
}

static void test_NCRedraw(void)
{
    WNDCLASSA wndclass;

    wndclass.lpszClassName = "TestNCRedrawClass";
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = TestNCRedraw_WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = 0;
    wndclass.hIcon = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    wndclass.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;

    RegisterClassA(&wndclass);

    run_NCRedrawLoop(RDW_INVALIDATE | RDW_FRAME);
    run_NCRedrawLoop(RDW_INVALIDATE);
}

static void test_GetWindowModuleFileName(void)
{
    HWND hwnd;
    HINSTANCE hinst;
    UINT ret1, ret2;
    char buf1[MAX_PATH], buf2[MAX_PATH];

    if (!pGetWindowModuleFileNameA)
    {
        win_skip("GetWindowModuleFileNameA is not available\n");
        return;
    }

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0,0,0,0, 0, 0, 0, NULL);
    assert(hwnd);

    hinst = (HINSTANCE)GetWindowLongPtrA(hwnd, GWLP_HINSTANCE);
    ok(hinst == 0 || broken(hinst == GetModuleHandleA(NULL)), /* win9x */ "expected 0, got %p\n", hinst);

    buf1[0] = 0;
    SetLastError(0xdeadbeef);
    ret1 = GetModuleFileNameA(hinst, buf1, sizeof(buf1));
    ok(ret1, "GetModuleFileName error %u\n", GetLastError());

    buf2[0] = 0;
    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, sizeof(buf2));
    ok(ret2 || broken(!ret2), /* nt4 sp 3 */
       "GetWindowModuleFileNameA error %u\n", GetLastError());

    if (ret2)
    {
        ok(ret1 == ret2 || broken(ret2 == ret1 + 1), /* win98 */ "%u != %u\n", ret1, ret2);
        ok(!strcmp(buf1, buf2), "%s != %s\n", buf1, buf2);
    }
    hinst = GetModuleHandleA(NULL);

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileNameA(hinst, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2 || broken(ret2 == ret1 - 3), /* win98 */
       "expected %u, got %u\n", ret1 - 2, ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileNameA(hinst, buf2, 0);
    ok(!ret2, "GetModuleFileName should return 0\n");
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2 || broken(ret2 == ret1 - 3) /* win98 */ || broken(!ret2), /* nt4 sp3 */
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

    hwnd = FindWindowA("Shell_TrayWnd", NULL);
    ok(IsWindow(hwnd) || broken(!hwnd), "got invalid tray window %p\n", hwnd);
    SetLastError(0xdeadbeef);
    ret1 = pGetWindowModuleFileNameA(hwnd, buf1, sizeof(buf1));
    ok(!ret1 || broken(ret1), /* win98 */ "expected 0, got %u\n", ret1);

    if (!ret1)  /* inter-process GetWindowModuleFileName works on win9x, so don't test the desktop there */
    {
        ret1 = GetModuleFileNameA(0, buf1, sizeof(buf1));
        hwnd = GetDesktopWindow();
        ok(IsWindow(hwnd), "got invalid desktop window %p\n", hwnd);
        SetLastError(0xdeadbeef);
        ret2 = pGetWindowModuleFileNameA(hwnd, buf2, sizeof(buf2));
        ok(!ret2 ||
           ret1 == ret2 || /* vista */
           broken(ret2),  /* some win98 return user.exe as file name */
           "expected 0 or %u, got %u %s\n", ret1, ret2, buf2);
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
        ok( !pGetAncestor(parent, GA_PARENT) || broken(pGetAncestor(parent, GA_PARENT) != 0), /* win2k */
            "parent shouldn't have parent %p\n", pGetAncestor(parent, GA_PARENT) );
        trace("parent %p root %p desktop %p\n", parent, root, desktop);
        if (!GetClassNameA( parent, buffer, sizeof(buffer) )) buffer[0] = 0;
        ok( !lstrcmpiA( buffer, "Message" ), "wrong parent class '%s'\n", buffer );
        GetWindowRect( parent, &rect );
        ok( rect.left == 0 && rect.right == 100 && rect.top == 0 && rect.bottom == 100,
            "wrong parent rect %s\n", wine_dbgstr_rect( &rect ));
    }
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 100 && rect.right == 300 && rect.top == 100 && rect.bottom == 300,
        "wrong window rect %s\n", wine_dbgstr_rect( &rect ));

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
    POINT pt = { 0, 0 };
    SIZE sz = { 200, 200 };
    HDC hdc;
    HBITMAP hbm;
    BOOL ret;

    if (!pGetLayeredWindowAttributes || !pSetLayeredWindowAttributes || !pUpdateLayeredWindow)
    {
        win_skip( "layered windows not supported\n" );
        return;
    }

    hdc = CreateCompatibleDC( 0 );
    hbm = CreateCompatibleBitmap( hdc, 200, 200 );
    SelectObject( hdc, hbm );

    hwnd = CreateWindowExA(0, "MainWindowClass", "message window", WS_CAPTION,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    assert( hwnd );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on non-layered window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError() );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on non-layered window\n" );
    ret = pSetLayeredWindowAttributes( hwnd, 0, 0, LWA_ALPHA );
    ok( !ret, "SetLayeredWindowAttributes should fail on non-layered window\n" );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on layered but not initialized window\n" );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on layered but not initialized window\n" );
    ret = pSetLayeredWindowAttributes( hwnd, 0x123456, 44, LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x123456 || key == 0, "wrong color key %x\n", key );
    ok( alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == LWA_ALPHA, "wrong flags %x\n", flags );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on layered but initialized window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError() );

    /* clearing WS_EX_LAYERED resets attributes */
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on non-layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on no longer layered window\n" );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( !ret, "GetLayeredWindowAttributes should fail on layered but not initialized window\n" );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow should succeed on layered window\n" );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE | ULW_EX_NORESIZE );
    ok( !ret, "UpdateLayeredWindow should fail with ex flag\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    if (pUpdateLayeredWindowIndirect)
    {
        UPDATELAYEREDWINDOWINFO info;
        info.cbSize   = sizeof(info);
        info.hdcDst   = 0;
        info.pptDst   = NULL;
        info.psize    = &sz;
        info.hdcSrc   = hdc;
        info.pptSrc   = &pt;
        info.crKey    = 0;
        info.pblend   = NULL;
        info.dwFlags  = ULW_OPAQUE | ULW_EX_NORESIZE;
        info.prcDirty = NULL;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( ret, "UpdateLayeredWindowIndirect should succeed on layered window\n" );
        sz.cx--;
        SetLastError(0);
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        /* particular error code differs from version to version, could be ERROR_INCORRECT_SIZE,
           ERROR_MR_MID_NOT_FOUND or ERROR_GEN_FAILURE (Win8/Win10) */
        ok( GetLastError() != 0, "wrong error %u\n", GetLastError() );
        info.dwFlags  = ULW_OPAQUE;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( ret, "UpdateLayeredWindowIndirect should succeed on layered window\n" );
        sz.cx++;
        info.dwFlags  = ULW_OPAQUE | 0xf00;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
        info.cbSize--;
        info.dwFlags  = ULW_OPAQUE;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
        ret = pUpdateLayeredWindowIndirect( hwnd, NULL );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    }

    ret = pSetLayeredWindowAttributes( hwnd, 0x654321, 22, LWA_COLORKEY | LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x654321, "wrong color key %x\n", key );
    ok( alpha == 22, "wrong alpha %u\n", alpha );
    ok( flags == (LWA_COLORKEY | LWA_ALPHA), "wrong flags %x\n", flags );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on layered but initialized window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError() );

    ret = pSetLayeredWindowAttributes( hwnd, 0x888888, 33, LWA_COLORKEY );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x888888, "wrong color key %x\n", key );
    /* alpha not changed on vista if LWA_ALPHA is not set */
    ok( alpha == 22 || alpha == 33, "wrong alpha %u\n", alpha );
    ok( flags == LWA_COLORKEY, "wrong flags %x\n", flags );

    /* color key may or may not be changed without LWA_COLORKEY */
    ret = pSetLayeredWindowAttributes( hwnd, 0x999999, 44, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x888888 || key == 0x999999, "wrong color key %x\n", key );
    ok( alpha == 22 || alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %x\n", flags );

    /* default alpha and color key is 0 */
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pSetLayeredWindowAttributes( hwnd, 0x222222, 55, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0 || key == 0x222222, "wrong color key %x\n", key );
    ok( alpha == 0 || alpha == 55, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %x\n", flags );

    DestroyWindow( hwnd );
    DeleteDC( hdc );
    DeleteObject( hbm );
}

static MONITORINFO mi;

static LRESULT CALLBACK fullscreen_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
            ok(cs->x == mi.rcMonitor.left && cs->y == mi.rcMonitor.top &&
               cs->cx == mi.rcMonitor.right && cs->cy == mi.rcMonitor.bottom,
               "expected %s, got (%d,%d)-(%d,%d)\n", wine_dbgstr_rect(&mi.rcMonitor),
               cs->x, cs->y, cs->cx, cs->cy);
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *minmax = (MINMAXINFO *)lp;
            ok(minmax->ptMaxPosition.x <= mi.rcMonitor.left, "%d <= %d\n", minmax->ptMaxPosition.x, mi.rcMonitor.left);
            ok(minmax->ptMaxPosition.y <= mi.rcMonitor.top, "%d <= %d\n", minmax->ptMaxPosition.y, mi.rcMonitor.top);
            ok(minmax->ptMaxSize.x >= mi.rcMonitor.right, "%d >= %d\n", minmax->ptMaxSize.x, mi.rcMonitor.right);
            ok(minmax->ptMaxSize.y >= mi.rcMonitor.bottom, "%d >= %d\n", minmax->ptMaxSize.y, mi.rcMonitor.bottom);
            break;
        }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void test_fullscreen(void)
{
    static const DWORD t_style[] = {
        WS_OVERLAPPED, WS_POPUP, WS_CHILD, WS_THICKFRAME, WS_DLGFRAME
    };
    static const DWORD t_ex_style[] = {
        0, WS_EX_APPWINDOW, WS_EX_TOOLWINDOW
    };
    WNDCLASSA cls;
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
    trace("monitor %s, work %s\n", wine_dbgstr_rect(&mi.rcMonitor), wine_dbgstr_rect(&mi.rcWork));

    cls.style = 0;
    cls.lpfnWndProc = fullscreen_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "fullscreen_class";
    RegisterClassA(&cls);

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
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            ok(rc.left >= mi.rcMonitor.left && rc.top >= mi.rcMonitor.top &&
               rc.right <= mi.rcMonitor.right && rc.bottom <= mi.rcMonitor.bottom,
               "%#x/%#x: window rect %s must be in %s\n", ex_style, style, wine_dbgstr_rect(&rc),
               wine_dbgstr_rect(&mi.rcMonitor));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#x/%#x) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            trace("%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            if (style & (WS_CHILD | WS_POPUP))
                ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
                   rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
                   "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            else
                ok(rc.left >= mi.rcMonitor.left && rc.top >= mi.rcMonitor.top &&
                   rc.right <= mi.rcMonitor.right && rc.bottom <= mi.rcMonitor.bottom,
                   "%#x/%#x: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);
        }
    }

    UnregisterClassA("fullscreen_class", GetModuleHandleA(NULL));
}

static BOOL test_thick_child_got_minmax;
static const char * test_thick_child_name;
static LONG test_thick_child_style;
static LONG test_thick_child_exStyle;

static LRESULT WINAPI test_thick_child_size_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    MINMAXINFO* minmax;
    int expectedMinTrackX;
    int expectedMinTrackY;
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
    LONG adjustedStyle;
    RECT rect;
    switch (msg)
    {
        case WM_GETMINMAXINFO:
        {
            minmax = (MINMAXINFO *)lparam;
            trace("hwnd %p, WM_GETMINMAXINFO, %08lx, %08lx\n", hwnd, wparam, lparam);
            dump_minmax_info( minmax );

            test_thick_child_got_minmax = TRUE;


            adjustedStyle = test_thick_child_style;
            if ((adjustedStyle & WS_CAPTION) == WS_CAPTION)
                adjustedStyle &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            GetClientRect(GetParent(hwnd), &rect);
            AdjustWindowRectEx(&rect, adjustedStyle, FALSE, test_thick_child_exStyle);

            if (test_thick_child_style & (WS_DLGFRAME | WS_BORDER))
            {
                expectedMinTrackX = GetSystemMetrics(SM_CXMINTRACK);
                expectedMinTrackY = GetSystemMetrics(SM_CYMINTRACK);
            }
            else
            {
                expectedMinTrackX = -2 * rect.left;
                expectedMinTrackY = -2 * rect.top;
            }
            actualMinTrackX =  minmax->ptMinTrackSize.x;
            actualMinTrackY =  minmax->ptMinTrackSize.y;

            ok(actualMinTrackX == expectedMinTrackX && actualMinTrackY == expectedMinTrackY,
                "expected minTrack %dx%d, actual minTrack %dx%d for %s\n",
                expectedMinTrackX, expectedMinTrackY, actualMinTrackX, actualMinTrackY,
                test_thick_child_name);

            actualMaxTrackX = minmax->ptMaxTrackSize.x;
            actualMaxTrackY = minmax->ptMaxTrackSize.y;
            expectedMaxTrackX = GetSystemMetrics(SM_CXMAXTRACK);
            expectedMaxTrackY = GetSystemMetrics(SM_CYMAXTRACK);
            ok(actualMaxTrackX == expectedMaxTrackX &&  actualMaxTrackY == expectedMaxTrackY,
                "expected maxTrack %dx%d, actual maxTrack %dx%d for %s\n",
                 expectedMaxTrackX, expectedMaxTrackY, actualMaxTrackX, actualMaxTrackY,
                test_thick_child_name);

            expectedMaxSizeX = rect.right - rect.left;
            expectedMaxSizeY = rect.bottom - rect.top;
            actualMaxSizeX = minmax->ptMaxSize.x;
            actualMaxSizeY = minmax->ptMaxSize.y;

            ok(actualMaxSizeX == expectedMaxSizeX &&  actualMaxSizeY == expectedMaxSizeY,
                "expected maxSize %dx%d, actual maxSize %dx%d for %s\n",
                expectedMaxSizeX, expectedMaxSizeY, actualMaxSizeX, actualMaxSizeY,
                test_thick_child_name);


            expectedPosX = rect.left;
            expectedPosY = rect.top;
            actualPosX = minmax->ptMaxPosition.x;
            actualPosY = minmax->ptMaxPosition.y;
            ok(actualPosX == expectedPosX && actualPosY == expectedPosY,
                "expected maxPosition (%d/%d), actual maxPosition (%d/%d) for %s\n",
                expectedPosX, expectedPosY, actualPosX, actualPosY, test_thick_child_name);

            break;
        }
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

#define NUMBER_OF_THICK_CHILD_TESTS 16
static void test_thick_child_size(HWND parentWindow)
{
    BOOL success;
    RECT childRect;
    RECT adjustedParentRect;
    HWND childWindow;
    LONG childWidth;
    LONG childHeight;
    LONG expectedWidth;
    LONG expectedHeight;
    WNDCLASSA cls;
    static const char className[] = "THICK_CHILD_CLASS";
    int i;
    LONG adjustedStyle;
    static const LONG styles[NUMBER_OF_THICK_CHILD_TESTS] = {
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER,
        WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER,
    };

    static const LONG exStyles[NUMBER_OF_THICK_CHILD_TESTS] = {
        0,
        0,
        0,
        0,
        WS_EX_DLGMODALFRAME,
        WS_EX_DLGMODALFRAME,
        WS_EX_DLGMODALFRAME,
        WS_EX_DLGMODALFRAME,
        WS_EX_STATICEDGE,
        WS_EX_STATICEDGE,
        WS_EX_STATICEDGE,
        WS_EX_STATICEDGE,
        WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME,
        WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME,
        WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME,
        WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME,
    };
    static const char *styleName[NUMBER_OF_THICK_CHILD_TESTS] = {
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME, exstyle= WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME exstyle= WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER exstyle= WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER exstyle= WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME exstyle= WS_EX_STATICEDGE",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME exstyle= WS_EX_STATICEDGE",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER exstyle= WS_EX_STATICEDGE",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER exstyle= WS_EX_STATICEDGE",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME, exstyle= WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME exstyle= WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_DLGFRAME | WS_BORDER exstyle= WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME",
        "style=WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_BORDER exstyle= WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME",
    };

    cls.style = 0;
    cls.lpfnWndProc = test_thick_child_size_winproc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = className;
    SetLastError(0xdeadbeef);
    success = RegisterClassA(&cls);
    ok(success,"RegisterClassA failed, error: %u\n", GetLastError());

    for(i = 0; i < NUMBER_OF_THICK_CHILD_TESTS; i++)
    {
        test_thick_child_name = styleName[i];
        test_thick_child_style = styles[i];
        test_thick_child_exStyle = exStyles[i];
        test_thick_child_got_minmax = FALSE;

        SetLastError(0xdeadbeef);
        childWindow = CreateWindowExA( exStyles[i], className, "", styles[i],  0, 0, 0, 0, parentWindow, 0,  GetModuleHandleA(0),  NULL );
        ok(childWindow != NULL, "Failed to create child window, error: %u\n", GetLastError());

        ok(test_thick_child_got_minmax, "Got no WM_GETMINMAXINFO\n");

        SetLastError(0xdeadbeef);
        success = GetWindowRect(childWindow, &childRect);
        ok(success,"GetWindowRect call failed, error: %u\n", GetLastError());
        childWidth = childRect.right - childRect.left;
        childHeight = childRect.bottom - childRect.top;

        adjustedStyle = styles[i];
        if ((adjustedStyle & WS_CAPTION) == WS_CAPTION)
            adjustedStyle &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
        GetClientRect(GetParent(childWindow), &adjustedParentRect);
        AdjustWindowRectEx(&adjustedParentRect, adjustedStyle, FALSE, test_thick_child_exStyle);


        if (test_thick_child_style & (WS_DLGFRAME | WS_BORDER))
        {
            expectedWidth = GetSystemMetrics(SM_CXMINTRACK);
            expectedHeight = GetSystemMetrics(SM_CYMINTRACK);
        }
        else
        {
            expectedWidth = -2 * adjustedParentRect.left;
            expectedHeight = -2 * adjustedParentRect.top;
        }

        ok((childWidth == expectedWidth) && (childHeight == expectedHeight),
            "size of window (%s) is wrong: expected size %dx%d != actual size %dx%d\n",
            test_thick_child_name, expectedWidth, expectedHeight, childWidth, childHeight);

        SetLastError(0xdeadbeef);
        success = DestroyWindow(childWindow);
        ok(success,"DestroyWindow call failed, error: %u\n", GetLastError());
    }
    ok(UnregisterClassA(className, GetModuleHandleA(NULL)),"UnregisterClass call failed\n");
}

static void test_handles( HWND full_hwnd )
{
    HWND hwnd = full_hwnd;
    BOOL ret;
    RECT rect;

    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( ret, "GetWindowRect failed for %p err %u\n", hwnd, GetLastError() );

#ifdef _WIN64
    if ((ULONG_PTR)full_hwnd >> 32)
        hwnd = (HWND)((ULONG_PTR)full_hwnd & ~0u);
    else
        hwnd = (HWND)((ULONG_PTR)full_hwnd | ((ULONG_PTR)~0u << 32));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( ret, "GetWindowRect failed for %p err %u\n", hwnd, GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & ~0u) | ((ULONG_PTR)0x1234 << 32));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( ret, "GetWindowRect failed for %p err %u\n", hwnd, GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & 0xffff) | ((ULONG_PTR)0x9876 << 16));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( !ret, "GetWindowRect succeeded for %p\n", hwnd );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & 0xffff) | ((ULONG_PTR)0x12345678 << 16));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( !ret, "GetWindowRect succeeded for %p\n", hwnd );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );
#endif
}

static void test_winregion(void)
{
    HWND hwnd;
    RECT r;
    int ret, width;
    HRGN hrgn;

    if (!pGetWindowRgnBox)
    {
        win_skip("GetWindowRgnBox not supported\n");
        return;
    }

    hwnd = CreateWindowExA(0, "static", NULL, WS_VISIBLE, 10, 10, 10, 10, NULL, 0, 0, NULL);
    /* NULL prect */
    SetLastError(0xdeadbeef);
    ret = pGetWindowRgnBox(hwnd, NULL);
    ok( ret == ERROR, "Expected ERROR, got %d\n", ret);
    ok( GetLastError() == 0xdeadbeef, "Expected , got %d\n", GetLastError());

    hrgn = CreateRectRgn(2, 3, 10, 15);
    ok( hrgn != NULL, "Region creation failed\n");
    if (hrgn)
    {
        SetWindowRgn(hwnd, hrgn, FALSE);

        SetLastError(0xdeadbeef);
        ret = pGetWindowRgnBox(hwnd, NULL);
        ok( ret == ERROR, "Expected ERROR, got %d\n", ret);
        ok( GetLastError() == 0xdeadbeef, "Expected , got %d\n", GetLastError());

        SetRectEmpty(&r);
        ret = pGetWindowRgnBox(hwnd, &r);
        ok( ret == SIMPLEREGION, "Expected SIMPLEREGION, got %d\n", ret);
        ok( r.left == 2 && r.top == 3 && r.right == 10 && r.bottom == 15,
           "Expected (2,3)-(10,15), got %s\n", wine_dbgstr_rect( &r ));
        if (pMirrorRgn)
        {
            hrgn = CreateRectRgn(2, 3, 10, 15);
            ret = pMirrorRgn( hwnd, hrgn );
            ok( ret == TRUE, "MirrorRgn failed %u\n", ret );
            SetRectEmpty(&r);
            GetWindowRect( hwnd, &r );
            width = r.right - r.left;
            SetRectEmpty(&r);
            ret = GetRgnBox( hrgn, &r );
            ok( ret == SIMPLEREGION, "GetRgnBox failed %u\n", ret );
            ok( r.left == width - 10 && r.top == 3 && r.right == width - 2 && r.bottom == 15,
                "Wrong rectangle %s for width %d\n", wine_dbgstr_rect( &r ), width );
        }
        else win_skip( "MirrorRgn not supported\n" );
    }
    DestroyWindow(hwnd);
}

static void test_rtl_layout(void)
{
    HWND parent, child;
    RECT r;
    POINT pt;

    if (!pSetProcessDefaultLayout)
    {
        win_skip( "SetProcessDefaultLayout not supported\n" );
        return;
    }

    parent = CreateWindowExA(WS_EX_LAYOUTRTL, "static", NULL, WS_POPUP, 100, 100, 300, 300, NULL, 0, 0, NULL);
    child = CreateWindowExA(0, "static", NULL, WS_CHILD, 10, 10, 20, 20, parent, 0, 0, NULL);

    GetWindowRect( parent, &r );
    ok( r.left == 100 && r.right == 400, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    GetClientRect( parent, &r );
    ok( r.left == 0 && r.right == 300, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    GetClientRect( child, &r );
    ok( r.left == 0 && r.right == 20, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    MapWindowPoints( child, parent, (POINT *)&r, 2 );
    ok( r.left == 10 && r.right == 30, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    GetWindowRect( child, &r );
    ok( r.left == 370 && r.right == 390, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    MapWindowPoints( NULL, parent, (POINT *)&r, 2 );
    ok( r.left == 10 && r.right == 30, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    GetWindowRect( child, &r );
    MapWindowPoints( NULL, parent, (POINT *)&r, 1 );
    MapWindowPoints( NULL, parent, (POINT *)&r + 1, 1 );
    ok( r.left == 30 && r.right == 10, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    pt.x = pt.y = 12;
    MapWindowPoints( child, parent, &pt, 1 );
    ok( pt.x == 22 && pt.y == 22, "wrong point %d,%d\n", pt.x, pt.y );
    SetWindowPos( parent, 0, 0, 0, 250, 250, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
    GetWindowRect( parent, &r );
    ok( r.left == 100 && r.right == 350, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    GetWindowRect( child, &r );
    ok( r.left == 320 && r.right == 340, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    SetWindowLongW( parent, GWL_EXSTYLE, 0 );
    GetWindowRect( child, &r );
    ok( r.left == 320 && r.right == 340, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    MapWindowPoints( NULL, parent, (POINT *)&r, 2 );
    ok( r.left == 220 && r.right == 240, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    SetWindowLongW( parent, GWL_EXSTYLE, WS_EX_LAYOUTRTL );
    GetWindowRect( child, &r );
    ok( r.left == 320 && r.right == 340, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    MapWindowPoints( NULL, parent, (POINT *)&r, 2 );
    ok( r.left == 10 && r.right == 30, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    SetWindowPos( child, 0, 0, 0, 30, 30, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
    GetWindowRect( child, &r );
    ok( r.left == 310 && r.right == 340, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    MapWindowPoints( NULL, parent, (POINT *)&r, 2 );
    ok( r.left == 10 && r.right == 40, "wrong rect %s\n", wine_dbgstr_rect( &r ));
    DestroyWindow( child );
    DestroyWindow( parent );
}

static void test_FlashWindow(void)
{
    HWND hwnd;
    BOOL ret;
    if (!pFlashWindow)
    {
        win_skip( "FlashWindow not supported\n" );
        return;
    }

    hwnd = CreateWindowExA( 0, "MainWindowClass", "FlashWindow", WS_POPUP,
                            0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pFlashWindow( NULL, TRUE );
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "FlashWindow returned with %d\n", GetLastError() );

    DestroyWindow( hwnd );

    SetLastError( 0xdeadbeef );
    ret = pFlashWindow( hwnd, TRUE );
    ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "FlashWindow returned with %d\n", GetLastError() );
}

static void test_FlashWindowEx(void)
{
    HWND hwnd;
    FLASHWINFO finfo;
    BOOL prev, ret;

    if (!pFlashWindowEx)
    {
        win_skip( "FlashWindowEx not supported\n" );
        return;
    }

    hwnd = CreateWindowExA( 0, "MainWindowClass", "FlashWindow", WS_POPUP,
                            0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );

    finfo.cbSize = sizeof(FLASHWINFO);
    finfo.dwFlags = FLASHW_TIMER;
    finfo.uCount = 3;
    finfo.dwTimeout = 200;
    finfo.hwnd = NULL;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "FlashWindowEx returned with %d\n", GetLastError());

    finfo.hwnd = hwnd;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(NULL);
    ok(!ret && GetLastError() == ERROR_NOACCESS,
       "FlashWindowEx returned with %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    todo_wine ok(!ret, "previous window state should not be active\n");

    finfo.cbSize = sizeof(FLASHWINFO) - 1;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "FlashWindowEx succeeded\n");

    finfo.cbSize = sizeof(FLASHWINFO) + 1;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "FlashWindowEx succeeded\n");
    finfo.cbSize = sizeof(FLASHWINFO);

    DestroyWindow( hwnd );

    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "FlashWindowEx returned with %d\n", GetLastError());

    ok(finfo.cbSize == sizeof(FLASHWINFO), "FlashWindowEx modified cdSize to %x\n", finfo.cbSize);
    ok(finfo.hwnd == hwnd, "FlashWindowEx modified hwnd to %p\n", finfo.hwnd);
    ok(finfo.dwFlags == FLASHW_TIMER, "FlashWindowEx modified dwFlags to %x\n", finfo.dwFlags);
    ok(finfo.uCount == 3, "FlashWindowEx modified uCount to %x\n", finfo.uCount);
    ok(finfo.dwTimeout == 200, "FlashWindowEx modified dwTimeout to %x\n", finfo.dwTimeout);

    hwnd = CreateWindowExA( 0, "MainWindowClass", "FlashWindow", WS_VISIBLE | WS_POPUPWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );
    finfo.hwnd = hwnd;

    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(NULL);
    ok(!ret && GetLastError() == ERROR_NOACCESS,
       "FlashWindowEx returned with %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    prev = pFlashWindowEx(&finfo);

    ok(finfo.cbSize == sizeof(FLASHWINFO), "FlashWindowEx modified cdSize to %x\n", finfo.cbSize);
    ok(finfo.hwnd == hwnd, "FlashWindowEx modified hwnd to %p\n", finfo.hwnd);
    ok(finfo.dwFlags == FLASHW_TIMER, "FlashWindowEx modified dwFlags to %x\n", finfo.dwFlags);
    ok(finfo.uCount == 3, "FlashWindowEx modified uCount to %x\n", finfo.uCount);
    ok(finfo.dwTimeout == 200, "FlashWindowEx modified dwTimeout to %x\n", finfo.dwTimeout);

    finfo.dwFlags = FLASHW_STOP;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(prev != ret, "previous window state should be different\n");

    DestroyWindow( hwnd );
}

static void test_FindWindowEx(void)
{
    HWND hwnd, found;

    hwnd = CreateWindowExA( 0, "MainWindowClass", "caption", WS_POPUP, 0,0,0,0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "" );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "caption" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    hwnd = CreateWindowExA( 0, "MainWindowClass", NULL, WS_POPUP, 0,0,0,0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %d\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %u WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    /* test behaviour with a window title that is an empty character */
    found = FindWindowExA( 0, 0, "Shell_TrayWnd", "" );
    ok( found != NULL, "found is NULL, expected a valid hwnd\n" );
    found = FindWindowExA( 0, 0, "Shell_TrayWnd", NULL );
    ok( found != NULL, "found is NULL, expected a valid hwnd\n" );
}

static void test_GetLastActivePopup(void)
{
    HWND hwndOwner, hwndPopup1, hwndPopup2;

    hwndOwner = CreateWindowExA(0, "MainWindowClass", NULL,
                                WS_VISIBLE | WS_POPUPWINDOW,
                                100, 100, 200, 200,
                                NULL, 0, GetModuleHandleA(NULL), NULL);
    hwndPopup1 = CreateWindowExA(0, "MainWindowClass", NULL,
                                 WS_VISIBLE | WS_POPUPWINDOW,
                                 100, 100, 200, 200,
                                 hwndOwner, 0, GetModuleHandleA(NULL), NULL);
    hwndPopup2 = CreateWindowExA(0, "MainWindowClass", NULL,
                                 WS_VISIBLE | WS_POPUPWINDOW,
                                 100, 100, 200, 200,
                                 hwndPopup1, 0, GetModuleHandleA(NULL), NULL);
    ok( GetLastActivePopup(hwndOwner) == hwndPopup2, "wrong last active popup\n" );
    DestroyWindow( hwndPopup2 );
    DestroyWindow( hwndPopup1 );
    DestroyWindow( hwndOwner );
}

static LRESULT WINAPI my_httrasparent_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static LRESULT WINAPI my_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void create_window_tree(HWND parent, HWND *window, int size)
{
    static const DWORD style[] = { 0, WS_VISIBLE, WS_DISABLED, WS_VISIBLE | WS_DISABLED };
    int i, pos;

    memset(window, 0, size * sizeof(window[0]));

    pos = 0;
    for (i = 0; i < sizeof(style)/sizeof(style[0]); i++)
    {
        assert(pos < size);
        window[pos] = CreateWindowExA(0, "my_window", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "my_window", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;

        assert(pos < size);
        window[pos] = CreateWindowExA(0, "my_httrasparent", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "my_httrasparent", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;

        assert(pos < size);
        window[pos] = CreateWindowExA(0, "my_button", NULL, style[i] | WS_CHILD | BS_GROUPBOX,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "my_button", NULL, style[i] | WS_CHILD | BS_GROUPBOX,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(0, "my_button", NULL, style[i] | WS_CHILD | BS_PUSHBUTTON,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "my_button", NULL, style[i] | WS_CHILD | BS_PUSHBUTTON,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;

        assert(pos < size);
        window[pos] = CreateWindowExA(0, "Button", NULL, style[i] | WS_CHILD | BS_GROUPBOX,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "Button", NULL, style[i] | WS_CHILD | BS_GROUPBOX,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(0, "Button", NULL, style[i] | WS_CHILD | BS_PUSHBUTTON,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "Button", NULL, style[i] | WS_CHILD | BS_PUSHBUTTON,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;

        assert(pos < size);
        window[pos] = CreateWindowExA(0, "Static", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
        assert(pos < size);
        window[pos] = CreateWindowExA(WS_EX_TRANSPARENT, "Static", NULL, style[i] | WS_CHILD,
                                     0, 0, 100, 100, parent, 0, 0, NULL);
        ok(window[pos] != 0, "CreateWindowEx failed\n");
        pos++;
    }
}

struct window_attributes
{
    char class_name[128];
    BOOL is_visible, is_enabled, is_groupbox, is_httransparent, is_extransparent;
};

static void get_window_attributes(HWND hwnd, struct window_attributes *attrs)
{
    DWORD style, ex_style, hittest;

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    attrs->class_name[0] = 0;
    GetClassNameA(hwnd, attrs->class_name, sizeof(attrs->class_name));
    hittest = SendMessageA(hwnd, WM_NCHITTEST, 0, 0);

    attrs->is_visible = (style & WS_VISIBLE) != 0;
    attrs->is_enabled = (style & WS_DISABLED) == 0;
    attrs->is_groupbox = !lstrcmpiA(attrs->class_name, "Button") && (style & BS_TYPEMASK) == BS_GROUPBOX;
    attrs->is_httransparent = hittest == HTTRANSPARENT;
    attrs->is_extransparent = (ex_style & WS_EX_TRANSPARENT) != 0;
}

static int window_to_index(HWND hwnd, HWND *window, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (!window[i]) break;
        if (window[i] == hwnd) return i;
    }
    return -1;
}

static void test_child_window_from_point(void)
{
    static const int real_child_pos[] = { 14,15,16,17,18,19,20,21,24,25,26,27,42,43,
                                          44,45,46,47,48,49,52,53,54,55,51,50,23,22,-1 };
    static const int real_child_pos_nt4[] = { 14,15,16,17,20,21,24,25,26,27,42,43,44,45,
                                              48,49,52,53,54,55,51,50,47,46,23,22,19,18,-1 };
    WNDCLASSA cls;
    HWND hwnd, parent, window[100];
    POINT pt;
    int found_invisible, found_disabled, found_groupbox, found_httransparent, found_extransparent;
    int ret, i;

    ret = GetClassInfoA(0, "Button", &cls);
    ok(ret, "GetClassInfo(Button) failed\n");
    cls.lpszClassName = "my_button";
    ret = RegisterClassA(&cls);
    ok(ret, "RegisterClass(my_button) failed\n");

    cls.lpszClassName = "my_httrasparent";
    cls.lpfnWndProc = my_httrasparent_proc;
    ret = RegisterClassA(&cls);
    ok(ret, "RegisterClass(my_httrasparent) failed\n");

    cls.lpszClassName = "my_window";
    cls.lpfnWndProc = my_window_proc;
    ret = RegisterClassA(&cls);
    ok(ret, "RegisterClass(my_window) failed\n");

    parent = CreateWindowExA(0, "MainWindowClass", NULL,
                            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE,
                            100, 100, 200, 200,
                            0, 0, GetModuleHandleA(NULL), NULL);
    ok(parent != 0, "CreateWindowEx failed\n");
    trace("parent %p\n", parent);

    create_window_tree(parent, window, sizeof(window)/sizeof(window[0]));

    found_invisible = 0;
    found_disabled = 0;
    found_groupbox = 0;
    found_httransparent = 0;
    found_extransparent = 0;

    /* FIXME: also test WindowFromPoint, ChildWindowFromPoint, ChildWindowFromPointEx */
    for (i = 0; i < sizeof(real_child_pos)/sizeof(real_child_pos[0]); i++)
    {
        struct window_attributes attrs;

        pt.x = pt.y = 50;
        hwnd = RealChildWindowFromPoint(parent, pt);
        ok(hwnd != 0, "RealChildWindowFromPoint failed\n");
        ret = window_to_index(hwnd, window, sizeof(window)/sizeof(window[0]));
        /* FIXME: remove once Wine is fixed */
        todo_wine_if (ret != real_child_pos[i])
            ok(ret == real_child_pos[i] || broken(ret == real_child_pos_nt4[i]), "expected %d, got %d\n", real_child_pos[i], ret);

        get_window_attributes(hwnd, &attrs);
        if (!attrs.is_visible) found_invisible++;
        if (!attrs.is_enabled) found_disabled++;
        if (attrs.is_groupbox) found_groupbox++;
        if (attrs.is_httransparent) found_httransparent++;
        if (attrs.is_extransparent) found_extransparent++;

        if (ret != real_child_pos[i] && ret != -1)
        {
            trace("found hwnd %p (%s), is_visible %d, is_enabled %d, is_groupbox %d, is_httransparent %d, is_extransparent %d\n",
                  hwnd, attrs.class_name, attrs.is_visible, attrs.is_enabled, attrs.is_groupbox, attrs.is_httransparent, attrs.is_extransparent);
            get_window_attributes(window[real_child_pos[i]], &attrs);
            trace("expected hwnd %p (%s), is_visible %d, is_enabled %d, is_groupbox %d, is_httransparent %d, is_extransparent %d\n",
                  window[real_child_pos[i]], attrs.class_name, attrs.is_visible, attrs.is_enabled, attrs.is_groupbox, attrs.is_httransparent, attrs.is_extransparent);
        }
        if (ret == -1)
        {
            ok(hwnd == parent, "expected %p, got %p\n", parent, hwnd);
            break;
        }
        DestroyWindow(hwnd);
    }

    DestroyWindow(parent);

    ok(!found_invisible, "found %d invisible windows\n", found_invisible);
    ok(found_disabled, "found %d disabled windows\n", found_disabled);
todo_wine
    ok(found_groupbox == 4, "found %d groupbox windows\n", found_groupbox);
    ok(found_httransparent, "found %d httransparent windows\n", found_httransparent);
todo_wine
    ok(found_extransparent, "found %d extransparent windows\n", found_extransparent);

    ret = UnregisterClassA("my_button", cls.hInstance);
    ok(ret, "UnregisterClass(my_button) failed\n");
    ret = UnregisterClassA("my_httrasparent", cls.hInstance);
    ok(ret, "UnregisterClass(my_httrasparent) failed\n");
    ret = UnregisterClassA("my_window", cls.hInstance);
    ok(ret, "UnregisterClass(my_window) failed\n");
}

static void simulate_click(int x, int y)
{
    INPUT input[2];
    UINT events_no;

    SetCursorPos(x, y);
    memset(input, 0, sizeof(input));
    input[0].type = INPUT_MOUSE;
    U(input[0]).mi.dx = x;
    U(input[0]).mi.dy = y;
    U(input[0]).mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    U(input[1]).mi.dx = x;
    U(input[1]).mi.dy = y;
    U(input[1]).mi.dwFlags = MOUSEEVENTF_LEFTUP;
    events_no = SendInput(2, input, sizeof(input[0]));
    ok(events_no == 2, "SendInput returned %d\n", events_no);
}

static WNDPROC def_static_proc;
static BOOL got_hittest;
static LRESULT WINAPI static_hook_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if(msg == WM_NCHITTEST)
        got_hittest = TRUE;
    if(msg == WM_LBUTTONDOWN)
        ok(0, "unexpected call\n");

    return def_static_proc(hwnd, msg, wp, lp);
}

static void window_from_point_proc(HWND parent)
{
    HANDLE start_event, end_event;
    HANDLE win, child_static, child_button;
    BOOL got_click;
    DWORD ret;
    POINT pt;
    MSG msg;

    start_event = OpenEventA(EVENT_ALL_ACCESS, FALSE, "test_wfp_start");
    ok(start_event != 0, "OpenEvent failed\n");
    end_event = OpenEventA(EVENT_ALL_ACCESS, FALSE, "test_wfp_end");
    ok(end_event != 0, "OpenEvent failed\n");

    child_static = CreateWindowExA(0, "static", "static", WS_CHILD | WS_VISIBLE,
            0, 0, 100, 100, parent, 0, NULL, NULL);
    ok(child_static != 0, "CreateWindowEx failed\n");
    pt.x = pt.y = 150;
    win = WindowFromPoint(pt);
    ok(win == parent, "WindowFromPoint returned %p, expected %p\n", win, parent);

    child_button = CreateWindowExA(0, "button", "button", WS_CHILD | WS_VISIBLE,
            100, 0, 100, 100, parent, 0, NULL, NULL);
    ok(child_button != 0, "CreateWindowEx failed\n");
    pt.x = 250;
    win = WindowFromPoint(pt);
    ok(win == child_button, "WindowFromPoint returned %p, expected %p\n", win, child_button);

    /* without this window simulate click test keeps sending WM_NCHITTEST
     * message to child_static in an infinite loop */
    win = CreateWindowExA(0, "button", "button", WS_CHILD | WS_VISIBLE,
            0, 0, 100, 100, parent, 0, NULL, NULL);
    ok(win != 0, "CreateWindowEx failed\n");
    def_static_proc = (void*)SetWindowLongPtrA(child_static,
            GWLP_WNDPROC, (LONG_PTR)static_hook_proc);
    flush_events(TRUE);
    SetEvent(start_event);

    got_hittest = FALSE;
    got_click = FALSE;
    while(!got_click && wait_for_message(&msg)) {
        if(msg.message == WM_LBUTTONUP) {
            ok(msg.hwnd == win, "msg.hwnd = %p, expected %p\n", msg.hwnd, win);
            got_click = TRUE;
        }
        DispatchMessageA(&msg);
    }
    ok(got_hittest, "transparent window didn't get WM_NCHITTEST message\n");
    ok(got_click, "button under static window didn't get WM_LBUTTONUP\n");

    ret = WaitForSingleObject(end_event, 5000);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", ret);

    CloseHandle(start_event);
    CloseHandle(end_event);
}

static void test_window_from_point(const char *argv0)
{
    HWND hwnd, child, win;
    POINT pt;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char cmd[MAX_PATH];
    HANDLE start_event, end_event;

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL, WS_POPUP | WS_VISIBLE,
            100, 100, 200, 100, 0, 0, NULL, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    pt.x = pt.y = 150;
    win = WindowFromPoint(pt);
    pt.x = 250;
    if(win == hwnd)
        win = WindowFromPoint(pt);
    if(win != hwnd) {
        skip("there's another window covering test window\n");
        DestroyWindow(hwnd);
        return;
    }

    child = CreateWindowExA(0, "static", "static", WS_CHILD | WS_VISIBLE,
            0, 0, 100, 100, hwnd, 0, NULL, NULL);
    ok(child != 0, "CreateWindowEx failed\n");
    pt.x = pt.y = 150;
    win = WindowFromPoint(pt);
    ok(win == hwnd, "WindowFromPoint returned %p, expected %p\n", win, hwnd);
    DestroyWindow(child);

    child = CreateWindowExA(0, "button", "button", WS_CHILD | WS_VISIBLE,
                0, 0, 100, 100, hwnd, 0, NULL, NULL);
    ok(child != 0, "CreateWindowEx failed\n");
    win = WindowFromPoint(pt);
    ok(win == child, "WindowFromPoint returned %p, expected %p\n", win, child);
    DestroyWindow(child);

    start_event = CreateEventA(NULL, FALSE, FALSE, "test_wfp_start");
    ok(start_event != 0, "CreateEvent failed\n");
    end_event = CreateEventA(NULL, FALSE, FALSE, "test_wfp_end");
    ok(start_event != 0, "CreateEvent failed\n");

    sprintf(cmd, "%s win create_children %p\n", argv0, hwnd);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    ok(CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL,
                &startup, &info), "CreateProcess failed.\n");
    ok(wait_for_event(start_event, 1000), "didn't get start_event\n");

    child = GetWindow(hwnd, GW_CHILD);
    win = WindowFromPoint(pt);
    ok(win == child, "WindowFromPoint returned %p, expected %p\n", win, child);

    simulate_click(150, 150);
    flush_events(TRUE);

    child = GetWindow(child, GW_HWNDNEXT);
    pt.x = 250;
    win = WindowFromPoint(pt);
    ok(win == child, "WindowFromPoint returned %p, expected %p\n", win, child);

    SetEvent(end_event);
    winetest_wait_child_process(info.hProcess);
    CloseHandle(start_event);
    CloseHandle(end_event);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    DestroyWindow(hwnd);
}

static void test_map_points(void)
{
    BOOL ret;
    POINT p;
    HWND wnd, wnd0, dwnd;
    INT n;
    DWORD err;
    POINT pos = { 100, 200 };
    int width = 150;
    int height = 150;
    RECT window_rect;
    RECT client_rect;

    /* Create test windows */
    wnd = CreateWindowA("static", "test1", WS_POPUP, pos.x, pos.y, width, height, NULL, NULL, NULL, NULL);
    ok(wnd != NULL, "Failed %p\n", wnd);
    wnd0 = CreateWindowA("static", "test2", WS_POPUP, 0, 0, width, height, NULL, NULL, NULL, NULL);
    ok(wnd0 != NULL, "Failed %p\n", wnd);
    dwnd = CreateWindowA("static", "test3", 0, 200, 300, 150, 150, NULL, NULL, NULL, NULL);
    DestroyWindow(dwnd);
    ok(dwnd != NULL, "Failed %p\n", dwnd);

    /* Verify window rect and client rect (they should have the same width and height) */
    GetWindowRect(wnd, &window_rect);
    ok(window_rect.left == pos.x, "left is %d instead of %d\n", window_rect.left, pos.x);
    ok(window_rect.top == pos.y, "top is %d instead of %d\n", window_rect.top, pos.y);
    ok(window_rect.right == pos.x + width, "right is %d instead of %d\n", window_rect.right, pos.x + width);
    ok(window_rect.bottom == pos.y + height, "bottom is %d instead of %d\n", window_rect.bottom, pos.y + height);
    GetClientRect(wnd, &client_rect);
    ok(client_rect.left == 0, "left is %d instead of 0\n", client_rect.left);
    ok(client_rect.top == 0, "top is %d instead of 0\n", client_rect.top);
    ok(client_rect.right == width, "right is %d instead of %d\n", client_rect.right, width);
    ok(client_rect.bottom == height, "bottom is %d instead of %d\n", client_rect.bottom, height);

    /* Test MapWindowPoints */

    /* MapWindowPoints(NULL or wnd, NULL or wnd, NULL, 1); crashes on Windows */

    SetLastError(0xdeadbeef);
    n = MapWindowPoints(NULL, NULL, NULL, 0);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    n = MapWindowPoints(wnd, wnd, NULL, 0);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    n = MapWindowPoints(wnd, NULL, NULL, 0);
    ok(n == MAKELONG(window_rect.left, window_rect.top), "Got %x, expected %x\n",
       n, MAKELONG(window_rect.left, window_rect.top));

    n = MapWindowPoints(NULL, wnd, NULL, 0);
    ok(n == MAKELONG(-window_rect.left, -window_rect.top), "Got %x, expected %x\n",
       n, MAKELONG(-window_rect.left, -window_rect.top));

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, wnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(NULL, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(wnd, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(NULL, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(wnd, wnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    p.x = p.y = 100;
    n = MapWindowPoints(wnd, NULL, &p, 1);
    ok(n == MAKELONG(window_rect.left, window_rect.top), "Got %x, expected %x\n",
       n, MAKELONG(window_rect.left, window_rect.top));
    ok((p.x == (window_rect.left + 100)) && (p.y == (window_rect.top + 100)), "Failed got (%d, %d), expected (%d, %d)\n",
       p.x, p.y, window_rect.left + 100, window_rect.top + 100);

    p.x = p.y = 100;
    n = MapWindowPoints(NULL, wnd, &p, 1);
    ok(n == MAKELONG(-window_rect.left, -window_rect.top), "Got %x, expected %x\n",
       n, MAKELONG(-window_rect.left, -window_rect.top));
    ok((p.x == (-window_rect.left + 100)) && (p.y == (-window_rect.top + 100)), "Failed got (%d, %d), expected (%d, %d)\n",
       p.x, p.y, -window_rect.left + 100, -window_rect.top + 100);

    SetLastError(0xdeadbeef);
    p.x = p.y = 0;
    n = MapWindowPoints(wnd0, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %x,  expected 0\n", n);
    ok((p.x == 0) && (p.y == 0), "Failed got (%d, %d), expected (0, 0)\n", p.x, p.y);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    p.x = p.y = 0;
    n = MapWindowPoints(NULL, wnd0, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %x,  expected 0\n", n);
    ok((p.x == 0) && (p.y == 0), "Failed got (%d, %d), expected (0, 0)\n", p.x, p.y);
    ok(err == 0xdeadbeef, "Got %x, expected %x\n", err, 0xdeadbeef);

    /* Test ClientToScreen */

    /* ClientToScreen(wnd, NULL); crashes on Windows */

    SetLastError(0xdeadbeef);
    ret = ClientToScreen(NULL, NULL);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ClientToScreen(NULL, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ClientToScreen(dwnd, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    p.x = p.y = 100;
    ret = ClientToScreen(wnd, &p);
    ok(ret, "Failed with error %u\n", GetLastError());
    ok((p.x == (window_rect.left + 100)) && (p.y == (window_rect.top + 100)), "Failed got (%d, %d), expected (%d, %d)\n",
       p.x, p.y, window_rect.left + 100, window_rect.top + 100);

    p.x = p.y = 0;
    ret = ClientToScreen(wnd0, &p);
    ok(ret, "Failed with error %u\n", GetLastError());
    ok((p.x == 0) && (p.y == 0), "Failed got (%d, %d), expected (0, 0)\n", p.x, p.y);

    /* Test ScreenToClient */

    /* ScreenToClient(wnd, NULL); crashes on Windows */

    SetLastError(0xdeadbeef);
    ret = ScreenToClient(NULL, NULL);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ScreenToClient(NULL, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ScreenToClient(dwnd, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%d, %d), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %x, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    p.x = p.y = 100;
    ret = ScreenToClient(wnd, &p);
    ok(ret, "Failed with error %u\n", GetLastError());
    ok((p.x == (-window_rect.left + 100)) && (p.y == (-window_rect.top + 100)), "Failed got(%d, %d), expected (%d, %d)\n",
       p.x, p.y, -window_rect.left + 100, -window_rect.top + 100);

    p.x = p.y = 0;
    ret = ScreenToClient(wnd0, &p);
    ok(ret, "Failed with error %u\n", GetLastError());
    ok((p.x == 0) && (p.y == 0), "Failed got (%d, %d), expected (0, 0)\n", p.x, p.y);

    DestroyWindow(wnd);
    DestroyWindow(wnd0);
}

static void test_update_region(void)
{
    HWND hwnd, parent, child;
    HRGN  rgn1, rgn2;
    const RECT rc = {15, 15, 40, 40};
    const POINT wnd_orig = {30, 20};
    const POINT child_orig = {10, 5};

    parent = CreateWindowExA(0, "MainWindowClass", NULL,
                WS_VISIBLE | WS_CLIPCHILDREN,
                0, 0, 300, 150, NULL, NULL, GetModuleHandleA(0), 0);
    hwnd = CreateWindowExA(0, "MainWindowClass", NULL,
                WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD,
                0, 0, 200, 100, parent, NULL, GetModuleHandleA(0), 0);
    child = CreateWindowExA(0, "MainWindowClass", NULL,
                WS_VISIBLE | WS_CHILD,
                child_orig.x, child_orig.y,  100, 50,
                hwnd, NULL, GetModuleHandleA(0), 0);
    assert(parent && hwnd && child);

    ValidateRgn(parent, NULL);
    ValidateRgn(hwnd, NULL);
    InvalidateRect(hwnd, &rc, FALSE);
    ValidateRgn(child, NULL);

    rgn1 = CreateRectRgn(0, 0, 0, 0);
    ok(GetUpdateRgn(parent, rgn1, FALSE) == NULLREGION,
            "has invalid area after ValidateRgn(NULL)\n");
    GetUpdateRgn(hwnd, rgn1, FALSE);
    rgn2 = CreateRectRgnIndirect(&rc);
    ok(EqualRgn(rgn1, rgn2), "assigned and retrieved update regions are different\n");
    ok(GetUpdateRgn(child, rgn2, FALSE) == NULLREGION,
            "has invalid area after ValidateRgn(NULL)\n");

    SetWindowPos(hwnd, 0, wnd_orig.x, wnd_orig.y,  0, 0,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

    /* parent now has non-simple update region, it consist of
     * two rects, that was exposed after hwnd moving ... */
    SetRectRgn(rgn1, 0, 0, 200, wnd_orig.y);
    SetRectRgn(rgn2, 0, 0, wnd_orig.x, 100);
    CombineRgn(rgn1, rgn1, rgn2, RGN_OR);
    /* ... and mapped hwnd's invalid area, that hwnd has before moving */
    SetRectRgn(rgn2, rc.left + wnd_orig.x, rc.top + wnd_orig.y,
            rc.right + wnd_orig.x, rc.bottom + wnd_orig.y);
    CombineRgn(rgn1, rgn1, rgn2, RGN_OR);
    GetUpdateRgn(parent, rgn2, FALSE);
todo_wine
    ok(EqualRgn(rgn1, rgn2), "wrong update region\n");

    /* hwnd has the same invalid region as before moving */
    SetRectRgn(rgn1, rc.left, rc.top, rc.right, rc.bottom);
    GetUpdateRgn(hwnd, rgn2, FALSE);
    ok(EqualRgn(rgn1, rgn2), "wrong update region\n");

    /* hwnd's invalid area maps to child during moving */
    SetRectRgn(rgn1, rc.left - child_orig.x , rc.top - child_orig.y,
            rc.right - child_orig.x, rc.bottom - child_orig.y);
    GetUpdateRgn(child, rgn2, FALSE);
todo_wine
    ok(EqualRgn(rgn1, rgn2), "wrong update region\n");

    DeleteObject(rgn1);
    DeleteObject(rgn2);
    DestroyWindow(parent);
}

static void test_window_without_child_style(void)
{
    HWND hwnd;

    hwnd = CreateWindowExA(0, "edit", NULL, WS_VISIBLE|WS_CHILD,
            0, 0, 50, 50, hwndMain, NULL, 0, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");

    ok(SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) & (~WS_CHILD)),
            "can't remove WS_CHILD style\n");

    SetActiveWindow(hwndMain);
    PostMessageW(hwnd, WM_LBUTTONUP, 0, 0);
    SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    check_active_state(hwnd, hwnd, hwnd);
    flush_events(TRUE);

    DestroyWindow(hwnd);
}


struct smresult_thread_data
{
    HWND main_hwnd;
    HWND thread_hwnd;
    HANDLE thread_started;
    HANDLE thread_got_wm_app;
    HANDLE main_in_wm_app_1;
    HANDLE thread_replied;
};


static LRESULT WINAPI smresult_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_APP:
    {
        struct smresult_thread_data *data = (struct smresult_thread_data*)lparam;

        ok(hwnd == data->thread_hwnd, "unexpected hwnd %p\n", hwnd);

        SendNotifyMessageA(data->main_hwnd, WM_APP+1, 0, lparam);

        /* Don't return until the main thread is processing our sent message. */
        ok(WaitForSingleObject(data->main_in_wm_app_1, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

        /* Break the PeekMessage loop so we can notify the main thread after we return. */
        SetEvent(data->thread_got_wm_app);

        return 0x240408ea;
    }
    case WM_APP+1:
    {
        struct smresult_thread_data *data = (struct smresult_thread_data*)lparam;
        LRESULT res;

        ok(hwnd == data->main_hwnd, "unexpected hwnd %p\n", hwnd);

        /* Ask the thread to reply to our WM_APP message. */
        SetEvent(data->main_in_wm_app_1);

        /* Wait until the thread has sent a reply. */
        ok(WaitForSingleObject(data->thread_replied, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

        /* Send another message while we have a reply queued for the current one. */
        res = SendMessageA(data->thread_hwnd, WM_APP+2, 0, lparam);
        ok(res == 0x449b0190, "unexpected result %lx\n", res);

        return 0;
    }
    case WM_APP+2:
    {
        struct smresult_thread_data *data = (struct smresult_thread_data*)lparam;

        ok(hwnd == data->thread_hwnd, "unexpected hwnd %p\n", hwnd);

        /* Don't return until we know the main thread is processing sent messages. */
        SendMessageA(data->main_hwnd, WM_NULL, 0, 0);

        return 0x449b0190;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI smresult_thread_proc(void *param)
{
    MSG msg;
    struct smresult_thread_data *data = param;

    data->thread_hwnd = CreateWindowExA(0, "SmresultClass", "window caption text", WS_OVERLAPPEDWINDOW,
                                      100, 100, 200, 200, 0, 0, 0, NULL);
    ok(data->thread_hwnd != 0, "Failed to create overlapped window\n");

    SetEvent(data->thread_started);

    /* Loop until we've processed WM_APP. */
    while (WaitForSingleObject(data->thread_got_wm_app, 0) != WAIT_OBJECT_0)
    {
        if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        else
        {
            MsgWaitForMultipleObjects(1, &data->thread_got_wm_app, FALSE, INFINITE, QS_SENDMESSAGE);
        }
    }

    /* Notify the main thread that we replied to its WM_APP message. */
    SetEvent(data->thread_replied);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

static void test_smresult(void)
{
    WNDCLASSA cls;
    HANDLE hThread;
    DWORD tid;
    struct smresult_thread_data data;
    BOOL ret;
    LRESULT res;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = smresult_wndproc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "SmresultClass";

    ret = RegisterClassA(&cls);
    ok(ret, "RegisterClassA failed\n");

    data.thread_started = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(data.thread_started != NULL, "CreateEventA failed\n");

    data.thread_got_wm_app = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(data.thread_got_wm_app != NULL, "CreateEventA failed\n");

    data.main_in_wm_app_1 = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(data.main_in_wm_app_1 != NULL, "CreateEventA failed\n");

    data.thread_replied = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(data.thread_replied != NULL, "CreateEventA failed\n");

    data.main_hwnd = CreateWindowExA(0, "SmresultClass", "window caption text", WS_OVERLAPPEDWINDOW,
                                      100, 100, 200, 200, 0, 0, 0, NULL);

    hThread = CreateThread(NULL, 0, smresult_thread_proc, &data, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(data.thread_started, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    res = SendMessageA(data.thread_hwnd, WM_APP, 0, (LPARAM)&data);
    ok(res == 0x240408ea, "unexpected result %lx\n", res);

    SendMessageA(data.thread_hwnd, WM_CLOSE, 0, 0);

    DestroyWindow(data.main_hwnd);

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    CloseHandle(data.thread_started);
    CloseHandle(data.thread_got_wm_app);
    CloseHandle(data.main_in_wm_app_1);
    CloseHandle(data.thread_replied);
}

static void test_GetMessagePos(void)
{
    HWND button;
    DWORD pos;
    MSG msg;

    button = CreateWindowExA(0, "button", "button", WS_VISIBLE,
            100, 100, 100, 100, 0, 0, 0, NULL);
    ok(button != 0, "CreateWindowExA failed\n");

    SetCursorPos(120, 140);
    flush_events(TRUE);
    pos = GetMessagePos();
    ok(pos == MAKELONG(120, 140), "pos = %08x\n", pos);

    SetCursorPos(340, 320);
    pos = GetMessagePos();
    ok(pos == MAKELONG(120, 140), "pos = %08x\n", pos);

    SendMessageW(button, WM_APP, 0, 0);
    pos = GetMessagePos();
    ok(pos == MAKELONG(120, 140), "pos = %08x\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08x\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    SetCursorPos(350, 330);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08x\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    SetCursorPos(320, 340);
    PostMessageA(button, WM_APP+1, 0, 0);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08x\n", pos);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(350, 330), "pos = %08x\n", pos);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP+1, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(320, 340), "pos = %08x\n", pos);

    SetTimer(button, 1, 250, NULL);
    SetCursorPos(330, 350);
    GetMessageA(&msg, button, 0, 0);
    while (msg.message == WM_PAINT)
    {
        UpdateWindow( button );
        GetMessageA(&msg, button, 0, 0);
    }
    ok(msg.message == WM_TIMER, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(330, 350), "pos = %08x\n", pos);
    KillTimer(button, 1);

    DestroyWindow(button);
}

#define SET_FOREGROUND_STEAL_1          0x01
#define SET_FOREGROUND_SET_1            0x02
#define SET_FOREGROUND_STEAL_2          0x04
#define SET_FOREGROUND_SET_2            0x08
#define SET_FOREGROUND_INJECT           0x10

struct set_foreground_thread_params
{
    UINT msg_quit, msg_command;
    HWND window1, window2, thread_window;
    HANDLE command_executed;
};

static DWORD WINAPI set_foreground_thread(void *params)
{
    struct set_foreground_thread_params *p = params;
    MSG msg;

    p->thread_window = CreateWindowExA(0, "static", "thread window", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 10, 10, 0, 0, 0, NULL);
    SetEvent(p->command_executed);

    while(GetMessageA(&msg, 0, 0, 0))
    {
        if (msg.message == p->msg_quit)
            break;

        if (msg.message == p->msg_command)
        {
            if (msg.wParam & SET_FOREGROUND_STEAL_1)
            {
                SetForegroundWindow(p->thread_window);
                check_wnd_state(p->thread_window, p->thread_window, p->thread_window, 0);
            }
            if (msg.wParam & SET_FOREGROUND_INJECT)
            {
                SendNotifyMessageA(p->window1, WM_ACTIVATEAPP, 0, 0);
            }
            if (msg.wParam & SET_FOREGROUND_SET_1)
            {
                SetForegroundWindow(p->window1);
                check_wnd_state(0, p->window1, 0, 0);
            }
            if (msg.wParam & SET_FOREGROUND_STEAL_2)
            {
                SetForegroundWindow(p->thread_window);
                check_wnd_state(p->thread_window, p->thread_window, p->thread_window, 0);
            }
            if (msg.wParam & SET_FOREGROUND_SET_2)
            {
                SetForegroundWindow(p->window2);
                check_wnd_state(0, p->window2, 0, 0);
            }

            SetEvent(p->command_executed);
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    DestroyWindow(p->thread_window);
    return 0;
}

static void test_activateapp(HWND window1)
{
    HWND window2, test_window;
    HANDLE thread;
    struct set_foreground_thread_params thread_params;
    DWORD tid;
    MSG msg;

    window2 = CreateWindowExA(0, "static", "window 2", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            300, 0, 10, 10, 0, 0, 0, NULL);
    thread_params.msg_quit = WM_USER;
    thread_params.msg_command = WM_USER + 1;
    thread_params.window1 = window1;
    thread_params.window2 = window2;
    thread_params.command_executed = CreateEventW(NULL, FALSE, FALSE, NULL);

    thread = CreateThread(NULL, 0, set_foreground_thread, &thread_params, 0, &tid);
    WaitForSingleObject(thread_params.command_executed, INFINITE);

    SetForegroundWindow(window1);
    check_wnd_state(window1, window1, window1, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

    /* Steal foreground: WM_ACTIVATEAPP(0) is delivered. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_STEAL_1, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    test_window = GetForegroundWindow();
    ok(test_window == thread_params.thread_window, "Expected foreground window %p, got %p\n",
            thread_params.thread_window, test_window);
    /* Active and Focus window are sometimes 0 on KDE. Ignore them.
     * check_wnd_state(window1, thread_params.thread_window, window1, 0); */
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    check_wnd_state(0, thread_params.thread_window, 0, 0);
    test_window = GetForegroundWindow();
    ok(test_window == thread_params.thread_window, "Expected foreground window %p, got %p\n",
            thread_params.thread_window, test_window);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    /* This message is reliable on Windows and inside a virtual desktop.
     * It is unreliable on KDE (50/50) and never arrives on FVWM.
     * ok(app_deactivated, "Expected WM_ACTIVATEAPP(0), did not receive it.\n"); */

    /* Set foreground: WM_ACTIVATEAPP (1) is delivered. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_SET_1, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    check_wnd_state(0, 0, 0, 0);
    test_window = GetForegroundWindow();
    ok(!test_window, "Expected foreground window 0, got %p\n", test_window);
    ok(!app_activated, "Received WM_ACTIVATEAPP(!= 0), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    check_wnd_state(window1, window1, window1, 0);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    ok(app_activated, "Expected WM_ACTIVATEAPP(1), did not receive it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");

    /* Steal foreground then set it back: No messages are delivered. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_STEAL_1 | SET_FOREGROUND_SET_1, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");

    /* This is not implemented with a plain WM_ACTIVATEAPP filter. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_STEAL_1
            | SET_FOREGROUND_INJECT | SET_FOREGROUND_SET_1, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(app_deactivated, "Expected WM_ACTIVATEAPP(0), did not receive it.\n");

    SetForegroundWindow(thread_params.thread_window);

    /* Set foreground then remove: Both messages are delivered. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_SET_1 | SET_FOREGROUND_STEAL_2, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    test_window = GetForegroundWindow();
    ok(test_window == thread_params.thread_window, "Expected foreground window %p, got %p\n",
            thread_params.thread_window, test_window);
    check_wnd_state(0, thread_params.thread_window, 0, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    test_window = GetForegroundWindow();
    ok(test_window == thread_params.thread_window, "Expected foreground window %p, got %p\n",
            thread_params.thread_window, test_window);
    /* Active and focus are window1 on wine because the internal WM_WINE_SETACTIVEWINDOW(0)
     * message is never generated. GetCapture() returns 0 though, so we'd get a test success
     * in todo_wine in the line below.
     * todo_wine check_wnd_state(0, thread_params.thread_window, 0, 0); */
    ok(app_activated, "Expected WM_ACTIVATEAPP(1), did not receive it.\n");
    todo_wine ok(app_deactivated, "Expected WM_ACTIVATEAPP(0), did not receive it.\n");

    SetForegroundWindow(window1);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);

    /* Switch to a different window from the same thread? No messages. */
    app_activated = app_deactivated = FALSE;
    PostThreadMessageA(tid, thread_params.msg_command, SET_FOREGROUND_STEAL_1 | SET_FOREGROUND_SET_2, 0);
    WaitForSingleObject(thread_params.command_executed, INFINITE);
    test_window = GetForegroundWindow();
    ok(test_window == window1, "Expected foreground window %p, got %p\n",
            window1, test_window);
    check_wnd_state(window1, window1, window1, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    test_window = GetForegroundWindow();
    ok(test_window == window2, "Expected foreground window %p, got %p\n",
            window2, test_window);
    check_wnd_state(window2, window2, window2, 0);
    ok(!app_activated, "Received WM_ACTIVATEAPP(1), did not expect it.\n");
    ok(!app_deactivated, "Received WM_ACTIVATEAPP(0), did not expect it.\n");

    PostThreadMessageA(tid, thread_params.msg_quit, 0, 0);
    WaitForSingleObject(thread, INFINITE);

    CloseHandle(thread_params.command_executed);
    DestroyWindow(window2);
}

static LRESULT WINAPI winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if(!hwnd) {
        int *count = (int*)lparam;
        (*count)++;
    }
    return 0;
}

static LRESULT WINAPI winproc_convA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if(msg == WM_SETTEXT)
    {
        const char *text = (const char*)lparam;

        ok(!wparam, "wparam = %08lx\n", wparam);
        ok(!strcmp(text, "text"), "WM_SETTEXT lparam = %s\n", text);
        return 1;
    }
    return 0;
}

static const WCHAR textW[] = {'t','e','x','t',0};
static LRESULT WINAPI winproc_convW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if(msg == WM_SETTEXT)
    {
        const WCHAR *text = (const WCHAR*)lparam;

        ok(!wparam, "wparam = %08lx\n", wparam);
        ok(!lstrcmpW(text, textW), "WM_SETTEXT lparam = %s\n", wine_dbgstr_w(text));
        return 1;
    }
    return 0;
}

static void test_winproc_handles(const char *argv0)
{
    static const WCHAR winproc_testW[] = {'w','i','n','p','r','o','c','_','t','e','s','t',0};

    HINSTANCE hinst = GetModuleHandleA(NULL);
    WNDCLASSA wnd_classA;
    WNDCLASSW wnd_classW;
    int count, ret;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char cmd[MAX_PATH];

    memset(&wnd_classA, 0, sizeof(wnd_classA));
    wnd_classA.lpszClassName = "winproc_test";
    wnd_classA.lpfnWndProc = winproc;
    ret = RegisterClassA(&wnd_classA);
    ok(ret, "RegisterClass failed with error %d\n", GetLastError());

    ret = GetClassInfoW(hinst, winproc_testW, &wnd_classW);
    ok(ret, "GetClassInfoW failed with error %d\n", GetLastError());
    ok(wnd_classA.lpfnWndProc != wnd_classW.lpfnWndProc,
            "winproc pointers should not be identical\n");

    count = 0;
    CallWindowProcA(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
    ok(count == 1, "winproc should be called once (%d)\n", count);
    count = 0;
    CallWindowProcW(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
    ok(count == 1, "winproc should be called once (%d)\n", count);

    ret = UnregisterClassW(winproc_testW, hinst);
    ok(ret, "UnregisterClass failed with error %d\n", GetLastError());

    /* crashes on 64-bit windows because lpfnWndProc handle is already freed */
    if (sizeof(void*) == 4)
    {
        count = 0;
        CallWindowProcA(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
        todo_wine ok(!count, "winproc should not be called (%d)\n", count);
        CallWindowProcW(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
        todo_wine ok(!count, "winproc should not be called (%d)\n", count);
    }

    sprintf(cmd, "%s win winproc_limit", argv0);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    ok(CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL,
                &startup, &info), "CreateProcess failed.\n");
    winetest_wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
}

static void test_winproc_limit(void)
{
    WNDPROC winproc_handle;
    LONG_PTR ret;
    HWND hwnd;
    int i;

    hwnd = CreateWindowExA(0, "static", "test", WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    ok(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)winproc),
            "SetWindowLongPtr failed\n");
    winproc_handle = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    ok(winproc_handle != winproc, "winproc pointers should not be identical\n");

    /* run out of winproc slots */
    for(i = 2; i<0xffff; i++)
    {
        ok(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, i), "SetWindowLongPtr failed (%d)\n", i);
        if(GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == i)
            break;
    }
    ok(i != 0xffff, "unable to run out of winproc slots\n");

    ret = SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)winproc_convA);
    ok(ret, "SetWindowLongPtr failed with error %d\n", GetLastError());
    ok(SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"text"), "WM_SETTEXT failed\n");
    ok(SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW), "WM_SETTEXT with conversion failed\n");

    ret = SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)winproc_convW);
    ok(ret, "SetWindowLongPtr failed with error %d\n", GetLastError());
    ok(SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"text"), "WM_SETTEXT failed\n");
    ok(SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW), "WM_SETTEXT with conversion failed\n");

    /* Show that there's no message conversion when CallWindowProc is used */
    ok(CallWindowProcA(winproc_convW, hwnd, WM_SETTEXT, 0, (LPARAM)textW) == 1,
            "winproc_convW returned error\n");
    ok(CallWindowProcW(winproc_convW, hwnd, WM_SETTEXT, 0, (LPARAM)textW) == 1,
            "winproc_convW returned error\n");

    i = 0;
    CallWindowProcA(winproc_handle, 0, 0, 0, (LPARAM)&i);
    ok(i == 1, "winproc should be called once (%d)\n", i);
    i = 0;
    CallWindowProcW(winproc_handle, 0, 0, 0, (LPARAM)&i);
    ok(i == 1, "winproc should be called once (%d)\n", i);

    DestroyWindow(hwnd);

    i = 0;
    CallWindowProcA(winproc_handle, 0, 0, 0, (LPARAM)&i);
    ok(i == 1, "winproc should be called once (%d)\n", i);
    i = 0;
    CallWindowProcW(winproc_handle, 0, 0, 0, (LPARAM)&i);
    ok(i == 1, "winproc should be called once (%d)\n", i);
}

static void test_deferwindowpos(void)
{
    HDWP hdwp, hdwp2;
    BOOL ret;

    hdwp = BeginDeferWindowPos(0);
    ok(hdwp != NULL, "got %p\n", hdwp);

    ret = EndDeferWindowPos(NULL);
    ok(!ret, "got %d\n", ret);

    hdwp2 = DeferWindowPos(NULL, NULL, NULL, 0, 0, 10, 10, 0);
todo_wine
    ok(hdwp2 == NULL && ((GetLastError() == ERROR_INVALID_DWP_HANDLE) ||
        broken(GetLastError() == ERROR_INVALID_WINDOW_HANDLE) /* before win8 */), "got %p, error %d\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos((HDWP)0xdead, GetDesktopWindow(), NULL, 0, 0, 10, 10, 0);
todo_wine
    ok(hdwp2 == NULL && ((GetLastError() == ERROR_INVALID_DWP_HANDLE) ||
        broken(GetLastError() == ERROR_INVALID_WINDOW_HANDLE) /* before win8 */), "got %p, error %d\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, NULL, NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %d\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, GetDesktopWindow(), NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %d\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, (HWND)0xdead, NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %d\n", hdwp2, GetLastError());

    ret = EndDeferWindowPos(hdwp);
    ok(ret, "got %d\n", ret);
}

START_TEST(win)
{
    char **argv;
    int argc = winetest_get_mainargs( &argv );
    HMODULE user32 = GetModuleHandleA( "user32.dll" );
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
    pGetAncestor = (void *)GetProcAddress( user32, "GetAncestor" );
    pGetWindowInfo = (void *)GetProcAddress( user32, "GetWindowInfo" );
    pGetWindowModuleFileNameA = (void *)GetProcAddress( user32, "GetWindowModuleFileNameA" );
    pGetLayeredWindowAttributes = (void *)GetProcAddress( user32, "GetLayeredWindowAttributes" );
    pSetLayeredWindowAttributes = (void *)GetProcAddress( user32, "SetLayeredWindowAttributes" );
    pUpdateLayeredWindow = (void *)GetProcAddress( user32, "UpdateLayeredWindow" );
    pUpdateLayeredWindowIndirect = (void *)GetProcAddress( user32, "UpdateLayeredWindowIndirect" );
    pGetMonitorInfoA = (void *)GetProcAddress( user32,  "GetMonitorInfoA" );
    pMonitorFromPoint = (void *)GetProcAddress( user32,  "MonitorFromPoint" );
    pGetWindowRgnBox = (void *)GetProcAddress( user32, "GetWindowRgnBox" );
    pGetGUIThreadInfo = (void *)GetProcAddress( user32, "GetGUIThreadInfo" );
    pGetProcessDefaultLayout = (void *)GetProcAddress( user32, "GetProcessDefaultLayout" );
    pSetProcessDefaultLayout = (void *)GetProcAddress( user32, "SetProcessDefaultLayout" );
    pFlashWindow = (void *)GetProcAddress( user32, "FlashWindow" );
    pFlashWindowEx = (void *)GetProcAddress( user32, "FlashWindowEx" );
    pGetLayout = (void *)GetProcAddress( gdi32, "GetLayout" );
    pSetLayout = (void *)GetProcAddress( gdi32, "SetLayout" );
    pMirrorRgn = (void *)GetProcAddress( gdi32, "MirrorRgn" );

    if (argc==4 && !strcmp(argv[2], "create_children"))
    {
        HWND hwnd;

        sscanf(argv[3], "%p", &hwnd);
        window_from_point_proc(hwnd);
        return;
    }

    if (argc==3 && !strcmp(argv[2], "winproc_limit"))
    {
        test_winproc_limit();
        return;
    }

    if (!RegisterWindowClasses()) assert(0);

    hwndMain = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window",
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE,
                               100, 100, 200, 200,
                               0, 0, GetModuleHandleA(NULL), NULL);
    assert( hwndMain );

    if(!SetForegroundWindow(hwndMain)) {
        /* workaround for foreground lock timeout */
        simulate_click(101, 101);
        ok(SetForegroundWindow(hwndMain), "SetForegroundWindow failed\n");
    }

    SetLastError(0xdeafbeef);
    GetWindowLongPtrW(GetDesktopWindow(), GWLP_WNDPROC);
    is_win9x = (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED);

    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    if (!hhook) win_skip( "Cannot set CBT hook, skipping some tests\n" );

    /* make sure that these tests are executed first */
    test_FindWindowEx();
    test_SetParent();

    hwndMain2 = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window 2",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_POPUP,
                                100, 100, 200, 200,
                                0, 0, GetModuleHandleA(NULL), NULL);
    assert( hwndMain2 );

    our_pid = GetWindowThreadProcessId(hwndMain, NULL);

    /* Add the tests below this line */
    test_child_window_from_point();
    test_window_from_point(argv[0]);
    test_thick_child_size(hwndMain);
    test_fullscreen();
    test_hwnd_message();
    test_nonclient_area(hwndMain);
    test_params();
    test_GetWindowModuleFileName();
    test_capture_1();
    test_capture_2();
    test_capture_3(hwndMain, hwndMain2);
    test_capture_4();
    test_rtl_layout();
    test_FlashWindow();
    test_FlashWindowEx();

    test_CreateWindow();
    test_parent_owner();
    test_enum_thread_windows();

    test_mdi();
    test_icons();
    test_SetWindowPos(hwndMain, hwndMain2);
    test_SetMenu(hwndMain);
    test_SetFocus(hwndMain);
    test_SetActiveWindow(hwndMain);
    test_NCRedraw();

    test_children_zorder(hwndMain);
    test_popup_zorder(hwndMain2, hwndMain, WS_POPUP);
    test_popup_zorder(hwndMain2, hwndMain, 0);
    test_GetLastActivePopup();
    test_keyboard_input(hwndMain);
    test_mouse_input(hwndMain);
    test_validatergn(hwndMain);
    test_nccalcscroll( hwndMain);
    test_scrollwindow( hwndMain);
    test_scrollvalidate( hwndMain);
    test_scrolldc( hwndMain);
    test_scroll();
    test_IsWindowUnicode();
    test_vis_rgn(hwndMain);

    test_AdjustWindowRect();
    test_window_styles();
    test_dialog_styles();
    test_dialog_parent();
    test_redrawnow();
    test_csparentdc();
    test_SetWindowLong();
    test_set_window_style();
    test_ShowWindow();
    test_gettext();
    test_GetUpdateRect();
    test_Expose();
    test_layered_window();

    test_SetForegroundWindow(hwndMain);
    test_shell_window();
    test_handles( hwndMain );
    test_winregion();
    test_map_points();
    test_update_region();
    test_window_without_child_style();
    test_smresult();
    test_GetMessagePos();

    test_activateapp(hwndMain);
    test_winproc_handles(argv[0]);
    test_deferwindowpos();

    /* add the tests above this line */
    if (hhook) UnhookWindowsHookEx(hhook);

    DestroyWindow(hwndMain2);
    DestroyWindow(hwndMain);
}
