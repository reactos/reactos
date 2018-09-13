/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains connect routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPConnect()

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
WSPConnect(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen,
    IN LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used to create a connection to the specified destination,
    and to perform a number of other ancillary operations that occur at
    connect time as well. If the socket, s, is unbound, unique values are
    assigned to the local association by the system, and the socket is marked
    as bound.

    For connection-oriented sockets (e.g., type SOCK_STREAM), an active
    connection is initiated to the specified host using name (an address in
    the name space of the socket; for a detailed description, please see
    WSPBind()). When this call completes successfully, the socket is ready to
    send/receive data. If the address field of the name structure is all
    zeroes, WSPConnect() will return the error WSAEADDRNOTAVAIL. Any attempt
    to re-connect an active connection will fail with the error code
    WSAEISCONN.

    For a connectionless socket (e.g., type SOCK_DGRAM), the operation
    performed by WSPConnect() is to establish a default destination address
    so that the socket may be used with subsequent connection-oriented send
    and receive operations (WSPSend(),WSPRecv()). Any datagrams received from
    an address other than the destination address specified will be discarded.
    If the address field of the name structure is all zeroes, the socket will
    be "dis-connected"  - the default remote address will be indeterminate,
    so WSPSend() and WSPRecv() calls will return the error code WSAENOTCONN,
    although WSPSendTo() and WSPRecvFrom() may still be used. The default
    destination may be changed by simply calling WSPConnect() again, even if
    the socket is already "connected". Any datagrams queued for receipt are
    discarded if name is different from the previous WSPConnect().

    For connectionless sockets, name may indicate any valid address, including
    a broadcast address. However, to connect to a broadcast address, a socket
    must have WSPSetSockOpt() SO_BROADCAST enabled, otherwise WSPConnect()
    will fail with the error code WSAEACCES.

    On connectionless sockets, exchange of user to user data is not possible
    and the corresponding parameters will be silently ignored.

    The WinSock SPI client is responsible for allocating any memory space
    pointed to directly or indirectly by any of the parameters it specifies.

    The lpCallerData is a value parameter which contains any user data that
    is to be sent along with the connection request. If lpCallerData is NULL,
    no user data will be passed to the peer. The lpCalleeData is a result
    parameter which will reference any user data passed back from the peer as
    part of the connection establishment. lpCalleeData->len initially
    contains the length of the buffer allocated by the WinSock SPI client
    and pointed to by lpCalleeData->buf. lpCalleeData->len will be set to 0
    if no user data has been passed back. The lpCalleeData information will
    be valid when the connection operation is complete. For blocking sockets,
    this will be when the WSPConnect() function returns. For non-blocking
    sockets, this will be after the FD_CONNECT notification has occurred. If
    lpCalleeData is NULL, no user data will be passed back. The exact format
    of the user data is specific to the address family to which the socket
    belongs and/or the applications involved.

    At connect time, a WinSock SPI client may use the lpSQOS and/or lpGQOS
    parameters to override any previous QOS specification made for the socket
    via WSPIoctl() with either the SIO_SET_QOS or SIO_SET_GROUP_QOS opcodes.

    lpSQOS specifies the flow specs for socket s, one for each direction,
    followed by any additional provider-specific parameters. If either the
    associated transport provider in general or the specific type of socket
    in particular cannot honor the QOS request, an error will be returned as
    indicated below. The sending or receiving flow spec values will be ignored,
    respectively, for any unidirectional sockets. If no provider-specific
    parameters are supplied, the buf and len fields of lpSQOS->ProviderSpecific
    should be set to NULL and 0, respectively. A NULL value for lpSQOS
    indicates no application supplied QOS.

    lpGQOS specifies the flow specs for the socket group (if applicable), one
    for each direction, followed by any additional provider-specific
    parameters. If no provider- specific parameters are supplied, the buf and
    len fields of lpSQOS->ProviderSpecific should be set to NULL and 0,
    respectively. A NULL value for lpGQOS indicates no application-supplied
    group QOS. This parameter will be ignored if s is not the creator of the
    socket group.

    When connected sockets break (i.e. become closed for whatever reason),
    they should be discarded and recreated. It is safest to assume that when
    things go awry for any reason on a connected socket, the WinSock SPI
    client must discard and recreate the needed sockets in order to return
    to a stable point.

Arguments:

    s - A descriptor identifying an unconnected socket.

    name- The name of the peer to which the socket is to be connected.

    namelen - The length of the name.

    lpCallerData - A pointer to the user data that is to be transferred
        to the peer during connection establishment.

    lpCalleeData - A pointer to a buffer into which may be copied any user
        data received from the peer during connection establishment.

    lpSQOS - A pointer to the flow specs for socket s, one for each
        direction.

    lpGQOS - A pointer to the flow specs for the socket group (if
        applicable).

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPConnect() returns 0. Otherwise, it returns
        SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPConnect", (PVOID)s, (PVOID)name, (PVOID)namelen, lpCallerData );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPConnect", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(CONNECT) {

            SOCK_PRINT((
                "WSPConnect failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPConnect", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Filter out any options we don't yet support.
    //

    if( lpCallerData != NULL ||
        lpCalleeData != NULL ||
        lpSQOS != NULL ||
        lpGQOS != NULL
        ) {

        err = WSAENOPROTOOPT;
        goto exit;

    }

    //
    // Determine if this is address is OK for the constrained group.
    // Note that we skip the validation check if either a) no address
    // was passed to routine or b) the address length is too small.
    // This is OK, as the hooker will (presumably) fail the request
    // when we make the "real" connect() call.
    //

    if( name != NULL &&
        namelen >= socketInfo->MinSockaddrLength &&
        socketInfo->GroupType == GroupTypeConstrained &&
        !SockValidateConstrainedGroup(
            socketInfo->GroupID,
            socketInfo->GroupType,
            (SOCKADDR *)name,
            namelen
            ) ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // Let the hooker do its thang.
    //

    SockPrepareForBlockingHook( socketInfo );
    SockPreApiCallout();

    result = socketInfo->Hooker->connect(
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

    SOCK_EXIT( "WSPConnect", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPConnect

