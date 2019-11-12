/*
 * ShellProgram.cpp
 *
 * Copyright 2019 Katayama Hirofumi MZ.
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
#include "shellmenu.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern "C"
BOOL Shell_GetShellProgram(LPWSTR pszProgram, SIZE_T cchProgramMax)
{
    if (!cchProgramMax)
        return FALSE;

    LONG err;
    HKEY hKey;
    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                        0, KEY_READ, &hKey);
    if (err)
    {
        ERR("err: %ld\n", err);
        StringCchCopyW(pszProgram, cchProgramMax, L"explorer.exe");
        return FALSE;
    }

    DWORD cbData = cchProgramMax * sizeof(WCHAR);
    err = RegQueryValueExW(hKey, L"Shell", NULL, NULL, (LPBYTE)pszProgram, &cbData);
    if (err)
    {
        StringCchCopyW(pszProgram, cchProgramMax, L"explorer.exe");
        ERR("err: %ld\n", err);
    }

    RegCloseKey(hKey);

    return !err;
}
