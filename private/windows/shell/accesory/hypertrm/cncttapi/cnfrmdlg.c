/*      File: D:\WACKER\cncttapi\cnfrmdlg.c (Created: 23-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 3/26/99 8:05a $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#include <tapi.h>
#pragma hdrstop

#include <prsht.h>
#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\errorbox.h>
#include <tdll\cnct.h>
#include <tdll\hlptable.h>
#include <tdll\globals.h>
#include <tdll\property.h>
#include <term\res.h>
#include <tdll\open_msc.h>

#include "cncttapi.hh"
#include "cncttapi.h"

static void InitConfirmDlg(const HWND hwnd, const HHDRIVER hhDriver);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ConfirmDlg
 *
 * DESCRIPTION:
 *	Displays dialog confirming user's choices for the requested connect.
 *	Assumes that EnumerateLines() and TranslateAddress() have been called.
 *
 * ARGUMENTS:
 *	Standard dialog
 *
 * RETURNS:
 *	Standard dialog
 *
 */
INT_PTR CALLBACK ConfirmDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	#define IDC_TF_LOCATION 107
	#define CB_LOCATION 	108
	#define PB_EDIT_NEW 	109
    #define IDI_ICON        102
    #define IDC_TF_PHONE    104
	#define TB_PHONE		105
	#define TB_CARD 		110
	#define IDC_TF_CARD     111
	#define TB_SESSNAME 	103
	#define PB_MODIFY		106
	#define PB_DIAL			117

    TCHAR   ach[128];
	int i;
	long lRet;
	LRESULT lr;
	HHDRIVER hhDriver;
	static	DWORD aHlpTable[] = {	CB_LOCATION,	 IDH_TERM_DIAL_LOCATION,
									IDC_TF_LOCATION, IDH_TERM_DIAL_LOCATION,
									IDC_TF_PHONE,	 IDH_TERM_DIAL_PHONENUMBER,
									TB_PHONE,		 IDH_TERM_DIAL_PHONENUMBER,
									TB_CARD,		 IDH_TERM_DIAL_CALLING_CARD,
									IDC_TF_CARD,	 IDH_TERM_DIAL_CALLING_CARD,
									PB_MODIFY,		 IDH_TERM_DIAL_MODIFY,
									PB_EDIT_NEW,	 IDH_TERM_DIAL_EDITNEW,
									PB_DIAL,		 IDH_TERM_DIAL_DIAL,
                                    IDCANCEL,        IDH_CANCEL,
									0,0};
	switch (uMsg)
		{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, DWLP_USER, lPar);
		hhDriver = (HHDRIVER)lPar;
		mscCenterWindowOnWindow(hwnd, sessQueryHwnd(hhDriver->hSession));

		if ((lRet = TranslateAddress(hhDriver)) != 0)
			{
			if (lRet == LINEERR_INIFILECORRUPT)
				{
				PostMessage(hwnd, WM_COMMAND,
					MAKEWPARAM(PB_EDIT_NEW, BN_CLICKED), (LPARAM)hwnd);
				}
			}

		InitConfirmDlg(hwnd, hhDriver);

		EnumerateTapiLocations(hhDriver, GetDlgItem(hwnd, CB_LOCATION),
			GetDlgItem(hwnd, TB_CARD));

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{
		case PB_DIAL:
            GetDlgItemText(hwnd, TB_PHONE, ach, sizeof(ach)/sizeof(TCHAR));
            if ( strcmp(ach, TEXT("")) != 0 )
                {
			    hhDriver = (HHDRIVER)GetWindowLongPtr(hwnd, DWLP_USER);
			    EndDialog(hwnd, TRUE);
                }
            else
                {
    			LoadString(glblQueryDllHinst(), 40808, ach, sizeof(ach)/sizeof(TCHAR));
                MessageBox(hwnd, ach, TEXT("HyperTerminal"), MB_OK);
                }

			break;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;

		case PB_EDIT_NEW:
			hhDriver = (HHDRIVER)GetWindowLongPtr(hwnd, DWLP_USER);

			lineTranslateDialog(hhDriver->hLineApp, hhDriver->dwLine,
				TAPI_VER, hwnd, hhDriver->achCanonicalDest);

			EnumerateTapiLocations(hhDriver, GetDlgItem(hwnd, CB_LOCATION),
				GetDlgItem(hwnd, TB_CARD));

			if (TranslateAddress(hhDriver) == 0)
				InitConfirmDlg(hwnd, hhDriver);

			break;

		case PB_MODIFY:
			hhDriver = (HHDRIVER)GetWindowLongPtr(hwnd, DWLP_USER);

			EnableWindow(GetDlgItem(hwnd, PB_MODIFY), FALSE);
			DoInternalProperties(hhDriver->hSession,
				hwnd);

			EnableWindow(GetDlgItem(hwnd, PB_MODIFY), TRUE);
            SetFocus(GetDlgItem(hwnd, PB_MODIFY));

			if ( IsNT() )
				{
				EnumerateLinesNT(hhDriver, 0);
				}
			else
				{
				EnumerateLines(hhDriver, 0);
				}

			if (TranslateAddress(hhDriver) == 0)
				InitConfirmDlg(hwnd, hhDriver);

			break;

		case CB_LOCATION:
			if (HIWORD(wPar) == CBN_SELENDOK)
				{
				hhDriver = (HHDRIVER)GetWindowLongPtr(hwnd, DWLP_USER);

				if ((i = (int)SendDlgItemMessage(hwnd, CB_LOCATION, CB_GETCURSEL,
						0, 0)) != CB_ERR)
					{
					lr = SendDlgItemMessage(hwnd, CB_LOCATION, CB_GETITEMDATA,
						(WPARAM)i, 0);

					if (lr != CB_ERR)
						{
						if (lineSetCurrentLocation(hhDriver->hLineApp,
								(DWORD)lr) == 0)
							{
							// Leave the hwndCB (second paramater) zero.
							// Otherwise we go into and infinite message loop

							EnumerateTapiLocations(hhDriver, 0,
								GetDlgItem(hwnd, TB_CARD));

							if (TranslateAddress(hhDriver) == 0)
								InitConfirmDlg(hwnd, hhDriver);
							}
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
 *	InitConfirmDlg
 *
 * DESCRIPTION:
 *	Used to initialize some fields in the Confirmation dialog.
 *
 * ARGUMENTS:
 *	hwnd		- confirmation dialog
 *	hhDriver	- private driver handle.
 *
 * RETURNS:
 *	void
 *
 */
static void InitConfirmDlg(const HWND hwnd, const HHDRIVER hhDriver)
	{
	TCHAR ach[512];

	SetDlgItemText(hwnd, TB_PHONE, hhDriver->achDisplayableDest);

	SendDlgItemMessage(hwnd, 101, STM_SETICON,
			(WPARAM)sessQueryIcon(hhDriver->hSession), 0);

	sessQueryName(hhDriver->hSession, ach, sizeof(ach));
	mscModifyToFit(GetDlgItem(hwnd, TB_SESSNAME), ach);
	SetDlgItemText(hwnd, TB_SESSNAME, ach);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	TranslateAddress
 *
 * DESCRIPTION:
 *	Translates the country code, area code, phone number into canonical
 *	format and then lets tapi translate it into the final dialable format.
 *	What is canonical format you say?
 *
 *	+Country Code SPACE [Area Code] SPACE Subscriber Number
 *
 *	Assumes EnumerateLines() has been called and a default device was
 *	selected.
 *
 * ARGUMENTS:
 *	hhDriver	- private connection driver handle
 *
 * RETURNS:
 *	0=OK
 *
 */
long TranslateAddress(const HHDRIVER hhDriver)
	{
	LONG lRet = 1;
	DWORD dwSize;
	TCHAR ach[100];
	LPLINECOUNTRYLIST pcl;
	LPLINECOUNTRYENTRY pce;
	LINETRANSLATEOUTPUT *pLnTransOutput;

	if (hhDriver == 0)
		{
		assert(FALSE);
		return -1;
		}

	if (CheckHotPhone(hhDriver, hhDriver->dwLine, &hhDriver->fHotPhone) != 0)
		return -1;	// error message displayed already.

	// Hot Phone is TAPI terminology for Direct Connects
	// We don't need to do address translation since we
	// not going to use it.

	if (hhDriver->fHotPhone)
		{
		hhDriver->achDialableDest[0] = TEXT('\0');
		hhDriver->achDisplayableDest[0] = TEXT('\0');
		return 0;
		}

	ach[0] = TEXT('\0'); // initialize the string!

	// If we not using the country code or area code, we don't want
	// or need TAPI's line translation.	 Just copy the destination
	// as the user entered it in the phonenumber field.
	//
	if (hhDriver->fUseCCAC)
		{
		/* --- Do lineGetCountry to get extension --- */

		if (DoLineGetCountry(hhDriver->dwCountryID, hhDriver->dwAPIVersion,
			&pcl) != 0)
			{
			assert(FALSE);
			return 2;
			}

		if ((pce = (LPLINECOUNTRYENTRY)
			((BYTE *)pcl + pcl->dwCountryListOffset)) == 0)
			{
			assert(FALSE);
			return 3;
			}

		/* --- Put country code in now --- */

		wsprintf(ach, "+%u ", pce->dwCountryCode);
		free(pcl);
		pcl = NULL;

		/* --- Area code ---*/

		#if 0 // mrw:4/20/95 (see phonedlg.c)
		if (hhDriver->achAreaCode[0])  &&
			fCountryUsesAreaCode(hhDriver->dwCountryID,
			hhDriver->dwAPIVersion))
		#endif
			if (!fIsStringEmpty(hhDriver->achAreaCode))
			{
			lstrcat(ach, "(");
			lstrcat(ach, hhDriver->achAreaCode);
			lstrcat(ach, ") ");
			}
		}

	lstrcat(ach, hhDriver->achDest);

	/* --- Allocate some space --- */

	pLnTransOutput = malloc(sizeof(LINETRANSLATEOUTPUT));

	if (pLnTransOutput == 0)
		{
		assert (FALSE);
		return 4;
		}

	pLnTransOutput->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	/* --- Now that we've satisifed the clergy, translate it --- */

	if ((lRet = TRAP(lineTranslateAddress(hhDriver->hLineApp,
			hhDriver->dwLine, TAPI_VER, ach, 0,
				LINETRANSLATEOPTION_CANCELCALLWAITING,
					pLnTransOutput))) != 0)
		{
		free(pLnTransOutput);
		pLnTransOutput = NULL;
		return lRet;
		}

	if (pLnTransOutput->dwTotalSize < pLnTransOutput->dwNeededSize)
		{
		dwSize = pLnTransOutput->dwNeededSize;
		free(pLnTransOutput);
		pLnTransOutput = NULL;

		if ((pLnTransOutput = malloc(dwSize)) == 0)
			{
			assert(FALSE);
			return 5;
			}

		pLnTransOutput->dwTotalSize = dwSize;

		if ((lRet = TRAP(lineTranslateAddress(hhDriver->hLineApp,
				hhDriver->dwLine, TAPI_VER, ach, 0,
					LINETRANSLATEOPTION_CANCELCALLWAITING,
						pLnTransOutput))) != 0)
			{
			assert(FALSE);
			free(pLnTransOutput);
			pLnTransOutput = NULL;
			return lRet;
			}
		}

	/* --- At last, some strings to throw at the modem --- */

	lstrcpy(hhDriver->achDialableDest,
		(LPSTR)pLnTransOutput + pLnTransOutput->dwDialableStringOffset);

	lstrcpy(hhDriver->achDisplayableDest,
		(LPSTR)pLnTransOutput + pLnTransOutput->dwDisplayableStringOffset);

	hhDriver->dwCountryCode = pLnTransOutput->dwDestCountry;
	lstrcpy(hhDriver->achCanonicalDest, ach);

	free(pLnTransOutput);
	pLnTransOutput = NULL;
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CheckHotPhone
 *
 * DESCRIPTION:
 *	Checks to see if the selected line is a hot phone (ie. direct connect
 *	that requires no dialing).
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle.
 *	dwLine		- line to test
 *	pfHotPhone	- result
 *
 * RETURNS:
 *	0=OK
 *
 */
int CheckHotPhone(const HHDRIVER hhDriver, const DWORD dwLine, int *pfHotPhone)
	{
	DWORD	dw;
	LPLINEADDRESSCAPS pac = 0;

	if (hhDriver == 0)
		return -6;

	/* --- Get Address caps to determine line type --- */

	if ((pac = (LPLINEADDRESSCAPS)malloc(sizeof(*pac))) == 0)
		{
		return -1;
		}

	pac->dwTotalSize = sizeof(*pac);

	if (lineGetAddressCaps(hhDriver->hLineApp, dwLine, 0, TAPI_VER, 0,
			pac) != 0)
		{
		free(pac);
		pac = NULL;
		return -2;
		}

	if (pac->dwNeededSize > pac->dwTotalSize)
		{
		dw = pac->dwNeededSize;
		free(pac);
		pac = NULL;

		if ((pac = (LPLINEADDRESSCAPS)malloc(dw)) == 0)
			{
			return -3;
			}

		pac->dwTotalSize = dw;

		if (lineGetAddressCaps(hhDriver->hLineApp, dwLine,
				0, TAPI_VER, 0, pac) != 0)
			{
			free(pac);
			pac = NULL;
			return -4;
			}
		}

	*pfHotPhone = !(pac->dwAddrCapFlags & LINEADDRCAPFLAGS_DIALED);
	free(pac);
	pac = NULL;
	return 0;
	}
