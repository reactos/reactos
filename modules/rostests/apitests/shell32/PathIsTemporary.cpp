/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for PathIsTemporaryA/W
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <undocshell.h>

static void Test_PathIsTemporaryA(void)
{
    CHAR szPath[MAX_PATH];
    ok_int(PathIsTemporaryA("C:\\"), FALSE);
    ok_int(PathIsTemporaryA("C:\\TestTestTest"), FALSE);

    GetWindowsDirectoryA(szPath, _countof(szPath));
    ok_int(PathIsTemporaryA(szPath), FALSE);

    GetTempPathA(_countof(szPath), szPath);
    ok_int(PathIsTemporaryA(szPath), TRUE);

    PathAppendA(szPath, "TestTestTest");
    ok_int(PathIsTemporaryA(szPath), FALSE);

    CreateDirectoryA(szPath, NULL);
    ok_int(PathIsTemporaryA(szPath), TRUE);

    RemoveDirectoryA(szPath);
}

static void Test_PathIsTemporaryW(void)
{
    WCHAR szPath[MAX_PATH];
    ok_int(PathIsTemporaryW(L"C:\\"), FALSE);
    ok_int(PathIsTemporaryW(L"C:\\TestTestTest"), FALSE);

    GetWindowsDirectoryW(szPath, _countof(szPath));
    ok_int(PathIsTemporaryW(szPath), FALSE);

    GetTempPathW(_countof(szPath), szPath);
    ok_int(PathIsTemporaryW(szPath), TRUE);

    PathAppendW(szPath, L"TestTestTest");
    ok_int(PathIsTemporaryW(szPath), FALSE);

    CreateDirectoryW(szPath, NULL);
    ok_int(PathIsTemporaryW(szPath), TRUE);

    RemoveDirectoryW(szPath);
}

START_TEST(PathIsTemporary)
{
    Test_PathIsTemporaryA();
    Test_PathIsTemporaryW();
}
