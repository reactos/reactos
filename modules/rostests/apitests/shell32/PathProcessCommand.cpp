/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for PathProcessCommand
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>

#define ok_wstri(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

typedef LONG (WINAPI *FN_PathProcessCommand)(LPCWSTR, LPWSTR, INT, DWORD);

static FN_PathProcessCommand s_PathProcessCommand = NULL;

static void
Test_PathProcessCommand(void)
{
    WCHAR buffer[MAX_PATH], szCalcExe[MAX_PATH];
    LONG result;
    FILE *fout;
    WCHAR szCurDir[MAX_PATH];
    WCHAR szFull1[MAX_PATH], szFull2[MAX_PATH], szFull3[MAX_PATH], szFull4[MAX_PATH];
    WCHAR szFull5[MAX_PATH];

    fout = _wfopen(L"_Test.exe", L"wb");
    fclose(fout);
    fout = _wfopen(L"test with spaces.exe", L"wb");
    fclose(fout);

    GetFullPathNameW(L".", _countof(szCurDir), szCurDir, NULL);
    GetShortPathNameW(szCurDir, szCurDir, _countof(szCurDir));

    lstrcpynW(szFull1, szCurDir, _countof(szFull1));
    lstrcpynW(szFull2, szCurDir, _countof(szFull2));
    lstrcpynW(szFull4, szCurDir, _countof(szFull4));
    lstrcpynW(szFull5, szCurDir, _countof(szFull5));

    lstrcatW(szFull1, L"\\_Test.exe");
    lstrcatW(szFull2, L"\\test with spaces.exe");
    wsprintfW(szFull3, L"\"%s\"", szFull2);
    lstrcatW(szFull4, L"\\_Test.exe arg1 arg2");
    lstrcatW(szFull5, L"\\_TestDir.exe");

    CreateDirectoryW(L"_TestDir.exe", NULL);

    // Test case 1: Basic functionality (no flags)
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_Test", buffer, _countof(buffer), 0);
    ok_int(result, lstrlenW(szFull1) + 1);
    ok_wstri(buffer, szFull1);

    // Test case 2: Quoted path
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"\"test with spaces\"", buffer, _countof(buffer), 0);
    ok_int(result, lstrlenW(szFull2) + 1);
    ok_wstri(buffer, szFull2);

    // Test case 3: Add quotes flag
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"test with spaces", buffer, _countof(buffer), PPCF_ADDQUOTES);
    ok_int(result, lstrlenW(szFull3) + 1);
    ok_wstri(buffer, szFull3);

    // Test case 4: Add arguments flag
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_Test arg1 arg2", buffer, _countof(buffer), PPCF_ADDARGUMENTS);
    ok_int(result, lstrlenW(szFull4) + 1);
    ok_wstri(buffer, szFull4);

    // calc.exe
    GetSystemDirectoryW(szCalcExe, _countof(szCalcExe));
    PathAppendW(szCalcExe, L"calc.exe");

    // Test case 5: Longest possible flag
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(szCalcExe, buffer, _countof(buffer), PPCF_LONGESTPOSSIBLE);
    ok_int(result, lstrlenW(szCalcExe) + 1);
    ok_wstri(buffer, szCalcExe);

    // Test case 6: Buffer too small
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_Test.exe", buffer, 5, 0);
    ok_int(result, -1);
    ok_wstri(buffer, L"<>");

    // Test case 7: Null input path
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(NULL, buffer, _countof(buffer), 0);
    ok_int(result, -1);
    ok_wstri(buffer, L"<>");

    // Test case 8: Relative path resolution
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_Test.exe", buffer, _countof(buffer), PPCF_FORCEQUALIFY);
    ok_int(result, lstrlenW(szFull1) + 1);
    ok_wstri(buffer, szFull1);

    // Test case 9: No directories
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_TestDir.exe", buffer, _countof(buffer), PPCF_NODIRECTORIES);
    ok_int(result, -1);
    ok_wstri(buffer, L"<>");

    // Test case 10: With directories
    lstrcpynW(buffer, L"<>", _countof(buffer));
    result = s_PathProcessCommand(L"_TestDir.exe", buffer, _countof(buffer), 0);
    ok_int(result, lstrlenW(szFull5) + 1);
    ok_wstri(buffer, szFull5);

    RemoveDirectoryW(L"_TestDir.exe");
    DeleteFileW(L"_Test.exe");
    DeleteFileW(L"test with spaces.exe");
}

START_TEST(PathProcessCommand)
{
    WCHAR szCurDir[MAX_PATH], szTempDir[MAX_PATH];

    s_PathProcessCommand = (FN_PathProcessCommand)
        GetProcAddress(GetModuleHandleW(L"shell32"), "PathProcessCommand");
    if (!s_PathProcessCommand)
    {
        s_PathProcessCommand = (FN_PathProcessCommand)
            GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(653));
    }
    if (!s_PathProcessCommand)
    {
        skip("PathProcessCommand not found\n");
        return;
    }

    GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
    GetEnvironmentVariableW(L"TEMP", szTempDir, _countof(szTempDir));
    SetCurrentDirectoryW(szTempDir);

    SetEnvironmentVariableW(L"PATH", szTempDir);

    Test_PathProcessCommand();

    SetCurrentDirectoryW(szCurDir);
}
