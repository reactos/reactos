/*
 * 3D Text OpenGL Screensaver (settings.c)
 *
 * Copyright 2007 Marc Piulachs
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

/* This program can actually be entered from two very different contexts.
 * One is that it can be entered when a normal user is signed into Windows.
 * This is the case when it is called from the Desk CPL in the ScreenSaver
 * Tab. In this context, we will be able to open a registry key for the
 * HKEY_CURRENT_USER and use it to find the Display String to use.
 * The other case is when is is run when the screen saver timeout occurs.
 * Now we are running without a normal logged in user and if we try and open
 * HKEY_CURRENT_USER, we will get a Windows Error #2 - File not Found.
 * In this case, we will have to use the values in HKEY_USER\.Default to
 * find the Display String to use.
*/

#include "3dtext.h"

#include <winreg.h>
#include <strsafe.h>

WCHAR m_Text[MAX_PATH] = L"ReactOS Rocks!";

VOID LoadSettings(VOID)
{
    HKEY hkey;
    DWORD len = MAX_PATH * sizeof(WCHAR);
    DWORD dwRet;

    dwRet = RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\ScreenSavers\\Text3D",
                          0,
                          KEY_READ,
                          &hkey);

    if (dwRet == ERROR_SUCCESS)
    {
        dwRet = RegQueryValueExW(hkey,
                                 L"DisplayString",
                                 NULL,
                                 NULL,
                                 (LPBYTE)m_Text,
                                 &len);

        if( dwRet == ERROR_SUCCESS )
        {
            /* This makes sure that the settings are saved in two places. */
            SaveSettings();
        }
    }
    else
    {
        dwRet = RegOpenKeyExW(HKEY_USERS,
                              L".Default\\Software\\Microsoft\\ScreenSavers\\Text3D",
                              0,
                              KEY_READ,
                              &hkey);

        if( dwRet == ERROR_SUCCESS )
        {
            dwRet = RegQueryValueExW(hkey,
                                     L"DisplayString",
                                     NULL,
                                     NULL,
                                     (LPBYTE)m_Text,
                                     &len);

            if( dwRet == ERROR_SUCCESS )
            {
                /* This makes sure that the settings are saved in two places. */
                SaveSettings();
            }
        }
        else
        {
            /* Save Default Settings in two places. */
            SaveSettings();
        }
    }
}

VOID SaveSettings(VOID)
{
    HKEY hkey;
    size_t CbDestLength = 0;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\ScreenSavers\\Text3D", 0,
        L"", 0, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        StringCbLengthW(m_Text, sizeof(m_Text), &CbDestLength);
        RegSetValueExW(hkey, L"DisplayString", 0, REG_SZ, (LPBYTE)m_Text, CbDestLength + sizeof(WCHAR));
        RegCloseKey(hkey);
    }

    CbDestLength = 0;
    if (RegCreateKeyExW(HKEY_USERS, L".DEFAULT\\Software\\Microsoft\\ScreenSavers\\Text3D", 0,
        L"", 0, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        StringCbLengthW(m_Text, sizeof(m_Text), &CbDestLength);
        RegSetValueExW(hkey, L"DisplayString", 0, REG_SZ, (LPBYTE)m_Text, CbDestLength + sizeof(WCHAR));
        RegCloseKey(hkey);
    }
}
