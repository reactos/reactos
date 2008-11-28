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

static HWND hScroll, hMainWnd;

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

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static void scrollbar_test1(void)
{
    BOOL ret;

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_DISABLE_BOTH );
    ok( ret, "The scrollbar should be disabled.\n" );
    todo_wine
    {
        ok( !IsWindowEnabled( hScroll ), "The scrollbar window should be disabled.\n" );
    }

    ret = EnableScrollBar( hScroll, SB_CTL, ESB_ENABLE_BOTH );
    ok( ret, "The scrollbar should be enabled.\n" );
    ok( IsWindowEnabled( hScroll ), "The scrollbar window should be enabled.\n" );

}

static void scrollbar_test2(void)
{
    int ret;

    trace("The scrollbar is disabled.\n");

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

    trace("The scrollbar is enabled.\n");

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
}

static void scrollbar_test3(void)
{
    BOOL    ret;

    ret = ShowScrollBar( hScroll, SB_CTL, FALSE );
    ok( ret, "The ShowScrollBar() should not failed.\n" );
    ok( !IsWindowVisible( hScroll ), "The scrollbar window should not be visible\n" );

    ret = ShowScrollBar( hScroll, SB_CTL, TRUE );
    ok( ret, "The ShowScrollBar() should not failed.\n" );
    ok( !IsWindowVisible( hScroll ), "The scrollbar window should be visible\n" );

    ret = ShowScrollBar( NULL, SB_CTL, TRUE );
    ok( !ret, "The ShowScrollBar() should failed.\n" );

}

static void scrollbar_test4(void)
{
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

    /* Test GetScrollBarInfo to make sure it returns rcScrollBar in screen
     * coordinates. */
    sbi.cbSize = sizeof(sbi);
    ret = pGetScrollBarInfo( hScroll, OBJID_CLIENT, &sbi);
    ok( ret, "The GetScrollBarInfo() call should not fail.\n" );
    GetWindowRect( hScroll, &rect );
    ok( ret, "The GetWindowRect() call should not fail.\n" );
    ok( !(sbi.rgstate[0] & (STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN)),
        "unexpected rgstate(0x%x)\n", sbi.rgstate[0]);
    ok( EqualRect(&rect, &sbi.rcScrollBar),
        "WindowRect(%d, %d, %d, %d) != rcScrollBar(%d, %d, %d, %d)\n",
        rect.top, rect.left, rect.bottom, rect.right,
        sbi.rcScrollBar.top, sbi.rcScrollBar.left,
        sbi.rcScrollBar.bottom, sbi.rcScrollBar.right );

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
    ok( EqualRect(&rect, &sbi.rcScrollBar),
        "PreviousRect(%d, %d, %d, %d) != CurrentRect(%d, %d, %d, %d)\n",
        rect.top, rect.left, rect.bottom, rect.right,
        sbi.rcScrollBar.top, sbi.rcScrollBar.left,
        sbi.rcScrollBar.bottom, sbi.rcScrollBar.right );

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
    ok( EqualRect(&rect, &sbi.rcScrollBar),
        "PreviousRect(%d, %d, %d, %d) != CurrentRect(%d, %d, %d, %d)\n",
        rect.top, rect.left, rect.bottom, rect.right,
        sbi.rcScrollBar.top, sbi.rcScrollBar.left,
        sbi.rcScrollBar.bottom, sbi.rcScrollBar.right );
}

START_TEST ( scroll )
{
    WNDCLASSA wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA( 0, "MyTestWnd", "Scroll",
      WS_OVERLAPPEDWINDOW|WS_VSCROLL|WS_HSCROLL,
      CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0 );

    if ( !ok( hMainWnd != NULL, "Failed to create parent window. Tests aborted.\n" ) )
        return;

    assert( hScroll );

    scrollbar_test1();
    scrollbar_test2();
    scrollbar_test3();
    scrollbar_test4();

    DestroyWindow(hScroll);
    DestroyWindow(hMainWnd);
}
