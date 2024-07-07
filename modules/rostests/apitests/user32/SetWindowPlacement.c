/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for Get/SetWindowPlacement
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

START_TEST(SetWindowPlacement)
{
    HWND hwnd;
    WINDOWPLACEMENT wndpl;
    BOOL ret;

    hwnd = CreateWindowW(L"BUTTON", L"Button", WS_POPUPWINDOW, 0, 0, 100, 100,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);

    SetLastError(0xDEADFACE);
    wndpl.length = 0xFFFF;
    ret = GetWindowPlacement(hwnd, &wndpl);
    ok_int(ret, TRUE);
    ok_err(0xDEADFACE);

    SetLastError(0xDEADFACE);
    wndpl.length = sizeof(wndpl);
    ret = GetWindowPlacement(hwnd, &wndpl);
    ok_int(ret, TRUE);
    ok_err(0xDEADFACE);

    SetLastError(0xDEADFACE);
    wndpl.length = 0xFFFF;
    ret = SetWindowPlacement(hwnd, &wndpl);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xDEADFACE);
    wndpl.length = sizeof(wndpl);
    ret = SetWindowPlacement(hwnd, &wndpl);
    ok_int(ret, TRUE);
    ok_err(ERROR_SUCCESS);

    DestroyWindow(hwnd);
}
