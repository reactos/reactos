/*	File: C:\WACKER\TDLL\XFDSPDLG.C (Created: 10-Jan-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\xfdspdlg.c (Created: 9-Oct-1992)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */
#include <windows.h>
#pragma hdrstop

// #define	DEBUGSTR	1

// As of 14-Apr-94 (build 89) still doesn't work
// As of the May Beta, it did work
#define	DO_FM	1

#include <term\res.h>
#include <term\xfer_dlg.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\misc.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\globals.h>
#include <tdll\xfer_msc.h>
#include <tdll\xfer_msc.hh>
#include <tdll\vu_meter.h>
#include <xfer\xfer.h>

#include "xfdspdlg.h"
#include "xfdspdlg.hh"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	HSESSION hSession;
	HWND	 hDlg;				/* our window handle */

	HBRUSH	 hBrush;			/* background brush */

	XD_TYPE *pstD;				/* transfer and display data */

	INT		 nIsCancelActive;	/* flag for cancel option */
	};

typedef	struct stSaveDlgStuff SDS;

VOID PASCAL xfrDisplayFunc(SDS *pstL);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	XfrDisplayDlg
 *
 * DESCRIPTION:
 *	This is the dialog function for the transfer display.  It is a little bit
 *	different in that it is a modeless dialog and it hangs around and shows
 *	the status of a ongoing transfer.
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK XfrDisplayDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;


	switch (wMsg)
		{
	case WM_INITDIALOG:
		{
		LPTSTR acPtrs[3];
		TCHAR acProto[64];
		TCHAR acFmt[64];
		TCHAR acName[128];
		TCHAR acBuffer[256];

		pS = (SDS *)malloc(sizeof(SDS));
		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			}

		pS->hSession = (HSESSION)lPar;
		pS->hDlg = hDlg;
		pS->hBrush = 0;
		pS->pstD = (XD_TYPE *)sessQueryXferHdl(pS->hSession);
		pS->nIsCancelActive = 0;

		mscCenterWindowOnWindow(hDlg, sessQueryHwnd(pS->hSession));

		/*
		 * We need to set the title on the display
		 */

		{
		int nIndex;
		int nState;
		XFR_PROTOCOL *pX;
		XFR_PARAMS *pP;
		/* This section is in braces because it may go into a function later */

		pP = (XFR_PARAMS *)0;
		xfrQueryParameters(sessQueryXferHdl(pS->hSession), (VOID **)&pP);
		assert(pP);

		if (pS->pstD->nDirection == XFER_RECV)
			nState = pP->nRecProtocol;
		else
			nState = pP->nSndProtocol;

		pX = (XFR_PROTOCOL *)0;
		xfrGetProtocols(pS->hSession, &pX);
		assert(pX);

		if (pX != (XFR_PROTOCOL *)0)
			{
			for (nIndex = 0; pX[nIndex].nProtocol != 0; nIndex += 1)
				{
				if (nState == pX[nIndex].nProtocol)
					{
					StrCharCopy(acProto, pX[nIndex].acName);
					break;
					}
				}
			free(pX);
			pX = NULL;
			}
		}

		sessQueryName(pS->hSession, acName, sizeof(acName) / sizeof(TCHAR));

		LoadString(glblQueryDllHinst(),
					(pS->pstD->nDirection == XFER_RECV) ?
									IDS_XD_RECV_TITLE : IDS_XD_SEND_TITLE,
					acFmt,
					sizeof(acFmt) / sizeof(TCHAR));

		acPtrs[0] = acProto;
		acPtrs[1] = acName;
		acPtrs[2] = 0;
#if defined(DO_FM)
		FormatMessage(
					FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					acFmt,
					0,				/* Message ID, ignored */
					0,				/* also ignored */
					acBuffer,		/* result */
					sizeof(acBuffer) / sizeof(TCHAR),
					(va_list *)&acPtrs[0]);
#else
		wsprintf(acBuffer, "%s transfer for %s", acPtrs[0], acPtrs[1]);
#endif

		SetWindowText(hDlg, acBuffer);

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);
		}
		break;

	case WM_CLOSE:
		{
		XD_TYPE *pX;

		pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
		assert(pS);
		pX = pS->pstD;
		assert(pX);
		if (pX)
			{
			pX->nUserCancel = XFER_ABORT;
			}
		}
		break;

	case WM_DESTROY:
		pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
		if (pS)
			{
			/* Free the storeage */
			free(pS);
			pS = (SDS *)0;
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);
			}
		break;

	case WM_DLG_TO_DISPLAY:
		// DbgOutStr("WM_DLG_TO_DISPLAY\r\n", 0,0,0,0,0);

		switch(wPar)
			{
			case XFR_SINGLE_TO_DOUBLE:
				{
				HWND hwndOld;
				XD_TYPE *pX;

				// DbgOutStr("XFR_SINGLE_TO_DOUBLE\r\n", 0,0,0,0,0);

				pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
				if (pS)
					{
					pX = pS->pstD;
					assert(pX);

					if (pX->nExpanded)
						{
						break;
						}

					pX->nExpanded = TRUE;

					if (pX->nLgSingleTemplate == pX->nLgMultiTemplate)
						{
						break;
						}

					/* Must actually need to change */

					DbgOutStr("New Display!!!\r\n", 0,0,0,0,0);

					hwndOld = pX->hwndXfrDisplay;

					pX->hwndXfrDisplay = DoModelessDialog(glblQueryDllHinst(),
										MAKEINTRESOURCE(pX->nLgMultiTemplate),
										sessQueryHwnd(pS->hSession),
										XfrDisplayDlg,
										(LPARAM)pS->hSession);
					assert(pX->hwndXfrDisplay);

					//JMH 03-11-96: Originally PostMessage, but it turns out
					// some of the flags could be updated and reset by the old
					// progress dialog after they were set below. Changing this
					// to SemdMessage forces the old dialog to end immediately.
					//
					SendMessage(sessQueryHwnd(pS->hSession),
								WM_SESS_ENDDLG,
								0, (LPARAM)hwndOld);

					pX->bChecktype     = 1;
					pX->bErrorCnt      = 1;
					pX->bPcktErrCnt    = 1;
					pX->bLastErrtype   = 1;
					pX->bTotalSoFar    = 1;
					pX->bFileSize      = 1;
					pX->bFileSoFar     = 1;
					pX->bPacketNumber  = 1;
					pX->bTotalCnt      = 1;
					pX->bTotalSize     = 1;
					pX->bFileCnt       = 1;
					pX->bEvent         = 1;
					pX->bStatus        = 1;
					pX->bElapsedTime   = 1;
					pX->bRemainingTime = 1;
					pX->bThroughput    = 1;
					pX->bProtocol      = 1;
					pX->bMessage       = 1;
					pX->bOurName       = 1;
					pX->bTheirName     = 1;

					// xfrDisplayFunc(pS);
					PostMessage(pX->hwndXfrDisplay,
								WM_DLG_TO_DISPLAY,
								XFR_UPDATE_DLG, 0);
					}
				}
				break;
			case XFR_BUTTON_PUSHED:
				/* Probably not needed any more */
				break;
			case XFR_UPDATE_DLG:
				// DbgOutStr("XFR_UPDATE_DLG\r\n", 0,0,0,0,0);
				/* Update the display */
				pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

				//assert(pS->pstD->bTheirName == 1);

				if (pS)
					{
					xfrDisplayFunc(pS);
					}
				break;
			default:
				break;
			}
		break;

	case WM_COMMAND:

		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case XFR_SHRINK:
			/* Not a feature in Lower Wacker */
			break;

		case XFR_SKIP:
			/* Only for some protocols */
			{
			XD_TYPE *pX;

			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			assert(pS);
			pX = pS->pstD;
			assert(pX);
			if (pX)
				{
				pX->nUserCancel = XFER_SKIP;
				}
			}
			break;

		case XFR_CANCEL:   // Yes, XFER_CANCEL and IDCANCEL
		case IDCANCEL:		// go together. - mrw
			{
			XD_TYPE *pX;

			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			assert(pS);
			pX = pS->pstD;
			assert(pX);
			if (pX)
				{
				pX->nUserCancel = XFER_ABORT;
				}
			}
			break;

		case XFR_EXPAND:
			/* Not a feature in Lower Wacker */
			break;

		case XFR_CBPS:
			{
			XD_TYPE *pX;

			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			assert(pS);
			pX = pS->pstD;
			assert(pX);
			if (pX)
				{
				if (pX->nBps)
					{
					pX->nBps = FALSE;
					}
				else
					{
					pX->nBps = TRUE;
					}
				}
			}
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	xfrDisplayFunc
 *
 * DESCRIPTION:
 *	Multiplex timer callback routine used for transfer display
 *
 * ARGUMENTS:
 *	DWORD	dwData	- double word data value passed thru timer
 *	ULONG	uTime	- contains time elapsed.
 *
 * RETURNS:
 *	TRUE always
 *
 */
VOID PASCAL xfrDisplayFunc(SDS *pstL)
	{
	XD_TYPE *pstD;
	HWND hwnd;
	UCHAR acBuffer[64];

	pstD = pstL->pstD;

	//assert(pstD->bTheirName == 1);

	if (pstD->bTheirName)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_THEIR_NAME_BOX);
		if (hwnd)
			{
			SetWindowText(hwnd, pstD->acTheirName);
			pstD->bTheirName = 0;
			}
		}

	if (pstD->bOurName)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_OUR_NAME_BOX);
		if (hwnd)
			{
			SetWindowText(hwnd, pstD->acOurName);
			pstD->bOurName = 0;
			}
		}

	if (pstD->bTotalCnt)
		{
		if (pstD->wTotalCnt > 1)
			{
			assert(pstD->hwndXfrDisplay);
			if (pstD->hwndXfrDisplay)
				{
				SendMessage(pstD->hwndXfrDisplay,
							WM_DLG_TO_DISPLAY,
							XFR_SINGLE_TO_DOUBLE,
							0L);
				pstD->bTotalCnt = 0;
				return;
				}
			}
		}

	if (pstD->bFileCnt)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_FILES_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];
			DWORD Args[2];

			LoadString(glblQueryDllHinst(),
						pstD->wTotalCnt ? IDS_XD_I_OF_I : IDS_XD_ONLY_1,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			Args[0] = (DWORD)pstD->wFileCnt;
			Args[1] = (DWORD)pstD->wTotalCnt;

			FormatMessage(FORMAT_MESSAGE_FROM_STRING |
						  FORMAT_MESSAGE_ARGUMENT_ARRAY,
						  acMsg,
						  0,
						  0,
						  acBuffer,
						  sizeof(acBuffer),
						  (va_list *)Args);

			//wsprintf(acBuffer, acMsg,
			//		  pstD->wFileCnt,
			//		  pstD->wTotalCnt);
#if FALSE
			if (pstD->wTotalCnt == 0)
				{
				wsprintf(acBuffer,
						 "%d",
						 pstD->wFileCnt);
				}
			else
				{
				wsprintf(acBuffer,
						 "%d of %d",
						 pstD->wFileCnt,
						 pstD->wTotalCnt);
				}
#endif
			SetWindowText(hwnd, acBuffer);
			pstD->bFileCnt = 0;
			}
		}

	if (pstD->bEvent)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_EVENT_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						xfrGetEventBase(sessQueryXferHdl(pstL->hSession))
							+ pstD->wEvent,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));
			SetWindowText(hwnd, acMsg);
			pstD->bEvent = 0;
			}
		}

	if (pstD->bPacketNumber)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_PACKET_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						IDS_XD_INT,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer, acMsg, pstD->lPacketNumber);
			SetWindowText(hwnd, acBuffer);
			pstD->bPacketNumber = 0;
			}
		}

	if (pstD->bErrorCnt)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_RETRIES_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						IDS_XD_INT,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer,
					 acMsg,
					 pstD->wErrorCnt);
			SetWindowText(hwnd, acBuffer);
			pstD->bErrorCnt = 0;
			}
		}

	if (pstD->bStatus)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_STATUS_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						xfrGetStatusBase(sessQueryXferHdl(pstL->hSession))
							+ pstD->wStatus,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));
			SetWindowText(hwnd, acMsg);
			pstD->bStatus = 0;
			}
		}

	if (pstD->bFileSize)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_FILE_METER);
		if (hwnd)
			{
			SendMessage(hwnd,
						WM_VU_SETMAXRANGE,
						0, pstD->lFileSize);
			SendMessage(hwnd,
						WM_VU_SETCURVALUE,
						0, 0L);
			}
		hwnd = GetDlgItem(pstL->hDlg, XFR_FILE_SIZE_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						IDS_XD_KILO,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer,
					 acMsg,
					 (UINT)((pstD->lFileSize + 1023) / 1024)
					);
			SetWindowText(hwnd, acBuffer);
			}
		pstD->bFileSize = 0;
		}

	if (pstD->bFileSoFar)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_FILE_METER);
		if (hwnd)
			{
			SendMessage(hwnd,
						WM_VU_SETCURVALUE,
						0, pstD->lFileSoFar);
			}

		hwnd = GetDlgItem(pstL->hDlg, XFR_FILE_BOX);
		if (hwnd)
			{
			LPTSTR acPtrs[3];
			TCHAR acNumF[8];
			TCHAR acNum1[32];
			TCHAR acNum2[32];
			TCHAR acMsg[128];
			// DbgOutStr("display filesofar %ld %ld 0x%lx\r\n",
			//			pstD->lFileSoFar, pstD->lFileSize, pstD, 0,0);

#if FALSE
			/* Changed to what follows for Internationalization */
			LoadString(glblQueryDllHinst(),
						pstD->lFileSize ? IDS_XD_K_OF_K : IDS_XD_KILO,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer, acMsg,
					 (UINT)((pstD->lFileSoFar + 1023) / 1024),
					 (UINT)((pstD->lFileSize + 1023) / 1024));
#endif
			if (pstD->lFileSize)
				{
				LoadString(glblQueryDllHinst(),
							IDS_XD_K_OF_K,
							acMsg,
							sizeof(acMsg) / sizeof(TCHAR));
				LoadString(glblQueryDllHinst(),
							IDS_XD_INT,
							acNumF,
							sizeof(acNumF) / sizeof(TCHAR));
				wsprintf(acNum1, acNumF, ((pstD->lFileSoFar + 1023) / 1024));
				wsprintf(acNum2, acNumF, ((pstD->lFileSize + 1023) / 1024));
				acPtrs[0] = acNum1;
				acPtrs[1] = acNum2;
				acPtrs[2] = NULL;
#if defined(DO_FM)
				FormatMessage(
					FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					acMsg,
					0,				/* String ID, ignored */
					0,				/* Also ignored */
					acBuffer,
					sizeof(acBuffer) / sizeof(TCHAR),
					(va_list *)&acPtrs[0]
					);
#else
				wsprintf(acBuffer, "%sk of %sK", acPtrs[0], acPtrs[1]);
#endif
				}
			else
				{
				LoadString(glblQueryDllHinst(),
							IDS_XD_KILO,
							acMsg,
							sizeof(acMsg) / sizeof(TCHAR));

				wsprintf(acBuffer, acMsg,
						 (UINT)((pstD->lFileSoFar + 1023) / 1024),
						 (UINT)((pstD->lFileSize + 1023) / 1024));
				}
			SetWindowText(hwnd, acBuffer);
			}
		pstD->bFileSoFar = 0;
		}

	if (pstD->bTotalSoFar)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_TOTAL_METER);
		// DbgOutStr("TotalSoFar %ld 0x%x\r\n", pstD->lTotalSoFar, hwnd, 0,0,0);
		if (hwnd)
			{
			SendMessage(hwnd,
						WM_VU_SETCURVALUE,
						0, pstD->lTotalSoFar);
			}

		hwnd = GetDlgItem(pstL->hDlg, XFR_TOTAL_BOX);
		if (hwnd)
			{
			LPTSTR acPtrs[3];
			TCHAR acNumF[8];
			TCHAR acNum1[32];
			TCHAR acNum2[32];
			TCHAR acMsg[128];
			// DbgOutStr("display totalsofar %ld %ld 0x%lx\r\n",
			//			pstD->lTotalSoFar, pstD->lTotalSize, pstD, 0,0);

#if FALSE
			/* Changed to what follows for Internationalization */
			LoadString(glblQueryDllHinst(),
						pstD->lTotalSize ? IDS_XD_K_OF_K : IDS_XD_KILO,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer, acMsg,
					 (UINT)((pstD->lTotalSoFar + 1023) / 1024),
					 (UINT)((pstD->lTotalSize + 1023) / 1024)
				 );
#endif
			if (pstD->lFileSize)
				{
				LoadString(glblQueryDllHinst(),
							IDS_XD_K_OF_K,
							acMsg,
							sizeof(acMsg) / sizeof(TCHAR));
				LoadString(glblQueryDllHinst(),
							IDS_XD_INT,
							acNumF,
							sizeof(acNumF) / sizeof(TCHAR));
				wsprintf(acNum1, acNumF, ((pstD->lTotalSoFar + 1023) / 1024));
				wsprintf(acNum2, acNumF, ((pstD->lTotalSize + 1023) / 1024));
				acPtrs[0] = acNum1;
				acPtrs[1] = acNum2;
				acPtrs[2] = NULL;
#if defined(DO_FM)
				FormatMessage(
					FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					acMsg,
					0,				/* String ID, ignored */
					0,				/* Also ignored */
					acBuffer,
					sizeof(acBuffer) / sizeof(TCHAR),
					(va_list *)&acPtrs[0]
					);
#else
				wsprintf(acBuffer, "%sk of %sK", acPtrs[0], acPtrs[1]);
#endif
				}
			else
				{
				LoadString(glblQueryDllHinst(),
							IDS_XD_KILO,
							acMsg,
							sizeof(acMsg) / sizeof(TCHAR));

				wsprintf(acBuffer, acMsg,
						 (UINT)((pstD->lTotalSoFar + 1023) / 1024),
						 (UINT)((pstD->lTotalSize + 1023) / 1024));
				}
			SetWindowText(hwnd, acBuffer);
			}

		if (pstD->lTotalSize != 0)
			{
			xfrSetPercentDone(sessQueryXferHdl(pstL->hSession),
						(int)((pstD->lTotalSoFar * 100L) / pstD->lTotalSize));
			}

		pstD->bTotalSoFar = 0;
		}

	if (pstD->bTotalSize)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_TOTAL_METER);
		// DbgOutStr("TotalSize %ld 0x%x\r\n", pstD->lTotalSize, hwnd, 0,0,0);
		if (hwnd)
			{
			SendMessage(hwnd,
						WM_VU_SETMAXRANGE,
						0, pstD->lTotalSize);
			SendMessage(hwnd,
						WM_VU_SETCURVALUE,
						0, 0L);
			pstD->bTotalSize = 0;
			}
		}

	if (pstD->bElapsedTime)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_ELAPSED_BOX);
		if (hwnd)
			{
			wsprintf(acBuffer,
					 " %02d:%02d:%02d",
					 (USHORT)(pstD->lElapsedTime / 3600),
					 (USHORT)((pstD->lElapsedTime / 60) % 60),
					 (USHORT)(pstD->lElapsedTime % 60)
					 );
			SetWindowText(hwnd, acBuffer);
			pstD->bElapsedTime = 0;
			}
		}

	if (pstD->bRemainingTime)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_REMAINING_BOX);
		if (hwnd)
			{
			if (pstD->lRemainingTime < 0)
				pstD->lRemainingTime = 0;

			wsprintf(acBuffer,
					 " %02d:%02d:%02d",
					 (USHORT)(pstD->lRemainingTime / 3600),
					 (USHORT)((pstD->lRemainingTime / 60) % 60),
					 (USHORT)(pstD->lRemainingTime % 60)
					 );
			SetWindowText(hwnd, acBuffer);
			pstD->bRemainingTime = 0;
			}
		}

	if (pstD->bThroughput)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_THRUPUT_BOX);
		if (hwnd)
			{
			int nValue;
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
					xfrGetXferDspBps(sessQueryXferHdl(pstL->hSession)) ?
						IDS_XD_BPS : IDS_XD_CPS,
					acMsg,
					sizeof(acMsg) / sizeof(TCHAR));

			nValue = pstD->lThroughput;
			if (xfrGetXferDspBps(sessQueryXferHdl(pstL->hSession)))
				nValue *= 10;

			wsprintf(acBuffer, acMsg, nValue);

			SetWindowText(hwnd, acBuffer);
			pstD->bThroughput = 0;
			}
		}

	if (pstD->bLastErrtype)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_LAST_ERROR_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						xfrGetStatusBase(sessQueryXferHdl(pstL->hSession))
							+ pstD->wLastErrtype,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));
			SetWindowText(hwnd, acMsg);
			pstD->bLastErrtype = 0;
			}
		}

	if (pstD->bPcktErrCnt)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_PACKET_RETRY_BOX);
		if (hwnd)
			{
			TCHAR acMsg[64];

			LoadString(glblQueryDllHinst(),
						IDS_XD_INT,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			wsprintf(acBuffer,
					 acMsg,
					 pstD->wPcktErrCnt);

			SetWindowText(hwnd, acBuffer);
			pstD->bPcktErrCnt = 0;
			}
		}

	if (pstD->bChecktype)
		{
		int nTag;
		TCHAR acMsg[64];

		hwnd = GetDlgItem(pstL->hDlg, XFR_ERROR_CHECKING_BOX);
		if (hwnd)
			{
			switch (pstD->wChecktype)
				{
				default:
				case 0:
					nTag = IDS_XD_CRC;
					break;
				case 1:
					nTag = IDS_XD_CHECK;
					break;
				case 2:
					nTag = IDS_XD_STREAM;
					break;
				}
			LoadString(glblQueryDllHinst(),
						nTag,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			SetWindowText(hwnd, acMsg);
			pstD->bChecktype = 0;
			}
		}

	if (pstD->bProtocol)
		{
		int nTag;
		TCHAR acMsg[64];

		hwnd = GetDlgItem(pstL->hDlg, XFR_PROTOCOL_BOX);
		if (hwnd)
			{
			switch (pstD->uProtocol)
				{
				default:
				case 1:
					nTag = IDS_XD_CB;
					break;
				case 2:
					nTag = IDS_XD_BP;
					break;
				}
			LoadString(glblQueryDllHinst(),
						nTag,
						acMsg,
						sizeof(acMsg) / sizeof(TCHAR));

			SetWindowText(hwnd, acMsg);
			pstD->bProtocol = 0;
			}
		}

	if (pstD->bMessage)
		{
		hwnd = GetDlgItem(pstL->hDlg, XFR_MESSAGE_BOX);
		if (hwnd)
			{
			SetWindowText(hwnd, pstD->acMessage);
			pstD->bMessage = 0;
			}
		}

	if (pstD->nClose)
		{
		HWND hWnd;

		/* It's time to quit */

		xfrSetPercentDone(sessQueryXferHdl(pstL->hSession), 0);

		// We decided that the new way might be a bit safer
		// EndModelessDialog(pstD->hwndXfrDisplay);

		hWnd = pstD->hwndXfrDisplay;
		pstD->hwndXfrDisplay = (HWND)0;

		PostMessage(sessQueryHwnd(pstL->hSession),
					WM_SESS_ENDDLG,
					0, (LPARAM)hWnd);
		}

	}
