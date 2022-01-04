/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/registry.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

static
BOOL
CopyKey(HKEY hDstKey,
        HKEY hSrcKey)
{
    LONG Error;

#if (_WIN32_WINNT >= 0x0600)
    Error = RegCopyTreeW(hSrcKey,
                         NULL,
                         hDstKey);
    if (Error != ERROR_SUCCESS)
    {
        SetLastError((DWORD)Error);
        return FALSE;
    }

    return TRUE;

#else
    FILETIME LastWrite;
    DWORD dwSubKeys;
    DWORD dwValues;
    DWORD dwType;
    DWORD dwMaxSubKeyNameLength;
    DWORD dwSubKeyNameLength;
    DWORD dwMaxValueNameLength;
    DWORD dwValueNameLength;
    DWORD dwMaxValueLength;
    DWORD dwValueLength;
    DWORD dwDisposition;
    DWORD i;
    LPWSTR lpNameBuffer;
    LPBYTE lpDataBuffer;
    HKEY hDstSubKey;
    HKEY hSrcSubKey;

    DPRINT ("CopyKey() called\n");

    Error = RegQueryInfoKey(hSrcKey,
                            NULL,
                            NULL,
                            NULL,
                            &dwSubKeys,
                            &dwMaxSubKeyNameLength,
                            NULL,
                            &dwValues,
                            &dwMaxValueNameLength,
                            &dwMaxValueLength,
                            NULL,
                            NULL);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKey() failed (Error %lu)\n", Error);
        SetLastError((DWORD)Error);
        return FALSE;
    }

    dwMaxSubKeyNameLength++;
    dwMaxValueNameLength++;

    DPRINT("dwSubKeys %lu\n", dwSubKeys);
    DPRINT("dwMaxSubKeyNameLength %lu\n", dwMaxSubKeyNameLength);
    DPRINT("dwValues %lu\n", dwValues);
    DPRINT("dwMaxValueNameLength %lu\n", dwMaxValueNameLength);
    DPRINT("dwMaxValueLength %lu\n", dwMaxValueLength);

    /* Copy subkeys */
    if (dwSubKeys != 0)
    {
        lpNameBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxSubKeyNameLength * sizeof(WCHAR));
        if (lpNameBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        for (i = 0; i < dwSubKeys; i++)
        {
            dwSubKeyNameLength = dwMaxSubKeyNameLength;
            Error = RegEnumKeyExW(hSrcKey,
                                  i,
                                  lpNameBuffer,
                                  &dwSubKeyNameLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &LastWrite);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Subkey enumeration failed (Error %lu)\n", Error);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                SetLastError((DWORD)Error);
                return FALSE;
            }

            Error = RegCreateKeyExW(hDstKey,
                                    lpNameBuffer,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE,
                                    NULL,
                                    &hDstSubKey,
                                    &dwDisposition);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Subkey creation failed (Error %lu)\n", Error);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                SetLastError((DWORD)Error);
                return FALSE;
            }

            Error = RegOpenKeyExW(hSrcKey,
                                  lpNameBuffer,
                                  0,
                                  KEY_READ,
                                  &hSrcSubKey);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                RegCloseKey(hDstSubKey);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                SetLastError((DWORD)Error);
                return FALSE;
            }

            if (!CopyKey(hDstSubKey,
                         hSrcSubKey))
            {
                DPRINT1("Error: %lu\n", GetLastError());
                RegCloseKey (hSrcSubKey);
                RegCloseKey (hDstSubKey);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return FALSE;
            }

            RegCloseKey(hSrcSubKey);
            RegCloseKey(hDstSubKey);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpNameBuffer);
    }

    /* Copy values */
    if (dwValues != 0)
    {
        lpNameBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxValueNameLength * sizeof(WCHAR));
        if (lpNameBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        lpDataBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxValueLength);
        if (lpDataBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            HeapFree(GetProcessHeap(),
                     0,
                     lpNameBuffer);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        for (i = 0; i < dwValues; i++)
        {
            dwValueNameLength = dwMaxValueNameLength;
            dwValueLength = dwMaxValueLength;
            Error = RegEnumValueW(hSrcKey,
                                  i,
                                  lpNameBuffer,
                                  &dwValueNameLength,
                                  NULL,
                                  &dwType,
                                  lpDataBuffer,
                                  &dwValueLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                HeapFree(GetProcessHeap(),
                         0,
                         lpDataBuffer);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                SetLastError((DWORD)Error);
                return FALSE;
            }

            Error = RegSetValueExW(hDstKey,
                                   lpNameBuffer,
                                   0,
                                   dwType,
                                   lpDataBuffer,
                                   dwValueLength);
            if (Error != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", Error);
                HeapFree(GetProcessHeap(),
                         0,
                         lpDataBuffer);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                SetLastError((DWORD)Error);
                return FALSE;
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpDataBuffer);

        HeapFree(GetProcessHeap(),
                 0,
                 lpNameBuffer);
    }

    DPRINT("CopyKey() done\n");

    return TRUE;
#endif
}


BOOL
CreateUserHive(LPCWSTR lpKeyName,
               LPCWSTR lpProfilePath)
{
    HKEY hDefaultKey = NULL;
    HKEY hUserKey = NULL;
    LONG Error;
    BOOL Ret = FALSE;

    DPRINT("CreateUserHive(%S) called\n", lpKeyName);

    Error = RegOpenKeyExW(HKEY_USERS,
                          L".Default",
                          0,
                          KEY_READ,
                          &hDefaultKey);
    if (Error != ERROR_SUCCESS)
    {
        SetLastError((DWORD)Error);
        goto Cleanup;
    }

    Error = RegOpenKeyExW(HKEY_USERS,
                          lpKeyName,
                          0,
                          KEY_ALL_ACCESS,
                          &hUserKey);
    if (Error != ERROR_SUCCESS)
    {
        SetLastError((DWORD)Error);
        goto Cleanup;
    }

    if (!CopyKey(hUserKey, hDefaultKey))
    {
        goto Cleanup;
    }

    if (!UpdateUsersShellFolderSettings(lpProfilePath,
                                        hUserKey))
    {
        goto Cleanup;
    }

    RegFlushKey(hUserKey);
    Ret = TRUE;

Cleanup:
    if (hUserKey != NULL)
        RegCloseKey (hUserKey);

    if (hDefaultKey != NULL)
        RegCloseKey (hDefaultKey);

    return Ret;
}

/* EOF */
