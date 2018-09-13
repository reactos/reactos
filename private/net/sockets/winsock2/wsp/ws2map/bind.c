/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    bind.c

Abstract:

    This module contains bind routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPBind()

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
WSPBind(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on an unconnected connectionless or connection-
    oriented socket, before subsequent WSPConnect()s or WSPListen()s. When
    a socket is created with WSPSocket(), it exists in a name space (address
    family), but it has no name or local address assigned. WSPBind()
    establishes the local association of the socket by assigning a local
    name to an unnamed socket.

    As an example, in the Internet address family, a name consists of three
    parts: the address family, a host address, and a port number which
    identifies the WinSock SPI client. In WinSock 2, the name parameter is
    not strictly interpreted as a pointer to a "sockaddr" struct. Service
    providers are free to regard it as a pointer to a block of memory of size
    namelen. The first two bytes in this block (corresponding to "sa_family"
    in the "sockaddr" declaration) must contain the address family that was
    used to create the socket. Otherwise the error WSAEFAULT shall be
    indicated.

    If a WinSock 2 SPI client does not care what local address is assigned to
    it, it will specify the manifest constant value ADDR_ANY for the sa_data
    field of the name parameter. This instructs the service provider to use
    any appropriate network address. For TCP/IP, if the port is specified
    as 0, the service provider will assign a unique port to the WinSock SPI
    client with a value between 1024 and 5000. The SPI client may use
    WSPGetSockName() after WSPBind() to learn the address and the port that
    has been assigned to it, but note that if the Internet address is equal
    to INADDR_ANY, WSPGetSockOpt() will not necessarily be able to supply the
    address until the socket is connected, since several addresses may be
    valid if the host is multi-homed.

Arguments:

    s - A descriptor identifying an unbound socket.

    name - The address to assign to the socket. The sockaddr structure is
        defined as follows:

            struct sockaddr {
                u_short sa_family;
                char sa_data[14];
            };

        Except for the sa_family field, sockaddr contents are expressed in
        network byte order.

    namelen - The length of the name.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPBind() returns 0. Otherwise, it returns
        SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPBind", (PVOID)s, (PVOID)name, (PVOID)namelen, lpErrno );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPBind", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(BIND) {

            SOCK_PRINT((
                "WSPBind failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPBind", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->bind(
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

    SOCK_EXIT( "WSPBind", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPBind

