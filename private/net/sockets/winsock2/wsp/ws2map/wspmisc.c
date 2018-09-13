/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    wspmisc.c

Abstract:

    This module contains miscellaneous routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPCancelBlockingCall()
        WSPDuplicateSocket()
        WSPGetOverlappedResult()
        WSPGetQOSByName()
        WSPJoinLeaf()

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
WSPCancelBlockingCall(
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine cancels any outstanding blocking operation for this thread.
    It is normally used in two situations:

        1. A WinSock SPI client is processing a message which has been
           received while a service provider is implementing pseudo
           blocking. In this case, WSAIsBlocking() will be true.

        2. A blocking call is in progress, and the WinSock service
           provider has called back to the WinSock SPI client's "blocking
           hook" function (via the callback function retrieved from
           WPUQueryBlockingCallback()), which in turn is invoking this
           function. Such a situation might arise, for instance, in
           implementing a Cancel option for an operation which require an
           extended time to complete.

    In each case, the original blocking call will terminate as soon as
    possible with the error WSAEINTR. (In (1), the termination will not take
    place until Windows message scheduling has caused control to revert back
    to the pseudo blocking routine in WinSock. In (2), the blocking call
    will be terminated as soon as the blocking hook function completes.)

    In the case of a blocking WSPConnect() operation, WinSock will terminate
    the blocking call as soon as possible, but it may not be possible for
    the socket resources to be released until the connection has completed
    (and then been reset) or timed out. This is likely to be noticeable only
    if the WinSock SPI client immediately tries to open a new socket (if no
    sockets are available), or to WSPConnect() to the same peer.

    Canceling an WSPAccept() or a WSPSelect() call does not adversely impact
    the sockets passed to these calls. Only the particular call fails; any
    operation that was legal before the cancel is legal after the cancel,
    and the state of the socket is not affected in any way.

    Canceling any operation other than WSPAccept() and WSPSelect() can leave
    the socket in an indeterminate state. If a WinSock SPI client cancels a
    blocking operation on a socket, the only operation that the WinSock SPI
    client can depend on being able to perform on the socket is a call to
    WSPCloseSocket(), although other operations may work on some WinSock
    service providers. If a WinSock SPI client desires maximum portability,
    it must be careful not to depend on performing operations after a cancel.
    A WinSock SPI client may reset the connection by setting the timeout on
    SO_LINGER to 0 and calling WSPCloseSocket().

    If a cancel operation compromised the integrity of a SOCK_STREAM's data
    stream in any way, the WinSock provider will reset the connection and
    fail all future operations other than WSPCloseSocket() with
    WSAECONNABORTED.

    Note it is acceptable for WSPCancelBlockingCall() to return successfully
    if the blocking network operation completes prior to being canceled. In
    this case, the blocking operation will return successfully as if
    WSPCancelBlockingCall() had never been called. The only way for the
    WinSock SPI client to know with certainty that an operation was actually
    canceled is to check for a return code of WSAEINTR from the blocking call.

Arguments:

    lpErrno - A pointer to the error code.

Return Value:

    The value returned by WSPCancelBlockingCall() is 0 if the operation was
        successfully canceled. Otherwise the value SOCKET_ERROR is returned,
        and a specific error code is available in lpErrno.

--*/

{

    INT err;
    INT result;
    PSOCK_TLS_DATA tlsData;

    SOCK_ENTER( "WSPCancelBlockingCall", lpErrno, NULL, NULL, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, TRUE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPCancelBlockingCall", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    tlsData = SOCK_GET_THREAD_DATA();
    SOCK_ASSERT( tlsData != NULL );

    //
    // This call is only valid when we are in a blocking call.
    //

    if( !tlsData->IsBlocking ) {

        *lpErrno = WSAEINVAL;
        SOCK_EXIT( "WSPCancelBlockingCall", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // The IO should not have been cancelled yet.
    //

    SOCK_ASSERT( tlsData->ReentrancyFlag );
    SOCK_ASSERT( !tlsData->IoCancelled );
    SOCK_ASSERT( tlsData->BlockingSocketInfo != NULL );

    //
    // Cancel it.
    //

    result = tlsData->BlockingSocketInfo->Hooker->WSACancelBlockingCall();

    if( result == SOCKET_ERROR ) {

        *lpErrno = tlsData->BlockingSocketInfo->Hooker->WSAGetLastError();
        SOCK_EXIT( "WSPCancelBlockingCall", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Remember that we've cancelled it.
    //

    tlsData->IoCancelled = TRUE;

    SOCK_EXIT( "WSPCancelBlockingCall", NO_ERROR, NULL );
    return NO_ERROR;

}   // WSPCancelBlockingCall



INT
WSPAPI
WSPDuplicateSocket(
    IN SOCKET s,
    IN DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    A source process calls WSPDuplicateSocket() to obtain a special
    WSAPROTOCOL_INFOW structure. It uses some interprocess communications
    (IPC) mechanism to pass the contents of this structure to a target
    process, which in turn uses it in a call to WSPSocket() to obtain a
    descriptor for the duplicated socket. Note that the special
    WSAPROTOCOL_INFOW structure may only be used once by the target process.

    It is the service provider's responsibility to perform whatever operations
    are needed in the source process context and to create a WSAPROTOCOL_INFOW
    structure that will be recognized when it subsequently appears as a
    parameter to WSPSocket() in the target processes' context. The provider
    must then return a socket descriptor that references a common underlying
    socket. The dwProviderReserved field of the WSAPROTOCOL_INFOW struct is
    available for the service provider's use, and may be used to store any
    useful context information, including a duplicated handle.

    When new socket descriptors are allocated IFS providers must call
    WPUModifyIFSHandle() and non-IFS providers must call
    WPUCreateSocketHandle().

    The descriptors that reference a shared socket may be used independently
    as far as I/O is concerned. However, the WinSock interface does not
    implement any type of access control, so it is up to the processes
    involved to coordinate their operations on a shared socket. A typical use
    for shared sockets is to have one process that is responsible for
        creating sockets and establishing connections, hand off sockets to
        other processes which are responsible for information exchange.

    Since what is duplicated are the socket descriptors and not the underlying
    socket, all of the state associated with a socket is held in common across
    all the descriptors. For example a WSPSetSockOpt() operation performed
    using one descriptor is subsequently visible using a WSPGetSockOpt() from
    any or all descriptors. A process may call WSPClosesocket() on a
    duplicated socket and the descriptor will become deallocated. The
    underlying socket, however, will remain open until WSPClosesocket() is
    called by the last remaining descriptor.

    Notification on shared sockets is subject to the usual constraints of
    WSPAsyncSelect() and WSPEventSelect().  Issuing either of these calls
    using any of the shared descriptors cancels any previous event
    registration for the socket, regardless of which descriptor was used to
    make that registration. Thus, for example, a shared socket cannot deliver
    FD_READ events to process A and FD_WRITE events to process B. For
    situations when such tight coordination is required, it is suggested that
    developers use threads instead of separate processes.

Arguments:

    s - Specifies the local socket descriptor.

    dwProcessId - Specifies the ID of the target process for which the
        shared socket will be used.

    lpProtocolInfo - A pointer to a buffer allocated by the client that
        is large enough to contain a WSAPROTOCOL_INFOW struct. The service
        provider copies the protocol info struct contents to this buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPDuplicateSocket() returns zero. Otherwise, the
        value of SOCKET_ERROR is returned, and a specific error number is
        available in lpErrno.

--*/

{

    *lpErrno = WSAEINVAL;
    return SOCKET_ERROR;

}   // WSPDuplicateSocket



BOOL
WSPAPI
WSPGetOverlappedResult(
    IN SOCKET s,
    IN LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    The results reported by the WSPGetOverlappedResult() function are those
    of the specified socket's last overlapped operation to which the specified
    WSAOVERLAPPED structure was provided, and for which the operation's results
    were pending. A pending operation is indicated when the function that
    started the operation returns FALSE, and the lpErrno is WSA_IO_PENDING.
    When an I/O operation is pending, the function that started the operation
    resets the hEvent member of the WSAOVERLAPPED structure to the nonsignaled
    state. Then when the pending operation has been completed, the system sets
    the event object to the signaled state.

    If the fWait parameter is TRUE, WSPGetOverlappedResult() determines whether
    the pending operation has been completed by blocking and waiting for the
    event object to be in the signaled state.

Arguments:

    s - Identifies the socket. This is the same socket that was specified
        when the overlapped operation was started by a call to WSPRecv(),
        WSPRecvFrom(), WSPSend(), WSPSendTo(), or WSPIoctl().

    lpOverlapped - Points to a WSAOVERLAPPED structure that was specified
        when the overlapped operation was started.

    lpcbTransfer - Points to a 32-bit variable that receives the number of
        bytes that were actually transferred by a send or receive operation,
        or by WSPIoctl().

    fWait - Specifies whether the function should wait for the pending
        overlapped operation to complete. If TRUE, the function does not
        return until the operation has been completed. If FALSE and the
        operation is still pending, the function returns FALSE and lpErrno
        is WSA_IO_INCOMPLETE.

    lpdwFlags - Points to a 32-bit variable that will receive one or more
        flags that supplement the completion status. If the overlapped
        operation was initiated via WSPRecv() or WSPRecvFrom(), this
        parameter will contain the results value for lpFlags parameter.

    lpErrno - A pointer to the error code.

Return Value:

    If WSPGetOverlappedResult() succeeds, the return value is TRUE. This
        means that the overlapped operation has completed successfully
        and that the value pointed to by lpcbTransfer has been updated.
        If WSPGetOverlappedResult() returns FALSE, this means that either
        the overlapped operation has not completed or the overlapped
        operation completed but with errors, or that completion status
        could not be determined due to errors in one or more parameters
        to WSPGetOverlappedResult(). On failure, the value pointed to by
        lpcbTransfer will not be updated. lpErrno indicates the cause of
        the failure (either of WSPGetOverlappedResult() or of the
        associated overlapped operation).

--*/

{

    PSOCKET_INFORMATION socketInfo;
    PSOCK_IO_STATUS ioStatus;
    INT err;
    BOOL result;

    SOCK_ENTER( "WSPGetOverlappedResult", (PVOID)s, lpOverlapped, lpcbTransfer, (PVOID)fWait );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetOverlappedResult", FALSE, lpErrno );
        return FALSE;

    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    result = FALSE;

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(OVERLAP) {

            SOCK_PRINT((
                "WSPGetOverlappedResult failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "GetOverlappedResult", FALSE, lpErrno );
        return FALSE;

    }

    //
    // Validate the other arguments.
    //

    if( lpOverlapped == NULL ||
        lpcbTransfer == NULL ||
        lpdwFlags == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Interpret the completion status (if any).
    //

    ioStatus = SOCK_OVERLAPPED_TO_IO_STATUS( lpOverlapped );

    if( ioStatus->Status == WSA_IO_PENDING ) {

        if( fWait ) {

            WaitForSingleObject(
                lpOverlapped->hEvent,
                INFINITE
                );

        } else {

            err = WSA_IO_INCOMPLETE;
            goto exit;

        }

    }

    //
    // At this point, ioStatus->Status should have been updated from
    // the IO completion (in SockCompleteRequest()).
    //

    SOCK_ASSERT( ioStatus->Status != WSA_IO_PENDING );

    err = (INT)ioStatus->Status;

    if( err != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

    *lpcbTransfer = ioStatus->Information;
    *lpdwFlags = 0;
    result = TRUE;

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = FALSE;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSAGetOverlappedResult", result, result ? NULL : lpErrno );
    return result;

}   // WSPGetOverlappedResult



BOOL
WSPAPI
WSPGetQOSByName(
    IN SOCKET s,
    IN LPWSABUF lpQOSName,
    OUT LPQOS lpQOS,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    Clients may use this routine to initialize a QOS structure to a set of
    known values appropriate for a particular service class or media type.
    These values are stored in a template which is referenced by a well-known
    name

Arguments:

    s - A descriptor identifying a socket.

    lpQOSName - Specifies the QOS template name.

    lpQOS - A pointer to the QOS structure to be filled.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPGetQOSByName() returns TRUE. Otherwise, a value of
        FALSE is returned, and a specific error code is available in
        lpErrno.

--*/

{

    INT err;
    DWORD bytesReturned;

    SOCK_ENTER( "WSPGetQOSByName", (PVOID)s, lpQOSName, lpQOS, lpErrno );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetQOSByName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // We'll take the totally cheesy way out and just return the
    // default QOS structure associated with the incoming socket.
    //

    if( WSPIoctl(
            s,
            SIO_GET_QOS,
            NULL,
            0,
            lpQOS,
            sizeof(*lpQOS),
            &bytesReturned,
            NULL,
            NULL,
            NULL,
            &err
            ) == SOCKET_ERROR ) {

        SOCK_ASSERT( err != NO_ERROR );
        goto exit;

    }

    SOCK_ASSERT( err == NO_ERROR );

exit:

    IF_DEBUG(QOS) {

        if ( err != NO_ERROR ) {

            SOCK_PRINT((
                "WSPGetQOSByName on socket %lx failed: %ld.\n",
                s,
                err
                ));

        } else {

            SOCK_PRINT((
                "WSPGetQOSByName on socket %lx succeeded\n",
                s
                ));

        }

    }

    if ( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetQOSByName", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    SOCK_EXIT( "WSPGetQOSByName", 0, NULL );
    return 0;

}   // WSPGetQOSByName



SOCKET
WSPAPI
WSPJoinLeaf(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen,
    IN LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    IN DWORD dwFlags,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used to join a leaf node to a multipoint session, and to
    perform a number of other ancillary operations that occur at session join
    time as well. If the socket, s, is unbound, unique values are assigned to
    the local association by the system, and the socket is marked as bound.

    WSPJoinLeaf() has the same parameters and semantics as WSPConnect() except
    that it returns a socket descriptor (as in WSPAccept()), and it has an
    additional dwFlags parameter. Only multipoint sockets created using
    WSPSocket() with appropriate multipoint flags set may be used for input
    parameter s in this routine. If the socket is in the non-blocking mode,
    the returned socket descriptor will not be useable until after a
    corresponding FD_CONNECT indication has been received, except that
    closesocket() may be invoked on this socket descriptor to cancel a pending
    join operation. A root node in a multipoint session may call WSPJoinLeaf()
    one or more times in order to add a number of leaf nodes, however at most
    one multipoint connection request may be outstanding at a time. Refer to
    section 3.14. Protocol-Independent Multicast and Multipoint for additional
    information.

    The socket descriptor returned by WSPJoinLeaf() is different depending on
    whether the input socket descriptor, s, is a c_root or a c_leaf. When used
    with a c_root socket, the name parameter designates a particular leaf node
    to be added and the returned  socket descriptor is a c_leaf socket
    corresponding to the newly added leaf node. The newly created socket has
    the same properties as s including asynchronous events registered with
    WSPAsyncSelect() or with WSPEventSelect(), but not including the c_root
    socket's group ID, if any. It is not intended to be used for exchange of
    multipoint data, but rather is used to receive network event indications
    (e.g. FD_CLOSE) for the connection that exists to the particular c_leaf.
    Some multipoint implementations may also allow this socket to be used for
    "side chats" between the root and an individual leaf node. An FD_CLOSE
    indication will be received for this socket if the corresponding leaf node
    calls WSPCloseSocket() to drop out of the multipoint session.
    Symmetrically, invoking WSPCloseSocket() on the c_leaf socket returned from
    WSPJoinLeaf() will cause the socket in the corresponding leaf node to get
    FD_CLOSE notification.

    When WSPJoinLeaf() is invoked with a c_leaf socket, the name parameter
    contains the address of the root node (for a rooted control scheme) or an
    existing multipoint session (non-rooted control scheme), and the returned
    socket descriptor is the same as the input socket descriptor. In other
    words, a new socket descriptor is not allocated. In a rooted control
    scheme, the root application would put its c_root socket in the listening
    mode by calling WSPListen(). The standard FD_ACCEPT notification will be
    delivered when the leaf node requests to join itself to the multipoint
    session. The root application uses the usual WSPAccept() functions to
    admit the new leaf node. The value returned from WSPAccept() is also a
    c_leaf socket descriptor just like those returned from WSPJoinLeaf(). To
    accommodate multipoint schemes that allow both root-initiated and leaf-
    initiated joins, it is acceptable for a c_root socket that is already in
    listening mode to be used as an input to WSPJoinLeaf().

    The WinSock SPI client is responsible for allocating any memory space
    pointed to directly or indirectly by any of the parameters it specifies.

    The lpCallerData is a value parameter which contains any user data that is
    to be sent along with the multipoint session join request. If lpCallerData
    is NULL, no user data will be passed to the peer. The lpCalleeData is a
    result parameter which will contain any user data passed back from the peer
    as part of the multipoint session establishment. lpCalleeData->len
    initially contains the length of the buffer allocated by the WinSock SPI
    client and pointed to by lpCalleeData->buf. lpCalleeData->len will be set
    to 0 if no user data has been passed back. The lpCalleeData information
    will be valid when the multipoint join operation is complete. For blocking
    sockets, this will be when the WSPJoinLeaf() function returns. For non-
    blocking sockets, this will be after the FD_CONNECT notification has
    occurred. If lpCalleeData is NULL, no user data will be passed back. The
    exact format of the user data is specific to the address family to which
    the socket belongs and/or the applications involved.

    At multipoint session establishment time, a WinSock SPI client may use the
    lpSQOS and/or lpGQOS parameters to override any previous QOS specification
    made for the socket via WSPIoctl() with either the SIO_SET_QOS or
    SIO_SET_GROUP_QOS opcodes.

    lpSQOS specifies the flow specs for socket s, one for each direction,
    followed by any additional provider-specific parameters. If either the
    associated transport provider in general or the specific type of socket in
    particular cannot honor the QOS request, an error will be returned as
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

    The dwFlags parameter is used to indicate whether the socket will be acting
    only as a sender (JL_SENDER_ONLY), only as a receiver (JL_RECEIVER_ONLY),
    or both (JL_BOTH).

    When connected sockets break (i.e. become closed for whatever reason), they
    should be discarded and recreated.  It is safest to assume that when things
    go awry for any reason on a connected socket, the WinSock SPI client must
    discard and recreate the needed sockets in order to return to a stable
    point.

Arguments:

    s - A descriptor identifying a multipoint socket.

    name - The name of the peer to which the socket is to be joined.

    namelen - The length of the name.

    lpCallerData - A pointer to the user data that is to be transferred to
        the peer during multipoint session establishment.

    lpCalleeData - A pointer to the user data that is to be transferred back
        from the peer during multipoint session establishment.

    lpSQOS - A pointer to the flow specs for socket s, one for each direction.

    lpGQOS - A pointer to the flow specs for the socket group (if applicable).

    dwFlags - Flags to indicate that the socket is acting as a sender,
        receiver, or both.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPJoinLeaf() returns a value of type SOCKET which is
        a descriptor for the newly created multipoint socket. Otherwise, a
        value of INVALID_SOCKET is returned, and a specific error code is
        available in lpErrno.

        On a blocking socket, the return value indicates success or failure of
        the join operation.

        With a non-blocking socket, successful initiation of a join operation
        is indicated by a return value of a valid socket descriptor.
        Subsequently, an FD_CONNECT indication is given when the join
        operation completes, either successfully or otherwise.

        Also, until the multipoint session join attempt completes all
        subsequent calls to WSPJoinLeaf() on the same socket will fail with
        the error code WSAEALREADY.

        If the return error code indicates the multipoint session join attempt
        failed (i.e. WSAECONNREFUSED, WSAENETUNREACH, WSAETIMEDOUT) the
        WinSock SPI client may call WSPJoinLeaf() again for the same socket.

--*/

{

    *lpErrno = WSAEINVAL;
    return INVALID_SOCKET;

}   // WSPJoinLeaf

