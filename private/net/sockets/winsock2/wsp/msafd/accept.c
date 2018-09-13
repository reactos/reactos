/*++

Copyright (c) 1992-1999 Microsoft Corporation

Module Name:

    accept.c

Abstract:

    This module contains support for the accept( ) WinSock API.

Author:

    David Treadwell (davidtr)    9-Mar-1992

Revision History:
    Vadim Eydelman  (vadime)     1998-1999 NT5.0 Feature additions:
                                    TRUE Conditional Accept
                                    QOS and connect data support for WSH dlls


--*/

#include "winsockp.h"

INT
SetHelperDllContext (
    IN PSOCKET_INFORMATION ParentSocket,
    IN PSOCKET_INFORMATION ChildSocket
    );


SOCKET
WSPAPI
WSPAccept(
    IN SOCKET Handle,
    OUT struct sockaddr *SocketAddress,
    OUT int *SocketAddressLength,
    IN LPCONDITIONPROC lpfnCondition,
    IN DWORD_PTR dwCallbackData,
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
        in lpErrno.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socketInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    PAFD_LISTEN_RESPONSE_INFO afdListenResponse;
    ULONG afdListenResponseLength;
#ifdef _AFD_SAN_SWITCH_
    AFD_SAN_ACCEPT_INFO afdAcceptInfo;
#else //_AFD_SAN_SWITCH_
    AFD_ACCEPT_INFO afdAcceptInfo;
#endif //_AFD_SAN_SWITCH_
    int err;
    SOCKET newSocketHandle;
    PSOCKET_INFORMATION newSocket;
    WSAPROTOCOL_INFOW protocolInfo;
    GROUP newGroup;
    PCHAR connectDataBuffer;
    INT connectDataBufferLength;
    PCHAR calleeDataBuf;
    LPQOS lpSQOS, lpGQOS;
    BYTE afdLocalListenResponse[MAX_FAST_LISTEN_RESPONSE];

    WS_ENTER( "WSPAccept", (PVOID)Handle, SocketAddress, (PVOID)SocketAddressLength, lpfnCondition );

    WS_ASSERT( lpErrno != NULL );

    IF_DEBUG(ACCEPT) {
        WS_PRINT(( "WSPAccept() on socket %lx addr %ld addrlen *%lx == %ld\n",
                       Handle, SocketAddress, SocketAddressLength,
                       SocketAddressLength ? *SocketAddressLength : 0 ));
    }

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPAccept", INVALID_SOCKET, TRUE );
        *lpErrno = err;
        return INVALID_SOCKET;

    }

    //
    // Set up local variables so that we know how to clean up on exit.
    //

    socketInfo = NULL;
    afdListenResponse = NULL;
    newSocket = NULL;
    newSocketHandle = INVALID_SOCKET;
    newGroup = 0;
    connectDataBuffer = NULL;
    calleeDataBuf = NULL;
    lpSQOS = NULL;
    lpGQOS = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socketInfo = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socketInfo == NULL ) {

        err = WSAENOTSOCK;
        goto exit;

    }

    //
    // Acquire the lock that protects sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're doing the accept on.
    //

    SockAcquireSocketLockExclusive( socketInfo );

    //
    // If this is a datagram socket, fail the request.
    //

    if ( IS_DGRAM_SOCK(socketInfo) ) {

        err = WSAEOPNOTSUPP;
        goto exit;

    }

    //
    // If the socket is not listening for connection attempts, fail this
    // request.
    //

    if ( !socketInfo->Listening ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // Make sure that the length of the address buffer is large enough
    // to hold a sockaddr structure for the address family of this
    // socket.
    //

    __try {
        if ( ARGUMENT_PRESENT( SocketAddressLength ) &&
                 socketInfo->HelperDll->MinSockaddrLength > *SocketAddressLength ) {

            err = WSAEFAULT;
            goto exit;

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

    //
    // Allocate space to hold the listen response structure.  This
    // buffer must be large enough to hold a TRANSPORT_STRUCTURE for the
    // address family of this socket.
    //

    afdListenResponseLength = sizeof(AFD_LISTEN_RESPONSE_INFO) -
                                  sizeof(TRANSPORT_ADDRESS) +
                                  socketInfo->HelperDll->MaxTdiAddressLength;

    if ( afdListenResponseLength <= sizeof(afdLocalListenResponse) ) {

        afdListenResponse = (PAFD_LISTEN_RESPONSE_INFO)afdLocalListenResponse;

    } else {

        afdListenResponse = ALLOCATE_HEAP( afdListenResponseLength );

        if ( afdListenResponse == NULL ) {

            err = WSAENOBUFS;
            goto exit;

        }

    }

    //
    // If the socket is non-blocking, determine whether data exists on
    // the socket.  If not, fail the request.
    //

    if ( socketInfo->NonBlocking ) {
        struct fd_set readfds;
        struct timeval timeout;
        int returnCode;

        FD_ZERO( &readfds );
        FD_SET( Handle, &readfds );
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        returnCode = WSPSelect(
                         1,
                         &readfds,
                         NULL,
                         NULL,
                         &timeout,
                         lpErrno
                         );

        if ( returnCode == SOCKET_ERROR ) {

            err = *lpErrno;
            goto exit;

        }

        if ( !FD_ISSET( Handle, &readfds ) ) {

            WS_ASSERT( returnCode == 0 );
            err = WSAEWOULDBLOCK;
            goto exit;

        }

        WS_ASSERT( returnCode == 1 );

    }

    //
    // Wait for a connection attempt to arrive.
    //

    status = NtDeviceIoControlFile(
                 socketInfo->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_WAIT_FOR_LISTEN,
                 NULL,
                 0,
                 afdListenResponse,
                 afdListenResponseLength
                 );

    if ( status == STATUS_PENDING ) {

        SockReleaseSocketLock( socketInfo );
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Handle,
            SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socketInfo );
        status = ioStatusBlock.Status;

    }

    if ( !NT_SUCCESS(status) ) {

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // If a condition function was specified, then give the user the
    // opportunity to accept/reject the incoming connection.
    //

    if( lpfnCondition != NULL ) {

        WSABUF callerId;
        WSABUF callerData;
        WSABUF calleeId;
        WSABUF calleeData;
        INT result;
        BYTE fastAddressBuffer[sizeof(SOCKADDR_IN)];
        PBYTE addressBuffer;
        ULONG addressBufferLength;
        INT remoteAddressLength;
        BOOLEAN isValidGroup;

        //
        // Allocate space for the remote address.
        //

        addressBufferLength = socketInfo->HelperDll->MaxSockaddrLength;

        if( addressBufferLength <= sizeof(fastAddressBuffer) ) {

            addressBuffer = fastAddressBuffer;

        } else {

            addressBuffer = ALLOCATE_HEAP( addressBufferLength );

            if( addressBuffer == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

        }

        connectDataBufferLength = 0;

        if( ( socketInfo->ServiceFlags1 & XP1_CONNECT_DATA ) != 0 ) {

            AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;

            //
            // Determine the size of the incoming connect data.
            //

            connectInfo.Sequence = afdListenResponse->Sequence;
            connectInfo.LengthOnly = TRUE;

            status = NtDeviceIoControlFile(
                         socketInfo->HContext.Handle,
                         tlsData->EventHandle,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA,
                         &connectInfo,
                         sizeof(connectInfo),
                         &connectInfo,
                         sizeof(connectInfo)
                         );

            if( status == STATUS_PENDING ) {

                SockWaitForSingleObject(
                    tlsData->EventHandle,
                    Handle,
                    SOCK_NEVER_CALL_BLOCKING_HOOK,
                    SOCK_NO_TIMEOUT
                    );

                status = ioStatusBlock.Status;

            }

            if( !NT_SUCCESS(status) ) {

                err = SockNtStatusToSocketError( status );
                goto exit;

            }

            //
            // If there is data on the connection, get it.
            //

            connectDataBufferLength = (ULONG)ioStatusBlock.Information;

            if( connectDataBufferLength > 0 ) {

                connectDataBuffer = ALLOCATE_HEAP( connectDataBufferLength );

                if( connectDataBuffer == NULL ) {

                    err = WSAENOBUFS;
                    goto exit;

                }

                //
                // Retrieve the data.
                //

                connectInfo.Sequence = afdListenResponse->Sequence;
                connectInfo.LengthOnly = FALSE;

                status = NtDeviceIoControlFile(
                             socketInfo->HContext.Handle,
                             tlsData->EventHandle,
                             NULL,
                             NULL,
                             &ioStatusBlock,
                             IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA,
                             &connectInfo,
                             sizeof(connectInfo),
                             connectDataBuffer,
                             connectDataBufferLength
                             );

                if( status == STATUS_PENDING ) {

                    SockWaitForSingleObject(
                        tlsData->EventHandle,
                        Handle,
                        SOCK_NEVER_CALL_BLOCKING_HOOK,
                        SOCK_NO_TIMEOUT
                        );

                    status = ioStatusBlock.Status;

                }

                if( !NT_SUCCESS(status) ) {

                    err = SockNtStatusToSocketError( status );
                    goto exit;

                }

            }

        }

        if( ( socketInfo->ServiceFlags1 & XP1_QOS_SUPPORTED ) != 0 ) {
            int result;
            DWORD   bytesReturned;
            afdAcceptInfo.Sequence = afdListenResponse->Sequence;
            afdAcceptInfo.AcceptHandle = socketInfo->HContext.Handle;
            bytesReturned = 0;
            WS_ASSERT (tlsData->AcceptInfo == NULL);
            tlsData->AcceptInfo = &afdAcceptInfo;
            __try {
                result = WSPIoctl (Handle,
                                    SIO_GET_QOS,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    &bytesReturned,
                                    NULL, NULL, NULL, &err);
            }
            __finally {
                tlsData->AcceptInfo = NULL;
            }
            if (result==SOCKET_ERROR) {
                if (err==WSAEFAULT) {
                    if (bytesReturned>0) {
                        lpSQOS = ALLOCATE_HEAP (bytesReturned);
                        if (lpSQOS==NULL) {
                            err = WSAENOBUFS;
                            goto exit;
                        }
                        WS_ASSERT (tlsData->AcceptInfo == NULL);
                        tlsData->AcceptInfo = &afdAcceptInfo;
                        __try {
                            result = WSPIoctl (Handle,
                                                SIO_GET_QOS,
                                                NULL,
                                                0,
                                                lpSQOS,
                                                bytesReturned,
                                                &bytesReturned,
                                                NULL, NULL, NULL, &err);
                        }
                        __finally {
                            tlsData->AcceptInfo = NULL;
                        }
                    }
                }
                else {
                    goto exit;
                }
            }

            bytesReturned = 0;
            WS_ASSERT (tlsData->AcceptInfo == NULL);
            tlsData->AcceptInfo = &afdAcceptInfo;
            __try {
                result = WSPIoctl (Handle,
                                    SIO_GET_GROUP_QOS,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    &bytesReturned,
                                    NULL, NULL, NULL, &err);
            }
            __finally {
                tlsData->AcceptInfo = NULL;
            }
            if (result==SOCKET_ERROR) {
                if (err==WSAEFAULT) {
                    if (bytesReturned>0) {
                        lpGQOS = ALLOCATE_HEAP (bytesReturned);
                        if (lpGQOS==NULL) {
                            err = WSAENOBUFS;
                            goto exit;
                        }

                        WS_ASSERT (tlsData->AcceptInfo == NULL);
                        tlsData->AcceptInfo = &afdAcceptInfo;
                        __try {
                            result = WSPIoctl (Handle,
                                                SIO_GET_GROUP_QOS,
                                                NULL,
                                                0,
                                                lpGQOS,
                                                bytesReturned,
                                                &bytesReturned,
                                                NULL, NULL, NULL, &err);
                        }
                        __finally {
                            tlsData->AcceptInfo = NULL;
                        }
                    }
                }
                else {
                    goto exit;
                }
            }
        }

        //
        // Build the addresses.
        //

        calleeId.buf = (CHAR *)socketInfo->LocalAddress;
        calleeId.len = (ULONG)socketInfo->LocalAddressLength;

        SockBuildSockaddr(
            (PSOCKADDR)addressBuffer,
            &remoteAddressLength,
            &afdListenResponse->RemoteAddress
            );

        callerId.buf = (CHAR *)addressBuffer;
        callerId.len = (ULONG)remoteAddressLength;

        //
        // Build the caller/callee data.
        //
        // Unfortunately, since we're "faking" this deferred accept
        // stuff (meaning that AFD has already fully accepted the
        // connection before the DLL ever sees it) there's no way
        // to support "callee data" in the condition function.
        //

        callerData.buf = connectDataBuffer;
        callerData.len = connectDataBufferLength;

        if (socketInfo->ConditionalAccept) {
            //
            // We can now support callee data in case where delayed
            // accept is enabled. However, we still have not idea
            // what should be the size of the buffer.  So we just allocate
            // 4k.  Should be reasonable enough for most cases.
            //
            #define CALLEE_DATA_BUFFER_SIZE 4096
            calleeDataBuf = ALLOCATE_HEAP (CALLEE_DATA_BUFFER_SIZE);
            if (calleeDataBuf!=NULL) {
                calleeData.buf = calleeDataBuf;
                calleeData.len = CALLEE_DATA_BUFFER_SIZE;
            }
            else
                calleeData.len = 0;
        }
        else {
            calleeData.buf = NULL;
            calleeData.len = 0;
        }

        result = (lpfnCondition)(
                     &callerId,                 // lpCallerId
                     callerData.buf == NULL     // lpCallerData
                        ? NULL
                        : &callerData,
                     lpSQOS,                    // lpSQOS
                     lpGQOS,                    // lpGQOS
                     &calleeId,                 // lpCalleeId
                     calleeData.buf == NULL     // lpCalleeData
                        ? NULL
                        : &calleeData,
                     &newGroup,                 // g
                     dwCallbackData             // dwCallbackData
                     );

        //
        // Before we free the buffers, validate the group ID returned
        // by the condition function.
        //

        isValidGroup = TRUE;

        if( result == CF_ACCEPT &&
            newGroup != 0 &&
            newGroup != SG_UNCONSTRAINED_GROUP &&
            newGroup != SG_CONSTRAINED_GROUP ) {

            err = SockIsAddressConsistentWithConstrainedGroup(
                               socketInfo,
                               newGroup,
                               (PSOCKADDR)addressBuffer,
                               remoteAddressLength
                               );
            isValidGroup = (err==NO_ERROR);

        }

        if( addressBuffer != fastAddressBuffer ) {

            FREE_HEAP( addressBuffer );

        }

        if( result == CF_ACCEPT ) {

            if( !isValidGroup ) {

                WS_ASSERT (err!=NO_ERROR);
                goto exit;

            }

            if( ( socketInfo->ServiceFlags1 & XP1_QOS_SUPPORTED ) != 0 ) {
                if (lpSQOS!=NULL) {
                    int result;
                    DWORD   bytesReturned;
                    afdAcceptInfo.Sequence = afdListenResponse->Sequence;
                    afdAcceptInfo.AcceptHandle = socketInfo->HContext.Handle;
                    bytesReturned = 0;
                    WS_ASSERT (tlsData->AcceptInfo == NULL);
                    tlsData->AcceptInfo = &afdAcceptInfo;
                    __try {
                        result = WSPIoctl (Handle,
                                            SIO_SET_QOS,
                                            lpSQOS, sizeof (*lpSQOS),
                                            NULL,
                                            0,
                                            &bytesReturned,
                                            NULL, NULL, NULL, &err);
                    }
                    __finally {
                        tlsData->AcceptInfo = NULL;
                    }

                    if (result==SOCKET_ERROR) {
                        WS_ASSERT (err!=NO_ERROR);
                        goto exit;
                    }
                }

                if (lpGQOS!=NULL) {
                    int result;
                    DWORD   bytesReturned;
                    afdAcceptInfo.Sequence = afdListenResponse->Sequence;
                    afdAcceptInfo.AcceptHandle = socketInfo->HContext.Handle;
                    bytesReturned = 0;

                    WS_ASSERT (tlsData->AcceptInfo == NULL);
                    tlsData->AcceptInfo = &afdAcceptInfo;
                    __try {
                        result = WSPIoctl (Handle,
                                            SIO_SET_GROUP_QOS,
                                            lpGQOS, sizeof (*lpGQOS),
                                            NULL,
                                            0,
                                            &bytesReturned,
                                            NULL, NULL, NULL, &err);
                    }
                    __finally {
                        tlsData->AcceptInfo = NULL;
                    }
                    if (result==SOCKET_ERROR) {
                        WS_ASSERT (err!=NO_ERROR);
                        goto exit;
                    }
                }
            }

            if (socketInfo->HelperDll->UseDelayedAcceptance && calleeData.len>0) {
                WS_ASSERT (tlsData->AcceptInfo == NULL);
                tlsData->AcceptInfo = &afdAcceptInfo;
                __try {
                    err = SockSetConnectData(
                            socketInfo,
                            IOCTL_AFD_SET_CONNECT_DATA,
                            calleeData.buf,
                            calleeData.len,
                            NULL
                            );  
                }
                __finally {
                    tlsData->AcceptInfo = NULL;
                }
                if (err!=NO_ERROR) {
                    goto exit;
                }
            }

        } else {

            //
            // If the condition function returned any value other than
            // CF_ACCEPT, then we'll need to tell AFD to deal with this,
            // and we may also need to retry the wait for listen.
            //

            AFD_DEFER_ACCEPT_INFO deferAcceptInfo;

            if( result != CF_DEFER && result != CF_REJECT ) {

                err = WSAEINVAL;
                goto exit;

            }

            deferAcceptInfo.Sequence = afdListenResponse->Sequence;
            deferAcceptInfo.Reject = ( result == CF_REJECT );

            status = NtDeviceIoControlFile(
                         socketInfo->HContext.Handle,
                         tlsData->EventHandle,
                         NULL,                   // APC Routine
                         NULL,                   // APC Context
                         &ioStatusBlock,
                         IOCTL_AFD_DEFER_ACCEPT,
                         &deferAcceptInfo,
                         sizeof(deferAcceptInfo),
                         NULL,
                         0
                         );

            //
            // IOCTL_AFD_DEFER_ACCEPT should never pend.
            //

            WS_ASSERT( status != STATUS_PENDING );

            if ( !NT_SUCCESS(status) ) {

                err = SockNtStatusToSocketError( status );
                goto exit;

            }

            //
            // The condition function returned either CF_REJECT or
            // CF_DEFER, so fail the WSPAccept() call with the
            // appropriate error code.
            //

            if( result == CF_REJECT ) {

                err = WSAECONNREFUSED;

            } else {

                WS_ASSERT( result == CF_DEFER );
                err = WSATRY_AGAIN;

            }

            goto exit;

        }

    }

    //
    // Create a new socket to use for the connection.
    //

    RtlZeroMemory( &protocolInfo, sizeof(protocolInfo) );

    protocolInfo.iAddressFamily = socketInfo->AddressFamily;
    protocolInfo.iSocketType = socketInfo->SocketType;
    protocolInfo.iProtocol = socketInfo->Protocol;
    protocolInfo.dwCatalogEntryId = socketInfo->CatalogEntryId;
    protocolInfo.dwServiceFlags1 = socketInfo->ServiceFlags1;
    protocolInfo.dwProviderFlags = socketInfo->ProviderFlags;

    newSocketHandle = WSPSocket(
                          socketInfo->AddressFamily,
                          socketInfo->SocketType,
                          socketInfo->Protocol,
                          &protocolInfo,
                          newGroup,
                          socketInfo->CreationFlags,
                          &err
                          );

    if ( newSocketHandle == INVALID_SOCKET ) {

        WS_ASSERT( err != NO_ERROR );
        goto exit;

    }

    //
    // Find a pointer to the new socket and reference the socket.
    //

    newSocket = SockFindAndReferenceSocket( newSocketHandle, FALSE );

    if( newSocket == NULL ) {

        //
        // Cannot find the newly created socket. This usually means the
        // app has closed a random socket handle that just happens to
        // be the one we just created.
        //

        err = WSAENOTSOCK;
        goto exit;

    }

    //
    // Set up to accept the connection attempt.
    //

    afdAcceptInfo.Sequence = afdListenResponse->Sequence;
    afdAcceptInfo.AcceptHandle = (HANDLE)newSocketHandle;

    //
    // Remember the address of the remote client in the new socket,
    // and copy the local address of the newly created socket into
    // the new socket.
    //

    SockBuildSockaddr(
        newSocket->RemoteAddress,
        &newSocket->RemoteAddressLength,
        &afdListenResponse->RemoteAddress
        );

	//
	// Copy port address etc. from listening socket before calling
	// AFD. Then when SockSanCompletion() gets called, it will
	// fill in correct IP address
	//
    RtlCopyMemory(
        newSocket->LocalAddress,
        socketInfo->LocalAddress,
        socketInfo->LocalAddressLength
        );

    newSocket->LocalAddressLength = socketInfo->LocalAddressLength;

    //
    // Do the actual accept.  This associates the new socket we just
    // opened with the connection object that describes the VC that
    // was just initiated.
    //

#ifdef _AFD_SAN_SWITCH_
retrywithsan:
    afdAcceptInfo.SanActive = SockSanEnabled;
#endif // _AFD_SAN_SWITCH_
    status = NtDeviceIoControlFile(
                 socketInfo->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_ACCEPT,
                 &afdAcceptInfo,
                 sizeof(afdAcceptInfo),
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {

        SockReleaseSocketLock( socketInfo );
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Handle,
            SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socketInfo );
        status = ioStatusBlock.Status;

    }

    if ( !NT_SUCCESS(status) ) {

#ifdef _AFD_SAN_SWITCH_
        if (status==STATUS_INVALID_PARAMETER_12) {
            WS_ASSERT (afdAcceptInfo.SanActive==FALSE);
            SockReleaseSocketLock( socketInfo );
            status = SockSanActivate ();
            SockAcquireSocketLockExclusive ( socketInfo );
            if (NT_SUCCESS (status))
                goto retrywithsan;
        }
#endif // _AFD_SAN_SWITCH_

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // Notify the helper DLL that the socket has been accepted.
    //

    err = SockNotifyHelperDll( newSocket, WSH_NOTIFY_ACCEPT );

    if ( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Copy the remote address into the caller's address buffer, and
    // indicate the size of the remote address.
    //

    if ( ARGUMENT_PRESENT( SocketAddress ) &&
             ARGUMENT_PRESENT( SocketAddressLength ) ) {

        err = SockBuildSockaddr(
            SocketAddress,
            SocketAddressLength,
            &afdListenResponse->RemoteAddress
            );

        if (err!=NO_ERROR) {

            goto exit;

        }

    }

    //
    // Do the core operations in accepting the socket.
    //

    err = SockCoreAccept( socketInfo, newSocket );

    if ( err != NO_ERROR ) {

        goto exit;

    }

exit:

    IF_DEBUG(ACCEPT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "    WSPAccept on socket %lx (%lx) failed: %ld\n",
                           Handle, socketInfo, err ));

        } else {

            WS_PRINT(( "    WSPAccept on socket %lx (%lx) returned socket "
                       "%lx (%lx), remote", Handle,
                       socketInfo, newSocketHandle, newSocket ));

			if (newSocket) {
				WsPrintSockaddr( newSocket->RemoteAddress, 
								 &newSocket->RemoteAddressLength );
			}

        }

    }

    if ( socketInfo != NULL ) {

        if ( SockAsyncSelectCalled ) {

            SockReenableAsyncSelectEvent( socketInfo, FD_ACCEPT );

        }

        SockReleaseSocketLock( socketInfo );
        SockDereferenceSocket( socketInfo );

    }

    if ( newSocket != NULL ) {

        SockDereferenceSocket( newSocket );

    }

    if ( afdListenResponse != (PAFD_LISTEN_RESPONSE_INFO)afdLocalListenResponse &&

        afdListenResponse != NULL ) {
        FREE_HEAP( afdListenResponse );

    }

    if( connectDataBuffer != NULL ) {

        FREE_HEAP( connectDataBuffer );

    }

    if( calleeDataBuf != NULL ) {

        FREE_HEAP( calleeDataBuf );

    }

    if( lpSQOS != NULL ) {

        FREE_HEAP( lpSQOS );

    }

    if( lpGQOS != NULL ) {

        FREE_HEAP( lpGQOS );

    }

    if ( err != NO_ERROR ) {

        if ( newSocketHandle != INVALID_SOCKET ) {

            int errTmp;

            WSPCloseSocket( newSocketHandle, &errTmp );

        }

        *lpErrno = err;
        return INVALID_SOCKET;

    }

    WS_EXIT( "WSPAccept", newSocketHandle, FALSE );
    return newSocketHandle;

} // WSPAccept


INT
SetHelperDllContext (
    IN PSOCKET_INFORMATION ParentSocket,
    IN PSOCKET_INFORMATION ChildSocket
    )
{

    PVOID context;
    ULONG helperDllContextLength;
    INT error;

    //
    // Get TDI handles for the child socket.
    //

    error = SockGetTdiHandles( ChildSocket );

    if ( error != NO_ERROR ) {

        return error;

    }

    //
    // Determine how much space we need for the helper DLL context
    // on the parent socket.
    //

    error = ParentSocket->HelperDll->WSHGetSocketInformation (
                ParentSocket->HelperDllContext,
                ParentSocket->Handle,
                ParentSocket->TdiAddressHandle,
                ParentSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                NULL,
                (PINT)&helperDllContextLength
                );

    if ( error != NO_ERROR ) {

        return error;

    }

    //
    // Allocate a buffer to hold all context information.
    //

    context = ALLOCATE_HEAP( helperDllContextLength );

    if ( context == NULL ) {

        return WSAENOBUFS;

    }

    //
    // Get the parent socket's information.
    //

    error = ParentSocket->HelperDll->WSHGetSocketInformation (
                ParentSocket->HelperDllContext,
                ParentSocket->Handle,
                ParentSocket->TdiAddressHandle,
                ParentSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                context,
                (PINT)&helperDllContextLength
                );

    if ( error != NO_ERROR ) {

        FREE_HEAP( context );
        return error;

    }

    //
    // Set the parent's context on the child socket.
    //

    error = ChildSocket->HelperDll->WSHSetSocketInformation (
                ChildSocket->HelperDllContext,
                ChildSocket->Handle,
                ChildSocket->TdiAddressHandle,
                ChildSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                context,
                helperDllContextLength
                );

    FREE_HEAP( context );

    if ( error != NO_ERROR ) {

        return error;

    }

    //
    // It all worked.
    //

    return NO_ERROR;

} // SetHelperDllContext


BOOL
AcceptEx (
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Combines several socket functions into a single API/kernel
    transition.  A new connection is accepted, both the local and remote
    addresses for the connection are returned, and a receive is done for
    the first chunk of data sent by the remote.  By combining all these
    functions into a sigle call, the performance of connection
    acceptance is significantly improved.

    Unlike the normal winsock accept() call, AcceptEx() is
    asynchronous, which lets it fit better with servers which use thread
    pooling to improve scalability.  As with all overlapped Win32
    functions, either Win32 events or completion ports may be used
    as a completion notification mechanism.

    Another key difference between this function and the normal accept()
    function is that this function requires the caller to specify
    both the listening socket AND the socket on which to accept the
    connection.  The sAcceptSocket must be an opened socket which
    is neither bound nor connected.  Note that calling TransmitFile()
    with both the TF_DISCONNECT and TF_REUSE_SOCKET flags will result
    in the specified socket being returned to the "open" state, so
    such a socket may be passed to AcceptEx() as sAcceptSocket.

    Note that only a single output buffer receives the data as well as
    the local and remote socket addresses.  Use of a single buffer
    improves performance, but the caller should call th
    GetAcceptExSockaddrs() function to locate the addresses in the
    output buffer.

    Because the addresses are written in an internal format, the caller
    must specify sisteen more bytes for them than the size of the
    SOCKADDR structure for the transport protocol in use.  For example,
    th size of a SOCKADDR_IN, the address structure for TCP/IP, is 16
    bytes, so at least 24 bytes must be specified as the buffer sizes
    for the local and remote addresses.

    The lpNumberOfBytesTransferred parameters of the
    GetQueuedCompletionStatus() and GetOverlappedResult() APIs indicates
    the number of bytes received in the request.

    When this operation completes successfully, sAcceptHandle may
    be used for only the following routines:

        ReadFile
        WriteFile
        send
        recv
        TransmitFile
        closesocket

    In order to use it with othr winsock routines, a setsockopt() with
    the SO_UPDATE_ACCEPT_CONTEXT option must be performed on the socket.
    This socket option initializes some user-mode state on the socket
    which allows other winsock routines to access the socket correctly.

    sAcceptSocket will not inherit the properties of sListenSocket until
    SO_UPDATE_ACCEPT_CONTEXT is set on the socket.  When AcceptEx()
    returns, sAcceptSocket is in the default state for a connected
    socket.

    To use the SO_UPDATE_ACCEPT_CONTEXT option, call the setsockopt()
    function, specifying sAcceptSocket as the socket handle to setsockopt()
    and specify sListenSocket as the option value.  For example,

        err = setsockopt( sAcceptSocket,
                          SOL_SOCKET,
                          SO_UPDATE_ACCEPT_CONTEXT,
                          (char *)&sListenSocket,
                          sizeof(sListenSocket) );

Arguments:

    sListenSocket - a listening socket on which to accept a connection.
        The listen() socket API must have been previously called on this
        handle.

    sAcceptSocket - an open socket handle on which to accept an incoming
        connection.  This socket must not be bound or connected.

    lpOutputBuffer - a pointer to a buffer which receives the first
        chunk of data sent on the connection and the local and remote
        addresses for the new connection.  The receive data is written
        to the first part of the buffer (starting at offset 0) and the
        addresses are written later in the buffer.  This parameter must
        be specified.

    dwReceiveDataLength - the number of bytes in the buffer that should
        be used for receiving data.  If this parameter is specified as
        0, then no receive operation is performed in conjunction with
        the accept--the accept completes as soon as a connection arrives
        without waiting for any data to arrive.

    dwLocalAddressLength - the number of bytes reserved for the local
        address information.  This must be at least sixteen bytes more
        than the maximum sockaddr length for the transport protocol in
        use.

    dwRemoteAddressLength - the number of bytes reserved for the remote
        address information.  This must be at least sixteen bytes more
        than the maximum sockaddr length for the transport protocol in
        use.

    lpdwBytesReceived - points to a DWORD which receives the count of
        bytes received.  This is only set if the operation completes
        synchronously; if it returns ERROR_IO_PENDING and completes
        later, then this DWORD is never set and the count of bytes read
        may be obtained from the completion notification mechnaism.

    lpOverlapped - an OVERLAPPED structure to use in processing the request.
        This parameter MUST be specified; it may not be NULL.


Return Value:

    TRUE if the operation completed successfully.  FALSE if there was an
    error, in which case GetLastError() may be called to return extended
    error information.  If GetLastError() returns ERROR_IO_PENDING then
    the operation was successfully initiated and is still in progress.

--*/

{
    DWORD inputBufferLength;
#ifdef _AFD_SAN_SWITCH_
    BYTE inputBuffer[sizeof(AFD_SAN_SUPER_ACCEPT_INFO) + 500];
    PAFD_SAN_SUPER_ACCEPT_INFO superAcceptInfo;
#else // _AFD_SAN_SWITCH_
    BYTE inputBuffer[sizeof(AFD_SUPER_ACCEPT_INFO) + 500];
    PAFD_SUPER_ACCEPT_INFO superAcceptInfo;
#endif // _AFD_SAN_SWITCH_
    NTSTATUS status;

    //
    // The lpOverlapped parameter is required for this function.
    //

    if ( !ARGUMENT_PRESENT( lpOverlapped ) ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // The input buffer must be large enough to hold both the super
    // accept info and the remote address.  If the stack space isn't
    // large enough, allocate some heap.
    //

#ifdef _AFD_SAN_SWITCH_
    inputBufferLength = sizeof(AFD_SAN_SUPER_ACCEPT_INFO) + dwRemoteAddressLength;
#else //_AFD_SAN_SWITCH_
    inputBufferLength = sizeof(AFD_SUPER_ACCEPT_INFO) + dwRemoteAddressLength;
#endif //_AFD_SAN_SWITCH_

    if ( sizeof(inputBuffer) < inputBufferLength ) {

        superAcceptInfo = ALLOCATE_HEAP( inputBufferLength );
        if ( superAcceptInfo == NULL ) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }

    } else {

#ifdef _AFD_SAN_SWITCH_
        superAcceptInfo = (PAFD_SAN_SUPER_ACCEPT_INFO)inputBuffer;
#else //_AFD_SAN_SWITCH_
        superAcceptInfo = (PAFD_SUPER_ACCEPT_INFO)inputBuffer;
#endif //_AFD_SAN_SWITCH_
    }

    //
    // Initialize the info we need to pass to AFD.
    //

    superAcceptInfo->AcceptHandle = (HANDLE)sAcceptSocket;
    superAcceptInfo->ReceiveDataLength = dwReceiveDataLength;
    superAcceptInfo->LocalAddressLength = dwLocalAddressLength;
    superAcceptInfo->RemoteAddressLength = dwRemoteAddressLength;
#ifdef _AFD_SAN_SWITCH_
retrywithsan:
    superAcceptInfo->SanActive = SockSanEnabled;
#endif //_AFD_SAN_SWITCH_

    __try {
        lpOverlapped->Internal = (UINT_PTR)STATUS_PENDING;

        //
        // Make the actual IOCTL call to AFD.
        //

        status = NtDeviceIoControlFile(
                     (HANDLE)sListenSocket,
                     lpOverlapped->hEvent,
                     NULL,
                     (((ULONG_PTR)lpOverlapped->hEvent) & 1) ? NULL : lpOverlapped,
                     (PIO_STATUS_BLOCK)(&lpOverlapped->Internal),
                     IOCTL_AFD_SUPER_ACCEPT,
                     superAcceptInfo,
                     inputBufferLength,
                     lpOutputBuffer,
                     dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength
                     );
#ifdef _AFD_SAN_SWITCH_
        if (status==STATUS_INVALID_PARAMETER_12) {
            WS_ASSERT (superAcceptInfo->SanActive==FALSE);
            status = SockSanActivate ();
            if (NT_SUCCESS (status))
                goto retrywithsan;
        }
#endif //_AFD_SAN_SWITCH_

        if ( (PBYTE)superAcceptInfo != inputBuffer ) {
            FREE_HEAP( superAcceptInfo );
        }

        if ( NT_SUCCESS(status) && status != STATUS_PENDING) {
            __try {
                *lpdwBytesReceived = (DWORD)lpOverlapped->InternalHigh;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                *lpdwBytesReceived = 0;
            }
            return TRUE;
        } else {
            SetLastError( SockNtStatusToSocketError(status) );
            return FALSE;
        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        if ( (PBYTE)superAcceptInfo != inputBuffer ) {
            FREE_HEAP( superAcceptInfo );
        }
        SetLastError( WSAEFAULT );
        return FALSE;
    }

} // AcceptEx


VOID
GetAcceptExSockaddrs (
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPSOCKADDR *LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT LPSOCKADDR *RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    )

/*++

Routine Description:

    Processes the lpOutputBuffer parameter after a successful AcceptEx()
    operation.  Because AcceptEx() writes address information in an
    internal (TDI) format, this routine is required to locate the
    SOCKADDR structures in the buffer.

Arguments:

    lpOutputBuffer - the same lpOutputBuffer parameter that was passed
        to AcceptEx().

    dwReceiveDataLength - must be equal to the dwReceiveDataLength
        parameter which was passed to AcceptEx().

    dwLocalAddressLength - must be equal to the dwLocalAddressLength
        parameter which was passed to AcceptEx().

    dwRemoteAddressLength - must be equal to the dwRemoteAddressLength
        parameter which was passed to AcceptEx().

    LocalSockaddr - receives a pointer to the SOCKADDR which describes
        the local address of the connection (the same information as
        would be returned by getsockname()).  This parameter must be
        specified.

    LocalSockaddrLength - receives the size of the local address.  This
        parameter must be specified.

    RemoteSockaddr - receives a pointer to the SOCKADDR which describes
        the remote address of the connection (the same information as
        would be returned by getpeername()).  This parameter must be
        specified.

    RemoteSockaddrLength - receives the size of the local address.  This
        parameter must be specified.

Return Value:

    None.

Note:
    We do not do exception handling in this routine because
    if we did, we won't be able to return any error indication
    to the caller.
--*/

{
    PTRANSPORT_ADDRESS tdiAddress;

    if (dwLocalAddressLength>0) {
        //
        // First locate the local address.  There is one ULONG between the
        // start of the local address section of the buffer and the actual
        // TDI address.
        //

        tdiAddress = (PTRANSPORT_ADDRESS)
            ( (PCHAR)lpOutputBuffer + dwReceiveDataLength + sizeof(ULONG) );


        //
        // Locate the local sockaddr and determine its length.
        //

        *LocalSockaddrLength =
            tdiAddress->Address[0].AddressLength +
            sizeof((*LocalSockaddr)->sa_family);

        *LocalSockaddr = (LPSOCKADDR)(&tdiAddress->Address[0].AddressType);

        //
        // If this is not an x86 machine, copy the sockaddr forward over the
        // TDI address.  This is required because there are 10 bytes of TDI
        // overhead in front of the buffer, so the sockaddr will be unaligned
        // if the input buffers were aligned.
        //
        // Note that we must use RtlMoveMemory here, not RtlCopyMemory,
        // because the buffers are likely to overlap.
        //

#ifndef i386
        if (*LocalSockaddrLength <= 
                ((INT)dwLocalAddressLength-((PCHAR)tdiAddress-(PCHAR)*LocalSockaddr))) {
            RtlMoveMemory( tdiAddress, *LocalSockaddr, *LocalSockaddrLength );
        }
        else {
            DbgPrint ("msafd!GetAcceptExSockaddrs: Remote address structure corrupted!!!\n"
                      "Second call to GetAcceptExSockaddrs with the same buffer???\n");
            ASSERT (*LocalSockaddrLength <=
                    ((INT)dwLocalAddressLength-((PCHAR)tdiAddress-(PCHAR)*LocalSockaddr)));
        }
        *LocalSockaddr = (LPSOCKADDR)tdiAddress;
#endif
    }

    if (dwRemoteAddressLength>0) {

        //
        // Repeat for the remote sockaddr.
        //

        tdiAddress = (PTRANSPORT_ADDRESS)
            ( (PCHAR)lpOutputBuffer + dwReceiveDataLength + dwLocalAddressLength );

        *RemoteSockaddrLength =
            tdiAddress->Address[0].AddressLength +
            sizeof((*RemoteSockaddr)->sa_family);

        *RemoteSockaddr = (LPSOCKADDR)(&tdiAddress->Address[0].AddressType);

        //
        // Again, move the sockaddr on risc machines.
        //

#ifndef i386
        if (*RemoteSockaddrLength <= 
                ((INT)dwRemoteAddressLength-((PCHAR)tdiAddress-(PCHAR)*RemoteSockaddr))) {
            RtlMoveMemory( tdiAddress, *RemoteSockaddr, *RemoteSockaddrLength );
        }
        else {
            DbgPrint ("msafd!GetAcceptExSockaddrs: Remote address structure corrupted!!!\n"
                      "Second call to GetAcceptExSockaddrs with the same buffer???\n");
            ASSERT (*RemoteSockaddrLength <=
                    ((INT)dwRemoteAddressLength-((PCHAR)tdiAddress-(PCHAR)*RemoteSockaddr)));
        }
        *RemoteSockaddr = (LPSOCKADDR)tdiAddress;
#endif
    }

    return;

} // GetAcceptExSockaddrs


INT
SockCoreAccept (
    IN PSOCKET_INFORMATION ListenSocket,
    IN PSOCKET_INFORMATION AcceptSocket
    )
{
    INT err;
    INT result;

    //
    // Indicate that the new socket is connected.
    //

    AcceptSocket->State = SocketStateConnected;

    //
    // Clone the properties of the listening socket to the new socket.
    //

    AcceptSocket->LingerInfo = ListenSocket->LingerInfo;
    AcceptSocket->ReceiveBufferSize = ListenSocket->ReceiveBufferSize;
    AcceptSocket->SendBufferSize = ListenSocket->SendBufferSize;
    AcceptSocket->Broadcast = ListenSocket->Broadcast;
    AcceptSocket->Debug = ListenSocket->Debug;
    AcceptSocket->OobInline = ListenSocket->OobInline;
    AcceptSocket->ReuseAddresses = ListenSocket->ReuseAddresses;
    AcceptSocket->SendTimeout = ListenSocket->SendTimeout;
    AcceptSocket->ReceiveTimeout = ListenSocket->ReceiveTimeout;

    //
    // Set up the new socket to have the same blocking, inline, and
    // timeout characteristics as the listening socket.
    //

    if ( ListenSocket->NonBlocking ) {
        BOOLEAN nb = ListenSocket->NonBlocking;
        err = SockSetInformation(
                    AcceptSocket,
                    AFD_NONBLOCKING_MODE,
                    &nb,
                    NULL,
                    NULL
                    );
        if ( err != NO_ERROR ) {
            return err;
        }
    }

    AcceptSocket->NonBlocking = ListenSocket->NonBlocking;

    if ( ListenSocket->OobInline ) {
        BOOLEAN oob = ListenSocket->OobInline;
        err = SockSetInformation(
                    AcceptSocket,
                    AFD_INLINE_MODE,
                    &oob,
                    NULL,
                    NULL
                    );
        if ( err != NO_ERROR ) {
            return err;
        }
    }

    AcceptSocket->OobInline = ListenSocket->OobInline;

    //
    // If the listening socket has been called with WSAAsyncSelect,
    // set up WSAAsyncSelect on this socket.  Otherwise, if the socket
    // has been called with WSAEventSelect, set up WSAEventSelect on
    // the socket.
    //

    if ( ListenSocket->AsyncSelectlEvent ) {

        result = WSPAsyncSelect(
                     AcceptSocket->Handle,
                     ListenSocket->AsyncSelecthWnd,
                     ListenSocket->AsyncSelectwMsg,
                     ListenSocket->AsyncSelectlEvent,
                     &err
                     );

        if ( result == SOCKET_ERROR ) {

            return err;

        }

    } else if ( ListenSocket->EventSelectlNetworkEvents ) {

        result = WSPEventSelect(
                     AcceptSocket->Handle,
                     ListenSocket->EventSelectEventObject,
                     ListenSocket->EventSelectlNetworkEvents,
                     &err
                     );

        if ( result == SOCKET_ERROR ) {

            return err;

        }

    }

    //
    // Clone the helper DLL's context from the listening socket to the
    // accepted socket.
    //

    err = SetHelperDllContext( ListenSocket, AcceptSocket );
    if ( err != NO_ERROR ) {
        err = NO_ERROR;
    }

    //
    // If the application has modified the send or receive buffer sizes,
    // then set up the buffer sizes on the socket.
    //

    err = SockUpdateWindowSizes( AcceptSocket, FALSE );
    if ( err != NO_ERROR ) {
        return err;
    }

    //
    // Remember the changed state of this socket.
    //

    err = SockSetHandleContext( AcceptSocket );
    if ( err != NO_ERROR ) {
        return err;
    }

    return NO_ERROR;

} // SockCoreAccept
