/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    listen.c

Abstract:

    This module contains support for the listen( ) WinSock API.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "winsockp.h"

// Moved to afd
//#define MINIMUM_BACKLOG     1
//#define MAXIMUM_BACKLOG_NTS 200
//#define MAXIMUM_BACKLOG_NTW 5


int
WSPAPI
WSPListen(
    IN SOCKET Handle,
    IN int Backlog,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    To accept connections, a socket is first created with socket(), a
    backlog for incoming connections is specified with listen(), and
    then the connections are accepted with accept().  listen() applies
    only to sockets that support connections, i.e.  those of type
    SOCK_STREAM.  The socket s is put into "passive'' mode where
    incoming connections are acknowledged and queued pending acceptance
    by the process.

    This function is typically used by servers that could have more than
    one connection request at a time: if a connection request arrives
    with the queue full, the client will receive an error with an
    indication of WSAECONNREFUSED.

    listen() attempts to continue to function rationally when there are
    no available descriptors.  It will accept connections until the
    queue is emptied.  If descriptors become available, a later call to
    listen() or accept() will re-fill the queue to the current or most
    recent "backlog'', if possible, and resume listening for incoming
    connections.

Arguments:

    s - A descriptor identifying a bound, unconnected socket.

    backlog - The maximum length to which the queue of pending
        connections may grow.

Return Value:

    If no error occurs, listen() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    IO_STATUS_BLOCK ioStatusBlock;
    PSOCKET_INFORMATION socket;
    int err;
#ifdef _AFD_SAN_SWITCH_
    AFD_SAN_LISTEN_INFO afdListenInfo;
#else //_AFD_SAN_SWITCH_
    AFD_LISTEN_INFO afdListenInfo;
#endif //_AFD_SAN_SWITCH_

    WS_ENTER( "WSPListen", (PVOID)Handle, (PVOID)Backlog, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPListen", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up local variables so we know how to clean up on exit.
    //

    socket = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        err = WSAENOTSOCK;
        goto exit;

    }

    //
    // Acquire the lock that protects sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we begin the listening
    // process.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Listen is not a legal operation on datagram sockets.
    //

    if ( IS_DGRAM_SOCK(socket) ) {

        err = WSAEOPNOTSUPP;
        goto exit;

    }

    //
    // To do a listen on a socket, the socket must be bound to an
    // address but not already connected (except if this is a C_Root
    // socket.  The only valid states for this call is bound and listening.  
    // Note that if the socket is already listening, we just no-op this call.  
    // Some unixes refill the connection backlog on a subsequent listen, 
    // but we don't do that.
    //

    if ( socket->Listening ) {

        err = NO_ERROR;
        goto exit;

    } 
    else if (!(socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)) {

        if ( SockIsSocketConnected( socket ) || socket->AsyncConnectContext) {

            err = WSAEISCONN;
            goto exit;
        }
        else if ( socket->State != SocketStateBound ) {

            err = WSAEINVAL;
            goto exit;
        }

    }
    else if (socket->State != SocketStateConnected ) {
        err = WSAEINVAL;
        goto exit;
    }

    //
    // Make sure that the backlog argument is within the legal range.
    // If it is out of range, just set it to the closest in-range
    // value--this duplicates BSD 4.3 behavior.  Note that NT Workstation
    // is tuned to have a lower backlog limit in order to conserve
    // resources on that product type.
    //

    //
    // Moved this code to afd
    //
/*
    if ( Backlog < MINIMUM_BACKLOG ) {

        Backlog = MINIMUM_BACKLOG;

    }

    if ( SockProductTypeWorkstation && Backlog > MAXIMUM_BACKLOG_NTW ) {

        Backlog = MAXIMUM_BACKLOG_NTW;

    }

    if ( !SockProductTypeWorkstation && Backlog > MAXIMUM_BACKLOG_NTS ) {

        Backlog = MAXIMUM_BACKLOG_NTS;

    }
*/
    //
    // Tell AFD to start listening on this endpoint.
    //

    afdListenInfo.MaximumConnectionQueue = Backlog;
    afdListenInfo.UseDelayedAcceptance = socket->ConditionalAccept;

#ifdef _AFD_SAN_SWITCH_
retrywithsan:
    afdListenInfo.SanActive = SockSanEnabled;
#endif //_AFD_SAN_SWITCH_
    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                    // APC Routine
                 NULL,                    // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_START_LISTEN,
                 &afdListenInfo,
                 sizeof(afdListenInfo),
                 NULL,
                 0L
                 );

    if ( status == STATUS_PENDING ) {

        SockReleaseSocketLock( socket );
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socket );
        status = ioStatusBlock.Status;

    }

    if ( !NT_SUCCESS(status) ) {
#ifdef _AFD_SAN_SWITCH_
        if (status==STATUS_INVALID_PARAMETER_12) {
            WS_ASSERT (afdListenInfo.SanActive==FALSE);
            status = SockSanActivate ();
            if (NT_SUCCESS (status))
                goto retrywithsan;
        }
#endif //_AFD_SAN_SWITCH_

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // Notify the helper DLL that the socket is now listening for
    // connections.
    //

    err = SockNotifyHelperDll( socket, WSH_NOTIFY_LISTEN );

    if ( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Indicate the listening state of this socket.
    //

    socket->Listening = TRUE;

    //
    // Remember the changed state of this socket.
    //

    err = SockSetHandleContext( socket );

    if ( err != NO_ERROR ) {

        goto exit;

    }

#ifdef _AFD_SAN_SWITCH_
	//
	// Once TCP socket is in listening state, try SAN
	//
    if (SockSanEnabled && socket->CatalogEntryId==SockTcpProviderInfo.dwCatalogEntryId) {
		INT err1;

		if (err1 = SockSanListen(socket, Backlog)) {

			WS_PRINT(( "WSPListen: SockSanListen failed, err=%d\n", err1));

			//
			// Ignore any error since WSPListen thru TCP has succeeded
			//

		}
	}
#endif // _AFD_SAN_SWITCH_

exit:

    IF_DEBUG(LISTEN) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPListen on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPListen on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPListen", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPListen", NO_ERROR, FALSE );
    return NO_ERROR;

} // listen
