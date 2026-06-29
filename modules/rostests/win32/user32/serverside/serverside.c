/*
 * PROJECT:         ReactOS test suite
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Test for existense of the kernel mode wndproc in the window
 * COPYRIGHT:       Copyright 2024 Oleg Dubinskiy (oleg.dubinskiy@reactos.org)
 */

/* This testapp tests behaviour of IsServerSideWindow() function in user32. */

#include <windows.h>
#include <stdio.h>
#include <undocuser.h>

#include <debug.h>

WCHAR WndClass[] = L"window class";

LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT msg,
                         WPARAM wParam,
                         LPARAM lParam)
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

int APIENTRY wWinMain(HINSTANCE hInst,
                      HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine,
                      int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEXW wcx;
    UINT result;
    BOOL ret;

    memset(&wcx, 0, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = (WNDPROC)WndProc;
    wcx.hInstance = hInst;
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcx.lpszClassName = WndClass;

    if (!(result = RegisterClassExW(&wcx)))
        return 1;

    /* 1. Window with a valid wndproc */
    hWnd = CreateWindowExW(0,
                           WndClass,
                           NULL,
                           WS_CAPTION | WS_SYSMENU,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 100,
                           NULL, 0,
                           hInst, NULL);
    if (!hWnd)
        return 1;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    ret = IsServerSideWindow(hWnd);
    if (ret)
        DPRINT("OK: the window %p has a valid kernel mode wndproc\n", hWnd);
    else
        DPRINT("FAIL: the window %p is not valid or has no valid kernel mode wndproc\n", hWnd);

    // TODO: this seems to be not a correct test condition.
    //       Find a valid condition to test a kernel mode wmdproc existence correctly!

    //wcx.lpfnWndProc = NULL;

    DestroyWindow(hWnd);

    /* 2. Window without a valid wndproc */
    hWnd = CreateWindowExW(0,
                           WndClass,
                           NULL,
                           WS_CAPTION | WS_SYSMENU,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 100,
                           NULL, 0,
                           hInst, NULL);

    if (!hWnd)
        return 1;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    ret = IsServerSideWindow(hWnd);
    if (ret)
        DPRINT("FAIL: the window %p has a valid kernel mode wndproc when it shouldn't\n", hWnd);
    else
        DPRINT("OK: the window %p has no valid kernel mode wndproc\n", hWnd);

    DestroyWindow(hWnd);

    UnregisterClassW(WndClass, hInst);
    return 0;
}
