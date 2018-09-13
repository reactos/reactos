/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module contains stubbed-out unimplemented SPI routines for the
    Winsock 2 to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPSend()
        WSPSendDisconnect()
        WSPSendTo()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private types.
//

typedef struct _SOCK_OVERLAPPED_SEND_CONTEXT {

    //
    // Links onto the queue list.
    //

    LIST_ENTRY QueueListEntry;

    //
    // Captured WSABUF array.
    //

    LPWSABUF CapturedBufferArray;
    DWORD CapturedBufferCount;

    //
    // Send flags.
    //

    DWORD Flags;

    //
    // send[to] flag.
    //

    BOOL SendTo;

    //
    // Address info (for WSPSendTo() only).
    //

    LPSOCKADDR SocketAddress;
    INT SocketAddressLength;

    //
    // Overlapped completion info.
    //

    LPWSAOVERLAPPED Overlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    WSATHREADID ThreadId;

} SOCK_OVERLAPPED_SEND_CONTEXT, *PSOCK_OVERLAPPED_SEND_CONTEXT;


//
// Private prototypes.
//

PSOCK_OVERLAPPED_SEND_CONTEXT
SockBuildSendContext(
    LPWSABUF BufferArray,
    DWORD BufferCount,
    DWORD Flags,
    LPSOCKADDR SocketAddress,
    INT SocketAddressLength,
    LPWSAOVERLAPPED Overlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    LPWSATHREADID ThreadId,
    BOOL SendTo
    );

INT
SockGatherSend(
    PSOCKET_INFORMATION SocketInfo,
    LPWSABUF BufferArray,
    DWORD BufferCount,
    DWORD Flags,
    LPDWORD NumberOfBytesSent,
    LPSOCKADDR SocketAddress,
    INT SocketAddressLength,
    BOOL SendTo,
    BOOL Overlapped
    );

DWORD
WINAPI
SockOverlappedSendThread(
    LPVOID Param
    );

#define SockFreeSendContext(context) SOCK_FREE_HEAP(context)


//
// Public functions.
//


INT
WSPAPI
WSPSend(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
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

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPSend", (PVOID)s, lpBuffers, (PVOID)dwBufferCount, lpNumberOfBytesSent );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPSend", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SEND) {

            SOCK_PRINT((
                "WSPSend failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPSend", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Filter out any options we don't yet support.
    //

    if( lpBuffers == NULL ||
        dwBufferCount == 0 ||
        lpNumberOfBytesSent == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    if( dwFlags & ~( MSG_OOB | MSG_DONTROUTE ) != 0 ) {

        err = WSAEOPNOTSUPP;
        goto exit;

    }

    //
    // If this is an overlapped request, build a context and post it
    // to the worker thread.
    //

    if( lpOverlapped != NULL &&
        ( socketInfo->CreationFlags & WSA_FLAG_OVERLAPPED ) != 0 ) {

        PSOCK_OVERLAPPED_SEND_CONTEXT context;
        HANDLE threadHandle;
        DWORD threadId;

        context = SockBuildSendContext(
                      lpBuffers,
                      dwBufferCount,
                      dwFlags,
                      NULL,
                      0,
                      lpOverlapped,
                      lpCompletionRoutine,
                      lpThreadId,
                      FALSE
                      );

        if( context == NULL ) {

            err = WSAENOBUFS;
            goto exit;

        }

        //
        // Acquire the lock protecting the socket.
        //

        SockAcquireSocketLock( socketInfo );

        //
        // Initialize the worker thread if necessary.
        //

        if( !SockInitializeOverlappedThread(
                socketInfo,
                &socketInfo->OverlappedSend,
                &SockOverlappedSendThread
                ) ) {

            err = WSAENOBUFS;
            SockReleaseSocketLock( socketInfo );
            goto exit;

        }

        //
        // Queue the request to the worker, then kick it.
        //

        InsertTailList(
            &socketInfo->OverlappedSend.QueueListHead,
            &context->QueueListEntry
            );

        SetEvent( &socketInfo->OverlappedSend.WakeupEvent );

        //
        // Release the socket lock, then "fail" this request with
        // WSA_IO_PENDING.
        //

        SockReleaseSocketLock( socketInfo );

        err = WSA_IO_PENDING;
        SOCK_OVERLAPPED_TO_IO_STATUS( lpOverlapped )->Status = err;

        goto exit;

    }

    //
    // Let SockGatherSend() do the dirty work.
    //

    err = SockGatherSend(
              socketInfo,
              lpBuffers,
              dwBufferCount,
              dwFlags,
              lpNumberOfBytesSent,
              NULL,
              0,
              FALSE,
              FALSE
              );

    if( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    result = 0;

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPSend", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPSend



INT
WSPAPI
WSPSendDisconnect(
    IN SOCKET s,
    IN LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno
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

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPSendDisconnect", (PVOID)s, (PVOID)lpOutboundDisconnectData, lpErrno, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPSendDisconnect", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SEND) {

            SOCK_PRINT((
                "WSPSendDisconnect failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPSendDisconnect", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Filter out any options we don't yet support.
    //

    if( lpOutboundDisconnectData != NULL ) {

        err = WSAENOPROTOOPT;
        goto exit;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->shutdown(
                 socketInfo->WS1Handle,
                 SD_SEND
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

    SOCK_EXIT( "WSPSendDisconnect", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPSendDisconnect



INT
WSPAPI
WSPSendTo(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN DWORD dwFlags,
    IN const struct sockaddr FAR * lpTo,
    IN int iTolen,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
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

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPSendTo", (PVOID)s, lpBuffers, (PVOID)dwBufferCount, lpNumberOfBytesSent );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPSendTo", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SEND) {

            SOCK_PRINT((
                "WSPSendTo failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPSendTo", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Filter out any options we don't yet support.
    //

    if( lpBuffers == NULL ||
        dwBufferCount == 0 ||
        lpNumberOfBytesSent == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    if( dwFlags & ~( MSG_OOB | MSG_DONTROUTE ) != 0 ) {

        err = WSAEOPNOTSUPP;
        goto exit;

    }

    //
    // If this is an overlapped request, build a context and post it
    // to the worker thread.
    //

    if( lpOverlapped != NULL &&
        ( socketInfo->CreationFlags & WSA_FLAG_OVERLAPPED ) != 0 ) {

        PSOCK_OVERLAPPED_SEND_CONTEXT context;
        HANDLE threadHandle;
        DWORD threadId;

        context = SockBuildSendContext(
                      lpBuffers,
                      dwBufferCount,
                      dwFlags,
                      (LPSOCKADDR)lpTo,
                      iTolen,
                      lpOverlapped,
                      lpCompletionRoutine,
                      lpThreadId,
                      TRUE
                      );

        if( context == NULL ) {

            err = WSAENOBUFS;
            goto exit;

        }

        //
        // Acquire the lock protecting the socket.
        //

        SockAcquireSocketLock( socketInfo );

        //
        // Initialize the worker thread if necessary.
        //

        if( !SockInitializeOverlappedThread(
                socketInfo,
                &socketInfo->OverlappedSend,
                &SockOverlappedSendThread
                ) ) {

            err = WSAENOBUFS;
            SockReleaseSocketLock( socketInfo );
            goto exit;

        }

        //
        // Queue the request to the worker, then kick it.
        //

        InsertTailList(
            &socketInfo->OverlappedSend.QueueListHead,
            &context->QueueListEntry
            );

        SetEvent( &socketInfo->OverlappedSend.WakeupEvent );

        //
        // Release the socket lock, then "fail" this request with
        // WSA_IO_PENDING.
        //

        SockReleaseSocketLock( socketInfo );

        err = WSA_IO_PENDING;
        SOCK_OVERLAPPED_TO_IO_STATUS( lpOverlapped )->Status = err;

        goto exit;

    }

    //
    // Let SockGatherSend() do the dirty work.
    //

    err = SockGatherSend(
              socketInfo,
              lpBuffers,
              dwBufferCount,
              dwFlags,
              lpNumberOfBytesSent,
              (LPSOCKADDR)lpTo,
              iTolen,
              TRUE,
              FALSE
              );

    if( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    result = 0;

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPSendTo", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPSendTo



//
// Private functions.
//

PSOCK_OVERLAPPED_SEND_CONTEXT
SockBuildSendContext(
    LPWSABUF BufferArray,
    DWORD BufferCount,
    DWORD Flags,
    LPSOCKADDR SocketAddress,
    INT SocketAddressLength,
    LPWSAOVERLAPPED Overlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    LPWSATHREADID ThreadId,
    BOOL SendTo
    )

/*++

Routine Description:

    Captures all data necessary to defer an overlapped send request
    and builds a context record containing that data.

Arguments:

    BufferArray - Pointer to an array of WSABUF structures.

    BufferCount - The number of WSABUF structures in the array.

    Flags - Send flags (MSG_*).

    SocketAddress - Pointer to a SOCKADDR structure that will receive
        the source address of the received data. This is for WSPRecvFrom()
        only.

    SocketAddressLength - The length of SocketAddress.

    Overlapped - Pointer to the WSAOVERLAPPED structure for this request.

    CompletionRoutine - Pointer to a completion routine to invoke after
        the operation is complete.

    ThreadId - Identifies the thread for this request.

    SendTo - TRUE if this is for a sendto() request, otherwise it's for
        a send() request.

Return Value:

    PSOCK_OVERLAPPED_SEND_CONTEXT - Pointer to the newly created
        context if successful, NULL otherwise.

--*/

{

    PSOCK_OVERLAPPED_SEND_CONTEXT context;
    DWORD bytesRequired;

    //
    // Sanity check.
    //

    SOCK_ASSERT( BufferArray != NULL );
    SOCK_ASSERT( BufferCount > 0 );
    SOCK_ASSERT( ThreadId != NULL );

    //
    // To make things a bit more efficient, we'll allocate the context
    // block and the captured WSABUF array in a single heap allocation.
    //

    bytesRequired = sizeof(*context) + ( BufferCount * sizeof(WSABUF) );

    context = SOCK_ALLOCATE_HEAP( bytesRequired );

    if( context != NULL ) {

        //
        // Initialize it.
        //

        context->CapturedBufferArray = (LPWSABUF)( context + 1 );
        context->CapturedBufferCount = BufferCount;

        CopyMemory(
            context->CapturedBufferArray,
            BufferArray,
            BufferCount * sizeof(WSABUF)
            );

        context->Flags = Flags;
        context->SendTo = SendTo;
        context->SocketAddress = SocketAddress;
        context->SocketAddressLength = SocketAddressLength;
        context->Overlapped = Overlapped;
        context->CompletionRoutine = CompletionRoutine;
        context->ThreadId = *ThreadId;

    }

    return context;

}   // SockBuildSendContext



INT
SockGatherSend(
    PSOCKET_INFORMATION SocketInfo,
    LPWSABUF BufferArray,
    DWORD BufferCount,
    DWORD Flags,
    LPDWORD NumberOfBytesSent,
    LPSOCKADDR SocketAddress,
    INT SocketAddressLength,
    BOOL SendTo,
    BOOL Overlapped
    )

/*++

Routine Description:

    Worker routine for WSPSend(), WSPSendTo(), and SockOverlappedSendThread().
    This routine is responsible for taking an array of buffers and sending
    them as a single buffer.

Arguments:

    SocketInfo - Pointer to the SOCKET_INFORMATION describing the socket
        to send.

    BufferArray - Pointer to an array of WSABUF structures.

    BufferCount - The number of entries in BufferArray.

    Flags - Flags to be passed to the send() or sendto() API.

    NumberOfBytesSent - Will receive the total number of bytes actually
        sent.

    SocketAddress - (sendto() only) Points to the destination address
        for the data.

    SocketAddressLength - (sendto() only) The length of SocketAddress.

    SendTo - TRUE if this is a sendto() operation, FALSE if this is a
        send() operation.

    Overlapped - TRUE if this is an overlapped operation, FALSE
        otherwise.

Return Value:

    INT - 0 if successful, WSAE* if not.

--*/

{

    PVOID bigBuffer;
    DWORD bigBufferLength;
    INT err;
    INT result;
    SOCKET ws1Handle;

    //
    // Sanity check.
    //

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( BufferArray != NULL );
    SOCK_ASSERT( BufferCount > 0 );
    SOCK_ASSERT( NumberOfBytesSent != NULL );

    //
    // Setup.
    //

    err = NO_ERROR;

    ws1Handle = SocketInfo->WS1Handle;
    SOCK_ASSERT( ws1Handle != INVALID_SOCKET );

    //
    // Allocate the giant buffer if necessary.
    //

    if( BufferCount == 1 ) {

        bigBuffer = BufferArray->buf;
        bigBufferLength = BufferArray->len;

    } else {

        DWORD bytesCopied;

        bigBufferLength = SockCalcBufferArrayByteLength(
                              BufferArray,
                              BufferCount
                              );

        bigBuffer = SOCK_ALLOCATE_HEAP( bigBufferLength );

        if( bigBuffer == NULL ) {

            return WSAENOBUFS;

        }

        //
        // Copy the user's data into the big buffer.
        //

        bytesCopied = SockCopyBufferArrayToFlatBuffer(
                          bigBuffer,
                          bigBufferLength,
                          BufferArray,
                          BufferCount
                          );

        SOCK_ASSERT( bytesCopied == (DWORD)bigBufferLength );

    }

    //
    // Do the send.
    //

    SockPrepareForBlockingHook( SocketInfo );
    SockPreApiCallout();

    for( ; ; ) {

        if( SendTo ) {

            result = SocketInfo->Hooker->sendto(
                         ws1Handle,
                         (char *)bigBuffer,
                         (int)bigBufferLength,
                         (int)Flags,
                         SocketAddress,
                         SocketAddressLength
                         );

        } else {

            result = SocketInfo->Hooker->send(
                         ws1Handle,
                         (char *)bigBuffer,
                         (int)bigBufferLength,
                         (int)Flags
                         );

        }

        if( result == SOCKET_ERROR ) {

            err = SocketInfo->Hooker->WSAGetLastError();
            SOCK_ASSERT( err != NO_ERROR );

            if( err == WSAEWOULDBLOCK && Overlapped ) {

                FD_SET writefds;

                //
                // The send[to]() failed with WSAEWOULDBLOCK (cannot send
                // at this time) and this is an overlapped operation on a
                // non-blocking socket. For overlapped sends, WSAEWOULDBLOCK
                // is meaningless, so we'll just call select() to wait for
                // the socket to become writeable and then reissue the
                // send[to]().
                //

                FD_ZERO( &writefds );
                FD_SET( ws1Handle, &writefds );

                result = SocketInfo->Hooker->select(
                             1,
                             NULL,
                             &writefds,
                             NULL,
                             NULL
                             );

                if( result == 1 ) {

                    //
                    // Got a socket.
                    //

                    SOCK_ASSERT( FD_ISSET( ws1Handle, &writefds ) );
                    continue;

                }

            }

            SockPostApiCallout();
            goto complete;

        } else {

            SOCK_ASSERT( result >= 0 );
            break;

        }

    }

    SockPostApiCallout();

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    *NumberOfBytesSent = (DWORD)result;

complete:

    if( BufferCount > 1 ) {

        SOCK_ASSERT( bigBuffer != BufferArray->buf );
        SOCK_FREE_HEAP( bigBuffer );

    } else {

        SOCK_ASSERT( bigBuffer == BufferArray->buf );

    }

    return err;

}   // SockGatherSend



DWORD
WINAPI
SockOverlappedSendThread(
    LPVOID Param
    )

/*++

Routine Description:

    This is the worker thread for overlapped send operations. It
    waits for overlapped send requests, then processes them.

Arguments:

    Param - Actually a pointer to the SOCKET_INFORMATION for the
        target socket.

Return Value:

    DWORD - Unused, always 0.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    PSOCK_OVERLAPPED_SEND_CONTEXT context;
    PSOCK_OVERLAPPED_DATA overlappedData;
    PLIST_ENTRY listEntry;
    DWORD result;
    DWORD bytesSent;
    INT err;
    HANDLE handles[2];

    //
    // Snag the socket info from the thread parameter.
    //

    socketInfo = (PSOCKET_INFORMATION)Param;
    SOCK_ASSERT( socketInfo != NULL );

    overlappedData = &socketInfo->OverlappedSend;

    //
    // Build the handle array we'll wait on.
    //

    handles[0] = overlappedData->WakeupEvent;
    handles[1] = socketInfo->ShutdownEvent;

    SOCK_ASSERT( handles[0] != NULL && handles[1] != NULL );

    //
    // Loop forever, waiting for receive requests to process or
    // the shutdown event.
    //

    for( ; ; ) {

        result = WaitForMultipleObjects(
                     2,
                     handles,
                     FALSE,
                     INFINITE
                     );

        if( result == WAIT_OBJECT_0 ) {

            //
            // We've got work to do. Acquire the socket lock and loop
            // through the queued work items.
            //

            SockAcquireSocketLock( socketInfo );

            while( !IsListEmpty( &overlappedData->QueueListHead ) ) {

                //
                // Pull the first item off the list.
                //

                listEntry = RemoveHeadList( &overlappedData->QueueListHead );

                context = CONTAINING_RECORD(
                              listEntry,
                              SOCK_OVERLAPPED_SEND_CONTEXT,
                              QueueListEntry
                              );

                //
                // Release the socket lock and call the actual send
                // routine.
                //

                SockReleaseSocketLock( socketInfo );

                err = SockGatherSend(
                          socketInfo,
                          context->CapturedBufferArray,
                          context->CapturedBufferCount,
                          context->Flags,
                          &bytesSent,
                          context->SocketAddress,
                          context->SocketAddressLength,
                          context->SendTo,
                          TRUE
                          );

                //
                // Complete the request.
                //

                SockCompleteRequest(
                    socketInfo,
                    (DWORD)err,
                    bytesSent,
                    context->Overlapped,
                    context->CompletionRoutine,
                    &context->ThreadId
                    );

                //
                // Free the context.
                //

                SockFreeSendContext( context );

                //
                // Reacquire the socket lock, then loop around and try
                // for more requests.
                //

                SockAcquireSocketLock( socketInfo );

            }

            //
            // Release the socket lock, then loop around and wait for
            // more work requests.
            //

            SockReleaseSocketLock( socketInfo );

        } else if( result == ( WAIT_OBJECT_0 + 1 ) ) {

            //
            // The socket is getting shut down.
            //

            break;

        } else {

            //
            // Bad news, WaitForMultipleObjects() failed.
            //

            SOCK_PRINT((
                "SockOverlappedSendThread: wait failure %ld (%08lx)\n",
                result,
                result
                ));

            break;

        }

    }

    //
    // SockInitializeOverlappedThread() added a special reference to the
    // socket just for us, so we'll remove it here.
    //

    SockDereferenceSocket( socketInfo );

    //
    // Just to make the compiler happy...
    //

    return 0;

}   // SockOverlappedSendThread

