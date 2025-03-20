/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for PathMakeUniqueName
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>

#define ok_wstri(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

START_TEST(PathMakeUniqueName)
{
    FILE *fout;
    WCHAR szPathName[MAX_PATH];
    BOOL ret;

    DeleteFileW(L"_TestFile.txt");
    DeleteFileW(L"_TestFile (1).txt");
    DeleteFileW(L"_TestFile (2).txt");

    ret = PathMakeUniqueName(szPathName, 0, L"_TestFile.txt", L"_TestFile.txt", L".");
    ok_int(ret, FALSE);

    ret = PathMakeUniqueName(NULL, _countof(szPathName), L"_TestFile.txt", L"_TestFile.txt", L".");
    ok_int(ret, FALSE);

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), L"_TestFile.txt", L"_TestFile.txt", L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), L"_TestFile.txt", NULL, L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), NULL, L"_TestFile.txt", L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    fout = _wfopen(L"_TestFile.txt", L"wb");
    if (fout)
        fclose(fout);

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), L"_TestFile.txt", L"_TestFile.txt", L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), L"_TestFile.txt", NULL, L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), NULL, L"_TestFile.txt", L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (1).txt");

    fout = _wfopen(L"_TestFile (1).txt", L"wb");
    if (fout)
        fclose(fout);

    szPathName[0] = UNICODE_NULL;
    ret = PathMakeUniqueName(szPathName, _countof(szPathName), NULL, L"_TestFile.txt", L".");
    ok_int(ret, TRUE);
    ok_wstri(szPathName, L".\\_TestFile (2).txt");

    DeleteFileW(L"_TestFile (1).txt");
    DeleteFileW(L"_TestFile.txt");
}
