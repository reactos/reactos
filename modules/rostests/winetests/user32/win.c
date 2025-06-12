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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"

#include "wine/test.h"

#ifndef WM_SYSTIMER
#define WM_SYSTIMER 0x0118
#endif

#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR
#ifdef __REACTOS__
#define DCX_USESTYLE 0x00010000
#define flaky
#define flaky_wine
#endif

void dump_region(HRGN hrgn);

static BOOL (WINAPI *pGetWindowInfo)(HWND,WINDOWINFO*);
static UINT (WINAPI *pGetWindowModuleFileNameA)(HWND,LPSTR,UINT);
static BOOL (WINAPI *pGetLayeredWindowAttributes)(HWND,COLORREF*,BYTE*,DWORD*);
static BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
static BOOL (WINAPI *pUpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
static BOOL (WINAPI *pUpdateLayeredWindowIndirect)(HWND,const UPDATELAYEREDWINDOWINFO*);
static int  (WINAPI *pGetWindowRgnBox)(HWND,LPRECT);
static BOOL (WINAPI *pGetGUIThreadInfo)(DWORD, GUITHREADINFO*);
static BOOL (WINAPI *pGetProcessDefaultLayout)( DWORD *layout );
static BOOL (WINAPI *pSetProcessDefaultLayout)( DWORD layout );
static BOOL (WINAPI *pFlashWindow)( HWND hwnd, BOOL bInvert );
static BOOL (WINAPI *pFlashWindowEx)( PFLASHWINFO pfwi );
static BOOL (WINAPI *pMirrorRgn)(HWND hwnd, HRGN hrgn);
static BOOL (WINAPI *pGetWindowDisplayAffinity)(HWND hwnd, DWORD *affinity);
static BOOL (WINAPI *pSetWindowDisplayAffinity)(HWND hwnd, DWORD affinity);
static BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT,DWORD,BOOL,DWORD,UINT);
static BOOL (WINAPI *pSystemParametersInfoForDpi)(UINT,UINT,void*,UINT,UINT);
static HICON (WINAPI *pInternalGetWindowIcon)(HWND window, UINT type);

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

static void hold_key( int vk )
{
    BYTE kstate[256];
    BOOL res;

    res = GetKeyboardState( kstate );
    ok(res, "GetKeyboardState failed.\n");
    kstate[vk] |= 0x80;
    res = SetKeyboardState( kstate );
    ok(res, "SetKeyboardState failed.\n");
}

static void release_key( int vk )
{
    BYTE kstate[256];
    BOOL res;

    res = GetKeyboardState( kstate );
    ok(res, "GetKeyboardState failed.\n");
    kstate[vk] &= ~0x80;
    res = SetKeyboardState( kstate );
    ok(res, "SetKeyboardState failed.\n");
}

static void dump_minmax_info( const MINMAXINFO *minmax )
{
    trace("Reserved=%ld,%ld MaxSize=%ld,%ld MaxPos=%ld,%ld MinTrack=%ld,%ld MaxTrack=%ld,%ld\n",
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

/* check the values returned by the various parent/owner functions on a given window */
static void check_parents( HWND hwnd, HWND ga_parent, HWND gwl_parent, HWND get_parent,
                           HWND gw_owner, HWND ga_root, HWND ga_root_owner )
{
    HWND res;

    res = GetAncestor( hwnd, GA_PARENT );
    ok( res == ga_parent, "Wrong result for GA_PARENT %p expected %p\n", res, ga_parent );
    res = (HWND)GetWindowLongPtrA( hwnd, GWLP_HWNDPARENT );
    ok( res == gwl_parent, "Wrong result for GWL_HWNDPARENT %p expected %p\n", res, gwl_parent );
    res = GetParent( hwnd );
    ok( res == get_parent, "Wrong result for GetParent %p expected %p\n", res, get_parent );
    res = GetWindow( hwnd, GW_OWNER );
    ok( res == gw_owner, "Wrong result for GW_OWNER %p expected %p\n", res, gw_owner );
    res = GetAncestor( hwnd, GA_ROOT );
    ok( res == ga_root, "Wrong result for GA_ROOT %p expected %p\n", res, ga_root );
    res = GetAncestor( hwnd, GA_ROOTOWNER );
    ok( res == ga_root_owner, "Wrong result for GA_ROOTOWNER %p expected %p\n", res, ga_root_owner );
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
        ok_(file, line)(foreground == GetForegroundWindow(), "GetForegroundWindow() = %p\n", GetForegroundWindow());
    ok_(file, line)(focus == GetFocus(), "GetFocus() = %p\n", GetFocus());
}

static BOOL ignore_message( UINT message, HWND hwnd )
{
    WCHAR buffer[256];

    if (GetClassNameW( hwnd, buffer, ARRAY_SIZE(buffer) ) == 22 &&
        !wcscmp( buffer, L"UserAdapterWindowClass" ))
        return TRUE;

    /* these are always ignored */
    return (message >= 0xc000 ||
            message == 0x0060 || /* Internal undocumented message introduced by Win11 */
            message == WM_GETICON ||
            message == WM_GETOBJECT ||
            message == WM_TIMER ||
            message == WM_SYSTIMER ||
            message == WM_TIMECHANGE ||
            message == WM_DEVICECHANGE);
}

static DWORD wait_for_events( DWORD count, HANDLE *events, DWORD timeout )
{
    DWORD ret, end = GetTickCount() + timeout;
    MSG msg;

    while ((ret = MsgWaitForMultipleObjects( count, events, FALSE, timeout, QS_ALLINPUT )) <= count)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageA( &msg );
        }
        if (ret < count) return ret;
        if (timeout == INFINITE) continue;
        if (end <= GetTickCount()) timeout = 0;
        else timeout = end - GetTickCount();
    }

    ok( ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    return ret;
}

static BOOL CALLBACK EnumChildProc( HWND hwndChild, LPARAM lParam)
{
    (*(LPINT)lParam)++;
    if (*(LPINT)lParam > 1) return FALSE;
    return TRUE;
}

/* will search for the given window */
static BOOL CALLBACK EnumChildProc1( HWND hwndChild, LPARAM lParam)
{
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

    if (winetest_debug > 1)
        trace( "main window %p main2 %p desktop %p child %p\n", hwndMain, hwndMain2, desktop, child );

    /* child without parent, should fail */
    SetLastError(0xdeadbeef);
    test = CreateWindowExA(0, "ToolWindowClass", "Tool window 1",
                           WS_CHILD, 0, 0, 100, 100, 0, 0, 0, NULL );
    ok( !test, "WS_CHILD without parent created\n" );
    ok( GetLastError() == ERROR_TLW_WITH_WSCHILD, "CreateWindowExA error %lu\n", GetLastError() );

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    style = GetWindowLongA( desktop, GWL_STYLE );
    ok( !SetWindowLongA( desktop, GWL_STYLE, WS_POPUP ), "Set GWL_STYLE on desktop succeeded\n" );
    ok( !SetWindowLongA( desktop, GWL_STYLE, 0 ), "Set GWL_STYLE on desktop succeeded\n" );
    ok( GetWindowLongA( desktop, GWL_STYLE ) == style, "Desktop style changed\n" );

    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    if (winetest_debug > 1) trace( "created child %p\n", test );
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
    if (winetest_debug > 1) trace( "created child of desktop %p\n", test );
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
    if (winetest_debug > 1) trace( "created child of child %p\n", test );
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
    if (winetest_debug > 1) trace( "created top-level %p\n", test );
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
    if (winetest_debug > 1) trace( "created owned top-level %p\n", test );
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
    if (winetest_debug > 1) trace( "created popup %p\n", test );
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
    if (winetest_debug > 1) trace( "created owned popup %p\n", test );
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
    if (winetest_debug > 1) trace( "created top-level owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, 0, hwndMain, test, test );
    DestroyWindow( test );

    /* top-level window owned by child (same as owned by top-level) with WS_MAXIMIZE */
    test = create_tool_window( WS_MAXIMIZE, child );
    DestroyWindow( test );

    /* popup owned by desktop (same as not owned) */
    test = create_tool_window( WS_POPUP, desktop );
    if (winetest_debug > 1) trace( "created popup owned by desktop %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* popup owned by desktop (same as not owned) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, desktop );
    DestroyWindow( test );

    /* popup owned by child (same as owned by top-level) */
    test = create_tool_window( WS_POPUP, child );
    if (winetest_debug > 1) trace( "created popup owned by child %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /* popup owned by child (same as owned by top-level) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_MAXIMIZE, child );
    DestroyWindow( test );

    /* not owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, 0 );
    if (winetest_debug > 1) trace( "created WS_CHILD popup %p\n", test );
    check_parents( test, desktop, 0, 0, 0, test, test );
    DestroyWindow( test );

    /* not owned popup with WS_CHILD | WS_MAXIMIZE (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD | WS_MAXIMIZE, 0 );
    DestroyWindow( test );

    /* owned popup with WS_CHILD (same as WS_POPUP only) */
    test = create_tool_window( WS_POPUP | WS_CHILD, hwndMain );
    if (winetest_debug > 1) trace( "created owned WS_CHILD popup %p\n", test );
    check_parents( test, desktop, hwndMain, hwndMain, hwndMain, test, hwndMain );
    DestroyWindow( test );

    /* owned popup with WS_CHILD (same as WS_POPUP only) with WS_MAXIMIZE */
    test = create_tool_window( WS_POPUP | WS_CHILD | WS_MAXIMIZE, hwndMain );
    DestroyWindow( test );

    /******************** parent changes *************************/

    /* desktop window */
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)hwndMain2 );
    ok( !ret, "Set GWL_HWNDPARENT succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    ok( !SetParent( desktop, hwndMain ), "SetParent succeeded on desktop\n" );
    check_parents( desktop, 0, 0, 0, 0, 0, 0 );
    /* normal child window */
    test = create_tool_window( WS_CHILD, hwndMain );
    if (winetest_debug > 1) trace( "created child %p\n", test );

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
    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)test );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, (LONG_PTR)child );
    ok( ret == 0, "GWL_HWNDPARENT return value %p expected 0\n", ret );
    check_parents( test, desktop, child, desktop, child, test, desktop );

    ret = (HWND)SetWindowLongPtrA( test, GWLP_HWNDPARENT, 0 );
    ok( ret == child, "GWL_HWNDPARENT return value %p expected %p\n", ret, child );
    check_parents( test, desktop, 0, desktop, 0, test, desktop );
    DestroyWindow( test );

    /* not owned top-level window */
    test = create_tool_window( 0, 0 );
    if (winetest_debug > 1) trace( "created top-level %p\n", test );

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
    if (winetest_debug > 1) trace( "created popup %p\n", test );

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
    if (winetest_debug > 1) trace( "created child %p\n", test );

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
    if (winetest_debug > 1) trace( "created top-level %p\n", test );

    ret = SetParent( test, child );
    ok( ret == desktop, "SetParent return value %p expected %p\n", ret, desktop );
    check_parents( test, child, child, 0, 0, hwndMain, test );

    ShowWindow( test, SW_SHOW );
    ret = SetParent( test, test );
    ok( ret == NULL, "SetParent return value %p expected %p\n", ret, NULL );
    ok( GetWindowLongA( test, GWL_STYLE ) & WS_VISIBLE, "window is not visible after SetParent\n" );
    check_parents( test, child, child, 0, 0, hwndMain, test );
    DestroyWindow( test );

    /* owned popup */
    test = create_tool_window( WS_POPUP, hwndMain2 );
    if (winetest_debug > 1) trace( "created owned popup %p\n", test );

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
    if (winetest_debug > 1) trace( "created owner %p and popup %p\n", owner, test );
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
    if (winetest_debug > 1) trace( "created owner %p and popup %p\n", owner, test );
    check_parents( test, desktop, owner, owner, owner, test, owner );
    DestroyWindow( owner );
    ok( !IsWindow(test), "Window %p not destroyed by owner destruction\n", test );

    /* top-level popup owned by child */
    owner = create_tool_window( WS_CHILD, hwndMain2 );
    test = create_tool_window( WS_POPUP, 0 );
    if (winetest_debug > 1) trace( "created owner %p and popup %p\n", owner, test );
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
        ok( ret, "GetGUIThreadInfo failed without message queue\n" );
        SetLastError( 0xdeadbeef );
        info.cbSize = sizeof(info) + 1;
        ret = pGetGUIThreadInfo( GetCurrentThreadId(), &info );
        ok( !ret, "GetGUIThreadInfo succeeded with wrong size\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
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

struct test_thread_exit_parent_params
{
    HWND hwnd;
    HANDLE created_event;
    HANDLE stop_event;
};

static DWORD CALLBACK test_thread_exit_parent_thread( void *args )
{
    struct test_thread_exit_parent_params *params = args;
    DWORD ret;
    MSG msg;

    params->hwnd = CreateWindowW( L"static", L"parent", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                  100, 100, 200, 200, 0, 0, 0, NULL );
    ok( params->hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError() );
    flush_events( TRUE );
    SetEvent( params->created_event );

    do
    {
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        ret = MsgWaitForMultipleObjects( 1, &params->stop_event, FALSE, INFINITE, QS_ALLINPUT );
    }
    while (ret != WAIT_OBJECT_0);

    return 0;
}

static LRESULT CALLBACK test_thread_exit_wnd_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if (msg == WM_USER) return 0xdeadbeef;
    if (msg == WM_USER + 1) return 0xfeedcafe;
    return DefWindowProcW( hwnd, msg, wp, lp );
}

static void test_thread_exit_destroy(void)
{
    struct test_thread_exit_parent_params params;
    HWND adopter, child1, child2, child3, child;
    WNDPROC old_wndproc, wndproc;
    WCHAR buffer[MAX_PATH];
    HANDLE thread;
    DWORD ret;
    HRGN rgn;
    HWND tmp;
    MSG msg;

    params.created_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    params.stop_event = CreateEventW( NULL, FALSE, FALSE, NULL );

    adopter = CreateWindowW( L"static", L"adopter", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                             300, 100, 200, 200, 0, 0, 0, NULL );
    ok( adopter != 0, "CreateWindowExW failed, error %lu\n", GetLastError() );
    flush_events( TRUE );

    thread = CreateThread( NULL, 0, test_thread_exit_parent_thread, &params, 0, NULL );
    ok( thread != 0, "CreateThread failed, error %lu\n", GetLastError() );
    WaitForSingleObject( params.created_event, INFINITE );

    child1 = CreateWindowW( L"static", L"child1", WS_CHILD|WS_VISIBLE,
                            50, 50, 50, 50, params.hwnd, 0, 0, NULL );
    ok( child1 != 0, "CreateWindowExW failed, error %lu\n", GetLastError() );
    child2 = CreateWindowW( L"static", L"child2", WS_CHILD|WS_VISIBLE,
                            100, 50, 50, 50, params.hwnd, 0, 0, NULL );
    ok( child2 != 0, "CreateWindowExW failed, error %lu\n", GetLastError() );
    child3 = CreateWindowW( L"static", L"child3", WS_CHILD|WS_VISIBLE,
                            50, 100, 50, 50, params.hwnd, 0, 0, NULL );
    ok( child3 != 0, "CreateWindowExW failed, error %lu\n", GetLastError() );
    flush_events( TRUE );

    trace("parent %p adopter %p child1 %p child2 %p child3 %p\n", params.hwnd, adopter, child1, child2, child3);

    SetActiveWindow( child1 );
    SetFocus( child1 );
    SetCapture( child1 );

    ok( GetActiveWindow() == params.hwnd, "GetActiveWindow %p, expected %p\n", GetActiveWindow(), params.hwnd );
    ok( GetFocus() == child1, "GetFocus %p, expected %p\n", GetFocus(), child1 );
    ok( GetCapture() == child1, "GetCapture %p, expected %p\n", GetCapture(), child1 );

    ret = SetPropW( child1, L"myprop", UlongToHandle(0xdeadbeef) );
    ok( ret, "SetPropW failed, error %lu\n", GetLastError() );
    ret = SetPropW( child2, L"myprop", UlongToHandle(0xdeadbeef) );
    ok( ret, "SetPropW failed, error %lu\n", GetLastError() );

    old_wndproc = (WNDPROC)GetWindowLongPtrW( child1, GWLP_WNDPROC );
    ok( old_wndproc != NULL, "GetWindowLongPtrW GWLP_WNDPROC failed, error %lu\n", GetLastError() );

    ret = GetWindowLongW( child1, GWL_STYLE );
    ok( ret == (WS_CHILD|WS_VISIBLE), "GetWindowLongW returned %#lx\n", ret );

    SetEvent( params.stop_event );
    ret = WaitForSingleObject( thread, INFINITE );
    ok( ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( thread );

    /* child windows should all still be alive but hidden */
    ret = IsWindow( child1 );
    ok( ret, "IsWindow returned %lu\n", ret );
    ret = IsWindow( child2 );
    ok( ret, "IsWindow returned %lu\n", ret );
    ret = IsWindow( child3 );
    ok( ret, "IsWindow returned %lu\n", ret );

    todo_wine
    ok( GetActiveWindow() == adopter, "GetActiveWindow %p, expected %p\n", GetActiveWindow(), adopter );
    todo_wine
    ok( GetFocus() == adopter, "GetFocus %p, expected %p\n", GetFocus(), adopter );
    ok( GetCapture() == child1, "GetCapture %p, expected %p\n", GetCapture(), child1 );

    SetActiveWindow( child1 );
    SetFocus( child1 );
    SetCapture( child1 );

    todo_wine
    ok( GetActiveWindow() == adopter, "GetActiveWindow %p, expected %p\n", GetActiveWindow(), adopter );
    todo_wine
    ok( GetFocus() == adopter, "GetFocus %p, expected %p\n", GetFocus(), adopter );
    ok( GetCapture() == child1, "GetCapture %p, expected %p\n", GetCapture(), child1 );

    SetLastError( 0xdeadbeef );
    ret = GetWindowLongW( child1, GWL_STYLE );
    todo_wine
    ok( ret == WS_CHILD, "GetWindowLongW returned %#lx\n", ret );
    ok( GetLastError() == 0xdeadbeef, "GetWindowLongW error %lu\n", GetLastError() );
    ret = SetWindowLongW( child1, GWL_STYLE, WS_CHILD|WS_VISIBLE );
    ok( ret, "SetWindowLongW failed, error %lu\n", GetLastError() );
    ret = GetWindowLongW( child1, GWL_STYLE );
    ok( ret == (WS_CHILD|WS_VISIBLE), "GetWindowLongW returned %#lx\n", ret );

    /* and cannot be adopted */
    SetLastError( 0xdeadbeef );
    tmp = GetParent( child1 );
    ok( tmp == params.hwnd, "GetParent returned %p, error %lu\n", tmp, GetLastError() );
    ok( GetLastError() == 0xdeadbeef, "GetWindowLongW error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp = SetParent( child1, adopter );
    ok( tmp == 0, "SetParent returned %p\n", tmp );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp = SetParent( child3, adopter );
    ok( tmp == 0, "SetParent returned %p\n", tmp );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp = GetParent( child1 );
    ok( tmp == params.hwnd, "GetParent returned %p, error %lu\n", tmp, GetLastError() );
    ok( GetLastError() == 0xdeadbeef, "GetWindowLongW error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = GetWindowLongW( params.hwnd, GWL_STYLE );
    ok( ret == 0, "GetWindowLongW returned %#lx\n", ret );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetWindowLongW error %lu\n", GetLastError() );

    wndproc = (WNDPROC)GetWindowLongPtrW( child1, GWLP_WNDPROC );
    ok( wndproc != NULL, "GetWindowLongPtrW GWLP_WNDPROC failed, error %lu\n", GetLastError() );
    ok( wndproc == old_wndproc, "GetWindowLongPtrW GWLP_WNDPROC returned %p\n", wndproc );

    tmp = GetPropW( child1, L"myprop" );
    ok( HandleToULong(tmp) == 0xdeadbeef, "GetPropW returned %p\n", tmp );
    tmp = GetPropW( child2, L"myprop" );
    ok( HandleToULong(tmp) == 0xdeadbeef, "GetPropW returned %p\n", tmp );

    child = CreateWindowExA( 0, "ToolWindowClass", "Tool window 1", WS_CHILD,
                             0, 0, 100, 100, child1, 0, 0, NULL );
    ok( !child && GetLastError() == ERROR_INVALID_PARAMETER,
        "CreateWindowExA returned %p %lu\n", child, GetLastError() );

    ret = MoveWindow( child1, 5, 5, 10, 10, FALSE );
    ok( ret, "MoveWindow failed: %lu\n", GetLastError() );

    /* destroying child1 ourselves succeeds */
    ret = DestroyWindow( child1 );
    ok( ret, "DestroyWindow returned %lu\n", ret );
    ret = DestroyWindow( child1 );
    ok( !ret, "DestroyWindow returned %lu\n", ret );
    ret = IsWindow( child1 );
    ok( !ret, "IsWindow returned %lu\n", ret );

    tmp = GetPropW( child1, L"myprop" );
    ok( HandleToULong(tmp) == 0, "GetPropW returned %p\n", tmp );

    /* child2 is still alive, for now */
    ret = IsWindow( child2 );
    ok( ret, "IsWindow returned %lu\n", ret );

    SetLastError( 0xdeadbeef );
    ret = SetWindowPos( child2, HWND_TOPMOST, 0, 0, 100, 100, SWP_NOSIZE|SWP_NOMOVE );
    todo_wine
    ok( !ret, "SetWindowPos succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "SetWindowPos returned error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = SetWindowPos( child2, 0, 10, 10, 200, 200, SWP_NOZORDER | SWP_NOACTIVATE );
    todo_wine
    ok( !ret, "SetWindowPos succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "SetWindowPos returned error %lu\n", GetLastError() );

    rgn = CreateRectRgn( 5, 5, 15, 15 );
    SetLastError( 0xdeadbeef );
    ret = SetWindowRgn( child2, rgn, TRUE );
    ok( ret, "SetWindowRgn failed, error %lu\n", GetLastError() );
    DeleteObject( rgn );

    wndproc = (WNDPROC)SetWindowLongPtrW( child2, GWLP_WNDPROC, (LONG_PTR)test_thread_exit_wnd_proc );
    ret = SendMessageW( child2, WM_USER, 0, 0 );
    ok( ret == 0xdeadbeef, "SendMessageW returned %lu, error %lu\n", ret, GetLastError() );
    ret = SendMessageW( child2, WM_USER + 1, 0, 0 );
    ok( ret == 0xfeedcafe, "SendMessageW returned %lu, error %lu\n", ret, GetLastError() );
    ret = SendMessageW( child2, WM_USER + 2, 0, 0 );
    ok( ret == 0, "SendMessageW returned %lu, error %lu\n", ret, GetLastError() );

    ret = GetWindowTextW( child2, buffer, ARRAY_SIZE(buffer) );
    ok( ret == 6, "GetWindowTextW returned %lu\n", ret );
    ok( !wcscmp( buffer, L"child2" ), "GetWindowTextW returned %s\n", debugstr_w( buffer ) );
    ret = IsWindow( child2 );
    ok( ret, "IsWindow returned %lu\n", ret );

    /* but peeking any message should reap them all */
    PeekMessageW( &msg, child2, 0, 0, PM_REMOVE );

    tmp = GetPropW( child2, L"myprop" );
    ok( HandleToULong(tmp) == 0, "GetPropW returned %p\n", tmp );

    ret = IsWindow( child2 );
    ok( !ret, "IsWindow returned %lu\n", ret );
    ret = IsWindow( child3 );
    todo_wine
    ok( !ret, "IsWindow returned %lu\n", ret );
    ret = DestroyWindow( child2 );
    ok( !ret, "DestroyWindow returned %lu\n", ret );

    DestroyWindow( adopter );

    CloseHandle( params.created_event );
    CloseHandle( params.stop_event );
}

static struct wm_gettext_override_data
{
    BOOL   enabled; /* when 1 bypasses default procedure */
    BOOL   dont_terminate; /* don't null terminate returned string in WM_GETTEXT handler */
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
	    if (!(winpos->flags & SWP_NOSIZE))
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
	{
            LRESULT ret;
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
            MapWindowPoints(GetAncestor(hwnd,GA_PARENT), 0, (LPPOINT)&rc2, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
               wine_dbgstr_rect(&rc2));

            GetClientRect(hwnd, &rc2);
            ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            ok(!ret, "got %08Ix\n", ret);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rc1),
               wine_dbgstr_rect(&rc2));
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
            else if (g_wm_gettext_override.dont_terminate)
            {
                char *text = (char *)lparam;
                if (text)
                {
                    memcpy(text, "text", 4);
                    return 4;
                }
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
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
        }
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
            else if (g_wm_gettext_override.dont_terminate)
            {
                static const WCHAR textW[] = {'t','e','x','t'};
                WCHAR *text = (WCHAR *)lparam;
                if (text)
                {
                    memcpy(text, textW, sizeof(textW));
                    return 4;
                }
                return 0;
            }
            break;

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
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

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
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
       "wrong dwStyle: %08lx != %08lx for %p in hook %s\n",
       info->dwStyle, GetWindowLongA(hwnd, GWL_STYLE), hwnd, hook);
    /* Windows reports some undocumented exstyles in WINDOWINFO, but
     * doesn't return them in GetWindowLong(hwnd, GWL_EXSTYLE).
     */
    ok((info->dwExStyle & ~0xe0000800) == (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE),
       "wrong dwExStyle: %08lx != %08lx for %p in hook %s\n",
       info->dwExStyle, GetWindowLongA(hwnd, GWL_EXSTYLE), hwnd, hook);
    status = (GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0;
    if (GetForegroundWindow())
        ok(info->dwWindowStatus == status, "wrong dwWindowStatus: %04lx != %04lx active %p fg %p in hook %s\n",
           info->dwWindowStatus, status, GetActiveWindow(), GetForegroundWindow(), hook);

    /* win2k and XP return broken border info in GetWindowInfo most of
     * the time, so there is no point in testing it.
     */
if (0)
{
    UINT border;
    ok(info->cxWindowBorders == (unsigned)(rcClient.left - rcWindow.left),
       "wrong cxWindowBorders %d != %ld\n", info->cxWindowBorders, rcClient.left - rcWindow.left);
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

static void test_window_info(const char *hook, HWND hwnd)
{
    WINDOWINFO info, info2;

    if (0)      /* crashes on Win10 */
    ok(!pGetWindowInfo(hwnd, NULL), "GetWindowInfo should fail\n");

    if (0) {    /* crashes on XP, 2003 */
    SetLastError(0xdeadbeef);
    ok(!pGetWindowInfo(0, NULL), "GetWindowInfo should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
       "got error %ld expected ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ok(!pGetWindowInfo(0, &info), "GetWindowInfo should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
       "got error %ld expected ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());

    info.cbSize = sizeof(WINDOWINFO);
    ok(pGetWindowInfo(hwnd, &info), "GetWindowInfo should not fail\n");
    verify_window_info(hook, hwnd, &info);

    /* test different cbSize values.
     * Windows ignores it (except for Win98, according to an old comment).
     */
    memset(&info, 0xcc, sizeof(WINDOWINFO));
    memset(&info2, 0xcc, sizeof(WINDOWINFO));

    info.cbSize = sizeof(WINDOWINFO);
    info2.cbSize = sizeof(WINDOWINFO);

    ok(pGetWindowInfo(hwnd, &info), "GetWindowInfo should not fail\n");
    ok(pGetWindowInfo(hwnd, &info2), "GetWindowInfo should not fail\n");
    ok(memcmp(&info, &info2, sizeof(WINDOWINFO)) == 0, "identical GetWindowInfo calls produce different result\n");

    memset(&info2, 0xcc, sizeof(WINDOWINFO));
    info2.cbSize = 0;
    ok(pGetWindowInfo(hwnd, &info2), "GetWindowInfo should not fail\n");
    info2.cbSize = sizeof(WINDOWINFO);
    ok(memcmp(&info, &info2, sizeof(WINDOWINFO)) == 0, "GetWindowInfo cbSize should be ignored\n");

    memset(&info2, 0xcc, sizeof(WINDOWINFO));
    info2.cbSize = sizeof(WINDOWINFO)/2;
    ok(pGetWindowInfo(hwnd, &info2), "GetWindowInfo should not fail\n");
    info2.cbSize = sizeof(WINDOWINFO);
    ok(memcmp(&info, &info2, sizeof(WINDOWINFO)) == 0, "GetWindowInfo cbSize should be ignored\n");

    memset(&info2, 0xcc, sizeof(WINDOWINFO));
    info2.cbSize = UINT_MAX;
    ok(pGetWindowInfo(hwnd, &info2), "GetWindowInfo should not fail\n");
    info2.cbSize = sizeof(WINDOWINFO);
    ok(memcmp(&info, &info2, sizeof(WINDOWINFO)) == 0, "GetWindowInfo cbSize should be ignored\n");
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
    NONCLIENTMETRICSW ncm;
    int adjust = 0;

    ncm.cbSize = offsetof( NONCLIENTMETRICSW, iPaddedBorderWidth );
    SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((exStyle & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME) adjust += ncm.iBorderWidth; /* The resize border */
    if ((style & (WS_BORDER|WS_DLGFRAME)) || (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= ncm.iSmCaptionHeight + 1;
        else
            rect->top -= ncm.iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm.iMenuHeight + 1;

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

static void wine_AdjustWindowRectExForDpi( RECT *rect, LONG style, BOOL menu, LONG exStyle, UINT dpi )
{
    NONCLIENTMETRICSW ncm;
    int adjust = 0;

    ncm.cbSize = sizeof(ncm);
    pSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi );

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE)
        adjust = 1; /* for the outer frame always present */
    else if ((exStyle & WS_EX_DLGMODALFRAME) || (style & (WS_THICKFRAME|WS_DLGFRAME)))
        adjust = 2; /* outer */

    if (style & WS_THICKFRAME) adjust += ncm.iBorderWidth + ncm.iPaddedBorderWidth;

    if ((style & (WS_BORDER|WS_DLGFRAME)) || (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= ncm.iSmCaptionHeight + 1;
        else
            rect->top -= ncm.iCaptionHeight + 1;
    }
    if (menu) rect->top -= ncm.iMenuHeight + 1;

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
}

static int offset;

static LRESULT CALLBACK test_standard_scrollbar_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_NCPAINT:
    {
        HRGN region;
        RECT rect;

        GetWindowRect(hwnd, &rect);
        region = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom - offset);
        DefWindowProcA(hwnd, msg, (WPARAM)region, lp);
        DeleteObject(region);
        return 0;
    }

    case WM_NCCALCSIZE:
    {
        LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wp, lp);
        ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wp, lp);
        return ret;
    }

    default:
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

static void test_nonclient_area(HWND hwnd)
{
    BOOL (WINAPI *pIsThemeActive)(void);
    POINT point, old_cursor_pos;
    COLORREF color, old_color;
    BOOL is_theme_active;
    DWORD style, exstyle;
    RECT rc_window, rc_client, rc;
    HWND child, parent;
    HMODULE uxtheme;
    WNDCLASSA cls;
    BOOL menu;
    LRESULT ret;
    HDC hdc;

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
       "window rect does not match: style:exstyle=0x%08lx:0x%08lx, menu=%d, win=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_window), wine_dbgstr_rect(&rc));

    rc = rc_client;
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc, 2);
    wine_AdjustWindowRectEx(&rc, style, menu, exstyle);
    ok(EqualRect(&rc, &rc_window),
       "window rect does not match: style:exstyle=0x%08lx:0x%08lx, menu=%d, win=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_window), wine_dbgstr_rect(&rc));


    rc = rc_window;
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    ok(!ret, "got %08Ix\n", ret);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    ok(EqualRect(&rc, &rc_client),
       "client rect does not match: style:exstyle=0x%08lx:0x%08lx, menu=%d client=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_client), wine_dbgstr_rect(&rc));

    /* NULL rectangle shouldn't crash */
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, 0);
    ok(ret == 0, "NULL rectangle returned %Id instead of 0\n", ret);

    /* and now test AdjustWindowRectEx and WM_NCCALCSIZE on synthetic data */
    SetRect(&rc_client, 0, 0, 250, 150);
    rc_window = rc_client;
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc_window, 2);
    FixedAdjustWindowRectEx(&rc_window, style, menu, exstyle);

    rc = rc_window;
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    ok(!ret, "got %08Ix\n", ret);
    MapWindowPoints(0, hwnd, (LPPOINT)&rc, 2);
    ok(EqualRect(&rc, &rc_client),
       "synthetic rect does not match: style:exstyle=0x%08lx:0x%08lx, menu=%d, client=%s, calc=%s\n",
       style, exstyle, menu, wine_dbgstr_rect(&rc_client), wine_dbgstr_rect(&rc));

    /* Test standard scroll bars */
    uxtheme = LoadLibraryA("uxtheme.dll");
    ok(!!uxtheme, "Failed to load uxtheme.dll, error %lu.\n", GetLastError());
    pIsThemeActive = (void *)GetProcAddress(uxtheme, "IsThemeActive");
    ok(!!pIsThemeActive, "Failed to load IsThemeActive, error %lu.\n", GetLastError());
    is_theme_active = pIsThemeActive();

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA(0);
    cls.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    cls.lpszClassName = "TestStandardScrollbarParentClass";
    RegisterClassA(&cls);

    cls.lpfnWndProc = test_standard_scrollbar_proc;
    cls.hbrBackground = GetStockObject(GRAY_BRUSH);
    cls.lpszClassName = "TestStandardScrollbarClass";
    RegisterClassA(&cls);

    parent = CreateWindowA("TestStandardScrollbarParentClass", "parent", WS_POPUP | WS_VISIBLE, 100,
                           100, 100, 100, NULL, NULL, 0, NULL);
    ok(!!parent, "Failed to create a parent window, error %lu.\n", GetLastError());
    GetCursorPos(&old_cursor_pos);

    /* Place the cursor on the standard scroll bar arrow button when not painting it at all.
     * Expects the standard scroll bar not to appear before and after the cursor position change */
    offset = GetSystemMetrics(SM_CYHSCROLL);
    child = CreateWindowA("TestStandardScrollbarClass", "test", WS_CHILD | WS_HSCROLL | WS_VISIBLE,
                          0, 0, 50, 50, parent, NULL, 0, NULL);
    ok(!!child, "Failed to create a test window, error %lu.\n", GetLastError());
    hdc = GetDC(parent);
    ok(!!hdc, "GetDC failed, error %ld.\n", GetLastError());

    SetCursorPos(0, 0);
    flush_events(TRUE);
    RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ERASENOW | RDW_FRAME);
    old_color = GetPixel(hdc, 50 - GetSystemMetrics(SM_CXVSCROLL) / 2,
                         50 - GetSystemMetrics(SM_CYHSCROLL) / 2);
    ok(old_color == 0xc0c0c0, "Expected color %#x, got %#lx.\n", 0xc0c0c0, old_color);

    point.x = 50 - GetSystemMetrics(SM_CXVSCROLL) / 2;
    point.y = 50 - GetSystemMetrics(SM_CYHSCROLL) / 2;
    ClientToScreen(child, &point);
    SetCursorPos(point.x, point.y);
    flush_events(TRUE);
    RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ERASENOW | RDW_FRAME);
    color = GetPixel(hdc, 50 - GetSystemMetrics(SM_CXVSCROLL) / 2,
                     50 - GetSystemMetrics(SM_CYHSCROLL) / 2);
    ok(color == old_color, "Expected color %#lx, got %#lx.\n", old_color, color);

    ReleaseDC(parent, hdc);
    DestroyWindow(child);

    /* Place the cursor on standard scroll bar arrow button when painting 1 pixel of the scroll bar.
     * Expects the scroll bar to appear after the cursor position change */
    offset = GetSystemMetrics(SM_CYHSCROLL) - 1;
    child = CreateWindowA("TestStandardScrollbarClass", "test", WS_CHILD | WS_HSCROLL | WS_VISIBLE,
                          0, 0, 50, 50, parent, NULL, 0, NULL);
    ok(!!child, "Failed to create a test window, error %lu.\n", GetLastError());
    hdc = GetDC(parent);
    ok(!!hdc, "GetDC failed, error %ld.\n", GetLastError());

    SetCursorPos(0, 0);
    flush_events(TRUE);
    RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ERASENOW | RDW_FRAME);
    old_color = GetPixel(hdc, 50 - GetSystemMetrics(SM_CXVSCROLL) / 2,
                         50 - GetSystemMetrics(SM_CYHSCROLL) / 2);

    point.x = 50 - GetSystemMetrics(SM_CXVSCROLL) / 2;
    point.y = 50 - GetSystemMetrics(SM_CYHSCROLL) / 2;
    ClientToScreen(child, &point);
    SetCursorPos(point.x, point.y);
    flush_events(TRUE);
    RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ERASENOW | RDW_FRAME);
    color = GetPixel(hdc, 50 - GetSystemMetrics(SM_CXVSCROLL) / 2,
                     50 - GetSystemMetrics(SM_CYHSCROLL) / 2);
    if (is_theme_active)
        ok(color != old_color || broken(color == old_color), /* Win 10 1507 64-bit */
           "Got unexpected color %#lx.\n", color);
    else
        todo_wine
        ok(color == old_color, "Expected color %#lx, got %#lx.\n", old_color, color);

    SetCursorPos(old_cursor_pos.x, old_cursor_pos.y);
    ReleaseDC(parent, hdc);
    DestroyWindow(parent);
    UnregisterClassA("TestStandardScrollbarClass", GetModuleHandleA(0));
    UnregisterClassA("TestStandardScrollbarParentClass", GetModuleHandleA(0));
    FreeLibrary(uxtheme);
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
                test_window_info(code_name, hwnd);

	    /* WS_VISIBLE should be turned off yet */
	    style = createwnd->lpcs->style & ~WS_VISIBLE;
            ok(style == GetWindowLongA(hwnd, GWL_STYLE),
                    "style of hwnd and style in the CREATESTRUCT do not match: %08lx != %08lx\n",
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
		ok(GetAncestor(hwnd, GA_PARENT) == hwndMessage, "GA_PARENT should be set to hwndMessage at this point\n");
		ok(GetAncestor(hwnd, GA_ROOT) == hwnd,
		   "GA_ROOT is set to %p, expected %p\n", GetAncestor(hwnd, GA_ROOT), hwnd);

		if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
		    ok(GetAncestor(hwnd, GA_ROOTOWNER) == hwndMessage,
		       "GA_ROOTOWNER should be set to hwndMessage at this point\n");
		else
		    ok(GetAncestor(hwnd, GA_ROOTOWNER) == hwnd,
		       "GA_ROOTOWNER is set to %p, expected %p\n", GetAncestor(hwnd, GA_ROOTOWNER), hwnd);

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
	    test_window_info(code_name, hwnd);
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

static DWORD WINAPI test_shell_window_thread(LPVOID param)
{
    BOOL ret;
    HMODULE hinst, hUser32;
    BOOL (WINAPI*SetShellWindow)(HWND);
    HWND hwnd1, hwnd2, hwnd3, hwnd4, hwnd5;
    HWND shellWindow, nextWnd;
    HDESK testDesktop = (HDESK)param;

    SetThreadDesktop(testDesktop);

    shellWindow = GetShellWindow();
    hinst = GetModuleHandleA(NULL);
    hUser32 = GetModuleHandleA("user32");

    ok(shellWindow == NULL, "Newly created desktop has a shell window %p\n", shellWindow);

    SetShellWindow = (void *)GetProcAddress(hUser32, "SetShellWindow");

    hwnd1 = CreateWindowExA(0, "#32770", "TEST1", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 100, 100, 300, 200, 0, 0, hinst, 0);

    ret = SetShellWindow(hwnd1);
    ok(ret, "first call to SetShellWindow(hwnd1)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd1, "wrong shell window: %p/%p\n", shellWindow,hwnd1);

    ret = SetShellWindow(hwnd1);
    ok(!ret, "second call to SetShellWindow(hwnd1)\n");

    ret = SetShellWindow(0);
    /* passes on Win XP, but not on Win98
    DWORD error = GetLastError();
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
    ret = SetShellWindow(hwnd2);
    ok(!ret, "SetShellWindow(%p) with WS_EX_TOPMOST\n", hwnd2);

    hwnd3 = CreateWindowExA(0, "#32770", "TEST3", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 200, 400, 300, 200, 0, 0, hinst, 0);

    hwnd4 = CreateWindowExA(0, "#32770", "TEST4", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 250, 500, 300, 200, 0, 0, hinst, 0);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==hwnd3, "wrong next window for hwnd4 %p: %p - expected %p\n", hwnd4, nextWnd, hwnd3);

    ret = SetShellWindow(hwnd4);
    ok(ret, "SetShellWindow(hwnd4)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected %p\n", shellWindow, hwnd4);

    nextWnd = GetWindow(hwnd4, GW_HWNDNEXT);
    ok(nextWnd==0, "wrong next window for hwnd4: %p - expected 0\n", nextWnd);

    ret = SetWindowPos(hwnd4, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, HWND_TOPMOST)\n");

    ret = SetWindowPos(hwnd4, hwnd3, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos(hwnd4, hwnd3\n");

    ret = SetShellWindow(hwnd3);
    ok(!ret, "SetShellWindow(hwnd3)\n");
    shellWindow = GetShellWindow();
    ok(shellWindow==hwnd4, "wrong shell window: %p - expected %p\n", shellWindow, hwnd4);

    hwnd5 = CreateWindowExA(0, "#32770", "TEST5", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 300, 600, 300, 200, 0, 0, hinst, 0);
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

    return 0;
}

static void test_shell_window(void)
{
    HDESK hdesk;
    HWND orig_shell_window;
    HANDLE hthread;

    orig_shell_window = GetShellWindow();
    ok(orig_shell_window != NULL, "default desktop doesn't have a shell window\n");

    hdesk = CreateDesktopA("winetest", NULL, NULL, 0, GENERIC_ALL, NULL);

    hthread = CreateThread(NULL, 0, test_shell_window_thread, (LPVOID)hdesk, 0, NULL);

    WaitForSingleObject(hthread, INFINITE);

    CloseHandle(hthread);

    CloseDesktop(hdesk);

    if (!orig_shell_window)
        skip("no shell window on default desktop\n");
    else
        ok(GetShellWindow() == orig_shell_window, "changing shell window on another desktop effected the default\n");
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
    ok(id == first_id, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_cs.style = 0x7fffffff; /* without WS_POPUP */
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
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
        ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateMDIWindowA("MDI_child_Class_1", "MDI child",
                                 0,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 mdi_client, GetModuleHandleA(NULL),
                                 (LPARAM)mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(id == first_id, "wrong child id %Id\n", id);
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
        ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_1", "MDI child",
                                0,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(id == first_id, "wrong child id %Id\n", id);
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
        ok(id == first_id, "wrong child id %Id\n", id);
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
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == first_id, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    exp_hwnd = (GetWindowLongW(mdi_child, GWL_STYLE) & WS_VISIBLE) ? mdi_child : 0;
    ok(hwnd == exp_hwnd, "WM_MDIGETACTIVE should return %p, got %p\n", exp_hwnd, hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok(!IsWindow(mdi_child), "WM_MDIDESTROY failed\n");

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_Class_2", "MDI child",
                                WS_CHILD,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parent, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(!mdi_child, "WS_EX_MDICHILD with a not MDIClient parent should fail\n");

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD, /* without WS_POPUP */
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %Id\n", id);
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
    ok(id == 0, "wrong child id %Id\n", id);
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
    ok(id == 0, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %Id\n", id);
    hwnd = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(!hwnd, "WM_MDIGETACTIVE should return 0, got %p\n", hwnd);
    ok(GetMenuItemCount(frame_menu) == 0, "Got wrong frame menu item count: %u\n", GetMenuItemCount(frame_menu));
    DestroyWindow(mdi_child);

    mdi_child = CreateWindowExA(0, "MDI_child_Class_2", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION | WS_THICKFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(NULL),
                                mdi_lParam_test_message);
    ok(mdi_child != 0, "MDI child creation failed\n");
    id = GetWindowLongPtrA(mdi_child, GWLP_ID);
    ok(id == 0, "wrong child id %Id\n", id);
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
    ok(stack[0] == child_4 && stack[1] == child_3 &&
        stack[2] == child_2 && stack[3] == child_1,
        "Unexpected initial order, should be: %p->%p->%p->%p\n",
            child_4, child_3, child_2, child_1);

    SendMessageA(mdi_client, WM_MDINEXT, (WPARAM)child_3, 0);

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    stack[2] = GetWindow(stack[1], GW_HWNDNEXT);
    stack[3] = GetWindow(stack[2], GW_HWNDNEXT);
    ok(stack[0] == child_2 && stack[1] == child_4 &&
        stack[2] == child_1 && stack[3] == child_3,
        "Broken MDI child stack:\nexpected: %p->%p->%p->%p, but got: %p->%p->%p->%p\n",
            child_2, child_4, child_1, child_3, stack[0], stack[1], stack[2], stack[3]);

    SendMessageA(mdi_client, WM_MDINEXT, (WPARAM)child_1, 1);

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    stack[2] = GetWindow(stack[1], GW_HWNDNEXT);
    stack[3] = GetWindow(stack[2], GW_HWNDNEXT);
    ok(stack[0] == child_4 && stack[1] == child_2 &&
        stack[2] == child_1 && stack[3] == child_3,
        "Broken MDI child stack:\nexpected: %p->%p->%p->%p, but got: %p->%p->%p->%p\n",
            child_4, child_2, child_1, child_3, stack[0], stack[1], stack[2], stack[3]);

    ShowWindow(child_4, SW_MINIMIZE);

    stack[0] = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    stack[1] = GetWindow(stack[0], GW_HWNDNEXT);
    todo_wine ok(stack[0] == child_4, "Expected %p, got %p\n", child_4, stack[0]);
    todo_wine ok(stack[1] == NULL, "Expected NULL, got %p\n", stack[1]);

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

            GetClientRect(client, &rc);

            GetClientRect(client, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);
            if (winetest_debug > 1)
            {
                trace("MDI child: calculated max window size = (%ld x %ld)\n", rc.right-rc.left, rc.bottom-rc.top);
                dump_minmax_info( minmax );
            }

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %ld != %ld\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);

            DefMDIChildProcA(hwnd, msg, wparam, lparam);

            if (winetest_debug > 1)
            {
                trace("DefMDIChildProc returned:\n");
                dump_minmax_info( minmax );
            }

            MDI_ChildGetMinMaxInfo(client, hwnd, &my_minmax);
            ok(minmax->ptMaxSize.x == my_minmax.ptMaxSize.x, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, my_minmax.ptMaxSize.x);
            ok(minmax->ptMaxSize.y == my_minmax.ptMaxSize.y, "default height of maximized child %ld != %ld\n",
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

            if (winetest_debug > 1)
                trace("%s: x %d, y %d, cx %d, cy %d\n", (msg == WM_NCCREATE) ? "WM_NCCREATE" : "WM_CREATE",
                      cs->x, cs->y, cs->cx, cs->cy);

            ok(!(cs->dwExStyle & WS_EX_MDICHILD), "WS_EX_MDICHILD should not be set\n");
            ok(cs->lpCreateParams == mdi_lParam_test_message, "wrong cs->lpCreateParams\n");

            ok(!lstrcmpA(cs->lpszClass, "MDI_child_Class_2"), "wrong class name\n");
            ok(!lstrcmpA(cs->lpszName, "MDI child"), "wrong title\n");

            /* CREATESTRUCT should have fixed values */
            ok(cs->x != CW_USEDEFAULT, "%d == CW_USEDEFAULT\n", cs->x);
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

            style = GetWindowLongA(hwnd, GWL_STYLE);
            exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);

            GetClientRect(parent, &rc);
            if (winetest_debug > 1)
            {
                trace("WM_GETMINMAXINFO: parent %p client size = (%ld x %ld)\n", parent, rc.right, rc.bottom);
                dump_minmax_info( minmax );
            }
            GetClientRect(parent, &rc);
            if ((style & WS_CAPTION) == WS_CAPTION)
                style &= ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
            AdjustWindowRectEx(&rc, style, 0, exstyle);

            ok(minmax->ptMaxSize.x == rc.right - rc.left, "default width of maximized child %ld != %ld\n",
               minmax->ptMaxSize.x, rc.right - rc.left);
            ok(minmax->ptMaxSize.y == rc.bottom - rc.top, "default height of maximized child %ld != %ld\n",
               minmax->ptMaxSize.y, rc.bottom - rc.top);
            break;
        }

        case WM_WINDOWPOSCHANGED:
        {
            LRESULT ret;
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
            ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc1);
            ok(!ret, "got %08Ix\n", ret);
            MapWindowPoints(0, hwnd, (LPPOINT)&rc1, 2);
            ok(EqualRect(&rc1, &rc2), "rects do not match, window=%s client=%s\n",
               wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
        }
        /* fall through */
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *winpos = (WINDOWPOS *)lparam;
            WINDOWPOS my_winpos = *winpos;

            if (winetest_debug > 1)
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

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
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
            SetRect(&rc2, winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy);
            /* note: winpos coordinates are relative to parent */
            MapWindowPoints(GetParent(hwnd), 0, (LPPOINT)&rc2, 2);
            ok(EqualRect(&rc1, &rc2), "rects %s %s do not match\n",
               wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));

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

            if (winetest_debug > 1)
                trace("%s: %p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                      (msg == WM_WINDOWPOSCHANGING) ? "WM_WINDOWPOSCHANGING" : "WM_WINDOWPOSCHANGED",
                      winpos->hwnd, winpos->hwndInsertAfter,
                      winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

            DefWindowProcA(hwnd, msg, wparam, lparam);

            if (winetest_debug > 1)
                trace("%p after %p, x %d, y %d, cx %d, cy %d flags %08x\n",
                      winpos->hwnd, winpos->hwndInsertAfter,
                      winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags);

            ok(!memcmp(&my_winpos, winpos, sizeof(WINDOWPOS)),
               "DefWindowProc should not change WINDOWPOS values\n");

            return 1;
        }

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefFrameProcA(hwnd, mdi_client, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
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

    for (i = 0; i < ARRAY_SIZE(style); i++)
    {
        SCROLLINFO si;
        BOOL ret, gotit;

        winetest_push_context("style %#lx", style[i]);
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
            ok(ret, "GetScrollInfo(SB_HORZ) failed\n");
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_HORZ) should fail\n");

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "GetScrollInfo(SB_VERT) failed\n");
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_VERT) should fail\n");

        SetWindowPos(mdi_child, 0, -100, -100, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        ret = GetScrollInfo(mdi_client, SB_HORZ, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "GetScrollInfo(SB_HORZ) failed\n");
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_HORZ) should fail\n");

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "GetScrollInfo(SB_VERT) failed\n");
            ok(si.nPage == 0, "expected 0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin == 0, "expected 0\n");
            ok(si.nMax == 100, "expected 100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_VERT) should fail\n");

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
            ok(ret, "GetScrollInfo(SB_HORZ) failed\n");
            todo_wine
            ok(si.nPage != 0, "expected !0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin != 0, "expected !0\n");
            ok(si.nMax != 100, "expected !100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_HORZ) should fail\n");

        ret = GetScrollInfo(mdi_client, SB_VERT, &si);
        if (style[i] & (WS_HSCROLL | WS_VSCROLL))
        {
            ok(ret, "GetScrollInfo(SB_VERT) failed\n");
            todo_wine
            ok(si.nPage != 0, "expected !0\n");
            ok(si.nPos == 0, "expected 0\n");
            ok(si.nTrackPos == 0, "expected 0\n");
            ok(si.nMin != 0, "expected !0\n");
            ok(si.nMax != 100, "expected !100\n");
        }
        else
            ok(!ret, "GetScrollInfo(SB_VERT) should fail\n");

        DestroyMenu(child_menu);
        DestroyWindow(mdi_child);
        DestroyWindow(mdi_client);
        winetest_pop_context();
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

#define check_icon_size(a, b, c) check_icon_size_(__LINE__, a, b, c)
static void check_icon_size_( int line, HICON icon, LONG width, LONG height )
{
    ICONINFO info = {sizeof(info)};
    BITMAP bitmap;
    BOOL ret;

    ret = GetIconInfo( icon, &info );
    ok_(__FILE__, line)(ret, "failed to get icon info, error %lu\n", GetLastError());
    ret = GetObjectW(info.hbmColor, sizeof(bitmap), &bitmap);
    ok_(__FILE__, line)(ret, "failed to get bitmap, error %lu\n", GetLastError());
    ok_(__FILE__, line)(bitmap.bmWidth == width, "expected width %ld, got %d\n", width, bitmap.bmWidth);
    ok_(__FILE__, line)(bitmap.bmHeight == height, "expected height %ld, got %d\n", height, bitmap.bmHeight);
}

#define check_internal_icon_size(a, b, c, d) check_internal_icon_size_(__LINE__, a, b, c, d)
static void check_internal_icon_size_( int line, HANDLE window, UINT type, LONG width, LONG height )
{
    HICON icon;
    BOOL ret;

    icon = pInternalGetWindowIcon( window, type );
    ok_(__FILE__, line)(icon != 0, "expected nonzero icon\n");
    check_icon_size_( line, icon, width, height );
    ret = DestroyIcon( icon );
    ok_(__FILE__, line)(ret, "failed to destroy icon, error %lu\n", GetLastError());
}

static DWORD WINAPI internal_get_icon_thread(void *arg)
{
    HICON icon;
    BOOL ret;

    icon = pInternalGetWindowIcon( arg, ICON_BIG );
    ok( icon != 0, "expected nonzero icon\n" );
    ret = DestroyIcon( icon );
    ok( ret, "got error %lu\n", GetLastError() );

    return 0;
}

static void test_icons(void)
{
    static const BYTE bitmap_bits[50 * 50 * 4];
    HICON icon = CreateIcon( 0, 10, 10, 1, 32, bitmap_bits, bitmap_bits );
    HICON icon2 = CreateIcon( 0, 20, 20, 1, 32, bitmap_bits, bitmap_bits );
    HICON icon3 = CreateIcon( 0, 30, 30, 1, 32, bitmap_bits, bitmap_bits );
    HICON icon4 = CreateIcon( 0, 40, 40, 1, 32, bitmap_bits, bitmap_bits );
    HICON icon5 = CreateIcon( 0, 50, 50, 1, 32, bitmap_bits, bitmap_bits );
    LONG big_width = GetSystemMetrics( SM_CXICON ), big_height = GetSystemMetrics( SM_CYICON );
    LONG small_width = GetSystemMetrics( SM_CXSMICON ), small_height = GetSystemMetrics( SM_CYSMICON );
    WNDCLASSEXA cls = {sizeof(cls)};
    HICON res, res2;
    HANDLE thread;
    HWND hwnd;
    BOOL ret;

    cls.lpfnWndProc = DefWindowProcA;
    cls.hIcon = icon4;
    cls.hIconSm = icon5;
    cls.lpszClassName = "IconWindowClass";
    RegisterClassExA(&cls);

    hwnd = CreateWindowExA(0, "IconWindowClass", "icon test", 0,
                           100, 100, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL);
    ok( hwnd != NULL, "failed to create window\n" );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == 0, "wrong big icon %p/0\n", res );
    SetLastError( 0xdeadbeef );
    res = pInternalGetWindowIcon( hwnd, ICON_BIG );
    ok( res != 0, "expected nonzero icon\n" );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    check_icon_size( res, 40, 40 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_BIG );
    ok( res2 && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, 40, 40 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );
    ret = DestroyIcon( res );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( !res, "wrong small icon %p\n", res );
    res = pInternalGetWindowIcon( hwnd, ICON_SMALL );
    ok( res != 0, "expected nonzero icon\n" );
    check_icon_size( res, 50, 50 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL );
    ok( res2 && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, 50, 50 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );
    ret = DestroyIcon( res );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( !res, "wrong small2 icon %p\n", res );
    res = pInternalGetWindowIcon( hwnd, ICON_SMALL2 );
    ok( res != 0, "expected nonzero icon\n" );
    check_icon_size( res, 50, 50 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL2 );
    ok( res2 && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, 50, 50 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );
    ret = DestroyIcon( res );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon );
    ok( res == 0, "wrong previous big icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon, "wrong big icon after set %p/%p\n", res, icon );
    res2 = pInternalGetWindowIcon( hwnd, ICON_BIG );
    ok( res2 && res2 != icon, "got %p\n", res2 );
    check_icon_size( res2, 10, 10 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon2 );
    ok( res == icon, "wrong previous big icon %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon2, "wrong big icon after set %p/%p\n", res, icon2 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_BIG );
    ok( res2 && res2 != icon2, "got %p\n", res2 );
    check_icon_size( res2, 20, 20 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == 0, "wrong small icon %p/0\n", res );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL );
    ok( res2 != 0, "expected nonzero icon\n" );
    check_icon_size( res2, small_width, small_height );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res && res != icon3 && res != icon2, "wrong small2 icon %p\n", res );
    res2 = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res2 == res, "expected icon to match\n" );
    check_icon_size( res, small_width, small_height );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL2 );
    ok( res2 && res2 != icon3 && res2 != icon2 && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, small_width, small_height );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon );
    ok( res == 0, "wrong previous small icon %p/0\n", res );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == icon, "wrong small icon after set %p/%p\n", res, icon );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL );
    ok( res2 && res2 != icon, "got %p\n", res2 );
    check_icon_size( res2, 10, 10 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == icon, "wrong small2 icon after set %p/%p\n", res, icon );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL2 );
    ok( res2 && res2 != icon && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, 10, 10 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon3 );
    ok( res == icon, "wrong previous small icon %p/%p\n", res, icon );
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL, 0 );
    ok( res == icon3, "wrong small icon after set %p/%p\n", res, icon3 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL );
    ok( res2 && res2 != icon3, "got %p\n", res2 );
    check_icon_size( res2, 30, 30 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_SMALL2, 0 );
    ok( res == icon3, "wrong small2 icon after set %p/%p\n", res, icon3 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_SMALL2 );
    ok( res2 && res2 != icon3 && res2 != res, "got %p\n", res2 );
    check_icon_size( res2, 30, 30 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    /* make sure the big icon hasn't changed */
    res = (HICON)SendMessageA( hwnd, WM_GETICON, ICON_BIG, 0 );
    ok( res == icon2, "wrong big icon after set %p/%p\n", res, icon2 );
    res2 = pInternalGetWindowIcon( hwnd, ICON_BIG );
    ok( res2 && res2 != icon2, "got %p\n", res2 );
    check_icon_size( res2, 20, 20 );
    ret = DestroyIcon( res2 );
    ok( ret, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    res = pInternalGetWindowIcon( NULL, ICON_BIG );
    ok( !res, "got %p\n", res );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    res = pInternalGetWindowIcon( hwnd, 0xdeadbeef );
    ok( !res, "got %p\n", res );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    thread = CreateThread( NULL, 0, internal_get_icon_thread, hwnd, 0, NULL );
    ret = WaitForSingleObject( thread, 1000 );
    ok( !ret, "wait timed out\n" );
    CloseHandle( thread );

    DestroyWindow( hwnd );
    UnregisterClassA( "IconWindowClass", GetModuleHandleA( NULL ) );

    /* check when the big class icon is missing */

    cls.hIcon = 0;
    cls.hIconSm = icon5;
    RegisterClassExA(&cls);

    hwnd = CreateWindowExA(0, "IconWindowClass", "icon test", 0,
                           100, 100, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL);
    ok( hwnd != NULL, "failed to create window\n" );

    check_internal_icon_size( hwnd, ICON_BIG, big_width, big_height );
    check_internal_icon_size( hwnd, ICON_SMALL, 50, 50 );
    check_internal_icon_size( hwnd, ICON_SMALL2, 50, 50 );

    DestroyWindow( hwnd );
    UnregisterClassA( "IconWindowClass", GetModuleHandleA( NULL ) );

    /* and when the small class icon is missing */

    cls.hIcon = icon4;
    cls.hIconSm = 0;
    RegisterClassExA(&cls);
    hwnd = CreateWindowExA(0, "IconWindowClass", "icon test", 0,
                           100, 100, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL);
    ok( hwnd != NULL, "failed to create window\n" );

    check_internal_icon_size( hwnd, ICON_BIG, 40, 40 );
    check_internal_icon_size( hwnd, ICON_SMALL, small_width, small_width );
    check_internal_icon_size( hwnd, ICON_SMALL2, small_width, small_width );

    DestroyWindow( hwnd );
    UnregisterClassA( "IconWindowClass", GetModuleHandleA( NULL ) );

    /* and when both are missing */

    cls.hIcon = 0;
    cls.hIconSm = 0;
    RegisterClassExA(&cls);
    hwnd = CreateWindowExA(0, "IconWindowClass", "icon test", 0,
                           100, 100, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL);
    ok( hwnd != NULL, "failed to create window\n" );

    check_internal_icon_size( hwnd, ICON_BIG, big_width, big_height );
    check_internal_icon_size( hwnd, ICON_SMALL, big_width, big_height );
    check_internal_icon_size( hwnd, ICON_SMALL2, big_width, big_height );

    DestroyWindow( hwnd );
    UnregisterClassA( "IconWindowClass", GetModuleHandleA( NULL ) );

    DestroyIcon( icon );
    DestroyIcon( icon2 );
    DestroyIcon( icon3 );
    DestroyIcon( icon4 );
    DestroyIcon( icon5 );
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
    RECT rc_expected;
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

    flush_events( TRUE ); /* needed by Win10 1709+ */

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

    ret = SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOMOVE);
    ok(ret, "Got %d\n", ret);
    ret = SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOMOVE);
    ok(ret, "Got %d\n", ret);

    ret = SetWindowPos(hwnd, 0, -32769, -40000, -32769, -90000, SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);
    ret = SetWindowPos(hwnd, 0, 32768, 40000, 32768, 40000, SWP_NOSIZE);
    ok(ret, "Got %d\n", ret);

    ret = SetWindowPos(hwnd, 0, orig_win_rc.left, orig_win_rc.top,
                       orig_win_rc.right, orig_win_rc.bottom, 0);
    ok(ret, "Got %d\n", ret);

    hwnd_desktop = GetDesktopWindow();
    ok(!!hwnd_desktop, "Failed to get hwnd_desktop window (%ld).\n", GetLastError());
    hwnd_child = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd);
    ok(!!hwnd_child, "Failed to create child window (%ld)\n", GetLastError());
    hwnd_grandchild = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd_child);
    ok(!!hwnd_child, "Failed to create child window (%ld)\n", GetLastError());
    hwnd_child2 = create_tool_window(WS_VISIBLE|WS_CHILD, hwnd);
    ok(!!hwnd_child2, "Failed to create second child window (%ld)\n", GetLastError());

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
    ret = SetWindowPos(hwnd_child, hwnd2, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc1, &rc2), "%s != %s.\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, HWND_NOTOPMOST, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    todo_wine ok(EqualRect(&rc1, &rc2), "%s != %s.\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);
    SetWindowPos(hwnd_child, HWND_NOTOPMOST, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, 0);

    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, HWND_TOPMOST, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    todo_wine ok(EqualRect(&rc1, &rc2), "%s != %s.\n", wine_dbgstr_rect(&rc1), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);
    SetWindowPos(hwnd_child, HWND_TOPMOST, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, 0);

    /* HWND_TOP / HWND_BOTTOM are different. */
    GetWindowRect(hwnd_child, &rc1);
    rc_expected.left = rc1.left + 1;
    rc_expected.top = rc1.top + 2;
    rc_expected.right = rc1.left + 4;
    rc_expected.bottom = rc1.top + 6;
    ret = SetWindowPos(hwnd_child, HWND_TOP, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc_expected, &rc2), "%s != %s.\n",
            wine_dbgstr_rect(&rc_expected), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);
    SetWindowPos(hwnd_child, HWND_TOP, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, 0);

    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, HWND_BOTTOM, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc_expected, &rc2), "%s != %s.\n",
            wine_dbgstr_rect(&rc_expected), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);
    SetWindowPos(hwnd_child, HWND_BOTTOM, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, 0);

    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, NULL, 1, 2, 3, 4, 0);
    ok(ret, "Got %d.\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc_expected, &rc2), "%s != %s.\n",
            wine_dbgstr_rect(&rc_expected), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);
    SetWindowPos(hwnd_child, NULL, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, 0);

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
    ok(EqualRect(&rc_expected, &rc2),
            "%s != %s.\n", wine_dbgstr_rect(&rc_expected), wine_dbgstr_rect(&rc2));
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* Given a sibling window, the window is properly resized. */
    GetWindowRect(hwnd_child, &rc1);
    ret = SetWindowPos(hwnd_child, hwnd_child2, 1, 2, 3, 4, 0);
    ok(ret, "Got %d\n", ret);
    GetWindowRect(hwnd_child, &rc2);
    ok(EqualRect(&rc_expected, &rc2),
            "%s != %s.\n", wine_dbgstr_rect(&rc_expected), wine_dbgstr_rect(&rc2));
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
    ok(!!hwnd_child, "Failed to create child window (%ld)\n", GetLastError());
    ret = SetWindowPos(hwnd_child, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    ok(ret, "Got %d\n", ret);
    flush_events( TRUE );
    flaky todo_wine check_active_state(hwnd2, hwnd2, hwnd2);
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
    ok( retok, "DestroyMenu error %ld\n", GetLastError());
    retok = IsMenu(hMenu);
    ok(!retok, "menu handle should be not valid after DestroyMenu\n");
    ret = GetMenu(parent);
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
        ok(child[i] != 0, "CreateWindowEx failed to create child window\n");
    }

    hwnd = GetWindow(parent, GW_CHILD);
    ok(hwnd != 0, "GetWindow(GW_CHILD) failed\n");
    ok(hwnd == GetWindow(child[total - 1], GW_HWNDFIRST), "GW_HWNDFIRST is wrong\n");
    ok(child[order[total - 1]] == GetWindow(child[0], GW_HWNDLAST), "GW_HWNDLAST is wrong\n");

    for (i = 0; i < total; i++)
    {
        ok(child[order[i]] == hwnd, "Z order of child #%Id is wrong\n", i);
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

    /* Give current thread foreground state otherwise the tests may fail. */
    if (!SetForegroundWindow(hwnd_D))
    {
        skip("SetForegroundWindow not working\n");
        return;
    }

    SetWindowPos(hwnd_E, hwnd_D, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);
    check_z_order(hwnd_E, 0, hwnd_D, 0, FALSE);

    hwnd_F = CreateWindowExA(0, "MainWindowClass", "Owner window",
                            WS_OVERLAPPED | WS_CAPTION,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandleA(NULL), NULL);
    check_z_order(hwnd_F, hwnd_D, 0, 0, FALSE);

    SetWindowPos(hwnd_F, hwnd_E, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, 0, 0, FALSE);

    hwnd_C = CreateWindowExA(0, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandleA(NULL), NULL);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, 0, hwnd_F, FALSE);

    hwnd_B = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            hwnd_F, 0, GetModuleHandleA(NULL), NULL);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, 0, hwnd_F, TRUE);

    hwnd_A = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", NULL,
                            style,
                            100, 100, 100, 100,
                            0, 0, GetModuleHandleA(NULL), NULL);
    check_z_order(hwnd_F, 0, hwnd_E, 0, FALSE);
    check_z_order(hwnd_E, hwnd_F, hwnd_D, 0, FALSE);
    check_z_order(hwnd_D, hwnd_E, hwnd_C, 0, FALSE);
    check_z_order(hwnd_C, hwnd_D, hwnd_B, hwnd_F, FALSE);
    check_z_order(hwnd_B, hwnd_C, hwnd_A, hwnd_F, TRUE);
    check_z_order(hwnd_A, hwnd_B, 0, 0, TRUE);

    if (winetest_debug > 1)
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
    ok( win_rect.left <= rgn_rect.left &&
        win_rect.top <= rgn_rect.top &&
        win_rect.right >= rgn_rect.right &&
        win_rect.bottom >= rgn_rect.bottom,
        "rgn %s not inside win %s\n", wine_dbgstr_rect(&rgn_rect), wine_dbgstr_rect(&win_rect));
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
    SetLastError(0xdeadbeef);
    EnableWindow(hwnd, FALSE);
    ok(GetLastError() == 0xdeadbeef, "got error %lu in EnableWindow call\n", GetLastError());
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
    ok( GetFocus() != hwnd, "Focus should not be on parent %p\n", hwnd );
    old_wnd_proc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)set_focus_on_activate_proc);
    ShowWindow(hwnd, SW_RESTORE);
    ok( GetActiveWindow() == hwnd, "parent window %p should be active\n", hwnd);
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

static void test_SetActiveWindow_0_proc( char **argv )
{
    HANDLE start_event, stop_event;
    HWND hwnd, other, tmp;
    BOOL ret;

    sscanf( argv[3], "%p", &other );
    start_event = CreateEventW( NULL, FALSE, FALSE, L"test_SetActiveWindow_0_start" );
    ok( start_event != 0, "CreateEventW failed, error %lu\n", GetLastError() );
    stop_event = CreateEventW( NULL, FALSE, FALSE, L"test_SetActiveWindow_0_stop" );
    ok( stop_event != 0, "CreateEventW failed, error %lu\n", GetLastError() );

    hwnd = CreateWindowExA( 0, "static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, 0, NULL, NULL );
    ok( !!hwnd, "CreateWindowExA failed, error %lu\n", GetLastError() );
    flush_events( TRUE );

    ret = SetForegroundWindow( hwnd );
    ok( ret, "SetForegroundWindow failed, error %lu\n", GetLastError() );

    tmp = GetForegroundWindow();
    ok( tmp == hwnd, "GetForegroundWindow returned %p\n", tmp );
    tmp = GetActiveWindow();
    ok( tmp == hwnd, "GetActiveWindow returned %p\n", tmp );
    tmp = GetFocus();
    ok( tmp == hwnd, "GetFocus returned %p\n", tmp );

    SetLastError( 0xdeadbeef );
    tmp = SetActiveWindow( 0 );
    if (!tmp) ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    else /* < Win10 */
    {
        ok( tmp == hwnd, "SetActiveWindow returned %p\n", tmp );
        ok( GetLastError() == 0, "got error %lu\n", GetLastError() );

        tmp = GetForegroundWindow();
        ok( tmp == other || tmp == 0, "GetForegroundWindow returned %p\n", tmp );
        tmp = GetActiveWindow();
        ok( tmp == 0, "GetActiveWindow returned %p\n", tmp );
        tmp = GetFocus();
        ok( tmp == 0, "GetFocus returned %p\n", tmp );

        SetEvent( start_event );
        wait_for_events( 1, &stop_event, INFINITE );

        tmp = GetForegroundWindow();
        todo_wine
        ok( tmp == other, "GetForegroundWindow returned %p\n", tmp );
        tmp = GetActiveWindow();
        ok( tmp == 0, "GetActiveWindow returned %p\n", tmp );
        tmp = GetFocus();
        ok( tmp == 0, "GetFocus returned %p\n", tmp );
    }

    tmp = SetActiveWindow( 0 );
    ok( tmp == 0, "SetActiveWindow returned %p\n", tmp );
    tmp = GetForegroundWindow();
    todo_wine
    ok( tmp == hwnd, "GetForegroundWindow returned %p\n", tmp );
    tmp = GetActiveWindow();
    todo_wine
    ok( tmp == hwnd, "GetActiveWindow returned %p\n", tmp );
    tmp = GetFocus();
    todo_wine
    ok( tmp == hwnd, "GetFocus returned %p\n", tmp );

    CloseHandle( start_event );
    CloseHandle( stop_event );
    DestroyWindow( hwnd );
}

static void test_SetActiveWindow_0( char **argv )
{
    STARTUPINFOA startup = {.cb = sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION info;
    char cmdline[MAX_PATH];
    HANDLE events[3];
    HWND hwnd, tmp;
    BOOL ret;

    hwnd = CreateWindowExA( 0, "static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, 0, NULL, NULL );
    ok( !!hwnd, "CreateWindowExA failed, error %lu\n", GetLastError() );
    SetWindowPos( hwnd, HWND_TOPMOST, 150, 150, 300, 300, SWP_FRAMECHANGED | SWP_SHOWWINDOW );
    flush_events( TRUE );

    tmp = GetForegroundWindow();
    flaky_wine
    ok( tmp == hwnd, "GetForegroundWindow returned %p\n", tmp );
    tmp = GetActiveWindow();
    flaky_wine
    ok( tmp == hwnd, "GetActiveWindow returned %p\n", tmp );
    tmp = GetFocus();
    flaky_wine
    ok( tmp == hwnd, "GetFocus returned %p\n", tmp );

    events[1] = CreateEventW( NULL, FALSE, FALSE, L"test_SetActiveWindow_0_start" );
    ok( events[1] != 0, "CreateEventW failed, error %lu\n", GetLastError() );
    events[2] = CreateEventW( NULL, FALSE, FALSE, L"test_SetActiveWindow_0_stop" );
    ok( events[2] != 0, "CreateEventW failed, error %lu\n", GetLastError() );

    sprintf( cmdline, "%s %s SetActiveWindow_0 %p", argv[0], argv[1], hwnd );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok( ret, "CreateProcessA failed, error %lu\n", GetLastError() );

    events[0] = info.hProcess;
    if (wait_for_events( 2, events, INFINITE ) == 1)
    {
        tmp = GetForegroundWindow();
        todo_wine
        ok( tmp == hwnd, "GetForegroundWindow returned %p\n", tmp );
        tmp = GetActiveWindow();
        todo_wine
        ok( tmp == hwnd, "GetActiveWindow returned %p\n", tmp );
        tmp = GetFocus();
        todo_wine
        ok( tmp == hwnd, "GetFocus returned %p\n", tmp );
        SetEvent( events[2] );
    }

    wait_child_process( info.hProcess );
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( events[1] );
    CloseHandle( events[2] );

    DestroyWindow( hwnd );
}

static void test_SetActiveWindow(HWND hwnd)
{
    HWND hwnd2, ret;

    flush_events( TRUE );
    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    /* Invisible window. */
    SetActiveWindow(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    
    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    SetActiveWindow(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, NULL);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    SetWindowPos(hwnd2,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    DestroyWindow(hwnd2);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    /* try to activate the desktop */
    SetLastError(0xdeadbeef);
    ret = SetActiveWindow(GetDesktopWindow());
    ok(ret == NULL, "expected NULL, got %p\n", ret);
    todo_wine
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", GetLastError());
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    /* activating a child should activate the parent */
    hwnd2 = CreateWindowExA(0, "MainWindowClass", "Child window", WS_CHILD, 0, 0, 0, 0, hwnd, 0, GetModuleHandleA(NULL), NULL);
    check_wnd_state(hwnd, hwnd, hwnd, 0);
    ret = SetActiveWindow(hwnd2);
    ok(ret == hwnd, "expected %p, got %p\n", hwnd, ret);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    DestroyWindow(hwnd2);
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
    ok(ret, "SetEvent failed, last error %#lx.\n", GetLastError());

    res = WaitForSingleObject(p->test_finished, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());

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
    check_wnd_state(0, 0, 0, 0);

    ShowWindow(hwnd, SW_SHOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

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
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
       "got error %ld expected ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_HIDEWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    check_wnd_state(hwnd, hwnd, hwnd, 0);

    hwnd2 = GetForegroundWindow();
    ok(hwnd2 == hwnd, "Wrong foreground window %p\n", hwnd2);
    ret = SetForegroundWindow( GetDesktopWindow() );
    ok(ret, "SetForegroundWindow(desktop) error: %ld\n", GetLastError());
    hwnd2 = GetForegroundWindow();
    ok(hwnd2 != hwnd, "Wrong foreground window %p\n", hwnd2);

    ShowWindow(hwnd, SW_HIDE);
    check_wnd_state(0, 0, 0, 0);

    /* Invisible window. */
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

    hwnd2 = CreateWindowA("static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, 0, 0, 0, 0);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    thread_params.window_created = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#lx.\n", GetLastError());
    thread_params.test_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#lx.\n", GetLastError());
    thread = CreateThread(NULL, 0, create_window_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#lx.\n", GetLastError());
    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());
    check_wnd_state(hwnd2, thread_params.window, hwnd2, 0);

    SetForegroundWindow(hwnd2);
    check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    if (0) check_wnd_state(hwnd2, hwnd2, hwnd2, 0);

    ok(GetActiveWindow() == hwnd2, "Expected active window %p, got %p.\n", hwnd2, GetActiveWindow());
    ok(GetFocus() == hwnd2, "Expected focus window %p, got %p.\n", hwnd2, GetFocus());

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

    flush_events(TRUE);
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

	check_wnd_state(button, button, button, button);

	ShowWindow(hwnd, SW_SHOWNOACTIVATE);

	check_wnd_state(button, button, button, button);

	DestroyWindow(hwnd);

	hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
	assert(hwnd);

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

    check_wnd_state(button, button, button, button);

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    check_wnd_state(button, button, button, button);

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
    assert(hwnd);

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
                ok(gti.flags & GUI_INMENUMODE, "Thread info incorrect (flags=%08lX)!\n", gti.flags);
            }
            cap_wnd = GetCapture();

            ok(cap_wnd == (HWND)lParam, "capture window %p does not match lparam %Ix\n", cap_wnd, lParam);
            todo_wine ok(cap_wnd == hWnd, "capture window %p does not match hwnd %p\n", cap_wnd, hWnd);

            /* check that re-setting the capture for the menu fails */
            set_cap_wnd = SetCapture(cap_wnd);
            ok(!set_cap_wnd, "SetCapture should have failed!\n");
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
                ok(gti.flags & GUI_INMENUMODE, "Thread info incorrect (flags=%08lX)!\n", gti.flags);
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
    ok( aclass, "RegisterClassA failed with error %ld\n", GetLastError());
    hwnd = CreateWindowA( wclass.lpszClassName, "MenuTest",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
                          400, 200, NULL, NULL, hInstance, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if (!hwnd) return;
    hmenu = CreatePopupMenu();

    ret = AppendMenuA( hmenu, MF_STRING, 1, "winetest2");
    ok( ret, "AppendMenuA has failed!\n");

    /* set main window to have initial capture */
    SetCapture(hwnd);

    /* create popup (it will self-destruct) */
    ret = TrackPopupMenu(hmenu, TPM_RETURNCMD, 100, 100, 0, hwnd, NULL);
    ok( ret == 0, "TrackPopupMenu returned %d expected zero\n", ret);

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
    } while (ret && ignore_message(msg->message, msg->hwnd));
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
    HWND popup, child = NULL;
    MSG msg;
    BOOL ret;

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );

    GetWindowRect(hwnd, &rc);

    popup = CreateWindowExA(0, "MainWindowClass", NULL, WS_POPUP,
                            rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                            hwnd, 0, 0, NULL);
    assert(popup != 0);
    ShowWindow(popup, SW_SHOW);
    UpdateWindow(popup);
    SetWindowPos( popup, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );

    GetWindowRect(popup, &rc);

    x = rc.left + (rc.right - rc.left) / 2;
    y = rc.top + (rc.bottom - rc.top) / 2;

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
        ok(msg.pt.x == x && msg.pt.y == y, "wrong message coords (%d,%d)/(%ld,%ld)\n",
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
        if (ignore_message(msg.message, msg.hwnd)) continue;
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

    ShowWindow(popup, SW_HIDE);

    /* Test sending double click to the non-client area, while capturing the window after
       the first click has been processed.  Use a child window to ensure that Wine's graphics
       driver isn't managing the non-client area. */

    GetWindowRect(hwnd, &rc);
    child = CreateWindowExA(0, "MainWindowClass", NULL, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                            rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                            hwnd, 0, 0, NULL);
    GetWindowRect(child, &rc);

    UpdateWindow(child);
    SetCursorPos( rc.left + 5, rc.top + 5 );
    flush_events( TRUE );

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    if (msg.message == WM_NCMOUSEMOVE) /* not sent by Win10 1709+ */
    {
        ok(msg.hwnd == child, "expected %p, got %p\n", child, msg.hwnd);
        ret = wait_for_message( &msg );
        ok(ret, "no message available\n");
    }
    ok(msg.hwnd == child && msg.message == WM_NCLBUTTONDOWN, "hwnd %p/%p message %04x\n",
       msg.hwnd, child, msg.message);
    ok(msg.wParam == HTSYSMENU, "wparam %Id\n", msg.wParam);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == child && msg.message == WM_NCLBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, child, msg.message);

    SetCapture( child );

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == child && msg.message == WM_LBUTTONDBLCLK, "hwnd %p/%p message %04x\n",
       msg.hwnd, child, msg.message);
    ok(msg.wParam == MK_LBUTTON, "wparam %Id\n", msg.wParam);

    ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    todo_wine
    ok(msg.hwnd == child && (msg.message == WM_NCMOUSELEAVE || broken(msg.message == WM_LBUTTONUP)),
       "hwnd %p/%p message %04x\n", msg.hwnd, child, msg.message);

    if (msg.message == WM_NCMOUSELEAVE)
        ret = wait_for_message( &msg );
    ok(ret, "no message available\n");
    ok(msg.hwnd == child && msg.message == WM_LBUTTONUP, "hwnd %p/%p message %04x\n",
       msg.hwnd, child, msg.message);

    ret = peek_message(&msg);
    ok(!ret, "message %04x available\n", msg.message);

done:
    flush_events( TRUE );

    if (child) DestroyWindow(child);
    DestroyWindow(popup);

    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
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
    LRESULT ret;
    RECT rc;
    MoveWindow( hwnd, 0, 0, x, y, 0);
    GetWindowRect( hwnd, prc);
    rc = *prc;
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)prc);
    ok(!ret, "got %08Ix\n", ret);
    if (winetest_debug > 1)
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

typedef struct
{
    DWORD cs_style;
    DWORD cs_exstyle;
    DWORD style;
    DWORD exstyle;
} test_style;

static LRESULT WINAPI cbt_proc(int ncode, WPARAM wparam, LPARAM lparam)
{
    CBT_CREATEWNDW* c = (CBT_CREATEWNDW*)lparam;
    HWND hwnd = (HWND)wparam;
    test_style *ts;
    DWORD style;

    if (ncode !=  HCBT_CREATEWND)
        return CallNextHookEx(NULL, ncode, wparam, lparam);

    ts = c->lpcs->lpCreateParams;
    ok(ts != NULL, "lpCreateParams not set\n");
    ok(c->lpcs->style == ts->cs_style, "style = 0x%08lx, expected 0x%08lx\n",
            c->lpcs->style, ts->cs_style);
    ok(c->lpcs->dwExStyle == ts->cs_exstyle, "exstyle = 0x%08lx, expected 0x%08lx\n",
            c->lpcs->dwExStyle, ts->cs_exstyle);

    style = GetWindowLongW(hwnd, GWL_STYLE);
    ok(style == ts->cs_style, "style = 0x%08lx, expected 0x%08lx\n",
            style, ts->cs_style);
    style = GetWindowLongW(hwnd, GWL_EXSTYLE);
    ok(style == (ts->cs_exstyle & ~WS_EX_LAYERED),
            "exstyle = 0x%08lx, expected 0x%08lx\n", style, ts->cs_exstyle);
    return CallNextHookEx(NULL, ncode, wparam, lparam);
}

static LRESULT WINAPI StyleCheckProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    CREATESTRUCTA *cs;
    test_style *ts;
    DWORD style;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        cs = (LPCREATESTRUCTA)lparam;
        ts = cs->lpCreateParams;

        ok(ts != NULL, "lpCreateParams not set\n");
        ok(cs->style == ts->cs_style, "style = 0x%08lx, expected 0x%08lx\n",
                cs->style, ts->cs_style);
        ok(cs->dwExStyle == ts->cs_exstyle, "exstyle = 0x%08lx, expected 0x%08lx\n",
                cs->dwExStyle, ts->cs_exstyle);

        style = GetWindowLongW(hwnd, GWL_STYLE);
        ok(style == ts->style, "style = 0x%08lx, expected 0x%08lx\n",
                style, ts->style);
        style = GetWindowLongW(hwnd, GWL_EXSTYLE);
        ok(style == ts->exstyle, "exstyle = 0x%08lx, expected 0x%08lx\n",
                style, ts->exstyle);
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
    test_style ts;
    HWND hwnd;
    HWND hwndParent = NULL;
    HHOOK hook;

    ts.cs_style = dwStyleIn;
    ts.cs_exstyle = dwExStyleIn;
    if ((dwExStyleIn & WS_EX_DLGMODALFRAME) ||
            ((!(dwExStyleIn & WS_EX_STATICEDGE)) &&
             (dwStyleIn & (WS_DLGFRAME | WS_THICKFRAME))))
        ts.cs_exstyle |= WS_EX_WINDOWEDGE;
    else
        ts.cs_exstyle &= ~WS_EX_WINDOWEDGE;
    ts.style = dwStyleOut;
    ts.exstyle = dwExStyleOut;

    if (dwStyleIn & WS_CHILD)
    {
        hwndParent = CreateWindowExA(0, "static", NULL,
            WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
        ok(hwndParent != NULL, "CreateWindowExA failed\n");
    }

    hook = SetWindowsHookExW(WH_CBT, cbt_proc, 0, GetCurrentThreadId());
    hwnd = CreateWindowExA(dwExStyleIn, (LPCSTR)MAKEINTATOM(atomStyleCheckClass), NULL,
                    dwStyleIn, 0, 0, 0, 0, hwndParent, NULL, NULL, &ts);
    ok(hwnd != NULL, "CreateWindowExA failed\n");
    UnhookWindowsHookEx(hook);

    flush_events( TRUE );

    dwActualStyle = GetWindowLongA(hwnd, GWL_STYLE);
    dwActualExStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(dwActualStyle == dwStyleOut, "expected style %#lx, got %#lx\n", dwStyleOut, dwActualStyle);
    ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#lx, got %#lx\n", dwExStyleOut, dwActualExStyle);

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
    ok(dwActualStyle == dwStyleOut, "expected style %#lx, got %#lx\n", dwStyleOut, dwActualStyle);
    ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#lx, got %#lx\n", dwExStyleOut, dwActualExStyle);

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
    ok(dwActualStyle == dwStyleOut, "expected style %#lx, got %#lx\n", dwStyleOut, dwActualStyle);
    ok(dwActualExStyle == dwExStyleOut, "expected ex_style %#lx, got %#lx\n", dwExStyleOut, dwActualExStyle);

    DestroyWindow(hwnd);
    if (hwndParent) DestroyWindow(hwndParent);
}

/* tests what window styles the window manager automatically adds */
static void test_window_styles(void)
{
    static const struct
    {
        DWORD style_in;
        DWORD exstyle_in;
        DWORD style_out;
        DWORD exstyle_out;
    } tests[] = {
        {0, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_DLGFRAME, 0, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_THICKFRAME, 0, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_DLGFRAME, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE},
        {WS_THICKFRAME, WS_EX_STATICEDGE, WS_THICKFRAME|WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_STATICEDGE},
        {WS_OVERLAPPEDWINDOW, 0, WS_CLIPSIBLINGS|WS_OVERLAPPEDWINDOW, WS_EX_WINDOWEDGE},
        {WS_CHILD, 0, WS_CHILD, 0},
        {WS_CHILD|WS_DLGFRAME, 0, WS_CHILD|WS_DLGFRAME, WS_EX_WINDOWEDGE},
        {WS_CHILD|WS_THICKFRAME, 0, WS_CHILD|WS_THICKFRAME, WS_EX_WINDOWEDGE},
        {WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_DLGFRAME, WS_EX_STATICEDGE},
        {WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_THICKFRAME, WS_EX_STATICEDGE},
        {WS_CHILD|WS_CAPTION, 0, WS_CHILD|WS_CAPTION, WS_EX_WINDOWEDGE},
        {WS_CHILD|WS_CAPTION|WS_SYSMENU, 0, WS_CHILD|WS_CAPTION|WS_SYSMENU, WS_EX_WINDOWEDGE},
        {WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0},
        {WS_CHILD, WS_EX_DLGMODALFRAME, WS_CHILD, WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME},
        {WS_CHILD, WS_EX_DLGMODALFRAME|WS_EX_STATICEDGE, WS_CHILD, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME},
        {WS_CHILD|WS_POPUP, 0, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0},
        {WS_CHILD|WS_POPUP|WS_DLGFRAME, 0, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE},
        {WS_CHILD|WS_POPUP|WS_THICKFRAME, 0, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE},
        {WS_CHILD|WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE},
        {WS_CHILD|WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_CHILD|WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE},
        {WS_CHILD|WS_POPUP, WS_EX_APPWINDOW, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, WS_EX_APPWINDOW},
        {WS_CHILD|WS_POPUP, WS_EX_WINDOWEDGE, WS_CHILD|WS_POPUP|WS_CLIPSIBLINGS, 0},
        {WS_CHILD, WS_EX_WINDOWEDGE, WS_CHILD, 0},
        {0, WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW},
        {WS_POPUP, 0, WS_POPUP|WS_CLIPSIBLINGS, 0},
        {WS_POPUP, WS_EX_WINDOWEDGE, WS_POPUP|WS_CLIPSIBLINGS, 0},
        {WS_POPUP|WS_DLGFRAME, 0, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE},
        {WS_POPUP|WS_THICKFRAME, 0, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE},
        {WS_POPUP|WS_DLGFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_DLGFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE},
        {WS_POPUP|WS_THICKFRAME, WS_EX_STATICEDGE, WS_POPUP|WS_THICKFRAME|WS_CLIPSIBLINGS, WS_EX_STATICEDGE},
        {WS_CAPTION, WS_EX_STATICEDGE, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_STATICEDGE|WS_EX_WINDOWEDGE},
        {0, WS_EX_APPWINDOW, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE},
        {0, WS_EX_LAYERED, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_WINDOWEDGE},
        {0, WS_EX_LAYERED|WS_EX_TRANSPARENT, WS_CLIPSIBLINGS|WS_CAPTION, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_WINDOWEDGE},
        {0, WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW, WS_CLIPSIBLINGS|WS_CAPTION,
            WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_WINDOWEDGE},
    };
    int i;

    register_style_check_class();

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if ((tests[i].exstyle_in & WS_EX_LAYERED) && !pGetLayeredWindowAttributes)
            continue;

        winetest_push_context("style %#lx exstyle %#lx", tests[i].style_in, tests[i].exstyle_in);
        check_window_style(tests[i].style_in, tests[i].exstyle_in, tests[i].style_out, tests[i].exstyle_out);
        winetest_pop_context();
    }
}

static INT_PTR WINAPI empty_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return 0;
}

static INT_PTR WINAPI empty_dlg_proc3(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
        EndDialog(hwnd, 0);

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

        parent_is_child = (GetWindowLongA(param->parent, GWL_STYLE) & (WS_POPUP | WS_CHILD)) == WS_CHILD;

        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        if (parent_is_child)
        {
            ok(IsWindowEnabled(param->parent), "wrong state for %08lx\n", style);
            disabled_hwnd = param->grand_parent;
        }
        else
        {
            ok(!IsWindowEnabled(param->parent), "wrong state for %08lx\n", style);
            disabled_hwnd = param->parent;
        }

        if (param->grand_parent)
        {
            if (parent_is_child)
                ok(!IsWindowEnabled(param->grand_parent), "wrong state for %08lx\n", style);
            else
                ok(IsWindowEnabled(param->grand_parent), "wrong state for %08lx\n", style);
        }

        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, disabled_hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(disabled_hwnd), "wrong state for %08lx\n", style);

        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        ok(IsWindowEnabled(param->parent), "wrong state for %p\n", param->parent);
        if (param->grand_parent)
            ok(IsWindowEnabled(param->grand_parent), "wrong state for %p (%08lx)\n", param->grand_parent, style);

        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p\n", hwnd);
        ok(IsWindowEnabled(param->parent), "wrong state for %p\n", param->parent);
        if (param->grand_parent)
            ok(IsWindowEnabled(param->grand_parent), "wrong state for %p (%08lx)\n", param->grand_parent, style);

        param->dlg_data->style |= WS_CHILD;
        DialogBoxIndirectParamA(GetModuleHandleA(NULL), param->dlg_data, hwnd, empty_dlg_proc3, 0);
        ok(IsWindowEnabled(hwnd), "wrong state for %p (%08lx)\n", hwnd, style);

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

    if (style_in & WS_CHILD)
    {
        grand_parent = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW,
                                0, 0, 0, 0, NULL, NULL, NULL, NULL);
        ok(grand_parent != 0, "grand_parent creation failed\n");
    }

    parent = CreateWindowExA(0, "static", NULL, style_in,
                             0, 0, 0, 0, grand_parent, NULL, NULL, NULL);
    ok(parent != 0, "parent creation failed, style %#lx\n", style_in);

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
    ok(hwnd != 0, "dialog creation failed, style %#lx, exstyle %#lx\n", style_in, ex_style_in);

    flush_events( TRUE );

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (style_out | DS_3DLOOK), "got %#lx\n", style);
    ok(ex_style == ex_style_out, "expected ex_style %#lx, got %#lx\n", ex_style_out, ex_style);

    ok(IsWindowEnabled(parent), "wrong parent state (dialog style %#lx)\n", style_in);

    /* try setting the styles explicitly */
    SetWindowLongA(hwnd, GWL_EXSTYLE, ex_style_in);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    ok(style == (style_out | DS_3DLOOK), "got %#lx\n", style);
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (ex_style_in & WS_EX_DLGMODALFRAME)
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else if ((style & (WS_DLGFRAME | WS_THICKFRAME)) && !(ex_style_in & WS_EX_STATICEDGE))
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else
        ex_style_out = ex_style_in & ~WS_EX_WINDOWEDGE;
    ok(ex_style == ex_style_out, "expected ex_style %#lx, got %#lx\n", ex_style_out, ex_style);

    SetWindowLongA(hwnd, GWL_STYLE, style_in);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ex_style = GetWindowLongA(hwnd, GWL_EXSTYLE);
    /* WS_CLIPSIBLINGS can't be reset on top-level windows */
    if ((style_in & (WS_CHILD | WS_POPUP)) == WS_CHILD) style_out = style_in;
    else style_out = style_in | WS_CLIPSIBLINGS;
    ok(style == style_out, "expected style %#lx, got %#lx\n", style_out, style);
    /* WS_EX_WINDOWEDGE can't always be changed */
    if (ex_style_in & WS_EX_DLGMODALFRAME)
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else if ((style & (WS_DLGFRAME | WS_THICKFRAME)) && !(ex_style_in & WS_EX_STATICEDGE))
        ex_style_out = ex_style_in | WS_EX_WINDOWEDGE;
    else
        ex_style_out = ex_style_in & ~WS_EX_WINDOWEDGE;
    ok(ex_style == ex_style_out, "expected ex_style %#lx, got %#lx\n", ex_style_out, ex_style);

    DestroyWindow(hwnd);

    param.parent = parent;
    param.grand_parent = grand_parent;
    param.dlg_data = &dlg_data.dt;
    DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, parent, empty_dlg_proc2, (LPARAM)&param);

    ok(IsWindowEnabled(parent), "wrong parent state (dialog style %#lx)\n", style_in);
    if (grand_parent)
        ok(IsWindowEnabled(grand_parent), "wrong grand parent state (dialog style %#lx)\n", style_in);

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
    ok(ret == 2, "DialogBoxIndirectParam returned %Id\n", ret);

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
    ok(ret == 2, "DialogBoxIndirectParam returned %Id\n", ret);

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
    ok(ret == 2, "DialogBoxIndirectParam returned %Id\n", ret);

    /* If we change parent in WM_INITDIALOG for WS_CHILD dialog WM_ENTERIDLE is still sent to the original
     * parent. EndDialog will enable the new parent. */
    EnableWindow(child, TRUE);
    EnableWindow(other, FALSE);
    dlg_data.dt.style = WS_CHILD;
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, child2, reparent_dlg_proc, (LPARAM)other);
    ok(ret == 2, "DialogBoxIndirectParam returned %Id\n", ret);
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
    ok(ret == 1, "DialogBoxIndirectParam returned %Id\n", ret);
    ok(!IsWindowEnabled(other), "other is not disabled\n");
    ok(!IsWindowEnabled(child), "child is not disabled\n");
    ok(IsWindowEnabled(child2), "child2 is not enabled\n");
    EnableWindow(child, TRUE);
    EnableWindow(other, TRUE);

    /* Quit dialog message loop by sending WM_QUIT message. Dialog owner is not enabled. */
    SetWindowLongPtrA(child, GWLP_WNDPROC, (ULONG_PTR)post_quit_dialog_owner_proc);
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, other, empty_dlg_proc, 0);
    ok(ret == 1, "DialogBoxIndirectParam returned %Id\n", ret);
    ok(!IsWindowEnabled(other), "other is enabled\n");
    EnableWindow(other, TRUE);

    /* Quit dialog message loop by destroying the window. Dialog owner is not enabled. */
    SetWindowLongPtrA(child, GWLP_WNDPROC, (ULONG_PTR)destroy_dialog_owner_proc);
    ret = DialogBoxIndirectParamA(GetModuleHandleA(NULL), &dlg_data.dt, other, empty_dlg_proc, 0);
    ok(ret == 1, "DialogBoxIndirectParam returned %Id\n", ret);
    ok(!IsWindowEnabled(other), "other is enabled\n");
    EnableWindow(other, TRUE);

    DestroyWindow(other);
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
    ok ( colr == 0, "pixel should be black, color is %08lx\n", colr);
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
    ok ( colr == 0, "pixel should be black, color is %08lx\n", colr);

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
    /* create a region with what is expected */
    SetRectRgn( exprgn, 39,0,49,74);
    SetRectRgn( tmprgn, 88,79,98,93);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 0,93,98,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn))
    {
        trace("update rect is %s\n", wine_dbgstr_rect(&rcu));
        dump_region(hrgn);
    }
    /* now with clipping region */
    SelectClipRgn( hdc, clipping);
    ScrollDC( hdc, -10, -5, &rc, &cliprc, hrgn, &rcu);
    /* create a region with what is expected */
    SetRectRgn( exprgn, 39,10,49,74);
    SetRectRgn( tmprgn, 80,79,90,85);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 10,85,90,90);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn))
    {
        trace("update rect is %s\n", wine_dbgstr_rect(&rcu));
        dump_region(hrgn);
    }
    ReleaseDC( hwnd1, hdc);

    /* test scrolling a rect by more than its size */
    DestroyWindow( hwnd2);
    ValidateRect( hwnd1, NULL);
    SetRect( &rc, 40,40, 50,50);
    InvalidateRect( hwnd1, &rc, 1);
    ScrollWindowEx( hwnd1, -20, 0, &rc, NULL, hrgn, &rcu,
      SW_SCROLLCHILDREN | SW_INVALIDATE);
    SetRectRgn( exprgn, 20, 40, 30, 50);
    SetRectRgn( tmprgn, 40, 40, 50, 50);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);
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
    SetRectRgn( exprgn, 88,0,98,98);
    SetRectRgn( tmprgn, 30, 40, 50, 50);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

    /* clear an update region */
    UpdateWindow( hwnd1 );

    SetRect( &rc, 0,40, 100,60);
    SetRect( &cliprc, 0,0, 100,100);
    ScrollWindowEx( hwnd1, 0, -25, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    SetRectRgn( exprgn, 0, 40, 98, 60 );
    ok( EqualRgn( exprgn, hrgn), "wrong update region in excessive scroll\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

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
    SetRectRgn( exprgn, 88,0,98,88);
    SetRectRgn( tmprgn, 0,88,98,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

    /* SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_SCROLLCHILDREN | SW_INVALIDATE);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

    /* no SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) & ~WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    /* expected region is the same as in previous test */
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

    /* WS_CLIPCHILDREN and no SW_SCROLLCHILDREN */
    SetWindowLongA( hwnd1, GWL_STYLE, GetWindowLongA( hwnd1, GWL_STYLE) | WS_CLIPCHILDREN );
    ValidateRect( hwnd1, NULL);
    ValidateRect( hwnd2, NULL);
    ScrollWindowEx( hwnd1, -10, -10, &rc, &cliprc, hrgn, &rcu, SW_INVALIDATE);
    SetRectRgn( exprgn, 88,0,98,20);
    SetRectRgn( tmprgn, 20,20,98,30);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 20,30,30,88);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    SetRectRgn( tmprgn, 0,88,30,98);
    CombineRgn( exprgn, exprgn, tmprgn, RGN_OR);
    ok( EqualRgn( exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn( exprgn, hrgn)) dump_region(hrgn);

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
    ok( ret, "GetScrollRange failed\n" );
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
    ok ( colr == 0, "pixel should be black, color is %08lx\n", colr);
    /* this scroll should not cause any visible changes */ 
    ScrollDC( hdc, 5, -20, &rc, &cliprc, hrgn, &rcu);
    colr = GetPixel( hdc, (rc.left+rc.right)/2, ( rc.top + rc.bottom) /2 - 1);
    ok ( colr == 0, "pixel should be black, color is %08lx\n", colr);
    /* test with NULL clip rect */
    ScrollDC( hdc, 20, -20, &rc, NULL, hrgn, &rcu);
    /*FillRgn(hdc, hrgn, GetStockObject(WHITE_BRUSH));*/
    SetRect(&rc2, 0, 0, 100, 100);
    ok(EqualRect(&rcu, &rc2), "rects do not match %s / %s\n", wine_dbgstr_rect(&rcu),
       wine_dbgstr_rect(&rc2));
    if (!EqualRect(&rcu, &rc2))
    {
        trace("update rect: %s\n", wine_dbgstr_rect(&rcu));
        dump_region(hrgn);
    }

    SetRectRgn( exprgn, 0, 0, 20, 80);
    SetRectRgn( tmprgn, 0, 80, 100, 100);
    CombineRgn(exprgn, exprgn, tmprgn, RGN_OR);
    ok(EqualRgn(exprgn, hrgn), "wrong update region\n");
    if (!EqualRgn(exprgn, hrgn)) dump_region(exprgn);
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
    if (!EqualRgn(exprgn, hrgn))
    {
        trace("update rect: %s\n", wine_dbgstr_rect(&rcu));
        dump_region(hrgn);
    }
    /* clean up */
    ReleaseDC(hwnd1, hdc);
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
    SetLastError(0xdeadbeef);
    rc = GetWindowTextA(hwndMain2, NULL, 1024);
    ok(!rc, "GetWindowText: rc=%d err=%ld\n",rc,GetLastError());

    SetLastError(0xdeadbeef);
    hwnd=CreateWindowA("LISTBOX", "TestList",
                      (LBS_STANDARD & ~LBS_SORT),
                      0, 0, 100, 100,
                      NULL, (HMENU)1, NULL, 0);

    ok(!hwnd || broken(hwnd != NULL), /* w2k3 sp2 */
       "CreateWindow with invalid menu handle should fail\n");
    if (!hwnd)
        ok(GetLastError() == ERROR_INVALID_MENU_HANDLE,
           "wrong last error value %ld\n", GetLastError());
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
    ok(hwnd != NULL, "Failed to create window class=%s, style=0x%08lx, exStyle=0x%08lx\n", class, style, exStyle);

    ShowWindow(hwnd, SW_SHOW);
    flush_events(TRUE);

    /* retry setting the maximized state to workaround a FVWM bug */
    if ((style & WS_MAXIMIZE) && !(GetWindowLongW(hwnd, GWL_STYLE) & WS_MAXIMIZE))
    {
        ShowWindow(hwnd, SW_MAXIMIZE);
        flush_events(TRUE);
    }

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
    for (i = 0; i < ARRAY_SIZE(styles); ++i)
        test_AWRwindow(szAWRClass, styles[i], 0, menu);
    for (i = 0; i < ARRAY_SIZE(exStyles); ++i) {
        test_AWRwindow(szAWRClass, WS_POPUP, exStyles[i], menu);
        test_AWRwindow(szAWRClass, WS_THICKFRAME, exStyles[i], menu);
    }
}

static void test_AWR_flags(void)
{
    static const DWORD styles[] = { WS_POPUP, WS_BORDER, WS_DLGFRAME, WS_THICKFRAME, WS_MINIMIZE };
    static const DWORD exStyles[] = { WS_EX_CLIENTEDGE, WS_EX_TOOLWINDOW, WS_EX_WINDOWEDGE,
                                      WS_EX_APPWINDOW, WS_EX_DLGMODALFRAME, WS_EX_STATICEDGE };

    DWORD i, j, k, style, exstyle;
    RECT rect, rect2;

    for (i = 0; i < (1 << ARRAY_SIZE(styles)); i++)
    {
        for (k = style = 0; k < ARRAY_SIZE(styles); k++) if (i & (1 << k)) style |= styles[k];

        for (j = 0; j < (1 << ARRAY_SIZE(exStyles)); j++)
        {
            for (k = exstyle = 0; k < ARRAY_SIZE(exStyles); k++) if (j & (1 << k)) exstyle |= exStyles[k];
            SetRect( &rect, 100, 100, 200, 200 );
            rect2 = rect;
            AdjustWindowRectEx( &rect, style, FALSE, exstyle );
            wine_AdjustWindowRectEx( &rect2, style, FALSE, exstyle );
            ok( EqualRect( &rect, &rect2 ), "%08lx %08lx rects do not match: win %s wine %s\n",
                style, exstyle, wine_dbgstr_rect( &rect ), wine_dbgstr_rect( &rect2 ));
            if (pAdjustWindowRectExForDpi)
            {
                SetRect( &rect, 100, 100, 200, 200 );
                rect2 = rect;
                pAdjustWindowRectExForDpi( &rect, style, FALSE, exstyle, 192 );
                wine_AdjustWindowRectExForDpi( &rect2, style, FALSE, exstyle, 192 );
                ok( EqualRect( &rect, &rect2 ), "%08lx %08lx rects do not match: win %s wine %s\n",
                    style, exstyle, wine_dbgstr_rect( &rect ), wine_dbgstr_rect( &rect2 ));
            }
        }
    }
}

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
   BOOL ret;

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
   ret = RegisterClassA(&cls);
   ok(ret, "Failed to register a test class.\n");

   hwndMain = CreateWindowA("RedrawWindowClass", "Main Window", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, 100, 100, NULL, NULL, 0, NULL);

   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   ShowWindow(hwndMain, SW_SHOW);
   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   RedrawWindow(hwndMain, NULL,NULL,RDW_UPDATENOW | RDW_ALLCHILDREN);
   ok( WMPAINT_count == 1,
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
        if (winetest_debug > 1)
            trace("WM_PAINT: hwnd %p, client rect %s, window rect %s\n", hwnd,
                  wine_dbgstr_rect(&t->client), wine_dbgstr_rect(&rc));
        BeginPaint(hwnd, &ps);
        t->paint = ps.rcPaint;
        GetClipBox(ps.hdc, &rc);
        t->clip = rc;
        if (winetest_debug > 1)
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
      ": expected %ld, got %ld\n", \
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
   BOOL ret;
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
   ret = RegisterClassA(&clsMain);
   ok(ret, "Failed to register a test class.\n");

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
   ret = RegisterClassA(&cls);
   ok(ret, "Failed to register a test class.\n");

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
    RegisterClassW(&classW);

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
        ok( cs->cx == expected_cx || broken(cs->cx == (short)expected_cx),
            "%p msg %x wrong x size %d/%d\n", hwnd, msg, cs->cx, expected_cx );
        ok( cs->cy == expected_cy || broken(cs->cy == (short)expected_cy),
            "%p msg %x wrong y size %d/%d\n", hwnd, msg, cs->cy, expected_cy );
        ok( (rect.right - rect.left == expected_rect.right - expected_rect.left &&
             rect.bottom - rect.top == expected_rect.bottom - expected_rect.top) ||
            (rect.right - rect.left == min( 65535, expected_rect.right - expected_rect.left ) &&
             rect.bottom - rect.top == min( 65535, expected_rect.bottom - expected_rect.top )) ||
            broken( rect.right - rect.left == broken_rect.right - broken_rect.left &&
                    rect.bottom - rect.top == broken_rect.bottom - broken_rect.top) ||
            broken( rect.right - rect.left == (short)broken_rect.right - (short)broken_rect.left &&
                    rect.bottom - rect.top == (short)broken_rect.bottom - (short)broken_rect.top),
            "%p msg %x wrong rect %s / %s\n", hwnd, msg, wine_dbgstr_rect( &rect ),
            wine_dbgstr_rect( &expected_rect ));
        return DefWindowProcA(hwnd, msg, wp, lp);
    }
    case WM_NCCALCSIZE:
    {
        LRESULT ret;
        RECT rect, *r = (RECT *)lp;
        GetWindowRect( hwnd, &rect );
        ok( EqualRect( &rect, r ), "passed rect %s doesn't match window rect %s\n",
            wine_dbgstr_rect( r ), wine_dbgstr_rect( &rect ));
        ret = DefWindowProcA(hwnd, msg, wp, lp);
        ok(!ret, "got %08Ix\n", ret);
        return ret;
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
    HDC hdc;

#define expect_menu(window, menu) \
    SetLastError(0xdeadbeef); \
    res = (GetMenu(window) == (HMENU)menu); \
    ok(res, "GetMenu error %ld\n", GetLastError())

#define expect_style(window, style)\
    ok((ULONG)GetWindowLongA(window, GWL_STYLE) == (style), "expected style %lx != %lx\n", (LONG)(style), GetWindowLongA(window, GWL_STYLE))

#define expect_ex_style(window, ex_style)\
    ok((ULONG)GetWindowLongA(window, GWL_EXSTYLE) == (ex_style), "expected ex_style %lx != %lx\n", (LONG)(ex_style), GetWindowLongA(window, GWL_EXSTYLE))

    hmenu = CreateMenu();
    assert(hmenu != 0);
    parent = GetDesktopWindow();
    assert(parent != 0);

    SetLastError(0xdeadbeef);
    res = IsMenu(hmenu);
    ok(res, "IsMenu error %ld\n", GetLastError());

    /* WS_CHILD */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, 1);
    expect_style(hwnd, WS_CHILD | WS_CAPTION);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);

    /* WS_POPUP */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    /* WS_CHILD | WS_POPUP */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd, "CreateWindowEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd, "CreateWindowEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd, "CreateWindowEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, 0);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, (HMENU)1, 0, NULL);
    ok(!hwnd, "CreateWindowEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());
    if (hwnd)
        DestroyWindow(hwnd);

    hmenu = CreateMenu();
    assert(hmenu != 0);
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD | WS_POPUP | WS_CAPTION,
                           0, 0, 100, 100, parent, hmenu, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
    expect_menu(hwnd, hmenu);
    expect_style(hwnd, WS_CHILD | WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS);
    expect_ex_style(hwnd, WS_EX_WINDOWEDGE);
    DestroyWindow(hwnd);
    SetLastError(0xdeadbeef);
    ok(!IsMenu(hmenu), "IsMenu should fail\n");
    ok(GetLastError() == ERROR_INVALID_MENU_HANDLE, "IsMenu set error %ld\n", GetLastError());

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
    ok(parent != 0, "CreateWindowEx error %ld\n", GetLastError());
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

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "MinMax_WndClass", NULL, WS_CHILD | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                          rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                          parent, (HMENU)1, 0, NULL);
    ok(hwnd != 0, "CreateWindowEx error %ld\n", GetLastError());
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
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 200000 || rc.right == 65535 || broken(rc.right == (short)200000),
        "invalid rect right %lu\n", rc.right );
    ok( rc.bottom == 200000 || rc.bottom == 65535 || broken(rc.bottom == (short)200000),
        "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -10;
    SetRectEmpty(&expected_rect);
    SetRect( &broken_rect, 0, 0, -10, -10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -20, -20, -10, -10, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %lu\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = expected_cy = -200000;
    SetRectEmpty(&expected_rect);
    SetRect( &broken_rect, 0, 0, -200000, -200000 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, -300000, -300000, -200000, -200000, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0, "invalid rect right %lu\n", rc.right );
    ok( rc.bottom == 0, "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    /* we need a parent at 0,0 so that child coordinates match */
    DestroyWindow(parent);
    parent = CreateWindowExA(0, "MinMax_WndClass", NULL, WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowEx error %ld\n", GetLastError());

    expected_cx = 100;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 10, 10, 110, 0x7fffffff );
    SetRect( &broken_rect, 10, 10, 110, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 10, 10, 100, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 100, "invalid rect right %lu\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || rc.bottom ==65535 || broken(rc.bottom == 0),
        "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    expected_cx = 0x7fffffff;
    expected_cy = 0x7fffffff;
    SetRect( &expected_rect, 20, 10, 0x7fffffff, 0x7fffffff );
    SetRect( &broken_rect, 20, 10, 0x7fffffffU + 20, 0x7fffffffU + 10 );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_CHILD, 20, 10, 0x7fffffff, 0x7fffffff, parent, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right == 0x7fffffff - 20 || rc.right == 65535 || broken(rc.right == 0),
        "invalid rect right %lu\n", rc.right );
    ok( rc.bottom == 0x7fffffff - 10 || rc.right == 65535 || broken(rc.bottom == 0),
        "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    /* top level window */
    expected_cx = expected_cy = 200000;
    SetRect( &expected_rect, 0, 0, GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CYMAXTRACK) );
    hwnd = CreateWindowExA(0, "Sizes_WndClass", NULL, WS_OVERLAPPEDWINDOW, 300000, 300000, 200000, 200000, 0, 0, 0, NULL);
    ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
    GetClientRect( hwnd, &rc );
    ok( rc.right <= expected_cx, "invalid rect right %lu\n", rc.right );
    ok( rc.bottom <= expected_cy, "invalid rect bottom %lu\n", rc.bottom );
    DestroyWindow(hwnd);

    /* invalid class */
    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "INVALID_CLASS", NULL, WS_CHILD, 10, 10, 100, 100, parent, 0, 0, NULL);
    ok(hwnd == 0, "CreateWindowEx succeeded\n");
    ok(GetLastError() == ERROR_CLASS_DOES_NOT_EXIST || GetLastError() == ERROR_CANNOT_FIND_WND_CLASS,
        "invalid error %lu\n", GetLastError());
    DestroyWindow(hwnd);

    hdc = GetDC( parent );
    SetLayout( hdc, LAYOUT_RTL );
    if (GetLayout( hdc ))
    {
        ReleaseDC( parent, hdc );
        DestroyWindow( parent );
        SetLastError( 0xdeadbeef );
        parent = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_LAYOUTRTL, "static", NULL, WS_POPUP,
                                0, 0, 100, 100, 0, 0, 0, NULL);
        ok( parent != 0, "creation failed err %lu\n", GetLastError());
        expect_ex_style( parent, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL );
        hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 20, 20, parent, 0, 0, NULL);
        ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
        expect_ex_style( hwnd, WS_EX_LAYOUTRTL );
        DestroyWindow( hwnd );
        hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0, 0, 20, 20, parent, 0, 0, NULL);
        ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
        expect_ex_style( hwnd, 0 );
        DestroyWindow( hwnd );
        SetWindowLongW( parent, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT );
        hwnd = CreateWindowExA(0, "static", NULL, WS_CHILD, 0, 0, 20, 20, parent, 0, 0, NULL);
        ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
        expect_ex_style( hwnd, 0 );
        DestroyWindow( hwnd );

        if (pGetProcessDefaultLayout && pSetProcessDefaultLayout)
        {
            DWORD layout;

            SetLastError( 0xdeadbeef );
            ok( !pGetProcessDefaultLayout( NULL ), "GetProcessDefaultLayout succeeded\n" );
            ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );
            SetLastError( 0xdeadbeef );
            res = pGetProcessDefaultLayout( &layout );
            ok( res, "GetProcessDefaultLayout failed err %lu\n", GetLastError ());
            ok( layout == 0, "GetProcessDefaultLayout wrong layout %lx\n", layout );
            SetLastError( 0xdeadbeef );
            res = pSetProcessDefaultLayout( 7 );
            ok( res, "SetProcessDefaultLayout failed err %lu\n", GetLastError ());
            res = pGetProcessDefaultLayout( &layout );
            ok( res, "GetProcessDefaultLayout failed err %lu\n", GetLastError ());
            ok( layout == 7, "GetProcessDefaultLayout wrong layout %lx\n", layout );
            SetLastError( 0xdeadbeef );
            res = pSetProcessDefaultLayout( LAYOUT_RTL );
            ok( res, "SetProcessDefaultLayout failed err %lu\n", GetLastError ());
            res = pGetProcessDefaultLayout( &layout );
            ok( res, "GetProcessDefaultLayout failed err %lu\n", GetLastError ());
            ok( layout == LAYOUT_RTL, "GetProcessDefaultLayout wrong layout %lx\n", layout );
            hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                                  0, 0, 100, 100, 0, 0, 0, NULL);
            ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
            expect_ex_style( hwnd, WS_EX_APPWINDOW | WS_EX_LAYOUTRTL );
            DestroyWindow( hwnd );
            hwnd = CreateWindowExA(WS_EX_APPWINDOW, "static", NULL, WS_POPUP,
                                  0, 0, 100, 100, parent, 0, 0, NULL);
            ok( hwnd != 0, "creation failed err %lu\n", GetLastError());
            expect_ex_style( hwnd, WS_EX_APPWINDOW );
            DestroyWindow( hwnd );
            pSetProcessDefaultLayout( 0 );
        }
        else win_skip( "SetProcessDefaultLayout not supported\n" );
    }
    else win_skip( "SetLayout not supported\n" );

    DestroyWindow(parent);

    UnregisterClassA("MinMax_WndClass", GetModuleHandleA(NULL));
    UnregisterClassA("Sizes_WndClass", GetModuleHandleA(NULL));

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

static void test_set_window_long_size(void)
{
#ifdef _WIN64
    WNDPROC wnd_proc, wnd_proc_2;
    LONG_PTR retval;
    HWND hwnd;
    LONG ret;

    /* It's not allowed to set or get 64-bit pointer values using 32-bit functions. */
    hwnd = CreateWindowExA(0, "MainWindowClass", "Child window", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CHILD |
            WS_MAXIMIZEBOX | WS_VISIBLE, 100, 100, 200, 200, hwndMain, 0, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create test window.\n");

    /* GWLP_WNDPROC */
    SetLastError(0xdeadbeef);
    wnd_proc = (WNDPROC)(LONG_PTR)GetWindowLongA(hwnd, GWLP_WNDPROC);
    ok(!wnd_proc && GetLastError() == ERROR_INVALID_INDEX, "Unexpected window proc.\n");

    wnd_proc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    ok(!!wnd_proc, "Unexpected window proc.\n");

    SetLastError(0xdeadbeef);
    wnd_proc_2 = (WNDPROC)(LONG_PTR)SetWindowLongA(hwnd, GWLP_WNDPROC, 0xdeadbeef);
    ok(!wnd_proc_2 && GetLastError() == ERROR_INVALID_INDEX, "Unexpected window proc.\n");

    wnd_proc_2 = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    ok(wnd_proc_2 == wnd_proc, "Unexpected window proc.\n");

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_WNDPROC);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected return value.\n");

    /* GWLP_USERDATA */
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, ((LONG_PTR)1 << 32) | 123);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    retval = GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    ok(retval > 123, "Unexpected user data.\n");
    ret = GetWindowWord(hwnd, GWLP_USERDATA);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    ret = SetWindowWord(hwnd, GWLP_USERDATA, 124);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret == 124, "Unexpected user data %#lx.\n", ret);
    retval = GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    ok(retval == 124, "Unexpected user data.\n");

    SetWindowLongA(hwnd, GWLP_USERDATA, (1 << 16) | 123);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret == ((1 << 16) | 123), "Unexpected user data %#lx.\n", ret);
    ret = GetWindowWord(hwnd, GWLP_USERDATA);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);

    ret = SetWindowWord(hwnd, GWLP_USERDATA, 124);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret == ((1 << 16) | 124), "Unexpected user data %#lx.\n", ret);
    ret = GetWindowWord(hwnd, GWLP_USERDATA);
    ok(ret == 124, "Unexpected user data %#lx.\n", ret);

    /* GWLP_ID */
    ret = SetWindowLongA(hwnd, GWLP_ID, 1);
    ok(!ret, "Unexpected id %#lx.\n", ret);

    ret = GetWindowLongA(hwnd, GWLP_ID);
    ok(ret == 1, "Unexpected id %#lx.\n", ret);

    ret = GetWindowLongW(hwnd, GWLP_ID);
    ok(ret == 1, "Unexpected id %#lx.\n", ret);

    SetWindowLongPtrA(hwnd, GWLP_ID, ((LONG_PTR)1 << 32) | 123);
    ret = GetWindowLongA(hwnd, GWLP_ID);
    ok(ret == 123, "Unexpected id %#lx.\n", ret);
    ret = GetWindowLongW(hwnd, GWLP_ID);
    ok(ret == 123, "Unexpected id %#lx.\n", ret);
    retval = GetWindowLongPtrA(hwnd, GWLP_ID);
    ok(retval > 123, "Unexpected id.\n");
    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_ID);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected id %#lx.\n", ret);

    /* GWLP_HINSTANCE */
    retval = GetWindowLongPtrA(hwnd, GWLP_HINSTANCE);
    ok(retval == (LONG_PTR)GetModuleHandleA(NULL), "Unexpected instance %#Ix.\n", retval);

    SetLastError(0xdeadbeef);
    ret = GetWindowLongA(hwnd, GWLP_HINSTANCE);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetWindowLongW(hwnd, GWLP_HINSTANCE);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_HINSTANCE);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = SetWindowLongA(hwnd, GWLP_HINSTANCE, 1);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = SetWindowLongW(hwnd, GWLP_HINSTANCE, 1);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    /* GWLP_HWNDPARENT */
    SetLastError(0xdeadbeef);
    ret = GetWindowLongA(hwnd, GWLP_HWNDPARENT);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected parent window %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = SetWindowLongA(hwnd, GWLP_HWNDPARENT, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected parent window %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = SetWindowLongW(hwnd, GWLP_HWNDPARENT, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected parent window %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_HWNDPARENT);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected parent window %#lx.\n", ret);

    DestroyWindow(hwnd);
#endif
}

static void test_set_window_word_size(void)
{
    WNDPROC wnd_proc, wnd_proc_2;
    LONG_PTR retval;
    HWND hwnd;
    LONG ret;

    /* It's not allowed to set or get 64-bit pointer values using 32-bit functions. */
    hwnd = CreateWindowExA(0, "MainWindowClass", "Child window", WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CHILD |
            WS_MAXIMIZEBOX | WS_VISIBLE, 100, 100, 200, 200, hwndMain, 0, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create test window.\n");

    /* GWLP_WNDPROC */
    wnd_proc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    ok(!!wnd_proc, "Unexpected window proc.\n");

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_WNDPROC);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected window proc.\n");

    SetLastError(0xdeadbeef);
    ret = SetWindowWord(hwnd, GWLP_WNDPROC, 0xbeef);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected window proc.\n");

    wnd_proc_2 = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    ok(wnd_proc_2 == wnd_proc, "Unexpected window proc.\n");

    /* GWLP_USERDATA */
    ret = SetWindowLongA(hwnd, GWLP_USERDATA, (1 << 16) | 123);
    ok(!ret, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret > 123, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowWord(hwnd, GWLP_USERDATA);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    ret = SetWindowWord(hwnd, GWLP_USERDATA, 124);
    ok(ret == 123, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowWord(hwnd, GWLP_USERDATA);
    ok(ret == 124, "Unexpected user data %#lx.\n", ret);
    ret = GetWindowLongA(hwnd, GWLP_USERDATA);
    ok(ret == ((1 << 16) | 124), "Unexpected user data %#lx.\n", ret);

    /* GWLP_ID */
    ret = SetWindowLongA(hwnd, GWLP_ID, 1);
    ok(!ret, "Unexpected id %#lx.\n", ret);
    ret = GetWindowLongA(hwnd, GWLP_ID);
    ok(ret == 1, "Unexpected id %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_ID);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected id %#lx.\n", ret);
    SetLastError(0xdeadbeef);
    ret = SetWindowWord(hwnd, GWLP_ID, 2);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected id %#lx.\n", ret);

    /* GWLP_HINSTANCE */
    retval = GetWindowLongPtrA(hwnd, GWLP_HINSTANCE);
    ok(retval == (LONG_PTR)GetModuleHandleA(NULL), "Unexpected instance %#Ix.\n", retval);

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_HINSTANCE);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = SetWindowWord(hwnd, GWLP_HINSTANCE, 0xdead);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected instance %#lx.\n", ret);

    /* GWLP_HWNDPARENT */
    retval = GetWindowLongPtrA(hwnd, GWLP_HWNDPARENT);
    ok(!!retval, "Unexpected parent window %#lx.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetWindowWord(hwnd, GWLP_HWNDPARENT);
    ok(!ret && GetLastError() == ERROR_INVALID_INDEX, "Unexpected parent window %#lx.\n", ret);

    DestroyWindow(hwnd);
}

static void test_SetWindowLong(void)
{
    LONG_PTR retval;
    WNDPROC old_window_procW;

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrA(NULL, GWLP_WNDPROC, 0);
    ok(!retval, "SetWindowLongPtr on invalid window handle should have returned 0 instead of 0x%Ix\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_WINDOW_HANDLE),
        "SetWindowLongPtr should have set error to ERROR_INVALID_WINDOW_HANDLE instead of %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrA(hwndMain, 0xdeadbeef, 0);
    ok(!retval, "SetWindowLongPtr on invalid index should have returned 0 instead of 0x%Ix\n", retval);
    ok(check_error(GetLastError(), ERROR_INVALID_INDEX),
        "SetWindowLongPtr should have set error to ERROR_INVALID_INDEX instead of %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrA(hwndMain, GWLP_WNDPROC, 0);
    ok((WNDPROC)retval == main_window_procA,
        "SetWindowLongPtr on invalid window proc should have returned address of main_window_procA instead of 0x%Ix\n", retval);
    ok(GetLastError() == 0xdeadbeef, "SetWindowLongPtr shouldn't have set the last error, instead of setting it to %ld\n", GetLastError());
    retval = GetWindowLongPtrA(hwndMain, GWLP_WNDPROC);
    ok((WNDPROC)retval == main_window_procA,
        "SetWindowLongPtr on invalid window proc shouldn't have changed the value returned by GetWindowLongPtr, instead of changing it to 0x%Ix\n", retval);
    ok(!IsWindowUnicode(hwndMain), "hwndMain shouldn't be Unicode\n");

    old_window_procW = (WNDPROC)GetWindowLongPtrW(hwndMain, GWLP_WNDPROC);
    SetLastError(0xdeadbeef);
    retval = SetWindowLongPtrW(hwndMain, GWLP_WNDPROC, 0);
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(GetLastError() == 0xdeadbeef, "SetWindowLongPtr shouldn't have set the last error, instead of setting it to %ld\n", GetLastError());
        ok(retval != 0, "SetWindowLongPtr error %ld\n", GetLastError());
        ok((WNDPROC)retval == old_window_procW,
            "SetWindowLongPtr on invalid window proc shouldn't have changed the value returned by GetWindowLongPtr, instead of changing it to 0x%Ix\n", retval);
        ok(IsWindowUnicode(hwndMain), "hwndMain should now be Unicode\n");

        /* Make sure nothing changes if we set the same proc */
        retval = SetWindowLongPtrW(hwndMain, GWLP_WNDPROC, (LONG_PTR)old_window_procW);
        todo_wine
        ok((WNDPROC)retval == main_window_procA, "unexpected proc 0x%Ix\n", retval);
        retval = GetWindowLongPtrW(hwndMain, GWLP_WNDPROC);
        ok((WNDPROC)retval == old_window_procW, "unexpected proc 0x%Ix\n", retval);
        retval = GetWindowLongPtrA(hwndMain, GWLP_WNDPROC);
        ok((WNDPROC)retval == main_window_procA, "unexpected proc 0x%Ix\n", retval);

        /* set it back to ANSI */
        SetWindowLongPtrA(hwndMain, GWLP_WNDPROC, 0);
    }

    test_set_window_long_size();
    test_set_window_word_size();
}

static LRESULT WINAPI check_style_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const STYLESTRUCT *expected = (STYLESTRUCT *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    const STYLESTRUCT *got = (STYLESTRUCT *)lParam;

    if (message == WM_STYLECHANGING && wParam == GWL_STYLE)
    {
        ok(got->styleOld == expected[0].styleOld, "expected old style %#lx, got %#lx\n",
            expected[0].styleOld, got->styleOld);
        ok(got->styleNew == expected[0].styleNew, "expected new style %#lx, got %#lx\n",
            expected[0].styleNew, got->styleNew);
    }
    else if (message == WM_STYLECHANGED && wParam == GWL_STYLE)
    {
        ok(got->styleOld == expected[1].styleOld, "expected old style %#lx, got %#lx\n",
            expected[1].styleOld, got->styleOld);
        ok(got->styleNew == expected[1].styleNew, "expected new style %#lx, got %#lx\n",
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

    for (i = 0; i < ARRAY_SIZE(tests); i++)
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
        ok(old_style == tests[i].creation_style, "expected old style %#lx, got %#lx\n",
            tests[i].creation_style, old_style);
        new_style = GetWindowLongA(hwnd, GWL_STYLE);
        ok(new_style == expected_style, "expected new style %#lx, got %#lx\n",
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
    RECT rcMain, rc, rcMinimized, rcClient, rcEmpty, rcMaximized, rcResized, rcNonClient, rcBroken;
    LPARAM ret;
    MONITORINFO mon_info;
    unsigned int i;

    DWORD test_style[] =
    {
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX | WS_THICKFRAME,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_THICKFRAME,
        WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU,
        WS_OVERLAPPED | WS_VISIBLE | WS_THICKFRAME,
        WS_OVERLAPPED | WS_VISIBLE
    };

    SetRect(&rcClient, 0, 0, 90, 90);
    rcMain = rcClient;
    OffsetRect(&rcMain, 120, 120);
    AdjustWindowRect(&rcMain, WS_CAPTION, 0);
    SetRect(&rcMinimized, -32000, -32000, -32000 + GetSystemMetrics(SM_CXMINIMIZED),
                                          -32000 + GetSystemMetrics(SM_CYMINIMIZED));
    SetRectEmpty(&rcEmpty);

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL,
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_POPUP,
                          rcMain.left, rcMain.top,
                          rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                          0, 0, 0, NULL);
    assert(hwnd);

    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mon_info);
    rcMaximized = mon_info.rcWork;
    AdjustWindowRectEx(&rcMaximized, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BORDER,
                       0, GetWindowLongA(hwnd, GWL_EXSTYLE));

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(!(style & WS_VISIBLE), "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));
    /* shouldn't be able to resize minimized windows */
    ret = SetWindowPos(hwnd, 0, 0, 0,
                       (rcMinimized.right - rcMinimized.left) * 2,
                       (rcMinimized.bottom - rcMinimized.top) * 2,
                       SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "not expected ret: %Iu\n", ret);
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));
    /* SetWindowPos shouldn't affect the client rect */
    ret = SetWindowPos(hwnd, 0, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "not expected ret: %Iu\n", ret);
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));
    /* test NC area */
    GetWindowRect(hwnd, &rc);
    SetRect(&rcNonClient, rc.left, rc.top, rc.left, rc.top);
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rc);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&rc, &rcNonClient), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcNonClient), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_MAXIMIZE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should be minimized\n");
    ok(style & WS_MAXIMIZE, "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMaximized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMaximized), wine_dbgstr_rect(&rc));
    /* maximized windows can be resized */
    ret = SetWindowPos(hwnd, 0, 300, 300, 200, 200, SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "not expected ret: %Iu\n", ret);
    SetRect(&rcResized, 300, 300, 500, 500);
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcResized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcResized), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));

    ret = EnableWindow(hwnd, FALSE);
    ok(!ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    ok(!ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    ok(!ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    ok(!ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "not expected ret: %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_DISABLED, "window should be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMain, &rc), "expected %s, got %s\n", wine_dbgstr_rect(&rcMain),
       wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcClient, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcClient), wine_dbgstr_rect(&rc));

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %Iu\n", ret);
    ok(IsWindow(hwnd), "window should exist\n");

    ret = EnableWindow(hwnd, TRUE);
    ok(ret, "not expected ret: %Iu\n", ret);

    ret = DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok(!ret, "not expected ret: %Iu\n", ret);
    ok(!IsWindow(hwnd), "window should not exist\n");

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL,
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_POPUP | WS_MINIMIZE,
                          rcMain.left, rcMain.top,
                          rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                          0, 0, 0, NULL);
    ok(hwnd != NULL, "failed to create window with error %lu\n", GetLastError());
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    GetWindowRect(hwnd, &rc);
    todo_wine
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL,
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_POPUP | WS_MINIMIZE | WS_VISIBLE,
                          rcMain.left, rcMain.top,
                          rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                          0, 0, 0, NULL);
    ok(hwnd != NULL, "failed to create window with error %lu\n", GetLastError());
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    GetWindowRect(hwnd, &rc);
    ok(EqualRect(&rcMinimized, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcMinimized), wine_dbgstr_rect(&rc));
    GetClientRect(hwnd, &rc);
    ok(EqualRect(&rcEmpty, &rc), "expected %s, got %s\n",
       wine_dbgstr_rect(&rcEmpty), wine_dbgstr_rect(&rc));
    DestroyWindow(hwnd);

    flush_events(TRUE);

    /* test maximize and restore windows without setting WS_CAPTION */

    for (i = 0; i < ARRAY_SIZE(test_style); ++i)
    {
        SetRect(&rcMain, 0, 0, 90, 90);
        OffsetRect(&rcMain, 120, 120);
        hwnd = CreateWindowExA(0, "MainWindowClass", NULL, test_style[i],
                               rcMain.left, rcMain.top,
                               rcMain.right - rcMain.left, rcMain.bottom - rcMain.top,
                               0, 0, 0, NULL);
        ok(hwnd != NULL, "Test %u: failed to create window with error %lu\n", i, GetLastError());

        GetWindowRect(hwnd, &rcMain);
        ok(rcMain.left   > mon_info.rcMonitor.left   &&
           rcMain.right  < mon_info.rcMonitor.right  &&
           rcMain.top    > mon_info.rcMonitor.top    &&
           rcMain.bottom < mon_info.rcMonitor.bottom,
           "Test %u: window should not be fullscreen\n", i);

        rcMaximized = (test_style[i] & WS_MAXIMIZEBOX) ? mon_info.rcWork : mon_info.rcMonitor;
        AdjustWindowRectEx(&rcMaximized, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BORDER,
                           0, GetWindowLongA(hwnd, GWL_EXSTYLE));

        ret = ShowWindow(hwnd, SW_MAXIMIZE);
        ok(ret, "unexpected ret: %Iu\n", ret);
        style = GetWindowLongA(hwnd, GWL_STYLE);
        ok(style & WS_MAXIMIZE, "Test %u: window should be maximized\n", i);
        GetWindowRect(hwnd, &rc);

        rcBroken = rcMaximized;
        if (test_style[i] & WS_THICKFRAME)
        {
            InflateRect(&rcBroken, -2, -2);
            OffsetRect(&rcBroken, -2, -2);
        }

        ok(EqualRect(&rcMaximized, &rc) || broken(EqualRect(&rcBroken, &rc)),
           "Test %u: expected %s, got %s\n", i, wine_dbgstr_rect(&rcMaximized),
           wine_dbgstr_rect(&rc));

        ret = ShowWindow(hwnd, SW_RESTORE);
        ok(ret, "unexpected ret: %Iu\n", ret);
        style = GetWindowLongA(hwnd, GWL_STYLE);
        ok(!(style & WS_MAXIMIZE), "Test %u: window should not be maximized\n", i);
        GetWindowRect(hwnd, &rc);
        ok(EqualRect(&rcMain, &rc), "Test %u: expected %s, got %s\n",
           i, wine_dbgstr_rect(&rcMain), wine_dbgstr_rect(&rc));

        DestroyWindow(hwnd);

        flush_events(TRUE);
    }
}

static void test_ShowWindow_owned(HWND hwndMain)
{
    MONITORINFO mon_info = {sizeof(mon_info)};
    RECT rect, orig, expect, nc;
    LPARAM ret;
    HWND hwnd, hwnd2;
    LONG style;

    GetMonitorInfoW(MonitorFromWindow(hwndMain, MONITOR_DEFAULTTOPRIMARY), &mon_info);
    SetRect(&orig, 20, 20, 210, 110);
    hwnd = CreateWindowA("MainWindowClass", "owned", WS_CAPTION | WS_SYSMENU |
                         WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                         orig.left, orig.top, orig.right - orig.left,
                         orig.bottom - orig.top, hwndMain, 0, 0, NULL);
    ok(!!hwnd, "failed to create window, error %lu\n", GetLastError());
    hwnd2 = CreateWindowA("MainWindowClass", "owned2", WS_CAPTION | WS_SYSMENU |
                          WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
                          orig.left, orig.top, orig.right - orig.left,
                          orig.bottom - orig.top, hwndMain, 0, 0, NULL);
    ok(!!hwnd2, "failed to create window, error %lu\n", GetLastError());

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(!(style & WS_VISIBLE), "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    SetRect(&expect, 0, mon_info.rcWork.bottom - GetSystemMetrics(SM_CYMINIMIZED),
            GetSystemMetrics(SM_CXMINIMIZED), mon_info.rcWork.bottom);
    todo_wine
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* shouldn't be able to resize minimized windows */
    ret = SetWindowPos(hwnd, 0, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    todo_wine
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* test NC area */
    GetWindowRect(hwnd, &rect);
    SetRect(&nc, rect.left, rect.top, rect.left, rect.top);
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rect);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&rect, &nc), "expected %s, got %s\n",
       wine_dbgstr_rect(&nc), wine_dbgstr_rect(&rect));

    /* multiple minimized owned windows stack next to each other (and eventually
     * on top of each other) */
    OffsetRect(&expect, GetSystemMetrics(SM_CXMINIMIZED), 0);
    ret = ShowWindow(hwnd2, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd2, &rect);
    todo_wine
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MAXIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should be minimized\n");
    ok(style & WS_MAXIMIZE, "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    expect = mon_info.rcWork;
    AdjustWindowRectEx(&expect, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BORDER,
                       0, GetWindowLongA(hwnd, GWL_EXSTYLE));
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* maximized windows can be resized */
    ret = SetWindowPos(hwnd, 0, 300, 300, 200, 200, SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    SetRect(&expect, 300, 300, 500, 500);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
}

static void test_ShowWindow_child(HWND hwndMain)
{
    RECT rect, orig, expect, nc;
    LPARAM ret;
    HWND hwnd, hwnd2;
    LONG style;
    POINT pt = {0};

    SetRect(&orig, 20, 20, 210, 110);
    hwnd = CreateWindowA("MainWindowClass", "child", WS_CAPTION | WS_SYSMENU |
                         WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CHILD,
                         orig.left, orig.top, orig.right - orig.left,
                         orig.bottom - orig.top, hwndMain, 0, 0, NULL);
    ok(!!hwnd, "failed to create window, error %lu\n", GetLastError());
    hwnd2 = CreateWindowA("MainWindowClass", "child2", WS_CAPTION | WS_SYSMENU |
                          WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CHILD | WS_VISIBLE,
                          orig.left, orig.top, orig.right - orig.left,
                          orig.bottom - orig.top, hwndMain, 0, 0, NULL);
    ok(!!hwnd2, "failed to create window, error %lu\n", GetLastError());

    ClientToScreen(hwndMain, &pt);
    OffsetRect(&orig, pt.x, pt.y);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(!(style & WS_VISIBLE), "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    GetClientRect(hwndMain, &expect);
    SetRect(&expect, 0, expect.bottom - GetSystemMetrics(SM_CYMINIMIZED),
            GetSystemMetrics(SM_CXMINIMIZED), expect.bottom);
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* shouldn't be able to resize minimized windows */
    ret = SetWindowPos(hwnd, 0, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* test NC area */
    GetWindowRect(hwnd, &rect);
    SetRect(&nc, rect.left, rect.top, rect.left, rect.top);
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rect);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&rect, &nc), "expected %s, got %s\n",
       wine_dbgstr_rect(&nc), wine_dbgstr_rect(&rect));

    /* multiple minimized children also stack; here the parent is too small to
     * fit more than one per row */
    OffsetRect(&expect, 0, -GetSystemMetrics(SM_CYMINIMIZED));
    ret = ShowWindow(hwnd2, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd2, &rect);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MAXIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should be minimized\n");
    ok(style & WS_MAXIMIZE, "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    GetClientRect(hwndMain, &expect);
    AdjustWindowRectEx(&expect, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BORDER,
                       0, GetWindowLongA(hwnd, GWL_EXSTYLE));
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* maximized windows can be resized */
    ret = SetWindowPos(hwnd, 0, 300, 300, 200, 200, SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    SetRect(&expect, 300, 300, 500, 500);
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
}

static void test_ShowWindow_mdichild(HWND hwndMain)
{
    RECT rect, orig, expect, nc;
    LPARAM ret;
    HWND mdiclient, hwnd, hwnd2;
    LONG style;
    POINT pt = {0};
    CLIENTCREATESTRUCT mdi_client_cs = {0,1};

    SetRect(&orig, 20, 20, 210, 110);
    GetClientRect(hwndMain, &rect);
    mdiclient = CreateWindowA("mdiclient", "MDI client", WS_CHILD,
                              rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                              hwndMain, 0, 0, &mdi_client_cs);
    ok(!!mdiclient, "failed to create window, error %lu\n", GetLastError());
    hwnd = CreateWindowExA(WS_EX_MDICHILD, "MainWindowClass", "MDI child",
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                           orig.left, orig.top, orig.right - orig.left,
                           orig.bottom - orig.top, mdiclient, 0, 0, NULL);
    ok(!!hwnd, "failed to create window, error %lu\n", GetLastError());
    hwnd2 = CreateWindowExA(WS_EX_MDICHILD, "MainWindowClass", "MDI child 2",
                            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                            orig.left, orig.top, orig.right - orig.left,
                            orig.bottom - orig.top, mdiclient, 0, 0, NULL);
    ok(!!hwnd2, "failed to create window, error %lu\n", GetLastError());

    ClientToScreen(hwndMain, &pt);
    OffsetRect(&orig, pt.x, pt.y);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should not be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    GetClientRect(hwndMain, &expect);
    SetRect(&expect, 0, expect.bottom - GetSystemMetrics(SM_CYMINIMIZED),
            GetSystemMetrics(SM_CXMINIMIZED), expect.bottom);
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* shouldn't be able to resize minimized windows */
    ret = SetWindowPos(hwnd, 0, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* test NC area */
    GetWindowRect(hwnd, &rect);
    SetRect(&nc, rect.left, rect.top, rect.left, rect.top);
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, 0, (LPARAM)&rect);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&rect, &nc), "expected %s, got %s\n",
       wine_dbgstr_rect(&nc), wine_dbgstr_rect(&rect));

    /* multiple minimized children also stack; here the parent is too small to
     * fit more than one per row */
    OffsetRect(&expect, 0, -GetSystemMetrics(SM_CYMINIMIZED));
    ret = ShowWindow(hwnd2, SW_MINIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd2, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(style & WS_MINIMIZE, "window should be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd2, &rect);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_MAXIMIZE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should be minimized\n");
    ok(style & WS_MAXIMIZE, "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    GetClientRect(hwndMain, &expect);
    AdjustWindowRectEx(&expect, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BORDER,
                       0, GetWindowLongA(hwnd, GWL_EXSTYLE));
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    /* maximized windows can be resized */
    ret = SetWindowPos(hwnd, 0, 300, 300, 200, 200, SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "wrong ret %Iu\n", ret);
    GetWindowRect(hwnd, &rect);
    SetRect(&expect, 300, 300, 500, 500);
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&expect, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "wrong ret %Iu\n", ret);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & WS_DISABLED), "window should not be disabled\n");
    ok(style & WS_VISIBLE, "window should be visible\n");
    ok(!(style & WS_MINIMIZE), "window should not be minimized\n");
    ok(!(style & WS_MAXIMIZE), "window should not be maximized\n");
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&orig, &rect), "expected %s, got %s\n",
       wine_dbgstr_rect(&orig), wine_dbgstr_rect(&rect));

    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
    DestroyWindow(mdiclient);
}

static DWORD CALLBACK enablewindow_thread(LPVOID arg)
{
    HWND hwnd = arg;
    EnableWindow(hwnd, FALSE);
    return 0;
}

static LRESULT CALLBACK enable_window_procA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CANCELMODE)
        return 0;
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void test_EnableWindow(void)
{
    WNDCLASSA cls;
    HWND hwnd;
    HANDLE hthread;
    DWORD tid;
    MSG msg;

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                           0, 0, 0, NULL);
    assert(hwnd);
    flush_events( TRUE );
    ok(IsWindowEnabled(hwnd), "window should be enabled\n");
    SetFocus(hwnd);
    SetCapture(hwnd);

    EnableWindow(hwnd, FALSE);
    check_wnd_state(hwnd, hwnd, 0, 0);
    ok(!IsWindowEnabled(hwnd), "window should not be enabled\n");

    SetFocus(hwnd);
    SetCapture(hwnd);
    check_wnd_state(hwnd, hwnd, 0, hwnd);

    EnableWindow(hwnd, TRUE);
    SetFocus(hwnd);
    check_wnd_state(hwnd, hwnd, hwnd, hwnd);
    ok(IsWindowEnabled(hwnd), "window should be enabled\n");

    /* test disabling from thread */
    hthread = CreateThread(NULL, 0, enablewindow_thread, hwnd, 0, &tid);

    while (MsgWaitForMultipleObjects(1, &hthread, FALSE, INFINITE, QS_SENDMESSAGE) != WAIT_OBJECT_0)
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE))
            DispatchMessageA(&msg);
    }

    ok(!IsWindowEnabled(hwnd), "window should not be enabled\n");
    check_active_state(hwnd, hwnd, hwnd);
    ok(0 == GetCapture(), "GetCapture() = %p\n", GetCapture());

    CloseHandle(hthread);
    DestroyWindow(hwnd);

    /* test preventing release of capture */
    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = enable_window_procA;
    cls.hInstance = GetModuleHandleA(0);
    cls.lpszClassName = "EnableWindowClass";
    ok(RegisterClassA(&cls), "RegisterClass failed\n");

    hwnd = CreateWindowExA(0, "EnableWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                           0, 0, 100, 100, 0, 0, 0, NULL);
    assert(hwnd);
    SetFocus(hwnd);
    SetCapture(hwnd);

    EnableWindow(hwnd, FALSE);
    check_wnd_state(hwnd, hwnd, 0, hwnd);

    DestroyWindow(hwnd);
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
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

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
    ok( num_settext_msgs == 1, "got %lu WM_SETTEXT messages\n", num_settext_msgs );

    return 0;
}

static void test_gettext(void)
{
    static const WCHAR textW[] = {'t','e','x','t'};
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
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    /* test GetWindowTextA */
    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

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
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    strcpy( buf, "blah" );
    g_wm_gettext_override.buff = buf;
    buf_len = GetWindowTextA( hwnd, buf, 0 );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( !strcmp(buf, "blah"), "got %s\n", buf );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    bufW[0] = 0xcc;
    buf_len = InternalGetWindowText( hwnd, bufW, ARRAYSIZE(bufW) );
    ok( buf_len == ARRAYSIZE("caption") - 1, "expected a nonempty window text\n" );
    ok( !lstrcmpW( bufW, L"caption" ), "got %s\n", debugstr_w(bufW) );

    g_wm_gettext_override.enabled = FALSE;

    /* same for W window */
    hwnd2 = CreateWindowExW( 0, mainclassW, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd2 != 0, "CreateWindowExA error %ld\n", GetLastError() );

    g_wm_gettext_override.enabled = TRUE;

    num_gettext_msgs = 0;
    memset( bufW, 0xcc, sizeof(bufW) );
    g_wm_gettext_override.buffW = bufW;
    buf_len = GetWindowTextW( hwnd2, bufW, ARRAY_SIZE(bufW));
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( *bufW == 0, "got %x\n", *bufW );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    memset( bufW, 0xcc, sizeof(bufW) );
    g_wm_gettext_override.buffW = bufW;
    buf_len = GetWindowTextW( hwnd2, bufW, 0 );
    ok( buf_len == 0, "got %d\n", buf_len );
    ok( *bufW == 0xcccc, "got %x\n", *bufW );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    g_wm_gettext_override.enabled = FALSE;

    DestroyWindow( hwnd2 );

    /* test WM_GETTEXT */
    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    r = SendMessageA( hwnd, WM_GETTEXT, sizeof(buf), (LONG_PTR)buf );
    ok( r != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    /* test SetWindowTextA */
    num_settext_msgs = 0;
    success = SetWindowTextA( hwnd, "new_caption" );
    ok( success, "SetWindowTextA failed\n" );
    ok( num_settext_msgs == 1, "got %lu WM_SETTEXT messages\n", num_settext_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "new_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    /* test WM_SETTEXT */
    num_settext_msgs = 0;
    r = SendMessageA( hwnd, WM_SETTEXT, 0, (ULONG_PTR)"another_caption" );
    ok( r != 0, "WM_SETTEXT failed\n" );
    ok( num_settext_msgs == 1, "got %lu WM_SETTEXT messages\n", num_settext_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "another_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
        DispatchMessageA( &msg );

    /* test interthread GetWindowTextA */
    num_msgs = 0;
    thread = CreateThread( NULL, 0, gettext_msg_thread, hwnd, 0, &tid );
    ok(thread != NULL, "CreateThread failed, error %ld\n", GetLastError());
    while (MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, QS_SENDMESSAGE ) != WAIT_OBJECT_0)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
            DispatchMessageA( &msg );
        num_msgs++;
    }
    CloseHandle( thread );
    ok( num_msgs >= 1, "got %lu wakeups from MsgWaitForMultipleObjects\n", num_msgs );

    /* test interthread SetWindowText */
    num_msgs = 0;
    thread = CreateThread( NULL, 0, settext_msg_thread, hwnd, 0, &tid );
    ok(thread != NULL, "CreateThread failed, error %ld\n", GetLastError());
    while (MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, QS_SENDMESSAGE ) != WAIT_OBJECT_0)
    {
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE ))
            DispatchMessageA( &msg );
        num_msgs++;
    }
    CloseHandle( thread );
    ok( num_msgs >= 1, "got %lu wakeups from MsgWaitForMultipleObjects\n", num_msgs );

    num_gettext_msgs = 0;
    memset( buf, 0, sizeof(buf) );
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len != 0, "expected a nonempty window text\n" );
    ok( !strcmp(buf, "thread_caption"), "got wrong window text '%s'\n", buf );
    ok( num_gettext_msgs == 1, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    /* WM_GETTEXT does not terminate returned string */
    memset( buf, 0x1c, sizeof(buf) );
    g_wm_gettext_override.dont_terminate = TRUE;
    buf_len = GetWindowTextA( hwnd, buf, sizeof(buf) );
    ok( buf_len == 4, "Unexpected text length, %d\n", buf_len );
    ok( !memcmp(buf, "text", 4), "Unexpected window text, '%s'\n", buf );
    ok( buf[4] == 0x1c, "Unexpected buffer contents\n" );
    g_wm_gettext_override.dont_terminate = FALSE;

    memset( bufW, 0x1c, sizeof(bufW) );
    g_wm_gettext_override.dont_terminate = TRUE;
    buf_len = GetWindowTextW( hwnd, bufW, ARRAY_SIZE(bufW));
    ok( buf_len == 4, "Unexpected text length, %d\n", buf_len );
    ok( !memcmp(bufW, textW, 4 * sizeof(WCHAR)), "Unexpected window text, %s\n", wine_dbgstr_w(bufW) );
    ok( bufW[4] == 0, "Unexpected buffer contents, %#x\n", bufW[4] );
    g_wm_gettext_override.dont_terminate = FALSE;

    hwnd2 = CreateWindowExW( 0, mainclassW, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd2 != 0, "CreateWindowExA error %ld\n", GetLastError() );

    memset( buf, 0x1c, sizeof(buf) );
    g_wm_gettext_override.dont_terminate = TRUE;
    buf_len = GetWindowTextA( hwnd2, buf, sizeof(buf) );
    ok( buf_len == 4, "Unexpected text length, %d\n", buf_len );
    ok( !memcmp(buf, "text", 4), "Unexpected window text, '%s'\n", buf );
    ok( buf[4] == 0, "Unexpected buffer contents, %#x\n", buf[4] );
    g_wm_gettext_override.dont_terminate = FALSE;

    memset( bufW, 0x1c, sizeof(bufW) );
    g_wm_gettext_override.dont_terminate = TRUE;
    buf_len = GetWindowTextW( hwnd2, bufW, ARRAY_SIZE(bufW));
    ok( buf_len == 4, "Unexpected text length, %d\n", buf_len );
    ok( !memcmp(bufW, textW, 4 * sizeof(WCHAR)), "Unexpected window text, %s\n", wine_dbgstr_w(bufW) );
    ok( bufW[4] == 0x1c1c, "Unexpected buffer contents, %#x\n", bufW[4] );
    g_wm_gettext_override.dont_terminate = FALSE;

    DestroyWindow(hwnd2);

    /* seems to crash on every modern Windows version */
    if (0)
    {
    r = SendMessageA( hwnd, WM_GETTEXT, 0x10, 0x1000);
    ok( r == 0, "WM_GETTEXT should return zero (%Id)\n", r );

    r = SendMessageA( hwnd, WM_GETTEXT, 0x10000, 0);
    ok( r == 0, "WM_GETTEXT should return zero (%Id)\n", r );

    r = SendMessageA( hwnd, WM_GETTEXT, 0xff000000, 0x1000);
    ok( r == 0, "WM_GETTEXT should return zero (%Id)\n", r );

    r = SendMessageA( hwnd, WM_GETTEXT, 0x1000, 0xff000000);
    ok( r == 0, "WM_GETTEXT should return zero (%Id)\n", r );
    }

    /* GetWindowText doesn't crash */
    r = GetWindowTextA( hwnd, (LPSTR)0x10, 0x1000 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextA( hwnd, (LPSTR)0x10000, 0 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextA( hwnd, (LPSTR)0xff000000, 0x1000 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextA( hwnd, (LPSTR)0x1000, 0xff000000 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );

    r = GetWindowTextW( hwnd, (LPWSTR)0x10, 0x1000 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextW( hwnd, (LPWSTR)0x10000, 0 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextW( hwnd, (LPWSTR)0xff000000, 0x1000 );
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );
    r = GetWindowTextW( hwnd, (LPWSTR)0x1000, 0xff000000);
    ok( r == 0, "GetWindowText should return zero (%Id)\n", r );

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
    ret = RegisterClassA(&cls);
    ok(ret, "Failed to register a test class.\n");

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

    flush_events( TRUE );
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
    ok(hinst == 0, "expected 0, got %p\n", hinst);

    buf1[0] = 0;
    SetLastError(0xdeadbeef);
    ret1 = GetModuleFileNameA(hinst, buf1, sizeof(buf1));
    ok(ret1, "GetModuleFileName error %lu\n", GetLastError());

    buf2[0] = 0;
    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, sizeof(buf2));
    ok(ret2, "GetWindowModuleFileNameA error %lu\n", GetLastError());

    if (ret2)
    {
        ok(ret1 == ret2, "%u != %u\n", ret1, ret2);
        ok(!strcmp(buf1, buf2), "%s != %s\n", buf1, buf2);
    }
    hinst = GetModuleHandleA(NULL);

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileNameA(hinst, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2, "expected %u, got %u\n", ret1 - 2, ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = GetModuleFileNameA(hinst, buf2, 0);
    ok(!ret2, "GetModuleFileName should return 0\n");
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, ret1 - 2);
    ok(ret2 == ret1 - 2, "expected %u, got %u\n", ret1 - 2, ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret2 = pGetWindowModuleFileNameA(hwnd, buf2, 0);
    ok(!ret2, "expected 0, got %u\n", ret2);
    ok(GetLastError() == 0xdeadbeef /* XP */ ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER, /* win2k3, vista */
       "expected 0xdeadbeef or ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    DestroyWindow(hwnd);

    hwnd = (HWND)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret1 = pGetWindowModuleFileNameA(hwnd, buf1, sizeof(buf1));
    ok(!ret1, "expected 0, got %u\n", ret1);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
       "expected ERROR_INVALID_WINDOW_HANDLE, got %lu\n", GetLastError());
}

static void test_hwnd_message(void)
{
    static const WCHAR mainwindowclassW[] = {'M','a','i','n','W','i','n','d','o','w','C','l','a','s','s',0};
    static const WCHAR message_windowW[] = {'m','e','s','s','a','g','e',' ','w','i','n','d','o','w',0};

    HWND parent = 0, hwnd, found;
    RECT rect;
    static const struct
    {
        int offset;
        ULONG_PTR expect;
        DWORD error;
    }
    tests[] =
    {
        { GWLP_USERDATA,   0, 0 },
        { GWL_STYLE,       WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0 },
        { GWL_EXSTYLE,     0, 0 },
        { GWLP_ID,         0, 0 },
        /* GWLP_HWNDPARENT - returns random values */
        /* GWLP_HINSTANCE  - not useful and not consistent between Windows versions */
        { GWLP_WNDPROC,    0, ERROR_ACCESS_DENIED },
        { DWLP_MSGRESULT,  0, ERROR_INVALID_INDEX }
    };
    HWND root, desktop = GetDesktopWindow();
    DWORD_PTR result;
    char buffer[100];
    int i;

    hwnd = CreateWindowExW(0, mainwindowclassW, message_windowW, WS_CAPTION | WS_VISIBLE,
                           100, 100, 200, 200, HWND_MESSAGE, 0, 0, NULL);
    ok( hwnd != 0, "CreateWindowExW with parent HWND_MESSAGE failed\n" );
    ok( !GetParent(hwnd), "GetParent should return 0 for message only windows\n" );

    parent = GetAncestor(hwnd, GA_PARENT);
    ok(parent != 0, "GetAncestor(GA_PARENT) should not return 0 for message windows\n");
    ok(parent != desktop, "GetAncestor(GA_PARENT) should not return desktop for message windows\n");
    root = GetAncestor(hwnd, GA_ROOT);
    ok(root == hwnd, "GetAncestor(GA_ROOT) should return hwnd for message windows\n");
    ok( !GetAncestor(parent, GA_PARENT),
        "parent shouldn't have parent %p\n", GetAncestor(parent, GA_PARENT) );
    if (!GetClassNameA( parent, buffer, sizeof(buffer) )) buffer[0] = 0;
    ok( !lstrcmpiA( buffer, "Message" ), "wrong parent class '%s'\n", buffer );
    GetWindowRect( parent, &rect );
    ok( rect.left == 0 && rect.right == 100 && rect.top == 0 && rect.bottom == 100,
        "wrong parent rect %s\n", wine_dbgstr_rect( &rect ));
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 100 && rect.right == 300 && rect.top == 100 && rect.bottom == 300,
        "wrong window rect %s\n", wine_dbgstr_rect( &rect ));

    /* test FindWindow behavior */

    found = FindWindowExA( 0, 0, 0, "message window" );
    ok( found == hwnd, "didn't find message window %p/%p\n", found, hwnd );
    SetLastError(0xdeadbeef);
    found = FindWindowExA( GetDesktopWindow(), 0, 0, "message window" );
    ok( found == 0, "found message window %p/%p\n", found, hwnd );
    ok( GetLastError() == 0xdeadbeef, "expected deadbeef, got %ld\n", GetLastError() );
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

    /* GetWindowLong */
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError( 0xdeadbeef );
        result = GetWindowLongPtrW( parent, tests[i].offset );
        ok( result == tests[i].expect, "offset %d, got %08Ix expect %08Ix\n",
            tests[i].offset, result, tests[i].expect );
        if (tests[i].error)
            ok( GetLastError() == tests[i].error, "offset %d: error %ld expect %ld\n",
                tests[i].offset, GetLastError(), tests[i].error );
        else
            ok( GetLastError() == 0xdeadbeef, "offset %d: error %ld expect unchanged\n",
                tests[i].offset, GetLastError() );
    }

    DestroyWindow(hwnd);
}

static HWND message_window_topmost_hwnd_msg = NULL;
static BOOL message_window_topmost_received_killfocus;

static LRESULT WINAPI message_window_topmost_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ok(hwnd != message_window_topmost_hwnd_msg, "Received message %u for message-only window %p\n", msg, hwnd);
    if (msg == WM_KILLFOCUS) message_window_topmost_received_killfocus = TRUE;
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}


static void test_message_window_topmost(void)
{
    /* All SWP_* flags except SWP_NOZORDER, which has a different effect. */
    const UINT swp_flags = SWP_ASYNCWINDOWPOS | SWP_DEFERERASE | SWP_DRAWFRAME | SWP_FRAMECHANGED
            | SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER
            | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_SHOWWINDOW;
    /* Same as above, except the flags that cause
     * ERROR_INVALID_PARAMETER to be returned by DeferWindowPos(). */
    const UINT dwp_flags = SWP_DRAWFRAME | SWP_FRAMECHANGED
            | SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER
            | SWP_NOREDRAW | SWP_NOSIZE | SWP_SHOWWINDOW;
    HWND hwnd, hwnd_msg;
    HDWP hdwp;
    RECT rect;
    BOOL ret;
    MSG msg;

    hwnd = CreateWindowW(L"static", L"main window", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            210, 211, 212, 213, NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(!!hwnd, "Cannot create main window\n");
    flush_events(TRUE);
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)message_window_topmost_proc);

    hwnd_msg = CreateWindowW(L"static", L"message window", 0,
            220, 221, 222, 223, HWND_MESSAGE, NULL, GetModuleHandleW(NULL), NULL);
    ok(!!hwnd_msg, "Cannot create message window\n");
    flush_events(TRUE);
    SetWindowLongPtrW(hwnd_msg, GWLP_WNDPROC, (LONG_PTR)message_window_topmost_proc);

    ret = GetWindowRect(hwnd_msg, &rect);
    ok(ret, "Unexpected failure when calling GetWindowRect()\n");
    ok(rect.left == 220 && rect.top == 221 && rect.right == 220 + 222 && rect.bottom == 221 + 223,
            "Unexpected rectangle %s\n", wine_dbgstr_rect(&rect));

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    message_window_topmost_hwnd_msg = hwnd_msg;

    SetLastError(0xdeadbeef);

    ret = SetWindowPos(hwnd_msg, HWND_TOPMOST, 230, 231, 232, 233, 0);
    ok(ret, "Unexpected failure when calling SetWindowPos()\n");

    ret = SetWindowPos(hwnd_msg, HWND_TOPMOST, 234, 235, 236, 237, swp_flags);
    ok(ret, "Unexpected failure when calling SetWindowPos()\n");

    ret = SetWindowPos(hwnd_msg, HWND_NOTOPMOST, 240, 241, 242, 243, 0);
    ok(ret, "Unexpected failure when calling SetWindowPos()\n");

    ret = SetWindowPos(hwnd_msg, HWND_NOTOPMOST, 244, 245, 246, 247, swp_flags);
    ok(ret, "Unexpected failure when calling SetWindowPos()\n");

    hdwp = BeginDeferWindowPos(4);
    ok(!!hdwp, "Unexpected failure when calling BeginDeferWindowPos()\n");

    hdwp = DeferWindowPos(hdwp, hwnd_msg, HWND_TOPMOST, 250, 251, 252, 253, 0);
    ok(!!hdwp, "Unexpected failure when calling DeferWindowPos()\n");

    hdwp = DeferWindowPos(hdwp, hwnd_msg, HWND_TOPMOST, 254, 255, 256, 257, dwp_flags);
    ok(!!hdwp, "Unexpected failure when calling DeferWindowPos()\n");

    hdwp = DeferWindowPos(hdwp, hwnd_msg, HWND_NOTOPMOST, 260, 261, 262, 263, 0);
    ok(!!hdwp, "Unexpected failure when calling DeferWindowPos()\n");

    hdwp = DeferWindowPos(hdwp, hwnd_msg, HWND_NOTOPMOST, 264, 265, 266, 267, dwp_flags);
    ok(!!hdwp, "Unexpected failure when calling DeferWindowPos()\n");

    ret = EndDeferWindowPos(hdwp);
    ok(ret, "Unexpected failure when calling EndDeferWindowPos()\n");

    ok(GetLastError() == 0xdeadbeef, "Last error unexpectedly set to %#lx\n", GetLastError());

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    message_window_topmost_hwnd_msg = NULL;

    ret = GetWindowRect(hwnd_msg, &rect);
    ok(ret, "Unexpected failure when calling GetWindowRect()\n");
    ok(rect.left == 220 && rect.top == 221 && rect.right == 220 + 222 && rect.bottom == 221 + 223,
            "Unexpected rectangle %s\n", wine_dbgstr_rect(&rect));

    ok(!message_window_topmost_received_killfocus, "Received WM_KILLFOCUS\n");
    message_window_topmost_received_killfocus = FALSE;

    ret = SetWindowPos(hwnd_msg, HWND_TOPMOST, 230, 231, 232, 233, SWP_NOZORDER);
    ok(ret, "Unexpected failure when calling SetWindowPos()\n");

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ok(message_window_topmost_received_killfocus, "Did not receive WM_KILLFOCUS\n");

    ret = GetWindowRect(hwnd_msg, &rect);
    ok(ret, "Unexpected failure when calling GetWindowRect()\n");
    ok(rect.left == 230 && rect.top == 231 && rect.right == 230 + 232 && rect.bottom == 231 + 233,
            "Unexpected rectangle %s\n", wine_dbgstr_rect(&rect));

    ok(DestroyWindow(hwnd_msg), "Cannot destroy main window\n");
    ok(DestroyWindow(hwnd), "Cannot destroy message window\n");
}


static void test_layered_window(void)
{
    HWND hwnd, child;
    COLORREF key = 0;
    BYTE alpha = 0;
    DWORD flags = 0;
    POINT pt = { 0, 0 };
    SIZE sz = { 200, 200 };
    HDC hdc;
    HBITMAP hbm;
    BOOL ret;
    MSG msg;

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
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError() );
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
    ok( key == 0x123456 || key == 0, "wrong color key %lx\n", key );
    ok( alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == LWA_ALPHA, "wrong flags %lx\n", flags );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on layered but initialized window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError() );

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
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
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
        ok( GetLastError() != 0, "wrong error %lu\n", GetLastError() );
        info.dwFlags  = ULW_OPAQUE;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( ret, "UpdateLayeredWindowIndirect should succeed on layered window\n" );
        sz.cx++;
        info.dwFlags  = ULW_OPAQUE | 0xf00;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        info.cbSize--;
        info.dwFlags  = ULW_OPAQUE;
        ret = pUpdateLayeredWindowIndirect( hwnd, &info );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        ret = pUpdateLayeredWindowIndirect( hwnd, NULL );
        ok( !ret, "UpdateLayeredWindowIndirect should fail\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }

    ret = pSetLayeredWindowAttributes( hwnd, 0x654321, 22, LWA_COLORKEY | LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x654321, "wrong color key %lx\n", key );
    ok( alpha == 22, "wrong alpha %u\n", alpha );
    ok( flags == (LWA_COLORKEY | LWA_ALPHA), "wrong flags %lx\n", flags );
    SetLastError( 0xdeadbeef );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on layered but initialized window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError() );

    ret = pSetLayeredWindowAttributes( hwnd, 0x888888, 33, LWA_COLORKEY );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x888888, "wrong color key %lx\n", key );
    /* alpha not changed on vista if LWA_ALPHA is not set */
    ok( alpha == 22 || alpha == 33, "wrong alpha %u\n", alpha );
    ok( flags == LWA_COLORKEY, "wrong flags %lx\n", flags );

    /* color key may or may not be changed without LWA_COLORKEY */
    ret = pSetLayeredWindowAttributes( hwnd, 0x999999, 44, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    alpha = 0;
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0x888888 || key == 0x999999, "wrong color key %lx\n", key );
    ok( alpha == 22 || alpha == 44, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %lx\n", flags );

    /* default alpha and color key is 0 */
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pSetLayeredWindowAttributes( hwnd, 0x222222, 55, 0 );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    ret = pGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags );
    ok( ret, "GetLayeredWindowAttributes should succeed on layered window\n" );
    ok( key == 0 || key == 0x222222, "wrong color key %lx\n", key );
    ok( alpha == 0 || alpha == 55, "wrong alpha %u\n", alpha );
    ok( flags == 0, "wrong flags %lx\n", flags );

    /* test layered window with WS_CLIPCHILDREN flag */
    SetWindowLongA( hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) | WS_CLIPCHILDREN );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    child = CreateWindowExA( 0, "button", "button", WS_VISIBLE | WS_CHILD,
            0, 0, 50, 50, hwnd, 0, 0, NULL );
    ok( child != NULL, "CreateWindowEx error %lu\n", GetLastError() );
    ShowWindow( hwnd, SW_SHOW );

    ret = pSetLayeredWindowAttributes( hwnd, 0, 255, LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    while (GetMessageA(&msg, 0, 0, 0))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_PAINT && msg.hwnd == child)
            break;
    }

    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ret = pUpdateLayeredWindow( hwnd, 0, NULL, &sz, hdc, &pt, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow should succeed on layered window\n" );

    ret = pSetLayeredWindowAttributes( hwnd, 0, 255, LWA_ALPHA );
    ok( ret, "SetLayeredWindowAttributes should succeed on layered window\n" );
    while (GetMessageA(&msg, 0, 0, 0))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_PAINT && msg.hwnd == child)
            break;
    }

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
            ok(minmax->ptMaxPosition.x <= mi.rcMonitor.left, "%ld <= %ld\n", minmax->ptMaxPosition.x, mi.rcMonitor.left);
            ok(minmax->ptMaxPosition.y <= mi.rcMonitor.top, "%ld <= %ld\n", minmax->ptMaxPosition.y, mi.rcMonitor.top);
            ok(minmax->ptMaxSize.x >= mi.rcMonitor.right, "%ld >= %ld\n", minmax->ptMaxSize.x, mi.rcMonitor.right);
            ok(minmax->ptMaxSize.y >= mi.rcMonitor.bottom, "%ld >= %ld\n", minmax->ptMaxSize.y, mi.rcMonitor.bottom);
            break;
        }
        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wp, lp);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wp, lp);
            return ret;
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
    RECT rc, virtual_rect, expected_rect;
    WNDCLASSA cls;
    int timeout;
    HWND hwnd;
    int i, j;
    POINT pt;
    HMONITOR hmon;
    LRESULT ret;

    pt.x = pt.y = 0;
    SetLastError(0xdeadbeef);
    hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    ok(hmon != 0, "MonitorFromPoint error %lu\n", GetLastError());

    mi.cbSize = sizeof(mi);
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo error %lu\n", GetLastError());
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

    for (i = 0; i < ARRAY_SIZE(t_style); i++)
    {
        DWORD style, ex_style;

        /* avoid a WM interaction */
        assert(!(t_style[i] & WS_VISIBLE));

        for (j = 0; j < ARRAY_SIZE(t_ex_style); j++)
        {
            int fixup;

            style = t_style[i];
            ex_style = t_ex_style[j];

            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
               rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
               "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_CAPTION | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            ok(rc.left >= mi.rcMonitor.left && rc.top >= mi.rcMonitor.top &&
               rc.right <= mi.rcMonitor.right && rc.bottom <= mi.rcMonitor.bottom,
               "%#lx/%#lx: window rect %s must be in %s\n", ex_style, style, wine_dbgstr_rect(&rc),
               wine_dbgstr_rect(&mi.rcMonitor));
            DestroyWindow(hwnd);

            style = t_style[i] | WS_MAXIMIZE | WS_MAXIMIZEBOX;
            hwnd = CreateWindowExA(ex_style, "fullscreen_class", NULL, style,
                                   mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
                                   GetDesktopWindow(), 0, GetModuleHandleA(NULL), NULL);
            ok(hwnd != 0, "%d: CreateWindowExA(%#lx/%#lx) failed\n", i, ex_style, style);
            GetWindowRect(hwnd, &rc);
            /* Windows makes a maximized window slightly larger (to hide the borders?) */
            fixup = min(abs(rc.left), abs(rc.top));
            InflateRect(&rc, -fixup, -fixup);
            if (style & (WS_CHILD | WS_POPUP))
                ok(rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
                   rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom,
                   "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            else
                ok(rc.left >= mi.rcMonitor.left && rc.top >= mi.rcMonitor.top &&
                   rc.right <= mi.rcMonitor.right && rc.bottom <= mi.rcMonitor.bottom,
                   "%#lx/%#lx: window rect %s\n", ex_style, style, wine_dbgstr_rect(&rc));
            DestroyWindow(hwnd);
        }
    }

    /* Test restoring a full screen window with WS_THICKFRAME style to normal */
    /* Add WS_THICKFRAME style later so that the window can cover the entire monitor */
    hwnd = CreateWindowA("fullscreen_class", NULL, WS_POPUP | WS_VISIBLE, 0, 0, mi.rcMonitor.right,
                         mi.rcMonitor.bottom, NULL, NULL, GetModuleHandleA(NULL), NULL);
    ok(!!hwnd, "CreateWindow failed, error %#lx.\n", GetLastError());
    flush_events(TRUE);

    /* Add WS_THICKFRAME and exit full screen */
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) | WS_THICKFRAME);
    SetWindowPos(hwnd, 0, 0, 0, 100, 100, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    flush_events(TRUE);

    /* TestBots need about 1000ms to exit full screen */
    timeout = 1000;
    while (timeout > 0)
    {
        timeout -= 200;
        flush_events(TRUE);
        GetWindowRect(hwnd, &rc);
        if (rc.right - rc.left == 100 && rc.bottom - rc.top == 100)
            break;
    }
    ok(rc.right - rc.left == 100, "Expect width %d, got %ld.\n", 100, rc.right - rc.left);
    ok(rc.bottom - rc.top == 100, "Expect height %d, got %ld.\n", 100, rc.bottom - rc.top);
    DestroyWindow(hwnd);

    UnregisterClassA("fullscreen_class", GetModuleHandleA(NULL));

    /* Test fullscreen windows spanning multiple monitors */
    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        /* Test windows covering all monitors */
        virtual_rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
        virtual_rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
        virtual_rect.right = virtual_rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
        virtual_rect.bottom = virtual_rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

        hwnd = CreateWindowA("static", NULL, WS_POPUP | WS_VISIBLE, virtual_rect.left,
                             virtual_rect.top, virtual_rect.right - virtual_rect.left,
                             virtual_rect.bottom - virtual_rect.top, NULL, NULL, NULL, NULL);
        ok(!!hwnd, "CreateWindow failed, error %#lx.\n", GetLastError());
        flush_events(TRUE);

        GetWindowRect(hwnd, &rc);
        /* FVWM used by TestBots doesn't support _NET_WM_FULLSCREEN_MONITORS */
        todo_wine_if(!EqualRect(&rc, &virtual_rect))
        ok(EqualRect(&rc, &virtual_rect), "Expected %s, got %s.\n",
           wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&rc));
        DestroyWindow(hwnd);

        /* Test windows covering one monitor and 1 pixel larger on available sides */
        expected_rect = mi.rcMonitor;
        InflateRect(&expected_rect, 1, 1);
        IntersectRect(&expected_rect, &expected_rect, &virtual_rect);
        hwnd = CreateWindowA("static", NULL, WS_POPUP | WS_VISIBLE, expected_rect.left,
                             expected_rect.top, expected_rect.right - expected_rect.left,
                             expected_rect.bottom - expected_rect.top, NULL, NULL, NULL, NULL);
        ok(!!hwnd, "CreateWindow failed, error %#lx.\n", GetLastError());
        flush_events(TRUE);

        GetWindowRect(hwnd, &rc);
        todo_wine
        ok(EqualRect(&rc, &expected_rect), "Expected %s, got %s.\n",
           wine_dbgstr_rect(&expected_rect), wine_dbgstr_rect(&rc));
        DestroyWindow(hwnd);
    }
    else
    {
        skip("This test requires at least two monitors.\n");
    }
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
            if (winetest_debug > 1)
            {
                trace("hwnd %p, WM_GETMINMAXINFO, %08Ix, %08Ix\n", hwnd, wparam, lparam);
                dump_minmax_info( minmax );
            }
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

        case WM_NCCALCSIZE:
        {
            LRESULT ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, wparam, lparam);
            ok(!ret, "got %08Ix (%08Ix %08Ix)\n", ret, wparam, lparam);
            return ret;
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
    ok(success,"RegisterClassA failed, error: %lu\n", GetLastError());

    for(i = 0; i < NUMBER_OF_THICK_CHILD_TESTS; i++)
    {
        test_thick_child_name = styleName[i];
        test_thick_child_style = styles[i];
        test_thick_child_exStyle = exStyles[i];
        test_thick_child_got_minmax = FALSE;

        SetLastError(0xdeadbeef);
        childWindow = CreateWindowExA( exStyles[i], className, "", styles[i],  0, 0, 0, 0, parentWindow, 0,  GetModuleHandleA(0),  NULL );
        ok(childWindow != NULL, "Failed to create child window, error: %lu\n", GetLastError());

        ok(test_thick_child_got_minmax, "Got no WM_GETMINMAXINFO\n");

        SetLastError(0xdeadbeef);
        success = GetWindowRect(childWindow, &childRect);
        ok(success,"GetWindowRect call failed, error: %lu\n", GetLastError());
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
            "size of window (%s) is wrong: expected size %ldx%ld != actual size %ldx%ld\n",
            test_thick_child_name, expectedWidth, expectedHeight, childWidth, childHeight);

        SetLastError(0xdeadbeef);
        success = DestroyWindow(childWindow);
        ok(success,"DestroyWindow call failed, error: %lu\n", GetLastError());
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
    ok( ret, "GetWindowRect failed for %p err %lu\n", hwnd, GetLastError() );

#ifdef _WIN64
    if ((ULONG_PTR)full_hwnd >> 32)
        hwnd = (HWND)((ULONG_PTR)full_hwnd & ~0u);
    else
        hwnd = (HWND)((ULONG_PTR)full_hwnd | ((ULONG_PTR)~0u << 32));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( ret, "GetWindowRect failed for %p err %lu\n", hwnd, GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & ~0u) | ((ULONG_PTR)0x1234 << 32));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( ret, "GetWindowRect failed for %p err %lu\n", hwnd, GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & 0xffff) | ((ULONG_PTR)0x9876 << 16));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( !ret, "GetWindowRect succeeded for %p\n", hwnd );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );

    hwnd = (HWND)(((ULONG_PTR)full_hwnd & 0xffff) | ((ULONG_PTR)0x12345678 << 16));
    SetLastError( 0xdeadbeef );
    ret = GetWindowRect( hwnd, &rect );
    ok( !ret, "GetWindowRect succeeded for %p\n", hwnd );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
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
    ok( GetLastError() == 0xdeadbeef, "Expected , got %ld\n", GetLastError());

    hrgn = CreateRectRgn(2, 3, 10, 15);
    ok( hrgn != NULL, "Region creation failed\n");
    if (hrgn)
    {
        SetWindowRgn(hwnd, hrgn, FALSE);

        SetLastError(0xdeadbeef);
        ret = pGetWindowRgnBox(hwnd, NULL);
        ok( ret == ERROR, "Expected ERROR, got %d\n", ret);
        ok( GetLastError() == 0xdeadbeef, "Expected , got %ld\n", GetLastError());

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
    ok( pt.x == 22 && pt.y == 22, "wrong point %ld,%ld\n", pt.x, pt.y );
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
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pFlashWindow( NULL, TRUE );
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                 GetLastError() == ERROR_INVALID_WINDOW_HANDLE),
        "FlashWindow returned with %ld\n", GetLastError() );

    DestroyWindow( hwnd );

    SetLastError( 0xdeadbeef );
    ret = pFlashWindow( hwnd, TRUE );
    ok( !ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                 GetLastError() == ERROR_INVALID_WINDOW_HANDLE),
        "FlashWindow returned with %ld\n", GetLastError() );
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
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    finfo.cbSize = sizeof(FLASHWINFO);
    finfo.dwFlags = FLASHW_TIMER;
    finfo.uCount = 3;
    finfo.dwTimeout = 200;
    finfo.hwnd = NULL;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(&finfo);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_INVALID_WINDOW_HANDLE),
       "FlashWindowEx returned with %ld\n", GetLastError());

    finfo.hwnd = hwnd;
    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(NULL);
    ok(!ret && GetLastError() == ERROR_NOACCESS,
       "FlashWindowEx returned with %ld\n", GetLastError());

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
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_INVALID_WINDOW_HANDLE),
       "FlashWindowEx returned with %ld\n", GetLastError());

    ok(finfo.cbSize == sizeof(FLASHWINFO), "FlashWindowEx modified cdSize to %x\n", finfo.cbSize);
    ok(finfo.hwnd == hwnd, "FlashWindowEx modified hwnd to %p\n", finfo.hwnd);
    ok(finfo.dwFlags == FLASHW_TIMER, "FlashWindowEx modified dwFlags to %lx\n", finfo.dwFlags);
    ok(finfo.uCount == 3, "FlashWindowEx modified uCount to %x\n", finfo.uCount);
    ok(finfo.dwTimeout == 200, "FlashWindowEx modified dwTimeout to %lx\n", finfo.dwTimeout);

    hwnd = CreateWindowExA( 0, "MainWindowClass", "FlashWindow", WS_VISIBLE | WS_POPUPWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );
    finfo.hwnd = hwnd;

    SetLastError(0xdeadbeef);
    ret = pFlashWindowEx(NULL);
    ok(!ret && GetLastError() == ERROR_NOACCESS,
       "FlashWindowEx returned with %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    prev = pFlashWindowEx(&finfo);

    ok(finfo.cbSize == sizeof(FLASHWINFO), "FlashWindowEx modified cdSize to %x\n", finfo.cbSize);
    ok(finfo.hwnd == hwnd, "FlashWindowEx modified hwnd to %p\n", finfo.hwnd);
    ok(finfo.dwFlags == FLASHW_TIMER, "FlashWindowEx modified dwFlags to %lx\n", finfo.dwFlags);
    ok(finfo.uCount == 3, "FlashWindowEx modified uCount to %x\n", finfo.uCount);
    ok(finfo.dwTimeout == 200, "FlashWindowEx modified dwTimeout to %lx\n", finfo.dwTimeout);

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
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "ClassThatDoesntExist", "" );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "ClassThatDoesntExist", NULL );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "" );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "caption" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    hwnd = CreateWindowExA( 0, "MainWindowClass", NULL, WS_POPUP, 0,0,0,0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", "" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowExA( 0, 0, "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    /* test behaviour with a window title that is an empty character */
    found = FindWindowExA( 0, 0, "Shell_TrayWnd", "" );
    ok( found != NULL, "found is NULL, expected a valid hwnd\n" );
    found = FindWindowExA( 0, 0, "Shell_TrayWnd", NULL );
    ok( found != NULL, "found is NULL, expected a valid hwnd\n" );
}

static void test_FindWindow(void)
{
    HWND hwnd, found;

    hwnd = CreateWindowExA( 0, "MainWindowClass", "caption", WS_POPUP, 0,0,0,0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowA( "ClassThatDoesntExist", "" );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowA( "ClassThatDoesntExist", NULL );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowA( "MainWindowClass", "" );
    ok( found == NULL, "expected a NULL hwnd\n" );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowA( "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowA( "MainWindowClass", "caption" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    hwnd = CreateWindowExA( 0, "MainWindowClass", NULL, WS_POPUP, 0,0,0,0, 0, 0, 0, NULL );
    ok( hwnd != 0, "CreateWindowExA error %ld\n", GetLastError() );

    num_gettext_msgs = 0;
    found = FindWindowA( "MainWindowClass", "" );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    num_gettext_msgs = 0;
    found = FindWindowA( "MainWindowClass", NULL );
    ok( found == hwnd, "found is %p, expected a valid hwnd\n", found );
    ok( num_gettext_msgs == 0, "got %lu WM_GETTEXT messages\n", num_gettext_msgs );

    DestroyWindow( hwnd );

    /* test behaviour with a window title that is an empty character */
    found = FindWindowA( "Shell_TrayWnd", "" );
    ok( found != NULL, "found is NULL, expected a valid hwnd\n" );
    found = FindWindowA( "Shell_TrayWnd", NULL );
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
    for (i = 0; i < ARRAY_SIZE(style); i++)
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

    create_window_tree(parent, window, ARRAY_SIZE(window));

    found_invisible = 0;
    found_disabled = 0;
    found_groupbox = 0;
    found_httransparent = 0;
    found_extransparent = 0;

    /* FIXME: also test WindowFromPoint, ChildWindowFromPoint, ChildWindowFromPointEx */
    for (i = 0; i < ARRAY_SIZE(real_child_pos); i++)
    {
        struct window_attributes attrs;

        pt.x = pt.y = 50;
        hwnd = RealChildWindowFromPoint(parent, pt);
        ok(hwnd != 0, "RealChildWindowFromPoint failed\n");
        ret = window_to_index(hwnd, window, ARRAY_SIZE(window));
        /* FIXME: remove once Wine is fixed */
        todo_wine_if (ret != real_child_pos[i])
            ok(ret == real_child_pos[i], "expected %d, got %d\n", real_child_pos[i], ret);

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
    POINT pt;

    GetCursorPos(&pt);
    SetCursorPos(x, y);
    memset(input, 0, sizeof(input));
    input[0].type = INPUT_MOUSE;
    input[0].mi.dx = x;
    input[0].mi.dy = y;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dx = x;
    input[1].mi.dy = y;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    events_no = SendInput(2, input, sizeof(input[0]));
    ok(events_no == 2, "SendInput returned %d\n", events_no);
    SetCursorPos(pt.x, pt.y);
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
    pt.x = pt.y = 50;
    ClientToScreen( child_static, &pt );
    win = WindowFromPoint(pt);
    ok(win == parent, "WindowFromPoint returned %p, expected %p\n", win, parent);

    child_button = CreateWindowExA(0, "button", "button", WS_CHILD | WS_VISIBLE,
            100, 0, 100, 100, parent, 0, NULL, NULL);
    ok(child_button != 0, "CreateWindowEx failed\n");
    pt.x = pt.y = 50;
    ClientToScreen( child_button, &pt );
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
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %lx\n", ret);

    CloseHandle(start_event);
    CloseHandle(end_event);
}

static void test_window_from_point(HWND main_window, const char *argv0)
{
    HWND hwnd, child, win;
    POINT pt;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char cmd[MAX_PATH];
    HANDLE start_event, end_event;

    hwnd = CreateWindowExA(0, "MainWindowClass", NULL, WS_POPUP | WS_VISIBLE,
            100, 100, 200, 100, main_window, 0, NULL, NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    pt.x = pt.y = 50;
    ClientToScreen( hwnd, &pt );
    win = WindowFromPoint(pt);
    pt.x = 150;
    pt.y = 50;
    ClientToScreen( hwnd, &pt );
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
    pt.x = pt.y = 50;
    ClientToScreen( hwnd, &pt );
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
    ok(wait_for_events(1, &start_event, 1000) == 0, "didn't get start_event\n");

    child = GetWindow(hwnd, GW_CHILD);
    win = WindowFromPoint(pt);
    ok(win == child, "WindowFromPoint returned %p, expected %p\n", win, child);

    simulate_click(pt.x, pt.y);
    flush_events(TRUE);

    child = GetWindow(child, GW_HWNDNEXT);
    pt.x = 150;
    pt.y = 50;
    ClientToScreen( hwnd, &pt );
    win = WindowFromPoint(pt);
    ok(win == child, "WindowFromPoint returned %p, expected %p\n", win, child);

    SetEvent(end_event);
    wait_child_process(info.hProcess);
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
    ok(window_rect.left == pos.x, "left is %ld instead of %ld\n", window_rect.left, pos.x);
    ok(window_rect.top == pos.y, "top is %ld instead of %ld\n", window_rect.top, pos.y);
    ok(window_rect.right == pos.x + width, "right is %ld instead of %ld\n", window_rect.right, pos.x + width);
    ok(window_rect.bottom == pos.y + height, "bottom is %ld instead of %ld\n", window_rect.bottom, pos.y + height);
    GetClientRect(wnd, &client_rect);
    ok(client_rect.left == 0, "left is %ld instead of 0\n", client_rect.left);
    ok(client_rect.top == 0, "top is %ld instead of 0\n", client_rect.top);
    ok(client_rect.right == width, "right is %ld instead of %d\n", client_rect.right, width);
    ok(client_rect.bottom == height, "bottom is %ld instead of %d\n", client_rect.bottom, height);

    /* Test MapWindowPoints */

    /* MapWindowPoints(NULL or wnd, NULL or wnd, NULL, 1); crashes on Windows */

    SetLastError(0xdeadbeef);
    n = MapWindowPoints(NULL, NULL, NULL, 0);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    n = MapWindowPoints(wnd, wnd, NULL, 0);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    n = MapWindowPoints(wnd, NULL, NULL, 0);
    ok(n == MAKELONG(window_rect.left, window_rect.top), "Got %x, expected %lx\n",
       n, MAKELONG(window_rect.left, window_rect.top));

    n = MapWindowPoints(NULL, wnd, NULL, 0);
    ok(n == MAKELONG(-window_rect.left, -window_rect.top), "Got %x, expected %lx\n",
       n, MAKELONG(-window_rect.left, -window_rect.top));

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, wnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(NULL, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(wnd, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(dwnd, dwnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(NULL, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    n = MapWindowPoints(wnd, wnd, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %d, expected %d\n", n, 0);
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    p.x = p.y = 100;
    n = MapWindowPoints(wnd, NULL, &p, 1);
    ok(n == MAKELONG(window_rect.left, window_rect.top), "Got %x, expected %lx\n",
       n, MAKELONG(window_rect.left, window_rect.top));
    ok((p.x == (window_rect.left + 100)) && (p.y == (window_rect.top + 100)), "Failed got (%ld, %ld), expected (%ld, %ld)\n",
       p.x, p.y, window_rect.left + 100, window_rect.top + 100);

    p.x = p.y = 100;
    n = MapWindowPoints(NULL, wnd, &p, 1);
    ok(n == MAKELONG(-window_rect.left, -window_rect.top), "Got %x, expected %lx\n",
       n, MAKELONG(-window_rect.left, -window_rect.top));
    ok((p.x == (-window_rect.left + 100)) && (p.y == (-window_rect.top + 100)), "Failed got (%ld, %ld), expected (%ld, %ld)\n",
       p.x, p.y, -window_rect.left + 100, -window_rect.top + 100);

    SetLastError(0xdeadbeef);
    p.x = p.y = 0;
    n = MapWindowPoints(wnd0, NULL, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %x,  expected 0\n", n);
    ok((p.x == 0) && (p.y == 0), "Failed got (%ld, %ld), expected (0, 0)\n", p.x, p.y);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    SetLastError(0xdeadbeef);
    p.x = p.y = 0;
    n = MapWindowPoints(NULL, wnd0, &p, 1);
    err = GetLastError();
    ok(n == 0, "Got %x,  expected 0\n", n);
    ok((p.x == 0) && (p.y == 0), "Failed got (%ld, %ld), expected (0, 0)\n", p.x, p.y);
    ok(err == 0xdeadbeef, "Got %lx, expected %x\n", err, 0xdeadbeef);

    /* Test ClientToScreen */

    /* ClientToScreen(wnd, NULL); crashes on Windows */

    SetLastError(0xdeadbeef);
    ret = ClientToScreen(NULL, NULL);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ClientToScreen(NULL, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ClientToScreen(dwnd, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    p.x = p.y = 100;
    ret = ClientToScreen(wnd, &p);
    ok(ret, "Failed with error %lu\n", GetLastError());
    ok((p.x == (window_rect.left + 100)) && (p.y == (window_rect.top + 100)), "Failed got (%ld, %ld), expected (%ld, %ld)\n",
       p.x, p.y, window_rect.left + 100, window_rect.top + 100);

    p.x = p.y = 0;
    ret = ClientToScreen(wnd0, &p);
    ok(ret, "Failed with error %lu\n", GetLastError());
    ok((p.x == 0) && (p.y == 0), "Failed got (%ld, %ld), expected (0, 0)\n", p.x, p.y);

    /* Test ScreenToClient */

    /* ScreenToClient(wnd, NULL); crashes on Windows */

    SetLastError(0xdeadbeef);
    ret = ScreenToClient(NULL, NULL);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ScreenToClient(NULL, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    SetLastError(0xdeadbeef);
    p.x = p.y = 100;
    ret = ScreenToClient(dwnd, &p);
    err = GetLastError();
    ok(!ret, "Should fail\n");
    ok(p.x == 100 && p.y == 100, "Failed got(%ld, %ld), expected (%d, %d)\n", p.x, p.y, 100, 100);
    ok(err == ERROR_INVALID_WINDOW_HANDLE, "Got %lx, expected %x\n", err, ERROR_INVALID_WINDOW_HANDLE);

    p.x = p.y = 100;
    ret = ScreenToClient(wnd, &p);
    ok(ret, "Failed with error %lu\n", GetLastError());
    ok((p.x == (-window_rect.left + 100)) && (p.y == (-window_rect.top + 100)), "Failed got(%ld, %ld), expected (%ld, %ld)\n",
       p.x, p.y, -window_rect.left + 100, -window_rect.top + 100);

    p.x = p.y = 0;
    ret = ScreenToClient(wnd0, &p);
    ok(ret, "Failed with error %lu\n", GetLastError());
    ok((p.x == 0) && (p.y == 0), "Failed got (%ld, %ld), expected (0, 0)\n", p.x, p.y);

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
        ok(res == 0x449b0190, "unexpected result %Ix\n", res);

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
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    ok(WaitForSingleObject(data.thread_started, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    res = SendMessageA(data.thread_hwnd, WM_APP, 0, (LPARAM)&data);
    ok(res == 0x240408ea, "unexpected result %Ix\n", res);

    SendMessageA(data.thread_hwnd, WM_CLOSE, 0, 0);

    DestroyWindow(data.main_hwnd);

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    CloseHandle(data.thread_started);
    CloseHandle(data.thread_got_wm_app);
    CloseHandle(data.main_in_wm_app_1);
    CloseHandle(data.thread_replied);
    CloseHandle(hThread);
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
    ok(pos == MAKELONG(120, 140), "pos = %08lx\n", pos);

    SetCursorPos(340, 320);
    pos = GetMessagePos();
    ok(pos == MAKELONG(120, 140), "pos = %08lx\n", pos);

    SendMessageW(button, WM_APP, 0, 0);
    pos = GetMessagePos();
    ok(pos == MAKELONG(120, 140), "pos = %08lx\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08lx\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    SetCursorPos(350, 330);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08lx\n", pos);

    PostMessageA(button, WM_APP, 0, 0);
    SetCursorPos(320, 340);
    PostMessageA(button, WM_APP+1, 0, 0);
    pos = GetMessagePos();
    ok(pos == MAKELONG(340, 320), "pos = %08lx\n", pos);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(350, 330), "pos = %08lx\n", pos);
    GetMessageA(&msg, button, 0, 0);
    ok(msg.message == WM_APP+1, "msg.message = %x\n", msg.message);
    pos = GetMessagePos();
    ok(pos == MAKELONG(320, 340), "pos = %08lx\n", pos);

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
    ok(pos == MAKELONG(330, 350), "pos = %08lx\n", pos);
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

        ok(!wparam, "wparam = %08Ix\n", wparam);
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

        ok(!wparam, "wparam = %08Ix\n", wparam);
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
    ok(ret, "RegisterClass failed with error %ld\n", GetLastError());

    ret = GetClassInfoW(hinst, winproc_testW, &wnd_classW);
    ok(ret, "GetClassInfoW failed with error %ld\n", GetLastError());
    ok(wnd_classA.lpfnWndProc != wnd_classW.lpfnWndProc,
            "winproc pointers should not be identical\n");

    count = 0;
    CallWindowProcA(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
    ok(count == 1, "winproc should be called once (%d)\n", count);
    count = 0;
    CallWindowProcW(wnd_classW.lpfnWndProc, 0, 0, 0, (LPARAM)&count);
    ok(count == 1, "winproc should be called once (%d)\n", count);

    ret = UnregisterClassW(winproc_testW, hinst);
    ok(ret, "UnregisterClass failed with error %ld\n", GetLastError());

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
    wait_child_process(info.hProcess);
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
    ok(ret, "SetWindowLongPtr failed with error %ld\n", GetLastError());
    ok(SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"text"), "WM_SETTEXT failed\n");
    ok(SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW), "WM_SETTEXT with conversion failed\n");

    ret = SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)winproc_convW);
    ok(ret, "SetWindowLongPtr failed with error %ld\n", GetLastError());
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
    HWND hwnd;
    BOOL ret;

    hdwp = BeginDeferWindowPos(0);
    ok(hdwp != NULL, "got %p\n", hdwp);

    ret = EndDeferWindowPos(NULL);
    ok(!ret, "got %d\n", ret);

    hdwp2 = DeferWindowPos(NULL, NULL, NULL, 0, 0, 10, 10, 0);
    todo_wine
    ok(hdwp2 == NULL && ((GetLastError() == ERROR_INVALID_DWP_HANDLE) ||
        broken(GetLastError() == ERROR_INVALID_WINDOW_HANDLE) /* before win8 */), "got %p, error %ld\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos((HDWP)0xdead, GetDesktopWindow(), NULL, 0, 0, 10, 10, 0);
    todo_wine
    ok(hdwp2 == NULL && ((GetLastError() == ERROR_INVALID_DWP_HANDLE) ||
        broken(GetLastError() == ERROR_INVALID_WINDOW_HANDLE) /* before win8 */), "got %p, error %ld\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, NULL, NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %ld\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, GetDesktopWindow(), NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %ld\n", hdwp2, GetLastError());

    hdwp2 = DeferWindowPos(hdwp, (HWND)0xdead, NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 == NULL && GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "got %p, error %ld\n", hdwp2, GetLastError());

    ret = EndDeferWindowPos(hdwp);
    ok(ret, "got %d\n", ret);
    hdwp = BeginDeferWindowPos(0);
    ok(hdwp != NULL, "got %p\n", hdwp);

    hwnd = create_tool_window(WS_POPUP, 0);
    hdwp2 = DeferWindowPos(hdwp, hwnd, NULL, 0, 0, 10, 10, 0);
    ok(hdwp2 != NULL, "got %p, error %ld\n", hdwp2, GetLastError());
    DestroyWindow(hwnd);

    ret = EndDeferWindowPos(hdwp);
    ok(ret, "got %d\n", ret);
}

static void test_LockWindowUpdate(HWND parent)
{
    typedef struct
    {
        HWND hwnd_lock, hwnd_draw;
        BOOL allow_drawing;
        BOOL expect_valid;
    } TEST;

    int i;
    HWND child = CreateWindowA("static", 0, WS_CHILD | WS_VISIBLE, 0, 0, 20, 20, parent, 0, 0, 0);

    TEST tests[] = {
        {child, child, 0, 0},
        {child, child, 1, 1},
        {child, parent, 0, 1},
        {child, parent, 1, 1},
        {parent, child, 0, 0},
        {parent, child, 1, 1},
        {parent, parent, 0, 0},
        {parent, parent, 1, 1}
    };

    if (!child)
    {
        skip("CreateWindow failed, skipping LockWindowUpdate tests\n");
        return;
    }

    ShowWindow(parent, SW_SHOW);
    UpdateWindow(parent);
    flush_events(TRUE);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        HDC hdc;
        POINT p = {10, 10};
        BOOL ret;
        const DWORD dc_flags = DCX_USESTYLE | (tests[i].allow_drawing ? DCX_LOCKWINDOWUPDATE : 0);
        const COLORREF c1 = 0x111100, c2 = 0x222200;

        hdc = GetDCEx(tests[i].hwnd_draw, 0, dc_flags);

#define TEST_PIXEL(c_valid, c_invalid) \
            do { \
                COLORREF c = GetPixel(hdc, p.x, p.y); \
                COLORREF e = tests[i].expect_valid ? (c_valid) : (c_invalid); \
                todo_wine_if(!tests[i].expect_valid) \
                ok(c == e, "%u: GetPixel: got %08lx, expected %08lx\n", i, c, e); \
            } while (0)

        SetPixel(hdc, p.x, p.y, c1);
        ret = LockWindowUpdate(tests[i].hwnd_lock);
        ok(ret, "%u: LockWindowUpdate failed\n", i);
        TEST_PIXEL(c1, CLR_INVALID);
        SetPixel(hdc, p.x, p.y, c2);
        TEST_PIXEL(c2, CLR_INVALID);
        LockWindowUpdate(0);
        TEST_PIXEL(c2, c1);
        ReleaseDC(tests[i].hwnd_draw, hdc);
#undef TEST_PIXEL
    }
    DestroyWindow(child);
}

static void test_hide_window(void)
{
    HWND hwnd, hwnd2, hwnd3;

    hwnd = CreateWindowExA(0, "MainWindowClass", "Main window", WS_POPUP | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    hwnd2 = CreateWindowExA(0, "MainWindowClass", "Main window 2", WS_POPUP | WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    if (winetest_debug > 1) trace("hwnd = %p, hwnd2 = %p\n", hwnd, hwnd2);
    check_active_state(hwnd2, hwnd2, hwnd2);
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));

    /* test hiding two normal windows */
    ShowWindow(hwnd2, SW_HIDE);
    check_active_state(hwnd, hwnd, hwnd);
    todo_wine
    ok(GetWindow(hwnd, GW_HWNDNEXT) == hwnd2, "expected %p, got %p\n", hwnd2, GetWindow(hwnd, GW_HWNDNEXT));

    ShowWindow(hwnd, SW_HIDE);
    check_active_state(hwndMain, 0, hwndMain);
    ok(GetWindow(hwnd, GW_HWNDNEXT) == hwnd2, "expected %p, got %p\n", hwnd2, GetWindow(hwnd, GW_HWNDNEXT));

    ShowWindow(hwnd, SW_SHOW);
    check_active_state(hwnd, hwnd, hwnd);

    ShowWindow(hwnd2, SW_SHOW);
    check_active_state(hwnd2, hwnd2, hwnd2);
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));

    /* hide a non-active window */
    ShowWindow(hwnd, SW_HIDE);
    check_active_state(hwnd2, hwnd2, hwnd2);
    todo_wine
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));

    /* hide a window in the middle */
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd2, SW_SHOW);
    hwnd3 = CreateWindowExA(0, "MainWindowClass", "Main window 3", WS_POPUP | WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    SetActiveWindow(hwnd2);
    ShowWindow(hwnd2, SW_HIDE);
    check_active_state(hwnd3, hwnd3, hwnd3);
    todo_wine {
    ok(GetWindow(hwnd3, GW_HWNDNEXT) == hwnd2, "expected %p, got %p\n", hwnd2, GetWindow(hwnd3, GW_HWNDNEXT));
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));
    }

    DestroyWindow(hwnd3);

    /* hide a normal window when there is a topmost window */
    hwnd3 = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", "Topmost window 3", WS_POPUP|WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd2, SW_SHOW);
    check_active_state(hwnd2, hwnd2, hwnd2);
    ShowWindow(hwnd2, SW_HIDE);
    todo_wine
    check_active_state(hwnd3, hwnd3, hwnd3);
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));

    /* hide a topmost window */
    ShowWindow(hwnd2, SW_SHOW);
    ShowWindow(hwnd3, SW_SHOW);
    ShowWindow(hwnd3, SW_HIDE);
    check_active_state(hwnd2, hwnd2, hwnd2);
    ok(GetWindow(hwnd2, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd2, GW_HWNDNEXT));

    DestroyWindow(hwnd3);

    /* hiding an owned window activates its owner */
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd2, SW_SHOW);
    hwnd3 = CreateWindowExA(0, "MainWindowClass", "Owned window 3", WS_POPUP|WS_VISIBLE,
                            100, 100, 200, 200, hwnd, 0, GetModuleHandleA(NULL), NULL);
    ShowWindow(hwnd3, SW_HIDE);
    check_active_state(hwnd, hwnd, hwnd);
    todo_wine {
    ok(GetWindow(hwnd3, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd3, GW_HWNDNEXT));
    ok(GetWindow(hwnd, GW_HWNDNEXT) == hwnd2, "expected %p, got %p\n", hwnd2, GetWindow(hwnd, GW_HWNDNEXT));
    }

    /* hide an owner window */
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(hwnd2, SW_SHOW);
    ShowWindow(hwnd3, SW_SHOW);
    ShowWindow(hwnd, SW_HIDE);
    check_active_state(hwnd3, hwnd3, hwnd3);
    ok(GetWindow(hwnd3, GW_HWNDNEXT) == hwnd, "expected %p, got %p\n", hwnd, GetWindow(hwnd3, GW_HWNDNEXT));
    ok(GetWindow(hwnd, GW_HWNDNEXT) == hwnd2, "expected %p, got %p\n", hwnd2, GetWindow(hwnd, GW_HWNDNEXT));

    DestroyWindow(hwnd3);
    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
}

static void test_minimize_window(HWND hwndMain)
{
    HWND hwnd, hwnd2, hwnd3;

    hwnd = CreateWindowExA(0, "MainWindowClass", "Main window", WS_POPUP | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    hwnd2 = CreateWindowExA(0, "MainWindowClass", "Main window 2", WS_POPUP | WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    if (winetest_debug > 1) trace("hwnd = %p, hwnd2 = %p\n", hwnd, hwnd2);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* test hiding two normal windows */
    ShowWindow(hwnd2, SW_MINIMIZE);
    todo_wine
    check_active_state(hwnd, hwnd, hwnd);

    ShowWindow(hwnd, SW_MINIMIZE);
    todo_wine
    if (GetActiveWindow() == 0)
        check_active_state(0, 0, 0);

    ShowWindow(hwnd, SW_RESTORE);
    check_active_state(hwnd, hwnd, hwnd);

    ShowWindow(hwnd2, SW_RESTORE);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* try SW_SHOWMINIMIZED */
    ShowWindow(hwnd2, SW_SHOWMINIMIZED);
    check_active_state(hwnd2, hwnd2, 0);

    ShowWindow(hwnd2, SW_RESTORE);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* hide a non-active window */
    ShowWindow(hwnd, SW_MINIMIZE);
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* hide a window in the middle */
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd2, SW_RESTORE);
    hwnd3 = CreateWindowExA(0, "MainWindowClass", "Main window 3", WS_POPUP | WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    SetActiveWindow(hwnd2);
    ShowWindow(hwnd2, SW_MINIMIZE);
    todo_wine
    check_active_state(hwnd3, hwnd3, hwnd3);

    DestroyWindow(hwnd3);

    /* hide a normal window when there is a topmost window */
    hwnd3 = CreateWindowExA(WS_EX_TOPMOST, "MainWindowClass", "Topmost window 3", WS_POPUP|WS_VISIBLE,
                            100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd2, SW_RESTORE);
    check_active_state(hwnd2, hwnd2, hwnd2);
    ShowWindow(hwnd2, SW_MINIMIZE);
    todo_wine
    check_active_state(hwnd3, hwnd3, hwnd3);

    /* hide a topmost window */
    ShowWindow(hwnd2, SW_RESTORE);
    ShowWindow(hwnd3, SW_RESTORE);
    ShowWindow(hwnd3, SW_MINIMIZE);
    check_active_state(hwnd2, hwnd2, hwnd2);

    DestroyWindow(hwnd3);

    /* hide an owned window */
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd2, SW_RESTORE);
    hwnd3 = CreateWindowExA(0, "MainWindowClass", "Owned window 3", WS_POPUP|WS_VISIBLE,
                            100, 100, 200, 200, hwnd, 0, GetModuleHandleA(NULL), NULL);
    ShowWindow(hwnd3, SW_MINIMIZE);
    todo_wine
    check_active_state(hwnd2, hwnd2, hwnd2);

    /* with SW_SHOWMINIMIZED */
    ShowWindow(hwnd3, SW_RESTORE);
    ShowWindow(hwnd3, SW_SHOWMINIMIZED);
    check_active_state(hwnd3, hwnd3, 0);

    /* hide an owner window */
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd2, SW_RESTORE);
    ShowWindow(hwnd3, SW_RESTORE);
    ShowWindow(hwnd, SW_MINIMIZE);
    todo_wine
    check_active_state(hwnd2, hwnd2, hwnd2);

    DestroyWindow(hwnd3);

    /* test a child window - focus should be yielded back to the parent */
    ShowWindow(hwnd, SW_RESTORE);
    hwnd3 = CreateWindowExA(0, "MainWindowClass", "Child window 3", WS_CHILD|WS_VISIBLE,
                            100, 100, 200, 200, hwnd, 0, GetModuleHandleA(NULL), NULL);
    SetFocus(hwnd3);
    check_active_state(hwnd, hwnd, hwnd3);
    ShowWindow(hwnd3, SW_MINIMIZE);
    check_active_state(hwnd, hwnd, hwnd);

    /* with SW_SHOWMINIMIZED */
    ShowWindow(hwnd3, SW_RESTORE);
    SetFocus(hwnd3);
    check_active_state(hwnd, hwnd, hwnd3);
    ShowWindow(hwnd3, SW_SHOWMINIMIZED);
    check_active_state(hwnd, hwnd, hwnd);

    DestroyWindow(hwnd3);
    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
}

static void test_desktop( void )
{
    HWND desktop = GetDesktopWindow();
    /* GetWindowLong Desktop window tests */
    static const struct
    {
        int offset;
        ULONG_PTR expect;
        DWORD error;
    }
    tests[] =
    {
        { GWLP_USERDATA,   0, 0 },
        { GWL_STYLE,       WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0 },
        { GWL_EXSTYLE,     0, 0 },
        { GWLP_ID,         0, 0 },
        { GWLP_HWNDPARENT, 0, 0 },
        /* GWLP_HINSTANCE - not useful and not consistent between Windows versions */
        { GWLP_WNDPROC,    0, ERROR_ACCESS_DENIED },
        { DWLP_MSGRESULT,  0, ERROR_INVALID_INDEX }
    };
    DWORD_PTR result;
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError( 0xdeadbeef );
        result = GetWindowLongPtrW( desktop, tests[i].offset );
        ok( result == tests[i].expect, "offset %d, got %08Ix expect %08Ix\n",
            tests[i].offset, result, tests[i].expect );
        if (tests[i].error)
            ok( GetLastError() == tests[i].error, "offset %d: error %ld expect %ld\n",
                tests[i].offset, GetLastError(), tests[i].error );
        else
            ok( GetLastError() == 0xdeadbeef, "offset %d: error %ld expect unchanged\n",
                tests[i].offset, GetLastError() );
    }
}

static BOOL is_topmost(HWND hwnd)
{
    return (GetWindowLongA(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

static void swp_after(HWND hwnd, HWND after)
{
    BOOL ret;

    ret = SetWindowPos(hwnd, after, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    ok(ret, "SetWindowPos failed\n");
    flush_events( TRUE );
}

static void reset_window_state(HWND *state, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        if (state[i])
        {
            swp_after(state[i], HWND_NOTOPMOST);
            todo_wine_if(i == 5) /* FIXME: remove once Wine is fixed */
            ok(!is_topmost(state[i]), "%d: hwnd %p is still topmost\n", i, state[i]);
            swp_after(state[i], HWND_TOP);
        }
    }
}

static void test_topmost(void)
{
    HWND owner, hwnd, hwnd2, hwnd_child, hwnd_child2, hwnd_grandchild, state[6] = { 0 };
    BOOL is_wine = !strcmp(winetest_platform, "wine");

    owner = create_tool_window(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                              WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE, 0);
    ok(owner != 0, "Failed to create owner window (%ld)\n", GetLastError());

    /* Give current thread foreground state otherwise the tests may fail. */
    if (!SetForegroundWindow(owner))
    {
        DestroyWindow(owner);
        skip("SetForegroundWindow not working\n");
        return;
    }

    hwnd = create_tool_window(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                              WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE, owner);
    ok(hwnd != 0, "Failed to create popup window (%ld)\n", GetLastError());
    hwnd2 = create_tool_window(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                              WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE, owner);
    ok(hwnd2 != 0, "Failed to create popup window (%ld)\n", GetLastError());

    flush_events( TRUE );

    if (winetest_debug > 1) trace("owner %p, hwnd %p, hwnd2 %p\n", owner, hwnd, hwnd2);
    state[0] = owner;
    state[1] = hwnd;
    state[2] = hwnd2;

    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, 0, hwnd2, owner, FALSE);

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd, HWND_TOP);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd2, hwnd);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd, hwnd2);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd2, HWND_TOP);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(owner, HWND_TOPMOST);
    todo_wine
    ok(is_topmost(owner), "owner should be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd2), "hwnd2 should be topmost\n");
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner) || broken(is_topmost(owner)) /*win7 64-bit*/, "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2) || broken(is_topmost(hwnd2)) /*win7 64-bit*/, "hwnd2 should NOT be topmost\n");
    if (0) /*win7 64-bit is broken*/
    check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    swp_after(hwnd2, HWND_NOTOPMOST);
    ok(!is_topmost(owner) || broken(is_topmost(owner)) /*win7 64-bit*/, "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, 0, hwnd2, owner, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd, HWND_BOTTOM);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, 0, hwnd2, owner, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    swp_after(hwnd, hwnd2);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    todo_wine
        ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, 0, hwnd2, owner, FALSE);
    /* FIXME: compensate todo_wine above */
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, 0, hwnd2, owner, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    hwnd_child2 = create_tool_window(WS_VISIBLE|WS_POPUP, hwnd);
    ok(hwnd_child2 != 0, "Failed to create popup window (%ld)\n", GetLastError());
    hwnd_child = create_tool_window(WS_VISIBLE|WS_POPUP, hwnd);
    ok(hwnd_child != 0, "Failed to create popup window (%ld)\n", GetLastError());
    hwnd_grandchild = create_tool_window(WS_VISIBLE|WS_POPUP, hwnd_child);
    ok(hwnd_grandchild != 0, "Failed to create popup window (%ld)\n", GetLastError());

    flush_events( TRUE );

    if (winetest_debug > 1)
        trace("hwnd_child %p, hwnd_child2 %p, hwnd_grandchild %p\n", hwnd_child, hwnd_child2, hwnd_grandchild);
    state[3] = hwnd_child2;
    state[4] = hwnd_child;
    state[5] = hwnd_grandchild;

    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd2), "hwnd2 should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child2), "child2 should be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
    check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child2), "child2 should be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd_child, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd), "hwnd should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child2), "child2 should be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, TRUE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd_grandchild, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd_child, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd_child, HWND_TOP);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild) || broken(is_topmost(hwnd_grandchild))/*win2008 64-bit*/, "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd_child, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd_grandchild, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    check_z_order(hwnd_child, hwnd_child2, 0, hwnd, FALSE);
    swp_after(hwnd_child2, HWND_NOTOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild) || broken(!is_topmost(hwnd_grandchild)) /* win8+ */, "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, 0, hwnd_child2, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd_child, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd_child, HWND_BOTTOM);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, 0, hwnd2, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
    check_z_order(hwnd_child, 0, hwnd_child2, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    swp_after(hwnd_child, HWND_TOPMOST);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    todo_wine
    ok(is_topmost(hwnd_child), "child should be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    ok(is_topmost(hwnd_grandchild), "grandchild should be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, hwnd_child2, 0, hwnd, TRUE);
    swp_after(hwnd_child, hwnd_child2);
    ok(!is_topmost(owner), "owner should NOT be topmost\n");
    ok(!is_topmost(hwnd), "hwnd should NOT be topmost\n");
    ok(!is_topmost(hwnd_child), "child should NOT be topmost\n");
    ok(!is_topmost(hwnd_child2), "child2 should NOT be topmost\n");
    todo_wine
    ok(!is_topmost(hwnd_grandchild), "grandchild should NOT be topmost\n");
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd, hwnd2, 0, owner, FALSE);
    if (!is_wine) /* FIXME: remove once Wine is fixed */
        check_z_order(hwnd_child, 0, hwnd_child2, hwnd, FALSE);
    reset_window_state(state, ARRAY_SIZE(state));

    DestroyWindow(hwnd_grandchild);
    DestroyWindow(hwnd_child);
    DestroyWindow(hwnd_child2);
    DestroyWindow(hwnd);
    DestroyWindow(hwnd2);
    DestroyWindow(owner);
}

static void test_display_affinity( HWND win )
{
    DWORD affinity;
    BOOL ret, dwm;
    LONG styleex;

    if (!pGetWindowDisplayAffinity || !pSetWindowDisplayAffinity)
    {
        win_skip("GetWindowDisplayAffinity or SetWindowDisplayAffinity missing\n");
        return;
    }

    ret = pGetWindowDisplayAffinity(NULL, NULL);
    ok(!ret, "GetWindowDisplayAffinity succeeded\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "Expected ERROR_INVALID_WINDOW_HANDLE, got %lu\n", GetLastError());

    ret = pGetWindowDisplayAffinity(NULL, &affinity);
    ok(!ret, "GetWindowDisplayAffinity succeeded\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "Expected ERROR_INVALID_WINDOW_HANDLE, got %lu\n", GetLastError());

    ret = pGetWindowDisplayAffinity(win, NULL);
    ok(!ret, "GetWindowDisplayAffinity succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "Expected ERROR_NOACCESS, got %lu\n", GetLastError());

    styleex = GetWindowLongW(win, GWL_EXSTYLE);
    SetWindowLongW(win, GWL_EXSTYLE, styleex & ~WS_EX_LAYERED);

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    /* Windows 7 fails with ERROR_NOT_ENOUGH_MEMORY when dwm compositing is disabled */
    ret = pSetWindowDisplayAffinity(win, WDA_MONITOR);
    ok(ret || GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
       "SetWindowDisplayAffinity failed with %lu\n", GetLastError());
    dwm = ret;

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    if (dwm) ok(affinity == WDA_MONITOR, "Expected WDA_MONITOR, got 0x%lx\n", affinity);
    else ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    ret = pSetWindowDisplayAffinity(win, WDA_NONE);
    ok(ret || GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
       "SetWindowDisplayAffinity failed with %lu\n", GetLastError());

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    SetWindowLongW(win, GWL_EXSTYLE, styleex | WS_EX_LAYERED);

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    ret = pSetWindowDisplayAffinity(win, WDA_MONITOR);
    ok(ret || GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
       "SetWindowDisplayAffinity failed with %lu\n", GetLastError());

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    if (dwm) ok(affinity == WDA_MONITOR, "Expected WDA_MONITOR, got 0x%lx\n", affinity);
    else ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    ret = pSetWindowDisplayAffinity(win, WDA_NONE);
    ok(ret || GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
       "SetWindowDisplayAffinity failed with %lu\n", GetLastError());

    affinity = 0xdeadbeef;
    ret = pGetWindowDisplayAffinity(win, &affinity);
    ok(ret, "GetWindowDisplayAffinity failed with %lu\n", GetLastError());
    ok(affinity == WDA_NONE, "Expected WDA_NONE, got 0x%lx\n", affinity);

    SetWindowLongW(win, GWL_EXSTYLE, styleex);
}

static struct destroy_data
{
    HWND main_wnd;
    HWND thread1_wnd;
    HWND thread2_wnd;
    HANDLE evt;
    DWORD main_tid;
    DWORD destroy_count;
    DWORD ncdestroy_count;
} destroy_data;

static LRESULT WINAPI destroy_thread1_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        ok( destroy_data.destroy_count > 0, "parent didn't get WM_DESTROY\n" );
        PostQuitMessage(0);
        break;
    case WM_NCDESTROY:
        ok( destroy_data.ncdestroy_count > 0, "parent didn't get WM_NCDESTROY\n" );
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI destroy_thread2_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        ok( destroy_data.destroy_count > 0, "parent didn't get WM_DESTROY\n" );
        break;
    case WM_NCDESTROY:
        ok( destroy_data.ncdestroy_count > 0, "parent didn't get WM_NCDESTROY\n" );
        ok( WaitForSingleObject(destroy_data.evt, 10000) != WAIT_TIMEOUT, "timeout\n" );
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI destroy_main_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        destroy_data.destroy_count++;
        break;
    case WM_NCDESTROY:
        destroy_data.ncdestroy_count++;
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static DWORD CALLBACK destroy_thread1(void *user)
{
    MSG msg;

    destroy_data.thread1_wnd = CreateWindowExA(0, "destroy_test_thread1",
            "destroy test thread", WS_CHILD, 100, 100, 100, 100,
            destroy_data.main_wnd, 0, GetModuleHandleA(NULL), NULL);
    ok(destroy_data.thread1_wnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    PostThreadMessageW(destroy_data.main_tid, WM_USER, 0, 0);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    PostThreadMessageW(destroy_data.main_tid, WM_USER + 2, 0, 0);
    ok( WaitForSingleObject(destroy_data.evt, 10000) != WAIT_TIMEOUT, "timeout\n" );
    ok( IsWindow( destroy_data.thread1_wnd ), "window destroyed\n" );
    return 0;
}

static DWORD CALLBACK destroy_thread2(void *user)
{
    MSG msg;

    destroy_data.thread2_wnd = CreateWindowExA(0, "destroy_test_thread2",
            "destroy test thread", WS_CHILD, 100, 100, 100, 100,
            destroy_data.main_wnd, 0, GetModuleHandleA(NULL), NULL);
    ok(destroy_data.thread2_wnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());

    PostThreadMessageW(destroy_data.main_tid, WM_USER + 1, 0, 0);
    Sleep( 100 );

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok( !IsWindow( destroy_data.thread2_wnd ), "window not destroyed\n" );
    return 0;
}

static void test_destroy_quit(void)
{
    MSG msg;
    WNDCLASSA wnd_classA;
    ATOM ret;
    HANDLE thread1, thread2;

    destroy_data.main_tid = GetCurrentThreadId();
    destroy_data.evt = CreateEventW(NULL, TRUE, FALSE, NULL);
    destroy_data.destroy_count = 0;
    destroy_data.ncdestroy_count = 0;

    memset(&wnd_classA, 0, sizeof(wnd_classA));
    wnd_classA.lpszClassName = "destroy_test_main";
    wnd_classA.lpfnWndProc = destroy_main_wndproc;
    ret = RegisterClassA(&wnd_classA);
    ok(ret, "RegisterClass failed with error %ld\n", GetLastError());

    wnd_classA.lpszClassName = "destroy_test_thread1";
    wnd_classA.lpfnWndProc = destroy_thread1_wndproc;
    ret = RegisterClassA(&wnd_classA);
    ok(ret, "RegisterClass failed with error %ld\n", GetLastError());

    wnd_classA.lpszClassName = "destroy_test_thread2";
    wnd_classA.lpfnWndProc = destroy_thread2_wndproc;
    ret = RegisterClassA(&wnd_classA);
    ok(ret, "RegisterClass failed with error %ld\n", GetLastError());

    destroy_data.main_wnd = CreateWindowExA(0, "destroy_test_main",
            "destroy test main", WS_OVERLAPPED | WS_CAPTION, 100, 100, 100, 100,
            0, 0, GetModuleHandleA(NULL), NULL);
    ok(destroy_data.main_wnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    if (!destroy_data.main_wnd)
    {
        CloseHandle(destroy_data.evt);
        return;
    }

    thread1 = CreateThread(NULL, 0, destroy_thread1, 0, 0, NULL);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        BOOL done = 0;
        switch (msg.message)
        {
        case WM_USER:
            thread2 = CreateThread(NULL, 0, destroy_thread2, 0, 0, NULL);
            CloseHandle( thread2 );
            break;
        case WM_USER + 1:
            DestroyWindow(destroy_data.main_wnd);
            break;
        case WM_USER + 2:
            SetEvent(destroy_data.evt);
            done = 1;
            break;
        default:
            DispatchMessageA(&msg);
            break;
        }
        if (done) break;
    }

    ok( WaitForSingleObject( thread1, 10000 ) != WAIT_TIMEOUT, "timeout\n" );
    ok( !IsWindow( destroy_data.thread1_wnd ), "window not destroyed\n" );
    CloseHandle( thread1 );
}

static void test_IsWindowEnabled(void)
{
    BOOL ret;
    HWND hwnd;

    ret = IsWindowEnabled(NULL);
    ok(!ret, "Expect IsWindowEnabled() return FALSE\n");

    hwnd = GetDesktopWindow();
    ret = IsWindowEnabled(hwnd);
    ok(ret, "Expect IsWindowEnabled() return TRUE\n");

    hwnd = create_tool_window(WS_CHILD | WS_VISIBLE, hwndMain);
    ret = IsWindowEnabled(hwnd);
    ok(ret, "Expect IsWindowEnabled() return TRUE\n");
    EnableWindow(hwnd, FALSE);
    ret = IsWindowEnabled(hwnd);
    ok(!ret, "Expect IsWindowEnabled() return FALSE\n");
    DestroyWindow(hwnd);
}

static void test_window_placement(void)
{
    RECT orig = {100, 200, 300, 400}, orig2 = {200, 300, 400, 500}, rect, work_rect;
    WINDOWPLACEMENT wp = {sizeof(wp)};
    MONITORINFO mon_info;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowA("MainWindowClass", "wp", WS_OVERLAPPEDWINDOW,
        orig.left, orig.top, orig.right - orig.left, orig.bottom - orig.top, 0, 0, 0, 0);
    ok(!!hwnd, "failed to create window, error %lu\n", GetLastError());

    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mon_info);
    work_rect = mon_info.rcWork;

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWNORMAL, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -1 && wp.ptMinPosition.y == -1,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_MINIMIZE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(!wp.flags, "got flags %#x\n", wp.flags);
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_RESTORE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWNORMAL, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_MAXIMIZE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    SetWindowPos(hwnd, 0, 100, 100, 100, 100, SWP_NOZORDER | SWP_NOACTIVATE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == 100 && wp.ptMaxPosition.y == 100,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    SetWindowPos(hwnd, 0, work_rect.left, work_rect.top, work_rect.right - work_rect.left,
                 work_rect.bottom - work_rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    SetWindowPos(hwnd, 0, work_rect.left, work_rect.top, work_rect.right - work_rect.left - 1,
                 work_rect.bottom - work_rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == work_rect.left && wp.ptMaxPosition.y == work_rect.top,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    SetWindowPos(hwnd, 0, work_rect.left, work_rect.top, work_rect.right - work_rect.left,
                 work_rect.bottom - work_rect.top - 1, SWP_NOZORDER | SWP_NOACTIVATE);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == work_rect.left && wp.ptMaxPosition.y == work_rect.top,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_MINIMIZE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.flags == WPF_RESTORETOMAXIMIZED, "got flags %#x\n", wp.flags);
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_RESTORE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_RESTORE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWNORMAL, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    wp.flags = WPF_SETMINPOSITION;
    wp.ptMinPosition.x = wp.ptMinPosition.y = 100;
    wp.ptMaxPosition.x = wp.ptMaxPosition.y = 100;
    wp.rcNormalPosition = orig2;
    ret = SetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to set window placement, error %lu\n", GetLastError());

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWNORMAL, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == 100 && wp.ptMinPosition.y == 100,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig2), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&rect, &orig2), "got window rect %s\n", wine_dbgstr_rect(&rect));

    ShowWindow(hwnd, SW_MINIMIZE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(!wp.flags, "got flags %#x\n", wp.flags);
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig2), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_RESTORE);

    wp.flags = WPF_SETMINPOSITION;
    wp.showCmd = SW_MINIMIZE;
    wp.ptMinPosition.x = wp.ptMinPosition.y = 100;
    wp.ptMaxPosition.x = wp.ptMaxPosition.y = 100;
    wp.rcNormalPosition = orig;
    ret = SetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to set window placement, error %lu\n", GetLastError());

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(!wp.flags, "got flags %#x\n", wp.flags);
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ShowWindow(hwnd, SW_RESTORE);

    wp.flags = WPF_SETMINPOSITION;
    wp.showCmd = SW_MAXIMIZE;
    wp.ptMinPosition.x = wp.ptMinPosition.y = 100;
    wp.ptMaxPosition.x = wp.ptMaxPosition.y = 100;
    wp.rcNormalPosition = orig;
    ret = SetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to set window placement, error %lu\n", GetLastError());

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == 100 && wp.ptMinPosition.y == 100,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    wp.flags = WPF_SETMINPOSITION;
    wp.showCmd = SW_NORMAL;
    wp.ptMinPosition.x = wp.ptMinPosition.y = 100;
    wp.ptMaxPosition.x = wp.ptMaxPosition.y = 100;
    wp.rcNormalPosition = orig;
    ret = SetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to set window placement, error %lu\n", GetLastError());

    ShowWindow(hwnd, SW_MINIMIZE);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));

    ret = SetWindowPos(hwnd, NULL, 100, 100, 151, 151, SWP_NOACTIVATE | SWP_NOZORDER);
    ok(ret, "failed to set window pos, error %lu\n", GetLastError());

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_SHOWMINIMIZED, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));
    GetWindowRect(hwnd, &rect);
    ok(rect.left == -32000 && rect.top == -32000, "got window rect %s\n", wine_dbgstr_rect(&rect));

    ShowWindow(hwnd, SW_SHOWNORMAL);

    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to get window placement, error %lu\n", GetLastError());
    ok(wp.showCmd == SW_NORMAL, "got show cmd %u\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -32000 && wp.ptMinPosition.y == -32000,
        "got minimized pos (%ld,%ld)\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
        "got maximized pos (%ld,%ld)\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    ok(EqualRect(&wp.rcNormalPosition, &orig), "got normal pos %s\n",
        wine_dbgstr_rect(&wp.rcNormalPosition));
    GetWindowRect(hwnd, &rect);
    ok(EqualRect(&rect, &orig), "got window rect %s\n", wine_dbgstr_rect(&rect));

    ret = SetWindowPlacement(hwnd, &wp);
    ok(ret, "failed to set window placement, error %lu\n", GetLastError());

    wp.length = 0;
    SetLastError(0xdeadbeef);
    ret = SetWindowPlacement(hwnd, &wp);
    ok(!ret, "SetWindowPlacement should have failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    DestroyWindow(hwnd);
}

static void test_arrange_iconic_windows(void)
{
    MINIMIZEDMETRICS mm = {sizeof(mm)}, oldmm = {sizeof(oldmm)};
    RECT orig = {100, 200, 300, 400}, rect, expect, parent_rect;
    POINT pt = {0};
    HWND parent, hwnds[10];
    UINT ret;
    int i, row, col;

    parent = CreateWindowA("MainWindowClass", "parent", WS_OVERLAPPEDWINDOW,
            100, 200, 500, 300, NULL, 0, 0, NULL);
    ok(!!parent, "failed to create window, error %lu\n", GetLastError());

    GetClientRect(parent, &parent_rect);
    ClientToScreen(parent, &pt);
    SystemParametersInfoA(SPI_GETMINIMIZEDMETRICS, sizeof(oldmm), &oldmm, 0);

    mm.iWidth = 100;
    mm.iHorzGap = 40;
    mm.iVertGap = 3;
    mm.iArrange = ARW_TOPLEFT | ARW_RIGHT;
    ret = SystemParametersInfoA(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);
    ok(ret, "failed to set minimized metrics, error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ArrangeIconicWindows(parent);
    ok(!ret, "wrong ret %u\n", ret);
    ok(GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        hwnds[i] = CreateWindowA("MainWindowClass", "child", WS_CHILD |
            WS_OVERLAPPEDWINDOW, orig.left, orig.top, orig.right - orig.left,
            orig.bottom - orig.top, parent, 0, 0, NULL);
        ok(!!hwnds[i], "failed to create window %u, error %lu\n", i, GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = ArrangeIconicWindows(parent);
    ok(!ret, "wrong ret %u\n", ret);
    ok(GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        GetWindowRect(hwnds[i], &rect);
        expect = orig;
        OffsetRect(&expect, pt.x, pt.y);
        ok(EqualRect(&rect, &expect), "hwnd %u: expected rect %s, got %s\n", i,
            wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    }

    ShowWindow(hwnds[0], SW_MINIMIZE);

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        ret = SetWindowPos(hwnds[i], 0, orig.left, orig.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        ok(ret, "hwnd %u: failed to move window, error %lu\n", i, GetLastError());
    }

    ret = ArrangeIconicWindows(parent);
    ok(ret == 1, "wrong ret %u\n", ret);

    GetWindowRect(hwnds[0], &rect);
    SetRect(&expect, 0, 0, GetSystemMetrics(SM_CXMINIMIZED), GetSystemMetrics(SM_CYMINIMIZED));
    OffsetRect(&expect, mm.iHorzGap, mm.iVertGap);
    OffsetRect(&expect, pt.x, pt.y);
    ok(EqualRect(&rect, &expect), "expected rect %s, got %s\n",
        wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

    for (i = 1; i < ARRAY_SIZE(hwnds); ++i)
    {
        GetWindowRect(hwnds[i], &rect);
        expect = orig;
        OffsetRect(&expect, pt.x, pt.y);
        ok(EqualRect(&rect, &expect), "hwnd %u: expected rect %s, got %s\n", i,
            wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));
    }

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        ShowWindow(hwnds[i], SW_MINIMIZE);
        ret = SetWindowPos(hwnds[i], 0, orig.left, orig.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        ok(ret, "hwnd %u: failed to move window, error %lu\n", i, GetLastError());
    }

    ret = ArrangeIconicWindows(parent);
    ok(ret == 10, "wrong ret %u\n", ret);

    col = mm.iHorzGap;
    row = mm.iVertGap;
    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        if (col + GetSystemMetrics(SM_CXMINIMIZED) > parent_rect.right - parent_rect.left)
        {
            col = mm.iHorzGap;
            row += GetSystemMetrics(SM_CYMINIMIZED) + mm.iVertGap;
        }

        GetWindowRect(hwnds[i], &rect);
        SetRect(&expect, col, row, col + GetSystemMetrics(SM_CXMINIMIZED),
            row + GetSystemMetrics(SM_CYMINIMIZED));
        OffsetRect(&expect, pt.x, pt.y);
        ok(EqualRect(&rect, &expect), "hwnd %u: expected rect %s, got %s\n", i,
            wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

        col += GetSystemMetrics(SM_CXMINIMIZED) + mm.iHorzGap;
    }

    mm.iArrange = ARW_BOTTOMRIGHT | ARW_UP;
    mm.iVertGap = 10;
    ret = SystemParametersInfoA(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);
    ok(ret, "failed to set minimized metrics, error %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        ret = SetWindowPos(hwnds[i], 0, orig.left, orig.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        ok(ret, "hwnd %u: failed to move window, error %lu\n", i, GetLastError());
    }

    ret = ArrangeIconicWindows(parent);
    ok(ret == 10, "wrong ret %u\n", ret);

    col = parent_rect.right - mm.iHorzGap;
    row = parent_rect.bottom - mm.iVertGap;
    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
    {
        if (row - GetSystemMetrics(SM_CYMINIMIZED) < parent_rect.top)
        {
            row = parent_rect.bottom - mm.iVertGap;
            col -= GetSystemMetrics(SM_CXMINIMIZED) + mm.iHorzGap;
        }

        GetWindowRect(hwnds[i], &rect);
        SetRect(&expect, col - GetSystemMetrics(SM_CXMINIMIZED),
            row - GetSystemMetrics(SM_CYMINIMIZED), col, row);
        OffsetRect(&expect, pt.x, pt.y);
        ok(EqualRect(&rect, &expect), "hwnd %u: expected rect %s, got %s\n", i,
            wine_dbgstr_rect(&expect), wine_dbgstr_rect(&rect));

        row -= GetSystemMetrics(SM_CYMINIMIZED) + mm.iVertGap;
    }

    for (i = 0; i < ARRAY_SIZE(hwnds); ++i)
        DestroyWindow(hwnds[i]);
    DestroyWindow(parent);

    ret = SystemParametersInfoA(SPI_SETMINIMIZEDMETRICS, sizeof(oldmm), &oldmm, 0);
    ok(ret, "failed to restore minimized metrics, error %lu\n", GetLastError());
}

static void other_process_proc(HWND hwnd)
{
    HANDLE window_ready_event, test_done_event;
    WINDOWPLACEMENT wp = {0};
    DWORD ret;

    window_ready_event = OpenEventA(EVENT_ALL_ACCESS, FALSE, "test_opw_window");
    ok(!!window_ready_event, "OpenEvent failed.\n");
    test_done_event = OpenEventA(EVENT_ALL_ACCESS, FALSE, "test_opw_test");
    ok(!!test_done_event, "OpenEvent failed.\n");

    /* SW_SHOW */
    ret = WaitForSingleObject(window_ready_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %lx.\n", ret);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "Unexpected ret %#lx.\n", ret);
    ok(wp.showCmd == SW_SHOWNORMAL, "Unexpected showCmd %#x.\n", wp.showCmd);
    ok(!wp.flags, "Unexpected flags %#x.\n", wp.flags);
    SetEvent(test_done_event);

    /* SW_SHOWMAXIMIZED */
    ret = WaitForSingleObject(window_ready_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %lx.\n", ret);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "Unexpected ret %#lx.\n", ret);
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "Unexpected showCmd %#x.\n", wp.showCmd);
    todo_wine ok(wp.flags == WPF_RESTORETOMAXIMIZED, "Unexpected flags %#x.\n", wp.flags);
    SetEvent(test_done_event);

    /* SW_SHOWMINIMIZED */
    ret = WaitForSingleObject(window_ready_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %lx.\n", ret);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "Unexpected ret %#lx.\n", ret);
    ok(wp.showCmd == SW_SHOWMINIMIZED, "Unexpected showCmd %#x.\n", wp.showCmd);
    todo_wine ok(wp.flags == WPF_RESTORETOMAXIMIZED, "Unexpected flags %#x.\n", wp.flags);
    SetEvent(test_done_event);

    /* SW_RESTORE */
    ret = WaitForSingleObject(window_ready_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %lx.\n", ret);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "Unexpected ret %#lx.\n", ret);
    ok(wp.showCmd == SW_SHOWMAXIMIZED, "Unexpected showCmd %#x.\n", wp.showCmd);
    todo_wine ok(wp.flags == WPF_RESTORETOMAXIMIZED, "Unexpected flags %#x.\n", wp.flags);
    SetEvent(test_done_event);

    CloseHandle(window_ready_event);
    CloseHandle(test_done_event);
}

static void test_SC_SIZE(void)
{
    HWND hwnd;
    RECT rect;
    MSG msg;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                100, 100, 100, 100, 0, 0, NULL, NULL);
    ok(!!hwnd, "CreateWindowEx failed.\n");

    GetWindowRect(hwnd, &rect);
    ok(rect.left == 100, "rect.left = %ld\n", rect.left);
    ok(rect.top == 100, "rect.top = %ld\n", rect.top);
    ok(rect.right == 200, "rect.right = %ld\n", rect.right);
    ok(rect.bottom == 200, "rect.bottom = %ld\n", rect.bottom);

    SetCursorPos(100, 100);
    PostMessageA(hwnd, WM_SYSCOMMAND, SC_SIZE | 9, MAKELONG(100, 100));
    SetCursorPos(110, 100);
    PostMessageA(hwnd, WM_MOUSEMOVE, 0, MAKELONG(110, 100));
    PostMessageA(hwnd, WM_KEYDOWN, VK_RETURN, 0);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if (msg.message == WM_SYSCOMMAND) break;
    }

    GetWindowRect(hwnd, &rect);
    ok(rect.left == 110, "rect.left = %ld\n", rect.left);
    ok(rect.top == 100, "rect.top = %ld\n", rect.top);
    ok(rect.right == 210, "rect.right = %ld\n", rect.right);
    ok(rect.bottom == 200, "rect.bottom = %ld\n", rect.bottom);

    DestroyWindow(hwnd);
}

static void test_other_process_window(const char *argv0)
{
    HANDLE window_ready_event, test_done_event;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char cmd[MAX_PATH];
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
            100, 100, 100, 100, 0, 0, NULL, NULL);
    ok(!!hwnd, "CreateWindowEx failed.\n");

    window_ready_event = CreateEventA(NULL, FALSE, FALSE, "test_opw_window");
    ok(!!window_ready_event, "CreateEvent failed.\n");
    test_done_event = CreateEventA(NULL, FALSE, FALSE, "test_opw_test");
    ok(!!test_done_event, "CreateEvent failed.\n");

    sprintf(cmd, "%s win test_other_process_window %p", argv0, hwnd);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);

    ok(CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL,
            &startup, &info), "CreateProcess failed.\n");

    ret = ShowWindow(hwnd, SW_SHOW);
    ok(!ret, "Unexpected ret %#x.\n", ret);
    SetEvent(window_ready_event);
    ret = WaitForSingleObject(test_done_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %x.\n", ret);

    ret = ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ok(ret, "Unexpected ret %#x.\n", ret);
    SetEvent(window_ready_event);
    ret = WaitForSingleObject(test_done_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %x.\n", ret);

    ret = ShowWindow(hwnd, SW_SHOWMINIMIZED);
    ok(ret, "Unexpected ret %#x.\n", ret);
    SetEvent(window_ready_event);
    ret = WaitForSingleObject(test_done_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %x.\n", ret);

    ret = ShowWindow(hwnd, SW_RESTORE);
    ok(ret, "Unexpected ret %#x.\n", ret);
    SetEvent(window_ready_event);
    ret = WaitForSingleObject(test_done_event, 5000);
    ok(ret == WAIT_OBJECT_0, "Unexpected ret %x.\n", ret);

    wait_child_process(info.hProcess);
    CloseHandle(window_ready_event);
    CloseHandle(test_done_event);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
    DestroyWindow(hwnd);
}

static void test_cancel_mode(void)
{
    HWND hwnd1, hwnd2, child;
    LRESULT ret;

    hwnd1 = CreateWindowA("MainWindowClass", "window 1", WS_OVERLAPPEDWINDOW,
            100, 200, 500, 300, NULL, NULL, NULL, NULL);
    hwnd2 = CreateWindowA("MainWindowClass", "window 2", WS_OVERLAPPEDWINDOW,
            100, 200, 500, 300, NULL, NULL, NULL, NULL);
    flush_events(TRUE);
    SetCapture(hwnd1);
    ok(GetCapture() == hwnd1, "got capture %p\n", GetCapture());

    ret = SendMessageA(hwnd2, WM_CANCELMODE, 0, 0);
    ok(!ret, "got %Id\n", ret);
    ok(GetCapture() == hwnd1, "got capture %p\n", GetCapture());

    ret = SendMessageA(hwnd1, WM_CANCELMODE, 0, 0);
    ok(!ret, "got %Id\n", ret);
    ok(!GetCapture(), "got capture %p\n", GetCapture());

    child = CreateWindowA("MainWindowClass", "child", WS_CHILD,
            0, 0, 100, 100, hwnd1, NULL, NULL, NULL);

    SetCapture(child);
    ok(GetCapture() == child, "got capture %p\n", GetCapture());

    ret = SendMessageA(hwnd2, WM_CANCELMODE, 0, 0);
    ok(!ret, "got %Id\n", ret);
    ok(GetCapture() == child, "got capture %p\n", GetCapture());

    ret = SendMessageA(hwnd1, WM_CANCELMODE, 0, 0);
    ok(!ret, "got %Id\n", ret);
    ok(GetCapture() == child, "got capture %p\n", GetCapture());

    ret = SendMessageA(child, WM_CANCELMODE, 0, 0);
    ok(!ret, "got %Id\n", ret);
    ok(!GetCapture(), "got capture %p\n", GetCapture());

    DestroyWindow(child);
    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
}

static void test_DragDetect(void)
{
    int cx_drag = GetSystemMetrics( SM_CXDRAG ), cy_drag = GetSystemMetrics( SM_CYDRAG );
    HWND hwnd;
    POINT pt;
    BOOL ret;

    ok(!GetCapture(), "got capture window %p\n", GetCapture());
    ok(!(GetKeyState( VK_LBUTTON ) & 0x8000), "got VK_LBUTTON\n");

    GetCursorPos(&pt);
    ret = DragDetect(hwndMain, pt);
    ok(!ret, "got %d\n", ret);

    ok(!GetCapture(), "got capture window %p\n", GetCapture());
    ok(!(GetKeyState( VK_LBUTTON ) & 0x8000), "got VK_LBUTTON\n");

    /* Test mouse moving out of the drag rectangle in client coordinates */
    hwnd = CreateWindowA( "static", "test", WS_POPUP | WS_VISIBLE, 100, 100, 50, 50, 0, 0, 0, 0 );
    hold_key( VK_LBUTTON );

    PostMessageA( hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(cx_drag, cy_drag) );

    pt.x = 0;
    pt.y = 0;
    ret = DragDetect( hwnd, pt );
    ok(ret, "Got unexpected %d.\n", ret);
    flush_events( TRUE );

    /* Test mouse not moving out of the drag rectangle in client coordinates */
    PostMessageA( hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(cx_drag - 1, cy_drag - 1) );
    PostMessageA( hwnd, WM_LBUTTONUP, 0, 0 );

    pt.x = 0;
    pt.y = 0;
    ret = DragDetect( hwnd, pt );
    ok(!ret || broken(ret) /* Win 7 */, "Got unexpected %d\n", ret);
    flush_events( TRUE );

    /* Test mouse moving out of the drag rectangle in screen coordinates */
    PostMessageA( hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(cx_drag, cy_drag) );

    pt.x = 0;
    pt.y = 0;
    ClientToScreen( hwnd, &pt );
    ret = DragDetect( hwnd, pt );
    ok(ret || broken(!ret) /* Win 7 */, "Got unexpected %d\n", ret);
    flush_events( TRUE );

    /* Test mouse not moving out of the drag rectangle in screen coordinates */
    PostMessageA( hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(0, 0) );

    pt.x = 0;
    pt.y = 0;
    ClientToScreen( hwnd, &pt );
    ret = DragDetect( hwnd, pt );
    /* NOTE that if DragDetect() do use screen coordinates for the second parameter for the initial
     * mouse position, then FALSE should be returned in this case. But TRUE is returned here. So
     * this means that DragDetect() don't use screen coordinates even though MSDN says that it does */
    ok(ret, "Got unexpected %d\n", ret);

    DestroyWindow( hwnd );
    release_key( VK_LBUTTON );
}

static LRESULT WINAPI ncdestroy_test_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    unsigned int ret;
    HWND parent, child;
    RECT rect, exp;

    switch (msg)
    {
    case WM_NCDESTROY:
        SetLastError( 0xdeadbeef );
        ret = SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 100, 100, SWP_NOSIZE|SWP_NOMOVE );
        todo_wine
        ok( !ret, "SetWindowPos succeeded\n" );
        todo_wine
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "SetWindowPos returned error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        parent = SetParent( hwnd, hwndMain );
        ok( parent == 0, "SetParent returned %p\n", parent );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

        ret = GetWindowRect( hwnd, &rect );
        ok( ret, "GetWindowRect failed: %lu\n", GetLastError() );
        SetRect( &exp, 10, 20, 110, 220 );
        ok( EqualRect( &rect, &exp ), "unexpected rect %s, expected %s\n", wine_dbgstr_rect( &rect ),
            wine_dbgstr_rect( &exp ));

        ret = MoveWindow( hwnd, 11, 12, 20, 30, FALSE );
        ok( ret, "MoveWindow failed: %lu\n", GetLastError() );
        ret = GetWindowRect( hwnd, &rect );
        ok( ret, "GetWindowRect failed: %lu\n", GetLastError() );
        SetRect( &exp, 11, 12, 31, 42 );
        ok( EqualRect( &rect, &exp ), "unexpected rect %s, expected %s\n", wine_dbgstr_rect( &rect ),
            wine_dbgstr_rect( &exp ));

        child = CreateWindowExA( 0, "ToolWindowClass", "Tool window 1", WS_CHILD,
                                 0, 0, 100, 100, hwnd, 0, 0, NULL );
        ok( !child && GetLastError() == ERROR_INVALID_PARAMETER,
            "CreateWindowExA returned %p %lu\n", child, GetLastError() );
        break;
    }

    return DefWindowProcW( hwnd, msg, wp, lp );
}

static void test_ncdestroy(void)
{
    HWND hwnd;
    hwnd = create_tool_window( WS_POPUP, 0 );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)ncdestroy_test_proc );
    MoveWindow( hwnd, 10, 20, 100, 200, FALSE );
    DestroyWindow(hwnd);
}

static void test_WM_NCCALCSIZE(void)
{
    WNDCLASSA cls;
    HWND hwnd;
    NCCALCSIZE_PARAMS params;
    WINDOWPOS winpos;
    RECT client_rect, window_rect;
    LRESULT ret;

    cls.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "dummy_window_class";
    ret = RegisterClassA(&cls);
    ok(ret, "RegisterClass error %lu\n", GetLastError());

    hwnd = CreateWindowExA(0, "dummy_window_class", NULL,
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP,
                           100, 100, 200, 200, 0, 0, GetModuleHandleA(NULL), NULL);
    ok(hwnd != 0, "CreateWindowEx error %lu\n", GetLastError());

    GetWindowRect(hwnd, &window_rect);
    params.rgrc[0] = window_rect;
    params.rgrc[1] = window_rect;
    GetClientRect(hwnd, &client_rect);
    MapWindowPoints(hwnd, 0, (POINT *)&client_rect, 2);
    params.rgrc[2] = client_rect;

    winpos.hwnd = hwnd;
    winpos.hwndInsertAfter = HWND_TOP;
    winpos.x = window_rect.left;
    winpos.y = window_rect.top;
    winpos.cx = window_rect.right - window_rect.left;
    winpos.cy = window_rect.bottom - window_rect.top;
    winpos.flags = SWP_NOMOVE;
    params.lppos = &winpos;

    ret = SendMessageW(hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&params.rgrc[0], &client_rect), "got %s\n", wine_dbgstr_rect(&params.rgrc[0]));

    params.rgrc[0] = window_rect;
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&params.rgrc[0], &client_rect), "got %s\n", wine_dbgstr_rect(&params.rgrc[0]));

    GetWindowRect(hwnd, &window_rect);
    ret = SendMessageW(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&window_rect);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&window_rect, &client_rect), "got %s\n", wine_dbgstr_rect(&window_rect));

    GetWindowRect(hwnd, &window_rect);
    ret = DefWindowProcA(hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&window_rect);
    ok(!ret, "got %08Ix\n", ret);
    ok(EqualRect(&window_rect, &client_rect), "got %s\n", wine_dbgstr_rect(&window_rect));

    DestroyWindow(hwnd);
}

#define TRAY_MINIMIZE_ALL 419
#define TRAY_MINIMIZE_ALL_UNDO 416

static void test_shell_tray(void)
{
    HWND hwnd, traywnd;

    if (!(traywnd = FindWindowA( "Shell_TrayWnd", NULL )))
    {
        skip( "Shell_TrayWnd not found, skipping tests.\n" );
        return;
    }

    hwnd = CreateWindowW( L"static", L"parent", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                  100, 100, 200, 200, 0, 0, 0, NULL );
    ok( !!hwnd, "failed, error %lu.\n", GetLastError() );
    flush_events( TRUE );

    ok( !IsIconic( hwnd ), "window is minimized.\n" );

    SendMessageA( traywnd, WM_COMMAND, TRAY_MINIMIZE_ALL, 0xdeadbeef );
    flush_events( TRUE );
    todo_wine ok( IsIconic( hwnd ), "window is not minimized.\n" );

    SendMessageA( traywnd, WM_COMMAND, TRAY_MINIMIZE_ALL_UNDO, 0xdeadbeef );
    flush_events( TRUE );
    ok( !IsIconic( hwnd ), "window is minimized.\n" );

    DestroyWindow(hwnd);
}

static int wm_mousemove_count;
static BOOL do_release_capture;

static LRESULT WINAPI test_ReleaseCapture_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_MOUSEMOVE)
    {
        wm_mousemove_count++;
        if (wm_mousemove_count >= 100)
            return 1;

        if (do_release_capture)
            ReleaseCapture();
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void test_ReleaseCapture(void)
{
    WNDCLASSA cls = {0};
    ATOM atom;
    HWND hwnd;
    POINT pt;
    BOOL ret;

    cls.lpfnWndProc = test_ReleaseCapture_proc;
    cls.hInstance = GetModuleHandleA(0);
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(BLACK_BRUSH);
    cls.lpszClassName = "test_ReleaseCapture_class";
    atom = RegisterClassA(&cls);
    ok(!!atom, "RegisterClassA failed, error %#lx.\n", GetLastError());

    hwnd = CreateWindowExA(WS_EX_TOPMOST, cls.lpszClassName, "", WS_POPUP | WS_VISIBLE, 100, 100,
                           100, 100, NULL, NULL, 0, NULL);
    ok(!!hwnd, "CreateWindowA failed, error %#lx.\n", GetLastError());
    ret = SetForegroundWindow(hwnd);
    ok(ret, "SetForegroundWindow failed, error %#lx.\n", GetLastError());
    GetCursorPos(&pt);
    ret = SetCursorPos(150, 150);
    ok(ret, "SetCursorPos failed, error %#lx.\n", GetLastError());
    flush_events(TRUE);

    /* Test a Wine bug that WM_MOUSEMOVE is post too many times when calling ReleaseCapture() during
     * handling WM_MOUSEMOVE and the cursor is on the window */
    do_release_capture = TRUE;
    ret = ReleaseCapture();
    ok(ret, "ReleaseCapture failed, error %#lx.\n", GetLastError());
    flush_events(TRUE);
    do_release_capture = FALSE;
    ok(wm_mousemove_count < 10, "Got too many WM_MOUSEMOVE.\n");

    /* Test that ReleaseCapture() should send a WM_MOUSEMOVE if a window is captured */
    SetCapture(hwnd);
    wm_mousemove_count = 0;
    ret = ReleaseCapture();
    ok(ret, "ReleaseCapture failed, error %#lx.\n", GetLastError());
    flush_events(TRUE);
    ok(wm_mousemove_count == 1, "Got no WM_MOUSEMOVE.\n");

    /* Test that ReleaseCapture() shouldn't send WM_MOUSEMOVE if no window is captured */
    wm_mousemove_count = 0;
    ret = ReleaseCapture();
    ok(ret, "ReleaseCapture failed, error %#lx.\n", GetLastError());
    flush_events(TRUE);
    ok(wm_mousemove_count == 0, "Got WM_MOUSEMOVE.\n");

    ret = SetCursorPos(pt.x, pt.y);
    ok(ret, "SetCursorPos failed, error %#lx.\n", GetLastError());
    DestroyWindow(hwnd);
    UnregisterClassA(cls.lpszClassName, GetModuleHandleA(0));
}

START_TEST(win)
{
    char **argv;
    int argc = winetest_get_mainargs( &argv );
    HMODULE user32 = GetModuleHandleA( "user32.dll" );
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
    pGetWindowInfo = (void *)GetProcAddress( user32, "GetWindowInfo" );
    pGetWindowModuleFileNameA = (void *)GetProcAddress( user32, "GetWindowModuleFileNameA" );
    pGetLayeredWindowAttributes = (void *)GetProcAddress( user32, "GetLayeredWindowAttributes" );
    pSetLayeredWindowAttributes = (void *)GetProcAddress( user32, "SetLayeredWindowAttributes" );
    pUpdateLayeredWindow = (void *)GetProcAddress( user32, "UpdateLayeredWindow" );
    pUpdateLayeredWindowIndirect = (void *)GetProcAddress( user32, "UpdateLayeredWindowIndirect" );
    pGetWindowRgnBox = (void *)GetProcAddress( user32, "GetWindowRgnBox" );
    pGetGUIThreadInfo = (void *)GetProcAddress( user32, "GetGUIThreadInfo" );
    pGetProcessDefaultLayout = (void *)GetProcAddress( user32, "GetProcessDefaultLayout" );
    pSetProcessDefaultLayout = (void *)GetProcAddress( user32, "SetProcessDefaultLayout" );
    pFlashWindow = (void *)GetProcAddress( user32, "FlashWindow" );
    pFlashWindowEx = (void *)GetProcAddress( user32, "FlashWindowEx" );
    pMirrorRgn = (void *)GetProcAddress( gdi32, "MirrorRgn" );
    pGetWindowDisplayAffinity = (void *)GetProcAddress( user32, "GetWindowDisplayAffinity" );
    pSetWindowDisplayAffinity = (void *)GetProcAddress( user32, "SetWindowDisplayAffinity" );
    pAdjustWindowRectExForDpi = (void *)GetProcAddress( user32, "AdjustWindowRectExForDpi" );
    pSystemParametersInfoForDpi = (void *)GetProcAddress( user32, "SystemParametersInfoForDpi" );
    pInternalGetWindowIcon = (void *)GetProcAddress( user32, "InternalGetWindowIcon" );

    if (argc == 4)
    {
        HWND hwnd;

        sscanf(argv[3], "%p", &hwnd);
        if (!strcmp(argv[2], "create_children"))
        {
            window_from_point_proc(hwnd);
            return;
        }
        else if (!strcmp(argv[2], "test_other_process_window"))
        {
            other_process_proc(hwnd);
            return;
        }
    }

    if (argc == 3 && !strcmp(argv[2], "winproc_limit"))
    {
        test_winproc_limit();
        return;
    }

    if (argc == 4 && !strcmp( argv[2], "SetActiveWindow_0" ))
    {
        test_SetActiveWindow_0_proc( argv );
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

    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    if (!hhook) win_skip( "Cannot set CBT hook, skipping some tests\n" );

    /* make sure that these tests are executed first */
    test_FindWindowEx();
    test_FindWindow();
    test_SetParent();

    hwndMain2 = CreateWindowExA(/*WS_EX_TOOLWINDOW*/ 0, "MainWindowClass", "Main window 2",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_POPUP,
                                100, 100, 200, 200,
                                0, 0, GetModuleHandleA(NULL), NULL);
    assert( hwndMain2 );

    our_pid = GetWindowThreadProcessId(hwndMain, NULL);

    /* This test is sensitive to other windows created with
     * CW_USEDEFAULT position, execute it early too */
    test_mdi();

    /* Add the tests below this line */
    test_child_window_from_point();
    test_window_from_point(hwndMain, argv[0]);
    test_thick_child_size(hwndMain);
    test_fullscreen();
    test_hwnd_message();
    test_message_window_topmost();
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
    test_thread_exit_destroy();
    test_ncdestroy();

#ifdef __REACTOS__
    if (!(GetVersion() & 0x05))
#endif
    test_icons();
    test_SetWindowPos(hwndMain, hwndMain2);
    test_SetMenu(hwndMain);
    test_SetFocus(hwndMain);
    test_SetActiveWindow_0( argv );
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
    test_ShowWindow_owned(hwndMain);
    test_ShowWindow_child(hwndMain);
    test_ShowWindow_mdichild(hwndMain);
    test_EnableWindow();
    test_gettext();
    test_GetUpdateRect();
    test_Expose();
    test_layered_window();

    test_SetForegroundWindow(hwndMain);
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
    test_LockWindowUpdate(hwndMain);
    test_desktop();
    test_display_affinity(hwndMain);
    test_hide_window();
    test_minimize_window(hwndMain);
    test_destroy_quit();
    test_IsWindowEnabled();
    test_window_placement();
    test_arrange_iconic_windows();
    test_other_process_window(argv[0]);
    test_SC_SIZE();
    test_cancel_mode();
    test_DragDetect();
    test_WM_NCCALCSIZE();
    test_ReleaseCapture();

    /* add the tests above this line */
    if (hhook) UnhookWindowsHookEx(hhook);

    DestroyWindow(hwndMain2);
    DestroyWindow(hwndMain);

    /* Make sure that following tests are executed last, under Windows they
     * tend to break the tests which are sensitive to z-order and activation
     * state of hwndMain and hwndMain2 windows.
     */
    test_topmost();

    test_shell_window();
    test_shell_tray();
}
