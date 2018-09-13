/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    getname.c

Abstract:

    This module contains name retrieval routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPGetPeerName()
        WSPGetSockName()

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
WSPGetPeerName(
    IN SOCKET s,
    OUT struct sockaddr FAR * name,
    IN OUT LPINT namelen,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine supplies the name of the peer connected to the socket s and
    stores it in the struct sockaddr referenced by name. It may be used only
    on a connected socket. For datagram sockets, only the name of a peer
    specified in a previous WSPConnect() call will be returned - any name
    specified by a previous WSPSendTo() call will not be returned by
    WSPGetPeerName().

    On return, the namelen argument contains the actual size of the name
    returned in bytes.

Arguments:

    s - A descriptor identifying a connected socket.

    name - A pointer to the structure which is to receive the name of the
        peer.

    namelen - A pointer to an integer which, on input, indicates the size
        of the structure pointed to by name, and on output indicates the
        size of the returned name.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPGetPeerName() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPGetPeerName", (PVOID)s, name, namelen, lpErrno );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetPeerName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(GETNAME) {

            SOCK_PRINT((
                "WSPGetPeerName failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPGetPeerName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->getpeername(
                 socketInfo->WS1Handle,
                 name,
                 namelen
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

    SOCK_EXIT( "WSPGetPeerName", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPGetPeerName



INT
WSPAPI
WSPGetSockName(
    IN SOCKET s,
    OUT struct sockaddr FAR * name,
    IN OUT LPINT namelen,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine retrieves the current name for the specified socket descriptor
    in name. It is used on a bound and/or connected socket specified by the s
    parameter. The local association is returned. This call is especially
    useful when a WSPConnect() call has been made without doing a WSPBind()
    first; as this call provides the only means by which the local association
    that has been set by the service provider can be determined.

    If a socket was bound to an unspecified address (e.g., ADDR_ANY),
    indicating that any of the host's addresses within the specified address
    family should be used for the socket, WSPGetSockName() will not necessarily
    return information about the host address, unless the socket has been
    connected with WSPConnect() or WSPAccept. The WinSock SPI client must not
    assume that an address will be specified unless the socket is connected.
    This is because for a multi-homed host the address that will be used for
    the socket is unknown until the socket is connected.

Arguments:

    s - A descriptor identifying a bound socket.

    name - A pointer to a structure used to supply the address (name) of
        the socket.

    namelen - A pointer to an integer which, on input, indicates the size
        of the structure pointed to by name, and on output indicates the
        size of the returned name.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPGetSockName() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPGetSockName", (PVOID)s, name, namelen, lpErrno );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetSockName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(GETNAME) {

            SOCK_PRINT((
                "WSPGetSockName failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPGetSockName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->getsockname(
                 socketInfo->WS1Handle,
                 name,
                 namelen
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

    SOCK_EXIT( "WSPGetSockName", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPGetSockName

