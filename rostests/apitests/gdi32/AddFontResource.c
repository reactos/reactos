/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontResource
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <apitest.h>
#include <wingdi.h>

#define COUNT 26

void Test_AddFontResourceA()
{
    CHAR szFileNameA[MAX_PATH];
    CHAR szFileNameFont1A[MAX_PATH];
    CHAR szFileNameFont2A[MAX_PATH];
    int result;

    GetCurrentDirectoryA(MAX_PATH,szFileNameA);

    memcpy(szFileNameFont1A,szFileNameA,MAX_PATH );
    strcat(szFileNameFont1A, "\\bin\\testdata\\test.ttf");

    memcpy(szFileNameFont2A,szFileNameA,MAX_PATH );
    strcat(szFileNameFont2A, "\\bin\\testdata\\test.otf");

    RtlZeroMemory(szFileNameA,MAX_PATH);

    /*
     * Start testing Ansi version
     *
     */

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
    result = AddFontResourceA(szFileNameFont1A);
    ok(result == 1, "AddFontResourceA(\"%s\") failed, result=%d\n", szFileNameFont1A, result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    /* Testing one otf font */
    SetLastError(ERROR_SUCCESS);
    result = AddFontResourceA(szFileNameFont2A);
    ok(result == 1, "AddFontResourceA failed, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    /* Testing two fonts */
    SetLastError(ERROR_SUCCESS);
    sprintf(szFileNameA,"%s|%s",szFileNameFont1A, szFileNameFont2A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    sprintf(szFileNameA,"%s |%s",szFileNameFont1A, szFileNameFont2A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    sprintf(szFileNameA,"%s | %s",szFileNameFont1A, szFileNameFont2A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 0, "AddFontResourceA succeeded, result=%d\n", result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());


    GetCurrentDirectoryA(MAX_PATH, szFileNameA);
    strcpy(szFileNameFont1A, szFileNameA);
    strcat(szFileNameFont1A, "\\bin\\testdata\\test.pfm");

    strcpy(szFileNameFont2A, szFileNameA);
    strcat(szFileNameFont2A, "\\bin\\testdata\\test.pfb");

    SetLastError(ERROR_SUCCESS);

    sprintf(szFileNameA,"%s|%s", szFileNameFont1A, szFileNameFont2A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 1, "AddFontResourceA(\"%s|%s\") failed, result=%d\n",
                    szFileNameFont1A, szFileNameFont2A, result);
    ok(GetLastError() == ERROR_SUCCESS, "GetLastError()=%ld\n", GetLastError());

    sprintf(szFileNameA,"%s | %s", szFileNameFont1A, szFileNameFont2A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 0, "AddFontResourceA(\"%s | %s\") succeeded, result=%d\n",
                    szFileNameFont1A, szFileNameFont2A, result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());

    sprintf(szFileNameA,"%s|%s", szFileNameFont2A, szFileNameFont1A);
    result = AddFontResourceA(szFileNameA);
    ok(result == 0, "AddFontResourceA(\"%s|%s\") succeeded, result=%d\n",
                    szFileNameFont2A, szFileNameFont1A, result);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError()=%ld\n", GetLastError());


}

START_TEST(AddFontResource)
{
    Test_AddFontResourceA();
}

