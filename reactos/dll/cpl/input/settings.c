/*
 *  ReactOS
 *  Copyright (C) 2007 ReactOS Team
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
/*
 *
 * PROJECT:         		input.dll
 * FILE:            		dll/win32/input/settings.c
 * PURPOSE:         		input.dll
 * PROGRAMMER:      	Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

#include "resource.h"
#include "input.h"

#define BUFSIZE 80

static
BOOL
CreateDefaultLangList(HWND hWnd)
{
    HKEY hKey;
    char szPreload[BUFSIZE],szCount[BUFSIZE],Lang[BUFSIZE];
    DWORD dwBufLen = BUFSIZE, dwBufCLen = BUFSIZE, cValues;
	LONG lRet;
	int Count;
	LCID Lcid;

    if(RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return FALSE;
	}
    RegQueryInfoKey(hKey,NULL,NULL,NULL,NULL,NULL,NULL,&cValues,NULL,NULL,NULL,NULL);

	if (cValues)
	{
	    for (Count = 0; Count < cValues; Count++)
        {
            szCount[0] = '\0';
            lRet = RegEnumValue(hKey,Count,(LPTSTR)szCount,&dwBufCLen,NULL,NULL,NULL,NULL);

			sprintf(szCount,"%d",Count + 1);
			RegQueryValueEx(hKey,(LPTSTR)szCount,NULL,NULL,(LPBYTE)szPreload,&dwBufLen);

			Lcid = wcstoul((LPTSTR)szPreload, NULL, 16);
			GetLocaleInfo(Lcid, LOCALE_SLANGUAGE, (LPTSTR)Lang, sizeof(Lang));

			SendMessage(hWnd,
						CB_INSERTSTRING,
						0,
						(LPARAM)Lang);
			if (Count == 0)
			{
				SendMessage(hWnd,
							CB_SELECTSTRING,
							(WPARAM) -1,
							(LPARAM)Lang);
			}
        }

	}

	RegCloseKey(hKey);

	return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
SettingPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  switch (uMsg)
  {
    case WM_INITDIALOG:
		CreateDefaultLangList(GetDlgItem(hwndDlg, IDC_DEFAULT_INPUT_LANG));
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDC_LANG_BAR_BUTTON:
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_LANGBAR),
                          hwndDlg,
                          LangBarDlgProc);
            break;
            case IDC_KEY_SETTINGS_BUTTON:
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_KEYSETTINGS),
                          hwndDlg,
                          KeySettingsDlgProc);
            break;
            case IDC_ADD_BUTTON:
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_ADD),
                          hwndDlg,
                          AddDlgProc);
            break;
            case IDC_PROP_BUTTON:
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_INPUT_LANG_PROP),
                          hwndDlg,
                          InputLangPropDlgProc);
            break;
			case IDC_DEFAULT_INPUT_LANG:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
				}
			break;
        }
      break;
  }

  return FALSE;
}

/* EOF */
