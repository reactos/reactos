/*
 * Copyright 2017 Jared Smudde
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Documentation: https://msdn.microsoft.com/en-us/library/windows/desktop/bb773712(v=vs.85).aspx */

#include <apitest.h>

static BOOL (WINAPI *pPathIsUNC)(PCWSTR);

#define CALL_ISUNC(exp, str) \
do { \
    BOOL ret = pPathIsUNC((str)); \
    ok(ret == (exp), "Expected %S to be %d, was %d\n", (str), (exp), ret); \
} while (0)

START_TEST(isuncpath)
{
    HMODULE hDll = LoadLibraryA("shlwapi.dll");

    pPathIsUNC = (void*)GetProcAddress(hDll, "PathIsUNCW");
    if (!hDll || !pPathIsUNC)
    {
        skip("shlwapi.dll or export PathIsUNCW not found! Tests will be skipped\n");
        return;
    }

    CALL_ISUNC(TRUE, L"\\\\path1\\path2");
    CALL_ISUNC(TRUE, L"\\\\path1");
    CALL_ISUNC(FALSE, L"reactos\\path4\\path5");
    CALL_ISUNC(TRUE, L"\\\\");
    CALL_ISUNC(TRUE, L"\\\\?\\UNC\\path1\\path2");
    CALL_ISUNC(TRUE, L"\\\\?\\UNC\\path1");
    CALL_ISUNC(TRUE, L"\\\\?\\UNC\\");
    CALL_ISUNC(FALSE, L"\\path1");
    CALL_ISUNC(FALSE, L"path1");
    CALL_ISUNC(FALSE, L"c:\\path1");

    /* MSDN says FALSE but the test shows TRUE on Windows 2003, but returns FALSE on Windows 7 */
    CALL_ISUNC(TRUE, L"\\\\?\\c:\\path1");

    CALL_ISUNC(TRUE, L"\\\\path1\\");
    CALL_ISUNC(FALSE, L"//");
    CALL_ISUNC(FALSE, L"////path1");
    CALL_ISUNC(FALSE, L"////path1//path2");
    CALL_ISUNC(FALSE, L"reactos//path3//path4");
    CALL_ISUNC(TRUE, L"\\\\reactos\\?");
    CALL_ISUNC(TRUE, L"\\\\reactos\\\\");
    CALL_ISUNC(FALSE, NULL);
    CALL_ISUNC(FALSE, L" ");

    /* The test shows TRUE on Windows 2003, but returns FALSE on Windows 7 */
    CALL_ISUNC(TRUE, L"\\\\?\\");
}
