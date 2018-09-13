/*	File: D:\WACKER\tdll\file_msc.c (Created: 26-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 8/18/99 10:52a $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\sf.h>
#include <tdll\tdll.h>
#include <tdll\sess_ids.h>
#include <tdll\assert.h>
#include <tdll\session.h>
#include <tdll\open_msc.h>
#include "tchar.h"

#include "file_msc.h"
#include "file_msc.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *
 *                            F I L E _ M S C . C
 *
 * This file contains functions that are needed to deal with files, names of
 * files, lists of files and just about anything else about files.
 *
 *=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

STATIC_FUNC int fmBFLinternal(void **pData,
							int *pCnt,
							LPCTSTR pszName,
							int nSubdir,
							LPCTSTR pszDirectory);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CreateFilesDirsHdl
 *
 * DESCRIPTION:
 *	This function is called to create the files and directory handle.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *
 * RETURNS:
 *	A pointer to the HFILES handle.
 */
HFILES CreateFilesDirsHdl(const HSESSION hSession)
	{
	FD_DATA *pFD;
	int nRet;

	pFD = (FD_DATA *)malloc(sizeof(FD_DATA));
	assert(pFD);
	memset(pFD, 0, sizeof(FD_DATA));

	if (pFD)
		{
		nRet = InitializeFilesDirsHdl(hSession, (HFILES)pFD);
		if (nRet)
			goto CFDHexit;
		}

	return (HFILES)pFD;

CFDHexit:
	if (pFD)
		{
		if (pFD->pszInternalSendDirectory)
			{
			free(pFD->pszInternalSendDirectory);
			pFD->pszInternalSendDirectory = NULL;
			}

		if (pFD->pszTransferSendDirectory)
			{
			free(pFD->pszTransferSendDirectory);
			pFD->pszTransferSendDirectory = NULL;
			}

		if (pFD->pszInternalRecvDirectory)
			{
			free(pFD->pszInternalRecvDirectory);
			pFD->pszInternalRecvDirectory = NULL;
			}

		if (pFD->pszTransferRecvDirectory)
			{
			free(pFD->pszTransferRecvDirectory);
			pFD->pszTransferRecvDirectory = NULL;
			}

		free(pFD);
		pFD = NULL;
		}
	return (HFILES)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	InitializeFilesDirs
 *
 * DESCRIPTION:
 *	This function is called to put the files and directorys handle into a
 *	known and safe state.
 *
 * PARAMETERS:
 *	hSession -- the session handle
 *	hFile    -- the files and directory handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
INT InitializeFilesDirsHdl(const HSESSION hSession, HFILES hFile)
	{
	FD_DATA *pFD;
	LPTSTR pszSname;
	LPTSTR pszRname;
	int nSize;
	TCHAR acDir[FNAME_LEN];

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	memset(pFD, 0, sizeof(FD_DATA));

	pszSname = pszRname = (LPTSTR)0;

	if (pFD)
		{
		pFD->hSession = hSession;

		//Changed to use working path rather than current path - mpt 8-18-99
		if ( !GetWorkingDirectory( acDir, FNAME_LEN ) )
			{
			GetCurrentDirectory(FNAME_LEN, acDir);
			}

		nSize = StrCharGetByteCount(acDir) + 1;

		pszSname = malloc(nSize);
		if (pszSname == (LPTSTR)0)
			goto IFDexit;
		pszRname = malloc(nSize);
		if (pszRname == (LPTSTR)0)
			goto IFDexit;

		if (pFD->pszInternalSendDirectory)
			{
			free(pFD->pszInternalSendDirectory);
			pFD->pszInternalSendDirectory = NULL;
			}
		pFD->pszInternalSendDirectory = pszSname;
		StrCharCopy(pFD->pszInternalSendDirectory, acDir);

		if (pFD->pszTransferSendDirectory)
			free(pFD->pszTransferSendDirectory);
		pFD->pszTransferSendDirectory = (LPTSTR)0;

		if (pFD->pszInternalRecvDirectory)
			{
			free(pFD->pszInternalRecvDirectory);
			pFD->pszInternalRecvDirectory = NULL;
			}
		pFD->pszInternalRecvDirectory = pszRname;
		StrCharCopy(pFD->pszInternalRecvDirectory, acDir);

		if (pFD->pszTransferRecvDirectory)
			free(pFD->pszTransferRecvDirectory);
		pFD->pszTransferRecvDirectory = (LPTSTR)0;
		}

	return 0;

IFDexit:
	if (pszSname)
		{
		free(pszSname);
		pszSname = NULL;
		}
	if (pszRname)
		{
		free(pszRname);
		pszRname = NULL;
		}

	return FM_ERR_NO_MEM;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	LoadFilesDirs
 *
 * DESCRIPTION:
 *	This function is called to read whatever values are in the session file
 *	into the files and directorys handle
 *
 * PARAMETERS:
 *	hFile -- the files and directory handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
INT LoadFilesDirsHdl(HFILES hFile)
	{
	INT nRet = 0;
	FD_DATA *pFD;
	long lSize;
	LPTSTR pszStr;

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	if (pFD == (FD_DATA *)0)
		return FM_ERR_BAD_HANDLE;

	InitializeFilesDirsHdl(pFD->hSession, hFile);

	if (nRet == 0)
		{
		lSize = 0;
		sfGetSessionItem(sessQuerySysFileHdl(pFD->hSession),
						SFID_XFR_SEND_DIR,
						&lSize,
						NULL);
		if (lSize != 0)
			{
			pszStr = (LPTSTR)malloc(lSize);
			if (pszStr)
				{
				sfGetSessionItem(sessQuerySysFileHdl(pFD->hSession),
								SFID_XFR_SEND_DIR,
								&lSize,
								pszStr);
				if (pFD->pszTransferSendDirectory)
					{
					free(pFD->pszTransferSendDirectory);
					pFD->pszTransferSendDirectory = NULL;
					}
				pFD->pszTransferSendDirectory = pszStr;
				}
			else
				{
				nRet = FM_ERR_NO_MEM;
				}
			}
		}

	if (nRet == 0)
		{
		lSize = 0;
		sfGetSessionItem(sessQuerySysFileHdl(pFD->hSession),
						SFID_XFR_RECV_DIR,
						&lSize,
						NULL);
		if (lSize != 0)
			{
			pszStr = (LPTSTR)malloc(lSize);
			if (pszStr)
				{
				sfGetSessionItem(sessQuerySysFileHdl(pFD->hSession),
								SFID_XFR_RECV_DIR,
								&lSize,
								pszStr);
				if (pFD->pszTransferRecvDirectory)
					{
					free(pFD->pszTransferRecvDirectory);
					pFD->pszTransferRecvDirectory = NULL;
					}
				pFD->pszTransferRecvDirectory = pszStr;
				}
			else
				{
				nRet = FM_ERR_NO_MEM;
				}
			}
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DestroyFilesDirsHdl
 *
 * DESCRIPTION:
 *	This function is called to free all the memory that is in a Files and
 *	Directorys handle.  Gone.  History.  Toast.
 *
 * PARAMETERS:
 *	hFile -- the files and directorys handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
INT DestroyFilesDirsHdl(const HFILES hFile)
	{
	INT nRet = 0;
	FD_DATA *pFD;

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	if (pFD == (FD_DATA *)0)
		return FM_ERR_BAD_HANDLE;

	if (pFD->pszInternalSendDirectory)
		{
		free(pFD->pszInternalSendDirectory);
		pFD->pszInternalSendDirectory = NULL;
		}
	if (pFD->pszTransferSendDirectory)
		{
		free(pFD->pszTransferSendDirectory);
		pFD->pszTransferSendDirectory = NULL;
		}
	if (pFD->pszInternalRecvDirectory)
		{
		free(pFD->pszInternalRecvDirectory);
		pFD->pszInternalRecvDirectory = NULL;
		}
	if (pFD->pszTransferRecvDirectory)
		{
		free(pFD->pszTransferRecvDirectory);
		pFD->pszTransferRecvDirectory = NULL;
		}
	free(pFD);
	pFD = NULL;
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SaveFilesDirsHdl
 *
 * DESCRIPTION:
 *	This function is called to save out to the session file all of the data
 *	that has changed in the Files and Directorys handle.
 *
 * PARAMETERS:
 *	hFile -- the files and directorys handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code.
 */
INT SaveFilesDirsHdl(const HFILES hFile)
	{
	FD_DATA *pFD;
	long lSize;

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	if (pFD == (FD_DATA *)0)
		return FM_ERR_BAD_HANDLE;

	if (pFD->pszTransferSendDirectory)
		{
		lSize = StrCharGetByteCount(pFD->pszTransferSendDirectory) + 1;
		sfPutSessionItem(sessQuerySysFileHdl(pFD->hSession),
						SFID_XFR_SEND_DIR,
						lSize,
						pFD->pszTransferSendDirectory);
		}

	if (pFD->pszTransferRecvDirectory)
		{
		lSize = StrCharGetByteCount(pFD->pszTransferRecvDirectory) + 1;
		sfPutSessionItem(sessQuerySysFileHdl(pFD->hSession),
						SFID_XFR_RECV_DIR,
						lSize,
						pFD->pszTransferRecvDirectory);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	filesQuerySendDirectory
 *
 * DESCRIPTION:
 *	This function returns a pointer to the current default transfer send
 *	directory.
 *
 * PARAMETERS:
 *	hFile -- the files and directorys handle
 *
 * RETURNS:
 *	A pointer to the current default transfer send directory
 *
 */
LPCTSTR filesQuerySendDirectory(HFILES hFile)
	{
	FD_DATA *pFD;

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	assert(pFD->pszInternalSendDirectory);

	if (pFD->pszTransferSendDirectory == (LPTSTR)0)
		return (LPCTSTR)pFD->pszInternalSendDirectory;

	if (StrCharGetStrLength(pFD->pszTransferSendDirectory) == 0)
		return (LPCTSTR)pFD->pszInternalSendDirectory;

	return (LPCTSTR)pFD->pszTransferSendDirectory;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	filesQueryRecvDirectory
 *
 * DESCRIPTION:
 *	This function returns a pointer to the current default transfer recv
 *	directory.
 *
 * PARAMETERS:
 *	hFile -- the files and directorys handle
 *
 * RETURNS:
 *	A pointer to the current default recv directory
 *
 */
LPCTSTR filesQueryRecvDirectory(HFILES hFile)
	{
	FD_DATA *pFD;

	pFD = (FD_DATA *)hFile;
	assert(pFD);
	assert(pFD->pszInternalRecvDirectory);

	if (pFD->pszTransferRecvDirectory == (LPTSTR)0)
		return (LPCTSTR)pFD->pszInternalRecvDirectory;

	if (StrCharGetStrLength(pFD->pszTransferRecvDirectory) == 0)
		return (LPCTSTR)pFD->pszInternalRecvDirectory;

	return (LPCTSTR)pFD->pszTransferRecvDirectory;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	filesSetSendDirectory
 *
 * DESCRIPTION:
 *	This function is called to change (maybe) the current default sending
 *	directory.
 *
 * PARAMETERS:
 *	hFile       -- the files and directorys handle
 *	pszDir      -- pointer to the new directory path
 *
 * RETURNS:
 *	Nothing.
 *
 */
VOID filesSetSendDirectory(HFILES hFile, LPCTSTR pszDir)
	{
	LPTSTR pszTmp;
	FD_DATA *pFD;

	pFD = (FD_DATA *)hFile;
	assert(pFD);

	pszTmp = (LPTSTR)malloc(StrCharGetByteCount(pszDir) + 1);
	assert(pszTmp);
	if (pszTmp == NULL)
		return;
	StrCharCopy(pszTmp, pszDir);
	if (pFD->pszTransferSendDirectory)
		{
		free(pFD->pszTransferSendDirectory);
		pFD->pszTransferSendDirectory = NULL;
		}
	pFD->pszTransferSendDirectory = pszTmp;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	filesSetRecvDirectory
 *
 * DESCRIPTION:
 *	This function is called to change (maybe) the current default receiving
 *	directory.
 *
 * PARAMETERS:
 *	hFile       -- the files and directorys handle
 *	pszDir      -- pointer to the new directory path
 *
 * RETURNS:
 *	Nothing.
 *
 */
VOID filesSetRecvDirectory(HFILES hFile, LPCTSTR pszDir)
	{
	LPTSTR pszTmp;
	FD_DATA *pFD;

	pFD = (FD_DATA *)hFile;
	assert(pFD);

	pszTmp = (LPTSTR)malloc(StrCharGetByteCount(pszDir) + 1);
	assert(pszTmp);
	if (pszTmp == NULL)
		return;
	StrCharCopy(pszTmp, pszDir);
	if (pFD->pszTransferRecvDirectory)
		{
		free(pFD->pszTransferRecvDirectory);
		pFD->pszTransferRecvDirectory = NULL;
		}
	pFD->pszTransferRecvDirectory = pszTmp;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fileBuildFileList
 *
 * DESCRIPTION:
 *	This function is called to build a list of all the files that match
 *	a given mask.
 *
 * PARAMETERS:
 *	pData        -- pointer to where to store the pointer to the data block
 *	pCnt         -- pointer to where the item count is returned
 *	pszName      -- file name mask used to build list
 *	nSubdir      -- if TRUE, search subdirectorys
 *	pszDirectory -- directory to start search from
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */
int fileBuildFileList(void **pData,
					int *pCnt,
					LPCTSTR pszName,
					int nSubdir,
					LPCTSTR pszDirectory)
	{
	int nRet = 0;
	void *pLocalData = NULL;
	int nLocalCnt = 0;
	LPTSTR pszStr;
	LPTSTR *pszArray;
	TCHAR pszLocalDirectory[FNAME_LEN];

	/* Make sure the directory string terminates correctly */
	StrCharCopy(pszLocalDirectory, pszDirectory);

	pszStr = StrCharLast(pszLocalDirectory);
	if (*pszStr != TEXT('\\'))
		{
		/* Make sure the last character is a "\" */
		StrCharCat(pszStr, TEXT("\\"));
		}

	pLocalData = malloc(sizeof(LPTSTR) * FM_CHUNK_SIZE);
	if (pLocalData == NULL)
		nRet = FM_ERR_NO_MEM;

	if (nRet == 0)
		{
		nRet = fmBFLinternal(&pLocalData,
							&nLocalCnt,
							pszName,
							nSubdir,
							pszLocalDirectory);
		}

	if (nRet == 0)
		{
		/* OK, no problem */
		*pData = pLocalData;
		*pCnt = nLocalCnt;
		}
	else
		{
		/* Error, clean up first and then go away */
		if (pLocalData)
			{
			pszArray = (LPTSTR *)pLocalData;
			while (--nLocalCnt >= 0)
				{
				free(*pszArray++);
				*pszArray = NULL;
				}
			free(pLocalData);
			pLocalData = NULL;
			}
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fmBFLinternal
 *
 * DESCRIPTION:
 *	This is the internal function that the previous function calls to do the
 *	actual work.
 *
 * PARAMETERS:
 *	The same as above.
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code
 *
 */
STATIC_FUNC int fmBFLinternal(void **pData,
							int *pCnt,
							LPCTSTR pszName,
							int nSubdir,
							LPCTSTR pszDirectory)
	{
	int nRet = 0;
	int nSize;
	WIN32_FIND_DATA stF;
	HANDLE sH;
	LPTSTR pszBuildName;
	LPTSTR pszStr;
	LPTSTR *pszArray;

	pszBuildName = (LPTSTR)malloc(FNAME_LEN * sizeof(TCHAR));
	if (pszBuildName == NULL)
		{
		nRet = FM_ERR_NO_MEM;
		goto fmBFLexit;
		}

	StrCharCopy(pszBuildName, pszDirectory);
	StrCharCat(pszBuildName, pszName);

	sH = FindFirstFile(pszBuildName, &stF);
	if (sH != INVALID_HANDLE_VALUE)
		{
		/* Handle is OK, we have something to work on */
		do {
			/* Is it a directory ?  If it is, skip it until later */
			if (stF.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			/* Must be a file. */
			if (stF.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;
			if (stF.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
				continue;
			/* Add the file to the list */
			if ((*pCnt > 0) && ((*pCnt % FM_CHUNK_SIZE) == 0))
				{
				/* realloc the chunk */
				nSize = *pCnt + FM_CHUNK_SIZE;
				*pData = realloc(*pData, nSize * sizeof(LPTSTR) );
				if (*pData == NULL)
					{
					nRet = FM_ERR_NO_MEM;
					goto fmBFLexit;
					}
				}
			nSize = StrCharGetByteCount(pszDirectory) +
					StrCharGetByteCount(stF.cFileName);
			nSize += 1;
			nSize *= sizeof(TCHAR);
			pszStr = (LPTSTR)malloc(nSize);
			if (pszStr == (LPTSTR)0)
				{
				nRet = FM_ERR_NO_MEM;
				goto fmBFLexit;
				}
			StrCharCopy(pszStr, pszDirectory);
			StrCharCat(pszStr, stF.cFileName);
			pszArray = (LPTSTR *)*pData;
			pszArray[*pCnt] = pszStr;
			*pCnt += 1;
		} while (FindNextFile(sH, &stF));
		FindClose(sH);
		sH = INVALID_HANDLE_VALUE;
		}
	else
		{
		nRet = FM_ERR_BAD_HANDLE;
		goto fmBFLexit;
		}

	if (nSubdir)
		{
		StrCharCopy(pszBuildName, pszDirectory);
		StrCharCat(pszBuildName, TEXT("*.*"));	/* This may need to change */
		sH = FindFirstFile(pszBuildName, &stF);
		if (sH != INVALID_HANDLE_VALUE)
			{
			/* Handle is OK, we have something to work on */
			do {
				/* Is it a directory ?  If it is, go recursive */
				if (stF.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
					if (StrCharCmp(stF.cFileName, TEXT(".")) == 0)
						continue;
					if (StrCharCmp(stF.cFileName, TEXT("..")) == 0)
						continue;
					StrCharCopy(pszBuildName, pszDirectory);
					StrCharCat(pszBuildName, stF.cFileName);
					StrCharCat(pszBuildName, TEXT("\\"));
					fmBFLinternal(pData,
								pCnt,
								pszName,
								nSubdir,
								pszBuildName);
					}
			} while (FindNextFile(sH, &stF));
			FindClose(sH);
			sH = INVALID_HANDLE_VALUE;
			}
		else
			{
			nRet = FM_ERR_BAD_HANDLE;
			goto fmBFLexit;
			}
		}

fmBFLexit:
	/* NOTE: all returns must come thru here */
	if (sH != INVALID_HANDLE_VALUE)
		FindClose(sH);

	if (pszBuildName)
		{
		free(pszBuildName);
		pszBuildName = NULL;
		}

	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fileFinalizeName
 *
 * DESCRIPTION:
 *	This function takes a possibly incomplete file name and trys to convert
 *	it to a fully qualified name, if possible, based upon the mode request.
 *
 * PARAMETERS:
 *	hSession	--	the almost universal session handle
 *	pszOldname	--	a pointer to the old name string
 *  pszOlddir	--	a pointer to an optional path
 *	pszNewname	--	a pointer to where the new string should go
 *	nMode		--	what should be done to the string, currently ignored
 *
 * RETURNS:
 *	TRUE if a conversion was completed and copied, FALSE if nothing was copied.
 */

static LPTSTR pszBS = TEXT("\\");

int fileFinalizeName(LPTSTR pszOldname,
					LPTSTR pszOlddir,
					LPTSTR pszNewname,
					const size_t cb)
	{
	TCHAR *pachFile;
	TCHAR achCurDir[MAX_PATH];

	assert(cb);
	assert(pszNewname);
	assert(pszOldname);
	achCurDir[0] = TEXT('\0');

	// If we're given a directory, save the current directory and
	// set the current directory to the one given.
	//
	if (pszOlddir && *pszOlddir != TEXT('\0'))
		{
		if (GetCurrentDirectory(sizeof(achCurDir), achCurDir) == 0)
			{
			assert(0);
			return FALSE;
			}

		if (SetCurrentDirectory(pszOlddir) == FALSE)
			{
			assert(0);
			return FALSE;
			}
		}

	// This function does the correct job of building a full path
	// name and works with UNC names.
	//
	if (GetFullPathName(pszOldname, cb, pszNewname, &pachFile) == 0)
		{
		assert(0);
		return FALSE;
		}

	// Restore the current directory we saved above.
	//
	if (achCurDir[0] != TEXT('\0'))
		{
		if (SetCurrentDirectory(achCurDir) == FALSE)
			{
			assert (0);
			return FALSE;
			}
		}

	return TRUE;

	#if 0 // DEADWOOD - mrw:6/13/95 - This code sucks!
	{
	int nRet = FALSE;
	LPTSTR pszPtr;
	LPTSTR pszStr;
	LPTSTR pszFoo;
	TCHAR acWorking[FNAME_LEN];
	TCHAR acDevice[32];
	TCHAR acDirectory[FNAME_LEN];
	TCHAR acName[FNAME_LEN];

	TCHAR_Fill(acWorking, TEXT('\0'), sizeof(acWorking) / sizeof(TCHAR));
	TCHAR_Fill(acDevice, TEXT('\0'), sizeof(acDevice) / sizeof(TCHAR));
	TCHAR_Fill(acDirectory, TEXT('\0'), sizeof(acDirectory) / sizeof(TCHAR));
	TCHAR_Fill(acName, TEXT('\0'), sizeof(acName) / sizeof(TCHAR));

	/*
	 * Slice and dice the input
	 */

	if ((pszOlddir != NULL) && (StrCharGetStrLength(pszOlddir) != 0))
		{
		pszStr = pszOldname;
		if (*pszStr != TEXT('\\'))
			{
			pszStr = StrCharNext(pszStr);
			if (*pszStr != TEXT(':'))
				{
				/* Oldname is probably just a name */
				StrCharCopy(acWorking, pszOlddir);
				pszStr = StrCharLast(acWorking);
				if (*pszStr != TEXT('\\'))
					{
					StrCharCat(acWorking, pszBS);
					}
				StrCharCat(acWorking, pszOldname);
				}
			}
		}

	if (StrCharGetStrLength(acWorking) == 0)
		StrCharCopy(acWorking, pszOldname);

	/* First try the device spec */
	for (pszPtr = acWorking;
		(*pszPtr != TEXT('\0')) && (*pszPtr != TEXT('\\'));
		pszPtr = StrCharNext(pszPtr))
		{
		if (*pszPtr == TEXT(':'))
			break;
		}

	if (*pszPtr == TEXT(':'))
		{
		/* Copy up thru the ':' */
        if (pszPtr >= (LPTSTR)acWorking)
		    MemCopy(acDevice, acWorking, pszPtr - (LPTSTR)acWorking + 1);
		/* Having a device implys a root path */
		acDirectory[0] = TEXT('\\');
		acDirectory[1] = TEXT('\0');

		pszPtr = StrCharNext(pszPtr);
		}
	else
		{
		pszPtr = acWorking;
		}

	/* Split the name and the path up */
	pszStr = StrCharFindLast(acWorking, TEXT('\\'));

	if (acDevice[0] == TEXT('\0'))
		if ((acDirectory[0] == TEXT('\\')) || (*pszPtr == TEXT('\\')))
			{
			/* TODO: figure out how to replace getdrive */
			// acDevice[0] = (UCHAR)(TEXT('A') + _getdrive() - 1);
			acDevice[0] = TEXT('C');
			acDevice[1] = TEXT(':');
			acDevice[2] = TEXT('\0');
			}

	pszFoo = acDirectory;
	if ((*pszFoo == TEXT('\\')) && (*pszPtr != TEXT('\\')))
		pszFoo = StrCharNext(pszFoo);
    if (pszStr > pszPtr)
        MemCopy(pszFoo, pszPtr, pszStr - pszPtr);
	pszPtr = pszStr;

	if (*pszStr == TEXT('\\'))
		pszStr = StrCharNext(pszStr);
	pszFoo = pszStr;
	while (*pszStr != TEXT('\0'))
		pszStr = StrCharNext(pszStr);
    if (pszStr > pszFoo)
        MemCopy(acName, pszFoo, pszStr - pszFoo);

	/* Moved down from the code that was removed */

	GetCurrentDirectory(sizeof(acWorking) / sizeof(TCHAR), acWorking);
	pszStr = acWorking;

	/* Build up the name */

	*pszNewname = TEXT('\0');

	// Don't try to put a drive name on a UNC file name.
	//
	if (acDirectory[1] != TEXT('\\') && acDirectory[1] != TEXT('/'))
		{
		StrCharCat(pszNewname, acDevice);
		}

	if ((acDirectory[0] != TEXT('\\')) && (acDevice[0] == TEXT('\0')))
		{
		StrCharCat(pszNewname, pszStr);
		pszStr = StrCharLast(pszNewname);
		if (*pszStr != TEXT('\\'))
			StrCharCat(pszNewname, pszBS);
		}
	StrCharCat(pszNewname, acDirectory);
	pszStr = StrCharLast(pszNewname);
	if (*pszStr != TEXT('\\'))
		StrCharCat(pszNewname, pszBS);

	StrCharCat(pszNewname, acName);

	nRet = TRUE;

	return nRet;
	#endif
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * FUNCTION:
 *	mfFinalizeDIR
 *
 * DESCRIPTION:
 *	This function is called to clean up a directory name.  At the present time
 *	it doesn't do very much.
 *
 * PARAMETERS:
 *	hSession	--	the almost universal session handle
 *	pszOldname	--	a pointer to the old directory name
 *	pszNewname	--	a pointer to where the new string should go
 *
 * RETURNS:
 *	TRUE if a copy was completed, FALSE if nothing was copied.
 */
int fileFinalizeDIR(HSESSION hSession,
					LPTSTR pszOldname,
					LPTSTR pszNewname)
	{
	LPTSTR pszPtr;
	LPTSTR pszFoo;


	StrCharCopy(pszNewname, pszOldname);
	pszPtr = StrCharNext(pszNewname);
	pszFoo = StrCharNext(pszPtr);

	if ((StrCharGetStrLength(pszNewname) == 2) &&
		(*pszPtr == TEXT(':')))
		{
		StrCharCat(pszNewname, pszBS);
		}
	else if ((StrCharGetStrLength(pszNewname) == 3) &&
			(*pszPtr == TEXT(':')) &&
			(*pszFoo == TEXT('\\')))
		{
		/* Do nothing */
		}
	else
		{
		pszPtr = StrCharLast(pszNewname);
		if (*pszPtr == TEXT('\\'))
			*pszPtr = TEXT('\0');
		}

	return 1;
	}

#if defined(BMP_FROM_FILE)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	fileReadBitmapFromFile
 *
 * DESCRIPTION:
 *	This function takes a filename, opens the file and attempts to interpret
 *	the file as a bitmap file, turning it into a bitmap.
 *
 * PARAMETERS:
 *	hDC     -- device context used to create the bitmap
 *	pszName	-- the name of the file
 *
 * RETURNS:
 *	A bitmap handle or NULL.
 *
 */
HBITMAP fileReadBitmapFromFile(HDC hDC, LPTSTR pszName, int fCmp)
	{
	HBITMAP hBmp = (HBITMAP)0;
	DWORD dwRead;
	DWORD dwSize;
	HANDLE hfbm;
	int hcbm;
	OFSTRUCT stOF;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	BITMAPINFO *lpbmi;
	VOID *lpvBits;

	lpbmi = NULL;
	lpvBits = NULL;

	if (fCmp)
		{
		memset(&stOF, 0, sizeof(OFSTRUCT));

		stOF.cBytes = sizeof(OFSTRUCT);

		hcbm = LZOpenFile(pszName,
						&stOF,
						OF_READ);
		if (hcbm < 0)
			return hBmp;
		}
	else
		{
		hfbm = CreateFile(pszName,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_READONLY,
						NULL);
		if (hfbm == INVALID_HANDLE_VALUE)
			return hBmp;
		}

	/* Retrieve the BITMAPFILEHEADER structure */
	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	if (fCmp)
		{
		dwRead = 0;
		dwRead = LZRead(hcbm,
						(unsigned char *)&bmfh,
						sizeof(BITMAPFILEHEADER));
		/* this is necessary because of a garbage return value */
		dwRead = sizeof(BITMAPFILEHEADER);
		}
	else
		{
		ReadFile(hfbm,
				&bmfh,
				sizeof(BITMAPFILEHEADER),
				&dwRead,
				NULL);
		}
	if (dwRead != sizeof(BITMAPFILEHEADER))
		goto fError;

	/* Retrieve the BITMAPINFOHEADER structure */
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	if (fCmp)
		{
		dwRead = 0;
		dwRead = LZRead(hcbm,
						(unsigned char *)&bmih,
						sizeof(BITMAPINFOHEADER));
		/* this is necessary because of a garbage return value */
		dwRead = sizeof(BITMAPINFOHEADER);
		}
	else
		{
		ReadFile(hfbm,
				&bmih,
				sizeof(BITMAPINFOHEADER),
				&dwRead,
				NULL);
		}
	if (dwRead != sizeof(BITMAPINFOHEADER))
		goto fError;

	/* allocate space for the BITMAPINFO structure */
	dwSize = sizeof(BITMAPINFOHEADER) +
				((1 << bmih.biBitCount) * sizeof(RGBQUAD));

	lpbmi = malloc(dwSize);
	if (lpbmi == NULL)
		goto fError;

	/* load BITMAPINFOHEADER into the BITMAPINFO structure */
	lpbmi->bmiHeader.biSize = bmih.biSize;
	lpbmi->bmiHeader.biWidth = bmih.biWidth;
	lpbmi->bmiHeader.biHeight = bmih.biHeight;
	lpbmi->bmiHeader.biPlanes = bmih.biPlanes;
	lpbmi->bmiHeader.biBitCount = bmih.biBitCount;
	lpbmi->bmiHeader.biCompression = bmih.biCompression;
	lpbmi->bmiHeader.biSizeImage = bmih.biSizeImage;
	lpbmi->bmiHeader.biXPelsPerMeter = bmih.biXPelsPerMeter;
	lpbmi->bmiHeader.biYPelsPerMeter = bmih.biYPelsPerMeter;
	lpbmi->bmiHeader.biClrUsed = bmih.biClrUsed;
	lpbmi->bmiHeader.biClrImportant = bmih.biClrImportant;

	/* read the color table */
	dwSize = (1 << bmih.biBitCount) * sizeof(RGBQUAD);
	if (fCmp)
		{
		dwRead = 0;
		dwRead = LZRead(hcbm,
						(unsigned char *)lpbmi->bmiColors,
						dwSize);
		/* this is necessary because of a garbage return value */
		dwRead = dwSize;
		}
	else
		{
		ReadFile(hfbm,
				lpbmi->bmiColors,
				dwSize,
				&dwRead,
				NULL);
		}
	if (dwSize != dwRead)
		goto fError;

	/* allocate memory for the bitmap data */
	dwSize = bmfh.bfSize - bmfh.bfOffBits;
	lpvBits = malloc(dwSize);
	if (lpvBits == NULL)
		goto fError;

	/* read in the bitmap data */
	if (fCmp)
		{
		dwRead = 0;
		dwRead = LZRead(hcbm,
						lpvBits,
						dwSize);
		/* this is necessary because of a garbage return value */
		dwRead = dwSize;
		}
	else
		{
		ReadFile(hfbm,
				lpvBits,
				dwSize,
				&dwRead,
				NULL);
		}
	if (dwSize != dwRead)
		goto fError;

	/* create the bitmap handle */
	hBmp = CreateDIBitmap(hDC,
						&bmih,
						CBM_INIT,
						lpvBits,
						lpbmi,
						DIB_RGB_COLORS);

	/* either it worked or it didn't */

fError:
	/* Clean up everything here */

	if (lpbmi != NULL)
		{
		free(lpbmi);
		lpbmi = NULL;
		}

	if (lpvBits != NULL)
		{
		free(lpvBits);
		lpvBits = NULL;
		}

	if (fCmp)
		{
		LZClose(hcbm);
		}
	else
		{
		CloseHandle(hfbm);
		}

	return hBmp;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION: GetFileSizeFromName
 *
 * DESCRIPTION:
 *	Returns the size of a named file. (The GetFileSize Win32 API call
 *	requires the file to be open, this doesn't).
 *	Note: the WIN32 API is structured to support 64 bits file size values
 *		  so this may need to be updated at some point.
 *
 * PARAMETERS:
 *	pszName -- the name of the file.
 *	pulFileSize -- Pointer to the var. that receives the file size.
 *				   (If NULL, this function can be used to test for the
 *					existense of a file).
 *
 * RETURNS:
 *	TRUE if file is found, FALSE if not
 */
int GetFileSizeFromName(TCHAR *pszName, unsigned long * const pulFileSize)
	{
	WIN32_FIND_DATA stFData;
	HANDLE hFind;
	int fReturnValue = FALSE;

	hFind = FindFirstFile(pszName, &stFData);
	if (hFind != INVALID_HANDLE_VALUE)
		{
		DWORD dwMask;

		/* This is just a guess.  If you need to change it, do so. */
		dwMask = FILE_ATTRIBUTE_DIRECTORY |
				 FILE_ATTRIBUTE_HIDDEN |
				 FILE_ATTRIBUTE_SYSTEM;

		if ((stFData.dwFileAttributes & dwMask) == 0)
			{
			fReturnValue = TRUE;
			// Strictly speaking, file sizes can now be 64 bits.
			assert(stFData.nFileSizeHigh == 0);
			if (pulFileSize)
				*pulFileSize = (unsigned long)stFData.nFileSizeLow;
			}
		FindClose(hFind);
		}
	return fReturnValue;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SetFileSize
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
int SetFileSize(const TCHAR *pszName, unsigned long ulFileSize)
    {
	HANDLE hFile;
    int     nRet = -1;

	/* Yes, we need to open the file */
	hFile = CreateFile(pszName,
						GENERIC_WRITE,
						FILE_SHARE_WRITE,
						0,
						OPEN_EXISTING,
						0,
						0);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1;								/* No such file */

    if (SetFilePointer(hFile, ulFileSize, NULL, FILE_BEGIN) == ulFileSize)
        {
        if (SetEndOfFile(hFile))
            nRet = 0;
        }

    CloseHandle(hFile);

    return nRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	GetModuleDirectoryName
 *
 * DESCRIPTION:
 *	This function corresponds to the Windows call "GetModuleFileName", except
 *	that it only returns the directory name.  It does this by actually getting
 *	the file name and then replacing the final "\" with a NULL.
 *
 * PARAMETERS:
 *	The same as what "GetModuleFileName" wants.
 *
 * RETURNS:
 *	The same as what "GetModuleFileName" returns.
 *
 */
int GetModuleDirectoryName(HINSTANCE hInst, LPTSTR pszName, int dLen)
	{
	int nRet;
	LPTSTR pszPtr;
	LPTSTR pszPrv;

	nRet = GetModuleFileName(hInst, pszName, dLen);

	if (nRet != 0)
		{
		pszPrv = NULL;
		for (pszPtr = pszName;
			*pszPtr != TEXT('\0');
			pszPtr = StrCharNext(pszPtr))
			{
			if (*pszPtr == TEXT('\\'))
				pszPrv = pszPtr;
			}
		if (pszPrv != NULL)
			*pszPrv = TEXT('\0');
		nRet = StrCharGetByteCount(pszName);
		}
	return nRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 * ValidateFileName
 *
 * DESCRIPTION:
 * Determine whether a file pathname is valid by attempting to open it.
 *
 * PARAMETERS:
 * 
 *      LPSTR pszName   -  the name/pathname
 *
 * RETURNS:
 *
 *		0,  if a file with the specified name could not be opened/created,
 *      1,  if it could.
 *
 */
int  ValidateFileName(LPSTR pszName)
	{
	HANDLE  hfile = 0;


	if (GetFileSizeFromName(pszName, NULL))
		{
	    hfile = CreateFile(pszName,
							GENERIC_READ,
							FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);
		}
	else
		{
	    hfile = CreateFile(pszName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							CREATE_NEW,
							FILE_FLAG_DELETE_ON_CLOSE,
							NULL);
		}

	if(hfile != INVALID_HANDLE_VALUE)
		{
		CloseHandle(hfile);
		return(1);
		}
	else
		{
		return(0);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 */
