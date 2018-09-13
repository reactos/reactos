/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    connect.c

Abstract:

    This module contains support for the WSPConnect and
    WSPJoinLeaf WinSock SPI.

Author:

    David Treadwell (davidtr)    28-Feb-1992

Revision History:

--*/

#include "winsockp.h"


BOOLEAN
IsSockaddrEqualToZero (
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    );


INT
SockDoConnect (
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
    );

INT
UnconnectDatagramSocket (
    IN PSOCKET_INFORMATION Socket
    );


INT
WSPAPI
WSPConnect(
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPINT lpErrno
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
    int err;

    WS_ENTER( "WSPConnect", (PVOID)Handle, (PVOID)SocketAddress, (PVOID)SocketAddressLength, lpCallerData );

    IF_DEBUG(CONNECT) {
        WS_PRINT(( "connect()ing socket %lx to remote addr ", Handle ));
        WsPrintSockaddr( (PSOCKADDR)SocketAddress, &SocketAddressLength );
    }

    WS_ASSERT( lpErrno != NULL );

    //
    // Check the lpCaller/CalleeData params for validity
    //

    if ( lpCallerData ) {

        if ( IsBadReadPtr (lpCallerData, sizeof (*lpCallerData)) ||
             IsBadReadPtr (lpCallerData->buf, lpCallerData->len) )  {

            err = WSAEFAULT;
            goto exit;
        }
    }

    if ( lpCalleeData ) {

        if ( IsBadReadPtr (lpCalleeData, sizeof (*lpCalleeData)) ||
             IsBadReadPtr (lpCalleeData->buf, lpCalleeData->len) )  {

            err = WSAEFAULT;
            goto exit;
        }
    }

    if (lpSQOS) {
        if ( IsBadReadPtr (lpSQOS, sizeof (*lpSQOS)) ||
             (lpSQOS->ProviderSpecific.buf && 
                lpSQOS->ProviderSpecific.len &&
                IsBadReadPtr (lpSQOS->ProviderSpecific.buf, lpSQOS->ProviderSpecific.len)) )  {

            err = WSAEFAULT;
            goto exit;
        }

    }

    if (lpGQOS) {
        if ( IsBadReadPtr (lpGQOS, sizeof (*lpGQOS)) ||
             (lpGQOS->ProviderSpecific.buf && 
                lpGQOS->ProviderSpecific.len &&
                IsBadReadPtr (lpGQOS->ProviderSpecific.buf, lpGQOS->ProviderSpecific.len)) )  {

            err = WSAEFAULT;
            goto exit;
        }
    }

    err = SockDoConnect(
              Handle,
              SocketAddress,
              SocketAddressLength,
              lpCallerData,
              lpCalleeData,
              lpSQOS,
              lpGQOS
              );

    if( err == NO_ERROR ) {

        WS_EXIT( "WSPConnect", 0, FALSE );
        return 0;

    }

exit:

    WS_EXIT( "WSPConnect", SOCKET_ERROR, TRUE );
    *lpErrno = err;
    return SOCKET_ERROR;

} // WSPConnect


SOCKET
WSPAPI
WSPJoinLeaf (
    SOCKET Handle,
    const struct sockaddr FAR * SocketAddress,
    int SocketAddressLength,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    DWORD dwFlags,
    LPINT lpErrno
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;
    SOCKET leafHandle;
    PSOCKET_INFORMATION socket;
    PSOCKET_INFORMATION leafSocket;
    WSAPROTOCOL_INFOW dummyProtocolInfo;
    PAFD_CONNECT_JOIN_INFO  connectInfo;
    ULONG connectInfoLength;
    UCHAR localBuffer[MAX_FAST_TDI_ADDRESS
                +FIELD_OFFSET(AFD_CONNECT_JOIN_INFO, RemoteAddress)];
    NTSTATUS    status;
    IO_STATUS_BLOCK ioStatusBlock;

    WS_ENTER( "WSPJoinLeaf", (PVOID)Handle, (PVOID)SocketAddress, (PVOID)SocketAddressLength, lpCallerData );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPJoinLeaf", INVALID_SOCKET, TRUE );
        *lpErrno = err;
        return INVALID_SOCKET;

    }

    //
    // Setup locals so we know how to cleanup.
    //

    leafHandle = INVALID_SOCKET;
    leafSocket = NULL;
    connectInfo = NULL;

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
    // performing operations on the socket we're using.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Bomb off if the helper DLL associated with this socket doesn't
    // export the WSHJoinLeaf entrypoint.
    //

    if( socket->HelperDll->WSHJoinLeaf == NULL ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // Validate the socket state.
    //

    if( socket->State != SocketStateOpen &&
        socket->State != SocketStateBound &&
        socket->State != SocketStateConnected) {

        err = WSAEINVAL;
        goto exit;

    }

    if (socket->AsyncConnectContext) {
        //
        // If socket connect has just completed with failure,
        // the AsyncConnectContext is not cleared until
        // post connect processing completes in the
        // async thread.  The function below will wait
        // for this completion.  It will only do the wait
        // if connect has actually completed and just
        // the post-processing part remains to be done.
        //
        SockIsSocketConnected (socket);
        if (socket->AsyncConnectContext) {
            err = WSAEALREADY;
            goto exit;
        }
        goto exit;
    }

    //
    // Validate the address parameters.
    //

    if( SocketAddress == NULL ||
        SocketAddressLength < socket->HelperDll->MinSockaddrLength ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // If the socket address is too long, truncate to the max possible
    // length.
    //

    if( SocketAddressLength > socket->HelperDll->MaxSockaddrLength ) {

        SocketAddressLength = socket->HelperDll->MaxSockaddrLength;

    }


    __try { // Protects access to SocketAddress
        if( ( socket->CreationFlags & ALL_MULTIPOINT_FLAGS ) == 0 ||
            socket->AddressFamily != SocketAddress->sa_family ) {

            err = WSAEINVAL;
            goto exit;

        }

        //
        // If this socket is not yet bound to an address, bind it to an
        // address.  We only do this if the helper DLL for the socket supports
        // a get wildcard address routine--if it doesn't, the app must bind
        // to an address manually.
        //

        if ( socket->State == SocketStateOpen &&
                 socket->HelperDll->WSHGetWildcardSockaddr != NULL ) {

            PSOCKADDR sockaddr;
            INT sockaddrLength = socket->HelperDll->MaxSockaddrLength;
            int result;
            UCHAR localBuffer[MAX_FAST_TDI_ADDRESS+FIELD_OFFSET(SOCKADDR, sa_data)];

            if (sockaddrLength<=sizeof (localBuffer)) {
                sockaddr = (PVOID)localBuffer;
            }
            else {
                sockaddr = ALLOCATE_HEAP( sockaddrLength );

                if ( sockaddr == NULL ) {

                    err = WSAENOBUFS;
                    goto exit;

                }
            }

            err = socket->HelperDll->WSHGetWildcardSockaddr(
                        socket->HelperDllContext,
                        sockaddr,
                        &sockaddrLength
                        );

            if ( err != NO_ERROR ) {

                if (sockaddr!=(PVOID)localBuffer) {
                    FREE_HEAP( sockaddr );
                }
                goto exit;

            }

            //
            // HACKHACK!
            //
            // For AF_INET sockets make sure the port we're binding to is
            // consistent with the destination port.  Unfortunately there's
            // no clean way to have the helper dll deal with this at this
            // time, so this is the expedient fix.
            //

            if (socket->AddressFamily == AF_INET)
            {
                ((struct sockaddr_in *) sockaddr)->sin_port =
                    ((struct sockaddr_in *) SocketAddress)->sin_port;
            }
            // same for AF_INET6
            else if (socket->AddressFamily == AF_INET6) {
                #include <ws2tcpip.h>
                ((struct sockaddr_in6 *) sockaddr)->sin6_port =
                    ((struct sockaddr_in6 *) SocketAddress)->sin6_port;
            }

            result = WSPBind(
                         Handle,
                         sockaddr,
                         sockaddrLength,
                         &err
                         );

            if (sockaddr!=(PVOID)localBuffer) {
                FREE_HEAP( sockaddr );
            }

            if( result == SOCKET_ERROR ) {

                WS_ASSERT (err!=NO_ERROR);
                goto exit;

            }

        } else if ( socket->State == SocketStateOpen ) {

            //
            // The socket is not bound and the helper DLL does not support
            // a wildcard socket address.  Fail, the app must bind manually.
            //

            err = WSAEINVAL;
            goto exit;

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

    //
    // Get the TDI handles for this socket.
    //

    err = SockGetTdiHandles( socket );

    if( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Conjur up a protocol info structure for the new socket. The only
    // field we really need for this is the dwCatalogEntryId, which we can
    // swipe from the incoming socket.
    //

    RtlZeroMemory(
        &dummyProtocolInfo,
        sizeof(dummyProtocolInfo)
        );

    dummyProtocolInfo.iAddressFamily = socket->AddressFamily;
    dummyProtocolInfo.iSocketType = socket->SocketType;
    dummyProtocolInfo.iProtocol = socket->Protocol;
    dummyProtocolInfo.dwCatalogEntryId = socket->CatalogEntryId;
    dummyProtocolInfo.dwServiceFlags1 = socket->ServiceFlags1;
    dummyProtocolInfo.dwProviderFlags = socket->ProviderFlags;

    //
    // If existing socket is a c_root then create a new socket that will
    // represent the multicast session.
    //

    if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT) {

        leafHandle = WSPSocket(
                         socket->AddressFamily,
                         socket->SocketType,
                         socket->Protocol,
                         &dummyProtocolInfo,
                         0,
                         socket->CreationFlags,
                         &err
                         );

        if( leafHandle == INVALID_SOCKET ) {

            WS_ASSERT( err != NO_ERROR );
            goto exit;

        }

        //
        // Find a pointer to the newly created socket.
        //

        leafSocket = SockFindAndReferenceSocket( leafHandle, FALSE );

        WS_ASSERT( leafSocket != NULL );
        SockAcquireSocketLockExclusive( leafSocket );
        //
        // Clone the properties of the root socket to the new socket.
        //

        leafSocket->LingerInfo = socket->LingerInfo;
        leafSocket->ReceiveBufferSize = socket->ReceiveBufferSize;
        leafSocket->SendBufferSize = socket->SendBufferSize;
        leafSocket->Broadcast = socket->Broadcast;
        leafSocket->Debug = socket->Debug;
        leafSocket->OobInline = socket->OobInline;
        leafSocket->ReuseAddresses = socket->ReuseAddresses;
        leafSocket->SendTimeout = socket->SendTimeout;
        leafSocket->ReceiveTimeout = socket->ReceiveTimeout;

        //
        // Set up the new socket to have the same blocking, inline, and
        // timeout characteristics as the listening socket.
        //

        if ( socket->NonBlocking ) {
            BOOLEAN nb = socket->NonBlocking;
            err = SockSetInformation(
                        leafSocket,
                        AFD_NONBLOCKING_MODE,
                        &nb,
                        NULL,
                        NULL
                        );
            if ( err != NO_ERROR ) {
                goto exit;
            }
        }

        leafSocket->NonBlocking = socket->NonBlocking;

        if ( socket->OobInline ) {
            BOOLEAN oob = socket->OobInline;
            err = SockSetInformation(
                        leafSocket,
                        AFD_INLINE_MODE,
                        &oob,
                        NULL,
                        NULL
                        );
            if ( err != NO_ERROR ) {
                goto exit;
            }
        }

        leafSocket->OobInline = socket->OobInline;

        //
        // If the listening socket has been called with WSAAsyncSelect,
        // set up WSAAsyncSelect on this socket.  Otherwise, if the socket
        // has been called with WSAEventSelect, set up WSAEventSelect on
        // the socket.
        //

        if ( socket->AsyncSelectlEvent ) {
            INT result;

            result = WSPAsyncSelect(
                         leafHandle,
                         socket->AsyncSelecthWnd,
                         socket->AsyncSelectwMsg,
                         socket->AsyncSelectlEvent,
                         &err
                         );

            if ( result == SOCKET_ERROR ) {

                goto exit;

            }

        } else if ( socket->EventSelectlNetworkEvents ) {
            INT result;

            result = WSPEventSelect(
                         leafHandle,
                         socket->EventSelectEventObject,
                         socket->EventSelectlNetworkEvents,
                         &err
                         );

            if ( result == SOCKET_ERROR ) {

                goto exit;

            }

        }

        //
        // If the application has modified the send or receive buffer sizes,
        // then set up the buffer sizes on the socket.
        //

        err = SockUpdateWindowSizes( leafSocket, FALSE );
        if ( err != NO_ERROR ) {
            goto exit;
        }

        //
        // Remember the changed state of this socket.
        //

        err = SockSetHandleContext( leafSocket );
        if ( err != NO_ERROR ) {
            goto exit;
        }

        err = socket->HelperDll->WSHJoinLeaf(
                  socket->HelperDllContext,
                  Handle,
                  socket->TdiAddressHandle,
                  socket->TdiConnectionHandle,
                  leafSocket->HelperDllContext,
                  leafHandle,
                  (PSOCKADDR)SocketAddress,
                  (DWORD)SocketAddressLength,
                  lpCallerData,
                  lpCalleeData,
                  lpSQOS,
                  lpGQOS,
                  dwFlags
                  );
    }
    else  {
        leafHandle = Handle;
        leafSocket = socket;
        err = socket->HelperDll->WSHJoinLeaf(
                  socket->HelperDllContext,
                  Handle,
                  socket->TdiAddressHandle,
                  socket->TdiConnectionHandle,
                  NULL,
                  INVALID_SOCKET,
                  (PSOCKADDR)SocketAddress,
                  (DWORD)SocketAddressLength,
                  lpCallerData,
                  lpCalleeData,
                  lpSQOS,
                  lpGQOS,
                  dwFlags
                  );
    }

    //
    // Let the helper DLL do the dirty work.
    //

    if (err!=NO_ERROR) {
        goto exit;
    }


    //
    // Disable the FD_CONNECT async select event before actually starting
    // the connect attempt--otherwise, the following could occur:
    //
    //     - call IOCTL to begin connect.
    //     - connect completes, async thread sets FD_CONNECT as disabled.
    //     - we reenable FD_CONNECT just before leaving this routine,
    //       but it shouldn't be enabled yet.
    //
    // Also disable FD_WRITE so that we don't get any FD_WRITE events
    // until after the socket is connected.
    //

    if ( (leafSocket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {

        leafSocket->DisabledAsyncSelectEvents |= FD_CONNECT | FD_WRITE;

        if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
            socket->DisabledAsyncSelectEvents |= FD_CONNECT;
    }

    //
    // Allocate and initialize the TDI request structure.
    //

    connectInfoLength = FIELD_OFFSET (AFD_CONNECT_JOIN_INFO, RemoteAddress) +
                           leafSocket->HelperDll->MaxTdiAddressLength;

    if (connectInfoLength<=sizeof (localBuffer)) {
        connectInfo = (PAFD_CONNECT_JOIN_INFO)localBuffer;
    }
    else {
        connectInfo = ALLOCATE_HEAP( connectInfoLength );

        if ( connectInfo == NULL ) {

            err = WSAENOBUFS;
            goto exit;

        }
    }

    //
    // Pass root socket handle if this is C_Root inviting leaf
    //
    if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
        connectInfo->RootEndpoint = socket->HContext.Handle;
    else
        connectInfo->RootEndpoint = NULL;

    err = SockBuildTdiAddress(
        &connectInfo->RemoteAddress,
        (PSOCKADDR)SocketAddress,
        SocketAddressLength
        );
    if (err!=NO_ERROR) {
        goto exit;
    }

    //
    // Save the name of the group we are joining to.
    // This lets getpeername work (WSPSend (as opposed to WSPSendTo)
    // is already handled by AFD).
    //

    RtlCopyMemory(
        leafSocket->RemoteAddress,
        SocketAddress,
        SocketAddressLength
        );

    leafSocket->RemoteAddressLength = SocketAddressLength;

    //
    // If this is a nonblocking socket, perform the connect asynchronously.
    // Datagram connects are done in this thread, since they are fast
    // and don't require network activity.
    //

    if ( leafSocket->NonBlocking && !IS_DGRAM_SOCK(leafSocket) ) {
        PSOCK_ASYNC_CONNECT_CONTEXT ConnectContext;
        //
        // Initialize the async thread if it hasn't already been started.
        //

        if ( !SockCheckAndReferenceAsyncThread( ) ) {
            err = WSAENOBUFS;
            goto exit;
        }
        if ( !SockCheckAndInitAsyncConnectHelper ()) {
            SockDereferenceAsyncThread ();
            err = WSAENOBUFS;
            goto exit;
        }

        //
        // Allocate connect context
        //
        ConnectContext = ALLOCATE_HEAP (sizeof (*ConnectContext) 
                                            + connectInfoLength);
        if (ConnectContext==NULL) {
            SockDereferenceAsyncThread ();
            err = WSAENOBUFS;
            goto exit;
        }


        RtlCopyMemory  (&ConnectContext->ConnectInfo,
                            connectInfo,
                            connectInfoLength);

        if (connectInfo!=(PAFD_CONNECT_JOIN_INFO)localBuffer) {
            FREE_HEAP (connectInfo);
            connectInfo = NULL;
        }
        ConnectContext->ConnectInfoLength = connectInfoLength;
        ConnectContext->Socket = leafSocket;
        if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT) {
            ASSERT (leafSocket!=socket);
            ConnectContext->RootSocket = socket;
        }
        else {
            ASSERT (leafSocket==socket);
            ConnectContext->RootSocket = NULL;
        }
        ConnectContext->IoctlCode = IOCTL_AFD_JOIN_LEAF;

        //
        // We use helper handle to get the connect request to
        // afd, pass real socket handle as the parameter in 
        // tdi request.
        //
        ConnectContext->ConnectInfo.ConnectEndpoint = leafSocket->HContext.Handle;

        do {

            //
            // Now, submit the connect request to AFD, specifying an APC
            // completion routine that will handle finishing up the request.
            //

            ConnectContext->IoStatusBlock.Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         SockAsyncConnectHelperHandle,
                         NULL,
                         NULL,
                         ConnectContext,
                         &ConnectContext->IoStatusBlock,
                         IOCTL_AFD_JOIN_LEAF,
                         &ConnectContext->ConnectInfo,
                         ConnectContext->ConnectInfoLength,
                         &ConnectContext->IoStatusBlock,
                         sizeof (ConnectContext->IoStatusBlock)
                         );


            if ( NT_SUCCESS(status) ) {
                leafSocket->AsyncConnectContext = ConnectContext;
                //
                // Release the socket lock--we don't need it any longer.
                //

                SockReleaseSocketLock( leafSocket );

                //
                // Release lock of the root socket if any
                //
                if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT) {
                    socket->AsyncConnectContext = ConnectContext;
                    SockReleaseSocketLock (socket);
                }

                //
                // Now just return.  The completion APC will handle rest of
                // the cleanup.
                //

                WS_EXIT( "WSPJoinLeaf", (INT)leafHandle, FALSE );
                return leafHandle;
            }


            //
            // If the request failed immediately, handle the error inline
            // to avoid recursion/unnecessary async select and error code
            // issues.
            //

            //
            // See if socket in process of being closed
            //

            if ( socket->State == SocketStateClosing ) {

                err = WSAENOTSOCK;
                goto exit;

            }

            //
            // If the connect attempt failed, notify the helper DLL as
            // appropriate.  This allows the helper DLL to do a dialin if a
            // RAS-style link is appropriate.
            //

            if ( !NT_SUCCESS(status)) {

                err = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT_ERROR );

            }

        } while ( err == WSATRY_AGAIN );

    }
    else {

        //
        // Call AFD to perform the actual connect operation.
        //

        do {
            ioStatusBlock.Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         leafSocket->HContext.Handle,
                         tlsData->EventHandle,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_JOIN_LEAF,
                         connectInfo,
                         connectInfoLength,
                         NULL,
                         0
                         );

            if ( status == STATUS_PENDING ) {

                //
                // Wait for the connect to complete.
                //

                SockReleaseSocketLock( leafSocket );
                if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
                    SockReleaseSocketLock (socket);

                SockWaitForSingleObject(
                    tlsData->EventHandle,
                    leafHandle,
                    SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                    SOCK_NO_TIMEOUT
                    );
                status = ioStatusBlock.Status;
                if (socket->CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
                    SockAcquireSocketLockExclusive (socket);
                SockAcquireSocketLockExclusive( leafSocket );
            }

            //
            // See if socket in process of being closed
            //

            if ( leafSocket->State == SocketStateClosing ) {

                err = WSAENOTSOCK;
                goto exit;

            }

            //
            // If the join attempt failed, notify the helper DLL as
            // appropriate.  This allows the helper DLL to do a dialin if a
            // RAS-style link is appropriate.
            //

            if ( !NT_SUCCESS(status) && leafSocket->State != SocketStateClosing ) {

                err = SockNotifyHelperDll( leafSocket, WSH_NOTIFY_CONNECT_ERROR );

            }

        } while ( err == WSATRY_AGAIN );
    }

    if ( !NT_SUCCESS(status) ) {

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    WS_ASSERT( status != STATUS_TIMEOUT );

    //
    // Finish up processing the connect.
    //

    err = SockPostProcessConnect( leafSocket );

    if( err != NO_ERROR ) {

        goto exit;

    }

    __try {
        //
        // If we have a buffer for inbound connect data, retrieve it
        // from the socket.
        //

        if( lpCalleeData != NULL &&
            lpCalleeData->buf != NULL &&
            lpCalleeData->len > 0 ) {

            //
            // Get the connect data from the socket.
            //

            err = SockGetConnectData(
                      leafSocket,
                      IOCTL_AFD_GET_CONNECT_DATA,
                      (PCHAR)lpCalleeData->buf,
                      (INT)lpCalleeData->len,
                      (PINT)&lpCalleeData->len
                      );

            if( err == NO_ERROR ) {

                if( lpCalleeData->len == 0 ) {
                    lpCalleeData->buf = NULL;
                }

            } else {

                //
                // We'll cheat a bit here and pretend we got no callee data.
                //

                lpCalleeData->len = 0;
                lpCalleeData->buf = NULL;
                err = NO_ERROR;

            }

        }

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }
exit:

    IF_DEBUG(MISC) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPJoinLeaf on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPJoinLeaf on socket %lx (%lx) succeeded\n",
                           Handle, socket ));

        }

    }

    if ( connectInfo!=NULL && connectInfo != (PAFD_CONNECT_JOIN_INFO)localBuffer ) {
        FREE_HEAP( connectInfo );
    }

    if ( leafSocket != NULL ) {

        if (err==NO_ERROR && leafSocket->NonBlocking) {
            //
            // Datagram socket needs to notify when async select
            // has completed.  If error occured, the caller will
            // know not to wait for the message.  Connection oriented
            // sockets are handled in the async thread.
            //
            BOOL posted;
            ASSERT (IS_DGRAM_SOCK(leafSocket));
            posted = (SockUpcallTable->lpWPUPostMessage)(
                         leafSocket->AsyncSelecthWnd,
                         leafSocket->AsyncSelectwMsg,
                         (WPARAM)leafSocket->Handle,
                         WSAMAKESELECTREPLY( FD_CONNECT, err )
                         );

            //WS_ASSERT( posted );

            IF_DEBUG(POST) {

                WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                               leafSocket->AsyncSelectwMsg,
                               leafSocket->AsyncSelecthWnd,
                               leafSocket->Handle, "FD_CONNECT", err ));

            }
        }
        
        //
        // If the socket is waiting for FD_WRITE messages, reenable them
        // now.  They were disabled in WSAAsyncSelect() because we don't
        // want to post FD_WRITE messages before FD_CONNECT messages.
        //

        if ( (leafSocket->AsyncSelectlEvent & FD_WRITE) != 0 ) {

            SockReenableAsyncSelectEvent( leafSocket, FD_WRITE );

        }

        SockReleaseSocketLock( leafSocket );
        SockDereferenceSocket( leafSocket );

    }

    if( (socket != NULL) && (leafSocket!=socket) ) {
        if (err==NO_ERROR) {
            if (socket->State!=SocketStateConnected) {
                ASSERT (socket->State==SocketStateBound);
                socket->State = SocketStateConnected;
            }

            if (socket->NonBlocking) {
                //
                // Datagram socket needs to notify when async select
                // has completed.  If error occured, the caller will
                // know not to wait for the message.  Connection oriented
                // sockets are handled in the async thread.
                //
                BOOL posted;
                ASSERT (IS_DGRAM_SOCK(socket));
                posted = (SockUpcallTable->lpWPUPostMessage)(
                             socket->AsyncSelecthWnd,
                             socket->AsyncSelectwMsg,
                             (WPARAM)socket->Handle,
                             WSAMAKESELECTREPLY( FD_CONNECT, err )
                             );

                //WS_ASSERT( posted );

                IF_DEBUG(POST) {

                    WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                                   socket->AsyncSelectwMsg,
                                   socket->AsyncSelecthWnd,
                                   socket->Handle, "FD_CONNECT", err ));

                }
            }
        }

        //
        // If the socket is waiting for FD_WRITE messages, reenable them
        // now.  They were disabled in WSAAsyncSelect() because we don't
        // want to post FD_WRITE messages before FD_CONNECT messages.
        //

        if ( (socket->AsyncSelectlEvent & FD_WRITE) != 0 ) {

            SockReenableAsyncSelectEvent( socket, FD_WRITE );

        }

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( err != NO_ERROR ) {

        if( (leafHandle != INVALID_SOCKET) 
                && (leafHandle!=Handle)) {

            INT dummy;

            WSPCloseSocket(
                leafHandle,
                &dummy
                );

        }

        WS_EXIT( "WSPJoinLeaf", INVALID_SOCKET, TRUE );
        *lpErrno = err;
        return INVALID_SOCKET;

    }

    WS_EXIT( "WSPJoinLeaf", (INT)leafHandle, FALSE );
    return leafHandle;

}   // WSPJoinLeaf


int
SockDoConnect (
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength,
    IN LPWSABUF lpCallerData,
    IN LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS
    )
{
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    int err;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG connectInfoLength;
#ifdef _AFD_SAN_SWITCH_
    PAFD_SAN_CONNECT_JOIN_INFO  connectInfo;
    UCHAR localBuffer[MAX_FAST_TDI_ADDRESS
                +FIELD_OFFSET(AFD_SAN_CONNECT_JOIN_INFO, RemoteAddress)];
#else //_AFD_SAN_SWITCH_
    PAFD_CONNECT_JOIN_INFO  connectInfo;
    UCHAR localBuffer[MAX_FAST_TDI_ADDRESS
                +FIELD_OFFSET(AFD_CONNECT_JOIN_INFO, RemoteAddress)];
#endif //_AFD_SAN_SWITCH_

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        return err;

    }

    //
    // Set up local variables so that we know how to clean up on exit.
    //

    socket = NULL;
    connectInfo = NULL;


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
    // performing operations on the socket we're connecting.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // The only valid socket states for this API are Open (only socket(
    // ) was called) and Bound (socket( ) and bind( ) were called).
    // Note that it is legal to reconnect a datagram socket.
    //

    if ( socket->State == SocketStateConnected &&
             !IS_DGRAM_SOCK(socket) ) {

        err = WSAEISCONN;
        goto exit;

    }

#ifdef _AFD_SAN_SWITCH_
	if (SockSanEnabled && socket->SanSocket) {
		DWORD isConnected = socket->SanSocket->IsConnected;	// capture
		switch (isConnected) {

		case 0:
			break;

		case CONNECTED:
			 err = WSAEISCONN;
			 goto exit;

		default:
			WS_ASSERT(isConnected == NON_BLOCKING_IN_PROGRESS ||
					  isConnected == BLOCKING_IN_PROGRESS);
			err = WSAEALREADY;
            goto exit;
		}
	}
#endif //_AFD_SAN_SWITCH_
			

    if( socket->AsyncConnectContext) {
        //
        // If socket connect has just completed with failure,
        // the AsyncConnectContext is not cleared until
        // post connect processing completes in the
        // async thread.  The function below will wait
        // for this completion.  It will only do the wait
        // if connect has actually completed and just
        // the post-processing part remains to be done.
        //
        SockIsSocketConnected (socket);
        if (socket->AsyncConnectContext) {
            err = WSAEALREADY;
            goto exit;
        }
    }

    if ( socket->State != SocketStateOpen  &&
             socket->State != SocketStateBound &&
             socket->State != SocketStateConnected ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // If the socket address is too long, truncate to the max possible
    // length.  If we didn't do this, the allocation below for the max
    // TDI address length would be insufficient and SockBuildSockaddr()
    // would overrun the allocated buffer.
    //

    if ( SocketAddressLength > socket->HelperDll->MaxSockaddrLength ) {

        SocketAddressLength = socket->HelperDll->MaxSockaddrLength;

    }

    //
    // If the socket address is too short, fail.
    //

    if ( SocketAddressLength < socket->HelperDll->MinSockaddrLength ) {

        err = WSAEFAULT;
        goto exit;

    }


    __try {
        //
        // If this is a connected datagram socket and the caller has
        // specified an address equal to all zeros, then this is a request
        // to "unconnect" the socket.
        //

        if ( socket->State == SocketStateConnected &&
                 IS_DGRAM_SOCK(socket) &&
                 IsSockaddrEqualToZero( SocketAddress, SocketAddressLength ) ) {

            err = UnconnectDatagramSocket( socket );
            goto exit;

        }

        //
        // Make sure that the address family passed in here is the same as
        // was passed in on the socket( ) call.
        //

        if ( socket->AddressFamily != SocketAddress->sa_family ) {

            err = WSAEAFNOSUPPORT;
            goto exit;

        }


        if (IS_DGRAM_SOCK(socket) && !socket->Broadcast) {
            SOCKADDR_INFO sockaddrInfo;

            //
            // Determine the type of the sockaddr.
            //

            err = socket->HelperDll->WSHGetSockaddrType(
                        (PSOCKADDR)SocketAddress,
                        SocketAddressLength,
                        &sockaddrInfo
                        );

            if ( err == NO_ERROR ) {

                if ( sockaddrInfo.AddressInfo == SockaddrAddressInfoBroadcast ) {
                    //
                    // Backward compatibility problem.
                    //
                    DbgPrint ("MSAFD, process:%ld - SO_BROADCAST must be set before connecting\n"
                              "     a datagram socket to broadcast address - see Winsock2 API specification.\n",
                              GetCurrentProcessId ());
                    err = WSAEACCES;
                }
            }
            //
            // Ignore whatever error we may have encountered.
            //
            err = NO_ERROR;
        }


        //
        // If this socket belongs to a constrained group, then search all
        // open sockets for one belonging to this same group and verify
        // the target addresses match.
        //

        if( socket->GroupType == GroupTypeConstrained &&
            ((err=SockIsAddressConsistentWithConstrainedGroup(
                socket,
                socket->GroupID,
                (PSOCKADDR)SocketAddress,
                SocketAddressLength
                ))!=NO_ERROR) ) {

            goto exit;

        }

        //
        // If this socket is not yet bound to an address, bind it to an
        // address.  We only do this if the helper DLL for the socket supports
        // a get wildcard address routine--if it doesn't, the app must bind
        // to an address manually.
        //

        if ( socket->State == SocketStateOpen &&
                 socket->HelperDll->WSHGetWildcardSockaddr != NULL ) {

            PSOCKADDR sockaddr;
            INT sockaddrLength = socket->HelperDll->MaxSockaddrLength;
            int result;

            sockaddr = ALLOCATE_HEAP( sockaddrLength );

            if ( sockaddr == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

            err = socket->HelperDll->WSHGetWildcardSockaddr(
                        socket->HelperDllContext,
                        sockaddr,
                        &sockaddrLength
                        );

            if ( err != NO_ERROR ) {

                FREE_HEAP( sockaddr );
                goto exit;

            }

            result = WSPBind(
                         Handle,
                         sockaddr,
                         sockaddrLength,
                         &err
                         );

            FREE_HEAP( sockaddr );

            if( result == SOCKET_ERROR ) {

                goto exit;

            }

        } else if ( socket->State == SocketStateOpen ) {

            //
            // The socket is not bound and the helper DLL does not support
            // a wildcard socket address.  Fail, the app must bind manually.
            //

            err = WSAEINVAL;
            goto exit;

        }

        //
        // If we have outbound connect data, set it on the socket.
        //

        if( lpCallerData != NULL &&
            lpCallerData->buf != NULL &&
            lpCallerData->len > 0 ) {

            INT bufferLength;

            //
            // Set the connect data on the socket.
            //

            bufferLength = (INT)lpCallerData->len;

            err = SockSetConnectData(
                      socket,
                      IOCTL_AFD_SET_CONNECT_DATA,
                      (PCHAR)lpCallerData->buf,
                      bufferLength,
                      &bufferLength
                      );

            if( err != NO_ERROR ) {

                goto exit;

            }

        }

        if (socket->ServiceFlags1 & XP1_QOS_SUPPORTED) {
            int result;
            DWORD   bytesReturned;

            if (lpSQOS) {
                result = WSPIoctl (Handle, SIO_SET_QOS, 
                                    lpSQOS, sizeof (*lpSQOS),
                                    NULL, 0,
                                    &bytesReturned,
                                    NULL, NULL, NULL, &err);
                if( result == SOCKET_ERROR ) {

                    goto exit;

                }
            }

            if (lpGQOS) {
                result = WSPIoctl (Handle, SIO_SET_GROUP_QOS, 
                                    lpGQOS, sizeof (*lpGQOS),
                                    NULL, 0,
                                    &bytesReturned,
                                    NULL, NULL, NULL, &err);
                if( result == SOCKET_ERROR ) {

                    goto exit;

                }
            }
        }

        //
        // Disable the FD_CONNECT async select event before actually starting
        // the connect attempt--otherwise, the following could occur:
        //
        //     - call IOCTL to begin connect.
        //     - connect completes, async thread sets FD_CONNECT as disabled.
        //     - we reenable FD_CONNECT just before leaving this routine,
        //       but it shouldn't be enabled yet.
        //
        // Also disable FD_WRITE so that we don't get any FD_WRITE events
        // until after the socket is connected.
        //

        if ( (socket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {

            socket->DisabledAsyncSelectEvents |= FD_CONNECT | FD_WRITE;

        }

        //
        // Allocate and initialize the AFD connect request structure.
        //

#ifdef _AFD_SAN_SWITCH_
        connectInfoLength = FIELD_OFFSET (AFD_SAN_CONNECT_JOIN_INFO, RemoteAddress)
#else //_AFD_SAN_SWITCH_
        connectInfoLength = FIELD_OFFSET (AFD_CONNECT_JOIN_INFO, RemoteAddress)
#endif //_AFD_SAN_SWITCH_
                                + socket->HelperDll->MaxTdiAddressLength;

        if (connectInfoLength<=sizeof (localBuffer)) {
#ifdef _AFD_SAN_SWITCH_
            connectInfo = (PAFD_SAN_CONNECT_JOIN_INFO)localBuffer;
#else //_AFD_SAN_SWITCH_
            connectInfo = (PAFD_CONNECT_JOIN_INFO)localBuffer;
#endif //_AFD_SAN_SWITCH_
        }
        else {
            connectInfo = ALLOCATE_HEAP( connectInfoLength );

            if ( connectInfo == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }
        }


        err = SockBuildTdiAddress(
            &connectInfo->RemoteAddress,
            (PSOCKADDR)SocketAddress,
            SocketAddressLength
            );

        if (err!=NO_ERROR) {

            goto exit;

        }

        //
        // Save the name of the server we're connecting to.
        //

        RtlCopyMemory(
            socket->RemoteAddress,
            SocketAddress,
            SocketAddressLength
            );

        socket->RemoteAddressLength = SocketAddressLength;

        //
        // If the user is expecting connect data back, then set a default
        // connect data buffer in AFD. It would be preferrable if we could
        // allocate that buffer on demand, but TDI does not allow for it.
        //

        if( lpCalleeData != NULL &&
            lpCalleeData->buf != NULL &&
            lpCalleeData->len > 0 ) {

            ULONG length;

            length = (ULONG)lpCalleeData->len;

            err = SockSetConnectData(
                      socket,
                      IOCTL_AFD_SIZE_CONNECT_DATA,
                      (PCHAR)&length,
                      sizeof(length),
                      NULL
                      );

            if( err != NO_ERROR ) {

                goto exit;

            }

        }
	}
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

#ifdef _AFD_SAN_SWITCH_
retrysan:
    if (SockSanEnabled && socket->CatalogEntryId==SockTcpProviderInfo.dwCatalogEntryId) {
		if (socket->DontTrySAN) {
			socket->DontTrySAN = FALSE;	// reset
		}
		else {
			if (SockSanConnect (socket,
								connectInfo,
								&err)) {
				//
				// Blocking connect succeeded or non-blocking connect successfully
				// initiated.
				//
				goto exit;
			}
			else if (err == WSAECONNRESET ||
					 err == WSAECONNREFUSED ||
					 err == WSAEHOSTUNREACH ||
					 TcpBypassFlag) {
				//
				// No connection at all is possible to remote address
				//
				goto exit;
			}
			else {
				//
				// Try thru TCP
				//
				err = 0;
			}
		}
    }
#endif //_AFD_SAN_SWITCH_

#ifdef _AFD_SAN_SWITCH_
    connectInfo->SanActive = SockSanEnabled;
#endif //_AFD_SAN_SWITCH_
    //
    // If this is a nonblocking socket, perform the connect asynchronously.
    // Datagram connects are done in this thread, since they are fast
    // and don't require network activity.
    //

    if ( socket->NonBlocking && !IS_DGRAM_SOCK(socket) ) {
        PSOCK_ASYNC_CONNECT_CONTEXT ConnectContext;
        //
        // Initialize the async thread if it hasn't already been started.
        //

        if ( !SockCheckAndReferenceAsyncThread( ) ) {
            err = WSAENOBUFS;
            goto exit;
        }
        
        if (!SockCheckAndInitAsyncConnectHelper ()) {
            SockDereferenceAsyncThread ();
            err = WSAENOBUFS;
            goto exit;
        }

        //
        // Allocate connect context
        //
        ConnectContext = ALLOCATE_HEAP (sizeof (*ConnectContext)+
                                            connectInfoLength);

        if (ConnectContext==NULL) {
            SockDereferenceAsyncThread ();
            err = WSAENOBUFS;
            goto exit;
        }

        RtlCopyMemory  (&ConnectContext->ConnectInfo,
                            connectInfo,
                            connectInfoLength);

#ifdef _AFD_SAN_SWITCH_
        if (connectInfo!=(PAFD_SAN_CONNECT_JOIN_INFO)localBuffer) {
#else //_AFD_SAN_SWITCH_
        if (connectInfo!=(PAFD_CONNECT_JOIN_INFO)localBuffer) {
#endif //_AFD_SAN_SWITCH_
            FREE_HEAP (connectInfo);
            connectInfo = NULL;
        }

        ConnectContext->ConnectInfoLength = connectInfoLength;
        ConnectContext->Socket = socket;
        ConnectContext->RootSocket = NULL;
        //
        // We use helper handle to get the connect request to
        // afd, pass real socket handle as the parameter in 
        // tdi request.
        //
        ConnectContext->ConnectInfo.ConnectEndpoint = socket->HContext.Handle;
        ConnectContext->IoctlCode = IOCTL_AFD_CONNECT;

        do {

            //
            // Now, submit the connect request to AFD, specifying an APC
            // completion routine that will handle finishing up the request.
            //

            ConnectContext->IoStatusBlock.Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         SockAsyncConnectHelperHandle,
                         NULL,
                         NULL,
                         ConnectContext,
                         &ConnectContext->IoStatusBlock,
                         IOCTL_AFD_CONNECT,
                         &ConnectContext->ConnectInfo,
                         ConnectContext->ConnectInfoLength,
                         &ConnectContext->IoStatusBlock,
                         sizeof (ConnectContext->IoStatusBlock)
                         );


            if ( NT_SUCCESS(status) ) {
                socket->AsyncConnectContext = ConnectContext;
                //
                // Release the socket lock--we don't need it any longer.
                //

                SockReleaseSocketLock( socket );

                //
                // Now just return.  The completion APC will handle all cleanup.
                //
                return WSAEWOULDBLOCK;
            }

            //
            // If the request failed immediately, handle the error inline
            // to avoid recursion/unnecessary async select and error code
            // issues.
            //

            //
            // See if socket in process of being closed
            //

            if ( socket->State == SocketStateClosing ) {

                status = STATUS_INVALID_HANDLE;
                break;

            }

#ifdef _AFD_SAN_SWITCH_
            if (status==STATUS_INVALID_PARAMETER_12) {
                break;
            }
#endif //_AFD_SAN_SWITCH_

            //
            // If the connect attempt failed, notify the helper DLL as
            // appropriate.  This allows the helper DLL to do a dialin if a
            // RAS-style link is appropriate.
            //

            if ( !NT_SUCCESS(status)) {

                err = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT_ERROR );

            }

        } while ( err == WSATRY_AGAIN );

        FREE_HEAP (ConnectContext);

    }
    else {


        //
        // Call AFD to perform the actual connect operation.
        //

        do {

            ioStatusBlock.Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         socket->HContext.Handle,
                         tlsData->EventHandle,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_CONNECT,
                         connectInfo,
                         connectInfoLength,
                         NULL,
                         0
                         );

            if ( status == STATUS_PENDING ) {

                //
                // Wait for the connect to complete.
                //

                SockReleaseSocketLock( socket );

                SockWaitForSingleObject(
                    tlsData->EventHandle,
                    Handle,
                    SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                    SOCK_NO_TIMEOUT
                    );
                status = ioStatusBlock.Status;
                SockAcquireSocketLockExclusive( socket );

            }

            //
            // See if socket in process of being closed
            //

            if ( socket->State == SocketStateClosing ) {

                err = WSAENOTSOCK;
                goto exit;

            }

#ifdef _AFD_SAN_SWITCH_
            if (status==STATUS_INVALID_PARAMETER_12) {
                break;
            }
#endif //_AFD_SAN_SWITCH_

            //
            // If the connect attempt failed, notify the helper DLL as
            // appropriate.  This allows the helper DLL to do a dialin if a
            // RAS-style link is appropriate.
            //

            if ( !NT_SUCCESS(status)) {

                err = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT_ERROR );

            }

        } while ( err == WSATRY_AGAIN );
    }


    if ( !NT_SUCCESS(status) ) {
#ifdef _AFD_SAN_SWITCH_
        if (status==STATUS_INVALID_PARAMETER_12) {
            WS_ASSERT (connectInfo->SanActive==FALSE);
            SockReleaseSocketLock( socket );
            status = SockSanActivate ();
            SockAcquireSocketLockExclusive ( socket );
            if (NT_SUCCESS (status))
                goto retrysan;
        }
#endif //_AFD_SAN_SWITCH_

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    WS_ASSERT( status != STATUS_TIMEOUT );

    //
    // Finish up processing the connect.
    //

    err = SockPostProcessConnect( socket );

    if( err != NO_ERROR ) {

        goto exit;

    }

    __try {

        //
        // If we have a buffer for inbound connect data, retrieve it
        // from the socket.
        //

        if( lpCalleeData != NULL &&
            lpCalleeData->buf != NULL &&
            lpCalleeData->len > 0 ) {

            //
            // Get the connect data from the socket.
            //

            err = SockGetConnectData(
                      socket,
                      IOCTL_AFD_GET_CONNECT_DATA,
                      (PCHAR)lpCalleeData->buf,
                      (INT)lpCalleeData->len,
                      (PINT)&lpCalleeData->len
                      );

            if( err == NO_ERROR ) {

                if( lpCalleeData->len == 0 ) {
                    lpCalleeData->buf = NULL;
                }

            } else {

                //
                // We'll cheat a bit here and pretend we got no callee data.
                //

                lpCalleeData->len = 0;
                lpCalleeData->buf = NULL;
                err = NO_ERROR;

            }

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

exit:

    IF_DEBUG(CONNECT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPConnect on socket %lx (%lx) failed: %ld\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPConnect on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

#ifdef _AFD_SAN_SWITCH_
    if ( connectInfo!=NULL && connectInfo != (PAFD_SAN_CONNECT_JOIN_INFO)localBuffer ) {
#else //_AFD_SAN_SWITCH_
    if ( connectInfo!=NULL && connectInfo != (PAFD_CONNECT_JOIN_INFO)localBuffer ) {
#endif //_AFD_SAN_SWITCH_
        FREE_HEAP( connectInfo );
    }
    //
    // Perform cleanup--dereference the socket if it was referenced,
    // free allocated resources.
    //

    if ( socket != NULL ) {

        //
        // If the socket is waiting for FD_WRITE messages, reenable them
        // now.  They were disabled in WSAAsyncSelect() because we don't
        // want to post FD_WRITE messages before FD_CONNECT messages.
        // Only do this if connect succeded.
        //

        if ( (err==NO_ERROR) &&
                 ((socket->AsyncSelectlEvent & FD_WRITE) != 0) ) {

            SockReenableAsyncSelectEvent( socket, FD_WRITE );

        }


        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    //
    // Return an error if appropriate.
    //

    return err;

} // SockDoConnect


BOOLEAN
IsSockaddrEqualToZero (
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    )
{

    int i;

    for ( i = 0; i < SocketAddressLength; i++ ) {

        if ( *((PCHAR)SocketAddress + i) != 0 ) {

            return FALSE;

        }

    }

    return TRUE;

} // IsSockaddrEqualToZero


INT
UnconnectDatagramSocket (
    IN PSOCKET_INFORMATION Socket
    )
{

    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
	PWINSOCK_TLS_DATA	tlsData;
    NTSTATUS status;
	tlsData = GET_THREAD_DATA ();

    //
    // *** This routine assumes that it is called with the socket
    //     referenced and the socket's lock held exclusively!
    //

    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );
    disconnectInfo.DisconnectMode = AFD_UNCONNECT_DATAGRAM;

    IF_DEBUG(CONNECT) {

        WS_PRINT(( "unconnecting datagram socket %lx(%lx)\n",
                       Socket->HContext.Handle, Socket ));

    }

    //
    // Send the IOCTL to AFD for processing.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_PARTIAL_DISCONNECT,
                 &disconnectInfo,
                 sizeof(disconnectInfo),
                 NULL,                      // OutputBuffer
                 0L                         // OutputBufferLength
                 );

    if ( status == STATUS_PENDING ) {

        SockReleaseSocketLock( Socket );
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( Socket );
        status = ioStatusBlock.Status;

    }

    if ( !NT_SUCCESS(status) ) {

        error = SockNtStatusToSocketError( status );

    }  else {

        //
        // The socket is now unconnected.
        //
        // !!! do we need to call SockSetHandleContext() for this socket?

        error = NO_ERROR;
        Socket->State = SocketStateBound;

    }

    return error;

} // UnconnectDatagramSocket




VOID
SockAsyncConnectCompletion (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
{
    PSOCK_ASYNC_CONNECT_CONTEXT connectContext;
    PSOCKET_INFORMATION socket, rootSocket;
    NTSTATUS status;
    INT err;
    BOOL posted;

    //
    // Initialize locals.
    //

    connectContext = ApcContext;
    socket = connectContext->Socket;
    rootSocket = connectContext->RootSocket;
    status = connectContext->IoStatusBlock.Status;


    if (rootSocket!=NULL) {
        //
        // Acquire the lock to synchronize access to the socket.
        // Root socket is always first to avoid deadlock.
        //

        SockAcquireSocketLockExclusive( rootSocket );
    }

    //
    // Acquire the lock to synchronize access to the socket.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If the connect attempt failed, notify the helper DLL as
    // appropriate.  This allows the helper DLL to do a dialin if a
    // RAS-style link is appropriate.
    //

    if ( !NT_SUCCESS(status) && socket->State != SocketStateClosing ) {

        if (status==STATUS_CANCELLED) {
            //
            // If request was cancelled, it is most likely due to the
            // thread exit (socket is not closed!!!).
            // Resubmit the request in our thread.
            //
            err = WSATRY_AGAIN;
        }
        else {
            err = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT_ERROR );
        }

        //
        // If requested by the helper DLL, resubmit the connect request
        // to AFD.
        //

        if ( err == WSATRY_AGAIN ) {

            WS_ASSERT (connectContext->IoctlCode == IOCTL_AFD_JOIN_LEAF
                        || connectContext->IoctlCode == IOCTL_AFD_CONNECT);
            connectContext->IoStatusBlock.Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         SockAsyncConnectHelperHandle,
                         NULL,
                         NULL,
                         connectContext,
                         &connectContext->IoStatusBlock,
                         connectContext->IoctlCode,
                         &connectContext->ConnectInfo,
                         connectContext->ConnectInfoLength,
                         &connectContext->IoStatusBlock,
                         sizeof (connectContext->IoStatusBlock)
                         );

            SockReleaseSocketLock( socket );

            if (rootSocket!=NULL) {
                SockReleaseSocketLock( rootSocket );
            }

            //
            // If the request failed immediately, call the completion
            // routine directly to perform cleanup.
            //

            if ( NT_ERROR(status) ) {

                connectContext->IoStatusBlock.Status = status;

                SockAsyncConnectCompletion(
                    connectContext,
                    &connectContext->IoStatusBlock
                    );

            }

            return;
        }
    }

    //
    // If the connect failed, bail now.
    //

    if( socket->State == SocketStateClosing ) {

        err = WSAENOTSOCK;
        goto exit;

    }

    if ( !NT_SUCCESS(status) ) {

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    WS_ASSERT( status != STATUS_TIMEOUT );

    //
    // Finish up processing the connect.
    //

    err = SockPostProcessConnect( socket );

exit:

    IF_DEBUG(CONNECT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "%s(async) on socket %lx (%lx) failed: %ld\n",
                           (connectContext->IoctlCode==IOCTL_AFD_CONNECT)
                                ? "WSPConnect"
                                : "WSPJoinLeaf",
                           socket->HContext.Handle, socket, err ));

        } else {

            WS_PRINT(( "%s(async) on socket %lx (%lx) succeeded.\n",
                           (connectContext->IoctlCode==IOCTL_AFD_CONNECT)
                                ? "WSPConnect"
                                : "WSPJoinLeaf",
                           socket->HContext.Handle, socket ));

        }

    }


    socket->LastError = err;

    //
    // Indicate there there is no longer a connect in progress on the
    // socket.
    //

    WS_ASSERT (socket->AsyncConnectContext==connectContext);
    socket->AsyncConnectContext = NULL;

    //
    // If the app has requested FD_CONNECT events, post an
    // appropriate FD_CONNECT message.
    //

    if (rootSocket==NULL) {
        //
        // Signal FD_CONNECT only on one socket.
        //
        if ( (socket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {

            posted = (SockUpcallTable->lpWPUPostMessage)(
                         socket->AsyncSelecthWnd,
                         socket->AsyncSelectwMsg,
                         (WPARAM)socket->Handle,
                         WSAMAKESELECTREPLY( FD_CONNECT, err )
                         );

            //WS_ASSERT( posted );

            IF_DEBUG(POST) {

                WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                               socket->AsyncSelectwMsg,
                               socket->AsyncSelecthWnd,
                               socket->Handle, "FD_CONNECT", err ));

            }
        }
    }
    else {

        WS_ASSERT (rootSocket->AsyncConnectContext==connectContext);
        rootSocket->AsyncConnectContext = NULL;

        if ( (rootSocket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {

            posted = (SockUpcallTable->lpWPUPostMessage)(
                         rootSocket->AsyncSelecthWnd,
                         rootSocket->AsyncSelectwMsg,
                         (WPARAM)rootSocket->Handle,
                         WSAMAKESELECTREPLY( FD_CONNECT, err )
                         );

            //WS_ASSERT( posted );

            IF_DEBUG(POST) {

                WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                               rootSocket->AsyncSelectwMsg,
                               rootSocket->AsyncSelecthWnd,
                               rootSocket->Handle, "FD_CONNECT", err ));

            }

        }
    }

    //
    // If the socket is waiting for FD_WRITE messages, reenable them
    // now.  They were disabled in WSAAsyncSelect() because we don't
    // want to post FD_WRITE messages before FD_CONNECT messages.
    //

    if ( (socket->AsyncSelectlEvent & FD_WRITE) != 0 ) {

        SockReenableAsyncSelectEvent( socket, FD_WRITE );

    }



    SockReleaseSocketLock( socket );
    SockDereferenceSocket( socket );

    if (rootSocket!=NULL) {
        if (err==NO_ERROR) {
            if (rootSocket->State!=SocketStateConnected) {
                //
                // Change the socket state and signal FD_WRITE 
                // for the first leaf only
                //
                ASSERT (rootSocket->State==SocketStateBound);
                rootSocket->State = SocketStateConnected;

                if ( (rootSocket->AsyncSelectlEvent & FD_WRITE) != 0 ) {

                    SockReenableAsyncSelectEvent( rootSocket, FD_WRITE );

                }

            }
        }

        SockReleaseSocketLock( rootSocket );
        SockDereferenceSocket( rootSocket );
    }
    FREE_HEAP( connectContext );

    SockDereferenceAsyncThread ();

    return;

} // AsyncConnectCompletionApc


INT
SockPostProcessConnect (
    IN PSOCKET_INFORMATION Socket
    )
{
    INT err;

    //
    // Notify the helper DLL that the socket is now connected.
    //

    err = SockNotifyHelperDll( Socket, WSH_NOTIFY_CONNECT );

    //
    // If the connect succeeded, remember this fact in the socket info
    // structure.
    //

    if ( err == NO_ERROR ) {

        Socket->State = SocketStateConnected;

        //
        // Remember the changed state of this socket.
        //

        err = SockSetHandleContext( Socket );

        if ( err != NO_ERROR ) {

            return err;

        }

    }

    //
    // If the application has modified the send or receive buffer sizes,
    // then set up the buffer sizes on the socket.
    //

#ifdef _AFD_SAN_SWITCH_
	//
	// Call SockUpdateWindowSizes only if connected thru TCP, because
	// it needs TDI handles to be present
	//
	if (!(Socket->SanSocket && Socket->SanSocket->IsConnected == CONNECTED)) {
		err = SockUpdateWindowSizes( Socket, FALSE );
	}
#else

	err = SockUpdateWindowSizes( Socket, FALSE );

#endif // _AFD_SAN_SWITCH_


    if ( err != NO_ERROR ) {

        return err;

    }

    return NO_ERROR;

} // SockPostProcessConnect


BOOL
SockCheckAndInitAsyncConnectHelper (
    VOID
    ) {
    HANDLE  Handle;
    UNICODE_STRING afdName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_COMPLETION_INFORMATION completionInfo;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    NTSTATUS status;

    //
    // If the async connect helper handle is already initialized, return.
    //

    if (SockAsyncConnectHelperHandle!=NULL)
        return TRUE;

    //
    // Acquire the global lock to synchronize the creation of the handle.
    //

	SockAcquireRwLockExclusive (&SocketGlobalLock);

    // Check again, in case another thread has already initialized
    // the async helper handle.
    //

    if (SockAsyncConnectHelperHandle!=NULL) {
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        return TRUE;
    }

    //
    // Set up to open a handle to AFD.
    //

    RtlInitUnicodeString( &afdName, L"\\Device\\Afd\\AsyncConnectHlp" );

    InitializeObjectAttributes(
        &objectAttributes,
        &afdName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to AFD.
    //

    status = NtCreateFile(
                 (PHANDLE)&Handle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                     // AllocationSize
                 0L,                                       // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                 FILE_OPEN_IF,                             // CreateDisposition
                 0,                                        // CreateOptions
                 NULL,                                     // EaBuffer
                 0                                         // EaBufferLength
                 );

    if ( !NT_SUCCESS(status) ) {
        WS_PRINT(( "SockCheckAndInitAsyncConnectHelper: NtCreateFile(helper) failed: %X\n",
                       status ));
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        return FALSE;
    }


    completionInfo.Port = SockAsyncQueuePort;
    completionInfo.Key = SockAsyncConnectCompletion;
    status = NtSetInformationFile (
                Handle,
                &ioStatusBlock,
                &completionInfo,
                sizeof (completionInfo),
                FileCompletionInformation);
    if (!NT_SUCCESS (status)) {
        WS_PRINT(( "SockCheckAndInitAsyncConnectHelper: NtSeInformationFile(completion) failed: %X\n",
                       status ));

        NtClose (Handle);
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        return FALSE;
    }

    //
    // Protect our handle from being closed by random NtClose call
    // by some messed up dll or application
    //
    HandleInfo.ProtectFromClose = TRUE;
    HandleInfo.Inherit = FALSE;

    status = NtSetInformationObject( Handle,
                    ObjectHandleFlagInformation,
                    &HandleInfo,
                    sizeof( HandleInfo )
                  );
    //
    // This shouldn't fail
    //
    WS_ASSERT (NT_SUCCESS (status));

    SockAsyncConnectHelperHandle = Handle;
    SockReleaseRwLockExclusive (&SocketGlobalLock);
    return TRUE;
}
