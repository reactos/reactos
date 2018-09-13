/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spi.c

Abstract:

    This module contains the necessary "glue" for plugging into the
    WinSock 2.0 Service Provider Interface.

Author:

    Keith Moore (keithmo)        6-Oct-1995

Revision History:

--*/

#include "winsockp.h"


//
// Our version number.
//

#define SPI_VERSION MAKEWORD( 2, 2 )


int
WSPAPI
WSPStartup(
    IN  WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable
    )

/*++

Routine Description:

    This routine MUST be the first WinSock SPI function called by a WinSock
    SPI client on a per-process basis. It allows the client to specify the
    version of WinSock SPI required and to provide its upcall dispatch table.
    All upcalls, i.e., functions prefixed with WPU, made by the WinSock
    service provider are invoked via the client's upcall dispatch table.
    This routine also allows the client to retrieve details of the specific
    WinSock service provider implementation. The WinSock SPI client may only
    issue further WinSock SPI functions after a successful WSPStartup()
    invocation. A table of pointers to the rest of the SPI functions is
    retrieved via the lpProcTable parameter.

    In order to support future versions of the WinSock SPI and the WinSock 2
    DLL which may have functionality differences from the current WinSock SPI,
    a negotiation takes place in WSPStartup(). The caller of WSPStartup()
    (either the WinSock 2 DLL or a layered protocol) and the WinSock service
    provider indicate to each other the highest version that they can support,
    and each confirms that the other's highest version is acceptable. Upon
    entry to WSPStartup(), the WinSock service provider examines the version
    requested by the client. If this version is equal to or higher than the
    lowest version supported by the service provider, the call succeeds and
    the service provider returns in wHighVersion the highest version it
    supports and in wVersion the minimum of its high version and
    wVersionRequested. The WinSock service provider then assumes that the
    WinSock SPI client will use wVersion. If the wVersion field of the WSPDATA
    structure is unacceptable to the caller, it should call WSPCleanup() and
    either search for another WinSock service provider or fail to initialize.

    This negotiation allows both a WinSock service provider and a WinSock SPI
    client to support a range of WinSock versions. A client can successfully
    utilize a WinSock service provider if there is any overlap in the version
    ranges. The following chart gives examples of how WSPStartup() works in
    conjunction with different WinSock DLL and WinSock service provider (SP)
    versions:

          DLL         SP        wVersion-   wVersion    wHigh-       End
        Version     Version     Requested               Version     Result
        ~~~~~~~     ~~~~~~~     ~~~~~~~~~   ~~~~~~~~    ~~~~~~~     ~~~~~~
        1.1         1.1         1.1         1.1         1.1         use 1.1
        1.0 1.1     1.0         1.1         1.0         1.0         use 1.0
        1.0         1.0 1.1     1.0         1.0         1.1         use 1.0
        1.1         1.0 1.1     1.1         1.1         1.1         use 1.1
        1.1         1.0         1.1         1.0         1.0         DLL fails
        1.0         1.1         1.0         ---         ---         WSAVERNOTSUPPORTED
        1.0 1.1     1.0 1.1     1.1         1.1         1.1         use 1.1
        1.1 2.0     1.1         2.0         1.1         1.1         use 1.1
        2.0         2.0         2.0         2.0         2.0         use 2.0

    The following code fragment demonstrates how a WinSock SPI client which
    supports only version 2.0 of WinSock SPI makes a WSPStartup() call:

        WORD wVersionRequested;
        WSPDATA WSPData;

        int err;

        WSPUPCALLTABLE upcallTable =
        {
            // initialize upcallTable with function pointers
        };

        LPWSPPROC_TABLE lpProcTable =
        {
            // allocate memory for the ProcTable
        };

        wVersionRequested = MAKEWORD( 2, 0 );

        err = WSPStartup( wVersionRequested, &WSPData,
        lpProtocolBuffer, upcallTable, lpProcTable );
        if ( err != 0 ) {
            // Tell the user that we couldn't find a useable
            // WinSock service provider.
            return;
        }

        // Confirm that the WinSock service provider supports 2.0.
        // Note that if the service provider supports versions
        // greater than 2.0 in addition to 2.0, it will still
        // return 2.0 in wVersion since that is the version we
        // requested.

        if ( LOBYTE( WSPData.wVersion ) != 2 ||
             HIBYTE( WSPData.wVersion ) != 0 ) {
            // Tell the user that we couldn't find a useable
            // WinSock service provider.
            WSPCleanup( );
            return;
        }

        // The WinSock service provider is acceptable. Proceed.

    And this code fragment demonstrates how a WinSock service provider which
    supports only version 2.0 performs the WSPStartup() negotiation:

        // Make sure that the version requested is >= 2.0.
        // The low byte is the major version and the high
        // byte is the minor version.

        if ( LOBYTE( wVersionRequested ) < 2) {
            return WSAVERNOTSUPPORTED;
        }

        // Since we only support 2.0, set both wVersion and
        // wHighVersion to 2.0.

        lpWSPData->wVersion = MAKEWORD( 2, 0 );
        lpWSPData->wHighVersion = MAKEWORD( 2, 0 );

    Once the WinSock SPI client has made a successful WSPStartup() call, it
    may proceed to make other WinSock SPI calls as needed. When it has
    finished using the services of the WinSock service provider, the client
    must call WSPCleanup() in order to allow the WinSock service provider to
    free any resources allocated for the client.

    Details of how WinSock service provider information is encoded in the
    WSPData structure is as follows:

        typedef struct WSPData {
            WORD            wVersion;
            WORD            wHighVersion;
            char            szDescription[WSPDESCRIPTION_LEN+1];
        } WSPDATA, FAR * LPWSPDATA;

    The members of this structure are:

        wVersion- The version of the WinSock SPI specification that the
            WinSock service provider expects the caller to use.

        wHighVersion - The highest version of the WinSock SPI specification
            that this service provider can support (also encoded as above).
            Normally this will be the same as wVersion.

        szDescription - A null-terminated ASCII string into which the
            WinSock provider copies a description of itself. The text
            (up to 256 characters in length) may contain any characters
            except control and formatting characters: the most likely use
            that a SPI client will put this to is to display it (possibly
            truncated) in a status message.

    A WinSock SPI client may call WSPStartup() more than once if it needs to
    obtain the WSPData structure information more than once. On each such
    call the client may specify any version number supported by the provider.

    There must be one WSPCleanup() call corresponding to every successful
    WSPStartup() call to allow third-party DLLs to make use of a WinSock
    provider. This means, for example, that if WSPStartup() is called three
    times, the corresponding call to WSPCleanup() must occur three times.
    The first two calls to WSPCleanup() do nothing except decrement an
    internal counter; the final WSPCleanup() call does all necessary resource
    deallocation.

Arguments:

    wVersionRequested - The highest version of WinSock SPI support that the
        caller can use. The high order byte specifies the minor version
        (revision) number; the low-order byte specifies the major version
        number.

    lpWSPData - A pointer to the WSPDATA data structure that is to receive
        details of the WinSock service provider.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFOW struct that defines the
        characteristics of the desired protocol. This is especially useful
        when a single provider DLL is capable of instantiating multiple
        different service providers..

    UpcallTable - The WinSock 2 DLL's upcall dispatch table.

    lpProcTable - A pointer to the table of SPI function pointers.

Return Value:

    WSPStartup() returns zero if successful. Otherwise it returns an error
        code.

--*/

{

    int err;

    WS_ENTER( "WSPStartup", (PVOID)wVersionRequested, lpWSPData, (PVOID)&UpcallTable, NULL );

    WS_ASSERT( lpWSPData != NULL );

//    err = SockEnterApi( FALSE, TRUE, FALSE );
	//	Can't use the call above because it expects startup to
	//  be called. There is no reason to clutter it for this
	//	single special case.

	if (SockProcessTerminating) {
		err = WSANOTINITIALISED;
        WS_EXIT( "WSPStartup", WSANOTINITIALISED, TRUE );
		return err;
	}
	else if ((GET_THREAD_DATA () == NULL)
			&& 	!SockThreadInitialize()) {
		err = WSAENOBUFS;
        WS_EXIT( "WSPStartup", WSAENOBUFS, TRUE );
		return err;
	}
	else
		err = NO_ERROR;

    //
    // Check the version number.
    //

    if (wVersionRequested != SPI_VERSION ) {

		err = WSAVERNOTSUPPORTED;
        WS_EXIT( "WSPStartup", WSAVERNOTSUPPORTED, TRUE );
        return err;

    }

	//
	// Take global lock. We shouldn't be called with
	// WSPStartup that often (only once per WSAPROTOCOL_INFO chain)
	// as ws2_32.dll shields plain WSAStartup calls from us.
	//
	SockAcquireRwLockExclusive (&SocketGlobalLock);


    WS_ASSERT (SockWspStartupCount >= 0);

	if (SockWspStartupCount==0) {
		//
		// Increment our load count, so that we are not unloaded
		// while there are outstanding APC's.
		// We decrement load count in WSPCleanup if there are
		// no outstanding APC which count we maintain.
		//
		TCHAR szPath[MAX_PATH];
		HINSTANCE hThisDll;

		GetModuleFileName( SockModuleHandle, szPath, sizeof( szPath ) );
		hThisDll = LoadLibrary( szPath );
		if (hThisDll==NULL) {
			//
			// Things are really bad.
			//
			WS_ASSERT (FALSE);
	        SockReleaseRwLockExclusive (&SocketGlobalLock);

			err = WSASYSCALLFAILURE;
			WS_EXIT( "WSPStartup", WSASYSCALLFAILURE, TRUE );
			return err;
		}
		//
		// We don't preserve new handle because as far
		// as we know it should be the same as the one passed to us
		// at DLL initialization.
		//
		WS_ASSERT (hThisDll==SockModuleHandle);

        //
        // Save the upcall table.
        //
        // !!! SPECBUGBUG: The upcall table should be passed by reference,
        // *not* by value!
        //

        SockUpcallTableHack = UpcallTable;
        SockUpcallTable = &SockUpcallTableHack;
	}


    //
    // Remember that the ws2_32 has called WSPStartup.
    //

    SockWspStartupCount += 1;

    //
    // Fill in the WSPData structure.
    //

    lpWSPData->wVersion = SPI_VERSION;
    lpWSPData->wHighVersion = SPI_VERSION;
    wcscpy(
        lpWSPData->szDescription,
        L"Microsoft Windows Sockets Version 2."
        );


    //
    // Return our proc table to the DLL.
    //

    *lpProcTable = SockProcTable;
#ifdef _AFD_SAN_SWITCH_
    if (SockProductTypeWorkstation==FALSE &&
		    //
		    // ws2_32.dll loads providers based solely on GUID. So, don't
		    // check anything other than GUID. This means SAN will get "enabled"
		    // even if app opens only UDP sockets. This just means one extra
		    // critical section being allocated (not too expensive) and only
		    // if sanOnTcpKey is present, so not too bad.
		    // This has one minor disadvantage: if SAN is installed and first
		    // socket is something like IPX, then first listen/connect call
		    // will fail with STATUS_INVALID_PARAMETER_12, at which point
		    // SanActivate will be called which will set SockSanEnabled=TRUE,
		    // and then AFD will get called again (one extra kernel roundtrip).
		    // This will happen only for the FIRST listen/connect and only
		    // if some SAN provider is installed.
		    //
            //lpProtocolInfo->iAddressFamily==SockTcpProviderInfo.iAddressFamily &&
            //lpProtocolInfo->iSocketType==SockTcpProviderInfo.iSocketType &&
            //lpProtocolInfo->iProtocol==SockTcpProviderInfo.iProtocol &&
            IsEqualGUID (&lpProtocolInfo->ProviderId, &SockTcpProviderInfo.ProviderId)) {
        NTSTATUS                status;
        RTL_OSVERSIONINFOEXW    versionInfo;

        versionInfo.dwOSVersionInfoSize = sizeof (versionInfo);
        status = RtlGetVersion ((LPOSVERSIONINFOW)&versionInfo);
        if (NT_SUCCESS (status)) {
#ifndef DONT_CHECK_FOR_DTC
            if (versionInfo.wSuiteMask & VER_SUITE_DATACENTER)
#endif //DONT_CHECK_FOR_DTC
            {
                //
                // Get TCP/IP provider's catalog entry id (initializes
				// SockTcpProviderInfo.dwCatalogEntryId)
                //
                SockSanGetTcpipCatalogId();

				if (!SockSanEnabled) {
					status = SockSanInitialize ();
				}
            }
        }
    }
#endif //_AFD_SAN_SWITCH_

	SockReleaseRwLockExclusive (&SocketGlobalLock);

    //
    // Cleanup & exit.
    //

    WS_EXIT( "WSPStartup", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPStartup

BOOL  
WINAPI
DeleteSockets(
    LPVOID              EnumCtx,
    LPWSHANDLE_CONTEXT  HContext
    ) {
    LINGER                  lingerInfo;
    PSOCKET_INFORMATION     socket;
    int err;

    socket = CONTAINING_RECORD (HContext, SOCKET_INFORMATION, HContext);
    SockAcquireSocketLockExclusive( socket );
    //
    // Make sure this socket is not about to be closed.
    //

    if ( socket->State != SocketStateClosing ) {
        //
        // Release the lock to avoid a deadlock
        // Note that because handle table writer lock is held
        // while we are in the context of enumeration callback,
        // no other thread can remove this socket from the table
        // much less destroy it or its handle.
        //
        SockReleaseSocketLock( socket );

        //
        // Set each socket to linger for 0 seconds.  This will cause
        // the connection to reset, if appropriate, when we close the
        // socket.
        //

        lingerInfo.l_onoff = 1;
        lingerInfo.l_linger = 0;

        WSPSetSockOpt(
            socket->Handle,
            SOL_SOCKET,
            SO_LINGER,
            (char *)&lingerInfo,
            sizeof(lingerInfo),
            &err
            );

        //
        // Perform the actual close of the socket.
        // Note that this thread will be able to recursively acquire
        // lock to the handle table and remove the socket form the
        // table
        //

        WSPCloseSocket( socket->Handle, &err );
    }
    else {
        SockReleaseSocketLock( socket );
    }

    return TRUE;
}



int
WSPAPI
WSPCleanup (
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    An application is required to perform a (successful) WSPStartup()
    call before it can use Windows Sockets services.  When it has
    completed the use of Windows Sockets, the application may call
    WSPCleanup() to deregister itself from a Windows Sockets
    implementation.

Arguments:

    None.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise
    the value SOCKET_ERROR is returned, and a specific error is available
    in lpErrno.

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;
#ifdef _AFD_SAN_SWITCH_
	BOOLEAN sockSanWasEnabled = FALSE;
#endif // _AFD_SAN_SWITCH_

    WS_ENTER( "WSPCleanup", NULL, NULL, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPCleanup", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

	//
	// Take global lock. We shouldn't be called with
	// WSPCleanup that often (only at when ws2_32.dll
	// reference count goes to 0) as ws2_32.dll shields 
	// plain WSACleanup calls from us.
	//
	SockAcquireRwLockExclusive (&SocketGlobalLock);

    //
    // If the count of calls to WSPStartup() is not 0, we shouldn't do
    // cleanup yet.  Just return.
    //

    if ( SockWspStartupCount == 1 ) {


		//
		// Indicate that the DLL is no longer initialized.  This will
		// result in all open sockets being abortively disconnected.
		//

		// SockTerminating = TRUE;;


		WahEnumerateHandleContexts (SockContextTable, DeleteSockets, NULL);
    
		//
		// Free cached information about helper DLLs.
        // It is now ref counted by each socket.
		//
		SockFreeHelperDlls( );

#ifdef _AFD_SAN_SWITCH_
        if (SockSanEnabled) {
			sockSanWasEnabled = TRUE;
            SockSanCleanup ( );
        }
		else {
			sockSanWasEnabled = FALSE;
		}
#endif // _AFD_SAN_SWITCH_
        //
        // Free array of protocol info structures.
        //

        if (SockProtocolInfoArray!=NULL) {
            FREE_HEAP (SockProtocolInfoArray);
            SockProtocolInfoArray = NULL;
        }

		//
		// Kill the async thread if it was started.
		//

		SockTerminateAsyncThread( );

		//
		// If there are any pending APC's then increment do an alertable sleep 
		// to give them some time to complete. If they all complete then decrement
		// the DLL load count (which we incremented at startup) and return,
		// otherwise leave the load count as is so this dll won't get
		// unloaded, preventing the potential of an AV when an APC fires.
		//

		if ( SockProcessPendingAPCCount != 0 ) {

			OutputDebugStringA(
				"MSAFD: Pending APCs in cleanup! Waiting...\n"
				);

			while ( SleepEx( 1000, TRUE ) != 0 &&
					SockProcessPendingAPCCount != 0 );

			if ( SockProcessPendingAPCCount == 0 ) {

				OutputDebugStringA(
					"MSAFD: (cleanup) ... all APCs fired, lowering DLL ref count\n"
					);

				FreeLibrary( SockModuleHandle );

			} else  {

				OutputDebugStringA(
					"MSAFD: (cleanup) ... No APCs fired, keeping DLL ref count\n"
					);
			}
		}
	}
	else {
		WS_ASSERT (SockWspStartupCount>1);
	}

	SockWspStartupCount -= 1;

	SockReleaseRwLockExclusive (&SocketGlobalLock);

#ifdef _AFD_SAN_SWITCH_
	if (sockSanWasEnabled) {
		//
		// After leaving SocketGlobalLock, see if we need to wait for
		// any socket migration/duplication to complete
		//
		SockSanWaitForSockDupToComplete ( );
	}
#endif // _AFD_SAN_SWITCH_

    IF_DEBUG(MISC) {

        WS_PRINT(( "Leaving WSPCleanup().\n" ));

    }

    WS_EXIT( "WSPCleanup", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPCleanup

