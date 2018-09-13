/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    accept.c

Abstract:

    This module contains accept routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPAccept()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


SOCKET
WSPAPI
WSPAccept(
    IN SOCKET s,
    OUT struct sockaddr FAR * addr,
    IN OUT LPINT addrlen,
    IN LPCONDITIONPROC lpfnCondition,
    IN ULONG_PTR dwCallbackData,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine extracts the first connection on the queue of pending
    connections on s, and checks it against the condition function, provided
    the condition function is specified (i.e., not NULL). The condition
    function must be executed in the same thread as this routine is. If the
    condition function returns CF_ACCEPT, this routine creates a new socket
    and performs any socket grouping as indicated by the result parameter g
    in the condition function . Newly created sockets have the same
    properties as s including network events registered with WSPAsyncSelect()
    or with WSPEventSelect(), but not including the listening socket's group
    ID, if any.

    If the condition function returns CF_REJECT, this routine rejects the
    connection request. If the client's accept/reject decision cannot be made
    immediately, the condition function will return CF_DEFER to indicate that
    no decision has been made, and no action about this connection request is
    to be taken by the service provider. When the client is ready to take
    action on the connection request, it will invoke WSPAccept() again and
    return either CF_ACCEPT or CF_REJECT as a return value from the condition
    function.

    For sockets which are in the (default) blocking mode, if no pending
    connections are present on the queue, WSPAccept() blocks the caller until
    a connection is present. For sockets in a non-blocking mode, if this
    function is called when no pending connections are present on the queue,
    WSPAccept() returns the error code WSAEWOULDBLOCK as described below. The
    accepted socket may not be used to accept more connections. The original
    socket remains open.

    The argument addr is a result parameter that is filled in with the address
    of the connecting entity, as known to the service provider. The exact
    format of the addr parameter is determined by the address family in which
    the communication is occurring. The addrlen is a value-result parameter;
    it will initially contain the amount of space pointed to by addr. On
    return, it must contain the actual length (in bytes) of the address
    returned by the service provider. This call is used with connection-
    oriented socket types such as SOCK_STREAM. If addr and/or addrlen are
    equal to NULL, then no information about the remote address of the
    accepted socket is returned. Otherwise, these two parameters shall be
    filled in regardless of whether the condition function is specified or
    what it returns.

    The prototype of the condition function is as follows:

        int
        CALLBACK
        ConditionFunc(
            IN LPWSABUF lpCallerId,
            IN LPWSABUF lpCallerData,
            IN OUT LPQOS lpSQOS,
            IN OUT LPQOS lpGQOS,
            IN LPWSABUF lpCalleeId,
            IN LPWSABUF lpCalleeData,
            OUT GROUP FAR * g,
            IN DWORD dwCallbackData
            );

        The lpCallerId and lpCallerData are value parameters which must
        contain the address of the connecting entity and any user data
        that was sent along with the connection request, respectively.
        If no caller ID or caller data is available, the corresponding
        parameter will be NULL.

        lpSQOS references the flow specs for socket s specified by the
        caller, one for each direction, followed by any additional
        provider-specific parameters. The sending or receiving flow spec
        values will be ignored as appropriate for any unidirectional
        sockets. A NULL value for lpSQOS indicates no caller supplied
        QOS. QOS information may be returned if a QOS negotiation is to
        occur.

        lpGQOS references the flow specs for the socket group the caller
        is to create, one for each direction, followed by any additional
        provider-specific parameters. A NULL value for lpGQOS indicates
        no caller-supplied group QOS. QOS information may be returned if
        a QOS negotiation is to occur.

        The lpCalleeId is a value parameter which contains the local
        address of the connected entity. The lpCalleeData is a result
        parameter used by the condition function to supply user data back
        to the connecting entity. The storage for this data must be
        provided by the service provider. lpCalleeData->len initially
        contains the length of the buffer allocated by the service
        provider and pointed to by lpCalleeData->buf. A value of zero
        means passing user data back to the caller is not supported. The
        condition function will copy up to lpCalleeData->len  bytes of
        data into lpCalleeData->buf , and then update lpCalleeData->len
        to indicate the actual number of bytes transferred. If no user
        data is to be passed back to the caller, the condition function
        will set lpCalleeData->len to zero. The format of all address and
        user data is specific to the address family to which the socket
        belongs.

        The result parameter g is assigned within the condition function
        to indicate the following actions:

            if &g is an existing socket group ID, add s to this
            group, provided all the requirements set by this group
            are met; or

            if &g = SG_UNCONSTRAINED_GROUP, create an unconstrained
            socket group and have s as the first member; or

            if &g = SG_CONSTRAINED_GROUP, create a constrained
            socket group and have s as the first member; or

            if &g = zero, no group operation is performed.

        Any set of sockets grouped together must be implemented by a
        single service provider. For unconstrained groups, any set of
        sockets may be grouped together. A constrained socket group may
        consist only of connection-oriented sockets, and requires that
        connections on all grouped sockets be to the same address on the
        same host. For newly created socket groups, the new group ID
        must be available for the WinSock SPI client to retrieve by
        calling WSPGetSockOpt() with option SO_GROUP_ID. A socket group
        and its associated ID remain valid until the last socket
        belonging to this socket group is closed. Socket group IDs are
        unique across all processes for a given service provider.

        dwCallbackData is supplied to the condition function exactly as
        supplied by the caller of WSPAccept().

Arguments:

    s - A descriptor identifying a socket which is listening for connections
        after a WSPListen().

    addr - An optional pointer to a buffer which receives the address of the
        connecting entity, as known to the service provider. The exact
        format of the addr argument is determined by the address family
        established when the socket was created.

    addrlen - An optional pointer to an integer which contains the length of
        the address addr.

    lpfnCondition - The procedure instance address of an optional, WinSock 2
        client- supplied condition function which will make an accept/reject
        decision based on the caller information passed in as parameters,
        and optionally create and/or join a socket group by assigning an
        appropriate value to the result parameter, g, of this routine.

    dwCallbackData - Callback data to be passed back to the WinSock 2 client
        as a condition function parameter. This parameter is not interpreted
        by the service provider.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPAccept() returns a value of type SOCKET which is
        a descriptor for the accepted socket. Otherwise, a value of
        INVALID_SOCKET is returned, and a specific error code is available
        in lpErrno. Valid error codes are:

--*/

{

    PSOCKET_INFORMATION socketInfo;
    PSOCKET_INFORMATION socketInfo2;
    SOCKET ws1Handle;
    SOCKET ws2Handle;
    INT err;
    GROUP newGroup;
    SOCK_GROUP_TYPE newGroupType;

    SOCK_ENTER( "WSPAccept", (PVOID)s, (PVOID)addr, (PVOID)addrlen, lpfnCondition );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPAccept", INVALID_SOCKET, lpErrno );
        return INVALID_SOCKET;

    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    socketInfo = NULL;
    socketInfo2 = NULL;
    ws1Handle = INVALID_SOCKET;
    ws2Handle = INVALID_SOCKET;
    newGroup = 0;

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(ACCEPT) {

            SOCK_PRINT((
                "WSPAccept failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPAccept", INVALID_SOCKET, lpErrno );
        return INVALID_SOCKET;

    }

    //
    // If there is no deferred accept pending on this socket, call the
    // hooker to get a new connection and store the result (socket and
    // address) in the socket info to prepare for possible deferred
    // accept.
    //

    ws1Handle = socketInfo->DeferSocket;

    if( ws1Handle == INVALID_SOCKET ) {

        INT addressLength;

        if( addrlen == NULL ) {

            addressLength = socketInfo->MaxSockaddrLength;

        } else {

            addressLength = min( socketInfo->MaxSockaddrLength, *addrlen );

        }

        //
        // Let the hooker do its thang.
        //

        SockPrepareForBlockingHook( socketInfo );
        SockPreApiCallout();

        ws1Handle = socketInfo->Hooker->accept(
                        socketInfo->WS1Handle,
                        (SOCKADDR *)( socketInfo + 1 ),
                        &addressLength
                        );

        if( ws1Handle == INVALID_SOCKET ) {

            err = socketInfo->Hooker->WSAGetLastError();
            SOCK_ASSERT( err != NO_ERROR );
            SockPostApiCallout();
            goto exit;

        }

        SockPostApiCallout();

        if( addrlen != NULL ) {

            *addrlen = addressLength;

        }

        socketInfo->DeferSocket = ws1Handle;
        socketInfo->DeferAddressLength = addressLength;

    }

    //
    // At this point, we can just always handle it as if it were
    // a deferred accept.
    //

    SOCK_ASSERT( socketInfo->DeferSocket != INVALID_SOCKET );

    if( addrlen != NULL && *addrlen < socketInfo->DeferAddressLength ) {

        err = WSAEFAULT;
        goto exit;

    }

    if( addr != NULL && addrlen != NULL ) {

        *addrlen = socketInfo->DeferAddressLength;

        CopyMemory(
            addr,
            ( socketInfo + 1 ),
            *addrlen
            );

    }

    //
    // Deal with the condition function if present.
    //

    if( lpfnCondition != NULL ) {

        INT result;
        WSABUF callerId;
        WSABUF calleeId;
        LPWSABUF calleeIdPointer;
        SOCKADDR_IN calleeAddr;
        INT calleeAddrLen;

        //
        // See if we can retrieve the local name for the newly
        // accepted socket.
        //

        calleeAddrLen = sizeof(calleeAddr);

        SockPreApiCallout();

        result = socketInfo->Hooker->getsockname(
                     socketInfo->WS1Handle,
                     (SOCKADDR *)&calleeAddr,
                     &calleeAddrLen
                     );

        SockPostApiCallout();

        //
        // Setup the caller & callee ID structures.
        //

        if( result == SOCKET_ERROR ) {

            calleeIdPointer = NULL;

        } else {

            calleeIdPointer = &calleeId;
            calleeId.buf = (char *)&calleeAddr;
            calleeId.len = (u_long)calleeAddrLen;

        }

        callerId.buf = (char *)( socketInfo + 1 );
        callerId.len = (u_long)socketInfo->DeferAddressLength;

        //
        // Call the condition function.
        //

        result = (lpfnCondition)(
                     &callerId,
                     NULL,
                     NULL,
                     NULL,
                     calleeIdPointer,
                     NULL,
                     &newGroup,
                     dwCallbackData
                     );

        //
        // Interpret the results.
        //

        SOCK_ASSERT( err == NO_ERROR );

        switch( result ) {

        case CF_ACCEPT :
            if( !SockGetGroup(
                    &newGroup,
                    &newGroupType
                    ) ||
                !SockValidateConstrainedGroup(
                    newGroup,
                    newGroupType,
                    (SOCKADDR *)( socketInfo + 1 ),
                    socketInfo->DeferAddressLength
                    ) ) {

                err = WSAEINVAL;

            }
            break;

        case CF_REJECT : {

            LINGER lingerInfo;

            //
            // Abort the connection.
            //

            lingerInfo.l_onoff = 1;
            lingerInfo.l_linger = 0;

            SockPreApiCallout();

            socketInfo->Hooker->setsockopt(
                ws1Handle,
                SOL_SOCKET,
                SO_LINGER,
                (char *)&lingerInfo,
                sizeof(lingerInfo)
                );

            socketInfo->Hooker->closesocket(
                ws1Handle
                );

            SockPostApiCallout();

            ws1Handle = INVALID_SOCKET;
            err = WSAECONNREFUSED;

            }
            break;

        case CF_DEFER :
            err = WSATRY_AGAIN;
            break;

        default :
            err = WSAEINVAL;
            break;
        }

        if( err != NO_ERROR ) {
            goto exit;
        }

    }

    //
    // Create a new socket to use for the connection.
    //

    socketInfo2 = SockCreateSocket(
                      socketInfo->Hooker,
                      socketInfo->AddressFamily,
                      socketInfo->SocketType,
                      socketInfo->Protocol,
                      socketInfo->MaxSockaddrLength,
                      socketInfo->MinSockaddrLength,
                      socketInfo->CreationFlags,
                      socketInfo->CatalogEntryId,
                      socketInfo->ServiceFlags1,
                      socketInfo->ServiceFlags2,
                      socketInfo->ServiceFlags3,
                      socketInfo->ServiceFlags4,
                      socketInfo->ProviderFlags,
                      newGroup,
                      newGroupType,
                      ws1Handle,
                      &err
                      );

    if( socketInfo2 == NULL ) {

        SOCK_ASSERT( err != NO_ERROR );
        goto exit;

    }

    SOCK_ASSERT( ws1Handle == socketInfo2->WS1Handle );

    //
    // Reference the hooker so it doesn't go away prematurely.
    //

    SockReferenceHooker( socketInfo->Hooker );

    //
    // Remember that we don't have any deferred accepts pending.
    //

    socketInfo->DeferSocket = INVALID_SOCKET;

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    ws2Handle = socketInfo2->WS2Handle;
    SOCK_ASSERT( ws2Handle != INVALID_SOCKET );

exit:

    if( err == NO_ERROR ) {

        //
        // Dereference the newly created socket.
        //

        SockDereferenceSocket( socketInfo2 );

    } else {

        //
        // We're failing this request, so we need to clean up.
        //
        // Remember any WS1 socket we managed to get from the hooker.
        // This handle may be INVALID_SOCKET, which is OK.
        //

        SOCK_ASSERT( socketInfo != NULL );
        socketInfo->DeferSocket = ws1Handle;

        //
        // Blow away the newly created socket. Note that, since sockets
        // are created with an initial reference count of 2, we need to
        // dereference it twice to force it to go away.
        //

        if( socketInfo2 != NULL ) {

            SockDereferenceSocket( socketInfo2 );
            SockDereferenceSocket( socketInfo2 );

        }

        if( newGroup != 0 ) {

            SockDereferenceGroup( newGroup );

        }

        *lpErrno = err;
        ws2Handle = INVALID_SOCKET;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPAccept", ws2Handle, (ws2Handle == INVALID_SOCKET) ? lpErrno : NULL );
    return ws2Handle;

}   // WSPAccept

