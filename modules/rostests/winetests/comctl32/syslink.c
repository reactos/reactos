/* Unit tests for the syslink control.
 *
 * Copyright 2011 Francois Gouget for CodeWeavers
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
#include "v6util.h"
#include "msg.h"

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define NUM_MSG_SEQUENCE 2
#define PARENT_SEQ_INDEX 0
#define SYSLINK_SEQ_INDEX 1

static HWND hWndParent;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCE];

static const struct message empty_wnd_seq[] = {
    {0}
};

static const struct message parent_create_syslink_wnd_seq[] = {
    { WM_GETFONT, sent|optional}, /* Only on XP */
    { WM_QUERYUISTATE, sent|optional},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent|wparam, 0},
    { WM_PARENTNOTIFY, sent|wparam, WM_CREATE},
    {0}
};

static const struct message visible_syslink_wnd_seq[] = {
    { WM_STYLECHANGING, sent|wparam, GWL_STYLE},
    { WM_STYLECHANGED, sent|wparam, GWL_STYLE},
    { WM_PAINT, sent},
    {0}
};

static const struct message parent_visible_syslink_wnd_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent|wparam, 0},
    {0}
};

/* Try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
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
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcW(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static const WCHAR parentClassW[] = {'S','y','s','l','i','n','k',' ','t','e','s','t',' ','p','a','r','e','n','t',' ','c','l','a','s','s',0};

static BOOL register_parent_wnd_class(void)
{
    WNDCLASSW cls;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = parentClassW;
    return RegisterClassW(&cls);
}

static HWND create_parent_window(void)
{
    static const WCHAR titleW[] = {'S','y','s','l','i','n','k',' ','t','e','s','t',' ','p','a','r','e','n','t',' ','w','i','n','d','o','w',0};
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowExW(0, parentClassW, titleW,
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                           WS_MAXIMIZEBOX | WS_VISIBLE,
                           0, 0, 200, 100, GetDesktopWindow(),
                           NULL, GetModuleHandleW(NULL), NULL);
}

static WNDPROC syslink_oldproc;

static LRESULT WINAPI syslink_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message msg = { 0 };
    LRESULT ret;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, SYSLINK_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcW(syslink_oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_syslink(DWORD style, HWND parent)
{
    HWND hWndSysLink;
    static const WCHAR linkW[] = {'H','e','a','d',' ','<','a',' ','h','r','e','f','=','"','l','i','n','k','1','"','>','N','a','m','e','1','<','/','a','>',' ','M','i','d','d','l','e',' ','<','a',' ','h','r','e','f','=','"','l','i','n','k','2','"','>','N','a','m','e','2','<','/','a','>',' ','T','a','i','l',0};

    /* Only Unicode will do here */
    hWndSysLink = CreateWindowExW(0, WC_LINK, linkW,
                                style, 0, 0, 150, 50,
                                parent, NULL, GetModuleHandleW(NULL), NULL);
    if (!hWndSysLink) return NULL;

    if (GetWindowLongPtrW(hWndSysLink, GWLP_USERDATA))
        /* On Windows XP SysLink takes GWLP_USERDATA for itself! */
        trace("SysLink makes use of GWLP_USERDATA\n");

    syslink_oldproc = (WNDPROC)SetWindowLongPtrW(hWndSysLink, GWLP_WNDPROC, (LONG_PTR)syslink_subclass_proc);

    return hWndSysLink;
}

static void test_create_syslink(void)
{
    HWND hWndSysLink;
    LONG oldstyle;

    /* Create an invisible SysLink control */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    hWndSysLink = create_syslink(WS_CHILD | WS_TABSTOP, hWndParent);
    ok(hWndSysLink != NULL, "Expected non NULL value (le %u)\n", GetLastError());
    flush_events();
    ok_sequence(sequences, SYSLINK_SEQ_INDEX, empty_wnd_seq, "create SysLink", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_create_syslink_wnd_seq, "create SysLink (parent)", TRUE);

    /* Make the SysLink control visible */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    oldstyle = GetWindowLongA(hWndSysLink, GWL_STYLE);
    SetWindowLongA(hWndSysLink, GWL_STYLE, oldstyle | WS_VISIBLE);
    RedrawWindow(hWndSysLink, NULL, NULL, RDW_INVALIDATE);
    flush_events();
    ok_sequence(sequences, SYSLINK_SEQ_INDEX, visible_syslink_wnd_seq, "visible SysLink", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_visible_syslink_wnd_seq, "visible SysLink (parent)", TRUE);

    DestroyWindow(hWndSysLink);
}

static void test_LM_GETIDEALHEIGHT(void)
{
    HWND hwnd;
    LONG ret;

    hwnd = create_syslink(WS_CHILD | WS_TABSTOP | WS_VISIBLE, hWndParent);
    ok(hwnd != NULL, "Failed to create SysLink window.\n");

    ret = SendMessageA(hwnd, LM_GETIDEALHEIGHT, 0, 0);
    ok(ret > 0, "Unexpected ideal height, %d.\n", ret);

    DestroyWindow(hwnd);
}

static void test_LM_GETIDEALSIZE(void)
{
    HWND hwnd;
    LONG ret;
    SIZE sz;

    hwnd = create_syslink(WS_CHILD | WS_TABSTOP | WS_VISIBLE, hWndParent);
    ok(hwnd != NULL, "Failed to create SysLink window.\n");

    memset(&sz, 0, sizeof(sz));
    ret = SendMessageA(hwnd, LM_GETIDEALSIZE, 0, (LPARAM)&sz);
    ok(ret > 0, "Unexpected return value, %d.\n", ret);
    if (sz.cy == 0)
        win_skip("LM_GETIDEALSIZE is not supported.\n");
    else
    {
        ok(sz.cx > 5, "Unexpected ideal width, %d.\n", sz.cx);
        ok(sz.cy == ret, "Unexpected ideal height, %d.\n", sz.cy);
    }

    DestroyWindow(hwnd);
}

START_TEST(syslink)
{
    ULONG_PTR ctx_cookie;
    HMODULE hComctl32;
    POINT orig_pos;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    /* LoadLibrary is needed. This file has no reference to functions in comctl32 */
    hComctl32 = LoadLibraryA("comctl32.dll");
    ok(hComctl32 != NULL, "Failed to load comctl32.dll.\n");

    /* Move the cursor off the parent window */
    GetCursorPos(&orig_pos);
    SetCursorPos(400, 400);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCE);

    /* Create parent window */
    hWndParent = create_parent_window();
    ok(hWndParent != NULL, "Failed to create parent Window!\n");
    flush_events();

    test_create_syslink();
    test_LM_GETIDEALHEIGHT();
    test_LM_GETIDEALSIZE();

    DestroyWindow(hWndParent);
    unload_v6_module(ctx_cookie, hCtx);
    SetCursorPos(orig_pos.x, orig_pos.y);
}
