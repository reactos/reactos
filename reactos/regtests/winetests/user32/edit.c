/* Unit test suite for edit control.
 *
 * Copyright 2004 Vitaliy Margolen
 * Copyright 2005 C. Scott Ananian
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "wine/test.h"

#ifndef ES_COMBO
#define ES_COMBO 0x200
#endif

#define ID_EDITTEST2 99
#define MAXLEN 200

struct edit_notify {
    int en_change, en_maxtext, en_update;
};

static struct edit_notify notifications;

static char szEditTest2Name[] = "Edit Test 2 window class";
static HINSTANCE hinst;
static HWND hwndET2;

static HWND create_editcontrol (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowEx(exstyle,
			  "EDIT",
			  NULL,
			  ES_AUTOHSCROLL | ES_AUTOVSCROLL | style,
			  10, 10, 300, 300,
			  NULL, NULL, NULL, NULL);
    assert (handle);
    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static LONG get_edit_style (HWND hwnd)
{
    return GetWindowLongA( hwnd, GWL_STYLE ) & (
	ES_LEFT |
/* FIXME: not implemented
	ES_CENTER |
	ES_RIGHT |
	ES_OEMCONVERT |
*/
	ES_MULTILINE |
	ES_UPPERCASE |
	ES_LOWERCASE |
	ES_PASSWORD |
	ES_AUTOVSCROLL |
	ES_AUTOHSCROLL |
	ES_NOHIDESEL |
	ES_COMBO |
	ES_READONLY |
	ES_WANTRETURN |
	ES_NUMBER
	);
}

static void set_client_height(HWND Wnd, unsigned Height)
{
    RECT ClientRect, WindowRect;

    GetWindowRect(Wnd, &WindowRect);
    GetClientRect(Wnd, &ClientRect);
    SetWindowPos(Wnd, NULL, WindowRect.left, WindowRect.top,
                 WindowRect.right - WindowRect.left,
                 Height + (WindowRect.bottom - WindowRect.top) - (ClientRect.bottom - ClientRect.top),
                 SWP_NOMOVE | SWP_NOZORDER);
}

#define edit_pos_ok(exp, got, txt) \
    ok(exp == got, "wrong " #txt " expected %d got %ld\n", exp, got);

#define edit_todo_pos_ok(exp, got, txt, todo) \
    if (todo) todo_wine { edit_pos_ok(exp, got, txt); } \
    else edit_pos_ok(exp, got, txt)

#define check_pos(hwEdit, set_height, test_top, test_height, test_left, todo_top, todo_height, todo_left) \
do { \
    RECT format_rect; \
    int left_margin; \
    set_client_height(hwEdit, set_height); \
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &format_rect); \
    left_margin = LOWORD(SendMessage(hwEdit, EM_GETMARGINS, 0, 0)); \
    edit_todo_pos_ok(test_top, format_rect.top, vertical position, todo_top); \
    edit_todo_pos_ok((int)test_height, format_rect.bottom - format_rect.top, height, todo_height); \
    edit_todo_pos_ok(test_left, format_rect.left - left_margin, left, todo_left); \
} while(0)

static void test_edit_control_1(void)
{
    HWND hwEdit;
    MSG msMessage;
    int i;
    LONG r;
    HFONT Font, OldFont;
    HDC Dc;
    TEXTMETRIC Metrics;

    msMessage.message = WM_KEYDOWN;

    trace("EDIT: Single line\n");
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL), "Wrong style expected 0xc0 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Single line want returns\n");
    hwEdit = create_editcontrol(ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN), "Wrong style expected 0x10c0 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multiline line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0xc4 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multi line want returns\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0x10c4 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    /* Get a stock font for which we can determine the metrics */
    Font = GetStockObject(SYSTEM_FONT);
    assert(NULL != Font);
    Dc = GetDC(NULL);
    assert(NULL != Dc);
    OldFont = SelectObject(Dc, Font);
    assert(NULL != OldFont);
    if (! GetTextMetrics(Dc, &Metrics))
    {
	assert(FALSE);
    }
    SelectObject(Dc, OldFont);
    ReleaseDC(NULL, Dc);

    trace("EDIT: Text position\n");
    hwEdit = create_editcontrol(0, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 1, 1, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 1, 1, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  3, 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 2, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 2, Metrics.tmHeight    , 2, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP, WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 0, Metrics.tmHeight    , 0, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  3, 0, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 2, Metrics.tmHeight    , 2, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 2, Metrics.tmHeight    , 2, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP, WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, ES_MULTILINE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, ES_MULTILINE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 1, 1, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, ES_MULTILINE | WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, ES_MULTILINE | WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, ES_MULTILINE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, ES_MULTILINE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 1, 1, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 1, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(0, ES_MULTILINE | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_BORDER, ES_MULTILINE | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    check_pos(hwEdit, Metrics.tmHeight -  1, 0, Metrics.tmHeight - 1, 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight     , 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  1, 0, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  2, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight +  4, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    check_pos(hwEdit, Metrics.tmHeight + 10, 1, Metrics.tmHeight    , 1, 0, 0, 0);
    DestroyWindow(hwEdit);
}

/* WM_SETTEXT is implemented by selecting all text, and then replacing the
 * selection.  This test checks that the first 'select all' doesn't generate
 * an UPDATE message which can escape and (via a handler) change the
 * selection, which would cause WM_SETTEXT to break.  This old bug
 * was fixed 18-Mar-2005; we check here to ensure it doesn't regress.
 */
static void test_edit_control_2(void)
{
    HWND hwndMain;
    char szLocalString[MAXLEN];

    /* Create main and edit windows. */
    hwndMain = CreateWindow(szEditTest2Name, "ET2", WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    assert(hwndMain);
    if (winetest_interactive)
        ShowWindow (hwndMain, SW_SHOW);

    hwndET2 = CreateWindow("EDIT", NULL,
                           WS_CHILD|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
                           0, 0, 150, 50, /* important this not be 0 size. */
                           hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    assert(hwndET2);
    if (winetest_interactive)
        ShowWindow (hwndET2, SW_SHOW);

    trace("EDIT: SETTEXT atomicity\n");
    /* Send messages to "type" in the word 'foo'. */
    SendMessage(hwndET2, WM_CHAR, 'f', 1);
    SendMessage(hwndET2, WM_CHAR, 'o', 1);
    SendMessage(hwndET2, WM_CHAR, 'o', 1);
    /* 'foo' should have been changed to 'bar' by the UPDATE handler. */
    GetWindowText(hwndET2, szLocalString, MAXLEN);
    ok(lstrcmp(szLocalString, "bar")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    /* OK, done! */
    DestroyWindow (hwndET2);
    DestroyWindow (hwndMain);
}

static void ET2_check_change(void) {
   char szLocalString[MAXLEN];
   /* This EN_UPDATE handler changes any 'foo' to 'bar'. */
   GetWindowText(hwndET2, szLocalString, MAXLEN);
   if (lstrcmp(szLocalString, "foo")==0) {
       lstrcpy(szLocalString, "bar");
       SendMessage(hwndET2, WM_SETTEXT, 0, (LPARAM) szLocalString);
   }
   /* always leave the cursor at the end. */
   SendMessage(hwndET2, EM_SETSEL, MAXLEN - 1, MAXLEN - 1);
}
static void ET2_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id==ID_EDITTEST2 && codeNotify == EN_UPDATE)
        ET2_check_change();
}
static LRESULT CALLBACK ET2_WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {
        HANDLE_MSG(hwnd, WM_COMMAND, ET2_OnCommand);
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

static BOOL RegisterWindowClasses (void)
{
    WNDCLASSA cls;
    cls.style = 0;
    cls.lpfnWndProc = ET2_WndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = hinst;
    cls.hIcon = NULL;
    cls.hCursor = LoadCursorA (NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = szEditTest2Name;
    if (!RegisterClassA (&cls)) return FALSE;

    return TRUE;
}

static void zero_notify(void)
{
    notifications.en_change = 0;
    notifications.en_maxtext = 0;
    notifications.en_update = 0;
}

#define test_notify(enchange, enmaxtext, enupdate) \
    ok(notifications.en_change == enchange, "expected %d EN_CHANGE notifications, " \
    "got %d\n", enchange, notifications.en_change); \
    ok(notifications.en_maxtext == enmaxtext, "expected %d EN_MAXTEXT notifications, " \
    "got %d\n", enmaxtext, notifications.en_maxtext); \
    ok(notifications.en_update == enupdate, "expected %d EN_UPDATE notifications, " \
    "got %d\n", enupdate, notifications.en_update)


static LRESULT CALLBACK edit3_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case EN_MAXTEXT:
                    notifications.en_maxtext++;
                    break;
                case EN_UPDATE:
                    notifications.en_update++;
                    break;
                case EN_CHANGE:
                    notifications.en_change++;
                    break;
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/* Test behaviour of WM_SETTEXT, WM_REPLACESEL and notificatisons sent in response
 * to these messages.
 */
static void test_edit_control_3(void)
{
    WNDCLASSA cls;
    HWND hWnd;
    HWND hParent;
    int len;
    static const char *str = "this is a long string.";
    static const char *str2 = "this is a long string.\r\nthis is a long string.\r\nthis is a long string.\r\nthis is a long string.";

    trace("EDIT: Test notifications\n");

    cls.style = 0;
    cls.lpfnWndProc = edit3_wnd_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "ParentWindowClass";

    assert(RegisterClassA(&cls));

    hParent = CreateWindowExA(0,
              "ParentWindowClass",
              NULL,
              0,
              CW_USEDEFAULT, CW_USEDEFAULT, 10, 10,
              NULL, NULL, NULL, NULL);
    assert(hParent);

    trace("EDIT: Single line, no ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              0,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) > len, "text should have been truncated\n");
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == len, "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Single line, ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_AUTOHSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, no ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL and ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);
}

/* Test EM_CHARFROMPOS and EM_POSFROMCHAR
 */
static void test_edit_control_4(void)
{
    HWND hwEdit;
    int lo, hi, mid;
    int ret;
    int i;

    trace("EDIT: Test EM_CHARFROMPOS and EM_POSFROMCHAR\n");
    hwEdit = create_editcontrol(0, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_RIGHT, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_CENTER, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_RIGHT, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_CENTER, 0);
    SendMessage(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessage(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessage(hwEdit, EM_CHARFROMPOS, 0, (LPARAM) i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessage(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);
}

static void test_margins(void)
{
    HWND hwEdit;
    RECT old_rect, new_rect;
    INT old_left_margin, old_right_margin;
    DWORD old_margins, new_margins;

    hwEdit = create_editcontrol(WS_BORDER, 0);
    
    old_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    old_left_margin = LOWORD(old_margins);
    old_right_margin = HIWORD(old_margins);
    
    /* Check if setting the margins works */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(10, 0));
    new_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == old_right_margin, "Wrong right margin: %d\n", HIWORD(new_margins));
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_RIGHTMARGIN, MAKELONG(0, 10));
    new_margins = SendMessage(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == 10, "Wrong right margin: %d\n", HIWORD(new_margins));
    
    
    /* The size of the rectangle must decrease if we increase the margin */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(5, 5));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(15, 20));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left + 10, "The left border of the rectangle is wrong\n");
    ok(new_rect.right == old_rect.right - 15, "The right border of the rectangle is wrong\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle must not change\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle must not change\n");
    
    
    /* If we set the margin to same value as the current margin,
       the rectangle must not change */
    
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    old_rect.left = 1;
    old_rect.right = 99;
    old_rect.top = 1;
    old_rect.bottom = 99;
    SendMessage(hwEdit, EM_SETRECT, 0, (LPARAM)&old_rect);    
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessage(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left, "The left border of the rectangle has changed\n");
    ok(new_rect.right == old_rect.right, "The right border of the rectangle has changed\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle has changed\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle has changed\n");
    
    DestroyWindow (hwEdit);
}

START_TEST(edit)
{
    hinst = GetModuleHandleA (NULL);
    if (!RegisterWindowClasses())
        assert(0);

    test_edit_control_1();
    test_edit_control_2();
    test_edit_control_3();
    test_edit_control_4();
    test_margins();
}
