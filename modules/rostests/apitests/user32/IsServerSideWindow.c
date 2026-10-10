/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for IsServerSideWindow
 * COPYRIGHT:   Copyright 2024 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 *              Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "precomp.h"

static const WCHAR WndClass[] = L"window class";

static LRESULT
CALLBACK
Test_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

START_TEST(IsServerSideWindow)
{
    HWND hWnd;
    WNDCLASSEXW wcx;
    BOOL ret;

    ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = Test_WndProc;
    wcx.hInstance = GetModuleHandleW(NULL);
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcx.lpszClassName = WndClass;

    if (!RegisterClassExW(&wcx))
    {
        skip("RegisterClassExW failed with error %lu\n", GetLastError());
        return;
    }

    /* 1. Invalid window */
    hWnd = (HWND)(UINT_PTR)0xdeadbeef;
    SetLastError(0xfeedfab1);
    ret = IsServerSideWindow(hWnd);
    ok(!ret, "The window %p is invalid but IsServerSideWindow() returned TRUE\n", hWnd);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_WINDOW_HANDLE);

    /* 2. Window with a kernel-mode WndProc.
     * ScrollBar is an example of a server-side window that can be created from user-mode code. */
    hWnd = CreateWindowExW(0,
                           L"ScrollBar",
                           NULL,
                           SBS_HORZ | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 100,
                           NULL, 0,
                           wcx.hInstance, NULL);
    if (!hWnd)
    {
        skip("CreateWindowExW failed with error %lu\n", GetLastError());
        goto Quit;
    }

    SetLastError(0xfeedfab1);
    ret = IsServerSideWindow(hWnd);
    ok(ret, "The window %p is invalid or doesn't have a valid kernel-mode WndProc\n", hWnd);
    ok_eq_ulong(GetLastError(), 0xfeedfab1); // The last-error shouldn't change.

    DestroyWindow(hWnd);

    /* 3. Window with a user-mode WndProc */
    hWnd = CreateWindowExW(0,
                           WndClass,
                           NULL,
                           WS_CAPTION | WS_SYSMENU,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 100,
                           NULL, 0,
                           wcx.hInstance, NULL);
    if (!hWnd)
    {
        skip("CreateWindowExW failed with error %lu\n", GetLastError());
        goto Quit;
    }

    SetLastError(0xfeedfab1);
    ret = IsServerSideWindow(hWnd);
    ok(!ret, "The window %p has a valid kernel-mode WndProc when it should not\n", hWnd);
    ok_eq_ulong(GetLastError(), 0xfeedfab1); // The last-error shouldn't change.

    DestroyWindow(hWnd);

Quit:
    UnregisterClassW(WndClass, wcx.hInstance);
    return;
}
