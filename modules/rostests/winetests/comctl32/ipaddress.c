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

START_TEST(ipaddress)
{
    ULONG_PTR cookie;
    HANDLE ctxt;

    test_get_set_text();

    if (!load_v6_module(&cookie, &ctxt))
        return;

    test_get_set_text();

    unload_v6_module(cookie, ctxt);
}
