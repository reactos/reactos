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

TASKBAR_SETTINGS TaskBarSettings;
const WCHAR szSettingsKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";
const WCHAR szAdvancedSettingsKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";

VOID
LoadTaskBarSettings(VOID)
{
    DWORD dwValue = NULL;
    
    LoadSettingDword(szAdvancedSettingsKey, L"TaskbarSizeMove", dwValue);
    TaskBarSettings.bLock = (dwValue == 0);
    
    LoadSettingDword(szAdvancedSettingsKey, L"ShowSeconds", dwValue);
    TaskBarSettings.bShowSeconds = (dwValue != 0);
    
    LoadSettingDword(szSettingsKey, L"EnableAutotray", dwValue);
    TaskBarSettings.bHideInactiveIcons = (dwValue != 0); 
    
    LoadSettingDword(szAdvancedSettingsKey, L"TaskbarGlomming", dwValue);
    TaskBarSettings.bGroupButtons = (dwValue != 0);
    
    TaskBarSettings.bShowQuickLaunch = TRUE;    //FIXME: Where is this stored, and how?
    
    /* FIXME: The following settings are stored in stuckrects2, do they have to be load here too? */
    TaskBarSettings.bShowClock = TRUE;
    TaskBarSettings.bAutoHide = FALSE;
    TaskBarSettings.bAlwaysOnTop = FALSE;

}

VOID
SaveTaskBarSettings(VOID)
{
    SaveSettingDword(szAdvancedSettingsKey, L"TaskbarSizeMove", TaskBarSettings.bLock);
    SaveSettingDword(szAdvancedSettingsKey, L"ShowSeconds", TaskBarSettings.bShowSeconds);
    SaveSettingDword(szSettingsKey, L"EnableAutotray", TaskBarSettings.bHideInactiveIcons);
    SaveSettingDword(szAdvancedSettingsKey, L"TaskbarGlomming", TaskBarSettings.bGroupButtons);
    
    /* FIXME: Show Clock, AutoHide and Always on top are stored in the stuckrects2 key but are not written to it with a click on apply. How is this done instead?
       AutoHide writes something to HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Desktop\Components\0 figure out what and why */
}

BOOL
LoadSettingDword(IN LPCWSTR pszKeyName,
                 IN LPCWSTR pszValueName,
                 OUT DWORD &dwValue)
{
    BOOL ret = FALSE;
    HKEY hKey;
    
    if (RegOpenKeyW(HKEY_CURRENT_USER, pszKeyName, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwValueLength, dwType;

        dwValueLength = sizeof(dwValue);
        ret = RegQueryValueExW(hKey, pszValueName, NULL, &dwType, (PBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD;
        
        RegCloseKey(hKey);
    }
    
    return ret;
}

BOOL
SaveSettingDword(IN LPCWSTR pszKeyName,
                 IN LPCWSTR pszValueName,
                 IN DWORD dwValue)
{
    BOOL ret = FALSE;
    HKEY hKey;

    if (RegCreateKeyW(HKEY_CURRENT_USER, pszKeyName, &hKey) == ERROR_SUCCESS)
    {
        ret = RegSetValueExW(hKey, pszValueName, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue)) == ERROR_SUCCESS;

        RegCloseKey(hKey);
    }

    return ret;
}

/* EOF */
