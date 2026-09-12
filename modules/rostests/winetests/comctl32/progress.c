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
#include "vssym32.h"

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

static HWND hProgressParentWnd;
static const char progressTestClass[] = "ProgressBarTestClass";
static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

/* For message tests */
enum seq_index
{
    CHILD_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static void CALLBACK msg_winevent_proc(HWINEVENTHOOK hevent,
                                       DWORD event,
                                       HWND hwnd,
                                       LONG object_id,
                                       LONG child_id,
                                       DWORD thread_id,
                                       DWORD event_time)
{
    struct message msg = {0};
    char class_name[256];

    /* ignore window and other system events */
    if (object_id != OBJID_CLIENT) return;

    /* ignore events not from a progress bar control */
    if (!GetClassNameA(hwnd, class_name, ARRAY_SIZE(class_name)) ||
        strcmp(class_name, PROGRESS_CLASSA) != 0)
        return;

    msg.message = event;
    msg.flags = winevent_hook|wparam|lparam;
    msg.wParam = object_id;
    msg.lParam = child_id;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);
}

static void init_winevent_hook(void) {
    hwineventhook = SetWinEventHook(EVENT_MIN, EVENT_MAX, GetModuleHandleA(0), msg_winevent_proc,
                                    0, GetCurrentThreadId(), WINEVENT_INCONTEXT);
    if (!hwineventhook)
        win_skip( "no win event hook support\n" );
}

static void uninit_winevent_hook(void) {
    if (!hwineventhook)
        return;

    UnhookWinEvent(hwineventhook);
    hwineventhook = 0;
}

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

static WNDPROC old_proc;

static const struct message paint_pbm_setstate_seq[] =
{
    {PBM_SETSTATE, sent},
    {EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, OBJID_CLIENT, 0},
    {WM_PAINT, sent},
    {WM_ERASEBKGND, sent | defwinproc},
    {0}
};

static const struct message pbm_setstate_seq[] =
{
    {PBM_SETSTATE, sent},
    {0}
};

static LRESULT WINAPI test_pbm_setstate_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    static int defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;

    if (message == PBM_SETSTATE
        || message == WM_PAINT
        || message == WM_ERASEBKGND)
    {
        msg.message = message;
        msg.flags = sent | wparam | lparam;
        if (defwndproc_counter)
            msg.flags |= defwinproc;
        msg.wParam = wp;
        msg.lParam = lp;
        add_message(sequences, CHILD_SEQ_INDEX, &msg);
    }

    ++defwndproc_counter;
    ret = CallWindowProcA(old_proc, hwnd, message, wp, lp);
    --defwndproc_counter;
    return ret;
}

void test_bar_states(void)
{
    HWND progress_bar;
    int state;

    static const struct
    {
        DWORD state;
        DWORD previous_state;
        const struct message *expected_seq;
        BOOL error;
    }
    tests[] =
    {
        {0, PBST_NORMAL, pbm_setstate_seq, 1},
        {PBST_NORMAL, PBST_NORMAL, pbm_setstate_seq, 0},
        {PBST_PAUSED, PBST_NORMAL, paint_pbm_setstate_seq, 0},
        {PBST_PAUSED, PBST_PAUSED, pbm_setstate_seq, 0},
        {PBST_ERROR, PBST_PAUSED, paint_pbm_setstate_seq, 0},
        {PBST_ERROR, PBST_ERROR, pbm_setstate_seq, 0},
        {PBFS_PARTIAL, PBST_ERROR, pbm_setstate_seq, 1}
    };

    progress_bar = create_progress(0);

    old_proc = (WNDPROC)SetWindowLongPtrA(progress_bar, GWLP_WNDPROC, (LONG_PTR)test_pbm_setstate_proc);
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    for (int i = 0; i < ARRAY_SIZE(tests); i++)
    {
        state = SendMessageA(progress_bar, PBM_SETSTATE, tests[i].state, 0);
        flush_events();
        ok_sequence(sequences, CHILD_SEQ_INDEX, tests[i].expected_seq, "PBM_SETSTATE", FALSE);
        ok(state == (tests[i].error ? 0 : tests[i].previous_state), "Expected %ld, but got %d.\n", tests[i].previous_state, state);
        state = SendMessageA(progress_bar, PBM_GETSTATE, 0, 0);
        ok(state == (tests[i].error ? tests[i].previous_state : tests[i].state), "Expected %ld, but got %d.\n", tests[i].state, state);
    }

    DestroyWindow(progress_bar);
}

static const struct message step_seq[] =
{
    {PBM_SETRANGE, sent},
    {PBM_SETPOS, sent},
    {EVENT_OBJECT_VALUECHANGE, winevent_hook, OBJID_CLIENT, 0},
    {PBM_SETPOS, sent},
    {PBM_SETSTEP, sent},
    {0}
};

static LRESULT WINAPI test_pbm_step_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    static int defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;

    if (message == PBM_SETSTEP
        || message == PBM_SETRANGE
        || message == PBM_SETPOS)
    {
        msg.message = message;
        msg.flags = sent | wparam | lparam;
        if (defwndproc_counter)
            msg.flags |= defwinproc;
        msg.wParam = wp;
        msg.lParam = lp;
        add_message(sequences, CHILD_SEQ_INDEX, &msg);
    }

    ++defwndproc_counter;
    ret = CallWindowProcA(old_proc, hwnd, message, wp, lp);
    --defwndproc_counter;
    return ret;
}

void test_step_messages(void)
{
    HWND progress_bar;

    progress_bar = create_progress(0);

    old_proc = (WNDPROC)SetWindowLongPtrA(progress_bar, GWLP_WNDPROC, (LONG_PTR)test_pbm_step_proc);

    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageA(progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageA(progress_bar, PBM_SETPOS, 10, 0);
    SendMessageA(progress_bar, PBM_SETPOS, 10, 0);
    SendMessageA(progress_bar, PBM_SETSTEP, 20, 0);

    ok_sequence(sequences, CHILD_SEQ_INDEX, step_seq, "step_seq", FALSE);

    DestroyWindow(progress_bar);
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
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    init_winevent_hook();

    test_setcolors();
    test_PBM_STEPIT();
    test_bar_states();
    test_step_messages();

    uninit_winevent_hook();

    unload_v6_module(ctx_cookie, hCtx);

    cleanup();
}
