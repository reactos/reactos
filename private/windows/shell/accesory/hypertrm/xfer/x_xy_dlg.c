/*	File: C:\WACKER\xfer\x_xy_dlg.c (Created: 17-Jan-1993)
 *	created from:
 *	File: C:\WACKER\TDLL\genrcdlg.c (Created: 16-Dec-1993)
 *	created from:
 *	File: C:\HA5G\ha5g\genrcdlg.c (Created: 12-Sep-1990)
 *
 *	Copyright 1990,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\mc.h>

#include "xfer.h"
#include "xfer.hh"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	LPARAM lPar;
	};

typedef	struct stSaveDlgStuff SDS;

#define AUTO_ERRCHK 	101
#define CRC_ERRCHK		102
#define CSUM_ERRCHK 	103

#define CMPRS_ON		105
#define CMPRS_OFF		106

#define BYTE_SECONDS	110
#define PACKET_WAIT 	112
#define PACKET_SECONDS	108

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
BOOL CALLBACK XandYmodemParamsDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
#if defined(UPPER_FEATURES)
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	XFR_XY_PARAMS *pP;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		{
		RECT rc;
		DWORD dw;
		int nSize;
		HWND hwndUpDown;
		HWND hwndCtl;

		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->lPar = lPar;
		pP = (XFR_XY_PARAMS *)lPar;

		SetWindowLong(hDlg, DWL_USER, (LONG)pS);

		hwndCtl = GetDlgItem(hDlg, AUTO_ERRCHK);
		if (hwndCtl)
			{
			switch(pP->nErrCheckType)
				{
				case XP_ECP_CRC:
					nSize = CRC_ERRCHK;
					break;

				case XP_ECP_CHECKSUM:
					nSize = CSUM_ERRCHK;
					break;

				case XP_ECP_AUTOMATIC:
				default:
					nSize = AUTO_ERRCHK;
					break;
				}
			SendMessage(GetDlgItem(hDlg, nSize),
						BM_SETCHECK, TRUE, 0L);
			}

		hwndCtl = GetDlgItem(hDlg, CMPRS_ON);
		if (hwndCtl)
			{
			nSize = CMPRS_OFF;
			SendMessage(GetDlgItem(hDlg, nSize),
						BM_SETCHECK, TRUE, 0L);
			}

		GetClientRect(GetDlgItem(hDlg, PACKET_SECONDS), &rc);
		nSize = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		hwndUpDown = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nSize / 3) * 2,/* width */
								nSize,			/* height */
								hDlg,			/* parent window */
								PACKET_SECONDS + 1,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, PACKET_SECONDS),
								60,				/* upper limit */
								1,				/* lower limit */
								pP->nPacketWait);/* starting position */
		assert(hwndUpDown);

		GetClientRect(GetDlgItem(hDlg, BYTE_SECONDS), &rc);
		nSize = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		hwndUpDown = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nSize / 3) * 2,/* width */
								nSize,			/* height */
								hDlg,			/* parent window */
								BYTE_SECONDS + 1,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, BYTE_SECONDS),
								60,				/* upper limit */
								1,				/* lower limit */
								pP->nByteWait);/* starting position */
		assert(hwndUpDown);

		GetClientRect(GetDlgItem(hDlg, PACKET_WAIT), &rc);
		nSize = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		hwndUpDown = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nSize / 3) * 2,/* width */
								nSize,			/* height */
								hDlg,			/* parent window */
								PACKET_WAIT + 1,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, PACKET_WAIT),
								25,				/* upper limit */
								1,				/* lower limit */
								pP->nNumRetries);/* starting position */
		assert(hwndUpDown);
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
			if (pS)
				{
				int nCtl;
				int nVal;
				/*
				 * Do whatever saving is necessary
				 */

				pP = (XFR_XY_PARAMS *)pS->lPar;

				nCtl = XP_ECP_AUTOMATIC;
				if (GetDlgItem(hDlg, AUTO_ERRCHK))
					{
					if (IsDlgButtonChecked(hDlg, AUTO_ERRCHK))
						nCtl = XP_ECP_AUTOMATIC;
					if (IsDlgButtonChecked(hDlg, CRC_ERRCHK))
						nCtl = XP_ECP_CRC;
					if (IsDlgButtonChecked(hDlg, CSUM_ERRCHK))
						nCtl = XP_ECP_CHECKSUM;
					}
				pP->nErrCheckType = nCtl;

				nCtl = TRUE;
				if (GetDlgItem(hDlg, CMPRS_ON))
					{
					if (IsDlgButtonChecked(hDlg, CMPRS_ON))
						nCtl = TRUE;
					else
						nCtl = FALSE;
					}

				nVal = GetDlgItemInt(hDlg, PACKET_SECONDS, &nCtl, FALSE);
				pP->nPacketWait = nVal;

				nVal = GetDlgItemInt(hDlg, BYTE_SECONDS, &nCtl, FALSE);
				pP->nByteWait = nVal;

				nVal = GetDlgItemInt(hDlg, PACKET_WAIT, &nCtl, FALSE);
				pP->nNumRetries = nVal;

				/* Free the storeage */
				free(pS);
				pS = (SDS *)0;
				}
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *	Nothing.
 */
