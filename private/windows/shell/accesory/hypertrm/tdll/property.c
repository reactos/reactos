/*	File: D:\WACKER\tdll\property.c (Created: 19-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <prsht.h>
#include <commctrl.h>
#include <time.h>

#include <tdll\assert.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\misc.h>
#include <tdll\globals.h>
#include <tdll\session.h>
#include <tdll\load_res.h>
#include <tdll\tchar.h>
#include <emu\emuid.h>
#include <emu\emu.h>
#include <emu\emudlgs.h>
#include <tdll\cnct.h>
#include <cncttapi\cncttapi.h>
#include <term\res.h>

#include "property.h"
#include "statusbr.h"
#include "tdll.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  DoInternalProperties
 *
 * DESCRIPTION:
 *  Display the property sheet as seen from within an open session, i.e., the
 *	general tab is not a part of it.
 *
 * PARAMETERS:
 *  hSession - the session handle.
 *	hwnd	 - window handle.
  *
 * RETURNS:
 *	Nothing.
 */
void DoInternalProperties(HSESSION hSession, HWND hwnd)
	{
	TCHAR			achName[256];
	PROPSHEETHEADER stH;
	HPROPSHEETPAGE  hP[2];
    PROPSHEETPAGE	stP;

    hP[0] = hP[1] = (HPROPSHEETPAGE)0;

	memset(&stP, 0, sizeof(stP));

	stP.dwSize		 = sizeof(PROPSHEETPAGE);
	stP.dwFlags 	 = 0;
	stP.hInstance	 = glblQueryDllHinst();
	stP.pszTemplate  = MAKEINTRESOURCE(IDD_TAB_PHONENUMBER);
	stP.pfnDlgProc	 = NewPhoneDlg;
	stP.lParam		 = (LPARAM)hSession;
	stP.pfnCallback  = 0;

	hP[0] = CreatePropertySheetPage(&stP);

	stP.dwSize		 = sizeof(PROPSHEETPAGE);
	stP.dwFlags 	 = 0;
	stP.hInstance	 = glblQueryDllHinst();
	stP.pszTemplate  = MAKEINTRESOURCE(IDD_TAB_TERMINAL);
	stP.pfnDlgProc	 = TerminalTabDlg;
	stP.lParam		 = (LPARAM)hSession;
	stP.pfnCallback  = 0;

	hP[1] = CreatePropertySheetPage(&stP);

    sessQueryName(hSession, achName, sizeof(achName));
	memset(&stH, 0, sizeof(stH));

	stH.dwSize 			= sizeof(PROPSHEETHEADER);
	stH.hwndParent 		= hwnd;
	stH.hInstance 		= glblQueryDllHinst();
	stH.pszCaption		= achName;
    stH.nPages			= 2;
	stH.nStartPage 		= 0;
	stH.phpage 			= hP;
	stH.dwFlags 		= PSH_PROPTITLE | PSH_NOAPPLYNOW;

	PropertySheet(&stH);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *  propUpdateTitle
 *
 * DESCRIPTION:
 *  When the user changes the name of the session we need to reflect the change
 *	in the property sheet title.  Right now it uses "Property sheet for <lpszStr>".
 *  in the english version. Since there appears to be no way to dynamically
 *  change property sheet's title, we implemented this function.  We avoid
 *  placing any part of the title in the resource file to prevent problems with
 *  international versions, order of words, etc. and most importantly possible
 *	discrepancy	with the title string used by Microsoft in the property sheet.
 *	Instead we read the current title, match on the old session name and replace
 *	it with the new session name.
 *
 * PARAMETERS:
 *  hSession 	- the session handle.
 *  hDlg 		- handle of the property sheet tab dialog.
 *  pachOldName - pointer to the old session name.
 *
 * RETURNS:
 *	Nothing.
 */
void propUpdateTitle(HSESSION hSession, HWND hDlg, LPTSTR pachOldName)
	{
	HWND	hwnd = GetParent(hDlg);
	TCHAR	acTitle[256], acName[256], acNewTitle[256];
	LPTSTR	pszStr, pszStr2;

	GetWindowText(hwnd, acTitle, sizeof(acTitle));
	sessQueryName(hSession, acName, sizeof(acName));
	if (acName[0] != TEXT('\0'))
		{
		TCHAR_Fill(acNewTitle, TEXT('\0'), sizeof(acNewTitle) / sizeof(TCHAR));

		// TODO: What if the session name matches title text, eg.
		// "Properties for properties"?
		// Also, looks like I will have to write my own strstr() but
		// let's wait and see what Mircorsoft will tell us...
		//
		// if ((pszStr = (LPTSTR)strstr(acTitle, pachOldName)) != NULL)
		if ((pszStr = StrCharStrStr(acTitle, pachOldName)) != NULL)
			{
			for (pszStr2 = pszStr;
				 *pszStr2 || *pachOldName != TEXT('\0');
				 pszStr2 = StrCharNext(pszStr2),
				 pachOldName = StrCharNext(pachOldName))
				 {
				 continue;
				 }

			*pszStr = TEXT('\0');

			StrCharCopy(acNewTitle, acTitle);
			StrCharCat(acNewTitle, acName);
			StrCharCat(acNewTitle, pszStr2);
			}

		SetWindowText(hwnd, acNewTitle);
		}
	}
