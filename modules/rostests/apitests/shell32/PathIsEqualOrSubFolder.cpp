/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for PathIsEqualOrSubFolder
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <undocshell.h>

START_TEST(PathIsEqualOrSubFolder)
{
    ok_int(PathIsEqualOrSubFolder(L"C:", L"C:"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:", L"C:\\"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\", L"C:"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\", L"C:\\"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\", L"C:\\TestTestTest"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\TestTestTest", L"C:\\"), FALSE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\TestTestTest", L"C:\\TestTestTest"), TRUE);
    ok_int(PathIsEqualOrSubFolder(L"C:\\TestTestTest", L"C:\\TestTestTest\\"), TRUE);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];

    GetWindowsDirectoryW(szPath1, _countof(szPath1));
    ok_int(PathIsEqualOrSubFolder(szPath1, szPath1), TRUE);

    GetWindowsDirectoryW(szPath2, _countof(szPath2));
    PathAppendW(szPath2, L"TestTestTest");

    ok_int(PathIsEqualOrSubFolder(szPath1, szPath2), TRUE);
    ok_int(PathIsEqualOrSubFolder(szPath2, szPath1), FALSE);
    ok_int(PathIsEqualOrSubFolder(szPath2, szPath2), TRUE);

    GetTempPathW(_countof(szPath1), szPath1);
    GetTempPathW(_countof(szPath2), szPath2);
    PathAppendW(szPath2, L"TestTestTest");

    ok_int(PathIsEqualOrSubFolder(szPath1, szPath2), TRUE);
    ok_int(PathIsEqualOrSubFolder(szPath2, szPath1), FALSE);
    ok_int(PathIsEqualOrSubFolder(szPath2, szPath2), TRUE);
}
