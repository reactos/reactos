/*	File: D:\WACKER\cnctstd\cnctdrvs.c (Created: 19-Jan-1994)
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
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\cnct.h>

#include "cnctdrv.hh"

BOOL WINAPI cnctdrvEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvEntry
 *
 * DESCRIPTION:
 *	Currently, just initializes the C-Runtime library but may be used
 *	for other things later.
 *
 * ARGUMENTS:
 *	hInstDll	- Instance of this DLL
 *	fdwReason	- Why this entry point is called
 *	lpReserved	- reserved
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL WINAPI cnctdrvEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
	{
	return _CRT_INIT(hInstDll, fdwReason, lpReserved);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvCreate
 *
 * DESCRIPTION:
 *	Initializes the connection driver and returns a handle to the driver
 *	if successful.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	Handle to driver if successful, else 0.
 *
 */
HDRIVER WINAPI cnctdrvCreate(const HCNCT hCnct, const HSESSION hSession)
	{
	HHDRIVER hhDriver;

	if (hCnct == 0)
		{
		assert(FALSE);
		return 0;
		}

	hhDriver = malloc(sizeof(*hhDriver));

	if (hhDriver == 0)
		{
		assert(FALSE);
		return 0;
		}

	memset(hhDriver, 0, sizeof(*hhDriver));
	hhDriver->hCnct = hCnct;
	hhDriver->hSession = hSession;
	hhDriver->iStatus = CNCT_STATUS_FALSE;

	InitializeCriticalSection(&hhDriver->cs);

	return (HDRIVER)hhDriver;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvDestroy
 *
 * DESCRIPTION:
 *	Destroys a connection driver handle.
 *
 * ARGUMENTS:
 *	hhDriver - private driver handle.
 *
 * RETURNS:
 *	0 or error code
 *
 */
int WINAPI cnctdrvDestroy(const HHDRIVER hhDriver)
	{
	if (hhDriver == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	// Disconnect if we're connected or in the process.
	// Note: cnctdrvDisconnect should terminate the thread.

	cnctdrvDisconnect(hhDriver, 0);

	/* --- Wait for connection loop thread to terminate --- */

	if (hhDriver->hThread)
		WaitForSingleObject(hhDriver->hThread, 10000);

	/* --- Free any allocated resources --- */

	if (hhDriver->hThread)
		CloseHandle(hhDriver->hThread);

	DeleteCriticalSection(&hhDriver->cs);
	free(hhDriver);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvLock
 *
 * DESCRIPTION:
 *	Locks the connection driver's critical section semaphore.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *
 * RETURNS:
 *	void
 *
 */
void cnctdrvLock(const HHDRIVER hhDriver)
	{
	EnterCriticalSection(&hhDriver->cs);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvUnlock
 *
 * DESCRIPTION:
 *	Unlocks the connection driver's critical section semaphore.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *
 * RETURNS:
 *	void
 *
 */
void cnctdrvUnlock(const HHDRIVER hhDriver)
	{
	LeaveCriticalSection(&hhDriver->cs);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvQueryStatus
 *
 * DESCRIPTION:
 *	Returns the current connection status as defined in <tdll\cnct.h>
 *
 * ARGUMENTS:
 *	hhDriver - private driver handle
 *
 * RETURNS:
 *	connection status or error code
 *
 */
int WINAPI cnctdrvQueryStatus(const HHDRIVER hhDriver)
	{
	int iStatus;

	if (hhDriver == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	cnctdrvLock(hhDriver);
	iStatus = hhDriver->iStatus;   //* hard-code for now.
	cnctdrvUnlock(hhDriver);

	return iStatus;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvConnect
 *
 * DESCRIPTION:
 *	Attempts to dial the modem.
 *
 * ARGUMENTS:
 *	hhDriver - private driver handle
 *	uFlags	 - connection flags
 *
 * RETURNS:
 *	0 or error
 *
 */
int WINAPI cnctdrvConnect(const HHDRIVER hhDriver, const unsigned int uFlags)
	{
	DWORD dwThreadId;

	if (hhDriver == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	/* --- Check to see we're not already connected --- */

	if (cnctdrvQueryStatus(hhDriver) != CNCT_STATUS_FALSE)
		{
		assert(FALSE);
		return CNCT_ERROR;
		}

	cnctdrvLock(hhDriver);

	/* --- If we created a thread before, then close handle --- */

	if (hhDriver->hThread)
		CloseHandle(hhDriver->hThread);

	/* --- Create the connection loop ---*/

	hhDriver->hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)
		ConnectLoop, hhDriver, 0, &dwThreadId);

	if (hhDriver->hThread == 0)
		{
		assert(FALSE);
		cnctdrvUnlock(hhDriver);
		return CNCT_ERROR;
		}

	cnctdrvUnlock(hhDriver);
	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvDisconnect
 *
 * DESCRIPTION:
 *	Signals a disconnect
 *
 * ARGUMENTS:
 *	hhDriver - private driver handle
 *	uFlags	 - disconnect flags
 *
 * RETURNS:
 *	0 or error
 *
 */
int WINAPI cnctdrvDisconnect(const HHDRIVER hhDriver, const unsigned int uFlags)
	{
	if (hhDriver == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	cnctdrvLock(hhDriver);

	if (hhDriver->hDiscnctEvent)
		SetEvent(hhDriver->hDiscnctEvent);

	cnctdrvUnlock(hhDriver);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvComEvent
 *
 * DESCRIPTION:
 *	Com routines call the this to notify connection routines that some
 *	significant event has happened (ie. carrier lost).	The connetion
 *	driver decides what it is interested in knowing however by
 *	querying the com drivers for the specific data.
 *
 * ARGUMENTS:
 *	hhDriver	- private connection driver handle
 *
 * RETURNS:
 *	0
 *
 */
int WINAPI cnctdrvComEvent(const HHDRIVER hhDriver)
	{
	if (hhDriver == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	return 0;
	}
