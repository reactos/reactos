/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module contains socket option routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPGetSockOpt()
        WSPSetSockOpt()

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
WSPGetSockOpt(
    IN SOCKET s,
    IN int level,
    IN int optname,
    OUT char FAR * optval,
    IN OUT LPINT optlen,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine retrieves the current value for a socket option associated
    with a socket of any type, in any state, and stores the result in optval.
    Options may exist at multiple protocol levels, but they are always present
    at the uppermost "socket" level. Options affect socket operations, such as
    the routing of packets, out-of-band data transfer, etc.

    The value associated with the selected option is returned in the buffer
    optval. The integer pointed to by optlen should originally contain the size
    of this buffer; on return, it will be set to the size of the value
    returned. For SO_LINGER, this will be the size of a struct linger; for most
    other options it will be the size of an integer.

    The WinSock SPI client is responsible for allocating any memory space
    pointed to directly or indirectly by any of the parameters it specifies.

    If the option was never set with WSPSetSockOpt(), then WSPGetSockOpt()
    returns the default value for the option.

        Value               Type            Meaning
        ~~~~~               ~~~~            ~~~~~~~
        SO_ACCEPTCONN       BOOL            Socket is WSPListen()ing.
        SO_BROADCAST        BOOL            Socket is configured for the
                                            transmission of broadcast messages
                                            on the socket.
        SO_DEBUG            BOOL            Debugging is enabled.
        SO_DONTROUTE        BOOL            Routing is disabled.
        SO_GROUP_ID         GROUP           The identifier of the group to
                                            which the socket belongs.
        SO_GROUP_PRIORITY   int             The relative priority for sockets
                                            that are part of a socket group.
        SO_KEEPALIVE        BOOL            Keepalives are being sent.
        SO_LINGER           struct linger   Returns the current linger options.
        SO_MAX_MSG_SIZE     unsigned int    Maximum size of a message for
                                            message-oriented socket types
                                            (e.g. SOCK_DGRAM). Has no meaning
                                            for stream-oriented sockets.
        SO_OOBINLINE        BOOL            Out-of-band data is being received
                                            in the normal data stream.
        SO_PROTOCOL_INFO  WSAPROTOCOL_INFOW Description of the protocol info
                                            for the protocol that is bound
                                            to this socket.
        SO_RCVBUF           int             Buffer size for receives.
        SO_REUSEADDR        BOOL            The socket may be bound to an
                                            address which is already in use.
        SO_SNDBUF           int             Buffer size for sends.
        SO_TYPE             int             The type of the socket (e.g.
                                            SOCK_STREAM).
        PVD_CONFIG          Service         An "opaque" data structure object
                            Provider        from the service provider
                            Dependent       associated with socket s. This
                                            object stores the current
                                            configuration information of the
                                            service provider. The exact format
                                            of this data structure is service
                                            provider specific.

    Calling WSPGetSockOpt() with an unsupported option will result in an error
    code of WSAENOPROTOOPT being returned in lpErrno.

    SO_DEBUG - WinSock service providers are encouraged (but not required) to
    supply output debug information if the SO_DEBUG option is set by a WinSock
    SPI client. The mechanism for generating the debug information and the form
    it takes are beyond the scope of this specification.

    SO_ERROR - The SO_ERROR option returns and resets the per-socket based
    error code (which is not necessarily the same as the per-thread error code
    that is maintained by the WinSock 2 DLL). A successful WinSock call on the
    socket does not reset the socket-based error code returned by the SO_ERROR
    option.

    SO_GROUP_ID - This is a get-only socket option which supplies the
    identifier of the group this socket belongs to. Note that socket group IDs
    are unique across all processes for a give service provider. If this socket
    is not a group socket, the value is NULL.

    SO_GROUP_PRIORITY - Group priority indicates the priority of the specified
    socket relative to other sockets within the socket group. Values are non-
    negative integers, with zero corresponding to the highest priority.
    Priority values represent a hint to the service provider about how
    potentially scarce resources should be allocated. For example, whenever
    two or more sockets are both ready to transmit data, the highest priority
    socket (lowest value for SO_GROUP_PRIORITY) should be serviced first, with
    the remainder serviced in turn according to their relative priorities.

    The WSAENOPROTOOPT error is indicated for non group sockets or for service
    providers which do not support group sockets.

    SO_KEEPALIVE - An WinSock SPI client may request that a TCP/IP provider
    enable the use of "keep-alive" packets on TCP connections by turning on the
    SO_KEEPALIVE socket option. A WinSock provider need not support the use of
    keep-alives: if it does, the precise semantics are implementation-specific
    but should conform to section 4.2.3.6 of RFC 1122: Requirements for
    Internet Hosts -- Communication Layers. If a connection is dropped as the
    result of "keep-alives" the error code WSAENETRESET is returned to any
    calls in progress on the socket, and any subsequent calls will fail with
    WSAENOTCONN.

    SO_LINGER - SO_LINGER controls the action taken when unsent data is queued
    on a socket and a WSPCloseSocket() is performed. See WSPCloseSocket() for a
    description of the way in which the SO_LINGER settings affect the semantics
    of WSPCloseSocket(). The WinSock SPI client sets the desired behavior by
    creating a struct linger (pointed to by the optval argument) with the
    following elements:

        struct linger {
            u_short l_onoff;
            u_short l_linger;
        };

    To enable SO_LINGER, a WinSock SPI client should set l_onoff to a non-zero
    value, set l_linger to 0 or the desired timeout (in seconds), and call
    WSPSetSockOpt(). To enable SO_DONTLINGER (i.e. disable SO_LINGER) l_onoff
    should be set to zero and WSPSetSockOpt() should be called. Note that
    enabling SO_LINGER with a non-zero timeout on a non-blocking socket is not
    recommended (see WSPCloseSocket() for details).

    Enabling SO_LINGER also disables SO_DONTLINGER, and vice versa. Note that
    if SO_DONTLINGER is DISABLED (i.e. SO_LINGER is ENABLED) then no timeout
    value is specified. In this case the timeout used is implementation
    dependent. If a previous timeout has been established for a socket (by
    enabling SO_LINGER), then this timeout value should be reinstated by the
    service provider.

    SO_MAX_MSG_SIZE - This is a get-only socket option which indicates the
    maximum size of a message for message-oriented socket types (e.g.
    SOCK_DGRAM) as implemented by the service provider. It has no meaning for
    byte stream oriented sockets.

    SO_PROTOCOL_INFO - This is a get-only option which supplies the
    WSAPROTOCOL_INFOW structure associated with this socket.

    SO_RCVBUF & SO_SNDBUF - When a Windows Sockets implementation supports the
    SO_RCVBUF and SO_SNDBUF options, a WinSock SPI client may request different
    buffer sizes (larger or smaller). The call may succeed even though the
    service provider did not make available the entire amount requested. A
    WinSock SPI client must call WSPGetSockOpt() with the same option to check
    the buffer size actually provided.

    SO_REUSEADDR - By default, a socket may not be bound (see WSPBind()) to a
    local address which is already in use. On occasions, however, it may be
    desirable to "re-use" an address in this way. Since every connection is
    uniquely identified by the combination of local and remote addresses, there
    is no problem with having two sockets bound to the same local address as
    long as the remote addresses are different. To inform the WinSock provider
    that a WSPBind() on a socket should be allowed to bind to a local address
    that is already in use by another socket, the WinSock SPI client should set
    the SO_REUSEADDR socket option for the socket before issuing the WSPBind().
    Note that the option is interpreted only at the time of the WSPBind(): it
    is therefore unnecessary (but harmless) to set the option on a socket which
    is not to be bound to an existing address, and setting or resetting the
    option after the WSPBind() has no effect on this or any other socket.

    PVD_CONFIG - This object stores the configuration information for the
    service provider associated with socket s. The exact format of this data
    structure is service provider specific.

Arguments:

    s - A descriptor identifying a socket.

    level - The level at which the option is defined; the supported levels
        include SOL_SOCKET.

    optname - The socket option for which the value is to be retrieved.

    optval - A pointer to the buffer in which the value for the requested
        option is to be returned.

    optlen - A pointer to the size of the optval buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPGetSockOpt() returns 0. Otherwise, a value of
        SOCKET_ERROR is returned, and a specific error code is available in
        lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPGetSockOpt", (PVOID)s, (PVOID)level, (PVOID)optname, optval );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPGetSockOpt", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SOCKOPT) {

            SOCK_PRINT((
                "WSPGetSockOpt failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPGetSockOpt", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Validate the parameters.
    //

    if( optval == NULL ||
        optlen == NULL ||
        *optlen < sizeof(INT) ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Handle new Winsock 2.x socket options first.
    //

    if( level == SOL_SOCKET ) {

        switch( optname ) {

        case SO_MAX_MSG_SIZE :

            *(LPINT)optval = socketInfo->Hooker->MaxUdpDatagramSize;
            *optlen = sizeof(INT);
            err = NO_ERROR;
            goto exit;

        case SO_PROTOCOL_INFOW :

            if( *optlen < sizeof(WSAPROTOCOL_INFOW) ) {

                err = WSAEFAULT;
                goto exit;

            }

            SockBuildProtocolInfoForSocket(
                socketInfo,
                (LPWSAPROTOCOL_INFOW)optval
                );

            *optlen = sizeof(WSAPROTOCOL_INFOW);
            err = NO_ERROR;
            goto exit;

        case SO_GROUP_ID :

            *(LPINT)optval = (INT)socketInfo->GroupID;
            *optlen = sizeof(INT);
            err = NO_ERROR;
            goto exit;

        case SO_GROUP_PRIORITY :

            *(LPINT)optval = socketInfo->GroupPriority;
            *optlen = sizeof(INT);
            err = NO_ERROR;
            goto exit;

        }

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->getsockopt(
                 socketInfo->WS1Handle,
                 level,
                 optname,
                 optval,
                 optlen
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

    SOCK_EXIT( "WSPGetSockOpt", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPGetSockOpt



INT
WSPAPI
WSPSetSockOpt(
    IN SOCKET s,
    IN int level,
    IN int optname,
    IN const char FAR * optval,
    IN int optlen,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine sets the current value for a socket option associated with a
    socket of any type, in any state. Although options may exist at multiple
    protocol levels, they are always present at the uppermost "socket' level.
    Options affect socket operations, such as whether broadcast messages may
    be sent on the socket, etc.

    There are two types of socket options: Boolean options that enable or
    disable a feature or behavior, and options which require an integer value
    or structure. To enable a Boolean option, optval points to a nonzero
    integer. To disable the option optval points to an integer equal to zero.
    optlen should be equal to sizeof(int) for Boolean options. For other
    options, optval points to the an integer or structure that contains the
    desired value for the option, and optlen is the length of the integer or
    structure.

        Value               Type            Meaning
        ~~~~~               ~~~~            ~~~~~~~
        SO_BROADCAST        BOOL            Allow transmission of broadcast
                                            messages on the socket.
        SO_DEBUG            BOOL            Record debugging information.
        SO_DONTLINGER       BOOL            Don't block close waiting for
                                            unsent data to be sent. Setting
                                            this option is equivalent to
                                            setting SO_LINGER with l_onoff set
                                            to zero.
        SO_DONTROUTE        BOOL            Don't route: send directly to
                                            interface.
        SO_GROUP_PRIORITY   int             Specify the relative priority to
                                            be established for sockets that
                                            are part of a socket group.
        SO_KEEPALIVE        BOOL            Send keepalives.
        SO_LINGER           struct linger   Linger on close if unsent data is
                                            present.
        SO_OOBINLINE        BOOL            Receive out-of-band data in the
                                            normal data stream.
        SO_RCVBUF           int             Specify buffer size for receives.
        SO_REUSEADDR        BOOL            Allow the socket to be bound to an
                                            address which is already in use.
                                            (See WSPBind().)
        SO_SNDBUF           int             Specify buffer size for sends.
        PVD_CONFIG          Service         This object stores the
                            Provider        configuration information for the
                            Dependent       service provider associated with
                                            socket s. The exact format of this
                                            data structure is service provider
                                            specific.

    Calling WSPSetSockOpt() with an unsupported option will result in an error
    code of WSAENOPROTOOPT being returned in lpErrno.

    SO_DEBUG - WinSock service providers are encouraged (but not required) to
    supply output debug information if the SO_DEBUG option is set by a WinSock
    SPI client. The mechanism for generating the debug information and the form
    it takes are beyond the scope of this specification.

    SO_GROUP_PRIORITY - Group priority indicates the priority of the specified
    socket relative to other sockets within the socket group. Values are non-
    negative integers, with zero corresponding to the highest priority.
    Priority values represent a hint to the service provider about how
    potentially scarce resources should be allocated. For example, whenever
    two or more sockets are both ready to transmit data, the highest priority
    socket (lowest value for SO_GROUP_PRIORITY) should be serviced first, with
    the remainder serviced in turn according to their relative priorities.

    The WSAENOPROTOOPT error is indicated for non group sockets or for service
    providers which do not support group sockets.

    SO_KEEPALIVE - An WinSock SPI client may request that a TCP/IP provider
    enable the use of "keep-alive" packets on TCP connections by turning on the
    SO_KEEPALIVE socket option. A WinSock provider need not support the use of
    keep-alives: if it does, the precise semantics are implementation-specific
    but should conform to section 4.2.3.6 of RFC 1122: Requirements for
    Internet Hosts -- Communication Layers. If a connection is dropped as the
    result of "keep-alives" the error code WSAENETRESET is returned to any
    calls in progress on the socket, and any subsequent calls will fail with
    WSAENOTCONN.

    SO_LINGER - SO_LINGER controls the action taken when unsent data is queued
    on a socket and a WSPCloseSocket() is performed. See WSPCloseSocket() for a
    description of the way in which the SO_LINGER settings affect the semantics
    of WSPCloseSocket(). The WinSock SPI client sets the desired behavior by
    creating a struct linger (pointed to by the optval argument) with the
    following elements:

        struct linger {
            u_short l_onoff;
            u_short l_linger;
        };

    To enable SO_LINGER, a WinSock SPI client should set l_onoff to a non-zero
    value, set l_linger to 0 or the desired timeout (in seconds), and call
    WSPSetSockOpt(). To enable SO_DONTLINGER (i.e. disable SO_LINGER) l_onoff
    should be set to zero and WSPSetSockOpt() should be called. Note that
    enabling SO_LINGER with a non-zero timeout on a non-blocking socket is not
    recommended (see WSPCloseSocket() for details).

    Enabling SO_LINGER also disables SO_DONTLINGER, and vice versa. Note that
    if SO_DONTLINGER is DISABLED (i.e. SO_LINGER is ENABLED) then no timeout
    value is specified. In this case the timeout used is implementation
    dependent. If a previous timeout has been established for a socket (by
    enabling SO_LINGER), then this timeout value should be reinstated by the
    service provider.

    SO_REUSEADDR - By default, a socket may not be bound (see WSPBind()) to a
    local address which is already in use. On occasions, however, it may be
    desirable to "re-use" an address in this way. Since every connection is
    uniquely identified by the combination of local and remote addresses, there
    is no problem with having two sockets bound to the same local address as
    long as the remote addresses are different. To inform the WinSock provider
    that a WSPBind() on a socket should be allowed to bind to a local address
    that is already in use by another socket, the WinSock SPI client should set
    the SO_REUSEADDR socket option for the socket before issuing the WSPBind().
    Note that the option is interpreted only at the time of the WSPBind(): it
    is therefore unnecessary (but harmless) to set the option on a socket which
    is not to be bound to an existing address, and setting or resetting the
    option after the WSPBind() has no effect on this or any other socket.

    SO_RCVBUF & SO_SNDBUF - When a Windows Sockets implementation supports the
    SO_RCVBUF and SO_SNDBUF options, a WinSock SPI client may request different
    buffer sizes (larger or smaller). The call may succeed even though the
    service provider did not make available the entire amount requested. A
    WinSock SPI client must call WSPGetSockOpt() with the same option to check
    the buffer size actually provided.

    PVD_CONFIG - This object stores the configuration information for the
    service provider associated with socket s. The exact format of this data
    structure is service provider specific.

Arguments:

    s - A descriptor identifying a socket.

    level - The level at which the option is defined; the supported levels
        include SOL_SOCKET.

    optname - The socket option for which the value is to be set.

    optval - A pointer to the buffer in which the value for the requested
        option is supplied.

    optlen - The size of the optval buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPSetSockOpt() returns 0. Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code is available in
    lpErrno.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPSetSockOpt", (PVOID)s, (PVOID)level, (PVOID)optname, (PVOID)optval );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPSetSockOpt", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(SOCKOPT) {

            SOCK_PRINT((
                "WSPSetSockOpt failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPSetSockOpt", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Validate the parameters.
    //

    if( optval == NULL ||
        optlen < sizeof(INT) ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Handle new Winsock 2.x socket options first.
    //

    if( level == SOL_SOCKET ) {

        switch( optname ) {

        case SO_GROUP_PRIORITY :

            socketInfo->GroupPriority = *(LPINT)optval;
            err = NO_ERROR;
            goto exit;

        }

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->setsockopt(
                 socketInfo->WS1Handle,
                 level,
                 optname,
                 optval,
                 optlen
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

    SOCK_EXIT( "WSPSetSockOpt", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPSetSockOpt

