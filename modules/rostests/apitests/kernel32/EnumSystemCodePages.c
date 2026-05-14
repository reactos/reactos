/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for EnumSystemCodePages
 * COPYRIGHT:   Copyright 2026 Aliaksandr Lebiadzevich <bsalex2025@gmail.com>
 *              Copyright 2026 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

char* func = NULL;
ULONG cntInstalled, cntSupported, cntLast;

BOOL
CALLBACK
EnumCodePagesProcW(
    LPWSTR lpCodePageString)
{
    ULONG CodePage = 0;
    int ret = swscanf(lpCodePageString, L"%u", &CodePage);

    ok(ret == 1, "%s: '%S' is not a valid number\n", func, lpCodePageString);
    if (ret == 1)
        trace("%s: '%S' is a valid number (%lu)\n", func, lpCodePageString, CodePage);

    cntLast++;
    return TRUE;
}

BOOL
CALLBACK
EnumCodePagesProcA(
    LPSTR lpCodePageString)
{
    BOOL ret;
    int len = MultiByteToWideChar(CP_ACP, 0, lpCodePageString, -1, NULL, 0);
    WCHAR *str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!str)
        return FALSE;

    MultiByteToWideChar(CP_ACP, 0, lpCodePageString, -1, str, len);
    ret = EnumCodePagesProcW(str);
    HeapFree(GetProcessHeap(), 0, str);

    return ret;
}

typedef struct ENTRY
{
    DWORD dwFlags;
    BOOL bReturn;
    DWORD dwError;
} ENTRY;

VOID
RunEnumTests(
    BOOL Ansi)
{
    BOOL ret;
    int i;

    static const ENTRY Tests[] =
    {
        { /* 1 */ CP_INSTALLED, TRUE, 0xDEADF00D },
        { /* 2 */ CP_SUPPORTED, TRUE, 0xDEADF00D },
        { 0, TRUE, 0xDEADF00D }, /* Windows handles this as CP_SUPPORTED */
        { 3, FALSE, ERROR_INVALID_FLAGS },
        { (DWORD)-1, FALSE, ERROR_INVALID_FLAGS },
    };

    for (i = 0; i < _countof(Tests); i++)
    {
        cntLast = 0;
        SetLastError(Tests[0].dwError);
        func = Ansi
            ? "EnumSystemCodePagesA"
            : "EnumSystemCodePagesW";
        ret = Ansi
            ? EnumSystemCodePagesA(EnumCodePagesProcA, Tests[i].dwFlags)
            : EnumSystemCodePagesW(EnumCodePagesProcW, Tests[i].dwFlags);
        if (ret) trace("%s enumerated %u entries\n", func, cntLast);
        ok(Tests[i].bReturn == ret, "%s unexpected return: %u, expected %u\n", func, ret, Tests[i].bReturn);
        ok(Tests[i].dwError == GetLastError(), "%s unexpected error: %u, expected %u\n", func, GetLastError(), Tests[i].dwError);

        if (Tests[i].dwFlags == CP_INSTALLED)
        {
            cntInstalled = cntLast;
            ok(cntInstalled > 0, "No installed codepages enumerated\n");
        }
        else if (Tests[i].dwFlags == CP_SUPPORTED)
        {
            cntSupported = cntLast;
            ok(cntSupported > 0, "No supported codepages enumerated\n");
        }
        else if (Tests[i].dwFlags == 0)
            ok(cntLast == cntSupported, "Number of codepages expected: %u, got %u\n", cntSupported, cntLast);
    }
}

START_TEST(EnumSystemCodePages)
{
    RunEnumTests(FALSE);
    RunEnumTests(TRUE);
}
