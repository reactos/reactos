/*	File: D:\WACKER\tdll\sessmenu.c (Created: 30-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 9/23/99 8:58a $
 */
// #define	DEBUGSTR	1

#include <windows.h>
#pragma hdrstop

#include <time.h>		// goes with cnct.h

#include "stdtyp.h"
#include "session.h"
#include "emu\emu.h"
#include "term.h"
#include "print.h"
#include <term\res.h>
#include <tdll\assert.h>
#include <tdll\capture.h>
#include <tdll\globals.h>
#include <tdll\xfer_msc.h>
#include "cnct.h"
#if defined(INCL_NAG_SCREEN)
    #include "nagdlg.h"
    #include "register.h"
#endif

static void MenuItemCheck(const HMENU hMenu, const UINT uID, BOOL fChecked);
static void MenuItemEnable(const HMENU hMenu, const UINT uID, BOOL fEnable);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessInitMenuPopupEdit
 *
 * DESCRIPTION:
 *	Initializes edit menu just before display.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	hMenu		- edit popup menu handle
 *
 * RETURNS:
 *	void
 *
 */
void sessInitMenuPopupCall(const HSESSION hSession, const HMENU hMenu)
	{
	BOOL	fCheck = FALSE;
	HCNCT	hCnct = (HCNCT)0;
	int		iRet = 0;

	// Enable disconnect option only if we are connected.
	//
	hCnct = sessQueryCnctHdl(hSession);

	if (hCnct)
		iRet = cnctQueryStatus(hCnct);
											
	fCheck = (iRet == CNCT_STATUS_TRUE);

	MenuItemEnable(hMenu, IDM_ACTIONS_DIAL,  !fCheck);
	MenuItemEnable(hMenu, IDM_ACTIONS_HANGUP, fCheck);

#ifdef INCL_CALL_ANSWERING
    // Enable "Wait for a Call" if we are not connected and not waiting.
    //
    fCheck = (iRet != CNCT_STATUS_TRUE && iRet != CNCT_STATUS_ANSWERING);
    MenuItemEnable(hMenu, IDM_ACTIONS_WAIT_FOR_CALL, fCheck);

    // Enable "Stop Waiting" if we are waiting for a call.
    //
    fCheck = (iRet == CNCT_STATUS_ANSWERING);
    MenuItemEnable(hMenu, IDM_ACTIONS_STOP_WAITING, fCheck);
    //Disable "Call" if we are waiting for a call - mpt 09-08-99
    MenuItemEnable(hMenu, IDM_ACTIONS_DIAL, !fCheck);
#endif
	return;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessInitMenuPopupEdit
 *
 * DESCRIPTION:
 *	Initializes edit menu just before display.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	hMenu		- edit popup menu handle
 *
 * RETURNS:
 *	void
 *
 */
void sessInitMenuPopupEdit(const HSESSION hSession, const HMENU hMenu)
	{
	BOOL	fCheck = FALSE, f;
	HCNCT	hCnct = (HCNCT)0;
	int		iRet = 0;

	// Don't enable the copy menu item unless we have something to copy.
	//
	if (SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_Q_MARKED, 0, 0))
		fCheck = TRUE;

	MenuItemEnable(hMenu, IDM_COPY, fCheck);

	// Enable Paste to Host if there is something on the clipboard and
	// we are connected.
	//
	hCnct = sessQueryCnctHdl(hSession);

	if (hCnct)
		iRet = cnctQueryStatus(hCnct);

	fCheck = IsClipboardFormatAvailable(CF_TEXT);

	f = fCheck && (iRet == CNCT_STATUS_TRUE);
    MenuItemEnable(hMenu, IDM_PASTE, f);
	DbgOutStr("Enable IDM_PASTE %d %d %d\r\n", f, fCheck, iRet == CNCT_STATUS_TRUE, 0,0,);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessInitMenuPopupView
 *
 * DESCRIPTION:
 *	Initializes view menu just before display.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	hMenu		- view popup menu handle
 *
 * RETURNS:
 *	void
 *
 */
void sessInitMenuPopupView(const HSESSION hSession, const HMENU hMenu)
	{
	BOOL f;
#if defined(TESTMENU) && !defined(NDEBUG)
	const HWND hwndTerm = sessQueryHwndTerminal(hSession);
#endif
	const HWND hwndToolbar = sessQueryHwndToolbar(hSession);
	const HWND hwndStatusbar = sessQueryHwndStatusbar(hSession);

#if defined(TESTMENU) && !defined(NDEBUG)
	f = (BOOL)SendMessage(hwndTerm, WM_TERM_Q_BEZEL, 0, 0);
	MenuItemCheck(hMenu, IDM_TEST_BEZEL, f);
#endif

	f = IsWindow(hwndToolbar) && sessQueryToolbarVisible(hSession);
	MenuItemCheck(hMenu, IDM_VIEW_TOOLBAR, f);

	f = IsWindow(hwndStatusbar) && sessQueryStatusbarVisible(hSession);
	MenuItemCheck(hMenu, IDM_VIEW_STATUS, f);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	sessInitMenuPopupActions
 *
 * DESCRIPTION:
 *	This function gets called when the Actions menu is about to be displayed
 *	so that any last minute changes can be made.
 *
 * PARAMETERS:
 *	hSession	- external session handle
 *	hMenu		- view popup menu handle
 *
 * RETURNS:
 *	void
 *
 */

#define TRANSFER_CAPTURE_OFFSET 2

void sessInitMenuPopupTransfer(const HSESSION hSession, const HMENU hMenu)
	{
	int nMode;
	BOOL f;
	VOID *pData;
	MENUITEMINFO stM;
	TCHAR acMessage[64];
	HMENU hSubMenu;


	pData = (VOID *)0;
	xfrQueryDataPointer(sessQueryXferHdl(hSession), &pData);

	/*
	 * A NULL pointer means no transfer in progress, a non-NULL pointer
	 * means that someone is transferring.
	 */
	f = (pData == (VOID *)0);

	MenuItemEnable(hMenu, IDM_ACTIONS_SEND, f);
	MenuItemEnable(hMenu, IDM_ACTIONS_RCV, f);

	/*
	 * This section is for the Capture Menu.  It is more of a pain.
	 */
	nMode = cpfGetCaptureState(sessQueryCaptureFileHdl(hSession));
	if (nMode == CPF_CAPTURE_OFF)
		{
		/* Set things so that they can get to the dialog box */
		LoadString(glblQueryDllHinst(),
					IDS_CPF_CAP_OFF,
					acMessage,
					sizeof(acMessage) / sizeof(TCHAR));

		memset(&stM, 0, sizeof(MENUITEMINFO));

		stM.cbSize = sizeof(MENUITEMINFO);
		stM.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
		stM.wID = IDM_ACTIONS_CAP;
		stM.fType = MFT_STRING;
		stM.hSubMenu = (HMENU)0;
		stM.dwTypeData = (LPTSTR)acMessage;

		DbgOutStr("Setting Capture to start dialog\r\n", 0,0,0,0,0);

		SetMenuItemInfo(hMenu,
						TRANSFER_CAPTURE_OFFSET,
						TRUE,			/* By Position */
						&stM);
		}
	else
		{
		LoadString(glblQueryDllHinst(),
					IDS_CPF_CAP_ON,
					acMessage,
					sizeof(acMessage) / sizeof(TCHAR));

		hSubMenu = cpfGetCaptureMenu(sessQueryCaptureFileHdl(hSession));

		memset(&stM, 0, sizeof(MENUITEMINFO));
		stM.cbSize = sizeof(MENUITEMINFO);
		stM.fMask = MIIM_TYPE | MIIM_SUBMENU;
		stM.hSubMenu = hSubMenu;
		stM.dwTypeData = (LPTSTR)acMessage;

		/* Set up the cascade for the alternative choices */
		switch (nMode)
			{
			default:
			case CPF_CAPTURE_ON:
				/* Disable RESUME, enable PAUSE */
				MenuItemEnable(hSubMenu, IDM_CAPTURE_RESUME, FALSE);
				MenuItemEnable(hSubMenu, IDM_CAPTURE_PAUSE,  TRUE);
				break;
			case CPF_CAPTURE_PAUSE:
				/* Disable PAUSE, enable RESUME */
				MenuItemEnable(hSubMenu, IDM_CAPTURE_RESUME, TRUE);
				MenuItemEnable(hSubMenu, IDM_CAPTURE_PAUSE,  FALSE);
				break;
			}

		DbgOutStr("Setting Capture to cascade menu\r\n", 0,0,0,0,0);

		SetMenuItemInfo(hMenu,
						TRANSFER_CAPTURE_OFFSET,
						TRUE,			/* By Position */
						&stM);
		}

	// Display setup for Printer Echo option.

	MenuItemEnable(hMenu, IDM_ACTIONS_PRINT,  TRUE);
	f = printQueryStatus(emuQueryPrintEchoHdl(sessQueryEmuHdl(hSession)));
	MenuItemCheck(hMenu, IDM_ACTIONS_PRINT, f);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessInitMenuPopupView
 *
 * DESCRIPTION:
 *	Initializes view menu just before display.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	hMenu		- view popup menu handle
 *
 * RETURNS:
 *	void
 *
 */
void sessInitMenuPopupHelp(const HSESSION hSession, const HMENU hMenu)
	{
#if defined(INCL_NAG_SCREEN)
    if ( !IsEval() )
        {
        MenuItemEnable(hMenu, IDM_PURCHASE_INFO, FALSE);
        MenuItemEnable(hMenu, IDM_REG_CODE, FALSE);
        }

    // If they are already registered take this menu item off
    //
    if (IsRegisteredUser())
        {
        DeleteMenu(hMenu, IDM_REGISTER, MF_BYCOMMAND);
        }
#endif  

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	MenuItemCheck
 *
 * DESCRIPTION:
 *	Once again the menu functions have changed.  Checking and unchecking
 *	menu items is a bit more complicated so I wrote a function to handle
 *	it.  Other common menu operations should be handled this way. - mrw
 *
 * ARGUMENTS:
 *	hMenu	- handle of menu
 *	uID 	- id of menu item (position not supported)
 *	fChecked- TRUE if item is to be checked
 *
 * RETURNS:
 *	void
 *
 */
static void MenuItemCheck(const HMENU hMenu, const UINT uID, BOOL fChecked)
	{
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;
	mii.fState = (fChecked) ? MFS_CHECKED : MFS_UNCHECKED;
	mii.wID = uID;

	SetMenuItemInfo(hMenu, uID, FALSE, &mii);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *	Please see the previous function.
 *
 * ARGUEMENTS:
 *	Please see the previous function.
 *
 * RETURNS:
 *	Nothing.
 */

static void MenuItemEnable(const HMENU hMenu, const UINT uID, BOOL fEnable)
	{
	MENUITEMINFO mii;

	memset(&mii, 0, sizeof(MENUITEMINFO));
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STATE;
	mii.fState = (fEnable) ? (MFS_ENABLED) : (MFS_DISABLED | MFS_GRAYED);
	mii.wID = uID;

	SetMenuItemInfo(hMenu, uID, FALSE, &mii);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	HandleContextMenu
 *
 * DESCRIPTION:
 *	Load and display the context menu.
 *
 * ARGUMENTS:
 *	hwnd	- session window handle.
 *	point 	- where the user clicked.
 *
 * RETURNS:
 *	void.
 *
 */
void HandleContextMenu(HWND hwnd, POINT point)
	{
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	HMENU hMenu;
	HMENU hMenuTrackPopup;
	BOOL  fCheck = FALSE, f;
	HCNCT hCnct = (HCNCT)0;
	int   iRet = 0;

	hMenu = LoadMenu(glblQueryDllHinst() , "SessionContextMenu");
	if (!hMenu)
		return;

	// Don't enable the copy menu item unless we have something to copy.
	//
	if (SendMessage(sessQueryHwndTerminal(hSession), WM_TERM_Q_MARKED, 0, 0))
		fCheck = TRUE;

	// Enable - 'Copy' menu item
	//
	MenuItemEnable(hMenu, IDM_CONTEXT_COPY, fCheck);

	// Enable - 'Paste to Host' menu item
	//
	hCnct = sessQueryCnctHdl(hSession);
	if (hCnct)
		iRet = cnctQueryStatus(hCnct);

	fCheck = IsClipboardFormatAvailable(CF_TEXT);

	f = fCheck && (iRet == CNCT_STATUS_TRUE);
    MenuItemEnable(hMenu, IDM_CONTEXT_PASTE, f);

	/* --- Snap doesn't make sense when we're maximized - mrw --- */

	if (IsZoomed(hwnd))
		MenuItemEnable(hMenu, IDM_CONTEXT_SNAP, FALSE);

	/* --- Normal context menu stuff --- */

	hMenuTrackPopup = GetSubMenu(hMenu, 0);

	ClientToScreen(hwnd, (LPPOINT)&point);

	TrackPopupMenu(hMenuTrackPopup, 0, point.x, point.y, 0, hwnd, NULL);

	DestroyMenu(hMenu);
	}
