/*	File: C:\WACKER\xfer\x_kr_dlg.c (Created: 27-Jan-1994)
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
#pragma hdrstop

#include <commctrl.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>

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

#define	CMP_CHECK		102

#define	BPP_UDC		104
#define	SWP_UDC		106
#define	ECS_UDC		108
#define	RTY_UDC		110
#define	PSC_UDC		112
#define	PEC_UDC		114
#define	NPD_UDC		116
#define	PDC_UDC		118

struct stUpDownControls
	{
	int nId;
	int nMin;
	int nMax;
	int nDef;
	};

typedef struct stUpDownControls UDC;

#if defined(UPPER_FEATURES)
UDC aUDC[10] =
	{
	{BPP_UDC, 20,  94,   94},
	{SWP_UDC, 5,   60,   5},
	{ECS_UDC, 1,   3,    1},
	{RTY_UDC, 1,   25,   5},
	{PSC_UDC, 0,   255,  1},
	{PEC_UDC, 0,   255,  13},
	{NPD_UDC, 0,   99,   0},
	{PDC_UDC, 0,   255,  0},
	{0, 0, 0, 0}
	};
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	KermitParamsDlg
 *
 * DESCRIPTION:
 *	The dialog proc for changing Kermit parameters.
 *
 * ARGUMENTS:
 *	Standard Windows dialog manager parameters
 *
 * RETURNS:
 *	Standard Windows dialog manager
 *
 */
BOOL CALLBACK KermitParamsDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
#if defined(UPPER_FEATURES)
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	XFR_KR_PARAMS *pK;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->lPar = lPar;				/* Save for later use */

		pK = (XFR_KR_PARAMS *)lPar;		/* We also need it for now */

		SetWindowLong(hDlg, DWL_USER, (LONG)pS);

		/*
		 * Build the Up/Down controls
		 */
		for (nId = 0; nId < 10; nId += 1)
			{
			RECT rc;
			HWND hwndTmp;
			DWORD dw;
			int nWide;

			if (aUDC[nId].nId != 0)
				{
				GetClientRect(GetDlgItem(hDlg, aUDC[nId].nId), &rc);
				nWide = rc.top - rc.bottom;
				dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
				dw |= UDS_ALIGNRIGHT;
				dw |= UDS_ARROWKEYS;
				dw |= UDS_SETBUDDYINT;
				hwndTmp = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nWide / 3) * 2,/* width */
								nWide,			/* height */
								hDlg,			/* parent window */
								aUDC[nId].nId + 100,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, aUDC[nId].nId),
								aUDC[nId].nMax,	/* upper limit */
								aUDC[nId].nMin,	/* lower limit */
								aUDC[nId].nDef);/* starting position */
				assert(hwndTmp);
				}
			}

		/*
		 * Set the controls to the correct values
		 */
		SetDlgItemInt(hDlg, BPP_UDC, pK->nBytesPerPacket, FALSE);
		SetDlgItemInt(hDlg, SWP_UDC, pK->nSecondsWaitPacket, FALSE);
		SetDlgItemInt(hDlg, ECS_UDC, pK->nErrorCheckSize, FALSE);
		SetDlgItemInt(hDlg, RTY_UDC, pK->nRetryCount, FALSE);
		SetDlgItemInt(hDlg, PSC_UDC, pK->nPacketStartChar, FALSE);
		SetDlgItemInt(hDlg, PEC_UDC, pK->nPacketEndChar, FALSE);
		SetDlgItemInt(hDlg, NPD_UDC, pK->nNumberPadChars, FALSE);
		SetDlgItemInt(hDlg, PDC_UDC, pK->nPadChar, FALSE);

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
			pK = (XFR_KR_PARAMS *)pS->lPar;

			pK->nBytesPerPacket    = GetDlgItemInt(hDlg, BPP_UDC, NULL, FALSE);
			pK->nSecondsWaitPacket = GetDlgItemInt(hDlg, SWP_UDC, NULL, FALSE);
			pK->nErrorCheckSize    = GetDlgItemInt(hDlg, ECS_UDC, NULL, FALSE);
			pK->nRetryCount        = GetDlgItemInt(hDlg, RTY_UDC, NULL, FALSE);
			pK->nPacketStartChar   = GetDlgItemInt(hDlg, PSC_UDC, NULL, FALSE);
			pK->nPacketEndChar     = GetDlgItemInt(hDlg, PEC_UDC, NULL, FALSE);
			pK->nNumberPadChars    = GetDlgItemInt(hDlg, NPD_UDC, NULL, FALSE);
			pK->nPadChar           = GetDlgItemInt(hDlg, PDC_UDC, NULL, FALSE);

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
