/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    startup.c

Abstract:

    This module contains the startup code for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPSocket()
        WSPCloseSocket()

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
WSPSocket(
    IN int af,
    IN int type,
    IN int protocol,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN GROUP g,
    IN DWORD dwFlags,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    WSPSocket() causes a socket descriptor and any related resources to be
    allocated. By default, the created socket will not have the overlapped
    attribute. WinSock providers are encouraged to be realized as Windows
    installable file systems, and supply system file handles as socket
    descriptors. These providers must call WPUModifyIFSHandle() prior to
    returning from this routine  For non-file-system WinSock providers,
    WPUCreateSocketHandle() must be used to acquire a unique socket descriptor
    from the WinSock 2 DLL prior to returning from this routine.

    The values for af, type and protocol are those supplied by the application
    in the corresponding API functions socket() or WSASocket(). A service
    provider is free to ignore or pay attention to any or all of these values
    as is appropriate for the particular protocol. However, the provider must
    be willing to accept the value of zero for af  and type, since the
    WinSock 2 DLL considers these to be wild card values. Also the value of
    manifest constant FROM_PROTOCOL_INFOW must be accepted for any of af, type
    and protocol. This value indicates that the WinSock 2 application wishes to
    use the corresponding values from the indicated WSAPROTOCOL_INFOW struct:

        iAddressFamily
        iSocketType
        iProtocol

    Parameter g is used to indicate the appropriate actions on socket groups:

        If g is an existing socket group ID, join the new socket to this
            group, provided all the requirements set by this group are met.

        If g = SG_UNCONSTRAINED_GROUP, create an unconstrained socket
            group and have the new socket be  the first member.

        If g = SG_CONSTRAINED_GROUP, create a constrained socket group and
            have the new socket be the first member.

        If g = zero, no group operation is performed.

    Any set of sockets grouped together must be implemented by a single
    service provider. For unconstrained groups, any set of sockets may be
    grouped together. A constrained socket group may consist only of
    connection-oriented sockets, and requires that connections on all grouped
    sockets be to the same address on the same host. For newly created socket
    groups, the new group ID must be available for the WinSock SPI client to
    retrieve by calling WSPGetSockOpt() with option SO_GROUP_ID. A socket group
    and its associated ID remain valid until the last socket belonging to this
    socket group is closed. Socket group IDs are unique across all processes
    for a given service provider.

    The dwFlags parameter may be used to specify the attributes of the socket
    by OR-ing any of the following Flags:

        WSA_FLAG_OVERLAPPED - This flag causes an overlapped socket to
            be created. Overlapped sockets may utilize WSPSend(),
            WSPSendTo(), WSPRecv(), WSPRecvFrom() and WSPIoctl() for
            overlapped I/O operations, which allows multiple operations
            to be initiated and in progress simultaneously.

        WSA_FLAG_MULTIPOINT_C_ROOT - Indicates that the socket created
            will be a c_root in a multipoint session. Only allowed if a
            rooted control plane is indicated in the protocol's
            WSAPROTOCOL_INFOW struct.

        WSA_FLAG_MULTIPOINT_C_LEAF - Indicates that the socket created
            will be a c_leaf in a multicast session. Only allowed if
            XP1_SUPPORT_MULTIPOINT is indicated in the protocol's
            WSAPROTOCOL_INFOW struct.

        WSA_FLAG_MULTIPOINT_D_ROOT - Indicates that the socket created
            will be a d_root in a multipoint session. Only allowed if a
            rooted data plane is indicated in the protocol's
            WSAPROTOCOL_INFOW struct.

        WSA_FLAG_MULTIPOINT_D_LEAF - Indicates that the socket created
            will be a d_leaf in a multipoint session. Only allowed if
            XP1_SUPPORT_MULTIPOINT is indicated in the protocol's
            WSAPROTOCOL_INFOW struct.

    N.B For multipoint sockets, exactly one of WSA_FLAG_MULTIPOINT_C_ROOT
    or WSA_FLAG_MULTIPOINT_C_LEAF must be specified, and exactly one of
    WSA_FLAG_MULTIPOINT_D_ROOT or WSA_FLAG_MULTIPOINT_D_LEAF must be
    specified.

    Connection-oriented sockets such as SOCK_STREAM provide full-duplex
    connections, and must be in a connected state before any data may be sent
    or received on them. A connection to another socket is created with a
    WSPConnect() call. Once connected, data may be transferred using WSPSend()
    and WSPRecv() calls. When a session has been completed, a WSPCloseSocket()
    must be performed.

    The communications protocols used to implement a reliable, connection-
    oriented socket ensure that data is not lost or duplicated. If data for
    which the peer protocol has buffer space cannot be successfully
    transmitted within a reasonable length of time, the connection is
    considered broken and subsequent calls will fail with the error code set
    to WSAETIMEDOUT.

    Connectionless, message-oriented sockets allow sending and receiving of
    datagrams to and from arbitrary peers using WSPSendTo() and WSPRecvFrom().
    If such a socket is WSPConnect()ed to a specific peer, datagrams may be
    sent to that peer using WSPSend() and may be received from (only) this
    peer using WSPRecv().

    Support for sockets with type SOCK_RAW is not required but service
    providers are encouraged to support raw sockets whenever it makes sense
    to do so.

    When a special WSAPROTOCOL_INFOW struct (obtained via the
    WSPDuplicateSocket() function and used to create additional descriptors
    for a shared socket) is passed as an input parameter to WSPSocket(),
    the g and dwFlags parameters are ignored.

Arguments:

    af- An address family specification.

    type - A type specification for the new socket.

    protocol - A particular protocol to be used with the socket which is
        specific to the indicated address family.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFOW struct that defines
        the characteristics of the socket to be created.

    g - The identifier of the socket group which the new socket is to join.

    dwFlags - The socket attribute specification.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPSocket() returns a descriptor referencing the
        new socket. Otherwise, a value of INVALID_SOCKET is returned, and a
        specific error code is available in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    PHOOKER_INFORMATION hookerInfo;
    SOCKET ws1Handle;
    SOCKET ws2Handle;
    INT err;
    SOCK_GROUP_TYPE groupType;

    SOCK_ENTER( "WSPSocket", (PVOID)af, (PVOID)type, (PVOID)protocol, lpProtocolInfo );

    SOCK_ASSERT( lpProtocolInfo != NULL );
    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPSocket", INVALID_SOCKET, lpErrno );
        return INVALID_SOCKET;

    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    socketInfo = NULL;
    hookerInfo = NULL;
    ws1Handle = INVALID_SOCKET;
    ws2Handle = INVALID_SOCKET;

    //
    // Snag the socket attributes from the protocol structure.
    //

    if( af == 0 || af == FROM_PROTOCOL_INFO ) {

        af = lpProtocolInfo->iAddressFamily;

    }

    if( type == 0 || type == FROM_PROTOCOL_INFO ) {

        type = lpProtocolInfo->iSocketType;

    }

    if( protocol == 0 || protocol == FROM_PROTOCOL_INFO ) {

        protocol = lpProtocolInfo->iProtocol;

    }

    //
    // Grab the global lock. We must hold this until the socket is
    // created an put on the global list.
    //

    //
    // Find a hooker for this socket. If this socket is successfully
    // created, the the hooker won't be dereferenced until the socket
    // is closed.
    //

    hookerInfo = SockFindAndReferenceHooker( &lpProtocolInfo->ProviderId );

    if( hookerInfo == NULL ) {

        //
        // WSAEINPROGRESS is a distinguished error code that will cause
        // WS2_32.DLL to continue its protocol catalog enumeration
        // and try another provider that supports this socket triple.
        //

        err = WSAEINPROGRESS;
        goto exit;

    }

    //
    // Validate the group.
    //

    if( !SockGetGroup(
            &g,
            &groupType
            ) ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // Create the WS1 socket.
    //

    SockPreApiCallout();

    ws1Handle = hookerInfo->socket( af, type, protocol );

    if( ws1Handle == INVALID_SOCKET ) {

        err = hookerInfo->WSAGetLastError();
        SOCK_ASSERT( err != NO_ERROR );
        SockPostApiCallout();
        goto exit;

    }

    SockPostApiCallout();

    //
    // Create our internal structure and the "visible" WS2 socket.
    //

    socketInfo = SockCreateSocket(
                     hookerInfo,
                     af,
                     type,
                     protocol,
                     lpProtocolInfo->iMaxSockAddr,
                     lpProtocolInfo->iMinSockAddr,
                     dwFlags,
                     lpProtocolInfo->dwCatalogEntryId,
                     lpProtocolInfo->dwServiceFlags1,
                     lpProtocolInfo->dwServiceFlags2,
                     lpProtocolInfo->dwServiceFlags3,
                     lpProtocolInfo->dwServiceFlags4,
                     lpProtocolInfo->dwProviderFlags,
                     g,
                     groupType,
                     ws1Handle,
                     &err
                     );

    if( socketInfo == NULL ) {

        SOCK_ASSERT( err != NO_ERROR );
        goto exit;

    }

    SOCK_ASSERT( err == NO_ERROR );
    SOCK_ASSERT( ws1Handle == socketInfo->WS1Handle );

    ws2Handle = socketInfo->WS2Handle;
    SOCK_ASSERT( ws2Handle != INVALID_SOCKET );

exit:

    if( err == NO_ERROR ) {

        IF_DEBUG(SOCKET) {

            SOCK_PRINT((
                "Opened socket %lx:%lx (%08lx) from hooker %08lx\n",
                socketInfo->WS2Handle,
                ws1Handle,
                socketInfo,
                hookerInfo
                ));

        }

        SockDereferenceSocket( socketInfo );

    } else {

        if( ws1Handle != INVALID_SOCKET ) {

            SockPreApiCallout();
            hookerInfo->closesocket( ws1Handle );
            SockPostApiCallout();

        }

        if( hookerInfo != NULL ) {

            SockDereferenceHooker( hookerInfo );

        }

        if( g != 0 ) {

            SockDereferenceGroup( g );

        }

        *lpErrno = err;
        ws2Handle = INVALID_SOCKET;

    }

    SOCK_EXIT( "WSPSocket", ws2Handle, (ws2Handle == INVALID_SOCKET) ? lpErrno : NULL );
    return ws2Handle;

}   // WSPSocket



INT
WSPAPI
WSPCloseSocket(
    IN  SOCKET s,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine closes a socket. More precisely, it releases the socket
    descriptor s, so that further references to s should fail with the error
    WSAENOTSOCK. If this is the last reference to an underlying socket, the
    associated naming information and queued data are discarded. Any blocking,
    asynchronous or overlapped calls pending on the socket (issued by any
    thread in this process) are canceled without posting any notification
    messages, signaling any event objects or invoking any completion routines.
    In this case, the pending overlapped operations fail with the error status
    WSA_OPERATION_ABORTED. FD_CLOSE will not be posted after WSPCloseSocket()
    is called.

    WSPClosesocket() behavior is summarized as follows:

        If SO_DONTLINGER enabled (the default setting) WSPCloseSocket()
            returns immediately - connection is gracefully closed "in
            the background".

        If SO_LINGER enabled with a zero timeout, WSPCloseSocket()
            returns immediately - connection is reset/aborted.

        If SO_LINGER enabled with non-zero timeout:

            - With a blocking socket, WSPCloseSocket() blocks
              until all data sent or timeout expires.

            - With a non-blocking socket, WSPCloseSocket()
              returns immediately indicating failure.

    The semantics of WSPCloseSocket() are affected by the socket options
    SO_LINGER and SO_DONTLINGER as follows:

        Option          Interval    Type of close   Wait for close?
        ~~~~~~          ~~~~~~~~    ~~~~~~~~~~~~~   ~~~~~~~~~~~~~~~
        SO_DONTLINGER   Don't care  Graceful        No
        SO_LINGER       Zero        Hard            No
        SO_LINGER       Non-zero    Graceful        Yes

    If SO_LINGER is set (i.e. the l_onoff field of the linger structure is
    non-zero) and the timeout interval, l_linger, is zero, WSPClosesocket()
    is not blocked even if queued data has not yet been sent or acknowledged.
    This is called a "hard" or "abortive" close, because the socket's virtual
    circuit is reset immediately, and any unsent data is lost. Any WSPRecv()
    call on the remote side of the circuit will fail with WSAECONNRESET.

    If SO_LINGER is set with a non-zero timeout interval on a blocking socket,
    the WSPClosesocket() call blocks on a blocking socket until the remaining
    data has been sent or until the timeout expires. This is called a graceful
    disconnect. If the timeout expires before all data has been sent, the
    service provider should abort the connection before WSPClosesocket()
    returns.

    Enabling SO_LINGER with a non-zero timeout interval on a non-blocking
    socket is not recommended. In this case, the call to WSPClosesocket() will
    fail with an error of WSAEWOULDBLOCK if the close operation cannot be
    completed immediately. If WSPClosesocket() fails with WSAEWOULDBLOCK the
    socket handle is still valid, and a disconnect is not initiated. The
    WinSock SPI client must call WSPClosesocket() again to close the socket,
    although WSPClosesocket() may continue to fail unless the WinSock SPI
    client disables  SO_DONTLINGER, enables SO_LINGER with a zero timeout, or
    calls WSPShutdown() to initiate closure.

    If SO_DONTLINGER is set on a stream socket (i.e. the l_onoff field of the
    linger structure is zero), the WSPClosesocket() call will return
    immediately. However, any data queued for transmission will be sent if
    possible before the underlying socket is closed. This is called a graceful
    disconnect and is the default behavior. Note that in this case the WinSock
    provider is allowed to retain any resources associated with the socket
    until such time as the graceful disconnect has completed or the provider
    aborts the connection due to an inability to complete the operation in a
    provider-determined amount of time. This may affect Winsock clients which
    expect to use all available sockets.

Arguments:

    s - A descriptor identifying a socket.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPCloseSocket() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT dummy;
    INT result;
    SOCK_STATE previousState;
    SOCKET ws1Handle;
    SOCKET ws2Handle;

    SOCK_ENTER( "WSPCloseSocket", (PVOID)s, NULL, NULL, NULL );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( FALSE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPCloseSocket", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SOCKET) {

            SOCK_PRINT((
                "WSPCloseSocket failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPCloseSocket", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Set the state of the socket to closing so that future closes will
    // fail. Capture the current state first, in case the close fails.
    //

    previousState = socketInfo->State;
    socketInfo->State = SocketStateClosing;

    //
    // Snag the handles from the structure.
    //

    ws1Handle = socketInfo->WS1Handle;
    SOCK_ASSERT( ws1Handle != INVALID_SOCKET );

    ws2Handle = socketInfo->WS2Handle;
    SOCK_ASSERT( ws2Handle != INVALID_SOCKET );

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->closesocket( ws1Handle );

    if( result == SOCKET_ERROR ) {
        err = socketInfo->Hooker->WSAGetLastError();
        SOCK_ASSERT( err != NO_ERROR );
        SockPostApiCallout();
        socketInfo->State = previousState;
        goto exit;
    }

    SockPostApiCallout();

    //
    // The WS1 handle is closed, now close the WS2 handle. If this fails,
    // there's not much we can do as we've already closed the hooker's
    // handle.
    //

    result = SockUpcallTable.lpWPUCloseSocketHandle(
                 ws2Handle,
                 &dummy
                 );

    if( result == SOCKET_ERROR ) {

        SOCK_PRINT((
            "WSPCloseSocket: WPUCloseSocketHandle( %lx ) failed, error %d\n",
            ws2Handle,
            dummy
            ));

    }

    //
    // Signal the socket's shutdown event so any worker threads can
    // cleanup and exit.
    //

    SetEvent( socketInfo->ShutdownEvent );

    //
    // Manually dereference the socket.  The dereference accounts for
    // the "active" reference of the socket and will cause the socket to
    // be deleted when the actual reference count goes to zero.
    //

    SOCK_ASSERT( socketInfo->ReferenceCount >= 2 );

    socketInfo->ReferenceCount--;

exit:

    SockDereferenceSocket( socketInfo );

    if( err == NO_ERROR ) {

        IF_DEBUG(SOCKET) {

            SOCK_PRINT((
                "WSPCloseSocket: closed %lx:%lx (%08lx)\n",
                ws2Handle,
                ws1Handle,
                socketInfo
                ));

        }

        SOCK_EXIT( "WSPCloseSocket", NO_ERROR, NULL );
        return NO_ERROR;

    }

    IF_DEBUG(SOCKET) {

        SOCK_PRINT((
            "WSPCloseSocket: %lx:%lx (%08lx) failed, error %d\n",
            ws2Handle,
            ws1Handle,
            socketInfo,
            err
            ));

    }

    SOCK_EXIT( "WSPCloseSocket", SOCKET_ERROR, lpErrno );
    *lpErrno = err;
    return SOCKET_ERROR;

}   // WSPCloseSocket

