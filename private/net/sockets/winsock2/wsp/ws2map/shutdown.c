/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module contains stubbed-out unimplemented SPI routines for the
    Winsock 2 to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPShutdown()

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
WSPShutdown(
    IN SOCKET s,
    IN int how,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on all types of sockets to disable reception,
    transmission, or both.

    If how is SD_RECEIVE, subsequent receives on the socket will be
    disallowed. This has no effect on the lower protocol layers. For TCP
    sockets, if there is still data queued on the socket waiting to be
    received, or data arrives subsequently, the connection is reset, since the
    data cannot be delivered to the user. For UDP sockets, incoming datagrams
    are accepted and queued. In no case will an ICMP error packet
    be generated.

    If how is SD_SEND, subsequent sends on the socket are disallowed. For TCP
    sockets, a FIN will be sent. Setting how to SD_BOTH disables both sends
    and receives as described above.

    Note that WSPShutdown() does not close the socket, and resources attached
    to the socket will not be freed until WSPCloseSocket() is invoked.

    WSPShutdown() does not block regardless of the SO_LINGER setting on the
    socket. A WinSock SPI client should not rely on being able to re-use a
    socket after it has been shut down. In particular, a WinSock service
    provider is not required to support the use of WSPConnect() on such a
    socket.

Arguments:

    s - A descriptor identifying a socket.

    how - A flag that describes what types of operation will no longer be
        allowed.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPShutdown() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPShutdown", (PVOID)s, (PVOID)how, lpErrno, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPShutdown", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SHUTDOWN) {

            SOCK_PRINT((
                "WSPShutdown failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPShutdown", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->shutdown(
                 socketInfo->WS1Handle,
                 how
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

    SOCK_EXIT( "WSPShutdown", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPShutdown

