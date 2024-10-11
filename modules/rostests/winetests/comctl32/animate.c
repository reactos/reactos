/* Unit tests for the animate control.
 *
 * Copyright 2016 Bruno Jesus
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/test.h"

#define SEARCHING_AVI_INDEX 151 /* From shell32 resource library */
#define INVALID_AVI_INDEX 0xffff

static HWND hAnimateParentWnd, hAnimateWnd;
static const char animateTestClass[] = "AnimateTestClass";
static WNDPROC animate_wndproc;
static HANDLE shell32;

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min(10,diff), QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}

static LRESULT CALLBACK animate_test_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0L;
}

static void update_window(HWND hWnd)
{
    UpdateWindow(hWnd);
    ok(!GetUpdateRect(hWnd, NULL, FALSE), "GetUpdateRect must return zero after UpdateWindow\n");
}

static void create_animate(DWORD parent_style, DWORD animate_style)
{
    WNDCLASSA wc;
    RECT rect;
    BOOL ret;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = animateTestClass;
    wc.lpfnWndProc = animate_test_wnd_proc;
    RegisterClassA(&wc);

    SetRect(&rect, 0, 0, 200, 200);
    ret = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    ok(ret, "got %d\n", ret);

    hAnimateParentWnd = CreateWindowExA(0, animateTestClass, "Animate Test", WS_OVERLAPPEDWINDOW | parent_style,
      CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(hAnimateParentWnd != NULL, "failed to create parent wnd\n");

    GetClientRect(hAnimateParentWnd, &rect);
    hAnimateWnd = CreateWindowExA(0, ANIMATE_CLASSA, NULL, WS_CHILD | WS_VISIBLE | animate_style,
      0, 0, rect.right, rect.bottom, hAnimateParentWnd, NULL, shell32, 0);
    ok(hAnimateWnd != NULL, "failed to create parent wnd\n");
    animate_wndproc = (WNDPROC)SetWindowLongPtrA(hAnimateWnd, GWLP_WNDPROC, 0);

    ShowWindow(hAnimateParentWnd, SW_SHOWNORMAL);
    ok(GetUpdateRect(hAnimateParentWnd, NULL, FALSE), "GetUpdateRect: There should be a region that needs to be updated\n");
    flush_events();
    update_window(hAnimateParentWnd);
}

static void destroy_animate(void)
{
    MSG msg;

    PostMessageA(hAnimateParentWnd, WM_CLOSE, 0, 0);
    while (GetMessageA(&msg,0,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    hAnimateParentWnd = NULL;
}

static void cleanup(void)
{
    UnregisterClassA(animateTestClass, GetModuleHandleA(NULL));
}

static void test_play(void)
{
    LONG res;
    DWORD err;

    create_animate(0, 0);
    SetLastError(0xdeadbeef);
    res = SendMessageA(hAnimateWnd, ACM_OPENA,(WPARAM)shell32, MAKEINTRESOURCE(INVALID_AVI_INDEX));
    err = GetLastError();
    ok(res == 0, "Invalid video should have failed\n");
    ok(err == ERROR_RESOURCE_NAME_NOT_FOUND, "Expected 1814, got %lu\n", err);

    SetLastError(0xdeadbeef);
    res = SendMessageA(hAnimateWnd, ACM_PLAY, (WPARAM) -1, MAKELONG(0, -1));
    err = GetLastError();
    ok(res == 0, "Play should have failed\n");
    ok(err == 0xdeadbeef, "Expected 0xdeadbeef, got %lu\n", err);
    destroy_animate();

    create_animate(0, 0);
    res = SendMessageA(hAnimateWnd, ACM_OPENA,(WPARAM)shell32, MAKEINTRESOURCE(SEARCHING_AVI_INDEX));
    ok(res != 0, "Load AVI resource failed\n");
    res = SendMessageA(hAnimateWnd, ACM_PLAY, (WPARAM) -1, MAKELONG(0, -1));
    ok(res != 0, "Play should have worked\n");
    destroy_animate();
}

START_TEST(animate)
{
    shell32 = LoadLibraryA("Shell32.dll");

    test_play();

    cleanup();
}
