/*	File: D:\WACKER\tdll\sessfile.c (Created: 30-Apr-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
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
#include "translat.h"
#include "misc.h"
#include "keyutil.h"

STATIC_FUNC void sessSaveHdl(HSESSION hSession);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessLoadSessionStuff
 *
 * DESCRIPTION:
 *	This function gets called whenever the user wants to read in the data
 *	from a session file.  If there is a currently opened session file, it
 *	is open, otherwise we prompt for one.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	BOOL
 */
BOOL sessLoadSessionStuff(const HSESSION hSession)
	{
	BOOL			bRet = TRUE;
	BOOL			fBool = TRUE;
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	unsigned long  			lSize;

	sessLoadIcons(hSession);

	if (bRet)
		{
		if (hhSess->hXferHdl)
			bRet = (LoadXferHdl((HXFER)hhSess->hXferHdl) == 0);
		assert(bRet);
		}

	if (bRet)
		{
		if (hhSess->hFilesHdl != (HFILES)0)
			bRet = (LoadFilesDirsHdl(sessQueryFilesDirsHdl(hSession))==0);
		assert(bRet);
		}

	if (bRet)
		{
		if (hhSess->hCaptFile)
			bRet = (LoadCaptureFileHandle(hhSess->hCaptFile) == 0);
		assert(bRet);
		}

	// Moved to before emulator load.  Some emulators like the Minitel
	// load fonts if the correct one is not loaded so we need to let
	// the terminal load its fonts before loading the emulator so there
	// is no conflict. mrw,3/2/95
	//
	if (bRet)
    	{
    	if (SendMessage(hhSess->hwndTerm, WM_TERM_LOAD_SETTINGS, 0, 0))
    		{
    		assert(FALSE);
    		bRet = FALSE;
    		}
    	}

	if (bRet)
		{
		if (hhSess->hEmu)
			bRet = (emuInitializeHdl(hhSess->hEmu) == 0);
		assert(bRet);
		}

	if (bRet)
		{
		if (hhSess->hPrint)
			bRet = (printInitializeHdl(hhSess->hPrint) == 0);
		assert(bRet);
		}

#if defined(CHARACTER_TRANSLATION)
	if (bRet)
		{
		if (hhSess->hTranslate)
			bRet = (LoadTranslateHandle(hhSess->hTranslate) == 0);
		assert(bRet);
		}
#endif

	if (bRet)
		{
		if (hhSess->hCLoop)
			bRet = (CLoopLoadHdl(hhSess->hCLoop) == 0);
		assert(bRet);
		}

	if (bRet)
		{
		if (hhSess->hCom)
			bRet = (ComLoadHdl(hhSess->hCom) == 0);
		assert(bRet);
		}

	if (bRet)
		{
		if (hhSess->hCnct)
			{
			if (cnctLoad(hhSess->hCnct))
				{
				assert(FALSE);
				bRet = FALSE;
                }
			}
		}

	if (bRet)
		{
		lSize = sizeof(hhSess->fSound);

		// Initialize... i.e., sound ON.
		//
		hhSess->fSound = 1;

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_SOUND,
						 &lSize,
						 &hhSess->fSound);
		}

	if (bRet)
		{
		lSize = sizeof(hhSess->fExit);

		// Initialize... i.e., exit OFF.
		//
		hhSess->fExit = 0;

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_EXIT,
						 &lSize,
						 &hhSess->fExit);
		}

	if (bRet)
		{
		lSize = sizeof(BOOL);
		fBool = TRUE;

		sfGetSessionItem(hhSess->hSysFile,
								SFID_TLBR_VISIBLE,
								&lSize,
								&fBool);

		sessSetToolbarVisible(hSession, fBool);
		}

	if (bRet)
		{
		lSize = sizeof(BOOL);
		fBool = TRUE;

		sfGetSessionItem(hhSess->hSysFile,
								SFID_STBR_VISIBLE,
								&lSize,
								&fBool);

		sessSetStatusbarVisible(hSession, fBool);
		}

	if (bRet)
		{
		lSize = sizeof(hhSess->achSessName);

		// Initialize...
		//
		TCHAR_Fill(hhSess->achSessName, TEXT('\0'),
			sizeof(hhSess->achSessName) / sizeof(TCHAR));
		
		sfGetSessionFileName(hhSess->hSysFile, 	
			sizeof(hhSess->achSessName) / sizeof(TCHAR), hhSess->achSessName);

	    // Hold on to just the session name, no path, no extension.  It is
		// usefull to keep it around.
		//
		mscStripPath(hhSess->achSessName);
		mscStripExt(hhSess->achSessName);

//      We should never be storing this internal string in the session file!
//		- jac. 10-06-94 03:44pm
//		sfGetSessionItem(hhSess->hSysFile,
//						 SFID_SESS_NAME,
//						 &lSize,
//						 hhSess->achSessName);

		/* This next line protects against trash in the session file */
		hhSess->achSessName[sizeof(hhSess->achSessName)-1] = TEXT('\0');

		StrCharCopy(hhSess->achOldSessName, hhSess->achSessName);
		}

	if (bRet)
		{
		if (sessQueryBackscrlHdl(hSession))
			{
			backscrlRead(sessQueryBackscrlHdl(hSession));
			/* Don't check this for now */
			sessRestoreBackScroll(hSession);
			}
		}

	if (bRet)
		{
		lSize = sizeof(LONG);

		memset(&hhSess->rcSess, 0, sizeof(RECT));

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_LEFT,
						 &lSize,
						 &hhSess->rcSess.left);

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_TOP,
						 &lSize,
						 &hhSess->rcSess.top);

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_RIGHT,
						 &lSize,
						 &hhSess->rcSess.right);

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_BOTTOM,
						 &lSize,
						 &hhSess->rcSess.bottom);
		}

	if (bRet)
		{
		lSize = sizeof(UINT);

		hhSess->iShowCmd = SW_SHOWNORMAL;

		sfGetSessionItem(hhSess->hSysFile,
						 SFID_SESS_SHOWCMD,
						 &lSize,
						 &hhSess->iShowCmd);
		}

    //
    // load the key macros
    //

#ifdef INCL_KEY_MACROS
	if (bRet)
		{
        keysLoadMacroList( hSession );
        }
#endif

	// Note: if you need to do any resizing, you must POST a message
	// to do so.  The emulator may have changed size and that won't
	// reflected until it processes the pending notification - mrw
	//
	if (hhSess->achSessCmdLn[0] == TEXT('\0') && IsWindow(hhSess->hwndSess))
		{
		PostMessage(hhSess->hwndSess, WM_COMMAND,
			MAKEWPARAM(IDM_CONTEXT_SNAP, 0), 0);
		}

	return bRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSaveSessionStuff
 *
 * DESCRIPTION:
 *	This function is called to call all the functions that save things in
 *	the session file.  If you have stuff to write into the session file, it
 *	should get called from here.  This function also makes sure that the
 *	user has a chance to specify the name of the session file if there is
 *	not one currently.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	Nothing.
 */
void sessSaveSessionStuff(const HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);

	/*
	 * Put in code to make sure we have an open session file handle
	 */
	assert(hhSess->hSysFile);	// any suggestions ?

	/* This doesn't have a handle since it isn't every used but once */
	sessSaveBackScroll(hSession);

	// Call this function if you've got to save settings that are stored
	// in the session handle itself.
	//
	sessSaveHdl(hSession);

	if (hhSess->hXferHdl != (HXFER)0)
		SaveXferHdl((HXFER)hhSess->hXferHdl);

	if (hhSess->hFilesHdl != (HFILES)0)
		SaveFilesDirsHdl(sessQueryFilesDirsHdl(hSession));

	if (hhSess->hCaptFile != (HCAPTUREFILE)0)
		SaveCaptureFileHandle(hhSess->hCaptFile);

	if (hhSess->hEmu != 0)
		emuSaveHdl(hhSess->hEmu);

	if (hhSess->hPrint != 0)
		printSaveHdl(hhSess->hPrint);

#if	defined(CHARACTER_TRANSLATION)
	if (hhSess->hTranslate)
		SaveTranslateHandle(hhSess->hTranslate);
#endif

	if (hhSess->hCLoop)
		CLoopSaveHdl(hhSess->hCLoop);

	if (hhSess->hCom)
		ComSaveHdl(hhSess->hCom);

	if (hhSess->hCnct)
		cnctSave(hhSess->hCnct);

	if (hhSess->hBackscrl)
		backscrlSave(hhSess->hBackscrl);

	if (hhSess->hwndTerm)
		SendMessage(hhSess->hwndTerm, WM_TERM_SAVE_SETTINGS, 0, 0);

#ifdef INCL_KEY_MACROS
    keysSaveMacroList(hSession );
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSaveHdl
 *
 * DESCRIPTION:
 *  Save items stored in the session handle.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	BOOL
 */
STATIC_FUNC void sessSaveHdl(HSESSION hSession)
	{
	const HHSESSION hhSess = VerifySessionHandle(hSession);
	WINDOWPLACEMENT stWP;

	sessSaveIcons(hSession);

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_SOUND,
					 sizeof(BOOL),
					 &hhSess->fSound);

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_EXIT,
					 sizeof(BOOL),
					 &hhSess->fExit);

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_TLBR_VISIBLE,
					 sizeof(BOOL),
					 &hhSess->fToolbarVisible);

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_STBR_VISIBLE,
					 sizeof(BOOL),
					 &hhSess->fStatusbarVisible);

//  We should NEVER put this name into the session file!!!
//  -jac. 10-06-94 03:45pm
//	sfPutSessionItem(hhSess->hSysFile,
//				 SFID_SESS_NAME,
//				 (StrCharGetByteCount(hhSess->achSessName) + 1) * sizeof(TCHAR),
//				 hhSess->achSessName);

	memset(&stWP, 0, sizeof(WINDOWPLACEMENT));
	stWP.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hhSess->hwndSess, &stWP);

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_LEFT,
					 sizeof(LONG),
					 &(stWP.rcNormalPosition.left));

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_TOP,
					 sizeof(LONG),
					 &(stWP.rcNormalPosition.top));

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_RIGHT,
					 sizeof(LONG),
					 &(stWP.rcNormalPosition.right));

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_BOTTOM,
					 sizeof(LONG),
					 &(stWP.rcNormalPosition.bottom));

	// mrw:4/21/95
	//
	if (stWP.showCmd == SW_SHOWMINIMIZED || stWP.showCmd == SW_MINIMIZE ||
		stWP.showCmd == SW_SHOWMINNOACTIVE)
		{
		stWP.showCmd = SW_SHOWNORMAL;
		}

	sfPutSessionItem(hhSess->hSysFile,
					 SFID_SESS_SHOWCMD,
					 sizeof(UINT),
					 &(stWP.showCmd));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessSaveBackScroll
 *
 * DESCRIPTION:
 *	This function is called to take the stuff that is in the backscroll and
 *	on the screen and save it away in the session file.
 *
 *	The first attempt to do this will be simply a brute force attack.  No
 *	real attempt to be tricky or cute.  Just slam it through.  Maybe it will
 *	need to be changed later.  But that is later.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	TRUE if everything is OK, otherwise FALSE
 *
 */
BOOL sessSaveBackScroll(const HSESSION hSession)
	{
	BOOL bRet = TRUE;
	int nRet;
	POINT pBeg;
	POINT pEnd;
	ECHAR *pszData;
	ECHAR *pszPtr;
	ECHAR *pszEnd;
	DWORD dwSize;

	/* --- Don't bother with this if nothing has changed --- */

	if (backscrlChanged(sessQueryBackscrlHdl(hSession)) == FALSE)
		return TRUE;

	// Also, if there is no session window, don't bother since there
	// won't be any terminal window and CopyTextFromTerminal() will
	// fault. - mrw

	if (!IsWindow(sessQueryHwnd(hSession)))
		return TRUE;


	pBeg.x = 0;
	pBeg.y = -backscrlGetUNumLines(sessQueryBackscrlHdl(hSession)); //-BKSCRL_USERLINES_DEFAULT_MAX;
	pEnd.x = 132;
	pEnd.y = 50;

	pszData = (ECHAR *)0;
	dwSize = 0;

	CopyTextFromTerminal(hSession,
						&pBeg, &pEnd,
						(void **)&pszData,
						&dwSize,
						FALSE);

	assert(pszData);

	if (pszData != (ECHAR *)0)
		{
		assert(dwSize);
		/*
		 * We need to do a little work here to make sure that whatever
		 * trailing blank lines there are don't get put into the saved
		 * text.
		 */
		pszPtr = pszData;
		pszEnd = pszPtr;
		while (*pszPtr != ETEXT('\0'))
			{
			if (*pszPtr != ETEXT('\r'))
				pszEnd = pszPtr;
			pszPtr = pszPtr++;
			}
		pszEnd = pszPtr++;
		dwSize = (DWORD)((pszEnd - pszData) * sizeof(ECHAR));

		nRet = sfPutSessionItem(sessQuerySysFileHdl(hSession),
								SFID_BKSC_TEXT,
								dwSize,
								pszData);
		free(pszData);
		pszData = NULL;
		}

	return bRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessRestoreBackScroll
 *
 * DESCRIPTION:
 *	This function is called to read a bunch of stuff (text) from the session
 *	file and cram it into the backscroll.
 *
 *	The first attempt to do this will be simply a brute force attack.  No
 *	real attempt to be tricky or cute.  Just slam it through.  Maybe it will
 *	need to be changed later.  But that is later.
 *
 * ARGUMENTS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	TRUE if everything is OK, otherwise FALSE
 *
 */
BOOL sessRestoreBackScroll(const HSESSION hSession)
    {

    BOOL bRet = TRUE;
    unsigned long lSize;
    ECHAR *pszData;
    ECHAR *pszPtr;
    ECHAR *pszEnd;
    HBACKSCRL hBS;

    hBS = sessQueryBackscrlHdl(hSession);
    assert(hBS);

    /* Whenever we load in new session file, get rid of the old BS */
    backscrlFlush(hBS);

    lSize = 0;
    sfGetSessionItem(sessQuerySysFileHdl(hSession), SFID_BKSC_TEXT, &lSize, NULL);

    if (lSize > 0)
        {
        pszData = (ECHAR*)malloc(lSize + sizeof(ECHAR));
        if (pszData == NULL)
            {
            assert(pszData);
            return FALSE;
            }

        memset(pszData, 0, lSize + sizeof(ECHAR));

        sfGetSessionItem(sessQuerySysFileHdl(hSession),
                                        SFID_BKSC_TEXT,
                                        &lSize,
                                        pszData);


        // JYF 29-Mar-1999 changed pszPtr <= pszData+lSize
        //  so we don't go beyond the end of the buffer.

        pszPtr = pszData;
        while ((*pszPtr != ETEXT('\0')) &&
               ((DWORD_PTR)pszPtr < ((DWORD_PTR)pszData + lSize)))
            {
            pszEnd = pszPtr;
            while (((DWORD_PTR)pszEnd < ((DWORD_PTR)pszData + lSize)) &&
                   (*pszEnd != ETEXT('\0')) &&
                   (*pszEnd != ETEXT('\r')))
                pszEnd = pszEnd++;

            /* Stuff the line into the backscroll */
            backscrlAdd(hBS, pszPtr, (int)(pszEnd - pszPtr));

            /* Bump pointer to the beginning of the next line */
            pszPtr = pszEnd;
            if (*pszPtr == ETEXT('\r'))
                pszPtr = pszPtr++;
            }

        if (pszData)
            {
            free(pszData);
            pszData = NULL;
            }
        }

    backscrlResetChangedFlag(hBS);
    return bRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	sessCheckAndLoadCmdLn
 *
 * DESCRIPTION:
 *	When the programs starts up, we save the command line.  If there is
 *	something on the command line, we check and see if maybe it is the name
 *	of a session file.	If it is, we open the session file. Other command
 *	line switches are processed here as well.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	0=OK, else error.
 */
int sessCheckAndLoadCmdLn(const HSESSION hSession)
	{
	int 				nIdx;
	int 				iRet = -1;
	LPTSTR 				pszStr;
#if defined(INCL_WINSOCK)
    LPTSTR              pszTmp;
	TCHAR*              pszTelnet = TEXT("telnet:");
	TCHAR*              pszPort = NULL;
#endif
	TCHAR				acPath[FNAME_LEN], acName[FNAME_LEN];
	const HHSESSION 	hhSess = VerifySessionHandle(hSession);

	if (hhSess->achSessCmdLn[0] == TEXT('\0'))
		return -1;

	acPath[0] = TEXT('\0');
	acName[0] = TEXT('\0');
    // Assume there's the name of a session file we want to open on the
    // command line. We'll accept a prepended /D as well.
    //
	hhSess->iCmdLnDial = CMDLN_DIAL_DIAL;
	nIdx = 0;

	for (pszStr = hhSess->achSessCmdLn;
		*pszStr != TEXT('\0');
		pszStr = StrCharNext(pszStr))
		{
		/*
		 * This works because we only allow certain characters as switches
		 */
		if (*pszStr == TEXT('/'))
			{
			/* Process as a switch */
			pszStr = StrCharNext(pszStr);
            //jmh 3/24/97 Check for end of string here...
            if (*pszStr == TEXT('\0'))
                {
                break;
                }

			if ((*pszStr == TEXT('D')) || (*pszStr == TEXT('d')))
                {
                // The name that follows is a session file
				hhSess->iCmdLnDial = CMDLN_DIAL_DIAL;
                }

#if defined(INCL_WINSOCK)
            if ((*pszStr == TEXT('T')) || (*pszStr == TEXT('t')))
                {
                pszTmp = StrCharNext(pszStr);
                //jmh 3/24/97 Check for end of string here...
                if (*pszTmp == TEXT('\0'))
                    {
                    break;
                    }
                else if (*pszTmp == TEXT(' '))
                    {
                    // The name that follows is a telnet address
                    hhSess->iCmdLnDial = CMDLN_DIAL_WINSOCK;
                    }
                }
#endif
			}
		else
			{
			/* Copy all non switch stuff to the buffer */
			if (IsDBCSLeadByte(*pszStr))
				{
				MemCopy(&acPath[nIdx], pszStr, (size_t)2 * sizeof(TCHAR));
				nIdx += 2;
				}
			else
				{
				acPath[nIdx++] = *pszStr;
				}
			}
		}

	// Removed old logic here and call GetFileNameFromCmdLine() which
	// does something similar to this function.  On return, we should
	// have a fully qualified path name. - mrw,3/2/95
	//
	acPath[nIdx] = TEXT('\0');
    TCHAR_Trim(acPath);     // Strip leading spaces

#if defined(INCL_WINSOCK)
	// If this is a telnet address from the browser, it will usually be preceeded
	// by the string telnet:  If so, we must remove it or it will confuse some of
	// the code to follow  jkh, 03/22/1997
	if (*acPath && hhSess->iCmdLnDial == CMDLN_DIAL_WINSOCK)
		{
		nIdx = strlen(pszTelnet);
                if (_strnicmp(acPath, pszTelnet, nIdx) == 0)
			{
			// Remove the telnet string from the front of acPath
			memmove(acPath, &acPath[nIdx], (strlen(acPath) - nIdx) + 1);
			}
		}

	// See if URL contains a port number. This will take the form of
    // addr:nnn where nnn is the port number i.e. culine.colorado.edu:860
    // or there might be the name of an assigned port like hilgraeve.com:finger.
    // We support numeric port right now, may add port names later. jkh, 3/22/1997
    pszPort = strchr(acPath, TEXT(':'));
    if (pszPort && isdigit(pszPort[1]))
        {
        pszPort[0] = TEXT('\0');
        ++pszPort;
		hhSess->iTelnetPort = atoi(pszPort);
        }
    else
        pszPort = NULL;

#endif
	GetFileNameFromCmdLine(acPath, acName, sizeof(acName));

    if (acName[0] == 0)
        {
        // Nothing on the command line
        hhSess->iCmdLnDial = CMDLN_DIAL_NEW;
        iRet = 0;
        }
    else
        {
        // Look for a pre-existing session file. First, the old TRM format
        //
        if (fTestOpenOldTrmFile(hhSess, acName) != 0)
            {
            // Next, try the more common HyperTerminal file format
            //
            if (sfOpenSessionFile(hhSess->hSysFile, acName) < SF_OK)
                {
                // Command-line argument is not an existing file. Decide
                // how to act based on command-line switches.
                //
                if (hhSess->iCmdLnDial == CMDLN_DIAL_DIAL)
                    {
                    // We were asked to open and dial a pre-existing session
                    // file, and failed.
                    //
    			    TCHAR acFormat[64];
	    		    TCHAR ach[FNAME_LEN];
		    	    LoadString(glblQueryDllHinst(),
			    			    IDS_ER_BAD_SESSION,
				    		    acFormat,
					    	    sizeof(acFormat) / sizeof(TCHAR));
			        wsprintf(ach, acFormat, acName);

			        TimedMessageBox(sessQueryHwnd(hSession),
							        ach,
							        TEXT(""),
							        MB_OK | MB_ICONEXCLAMATION,
							        sessQueryTimeout(hSession));

			        sfSetSessionFileName(hhSess->hSysFile, TEXT(""));

                    // Go to the Open dialog
			        hhSess->iCmdLnDial = CMDLN_DIAL_OPEN;
                    }
#if defined(INCL_WINSOCK)
                else if (hhSess->iCmdLnDial == CMDLN_DIAL_WINSOCK)
                    {
                    //jmh 3/24/97 For future maintainers: there's some
                    // skulduggery going on here that's worth explaining. When
                    // you try to open a non-existent file, the name is still
                    // stored. The code to do a telnet command-line dial
                    // depends on this. Honest! I didn't write this...
                    //
                    //jmh 3/24/97 Mark this as a new session, so user will be
                    // prompted to save on exit.
                    hhSess->fIsNewSession = TRUE;
                    iRet = 0;
                    }
#endif
                }
            else
                {
                // Command-line argument is an existing HyperTerminal file
                //
                hhSess->iCmdLnDial = CMDLN_DIAL_DIAL;
                iRet = 0;
                }
            }
        else
            {
            // Command-line argument is an existing TRM file
            //
            hhSess->iCmdLnDial = CMDLN_DIAL_DIAL;
            iRet = 0;
            }
        }

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fTestOpenOldTrmFile
 *
 * DESCRIPTION:
 *	Tests if its an old trm file. If so, it opens it and reads the
 *	data out.
 *
 * ARGUMENTS:
 *	hSession	- our friend the public session handle
 *	ach 		- name of file.
 *
 * RETURNS:
 *	0=OK, else not trm file
 *
 */
int fTestOpenOldTrmFile(const HHSESSION hhSess, TCHAR *pachName)
	{
	int iRet = -1;
	HANDLE hFile;
	DWORD  dw;
	LPTSTR pszPtr;
	TCHAR  ach[80];
	TCHAR  achName[256];

	StrCharCopy(achName, pachName);
	pszPtr = StrCharFindLast(achName, TEXT('.'));

	if (pszPtr && (StrCharCmpi(pszPtr, TEXT(".TRM")) == 0))
		{
		/* Old .TRM files case */
		hFile = CreateFile(achName, GENERIC_READ, FILE_SHARE_READ, 0,
				OPEN_EXISTING, 0, 0);

		if (hFile != INVALID_HANDLE_VALUE)
			{
			// Phone number is always at offset 0x282 for old .trm files.
			//
			if (SetFilePointer(hFile, 0x282, 0, FILE_BEGIN) != (DWORD)-1)
				{
				if (ReadFile(hFile, ach, sizeof(ach), &dw, 0) == TRUE)
					{
					CloseHandle(hFile);
					ach[sizeof(ach)-1] = TEXT('\0');

					cnctSetDestination(hhSess->hCnct, ach,
						StrCharGetByteCount(ach));

					*pszPtr = TEXT('\0');
					mscStripExt(mscStripPath(achName));
					sessSetName((HSESSION)hhSess, achName);
					hhSess->iCmdLnDial = CMDLN_DIAL_OPEN;
					hhSess->fIsNewSession = TRUE;	// so it asks to save
					hhSess->nIconId = IDI_PROG1;
					hhSess->hIcon = extLoadIcon(MAKEINTRESOURCE(IDI_PROG1));
					iRet = 0;
					}

				else
					{
					DbgShowLastError();
					CloseHandle(hFile);
					}
				}

			else
				{
				DbgShowLastError();
				CloseHandle(hFile);
				}
			}

		else
			{
			DbgShowLastError();
			}
		}

	return iRet;
	}
