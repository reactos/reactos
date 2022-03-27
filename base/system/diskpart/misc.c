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
    _In_ PWSTR pszPrefix)
{
    return (_wcsnicmp(pszString, pszPrefix, wcslen(pszPrefix)) == 0);
}


ULONGLONG
RoundingDivide(
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor)
{
    return (Dividend + Divisor / 2) / Divisor;
}
