/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Some miscellaneous resource functions
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller (thomas@reactsoft.com)
 *              Copyright 2008 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static INT
LengthOfStrResource(IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCEW((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInstance, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInstance, hrSrc)) &&
        (lpStr = LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

int
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInstance, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

static DWORD
VarListLoadAndFormatString(IN UINT uID, OUT PWSTR *lpTarget, IN va_list* Args)
{
    DWORD Ret = 0;
    PWSTR lpFormat;

    if (AllocAndLoadString(&lpFormat, uID) > 0)
    {
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             Args);

        HeapFree(hProcessHeap, 0, lpFormat);
    }

    return Ret;
}

DWORD
LoadAndFormatString(IN UINT uID, OUT PWSTR *lpTarget, ...)
{
    DWORD Ret;
    va_list Args;

    va_start(Args, lpTarget);
    Ret = VarListLoadAndFormatString(uID, lpTarget, &Args);
    va_end(Args);

    return Ret;
}

VOID
LocalizedError(IN UINT uID, ...)
{
    PWSTR pszError;
    va_list Args;

    va_start(Args, uID);
    VarListLoadAndFormatString(uID, &pszError, &Args);
    va_end(Args);

    MessageBoxW(NULL, pszError, szAppName, MB_ICONERROR);
    HeapFree(hProcessHeap, 0, pszError);
}
