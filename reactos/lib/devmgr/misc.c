/*
 * ReactOS Device Manager Applet
 * Copyright (C) 2004 - 2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id: devmgr.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/misc.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2005/11/24  Created
 */
#include <precomp.h>

HINSTANCE hDllInstance = NULL;

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
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

static INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd)
{
    int Index;

    Index = ListView_GetNextItem(hwnd,
                                 -1,
                                 LVNI_SELECTED);
    if (Index != -1)
    {
        LVITEM li;

        li.mask = LVIF_PARAM;
        li.iItem = Index;
        li.iSubItem = 0;

        if (ListView_GetItem(hwnd,
                             &li))
        {
            return li.lParam;
        }
    }

    return 0;
}

LPWSTR
ConvertMultiByteToUnicode(IN LPCSTR lpMultiByteStr,
                          IN UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    INT nLength;

    nLength = MultiByteToWideChar(uCodePage,
                                  0,
                                  lpMultiByteStr,
                                  -1,
                                  NULL,
                                  0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = HeapAlloc(GetProcessHeap(),
                             0,
                             nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
        return NULL;

    if (!MultiByteToWideChar(uCodePage,
                             0,
                             lpMultiByteStr,
                             nLength,
                             lpUnicodeStr,
                             nLength))
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}

HINSTANCE
LoadAndInitComctl32(VOID)
{
    typedef VOID (WINAPI *PINITCOMMONCONTROLS)(VOID);
    PINITCOMMONCONTROLS pInitCommonControls;
    HINSTANCE hComCtl32;

    hComCtl32 = LoadLibrary(L"comctl32.dll");
    if (hComCtl32 != NULL)
    {
        /* initialize the common controls */
        pInitCommonControls = (PINITCOMMONCONTROLS)GetProcAddress(hComCtl32,
                                                                  "InitCommonControls");
        if (pInitCommonControls == NULL)
        {
            FreeLibrary(hComCtl32);
            return NULL;
        }

        pInitCommonControls();
    }

    return hComCtl32;
}

BOOL
STDCALL
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
	    IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hDllInstance = hinstDLL;
            break;
    }

    return TRUE;
}
