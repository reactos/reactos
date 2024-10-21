/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Testing ShellExecuteEx
 * PROGRAMMER:      Yaroslav Veremenko <yaroslav@veremenko.info>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include "closewnd.h"
#include <pstypes.h>
#include <psfuncs.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>
#include <versionhelpers.h>
#include <shellutils.h>
#include "shell32_apitest_sub.h"

static WCHAR s_win_dir[MAX_PATH];
static WCHAR s_sys_dir[MAX_PATH];
static WCHAR s_win_notepad[MAX_PATH];
static WCHAR s_sys_notepad[MAX_PATH];
static WCHAR s_win_test_exe[MAX_PATH];
static WCHAR s_sys_test_exe[MAX_PATH];
static WCHAR s_win_bat_file[MAX_PATH];
static WCHAR s_sys_bat_file[MAX_PATH];
static WCHAR s_win_txt_file[MAX_PATH];
static WCHAR s_sys_txt_file[MAX_PATH];
static WCHAR s_win_notepad_cmdline[MAX_PATH];
static WCHAR s_sys_notepad_cmdline[MAX_PATH];
static WCHAR s_win_test_exe_cmdline[MAX_PATH];
static WCHAR s_sys_test_exe_cmdline[MAX_PATH];
static BOOL s_bWow64;

#define REG_APPPATHS L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\"

typedef enum TEST_RESULT
{
    TEST_FAILED,
    TEST_SUCCESS_NO_PROCESS,
    TEST_SUCCESS_WITH_PROCESS,
} TEST_RESULT;

typedef struct TEST_ENTRY
{
    INT line;
    TEST_RESULT result;
    LPCWSTR lpFile;
    LPCWSTR cmdline;
} TEST_ENTRY, *PTEST_ENTRY;

static void
TEST_DoTestEntry(INT line, TEST_RESULT result, LPCWSTR lpFile, LPCWSTR cmdline = NULL);

static void TEST_DoTestEntries(void)
{
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, NULL);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"");
    TEST_DoTestEntry(__LINE__, TEST_FAILED, L"This is an invalid path.");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_sys_bat_file, NULL);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_sys_test_exe, s_sys_test_exe_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_sys_txt_file, NULL);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_win_bat_file, NULL);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_win_notepad, s_win_notepad_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_win_test_exe, s_win_test_exe_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, s_win_txt_file, NULL);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"notepad", s_sys_notepad_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"notepad.exe", s_sys_notepad_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"\"notepad.exe\"", s_sys_notepad_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"\"notepad\"", s_sys_notepad_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"test program.exe", s_sys_test_exe_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"\"test program.exe\"", s_sys_test_exe_cmdline);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, s_win_dir);
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, s_sys_dir);
    TEST_DoTestEntry(__LINE__, TEST_FAILED, L"shell:ThisIsAnInvalidName");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"); // My Computer
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"); // My Computer (with shell:)

    if (!IsWindowsVistaOrGreater())
    {
        WCHAR szCurDir[MAX_PATH];
        GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
        SetCurrentDirectoryW(s_sys_dir);
        TEST_DoTestEntry(__LINE__, TEST_FAILED, L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}"); // Control Panel (without path)
        SetCurrentDirectoryW(szCurDir);
    }

    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"); // Control Panel (with path)
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"); // Control Panel (with path and shell:)
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:AppData");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Common Desktop");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Common Programs");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Common Start Menu");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Common StartUp");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:ControlPanelFolder");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Desktop");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Favorites");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Fonts");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Local AppData");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:My Pictures");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Personal");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Programs");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Recent");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:RecycleBinFolder");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:SendTo");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:Start Menu");
    TEST_DoTestEntry(__LINE__, TEST_SUCCESS_NO_PROCESS, L"shell:StartUp");
}

static LPWSTR
getCommandLineFromProcess(HANDLE hProcess)
{
    PEB peb;
    PROCESS_BASIC_INFORMATION info;
    RTL_USER_PROCESS_PARAMETERS Params;
    NTSTATUS Status;
    BOOL ret;

    Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ret = ReadProcessMemory(hProcess, info.PebBaseAddress, &peb, sizeof(peb), NULL);
    if (!ret)
        trace("ReadProcessMemory failed (%ld)\n", GetLastError());

    ReadProcessMemory(hProcess, peb.ProcessParameters, &Params, sizeof(Params), NULL);
    if (!ret)
        trace("ReadProcessMemory failed (%ld)\n", GetLastError());

    LPWSTR cmdline = Params.CommandLine.Buffer;
    if (!cmdline)
        trace("!cmdline\n");

    SIZE_T cbCmdLine = Params.CommandLine.Length;
    if (!cbCmdLine)
        trace("!cbCmdLine\n");

    LPWSTR pszBuffer = (LPWSTR)calloc(cbCmdLine + sizeof(WCHAR), 1);
    if (!pszBuffer)
        trace("!pszBuffer\n");

    ret = ReadProcessMemory(hProcess, cmdline, pszBuffer, cbCmdLine, NULL);
    if (!ret)
        trace("ReadProcessMemory failed (%ld)\n", GetLastError());

    pszBuffer[cbCmdLine / sizeof(WCHAR)] = UNICODE_NULL;

    return pszBuffer; // needs free()
}

static void TEST_DoTestEntryStruct(const TEST_ENTRY *pEntry)
{
    SHELLEXECUTEINFOW info = { sizeof(info) };
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_WAITFORINPUTIDLE |
                 SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
    info.hwnd = NULL;
    info.lpVerb = NULL;
    info.lpFile = pEntry->lpFile;
    info.nShow = SW_SHOWNORMAL;

    BOOL ret = ShellExecuteExW(&info);

    TEST_RESULT result;
    if (ret && info.hProcess)
        result = TEST_SUCCESS_WITH_PROCESS;
    else if (ret && !info.hProcess)
        result = TEST_SUCCESS_NO_PROCESS;
    else
        result = TEST_FAILED;

    ok(pEntry->result == result,
       "Line %d: result: %d vs %d\n", pEntry->line, pEntry->result, result);

    if (pEntry->result == TEST_SUCCESS_WITH_PROCESS && pEntry->cmdline && !s_bWow64)
    {
        LPWSTR cmdline = getCommandLineFromProcess(info.hProcess);
        if (!cmdline)
        {
            skip("!cmdline\n");
        }
        else
        {
            ok(lstrcmpiW(pEntry->cmdline, cmdline) == 0,
               "Line %d: cmdline: '%ls' vs '%ls'\n", pEntry->line,
               pEntry->cmdline, cmdline);
        }

        TerminateProcess(info.hProcess, 0xDEADFACE);
        free(cmdline);
    }

    CloseHandle(info.hProcess);
}

static void
TEST_DoTestEntry(INT line, TEST_RESULT result, LPCWSTR lpFile, LPCWSTR cmdline)
{
    TEST_ENTRY entry = { line, result, lpFile, cmdline };
    TEST_DoTestEntryStruct(&entry);
}

static BOOL
enableTokenPrivilege(LPCWSTR pszPrivilege)
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    TOKEN_PRIVILEGES tkp = { 0 };
    if (!LookupPrivilegeValueW(NULL, pszPrivilege, &tkp.Privileges[0].Luid))
        return FALSE;

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    return AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL);
}

static WINDOW_LIST s_List1, s_List2;

static BOOL TEST_Start(void)
{
    // Check Wow64
    s_bWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &s_bWow64);
    if (s_bWow64)
        skip("Wow64: Command Line check is skipped\n");

    // getCommandLineFromProcess needs this
    enableTokenPrivilege(SE_DEBUG_NAME);

    // s_win_dir
    GetWindowsDirectoryW(s_win_dir, _countof(s_win_dir));

    // s_sys_dir
    GetSystemDirectoryW(s_sys_dir, _countof(s_sys_dir));

    // s_win_notepad
    GetWindowsDirectoryW(s_win_notepad, _countof(s_win_notepad));
    PathAppendW(s_win_notepad, L"notepad.exe");

    // s_sys_notepad
    GetSystemDirectoryW(s_sys_notepad, _countof(s_sys_notepad));
    PathAppendW(s_sys_notepad, L"notepad.exe");

    // s_win_test_exe
    GetWindowsDirectoryW(s_win_test_exe, _countof(s_win_test_exe));
    PathAppendW(s_win_test_exe, L"test program.exe");
    BOOL ret = CopyFileW(s_win_notepad, s_win_test_exe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        return FALSE;
    }

    // s_sys_test_exe
    GetSystemDirectoryW(s_sys_test_exe, _countof(s_sys_test_exe));
    PathAppendW(s_sys_test_exe, L"test program.exe");
    ok_int(CopyFileW(s_win_notepad, s_sys_test_exe, FALSE), TRUE);

    // s_win_bat_file
    GetWindowsDirectoryW(s_win_bat_file, _countof(s_win_bat_file));
    PathAppendW(s_win_bat_file, L"test program.bat");
    FILE *fp = _wfopen(s_win_bat_file, L"wb");
    fprintf(fp, "exit /b 3");
    fclose(fp);
    ok_int(PathFileExistsW(s_win_bat_file), TRUE);

    // s_sys_bat_file
    GetSystemDirectoryW(s_sys_bat_file, _countof(s_sys_bat_file));
    PathAppendW(s_sys_bat_file, L"test program.bat");
    fp = _wfopen(s_sys_bat_file, L"wb");
    fprintf(fp, "exit /b 4");
    fclose(fp);
    ok_int(PathFileExistsW(s_sys_bat_file), TRUE);

    // s_win_txt_file
    GetWindowsDirectoryW(s_win_txt_file, _countof(s_win_txt_file));
    PathAppendW(s_win_txt_file, L"test_file.txt");
    fp = _wfopen(s_win_txt_file, L"wb");
    fclose(fp);
    ok_int(PathFileExistsW(s_win_txt_file), TRUE);

    // s_sys_txt_file
    GetSystemDirectoryW(s_sys_txt_file, _countof(s_sys_txt_file));
    PathAppendW(s_sys_txt_file, L"test_file.txt");
    fp = _wfopen(s_sys_txt_file, L"wb");
    fclose(fp);
    ok_int(PathFileExistsW(s_sys_txt_file), TRUE);

    // Check .txt settings
    WCHAR szPath[MAX_PATH];
    FindExecutableW(s_sys_txt_file, NULL, szPath);
    if (lstrcmpiW(PathFindFileNameW(szPath), L"notepad.exe") != 0)
    {
        skip("Please associate .txt with notepad.exe before tests\n");
        return FALSE;
    }

    // command lines
    StringCchPrintfW(s_win_notepad_cmdline, _countof(s_win_notepad_cmdline),
                     L"\"%s\" ", s_win_notepad);
    StringCchPrintfW(s_sys_notepad_cmdline, _countof(s_sys_notepad_cmdline),
                     L"\"%s\" ", s_sys_notepad);
    StringCchPrintfW(s_win_test_exe_cmdline, _countof(s_win_test_exe_cmdline),
                     L"\"%s\" ", s_win_test_exe);
    StringCchPrintfW(s_sys_test_exe_cmdline, _countof(s_sys_test_exe_cmdline),
                     L"\"%s\" ", s_sys_test_exe);

    GetWindowList(&s_List1);

    return TRUE;
}

static void TEST_End(void)
{
    GetWindowListForClose(&s_List2);
    CloseNewWindows(&s_List1, &s_List2);
    FreeWindowList(&s_List1);
    FreeWindowList(&s_List2);

    DeleteFileW(s_win_test_exe);
    DeleteFileW(s_sys_test_exe);
    DeleteFileW(s_win_txt_file);
    DeleteFileW(s_sys_txt_file);
    DeleteFileW(s_win_bat_file);
    DeleteFileW(s_sys_bat_file);
}

static void test_properties()
{
    HRESULT hrCoInit = CoInitialize(NULL);

    WCHAR Buffer[MAX_PATH * 4];
    GetModuleFileNameW(NULL, Buffer, _countof(Buffer));

    SHELLEXECUTEINFOW info = { sizeof(info) };
    info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"properties";
    info.lpFile = Buffer;
    info.nShow = SW_SHOW;

    BOOL bRet = ShellExecuteExW(&info);
    ok(bRet, "Failed! (GetLastError(): %d)\n", (int)GetLastError());
    ok_ptr(info.hInstApp, (HINSTANCE)42);

    WCHAR* Extension = PathFindExtensionW(Buffer);
    if (Extension)
    {
        // The inclusion of this depends on the file display settings!
        *Extension = UNICODE_NULL;
    }

    // Now retry it with the extension cut off
    bRet = ShellExecuteExW(&info);
    ok(bRet, "Failed! (GetLastError(): %d)\n", (int)GetLastError());
    ok_ptr(info.hInstApp, (HINSTANCE)42);

    // Now retry it with complete garabage
    info.lpFile = L"complete garbage, cannot run this!";
    bRet = ShellExecuteExW(&info);
    ok_int(bRet, 0);
    ok_ptr(info.hInstApp, (HINSTANCE)2);

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}

static void test_sei_lpIDList()
{
    // Note: SEE_MASK_FLAG_NO_UI prevents the test from blocking with a MessageBox
    WCHAR path[MAX_PATH];

    /* This tests ShellExecuteEx with lpIDList for explorer C:\ */
    GetSystemDirectoryW(path, _countof(path));
    PathStripToRootW(path);
    LPITEMIDLIST pidl = ILCreateFromPathW(path);
    if (!pidl)
    {
        skip("Unable to initialize test\n");
        return;
    }

    SHELLEXECUTEINFOW ShellExecInfo = { sizeof(ShellExecInfo) };
    ShellExecInfo.nShow = SW_SHOWNORMAL;
    ShellExecInfo.fMask = SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    ShellExecInfo.lpIDList = pidl;
    BOOL ret = ShellExecuteExW(&ShellExecInfo);
    ok_int(ret, TRUE);
    ILFree(pidl);

    /* This tests ShellExecuteEx with lpIDList going through IContextMenu */
    CCoInit ComInit;
    pidl = SHCloneSpecialIDList(NULL, CSIDL_PROFILE, TRUE);
    if (!pidl)
    {
        skip("Unable to initialize test\n");
        return;
    }
    ShellExecInfo.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    ShellExecInfo.lpIDList = pidl;
    ret = ShellExecuteExW(&ShellExecInfo);
    ok_int(ret, TRUE);
    ILFree(pidl);
}

static BOOL
CreateAppPath(LPCWSTR pszName, LPCWSTR pszValue)
{
    WCHAR szSubKey[MAX_PATH];
    StringCchPrintfW(szSubKey, _countof(szSubKey), L"%s\\%s", REG_APPPATHS, pszName);

    LSTATUS error;
    HKEY hKey;
    error = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, NULL, 0, KEY_WRITE, NULL,
                            &hKey, NULL);
    if (error != ERROR_SUCCESS)
        trace("Could not create test key (%lu)\n", error);

    DWORD cbValue = (lstrlenW(pszValue) + 1) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)pszValue, cbValue);
    if (error != ERROR_SUCCESS)
        trace("Could not set value of the test key (%lu)\n", error);

    RegCloseKey(hKey);

    return error == ERROR_SUCCESS;
}

static VOID
DeleteAppPath(LPCWSTR pszName)
{
    WCHAR szSubKey[MAX_PATH];
    StringCchPrintfW(szSubKey, _countof(szSubKey), L"%s\\%s", REG_APPPATHS, pszName);

    LSTATUS error = RegDeleteKeyW(HKEY_LOCAL_MACHINE, szSubKey);
    if (error != ERROR_SUCCESS)
        trace("Could not remove the test key (%lu)\n", error);
}

static void TEST_AppPath(void)
{
    if (CreateAppPath(L"app_path_test.bat", s_win_test_exe))
    {
        TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"app_path_test.bat");
        TEST_DoTestEntry(__LINE__, TEST_FAILED, L"app_path_test.bat.exe");
        DeleteAppPath(L"app_path_test.bat");
    }

    if (CreateAppPath(L"app_path_test.bat.exe", s_sys_test_exe))
    {
        TEST_DoTestEntry(__LINE__, TEST_FAILED, L"app_path_test.bat");
        TEST_DoTestEntry(__LINE__, TEST_SUCCESS_WITH_PROCESS, L"app_path_test.bat.exe");
        DeleteAppPath(L"app_path_test.bat.exe");
    }
}

static void test_DoInvalidDir(void)
{
    WCHAR szSubProgram[MAX_PATH];
    if (!FindSubProgram(szSubProgram, _countof(szSubProgram)))
    {
        skip("shell32_apitest_sub.exe not found\n");
        return;
    }

    DWORD dwExitCode;
    SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS };
    sei.lpFile = szSubProgram;
    sei.lpParameters = L"TEST";
    sei.nShow = SW_SHOWNORMAL;

    // Test invalid path on sei.lpDirectory
    WCHAR szInvalidPath[MAX_PATH] = L"M:\\This is an invalid path\n";
    sei.lpDirectory = szInvalidPath;
    ok_int(ShellExecuteExW(&sei), TRUE);
    WaitForSingleObject(sei.hProcess, 20 * 1000);
    GetExitCodeProcess(sei.hProcess, &dwExitCode);
    ok_long(dwExitCode, 0);
    CloseHandle(sei.hProcess);
}

START_TEST(ShellExecuteEx)
{
#ifdef _WIN64
    skip("Win64 is not supported yet\n");
    return;
#endif

    if (!TEST_Start())
        return;

    TEST_AppPath();
    TEST_DoTestEntries();
    test_properties();
    test_sei_lpIDList();
    test_DoInvalidDir();

    TEST_End();
}
