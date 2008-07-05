/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 only
 * FILE:        devutils/vgafontedit/misc.c
 * PURPOSE:     Some miscellaneous resource functions (copied from "devmgmt") and modified
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "precomp.h"

static INT
LengthOfStrResource(IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    PWSTR lpName, lpStr;

    /* There are always blocks of 16 strings */
    lpName = (PWSTR) MAKEINTRESOURCEW((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInstance, lpName, (PWSTR)RT_STRING)) != 0 &&
        (hRes = LoadResource(hInstance, hrSrc)) != 0 &&
        (lpStr = (PWSTR)LockResource(hRes)) != 0)
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */

        for (x = 0; x < uID; x++)
            lpStr += (*lpStr) + 1;

        /* Found the string */
        return (int)(*lpStr);
    }

    return -1;
}

INT
AllocAndLoadString(OUT PWSTR *lpTarget, IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(uID);

    if (ln++ > 0)
    {
        (*lpTarget) = (PWSTR) HeapAlloc( hProcessHeap, 0, ln * sizeof(WCHAR) );

        if (*lpTarget)
        {
            INT nRet;

            nRet = LoadStringW(hInstance, uID, *lpTarget, ln);

            if (!nRet)
                HeapFree(hProcessHeap, 0, *lpTarget);

            return nRet;
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
