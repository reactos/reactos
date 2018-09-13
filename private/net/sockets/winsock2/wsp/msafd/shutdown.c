/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    shutdown.c

Abstract:

    This module contains support for the socket( ) and closesocket( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"


int
WSPAPI
WSPShutdown(
    IN SOCKET Handle,
    IN int HowTo,
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

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;
    DWORD notificationEvent;

    WS_ENTER( "WSPShutdown", (PVOID)Handle, (PVOID)HowTo, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPShutdown", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
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
    // Acquire the lock that protect this socket.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If this is not a datagram socket, then it must be connected in order
    // for WSPShutdown() to be a legal operation.
    //

    if ( !IS_DGRAM_SOCK(socket) &&
             !SockIsSocketConnected( socket ) ) {

        err = WSAENOTCONN;
        goto exit;

    }

    //
    // Translate the How parameter into the AFD disconnect information
    // structure.
    //

    switch ( HowTo ) {

    case SD_RECEIVE:

        disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_RECEIVE;
        socket->ReceiveShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_RECEIVE;
        break;

    case SD_SEND:

        disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_SEND;
        socket->SendShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_SEND;
        break;

    case SD_BOTH:

        disconnectInfo.DisconnectMode =
            AFD_PARTIAL_DISCONNECT_RECEIVE | AFD_PARTIAL_DISCONNECT_SEND;
        socket->ReceiveShutdown = TRUE;
        socket->SendShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_ALL;
        break;

    default:

        err = WSAEINVAL;
        goto exit;

    }


#ifdef _AFD_SAN_SWITCH_
	//
	// If we have a SAN socket open, close that first
	//
	if (SockSanEnabled && socket->SanSocket &&
		socket->SanSocket->IsConnected == CONNECTED) {

		err = SockSanShutdown(socket, HowTo);
		
		goto exit;
	}
#endif //_AFD_SAN_SWITCH_


    // !!! temporary HACK for tp4!

    if ( (HowTo == 1 || HowTo == 2) && socket->AddressFamily == AF_OSI ) {

        disconnectInfo.DisconnectMode = AFD_ABORTIVE_DISCONNECT;

    }

    //
    // This routine should complete immediately, not when the remote client
    // acknowledges the disconnect.
    //

    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );

    IF_DEBUG(CLOSE) {

        WS_PRINT(( "starting WSPShutdown for socket %lx\n", Handle ));

    }

    //
    // Send the IOCTL to AFD for processing.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
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

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // Notify the helper DLL that the socket has been shut down.
    //

    err = SockNotifyHelperDll( socket, notificationEvent );

    if ( err != NO_ERROR ) {

        goto exit;

    }

exit:

    IF_DEBUG(SHUTDOWN) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPShutdown(%ld) on socket %lx (%lx) failed: %ld.\n",
                           HowTo, Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPShutdown(%ld) on socket %lx (%lx) succeeded.\n",
                           HowTo, Handle, socket ));

        }

    }

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPShutdown", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPShutdown", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPShutdown


int
WSPAPI
WSPRecvDisconnect(
    SOCKET Handle,
    LPWSABUF lpInboundDisconnectData,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on connection-oriented sockets to disable reception,
    and retrieve any incoming disconnect data from the remote party.

    After this routine has been successfully issued, subsequent receives on the
    socket will be disallowed. This has no effect on the lower protocol layers.
    For TCP, the TCP window is not changed and incoming data will be accepted
    (but not acknowledged) until the window is exhausted. For UDP, incoming
    datagrams are accepted and queued. In no case will an ICMP error packet be
    generated.

    To successfully receive incoming disconnect data, a WinSock SPI client must
    use other mechanisms to determine that the circuit has been closed. For
    example, a client needs to receive an FD_CLOSE notification, or get a 0
    return value, or a WSAEDISCON error code from WSPRecv().

    Note that WSPRecvDisconnect() does not close the socket, and resources
    attached to the socket will not be freed until WSPCloseSocket() is invoked.

    WSPRecvDisconnect() does not block regardless of the SO_LINGER setting on
    the socket.

    A WinSock SPI client should not rely on being able to re-use a socket after
    it has been WSPRecvDisconnect()ed. In particular, a WinSock provider is not
    required to support the use of WSPConnect() on such a socket.

Arguments:

    s - A descriptor identifying a socket.

    lpInboundDisconnectData - A pointer to a buffer into which disconnect
        data is to be copied.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPRecvDisconnect() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;

    WS_ENTER( "WSPRecvDisconnect", (PVOID)Handle, (PVOID)lpInboundDisconnectData, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPRecvDisconnect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
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
    // Acquire the lock that protect this socket.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If this is not a datagram socket, then it must be connected in order
    // for WSPRecvDisconnect() to be a legal operation.
    //

    if ( !IS_DGRAM_SOCK(socket) &&
             !SockIsSocketConnected( socket ) ) {

        err = WSAENOTCONN;
        goto exit;

    }

    //
    // Setup the disconnect info.
    //

    disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_RECEIVE;
    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );

    // !!! temporary HACK for tp4!

    if( socket->AddressFamily == AF_OSI ) {

        disconnectInfo.DisconnectMode = AFD_ABORTIVE_DISCONNECT;

    }

    //
    // Note in the socket state that receives are shutdown.
    //

    socket->ReceiveShutdown = TRUE;

    IF_DEBUG(SHUTDOWN) {

        WS_PRINT(( "starting WSPRecvDisconnect for socket %lx\n", Handle ));

    }

#ifdef _AFD_SAN_SWITCH_
	//
	// If we have a SAN socket open, close that first
	//
	if (SockSanEnabled && socket->SanSocket &&
		socket->SanSocket->IsConnected == CONNECTED) {

		err = SockSanRecvDisconnect(socket);

		goto exit;
	}
#endif //_AFD_SAN_SWITCH_

    //
    // Send the IOCTL to AFD for processing.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
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

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // Notify the helper DLL that the socket has been shut down.
    //

    err = SockNotifyHelperDll( socket, WSH_NOTIFY_SHUTDOWN_RECEIVE );

    if ( err != NO_ERROR ) {

        goto exit;

    }

    __try {
        if( lpInboundDisconnectData != NULL &&
            lpInboundDisconnectData->buf != NULL &&
            lpInboundDisconnectData->len > 0 ) {

            INT bufferLength;

            //
            // Retrieve the disconnect data from AFD.
            //

            bufferLength = (INT)lpInboundDisconnectData->len;

            err = SockGetConnectData(
                      socket,
                      IOCTL_AFD_GET_DISCONNECT_DATA,
                      (PCHAR)lpInboundDisconnectData->buf,
                      bufferLength,
                      &bufferLength
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


exit:

    IF_DEBUG(SHUTDOWN) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPRecvDisconnect() on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPRecvDisconnect() on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPRecvDisconnect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPRecvDisconnect", NO_ERROR, FALSE );
    return NO_ERROR;


}   // WSPRecvDisconnect


int
WSPAPI
WSPSendDisconnect (
    SOCKET Handle,
    LPWSABUF lpOutboundDisconnectData,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on connection-oriented sockets to disable
    transmission, and to initiate termination of the connection along with the
    transmission of disconnect data, if any.

    After this routine has been successfully issued, subsequent sends are
    disallowed.

    lpOutboundDisconnectData, if not NULL, points to a buffer containing the
    outgoing disconnect data to be sent to the remote party.

    Note that WSPSendDisconnect() does not close the socket, and resources
    attached to the socket will not be freed until WSPCloseSocket() is invoked.

    WSPSendDisconnect() does not block regardless of the SO_LINGER setting on
    the socket.

    A WinSock SPI client should not rely on being able to re-use a socket after
    it has been WSPSendDisconnect()ed. In particular, a WinSock provider is not
    required to support the use of WSPConnect() on such a socket.

Arguments:

    s - A descriptor identifying a socket.

    lpOutboundDisconnectData - A pointer to the outgoing disconnect data.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPSendDisconnect() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;

    WS_ENTER( "WSPSendDisconnect", (PVOID)Handle, (PVOID)lpOutboundDisconnectData, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSendDisconnect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
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
    // Acquire the lock that protect this socket.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If this is not a datagram socket, then it must be connected in order
    // for WSPSendDisconnect() to be a legal operation.
    //

    if ( !IS_DGRAM_SOCK(socket) &&
             !SockIsSocketConnected( socket ) ) {

        err = WSAENOTCONN;
        goto exit;

    }

#ifdef _AFD_SAN_SWITCH_
	//
	// Handle SAN disconnect first
	//
	if (SockSanEnabled && socket->SanSocket &&
		socket->SanSocket->IsConnected == CONNECTED) {

		socket->SendShutdown = TRUE;

		SockSanShutdown(socket, SD_SEND);

		goto exit;
	}
#endif //_AFD_SAN_SWITCH_

    //
    // Setup the disconnect info.
    //

    disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_SEND;
    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );

    // !!! temporary HACK for tp4!

    if( socket->AddressFamily == AF_OSI ) {

        disconnectInfo.DisconnectMode = AFD_ABORTIVE_DISCONNECT;

    }

    __try {
        if( lpOutboundDisconnectData != NULL &&
            lpOutboundDisconnectData->buf != NULL &&
            lpOutboundDisconnectData->len > 0 ) {

            INT bufferLength;

            //
            // Set the disconnect data on the socket.
            //

            bufferLength = (INT)lpOutboundDisconnectData->len;

            err = SockSetConnectData(
                      socket,
                      IOCTL_AFD_SET_DISCONNECT_DATA,
                      (PCHAR)lpOutboundDisconnectData->buf,
                      bufferLength,
                      &bufferLength
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

    //
    // Note in the socket state that sends are shutdown.
    //

    socket->SendShutdown = TRUE;

    IF_DEBUG(SHUTDOWN) {

        WS_PRINT(( "starting WSPSendDisconnect for socket %lx\n", Handle ));

    }

    //
    // Send the IOCTL to AFD for processing.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
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

        err = SockNtStatusToSocketError( status );
        goto exit;

    }

    //
    // Notify the helper DLL that the socket has been shut down.
    //

    err = SockNotifyHelperDll( socket, WSH_NOTIFY_SHUTDOWN_SEND );

    if ( err != NO_ERROR ) {

        goto exit;

    }

exit:

    IF_DEBUG(SHUTDOWN) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPSendDisconnect() on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPSendDisconnect() on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPSendDisconnect", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPSendDisconnect", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPSendDisconnect
