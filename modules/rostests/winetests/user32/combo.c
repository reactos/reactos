/* Unit test suite for combo boxes.
 *
 * Copyright 2007 Mikolaj Zalewski
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

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "wine/test.h"

#define COMBO_ID 1995

#define COMBO_YBORDERSIZE() 2

static HWND hMainWnd;

#define expect_eq(expr, value, type, fmt); { type val = expr; ok(val == (value), #expr " expected " #fmt " got " #fmt "\n", (value), val); }
#define expect_rect(r, _left, _top, _right, _bottom) ok(r.left == _left && r.top == _top && \
    r.bottom == _bottom && r.right == _right, "Invalid rect %s vs (%d,%d)-(%d,%d)\n", \
    wine_dbgstr_rect(&r), _left, _top, _right, _bottom);

static HWND build_combo(DWORD style)
{
    return CreateWindowA("ComboBox", "Combo", WS_VISIBLE|WS_CHILD|style, 5, 5, 100, 100, hMainWnd, (HMENU)COMBO_ID, NULL, 0);
}

static int font_height(HFONT hFont)
{
    TEXTMETRICA tm;
    HFONT hFontOld;
    HDC hDC;

    hDC = CreateCompatibleDC(NULL);
    hFontOld = SelectObject(hDC, hFont);
    GetTextMetricsA(hDC, &tm);
    SelectObject(hDC, hFontOld);
    DeleteDC(hDC);

    return tm.tmHeight;
}

static INT CALLBACK is_font_installed_proc(const LOGFONTA *elf, const TEXTMETRICA *tm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const char *name)
{
    HDC hdc = GetDC(NULL);
    BOOL ret = !EnumFontFamiliesA(hdc, name, is_font_installed_proc, 0);
    ReleaseDC(NULL, hdc);
    return ret;
}

static void test_setitemheight(DWORD style)
{
    HWND hCombo = build_combo(style);
    RECT r;
    int i;

    trace("Style %x\n", style);
    GetClientRect(hCombo, &r);
    expect_rect(r, 0, 0, 100, font_height(GetStockObject(SYSTEM_FONT)) + 8);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    todo_wine expect_rect(r, 5, 5, 105, 105);

    for (i = 1; i < 30; i++)
    {
        SendMessageA(hCombo, CB_SETITEMHEIGHT, -1, i);
        GetClientRect(hCombo, &r);
        expect_eq(r.bottom - r.top, i + 6, int, "%d");
    }

    DestroyWindow(hCombo);
}

static void test_setfont(DWORD style)
{
    HWND hCombo;
    HFONT hFont1, hFont2;
    RECT r;
    int i;

    if (!is_font_installed("Marlett"))
    {
        skip("Marlett font not available\n");
        return;
    }

    trace("Style %x\n", style);

    hCombo = build_combo(style);
    hFont1 = CreateFontA(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
    hFont2 = CreateFontA(8, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");

    GetClientRect(hCombo, &r);
    expect_rect(r, 0, 0, 100, font_height(GetStockObject(SYSTEM_FONT)) + 8);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    todo_wine expect_rect(r, 5, 5, 105, 105);

    /* The size of the dropped control is initially equal to the size
       of the window when it was created.  The size of the calculated
       dropped area changes only by how much the selection area
       changes, not by how much the list area changes.  */
    if (font_height(hFont1) == 10 && font_height(hFont2) == 8)
    {
        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (font_height(GetStockObject(SYSTEM_FONT)) - font_height(hFont1)));

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont2, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 16);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (font_height(GetStockObject(SYSTEM_FONT)) - font_height(hFont2)));

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 105 - (font_height(GetStockObject(SYSTEM_FONT)) - font_height(hFont1)));
    }
    else
    {
        ok(0, "Expected Marlett font heights 10/8, got %d/%d\n",
           font_height(hFont1), font_height(hFont2));
    }

    for (i = 1; i < 30; i++)
    {
        HFONT hFont = CreateFontA(i, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
        int height = font_height(hFont);

        SendMessageA(hCombo, WM_SETFONT, (WPARAM)hFont, FALSE);
        GetClientRect(hCombo, &r);
        expect_eq(r.bottom - r.top, height + 8, int, "%d");
        SendMessageA(hCombo, WM_SETFONT, 0, FALSE);
        DeleteObject(hFont);
    }

    DestroyWindow(hCombo);
    DeleteObject(hFont1);
    DeleteObject(hFont2);
}

static LRESULT (CALLBACK *old_parent_proc)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static LPCSTR expected_edit_text;
static LPCSTR expected_list_text;
static BOOL selchange_fired;

static LRESULT CALLBACK parent_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (wparam)
        {
            case MAKEWPARAM(COMBO_ID, CBN_SELCHANGE):
            {
                HWND hCombo = (HWND)lparam;
                int idx;
                char list[20], edit[20];

                memset(list, 0, sizeof(list));
                memset(edit, 0, sizeof(edit));

                idx = SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
                SendMessageA(hCombo, CB_GETLBTEXT, idx, (LPARAM)list);
                SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);

                ok(!strcmp(edit, expected_edit_text), "edit: got %s, expected %s\n",
                   edit, expected_edit_text);
                ok(!strcmp(list, expected_list_text), "list: got %s, expected %s\n",
                   list, expected_list_text);

                selchange_fired = TRUE;
            }
            break;
        }
        break;
    }

    return CallWindowProcA(old_parent_proc, hwnd, msg, wparam, lparam);
}

static void test_selection(DWORD style, const char * const text[],
                           const int *edit, const int *list)
{
    INT idx;
    HWND hCombo;

    hCombo = build_combo(style);

    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)text[0]);
    SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)text[1]);
    SendMessageA(hCombo, CB_SETCURSEL, -1, 0);

    old_parent_proc = (void *)SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)parent_wnd_proc);

    idx = SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
    ok(idx == -1, "expected selection -1, got %d\n", idx);

    /* keyboard navigation */

    expected_list_text = text[list[0]];
    expected_edit_text = text[edit[0]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_DOWN, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    expected_list_text = text[list[1]];
    expected_edit_text = text[edit[1]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_DOWN, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    expected_list_text = text[list[2]];
    expected_edit_text = text[edit[2]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, WM_KEYDOWN, VK_UP, 0);
    ok(selchange_fired, "CBN_SELCHANGE not sent!\n");

    /* programmatic navigation */

    expected_list_text = text[list[3]];
    expected_edit_text = text[edit[3]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, CB_SETCURSEL, list[3], 0);
    ok(!selchange_fired, "CBN_SELCHANGE sent!\n");

    expected_list_text = text[list[4]];
    expected_edit_text = text[edit[4]];
    selchange_fired = FALSE;
    SendMessageA(hCombo, CB_SETCURSEL, list[4], 0);
    ok(!selchange_fired, "CBN_SELCHANGE sent!\n");

    SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)old_parent_proc);
    DestroyWindow(hCombo);
}

static void test_CBN_SELCHANGE(void)
{
    static const char * const text[] = { "alpha", "beta", "" };
    static const int sel_1[] = { 2, 0, 1, 0, 1 };
    static const int sel_2[] = { 0, 1, 0, 0, 1 };

    test_selection(CBS_SIMPLE, text, sel_1, sel_2);
    test_selection(CBS_DROPDOWN, text, sel_1, sel_2);
    test_selection(CBS_DROPDOWNLIST, text, sel_2, sel_2);
}

static void test_WM_LBUTTONDOWN(void)
{
    HWND hCombo, hEdit, hList;
    COMBOBOXINFO cbInfo;
    UINT x, y, item_height;
    LRESULT result;
    int i, idx;
    RECT rect;
    CHAR buffer[3];
    static const UINT choices[] = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
    static const CHAR stringFormat[] = "%2d";
    BOOL ret;

    hCombo = CreateWindowA("ComboBox", "Combo", WS_VISIBLE|WS_CHILD|CBS_DROPDOWN,
            0, 0, 200, 150, hMainWnd, (HMENU)COMBO_ID, NULL, 0);

    for (i = 0; i < ARRAY_SIZE(choices); i++){
        sprintf(buffer, stringFormat, choices[i]);
        result = SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
        ok(result == i,
           "Failed to add item %d\n", i);
    }

    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(hCombo, &cbInfo);
    ok(ret, "Failed to get combobox info structure. LastError=%d\n",
       GetLastError());
    hEdit = cbInfo.hwndItem;
    hList = cbInfo.hwndList;

    trace("hMainWnd=%p, hCombo=%p, hList=%p, hEdit=%p\n", hMainWnd, hCombo, hList, hEdit);
    ok(GetFocus() == hMainWnd, "Focus not on Main Window, instead on %p\n", GetFocus());

    /* Click on the button to drop down the list */
    x = cbInfo.rcButton.left + (cbInfo.rcButton.right-cbInfo.rcButton.left)/2;
    y = cbInfo.rcButton.top + (cbInfo.rcButton.bottom-cbInfo.rcButton.top)/2;
    result = SendMessageA(hCombo, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
       GetLastError());
    ok(SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should have appeared after clicking the button.\n");

    ok(GetFocus() == hEdit,
       "Focus not on ComboBox's Edit Control, instead on %p\n", GetFocus());
    result = SendMessageA(hCombo, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(result, "WM_LBUTTONUP was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hEdit,
       "Focus not on ComboBox's Edit Control, instead on %p\n", GetFocus());

    /* Click on the 5th item in the list */
    item_height = SendMessageA(hCombo, CB_GETITEMHEIGHT, 0, 0);
    ok(GetClientRect(hList, &rect), "Failed to get list's client rect.\n");
    x = rect.left + (rect.right-rect.left)/2;
    y = item_height/2 + item_height*4;
    result = SendMessageA(hList, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hEdit,
       "Focus not on ComboBox's Edit Control, instead on %p\n", GetFocus());

    result = SendMessageA(hList, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
    ok(!result, "WM_MOUSEMOVE was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hEdit,
       "Focus not on ComboBox's Edit Control, instead on %p\n", GetFocus());
    ok(SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should still be visible.\n");

    result = SendMessageA(hList, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    ok(!result, "WM_LBUTTONUP was not processed. LastError=%d\n",
       GetLastError());
    ok(GetFocus() == hEdit,
       "Focus not on ComboBox's Edit Control, instead on %p\n", GetFocus());
    ok(!SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0),
       "The dropdown list should have been rolled up.\n");
    idx = SendMessageA(hCombo, CB_GETCURSEL, 0, 0);
    ok(idx, "Current Selection: expected %d, got %d\n", 4, idx);

    DestroyWindow(hCombo);
}

static void test_changesize( DWORD style)
{
    HWND hCombo = build_combo(style);
    RECT rc;
    INT ddheight, clheight, ddwidth, clwidth;
    /* get initial measurements */
    GetClientRect( hCombo, &rc);
    clheight = rc.bottom - rc.top;
    clwidth = rc.right - rc.left;
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ddheight = rc.bottom - rc.top;
    ddwidth = rc.right - rc.left;
    /* use MoveWindow to move & resize the combo */
    /* first make it slightly smaller */
    MoveWindow( hCombo, 10, 10, clwidth - 2, clheight - 2, TRUE);
    GetClientRect( hCombo, &rc);
    ok( rc.right - rc.left == clwidth - 2, "clientrect width is %d vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %d vs %d\n",
                rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth - 2, "drop-down rect width is %d vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == ddheight, "drop-down rect height is %d vs %d\n",
            rc.bottom - rc.top, ddheight);
    ok( rc.right - rc.left == ddwidth -2, "drop-down rect width is %d vs %d\n",
            rc.right - rc.left, ddwidth - 2);
    /* new cx, cy is slightly bigger than the initial values */
    MoveWindow( hCombo, 10, 10, clwidth + 2, clheight + 2, TRUE);
    GetClientRect( hCombo, &rc);
    ok( rc.right - rc.left == clwidth + 2, "clientrect width is %d vs %d\n",
            rc.right - rc.left, clwidth + 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %d vs %d\n",
            rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth + 2, "drop-down rect width is %d vs %d\n",
            rc.right - rc.left, clwidth + 2);
    todo_wine {
        ok( rc.bottom - rc.top == clheight + 2, "drop-down rect height is %d vs %d\n",
                rc.bottom - rc.top, clheight + 2);
    }

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, -1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, clwidth - 1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, clwidth << 1, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == (clwidth << 1), "drop-width is %d vs %d\n", ddwidth, clwidth << 1);

    ddwidth = SendMessageA(hCombo, CB_SETDROPPEDWIDTH, 1, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);
    ddwidth = SendMessageA(hCombo, CB_GETDROPPEDWIDTH, 0, 0);
    ok( ddwidth == clwidth + 2, "drop-width is %d vs %d\n", ddwidth, clwidth + 2);

    DestroyWindow(hCombo);
}

static void test_editselection(void)
{
    HWND hCombo;
    INT start,end;
    HWND hEdit;
    COMBOBOXINFO cbInfo;
    BOOL ret;
    DWORD len;
    char edit[20];

    /* Build a combo */
    hCombo = build_combo(CBS_SIMPLE);
    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(hCombo, &cbInfo);
    ok(ret, "Failed to get combobox info structure. LastError=%d\n",
       GetLastError());
    hEdit = cbInfo.hwndItem;

    /* Initially combo selection is empty*/
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Set some text, and press a key to replace it */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)"Jason1");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason1")==0, "Unexpected text retrieved %s\n", edit);

    /* Now what is the selection - still empty */
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==0, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Give it focus, and it gets selected */
    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==6, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==6, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now emulate a key press */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_CHAR, 'A', 0x1c0001);
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "A")==0, "Unexpected text retrieved %s\n", edit);

    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==1, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==1, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now what happens when it gets more focus a second time - it doesn't reselect */
    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==1, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==1, "Unexpected end position for selection %d\n", HIWORD(len));
    DestroyWindow(hCombo);

    /* Start again - Build a combo */
    hCombo = build_combo(CBS_SIMPLE);
    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(hCombo, &cbInfo);
    ok(ret, "Failed to get combobox info structure. LastError=%d\n",
       GetLastError());
    hEdit = cbInfo.hwndItem;

    /* Set some text and give focus so it gets selected */
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)"Jason2");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason2")==0, "Unexpected text retrieved %s\n", edit);

    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);

    /* Now what is the selection */
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
    ok(start==0, "Unexpected start position for selection %d\n", start);
    ok(end==6, "Unexpected end position for selection %d\n", end);
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==6, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Now change the selection to the apparently invalid start -1, end -1 and
       show it means no selection (ie start -1) but cursor at end              */
    SendMessageA(hCombo, CB_SETEDITSEL, 0, -1);
    edit[0] = 0x00;
    SendMessageA(hCombo, WM_CHAR, 'A', 0x1c0001);
    SendMessageA(hCombo, WM_GETTEXT, sizeof(edit), (LPARAM)edit);
    ok(strcmp(edit, "Jason2A")==0, "Unexpected text retrieved %s\n", edit);
    DestroyWindow(hCombo);
}

static WNDPROC edit_window_proc;
static long setsel_start = 1, setsel_end = 1;
static HWND hCBN_SetFocus, hCBN_KillFocus;

static LRESULT CALLBACK combobox_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == EM_SETSEL)
    {
        setsel_start = wParam;
        setsel_end = lParam;
    }
    return CallWindowProcA(edit_window_proc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK test_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case CBN_SETFOCUS:
            hCBN_SetFocus = (HWND)lParam;
            break;
        case CBN_KILLFOCUS:
            hCBN_KillFocus = (HWND)lParam;
            break;
        }
        break;
    case WM_NEXTDLGCTL:
        SetFocus((HWND)wParam);
        break;
    }
    return CallWindowProcA(old_parent_proc, hwnd, msg, wParam, lParam);
}

static void test_editselection_focus(DWORD style)
{
    HWND hCombo, hEdit, hButton;
    COMBOBOXINFO cbInfo;
    BOOL ret;
    const char wine_test[] = "Wine Test";
    char buffer[16] = {0};
    DWORD len;

    hCombo = build_combo(style);
    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(hCombo, &cbInfo);
    ok(ret, "Failed to get COMBOBOXINFO structure; LastError: %u\n", GetLastError());
    hEdit = cbInfo.hwndItem;

    hButton = CreateWindowA("Button", "OK", WS_VISIBLE|WS_CHILD|BS_DEFPUSHBUTTON,
                            5, 50, 100, 20, hMainWnd, NULL,
                            (HINSTANCE)GetWindowLongPtrA(hMainWnd, GWLP_HINSTANCE), NULL);

    old_parent_proc = (WNDPROC)SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)test_window_proc);
    edit_window_proc = (WNDPROC)SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (ULONG_PTR)combobox_subclass_proc);

    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == INT_MAX, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_SetFocus == hCombo, "Wrong handle set by CBN_SETFOCUS; got %p\n", hCBN_SetFocus);
    ok(GetFocus() == hEdit, "hEdit should have keyboard focus\n");

    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hButton, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == 0, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_KillFocus == hCombo, "Wrong handle set by CBN_KILLFOCUS; got %p\n", hCBN_KillFocus);
    ok(GetFocus() == hButton, "hButton should have keyboard focus\n");

    SendMessageA(hCombo, WM_SETTEXT, 0, (LPARAM)wine_test);
    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hCombo, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == INT_MAX, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_SetFocus == hCombo, "Wrong handle set by CBN_SETFOCUS; got %p\n", hCBN_SetFocus);
    ok(GetFocus() == hEdit, "hEdit should have keyboard focus\n");
    SendMessageA(hCombo, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ok(!strcmp(buffer, wine_test), "Unexpected text in edit control; got '%s'\n", buffer);

    SendMessageA(hMainWnd, WM_NEXTDLGCTL, (WPARAM)hButton, TRUE);
    ok(setsel_start == 0, "Unexpected EM_SETSEL start value; got %ld\n", setsel_start);
    todo_wine ok(setsel_end == 0, "Unexpected EM_SETSEL end value; got %ld\n", setsel_end);
    ok(hCBN_KillFocus == hCombo, "Wrong handle set by CBN_KILLFOCUS; got %p\n", hCBN_KillFocus);
    ok(GetFocus() == hButton, "hButton should have keyboard focus\n");
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0, 0);
    ok(len == 0, "Unexpected text selection; start: %u, end: %u\n", LOWORD(len), HIWORD(len));

    SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (ULONG_PTR)old_parent_proc);
    DestroyWindow(hButton);
    DestroyWindow(hCombo);
}

static void test_listbox_styles(DWORD cb_style)
{
    HWND combo;
    COMBOBOXINFO info;
    DWORD style, exstyle, expect_style, expect_exstyle;
    BOOL ret;

    expect_style = WS_CHILD|WS_CLIPSIBLINGS|LBS_COMBOBOX|LBS_HASSTRINGS|LBS_NOTIFY;
    if (cb_style == CBS_SIMPLE)
    {
        expect_style |= WS_VISIBLE;
        expect_exstyle = WS_EX_CLIENTEDGE;
    }
    else
    {
        expect_style |= WS_BORDER;
        expect_exstyle = WS_EX_TOOLWINDOW;
    }

    combo = build_combo(cb_style);
    info.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(combo, &info);
    ok(ret, "Failed to get combobox info structure.\n");

    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == expect_style, "%08x: got %08x\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08x: got %08x\n", cb_style, exstyle);

    if (cb_style != CBS_SIMPLE)
        expect_exstyle |= WS_EX_TOPMOST;

    SendMessageW(combo, CB_SHOWDROPDOWN, TRUE, 0 );
    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == (expect_style | WS_VISIBLE), "%08x: got %08x\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08x: got %08x\n", cb_style, exstyle);

    SendMessageW(combo, CB_SHOWDROPDOWN, FALSE, 0 );
    style = GetWindowLongW( info.hwndList, GWL_STYLE );
    exstyle = GetWindowLongW( info.hwndList, GWL_EXSTYLE );
    ok(style == expect_style, "%08x: got %08x\n", cb_style, style);
    ok(exstyle == expect_exstyle, "%08x: got %08x\n", cb_style, exstyle);

    DestroyWindow(combo);
}

static void test_listbox_size(DWORD style)
{
    HWND hCombo, hList;
    COMBOBOXINFO cbInfo;
    UINT x, y;
    BOOL ret;
    int i, test;
    const char wine_test[] = "Wine Test";

    static const struct list_size_info
    {
        int num_items;
        int height_combo;
        BOOL todo;
    } info_height[] = {
        {2, 24, FALSE},
        {2, 41, TRUE},
        {2, 42, FALSE},
        {2, 50, FALSE},
        {2, 60},
        {2, 80},
        {2, 89},
        {2, 90},
        {2, 100},

        {10, 24, FALSE},
        {10, 41, TRUE},
        {10, 42, FALSE},
        {10, 50, FALSE},
        {10, 60, FALSE},
        {10, 80, FALSE},
        {10, 89, TRUE},
        {10, 90, FALSE},
        {10, 100, FALSE},
    };

    for(test = 0; test < ARRAY_SIZE(info_height); test++)
    {
        const struct list_size_info *info_test = &info_height[test];
        int height_item; /* Height of a list item */
        int height_list; /* Height of the list we got */
        int expected_count_list;
        int expected_height_list;
        int list_height_nonclient;
        int list_height_calculated;
        RECT rect_list_client, rect_list_complete;

        hCombo = CreateWindowA("ComboBox", "Combo", WS_VISIBLE|WS_CHILD|style, 5, 5, 100,
                               info_test->height_combo, hMainWnd, (HMENU)COMBO_ID, NULL, 0);

        cbInfo.cbSize = sizeof(COMBOBOXINFO);
        SetLastError(0xdeadbeef);
        ret = GetComboBoxInfo(hCombo, &cbInfo);
        ok(ret, "Failed to get COMBOBOXINFO structure; LastError: %u\n", GetLastError());

        hList = cbInfo.hwndList;
        for (i = 0; i < info_test->num_items; i++)
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM) wine_test);

        /* Click on the button to drop down the list */
        x = cbInfo.rcButton.left + (cbInfo.rcButton.right-cbInfo.rcButton.left)/2;
        y = cbInfo.rcButton.top + (cbInfo.rcButton.bottom-cbInfo.rcButton.top)/2;
        ret = SendMessageA(hCombo, WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
        ok(ret, "WM_LBUTTONDOWN was not processed. LastError=%d\n",
           GetLastError());
        ok(SendMessageA(hCombo, CB_GETDROPPEDSTATE, 0, 0),
           "The dropdown list should have appeared after clicking the button.\n");

        GetClientRect(hList, &rect_list_client);
        GetWindowRect(hList, &rect_list_complete);
        height_list = rect_list_client.bottom - rect_list_client.top;
        height_item = (int)SendMessageA(hList, LB_GETITEMHEIGHT, 0, 0);

        list_height_nonclient = (rect_list_complete.bottom - rect_list_complete.top)
                                - (rect_list_client.bottom - rect_list_client.top);

        /* Calculate the expected client size of the listbox popup from the size of the combobox. */
        list_height_calculated = info_test->height_combo
                - (cbInfo.rcItem.bottom + COMBO_YBORDERSIZE())
                - list_height_nonclient
                - 1;

        expected_count_list = list_height_calculated / height_item;
        if(expected_count_list < 0)
            expected_count_list = 0;
        expected_count_list = min(expected_count_list, info_test->num_items);
        expected_height_list = expected_count_list * height_item;

        todo_wine_if(info_test->todo)
        ok(expected_height_list == height_list,
           "Test %d, expected list height to be %d, got %d\n", test, expected_height_list, height_list);

        DestroyWindow(hCombo);
    }
}

static void test_WS_VSCROLL(void)
{
    HWND hCombo, hList;
    COMBOBOXINFO info;
    DWORD style;
    BOOL ret;
    int i;

    info.cbSize = sizeof(info);
    hCombo = build_combo(CBS_DROPDOWNLIST);

    SetLastError(0xdeadbeef);
    ret = GetComboBoxInfo(hCombo, &info);
    ok(ret, "Failed to get COMBOBOXINFO structure; LastError: %u\n", GetLastError());
    hList = info.hwndList;

    for(i = 0; i < 3; i++)
    {
        char buffer[2];
        sprintf(buffer, "%d", i);
        SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
    }

    style = GetWindowLongA(info.hwndList, GWL_STYLE);
    SetWindowLongA(hList, GWL_STYLE, style | WS_VSCROLL);

    SendMessageA(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
    SendMessageA(hCombo, CB_SHOWDROPDOWN, FALSE, 0);

    style = GetWindowLongA(hList, GWL_STYLE);
    ok((style & WS_VSCROLL) != 0, "Style does not include WS_VSCROLL\n");

    DestroyWindow(hCombo);
}

START_TEST(combo)
{
    hMainWnd = CreateWindowA("static", "Test", WS_OVERLAPPEDWINDOW, 10, 10, 300, 300, NULL, NULL, NULL, 0);
    ShowWindow(hMainWnd, SW_SHOW);

    test_WS_VSCROLL();
    test_setfont(CBS_DROPDOWN);
    test_setfont(CBS_DROPDOWNLIST);
    test_setitemheight(CBS_DROPDOWN);
    test_setitemheight(CBS_DROPDOWNLIST);
    test_CBN_SELCHANGE();
    test_WM_LBUTTONDOWN();
    test_changesize(CBS_DROPDOWN);
    test_changesize(CBS_DROPDOWNLIST);
    test_editselection();
    test_editselection_focus(CBS_SIMPLE);
    test_editselection_focus(CBS_DROPDOWN);
    test_listbox_styles(CBS_SIMPLE);
    test_listbox_styles(CBS_DROPDOWN);
    test_listbox_styles(CBS_DROPDOWNLIST);
    test_listbox_size(CBS_DROPDOWN);

    DestroyWindow(hMainWnd);
}
