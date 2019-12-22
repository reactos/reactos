/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteW
 * PROGRAMMER:      Doug Lyons <douglyons@douglyons.com>
 */

#include "shelltest.h"
#include <stdio.h>
#include <winbase.h>

/*
* Returns TRUE on success and FALSE on failure 
* On success returns the OS Vendor String in VendorString
*/
BOOL GetOSVendor(char* VendorString)
{
    CHAR file_name[25] = {"OperatingSystem.txt"};
    CHAR cmdline[100] = {"ver > "};
    FILE *fp;
    CHAR OSVendor[100] = {0};
    CHAR *myreturn;
    INT pos;
    char *space_ptr;
    INT iResult;

    strcpy(VendorString, "Unknown"); // Initialize bad return value

    fp = fopen(file_name, "r"); // read mode

    if(fp != NULL)
        return FALSE;

    strcat(cmdline, file_name);

    system(cmdline);

    fp = fopen(file_name, "r"); // read mode

    if(fp == NULL)
        return FALSE;

    /* first we must read past the <CR><LF> at the beginning of the file */
    myreturn = fgets(OSVendor , 100 , fp);

    if(myreturn == NULL)
        return FALSE;

    /* Now we can read the first actual text line */
    myreturn = fgets(OSVendor , 100 , fp);

    if(myreturn == NULL)
        return FALSE;
      
    /* diagnostic only */
//    printf("OSVendor is '%s'.\n", OSVendor);

    space_ptr = strchr(OSVendor, ' ');
    pos = -1;
    if (space_ptr != NULL)
        {
            pos = space_ptr - OSVendor;
        }

    /* diagnostic only */
//    printf("Space is at position '%d'.\n", pos);

    /* move zero into previous first space character position */
    if(OSVendor[pos] == 32) OSVendor[pos]=0;

    /* diagnostic only */
//    printf("The OS Vendor from command is '%s'.\n", OSVendor);

    iResult = fclose(fp);

    if(iResult != 0)
        return FALSE;

    iResult = remove(file_name);

    if(iResult != 0)
        return FALSE;

   /* copy our result back to the calling function */
   strcpy(VendorString, OSVendor);

   return TRUE;
}

static
VOID
TestShellExecuteW()
{
    HINSTANCE hInstance;
    INT retval;
    CHAR OSVendor[100] = {0};
    BOOL  IsWindows = FALSE;
    HWND hWnd;
    HWND hWnd1;
    const WCHAR *Name = L"ShellExecuteW";


    GetOSVendor(OSVendor);
    printf("OSVendor is '%s'.\n", OSVendor);

    if(strcmp(OSVendor, "Microsoft") == 0)
        IsWindows = TRUE;
        
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
        hWnd = GetDesktopWindow();
        Sleep(1000);
        hWnd1 = FindWindowExW(hWnd, NULL, NULL, lpWinTitle);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_QUIT, 0, 0);
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
        hWnd = GetDesktopWindow();
        Sleep(1000);
        hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_QUIT, 0, 0);
    }
// End of test #2 - Open notepad.exe


//  ShellExecuteW(handle, NULL, <fully_qualified_path_to_folder>, NULL, NULL, SW_SHOWNORMAL);

    if(strcmp(OSVendor, "Microsoft") == 0)
        hInstance = ShellExecuteW(NULL, NULL, L"C:\\Windows", NULL,
                                  NULL, SW_SHOWNORMAL);
    else
        hInstance = ShellExecuteW(NULL, NULL, L"C:\\ReactOS", NULL,
                                  NULL, SW_SHOWNORMAL);

    retval = (UINT_PTR) hInstance;

    printf("Return Value for Open (C:\\Windows | C:\\Reactos) is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance) 
    {
        LPCWSTR lpWinClass = L"CabinetWClass";
        LPCWSTR lpWinTitle1 = L"C:\\Windows";
        LPCWSTR lpWinTitle2 = L"C:\\ReactOS";

        hWnd = GetDesktopWindow();
        Sleep(1000);
        if (IsWindows)
            hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle1);
        else
            hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle2);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
    }
// End of test #3 - Open C:\Windows


//  ShellExecuteW(handle, "open", <fully_qualified_path_to_folder>, NULL, NULL, SW_SHOWNORMAL);

    if(strcmp(OSVendor, "Microsoft") == 0)
        hInstance = ShellExecuteW(NULL, L"open", L"C:\\Windows\\system32", NULL,
                                  NULL, SW_SHOWNORMAL);
    else
        hInstance = ShellExecuteW(NULL, L"open", L"C:\\ReactOS\\system32", NULL,
                                  NULL, SW_SHOWNORMAL);

    retval = (UINT_PTR) hInstance;

    printf("Return Value for C:\\...\\system32 is '%d'.\n", retval);

    ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                     wine_dbgstr_w(Name), GetLastError());

    if (hInstance)
    {
        LPCWSTR lpWinClass = L"CabinetWClass";
        LPCWSTR lpWinTitle = L"C:\\Windows\\system32";
        hWnd = GetDesktopWindow();
        Sleep(1000);
        hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
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
        hWnd = GetDesktopWindow();
        Sleep(1000);
        hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
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
        hWnd = GetDesktopWindow();
        Sleep(1000);
        hWnd1 = FindWindowExW(hWnd, NULL, lpWinClass, lpWinTitle);
        CloseWindow(hWnd1);
        PostMessage(hWnd1, WM_SYSCOMMAND, 0xF060, 0);  // Terminate Window
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
