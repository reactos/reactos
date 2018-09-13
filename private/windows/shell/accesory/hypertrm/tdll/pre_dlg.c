/*	File: C:\WACKER\TDLL\PRE_DLG.C (Created: 12-Jan-1994)
 *	created from:
 *	File: C:\WACKER\TDLL\genrcdlg.c (Created: 16-Dec-1993)
 *	created from:
 *	File: C:\HA5G\ha5g\genrcdlg.c (Created: 12-Sep-1990)
 *
 *	Copyright 1990,1993,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#include "stdtyp.h"
#include "mc.h"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	int nOldHelpId;
	/*
	 * Put in whatever else you might need to access later
	 */
	};

typedef	struct stSaveDlgStuff SDS;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	PrintEchoDlg
 *
 * DESCRIPTION:
 *	This function is the dialog proc for the Print Echo dialog box.  No real
 *	suprises here.
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
BOOL CALLBACK PrintEchoDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->nOldHelpId = 0;

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);
		break;

	case WM_DESTROY:
		break;

	case WM_COMMAND:

		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case IDHELP:
			break;

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
