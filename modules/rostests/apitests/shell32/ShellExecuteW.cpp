/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteW
 * PROGRAMMER:      Doug Lyons <douglyons@douglyons.com>
 */

#include "shelltest.h"
#include <stdio.h>
#include <winbase.h>

static
VOID
TestShellExecuteW()
{
    HINSTANCE hInstance;
    INT retval;
    HWND hWnd;
    const WCHAR *Name = L"ShellExecuteW";
    WCHAR WindowsDirectory[MAX_PATH];
    BOOL IsReactOS;

    /* Check if we are running under ReactOS from the SystemRoot directory */
    if(!GetWindowsDirectoryW(WindowsDirectory, MAX_PATH))
        printf("GetWindowsDirectoryW failed\n");

    IsReactOS = !_wcsnicmp(&WindowsDirectory[3], L"reactos", 7);

    printf("OSVendor %s ReactOS.\n", IsReactOS ? "is" : "is not");

//  ShellExecuteW(handle, "open", <fully_qualified_path_to_executable>, <parameters>, NULL, SW_SHOWNORMAL);

    hInstance = ShellExecuteW(NULL, L"open", L"rundll32.exe", L"shell32.dll,Control_RunDLL desk.cpl",
                              NULL, SW_SHOWNORMAL);
    retval = (UINT_PTR) hInstance;

    printf("Return Value for Open Control Panel is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance) 
    {
        LPCWSTR lpWinTitle = L"Display Properties";
        Sleep(1000);
        hWnd = FindWindowW(NULL, lpWinTitle);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #1 - Open Control Panel


//  ShellExecuteW(handle, "open", <fully_qualified_path_to_executable>, NULL, NULL, SW_SHOWNORMAL);

    hInstance = ShellExecuteW(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWNORMAL);
    retval = (UINT_PTR) hInstance;

    printf("Return Value for Open notepad.exe is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance)
    {
        LPCWSTR lpWinClass = L"Notepad";
        LPCWSTR lpWinTitle = L"Untitled - Notepad";
        Sleep(1000);
        hWnd = FindWindowW(lpWinClass, lpWinTitle);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #2 - Open notepad.exe


//  ShellExecuteW(handle, NULL, <fully_qualified_path_to_folder>, NULL, NULL, SW_SHOWNORMAL);

    if(IsReactOS)
        hInstance = ShellExecuteW(NULL, NULL, L"C:\\ReactOS", NULL,
                                  NULL, SW_SHOWNORMAL);
    else
        hInstance = ShellExecuteW(NULL, NULL, L"C:\\Windows", NULL,
                                  NULL, SW_SHOWNORMAL);

    retval = (UINT_PTR) hInstance;

    printf("Return Value for Open %s is '%d'.\n", IsReactOS ? "C:\\ReactOS" : "C:\\Windows", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance) 
    {
        LPCWSTR lpWinClass = L"CabinetWClass";
        LPCWSTR lpWinTitleWindows = L"C:\\Windows";
        LPCWSTR lpWinTitleReactOS = L"C:\\ReactOS";

        Sleep(1000);
        if (IsReactOS)
            hWnd = FindWindowW(lpWinClass, lpWinTitleReactOS);
        else
            hWnd = FindWindowW(lpWinClass, lpWinTitleWindows);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #3 - Open C:\Windows


//  ShellExecuteW(handle, "open", <fully_qualified_path_to_folder>, NULL, NULL, SW_SHOWNORMAL);

    if(IsReactOS)
        hInstance = ShellExecuteW(NULL, L"open", L"C:\\ReactOS\\system32", NULL,
                                  NULL, SW_SHOWNORMAL);
    else
        hInstance = ShellExecuteW(NULL, L"open", L"C:\\Windows\\system32", NULL,
                                  NULL, SW_SHOWNORMAL);

    retval = (UINT_PTR) hInstance;

    printf("Return Value for C:\\...\\system32 is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance)
    {
        LPCWSTR lpWinClass = L"CabinetWClass";
        LPCWSTR lpWinTitle = L"C:\\Windows\\system32";
        Sleep(1000);
        hWnd = FindWindowW(lpWinClass, lpWinTitle);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #4 - Open C:\Windows\system32


//  ShellExecuteW(handle, "explore", <fully_qualified_path_to_folder>, NULL, NULL, SW_SHOWNORMAL);

    hInstance = ShellExecuteW(NULL, L"explore", L"C:\\", NULL, NULL, SW_SHOWNORMAL);
    retval = (UINT_PTR) hInstance;

    printf("Return Value for explore c:\\ is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance)
    {
        LPCWSTR lpWinClass = L"ExploreWClass";
        LPCWSTR lpWinTitle = L"C:\\";
        Sleep(1000);
        hWnd = FindWindowW(lpWinClass, lpWinTitle);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #5 - explore C:


//  ShellExecuteW(handle, "find", <fully_qualified_path_to_folder>, NULL, NULL, 0);

    hInstance = ShellExecuteW(NULL, L"find", L"C:\\", NULL, NULL, SW_SHOWNORMAL);
    retval = (UINT_PTR) hInstance;

    printf("Return Value for find is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance)
    {
        LPCWSTR lpWinClass = L"CabinetWClass";
        LPCWSTR lpWinTitle = L"Search Results";
        Sleep(1000);
        hWnd = FindWindowW(lpWinClass, lpWinTitle);
        PostMessage(hWnd, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #6 - find
}

START_TEST(ShellExecuteW)
{
    TestShellExecuteW();
}

// Windows Server 2003 and Windows XP SP3 return values (Win 7 returns 42 in all cases)
// ShellExecuteW(NULL, L"open",    L"rundll32.exe",          L"shell32.dll,Control_RunDLL desk.cpl", NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, L"open",    L"notepad.exe",           NULL,                                   NULL, SW_SHOWNORMAL) = 42
// ShellExecuteW(NULL, NULL,       L"C:\\Windows",           NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"open",    L"C:\\Windows\\system32", NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"explore", L"C:\\",                  NULL,                                   NULL, SW_SHOWNORMAL) = 33
// ShellExecuteW(NULL, L"find",    L"C:\\",                  NULL,                                   NULL, SW_SHOWNORMAL) = 33