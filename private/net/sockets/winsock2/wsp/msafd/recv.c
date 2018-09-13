/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    recv.c

Abstract:

    This module contains support for the recv( ), recvfrom( ), WSARecv( ),
    and WSARecvFrom( ) WinSock APIs.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "winsockp.h"

#define WSAEMSGPARTIAL (WSABASEERR+100)


int
WSPAPI
WSPRecv(
    SOCKET Handle,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRead,
    LPDWORD ReceiveFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on connected sockets or bound connectionless sockets
    specified by the s parameter and is used to read incoming data.

    For overlapped sockets WSPRecv() is used to post one or more buffers into
    which incoming data will be placed as it becomes available, after which the
    WinSock SPI client-specified completion indication (invocation of the
    completion routine or setting of an event object) occurs. If the operation
    does not complete immediately, the final completion status is retrieved
    via the completion routine or WSPGetOverlappedResult().

    If both lpOverlapped and lpCompletionRoutine are NULL, the socket in this
    routine will be treated as a non-overlapped socket.

    For non-overlapped sockets, the lpOverlapped, lpCompletionRoutine, and
    lpThreadId parameters are ignored. Any data which has already been received
    and buffered by the transport will be copied into the supplied user
    buffers. For the case of a blocking socket with no data currently having
    been received and buffered by the transport, the call will block until data
    is received.

    The supplied buffers are filled in the order in which they appear in the
    array pointed to by lpBuffers, and the buffers are packed so that no holes
    are created.

    The array of WSABUF structures pointed to by the lpBuffers parameter is
    transient. If this operation completes in an overlapped manner, it is the
    service provider's responsibility to capture this array of pointers to
    WSABUF structures before returning from this call. This enables WinSock SPI
    clients to build stack-based WSABUF arrays.

    For byte stream style sockets (e.g., type SOCK_STREAM), incoming data is
    placed into the buffers until the buffers are filled, the connection is
    closed, or internally buffered data is exhausted. Regardless of whether or
    not the incoming data fills all the buffers, the completion indication
    occurs for overlapped sockets. For message-oriented sockets (e.g., type
    SOCK_DGRAM), an incoming message is placed into the supplied buffers, up
    to the total size of the buffers supplied, and the completion indication
    occurs for overlapped sockets. If the message is larger than the buffers
    supplied, the buffers are filled with the first part of the message. If the
    MSG_PARTIAL feature is supported by the service provider, the MSG_PARTIAL
    flag is set in lpFlags and subsequent receive operation(s) may be used to
    retrieve the rest of the message. If MSG_PARTIAL is not supported but the
    protocol is reliable, WSPRecv() generates the error WSAEMSGSIZE and a
    subsequent receive operation with a larger buffer can be used to retrieve
    the entire message. Otherwise (i.e. the protocol is unreliable and does not
    support MSG_PARTIAL), the excess data is lost, and WSPRecv() generates the
    error WSAEMSGSIZE.

    For connection-oriented sockets, WSPRecv() can indicate the graceful
    termination of the virtual circuit in one of two ways, depending on whether
    the socket is a byte stream or message-oriented. For byte streams, zero
    bytes having been read indicates graceful closure and that no more bytes
    will ever be read. For message-oriented sockets, where a zero byte message
    is often allowable, a return error code of WSAEDISCON is used to indicate
    graceful closure. In any case a return error code of WSAECONNRESET
    indicates an abortive close has occurred.

    lpFlags may be used to influence the behavior of the function invocation
    beyond the options specified for the associated socket. That is, the
    semantics of this routine are determined by the socket options and the
    lpFlags parameter. The latter is constructed by or-ing any of the
    following values:

        MSG_PEEK - Peek at the incoming data. The data is copied into the
        buffer but is not removed from the input queue. This flag is valid
        only for non-overlapped sockets.

        MSG_OOB - Process out-of-band data.

        MSG_PARTIAL - This flag is for message-oriented sockets only. On
        output, indicates that the data supplied is a portion of the message
        transmitted by the sender. Remaining portions of the message will be
        supplied in subsequent receive operations. A subsequent receive
        operation with MSG_PARTIAL flag cleared indicates end of sender's
        message.

        As an input parameter, MSG_PARTIAL indicates that the receive
        operation should complete even if only part of a message has been
        received by the service provider.

    If an overlapped operation completes immediately, WSPRecv() returns a
    value of zero and the lpNumberOfBytesRecvd parameter is updated with the
    number of bytes received. If the overlapped operation is successfully
    initiated and will complete later, WSPRecv() returns SOCKET_ERROR and
    indicates error code WSA_IO_PENDING. In this case, lpNumberOfBytesRecvd is
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
    posted buffers are guaranteed to be filled in the same order they are
    supplied.

Arguments:

    s - A descriptor identifying a connected socket.

    lpBuffers - A pointer to an array of WSABUF structures. Each WSABUF
        structure contains a pointer to a buffer and the length of the
        buffer.

    dwBufferCount - The number of WSABUF structures in the lpBuffers
        array.

    lpNumberOfBytesRecvd - A pointer to the number of bytes received by
        this call.

    lpFlags - A pointer to flags.

    lpOverlapped - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A pointer to the completion routine called when
        the receive operation has been completed.

    lpThreadId - A pointer to a thread ID structure to be used by the
        provider in a subsequent call to WPUQueueApc(). The provider should
        store the referenced WSATHREADID structure (not the pointer to same)
        until after the WPUQueueApc() function returns.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs and the receive operation has completed immediately,
        WSPRecv() returns 0. Note that in this case the completion routine,
        if specified, will have already been queued. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno. The error code WSA_IO_PENDING indicates that the overlapped
        operation has been successfully initiated and that completion will be
        indicated at a later time. Any other error code indicates that no
        overlapped operations was initiated and no completion indication will
        occur.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    IO_STATUS_BLOCK localIoStatusBlock;
    PIO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_RECV_INFO recvInfo;
    HANDLE event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID apcContext;
    PSOCKET_INFORMATION socket = NULL;

    WS_ENTER( "WSPRecv", (PVOID)Handle, lpBuffers, (PVOID)dwBufferCount, (PVOID)ReceiveFlags );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPRecv", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

#ifdef _AFD_SAN_SWITCH_
    if (SockSanEnabled) {
        socket = SockFindAndReferenceSocket( Handle, TRUE );

		recvInfo.TdiFlags = 0;	// see comments below

        if ( socket == NULL ) {
            err = WSAENOTSOCK;
            goto exit;
        }

        if (socket->SanSocket!=NULL) {

			//
			// Remember if this is OOB in case we do AsyncSelect handling
			// later on
			//
			if ( (*ReceiveFlags & MSG_OOB) != 0 ) {
				recvInfo.TdiFlags = TDI_RECEIVE_EXPEDITED;
			}

            err = SockSanRecv (
                    socket,
                    lpBuffers,
                    dwBufferCount,
                    lpNumberOfBytesRead,
                    ReceiveFlags,
                    lpOverlapped,
                    lpCompletionRoutine,
                    lpThreadId->ThreadHandle);
            goto exit;
        }
    }
#endif //_AFD_SAN_SWITCH_
    //
    // Set up the AFD_RECV_INFO structure.
    //

    recvInfo.BufferArray = lpBuffers;
    recvInfo.BufferCount = dwBufferCount;
    recvInfo.AfdFlags = 0;
    recvInfo.TdiFlags = 0;

    __try {
        if ( *ReceiveFlags == 0 ) {

            recvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;

        } else {

            //
            // The legal flags are MSG_OOB, MSG_PEEK, MSG_PARTIAL.  MSG_OOB is
            // not legal on datagram sockets.
            //

            if ( (*ReceiveFlags & ~(MSG_OOB | MSG_PEEK | MSG_PARTIAL)) != 0 ) {

                err = WSAEOPNOTSUPP;
                goto exit;

            }

            if ( (*ReceiveFlags & MSG_OOB) != 0 ) {

                recvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;

            } else {

                recvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;

            }

            if ( (*ReceiveFlags & MSG_PEEK) != 0 ) {

                recvInfo.TdiFlags |= TDI_RECEIVE_PEEK;

            }

            if ( (*ReceiveFlags & MSG_PARTIAL) != 0 ) {

                recvInfo.TdiFlags |= TDI_RECEIVE_PARTIAL;

            }


        }


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

                recvInfo.AfdFlags |= AFD_NO_FAST_IO;

            }

            //
            // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
            //

            ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

            //
            // Tell AFD this is an overlapped operation.
            //

            recvInfo.AfdFlags |= AFD_OVERLAPPED;

        }

        ioStatusBlock->Status = STATUS_PENDING;

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

    //
    // Receive the data on the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 event,
                 apcRoutine,
                 apcContext,
                 ioStatusBlock,
                 IOCTL_AFD_RECEIVE,
                 &recvInfo,
                 sizeof(recvInfo),
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
                      SOCK_RECEIVE_TIMEOUT
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
            status = ioStatusBlock->Status;
            if (status==STATUS_CANCELLED)
                status = STATUS_IO_TIMEOUT;
        }

    }

    __try {

        *ReceiveFlags = 0;

        //
        // Determine the completion status. In the overlapped
        // STATUS_BUFFER_OVERFLOW case we return PENDING rather
        // than an error because the completion routine/etc will
        // still get called, and we don't want to confuse the app.
        //

        switch( status ) {
        case STATUS_SUCCESS:
            break;

        case STATUS_PENDING :
            err = WSA_IO_PENDING;
            goto exit;      // bypass setting NumberOfBytesRead

        case STATUS_BUFFER_OVERFLOW:
            if ( lpOverlapped ) {
                err = WSA_IO_PENDING;
                goto exit;  // bypass setting NumberOfBytesRead
            }
            err = WSAEMSGSIZE;
            break;

        case STATUS_RECEIVE_EXPEDITED:
            *ReceiveFlags = MSG_OOB;
            break;

        case STATUS_RECEIVE_PARTIAL_EXPEDITED :
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB;
            break;

        case STATUS_RECEIVE_PARTIAL :
            *ReceiveFlags = MSG_PARTIAL;
            break;

        default:
            if( !NT_SUCCESS(status) ) {
                err = SockNtStatusToSocketError( status );
                goto exit;  // bypass setting NumberOfBytesRead
            }
            break;
        }

        //
        // It is possible that application deallocated lpOverlapped 
        // in another thread if completion port was used to receive
        // completion indication. We do not want to confuse the
        // application by returning failure, just pretend that we didn't
        // know about synchronous completion (note that if number of bytes
        // buffer is busted, it will cause and exception that will be
        // captured by the outer handler and return failure as expected).
        //
        __try {
            *lpNumberOfBytesRead = (DWORD)ioStatusBlock->Information;
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
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

exit:

    IF_DEBUG(RECEIVE) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPRecv on socket %lx failed: %ld (status %X).\n",
                           Handle, err, status ));

        } else {

#ifdef _AFD_SAN_SWITCH_
            WS_PRINT(( "WSPRecv on socket %lx succeeded, "
                       "bytes = %ld\n", Handle, *lpNumberOfBytesRead ));
#else
            WS_PRINT(( "WSPRecv on socket %lx succeeded, "
                       "bytes = %ld\n", Handle, ioStatusBlock->Information ));
#endif //_AFD_SAN_SWITCH_

        }

    }

    //
    // If there async select has been called in this process, get a
    // pointer to the socket information structure and reenable the
    // appropriate event.  We don't do this if no async thread as a
    // performance optimization.
    //

    if ( SockAsyncSelectCalled ) {
#ifdef _AFD_SAN_SWITCH_
        if (socket==NULL) {
            socket = SockFindAndReferenceSocket( Handle, TRUE );
        }
#else //_AFD_SAN_SWITCH_
        socket = SockFindAndReferenceSocket( Handle, TRUE );
#endif //_AFD_SAN_SWITCH_

        //
        // If the socket was found, reenable the right event.  If it
        // was not found, then presumably the socket handle was
        // invalid.
        //

        if ( socket != NULL ) {

            SockAcquireSocketLockExclusive( socket );

            if ( (recvInfo.TdiFlags & TDI_RECEIVE_EXPEDITED) != 0 ) {

                SockReenableAsyncSelectEvent( socket, FD_OOB );

            } else {

                SockReenableAsyncSelectEvent( socket, FD_READ );

            }

            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

        } else {

            WS_PRINT(( "WSPRecv: SockFindAndReferenceSocket failed.\n" ));

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

        WS_EXIT( "WSPRecv", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPRecv", 0, FALSE );
    return 0;

} // WSPRecv


int
WSPAPI
WSPRecvFrom(
    SOCKET Handle,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRead,
    LPDWORD ReceiveFlags,
    OUT struct sockaddr *SocketAddress,
    OUT int *SocketAddressLength,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used primarily on a connectionless socket specified by s.

    For overlapped sockets WSPRecv() is used to post one or more buffers into
    which incoming data will be placed as it becomes available, after which the
    WinSock SPI client-specified completion indication (invocation of the
    completion routine or setting of an event object) occurs. If the operation
    does not complete immediately, the final completion status is retrieved
    via the completion routine or WSPGetOverlappedResult(). Also note that the
    values pointed to by lpFrom and lpFromlen are not updated until completion
    is indicated. Applications must not use or disturb these values until they
    have been updated, therefore the client must not use automatic (i.e stack-
    based) variables for these parameters.

    If both lpOverlapped and lpCompletionRoutine are NULL, the socket in this
    routine will be treated as a non-overlapped socket.

    For non-overlapped sockets, the lpOverlapped, lpCompletionRoutine, and
    lpThreadId parameters are ignored. Any data which has already been received
    and buffered by the transport will be copied into the supplied user
    buffers. For the case of a blocking socket with no data currently having
    been received and buffered by the transport, the call will block until data
    is received.

    The supplied buffers are filled in the order in which they appear in the
    array pointed to by lpBuffers, and the buffers are packed so that no holes
    are created.

    The array of WSABUF structures pointed to by the lpBuffers parameter is
    transient. If this operation completes in an overlapped manner, it is the
    service provider's responsibility to capture this array of pointers to
    WSABUF structures before returning from this call. This enables WinSock SPI
    clients to build stack-based WSABUF arrays.

    For connectionless socket types, the address from which the data originated
    is copied to the buffer pointed by lpFrom. On input, the value pointed to
    by lpFromlen is initialized to the size of this buffer, and is modified on
    completion to indicate the actual size of the address stored there. As
    noted previously for overlapped sockets, the lpFrom and lpFromlen
    parameters are not updated until after the overlapped I/O has completed.
    The memory pointed to by these parameters must, therefore, remain available
    to the service provider and cannot be allocated on the WinSock SPI client's
    stack frame. The lpFrom and lpFromlen parameters are ignored for
    connection-oriented sockets.

    For byte stream style sockets (e.g., type SOCK_STREAM), incoming data is
    placed into the buffers until the buffers are filled, the connection is
    closed, or internally buffered data is exhausted. Regardless of whether or
    not the incoming data fills all the buffers, the completion indication
    occurs for overlapped sockets. For message-oriented sockets (e.g., type
    SOCK_DGRAM), an incoming message is placed into the supplied buffers, up
    to the total size of the buffers supplied, and the completion indication
    occurs for overlapped sockets. If the message is larger than the buffers
    supplied, the buffers are filled with the first part of the message. If the
    MSG_PARTIAL feature is supported by the service provider, the MSG_PARTIAL
    flag is set in lpFlags and subsequent receive operation(s) may be used to
    retrieve the rest of the message. If MSG_PARTIAL is not supported but the
    protocol is reliable, WSPRecvFrom() generates the error WSAEMSGSIZE and a
    subsequent receive operation with a larger buffer can be used to retrieve
    the entire message. Otherwise (i.e. the protocol is unreliable and does not
    support MSG_PARTIAL), the excess data is lost, and WSPRecvFrom() generates
    the error WSAEMSGSIZE.

    For connection-oriented sockets, WSPRecvFrom() can indicate the graceful
    termination of the virtual circuit in one of two ways, depending on whether
    the socket is a byte stream or message-oriented. For byte streams, zero
    bytes having been read indicates graceful closure and that no more bytes
    will ever be read. For message-oriented sockets, where a zero byte message
    is often allowable, a return error code of WSAEDISCON is used to indicate
    graceful closure. In any case a return error code of WSAECONNRESET
    indicates an abortive close has occurred.

    lpFlags may be used to influence the behavior of the function invocation
    beyond the options specified for the associated socket. That is, the
    semantics of this routine are determined by the socket options and the
    lpFlags parameter. The latter is constructed by or-ing any of the
    following values:

        MSG_PEEK - Peek at the incoming data. The data is copied into the
        buffer but is not removed from the input queue. This flag is valid
        only for non-overlapped sockets.

        MSG_OOB - Process out-of-band data.

        MSG_PARTIAL - This flag is for message-oriented sockets only. On
        output, indicates that the data supplied is a portion of the message
        transmitted by the sender. Remaining portions of the message will be
        supplied in subsequent receive operations. A subsequent receive
        operation with MSG_PARTIAL flag cleared indicates end of sender's
        message.

        As an input parameter, MSG_PARTIAL indicates that the receive
        operation should complete even if only part of a message has been
        received by the service provider.

        For message-oriented sockets, the MSG_PARTIAL bit is set in the lpFlags
        parameter if a partial message is received. If a complete message is
        received, MSG_PARTIAL is cleared  in lpFlags. In the case of delayed
        completion, the value pointed to by lpFlags is not updated. When
        completion has been indicated the WinSock SPI client should call
        WSPGetOverlappedResult() and examine the flags pointed to by the
        lpdwFlags parameter.

    If an overlapped operation completes immediately, WSPRecvFrom() returns a
    value of zero and the lpNumberOfBytesRecvd parameter is updated with the
    number of bytes received. If the overlapped operation is successfully
    initiated and will complete later, WSPRecvFrom() returns SOCKET_ERROR and
    indicates error code WSA_IO_PENDING. In this case, lpNumberOfBytesRecvd is
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
    posted buffers are guaranteed to be filled in the same order they are
    supplied.

Arguments:

    s - A descriptor identifying a socket.

    lpBuffers - A pointer to an array of WSABUF structures. Each WSABUF
        structure contains a pointer to a buffer and the length of the
        buffer.

    dwBufferCount - The number of WSABUF structures in the lpBuffers array.

    lpNumberOfBytesRecvd - A pointer to the number of bytes received by
        this call.

    lpFlags - A pointer to flags.

    lpFrom -  An optional pointer to a buffer which will hold the source
        address upon the completion of the overlapped operation.

    lpFromlen - A pointer to the size of the from buffer, required only if
        lpFrom is specified.

    lpOverlapped - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A pointer to the completion routine called when
        the receive operation has been completed.

    lpThreadId - A pointer to a thread ID structure to be used by the
        provider in a subsequent call to WPUQueueApc().The provider should
        store the referenced WSATHREADID structure (not the pointer to same)
        until after the WPUQueueApc() function returns.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs and the receive operation has completed immediately,
        WSPRecvFrom() returns 0. Note that in this case the completion routine,
        if specified will have already been queued. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno. The error code WSA_IO_PENDING indicates that the overlapped
        operation has been successfully initiated and that completion will be
        indicated at a later time. Any other error code indicates that no
        overlapped operations was initiated and no completion indication will
        occur.

--*/

{

    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK localIoStatusBlock;
    PIO_STATUS_BLOCK ioStatusBlock;
    int err;
    AFD_RECV_DATAGRAM_INFO recvInfo;
    HANDLE event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID apcContext;

    WS_ENTER( "WSPRecvFrom", (PVOID)Handle, lpBuffers, (PVOID)dwBufferCount, (PVOID)ReceiveFlags );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPRecvFrom", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        err = WSAENOTSOCK;
        goto exit;

    }
/*
    //
    // If this is a connected datagram socket, then it is not legal to
    // specify a destination address.
    //

    if ( IS_DGRAM_SOCK(socket) &&
         socket->State == SocketStateConnected &&
         (SocketAddress != NULL || SocketAddressLength != NULL) ) {

        err = WSAEISCONN;
        goto exit;

    }
*/
    //
    // This is only legal on bound sockets.
    //

    if ( socket->State == SocketStateOpen ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // If this is not a datagram socket or if the socket is connected,
    // just call WSPRecv() to process the call.
    //

    if ( !IS_DGRAM_SOCK(socket) ||
         ( SocketAddress == NULL && SocketAddressLength == NULL ) ) {

        INT ret;

        SockDereferenceSocket( socket );

        ret = WSPRecv(
                  Handle,
                  lpBuffers,
                  dwBufferCount,
                  lpNumberOfBytesRead,
                  ReceiveFlags,
                  lpOverlapped,
                  lpCompletionRoutine,
                  lpThreadId,
                  lpErrno
                  );

        WS_EXIT( "WSPRecvFrom", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
        return ret;

    }


    //
    // If data receive has been shut down, fail.
    //

    if ( socket->ReceiveShutdown ) {
	
		err = WSAESHUTDOWN;
        goto exit;
	
    }

    __try {

        //
        // Verify that we either got both components of an address, or
        // we got neither.
        //

        if( (SocketAddress == NULL) ^
            (SocketAddressLength == NULL || *SocketAddressLength == 0) ) {

	        err = WSAEFAULT;
            goto exit;

        }

        //
        // Only MSG_PEEK and MSG_PARTIAL are legal on WSPRecvFrom()
        // with a datagram socket.
        //

        if ( (*ReceiveFlags & ~(MSG_PEEK|MSG_PARTIAL)) != 0 ) {

            err = WSAEOPNOTSUPP;
            goto exit;

        }

        // Make sure that the address structure passed in is legitimate.  Since
        // it is an output parameter, all we really care about is that the
        // length of the buffer is sufficient.
        //

        if ( SocketAddressLength != NULL &&
            (LONG)*SocketAddressLength < (LONG)socket->HelperDll->MinSockaddrLength ) {
	    
            err = WSAEFAULT;
            goto exit;
	    
        }

        //
        //
        // Acquire the lock that protect this sockets.  We hold this lock
        // throughout this routine to synchronize against other callers
        // performing operations on the socket we're receiving data on.
        //
	    // All the checks above can be performed outside the lock.

        // SockAcquireSocketLockExclusive( socket );
        // SockReleaseSocketLock( socket );
        //
        // Set up the AFD_RECV_DATAGRAM_INFO structure.
        //

        recvInfo.BufferArray = lpBuffers;
        recvInfo.BufferCount = dwBufferCount;
        recvInfo.AfdFlags = 0;
        recvInfo.TdiFlags = TDI_RECEIVE_NORMAL;
        recvInfo.Address = SocketAddress;
        recvInfo.AddressLength = SocketAddressLength;

        if ( (*ReceiveFlags & MSG_PEEK) != 0 ) {

            recvInfo.TdiFlags |= TDI_RECEIVE_PEEK;

        }

        if ( (*ReceiveFlags & MSG_PARTIAL) != 0 ) {

            recvInfo.TdiFlags |= TDI_RECEIVE_PARTIAL;

        }

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

                recvInfo.AfdFlags |= AFD_NO_FAST_IO;

            }

            //
            // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
            //

            ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

            //
            // Tell AFD this is an overlapped operation.
            //

            recvInfo.AfdFlags |= AFD_OVERLAPPED;

        }

        ioStatusBlock->Status = STATUS_PENDING;

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

    //
    // Receive the data on the socket.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
                 event,
                 apcRoutine,
                 apcContext,
                 ioStatusBlock,
                 IOCTL_AFD_RECEIVE_DATAGRAM,
                 &recvInfo,
                 sizeof(recvInfo),
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
                      SOCK_RECEIVE_TIMEOUT
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

    __try {

        *ReceiveFlags = 0;

        //
        // Determine the completion status. In the overlapped
        // STATUS_BUFFER_OVERFLOW case we return PENDING rather
        // than an error because the completion routine/etc will
        // still get called, and we don't want to confuse the app.
        //

        switch( status ) {
        case STATUS_SUCCESS:
            break;
        case STATUS_PENDING :
            err = WSA_IO_PENDING;
            goto exit;      // bypass setting NumberOfBytesRead

        case STATUS_BUFFER_OVERFLOW:
            if ( lpOverlapped ) {
                err = WSA_IO_PENDING;
                goto exit;  // bypass setting NumberOfBytesRead
            }
            err = WSAEMSGSIZE;
            break;

        case STATUS_RECEIVE_EXPEDITED:
            *ReceiveFlags = MSG_OOB;
            break;

        case STATUS_RECEIVE_PARTIAL_EXPEDITED :
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB;
            break;

        case STATUS_RECEIVE_PARTIAL :
            *ReceiveFlags = MSG_PARTIAL;
            break;

        default:
            if( !NT_SUCCESS(status) ) {
                err = SockNtStatusToSocketError( status );
                goto exit;  // bypass setting NumberOfBytesRead
            }
            break;
        }

        //
        // Return the number of bytes transferred.
        // It is possible that application deallocated lpOverlapped 
        // in another thread if completion port was used to receive
        // completion indication. We do not want to confuse the
        // application by returning failure, just pretend that we didn't
        // know about synchronous completion (note that if number of bytes
        // buffer is busted, it will cause and exception that will be
        // captured by the outer handler and return failure as expected).
        //
        __try {
            *lpNumberOfBytesRead = (DWORD)ioStatusBlock->Information;
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

    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        goto exit;
    }

exit:

    IF_DEBUG(RECEIVE) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPRecvFrom on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPRecvFrom on socket %lx (%lx) succeeded, "
                       "bytes %ld",
                           Handle, socket, ioStatusBlock->Information ));
            WsPrintSockaddr( SocketAddress, SocketAddressLength );

        }

    }

    if ( socket != NULL ) {

		//
		// If there async select has been called in this process,
		// reenable the appropriate event.  We don't do this if 
		// no async thread as a performance optimazation.
		//

	    if ( SockAsyncSelectCalled ) {
		    SockAcquireSocketLockExclusive( socket );

			SockReenableAsyncSelectEvent( socket, FD_READ );

			SockReleaseSocketLock( socket );
		}

        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPRecvFrom", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPRecvFrom", 0, FALSE );
    return 0;

} // WSPRecvFrom
