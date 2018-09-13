/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    sockopt.c

Abstract:

    This module contains support for the getsockopt( ) and setsockopt( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    31-Mar-1992

Revision History:

--*/

#include "winsockp.h"

#ifdef _AFD_SAN_SWITCH_
#include "sanflow.h"
#endif //_AFD_SAN_SWITCH_



typedef struct _SOCK_EXTENSIONS {

    GUID Guid;
    LPVOID Function;

} SOCK_EXTENSIONS, *PSOCK_EXTENSIONS;

SOCK_EXTENSIONS SockExtensions[] =
    {
        {
            WSAID_TRANSMITFILE,
            (LPVOID)&TransmitFile
        },

        {
            WSAID_ACCEPTEX,
            (LPVOID)&AcceptEx
        },

        {
            WSAID_GETACCEPTEXSOCKADDRS,
            (LPVOID)&GetAcceptExSockaddrs
        }
    };

#define NUM_EXTENSIONS ( sizeof(SockExtensions) / sizeof(SockExtensions[0]) )

int
GetReceiveInformation (
    IN PSOCKET_INFORMATION Socket,
    OUT PAFD_RECEIVE_INFORMATION ReceiveInformation
    );

BOOLEAN
IsValidOptionForSocket (
    IN PSOCKET_INFORMATION Socket,
    IN int Level,
    IN int OptionName
    );

#ifdef _PNP_POWER_
int
WSPAPI
DoAsyncIoctl (
    PSOCKET_INFORMATION socket,
    HANDLE              transportHandle,
    DWORD               pollEvent,
    DWORD               dwIoControlCode,
    LPVOID              lpvInBuffer,
    DWORD               cbInBuffer,
    LPVOID              lpvOutBuffer,
    DWORD               cbOutBuffer,
    LPDWORD             lpcbBytesReturned,
    LPWSAOVERLAPPED     lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPINT               lpErrno
    );
#endif // _PNP_POWER_


int
WSPAPI
WSPGetSockOpt (
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    char *OptionValue,
    int *OptionLength,
    LPINT lpErrno
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
        SO_PROTOCOL_INFOW WSAPROTOCOL_INFOW Description of the protocol info
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

    SO_PROTOCOL_INFOW - This is a get-only option which supplies the
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
	PWINSOCK_TLS_DATA	tlsData;
    int err;
    PSOCKET_INFORMATION socket;

    WS_ENTER( "WSPGetSockOpt", (PVOID)Handle, (PVOID)Level, (PVOID)OptionName, OptionValue );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPGetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    socket = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        WS_EXIT( "WSPGetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;

    }

    //
    // Get exclusive access to the socket in question.  This is
    // necessary because we'll be changing the socket information
    // data structure.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Ensure the socket isn't closing on us.
    //

    if( socket->State == SocketStateClosing ) {

        err = WSAENOTSOCK;
        goto exit;

    }

    __try {
        //
        // OptionValue must point to a buffer of at least sizeof(char) bytes.
        //

        if ( !ARGUMENT_PRESENT( OptionValue ) ||
                 !ARGUMENT_PRESENT( OptionLength ) ||
                 *OptionLength < sizeof(char) ||
                 (*OptionLength & 0x80000000) != 0 ) {

            err = WSAEFAULT;
            goto exit;

        }

        //
        // Make sure that this is a legitimate option for the socket.
        //

        if ( !IsValidOptionForSocket( socket, Level, OptionName ) ) {

            err = WSAENOPROTOOPT;
            goto exit;

        }

        //
        // If the specified option is supprted here, clear out the input
        // buffer so that we know we start with a clean slate.
        //

        if ( Level == SOL_SOCKET &&
               ( OptionName == SO_BROADCAST ||
                 OptionName == SO_DEBUG ||
                 OptionName == SO_DONTLINGER ||
                 OptionName == SO_LINGER ||
                 OptionName == SO_OOBINLINE ||
                 OptionName == SO_RCVBUF ||
                 OptionName == SO_REUSEADDR ||
                 OptionName == SO_EXCLUSIVEADDRUSE ||
                 OptionName == SO_CONDITIONAL_ACCEPT ||
                 OptionName == SO_SNDBUF ||
                 OptionName == SO_TYPE ||
                 OptionName == SO_ACCEPTCONN ||
                 OptionName == SO_ERROR ) ) {

            RtlZeroMemory( OptionValue, *OptionLength );
        }

        //
        // Act based on the level and option being set.
        //

        switch ( Level ) {

        case SOL_SOCKET:

            switch ( OptionName ) {

            case SO_BROADCAST:

                *OptionValue = socket->Broadcast;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_DEBUG:

                *OptionValue = socket->Debug;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_ERROR:

                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }
                //
                // If socket connect has just completed with failure,
                // the error code is not available until
                // post connect processing completes in the
                // async thread.  The function below will wait
                // for this completion.  It will only do the wait
                // if connect has actually completed and just
                // the post-processing part remains to be done.
                //
                SockIsSocketConnected (socket);

                *((INT UNALIGNED *)OptionValue) = socket->LastError;
                *OptionLength = sizeof (int);
                break;


            case SO_DONTLINGER:

                *OptionValue = socket->LingerInfo.l_onoff == 0 ? TRUE : FALSE;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_LINGER:

                if ( *OptionLength < sizeof(struct linger) ) {

                    err = WSAEFAULT;
                    goto exit;
                }


                *((struct linger *)OptionValue) = socket->LingerInfo;
                *OptionLength = sizeof(struct linger);

                break;

            case SO_OOBINLINE:

                *OptionValue = socket->OobInline;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_RCVBUF:

                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->ReceiveBufferSize;
                *OptionLength = sizeof (int);
                break;

            case SO_REUSEADDR:

                *OptionValue = socket->ReuseAddresses;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_EXCLUSIVEADDRUSE:

                *OptionValue = socket->ExclusiveAddressUse;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_CONDITIONAL_ACCEPT:
                *OptionValue = socket->ConditionalAccept;
                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_SNDBUF:

                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->SendBufferSize;
                *OptionLength = sizeof (int);
                break;

            case SO_TYPE:

                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->SocketType;
                *OptionLength = sizeof (int);
                break;

            case SO_ACCEPTCONN:

                if ( socket->Listening ) {

                    *OptionValue = TRUE;

                } else {

                    *OptionValue = FALSE;

                }

                *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
                break;

            case SO_CONNDATA:

                err = SockGetConnectData(
                            socket,
                            IOCTL_AFD_GET_CONNECT_DATA,
                            (PCHAR)OptionValue,
                            *OptionLength,
                            OptionLength
                            );
                goto exit;

            case SO_CONNOPT:

                err = SockGetConnectData(
                            socket,
                            IOCTL_AFD_GET_CONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            *OptionLength,
                            OptionLength
                            );
                goto exit;

            case SO_DISCDATA:

                err = SockGetConnectData(
                            socket,
                            IOCTL_AFD_GET_DISCONNECT_DATA,
                            (PCHAR)OptionValue,
                            *OptionLength,
                            OptionLength
                            );
                goto exit;

            case SO_DISCOPT:

                err = SockGetConnectData(
                            socket,
                            IOCTL_AFD_GET_DISCONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            *OptionLength,
                            OptionLength
                            );
                goto exit;

            case SO_SNDTIMEO:
                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->SendTimeout;
                *OptionLength = sizeof (int);
                goto exit;

            case SO_RCVTIMEO:

                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->ReceiveTimeout;
                *OptionLength = sizeof (int);
                goto exit;

            case SO_MAXDG:
            case SO_MAX_MSG_SIZE :
                if (*OptionLength<sizeof (ULONG)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                err = SockGetInformation(
                            socket,
                            AFD_MAX_SEND_SIZE,
                            NULL,
                            0,
                            NULL,
                            (PULONG)OptionValue,
                            NULL
                            );

                if ( err == NO_ERROR ) {

                    *OptionLength = sizeof(ULONG);

                }

                goto exit;

            case SO_CONNECT_TIME:

                if (*OptionLength<sizeof (ULONG)) {
                    err = WSAEFAULT;
                    goto exit;
                }

#ifdef _AFD_SAN_SWITCH_
				//
				// If we have a SAN socket open, get info from there
				//
				if (SockSanEnabled && socket->SanSocket && 
					socket->SanSocket->IsConnected == CONNECTED) {
					
					*(PULONG)OptionValue = GetTickCount() - socket->SanSocket->ConnectTime;
					*OptionLength = sizeof(ULONG);
					err = NO_ERROR;
					goto exit;
				}
#endif //_AFD_SAN_SWITCH_

                err = SockGetInformation(
                            socket,
                            AFD_CONNECT_TIME,
                            NULL,
                            0,
                            NULL,
                            (PULONG)OptionValue,
                            NULL
                            );

                if ( err == NO_ERROR ) {

                    *OptionLength = sizeof(ULONG);

                }

                goto exit;

            case SO_MAXPATHDG: {

                PTRANSPORT_ADDRESS tdiAddress;
                ULONG tdiAddressLength;

                //
                // Allocate enough space to hold the TDI address structure
                // we'll pass to AFD.
                //

                tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;

                tdiAddress = ALLOCATE_HEAP( tdiAddressLength );

                if ( tdiAddress == NULL ) {

                    err = WSAENOBUFS;
                    goto exit;

                }

                //
                // Convert the address from the sockaddr structure to the
                // appropriate TDI structure.
                //

                SockBuildTdiAddress(
                    tdiAddress,
                    (PSOCKADDR)OptionValue,
                    *OptionLength
                    );


                err = SockGetInformation(
                            socket,
                            AFD_MAX_PATH_SEND_SIZE,
                            tdiAddress,
                            tdiAddress->Address[0].AddressLength,
                            NULL,
                            (PULONG)OptionValue,
                            NULL
                            );

                FREE_HEAP( tdiAddress );

                if ( err == NO_ERROR ) {

                    *OptionLength = sizeof(ULONG);

                }

                goto exit;
                }

            case SO_PROTOCOL_INFOW: {

                LPWSAPROTOCOL_INFOW protocolInfo = (LPWSAPROTOCOL_INFOW)OptionValue;

                if( *OptionLength < sizeof(WSAPROTOCOL_INFOW) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                SockReleaseSocketLock (socket);

                err = SockBuildProtocolInfoForSocket(
                    socket,
                    protocolInfo
                    );

#ifdef _AFD_SAN_SWITCH_
				//
				// If we have a SAN socket open, then it is NOT an IFS handle
				//
				if (SockSanEnabled && socket->SanSocket && 
					socket->SanSocket->IsConnected == CONNECTED) {
					
					protocolInfo->dwServiceFlags1 &= ~XP1_IFS_HANDLES;

				}
#endif //_AFD_SAN_SWITCH_

                *OptionLength = sizeof(WSAPROTOCOL_INFOW);
                SockDereferenceSocket (socket);

                }
                goto exit_no_deref;

            case SO_GROUP_ID:
                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->GroupID;
                *OptionLength = sizeof (int);
                goto exit;

            case SO_GROUP_PRIORITY:
                if (*OptionLength<sizeof (int)) {
                    err = WSAEFAULT;
                    goto exit;
                }

                *((INT UNALIGNED *)OptionValue) = socket->GroupPriority;
                *OptionLength = sizeof (int);
                goto exit;

            case SO_DONTROUTE:
            case SO_KEEPALIVE:
            case SO_RCVLOWAT:
            case SO_SNDLOWAT:
            default:

                //
                // We don't support this option here in the winsock DLL.  Give
                // it to the helper DLL.
                //

                err = SockGetTdiHandles( socket );

                if ( err != NO_ERROR ) {

                    goto exit;

                }

                err = socket->HelperDll->WSHGetSocketInformation(
                            socket->HelperDllContext,
                            Handle,
                            socket->TdiAddressHandle,
                            socket->TdiConnectionHandle,
                            Level,
                            OptionName,
                            OptionValue,
                            OptionLength
                            );

                if ( err != NO_ERROR ) {

                    goto exit;

                }

                break;
            }

            break;

        default:

            //
            // The specified level isn't supported here in the winsock DLL.
            // Give the request to the helper DLL.
            //

            err = SockGetTdiHandles( socket );

            if ( err != NO_ERROR ) {

                goto exit;

            }

            err = socket->HelperDll->WSHGetSocketInformation(
                        socket->HelperDllContext,
                        Handle,
                        socket->TdiAddressHandle,
                        socket->TdiConnectionHandle,
                        Level,
                        OptionName,
                        OptionValue,
                        OptionLength
                        );

            if ( err != NO_ERROR ) {

                goto exit;

            }

            break;

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {

        err = WSAEFAULT;
        goto exit;
    }

exit:

    //
    // Release the resource, dereference the socket, set the error if
    // any, and return.
    //

    SockReleaseSocketLock( socket );
    SockDereferenceSocket( socket );

exit_no_deref:

    IF_DEBUG(SOCKOPT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPGetSockOpt on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPGetSockOpt on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPGetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPGetSockOpt", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPGetSockOpt


int
WSPAPI
WSPIoctl (
    SOCKET Handle,
    DWORD dwIoControlCode,
    LPVOID lpvInBuffer,
    DWORD cbInBuffer,
    LPVOID lpvOutBuffer,
    DWORD cbOutBuffer,
    LPDWORD lpcbBytesReturned,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used to set or retrieve operating parameters associated
    with the socket, the transport protocol, or the communications subsystem.
    For non-overlapped sockets, lpOverlapped and lpCompletionRoutine
    parameters are ignored, and this routine may block if socket s is in the
    blocking mode. Note that if socket s is in the non-blocking mode, this
    routine may return WSAEWOULDBLOCK if the specified operation cannot be
    finished immediately. In this case, the WinSock SPI client should change
    the socket to the blocking mode and reissue the request. For overlapped
    sockets, operations that cannot be completed immediately will be initiated,
    and completion will be indicated at a later time. The final completion
    status is retrieved via the WSPGetOverlappedResult().

    In as much as the dwIoControlCode parameter is now a 32 bit entity, it is
    possible to adopt an encoding scheme that provides a convenient way to
    partition the opcode identifier space. The dwIoControlCode parameter is
    architected to allow for protocol and vendor independence when adding new
    control codes, while retaining backward compatibility with Windows Sockets
    1.1 and Unix control codes. The dwIoControlCode parameter has the following
    form:

        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1| | | | | | | | | | |
        |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |I|O|V| T |Vendor/Address Family|             Code              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        I - Set if the input buffer is valid for the code, as with IOC_IN.

        O - Set if the output buffer is valid for the code, as with IOC_OUT.
        Note that for codes with both input and output parameters, both I
        and O will be set.

        V - Set if there are no parameters for the code, as with IOC_VOID.

        T - A two-bit quantity which defines the type of ioctl. The following
        values are defined:

            0 - The ioctl is a standard Unix ioctl code, as with FIONREAD,
            FIONBIO, etc.

            1 - The ioctl is a generic Windows Sockets 2 ioctl code. New
            ioctl codes defined for Windows Sockets 2 will have T == 1.

            2 - The ioctl applies only to a specific address family.

            3 - The ioctl applies only to a specific vendor's provider. This
            type allows companies to be assigned a vendor number which
            appears in the Vendor/Address Family field, and then the vendor
            can define new ioctls specific to that vendor without having to
            register the ioctl with a clearinghouse, thereby providing vendor
            flexibility and privacy.

        Vendor/Address Family - An 11-bit quantity which defines the vendor
        who owns the code (if T == 3) or which contains the address family
        to which the code applies (if T == 2). If this is a Unix ioctl code
        (T == 0) then this field has the same value as the code on Unix. If
        this is a generic Windows Sockets 2 ioctl (T == 1) then this field
        can be used as an extension of the "code" field to provide additional
        code values.

        Code - The specific ioctl code for the operation.

    The following Unix commands are supported:

    FIONBIO - Enable or disable non-blocking mode on socket s. lpvInBuffer
    points at an unsigned long, which is non-zero if non-blocking mode is to be
    enabled and zero if it is to be disabled. When a socket is created, it
    operates in blocking mode (i.e. non-blocking mode is disabled). This is
    consistent with BSD sockets.

    The WSPAsyncSelect() or WSPEventSelect() routine automatically sets a
    socket to nonblocking mode. If WSPAsyncSelect() or WSPEventSelect() has
    been issued on a socket, then any attempt to use WSPIoctl() to set the
    socket back to blocking mode will fail with WSAEINVAL. To set the socket
    back to blocking mode, a WinSock SPI client must first disable
    WSPAsyncSelect() by calling WSPAsyncSelect() with the lEvent parameter
    equal to 0, or disable WSPEventSelect() by calling WSPEventSelect() with
    the lNetworkEvents parameter equal to 0.

    FIONREAD - Determine the amount of data which can be read atomically from
    socket s. lpvOutBuffer points at an unsigned long in which WSPIoctl()
    stores the result. If s is stream-oriented (e.g., type SOCK_STREAM),
    FIONREAD returns the total amount of data which may be read in a single
    receive operation; this is normally the same as the total amount of data
    queued on the socket. If s is message-oriented (e.g., type SOCK_DGRAM),
    FIONREAD returns the size of the first datagram (message) queued on the
    socket.

    SIOCATMARK - Determine whether or not all out-of-band data has been read.
    This applies only to a socket of stream style (e.g., type SOCK_STREAM)
    which has been configured for in-line reception of any out-of-band data
    (SO_OOBINLINE). If no out-of-band data is waiting to be read, the operation
    returns TRUE. Otherwise it returns FALSE, and the next receive operation
    performed on the socket will retrieve some or all of the data preceding the "
    mark"; the WinSock SPI client should use the SIOCATMARK operation to
    determine whether any remains. If there is any normal data preceding the
    "urgent" (out of band) data, it will be received in order. (Note that
    receive operations will never mix out-of-band and normal data in the same
    call.) lpvOutBuffer points at a BOOL in which WSPIoctl() stores the result.

    The following WinSock 2 commands are supported:

    SIO_ASSOCIATE_HANDLE (opcode setting: I, T==1) - Associate this socket with
    the specified handle of a companion interface. The input buffer contains
    the integer value corresponding to the manifest constant for the companion
    interface (e.g., TH_NETDEV, TH_TAPI, etc.), followed by a value which is a
    handle of the specified companion interface, along with any other required
    information. Refer to the appropriate section in the Windows Sockets 2
    Protocol-Specific Annex and/or documentation for the particular companion
    interface for additional details. The total size is reflected in the input
    buffer length. No output buffer is required. The WSAENOPROTOOPT error code
    is indicated for service providers which do not support this ioctl.

    SIO_ENABLE_CIRCULAR_QUEUEING (opcode setting: V, T==1) - Indicates to a
    message-oriented service provider that a newly arrived message should
    never be dropped because of a buffer queue overflow. Instead, the oldest
    message in the queue should be eliminated in order to accommodate the newly
    arrived message. No input and output buffers are required. Note that this
    ioctl is only valid for sockets associated with unreliable, message-
    oriented protocols. The WSAENOPROTOOPT error code is indicated for service
    providers which do not support this ioctl.

    SIO_FIND_ROUTE (opcode setting: O, T==1) - When issued, this ioctl requests
    that the route to the remote address specified as a sockaddr in the input
    buffer be discovered. If the address already exists in the local cache, its
    entry is invalidated. In the case of Novell's IPX, this call initiates an
    IPX GetLocalTarget (GLT), which queries the network for the given remote
    address.

    SIO_FLUSH (opcode setting: V, T==1) - Discards current contents of the
    sending queue associated with this socket. No input and output buffers are
    required. The WSAENOPROTOOPT error code is indicated for service providers
    which do not support this ioctl.

    SIO_GET_BROADCAST_ADDRESS (opcode setting: O, T==1) - This ioctl fills the
    output buffer with a sockaddr struct containing a suitable broadcast
    address for use with WSPIoctl().

    SIO_GET_EXTENSION_FUNCTION_POINTER (opcode setting: O, I, T==1) - Retrieve
    a pointer to the specified extension function supported by the associated
    service provider. The input buffer contains a GUID whose value identifies
    the extension function in question. The pointer to the desired function is
    returned in the output buffer. Extension function identifiers are
    established by service provider vendors and should be included in vendor
    documentation that describes extension function capabilities and semantics.

    SIO_GET_QOS (opcode setting: O,I, T==1) - Retrieve the QOS structure
    associated with the socket. The input buffer is optional. Some protocols
    (e.g. RSVP) allow the input buffer to be used to qualify a QOS request.
    The QOS structure will be copied into the output buffer. The output buffer
    must be sized large enough to be able to contain the full QOS struct. The
    WSAENOPROTOOPT error code is indicated for service providers which do not
    support QOS.

    SIO_GET_GROUP_QOS (opcode setting: O,I, T==1) - Retrieve the QOS structure
    associated with the socket group to which this socket belongs. The input
    buffer is optional. Some protocols (e.g. RSVP) allow the input buffer to
    be used to qualify a QOS request. The QOS structure will be copied into
    the output buffer. If this socket does not belong to an appropriate socket
    group, the SendingFlowspec and ReceivingFlowspec fields of the returned QOS
    struct are set to NULL. The WSAENOPROTOOPT error code is indicated for
    service providers which do not support QOS.

    SIO_MULTIPOINT_LOOPBACK (opcode setting: I, T==1) - Controls whether data
    sent in a multipoint session will also be received by the same socket on
    the local host. A value of TRUE causes loopback reception to occur while a
    value of FALSE  prohibits this.

    SIO_MULTICAST_SCOPE (opcode setting: I, T==1) - Specifies the scope over
    which multicast transmissions will occur. Scope is defined as the number of
    routed network segments to be covered. A scope of zero would indicate that
    the multicast transmission would not be placed "on the wire" but could be
    disseminated across sockets within the local host. A scope value of one
    (the default) indicates that the transmission will be placed on the wire,
    but will not cross any routers. Higher scope values determine the number of
    routers that may be crossed. Note that this corresponds to the time-to-live
    (TTL) parameter in IP multicasting.

    SIO_SET_QOS (opcode setting: I, T==1) - Associate the supplied QOS
    structure with the socket. No output buffer is required, the QOS structure
    will be obtained from the input buffer. The WSAENOPROTOOPT error code is
    indicated for service providers which do not support QOS.

    SIO_SET_GROUP_QOS   (opcode setting: I, T==1) - Establish the supplied QOS
    structure with the socket group to which this socket belongs. No output
    buffer is required, the QOS structure will be obtained from the input
    buffer. The WSAENOPROTOOPT error code is indicated for service providers
    which do not support QOS, or if the socket descriptor specified is not the
    creator of the associated socket group.

    SIO_TRANSLATE_HANDLE (opcode setting: I, O, T==1) - To obtain a
    corresponding handle for socket s that is valid in the context of a
    companion interface (e.g., TH_NETDEV, TH_TAPI, etc.). A manifest constant
    identifying the companion interface along with any other needed parameters
    are specified in the input buffer. The corresponding handle will be
    available in the output buffer upon completion of this routine. Refer to
    the appropriate section in the Windows Sockets 2 Protocol-Specific Annex
    and/or documentation for the particular companion interface for additional
    details. The WSAENOPROTOOPT error code is indicated for service providers
    which do not support this ioctl for the specified companion interface.

    If an overlapped operation completes immediately, WSPIoctl() returns a
    value of zero and the lpNumberOfBytesReturned parameter is updated with
    the number of bytes returned. If the overlapped operation is successfully
    initiated and will complete later, WSPIoctl() returns SOCKET_ERROR and
    indicates error code WSA_IO_PENDING. In this case, lpNumberOfBytesReturned
    is not updated. When the overlapped operation completes the amount of data
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

    The ioctl codes with T == 0 are a subset of the ioctl codes used in
    Berkeley sockets. In particular, there is no command which is equivalent
    to FIOASYNC.

Arguments:

    s - Handle to a socket

    dwIoControlCode - Control code of operation to perform

    lpvInBuffer - Address of input buffer

    cbInBuffer - Size of input buffer

    lpvOutBuffer - Address of output buffer

    cbOutBuffer - Size of output buffer

    lpcbBytesReturned - A pointer to the size of output buffer's contents.

    lpOverlapped - Address of WSAOVERLAPPED structure

    lpCompletionRoutine - A pointer to the completion routine called when
        the operation has been completed.

    lpThreadId - A pointer to a thread ID structure to be used by the
        provider in a subsequent call to WPUQueueApc(). The provider
        should store the referenced WSATHREADID structure (not the pointer
        to same) until after the WPUQueueApc() function returns.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs and the operation has completed immediately, WSPIoctl()
        returns 0. Note that in this case the completion routine, if specified,
        will have already been queued. Otherwise, a value of SOCKET_ERROR is
        returned, and a specific error code is available in lpErrno. The error
        code WSA_IO_PENDING indicates that an overlapped operation has been
        successfully initiated and that completion will be indicated at a later
        time. Any other error code indicates that no overlapped operation was
        initiated and no completion indication will occur.

--*/

{
	PWINSOCK_TLS_DATA	tlsData;
    int err;
    PSOCKET_INFORMATION socket;
    AFD_RECEIVE_INFORMATION receiveInformation;
    BOOLEAN blocking;
    u_long * inArgument;
    u_long * outArgument;
    BOOL needsCompletion;

    WS_ENTER( "WSPIoctl", (PVOID)Handle, (PVOID)dwIoControlCode, lpvInBuffer, (PVOID)cbInBuffer );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPIoctl", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    if( lpcbBytesReturned == NULL ) {

        WS_EXIT( "WSPIoctl", SOCKET_ERROR, TRUE );
        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;

    }

    //
    // Quick code path for extension query
    //
    if ((dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER) &&
            (lpOverlapped==NULL)) {

        INT i;

        if( cbInBuffer < sizeof(GUID) || lpvInBuffer == NULL ||
            cbOutBuffer < sizeof(LPVOID) || lpvOutBuffer == NULL ) {

            err = WSAEFAULT;
            return SOCKET_ERROR;

        }

        __try {
            for( i = 0 ; i < NUM_EXTENSIONS ; i++ ) {

                if( RtlEqualMemory(
                        lpvInBuffer,
                        &SockExtensions[i].Guid,
                        sizeof(GUID)
                        ) ) {

                    *((LPVOID *)lpvOutBuffer) = SockExtensions[i].Function;
                    *lpcbBytesReturned = sizeof(LPVOID);

                    WS_EXIT( "WSPIoctl", NO_ERROR, FALSE );
                    return NO_ERROR;
                }
            }
        }
        __except (SOCK_EXCEPTION_FILTER()) {
            err = WSAEFAULT;
            return SOCKET_ERROR;
        }
    }


    //
    // Set up locals so that we know how to clean up on exit.
    //

    inArgument = (u_long *)lpvInBuffer;
    outArgument = (u_long *)lpvOutBuffer;
    needsCompletion = TRUE;
    socket = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        WS_EXIT( "WSPIoctl", SOCKET_ERROR, TRUE );
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;

    }

    //
    // Get exclusive access to the socket in question.  This is necessary
    // because we'll be changing the socket information data structure.
    //

    SockAcquireSocketLockExclusive( socket );

    __try {
        //
        // Act based on the specified command.
        //

        switch ( dwIoControlCode ) {

        case FIONBIO:

            //
            // Put the socket into or take it out of non-blocking mode.
            //

            if( cbInBuffer < sizeof(ULONG) || lpvInBuffer == NULL ) {

                err = WSAEFAULT;
                goto exit;

            }

            *lpcbBytesReturned = 0;

            if ( *inArgument != 0 ) {

                blocking = TRUE;

                err = SockSetInformation(
                            socket,
                            AFD_NONBLOCKING_MODE,
                            &blocking,
                            NULL,
                            NULL
                            );

                if ( err == NO_ERROR ) {

                    socket->NonBlocking = TRUE;

                }

            } else {

                //
                // It is illegal to set a socket to blocking if there are
                // WSAAsyncSelect() or WSAEventSelect() events on the socket.
                //

                if ( socket->AsyncSelectlEvent != 0 ||
                     socket->EventSelectlNetworkEvents != 0 ) {

                    err = WSAEINVAL;
                    goto exit;

                }

                blocking = FALSE;

                err = SockSetInformation(
                            socket,
                            AFD_NONBLOCKING_MODE,
                            &blocking,
                            NULL,
                            NULL
                            );

                if ( err == NO_ERROR ) {

                    socket->NonBlocking = FALSE;

                }

            }

            break;

        case FIONREAD:

            //
            // Return the number of bytes that can be read from the socket
            // without blocking.
            //

            if( cbOutBuffer < sizeof(ULONG) || lpvOutBuffer == NULL ) {

                err = WSAEFAULT;
                goto exit;

            }

#ifdef _AFD_SAN_SWITCH_
			//
			// If we have a SAN socket open, then get info from there
			//
			if (SockSanEnabled && socket->SanSocket && 
				socket->SanSocket->IsConnected == CONNECTED) {

				DWORD largeSendSize;
				DWORD FCCode;

				receiveInformation.BytesAvailable = socket->SanSocket->ReceiveBytesBuffered;
				receiveInformation.ExpeditedBytesAvailable =
					socket->SanSocket->ExpeditedBytesBuffered;

				//
				// Handle case when remote side has told us of a large send. Include that
				// in num. data bytes available for recv. 
				//
				FCCode = GetLargeSendFCInfoNoPop(socket->SanSocket, &largeSendSize, TRUE);
				if ( FCCode	== SEND_AVAILABLE)
					receiveInformation.BytesAvailable += largeSendSize;

				err = NO_ERROR;
			}
			else {
#endif //_AFD_SAN_SWITCH_

            err = GetReceiveInformation( socket, &receiveInformation );

#ifdef _AFD_SAN_SWITCH_
			}
#endif //_AFD_SAN_SWITCH_

            if ( err != NO_ERROR ) {

                goto exit;

            }

            *lpcbBytesReturned = sizeof(ULONG);

            //
            // If this socket is set for reading out-of-band data inline,
            // include the number of expedited bytes available in the result.
            // If the socket is not set for SO_OOBINLINE, just return the
            // number of normal bytes available.
            //

            if ( socket->OobInline ) {

                *outArgument = receiveInformation.BytesAvailable +
                                receiveInformation.ExpeditedBytesAvailable;

            } else {

                *outArgument = receiveInformation.BytesAvailable;

            }

            //
            // If there are more bytes available than the size of the socket's
            // buffer, truncate to the buffer size.
            //

            if ( *outArgument > socket->ReceiveBufferSize ) {

                *outArgument = socket->ReceiveBufferSize;

            }

            break;


        case SIOCATMARK: {

            //
            // If this is a datagram socket, fail the request.
            //

            if( cbOutBuffer < sizeof(ULONG) || lpvOutBuffer == NULL ) {

                err = WSAEFAULT;
                goto exit;

            }

            if ( IS_DGRAM_SOCK(socket) ) {

                err = WSAEINVAL;
                goto exit;

            }

            //
            // Return a BOOL that indicates whether there is expedited data
            // to be read on the socket.
            //

            err = GetReceiveInformation( socket, &receiveInformation );

            if ( err != NO_ERROR ) {

                goto exit;

            }

            *lpcbBytesReturned = sizeof(ULONG);

            if ( receiveInformation.ExpeditedBytesAvailable != 0 ) {

                *outArgument = FALSE;

            } else {

                *outArgument = TRUE;

            }

            }
            break;

        case SIO_GET_QOS :
        case SIO_GET_GROUP_QOS :

            if (socket->ServiceFlags1&XP1_QOS_SUPPORTED) {
                goto PassToHelper;
            }
            else {
                NTSTATUS status;
                AFD_QOS_INFO qosInfo;
                IO_STATUS_BLOCK ioStatusBlock;

                if( cbOutBuffer < sizeof(QOS) || lpvOutBuffer == NULL ) {

                    *lpcbBytesReturned = sizeof(QOS);
                    err = WSAEFAULT;
                    goto exit;

                }

                //
                // Get it from AFD.
                //

                qosInfo.GroupQos = ( dwIoControlCode == SIO_GET_GROUP_QOS );

                status = NtDeviceIoControlFile(
                             (HANDLE)Handle,
                             tlsData->EventHandle,
                             NULL,
                             NULL,
                             &ioStatusBlock,
                             IOCTL_AFD_GET_QOS,
                             &qosInfo,
                             sizeof(qosInfo),
                             &qosInfo,
                             sizeof(qosInfo)
                             );

                if( status == STATUS_PENDING ) {

                    BOOL success;

                    success = SockWaitForSingleObject(
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
                // Success!
                //

                RtlCopyMemory(
                    lpvOutBuffer,
                    &qosInfo.Qos,
                    sizeof(QOS)
                    );

                *lpcbBytesReturned = sizeof(QOS);
            }
            break;

        case SIO_SET_QOS :
        case SIO_SET_GROUP_QOS : 

            //
            // Reenabled async select events on any call (event failing)
            // of the IOCTL
            //
            if ( dwIoControlCode == SIO_SET_GROUP_QOS )
                socket->DisabledAsyncSelectEvents &= ~FD_GROUP_QOS;
            else
                socket->DisabledAsyncSelectEvents &= ~FD_QOS;

            if (socket->ServiceFlags1&XP1_QOS_SUPPORTED) {
                goto PassToHelper;
            }
            else {

                NTSTATUS status;
                AFD_QOS_INFO qosInfo;
                IO_STATUS_BLOCK ioStatusBlock;

                if( cbInBuffer < sizeof(QOS) || lpvInBuffer == NULL ) {

                    err = WSAEFAULT;
                    goto exit;

                }

                //
                // Give it to AFD.
                //

                RtlCopyMemory(
                    &qosInfo.Qos,
                    lpvInBuffer,
                    sizeof(QOS)
                    );

                qosInfo.GroupQos = ( dwIoControlCode == SIO_SET_GROUP_QOS );

                status = NtDeviceIoControlFile(
                             (HANDLE)Handle,
                             tlsData->EventHandle,
                             NULL,
                             NULL,
                             &ioStatusBlock,
                             IOCTL_AFD_SET_QOS,
                             &qosInfo,
                             sizeof(qosInfo),
                             NULL,
                             0
                             );

                if( status == STATUS_PENDING ) {

                    BOOL success;

                    success = SockWaitForSingleObject(
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
                // Success!
                //

                *lpcbBytesReturned = 0;

            }
            break;

        case SIO_ENABLE_CIRCULAR_QUEUEING : {

            BOOLEAN enable = TRUE;

            if( !IS_DGRAM_SOCK( socket ) ) {
                err = WSAEINVAL;
                goto exit;
            }

            err = SockSetInformation(
                      socket,
                      AFD_CIRCULAR_QUEUEING,
                      &enable,
                      NULL,
                      NULL
                      );

            }
            break;

        case SIO_GET_BROADCAST_ADDRESS : {

            PWSH_GET_BROADCAST_SOCKADDR getBroadcastSockaddr;
            INT sockaddrLength;

            if( !IS_DGRAM_SOCK( socket ) ) {
                err = WSAEINVAL;
                goto exit;
            }

            getBroadcastSockaddr = socket->HelperDll->WSHGetBroadcastSockaddr;

            if( getBroadcastSockaddr == NULL ) {
                err = WSAENOPROTOOPT;
                goto exit;
            }

            sockaddrLength = (INT)cbOutBuffer;

            err = (getBroadcastSockaddr)(
                      socket->HelperDllContext,
                      (PSOCKADDR)lpvOutBuffer,
                      &sockaddrLength
                      );

            if( err == NO_ERROR ) {
                *lpcbBytesReturned = (DWORD)sockaddrLength;
            }

            }
            break;

        case SIO_ASSOCIATE_HANDLE :
        case SIO_TRANSLATE_HANDLE :
            err = WSAENOPROTOOPT;
            break;

#ifdef _PNP_POWER_
        case SIO_ROUTING_INTERFACE_QUERY: {
            PTRANSPORT_ADDRESS tdiAddress;
            ULONG tdiAddressLength;
            SOCKADDR_INFO sockaddrInfo;
            NTSTATUS    status;
            IO_STATUS_BLOCK ioStatusBlock;

            if ( lpvInBuffer==NULL ||
                    ((int)cbInBuffer < socket->HelperDll->MinSockaddrLength) ) {
                err = WSAEINVAL;
                goto exit;

            }
            //
            // If the buffer passed in is larger than needed for a socket address,
            // just truncate it.  Otherwise, the TDI address buffer we allocate
            // may not be large enough.
            //

            if ( (int)cbInBuffer > socket->HelperDll->MaxSockaddrLength ) {

                cbInBuffer = socket->HelperDll->MaxSockaddrLength;

            }

            //
            // Make sure that the address structure passed in is legitimate,
            // and determine the type of address we're binding to.
            //

            err = socket->HelperDll->WSHGetSockaddrType(
                        (PSOCKADDR)lpvInBuffer,
                        cbInBuffer,
                        &sockaddrInfo
                        );

            if ( err != NO_ERROR) {

                goto exit;

            }

            if (sockaddrInfo.AddressInfo == SockaddrAddressInfoWildcard) {
                err = WSAEINVAL;

                goto exit;
            }

            // Allocate enough space to hold the TDI address structure we'll pass
            // to AFD.
            //

            tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;

            tdiAddress = ALLOCATE_HEAP( tdiAddressLength + 4 );

            if ( tdiAddress == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

            //
            // Convert the address from the sockaddr structure to the appropriate
            // TDI structure.
            //

            SockBuildTdiAddress(
                tdiAddress,
                (PSOCKADDR)lpvInBuffer,
                cbInBuffer
                );

        
            status = NtDeviceIoControlFile(
                         (HANDLE)Handle,
                         tlsData->EventHandle,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_ROUTING_INTERFACE_QUERY,
                         tdiAddress,
                         FIELD_OFFSET (TRANSPORT_ADDRESS,
                            Address[0].Address[tdiAddress->Address[0].AddressLength]),
                         lpvOutBuffer,
                         cbOutBuffer
                         );

            if( status == STATUS_PENDING ) {

                BOOL success;

                success = SockWaitForSingleObject(
                              tlsData->EventHandle,
                              Handle,
                              SOCK_NEVER_CALL_BLOCKING_HOOK,
                              SOCK_NO_TIMEOUT
                              );

                status = ioStatusBlock.Status;

            }

            FREE_HEAP (tdiAddress);
            if (status==STATUS_BUFFER_OVERFLOW) {
                err = WSAEFAULT;
            }
            else if( !NT_SUCCESS(status) ) {

                err = SockNtStatusToSocketError( status );
                goto exit;

            }

            *lpcbBytesReturned = (DWORD)ioStatusBlock.Information;

            break;
            }

        case SIO_ROUTING_INTERFACE_CHANGE: {
            PTRANSPORT_ADDRESS tdiAddress;
            ULONG tdiAddressLength;
            SOCKADDR_INFO sockaddrInfo;

            if ( lpvInBuffer==NULL ||
                    ((int)cbInBuffer < socket->HelperDll->MinSockaddrLength) ||
                    (cbOutBuffer!=0) ) {
                err = WSAEINVAL;
                goto exit;
            }

            //
            // If the buffer passed in is larger than needed for a socket address,
            // just truncate it.  Otherwise, the TDI address buffer we allocate
            // may not be large enough.
            //

            if ( (int)cbInBuffer > socket->HelperDll->MaxSockaddrLength ) {

                cbInBuffer = socket->HelperDll->MaxSockaddrLength;

            }

            //
            // Make sure that the address structure passed in is legitimate,
            // and determine the type of address we're binding to.
            //

            err = socket->HelperDll->WSHGetSockaddrType(
                        (PSOCKADDR)lpvInBuffer,
                        cbInBuffer,
                        &sockaddrInfo
                        );

            if ( err != NO_ERROR) {

                goto exit;

            }

            // Allocate enough space to hold the TDI address structure we'll pass
            // to AFD.
            //

            tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;

            tdiAddress = ALLOCATE_HEAP( tdiAddressLength + 4 );

            if ( tdiAddress == NULL ) {

                err = WSAENOBUFS;
                goto exit;

            }

            //
            // Convert the address from the sockaddr structure to the appropriate
            // TDI structure.
            //

            SockBuildTdiAddress(
                tdiAddress,
                (PSOCKADDR)lpvInBuffer,
                cbInBuffer
                );

        
            WS_ASSERT( (IOCTL_AFD_ROUTING_INTERFACE_CHANGE & 0x03) == METHOD_BUFFERED );
            err = DoAsyncIoctl (
                         socket,
                         NULL,
                         AFD_POLL_ROUTING_IF_CHANGE,
                         IOCTL_AFD_ROUTING_INTERFACE_CHANGE,
                         tdiAddress,
                         FIELD_OFFSET (TRANSPORT_ADDRESS,
                            Address[0].Address[tdiAddress->Address[0].AddressLength]),
                         NULL,
                         0,
                         lpcbBytesReturned,
                         lpOverlapped,
                         lpCompletionRoutine,
                         lpErrno
                         );

            FREE_HEAP (tdiAddress);

            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

            return err;

            }

        case SIO_ADDRESS_LIST_QUERY: {
            PTRANSPORT_ADDRESS      tAddrList;
            PUCHAR                  tAddrBuf;
            ULONG                   tAddrBufLen;
            LPSOCKET_ADDRESS_LIST   sAddrList;
            PUCHAR                  sAddrBuf;
            NTSTATUS                status;
            IO_STATUS_BLOCK         ioStatusBlock;
            INT                     taAddressCount;
            INT                     i;
            USHORT                  family;


            *lpcbBytesReturned = 0;

            if (cbInBuffer!=0) {
                err = WSAEINVAL;
                goto exit;
            }

            if ((lpvOutBuffer!=NULL) && (cbOutBuffer>0)) {
                tAddrBufLen = cbOutBuffer;
                tAddrList = ALLOCATE_HEAP (tAddrBufLen);
                if (tAddrList==NULL) {
                    err = WSAENOBUFS;
                    goto exit;
                }
            }
            else {
                tAddrList = (PTRANSPORT_ADDRESS)&taAddressCount;
                tAddrBufLen = sizeof (ULONG);
            }

            family = (USHORT)socket->AddressFamily;

            status = NtDeviceIoControlFile(
                         (HANDLE)Handle,
                         tlsData->EventHandle,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IOCTL_AFD_ADDRESS_LIST_QUERY,
                         &family,
                         sizeof (family),
                         tAddrList,
                         tAddrBufLen
                         );

            if( status == STATUS_PENDING ) {

                BOOL success;

                success = SockWaitForSingleObject(
                              tlsData->EventHandle,
                              Handle,
                              SOCK_NEVER_CALL_BLOCKING_HOOK,
                              SOCK_NO_TIMEOUT
                              );

                status = ioStatusBlock.Status;

            }

            if ((status==STATUS_SUCCESS) || (status==STATUS_BUFFER_OVERFLOW)) {
                *lpcbBytesReturned = (ULONG)ioStatusBlock.Information
                                        + tAddrList->TAAddressCount
                                            *(sizeof (SOCKET_ADDRESS)-sizeof(USHORT));
                if ((status==STATUS_SUCCESS)
                        && (*lpcbBytesReturned<=cbOutBuffer)) {
                    tAddrBuf = (PUCHAR)tAddrList->Address;
                    sAddrList = lpvOutBuffer;
                    sAddrBuf = (PUCHAR)&sAddrList->Address[tAddrList->TAAddressCount];
                    sAddrList->iAddressCount = tAddrList->TAAddressCount;
                    for (i=0; i<tAddrList->TAAddressCount; i++) {
                        PTA_ADDRESS ta = (PTA_ADDRESS)tAddrBuf;
                        ULONG       taLen = FIELD_OFFSET (TA_ADDRESS,
                                                            Address[ta->AddressLength]);
                        INT         saLen = ta->AddressLength+sizeof(USHORT);
                        sAddrList->Address[i].lpSockaddr = (LPSOCKADDR)sAddrBuf;
                        sAddrList->Address[i].iSockaddrLength = saLen;
                        RtlCopyMemory (sAddrBuf, &ta->AddressType, saLen);
                        sAddrBuf += saLen;
                        tAddrBuf += taLen;
                    }
                }
                else {
                    err = WSAEFAULT;
                }
            }
            else {
                err = SockNtStatusToSocketError( status );
            }
            if (tAddrList != (PTRANSPORT_ADDRESS)&taAddressCount) {
                FREE_HEAP (tAddrList);
            }

            break;
            }
        case SIO_ADDRESS_LIST_CHANGE: {
            USHORT                  family;

            if (cbInBuffer!=0 || cbOutBuffer!=0) {
                err = WSAEINVAL;
                goto exit;
            }

            family = (USHORT)socket->AddressFamily;
            err = DoAsyncIoctl (
                         socket,
                         NULL,
                         AFD_POLL_ADDRESS_LIST_CHANGE,
                         IOCTL_AFD_ADDRESS_LIST_CHANGE,
                         &family,
                         sizeof (family),
                         NULL,
                         0,
                         lpcbBytesReturned,
                         lpOverlapped,
                         lpCompletionRoutine,
                         lpErrno
                         );

            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

            return err;

            }
#endif // _PNP_POWER_
        case SIO_GET_EXTENSION_FUNCTION_POINTER : {

            LPVOID * outBuffer;
            INT i;

            if( cbInBuffer < sizeof(GUID) || lpvInBuffer == NULL ||
                cbOutBuffer < sizeof(LPVOID) || lpvOutBuffer == NULL ) {

                err = WSAEFAULT;
                goto exit;

            }

            outBuffer = (LPVOID *)lpvOutBuffer;

            for( i = 0 ; i < NUM_EXTENSIONS ; i++ ) {

                if( RtlEqualMemory(
                        lpvInBuffer,
                        &SockExtensions[i].Guid,
                        sizeof(GUID)
                        ) ) {

                    *outBuffer = SockExtensions[i].Function;
                    *lpcbBytesReturned = sizeof(LPVOID);
                    goto exit;

                }

            }

            }

        default: 
        PassToHelper:
            {

            PWSH_IOCTL wshIoctl;

            //
            // Unknown IOCTL. If this socket's helper DLL supports the
            // WSHIoctl entrypoint, just pass the request off to it.
            // Otherwise, fail it here.
            //

            wshIoctl = socket->HelperDll->WSHIoctl;

            if( wshIoctl == NULL ) {

                err = WSAEINVAL;
                goto exit;

            }

            err = (wshIoctl)(
                      socket->HelperDllContext,
                      Handle,
                      socket->TdiAddressHandle,
                      socket->TdiConnectionHandle,
                      dwIoControlCode,
                      lpvInBuffer,
                      cbInBuffer,
                      lpvOutBuffer,
                      cbOutBuffer,
                      lpcbBytesReturned,
                      lpOverlapped,
                      lpCompletionRoutine,
                      &needsCompletion
                      );

            if( err != NO_ERROR &&
                dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER ) {

                //
                // Ensure the proper error is returned.
                //

                err = WSAEINVAL;

            }

            }
            break;

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {

        err = WSAEFAULT;
        goto exit;
    }

exit:

    IF_DEBUG(SOCKOPT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPIoctl on socket %lx, command %lx failed: %ld\n",
                           Handle, dwIoControlCode, err ));

        } else {

            WS_PRINT(( "WSPIoctl on socket %lx command %lx returning arg "
                       "%ld\n", Handle, dwIoControlCode, *outArgument ));

        }

    }

    //
    // If this is an overlapped request, then send a "NOP" IOCTL to
    // AFD to do all of the correct IO completion stuff. This is necessary
    // as we do not yet (really) support overlapped WSPIoctl(), but the
    // caller will be expecting the call to Do The Right Thing regarding
    // completion routines, event object, and IO completion ports.
    //

    if( lpOverlapped != NULL &&
        err == NO_ERROR &&
        ( socket->CreationFlags & WSA_FLAG_OVERLAPPED ) != 0 &&
        needsCompletion ) {

        NTSTATUS status;
        PIO_STATUS_BLOCK ioStatusBlock;
        HANDLE event;
        PIO_APC_ROUTINE apcRoutine;
        PVOID apcContext;

        __try {

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

            }

            //
            // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
            //

            ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

            //
            // Issue the request.
            //

            ioStatusBlock->Status = STATUS_PENDING;
            status = NtDeviceIoControlFile(
                         (HANDLE)Handle,
                         event,
                         apcRoutine,
                         apcContext,
                         ioStatusBlock,
                         IOCTL_AFD_NO_OPERATION,
                         NULL,
                         0,
                         NULL,
                         0
                         );

            WS_ASSERT (status!=STATUS_PENDING);

            switch (status) {
            case STATUS_SUCCESS:
                break;

            case STATUS_INVALID_HANDLE:
            case STATUS_OBJECT_TYPE_MISMATCH:
                //
                // Check if invalid handle status code is
                // returned for the event handle and return
                // appropriate error code instead of WSAENOTSOCK
                //
                if ((lpOverlapped!=NULL) && 
                        (lpCompletionRoutine==NULL) &&
                        (((ULONG_PTR)event & 1)!=0)) {
                    EVENT_BASIC_INFORMATION info;
                    if (NtQueryEvent (event,
                                        EventBasicInformation,
                                        &info,
                                        sizeof (info),
                                        NULL)==status) {
                         err = WSA_INVALID_HANDLE;
                         break;
                    }
                }
                err = WSAENOTSOCK;
                break;
            default:
                if (!NT_SUCCESS(status) ) {
                    //
                    // Map the NTSTATUS to a WinSock error code.
                    //

                    err = SockNtStatusToSocketError( status );
                }
                break;
            }

        }
        __except (SOCK_EXCEPTION_FILTER()) {

            err = WSAEFAULT;
        }
    }

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPIoctl", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPIoctl", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPIoctl


int
WSPAPI
WSPSetSockOpt (
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    IN const char *OptionValue,
    IN int OptionLength,
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
	PWINSOCK_TLS_DATA	tlsData;
    int err;
    PSOCKET_INFORMATION socket;
    INT optionValue;
    INT previousValue;

    WS_ENTER( "WSPSetSockOpt", (PVOID)Handle, (PVOID)Level, (PVOID)OptionName, (PVOID)OptionValue );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPSetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    socket = NULL;

    //
    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {

        WS_EXIT( "WSPSetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;

    }

    //
    // Get exclusive access to the socket in question.  This is necessary
    // because we'll be changing the socket information data structure.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Ensure the socket isn't closing on us.
    //

    if( socket->State == SocketStateClosing ) {

        err = WSAENOTSOCK;
        goto exit;

    }

    //
    // Make sure that the OptionValue argument is present.
    //

    if ( !ARGUMENT_PRESENT( OptionValue ) ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Make sure that this is a legitimate option for the socket.
    //

    if ( !IsValidOptionForSocket( socket, Level, OptionName ) ) {

        err = WSAENOPROTOOPT;
        goto exit;

    }

    __try {

        if (OptionLength>=sizeof (int)) {
            optionValue = *((INT UNALIGNED *)OptionValue);
        }
        else {
            optionValue = (UCHAR)*OptionValue;
        }

        //
        // Act on the specified level.
        //

        switch ( Level ) {

        case SOL_SOCKET:

            //
            // Act based on the option being set.
            //

            switch ( OptionName ) {

            case SO_BROADCAST:
            {
                BOOLEAN PrevBroadcast = socket->Broadcast;

                if ( optionValue == 0 ) {

                    socket->Broadcast = FALSE;

                } else {

                    socket->Broadcast = TRUE;

                }

                //
                // Now pass on to helper dll, but don't return any errors
                // in doing so, since we never did this (or returned an
                // error) in NT 4.0.
                //

                if ( ( PrevBroadcast != socket->Broadcast )  &&
                     ( SockGetTdiHandles( socket ) == NO_ERROR ) ) {

                    socket->HelperDll->WSHSetSocketInformation(
                        socket->HelperDllContext,
                        Handle,
                        socket->TdiAddressHandle,
                        socket->TdiConnectionHandle,
                        Level,
                        OptionName,
                        (PCHAR)OptionValue,
                        OptionLength
                        );
                }

                break;
            }
            case SO_DEBUG:

                if ( optionValue == 0 ) {

                    socket->Debug = FALSE;

                } else {

                    socket->Debug = TRUE;

                }

#ifdef _AFD_SAN_SWITCH_
				//
				// If we have a SAN socket open, then pass option on to SAN provider
				//
				if (SockSanEnabled && socket->SanSocket && 
					socket->SanSocket->IsConnected == CONNECTED) {

					SAN_CALL_SPI(socket->SanSocket->SanProvider,
							WSPSetSockOpt, (PROVIDER(socket->SanSocket->SanProvider)
                                 socket->SanSocket->SanSocket,
                                 Level,
                                 OptionName,
                                 OptionValue,
                                 OptionLength,
                                 &err
                                 ));
				}
#endif //_AFD_SAN_SWITCH_

                break;

            case SO_DONTLINGER:

                if ( optionValue == 0 ) {

                    socket->LingerInfo.l_onoff = 1;

                } else {

                    socket->LingerInfo.l_onoff = 0;

                }

                break;

            case SO_LINGER:

                if ( OptionLength < sizeof(struct linger) ) {

                    err = WSAEFAULT;
                    goto exit;

                }

                RtlCopyMemory(
                    &socket->LingerInfo,
                    OptionValue,
                    sizeof(socket->LingerInfo)
                    );

                break;

            case SO_OOBINLINE: {

                BOOLEAN inLine;

                if ( optionValue == 0 ) {

                    inLine = FALSE;

                } else {

                    inLine = TRUE;

                }

                err = SockSetInformation(
                            socket,
                            AFD_INLINE_MODE,
                            &inLine,
                            NULL,
                            NULL
                            );

                if ( err != NO_ERROR ) {

                    goto exit;

                }

                socket->OobInline = inLine;

                break;
            }

            case SO_RCVBUF:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                previousValue = socket->ReceiveBufferSize;
                socket->ReceiveBufferSize = optionValue;

                err = SockUpdateWindowSizes( socket, TRUE );

                if ( err != NO_ERROR ) {

                    socket->ReceiveBufferSize = previousValue;
                    goto exit;

                }

                break;

            case SO_SNDBUF:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                previousValue = socket->SendBufferSize;
                socket->SendBufferSize = optionValue;

                err = SockUpdateWindowSizes( socket, TRUE );

                if ( err != NO_ERROR ) {

                    socket->SendBufferSize = previousValue;
                    goto exit;

                }

                break;

            case SO_REUSEADDR:

                if ( optionValue == 0 ) {

                    socket->ReuseAddresses = FALSE;

                } else {

                    if (socket->ExclusiveAddressUse) {
                        err = WSAEINVAL;
                        goto exit;
                    }
                    socket->ReuseAddresses = TRUE;

                }

                break;

            case SO_EXCLUSIVEADDRUSE:

                if ( optionValue == 0 ) {

                    socket->ExclusiveAddressUse = FALSE;

                } else {

                    if (socket->ReuseAddresses) {
                        err = WSAEINVAL;
                        goto exit;
                    }
                    socket->ExclusiveAddressUse = TRUE;
                }

                break;

            case SO_CONDITIONAL_ACCEPT:
                if (socket->Listening) {
                    err = WSAEINVAL;
                    goto exit;
                }

                if (optionValue==0) {
                    socket->ConditionalAccept = FALSE;
                }
                else if (socket->HelperDll->UseDelayedAcceptance>=0) {
                    socket->ConditionalAccept = TRUE;
                }
                else {
                    //
                    // Winsock support for TDI_QUERY_ACCEPT (delayed acceptance)
                    // was not specifically enabled for this transport in the
                    // registry (Transport\Parameters\Winsock\UseDelayedAcceptance)
                    // 
                    err = WSAENOPROTOOPT;
                    goto exit;
                }
                break;

            case SO_CONNDATA:

                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SET_CONNECT_DATA,
                            (PVOID)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_CONNOPT:

                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SET_CONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_DISCDATA:

                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SET_DISCONNECT_DATA,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_DISCOPT:

                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SET_DISCONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_CONNDATALEN:

                if ( OptionLength < sizeof(ULONG) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SIZE_CONNECT_DATA,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_CONNOPTLEN:

                if ( OptionLength < sizeof(ULONG) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SIZE_CONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_DISCDATALEN:

                if ( OptionLength < sizeof(ULONG) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SIZE_DISCONNECT_DATA,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_DISCOPTLEN:

                if ( OptionLength < sizeof(ULONG) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                err = SockSetConnectData(
                            socket,
                            IOCTL_AFD_SIZE_DISCONNECT_OPTIONS,
                            (PCHAR)OptionValue,
                            OptionLength,
                            NULL
                            );
                goto exit;

            case SO_SNDTIMEO:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                socket->SendTimeout = optionValue;
                goto exit;

            case SO_RCVTIMEO:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                socket->ReceiveTimeout = optionValue;
                goto exit;

            case SO_ERROR:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                socket->LastError = optionValue;
                goto exit;

            case SO_UPDATE_ACCEPT_CONTEXT: {

                PSOCKET_INFORMATION listenSocket;

                if ( OptionLength < sizeof(SOCKET) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                //
                // First free any old, leftover information.
                //

                if ( socket->TdiAddressHandle != NULL ) {

                    NtClose( socket->TdiAddressHandle );
                    socket->TdiAddressHandle = NULL;

                }

                if ( socket->TdiConnectionHandle != NULL ) {

                    NtClose( socket->TdiConnectionHandle );
                    socket->TdiConnectionHandle = NULL;

                }

                listenSocket = SockFindAndReferenceSocket( (SOCKET)optionValue, FALSE );

                if ( listenSocket == NULL ) {

                    err = WSAENOTSOCK;
                    goto exit;

                }

                //
                // Get exclusive access to the listening socket.
                //

                SockAcquireSocketLockExclusive( listenSocket );

                //
                // Ensure the socket isn't closing on us.
                //

                if( listenSocket->State == SocketStateClosing ) {

                    err = WSAENOTSOCK;
                    SockReleaseSocketLock( listenSocket );
                    goto exit;

                }

                err = SockCoreAccept( listenSocket, socket );

                SockReleaseSocketLock( listenSocket );
                SockDereferenceSocket( listenSocket );

                goto exit;

            }

            case 0x8000:

                if ( OptionLength < sizeof(USHORT) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                //
                // This is the "special" allow-us-to-bind-to-the-zero-address
                // hack.  Put special stuff in the sin_zero part of the address
                // to tell UDP that 0.0.0.0 means bind to that address, rather
                // than wildcard.
                //
                // This feature is needed to allow DHCP to actually bind to
                // the zero address.
                //

                if ( *(PUSHORT)OptionValue == 1234 ) {

                    socket->DontUseWildcard = TRUE;

                } else {

                    err = WSAENOPROTOOPT;

                }

                break;

            case SO_GROUP_PRIORITY:

                if ( OptionLength < sizeof(int) ) {

                    err = WSAEFAULT;
                    goto exit;

                }
                socket->GroupPriority = optionValue;
                goto exit;

            case SO_DONTROUTE:
            case SO_ACCEPTCONN:
            case SO_KEEPALIVE:
            case SO_RCVLOWAT:
            case SO_SNDLOWAT:
            case SO_TYPE:
            default:

                //
                // The specified option isn't supported here in the winsock DLL.
                // Give the request to the helper DLL.
                //

                err = SockGetTdiHandles( socket );

                if ( err != NO_ERROR ) {

                    goto exit;

                }

                err = socket->HelperDll->WSHSetSocketInformation(
                            socket->HelperDllContext,
                            Handle,
                            socket->TdiAddressHandle,
                            socket->TdiConnectionHandle,
                            Level,
                            OptionName,
                            (PCHAR)OptionValue,
                            OptionLength
                            );

                if ( err != NO_ERROR ) {

                    goto exit;

                }

            }

            break;

        default:

            //
            // The specified level isn't supported here in the winsock DLL.
            // Give the request to the helper DLL.
            //

            err = SockGetTdiHandles( socket );

            if ( err != NO_ERROR ) {

                goto exit;

            }

            err = socket->HelperDll->WSHSetSocketInformation(
                        socket->HelperDllContext,
                        Handle,
                        socket->TdiAddressHandle,
                        socket->TdiConnectionHandle,
                        Level,
                        OptionName,
                        (PCHAR)OptionValue,
                        OptionLength
                        );

            if ( err != NO_ERROR ) {

                goto exit;

            }

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {

        err = WSAEFAULT;
        goto exit;
    }

exit:

    //
    // Release the resource, dereference the socket, set the error if
    // any, and return.
    //

    IF_DEBUG(SOCKOPT) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPSetSockOpt on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPSetSockOpt on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));

        }

    }

    if ( err != NO_ERROR ) {

        if ( socket != NULL ) {

            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

        }

        WS_EXIT( "WSPSetSockOpt", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Remember the changed state of this socket.
    //

    if ( socket != NULL ) {

        err = SockSetHandleContext( socket );

        if ( err != NO_ERROR ) {

            goto exit;

        }

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    WS_EXIT( "WSPSetSockOpt", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPSetSockOpt


int
GetReceiveInformation (
    IN PSOCKET_INFORMATION Socket,
    OUT PAFD_RECEIVE_INFORMATION ReceiveInformation
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    //
    // Get data about the bytes available to be read on the socket.
    //

    status = NtDeviceIoControlFile(
                 Socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_QUERY_RECEIVE_INFO,
                 NULL,                      // InputBuffer
                 0L,                        // InputBufferLength
                 ReceiveInformation,
                 sizeof(*ReceiveInformation)
                 );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    return NO_ERROR;

} // GetReceiveInformation


BOOLEAN
IsValidOptionForSocket (
    IN PSOCKET_INFORMATION Socket,
    IN int Level,
    IN int OptionName
    )
{

    //
    // All levels other than SOL_SOCKET and SOL_INTERNAL could be legal.
    // SOL_INTERNAL is never legal; it is used only for internal
    // communication between the Windows Sockets DLL and helper
    // DLLs.
    //

    if ( Level == SOL_INTERNAL ) {
        return FALSE;
    }

    if ( Level != SOL_SOCKET ) {
        return TRUE;
    }

    //
    // Based on the option, determine whether it is possibly legal for
    // the socket.  For unknown options, assume that the helper DLL will
    // take care of it and return that it is a legal option.
    //

    switch ( OptionName ) {

    case SO_DONTLINGER:
    case SO_KEEPALIVE:
    case SO_LINGER:
    case SO_OOBINLINE:
    case SO_ACCEPTCONN:

        //
        // These options are only legal on VC sockets--if this is a
        // datagram socket, fail.
        //

        if ( IS_DGRAM_SOCK(Socket) ) {
            return FALSE;
        }

        return TRUE;

    case SO_BROADCAST:

        //
        // These options are only valid on datagram sockets--if this is
        // a VC socket, fail.
        //

        if (!IS_DGRAM_SOCK(Socket) ) {
            return FALSE;
        }

        return TRUE;

    case SO_PROTOCOL_INFOA:

        //
        // We should never see this one; it gets mapped at the main
        // WinSock 2 DLL level.
        //

        return FALSE;

    case SO_DEBUG:
    case SO_DONTROUTE:
    case SO_ERROR:
    case SO_RCVBUF:
    case SO_REUSEADDR:
    case SO_SNDBUF:
    case SO_TYPE:
    case SO_PROTOCOL_INFOW:
    case SO_GROUP_ID:
    case SO_GROUP_PRIORITY:
    default:

        //
        // These options are legal on any socket.  Succeed.
        //

        return TRUE;

    }

} // IsValidOptionForSocket


INT
SockUpdateWindowSizes (
    IN PSOCKET_INFORMATION Socket,
    IN BOOLEAN AlwaysUpdate
    )
{
    INT error;

    //
    // If this is an unbound datagram socket or an unconnected VC
    // socket, don't do anything here.
    //

    if ( ( IS_DGRAM_SOCK(Socket) &&
           Socket->State == SocketStateOpen
         )
         ||
         ( !IS_DGRAM_SOCK(Socket) &&
           Socket->State != SocketStateConnected
         )
       ) {

        return NO_ERROR;
    }

    //
    // Give the helper DLL the receive buffer size so it can tell the
    // transport the window size to advertize.
    //

    if ( !IS_DGRAM_SOCK(Socket) ) {

        error = SockGetTdiHandles( Socket );
        if ( error != NO_ERROR ) {
            return error;
        }

        error = Socket->HelperDll->WSHSetSocketInformation(
                    Socket->HelperDllContext,
                    Socket->Handle,
                    Socket->TdiAddressHandle,
                    Socket->TdiConnectionHandle,
                    SOL_SOCKET,
                    SO_RCVBUF,
                    (PCHAR)&Socket->ReceiveBufferSize,
                    sizeof(Socket->ReceiveBufferSize)
                    );
    }

    //
    // If the receive buffer changed or if we should always update the
    // count in AFD, then give the receive info to AFD.
    //

    if ( Socket->ReceiveBufferSize != SockReceiveBufferWindow || AlwaysUpdate ) {
        error = SockSetInformation(
                    Socket,
                    AFD_RECEIVE_WINDOW_SIZE,
                    NULL,
                    &Socket->ReceiveBufferSize,
                    NULL
                    );
        if ( error != NO_ERROR ) {
            return error;
        }
    }

    //
    // Repeat for the send buffer.
    //

    if ( Socket->SendBufferSize != SockSendBufferWindow || AlwaysUpdate ) {
        error = SockSetInformation(
                    Socket,
                    AFD_SEND_WINDOW_SIZE,
                    NULL,
                    &Socket->SendBufferSize,
                    NULL
                    );
        if ( error != NO_ERROR ) {
            return error;
        }
    }

    return NO_ERROR;

} // SockSetSocketReceiveBuffer


INT
SockSetConnectData (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    if ((tlsData->AcceptInfo!=NULL)
            && (tlsData->AcceptInfo->AcceptHandle==Socket->HContext.Handle)) {

        AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;
        connectInfo.Sequence = tlsData->AcceptInfo->Sequence;
        connectInfo.LengthOnly = FALSE;

        status = NtDeviceIoControlFile(
                     Socket->HContext.Handle,
                     tlsData->EventHandle,
                     NULL,
                     0,
                     &ioStatusBlock,
                     AfdIoctl,
                     &connectInfo,
                     sizeof (connectInfo),
                     Buffer,
                     BufferLength
                     );
    }
    else {
        //
        // Give request to AFD.
        //

        status = NtDeviceIoControlFile(
                     Socket->HContext.Handle,
                     tlsData->EventHandle,
                     NULL,
                     0,
                     &ioStatusBlock,
                     AfdIoctl,
                     NULL,
                     0,
                     Buffer,
                     BufferLength
                     );
    }

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    //
    // If requested, set up the length of returned buffer.
    //

    if ( ARGUMENT_PRESENT(OutBufferLength) ) {
        *OutBufferLength = (INT)ioStatusBlock.Information;
    }

    return NO_ERROR;

} // SockSetConnectData


INT
SockGetConnectData (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    if ((tlsData->AcceptInfo!=NULL)
            && (tlsData->AcceptInfo->AcceptHandle==Socket->HContext.Handle)) {

        AFD_UNACCEPTED_CONNECT_DATA_INFO connectInfo;
        connectInfo.Sequence = tlsData->AcceptInfo->Sequence;
        connectInfo.LengthOnly = FALSE;

        status = NtDeviceIoControlFile(
                     Socket->HContext.Handle,
                     tlsData->EventHandle,
                     NULL,
                     0,
                     &ioStatusBlock,
                     AfdIoctl,
                     &connectInfo,
                     sizeof (connectInfo),
                     Buffer,
                     BufferLength
                     );
    }
    else {
        //
        // Give request to AFD.
        //

        status = NtDeviceIoControlFile(
                     Socket->HContext.Handle,
                     tlsData->EventHandle,
                     NULL,
                     0,
                     &ioStatusBlock,
                     AfdIoctl,
                     NULL,
                     0,
                     Buffer,
                     BufferLength
                     );
    }

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            tlsData->EventHandle,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    //
    // If requested, set up the length of returned buffer.
    //

    if ( ARGUMENT_PRESENT(OutBufferLength) ) {
        *OutBufferLength = (INT)ioStatusBlock.Information;
    }

    return NO_ERROR;

} // SockGetConnectData


#ifdef _PNP_POWER_
int
WSPAPI
DoAsyncIoctl (
    PSOCKET_INFORMATION socket,
    HANDLE              transportHandle,
    DWORD               pollEvent,
    DWORD               dwIoControlCode,
    LPVOID              lpvInBuffer,
    DWORD               cbInBuffer,
    LPVOID              lpvOutBuffer,
    DWORD               cbOutBuffer,
    LPDWORD             lpcbBytesReturned,
    LPWSAOVERLAPPED     lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    LPINT               lpErrno
    ) 
{
    NTSTATUS status;
    IO_STATUS_BLOCK localIoStatusBlock;
    PIO_STATUS_BLOCK ioStatusBlock;
    AFD_TRANSPORT_IOCTL_INFO   ioctlInfo;
    int err = 0;
    HANDLE event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID apcContext;
	PWINSOCK_TLS_DATA	tlsData;
	tlsData = GET_THREAD_DATA ();

    ioctlInfo.AfdFlags = 0;
    ioctlInfo.Handle = transportHandle;
    ioctlInfo.PollEvent = pollEvent;
    ioctlInfo.IoControlCode = dwIoControlCode;
    ioctlInfo.InputBuffer = lpvInBuffer;
    ioctlInfo.InputBufferLength = cbInBuffer;

    ASSERT (transportHandle==NULL);
//
// This is currently not used by helper dlls.
// Commented out because of security concerns
//
#if 0
    if (transportHandle!=NULL) {
        //
        // If we are passing IOCTL to the transport,
        // use special ioctl code with the method specified
        // in the transport IOCTL.
        //
        dwIoControlCode = IOCTL_AFD_TRANSPORT_IOCTL | (dwIoControlCode & 3);
    }
#endif
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

        //
        // Let afd know that this is non-overlapped request so it can 
        // handle it correctly on non-blocking sockets 
        //

        ioStatusBlock->Status = STATUS_UNSUCCESSFUL;

    } else {
        ioctlInfo.AfdFlags |= AFD_OVERLAPPED;

        if( lpCompletionRoutine == NULL ) {

            //
            // No APC, use event object from OVERLAPPED structure.
            //

            event = lpOverlapped->hEvent;

            apcRoutine = NULL;
            apcContext = ( (ULONG_PTR)lpOverlapped->hEvent & 1 ) ? NULL : lpOverlapped;

        } else {

            //
            // APC, ignore event object.
            //

            event = NULL;

            apcRoutine = SockIoCompletion;
            apcContext = lpCompletionRoutine;


        }

        //
        // Use part of the OVERLAPPED structure as our IO_STATUS_BLOCK.
        //

        ioStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

        //
        // Let afd know that this is overlapped request so it can 
        // handle it correctly on non-blocking sockets 
        //

        ioStatusBlock->Status = STATUS_PENDING;

    }

    //
    // Execute IOCTL.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
                 event,
                 apcRoutine,
                 apcContext,
                 ioStatusBlock,
                 dwIoControlCode,
                 &ioctlInfo,
                 sizeof (ioctlInfo),
                 lpvOutBuffer,
                 cbOutBuffer
                 );

    //
    // If this request has no overlapped structure, then wait for
    // the operation to complete.
    //

    if ( status == STATUS_PENDING &&
         lpOverlapped == NULL ) {

        BOOL success;

        SockReleaseSocketLock( socket );

        success = SockWaitForSingleObject(
                      event,
                      socket->Handle,
                      SOCK_NEVER_CALL_BLOCKING_HOOK,
                      SOCK_NO_TIMEOUT
                      );

        SockAcquireSocketLockExclusive( socket );

        //
        // If the wait completed successfully, look in the IO status
        // block to determine the real status code of the request.  If
        // the wait timed out, then cancel the IO and set up for an
        // error return.
        //

        if ( success ) {

            status = ioStatusBlock->Status;

        } else {

            SockCancelIo( socket->Handle );
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
        *lpcbBytesReturned = (DWORD)ioStatusBlock->Information;
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
    if ( err != NO_ERROR ) {

        *lpErrno = err;
        return SOCKET_ERROR;

    }

    return 0;
}

#endif _PNP_POWER_

