/*	File: D:\WACKER\cncttapi\pcmcia.c (Created: 28-Feb-1995)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:19p $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#include <tapi.h>
#pragma hdrstop

#include <prsht.h>
#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\misc.h>
#include <tdll\cnct.h>

#include "cncttapi.h"
#include "cncttapi.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ConfirmDlg
 *
 * DESCRIPTION:
 *	PCMCIADlg pops up when a hotplug modem is specified but
 *	not inservice.
 *
 * AUTHOR: Mike Ward, 28-Feb-1995
 */
INT_PTR CALLBACK PCMCIADlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	HHDRIVER hhDriver;

	switch (uMsg)
		{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, DWLP_USER, lPar);
		hhDriver = (HHDRIVER)lPar;
		hhDriver->hwndPCMCIA = hwnd;
		mscCenterWindowOnWindow(hwnd, sessQueryHwnd(hhDriver->hSession));
		break;

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{
		case IDOK:
			// There is no OK button.  Instead, when the user plugs the
			// modem in, the tapi callback function will send a message.
			//
			EndDialog(hwnd, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;

		default:
			break;
			}
		break;

	case WM_DESTROY:
		hhDriver = (HHDRIVER)GetWindowLongPtr(hwnd, DWLP_USER);

		if (hhDriver)
			hhDriver->hwndPCMCIA = 0;

		break;

	default:
		return FALSE;
		}

	return TRUE;
	}
