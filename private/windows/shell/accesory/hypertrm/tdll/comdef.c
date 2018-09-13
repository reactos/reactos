/* comdef.c -- Default com driver routines.
 *			   These routines are pointed to by various com function pointers
 *			   before a valid device driver is loaded.
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */
#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>
#include <tdll\assert.h>
#include <tdll\sf.h>
#include "com.h"
#include "comdev.h"
#include "com.hh"

// since these are all dummy functions that serve as place-holders for
//	real functions during the time when no driver is loaded, they often
//	do not use the arguments passed to them. The following line will
//	suppress the warnings that lint issues
/*lint -e715*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefDoNothing
 *
 * DESCRIPTION:
 *	A filler function that various com function pointers can be set to
 *	until they receive actual values from a com driver. This prevents
 *	the function pointers from pointing to invalid code.
 *
 * ARGUMENTS:
 *	pvDriverData -- Data being passed to driver
 *
 * RETURNS:
 *	always returns COM_PORT_NOT_OPEN;
 */
int WINAPI ComDefDoNothing(void *pvDriverData)
	{
	pvDriverData = pvDriverData;

	return COM_PORT_NOT_OPEN;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
int WINAPI ComDefPortPreconnect(void *pvDriverData,
		TCHAR *pszPortName,
		HWND hwndParent)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;
	pszPortName = pszPortName;
	hwndParent = hwndParent;

	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefDeviceDialog
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
int WINAPI ComDefDeviceDialog(void *pvDriverData, HWND hwndParent)
	{
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
int WINAPI ComDefPortActivate(void *pvDriverData,
			TCHAR *pszPortName,
			DWORD_PTR dwMediaHdl)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;
	pszPortName = pszPortName;
	dwMediaHdl = dwMediaHdl;

	return COM_DEVICE_INVALID;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefBufrRefill
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
int WINAPI ComDefBufrRefill(void *pvDriverData)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;

	return FALSE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefSndBufrSend
 *
 * DESCRIPTION:
 *	Starts background transmission of a buffer of characters. Optionally
 *	waits for any prior buffer transmission to be completed.
 *
 * ARGUMENTS:
 *	pvDriverData
 *	pchBufr -- Pointer to buffer of characters to be transmitted
 *	usCount -- Number of characters in buffer
 *	usWait	-- Time (in tenths of a second) to wait for any prior,
 *				unfinished buffer to finish. If this value is zero, the call
 *				will fail immediately if the transmitter is busy.
 *
 * RETURNS:
 *	COM_OK if transmission is started. Note that this call returns as soon
 *			as the transmission is started -- it does not wait for the entire
 *			transmission to be completed.
 *	COM_BUSY if transmitter is busy with a prior buffer and time limit is
 *			exceeded
 *	COM_PORT_NOT_OPEN if called when no port is active
 */
int WINAPI ComDefSndBufrSend(void *pvDriverData, void *pvBufr, int nCount)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;
	pvBufr = pvBufr;
	nCount = nCount;

	return COM_PORT_NOT_OPEN;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSndBufrBusy
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
int WINAPI ComDefSndBufrBusy(void *pvDriverData)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;

	// Act like we're never busy to keep program from hanging when
	//	no com driver is active.
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
int WINAPI ComDefSndBufrClear(void *pvDriverData)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;

	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
int WINAPI ComDefSndBufrQuery(void *pvDriverData,
		unsigned *pafStatus,
		long *plHandshakeDelay)
	{
	// Avoid lint complaints
	pvDriverData = pvDriverData;
	if (pafStatus)
		*pafStatus = 0;

	if (plHandshakeDelay)
		*plHandshakeDelay = 0L;

	return COM_OK;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
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
void WINAPI ComDefIdle(void)
	{
	// Do nothing. This fills in for real function that can be set by caller.
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefDeviceGetCommon
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
int WINAPI ComDefDeviceGetCommon(void *pvPrivate, ST_COMMON *pstCommon)
	{
	// Keep lint from complaining
	pvPrivate = pvPrivate;
	// Indicate that we do not support any common items
	pstCommon->afItem = 0;

	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefDeviceSetCommon
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
int WINAPI ComDefDeviceSetCommon(void *pvPrivate, struct s_common *pstCommon)
	{
	// Keep lint from complaining
	pvPrivate = pvPrivate;
	pstCommon = pstCommon;

	// Don't set anything, pretend that all is well
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDefDeviceSpecial
 *
 * DESCRIPTION:
 *	The means for others to control any special features in this driver
 *	that are not supported by all drivers.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *	COM_NOT_SUPPORTED if the instruction string was not recognized
 *	otherwise depends on instruction string
 */
int WINAPI ComDefDeviceSpecial(void *pvPrivate,
		const TCHAR *pszInstructions,
		TCHAR *pszResult,
		int nBufrSize)
	{
	return COM_NOT_SUPPORTED;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComDefDeviceLoadSaveHdl
 *
 * DESCRIPTION:
 *	Dummy routine for pfDeviceLoadHdl and pfDeviceSaveHdl used when no
 *	com driver is loaded
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *	always returns TRUE
 */
int WINAPI ComDefDeviceLoadSaveHdl(void *pvPrivate, SF_HANDLE sfHdl)
	{
	pvPrivate = pvPrivate;
	sfHdl = sfHdl;

	return TRUE;
	}
