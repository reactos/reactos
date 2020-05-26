/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Utility Functions related to Print Processors
* COPYRIGHT:   Copyright 2020 Doug Lyons (douglyons@douglyons.com)
*/

#include "precomp.h"

/*
 * Converts an incoming Unicode string to an ANSI string.
 * It is only useful for "in-place" conversions where the ANSI string goes
 * back into the same place where the Unicode string came into this function.
 *
 * It returns an error code.
 */
// TODO: It seems that many of the functions involving printing could use this.
DWORD UnicodeToAnsiInPlace(PWSTR pwszField)
{
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
        return ERROR_SUCCESS;
    }

    cch = wcslen(pwszField);
    if (cch == 0)
    {
        return ERROR_SUCCESS;
    }

    pszTemp = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
    if (!pszTemp)
    {
        ERR("HeapAlloc failed!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    WideCharToMultiByte(CP_ACP, 0, pwszField, -1, pszTemp, cch + 1, NULL, NULL);
    StringCchCopyA(pszField, cch + 1, pszTemp);

    HeapFree(hProcessHeap, 0, pszTemp);

    return ERROR_SUCCESS;
}
