/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SwitchToThisWindow
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static const WCHAR s_szClassName[] = L"SwitchTest";

static BOOL s_bTracing = FALSE;

static INT s_nWM_SYSCOMMAND_SC_RESTORE = 0;
static INT s_nWM_SYSCOMMAND_NOT_SC_RESTORE = 0;
static INT s_nWM_NCACTIVATE = 0;
static INT s_nWM_WINDOWPOSCHANGING = 0;
static INT s_nWM_ACTIVATE = 0;

#define TIMER_INTERVAL 200

static const char *
DumpInSMEX(void)
{
    static char s_buf[128];
    DWORD dwRet = InSendMessageEx(NULL);
    if (dwRet == ISMEX_NOSEND)
    {
        strcpy(s_buf, "ISMEX_NOSEND,");
        return s_buf;
    }
    s_buf[0] = 0;
    if (dwRet & ISMEX_CALLBACK)
        strcat(s_buf, "ISMEX_CALLBACK,");
    if (dwRet & ISMEX_NOTIFY)
        strcat(s_buf, "ISMEX_NOTIFY,");
    if (dwRet & ISMEX_REPLIED)
        strcat(s_buf, "ISMEX_REPLIED,");
    if (dwRet & ISMEX_SEND)
        strcat(s_buf, "ISMEX_SEND,");
    return s_buf;
}

static void
DoMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TIMER || !s_bTracing)
        return;

    trace("%s: uMsg:0x%04X, wParam:0x%08lX, lParam:0x%08lX, ISMEX_:%s\n",
          (InSendMessage() ? "S" : "P"), uMsg, (LONG)wParam, (LONG)lParam,
           DumpInSMEX());

    if (uMsg == WM_SYSCOMMAND)  // 0x0112
    {
        ok(InSendMessageEx(NULL) == ISMEX_NOSEND,
           "InSendMessageEx(NULL) was 0x%08lX\n", InSendMessageEx(NULL));
        if (wParam == SC_RESTORE)
            ++s_nWM_SYSCOMMAND_SC_RESTORE;
        else
            ++s_nWM_SYSCOMMAND_NOT_SC_RESTORE;
    }

    if (uMsg == WM_NCACTIVATE)  // 0x0086
    {
        ok(InSendMessageEx(NULL) == ISMEX_NOSEND,
           "InSendMessageEx(NULL) was 0x%08lX\n", InSendMessageEx(NULL));
        ++s_nWM_NCACTIVATE;
    }

    if (uMsg == WM_WINDOWPOSCHANGING)   // 0x0046
    {
        ok(InSendMessageEx(NULL) == ISMEX_NOSEND,
           "InSendMessageEx(NULL) was 0x%08lX\n", InSendMessageEx(NULL));
        ++s_nWM_WINDOWPOSCHANGING;
    }

    if (uMsg == WM_ACTIVATE)    // 0x0006
    {
        ok(InSendMessageEx(NULL) == ISMEX_NOSEND,
           "InSendMessageEx(NULL) was 0x%08lX\n", InSendMessageEx(NULL));
        ++s_nWM_ACTIVATE;
    }
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
            s_nWM_SYSCOMMAND_SC_RESTORE = 0;
            s_nWM_SYSCOMMAND_NOT_SC_RESTORE = 0;
            s_nWM_NCACTIVATE = 0;
            s_nWM_WINDOWPOSCHANGING = 0;
            s_nWM_ACTIVATE = 0;
            s_bTracing = TRUE;
            SwitchToThisWindow(hwnd, TRUE);
            trace("SwitchToThisWindow(TRUE): tracing...\n");
            break;
        case 2: // tracing done
            s_bTracing = FALSE;
            trace("SwitchToThisWindow(TRUE): tracing done\n");
            ok(GetForegroundWindow() == hwnd, "GetForegroundWindow() != hwnd\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == hwnd, "GetFocus() != hwnd\n");
            ok(s_nWM_SYSCOMMAND_SC_RESTORE == 1, "WM_SYSCOMMAND SC_RESTORE: %d\n", s_nWM_SYSCOMMAND_SC_RESTORE);
            ok(!s_nWM_SYSCOMMAND_NOT_SC_RESTORE, "WM_SYSCOMMAND non-SC_RESTORE: %d\n", s_nWM_SYSCOMMAND_NOT_SC_RESTORE);
            ok(s_nWM_NCACTIVATE == 1, "WM_NCACTIVATE: %d\n", s_nWM_NCACTIVATE);
            ok(s_nWM_WINDOWPOSCHANGING == 2, "WM_WINDOWPOSCHANGING: %d\n", s_nWM_WINDOWPOSCHANGING);
            ok(s_nWM_ACTIVATE == 1, "WM_ACTIVATE: %d\n", s_nWM_ACTIVATE);
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
            s_nWM_SYSCOMMAND_SC_RESTORE = 0;
            s_nWM_SYSCOMMAND_NOT_SC_RESTORE = 0;
            s_nWM_NCACTIVATE = 0;
            s_nWM_WINDOWPOSCHANGING = 0;
            s_nWM_ACTIVATE = 0;
            s_bTracing = TRUE;
            SwitchToThisWindow(hwnd, FALSE);
            trace("SwitchToThisWindow(FALSE): tracing...\n");
            break;
        case 5: // tracing done
            s_bTracing = FALSE;
            trace("SwitchToThisWindow(FALSE): tracing done\n");
            ok(GetForegroundWindow() == NULL, "GetForegroundWindow() != NULL\n");
            ok(GetActiveWindow() == hwnd, "GetActiveWindow() != hwnd\n");
            ok(GetFocus() == NULL, "GetFocus() != NULL\n");
            ok(!s_nWM_SYSCOMMAND_SC_RESTORE, "WM_SYSCOMMAND SC_RESTORE: %d\n", s_nWM_SYSCOMMAND_SC_RESTORE);
            ok(!s_nWM_SYSCOMMAND_NOT_SC_RESTORE, "WM_SYSCOMMAND non-SC_RESTORE: %d\n", s_nWM_SYSCOMMAND_NOT_SC_RESTORE);
            ok(!s_nWM_NCACTIVATE, "WM_NCACTIVATE: %d\n", s_nWM_NCACTIVATE);
            ok(!s_nWM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING: %d\n", s_nWM_WINDOWPOSCHANGING);
            ok(!s_nWM_ACTIVATE, "WM_ACTIVATE: %d\n", s_nWM_ACTIVATE);
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
