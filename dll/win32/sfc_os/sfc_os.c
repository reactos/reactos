/*
 * System File Checker (Windows File Protection)
 *
 * Copyright 2008 Pierre Schweitzer
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"
#include "debug.h"

HINSTANCE hLibModule;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL);
            hLibModule = hinstDLL;
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }

    return TRUE;
}


/******************************************************************
 *              SfcIsFileProtected     [sfc_os.@]
 *
 * Check, if the given File is protected by the System
 *
 * PARAMS
 *  RpcHandle    [I] This must be NULL
 *  ProtFileName [I] Filename with Path to check
 *
 * RETURNS
 *  Failure: FALSE with GetLastError() != ERROR_FILE_NOT_FOUND
 *  Success: TRUE, when the File is Protected
 *           FALSE with GetLastError() == ERROR_FILE_NOT_FOUND,
 *           when the File is not Protected
 *
 *
 * BUGS
 *  We return always the Result for: "File is not Protected"
 *
 */
BOOL WINAPI SfcIsFileProtected(HANDLE RpcHandle, LPCWSTR ProtFileName)
{
    static BOOL reported = FALSE;

    if (reported) {
        DPRINT("(%p, %S) stub\n", RpcHandle, ProtFileName);
    }
    else
    {
        DPRINT1("(%p, %S) stub\n", RpcHandle, ProtFileName);
        reported = TRUE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/******************************************************************
 *              SfcIsKeyProtected     [sfc_os.@]
 *
 * Check, if the given Registry Key is protected by the System
 *
 * PARAMS
 *  hKey          [I] Handle to the root registry key
 *  lpSubKey      [I] Name of the subkey to check
 *  samDesired    [I] The Registry View to Examine (32 or 64 bit)
 *
 * RETURNS
 *  Failure: FALSE with GetLastError() != ERROR_FILE_NOT_FOUND
 *  Success: TRUE, when the Key is Protected
 *           FALSE with GetLastError() == ERROR_FILE_NOT_FOUND,
 *           when the Key is not Protected
 *
 */
BOOL WINAPI SfcIsKeyProtected(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired)
{
    static BOOL reported = FALSE;

    if (reported) {
        DPRINT("(%p, %S) stub\n", hKey, lpSubKey);
    }
    else
    {
        DPRINT1("(%p, %S) stub\n", hKey, lpSubKey);
        reported = TRUE;
    }

    if( !hKey ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

