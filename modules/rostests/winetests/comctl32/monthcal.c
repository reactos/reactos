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
#include "v6util.h"
#include <windows.h>
#include "msg.h"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}

#define expect_hex(expected,got) expect_hex_(__LINE__, expected, got)
static inline void expect_hex_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %lx, got %lx\n", expected, got);
}
#define expect_d(expected, got) ok(abs((expected) - (got)) <= 2, "Expected %ld, got %ld\n", expected, got);

#define NUM_MSG_SEQUENCES   2
#define PARENT_SEQ_INDEX    0
#define MONTHCAL_SEQ_INDEX  1

#define SEL_NOTIFY_TEST_ID  100

static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static HWND parent_wnd;
static BOOL got_MCN_SELECT, got_MCN_SELCHANGE;

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
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
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
    { WM_PARENTNOTIFY, sent },
    { 0 }
};

static const struct message monthcal_curr_date_seq[] = {
    { MCM_SETCURSEL, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
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
    { 0 }
};

static const struct message monthcal_todaylink_seq[] = {
    { MCM_HITTEST, sent|wparam, 0},
    { MCM_SETTODAY, sent|wparam, 0},
    { WM_PAINT, sent|wparam|lparam|defwinproc, 0, 0},
    { MCM_GETTODAY, sent|wparam, 0},
    { WM_LBUTTONDOWN, sent|wparam, MK_LBUTTON},
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
    { WM_SHOWWINDOW, sent|wparam|lparam, 0, 0},
    { WM_WINDOWPOSCHANGING, sent|wparam, 0},
    { WM_WINDOWPOSCHANGED, sent|wparam, 0},
    { WM_DESTROY, sent|wparam|lparam, 0, 0},
    { WM_NCDESTROY, sent|wparam|lparam, 0, 0},
    { 0 }
};

static void test_monthcal(void)
{
    HWND hwnd;
    SYSTEMTIME st[2], st1[2], today;
    int res, month_range;
    DWORD limits;
    BOOL r;

    hwnd = CreateWindowA(MONTHCAL_CLASSA, "MonthCal", WS_POPUP | WS_VISIBLE, CW_USEDEFAULT,
                         0, 300, 300, 0, 0, NULL, NULL);
    ok(hwnd != NULL, "Failed to create MonthCal\n");

    /* test range just after creation */
    memset(&st, 0xcc, sizeof(st));
    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st);
    ok(limits == 0 ||
       broken(limits == GDTR_MIN), /* comctl32 <= 4.70 */
       "No limits should be set (%ld)\n", limits);
    if (limits == GDTR_MIN)
    {
        win_skip("comctl32 <= 4.70 is broken\n");
        DestroyWindow(hwnd);
        return;
    }

    ok(0 == st[0].wYear ||
       broken(1752 == st[0].wYear), /* comctl32 <= 4.72 */
       "Expected 0, got %d\n", st[0].wYear);
    ok(0 == st[0].wMonth ||
       broken(9 == st[0].wMonth), /* comctl32 <= 4.72 */
       "Expected 0, got %d\n", st[0].wMonth);
    ok(0 == st[0].wDay ||
       broken(14 == st[0].wDay), /* comctl32 <= 4.72 */
       "Expected 0, got %d\n", st[0].wDay);
    expect(0, st[0].wDayOfWeek);
    expect(0, st[0].wHour);
    expect(0, st[0].wMinute);
    expect(0, st[0].wSecond);
    expect(0, st[0].wMilliseconds);

    expect(0, st[1].wYear);
    expect(0, st[1].wMonth);
    expect(0, st[1].wDay);
    expect(0, st[1].wDayOfWeek);
    expect(0, st[1].wHour);
    expect(0, st[1].wMinute);
    expect(0, st[1].wSecond);
    expect(0, st[1].wMilliseconds);

    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, 0);
    ok(limits == 0, "got %lu\n", limits);

    GetSystemTime(&st[0]);
    st[1] = st[0];

    SendMessageA(hwnd, MCM_GETTODAY, 0, (LPARAM)&today);

    /* Invalid date/time */
    st[0].wYear  = 2000;
    /* Time should not matter */
    st[1].wHour = st[1].wMinute = st[1].wSecond = 70;
    st[1].wMilliseconds = 1200;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set MAX limit\n");
    /* invalid timestamp is written back with today data and msecs untouched */
    expect(today.wHour, st[1].wHour);
    expect(today.wMinute, st[1].wMinute);
    expect(today.wSecond, st[1].wSecond);
    expect(1200, st[1].wMilliseconds);

    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lower limit changed\n");
    /* invalid timestamp should be replaced with today data, except msecs */
    expect(today.wHour, st1[1].wHour);
    expect(today.wMinute, st1[1].wMinute);
    expect(today.wSecond, st1[1].wSecond);
    expect(1200, st1[1].wMilliseconds);

    /* Invalid date/time with invalid milliseconds only */
    GetSystemTime(&st[0]);
    st[1] = st[0];
    /* Time should not matter */
    st[1].wMilliseconds = 1200;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set MAX limit\n");
    /* invalid milliseconds field doesn't lead to invalid timestamp */
    expect(st[0].wHour,   st[1].wHour);
    expect(st[0].wMinute, st[1].wMinute);
    expect(st[0].wSecond, st[1].wSecond);
    expect(1200, st[1].wMilliseconds);

    GetSystemTime(&st[0]);

    st[1].wMonth = 0;
    ok(!SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st),
            "Should have failed to set limits\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lower limit changed\n");
    ok(!SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st),
            "Should have failed to set MAX limit\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "No limits should be set\n");
    ok(st1[0].wYear != 2000, "Lower limit changed\n");

    GetSystemTime(&st[0]);
    st[0].wDay = 20;
    st[0].wMonth = 5;
    st[1] = st[0];

    month_range = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    st[1].wMonth--;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st),
            "Failed to set both min and max limits\n");
    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == (GDTR_MIN|GDTR_MAX),
            "Limits should be set\n");

    st[1].wMonth += 2;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st),
            "Failed to set both min and max limits\n");
    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st1);
    ok(res == month_range, "Invalid month range (%d)\n", res);

    st[1].wYear --;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st),
            "Failed to set both min and max limits\n");
    st[1].wYear += 1;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st),
            "Failed to set both min and max limits\n");

    st[1].wMonth -= 3;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX,
            "Only MAX limit should be set\n");
    st[1].wMonth += 4;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear -= 3;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear += 4;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX,
            "Only MAX limit should be set\n");

    /* set both limits, then set max < min */
    GetSystemTime(&st[0]);
    st[0].wDay = 25;
    st[1] = st[0];
    st[1].wYear++;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN|GDTR_MAX, (LPARAM)st), "Failed to set limits\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == (GDTR_MIN|GDTR_MAX),
            "Min limit expected\n");
    st[1].wYear -= 2;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set limits\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MAX, "Max limit expected\n");

    expect(0, st1[0].wYear);
    expect(0, st1[0].wMonth);
    expect(0, st1[0].wDay);
    expect(0, st1[0].wDayOfWeek);
    expect(0, st1[0].wHour);
    expect(0, st1[0].wMinute);
    expect(0, st1[0].wSecond);
    expect(0, st1[0].wMilliseconds);

    expect(st[1].wYear,      st1[1].wYear);
    expect(st[1].wMonth,     st1[1].wMonth);
    expect(st[1].wDay,       st1[1].wDay);
    expect(st[1].wDayOfWeek, st1[1].wDayOfWeek);
    expect(st[1].wHour,      st1[1].wHour);
    expect(st[1].wMinute,    st1[1].wMinute);
    expect(st[1].wSecond,    st1[1].wSecond);
    expect(st[1].wMilliseconds, st1[1].wMilliseconds);

    st[1] = st[0];
    st[1].wYear++;
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN|GDTR_MAX, (LPARAM)st), "Failed to set limits\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == (GDTR_MIN|GDTR_MAX),
            "Min limit expected\n");
    st[0].wYear++; /* start == end now */
    ok(SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN, (LPARAM)st), "Failed to set limits\n");
    ok(SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1) == GDTR_MIN, "Min limit expected\n");

    expect(st[0].wYear,      st1[0].wYear);
    expect(st[0].wMonth,     st1[0].wMonth);
    expect(st[0].wDay,       st1[0].wDay);
    expect(st[0].wDayOfWeek, st1[0].wDayOfWeek);
    expect(st[0].wHour,      st1[0].wHour);
    expect(st[0].wMinute,    st1[0].wMinute);
    expect(st[0].wSecond,    st1[0].wSecond);
    expect(st[0].wMilliseconds, st1[0].wMilliseconds);

    expect(0, st1[1].wYear);
    expect(0, st1[1].wMonth);
    expect(0, st1[1].wDay);
    expect(0, st1[1].wDayOfWeek);
    expect(0, st1[1].wHour);
    expect(0, st1[1].wMinute);
    expect(0, st1[1].wSecond);
    expect(0, st1[1].wMilliseconds);

    /* 0 limit flags */
    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1);
    ok(limits == GDTR_MIN, "got 0x%08lx\n", limits);

    GetSystemTime(st);
    st[0].wDay = 25;
    st[1] = st[0];
    st[1].wYear++;
    r = SendMessageA(hwnd, MCM_SETRANGE, 0, (LPARAM)st);
    ok(r, "got %d\n", r);

    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st);
    ok(limits == 0, "got 0x%08lx\n", limits);
    ok(st[0].wYear == 0 && st[1].wYear == 0, "got %u, %u\n", st[0].wYear, st[1].wYear);

    /* flags are 0, set min limit */
    GetSystemTime(st);
    st[0].wDay = 25;
    st[1] = st[0];
    st[1].wYear++;

    r = SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN, (LPARAM)st);
    ok(r, "got %d\n", r);

    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1);
    ok(limits == GDTR_MIN, "got 0x%08lx\n", limits);
    ok(st1[1].wYear == 0, "got %u\n", st1[1].wYear);

    /* now set max limit, check flags */
    r = SendMessageA(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st);
    ok(r, "got %d\n", r);

    limits = SendMessageA(hwnd, MCM_GETRANGE, 0, (LPARAM)st1);
    ok(limits == GDTR_MAX, "got 0x%08lx\n", limits);
    ok(st1[0].wYear == 0, "got %u\n", st1[0].wYear);

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
        message != WM_DEVICECHANGE &&
        message != WM_IME_SETCONTEXT &&
        message != WM_IME_NOTIFY)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    if (message == WM_NOTIFY)
    {
        NMHDR *hdr = (NMHDR*)lParam;
        switch (hdr->code)
        {
          case MCN_GETDAYSTATE:
          {
            NMDAYSTATE *nmstate = (NMDAYSTATE*)lParam;
            static MONTHDAYSTATE months[14] = { 0 };

            ok(nmstate->cDayState > 0, "got %d\n", nmstate->cDayState);
            ok(nmstate->cDayState <= 14, "got %d\n", nmstate->cDayState);
            ok(nmstate->prgDayState != NULL, "got %p\n", nmstate->prgDayState);

            nmstate->prgDayState = months;

            return TRUE;
          }
          case MCN_SELECT:
          case MCN_SELCHANGE:
          {
            NMSELCHANGE *nmchg = (NMSELCHANGE*)lParam;
            SYSTEMTIME st[2];
            BOOL is_multisel = GetWindowLongPtrA(nmchg->nmhdr.hwndFrom, GWL_STYLE) & MCS_MULTISELECT;

            if(GetWindowLongPtrA(nmchg->nmhdr.hwndFrom, GWLP_ID) != SEL_NOTIFY_TEST_ID)
              break;

            if (hdr->code == MCN_SELECT)
                got_MCN_SELECT = TRUE;
            else
                got_MCN_SELCHANGE = TRUE;

            SendMessageA(nmchg->nmhdr.hwndFrom, is_multisel ? MCM_GETSELRANGE : MCM_GETCURSEL,
                    0, (LPARAM)st);

            expect(st[0].wYear,  nmchg->stSelStart.wYear);
            expect(st[0].wMonth, nmchg->stSelStart.wMonth);
            expect(0,            nmchg->stSelStart.wDayOfWeek);
            expect(st[0].wDay,   nmchg->stSelStart.wDay);

            if(is_multisel)
            {
              expect(st[1].wYear,  nmchg->stSelEnd.wYear);
              expect(st[1].wMonth, nmchg->stSelEnd.wMonth);
              expect(0,            nmchg->stSelEnd.wDayOfWeek);
              expect(st[1].wDay,   nmchg->stSelEnd.wDay);
            }
            else
              ok(!(nmchg->stSelEnd.wYear | nmchg->stSelEnd.wMonth |
                        nmchg->stSelEnd.wDayOfWeek | nmchg->stSelEnd.wDay |
                        nmchg->stSelEnd.wHour | nmchg->stSelEnd.wMinute |
                        nmchg->stSelEnd.wSecond | nmchg->stSelEnd.wMilliseconds),
                        "Non-zero member in stSelEnd\n");
            return TRUE;
          }
          default:
            break;
        }
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Month-Cal test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    HWND hwnd;

    /* flush message sequences, so we can check the new sequence by the end of function */
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    if (!register_parent_wnd_class())
        return NULL;

    hwnd = CreateWindowExA(0, "Month-Cal test parent class", "Month-Cal test parent window",
            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
            0, 0, 500, 500, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "failed to create parent wnd\n");

    /* check for message sequences */
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_parent_window_seq, "create parent window", FALSE);

    return hwnd;
}

static LRESULT WINAPI monthcal_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, MONTHCAL_SEQ_INDEX, &msg);

    /* some debug output for style changing */
    if ((message == WM_STYLECHANGING ||
         message == WM_STYLECHANGED) && lParam)
    {
        STYLESTRUCT *style = (STYLESTRUCT*)lParam;
        trace("\told style: 0x%08lx, new style: 0x%08lx\n", style->styleOld, style->styleNew);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_monthcal_control(DWORD style)
{
    WNDPROC oldproc;
    RECT rect;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA(0, MONTHCAL_CLASSA, "", WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                    0, 0, 300, 400, parent_wnd, NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "failed to create monthcal wnd\n");
    if (!hwnd) return NULL;

    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC,
                                        (LONG_PTR)monthcal_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

    SendMessageA(hwnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);

    /* make sure calendar grid is 2x1 */
    ret = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&rect);
    ok(ret, "got %d\n", ret);

    ret = SetWindowPos(hwnd, NULL, 0, 0, rect.right * 5 / 2, rect.bottom * 3 / 2, SWP_NOMOVE);
    ok(ret, "got %d\n", ret);

    return hwnd;
}


/* Setter and Getters Tests */

static void test_color(void)
{
    COLORREF color, prev;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

    /* invalid color index */
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT + 1, 0);
    expect(~0u, color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT + 1, RGB(255,255,255));
    expect(~0u, prev);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_BACKGROUND, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_BACKGROUND, 0);
    expect(RGB(255,255,255), color);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_MONTHBK, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_MONTHBK, 0);
    expect(RGB(255,255,255), color);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TEXT, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TEXT, 0);
    expect(RGB(255,255,255), color);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TITLEBK, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLEBK, 0);
    expect(RGB(255,255,255), color);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TITLETEXT, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TITLETEXT, 0);
    expect(RGB(255,255,255), color);

    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(0,0,0));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(0,0,0), color);
    prev = SendMessageA(hwnd, MCM_SETCOLOR, MCSC_TRAILINGTEXT, RGB(255,255,255));
    expect(color, prev);
    color = SendMessageA(hwnd, MCM_GETCOLOR, MCSC_TRAILINGTEXT, 0);
    expect(RGB(255,255,255), color);

    DestroyWindow(hwnd);
}

static void test_currdate(void)
{
    SYSTEMTIME st_original, st_new, st_test;
    int res;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

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
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(1,res);

    /* Overflow matters, check for wDay */
    st_test.wDay += 4;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(0,res);

    /* correct wDay before checking for wMonth */
    st_test.wDay -= 4;
    expect(st_original.wDay, st_test.wDay);

    /* Overflow matters, check for wMonth */
    st_test.wMonth += 4;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(0,res);

    /* checking if gets the information right, modify st_new */
    st_new.wYear += 4;
    st_new.wMonth += 4;
    st_new.wDay += 4;
    st_new.wHour += 4;
    st_new.wMinute += 4;
    st_new.wSecond += 4;

    res = SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_new change to st_origin, above settings with overflow */
    /* should not change the current settings */
    expect(st_original.wYear, st_new.wYear);
    expect(st_original.wMonth, st_new.wMonth);
    expect(st_original.wDay, st_new.wDay);
    ok(st_original.wHour == st_new.wHour ||
       broken(0 == st_new.wHour), /* comctl32 <= 4.70 */
       "Expected %d, got %d\n", st_original.wHour, st_new.wHour);
    ok(st_original.wMinute == st_new.wMinute ||
       broken(0 == st_new.wMinute), /* comctl32 <= 4.70 */
       "Expected %d, got %d\n", st_original.wMinute, st_new.wMinute);
    ok(st_original.wSecond == st_new.wSecond ||
       broken(0 == st_new.wSecond), /* comctl32 <= 4.70 */
       "Expected %d, got %d\n", st_original.wSecond, st_new.wSecond);

    /* lparam cannot be NULL */
    res = SendMessageA(hwnd, MCM_GETCURSEL, 0, 0);
    expect(0, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_curr_date_seq, "monthcal currDate", TRUE);

    /* December, 31, 9999 is the maximum allowed date */
    memset(&st_new, 0, sizeof(st_new));
    st_new.wYear = 9999;
    st_new.wMonth = 12;
    st_new.wDay = 31;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);
    memset(&st_test, 0, sizeof(st_test));
    res = SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_test);
    expect(1, res);
    expect(st_new.wYear, st_test.wYear);
    expect(st_new.wMonth, st_test.wMonth);
    expect(st_new.wDay, st_test.wDay);
    expect(st_new.wHour, st_test.wHour);
    expect(st_new.wMinute, st_test.wMinute);
    expect(st_new.wSecond, st_test.wSecond);
    /* try one day later */
    st_original = st_new;
    st_new.wYear = 10000;
    st_new.wMonth = 1;
    st_new.wDay = 1;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_new);
    ok(0 == res ||
       broken(1 == res), /* comctl32 <= 4.72 */
       "Expected 0, got %d\n", res);
    if (0 == res)
    {
        memset(&st_test, 0, sizeof(st_test));
        res = SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_test);
        expect(1, res);
        expect(st_original.wYear, st_test.wYear);
        expect(st_original.wMonth, st_test.wMonth);
        expect(st_original.wDay, st_test.wDay);
        expect(st_original.wHour, st_test.wHour);
        expect(st_original.wMinute, st_test.wMinute);
        expect(st_original.wSecond, st_test.wSecond);
    }

    /* setting selection equal to current reports success even if out range */
    memset(&st_new, 0, sizeof(st_new));
    st_new.wYear = 2009;
    st_new.wDay  = 5;
    st_new.wMonth = 10;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);
    memset(&st_test, 0, sizeof(st_test));
    st_test.wYear = 2009;
    st_test.wDay  = 6;
    st_test.wMonth = 10;
    res = SendMessageA(hwnd, MCM_SETRANGE, GDTR_MIN, (LPARAM)&st_test);
    expect(1, res);
    /* set to current again */
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);

    /* set with invalid day of week */
    memset(&st_test, 0, sizeof(st_test));
    st_test.wYear = 2009;
    st_test.wDay  = 7;
    st_test.wMonth = 10;
    st_test.wDayOfWeek = 100;
    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st_test);
    expect(1, res);

    memset(&st_test, 0, sizeof(st_test));
    res = SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_test);
    expect(1, res);
    expect(2009, st_test.wYear);
    expect(7, st_test.wDay);
    expect(10, st_test.wMonth);
    expect(3, st_test.wDayOfWeek);

    DestroyWindow(hwnd);
}

static void test_firstDay(void)
{
    int res, fday, i, prev;
    CHAR b[128], caltype[3];
    LCID lcid = LOCALE_USER_DEFAULT;
    HWND hwnd;
    LRESULT ret;

    SetLastError(0xdeadbeef);
    ret = GetLocaleInfoA(lcid, LOCALE_ICALENDARTYPE, caltype, 3);
    if (ret == 0) {
        skip("Must know local calendar type (%lx)\n", GetLastError());
        return;
    } else if (atoi(caltype) != CAL_GREGORIAN) {
        skip("MonthCalendar Control only supports Gregorian calendar (type: %s)\n", caltype);
        return;
    }

    hwnd = create_monthcal_control(0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for first day of week */
    /* check for locale first day */
    if(GetLocaleInfoA(lcid, LOCALE_IFIRSTDAYOFWEEK, b, 128)){
        fday = atoi(b);
        res = SendMessageA(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
        expect(fday, res);
        prev = fday;

        /* checking for the values that actually will be stored as */
        /* current first day when we set a new value */
        for (i = -5; i < 12; i++){
            res = SendMessageA(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, i);
            expect(prev, res);
            res = SendMessageA(hwnd, MCM_GETFIRSTDAYOFWEEK, 0, 0);
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

    DestroyWindow(hwnd);
}

static void test_unicode(void)
{
    int res, temp;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for Unicode format */

    /* getting the current settings */
    temp = SendMessageA(hwnd, MCM_GETUNICODEFORMAT, 0, 0);

    /* setting to 1, should return previous settings */
    res = SendMessageA(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(temp, res);

    /* current setting is 1, so, should return 1 */
    res = SendMessageA(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    ok(1 == res ||
       broken(0 == res), /* comctl32 <= 4.70 */
       "Expected 1, got %d\n", res);

    /* setting to 0, should return previous settings */
    res = SendMessageA(hwnd, MCM_SETUNICODEFORMAT, 0, 0);
    ok(1 == res ||
       broken(0 == res), /* comctl32 <= 4.70 */
       "Expected 1, got %d\n", res);

    /* current setting is 0, so, it should return 0 */
    res = SendMessageA(hwnd, MCM_GETUNICODEFORMAT, 0, 0);
    expect(0, res);

    /* should return previous settings */
    res = SendMessageA(hwnd, MCM_SETUNICODEFORMAT, 1, 0);
    expect(0, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_unicode_seq, "monthcal unicode", FALSE);

    DestroyWindow(hwnd);
}

static void test_hittest(void)
{
    typedef struct hittest_test
    {
	UINT ht;
        BOOL  todo;
    } hittest_test_t;

    static const hittest_test_t title_hits[] = {
        /* Start is the same everywhere */
        { MCHT_TITLE,        FALSE },
        { MCHT_TITLEBTNPREV, FALSE },
        /* The middle piece is only tested for presence of items */
        /* End is the same everywhere */
        { MCHT_TITLEBTNNEXT, FALSE },
        { MCHT_TITLE,        FALSE },
        { MCHT_NOWHERE,      TRUE }
    };

    MCHITTESTINFO mchit;
    UINT res, old_res;
    SYSTEMTIME st;
    LONG x;
    UINT title_index;
    HWND hwnd;
    RECT r;
    char yearmonth[80], *locale_month, *locale_year;
    int month_count, year_count;
    BOOL in_the_middle;

    memset(&mchit, 0, sizeof(MCHITTESTINFO));

    hwnd = create_monthcal_control(0);

    /* test with invalid structure size */
    mchit.cbSize = MCHITTESTINFO_V1_SIZE - 1;
    mchit.pt.x = 0;
    mchit.pt.y = 0;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(0, mchit.pt.x);
    expect(0, mchit.pt.y);
    expect(~0u, res);
    expect(0, mchit.uHit);
    /* test with invalid pointer */
    res = SendMessageA(hwnd, MCM_HITTEST, 0, 0);
    expect(~0u, res);

    /* resize control to display single Calendar */
    res = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r);
    if (res == 0)
    {
        win_skip("Message MCM_GETMINREQRECT unsupported. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }
    MoveWindow(hwnd, 0, 0, r.right, r.bottom, FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    st.wYear = 2007;
    st.wMonth = 4;
    st.wDay = 11;
    st.wHour = 1;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    st.wDayOfWeek = 0;

    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st);
    expect(1,res);

    /* (0, 0) is the top left of the control - title */
    mchit.cbSize = MCHITTESTINFO_V1_SIZE;
    mchit.pt.x = 0;
    mchit.pt.y = 0;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(0, mchit.pt.x);
    expect(0, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_TITLE, res);

    /* bottom right of the control and should not be active */
    mchit.pt.x = r.right;
    mchit.pt.y = r.bottom;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right,  mchit.pt.x);
    expect(r.bottom, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine expect_hex(MCHT_NOWHERE, res);

    /* completely out of the control, should not be active */
    mchit.pt.x = 2 * r.right;
    mchit.pt.y = 2 * r.bottom;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(2 * r.right, mchit.pt.x);
    expect(2 * r.bottom, mchit.pt.y);
    expect(mchit.uHit, res);
    todo_wine expect_hex(MCHT_NOWHERE, res);

    /* in active area - day of the week */
    mchit.pt.x = r.right / 2;
    mchit.pt.y = r.bottom / 2;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 2, mchit.pt.x);
    expect(r.bottom / 2, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_CALENDARDATE, res);

    /* in active area - day of the week #2 */
    mchit.pt.x = r.right / 14; /* half of first day rect */
    mchit.pt.y = r.bottom / 2;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 14, mchit.pt.x);
    expect(r.bottom / 2, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_CALENDARDATE, res);

    /* in active area - date from prev month */
    mchit.pt.x = r.right / 14; /* half of first day rect */
    mchit.pt.y = 6 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 14, mchit.pt.x);
    expect(6 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_CALENDARDATEPREV, res);

if (0)
{
    /* (125, 115) is in active area - date from this month */
    mchit.pt.x = 125;
    mchit.pt.y = 115;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(125, mchit.pt.x);
    expect(115, mchit.pt.y);
    expect(mchit.uHit, res);
    expect(MCHT_CALENDARDATE, res);
}

    /* in active area - date from next month */
    mchit.pt.x = 11 * r.right / 14;
    mchit.pt.y = 16 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(11 * r.right / 14, mchit.pt.x);
    expect(16 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_CALENDARDATENEXT, res);

    /* in active area - today link */
    mchit.pt.x = r.right / 14;
    mchit.pt.y = 18 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 14, mchit.pt.x);
    expect(18 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_TODAYLINK, res);

    /* in active area - today link */
    mchit.pt.x = r.right / 2;
    mchit.pt.y = 18 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 2, mchit.pt.x);
    expect(18 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_TODAYLINK, res);

    /* in active area - today link */
    mchit.pt.x = r.right / 10;
    mchit.pt.y = 18 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 10, mchit.pt.x);
    expect(18 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect_hex(MCHT_TODAYLINK, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_hit_test_seq, "monthcal hit test", TRUE);

    /* The horizontal position of title bar elements depends on locale (y pos
       is constant), so we sample across a horizontal line and make sure we
       find all elements. */

    /* Get the format of the title */
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SYEARMONTH, yearmonth, 80);
    /* Find out if we have a month and/or year */
    locale_year = strstr(yearmonth, "y");
    locale_month = strstr(yearmonth, "M");

    mchit.pt.x = 0;
    mchit.pt.y = (5/2) * r.bottom / 19;
    title_index = 0;
    old_res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect_hex(title_hits[title_index].ht, old_res);

    in_the_middle = FALSE;
    month_count = year_count = 0;
    for (x = 0; x < r.right; x++){
        mchit.pt.x = x;
        res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
        expect(x, mchit.pt.x);
        expect((5/2) * r.bottom / 19, mchit.pt.y);
        expect(mchit.uHit, res);
        if (res != old_res) {

            if (old_res == MCHT_TITLEBTNPREV)
                in_the_middle = TRUE;

            if (res == MCHT_TITLEBTNNEXT)
                in_the_middle = FALSE;

            if (in_the_middle) {
                if (res == MCHT_TITLEMONTH)
                    month_count++;
                else if (res == MCHT_TITLEYEAR)
                    year_count++;
            } else {
                title_index++;

                if (ARRAY_SIZE(title_hits) <= title_index)
                    break;

                todo_wine_if(title_hits[title_index].todo)
                    ok(title_hits[title_index].ht == res, "Expected %x, got %x, pos %ld\n",
                                                          title_hits[title_index].ht, res, x);
            }
            old_res = res;
        }
    }

    /* There are some limits, even if LOCALE_SYEARMONTH contains rubbish
     * or no month/year indicators at all */
    if (locale_month)
        todo_wine ok(month_count == 1, "Expected 1 month item, got %d\n", month_count);
    else
        ok(month_count <= 1, "Too many month items: %d\n", month_count);

    if (locale_year)
        todo_wine ok(year_count == 1, "Expected 1 year item, got %d\n", year_count);
    else
        ok(year_count <= 1, "Too many year items: %d\n", year_count);

    todo_wine ok(month_count + year_count >= 1, "Not enough month and year items\n");

    ok(r.right <= x && title_index + 1 == ARRAY_SIZE(title_hits), "Wrong title layout\n");

    DestroyWindow(hwnd);
}

static void test_todaylink(void)
{
    MCHITTESTINFO mchit;
    SYSTEMTIME st_test, st_new;
    UINT res;
    HWND hwnd;
    RECT r;

    memset(&mchit, 0, sizeof(MCHITTESTINFO));

    hwnd = create_monthcal_control(0);

    res = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r);
    expect(1, res);
    MoveWindow(hwnd, 0, 0, r.right, r.bottom, FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* hit active area - today link */
    mchit.cbSize = MCHITTESTINFO_V1_SIZE;
    mchit.pt.x = r.right / 14;
    mchit.pt.y = 18 * r.bottom / 19;
    res = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect(r.right / 14, mchit.pt.x);
    expect(18 * r.bottom / 19, mchit.pt.y);
    expect(mchit.uHit, res);
    expect(MCHT_TODAYLINK, res);

    st_test.wDay = 1;
    st_test.wMonth = 1;
    st_test.wYear = 2005;

    res = SendMessageA(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);
    expect(0, res);

    memset(&st_new, 0, sizeof(st_new));
    res = SendMessageA(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);
    expect(1, st_new.wDay);
    expect(1, st_new.wMonth);
    expect(2005, st_new.wYear);

    res = SendMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(mchit.pt.x, mchit.pt.y));
    expect(0, res);

    memset(&st_new, 0, sizeof(st_new));
    res = SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st_new);
    expect(1, res);
    expect(1, st_new.wDay);
    expect(1, st_new.wMonth);
    expect(2005, st_new.wYear);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_todaylink_seq, "monthcal hit test", TRUE);

    DestroyWindow(hwnd);
}

static void test_today(void)
{
    SYSTEMTIME st_test, st_new;
    int res;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for "today" information */

    /* check for overflow, should be ok */
    memset(&st_test, 0, sizeof(st_test));
    st_test.wDay = 38;
    st_test.wMonth = 38;

    st_new.wDay = 27;
    st_new.wMonth = 27;

    res = SendMessageA(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);
    expect(0, res);

    res = SendMessageA(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
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

    res = SendMessageA(hwnd, MCM_SETTODAY, 0, (LPARAM)&st_test);
    expect(0, res);

    res = SendMessageA(hwnd, MCM_GETTODAY, 0, (LPARAM)&st_new);
    expect(1, res);

    /* st_test should not change */
    expect(0, st_test.wDay);
    expect(0, st_test.wMonth);

    /* st_new should change to zero*/
    expect(0, st_new.wDay);
    expect(0, st_new.wMonth);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_today_seq, "monthcal today", TRUE);

    DestroyWindow(hwnd);
}

static void test_scroll(void)
{
    int res;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

    res = SendMessageA(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(2, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for scroll rate */
    res = SendMessageA(hwnd, MCM_SETMONTHDELTA, 2, 0);
    expect(0, res);

    res = SendMessageA(hwnd, MCM_SETMONTHDELTA, 3, 0);
    expect(2, res);
    res = SendMessageA(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(3, res);

    res = SendMessageA(hwnd, MCM_SETMONTHDELTA, 12, 0);
    expect(3, res);
    res = SendMessageA(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(12, res);

    res = SendMessageA(hwnd, MCM_SETMONTHDELTA, 15, 0);
    expect(12, res);
    res = SendMessageA(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(15, res);

    res = SendMessageA(hwnd, MCM_SETMONTHDELTA, -5, 0);
    expect(15, res);
    res = SendMessageA(hwnd, MCM_GETMONTHDELTA, 0, 0);
    expect(-5, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_scroll_seq, "monthcal scroll", FALSE);

    DestroyWindow(hwnd);
}

static void test_monthrange(void)
{
    int res;
    SYSTEMTIME st_visible[2], st_daystate[2], st;
    HWND hwnd;
    RECT r;

    hwnd = create_monthcal_control(0);

    memset(&st_visible, 0, sizeof(st_visible));
    memset(&st_daystate, 0, sizeof(st_daystate));

    st.wYear = 2000;
    st.wMonth = 11;
    st.wDay = 28;
    st.wHour = 11;
    st.wMinute = 59;
    st.wSecond = 30;
    st.wMilliseconds = 0;
    st.wDayOfWeek = 0;

    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st);
    expect(1,res);

    /* to be locale independent */
    SendMessageA(hwnd, MCM_SETFIRSTDAYOFWEEK, 0, (LPARAM)6);

    res = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r);
    expect(TRUE, res);
    /* resize control to display two Calendars */
    MoveWindow(hwnd, 0, 0, r.right, (5/2)*r.bottom, FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st_visible);
    expect(2, res);
    expect(2000, st_visible[0].wYear);
    expect(11, st_visible[0].wMonth);
    expect(1, st_visible[0].wDay);
    expect(2000, st_visible[1].wYear);
    expect(12, st_visible[1].wMonth);
    expect(31, st_visible[1].wDay);

    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE, (LPARAM)st_daystate);
    expect(4, res);
    expect(2000, st_daystate[0].wYear);
    expect(10, st_daystate[0].wMonth);
    expect(29, st_daystate[0].wDay);
    expect(2001, st_daystate[1].wYear);
    expect(1, st_daystate[1].wMonth);
    expect(6, st_daystate[1].wDay);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_monthrange_seq, "monthcal monthrange", FALSE);

    /* with null date array parameter */
    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, 0);
    expect(2, res);

    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE, 0);
    expect(4, res);

    /* resize control to display single Calendar */
    MoveWindow(hwnd, 0, 0, r.right, r.bottom, FALSE);

    memset(&st, 0, sizeof(st));
    st.wMonth = 9;
    st.wYear  = 1752;
    st.wDay   = 14;

    res = SendMessageA(hwnd, MCM_SETCURSEL, 0, (LPARAM)&st);
    expect(1, res);

    /* September 1752 has 19 days */
    res = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_VISIBLE, (LPARAM)st_visible);
    expect(1, res);

    expect(1752, st_visible[0].wYear);
    expect(9, st_visible[0].wMonth);
    ok(14 == st_visible[0].wDay ||
       broken(1 == st_visible[0].wDay), /* comctl32 <= 4.72 */
       "Expected 14, got %d\n", st_visible[0].wDay);

    expect(1752, st_visible[1].wYear);
    expect(9, st_visible[1].wMonth);
    expect(19, st_visible[1].wDay);

    DestroyWindow(hwnd);
}

static void test_maxselday(void)
{
    int res;
    HWND hwnd;
    DWORD style;

    hwnd = create_monthcal_control(0);
    /* if no style specified default to 1 */
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(1, res);
    res = SendMessageA(hwnd, MCM_SETMAXSELCOUNT, 5, 0);
    expect(0, res);
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(1, res);

    /* try to set style */
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style | MCS_MULTISELECT);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & MCS_MULTISELECT), "Expected MCS_MULTISELECT not to be set\n");
    DestroyWindow(hwnd);

    hwnd = create_monthcal_control(MCS_MULTISELECT);
    /* try to remove style */
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style & ~MCS_MULTISELECT);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & MCS_MULTISELECT, "Expected MCS_MULTISELECT to be set\n");
    DestroyWindow(hwnd);

    hwnd = create_monthcal_control(MCS_MULTISELECT);

    /* default width is a week */
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(7, res);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Setter and Getters for max selected days */
    res = SendMessageA(hwnd, MCM_SETMAXSELCOUNT, 5, 0);
    expect(1, res);
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(5, res);

    res = SendMessageA(hwnd, MCM_SETMAXSELCOUNT, 15, 0);
    expect(1, res);
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(15, res);

    /* test invalid value */
    res = SendMessageA(hwnd, MCM_SETMAXSELCOUNT, -1, 0);
    expect(0, res);
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(15, res);

    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, monthcal_max_sel_day_seq, "monthcal MaxSelDay", FALSE);

    /* zero value is invalid too */
    res = SendMessageA(hwnd, MCM_SETMAXSELCOUNT, 0, 0);
    expect(0, res);
    res = SendMessageA(hwnd, MCM_GETMAXSELCOUNT, 0, 0);
    expect(15, res);

    DestroyWindow(hwnd);
}

static void test_size(void)
{
    int res;
    RECT r1, r2;
    HFONT hFont1, hFont2;
    LOGFONTA logfont;
    HWND hwnd;

    hwnd = create_monthcal_control(0);

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    hFont1 = CreateFontIndirectA(&logfont);

    logfont.lfHeight = 24;
    hFont2 = CreateFontIndirectA(&logfont);

    /* initialize to a font we can compare against */
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hFont1, 0);
    res = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r1);
    ok(res, "SendMessageA(MCM_GETMINREQRECT) failed\n");

    /* check that setting a larger font results in an larger rect */
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hFont2, 0);
    res = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r2);
    ok(res, "SendMessageA(MCM_GETMINREQRECT) failed\n");

    OffsetRect(&r1, -r1.left, -r1.top);
    OffsetRect(&r2, -r2.left, -r2.top);

    ok(r1.bottom < r2.bottom, "Failed to get larger rect with larger font\n");

    DestroyWindow(hwnd);
}

static void test_create(void)
{
    HWND hwnd;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hwnd = create_monthcal_control(0);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_monthcal_control_seq, "create monthcal control", TRUE);

    DestroyWindow(hwnd);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    hwnd = create_monthcal_control(MCS_MULTISELECT);
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_monthcal_multi_sel_style_seq, "create monthcal (multi sel style)", TRUE);
    DestroyWindow(hwnd);
}

static void test_destroy(void)
{
    HWND hwnd;

    hwnd = create_monthcal_control(0);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, PARENT_SEQ_INDEX, destroy_monthcal_parent_msgs_seq, "Destroy monthcal (parent msg)", FALSE);
    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, destroy_monthcal_child_msgs_seq, "Destroy monthcal (child msg)", FALSE);

    /* MCS_MULTISELECT */
    hwnd = create_monthcal_control(MCS_MULTISELECT);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);
    DestroyWindow(hwnd);
    ok_sequence(sequences, MONTHCAL_SEQ_INDEX, destroy_monthcal_multi_sel_style_seq, "Destroy monthcal (multi sel style)", FALSE);
}

static void test_selrange(void)
{
    HWND hwnd;
    SYSTEMTIME st, range[2], range2[2];
    BOOL ret, old_comctl32 = FALSE;

    hwnd = create_monthcal_control(MCS_MULTISELECT);

    /* just after creation selection should start and end today */
    ret = SendMessageA(hwnd, MCM_GETTODAY, 0, (LPARAM)&st);
    expect(TRUE, ret);

    memset(range, 0xcc, sizeof(range));
    ret = SendMessageA(hwnd, MCM_GETSELRANGE, 0, (LPARAM)range);
    expect(TRUE, ret);
    expect(st.wYear,      range[0].wYear);
    expect(st.wMonth,     range[0].wMonth);
    expect(st.wDay,       range[0].wDay);
    if (range[0].wDayOfWeek != st.wDayOfWeek)
    {
        win_skip("comctl32 <= 4.70 doesn't set some values\n");
        old_comctl32 = TRUE;
    }
    else
    {
        expect(st.wDayOfWeek, range[0].wDayOfWeek);
        expect(st.wHour,      range[0].wHour);
        expect(st.wMinute,    range[0].wMinute);
        expect(st.wSecond,    range[0].wSecond);
        expect(st.wMilliseconds, range[0].wMilliseconds);
    }

    expect(st.wYear,      range[1].wYear);
    expect(st.wMonth,     range[1].wMonth);
    expect(st.wDay,       range[1].wDay);
    if (!old_comctl32)
    {
        expect(st.wDayOfWeek, range[1].wDayOfWeek);
        expect(st.wHour,      range[1].wHour);
        expect(st.wMinute,    range[1].wMinute);
        expect(st.wSecond,    range[1].wSecond);
        expect(st.wMilliseconds, range[1].wMilliseconds);
    }

    /* bounds are swapped if min > max */
    memset(&range[0], 0, sizeof(range[0]));
    range[0].wYear  = 2009;
    range[0].wMonth = 10;
    range[0].wDay   = 5;
    range[1] = range[0];
    range[1].wDay   = 3;

    ret = SendMessageA(hwnd, MCM_SETSELRANGE, 0, (LPARAM)range);
    expect(TRUE, ret);

    ret = SendMessageA(hwnd, MCM_GETSELRANGE, 0, (LPARAM)range2);
    expect(TRUE, ret);

    expect(range[1].wYear,      range2[0].wYear);
    expect(range[1].wMonth,     range2[0].wMonth);
    expect(range[1].wDay,       range2[0].wDay);
    expect(6, range2[0].wDayOfWeek);
    expect(range[1].wHour,      range2[0].wHour);
    expect(range[1].wMinute,    range2[0].wMinute);
    expect(range[1].wSecond,    range2[0].wSecond);
    expect(range[1].wMilliseconds, range2[0].wMilliseconds);

    expect(range[0].wYear,      range2[1].wYear);
    expect(range[0].wMonth,     range2[1].wMonth);
    expect(range[0].wDay,       range2[1].wDay);
    expect(1, range2[1].wDayOfWeek);
    expect(range[0].wHour,      range2[1].wHour);
    expect(range[0].wMinute,    range2[1].wMinute);
    expect(range[0].wSecond,    range2[1].wSecond);
    expect(range[0].wMilliseconds, range2[1].wMilliseconds);

    /* try with range larger than maximum configured */
    memset(&range[0], 0, sizeof(range[0]));
    range[0].wYear  = 2009;
    range[0].wMonth = 10;
    range[0].wDay   = 1;
    range[1] = range[0];

    ret = SendMessageA(hwnd, MCM_SETSELRANGE, 0, (LPARAM)range);
    expect(TRUE, ret);

    range[1] = range[0];
    /* default max. range is 7 days */
    range[1].wDay = 8;

    ret = SendMessageA(hwnd, MCM_SETSELRANGE, 0, (LPARAM)range);
    expect(FALSE, ret);

    ret = SendMessageA(hwnd, MCM_GETSELRANGE, 0, (LPARAM)range2);
    expect(TRUE, ret);

    expect(range[0].wYear,  range2[0].wYear);
    expect(range[0].wMonth, range2[0].wMonth);
    expect(range[0].wDay,   range2[0].wDay);
    expect(range[0].wYear,  range2[1].wYear);
    expect(range[0].wMonth, range2[1].wMonth);
    expect(range[0].wDay,   range2[1].wDay);

    DestroyWindow(hwnd);
}

static void test_killfocus(void)
{
    HWND hwnd;
    DWORD style;

    hwnd = create_monthcal_control(0);

    /* make parent invisible */
    style = GetWindowLongA(parent_wnd, GWL_STYLE);
    SetWindowLongA(parent_wnd, GWL_STYLE, style & ~WS_VISIBLE);

    SendMessageA(hwnd, WM_KILLFOCUS, (WPARAM)GetDesktopWindow(), 0);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & WS_VISIBLE, "Expected WS_VISIBLE to be set\n");

    style = GetWindowLongA(parent_wnd, GWL_STYLE);
    SetWindowLongA(parent_wnd, GWL_STYLE, style | WS_VISIBLE);

    DestroyWindow(hwnd);
}

static void test_hittest_v6(void)
{
    MCHITTESTINFO mchit;
    DWORD ret;
    HWND hwnd;
    RECT r;

    hwnd = create_monthcal_control(0);
    SendMessageA(hwnd, MCM_SETCALENDARBORDER, TRUE, 0);

    SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r);
    /* reserving some area around calendar */
    MoveWindow(hwnd, 0, 0, r.right * 3 / 2, r.bottom * 3 / 2, FALSE);
    mchit.cbSize = sizeof(MCHITTESTINFO);
    mchit.pt.x = mchit.pt.y = 0;
    mchit.iOffset = -1;
    mchit.iRow = -1;
    mchit.iCol = -1;
    ret = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    if (ret == ~0u)
    {
        win_skip("Only MCHITTESTINFO_V1 supported\n");
        DestroyWindow(hwnd);
        return;
    }
    todo_wine expect_hex(MCHT_NOWHERE, ret);
    expect(-1, mchit.iOffset);
    expect(-1, mchit.iRow);
    expect(-1, mchit.iCol);

    MoveWindow(hwnd, 0, 0, r.right, r.bottom, FALSE);
    mchit.pt.x = r.right / 2;
    mchit.pt.y = r.bottom / 2;
    mchit.iOffset = -1;
    ret = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect_hex(MCHT_CALENDARDATE, ret);
    expect(0, mchit.iOffset);

    /* over day area */
    mchit.pt.x = r.right / (7*2);
    mchit.pt.y = r.bottom / 2;
    mchit.iOffset = -1;
    mchit.iCol = mchit.iRow = -1;
    mchit.uHit = 0;
    SetRect(&mchit.rc, -1, -1, -1, -1);
    ret = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect_hex(MCHT_CALENDARDATE, ret);
    expect_hex(MCHT_CALENDARDATE, mchit.uHit);
    expect(0, mchit.iOffset);
    expect(2, mchit.iRow);
    expect(0, mchit.iCol);
    /* returned a one day rectangle */
    expect_d(r.right / 7, mchit.rc.right - mchit.rc.left);
    expect_d(r.bottom / 10, mchit.rc.bottom - mchit.rc.top);

    /* title */
    mchit.pt.x = 1;
    mchit.pt.y = 1;
    mchit.iOffset = -1;
    mchit.iCol = mchit.iRow = -1;
    mchit.uHit = 0;
    SetRect(&mchit.rc, -1, -1, -1, -1);
    ret = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    expect_hex(MCHT_TITLE, ret);
    expect_hex(MCHT_TITLE, mchit.uHit);
    expect(0, mchit.iOffset);
    expect(-1, mchit.iRow);
    expect(-1, mchit.iCol);
    expect(0, mchit.rc.left);
    expect(0, mchit.rc.top);
    expect_d(r.right, mchit.rc.right);
    ok(mchit.rc.bottom > 0, "got %ld\n", mchit.rc.bottom);

    /* between two calendars */
    MoveWindow(hwnd, 0, 0, r.right * 5/2, r.bottom, FALSE);
    mchit.pt.x = r.right / (5*4);
    mchit.pt.y = r.bottom / 2;
    mchit.iOffset = -2;
    mchit.iCol = mchit.iRow = -2;
    mchit.uHit = ~0;
    SetRect(&mchit.rc, -1, -1, -1, -1);
    ret = SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
    todo_wine expect_hex(MCHT_NOWHERE, ret);
    todo_wine expect_hex(MCHT_NOWHERE, mchit.uHit);
    expect(-2, mchit.iOffset);
    expect(-2, mchit.iRow);
    expect(-2, mchit.iCol);
    todo_wine expect(0, mchit.rc.left);
    todo_wine expect(0, mchit.rc.top);
    todo_wine expect_d(r.right * 5/2, mchit.rc.right);
    todo_wine expect_d(r.bottom, mchit.rc.bottom);

    DestroyWindow(hwnd);
}

static void test_get_set_border(void)
{
    HWND hwnd;
    DWORD ret;

    hwnd = create_monthcal_control(0);

    /* a non-default value */
    ret = SendMessageA(hwnd, MCM_SETCALENDARBORDER, TRUE, 10);
    expect(0, ret);

    ret = SendMessageA(hwnd, MCM_GETCALENDARBORDER, 0, 0);

    if (ret != 10)
    {
        skip("MCM_GET/SETCALENDARBORDER not supported\n");
        DestroyWindow(hwnd);
        return;
    }

    expect(10, ret);

    DestroyWindow(hwnd);
}

static void test_MCM_SIZERECTTOMIN(void)
{
    HWND hwnd;
    DWORD ret;
    RECT r, r2;

    hwnd = create_monthcal_control(0);

    ret = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r2);
    if (ret == 0)
    {
        win_skip("Message MCM_GETMINREQRECT unsupported. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }

    ret = SendMessageA(hwnd, MCM_SIZERECTTOMIN, 0, 0);
    ok(ret == 0, "got %ld\n", ret);

    SetRectEmpty(&r);
    ret = SendMessageA(hwnd, MCM_SIZERECTTOMIN, 0, (LPARAM)&r);
    if (ret == 0)
    {
        skip("Message MCM_SIZERECTTOMIN unsupported. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }
    ok(ret == 1, "got %ld\n", ret);
    ok(r.left == 0 && r.right > 0, "got %ld, %ld\n", r.left, r.right);

    r = r2;
    ret = SendMessageA(hwnd, MCM_SIZERECTTOMIN, 0, (LPARAM)&r);
    ok(ret == 1, "got %ld\n", ret);

    r2.right = (r2.right - r2.left) * 3;
    r2.bottom = (r2.bottom - r2.top) * 3;
    r2.left = r2.top = 0;
    ret = SendMessageA(hwnd, MCM_SIZERECTTOMIN, 0, (LPARAM)&r2);
    ok(ret == 1, "got %ld\n", ret);

    DestroyWindow(hwnd);
}

static void test_MCM_GETCALENDARCOUNT(void)
{
    HWND hwnd;
    DWORD ret;

    hwnd = create_monthcal_control(0);

    ret = SendMessageA(hwnd, MCM_GETCALENDARCOUNT, 0, 0);
    if (ret == 0)
    {
        win_skip("Message MCM_GETCALENDARCOUNT unsupported. Skipping.\n");
        DestroyWindow(hwnd);
        return;
    }

    expect(2, ret);

    DestroyWindow(hwnd);
}

static void test_daystate(void)
{
    MONTHDAYSTATE state[4];
    DWORD ret, style;
    HWND hwnd;
    RECT r;

    /* without MCS_DAYSTATE */
    hwnd = create_monthcal_control(0);

    ret = SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&r);
    expect(TRUE, ret);

    /* resize control to display two Calendars */
    MoveWindow(hwnd, 0, 0, r.right, (5/2)*r.bottom, FALSE);

    ret = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE, 0);
    expect(4, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 4, (LPARAM)&state);
    expect(0, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 2, (LPARAM)&state);
    expect(0, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 0, 0);
    expect(0, ret);

    /* try to switch on */
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) | MCS_DAYSTATE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & MCS_DAYSTATE) == 0, "got 0x%08lx\n", style);

    DestroyWindow(hwnd);

    /* with MCS_DAYSTATE */
    hwnd = create_monthcal_control(MCS_DAYSTATE);

    ret = SendMessageA(hwnd, MCM_GETMONTHRANGE, GMR_DAYSTATE, 0);
    expect(4, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 4, (LPARAM)&state);
    expect(1, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 2, (LPARAM)&state);
    expect(0, ret);

    ret = SendMessageA(hwnd, MCM_SETDAYSTATE, 0, 0);
    expect(0, ret);

    /* try to switch off */
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) & ~MCS_DAYSTATE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok((style & MCS_DAYSTATE) == MCS_DAYSTATE, "got 0x%08lx\n", style);

    DestroyWindow(hwnd);
}

static void test_sel_notify(void)
{
    typedef struct
    {
        DWORD       val;
        const char* name;
    } Monthcal_style;

    HWND hwnd;
    RECT rc;
    MCHITTESTINFO mchit = {MCHITTESTINFO_V1_SIZE};
    SYSTEMTIME st;
    Monthcal_style styles[] = {
        {MCS_NOTODAY,                    "MCS_NOTODAY"},
        {MCS_NOTODAY | MCS_MULTISELECT,  "MCS_NOTODAY | MCS_MULTISELECT"},
        {MCS_DAYSTATE,                   "MCS_DAYSTATE"},
        {MCS_DAYSTATE | MCS_MULTISELECT, "MCS_DAYSTATE | MCS_MULTISELECT"}
    };
    int i;

    for(i = 0; i < ARRAY_SIZE(styles); i++)
    {
        hwnd = create_monthcal_control(styles[i].val);
        SetWindowLongPtrA(hwnd, GWLP_ID, SEL_NOTIFY_TEST_ID);
        SendMessageA(hwnd, MCM_GETMINREQRECT, 0, (LPARAM)&rc);
        MoveWindow(hwnd, 0, 0, rc.right, rc.bottom, FALSE);
        /* Simulate mouse click on some unselected day to generate
            MCN_SELECT and MCN_SELCHANGE notifications */
        mchit.pt.x = mchit.pt.y = 0;
        SendMessageA(hwnd, MCM_GETCURSEL, 0, (LPARAM)&st);

        do
        {
            /* we assume box for day is larger than 3x3 */
            if ((mchit.pt.y += 3) >= rc.bottom)
            {
                mchit.pt.y = 0;
                if ((mchit.pt.x += 3) >= rc.right) break;
            }
            SendMessageA(hwnd, MCM_HITTEST, 0, (LPARAM)&mchit);
        } while (mchit.uHit != MCHT_CALENDARDATE || st.wDay == mchit.st.wDay); /* Ensure that mchit.pt points to unselected day */
        got_MCN_SELECT = got_MCN_SELCHANGE = FALSE;
        SendMessageA(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(mchit.pt.x, mchit.pt.y));
        if (styles[i].val & MCS_MULTISELECT)
            ok(got_MCN_SELCHANGE, "%d: MCN_SELCHANGE should be sent\n", i);
        else
            ok(!got_MCN_SELCHANGE, "%d: MCN_SELCHANGE should not be sent\n", i);
        ok(!got_MCN_SELECT, "%d: MCN_SELECT should not be sent\n", i);
        got_MCN_SELECT = got_MCN_SELCHANGE = FALSE;
        SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(mchit.pt.x, mchit.pt.y));
        if (styles[i].val & MCS_MULTISELECT)
            ok(!got_MCN_SELCHANGE, "%d: MCN_SELCHANGE should not be sent\n", i);
        else
            ok(got_MCN_SELCHANGE, "%d: MCN_SELCHANGE should be sent\n", i);
        ok(got_MCN_SELECT, "%d: MCN_SELECT should be sent\n", i);
        DestroyWindow(hwnd);
    }
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(InitCommonControlsEx);
#undef X
}

START_TEST(monthcal)
{
    INITCOMMONCONTROLSEX iccex;
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_functions();

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_DATE_CLASSES;
    pInitCommonControlsEx(&iccex);

    test_monthcal();

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = create_parent_window();

    test_create();
    test_destroy();
    test_color();
    test_currdate();
    test_firstDay();
    test_unicode();
    test_today();
    test_scroll();
    test_monthrange();
    test_hittest();
    test_todaylink();
    test_size();
    test_maxselday();
    test_selrange();
    test_killfocus();
    test_daystate();
    test_sel_notify();

    if (!load_v6_module(&ctx_cookie, &hCtx))
    {
        DestroyWindow(parent_wnd);
        return;
    }

    test_hittest_v6();
    test_get_set_border();
    test_MCM_SIZERECTTOMIN();
    test_MCM_GETCALENDARCOUNT();

    unload_v6_module(ctx_cookie, hCtx);

    DestroyWindow(parent_wnd);
}
