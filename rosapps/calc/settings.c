/*
 *  ReactOS calc
 *
 *  settings.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
    
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
    
#include "main.h"
#include "settings.h"


BOOL CheckResult(LONG error)
{
    if (error != ERROR_SUCCESS) {
    	PTSTR msg;
    	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
	    	0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		    MessageBox(NULL, msg, szTitle, MB_ICONERROR | MB_OK);
    	else
	    	MessageBox(NULL, _T("Error"), szTitle, MB_ICONERROR | MB_OK);
    	LocalFree(msg);
        return FALSE;
    }
    return TRUE;
}

void LoadSettings(void)
{
    HKEY  hKey;
    DWORD dwSize;
    LONG  result;
    char  szSubKey[] = "Software\\ReactWare\\Calculator";

    // Open the key
    result = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey);
    if (!CheckResult(result)) return;

    // Read the settings
    dwSize = sizeof(CALC_TYPES);
    result = RegQueryValueEx(hKey, _T("Preferences"), NULL, NULL, (LPBYTE)&CalcType, &dwSize);
    if (!CheckResult(result)) goto abort;

abort:
    // Close the key
    RegCloseKey(hKey);
}

void SaveSettings(void)
{
    HKEY hKey;
    LONG  result;
    char szSubKey1[] = "Software";
    char szSubKey2[] = "Software\\ReactWare";
    char szSubKey3[] = "Software\\ReactWare\\Calculator";

    // Open (or create) the key
    hKey = NULL;
    result = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey1, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (!CheckResult(result)) return;
    RegCloseKey(hKey);
    hKey = NULL;
    result = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (!CheckResult(result)) return;
    RegCloseKey(hKey);
    hKey = NULL;
    result = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey3, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (!CheckResult(result)) return;

    // Save the settings
    result = RegSetValueEx(hKey, _T("Preferences"), 0, REG_DWORD, (LPBYTE)&CalcType, sizeof(CALC_TYPES));
    if (!CheckResult(result)) goto abort;

abort:
    // Close the key
    RegCloseKey(hKey);
}
