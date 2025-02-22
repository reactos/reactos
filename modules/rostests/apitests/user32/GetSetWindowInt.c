/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetClassInfo
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

START_TEST(GetSetWindowInt)
{
    WNDCLASSEXW wcex = { 0 };
    ATOM atom;
    HWND hwnd;

    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = 0x1;
    wcex.lpfnWndProc = DefWindowProcW;
    wcex.cbClsExtra = 1;
    wcex.cbWndExtra = 5;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.lpszClassName = L"ProTestClass1";

    atom = RegisterClassExW(&wcex);
    ok(atom != 0, "Failed to register class!\n");

    hwnd = CreateWindowW(wcex.lpszClassName,
        L"WindowTitle",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);
    ok(hwnd != 0, "\n");

    SetLastError(0xdeadbeef);
    ok_hex(SetWindowWord(hwnd, 0, 0x1234), 0);
    ok_hex(GetWindowWord(hwnd, 0), 0x1234);
    ok_hex(SetWindowWord(hwnd, 1, 0x2345), 0x12);
    ok_hex(GetWindowWord(hwnd, 1), 0x2345);
    ok_hex(SetWindowWord(hwnd, 2, 0x3456), 0x23);
    ok_hex(GetWindowWord(hwnd, 2), 0x3456);
    ok_hex(SetWindowWord(hwnd, 3, 0x4567), 0x34);
    ok_hex(GetWindowWord(hwnd, 3), 0x4567);
    ok_err(0xdeadbeef);
    ok_hex(SetWindowWord(hwnd, 4, 0x5678), 0);
    ok_err(ERROR_INVALID_INDEX);
    SetLastError(0xdeadbeef);
    ok_hex(GetWindowWord(hwnd, 4), 0);
    ok_err(ERROR_INVALID_INDEX);

    SetLastError(0xdeadbeef);
    ok_hex(SetWindowLong(hwnd, 0, 0x12345678), 0x67564534);
    ok_hex(GetWindowLong(hwnd, 0), 0x12345678);
    ok_hex(SetWindowLong(hwnd, 1, 0x23456789), 0x45123456);
    ok_hex(GetWindowLong(hwnd, 1), 0x23456789);
    ok_err(0xdeadbeef);
    ok_hex(SetWindowLong(hwnd, 2, 0x3456789a), 0);
    ok_err(ERROR_INVALID_INDEX);
    SetLastError(0xdeadbeef);
    ok_hex(GetWindowLong(hwnd, 2), 0);
    ok_err(ERROR_INVALID_INDEX);

#ifdef _WIN64
    SetLastError(0xdeadbeef);
    ok_hex(SetWindowLongPtr(hwnd, 0, 123), 0);
    ok_err(ERROR_INVALID_INDEX);
    SetLastError(0xdeadbeef);
    ok_hex(GetWindowLongPtr(hwnd, 0), 0);
    ok_err(ERROR_INVALID_INDEX);
#endif

}
