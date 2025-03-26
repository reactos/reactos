/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteW
 * PROGRAMMERS:     Doug Lyons <douglyons@douglyons.com>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <stdio.h>
#include <winbase.h>
#include <shlwapi.h>

#define WAIT_SLEEP 700

// ShellExecuteW(handle, "open", <path_to_executable>, <parameters>, NULL, SW_SHOWNORMAL);

START_TEST(ShellExecuteW)
{
    INT ret;
    HINSTANCE hInstance;
    HWND hWnd;
    WCHAR WinDir[MAX_PATH], SysDir[MAX_PATH], SysDrive[MAX_PATH];

    if (!GetWindowsDirectoryW(WinDir, _countof(WinDir)))
    {
        skip("GetWindowsDirectoryW failed\n");
        return;
    }
    if (!GetSystemDirectoryW(SysDir, _countof(SysDir)))
    {
        skip("GetSystemDirectoryW failed\n");
        return;
    }
    if (!GetEnvironmentVariableW(L"SystemDrive", SysDrive, _countof(SysDrive)))
    {
        trace("GetEnvironmentVariableW('SystemDrive') failed\n");
        SysDrive[0] = SysDir[0];
        SysDrive[1] = L':';
        SysDrive[2] = 0;
    }
    PathAddBackslashW(SysDrive);

    // TEST #1: Open Control Panel
    hInstance = ShellExecuteW(NULL, L"open", L"rundll32.exe", L"shell32.dll,Control_RunDLL desk.cpl",
                              NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #1: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #1 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(NULL, L"Display Properties");
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #2: Open Notepad
    hInstance = ShellExecuteW(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #2: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #2 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"Notepad", L"Untitled - Notepad");
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #3: Open Windows folder
    hInstance = ShellExecuteW(NULL, NULL, WinDir, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #3: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #3 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"CabinetWClass", WinDir);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #4: Open system32 folder
    hInstance = ShellExecuteW(NULL, L"open", SysDir, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #4: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #4 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"CabinetWClass", SysDir);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #5: Open %SystemDrive%
    hInstance = ShellExecuteW(NULL, L"explore", SysDrive, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #5: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #5 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"ExploreWClass", SysDrive);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #6: Open Explorer Search on %SYSTEMDRIVE%
    hInstance = ShellExecuteW(NULL, L"find", SysDrive, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #6: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #6 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"CabinetWClass", L"Search Results");
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #7: Open My Documents ("::{450d8fba-ad25-11d0-98a8-0800361b1103}")
    hInstance = ShellExecuteW(NULL, NULL, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #7: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #7 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"CabinetWClass", NULL);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // TEST #8: Open My Documents ("shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}")
    hInstance = ShellExecuteW(NULL, L"open", L"shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #8: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #8 ret: %d.\n", ret);
    if (hInstance)
    {
        Sleep(WAIT_SLEEP);
        // Terminate Window
        hWnd = FindWindowW(L"CabinetWClass", NULL);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}

// Windows Server 2003 and Windows XP SP3 return values (Win 7 returns 42 in all cases)
// ShellExecuteW(NULL, L"open",    L"rundll32.exe", L"shell32.dll,Control_RunDLL desk.cpl", NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, L"open",    L"notepad.exe",  NULL,                                   NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, NULL,       WinDir,          NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"open",    SysDir,          NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"explore", SysDrive,        NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"find",    SysDrive,        NULL,                                   NULL, SW_SHOWNORMAL) = 33
