/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for TrackPopupMenuEx
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

#define CLASSNAME L"TrackPopupMenuEx tests"
#define MENUCLASS L"#32768"

#define VALID_TPM_FLAGS ( \
    TPM_LAYOUTRTL | TPM_NOANIMATION | TPM_VERNEGANIMATION | TPM_VERPOSANIMATION | \
    TPM_HORNEGANIMATION | TPM_HORPOSANIMATION | TPM_RETURNCMD | \
    TPM_NONOTIFY | TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_VCENTERALIGN | \
    TPM_RIGHTALIGN | TPM_CENTERALIGN | TPM_RIGHTBUTTON | TPM_RECURSE \
)

#ifndef TPM_WORKAREA
#define TPM_WORKAREA 0x10000
#endif

static VOID
TEST_InvalidFlags(VOID)
{
    HWND hwnd = GetDesktopWindow();
    HMENU hMenu = CreatePopupMenu();
    BOOL ret;

    ret = AppendMenuW(hMenu, MF_STRING, 100, L"(Dummy)");
    ok_int(ret, TRUE);

    INT iBit;
    UINT uFlags;
    for (iBit = 0; iBit < sizeof(DWORD) * CHAR_BIT; ++iBit)
    {
        uFlags = (1 << iBit);
        if (uFlags & ~VALID_TPM_FLAGS)
        {
            SetLastError(0xBEEFCAFE);
            ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, NULL);
            ok_int(ret, FALSE);
            if (uFlags == TPM_WORKAREA && IsWindows7OrGreater())
                ok_err(ERROR_INVALID_PARAMETER);
            else
                ok_err(ERROR_INVALID_FLAGS);
        }
    }

    DestroyMenu(hMenu);
}

static VOID
TEST_InvalidSize(VOID)
{
    HWND hwnd = GetDesktopWindow();
    HMENU hMenu = CreatePopupMenu();
    TPMPARAMS params;
    UINT uFlags = TPM_RIGHTBUTTON;
    BOOL ret;

    ZeroMemory(&params, sizeof(params));

    ret = AppendMenuW(hMenu, MF_STRING, 100, L"(Dummy)");
    ok_int(ret, TRUE);

    SetLastError(0xBEEFCAFE);
    params.cbSize = 0;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    params.cbSize = sizeof(params) - 1;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    params.cbSize = sizeof(params) + 1;
    ret = TrackPopupMenuEx(hMenu, uFlags, 0, 0, hwnd, &params);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    DestroyMenu(hMenu);
}

#define DELAY 100
#define INTERVAL 300

typedef enum tagAUTO_CLICK
{
    AUTO_LEFT_CLICK,
    AUTO_RIGHT_CLICK,
    AUTO_LEFT_DOUBLE_CLICK,
    AUTO_RIGHT_DOUBLE_CLICK,
} AUTO_CLICK;

typedef enum tagAUTO_KEY
{
    AUTO_KEY_DOWN,
    AUTO_KEY_UP,
    AUTO_KEY_DOWN_UP,
} AUTO_KEY;

static VOID
AutoKey(AUTO_KEY type, UINT vKey)
{
    if (type == AUTO_KEY_DOWN_UP)
    {
        AutoKey(AUTO_KEY_DOWN, vKey);
        AutoKey(AUTO_KEY_UP, vKey);
        return;
    }

    INPUT input;
    ZeroMemory(&input, sizeof(input));

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vKey;
    input.ki.dwFlags = ((type == AUTO_KEY_UP) ? KEYEVENTF_KEYUP : 0);
    SendInput(1, &input, sizeof(INPUT));
    Sleep(DELAY);
}

static VOID
AutoClick(AUTO_CLICK type, INT x, INT y)
{
    INPUT input;
    ZeroMemory(&input, sizeof(input));

    INT nScreenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
    INT nScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;

    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)(x * (65535.0f / nScreenWidth));
    input.mi.dy = (LONG)(y * (65535.0f / nScreenHeight));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(DELAY);

    input.mi.dx = input.mi.dy = 0;

    INT i, count = 1;
    switch (type)
    {
        case AUTO_LEFT_DOUBLE_CLICK:
            count = 2;
            // FALL THROUGH
        case AUTO_LEFT_CLICK:
        {
            for (i = 0; i < count; ++i)
            {
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);

                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);
            }
            break;
        }
        case AUTO_RIGHT_DOUBLE_CLICK:
            count = 2;
            // FALL THROUGH
        case AUTO_RIGHT_CLICK:
        {
            for (i = 0; i < count; ++i)
            {
                input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);

                input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                SendInput(1, &input, sizeof(INPUT));
                Sleep(DELAY);
            }
            break;
        }
    }
}

static POINT
CenterPoint(const RECT *prc)
{
    POINT pt = { (prc->left + prc->right) / 2, (prc->top + prc->bottom) / 2 };
    return pt;
}

typedef struct tagCOUNTMENUWND
{
    INT nMenuCount;
} COUNTMENUWND, *PCOUNTMENUWND;

static BOOL CALLBACK
CountMenuWndProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    WCHAR szClass[64];
    GetClassNameW(hwnd, szClass, _countof(szClass));
    if (lstrcmpiW(szClass, MENUCLASS) != 0)
        return TRUE;

    PCOUNTMENUWND pData = (PCOUNTMENUWND)lParam;
    pData->nMenuCount += 1;
    return TRUE;
}

static INT
CountMenuWnds(VOID)
{
    COUNTMENUWND data = { 0 };
    EnumWindows(CountMenuWndProc, (LPARAM)&data);
    return data.nMenuCount;
}

static DWORD WINAPI
TEST_Tracking_ThreadFunc(LPVOID arg)
{
    HWND hwnd = (HWND)arg;

    ok_int(CountMenuWnds(), 0);

    RECT rc;
    GetWindowRect(hwnd, &rc);
    POINT pt = CenterPoint(&rc);

    AutoClick(AUTO_RIGHT_CLICK, pt.x, pt.y);
    Sleep(INTERVAL);

    ok_int(CountMenuWnds(), 1);

    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    Sleep(INTERVAL);

    ok_int(CountMenuWnds(), 1);

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);
    Sleep(INTERVAL);

    ok_int(CountMenuWnds(), 0);

    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static VOID
OnRButtonDown(HWND hwnd)
{
    SetForegroundWindow(hwnd);

    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    BOOL ret = AppendMenuW(hMenu, MF_STRING, 100, L"(Dummy)");
    ok_int(ret, TRUE);

    UINT uFlags = TPM_RIGHTBUTTON | TPM_RETURNCMD;
    INT nCmdID = (INT)TrackPopupMenuEx(hMenu, uFlags, pt.x, pt.y, hwnd, NULL);

    ok_int(nCmdID, 100);

    DestroyMenu(hMenu);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return 0;
        case WM_RBUTTONDOWN:
            OnRButtonDown(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static VOID
TEST_Tracking(VOID)
{
    HINSTANCE hInstance = GetModuleHandleW(NULL);;

    WNDCLASSW wc = { 0, WindowProc };
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)UlongToHandle(COLOR_3DFACE + 1);
    wc.lpszClassName = CLASSNAME;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    HWND hwnd = CreateWindowW(CLASSNAME, CLASSNAME, style,
                              0, 0, 320, 200, NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    HANDLE hThread = CreateThread(NULL, 0, TEST_Tracking_ThreadFunc, hwnd, 0, NULL);
    if (!hThread)
    {
        skip("CreateThread failed\n");
        DestroyWindow(hwnd);
        return;
    }
    CloseHandle(hThread);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

START_TEST(TrackPopupMenuEx)
{
    TEST_InvalidFlags();
    TEST_InvalidSize();
    TEST_Tracking();
}
