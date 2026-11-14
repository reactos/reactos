/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontResource
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#define COUNT 26

void Test_AddFontResourceA()
{
    CHAR szCurrentDir[MAX_PATH];
    CHAR szFileNameFont1[MAX_PATH];
    CHAR szFileNameFont2[MAX_PATH];
    CHAR szFileName[MAX_PATH*2 + 3];
    int result;

    GetCurrentDirectoryA(MAX_PATH, szCurrentDir);

    snprintf(szFileNameFont1, MAX_PATH, "%s\\testdata\\test.ttf", szCurrentDir);
    snprintf(szFileNameFont2, MAX_PATH, "%s\\testdata\\test.otf", szCurrentDir);

    //RtlZeroMemory(szFileNameA, sizeof(szFileNameA));

    /* Testing NULL pointer */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA(NULL);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    /* Testing -1 pointer */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA((CHAR*)-1);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    /* Testing address 1 pointer */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA((CHAR*)1);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    /* Testing address empty string */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA("");
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%ld\n", GetLastError());

    /* Testing one ttf font */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA(szFileNameFont1);
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileNameFont1, result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());
    RemoveFontResourceA(szFileNameFont1);

    /* Testing one otf font */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA(szFileNameFont2);
    ok(result == 1, "AddFontResourceA failed, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());
    RemoveFontResourceA(szFileNameFont2);

    /* Testing two fonts */
    SetLastError(ERROR_SUCCESS);
    sprintf(szFileName,"%s|%s",szFileNameFont1, szFileNameFont2);
    result = AddFontResourceA(szFileName);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    sprintf(szFileName,"%s |%s",szFileNameFont1, szFileNameFont2);
    result = AddFontResourceA(szFileName);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    sprintf(szFileName,"%s | %s",szFileNameFont1, szFileNameFont2);
    result = AddFontResourceA(szFileName);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());

    snprintf(szFileNameFont1, MAX_PATH, "%s\\testdata\\test.pfm", szCurrentDir);
    snprintf(szFileNameFont2, MAX_PATH, "%s\\testdata\\test.pfb", szCurrentDir);

    SetLastError(ERROR_SUCCESS);

    sprintf(szFileName,"%s|%s", szFileNameFont1, szFileNameFont2);
    result = AddFontResourceA(szFileName);
    ok(result == 1, "AddFontResourceA(\"%s|%s\") failed, result=%d\n",
                    szFileNameFont1, szFileNameFont2, result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());
    RemoveFontResourceA(szFileName);

    sprintf(szFileName,"%s | %s", szFileNameFont1, szFileNameFont2);
    result = AddFontResourceA(szFileName);
    ok(result == 0, "AddFontResourceA(\"%s | %s\") succeeded, result=%d\n",
                    szFileNameFont1, szFileNameFont2, result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());
    RemoveFontResourceA(szFileName);

    sprintf(szFileName,"%s|%s", szFileNameFont2, szFileNameFont1);
    result = AddFontResourceA(szFileName);
    ok(result == 0, "AddFontResourceA(\"%s|%s\") succeeded, result=%d\n",
                    szFileNameFont2, szFileNameFont1, result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());
}

static void Test_DeleteLoadedFont(void)
{
    CHAR szSource[MAX_PATH], szTempPath[MAX_PATH], szFont[MAX_PATH];
    BOOL bResult;
    int result;

    GetWindowsDirectoryA(szSource, MAX_PATH);
    strcat(szSource, "\\Fonts\\tahoma.ttf");
    GetTempPathA(MAX_PATH, szTempPath);
    GetTempFileNameA(szTempPath, "fnt", 0, szFont);

    if (!CopyFileA(szSource, szFont, FALSE))
    {
        skip("Unable to copy '%s' (%lu)\n", szSource, GetLastError());
        DeleteFileA(szFont);
        return;
    }

    result = AddFontResourceA(szFont);
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFont, result);

    /* The loaded font keeps the file mapped */
    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(szFont);
    ok(bResult == FALSE, "DeleteFileA succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "GetLastError()=%ld\n", GetLastError());

    /* Nor can it be overwritten (CORE-19789) */
    SetLastError(0xdeadbeef);
    bResult = CopyFileA(szSource, szFont, FALSE);
    ok(bResult == FALSE, "CopyFileA succeeded\n");
    ok(GetLastError() == ERROR_USER_MAPPED_FILE, "GetLastError()=%ld\n", GetLastError());

    bResult = RemoveFontResourceA(szFont);
    ok(bResult == TRUE, "RemoveFontResourceA failed\n");

    SetLastError(0xdeadbeef);
    bResult = DeleteFileA(szFont);
    ok(bResult == TRUE, "DeleteFileA failed with %lu\n", GetLastError());
}

START_TEST(AddFontResource)
{
    Test_AddFontResourceA();
    Test_DeleteLoadedFont();
}

