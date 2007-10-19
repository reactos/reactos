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
 * FILE:                 		dll/win32/input/add.c
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

HWND hLanguageList;

/*struct LangAndLayout
{
	TCHAR Lang;
	TCHAR Layout;
	TCHAR SubLayout;
} VarLang[133];

char *SubLang[133] = {}

VOID CreateLangStruct(VOID)
{
	UINT Count;
	TCHAR Layout[256];

	for(Count = 0; Count < END_LAYOUT - BEGIN_LAYOUT; Count++)
	{
		LoadString(hApplet,
				   Count,
				   Layout,
				   sizeof(Layout) / sizeof(TCHAR));
		strcpy(VarLang[Count].Layout,Layout);
	}
}*/

/* Language enumerate procedure */
BOOL
CALLBACK
LanguagesEnumProc(LPTSTR lpLanguage)
{
    LCID Lcid;
    TCHAR Lang[1024];
    int Index;

    Lcid = wcstoul(lpLanguage, NULL, 16);

    GetLocaleInfo(Lcid, LOCALE_SLANGUAGE, Lang, sizeof(Lang));
    Index = (int) SendMessage(hLanguageList,
                              CB_ADDSTRING,
                              0,
                              (LPARAM)Lang);

    SendMessage(hLanguageList,
                 CB_SETITEMDATA,
                 Index,
                 (LPARAM)Lcid);

    return TRUE;
}

/* Enumerate all installed language identifiers */
static
VOID
CreateLanguagesList(HWND hWnd)
{
    TCHAR LangSel[256];
    hLanguageList = hWnd;
    EnumSystemLocales(LanguagesEnumProc, LCID_INSTALLED);

	LoadString(hApplet,
			   IDS_SELECTED_LANGUAGE,
			   LangSel,
			   sizeof(LangSel) / sizeof(TCHAR));

    SendMessage(hLanguageList,
                CB_SELECTSTRING,
                (WPARAM) -1,
                (LPARAM)LangSel);
}

static
VOID
SelectCurrentLayout(HWND hWnd)
{
	TCHAR Layout[256];

	LoadString(hApplet,
			   IDS_SELECTED_LAYOUT,
			   Layout,
			   sizeof(Layout) / sizeof(TCHAR));
	SendMessage(hWnd,
			    CB_SELECTSTRING,
				(WPARAM) -1,
				(LPARAM)Layout);
}

INT_PTR CALLBACK
AddDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
			CreateLanguagesList(GetDlgItem(hDlg, IDC_INPUT_LANGUAGE_COMBO));
			CreateKeyboardLayoutList(GetDlgItem(hDlg, IDC_KEYBOARD_LAYOUT_COMBO));
			SelectCurrentLayout(GetDlgItem(hDlg, IDC_KEYBOARD_LAYOUT_COMBO));
        }
        case WM_COMMAND:
        {
			switch (LOWORD(wParam))
			{
				case IDC_INPUT_LANGUAGE_COMBO:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						SelectCurrentLayout(GetDlgItem(hDlg, IDC_KEYBOARD_LAYOUT_COMBO));
					}
				break;
				case IDOK:

				break;
				case IDCANCEL:
					EndDialog(hDlg,LOWORD(wParam));
					return TRUE;
				break;
			}
        }
        break;
    }

    return FALSE;
}

/* EOF */
