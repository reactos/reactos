/*	File: cloopout.c (created 12/28/93, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:55p $
 */
#include <windows.h>
#pragma hdrstop

// #define DEBUGSTR
#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\timers.h>
#include <tdll\file_msc.h>
#include <tdll\misc.h>
#include <tdll\tdll.h>
#include <term\res.h>
#include "tchar.h"
#include "cloop.h"
#include "cloop.hh"
#include "tchar.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopSend
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int CLoopSend
		(
		const HCLOOP	hCLoop,
			  void	   *pvData,
		const size_t	sztItems,
			  unsigned	uOptions)
	{
	ST_CLOOP	 * const pstCLoop = (ST_CLOOP *)hCLoop;
	int 		  fRetval = TRUE;
	ST_CLOOP_OUT *pstLast;
	ST_CLOOP_OUT *pstNew = NULL;
	size_t		  sztBytes;
	size_t		  sztAllocate;

	assert(pstCLoop);
	assert(pvData);

	if (!sztItems)
		return TRUE;

	EnterCriticalSection(&pstCLoop->csect);
	sztBytes = sztItems;
	if (bittest(uOptions, CLOOP_KEYS))
		sztBytes *= sizeof(KEY_T);

	// Notify the client that characters went out so the terminal can
	// do the appropriate tracking so user gets feedback about the
	// send operation, so there! - mrw

	NotifyClient(pstCLoop->hSession, (WORD)EVENT_CLOOP_SEND, 0);

	pstLast = pstCLoop->pstLastOutBlock;
	bitset(uOptions, CLOOP_CHARACTERS);

	// If new data is not in an allocated block and it will fit in the
	// block currently at the end of the chain, just add it in.
	if (!bittest(uOptions, CLOOP_ALLOCATED | CLOOP_SHARED) &&
			pstLast &&
			bittest(pstLast->uOptions, CLOOP_KEYS | CLOOP_CHARACTERS) ==
			bittest(uOptions, CLOOP_KEYS | CLOOP_CHARACTERS) &&
			sztBytes < (size_t)(pstLast->puchLimit - pstLast->puchHead))
		{
		// Copy data into existing block
		if (pstLast->puchHead == pstLast->puchTail)
			{
			// block is empty, reset pointers to beginning
			pstLast->puchHead = pstLast->puchTail = pstLast->puchData;
			}

        if (sztBytes)
            MemCopy(pstLast->puchHead, pvData, sztBytes);
		pstLast->puchHead += sztBytes;
		}

	else
		{
		// Couldn't put data into an existing block, try to add a new block
		// First create the block control structure
		if ((pstNew = CLoopNewOutBlock(pstCLoop, 0)) == NULL)
			{
			fRetval = FALSE;
			goto done;
			}

		pstNew->uOptions = uOptions;

		// Unless data is already in allocated memory that we can keep,
		// try to allocate memory for the new block
		if (!bittest(uOptions, CLOOP_ALLOCATED | CLOOP_SHARED))
			{
			sztAllocate = sztBytes;
			if (sztAllocate <= STD_BLOCKSIZE)
				{
				sztAllocate = STD_BLOCKSIZE;
				bitset(pstNew->uOptions, CLOOP_STDSIZE);
				}
			if ((pstNew->puchData = malloc(sztAllocate)) == NULL)
				{
				fRetval = FALSE;
				goto done;
				}
            if (sztBytes)
                MemCopy(pstNew->puchData, pvData, sztBytes);
			}
		else if (bittest(uOptions, CLOOP_SHARED))
			{
			// pvData actually contains a shared memory handle
			pstNew->hdlShared = (HGLOBAL)pvData;
			pstNew->puchData = GlobalLock(pstNew->hdlShared);
			sztAllocate = sztBytes;
			}
		else
			{
			// block was passed to us as allocated block that we now own.
			pstNew->puchData = pvData;
			sztAllocate = sztBytes;
			}

		pstNew->puchLimit = pstNew->puchData + sztAllocate;
		pstNew->puchHead = pstNew->puchData + sztBytes; // where data goes in
		pstNew->puchTail = pstNew->puchData; // where data comes out
		}

	done:
	if (!fRetval)
		{
		if (pstNew)
			{
			if (pstNew->puchData)
				{
				free(pstNew->puchData);
				pstNew->puchData = NULL;
				}
			free(pstNew);
			pstNew = NULL;
			}
		}
	else
		{
		pstCLoop->ulOutCount += sztItems;
		CLoopSndControl((HCLOOP)pstCLoop, CLOOP_RESUME, CLOOP_SB_NODATA);
		}
	LeaveCriticalSection(&pstCLoop->csect);
	return fRetval;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopSendTextFile
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int CLoopSendTextFile(const HCLOOP hCLoop, TCHAR *pszFileName)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;
	ST_CLOOP_OUT FAR *pstNew = NULL;
	int 			  fRetval = FALSE;
	unsigned long	  ulFileSize;

	assert(pstCLoop);
	assert(pszFileName);

	EnterCriticalSection(&pstCLoop->csect);
	if (GetFileSizeFromName(pszFileName, &ulFileSize) && ulFileSize > 0)
		{
		if ((pstNew = CLoopNewOutBlock(pstCLoop,
			(size_t)StrCharGetByteCount(pszFileName) + sizeof(TCHAR))) == NULL)
			goto done;		// leave fRetval FALSE

		pstNew->uOptions = CLOOP_TEXTFILE;
		StrCharCopy(pstNew->puchData, pszFileName);
		pstNew->ulBytes = ulFileSize;
		pstCLoop->ulOutCount += ulFileSize;

		// Set head and tail pointers even though they will not be directly
		//	used. When they are set to equal each other, the block will be
		//	removed from the chain
		pstNew->puchTail = pstNew->puchData;
		pstNew->puchHead = pstNew->puchData + 1;

		fRetval = TRUE;
		}

	done:
	if (!fRetval)
		{
		if (pstNew)
			{
			if (pstNew->puchData)
				{
				free(pstNew->puchData);
				pstNew->puchData = NULL;
				}
			free(pstNew);
			pstNew = NULL;
			}
		}
	else
		CLoopSndControl(hCLoop, CLOOP_RESUME, CLOOP_SB_NODATA);

	LeaveCriticalSection(&pstCLoop->csect);
	return fRetval;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopClearOutput
 *
 * DESCRIPTION:
 *	Clears all pending output from CLoop
 *
 * ARGUMENTS:
 *	pstCLoop -- The CLoop handle
 *
 * RETURNS:
 *	nothing
 */
void CLoopClearOutput(const HCLOOP hCLoop)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;

	EnterCriticalSection(&pstCLoop->csect);
	pstCLoop->ulOutCount = 0L;

	//* CLoopTextDspStop(pstCLoop);
	pstCLoop->fTextDisplay = FALSE;
	while (pstCLoop->pstFirstOutBlock)
		CLoopRemoveFirstOutBlock(pstCLoop);
	//* ComSendClear(pstCLoop->hCom);
	CLoopSndControl(hCLoop, CLOOP_SUSPEND, CLOOP_SB_NODATA);
	LeaveCriticalSection(&pstCLoop->csect);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopGetOutputCount
 *
 * DESCRIPTION:
 *	Returns the number of characters queued for output
 *
 * ARGUMENTS:
 *	hCLoop -- The CLoop handle
 *
 * RETURNS:
 *	The number of characters queued for output
 */
unsigned long CLoopGetOutputCount(const HCLOOP hCLoop)
	{
	unsigned uCount;

	EnterCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	uCount = ((ST_CLOOP *)hCLoop)->ulOutCount;
	LeaveCriticalSection(&((ST_CLOOP *)hCLoop)->csect);
	return uCount;
	}


/* --- Internal routines --- */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopGetNextOutput
 *
 * DESCRIPTION:
 *	Fetchs the next item from the output queue for transmission.
 *
 * ARGUMENTS:
 *	hCLoop -- The CLoop handle
 *	pkKey  -- Place to put the next item to be transmitted
 *
 * RETURNS:
 *	TRUE if there is data to be transmitted.
 */
int CLoopGetNextOutput(ST_CLOOP * const pstCLoop, KEY_T * const pkKey)
	{
	ST_CLOOP_OUT FAR *	pstBlock;
	int 				fRetval = FALSE;
	unsigned long		nBytesRead = 0L;
	KEY_T			   *pkGet;
	TCHAR				chFileChar;
	TCHAR				chChar;
	HWND				hwndDsp;
	CHAR				achBuf[2];

	assert(pstCLoop);
	assert(pkKey);				// Caller must provide pointer for result

	if (pstCLoop->keyHoldKey)
		{
		*pkKey = pstCLoop->keyHoldKey;
		pstCLoop->keyHoldKey = (KEY_T)0;
		fRetval = TRUE;
		}
	else if (pstCLoop->ulOutCount || pstCLoop->pstFirstOutBlock)
		{
		pstBlock = pstCLoop->pstFirstOutBlock;
		assert(pstBlock);	// should never be NULL while ulOutCount > 0
		// Skip any empty blocks (shouldn't be more than one)
		while (pstBlock->puchHead == pstBlock->puchTail)
			{
			CLoopRemoveFirstOutBlock(pstCLoop);
			pstBlock = pstCLoop->pstFirstOutBlock;
			assert(pstBlock);	// should never remove all blocks
			}

		if (bittest(pstBlock->uOptions, CLOOP_CHARACTERS))
			{
			if (bittest(pstBlock->uOptions, CLOOP_KEYS))
				{
				pkGet = (KEY_T *)pstBlock->puchTail;
				*pkKey = *pkGet++;
				pstBlock->puchTail = (TCHAR *)pkGet;
				--pstCLoop->ulOutCount;
				fRetval = TRUE;
				}
			else
				{
				chChar = *pstBlock->puchTail++;
				*pkKey = (KEY_T)chChar;
				--pstCLoop->ulOutCount;
				// We must strip the LF from CR-LF pairs. This data may come from
				//	the clipboard, in which case, line endings will be stored as
				//	CR-LF. Terminals normally send only a CR at line ends. If we
				//	DO need to send the LF, it will be added in the cloop output
				//	routines if the append LF option is set.
				if (chChar != TEXT('\n') ||
						pstBlock->chLastChar != TEXT('\r'))
					{
					pstBlock->chLastChar = chChar;
					fRetval = TRUE;
					}
				}
			}

		else if (bittest(pstBlock->uOptions, CLOOP_OPENFILE))
			{
			//* TODO: this stuff will have to be expanded eventually to
			//	handle SBCS, DBCS and Unicode input files. For now, all
			//	text files are assumed to be SBCS.
			if (ReadFile(pstCLoop->hOutFile, achBuf, 1, &nBytesRead,
					(LPOVERLAPPED)0))
				{
				if (nBytesRead > 0)
					{
					//OemToCharBuff(achBuf, &chFileChar, 1);-  mrw:10/20/95
                    chFileChar = achBuf[0];  // mrw:10/20/95
					pstCLoop->ulSentSoFar += 1;
					*pkKey = (KEY_T)chFileChar;
					--pstBlock->ulBytes;
					--pstCLoop->ulOutCount;

					// We must strip the LF from CR-LF pairs. If line ends are
					//	set to CR-LF in the settings, an LF will be added back in
					//	later.
					if (chFileChar != TEXT('\n') ||
							pstBlock->chLastChar != TEXT('\r'))
						{
						pstBlock->chLastChar = chFileChar;
						fRetval = TRUE;
						}
					}
				else
					{
					// When ReadFile returns TRUE but 0 chars. were read,
					// it indicates end of file.
					CloseHandle(pstCLoop->hOutFile);
					pstCLoop->hOutFile = (HANDLE *)0;
					pstBlock->puchTail = pstBlock->puchHead;
					}
				}
			else
				{
				// ReadFile returned an error
				//* TODO: display error message
				CloseHandle(pstCLoop->hOutFile);
				pstCLoop->hOutFile = (HANDLE *)0;
				pstBlock->puchTail = pstBlock->puchHead;
				// pstCLoop->ulOutCount -= pstBlock->ulBytes;
                pstCLoop->ulOutCount = 0;   //JMH 03-25-96
				}
			}
		else if (bittest(pstBlock->uOptions, CLOOP_TEXTFILE))
			{
			// New block with text file name, open file & start emptying
			pstCLoop->hOutFile = CreateFile(pstBlock->puchData,
					GENERIC_READ, FILE_SHARE_READ,
					(LPSECURITY_ATTRIBUTES)0, OPEN_EXISTING,
					FILE_FLAG_SEQUENTIAL_SCAN, (HANDLE)0);
			if (pstCLoop->hOutFile == INVALID_HANDLE_VALUE)
				{
				//* Display file error
                pstCLoop->hOutFile = (HANDLE *)0;
				pstBlock->puchTail = pstBlock->puchHead; // Remove from queue
                pstCLoop->ulOutCount = 0;
                PostMessage(sessQueryHwnd(pstCLoop->hSession),
                    WM_ERROR_MSG, (WPARAM) IDS_ER_OPEN_FAILED, 0);
				}
			else
				{
				pstCLoop->ulTotalSend += pstBlock->ulBytes;
				//* CLoopTextDspFilename(pstCLoop, (LPTSTR)pstBlock->puchData);
				bitset(pstBlock->uOptions, CLOOP_OPENFILE);
				}
			}
		else if (bittest(pstBlock->uOptions, CLOOP_TEXTDSP))
			{
			MemCopy(&hwndDsp, pstBlock->puchData, sizeof(hwndDsp));
			//* CLoopDoTextDsp(pstCLoop, hwndDsp);
			// Set puchHead == puchTail to cause this block to be dropped
			pstBlock->puchHead = pstBlock->puchTail = pstBlock->puchData;
			}
		if (pstCLoop->ulOutCount == 0 && !pstBlock->pstNext)
            {
			CLoopSndControl((HCLOOP)pstCLoop, CLOOP_SUSPEND, CLOOP_SB_NODATA);

            // mrw:3/11/96 - fixes send files being locked open after done.
            //
            if (pstCLoop->hOutFile)
                {
                CloseHandle(pstCLoop->hOutFile);
                pstCLoop->hOutFile = 0;
                pstBlock->puchTail = pstBlock->puchHead;
                }
            }
		}

	if (fRetval)
		{
		if (*pkKey == TEXT('\r') && pstCLoop->keyLastKey == TEXT('\r') &&
				pstCLoop->stWorkSettings.fExpandBlankLines)
			{
			pstCLoop->keyHoldKey = *pkKey;	// this will be returned next call
			*pkKey = TEXT(' '); 		  // add space to expand blank line
			}
		pstCLoop->keyLastKey = *pkKey;
		}

	return fRetval;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopNewOutBlock
 *
 * DESCRIPTION:
 *	Internal routine to allocate and initialize a control block for the
 *	output chain.
 *
 * ARGUMENTS:
 *	siztData -- If non-zero, the routine will attempt to allocate memory
 *				of the specfied size and assign it to the puchData member.
 *				If zero the puchData member is set to NULL.
 *
 * RETURNS:
 *	Pointer to a new control block for the output chain
 */
ST_CLOOP_OUT *CLoopNewOutBlock(ST_CLOOP *pstCLoop, const size_t sizetData)
	{
	ST_CLOOP_OUT *pstNew = NULL;

	if ((pstNew = (ST_CLOOP_OUT *)malloc(sizeof(*pstNew))) != NULL)
		{
		pstNew->pstNext    =  NULL;
		pstNew->uOptions   =  0;
		pstNew->hdlShared  =  (HGLOBAL)0;
		pstNew->puchData   =  NULL;
		pstNew->puchLimit  =  NULL;
		pstNew->puchHead   =  NULL;
		pstNew->puchTail   =  NULL;
		pstNew->ulBytes    =  0L;
		pstNew->chLastChar = 0;

		if (sizetData > 0)
			{
			if ((pstNew->puchData = (TCHAR *)malloc(sizetData)) == NULL)
				{
				free(pstNew);
				pstNew = NULL;
				}
			}
		}
	if (!pstNew)
		{
		//* utilReportError(pstCLoop->hSession, RE_ERROR | RE_OK, NM_NEED_MEM,
		//* 	   strldGet(mGetStrldHdl(pstCLoop->hSession), NM_XFER_DISPLAY));
		}
	else
		{
		// link new block into chain
		if (pstCLoop->pstLastOutBlock)
			pstCLoop->pstLastOutBlock->pstNext = pstNew;
		else
			{
			assert(!pstCLoop->pstFirstOutBlock);
			pstCLoop->pstFirstOutBlock = pstNew;
			}
		pstCLoop->pstLastOutBlock = pstNew;
		}

	return pstNew;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopRemoveFirstOutBlock
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void CLoopRemoveFirstOutBlock(ST_CLOOP * const pstCLoop)
	{
	ST_CLOOP_OUT *pstBlock;

	pstBlock = pstCLoop->pstFirstOutBlock;
	if (pstBlock)
		{
		// link around block to be removed
		pstCLoop->pstFirstOutBlock = pstBlock->pstNext;
		if (!pstCLoop->pstFirstOutBlock)
			{
			// empty list
			assert(pstCLoop->pstLastOutBlock == pstBlock);
			pstCLoop->pstLastOutBlock = (ST_CLOOP_OUT *)0;
			}

		// free storage memory
		if (bittest(pstBlock->uOptions, CLOOP_SHARED))
			{
			GlobalUnlock(pstBlock->hdlShared);
			GlobalFree(pstBlock->hdlShared);
			}
		else
			{
			free(pstBlock->puchData);
			pstBlock->puchData = NULL;
			}

		// Now free the block control structure
		free(pstBlock);
		pstBlock = NULL;
		}
	}
