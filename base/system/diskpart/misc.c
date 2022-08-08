/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/misc.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
 */

#include "diskpart.h"

/* FUNCTIONS ******************************************************************/

BOOL
IsDecString(
    _In_ PWSTR pszDecString)
{
    PWSTR ptr;

    if ((pszDecString == NULL) || (*pszDecString == UNICODE_NULL))
        return FALSE;

    ptr = pszDecString;
    while (*ptr != UNICODE_NULL)
    {
        if (!iswdigit(*ptr))
            return FALSE;

        ptr++;
    }

    return TRUE;
}


BOOL
IsHexString(
    _In_ PWSTR pszHexString)
{
    PWSTR ptr;

    if ((pszHexString == NULL) || (*pszHexString == UNICODE_NULL))
        return FALSE;

    ptr = pszHexString;
    while (*ptr != UNICODE_NULL)
    {
        if (!iswxdigit(*ptr))
            return FALSE;

        ptr++;
    }

    return TRUE;
}


BOOL
HasPrefix(
    _In_ PWSTR pszString,
    _In_ PWSTR pszPrefix,
    _Out_opt_ PWSTR *ppszSuffix)
{
    INT nPrefixLength, ret;

    nPrefixLength = wcslen(pszPrefix);
    ret = _wcsnicmp(pszString, pszPrefix, nPrefixLength);
    if ((ret == 0) && (ppszSuffix != NULL))
        *ppszSuffix = &pszString[nPrefixLength];

    return (ret == 0);
}


ULONGLONG
RoundingDivide(
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor)
{
    return (Dividend + Divisor / 2) / Divisor;
}


PWSTR
DuplicateQuotedString(
    _In_ PWSTR pszInString)
{
    PWSTR pszOutString = NULL;
    PWSTR pStart, pEnd;
    INT nLength;

    if ((pszInString == NULL) || (pszInString[0] == UNICODE_NULL))
        return NULL;

    if (pszInString[0] == L'"')
    {
        if (pszInString[1] == UNICODE_NULL)
            return NULL;

        pStart = &pszInString[1];
        pEnd = wcschr(pStart, '"');
        if (pEnd == NULL)
        {
            nLength = wcslen(pStart);
        }
        else
        {
            nLength = (pEnd - pStart);
        }
    }
    else
    {
        pStart = pszInString;
        nLength = wcslen(pStart);
    }

    pszOutString = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   (nLength + 1) * sizeof(WCHAR));
    if (pszOutString == NULL)
        return NULL;

    wcsncpy(pszOutString, pStart, nLength);

    return pszOutString;
}


PWSTR
DuplicateString(
    _In_ PWSTR pszInString)
{
    PWSTR pszOutString = NULL;
    INT nLength;

    if ((pszInString == NULL) || (pszInString[0] == UNICODE_NULL))
        return NULL;

    nLength = wcslen(pszInString);
    pszOutString = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   (nLength + 1) * sizeof(WCHAR));
    if (pszOutString == NULL)
        return NULL;

    wcscpy(pszOutString, pszInString);

    return pszOutString;
}
