/*
 * comctl32 month calendar unit tests
 *
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2007 Farshad Agah
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
#include "winuser.h"

#include "commctrl.h"

#include "wine/test.h"
#include <assert.h>
#include <windows.h>
#include "msg.h"

#define expect(expected, got) ok(expected == got, "Expected %d, got %d\n", expected, got);

#define NUM_MSG_SEQUENCES   2
#define PARENT_SEQ_INDEX    0
#define MONTHCAL_SEQ_INDEX  1

struct subclass_info
{
    WNDPROC oldproc;
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_window_seq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_QUERYNEWPALETTE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, 0 },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|defwinproc|optional },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent, /*|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE*/ },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_SIZE, sent },
    { WM_MOVE, sent },
    { 0 }
};

static const struct message create_monthcal_control_seq[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent|optional },
    { WM_GETFONT, sent },
    { WM_PARENTNOTIFY, sent|wparam, WM_CREATE},
    { 0 }
};

static const struct message create_monthcal_multi_sel_style_seq[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent|optional },
    { WM_GETFONT, sent },
    { 0 }
};

static const struct message monthcal_color_seq[] = {
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, 0},

    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_MONTHBK, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_MONTHBK, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_MONTHBK, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_MONTHBK, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_MONTHBK, 0},

    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TEXT, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TEXT, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TEXT, 0},

    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLEBK, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TITLEBK, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLEBK, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TITLEBK, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLEBK, 0},

    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, 0},

    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, RGB(0,0,0)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, 0},
    { MCM_SETCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, RGB(255,255,255)},
    { MCM_GETCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, 0},
    { 0 }
};

static const struct message monthcal_curr_date_seq[] = {
    { MCM_SETCURSEL, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { WM_ERASEBKGND, sent|lparam|defwinproc, 0},
    { MCM_SETCURSEL, sent|wparam, 0},
    { MCM_SETCURSEL, sent|wparam, 0},
    { MCM_GETCURSEL, sent|wparam, 0},
    { MCM_GETCURSEL, sent|wparam|lparam, 0, 0},
    { 0 }
};

static const struct message monthcal_first_day_seq[] = {
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, -5},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, -4},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, -3},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, -2},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, -1},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 1},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 2},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 3},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 4},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 5},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 6},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 7},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 8},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 9},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 10},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},

    { MCM_SETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 11},
    { MCM_GETFIRSTDAYOFWEEK, sent|wparam|lparam, 0, 0},
    { 0 }
};

static const struct message monthcal_unicode_seq[] = {
    { MCM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0},
    { MCM_SETUNICODEFORMAT, sent|wparam|lparam, 1, 0},
    { MCM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0},
    { MCM_SETUNICODEFORMAT, sent|wparam|lparam, 0, 0},
    { MCM_GETUNICODEFORMAT, sent|wparam|lparam, 0, 0},
    { MCM_SETUNICODEFORMAT, sent|wparam|lparam, 1, 0},
    { 0 }
};

static const struct message monthcal_hit_test_seq[] = {
    { MCM_SETCURSEL, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_HITTEST, sent|wparam, 0},
    { 0 }
};

static const struct message monthcal_todaylink_seq[] = {
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_SETTODAY, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_GETTODAY, sent|wparam, 0},
    { WM_LBUTTONDOWN, sent|wparam|lparam, MK_LBUTTON, MAKELONG(70, 370)},
    { WM_CAPTURECHANGED, sent|wparam|lparam|defwinproc, 0, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_GETCURSEL, sent|wparam, 0},
    { 0 }
};

static const struct message monthcal_today_seq[] = {
    { MCM_SETTODAY, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_GETTODAY, sent|wparam, 0},
    { MCM_SETTODAY, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_GETTODAY, sent|wparam, 0},
    { 0 }
};

static const struct message monthcal_scroll_seq[] = {
    { MCM_SETMONTHDELTA, sent|wparam|lparam, 2, 0},
    { MCM_SETMONTHDELTA, sent|wparam|lparam, 3, 0},
    { MCM_GETMONTHDELTA, sent|wparam|lparam, 0, 0},
    { MCM_SETMONTHDELTA, sent|wparam|lparam, 12, 0},
    { MCM_GETMONTHDELTA, sent|wparam|lparam, 0, 0},
    { MCM_SETMONTHDELTA, sent|wparam|lparam, 15, 0},
    { MCM_GETMONTHDELTA, sent|wparam|lparam, 0, 0},
    { MCM_SETMONTHDELTA, sent|wparam|lparam, -5, 0},
    { MCM_GETMONTHDELTA, sent|wparam|lparam, 0, 0},
    { 0 }
};

static const struct message monthcal_monthrange_seq[] = {
    { MCM_GETMONTHRANGE, sent|wparam, GMR_VISIBLE},
    { MCM_GETMONTHRANGE, sent|wparam, GMR_DAYSTATE},
    { 0 }
};

static const struct message monthcal_max_sel_day_seq[] = {
    { MCM_SETMAXSELCOUNT, sent|wparam|lparam, 5, 0},
    { MCM_GETMAXSELCOUNT, sent|wparam|lparam, 0, 0},
    { MCM_SETMAXSELCOUNT, sent|wparam|lparam, 15, 0},
    { MCM_GETMAXSELCOUNT, sent|wparam|lparam, 0, 0},
    { MCM_SETMAXSELCOUNT, sent|wparam|lparam, -1, 0},
    { MCM_GETMAXSELCOUNT, sent|wparam|lparam, 0, 0},
    { 0 }
};

/* expected message sequence for parent*/
static const struct message destroy_monthcal_parent_msgs_seq[] = {
    { WM_PARENTNOTIFY, sent|wparam, WM_DESTROY},
    { 0 }
};

/* expected message sequence for child*/
static const struct message destroy_monthcal_child_msgs_seq[] = {
    { 0x0090, sent|optional }, /* Vista */
    { WM_SHOWWINDOW, sent|wparam|lparam, 0, 0},
    { WM_WINDOWPOSCHANGING, sent|wparam, 0},
    { WM_WINDOWPOSCHANGED, sent|wparam, 0},
    { WM_DESTROY, sent|wparam|lparam, 0, 0},
    { WM_NCDESTROY, sent|wparam|lparam, 0, 0},
    { 0 }
};

static const struct message destroy_monthcal_multi_sel_style_seq[] = {
    { 0x0090, sent|optional }, /* Vista */
    { WM_DESTROY, sent|wparam|lparam, 0, 0},
    { WM_NCDESTROY, sent|wparam|lparam, 0, 0},
    { 0 }
};

/* expected message sequence for parent window*/
static const struct message destroy_parent_seq[] = {
    { 0x0090, sent|optional }, /* Vista */
    { WM_WINDOWPOSCHANGING, sent|wparam, 0},
    { WM_WINDOWPOSCHANGED, sent|wparam, 0},
    { WM_NCACTIVATE, sent|wparam, 0},
    { WM_ACTIVATE, sent|wparam, 0},
    { WM_NCACTIVATE, sent|wparam|lparam|optional, 0, 0},
    { WM_ACTIVATE, sent|wparam|lparam|optional, 0, 0},
    { WM_ACTIVATEAPP, sent|wparam, 0},
    { WM_KILLFOCUS, sent|wparam|lparam, 0, 0},
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0},
    { WM_IME_NOTIFY, sent|wparam|lparam|defwinproc|optional, 1, 0},
    { WM_DESTROY, sent|wparam|lparam, 0, 0},
    { WM_NCDESTROY, sent|wparam|lparam, 0, 0},
    { 0 }
};

static void test_monthcal(void)
{
    HWND hwnd;
    SYSTEMTIME st[2], st1[2];
    int res, month_range;

    hwnd = CreateWindowA(MONTHCAL_CLASSA, "MonthCal", WS_POPUP | WS_VISIBLE, CW_USEDEFAULT,
                         0, 300, 300, 0, 0, NULL, NULL);
    ok(hwnd != NULL, "Failed to create MonthCal\n");
    GetSystemTime(&st[0]);
    st[1] = st[0];

    /* Invalid date/time */
    st[0].wYear  = 2000;
    /* Time should not matter */
    st[1].wHour = st[1].wMinute = st[1].wSecond = 70;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set MAX limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");

    st[1].wMonth = 0;
    ok(!SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Should have failed to set limits\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");
    ok(!SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Should have failed to set MAX limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lover limit changed\n");

    GetSystemTime(&st[0]);
    st[0].wDay = 20;
    st[0].wMonth = 5;
    st[1] = st[0];

    month_range = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    st[1].wMonth--;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == (GDTR_MIN|GDTR_MAX), "Limits should be set\n");

    st[1].wMonth += 2;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);

    st[1].wYear --;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    st[1].wYear += 1;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");

    st[1].wMonth -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "Only MAX limit should be set\n");
    st[1].wMonth += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessage(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "Only MAX limit should be set\n");

    DestroyWindow(hwnd);
}

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* log system messages, except for painting */
    if (message < WM_USER &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        trace("parent: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL register_parent_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Month-Cal test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    HWND hwnd;

    InitCommonControls();

    /* flush message sequences, so we can check the new sequence by the end of function */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    if (!register_parent_wnd_class())
        return NULL;

    hwnd = CreateWindowEx(0, "Month-Cal test parent class",
                          "Month-Cal test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 500, 500,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);

    assert(hwnd);

    /* check for message sequences */
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_parent_window_seq, "create parent window", FALSE);

    return hwnd;
}

static LRESULT WINAPI monthcal_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, MONTHCAL_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_monthcal_control(DWORD style, HWND parent_window)
{
    struct subclass_info *info;
    HWND hwnd;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    hwnd = CreateWindowEx(0,
                    MONTHCAL_CLASS,
                    "",
                    style,
                    0, 0, 300, 400,
                    parent_window, NULL, GetModuleHandleA(NULL), NULL);

    if (!hwnd)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                            (LONG_PTR)monthcal_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)info);

    return hwnd;
}


/* Setter and Getters Tests */

static void test_monthcal_color(HWND hwnd)
{
    int res, temp;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for color*/
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(255,255,255), temp);

    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(0,0,0));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(0,0,0), temp);
    res = SendMessage(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(255,255,255));
    expect(temp, res);
    temp = SendMessage(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(255,255,255), temp);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_color_seq, "monthcal color", FALSE);
}

static void test_monthcal_currDate(HWND hwnd)
{
    SYSTEMTIME st_original, st_new, st_test;
    int res;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for current date selected */
    st_original.wYear = 2000;
    st_original.wMonth = 11;
    st_original.wDay = 28;
    st_original.wHour = 11;
    st_original.wMinute = 59;
    st_original.wSecond = 30;
    st_original.wMilliseconds = 0;
    st_original.wDayOfWeek = 0;

    st_new = st_test = st_original;

    /* Should not validate the time */
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(1,res);

    /* Overflow matters, check for wDay */
    st_test.wDay += 4;
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(0,res);

    /* correct wDay before checking for wMonth */
    st_test.wDay -= 4;
    expect(st_original.wDay, st_test.wDay);

    /* Overflow matters, check for wMonth */
    st_test.wMonth += 4;
    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(0,res);

    /* checking if gets the information right, modify st_new */
    st_new.wYear += 4;
    st_new.wMonth += 4;
    st_new.wDay += 4;
    st_new.wHour += 4;
    st_new.wMinute += 4;
    st_new.wSecond += 4;

    res = SendMessage(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_new change to st_origin, above settings with overflow */
    /* should not change the current settings */
    expect(st_original.wYear, st_new.wYear);
    expect(st_original.wMonth, st_new.wMonth);
    expect(st_original.wDay, st_new.wDay);
    expect(st_original.wHour, st_new.wHour);
    expect(st_original.wMinute, st_new.wMinute);
    expect(st_original.wSecond, st_new.wSecond);

    /* lparam cannot be NULL */
    res = SendMessage(hwnd, MCM_GETCURSEL, 0, 0);
    expect(0, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_curr_date_seq, "monthcal currDate", TRUE);
}

static void test_monthcal_firstDay(HWND hwnd)
{
    int res, fday, i, prev;
    TCHAR b[128];
    LCID lcid = LOCALE_USER_DEFAULT;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for first day of week */
    /* check for locale first day */
    if(GetLocaleInfo(lcid, LOCALE_IFIRSTDAYOFWEEK, b, 128)){
        fday = atoi(b);
        trace("fday: %d\n", fday);
        res = SendMessage(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
        expect(fday, res);
        prev = fday;

        /* checking for the values that actually will be stored as */
        /* current first day when we set a new value */
        for (i = -5; i < 12; i++){
            res = SendMessage(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM) i);
            expect(prev, res);
            res = SendMessage(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
            prev = res;

            if (i == -1){
                expect(MAKELONG(fday, FALSE), res);
            }else if (i >= 7){
                /* out of range sets max first day of week, locale is ignored */
                expect(MAKELONG(6, TRUE), res);
            }else{
                expect(MAKELONG(i, TRUE), res);
            }
        }

        ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_first_day_seq, "monthcal firstDay", FALSE);

    }else{
        skip("Cannot retrieve first day of the week\n");
    }

}

static void test_monthcal_unicode(HWND hwnd)
{
    int res, temp;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for Unicode format */

    /* getting the current settings */
    temp = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);

    /* setting to 1, should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(temp, res);

    /* current setting is 1, so, should return 1 */
    res = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    todo_wine {expect(1, res);}

    /* setting to 0, should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 0, 0);
    todo_wine {expect(1, res);}

    /* current setting is 0, so, it should return 0 */
    res = SendMessage(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    expect(0, res);

    /* should return previous settings */
    res = SendMessage(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(0, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_unicode_seq, "monthcal unicode", FALSE);
}

static void test_monthcal_HitTest(HWND hwnd)
{
    MCHITTESTINFO mchit;
    UINT res;
    SYSTEMTIME st;
    LONG x;
    UINT title_index;
    static const UINT title_hits[] =
        { MCHT_NOWHERE, MCHT_TITLEBK, MCHT_TITLEBTNPREV, MCHT_TITLEBK,
          MCHT_TITLEMONTH, MCHT_TITLEBK, MCHT_TITLEYEAR, MCHT_TITLEBK,
          MCHT_TITLEBTNNEXT, MCHT_TITLEBK, MCHT_NOWHERE };

    memset(&mchit, 0, sizeof(MCHITTESTINFO));

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    st.wYear = 2007;
    st.wMonth = 4;
    st.wDay = 11;
    st.wHour = 1;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    st.wDayOfWeek = 0;

    res = SendMessage(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st);
    expect(1,res);

    /* (0, 0) is the top left of the control and should not be active */
    mchit.cbSize = sizeof(MCHITTESTINFO);
    mchit.pt.x = 0;
    mchit.pt.y = 0;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(0, mchit.pt.x);
    expect(0, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (300, 400) is the bottom right of the control and should not be active */
    mchit.pt.x = 300;
    mchit.pt.y = 400;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(300, mchit.pt.x);
    expect(400, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (500, 500) is completely out of the control and should not be active */
    mchit.pt.x = 500;
    mchit.pt.y = 500;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(500, mchit.pt.x);
    expect(500, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_NOWHERE, res);}

    /* (120, 180) is in active area - calendar background */
    mchit.pt.x = 120;
    mchit.pt.y = 180;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(120, mchit.pt.x);
    expect(180, mchit.pt.y);
    expect(mchit.uHit, res);
    expect(MCHT_CALENDARBK, res);

    /* (70, 70) is in active area - day of the week */
    mchit.pt.x = 70;
    mchit.pt.y = 70;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(70, mchit.pt.x);
    expect(70, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_CALENDARDAY, res);}

    /* (70, 90) is in active area - date from prev month */
    mchit.pt.x = 70;
    mchit.pt.y = 90;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(70, mchit.pt.x);
    expect(90, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_CALENDARDATEPREV, res);}

#if 0
    /* (125, 115) is in active area - date from this month */
    mchit.pt.x = 125;
    mchit.pt.y = 115;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(125, mchit.pt.x);
    expect(115, mchit.pt.y);
    expect(mchit.uHit, res);
    expect(MCHT_CALENDARDATE, res);
#endif

    /* (80, 220) is in active area - background section of the title */
    mchit.pt.x = 80;
    mchit.pt.y = 220;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(80, mchit.pt.x);
    expect(220, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TITLEBK, res);}

    /* (140, 215) is in active area - month section of the title */
    mchit.pt.x = 140;
    mchit.pt.y = 215;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(140, mchit.pt.x);
    expect(215, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TITLEMONTH, res);}

    /* (170, 215) is in active area - year section of the title */
    mchit.pt.x = 170;
    mchit.pt.y = 215;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(170, mchit.pt.x);
    expect(215, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TITLEYEAR, res);}

    /* (150, 260) is in active area - date from this month */
    mchit.pt.x = 150;
    mchit.pt.y = 260;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(150, mchit.pt.x);
    expect(260, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_CALENDARDATE, res);}

    /* (150, 350) is in active area - date from next month */
    mchit.pt.x = 150;
    mchit.pt.y = 350;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(150, mchit.pt.x);
    expect(350, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_CALENDARDATENEXT, res);}

    /* (150, 370) is in active area - today link */
    mchit.pt.x = 150;
    mchit.pt.y = 370;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(150, mchit.pt.x);
    expect(370, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TODAYLINK, res);}

    /* (70, 370) is in active area - today link */
    mchit.pt.x = 70;
    mchit.pt.y = 370;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(70, mchit.pt.x);
    expect(370, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TODAYLINK, res);}

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_hit_test_seq, "monthcal hit test", TRUE);

    /* The horizontal position of title bar elements depends on locale (y pos
       is constant), so we sample across a horizontal line and make sure we
       find all elements. */
    mchit.pt.y = 40;
    title_index = 0;
    for (x = 0; x < 300; x++){
        mchit.pt.x = x;
        res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
        expect(x, mchit.pt.x);
        expect(40, mchit.pt.y);
        expect(mchit.uHit, res);
        if (res != title_hits[title_index]){
            title_index++;
            if (sizeof(title_hits) / sizeof(title_hits[0]) <= title_index)
                break;
            todo_wine {expect(title_hits[title_index], res);}
        }
    }
    todo_wine {ok(300 <= x && title_index + 1 == sizeof(title_hits) / sizeof(title_hits[0]),
        "Wrong title layout\n");}
}

static void test_monthcal_todaylink(HWND hwnd)
{
    MCHITTESTINFO mchit;
    SYSTEMTIME st_test, st_new;
    BOOL error = FALSE;
    UINT res;

    memset(&mchit, 0, sizeof(MCHITTESTINFO));

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* (70, 370) is in active area - today link */
    mchit.cbSize = sizeof(MCHITTESTINFO);
    mchit.pt.x = 70;
    mchit.pt.y = 370;
    res = SendMessage(hwnd, MCM_HITTEST, 0, (LPARAM) & mchit);
    expect(70, mchit.pt.x);
    expect(370, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine {expect(MCHT_TODAYLINK, res);}
    if (70 != mchit.pt.x || 370 != mchit.pt.y || mchit.uHit != res
        || MCHT_TODAYLINK != res)
        error = TRUE;

    st_test.wDay = 1;
    st_test.wMonth = 1;
    st_test.wYear = 2005;
    memset(&st_new, 0, sizeof(SYSTEMTIME));

    SendMessage(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);

    res = SendMessage(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);
    expect(1, st_new.wDay);
    expect(1, st_new.wMonth);
    expect(2005, st_new.wYear);
    if (1 != res || 1 != st_new.wDay || 1 != st_new.wMonth
        || 2005 != st_new.wYear)
        error = TRUE;

    if (error) {
        skip("cannot perform today link test\n");
        return;
    }

    res = SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(70, 370));
    expect(0, res);

    memset(&st_new, 0, sizeof(SYSTEMTIME));
    res = SendMessage(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);
    expect(1, st_new.wDay);
    expect(1, st_new.wMonth);
    expect(2005, st_new.wYear);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_todaylink_seq, "monthcal hit test", TRUE);
}

static void test_monthcal_today(HWND hwnd)
{
    SYSTEMTIME st_test, st_new;
    int res;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for "today" information */

    /* check for overflow, should be ok */
    st_test.wDay = 38;
    st_test.wMonth = 38;

    st_new.wDay = 27;
    st_new.wMonth = 27;

    SendMessage(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);

    res = SendMessage(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_test should not change */
    expect(38, st_test.wDay);
    expect(38, st_test.wMonth);

    /* st_new should change, overflow does not matter */
    expect(38, st_new.wDay);
    expect(38, st_new.wMonth);

    /* check for zero, should be ok*/
    st_test.wDay = 0;
    st_test.wMonth = 0;

    SendMessage(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);

    res = SendMessage(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_test should not change */
    expect(0, st_test.wDay);
    expect(0, st_test.wMonth);

    /* st_new should change to zero*/
    expect(0, st_new.wDay);
    expect(0, st_new.wMonth);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_today_seq, "monthcal today", TRUE);
}

static void test_monthcal_scroll(HWND hwnd)
{
    int res;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for scroll rate */
    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 2, 0);
    expect(0, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 3, 0);
    expect(2, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(3, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 12, 0);
    expect(3, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(12, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, 15, 0);
    expect(12, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(15, res);

    res = SendMessage(hwnd, MCM_SETMONTHDELTA, -5, 0);
    expect(15, res);
    res = SendMessage(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(-5, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_scroll_seq, "monthcal scroll", FALSE);
}

static void test_monthcal_monthrange(HWND hwnd)
{
    int res;
    SYSTEMTIME st_visible[2], st_daystate[2];

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    st_visible[0].wYear = 0;
    st_visible[0].wMonth = 0;
    st_visible[0].wDay = 0;
    st_daystate[1] = st_daystate[0] = st_visible[1] = st_visible[0];

    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st_visible);
    todo_wine {
        expect(2, res);
        expect(2000, st_visible[0].wYear);
        expect(11, st_visible[0].wMonth);
        expect(1, st_visible[0].wDay);
        expect(2000, st_visible[1].wYear);
        expect(12, st_visible[1].wMonth);
        expect(31, st_visible[1].wDay);
    }
    res = SendMessage(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE, (LPARAM)st_daystate);
    todo_wine {
        expect(4, res);
        expect(2000, st_daystate[0].wYear);
        expect(10, st_daystate[0].wMonth);
        expect(29, st_daystate[0].wDay);
        expect(2001, st_daystate[1].wYear);
        expect(1, st_daystate[1].wMonth);
        expect(6, st_daystate[1].wDay);
    }

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_monthrange_seq, "monthcal monthrange", FALSE);
}

static void test_monthcal_MaxSelDay(HWND hwnd)
{
    int res;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for max selected days */
    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, 5, 0);
    expect(1, res);
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(5, res);

    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, 15, 0);
    expect(1, res);
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(15, res);

    res = SendMessage(hwnd, MCM_SETMAXSELCOUNT, -1, 0);
    todo_wine {expect(0, res);}
    res = SendMessage(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    todo_wine {expect(15, res);}

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_max_sel_day_seq, "monthcal MaxSelDay", FALSE);
}

static void test_monthcal_size(HWND hwnd)
{
    int res;
    RECT r1, r2;
    HFONT hFont1, hFont2;
    LOGFONTA logfont;

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    hFont1 = CreateFontIndirectA(&logfont);

    logfont.lfHeight = 24;
    hFont2 = CreateFontIndirectA(&logfont);

    /* initialize to a font we can compare against */
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont1, 0);
    res = SendMessage(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r1);

    /* check that setting a larger font results in an larger rect */
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont2, 0);
    res = SendMessage(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r2);

    OffsetRect(&r1, -r1.left, -r1.top);
    OffsetRect(&r2, -r2.left, -r2.top);

    ok(r1.bottom < r2.bottom, "Failed to get larger rect with larger font\n");
}

START_TEST(monthcal)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
    INITCOMMONCONTROLSEX iccex;
    HWND hwnd, parent_wnd;

    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (!pInitCommonControlsEx)
    {
        skip("InitCommonControlsEx() is missing. Skipping the tests\n");
        return;
    }
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_DATE_CLASSES;
    pInitCommonControlsEx(&iccex);

    test_monthcal();

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = create_parent_window();

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hwnd = create_monthcal_control(WS_CHILD | WS_BORDER | WS_VISIBLE, parent_wnd);
    assert(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_monthcal_control_seq, "create monthcal control", TRUE);

    SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);

    test_monthcal_color(hwnd);
    test_monthcal_currDate(hwnd);
    test_monthcal_firstDay(hwnd);
    test_monthcal_unicode(hwnd);
    test_monthcal_today(hwnd);
    test_monthcal_scroll(hwnd);
    test_monthcal_monthrange(hwnd);
    test_monthcal_HitTest(hwnd);
    test_monthcal_todaylink(hwnd);
    test_monthcal_size(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, destroy_monthcal_parent_msgs_seq, "Destroy monthcal (parent msg)", FALSE);
    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, destroy_monthcal_child_msgs_seq, "Destroy monthcal (child msg)", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hwnd = create_monthcal_control(MCS_MULTISELECT, parent_wnd);
    assert(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_monthcal_multi_sel_style_seq, "create monthcal (multi sel style)", TRUE);

    test_monthcal_MaxSelDay(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, destroy_monthcal_multi_sel_style_seq, "Destroy monthcal (multi sel style)", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(parent_wnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, destroy_parent_seq, "Destroy parent window", FALSE);
}
