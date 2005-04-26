/*
 *  ReactOS
 *  Copyright (C) 2004, 2005 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/locale.c
 * PURPOSE:         Locale property page
 * PROGRAMMER:      Eric Kohl
 *                  Klemens Friedl
 *                  Aleksey Bragin
 */

#define WINVER 0x0501

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdio.h>

#include "intl.h"
#include "resource.h"

HWND hList;

BOOL CALLBACK LocalesEnumProc(
  LPTSTR lpLocale // locale id
)
{
	LCID lcid;
	TCHAR lang[255];
	int index;

	//swscanf(lpLocale, L"%lx", &lcid); // maybe use wcstoul?
	lcid = wcstoul(lpLocale, NULL, 16);

	GetLocaleInfo(lcid, LOCALE_SLANGUAGE, lang, sizeof(lang));

    index = SendMessageW(hList,
		   CB_ADDSTRING,
		   0,
		   (LPARAM)lang);

	SendMessageW(hList,
		   CB_SETITEMDATA,
		   index,
		   (LPARAM)lcid);

	return TRUE;
}


static VOID
CreateLanguagesList(HWND hwnd)
{
	TCHAR langSel[255];

	hList = hwnd;
	EnumSystemLocalesW(LocalesEnumProc, LCID_SUPPORTED);

	// Select current locale
	GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SLANGUAGE, langSel, sizeof(langSel)); // or should it be System and not user?
	
	SendMessageW(hList,
		   CB_SELECTSTRING,
		   -1,
		   (LPARAM)langSel);
}

/*
static VOID
ShowLanguagesList(HWND hwnd)
{
  TIME_ZONE_INFORMATION TimeZoneInfo;
  PTIMEZONE_ENTRY Entry;
  DWORD dwIndex;
  DWORD i;

  GetTimeZoneInformation(&TimeZoneInfo);

  dwIndex = 0;
  i = 0;
  Entry = TimeZoneListHead;
  while (Entry != NULL)
    {
      SendMessageW(hwnd,
		   CB_ADDSTRING,
		   0,
		   (LPARAM)Entry->Description);

      if (!wcscmp(Entry->StandardName, TimeZoneInfo.StandardName))
	dwIndex = i;

      i++;
      Entry = Entry->Next;
    }

  SendMessageW(hwnd,
	       CB_SETCURSEL,
	       (WPARAM)dwIndex,
	       0);
}
*/

/* Property page dialog callback */
INT_PTR CALLBACK
LocalePageProc(HWND hwndDlg,
	       UINT uMsg,
	       WPARAM wParam,
	       LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		CreateLanguagesList(GetDlgItem(hwndDlg, IDC_LANGUAGELIST));
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LANGUAGELIST:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
		}
		break;

	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;
			if (lpnm->code == PSN_APPLY)
			{
				// Apply changes
				LCID NewLcid;
				int iCurSel;
				char tmp[100];

				// Acquire new value
				iCurSel = SendMessageW(hList,
					CB_GETCURSEL,
					0,
					0);
				if (iCurSel == CB_ERR)
					break;

				NewLcid = SendMessageW(hList,
					CB_GETITEMDATA,
					iCurSel,
					0);

				if (NewLcid == CB_ERR)
					break;


				//TOOD: Actually set new locale

				sprintf(tmp, "%x, cursel=%d", NewLcid, iCurSel);
				MessageBoxA(hwndDlg, tmp, "debug", MB_OK);
			}
		}
		break;
	}
	return FALSE;
}


/* EOF */
