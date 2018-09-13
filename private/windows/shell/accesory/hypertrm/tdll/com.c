/* com.c -- High level com routines
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#include <windows.h>
#pragma hdrstop

// #define DEBUGSTR
#include <time.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\cnct.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\cloop.h>
#include <tdll\tdll.h>
#include <tdll\sf.h>
#include "tchar.h"
#include "com.h"
#include "comdev.h"
#include "com.hh"
#include <comstd\comstd.hh> // Drivers are linked directly in in this vers.
#if defined(INCL_WINSOCK)
#include <comwsock\comwsock.hh>
#endif  // defined(INCL_WINSOCK)

int WINAPI WsckDeviceInitialize(HCOM hCom,
    unsigned nInterfaceVersion,
    void **ppvDriverData);


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComCreateHandle
 *
 * DESCRIPTION:
 *	Creates a communications handle to be used with subsequent Com calls.
 *	The resulting com handle will not be associated with any actual device
 *	or port initially.
 *
 * ARGUMENTS:
 *	hSession   -- Session handle of session creating com handle
 *	hwndNotify -- Window to receive Com notifications
 *	phcom	   -- pointer to a var. of type HCOM to receive new com handle
 *
 * RETURNS:
 *	COM_OK
 *	COM_NOT_ENOUGH_MEMORY if there is insufficient memory
 *	COM_FAILED			  if resources could not be obtained
 */
int ComCreateHandle(const HSESSION hSession, HCOM *phcom)
	{
	int  iRet = COM_OK;
	HCOM pstCom;

	assert(phcom);
	*phcom = (HCOM)0;

	DBGOUT_NORMAL("+ComCreateHandle for session %08lX\r\n", hSession,0,0,0,0);
	// See if we can get memory for a handle
	if ((pstCom = malloc(sizeof(*pstCom))) == NULL)
		{
		// This error can't be reported by ComReportError because no
		//	Com Handle exists yet.
		//* utilReportError(hSession, RE_ERROR | RE_OK, NM_NEED_MEM,
		//* 	   strldGet(mGetStrldHdl(hSession), NM_CREATE_SESSION));
		DBGOUT_NORMAL("-ComCreateHandle returning COM_NOT_ENOUGH_MEMORY",
				0,0,0,0,0);
		iRet = COM_NOT_ENOUGH_MEMORY;
		goto Checkout;
		}

	// Initialize to all zeros just to be on the safe side
	memset(pstCom, 0, sizeof(*pstCom));

	// ComInitHdl will initialize most values. We must pre-initialize
	// enough so that ComInitHdl knows if it needs to shut anything down.
	pstCom->hSession	  = hSession;
	pstCom->hDriverModule = (HANDLE)0;
	pstCom->fPortActive   = FALSE;
	pstCom->nGuard		  = COM_VERSION;

	pstCom->hRcvEvent = NULL;
	pstCom->hSndReady = NULL;
	pstCom->hRcvEvent = CreateEvent((LPSECURITY_ATTRIBUTES)0,
			TRUE,					// must be manually reset
			FALSE,					// create unsignalled
			(LPCTSTR)0);			// unnamed
	if (!pstCom->hRcvEvent)
		{
		iRet = COM_FAILED;
		goto Checkout;
		}

    pstCom->hSndReady = CreateEvent((LPSECURITY_ATTRIBUTES)0,
            TRUE,                   // must be manually reset
            FALSE,                  // create unsignalled
            (LPCTSTR)0);            // unnamed
    if (!pstCom->hSndReady)
        {
        iRet = COM_FAILED;
        goto Checkout;
        }

	if ((iRet = ComInitHdl(pstCom)) != COM_OK)
		goto Checkout;


	Checkout:

	if (iRet == COM_OK)
		*phcom = (HCOM)pstCom;
	else
		{
		*phcom = (HCOM)0;
		if (pstCom)
			{
			if (pstCom->hRcvEvent)
				CloseHandle(pstCom->hRcvEvent);
            if (pstCom->hSndReady)
                CloseHandle(pstCom->hSndReady);
			free(pstCom);
			pstCom = NULL;
			}
		}

	DBGOUT_NORMAL("ComCreateHandle returning %d, pstCom == %08lX\r\n",
			iRet, pstCom, 0,0,0);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDestroyHandle
 *
 * DESCRIPTION:
 *	Shuts down an existing com handle and frees all resources assigned to it.
 *
 * ARGUMENTS:
 *	hCom -- A com handle returned from an earlier call to ComCreateHandle
 *			(or ComCreateWudgeHandle)
 *
 * RETURNS:
 *	COM_OK
 */
int ComDestroyHandle(HCOM *phCom)
	{
	int   iRetVal = COM_OK;
	HCOM  pstCom;

	DBGOUT_NORMAL("+ComDestroyHandle(%#08lx)\r\n", *phCom,0,0,0,0);
	assert(phCom);

	// OK to pass null handle to this function
	if (*phCom == (HCOM)0)
		{
		DBGOUT_NORMAL("-ComDestroyHandle returning COM_OK\r\n", 0,0,0,0,0);
		return COM_OK;
		}

	pstCom = *phCom;
	assert(ComValidHandle(pstCom));

	// Disconnect from driver
	ComFreeDevice(pstCom);

	free(pstCom);
	*phCom = (HCOM)0;
	DBGOUT_NORMAL("-ComDestroyHandle returned %d\r\n",
			usRetVal,0,0,0,0);
	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComInitHdl
 *
 * DESCRIPTION:
 *	Called to initialize the Com handle to its default state. Calling this
 *	function will clear any existing settings or states and reset for a
 *	new session.
 *
 * ARGUMENTS:
 *	pstCom -- Pointer to our handle data.
 *
 * RETURNS:
 *	COM_OK if all is well.
 */
int ComInitHdl(const HCOM pstCom)
	{
	int iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	// Make sure we're disconnected from any driver loaded earlier
	ComFreeDevice(pstCom);

	// Fill in default values in exported com structure
	pstCom->stComCntrl.puchRBData		= &pstCom->chDummy;
	pstCom->stComCntrl.puchRBDataLimit	= &pstCom->chDummy;

	// Fill in default values for user-settable fields
	pstCom->stWorkSettings.szDeviceFile[0] = (TCHAR)0;
	pstCom->stWorkSettings.szPortName[0] = (TCHAR)0;
	pstCom->stFileSettings = pstCom->stWorkSettings;

	// Fill in default values in private com structure
	pstCom->fPortActive    = FALSE;
	pstCom->fErrorReported = FALSE;
	pstCom->hDriverModule  = (HANDLE)0;
	pstCom->szDeviceName[0]= (TCHAR)0;
	pstCom->chDummy 	   = (TCHAR)0;
	pstCom->afOverride	   = 0;

	pstCom->puchSendBufr1  = pstCom->auchDummyBufr;
	pstCom->puchSendBufr2  = pstCom->auchDummyBufr;;
	pstCom->puchSendBufr   = pstCom->auchDummyBufr;
	pstCom->puchSendPut    = pstCom->auchDummyBufr;
	pstCom->nSBufrSize	   = sizeof(pstCom->auchDummyBufr);
	pstCom->nSendCount	   = 0;
	pstCom->fUserCalled    = FALSE;
	pstCom->pfUserFunction = ComSendDefaultStatusFunction;

	// fill in defaults for driver functions

	pstCom->pfDeviceClose		  = ComDefDoNothing;
	pstCom->pfDeviceDialog		  = ComDefDeviceDialog;
	pstCom->pfDeviceGetCommon	  = ComDefDeviceGetCommon;
	pstCom->pfDeviceSetCommon	  = ComDefDeviceSetCommon;
	pstCom->pfDeviceSpecial 	  = ComDefDeviceSpecial;
	pstCom->pfDeviceLoadHdl 	  = ComDefDeviceLoadSaveHdl;
	pstCom->pfDeviceSaveHdl 	  = ComDefDeviceLoadSaveHdl;
	pstCom->pfPortConfigure 	  = ComDefDoNothing;
	pstCom->pfPortPreconnect	  = ComDefPortPreconnect;
	pstCom->pfPortActivate		  = ComDefPortActivate;
	pstCom->pfPortDeactivate	  = ComDefDoNothing;

	pstCom->pfPortConnected 	  = ComDefDoNothing;
	pstCom->pfRcvRefill 		  = ComDefBufrRefill;
	pstCom->pfRcvClear			  = ComDefDoNothing;
	pstCom->pfSndBufrSend		  = ComDefSndBufrSend;
	pstCom->pfSndBufrIsBusy 	  = ComDefSndBufrBusy;
	pstCom->pfSndBufrClear		  = ComDefSndBufrClear;
	pstCom->pfSndBufrQuery		  = ComDefSndBufrQuery;
	pstCom->pfSendXon			  = ComDefDoNothing;

	pstCom->pvDriverData		  = NULL;

	ResetEvent(pstCom->hRcvEvent);
    ResetEvent(pstCom->hSndReady);
	// Normally, we would load the port type and port name values from the session file and set them,
	// but since we inherit such things from TAPI, just call ComSetDeviceFromFile with a dummy
	// name to get the proper initialization of the com driver.
	ComSetDeviceFromFile((HCOM)pstCom, "comstd.dll");
	ComSetPortName((HCOM)pstCom, "COM1");

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComLoadHdl
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ComLoadHdl(const HCOM pstCom)
	{
	const SF_HANDLE sfHdl = sessQuerySysFileHdl(pstCom->hSession);
    int (WINAPI *pfDeviceLoadHdl)(void *pvDevData, SF_HANDLE sfHdl);
    int     iRetVal;

    pfDeviceLoadHdl = DeviceLoadHdl;
    iRetVal = (*pfDeviceLoadHdl)(pstCom->pvDriverData, sfHdl);

#if defined(INCL_WINSOCK)
    if (iRetVal == SF_OK)
        {
        pfDeviceLoadHdl = WsckDeviceLoadHdl;
        iRetVal = (*pfDeviceLoadHdl)(pstCom->pvDriverData, sfHdl);
        }
#endif  // defined(INCL_WINSOCK)

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComSaveHdl
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ComSaveHdl(const HCOM pstCom)
	{
	const SF_HANDLE sfHdl = sessQuerySysFileHdl(pstCom->hSession);
	int (WINAPI *pfDeviceSaveHdl)(void *pvDevData, SF_HANDLE sfHdl);
    int     iRetVal;

    pfDeviceSaveHdl = DeviceSaveHdl;
    iRetVal = (*pfDeviceSaveHdl)(pstCom->pvDriverData, sfHdl);

#if defined(INCL_WINSOCK)
    if (iRetVal == SF_OK)
        {
        pfDeviceSaveHdl = WsckDeviceSaveHdl;
        iRetVal = (*pfDeviceSaveHdl)(pstCom->pvDriverData, sfHdl);
        }
#endif  // defined(INCL_WINSOCK)

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ComSetDeviceFromFile(const HCOM pstCom, const TCHAR * const pszFileName)
	{
	int    iRetVal = COM_OK;
	int    (WINAPI *pfDeviceInit)(HCOM, unsigned, void **);

	if (pstCom->pvDriverData)
		return COM_OK;
	
	// If loadable com drivers were actually implemented, we wouldl load the proper .DLL module here
	// and initialize it. In this version, though, we have only one com driver and it is linked right
	// in. So rather than doing GetProcAddress calls to link to the driver, we can simply load function
	// addresses right into function pointers.
    //
    // Not true anymore! We now have two com drivers to support. But since
    // we still don't load from DLLs, we just let the two drivers share the
    // driver data structure, and each initializes its own specific members.
    // - jmh 02-22-96
	pstCom->hDriverModule = (HANDLE)1;		// Set this to fake value so we can close
	pfDeviceInit = DeviceInitialize;

	if ((iRetVal = (*pfDeviceInit)(pstCom, COM_VERSION,
			&pstCom->pvDriverData)) != COM_OK)
		{
		// The device driver cannot report errors itself until it has
		// been initialized. So we must report any errors it encountered.
		//* if (iRetVal == COM_DEVICE_VERSION_ERROR)
		//* ComReportError(pstCom, CM_ERR_WRONG_VERSION, pszFileName, TRUE);
		//* else
		//* ComReportError(pstCom, CM_ERR_CANT_INIT, pszFileName, TRUE);

		DBGOUT_NORMAL(" ComSetDevice: *pfDeviceInit failed\r\n",0,0,0,0,0);
		goto Checkout;
		}

#if defined(INCL_WINSOCK)
    // Initialize the driver data structure members specific to WinSock.
    //
	pfDeviceInit = WsckDeviceInitialize;

	if ((iRetVal = (*pfDeviceInit)(pstCom, COM_VERSION,
			&pstCom->pvDriverData)) != COM_OK)
		{
		goto Checkout;
		}
#endif  // defined(INCL_WINSOCK)

	pstCom->pfDeviceClose = DeviceClose;
	pstCom->pfDeviceDialog = DeviceDialog;
	pstCom->pfDeviceGetCommon = DeviceGetCommon;
	pstCom->pfDeviceSetCommon = DeviceSetCommon;
	pstCom->pfDeviceSpecial = DeviceSpecial;
	pstCom->pfPortConfigure = PortConfigure;
	//pstCom->pfPortPreconnect = PortPreconnect;
	pstCom->pfPortPreconnect = ComDefPortPreconnect;
	pstCom->pfPortActivate = PortActivate;

	Checkout:
	// if something went wrong, set comm to invalid driver state and return err
	if (iRetVal != COM_OK)
		ComFreeDevice(pstCom);

	DBGOUT_NORMAL("-ComSetDevice returning %d\r\n", iRetVal,0,0,0,0);
	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetDeviceName
 *
 * DESCRIPTION:
 *	Returns name of device associated with a com handle
 *
 * ARGUMENTS:
 *	pstCom	-- com handle returned from earlier call to ComCreateHandle
 *	pszName -- pointer to buffer to receive device name (may be NULL)
 *	pusLen	-- pointer length variable. If pszName is not NULL, this variable
 *				should contain the size of the buffer pointed to by pszName.
 *				In either case, *pusLen will be set to the size of the
 *				device name to be returned.
 *
 * RETURNS:
 *	COM_OK
 *	COM_INVALID_HANDLE
 */
int ComGetDeviceName(const HCOM pstCom,
			TCHAR * const pszName,
			int * const pnLen)
	{
	int iRetVal = COM_OK;
	int nTheirLen;

	DBGOUT_NORMAL("+ComGetDevice(%#08lx)\r\n", pstCom,0,0,0,0);
	assert(ComValidHandle(pstCom));
	assert(pnLen);

	nTheirLen = *pnLen;
	*pnLen = StrCharGetByteCount(pstCom->szDeviceName);

	if (pszName)
		{
		assert(nTheirLen >= (*pnLen + 1));
		if (nTheirLen >= (*pnLen + 1))
			StrCharCopy(pszName, pstCom->szDeviceName);
		DBGOUT_NORMAL(" ComGetDevice: providing name (%s)\r\n", pszName,0,0,0,0);
		}
	DBGOUT_NORMAL("-ComGetDevice returning %d\r\n", iRetVal,0,0,0,0);
	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComGetRcvEvent
 *
 * DESCRIPTION:
 *	Returns a handle to an event object that can be used to wait for
 *	received data to be available from the com routines.
 *
 * ARGUMENTS:
 *	pstCom	  -- com handle returned from earlier call to ComCreateHandle
 *
 * RETURNS:
 *	The Receive event object
 */
HANDLE ComGetRcvEvent(HCOM pstCom)
	{
	return pstCom->hRcvEvent;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetSession
 *
 * DESCRIPTION:
 *	Returns Session Handle associated with a Com handle
 *
 * ARGUMENTS:
 *	pstCom	  -- com handle returned from earlier call to ComCreateHandle
 *	phSession -- pointer to session handle to receive result
 *
 * RETURNS:
 *	always returns COM_OK
 */
int ComGetSession(const HCOM pstCom, HSESSION * const phSession)
	{
	assert(ComValidHandle(pstCom));
	assert(phSession);

	*phSession = pstCom->hSession;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComNotify
 *
 * DESCRIPTION:
 *	Called by driver modules to notify com routines of significant events
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void ComNotify(const HCOM pstCom, enum COM_EVENTS event)
	{

	assert(ComValidHandle(pstCom));

	switch (event)
		{
	case CONNECT:
		cnctComEvent(sessQueryCnctHdl(pstCom->hSession), CONNECT);
		break;

	case DATA_RECEIVED:
		SetEvent(pstCom->hRcvEvent);
		CLoopRcvControl(sessQueryCLoopHdl(pstCom->hSession), CLOOP_RESUME,
				CLOOP_RB_NODATA);
		break;

	case NODATA:
		ResetEvent(pstCom->hRcvEvent);
		break;

	case SEND_STARTED:
		// NotifyClient(pstCom->hSession, EVENT_LED_SD_ON, 0);
		//DbgOutStr("Send started\n",0,0,0,0,0);
        ResetEvent(pstCom->hSndReady);
		break;

	case SEND_DONE:
		// NotifyClient(pstCom->hSession, EVENT_LED_SD_OFF, 0);
		//DbgOutStr("Send done\n",0,0,0,0,0);
        SetEvent(pstCom->hSndReady);
		break;

	default:
		assert(FALSE);
		break;
		}
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComIsActive
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
int ComIsActive(const HCOM pstCom)
	{
	assert(ComValidHandle(pstCom));

	if (pstCom == (HCOM)0 || !pstCom->fPortActive)
		return COM_PORT_NOT_OPEN;

	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetPortName
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
int ComSetPortName(const HCOM pstCom, const TCHAR * const pszPortName)
	{
	int iRetVal = COM_OK;

	DBGOUT_NORMAL("+ComSetPortName(%#08lx, %s)\r\n", pstCom, pszPortName,0,0,0);
	assert(ComValidHandle(pstCom));

	if (!pszPortName)
		iRetVal = COM_PORT_INVALID_NAME;

	else if (ComIsActive(pstCom) == COM_OK)
		iRetVal = COM_PORT_IN_USE;

	if (StrCharCmp(pszPortName, pstCom->stWorkSettings.szPortName) != 0)
		{
		//* TODO: call driver to check validity of name
		StrCharCopy(pstCom->stWorkSettings.szPortName, pszPortName);
		}

	DBGOUT_NORMAL("-ComSetPortName returned %u\r\n", iRetVal, 0,0,0,0);

	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetPortName
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
int ComGetPortName(const HCOM pstCom, TCHAR * const pszName, int * const pnLen)
	{
	int iRetVal = COM_OK;
	int nTheirLen = *pnLen;

	assert(ComValidHandle(pstCom));
	assert(pnLen);

	DBGOUT_NORMAL("+ComGetPortName(%#08lx)\r\n", pstCom, 0,0,0,0);

	*pnLen = StrCharGetByteCount(pstCom->stWorkSettings.szPortName);

	if (pszName)
		{
		assert(nTheirLen >= (*pnLen + 1));
		if (nTheirLen >= (*pnLen + 1))
			StrCharCopy(pszName, pstCom->stWorkSettings.szPortName);
		else
			pszName[0] = (TCHAR)0;
		}
	DBGOUT_NORMAL("-ComGetPortName returning %u, size = %u, name = %s\r\n",
			iRetVal, *pnLen, pszName ? pszName : " ",0,0);
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetAutoDetect
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
int ComGetAutoDetect(HCOM pstCom, int *pfAutoDetect)
	{
	int iRet = COM_OK;
	struct s_common stCommon;

	assert(ComValidHandle(pstCom));
	assert(pfAutoDetect);

	if (pstCom->pfDeviceGetCommon == NULL)
		iRet = COM_NOT_SUPPORTED;
	else if ((*pstCom->pfDeviceGetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		iRet = COM_DEVICE_ERROR;
	else if (!bittest(stCommon.afItem, COM_AUTO))
		iRet = COM_NOT_SUPPORTED;
	else if (pfAutoDetect)
		*pfAutoDetect = stCommon.fAutoDetect;

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetAutoDetect
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int ComSetAutoDetect(HCOM pstCom, int fAutoDetect)
	{
	struct s_common stCommon;
	int 	  		fDummy;
	int 			iRet = COM_OK;

	assert(ComValidHandle(pstCom));

	if (ComGetAutoDetect(pstCom, &fDummy) == COM_NOT_SUPPORTED)
		iRet = COM_NOT_SUPPORTED;
	else
		{
		stCommon.afItem = COM_AUTO;
		stCommon.fAutoDetect = fAutoDetect;

		if ((*pstCom->pfDeviceSetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
			iRet = COM_DEVICE_ERROR;
		}

	return iRet;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetBaud
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
int ComGetBaud(const HCOM pstCom, long * const plBaud)
	{
	ST_COMMON stCommon;

	assert(ComValidHandle(pstCom));
	assert(plBaud);

	if (pstCom->pfDeviceGetCommon == NULL)
		return COM_NOT_SUPPORTED;

	if ((*pstCom->pfDeviceGetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		return COM_DEVICE_ERROR;

	if (!bittest(stCommon.afItem, COM_BAUD))
		return COM_NOT_SUPPORTED;

	*plBaud = stCommon.lBaud;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetBaud
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
int ComSetBaud(const HCOM pstCom, const long lBaud)
	{
	ST_COMMON stCommon;
	long	  lDummy;
	int 	  iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	if (ComGetBaud(pstCom, &lDummy) == COM_NOT_SUPPORTED)
		iRetVal =  COM_NOT_SUPPORTED;
	else
		{
		stCommon.afItem = COM_BAUD;
		stCommon.lBaud = lBaud;

		if ((*pstCom->pfDeviceSetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
			iRetVal = COM_DEVICE_ERROR;
		}
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetDataBits
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
int ComGetDataBits(const HCOM pstCom, int * const pnDataBits)
	{
	ST_COMMON stCommon;

	assert(ComValidHandle(pstCom));
	assert(pnDataBits);

	if (pstCom->pfDeviceGetCommon == NULL)
		return COM_NOT_SUPPORTED;

	if ((*pstCom->pfDeviceGetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		return COM_DEVICE_ERROR;

	if (!bittest(stCommon.afItem, COM_DATABITS))
		return COM_NOT_SUPPORTED;

	*pnDataBits = stCommon.nDataBits;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetDataBits
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
int ComSetDataBits(const HCOM pstCom, const int nDataBits)
	{
	ST_COMMON stCommon;
	int 	  nDummy;
	int 	  iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	if (ComGetDataBits(pstCom, &nDummy) == COM_NOT_SUPPORTED)
		return COM_NOT_SUPPORTED;

	stCommon.afItem = COM_DATABITS;
	stCommon.nDataBits = nDataBits;

	if ((*pstCom->pfDeviceSetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		iRetVal = COM_DEVICE_ERROR;

	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetStopBits
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
int ComGetStopBits(const HCOM pstCom, int * const pnStopBits)
	{
	ST_COMMON stCommon;

	assert(ComValidHandle(pstCom));
	assert(pnStopBits);

	if (pstCom->pfDeviceGetCommon == NULL)
		return COM_NOT_SUPPORTED;

	if ((*pstCom->pfDeviceGetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		return COM_DEVICE_ERROR;

	if (!bittest(stCommon.afItem, COM_STOPBITS))
		return COM_NOT_SUPPORTED;

	*pnStopBits = stCommon.nStopBits;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetStopBits
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
int ComSetStopBits(const HCOM pstCom, const int nStopBits)
	{
	ST_COMMON stCommon;
	int 	  nDummy;
	int 	  iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	if (ComGetStopBits(pstCom, &nDummy) == COM_NOT_SUPPORTED)
		return COM_NOT_SUPPORTED;

	stCommon.afItem = COM_STOPBITS;
	stCommon.nStopBits = nStopBits;

	if ((*pstCom->pfDeviceSetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		iRetVal = COM_DEVICE_ERROR;

	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetParity
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
int ComGetParity(const HCOM pstCom, int * const pnParity)
	{
	ST_COMMON stCommon;

	assert(ComValidHandle(pstCom));
	assert(pnParity);

	if (pstCom->pfDeviceGetCommon == NULL)
		return COM_NOT_SUPPORTED;

	if ((*pstCom->pfDeviceGetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		return COM_DEVICE_ERROR;

	if (!bittest(stCommon.afItem, COM_PARITY))
		return COM_NOT_SUPPORTED;

	*pnParity = stCommon.nParity;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSetParity
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
int ComSetParity(const HCOM pstCom, const int nParity)
	{
	ST_COMMON stCommon;
	int 	  nDummy;
	int 	  iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	if (ComGetParity(pstCom, &nDummy) == COM_NOT_SUPPORTED)
		return COM_NOT_SUPPORTED;

	stCommon.afItem = COM_PARITY;
	stCommon.nParity = nParity;

	if ((*pstCom->pfDeviceSetCommon)(pstCom->pvDriverData, &stCommon) != COM_OK)
		iRetVal = COM_DEVICE_ERROR;

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComPreconnect
 *
 * DESCRIPTION:
 *	This function is called just before a connection is attempted. It is
 *	called at a point in the connection process when user interaction is
 *	straight-forward. Certain devices may need to interact with the user
 *	in order to work (having user insert a card, or select from a pool of
 *	devices, etc.). User interaction may not be possible at the time that
 *	ComActivatePort is called, so it should be done here. This routine
 *	may lay claim to a resource and hold it pending the call to
 *	ComActivatePort. Once this routine is called, ComActivatePort will
 *	usually be called (but not necessarily always); ComDeactivatePort will
 *	always be called.
 *
 * ARGUMENTS:
 *	pstCom		-- a com handle as returned by ComCreateHandle
 *
 * RETURNS:
 *	COM_OK		-- if the connection attempt should continue
 *	COM_FAILED	-- if the connection attempt should be abandoned. (in this
 *				   case, it is up to the driver to display the reason
 *				   before returning)
 */
int ComPreconnect(const HCOM pstCom)
	{
	int iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	iRetVal = (*pstCom->pfPortPreconnect)(pstCom->pvDriverData,
			pstCom->stWorkSettings.szPortName, sessQueryHwnd(pstCom->hSession));

	if (iRetVal != COM_OK)
		iRetVal = COM_FAILED;

	return iRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComActivatePort
 *
 * DESCRIPTION:
 *	Attempts to activate the port associated with a com handle. This call
 *	will not necessarily attempt to complete a connection.
 *	Note: this function will display an error messages for all errors except
 *		  COM_PORT_IN_USE. If a COM_PORT_IN_USE error is encountered and
 *		  is not rectified by borrowing or changing ports, the error message
 *		  should be displayed by the calling routine.
 *
 * ARGUMENTS:
 *	pstCom		-- a com handle as returned by ComCreateHandle
 *
 * RETURNS:
 *	COM_OK
 *	COM_PORT_IN_USE -- Port is in use by another process.
 *	or error code as defined in COM.H
 */
int ComActivatePort(const HCOM pstCom, DWORD_PTR dwMediaHdl)
	{
	int iRetVal = COM_OK;

	// This function (or the functions it calls) should report all errors
	//	 except for COM_PORT_IN_USE. Higher level routines may want to
	//	 try some recovery techniques before reporting an unavailable port
	assert(ComValidHandle(pstCom));

	DBGOUT_NORMAL("+ComActivatePort(%#08x)\r\n", pstCom, 0,0,0,0);
	if (ComIsActive(pstCom) != COM_OK)
		{
		//* TODO: this is temporary until we resolve how driver and program
		//		 decide on size of send buffers.
		pstCom->nSBufrSize = 128;

		// Allocate ComSend buffers
		if ((pstCom->puchSendBufr1 =
				malloc((size_t)pstCom->nSBufrSize)) == NULL ||
				(pstCom->puchSendBufr2 =
				malloc((size_t)pstCom->nSBufrSize)) == NULL)
			{
			DBGOUT_NORMAL(" ComActivatePort -- no memory for send buffers\r\n",
					0,0,0,0,0);
			//* ComReportError(pstCom, NM_NEED_MEMFOR,
			//* 		strldGet(mGetStrldHdl(pstCom->hSession), CM_NM_COMDRIVER), TRUE);
			iRetVal = COM_NOT_ENOUGH_MEMORY;
			goto checkout;
			}

		pstCom->puchSendBufr = pstCom->puchSendPut = pstCom->puchSendBufr1;
		pstCom->nSendCount = 0;
		pstCom->fUserCalled = FALSE;
		pstCom->pfUserFunction = ComSendDefaultStatusFunction;

 		// Now call on driver code to activate the physical device
		if ((iRetVal = (*pstCom->pfPortActivate)(pstCom->pvDriverData,
				pstCom->stWorkSettings.szPortName, dwMediaHdl)) == COM_OK)
			pstCom->fPortActive = TRUE;
		}

	checkout:
	if (iRetVal != COM_OK)
		{
		ComDeactivatePort(pstCom);
		}
	DBGOUT_NORMAL("-ComActivatePort returning %u\r\n", iRetVal, 0,0,0,0);
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDeactivatePort
 *
 * DESCRIPTION:
 *	Attempts to deactivate the port associated with a com handle. This call
 *
 * ARGUMENTS:
 *	pstCom		-- a com handle as returned by ComCreateHandle
 *
 * RETURNS:
 *	COM_OK
 *	or error code as defined in COM.H
 */
int ComDeactivatePort(const HCOM pstCom)
	{
	int iRetVal = COM_OK;

	DBGOUT_NORMAL("+ComDeactivatePort(%#08x)\r\n", pstCom,0,0,0,0);

	if (ComValidHandle(pstCom) == 0)
		{
		assert(0);
		return COM_INVALID_HANDLE;
		}

	if (pstCom->fPortActive)
		{
		// Call on driver code to deactivate the physical device
		if ((iRetVal =
				(*pstCom->pfPortDeactivate)(pstCom->pvDriverData)) == COM_OK)
			pstCom->fPortActive = FALSE;
		}

	pstCom->pfPortDeactivate	= ComDefDoNothing;
	pstCom->pfPortConnected 	= ComDefDoNothing;
	pstCom->pfRcvRefill 		= ComDefBufrRefill;
	pstCom->pfRcvClear			= ComDefDoNothing;
	pstCom->pfSndBufrSend		= ComDefSndBufrSend;
	pstCom->pfSndBufrIsBusy 	= ComDefSndBufrBusy;
	pstCom->pfSndBufrClear		= ComDefSndBufrClear;
	pstCom->pfSndBufrQuery		= ComDefSndBufrQuery;
	pstCom->pfSendXon			= ComDefDoNothing;

	DBGOUT_NORMAL("-ComDeactivatePort returned %u\r\n", iRetVal, 0,0,0,0);
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComOverride
 *
 * DESCRIPTION:
 *	Used to temporarily override the current com settings. Allows setting
 *	the com channel to support specific data transfer needs without
 *	specific knowledge of the current com device or its settings.
 *
 * ARGUMENTS:
 *	pstCom			Com handle returned by an call to CreateComHandle
 *	uiOptions		Options which specify transfer requirements. Currently:
 *						COM_OVERRIDE_8BIT	temporarily switchs port to
 *											8 bit, no parity mode
 *						COM_OVERRIDE_RCVALL temporarily suspends any com
 *											settings that would prevent some
 *											characters from being received:
 *											typically suspends recognition
 *											of received XON/XOFF codes
 *						COM_OVERRIDE_SNDALL temporarily suspends any com
 *											settings that would prevent some
 *											characters from being sent.
 *	puiOldOptions	Pointer to a unsigned variable to receive the options in
 *					force prior to this call. The value returned in this
 *					field should be used to restore the com driver when
 *					the override is no longer needed. If this value is not
 *					needed, puiOldOptions can be set to NULL.
 *
 * RETURNS:
 *	COM_OK if requested override is possible with the current com device
 *	COM_CANT_OVERRIDE if the current device cannot support the request
 *
 */
int ComOverride(const HCOM pstCom,
			const unsigned afOptions,
				  unsigned * const pafOldOptions)
	{
	unsigned afOldOverride;
	int 	 iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));

	afOldOverride = pstCom->afOverride;
	if (pafOldOptions)
		*pafOldOptions = afOldOverride;
	pstCom->afOverride = afOptions;

	if ((iRetVal = ComConfigurePort(pstCom)) == COM_CANT_OVERRIDE)
		pstCom->afOverride = afOldOverride;

	return iRetVal;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComQueryOverride
 *
 * DESCRIPTION:
 *	Returns the value of the override flags as described in ComOverride
 *
 * ARGUMENTS:
 *	pstCom			Com handle returned by an call to CreateComHandle
 *	pafOptions		Pointer to UINT to receive copy of override option flags
 *
 * RETURNS:
 *	Always returns COM_OK
 */
int ComQueryOverride(HCOM pstCom, unsigned *pafOptions)
	{
	assert(ComValidHandle(pstCom));
	assert(pafOptions);

	*pafOptions = pstCom->afOverride;
	return COM_OK;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComConfigurePort
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
int ComConfigurePort(const HCOM pstCom)
	{
	int iRetVal = COM_OK;

	DBGOUT_NORMAL("+ComconfigurePort(%#08x)\r\n", pstCom, 0,0,0,0);
	assert(ComValidHandle(pstCom));

	if (ComIsActive(pstCom) == COM_OK)
		iRetVal = (*pstCom->pfPortConfigure)(pstCom->pvDriverData);

	DBGOUT_NORMAL("-ComConfigurePort returning %u\r\n", iRetVal, 0,0,0,0);
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComRcvBufrRefill
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
int ComRcvBufrRefill(const HCOM pstCom, TCHAR * const tc, const int fRemoveChar)
	{
	int iRetVal;
	ST_COM_CONTROL *pstComCntrl = (ST_COM_CONTROL *)pstCom;

	iRetVal = (*pstCom->pfRcvRefill)(pstCom->pvDriverData);
	if (iRetVal)
		{
		if (tc)
			*tc = *pstComCntrl->puchRBData;
		if (fRemoveChar)
			++pstComCntrl->puchRBData;
		}

	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComRcvBufrClear
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
int ComRcvBufrClear(const HCOM pstCom)
	{
	return ((*pstCom->pfRcvClear)(pstCom->pvDriverData));
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSndBufrSend
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
int ComSndBufrSend(
		const HCOM pstCom,
		void * const pvBufr,
		const int nCount,
		const int nWait)
	{
	int iRetVal = COM_OK;

	assert(ComValidHandle(pstCom));
	assert(pvBufr);
	if (nCount > 0)
		{
		if (ComSndBufrBusy(pstCom) == COM_BUSY &&
				(!nWait || ComSndBufrWait(pstCom, nWait) != COM_OK))
			iRetVal = COM_BUSY;
		else
			{
			iRetVal = (*pstCom->pfSndBufrSend)(pstCom->pvDriverData,
					pvBufr, nCount);
			}
		}

	return iRetVal;
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
int ComSndBufrBusy(const HCOM pstCom)
	{
	int usResult;

	usResult =	(*pstCom->pfSndBufrIsBusy)(pstCom->pvDriverData);

	return usResult;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSndBufrWait
 *
 * DESCRIPTION:
 *	Waits until the Com driver can transmit more data. The amount of time to
 *	wait can be specified. While waiting, a settable idle function is
 *	repeatedly called.
 *
 * ARGUMENTS:
 *	pstCom -- Com handle
 *	nWait -- Time to wait in tenths of a second
 *
 * RETURNS:
 *	COM_OK if driver can accept new data within the timeout interval
 *	COM_BUSY if the transmitter is still not available after timeout interval
 */
int ComSndBufrWait(const HCOM pstCom, const int nWait)
	{
	int     iRetVal = COM_OK;
	DWORD   dwRet;

	if ((iRetVal = ComSndBufrBusy(pstCom)) != COM_OK && nWait)
		{
        //DbgOutStr("DBG_WRITE: %d Wait started\n",GetTickCount(),0,0,0,0);
        dwRet = WaitForSingleObject(pstCom->hSndReady, nWait * 100);
        if (dwRet != WAIT_OBJECT_0)
            {
            iRetVal = COM_BUSY;
            }
        else
            {
            iRetVal = COM_OK;
            }

        }
    else
        {
        //DbgOutStr("DBG_WRITE: %d No wait\n",GetTickCount(),0,0,0,0);
        }

	return iRetVal;

#if 0   // jmh 01-11-96 This was the previous method, which didn't block at all
	int  iRetVal = COM_OK;
	DWORD dwTimer;

	if ((iRetVal = ComSndBufrBusy(pstCom)) != COM_OK && nWait)
		{
		dwTimer = startinterval();
		while (interval(dwTimer) < (DWORD)nWait)
			{
			//* With thread model, not sure we still need ComIdle
			//* ComIdle(pstCom);	// Keep from locking up the program
			if (ComSndBufrBusy(pstCom) == COM_OK)
				{
				iRetVal = COM_OK;
				break;
				}
			}
		}

	return iRetVal;
#endif  // 0
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSndBufrClear
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
int ComSndBufrClear(const HCOM pstCom)
	{
	return (*pstCom->pfSndBufrClear)(pstCom->pvDriverData);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComSndBufrQuery
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
int ComSndBufrQuery(const HCOM pstCom, unsigned * const pafStatus,
		long * const plHandshakeDelay)
	{
	return (*pstCom->pfSndBufrQuery)(pstCom->pvDriverData, pafStatus,
				plHandshakeDelay);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDeviceDialog
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
int ComDeviceDialog(const HCOM pstCom, const HWND hwndParent)
	{
	int iRetVal;

	assert(ComValidHandle(pstCom));
	iRetVal = (*pstCom->pfDeviceDialog)(pstCom->pvDriverData, hwndParent);

	return iRetVal;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComDriverSpecial
 *
 * DESCRIPTION:
 *	Allows access to special features of specific Com Device Drivers using
 *	a common API.
 *
 * ARGUMENTS:
 *	pstCom			-- A Com Handle
 *	pszInstructions -- A driver specific string providing instructions
 *					   on what task a driver should carry out.
 *	pszResults		-- A buffer to receive a driver specific result string.
 *	uiBufrSize		-- The size (in bytes) of the pszResults buffer.
 *
 * RETURNS:
 *
 */
int ComDriverSpecial(const HCOM pstCom, const TCHAR * const pszInstructions,
						 TCHAR * const pszResults, const int nBufrSize)
	{
	int iRetVal = COM_NOT_SUPPORTED;

	if (pstCom == NULL)
		return iRetVal;

	if (pstCom->pfDeviceSpecial)
		iRetVal = (*pstCom->pfDeviceSpecial)(pstCom->pvDriverData,
				pszInstructions, pszResults, nBufrSize);

	return iRetVal;
	}





/* --- Internal functions --- */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComReportError
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
void ComReportError(const HCOM pstCom, int iErrStr,
		const TCHAR * const pszOptInfo, const int fFirstOnly)
	{
	if (!fFirstOnly || !pstCom->fErrorReported)
		{
		//* if (iErrStr == 0)
		//* 	iErrStr = GM_TEST_FORMAT;  // just %s

		// Most error messages can be reported with a message error
		// string and (maybe) an optional string field. The optional
		// string is passed to utilReportError whether needed or not
		// since it does no harm if it is not referenced.

		//* utilReportError(pstCom->hSession, RE_ERROR | RE_OK,
		//* 		iErrStr, pszOptInfo);

		if (fFirstOnly)
			pstCom->fErrorReported = TRUE;
		}

	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComFreeDevice
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
void ComFreeDevice(const HCOM pstCom)
	{
	ComDeactivatePort(pstCom);

	if (pstCom->hDriverModule != (HANDLE)0)
		{
		(void)(*pstCom->pfDeviceClose)(pstCom->pvDriverData);
        pstCom->pvDriverData = 0;
		// FreeLibrary(pstCom->hDriverModule);
		pstCom->hDriverModule = (HANDLE)0;
		}

	pstCom->pfDeviceClose		  = ComDefDoNothing;
	pstCom->pfDeviceDialog		  = ComDefDeviceDialog;
	pstCom->pfDeviceGetCommon	  = ComDefDeviceGetCommon;
	pstCom->pfDeviceSetCommon	  = ComDefDeviceSetCommon;
	pstCom->pfDeviceSpecial 	  = ComDefDeviceSpecial;
    pstCom->pfDeviceLoadHdl 	  = ComDefDeviceLoadSaveHdl;
	pstCom->pfDeviceSaveHdl 	  = ComDefDeviceLoadSaveHdl;
	pstCom->pfPortConfigure 	  = ComDefDoNothing;
	pstCom->pfPortPreconnect	  = ComDefPortPreconnect;
	pstCom->pfPortActivate		  = ComDefPortActivate;
	pstCom->pfPortDeactivate	  = ComDefDoNothing;

	pstCom->fPortActive = FALSE;
	pstCom->szDeviceName[0] = (TCHAR)0;
	pstCom->stWorkSettings.szDeviceFile[0] = (TCHAR)0;
	pstCom->stWorkSettings.szPortName[0]   = (TCHAR)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ComValidHandle
 *
 * DESCRIPTION:
 *	Tests whether a com handle points to a valid, initialize structure
 *
 * ARGUMENTS:
 *	pstCom -- com handle to be tested
 *
 * RETURNS:
 *	TRUE if com handle appears to be valid
 *	FALSE if com handle if NULL or points to an invalid structure
 */
int ComValidHandle(HCOM pstCom)
	{
#if !defined(NDEBUG)
	if (pstCom == (HCOM)0 || pstCom->nGuard != COM_VERSION)
		return FALSE;
#endif
	return TRUE;
	}

