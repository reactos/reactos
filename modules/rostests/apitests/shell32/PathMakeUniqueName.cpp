/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for PathMakeUniqueName
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>
#include <versionhelpers.h>

#define ok_wstri(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

/* IsLFNDriveW */
typedef BOOL (WINAPI *FN_IsLFNDriveW)(LPCWSTR);

START_TEST(PathMakeUniqueName)
{
    WCHAR szPath[MAX_PATH], szCurDir[MAX_PATH], szTempDir[MAX_PATH];
    BOOL result, bUseLong = FALSE;
    FN_IsLFNDriveW pIsLFNDriveW =
        (FN_IsLFNDriveW)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(42));

    // Move to temporary folder
    GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
    GetEnvironmentVariableW(L"TEMP", szTempDir, _countof(szTempDir));
    SetCurrentDirectoryW(szTempDir);

    if (pIsLFNDriveW)
        bUseLong = pIsLFNDriveW(szTempDir) && (GetNTVersion() >= _WIN32_WINNT_WIN10);
    trace("bUseLong: %d\n", bUseLong);

    DeleteFileW(L"test.txt");

    // Test 1: Basic operation
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, NULL);
    ok_int(result, TRUE);
    ok_wstri(szPath, L"test (1).txt");

    // Test 2: Specify directory
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, L".");
    ok_int(result, TRUE);
    ok_wstri(szPath, (bUseLong ? L".\\test (1).txt" : L".\\test1.txt"));

    // Test 3: Duplicated filename
    CreateFileW(L"test.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, NULL);
    ok_int(result, TRUE);
    ok_wstri(szPath, L"test (1).txt");
    DeleteFileW(L"test.txt");

    // Build long name
    WCHAR longName[MAX_PATH + 32];
    for (auto& ch : longName)
        ch = L'A';
    longName[_countof(longName) - 10] = UNICODE_NULL;
    lstrcatW(longName, L".txt");

    // Test 4: Long filename
    result = PathMakeUniqueName(szPath, _countof(szPath), longName, NULL, NULL);
    szPath[0] = UNICODE_NULL;
    ok_int(result, FALSE);
    ok_wstri(szPath, L"");

    // Test 5: Invalid parameter
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(NULL, 0, L"test.txt", NULL, NULL);
    ok_int(result, FALSE);

    // Test 6: Template and longplate
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"template.txt", L"longplate.txt", NULL);
    ok_int(result, TRUE);
    ok_wstri(szPath, L"longplate (1).txt");

    // Test 7: Template only
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"template.txt", NULL, NULL);
    ok_int(result, TRUE);
    ok_wstri(szPath, L"template (1).txt");

    // Test 8: Folder and duplicated filename
    CreateFileW(L".\\temp\\test.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, L".");
    ok_int(result, TRUE);
    ok_wstri(szPath, (bUseLong ? L".\\test (1).txt" : L".\\test1.txt"));
    DeleteFileW(L".\\test.txt");

    // Test 9: Test extension
    CreateFileW(L".\\test.hoge", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.hoge", NULL, L".");
    ok_int(result, TRUE);
    ok_wstri(szPath, (bUseLong ? L".\\test (1).hoge" : L".\\test1.hoge"));
    DeleteFileW(L".\\test.hoge");

    // Test 10: Folder in folder
    CreateDirectoryW(L".\\hoge", NULL);
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, L".\\hoge");
    ok_int(result, TRUE);
    ok_wstri(szPath, (bUseLong ? L".\\hoge\\test (1).txt" : L".\\hoge\\test1.txt"));
    RemoveDirectoryW(L".\\hoge");

    // Test 11: File in folder
    CreateFileW(L".\\hoge.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    szPath[0] = UNICODE_NULL;
    result = PathMakeUniqueName(szPath, _countof(szPath), L"test.txt", NULL, L".");
    ok_int(result, TRUE);
    ok_wstri(szPath, (bUseLong ? L".\\test (1).txt" : L".\\test1.txt"));
    DeleteFileW(L".\\hoge.txt");

    SetCurrentDirectoryW(szCurDir);
}
