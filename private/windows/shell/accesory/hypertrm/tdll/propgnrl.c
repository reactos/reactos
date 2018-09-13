/*	File: D:\WACKER\tdll\propphon.c (Created: 22-Feb-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */
#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include <winnls.h>

#include "assert.h"
#include "stdtyp.h"
#include "misc.h"
#include "mc.h"
#include "globals.h"
#include "session.h"
#include "load_res.h"
#include "statusbr.h"
#include "tdll.h"
#include "sf.h"
#include "file_msc.h"

#include <term\res.h>
#include <emu\emuid.h>
#include <emu\emu.h>
#include <emu\emudlgs.h>

#include "property.h"
#include "property.hh"

// Function prototypes...
//
STATIC_FUNC void prop_WM_INITDIALOG_General(HWND hDlg);
STATIC_FUNC void prop_SAVE_General(HWND hDlg);

STATIC_FUNC LRESULT prop_WM_CMD(const HWND hwnd, const int nId,
						        const int nNotify,  const HWND hwndCtrl);

STATIC_FUNC LRESULT prop_WM_NOTIFY(const HWND hwnd, const int nId);
STATIC_FUNC void propGetLast_Date(HWND hDlg, int nId, LPTSTR pszFile, int fGetAccess);

#define IDC_TF_NAME		102
#define IDC_TF_TYPE		105
#define IDC_TF_SIZE 	107
#define IDC_TF_LOCA 	109
#define IDC_TF_FILENAME	112
#define IDC_TF_CHANGED	114
#define IDC_TF_ACCESSED 116
#define IDC_IC_ICON		118
#define IDC_CK_READONLY 119
#define IDC_CK_HIDDEN   120
#define IDC_CK_ARCHIVE  121
#define IDC_CK_SYSTEM   122
#define IDC_TF_PATH     125


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	General Dialog
 *
 * DESCRIPTION:
 *	Dialog manager stub
 *
 * ARGUMENTS:
 *	Standard Windows dialog manager
 *
 * RETURNS:
 *	Standard Windows dialog manager
 *
 */
BOOL CALLBACK GeneralTabDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	pSDS pS;

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->hSession = (HSESSION)(((LPPROPSHEETPAGE)lPar)->lParam);

		CenterWindowOnWindow(GetParent(hDlg), sessQueryHwnd(pS->hSession));

		SetWindowLong(hDlg, DWL_USER, (LONG)pS);
		prop_WM_INITDIALOG_General(hDlg);

		break;

	case WM_DESTROY:
		break;

	case WM_NOTIFY:
		return prop_WM_NOTIFY(hDlg, ((NMHDR *)lPar)->code);

	case WM_COMMAND:
		return prop_WM_CMD(hDlg, LOWORD(wPar), HIWORD(wPar), (HWND)lPar);

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_NOTIFY
 *
 * DESCRIPTION:
 *  Process Property Sheet Notification messages.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC LRESULT prop_WM_NOTIFY(const HWND hDlg, const int nId)
	{
	pSDS	pS;
	TCHAR	ach[256];

	switch (nId)
		{
		default:
			break;

		case PSN_SETACTIVE:
			pS = (pSDS)GetWindowLong(hDlg, DWL_USER);

			sessQueryName(pS->hSession, ach, sizeof(ach));
			SetDlgItemText(hDlg, IDC_TF_NAME, ach);
			break;

		case PSN_KILLACTIVE:
			break;

		case PSN_APPLY:
			pS = (pSDS)GetWindowLong(hDlg, DWL_USER);

			//
			// Do whatever saving is necessary
			//
			prop_SAVE_General(hDlg);

			free(pS);		// free the storage
			pS = (pSDS)0;
			break;

		case PSN_RESET:
			// Cancel has been selected... good place to confirm.
			pS = (pSDS)GetWindowLong(hDlg, DWL_USER);
			free(pS);		// free the sotrage
			pS = (pSDS)0;
			break;

		case PSN_HASHELP:
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			break;

		case PSN_HELP:
			// Display help in whatever way is appropriate
			//WinHelp(...);
			break;
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_CMD
 *
 * DESCRIPTION:
 *  Process WM_COMMAND messages.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC LRESULT prop_WM_CMD(const HWND hDlg, const int nId,
						        const int nNotify,  const HWND hwndCtrl)
	{
	//pSDS	pS;

	switch(nId)
		{
	default:
		return FALSE;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_INITDIALOG_General
 *
 * DESCRIPTION:
 *  Do appropriate initializations to the General tab dialogs.
 *
 * ARGUMENTS:
 *  hDlg - dialog window.
 *
 * RETURNS:
 *
 */
STATIC_FUNC void prop_WM_INITDIALOG_General(HWND hDlg)
	{
	pSDS	pS;
	TCHAR	ach[256], acBuffer[256];
	HICON	hIcon;
	DWORD	nRet = 0L;
	TCHAR	acFile[256];
	unsigned long  ulFileSize;

	pS = (pSDS)GetWindowLong(hDlg, DWL_USER);

	// Set session name...
	//
	memset(ach, 0, sizeof(ach));
	sessQueryName(pS->hSession, ach, sizeof(ach));
	SetDlgItemText(hDlg, IDC_TF_NAME, ach);

	// Set session icon...
	//
	hIcon = sessQueryIcon(pS->hSession);
	if (hIcon != (HICON)0)
		SendDlgItemMessage(hDlg, IDC_IC_ICON, STM_SETICON, (WPARAM)hIcon, 0);

	// Display the type... TODO
	//
	SetDlgItemText(hDlg, IDC_TF_TYPE, TEXT("Wacker Connection"));

	// Display the file size... TODO
	//
	memset(acFile, 0, sizeof(acFile));
	sfGetSessionFileName(sessQuerySysFileHdl(pS->hSession),	sizeof(acFile)/sizeof(TCHAR), acFile);
	GetFileSizeFromName(acFile, &ulFileSize);

	LoadString((HINSTANCE)0, IDS_XD_KILO, ach, sizeof(ach) / sizeof(TCHAR));
	memset(acBuffer, 0, sizeof(acBuffer));
	wsprintf(acBuffer, ach, ulFileSize);
	SetDlgItemText(hDlg, IDC_TF_SIZE, acBuffer);

	// Display the location... TODO
	//
	SetDlgItemText(hDlg, IDC_TF_LOCA, TEXT("Wacker Folder"));

	// Display the MS-DOS name... TODO
	//
	SetDlgItemText(hDlg, IDC_TF_FILENAME, strippath(acFile));

	// Display the last changed date...
	//
	propGetLast_Date(hDlg, IDC_TF_CHANGED, acFile, FALSE);

	// Display the last modified date...
	//
	propGetLast_Date(hDlg, IDC_TF_ACCESSED, acFile, TRUE);

	// Display the Attributes...
	//
	nRet = GetFileAttributes(acFile);
	if (nRet == 0xFFFFFFF)
		{
		nRet = 0L;
		}

	SendDlgItemMessage(hDlg, IDC_CK_READONLY, BM_SETCHECK, nRet & FILE_ATTRIBUTE_READONLY, 0);
	SendDlgItemMessage(hDlg, IDC_CK_HIDDEN, BM_SETCHECK, nRet & FILE_ATTRIBUTE_HIDDEN, 0);
	SendDlgItemMessage(hDlg, IDC_CK_ARCHIVE, BM_SETCHECK, nRet & FILE_ATTRIBUTE_ARCHIVE, 0);
	SendDlgItemMessage(hDlg, IDC_CK_SYSTEM, BM_SETCHECK, nRet & FILE_ATTRIBUTE_SYSTEM, 0);

	// Display path... TODO
	//
	SetDlgItemText(hDlg, IDC_TF_PATH, stripname(acFile));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_SAVE_General
 *
 * DESCRIPTION:
 *  We are either applying the changes or closing the property sheet, so
 *  commit all of the changes.
 *
 * ARGUMENTS:
 *  hDlg - dialog handle.
 *
 * RETURNS:
 *
 */
STATIC_FUNC void prop_SAVE_General(HWND hDlg)
	{
	pSDS	pS;

	pS = (pSDS)GetWindowLong(hDlg, DWL_USER);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propGetLast_Date
 *
 * DESCRIPTION:
 *	Retrieve the file date for a given file.  The fGetAccess flag specifies
 *	whether to retrieve the last accessed time or the last write time.
 *
 * ARGUMENTS:
 *	pszFile - name of the file.
 *	pszDate - string receiving the date.
 *	fGetAccess - TRUE if we are to return the last accessed date.
 *
 * RETURNS:
 *
 */
STATIC_FUNC void propGetLast_Date(HWND hDlg, 	  int nId,
								  LPTSTR pszFile, int fGetAccess)
	{
	HANDLE		hFile;
	FILETIME	ftAccess, ftWrite;
	SYSTEMTIME	stSystem;
	TCHAR		acDate[10];

	hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Get file times...
	//
	if (!GetFileTime(hFile, NULL, &ftAccess, &ftWrite))
		return;

	// Convert file time to system time format.
	//
	if (!FileTimeToSystemTime((fGetAccess) ? &ftAccess : &ftWrite, &stSystem))
		return;

	// Get the date string formatted properly for a given locale.
	//
	memset(acDate, 0, sizeof(acDate));
	GetDateFormatA(GetUserDefaultLCID(), DATE_SHORTDATE, &stSystem, NULL, acDate, sizeof(acDate));
	SetDlgItemText(hDlg, nId, acDate);
	}
