/*	File: D:\WACKER\tdll\autosave.c (Created: 19-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 6 $
 *	$Date: 8/18/99 10:52a $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\features.h>

#include "stdtyp.h"
#include "sf.h"
#include "mc.h"
#include "term.h"
#include "assert.h"
#include "globals.h"
#include "sess_ids.h"
#include "load_res.h"
#include "open_msc.h"
#include "file_msc.h"
#include <term\res.h>
#include "session.h"
#include "session.hh"
#include "errorbox.h"
#include "tdll.h"
#include "tchar.h"
#include "misc.h"
#include <emu\emu.h>


//
// Static function prototypes...
//
STATIC_FUNC BOOL asCreateFullFileName(const HSESSION hSession,
									  const int iSize,
									  LPTSTR acFile,
									  BOOL fExplicit);
STATIC_FUNC int  asOverwriteExistingFile(HWND hwnd, LPTSTR pacName);
STATIC_FUNC void asBadSessionFileMsg(HSESSION hSession, LPTSTR pacName);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SilentSaveSession
 *
 * DESCRIPTION:
 *	This function is called whenever the user selects SAVE from the menus.
 *	We will prompt to overwrite if a file with same name already exists.
 *  If the file doen't exist, save it in the defult directory, with the file
 *  name corresponding to the session name the user gave us.
 *	NOTE: This function can also be called from SaveSession to do the
 *	saving work.
 *
 * ARGUEMENTS:
 *	hSession 	-- the session handle
 *	hwnd     	-- handle of parent window
 *	fExplicit 	-- TRUE if the user selected "Save", false otherwise.
 *
 * RETURNS:
 *	Nothing.
 *
 */
void SilentSaveSession(const HSESSION hSession, HWND hwnd, BOOL fExplicit)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	TCHAR	acName[FNAME_LEN],
			acOldFile[FNAME_LEN];

	BOOL	fNameChanged = FALSE;

	// We don't want to silent save the default session.  This can happen
	// wen the user cancels the first New Connection Dialog, then quits
	// the program.  jcm 2-22-95
	//
	if (fExplicit == 0)
		{
		acName[0] = TEXT('\0');
		sessQueryName(hSession, acName, sizeof(acName) / sizeof(TCHAR));
		if (sessIsSessNameDefault(acName))
			return;
		}

	acName[0] = TEXT('\0');

	sfGetSessionFileName(hhSess->hSysFile,
							sizeof(acName) / sizeof(TCHAR), acName);

	acOldFile[0] = TEXT('\0');

	if (StrCharCmp(hhSess->achOldSessName, hhSess->achSessName) != 0)
		{
		StrCharCopy(acOldFile, acName);
		fNameChanged = TRUE;
		}

	// This file hasn't been saved yet or the user has changed the session
	// name, in either case we will need to come up with the new fully
	// qualified file name.
	//
	if (acName[0] == TEXT('\0') || fNameChanged)
		{
		if (!asCreateFullFileName(hSession,
				sizeof(acName) / sizeof(TCHAR), acName, fExplicit))
			{
			// If asCreateFullFileName failed, it is because the fully qualified
			// file name is too long.  In this case, call save as, and let the
			// common dialog in that routine restrict the users name length.
			//
			SaveAsSession(hSession, hwnd);
			return;
			}

		sfReleaseSessionFile(hhSess->hSysFile);
		hhSess->hSysFile = CreateSysFileHdl();
		assert(hhSess->hSysFile);
		sfOpenSessionFile(hhSess->hSysFile, acName);

		if (!asOverwriteExistingFile(hwnd, acName))
			{
			SaveAsSession(hSession, hwnd);
		    return;
	   		}
		}

	// Now we have the user's OK and a correct path/file name so
	// let's save it.
	//
	sessSaveSessionStuff(hSession);					// Commit changes if any
	sfFlushSessionFile(hhSess->hSysFile);			// Write info to disk

	// Make sure the name is shadowed, before this was handled by re-reading
	// the session file back in, now we don't do that.
	//
 	StrCharCopy(hhSess->achOldSessName, hhSess->achSessName);

	if (hhSess->fIsNewSession)
		hhSess->fIsNewSession = 0;
	else
		{
		if (acOldFile[0] != TEXT('\0') && fNameChanged)
			{
			// If the user changed the file name for an existing session
			// now is the time to delete the old one... since we have already
			// saved the new one.
			//
			DeleteFile(acOldFile);
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SaveSession
 *
 * DESCRIPTION:
 *	This function is called whenever the user selects "New Connection", "Open",
 *	"Exit" from the menus or closed the app.  If this session has never been
 *	saved before then we will warn the user and prompt them to save.  Otherwise
 *	save the current settings silently.
 *
 * ARGUEMENTS:
 *	hSession -- the session handle
 *	hwnd     -- handle of parent window
 *
 * RETURNS:
 *	TRUE  - if all went ok with saving or the user didn't want to save.
 *	FALSE - if the user canceled the operation that brought this to action.
 *
 */
BOOL SaveSession(const HSESSION hSession, HWND hwnd)
	{
	TCHAR	achText[256],
			achSessName[256],
			ach[512];

	int 	nRet;

	if (sessQueryIsNewSession(hSession) == TRUE)
		{
		TCHAR_Fill(achSessName, TEXT('\0'), sizeof(achSessName) / sizeof(TCHAR));
		TCHAR_Fill(achText, TEXT('\0'), sizeof(achText) / sizeof(TCHAR));

		LoadString(glblQueryDllHinst(), IDS_GNRL_CNFRM_SAVE, achText,
			sizeof(achText) / sizeof(TCHAR));

		sessQueryName(hSession, achSessName, sizeof(achSessName));
		if (sessIsSessNameDefault(achSessName))
			{
			// For now ignore sessions with a default name...
			//
			return TRUE;
			}

		wsprintf(ach, achText, achSessName);

		// Warn the user that they haven't saved the session...
		//
		LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, achText,
			sizeof(achText) / sizeof(TCHAR));

		if ((nRet = TimedMessageBox(hwnd, ach, achText,
			MB_YESNOCANCEL | MB_ICONQUESTION, 0)) != IDYES)
			return (nRet == IDNO);
		}

	SilentSaveSession(hSession, hwnd, FALSE);
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	OpenSession
 *
 * DESCRIPTION:
 *	This function will open a session file.
 *  If there is an opened, unsaved, new session then the user will be prompted
 *  to save it, otherwise the opened session will be saved silently only after
 *  the user has pressed the OK button.
 *  
 * ARGUEMENTS:
 *	hSession -- the session handle
 *	hwnd     -- handle of parent window
 *
 * RETURNS:
 *	Nothing.
 *
 */
int OpenSession(const HSESSION hSession, HWND hwnd)
	{
    int iRet = 0;
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	LPTSTR 			pszStr;
	TCHAR 			acMask[64], acTitle[64];
	TCHAR 			acDir[FNAME_LEN], acName[FNAME_LEN];


	TCHAR_Fill(acMask,  TEXT('\0'), sizeof(acMask)  / sizeof(TCHAR));
	TCHAR_Fill(acName,  TEXT('\0'), sizeof(acName)  / sizeof(TCHAR));
	TCHAR_Fill(acTitle, TEXT('\0'), sizeof(acTitle) / sizeof(TCHAR));

	resLoadFileMask(glblQueryDllHinst(), IDS_CMM_HAS_FILES1, 2, acMask,
		sizeof(acMask) / sizeof(TCHAR));

	LoadString(glblQueryDllHinst(), IDS_CMM_LOAD_SESS, acTitle,
		sizeof(acTitle) / sizeof(TCHAR));

#ifdef NT_EDITION
	//mpt:07-30-97 
	if ( IsNT() )
#endif
		GetUserDirectory(acDir, sizeof(acDir) / sizeof(TCHAR));
#ifdef NT_EDITION
	else
		{
		GetModuleFileName(glblQueryHinst(), acDir, sizeof(acDir));
		mscStripName(acDir);
		}
#endif

	pszStr = gnrcFindFileDialog(hwnd, acTitle, acDir, acMask);

	if (pszStr)
		{
		// If a session is opened then prompt to save it now or just 
		// save silently in SaveSession()
		//
		if (SaveSession(hSession, hwnd))
			{
			if (ReinitializeSessionHandle(hSession, TRUE) == FALSE)
				{
				assert(0);
				free(pszStr);
				pszStr = NULL;
				return -1;
				}

			StrCharCopy(acName, pszStr);
			free(pszStr);
			pszStr = NULL;
			}
		else
			{
			free(pszStr);
			pszStr = NULL;
		    return -2;
			}
		}

	else
		{
		return -4; // mrw:4/21/95
		}

	if (StrCharGetByteCount(acName) > 0)
		{
		if (fTestOpenOldTrmFile(hhSess, acName) != 0)
			{
			if (sfOpenSessionFile(hhSess->hSysFile, acName) < SF_OK)
				{
				asBadSessionFileMsg(hSession, acName);
				return -3;
				}

			// If there was a command line we should get rid of it.
			//
			TCHAR_Fill(hhSess->achSessCmdLn,
				TEXT('\0'),
				sizeof(hhSess->achSessCmdLn) / sizeof(TCHAR));

			if (sessLoadSessionStuff(hSession) == FALSE)
                iRet = -4;

			emuHomeHostCursor(hhSess->hEmu);
			emuEraseTerminalScreen(hhSess->hEmu);
			hhSess->fIsNewSession = FALSE;
			}		

		sessUpdateAppTitle(hSession);

		PostMessage(hhSess->hwndSess, WM_SETICON, (WPARAM)TRUE,
			(LPARAM)hhSess->hIcon);
		}

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SaveAsSession
 *
 * DESCRIPTION:
 *	This function is called whenever the user selects SAVEAS from the menus.
 *
 * ARGUEMENTS:
 *	hSession -- the session handle
 *	hwnd     -- handle of parent window
 *
 * RETURNS:
 *	Nothing.
 *
 */
void SaveAsSession(const HSESSION hSession, HWND hwnd)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	long 			lValue = -1;
	unsigned long   lSize = 0;
	LPTSTR 			pszStr;
	TCHAR 			acMask[64];
	TCHAR 			acTitle[64];
	TCHAR			acFileName[512];
	TCHAR           acDir[FNAME_LEN];
	TCHAR           ach[256];

	resLoadFileMask(glblQueryDllHinst(), IDS_CMM_HAS_FILES1, 1,	acMask,
		sizeof(acMask) / sizeof(TCHAR));

	LoadString(glblQueryDllHinst(),  IDS_CMM_SAVE_AS, acTitle,
	 	sizeof(acTitle) / sizeof(TCHAR));

	if (sfGetSessionFileName(sessQuerySysFileHdl(hSession),
			sizeof(acFileName) / sizeof(TCHAR), acFileName) != SF_OK ||
			sfGetSessionFileName(sessQuerySysFileHdl(hSession),
			sizeof(acDir) / sizeof(TCHAR),	acDir) != SF_OK)
		{
		acFileName[0] = TEXT('\0');

		if (asCreateFullFileName(hSession,
								sizeof(acFileName) / sizeof(TCHAR),
								acFileName,
								TRUE) == TRUE)
			{
			StrCharCopy(acDir, acFileName);
			mscStripName(acDir);
			}
		else
			{
			//Changed from current directory to user directory - mpt 8-18-99
			if ( !GetUserDirectory(acDir, sizeof(acDir) / sizeof(TCHAR)) )
				{
				GetCurrentDirectory(sizeof(acDir) / sizeof(TCHAR), acDir);
				}
			sessQueryName(hSession, acFileName, sizeof(acFileName) / sizeof(TCHAR));
			}
		}
    else
        {
        mscStripName(acDir);
        }

	pszStr = StrCharLast(acDir);

	// Remove trailing backslash from the directory name if there is one.
	//
	if (pszStr && *pszStr == TEXT('\\'))
		*pszStr = TEXT('\0');

	pszStr = gnrcSaveFileDialog(hwnd, acTitle, acDir, acMask, acFileName);

	if (pszStr)
		{
		sfReleaseSessionFile(hhSess->hSysFile);
		hhSess->hSysFile = CreateSysFileHdl();
		if (!hhSess)
			{
			assert(hhSess->hSysFile);
			free(pszStr);
			pszStr = NULL;
			return;
			}
		sfOpenSessionFile(hhSess->hSysFile, pszStr);

		// In the "SaveAs" operation we take the file name and make it the
		// session name.
		//
		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		StrCharCopy(ach, pszStr);
		StrCharCopy(hhSess->achSessName, mscStripExt(mscStripPath(ach)));
		StrCharCopy(hhSess->achOldSessName, hhSess->achSessName);

		/* A "SaveAs" operation requires that the file actually be saved.
		 * To do this, we fiddle with a special reserve item in the file. DLW
		 */
		sfGetSessionItem(hhSess->hSysFile, SFID_INTERNAL_TAG, &lSize, &lValue);
		if (lValue == (-1))
			{
			lValue = 0x12345678;
			/* We only write it if we don't have it */
			sfPutSessionItem(hhSess->hSysFile, SFID_INTERNAL_TAG,
				sizeof(long), &lValue);
			}

		// Save the new session file...
		//
 		sessSaveSessionStuff(hSession);					// Commit changes if any
		sfFlushSessionFile(hhSess->hSysFile);			// Write info to disk
		sessUpdateAppTitle(hSession);					// Show name in title

		// Since we just saved it's not a new session anymore...
		//
		hhSess->fIsNewSession = FALSE;
		free(pszStr);
		pszStr = NULL;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  asCreateFullFileName
 *
 * DESCRIPTION:
 *  This function will either make up a fully qualified session name using
 *  the current directory and the session name for NEW sessions, or change
 *  the existing file name to the user given file name.
 *
 * ARGUMENTS:
 *	hSession	- the session handle.
 *	iSize		- the size of the following buffer.
 *  acFile   	- the old file path and name.
 *  fExplicit	- TRUE if the user selected "Save" from the menus.
 *
 * RETURNS:
 *  void
 *
 */
STATIC_FUNC BOOL asCreateFullFileName(const HSESSION hSession,
											const int iSize,
											LPTSTR acFile,
											BOOL fExplicit)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	TCHAR			acSessName[FNAME_LEN],
					acDir[_MAX_PATH],
					acExt[FNAME_LEN];

	LPTSTR			pszStr;

	int 			iDirLen,
					iNameLen,
					iExtLen;

    acDir[0] = TEXT('\0');

	if (acFile[0] == TEXT('\0'))
		{
		// This is a brand new session.
		//

#ifdef NT_EDITION
		// mpt:07-30-97
		if ( IsNT() )
#endif
			GetUserDirectory(acDir, sizeof(acDir) / sizeof(TCHAR));
#ifdef NT_EDITION	
		else
			{
			GetModuleFileName(glblQueryHinst(), acDir, sizeof(acDir));
			mscStripName(acDir);
			}
#endif
		}
	else
		{
		// The user has changed the session name.
		//
		StrCharCopy(acDir, acFile);
		mscStripName(acDir);
		}

	// See if we need to append a trailing backslash to the
	// directory name.
	//
	pszStr = StrCharLast(acDir);

	if (pszStr && *pszStr != TEXT('\\'))
		{
		StrCharCat(acDir, TEXT("\\"));
		}

	// Save the length of the path information.  We'll use this below.
	//
	iDirLen = StrCharGetByteCount(acDir);

	// Get the session name given by the user...
	//
	acSessName[0] = TEXT('\0');

	sessQueryName(hSession, acSessName, sizeof(acSessName));

	if (sessIsSessNameDefault(acSessName))
		{
		// We ignore a session with a default name if the user hasn't
		// explicitly selected "Save" off of the menus.
		//
		if (!fExplicit)
			return FALSE;

		// This seems little odd from the user's perspective, maybe a whole
		// new dialog would make more sense?
		//
		if (DialogBoxParam(glblQueryDllHinst(),
						   MAKEINTRESOURCE(IDD_NEWCONNECTION),
						   sessQueryHwnd(hSession),
						   NewConnectionDlg, (LPARAM)hSession) == FALSE)
			{
			return FALSE;
			}

		sessQueryName(hSession, acSessName, sizeof(acSessName));
		}

	iNameLen = StrCharGetByteCount(acSessName);

	// Get the extension we are using from the resource file.
	//
	acExt[0] = TEXT('\0');

	LoadString(glblQueryDllHinst(), IDS_GNRL_HAS, acExt, sizeof(acExt) / sizeof(TCHAR));

	iExtLen =  StrCharGetByteCount(acExt);

	// We're about to put a fully qualified file name together.  Let's make
	// sure that the combined component length is valid.
	//
	if ( (iDirLen + iNameLen + iExtLen) > 254)
		{
		return(FALSE);
		}

	if ( (iDirLen + iNameLen + iExtLen) > iSize)
		{
		assert(FALSE);
		return(FALSE);
		}

	// Put the pieces together, now that we know it's going to work.
	//
	StrCharCopy(acFile, acDir);
	StrCharCat(acFile, acSessName);
	StrCharCat(acFile, acExt);

	sfSetSessionFileName(hhSess->hSysFile, acFile);

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  asOverwriteExistingFile
 *
 * DESCRIPTION:
 *	If a file exists prompt the user to overwrite the file.
 *
 * ARGUMENTS:
 *  pacName  - the session file name.
 *
 * RETURNS:
 *  TRUE 	- if file doesn't exist or it's ok to overwrite.
 *	FALSE 	- if the user doesn't want to overwrite.
 *
 */
STATIC_FUNC int asOverwriteExistingFile(HWND hwnd, LPTSTR pacName)
	{
	TCHAR 	ach[256], achTitle[256], achText[256];
	int 	nRet = 0;

	if (GetFileSizeFromName(pacName, NULL))
		{
		// Warn the user that a file with that name already exists...
		//
		LoadString(glblQueryDllHinst(), IDS_GNRL_CNFRM_OVER, achText,
			sizeof(achText) / sizeof(TCHAR));
		wsprintf(ach, achText, pacName);

		LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, achTitle,
			sizeof(achTitle) / sizeof(TCHAR));

		nRet = TimedMessageBox(hwnd, ach, achTitle,
			MB_YESNO | MB_ICONEXCLAMATION, 0);

		return (nRet == IDYES);
		}

	return 1;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  asBadSessionFileMsg
 *
 * DESCRIPTION:
 *  Display a message if a bad session file is found.
 *
 * ARGUMENTS:
 *	hSession - the session handle.
 *  pacName  - the session file name.
 *
 * RETURNS:
 *  TRUE 	- if file doesn't exist or it's ok to overwrite.
 *	FALSE 	- if the user doesn't want to overwrite.
 *
 */
STATIC_FUNC void asBadSessionFileMsg(HSESSION hSession, LPTSTR pacName)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	TCHAR acFormat[64], acTitle[256], ach[256];

	TCHAR_Fill(acFormat, TEXT('\0'), sizeof(acFormat) / sizeof(TCHAR));
	TCHAR_Fill(acTitle, TEXT('\0'), sizeof(acTitle) / sizeof(TCHAR));

	LoadString(glblQueryDllHinst(), IDS_CMM_LOAD_SESS, acTitle,
		sizeof(acTitle) / sizeof(TCHAR));
	LoadString(glblQueryDllHinst(),	IDS_ER_BAD_SESSION, acFormat,
		sizeof(acFormat) / sizeof(TCHAR));
	wsprintf(ach, acFormat, pacName);

	TimedMessageBox(sessQueryHwnd(hSession), ach, acTitle,
		MB_OK | MB_ICONEXCLAMATION, sessQueryTimeout(hSession));

	sfSetSessionFileName(hhSess->hSysFile, TEXT(""));
	}
