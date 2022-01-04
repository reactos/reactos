/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for lstrcpynW
 */

#include "precomp.h"

START_TEST(lstrcpynW)
{
    WCHAR buffer[256];

    /* Test basic functionality */
    ok(lstrcpynW(buffer, L"Copy this string", 256) == buffer, "lstrncpyW failed!\n");
    ok(!lstrcmpW(buffer, L"Copy this string"), "Copy went wrong.\n");

    /* Test for buffer too small */
    ok(lstrcpynW(buffer, L"Copy this string", 10) == buffer, "lstrncpyW failed!\n");
    ok(buffer[9] == 0, "lstrncpyW should have NULL-terminated the string");
    ok(!lstrcmpW(buffer, L"Copy this"), "Copy went wrong.\n");

    /* Test some invalid buffer */
    ok(lstrcpynW((LPWSTR)(LONG_PTR)0xbaadf00d, L"Copy this string", 256) == NULL, "lstrncpyW should have returned NULL.\n");
}
