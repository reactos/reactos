/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SwitchToThisWindow
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static const WCHAR s_szClassName[] = L"SwitchTest";

static BOOL s_bTracing = FALSE;

static BOOL s_bWM_SYSCOMMAND_SC_RESTORE = FALSE;
static BOOL s_bWM_SYSCOMMAND_NOT_SC_RESTORE = FALSE;
static BOOL s_bWM_NCACTIVATE = FALSE;
static BOOL s_bWM_WINDOWPOSCHANGING = FALSE;
static BOOL s_bWM_ACTIVATE = FALSE;

#define INTERVAL 200

static void
DoMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TIMER || !s_bTracing)
        return;

    trace("uMsg:0x%04X, wParam:0x%08lX, lParam:0x%08lX\n", uMsg, (LONG)wParam, (LONG)lParam);

    if (uMsg == WM_SYSCOMMAND)
    {
        if (wParam == SC_RESTORE)
            s_bWM_SYSCOMMAND_SC_RESTORE = TRUE;
        else
            s_bWM_SYSCOMMAND_NOT_SC_RESTORE = TRUE;
    }

    if (uMsg == WM_NCACTIVATE)
        s_bWM_NCACTIVATE = TRUE;

    if (uMsg == WM_WINDOWPOSCHANGING)
        s_bWM_WINDOWPOSCHANGING = TRUE;

    if (uMsg == WM_ACTIVATE)
        s_bWM_ACTIVATE = TRUE;
}

// WM_TIMER
static void
OnTimer(HWND hwnd, UINT id)
{
    KillTimer(hwnd, id);
    switch (id)
    {
        //
        // SwitchToThisWindow(TRUE)
        //
        case 0: // minimize
            SetForegroundWindow(GetDesktopWindow());
            SetActiveWindow(GetDesktopWindow());
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == NULL, "GetActiveWindow() != NULL\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            CloseWindow(hwnd);  // minimize
            break;
        case 1: // start tracing
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_bTracing = TRUE;
            s_bWM_SYSCOMMAND_SC_RESTORE = FALSE;
            s_bWM_SYSCOMMAND_NOT_SC_RESTORE = FALSE;
            s_bWM_NCACTIVATE = FALSE;
            s_bWM_WINDOWPOSCHANGING = FALSE;
            s_bWM_ACTIVATE = FALSE;
            SwitchToThisWindow(hwnd, TRUE);
            trace("SwitchToThisWindow(TRUE): tracing...\n");
            break;
        case 2: // tracing done
            s_bTracing = FALSE;
            trace("SwitchToThisWindow(TRUE): tracing done\n");
            ok(GetForegroundWindow() == hwnd, "GetForegroundWindow() != hwnd\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == hwnd, "GetFocus() != hwnd\n");
            ok(s_bWM_SYSCOMMAND_SC_RESTORE, "WM_SYSCOMMAND SC_RESTORE: not found\n");
            ok(!s_bWM_SYSCOMMAND_NOT_SC_RESTORE, "WM_SYSCOMMAND not SC_RESTORE: found\n");
            ok(s_bWM_NCACTIVATE, "WM_NCACTIVATE: not found\n");
            ok(s_bWM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING: not found\n");
            ok(s_bWM_ACTIVATE, "WM_ACTIVATE: not found\n");
            break;
        //
        // SwitchToThisWindow(FALSE)
        //
        case 3: // minimize
            SetForegroundWindow(GetDesktopWindow());
            SetActiveWindow(GetDesktopWindow());
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == NULL, "GetActiveWindow() != NULL\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            CloseWindow(hwnd);  // minimize
            break;
        case 4: // start tracing
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            s_bTracing = TRUE;
            s_bWM_SYSCOMMAND_SC_RESTORE = FALSE;
            s_bWM_SYSCOMMAND_NOT_SC_RESTORE = FALSE;
            s_bWM_NCACTIVATE = FALSE;
            s_bWM_WINDOWPOSCHANGING = FALSE;
            s_bWM_ACTIVATE = FALSE;
            SwitchToThisWindow(hwnd, FALSE);
            trace("SwitchToThisWindow(FALSE): tracing...\n");
            break;
        case 5: // tracing done
            s_bTracing = FALSE;
            trace("SwitchToThisWindow(FALSE): tracing done\n");
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            ok(!s_bWM_SYSCOMMAND_SC_RESTORE, "WM_SYSCOMMAND SC_RESTORE: found\n");
            ok(!s_bWM_SYSCOMMAND_NOT_SC_RESTORE, "WM_SYSCOMMAND not SC_RESTORE: found\n");
            ok(!s_bWM_NCACTIVATE, "WM_NCACTIVATE: found\n");
            ok(!s_bWM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING: not found\n");
            ok(!s_bWM_ACTIVATE, "WM_ACTIVATE: found\n");
            break;
        default: // finish
            DestroyWindow(hwnd);
            return;
    }
    SetTimer(hwnd, id + 1, INTERVAL, NULL);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DoMessage(hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CREATE:
        SetTimer(hwnd, 0, INTERVAL, NULL);
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

START_TEST(SwitchToThisWindow)
{
    WNDCLASSW wc;
    HICON hIcon;
    HCURSOR hCursor;
    ATOM atom;
    HWND hwnd;
    MSG msg;

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

    hwnd = CreateWindowW(s_szClassName, L"SwitchToThisWindow", WS_OVERLAPPEDWINDOW,
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
