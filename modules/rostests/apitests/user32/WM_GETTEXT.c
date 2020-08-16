/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for WM_GETTEXT and WM_GETTEXTLENGTH
 * COPYRIGHT:   Copyright 2020 Thomas Faber <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static const LPCSTR s_szNameA[] =
{
    "The WM_GETTEXT testcase (Ansi)",
    "The WM_GETTEXT testcase (Unicode)"
};
static const LPCWSTR s_szNameW[] =
{
    L"The WM_GETTEXT ",
    L"The WM_GETTEXT testcase (Unicode)"
};

static INT s_nLengthA[4];
static CHAR s_szBuffA[2][256];

static INT s_nLengthW[4];
static WCHAR s_szBuffW[2][256];

static LRESULT CALLBACK
WindowProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            SetTimer(hwnd, 999, 50, NULL);
            break;
        }
        case WM_TIMER:
        {
            KillTimer(hwnd, 999);
            s_nLengthA[0] = (INT)SendMessageA(hwnd, WM_GETTEXTLENGTH, 0, 0);
            s_nLengthA[1] = (INT)SendMessageA(hwnd, WM_GETTEXT, ARRAYSIZE(s_szBuffA[0]), (LPARAM)s_szBuffA[0]);
            s_nLengthA[2] = (INT)SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
            s_nLengthA[3] = (INT)SendMessageW(hwnd, WM_GETTEXT, ARRAYSIZE(s_szBuffW[0]), (LPARAM)s_szBuffW[0]);
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK
WindowProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            SetTimer(hwnd, 999, 50, NULL);
            break;
        }
        case WM_TIMER:
        {
            KillTimer(hwnd, 999);
            s_nLengthW[0] = (INT)SendMessageA(hwnd, WM_GETTEXTLENGTH, 0, 0);
            s_nLengthW[1] = (INT)SendMessageA(hwnd, WM_GETTEXT, ARRAYSIZE(s_szBuffA[1]), (LPARAM)s_szBuffA[1]);
            s_nLengthW[2] = (INT)SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
            s_nLengthW[3] = (INT)SendMessageW(hwnd, WM_GETTEXT, ARRAYSIZE(s_szBuffW[1]), (LPARAM)s_szBuffW[1]);
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

START_TEST(WM_GETTEXT)
{
    HWND hwnd;
    MSG msg;
    WNDCLASSA wcA;
    WNDCLASSW wcW;

    ZeroMemory(&wcA, sizeof(wcA));
    wcA.lpfnWndProc = WindowProcA;
    wcA.hInstance = GetModuleHandleA(NULL);
    wcA.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcA.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcA.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcA.lpszClassName = s_szNameA[0];
    if (!RegisterClassA(&wcA))
    {
        skip("RegisterClass failed\n");
        return;
    }

    ZeroMemory(&wcW, sizeof(wcW));
    wcW.lpfnWndProc = WindowProcW;
    wcW.hInstance = GetModuleHandleW(NULL);
    wcW.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcW.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcW.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcW.lpszClassName = s_szNameW[1];
    if (!RegisterClassW(&wcW))
    {
        skip("RegisterClass failed\n");
        return;
    }

    hwnd = CreateWindowA(s_szNameA[0], s_szNameA[0], WS_OVERLAPPEDWINDOW,
                         0, 0, 100, 100, NULL, NULL, wcA.hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindow failed\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    hwnd = CreateWindowW(s_szNameW[1], s_szNameW[1], WS_OVERLAPPEDWINDOW,
                         0, 0, 100, 100, NULL, NULL, wcW.hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindow failed\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ok_int(s_nLengthA[0], 15);
    ok_int(s_nLengthA[1], 15);
    ok_int(s_nLengthA[2], 15);
    ok_int(s_nLengthA[3], 15);

    ok_int(s_nLengthW[0], 33);
    ok_int(s_nLengthW[1], 33);
    ok_int(s_nLengthW[2], 33);
    ok_int(s_nLengthW[3], 33);

    ok_str(s_szBuffA[0], s_szNameA[0]);
    ok_str(s_szBuffA[1], s_szNameA[1]);

    ok_wstr(s_szBuffW[0], s_szNameW[0]);
    ok_wstr(s_szBuffW[1], s_szNameW[1]);
}
