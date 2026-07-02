/*
 * PROJECT:         ReactOS test suite
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Test for existence of the kernel-mode WndProc in the window
 * COPYRIGHT:       Copyright 2024 Oleg Dubinskiy (oleg.dubinskiy@reactos.org)
 */

/* This testapp tests behaviour of IsServerSideWindow() function in user32. */

#include "precomp.h"

static const WCHAR WndClass[] = L"window class";

LRESULT
CALLBACK
Test_IsServerSideWindow_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
    UINT result;

    ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = Test_IsServerSideWindow_WndProc;
    wcx.hInstance = GetModuleHandleW(NULL);
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcx.lpszClassName = WndClass;

    if (!(result = RegisterClassExW(&wcx)))
    {
        skip("RegisterClassExW failed with error %lu\n", GetLastError());
        return;
    }

    /* 1. Invalid window */
    hWnd = (HWND)0xdeadbeef;
    SetLastError(0xfeedfab1);
    ok(!IsServerSideWindow(hWnd), "The window %p is invalid but IsServerSideWindow() returned TRUE\n", hWnd);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() returned %lu instead of ERROR_INVALID_WINDOW_HANDLE\n", GetLastError());

    /* 2. Window with a kernel-mode WndProc */
    /* ScrollBar is an example of a server-side window that can be created from user-mode code */
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
        UnregisterClassW(WndClass, wcx.hInstance);
        return;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    ok(IsServerSideWindow(hWnd), "The window %p is invalid or doesn't have a valid kernel-mode WndProc\n", hWnd);

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
        UnregisterClassW(WndClass, wcx.hInstance);
        return;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    ok(!IsServerSideWindow(hWnd), "The window %p has a valid kernel-mode WndProc when it should not\n", hWnd);

    DestroyWindow(hWnd);

    UnregisterClassW(WndClass, wcx.hInstance);
    return;
}
