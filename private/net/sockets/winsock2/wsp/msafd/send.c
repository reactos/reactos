/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains support for the send( ), sendto( ),
    WSASend( ), and WSASendTo( ) WinSock API.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "winsockp.h"


int
WSPAPI
WSPSend (
    SOCKET Handle,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD iFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used to write outgoing data from one or more buffers on a
    connection-oriented socket specified by s. It may also be used, however,
    on connectionless sockets which have a stipulated default peer address
    established via the WSPConnect() function.

    For overlapped sockets (created using WSPSocket() with flag
    WSA_FLAG_OVERLAPPED) this will occur using overlapped I/O, unless both
    lpOverlapped and lpCompletionRoutine are NULL in which case the socket is
    treated as a non-overlapped socket. A completion indication will occur
    (invocation of the completion routine or setting of an event object) when
    the supplied buffer(s) have been consumed by the transport. If the
    operation does not complete immediately, the final completion status is
    retrieved via the completion routine or WSPGetOverlappedResult().

    For non-overlapped sockets, the parameters lpOverlapped,
    lpCompletionRoutine, and lpThreadId are ignored and WSPSend() adopts the
    regular synchronous semantics. Data is copied from the supplied buffer(s)
    into the transport's buffer. If the socket is non-blocking and stream-
    oriented, and there is not sufficient space in the transport's buffer,
    WSPSend() will return with only part of the supplied buffers having been
    consumed. Given the same buffer situation and a blocking socket, WSPSend()
    will block until all of the supplied buffer contents have been consumed.

    The array of WSABUF structures pointed to by the lpBuffers parameter is
    transient. If this operation completes in an overlapped manner, it is the
    service provider's responsibility to capture these WSABUF structures
    before returning from this call. This enables applications to build stack-
    based WSABUF arrays.

    For message-oriented sockets, care must be taken not to exceed the maximum
    message size of the underlying provider, which can be obtained by getting
    the value of socket option SO_MAX_MSG_SIZE. If the data is too long to
    pass atomically through the underlying protocol the error WSAEMSGSIZE is
    returned, and no data is transmitted.

    Note that the successful completion of a WSPSend() does not indicate that
    the data was successfully delivered.

    dwFlags may be used to influence the behavior of the function invocation
    beyond the options specified for the associated socket. That is, the
    semantics of this routine are determined by the socket options and the
    dwFlags parameter. The latter is constructed by or-ing any of the
    following values:

        MSG_DONTROUTE - Specifies that the data should not be subject
        to routing. A WinSock service provider may choose to ignore this
        flag.

        MSG_OOB - Send out-of-band data (stream style socket such as
        SOCK_STREAM only).

        MSG_PARTIAL - Specifies that lpBuffers only contains a partial
        message. Note that the error code WSAEOPNOTSUPP will be returned
        which do not support partial message transmissions.

    If an overlapped operation completes immediately, WSPSend() returns a
    value of zero and the lpNumberOfBytesSent parameter is updated with the
    number of bytes sent. If the overlapped operation is successfully
    initiated and will complete later, WSPSend() returns SOCKET_ERROR and
    indicates error code WSA_IO_PENDING. In this case, lpNumberOfBytesSent is
    not updated. When the overlapped operation completes the amount of data
    transferred is indicated either via the cbTransferred parameter in the
    completion routine (if specified), or via the lpcbTransfer parameter in
    WSPGetOverlappedResult().

    Providers must allow this routine to be called from within the completion
    routine of a previous WSPRecv(), WSPRecvFrom(), WSPSend() or WSPSendTo()
    function. However, for a given socket, I/O completion routines may not be
    nested. This permits time-sensitive data transmissions to occur entirely
    within a preemptive context.

    The lpOverlapped parameter must be valid for the duration of the
    overlapped operation. If multiple I/O operations are simultaneously
    outstanding, each must reference a separate overlapped structure. The
    WSAOVERLAPPED structure has the following form:

        typedef struct _WSAOVERLAPPED {
            DWORD       Internal;       // reserved
            DWORD       InternalHigh;   // reserved
            DWORD       Offset;         // reserved
            DWORD       OffsetHigh;     // reserved
            WSAEVENT    hEvent;
        } WSAOVERLAPPED, FAR * LPWSAOVERLAPPED;

    If the lpCompletionRoutine parameter is NULL, the service provider signals
    the hEvent field of lpOverlapped when the overlapped operation completes
    if it contains a valid event object handle. The WinSock SPI client can use
    WSPGetOverlappedResult() to wait or poll on the event object.

    If lpCompletionRoutine is not NULL, the hEvent field is ignored and can be
    used by the WinSock SPI client to pass context information to the
    completion routine. It is the service provider's responsibility to arrange
    for invocation of the client-specified completion routine when the
    overlapped operation completes. Since the completion routine must be
    executed in the context of the same thread that initiated the overlapped
    operation, it cannot be invoked directly from the service provider. The
    WinSock DLL offers an asynchronous procedure call (APC) mechanism to
    facilitate invocation of completion routines.

    A service provider arranges for a function to be executed in the proper
    thread by calling WPUQueueApc(). Note that this routine must be invoked
    while in the context of the same process (but not necessarily the same
    thread) that was used to initiate the overlapped operation. It is the
    service provider's responsibility to arrange for this process context to
    be active prior to calling WPUQueueApc().

    WPUQueueApc() takes as input parameters a pointer to a WSATHREADID
    structure (supplied to the provider via the lpThreadId input parameter),
    a pointer to an APC function to be invoked, and a 32 bit context value
    that is subsequently passed to the APC function. Because only a single
    32-bit context value is available, the APC function cannot itself be the
    client-specified completion routine. The service provider must instead
    supply a pointer to its own APC function which uses the supplied context
    value to access the needed result information for the overlapped operation,
    and then invokes the client-specified completion routine.

    The prototype for the client-supplied completion routine is as follows:

        void
        CALLBACK
        CompletionRoutine(
            IN DWORD dwError,
            IN DWORD cbTransferred,
            IN LPWSAOVERLAPPED lpOverlapped,
            IN DWORD dwFlags
            );

        CompletionRoutine is a placeholder for a client supplied function
        name.

        dwError specifies the completion status for the overlapped
        operation as indicated by lpOverlapped.

        cbTransferred specifies the number of bytes sent.

        No flag values are currently defined and the dwFlags value will
        be zero.

        This routine does not return a value.

    The completion routines may be called in any order, not necessarily in
    the same order the overlapped operations are completed. However, the
    service provider guarantees to the client that posted buffers are sent
    in the same order they are supplied.

Arguments:

    s - A descriptor identifying a connected socket.

    lpBuffers - A pointer to an array of WSABUF structures. Each WSABUF
        structure contains a pointer to a buffer and the length of the
        buffer. This array must remain valid for the duration of the
        send operation.

    dwBufferCount - The number of WSABUF structures in the lpBuffers array.

    lpNumberOfBytesSent - A pointer to the number of bytes sent by this
        call.

    dwFlags - Specifies the way in which the call is made.

    lpOverlapped - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A pointer to the completion routine called when
        the send operation has been completed.

    lpThreadId - A pointer to a thread ID structure to be used by the
        provider in a subsequent call to WPUQueueApc(). The provider should
        store the referenced WSATHREADID structure (not the pointer to same)
        until after the WPUQueueApc() function returns.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs and the send operation has completed immediately,
    WSPSend() returns 0. Note that in this case the completion routine, if
    specified, will have already been queued. Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code is available in
    lpErrno. The error code WSA_IO_PENDING indicates that the overlapped
    operation has been successfully initiated and that completion will be
    indicated at a later time. Any other error code indicates that no
    overlapped operation was initiated and no completion indication will
    occur.

--*/

{
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    IO_STATUS_BLOCK localIoStatusBlock;
    PIO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_SEND_INFO sendInfo;
    HANDLE event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID apcContext;
    PSOCKET_INFORMATION socket = NULL;


    WS_ENTER( "WSPSend", (PVOID)Handle, (PVOID)lpBuffers, (PVOID)dwBufferCount, (PVOID)iFlags );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSend", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

#ifdef _AFD_SAN_SWITCH_
    if (SockSanEnabled) {
        socket = SockFindAndReferenceSocket( Handle, TRUE );

        if ( socket == NULL ) {
            err = WSAENOTSOCK;
            goto exit;
        }

        if (socket->SanSocket!=NULL) {
            err = SockSanSend (
                    socket,
                    lpBuffers,
                    dwBufferCount,
                    lpNumberOfBytesSent,
                    iFlags,
                    lpOverlapped,
                    lpCompletionRoutine,
                    lpThreadId->ThreadHandle);
            goto exit;
        }
    }
#endif //_AFD_SAN_SWITCH_

    //
    // Set up AFD_SEND_INFO.TdiFlags;
    //

    sendInfo.BufferArray = lpBuffers;
    sendInfo.BufferCount = dwBufferCount;
    sendInfo.AfdFlags = 0;
    sendInfo.TdiFlags = 0;

    if ( iFlags != 0 ) {

        //
        // The legal flags are MSG_OOB, MSG_DONTROUTE and MSG_PARTIAL.
        // MSG_OOB is not legal on datagram sockets.
        //

        if ( ( (iFlags & ~(MSG_OOB | MSG_DONTROUTE | MSG_PARTIAL)) != 0 ) ) {

            err = WSAEOPNOTSUPP;
            goto exit;

        }

        if ( (iFlags & MSG_OOB) != 0 ) {

            sendInfo.TdiFlags |= TDI_SEND_EXPEDITED;

        }

        if ( (iFlags & MSG_PARTIAL) != 0 ) {

            sendInfo.TdiFlags |= TDI_SEND_PARTIAL;

        }

    }

    __try {
        //
        // Determine the appropriate APC routine & context, event handle,
        // and IO status block to use for the request.
        //

        if( lpOverlapped == NULL ) {

            //
            // This a synchronous request, use per-thread event object.
            //

            apcRoutine = NULL;
            apcContext = NULL;

            event = tlsData->EventHandle;

            ioStatusBlock = &localIoStatusBlock;

        } else {

            if( lpCompletionRoutine == NULL ) {

                //
                // No APC, use event object from OVERLAPPED structure.
                //

                event = lpOverlapped->hEvent;

                apcRoutine = NULL;
                apcContext = ( (ULONG_PTR)event & 1 ) ? NULL : lpOverlapped;

            } else {

                //
                // APC, ignore event object.
                //

                event = NULL;

                apcRoutine = SockIoCompletion;
                apcContext = lpCompletionRoutine;

                //
                // Tell AFD to skip fast IO on this request.
                //

                sendInfo.AfdFlags |= AFD_NO_FAST_IO;

            }

            //
            // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
            //

            ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

            //
            // Tell AFD this is an overlapped operation.
            //

            sendInfo.AfdFlags |= AFD_OVERLAPPED;

        }

        ioStatusBlock->Status = STATUS_PENDING;


    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }
    //
    // Send the data over the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 event,
                 apcRoutine,
                 apcContext,
                 ioStatusBlock,
                 IOCTL_AFD_SEND,
                 &sendInfo,
                 sizeof(sendInfo),
                 NULL,
                 0
                 );

    if ( apcRoutine != NULL  &&  !NT_ERROR(status) ) {

        tlsData->PendingAPCCount++;
        InterlockedIncrement( &SockProcessPendingAPCCount );
    }

    //
    // If this request has no overlapped structure, then wait for
    // the operation to complete.
    //

    if ( status == STATUS_PENDING &&
         lpOverlapped == NULL ) {

        BOOL success;

        success = SockWaitForSingleObject(
                      event,
                      Handle,
                      SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                      SOCK_SEND_TIMEOUT
                      );

        //
        // If the wait completed successfully, look in the IO status
        // block to determine the real status code of the request.  If
        // the wait timed out, then cancel the IO and set up for an
        // error return.
        //

        if ( success ) {

            status = ioStatusBlock->Status;

        } else {

            SockCancelIo( Handle );
            status = STATUS_IO_TIMEOUT;
        }

    }

    switch (status) {
    case STATUS_SUCCESS:
        break;

    case STATUS_PENDING:
        err = WSA_IO_PENDING;
        goto exit;

    default:
        if (!NT_SUCCESS(status) ) {
            //
            // Map the NTSTATUS to a WinSock error code.
            //

            err = SockNtStatusToSocketError( status );
            goto exit;
        }
    }

    //
    // The request completed immediately, so return the number of
    // bytes sent to the user.
    // It is possible that application deallocated lpOverlapped 
    // in another thread if completion port was used to receive
    // completion indication. We do not want to confuse the
    // application by returning failure, just pretend that we didn't
    // know about synchronous completion
    //

    __try {
        *lpNumberOfBytesSent = (DWORD)ioStatusBlock->Information;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (lpOverlapped) {
            err = WSA_IO_PENDING;
        }
        else {
            err = WSAEFAULT;
        }
        goto exit;
    }

exit:

    IF_DEBUG(SEND) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPSend on socket %lx failed: %ld.\n",
                           Handle, err ));

        } else {

#ifdef _AFD_SAN_SWITCH_
            WS_PRINT(( "WSPSend on socket %lx succeeded, "
                       "bytes = %ld\n", Handle, *lpNumberOfBytesSent ));
#else
            WS_PRINT(( "WSPSend on socket %lx succeeded, "
                       "bytes = %ld\n", Handle, ioStatusBlock->Information ));
#endif //_AFD_SAN_SWITCH_

        }

    }

    //
    // If there is an async thread in this process, get a pointer to the
    // socket information structure and reenable the appropriate event.
    // We don't do this if there is no async thread as a performance
    // optimization.  Also, if we're not going to return WSAEWOULDBLOCK
    // we don't need to reenable FD_WRITE events.
    //

    if ( SockAsyncSelectCalled && err == WSAEWOULDBLOCK ) {
#ifdef _AFD_SAN_SWITCH_
        if (socket==NULL) {
            socket = SockFindAndReferenceSocket( Handle, TRUE );
        }
#else //_AFD_SAN_SWITCH_
        socket = SockFindAndReferenceSocket( Handle, TRUE );
#endif //_AFD_SAN_SWITCH_

        //
        // If the socket was found reenable the right event.  If it was
        // not found, then presumably the socket handle was invalid.
        //

        if ( socket != NULL ) {

            SockAcquireSocketLockExclusive( socket );
            SockReenableAsyncSelectEvent( socket, FD_WRITE );
            SockReleaseSocketLock( socket );

            SockDereferenceSocket( socket );

        } else {

            if ( socket == NULL ) {

                WS_PRINT(( "WSPSend: SockFindAndReferenceSocket failed.\n" ));

            }

        }

    }
#ifdef _AFD_SAN_SWITCH_
    else {
        if (socket!=NULL) {
            SockDereferenceSocket( socket );
        }
    }
#endif //_AFD_SAN_SWITCH_

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPSend", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPSend", 0, FALSE );
    return 0;

}   // WSPSend


int
WSPAPI
WSPSendTo (
    SOCKET Handle,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD iFlags,
    const struct sockaddr *SocketAddress,
    int SocketAddressLength,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is normally used on a connectionless socket specified by s
    to send a datagram contained in one or more buffers to a specific peer
    socket identified by the lpTo parameter. On a connection-oriented socket,
    the lpTo and iToLen parameters are ignored; in this case the WSPSendTo()
    is equivalent to WSPSend().

    For overlapped sockets (created using WSPSocket() with flag
    WSA_FLAG_OVERLAPPED) this will occur using overlapped I/O, unless both
    lpOverlapped and lpCompletionRoutine are NULL in which case the socket is
    treated as a non-overlapped socket. A completion indication will occur
    (invocation of the completion routine or setting of an event object) when
    the supplied buffer(s) have been consumed by the transport. If the
    operation does not complete immediately, the final completion status is
    retrieved via the completion routine or WSPGetOverlappedResult().

    For non-overlapped sockets, the parameters lpOverlapped,
    lpCompletionRoutine, and lpThreadId are ignored and WSPSend() adopts the
    regular synchronous semantics. Data is copied from the supplied buffer(s)
    into the transport's buffer. If the socket is non-blocking and stream-
    oriented, and there is not sufficient space in the transport's buffer,
    WSPSend() will return with only part of the supplied buffers having been
    consumed. Given the same buffer situation and a blocking socket, WSPSend()
    will block until all of the supplied buffer contents have been consumed.

    The array of WSABUF structures pointed to by the lpBuffers parameter is
    transient. If this operation completes in an overlapped manner, it is the
    service provider's responsibility to capture these WSABUF structures
    before returning from this call. This enables applications to build stack-
    based WSABUF arrays.

    For message-oriented sockets, care must be taken not to exceed the maximum
    message size of the underlying provider, which can be obtained by getting
    the value of socket option SO_MAX_MSG_SIZE. If the data is too long to
    pass atomically through the underlying protocol the error WSAEMSGSIZE is
    returned, and no data is transmitted.

    Note that the successful completion of a WSPSendTo() does not indicate that
    the data was successfully delivered.

    dwFlags may be used to influence the behavior of the function invocation
    beyond the options specified for the associated socket. That is, the
    semantics of this routine are determined by the socket options and the
    dwFlags parameter. The latter is constructed by or-ing any of the
    following values:

        MSG_DONTROUTE - Specifies that the data should not be subject
        to routing. A WinSock service provider may choose to ignore this
        flag.

        MSG_OOB - Send out-of-band data (stream style socket such as
        SOCK_STREAM only).

        MSG_PARTIAL - Specifies that lpBuffers only contains a partial
        message. Note that the error code WSAEOPNOTSUPP will be returned
        which do not support partial message transmissions.

    If an overlapped operation completes immediately, WSPSendTo() returns a
    value of zero and the lpNumberOfBytesSent parameter is updated with the
    number of bytes sent. If the overlapped operation is successfully
    initiated and will complete later, WSPSendTo() returns SOCKET_ERROR and
    indicates error code WSA_IO_PENDING. In this case, lpNumberOfBytesSent is
    not updated. When the overlapped operation completes the amount of data
    transferred is indicated either via the cbTransferred parameter in the
    completion routine (if specified), or via the lpcbTransfer parameter in
    WSPGetOverlappedResult().

    Providers must allow this routine to be called from within the completion
    routine of a previous WSPRecv(), WSPRecvFrom(), WSPSend() or WSPSendTo()
    function. However, for a given socket, I/O completion routines may not be
    nested. This permits time-sensitive data transmissions to occur entirely
    within a preemptive context.

    The lpOverlapped parameter must be valid for the duration of the
    overlapped operation. If multiple I/O operations are simultaneously
    outstanding, each must reference a separate overlapped structure. The
    WSAOVERLAPPED structure has the following form:

        typedef struct _WSAOVERLAPPED {
            DWORD       Internal;       // reserved
            DWORD       InternalHigh;   // reserved
            DWORD       Offset;         // reserved
            DWORD       OffsetHigh;     // reserved
            WSAEVENT    hEvent;
        } WSAOVERLAPPED, FAR * LPWSAOVERLAPPED;

    If the lpCompletionRoutine parameter is NULL, the service provider signals
    the hEvent field of lpOverlapped when the overlapped operation completes
    if it contains a valid event object handle. The WinSock SPI client can use
    WSPGetOverlappedResult() to wait or poll on the event object.

    If lpCompletionRoutine is not NULL, the hEvent field is ignored and can be
    used by the WinSock SPI client to pass context information to the
    completion routine. It is the service provider's responsibility to arrange
    for invocation of the client-specified completion routine when the
    overlapped operation completes. Since the completion routine must be
    executed in the context of the same thread that initiated the overlapped
    operation, it cannot be invoked directly from the service provider. The
    WinSock DLL offers an asynchronous procedure call (APC) mechanism to
    facilitate invocation of completion routines.

    A service provider arranges for a function to be executed in the proper
    thread by calling WPUQueueApc(). Note that this routine must be invoked
    while in the context of the same process (but not necessarily the same
    thread) that was used to initiate the overlapped operation. It is the
    service provider's responsibility to arrange for this process context to
    be active prior to calling WPUQueueApc().

    WPUQueueApc() takes as input parameters a pointer to a WSATHREADID
    structure (supplied to the provider via the lpThreadId input parameter),
    a pointer to an APC function to be invoked, and a 32 bit context value
    that is subsequently passed to the APC function. Because only a single
    32-bit context value is available, the APC function cannot itself be the
    client-specified completion routine. The service provider must instead
    supply a pointer to its own APC function which uses the supplied context
    value to access the needed result information for the overlapped operation,
    and then invokes the client-specified completion routine.

    The prototype for the client-supplied completion routine is as follows:

        void
        CALLBACK
        CompletionRoutine(
            IN DWORD dwError,
            IN DWORD cbTransferred,
            IN LPWSAOVERLAPPED lpOverlapped,
            IN DWORD dwFlags
            );

        CompletionRoutine is a placeholder for a client supplied function
        name.

        dwError specifies the completion status for the overlapped
        operation as indicated by lpOverlapped.

        cbTransferred specifies the number of bytes sent.

        No flag values are currently defined and the dwFlags value will
        be zero.

        This routine does not return a value.

    The completion routines may be called in any order, not necessarily in
    the same order the overlapped operations are completed. However, the
    service provider guarantees to the client that posted buffers are sent
    in the same order they are supplied.

Arguments:

    s - A descriptor identifying a socket.

    lpBuffers - A pointer to an array of WSABUF structures. Each WSABUF
        structure contains a pointer to a buffer and the length of the
        buffer. This array must remain valid for the duration of the
        send operation.

    dwBufferCount - The number of WSABUF structures in the lpBuffers
        array.

    lpNumberOfBytesSent - A pointer to the number of bytes sent by this
        call.

    dwFlags - Specifies the way in which the call is made.

    lpTo - An optional pointer to the address of the target socket.

    iTolen - The size of the address in lpTo.

    lpOverlapped - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A pointer to the completion routine called
        when the send operation has been completed.

    lpThreadId - A pointer to a thread ID structure to be used by the
        provider in a subsequent call to WPUQueueApc(). The provider
        should store the referenced WSATHREADID structure (not the
        pointer to same) until after the WPUQueueApc() function returns.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs and the send operation has completed immediately,
    WSPSendTo() returns 0. Note that in this case the completion routine, if
    specified, will have already been queued. Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code is available in
    lpErrno. The error code WSA_IO_PENDING indicates that the overlapped
    operation has been successfully initiated and that completion will be
    indicated at a later time. Any other error code indicates that no
    overlapped operation was initiated and no completion indication will occur.

--*/

{
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK localIoStatusBlock;
    PIO_STATUS_BLOCK ioStatusBlock;
    AFD_SEND_DATAGRAM_INFO sendInfo;
    PTRANSPORT_ADDRESS tdiAddress;
    ULONG tdiAddressLength;
    int err;
    UCHAR tdiAddressBuffer[MAX_FAST_TDI_ADDRESS];
    HANDLE event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID apcContext;

    WS_ENTER( "WSPSendTo", (PVOID)Handle, (PVOID)lpBuffers, (PVOID)dwBufferCount, (PVOID)iFlags );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSendTo", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    tdiAddress = (PTRANSPORT_ADDRESS)tdiAddressBuffer;

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
    // If this is not a datagram socket, just call send() to process the
    // call.  The address and address length parameters are not checked.
    //

    if ( !IS_DGRAM_SOCK(socket)
		    || ( (socket->State==SocketStateConnected)
                && ( SocketAddress == NULL || SocketAddressLength == 0 ))
        ) {

        INT ret;

        SockDereferenceSocket( socket );

        ret = WSPSend(
                  Handle,
                  lpBuffers,
                  dwBufferCount,
                  lpNumberOfBytesSent,
                  iFlags,
                  lpOverlapped,
                  lpCompletionRoutine,
                  lpThreadId,
                  lpErrno
                  );

        WS_EXIT( "WSPSendTo", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
        return ret;

    }

    IF_DEBUG(SEND) {

        WS_PRINT(( "WSASendTo() on socket %lx to addr", Handle ));
        WsPrintSockaddr( (PSOCKADDR)SocketAddress, &SocketAddressLength );

    }

    //
    // If the socket is not connected, then the Address and AddressLength
    // fields must be specified.
    //

    if ( socket->State != SocketStateConnected ) {

        if ( SocketAddress == NULL ) {

            err = WSAENOTCONN;
            goto exit;

        }

    }

    // Note: we simply truncate sockaddr's > MaxSockaddrLength down below
    if ( SocketAddressLength < socket->HelperDll->MinSockaddrLength ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // The legal flags are MSG_OOB, MSG_DONTROUTE, and MSG_PARTIAL.
    // MSG_OOB is not legal on datagram sockets.
    //

    WS_ASSERT( IS_DGRAM_SOCK( socket ) );

    if ( ( (iFlags & ~(MSG_DONTROUTE)) != 0 ) ) {

        err = WSAEOPNOTSUPP;
        goto exit;

    }


    //
    // If data send has been shut down, fail.
    //

    if ( socket->SendShutdown ) {

        err = WSAESHUTDOWN;
        goto exit;

    }

    __try {
        //
        // Make sure that the address family passed in here is the same as
        // was passed in on the socket( ) call.
        //

        if ( (short)socket->AddressFamily != SocketAddress->sa_family ) {

            err = WSAEAFNOSUPPORT;
            goto exit;

        }

        //
        // If this socket has not been set to allow broadcasts, check if this
        // is an attempt to send to a broadcast address.
        //

        if ( !socket->Broadcast ) {

            SOCKADDR_INFO sockaddrInfo;

            err = socket->HelperDll->WSHGetSockaddrType(
                        (PSOCKADDR)SocketAddress,
                        SocketAddressLength,
                        &sockaddrInfo
                        );

            if ( err != NO_ERROR) {

                goto exit;

            }

            //
            // If this is an attempt to send to a broadcast address, reject
            // the attempt.
            //

            if ( sockaddrInfo.AddressInfo == SockaddrAddressInfoBroadcast ) {

                err = WSAEACCES;
                goto exit;

            }

        }

        //
        // If this socket is not yet bound to an address, bind it to an
        // address.  We only do this if the helper DLL for the socket supports
        // a get wildcard address routine--if it doesn't, the app must bind
        // to an address manually.
        //

        if ( socket->State == SocketStateOpen) {
		    if (socket->HelperDll->WSHGetWildcardSockaddr != NULL ) {

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

			    //
			    // Acquire the lock that protect this sockets.  We hold this lock
			    // throughout this routine to synchronize against other callers
			    // performing operations on the socket we're sending data on.
			    //

			    SockAcquireSocketLockExclusive( socket );

			    //
			    // Recheck socket state under the lock
			    //
			    if (socket->State == SocketStateOpen) {
				    result = WSPBind(
							     Handle,
							     sockaddr,
							     sockaddrLength,
							     &err
							     );
			    }
                else
                    result = ERROR_SUCCESS;

	            SockReleaseSocketLock( socket );
			    FREE_HEAP( sockaddr );

			    if( result == SOCKET_ERROR ) {

				    goto exit;

			    }

	        } else {

			    //
			    // The socket is not bound and the helper DLL does not support
			    // a wildcard socket address.  Fail, the app must bind manually.
			    //

			    err = WSAEINVAL;
			    goto exit;
		    }
        }

        //
        // Allocate enough space to hold the TDI address structure we'll pass
        // to AFD.  Note that is the address is small enough, we just use
        // an automatic in order to improve performance.
        //

        tdiAddressLength =  socket->HelperDll->MaxTdiAddressLength;

        if ( tdiAddressLength > MAX_FAST_TDI_ADDRESS ) {

            tdiAddress = ALLOCATE_HEAP( tdiAddressLength );

            if ( tdiAddress == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

        } else {

            WS_ASSERT( (PUCHAR)tdiAddress == tdiAddressBuffer );

        }

        //
        // Convert the address from the sockaddr structure to the appropriate
        // TDI structure.
        //
        // Note: We'll truncate any part of the specifed sock addr beyond
        //       that which the helper considers valid
        //

        err = SockBuildTdiAddress(
            tdiAddress,
            (PSOCKADDR)SocketAddress,
            (SocketAddressLength > socket->HelperDll->MaxSockaddrLength ?
                socket->HelperDll->MaxSockaddrLength :
                SocketAddressLength)
            );

        if (err!=NO_ERROR) {
            goto exit;
        }



        //
        // Set up the AFD_SEND_DATAGRAM_INFO structure.
        //

        sendInfo.BufferArray = lpBuffers;
        sendInfo.BufferCount = dwBufferCount;
        sendInfo.AfdFlags = 0;

        //
        // Set up the TDI_REQUEST structure to send the datagram.
        //

        sendInfo.TdiConnInfo.RemoteAddressLength = tdiAddressLength;
        sendInfo.TdiConnInfo.RemoteAddress = tdiAddress;


        //
        // Determine the appropriate APC routine & context, event handle,
        // and IO status block to use for the request.
        //

        if( lpOverlapped == NULL ) {

            //
            // This a synchronous request, use per-thread event object.
            //

            apcRoutine = NULL;
            apcContext = NULL;

            event = tlsData->EventHandle;

            ioStatusBlock = &localIoStatusBlock;

        } else {

            if( lpCompletionRoutine == NULL ) {

                //
                // No APC, use event object from OVERLAPPED structure.
                //

                event = lpOverlapped->hEvent;

                apcRoutine = NULL;
                apcContext = ( (ULONG_PTR)event & 1 ) ? NULL : lpOverlapped;

            } else {

                //
                // APC, ignore event object.
                //

                event = NULL;

                apcRoutine = SockIoCompletion;
                apcContext = lpCompletionRoutine;

                //
                // Tell AFD to skip fast IO on this request.
                //

                sendInfo.AfdFlags |= AFD_NO_FAST_IO;

            }

            //
            // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
            //

            ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

            //
            // Tell AFD this is an overlapped operation.
            //

            sendInfo.AfdFlags |= AFD_OVERLAPPED;

        }

        ioStatusBlock->Status = STATUS_PENDING;

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }
    //
    // Send the data over the socket.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
                 event,
                 apcRoutine,
                 apcContext,
                 ioStatusBlock,
                 IOCTL_AFD_SEND_DATAGRAM,
                 &sendInfo,
                 sizeof(sendInfo),
                 NULL,
                 0
                 );

    if ( apcRoutine != NULL  &&  !NT_ERROR(status) ) {

        tlsData->PendingAPCCount++;
        InterlockedIncrement( &SockProcessPendingAPCCount );
    }

    //
    // If this request has no overlapped structure, then wait for
    // the operation to complete.
    //

    if ( status == STATUS_PENDING &&
         lpOverlapped == NULL ) {

        BOOL success;

        success = SockWaitForSingleObject(
                      event,
                      Handle,
                      SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                      SOCK_SEND_TIMEOUT
                      );

        //
        // If the wait completed successfully, look in the IO status
        // block to determine the real status code of the request.  If
        // the wait timed out, then cancel the IO and set up for an
        // error return.
        //

        if ( success ) {

            status = ioStatusBlock->Status;

        } else {

            SockCancelIo( Handle );
            status = STATUS_IO_TIMEOUT;
        }

    }

    switch (status) {
    case STATUS_SUCCESS:
        break;

    case STATUS_PENDING:
        err = WSA_IO_PENDING;
        goto exit;

    default:
        if (!NT_SUCCESS(status) ) {
            //
            // Map the NTSTATUS to a WinSock error code.
            //

            err = SockNtStatusToSocketError( status );
            goto exit;
        }
    }

    //
    // The request completed immediately, so return the number of
    // bytes sent to the user.
    // It is possible that application deallocated lpOverlapped 
    // in another thread if completion port was used to receive
    // completion indication. We do not want to confuse the
    // application by returning failure, just pretend that we didn't
    // know about synchronous completion
    //

    __try {
        *lpNumberOfBytesSent = (DWORD)ioStatusBlock->Information;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (lpOverlapped) {
            err = WSA_IO_PENDING;
        }
        else {
            err = WSAEFAULT;
        }
        goto exit;
    }

exit:

    IF_DEBUG(SEND) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPSendTo on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPSendTo on socket %lx (%lx) succeeded, "
                       "bytes = %ld\n",
                           Handle, socket, ioStatusBlock->Information ));

        }

    }

    if ( socket != NULL ) {

	    if ( SockAsyncSelectCalled && err == WSAEWOULDBLOCK ) {

			SockAcquireSocketLockExclusive( socket );

            SockReenableAsyncSelectEvent( socket, FD_WRITE );

	        SockReleaseSocketLock( socket );
        }

        SockDereferenceSocket( socket );

    }

    if ( tdiAddress != NULL &&
         tdiAddress != (PTRANSPORT_ADDRESS)tdiAddressBuffer ) {

        FREE_HEAP( tdiAddress );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPSendTo", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPSendTo", 0, FALSE );
    return 0;

}   // WSPSendTo
