/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontResource
 * PROGRAMMERS:     Timo Kreuzer
 *                  Serge Gautherie
 */

#include "precomp.h"

void Test_AddFontResourceA()
{
    CHAR szCurrentDir[MAX_PATH];
    CHAR szFileNameFont1[MAX_PATH];
    CHAR szFileNameFont2[MAX_PATH];
    CHAR szFileName[2 * (MAX_PATH - 1) + 2 + 1];
    int result;

    GetCurrentDirectoryA(_countof(szCurrentDir), szCurrentDir);

    /* Testing NULL pointer */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(NULL);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(NULL) succeeded, result=%d\n", result);

    /* Testing -1 pointer */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA((CHAR*)-1);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(-1) succeeded, result=%d\n", result);

    /* Testing address 1 pointer */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA((CHAR*)1);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(1) succeeded, result=%d\n", result);

    /* Testing address empty string */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA("");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"\") succeeded, result=%d\n", result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\NoDir\\", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata\\", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata\\NoFile.ttf", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata\\test", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata\\test.ttf", szCurrentDir);
    snprintf(szFileNameFont2, _countof(szFileNameFont2), "%s\\testdata\\test.otf", szCurrentDir);

    /* Testing one ttf font */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    // Windows 10: v1507 still unchanged, v1607-v2009(+) ERROR_INSUFFICIENT_BUFFER (122).
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileNameFont1, result);
    if (result != 0)
    {
        ok(RemoveFontResourceA(szFileNameFont1),
           "RemoveFontResourceA(\"%s\") failed\n", szFileNameFont1);
    }

    /* Testing one otf font */
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont2);
    // Windows 10 v1607 only: ERROR_FILE_NOT_FOUND (2).
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileNameFont2, result);
    if (result != 0)
    {
        ok(RemoveFontResourceA(szFileNameFont2),
           "RemoveFontResourceA(\"%s\") failed\n", szFileNameFont2);
    }

    /* Testing two fonts */
    snprintf(szFileName, _countof(szFileName), "%s|%s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileName, _countof(szFileName), "%s |%s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileName, _countof(szFileName), "%s| %s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileNameFont1, _countof(szFileNameFont1), "%s\\testdata\\test.pfm", szCurrentDir);
    snprintf(szFileNameFont2, _countof(szFileNameFont2), "%s\\testdata\\test.pfb", szCurrentDir);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont1);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont1, result);

    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileNameFont2);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileNameFont2, result);

    snprintf(szFileName, _countof(szFileName), "%s|", szFileNameFont1);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileName, _countof(szFileName), "|%s", szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileName, _countof(szFileName), "%s|%s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    // Windows 10 v1607 only: ERROR_FILE_NOT_FOUND (2).
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileName, result);
    if (result != 0)
    {
        ok(RemoveFontResourceA(szFileName),
           "RemoveFontResourceA(\"%s\") failed\n", szFileName);
    }

    snprintf(szFileName, _countof(szFileName), "%s |%s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    // Windows 10 v1607 only: ERROR_FILE_NOT_FOUND (2).
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileName, result);
    if (result != 0)
    {
        ok(RemoveFontResourceA(szFileName),
           "RemoveFontResourceA(\"%s\") failed\n", szFileName);
    }

    snprintf(szFileName, _countof(szFileName), "%s| %s", szFileNameFont1, szFileNameFont2);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);

    snprintf(szFileName, _countof(szFileName), "%s|%s", szFileNameFont2, szFileNameFont1);
    SetLastError(0xdeadbeef);
    result = AddFontResourceA(szFileName);
    ok(GetLastError() == 0xdeadbeef, "GetLastError()=%lu\n", GetLastError());
    ok(result == 0, "AddFontResourceA(\"%s\") succeeded, result=%d\n", szFileName, result);
}

START_TEST(AddFontResource)
{
    Test_AddFontResourceA();
}
