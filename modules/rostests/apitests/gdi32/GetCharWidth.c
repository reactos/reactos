/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for GetCharWidth... functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

static void Test_CharWidthA(HDC hDC)
{
    BOOL ret;
    INT anBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(NULL, 'A', 'Z', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(NULL, 'B', 'A', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(hDC, 'A', 'Z', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthA(hDC, 'A', 'B', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

static void Test_CharWidthW(HDC hDC)
{
    BOOL ret;
    INT anBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(NULL, 'A', 'Z', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(NULL, 'B', 'A', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(hDC, 'A', 'Z', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthW(hDC, 'A', 'B', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

static void Test_CharWidthI(HDC hDC)
{
    BOOL ret;
    WORD awBuffer['Z' - 'A' + 1];
    INT anWidths['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(NULL, 0, 0, NULL, NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(hDC, 0, 0, NULL, NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(NULL, 0, 1, awBuffer, anWidths);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_HANDLE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(hDC, 0, 1, awBuffer, NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(hDC, 0, 1, awBuffer, anWidths);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthI(hDC, 0, _countof(awBuffer), awBuffer, anWidths);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

#if 0 // These tests take time in ReactOS
    INT count = 0x2710000 / sizeof(INT);
    PINT pn = malloc((count + 1) * sizeof(INT));
    PWORD pw = malloc((count + 1) * sizeof(WORD));

    if (!pn || !pw)
    {
        skip("allocation failed\n");
    }
    else
    {
        SetLastError(0xBEEFCAFE);
        ret = GetCharWidthI(hDC, 0, count, pw, pn);
        ok_int(ret, TRUE);
        ok_err(0xBEEFCAFE);

        SetLastError(0xBEEFCAFE);
        ret = GetCharWidthI(hDC, 0, count + 1, pw, pn);
        ok_int(ret, FALSE);
        ok_err(0xBEEFCAFE);
    }

    free(pn);
    free(pw);
#endif
}

static void Test_CharWidth32A(HDC hDC)
{
    BOOL ret;
    INT anBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(NULL, 'A', 'Z', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(NULL, 'B', 'A', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(hDC, 'A', 'Z', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32A(hDC, 'A', 'B', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

static void Test_CharWidth32W(HDC hDC)
{
    BOOL ret;
    INT anBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(NULL, 'A', 'Z', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(NULL, 'B', 'A', anBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(hDC, 'A', 'Z', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidth32W(hDC, 'A', 'B', anBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

static void Test_CharWidthFloatA(HDC hDC)
{
    BOOL ret;
    FLOAT aeBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(NULL, 'A', 'Z', aeBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(NULL, 'B', 'A', aeBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER); //

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(hDC, 'A', 'Z', aeBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatA(hDC, 'A', 'B', aeBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

static void Test_CharWidthFloatW(HDC hDC)
{
    BOOL ret;
    FLOAT aeBuffer['Z' - 'A' + 1];

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(NULL, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(NULL, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(NULL, 'A', 'Z', aeBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(NULL, 'B', 'A', aeBuffer);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(hDC, 'A', 'Z', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(hDC, 'B', 'A', NULL);
    ok_int(ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(hDC, 'A', 'Z', aeBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);

    SetLastError(0xBEEFCAFE);
    ret = GetCharWidthFloatW(hDC, 'A', 'B', aeBuffer);
    ok_int(ret, TRUE);
    ok_err(0xBEEFCAFE);
}

START_TEST(GetCharWidth)
{
    HDC hDC = CreateCompatibleDC(NULL);
    Test_CharWidthA(hDC);
    Test_CharWidthW(hDC);
    Test_CharWidthI(hDC);
    Test_CharWidth32A(hDC);
    Test_CharWidth32W(hDC);
    Test_CharWidthFloatA(hDC);
    Test_CharWidthFloatW(hDC);
    DeleteDC(hDC);
}
