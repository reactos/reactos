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

LSTATUS WINAPI RegCopyTreeW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ HKEY);

/* FUNCTIONS ***************************************************************/

static
BOOL
CopyKey(HKEY hDstKey,
        HKEY hSrcKey)
{
    LONG Error;

    Error = RegCopyTreeW(hSrcKey,
                         NULL,
                         hDstKey);
    if (Error != ERROR_SUCCESS)
    {
        SetLastError((DWORD)Error);
        return FALSE;
    }

    return TRUE;
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
