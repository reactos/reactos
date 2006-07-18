/*
 * ReactOS Sound Volume Control
 * Copyright (C) 2004-2005 Thomas Weidenmueller
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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        subsys/system/sndvol32/misc.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */
#include <sndvol32.h>

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
    if ((hrSrc = FindResourceW(hInst,
                               lpName,
                               (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst,
                             hrSrc)) &&
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

INT
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
            INT Ret = LoadStringW(hInst,
                                  uID,
                                  *lpTarget,
                                  ln);
            if (!Ret)
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
        /* let's use FormatMessage to format it because it has the ability to
           allocate memory automatically */
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

/* NOTE: do NOT modify SNDVOL_REG_LINESTATE for binary compatibility with XP! */
typedef struct _SNDVOL_REG_LINESTATE
{
    DWORD Flags;
    WCHAR LineName[MIXER_LONG_NAME_CHARS];
} SNDVOL_REG_LINESTATE, *PSNDVOL_REG_LINESTATE;

static const TCHAR AppRegSettings[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Volume Control");
static const TCHAR AppOptionsKey[] = TEXT("Options");
static const TCHAR LineStatesValue[] = TEXT("LineStates");
static const TCHAR StyleValue[] = TEXT("Style");

HKEY hAppSettingsKey = NULL;

BOOL
InitAppConfig(VOID)
{
    return RegCreateKeyEx(HKEY_CURRENT_USER,
                          AppRegSettings,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &hAppSettingsKey,
                          NULL) == ERROR_SUCCESS;
}

VOID
CloseAppConfig(VOID)
{
    if (hAppSettingsKey != NULL)
    {
        RegCloseKey(hAppSettingsKey);
        hAppSettingsKey = NULL;
    }
}

BOOL
ReadLineConfig(IN LPTSTR szDeviceName,
               IN LPTSTR szLineName,
               IN LPTSTR szControlName,
               OUT DWORD *Flags)
{
    HKEY hLineKey;
    DWORD Type;
    DWORD i, Size = 0;
    PSNDVOL_REG_LINESTATE LineStates = NULL;
    TCHAR szDevRegKey[MAX_PATH];
    BOOL Ret = FALSE;

    _stprintf(szDevRegKey,
              TEXT("%s\\%s"),
              szDeviceName,
              szLineName);

    if (RegCreateKeyEx(hAppSettingsKey,
                       szDevRegKey,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hLineKey,
                       NULL) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hLineKey,
                            LineStatesValue,
                            NULL,
                            &Type,
                            NULL,
                            &Size) != ERROR_SUCCESS ||
            Type != REG_BINARY ||
            Size == 0 || (Size % sizeof(SNDVOL_REG_LINESTATE) != 0))
        {
            goto ExitClose;
        }
        
        LineStates = HeapAlloc(GetProcessHeap(),
                               0,
                               Size);

        if (LineStates != NULL)
        {
            if (RegQueryValueEx(hLineKey,
                                LineStatesValue,
                                NULL,
                                &Type,
                                (LPBYTE)LineStates,
                                &Size) != ERROR_SUCCESS ||
                Type != REG_BINARY ||
                Size == 0 || (Size % sizeof(SNDVOL_REG_LINESTATE) != 0))
            {
                goto ExitClose;
            }
            
            /* try to find the control */
            for (i = 0; i < Size / sizeof(SNDVOL_REG_LINESTATE); i++)
            {
                if (!_tcscmp(szControlName,
                             LineStates[i].LineName))
                {
                    *Flags = LineStates[i].Flags;
                    Ret = TRUE;
                    break;
                }
            }
        }

ExitClose:
        HeapFree(GetProcessHeap(),
                 0,
                 LineStates);
        RegCloseKey(hLineKey);
    }
    
    return Ret;
}
