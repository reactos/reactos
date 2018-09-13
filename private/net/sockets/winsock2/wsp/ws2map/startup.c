/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    startup.c

Abstract:

    This module contains the startup code for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPStartup()
        WSPCleanup()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Our version number.
//

#define SPI_VERSION MAKEWORD( 2, 2 )


//
// Public functions.
//


INT
WSPAPI
WSPStartup(
    IN WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN WSPUPCALLTABLE UpcallTable,
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

    SOCK_ENTER( "WSPStartup", (PVOID)wVersionRequested, lpWSPData, (PVOID)&UpcallTable, NULL );

    SOCK_ASSERT( lpWSPData != NULL );

    err = SockEnterApi( FALSE, TRUE );

    if( err != NO_ERROR ) {

        SOCK_EXIT( "WSPStartup", err, &err );
        return err;

    }

    //
    // Check the version number.
    //

    if ( SockWspStartupCount == 0 &&
             wVersionRequested != SPI_VERSION ) {

        err = WSAVERNOTSUPPORTED;
        SOCK_EXIT( "WSPStartup", WSAVERNOTSUPPORTED, &err );
        return err;

    }

    //
    // Remember that the app has called WSPStartup.
    //

    InterlockedIncrement( &SockWspStartupCount );

    //
    // Fill in the WSPData structure.
    //

    lpWSPData->wVersion = SPI_VERSION;
    lpWSPData->wHighVersion = SPI_VERSION;

    wcscpy(
        lpWSPData->szDescription,
        L"Microsoft Windows Sockets 2 to 1.1 Mapper."
        );

    //
    // Save the upcall table.
    //

    SockUpcallTable = UpcallTable;

    //
    // Return our proc table to the DLL.
    //

    *lpProcTable = SockProcTable;

    //
    // Cleanup & exit.
    //

    SockTerminating = FALSE;

    SOCK_EXIT( "WSPStartup", NO_ERROR, NULL );
    return NO_ERROR;

}   // WSPStartup



INT
WSPAPI
WSPCleanup(
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    The WinSock 2 SPI client is required to perform a successful WSPStartup()
    call before it can use WinSock service providers. When it has completed
    the use of WinSock service providers, the SPI client will call
    WSPCleanup() to deregister itself from a WinSock service provider and
    allow the service provider to free any resources allocated on behalf of
    the WinSock 2 client. It is permissible for SPI clients to make more than
    one WSPStartup() call. For each WSPStartup() call a corresponding
    WSPCleanup() call will also be issued. Only the final WSPCleanup() for
    the service provider does the actual cleanup; the preceding calls simply
    decrement an internal reference count in the WinSock service provider.

    When the internal reference count reaches zero and actual cleanup
    operations commence, any pending blocking or asynchronous calls issued by
    any thread in this process are canceled without posting any notification
    messages or signaling any event objects. Any pending overlapped send and
    receive operations (WSPSend()/WSPSendTo()/WSPRecv()/WSPRecvFrom() with an
    overlapped socket) issued by any thread in this process are also canceled
    without setting the event object or invoking the completion routine, if
    specified. In this case, the pending overlapped operations fail with the
    error status WSA_OPERATION_ABORTED. Any sockets open when WSPCleanup() is
    called are reset and automatically deallocated as if WSPClosesocket() was
    called; sockets which have been closed with WSPCloseSocket() but which
    still have pending data to be sent are not affected--the pending data is
    still sent.

    This routine should not return until the service provider DLL is
    prepared to be unloaded from memory. In particular, any data remaining
    to be transmitted must either already have been sent or be queued for
    transmission by portions of the transport stack that will not be unloaded
    from memory along with the service provider's DLL.

    A WinSock service provider must be prepared to deal with a process which
    terminates without invoking WSPCleanup() - for example, as a result of an
    error. A WinSock service provider must ensure that WSPCleanup() leaves
    things in a state in which the WinSock 2 DLL can immediately invoke
    WSPStartup() to re-establish WinSock usage.

Arguments:

    lpErrno - A pointer to the error code.

Return Value:

    The return value is 0 if the operation has been successfully initiated.
        Otherwise the value SOCKET_ERROR is returned, and a specific error
        number is available in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    LINGER lingerInfo;
    PLIST_ENTRY listEntry;
    LONG startupCount;
    INT err;
    BOOL globalLockHeld;

    SOCK_ENTER( "WSPCleanup", NULL, NULL, NULL, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPCleanup", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Decrement the reference count.
    //

    startupCount = InterlockedDecrement( &SockWspStartupCount );

    //
    // If the count of calls to WSPStartup() is not 0, we shouldn't do
    // cleanup yet.  Just return.
    //

    if( startupCount != 0 ) {

        IF_DEBUG(MISC) {

            SOCK_PRINT(( "Leaving WSPCleanup().\n" ));

        }

        SOCK_EXIT( "WSPCleanup", NO_ERROR, NULL );
        return NO_ERROR;

    }

    //
    // Indicate that the DLL is no longer initialized.  This will
    // result in all open sockets being abortively disconnected.
    //

    SockTerminating = TRUE;;

    //
    // Close each open socket.  We loop looking for open sockets until
    // all sockets are either off the list of in the closing state.
    //

    SockAcquireGlobalLock();
    globalLockHeld = TRUE;

    try {

        for( listEntry = SockGlobalSocketListHead.Flink ;
             listEntry != &SockGlobalSocketListHead ; ) {

            SOCKET socketHandle;
            int errTmp;

            socketInfo = CONTAINING_RECORD(
                             listEntry,
                             SOCKET_INFORMATION,
                             SocketListEntry
                             );

            //
            // If this socket is about to close, go on to the next socket.
            //

            if( socketInfo->State == SocketStateClosing ) {

                listEntry = listEntry->Flink;
                continue;

            }

            //
            // Pull the handle into a local in case another thread closes
            // this socket just as we are trying to close it.
            //

            socketHandle = socketInfo->WS2Handle;

            //
            // Release the global lock so that we don't cause a deadlock
            // from out-of-order lock acquisitions.
            //

            SockReleaseGlobalLock();
            globalLockHeld = FALSE;

            //
            // Set each socket to linger for 0 seconds.  This will cause
            // the connection to reset, if appropriate, when we close the
            // socket. If the setsockopt() fails, press on regardless.
            //

            lingerInfo.l_onoff = 1;
            lingerInfo.l_linger = 0;

            WSPSetSockOpt(
                socketHandle,
                SOL_SOCKET,
                SO_LINGER,
                (char *)&lingerInfo,
                sizeof(lingerInfo),
                &errTmp
                );

            //
            // Perform the actual close of the socket.
            //

            WSPCloseSocket( socketHandle, &errTmp );

            SockAcquireGlobalLock();
            globalLockHeld = TRUE;

            //
            // Restart the search from the beginning of the list.  We cannot
            // use listEntry->Flink because the socket that is pointed to by
            // listEntry may have been freed.
            //

            listEntry = SockGlobalSocketListHead.Flink;

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    if( globalLockHeld ) {

        SockReleaseGlobalLock();

    }

    //
    // Free hooker info structures.
    //

    SockFreeAllHookers();

    IF_DEBUG(MISC) {

        SOCK_PRINT(( "Leaving WSPCleanup().\n" ));

    }

    SOCK_EXIT( "WSPCleanup", NO_ERROR, NULL );
    return NO_ERROR;

}   // WSPCleanup

