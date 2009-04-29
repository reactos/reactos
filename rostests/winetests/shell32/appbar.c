/* Unit tests for appbars
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
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

#include <windows.h>

#include "wine/test.h"

#define MSG_APPBAR WM_APP

static const CHAR testwindow_class[] = "testwindow";

static HMONITOR (WINAPI *pMonitorFromWindow)(HWND, DWORD);

typedef BOOL (*boolean_function)(void);

struct testwindow_info
{
    HWND hwnd;
    BOOL registered;
    RECT desired_rect;
    UINT edge;
    RECT allocated_rect;
};

static struct testwindow_info windows[3];

static int expected_bottom;

static void testwindow_setpos(HWND hwnd)
{
    struct testwindow_info* info = (struct testwindow_info*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    APPBARDATA abd;
    BOOL ret;

    ok(info != NULL, "got unexpected ABN_POSCHANGED notification\n");

    if (!info || !info->registered)
    {
        return;
    }

    abd.cbSize = sizeof(abd);
    abd.hWnd = hwnd;
    abd.uEdge = info->edge;
    abd.rc = info->desired_rect;
    ret = SHAppBarMessage(ABM_QUERYPOS, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    switch (info->edge)
    {
        case ABE_BOTTOM:
            ok(info->desired_rect.top == abd.rc.top, "ABM_QUERYPOS changed top of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.top = abd.rc.bottom - (info->desired_rect.bottom - info->desired_rect.top);
            break;
        case ABE_LEFT:
            ok(info->desired_rect.right == abd.rc.right, "ABM_QUERYPOS changed right of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.right = abd.rc.left + (info->desired_rect.right - info->desired_rect.left);
            break;
        case ABE_RIGHT:
            ok(info->desired_rect.left == abd.rc.left, "ABM_QUERYPOS changed left of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.left = abd.rc.right - (info->desired_rect.right - info->desired_rect.left);
            break;
        case ABE_TOP:
            ok(info->desired_rect.bottom == abd.rc.bottom, "ABM_QUERYPOS changed bottom of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.bottom = abd.rc.top + (info->desired_rect.bottom - info->desired_rect.top);
            break;
    }

    ret = SHAppBarMessage(ABM_SETPOS, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    info->allocated_rect = abd.rc;
    MoveWindow(hwnd, abd.rc.left, abd.rc.top, abd.rc.right-abd.rc.left, abd.rc.bottom-abd.rc.top, TRUE);
}

static LRESULT CALLBACK testwindow_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case MSG_APPBAR:
        {
            switch(wparam)
            {
                case ABN_POSCHANGED:
                    testwindow_setpos(hwnd);
                    break;
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* process pending messages until a condition is true or 3 seconds pass */
static void do_events_until(boolean_function test)
{
    MSG msg;
    UINT_PTR timerid;
    BOOL timedout=FALSE;

    timerid = SetTimer(0, 0, 3000, NULL);

    while (1)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.hwnd == 0 && msg.message == WM_TIMER && msg.wParam == timerid)
                timedout = TRUE;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (timedout || test())
            break;
        WaitMessage();
    }

    KillTimer(0, timerid);
}

/* process any pending messages */
static void do_events(void)
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static BOOL no_appbars_intersect(void)
{
    int i, j;
    RECT rc;

    for (i=0; i<2; i++)
    {
        for (j=i+1; j<3; j++)
        {
            if (windows[i].registered && windows[j].registered &&
                IntersectRect(&rc, &windows[i].allocated_rect, &windows[j].allocated_rect))
                return FALSE;
        }
    }
    return TRUE;
}

static BOOL got_expected_bottom(void)
{
    return (no_appbars_intersect() && windows[1].allocated_rect.bottom == expected_bottom);
}

static void register_testwindow_class(void)
{
    WNDCLASSEXA cls;

    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = 0;
    cls.lpfnWndProc = testwindow_wndproc;
    cls.hInstance = NULL;
    cls.hCursor = LoadCursor(0, IDC_ARROW);
    cls.hbrBackground = (HBRUSH) COLOR_WINDOW;
    cls.lpszClassName = testwindow_class;

    RegisterClassExA(&cls);
}

#define test_window_rects(a, b) \
    ok(!IntersectRect(&rc, &windows[a].allocated_rect, &windows[b].allocated_rect), \
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n", \
        windows[a].allocated_rect.left, windows[a].allocated_rect.top, windows[a].allocated_rect.right, windows[a].allocated_rect.bottom, \
        windows[b].allocated_rect.left, windows[b].allocated_rect.top, windows[b].allocated_rect.right, windows[b].allocated_rect.bottom)

static void test_setpos(void)
{
    APPBARDATA abd;
    RECT rc;
    int screen_width, screen_height;
    BOOL ret;

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);

    /* create and register windows[0] */
    windows[0].hwnd = CreateWindowExA(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(windows[0].hwnd != NULL, "couldn't create window\n");
    do_events();
    abd.cbSize = sizeof(abd);
    abd.hWnd = windows[0].hwnd;
    abd.uCallbackMessage = MSG_APPBAR;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* ABM_NEW should return FALSE if the window is already registered */
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == FALSE, "SHAppBarMessage returned %i\n", ret);
    do_events();

    /* dock windows[0] to the bottom of the screen */
    windows[0].registered = TRUE;
    windows[0].edge = ABE_BOTTOM;
    windows[0].desired_rect.left = 0;
    windows[0].desired_rect.right = screen_width;
    windows[0].desired_rect.top = screen_height - 15;
    windows[0].desired_rect.bottom = screen_height;
    SetWindowLongPtr(windows[0].hwnd, GWLP_USERDATA, (LONG_PTR)&windows[0]);
    testwindow_setpos(windows[0].hwnd);
    do_events();

    /* create and register windows[1] */
    windows[1].hwnd = CreateWindowExA(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(windows[1].hwnd != NULL, "couldn't create window\n");
    abd.hWnd = windows[1].hwnd;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* dock windows[1] to the bottom of the screen */
    windows[1].registered = TRUE;
    windows[1].edge = ABE_BOTTOM;
    windows[1].desired_rect.left = 0;
    windows[1].desired_rect.right = screen_width;
    windows[1].desired_rect.top = screen_height - 10;
    windows[1].desired_rect.bottom = screen_height;
    SetWindowLongPtr(windows[1].hwnd, GWLP_USERDATA, (LONG_PTR)&windows[1]);
    testwindow_setpos(windows[1].hwnd);

    /* the windows are adjusted to they don't overlap */
    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);

    /* make windows[0] larger, forcing windows[1] to move out of its way */
    windows[0].desired_rect.top = screen_height - 20;
    testwindow_setpos(windows[0].hwnd);
    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);

    /* create and register windows[2] */
    windows[2].hwnd = CreateWindowExA(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(windows[2].hwnd != NULL, "couldn't create window\n");
    do_events();

    abd.hWnd = windows[2].hwnd;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* dock windows[2] to the bottom of the screen */
    windows[2].registered = TRUE;
    windows[2].edge = ABE_BOTTOM;
    windows[2].desired_rect.left = 0;
    windows[2].desired_rect.right = screen_width;
    windows[2].desired_rect.top = screen_height - 10;
    windows[2].desired_rect.bottom = screen_height;
    SetWindowLongPtr(windows[2].hwnd, GWLP_USERDATA, (LONG_PTR)&windows[2]);
    testwindow_setpos(windows[2].hwnd);

    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);
    test_window_rects(0, 2);
    test_window_rects(1, 2);

    /* move windows[2] to the right side of the screen */
    windows[2].edge = ABE_RIGHT;
    windows[2].desired_rect.left = screen_width - 15;
    windows[2].desired_rect.right = screen_width;
    windows[2].desired_rect.top = 0;
    windows[2].desired_rect.bottom = screen_height;
    testwindow_setpos(windows[2].hwnd);

    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);
    test_window_rects(0, 2);
    test_window_rects(1, 2);

    /* move windows[1] to the top of the screen */
    windows[1].edge = ABE_TOP;
    windows[1].desired_rect.left = 0;
    windows[1].desired_rect.right = screen_width;
    windows[1].desired_rect.top = 0;
    windows[1].desired_rect.bottom = 15;
    testwindow_setpos(windows[1].hwnd);

    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);
    test_window_rects(0, 2);
    test_window_rects(1, 2);

    /* move windows[1] back to the bottom of the screen */
    windows[1].edge = ABE_BOTTOM;
    windows[1].desired_rect.left = 0;
    windows[1].desired_rect.right = screen_width;
    windows[1].desired_rect.top = screen_height - 10;
    windows[1].desired_rect.bottom = screen_height;
    testwindow_setpos(windows[1].hwnd);

    do_events_until(no_appbars_intersect);
    test_window_rects(0, 1);
    test_window_rects(0, 2);
    test_window_rects(1, 2);

    /* removing windows[0] will cause windows[1] to move down into its space */
    expected_bottom = max(windows[0].allocated_rect.bottom, windows[1].allocated_rect.bottom);

    abd.hWnd = windows[0].hwnd;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    windows[0].registered = FALSE;
    DestroyWindow(windows[0].hwnd);

    do_events_until(got_expected_bottom);

    ok(windows[1].allocated_rect.bottom == expected_bottom, "windows[1]'s bottom is %i, expected %i\n", windows[1].allocated_rect.bottom, expected_bottom);

    test_window_rects(1, 2);

    /* remove the other windows */
    abd.hWnd = windows[1].hwnd;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    windows[1].registered = FALSE;
    DestroyWindow(windows[1].hwnd);

    abd.hWnd = windows[2].hwnd;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    windows[2].registered = FALSE;
    DestroyWindow(windows[2].hwnd);
}

static void test_appbarget(void)
{
    APPBARDATA abd;
    HWND hwnd, foregnd, unset_hwnd;
    UINT_PTR ret;

    memset(&abd, 0xcc, sizeof(abd));
    memset(&unset_hwnd, 0xcc, sizeof(unset_hwnd));
    abd.cbSize = sizeof(abd);
    abd.uEdge = ABE_BOTTOM;

    hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
    ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
    ok(abd.hWnd == unset_hwnd, "hWnd overwritten %p\n",abd.hWnd);

    if (!pMonitorFromWindow)
    {
        win_skip("MonitorFromWindow is not available\n");
    }
    else
    {
        /* Presumably one can pass a hwnd with ABM_GETAUTOHIDEBAR to specify a monitor.
           Pass the foreground window and check */
        foregnd = GetForegroundWindow();
        if(foregnd)
        {
            abd.hWnd = foregnd;
            hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
            ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
            ok(abd.hWnd == foregnd, "hWnd overwritten\n");
            if(hwnd)
            {
                HMONITOR appbar_mon, foregnd_mon;
                appbar_mon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                foregnd_mon = pMonitorFromWindow(foregnd, MONITOR_DEFAULTTONEAREST);
                ok(appbar_mon == foregnd_mon, "Windows on different monitors\n");
            }
        }
    }

    memset(&abd, 0xcc, sizeof(abd));
    abd.cbSize = sizeof(abd);
    ret = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    if(ret)
    {
        ok(abd.hWnd == (HWND)0xcccccccc, "hWnd overwritten\n");
        ok(abd.uEdge <= ABE_BOTTOM, "uEdge not returned\n");
        ok(abd.rc.left != 0xcccccccc, "rc not updated\n");
    }

    return;
}

START_TEST(appbar)
{
    HMODULE huser32;

    huser32 = GetModuleHandleA("user32.dll");
    pMonitorFromWindow = (void*)GetProcAddress(huser32, "MonitorFromWindow");

    register_testwindow_class();

    test_setpos();
    test_appbarget();
}
