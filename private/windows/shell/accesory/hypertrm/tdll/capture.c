/*
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 8/18/99 10:52a $
 */
#include <windows.h>
#pragma hdrstop

// #define	DEBUGSTR	1

#define	DO_RAW_MODE	1

#include <string.h>

#include <tdll\stdtyp.h>

#include <tdll\mc.h>
#include <tdll\sf.h>
#include <tdll\assert.h>
#include <tdll\file_io.h>
#include <tdll\globals.h>
#include <tdll\session.h>
#include <tdll\sess_ids.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\open_msc.h>

#include <term\res.h>

#include "capture.h"
#include "capture.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CreateCaptureFileHandle
 *
 * DESCRIPTION:
 *	This function is called to create a capture file handle and fill it with
 *	some reasonable set of defaults.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A capture file handle or ZERO if there was an error.
 *
 */
HCAPTUREFILE CreateCaptureFileHandle(const HSESSION hSession)
	{
	int nRet;
	STCAPTURE *pST = NULL;	// REV 8/27/98

	pST = (STCAPTURE *)malloc(sizeof(STCAPTURE));
	if (pST == (STCAPTURE *)0)
		goto CCFHexit;

	memset(pST, 0, sizeof(STCAPTURE));

	nRet = InitializeCaptureFileHandle(hSession, (HCAPTUREFILE)pST);

	if (nRet == 0)
		return (HCAPTUREFILE)pST;

CCFHexit:

	if (pST != (STCAPTURE *)0)
		{
		if (pST->pszInternalCaptureName != (LPTSTR)0)
			{
			free(pST->pszInternalCaptureName);
			pST->pszInternalCaptureName = NULL;
			}
		if (pST->pszDefaultCaptureName != (LPTSTR)0)
			{
			free(pST->pszDefaultCaptureName);
			pST->pszDefaultCaptureName = NULL;
			}
		if (pST->pszTempCaptureName != (LPTSTR)0)
			{
			free(pST->pszTempCaptureName);
			pST->pszTempCaptureName = NULL;
			}
		free(pST);
		pST = NULL;
		}
	return (HCAPTUREFILE)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DestroyCaptureFileHandle
 *
 * DESCRIPTION:
 *	This function is called to free up all the resources that are associated
 *	with the capture file handle.  After this, it is GONE.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *
 * RETURNS:
 *	Nothing.
 *
 */
void DestroyCaptureFileHandle(HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST = NULL;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		if (pST->hCaptureFile != NULL)
			{
			fio_close(pST->hCaptureFile);
			}
		pST->hCaptureFile = NULL;

		if (pST->pszDefaultCaptureName != (LPTSTR)0)
			{
			free(pST->pszDefaultCaptureName);
			pST->pszDefaultCaptureName = NULL;
			}
		if (pST->pszInternalCaptureName != (LPTSTR)0)
			{
			free(pST->pszInternalCaptureName);
			pST->pszInternalCaptureName = NULL;
			}
		if (pST->pszTempCaptureName != (LPTSTR)0)
			{
			free(pST->pszTempCaptureName);
			pST->pszTempCaptureName = NULL;
			}

		if (pST->hMenu)
			{
			/* We only destroy it if it is not in use */
			switch (pST->nState)
				{
				case CPF_CAPTURE_ON:
				case CPF_CAPTURE_PAUSE:
				case CPF_CAPTURE_RESUME:
					DestroyMenu(pST->hMenu);
					break;
				case CPF_CAPTURE_OFF:
				default:
					break;
				}
			}

		free(pST);
		pST = NULL;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	InitializeCaptureFileHandle
 *
 * DESCRIPTION:
 *	This function is called to set the capture file handle to a known state
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	ZERO if everything was OK, otherwise an error code
 */
int InitializeCaptureFileHandle(const HSESSION hSession, HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST = (STCAPTURE *)hCapt;
	int nLen;
	LPTSTR pszStr;
	TCHAR acBuffer[FNAME_LEN];

	assert(pST);
	memset(acBuffer, '\0', FNAME_LEN); // REV 8/27/98

	pST->hSession = hSession;

	/* Put together a reasonable default for the capture file */
	//Changed to remember the working folder path - mpt 8-18-99
	if ( !GetWorkingDirectory(acBuffer, sizeof(acBuffer) / sizeof(TCHAR)) )
		{
		GetCurrentDirectory(sizeof(acBuffer) / sizeof(TCHAR), acBuffer);
		}
	pszStr = StrCharLast(acBuffer);
	if (*pszStr != TEXT('\\'))
		StrCharCat(pszStr, TEXT("\\"));

	/* Decide if this should be in the resource strings */
	// StrCharCat(acBuffer, TEXT("CAPTURE.TXT"));
	pszStr = StrCharEnd(acBuffer);
	LoadString(glblQueryDllHinst(),
				IDS_CPF_CAP_FILE,
				pszStr,
				(int)(sizeof(acBuffer) - (pszStr - acBuffer) / sizeof(TCHAR)));
	nLen = StrCharGetByteCount(acBuffer) + 1;

	if (pST->pszInternalCaptureName != (LPTSTR)0)
		{
		free(pST->pszInternalCaptureName);
		pST->pszInternalCaptureName = NULL;
		}
	pST->pszInternalCaptureName = (LPTSTR)malloc((unsigned int)nLen * sizeof(TCHAR));
	if (pST->pszInternalCaptureName == (LPTSTR)0)
		goto ICFHexit;
	StrCharCopy(pST->pszInternalCaptureName, acBuffer);

	if (pST->pszDefaultCaptureName != (LPTSTR)0)
		free(pST->pszDefaultCaptureName);
	pST->pszDefaultCaptureName = (LPTSTR)0;

	if (pST->pszTempCaptureName != (LPTSTR)0)
		free(pST->pszTempCaptureName);
	pST->pszTempCaptureName = (LPTSTR)0;

#if defined(DO_RAW_MODE)
	pST->nDefaultCaptureMode = CPF_MODE_RAW;
	pST->nTempCaptureMode = CPF_MODE_RAW;
#else
	pST->nDefaultCaptureMode = CPF_MODE_LINE;
	pST->nTempCaptureMode = CPF_MODE_LINE;
#endif

	pST->nDefaultFileMode = CPF_FILE_APPEND;
	pST->nTempFileMode = CPF_FILE_APPEND;

	pST->nState = CPF_CAPTURE_OFF;

	return 0;

ICFHexit:

	if (pST != (STCAPTURE *)0)
		{
		if (pST->pszInternalCaptureName != (LPTSTR)0)
			{
			free(pST->pszInternalCaptureName);
			pST->pszInternalCaptureName = NULL;
			}
		}
	return CPF_NO_MEMORY;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	LoadCaptureFileHandle
 *
 * DESCRIPTION:
 *	This function is called to load the capture file settings from the system
 *	file.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	ZERO if everythis was OK, otherwise an error code.
 *
 */
int LoadCaptureFileHandle(HCAPTUREFILE hCapt)
	{
	int nRet = 0;
	long lSize;
	STCAPTURE *pOld;

	pOld = (STCAPTURE *)hCapt;
	assert(pOld);
	if (pOld->hCaptureFile != NULL)
		{
		fio_close(pOld->hCaptureFile);
		}
	pOld->hCaptureFile = NULL;
	pOld->nState = CPF_CAPTURE_OFF;

	nRet = InitializeCaptureFileHandle(pOld->hSession, hCapt);
	if (nRet)
		return nRet;

	lSize = 0;
	sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
					SFID_CPF_MODE,
					&lSize, NULL);
	if (lSize)
		{
		sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
						SFID_CPF_MODE,
						&lSize,
						&pOld->nDefaultCaptureMode);
		if (pOld->nDefaultCaptureMode == 0)
#if defined(DO_RAW_MODE)
			pOld->nDefaultCaptureMode = CPF_MODE_RAW;
#else
			pOld->nDefaultCaptureMode = CPF_MODE_LINE;
#endif
		pOld->nTempCaptureMode = pOld->nDefaultCaptureMode;
		}

	lSize = 0;
	sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
					SFID_CPF_FILE,
					&lSize, NULL);
	if (lSize)
		{
		lSize = sizeof(int);
		sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
						SFID_CPF_FILE,
						&lSize,
						&pOld->nDefaultFileMode);
		if (pOld->nDefaultFileMode == 0)
			pOld->nDefaultFileMode = CPF_FILE_APPEND;
		pOld->nTempFileMode = pOld->nDefaultFileMode;
		}

	lSize = 0;
	sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
					SFID_CPF_FILENAME,
					&lSize, NULL);
	if (lSize)
		{
		LPTSTR pszName;

		pszName = (LPTSTR)0;
		pszName = malloc((unsigned int)lSize);
		if (!pszName)
			{
			return CPF_NO_MEMORY;
			}
		sfGetSessionItem(sessQuerySysFileHdl(pOld->hSession),
						SFID_CPF_FILENAME,
						&lSize, pszName);

		if (pOld->pszDefaultCaptureName)
			{
			free(pOld->pszDefaultCaptureName);
			pOld->pszDefaultCaptureName = NULL;
			}
		pOld->pszDefaultCaptureName = pszName;

		if (pOld->pszTempCaptureName)
			free(pOld->pszTempCaptureName);
		pOld->pszTempCaptureName = (LPTSTR)0;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SaveCaptureFileHandle
 *
 * DESCRIPTION:
 *	This function is called to save whatever has been changed in the capture
 *	file handle out to the system file.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */
int SaveCaptureFileHandle(HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST;
	long lSize;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		/* Need to save the new value */
		if (pST->pszDefaultCaptureName)
			{
			lSize = StrCharGetByteCount(pST->pszDefaultCaptureName);
			if (lSize > 0)
				{
				lSize += 1;
				lSize *= sizeof(TCHAR);

				sfPutSessionItem(sessQuerySysFileHdl(pST->hSession),
								SFID_CPF_FILENAME,
								(unsigned long)lSize,
								pST->pszDefaultCaptureName);
				}
			}

		/* Need to save the new value */
		sfPutSessionItem(sessQuerySysFileHdl(pST->hSession),
						SFID_CPF_MODE,
						sizeof(int),
						&pST->nDefaultCaptureMode);

		/* Need to save the new value */
		sfPutSessionItem(sessQuerySysFileHdl(pST->hSession),
						SFID_CPF_FILE,
						sizeof(int),
						&pST->nDefaultFileMode);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfGetCaptureFilename
 *
 * DESCRIPTION:
 *	This function is used to get the default capture file name from the
 *	handle in order to load it into the dialog box.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	pszName  -- pointer to where to copy the file name
 *	nLen     -- size of the buffer
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */
int cpfGetCaptureFilename(HCAPTUREFILE hCapt,
						LPTSTR pszName,
						const int nLen)
	{
	STCAPTURE *pST;
	LPTSTR pszStr;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		if ((pszStr = pST->pszTempCaptureName) == (LPTSTR)0)
			{
			if ((pszStr = pST->pszDefaultCaptureName) == (LPTSTR)0)
				pszStr = pST->pszInternalCaptureName;
			}
		if (StrCharGetByteCount(pszStr) >= nLen)
			return CPF_SIZE_ERROR;
		StrCharCopy(pszName, pszStr);
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfSetCaptureFilename
 *
 * DESCRIPTION:
 *	This function is called to set the name of the capture file.  If the mode
 *	flag is FALSE, only the temporary name is set.  If the mode file is TRUE,
 *	both the temporary and default names are set.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	pszName  -- the file name
 *	nMode    -- the mode flag, see above
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */
int cpfSetCaptureFilename(HCAPTUREFILE hCapt,
						LPCTSTR pszName,
						const int nMode)
	{
	int nRet = 0;
	int nLen;
	LPTSTR pszTemp;
	LPTSTR pszDefault;
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		nLen = StrCharGetByteCount(pszName) + 1;
		pszTemp = (LPTSTR)0;
		pszDefault = (LPTSTR)0;

		pszTemp = (LPTSTR)malloc((unsigned int)nLen * sizeof(TCHAR));
		if (pszTemp == (LPTSTR)0)
			{
			nRet = CPF_NO_MEMORY;
			goto SCFexit;
			}
		StrCharCopy(pszTemp, pszName);
		if (nMode)
			{
			pszDefault = (LPTSTR)malloc((unsigned int)nLen * sizeof(TCHAR));
			if (pszDefault == (LPTSTR)0)
				{
				nRet = CPF_NO_MEMORY;
				goto SCFexit;
				}
			StrCharCopy(pszDefault, pszName);
			}
		/* Got the memory that we need */
		if (pST->pszTempCaptureName)
			{
			free(pST->pszTempCaptureName);
			pST->pszTempCaptureName = NULL;
			}
		pST->pszTempCaptureName = pszTemp;
		if (nMode)
			{
			if (pST->pszDefaultCaptureName)
				{
				free(pST->pszDefaultCaptureName);
				pST->pszDefaultCaptureName = NULL;
				}
			pST->pszDefaultCaptureName = pszDefault;
			}
		}
	return nRet;

SCFexit:
	if (pszTemp)
		{
		free(pszTemp);
		pszTemp = NULL;
		}
	if (pszDefault)
		{
		free(pszDefault);
		pszDefault = NULL;
		}
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfGetCaptureMode
 *
 * DESCRIPTION:
 *	This function returns the current default capture mode.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *
 * RETURNS:
 *	The current capture mode.
 *
 */
int cpfGetCaptureMode(HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		return pST->nDefaultCaptureMode;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfSetCaptureMode
 *
 * DESCRIPTION:
 *	This function changes the capture mode.  If the mode flag is set to FALSE,
 *	only the temporary mode is set.  If the mode flag is set to TRUE, both the
 *	temporary and default mode flags are set.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	nCaptMode -- the capture mode
 *	nModeFlag -- the mode flag
 *
 * RETURNS:
 *	ZERO of everything is OK, otherwise an error code
 *
 */
int cpfSetCaptureMode(HCAPTUREFILE hCapt,
						const int nCaptMode,
						const int nModeFlag)
	{
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		pST->nTempCaptureMode = nCaptMode;
		if (nModeFlag)
			pST->nDefaultCaptureMode = nCaptMode;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfGetCaptureFileflag
 *
 * DESCRIPTION:
 *	This function is called to get the file save flags for capture to file.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *
 * RETURNS:
 *	The file save flags.
 *
 */
int cpfGetCaptureFileflag(HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		return pST->nDefaultFileMode;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfSetCaptureFileflag
 *
 * DESCRIPTION:
 *	This function is called to set the file save mode flag.  If the mode flag
 *	is set to FALSE, only the temporary value is changed, if the mode flag is
 *	TRUE, both the temporary and default values are changed.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	nSaveMode -- the new file save mode value
 *	nModeFlag -- the mode flag, see above
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 *
 */
int cpfSetCaptureFileflag(HCAPTUREFILE hCapt,
						const int nSaveMode,
						const int nModeFlag)
	{
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		pST->nTempFileMode = nSaveMode;
		if (nModeFlag)
			pST->nDefaultFileMode = nSaveMode;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfGetCaptureState
 *
 * DESCRIPTION:
 *	This function returns a value that reflects the state of capturing.  It can
 *	be on, off, or paused.  See "capture.h" for the actual values.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *
 * RETURNS:
 *	The current state of capturing.
 *
 */
int cpfGetCaptureState(HCAPTUREFILE hCapt)
	{
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		return pST->nState;
		}
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	cpfSetCaptureState
 *
 * DESCRIPTION:
 *	This function changes the state of capturing.  It can turn it on, off,
 *	pause capturing or resume capturing.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	nState   -- the new state to change to
 *
 * RETURNS:
 *	The previous state of capturing.
 *
 */
int cpfSetCaptureState(HCAPTUREFILE hCapt, int nState)
	{
	int nOldState = 0;
	STCAPTURE *pST;
	LPTSTR pszStr;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		switch (nState)
			{
			case CPF_CAPTURE_ON:
				/* Open the capture file */
				if (pST->hCaptureFile)
					{
					/*
					 * We get here one of two ways.  The first and expected
					 * way is by selecting "Resume" from the menu.  This is
					 * not a problem.  the other way is that there is still
					 * a problem with the menus.  Go figure.
					 */
					break;
					}
				if ((pszStr = pST->pszTempCaptureName) == (LPTSTR)0)
					{
					if ((pszStr = pST->pszDefaultCaptureName) == (LPTSTR)0)
						pszStr = pST->pszInternalCaptureName;
					}
				switch (pST->nTempFileMode)
					{
					default:
					case CPF_FILE_OVERWRITE:
						pST->hCaptureFile = fio_open(pszStr,
													FIO_CREATE | FIO_WRITE);
						assert(pST->hCaptureFile);
						break;
					case CPF_FILE_APPEND:
						pST->hCaptureFile = fio_open(pszStr,
													FIO_WRITE | FIO_APPEND);
						assert(pST->hCaptureFile);
						break;
					case CPF_FILE_REN_SEQ:
						assert(TRUE);
						break;
					case CPF_FILE_REN_DATE:
						assert(TRUE);
						break;
					}
				break;
			case CPF_CAPTURE_OFF:
				/* Close the capture file */
				assert(pST->hCaptureFile);
				fio_close(pST->hCaptureFile);
				pST->hCaptureFile = NULL;
				break;
			default:
				break;
			}

		nOldState = pST->nState;
		pST->nState = nState;
		}
	return nOldState;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
HMENU cpfGetCaptureMenu(HCAPTUREFILE hCapt)
	{
	HMENU hRet = (HMENU)0;
	STCAPTURE *pST;

	pST = (STCAPTURE *)hCapt;
	assert(pST);

	if (pST != (STCAPTURE *)0)
		{
		// The SetMenuItemInfo() call will destroy this submenu whenever
		// its replaced by something else (like a plain menu item).  The
		// result was you could go into capture once, but the second time
		// you activated the menu it wouldn't load because the handle was
		// no longer valid.  So I moved the code to load the menu here. - mrw
		//
		if (!IsMenu(pST->hMenu))
			pST->hMenu = LoadMenu(glblQueryDllHinst(), TEXT("MenuCapture"));

		assert(pST->hMenu);
		hRet = pST->hMenu;
		}

	return hRet;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CaptureChar
 *
 * DESCRIPTION:
 *	This function is called whenever the emulators have a character that might
 *	need to be capture.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	nFlags   -- capture flags (must match the capture state we are in)
 *	cData    -- the character to be captured
 *
 * RETURNS:
 *
 */
void CaptureChar(HCAPTUREFILE hCapt, int nFlags, ECHAR cData)
	{
	STCAPTURE *pST = (STCAPTURE *)hCapt;
	int nLen = 0;
	int i    = 0;
	TCHAR cChar[3];
//	TCHAR cChar = (TCHAR)cData;

	if (pST == NULL)
		{
		assert(pST);
		return;
		}

	/* Check the state */
	if (pST->nState == CPF_CAPTURE_OFF)
		return;
	if (pST->nState == CPF_CAPTURE_PAUSE)
		return;

	/* Check the file next */
	if (pST->hCaptureFile == NULL)
		return;

	/* Check the mode */
	if (pST->nTempCaptureMode != nFlags)
		return;

	DbgOutStr("Cc 0x%x %c (0x%x)\r\n", nFlags, cData, cData, 0,0);

	CnvrtECHARtoTCHAR(cChar, sizeof(cChar), cData);

	/* Write out the character */
//	fio_putc(cChar, pST->hCaptureFile);
//	nLen = StrCharGetByteCount(cChar);
//	for (i = 0; i < nLen; i++)
// 		fio_putc(cChar[i], pST->hCaptureFile);

    fio_putc(cChar[0], pST->hCaptureFile);
    if (cChar[1])
        fio_putc(cChar[1], pST->hCaptureFile);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CaptureLine
 *
 * DESCRIPTION:
 *	This function is called whenever the emulators have a line that might need
 *	to be captured.
 *
 * PARAMETERS:
 *	hCapt    -- the capture handle
 *	nFlags   -- capture flags (must match the capture state we are in)
 *	pszStr   -- pointer to line to capture (sans <cr/lf>)
 *
 * RETURNS:
 *
 */
void CaptureLine(HCAPTUREFILE hCapt, int nFlags, ECHAR *achStr, int nLen)
	{
	STCAPTURE *pST = (STCAPTURE *)hCapt;
	LPTSTR pchEnd = NULL;
	TCHAR *pszStr = NULL;

	if (pST == NULL)
		{
		assert(pST);
		return;
		}

	/* Check the state */
	if (pST->nState == CPF_CAPTURE_OFF)
		return;
	if (pST->nState == CPF_CAPTURE_PAUSE)
		return;

	/* Check the file next */
	if (pST->hCaptureFile == NULL)
		return;

	/* Check the mode */
	if (pST->nTempCaptureMode != nFlags)
		return;


	// Allocate space assuming every character is double byte
	pszStr = (TCHAR *)malloc(((unsigned int)StrCharGetEcharLen(achStr) +
							  sizeof(ECHAR)) * sizeof(ECHAR));
	if (pszStr == NULL)
		{
		assert(FALSE);
		return;
		}

	CnvrtECHARtoMBCS(pszStr, ((unsigned long)StrCharGetEcharLen(achStr) + 1)
					* sizeof(ECHAR), achStr,
					StrCharGetEcharByteCount(achStr) + sizeof(ECHAR)); // mrw:5/17/95

	pchEnd = pszStr + (StrCharGetByteCount(pszStr) - 1);

	/* Write out the string */
	while (pszStr <= pchEnd)		
		fio_putc(*pszStr++, pST->hCaptureFile);

	free(pszStr);
	pszStr = NULL;

	fio_putc(TEXT('\r'), pST->hCaptureFile);
	fio_putc(TEXT('\n'), pST->hCaptureFile);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
