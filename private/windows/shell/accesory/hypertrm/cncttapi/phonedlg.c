/*      File: D:\WACKER\cncttapi\phonedlg.c (Created: 23-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 5 $
 *	$Date: 3/26/99 8:05a $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#include <tapi.h>
#pragma hdrstop

#include <prsht.h>
#include <shlobj.h>
#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\errorbox.h>
#include <tdll\cnct.h>
#include <tdll\hlptable.h>
#include <tdll\globals.h>
#include <tdll\property.h>
#include <tdll\tchar.h>
#include <term\res.h>
#include <tdll\open_msc.h>

#include "cncttapi.hh"
#include "cncttapi.h"

STATIC_FUNC int tapi_SAVE_NEWPHONENUM(HWND hwnd);
STATIC_FUNC LRESULT tapi_WM_NOTIFY(const HWND hwnd, const int nId);
STATIC_FUNC void EnableCCAC(const HWND hwnd);
STATIC_FUNC void ModemCheck(const HWND hwnd);
static int ValidatePhoneDlg(const HWND hwnd);
static int CheckWindow(const HWND hwnd, const int id);
static int VerifyAddress(const HWND hwnd);
STATIC_FUNC int wsck_SAVE_NEWIPADDR(HWND hwnd);

// Local structure...
// Put in whatever else you might need to access later
//
typedef struct SDS
	{

	HSESSION 	hSession;
	HDRIVER		hDriver;

	// Store these so that we can restore the values if the user cancels
	// the property sheet.
	//
	TCHAR		acSessNameCopy[256];
	int			nIconID;
	HICON		hIcon;
	//HICON 	  hLittleIcon;

	} SDS, *pSDS;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	NewPhoneDlg
 *
 * DESCRIPTION:
 *	Displays dialog for getting new connection phone number and info.
 *
 *  NOTE:  Since this dialog proc is also called by the property sheet's
 *	phone number tab dialog it has to assume that the lPar contains the
 *	LPPROPSHEETPAGE.
 *
 * ARGUMENTS:
 *	Standard dialog
 *
 * RETURNS:
 *	Standard dialog
 *
 */
INT_PTR CALLBACK NewPhoneDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	/*
	 * NOTE: these defines must match the templates in both places, here and
	 *       int term\dialogs.rc
	 */
	#define IDC_TF_CNTRYCODES	113
	#define IDC_TF_AREACODES    106
	#define IDC_TF_MODEMS       110
	#define IDC_TF_PHONENUM     108
	
	#define IDC_IC_ICON			101
	#define IDC_CB_CNTRYCODES   114
	#define IDC_EB_AREACODE		107
	#define IDC_EB_PHONENUM 	109
	#define IDC_CB_MODEMS		111
	
	#define IDC_TB_NAME 		103
	#define IDC_PB_EDITICON 	117
	#define IDC_PB_CONFIGURE	115
	#define IDC_XB_USECCAC		116
    #define IDC_XB_REDIAL       119

    #define IDC_EB_HOSTADDR     214
    #define IDC_TF_PHONEDETAILS 105
    #define IDC_TF_TCPIPDETAILS 205
    #define IDC_TF_HOSTADDR     213
    #define IDC_TF_PORTNUM      206
    #define IDC_EB_PORTNUM      207
    #define IDC_TF_ACPROMPT     118

	HWND	 hwndParent;
	HHDRIVER hhDriver;
	TCHAR	 ach[256];
	TCHAR 	 acNameCopy[256];
	int 	 i;
	PSTLINEIDS pstLineIds;

	static 	 DWORD aHlpTable[] = {IDC_CB_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_TF_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_EB_AREACODE,	 IDH_TERM_NEWPHONE_AREA,
								  IDC_TF_AREACODES,  IDH_TERM_NEWPHONE_AREA,
								  IDC_EB_PHONENUM,	 IDH_TERM_NEWPHONE_NUMBER,
								  IDC_TF_PHONENUM,   IDH_TERM_NEWPHONE_NUMBER,
								  IDC_PB_CONFIGURE,  IDH_TERM_NEWPHONE_CONFIGURE,
								  IDC_TF_MODEMS,     IDH_TERM_NEWPHONE_DEVICE,
								  IDC_CB_MODEMS,	 IDH_TERM_NEWPHONE_DEVICE,
								  IDC_PB_EDITICON,	 IDH_TERM_PHONEPROP_CHANGEICON,
                                  IDC_XB_USECCAC,    IDH_TERM_NEWPHONE_USECCAC,
                                  IDC_XB_REDIAL,     IDH_TERM_NEWPHONE_REDIAL,
								  IDC_EB_HOSTADDR,   IDH_TERM_NEWPHONE_HOSTADDRESS,
								  IDC_TF_HOSTADDR,   IDH_TERM_NEWPHONE_HOSTADDRESS,
								  IDC_EB_PORTNUM,    IDH_TERM_NEWPHONE_PORTNUMBER,
								  IDC_TF_PORTNUM,    IDH_TERM_NEWPHONE_PORTNUMBER,
                                  IDCANCEL,                           IDH_CANCEL,
                                  IDOK,                               IDH_OK,
								  0,0,
								  };
	pSDS	 pS = NULL;

	switch (uMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));

		if (pS == (SDS *)0)
			{
			assert(FALSE);
			EndDialog(hwnd, FALSE);
			break;
			}

		// In the effort to keep the internal driver handle internal
		// we are passing the session handle from the property sheet tab
		// dialog.
		//
		pS->hSession = (HSESSION)(((LPPROPSHEETPAGE)lPar)->lParam);

		pS->hDriver = cnctQueryDriverHdl(sessQueryCnctHdl(pS->hSession));
		hhDriver = (HHDRIVER)(pS->hDriver);

		SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pS);

		// In order to center the property sheet we need to center the parent
		// of the hwnd on top of the session window.
		// If the parent of hwnd is the session window then this dialog has
		// not been called from the property sheet.
		//
		hwndParent = GetParent(hwnd);

		if (hwndParent != sessQueryHwnd(pS->hSession))
			mscCenterWindowOnWindow(hwndParent, sessQueryHwnd(pS->hSession));

		else
			mscCenterWindowOnWindow(hwnd, sessQueryHwnd(pS->hSession));

		// Display the session icon...
		//
		pS->nIconID = sessQueryIconID(hhDriver->hSession);
		pS->hIcon = sessQueryIcon(hhDriver->hSession);
		//pS->hLittleIcon = sessQueryLittleIcon(hhDriver->hSession);

		SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
			(WPARAM)pS->hIcon, 0);

		/* --- Need to initialize TAPI if not already done --- */

		if (hhDriver->hLineApp == 0)
			{
            extern const TCHAR *g_achApp;

			if (lineInitialize(&hhDriver->hLineApp, glblQueryDllHinst(),
					lineCallbackFunc, g_achApp, &hhDriver->dwLineCnt))
				{
				assert(FALSE);
				}
			}

		SendDlgItemMessage(hwnd, IDC_EB_PHONENUM, EM_SETLIMITTEXT,
			sizeof(hhDriver->achDest)-1, 0);

		SendDlgItemMessage(hwnd, IDC_EB_AREACODE, EM_SETLIMITTEXT,
			sizeof(hhDriver->achAreaCode)-1, 0);

		if (hhDriver->achDest[0])
			SetDlgItemText(hwnd, IDC_EB_PHONENUM, hhDriver->achDest);

#if defined(INCL_WINSOCK)
		SendDlgItemMessage(hwnd, IDC_EB_HOSTADDR, EM_SETLIMITTEXT,
			sizeof(hhDriver->achDestAddr)-1, 0);

		if (hhDriver->achDestAddr[0])
			SetDlgItemText(hwnd, IDC_EB_HOSTADDR, hhDriver->achDestAddr);

		wsprintf(ach, "%d", hhDriver->iPort);
		SetDlgItemText(hwnd, IDC_EB_PORTNUM, ach);
#endif

		TCHAR_Fill(pS->acSessNameCopy, TEXT('\0'),
			sizeof(pS->acSessNameCopy) / sizeof(TCHAR));

		sessQueryName(hhDriver->hSession, pS->acSessNameCopy,
			sizeof(pS->acSessNameCopy));

		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		lstrcpy(ach, pS->acSessNameCopy);
		mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach);
		SetDlgItemText(hwnd, IDC_TB_NAME, ach);

		EnumerateCountryCodes(hhDriver, GetDlgItem(hwnd, IDC_CB_CNTRYCODES));
		EnumerateAreaCodes(hhDriver, GetDlgItem(hwnd, IDC_EB_AREACODE));
		if ( IsNT() )
			{
			EnumerateLinesNT(hhDriver, GetDlgItem(hwnd, IDC_CB_MODEMS));
			}
		else
			{	
			EnumerateLines(hhDriver, GetDlgItem(hwnd, IDC_CB_MODEMS));
			}

		//mpt 6-23-98 disable the port list drop-down if we are connected
		EnableWindow(GetDlgItem(hwnd, IDC_CB_MODEMS),
			cnctdrvQueryStatus(hhDriver) == CNCT_STATUS_FALSE);

		if (hhDriver->fUseCCAC) 	// Use country code and area code?
			{
			CheckDlgButton(hwnd, IDC_XB_USECCAC, TRUE);
			SetFocus(GetDlgItem(hwnd, IDC_EB_PHONENUM));
			}

        #if defined(INCL_REDIAL_ON_BUSY)
        if (hhDriver->fRedialOnBusy)
            {
			CheckDlgButton(hwnd, IDC_XB_REDIAL, TRUE);
			SetFocus(GetDlgItem(hwnd, IDC_EB_PHONENUM));
            }
        #endif

		// Call after Use CCAC checkbox checked or unchecked.
		//
		EnableCCAC(hwnd);
		ModemCheck(hwnd);

		/* --- Pick which control to give focus too --- */

		if (hhDriver->fUseCCAC)		// Use country code and area code?
			{
			
			if (SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL,
					0, 0) == CB_ERR)
				{
				SetFocus(GetDlgItem(hwnd, IDC_CB_CNTRYCODES));
				}

			else if (GetDlgItemText(hwnd, IDC_EB_AREACODE, ach,
					sizeof(ach)) == 0)
				{
				SetFocus(GetDlgItem(hwnd, IDC_EB_AREACODE));
				}
			}

		// If we have an old session and we have not matched our stored
		// permanent line id, then pop-up a message saying the TAPI
		// configuration has changed.
		//
		if (!hhDriver->fMatchedPermanentLineID &&
				hhDriver->dwPermanentLineId != (DWORD)-1)
			{
			LoadString(glblQueryDllHinst(), IDS_ER_TAPI_CONFIG,
				ach, sizeof(ach));

			TimedMessageBox(hwnd, ach, 0, MB_OK | MB_ICONINFORMATION, 0);
			}

		return 0;

	case WM_CONTEXTMENU:
        doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_DESTROY:
		// OK, now we know that we are actually leaving the dialog for good, so
		// free the storage...
		//
		pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
		if (pS)
			{
			free(pS);
			pS = NULL;
			}

		ResetComboBox(GetDlgItem(hwnd, IDC_CB_MODEMS));
		break;

	case WM_NOTIFY:
		//
		// Property sheet messages are being channeled through here...
		//
		return tapi_WM_NOTIFY(hwnd, (int)((NMHDR *)lPar)->code);

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{
		case IDC_CB_CNTRYCODES:
			if (HIWORD(wPar) == CBN_SELENDOK)
				EnableCCAC(hwnd);
			break;

		case IDC_CB_MODEMS:
			if (HIWORD(wPar) == CBN_SELENDOK)
				ModemCheck(hwnd);

			break;

		//
		// Property sheet's TAB_PHONENUMBER dialog is using this dialog proc
		// also, the following two buttons appear only in this tabbed dialog
		// template.
		//
		case IDC_PB_EDITICON:
			{
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);

			sessQueryName(pS->hSession, acNameCopy, sizeof(acNameCopy));

			if (DialogBoxParam(glblQueryDllHinst(),
				MAKEINTRESOURCE(IDD_NEWCONNECTION),
					hwnd, NewConnectionDlg,
						(LPARAM)pS->hSession) == FALSE)
				{
				return 0;
				}

			SetFocus(GetDlgItem(hwnd, IDC_PB_EDITICON));
			ach[0] = TEXT('\0');
			sessQueryName(pS->hSession, ach, sizeof(ach));
			mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach);
			SetDlgItemText(hwnd, IDC_TB_NAME, ach);

			SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
				(WPARAM)sessQueryIcon(pS->hSession), 0);

			// The user may have changed the name of the session.
			// The new name should be reflected in the property sheet title
			// and in the app title.
			//
			propUpdateTitle(pS->hSession, hwnd, acNameCopy);
			}
			break;

		case IDC_PB_CONFIGURE:
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
			hhDriver = (HHDRIVER)(pS->hDriver);

			if ((i = (int)SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL,
					0, 0)) != CB_ERR)
				{
				if (((LRESULT)pstLineIds = SendDlgItemMessage(hwnd,
						IDC_CB_MODEMS,  CB_GETITEMDATA, (WPARAM)i, 0))
							!= CB_ERR)
					{
					if ((LRESULT)pstLineIds != CB_ERR)
						{
						// I've "reserved" 4 permanent line ids to indentify
						// the direct to com port lines.
						//
						if (IN_RANGE(pstLineIds->dwPermanentLineId,
								DIRECT_COM1, DIRECT_COM4))
							{
							wsprintf(ach, "COM%d",
								pstLineIds->dwPermanentLineId - DIRECT_COM1 + 1);

							ComSetPortName(sessQueryComHdl(pS->hSession), ach);

                            // mrw: 2/20/96 - Set AutoDetect off if user clicks
                            // OK in this dialog.
                            //
							if (ComDeviceDialog(sessQueryComHdl(pS->hSession), hwnd) == 0)
                                ComSetAutoDetect(sessQueryComHdl(pS->hSession), FALSE);

							}

                        else if ( IsNT() && pstLineIds->dwPermanentLineId == DIRECT_COM_DEVICE)
							{
							// Get device from combobox... mrw:6/5/96
							//
							SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
								CB_GETLBTEXT, (WPARAM)i,(LPARAM)ach);

							ComSetPortName(sessQueryComHdl(pS->hSession), ach);

							// mrw: 2/20/96 - Set AutoDetect off if user clicks
							// OK in this dialog.
							//
							if (ComDeviceDialog(sessQueryComHdl(pS->hSession), hwnd) == 0)
								{
								ComSetAutoDetect(sessQueryComHdl(pS->hSession), FALSE);
								ComConfigurePort(sessQueryComHdl(pS->hSession));
								}
							}
						else
							{
							lineConfigDialog(pstLineIds->dwLineId,
								hwnd,  "TAPI/LINE");
							}
						}
					}
				}

			else
				{
				MessageBeep(MB_ICONHAND);
				}

			break;

		case IDC_XB_USECCAC:
			EnableCCAC(hwnd);
			break;

		case IDOK:
			if (ValidatePhoneDlg(hwnd) == 0)
				{
				if (tapi_SAVE_NEWPHONENUM(hwnd) != 0)
					break;

				EndDialog(hwnd, TRUE);
				}

			break;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
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
 *  tapi_WM_NOTIFY
 *
 * DESCRIPTION:
 *  Process Property Sheet Notification messages.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC LRESULT tapi_WM_NOTIFY(const HWND hDlg, const int nId)
	{
	pSDS	pS;

	switch (nId)
		{
		default:
			break;

		case PSN_APPLY:
			pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				if (ValidatePhoneDlg(hDlg) != 0)
					{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					return TRUE;
					}

				//
				// Do whatever saving is necessary
				//
				tapi_SAVE_NEWPHONENUM(hDlg);
				}
			break;

		case PSN_RESET:
			pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				//
				// If the user cancels make sure the old session name and its
				// icon are restored.
				//
				sessSetName(pS->hSession, pS->acSessNameCopy);
				sessSetIconID(pS->hSession, pS->nIconID);
				sessUpdateAppTitle(pS->hSession);

				SendMessage(sessQueryHwnd(pS->hSession), WM_SETICON,
					(WPARAM)TRUE, (LPARAM)pS->hIcon);
				}
			break;
#if 0
		case PSN_HASHELP:
			// For now gray the help button...
			//
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
			break;
#endif
		case PSN_HELP:
			// Display help in whatever way is appropriate
			break;
		}

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  tapi_SAVE_NEWPHONENUM
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC int tapi_SAVE_NEWPHONENUM(HWND hwnd)
	{
	pSDS		pS;
	HHDRIVER	hhDriver;
	LRESULT		lr, lrx;
	PSTLINEIDS	pstLineIds;
#if defined(INCL_WINSOCK)
	TCHAR		achPort[6];
#endif

	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	/* --- Get selected modem --- */

	lrx = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL, 0, 0);

	if (lrx != CB_ERR)
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETLBTEXT, (WPARAM)lrx,
			(LPARAM)hhDriver->achLineName);

		if (lr != CB_ERR)
			{
			pstLineIds = (PSTLINEIDS)SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
				CB_GETITEMDATA, (WPARAM)lrx, 0);

			if ((LRESULT)pstLineIds != CB_ERR)
				{
				hhDriver->dwPermanentLineId = pstLineIds->dwPermanentLineId;
				hhDriver->dwLine = pstLineIds->dwLineId;

				if ( IsNT() )
					{
					if (hhDriver->dwPermanentLineId == DIRECT_COM_DEVICE)
						{
						SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
							CB_GETLBTEXT, (WPARAM)lrx,
							(LPARAM)hhDriver->achComDeviceName);
						}
					}
				}

			else
				{
				assert(FALSE);
				}
			}

		else
			{
			assert(FALSE);
			}
		}
	else
		{
		MessageBeep(MB_ICONHAND);
		assert(FALSE);
		return 1;
		}

	/* --- Get Country Code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CB_CNTRYCODES)))
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

		if (lr != CB_ERR)
			{
			lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
				(WPARAM)lr, 0);

			if (lr != CB_ERR)
				hhDriver->dwCountryID = (DWORD)lr;

			else
				assert(FALSE);
			}
		}

	/* --- Get area code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_AREACODE)))
		{
		GetDlgItemText(hwnd, IDC_EB_AREACODE, hhDriver->achAreaCode,
			sizeof(hhDriver->achAreaCode));
		}

	/* --- Get phone number --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_PHONENUM)))
		{
		GetDlgItemText(hwnd, IDC_EB_PHONENUM, hhDriver->achDest,
			sizeof(hhDriver->achDest));
		}

#if defined(INCL_WINSOCK)
    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_HOSTADDR)))
        {
        GetDlgItemText(hwnd, IDC_EB_HOSTADDR, hhDriver->achDestAddr,
			sizeof(hhDriver->achDestAddr));
        }

    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_PORTNUM)))
        {
		GetDlgItemText(hwnd, IDC_EB_PORTNUM, achPort,
			sizeof(achPort));
		hhDriver->iPort = atoi(achPort);
        }
#endif  // defined(INCL_WINSOCK)

	/* --- Get Use country code, area code info --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		hhDriver->fUseCCAC = (BOOL)IsDlgButtonChecked(hwnd, IDC_XB_USECCAC);

    #if defined(INCL_REDIAL_ON_BUSY)
	/* --- Get Redial on Busy setting --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_REDIAL)))
        {
		hhDriver->fRedialOnBusy =
		    (BOOL)IsDlgButtonChecked(hwnd, IDC_XB_REDIAL);
        }
    #endif

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnableCCAC
 *
 * DESCRIPTION:
 *	Enables/disables controls associated with the Use Country Code,
 *	Area Code control
 *
 * ARGUMENTS:
 *	hwnd	- dialog window
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void EnableCCAC(const HWND hwnd)
	{
	BOOL				fUseCCAC = TRUE;
	BOOL				fUseAC = TRUE;
	DWORD				dwCountryId;
	pSDS				pS;
	HHDRIVER			hhDriver;
	LRESULT 			lr;

	// Different templates use this same dialog proc.  If this window
	// is not there, don't do the work.  Also, selection of the direct
	// connect stuff can disable the control.

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		{
		fUseCCAC = (BOOL)IsDlgButtonChecked(hwnd, IDC_XB_USECCAC);
		EnableWindow(GetDlgItem(hwnd, IDC_CB_CNTRYCODES), fUseCCAC);
		}

	// We want to enable the area code only if both the use Country
	// code, Area code checkbox is checked and the country in
	// question uses area codes. - mrw, 2/12/95
	//
	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	// Country code from dialog
	//
	lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

	if (lr != CB_ERR)
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
			(WPARAM)lr, 0);

		if (lr != CB_ERR)
			dwCountryId = (DWORD)lr;

		//fUseAC = fCountryUsesAreaCode(dwCountryId, hhDriver->dwAPIVersion);
		fUseAC = TRUE; // Microsoft changed its mind on this one -mrw:4/20/95
		}

	EnableWindow(GetDlgItem(hwnd, IDC_EB_AREACODE), fUseCCAC && fUseAC);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ResetComboBox
 *
 * DESCRIPTION:
 *	The modem combobox allocates memory to store info about each item.
 *	This routine will free those allocated chunks.
 *
 * ARGUMENTS:
 *	hwnd	- window handle to combobox
 *
 * RETURNS:
 *	void
 *
 */
void ResetComboBox(const HWND hwnd)
	{
	void *pv = NULL;
	LRESULT lr, i;

	if (!IsWindow(hwnd))
		return;

	if ((lr = SendMessage(hwnd, CB_GETCOUNT, 0, 0)) != CB_ERR)
		{
		for (i = 0 ; i < lr ; ++i)
			{
			if (((LRESULT)pv = SendMessage(hwnd, CB_GETITEMDATA, (WPARAM)i, 0))
					!= CB_ERR)
				{
				if (pv)
					{
					free(pv);
					pv = NULL;
					}
				}
			}
		}

	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ModemCheck
 *
 * DESCRIPTION:
 *	Checks if the currently selected "modem" is one of the Direct to Com?
 *	selections.  If it is, it disables the country code, area code, phone
 *	number, and Use country code area code check box.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window handle
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void ModemCheck(const HWND hwnd)
	{
	int fModem;
	int fHotPhone;
    int fWinSock;
	LRESULT lr;
	PSTLINEIDS	pstLineIds;
	HHDRIVER hhDriver;
	const pSDS pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	const HWND hwndCB = GetDlgItem(hwnd, IDC_CB_MODEMS);
    HWND hwndTmp;

	if (!IsWindow(hwndCB))
		return;

	if ((lr = SendMessage(hwndCB, CB_GETCURSEL, 0, 0)) != CB_ERR)
		{
		pstLineIds = (PSTLINEIDS)SendMessage(hwndCB, CB_GETITEMDATA, lr, 0);

		if ((LRESULT)pstLineIds != CB_ERR)
			{
			fModem = TRUE;
            fWinSock = FALSE;

			if (IN_RANGE(pstLineIds->dwPermanentLineId, DIRECT_COM1,
					DIRECT_COM4))

				{
				fModem = FALSE;
				}

			if ( IsNT() )
				{
				if (pstLineIds->dwPermanentLineId == DIRECT_COM_DEVICE)
					{
					fModem = FALSE;
					}
				}

#if defined(INCL_WINSOCK)
			if (pstLineIds->dwPermanentLineId == DIRECT_COMWINSOCK)
			    {
			    fModem = FALSE;
			    fWinSock = TRUE;
			    }
#endif

			// Also check if we have a hotphone
			//
			if (fModem == TRUE && pS)
				{
				hhDriver = (HHDRIVER)(pS->hDriver);

				if (hhDriver)
					{
					if (CheckHotPhone(hhDriver, pstLineIds->dwLineId,
							&fHotPhone) == 0)
						{
						fModem = !fHotPhone;
						}
					}
				}

            // Swap between phone number and host address prompts
            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_PHONEDETAILS)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
                }
            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_TCPIPDETAILS)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
                }
            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_ACPROMPT)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
                }

            // Swap between Country code and Host address static text
            hwndTmp = GetDlgItem(hwnd, IDC_TF_CNTRYCODES);
            ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
            EnableWindow(hwndTmp, ! fWinSock);
            hwndTmp = GetDlgItem(hwnd, IDC_TF_HOSTADDR);
            ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
            EnableWindow(hwndTmp, fWinSock);

            // Swap between country code and host address edit boxes
            hwndTmp = GetDlgItem(hwnd, IDC_CB_CNTRYCODES);
            ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
			EnableWindow(hwndTmp, fModem);
            hwndTmp = GetDlgItem(hwnd, IDC_EB_HOSTADDR);
            ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
            EnableWindow(hwndTmp, fWinSock);

            // Swap between area code and port number static text
            hwndTmp = GetDlgItem(hwnd, IDC_TF_AREACODES);
            ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
            EnableWindow(hwndTmp, ! fWinSock);
            hwndTmp = GetDlgItem(hwnd, IDC_TF_PORTNUM);
            ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
            EnableWindow(hwndTmp, fWinSock);

            // Swap between area code and port number edit boxes
            hwndTmp = GetDlgItem(hwnd, IDC_EB_AREACODE);
            ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
			EnableWindow(hwndTmp, fModem);
            hwndTmp = GetDlgItem(hwnd, IDC_EB_PORTNUM);
            ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
            EnableWindow(hwndTmp, fWinSock);

            hwndTmp = GetDlgItem(hwnd, IDC_TF_PHONENUM);
            ShowWindow(hwndTmp, ! fWinSock);
            EnableWindow(hwndTmp, ! fWinSock);
            hwndTmp = GetDlgItem(hwnd, IDC_EB_PHONENUM);
            ShowWindow(hwndTmp, ! fWinSock);
            EnableWindow(hwndTmp, fModem);

            if ((hwndTmp = GetDlgItem(hwnd, IDC_XB_USECCAC)))
                {
                ShowWindow(hwndTmp, ! fWinSock);
                EnableWindow(hwndTmp, fModem);
                }

            if ((hwndTmp = GetDlgItem(hwnd, IDC_PB_CONFIGURE)))
                {
                ShowWindow(hwndTmp, !fWinSock);
                EnableWindow(hwndTmp, !fWinSock);
                if (pS)
				    EnableWindow(hwndTmp, cnctdrvQueryStatus((HHDRIVER)(pS->hDriver)) == CNCT_STATUS_FALSE);
                }

            // Set focus to modem combo when direct connect selected.
            // mrw:11/3/95
            //
            if (fModem == FALSE)
                {
                SetFocus(fWinSock ? GetDlgItem(hwnd,IDC_EB_HOSTADDR) : hwndCB);
                }

#if defined(INCL_REDIAL_ON_BUSY)
            if ((hwndTmp = GetDlgItem(hwnd, IDC_XB_REDIAL)))
                {
                ShowWindow(hwndTmp, ! fWinSock);
                EnableWindow(hwndTmp, fModem);
                }
#endif

			if (fModem == TRUE)
				EnableCCAC(hwnd);
			}
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ValidatePhoneDlg
 *
 * DESCRIPTION:
 *	Checks phone dialog entries for proper values.	This mostly means
 *	checking for blank entry fields.
 *
 * ARGUMENTS:
 *	hwnd		- phone dialog
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
static int ValidatePhoneDlg(const HWND hwnd)
	{
	if (CheckWindow(hwnd, IDC_CB_CNTRYCODES) != 0)
		return -1;

	//if (CheckWindow(hwnd, IDC_EB_AREACODE) != 0) - mrw:4/20/95
	//	  return -2;

	// Removed per MHG discussions - MPT 12/21/95
	//if (CheckWindow(hwnd, IDC_EB_PHONENUM) != 0)
	//	return -3;

	if (CheckWindow(hwnd, IDC_CB_MODEMS) != 0)
		return -4;

	if (VerifyAddress(hwnd) != 0)
		return -5;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CheckWindow
 *
 * DESCRIPTION:
 *	Since the dialog only enables windows that require entries, it just
 *	needs to check if an enabled window has text.  This function sets
 *	the focus to the offending field and beeps.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window
 *	id		- control id
 *
 * RETURNS:
 *	0=OK, else not ok.
 *
 */
static int CheckWindow(const HWND hwnd, const int id)
	{
	char ach[256];

	if (IsWindowEnabled(GetDlgItem(hwnd, id)))
		{
		if (GetDlgItemText(hwnd, id, ach, sizeof(ach)) == 0)
			{
			MessageBeep(MB_ICONHAND);
			SetFocus(GetDlgItem(hwnd, id));
			return -1;
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VerifyAddress
 *
 * DESCRIPTION:
 *	I can't believe how much code it takes to verify a stinking address.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window handle.
 *
 * RETURNS:
 *	0=OK
 *
 */
static int VerifyAddress(const HWND hwnd)
	{
	pSDS		pS;
	HHDRIVER	hhDriver;
	LRESULT		lr, lrx;
	PSTLINEIDS	pstLineIds;
	int   fHotPhone;
	int   fUseCCAC;
	long  lRet;
	DWORD dwSize;
	DWORD dwLine;
	DWORD dwCountryID;
	DWORD dwPermanentLineId;
	TCHAR achAreaCode[10];
	TCHAR achDest[(TAPIMAXDESTADDRESSSIZE/2)+1];
	TCHAR ach[256];
	LPLINECOUNTRYLIST pcl;
	LPLINECOUNTRYENTRY pce;
	LINETRANSLATEOUTPUT *pLnTransOutput;

	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	/* --- Get selected modem --- */

	lrx = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL, 0, 0);

	if (lrx != CB_ERR)
		{
		pstLineIds = (PSTLINEIDS)SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
			CB_GETITEMDATA, (WPARAM)lrx, 0);

		if ((LRESULT)pstLineIds != CB_ERR)
			{
			dwPermanentLineId = pstLineIds->dwPermanentLineId;
			dwLine = pstLineIds->dwLineId;
			}
		}

	else
		{
		return 0;
		}

	/* --- Get Country Code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CB_CNTRYCODES)))
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

		if (lr != CB_ERR)
			{
			lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
				(WPARAM)lr, 0);

			if (lr != CB_ERR)
				dwCountryID = (DWORD)lr;
			}
		}

	else
		{
		return 0;
		}

	/* --- Get area code --- */

	achAreaCode[0] = TEXT('\0');
	GetDlgItemText(hwnd, IDC_EB_AREACODE, achAreaCode, sizeof(achAreaCode));

	/* --- Get phone number --- */

	achDest[0] = TEXT('\0');
	GetDlgItemText(hwnd, IDC_EB_PHONENUM, achDest, sizeof(achDest));

	/* --- Get Use country code, area code info --- */

	fUseCCAC = TRUE;

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		fUseCCAC = (BOOL)IsDlgButtonChecked(hwnd, IDC_XB_USECCAC);

	/* --- Try to translate --- */

	if (CheckHotPhone(hhDriver, dwLine, &fHotPhone) != 0)
		{
		assert(0);
		return 0;  // error message displayed already.
		}

	// Hot Phone is TAPI terminology for Direct Connects
	// We don't need to do address translation since we
	// not going to use it.

	if (fHotPhone)
		return 0;

	ach[0] = TEXT('\0');

	// If we not using the country code or area code, we still need to
	// pass a dialable string format to TAPI so that we get the
	// pulse/tone dialing modifiers in the dialable string.
	//
	if (fUseCCAC)
		{
		/* --- Do lineGetCountry to get extension --- */

		if (DoLineGetCountry(dwCountryID, TAPI_VER, &pcl) != 0)
			{
			assert(FALSE);
			return 0;
			}

		if ((pce = (LPLINECOUNTRYENTRY)
				((BYTE *)pcl + pcl->dwCountryListOffset)) == 0)
			{
			assert(FALSE);
			return 0;
			}

		/* --- Put country code in now --- */

		wsprintf(ach, "+%u ", pce->dwCountryCode);
		free(pcl);
		pcl = NULL;

		if (!fIsStringEmpty(achAreaCode))
			{
			lstrcat(ach, "(");
			lstrcat(ach, achAreaCode);
			lstrcat(ach, ") ");
			}
		}

	lstrcat(ach, achDest);

	/* --- Allocate some space --- */

	pLnTransOutput = malloc(sizeof(LINETRANSLATEOUTPUT));

	if (pLnTransOutput == 0)
		{
		assert (FALSE);
		return 0;
		}

	pLnTransOutput->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	/* --- Now that we've satisifed the clergy, translate it --- */

	if (TRAP(lRet = lineTranslateAddress(hhDriver->hLineApp,
			dwLine, TAPI_VER, ach, 0,
				LINETRANSLATEOPTION_CANCELCALLWAITING,
					pLnTransOutput)) != 0)
		{
		free(pLnTransOutput);
		pLnTransOutput = NULL;

		if (lRet = LINEERR_INVALADDRESS)
			goto MSG_EXIT;

		return 0;
		}

	if (pLnTransOutput->dwTotalSize < pLnTransOutput->dwNeededSize)
		{
		dwSize = pLnTransOutput->dwNeededSize;
		free(pLnTransOutput);
		pLnTransOutput = NULL;

		if ((pLnTransOutput = malloc(dwSize)) == 0)
			{
			assert(FALSE);
			return 0;
			}

		pLnTransOutput->dwTotalSize = dwSize;

		if ((lRet = lineTranslateAddress(hhDriver->hLineApp,
				dwLine, TAPI_VER, ach, 0,
					LINETRANSLATEOPTION_CANCELCALLWAITING,
						pLnTransOutput)) != 0)
			{
			assert(FALSE);
			free(pLnTransOutput);
			pLnTransOutput = NULL;

			if (lRet == LINEERR_INVALADDRESS)
				goto MSG_EXIT;
			}
		}

	free(pLnTransOutput);
	pLnTransOutput = NULL;
	return 0;

	MSG_EXIT:
		// per MHG discussion - MPT 12/21/95
		return 0;		
		//LoadString(glblQueryDllHinst(), IDS_ER_CNCT_BADADDRESS, ach,
		//	sizeof(ach));

		//TimedMessageBox(hwnd, ach, 0, MB_OK | MB_ICONINFORMATION, 0);

		//return -2;
	}

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fCountryUsesAreaCod
 *
 * DESCRIPTION:
 *	Checks if the specified country uses area codes.
 *
 * ARGUMENTS:
 *	hwnd	- window handle of dialog.
 *
 * RETURNS:
 *	TRUE/FALSE, <0=error
 *
 * AUTHOR: Mike Ward, 26-Jan-1995
 */
int fCountryUsesAreaCode(const DWORD dwCountryID, const DWORD dwAPIVersion)
	{
	LPTSTR pachLongDistDialRule;
	LPLINECOUNTRYLIST pcl;
	LPLINECOUNTRYENTRY pce;

	// Get country information
	//
	if (DoLineGetCountry(dwCountryID, TAPI_VER, &pcl) != 0)
		{
		assert(0);
		return -1;
		}

	// Find offset to country info.
	//
	if ((pce = (LPLINECOUNTRYENTRY)
			((BYTE *)pcl + pcl->dwCountryListOffset)) == 0)
		{
		assert(0);
		return -1;
		}

	// Get long distance dialing rule
	//
	pachLongDistDialRule = (BYTE *)pcl + pce->dwLongDistanceRuleOffset;

	// If dial rule has an 'F', we need the area code.
	//
	if (strchr(pachLongDistDialRule, 'F'))
		return TRUE;

	return FALSE;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fIsStringEmpty
 *
 * DESCRIPTION:
 *	Used for checking if areacode is just blanks.  lineTranslateAddress
 *	pukes badly if you give it a string of blanks for the area code.
 *
 * ARGUMENTS:
 *	ach 	- areacode string (can be NULL)
 *
 * RETURNS:
 *	1=emtpy, 0=not empty
 *
 * AUTHOR: Mike Ward, 20-Apr-1995
 */
int fIsStringEmpty(LPTSTR ach)
	{
	int i;

	if (ach == 0)
		return 1;

	if (ach[0] == TEXT('\0'))
		return 1;

	for (i = lstrlen(ach) - 1 ; i >= 0 ; --i)
		{
		if (ach[i] != TEXT(' '))
			return 0;
		}

	return 1;
	}


#if 0   //JMH 02-05-96 Previously defined (INCL_WINSOCK)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctwsNewPhoneDlg
 *
 * DESCRIPTION:
 *	Displays dialog for getting new connection info.
 *
 *  NOTE:  Since this dialog proc is also called by the property sheet's
 *	phone number tab dialog it has to assume that the lPar contains the
 *	LPPROPSHEETPAGE.
 *
 * ARGUMENTS:
 *	Standard dialog
 *
 * RETURNS:
 *	Standard dialog
 *
 * AUTHOR
 *	mcc 01/04/96
 *
 */
BOOL CALLBACK cnctwsNewPhoneDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	/*
	 * NOTE: these defines must match the templates in both places, here and
	 *       int term\dialogs.rc
	 */
	//#define IDC_TF_PORTNUM      110
	//#define IDC_TF_IPADDR	    108
	
	#define IDC_IC_ICON			101
	#define IDC_EB_IPADDR	 	109
	#define IDC_EB_PORTNUM		111
	
	#define IDC_TB_NAME 		103
	#define IDC_PB_EDITICON 	117
	#define IDC_PB_CONFIGURE	115

	HWND	 hwndParent;
	HHDRIVER hhDriver;
	TCHAR	 ach[256];
	TCHAR	 achPort[6];
	TCHAR 	 acNameCopy[256];
	//int 	 i;

#if 0 // TODO:mcc 01/03/96
	static 	 DWORD aHlpTable[] = {IDC_CB_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_TF_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_EB_AREACODE,	 IDH_TERM_NEWPHONE_AREA,
								  IDC_TF_AREACODES,  IDH_TERM_NEWPHONE_AREA,
								  IDC_EB_PHONENUM,	 IDH_TERM_NEWPHONE_NUMBER,
								  IDC_TF_PHONENUM,   IDH_TERM_NEWPHONE_NUMBER,
								  IDC_PB_CONFIGURE,  IDH_TERM_NEWPHONE_CONFIGURE,
								  IDC_TF_MODEMS,     IDH_TERM_NEWPHONE_DEVICE,
								  IDC_CB_MODEMS,	 IDH_TERM_NEWPHONE_DEVICE,
								  IDC_PB_EDITICON,	 IDH_TERM_PHONEPROP_CHANGEICON,
                                  IDCANCEL,                           IDH_CANCEL,
                                  IDOK,                               IDH_OK,
								  0,0,
								  };
#endif
	pSDS	 pS;

	switch (uMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));

		if (pS == (SDS *)0)
			{
			assert(FALSE);
			EndDialog(hwnd, FALSE);
			break;
			}

		// In the effort to keep the internal driver handle internal
		// we are passing the session handle from the property sheet tab
		// dialog.
		//
		pS->hSession = (HSESSION)(((LPPROPSHEETPAGE)lPar)->lParam);

		pS->hDriver = cnctQueryDriverHdl(sessQueryCnctHdl(pS->hSession));
		hhDriver = (HHDRIVER)(pS->hDriver);

		SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pS);

		// In order to center the property sheet we need to center the parent
		// of the hwnd on top of the session window.
		// If the parent of hwnd is the session window then this dialog has
		// not been called from the property sheet.
		//
		hwndParent = GetParent(hwnd);

		if (hwndParent != sessQueryHwnd(pS->hSession))
			mscCenterWindowOnWindow(hwndParent, sessQueryHwnd(pS->hSession));

		else
			mscCenterWindowOnWindow(hwnd, sessQueryHwnd(pS->hSession));

		// Display the session icon...
		//
		pS->nIconID = sessQueryIconID(hhDriver->hSession);
		pS->hIcon = sessQueryIcon(hhDriver->hSession);
		//pS->hLittleIcon = sessQueryLittleIcon(hhDriver->hSession);

		SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
			(WPARAM)pS->hIcon, 0);

		SendDlgItemMessage(hwnd, IDC_EB_IPADDR, EM_SETLIMITTEXT,
			sizeof(hhDriver->achDestAddr)-1, 0);

		if (hhDriver->achDestAddr[0])
			SetDlgItemText(hwnd, IDC_EB_IPADDR, hhDriver->achDestAddr);

		wsprintf(achPort, "%d", hhDriver->iPort);
		SetDlgItemText(hwnd, IDC_EB_PORTNUM, achPort);

		TCHAR_Fill(pS->acSessNameCopy, TEXT('\0'),
			sizeof(pS->acSessNameCopy) / sizeof(TCHAR));

		sessQueryName(hhDriver->hSession, pS->acSessNameCopy,
			sizeof(pS->acSessNameCopy));

		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		lstrcpy(ach, pS->acSessNameCopy);
		mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach);
		SetDlgItemText(hwnd, IDC_TB_NAME, ach);

		return 0;

	case WM_CONTEXTMENU:
		//WinHelp((HWND)wPar, glblQueryHelpFileName(), HELP_CONTEXTMENU,
			//(DWORD)(LPSTR)aHlpTable);
		break;

	case WM_HELP:
	   //	WinHelp(((LPHELPINFO)lPar)->hItemHandle, glblQueryHelpFileName(),
			//HELP_WM_HELP, (DWORD)(LPSTR)aHlpTable);
		break;

	case WM_DESTROY:
		// OK, now we know that we are actually leaving the dialog for good, so
		// free the storage...
		//
		pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
		if (pS)
			{
			free(pS);
			pS = NULL;
			}

		break;

	case WM_NOTIFY:
		//
		// Property sheet messages are being channeled through here...
		//
		// TODO:mcc ??? return wsck_WM_NOTIFY(hwnd, (int)((NMHDR *)lPar)->code);
		break;

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{

		//
		// Property sheet's TAB_PHONENUMBER dialog is using this dialog proc
		// also, the following two buttons appear only in this tabbed dialog
		// template.
		//
		case IDC_PB_EDITICON:
			{
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);

			sessQueryName(pS->hSession, acNameCopy, sizeof(acNameCopy));

			if (DialogBoxParam(glblQueryDllHinst(),
				MAKEINTRESOURCE(IDD_NEWCONNECTION),
					hwnd, NewConnectionDlg,
						(LPARAM)pS->hSession) == FALSE)
				{
				return 0;
				}

			SetFocus(GetDlgItem(hwnd, IDC_PB_EDITICON));
			ach[0] = TEXT('\0');
			sessQueryName(pS->hSession, ach, sizeof(ach));
			mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach);
			SetDlgItemText(hwnd, IDC_TB_NAME, ach);

			SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
				(WPARAM)sessQueryIcon(pS->hSession), 0);

			// The user may have changed the name of the session.
			// The new name should be reflected in the property sheet title
			// and in the app title.
			//
			propUpdateTitle(pS->hSession, hwnd, acNameCopy);
			}
			break;

		case IDC_PB_CONFIGURE:
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
			hhDriver = (HHDRIVER)(pS->hDriver);

			break;

		case IDOK:
			wsck_SAVE_NEWIPADDR(hwnd);
			EndDialog(hwnd, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
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
 *  wsck_SAVE_NEWIPADDR
 *
 * DESCRIPTION:
 *	Saves chanes to the address/port setting back to the handle
 *
 * ARGUMENTS:
 *	hwnd		Window handle of "New Phonebook Entry" dialog
 *
 * RETURNS:
 *	0 		if data entered for all necessary values
 * 	-1		otherwise
 *
 */
STATIC_FUNC int wsck_SAVE_NEWIPADDR(HWND hwnd)
  	{
	int			iErr = 0;
	pSDS		pS;
	HHDRIVER	hhDriver;
	TCHAR		achPort[6];

	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);


	/* --- Get port number --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_PORTNUM)))
		{
		GetDlgItemText(hwnd, IDC_EB_PORTNUM, achPort,
			sizeof(achPort));
		hhDriver->iPort = atoi(achPort);
		}
	else
		{
		iErr = -1;
		goto cleanup;
		}

	/* --- Get IP Address --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_IPADDR)))
		{
		GetDlgItemText(hwnd, IDC_EB_IPADDR, hhDriver->achDestAddr,
			sizeof(hhDriver->achDestAddr));
		}
	else
		iErr = -1;

cleanup:
	return iErr;
	}


#endif
