/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for PathFileExistsDefExtAndAttributesW
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>

START_TEST(PathFileExistsDefExtAndAttributesW)
{
    WCHAR szPath[MAX_PATH];
    DWORD attrs;
    BOOL ret;

    /* NULL check */
    ret = PathFileExistsDefExtAndAttributesW(NULL, 0, NULL);
    ok_int(ret, FALSE);

    /* Not existent file */
    lstrcpynW(szPath, L"Not Existent File.txt", _countof(szPath));
    ret = PathFileExistsDefExtAndAttributesW(szPath, 0, NULL);
    ok_int(ret, FALSE);

    /* "Windows" directory */
    GetWindowsDirectoryW(szPath, _countof(szPath));
    ret = PathFileExistsDefExtAndAttributesW(szPath, 0, NULL);
    ok_int(ret, TRUE);

    /* "Windows" directory with attributes check */
    attrs = 0;
    ret = PathFileExistsDefExtAndAttributesW(szPath, 0, &attrs);
    ok_int(ret, TRUE);
    ok(attrs != 0 && attrs != INVALID_FILE_ATTRIBUTES, "attrs was 0x%lX\n", attrs);

    /* Find notepad.exe */
    SearchPathW(NULL, L"notepad.exe", NULL, _countof(szPath), szPath, NULL);
    ret = PathFileExistsW(szPath);
    ok_int(ret, TRUE);

    /* Remove .exe */
    PathRemoveExtensionW(szPath);
    ret = PathFileExistsW(szPath);
    ok_int(ret, FALSE);

    /* Add .exe */
    ret = PathFileExistsDefExtAndAttributesW(szPath, WHICH_EXE, NULL);
    ok_int(ret, TRUE);
    ret = PathFileExistsW(szPath);
    ok_int(ret, TRUE);

    /* notepad.cmd doesn't exist */
    PathRemoveExtensionW(szPath);
    ret = PathFileExistsDefExtAndAttributesW(szPath, WHICH_CMD, NULL);
    ok_int(ret, FALSE);
    ret = PathFileExistsW(szPath);
    ok_int(ret, FALSE);
}
