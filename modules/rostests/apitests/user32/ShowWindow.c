/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for ShowWindow
 * COPYRIGHT:   Copyright 2021-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

typedef struct TEST_ENTRY
{
    INT lineno;
    BOOL ret;
    INT nCmdShow;
    DWORD style0;
    DWORD style1;
} TEST_ENTRY;

static const CHAR s_name[] = "ShowWindow test window";

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    HWND hwnd;
    DWORD style;
    BOOL ret;

    hwnd = CreateWindowA(s_name, s_name, pEntry->style0,
        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
        NULL, NULL, GetModuleHandleA(NULL), NULL);
    ok(!!hwnd, "Line %d: CreateWindowA failed\n", pEntry->lineno);

    ret = !!ShowWindow(hwnd, pEntry->nCmdShow);
    ok(ret == pEntry->ret, "Line %d: ShowWindow returned %s\n", pEntry->lineno,
       (ret ? "non-zero" : "zero"));

    style = (LONG)GetWindowLongPtrA(hwnd, GWL_STYLE);
    ok(style == pEntry->style1, "Line %d: style was 0x%lX\n", pEntry->lineno, style);

    DestroyWindow(hwnd);
}

#define STYLE_0 WS_OVERLAPPEDWINDOW
#define STYLE_1 (WS_OVERLAPPEDWINDOW | WS_MAXIMIZE)
#define STYLE_2 (WS_OVERLAPPEDWINDOW | WS_MINIMIZE)
#define STYLE_3 (WS_OVERLAPPEDWINDOW | WS_VISIBLE)
#define STYLE_4 (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE)
#define STYLE_5 (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MINIMIZE)

#define SW_0 -1
#define SW_1 SW_HIDE
#define SW_2 SW_MAXIMIZE
#define SW_3 SW_MINIMIZE
#define SW_4 SW_RESTORE
#define SW_5 SW_SHOW
#define SW_6 SW_SHOWDEFAULT
#define SW_7 SW_SHOWMAXIMIZED
#define SW_8 SW_SHOWMINIMIZED
#define SW_9 SW_SHOWMINNOACTIVE
#define SW_10 SW_SHOWNA
#define SW_11 SW_SHOWNOACTIVATE
#define SW_12 SW_SHOWNORMAL

static const TEST_ENTRY s_entries[] =
{
    // STYLE_0
    { __LINE__, FALSE, SW_0, STYLE_0, STYLE_0 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_1, STYLE_0, STYLE_0 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_2, STYLE_0, STYLE_0 | WS_VISIBLE | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_3, STYLE_0, STYLE_0 | WS_VISIBLE | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_4, STYLE_0, STYLE_0 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_5, STYLE_0, STYLE_0 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_6, STYLE_0, STYLE_0 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_7, STYLE_0, STYLE_0 | WS_VISIBLE | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_8, STYLE_0, STYLE_0 | WS_VISIBLE | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_9, STYLE_0, STYLE_0 | WS_VISIBLE | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_10, STYLE_0, STYLE_0 | WS_VISIBLE| WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_11, STYLE_0, STYLE_0 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_12, STYLE_0, STYLE_0 | WS_VISIBLE | WS_CLIPSIBLINGS },
    // STYLE_1
    { __LINE__, FALSE, SW_0, STYLE_1, STYLE_1 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_1, STYLE_1, STYLE_1 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_2, STYLE_1, STYLE_1 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_3, STYLE_1, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_4, STYLE_1, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_5, STYLE_1, STYLE_1 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_6, STYLE_1, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_7, STYLE_1, STYLE_1 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_8, STYLE_1, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_9, STYLE_1, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_10, STYLE_1, STYLE_1 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_11, STYLE_1, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_12, STYLE_1, STYLE_3 | WS_CLIPSIBLINGS },
    // STYLE_2
    { __LINE__, FALSE, SW_0, STYLE_2, STYLE_2 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_1, STYLE_2, STYLE_2 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_2, STYLE_2, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_3, STYLE_2, STYLE_2 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_4, STYLE_2, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_5, STYLE_2, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_6, STYLE_2, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_7, STYLE_2, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_8, STYLE_2, STYLE_2 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_9, STYLE_2, STYLE_2 | WS_VISIBLE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_10, STYLE_2, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_11, STYLE_2, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, FALSE, SW_12, STYLE_2, STYLE_3 | WS_CLIPSIBLINGS },
    // STYLE_3
    { __LINE__, FALSE, SW_0, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_1, STYLE_3, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_2, STYLE_3, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_3, STYLE_3, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_4, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_5, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_6, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_7, STYLE_3, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_8, STYLE_3, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_9, STYLE_3, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_10, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_11, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_12, STYLE_3, STYLE_3 | WS_CLIPSIBLINGS },
    // STYLE_4
    { __LINE__, FALSE, SW_0, STYLE_4, STYLE_4 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_1, STYLE_4, WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_2, STYLE_4, STYLE_4 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_3, STYLE_4, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_4, STYLE_4, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_5, STYLE_4, STYLE_4 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_6, STYLE_4, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_7, STYLE_4, STYLE_4 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_8, STYLE_4, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_9, STYLE_4, STYLE_3 | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_10, STYLE_4, STYLE_4 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_11, STYLE_4, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_12, STYLE_4, STYLE_3 | WS_CLIPSIBLINGS },
    // STYLE_5
    { __LINE__, FALSE, SW_0, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_1, STYLE_5, WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_2, STYLE_5, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_3, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_4, STYLE_5, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_5, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_6, STYLE_5, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_7, STYLE_5, STYLE_3 | WS_MAXIMIZE | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_8, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_9, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_10, STYLE_5, STYLE_5 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_11, STYLE_5, STYLE_3 | WS_CLIPSIBLINGS },
    { __LINE__, TRUE, SW_12, STYLE_5, STYLE_3 | WS_CLIPSIBLINGS },
};

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static VOID Test_ShowWindow_Main(VOID)
{
    WNDCLASSA wc;
    UINT iTest;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_name;
    ok_int(1, !!RegisterClassA(&wc));

    for (iTest = 0; iTest < _countof(s_entries); ++iTest)
    {
        DoTestEntry(&s_entries[iTest]);
    }
}

#define RED RGB(255, 0, 0)

static COLORREF CheckColor(VOID)
{
    HDC hDC = GetDC(NULL);
    COLORREF color = GetPixel(hDC, 100, 100);
    ReleaseDC(NULL, hDC);
    return color;
}

static DWORD WINAPI
ForceMinimizeThreadFunc(LPVOID arg)
{
    BOOL ret;
    HWND hwnd = (HWND)arg;
    DWORD style, exstyle;

    Sleep(100);
    ok_long(CheckColor(), RED);

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    ShowWindow(hwnd, SW_MINIMIZE);
    Sleep(100);
    ok(CheckColor() != RED, "Color was red\n");

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    Sleep(100);
    ok_long(CheckColor(), RED);

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    ok(ret != FALSE, "ret was FALSE\n");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_MAKEVISIBLEWHENUNGHOSTED);
    ret = ShowWindow(hwnd, SW_FORCEMINIMIZE);
    Sleep(100);
    if (IsWindowsVistaOrGreater())
        ok(ret != FALSE, "ret was FALSE\n");
    else
        ok_bool_false(ret, "Return was");
    ok(CheckColor() != RED, "Color was red\n");
    style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (IsWindowsVistaOrGreater())
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP | WS_MINIMIZE | WS_VISIBLE), "style was 0x%08lX\n", style);
    else
        ok((style & (WS_POPUP | WS_MINIMIZE | WS_VISIBLE)) == (WS_POPUP), "style was 0x%08lX\n", style);
    exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ok_long(exstyle, 0);

    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static LRESULT CALLBACK
ForceMinimizeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static VOID
Test_WS_FORCEMINIMIZE_Sub(HBRUSH hbr)
{
    WNDCLASSW wc;
    HWND hwnd;
    DWORD style;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    DWORD dwThreadId;
    HANDLE hThread;
    MSG msg;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = ForceMinimizeWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(NULL, (PCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, (PCWSTR)IDC_ARROW);
    wc.hbrBackground = hbr;
    wc.lpszClassName = L"SW_FORCEMINIMIZE";
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    style = WS_POPUP | WS_VISIBLE;
    hwnd = CreateWindowExW(0, L"SW_FORCEMINIMIZE", L"SW_FORCEMINIMIZE", style,
                           50, 50, 100, 100, NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindowExW failed\n");
        return;
    }

    hThread = CreateThread(NULL, 0, ForceMinimizeThreadFunc, hwnd, 0, &dwThreadId);
    if (!hThread)
    {
        skip("CreateThread failed\n");
        DestroyWindow(hwnd);
        return;
    }
    CloseHandle(hThread);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static VOID
Test_WS_FORCEMINIMIZE()
{
    HBRUSH hbr = CreateSolidBrush(RED);
    Test_WS_FORCEMINIMIZE_Sub(hbr);
    DeleteObject(hbr);
}

START_TEST(ShowWindow)
{
    Test_ShowWindow_Main();
    Test_WS_FORCEMINIMIZE();
}
