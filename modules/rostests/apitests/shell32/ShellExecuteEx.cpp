/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteEx
 * PROGRAMMER:      Yaroslav Veremenko <yaroslav@veremenko.info>
 */

#include "shelltest.h"

#define ok_ShellExecuteEx (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : TestShellExecuteEx

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

START_TEST(ShellExecuteEx)
{
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
}
