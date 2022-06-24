/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteEx
 * PROGRAMMER:      Yaroslav Veremenko <yaroslav@veremenko.info>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <shlwapi.h>
#include <stdio.h>
#include "shell32_apitest_sub.h"

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

static void DoAppPathTest(void)
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

typedef struct TEST_ENTRY
{
    INT lineno;
    BOOL ret;
    BOOL bProcessHandle;
    LPCSTR file;
    LPCSTR params;
    LPCSTR curdir;
} TEST_ENTRY;

static char s_sub_program[MAX_PATH];
static char s_win_test_exe[MAX_PATH];
static char s_sys_test_exe[MAX_PATH];
static char s_win_bat_file[MAX_PATH];
static char s_sys_bat_file[MAX_PATH];
static char s_win_txt_file[MAX_PATH];
static char s_sys_txt_file[MAX_PATH];

#define DONT_CARE 0x0BADF00D

static const TEST_ENTRY s_entries_1[] =
{
    { __LINE__, TRUE, TRUE, "test program" },
    { __LINE__, TRUE, TRUE, "test program.bat" },
    { __LINE__, TRUE, TRUE, "test program.exe" },
    { __LINE__, FALSE, FALSE, "  test program" },
    { __LINE__, FALSE, FALSE, "  test program.bat" },
    { __LINE__, FALSE, FALSE, "  test program.exe" },
    { __LINE__, FALSE, FALSE, "test program  " },
    { __LINE__, TRUE, TRUE, "test program.bat  " },
    { __LINE__, TRUE, TRUE, "test program.exe  " },
    { __LINE__, TRUE, TRUE, "test program", "TEST" },
    { __LINE__, TRUE, TRUE, "test program.bat", "TEST" },
    { __LINE__, TRUE, TRUE, "test program.exe", "TEST" },
    { __LINE__, FALSE, FALSE, ".\\test program.bat" },
    { __LINE__, FALSE, FALSE, ".\\test program.exe" },
    { __LINE__, TRUE, TRUE, "\"test program\"" },
    { __LINE__, TRUE, TRUE, "\"test program.bat\"" },
    { __LINE__, TRUE, TRUE, "\"test program.exe\"" },
    { __LINE__, FALSE, FALSE, "\"test program\" TEST" },
    { __LINE__, FALSE, FALSE, "\"test program.bat\" TEST" },
    { __LINE__, FALSE, FALSE, "\"test program.exe\" TEST" },
    { __LINE__, FALSE, FALSE, "  \"test program\"" },
    { __LINE__, FALSE, FALSE, "  \"test program.bat\"" },
    { __LINE__, FALSE, FALSE, "  \"test program.exe\"" },
    { __LINE__, FALSE, FALSE, "\"test program\"  " },
    { __LINE__, FALSE, FALSE, "\"test program.bat\"  " },
    { __LINE__, FALSE, FALSE, "\"test program.exe\"  " },
    { __LINE__, FALSE, FALSE, "\".\\test program.bat\"" },
    { __LINE__, FALSE, FALSE, "\".\\test program.exe\"" },
    { __LINE__, TRUE, TRUE, s_win_test_exe },
    { __LINE__, TRUE, TRUE, s_sys_test_exe },
    { __LINE__, TRUE, TRUE, s_win_bat_file },
    { __LINE__, TRUE, TRUE, s_sys_bat_file },
    { __LINE__, TRUE, TRUE, s_win_bat_file, "TEST" },
    { __LINE__, TRUE, TRUE, s_sys_bat_file, "TEST" },
    { __LINE__, FALSE, FALSE, "invalid program" },
    { __LINE__, FALSE, FALSE, "invalid program.bat" },
    { __LINE__, FALSE, FALSE, "invalid program.exe" },
    { __LINE__, TRUE, TRUE, "test_file.txt" },
    { __LINE__, TRUE, TRUE, "test_file.txt", "parameters parameters" },
    { __LINE__, TRUE, TRUE, "test_file.txt", "parameters parameters", "." },
    { __LINE__, TRUE, TRUE, "shell32_apitest_sub.exe" },
    { __LINE__, TRUE, TRUE, ".\\shell32_apitest_sub.exe" },
    { __LINE__, TRUE, TRUE, "\"shell32_apitest_sub.exe\"" },
    { __LINE__, TRUE, TRUE, "\".\\shell32_apitest_sub.exe\"" },
    { __LINE__, TRUE, DONT_CARE, "https://google.com" },
    { __LINE__, TRUE, FALSE, "::{450d8fba-ad25-11d0-98a8-0800361b1103}" },
    { __LINE__, TRUE, FALSE, "shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}" },
    { __LINE__, TRUE, FALSE, "shell:sendto" },
};

static const TEST_ENTRY s_entries_2[] =
{
    { __LINE__, TRUE, TRUE, "test program" },
    { __LINE__, TRUE, TRUE, "test program", "TEST" },
    { __LINE__, TRUE, TRUE, "\"test program\"" },
    { __LINE__, TRUE, TRUE, s_win_test_exe },
    { __LINE__, TRUE, TRUE, s_sys_test_exe },
    { __LINE__, FALSE, FALSE, s_win_bat_file },
    { __LINE__, FALSE, FALSE, s_sys_bat_file },
    { __LINE__, FALSE, FALSE, s_win_bat_file, "TEST" },
    { __LINE__, FALSE, FALSE, s_sys_bat_file, "TEST" },
};

typedef struct OPENWNDS
{
    UINT count;
    HWND *phwnd;
} OPENWNDS;

static OPENWNDS s_wi0 = { 0 }, s_wi1 = { 0 };

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    OPENWNDS *info = (OPENWNDS *)lParam;
    info->phwnd = (HWND *)realloc(info->phwnd, (info->count + 1) * sizeof(HWND));
    if (!info->phwnd)
        return FALSE;
    info->phwnd[info->count] = hwnd;
    ++(info->count);
    return TRUE;
}

static void CleanupNewlyCreatedWindows(void)
{
    EnumWindows(EnumWindowsProc, (LPARAM)&s_wi1);
    for (UINT i1 = 0; i1 < s_wi1.count; ++i1)
    {
        BOOL bFound = FALSE;
        for (UINT i0 = 0; i0 < s_wi0.count; ++i0)
        {
            if (s_wi1.phwnd[i1] == s_wi0.phwnd[i0])
            {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
            PostMessageW(s_wi1.phwnd[i1], WM_CLOSE, 0, 0);
    }
    free(s_wi1.phwnd);
    ZeroMemory(&s_wi1, sizeof(s_wi1));
}

static VOID DoTestEntry(const TEST_ENTRY *pEntry)
{
    SHELLEXECUTEINFOA info = { sizeof(info) };
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    info.nShow = SW_SHOWNORMAL;
    info.lpFile = pEntry->file;
    info.lpParameters = pEntry->params;
    info.lpDirectory = pEntry->curdir;
    BOOL ret = ShellExecuteExA(&info);
    ok(ret == pEntry->ret, "Line %u: ret expected %d, got %d\n",
       pEntry->lineno, pEntry->ret, ret);
    if (!pEntry->ret)
        return;

    if ((UINT)pEntry->bProcessHandle != DONT_CARE)
    {
        if (pEntry->bProcessHandle)
        {
            ok(!!info.hProcess, "Line %u: hProcess expected non-NULL\n", pEntry->lineno);
        }
        else
        {
            ok(!info.hProcess, "Line %u: hProcess expected NULL\n", pEntry->lineno);
            return;
        }
    }

    WaitForInputIdle(info.hProcess, INFINITE);

    CleanupNewlyCreatedWindows();

    if (WaitForSingleObject(info.hProcess, 10 * 1000) == WAIT_TIMEOUT)
    {
        TerminateProcess(info.hProcess, 11);
        ok(0, "Process %s did not quit!\n", pEntry->file);
    }
    CloseHandle(info.hProcess);
}

static BOOL
GetSubProgramPath(void)
{
    GetModuleFileNameA(NULL, s_sub_program, _countof(s_sub_program));
    PathRemoveFileSpecA(s_sub_program);
    PathAppendA(s_sub_program, "shell32_apitest_sub.exe");

    if (!PathFileExistsA(s_sub_program))
    {
        PathRemoveFileSpecA(s_sub_program);
        PathAppendA(s_sub_program, "testdata\\shell32_apitest_sub.exe");

        if (!PathFileExistsA(s_sub_program))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void DoTestEntries(void)
{
    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe is not found\n");
        return;
    }

    // s_win_test_exe
    GetWindowsDirectoryA(s_win_test_exe, _countof(s_win_test_exe));
    PathAppendA(s_win_test_exe, "test program.exe");
    BOOL ret = CopyFileA(s_sub_program, s_win_test_exe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        return;
    }

    // record open windows
    if (!EnumWindows(EnumWindowsProc, (LPARAM)&s_wi0))
    {
        skip("EnumWindows failed\n");
        DeleteFileA(s_win_test_exe);
        free(s_wi0.phwnd);
        return;
    }

    // s_sys_test_exe
    GetSystemDirectoryA(s_sys_test_exe, _countof(s_sys_test_exe));
    PathAppendA(s_sys_test_exe, "test program.exe");
    ok_int(CopyFileA(s_sub_program, s_sys_test_exe, FALSE), TRUE);

    // s_win_bat_file
    GetWindowsDirectoryA(s_win_bat_file, _countof(s_win_bat_file));
    PathAppendA(s_win_bat_file, "test program.bat");
    FILE *fp = fopen(s_win_bat_file, "wb");
    fprintf(fp, "exit /b 3");
    fclose(fp);
    ok_int(PathFileExistsA(s_win_bat_file), TRUE);

    // s_sys_bat_file
    GetSystemDirectoryA(s_sys_bat_file, _countof(s_sys_bat_file));
    PathAppendA(s_sys_bat_file, "test program.bat");
    fp = fopen(s_sys_bat_file, "wb");
    fprintf(fp, "exit /b 4");
    fclose(fp);
    ok_int(PathFileExistsA(s_sys_bat_file), TRUE);

    // s_win_txt_file
    GetWindowsDirectoryA(s_win_txt_file, _countof(s_win_txt_file));
    PathAppendA(s_win_txt_file, "test_file.txt");
    fp = fopen(s_win_txt_file, "wb");
    fclose(fp);
    ok_int(PathFileExistsA(s_win_txt_file), TRUE);

    // s_sys_txt_file
    GetSystemDirectoryA(s_sys_txt_file, _countof(s_sys_txt_file));
    PathAppendA(s_sys_txt_file, "test_file.txt");
    fp = fopen(s_sys_txt_file, "wb");
    fclose(fp);
    ok_int(PathFileExistsA(s_sys_txt_file), TRUE);

    for (UINT iTest = 0; iTest < _countof(s_entries_1); ++iTest)
    {
        DoTestEntry(&s_entries_1[iTest]);
    }

    DeleteFileA(s_win_bat_file);
    DeleteFileA(s_sys_bat_file);

    for (UINT iTest = 0; iTest < _countof(s_entries_2); ++iTest)
    {
        DoTestEntry(&s_entries_2[iTest]);
    }

    DeleteFileA(s_win_test_exe);
    DeleteFileA(s_sys_test_exe);
    DeleteFileA(s_win_txt_file);
    DeleteFileA(s_sys_txt_file);

    free(s_wi0.phwnd);
}

WCHAR* ExeName = NULL;

BOOL CALLBACK EnumProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() &&
        IsWindowVisible(hwnd))
    {
        WCHAR Buffer[512] = {0};

        GetWindowTextW(hwnd, Buffer, _countof(Buffer) - 1);
        if (Buffer[0] && StrStrIW(Buffer, ExeName))
        {
            HWND* pHwnd = (HWND*)lParam;
            *pHwnd = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

BOOL WaitAndCloseWindow()
{
    HWND hWnd = NULL;
    for (int n = 0; n < 100; ++n)
    {
        Sleep(50);

        EnumWindows(EnumProc, (LPARAM)&hWnd);

        if (hWnd)
        {
            SendMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            return TRUE;
            break;
        }
    }
    return FALSE;
}

static void test_properties()
{
    WCHAR Buffer[MAX_PATH * 4];

    CoInitialize(NULL);

    GetModuleFileNameW(NULL, Buffer, _countof(Buffer));
    SHELLEXECUTEINFOW info = { 0 };

    info.cbSize = sizeof(SHELLEXECUTEINFOW);
    info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"properties";
    info.lpFile = Buffer;
    info.lpParameters = L"";
    info.nShow = SW_SHOW;

    BOOL bRet = ShellExecuteExW(&info);
    ok(bRet, "Failed! (GetLastError(): %d)\n", (int)GetLastError());
    ok_ptr(info.hInstApp, (HINSTANCE)42);

    ExeName = PathFindFileNameW(Buffer);
    WCHAR* Extension = PathFindExtensionW(Buffer);
    if (Extension)
    {
        // The inclusion of this depends on the file display settings!
        *Extension = UNICODE_NULL;
    }

    if (bRet)
    {
        ok(WaitAndCloseWindow(), "Could not find properties window!\n");
    }

    // Now retry it with the extension cut off
    bRet = ShellExecuteExW(&info);
    ok(bRet, "Failed! (GetLastError(): %d)\n", (int)GetLastError());
    ok_ptr(info.hInstApp, (HINSTANCE)42);

    if (bRet)
    {
        ok(WaitAndCloseWindow(), "Could not find properties window!\n");
    }

    info.lpFile = L"complete garbage, cannot run this!";

    // Now retry it with complete garabage
    bRet = ShellExecuteExW(&info);
    ok(bRet == 0, "Succeeded!\n");
    ok_ptr(info.hInstApp, (HINSTANCE)2);
}

START_TEST(ShellExecuteEx)
{
    DoAppPathTest();
    DoTestEntries();
    test_properties();

    DoWaitForWindow(CLASSNAME, CLASSNAME, TRUE, TRUE);
    Sleep(100);
}
