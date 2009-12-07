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
#include <assert.h>

#include "wine/test.h"

#define expect(expected, got) ok(expected == got, "expected %d, got %d\n", expected,got)

static HWND create_ipaddress_control (void)
{
    HWND handle;

    handle = CreateWindowEx(0, WC_IPADDRESS, NULL,
			    WS_BORDER|WS_VISIBLE, 0, 0, 0, 0,
			    NULL, NULL, NULL, NULL);
    assert(handle);

    return handle;
}

static void test_get_set_text(void)
{
    HWND hwnd;
    CHAR ip[16];
    INT r;

    hwnd = create_ipaddress_control();

    /* check text just after creation */
    r = GetWindowText(hwnd, ip, sizeof(ip)/sizeof(CHAR));
    expect(7, r);
    ok(strcmp(ip, "0.0.0.0") == 0, "Expected null IP address, got %s\n", ip);

    SendMessage(hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
    r = GetWindowText(hwnd, ip, sizeof(ip)/sizeof(CHAR));
    expect(9, r);
    ok(strcmp(ip, "127.0.0.1") == 0, "Expected 127.0.0.1, got %s\n", ip);

    DestroyWindow(hwnd);
}

static int init(void)
{
    HMODULE hComctl32;
    BOOL (WINAPI *pInitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
    INITCOMMONCONTROLSEX iccex;

    hComctl32 = GetModuleHandleA("comctl32.dll");
    pInitCommonControlsEx = (void*)GetProcAddress(hComctl32, "InitCommonControlsEx");
    if (!pInitCommonControlsEx)
    {
        win_skip("InitCommonControlsEx() is missing.\n");
        return 0;
    }

    iccex.dwSize = sizeof(iccex);
    /* W2K and below need ICC_INTERNET_CLASSES for the IP Address Control */
    iccex.dwICC  = ICC_INTERNET_CLASSES;
    pInitCommonControlsEx(&iccex);

    return 1;
}

START_TEST(ipaddress)
{
    if (!init())
        return;

    test_get_set_text();
}
