/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Japanese era support
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <k32.h>

#define NDEBUG
#include <debug.h>
#include "japanese.h"

#define JAPANESE_ERA_MAX 16

/* #define DONT_USE_REGISTRY */

static BOOL         s_bIsGannenCached = FALSE;
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

void JapaneseEra_ClearCache(void)
{
    s_bIsGannenCached = FALSE;
    s_JapaneseEraCount = 0;
}

static INT JapaneseEra_Compare(const void *e1, const void *e2)
{
    PCJAPANESE_ERA pEra1 = (PCJAPANESE_ERA)e1;
    PCJAPANESE_ERA pEra2 = (PCJAPANESE_ERA)e2;
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

BOOL JapaneseEra_IsFirstYearGannen(void)
{
#ifdef DONT_USE_REGISTRY
    return TRUE;
#else
    HANDLE KeyHandle;
    DWORD dwIndex;
    WCHAR szName[32], szValue[32];
    static BOOL s_bFirstIsGannen = TRUE;

    if (s_bIsGannenCached)
        return s_bFirstIsGannen;

    KeyHandle = NLS_RegOpenKey(NULL, L"\\Registry\\Machine\\System\\"
        L"CurrentControlSet\\Control\\Nls\\Calendars\\Japanese");
    if (!KeyHandle)
        return TRUE;

    s_bFirstIsGannen = TRUE;
    for (dwIndex = 0; dwIndex < 16; ++dwIndex)
    {
        if (!NLS_RegEnumValue(KeyHandle, dwIndex, szName, sizeof(szName),
                              szValue, sizeof(szValue)))
        {
            break;
        }

        if (lstrcmpiW(szName, L"InitialEraYear") == 0)
        {
            s_bFirstIsGannen = (szValue[0] == 0x5143);
            break;
        }
    }

    NtClose(KeyHandle);

    s_bIsGannenCached = TRUE;

    return s_bFirstIsGannen;
#endif
}

/*
 * SEE ALSO:
 * https://en.wikipedia.org/wiki/Japanese_era_name
 * https://docs.microsoft.com/en-us/windows/desktop/Intl/era-handling-for-the-japanese-calendar
 */
static PCJAPANESE_ERA JapaneseEra_Load(DWORD *pdwCount)
{
#ifndef DONT_USE_REGISTRY
    HANDLE KeyHandle;
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
    KeyHandle = NLS_RegOpenKey(NULL, L"\\Registry\\Machine\\System\\"
        L"CurrentControlSet\\Control\\Nls\\Calendars\\Japanese\\Eras");
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
            break;
        }
        *pch2++ = UNICODE_NULL;

        pch3 = wcschr(pch2, L' ');
        if (pch3 == NULL)
        {
            break;
        }
        *pch3++ = UNICODE_NULL;

        pEntry->wYear = _wtoi(pch1);
        pEntry->wMonth = _wtoi(pch2);
        pEntry->wDay = _wtoi(pch3);
        if (pEntry->wYear == 0 || pEntry->wMonth == 0 || pEntry->wDay == 0)
        {
            break;
        }

        /* split fields */
        pch1 = szValue;
        pch2 = wcschr(pch1, L'_');
        if (pch2 == NULL)
        {
            break;
        }
        *pch2++ = UNICODE_NULL;

        pch3 = wcschr(pch2, L'_');
        if (pch3 == NULL)
        {
            break;
        }
        *pch3++ = UNICODE_NULL;

        pch4 = wcschr(pch3, L'_');
        if (pch4 == NULL)
        {
            break;
        }
        *pch4++ = UNICODE_NULL;

        /* store */
        RtlStringCbCopyW(pEntry->szEraName, sizeof(pEntry->szEraName), pch1);
        RtlStringCbCopyW(pEntry->szEraAbbrev, sizeof(pEntry->szEraAbbrev), pch2);
        RtlStringCbCopyW(pEntry->szEnglishEraName, sizeof(pEntry->szEnglishEraName), pch3);
        RtlStringCbCopyW(pEntry->szEnglishEraAbbrev, sizeof(pEntry->szEnglishEraAbbrev), pch4);
    }

    /* close key */
    NtClose(KeyHandle);

    /* sort */
    qsort(s_JapaneseEraTable, s_JapaneseEraCount, sizeof(JAPANESE_ERA),
          JapaneseEra_Compare);

    /* make cache */
    s_JapaneseEraCount = dwIndex;
#endif

    *pdwCount = s_JapaneseEraCount;

    return s_JapaneseEraTable;
}

static BOOL JapaneseEra_ToSystemTime(PCJAPANESE_ERA pEra, LPSYSTEMTIME pst)
{
    ASSERT(pEra != NULL);
    ASSERT(pst != NULL);

    ZeroMemory(pst, sizeof(*pst));
    pst->wYear = pEra->wYear;
    pst->wMonth = pEra->wMonth;
    pst->wDay = pEra->wDay;
    return TRUE;
}

PCJAPANESE_ERA JapaneseEra_Find(const SYSTEMTIME *pst OPTIONAL)
{
    DWORD dwIndex, dwCount = 0;
    PCJAPANESE_ERA pTable, pEntry, pPrevEntry = NULL;
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
