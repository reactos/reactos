/*	File: D:\WACKER\cnctstd\cncts.c (Created: 19-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:00p $
 */

#include <windows.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\cnct.h>
#include <tdll\com.h>
#include "cnctdrv.hh"

static BOOL ProcessEvent(const HHDRIVER hhDriver);
static void SetStatus(const HHDRIVER hhDriver, const int iStatus);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvCnctLoop
 *
 * DESCRIPTION:
 *	This is the connection loop which runs as its own thread.  It waits
 *	for events such as a match or a request to disconnect and executes
 *	the request according to the current execution state.
 *
 * ARGUMENTS:
 *	hhDriver	- private connection driver handle
 *
 * RETURNS:
 *	0 or error code
 *
 */
DWORD WINAPI ConnectLoop(const HHDRIVER hhDriver)
	{
	DWORD dwEvent;
	HANDLE ahEvents[2];

	cnctdrvLock(hhDriver);

	/* --- Create event objects for matching and disconnect --- */

	ahEvents[0] = hhDriver->hDiscnctEvent = CreateEvent(0, FALSE, FALSE, 0);

	if (hhDriver->hDiscnctEvent == 0)
		{
		assert(FALSE);
		cnctdrvUnlock(hhDriver);
		return (DWORD)CNCT_ERROR;
		}

	ahEvents[1] = hhDriver->hMatchEvent = CreateEvent(0, FALSE, TRUE, 0);

	if (hhDriver->hMatchEvent == 0)
		{
		assert(FALSE);
		CloseHandle(hhDriver->hDiscnctEvent);
		cnctdrvUnlock(hhDriver);
		return (DWORD)CNCT_ERROR;
		}

	/* --- Main Connection Loop --- */

	hhDriver->dwTime = INFINITE;
	hhDriver->iState = STATE_START;

	for (;;)
		{
		cnctdrvUnlock(hhDriver);

		dwEvent = WaitForMultipleObjects(2, ahEvents, FALSE, hhDriver->dwTime);

		cnctdrvLock(hhDriver);

		switch (dwEvent)
			{
		case WAIT_OBJECT_0+0:
			hhDriver->iState = STATE_DISCONNECT;
			/* --------------------------------------- */
			/* --- Fall through to WAIT_OBJECT_0+1 --- */
			/* --------------------------------------- */

		case WAIT_OBJECT_0+1:
			if (ProcessEvent(hhDriver) == FALSE)
				goto EXIT_THREAD;

			break;

		case WAIT_TIMEOUT:
			// do a quick and dirty disconnect maybe
			assert(FALSE);
			goto EXIT_THREAD;

		case WAIT_ABANDONED_0 + 0:
		case WAIT_ABANDONED_0 + 1:
			// do a quick and dirty disconnect
			assert(FALSE);
			goto EXIT_THREAD;

		case WAIT_FAILED:
			// do a quick and dirty disconnect
			assert(FALSE);
			goto EXIT_THREAD;

		default:
			assert(FALSE);
			goto EXIT_THREAD;
			}
		}

	EXIT_THREAD:

	CloseHandle(hhDriver->hDiscnctEvent);
	hhDriver->hDiscnctEvent = 0;

	CloseHandle(hhDriver->hMatchEvent);
	hhDriver->hMatchEvent = 0;

	cnctdrvUnlock(hhDriver);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ProcessMatchEvent
 *
 * DESCRIPTION:
 *	Handles the match process.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *
 * RETURNS:
 *	BOOL
 *
 */
static BOOL ProcessEvent(const HHDRIVER hhDriver)
	{
	switch (hhDriver->iState)
		{
	case STATE_START:
		if (ComActivatePort(sessQueryComHdl(hhDriver->hSession)) != COM_OK)
			{
			assert(FALSE);
			return FALSE;
			}

		// For now we're doing direct connects only
		//
		SetStatus(hhDriver, CNCT_STATUS_TRUE);
		break;

	case STATE_DISCONNECT:
		if (hhDriver->iStatus == CNCT_STATUS_FALSE)
			break;

		SetStatus(hhDriver, CNCT_STATUS_FALSE);
		ComDeactivatePort(sessQueryComHdl(hhDriver->hSession));
		break;

	default:
		assert(FALSE);
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	SetStatus
 *
 * DESCRIPTION:
 *	There's actually more to setting the connection status than just
 *	setting the status variable as the code below indicates.  Dumb
 *	question:  Why aren't there any locks in this code.  Dumb Answer:
 *	This function is only called from the ConnectLoop thread context
 *	which has already locked things down.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	iStatus 	- new status
 *
 * RETURNS:
 *	void
 *
 */
static void SetStatus(const HHDRIVER hhDriver, const int iStatus)
	{
	/* --- Don't do things twice --- */

	if (iStatus == hhDriver->iStatus)
		return;

	/* --- Set the status, an exciting new adventure game --- */

	switch (iStatus)
		{
	case CNCT_STATUS_TRUE:
		hhDriver->iStatus = CNCT_STATUS_TRUE;
		NotifyClient(hhDriver->hSession, EVENT_CONNECTION_OPENED, 0);
		break;

	case CNCT_STATUS_CONNECTING:
		hhDriver->iStatus = CNCT_STATUS_CONNECTING;
		break;

	case CNCT_STATUS_DISCONNECTING:
		hhDriver->iStatus = CNCT_STATUS_DISCONNECTING;
		break;

	case CNCT_STATUS_FALSE:
		hhDriver->iStatus = CNCT_STATUS_FALSE;
		NotifyClient(hhDriver->hSession, EVENT_CONNECTION_CLOSED, 0);
		break;

	default:
		assert(FALSE);
		break;
		}

	return;
	}
