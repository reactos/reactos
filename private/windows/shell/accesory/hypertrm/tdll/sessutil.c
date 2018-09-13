/*	File: D:\WACKER\tdll\sessutil.c (Created: 30-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#include <time.h>

#include <tdll\features.h>

#include "stdtyp.h"
#include "sf.h"
#include "mc.h"
#include "term.h"
#include "cnct.h"
#include "print.h"
#include "assert.h"
#include "capture.h"
#include "globals.h"
#include "sess_ids.h"
#include "load_res.h"
#include "open_msc.h"
#include "xfer_msc.h"
#include "file_msc.h"
#include "backscrl.h"
#include "cloop.h"
#include "com.h"
#include <term\res.h>
#include "session.h"
#include "session.hh"
#include "errorbox.h"
#include <emu\emu.h>
#include "tdll.h"
#include "tchar.h"
#include "misc.h"
#include "nagdlg.h"

STATIC_FUNC void sessPreventOverlap(const HSESSION hSession, BOOL fIsToolbar);
STATIC_FUNC int sessCountMenuLines(HWND hwnd);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSnapToTermWindow
 *
 * DESCRIPTION:
 *	Sizes the session window so that the terminal is full size (usually
 *	80 x 24).
 *
 * ARGUMENTS:
 *	hwnd	- session window handle
 *
 * RETURNS:
 *	void
 *
 */
void sessSnapToTermWindow(const HWND hwnd)
	{
	RECT rc;
	RECT rc2;
	LONG lw;
	LONG lh;
	LONG l2w;
	LONG l2h;
	const HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	/* --- Doesn't make much sense to snap when we're maximized - mrw --- */

	if (IsZoomed(hwnd))
		return;

	if (sessComputeSnapSize(hSession, &rc))
		{
		// Make sure we don't go beyond size of screen
		// mrw,1/20/95
		//
		if (SystemParametersInfo(SPI_GETWORKAREA, sizeof(LPRECT), &rc2, 0))
			{
			lw = rc.right - rc.left;
			lh = rc.bottom - rc.top;

			l2w = rc2.right - rc2.left;
			l2h = rc2.bottom - rc2.top;

			GetWindowRect(hwnd, &rc);

			// When we first start-up we move the window off screen.
			// If we're off screen, we don't do any screen bounds
			// checking.
			//
			if (rc.top > rc2.bottom)
				{
				SetWindowPos(hwnd, 0, 0, 0, lw, lh, SWP_NOMOVE);
				}

			else
				{
				// Calculate new size in desktop coordinates.
				//
				rc.right = rc.left + lw;
				rc.bottom = rc.top + lh;

				// Check if too wide
				//
				if (lw > l2w)
					{
					rc.left = rc2.left;
					rc.right = rc2.right;
					}

				// Check if to high
				//
				if (lh > l2h)
					{
					rc.top = rc2.top;
					rc.bottom = rc2.bottom;
					}

				// Check if we're off to the right
				//
				if (rc.right > rc2.right)
					{
					lw = rc.right - rc2.right;
					rc.right -= lw;
					rc.left  -= lw;
					}

				// Check if we're off the bottom
				//
				if (rc.bottom > rc2.bottom)
					{
					lh = rc.bottom - rc2.bottom;
					rc.bottom -= lh;
					rc.top -= lh;
					}

				SetWindowPos(hwnd, 0, rc.left, rc.top,
					 rc.right-rc.left, rc.bottom-rc.top, 0);
				}
			}
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSetMinMaxInfo
 *
 * DESCRIPTION:
 *	Calculates the max horizontal size for the session and sets that
 *	value in the mmi structure.  If the window is being maximized, then
 *	we just return.  Later, we'll have flags to control autosnapping so
 *	this function will change.	Also some minimum tracking size is set.
 *
 * ARGUMENTS:
 *	hSession	- public session handle.
 *	pmmi		- pointer to MINMAXINFO structure
 *
 * RETURNS:
 *	void
 *
 */
void sessSetMinMaxInfo(const HSESSION hSession, const PMINMAXINFO pmmi)
	{
	RECT 				rc;
	HWND 				hwndStatusbar, hwndToolbar,  hwndSess;
	NONCLIENTMETRICS 	stNCM;
	int					i, iLineCnt = 0;

	/* --- Believe it or not, this gets called before WM_CREATE --- */

	if (hSession == (HSESSION)0)
		return;

	hwndSess = sessQueryHwnd(hSession);

	if (IsZoomed(hwndSess))
		return;

	#if 0	// removed on a trial basis - mrw
	if (sessComputeSnapSize(hSession, &rc))
		pmmi->ptMaxTrackSize.x = (rc.right - rc.left);
	#endif

	/* --- Set the minimum height for the session --- */

	memset(&rc, 0, sizeof(RECT));
	hwndStatusbar = sessQueryHwndStatusbar(hSession);

	if (IsWindow(hwndStatusbar) && IsWindowVisible(hwndStatusbar))
		{
		GetWindowRect(hwndStatusbar, &rc);
		pmmi->ptMinTrackSize.y += (rc.bottom - rc.top);
		}

	memset(&rc, 0, sizeof(RECT));
	hwndToolbar = sessQueryHwndToolbar(hSession);

	if (IsWindow(hwndToolbar) && IsWindowVisible(hwndToolbar))
		{
		GetWindowRect(hwndToolbar, &rc);
		pmmi->ptMinTrackSize.y += (rc.bottom - rc.top);
		}

	// Menus take up at least one iMenuHeight...
	//
	stNCM.cbSize = sizeof(NONCLIENTMETRICS);

	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
			sizeof(NONCLIENTMETRICS), &stNCM, 0) == TRUE)
		{
		pmmi->ptMinTrackSize.y += (stNCM.iMenuHeight - 1);

		// And if they take up more than that then adjust for it.
		//
		iLineCnt = sessCountMenuLines(hwndSess);

		for (i = 1; i < iLineCnt; i++)
			pmmi->ptMinTrackSize.y += (stNCM.iMenuHeight);

		DbgOutStr("%i, %i ", stNCM.iMenuHeight, iLineCnt, 0, 0, 0);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessCountMenuLines
 *
 * DESCRIPTION:
 *	Counts the lines a menu associated with a window is taking up.
 *
 * ARGUMENTS:
 *  hwnd - the window handle.
 *
 * RETURNS:
 *	How many lines a menu is taking up.
 *
 */
STATIC_FUNC int sessCountMenuLines(HWND hwnd)
	{
	int 	i, iLineCnt = 0, iRemembered = 0;
	HMENU 	hMenu;
	RECT	rc;

	hMenu = GetMenu(hwnd);
	memset(&rc, 0, sizeof(RECT));

	for (i = 0; i < GetMenuItemCount(hMenu); i++)
		{
		GetMenuItemRect(hwnd, hMenu, (UINT)i, &rc);
		if ((int)rc.bottom > iRemembered)
			{
			iRemembered = (int)rc.bottom;
			iLineCnt++;
			}
		}
	return (iLineCnt);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessComputeSnapSize
 *
 * DESCRIPTION:
 *	Computes the minimum size of the session window that will display
 *	the entire terminal screen.
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *	prc 		- pointer to rectangle that contains minimum size
 *
 * RETURNS:
 *	TRUE if success, FALSE contains worthless data
 *
 */
BOOL sessComputeSnapSize(const HSESSION hSession, const LPRECT prc)
	{
	RECT rcTmp;
	const HWND hwndTerm = sessQueryHwndTerminal(hSession);
	const HWND hwndToolbar = sessQueryHwndToolbar(hSession);
	const HWND hwndStatusbar = sessQueryHwndStatusbar(hSession);
	const HWND hwndSidebar = sessQuerySidebarHwnd(hSession);

	if (IsWindow(hwndTerm) == FALSE)
		return FALSE;

	/* --- Ask terminal for it's snapped size --- */

	SendMessage(hwndTerm, WM_TERM_Q_SNAP, 0, (LPARAM)prc);

	/* --- Compute the client window height --- */

	if (IsWindow(hwndToolbar) && sessQueryToolbarVisible(hSession))
		{
		GetWindowRect(hwndToolbar, &rcTmp);
		prc->top -= (rcTmp.bottom - rcTmp.top);
		}

	if (IsWindow(hwndStatusbar) && sessQueryStatusbarVisible(hSession))
		{
		GetWindowRect(hwndStatusbar, &rcTmp);
		prc->bottom += (rcTmp.bottom - rcTmp.top);
		}

	if (IsWindow(hwndSidebar) && IsWindowVisible(hwndSidebar))
		{
		GetWindowRect(hwndSidebar, &rcTmp);
		prc->right += (rcTmp.right - rcTmp.left);
		}

	/* --- Compute the necessary frame size --- */

	if (AdjustWindowRectEx(prc, WS_OVERLAPPEDWINDOW, TRUE, WS_EX_WINDOWEDGE))
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	NotifyClient
 *
 * DESCRIPTION:
 *	Called from the engine thread primarily, this function notifies the
 *	main thread of some event in the engine, such as new data for the
 *	terminal to display.
 *
 * ARGUMENTS:
 *	hSession	- external session handle
 *	nEvent		- event that occured
 *	lExtra		- extra data to pass
 *
 * RETURNS:
 *	void
 *
 */
void NotifyClient(const HSESSION hSession, const NOTIFICATION nEvent,
				  const long lExtra)
	{
	const HHSESSION hhSess = (HHSESSION)hSession;
	PostMessage(hhSess->hwndSess, WM_SESS_NOTIFY, (WPARAM)nEvent, lExtra);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void  sessInitializeIcons(HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hhSess->nIconId = IDI_PROG;
	hhSess->hIcon = extLoadIcon(MAKEINTRESOURCE(IDI_PROG));
	//hhSess->hIcon = LoadIcon(glblQueryDllHinst(),
	//						  MAKEINTRESOURCE(IDI_PROG));
	//hhSess->hLittleIcon = LoadIcon(glblQueryDllHinst(),
	//						  MAKEINTRESOURCE(IDI_PROG + IDI_PROG_ICON_CNT));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void  sessLoadIcons(HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	long lSize;

	lSize = sizeof(hhSess->nIconId);

	sfGetSessionItem(hhSess->hSysFile, SFID_ICON_DEFAULT, &lSize,
		&hhSess->nIconId);

	hhSess->hIcon = extLoadIcon(MAKEINTRESOURCE(hhSess->nIconId));
	//hhSess->hIcon = LoadIcon(glblQueryDllHinst(),
	//						  MAKEINTRESOURCE(hhSess->nIconId));
	//hhSess->hLittleIcon = LoadIcon(glblQueryDllHinst(),
	//						  MAKEINTRESOURCE(hhSess->nIconId+IDI_PROG_ICON_CNT));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
void  sessSaveIcons(HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	sfPutSessionItem(hhSess->hSysFile, SFID_ICON_DEFAULT,
		sizeof(hhSess->nIconId), &hhSess->nIconId);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSetSuspend
 *
 * DESCRIPTION:
 *	We need to block the flow of data going into the emulators which is
 *	done by suspending the CLoop.  Suspending is done for several reasons.
 *	User presses the scroll lock key.  User has pressed left mouse button
 *	down in preparation for marking (note: user could just let up mouse
 *	without marking so its a separate reason).	User is marking or has
 *	marked text.
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *	iReason 	- which event has called this routine
 *
 * RETURNS:
 *	void
 *
 */
void sessSetSuspend(const HSESSION hSession, const int iReason)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);

	switch (iReason)
		{
	case SUSPEND_SCRLCK:
		hhSession->fSuspendScrlLck = TRUE;
		break;

	case SUSPEND_TERMINAL_MARKING:
		hhSession->fSuspendTermMarking = TRUE;
		break;

	case SUSPEND_TERMINAL_LBTNDN:
		hhSession->fSuspendTermLBtnDn = TRUE;
		break;

	case SUSPEND_TERMINAL_COPY:
		hhSession->fSuspendTermCopy = TRUE;
		break;

	default:
		assert(FALSE);
		return;
		}

	CLoopRcvControl(sessQueryCLoopHdl(hSession), CLOOP_SUSPEND,
		CLOOP_RB_SCRLOCK);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessClearSuspend
 *
 * DESCRIPTION:
 *	Clearing suspends is essentially the opposite of setting execept that
 *	we do not resume cloop processing until all flags are FALSE.  Sets
 *	and clears are not cumulative which is what I first wanted to do but
 *	particularly in the area's of scroll lock keys and text marking the
 *	events are not always toggled (ie. might get many marks but only one
 *	unmark).  Also, event can be easily added although none come to mind.
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *	iReason 	- which event has called this routine
 *
 * RETURNS:
 *	void
 *
 */
void sessClearSuspend(const HSESSION hSession, const int iReason)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);

	switch (iReason)
		{
	case SUSPEND_SCRLCK:
		hhSession->fSuspendScrlLck = FALSE;
		break;

	case SUSPEND_TERMINAL_MARKING:
		hhSession->fSuspendTermMarking = FALSE;
		break;

	case SUSPEND_TERMINAL_LBTNDN:
		hhSession->fSuspendTermLBtnDn = FALSE;
		break;

	case SUSPEND_TERMINAL_COPY:
		hhSession->fSuspendTermCopy = FALSE;
		break;

	default:
		assert(FALSE);
		return;
		}

	if (!hhSession->fSuspendScrlLck && !hhSession->fSuspendTermMarking
			&& !hhSession->fSuspendTermLBtnDn && !hhSession->fSuspendTermCopy)
		{
		CLoopRcvControl(sessQueryCLoopHdl(hSession), CLOOP_RESUME,
			CLOOP_RB_SCRLOCK);
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	IsSessionSuspended
 *
 * DESCRIPTION:
 *	Checks if the session is suspended.  Suspends occur
 *	whenever the user is marking text, holds the mouse button down, or
 *	presses the scroll lock key.
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *
 * RETURNS:
 *	TRUE if were suspended.
 *
 */
BOOL IsSessionSuspended(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	return (hhSess->fSuspendScrlLck || hhSess->fSuspendTermMarking ||
			hhSess->fSuspendTermLBtnDn || hhSess->fSuspendTermCopy);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessQueryToolbarVisible
 *
 * DESCRIPTION:
 *	This function returns the expected visibility state of the Toolbar.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	The expected visibility state of the Toolbar.
 *
 */
BOOL sessQueryToolbarVisible(const HSESSION hSession)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);
	return hhSession->fToolbarVisible;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSetToolbarVisible
 *
 * DESCRIPTION:
 *	This function changes the expected visibility state of the Toolbar.
 *	It does not change the actual visibility of the Toolbar.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *	fVisible -- indicates visibility as TRUE or FALSE
 *
 * RETURNS:
 *	The previous visibility state of the Toolbar.
 *
 */
BOOL sessSetToolbarVisible(const HSESSION hSession, const BOOL fVisible)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);
	BOOL 			bRet = TRUE;

	bRet = hhSession->fToolbarVisible;
	hhSession->fToolbarVisible = (fVisible != FALSE);

	if (fVisible)
		sessPreventOverlap(hSession, TRUE);

	ShowWindow(hhSession->hwndToolbar, (fVisible) ? SW_SHOW : SW_HIDE);
	return bRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessQueryStatusbarVisible
 *
 * DESCRIPTION:
 *	This function returns the expected visibility state of the Statusbar.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	The expected visibility state of the Statusbar.
 *
 */
BOOL sessQueryStatusbarVisible(const HSESSION hSession)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);
	return hhSession->fStatusbarVisible;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSetStatusbarVisible
 *
 * DESCRIPTION:
 *	This function changes the expected visibility state of the Statusbar.
 *	It does not change the actual visibility of the Statusbar.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *	fVisible -- indicates visibility as TRUE or FALSE
 *
 * RETURNS:
 *	The previous visibility state of the Statusbar.
 *
 */
BOOL sessSetStatusbarVisible(const HSESSION hSession, const BOOL fVisible)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);
	BOOL 			bRet = TRUE;

	bRet = hhSession->fStatusbarVisible;
	hhSession->fStatusbarVisible = (fVisible != FALSE);

	if (fVisible)
		sessPreventOverlap(hSession, FALSE);

	ShowWindow(hhSession->hwndStatusbar, (fVisible) ? SW_SHOW : SW_HIDE);
	return bRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessCmdLnDial
 *
 * DESCRIPTION:
 *	Everytime we open a session, we call this function to attempt to
 *	dial.
 *
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *
 * RETURNS:
 *	void
 *
 */
void sessCmdLnDial(const HSESSION hSession)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);

	switch (hhSession->iCmdLnDial)
		{
	case CMDLN_DIAL_DIAL:
		cnctConnect(sessQueryCnctHdl(hSession), 0);
		break;

	case CMDLN_DIAL_NEW:
		cnctConnect(sessQueryCnctHdl(hSession), CNCT_NEW);
		break;

	case CMDLN_DIAL_WINSOCK:
		cnctConnect(sessQueryCnctHdl(hSession), CNCT_WINSOCK);
		break;

	case CMDLN_DIAL_OPEN:
	default:
		break;
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessUpdateAppTitle
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *  hwnd - app window.
 *
 * RETURNS:
 *  void.
 *
 */
void sessUpdateAppTitle(const HSESSION hSession)
	{
	HWND	hwnd = sessQueryHwnd(hSession);
	TCHAR	acTitle[256], acName[256], acNewTitle[256];
#ifdef INCL_NAG_SCREEN
	TCHAR   acUnregistered[256];
#endif
	LPTSTR	pNewTitle;
	int 	iSize;
	TCHAR	*pszSeperator = TEXT(" - ");
    BOOL    bEval = FALSE;

	TCHAR_Fill(acNewTitle, TEXT('\0'), sizeof(acNewTitle) / sizeof(TCHAR));
	TCHAR_Fill(acTitle, TEXT('\0'), sizeof(acTitle) / sizeof(TCHAR));

	sessQueryName(hSession, acName, sizeof(acName));
	if (sessIsSessNameDefault(acName))
		{
		LoadString(glblQueryDllHinst(), IDS_GNRL_NEW_CNCT, acName,
			sizeof(acName) / sizeof(TCHAR));
		}
 	StrCharCopy(acNewTitle, acName);

	LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, acTitle,
		sizeof(acTitle) / sizeof(TCHAR));

#ifdef INCL_NAG_SCREEN
    if ( IsEval() )
        {
        bEval = TRUE;
        LoadString(glblQueryDllHinst(), IDS_GNRL_UNREGISTERED, acUnregistered,
		    sizeof(acUnregistered) / sizeof(TCHAR));
        }
#endif

    iSize =  StrCharGetByteCount(acNewTitle);
	iSize += StrCharGetByteCount(pszSeperator);
	iSize += StrCharGetByteCount(acTitle);
#ifdef INCL_NAG_SCREEN
    if ( bEval )
        {
        iSize += StrCharGetByteCount(acUnregistered);
        }
#endif
	iSize += sizeof(TCHAR);

	pNewTitle = malloc(iSize);

	if (pNewTitle)
		{
		StrCharCopy(pNewTitle, acNewTitle);
		StrCharCat(pNewTitle, pszSeperator);
		StrCharCat(pNewTitle, acTitle);
#ifdef INCL_NAG_SCREEN
        if ( bEval )
            {
            StrCharCat(pNewTitle, acUnregistered);
            }
#endif
		SetWindowText(hwnd, pNewTitle);

		free(pNewTitle);
		pNewTitle = NULL;
		}
	else
		{
		SetWindowText(hwnd, acNewTitle);
		}

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessDisconnectToContinue
 *
 * DESCRIPTION:
 *	If connecterd, ask the user if he wants to disconnect and continue with
 *  whatever he set out to perform, such as close the app, open a new session,
 *	create a new connection, etc.
 *
 * ARGUMENTS:
 *  hSession 	- the Session handle.
 *  hwnd		- the session window.
 *
 * RETURNS:
 *  TRUE 	- if the user wants to disconnect and go on with his request.
 *	FALSE 	- if he changes his mind.
 *
 */
BOOL sessDisconnectToContinue(const HSESSION hSession, HWND hwnd)
	{
	HCNCT	hCnct = (HCNCT)0;
	int		iRet = 0;
	TCHAR	ach[256], achTitle[100];

	hCnct = sessQueryCnctHdl(hSession);
	if (hCnct)
		iRet = cnctQueryStatus(hCnct);

	if (iRet == CNCT_STATUS_TRUE)
		{
		LoadString(glblQueryDllHinst(), IDS_GNRL_CNFRM_DCNCT,
			ach, sizeof(ach) / sizeof(TCHAR));

		LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, achTitle,
			sizeof(achTitle) / sizeof(TCHAR));

		if ((iRet = TimedMessageBox(hwnd, ach, achTitle,
			MB_YESNO | MB_ICONEXCLAMATION, 0)) == IDYES)
			{
			SendMessage(hwnd, WM_COMMAND, IDM_ACTIONS_HANGUP, 0L);
			}
		else
			return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSizeAndShow
 *
 * DESCRIPTION:
 *	Called only once from the InitInstance() this function will size
 *	the window via snap and insure it lands on the desktop.  This
 *	function is called via a posted message because it was found that
 *	the session was not fully initialized after returning from the
 *	CreateWindow call.
 *
 * ARGUMENTS:
 *  hwnd		- session window.
 *	nCmdShow	- Show command passed from WinMain()
 *
 * RETURNS:
 *	void
 *
 */
void sessSizeAndShow(const HWND hwnd, const int nCmdShow)
	{
	HSESSION hSession = (HSESSION)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	const HWND hwndBanner = glblQueryHwndBanner();
	WINDOWPLACEMENT stWP;
	RECT	 rc, rc2, rc3;
	int 	 cx, cy;
	int 	 xWA, yWA, cxWA, cyWA;
	int		 iWidth = 0, iHeight = 0;

	// Size session window...
	//
	if (!sessQueryIsNewSession(hSession))
		{
		// Size and position the session window according to the
		// remembered values, if no values were remembered, i.e., an old
		// session, then treat it as a default session.
		//
		memset(&rc, 0, sizeof(RECT));
		sessQueryWindowRect(hSession, &rc);

		iWidth  = rc.right - rc.left;
		iHeight = rc.bottom - rc.top;

		if (iWidth != 0 && iHeight != 0)
			{
			stWP.length  = sizeof(WINDOWPLACEMENT);
			stWP.flags	 = 0;
			stWP.showCmd = (UINT)sessQueryWindowShowCmd(hSession);
			memmove(&stWP.rcNormalPosition, &rc, sizeof(RECT));

			SetWindowPlacement(hwnd, &stWP);
			UpdateWindow(hwnd);

			if (IsWindow(hwndBanner))
				PostMessage(hwndBanner, WM_CLOSE, 0, 0);

			return;
			}
		}

	// Well, were back to our stupid windows tricks.  The toolbar height
	// has changed with the introduction of the larger bitmaps (from
	// 16x16 to 22x24).  On program startup, the toolbar window reports
	// a default size which is not the correct size since the new bitmaps
	// will force it to be larger.	This doesn't happen however, until
	// the window is displayed.  So we display the window off screen,
	// do the resizing, then move it back on screen.

	GetWindowRect(hwnd, &rc);
	SetWindowPos(hwnd, 0, 20000, 20000, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// Size to terminal window for a default session, i.e., the user
	// did not specify a session name on the command line.
	//
	sessSnapToTermWindow(hwnd);

	// Get new window size but keep original origin
	//
	GetWindowRect(hwnd, &rc2);
	rc.right = rc.left + (rc2.right - rc2.left);
	rc.bottom = rc.top + (rc2.bottom - rc2.top);

	// Until the SPI parameters are defined
	//
	if (SystemParametersInfo(SPI_GETWORKAREA, sizeof(LPRECT), &rc3, 0) == TRUE)
		{
		xWA = rc3.left;
		yWA = rc3.top;
		cxWA = rc3.right - rc3.left;
		cyWA = rc3.bottom - rc3.top;
		}

	else
		{
		xWA = 0;
		yWA = 0;
		cxWA = GetSystemMetrics(SM_CXSCREEN);
		cyWA = GetSystemMetrics(SM_CYSCREEN);
		}

	cx = rc.left;
	cy = rc.top;

	if (rc.right > (xWA + cxWA))
		cx = max(xWA, rc.left - (rc.right - (xWA + cxWA)));

	if (rc.bottom > (yWA + cyWA))
		cy = max(yWA, rc.top - (rc.bottom - (yWA + cyWA)));

	// Move window back to either its original or adjusted position.
	//
	SetWindowPos(hwnd, 0, cx, cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	UpdateWindow(hwnd);

	if (IsWindow(hwndBanner))
		PostMessage(hwndBanner, WM_CLOSE, 0, 0);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessPreventOverlap
 *
 * DESCRIPTION:
 *	The main purpose for existance of this function is to prevent the toolbar
 *	window and the status window overlapping each other.  That is accomplished
 *	by sizing the session window to be big enough to display both of the
 *	windows comfortably.
 *
 * ARGUMENTS:
 *  hSession 	- the session handle.
 *	fIsToolbar 	- TRUE if we showing the toolbar window, FALSE otherwise.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void sessPreventOverlap(const HSESSION hSession, BOOL fIsToolbar)
	{
	const HHSESSION hhSession = VerifySessionHandle(hSession);
	RECT			rcSess, rcTool, rcStat;

	if ((fIsToolbar) ? hhSession->fStatusbarVisible :
					   hhSession->fToolbarVisible)
		{
		GetWindowRect(hhSession->hwndSess, &rcSess);
		GetWindowRect(hhSession->hwndToolbar, &rcTool);
		GetWindowRect(hhSession->hwndStatusbar, &rcStat);

		if (rcTool.bottom > rcStat.top)
			{
			SetWindowPos(hhSession->hwndSess, 0,
				0, 0, rcSess.right - rcSess.left,
				(rcSess.bottom - rcSess.top) + (rcTool.bottom - rcStat.top),
				SWP_NOMOVE | SWP_NOZORDER);
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoBeeper
 *
 * DESCRIPTION:
 *	Produces three audible beeps in sequence
 *
 * ARGUMENTS:
 *	dw	- required by CreateThread()
 *
 * RETURNS:
 *	DWORD	- required by CreateThread
 *
 */
DWORD WINAPI DoBeeper(DWORD dw)
 	{
    //mpt:06-04-98 changed to use windows sounds
    MessageBeep(MB_ICONEXCLAMATION);
#if 0
    int i;

	for (i = 0 ; i < 3 ; ++i)
		{
		MessageBeep((unsigned)-1);
		Sleep(300);
		}
#endif
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessBeeper
 *
 * DESCRIPTION:
 *	Launches thread to do beeps if session setting is on.  Seems silly
 *	to launch a thread to do something like this but it really is the
 *	simplist and most direct way. - mrw
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *
 * RETURNS:
 *	void
 *
 */
void sessBeeper(const HSESSION hSession)
	{
	DWORD dwID;

	if (sessQuerySound(hSession))
		{
		if (CreateThread(0, 100, (LPTHREAD_START_ROUTINE)DoBeeper, 0, 0,
				&dwID) == 0)
			{
			DbgShowLastError();
			}
		}

	return;
	}
