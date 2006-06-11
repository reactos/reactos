/*
 * comctl32 month calendar unit tests
 *
 * Copyright (C) 2006 Vitaliy Margolen
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

#include "windows.h"

#include "commctrl.h"

#include "wine/test.h"

void test_monthcal(void)
{
    HWND hwnd;
    SYSTEMTIME st[2], st1[2];
    INITCOMMONCONTROLSEX ic = {sizeof(INITCOMMONCONTROLSEX), ICC_DATE_CLASSES};
    int res, month_range;

    InitCommonControlsEx(&ic);
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
}

START_TEST(monthcal)
{
    test_monthcal();
}
