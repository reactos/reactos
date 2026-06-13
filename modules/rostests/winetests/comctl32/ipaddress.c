/* Unit test suite for IP Address control.
 *
 * Copyright 2009 Nikolay Sivov
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

#define expect(expected, got) ok(expected == got, "expected %d, got %d\n", expected,got)

static HWND create_ipaddress_control (void)
{
    HWND handle;

    handle = CreateWindowExA(0, WC_IPADDRESSA, NULL,
			     WS_BORDER|WS_VISIBLE, 0, 0, 0, 0,
			     NULL, NULL, NULL, NULL);
    return handle;
}

static void test_get_set_text(void)
{
    HWND hwnd;
    CHAR ip[16];
    INT r;

    hwnd = create_ipaddress_control();
    if (!hwnd)
    {
        win_skip("IPAddress control not implemented\n");
        return;
    }

    /* check text just after creation */
    r = GetWindowTextA(hwnd, ip, ARRAY_SIZE(ip));
    expect(7, r);
    ok(strcmp(ip, "0.0.0.0") == 0, "Expected null IP address, got %s\n", ip);

    SendMessageA(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
    r = GetWindowTextA(hwnd, ip, ARRAY_SIZE(ip));
    expect(9, r);
    ok(strcmp(ip, "127.0.0.1") == 0, "Expected 127.0.0.1, got %s\n", ip);

    DestroyWindow(hwnd);
}

struct child_enum
{
    HWND fields[4];
    unsigned int count;
};

static BOOL CALLBACK test_child_enum_proc(HWND hwnd, LPARAM param)
{
    struct child_enum *child_enum = (struct child_enum *)param;
    char buff[16] = { 0 };
    unsigned int index;

    GetWindowTextA(hwnd, buff, ARRAY_SIZE(buff));
    index = atoi(buff);
    ok(index < 4, "Unexpected index.\n");
    if (index < 4)
        child_enum->fields[index] = hwnd;

    return ++child_enum->count < 4;
}

static void test_IPM_SETFOCUS(void)
{
    struct child_enum child_enum = {{ 0 }};
    unsigned int ret, from, to, i;
    HWND hwnd;

    hwnd = create_ipaddress_control();
    ok(!!hwnd, "Failed to create control.\n");

    ret = SendMessageA(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(0, 1, 2, 3));
    ok(ret, "Unexpected return value %u.\n", ret);

    EnumChildWindows(hwnd, test_child_enum_proc, (LPARAM)&child_enum);
    ok(child_enum.count == 4, "Unexpected child count %u.\n", child_enum.count);

    for (i = 0; i < 4; ++i)
        SendMessageA(child_enum.fields[i], EM_SETSEL, -1, 0);

    SendMessageA(child_enum.fields[0], EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    ok(from == 0 && to == 0, "Unexpected selection %u x %u.\n", from, to);

    ret = SendMessageA(hwnd, IPM_SETFOCUS, 0, 0);
    ok(ret, "Unexpected return value %u.\n", ret);

    SendMessageA(child_enum.fields[0], EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    ok(from == 0 && to == 1, "Unexpected selection %u x %u.\n", from, to);

    DestroyWindow(hwnd);
}

static void test_WM_SETFOCUS(void)
{
    struct child_enum child_enum = {{ 0 }};
    unsigned int ret, from, to, i;
    HWND hwnd;

    hwnd = create_ipaddress_control();
    ok(!!hwnd, "Failed to create control.\n");

    ret = SendMessageA(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(0, 1, 2, 3));
    ok(ret, "Unexpected return value %u.\n", ret);

    EnumChildWindows(hwnd, test_child_enum_proc, (LPARAM)&child_enum);
    ok(child_enum.count == 4, "Unexpected child count %u.\n", child_enum.count);

    SetFocus(child_enum.fields[3]);

    for (i = 0; i < 4; ++i)
        SendMessageA(child_enum.fields[i], EM_SETSEL, -1, 0);

    SendMessageA(child_enum.fields[0], EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    ok(from == 0 && to == 0, "Unexpected selection %u x %u.\n", from, to);

    SetFocus(hwnd);

    SendMessageA(child_enum.fields[0], EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    ok(from == 0 && to == 1, "Unexpected selection %u x %u.\n", from, to);

    DestroyWindow(hwnd);
}

static void test_IPM_CLEARADDRESS(void)
{
    struct child_enum child_enum = {{ 0 }};
    char buff[16];
    int i, ret;
    HWND hwnd;

    hwnd = create_ipaddress_control();
    ok(!!hwnd, "Failed to create control.\n");

    ret = SendMessageA(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(0, 1, 2, 3));
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    EnumChildWindows(hwnd, test_child_enum_proc, (LPARAM)&child_enum);
    ok(child_enum.count == 4, "Unexpected child count %u.\n", child_enum.count);

    ret = SendMessageA(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(1, 2, 3, 4));
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    ret = GetWindowTextA(hwnd, buff, ARRAY_SIZE(buff));
    ok(ret == 7, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "1.2.3.4"), "Unexpected address %s.\n", buff);

    ret = SendMessageA(hwnd, IPM_CLEARADDRESS, 0, 0);
    ok(ret, "Unexpected return value %d.\n", ret);

    ret = GetWindowTextA(hwnd, buff, ARRAY_SIZE(buff));
    ok(ret == 7, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "0.0.0.0"), "Unexpected address %s.\n", buff);

    for (i = 0; i < 4; ++i)
    {
        buff[0] = 1;
        ret = GetWindowTextA(child_enum.fields[i], buff, ARRAY_SIZE(buff));
        ok(ret == 0, "Unexpected return value %d.\n", ret);
        ok(!*buff, "Unexpected field text %s.\n", buff);
    }

    DestroyWindow(hwnd);
}

START_TEST(ipaddress)
{
    ULONG_PTR cookie;
    HANDLE ctxt;

    test_get_set_text();
    test_IPM_SETFOCUS();
    test_WM_SETFOCUS();
    test_IPM_CLEARADDRESS();

    if (!load_v6_module(&cookie, &ctxt))
        return;

    test_get_set_text();
    test_IPM_SETFOCUS();
    test_WM_SETFOCUS();
    test_IPM_CLEARADDRESS();

    unload_v6_module(cookie, ctxt);
}
