/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for GW_ENABLEDPOPUP
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <windowsx.h>

static HWND s_hwnd = NULL;
static HWND s_hwndChild1 = NULL;
static HWND s_hwndChild2 = NULL;
static HWND s_hwndChild3 = NULL;

static VOID DoTestBody(VOID)
{
    HWND hwndRet;
#define CHECK(hwndTarget) do { \
    hwndRet = GetWindow(s_hwnd, GW_ENABLEDPOPUP); \
    trace("hwndRet: %p\n", hwndRet); \
    ok_int(hwndRet == hwndTarget, TRUE); \
} while (0)

    trace("%p, %p, %p, %p\n", s_hwnd, s_hwndChild1, s_hwndChild2, s_hwndChild3);

    CHECK(s_hwndChild3);

    EnableWindow(s_hwndChild3, FALSE);

    CHECK(s_hwndChild2);

    EnableWindow(s_hwndChild2, FALSE);

    CHECK(NULL);

    EnableWindow(s_hwndChild2, TRUE);

    CHECK(s_hwndChild2);

    ShowWindow(s_hwndChild1, SW_HIDE);

    CHECK(s_hwndChild2);

    ShowWindow(s_hwndChild2, SW_HIDE);

    CHECK(NULL);

    ShowWindow(s_hwndChild2, SW_SHOWNOACTIVATE);

    CHECK(s_hwndChild2);

    EnableWindow(s_hwndChild1, TRUE);
    ShowWindow(s_hwndChild2, SW_SHOWNOACTIVATE);

    CHECK(s_hwndChild2);

    ShowWindow(s_hwndChild1, SW_SHOWNOACTIVATE);

    CHECK(s_hwndChild2);

    ShowWindow(s_hwndChild2, SW_HIDE);

    CHECK(s_hwndChild1);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            SetTimer(hwnd, 999, 500, NULL);
            break;
        case WM_DESTROY:
            if (s_hwnd == hwnd)
            {
                DestroyWindow(s_hwndChild1);
                DestroyWindow(s_hwndChild2);
                DestroyWindow(s_hwndChild3);
                PostQuitMessage(0);
            }
            break;
        case WM_TIMER:
            KillTimer(hwnd, wParam);
            if (wParam == 999 && hwnd == s_hwnd)
            {
                DoTestBody();
                DestroyWindow(hwnd);
                break;
            }
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(GW_ENABLEDPOPUP)
{
    WNDCLASSW wc = { 0 };
    MSG msg;

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = L"GW_ENABLEDPOPUP";
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClass failed\n");
        return;
    }

    s_hwnd = CreateWindowW(L"GW_ENABLEDPOPUP", L"GW_ENABLEDPOPUP",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           0, 0, 100, 100,
                           NULL, NULL, wc.hInstance, NULL);
    s_hwndChild1 = CreateWindowW(L"GW_ENABLEDPOPUP", L"Child #1",
                                 WS_POPUPWINDOW | WS_CHILD | WS_VISIBLE | WS_DISABLED,
                                 100, 100, 100, 100,
                                 s_hwnd, NULL, wc.hInstance, NULL);
    s_hwndChild2 = CreateWindowW(L"GW_ENABLEDPOPUP", L"Child #2",
                                 WS_POPUPWINDOW | WS_CHILD | WS_VISIBLE,
                                 200, 200, 100, 100,
                                 s_hwnd, NULL, wc.hInstance, NULL);
    s_hwndChild3 = CreateWindowW(L"GW_ENABLEDPOPUP", L"Child #3",
                                 WS_POPUPWINDOW | WS_CHILD | WS_VISIBLE,
                                 300, 100, 100, 100,
                                 s_hwndChild2, NULL, wc.hInstance, NULL);
    if (!s_hwnd || !s_hwndChild1 || !s_hwndChild2 || !s_hwndChild3)
    {
        skip("CreateWindow failed\n");
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
