/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CloseWindow
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static const WCHAR s_szClassName[] = L"CloseWindowTest";

static BOOL s_bTracing = FALSE;

static INT s_nWM_SYSCOMMAND = 0;
static INT s_nWM_NCACTIVATE = 0;
static INT s_nWM_WINDOWPOSCHANGING = 0;
static INT s_nWM_ACTIVATE = 0;

#define TIMER_INTERVAL 200

static void
DoMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TIMER || !s_bTracing)
        return;

    trace("uMsg:0x%04X, wParam:0x%08lX, lParam:0x%08lX\n", uMsg, (LONG)wParam, (LONG)lParam);

    if (uMsg == WM_SYSCOMMAND)
    {
        ++s_nWM_SYSCOMMAND;
    }

    if (uMsg == WM_NCACTIVATE)
        ++s_nWM_NCACTIVATE;

    if (uMsg == WM_WINDOWPOSCHANGING)
        ++s_nWM_WINDOWPOSCHANGING;

    if (uMsg == WM_ACTIVATE)
        ++s_nWM_ACTIVATE;
}

// WM_TIMER
static void
OnTimer(HWND hwnd, UINT id)
{
    KillTimer(hwnd, id);
    switch (id)
    {
        //
        // SetActiveWindow(GetDesktopWindow());
        //
        case 0:
            SetForegroundWindow(GetDesktopWindow());
            SetActiveWindow(GetDesktopWindow());
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == NULL, "GetActiveWindow() != NULL\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_nWM_SYSCOMMAND = 0;
            s_nWM_NCACTIVATE = 0;
            s_nWM_WINDOWPOSCHANGING = 0;
            s_nWM_ACTIVATE = 0;
            s_bTracing = TRUE;
            trace("CloseWindow(hwnd): tracing...\n");
            ok(CloseWindow(hwnd), "CloseWindow failed\n");
            break;
        case 1:
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_bTracing = FALSE;
            trace("CloseWindow(hwnd): tracing done\n");
            ok(s_nWM_SYSCOMMAND == 0, "WM_SYSCOMMAND: %d\n", s_nWM_SYSCOMMAND);
            ok(s_nWM_NCACTIVATE == 1, "WM_NCACTIVATE: %d\n", s_nWM_NCACTIVATE);
            ok(s_nWM_WINDOWPOSCHANGING == 2, "WM_WINDOWPOSCHANGING: %d\n", s_nWM_WINDOWPOSCHANGING);
            ok(s_nWM_ACTIVATE == 1, "WM_ACTIVATE: %d\n", s_nWM_ACTIVATE);
            ShowWindow(hwnd, SW_RESTORE);
            break;
        //
        // SetActiveWindow(hwnd);
        //
        case 2:
            SetForegroundWindow(GetDesktopWindow());
            SetActiveWindow(hwnd);
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == hwnd, "GetFocus() != hwnd\n");
            s_nWM_SYSCOMMAND = 0;
            s_nWM_NCACTIVATE = 0;
            s_nWM_WINDOWPOSCHANGING = 0;
            s_nWM_ACTIVATE = 0;
            s_bTracing = TRUE;
            trace("CloseWindow(hwnd): tracing...\n");
            ok(CloseWindow(hwnd), "CloseWindow failed\n");
            break;
        case 3:
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_bTracing = FALSE;
            trace("CloseWindow(hwnd): tracing done\n");
            ok(s_nWM_SYSCOMMAND == 0, "WM_SYSCOMMAND: %d\n", s_nWM_SYSCOMMAND);
            ok(s_nWM_NCACTIVATE == 0, "WM_NCACTIVATE: %d\n", s_nWM_NCACTIVATE);
            ok(s_nWM_WINDOWPOSCHANGING == 1, "WM_WINDOWPOSCHANGING: %d\n", s_nWM_WINDOWPOSCHANGING);
            ok(s_nWM_ACTIVATE == 0, "WM_ACTIVATE: %d\n", s_nWM_ACTIVATE);
            ShowWindow(hwnd, SW_RESTORE);
            break;
        //
        // SetForegroundWindow(hwnd);
        //
        case 4:
            SetActiveWindow(GetDesktopWindow());
            SetForegroundWindow(hwnd);
            ok(GetForegroundWindow() == hwnd, "GetForegroundWindow() != hwnd\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == hwnd, "GetFocus() != hwnd\n");
            s_nWM_SYSCOMMAND = 0;
            s_nWM_NCACTIVATE = 0;
            s_nWM_WINDOWPOSCHANGING = 0;
            s_nWM_ACTIVATE = 0;
            s_bTracing = TRUE;
            trace("CloseWindow(hwnd): tracing...\n");
            ok(CloseWindow(hwnd), "CloseWindow failed\n");
            break;
        case 5:
            ok(GetForegroundWindow() == hwnd, "GetForegroundWindow() != hwnd\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_bTracing = FALSE;
            trace("CloseWindow(hwnd): tracing done\n");
            ok(s_nWM_SYSCOMMAND == 0, "WM_SYSCOMMAND: %d\n", s_nWM_SYSCOMMAND);
            ok(s_nWM_NCACTIVATE == 0, "WM_NCACTIVATE: %d\n", s_nWM_NCACTIVATE);
            ok(s_nWM_WINDOWPOSCHANGING == 1, "WM_WINDOWPOSCHANGING: %d\n", s_nWM_WINDOWPOSCHANGING);
            ok(s_nWM_ACTIVATE == 0, "WM_ACTIVATE: %d\n", s_nWM_ACTIVATE);
            ShowWindow(hwnd, SW_RESTORE);
            break;
        default: // finish
            DestroyWindow(hwnd);
            return;
    }
    SetTimer(hwnd, id + 1, TIMER_INTERVAL, NULL);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DoMessage(hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
        case WM_CREATE:
            SetTimer(hwnd, 0, TIMER_INTERVAL, NULL);
            break;
        case WM_TIMER:
            OnTimer(hwnd, (UINT)wParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(CloseWindow)
{
    WNDCLASSW wc;
    HICON hIcon;
    HCURSOR hCursor;
    ATOM atom;
    HWND hwnd;
    MSG msg;

    ok(!CloseWindow(NULL), "CloseWindow(NULL) should be failed\n");

    hIcon = LoadIcon(NULL, IDI_APPLICATION);
    ok(hIcon != NULL, "hIcon was NULL\n");
    hCursor = LoadCursor(NULL, IDC_ARROW);
    ok(hCursor != NULL, "hCursor was NULL\n");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = hIcon;
    wc.hCursor = hCursor;
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szClassName;
    atom = RegisterClassW(&wc);
    ok(atom != 0, "RegisterClassW failed\n");

    if (!atom)
    {
        skip("atom is zero\n");
        DestroyIcon(hIcon);
        DestroyCursor(hCursor);
        return;
    }

    hwnd = CreateWindowW(s_szClassName, L"CloseWindow testcase", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(hwnd != NULL, "CreateWindowW failed\n");
    trace("hwnd: %p\n", hwnd);

    if (!hwnd)
    {
        skip("hwnd is NULL\n");
        UnregisterClassW(s_szClassName, GetModuleHandleW(NULL));
        DestroyIcon(hIcon);
        DestroyCursor(hCursor);
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClassW(s_szClassName, GetModuleHandleW(NULL));
    DestroyIcon(hIcon);
    DestroyCursor(hCursor);
}
