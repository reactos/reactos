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

#include <stdarg.h>
#include <stdio.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "wine/test.h"

#define COMBO_ID 1995

static HWND hMainWnd;

#define expect_eq(expr, value, type, fmt); { type val = expr; ok(val == (value), #expr " expected " #fmt " got " #fmt "\n", (value), val); }
#define expect_rect(r, _left, _top, _right, _bottom) ok(r.left == _left && r.top == _top && \
    r.bottom == _bottom && r.right == _right, "Invalid rect (%d,%d) (%d,%d) vs (%d,%d) (%d,%d)\n", \
    r.left, r.top, r.right, r.bottom, _left, _top, _right, _bottom);

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
    BOOL (WINAPI *pGetComboBoxInfo)(HWND, PCOMBOBOXINFO);

    pGetComboBoxInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetComboBoxInfo");
    if (!pGetComboBoxInfo){
        win_skip("GetComboBoxInfo is not available\n");
        return;
    }

    hCombo = CreateWindowA("ComboBox", "Combo", WS_VISIBLE|WS_CHILD|CBS_DROPDOWN,
            0, 0, 200, 150, hMainWnd, (HMENU)COMBO_ID, NULL, 0);

    for (i = 0; i < sizeof(choices)/sizeof(UINT); i++){
        sprintf(buffer, stringFormat, choices[i]);
        result = SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
        ok(result == i,
           "Failed to add item %d\n", i);
    }

    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = pGetComboBoxInfo(hCombo, &cbInfo);
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
    ok( rc.right - rc.left == clwidth - 2, "clientrect witdh is %d vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %d vs %d\n",
                rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth - 2, "drop-down rect witdh is %d vs %d\n",
            rc.right - rc.left, clwidth - 2);
    ok( rc.bottom - rc.top == ddheight, "drop-down rect height is %d vs %d\n",
            rc.bottom - rc.top, ddheight);
    ok( rc.right - rc.left == ddwidth -2, "drop-down rect width is %d vs %d\n",
            rc.right - rc.left, ddwidth - 2);
    /* new cx, cy is slightly bigger than the initial values */
    MoveWindow( hCombo, 10, 10, clwidth + 2, clheight + 2, TRUE);
    GetClientRect( hCombo, &rc);
    ok( rc.right - rc.left == clwidth + 2, "clientrect witdh is %d vs %d\n",
            rc.right - rc.left, clwidth + 2);
    ok( rc.bottom - rc.top == clheight, "clientrect height is %d vs %d\n",
            rc.bottom - rc.top, clheight);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);
    ok( rc.right - rc.left == clwidth + 2, "drop-down rect witdh is %d vs %d\n",
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
    BOOL (WINAPI *pGetComboBoxInfo)(HWND, PCOMBOBOXINFO);
    char edit[20];

    pGetComboBoxInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetComboBoxInfo");
    if (!pGetComboBoxInfo){
        win_skip("GetComboBoxInfo is not available\n");
        return;
    }

    /* Build a combo */
    hCombo = build_combo(CBS_SIMPLE);
    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    SetLastError(0xdeadbeef);
    ret = pGetComboBoxInfo(hCombo, &cbInfo);
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
    len = SendMessageA(hCombo, CB_GETEDITSEL, 0,0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    /* Give it focus, and it gets selected */
    SendMessageA(hCombo, WM_SETFOCUS, 0, (LPARAM)hEdit);
    SendMessageA(hCombo, CB_GETEDITSEL, (WPARAM)&start, (WPARAM)&end);
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
    ret = pGetComboBoxInfo(hCombo, &cbInfo);
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

static void test_listbox_styles(DWORD cb_style)
{
    BOOL (WINAPI *pGetComboBoxInfo)(HWND, PCOMBOBOXINFO);
    HWND combo;
    COMBOBOXINFO info;
    DWORD style, exstyle, expect_style, expect_exstyle;
    BOOL ret;

    pGetComboBoxInfo = (void*)GetProcAddress(GetModuleHandleA("user32.dll"), "GetComboBoxInfo");
    if (!pGetComboBoxInfo){
        win_skip("GetComboBoxInfo is not available\n");
        return;
    }

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
    ret = pGetComboBoxInfo(combo, &info);
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

START_TEST(combo)
{
    hMainWnd = CreateWindowA("static", "Test", WS_OVERLAPPEDWINDOW, 10, 10, 300, 300, NULL, NULL, NULL, 0);
    ShowWindow(hMainWnd, SW_SHOW);

    test_setfont(CBS_DROPDOWN);
    test_setfont(CBS_DROPDOWNLIST);
    test_setitemheight(CBS_DROPDOWN);
    test_setitemheight(CBS_DROPDOWNLIST);
    test_CBN_SELCHANGE();
    test_WM_LBUTTONDOWN();
    test_changesize(CBS_DROPDOWN);
    test_changesize(CBS_DROPDOWNLIST);
    test_editselection();
    test_listbox_styles(CBS_SIMPLE);
    test_listbox_styles(CBS_DROPDOWN);
    test_listbox_styles(CBS_DROPDOWNLIST);

    DestroyWindow(hMainWnd);
}
