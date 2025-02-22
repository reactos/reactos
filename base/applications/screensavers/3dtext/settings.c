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

#include "3dtext.h"

#include <winreg.h>

TCHAR m_Text[MAX_PATH] = _T("ReactOS Rocks!");

VOID LoadSettings(VOID)
{
	HKEY hkey;
	DWORD len = MAX_PATH * sizeof(TCHAR);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\ScreenSavers\\Text3D"), 0,
		_T(""), 0, KEY_READ, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hkey, _T("DisplayString"), NULL, NULL, (LPBYTE)m_Text, &len);
        RegCloseKey(hkey);
    }
}

VOID SaveSettings(VOID)
{
	HKEY hkey;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\ScreenSavers\\Text3D"), 0,
		_T(""), 0, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, _T("DisplayString"), 0, REG_SZ, (LPBYTE)m_Text, (_tcslen(m_Text) + 1) * sizeof(TCHAR));
        RegCloseKey(hkey);
    }
}
