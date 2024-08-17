/*
 * Copyright (c) 2024 Marek Benc <benc.marek.elektro98@proton.me>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>
#include "wine/test.h"

#define TEST_CLASS_NAME   "scrollbar_redraw"
#define TEST_WINDOW_TITLE "scrollbar_redraw"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

static LRESULT CALLBACK
proc_cb(HWND window, UINT message, WPARAM w_param, LPARAM l_param);

#define TEST_COLOR_COUNT 16
static COLORREF colors[TEST_COLOR_COUNT] = { 0 };
static HBRUSH   color_brushes[TEST_COLOR_COUNT] = { 0 };

static BOOL have_hredraw = FALSE;
static BOOL have_vredraw = FALSE;

enum fsm_state
{
    FSM_STATE_START,
    FSM_STATE_VSCR_SHOWN,
    FSM_STATE_VSCR_HIDDEN,
    FSM_STATE_HSCR_SHOWN,
    FSM_STATE_HSCR_HIDDEN,
    FSM_STATE_BSCR_SHOWN,
    FSM_STATE_BSCR_HIDDEN,
    FSM_STATE_WIDTH_SHRUNK,
    FSM_STATE_WIDTH_EXPANDED,
    FSM_STATE_HEIGHT_SHRUNK,
    FSM_STATE_HEIGHT_EXPANDED,
    FSM_STATE_BOTH_SHRUNK,
    FSM_STATE_BOTH_EXPANDED,
    FSM_STATE_END
};

static UINT_PTR fsm_timer = 0;
static unsigned int current_color = 0;
static enum fsm_state fsm_state = FSM_STATE_START;

static int client_width = 0;
static int client_height = 0;

static int orig_width = 0;
static int orig_height = 0;
static int small_width = 0;
static int small_height = 0;

static void
colors_cleanup()
{
    unsigned int iter;

    for (iter = 0; iter < TEST_COLOR_COUNT; iter++)
    {
        if (color_brushes[iter] != NULL)
        {
            DeleteObject(color_brushes[iter]);
            color_brushes[iter] = NULL;
        }
    }
}

static BOOL
colors_init(void)
{
    unsigned int iter;

    if (TEST_COLOR_COUNT != 16)
    {
        return FALSE;
    }

    /* Standard Windows 16-color VGA color palette. */
    colors[0]  = RGB(0x00, 0x00, 0x00);  /* Black */
    colors[1]  = RGB(0x00, 0x00, 0x80);  /* Dark Blue */
    colors[2]  = RGB(0x00, 0x80, 0x00);  /* Dark Green */
    colors[3]  = RGB(0x00, 0x80, 0x80);  /* Dark Cyan */
    colors[4]  = RGB(0x80, 0x00, 0x00);  /* Dark Red */
    colors[5]  = RGB(0x80, 0x00, 0x80);  /* Dark Magenta */
    colors[6]  = RGB(0x80, 0x80, 0x00);  /* Dark Yellow */
    colors[7]  = RGB(0xC0, 0xC0, 0xC0);  /* Light Gray */
    colors[8]  = RGB(0x80, 0x80, 0x80);  /* Dark Gray */
    colors[9]  = RGB(0x00, 0x00, 0xFF);  /* Blue */
    colors[10] = RGB(0x00, 0xFF, 0x00);  /* Green */
    colors[11] = RGB(0x00, 0xFF, 0xFF);  /* Cyan */
    colors[12] = RGB(0xFF, 0x00, 0x00);  /* Red */
    colors[13] = RGB(0xFF, 0x00, 0xFF);  /* Magenta */
    colors[14] = RGB(0xFF, 0xFF, 0x00);  /* Yellow */
    colors[15] = RGB(0xFF, 0xFF, 0xFF);  /* White */

    for (iter = 0; iter < TEST_COLOR_COUNT; iter++)
    {
        color_brushes[iter] = CreateSolidBrush(colors[iter]);
        if (color_brushes[iter] == NULL)
        {
            colors_cleanup();
            return FALSE;
        }
    }

    return TRUE;
}

static void
run_test_window(const char *class_name, const char *window_title, UINT class_style)
{
    WNDCLASS class = { };
    HWND window;
    MSG  msg;
    ATOM class_atom;

    current_color = 0;

    class.style         = class_style;
    class.lpfnWndProc   = proc_cb;
    class.cbClsExtra    = 0;
    class.cbWndExtra    = 0;
    class.hInstance     = HINST_THISCOMPONENT;
    class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    class.hbrBackground = color_brushes[current_color];
    class.lpszMenuName  = NULL;
    class.lpszClassName = class_name;

    class_atom = RegisterClass(&class);
    ok(class_atom != 0, "Failed to register window class \"%s\", code: %ld\n", class_name, GetLastError());
    if (class_atom == 0)
    {
        return;
    }

    window = CreateWindow(
            class_name,
            window_title,
            WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            NULL);

    ok(window != NULL, "Failed to create window of class \"%s\", code: %ld\n", class_name, GetLastError());
    if (window == NULL)
    {
        return;
    }

    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

START_TEST(ScrollBarRedraw)
{
    BOOL bool_ret;

    bool_ret = colors_init();
    ok(bool_ret, "Failed to initialize colors and solid color brushes\n");
    if (!bool_ret)
    {
        return;
    }

    trace("Running test without specifying either CS_HREDRAW or CS_HREDRAW\n");
    have_hredraw = FALSE;
    have_vredraw = FALSE;
    run_test_window(
            TEST_CLASS_NAME   "_noredraw",
            TEST_WINDOW_TITLE "_noredraw",
            0);

    trace("Running test with CS_HREDRAW\n");
    have_hredraw = TRUE;
    have_vredraw = FALSE;
    run_test_window(
            TEST_CLASS_NAME   "_hredraw",
            TEST_WINDOW_TITLE "_hredraw",
            CS_HREDRAW);

    trace("Running test with CS_VREDRAW\n");
    have_hredraw = FALSE;
    have_vredraw = TRUE;
    run_test_window(
            TEST_CLASS_NAME   "_vredraw",
            TEST_WINDOW_TITLE "_vredraw",
            CS_VREDRAW);

    trace("Running test with both CS_HREDRAW and CS_VREDRAW\n");
    have_hredraw = TRUE;
    have_vredraw = TRUE;
    run_test_window(
            TEST_CLASS_NAME   "_hredraw_vredraw",
            TEST_WINDOW_TITLE "_hredraw_vredraw",
            CS_HREDRAW | CS_VREDRAW);

    trace("Test complete\n");
    colors_cleanup();
}

static void
hide_vert_scroll_bar(HWND window)
{
    SCROLLINFO scroll_info;

    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nPage = client_height;

    scroll_info.nMin = 0;
    scroll_info.nMax = client_height-1;
    scroll_info.nPos = 0;

    SetScrollInfo(window, SB_VERT, &scroll_info, TRUE);
}

static void
show_vert_scroll_bar(HWND window)
{
    SCROLLINFO scroll_info;

    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nPage = client_height;

    scroll_info.nMin = 0;
    scroll_info.nMax = (3*client_height)-1;
    scroll_info.nPos = 0;

    SetScrollInfo(window, SB_VERT, &scroll_info, TRUE);
}

static void
hide_horz_scroll_bar(HWND window)
{
    SCROLLINFO scroll_info;

    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nPage = client_width;

    scroll_info.nMin = 0;
    scroll_info.nMax = client_width-1;
    scroll_info.nPos = 0;

    SetScrollInfo(window, SB_HORZ, &scroll_info, TRUE);
}

static void
show_horz_scroll_bar(HWND window)
{
    SCROLLINFO scroll_info;

    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nPage = client_width;

    scroll_info.nMin = 0;
    scroll_info.nMax = (3*client_width)-1;
    scroll_info.nPos = 0;

    SetScrollInfo(window, SB_HORZ, &scroll_info, TRUE);
}

static int
fsm_transition(HWND window)
{
    static COLORREF prev_color = CLR_INVALID;
    COLORREF color = CLR_INVALID;
    HDC dev_ctx = NULL;

    if (fsm_state != FSM_STATE_END)
    {
        dev_ctx = GetDC(window);

        ok(dev_ctx != NULL, "Failed to get device context\n");
        if (dev_ctx == NULL)
        {
            DestroyWindow(window);
            fsm_state = FSM_STATE_END;
        }
        else
        {
            color = GetPixel(dev_ctx, client_width / 4, client_height / 4);
            ok(color != CLR_INVALID, "Failed to get window color\n");
        }
    }

    trace("fsm_state: %d, color: 0x%.8lX\n", fsm_state, color);

    switch (fsm_state)
    {
    case FSM_STATE_START:

        show_vert_scroll_bar(window);
        fsm_state = FSM_STATE_VSCR_SHOWN;
        break;

    case FSM_STATE_VSCR_SHOWN:

        if (have_hredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW specified, but appearence of vertical scroll bar didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_HREDRAW not specified, but appearence of vertical scroll bar triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        hide_vert_scroll_bar(window);
        fsm_state = FSM_STATE_VSCR_HIDDEN;

        break;

    case FSM_STATE_VSCR_HIDDEN:

        if (have_hredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW specified, but disappearence of vertical scroll bar didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_HREDRAW not specified, but disappearence of vertical scroll bar triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        show_horz_scroll_bar(window);
        fsm_state = FSM_STATE_HSCR_SHOWN;
        break;

    case FSM_STATE_HSCR_SHOWN:

        if (have_vredraw)
        {
            ok(color != prev_color,
               "CS_VREDRAW specified, but appearence of horizontal scroll bar didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_VREDRAW not specified, but appearence of horizontal scroll bar triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        hide_horz_scroll_bar(window);
        fsm_state = FSM_STATE_HSCR_HIDDEN;
        break;

    case FSM_STATE_HSCR_HIDDEN:

        if (have_vredraw)
        {
            ok(color != prev_color,
               "CS_VREDRAW specified, but disappearence of horizontal scroll bar didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_VREDRAW not specified, but disappearence of horizontal scroll bar triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        show_vert_scroll_bar(window);
        show_horz_scroll_bar(window);

        fsm_state = FSM_STATE_BSCR_SHOWN;
        break;

    case FSM_STATE_BSCR_SHOWN:

        if (have_hredraw || have_vredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW or CS_VREDRAW specified, but appearence of both scroll bars didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "Neither CS_HREDRAW nor CS_VREDRAW specified, but appearence of both scroll bars triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        hide_vert_scroll_bar(window);
        hide_horz_scroll_bar(window);

        fsm_state = FSM_STATE_BSCR_HIDDEN;
        break;

    case FSM_STATE_BSCR_HIDDEN:

        if (have_hredraw || have_vredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW or CS_VREDRAW specified, but disappearence of both scroll bars didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "Neither CS_HREDRAW nor CS_VREDRAW specified, but disappearence of both scroll bars triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, small_width, orig_height, SWP_NOMOVE);
        fsm_state = FSM_STATE_WIDTH_SHRUNK;
        break;

    case FSM_STATE_WIDTH_SHRUNK:

        if (have_hredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW specified, but horizontal window shrinkage didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_HREDRAW not specified, but horizontal window shrinkage triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, orig_width, orig_height, SWP_NOMOVE);
        fsm_state = FSM_STATE_WIDTH_EXPANDED;
        break;

    case FSM_STATE_WIDTH_EXPANDED:

        if (have_hredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW specified, but horizontal window expansion didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_HREDRAW not specified, but horizontal window expansion triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, orig_width, small_height, SWP_NOMOVE);
        fsm_state = FSM_STATE_HEIGHT_SHRUNK;
        break;

    case FSM_STATE_HEIGHT_SHRUNK:

        if (have_vredraw)
        {
            ok(color != prev_color,
               "CS_VREDRAW specified, but vertical window shrinkage didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_VREDRAW not specified, but vertical window shrinkage triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, orig_width, orig_height, SWP_NOMOVE);
        fsm_state = FSM_STATE_HEIGHT_EXPANDED;
        break;

    case FSM_STATE_HEIGHT_EXPANDED:

        if (have_vredraw)
        {
            ok(color != prev_color,
               "CS_VREDRAW specified, but vertical window expansion didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "CS_VREDRAW not specified, but vertical window expansion triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, small_width, small_height, SWP_NOMOVE);

        fsm_state = FSM_STATE_BOTH_SHRUNK;
        break;

    case FSM_STATE_BOTH_SHRUNK:

        if (have_hredraw || have_vredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW or CS_VREDRAW specified, but combined vertical/horizontal shrinkage didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "Neither CS_HREDRAW nor CS_VREDRAW specified, but combined vertical/horizontal shrinkage triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        SetWindowPos(window, HWND_TOPMOST, 0, 0, orig_width, orig_height, SWP_NOMOVE);

        fsm_state = FSM_STATE_BOTH_EXPANDED;
        break;

    case FSM_STATE_BOTH_EXPANDED:

        if (have_hredraw || have_vredraw)
        {
            ok(color != prev_color,
               "CS_HREDRAW or CS_VREDRAW specified, but combined vertical/horizontal expansion didn't trigger redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }
        else
        {
            ok(color == prev_color,
               "Neither CS_HREDRAW nor CS_VREDRAW specified, but combined vertical/horizontal expansion triggered unneccessary redraw, prev_color: 0x%.8lX, color: 0x%.8lX\n",
               prev_color, color);
        }

        DestroyWindow(window);
        fsm_state = FSM_STATE_END;
        break;

    case FSM_STATE_END:
        break;
    }

    if (dev_ctx != NULL)
    {
        prev_color = color;
        ReleaseDC(window, dev_ctx);
    }

    return 0;
}

static int
on_resize(HWND window, int new_width, int new_height)
{
    current_color = (current_color + 1) % TEST_COLOR_COUNT;
    SetClassLongPtr(
            window,
            GCLP_HBRBACKGROUND,
            (LONG_PTR)color_brushes[current_color]);

    trace("New window size: %d x %d, new color: 0x%.8lX\n",
          new_width, new_height, colors[current_color]);

    return 0;
}

static LRESULT CALLBACK
proc_cb(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    RECT rect;
    BOOL bool_ret;
    int width;
    int height;

    switch (message)
    {
    case WM_CREATE:

        /* It's important for the test that the entire window is visible. */
        bool_ret = SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ok(bool_ret, "Failed to set window as top-most, code: %ld\n", GetLastError());

        bool_ret = GetClientRect(window, &rect);
        ok(bool_ret, "Failed to retrieve client window dimensions, code: %ld\n", GetLastError());

        if (bool_ret)
        {
            client_width = rect.right;
            client_height = rect.bottom;
        }
        else
        {
            return -1;
        }

        bool_ret = GetWindowRect(window, &rect);
        ok(bool_ret, "Failed to retrieve window dimensions, code: %ld\n", GetLastError());

        if (bool_ret)
        {
            orig_width  = rect.right - rect.left;
            orig_height = rect.bottom - rect.top;
        }
        else
        {
            return -1;
        }

        small_width  = max((orig_width  * 3) / 4, 1);
        small_height = max((orig_height * 3) / 4, 1);
        orig_width   = max(orig_width,  small_width+1);
        orig_height  = max(orig_height, small_height+1);

        trace("orig_width: %d, orig_height: %d, small_width: %d, small_height: %d\n",
              orig_width, orig_height, small_width, small_height);

        hide_vert_scroll_bar(window);
        hide_horz_scroll_bar(window);

        fsm_state = FSM_STATE_START;
        fsm_timer = SetTimer(window, 1, 500, NULL);
        ok(fsm_timer != 0, "Failed to initialize FSM timer, code: %ld\n", GetLastError());

        if (fsm_timer == 0)
        {
            return -1;
        }
        return 0;

    case WM_SIZE:

        width = LOWORD(l_param);
        height = HIWORD(l_param);

        if (width != 0 && height != 0 &&
            (width != client_width || height != client_height))
        {
            return on_resize(window, width, height);

            client_width = width;
            client_height = height;
        }
        return 0;

    case WM_TIMER:

        if (w_param != 0 && w_param == fsm_timer)
        {
            return fsm_transition(window);
        }
        break;

    case WM_NCDESTROY:

        if (fsm_timer != 0)
        {
            KillTimer(window, fsm_timer);
            fsm_timer = 0;
        }
        return 0;

    case WM_DESTROY:

        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(window, message, w_param, l_param);
}
