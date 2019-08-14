/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteEx
 * PROGRAMMERS:     Yaroslav Veremenko <yaroslav@veremenko.info>
 *                  Doug Lyons <douglyons@douglyons.com>
 */

#include "shelltest.h"
#include<stdio.h>

#define ok_ShellExecuteEx (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : TestShellExecuteEx
#define ok_ShellExecuteEx1 (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : TestShellExecuteEx1


/* returns TRUE on success and FALSE on failure */
/* Returns the OS Vendor String in VendorString if found */
BOOL GetOSVendor(char* VendorString, int length)
{
    CHAR file_name[25] = {"OperatingSystem.txt"};
    CHAR cmdline[100] = {"ver > "};
    FILE *fp;
    CHAR OSVendor[100] = {0};
    CHAR *myreturn;
    INT pos;
    char *space_ptr;
    INT iResult;

    if (VendorString && (length >= 20))
        strcpy(VendorString, "Unknown");
    else
        return FALSE;

    fp = fopen(file_name, "r"); // read mode

    if (fp != NULL)
        return FALSE;

    strcat(cmdline, file_name);

    system(cmdline);

    fp = fopen(file_name, "r"); // read mode

    if (fp == NULL)
        return FALSE;

    /* first we must read past the <CR><LF> at the beginning of the file */
    myreturn = fgets(OSVendor , 100 , fp);

    if (myreturn == NULL)
        return FALSE;

    /* Now we can read the first actual text line */
    myreturn = fgets(OSVendor , 100 , fp);

    if (myreturn == NULL)
        return FALSE;

    space_ptr = strchr(OSVendor, ' ');
    pos = -1;
    if (space_ptr != NULL)
        {
            pos = space_ptr - OSVendor;
        }

    /* move zero into previous first space character position */
    if (OSVendor[pos] == 32) OSVendor[pos]=0;

    iResult = fclose(fp);

    if (iResult != 0)
        return FALSE;

    iResult = remove(file_name);

    if (iResult != 0)
        return FALSE;

   /* copy our result back to the calling function */
   strcpy(VendorString, OSVendor);

   return TRUE;
}


static
BOOL
CreateAppPathRegKey(const WCHAR* Name)
{
    HKEY RegistryKey;
    LONG Result;
    WCHAR Buffer[1024];
    WCHAR KeyValue[1024];
    DWORD Length = sizeof(KeyValue);
    DWORD Disposition;

    wcscpy(Buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(Buffer, L"IEXPLORE.EXE");
    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, Buffer, 0, KEY_READ, &RegistryKey);
    if (Result != ERROR_SUCCESS) trace("Could not open iexplore.exe key. Status: %lu\n", Result);
    if (Result) goto end;
    Result = RegQueryValueExW(RegistryKey, NULL, NULL, NULL, (LPBYTE)KeyValue, &Length);
    if (Result != ERROR_SUCCESS) trace("Could not read iexplore.exe key. Status: %lu\n", Result);
    if (Result) goto end;
    RegCloseKey(RegistryKey);

    wcscpy(Buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(Buffer, Name);
    Result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Buffer, 0, NULL,
        0, KEY_WRITE, NULL, &RegistryKey, &Disposition);
    if (Result != ERROR_SUCCESS) trace("Could not create test key. Status: %lu\n", Result);
    if (Result) goto end;
    Result = RegSetValueW(RegistryKey, NULL, REG_SZ, KeyValue, 0);
    if (Result != ERROR_SUCCESS) trace("Could not set value of the test key. Status: %lu\n", Result);
    if (Result) goto end;
    RegCloseKey(RegistryKey);
end:
    if (RegistryKey) RegCloseKey(RegistryKey);
    return Result == ERROR_SUCCESS;
}

static
VOID
DeleteAppPathRegKey(const WCHAR* Name)
{
    LONG Result;
    WCHAR Buffer[1024];
    wcscpy(Buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(Buffer, Name);
    Result = RegDeleteKeyW(HKEY_LOCAL_MACHINE, Buffer);
    if (Result != ERROR_SUCCESS) trace("Could not remove the test key. Status: %lu\n", Result);
}

static
VOID
TestShellExecuteEx(const WCHAR* Name, BOOL ExpectedResult)
{
    SHELLEXECUTEINFOW ShellExecInfo;
    BOOL Result;
    ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
    ShellExecInfo.cbSize = sizeof(ShellExecInfo);
    ShellExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    ShellExecInfo.hwnd = NULL;
    ShellExecInfo.nShow = SW_SHOWNORMAL;
    ShellExecInfo.lpFile = Name;
    ShellExecInfo.lpDirectory = NULL;
    Result = ShellExecuteExW(&ShellExecInfo);
    ok(Result == ExpectedResult, "ShellExecuteEx lpFile %s failed. Error: %lu\n", wine_dbgstr_w(Name), GetLastError());
    if (ShellExecInfo.hProcess) 
    {
        Result = TerminateProcess(ShellExecInfo.hProcess, 0);
        if (!Result) trace("Terminate process failed. Error: %lu\n", GetLastError());
        WaitForSingleObject(ShellExecInfo.hProcess, INFINITE);
        CloseHandle(ShellExecInfo.hProcess);
    }
}

static
VOID
TestShellExecuteEx1(const WCHAR* Name, const WCHAR* Params)
{
    SHELLEXECUTEINFOW ShellExecInfo;
    BOOL Result;
    CHAR OSVendor[100] = {0};
    INT iResult;
    UINT_PTR retval = SE_ERR_NOASSOC;

    ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
    ShellExecInfo.cbSize = sizeof(ShellExecInfo);
    ShellExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    ShellExecInfo.hwnd = NULL;
    ShellExecInfo.nShow = SW_SHOWNORMAL;
    ShellExecInfo.lpFile = Name;
    ShellExecInfo.lpParameters = Params;
    ShellExecInfo.lpDirectory = NULL;
    ShellExecInfo.lpVerb = L"open";

    iResult = GetOSVendor(OSVendor, sizeof(OSVendor));
    printf("OSVendor is '%s'.\n", OSVendor);

    if (strcmp(OSVendor, "Microsoft") == 0)
        skip("Test does not work on MS Windows.\n");

    if (iResult == FALSE)
        skip("Unable to determine Operating System.\n");

    Result = ShellExecuteExW(&ShellExecInfo);

    ok(Result, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                wine_dbgstr_w(Name), GetLastError());

    if (strcmp(OSVendor, "Microsoft") == 0)
        skip("Test does not work on MS Windows.\n");

    if (iResult == FALSE)
        skip("Unable to determine Operating System.\n");

    if (Result)
    {
        retval = (UINT_PTR) ShellExecInfo.hInstApp;
        ok(retval > 31, "ShellExecuteEx lpFile %s failed. Error: %lu\n",
                                  wine_dbgstr_w(Name), GetLastError());
    }

    if (ShellExecInfo.hProcess) 
    {
        Result = TerminateProcess(ShellExecInfo.hProcess, 0);
        if (!Result) trace("Terminate process failed. Error: %lu\n", GetLastError());
        WaitForSingleObject(ShellExecInfo.hProcess, INFINITE);
        CloseHandle(ShellExecInfo.hProcess);
    }

}

START_TEST(ShellExecuteEx)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ok_ShellExecuteEx(L"iexplore", TRUE);
    ok_ShellExecuteEx(L"iexplore.exe", TRUE);

    if (CreateAppPathRegKey(L"iexplore.bat"))
    {
        ok_ShellExecuteEx(L"iexplore.bat", TRUE);
        ok_ShellExecuteEx(L"iexplore.bat.exe", FALSE);
        DeleteAppPathRegKey(L"iexplore.bat");
    }

    if (CreateAppPathRegKey(L"iexplore.bat.exe"))
    {
        ok_ShellExecuteEx(L"iexplore.bat", FALSE);
        ok_ShellExecuteEx(L"iexplore.bat.exe", TRUE);
        DeleteAppPathRegKey(L"iexplore.bat.exe");
    }

    ok_ShellExecuteEx1(L"rundll32.exe", L"shell32.dll,Control_RunDLL desk.cpl");
}
