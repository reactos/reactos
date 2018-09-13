/*	File: D:\WACKER\tdll\dodialog.c (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "session.h"
#include "tdll.h"
#include "globals.h"
#include "statusbr.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	DoDialog
 *
 * DESCRIPTION: Use this routine to call dialogs.  It creates ProcInstance
 *				of the dialog procedure automatically and destroys it on
 *				exit.
 *
 * ARGUMENTS:	hInstance	   - instance handle of template's module
 *				lpTemplateName - name of dialog-box template
 *				hwndParent	   - window thats get focus when done
 *				lpProc		   - far pointer to the dialog procedure.
 *							   - Note: lpProc is NOT the pointer
 *								 obtained from MakeProcInstance...
 *				lPar		   - Can be used to pass data to dlg proc.
 *
 * RETURNS: 	Whatever the dilogbox returns on exit.
 *
 */
INT DoDialog(HINSTANCE hInstance, LPCTSTR lpTemplateName,
			 HWND hwndParent, DLGPROC lpProc, LPARAM lPar)
	{
	//DLGPROC	  lpfnDlgProc;
	INT 		sRetVal;			// return value for DialogBox()
	HWND		hwndFrame;
	HSESSION	hSession;

	// Normal dialog box stuff...

	// lpfnDlgProc = (DLGPROC)MakeProcInstance((FARPROC)lpProc, hInstance);

	sRetVal = (INT)DialogBoxParam(hInstance, lpTemplateName, hwndParent,
		lpProc, lPar);

	// FreeProcInstance((FARPROC)lpfnDlgProc);

	#if !defined(NDEBUG)
		if (sRetVal == -1)
			{
			TCHAR str[128], awch[50];

			OemToChar("Couldn't load %s. (%s, %d)", awch);
			wsprintf(str, awch, lpTemplateName, (LPTSTR)__FILE__, __LINE__);

			OemToChar("Internal Error", awch);
			MessageBox(hwndParent, str, awch, MB_OK | MB_ICONHAND);
			}
	#endif

	// We should force the statusbar window ro refresh its display
	// here to reflect the state of the keys the user might have pressed while
	// the dialog was up.
	//
	// Yes, I know we are relying on the fact that the frame window is the
	// the session window.  This may have to change in UPPER-WACKER.
	//
	hwndFrame = glblQueryHwndFrame();

	if (IsWindow(hwndFrame) && (hwndFrame == hwndParent))
		{
		hSession = (HSESSION)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
		PostMessage(sessQueryHwndStatusbar(hSession), SBR_NTFY_REFRESH,
			SBR_KEY_PARTS, 0);
		}

	return sRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	DoModelessDialog
 *
 * DESCRIPTION: This is a verion of the previous function that differs
 *				in that it creates and registers a modeless dialog
 *
 * ARGUMENTS:	hInstance	   - instance handle of template's module
 *				lpTemplateName - name of dialog-box template
 *				hwndOwner	   - window thats get focus when done
 *				lpProc		   - far pointer to the dialog procedure.
 *							   - Note: lpProc is NOT the pointer
 *								 obtained from MakeProcInstance...
 *				lPar		   - Can be used to pass data to dlg proc.
 *
 * RETURNS:
 *	The window handle of the dialog box that was created.
 *
 */
HWND DoModelessDialog(HINSTANCE hInstance, LPCTSTR lpTemplateName,
			 HWND hwndOwner, DLGPROC lpProc, LPARAM lPar)
	{
	HWND		hwndBox;

	hwndBox = CreateDialogParam(hInstance,
								lpTemplateName,
								hwndOwner,
								lpProc,
								lPar);

	if (hwndBox)
		{
		glblAddModelessDlgHwnd(hwndBox);
		}

	#if !defined(NDEBUG)
		if (hwndBox == NULL)
			{
			TCHAR str[128], awch[50];

			OemToChar("Couldn't load %s. (%s, %d)", awch);
			wsprintf(str, awch, lpTemplateName, (LPTSTR)__FILE__, __LINE__);

			OemToChar("Internal Error", awch);
			MessageBox(hwndOwner, str, awch, MB_OK | MB_ICONHAND);
			}
	#endif

	return hwndBox;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	EndModelessDialog
 *
 * DESCRIPTION:
 *	This function is used to remove a modeless dialog from whatever we are
 *	doing with it.
 *
 * PARAMETERS:
 *	hDlg -- the window handle of the modeless dialog
 *
 * RETURNS:
 *	ZERO for now.  Maybe something else later.
 *
 */
INT EndModelessDialog(HWND hDlg)
	{
	if (IsWindow(hDlg))
		PostMessage(glblQueryHwndFrame(), WM_SESS_ENDDLG, 0, (LPARAM)hDlg);

	return 0;
	}
