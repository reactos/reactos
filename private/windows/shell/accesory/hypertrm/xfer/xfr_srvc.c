/* xfr_srcv.c -- transfer service routines
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

#define BYTE	char

#include <tdll\stdtyp.h>
#include <tdll\com.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\file_msc.h>
#include <tdll\xfer_msc.hh>
#include <tdll\file_io.h>
#include <tdll\tdll.h>
#include <tdll\tchar.h>
#include <tdll\misc.h>
#include <tdll\globals.h>
#include <tdll\errorbox.h>
#include <term\res.h>

#include "itime.h"

#include "xfer.h"
#include "xfer.hh"

#include "xfer_tsc.h"
#include "xfr_srvc.h"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 *                                                                            *
 *                             R E A D    M E                                 *
 *                                                                            *
 * Everybody keeps changing the TIME standard to whatever they feel might be  *
 * a little bit better for them.  So far I have found 3 different standards   *
 * in Microsoft functions.  This does not even count the fact that HyperP     *
 * uses its own format for time.                                              *
 *                                                                            *
 * Henceforth, all time values that are passed around in the program will be  *
 * based on the old UCT format of the number of seconds since Jan 1, 1970.    *
 *                                                                            *
 * Please use an unsigned long for these values.                              *
 *                                                                            *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_set_pointer
 *
 * DESCRIPTION:
 *	When a transfer is started, it is passed a parameter block.  This is
 *	where the address of that block gets stored.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pV       -- pointer to the parameter block
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfer_set_pointer(HSESSION hSession, void *pV)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pX->pXferStuff = pV;
		}
	}

void *xfer_get_pointer(HSESSION hSession)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		return (void *)pX->pXferStuff;
		}
	return (void *)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_idle
 *
 * DESCRIPTION:
 *	This function got called in Windows to make sure that the transfer
 *	display (and other tasks) got some time every now and then during a
 *	transfer.  I don't know if this needs to be done under CHICAGO, with
 *	a pre-emptive multi-tasking design.  The call is still here until it
 *	can be determined one way or another.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	Nothing.
 *
 */

#define	IDLE_WAIT		150

void xfer_idle(HSESSION h, int nMode)
	{
	HCOM   hComHandle;
	HANDLE hComEvent;

	/*
	 * This is set up for the mode flags to be OR'ed together if necessary
	 */
	if (nMode & XFER_IDLE_IO)
		{
		hComHandle = sessQueryComHdl(h);
		if (hComHandle)
			{
			hComEvent = ComGetRcvEvent(hComHandle);
			if (hComEvent)
				{
				WaitForSingleObject(hComEvent, IDLE_WAIT);
				}
			}
		}

	if (nMode & XFER_IDLE_DISPLAY)
		{
		/*
		 * The documentation says that this caused the thread to yield,
		 * presumably back to the scheduler cycle.  It tries to let the
		 * display update if possible.
		 */
		Sleep(0);
		}

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_user_interrupt
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to determine if the user
 *	has hit any of the cancel or skip buttons in the display window.
 *
 * PARAMETERS:
 *	hSession -- the session handle.
 *
 * RETURNS:
 *	ZERO if nothing to report, otherwise a CANCEL or SKIP indicator.
 *
 */
int	xfer_user_interrupt(HSESSION hSession)
	{
	INT nRetVal;
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX == NULL)
		{
		// DbgOutStr("xfer_user_interrupt returns an error\r\n", 0,0,0,0,0);
		return FALSE;
		}

	switch (pX->nUserCancel)
		{
		case XFER_ABORT:
			nRetVal = XFER_ABORT;
			pX->nUserCancel = 0;		// Reset to default value
			// DbgOutStr("xfer_user_interrupt returns 1\r\n", 0,0,0,0,0);
			break;

		case XFER_SKIP:
			nRetVal = XFER_SKIP;
			pX->nUserCancel = 0;		// Reset to default value
			// DbgOutStr("xfer_user_interrupt returns 2\r\n", 0,0,0,0,0);
			break;

		default:
			// DbgOutStr("xfer_user_interrupt returns 0\r\n", 0,0,0,0,0);
			nRetVal = 0;
			break;
		}
	return nRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_user_abort
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
int  xfer_user_abort(HSESSION hSession, int p)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX == NULL)
		{
		// TODO: decide if we need CLoopClearOutput
		// CLoopClearOutput(sessQueryCLoopHdl(hSession));
		return TRUE;
		}

	switch (p)
		{
		case 0:
		case XFER_ABORT:
		case XFER_SKIP:
			pX->nUserCancel = p;
			break;
		default:
			pX->nUserCancel = 0;
			break;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_carrier_lost
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to determine if the
 *	session is still connected to something.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	TRUE if carrier has been lost, otherwise FALSE;
 *
 */
int xfer_carrier_lost(HSESSION hSession)
	{
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX == NULL)
		{
		// DbgOutStr("xfer_user_interrupt returns an error\r\n", 0,0,0,0,0);
		return FALSE;
		}

	return pX->nCarrierLost;	/* TODO: set this somewhere */
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_purgefile
 *
 * DESCRIPTION:
 *	This function is called after a VIRUS has been detected.  It is supposed
 *	to make sure that whatever was written out to disk from the infected file
 *	gets seriously blasted.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	fname    -- the name of the file
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfer_purgefile(HSESSION hSession, TCHAR *fname)
	{

	/*
	 * Given the way buffering and deletion recovery can be done in modern
	 * systems, I am not real sure what should be done here.
	 */
	DeleteFile(fname);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrUniqueName
 *
 * DESCRIPTION:
 *	This function is called to build a NEW (currently unused) file name from
 *	an existing file name by using a sequential numbering operation.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pszSrc   -- the origional file name
 *	pszDst   -- where to put the new file name
 *
 * RETURNS:
 *	0 if everything is OK, otherwise a negative number.
 *
 */
static int xfrUniqueName(HSESSION hSession, LPTSTR pszSrc, LPTSTR pszDst)
	{
	int nRetVal = -1;
	TCHAR szSrc[_MAX_PATH];
	TCHAR szName[_MAX_PATH];
	TCHAR szTag[10];			// big enough to hold "0" to "9999"
	TCHAR *pszFName = NULL;
	TCHAR *pszExtension = NULL;
	TCHAR *pszScan = NULL;
	long  nComponentSize = 0L;
	int   nNameSpace;
	int   nTag = 0;
	int nSize = 0;

	// Let Operating system figure out full name. This will also set pszFName
	//	to point to the file name component of the path.
	nSize = GetFullPathName(pszSrc, sizeof(szSrc), szSrc, &pszFName);
	if (nSize)
		{
		if (pszFName)
			{
			// Copy name portion off for later manipulation and remove ext.
			StrCharCopy(szName, pszFName);
			mscStripExt(szName);

			// Isolate the dir portion of the path
			pszScan = StrCharPrev(szSrc, pszFName);
			if (pszScan)
				*pszScan = TEXT('\0');

			// Keep pointer to extension, if any, in original string
			pszExtension = StrCharFindLast(pszFName, TEXT('.'));
			}

		// Find maximum length of path component (this is platform dependent)

		// TODO:jkh, 12/19/94  Different drives may use different sizes
		if (!GetVolumeInformation(NULL, NULL, 0, NULL, &nComponentSize,
				NULL, NULL, 0))
			nComponentSize = 12;	// Safest size if call fails

		// Try attaching numeric tags to the name until name is unique
		nNameSpace = nComponentSize - StrCharGetByteCount(pszExtension);
		for (nTag = 0; nTag < 10000; ++nTag)
			{
			_itoa(nTag, szTag, 10);
			// make sure tag will fit on filename
			while (StrCharGetByteCount(szName) >
					nNameSpace - StrCharGetByteCount(szTag))
				{
				pszScan = StrCharLast(szName);
				*pszScan = TEXT('\0');
				}
			StrCharCopy(pszDst, szSrc); 	// start with dir portion
			StrCharCat(pszDst, TEXT("\\")); // separator
			StrCharCat(pszDst, szName); 	// original file name (truncated)
			StrCharCat(pszDst, szTag);		// numeric tag to make unique
			StrCharCat(pszDst, pszExtension); // Extension (if any)

			if (!mscIsDirectory(pszDst) && !GetFileSizeFromName(pszDst, 0))
				{
				nRetVal = 0;
				break;	/* Exit with good name */
				}
			}
		}

	return nRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfrUniqueDateName
 *
 * DESCRIPTION:
 *	This function is called to build a NEW (currently unused) file name from
 *	an existing file name by using the current data/time.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pszSrc   -- the origional file name
 *	pszDst   -- where to put the new file name
 *
 * RETURNS:
 *	0 if everything is OK, otherwise a negative number.
 *
 */
static int xfrUniqueDateName(HSESSION hSession, LPTSTR pszSrc, LPTSTR pszDst)
	{
	int nRet = 0;
	int nTag;
	LPTSTR pszDir;
	LPTSTR pszExt;
	SYSTEMTIME stT;
	TCHAR acDatestr[8];
	TCHAR acFrm[16];
	TCHAR acSrc[FNAME_LEN];
	TCHAR acDst[FNAME_LEN];

	/* Get a pointer to the path portion only */
	StrCharCopy(acSrc, pszSrc);
	pszDir = acSrc;
	pszExt = StrCharFindLast(acSrc, TEXT('\\'));

	/* Get a pointer to the file name section */
	nTag = 0;
	while ((*pszExt != TEXT('.')) && (nTag < 8))
		acFrm[nTag++] = *pszExt++;
	acFrm[nTag] = TEXT('\0');
	if (StrCharGetByteCount(acFrm) == 0)
		StrCharCopy(acFrm, TEXT("D"));

	/* Get a pointer to the extension */
	pszExt = StrCharFindLast(pszDst, TEXT('.'));
	if (pszExt == NULL)
		pszExt = ".FIL";

	GetLocalTime(&stT);
	wsprintf(acDatestr, "%x%02d%02d%1d",
						stT.wMonth,
						stT.wDay,
						stT.wHour,
						stT.wMinute % 10);
	acFrm[8 - StrCharGetByteCount(acDatestr)] = TEXT('\0');

	wsprintf(acDst, "%s%s%s%s",
					pszDir,
					acFrm,
					acDatestr,
					pszExt);

	return xfrUniqueName(hSession, acDst, pszDst);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_modify_rcv_name
 *
 * DESCRIPTION:
 *	This function is called to modify the name as necessary based on the users
 *	parameters.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pszName  -- the file name
 *	lTime    -- our internal time format, see READ ME above
 *	lFlags   -- flags
 *	pfFlags  -- pointer to returned flags
 *
 * RETURNS:
 *	    0 -- everything was OK
 *	   -1 -- file error
 *	   -2 -- reject due to date
 *	   -4 -- no date, time provided
 *	   -6 -- unconditinally refuse file
 *	   -7 -- general failure
 *
 */
int xfer_modify_rcv_name(HSESSION hSession,
						LPTSTR pszName,
						unsigned long ulTime,
						long lFlags,
						int *pfFlags)
	{
	int nRetVal = 0;
	int nOpenFlags = 0;
	int isFile = 0;
	unsigned long locTime;
    TCHAR   szNewName[FNAME_LEN];
    DWORD   dwRetVal;

	isFile = GetFileSizeFromName(pszName, NULL);

	if (isFile == FALSE)
		{
		nOpenFlags = 0;
		}
	else
		{
		switch (lFlags)
			{
		case XFR_RO_APPEND:
			nOpenFlags = TRUE;
			break;

		case XFR_RO_ALWAYS:
			nOpenFlags = 0;
			break;

		case XFR_RO_NEWER:
			if (ulTime != 0)					// Let's check the time.
				{
				locTime = itimeGetFileTime(pszName);
				if (locTime != 0)
				   	if (locTime <= ulTime)	// File is newer, accept it.
						nOpenFlags = 0;
				   	else
				   		nRetVal = -2;		// Reject it due to date.
				else
					nRetVal = -1;	 		// File error...
				}
			else
				nRetVal = -4;				// No date, time supplied.
			break;

		case XFR_RO_REN_DATE:
			//
			// Build a new name, based upon the date of the new file.
			//
			nRetVal = xfrUniqueDateName(hSession, pszName, pszName);
			if (nRetVal < 0)
				nRetVal = -7;				// Ambiguous file name.
			nOpenFlags = 0;
			break;

		default:
		case XFR_RO_REN_SEQ:
			//
			// Build a new name, based upon a sequence number algorithm.
			//
			nRetVal = xfrUniqueName(hSession, pszName, szNewName);
			if (nRetVal < 0)
				nRetVal = -7;				// Ambiguous file name.
            else
                {
                dwRetVal = GetFileAttributes(pszName);
                if (dwRetVal != 0xFFFFFFFF &&
                    (dwRetVal & FILE_ATTRIBUTE_DIRECTORY) != 0)
                    {
                    nRetVal = -8;   // File is a directory
                    }
                else if (MoveFile(pszName, szNewName) == FALSE)
                    {
                    nRetVal = -8;   // File is opened
                    }
                }
			nOpenFlags = 0;
			break;

		case XFR_RO_NEVER:
			nRetVal = -6;
			break;
			}
		}

	if (nRetVal >= 0)
		*pfFlags = nOpenFlags;

	return nRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_makepaths
 *
 * DESCRIPTION:
 *	This function is called to make sure that a pathname exists.  It creates
 *	whatever portion of the pathname needs to be created.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pszPath  -- the path
 *
 * RETURNS:
 *	    0 -- everything was OK
 *	   -1 -- bad path format
 *	   -2 -- disk error of some sort
 *
 */
int xfer_makepaths(HSESSION hSession, LPTSTR pszPath)
	{
	TCHAR ach[256], achFormat[256], ach2[50];

	if (pszPath == 0)
		return -1;

	if (!mscIsDirectory(pszPath))
		{
		if (LoadString(glblQueryDllHinst(), IDS_GNRL_CREATE_PATH, achFormat,
				sizeof(achFormat)) == 0)
			{
			DbgShowLastError();
			return -3;
			}

		if (LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, ach2,
				sizeof(ach2)) == 0)
			{
			DbgShowLastError();
			return -4;
			}

		wsprintf(ach, achFormat, pszPath);

		if (TimedMessageBox(0, ach, ach2,
				MB_YESNO | MB_TASKMODAL | MB_ICONQUESTION,
					sessQueryTimeout(hSession)) == IDYES)
			{
			if (mscCreatePath(pszPath) != 0)
				{
				assert(0);
				return -2;
				}
			}

		else
			{
			return -5;
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_create_rcv_file
 *
 * DESCRIPTION:
 *	This function is called to open the file for receiving.  It has the code
 *	to create the path to the file for those protocols that can transfer a
 *	path as well as a file name.
 *
 * PARAMETERS:
 *	hSession   -- the session handle
 *	pszName    -- the complete path name of the file
 *	lOpenFlags -- the flags to pass to fio_open
 *	              nowdays TRUE means APPEND, FALSE means overwrite
 *	phRet      -- where to return the file handle
 *
 * RETURNS:
 *	    0 -- everything was OK
 *	   -1 -- couldn't create the file
 *
 */
int xfer_create_rcv_file(HSESSION hSession,
						LPTSTR pszName,
						long lOpenFlags,
						HANDLE *phRet)
	{
	ST_IOBUF *hFile;
	LPTSTR pszStr;
	TCHAR acDir[FNAME_LEN];

	StrCharCopy(acDir, pszName);
	pszStr = StrCharLast(acDir);
	while ((*pszStr != TEXT('\\')) && (pszStr > acDir))
		pszStr = StrCharPrev(acDir, pszStr);

	if (pszStr == acDir)
		return -1;

	*pszStr = TEXT('\0');
	if (xfer_makepaths(hSession, acDir) < 0)
		return -1;

	if (lOpenFlags)
		{
		/* Open for appending */
		hFile = fio_open(pszName, FIO_APPEND | FIO_WRITE);
		}
	else
		{
		hFile = fio_open(pszName, FIO_CREATE | FIO_WRITE);
		}
	if (hFile == NULL)
		return -1;

	*phRet = hFile;
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_open_rcv_file
 *
 * DESCRIPTION:
 *	This function is called to actually do the open of the receive file.  It
 *	calls a bunch of other stuff, fiddles with names, and eventually returns.
 *
 * PARAMETERS:
 *	hSesssion  -- the session handle
 *	pstRcv     -- pointer to the receive open structure
 *	ulOverRide -- if set, flags to use instead of contents of pstRcv
 *
 * RETURNS:
 *	 0 if A-OK
 *	-1 if error occurred
 *	-2 if rejected due to date
 *	-3 if rejected because it can't save file
 *	-4 if no date/time supplied when required
 *	-5 if unable to create needed directories
 *	-6 if file rejected unconditionally
 *	-7 if general failure
 *
 */
int xfer_open_rcv_file(HSESSION hSession,
					 struct st_rcv_open *pstRcv,
					 unsigned long ulOverRide)
	{
	unsigned long ulFlags = 0;
	int nOpenFlags;
	int nRetVal = 0;
#if FALSE
	// Lower Wacker does not support message logging
	int msgIndex = -1;
#endif
	XD_TYPE *pX;
	XFR_PARAMS *pP;

	pstRcv->bfHdl = NULL;
	pstRcv->lInitialSize = 0;

	xfer_build_rcv_name(hSession, pstRcv);

	/* Get the overwrite parameters */
	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
	    pP = (XFR_PARAMS *)pX->xfer_params;
	    if (pP)
		    {
		    ulFlags = pP->nRecOverwrite;
		    }
		}
	if (ulOverRide != 0)
		ulFlags = ulOverRide;

	nRetVal = xfer_modify_rcv_name(hSession,
									pstRcv->pszActualName,
									pstRcv->lFileTime,
									ulFlags,
									&nOpenFlags);

	if (nRetVal >= 0)
		{
		HANDLE lRet;
		unsigned long size;

		size = 0;
		// if (nOpenFlags & O_APPEND)
		if (nOpenFlags)
			{
			if (!GetFileSizeFromName(pstRcv->pszActualName, &size))
				{
				size = 0;
				}
			}

		nRetVal = xfer_create_rcv_file(hSession,
										pstRcv->pszActualName,
										nOpenFlags,
										&lRet);
		if (nRetVal >= 0)
			{
			pstRcv->bfHdl = lRet;

			// if (nOpenFlags & O_APPEND)
			if (nOpenFlags)
				{
				pstRcv->lInitialSize = size;
				}
			}
		}

#if FALSE
	// Lower Wacker does not support logging
	if (nRetVal < 0)
		{
		switch (nRetVal)
			{
		case -6:            // File was rejected unconditionally
			msgIndex = 23;	// "User refused"
			break;
		case -5:			// Were unable to create needed directories
			msgIndex = 11;	// "Fatal disk error"
			break;
		case -4:  			// No date, time supplied when required
			msgIndex = 17;	// "No file time available"
			break;
		case -3:			// File could not be saved
			msgIndex = 9;	// "Cannot writ file to disk"
			break;
		case -2:  			// File was rejected due to date
			msgIndex = 16;	// "File is too old"
			break;
		case -1:			// Some error occured
			msgIndex = 10;  // "Cannot open file"
			break;
		default:			// Failed
			msgIndex = 19;  // "General failure"
			break;
			}
		xfer_log_xfer(	hSession,
			   			FALSE,
			   			pstRcv->pszSuggestedName,
			   			pstRcv->pszActualName,
			   			msgIndex	);
		}
#endif

	return nRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_build_rcv_name
 *
 * DESCRIPTION:
 *	This function is called to help build the name of the file that the
 *	transfer receive code is going to dump the data into.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	pstRcv   -- pointer to the receive open structure (contains the name)
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfer_build_rcv_name(HSESSION hSession,
						  struct st_rcv_open *pstRcv)
	{
	int nSingle;
	XD_TYPE *pX;
	XFR_PARAMS *pP;
	XFR_RECEIVE *pR;
#if defined(INC_VSCAN)
	SSHDLMCH 	ssVscanMch;
#endif
	LPTSTR pszStr;
	TCHAR acBuffer[FNAME_LEN];

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pP = (XFR_PARAMS *)pX->xfer_params;
		if (pP)
			{
			/* Just continue on to the rest of the function */
			}
		else
			{
			assert(FALSE);
			return;
			}
		pR = pX->pXferStuff;
		if (pR)
			{
			/* Just continue on to the rest of the function */
			}
		else
			{
			assert(FALSE);
			return;
			}
		}
	else
		{
		assert(FALSE);
		return;
		}

	nSingle = !pP->fUseFilenames;
	nSingle |= (pP->nRecProtocol == XF_XMODEM);
	nSingle |= (pP->nRecProtocol == XF_XMODEM_1K);

	if (nSingle)
		{
		/* User specified a single file */
		StrCharCopy(acBuffer, pR->pszDir);
		pszStr = (LPTSTR)StrCharLast(acBuffer);
		if (*pszStr != TEXT('\\'))
			{
			pszStr += 1;
			*pszStr = TEXT('\\');
			pszStr += 1;
			*pszStr = TEXT('\0');
			}
		StrCharCat(acBuffer, pR->pszName);

		fileFinalizeName(
						acBuffer,					/* values to use */
						pstRcv->pszSuggestedName,	/* filler */
						pstRcv->pszActualName,		
						MAX_PATH);
		}
	else if (!pP->fUseDirectory)
		{
		/* Use directory flag is not set */
		//
		// Ignore all paths sent to us in this case.
		//
		pszStr = StrCharLast(pstRcv->pszSuggestedName);

		/* TODO: fix this up for wide characters */
		while (pszStr >= pstRcv->pszSuggestedName)
			{
			if ((*pszStr == TEXT('\\')) || (*pszStr == TEXT(':')))
				{
				StrCharCopy(pstRcv->pszSuggestedName, ++pszStr);
				break;
				}
			else
				{
				if (pszStr == pstRcv->pszSuggestedName)
					break;
				pszStr = (LPTSTR)StrCharPrev(pstRcv->pszSuggestedName, pszStr);
				}
			}

		fileFinalizeName(
					 pstRcv->pszSuggestedName,		/* values to use */
					 pR->pszDir,					/* filler */
					 pstRcv->pszActualName,
					 MAX_PATH);
		}
	else
		{
		/* I am not a all sure about this stuff */
		if ((pstRcv->pszSuggestedName[0] == TEXT('\\')) ||
			(pstRcv->pszSuggestedName[1] == TEXT(':')))
			{
			/* if full path given */
			StrCharCopy(pstRcv->pszActualName, pstRcv->pszSuggestedName);
			}
		else
			{
			/* else use our path 1st */
			StrCharCopy(pstRcv->pszActualName, pR->pszDir);
			if ((pR->pszName != NULL) &&
				(StrCharGetByteCount(pR->pszName) > 0))
				StrCharCat(pstRcv->pszActualName, pR->pszName);
			else
				StrCharCat(pstRcv->pszActualName, pstRcv->pszSuggestedName);

			fileFinalizeName(
						 pstRcv->pszActualName,		/* values to use */
						 pstRcv->pszSuggestedName,	/* filler */
						 pstRcv->pszActualName,
						 MAX_PATH);
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_close_rcv_file
 *
 * DESCRIPTION:
 *	This function is called at the end of a transfer.  It does various things
 *	such as setting the file time/date, the size, saving partial files, and
 *	logging the transfer.  A cleanup routine.
 *
 * PARAMETERS:
 *	hSession      -- the session handle
 *	fhdl          -- the actual file handle
 *	nReason       -- transfer status code
 *	pszRemoteName -- the file name as it was sent to us
 *	pszOurName    -- the file name we actually used to save the data
 *	nSave         -- partial save flag
 *	lFilesize     -- size to set the file to
 *	lTime         -- date/time value to use to set the file
 *
 * RETURNS:
 *	TRUE if the transfer was successful, otherwise FALSE.
 *
 */
int xfer_close_rcv_file(HSESSION Hsession,
					  void *vhdl,
					  int nReason,
					  TCHAR *pszRemoteName,
					  TCHAR *pszOurName,
					  int nSave,
					  unsigned long lFilesize,
					  unsigned long lTime)		/* Fix this later */
	{
	ST_IOBUF *fhdl = (ST_IOBUF *)vhdl;

	if (nReason == TSC_COMPLETE)
		nReason = TSC_OK;

	if (fio_close(fhdl) == 0)
		{
		/* Set the size */
		if (lFilesize > 0 && nReason == TSC_OK) /*lFilesize != 0 jmh 03-08-96 */
            SetFileSize(pszOurName, lFilesize);

		/* Set the date/time */
		if (lTime != 0)
			itimeSetFileTime(pszOurName, lTime);
		}
	else
		{
		nReason = TSC_DISK_ERROR;
		}

#if FALSE
	// Lower Wacker does not log transfers */
	xfer_log_xfer(hSession, FALSE, pszRemoteName, pszOurName, nReason);
#endif

	if (nReason == TSC_OLDER_FILE)
		nReason = TSC_OK;

	if (nReason != TSC_OK && pszOurName && *pszOurName)
		{
		if (nSave == FALSE)
			DeleteFile(pszOurName);
		}

	return (nReason == TSC_OK);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_get_params
 *
 * DESCRIPTION:
 *	This function gets the protocol specific parameters for the transfer
 *	routines.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	nProtocol -- the protocol ID
 *
 * RETURNS:
 *	A pointer to the protocol block, or a NULL.
 *
 */
void *xfer_get_params(HSESSION hSession, int nProtocol)
	{
	void *pVret = (void *)0;
	XD_TYPE *pX;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		if (nProtocol < 16)
			{
			if (pX->xfer_proto_params[nProtocol] == (void *)0)
				{
				xfrInitializeParams(hSession,
									nProtocol,
									&pX->xfer_proto_params[nProtocol]);
				}
			pVret = (void *)pX->xfer_proto_params[nProtocol];
			}
		}
	return pVret;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_set_comport
 *
 * DESCRIPTION:
 *	This function is called to save the current com port settings so that the
 *	transfer code can go ahead and change them to whatever it likes.
 *
 * PARAMETERS:
 *	hSession      -- the session handle
 *	fSending      -- TRUE if sending, FALSE if receiving
 *	puiOldOptions -- where to store the old settings
 *
 * RETURNS:
 *	TRUE if everything was OK, otherwise FALSE
 *
 */
int xfer_set_comport(HSESSION hSession, int fSending, unsigned *puiOldOptions)
	{
	unsigned uiOptions = COM_OVERRIDE_8BIT;
	unsigned uiOldOptions;

	if (fSending)
		bitset(uiOptions, COM_OVERRIDE_SNDALL);
	else
		bitset(uiOptions, COM_OVERRIDE_RCVALL);

	/* TODO: find out how to decide which things need to be changed, BFMI */
	if (ComOverride(sessQueryComHdl(hSession),
					uiOptions,
					&uiOldOptions) != COM_OK)
		{
		return FALSE;
		}
	if (puiOldOptions != NULL)
		*puiOldOptions = uiOldOptions;
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_restore_comport
 *
 * DESCRIPTION:
 *	This function is called to restore the parameters that the previous call
 *	saved.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	uiOldOptions -- the old comm parameters
 *
 * RETURNS:
 *	TRUE if everything was OK, otherwise FALSE
 *
 */
int xfer_restore_comport(HSESSION hSession, unsigned uiOldOptions)
	{

	ComSndBufrWait(sessQueryComHdl(hSession), 10);

	// Let any trailing data get sent
	ComSndBufrWait(sessQueryComHdl(hSession), 10);
	if (ComOverride(sessQueryComHdl(hSession), uiOldOptions, NULL) != COM_OK)
		return FALSE;
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_save_partial
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to decide if it is
 *	OK to leave a partial file around if a transfer is aborted.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	TRUE if it is OK, otherwise FALSE
 *
 */
int xfer_save_partial(HSESSION hSession)
	{
	XD_TYPE *pX;
	XFR_PARAMS *pP;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pP = (XFR_PARAMS *)pX->xfer_params;
		if (pP)
			{
			return pP->fSavePartial;
			}
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_nextfile
 *
 * DESCRIPTION:
 *	This function is called by the transfer routines to get the name of the
 *	next file that is to be sent on over to the other side.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	filename -- where to copy the file name
 *
 * RETURNS:
 *	TRUE if there was a filename available, otherwise FALSE
 *
 */
int xfer_nextfile(HSESSION hSession, TCHAR *filename)
	{
	XD_TYPE *pX;
	XFR_SEND *pS;
	LPTSTR pszStr;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pS = (XFR_SEND *)pX->pXferStuff;
		if (pS)
			{
			if (pS->nIndex < pS->nCount)
				{
				pszStr = pS->pList[pS->nIndex].pszName;
				StrCharCopy(filename, pszStr);
				pS->nIndex += 1;
				/*
				 * TODO: decide where the memory gets freed
				 */
				return TRUE;
				}
			}
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_log_xfer
 *
 * DESCRIPTION:
 *	This function is called after a file is sent or received to place that
 *	information in the log file.
 *
 * PARAMETERS:
 *	hSession  -- the session handle
 *	sending   -- TRUE if the file was sent, otherwise FALSE
 *	theirname -- the name that was given to the other system
 *	ourname   -- the name of the file on this system
 *	result    -- the final transfer status code
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfer_log_xfer(HSESSION hSession,
				  int sending,
				  TCHAR *theirname,
				  TCHAR *ourname,
				  int result)
	{
	/*
	 * Lower Wacker does not do transfer logging.  This is here mostly as a
	 * place holder and for the eventual conversion to Upper Wacker.
	 */
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_opensendfile
 *
 * DESCRIPTION:
 *	This function is called to open a file that is to be sent to another
 *	system.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	fp           -- where to store the open file handle
 *	file_to_open -- the name of the file to open (duh!)
 *	size         -- where to save the size of the file
 *	name_to_send -- what name to send to the other system
 *	ft           -- currently unused
 *
 * RETURNS:
 *	    0 if everything is OK
 *	   -1 if an error occurred
 *	   -2 if the file was not found
 *
 */
int xfer_opensendfile(HSESSION hSession,
					 HANDLE *fp,
					 TCHAR *file_to_open,
					 long *size,
					 TCHAR *name_to_send,
					 void *ft)
	{
	DWORD dwFoo;

	*fp = (HANDLE)0;

	/*
	 * Just try an open the file
	 */
	*fp = fio_open(file_to_open, FIO_READ);

	if (*fp == NULL)
		{
		*fp = (HANDLE)0;
		return -1;
		}

	/*
	 * Got the file open, get the size
	 */
	*size = GetFileSize(fio_gethandle((ST_IOBUF *)*fp), &dwFoo);

	/*
	 * TODO: do the date and time stuff
	 */

	/*
	 * Give them a file name
	 */
	if (name_to_send != NULL)
		xfer_name_to_send(hSession, file_to_open, name_to_send);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	xfer_name_to_send
 *
 * DESCRIPTION:
 *	This function is called to modify the file name into some sort of form
 *	that should be sent over to the other side.  Kind of sounds like an
 *	exchange of captured spys at Checkpoint Charlie.
 *
 * PARAMETERS:
 *	hSession     -- the session handle
 *	local_name   -- what the name is on this system
 *	name_to_send -- where to put the processed name
 *
 * RETURNS:
 *	Nothing.
 *
 */
void xfer_name_to_send(HSESSION hSession,
					  TCHAR *local_name,
					  TCHAR *name_to_send)
	{
	TCHAR *pszStr;
	XD_TYPE *pX;
	XFR_PARAMS *pP;

	if (local_name == NULL)
		return;
	if (name_to_send == NULL)
		return;

	pX = (XD_TYPE *)sessQueryXferHdl(hSession);
	if (pX)
		{
		pP = (XFR_PARAMS *)pX->xfer_params;
		if (pP)
			{
			if (pP->fIncPaths)
				{
				StrCharCopy(name_to_send, local_name);
				}
			}
		}
	/*
	 * Otherwise, just do this
	 */
	pszStr = StrCharFindLast(local_name, TEXT('\\'));
	if (*pszStr == TEXT('\\'))
		pszStr += 1;
	StrCharCopy(name_to_send, pszStr);
	}
