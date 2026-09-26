/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for PathIsSlowA/W
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include "shelltest.h"
#include <shlobj.h>

static void Test_PathIsSlowW(void)
{
    WCHAR szPath[MAX_PATH];

    ok_int(PathIsSlowW(L"C:\\", INVALID_FILE_ATTRIBUTES), FALSE);
    ok_int(PathIsSlowW(L"C:\\TestTestTest", INVALID_FILE_ATTRIBUTES), FALSE);

    GetWindowsDirectoryW(szPath, _countof(szPath));
    ok_int(PathIsSlowW(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    GetSystemDirectoryW(szPath, _countof(szPath));
    ok_int(PathIsSlowW(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    GetTempPathW(_countof(szPath), szPath);
    ok_int(PathIsSlowW(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    ok_int(PathIsSlowW(L"C:\\ThisPathDoesNotExist_ROS_Test", INVALID_FILE_ATTRIBUTES), FALSE);

    ok_int(PathIsSlowW(L"C:\\Windows", FILE_ATTRIBUTE_DIRECTORY), FALSE);
    ok_int(PathIsSlowW(L"C:\\Windows", FILE_ATTRIBUTE_HIDDEN), FALSE);
    ok_int(PathIsSlowW(L"C:\\Windows", FILE_ATTRIBUTE_SYSTEM), FALSE);
    ok_int(PathIsSlowW(L"C:\\Windows", FILE_ATTRIBUTE_READONLY), FALSE);
    ok_int(PathIsSlowW(L"C:\\Windows", FILE_ATTRIBUTE_NORMAL), FALSE);

    ok_int(PathIsSlowW(L"C:\\SomeLocalFile.txt", FILE_ATTRIBUTE_OFFLINE), TRUE);

    ok_int(PathIsSlowW(L"C:\\SomeLocalFile.txt",
                       FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NORMAL), TRUE);
    ok_int(PathIsSlowW(L"C:\\SomeLocalFile.txt",
                       FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_READONLY), TRUE);
    ok_int(PathIsSlowW(L"C:\\SomeLocalFile.txt",
                       FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_DIRECTORY), TRUE);

    GetWindowsDirectoryW(szPath, _countof(szPath));
    ok_int(PathIsSlowW(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    GetTempPathW(_countof(szPath), szPath);
    ok_int(PathIsSlowW(szPath, INVALID_FILE_ATTRIBUTES), FALSE);
}

static void Test_PathIsSlowA(void)
{
    CHAR szPath[MAX_PATH];

    ok_int(PathIsSlowA("C:\\", INVALID_FILE_ATTRIBUTES), FALSE);
    ok_int(PathIsSlowA("C:\\TestTestTest", INVALID_FILE_ATTRIBUTES), FALSE);

    GetWindowsDirectoryA(szPath, _countof(szPath));
    ok_int(PathIsSlowA(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    GetSystemDirectoryA(szPath, _countof(szPath));
    ok_int(PathIsSlowA(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    GetTempPathA(_countof(szPath), szPath);
    ok_int(PathIsSlowA(szPath, INVALID_FILE_ATTRIBUTES), FALSE);

    ok_int(PathIsSlowA("C:\\ThisPathDoesNotExist_ROS_Test", INVALID_FILE_ATTRIBUTES), FALSE);

    ok_int(PathIsSlowA("C:\\Windows", FILE_ATTRIBUTE_DIRECTORY), FALSE);
    ok_int(PathIsSlowA("C:\\Windows", FILE_ATTRIBUTE_NORMAL), FALSE);
    ok_int(PathIsSlowA("C:\\SomeLocalFile.txt", FILE_ATTRIBUTE_OFFLINE), TRUE);
    ok_int(PathIsSlowA("C:\\SomeLocalFile.txt",
                       FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_NORMAL), TRUE);
    ok_int(PathIsSlowA("C:\\SomeLocalFile.txt",
                       FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_READONLY), TRUE);

    ok_int(PathIsSlowA("C:\\Windows\\notepad.exe", INVALID_FILE_ATTRIBUTES),
           PathIsSlowW(L"C:\\Windows\\notepad.exe", INVALID_FILE_ATTRIBUTES));

    ok_int(PathIsSlowA("C:\\foo.txt", FILE_ATTRIBUTE_OFFLINE),
           PathIsSlowW(L"C:\\foo.txt", FILE_ATTRIBUTE_OFFLINE));

    ok_int(PathIsSlowA("C:\\foo.txt", FILE_ATTRIBUTE_NORMAL),
           PathIsSlowW(L"C:\\foo.txt", FILE_ATTRIBUTE_NORMAL));
}

START_TEST(PathIsSlow)
{
    Test_PathIsSlowW();
    Test_PathIsSlowA();
}
