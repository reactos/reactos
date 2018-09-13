/*	File: C:\WACKER\TDLL\recv_dlg.c (Created: 27-Dec-1993)
 *
 *	Copyright 1990,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 5/25/99 8:55a $
 */

#include <windows.h>
#pragma hdrstop

#include <time.h>
#include "stdtyp.h"
#include "mc.h"
#include "hlptable.h"

#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\assert.h>
#include <term\res.h>
#include <tdll\session.h>
#include <tdll\globals.h>
#include <tdll\file_msc.h>
#include <tdll\load_res.h>
#include <tdll\open_msc.h>
#include <tdll\errorbox.h>
#include <tdll\cnct.h>
#include <tdll\tchar.h>

#include <tdll\xfer_msc.h>
#include <xfer\xfer.h>

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	HSESSION hSession;
	};

typedef	struct stSaveDlgStuff SDS;

#define IDC_TF_FILENAME 100
#define	IDC_EB_DIR 		101
#define	IDC_PB_BROWSE	102
#define IDC_TF_PROTOCOL 103
#define IDC_CB_PROTOCOL 104
#define IDC_PB_CLOSE	105
#define IDC_PB_RECEIVE  106

INT_PTR CALLBACK TransferReceiveFilenameDlg(HWND hDlg,
	         								UINT wMsg,
			            					WPARAM wPar,
						    				LPARAM lPar);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	TransferReceiveDlg
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK TransferReceiveDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	int 	nProto;
	int 	nIndex;
	int 	nState;
	int 	nProtocol;
	TCHAR	acBuffer[FNAME_LEN];
	TCHAR	acName[FNAME_LEN];
	LPCTSTR pszDir;
	LPTSTR	pszPtr;
	LPTSTR	pszStr;
	XFR_PARAMS *pP;
	XFR_PROTOCOL *pX;
	HSESSION hSession;

	static	DWORD aHlpTable[] = {IDC_EB_DIR,		IDH_TERM_RECEIVE_DIRECTORY,
								 IDC_TF_FILENAME,	IDH_TERM_RECEIVE_DIRECTORY,
								 IDC_PB_BROWSE, 	IDH_BROWSE,
								 IDC_TF_PROTOCOL,	IDH_TERM_RECEIVE_PROTOCOL,
								 IDC_CB_PROTOCOL,	IDH_TERM_RECEIVE_PROTOCOL,
                                 IDC_PB_RECEIVE,    IDH_TERM_RECEIVE_RECEIVE,
								 IDC_PB_CLOSE,		IDH_CLOSE_DIALOG,
                                 IDCANCEL,          IDH_CANCEL,
                                 IDOK,              IDH_OK,
								 0, 				0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		if (pS == 0)
			{
			EndDialog(hDlg, FALSE);
			break;
			}

		SendMessage(GetDlgItem(hDlg, IDC_EB_DIR),
					EM_SETLIMITTEXT,
					FNAME_LEN, 0);

		hSession = (HSESSION)lPar;
		pS->hSession = hSession;

		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		pP = (XFR_PARAMS *)0;
		xfrQueryParameters(sessQueryXferHdl(hSession), (VOID **)&pP);
		assert(pP);

		nState = pP->nRecProtocol;

		/*
		 * Load selections into the PROTOCOL COMBO box
		 */
		nProto = 0;

		pX = (XFR_PROTOCOL *)0;
		xfrGetProtocols(hSession, &pX);
		assert(pX);

		if (pX != (XFR_PROTOCOL *)0)
			{
			for (nIndex = 0; pX[nIndex].nProtocol != 0; nIndex += 1)
				{
				if (nState == pX[nIndex].nProtocol)
					nProto = nIndex;

                //jmh 02-13-96 Use CB_ADDSTRING to sort entries as
                // they are added. CB_INSERTSTRING doesn't do this,
                // even if the combo-box has the CBS_SORT style.
				SendMessage(GetDlgItem(hDlg, IDC_CB_PROTOCOL),
							CB_ADDSTRING,
							0,  //(UINT) -1
							(LPARAM)&pX[nIndex].acName[0]);
				}

            SendMessage(GetDlgItem(hDlg, IDC_CB_PROTOCOL),
                        CB_SELECTSTRING,
                        0,
                        (LPARAM) &pX[nProto].acName[0]);

			free(pX);
			pX = NULL;
			}

		PostMessage(hDlg, WM_COMMAND,
					IDC_CB_PROTOCOL,
					MAKELONG(GetDlgItem(hDlg, IDC_CB_PROTOCOL),CBN_SELCHANGE));
		/*
		 * Set the current directory
		 */
		pszDir = filesQueryRecvDirectory(sessQueryFilesDirsHdl(hSession));
		SetDlgItemText(hDlg, IDC_EB_DIR, pszDir);

		// Check if we're connected.  If not, disable Send button.
		//
		if (cnctQueryStatus(sessQueryCnctHdl(hSession)) != CNCT_STATUS_TRUE)
			EnableWindow(GetDlgItem(hDlg, IDC_PB_RECEIVE), FALSE);

		/*
		 * Set the focus on this control
		 */
		SetFocus(GetDlgItem(hDlg, IDC_EB_DIR));
		return 0;

	case WM_DESTROY:
		pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

		if (pS)
			{
			free(pS);
			pS = NULL;
			}

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
		case IDC_PB_RECEIVE:
		case IDC_PB_CLOSE:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			hSession = pS->hSession;

			/*
			 * Do whatever saving is necessary
			 */
			pP = (XFR_PARAMS *)0;
			xfrQueryParameters(sessQueryXferHdl(hSession), (VOID **)&pP);
			assert(pP);

			/*
			 * Save selection from the PROTOCOL COMBO box
			 */
			pX = (XFR_PROTOCOL *)0;
			xfrGetProtocols(hSession, &pX);
			assert(pX);

			nProtocol = 0;

			if (pX != (XFR_PROTOCOL *)0)
				{
				GetDlgItemText(hDlg,
								IDC_CB_PROTOCOL,
								acBuffer,
								(sizeof(acBuffer) / sizeof(TCHAR)));
				for (nIndex = 0; pX[nIndex].nProtocol != 0; nIndex += 1)
					{
					if (StrCharCmp(acBuffer, pX[nIndex].acName) == 0)
						{
						nProtocol = pX[nIndex].nProtocol;
						break;
						}
					}
				free(pX);
				pX = NULL;
				}

			/*
			 * Save the current directory
			 */
			GetDlgItemText(hDlg, IDC_EB_DIR,
							acBuffer,
							sizeof(acBuffer) / sizeof(TCHAR));

			// xfer_makepaths checks for the validity of the path and
			// prompts to create it if not there.
			//
			if (xfer_makepaths(hSession, acBuffer) != 0)
				break;

			if (nId == IDC_PB_RECEIVE)
				{
				acName[0] = TEXT('\0');

				switch (nProtocol)
					{
					case XF_XMODEM:
					case XF_XMODEM_1K:
						pszPtr = (LPTSTR)DoDialog(glblQueryDllHinst(),
										MAKEINTRESOURCE(IDD_RECEIVEFILENAME),
										hDlg,
										TransferReceiveFilenameDlg,
										(LPARAM)hSession);

						if (pszPtr == NULL)
							return FALSE;

						StrCharCopy(acName, pszPtr);
						free(pszPtr);
						pszPtr = NULL;
						break;

					default:
						break;
					}
				}

			/*
			 * Save anything that needs to be saved
			 */
			pP->nRecProtocol = nProtocol;
			xfrSetParameters(sessQueryXferHdl(hSession), (VOID *)pP);
			filesSetRecvDirectory(sessQueryFilesDirsHdl(hSession), acBuffer);

			if (nId == IDC_PB_RECEIVE)
				{
				/*
				 * The directory to use should be in "acBuffer", and the
				 * file name (if any) should be in acName.
				 */
				xfrRecvStart(sessQueryXferHdl(pS->hSession), acBuffer,
						acName);
				}

			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		case IDC_PB_BROWSE:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				GetDlgItemText(hDlg,
								IDC_EB_DIR,
								acBuffer,
								sizeof(acBuffer) / sizeof(TCHAR));

				pszStr = gnrcFindDirectoryDialog(hDlg, pS->hSession, acBuffer);
				if (pszStr)
					{
					SetDlgItemText(hDlg, IDC_EB_DIR, pszStr);
					free(pszStr);
					pszStr = NULL;
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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	TransferReceiveFilenameDlg
 *
 * DESCRIPTION:
 *	This function is called to prompt the user to enter a filename.  This is
 *	necessary for some protocols like XMODEM, which does not send a filename,
 *	and for other protocols when the user has not checked the option to use
 *	the received file name.
 *
 * PARAMETERS:
 *	Standard dialog box parameters.
 *
 * RETURNS:
 *	Indirectly, a pointer to a string (that must be freed by the caller) that
 *	contains the name, or a NULL if the user canceled.
 *
 */

#define FOLDER		101
#define	DIR_TEXT	102
#define FNAME_LABEL 103
#define	FNAME_EDIT	104

#define	FILL_TEXT	105

INT_PTR CALLBACK TransferReceiveFilenameDlg(HWND hDlg,
	         								UINT wMsg,
			    							WPARAM wPar,
				    						LPARAM lPar)
	{
	HWND	hwndChild;
	INT		nId;
	INT		nNtfy;
	SDS    *pS;
	HWND	hwndParent;
	TCHAR	acDir[FNAME_LEN];
	TCHAR	acBuffer[FNAME_LEN];
	LPTSTR	pszStr;


	static	DWORD aHlpTable[] = {FOLDER,			IDH_TERM_RECEIVE_DIRECTORY,
								 FNAME_LABEL,		IDH_TERM_RECEIVE_PROTOCOL,
								 FNAME_EDIT,        IDH_TERM_RECEIVE_PROTOCOL,
								 IDCANCEL,          IDH_CANCEL,
                                 IDOK,              IDH_OK,
								 0, 				0};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		if (pS == (SDS *)0)
			{
			EndDialog(hDlg, FALSE);
			break;
			}

		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		SendMessage(GetDlgItem(hDlg, FNAME_EDIT),
					EM_SETLIMITTEXT,
					FNAME_LEN, 0);

		hwndParent = GetParent(hDlg);
		if (IsWindow(hwndParent))
			{
			acDir[0] = TEXT('\0');
			GetDlgItemText(hwndParent,
							IDC_EB_DIR,
							acDir,
							sizeof(acDir) / sizeof(TCHAR));
			if (StrCharGetStrLength(acDir) > 0)
				{
				SetDlgItemText(hDlg,
								DIR_TEXT,
								acDir);
				}
			}
		break;

	case WM_DESTROY:
		pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

		if (pS)
			{
			free(pS);
			pS = NULL;
			}

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
			pszStr = NULL;
			acBuffer[0] = TEXT('\0');
			GetDlgItemText(hDlg, FNAME_EDIT,
							acBuffer,
							sizeof(acBuffer) / sizeof(TCHAR));

			if (StrCharGetStrLength(acBuffer) > 0)
				{
				pszStr = (LPTSTR)malloc(StrCharGetByteCount(acBuffer) + 1);

				if (pszStr)
					StrCharCopy(pszStr, acBuffer);
				}

			EndDialog(hDlg, (INT_PTR)pszStr);
			break;

		case IDCANCEL:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
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
