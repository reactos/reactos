/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Utility Functions related to Print Processors
* COPYRIGHT:   Copyright 2020 Doug Lyons (douglyons@douglyons.com)
*/

#include <strsafe.h>

BOOL UnicodeToAnsiInPlace(PWSTR pwszField)
{
    /*
     * This converts an incoming Unicode string to an ANSI string.
     * It returns FALSE on failure, otherwise it returns TRUE.
     * It is only useful for "in-place" conversions where the ANSI string goes
     * back into the same place where the Unicode string came into this function.
     * It seems that many of the functions involving printing can use this.
     */

    PSTR pszField;
    DWORD cch;
    BOOL bReturn = TRUE;

    /*
     * Map the incoming Unicode pwszField string to an ANSI one here so that we can do
     * in-place conversion. We read the Unicode input and then we write back the ANSI
     * conversion into the same buffer for use with our GetPrinterDriverA function
     */
    PSTR pwszOutput = (PSTR)pwszField;

    if (!pwszField)
    {
        goto Exit;
    }
    else
    {
        cch = wcslen(pwszField);
    }

    if (cch == 0)
    {
        goto Exit;
    }

    pszField = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(CHAR));
    if (!pszField)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ERR("HeapAlloc failed!\n");
        bReturn = FALSE;   // indicates a failure to be handled by caller
        goto Exit;
    }

    WideCharToMultiByte(CP_ACP, 0, pwszField, -1, pszField, cch + 1, NULL, NULL);
    StringCchCopyA(pwszOutput, cch + 1, pszField);

    HeapFree(hProcessHeap, 0, pszField);

Exit:

    return bReturn;
}
