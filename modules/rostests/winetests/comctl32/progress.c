/* Unit tests for the progress bar control.
 *
 * Copyright 2005 Michael Kaufmann
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

#include "v6util.h"

static HWND hProgressParentWnd;
static const char progressTestClass[] = "ProgressBarTestClass";
static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

static HWND create_progress(DWORD style)
{
    return CreateWindowExA(0, PROGRESS_CLASSA, "", WS_VISIBLE | style,
      0, 0, 100, 20, NULL, NULL, GetModuleHandleA(NULL), 0);
}

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

static LRESULT CALLBACK progress_test_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
  
    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    
    return 0L;
}

static WNDPROC progress_wndproc;
static BOOL erased;
static RECT last_paint_rect;

static LRESULT CALLBACK progress_subclass_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_PAINT)
    {
        GetUpdateRect(hWnd, &last_paint_rect, FALSE);
    }
    else if (msg == WM_ERASEBKGND)
    {
        erased = TRUE;
    }
    return CallWindowProcA(progress_wndproc, hWnd, msg, wParam, lParam);
}


static void update_window(HWND hWnd)
{
    UpdateWindow(hWnd);
    ok(!GetUpdateRect(hWnd, NULL, FALSE), "GetUpdateRect must return zero after UpdateWindow\n");    
}


static void init(void)
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
    wc.lpszClassName = progressTestClass;
    wc.lpfnWndProc = progress_test_wnd_proc;
    RegisterClassA(&wc);

    SetRect(&rect, 0, 0, 400, 20);
    ret = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    ok(ret, "got %d\n", ret);
    
    hProgressParentWnd = CreateWindowExA(0, progressTestClass, "Progress Bar Test", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(hProgressParentWnd != NULL, "failed to create parent wnd\n");

}

static void cleanup(void)
{
    MSG msg;
    
    PostMessageA(hProgressParentWnd, WM_CLOSE, 0, 0);
    while (GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    UnregisterClassA(progressTestClass, GetModuleHandleA(NULL));
}


/*
 * Tests if a progress bar repaints itself immediately when it receives
 * some specific messages.
 */
static void test_redraw(void)
{
    RECT client_rect, rect;
    HWND hProgressWnd;
    LRESULT ret;

    GetClientRect(hProgressParentWnd, &rect);
    hProgressWnd = CreateWindowExA(0, PROGRESS_CLASSA, "", WS_CHILD | WS_VISIBLE,
      0, 0, rect.right, rect.bottom, hProgressParentWnd, NULL, GetModuleHandleA(NULL), 0);
    ok(hProgressWnd != NULL, "Failed to create progress bar.\n");
    progress_wndproc = (WNDPROC)SetWindowLongPtrA(hProgressWnd, GWLP_WNDPROC, (LPARAM)progress_subclass_proc);

    ShowWindow(hProgressParentWnd, SW_SHOWNORMAL);
    ok(GetUpdateRect(hProgressParentWnd, NULL, FALSE), "GetUpdateRect: There should be a region that needs to be updated\n");
    flush_events();
    update_window(hProgressParentWnd);

    SendMessageA(hProgressWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageA(hProgressWnd, PBM_SETPOS, 10, 0);
    SendMessageA(hProgressWnd, PBM_SETSTEP, 20, 0);
    update_window(hProgressWnd);

    /* PBM_SETPOS */
    ok(SendMessageA(hProgressWnd, PBM_SETPOS, 50, 0) == 10, "PBM_SETPOS must return the previous position\n");
    ok(!GetUpdateRect(hProgressWnd, NULL, FALSE), "PBM_SETPOS: The progress bar should be redrawn immediately\n");

    /* PBM_DELTAPOS */
    ok(SendMessageA(hProgressWnd, PBM_DELTAPOS, 15, 0) == 50, "PBM_DELTAPOS must return the previous position\n");
    ok(!GetUpdateRect(hProgressWnd, NULL, FALSE), "PBM_DELTAPOS: The progress bar should be redrawn immediately\n");

    /* PBM_SETPOS */
    ok(SendMessageA(hProgressWnd, PBM_SETPOS, 80, 0) == 65, "PBM_SETPOS must return the previous position\n");
    ok(!GetUpdateRect(hProgressWnd, NULL, FALSE), "PBM_SETPOS: The progress bar should be redrawn immediately\n");

    /* PBM_STEPIT */
    ok(SendMessageA(hProgressWnd, PBM_STEPIT, 0, 0) == 80, "PBM_STEPIT must return the previous position\n");
    ok(!GetUpdateRect(hProgressWnd, NULL, FALSE), "PBM_STEPIT: The progress bar should be redrawn immediately\n");
    ret = SendMessageA(hProgressWnd, PBM_GETPOS, 0, 0);
    if (ret == 0)
        win_skip("PBM_GETPOS needs comctl32 > 4.70\n");
    else
        ok(ret == 100, "PBM_GETPOS returned a wrong position : %d\n", (UINT)ret);

    /* PBM_SETRANGE and PBM_SETRANGE32:
    Usually the progress bar doesn't repaint itself immediately. If the
    position is not in the new range, it does.
    Don't test this, it may change in future Windows versions. */

    SendMessageA(hProgressWnd, PBM_SETPOS, 0, 0);
    update_window(hProgressWnd);

    /* increase to 10 - no background erase required */
    erased = FALSE;
    SetRectEmpty(&last_paint_rect);
    SendMessageA(hProgressWnd, PBM_SETPOS, 10, 0);
    GetClientRect(hProgressWnd, &client_rect);
    ok(EqualRect(&last_paint_rect, &client_rect), "last_paint_rect was %s instead of %s\n",
       wine_dbgstr_rect(&last_paint_rect), wine_dbgstr_rect(&client_rect));
    update_window(hProgressWnd);
    ok(!erased, "Progress bar shouldn't have erased the background\n");

    /* decrease to 0 - background erase will be required */
    erased = FALSE;
    SetRectEmpty(&last_paint_rect);
    SendMessageA(hProgressWnd, PBM_SETPOS, 0, 0);
    GetClientRect(hProgressWnd, &client_rect);
    ok(EqualRect(&last_paint_rect, &client_rect), "last_paint_rect was %s instead of %s\n",
       wine_dbgstr_rect(&last_paint_rect), wine_dbgstr_rect(&client_rect));
    update_window(hProgressWnd);
    ok(erased, "Progress bar should have erased the background\n");

    DestroyWindow(hProgressWnd);
}

static void test_setcolors(void)
{
    HWND progress;
    COLORREF clr;

    progress = create_progress(PBS_SMOOTH);

    clr = SendMessageA(progress, PBM_SETBARCOLOR, 0, 0);
    ok(clr == CLR_DEFAULT, "got %lx\n", clr);

    clr = SendMessageA(progress, PBM_SETBARCOLOR, 0, RGB(0, 255, 0));
    ok(clr == 0, "got %lx\n", clr);

    clr = SendMessageA(progress, PBM_SETBARCOLOR, 0, CLR_DEFAULT);
    ok(clr == RGB(0, 255, 0), "got %lx\n", clr);

    clr = SendMessageA(progress, PBM_SETBKCOLOR, 0, 0);
    ok(clr == CLR_DEFAULT, "got %lx\n", clr);

    clr = SendMessageA(progress, PBM_SETBKCOLOR, 0, RGB(255, 0, 0));
    ok(clr == 0, "got %lx\n", clr);

    clr = SendMessageA(progress, PBM_SETBKCOLOR, 0, CLR_DEFAULT);
    ok(clr == RGB(255, 0, 0), "got %lx\n", clr);

    DestroyWindow(progress);
}

static void test_PBM_STEPIT(void)
{
    struct stepit_test
    {
        int min;
        int max;
        int step;
    } stepit_tests[] =
    {
        { 3, 15,  5 },
        { 3, 15, -5 },
        { 3, 15, 50 },
        { -15, 15,  5 },
        { -3, -2, -5 },
        { 0, 0, 1 },
        { 5, 5, 1 },
        { 0, 0, -1 },
        { 5, 5, -1 },
        { 10, 5, 2 },
    };
    HWND progress;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(stepit_tests); i++)
    {
        struct stepit_test *test = &stepit_tests[i];
        PBRANGE range;
        LRESULT ret;

        progress = create_progress(0);

        ret = SendMessageA(progress, PBM_SETRANGE32, test->min, test->max);
        ok(ret != 0, "Unexpected return value.\n");

        SendMessageA(progress, PBM_GETRANGE, 0, (LPARAM)&range);
        ok(range.iLow == test->min && range.iHigh == test->max, "Unexpected range.\n");

        SendMessageA(progress, PBM_SETPOS, test->min, 0);
        SendMessageA(progress, PBM_SETSTEP, test->step, 0);

        for (j = 0; j < test->max; j++)
        {
            int pos = SendMessageA(progress, PBM_GETPOS, 0, 0);
            int current;

            pos += test->step;
            if (test->min != test->max)
            {
                if (pos > test->max)
                    pos = (pos - test->min) % (test->max - test->min) + test->min;
                if (pos < test->min)
                    pos = (pos - test->min) % (test->max - test->min) + test->max;
            }
            else
                pos = test->min;

            SendMessageA(progress, PBM_STEPIT, 0, 0);

            current = SendMessageA(progress, PBM_GETPOS, 0, 0);
            ok(current == pos, "%u: unexpected position %d, expected %d.\n", i, current, pos);
        }

        DestroyWindow(progress);
    }
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(InitCommonControlsEx);
#undef X
}

START_TEST(progress)
{
    INITCOMMONCONTROLSEX iccex;
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_functions();

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_PROGRESS_CLASS;
    pInitCommonControlsEx(&iccex);

    init();

    test_redraw();
    test_setcolors();
    test_PBM_STEPIT();

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    test_setcolors();
    test_PBM_STEPIT();

    unload_v6_module(ctx_cookie, hCtx);

    cleanup();
}
