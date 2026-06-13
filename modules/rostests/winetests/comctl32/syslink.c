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

#define COBJMACROS
#include <windows.h>
#include <commctrl.h>

#include <initguid.h>
#include <oleacc.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define NUM_MSG_SEQUENCE 2
#define PARENT_SEQ_INDEX 0
#define SYSLINK_SEQ_INDEX 1

static HWND hWndParent;
static int g_link_id;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCE];

static void CALLBACK msg_winevent_proc(HWINEVENTHOOK hevent,
                                       DWORD event,
                                       HWND hwnd,
                                       LONG object_id,
                                       LONG child_id,
                                       DWORD thread_id,
                                       DWORD event_time)
{
    struct message msg = {0};
    WCHAR class_name[256];

    /* ignore events not from a syslink control */
    if (!GetClassNameW(hwnd, class_name, ARRAY_SIZE(class_name)) ||
        wcscmp(class_name, WC_LINK) != 0)
        return;

    msg.message = event;
    msg.flags = winevent_hook|wparam|lparam;
    msg.wParam = object_id;
    msg.lParam = child_id;
    add_message(sequences, SYSLINK_SEQ_INDEX, &msg);
}

static void init_winevent_hook(void) {
    hwineventhook = SetWinEventHook(EVENT_MIN, EVENT_MAX, GetModuleHandleA(0), msg_winevent_proc,
                                    0, GetCurrentThreadId(), WINEVENT_INCONTEXT);
    if (!hwineventhook)
        win_skip( "no win event hook support\n" );
}

static void uninit_winevent_hook(void) {
    if (!hwineventhook)
        return;

    UnhookWinEvent(hwineventhook);
    hwineventhook = 0;
}

static const struct message create_syslink_wnd_seq[] = {
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_WINDOW, CHILDID_SELF },
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

static const struct message settext_syslink_wnd_seq[] = {
    { WM_SETTEXT, sent },
    { EVENT_OBJECT_NAMECHANGE, winevent_hook|wparam|lparam, OBJID_WINDOW, CHILDID_SELF },
    { WM_PAINT, sent },
    { WM_ERASEBKGND, sent|defwinproc|optional }, /* Wine only */
    {0}
};

static const struct message parent_settext_syslink_wnd_seq[] = {
    { WM_CTLCOLORSTATIC, sent },
    { WM_NOTIFY, sent|wparam|lparam|optional, 0, NM_CUSTOMDRAW }, /* FIXME: Not sent on Wine */
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
        if (message == WM_NOTIFY && lParam)
            msg.lParam = ((NMHDR*)lParam)->code;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    switch(message)
    {
       case WM_NOTIFY:
       {
           NMLINK *nml = ((NMLINK *)lParam);
           if (nml && NM_CLICK == nml->hdr.code)
           {
               g_link_id = nml->item.iLink;
           }
           break;
       }
       default:
            break;
    }
    defwndproc_counter++;
    ret = DefWindowProcW(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

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
    cls.lpszClassName = L"Syslink test parent class";
    return RegisterClassW(&cls);
}

static HWND create_parent_window(void)
{
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowExW(0, L"Syslink test parent class", L"Syslink test parent window",
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

    /* Only Unicode will do here */
    hWndSysLink = CreateWindowExW(0, WC_LINK, L"Head <a href=\"link1\">Name1</a> Middle <a href=\"link2\">Name2</a> Tail",
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
    LRESULT ret;
    LITEM item;

    /* Create an invisible SysLink control */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    hWndSysLink = create_syslink(WS_CHILD | WS_TABSTOP, hWndParent);
    ok(hWndSysLink != NULL, "Expected non NULL value (le %lu)\n", GetLastError());
    flush_events();
    ok_sequence(sequences, SYSLINK_SEQ_INDEX, create_syslink_wnd_seq, "create SysLink", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_create_syslink_wnd_seq, "create SysLink (parent)", TRUE);

    /* Get first item */
    item.mask = LIF_ITEMINDEX|LIF_ITEMID|LIF_URL;
    item.iLink = 0;
    ret = SendMessageW(hWndSysLink, LM_GETITEM, 0, (LPARAM)&item);
    ok(ret == 1, "LM_GETITEM failed\n");
    ok(!wcscmp(item.szUrl, L"link1"), "unexpected url %s\n", debugstr_w(item.szUrl));

    /* Make the SysLink control visible */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    oldstyle = GetWindowLongA(hWndSysLink, GWL_STYLE);
    SetWindowLongA(hWndSysLink, GWL_STYLE, oldstyle | WS_VISIBLE);
    RedrawWindow(hWndSysLink, NULL, NULL, RDW_INVALIDATE);
    flush_events();
    ok_sequence(sequences, SYSLINK_SEQ_INDEX, visible_syslink_wnd_seq, "visible SysLink", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_visible_syslink_wnd_seq, "visible SysLink (parent)", TRUE);

    /* Change contents */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    SetWindowTextW(hWndSysLink, L"Head <a href=\"link\">link</a> Tail");
    flush_events();
    ok_sequence(sequences, SYSLINK_SEQ_INDEX, settext_syslink_wnd_seq, "SetWindowText", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_settext_syslink_wnd_seq, "SetWindowText (parent)", FALSE);

    /* Get first item */
    item.mask = LIF_ITEMINDEX|LIF_ITEMID|LIF_URL;
    item.iLink = 0;
    ret = SendMessageW(hWndSysLink, LM_GETITEM, 0, (LPARAM)&item);
    ok(ret == 1, "LM_GETITEM failed\n");
    ok(!wcscmp(item.szUrl, L"link"), "unexpected url %s\n", debugstr_w(item.szUrl));

    DestroyWindow(hWndSysLink);
}

static void test_LM_GETIDEALHEIGHT(void)
{
    HWND hwnd;
    LONG ret;

    hwnd = create_syslink(WS_CHILD | WS_TABSTOP | WS_VISIBLE, hWndParent);
    ok(hwnd != NULL, "Failed to create SysLink window.\n");

    ret = SendMessageA(hwnd, LM_GETIDEALHEIGHT, 0, 0);
    ok(ret > 0, "Unexpected ideal height, %ld.\n", ret);

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
    ok(ret > 0, "Unexpected return value, %ld.\n", ret);
    if (sz.cy == 0)
        win_skip("LM_GETIDEALSIZE is not supported.\n");
    else
    {
        ok(sz.cx > 5, "Unexpected ideal width, %ld.\n", sz.cx);
        ok(sz.cy == ret, "Unexpected ideal height, %ld.\n", sz.cy);
    }

    DestroyWindow(hwnd);
}

static void test_link_id(void)
{
    HWND hwnd;

    hwnd = create_syslink(WS_CHILD | WS_TABSTOP | WS_VISIBLE, hWndParent);
    ok(hwnd != NULL, "Failed to create SysLink window.\n");

    /* test link1 at (50, 10) */
    g_link_id = 0;
    SendMessageA(hwnd, WM_LBUTTONDOWN, 1, MAKELPARAM(50, 10));
    SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(50, 10));
    ok(g_link_id == 0, "Got unexpected link id %d.\n", g_link_id);

    /* test link2 at (25, 25) */
    g_link_id = 0;
    SendMessageA(hwnd, WM_LBUTTONDOWN, 1, MAKELPARAM(25, 25));
    SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(25, 25));
    ok(g_link_id == 1, "Got unexpected link id %d.\n", g_link_id);

    DestroyWindow(hwnd);
}

static void wait_link_click(DWORD timeout)
{
    DWORD start_time = GetTickCount();
    DWORD time_waited;

    if (g_link_id == -1)
        flush_events();

    while (g_link_id == -1 && (time_waited = GetTickCount() - start_time) < timeout)
    {
        MsgWaitForMultipleObjects(0, NULL, FALSE, timeout - time_waited, QS_ALLEVENTS);
        flush_events();
    }
}

static void test_msaa(void)
{
    HWND hwnd, ret_hwnd;
    HRESULT hr;
    LRESULT lr;
    IAccessible *acc;
    VARIANT varChild, varResult;
    BSTR name;
    LONG left, top, width, height, hwnd_left, hwnd_top, count=0;
    IDispatch *child;
    IOleWindow *ole_window;

    hwnd = create_syslink(WS_CHILD | WS_TABSTOP | WS_VISIBLE, hWndParent);
    ok(hwnd != NULL, "Failed to create SysLink window.\n");

    lr = SendMessageA(hwnd, WM_GETOBJECT, 0, OBJID_CLIENT);
    ok(lr != 0, "No IAccessible object\n");
    if (lr == 0)
    {
        DestroyWindow(hwnd);
        return;
    }

    hr = ObjectFromLresult(lr, &IID_IAccessible, 0, (void**)&acc);
    ok(hr == S_OK, "ObjectFromLresult failed, hr=%lx", hr);

    VariantInit(&varChild);
    VariantInit(&varResult);

    V_VT(&varChild) = VT_I4;
    V_I4(&varChild) = CHILDID_SELF;

    hr = IAccessible_get_accRole(acc, varChild, &varResult);
    ok(hr == S_OK, "accRole failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accRole returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == ROLE_SYSTEM_CLIENT, "accRole returned %li\n", V_I4(&varResult));

    VariantClear(&varResult);
    hr = IAccessible_get_accState(acc, varChild, &varResult);
    ok(hr == S_OK, "accState failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accState returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == STATE_SYSTEM_FOCUSABLE, "accState returned %li\n", V_I4(&varResult));

    hr = IAccessible_get_accName(acc, varChild, &name);
    ok(hr == S_OK, "accName failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr)) {
        ok(!!name && !wcscmp(name, L"Head Name1 Middle Name2 Tail"),
            "unexpected name %s\n", debugstr_w(name));
        SysFreeString(name);
    }

    hr = IAccessible_get_accDefaultAction(acc, varChild, &name);
    ok(hr == S_FALSE, "accDefaultAction failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
        ok(!name, "unexpected default action %s\n", debugstr_w(name));

    hr = IAccessible_accDoDefaultAction(acc, varChild);
    ok(hr == E_INVALIDARG, "accDoDefaultAction should fail, hr=%lx\n", hr);

    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, varChild);
    ok(hr == S_OK, "accLocation failed, hr=%lx\n", hr);
    hwnd_left = left;
    hwnd_top = top;

    hr = IAccessible_get_accChildCount(acc, &count);
    ok(hr == S_OK, "accChildCount failed, hr=%lx\n", hr);
    ok(count == 2, "accChildCount returned %li\n", count);

    /* child 1 */
    V_I4(&varChild) = 1;
    hr = IAccessible_get_accChild(acc, varChild, &child);
    ok(hr == S_FALSE, "accChild hr=%lx\n", hr);
    ok(!child, "accChild returned IDispatch\n");

    hr = IAccessible_get_accRole(acc, varChild, &varResult);
    ok(hr == S_OK, "accRole failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accRole returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == ROLE_SYSTEM_LINK, "accRole returned %li\n", V_I4(&varResult));

    VariantClear(&varResult);
    hr = IAccessible_get_accState(acc, varChild, &varResult);
    ok(hr == S_OK, "accState failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accState returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == (STATE_SYSTEM_FOCUSABLE|STATE_SYSTEM_LINKED), "accState returned %li\n", V_I4(&varResult));

    hr = IAccessible_get_accName(acc, varChild, &name);
    ok(hr == S_OK, "accName failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr)) {
        ok(!!name && !wcscmp(name, L"Name1"),
            "unexpected name %s\n", debugstr_w(name));
        SysFreeString(name);
    }

    hr = IAccessible_get_accDefaultAction(acc, varChild, &name);
    ok(hr == S_OK, "accDefaultAction failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
        {
            skip("Non-English locale (test with hardcoded 'Click')\n");
        }
        else
        {
            ok(!!name && !wcscmp(name, L"Click"),
                "unexpected name %s\n", debugstr_w(name));
        }
        SysFreeString(name);
    }

    g_link_id = -1;
    hr = IAccessible_accDoDefaultAction(acc, varChild);
    ok(hr == S_OK, "accDoDefaultAction failed, hr=%lx\n", hr);
    wait_link_click(500);
    ok(g_link_id == 0, "Got unexpected link id %d.\n", g_link_id);

    g_link_id = -1;
    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, varChild);
    ok(hr == S_OK, "accLocation failed, hr=%lx\n", hr);
    SendMessageA(hwnd, WM_LBUTTONDOWN, 1, MAKELPARAM(left - hwnd_left + width / 2, top - hwnd_top + height / 2));
    SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(left - hwnd_left + width / 2, top - hwnd_top + height / 2));
    ok(g_link_id == 0, "Got unexpected link id %d.\n", g_link_id);

    /* child 2 */
    V_I4(&varChild) = 2;
    hr = IAccessible_get_accChild(acc, varChild, &child);
    ok(hr == S_FALSE, "accChild hr=%lx\n", hr);
    ok(!child, "accChild returned IDispatch\n");

    hr = IAccessible_get_accRole(acc, varChild, &varResult);
    ok(hr == S_OK, "accRole failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accRole returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == ROLE_SYSTEM_LINK, "accRole returned %li\n", V_I4(&varResult));

    VariantClear(&varResult);
    hr = IAccessible_get_accState(acc, varChild, &varResult);
    ok(hr == S_OK, "accState failed, hr=%lx\n", hr);
    ok(V_VT(&varResult) == VT_I4, "accState returned vt=%x\n", V_VT(&varResult));
    ok(V_I4(&varResult) == (STATE_SYSTEM_FOCUSABLE|STATE_SYSTEM_LINKED), "accState returned %li\n", V_I4(&varResult));

    hr = IAccessible_get_accName(acc, varChild, &name);
    ok(hr == S_OK, "accName failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr)) {
        ok(!!name && !wcscmp(name, L"Name2"),
            "unexpected name %s\n", debugstr_w(name));
        SysFreeString(name);
    }

    hr = IAccessible_get_accDefaultAction(acc, varChild, &name);
    ok(hr == S_OK, "accDefaultAction failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
        {
            skip("Non-English locale (test with hardcoded 'Click')\n");
        }
        else
        {
            ok(!!name && !wcscmp(name, L"Click"),
                "unexpected name %s\n", debugstr_w(name));
        }
        SysFreeString(name);
    }

    g_link_id = -1;
    hr = IAccessible_accDoDefaultAction(acc, varChild);
    ok(hr == S_OK, "accDoDefaultAction failed, hr=%lx\n", hr);
    wait_link_click(500);
    ok(g_link_id == 1, "Got unexpected link id %d.\n", g_link_id);

    g_link_id = -1;
    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, varChild);
    ok(hr == S_OK, "accLocation failed, hr=%lx\n", hr);
    SendMessageA(hwnd, WM_LBUTTONDOWN, 1, MAKELPARAM(left - hwnd_left + width / 2, top - hwnd_top + height / 2));
    SendMessageA(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(left - hwnd_left + width / 2, top - hwnd_top + height / 2));
    ok(g_link_id == 1, "Got unexpected link id %d.\n", g_link_id);

    hr = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void**)&ole_window);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IOleWindow_GetWindow(ole_window, &ret_hwnd);
        ok(hr == S_OK, "GetWindow failed, hr=%lx\n", hr);
        ok(ret_hwnd == hwnd, "GetWindow returned wrong hwnd\n");

        IOleWindow_Release(ole_window);
    }

    IAccessible_Release(acc);

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

    init_winevent_hook();

    /* Create parent window */
    hWndParent = create_parent_window();
    ok(hWndParent != NULL, "Failed to create parent Window!\n");
    flush_events();

    test_create_syslink();
    test_LM_GETIDEALHEIGHT();
    test_LM_GETIDEALSIZE();
    test_link_id();
    test_msaa();

    uninit_winevent_hook();

    DestroyWindow(hWndParent);
    unload_v6_module(ctx_cookie, hCtx);
    SetCursorPos(orig_pos.x, orig_pos.y);
}
