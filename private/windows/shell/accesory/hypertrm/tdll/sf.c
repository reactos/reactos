/*      File: D:\WACKER\tdll\sf.c (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#include <shlobj.h>
#pragma hdrstop

#include <stdio.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "stdtyp.h"
#include "tdll.h"
#include "tchar.h"
#include "sf.h"
#include "mc.h"

typedef int int32;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 * DATA SECTION
 *
 * This section contains all of the static data that the DLL uses.  Once this
 * data is exhausted, the DLL can do no more.  It just returns errors.  This
 * means that it is important to close or release any and all session files
 * that get opened.
 *
 */

struct stDllSessionFileIndexItem
	{
	int32       uIndex; 		/* Index of the item */
	int32		dwSize; 		/* Size of the item */
	int32		dwOffset;		/* Offset into data block of the item */
	};

typedef	struct stDllSessionFileIndexItem stDSII;
typedef stDSII *pstDSII;

struct stDllSessionFilePointer
	{
	int	uBusy;			            /* TRUE means item is in use */
	int	fOpen;			            /* TRUE means open session file */
	int	uChanged;		            /* TRUE means something is different */

	TCHAR *hFilename;		        /* Memory containing the file name */

	/* These make up the index */
	int	uItemCount; 	            /* Current items in the index */
	int	uItemsAlloc;	            /* Max items in current space */
	pstDSII	hIndex; 		        /* Memory allocated for index */

	/* This is the data segment */
	int  dwDataUsed; 	/* Amount used in data block */
	int	 dwDataSize; 	/* Current sizeof the data block  */
	BYTE *hData;		/* Memory allocated for file data */
	};

typedef	struct stDllSessionFilePointer stDSFP;
typedef stDSFP *pstDSFP;

#define ROUND_UP(x) 	((x+0xFFFL)&(~0x0FFFL))

// used for testing #define	ROUND_UP(x)		((x+0x3F)&(~0x03F))

#define	SESS_FILE_MAX	64

static stDSFP asSessionFiles[SESS_FILE_MAX];

#define HDR_SIZE	256

TCHAR pszHdr[HDR_SIZE] =
	{
	TEXT("HyperTerminal 1.0 -- HyperTerminal data file\r\n")
	TEXT("Please do not attempt to modify this file directly.\r\n")
	TEXT("\r\n\r\n")
	TEXT("\x1A")
	};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfAddToIndex
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *	The offset of the item in the index array, or -1.
 */
int32 sfAddToIndex(const int uNdx,
				 const int32 sID,
				 const int32 dwSZ,
				 const int32 dwOffset)
	{
	int nRv = -1;
	int nCnt;
	int low, mid, high;
	int found;
	pstDSFP pD;
	pstDSII	pI;
	pstDSII pIx;
	pstDSII pIy;

	pD = &asSessionFiles[uNdx];

	if (pD->uItemCount >= pD->uItemsAlloc)
		{
#if defined(DEBUG_OUTPUT)
		OutputDebugString("Index expanded\r\n");
#endif
		/*
		 * We need to get a bigger chunk of memory
		 */
		if (pD->hIndex == 0)
			{
			pD->uItemsAlloc = 64;
			pD->hIndex = (pstDSII)malloc(sizeof(stDSII) * 64);
			}
		else
			{
			pD->uItemsAlloc *= 2;
			pD->hIndex = (pstDSII)
				realloc(pD->hIndex, (size_t)pD->uItemsAlloc * sizeof(stDSII));
			}
		}

	if (pD->hData == 0)
		{
		pD->hData = (BYTE *)malloc(ROUND_UP(4096));
		memset(pD->hData, 0, HDR_SIZE);
		pD->dwDataSize = ROUND_UP(4096);
		pD->dwDataUsed = HDR_SIZE;
		}

	/*
	 * Find where the item goes
	 */
	pI = pD->hIndex;

	found = FALSE;
    pIx = 0;
	low = 0;
	high = pD->uItemCount - 1;
    mid = high/2;

	if (pD->uItemCount > 0)
		{
		while (low <= high)
			{
			mid = (low + high) / 2;
			pIx = pI + mid;

			if (sID < pIx->uIndex)
				{
				high = mid - 1;
				}
			else if (sID > pIx->uIndex)
				{
				low = mid + 1;
				}
			else
				{
				/* found a match */
				found = TRUE;
				break;
				}
			}
		}

	if (found)
		{
		if (dwSZ != 0)
			{
			/*
			 * Special case.  Preserve the old values so that we can
			 * adjust the data section for the replacement value.
			 */
			pIx->dwSize = dwSZ;
			pIx->dwOffset = dwOffset;
			}
		nRv = mid;
		}
	else
		{
		/*
		 * The problem with a binary search is that it is unclear where
		 * you are if no match occurs.  So we do it the old way.
		 */
		pIx = pI;
		nCnt = 0;
		for (;;)
			{
			/* TODO: switch to binary search */
			if (nCnt >= pD->uItemCount)
				{
				/*
				 * We have gone past the end of the list
				 */
				pIx->uIndex = sID;
				pIx->dwSize = dwSZ;
				pIx->dwOffset = dwOffset;
				nRv = nCnt;
				break;
				}
			else if (pIx->uIndex >= sID)
				{
				if (pIx->uIndex > sID)
					{
					/*
					 * Slide the remaining items down by one
					 */

					pIy = pI + pD->uItemCount;
					while (pIy > pIx)
						{
						*pIy = *(pIy - 1);
						pIy -= 1;
						}

				#if 0
					/* Don, would this work better? */

					_fmemmove(pIx+1, pIx,
						(pD->uItemCount - nCnt) * sizeof(stDSII));

				#endif

					pIx->uIndex = sID;
					pIx->dwSize = dwSZ;
					pIx->dwOffset = dwOffset;
					}
				else		/* == */
					{
					if (dwSZ != 0)
						{
						/*
						 * Special case.  Preserve the old values so that we can
						 * adjust the data section for the replacement value.
						 */
						pIx->dwSize = dwSZ;
						pIx->dwOffset = dwOffset;
						}
					pD->uItemCount -= 1;
					}

				nRv = (int)(pIx - pI);
				break;
				}
			pIx += 1;
			nCnt += 1;
			}
		pD->uItemCount += 1;
		}

	return nRv;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateSysFileHdl
 *
 * DESCRIPTION:
 *	Creates a session file handle
 *
 * ARGUMENTS:
 *	none
 *
 * RETURNS:
 *	SF_HANDLE or < 0 for error
 *
 */
SF_HANDLE CreateSysFileHdl(void)
	{
	int uNdx;

	for (uNdx = 0; uNdx < SESS_FILE_MAX; uNdx += 1)
		{
		/*
		 * See if an unused slot is available
		 */
		if (asSessionFiles[uNdx].uBusy == 0)
			{
			memset(&asSessionFiles[uNdx], 0, sizeof(stDSFP));
			asSessionFiles[uNdx].uBusy = TRUE;
			return (uNdx + 1);
			}
		}

	return SF_ERR_BAD_PARAMETER;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfOpenSessionFile
 *
 * DESCRIPTION:
 *	This function is called to build an in memory data structure that
 *	represents the data currently in a session file.  If the file specified
 *	is not a valid session file, an error is returned.
 *
 * ARGUEMENTS:
 *	TCHAR *pszName	 --  the name of the session file.
 *
 * RETURNS:
 *	0 or an error code < 0;
 */
int sfOpenSessionFile(const SF_HANDLE sF, const TCHAR *pszName)
	{
	int uRv = 0;
	int uNdx = sF - 1;
	unsigned long uSize;
	TCHAR *pszStr;
	TCHAR *pszPtr;
	HANDLE hFile;
	int sID, sOldID;
	int32 dwSZ;
	int nOrderOK;
	DWORD dw;

	if (uNdx >= SESS_FILE_MAX)
		{
		uRv = SF_ERR_BAD_PARAMETER;
		goto OSFexit;
		}

	if (asSessionFiles[uNdx].uBusy == 0)
		{
		uRv = SF_ERR_BAD_PARAMETER;
		goto OSFexit;
		}

	if (asSessionFiles[uNdx].fOpen)
		{
		uRv = SF_ERR_FILE_ACCESS;
		goto OSFexit;
		}

	if (pszName && (StrCharGetStrLength(pszName) > 0))
		{
		int nRv = -1;
		pstDSFP pD;
		pstDSII pI;
		pstDSII pIx;

		/*
		 * Do everything that can be done with a name
		 */
		asSessionFiles[uNdx].hFilename = (TCHAR *)malloc(FNAME_LEN * sizeof(TCHAR));

		if (asSessionFiles[uNdx].hFilename == 0)
			goto OSFexit;

		pszStr = asSessionFiles[uNdx].hFilename;

		StrCharCopy(pszStr, pszName);
		pszStr = 0;

		/*
		 * Try to open file.  If we can then proceed
		 */

		hFile = CreateFile(pszName, GENERIC_READ, 0, 0,
							OPEN_EXISTING, 0, 0);

		/*
		 * Get the size of the file
		 */
		if (hFile == INVALID_HANDLE_VALUE)
			{
			asSessionFiles[uNdx].fOpen = TRUE;
			uRv = SF_ERR_FILE_ACCESS;
			return uRv;
			}

		else
			{
			/*
			 * Allocate a data block
			 */
			asSessionFiles[uNdx].fOpen = TRUE;

			uSize = GetFileSize(hFile, &dw);
			uSize = ROUND_UP(uSize);
			asSessionFiles[uNdx].dwDataSize = (int)uSize;
			asSessionFiles[uNdx].dwDataUsed = 0;
			asSessionFiles[uNdx].hData = (BYTE *)malloc(uSize);

			if (asSessionFiles[uNdx].hData == 0)
				{
				uRv = SF_ERR_MEMORY_ERROR;
				goto OSFexit;
				}

			pszStr = (TCHAR *)asSessionFiles[uNdx].hData;
			pszPtr = pszStr;
			memset(pszStr, 0, uSize * sizeof(TCHAR));

			/*
			 * Read in the header and check it
			 */
			//fread(pszPtr, 1, HDR_SIZE, f);
			ReadFile(hFile, pszPtr, HDR_SIZE * sizeof(TCHAR), &dw, 0);

			if (memcmp(pszPtr, pszHdr, 10 * sizeof(TCHAR)) != 0)
				{
				CloseHandle(hFile);
				uRv = SF_ERR_FILE_FORMAT;
				goto OSFexit;
				}

			pszPtr += HDR_SIZE;
			asSessionFiles[uNdx].dwDataUsed += HDR_SIZE;

			/*
			 * Initialize the index
			 */
			asSessionFiles[uNdx].uItemCount = 0;
			asSessionFiles[uNdx].uItemsAlloc = 64;
			uSize = sizeof(stDSII) * 64;
			asSessionFiles[uNdx].hIndex = (pstDSII)malloc(uSize);

			if (asSessionFiles[uNdx].hIndex == (pstDSII)0)
				{
				CloseHandle(hFile);
				uRv = SF_ERR_MEMORY_ERROR;
				goto OSFexit;
				}

			/*
			 * Read in the data items and add them to to structure
			 * The file is in the format:
			 *
			 *	USHORT	index
			 *	DWORD	dwSize
			 *	CHAR * size
			 */

			pD = &asSessionFiles[uNdx];

			pI = pD->hIndex;

			nOrderOK = TRUE;
			sOldID = 0;

			for (;;)
				{
				sID = 0;
				dwSZ = -1;

				ReadFile(hFile, &sID, sizeof(SHORT), &dw, 0);
				ReadFile(hFile, &dwSZ, sizeof(DWORD), &dw, 0);

				if ((sID == 0) && (dwSZ == -1))
					break;

				if ((sID == 0) && (dwSZ == 0))
					continue;

				if (sOldID > sID)
					{
					nOrderOK = FALSE;
					pI = (pstDSII)0;
					pszStr = NULL;
					}

				sOldID = sID;			/* For next time around */

				#if defined(DEBUG_OUTPUT)
				wsprintf(acBuffer,
						"r %c %d(0x%x) %d\r\n",
						nOrderOK ? 'a' : 'i',
						sID, sID, dwSZ);
				OutputDebugString(acBuffer);
				#endif

				if (nOrderOK)
					{
					// Do it inline

					if (pD->uItemCount >= pD->uItemsAlloc)
						{
						#if defined(DEBUG_OUTPUT)
						OutputDebugString("Index expanded\r\n");
						#endif
						/*
						 * We need to get a bigger chunk of memory
						 */
						if (pD->hIndex == (pstDSII)0)
							{
							pD->uItemsAlloc = 64;
							pD->hIndex = (pstDSII)malloc(sizeof(stDSII) * 64);
							}
						else
							{
							pD->uItemsAlloc *= 2;
							pD->hIndex = (pstDSII)realloc(pD->hIndex,
									  (size_t)pD->uItemsAlloc * sizeof(stDSII));
							}
						}

					if (pD->hData == 0)
						{
						pD->hData = (BYTE *)malloc(ROUND_UP(4096));
						pD->dwDataSize = ROUND_UP(4096);
						pD->dwDataUsed = 0;
						}

					/*
					 * Add the item at the end of the list
					 */
					nRv = pD->uItemCount;

					pIx = pI;
					pIx += nRv;

					pIx->uIndex = sID;
					pIx->dwSize = dwSZ;
					pIx->dwOffset = (DWORD)(pszPtr - pszStr);

					pD->uItemCount += 1;

					//fread(pszPtr, sSZ, 1, f);
					ReadFile(hFile, pszPtr, (DWORD)dwSZ, &dw, 0);
					pszPtr += dwSZ;
					asSessionFiles[uNdx].dwDataUsed += dwSZ;
					}
				else
					{
					TCHAR acBuffer[FNAME_LEN];

					//fread(acBuffer, dwSZ, 1, f);
					ReadFile(hFile, acBuffer, (DWORD)dwSZ, &dw, 0);
					sfPutSessionItem(uNdx + 1, (unsigned int)sID,
					    (unsigned long)dwSZ, acBuffer);
					}

				}

			/*
			 * Free things up
			 */

			CloseHandle(hFile);
			}

		return SF_OK;
		}

OSFexit:
	if (uNdx != SESS_FILE_MAX)
		{
		/*
		 * General cleanup
		 */
		if (asSessionFiles[uNdx].hFilename)
			{
			free(asSessionFiles[uNdx].hFilename);
			asSessionFiles[uNdx].hFilename = NULL;
			}

		if (asSessionFiles[uNdx].hData)
			{
			free(asSessionFiles[uNdx].hData);
			asSessionFiles[uNdx].hData = NULL;
			}

		if (asSessionFiles[uNdx].hIndex)
			{
			free(asSessionFiles[uNdx].hIndex);
			asSessionFiles[uNdx].hIndex = NULL;
			}

		memset(&asSessionFiles[uNdx], 0, sizeof(stDSFP));
		}

	return uRv;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfFlushSessionFile
 *
 * DESCRIPTION:
 *	This function is called to write all of the data in a session file out to
 *	disk and release any and all memory associated with the session file handle.
 *	It will not do anything if the session file does not have a file name
 *	associated with it.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	--	the session file handle
 *
 * RETURNS:
 *	ZERO if the file is written, an error code < 0 if there is and error.
 */
int sfFlushSessionFile(const SF_HANDLE sF)
	{
	int nRv = 0;
	int uNdx = sF - 1;
	int x;
	USHORT usIndex;
	TCHAR *pszName;
	TCHAR *pszPtr;
	pstDSII pI;
	HANDLE hFile;
	DWORD dw;

	if (uNdx >= SESS_FILE_MAX)
		{
		nRv = SF_ERR_BAD_PARAMETER;
		goto CSFexit;
		}

	if (asSessionFiles[uNdx].uBusy == 0)
		{
		nRv = SF_ERR_BAD_PARAMETER;
		goto CSFexit;
		}

	if (asSessionFiles[uNdx].fOpen == 0)
		{
		nRv = SF_ERR_FILE_ACCESS;
		goto CSFexit;
		}

	if (asSessionFiles[uNdx].uChanged != 0)
		{
		if (asSessionFiles[uNdx].hFilename == 0)
			{
			nRv = SF_ERR_FILE_ACCESS;
			goto CSFexit;
			}

		pszName = asSessionFiles[uNdx].hFilename;

		/* TODO: put in code to create directorys as necessary */

		//f = fopen(pszName, "wb");

		hFile = CreateFile(pszName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

		if (hFile == INVALID_HANDLE_VALUE)
			{
			nRv = SF_ERR_FILE_ACCESS;
			goto CSFexit;
			}

		pszPtr = (TCHAR *)asSessionFiles[uNdx].hData;

		/*
		 * Write out the header first
		 */
		if (StrCharGetStrLength(pszPtr) == 0)
			{
			/*
			 * Write out an empty header
			 */
			WriteFile(hFile, pszHdr, HDR_SIZE * sizeof(TCHAR), &dw, 0);
			}
		else
			{
			/*
			 * Write out the current header
			 */
			WriteFile(hFile, pszPtr, HDR_SIZE * sizeof(TCHAR), &dw, 0);
			}

		pszPtr += HDR_SIZE;


		pI = asSessionFiles[uNdx].hIndex;

		/*
		 * We loop thru the index and write out all of the stuff
		 */
		for (x = 0; x < asSessionFiles[uNdx].uItemCount; x += 1)
			{
			if (pI->dwSize != 0)
				{
				#if defined(DEBUG_OUTPUT)
				unsigned char acBuffer[64];

				wsprintf(acBuffer,
						"w %d(0x%x) %d\r\n",
						pI->uIndex, pI->uIndex, pI->dwSize);
				OutputDebugString(acBuffer);
				#endif

				usIndex = (USHORT)pI->uIndex;
				WriteFile(hFile, &usIndex, sizeof(USHORT), &dw, 0);
				WriteFile(hFile, &pI->dwSize, sizeof(DWORD), &dw, 0);

				WriteFile(hFile, pszPtr, (size_t)pI->dwSize * sizeof(TCHAR),
				    &dw, 0);

				pszPtr += pI->dwSize;
				}
		  	pI += 1;
			}

		CloseHandle(hFile);
		asSessionFiles[uNdx].uChanged = 0;

		// Finally, notify the shell so it can update the icon.
		//
		SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH,
			asSessionFiles[uNdx].hFilename, 0);
		}

CSFexit:

	return nRv;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfReleaseSessionFile
 *
 * DESCRIPTION:
 *	This function is called to release any and all memory associated with the
 *	session file handle.  This function by itself DOES NOT write any data out
 *	to the file.  That must be done elsewhere.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code < 0.
 */
int sfReleaseSessionFile(const SF_HANDLE sF)
	{
	unsigned int uNdx = (unsigned int)sF - 1;

	if (uNdx > SESS_FILE_MAX)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].uBusy == 0)
		return SF_ERR_BAD_PARAMETER;

	asSessionFiles[uNdx].uBusy = 0;

	if (asSessionFiles[uNdx].hFilename)
		{
		free(asSessionFiles[uNdx].hFilename);
		asSessionFiles[uNdx].hFilename = NULL;
		}

	if (asSessionFiles[uNdx].hData)
		{
		free(asSessionFiles[uNdx].hData);
		asSessionFiles[uNdx].hData = NULL;
		}

	if (asSessionFiles[uNdx].hIndex)
		{
		free(asSessionFiles[uNdx].hIndex);
		asSessionFiles[uNdx].hIndex = NULL;
		}

	memset(&asSessionFiles[uNdx], 0, sizeof(stDSFP));
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfCloseSessionFile
 *
 * DESCRIPTION:
 *	This function is called to release any and all memory associated with the
 *	session file handle.  This function by itself DOES NOT write any data out
 *	to the file.  That must be done elsewhere.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code < 0.
 */
int sfCloseSessionFile(const SF_HANDLE sF)
	{
	int rV1, rV2;

	rV1 = sfFlushSessionFile(sF);
	rV2 = sfReleaseSessionFile(sF);

	if (rV1 != 0)
		return rV1;

	return rV2;
	}
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfGetSessionFileName
 *
 * DESCRIPTION:
 *	This function is called to return the file name currently associated with
 *	the session file handle.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *	INT nSize		-- the size of the following buffer
 *	PCHAR pszName	-- the address of the buffer to copy the name to
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code < 0;
 */
int sfGetSessionFileName(const SF_HANDLE sF, const int nSize, TCHAR *pszName)
	{
	int uNdx = sF - 1;
        TCHAR *pszStr;
        int len;

	if (uNdx > SESS_FILE_MAX)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].uBusy == 0)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].hFilename == 0)
		return SF_ERR_BAD_PARAMETER;

        pszStr = asSessionFiles[uNdx].hFilename;


	//strncpy(pszName, pszStr, nSize);


        // JYF 03-Dec-1998 we don't want to read more than strlen
        //  of pszStr and nSize so not to read from inaccessible
        //  memory when running with debug heap.

        len = min (nSize, lstrlen (pszStr)+1);
        MemCopy(pszName, pszStr, (size_t)len * sizeof(TCHAR));
        pszName[nSize-1] = TEXT('\0');

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfSetSessionFileName
 *
 * DESCRIPTION:
 *	This function is called to change the name currently associated with the
 *	session file handle.  It does not cause the session file handle to reload
 *	any data or access the disk at all.  Data files are read when the session
 *	file is opened and written when the session file is closed.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *	PCHAR pszName	-- the address of the new file name
 *
 * RETURNS:
 *	ZERO if  everything is OK, otherwise and error code < 0;
 */
int sfSetSessionFileName(const SF_HANDLE sF, const TCHAR *pszName)
	{
	unsigned int uNdx = (unsigned int)sF - 1;
	TCHAR *pszStr;

	if (uNdx > SESS_FILE_MAX)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].uBusy == 0)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].fOpen == 0)
		return SF_ERR_FILE_ACCESS;

	if (asSessionFiles[uNdx].hFilename == 0)
		{
		asSessionFiles[uNdx].hFilename = (TCHAR *)malloc(FNAME_LEN * sizeof(TCHAR));

		if (asSessionFiles[uNdx].hFilename == 0)
			return SF_ERR_MEMORY_ERROR;
		}

	pszStr = asSessionFiles[uNdx].hFilename;

	memset(pszStr, 0, FNAME_LEN * sizeof(TCHAR));
	StrCharCopy(pszStr, pszName);

	asSessionFiles[uNdx].uChanged = 1;
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfGetSessionItem
 *
 * DESCRIPTION:
 *	This function is called to get data from the session file handle.  It can
 *	also be used to get the size of a data item.
 *
 *	If the last arguement, pvData, is NULL, then the size of the item is
 *	returned in *pulSize.  If pvData is not NULL, up to *pulSize bytes are
 *	returned at pvData.  If the number of bytes returned is less than *pulData,
 *	the new size is set in *pulSize.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *	uId				-- ID if the item
 *	pulSize			-- where the size is found or returned
 *	pvData			-- where the data is to be placed
 *
 * RETURNS:
 *	ZERO if  everything is OK, otherwise and error code < 0;
 */
int sfGetSessionItem(const SF_HANDLE sF,
					 const unsigned int uId,
					 unsigned long *pulSize,
					 void *pvData)
	{
	int uNdx = sF - 1;
	int dwMinSize;
	int low, mid, high;
	pstDSII pI;
	pstDSII pIx;
	BYTE *pszData;

	if (uNdx > SESS_FILE_MAX)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].uBusy == 0)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].fOpen == 0)
		return SF_ERR_FILE_ACCESS;

	if (asSessionFiles[uNdx].uItemCount == 0)
		return SF_ERR_BAD_PARAMETER;

	if ((uId < 1) || (uId > 0x7FFF))
		return SF_ERR_BAD_PARAMETER;

	/*
	 * See if the item is in the index
	 */
	if (asSessionFiles[uNdx].hIndex == (pstDSII)0)
		return SF_ERR_BAD_PARAMETER;

	pI = asSessionFiles[uNdx].hIndex;

	low = 0;
	high = asSessionFiles[uNdx].uItemCount - 1;

	while (low <= high)
		{
		mid = (low + high) / 2;
		pIx = pI + mid;

		if ((int)uId < pIx->uIndex)
			high = mid - 1;

		else if ((int)uId > pIx->uIndex)
			low = mid + 1;

		else
			{
			/*
			 * Found the item, see what they want to know
			 */
			if (pvData == NULL)
				{
				*pulSize = (unsigned long)pIx->dwSize;
				}
			else
				{
				pszData = asSessionFiles[uNdx].hData;
				dwMinSize = (int)*pulSize;

				if (dwMinSize > pIx->dwSize)
					dwMinSize = pIx->dwSize;

				if (dwMinSize)
                    MemCopy(pvData, pszData + pIx->dwOffset, (size_t)dwMinSize);

				}

			return 0;
			}
		}
	return SF_ERR_BAD_PARAMETER;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfCompareSessionItem
 *
 * DESCRIPTION:
 *	This function is called to check and see if an item exists in the data
 *	and if it is the same as the passed in item.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *	uId				-- the ID of the item
 *	ulSize			-- the size of the item
 *	pvData			-- address of the data
 *
 * RETURNS:
 *	TRUE if the two items are the same, otherwise false.
 */
int sfCompareSessionItem(const SF_HANDLE sF,
						 const unsigned int uId,
						 const unsigned long ulSize,
						 const void *pvData)
	{
	int uNdx = sF - 1;
	int low, mid, high;
	pstDSII pI;
	pstDSII pIx;
	BYTE *pszData;

	/*
	 * See if the item is in the index
	 */
	if (asSessionFiles[uNdx].hIndex == (pstDSII)0)
		return FALSE;

	pI = asSessionFiles[uNdx].hIndex;

	low = 0;
	high = asSessionFiles[uNdx].uItemCount - 1;

	while (low <= high)
		{
		mid = (low + high) / 2;
		pIx = pI + mid;

		if ((int)uId < pIx->uIndex)
			high = mid - 1;

		else if ((int)uId > pIx->uIndex)
			low = mid + 1;

		else
			{
			/*
			 * Check and see if the sizes are the same
			 */
			if (ulSize != (unsigned long)pIx->dwSize)
				return FALSE;

			/*
			 * Check and see if the data is the same
			 */
			pszData = asSessionFiles[uNdx].hData + pIx->dwOffset;

			if (memcmp((BYTE *)pvData, pszData, ulSize) == 0)
				return TRUE;
			return FALSE;
			}
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sfPutSessionItem
 *
 * DESCRIPTION:
 *	This function is called to add or modify an entry in the session file data
 *	associated with the current session file handle.  It does not cause the
 *	actual session file itself to be written.  That is done only when the
 *	session file handle is closed.
 *
 * ARGUEMENTS:
 *	SF_HANDLE sF	-- the session file handle
 *	uId				-- the ID of the item
 *	ulSize			-- the size of the item
 *	pvData			-- address of the data
 *
 * RETURNS:
 *	ZERO if  everything is OK, otherwise and error code < 0;
 */
int sfPutSessionItem(const SF_HANDLE sF,
					 const unsigned int uId,
					 const unsigned long ulSize,
					 const void *pvData)
	{
	int uNdx = sF - 1;
	int x, y;
	int32 dwSlide;
	int32 dwOffset;
	int32 dwNewSize;
	pstDSII pI;
	pstDSII pIx, pIy;
	BYTE *pszData;

#if defined(DEBUG_OUTPUT)
	unsigned char acBuffer[80];
#endif

	if (uNdx > SESS_FILE_MAX)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].uBusy == 0)
		return SF_ERR_BAD_PARAMETER;

	if (asSessionFiles[uNdx].fOpen == 0)
		return SF_ERR_FILE_ACCESS;

	if ((uId < 1) || (uId > 0x7FFF))
		return SF_ERR_BAD_PARAMETER;

	if (sfCompareSessionItem(sF, uId, ulSize, pvData))
		return SF_OK;

	x = sfAddToIndex(uNdx, (unsigned short)uId, 0, 0);

	pI = asSessionFiles[uNdx].hIndex;

	if (x != (-1))
		{
		pIx = pI + x;
		if (pIx->dwSize == 0)
			{
			/*
			 * Its a new item
			 */
			dwSlide = (int32)ulSize;
			if (x == 0)
				dwOffset = HDR_SIZE;
			else
				dwOffset = (pI+x-1)->dwOffset + (pI+x-1)->dwSize;

			#if defined(DEBUG_OUTPUT)
			wsprintf(acBuffer,
					"New 0x%x slide %d offset 0x%x",
					uId, dwSlide, dwOffset);
			OutputDebugString(acBuffer);
			#endif
			}
		else
			{
			/*
			 * Its a replacement item
			 */
			dwSlide = (int)ulSize - (int)pIx->dwSize;
			dwOffset = pIx->dwOffset;

			#if defined(DEBUG_OUTPUT)
			wsprintf(acBuffer,
					"Rep 0x%x slide %d offset 0x%x",
					uId, dwSlide, dwOffset);
			OutputDebugString(acBuffer);
			#endif
			}

		/*
		 * Check the memory requirements
		 */
		dwNewSize = asSessionFiles[uNdx].dwDataUsed + dwSlide;

		if (dwNewSize > asSessionFiles[uNdx].dwDataSize)
			{
			BYTE *hG;

			/*
			 * Need to allocate a new chunk of memory
			 */
			hG = asSessionFiles[uNdx].hData;
			hG = (BYTE *)realloc(hG, ROUND_UP(dwNewSize));

			if (hG == 0)
				{
				return SF_ERR_MEMORY_ERROR;
				}

			asSessionFiles[uNdx].hData = hG;
			asSessionFiles[uNdx].dwDataSize = ROUND_UP(dwNewSize);
			}

		/*
		 * Move the old data to the point necessary
		 */
		pszData = asSessionFiles[uNdx].hData;
		asSessionFiles[uNdx].dwDataUsed = dwNewSize;

		if (dwSlide < 0)
			{
			/* shrink the current space */
			dwNewSize = asSessionFiles[uNdx].dwDataSize;
			dwNewSize -= dwOffset;
			dwNewSize += dwSlide;

			memmove(pszData + dwOffset, 			 /* destination */
					pszData + dwOffset - dwSlide,	 /* source */
					(size_t)(dwNewSize - 1));		 /* count */

			#if defined(DEBUG_OUTPUT)
			wsprintf(acBuffer,
					" shrink from 0x%lx to 0x%lx size 0x%x\r\n",
					(long)(pszData + dwOffset - dwSlide),
					(long)(pszData + dwOffset),
					dwNewSize -1);
			OutputDebugString(acBuffer);
			#endif
			}
		else
			{
			/* expand the current space */
			dwNewSize = asSessionFiles[uNdx].dwDataSize;
			dwNewSize -= dwOffset;
			dwNewSize -= dwSlide;

			memmove(pszData + dwOffset + dwSlide,	 /* destination */
					pszData + dwOffset, 			 /* source */
					(size_t)(dwNewSize - 1)); 		 /* count */

			#if defined(DEBUG_OUTPUT)
			wsprintf(acBuffer,
					" expand from 0x%lx to 0x%lx size 0x%x\r\n",
					(long)(pszData + dwOffset),
					(long)(pszData + dwOffset + dwSlide),
					dwNewSize - 1);
			OutputDebugString(acBuffer);
			#endif
			}

		/*
		 * Copy in the new data
		 */
		memmove(pszData + dwOffset, 				/* destination */
				pvData, 							/* source */
				(unsigned int)ulSize);				/* count */


		/*
		 * Update the item in the index for the current item
		 */
		pIx->dwSize = (int32)ulSize;
		pIx->dwOffset = dwOffset;

		/*
		 * Adjust all the following items in the index
		 */
		for (y = x + 1; y < (int)asSessionFiles[uNdx].uItemCount; y += 1)
			{
			pIy = pI + y;
			pIy->dwOffset += dwSlide;
			}

		asSessionFiles[uNdx].uChanged = 1;
		return 0;
		}

	return SF_ERR_BAD_PARAMETER;
	}
