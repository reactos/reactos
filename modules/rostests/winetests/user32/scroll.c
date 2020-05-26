/*
 * Unit tests for scrollbar
 *
 * Copyright 2008 Lyutin Anatoly (Etersoft)
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
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#include "wine/test.h"

static HWND hScroll;
static BOOL bThemeActive = FALSE;

static LRESULT CALLBACK MyWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg)
    {

    case WM_CREATE:
    {
        hScroll = CreateWindowA( "SCROLLBAR", "", WS_CHILD | WS_VISIBLE, 0, 0, 120, 100, hWnd, (HMENU)100, GetModuleHandleA(0), 0 );

        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        /* stop tracking */
        ReleaseCapture();
        return 0;
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static HWND create_main_test_wnd(void)
{
    HWND hMainWnd;

    hScroll = NULL;
    hMainWnd = CreateWindowExA( 0, "MyTestWnd", "Scroll",
      WS_OVERLAPPEDWINDOW|WS_VSCROLL|WS_HSCROLL,
      CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0 );
    ok(hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n");
    ok(hScroll != NULL, "got NULL scroll bar handle\n");

    return hMainWnd;
}

static void scrollbar_test_track(void)
{
    HWND mainwnd;

    mainwnd = create_main_test_wnd();

    /* test that scrollbar tracking is terminated when
     * the control loses mouse capture */
    SendMessageA( hScroll, WM_LBUTTONDOWN, 0, MAKELPARAM( 1, 1));
    /* a normal return from SendMessage */
    /* not normal for instances such as closing the window */
    ok( IsWindow( hScroll), "Scrollbar has gone!\n");

    DestroyWindow(hScroll);
    DestroyWindow(mainwnd);
}

static void test_EnableScrollBar(void)
{
    HWND mainwnd;
    BOOL ret;

    mainwnd = create_main_test_wnd();

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_DISABLE_BOTH );
    ok( ret, "The scrollbar should be disabled.\n" );
    ok( !IsWindowEnabled( hScroll ), "The scrollbar window should be disabled.\n" );

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_ENABLE_BOTH );
    ok( ret, "The scrollbar should be enabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );

    /* test buttons separately */
    ret = EnableScrollBar( hScroll, SB_CTL, ESB_DISABLE_LTUP );
    ok( ret, "The scrollbar LTUP button should be disabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );
    ret = EnableScrollBar( hScroll, SB_CTL, ESB_ENABLE_BOTH );
    ok( ret, "The scrollbar should be enabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_DISABLE_RTDN );
    ok( ret, "The scrollbar RTDN button should be disabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );
    ret = EnableScrollBar( hScroll, SB_CTL, ESB_ENABLE_BOTH );
    ok( ret, "The scrollbar should be enabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );

    SetLastError( 0xdeadbeef );
    ret = EnableScrollBar( mainwnd, SB_CTL, ESB_ENABLE_BOTH );
    ok( !ret, "EnableScrollBar should fail.\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_PARAMETER
        || broken(GetLastError() == 0xdeadbeef), /* winxp */
        "GetLastError() = %u\n", GetLastError() );

    /* disable window, try to re-enable */
    ret = EnableWindow( hScroll, FALSE );
    ok( !ret, "got %d\n", ret );
    ok( !IsWindowEnabled( hScroll ), "The scrollbar window should be disabled.\n" );

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_ENABLE_BOTH );
    ok( ret, "got %d\n", ret );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be disabled.\n" );

    DestroyWindow(hScroll);
    DestroyWindow(mainwnd);
}

static void test_SetScrollPos(void)
{
    HWND mainwnd;
    int ret;

    mainwnd = create_main_test_wnd();

    EnableWindow( hScroll, FALSE );
    ok( !IsWindowEnabled( hScroll ), "The scroll should be disabled.\n" );

    ret = SetScrollPos( hScroll, SB_CTL, 30, TRUE);
    ok( !ret, "The position should not be set.\n" );

    ret = GetScrollPos( hScroll, SB_CTL);
    ok( !ret, "The position should be equal to zero\n");

    ret = SetScrollRange( hScroll, SB_CTL, 0, 100, TRUE );
    ok( ret, "The range should be set.\n" );

    ret = SetScrollPos( hScroll, SB_CTL, 30, TRUE);
    ok( !ret , "The position should not be set.\n" );

    ret = GetScrollPos( hScroll, SB_CTL);
    ok( ret == 30, "The position should be set!!!\n");

    EnableWindow( hScroll, TRUE );
    ok( IsWindowEnabled( hScroll ), "The scroll should be enabled.\n" );

    ret = SetScrollPos( hScroll, SB_CTL, 30, TRUE);
    ok( ret == 30, "The position should be set.\n" );

    ret = GetScrollPos( hScroll, SB_CTL);
    ok( ret == 30, "The position should not be equal to zero\n");

    ret = SetScrollRange( hScroll, SB_CTL, 0, 100, TRUE );
    ok( ret, "The range should be set.\n" );

    ret = SetScrollPos( hScroll, SB_CTL, 30, TRUE);
    ok( ret == 30, "The position should be set.\n" );

    ret = GetScrollPos( hScroll, SB_CTL);
    ok( ret == 30, "The position should not be equal to zero\n");

    SetLastError( 0xdeadbeef );
    ret = SetScrollPos( mainwnd, SB_CTL, 30, TRUE );
    ok( !ret, "The position should not be set.\n" );
    ok( GetLastError() == 0xdeadbeef, "GetLastError() = %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = GetScrollPos( mainwnd, SB_CTL );
    ok( !ret, "The position should be equal to zero\n");
    ok( GetLastError() == 0xdeadbeef, "GetLastError() = %u\n", GetLastError() );

    DestroyWindow(hScroll);
    DestroyWindow(mainwnd);
}

static void test_ShowScrollBar(void)
{
    HWND mainwnd;
    BOOL    ret;

    mainwnd = create_main_test_wnd();

    ret = ShowScrollBar( hScroll, SB_CTL, FALSE );
    ok( ret, "The ShowScrollBar() should not failed.\n" );
    ok( !IsWindowVisible( hScroll ), "The scrollbar window should not be visible\n" );

    ret = ShowScrollBar( hScroll, SB_CTL, TRUE );
    ok( ret, "The ShowScrollBar() should not failed.\n" );
    ok( !IsWindowVisible( hScroll ), "The scrollbar window should be visible\n" );

    ret = ShowScrollBar( NULL, SB_CTL, TRUE );
    ok( !ret, "The ShowScrollBar() should failed.\n" );

    ret = ShowScrollBar( mainwnd, SB_CTL, TRUE );
    ok( ret, "The ShowScrollBar() should not fail.\n" );

    DestroyWindow(hScroll);
    DestroyWindow(mainwnd);
}

static void test_GetScrollBarInfo(void)
{
    HWND hMainWnd;
    BOOL ret;
    SCROLLBARINFO sbi;
    RECT rect;
    BOOL (WINAPI *pGetScrollBarInfo)(HWND, LONG, LPSCROLLBARINFO);

    pGetScrollBarInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetScrollBarInfo");
    if (!pGetScrollBarInfo)
    {
        win_skip("GetScrollBarInfo is not available\n");
        return;
    }

    hMainWnd = create_main_test_wnd();

    /* Test GetScrollBarInfo to make sure it returns rcScrollBar in screen
     * coordinates. */
    sbi.cbSize = sizeof(sbi);
    ret = pGetScrollBarInfo( hScroll, OBJID_CLIENT, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    GetWindowRect( hScroll, &rect );
    ok( ret, "The GetWindowRect() call should not fail.\n" );
    ok( !(sbi.rgstate[0] & (STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN)),
        "unexpected rgstate(0x%x)\n", sbi.rgstate[0]);
    ok(EqualRect(&rect, &sbi.rcScrollBar), "WindowRect %s != rcScrollBar %s\n",
       wine_dbgstr_rect(&rect), wine_dbgstr_rect(&sbi.rcScrollBar));

    /* Test windows horizontal and vertical scrollbar to make sure rcScrollBar
     * is still returned in screen coordinates by moving the window, and
     * making sure that it shifts the rcScrollBar value. */
    ShowWindow( hMainWnd, SW_SHOW );
    sbi.cbSize = sizeof(sbi);
    ret = pGetScrollBarInfo( hMainWnd, OBJID_HSCROLL, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    GetWindowRect( hMainWnd, &rect );
    ok( ret, "The GetWindowRect() call should not fail.\n" );
    MoveWindow( hMainWnd, rect.left+5, rect.top+5,
                rect.right-rect.left, rect.bottom-rect.top, TRUE );
    rect = sbi.rcScrollBar;
    OffsetRect(&rect, 5, 5);
    ret = pGetScrollBarInfo( hMainWnd, OBJID_HSCROLL, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    ok(EqualRect(&rect, &sbi.rcScrollBar), "PreviousRect %s != CurrentRect %s\n",
       wine_dbgstr_rect(&rect), wine_dbgstr_rect(&sbi.rcScrollBar));

    sbi.cbSize = sizeof(sbi);
    ret = pGetScrollBarInfo( hMainWnd, OBJID_VSCROLL, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    GetWindowRect( hMainWnd, &rect );
    ok( ret, "The GetWindowRect() call should not fail.\n" );
    MoveWindow( hMainWnd, rect.left+5, rect.top+5,
                rect.right-rect.left, rect.bottom-rect.top, TRUE );
    rect = sbi.rcScrollBar;
    OffsetRect(&rect, 5, 5);
    ret = pGetScrollBarInfo( hMainWnd, OBJID_VSCROLL, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    ok(EqualRect(&rect, &sbi.rcScrollBar), "PreviousRect %s != CurrentRect %s\n",
       wine_dbgstr_rect(&rect), wine_dbgstr_rect(&sbi.rcScrollBar));

    DestroyWindow(hScroll);
    DestroyWindow(hMainWnd);
}

/* some tests designed to show that Horizontal and Vertical
 * window scroll bar info are not created independently */
static void scrollbar_test_default( DWORD style)
{
    INT min, max, ret;
    DWORD winstyle;
    HWND hwnd;
    SCROLLINFO si = { sizeof( SCROLLINFO), SIF_TRACKPOS };

    hwnd = CreateWindowExA( 0, "static", "", WS_POPUP | style,
                0, 0, 10, 10, 0, 0, 0, NULL);
    assert( hwnd != 0);

    ret = GetScrollRange( hwnd, SB_VERT, &min, &max);
    ok( ret ||
            broken( !ret) /* Win 9x/ME */ , "GetScrollRange failed.\n");
    /* range is 0,0 if there are no H or V scroll bars. 0,100 otherwise */
    if( !( style & ( WS_VSCROLL | WS_HSCROLL)))
        ok( min == 0 && max == 0,
                "Scroll bar range is %d,%d. Expected 0,0. Style %08x\n", min, max, style);
    else
        ok(( min == 0 && max == 100) ||
                broken( min == 0 && max == 0), /* Win 9x/ME */
                "Scroll bar range is %d,%d. Expected 0,100. Style %08x\n", min, max, style);
    ret = GetScrollRange( hwnd, SB_HORZ, &min, &max);
    ok( ret ||
            broken( !ret) /* Win 9x/ME */ , "GetScrollRange failed.\n");
    /* range is 0,0 if there are no H or V scroll bars. 0,100 otherwise */
    if( !( style & ( WS_VSCROLL | WS_HSCROLL)))
        ok( min == 0 && max == 0,
                "Scroll bar range is %d,%d. Expected 0,0. Style %08x\n", min, max, style);
    else
        ok(( min == 0 && max == 100) ||
                broken( min == 0 && max == 0), /* Win 9x/ME */
                "Scroll bar range is %d,%d. Expected 0,100. Style %08x\n", min, max, style);
    /* test GetScrollInfo, vist for vertical SB */
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should fail if no H or V scroll bar styles are present. Succeed otherwise */
    if( !( style & ( WS_VSCROLL | WS_HSCROLL)))
        ok( !ret, "GetScrollInfo succeeded unexpectedly. Style is %08x\n", style);
    else
        ok( ret ||
                broken( !ret), /* Win 9x/ME */
                "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    /* Same for Horizontal SB */
    ret = GetScrollInfo( hwnd, SB_HORZ, &si);
    /* should fail if no H or V scroll bar styles are present. Succeed otherwise */
    if( !( style & ( WS_VSCROLL | WS_HSCROLL)))
        ok( !ret, "GetScrollInfo succeeded unexpectedly. Style is %08x\n", style);
    else
        ok( ret ||
                broken( !ret), /* Win 9x/ME */
                "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    /* now set the Vertical Scroll range to something that could be the default value it
     * already has */;
    ret = SetScrollRange( hwnd, SB_VERT, 0, 100, FALSE);
    ok( ret, "SetScrollRange failed.\n");
    /* and request the Horizontal range */
    ret = GetScrollRange( hwnd, SB_HORZ, &min, &max);
    ok( ret, "GetScrollRange failed.\n");
    /* now the range should be 0,100 in ALL cases */
    ok( min == 0 && max == 100,
            "Scroll bar range is %d,%d. Expected 0,100. Style %08x\n", min, max, style);
    /* See what is different now for GetScrollRange */
    ret = GetScrollInfo( hwnd, SB_HORZ, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    /* report the windows style */
    winstyle = GetWindowLongA( hwnd, GWL_STYLE );
    /* WS_VSCROLL added to the window style */
    if( !(style & WS_VSCROLL))
    {
        if (bThemeActive || style != WS_HSCROLL)
todo_wine
            ok( (winstyle & (WS_HSCROLL|WS_VSCROLL)) == ( style | WS_VSCROLL),
                "unexpected style change %08x/%08x\n", winstyle, style);
        else
            ok( (winstyle & (WS_HSCROLL|WS_VSCROLL)) == style,
                "unexpected style change %08x/%08x\n", winstyle, style);
    }
    /* do the test again with H and V reversed.
     * Start with a clean window */
    DestroyWindow( hwnd);
    hwnd = CreateWindowExA( 0, "static", "", WS_POPUP | style,
                0, 0, 10, 10, 0, 0, 0, NULL);
    assert( hwnd != 0);
    /* Set Horizontal Scroll range to something that could be the default value it
     * already has */;
    ret = SetScrollRange( hwnd, SB_HORZ, 0, 100, FALSE);
    ok( ret, "SetScrollRange failed.\n");
    /* and request the Vertical range */
    ret = GetScrollRange( hwnd, SB_VERT, &min, &max);
    ok( ret, "GetScrollRange failed.\n");
    /* now the range should be 0,100 in ALL cases */
    ok( min == 0 && max == 100,
            "Scroll bar range is %d,%d. Expected 0,100. Style %08x\n", min, max, style);
    /* See what is different now for GetScrollRange */
    ret = GetScrollInfo( hwnd, SB_HORZ, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    /* report the windows style */
    winstyle = GetWindowLongA( hwnd, GWL_STYLE );
    /* WS_HSCROLL added to the window style */
    if( !(style & WS_HSCROLL))
    {
        if (bThemeActive || style != WS_VSCROLL)
todo_wine
            ok( (winstyle & (WS_HSCROLL|WS_VSCROLL)) == ( style | WS_HSCROLL),
                "unexpected style change %08x/%08x\n", winstyle, style);
        else
            ok( (winstyle & (WS_HSCROLL|WS_VSCROLL)) == style,
                "unexpected style change %08x/%08x\n", winstyle, style);
    }
    /* Slightly change the test to use SetScrollInfo
     * Start with a clean window */
    DestroyWindow( hwnd);
    hwnd = CreateWindowExA( 0, "static", "", WS_POPUP | style,
                0, 0, 10, 10, 0, 0, 0, NULL);
    assert( hwnd != 0);
    /* set Horizontal position with SetScrollInfo */
    si.nPos = 0;
    si.nMin = 11;
    si.nMax = 22;
    si.fMask |= SIF_RANGE;
    ret = SetScrollInfo( hwnd, SB_HORZ, &si, FALSE);
    ok( ret, "SetScrollInfo failed. Style is %08x\n", style);
    /* and request the Vertical range */
    ret = GetScrollRange( hwnd, SB_VERT, &min, &max);
    ok( ret, "GetScrollRange failed.\n");
    /* now the range should be 0,100 in ALL cases */
    ok( min == 0 && max == 100,
            "Scroll bar range is %d,%d. Expected 0,100. Style %08x\n", min, max, style);
    /* See what is different now for GetScrollRange */
    ret = GetScrollInfo( hwnd, SB_HORZ, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should succeed in ALL cases */
    ok( ret, "GetScrollInfo failed unexpectedly. Style is %08x\n", style);
    /* also test if the window scroll bars are enabled */
    ret = EnableScrollBar( hwnd, SB_VERT, ESB_ENABLE_BOTH);
    ok( !ret, "Vertical window scroll bar was not enabled\n");
    ret = EnableScrollBar( hwnd, SB_HORZ, ESB_ENABLE_BOTH);
    ok( !ret, "Horizontal window scroll bar was not enabled\n");
    DestroyWindow( hwnd);
    /* finally, check if adding a WS_[HV]SCROLL style of a window makes the scroll info
     * available */
    if( style & (WS_HSCROLL | WS_VSCROLL)) return;/* only test if not yet set */
    /* Start with a clean window */
    DestroyWindow( hwnd);
    hwnd = CreateWindowExA( 0, "static", "", WS_POPUP ,
                0, 0, 10, 10, 0, 0, 0, NULL);
    assert( hwnd != 0);
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should fail */
    ok( !ret, "GetScrollInfo succeeded unexpectedly. Style is %08x\n", style);
    /* add scroll styles */
    winstyle = GetWindowLongA( hwnd, GWL_STYLE );
    SetWindowLongW( hwnd, GWL_STYLE, winstyle | WS_VSCROLL | WS_HSCROLL);
    ret = GetScrollInfo( hwnd, SB_VERT, &si);
    /* should still fail */
    ok( !ret, "GetScrollInfo succeeded unexpectedly. Style is %08x\n", style);
    /* clean up */
    DestroyWindow( hwnd);
}

static LRESULT CALLBACK scroll_init_proc(HWND hwnd, UINT msg,
                                        WPARAM wparam, LPARAM lparam)
{
    SCROLLINFO horz, vert;
    CREATESTRUCTA *cs = (CREATESTRUCTA *)lparam;
    BOOL h_ret, v_ret;

    switch(msg)
    {
        case WM_NCCREATE:
            return cs->lpCreateParams ? DefWindowProcA(hwnd, msg, wparam, lparam) :
                                        TRUE;

        case WM_CREATE:
            horz.cbSize = sizeof horz;
            horz.fMask  = SIF_ALL;
            horz.nMin   = 0xdeadbeaf;
            horz.nMax   = 0xbaadc0de;
            vert = horz;
            h_ret = GetScrollInfo(hwnd, SB_HORZ, &horz);
            v_ret = GetScrollInfo(hwnd, SB_VERT, &vert);

            if(cs->lpCreateParams)
            {
                /* WM_NCCREATE was passed to DefWindowProc */
                if(cs->style & (WS_VSCROLL | WS_HSCROLL))
                {
                    ok(h_ret && v_ret, "GetScrollInfo() should return NON-zero "
                            "but got h_ret=%d v_ret=%d\n", h_ret, v_ret);
                    ok(vert.nMin == 0 && vert.nMax == 100,
                            "unexpected init values(SB_VERT): min=%d max=%d\n",
                            vert.nMin, vert.nMax);
                    ok(horz.nMin == 0 && horz.nMax == 100,
                            "unexpected init values(SB_HORZ): min=%d max=%d\n",
                            horz.nMin, horz.nMax);
                }
                else
                {
                    ok(!h_ret && !v_ret, "GetScrollInfo() should return zeru, "
                            "but got h_ret=%d v_ret=%d\n", h_ret, v_ret);
                    ok(vert.nMin == 0xdeadbeaf && vert.nMax == 0xbaadc0de,
                            "unexpected  initialization(SB_VERT): min=%d max=%d\n",
                            vert.nMin, vert.nMax);
                    ok(horz.nMin == 0xdeadbeaf && horz.nMax == 0xbaadc0de,
                            "unexpected  initialization(SB_HORZ): min=%d max=%d\n",
                            horz.nMin, horz.nMax);
                }
            }
            else
            {
                ok(!h_ret && !v_ret, "GetScrollInfo() should return zeru, "
                    "but got h_ret=%d v_ret=%d\n", h_ret, v_ret);
                ok(horz.nMin == 0xdeadbeaf && horz.nMax == 0xbaadc0de &&
                    vert.nMin == 0xdeadbeaf && vert.nMax == 0xbaadc0de,
                        "unexpected initialization\n");
            }
            return FALSE; /* abort creation */

        default:
            /* need for WM_GETMINMAXINFO, which precedes WM_NCCREATE */
            return 0;
    }
}

static void scrollbar_test_init(void)
{
    WNDCLASSEXA wc;
    CHAR cls_name[] = "scroll_test_class";
    LONG style[] = {WS_VSCROLL, WS_HSCROLL, WS_VSCROLL | WS_HSCROLL, 0};
    int i;

    memset( &wc, 0, sizeof wc );
    wc.cbSize        = sizeof wc;
    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandleA(0);
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = cls_name;
    wc.lpfnWndProc   = scroll_init_proc;
    RegisterClassExA(&wc);

    for(i = 0; i < ARRAY_SIZE(style); i++)
    {
        /* need not to destroy these windows due creation abort */
        CreateWindowExA(0, cls_name, NULL, style[i],
                100, 100, 100, 100, NULL, NULL, wc.hInstance, (LPVOID)TRUE);
        CreateWindowExA(0, cls_name, NULL, style[i],
                100, 100, 100, 100, NULL, NULL, wc.hInstance, (LPVOID)FALSE);
    }
    UnregisterClassA(cls_name, wc.hInstance);
}

static void test_SetScrollInfo(void)
{
    SCROLLINFO si;
    HWND mainwnd;
    BOOL ret;

    mainwnd = create_main_test_wnd();

    ret = IsWindowEnabled(hScroll);
    ok(ret, "scroll bar disabled\n");

    EnableScrollBar(hScroll, SB_CTL, ESB_DISABLE_BOTH);

    ret = IsWindowEnabled(hScroll);
    ok(!ret, "scroll bar enabled\n");

    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = 0xf;
    ret = GetScrollInfo(hScroll, SB_CTL, &si);
    ok(ret, "got %d\n", ret);

    /* SetScrollInfo */
    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    ret = IsWindowEnabled(hScroll);
    ok(!ret, "scroll bar enabled\n");
    si.fMask = SIF_POS|SIF_RANGE|SIF_PAGE|SIF_DISABLENOSCROLL;
    si.nMax = 100;
    si.nMin = 10;
    si.nPos = 0;
    si.nPage = 100;
    SetScrollInfo(hScroll, SB_CTL, &si, TRUE);
    ret = IsWindowEnabled(hScroll);
    ok(!ret, "scroll bar enabled\n");

    si.fMask = 0xf;
    ret = GetScrollInfo(hScroll, SB_CTL, &si);
    ok(ret, "got %d\n", ret);

    EnableScrollBar(hScroll, SB_CTL, ESB_ENABLE_BOTH);
    ok(IsWindowEnabled(hScroll), "expected enabled scrollbar\n");

    si.fMask = SIF_POS|SIF_RANGE|SIF_PAGE|SIF_DISABLENOSCROLL;
    si.nMax = 10;
    si.nMin = 100;
    si.nPos = 0;
    si.nPage = 100;
    SetScrollInfo(hScroll, SB_CTL, &si, TRUE);
    ret = IsWindowEnabled(hScroll);
    ok(ret, "scroll bar disabled\n");

    DestroyWindow(hScroll);
    DestroyWindow(mainwnd);
}

static WNDPROC scrollbar_wndproc;

static SCROLLINFO set_scrollinfo;

static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_CREATE && ((CREATESTRUCTA*)lparam)->lpCreateParams)
        return DefWindowProcA(hwnd, msg, wparam, lparam);

    if (msg == SBM_SETSCROLLINFO)
        set_scrollinfo = *(SCROLLINFO*)lparam;

    return CallWindowProcA(scrollbar_wndproc, hwnd, msg, wparam, lparam);
}

static void test_subclass(void)
{
    SCROLLBARINFO scroll_info;
    WNDCLASSEXA class_info;
    WNDCLASSA wc;
    LRESULT res;
    HWND hwnd;
    BOOL r;

    r = GetClassInfoExA(GetModuleHandleA(NULL), "SCROLLBAR", &class_info);
    ok(r, "GetClassInfoEx failed: %u\n", GetLastError());
    scrollbar_wndproc = class_info.lpfnWndProc;

    memset(&wc, 0, sizeof(wc));
    wc.cbWndExtra = class_info.cbWndExtra + 3; /* more space than needed works */
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "MyTestSubclass";
    wc.lpfnWndProc = subclass_proc;
    r = RegisterClassA(&wc);
    ok(r, "RegisterClass failed: %u\n", GetLastError());

    hwnd = CreateWindowExA( 0, "MyTestSubclass", "Scroll", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0 );
    ok(hwnd != NULL, "Failed to create window: %u\n", GetLastError());

    r = SetScrollRange(hwnd, SB_CTL, 0, 100, TRUE);
    ok(r, "SetScrollRange failed: %u\n", GetLastError());

    res = SetScrollPos(hwnd, SB_CTL, 2, FALSE);
    ok(!res, "SetScrollPos returned %lu\n", res);

    memset(&set_scrollinfo, 0xcc, sizeof(set_scrollinfo));
    res = SetScrollPos(hwnd, SB_CTL, 1, FALSE);
    ok(res == 2, "SetScrollPos returned %lu\n", res);
    ok(set_scrollinfo.cbSize == sizeof(SCROLLINFO), "cbSize = %u\n", set_scrollinfo.cbSize);
    todo_wine
    ok(set_scrollinfo.fMask == (0x1000 | SIF_POS), "fMask = %x\n", set_scrollinfo.fMask);
    ok(set_scrollinfo.nPos == 1, "nPos = %x\n", set_scrollinfo.nPos);

    memset(&scroll_info, 0xcc, sizeof(scroll_info));
    scroll_info.cbSize = sizeof(scroll_info);
    res = SendMessageA(hwnd, SBM_GETSCROLLBARINFO, 0, (LPARAM)&scroll_info);
    ok(res == 1, "SBM_GETSCROLLBARINFO returned %lu\n", res);

    DestroyWindow(hwnd);

    /* if we skip calling wndproc for WM_CREATE, window is not considered a scrollbar */
    hwnd = CreateWindowExA( 0, "MyTestSubclass", "Scroll", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandleA(NULL), (void *)1 );
    ok(hwnd != NULL, "Failed to create window: %u\n", GetLastError());

    memset(&scroll_info, 0xcc, sizeof(scroll_info));
    scroll_info.cbSize = sizeof(scroll_info);
    res = SendMessageA(hwnd, SBM_GETSCROLLBARINFO, 0, (LPARAM)&scroll_info);
    ok(!res, "SBM_GETSCROLLBARINFO returned %lu\n", res);

    DestroyWindow(hwnd);

    /* not enough space in extra data */
    wc.cbWndExtra = class_info.cbWndExtra - 1;
    wc.lpszClassName = "MyTestSubclass2";
    r = RegisterClassA(&wc);
    ok(r, "RegisterClass failed: %u\n", GetLastError());

    hwnd = CreateWindowExA( 0, "MyTestSubclass2", "Scroll", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0 );
    ok(hwnd != NULL, "Failed to create window: %u\n", GetLastError());

    memset(&scroll_info, 0xcc, sizeof(scroll_info));
    scroll_info.cbSize = sizeof(scroll_info);
    res = SendMessageA(hwnd, SBM_GETSCROLLBARINFO, 0, (LPARAM)&scroll_info);
    ok(!res, "SBM_GETSCROLLBARINFO returned %lu\n", res);

    memset(&set_scrollinfo, 0xcc, sizeof(set_scrollinfo));
    res = SetScrollPos(hwnd, SB_CTL, 1, FALSE);
    ok(res == 0, "SetScrollPos returned %lu\n", res);
    ok(set_scrollinfo.cbSize == sizeof(SCROLLINFO), "cbSize = %u\n", set_scrollinfo.cbSize);
    todo_wine
    ok(set_scrollinfo.fMask == (0x1000 | SIF_POS), "fMask = %x\n", set_scrollinfo.fMask);
    ok(set_scrollinfo.nPos == 1, "nPos = %x\n", set_scrollinfo.nPos);

    DestroyWindow(hwnd);
}

START_TEST ( scroll )
{
    WNDCLASSA wc;
    HMODULE hUxtheme;
    BOOL (WINAPI * pIsThemeActive)(VOID);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);

    test_EnableScrollBar();
    test_SetScrollPos();
    test_ShowScrollBar();
    test_GetScrollBarInfo();
    scrollbar_test_track();
    test_SetScrollInfo();
    test_subclass();

    /* Some test results vary depending of theming being active or not */
    hUxtheme = LoadLibraryA("uxtheme.dll");
    if (hUxtheme)
    {
        pIsThemeActive = (void*)GetProcAddress(hUxtheme, "IsThemeActive");
        if (pIsThemeActive)
            bThemeActive = pIsThemeActive();
        FreeLibrary(hUxtheme);
    }

    scrollbar_test_default( 0);
    scrollbar_test_default( WS_HSCROLL);
    scrollbar_test_default( WS_VSCROLL);
    scrollbar_test_default( WS_HSCROLL | WS_VSCROLL);

    scrollbar_test_init();
}
