/*	File: cloopctl.c (created 12/16/93, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */
#include <windows.h>
#pragma hdrstop

#include <time.h>

// #define DEBUGSTR
#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\timers.h>
#include <tdll\com.h>
#include <tdll\mc.h>
#include <tdll\cnct.h>
#include "cloop.h"
#include "cloop.hh"
#include "tchar.h"
#include <tdll\assert.h>


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopCreateHandle
 *
 * DESCRIPTION:
 *	Creates handle for use of com loop routines. This function assumes that
 *	the emulator and com handles have already been created and stored in the
 *	session handle when it is called.
 *
 * ARGUMENTS:
 *	hSession -- Session handle
 *
 * RETURNS:
 *	A handle to be used in all other CLoop calls or NULL if unsuccessful
 */
HCLOOP CLoopCreateHandle(const HSESSION hSession)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)0;
	unsigned  ix;
	int 	  fSuccess = TRUE;

	pstCLoop = malloc(sizeof(*pstCLoop));
	if (!pstCLoop)
		fSuccess = FALSE;
	else
		{
		// Initialize structure
		memset(pstCLoop, 0, sizeof(ST_CLOOP));

		pstCLoop->hSession			= hSession;
		pstCLoop->hEmu				= sessQueryEmuHdl(hSession);
		pstCLoop->hCom				= sessQueryComHdl(hSession);
		pstCLoop->afControl 		= 0;
		pstCLoop->afRcvBlocked		= 0;
		pstCLoop->afSndBlocked		= CLOOP_SB_NODATA;
		pstCLoop->nRcvBlkCnt		= 0;
		pstCLoop->fRcvBlkOverride	= FALSE;
		pstCLoop->fSuppressDsp		= FALSE;
		pstCLoop->fDataReceived 	= FALSE;
		pstCLoop->htimerRcvDelay	= (HTIMER)0;
		pstCLoop->pfRcvDelay		= CLoopRcvDelayProc;
		pstCLoop->pfCharDelay		= CLoopCharDelayProc;
		pstCLoop->pstFirstOutBlock	= NULL;
		pstCLoop->pstLastOutBlock	= NULL;
		pstCLoop->ulOutCount		= 0L;
		pstCLoop->hOutFile			= (HANDLE)0;
		pstCLoop->keyLastKey		= (KEY_T)0;
		pstCLoop->keyHoldKey		= (KEY_T)0;
		pstCLoop->pstRmtChain		= NULL;
		pstCLoop->pstRmtChainNext	= NULL;
		pstCLoop->fRmtChain 		= FALSE;
		pstCLoop->fTextDisplay		= FALSE;
		pstCLoop->hDisplayBlock		= (HANDLE)0;

		// Set default user settings
		CLoopInitHdl((HCLOOP)pstCLoop);

		pstCLoop->lpLearn			= (LPVOID)0;

		for (ix = 0; ix < DIM(pstCLoop->ahEvents); ++ix)
			pstCLoop->ahEvents[ix] = (HANDLE)0;
		pstCLoop->hEngineThread = (HANDLE)0;

		pstCLoop->fDoMBCS			= FALSE;
		pstCLoop->cLeadByte			= 0;
		pstCLoop->cLocalEchoLeadByte= 0;
#if defined(CHAR_MIXED)
		// Added for debugging
		pstCLoop->fDoMBCS			= TRUE;
#endif

		// Create synchronization objects
		InitializeCriticalSection(&pstCLoop->csect);
		}

	if (!fSuccess)
		CLoopDestroyHandle((HCLOOP *)&pstCLoop);

	DBGOUT_NORMAL("CLoopCreateHandle(%lX) returned %lX\r\n",
			hSession,  pstCLoop, 0, 0, 0);
	return (HCLOOP)pstCLoop;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopDestroyHandle
 *
 * DESCRIPTION:
 *	Destroys a handle created by CLoopCreateHandle and sets the storage
 *	variable to NULL;
 *
 * ARGUMENTS:
 *	ppstCLoop -- address of a variable holding a CLoop Handle
 *
 * RETURNS:
 *	nothing
 */
void CLoopDestroyHandle(HCLOOP * const ppstCLoop)
	{
	ST_CLOOP *pstCLoop;

	assert(ppstCLoop);

	pstCLoop = (ST_CLOOP *)*ppstCLoop;

	if (pstCLoop)
		{
		CLoopDeactivate((HCLOOP)pstCLoop);

		free(pstCLoop);
		pstCLoop = NULL;
		}

	*ppstCLoop = (HCLOOP)0;
	DBGOUT_NORMAL("CLoopDestroyHandle(l%X)\r\n", pstCLoop, 0,0,0,0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopActivate
 *
 * DESCRIPTION:
 *	Prepares cloop handle for actual use by starting its thread.
 *
 * ARGUMENTS:
 *	pstCLoop -- CLoop handle returned from CLoopCreateHandle
 *
 * RETURNS:
 *	TRUE if thread was started, FALSE otherwise
 */
int CLoopActivate(const HCLOOP hCLoop)
	{
	int 	  fSuccess = TRUE;
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;
	unsigned  ix;
	DWORD	  dwThreadId;

	// Store these in handle for fast access
	pstCLoop->hEmu = sessQueryEmuHdl(pstCLoop->hSession);
	pstCLoop->hCom = sessQueryComHdl(pstCLoop->hSession);

	for (ix = 0; ix < DIM(pstCLoop->ahEvents); ++ix)
		{
		pstCLoop->ahEvents[ix] = CreateEvent((LPSECURITY_ATTRIBUTES)0,
				TRUE, FALSE, NULL);
		if (!pstCLoop->ahEvents[ix])
			{
			fSuccess = FALSE;
			break;
			}
		}

	if (fSuccess)
		{
		// Start the thread to handle cloop's responsibilities
		// (Gentleprograms, start your engines
		EnterCriticalSection(&pstCLoop->csect);

		pstCLoop->hEngineThread = CreateThread(
				(LPSECURITY_ATTRIBUTES)0,
				4096,
				(LPTHREAD_START_ROUTINE)CLoop,
				(LPVOID)pstCLoop,
				0,
				&dwThreadId);


		if (!pstCLoop->hEngineThread)
			fSuccess = FALSE;
#if 0   //jmh 07-10-96
		else
			SetThreadPriority(pstCLoop->hEngineThread,
					THREAD_PRIORITY_HIGHEST);
#endif  // 0

		LeaveCriticalSection(&pstCLoop->csect);
		}
	return fSuccess;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopDeactivate
 *
 * DESCRIPTION:
 *	Shuts down cloop but does not destroy the handle
 *
 * ARGUMENTS:
 *	pstCLoop -- CLoop handle returned from CLoopCreateHandle
 *
 * RETURNS:
 *	nothing
 */
void CLoopDeactivate(const HCLOOP hCLoop)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;
	unsigned  ix;

	if (pstCLoop)
		{
		// This next call should cause the cloop thread to exit
		// Don't enter critical section to do this, since thread won't
		//	be able to exit
		if (pstCLoop->hEngineThread)
			{
			//JMH 05-28-96 CLoop was blowing up when no phone number was
			// provided, and you exited, answering no to the save prompt.
			// Suspending receive before terminating the thread fixed it.
			//
			CLoopRcvControl((HCLOOP)pstCLoop, CLOOP_SUSPEND, CLOOP_RB_INACTIVE);
			CLoopControl((HCLOOP)pstCLoop, CLOOP_SET, CLOOP_TERMINATE);

			// Wait for the engine thread to exit
			if (WaitForSingleObject(pstCLoop->hEngineThread, 10000) ==
					WAIT_OBJECT_0)
				{
				CloseHandle(pstCLoop->hEngineThread);
				pstCLoop->hEngineThread = (HANDLE)0;
				}
			}

		for (ix = 0; ix < DIM(pstCLoop->ahEvents); ++ix)
			{
			if (pstCLoop->ahEvents[ix])
				{
				CloseHandle(pstCLoop->ahEvents[ix]);
				pstCLoop->ahEvents[ix] = (HANDLE)0;
				}
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopReset
 *
 * DESCRIPTION:
 *	Initializes the cloop routines for a new call. This is not really
 *	necessary prior to the first connection, but prevents left-over flags,
 *	buffers and such from one connection from affecting later connections.
 *
 * ARGUMENTS:
 *	pstCLoop -- CLoop handle returned from CLoopCreateHandle
 *
 * RETURNS:
 *	nothing
 */
void CLoopReset(const HCLOOP hCLoop)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;

	pstCLoop->afControl = 0;
	pstCLoop->afRcvBlocked = 0;
	pstCLoop->afSndBlocked = CLOOP_SB_NODATA;
	CLoopClearOutput((HCLOOP)pstCLoop);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopRcvControl
 *
 * DESCRIPTION:
 *	Used to suspend and resume receiving in the CLoop routines. Can be
 *	called from Client or Wudge. Requests can be made to suspend or resume
 *	receiving for any of several reasons. Receiving can be suspended for
 *	more than one reason at a time. It will not resume until all reasons
 *	have been cleared.
 *
 * ARGUMENTS:
 *	hCLoop -- CLoop handle returned from CLoopCreateHandle
 *	iAction -- One of CLOOP_SUSPEND or CLOOP_RESUME
 *	iReason -- Reason for action as defined in cloop.h
 *				Ex.: CLOOP_RB_NODATA,  CLOOP_RB_INACTIVE
 *					 CLOOP_RB_SCRLOCK, CLOOP_RB_SCRIPT
 *
 * RETURNS:
 *	nothing
 */
void CLoopRcvControl(const HCLOOP hCLoop,
					 const unsigned uAction,
					 const unsigned uReason)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;

	assert(uAction == CLOOP_SUSPEND || uAction == CLOOP_RESUME);

	EnterCriticalSection(&pstCLoop->csect);
	if (uReason == CLOOP_RB_SCRIPT)
		{
		/*
		 * Scripts work a little bit different
		 */
		// DbgOutStr("CLOOP_RB_SCRIPT %d", pstCLoop->nRcvBlkCnt, 0,0,0,0);
		switch (uAction)
			{
		default:
			break;

		case CLOOP_SUSPEND:
			pstCLoop->nRcvBlkCnt += 1;
			break;

		case CLOOP_RESUME:
			pstCLoop->nRcvBlkCnt -= 1;

			// There used to be code to prevent this value from going neg.
			// but we must allow this to go negative so that cleanup
			//	 can occur when we abort within an API call.
			break;
			}
		// DbgOutStr(" %d\r\n", pstCLoop->nRcvBlkCnt, 0,0,0,0);

		if (pstCLoop->fRcvBlkOverride == FALSE)
			{
			/*
			 * If someone is overriding us, let them restore the bits
			 */
			if (pstCLoop->nRcvBlkCnt > 0)
				{
				bitset(pstCLoop->afRcvBlocked, CLOOP_RB_SCRIPT);
				}
			else
				{
				bitclear(pstCLoop->afRcvBlocked, CLOOP_RB_SCRIPT);
				}
			}
		}
	else
		{
		if (uAction == CLOOP_SUSPEND)
			bitset(pstCLoop->afRcvBlocked, uReason);
		else if (uAction == CLOOP_RESUME)
			bitclear(pstCLoop->afRcvBlocked, uReason);
		}

	DBGOUT_NORMAL("CLoopRcvControl(%08lx):%04X (fRcvBlkOverride=%d, nRcvBlkCnt=%d)\r\n",
			pstCLoop, pstCLoop->afRcvBlocked, pstCLoop->fRcvBlkOverride,
			pstCLoop->nRcvBlkCnt,0);

	if (pstCLoop->afRcvBlocked)
		ResetEvent(pstCLoop->ahEvents[EVENT_RCV]);
	else
		SetEvent(pstCLoop->ahEvents[EVENT_RCV]);

	LeaveCriticalSection(&pstCLoop->csect);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopOverrideControl
 *
 * DESCRIPTION:
 *	This function can be called internally from the engine in order to
 *	override the CLoopRcvControl \CLOOP_RB_SCRIPT blocking state.  It is
 *	intended to be used during a connection process, when the connection
 *	stuff needs to see the data coming in.
 *
 * ARGUEMENTS:
 *	hCLoop	  -- CLoop handle returned from CLoopCreateHandle
 *	fOverride	-- TRUE to override, FALSE to restore things
 *
 * RETURNS:
 *	Nothing.
 */
void CLoopOverrideControl(const HCLOOP hCLoop, const int fOverride)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;

	EnterCriticalSection(&pstCLoop->csect);
	if (fOverride)
		{
		pstCLoop->fRcvBlkOverride = TRUE;

		if (pstCLoop->nRcvBlkCnt > 0)
			{
			bitclear(pstCLoop->afRcvBlocked, CLOOP_RB_SCRIPT);
			if (!pstCLoop->afRcvBlocked)
				SetEvent(pstCLoop->ahEvents[EVENT_RCV]);
			}
		}
	else
		{
		pstCLoop->fRcvBlkOverride = FALSE;

		if (pstCLoop->nRcvBlkCnt > 0)
			{
			bitset(pstCLoop->afRcvBlocked, CLOOP_RB_SCRIPT);
			ResetEvent(pstCLoop->ahEvents[EVENT_RCV]);
			}
		}

	LeaveCriticalSection(&pstCLoop->csect);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopSndControl
 *
 * DESCRIPTION:
 *	Used to suspend and resume sending from the CLoop routines.
 *	Requests can be made to suspend or resume sending for any of several
 *	reasons. Sending can be suspended for more than one reason at a time.
 *	It will not resume until all reasons have been cleared.
 *
 * ARGUMENTS:
 *	hCLoop -- CLoop handle returned from CLoopCreateHandle
 *	uAction -- One of CLOOP_SUSPEND or CLOOP_RESUME
 *	uReason -- Reason for action as defined in cloop.h
 *				Ex.: CLOOP_SB_NODATA,  CLOOP_SB_INACTIVE
 *					 CLOOP_SB_SCRLOCK, CLOOP_SB_LINEWAIT
 *
 * RETURNS:
 *	nothing
 */
void CLoopSndControl(const HCLOOP hCLoop,
					 const unsigned uAction,
					 const unsigned uReason)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;

	assert(uAction == CLOOP_SUSPEND || uAction == CLOOP_RESUME);
	DbgOutStr("CLoopSndControl(%x, %x)\r\n", uAction, uReason, 0,0,0);

	EnterCriticalSection(&pstCLoop->csect);

	if (uAction == CLOOP_SUSPEND)
		{
		if (!pstCLoop->afSndBlocked)
			{
			DbgOutStr("Resetting EVENT_SEND (0x%x)\r\n", 0,0,0,0,0);
			ResetEvent(pstCLoop->ahEvents[EVENT_SEND]);
			}
		bitset(pstCLoop->afSndBlocked, uReason);
		}
	else if (uAction == CLOOP_RESUME)
		{
		// Allow this bit to be reset
		//
		if (bittest(uReason, CLOOP_SB_CNCTDRV))
			bitclear(pstCLoop->afSndBlocked, CLOOP_SB_CNCTDRV);

		// Only do this if we're not in the process of connecting
		//
		if (! bittest(pstCLoop->afSndBlocked, CLOOP_SB_CNCTDRV))
			{
			bitclear(pstCLoop->afSndBlocked, uReason);

			// If sending is being resumed because new data has arrived,
			// make sure we're connected. If not, send off a message to
			// try to connect without dialing
			if (bittest(uReason, CLOOP_SB_NODATA))
				{
				if (cnctQueryStatus(sessQueryCnctHdl(pstCLoop->hSession)) ==
						CNCT_STATUS_FALSE &&
							!bittest(pstCLoop->afSndBlocked, CLOOP_SB_UNCONNECTED))
					{
					bitset(pstCLoop->afSndBlocked, CLOOP_SB_UNCONNECTED);
					NotifyClient(pstCLoop->hSession, (int) EVENT_PORTONLY_OPEN, 0);
					}
				}
			}

		if (!pstCLoop->afSndBlocked)
			{
			DbgOutStr("Setting EVENT_SEND\r\n", 0,0,0,0,0);
			SetEvent(pstCLoop->ahEvents[EVENT_SEND]);
			}
		}

	LeaveCriticalSection(&pstCLoop->csect);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopControl
 *
 * DESCRIPTION:
 *	Used to control the action of the CLoop engine.
 *	The CLoop engine normally processes any characters
 *	received from the remote system and any characters or keys queued for
 *	output. This call can be used to alter the normal flow of data for
 *	special needs such as file transfers and scripts.
 *
 * ARGUMENTS:
 *	hCLoop -- Handle returned from call to CLoopCreateHandle
 *	uAction -- CLOOP_SET or CLOOP_CLEAR to set or clear control bits
 *	uReason -- Value indicating what is being controlled. Values are
 *				listed in cloop.h
 *				Ex:CLOOP_TERMINATE CLOOP_OUTPUT_WAITING CLOOP_TRANSFER_READY
 *
 * RETURNS:
 *	nothing
 */
void CLoopControl(
		const HCLOOP hCLoop,
			  unsigned uAction,
			  unsigned uReason)
	{
    ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;

    if ( !pstCLoop )
        return; //MPT:10SEP98 to prevent an access violation

    if (bittest(uReason, CLOOP_MBCS))
		{
		if (uAction == CLOOP_SET)
			pstCLoop->fDoMBCS = TRUE;
		else
			pstCLoop->fDoMBCS = FALSE;

		bitclear(uReason, CLOOP_MBCS);
		// I am not sure about returning here or not ...
		// As is, it requires that this be the only flag used for this call.
		return;
		}

	assert(uAction == CLOOP_SET || uAction == CLOOP_CLEAR);
	EnterCriticalSection(&pstCLoop->csect);

	if (bittest(uReason, CLOOP_CONNECTED))
		{
		if (uAction == CLOOP_SET)
			{
			CLoopSndControl(hCLoop, CLOOP_RESUME, CLOOP_SB_UNCONNECTED);
			}
		else
			{
			CLoopClearOutput(hCLoop);
			CLoopSndControl(hCLoop, CLOOP_SUSPEND, CLOOP_SB_NODATA);
			}
		// Don't put this flag in afControl
		bitclear(uReason, CLOOP_CONNECTED);
		}

	if (bittest(uReason, CLOOP_SUPPRESS_DSP))
		{
		// This bit cannot be kept in afControl because doing so would
		// keep cloop from suspending itself when appropriate
		pstCLoop->fSuppressDsp = (uAction == CLOOP_SET);
		bitclear(uAction, CLOOP_SUPPRESS_DSP);
		}

#if 0
	if (uAction == CLOOP_SET)
		{
		bitset(pstCLoop->afControl, uReason);
		SetEvent(pstCLoop->ahEvents[EVENT_SEND]);
		}
	else if (uAction == CLOOP_CLEAR)
		{
		bitclear(pstCLoop->afControl, uReason);
		if (!pstCLoop->afControl)
			ResetEvent(pstCLoop->ahEvents[EVENT_SEND]);
		}
#endif

	if (bittest(uReason, CLOOP_MBCS))
		{
		if (uAction == CLOOP_SET)
			pstCLoop->fDoMBCS = TRUE;
		else
			pstCLoop->fDoMBCS = FALSE;

		bitclear(uReason, CLOOP_MBCS);
		// I am not sure about returning here or not ...
		LeaveCriticalSection(&pstCLoop->csect);
		return;
		}


	if (uAction == CLOOP_SET)
		bitset(pstCLoop->afControl, uReason);
	else if (uAction == CLOOP_CLEAR)
		bitclear(pstCLoop->afControl, uReason);

	if (pstCLoop->afControl)
		SetEvent(pstCLoop->ahEvents[EVENT_CONTROL]);
	else
		ResetEvent(pstCLoop->ahEvents[EVENT_CONTROL]);

	LeaveCriticalSection(&pstCLoop->csect);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopRegisterRmtInputChain
 *
 * DESCRIPTION:
 *	Adds a function to a chain of functions that are called whenever
 *	a new character is received from the remote system.
 *
 * ARGUMENTS:
 *	pstCLoop   -- Value returned from CLoopCreateHandle
 *	pfFunc	   -- Pointer to a function to be called when each remote
 *				  character is received. The function should be declared as:
 *				  VOID FAR PASCAL FunctionName(METACHAR mc, VOID FAR *pvUserData)
 *				  The value passed in the argument should be the return value
 *				  from MakeProcInstance.
 *	pvUserData -- Any arbitrary value that can be cast to VOID FAR *. This
 *				  value will be maintained in the chain and passed back to
 *				  the caller whenever (l*pfFunc) is called. No use is made
 *				  of this value by the CLoop routines.
 *
 * RETURNS:
 *	A void * return handle that must be saved and passed to
 *	CLoopUnregisterRmtInputChain when the function is no longer needed.
 */
void * CLoopRegisterRmtInputChain(const HCLOOP hCLoop,
			const CHAINFUNC pfFunc,
				  void *pvUserData)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;
	ST_FCHAIN *pstNew = NULL;

	assert(pfFunc);
	assert(pstCLoop);

	if ((pstNew = (ST_FCHAIN *)malloc(sizeof(*pstNew))) != NULL)
		{
		EnterCriticalSection(&pstCLoop->csect);

		// Initialize new node
		pstNew->pstParent = pstCLoop;
		pstNew->pfFunc = pfFunc;
		pstNew->pvUserData = pvUserData;

		DBGOUT_NORMAL("CLoopRegisterRmtInputChain(0x%lx, 0x%lx) -> %lX\r\n",
					(LONG)pstNew->pfFunc, (LONG)pstNew->pvUserData,
					pstNew, 0, 0);

		// Always link new functions into beginning of chain. That way, if
		//	a new function is added into the chain from within a current
		//	callback, it will not be called until the next char. arrives.
		pstNew->pstNext = pstCLoop->pstRmtChain;
		pstCLoop->pstRmtChain = pstNew;
		pstNew->pstPrior = NULL;
		pstCLoop->fRmtChain = TRUE;
		LeaveCriticalSection(&pstCLoop->csect);
		}

	return (void *)pstNew;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopUnregisterRmtInputChain
 *
 * DESCRIPTION:
 *	Removes a function from the chain of functions called when a remote
 *	character is received.
 *
 * ARGUMENTS:
 *	pvHdl -- Value returned from an earlier call to CLoopRegisterRmtInputChain
 *
 * RETURNS:
 *	nothing
 */
void CLoopUnregisterRmtInputChain(void *pvHdl)
	{
	ST_FCHAIN *pstNode = (ST_FCHAIN *)pvHdl;
	ST_CLOOP *pstCLoop;

	assert(pstNode);

	pstCLoop = pstNode->pstParent;
	EnterCriticalSection(&pstCLoop->csect);

	// See if we are removing next scheduled chain function
	if (pstNode == pstCLoop->pstRmtChainNext)
		pstCLoop->pstRmtChainNext = pstNode->pstNext;

	// Unlink node
	if (pstNode->pstPrior)
		pstNode->pstPrior->pstNext = pstNode->pstNext;
	else
		{
		pstCLoop->pstRmtChain = pstNode->pstNext;
		if (!pstNode->pstNext)
			pstCLoop->fRmtChain = FALSE;
		}

	if (pstNode->pstNext)
		pstNode->pstNext->pstPrior = pstNode->pstPrior;

	DBGOUT_NORMAL("CLoopUnregisterRmtInputChain(0x%lx)\r\n",
				(LONG)pvHdl, 0, 0, 0, 0);

	free(pstNode);
	pstNode = NULL;
	LeaveCriticalSection(&pstCLoop->csect);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: QueryCLoopMBCSState
 *
 * DESCRIPTION: Determines the current DBCS mode.
 *	
 * ARGUMENTS:
 *	hCLoop -- Handle returned from call to CLoopCreateHandle
 *
 * RETURNS:
 *    TRUE  -  if we are in DBCS mode.
 *    FALSE -  if we're not
 */
int QueryCLoopMBCSState(HCLOOP hCLoop)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;

	assert(pstCLoop);
	return(pstCLoop->fDoMBCS);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: SetCLoopMBCSState
 *
 * DESCRIPTION:  Turns on/off DBCS mode.
 *	
 * ARGUMENTS:
 *	hCLoop -- Handle returned from call to CLoopCreateHandle
 *           fState -- TRUE,  if we're turning MBCS on,
 *                         FALSE, if we're turning MBCS off.
 *
 * RETURNS:
 *    the value of dDoMBCS from the CLoop's internal struct
 */
int SetCLoopMBCSState(HCLOOP hCLoop, int fState)
   {
	ST_CLOOP *pstCLoop = (ST_CLOOP *)hCLoop;

	assert(pstCLoop);
	pstCLoop->fDoMBCS = fState;

	return(pstCLoop->fDoMBCS);
	}

