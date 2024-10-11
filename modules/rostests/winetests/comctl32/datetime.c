/* Unit test suite for datetime control.
*
* Copyright 2007 Kanit Therdsteerasukdi
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

#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "msg.h"
#include "v6util.h"

#define expect(EXPECTED, GOT) ok((GOT)==(EXPECTED), "Expected %d, got %Id\n", (EXPECTED), (GOT))

#define expect_unsuccess(EXPECTED, GOT) ok((GOT)==(EXPECTED), "Expected %d(unsuccessful), got %Id(successful)\n", (EXPECTED), (GOT))

#define NUM_MSG_SEQUENCES   1
#define DATETIME_SEQ_INDEX    0

static BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message test_dtm_set_format_seq[] = {
    { DTM_SETFORMATA, sent|wparam|lparam, 0, 0 },
    { DTM_SETFORMATA, sent|wparam, 0 },
    { 0 }
};

static const struct message test_dtm_set_and_get_mccolor_seq[] = {
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_BACKGROUND, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_MONTHBK, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_MONTHBK, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_MONTHBK, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_MONTHBK, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TEXT, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TEXT, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TEXT, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_TEXT, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLEBK, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLEBK, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLEBK, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_TITLEBK, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_TITLETEXT, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, 0 },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, RGB(255, 255, 255) },
    { DTM_SETMCCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, RGB(100, 180, 220) },
    { DTM_GETMCCOLOR, sent|wparam|lparam, MCSC_TRAILINGTEXT, 0 },
    { 0 }
};

static const struct message test_dtm_set_and_get_mcfont_seq[] = {
    { DTM_SETMCFONT, sent|lparam, 0, 1 },
    { DTM_GETMCFONT, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_dtm_get_monthcal_seq[] = {
    { DTM_GETMONTHCAL, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message test_dtm_set_and_get_range_seq[] = {
    { DTM_SETRANGE, sent|wparam, GDTR_MIN },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MAX },
    { DTM_SETRANGE, sent|wparam, GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { 0 }
};

static const struct message test_dtm_set_range_swap_min_max_seq[] = {
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { 0 }
};

static const struct message test_dtm_set_and_get_system_time_seq[] = {
    { DTM_SETSYSTEMTIME, sent|wparam, GDT_NONE },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { 0 }
};

static const struct message test_dtm_set_and_get_systime_with_limits[] = {
    { DTM_SETRANGE, sent|wparam, GDTR_MIN | GDTR_MAX },
    { DTM_GETRANGE, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { DTM_SETSYSTEMTIME, sent|wparam, 0 },
    { DTM_GETSYSTEMTIME, sent|wparam, 0 },
    { 0 }
};

static LRESULT WINAPI datetime_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    add_message(sequences, DATETIME_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_datetime_control(DWORD style)
{
    WNDPROC oldproc;
    HWND hWndDateTime = NULL;

    hWndDateTime = CreateWindowExA(0,
        DATETIMEPICK_CLASSA,
        NULL,
        style,
        0,50,300,120,
        NULL,
        NULL,
        NULL,
        NULL);

    if (!hWndDateTime) return NULL;

    oldproc = (WNDPROC)SetWindowLongPtrA(hWndDateTime, GWLP_WNDPROC,
                                         (LONG_PTR)datetime_subclass_proc);
    SetWindowLongPtrA(hWndDateTime, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hWndDateTime;
}

static void test_dtm_set_format(void)
{
    HWND hWnd;
    CHAR txt[256];
    SYSTEMTIME systime;
    LRESULT r;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hWnd, DTM_SETFORMATA, 0, 0);
    expect(1, r);

    r = SendMessageA(hWnd, DTM_SETFORMATA, 0,
		    (LPARAM)"'Today is: 'hh':'m':'s dddd MMM dd', 'yyyy");
    expect(1, r);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_format_seq, "test_dtm_set_format", FALSE);

    r = SendMessageA(hWnd, DTM_SETFORMATA, 0, (LPARAM)"'hh' hh");
    expect(1, r);
    ZeroMemory(&systime, sizeof(systime));
    systime.wYear = 2000;
    systime.wMonth = systime.wDay = 1;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, 0, (LPARAM)&systime);
    expect(1, r);
    GetWindowTextA(hWnd, txt, 256);
    ok(strcmp(txt, "hh 12") == 0, "String mismatch (\"%s\" vs \"hh 12\")\n", txt);

    DestroyWindow(hWnd);
}

static void test_mccolor_types(HWND hWndDateTime, int mccolor_type, const char* mccolor_name)
{
    COLORREF theColor, prevColor, crColor;

    theColor=RGB(0,0,0);
    crColor = SendMessageA(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(crColor != ~0u, "%s: Set RGB(0,0,0): Expected COLORREF of previous value, got %ld\n", mccolor_name, crColor);
    prevColor=theColor;
    theColor=RGB(255,255,255);
    crColor = SendMessageA(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(crColor==prevColor, "%s: Set RGB(255,255,255): Expected COLORREF of previous value, got %ld\n", mccolor_name, crColor);
    prevColor=theColor;
    theColor=RGB(100,180,220);
    crColor = SendMessageA(hWndDateTime, DTM_SETMCCOLOR, mccolor_type, theColor);
    ok(crColor==prevColor, "%s: Set RGB(100,180,220): Expected COLORREF of previous value, got %ld\n", mccolor_name, crColor);
    crColor = SendMessageA(hWndDateTime, DTM_GETMCCOLOR, mccolor_type, 0);
    ok(crColor==theColor, "%s: GETMCCOLOR: Expected %ld, got %ld\n", mccolor_name, theColor, crColor);
}

static void test_dtm_set_and_get_mccolor(void)
{
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    test_mccolor_types(hWnd, MCSC_BACKGROUND, "MCSC_BACKGROUND");
    test_mccolor_types(hWnd, MCSC_MONTHBK, "MCSC_MONTHBK");
    test_mccolor_types(hWnd, MCSC_TEXT, "MCSC_TEXT");
    test_mccolor_types(hWnd, MCSC_TITLEBK, "MCSC_TITLEBK");
    test_mccolor_types(hWnd, MCSC_TITLETEXT, "MCSC_TITLETEXT");
    test_mccolor_types(hWnd, MCSC_TRAILINGTEXT, "MCSC_TRAILINGTEXT");

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_and_get_mccolor_seq, "test_dtm_set_and_get_mccolor", FALSE);

    DestroyWindow(hWnd);
}

static void test_dtm_set_and_get_mcfont(void)
{
    HFONT hFontOrig, hFontNew;
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hFontOrig = GetStockObject(DEFAULT_GUI_FONT);
    SendMessageA(hWnd, DTM_SETMCFONT, (WPARAM)hFontOrig, TRUE);
    hFontNew = (HFONT)SendMessageA(hWnd, DTM_GETMCFONT, 0, 0);
    ok(hFontOrig == hFontNew, "Expected hFontOrig==hFontNew, hFontOrig=%p, hFontNew=%p\n", hFontOrig, hFontNew);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_and_get_mcfont_seq, "test_dtm_set_and_get_mcfont", FALSE);
    DestroyWindow(hWnd);
}

static void test_dtm_get_monthcal(void)
{
    LRESULT r;
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    todo_wine {
        r = SendMessageA(hWnd, DTM_GETMONTHCAL, 0, 0);
        ok(r == 0, "Expected NULL(no child month calendar control), got %Id\n", r);
    }

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_get_monthcal_seq, "test_dtm_get_monthcal", FALSE);
    DestroyWindow(hWnd);
}

static void fill_systime_struct(SYSTEMTIME *st, int year, int month, int dayofweek, int day, int hour, int minute, int second, int milliseconds)
{
    st->wYear = year;
    st->wMonth = month;
    st->wDayOfWeek = dayofweek;
    st->wDay = day;
    st->wHour = hour;
    st->wMinute = minute;
    st->wSecond = second;
    st->wMilliseconds = milliseconds;
}

static LPARAM compare_systime_date(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    return (st1->wYear == st2->wYear)
            && (st1->wMonth == st2->wMonth)
            && (st1->wDayOfWeek == st2->wDayOfWeek)
            && (st1->wDay == st2->wDay);
}

static LPARAM compare_systime_time(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    return (st1->wHour == st2->wHour)
            && (st1->wMinute == st2->wMinute)
            && (st1->wSecond == st2->wSecond)
            && (st1->wMilliseconds == st2->wMilliseconds);
}

static LPARAM compare_systime(SYSTEMTIME *st1, SYSTEMTIME *st2)
{
    if(!compare_systime_date(st1, st2))
        return 0;

    return compare_systime_time(st1, st2);
}

#define expect_systime(ST1, ST2) ok(compare_systime((ST1), (ST2))==1, "ST1 != ST2\n")
#define expect_systime_date(ST1, ST2) ok(compare_systime_date((ST1), (ST2))==1, "ST1.date != ST2.date\n")
#define expect_systime_time(ST1, ST2) ok(compare_systime_time((ST1), (ST2))==1, "ST1.time != ST2.time\n")

static void test_dtm_set_and_get_range(void)
{
    LRESULT r;
    SYSTEMTIME st[2];
    SYSTEMTIME getSt[2];
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* initialize st[1] to all invalid numbers */
    fill_systime_struct(&st[1], 0, 0, 7, 0, 24, 60, 60, 1000);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == GDTR_MIN, "Expected %x, not %x(GDTR_MAX) or %x(GDTR_MIN | GDTR_MAX), got %Ix\n", GDTR_MIN, GDTR_MAX, GDTR_MIN | GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MAX, (LPARAM)st);
    expect_unsuccess(0, r);

    /* set st[0] to all invalid numbers */
    fill_systime_struct(&st[0], 0, 0, 7, 0, 24, 60, 60, 1000);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == GDTR_MAX, "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MIN | GDTR_MAX), got %Ix\n", GDTR_MAX, GDTR_MIN, GDTR_MIN | GDTR_MAX, r);
    expect_systime(&st[1], &getSt[1]);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN, (LPARAM)st);
    expect_unsuccess(0, r);
    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect_unsuccess(0, r);

    /* set st[0] to highest possible value */
    fill_systime_struct(&st[0], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    /* set st[0] to value higher than minimum */
    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);
    /* set st[1] to value lower than maximum */
    fill_systime_struct(&st[1], 2007, 3, 2, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_and_get_range_seq, "test_dtm_set_and_get_range", FALSE);

    /* DTM_SETRANGE with 0 flags */
    r = SendMessageA(hWnd, DTM_SETRANGE, 0, (LPARAM)st);
    ok(r, "got %Iu\n", r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == 0, "got %Iu\n", r);
    ok(getSt[0].wYear == 0 && getSt[1].wYear == 0, "got %u, %u\n", getSt[0].wYear, getSt[1].wYear);

    DestroyWindow(hWnd);
}

/* when max<min for DTM_SETRANGE, Windows seems to swap the min and max values,
although that's undocumented.  However, it doesn't seem to be implemented
correctly, causing some strange side effects */
static void test_dtm_set_range_swap_min_max(void)
{
    LRESULT r;
    SYSTEMTIME st[2];
    SYSTEMTIME getSt[2];
    SYSTEMTIME origSt;
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    fill_systime_struct(&st[0], 2007, 2, 4, 15, 2, 2, 2, 2);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&origSt);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&st[0], &origSt);

    /* set st[0] to value higher than st[1] */
    fill_systime_struct(&st[0], 2007, 3, 2, 31, 23, 59, 59, 999);
    fill_systime_struct(&st[1], 1980, 1, 3, 23, 14, 34, 37, 465);

    /* since min>max, min and max values should be swapped by DTM_SETRANGE
    automatically */
    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    todo_wine {
        ok(compare_systime(&st[0], &getSt[0]) == 1 ||
           broken(compare_systime(&st[0], &getSt[1]) == 1), /* comctl32 version  <= 5.80 */
           "ST1 != ST2\n");

        ok(compare_systime(&st[1], &getSt[1]) == 1 ||
           broken(compare_systime(&st[1], &getSt[0]) == 1), /* comctl32 version  <= 5.80 */
           "ST1 != ST2\n");
    }

    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt[0]);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    /* the time part seems to not change after swapping the min and max values
    and doing DTM_SETSYSTEMTIME */
    expect_systime_date(&st[0], &getSt[0]);
    todo_wine {
        ok(compare_systime_time(&origSt, &getSt[0]) == 1 ||
           broken(compare_systime_time(&st[0], &getSt[0]) == 1), /* comctl32 version  <= 5.80 */
           "ST1.time != ST2.time\n");
    }

    /* set st[0] to value higher than minimum */
    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);
    /* set st[1] to value lower than maximum */
    fill_systime_struct(&st[1], 2007, 3, 2, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    /* for some reason after we swapped the min and max values before,
    whenever we do a DTM_SETRANGE, the DTM_GETRANGE will return the values
    swapped*/
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    todo_wine {
        ok(compare_systime(&st[0], &getSt[1]) == 1 ||
           broken(compare_systime(&st[0], &getSt[0]) == 1), /* comctl32 version  <= 5.80 */
           "ST1 != ST2\n");

        ok(compare_systime(&st[1], &getSt[0]) == 1 ||
           broken(compare_systime(&st[1], &getSt[1]) == 1), /* comctl32 version <= 5.80 */
           "ST1 != ST2\n");
    }

    /* set st[0] to value higher than st[1] */
    fill_systime_struct(&st[0], 2007, 3, 2, 31, 23, 59, 59, 999);
    fill_systime_struct(&st[1], 1980, 1, 3, 23, 14, 34, 37, 465);

    /* set min>max again, so that the return values of DTM_GETRANGE are no
    longer swapped the next time we do a DTM SETRANGE and DTM_GETRANGE*/
    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[1]);
    expect_systime(&st[1], &getSt[0]);

    /* initialize st[0] to lowest possible value */
    fill_systime_struct(&st[0], 1601, 1, 0, 1, 0, 0, 0, 0);
    /* set st[1] to highest possible value */
    fill_systime_struct(&st[1], 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_range_swap_min_max_seq, "test_dtm_set_range_swap_min_max", FALSE);

    DestroyWindow(hWnd);
}

static void test_dtm_set_and_get_system_time(void)
{
    LRESULT r;
    SYSTEMTIME st, getSt, ref;
    HWND hWnd;

    hWnd = create_datetime_control(0);
    ok(hWnd !=NULL, "Expected non NULL, got %p\n", hWnd);
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM)&st);
    expect(0, r);

    DestroyWindow(hWnd);

    hWnd = create_datetime_control(DTS_SHOWNONE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM)&st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    ok(r == GDT_NONE, "Expected %d, not %d(GDT_VALID) or %d(GDT_ERROR), got %Id\n", GDT_NONE, GDT_VALID, GDT_ERROR, r);

    /* set st to lowest possible value */
    fill_systime_struct(&st, 1601, 1, 0, 1, 0, 0, 0, 0);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);

    /* set st to highest possible value */
    fill_systime_struct(&st, 30827, 12, 6, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);

    /* set st to value between min and max */
    fill_systime_struct(&st, 1980, 1, 3, 23, 14, 34, 37, 465);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&st, &getSt);

    /* set st to invalid value */
    fill_systime_struct(&st, 0, 0, 7, 0, 24, 60, 60, 1000);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect_unsuccess(0, r);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_and_get_system_time_seq, "test_dtm_set_and_get_system_time", FALSE);

    /* set to some valid value */
    GetSystemTime(&ref);
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&ref);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);

    /* year invalid */
    st = ref;
    st.wYear = 0;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    todo_wine expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* month invalid */
    st = ref;
    st.wMonth = 13;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* day invalid */
    st = ref;
    st.wDay = 32;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* day invalid for current month */
    st = ref;
    st.wDay = 30;
    st.wMonth = 2;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* day of week isn't validated */
    st = ref;
    st.wDayOfWeek = 10;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* hour invalid */
    st = ref;
    st.wHour = 25;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* minute invalid */
    st = ref;
    st.wMinute = 60;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* sec invalid */
    st = ref;
    st.wSecond = 60;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);
    /* msec invalid */
    st = ref;
    st.wMilliseconds = 1000;
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(0, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    expect_systime(&ref, &getSt);

    /* day of week should be calculated automatically,
       actual day of week for this date is 4 */
    fill_systime_struct(&st, 2009, 10, 1, 1, 0, 0, 10, 200);
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt);
    expect(GDT_VALID, r);
    /* 01.10.2009 is Thursday */
    expect(4, (LRESULT)getSt.wDayOfWeek);
    st.wDayOfWeek = 4;
    expect_systime(&st, &getSt);
}

static void test_dtm_set_and_get_systemtime_with_limits(void)
{
    LRESULT r;
    SYSTEMTIME st[2], getSt[2], refSt;
    HWND hWnd;

    hWnd = create_datetime_control(DTS_SHOWNONE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* set range */
    fill_systime_struct(&st[0], 1980, 1, 3, 23, 14, 34, 37, 465);
    fill_systime_struct(&st[1], 2007, 3, 2, 31, 23, 59, 59, 999);

    r = SendMessageA(hWnd, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETRANGE, 0, (LPARAM)getSt);
    ok(r == (GDTR_MIN | GDTR_MAX), "Expected %x, not %x(GDTR_MIN) or %x(GDTR_MAX), got %Ix\n", (GDTR_MIN | GDTR_MAX), GDTR_MIN, GDTR_MAX, r);
    expect_systime(&st[0], &getSt[0]);
    expect_systime(&st[1], &getSt[1]);

    /* Initially set a valid time */
    fill_systime_struct(&refSt, 1999, 9, 4, 9, 19, 9, 9, 999);
    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&refSt);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt[0]);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&refSt, &getSt[0]);

    /* Now set an out-of-bounds time */
    fill_systime_struct(&st[0], 2010, 1, 0, 1, 0, 0, 0, 0);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt[0]);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&refSt, &getSt[0]);

    fill_systime_struct(&st[0], 1977, 1, 0, 1, 0, 0, 0, 0);

    r = SendMessageA(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st[0]);
    expect(1, r);
    r = SendMessageA(hWnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&getSt[0]);
    ok(r == GDT_VALID, "Expected %d, not %d(GDT_NONE) or %d(GDT_ERROR), got %Id\n", GDT_VALID, GDT_NONE, GDT_ERROR, r);
    expect_systime(&refSt, &getSt[0]);

    ok_sequence(sequences, DATETIME_SEQ_INDEX, test_dtm_set_and_get_systime_with_limits, "test_dtm_set_and_get_systime_with_limits", FALSE);

    DestroyWindow(hWnd);
}

static void test_dtm_get_ideal_size(void)
{
    HWND hwnd;
    HDC hdc;
    HFONT hfont;
    LOGFONTA lf;
    TEXTMETRICA tm;
    SIZE size;
    BOOL r;

    hwnd = create_datetime_control(0);
    r = SendMessageA(hwnd, DTM_GETIDEALSIZE, 0, (LPARAM)&size);
    if (!r)
    {
        win_skip("DTM_GETIDEALSIZE is not available\n");
        DestroyWindow(hwnd);
        return;
    }

    /* Set font so that the test is consistent on Wine and Windows */
    ZeroMemory(&lf, sizeof(lf));
    lf.lfWeight = FW_NORMAL;
    lf.lfHeight = 20;
    lstrcpyA(lf.lfFaceName, "Tahoma");
    hfont = CreateFontIndirectA(&lf);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

    hdc = GetDC(hwnd);
    GetTextMetricsA(hdc, &tm);
    ReleaseDC(hwnd, hdc);

    r = SendMessageA(hwnd, DTM_GETIDEALSIZE, 0, (LPARAM)&size);
    ok(r, "Expect DTM_GETIDEALSIZE message to return true\n");
    ok(size.cx > 0 && size.cy >= tm.tmHeight,
       "Expect size.cx > 0 and size.cy >= %ld, got cx:%ld cy:%ld\n", tm.tmHeight, size.cx, size.cy);

    DestroyWindow(hwnd);
    DeleteObject(hfont);
}

static void test_wm_set_get_text(void)
{
    static const CHAR a_str[] = "a";
    CHAR buff[16], time[16], caltype[3];
    WCHAR buffW[16];
    HWND hWnd;
    LRESULT ret;

    hWnd = create_datetime_control(0);

    ret = SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)a_str);
    ok(CB_ERR == ret ||
       broken(0 == ret) || /* comctl32 <= 4.72 */
       broken(1 == ret), /* comctl32 <= 4.70 */
       "Expected CB_ERR, got %Id\n", ret);

    buff[0] = 0;
    ret = SendMessageA(hWnd, WM_GETTEXT, sizeof(buff), (LPARAM)buff);
    ok(strcmp(buff, a_str) != 0, "Expected text to change, got %s\n", buff);
    ok(ret != 0, "Expected non-zero return value\n");

    SetLastError(0xdeadbeef);
    ret = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, caltype, 3);
    if (ret == 0)
        skip("Must know local calendar type (%lx)\n", GetLastError());
    else if (atoi(caltype) != CAL_GREGORIAN)
        skip("DateTimePicker Control only supports Gregorian calendar (type: %s)\n", caltype);
    else {
        SetLastError(0xdeadbeef);
        ret = GetDateFormatA(LOCALE_USER_DEFAULT, 0, NULL, NULL, time, sizeof(time));
        if (ret == 0)
            skip("GetDateFormat failed, returned %Id, error %ld\n", ret, GetLastError());
        else
        {
            ok(!strcmp(buff, time), "Expected %s, got %s\n", time, buff);

            ret = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
            ok(ret == strlen(time), "Got wrong length: %Id, expected %Id.\n", ret, strlen(time));
        }
    }

    DestroyWindow(hWnd);

    /* Window text is not preserved. */
    hWnd = CreateWindowExA(0, DATETIMEPICK_CLASSA, "testname", 0, 0, 50, 300, 120,
            NULL, NULL, NULL, NULL);

    buffW[0] = 1;
    InternalGetWindowText(hWnd, buffW, ARRAY_SIZE(buffW));
    ok(!buffW[0], "Unexpected window text %s.\n", wine_dbgstr_w(buffW));

    DestroyWindow(hWnd);
}

static void test_dts_shownone(void)
{
    HWND hwnd;
    DWORD style;

    /* it isn't allowed to change DTS_SHOWNONE after creation */
    hwnd = create_datetime_control(0);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style | DTS_SHOWNONE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(style & DTS_SHOWNONE), "Expected DTS_SHOWNONE not to be set\n");
    DestroyWindow(hwnd);

    hwnd = create_datetime_control(DTS_SHOWNONE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style & ~DTS_SHOWNONE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    ok(style & DTS_SHOWNONE, "Expected DTS_SHOWNONE to be set\n");
    DestroyWindow(hwnd);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(InitCommonControlsEx);
#undef X
}

START_TEST(datetime)
{
    INITCOMMONCONTROLSEX iccex;
    ULONG_PTR cookie;
    HANDLE ctxt;

    init_functions();

    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_DATE_CLASSES;
    pInitCommonControlsEx(&iccex);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_dtm_set_format();
    test_dtm_set_and_get_mccolor();
    test_dtm_set_and_get_mcfont();
    test_dtm_get_monthcal();
    test_dtm_set_and_get_range();
    test_dtm_set_range_swap_min_max();
    test_dtm_set_and_get_system_time();
    test_dtm_set_and_get_systemtime_with_limits();
    test_wm_set_get_text();
    test_dts_shownone();

    if (!load_v6_module(&cookie, &ctxt))
        return;

    test_dtm_set_format();
    test_dtm_set_and_get_mccolor();
    test_dtm_set_and_get_mcfont();
    test_dtm_get_monthcal();
    test_dtm_get_ideal_size();
    test_wm_set_get_text();
    test_dts_shownone();

    unload_v6_module(cookie, ctxt);
}
