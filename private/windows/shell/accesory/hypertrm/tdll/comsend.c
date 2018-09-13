/* ComSend -- Text sending routines for HyperACCESS
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */
#include <windows.h>
#pragma hdrstop

// #define DEBUGSTR

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include "com.h"
#include "comdev.h"
#include "com.hh"

/* --- Internal prototypes --- */

static int ComSendCheck(const HCOM pstCom, const int fDataWaiting);



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendChar
 *
 * DESCRIPTION:
 *	Adds a character to the send buffer to be transmitted. The
 *	character will not actually be transferred to the transmit
 *	routines until the buffer fills up or a call to ComSendCharNow
 *	is made or a call to ComSendPush is made while the transmitter
 *	is not busy.
 *
 * ARGUMENTS:
 *	pstCom	  -- handle to comm session
 *	uchCode -- The character to be transmitted.
 *
 * RETURNS:
 *	COM_OK if the character is successfully buffered.
 *	COM_INVALID_HANDLE if invalid com handle
 *	COM_SEND_BUFFER_FULL if the buffer is full and the
 *		caller-supplied handshake function returns a code
 *		indicating that waiting data should be discarded.
 */
int ComSendChar(const HCOM pstCom, const TCHAR chCode)
	{
	assert(ComValidHandle(pstCom));

	while (pstCom->nSendCount >= pstCom->nSBufrSize)
		{
		/* wait until there is room in buffer or we're told to give up. */
		if (ComSendCheck(pstCom, TRUE) != COM_OK)
			return FALSE;
		if (pstCom->nSendCount >= pstCom->nSBufrSize)
			(void)ComSndBufrWait(pstCom, 2);
		}

	/* Place char in buffer and assume it will get launched later. */

	*pstCom->puchSendPut++ = chCode;
	++pstCom->nSendCount;
	return TRUE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendCharNow
 *
 * DESCRIPTION:	Adds a character to the send buffer and then waits to make
 *				sure the send buffer gets passed to the transmission routine.
 *				This function does NOT wait until the character is actually
 *				transmitted. Handshaking may still delay actual transmission
 *				but no subsequent calls to any ComSend??? routines are needed
 *				to get the character on its way. ComSendWait can be used to
 *				wait until all characters are actually out the port.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *				chCode -- The character to be transmitted.
 *
 * RETURNS:		COM_OK if the character is successfully buffered and passed to
 *					 the transmission routines.
 *				COM_INVALID_HANDLE if invalid com handle
 *				COM_SEND_QUEUE_STUCK if the caller-supplied handshake function
 *					 returns a code indicating that waiting data should be
 *					 discarded before the buffer can be	queued for transmission.
 */
int ComSendCharNow(const HCOM pstCom, const TCHAR chCode)
	{
	assert(ComValidHandle(pstCom));

	while (pstCom->nSendCount >= pstCom->nSBufrSize)
		{
		/* buffer is full, wait until there is room or we are
		 *	 told to give up
		 */
		if (ComSendCheck(pstCom, TRUE) != COM_OK)
			return FALSE;
		if (pstCom->nSendCount >= pstCom->nSBufrSize)
			ComSndBufrWait(pstCom, 2);
		}

	*pstCom->puchSendPut++ = chCode;
	++pstCom->nSendCount;

	/* wait until local buffer is passed to SndBufr or we are
	 * told to give up
	 */
	while (pstCom->nSendCount > 0)
		{
		// This will pass buffer to SndBufr as soon as possible
		if (ComSendCheck(pstCom, TRUE) != COM_OK)
			return FALSE;
		if (pstCom->nSendCount > 0)
			ComSndBufrWait(pstCom, 2);
		}

	return TRUE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendPush
 *
 * DESCRIPTION:	This routine should be called periodically by any code that
 *				uses ComSendChar() when there are no characters to be
 *				transmitted immediately. Calling this function accomplishes
 *				two things.
 *	1. It will cause any buffered send characters to be passed to the actual
 *		transmission routines as soon as they are not busy.
 *	2. It will cause the caller-registered handshake handler function to be
 *		called if transmission is suspended by handshaking.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *
 * RETURNS:		same as ComSendCheck()
 */
int ComSendPush(const HCOM pstCom)
	{
	return ComSendCheck(pstCom, FALSE);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendWait
 *
 * DESCRIPTION:	This function waits until all buffered send data is actually
 *				passed to the transmit hardware or until the handshake handling
 *				function returns a code indicating that data should be discared.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *	none
 *
 * RETURNS:		COM_OK if all data has been transmitted.
 *				COM_SEND_QUEUE_STUCK if a handshake handling function
 *					 indicated that data should be discarded.
 */
int ComSendWait(const HCOM pstCom)
	{
	assert(ComValidHandle(pstCom));

	while (pstCom->nSendCount > 0 || ComSndBufrWait(pstCom, 2) != COM_OK)
		{
		if (ComSendCheck(pstCom, FALSE) != COM_OK)
			{
			return COM_SEND_QUEUE_STUCK;
			}
		}
	return COM_OK;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendClear
 *
 * DESCRIPTION:	Clears all data waiting for tranmission, both in the local
 *				ComSend buffer and the SndBufr buffer currently being
 *				transmitted.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *
 * RETURNS: 	always returns COM_OK
 */
int ComSendClear(const HCOM pstCom)
	{
	assert(ComValidHandle(pstCom));

	ComSndBufrClear(pstCom);
	pstCom->puchSendPut = pstCom->puchSendBufr;
	pstCom->nSendCount = 0;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendSetStatusFunction
 *
 * DESCRIPTION:	Registers a function to be called to handle handshaking status
 *				displays, timeouts, etc. while sending.
 *
 *	The registered function is called when it is registered, when it is being
 *	replaced, and during sending when a handshaking suspension is detected.
 *	Normally, the function is not called if transmission is not suspended.
 *  After being called one or more times with a suspension, though, it will
 *	be called one additional time after the suspension clears to allow the
 *	function to clear any visible indicators.
 *
 *	The registered function is passed the following arguments
 *		usReason	-- Contains a code indicating the reason the function
 *					   was called. It will be one of:
 *					   COMSEND_FIRSTCALL -- if function is being installed
 *					   COMSEND_LASTCALL  -- if function is being replaced
 *					   COMSEND_DATA_WAITING -- if there is data waiting
 *							that will not fit in the send buffer.
 *					   COMSEND_NORMAL	 -- if called due to handshake
 *							condition but no data is in danger of being lost.
 *		fusHsStatus -- A value contining bits which indicate what transmission
 *							is waiting for. The bits are defined in com.h as
 *							COMSB_WAIT_XXX.
 *		lDelay		-- The amount of time in tenths of seconds since
 *							transmission was suspended. This time will not
 *							begin incrementing until there is data to transmit.
 *
 *	The registered function should return a value indicating what action the
 *	  ComSend routines should take regarding handshake suspensions:
 *		COMSEND_OK					no action, if data is waiting, keep waiting
 *		COMSEND_GIVEUP				if data is waiting, discard it and return
 *									  from ComSend??? call.
 *		COMSEND_CLEAR_DATA			discard all transmit buffers, this discards
 *									  any data waiting in a ComSend command
 *									  AND any data previously buffered.
 *		COMSEND_FORCE_CONTINUATION	force data to be transmitted, if waiting
 *									  for XON, pretend it was received. If
 *									  waiting for hardware handshake, disable
 *									  it. ComSend routine will continue trying
 *									  to send any waiting data.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *				pfNewStatusFunct -- A pointer to a function matching the specs
 *					 described above or NULL if a default, do-nothing function
 *					 should be used. If the default function is used, ComSend
 *					 commands will essentially wait forever to send data.
 *				ppfOldStatusFunct -- Address of pointer to put the pointer to
 *					 the previously registered function
 *
 * RETURNS:		COM_OK if everything went ok
 *				COM_INVALID_HANDLE if invalid com handle
 */
int ComSendSetStatusFunction(const HCOM pstCom, STATUSFUNCT pfNewStatusFunct,
			 STATUSFUNCT *ppfOldStatusFunct)
	{

	STATUSFUNCT pfHold = pstCom->pfUserFunction;
	unsigned afXmitStatus;
	long   lHandshakeDelay;

	assert(ComValidHandle(pstCom));

	/* If user want's no status function, use an internal function to
	 *	 avoid constant checks for null
	 */
	if (pfNewStatusFunct == NULL)
		{
		pfNewStatusFunct = ComSendDefaultStatusFunction;
		}

	if (pfNewStatusFunct != pfHold)
		{
		ComSndBufrQuery(pstCom, &afXmitStatus, &lHandshakeDelay);

		/* call old function to give it a change to clear up details */
		(void)(*pfHold)(COMSEND_LASTCALL, afXmitStatus, lHandshakeDelay);

		/* call new function so it can initialize */
		pstCom->pfUserFunction = pfNewStatusFunct;
		(void)(*(pstCom->pfUserFunction))(COMSEND_FIRSTCALL, afXmitStatus,
				lHandshakeDelay);
		}

	if (ppfOldStatusFunct)
		*ppfOldStatusFunct = pfHold;

	return COM_OK;
	}



/* * * * * * * * * * * *
 *	Private functions  *
 * * * * * * * * * * * */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendDefaultStatusFunction
 *
 * DESCRIPTION:	This function is used as the handshake handling function when
 *				no caller-supplied function is available, that is, at program
 *				start up or when the caller registeres the NULL function.
 *
 * ARGUMENTS:	See description of handler in ComSendSetStatusFunction
 *
 * RETURNS: 	See description of handler in ComSendSetStatusFunction
 */
int ComSendDefaultStatusFunction(int iReason, unsigned afHsStatus,
		long lDelay)
	{
	/* suppress complaints from lint and the compiler */
	iReason = iReason;
	afHsStatus = afHsStatus;
	lDelay = lDelay;

	/* This function does nothing, it is here to have something to point
	 * pfUserFunction to when ComSendSetStatusFunction is called with
	 * NULL argument.
	 */
	return COM_OK;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * ComSendCheck
 *
 * DESCRIPTION:	This function is used internally to keep the flow of
 *				transmitted data moving. It handles setting up and calling the
 *				handshake handling functions and getting the local transmit
 *				buffer passed to the SndBufr routines when they are ready.
 *
 * ARGUMENTS:	pstCom -- handle to comm session
 *				fDataWaiting -- TRUE if being called by a function that has
 *					 data to place in the send buffer when the buffer is full.
 *
 * RETURNS:		COM_OK if the calling function should continue waiting for
 *					 space in the transmit buffer.
 *				COM_INVALID_HANDLE if invalid com handle
 *				COM_SEND_QUEUE_STUCK if the calling function should discard
 *					 any unbuffered data and return.
 */
static int ComSendCheck(const HCOM pstCom, const int fDataWaiting)
	{
	int 	 fResult = TRUE;
	unsigned afXmitStatus;
	long	 lHandshakeDelay;

	if (ComSndBufrBusy(pstCom) != COM_OK)
		{
		ComSndBufrQuery(pstCom, &afXmitStatus, &lHandshakeDelay);

		if (afXmitStatus != 0)
			{
			switch((*(pstCom->pfUserFunction))(fDataWaiting ?
					COMSEND_DATA_WAITING : COMSEND_NORMAL,
					afXmitStatus, lHandshakeDelay))
				{
			case COMSEND_OK:
				break;

			case COMSEND_GIVEUP:
				fResult = FALSE;
				break;

			case COMSEND_CLEAR_DATA:
				ComSendClear(pstCom);
				fResult = FALSE;
				break;

#if 0	//* this should be replaced with a more general mechanism
			case COMSEND_FORCE_CONTINUATION:
				if (bittest(afXmitStatus, COMSB_WAIT_XON))
					ComSendXon(pstCom);
				else if (bittest(afXmitStatus,
						(COMSB_WAIT_CTS | COMSB_WAIT_DSR | COMSB_WAIT_DCD)))
					{
					// TODO: this will be replaced by ComSndBufrForce or such
					// (VOID)ComDisableHHS(pstCom);
					}
				else if (bittest(afXmitStatus, COMSB_WAIT_BUSY))
					{
					ComSendClear(pstCom);
					fResult = FALSE;
					}
				break;
#endif
			default:
				assert(FALSE);
				break;
				}

			pstCom->fUserCalled = TRUE;
			}
		else if (pstCom->fUserCalled)
			{
			(void)(*(pstCom->pfUserFunction))(COMSEND_NORMAL, 0, 0L);
			pstCom->fUserCalled = FALSE;
			}
		}
	else
		{
		if (pstCom->nSendCount > 0)
			{
			ComSndBufrSend(pstCom, pstCom->puchSendBufr, pstCom->nSendCount, 1);
			pstCom->puchSendBufr = pstCom->puchSendPut =
					((pstCom->puchSendBufr == pstCom->puchSendBufr1) ?
							pstCom->puchSendBufr2 : pstCom->puchSendBufr1);
			pstCom->nSendCount = 0;
			}

		if (pstCom->fUserCalled)
			{
			(void)(*(pstCom->pfUserFunction))(COMSEND_NORMAL, 0, 0L);
			pstCom->fUserCalled = FALSE;
			}
		}

	return(fResult ? COM_OK : COM_SEND_QUEUE_STUCK);
	}



/********************** end of comsend.c ***********************/
