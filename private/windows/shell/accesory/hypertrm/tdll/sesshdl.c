/*	File: D:\WACKER\tdll\sesshdl.c (Created: 01-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 8/05/99 2:31p $
 */

#include <windows.h>
#pragma hdrstop

#include <time.h>

#include <tdll\features.h>

#include "stdtyp.h"
#include "mc.h"
#include "assert.h"
#include "session.h"
#include "session.hh"
#include "sf.h"
#include "backscrl.h"
#include "globals.h"
#include "xfer_msc.h"
#include "file_msc.h"
#include "print.h"
#include <tdll\capture.h>
#include <tdll\timers.h>
#include <tdll\com.h>
#include <tdll\cloop.h>
#include <tdll\errorbox.h>
#include <tdll\tdll.h>
#include "tchar.h"
#include <term\res.h>
#include <emu\emu.h>
#include "update.h"
#include "cnct.h"
#include "statusbr.h"
#include "sess_ids.h"
#include "misc.h"
#include "translat.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateSessionHandle
 *
 * DESCRIPTION:
 *	Creates a session handle.  Note, hwndSession can be 0 if you need
 *	to create a stand alone session handle.
 *
 * ARGUMENTS:
 *	hwndSession - session window handle (can be 0)
 *
 * RETURNS:
 *	Session handle or 0.
 *
 */
HSESSION CreateSessionHandle(const HWND hwndSession)
	{
	HHSESSION hhSess;

	hhSess = (HHSESSION)malloc(sizeof(*hhSess));

	if (hhSess == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hhSess, 0, sizeof(*hhSess));

	hhSess->lPrefix = PRE_MAGIC;
	hhSess->lPostfix = POST_MAGIC;

	InitializeCriticalSection(&hhSess->csSess);
	hhSess->hwndSess = hwndSession;

	return (HSESSION)hhSess;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	InitializeSessionHandle
 *
 * DESCRIPTION:
 *	Does all the dirty work of initilizing the session handle
 *
 *	A special case for this code is when it gets called by the stuff in the
 *	shell extensions for property sheets.  This case can be recognised by the
 *	fact that the session window handle is NULL, as is the pointer to the
 *	CREATESTRUCT.
 *
 * ARGUMENTS:
 *	hSession	- session handle
 *	hwnd		- session window handle
 *  *pcs		- pointer to CREATESTRUCT, passed along from CreateWindowEx().
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL InitializeSessionHandle(const HSESSION hSession, const HWND hwnd,
							 const CREATESTRUCT *pcs)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	TCHAR 	ach[256], achTitle[100], achFormat[100];

	/* --- Any session command data is saved for later --- */

	if (pcs)
		{
		if (pcs->lpCreateParams)
			{
			StrCharCopy(hhSess->achSessCmdLn, pcs->lpCreateParams);
			}
		}

	/* --- Create multiplexed timer --- */

	if (hwnd)
		{
		if (TimerMuxCreate(hwnd, 0, &hhSess->hTimerMux) != TIMER_OK)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	/* --- Create status window using a common control --- */

	if (hwnd)
		{
		hhSess->hwndStatusbar = sbrCreateSessionStatusbar(hSession);

		if (!hhSess->hwndStatusbar)
			{
			assert(FALSE);
			return FALSE;
			}

		sessSetStatusbarVisible(hSession, TRUE);
		}

	/* --- Create a session toolbar --- */

	if (hwnd)
		{
		hhSess->hwndToolbar = CreateSessionToolbar(hSession, hwnd);

		if (!hhSess->hwndToolbar)
			{
			assert(FALSE);
			return FALSE;
			}

		sessSetToolbarVisible(hSession, TRUE);
		}

	if (hwnd)
		{
		hhSess->hwndSidebar = CreateSidebar(hwnd, hSession);
		}

	/* --- Create a Backscroll handle --- */

	hhSess->hBackscrl = backscrlCreate(hSession, 250*132);

	if (!hhSess->hBackscrl)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create an Update handle --- */

	hhSess->hUpdate = updateCreate(hSession);

	if (!hhSess->hUpdate)
		{
		assert(FALSE);
		return FALSE;
		}

	/* -- Create a CLoop handle --- */
	hhSess->hCLoop = CLoopCreateHandle(hSession);

	if (!hhSess->hCLoop)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create an Emulator handle --- */

	hhSess->hEmu = emuCreateHdl(hSession);

	if (!hhSess->hEmu)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create a terminal window --- */

	if (hwnd)
		{
		hhSess->hwndTerm = CreateTerminalWindow(hwnd);

		if (!hhSess->hwndTerm)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	/* -- Create a Com handle --- */
	if (ComCreateHandle(hSession, &hhSess->hCom) != COM_OK)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create a transfer handle --- */
	if (hwnd)
		{
		hhSess->hXferHdl = CreateXferHdl(hSession);
		if (!hhSess->hXferHdl)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	/* --- Create a Files and Directorys handle --- */
	if (hwnd)
		{
		hhSess->hFilesHdl = CreateFilesDirsHdl(hSession);
		if (!hhSess->hFilesHdl)
			{
			assert(FALSE);
			return FALSE;
			}
		}

	/* --- Create a session data file handle --- */

	hhSess->hSysFile = CreateSysFileHdl();
	if (hhSess->hSysFile == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create a connection handle --- */

	hhSess->hCnct = cnctCreateHdl(hSession);

	if (hhSess->hCnct == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create a capture file handle --- */

	hhSess->hCaptFile = CreateCaptureFileHandle(hSession);
	if (hhSess->hCaptFile == 0)
		{
		assert(FALSE);
		return FALSE;
		}

    /* --- Create a print handle --- */

	hhSess->hPrint = printCreateHdl(hSession);
	if (hhSess->hPrint == 0)
		{
		assert(FALSE);
		return FALSE;
		}

#if	defined(CHARACTER_TRANSLATION)
	hhSess->hTranslate = CreateTranslateHandle(hSession);
	if (hhSess->hTranslate == 0)
		{
		assert(FALSE);
		return FALSE;
		}
#endif

	/* --- Initialize the error message timeout value --- */

	hhSess->nTimeout = 0;

	/* --  Start the engine  --- */

	if (hwnd && hhSess->hCLoop)
		CLoopActivate(hhSess->hCLoop);

	// Set default sound setting...
	//
	hhSess->fSound = FALSE;

	// Set default exit setting...
	//
	hhSess->fExit = FALSE;

	// Store some default values in the rcSess...

	hhSess->rcSess.top = hhSess->rcSess.bottom = 0;
	hhSess->rcSess.right = hhSess->rcSess.left = 0;

	// Load program icon as session icon, load routine can overwrite this
	// to user defined icon.
	//
	sessInitializeIcons((HSESSION)hhSess);

	/* --- Process the command line stuff, if any --- */

	// Is this a new connection... i.e., hasn't been saved before.
	//
	hhSess->fIsNewSession = FALSE;

	if (hwnd)
		{
		// if (StrCharGetStrLength(hhSess->achSessCmdLn) > 0)
		if (sessCheckAndLoadCmdLn(hSession) == 0)
			{
			if (sessLoadSessionStuff(hSession) == FALSE)
                {
                LoadString(glblQueryDllHinst(), IDS_ER_BAD_SESSION,
                    achFormat, sizeof(achFormat)/sizeof(TCHAR));

                // mrw:10/7/96
                //
			    wsprintf(ach, achFormat, "");   // get rid of %s

                LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN,
                    achTitle, sizeof(achTitle)/sizeof(TCHAR));

                TimedMessageBox(hwnd, ach, achTitle, 
                    MB_OK | MB_ICONEXCLAMATION, hhSess->nTimeout);

                if (ReinitializeSessionHandle(hSession, TRUE) == FALSE)
                    {
                    LoadString(glblQueryDllHinst(), IDS_ER_REINIT,
                        ach, sizeof(ach)/sizeof(TCHAR));

                    LoadString(glblQueryDllHinst(), IDS_MB_TITLE_ERR,
                        achTitle, sizeof(achTitle)/sizeof(TCHAR));

                    TimedMessageBox(hwnd, ach, achTitle, 
                        MB_OK | MB_ICONSTOP, hhSess->nTimeout);

                    PostQuitMessage(1);
                    return FALSE;
                    }
				}

			emuHomeHostCursor(hhSess->hEmu);
			emuEraseTerminalScreen(hhSess->hEmu);
			}
		}

	if (hwnd)
		{
		if (hhSess->achSessName[0] == TEXT('\0'))
			{
			ach[0] = TEXT('\0');

  			LoadString(glblQueryDllHinst(), IDS_GNRL_NEW_CNCT, ach,
				sizeof(ach) / sizeof(TCHAR));

			StrCharCopy(hhSess->achSessName, ach);
			StrCharCopy(hhSess->achOldSessName, ach);
			hhSess->fIsNewSession = TRUE;
			}

		sessUpdateAppTitle(hSession);
		PostMessage(hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)hhSess->hIcon);
		}

	/* --- Force status line to update --- */

	if (hwnd)
		SendMessage(hhSess->hwndStatusbar, SBR_NTFY_INITIALIZE, 0, 0);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ReinitializeSessionHandle
 *
 * DESCRIPTION:
 *	Calls a bunch of functions to set the session handle back to a known,
 *	blank state, without having to destroy it.
 *
 * ARGUMENTS:
 *	hSession	- external session handle.
 *  fUpdateTitle- reset the app window title if this is TRUE
 *
 * RETURNS:
 */
BOOL ReinitializeSessionHandle(const HSESSION hSession, const int fUpdateTitle)
	{
    int iRet = 0;

	const HHSESSION hhSess = VerifySessionHandle(hSession);

	/* --- Reinitialize the X(trans)fer handle --- */
	if (InitializeXferHdl(hSession,
							sessQueryXferHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Reinitialize the Files and Directorys handle --- */
	if (InitializeFilesDirsHdl(hSession,
								sessQueryFilesDirsHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Reinitialize the Capture File handle --- */
	if (InitializeCaptureFileHandle(hSession,
							   sessQueryCaptureFileHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Init the connection handle --- */
    // NOTE: cnctInit() will return -4 if no modem has ever been installed
    // (lineInitialize() returns LINEERR_OPERATIONUNAVAIL) rev:08/05/99.
    //
    iRet = cnctInit(sessQueryCnctHdl(hSession));
	if (iRet != 0 && iRet != -4)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Init the com handle	--- */
	if (ComInitHdl(sessQueryComHdl(hSession)) != COM_OK)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Create a session data file handle --- */
	sfReleaseSessionFile(hhSess->hSysFile);
	hhSess->hSysFile = CreateSysFileHdl();

	if (hhSess->hSysFile == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Init the cloop handle --- */
	if (CLoopInitHdl(sessQueryCLoopHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	/* --- Reinitialize the Emulator handle --- */
	if (emuInitializeHdl(sessQueryEmuHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

	// Home the cursor (different than doing set_curpos(0,0) and
	// erase the terminal screen.
	//
	emuHomeHostCursor(hhSess->hEmu);
	emuEraseTerminalScreen(hhSess->hEmu);

	/* --- Reinitialize the Print handle --- */
	if (printInitializeHdl(sessQueryPrintHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}

#if	defined(CHARACTER_TRANSLATION)
	if (InitTranslateHandle(sessQueryTranslateHdl(hSession)) != 0)
		{
		assert(FALSE);
		return FALSE;
		}
#endif

	/* --- Re-Create a Backscroll handle --- */

	// No backscrlInitialize() was written so for now do this...
	//
	backscrlFlush(hhSess->hBackscrl);

	/* --- Initialize the error message timeout value --- */

	// hhSess->nTimeout = 30;		// initialize to 30 seconds
	hhSess->nTimeout = 0;			// disable in Lower Wacker


	// Set default sound setting...
	//
	hhSess->fSound	  = FALSE;

	// Set default exit setting...
	//
	hhSess->fExit	  = FALSE;

	// Load program icon as session icon, load routine can overwrite this
	// to user defined icon.
	//
	sessInitializeIcons((HSESSION)hhSess);

	// Zap the command line
	//
	TCHAR_Fill(hhSess->achSessCmdLn,
				TEXT('\0'),
				sizeof(hhSess->achSessCmdLn) / sizeof(TCHAR));

	// Make this a new connection
	//
	hhSess->fIsNewSession = TRUE;

	TCHAR_Fill(hhSess->achSessName,
				TEXT('\0'),
				sizeof(hhSess->achSessName) / sizeof(TCHAR));
	TCHAR_Fill(hhSess->achOldSessName,
				TEXT('\0'),
				sizeof(hhSess->achOldSessName) / sizeof(TCHAR));

	LoadString(glblQueryDllHinst(),
				IDS_GNRL_NEW_CNCT,
				hhSess->achSessName,
				sizeof(hhSess->achSessName) / sizeof(TCHAR));

	StrCharCopy(hhSess->achOldSessName, hhSess->achSessName);

	// Update the title - mrw:6/16/95
	//
    if (fUpdateTitle)
	    sessUpdateAppTitle(hSession);

	/* --- Force status line to update --- */

	PostMessage(hhSess->hwndStatusbar, SBR_NTFY_REFRESH,
		(WPARAM)SBR_MAX_PARTS, 0);

	// Refresh the terminal window - necessary - mrw:6/16/95
	//
	SendMessage(hhSess->hwndTerm, WM_SIZE, 0, 0);
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DestroySessionHandle
 *
 * DESCRIPTION:
 *	Destroys the session handle created by CreateSessionHandle.
 *
 * ARGUMENTS:
 *	hSession	- external session handle.
 *
 * RETURNS:
 *	void
 *
 */
void DestroySessionHandle(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	if (hhSess == 0)
		return;

	if (hhSess->hCLoop)
		CLoopDestroyHandle(&hhSess->hCLoop);

	if (hhSess->hUpdate)
		{
		updateDestroy(hhSess->hUpdate);
		hhSess->hUpdate = NULL; // REV 8/27/98
		}

	if (hhSess->hEmu)
		{
		emuDestroyHdl(hhSess->hEmu);
		hhSess->hEmu = NULL;
		}

	if (hhSess->hBackscrl)
		{
		backscrlDestroy(hhSess->hBackscrl);
		hhSess->hBackscrl = NULL;
		}

	if (hhSess->hXferHdl)
		{
		DestroyXferHdl((HXFER)hhSess->hXferHdl);
		hhSess->hXferHdl = NULL; // REV 8/27/98
		}

	if (hhSess->hFilesHdl)
		{
		DestroyFilesDirsHdl(sessQueryFilesDirsHdl(hSession));
		hhSess->hFilesHdl = NULL; // REV 8/27/98
		}

	if (hhSess->hSysFile)
		{
		sfCloseSessionFile(hhSess->hSysFile);
		hhSess->hSysFile = 0;
		}

	if (hhSess->hCnct)
		{
		cnctDestroyHdl(hhSess->hCnct);
		hhSess->hCnct = NULL;
		}

	// ComDestroy must follow cnctDestroy since cnctDestroy does
	// a port deactivate. - mrw
	//
	if (hhSess->hCom)
		ComDestroyHandle(&hhSess->hCom);

	if (hhSess->hCaptFile)
		{
		DestroyCaptureFileHandle(hhSess->hCaptFile);
		hhSess->hCaptFile = NULL;
		}

	if (hhSess->hPrint)
		{
		printDestroyHdl(hhSess->hPrint);
		hhSess->hPrint = NULL;
		}

#if	defined(CHARACTER_TRANSLATION)
	if (hhSess->hTranslate)
		{
		DestroyTranslateHandle(hhSess->hTranslate);
		hhSess->hTranslate = NULL;
		}
#endif

	DeleteCriticalSection(&hhSess->csSess);
	free(hhSess);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VerifySessionHandle
 *
 * DESCRIPTION:
 *	Every session function calls here to verify and get the internal handle.
 *	Saves having to type this chunk of code and takes less space than
 *	a macro.  We may want to add further checks to verify the handle.
 *
 *
 * ARGUMENTS:
 *	hSession		- external session handle
 *	fSynchronize	- if TRUE, we wait for mutex
 *
 * RETURNS:
 *	Internal session handle or zero.
 *
 */
HHSESSION VerifySessionHandle(const HSESSION hSession)
	{
	const HHSESSION hhSess = (HHSESSION)hSession;

	if (hSession == 0)
		{
		assert(FALSE);
		ExitProcess(1);
		}

	/* Above mentioned further checks, added by DLW */
	assert(hhSess->lPrefix == PRE_MAGIC);
	assert(hhSess->lPostfix == POST_MAGIC);

	return hhSess;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	hLock
 *
 * DESCRIPTION:
 *	Use the function to get ownership of the mutex semaphore for
 *	synchronized access.
 *
 * ARGUMENTS:
 *	hhSess	- internal session handle.
 *
 * RETURNS:
 *	void
 *
 */
void hLock(const HHSESSION hhSess)
	{
	if (hhSess == 0)
		{
		assert(FALSE);
		ExitProcess(1);
		}

	EnterCriticalSection(&hhSess->csSess);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	hUnlock
 *
 * DESCRIPTION:
 *	Releases the mutex semaphore
 *
 * ARGUMENTS:
 *	hhSess	- internal session handle
 *
 * RETURNS:
 *	void
 *
 */
void hUnlock(const HHSESSION hhSess)
	{
	if (hhSess == 0)
		{
		assert(FALSE);
		ExitProcess(1);
		}

	LeaveCriticalSection(&hhSess->csSess);
	return;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HWND sessQueryHwnd(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

    return hhSess->hwndSess;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HWND sessQueryHwndStatusbar(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hwndStatusbar;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HWND sessQueryHwndToolbar(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hwndToolbar;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HWND sessQueryHwndTerminal(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hwndTerm;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HUPDATE sessQueryUpdateHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hUpdate;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HTIMERMUX sessQueryTimerMux(const HSESSION hSession)
	{
	HTIMERMUX hTimerMux;
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	hTimerMux = hhSess->hTimerMux;
	hUnlock(hhSess);

    return hTimerMux;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HCLOOP sessQueryCLoopHdl(const HSESSION hSession)
	{
	HCLOOP hCLoop;
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	hCLoop = hhSess->hCLoop;
	hUnlock(hhSess);

 	return hCLoop;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HCOM sessQueryComHdl(const HSESSION hSession)
	{
	HCOM hCom;
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	hCom = hhSess->hCom;
	hUnlock(hhSess);
    return hCom;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HEMU sessQueryEmuHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	return hhSess->hEmu;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HPRINT sessQueryPrintHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hPrint;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessSetSysFileHdl(const HSESSION hSession, const SF_HANDLE hSF)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	hhSess->hSysFile = hSF;
	hUnlock(hhSess);
    return;
	}

SF_HANDLE sessQuerySysFileHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hSysFile;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HBACKSCRL sessQueryBackscrlHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hBackscrl;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HXFER sessQueryXferHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hXferHdl;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HFILES sessQueryFilesDirsHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hFilesHdl;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HCAPTUREFILE sessQueryCaptureFileHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hCaptFile;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessQueryCmdLn(const HSESSION hSession, LPTSTR pach, const int len)
	{
	int i;
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	TCHAR *pachCmdLn = hhSess->achSessCmdLn;

	for (i = 0 ; i < len ; ++i)
		{
		if (*pachCmdLn == (TCHAR)0)
			break;

		// *pach++ = *pachCmdLn++;
		if (IsDBCSLeadByte(*pachCmdLn))
			{
			*(WORD *)pach = *(WORD *)pachCmdLn;
			}
		else
			{
			*pach = *pachCmdLn;
			}
		pach = StrCharNext(pach);
		pachCmdLn = StrCharNext(pachCmdLn);
		}

	return;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int sessQueryTimeout(const HSESSION hSession)
	{
	int nTimeout;
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	nTimeout = hhSess->nTimeout;
	hUnlock(hhSess);

    return nTimeout;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessSetTimeout(const HSESSION hSession, int nTimeout)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	hLock(hhSess);
	hhSess->nTimeout = nTimeout;
	hUnlock(hhSess);
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

HCNCT sessQueryCnctHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hCnct;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#if defined(INCL_WINSOCK)
int sessQueryTelnetPort(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->iTelnetPort;
	}
#endif


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessQueryOldName(const HSESSION hSession, const LPTSTR pach, unsigned uSize)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	if (pach == 0)
		return;

	pach[0] = TEXT('\0');

	/* --- uSize is the number of BYTES in the buffer! ---- */

	uSize = min(uSize, sizeof(hhSess->achOldSessName));
    if (uSize)
        MemCopy(pach, hhSess->achOldSessName, uSize);
	pach[uSize-1] = TEXT('\0');
	return;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessSetIconID(const HSESSION hSession, const int nID)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	if (hhSess->nIconId != nID)
		{
		hhSess->nIconId = nID;
		hhSess->hIcon = extLoadIcon(MAKEINTRESOURCE(nID));
		//hhSess->hIcon = LoadIcon(glblQueryDllHinst(), MAKEINTRESOURCE(nID));
		//hhSess->hLittleIcon = LoadIcon(glblQueryDllHinst(),
		//						  MAKEINTRESOURCE(nID + IDI_PROG_ICON_CNT));
		}
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int sessQueryIconID(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->nIconId;
	}

HICON sessQueryIcon(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hIcon;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void sessSetName(const HSESSION hSession, const LPTSTR pach)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	/* This is here to catch an overrun I can't reproduce. DLW */
#if !defined(NDEBUG)
	if (StrCharGetStrLength(pach) > 255)
		assert(FALSE);
#endif
	StrCharCopy(hhSess->achSessName, pach);
	return;
	}


void sessQueryName(const HSESSION hSession, const LPTSTR pach, unsigned uSize)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	if (pach == 0 || uSize == 0)
		return;

	pach[0] = TEXT('\0');

	/* --- uSize is the number of BYTES in the buffer! ---- */

	uSize = min(uSize, sizeof(hhSess->achSessName));
    if (uSize)
        MemCopy(pach, hhSess->achSessName, uSize);
	pach[uSize-1] = TEXT('\0');
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

HTRANSLATE sessQueryTranslateHdl(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hTranslate;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessQuerySound
 *
 * DESCRIPTION:
 *	Return the sound setting for the session.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *
 * RETURNS:
 *	fSound - the sound setting.
 */
int sessQuerySound(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return ((int)hhSess->fSound);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessSetSound
 *
 * DESCRIPTION:
 *	Set the sound setting for the session.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *
 * RETURNS:
 */
void sessSetSound(const HSESSION hSession, int fSound)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	hhSess->fSound = fSound;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessQueryExit
 *
 * DESCRIPTION:
 *	Return the exit setting for the session.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *
 * RETURNS:
 *	fExit - the exit setting.
 */
int sessQueryExit(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return ((int)hhSess->fExit);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessSetExit
 *
 * DESCRIPTION:
 *	Set the exit setting for the session.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *
 * RETURNS:
 */
void sessSetExit(const HSESSION hSession, int fExit)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	hhSess->fExit = fExit;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessSetIsNewSession
 *
 * DESCRIPTION:
 *  Set the fIsNewSession flag.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *  fIsNewSession - set appropriate session structure item to this value.
 *
 * RETURNS:
 */
void sessSetIsNewSession(const HSESSION hSession, int fIsNewSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	hhSess->fIsNewSession = fIsNewSession;
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessQueryIsNewSession
 *
 * DESCRIPTION:
 *  Query the setting of fIsNewSession flag.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *
 * RETURNS:
 */
int sessQueryIsNewSession(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return ((int)hhSess->fIsNewSession);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessIsSessNameDefault
 *
 * DESCRIPTION:
 *  Checks to see if the session name is still the default session name
 *  or has the user provided us with a custom session name.
 *
 * ARGUMENTS:
 *  pacName - session file name.
 *
 * RETURNS:
 *
 */
BOOL sessIsSessNameDefault(LPTSTR pacName)
	{
	TCHAR ach[FNAME_LEN];

	if (pacName[0] == TEXT('\0'))
		return TRUE;

	TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
  	LoadString(glblQueryDllHinst(), IDS_GNRL_NEW_CNCT, ach,
		sizeof(ach) / sizeof(TCHAR));

	if (StrCharCmp(ach, pacName) == 0)
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessQueryWindowRect
 *
 * DESCRIPTION:
 *  Query the setting of the session window RECT.
 *
 * ARGUMENTS:
 *	hSession 	- the session handle.
 *  prc         - pointer to RECT.
 *
 * RETURNS:
 *	void.
 */
void sessQueryWindowRect(const HSESSION hSession, RECT *prc)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    *prc = hhSess->rcSess; // mrw:3/10/95

    return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  sessQueryWindowShowCmd
 *
 * DESCRIPTION:
 *  Query the setting of the session window show state.
 *
 * ARGUMENTS:
 *	hSession 	- the session handle.
 *
 * RETURNS:
 *	void.
 */
int sessQueryWindowShowCmd(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return ((int)hhSess->iShowCmd);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessQuerySidebarHwnd
 *
 * DESCRIPTION:
 *	Returns the sidebar window handle
 *
 * ARGUMENTS:
 *	hSession	- public session handle.
 *
 * RETURNS:
 *	Sidebar window handle.
 *
 * AUTHOR: Mike Ward, 10-Mar-1995
 */
HWND sessQuerySidebarHwnd(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
    return hhSess->hwndSidebar;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
