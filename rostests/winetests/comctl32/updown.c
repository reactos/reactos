/* Unit tests for the up-down control
 *
 * Copyright 2005 C. Scott Ananian
 * Copyright (C) 2007 James Hawkins
 * Copyright (C) 2007 Leslie Choong
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

/* TO TEST:
 *   - send click messages to the up-down control, check the current position
 *   - up-down control automatically positions itself next to its buddy window
 *   - up-down control sets the caption of the buddy window
 *   - test CreateUpDownControl API
 *   - check UDS_AUTOBUDDY style, up-down control selects previous window in z-order
 *   - check UDM_SETBUDDY message
 *   - check UDM_GETBUDDY message
 *   - up-down control and buddy control must have the same parent
 *   - up-down control notifies its parent window when its position changes with UDN_DELTAPOS + WM_VSCROLL or WM_HSCROLL
 *   - check UDS_ALIGN[LEFT,RIGHT]...check that width of buddy window is decreased
 *   - check that UDS_SETBUDDYINT sets the caption of the buddy window when it is changed
 *   - check that the thousands operator is set for large numbers
 *   - check that the thousands operator is not set with UDS_NOTHOUSANDS
 *   - check UDS_ARROWKEYS, control subclasses the buddy window so that it processes the keys when it has focus
 *   - check UDS_HORZ
 *   - check changing past min/max values
 *   - check UDS_WRAP wraps values past min/max, incrementing past upper value wraps position to lower value
 *   - can change control's position, min/max pos, radix
 *   - check UDM_GETPOS, for up-down control with a buddy window, position is the caption of the buddy window, so change the
 *     caption of the buddy window then call UDM_GETPOS
 *   - check UDM_SETRANGE, max can be less than min, so clicking the up arrow decreases the current position
 *   - more stuff to test
 */

#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"
#include "msg.h"

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))

#define NUM_MSG_SEQUENCES   3
#define PARENT_SEQ_INDEX    0
#define EDIT_SEQ_INDEX      1
#define UPDOWN_SEQ_INDEX    2

static HWND parent_wnd, edit, updown;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static const struct message create_parent_wnd_seq[] = {
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_QUERYNEWPALETTE,   sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
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

static const struct message add_edit_to_parent_seq[] = {
    { WM_PARENTNOTIFY, sent|wparam, WM_CREATE },
    { 0 }
};

static const struct message add_updown_with_edit_seq[] = {
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_RESTORED /*, MAKELONG(91, 75) exact size depends on font */ },
    { 0 }
};

static const struct message add_updown_to_parent_seq[] = {
    { WM_NOTIFYFORMAT, sent|lparam, 0, NF_QUERY },
    { WM_QUERYUISTATE, sent|optional },
    { WM_PARENTNOTIFY, sent|wparam, MAKELONG(WM_CREATE, WM_CREATE) },
    { 0 }
};

static const struct message get_edit_text_seq[] = {
    { WM_GETTEXT, sent },
    { 0 }
};

static const struct message test_updown_pos_seq[] = {
    { UDM_SETRANGE, sent|lparam, 0, MAKELONG(100,0) },
    { UDM_GETRANGE, sent},
    { UDM_SETPOS, sent|lparam, 0, 5},
    { UDM_GETPOS, sent},
    { UDM_SETPOS, sent|lparam, 0, 0},
    { UDM_GETPOS, sent},
    { UDM_SETPOS, sent|lparam, 0, MAKELONG(-1,0)},
    { UDM_GETPOS, sent},
    { UDM_SETPOS, sent|lparam, 0, 100},
    { UDM_GETPOS, sent},
    { UDM_SETPOS, sent|lparam, 0, 101},
    { UDM_GETPOS, sent},
    { 0 }
};

static const struct message test_updown_pos32_seq[] = {
    { UDM_SETRANGE32, sent|lparam, 0, 1000 },
    { UDM_GETRANGE32, sent}, /* Cannot check wparam and lparam as they are ptrs */
    { UDM_SETPOS32, sent|lparam, 0, 500 },
    { UDM_GETPOS32, sent},
    { UDM_SETPOS32, sent|lparam, 0, 0 },
    { UDM_GETPOS32, sent},
    { UDM_SETPOS32, sent|lparam, 0, -1 },
    { UDM_GETPOS32, sent},
    { UDM_SETPOS32, sent|lparam, 0, 1000 },
    { UDM_GETPOS32, sent},
    { UDM_SETPOS32, sent|lparam, 0, 1001 },
    { UDM_GETPOS32, sent},
    { 0 }
};

static const struct message test_updown_buddy_seq[] = {
    { UDM_GETBUDDY, sent },
    { UDM_SETBUDDY, sent },
    { WM_STYLECHANGING, sent|defwinproc },
    { WM_STYLECHANGED, sent|defwinproc },
    { WM_STYLECHANGING, sent|defwinproc },
    { WM_STYLECHANGED, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|optional|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_MOVE, sent|defwinproc },
    { UDM_GETBUDDY, sent },
    { 0 }
};

static const struct message test_updown_base_seq[] = {
    { UDM_SETBASE, sent|wparam, 10 },
    { UDM_GETBASE, sent },
    { UDM_SETBASE, sent|wparam, 80 },
    { UDM_GETBASE, sent },
    { UDM_SETBASE, sent|wparam, 16 },
    { UDM_GETBASE, sent },
    { UDM_SETBASE, sent|wparam, 80 },
    { UDM_GETBASE, sent },
    { UDM_SETBASE, sent|wparam, 10 },
    { UDM_GETBASE, sent },
    { 0 }
};

static const struct message test_updown_unicode_seq[] = {
    { UDM_SETUNICODEFORMAT, sent|wparam, 0 },
    { UDM_GETUNICODEFORMAT, sent },
    { UDM_SETUNICODEFORMAT, sent|wparam, 1 },
    { UDM_GETUNICODEFORMAT, sent },
    { UDM_SETUNICODEFORMAT, sent|wparam, 0 },
    { UDM_GETUNICODEFORMAT, sent },
    { 0 }
};

static const struct message test_updown_destroy_seq[] = {
    { WM_SHOWWINDOW, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent},
    { WM_WINDOWPOSCHANGED, sent},
    { WM_DESTROY, sent},
    { WM_NCDESTROY, sent},
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
    cls.lpszClassName = "Up-Down test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowEx(0, "Up-Down test parent class",
                          "Up-Down test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 100, 100,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

struct subclass_info
{
    WNDPROC oldproc;
};

static LRESULT WINAPI edit_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("edit: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, EDIT_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;
    return ret;
}

static HWND create_edit_control(void)
{
    struct subclass_info *info;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(parent_wnd, &rect);
    edit = CreateWindowExA(0, "EDIT", NULL, WS_CHILD | WS_BORDER | WS_VISIBLE,
                           0, 0, rect.right, rect.bottom,
                           parent_wnd, NULL, GetModuleHandleA(NULL), NULL);
    if (!edit)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongPtrA(edit, GWLP_WNDPROC,
                                            (LONG_PTR)edit_subclass_proc);
    SetWindowLongPtrA(edit, GWLP_USERDATA, (LONG_PTR)info);

    return edit;
}

static LRESULT WINAPI updown_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct subclass_info *info = (struct subclass_info *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    trace("updown: %p, %04x, %08lx, %08lx\n", hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, UPDOWN_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(info->oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_updown_control(void)
{
    struct subclass_info *info;
    HWND updown;
    RECT rect;

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(struct subclass_info));
    if (!info)
        return NULL;

    GetClientRect(parent_wnd, &rect);
    updown = CreateUpDownControl(WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_ALIGNRIGHT,
                                 0, 0, rect.right, rect.bottom, parent_wnd, 1, GetModuleHandleA(NULL), edit,
                                 100, 0, 50);
    if (!updown)
    {
        HeapFree(GetProcessHeap(), 0, info);
        return NULL;
    }

    info->oldproc = (WNDPROC)SetWindowLongPtrA(updown, GWLP_WNDPROC,
                                            (LONG_PTR)updown_subclass_proc);
    SetWindowLongPtrA(updown, GWLP_USERDATA, (LONG_PTR)info);

    return updown;
}

static void test_updown_pos(void)
{
    int r;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Set Range from 0 to 100 */
    SendMessage(updown, UDM_SETRANGE, 0 , MAKELONG(100,0) );
    r = SendMessage(updown, UDM_GETRANGE, 0,0);
    expect(100,LOWORD(r));
    expect(0,HIWORD(r));

    /* Set the position to 5, return is not checked as it was set before func call */
    SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(5,0) );
    /* Since UDM_SETBUDDYINT was not set at creation HIWORD(r) will always be 1 as a return from UDM_GETPOS */
    /* Get the position, which should be 5 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    expect(5,LOWORD(r));
    expect(1,HIWORD(r));

    /* Set the position to 0, return should be 5 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(0,0) );
    expect(5,r);
    /* Get the position, which should be 0 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    expect(0,LOWORD(r));
    expect(1,HIWORD(r));

    /* Set the position to -1, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(-1,0) );
    expect(0,r);
    /* Get the position, which should be 0 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    expect(0,LOWORD(r));
    expect(1,HIWORD(r));

    /* Set the position to 100, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(100,0) );
    expect(0,r);
    /* Get the position, which should be 100 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    expect(100,LOWORD(r));
    expect(1,HIWORD(r));

    /* Set the position to 101, return should be 100 */
    r = SendMessage(updown, UDM_SETPOS, 0 , MAKELONG(101,0) );
    expect(100,r);
    /* Get the position, which should be 100 */
    r = SendMessage(updown, UDM_GETPOS, 0 , 0 );
    expect(100,LOWORD(r));
    expect(1,HIWORD(r));

    ok_sequence(sequences, UPDOWN_SEQ_INDEX, test_updown_pos_seq , "test updown pos", FALSE);
}

static void test_updown_pos32(void)
{
    int r;
    int low, high;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Set the position to 0 to 1000 */
    SendMessage(updown, UDM_SETRANGE32, 0 , 1000 );

    r = SendMessage(updown, UDM_GETRANGE32, (WPARAM) &low , (LPARAM) &high );
    expect(0,low);
    expect(1000,high);

    /* Set position to 500, don't check return since it is unset*/
    SendMessage(updown, UDM_SETPOS32, 0 , 500 );

    /* Since UDM_SETBUDDYINT was not set at creation bRet will always be true as a return from UDM_GETPOS32 */

    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    expect(500,r);
    expect(1,high);

    /* Set position to 0, return should be 500 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 0 );
    expect(500,r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    expect(0,r);
    expect(1,high);

    /* Set position to -1 which should become 0, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , -1 );
    expect(0,r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    expect(0,r);
    expect(1,high);

    /* Set position to 1000, return should be 0 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 1000 );
    expect(0,r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    expect(1000,r);
    expect(1,high);

    /* Set position to 1001 which should become 1000, return should be 1000 */
    r = SendMessage(updown, UDM_SETPOS32, 0 , 1001 );
    expect(1000,r);
    r = SendMessage(updown, UDM_GETPOS32, 0 , (LPARAM) &high );
    expect(1000,r);
    expect(1,high);

    ok_sequence(sequences, UPDOWN_SEQ_INDEX, test_updown_pos32_seq, "test updown pos32", FALSE);
}

static void test_updown_buddy(void)
{
    HWND buddyReturn;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    buddyReturn = (HWND)SendMessage(updown, UDM_GETBUDDY, 0 , 0 );
    ok(buddyReturn == edit, "Expected edit handle\n");

    buddyReturn = (HWND)SendMessage(updown, UDM_SETBUDDY, (WPARAM) edit, 0);
    ok(buddyReturn == edit, "Expected edit handle\n");

    buddyReturn = (HWND)SendMessage(updown, UDM_GETBUDDY, 0 , 0 );
    ok(buddyReturn == edit, "Expected edit handle\n");

    ok_sequence(sequences, UPDOWN_SEQ_INDEX, test_updown_buddy_seq, "test updown buddy", TRUE);
    ok_sequence(sequences, EDIT_SEQ_INDEX, add_updown_with_edit_seq, "test updown buddy_edit", FALSE);
}

static void test_updown_base(void)
{
    int r;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessage(updown, UDM_SETBASE, 10 , 0);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    expect(10,r);

    /* Set base to an invalid value, should return 0 and stay at 10 */
    r = SendMessage(updown, UDM_SETBASE, 80 , 0);
    expect(0,r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    expect(10,r);

    /* Set base to 16 now, should get 16 as the return */
    r = SendMessage(updown, UDM_SETBASE, 16 , 0);
    expect(10,r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    expect(16,r);

    /* Set base to an invalid value, should return 0 and stay at 16 */
    r = SendMessage(updown, UDM_SETBASE, 80 , 0);
    expect(0,r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    expect(16,r);

    /* Set base back to 10, return should be 16 */
    r = SendMessage(updown, UDM_SETBASE, 10 , 0);
    expect(16,r);
    r = SendMessage(updown, UDM_GETBASE, 0 , 0);
    expect(10,r);

    ok_sequence(sequences, UPDOWN_SEQ_INDEX, test_updown_base_seq, "test updown base", FALSE);
}

static void test_updown_unicode(void)
{
    int r;

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Set it to ANSI, don't check return as we don't know previous state */
    SendMessage(updown, UDM_SETUNICODEFORMAT, 0 , 0);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    expect(0,r);

    /* Now set it to Unicode format */
    r = SendMessage(updown, UDM_SETUNICODEFORMAT, 1 , 0);
    expect(0,r);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    expect(1,r);

    /* And now set it back to ANSI */
    r = SendMessage(updown, UDM_SETUNICODEFORMAT, 0 , 0);
    expect(1,r);
    r = SendMessage(updown, UDM_GETUNICODEFORMAT, 0 , 0);
    expect(0,r);

    ok_sequence(sequences, UPDOWN_SEQ_INDEX, test_updown_unicode_seq, "test updown unicode", FALSE);
}


static void test_create_updown_control(void)
{
    CHAR text[MAX_PATH];

    parent_wnd = create_parent_window();
    ok(parent_wnd != NULL, "Failed to create parent window!\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, create_parent_wnd_seq, "create parent window", TRUE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    edit = create_edit_control();
    ok(edit != NULL, "Failed to create edit control\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_edit_to_parent_seq, "add edit control to parent", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    updown = create_updown_control();
    ok(updown != NULL, "Failed to create updown control\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, add_updown_to_parent_seq, "add updown control to parent", TRUE);
    ok_sequence(sequences, EDIT_SEQ_INDEX, add_updown_with_edit_seq, "add updown control with edit", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    GetWindowTextA(edit, text, MAX_PATH);
    ok(lstrlenA(text) == 0, "Expected empty string\n");
    ok_sequence(sequences, EDIT_SEQ_INDEX, get_edit_text_seq, "get edit text", FALSE);

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    test_updown_pos();
    test_updown_pos32();
    test_updown_buddy();
    test_updown_base();
    test_updown_unicode();
}

START_TEST(updown)
{
    InitCommonControls();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_create_updown_control();
}
