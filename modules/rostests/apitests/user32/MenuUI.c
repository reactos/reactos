/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for Menu UI
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <shellapi.h>

#define SUB_PROGRAM L"user32_apitest_menuui.exe"
#define CLASSNAME L"MenuUITest"
#define MENUCLASS L"#32768"

#define MENUID_100 100
#define MENUID_101 101
#define MENUID_200 200
#define MENUID_201 201

typedef BOOL (WINAPI *FN_ShellExecuteExW)(SHELLEXECUTEINFOW *);
static FN_ShellExecuteExW s_pShellExecuteExW = NULL;

typedef HRESULT (WINAPI *FN_DwmGetWindowAttribute)(HWND, DWORD, PVOID, DWORD);

#define DELAY 100
#define INTERVAL 300
static HANDLE s_hThread = NULL;

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

static INT
GetHitID(HWND hwndTarget)
{
    return HandleToUlong(GetPropW(hwndTarget, L"Hit"));
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

static VOID
CloseSubPrograms(VOID)
{
    for (INT i = 0; i < 10; ++i)
    {
        HWND hwnd1 = FindWindowW(L"user32_apitest_menuui", L"#1");
        if (!hwnd1)
            break;
        PostMessage(hwnd1, WM_CLOSE, 0, 0);
        Sleep(INTERVAL);
    }
    for (INT i = 0; i < 10; ++i)
    {
        HWND hwnd2 = FindWindowW(L"user32_apitest_menuui", L"#2");
        if (!hwnd2)
            break;
        PostMessage(hwnd2, WM_CLOSE, 0, 0);
        Sleep(INTERVAL);
    }
}

static HWND
GetThreadActiveWnd(DWORD dwThreadID)
{
    GUITHREADINFO info = { sizeof(info) };
    GetGUIThreadInfo(dwThreadID, &info);
    return info.hwndActive;
}

static HWND
GetThreadFocus(DWORD dwThreadID)
{
    GUITHREADINFO info = { sizeof(info) };
    GetGUIThreadInfo(dwThreadID, &info);
    return info.hwndFocus;
}

static HWND
GetThreadCapture(DWORD dwThreadID)
{
    GUITHREADINFO info = { sizeof(info) };
    GetGUIThreadInfo(dwThreadID, &info);
    return info.hwndCapture;
}

static DWORD WINAPI
ThreadFunc(LPVOID arg)
{
    HWND hwnd = FindWindowW(L"MenuUITest", L"MenuUITest");
    ShowWindow(hwnd, SW_MINIMIZE);
    trace("hwnd: %p\n", hwnd);

    CloseSubPrograms();

    SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS };

    sei.lpFile = SUB_PROGRAM;
    sei.nShow = SW_SHOWNORMAL;

    // Start up sub program #1
    sei.lpParameters = L"#1";
    if (!s_pShellExecuteExW(&sei))
    {
        skip("ShellExecuteExW failed\n");
        return -1;
    }
    WaitForInputIdle(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);

    // Start up sub program #2
    sei.lpParameters = L"#2";
    if (!s_pShellExecuteExW(&sei))
    {
        skip("ShellExecuteExW failed\n");
        return -1;
    }
    WaitForInputIdle(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);

    Sleep(INTERVAL);
    HWND hwnd1 = FindWindowW(L"user32_apitest_menuui", L"#1");
    HWND hwnd2 = FindWindowW(L"user32_apitest_menuui", L"#2");
    trace("hwnd1: %p\n", hwnd1);
    trace("hwnd2: %p\n", hwnd2);
    ok(hwnd != NULL, "hwnd was NULL\n");
    ok(hwnd1 != NULL, "hwnd1 was NULL\n");
    ok(hwnd2 != NULL, "hwnd2 was NULL\n");
    ok(hwnd1 != hwnd2, "hwnd1 == hwnd2\n");

    DWORD dwTID1 = GetWindowThreadProcessId(hwnd1, NULL);
    DWORD dwTID2 = GetWindowThreadProcessId(hwnd2, NULL);

    RECT rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
    INT cxWork = (rcWork.right - rcWork.left);

    RECT rc1 = { rcWork.left, rcWork.top, rcWork.left + cxWork / 2, rcWork.bottom };
    RECT rc2 = { rcWork.left + cxWork / 2, rcWork.top, rcWork.right, rcWork.bottom };
    MoveWindow(hwnd1, rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE);
    MoveWindow(hwnd2, rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top, TRUE);

    POINT pt1 = CenterPoint(&rc1);
    POINT pt2 = CenterPoint(&rc2);

    // Right click on hwnd1
    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    Sleep(INTERVAL);

    HWND hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    HWND hwndFore, hwndActive, hwndFocus, hwndCapture;

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    // Right click on hwnd2
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    Sleep(INTERVAL);

    HWND hwndMenu2 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu2: %p\n", hwndMenu2);
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");
    ok(hwndMenu1 != hwndMenu2, "hwndMenu1 == hwndMenu2\n");

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd2, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd2, "hwndCapture was %p\n", hwndCapture);

    // Type Esc key
    AutoKey(AUTO_KEY_DOWN_UP, VK_ESCAPE);
    Sleep(INTERVAL);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd2, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndCapture was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndCapture was %p\n", hwndCapture);

    ok(GetHitID(hwnd2) == 0, "GetHitID(hwnd2) was %d\n", GetHitID(hwnd2));
    HWND hwndMenu0 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu0: %p\n", hwndMenu0);
    ok(!IsWindowVisible(hwndMenu0), "hwndMenu0 was visible\n");

    // Click on hwnd1
    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible");

    RECT rcMenu1;
    GetWindowRect(hwndMenu1, &rcMenu1);
    POINT ptMenu1 = CenterPoint(&rcMenu1); // Separator

    // Clicking on separator is not effective
    AutoClick(AUTO_LEFT_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    AutoClick(AUTO_LEFT_DOUBLE_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    AutoClick(AUTO_RIGHT_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    AutoClick(AUTO_RIGHT_DOUBLE_CLICK, ptMenu1.x, ptMenu1.y);
    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    trace("hwndMenu1: %p\n", hwndMenu1);
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    // Click on first item of hwnd1 context menu
    POINT pt1_3 = { ptMenu1.x, (2 * rcMenu1.top + 1 * rcMenu1.bottom) / (1 + 2) }; // First item
    AutoClick(AUTO_LEFT_CLICK, pt1_3.x, pt1_3.y);
    Sleep(INTERVAL);

    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    ok(!IsWindowVisible(hwndMenu1), "hwndMenu1 was visible\n");
    ok(GetHitID(hwnd1) == MENUID_100, "GetHitID(hwnd1) was %d\n", GetHitID(hwnd1));

    // Click on hwnd2
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");

    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN); // Down key
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    Sleep(INTERVAL);

    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(!IsWindowVisible(hwndMenu2), "hwndMenu2 was visible");
    ok(GetHitID(hwnd2) == MENUID_100, "GetHitID(hwnd2) was %d\n", GetHitID(hwnd2));

    // Click on hwnd2
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");

    AutoKey(AUTO_KEY_DOWN_UP, VK_UP); // Up key
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    Sleep(INTERVAL);

    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(!IsWindowVisible(hwndMenu2), "hwndMenu2 was visible");
    ok(GetHitID(hwnd2) == 101, "GetHitID(hwnd2) was %d\n", GetHitID(hwnd2));

    INT nMenuCount;

    // Shift + Right click on hwnd2
    AutoKey(AUTO_KEY_DOWN, VK_SHIFT);
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    AutoKey(AUTO_KEY_UP, VK_SHIFT);
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");
    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_UP); // Up key
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");

    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");
    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 2, "nMenuCount was %d\n", nMenuCount);

    // Right click on hwnd1
    AutoClick(AUTO_RIGHT_CLICK, pt1.x, pt1.y);
    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN); // Down key
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    Sleep(INTERVAL);

    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    ok(!IsWindowVisible(hwndMenu1), "hwndMenu1 was visible\n");

    // Getting menu bar info
    MENUBARINFO mbi = { sizeof(mbi) };
    GetMenuBarInfo(hwnd1, OBJID_MENU, 0, &mbi);
    INT xMenuBar1 = mbi.rcBar.left + 16;
    INT yMenuBar1 = (mbi.rcBar.top + mbi.rcBar.bottom) / 2;
    GetMenuBarInfo(hwnd2, OBJID_MENU, 0, &mbi);
    INT xMenuBar2 = mbi.rcBar.left + 16;
    INT yMenuBar2 = (mbi.rcBar.top + mbi.rcBar.bottom) / 2;

    // Click on menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    Sleep(INTERVAL);

    hwndMenu1 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu1), "hwndMenu1 not visible\n");

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    // Right click on hwnd2
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    Sleep(INTERVAL);

    ok(!IsWindowVisible(hwndMenu1), "hwndMenu1 was visible\n");
    hwndMenu2 = FindWindowW(MENUCLASS, L"");
    ok(IsWindowVisible(hwndMenu2), "hwndMenu2 not visible\n");
    ok(hwndMenu1 != hwndMenu2, "hwndMenu1 == hwndMenu2\n");

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd2, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd2, "hwndFocus was %p\n", hwndCapture);

    // Click on hwnd1 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    Sleep(INTERVAL);

    ok(!IsWindowVisible(hwndMenu2), "hwndMenu2 was visible\n");
    hwndMenu1 = FindWindowW(MENUCLASS, L"");

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    // Shift + Right click on hwnd2
    AutoKey(AUTO_KEY_DOWN, VK_SHIFT);
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    AutoKey(AUTO_KEY_UP, VK_SHIFT);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd2, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd2, "hwndFocus was %p\n", hwndCapture);

    ok(!IsWindowVisible(hwndMenu1), "hwndMenu1 was visible\n");
    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    ok(GetHitID(hwnd2) == 0, "GetHitID(hwnd2) was %d\n", GetHitID(hwnd2));
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);
    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN);
    ok(GetHitID(hwnd2) == MENUID_101, "GetHitID(hwnd2) was %d\n", GetHitID(hwnd2));

    // Shift + Right click on hwnd2
    AutoKey(AUTO_KEY_DOWN, VK_SHIFT);
    AutoClick(AUTO_RIGHT_CLICK, pt2.x, pt2.y);
    AutoKey(AUTO_KEY_UP, VK_SHIFT);
    Sleep(INTERVAL);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_UP); // Up key
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    Sleep(INTERVAL);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 2, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_LEFT); // Left key
    Sleep(INTERVAL);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_RIGHT); // Right key
    Sleep(INTERVAL);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 2, "nMenuCount was %d\n", nMenuCount);

    // Left click on hwnd1 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    Sleep(INTERVAL);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN); // Down key
    AutoKey(AUTO_KEY_DOWN_UP, VK_RETURN); // Enter key
    Sleep(INTERVAL);

    ok(GetHitID(hwnd1) == MENUID_200, "GetHitID(hwnd1) was %d\n", GetHitID(hwnd1));

    // Left click on hwnd1 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    AutoKey(AUTO_KEY_DOWN_UP, VK_DOWN);
    AutoKey(AUTO_KEY_DOWN_UP, VK_ESCAPE);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    Sleep(INTERVAL);
    ok(GetHitID(hwnd1) == 0, "GetHitID(hwnd1) was %d\n", GetHitID(hwnd1));

    // Left click on hwnd1 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    Sleep(INTERVAL);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    // Left click on hwnd2 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar2, yMenuBar2);
    Sleep(INTERVAL);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd2, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd2, "hwndFocus was %p\n", hwndCapture);

    // Left click on hwnd1 menu bar
    AutoClick(AUTO_LEFT_CLICK, xMenuBar1, yMenuBar1);
    Sleep(INTERVAL);

    hwndFore = GetForegroundWindow();
    ok(hwndFore == hwnd1, "hwndFore was %p\n", hwndFore);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndCapture was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    // Get title bar info
    TITLEBARINFO titleInfo = { sizeof(titleInfo) };
    RECT rcTitleBar1, rcTitleBar2;
    GetTitleBarInfo(hwnd1, &titleInfo);
    rcTitleBar1 = titleInfo.rcTitleBar;
    GetTitleBarInfo(hwnd2, &titleInfo);
    rcTitleBar2 = titleInfo.rcTitleBar;
    POINT ptTitleBar1 = CenterPoint(&rcTitleBar1);
    POINT ptTitleBar2 = CenterPoint(&rcTitleBar2);

    // Right click on hwnd2 title bar
    AutoClick(AUTO_RIGHT_CLICK, ptTitleBar2.x, ptTitleBar2.y);
    Sleep(INTERVAL);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(hwndActive == hwnd2, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd2, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd2, "hwndFocus was %p\n", hwndCapture);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    // Right click on hwnd1 title bar
    AutoClick(AUTO_RIGHT_CLICK, ptTitleBar1.x, ptTitleBar1.y);
    Sleep(INTERVAL);

    hwndActive = GetThreadActiveWnd(dwTID1);
    hwndFocus = GetThreadFocus(dwTID1);
    hwndCapture = GetThreadCapture(dwTID1);
    ok(hwndActive == hwnd1, "hwndActive was %p\n", hwndActive);
    ok(hwndFocus == hwnd1, "hwndFocus was %p\n", hwndFocus);
    ok(hwndCapture == hwnd1, "hwndFocus was %p\n", hwndCapture);

    hwndActive = GetThreadActiveWnd(dwTID2);
    hwndFocus = GetThreadFocus(dwTID2);
    hwndCapture = GetThreadCapture(dwTID2);
    ok(!hwndActive, "hwndActive was %p\n", hwndActive);
    ok(!hwndFocus, "hwndFocus was %p\n", hwndFocus);
    ok(!hwndCapture, "hwndFocus was %p\n", hwndCapture);

    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 1, "nMenuCount was %d\n", nMenuCount);

    // Left double click on hwnd1 system menu icon (it will close hwnd1)
    POINT ptSysMenu = { rcTitleBar1.left - GetSystemMetrics(SM_CXSMICON) / 2, ptTitleBar1.y };
    ok(IsWindowVisible(hwnd1), "hwnd1 not visible\n");
    AutoClick(AUTO_LEFT_DOUBLE_CLICK, ptSysMenu.x, ptSysMenu.y);
    Sleep(INTERVAL);

    ok(!IsWindowVisible(hwnd1), "hwnd1 was visible\n");
    nMenuCount = CountMenuWnds();
    ok(nMenuCount == 0, "nMenuCount was %d\n", nMenuCount);

    PostMessageW(hwnd1, WM_CLOSE, 0, 0);
    PostMessageW(hwnd2, WM_CLOSE, 0, 0);

    ShowWindow(hwnd, SW_RESTORE);
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return 0;
}

static INT
OnCreate(HWND hwnd)
{
    s_hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
    if (!s_hThread)
        return -1;
    return 0;
}

static VOID
OnDestroy(HWND hwnd)
{
    if (s_hThread)
    {
        CloseHandle(s_hThread);
        s_hThread = NULL;
    }

    PostQuitMessage(0);
}

static
LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return OnCreate(hwnd);
        case WM_DESTROY:
            OnDestroy(hwnd);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static VOID
TEST_MenuUI(VOID)
{
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSW wc = { CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, WindowProc };
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASSNAME;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    HWND hwnd = CreateWindowW(CLASSNAME, L"MenuUITest", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                              NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed\n");
        return;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

START_TEST(MenuUI)
{
    if (GetFileAttributesW(SUB_PROGRAM) == INVALID_FILE_ATTRIBUTES)
    {
        skip("'%ls' not found\n", SUB_PROGRAM);
        return;
    }

    HINSTANCE hShell32 = LoadLibraryW(L"shell32.dll");
    s_pShellExecuteExW = (FN_ShellExecuteExW)GetProcAddress(hShell32, "ShellExecuteExW");
    if (!s_pShellExecuteExW)
    {
        skip("ShellExecuteExW not found\n");
        FreeLibrary(hShell32);
        return;
    }

    TEST_MenuUI();

    CloseSubPrograms();

    FreeLibrary(hShell32);
}
