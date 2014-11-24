/*
 * ReactOS Explorer
 *
 * Copyright 2013 - Edijs Kolesnikovics
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

ADVANCED_SETTINGS AdvancedSettings;
const WCHAR szAdvancedSettingsKey[] = TEXT("Software\\ReactOS\\Features\\Explorer");

VOID
LoadAdvancedSettings(VOID)
{
    HKEY hKey;

    /* Set defaults */
    AdvancedSettings.bShowSeconds = FALSE;

    /* Check registry */
    if (RegOpenKey(HKEY_CURRENT_USER, szAdvancedSettingsKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwValue, dwValueLength, dwType;

        dwValueLength = sizeof(dwValue);
        if (RegQueryValueEx(hKey, TEXT("ShowSeconds"), NULL, &dwType, (PBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD)
            AdvancedSettings.bShowSeconds = dwValue != 0;

        RegCloseKey(hKey);
    }
}

BOOL
SaveSettingDword(IN PCTSTR pszKeyName,
                 IN PCTSTR pszValueName,
                 IN DWORD dwValue)
{
    BOOL ret = FALSE;
    HKEY hKey;

    if (RegCreateKey(HKEY_CURRENT_USER, pszKeyName, &hKey) == ERROR_SUCCESS)
    {
        ret = RegSetValueEx(hKey, pszValueName, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue)) == ERROR_SUCCESS;

        RegCloseKey(hKey);
    }

    return ret;
}

/* EOF */
