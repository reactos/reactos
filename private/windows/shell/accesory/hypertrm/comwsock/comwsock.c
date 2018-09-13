/*  File: D:\WACKER\comwsock\comwsock.c (Created: 12/26/95)
 *
 *  Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 2 $
 *  $Date: 2/05/99 3:20p $
 *
 *  $Log: /wacker/Comwsock/Comwsock.c $
 * 
 * 2     2/05/99 3:20p Supervisor
 * 64-bit changes and bug fixes for Microsoft
 *
 * 1     10/05/98 1:02p Supervisor
 * Revision 1.16  1998/09/11  11:41:41  JKH
 * none
 *
 * Revision 1.15  1998/09/10  14:54:58  bld
 * none
 *
 * Revision 1.14  1998/09/10  11:04:39  bld
 * none
 *
 * Revision 1.13  1998/09/09  16:15:37  rev
 * none
 *
 * Revision 1.12  1998/08/28  15:24:13  rev
 * none
 *
 * Revision 1.11  1998/08/28  10:31:23  bld
 * none
 *
 * Revision 1.10  1998/06/17  16:04:17  JKH
 * none
 *
 * Revision 1.9  1998/03/10  15:49:00  bld
 * none
 *
 * Revision 1.8  1997/03/24  09:53:07  JKH
 * Added Telnet break and command line telnet port selection
 *
 * Revision 1.7  1997/02/26  09:34:37  dmn
 * none
 *
 * Revision 1.6  1996/11/21  14:15:43  cab
 * Added call answering
 *
 * Revision 1.5  1996/02/22  14:24:07  jmh
 * Winsock com driver now uses same private data structure as standard com.
 *
 * Revision 1.4  1996/02/22  11:22:39  mcc
 * none
 *
 * Revision 1.3  1996/02/22  10:20:19  mcc
 * none
 *
 * Revision 1.2  1996/02/05  14:17:12  mcc
 * none
 *
 * Revision 1.1  1996/01/31  15:52:15  mcc
 * Winsock Comm driver
 *

 *Design Overview =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    This module would do Dr. Frankenstein proud ... it's basic structure is
	taken from WACKER's COMSTD module, but the internal logic is taken from
	HAWIN.  (I tried to port NPORT's OS/2 code but there is apparently some
	flaw in Win95's support for threaded Winsock apps, anyway I could not
	get it to work). This code is preserved here (controlled by #ifdef
	MULTITHREAD), it would be interesting to see if it works under Windows NT.
	
	A few random bits from NPORT are stitched in too ...
	
	This driver gets its remote address and port number settings from the
	connection driver via DeviceSpecial calls.  It stores nothing in
	the session file, so those load/save calls are stubs.
	
	Tidbits of possibly useful information:
	
		Sometimes a send() appears to succeed but no data appears at the other
		end. It appears that sometimes Winsock is internally waiting forever
		for something to happen, and never actually sends the data.  This
		might be caused by a recv() call outstanding that asks for
		a large amount of data, and maybe the send() call can't get enough
		memory until it completes (but it never WILL complete because the
		other system is waiting for data).  Also, I've seen this happening
		with the MULTITHREAD code, possibly due to a Winsock bug
		under Win95.
		
		The EscFF business is because Telnet (a protocol on top of TCP/IP
		that we will often encounter) uses FF as a command character,
		and sends FF FF as a literal FF.  Thus, file transfers to
		a system running Telnet must escape FF characters in a file
		by doubling them.
		
		The STDCOM drivers work by activating the port then sending
		data out to dial the modem, thus there is no ComConnect
		call in the high-level Com API.  For better or worse, TCP/IP
		requires that the IP address and Port number be supplied
		by the CNCT driver (via ComDeviceSpecial calls) before
		ComActivatePort is called so that activating the port actually
		establishes the link to the other system. In this kinda kludgy
		non-threaded implementation, ComActivatePort cannot block, so
		it will return success if it can successfully send out a
		connection request.  If the request ultimately fails, this
		driver will call ComNotify to let the CNCT driver know that there
		is a connection status change, and it must pick up the pieces.
		
		

*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

//#define DEBUGSTR

#include <windows.h>
#pragma hdrstop

#include <tdll\stdtyp.h>

#if defined(INCL_WINSOCK)

#include <tdll\session.h>
#include <tdll\mc.h>
#include <tdll\sf.h>
#include <tdll\timers.h>
#include <tdll\com.h>
#include <tdll\comdev.h>
#include <comstd\comstd.hh>
#include "comwsock.hh"
#include <tdll\assert.h>
#include <tdll\statusbr.h>
#include <tdll\tchar.h>
#include <tdll\com.hh>

BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
int wsckResolveAddress(TCHAR *pszRemote, unsigned long *pulAddr);
LRESULT FAR PASCAL WndSockWndProc(HWND hWnd, UINT uiMsg, WPARAM uiPar1, LPARAM lPar2);
BOOL WinSockCreateEventWindow (ST_STDCOM *pstPrivate);
int FAR PASCAL sndQueueAppend(ST_STDCOM *pstPrivate,
						VOID FAR *pvBufr, int nBytesToAppend);
int WinSockConnectSpecial(ST_STDCOM *pstPrivate);
int WinSockAnswerSpecial(ST_STDCOM *pstPrivate);

LONG WinSockConnectEvent(ST_STDCOM* pstPrivate, LPARAM lPar);
LONG WinSockReadEvent(ST_STDCOM* pstPrivate, LPARAM lPar);
LONG WinSockWriteEvent(ST_STDCOM*pstPrivate, LPARAM lPar);
LONG WinSockResolveEvent(ST_STDCOM* pstPrivate, LPARAM lPar);
LONG WinSockCloseEvent(ST_STDCOM* pstPrivate, LPARAM lPar);
LONG WinSockAcceptEvent(ST_STDCOM*pstPrivate, LPARAM lPar);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  ComLoadWinsockDriver
 *
 * DESCRIPTION:
 *  Loads the COM handle with pointers to the Winsock driver functions
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *  COM_OK		if successful
 *	COM_FAILED	otherwise
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int ComLoadWinsockDriver(HCOM pstCom)
	{

	int	iRetVal = COM_OK;

	if ( !pstCom )
		return COM_FAILED;

	pstCom->pfPortActivate   = WsckPortActivate;
	pstCom->pfPortDeactivate = WsckPortDeactivate;
	pstCom->pfPortConnected  = WsckPortConnected;
	pstCom->pfRcvRefill 	 = WsckRcvRefill;
	pstCom->pfRcvClear		 = WsckRcvClear;
	pstCom->pfSndBufrSend	 = WsckSndBufrSend;
	pstCom->pfSndBufrIsBusy  = WsckSndBufrIsBusy;
	pstCom->pfSndBufrClear	 = WsckSndBufrClear;
	pstCom->pfSndBufrQuery	 = WsckSndBufrQuery;
	pstCom->pfDeviceSpecial	 = WsckDeviceSpecial;
	pstCom->pfPortConfigure	 = WsckPortConfigure;

	return iRetVal;
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  WsckComWinsockEntry
 *
 * DESCRIPTION:
 *  Currently, just initializes the C-Runtime library but may be used
 *  for other things later.
 *
 * ARGUMENTS:
 *  hInstDll    - Instance of this DLL
 *  fdwReason   - Why this entry point is called
 *  lpReserved  - reserved
 *
 * RETURNS:
 *  BOOL
 *
 * AUTHOR:
 * mcc 12/26/95
 */
BOOL WINAPI WsckComWinsockEntry(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
    {
    hinstDLL = hInst;
    return _CRT_INIT(hInst, fdwReason, lpReserved);
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckDeviceInitialize
 *
 * DESCRIPTION:
 *  Called whenever the driver is being loaded
 *
 * ARGUMENTS:
 *  hCom               -- A copy of the com handle. Can be used in the
 *                          driver code to call com services
 *  usInterfaceVersion -- A version number identifying the version of the
 *                          driver interface
 *  ppvDriverData      -- A place to put the pointer to our private data.
 *                          This value will be passed back to us in all
 *                          subsequent calls.
 *
 * RETURNS:
 *  COM_OK if all is hunky dory
 *  COM_DEVICE_VERSION_ERROR if Wacker expects a different interface version.
 *  COM_NOT_ENOUGH_MEMORY
 *  COM_DEVICE_ERROR if anything else goes wrong
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckDeviceInitialize(HCOM hCom,
    unsigned nInterfaceVersion,
    void **ppvDriverData)
    {
    int        iRetVal = COM_OK;
    int        ix;
    ST_STDCOM *pstPrivate = NULL;

    //              Check version number and compatibility

    if (nInterfaceVersion != COM_VERSION)
        {
        // This error is reported by Com Routines. We cannot report errors
        // until after DeviceInitialize has completed.
        return COM_DEVICE_VERSION_ERROR;
        }

    if (*ppvDriverData)
        {
        pstPrivate = (ST_STDCOM*) *ppvDriverData;
        }
    else
        {
        // Allocate our private storage structure
        if ((pstPrivate = malloc(sizeof *pstPrivate)) == NULL)
            return COM_NOT_ENOUGH_MEMORY;
        *ppvDriverData = pstPrivate;
	    pstPrivate->hCom = hCom;
        pstPrivate->fNotifyRcv = TRUE;
	    pstPrivate->dwEventMask = 0;
	    pstPrivate->fSending = FALSE;
        pstPrivate->lSndTimer = 0L;
        pstPrivate->lSndLimit = 0L;
        pstPrivate->lSndStuck = 0L;
        pstPrivate->hwndEvents = (HWND)0;
        pstPrivate->nRBufrSize = WSOCK_SIZE_INQ;
        pstPrivate->pbBufrStart = NULL;
        pstPrivate->fHaltThread = TRUE;

        InitializeCriticalSection(&pstPrivate->csect);
        for (ix = 0; ix < EVENT_COUNT; ++ix)
            {
            pstPrivate->ahEvent[ix] = CreateEvent((LPSECURITY_ATTRIBUTES)0,
                TRUE, FALSE, NULL);
            if (!pstPrivate->ahEvent[ix])
                {
                iRetVal = COM_FAILED;
                break;
                }
            }
        }

    // Setup up reasonable default device values in case this type of
    //  device has not been used in a session before
	pstPrivate->hSocket = INVALID_SOCKET;
	pstPrivate->nPort = 23;
	pstPrivate->fConnected = 0;
    pstPrivate->hComReadThread = NULL;
    pstPrivate->hComWriteThread = NULL;
	pstPrivate->fEscapeFF = TRUE;
#ifdef INCL_CALL_ANSWERING
    pstPrivate->fAnswer = 0;
#endif

    if (iRetVal != COM_OK)
        {
        if (pstPrivate)
			{
            free(pstPrivate);
			pstPrivate = NULL;
			}
        }

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckDeviceClose
 *
 * DESCRIPTION:
 *  Called when Wacker is done with this driver and is about to release .DLL
 *
 * ARGUMENTS:
 *  pstPrivate -- Pointer to our private data structure
 *
 * RETURNS:
 *  COM_OK
 *
 * AUTHOR:
 * mcc 01/19/96
 */
int WINAPI WsckDeviceClose(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int ix;

    // Driver is about to be let go, do any cleanup
    // Port should have been deactivated before we are called, but
    //  check anyway.
    WsckPortDeactivate(pstPrivate);

    for (ix = 0; ix < EVENT_COUNT; ++ix)
        {
        CloseHandle(pstPrivate->ahEvent[ix]);
        }
    DeleteCriticalSection(&pstPrivate->csect);
    // Free our private data area
    free(pstPrivate);
	pstPrivate = NULL;

	DbgOutStr("WsckDeviceClose complete", 0,0,0,0,0);

    return COM_OK;
    }



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckDeviceSpecial
 *
 * DESCRIPTION:
 *  The means for others to control any special features in this driver
 *  that are not supported by all drivers.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *  COM_NOT_SUPPORTED if the instruction string was not recognized
 *  otherwise depends on instruction string
 *
 * AUTHOR:
 * mcc 12/26/95	(ported from NPORT)
 */
int WINAPI WsckDeviceSpecial(void *pvPrivate,
    const TCHAR *pszInstructions,
    TCHAR *pszResult,
    int   nBufrSize)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
	int				iRetVal = COM_NOT_SUPPORTED;
	unsigned long   ulSetVal;
	TCHAR			*pszEnd;
	TCHAR			achInstructions[100];
	TCHAR			*pszToken = achInstructions;
	int				iIndex;
	TCHAR			szResult[100];
	//ULONG			dwThreadID;

	static TCHAR *apszItems[] =
		{
		"IPADDR",
		"PORTNUM",
		"ISCONNECTED",
		"ESC_FF",					/* 3 */
        "ANSWER",
		NULL
		};

	// supported instruction strings:
	// "Set xxx=vv"
	// "Query xxx"


	if (!pszInstructions || !*pszInstructions)
		return COM_FAILED;

	//DbgOutStr("DevSpec: %s", pszInstructions, 0,0,0,0);

	if (sizeof(achInstructions) < (size_t)(StrCharGetStrLength(pszInstructions) + 1))
		return COM_NOT_SUPPORTED;

	StrCharCopy(achInstructions, pszInstructions);

	if (pszResult)
		*pszResult = '\0';

	pszToken = strtok(achInstructions, " ");
	if (!pszToken)
		return COM_NOT_SUPPORTED;

	EnterCriticalSection(&pstPrivate->csect);

	if (StrCharCmpi(pszToken, "SET") == 0)
		{
		iRetVal = COM_OK;
		pszToken = strtok(NULL, " =");
		if (!pszToken)
			pszToken = "";

		// Look up the item to set.
		for (iIndex = 0; apszItems[iIndex]; ++iIndex)
			if (StrCharCmpi(pszToken, apszItems[iIndex]) == 0)
				break;

		// Isolate the new value to be set
		pszToken = strtok(NULL, "\n");

		if (pszToken && *pszToken)
			{
			// Several items take numeric values
			ulSetVal = strtoul(pszToken, &pszEnd, 0);

			switch(iIndex)
				{
			case 0: // IPADDR
				ulSetVal = (unsigned) StrCharGetByteCount(pszToken);
				if ( ulSetVal < sizeof(pstPrivate->szRemoteAddr))
					{
					StrCharCopy(pstPrivate->szRemoteAddr, pszToken);
					iRetVal = 0;
					}
				else
					iRetVal = -1;
				break;

			case 1: // PORTNUM
				pstPrivate->nPort = (short) ulSetVal;
				iRetVal = 0;
				break;

			case 3: // ESC_FF
				pstPrivate->fEscapeFF = (int) atoi(pszToken);
				//DbgOutStr("set fEscapeFF = %d (%d) %s %d",
				//pstPrivate->fEscapeFF,ulSetVal,pszToken,
				//(int) atoi(pszToken),0);
				break;

            case 4: // ANSWER
#ifdef INCL_CALL_ANSWERING
                pstPrivate->fAnswer = ulSetVal;
                iRetVal = 0;
#else
                iRetVal = COM_FAILED;
#endif
                break;

			default:
				iRetVal = COM_FAILED;
				//DbgOutStr("DevSpec: Unrecognized instructions!", 0,0,0,0,0);
				break;
				}
			}
		else	// if (pszToken && *pszToken)
			{
			assert(0);
			iRetVal = COM_NOT_SUPPORTED;
			}
		}
	else if (StrCharCmpi(pszToken, "QUERY") == 0)
		{
		iRetVal = COM_OK;
		pszToken = strtok(NULL, "\n");
		szResult[0] = '\0';

		// Look up the item to query
		for (iIndex = 0; apszItems[iIndex]; ++iIndex)
			if (StrCharCmpi(pszToken, apszItems[iIndex]) == 0)
				break;

		if (*pszToken)
			{
			switch(iIndex)
				{
			case 0: // IPADDR
				StrCharCopy(szResult, pstPrivate->szRemoteAddr);
				iRetVal = 0;
				break;

			case 1: // PORTNUM
				wsprintf(szResult, "%d", pstPrivate->nPort);
				iRetVal = 0;
				break;

			case 2: // ISCONNECTED
				wsprintf(szResult, "%d", pstPrivate->fConnected);
				iRetVal = 0;
				break;

            case 4: // ANSWER
#ifdef INCL_CALL_ANSWERING
                wsprintf(szResult, "%d", pstPrivate->fAnswer);
                iRetVal = 0;
#else
                iRetVal = COM_FAILED;
#endif
                break;

			default:
				iRetVal = COM_FAILED;
				break;
				}
			if ( iRetVal == 0 && StrCharGetByteCount(szResult) <
				nBufrSize )
				StrCharCopy(pszResult, szResult);
			else
				iRetVal = COM_FAILED;
			}
		}
     else if (lstrcmpi(pszInstructions, "Send Break") == 0)
        {
        // This is the telent "Break" key processing.  When
        // the user presses Ctrl-Break on the terminal sreen
        // with a WinSOck connection, we arrive here.
        //
        // Please refer to RFC 854 for specifics on this
        // implementation.  Basically, we need to..
        // send the IAC BREAK signal (0xFF 0xF3)
        unsigned char ach[2];

        ach[0] = IAC;
        ach[1] = BREAK;

		if (send(pstPrivate->hSocket, ach,	2, 0) != 2)
            {
            assert(0);
            }

        iRetVal = COM_OK;
        }
   else if (lstrcmpi(pszInstructions, "Send IP") == 0)
        {
        // This is the telent Interrupt Process.  When
        // the user presses Alt-Break on the terminal sreen
        // with a WinSock connection, we arrive here.
        //
        // Please refer to RFC 854 for specifics on this
        // implementation.  Basically, we need to..
        //
        // Send the Telnet IP (Interrupt Processs)
        // sequence (0xFF 0xF4).
        //
        // Send the Telnet SYNC sequence.  That is,
        // send the Data Mark (DM) as the only character
        // is a TCP urgent mode send operation (the mode
        // flag MSG_OOB does this for us).
        //
        unsigned char ach[2];

        SndBufrClear(pstPrivate);

        ach[0] = IAC;
        ach[1] = IP;

		if (send(pstPrivate->hSocket, ach,	2, 0) != 2)
            {
            assert(0);
            }

        ach[0] = IAC;
        ach[1] = DM;

        if (send(pstPrivate->hSocket, ach,	2, MSG_OOB) != 2)
            {
            assert(0);
            }


        iRetVal = COM_OK;
        }
	else if (lstrcmpi(pszInstructions, "Update Terminal Size") == 0)
        {
		// The dimensions of the terminal have changed. If we have negotiated
		// to use the Telnet NAWS option, (Negotiate About Terminal Size), then
		// we must send the new terminal size to the server. This method will
		// only send data out if the option has been enabled.
		WinSockSendNAWS( pstPrivate );
		}

	LeaveCriticalSection(&pstPrivate->csect);

	return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  WsckDeviceLoadHdl
 *
 * DESCRIPTION:
 *	We need a function that appears to load/save to the session file,
 *  returning SF_OK, but actually doing nothing since this driver saves
 *  no settings.
 *
 * ARGUMENTS:
 *  pstPrivate  -- dummy (not used)
 *  sfHdl       -- dummy (not used)
 *
 * RETURNS:
 *
 * AUTHOR:
 * mcc 01/19/95
 */
int WINAPI WsckDeviceLoadHdl(void *pvPrivate, SF_HANDLE sfHdl)
    {
    return SF_OK;
    }/*lint !e715 */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  WsckDeviceSaveStub
 *
 * DESCRIPTION:
 *	We need a function that appears to Save/save to the session file,
 *  returning SF_OK, but actually doing nothing since this driver saves
 *  no settings.
 *
 * ARGUMENTS:
 *  pstPrivate  -- dummy (not used)
 *  sfHdl       -- dummy (not used)
 *
 * RETURNS:
 *
 * AUTHOR:
 * mcc 01/19/95
 */
int WINAPI WsckDeviceSaveHdl(void *pvPrivate, SF_HANDLE sfHdl)
    {
    return SF_OK;
    }/*lint !e715 */



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckDeviceStub
 *
 * DESCRIPTION:
 *  Stub that returns COM_OK (unlike default stubs)
 *
 * ARGUMENTS:
 *  pstPrivate -- not used
 *
 * RETURNS:
 *  COM_OK if port is configured successfully
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckDeviceStub(void *pvPrivate)
    {
    int          iRetVal = COM_OK;

    return iRetVal;
    } /*lint !e715 */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortConfigure
 *
 * DESCRIPTION:
 *  Configures an open port with the current set of user settings
 *
 * ARGUMENTS:
 *  pstPrivate -- The driver data structure
 *
 * RETURNS:
 *  COM_OK if port is configured successfully
 *  COM_DEVICE_ERROR if API errors are encountered
 *  COM_DEVICE_INVALID_SETTING if some user settings are not valid
 */
int WINAPI WsckPortConfigure(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int          iRetVal = COM_OK;
    unsigned     uOverrides = 0;

    // Check for overrides
    ComQueryOverride(pstPrivate->hCom, &uOverrides);
    if (bittest(uOverrides, COM_OVERRIDE_8BIT))
        {
    	DbgOutStr("Requesting binary Telnet mode\n", 0,0,0,0,0);
		// Ask the other side to send binary data (default
		// is 7-bit ASCII), and inform them that we will
		// be sending binary data.
		WinSockSendMessage(pstPrivate, DO, TELOPT_BINARY);
		WinSockSendMessage(pstPrivate, WILL, TELOPT_BINARY);
    }

    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortConnected
 *
 * DESCRIPTION:
 *  Determines whether the driver is currently connected to a host system.
 *	(Sort of like having a "carrier" in the STDCOM drivers)
 *
 * ARGUMENTS:
 *  pstPrivate -- Our private data structure
 *
 * RETURNS:
 *  TRUE if we have an active connection
 *  FALSE otherwise
 *
 * AUTHOR:
 * mcc 01/19/96
*/
int WINAPI WsckPortConnected(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;

    return pstPrivate->fConnected;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckSndBufrIsBusy
 *
 * DESCRIPTION:
 *  Determines whether the driver is available to transmit a buffer of
 *  data or not.
 *
 * ARGUMENTS:
 *  pstPrivate -- address of com driver's data structure
 *
 * RETURNS:
 *  COM_OK   if data can be transmitted
 *  COM_BUSY if driver is still working on a previous buffer
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckSndBufrIsBusy(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int  iRetVal = COM_OK;

    EnterCriticalSection(&pstPrivate->csect);

    if (pstPrivate->fSending)
		{
        iRetVal = COM_BUSY;
		}

    LeaveCriticalSection(&pstPrivate->csect);

    // DBG_WRITE((iRetVal==COM_BUSY)?"Snd Bufr Busy\r\n":"Snd Bufr Ready\r\n",
		// 0,0,0,0,0);

    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	WsckSndBufrQuery
 *
 * DESCRIPTION:
 *	A stub; I'm not really sure what else it COULD do in TCP/IP
 *
 * ARGUMENTS:
 *	ignored
 *
 * RETURNS:
 *	COM_OK
 *
 * AUTHOR:
 * 	mcc 12/26/95
 */	
int WINAPI WsckSndBufrQuery(void *pvPrivate,
    unsigned *pafStatus,
    long *plHandshakeDelay)
    {
    int     iRetVal = COM_OK;

    return iRetVal;
    }


#if !defined(MULTITHREAD)
	// WINSOCK the way we know and love it from Win3.1 days

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortActivate
 *
 * DESCRIPTION:
 *  Called to activate the port and connect to destination
 *
 * ARGUMENTS:
 *  pstPrivate  -- driver data structure
 *  pszPortName -- not used
 *  dwMediaHdl  -- not used (stub used only by TAPI-aware drivers)
 *
 * RETURNS:
 *  COM_OK if port is successfully activated
 *  COM_NOT_ENOUGH_MEMORY if there in insufficient memory for data storage
 *  COM_NOT_FOUND if named port cannot be opened
 *  COM_DEVICE_ERROR if API errors are encountered
 *
 * AUTHOR:
 * 	mcc 12/26/95
 */
int WINAPI WsckPortActivate(void *pvPrivate,
    TCHAR *pszPortName,
    DWORD_PTR dwMediaHdl)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int             iRetVal = COM_OK;
	WNDCLASS wc;
    ST_COM_CONTROL *pstComCntrl;

    // Make sure we can get enough memory for buffers before opening device
    pstPrivate->pbBufrStart = malloc((size_t)pstPrivate->nRBufrSize);

    if (pstPrivate->pbBufrStart == NULL)
        {
        iRetVal = COM_NOT_ENOUGH_MEMORY;
        //* DeviceReportError(pstPrivate, SID_ERR_NOMEM, 0, TRUE);
        goto checkout;
        }

    pstPrivate->pbBufrEnd = pstPrivate->pbBufrStart + pstPrivate->nRBufrSize;
    pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;
    pstPrivate->pbComStart = pstPrivate->pbComEnd = pstPrivate->pbBufrStart;
    pstPrivate->fBufrEmpty = TRUE;
	pstPrivate->nSendBufrLen = 0;


    if (iRetVal == COM_OK)
        {
        pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
        pstComCntrl->puchRBData =
            pstComCntrl->puchRBDataLimit =
            pstPrivate->pbBufrStart;

        pstPrivate->dwEventMask = EV_ERR | EV_RLSD;
        pstPrivate->fNotifyRcv = TRUE;
        pstPrivate->fBufrEmpty = TRUE;
		}



	// Register event window class to handle Winsock asynchronous
	// notifications
	wc.style         = CS_GLOBALCLASS;
	wc.lpfnWndProc   = WndSockWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = sizeof(ST_STDCOM*);
	wc.hInstance     = hinstDLL;
	wc.hIcon         = (HICON)0;
	wc.hCursor       = (HCURSOR)0;
	wc.hbrBackground = (HBRUSH)0;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = WINSOCK_EVENT_WINDOW_CLASS;

	// Register the class, we don't check for failure because the driver
	//  can operate without it if it has to
	RegisterClass(&wc);
		
	WinSockCreateNVT(pstPrivate);
		
		
	if (!WinSockCreateEventWindow(pstPrivate))
		{
		iRetVal = COM_DEVICE_ERROR;
		goto checkout;
		}

	// Kick off Winsock processing
	PostMessage(pstPrivate->hwndEvents, WM_WINSOCK_STARTUP,
				0, 0L);


checkout:
    if (iRetVal != COM_OK)
        WsckPortDeactivate(pstPrivate);

    return iRetVal;
    }  /*lint !e715 */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortDeactivate
 *
 * DESCRIPTION:
 *  Deactivates and closes an open port
 *
 * ARGUMENTS:
 *  pstPrivate -- Driver data structure
 *
 * RETURNS:
 *  COM_OK
 *
 * AUTHOR:
 * 	mcc 12/26/95
 */
int WINAPI WsckPortDeactivate(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int iRetVal = COM_OK;


	// Shut down socket and WINSOCK
	shutdown(pstPrivate->hSocket, 2);
	closesocket(pstPrivate->hSocket);
	WSACleanup();
	pstPrivate->hSocket = INVALID_SOCKET;
	
	// Destroy the WINSOCK event window
	if (pstPrivate->hwndEvents)
		{
		DestroyWindow(pstPrivate->hwndEvents);
		pstPrivate->hwndEvents = 0;
		}
		
	// Destroy the read buffer		
    if (pstPrivate->pbBufrStart)
        {
        free(pstPrivate->pbBufrStart);
        pstPrivate->pbBufrStart = NULL;
        }

    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckRcvRefill
 *
 * DESCRIPTION:
 *  Called when the receive buffer is empty to refill it. This routine
 *  should attempt to refill the buffer and return the first character.
 *  It is important that this function be implemented efficiently.
 *
 * ARGUMENTS:
 *  pstPrivate -- the driver data structure
 *
 * RETURNS:
 *  TRUE if data is put in the receive buffer
 *  FALSE if there is no new incoming data
 *
 * AUTHOR:
 * 	mcc 01/18/95 (from HAWIN)
 */
int WINAPI WsckRcvRefill(void *pvPrivate)
	{
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
	int nIndx;
	int iBytesRead = 0;
	int nNVTRes;
	int nBytesCopied;
	ST_COM_CONTROL FAR *pstComCntrl;

	if (pstPrivate->fConnected == 0)
		return FALSE;

	//for (nIndx = 0; nIndx < (int)pstPrivate->usRBufrSize; nIndx += 1)
		//pstPrivate->puchRBufr[nIndx] = 0;

	// Read up to pstPrivate->usRBufrSize bytes into pstPrivate->puchRBufr
	// and set iBytesRead to the number read.
	iBytesRead = 0;
	iBytesRead = recv(pstPrivate->hSocket,
						(LPSTR)pstPrivate->pbBufrStart,
						(int)pstPrivate->nRBufrSize,
						0);
	if (iBytesRead == SOCKET_ERROR)
		{
		int iErr;

		iBytesRead = 0;
		iErr = WSAGetLastError();
		if (iErr != WSAEWOULDBLOCK)
			DbgOutStr("Refill: error %d reading %d bytes on socket %d\n", iErr,
			pstPrivate->nRBufrSize,
				pstPrivate->hSocket,0,0);
		}

	if (iBytesRead == 0)
		{
		ComNotify(pstPrivate->hCom, NODATA);
		return FALSE;
		}
		

	// update the com handle with info on new data. This is implemented
	// this way to allow HA to access these characters quickly
	nBytesCopied = 0;
	for (nIndx = 0; nIndx < iBytesRead; nIndx++)
		{
		// If we have an FF or we are in the middle of a Telnet
		// command, run this character thru the NVT.  Unless the
		// says to discard the character, we then copy it to the
		// output position.
		if (pstPrivate->pbBufrStart[nIndx] == 0xFF ||
		    pstPrivate->NVTstate != NVT_THRU)
			{
			nNVTRes = WinSockNetworkVirtualTerminal(
				(ECHAR) pstPrivate->pbBufrStart[nIndx],
				(void far *) pstPrivate);
			//DbgOutStr("NVT returns %d\n", nNVTRes, 0,0,0,0);		
			}
		else
			nNVTRes = NVT_KEEP;
		
		if (nNVTRes != NVT_DISCARD)
			{
			pstPrivate->pbBufrStart[nBytesCopied] = pstPrivate->pbBufrStart[nIndx];
			nBytesCopied++;
			}
		}

	// if we got no data (perhaps because data were "eaten" by NVT),
	// make sure we return -1
	if (nBytesCopied == 0)
		*(pstPrivate->pbBufrStart) = (char) -1;

	pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
	pstComCntrl->puchRBData = pstPrivate->pbBufrStart;
	pstComCntrl->puchRBDataLimit = pstPrivate->pbBufrStart + nBytesCopied;

	return TRUE;	// Return first character
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckRcvClear
 *
 * DESCRIPTION:
 *  Clears the receiver of all received data.
 *
 * ARGUMENTS:
 *  hCom -- a comm handle returned by an earlier call to ComCreateHandle
 *
 * RETURNS:
 *  COM_OK if data is cleared
 *  COM_DEVICE_ERROR if Windows com device driver returns an error
 *
 * AUTHOR:
 * mcc 01/18/96	(taken almost entirely from HAWIN)
 */
int WINAPI WsckRcvClear(void *pvPrivate)
	{
	CHAR ch[128];
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    ST_COM_CONTROL *pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;

	if (pstPrivate->fConnected == 0)
		return COM_DEVICE_ERROR;

	pstComCntrl->puchRBData = pstComCntrl->puchRBDataLimit =
			pstPrivate->pbBufrStart;

	// Do whatever is necessary to remove any buffered data from the com port

	while (recv(pstPrivate->hSocket, ch, 128, 0) != SOCKET_ERROR)
			{
			}

	return COM_OK;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckSndBufrSend
 *
 * DESCRIPTION:
 *	Transmits a buffer of characters. This routine need only queue up a
 *	buffer to be transmitted. If the com device supports interrupt-driven
 *	or hardware-controlled transmission, this function should start the
 *	process but should not wait until all the data has actually been sent.
 *
 * ARGUMENTS:
 *	pstPrivate -- Pointer to driver data structure
 *	pvBufr	   -- Pointer to data to send
 *	nSize	   -- Number of bytes to send
 *
 * RETURNS:
 *	COM_OK
 *	or appropriate error code
 *
 * AUTHOR:
 * 	mcc 01/19/96
 */
int WINAPI WsckSndBufrSend(void *pvPrivate, void *pvBufr, int  nBytesToSend)
	{
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
	int 	nCount;
	int 	nError;
	int	 	nSize;		 	// num bytes to send during this pass
	int		usReturnValue = COM_OK;
	unsigned char *pszPtr = (unsigned char *)pvBufr;
  	unsigned char *pcThisPassData;
	int		fGotFF = FALSE;	// TRUE if last char detected was an FF
	int		nOffset;
	LPSTR   puchRemains;
	int		fQueueing = FALSE;


	assert(pvBufr != (VOID FAR *)0);
	assert(nBytesToSend <= WSOCK_SIZE_OUTQ);
	
	
	if (pstPrivate->fSending)
		{
		DbgOutStr("SBS: Busy = %d\n", pstPrivate->fSending, 0,0,0,0);
		return COM_BUSY;
		}
		
	assert(pstPrivate->nSendBufrLen == 0);

	if (pstPrivate->fConnected == 0)	
		return COM_DEVICE_ERROR;


	// If we are escaping FF characters by sending them as FFFF,
	// things are a bit tricky because we have no extra room in the buffer
	// for the doubled characters.  The idea here is to send the buffer
	// in several passes; if an FF character is found, it is sent once
	// at the end of one pass and again at the beginning of the next
	// At the end of each pass, we decrement nBytesToSend by the
	// number of bytes sent during the pass (nSize), and keep looping
	// until all the data are sent
	nOffset = 0;
	ComNotify(pstPrivate->hCom, SEND_STARTED);						
	while (nBytesToSend > 0 && usReturnValue == COM_OK)
		{
		if (pstPrivate->fEscapeFF)
			{
			pcThisPassData = &pszPtr[nOffset];
			
			// If we are processing an FF that was found on the
			// last pass, send it out again by itself.	Otherwise,
			// search for the next FF and send everything up to and
			// including it.
			if (fGotFF)
				{
				//DbgOutStr("SndBufrSend: 2nd FF\n", 0,0,0,0,0);
				nSize = 1;
				nOffset++;
				fGotFF = FALSE;
				}
			else
				{
				nSize = 0;
				while (pszPtr[nOffset] != 0xFF && nOffset < nBytesToSend)
					nSize++, nOffset++;
				
				// If no FF's were found, send everything
				if (nOffset >= nBytesToSend)	
					{
					nBytesToSend = 0;
					fGotFF = 0;
					}
				// otherwise, send data up to and including FF
				else
					{
					nSize++;  		// include the FF!
					fGotFF = TRUE;	// Send the 2nd FF on next pass
					//DbgOutStr("SndBufrSend: 1st FF ...", 0,0,0,0,0);
					}
				}
													
			}
		else  // send everything in one pass
			{
			nSize = nBytesToSend;
			nBytesToSend = 0;
			pcThisPassData = pvBufr;
			}
		// If we already have data queued, don't try to send directly, since
		// it might get out before the queued data do.	
		if (fQueueing)
			{
			DbgOutStr("SBS queueing output.  Queueing %d bytes\n",
						nSize,0,0,0,0);
			if (sndQueueAppend(pstPrivate,pcThisPassData,nSize) != COM_OK)
						usReturnValue = COM_DEVICE_ERROR;
			}
		else
			{
			// Pass data to TCP/IP
			nCount = 0;
			nCount = send(pstPrivate->hSocket,
							pcThisPassData,	(int)nSize,	0);
								
			// If we got a "would block" error, copy the data to send
			// to pstPrivate->auchSndBufr.  (Since the FF processing may
			// cause this block of code to be executed multiple times in
			// one SndBufrSend call, we will append new data to existing
			// data in the buffer).
			if (nCount == SOCKET_ERROR)
				{								
				nError = WSAGetLastError();

				if (nError == WSAEWOULDBLOCK)
					{
					// Winsock won't accept data, so queue it up for
					// WinSockWriteEvent to handle.  Also, lock the handle
					// until we are done so that WinSockWriteEvent can't get in
					// there.
					fQueueing = TRUE;

					DbgOutStr("SBS would block.  Queueing %d bytes\n",
						nSize,0,0,0,0);
					EnterCriticalSection(&pstPrivate->csect);
					if (sndQueueAppend(pstPrivate,pcThisPassData,nSize) != COM_OK)
						usReturnValue = COM_DEVICE_ERROR;
					}
				else
					{
					DbgOutStr("WinSock send error %d\r\n", nError, 0,0,0,0);
					//DeviceReportError(pstPrivate, (UINT)nError, 0, FALSE);
					usReturnValue = COM_DEVICE_ERROR;
					DbgOutStr("SBS: Bad error\n", 0,0,0,0,0);
					}
				}
			else
				{
   				ComNotify(pstPrivate->hCom, SEND_DONE);
				if (nCount < (int)nSize)
					{
					/*
					 * Set stuff up for next time through
					 */
					DbgOutStr("SBS send incomplete..  Queueing %d bytes\n",
						(int)nSize-nCount,0,0,0,0);
			
					// Get pointer to remaining data and queue it up		
					puchRemains = pcThisPassData + nCount;
					if (sndQueueAppend(pstPrivate, puchRemains,
							(nSize - nCount)) != COM_OK)
						{
						usReturnValue = COM_DEVICE_ERROR;
						DbgOutStr("SBS: Bad error\n", 0,0,0,0,0);
						}
					}
				}
			}

   	}
	if (fQueueing)
		LeaveCriticalSection(&pstPrivate->csect);

	return   usReturnValue;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: sndQueueAppend
 *
 * DESCRIPTION:
 *  Appends new data to any data that have been queued up for
 *  re-transmission by WinSockWriteEvent
 *
 * ARGUMENTS:
 *	pstPrivate 		-- address of com driver's data structure
 *  pvBufr      	-- address of new data block to queue up
 *  nBytesToAppend	-- size of new data	block to append
 *
 * RETURNS:
 *	COM_OK	 if data can be transmitted
 *	COM_BUSY if no more room for data to queue up
 *
 * AUTHOR:
 * 	mcc 01/19/96 (from HAWIN)
 */

int FAR PASCAL sndQueueAppend(ST_STDCOM* pstPrivate,
						VOID FAR *pvBufr, int nBytesToAppend)
						
	{					
	LPSTR	puchEnd;
	USHORT  usReturns = COM_OK;

	assert( pstPrivate != NULL );
	assert( pvBufr != NULL );

	//jkh 9/11/98 to avoid memcpy with invalid params
	if ( pstPrivate && pvBufr && nBytesToAppend > 0 )
		{
		if (pstPrivate->nSendBufrLen + nBytesToAppend >
				(int) sizeof(pstPrivate->abSndBufr))
			{
			DbgOutStr("SQAPP: buffer full", 0,0,0,0,0);
			return COM_BUSY;
			}
	
		// Set the flag that SndBufrIsBusy looks at; we are
		// in the middle of a send until the buffer is cleared
		// by WinSockWriteEvent
		pstPrivate->fSending = TRUE;
		
		pstPrivate->pbSendBufr = pstPrivate->abSndBufr;			
		puchEnd = pstPrivate->pbSendBufr + pstPrivate->nSendBufrLen;

		DbgOutStr("sQA: appending %d bytes to addr = %lx.  Existing buffer is %d bytes at %lx\n",
			nBytesToAppend,	puchEnd, pstPrivate->nSendBufrLen, pstPrivate->pbSendBufr,0);
		pstPrivate->nSendBufrLen += nBytesToAppend;
		MemCopy(puchEnd, (LPSTR) pvBufr, (unsigned) nBytesToAppend);
		}

	DbgOutStr("sQA: copy done\n", 0,0,0,0,0);
	return usReturns;
	}




/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckSndBufrClear
 *
 * DESCRIPTION:
 *	Clear any data waiting to be transmitted
 *
 * ARGUMENTS:
 *	pstPrivate -- pointer to driver data structure
 *
 * RETURNS:
 *	COM_OK
 *	or appropriate error code
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
int WINAPI WsckSndBufrClear(ST_STDCOM *pstPrivate)
	{
	USHORT usReturnValue = COM_OK;

	DbgOutStr("SndBufrClear called", 0,0,0,0,0);

	if (WsckSndBufrIsBusy(pstPrivate))
		{
		pstPrivate->nSendBufrLen = 0;
		pstPrivate->pbSendBufr = 0;
		}

	// Call SndBufrIsBusy again to clear flags, timers, etc.
	WsckSndBufrIsBusy(pstPrivate);
	return usReturnValue;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WndSockWndProc
 *
 * DESCRIPTION:
 *	This is the window procedure for the window that is used to receive event
 *	messages from the WinSock interface.  It uses this to get around the pesky
 *	problem of blocking vs. non-blocking requirements and expectations.
 *
 * ARGUEMENTS:
 *	The usual stuff for a window procedure.
 *
 * RETURNS:
 *	The usual stuff for a window procedure.
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
LRESULT FAR PASCAL WndSockWndProc(HWND hWnd, UINT uiMsg, WPARAM uiPar1, LPARAM lPar2)
	{
	ST_STDCOM*pstPrivate;
	WORD wVersion;
	WSADATA stWsaData;

	switch (uiMsg)
		{
		case WM_WINSOCK_STARTUP:
			pstPrivate = (ST_STDCOM*)GetWindowLongPtr(hWnd, 0);
			if (pstPrivate == (ST_STDCOM*)0)
				break;

			DbgOutStr("Calling WSAStartup\n", 0,0,0,0,0);
			/*
			 * Initialize the Windows Socket DLL
			 */
			wVersion = 0x0101;			// The version of WinSock that we want
			if (WSAStartup(wVersion, &stWsaData) != 0)
				{
				/* No DLL was available */
				return COM_DEVICE_ERROR;
				}
			DbgOutStr("Done calling WSAStartup\n", 0,0,0,0,0);
		   //	pstPrivate->fActive = TRUE;

			/* Confirm that the Windows Socket DLL supports 1.1. */
			/* Note that if the DLL supports versions greater    */
			/* than 1.1 in addition to 1.1, it will still return */
			/* 1.1 in wVersion since that is the version we      */
			/* requested                                         */
			if ((LOBYTE(stWsaData.wVersion) != 1) &&
				(HIBYTE(stWsaData.wVersion) != 1))
				{
				/* No acceptable DLL was available */
				return COM_DEVICE_ERROR;
				}

			/*
			 * Create a socket for later use.
			 */
			DbgOutStr("Calling socket\n", 0,0,0,0,0);
			
			pstPrivate->hSocket = socket(PF_INET, SOCK_STREAM, 0);
			if (pstPrivate->hSocket == INVALID_SOCKET)
				{
				return COM_DEVICE_ERROR;
				}
			DbgOutStr("Done calling socket\n", 0,0,0,0,0);

#ifdef INCL_CALL_ANSWERING			
            if (pstPrivate->fAnswer)
                {
			    WinSockAnswerSpecial(pstPrivate);
                }
            else
                {
			    WinSockConnectSpecial(pstPrivate);
                }
#else
			WinSockConnectSpecial(pstPrivate);
#endif
			break;
			
		case WM_WINSOCK_NOTIFY:
			{
			pstPrivate = (ST_STDCOM*)GetWindowLongPtr(hWnd, 0);
			if (pstPrivate == (ST_STDCOM*)0)
				break;

			switch(LOWORD(lPar2))
				{
				case FD_READ:
					return WinSockReadEvent(pstPrivate, lPar2);

				case FD_WRITE:
					return WinSockWriteEvent(pstPrivate, lPar2);

				case FD_CLOSE:
					//DbgOutStr("FD_CLOSE\r\n", 0,0,0,0,0);
					return WinSockCloseEvent(pstPrivate, lPar2);

				case FD_CONNECT:
					//DbgOutStr("FD_CONNECT\r\n", 0,0,0,0,0);
					return WinSockConnectEvent(pstPrivate, lPar2);

				case FD_ACCEPT:
					//DbgOutStr("FD_ACCEPT\r\n", 0,0,0,0,0);
					return WinSockAcceptEvent(pstPrivate, lPar2);

				default:
					break;
				}
			}
			break;

		case WM_WINSOCK_RESOLVE:
			{
			/* We get here after a call to WSAAsyncGetHostByName */
			pstPrivate = (ST_STDCOM*)GetWindowLongPtr(hWnd, 0);
			if (pstPrivate == (ST_STDCOM*)0)
				break;

			return WinSockResolveEvent(pstPrivate, lPar2);
			}

		default:
			break;
		}

	return DefWindowProc(hWnd, uiMsg, uiPar1, lPar2);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WndSockCreateEventWindow
 *
 * DESCRIPTION:
 *	Creates the event window used to process messages sent by the WinSock DLL.
 *
 * ARGUMENTS:
 *	pstPrivate		pointer to private data structure; attach to window
 *
 * RETURNS:
 *	TRUE if everything is OK, otherwise FALSE.
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
BOOL WinSockCreateEventWindow (ST_STDCOM *pstPrivate)
	{
	BOOL fRetVal = TRUE;

	if (fRetVal)
		{
		pstPrivate->hwndEvents = CreateWindow(
										WINSOCK_EVENT_WINDOW_CLASS,
										"",
										WS_OVERLAPPEDWINDOW,
										0, 0, 0, 0,
										HWND_DESKTOP,
										NULL,
										hinstDLL,
										NULL);
		fRetVal = (pstPrivate->hwndEvents != (HWND)0);
		}

	if (fRetVal)
		{
		SetWindowLongPtr(pstPrivate->hwndEvents, 0, (LONG_PTR)pstPrivate);
		}

	return fRetVal;
	}
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	WinSockSendBreak
 *
 * DESCRIPTION:
 *	This function attempts to send a break condition to the NVT on the other
 *	end of the connection.  See RFC854 for details and good luck.
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *
 * RETURNS:
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
VOID WinSockSendBreak(ST_STDCOM*pstPrivate)
	{
	UCHAR acSendBreak[2];

	//DbgOutStr("WinSockSendBreak\r\n", 0,0,0,0,0);

	acSendBreak[0] = 255;
	acSendBreak[1] = 243;
	send(pstPrivate->hSocket, acSendBreak, 2, 0);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 */
USHORT WinSockSendBreakSpecial(ST_STDCOM *pstPrivate,
							LPSTR pszData,
							UINT uiSize)
	{
	if (pstPrivate->nSendBufrLen > 0)
		{
		/* Can't do it now, wait until next time */
		pstPrivate->fSndBreak = TRUE;
		}
	else
		{
		WinSockSendBreak(pstPrivate);
		}

	return 0;
	} /*lint !e715 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
int WinSockConnectSpecial(ST_STDCOM*pstPrivate)
	{
	USHORT usRetVal;
	int nIndx;
	int nError;
	unsigned long ulAddr;
	struct sockaddr_in	srv_addr;
	struct sockaddr_in	cli_addr;
	HANDLE  hReturn;

	usRetVal = COM_OK;

	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = INADDR_ANY;
	cli_addr.sin_port = 0;

	// Bind the socket to any internet address.  Ignore errors;
	// real ones will be detected later, and this WILL fail
	// if the socket is already bound.
	usRetVal = (USHORT)bind(pstPrivate->hSocket, (LPSOCKADDR)&cli_addr,
			sizeof(cli_addr));
	DbgOutStr("Socket %d bind returns %d\n", pstPrivate->hSocket, usRetVal, 0,0,0);

	// See if the remote address has been entered in numeric form.  If not,
	// we will have to call WSAAsynchGetHostByName to translate it; in that
	// case, the EventWindow handler will call connect.
	ulAddr = inet_addr(pstPrivate->szRemoteAddr);
	if ((ulAddr == INADDR_NONE) || (ulAddr == 0))
		{
		DbgOutStr("WSCnctSp: calling WSA...HostByName\n", 0,0,0,0,0);
		/* We take the long way around */
		hReturn = WSAAsyncGetHostByName(
							pstPrivate->hwndEvents,
							WM_WINSOCK_RESOLVE,
							pstPrivate->szRemoteAddr,
							(char *) pstPrivate->pstHostBuf,
							MAXGETHOSTSTRUCT);
		
		if (hReturn == 0)
			nError = WSAGetLastError();
		else
			nError = 0 ;		
		DbgOutStr("WSAAsynchGetHostByName returns %lx (err = %d)\n", hReturn,
				nError,0,0,0);
		goto WSCSexit;
		}

	/*
	 * This is the alternate pathway for the connect.  We go thru here if
	 * the address was in the form of a 123.456.789.123 address.
	 */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = ulAddr;
	srv_addr.sin_port = htons((USHORT)pstPrivate->nPort);

	nIndx = WSAAsyncSelect(pstPrivate->hSocket,
							pstPrivate->hwndEvents,
							WM_WINSOCK_NOTIFY,
							FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE
							);

	if (nIndx != 0)
		{
		/* Oops, something goofed */
		usRetVal = COM_DEVICE_ERROR;
		DbgOutStr("WSAAsyncSelect failed\n", 0,0,0,0,0);
		goto WSCSexit;
		}

	if (connect(pstPrivate->hSocket,
							(LPSOCKADDR)&srv_addr,
							sizeof(srv_addr)) == SOCKET_ERROR)
		{
		nIndx = WSAGetLastError();
		if (nIndx != WSAEWOULDBLOCK)
			{
			usRetVal = COM_DEVICE_ERROR;
			//DeviceReportError(pstPrivate, (UINT)nIndx, 0, FALSE);
			DbgOutStr("Connect failed (err = %d)\n", nIndx, 0,0,0,0);
			goto WSCSexit;
			}
		}

WSCSexit:
	//DbgOutStr(" returns %d\r\n", usRetVal, 0,0,0,0);
	if (usRetVal != COM_OK)
		{
	   closesocket(pstPrivate->hSocket);
		}

	return usRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  WinSockAnswerSpecial
 *
 * DESCRIPTION:
 *  Sets up WinSock to answer a call.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR:  C. Baumgartner, 11/19/96 (ported from HAWin16)
 */
int WinSockAnswerSpecial(ST_STDCOM*pstPrivate)
	{
	int                nError = 0;
	USHORT             usRetVal = COM_OK;
	struct sockaddr_in host_addr;

    // Create our local internet address.
    //
	host_addr.sin_family = AF_INET;
	host_addr.sin_addr.s_addr = INADDR_ANY;
	host_addr.sin_port = htons((USHORT)pstPrivate->nPort);

    // Bind the socket to our local address.
    //
    nError = bind(pstPrivate->hSocket, (LPSOCKADDR)&host_addr,
        sizeof(host_addr));
    DbgOutStr("Socket %d bind returns %d\n", pstPrivate->hSocket, nError, 0,0,0);

	if (nError != 0)
		{
		usRetVal = COM_DEVICE_ERROR;
		goto WSASexit;
		}

    // Tell the socket to notify us of events that we are interested
    // in (like when somebody connects to us).
    //
	nError = WSAAsyncSelect(pstPrivate->hSocket, pstPrivate->hwndEvents,
        WM_WINSOCK_NOTIFY, FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE);

    if (nError != 0)
        {
        usRetVal = COM_DEVICE_ERROR;
        goto WSASexit;
        }

    // Tell the socket to wait for incoming calls.
    //
	if (listen(pstPrivate->hSocket, 1) != 0)
		{
		usRetVal = COM_DEVICE_ERROR;
		goto WSASexit;
		}

WSASexit:
	if (usRetVal != COM_OK)
		{
		closesocket(pstPrivate->hSocket);
		}

	return usRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
USHORT WinSockDisconnectSpecial(ST_STDCOM*pstPrivate,
							LPSTR pszData,
							UINT uiSize)
	{

	if (pstPrivate->hSocket != INVALID_SOCKET)
		{
		closesocket(pstPrivate->hSocket);
		pstPrivate->hSocket = INVALID_SOCKET;
		}

	pstPrivate->fConnected = 0;

	return COM_OK;
	} /*lint !e715 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
LONG WinSockCloseEvent(ST_STDCOM*pstPrivate, LPARAM lPar)
	{
	pstPrivate->fConnected = 0;
	ComNotify(pstPrivate->hCom, CONNECT);

	return 0;
	}  /*lint !e715 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *  lPar			status code from Winsock
 *
 * RETURNS:
 */
LONG WinSockConnectEvent(ST_STDCOM*pstPrivate, LPARAM lPar)
	{
	int status;
	status = (int)HIWORD(lPar);
	if (status)
		{
		pstPrivate->fConnected = 0;
		ComNotify(pstPrivate->hCom, CONNECT);
		}
	else
		{
		pstPrivate->fConnected = 1;
		ComNotify(pstPrivate->hCom, CONNECT);
		}
	return 0;
	}	/*lint !e715 */


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  WinSockAcceptEvent
 *
 * DESCRIPTION:
 *  Accepts an incoming call.
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *  lPar			status code from Winsock
 *
 * RETURNS:
 *
 * AUTHOR:  C. Baumgartner, 11/19/96 (ported from HAWin16)
 */
LONG WinSockAcceptEvent(ST_STDCOM*pstPrivate, LPARAM lPar)
	{
	int    status = HIWORD(lPar);
	SOCKET hAnswer = INVALID_SOCKET;
    SOCKET hOldSocket = INVALID_SOCKET;

    // We aren't connected yet.
    //
	pstPrivate->fConnected = 0;

    // Attempt to accept the call.
    //
	hAnswer = accept(pstPrivate->hSocket, NULL, 0);

	if (hAnswer != INVALID_SOCKET)
		{
        // Now we are connected.
        //
		pstPrivate->fConnected = 1;

        // Get the newly accepted socket.
        //
        hOldSocket = pstPrivate->hSocket;
		pstPrivate->hSocket = hAnswer;

        // Now close the old socket because we don't want to
        // be listening when we are connected.
        //
        closesocket(hOldSocket);
		}

    // Let the rest of the world know that we are connected.
    //
	ComNotify(pstPrivate->hCom, CONNECT);

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *  lPar			status code from Winsock
 *
 * RETURNS:
 *
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)
 */
LONG WinSockReadEvent(ST_STDCOM*pstPrivate, LPARAM lPar)
	{

	ComNotify(pstPrivate->hCom, DATA_RECEIVED);
	return 0;
	}   /*lint !e715 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	WinSockWriteEvent
 *
 * DESCRIPTION:
 *	Called back by Winsock when write completes; updates data structures
 *	to allow another write to take place
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *  lPar			status code from Winsock
 *
 * RETURNS:
 *	
 * AUTHOR
 * 	mcc 01/19/96 (from HAWIN)

 */
LONG WinSockWriteEvent(ST_STDCOM *pstPrivate, LPARAM lPar)
	{
	int nCount;
	int nError;


	DbgOutStr("WinSockWriteEvent called to handle %d bytes ...",
      		pstPrivate->nSendBufrLen,0,0,0,0);
	
	if (pstPrivate->fConnected == 0)
		return 0;

	if (pstPrivate->fSndBreak)
		{
		WinSockSendBreak(pstPrivate);
		pstPrivate->fSndBreak = FALSE;
		}

	if (pstPrivate->nSendBufrLen)
		{
		// This is done to keep the "main thread" from updating the buffer
		//  while we send it.  OK, I think, since send won't block.
	    EnterCriticalSection(&pstPrivate->csect);

		nCount = send(pstPrivate->hSocket,
								pstPrivate->pbSendBufr,
								(int)pstPrivate->nSendBufrLen,
								0);
	    LeaveCriticalSection(&pstPrivate->csect);


		// DbgOutStr("WinSock send returned %d\r\n", nCount, 0,0,0,0);

		// assert((int)pstPrivate->SendBufrLen == nCount);

		if (nCount == SOCKET_ERROR)
			{
			nError = WSAGetLastError();

			if (nError == WSAEWOULDBLOCK)
				{
				DbgOutStr("  still blocked\n", 0,0,0,0,0);
				/*
				 * Nothing to do in this case
				 */
				}
			else
				{
				// Got some weird error.  Notify interested parties
				// that our connection is suspect
				DbgOutStr(" got error %d\r\n", nError, 0,0,0,0);
				pstPrivate->nSendBufrLen = 0;
				pstPrivate->pbSendBufr = 0;
				ComNotify(pstPrivate->hCom, CONNECT);
				}
			}
		else
			{
			DbgOutStr("%d bytes sent.\n", nCount, 0,0,0,0);
	
			if (nCount < (int)pstPrivate->nSendBufrLen)
				{
				pstPrivate->nSendBufrLen -= (USHORT)nCount;
				pstPrivate->pbSendBufr += nCount;
				}
			else
				{
				pstPrivate->nSendBufrLen = 0;
				pstPrivate->pbSendBufr = 0;
				}
			}
		}

	if (pstPrivate->nSendBufrLen == 0)
		{
		pstPrivate->fSending = FALSE;


		//DbgOutStr("Sending WM_COM_SEND_DONE in WinSockWriteEvent\n",0,0,0,0,0);
   		ComNotify(pstPrivate->hCom, SEND_DONE);
		}

	return 0;
	}   /*lint !e715 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	WinSockResolveEvent
 *
 * DESCRIPTION:
 *	Called once WSAAsynchGetHostByName has done its dirty work to actually
 *  generate a connect call
 *
 * PARAMETERS:
 *	pstPrivate		Driver's private data structure
 *  lPar			returned from previous Winsock call
 *
 * RETURNS:
 *	0
 *
 * AUTHOR
 *	mcc 01/18/96 (borrowed from HAWIN)
 */
LONG WinSockResolveEvent(ST_STDCOM *pstPrivate, LPARAM lPar)
	{
	int nError;
	struct sockaddr_in	srv_addr;
	LPHOSTENT			pstHost;
	ULONG				*pulAddress;

	DbgOutStr("WinSockResolveEvent called\n", 0,0,0,0,0);

	nError = HIWORD(lPar);
	if (nError)
		{
		// Notify the connection driver that a change in status may
		// have taken place.  It will follow up and display
		// an appropriate message
		DbgOutStr("Resolve: hiword of lpar = %d\n", nError,0,0,0,0);
		
		ComNotify(pstPrivate->hCom, CONNECT);
		pstPrivate->fConnected = 0;
		
		return 0;
		}

	pstHost = (LPHOSTENT)pstPrivate->pstHostBuf;
	pulAddress = (ULONG FAR *)*(pstHost->h_addr_list);

	nError = WSAAsyncSelect(pstPrivate->hSocket,
							pstPrivate->hwndEvents,
							WM_WINSOCK_NOTIFY,
							FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE
							);

	if (nError != 0)
		{
		DbgOutStr("Resolve: WSAAsyncSelect failed\n", 0,0,0,0,0);
		ComNotify(pstPrivate->hCom, CONNECT);
		return 0;
		}


	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = *pulAddress;
	srv_addr.sin_port = (short) htons(pstPrivate->nPort);

 	if (connect(pstPrivate->hSocket,
							(LPSOCKADDR)&srv_addr,
							sizeof(srv_addr)) == SOCKET_ERROR)
		{
		nError = WSAGetLastError();
		if (nError != WSAEWOULDBLOCK)
			{
			DbgOutStr("Resolve: connect failed, code = %d\n", nError,
					0,0,0,0);
			//DeviceReportError(pstPrivate, (UINT)nError, 0, FALSE);
			ComNotify(pstPrivate->hCom, CONNECT);
			}
		}

	return 0;
	}
	
	
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	WinSockSendMessage
 *
 * DESCRIPTION:
 *	Used to send a Telnet option message. This calls send() directly so
 *	that data can go out while SndBufrSend is reporting COM_BUSY.
 *
 * PARAMETERS:
 *	pstPrivate		Com driver private data
 *	nMsg			The message number , e.g. DO, WILL, WONT, (see comwsock.hh)
 *	nChar			The message data, e.g. TELOPT_BINARY (see comwsock.hh)
 *
 * RETURNS:
 *	void
 *
 * AUTHOR
 *	mcc 02/06/96
 */
VOID WinSockSendMessage(ST_STDCOM * pstPrivate, INT nMsg, INT nChar)
    {
	unsigned char 	acMsg[3];

#if defined(_DEBUG)
	char *nNames[] = {"WILL", "WONT", "DO", "DONT"};
	assert( nMsg >= WILL && nMsg <= DONT );
	DbgOutStr("Send %s: %lx\r\n", nNames[nMsg - WILL], nChar,0,0,0);
#endif
	acMsg[0] = IAC;
	acMsg[1] = (UCHAR) nMsg;
	acMsg[2] = (UCHAR) nChar;
			
	
	WinSockSendBuffer(pstPrivate, 3, acMsg);
	
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	WinSockSendBuffer
 *
 * DESCRIPTION:
 *	Used to send an arbitrary string of data (e.g., a terminal type)
 *  during Telnet option negotiation
 *
 * PARAMETERS:
 *	pstPrivate		Winsock Com driver private data
 *	nSize			Number of bytes to send
 *	pszBuffer		Pointer to data to send
 *
 * RETURNS:
 *	void
 *
 * AUTHOR
 *	mcc 02/06/96
 */
VOID WinSockSendBuffer(ST_STDCOM * pstPrivate, INT nSize, LPSTR pszBuffer)
	{
	int nCount, nError;
	
	nCount = send(pstPrivate->hSocket, pszBuffer, nSize,0);
						
	if (nCount == SOCKET_ERROR)
		{								
		nError = WSAGetLastError();

		if (nError == WSAEWOULDBLOCK)
			{
			DbgOutStr("WSSB would block.  Queueing 3 bytes\n",
				0,0,0,0,0);
			if (sndQueueAppend(pstPrivate,pszBuffer, 3) != COM_OK)
				ComNotify(pstPrivate->hCom, CONNECT);
			}
		else
			ComNotify(pstPrivate->hCom, CONNECT);
		
		}
	else
		{
		int i;
		DbgOutStr("%4d >> ", nCount,0,0,0,0);
    	for (i = 0; i < nCount; i++)
    	DbgOutStr("%x ", pszBuffer[i],0,0,0,0);
		DbgOutStr("\n", 0,0,0,0,0);	
		}	
	}

	

#endif



#ifdef MULTITHREAD
	// This is essentially the NPORT TCPCOM driver ported to
	// Win32.  It uses different threads for reading and
	// writing to the TCP socket.
	// This code is deadwood at the moment, but might prove
	// useful in Upper Wacker if someone can figure out
	// why it does not work reliably under Win95. (send() calls
	// would often appear to work but no data would come out
	// over the socket.)

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortActivate
 *
 * DESCRIPTION:
 *  Called to activate the port and connect to destination
 *
 * ARGUMENTS:
 *  pstPrivate  -- driver data structure
 *  pszPortName -- not used
 *
 * RETURNS:
 *  COM_OK if port is successfully activated
 *  COM_NOT_ENOUGH_MEMORY if there in insufficient memory for data storage
 *  COM_NOT_FOUND if named port cannot be opened
 *  COM_DEVICE_ERROR if API errors are encountered
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckPortActivate(void *pvPrivate,
    TCHAR *pszPortName,
    DWORD_PTR dwMediaHdl)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int             iRetVal = COM_OK;
    ST_COM_CONTROL *pstComCntrl;
    DWORD           dwThreadID;
	WORD wVersion;
	WSADATA stWsaData;
	struct sockaddr_in	cli_addr;



	/*
	 * Initialize the Windows Socket DLL
	 */
	wVersion = 0x0101;			// The version of WinSock that we want
	if (WSAStartup(wVersion, &stWsaData) != 0)
		{
		/* No DLL was available */
		iRetVal = COM_DEVICE_ERROR;
        goto checkout;
		}
	DbgOutStr("Done calling WSAStartup\n", 0,0,0,0,0);

	/* Confirm that the Windows Socket DLL supports 1.1. */
	/* Note that if the DLL supports versions greater    */
	/* than 1.1 in addition to 1.1, it will still return */
	/* 1.1 in wVersion since that is the version we      */
	/* requested                                         */
	if ((LOBYTE(stWsaData.wVersion) != 1) &&
		(HIBYTE(stWsaData.wVersion) != 1))
		{
		/* No acceptable DLL was available */
		iRetVal = COM_DEVICE_ERROR;
        goto checkout;
		}

	/*
	 * Create a socket for later use.
	 */
	pstPrivate->hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (pstPrivate->hSocket == INVALID_SOCKET)
		{
		iRetVal = WSAGetLastError();
		DbgOutStr("Error %d creating socket\n", iRetVal, 0,0,0,0);
		iRetVal = COM_DEVICE_ERROR;
		goto checkout;
		}
	DbgOutStr("Done creating socket %d \n", pstPrivate->hSocket,0,0,0,0);




	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = INADDR_ANY;
	cli_addr.sin_port = 0;

	// Bind the socket to any internet address.  Ignore errors;
	// real ones will be detected later, and this WILL fail
	// if the socket is already bound.
	iRetVal = bind(pstPrivate->hSocket, (LPSOCKADDR)&cli_addr,
			sizeof(cli_addr));
	DbgOutStr("Socket %d bind returns %d\n", pstPrivate->hSocket, iRetVal, 0,0,0);

    // Make sure we can get enough memory for buffers before opening device
    pstPrivate->pbBufrStart = malloc((size_t)pstPrivate->nRBufrSize);

    if (pstPrivate->pbBufrStart == NULL)
        {
        iRetVal = COM_NOT_ENOUGH_MEMORY;
        //* DeviceReportError(pstPrivate, SID_ERR_NOMEM, 0, TRUE);
        goto checkout;
        }

    pstPrivate->pbBufrEnd = pstPrivate->pbBufrStart + pstPrivate->nRBufrSize;
    pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;
    pstPrivate->pbComStart = pstPrivate->pbComEnd = pstPrivate->pbBufrStart;
    pstPrivate->fBufrEmpty = TRUE;


    if (iRetVal == COM_OK)
        {
        pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
        pstComCntrl->puchRBData =
            pstComCntrl->puchRBDataLimit =
            pstPrivate->pbBufrStart;

        pstPrivate->dwEventMask = EV_ERR | EV_RLSD;
        pstPrivate->fNotifyRcv = TRUE;
        pstPrivate->fBufrEmpty = TRUE;


        // Start thread to handle Reading, Writing (& 'rithmetic) & events
        pstPrivate->fHaltThread = FALSE;
        pstPrivate->hComReadThread = CreateThread((LPSECURITY_ATTRIBUTES)0,
                    16384, WsckComReadThread, pstPrivate, 0, &dwThreadID);
		DBG_THREAD("CreateThread (Read Thread)  returned %08X %08X\r\n",
            pstPrivate->hComReadThread,0,0,0,0);

        pstPrivate->hComWriteThread = CreateThread((LPSECURITY_ATTRIBUTES)0,
                    16384, WsckComWriteThread, pstPrivate, 0, &dwThreadID);
        DBG_THREAD("CreateThread  (Write Thread) returned %08X %08X\r\n",
            pstPrivate->hComWriteThread,0,0,0,0);
			
		// TODO discuss with JKH what thread priorities should be

		// Make sure that we have a valid address to connect to
		if ( wsckResolveAddress(pstPrivate->szRemoteAddr, &pstPrivate->ulAddr) != COM_OK )
			{
			pstPrivate->fConnected = 0;
			ComNotify(pstPrivate->hCom, CONNECT);
			iRetVal = COM_NOT_FOUND;
			goto checkout;
			}

		// Connect to the specified host
		pstPrivate->stHost.sin_family = AF_INET;
		pstPrivate->stHost.sin_addr.s_addr = pstPrivate->ulAddr;
		pstPrivate->stHost.sin_port = htons(pstPrivate->nPort);
		//DbgOutStr("About to call connect", 0,0,0,0,0);
		iRetVal = connect(pstPrivate->hSocket,
					  (struct sockaddr *) &pstPrivate->stHost,
					  sizeof(pstPrivate->stHost));
		if ( iRetVal == COM_OK )
			{
			pstPrivate->fConnected = TRUE;
			// Turn loose the read thread
			DbgOutStr("connect OK", 0,0,0,0,0);
			SetEvent(pstPrivate->ahEvent[EVENT_READ]);
			SetEvent(pstPrivate->ahEvent[EVENT_WRITE]);
			}
		else
			{
			iRetVal = WSAGetLastError();
			DbgOutStr(" connect() failed, rc = %d",iRetVal, 0,0,0,0);
			iRetVal = COM_NOT_FOUND;
			pstPrivate->fConnected = 0;
			ComNotify(pstPrivate->hCom, CONNECT);
			}
        }

checkout:
    if (iRetVal != COM_OK)
        WsckPortDeactivate(pstPrivate);

    return iRetVal;
    }  /*lint !e715 */


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckPortDeactivate
 *
 * DESCRIPTION:
 *  Deactivates and closes an open port
 *
 * ARGUMENTS:
 *  pstPrivate -- Driver data structure
 *
 * RETURNS:
 *  COM_OK
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckPortDeactivate(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int iRetVal = COM_OK;



	
	if (pstPrivate->hComReadThread || pstPrivate->hComWriteThread)
        {
        // Halt the thread by setting a flag for the thread to detect and then
        // forcing WaitCommEvent to return by changing the event mask
        DBG_THREAD("DBG_THREAD: Shutting down ComWinsock thread\r\n", 0,0,0,0,0);
        pstPrivate->fHaltThread = TRUE;

        // Read thread should exit now, it's handle will signal when it has exited
        CloseHandle(pstPrivate->hComReadThread);
		WaitForSingleObject(pstPrivate->hComReadThread, 5000);

        pstPrivate->hComReadThread = NULL;
        DBG_THREAD("DBG_THREAD: ComWinsock thread has shut down\r\n", 0,0,0,0,0);

		// Write thread should exit now, it's handle will signal when it has exited
        CloseHandle(pstPrivate->hComWriteThread);
		WaitForSingleObject(pstPrivate->hComWriteThread, 5000);

        pstPrivate->hComWriteThread = NULL;
        DBG_THREAD("DBG_THWrite: ComWriteThread has shut down\r\n", 0,0,0,0,0);
		}

    if (pstPrivate->pbBufrStart)
        {
        free(pstPrivate->pbBufrStart);
        pstPrivate->pbBufrStart = NULL;
        }


	// Shut down socket and WINSOCK
	closesocket(pstPrivate->hSocket);


	WSACleanup();

	pstPrivate->hSocket = INVALID_SOCKET;

    return iRetVal;
    }



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckRcvRefill
 *
 * DESCRIPTION:
 *  Called when the receive buffer is empty to refill it. This routine
 *  should attempt to refill the buffer and return the first character.
 *  It is important that this function be implemented efficiently.
 *
 * ARGUMENTS:
 *  pstPrivate -- the driver data structure
 *
 * RETURNS:
 *  TRUE if data is put in the receive buffer
 *  FALSE if there is no new incoming data
 *
 * AUTHOR:
 * mcc 12/26/95	(taken almost entirely from comstd.c)
 */
int WINAPI WsckRcvRefill(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int fRetVal = FALSE;
    ST_COM_CONTROL *pstComCntrl;

    EnterCriticalSection(&pstPrivate->csect);

    pstPrivate->pbComStart = (pstPrivate->pbComEnd == pstPrivate->pbBufrEnd) ?
        pstPrivate->pbBufrStart : pstPrivate->pbComEnd;
    pstPrivate->pbComEnd = (pstPrivate->pbReadEnd >= pstPrivate->pbComStart) ?
        pstPrivate->pbReadEnd : pstPrivate->pbBufrEnd;
    DBG_READ("DBG_READ: Refill ComStart==%x, ComEnd==%x (ReadEnd==%x)\r\n",
        pstPrivate->pbComStart, pstPrivate->pbComEnd,
        pstPrivate->pbReadEnd, 0,0);
    if (pstPrivate->fBufrFull)
        {
        DBG_READ("DBG_READ: Refill Signalling EVENT_READ\r\n", 0,0,0,0,0);
        SetEvent(pstPrivate->ahEvent[EVENT_READ]);
        }
    if (pstPrivate->pbComStart == pstPrivate->pbComEnd)
        {
        DBG_READ("DBG_READ: Refill setting fBufrEmpty = TRUE\r\n", 0,0,0,0,0);
        pstPrivate->fBufrEmpty = TRUE;
        ComNotify(pstPrivate->hCom, NODATA);
        }
    else
        {
        pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
        pstComCntrl->puchRBData = pstPrivate->pbComStart;
        pstComCntrl->puchRBDataLimit = pstPrivate->pbComEnd;
        fRetVal = TRUE;
        }

    LeaveCriticalSection(&pstPrivate->csect);
    return fRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckRcvClear
 *
 * DESCRIPTION:
 *  Clears the receiver of all received data.
 *
 * ARGUMENTS:
 *  hCom -- a comm handle returned by an earlier call to ComCreateHandle
 *
 * RETURNS:
 *  COM_OK if data is cleared
 *  COM_DEVICE_ERROR if Windows com device driver returns an error
 *
 * AUTHOR:
 * mcc 12/26/95	(taken almost entirely from comstd.c)
 */
int WINAPI WsckRcvClear(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int iRetVal = COM_OK;
    ST_COM_CONTROL *pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;

    EnterCriticalSection(&pstPrivate->csect);

    // Set buffer pointers to clear out any data we might have queued
    pstComCntrl->puchRBData = pstComCntrl->puchRBDataLimit =
        pstPrivate->pbBufrStart;
    pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;
    pstPrivate->pbComStart = pstPrivate->pbComEnd = pstPrivate->pbBufrStart;

    LeaveCriticalSection(&pstPrivate->csect);
    return iRetVal;
    }



//          Buffered send routines


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckSndBufrSend
 *
 * DESCRIPTION:
 *	Sends a buffer over the socket
 *
 * ARGUMENTS:
 *	pstPrivate		Driver's private data structure
 *  pvBufr			Pointer to data to send
 *	nSize			Number of bytes to send
 *
 * RETURNS:
 *	COM_OK			if successful
 *  COM_FAILED		otherwise
 *
 *
 * AUTHOR:
 * mcc 12/26/95	
 */
int WINAPI WsckSndBufrSend(void *pvPrivate, void *pvBufr, int  nSize)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int  iRetVal = COM_OK;
	int  iCode;

    assert(pvBufr != (void *)0);
    assert(nSize <= WSOCK_SIZE_OUTQ);

	if (pstPrivate->fSending)
		{
		DbgOutStr("SBS: Busy", 0,0,0,0,0);
		return COM_BUSY;
		}
    else if (nSize > 0)
        {
        ComNotify(pstPrivate->hCom, SEND_STARTED);
        EnterCriticalSection(&pstPrivate->csect);
		pstPrivate->pbSendBufr = pvBufr;
		pstPrivate->nSendBufrLen = nSize;
		pstPrivate->fSending = TRUE;


		LeaveCriticalSection(&pstPrivate->csect);
		
		// Tell the write thread to run
		iCode = SetEvent(pstPrivate->ahEvent[EVENT_WRITE]);
		DbgOutStr("SBS: %d bytes in buffer. SetEvent returns %d", nSize,
				iCode,0,0,0);

        }

    return iRetVal;
    }





/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckSndBufrClear
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int WINAPI WsckSndBufrClear(void *pvPrivate)
    {
	ST_STDCOM *pstPrivate = (ST_STDCOM *) pvPrivate;
    int iRetVal = COM_OK;

    EnterCriticalSection(&pstPrivate->csect);
    if (WsckSndBufrIsBusy(pstPrivate))
        {
		pstPrivate->fClearSendBufr = TRUE;
        }
    LeaveCriticalSection(&pstPrivate->csect);

    return iRetVal;
    }



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckComWriteThread
 *
 * DESCRIPTION:
 *  One of the "main" threads of the comm driver ...
 *  Waits on an "anything to write?" semaphore
 *  When awakened, writes the write buffer in the driver data structure
 *  to the comm port and resets the semaphore
 *
 * ARGUMENTS
 *  pvData		Address of driver data structure
 *
 *
 * RETURNS:
 *  Nothing
 *
 * AUTHOR
 *	mcc 12/27/95 (stolen from Northport)
 */
DWORD  WINAPI WsckComWriteThread(void *pvData)
	{
	ST_STDCOM		*pstPrivate =  (ST_STDCOM *)pvData;
	int				nBytesWritten;
	unsigned		uSize, nBytesToSend, nBytesSent;
	int				fRunning = TRUE;
	DWORD			iResult = COM_OK;
	char			*pchData;
	int				iCode;



	DBG_THREAD("DBG_THREAD: ComWriteThread starting",0,0,0,0,0);

	// Initialize the "Something to write" semaphore to Reset, so that
	// we will wait for SndBufrSend to hand something to us
	if (! ResetEvent(pstPrivate->ahEvent[EVENT_WRITE]))
		{
		assert(0);
		}

	while (fRunning)
		{
		// Wait on a semaphore for something to write
		iCode = WaitForSingleObject(pstPrivate->ahEvent[EVENT_WRITE],
			(unsigned long) 60000);

		DBG_WRITE("WrThread: Got EVENT_WRITE %d\n", iCode,0,0,0,0);

		// Has anybody told us to shut down?
		//
		if (pstPrivate->fHaltThread)
			{
			DBG_WRITE("  WrThread: fHaltThread==TRUE, shutting down", 0,0,0,0,0);
			ExitThread(0);
			}

		else
			{
			iResult = COM_OK;
			EnterCriticalSection(&pstPrivate->csect);
			pchData = pstPrivate->pbSendBufr;
			nBytesToSend = (unsigned) pstPrivate->nSendBufrLen;

			nBytesSent = 0;
			if (nBytesToSend > 0)
				{
				DbgOutStr("WriteThrd: %d to send\n", nBytesToSend, 0,0,0,0);

				// Loop until we send all the requested data
				while ( fRunning && nBytesSent < nBytesToSend )
					{
					uSize = nBytesToSend - nBytesSent;
					LeaveCriticalSection(&pstPrivate->csect);
					assert(uSize > 0 && uSize < 32767);
					nBytesWritten = send(pstPrivate->hSocket,
						pchData,(int) uSize, 0);
					DbgOutStr("WriteThrd: %d bytes of %d sent. 1st 3 = %x %x %x\n",
							nBytesWritten, uSize, pchData[0], pchData[1], pchData[2]);

					// We have an error -- probably the connection got dropped
					// report it to the various interested parties
					if ( nBytesWritten == -1 )
						{
						iResult = (unsigned) WSAGetLastError();
						DbgOutStr("WriteThrd: error %d sending %d bytes (%d - %d). Byebye.\n",
							iResult, (int) uSize, nBytesToSend,nBytesSent,0);
						ComNotify(pstPrivate->hCom, CONNECT);
						pstPrivate->fConnected = 0;
						fRunning = 0;
						}
					nBytesSent += (unsigned) nBytesWritten;
					if (nBytesSent < nBytesToSend  )
						{
						DbgOutStr("WrtThrd: can't send all data to socket\n",
							 0,0,0,0,0);
						pchData += nBytesWritten;
						}
					}
				EnterCriticalSection(&pstPrivate->csect);
				//DbgOutStr("  WrThread: Wrote %u bytes, %lu written, ret=%lu\n",
				//uSize, nBytesWritten, 0, 0, 0);

				// We've sent the buffer, so clear the Sending flag
				pstPrivate->fSending = FALSE;

				pstPrivate->nSendBufrLen = 0;
				pstPrivate->pbSendBufr = NULL;
				pstPrivate->fClearSendBufr = FALSE;
				
				//DBG_WRITE("  WrThread: posting EVENT_SENT", 0,0,0,0,0);
				// TOCO:mcc 12/29/95 SetEvent(pstPrivate->ahEvent[EVENT_SENT]);
				}
			if (pstPrivate->fHaltThread)
				{
				DBG_WRITE("  WrThread: fHaltThread==TRUE, shutting down", 0,0,0,0,0);
				ExitThread(0);
				}
			DbgOutStr("  WrThread: setting fSending=FALSE, resetting EVENT_WRITE\n",
				0,0,0,0,0);
			if (!ResetEvent(pstPrivate->ahEvent[EVENT_WRITE]))
				{
				assert(0);
				}
			LeaveCriticalSection(&pstPrivate->csect);
			ComNotify(pstPrivate->hCom, SEND_DONE);
			}
		}

	DbgOutStr("WriteThread exiting ...", 0,0,0,0,0);
	ExitThread(0);
	return (iResult);
	}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: WsckComReadThread
 *
 * DESCRIPTION:
 *  One of the main threads of the comm driver ...
 *  Reads data from the comm port as long as we have a place to put it
 *  If the buffer fills up, go to sleep until RcvRefill takes some data
 *  out of the buffer
 *
 *
 * ARGUMENTS:
 *  pvData		 address of our private data structure
 *
 * RETURNS:
 *  nothing
 *
 * AUTHOR
 *	mcc 12/27/95 (stolen from Northport)
 */
DWORD  WINAPI WsckComReadThread(void *pvData)
	{
	ST_STDCOM			*pstPrivate =  (ST_STDCOM *)pvData;
	int					fRunning = TRUE;
	int					fReading = TRUE;
	char				*pbReadFrom, *pOut;
	unsigned			nReadSize;
	long				lBytesRead, nFFs;
	int					iResult;
	int					nIndx;
	DWORD				rc;


	DBG_THREAD("DBG_THREAD: ComtcpReadThread starting",0,0,0,0,0);
	EnterCriticalSection(&pstPrivate->csect);

	// Set Read event to reset so we don't read until the connection is up
	// By setting fBufrFull TRUE, the thread will think it is being
	// reawakened from a full buffer condition when PortActivate posts the
	// semaphore
	pstPrivate->fBufrFull = TRUE;
	ResetEvent(pstPrivate->ahEvent[EVENT_READ]);
	LeaveCriticalSection(&pstPrivate->csect);

	pstPrivate->fSeenFF = 0;
	while ( fRunning )
		{

		// Wait for a wakeup call if we put ourself to sleep
		//DbgOutStr("ReadThread: Waiting for EVENT_READ", 0,0,0,0,0);
		rc = WaitForSingleObject(pstPrivate->ahEvent[EVENT_READ], 60000);
		if ( rc != 0 )
			{
			DbgOutStr("ReadThread: EVENT_READ timed out. fBufFull=%d",
				pstPrivate->fBufrFull,0,0,0,0);
			}
		else
			{
			//DbgOutStr("ReadThread: Got EVENT_READ.", 0,0,0,0,0);
			}

		// To get this thread to exit, the deactivate routine forces a
		// fake com event by posting EVENT_READ
		if (pstPrivate->fHaltThread)
			{
			DBG_THREAD("DBG_THREAD: Comtcp exiting thread",0,0,0,0,0);
			fRunning = FALSE;
			}
		else
			{
			EnterCriticalSection(&pstPrivate->csect);
			if (pstPrivate->fBufrFull)
				{
				//DbgOutStr("ReadThread: fBufrFull = FALSE", 0,0,0,0,0);
				pstPrivate->fBufrFull = FALSE;
				fReading = TRUE;
				}
			LeaveCriticalSection(&pstPrivate->csect);

			// Do reads until we fill the buffer
			while (fReading && fRunning)
				{
				// Check for wrap around in circular buffer
                pbReadFrom = (pstPrivate->pbReadEnd >= pstPrivate->pbBufrEnd) ?
                    pstPrivate->pbBufrStart : pstPrivate->pbReadEnd;

                nReadSize = (unsigned) (pbReadFrom < pstPrivate->pbComStart) ?
                    (unsigned) (pstPrivate->pbComStart - pbReadFrom - 1) :
                    (unsigned) (pstPrivate->pbBufrEnd - pbReadFrom);

                if (nReadSize > WSOCK_MAX_READSIZE)
                    nReadSize = WSOCK_MAX_READSIZE;

                if (!nReadSize)
                    {
                    DBG_READ("Read Thread -- fBufrFull = TRUE, unsignalling EVENT_READ\r\n",
                        0,0,0,0,0);
                    pstPrivate->fBufrFull = TRUE;
                    ResetEvent(pstPrivate->ahEvent[EVENT_READ]);
                    break;
                    }
                else
                    {
					DBG_READ("ReadThread posting a recv\n", 0,0,0,0,0);
					lBytesRead = recv(pstPrivate->hSocket, pbReadFrom,
													  (int)nReadSize, 0);
                    if (lBytesRead > 0)
                        {
                        pstPrivate->pbReadEnd += lBytesRead;

                        if (pstPrivate->pbReadEnd >= pstPrivate->pbBufrEnd)
                            pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;

                        DBG_READ("DBG_READ: Thread -- recv completed synchronously,"
                            " lBytesRead==%ld, ReadEnd==%x\r\n",
                            lBytesRead, pstPrivate->pbReadEnd,0,0,0);

                        if (pstPrivate->fBufrEmpty)
                            {
                            DBG_READ("DBG_READ: Thread -- fBufrEmpty = FALSE\r\n", 0,0,0,0,0);
                            pstPrivate->fBufrEmpty = FALSE;
                            ComNotify(pstPrivate->hCom, DATA_RECEIVED);
                            }

						if (pstPrivate->fEscapeFF)
							{
							// The sender escaped FF characters by doubling
							// them.  Copy the received data buffer onto itself,
							// skipping every other FF
							pOut = pbReadFrom;
							nFFs = 0;
							for (nIndx = 0; nIndx < lBytesRead; nIndx += 1)
								{
								if (pstPrivate->fSeenFF)
									{
									if (pbReadFrom[nIndx] == 0xFF)
										{
										// Skip this one
										nFFs++;
										}
									else
										{
										// This should not happen, but copy
										// anyway
										*pOut = pbReadFrom[nIndx];
										pOut++;
										}
									pstPrivate->fSeenFF = FALSE;
									}
								else
									{
									// Test to see if this is an FF; copy
									// input to output.
									if (pbReadFrom[nIndx] == 0xFF)
										{
										pstPrivate->fSeenFF = TRUE;
										}
									*pOut = pbReadFrom[nIndx];
									pOut++;
									}
								}
							// Decrement the number of bytes read by the
							// number of duplicate FF's we tossed.
							lBytesRead -= nFFs;
							}

						// Notify application that we got some data
						// if buffer had been empty
						EnterCriticalSection(&pstPrivate->csect);
						if (pstPrivate->fBufrEmpty)
							{
							DBG_READ("DBG_READ: Thread -- fBufrEmpty = FALSE", 0,0,0,0,0);
							pstPrivate->fBufrEmpty = FALSE;
							}
						LeaveCriticalSection(&pstPrivate->csect);
						ComNotify(pstPrivate->hCom, DATA_RECEIVED);

						}
					else
						{

						// 0 value from recv indicates that the connection is closed;
						// -1 indicates another error.  Notify CNCT driver.
						// (re-set the connection status
						// if the error code indicates that the connection is down)
						iResult = WSAGetLastError();
						DbgOutStr("ReadThread: Got no data, err=%d", iResult, 0,0,0,0);
						EnterCriticalSection(&pstPrivate->csect);
						if ( lBytesRead == 0 ||
							iResult == WSAENETDOWN ||
							iResult == WSAENOTCONN ||
							iResult == WSAEHOSTDOWN ||
							iResult == WSAETIMEDOUT)
							{
							// Wait until we are told to shut down; don't try to read
							// anymore!
							ResetEvent(pstPrivate->ahEvent[EVENT_READ]);
							pstPrivate->fConnected = 0;
							}

						LeaveCriticalSection(&pstPrivate->csect);
						ComNotify(pstPrivate->hCom, DATA_RECEIVED);

						}

					}


				if (pstPrivate->fHaltThread)
					{
					DBG_THREAD("DBG_THREAD: Comtcp exiting thread",0,0,0,0,0);
					fRunning = FALSE;
					}
				}  // end of fReading && fRunning
			}

		}  // end of fRunning loop

	EnterCriticalSection(&pstPrivate->csect);
	pstPrivate->hComReadThread = 0;
	LeaveCriticalSection(&pstPrivate->csect);
	DbgOutStr("ReadThread exiting ...", 0,0,0,0,0);
	ExitThread(0);

	return(0);
	}
	
	
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	wsckResolveAddress
 *
 * DESCRIPTION:
 *	Takes a numeric or symbolic net address string and returns
 *  a valid binary internet address.
 *
 * ARGUMENTS:
 *  pszRemote		The remote system's address as a string
 *  pulAddr			Pointer to binary internet address
 *
 * RETURNS:
 *  COM_OK			If successful
 *	COM_NOT_FOUND	If the address is invalid
 *
 * AUTHOR
 *	mcc 01/09/96 (stolen from Northport)
 */
int wsckResolveAddress(TCHAR *pszRemote, unsigned long *pulAddr)
	{
	int				iRetVal = COM_NOT_FOUND;
	struct hostent  *pstHost;

	assert(pszRemote);
	assert(pulAddr);
	
	if (pszRemote && pulAddr)
		{

		// Convert pszRemote to an internet address.  If not successful,
		// assume that the string is a host NAME and try to turn
		// that into an address
		*pulAddr = inet_addr(pszRemote);
		if ((*pulAddr == INADDR_NONE) || (*pulAddr == 0))
			{
			// If not a valid network address, it should be a name, so
			// look that up (in the hosts file or via a name server)
			pstHost = gethostbyname(pszRemote);
			if ( pstHost)
				*pulAddr = *((unsigned long *)pstHost->h_addr);
			}
		if ((*pulAddr != INADDR_NONE) && (*pulAddr != 0))
			iRetVal = COM_OK;
		else
			iRetVal = COM_NOT_FOUND;
		}

	return iRetVal;
	}


#endif // MULTITHREAD
#endif // INCL_WINSOCK
