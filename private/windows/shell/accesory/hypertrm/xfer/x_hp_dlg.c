/*	File: C:\WACKER\XFER\x_hp_dlg.c (Created: 24-Jan-1994)
 *	created from:
 *	File: C:\WACKER\TDLL\genrcdlg.c (Created: 16-Dec-1993)
 *	created from:
 *	File: C:\HA5G\ha5g\genrcdlg.c (Created: 12-Sep-1990)
 *
 *	Copyright 1990,1993,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#include <commctrl.h>
#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\mc.h>

#define	RESYNC		102
#define	RESYNC_UD	202

#define BLOCKS		104
#define	BLOCKS_UD	204

#define CMP_ON	106
#define CMP_OFF 107

#define CRC 	109
#define CSUM	110

#define	RMIN	3
#define RMAX	60

#define SMIN	128
#define SMAX	16384

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	int nOldHelp;
	/*
	 * Put in whatever else you might need to access later
	 */
	};

typedef	struct stSaveDlgStuff SDS;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	HyperProtocolParamsDlg
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
BOOL CALLBACK HyperProtocolParamsDlg(HWND hDlg,
									UINT wMsg,
									WPARAM wPar,
									LPARAM lPar)
	{
#if defined(UPPER_FEATURES)
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		{
		RECT rc;
		int nLoop;
		DWORD dw;

		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		/*
		 * We need to create a couple of UP/DOWN controls
		 */
		GetClientRect(GetDlgItem(hDlg, RESYNC), &rc);
		nLoop = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		hwndChild = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nLoop / 3) * 2,/* width */
								nLoop,			/* height */
								hDlg,			/* parent window */
								RESYNC_UD,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, RESYNC),
								RMAX,			/* upper limit */
								RMIN,			/* lower limit */
								5);				/* starting position */
		assert(hwndChild);

		GetClientRect(GetDlgItem(hDlg, BLOCKS), &rc);
		nLoop = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		hwndChild = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nLoop / 3) * 2,/* width */
								nLoop,			/* height */
								hDlg,			/* parent window */
								BLOCKS_UD,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, BLOCKS),
								SMAX,			/* upper limit */
								SMIN,			/* lower limit */
								2048);			/* starting position */
		assert(hwndChild);

		SetWindowLong(hDlg, DWL_USER, (LONG)pS);
		}
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
		case IDOK:
			pS = (SDS *)GetWindowLong(hDlg, DWL_USER);
			/*
			 * Do whatever saving is necessary
			 */

			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			pS = (SDS *)GetWindowLong(hDlg, DWL_USER);
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

#endif
	return TRUE;
	}

