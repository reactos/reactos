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
#include "closewnd.h"

#define WAIT_SLEEP 700

// ShellExecuteW(handle, "open", <path_to_executable>, <parameters>, NULL, SW_SHOWNORMAL);

static WINDOW_LIST s_List1, s_List2;

START_TEST(ShellExecuteW)
{
    INT ret;
    HINSTANCE hInstance;
    WCHAR WinDir[MAX_PATH], SysDir[MAX_PATH], SysDrive[MAX_PATH];

    GetWindowList(&s_List1);

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

    // TEST #2: Open Notepad
    hInstance = ShellExecuteW(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #2: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #2 ret: %d.\n", ret);

    // TEST #3: Open Windows folder
    hInstance = ShellExecuteW(NULL, NULL, WinDir, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #3: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #3 ret: %d.\n", ret);

    // TEST #4: Open system32 folder
    hInstance = ShellExecuteW(NULL, L"open", SysDir, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #4: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #4 ret: %d.\n", ret);

    // TEST #5: Open %SystemDrive%
    hInstance = ShellExecuteW(NULL, L"explore", SysDrive, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #5: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #5 ret: %d.\n", ret);

    // TEST #6: Open Explorer Search on %SYSTEMDRIVE%
    hInstance = ShellExecuteW(NULL, L"find", SysDrive, NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #6: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #6 ret: %d.\n", ret);

    // TEST #7: Open My Documents ("::{450d8fba-ad25-11d0-98a8-0800361b1103}")
    hInstance = ShellExecuteW(NULL, NULL, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #7: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #7 ret: %d.\n", ret);

    // TEST #8: Open My Documents ("shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}")
    hInstance = ShellExecuteW(NULL, L"open", L"shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL, NULL, SW_SHOWNORMAL);
    ret = (INT)(UINT_PTR)hInstance;
    ok(ret > 31, "TEST #8: ret:%d, LastError: %ld\n", ret, GetLastError());
    trace("TEST #8 ret: %d.\n", ret);

    // Execution can be asynchronous; you have to wait for it to finish.
    Sleep(2000);

    // Close newly-opened window(s)
    GetWindowList(&s_List2);
    CloseNewWindows(&s_List1, &s_List2);
    FreeWindowList(&s_List1);
    FreeWindowList(&s_List2);
}

// Windows Server 2003 and Windows XP SP3 return values (Win 7 returns 42 in all cases)
// ShellExecuteW(NULL, L"open",    L"rundll32.exe", L"shell32.dll,Control_RunDLL desk.cpl", NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, L"open",    L"notepad.exe",  NULL,                                   NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, NULL,       WinDir,          NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"open",    SysDir,          NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"explore", SysDrive,        NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"find",    SysDrive,        NULL,                                   NULL, SW_SHOWNORMAL) = 33
