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

    ok_int(PathFileExistsDefExtAndAttributesW(NULL, 0, NULL), FALSE);

    GetWindowsDirectoryW(szPath, _countof(szPath));

    ok_int(PathFileExistsDefExtAndAttributesW(szPath, 0, NULL), TRUE);

    attrs = 0;
    ok_int(PathFileExistsDefExtAndAttributesW(szPath, 0, &attrs), TRUE);
    ok(attrs != 0 && attrs != INVALID_FILE_ATTRIBUTES, "attrs was 0x%lX\n", attrs);
}
