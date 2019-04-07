/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/winnls/string/japanese.c
 * PURPOSE:         Japanese era support
 * PROGRAMMER:      Katayama Hirofumi MZ
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>
#include "japanese.h"

#define JAPANESE_ERA_MAX 16

#define DONT_USE_REGISTRY

static DWORD        s_JapaneseEraCount = 0;
static JAPANESE_ERA s_JapaneseEraTable[JAPANESE_ERA_MAX]
#ifdef DONT_USE_REGISTRY
=
{
    {1868, 1, 1, {0x660E, 0x6CBB}, {0x660E, 0}, L"Meiji", L"M"},
    {1912, 7, 30, {0x5927, 0x6B63}, {0x5927, 0}, L"Taisho", L"T"},
    {1926, 12, 25, {0x662D, 0x548C}, {0x662D, 0}, L"Showa", L"S"},
    {1989, 1, 8, {0x5E73, 0x6210}, {0x5E73, 0}, L"Heisei", L"H"},
    {2019, 5, 1, {0x4EE4, 0x548C}, {0x4EE4, 0}, L"Reiwa", L"R"},
}
#endif
;

HANDLE NLS_RegOpenKey(HANDLE hRootKey, LPCWSTR szKeyName);

BOOL NLS_RegEnumValue(HANDLE hKey, UINT ulIndex,
                      LPWSTR szValueName, ULONG valueNameSize,
                      LPWSTR szValueData, ULONG valueDataSize);

INT JapaneseEra_Compare(LPCJAPANESE_ERA pEra1, LPCJAPANESE_ERA pEra2)
{
    if (pEra1->wYear < pEra2->wYear)
        return -1;
    if (pEra1->wYear > pEra2->wYear)
        return 1;
    if (pEra1->wMonth < pEra2->wMonth)
        return -1;
    if (pEra1->wMonth > pEra2->wMonth)
        return 1;
    if (pEra1->wDay < pEra2->wDay)
        return -1;
    if (pEra1->wDay > pEra2->wDay)
        return 1;
    return 0;
}

INT JapaneseEra_Compare0(const void *pEra1, const void *pEra2)
{
    return JapaneseEra_Compare((LPCJAPANESE_ERA)pEra1, (LPCJAPANESE_ERA)pEra2);
}

/* 
 * SEE ALSO:
 * https://en.wikipedia.org/wiki/Japanese_era_name
 * https://docs.microsoft.com/en-us/windows/desktop/Intl/era-handling-for-the-japanese-calendar
 */
LPCJAPANESE_ERA JapaneseEra_Load(DWORD *pdwCount)
{
#ifndef DONT_USE_REGISTRY
    HANDLE KeyHandle = NULL;
    DWORD dwIndex;
    WCHAR szName[128], szValue[128];
    JAPANESE_ERA *pEntry;
    LPWSTR pch1, pch2, pch3, pch4;
#endif

    ASSERT(pdwCount != NULL);

    /* return cache if any */
    if (s_JapaneseEraCount != 0)
    {
        *pdwCount = s_JapaneseEraCount;
        return s_JapaneseEraTable;
    }

#ifdef DONT_USE_REGISTRY
    s_JapaneseEraCount = ARRAYSIZE(s_JapaneseEraTable);
#else
    /* init */
    *pdwCount = 0;
    RtlZeroMemory(&s_JapaneseEraTable, sizeof(s_JapaneseEraTable));

    /* open registry key */
    KeyHandle = NLS_RegOpenKey(NULL, L"\\Registry\\Machine\\SYSTEM\\"
        L"CurrentControlSet\\Control\\NLS\\Calendars\\Japanese\\Eras");
    if (!KeyHandle)
        return NULL;

    /* for all values */
    for (dwIndex = 0; dwIndex < JAPANESE_ERA_MAX; ++dwIndex)
    {
        pEntry = &s_JapaneseEraTable[dwIndex];

        /* get name and data */
        if (!NLS_RegEnumValue(KeyHandle, dwIndex, szName, sizeof(szName),
                              szValue, sizeof(szValue)))
        {
            break;
        }

        /* split fields */
        pch1 = szName;
        pch2 = wcschr(pch1, L' ');
        if (pch2 == NULL)
        {
            ASSERT(FALSE);
            break;
        }
        *pch2++ = UNICODE_NULL;

        pch3 = wcschr(pch2, L' ');
        if (pch3 == NULL)
        {
            ASSERT(FALSE);
            break;
        }
        *pch3++ = UNICODE_NULL;

        pEntry->wYear = _wtoi(pch1);
        pEntry->wMonth = _wtoi(pch2);
        pEntry->wDay = _wtoi(pch3);
        if (pEntry->wYear == 0 || pEntry->wMonth == 0 || pEntry->wDay == 0)
        {
            ASSERT(FALSE);
            break;
        }

        /* split fields */
        pch1 = szValue;
        pch2 = wcschr(pch1, L'_');
        if (pch2 == NULL)
        {
            ASSERT(FALSE);
            break;
        }
        *pch2++ = UNICODE_NULL;

        pch3 = wcschr(pch2, L'_');
        if (pch3 == NULL)
        {
            ASSERT(FALSE);
            break;
        }
        *pch3++ = UNICODE_NULL;

        pch4 = wcschr(pch3, L'_');
        if (pch4 == NULL)
        {
            ASSERT(FALSE);
            break;
        }
        *pch4++ = UNICODE_NULL;

        /* store */
        RtlStringCbCopyW(pEntry->szEraName, sizeof(pEntry->szEraName), pch1);
        RtlStringCbCopyW(pEntry->szEraInitial, sizeof(pEntry->szEraInitial), pch2);
        RtlStringCbCopyW(pEntry->szEnglishEraName, sizeof(pEntry->szEnglishEraName), pch3);
        RtlStringCbCopyW(pEntry->szEnglishEraInitial, sizeof(pEntry->szEnglishEraInitial), pch4);
    }

    /* close key */
    NtClose(KeyHandle);

    /* sort */
    qsort(s_JapaneseEraTable, s_JapaneseEraCount, sizeof(JAPANESE_ERA),
          JapaneseEra_Compare0);

    /* make cache */
    ASSERT(dwIndex > 0);
    s_JapaneseEraCount = dwIndex;
#endif

    *pdwCount = s_JapaneseEraCount;

    return s_JapaneseEraTable;
}

BOOL JapaneseEra_ToSystemTime(LPCJAPANESE_ERA pEra, LPSYSTEMTIME pst)
{
    ASSERT(pEra != NULL);
    ASSERT(pst != NULL);

    ZeroMemory(pst, sizeof(*pst));
    pst->wYear = pEra->wYear;
    pst->wMonth = pEra->wMonth;
    pst->wDay = pEra->wDay;
    return TRUE;
}

LPCJAPANESE_ERA JapaneseEra_Find(const SYSTEMTIME *pst OPTIONAL)
{
    DWORD dwIndex, dwCount = 0;
    LPCJAPANESE_ERA pTable, pEntry, pPrevEntry = NULL;
    SYSTEMTIME st1, st2;
    FILETIME ft1, ft2;
    LONG nCompare;

    /* pst --> ft1 */
    if (pst == NULL)
    {
        GetLocalTime(&st1);
        pst = &st1;
    }
    SystemTimeToFileTime(pst, &ft1);

    /* load era table */
    pTable = JapaneseEra_Load(&dwCount);
    if (pTable == NULL || dwCount == 0 || dwCount > JAPANESE_ERA_MAX)
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* for all eras */
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        pEntry = &pTable[dwIndex];

        /* pEntry --> st2 --> ft2 */
        JapaneseEra_ToSystemTime(pEntry, &st2);
        SystemTimeToFileTime(&st2, &ft2);

        /* ft1 <=> ft2 */
        nCompare = CompareFileTime(&ft1, &ft2);
        if (nCompare == 0)
            return pEntry;
        if (nCompare < 0)
            return pPrevEntry;
        pPrevEntry = pEntry;
    }

    return pPrevEntry;
}

LPCJAPANESE_ERA JapaneseEra_ConvertYear(const SYSTEMTIME *pst OPTIONAL, LPWORD pwNengoYearOut)
{
    SYSTEMTIME st;
    LPCJAPANESE_ERA pEra;
    ASSERT(pwNengoYearOut);

    if (pst == NULL)
    {
        GetLocalTime(&st);
        pst = &st;
    }

    pEra = JapaneseEra_Find(pst);
    ASSERT(pEra != NULL);
    if (pEra == NULL)
    {
        *pwNengoYearOut = 0;
        return NULL;
    }

    *pwNengoYearOut = pst->wYear - pEra->wYear + 1;
    return pEra;
}
