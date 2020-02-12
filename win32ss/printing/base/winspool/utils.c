/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Utility Functions related to Print Processors
* COPYRIGHT:   Copyright 2020 Doug Lyons (douglyons@douglyons.com)
*/

#include "precomp.h"

BOOL UnicodeToAnsiInPlace(PWSTR pwszField)
{
    /*
     * This converts an incoming Unicode string to an ANSI string.
     * It returns FALSE on failure, otherwise it returns TRUE.
     * It is only useful for "in-place" conversions where the ANSI string goes
     * back into the same place where the Unicode string came into this function.
     * It seems that many of the functions involving printing can use this.
     */

    PSTR pszTemp;
    DWORD cch;

    /*
     * Map the incoming Unicode pwszField string to an ANSI one here so that we can do
     * in-place conversion. We read the Unicode input and then we write back the ANSI
     * conversion into the same buffer for use with our GetPrinterDriverA function
     */
    PSTR pszField = (PSTR)pwszField;

    if (!pwszField)
    {
        return TRUE;
    }

    cch = wcslen(pwszField);
    if (cch == 0)
    {
        return TRUE;
    }

    pszTemp = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
    if (!pszField)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ERR("HeapAlloc failed!\n");
        return FALSE;   // indicates a failure to be handled by caller
    }

    WideCharToMultiByte(CP_ACP, 0, pwszField, -1, pszTemp, cch + 1, NULL, NULL);
    StringCchCopyA(pszField, cch + 1, pszTemp);

    HeapFree(hProcessHeap, 0, pszTemp);

    return TRUE;
}
