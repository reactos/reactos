/*	File: cloop.c (created 12/27/93, JKH)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */
#include <windows.h>
#pragma hdrstop

#include <tdll\features.h>

// #define DEBUGSTR
#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\globals.h>
#include <tdll\timers.h>
#include <tdll\com.h>
#include <tdll\xfer_msc.h>
#include <emu\emu.h>
#if defined(CHARACTER_TRANSLATION)
#include <tdll\translat.hh>
#endif
#include "cloop.h"
#include "cloop.hh"
#include "tchar.h"
#include <tdll\assert.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoop
 *
 * DESCRIPTION:
 *	This function comprises the 'engine' of the engine thread. Its action
 *	is controlled by a series of control bits that are set and cleared
 *	by calls to CLoopRcvControl and CLoopControl
 *
 * ARGUMENTS:
 *	pstCLoop -- Handle returned from CLoopCreateHandle
 *
 * RETURNS:
 *	nothing
 */
DWORD WINAPI CLoop(LPVOID pvData)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)pvData;
	HCOM	   hCom;
	int 	   nRcvCount;
	TCHAR	   chData = (TCHAR)0;
	ECHAR      echData = (ECHAR)0;
	HSESSION   hSession;
	HEMU	   hEmu;
	KEY_T	   keyOut;
	ST_FCHAIN *pstCurrent;
	CHAINFUNC  pfChainFunc;
	int		   fValidChar;
#if defined(CHARACTER_TRANSLATION)
	HHTRANSLATE hTrans;
#endif
	int        nCount;
	int        nIndx;
	CHAR       chBuffer[32];		/* Yes, it is supposed to be a CHAR */
	int		   mc;

	DBGOUT_NORMAL("In CLoop(%lX)\r\n", pstCLoop,0,0,0,0);
	assert(pstCLoop);

	EnterCriticalSection(&pstCLoop->csect);

	hSession = pstCLoop->hSession;
	hCom = pstCLoop->hCom;
	hEmu = pstCLoop->hEmu;
#if defined(CHARACTER_TRANSLATION)
	hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pstCLoop->hSession);
#endif

	assert(hSession);
	assert(hCom);
	assert(hEmu);

	DBGOUT_NORMAL("CLoop got critical section\r\n", 0,0,0,0,0);
	while (!bittest(pstCLoop->afControl, CLOOP_TERMINATE))
		{
		DWORD dwRet;

		// Normally, this CLoop function owns the critical section
		//	controlling the CLoop data. We give it up at least once each
		//	time through the loop to give other threads a change to call
		//	control functions.
		LeaveCriticalSection(&pstCLoop->csect);

		// Block if there is nothing to do
		DBGOUT_YIELD("W+ ", 0,0,0,0,0);
		dwRet = WaitForMultipleObjects(DIM(pstCLoop->ahEvents),
				pstCLoop->ahEvents, FALSE, INFINITE);
		DBGOUT_YIELD("W(0x%x)", dwRet,0,0,0,0);
		EnterCriticalSection(&pstCLoop->csect);
		DBGOUT_YIELD(": ", 0,0,0,0,0);

		// See if a transfer has been initiated
		if (bittest(pstCLoop->afControl, CLOOP_TRANSFER_READY))
			{
			DBGOUT_NORMAL("CLoop calling xfer\r\n", 0,0,0,0,0);

			LeaveCriticalSection(&pstCLoop->csect);

			xfrDoTransfer(sessQueryXferHdl(hSession));
			DBGOUT_NORMAL("CLoop back from xfer\r\n", 0,0,0,0,0);

			EnterCriticalSection(&pstCLoop->csect);

			CLoopControl((HCLOOP)pstCLoop, CLOOP_CLEAR, CLOOP_TRANSFER_READY);
			}

		// If there is input waiting and we are not blocked from receiving

		if (!pstCLoop->afRcvBlocked)
			{
			// Give priority to receiving over sending since normally
			//	we will receive more data than we send (CR becomes CR LF)
			//	and since we have more control over the send rate
			// TODO: implement priority setting by adjusting rcv count
			for (nRcvCount = 10; nRcvCount--; )
				{
				fValidChar = FALSE;
				if (mComRcvChar(hCom, &chData))
					{
					DBGOUT_NORMAL("Recieved Char\r\n", 0,0,0,0,0);

					// Check for force to 7-bit ASCII
					if (pstCLoop->stWorkSettings.fASCII7)
						chData &= 0x7F;

					nCount = 0;
#if defined(CHARACTER_TRANSLATION)
					LeaveCriticalSection(&pstCLoop->csect);
					hTrans->pfnIn(hTrans->pDllHandle, 
								  chData,
								  &nCount,
								  sizeof(chBuffer),
								  chBuffer);
					EnterCriticalSection(&pstCLoop->csect);
#else
					chBuffer[0] = chData;
					nCount = 1; 
#endif

					for (nIndx = 0; nIndx < nCount; nIndx++)
						{
						if (pstCLoop->fDoMBCS)
							{
							if (pstCLoop->cLeadByte)
								{
								chData = 0;
								echData = chBuffer[nIndx] & 0xFF;
								// There must be a macro for this somewhere
								echData |= (ECHAR)(pstCLoop->cLeadByte << 8);
								pstCLoop->cLeadByte = 0;
								fValidChar = TRUE;
								}
							else
								{
								if (IsDBCSLeadByte(chBuffer[nIndx]))
									{
									pstCLoop->cLeadByte = chBuffer[nIndx];
									}
								else
									{
									echData = chBuffer[nIndx] & 0xFF;
									fValidChar = TRUE;
									}
								}
							} // if (fDoMBCS)
						else
							{
							echData 	= ETEXT(chBuffer[nIndx]);
							fValidChar 	= TRUE;
							} // else (fDoMBCS)

						if (fValidChar)
							{
#if FALSE
							// Moved up above 11-Jan-95 DLW
							// Check for force to 7-bit ASCII
							if (pstCLoop->stWorkSettings.fASCII7)
								echData &= 0x7F;
#endif

							// Character translation/stripping
							//*if ((chData = pstCLoop->panFltrIn[chData]) == -1)
							//*	continue;


							// walk remote input function chain if it exists
							if (pstCLoop->fRmtChain)
								{
								pstCLoop->pstRmtChainNext =
														pstCLoop->pstRmtChain;
								while (pstCLoop->pstRmtChainNext)
									{
									pstCurrent = pstCLoop->pstRmtChainNext;
									pstCLoop->pstRmtChainNext =
														pstCurrent->pstNext;
									pfChainFunc = pstCurrent->pfFunc;

									LeaveCriticalSection(&pstCLoop->csect);
									mc = (*pfChainFunc)(chData,
													pstCurrent->pvUserData);
									EnterCriticalSection(&pstCLoop->csect);
									if ( mc == CLOOP_DISCARD )
										break;									
									}
								}

							if ( mc != CLOOP_DISCARD )
								{
								pstCLoop->fDataReceived = TRUE;
								CLoopCharIn(pstCLoop, echData);
								}


							if (pstCLoop->afRcvBlocked)
								{
								/*
								 * This is necessary because a script may be
								 * checking input and then stopping after it
								 * finds what it is looking for.  If the loop
								 * count is still possitive, the remaining
								 * characters will "slip thru" before this
								 * loop terminates.
								 * This won't break in any obvious way, so be
								 * careful to preserve this feature if this
								 * loop is rewritten.
								 *
								 * TODO: when scripts are added, make sure this
								 * works correctly with character translation.
								 */
								// Receiving has been blocked,
								// make sure any received
								// data gets displayed.
    					        LeaveCriticalSection(&pstCLoop->csect);
						        emuComDone(hEmu);
    					        EnterCriticalSection(&pstCLoop->csect);
								break;
								}

							}
						}
					}
				else
					{
					CLoopRcvControl((HCLOOP)pstCLoop, CLOOP_SUSPEND,
							CLOOP_RB_NODATA);
					if (pstCLoop->fDataReceived)
						{
						// Notify emulator that there is a pause in the
						//	stream of received data
    					LeaveCriticalSection(&pstCLoop->csect);
						emuComDone(hEmu);
    					EnterCriticalSection(&pstCLoop->csect);

						// If we had a delay timer already running, kill it
						if (pstCLoop->htimerRcvDelay)
							TimerDestroy(&pstCLoop->htimerRcvDelay);

						// The stream of incoming data has stopped, set a
						//	 timer so we can tell if it stops long enough to
						//	 do cursor tracking etc.
						if (TimerCreate(sessQueryTimerMux(pstCLoop->hSession),
								&pstCLoop->htimerRcvDelay,
								CLOOP_TRACKING_DELAY,
								/* pstCLoop->pfRcvDelay, */
								CLoopRcvDelayProc,
								(void *)pstCLoop) != TIMER_OK)
							{
							pstCLoop->htimerRcvDelay = (HTIMER)0;
							assert(FALSE);
							}

						pstCLoop->fDataReceived = FALSE;

						}
					break;
					}
				}
			}

		// Check for outgoing data.
		if (!pstCLoop->afSndBlocked)
			{
            // (Taken verbatim from HA5G, by JMH 03-22-96):
			// This change was added to fix a deadlock problem. While sending
			// and receiving data simultaneously at high speed, it was
			// possible for us to issue a handshake stop at the same time
			// we would receive one from the host. Our code would wait
			// in the ComSendBufr call and stop processing. Because of this,
			// we would not be processing incoming data so we would never
			// clear the handshake stop we had issued to the other end. If
			// the other end was caught in the same state -- deadlock.
			// This test slows our text transmission down. We should
			// redesgin our transmission model to fix this.  jkh, 1/19/95

			// if (CLoopGetNextOutput(pstCLoop, &keyOut)) DEADWOOD:jmh 03-22-96
            if (ComSndBufrBusy(hCom) == COM_BUSY)
                Sleep(10);              // jkh  04/29/1998 don't peg CPU
			else if (CLoopGetNextOutput(pstCLoop, &keyOut))
				{
				// Check for tab expansion
				//DbgOutStr("C", 0,0,0,0,0);
				if (keyOut == TEXT('\t') &&
						pstCLoop->stWorkSettings.fExpandTabsOut &&
						pstCLoop->stWorkSettings.nTabSizeOut)
					{
					//* int   i;
					//* POINT pt;

					//* mEmuGetCursorPos(
					//*			mGetEmulatorHdl(pstCLoop->hSession),
					//*			&pt);

					//* i = pstCLoop->stWorkSettings.usTabSizeOut -
					//* 	((pt.x + 1) % pstCLoop->stWorkSettings.usTabSizeOut);

					//* while (i-- > 0)
					//* 	(VOID)mEmuKbdIn(hEmu, (KEY_T)TEXT(' '), FALSE);
					}

				else
					{
                    //jmh 03-22-96 We need to unlock cloop around calls to
                    // emuKbdIn and emuDataIn, because they call the com
                    // thread, which may be stuck waiting to access cloop.
                    //
					LeaveCriticalSection(&pstCLoop->csect);
					emuKbdIn(hEmu, keyOut, FALSE);
					EnterCriticalSection(&pstCLoop->csect);
					}


				if (keyOut == TEXT('\r'))
					{
					if (pstCLoop->stWorkSettings.fLineWait)
						{
						CLoopSndControl((HCLOOP)pstCLoop, CLOOP_SUSPEND,
								CLOOP_SB_LINEWAIT);
						}

					if (pstCLoop->stWorkSettings.fSendCRLF)
						{
                        //DEADWOOD:jkh 8/18/97 
                        // Can't use CLoopSend here because the '\n' is placed
                        // behind any other codes waiting in the output queue.
                        // This caused all LF codes in a text file send to be
                        // sent AFTER the whole file, rather than after each CR

						// CLoopSend((HCLOOP) pstCLoop, TEXT("\n"), 1, 0);

    					LeaveCriticalSection(&pstCLoop->csect);
						emuKbdIn(hEmu, (KEY_T)'\n', FALSE);
    					EnterCriticalSection(&pstCLoop->csect);
#if 0 //DEADWOOD: RDE 20AUG98 MPT Fixed a local echo problem down in 
      //               CloopCharIn which then caused an extra linefeed
      //               to be displayed on the local terminal.
                        if (pstCLoop->stWorkSettings.fLocalEcho &&
								!pstCLoop->fSuppressDsp)
							{
							//jmh 03-22-96 We need to unlock cloop around calls to
							// emuKbdIn and emuDataIn, because they call the com
							// thread, which may be stuck waiting to access cloop.
							//
        					LeaveCriticalSection(&pstCLoop->csect);
							emuDataIn(hEmu, TEXT('\n'));
							emuComDone(hEmu);
        					EnterCriticalSection(&pstCLoop->csect);
							}
#endif
                        }

					if (pstCLoop->stWorkSettings.nLineDelay)
						{
						if (TimerCreate(
									sessQueryTimerMux(pstCLoop->hSession),
									&pstCLoop->htimerCharDelay,
									pstCLoop->stWorkSettings.nLineDelay,
									pstCLoop->pfCharDelay,
									(void *)pstCLoop) == TIMER_OK)
							CLoopSndControl((HCLOOP)pstCLoop, CLOOP_SUSPEND,
									CLOOP_SB_DELAY);
						}
					}
				else
					{
					if (pstCLoop->stWorkSettings.nCharDelay)
						{
						if (TimerCreate(
									sessQueryTimerMux(pstCLoop->hSession),
									&pstCLoop->htimerCharDelay,
									pstCLoop->stWorkSettings.nCharDelay,
									pstCLoop->pfCharDelay,
									(void *)pstCLoop) == TIMER_OK)
							CLoopSndControl((HCLOOP)pstCLoop, CLOOP_SUSPEND,
									CLOOP_SB_DELAY);
						}
					}


				//* if (pstCLoop->lpLearn)
				//* 	{
				//* 	CLearnSendChar(pstCLoop->lpLearn, (METACHAR)keyOut);
				//* 	}
				}
			}

		// Make sure any buffered output waiting gets sent.
		//* ComSendPush(pstCLoop->hCom);

		}
	DBGOUT_NORMAL("Leaving CLoop(%lX)\r\n", pstCLoop,0,0,0,0);
	return 0;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopCharIn
 *
 * DESCRIPTION:
 *	Does rec eive processing for characters received from the remote system
 *
 * ARGUMENTS:
 *	pstCLoop -- Handle returned from CLoopCreateHandle
 *	mc		 -- Character to be processed.
 *
 * RETURNS:
 *	nothing
 */
void CLoopCharIn(ST_CLOOP *pstCLoop, ECHAR chIn)
	{
	// Check for echoplex
	if (pstCLoop->stWorkSettings.fEchoplex)
		{
		
		//mpt:1-27-98 converting echars to tchar was converting
		//            the character to a single byte. Consequently,
		//            echoing of a dbcs character was only echoing
		//            the second byte of the character.
		if ( (pstCLoop->fDoMBCS) && (chIn > 255) )
			{
			//send first byte
			ComSendCharNow(pstCLoop->hCom, (TCHAR) (chIn >> 8));
			//send second byte
			ComSendCharNow(pstCLoop->hCom, (TCHAR) (chIn & 0xFF));
			}
		else
			{
			ComSendCharNow(pstCLoop->hCom, (TCHAR) chIn);
			}

		if (chIn == ETEXT('\r'))
			{
			ComSendCharNow(pstCLoop->hCom, TEXT('\n'));
			}
		}

	if (pstCLoop->stWorkSettings.fLineWait &&
			chIn == pstCLoop->stWorkSettings.chWaitChar)
		CLoopSndControl((HCLOOP)pstCLoop, CLOOP_RESUME, CLOOP_SB_LINEWAIT);

	// Display character in normal or image mode
	if (!pstCLoop->fSuppressDsp)
		{
		// DbgOutStr("Dsp %02X (%c)\r\n", chIn, chIn, 0,0,0);
		emuDataIn(pstCLoop->hEmu, chIn);
		if (chIn == ETEXT('\r') && pstCLoop->stWorkSettings.fAddLF)
			emuDataIn(pstCLoop->hEmu, ETEXT('\n'));
		}
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopCharOut
 *
 * DESCRIPTION:
 *	Called to send a character out the port with processing by the CLoop
 *	routines. Note that this differs from CLoopSend.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void CLoopCharOut(HCLOOP hCLoop, TCHAR chOut)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;
	int fCharValid = FALSE;
	ECHAR echOut = (ECHAR)0;
#if defined(CHARACTER_TRANSLATION)
	HHTRANSLATE hTrans;
	int        nCount;
	int        nIndx;
	CHAR       chBuffer[32];		/* Yes, it is supposed to be a CHAR */

	hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pstCLoop->hSession);

	(*hTrans->pfnOut)(hTrans->pDllHandle, 
					chOut,
					&nCount,
					sizeof(chBuffer),
					chBuffer);

	for (nIndx = 0; nIndx < nCount; nIndx += 1)
		{
		chOut = chBuffer[nIndx];
#endif

		ComSendCharNow(pstCLoop->hCom, chOut);
		if (pstCLoop->stWorkSettings.fLocalEcho &&
				!pstCLoop->fSuppressDsp)
			{
			if (pstCLoop->fDoMBCS)
				{
				if (pstCLoop->cLocalEchoLeadByte)
					{
					echOut = chOut & 0xFF;
					echOut |= (ECHAR)(pstCLoop->cLocalEchoLeadByte << 8);
					pstCLoop->cLocalEchoLeadByte = 0;
					fCharValid = TRUE;
					}
				else
					{
					if (IsDBCSLeadByte(chOut))
						{
						pstCLoop->cLocalEchoLeadByte = chOut;
						}
					else
						{
						echOut = chOut & 0xFF;
						fCharValid = TRUE;
						}
					}
				}
			else
				{
				echOut = (ECHAR)chOut;
                fCharValid = TRUE;
				}

			if (fCharValid)
				{
				emuDataIn(pstCLoop->hEmu, echOut);
				emuComDone(pstCLoop->hEmu);
				}
			}
#if defined(CHARACTER_TRANSLATION)
		}
#endif

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CLoopBufrOut
 *
 * DESCRIPTION:
 *	Called to send a buffer of characters out the port with processing by the CLoop
 *	routines. Note that this differs from CLoopSend.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *	void
 *
 */
void CLoopBufrOut(HCLOOP hCLoop, TCHAR *pchOut, int nLen)
	{
	ST_CLOOP * const pstCLoop = (ST_CLOOP *)hCLoop;
	int fCharValid = FALSE;

	while (nLen--)
		{
		ComSendChar(pstCLoop->hCom, *pchOut);	// place chars in comsend buffer
		if (pstCLoop->stWorkSettings.fLocalEcho &&
				!pstCLoop->fSuppressDsp)
			{
			if (pstCLoop->fDoMBCS)
				{
				if (pstCLoop->cLocalEchoLeadByte != 0)
					{
					*pchOut |= (TCHAR)(pstCLoop->cLocalEchoLeadByte << 8);
					pstCLoop->cLocalEchoLeadByte = 0;
					fCharValid = TRUE;
					}
				else
					{
					if (IsDBCSLeadByte(*pchOut))
						{
						pstCLoop->cLocalEchoLeadByte = *pchOut;
						}
					else
						{
						fCharValid = TRUE;
						}
					}
				}
			else
				{
				fCharValid = TRUE;
				}

			if (fCharValid)
				{
				emuDataIn(pstCLoop->hEmu, *pchOut);
				emuComDone(pstCLoop->hEmu);
				}
			}
		++pchOut;
		}
	ComSendPush(pstCLoop->hCom);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopRcvDelayProc
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
/* ARGSUSED */
void CALLBACK CLoopRcvDelayProc(void *pvData, long lSince)
	{
	// This timer was set the last time data input stopped. If no data
	//	has been received since, notify the display routines so they can
	//	do cursor tracking or whatever silly thing it is they do.
	ST_CLOOP *pstCLoop = (ST_CLOOP *)pvData;

	EnterCriticalSection(&pstCLoop->csect);
	if (!pstCLoop->fDataReceived)
		{
		emuTrackingNotify(pstCLoop->hEmu);
		}
	TimerDestroy(&pstCLoop->htimerRcvDelay);

	(void)&lSince;		// avoid compiler warnings
	LeaveCriticalSection(&pstCLoop->csect);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: CLoopCharDelayProc
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
/* ARGSUSED */
void CALLBACK CLoopCharDelayProc(void *pvData, long lSince)
	{
	ST_CLOOP *pstCLoop = (ST_CLOOP *)pvData;

	EnterCriticalSection(&pstCLoop->csect);

	TimerDestroy(&pstCLoop->htimerCharDelay);
	CLoopSndControl((HCLOOP)pstCLoop, CLOOP_RESUME, CLOOP_SB_DELAY);

	LeaveCriticalSection(&pstCLoop->csect);

	(void)&lSince;		// avoid compiler warnings
	}
