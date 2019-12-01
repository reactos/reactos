/* Unit test suite for rawinput.
 *
 * Copyright 2019 Remi Bernon for CodeWeavers
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

static void test_RegisterRawInputDevices(void)
{
    HWND hwnd;
    RAWINPUTDEVICE raw_devices[1];
    BOOL res;

    raw_devices[0].usUsagePage = 0x01;
    raw_devices[0].usUsage = 0x05;

    hwnd = CreateWindowExA(WS_EX_TOPMOST, "static", "dinput", WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowExA failed\n");


    res = RegisterRawInputDevices(NULL, 0, 0);
    ok(res == FALSE, "RegisterRawInputDevices succeeded\n");


    raw_devices[0].dwFlags = 0;
    raw_devices[0].hwndTarget = 0;

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), 0);
    ok(res == FALSE, "RegisterRawInputDevices succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RegisterRawInputDevices returned %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
    ok(res == TRUE, "RegisterRawInputDevices failed\n");
    ok(GetLastError() == 0xdeadbeef, "RegisterRawInputDevices returned %08x\n", GetLastError());


    /* RIDEV_REMOVE requires hwndTarget == NULL */
    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = hwnd;

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
    ok(res == FALSE, "RegisterRawInputDevices succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RegisterRawInputDevices returned %08x\n", GetLastError());

    raw_devices[0].hwndTarget = 0;

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
    ok(res == TRUE, "RegisterRawInputDevices failed\n");
    ok(GetLastError() == 0xdeadbeef, "RegisterRawInputDevices returned %08x\n", GetLastError());


    /* RIDEV_INPUTSINK requires hwndTarget != NULL */
    raw_devices[0].dwFlags = RIDEV_INPUTSINK;
    raw_devices[0].hwndTarget = 0;

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(res == FALSE, "RegisterRawInputDevices failed\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "RegisterRawInputDevices returned %08x\n", GetLastError());

    raw_devices[0].hwndTarget = hwnd;

    SetLastError(0xdeadbeef);
    res = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
    ok(res == TRUE, "RegisterRawInputDevices succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "RegisterRawInputDevices returned %08x\n", GetLastError());

    DestroyWindow(hwnd);
}

START_TEST(rawinput)
{
    test_RegisterRawInputDevices();
}
