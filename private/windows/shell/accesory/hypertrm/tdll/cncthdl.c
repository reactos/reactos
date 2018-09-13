/*	File: D:\WACKER\tdll\cncthdl.c (Created: 10-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#include <tapi.h>
#pragma hdrstop

#include <time.h>

#include "stdtyp.h"
#include "session.h"
#include "mc.h"
#include "globals.h"
#include "assert.h"
#include "errorbox.h"
#include <term\res.h>
#include "cnct.h"
#include "cnct.hh"
#include <cncttapi\cncttapi.hh>
#include <emu\emu.h>
#include "tchar.h"
#include "tdll.h"

// TODO: This code should be removed once Microsoft includes it in
// some header files.
//
#ifdef UNICODE
	#define GetTimeFormat  GetTimeFormatW
	#define GetDateFormat  GetDateFormatW
#else
	//#define GetTimeFormat  GetTimeFormatA
	//#define GetDateFormat  GetDateFormatA
#endif

static int cnctLoadDriver(const HHCNCT hhCnct);

#define	USE_FORMATMSG

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctCreateHdl
 *
 * DESCRIPTION:
 *	Creates a connection handle which is used to perform a connection
 *	activity.
 *
 * ARGUMENTS:
 *	hSession	- public session handle
 *
 * RETURNS:
 *	Connection handle or zero on error.
 *
 */
HCNCT cnctCreateHdl(const HSESSION hSession)
	{
	HHCNCT hhCnct = 0;

	hhCnct = malloc(sizeof(*hhCnct));

	if (hhCnct == 0)
		{
		assert(FALSE);
		return FALSE;
		}

	memset(hhCnct, 0, sizeof(*hhCnct));
	hhCnct->hSession = hSession;
	InitializeCriticalSection(&hhCnct->csCnct);
	cnctStubAll(hhCnct);

	/* Lower Wacker is hardwired to this */
	cnctSetDevice((HCNCT)hhCnct, 0);

	return (HCNCT)hhCnct;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctDestroyHdl
 *
 * DESCRIPTION:
 *	Destroys a valid connection handle.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle.
 *
 * RETURNS:
 *	void
 *
 */
void cnctDestroyHdl(const HCNCT hCnct)
	{
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	if (hhCnct == 0)
		return;

	DeleteCriticalSection(&hhCnct->csCnct);

	if (hhCnct->hDriver)
		(*hhCnct->pfDestroy)(hhCnct->hDriver);

	if (hhCnct->hModule)
		FreeLibrary(hhCnct->hModule);

	free(hhCnct);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctLock
 *
 * DESCRIPTION:
 *	Locks the connection handle critical section semaphore
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *
 * RETURNS:
 *	void
 *
 */
void cnctLock(const HHCNCT hhCnct)
	{
	EnterCriticalSection(&hhCnct->csCnct);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctUnlock
 *
 * DESCRIPTION:
 *	Unlocks the connection handle critical section semaphore
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *
 * RETURNS:
 *	void
 *
 */
void cnctUnlock(const HHCNCT hhCnct)
	{
	LeaveCriticalSection(&hhCnct->csCnct);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctSetDevice
 *
 * DESCRIPTION:
 *	Sets the connection device
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	0=success, else error code
 *
 */
int cnctSetDevice(const HCNCT hCnct, const LPTSTR pachDevice)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	if (hhCnct == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	/* --- Can't set a device while we're connected --- */

	iRet = cnctQueryStatus(hCnct);

	if (iRet != CNCT_STATUS_FALSE && iRet != CNCT_NOT_SUPPORTED)
		{
		assert(FALSE);
		return CNCT_ERROR;
		}

	/* --- Wacker has only one driver (TAPI) so hard code it here --- */

	cnctLock(hhCnct);
	iRet = cnctLoadDriver(hhCnct);	// driver reports errors
	cnctUnlock(hhCnct);

	return iRet;
	}

#if 0  // mcc 01/06/95 -- hacked in to test Winsock
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctLoadDriver
 *
 * DESCRIPTION:
 *	staticly binds to Winsock connection code
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *
 * RETURNS:
 *	0=success, else error code
 *
 */
static int cnctLoadDriver(const HHCNCT hhCnct)
	{
	if (cnctLoadWinsockDriver(hhCnct) != 0)
		{
		assert(FALSE);
		return CNCT_LOAD_DLL_FAILED;
		}

	return 0;
	}
#endif

#if 1 // mcc 01/05/95 -- this is the "real" Wacker one
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctLoadDriver
 *
 * DESCRIPTION:
 *	staticly binds to TAPI connection code
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *
 * RETURNS:
 *	0=success, else error code
 *
 */
static int cnctLoadDriver(const HHCNCT hhCnct)
	{
	if (hhCnct->hDriver)
		return 0;

	hhCnct->hDriver = cnctdrvCreate((HCNCT)hhCnct, hhCnct->hSession);

	if (hhCnct->hDriver == 0)
		{
		assert(FALSE);
		return CNCT_LOAD_DLL_FAILED;
		}

	cnctStubAll(hhCnct);

	hhCnct->pfDestroy = (int (WINAPI *)(const HDRIVER))cnctdrvDestroy;
	hhCnct->pfQueryStatus = (int (WINAPI *)(const HDRIVER))cnctdrvQueryStatus;

	hhCnct->pfConnect = (int (WINAPI *)(const HDRIVER, const unsigned int))
		cnctdrvConnect;

	hhCnct->pfDisconnect = (int (WINAPI *)(const HDRIVER, const unsigned int))
		cnctdrvDisconnect;

	hhCnct->pfComEvent = (int (WINAPI *)(const HDRIVER, const enum COM_EVENTS))
        cnctdrvComEvent;
	hhCnct->pfInit = (int (WINAPI *)(const HDRIVER))cnctdrvInit;
	hhCnct->pfLoad = (int (WINAPI *)(const HDRIVER))cnctdrvLoad;
	hhCnct->pfSave = (int (WINAPI *)(const HDRIVER))cnctdrvSave;

	hhCnct->pfSetDestination = 
		(int (WINAPI *)(const HDRIVER, TCHAR *const, const size_t))
			cnctdrvSetDestination;

	hhCnct->pfGetComSettingsString = (int (WINAPI *)(const HDRIVER,
		LPTSTR pachStr, const size_t cb))cnctdrvGetComSettingsString;

	return 0;
	}
#endif


#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctLoadDriver
 *
 * DESCRIPTION:
 *	Tries to load the given dll
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *	pachDllName - name of dll to load
 *
 * RETURNS:
 *	0=success, else error code
 *
 */
static int cnctLoadDriver(const HHCNCT hhCnct, const LPTSTR pachDllName)
	{
	#define LOADPROC(x,y) \
		(FARPROC)x = (fp = GetProcAddress(hhCnct->hModule, y)) ? fp : x

	HMODULE hModule;
	HDRIVER hDriver;
	FARPROC fp;
	HDRIVER (WINAPI *pfCreate)(const HCNCT hCnct, const HSESSION hSession);

	/* --- Check to see if we've loaded the driver already --- */

	if (hhCnct->hDriver && StrCharCmp(hhCnct->achDllName, pachDllName) == 0)
		return 0;

	/* --- Try to load the given library name --- */

	hModule = LoadLibrary(pachDllName);

	if (hModule == 0)
		{
		assert(FALSE);
		return CNCT_FIND_DLL_FAILED;
		}

	/* --- Get the create function --- */

	(FARPROC)pfCreate = GetProcAddress(hModule, "cnctwsCreate@8");

	if (pfCreate == 0)
		{
		assert(FALSE);
		FreeLibrary(hModule);
		return CNCT_LOAD_DLL_FAILED;
		}

	/* --- Call the init function --- */

	hDriver = (*pfCreate)((HCNCT)hhCnct, hhCnct->hSession);

	if (hDriver == 0)
		{
		assert(FALSE);
		FreeLibrary(hModule);
		return CNCT_LOAD_DLL_FAILED;
		}

	/* --- If driver initialized, then we can commit to this handle ---*/

	if (hhCnct->hModule)
		FreeLibrary(hhCnct->hModule);

	cnctStubAll(hhCnct);
	hhCnct->hModule = hModule;
	hhCnct->hDriver = hDriver;
	StrCharCopy(hhCnct->achDllName, pachDllName);

	/* --- Drivers only required to support a Create function --- */

	LOADPROC(hhCnct->pfDestroy, "cnctwsDestroy@4");
	LOADPROC(hhCnct->pfQueryStatus, "cnctwsQueryStatus@4");
	LOADPROC(hhCnct->pfConnect, "cnctwsConnect@8");
	LOADPROC(hhCnct->pfDisconnect, "cnctwsDisconnect@8");
	LOADPROC(hhCnct->pfComEvent, "cnctwsComEvent@4");
	return 0;

	#undef LOADPROC
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctQueryStatus
 *
 * DESCRIPTION:
 *	Returns the status of the connection.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	status of connection or error code.
 *
 */
int cnctQueryStatus(const HCNCT hCnct)
	{
	int iStatus;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	if (hhCnct == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	cnctLock(hhCnct);
	iStatus = (*hhCnct->pfQueryStatus)(hhCnct->hDriver);
	cnctUnlock(hhCnct);
	return iStatus;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctConnect
 *
 * DESCRIPTION:
 *	Establishes a connection.
 *
 * ARGUMENTS:
 *	hCnct		- public connect handle
 *	uCnctFlags	- how to connect
 *
 * RETURNS:
 *	0=success, or error-code.
 *
 */
int cnctConnect(const HCNCT hCnct, const unsigned int uCnctFlags)
	{
	int iStatus;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	if (hhCnct == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	cnctLock(hhCnct);
	iStatus = (*hhCnct->pfConnect)(hhCnct->hDriver, uCnctFlags);
	cnctUnlock(hhCnct);

	return iStatus;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctDisconnect
 *
 * DESCRIPTION:
 *	Terminates a connection
 *
 * ARGUMENTS:
 *	hCnct		- public connect handle
 *	uCnctFlags	- how to connect
 *
 * RETURNS:
 *	0=success, or error-code.
 *
 */
int cnctDisconnect(const HCNCT hCnct, const unsigned int uCnctFlags)
	{
	int iStatus;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	if (hhCnct == 0)
		{
		assert(FALSE);
		return CNCT_BAD_HANDLE;
		}

	cnctLock(hhCnct);
	iStatus = (*hhCnct->pfDisconnect)(hhCnct->hDriver, uCnctFlags);
	cnctUnlock(hhCnct);

	return iStatus;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctInit
 *
 * DESCRIPTION:
 *	Initializes the connection driver to a base state
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	0=OK
 *
 */
int cnctInit(const HCNCT hCnct)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfInit)(hhCnct->hDriver);
	cnctUnlock(hhCnct);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctLoad
 *
 * DESCRIPTION:
 *	Reads session file values.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	0=OK
 *
 */
int cnctLoad(const HCNCT hCnct)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfLoad)(hhCnct->hDriver);
	cnctUnlock(hhCnct);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctSave
 *
 * DESCRIPTION:
 *	Saves connection stuff to session file.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	0=OK
 *
 */
int cnctSave(const HCNCT hCnct)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfSave)(hhCnct->hDriver);
	cnctUnlock(hhCnct);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctQueryDriverHdl
 *
 * DESCRIPTION:
 *  Return the driver handle.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *
 * RETURNS:
 *	0 or error
 *
 */
HDRIVER cnctQueryDriverHdl(const HCNCT hCnct)
	{
	const HHCNCT hhCnct = (HHCNCT)hCnct;
	return hhCnct->hDriver;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctSetStartTime
 *
 * DESCRIPTION:
 *	This function should only be called by the connection driver.  It
 *	records the current system time when the connection is estabalished
 *	which only the driver really knows.
 *
 * ARGUMENTS:
 *	HCNCT	hCnct	- exteranl connection handle.
 *
 * RETURNS:
 *	0 if everything is OK.
 *
 */
int cnctSetStartTime(HCNCT hCnct)
	{
	time_t  t;
	HHCNCT  hhCnct = (HHCNCT)hCnct;

	assert(hCnct);

	if (hCnct == (HCNCT)0)
		return -1;

	time(&t);
	hhCnct->tStartTime = t;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctQueryStartTime
 *
 * DESCRIPTION:
 *	Returns the time in C standard time_t format of when the connection
 *	was established.
 *
 * ARGUMENTS:
 *	HCNCT		hCnct	    - external connection handle
 *	time_t FAR *pTime		- pointer to time_t variable for time.
 *
 * RETURNS:
 *
 */
int cnctQueryStartTime(const HCNCT hCnct, time_t *pTime)
	{
	HHCNCT	hhCnct;

	assert(hCnct && pTime);

	if (hCnct == (HCNCT)0)
		return -1;

	hhCnct = (HHCNCT)hCnct;

	*pTime = hhCnct->tStartTime;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctQueryElapsedTime
 *
 * DESCRIPTION:
 *	Returns the number of seconds since the connection was established.
 *	This function set *pTime to zero of a connection is not established.
 *
 * ARGUMENTS:
 *	HCNCT		hCnct   	- external connection handle
 *	time_t FAR *pTime		- pointer to time_t variable for time.
 *
 * RETURNS:
 *	0 if everything is OK.
 *
 */
int cnctQueryElapsedTime(HCNCT hCnct, time_t *pTime)
	{
	int		iRet,  iStatus;
	time_t 	tTime, tStartTime;

	assert(hCnct && pTime);

	if (hCnct == (HCNCT)0)
		return -1;

	if ((iStatus = cnctQueryStatus(hCnct)) != CNCT_STATUS_TRUE)
		{
		*pTime = (time_t)0;
		return 0;
		}

	iRet = cnctQueryStartTime(hCnct, &tStartTime);

	time(&tTime);
	*pTime = tTime - tStartTime;

	if (*pTime < 0 || *pTime >= 86400) // rollover after 24 hours
		{
		cnctSetStartTime(hCnct);
		*pTime = 0;
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctMessage
 *
 * DESCRIPTION:
 *	Calls emuDataIn and gives it the requested string.	Useful for
 *	displaying connected, disconnected message
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *	idMsg	- rc identifier of message
 *
 * RETURNS:
 *	void
 *
 */

#if FALSE

void cnctMessage(const HCNCT hCnct, const int idMsg)
	{
	TCHAR 		 ach[256], achFormat[256];
	TCHAR		 *pach, *pachTime, *pachDate;
	int 		 i, nSize;
	const HHCNCT hhCnct = (HHCNCT)hCnct;
	const HEMU 	 hEmu = sessQueryEmuHdl(hhCnct->hSession);
	LCID		 lcId = GetSystemDefaultLCID();
	SYSTEMTIME	 stSysTimeDate;
	LPTSTR 		 acPtrs[3];
	TCHAR 		 acArg1[100], acArg2[100];

	// Load the "====> Connected %1, %2" or "====> Disconnected %1, %2" msg
	// from the resource...
	//
	TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
	if (LoadString(glblQueryDllHinst(), idMsg, achFormat,
		sizeof(achFormat) / sizeof(TCHAR)) == 0)
		{
		assert(FALSE);
		return;
		}

	// Get formats appropriate for the given locale...
	//
 	nSize = GetTimeFormat(lcId, 0, NULL, NULL, NULL, 0);
 	pachTime = malloc((unsigned int)(nSize+1) * sizeof(TCHAR));
 	memset(pachTime, 0, (unsigned int)(nSize+1) * sizeof(TCHAR));

	GetLocalTime(&stSysTimeDate);
	GetTimeFormat(lcId,	0, &stSysTimeDate, NULL, pachTime, nSize+1);

	// NOTE: The 2nd parameter to GetDateFormat() should be DATE_LONGDATE but
	// right now that causes the function to return garbage, so for now use
	// what works!
	//
	nSize = GetDateFormat(lcId, 0, NULL, NULL, NULL, 0);
 	pachDate = malloc((unsigned int)(nSize+1) * sizeof(TCHAR));
 	memset(pachDate, 0, (unsigned int)(nSize+1) * sizeof(TCHAR));

	GetDateFormat(lcId,	0, &stSysTimeDate, NULL, pachDate, nSize+1);

	// Format the string...
	//
	wsprintf(acArg1, "%s", pachTime);
	wsprintf(acArg2, "%s", pachDate);

	acPtrs[0] = acArg1;
	acPtrs[1] = acArg2;
	acPtrs[2] = NULL;

#if defined(USE_FORMATMSG)

	FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				achFormat, 0, 0, ach, sizeof(ach) / sizeof(TCHAR), acPtrs);

#else
	// Hard code until FormatMessage() works!
	//
	if (idMsg == IDS_CNCT_CLOSE)
		wsprintf(ach, "\r\n=====> Disconnected  %s, %s", acPtrs[0], acPtrs[1]);
	else
		wsprintf(ach, "\r\n=====> Connected %s, %s\r\n", acPtrs[0], acPtrs[1]);
#endif

	free(pachTime);
	pachTime = NULL;
	free(pachDate);
	pachDate = NULL;

	// Display the message on terminal window...
	//
	pach = ach;
	for (i = StrCharGetStrLength(ach); i > 0; --i, pach = StrCharNext(pach))
		emuDataIn(hEmu, *pach);

	return;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctSetDestination
 *
 * DESCRIPTION:
 *	Sets the destination to be dialed
 *
 * ARGUMENTS:
 *	HCNCT		hCnct	    - external connection handle
 *	char		*ach		- destination
 *	size_t		cb			- sizeof of buffer
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int	cnctSetDestination(const HCNCT hCnct, TCHAR *const ach, const size_t cb)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfSetDestination)(hhCnct->hDriver, ach, cb);
	cnctUnlock(hhCnct);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctGetComSettingsString
 *
 * DESCRIPTION:
 *	Returns a string suitable for use in the com settings portion of the
 *	status bar.
 *
 * ARGUMENTS:
 *	hCnct	- public connection handle
 *	pach	- buffer to store string
 *	cb		- max size of buffer
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int cnctGetComSettingsString(const HCNCT hCnct, LPTSTR pach, const size_t cb)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfGetComSettingsString)(hhCnct->hDriver, pach, cb);
	cnctUnlock(hhCnct);

	return iRet;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctComEvent
 *
 * DESCRIPTION:
 *	Calls the driver-specific function to handle notifications from the COM 
 *  driver
 *
 * ARGUMENTS:
 *	HCNCT		hCnct	    - external connection handle
 *
 * RETURNS:
 *	0=OK, else error
 *
 */
int	cnctComEvent(const HCNCT hCnct, const enum COM_EVENTS event)
	{
	int iRet;
	const HHCNCT hhCnct = (HHCNCT)hCnct;

	cnctLock(hhCnct);
	iRet = (*hhCnct->pfComEvent)(hhCnct->hDriver, event);
	cnctUnlock(hhCnct);

	return iRet;
	}
