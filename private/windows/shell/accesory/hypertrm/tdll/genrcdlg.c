/*	File: C:\WACKER\TDLL\genrcdlg.c (Created: 16-Dec-1993)
 *	created from:
 *	File: C:\HA5G\ha5g\genrcdlg.c (Created: 12-Sep-1990)
 *
 *	Copyright 1990,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 3/26/99 8:07a $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "mc.h"

#include <tdll\misc.h>
#include <tdll\globals.h>
#include "hlptable.h"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	int	nDummyVariable;
	/*
	 * Put in whatever else you might need to access later
	 */
	};

typedef	struct stSaveDlgStuff SDS;

// Dialog control defines...
//
#define IDC_CB_
#define IDC_RB_
#define IDC_PB_

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	Generic Dialog
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
BOOL CALLBACK GenericDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	static  aHlpTable[] = {0,0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			break;
			}

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		break;

	case WM_DESTROY:
		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:

		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case IDOK:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			/*
			 * Do whatever saving is necessary
			 */

			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}
