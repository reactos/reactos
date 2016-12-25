/*
 * Unit tests for the pager control
 *
 * Copyright 2012 Alexandre Julliard
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

#include <wine/test.h>

//#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>

#include "msg.h"

#define NUM_MSG_SEQUENCES   1
#define PAGER_SEQ_INDEX     0

static HWND parent_wnd;

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message set_child_seq[] = {
    { PGM_SETCHILD, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NOTIFY, sent|id|parent, 0, 0, PGN_CALCSIZE },
    { WM_WINDOWPOSCHANGED, sent },
    { 0 }
};

static const struct message set_pos_seq[] = {
    { PGM_SETPOS, sent },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NOTIFY, sent|id|parent, 0, 0, PGN_CALCSIZE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|optional },
    { WM_SIZE, sent|optional },
    { 0 }
};

static const struct message set_pos_empty_seq[] = {
    { PGM_SETPOS, sent },
    { 0 }
};

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
        msg.message = message;
        msg.flags = sent|wparam|lparam|parent;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        if (message == WM_NOTIFY && lParam) msg.id = ((NMHDR*)lParam)->code;
        add_message(sequences, PAGER_SEQ_INDEX, &msg);
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
    cls.lpszClassName = "Pager test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowA("Pager test parent class", "Pager test parent window",
                        WS_OVERLAPPED | WS_VISIBLE,
                        0, 0, 200, 200, 0, NULL, GetModuleHandleA(NULL), NULL );
}

static LRESULT WINAPI pager_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    struct message msg;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.id     = 0;
    add_message(sequences, PAGER_SEQ_INDEX, &msg);
    return CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
}

static HWND create_pager_control( DWORD style )
{
    WNDPROC oldproc;
    HWND hwnd;
    RECT rect;

    GetClientRect( parent_wnd, &rect );
    hwnd = CreateWindowA( WC_PAGESCROLLERA, "pager", WS_CHILD | WS_BORDER | WS_VISIBLE | style,
                          0, 0, 100, 100, parent_wnd, 0, GetModuleHandleA(0), 0 );
    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)pager_subclass_proc);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);
    return hwnd;
}

static void test_pager(void)
{
    HWND pager, child;
    RECT rect;

    pager = create_pager_control( PGS_HORZ );
    if (!pager)
    {
        win_skip( "Pager control not supported\n" );
        return;
    }
    child = CreateWindowA( "BUTTON", "button", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 300, 300,
                           pager, 0, GetModuleHandleA(0), 0 );

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETCHILD, 0, (LPARAM)child );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_child_seq, "set child", TRUE);
    GetWindowRect( pager, &rect );
    ok( rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top );

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 10 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_seq, "set pos", TRUE);
    GetWindowRect( pager, &rect );
    ok( rect.right - rect.left == 100 && rect.bottom - rect.top == 100,
        "pager resized %dx%d\n", rect.right - rect.left, rect.bottom - rect.top );

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 10 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_empty_seq, "set pos empty", TRUE);

    flush_sequences( sequences, NUM_MSG_SEQUENCES );
    SendMessageA( pager, PGM_SETPOS, 0, 9 );
    ok_sequence(sequences, PAGER_SEQ_INDEX, set_pos_seq, "set pos", TRUE);

    DestroyWindow( pager );
}

START_TEST(pager)
{
    HMODULE mod = GetModuleHandleA("comctl32.dll");

    pSetWindowSubclass = (void*)GetProcAddress(mod, (LPSTR)410);

    InitCommonControls();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    parent_wnd = create_parent_window();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");

    test_pager();
}
