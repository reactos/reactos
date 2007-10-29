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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include "3dtext.h"

TCHAR m_Text[MAX_PATH];

void LoadSettings()
{
	HKEY hkey;
	DWORD len = MAX_PATH;

	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\ScreenSavers\\Text3D"), 0,
		_T(""), 0, KEY_READ, NULL, &hkey, NULL);

	if(RegQueryValueEx(hkey, _T("DisplayString"),  0, 0, (LPBYTE)m_Text, &len) != ERROR_SUCCESS)
	{
		_tcscpy(m_Text  , _TEXT("ReactOS Rocks!"));
	}

	RegCloseKey(hkey);
}

void SaveSettings()
{
	HKEY hkey;

	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\ScreenSavers\\Text3D"), 0,
		_T(""), 0, KEY_WRITE, NULL, &hkey, NULL);

	RegSetValueEx(hkey, _T("DisplayString"), 0, REG_SZ, (BYTE *)&m_Text, sizeof (m_Text));

	RegCloseKey(hkey);
}
