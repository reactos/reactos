/*	File: C:\WACKER\XFER\x_zm_dlg.c (Created: 17-Dec-1993)
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
#include <term\res.h>
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
	LPARAM lPar;
	HWND hwndWaitUpDown;
	};

typedef struct stSaveDlgStuff SDS;

/* These are the control ID numbers */

#define	AUTOSTART_CHECK			102

#define	USE_SENDER_PB			104
#define	USE_LOCAL_PB			105

#define	REC_NEGOTIATE_PB		107
#define	REC_NEVER_PB			108
#define	REC_ALWAYS_PB			109

#define	AO_COMBO				112

#define	SEND_NEGOTIATE_PB		114
#define	SEND_ONE_TIME_PB		115
#define	SEND_ALWAYS_PB			116

#define	STREAMING_PB			119
#define	WINDOWED_PB				120
#define	WINDOW_COMBO			121

#define	PACKET_COMBO			124

#define	WAIT_ROCKER				130
#define	ROCKER_ID				131
#define	WMAX					100

#define	BIT_16_PB				127
#define	BIT_32_PB				128

#define	EOL_PB					132

#define	ESC_CODE_PB				133

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ZmodemParamsDlg
 *
 * DESCRIPTION:
 *	This function is called to allow the user to modify the ZMODEM transfer
 *	protocol parameters.
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
BOOL CALLBACK ZmodemParamsDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
#if defined(UPPER_FEATURES)
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	int		nLoop;
	RECT    rc;
	DWORD	dw;
	SDS    *pD;
	XFR_Z_PARAMS *pZ;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		/* Save the parameter block for the exit path */
		pD = (SDS *)malloc(sizeof(SDS));
		if (pD == (SDS *)0)
			{
			/* Error, bail out, pull rip cord */
			/* TODO: decide if we need an error message */
			EndDialog(hDlg, FALSE);
			}

		pD->lPar = lPar;
		SetWindowLong(hDlg, DWL_USER, (LONG)pD);

		/* Get the parameter block to use now*/
		pZ = (XFR_Z_PARAMS *)lPar;

		/*
		 * Do the receiving stuff
		 */
		SendMessage(GetDlgItem(hDlg, AUTOSTART_CHECK),
					BM_SETCHECK, pZ->nAutostartOK, 0L);

		if (pZ->nFileExists == ZP_FE_SENDER)
			SendMessage(GetDlgItem(hDlg, USE_SENDER_PB),
						BM_SETCHECK, 1, 0L);
		else
			SendMessage(GetDlgItem(hDlg, USE_LOCAL_PB),
						BM_SETCHECK, 1, 0L);

		switch (pZ->nCrashRecRecv)
			{
			default:
			case ZP_CRR_NEG:
				SendMessage(GetDlgItem(hDlg, REC_NEGOTIATE_PB),
							BM_SETCHECK, 1, 0L);
				break;
			case ZP_CRR_NEVER:
				SendMessage(GetDlgItem(hDlg, REC_NEVER_PB),
							BM_SETCHECK, 1, 0L);
				break;
			case ZP_CRR_ALWAYS:
				SendMessage(GetDlgItem(hDlg, REC_ALWAYS_PB),
							BM_SETCHECK, 1, 0L);
				break;
			}

		/*
		 * Do the sending stuff
		 */

		for (nLoop = 0; nLoop < 8; nLoop += 1)
			{
			TCHAR acStr[64];

			LoadString(glblQueryDllHinst(),
						IDS_TM_SD_ONE + nLoop,
						acStr,
						sizeof(acStr) / sizeof(TCHAR));

			SendMessage(GetDlgItem(hDlg, AO_COMBO),
						CB_INSERTSTRING,
						(UINT)nLoop,
						(LONG)acStr);
			}
		SendMessage(GetDlgItem(hDlg, AO_COMBO),
					CB_SETCURSEL,
					pZ->nOverwriteOpt - 1,
					0L);

		if (pZ->nCrashRecSend == ZP_CRS_NEG)
			SendMessage(GetDlgItem(hDlg, SEND_NEGOTIATE_PB),
						BM_SETCHECK, 1, 0L);
		else
			SendMessage(GetDlgItem(hDlg, SEND_ALWAYS_PB),
						BM_SETCHECK, 1, 0L);

		/*
		 * Do the generic stuff
		 */
		for (nLoop = 0; nLoop < 16; nLoop += 1)
			{
			BYTE acBuffer[16];

			wsprintf(acBuffer, (LPSTR)"%d K", nLoop + 1);
			SendMessage(GetDlgItem(hDlg, WINDOW_COMBO),
						CB_INSERTSTRING,
						(UINT)nLoop,
						(LONG)((LPSTR)acBuffer));
			}
		SendMessage(GetDlgItem(hDlg, WINDOW_COMBO),
					CB_SETCURSEL, pZ->nWinSize, 0L);

		if (pZ->nXferMthd == ZP_XM_STREAM)
			{
			SendMessage(GetDlgItem(hDlg, STREAMING_PB),
						BM_SETCHECK, 1, 0L);
			EnableWindow(GetDlgItem(hDlg, WINDOW_COMBO), FALSE);
			}
		else
			{
			SendMessage(GetDlgItem(hDlg, WINDOWED_PB),
						BM_SETCHECK, 1, 0L);
			}

		for (nLoop = 0; nLoop < 6; nLoop += 1)
			{
			BYTE acBuffer[16];

			wsprintf(acBuffer, (LPSTR)"%d", (1 << (nLoop + 5)));
			SendMessage(GetDlgItem(hDlg, PACKET_COMBO),
						CB_INSERTSTRING,
						(UINT)nLoop,
						(LONG)((LPSTR)acBuffer));
			}
		SendMessage(GetDlgItem(hDlg, PACKET_COMBO),
					CB_SETCURSEL, pZ->nBlkSize, 0L);

		// SetDlgItemInt(hDlg, WAIT_ROCKER, pZ->nRetryWait, FALSE);
		GetClientRect(GetDlgItem(hDlg, WAIT_ROCKER), &rc);
		nLoop = rc.top - rc.bottom;
		dw = WS_CHILD | WS_BORDER | WS_VISIBLE;
		dw |= UDS_ALIGNRIGHT;
		dw |= UDS_ARROWKEYS;
		dw |= UDS_SETBUDDYINT;
		pD->hwndWaitUpDown = CreateUpDownControl(
								dw,				/* create window flags */
								rc.right,		/* left edge */
								rc.top,			/* top edge */
								(nLoop / 3) * 2,/* width */
								nLoop,			/* height */
								hDlg,			/* parent window */
								ROCKER_ID,
								(HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE),
								GetDlgItem(hDlg, WAIT_ROCKER),
								WMAX,			/* upper limit */
								1,				/* lower limit */
								pZ->nRetryWait);/* starting position */
		assert(pD->hwndWaitUpDown);

#if 0
		/* Do we still use rockers ? */
		SendMessage(GetDlgItem(hDlg, WAIT_ROCKER),
					RS_SETMIN,
					0, (LONG)1);
		SendMessage(GetDlgItem(hDlg, WAIT_ROCKER),
					RS_SETMAX,
					0, (LONG)WMAX);
		SendMessage(GetDlgItem(hDlg, WAIT_ROCKER),
					RS_SETVALUE,
					0, (LONG)wFlag);
#endif

		if (pZ->nCrcType == ZP_CRC_32)
			SendMessage(GetDlgItem(hDlg, BIT_32_PB),
						BM_SETCHECK, 1, 0L);
		else
			SendMessage(GetDlgItem(hDlg, BIT_16_PB),
						BM_SETCHECK, 1, 0L);

		if (pZ->nEolConvert)
			SendMessage(GetDlgItem(hDlg, EOL_PB),
						BM_SETCHECK, 1, 0L);

		if (pZ->nEscCtrlCodes)
			SendMessage(GetDlgItem(hDlg, ESC_CODE_PB),
						BM_SETCHECK, 1, 0L);
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
			pD = (SDS *)GetWindowLong(hDlg, DWL_USER);
			assert(pD);

			pZ = (XFR_Z_PARAMS *)pD->lPar;
			assert(pZ);
			/*
			 * TODO: decide how we are going to handle confirmable changes
			 */

			/*
			 * Do the receive stuff
			 */
			pZ->nAutostartOK = IsDlgButtonChecked(hDlg, AUTOSTART_CHECK);

			pZ->nFileExists = ZP_FE_DLG;
			if (IsDlgButtonChecked(hDlg, USE_SENDER_PB))
				pZ->nFileExists = ZP_FE_SENDER;

			if (IsDlgButtonChecked(hDlg, REC_NEGOTIATE_PB))
				pZ->nCrashRecRecv = ZP_CRR_NEG;
			else if (IsDlgButtonChecked(hDlg, REC_NEVER_PB))
				pZ->nCrashRecRecv = ZP_CRR_NEVER;
			else
				pZ->nCrashRecRecv = ZP_CRR_ALWAYS;

			/*
			 * Do the send stuff
			 */
			pZ->nOverwriteOpt = (LONG)SendMessage(GetDlgItem(hDlg, AO_COMBO),
										CB_GETCURSEL, 0, 0L);
			if (pZ->nOverwriteOpt == CB_ERR)
				{
				pZ->nOverwriteOpt = 1;
				}
			else
				{
				pZ->nOverwriteOpt += 1;			/* Zero vs. one base list */
				}

			pZ->nCrashRecSend = ZP_CRS_ALWAYS;
			if (IsDlgButtonChecked(hDlg, SEND_NEGOTIATE_PB))
				pZ->nCrashRecSend = ZP_CRS_NEG;
			else if (IsDlgButtonChecked(hDlg, SEND_ONE_TIME_PB))
				pZ->nCrashRecSend = ZP_CRS_ONCE;

			/*
			 * Do the generic stuff
			 */
			pZ->nWinSize = (LONG)SendMessage(GetDlgItem(hDlg, WINDOW_COMBO),
										CB_GETCURSEL, 0, 0L);
			if (pZ->nWinSize == CB_ERR)
				{
				pZ->nWinSize = 1;
				/* TODO: check the format */
				}

			if (IsDlgButtonChecked(hDlg, STREAMING_PB))
				pZ->nXferMthd = ZP_XM_STREAM;
			else
				pZ->nXferMthd = ZP_XM_WINDOW;

			pZ->nBlkSize = (LONG)SendMessage(GetDlgItem(hDlg, PACKET_COMBO),
										CB_GETCURSEL, 0, 0L);
			if (pZ->nBlkSize == CB_ERR)
				{
				/* TODO: check the format */
				pZ->nBlkSize = 1;
				}

			if (IsDlgButtonChecked(hDlg, BIT_32_PB))
				pZ->nCrcType = ZP_CRC_32;
			else
				pZ->nCrcType = ZP_CRC_16;

			/* TODO: remember that this used to be a rocker */
			pZ->nRetryWait = GetDlgItemInt(hDlg, WAIT_ROCKER, NULL, FALSE);
			if (pZ->nRetryWait < 5)
				pZ->nRetryWait = 5;
			if (pZ->nRetryWait > 100)
				pZ->nRetryWait = 100;

			pZ->nEolConvert = IsDlgButtonChecked(hDlg, EOL_PB);

			pZ->nEscCtrlCodes = IsDlgButtonChecked(hDlg, ESC_CODE_PB);

			free(pD);
			pD = (SDS *)0;
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			/* Not much to do except free the memory */
			pD = (SDS *)GetWindowLong(hDlg, DWL_USER);
			free(pD);
			pD = (SDS *)0;
			EndDialog(hDlg, FALSE);
			break;

		case STREAMING_PB:
			if (IsDlgButtonChecked(hDlg, STREAMING_PB))
				EnableWindow(GetDlgItem(hDlg, WINDOW_COMBO), FALSE);
			break;

		case WINDOWED_PB:
			if (IsDlgButtonChecked(hDlg, WINDOWED_PB))
				EnableWindow(GetDlgItem(hDlg, WINDOW_COMBO), TRUE);
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
