/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    listen.c

Abstract:

    This module contains listen routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPListen()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


INT
WSPAPI
WSPListen(
    IN SOCKET s,
    IN int backlog,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    To accept connections, a socket is first created with WSPSocket(), a
    backlog for incoming connections is specified with WSPListen(), and then
    the connections are accepted with WSPAccept. WSPListen() applies only to
    sockets that are connection-oriented (e.g.,  SOCK_STREAM). The socket s is
    put into "passive'' mode where incoming connection requests are
    acknowledged and queued pending acceptance by the WinSock SPI client.

    This routine is typically used by servers that could have more than one
    connection request at a time: if a connection request arrives with the
    queue full, the client will receive an error with an indication of
    WSAECONNREFUSED.

    WSPListen() should continue to function rationally when there are no
    available descriptors. It should accept connections until the queue is
    emptied. If descriptors become available, a later call to WSPListen() or
    WSPAccept() will re-fill the queue to the current or most recent "backlog",
    if possible, and resume listening for incoming connections.

    A WinSock SPI client may call WSPListen() more than once on the same
    socket. This has the effect of updating the current backlog for the
    listening socket. Should there be more pending connections than the new
    backlog value, the excess pending connections will be reset and dropped.

    Backlog is limited (silently) to a reasonable value as determined by the
    service provider. Illegal values are replaced by the nearest legal value.

Arguments:

    s - A descriptor identifying a bound, unconnected socket.

    backlog - The maximum length to which the queue of pending connections
        may grow. If this value is SOMAXCONN, then the service provider
        should set the backlog to a maximum "reasonable" value.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPListen() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPListen", (PVOID)s, (PVOID)backlog, lpErrno, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPListen", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(LISTEN) {

            SOCK_PRINT((
                "WSPListen failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPListen", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->listen(
                 socketInfo->WS1Handle,
                 backlog
                 );

    if( result == SOCKET_ERROR ) {

        err = socketInfo->Hooker->WSAGetLastError();
        SOCK_ASSERT( err != NO_ERROR );
        SockPostApiCallout();
        goto exit;

    }

    SockPostApiCallout();

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );
    SOCK_ASSERT( result != SOCKET_ERROR );

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPListen", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPListen

