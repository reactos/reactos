/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for ShowWindow.WS_FORCEMINIMIZE
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

#define RED RGB(255, 0, 0)

static HANDLE g_hThread = NULL;

static COLORREF CheckColor(VOID)
{
    HDC hDC = GetDC(NULL);
    COLORREF color = GetPixel(hDC, 100, 100);
    ReleaseDC(NULL, hDC);
    return color;
}

static DWORD WINAPI
ThreadFunc(LPVOID arg)
{
    BOOL ret;
    HWND hwnd = (HWND)arg;
    DWORD style, exstyle;

    Sleep(100);
    ok_long(CheckColor(), RED);

    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    ShowWindow(hwnd, SW_MINIMIZE);
    Sleep(100);
    ok(CheckColor() != RED, "Color was red\n");

    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    Sleep(100);
    ok_long(CheckColor(), RED);

    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    if (IsWindowsVistaOrGreater())
        ok(ret != FALSE, "ret was FALSE\n");
    else
        ok_bool_false(ret, "Return was");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    PostMessageA(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            DWORD dwThreadId;
            g_hThread = CreateThread(NULL, 0, ThreadFunc, hwnd, 0, &dwThreadId);
            if (!g_hThread)
                return -1;
            CloseHandle(g_hThread);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static VOID
Test_WS_FORCEMINIMIZE(HBRUSH hbr)
{
    WNDCLASSA wc;
    HWND hwnd;
    DWORD style;
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    MSG msg;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = hbr;
    wc.lpszClassName = "SW_FORCEMINIMIZE";
    if (!RegisterClassA(&wc))
    {
        skip("RegisterClassA failed\n");
        return;
    }

    style = WS_POPUP | WS_VISIBLE;
    hwnd = CreateWindowExA(0, "SW_FORCEMINIMIZE", "SW_FORCEMINIMIZE", style,
                           50, 50, 100, 100, NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindowExA failed\n");
        return;
    }

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

START_TEST(SW_FORCEMINIMIZE)
{
    HBRUSH hbr = CreateSolidBrush(RED);
    Test_WS_FORCEMINIMIZE(hbr);
    DeleteObject(hbr);
}
