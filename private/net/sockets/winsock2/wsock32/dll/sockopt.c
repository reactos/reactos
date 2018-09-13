/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Sockopt.c

Abstract:

    This module contains support for the getsockopt( ) and setsockopt( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    31-Mar-1992

Revision History:

--*/

#define WINSOCK_API_LINKAGE
#define getsockopt getsockopt_v11
#define setsockopt setsockopt_v11

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//
// The versions of WSOCK32.DLL that shiped with NT 3.1, NT 3.5, NT 3.51,
// TCP/IP-32 for WFW, and Win95 all use the "Steve Deering" values for the
// IP Multicast options. Unfortunately, the TCP/IP subgroup of the Windows
// Sockets 2.0 standards effort chose to use the BSD values for these options.
// Since these values overlap considerably, we have a rather unfortunate
// situation.
//
// Here's how we'll deal with this.
//
// Applications built using WINSOCK2.H & WS2TCPIP.H will use the BSD
// values as #defined in WS2TCPIP.H. These applications will link with
// WS2_32.DLL, and life is swell.
//
// Applications built using WINSOCK.H will use the Steve Deering values
// as #defined in WINSOCK.H. These applications will link with WSOCK32.DLL,
// which will map these options to the BSD values before passing them
// down to WS2_32.DLL. Life is still swell.
//
// These are the "old" Steve Deering values that must be mapped:
//

#define OLD_IP_MULTICAST_IF     2
#define OLD_IP_MULTICAST_TTL    3
#define OLD_IP_MULTICAST_LOOP   4
#define OLD_IP_ADD_MEMBERSHIP   5
#define OLD_IP_DROP_MEMBERSHIP  6
#define OLD_IP_TTL              7
#define OLD_IP_TOS              8
#define OLD_IP_DONTFRAGMENT     9

#define TCP_BSDURGENT           0x7000

INT
MapOldIpMulticastOptionToBsdValue(
    INT OptionName
    );


int PASCAL
getsockopt(
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    char *OptionValue,
    int *OptionLength
    )

/*++

Routine Description:

    getsockopt() retrieves the current value for a socket option
    associated with a socket of any type, in any state, and stores the
    result in optval.  Options may exist at multiple protocol levels,
    but they are always present at the uppermost "socket'' level.
    Options affect socket operations, such as whether an operation
    blocks or not, the routing of packets, out-of-band data transfer,
    etc.

    The value associated with the selected option is returned in the
    buffer optval.  The integer pointed to by optlen should originally
    contain the size of this buffer; on return, it will be set to the
    size of the value returned.  For SO_LINGER, this will be the size of
    a struct linger; for all other options it will be the size of an
    integer.

    If the option was never set with setsockopt(), then getsockopt()
    returns the default value for the option.

    The following options are supported for
    getsockopt().  The Type identifies the type of
    data addressed by optval.

         Value         Type     Meaning

         SO_ACCEPTCONN BOOL     Socket is listen()ing.

         SO_BROADCAST  BOOL     Socket is configured for the transmission
                                of broadcast messages.

         SO_DEBUG      BOOL     Debugging is enabled.

         SO_DONTLINGER BOOL     If true, the SO_LINGER option is disabled.

         SO_DONTROUTE  BOOL     Routing is disabled.

         SO_ERROR      int      Retrieve error status and clear.

         SO_KEEPALIVE  BOOL     Keepalives are being sent.

         SO_LINGER     struct   Returns the current linger
                       linger   options.
                       FAR *

         SO_OOBINLINE  BOOL     Out-of-band data is being received in the
                                normal data stream.

         SO_RCVBUF     int      Buffer size for receives

         SO_REUSEADDR  BOOL     The socket may be bound to an address which
                                is already in use.

         SO_SNDBUF     int      Buffer size for sends

         SO_TYPE       int      The type of the socket (e.g. SOCK_STREAM).

Arguments:

    s - A descriptor identifying a socket.

    level - The level at which the option is defined; the only supported
        level is SOL_SOCKET.

    optname - The socket option for which the value is to be retrieved.

    optval - A pointer to the buffer in which the value for the
        requested option is to be returned.

    optlen - A pointer to the size of the optval buffer.

Return Value:

    If no error occurs, getsockopt() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    ULONG error;
#undef getsockopt
    extern int WSAAPI getsockopt( SOCKET s, int level, int optname,
                                        char FAR * optval, int FAR * optlen );

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;

    //
    // Map the old IP multicast values to their BSD equivilants.
    //

    if( Level == IPPROTO_IP ) {

        OptionName = MapOldIpMulticastOptionToBsdValue( OptionName );

    }

    //
    // Handle TCP_BSDURGENT specially.
    //

    if( Level == IPPROTO_TCP && OptionName == TCP_BSDURGENT ) {

        if( getsockopt(
                Handle,
                Level,
                TCP_EXPEDITED_1122,
                OptionValue,
                OptionLength
                ) == SOCKET_ERROR ) {

            return SOCKET_ERROR;

        }

        //
        // TCP_BSDURGENT is the inverse of TCP_EXPEDITED_1122.
        //

        *OptionValue = !(*OptionValue);
        goto exit;

    }

    //
    // Forward it to the "real" WS2_32.DLL.
    //

    if( getsockopt(
            Handle,
            Level,
            OptionName,
            OptionValue,
            OptionLength
            ) == SOCKET_ERROR ) {

        return SOCKET_ERROR;

    }

exit:

    if ( error != NO_ERROR ) {
        SetLastError( error );
        return SOCKET_ERROR;
    }

    return NO_ERROR;

} // getsockopt


int PASCAL
setsockopt(
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    IN const char *OptionValue,
    IN int OptionLength
    )

/*++

Routine Description:

    setsockopt() sets the current value for a socket option associated
    with a socket of any type, in any state.  Although options may exist
    at multiple protocol levels, this specification only defines options
    that exist at the uppermost "socket'' level.  Options affect socket
    operations, such as whether expedited data is received in the normal
    data stream, whether broadcast messages may be sent on the socket,
    etc.

    There are two types of socket options: Boolean options that enable
    or disable a feature or behavior, and options which require an
    integer value or structure.  To enable a Boolean option, optval
    points to a nonzero integer.  To disable the option optval points to
    an integer equal to zero.  optlen should be equal to sizeof(int) for
    Boolean options.  For other options, optval points to the an integer
    or structure that contains the desired value for the option, and
    optlen is the length of the integer or structure.

    SO_LINGER controls the action taken when unsent data is queued on a
    socket and a closesocket() is performed.  See closesocket() for a
    description of the way in which the SO_LINGER settings affect the
    semantics of closesocket().  The application sets the desired
    behavior by creating a struct linger (pointed to by the optval
    argument) with the following elements:

        struct linger {
             int  l_onoff;
             int  l_linger;
        }

    To enable SO_LINGER, the application should set l_onoff to a
    non-zero value, set l_linger to 0 or the desired timeout (in
    seconds), and call setsockopt().  To enable SO_DONTLINGER (i.e.
    disable SO_LINGER) l_onoff should be set to zero and setsockopt()
    should be called.

    By default, a socket may not be bound (see bind()) to a local
    address which is already in use.  On occasions, however, it may be
    desirable to "re- use" an address in this way.  Since every
    connection is uniquely identified by the combination of local and
    remote addresses, there is no problem with having two sockets bound
    to the same local address as long as the remote addresses are
    different.  To inform the Windows Sockets implementation that a
    bind() on a socket should not be disallowed because of address
    re-use, the application should set the SO_REUSEADDR socket option
    for the socket before issuing the bind().  Note that the option is
    interpreted only at the time of the bind(): it is therefore
    unnecessary (but harmless) to set the option on a socket which is
    not to be bound to an existing address, and setting or resetting the
    option after the bind() has no effect on this or any other socket..

    An application may request that the Windows Sockets implementation
    enable the use of "keep- alive" packets on TCP connections by
    turning on the SO_KEEPALIVE socket option.  A Windows Sockets
    implementation need not support the use of keep- alives: if it does,
    the precise semantics are implementation-specific but should conform
    to section 4.2.3.6 of RFC 1122: Requirements for Internet Hosts --
    Communication Layers.  If a connection is dropped as the result of
    "keep- alives" the error code WSAENETRESET is returned to any calls
    in progress on the socket, and any subsequent calls will fail with
    WSAENOTCONN.

    The following options are supported for setsockopt().  The Type
    identifies the type of data addressed by optval.

         Value         Type     Meaning

         SO_ACCEPTCONN BOOL     Socket is listen()ing.

         SO_BROADCAST  BOOL     Socket is configured for the transmission
                                of broadcast messages.

         SO_DEBUG      BOOL     Debugging is enabled.

         SO_DONTLINGER BOOL     If true, the SO_LINGER option is disabled.

         SO_DONTROUTE  BOOL     Routing is disabled.

         SO_ERROR      int      Retrieve error status and clear.

         SO_KEEPALIVE  BOOL     Keepalives are being sent.

         SO_LINGER     struct   Returns the current linger
                       linger   options.
                       FAR *

         SO_OOBINLINE  BOOL     Out-of-band data is being received in the
                                normal data stream.

         SO_RCVBUF     int      Buffer size for receives

         SO_REUSEADDR  BOOL     The socket may be bound to an address which
                                is already in use.

         SO_SNDBUF     int      Buffer size for sends

         SO_TYPE       int      The type of the socket (e.g. SOCK_STREAM).

Arguments:

Return Value:

    If no error occurs, setsockopt() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    ULONG error;
    INT optionValue;
    INT invertedValue;
    char FAR * valuePointer;

#undef setsockopt
    extern int WSAAPI setsockopt( SOCKET s, int level, int optname,
                                        const char FAR * optval, int optlen );

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;

    //
    // Map the old IP multicast values to their BSD equivilants.
    //

    if( Level == IPPROTO_IP ) {

        OptionName = MapOldIpMulticastOptionToBsdValue( OptionName );

    }

    //
    // Handle TCP_BSDURGENT specially.
    //

    valuePointer = (char FAR *)OptionValue;

    if( Level == IPPROTO_TCP && OptionName == TCP_BSDURGENT ) {

        OptionName = TCP_EXPEDITED_1122;

        if( OptionLength >= sizeof(INT) ) {

            invertedValue = !(*OptionValue);
            valuePointer = (char FAR *)&invertedValue;
            OptionLength = sizeof(invertedValue);

        }

    }

    return setsockopt(
               Handle,
               Level,
               OptionName,
               valuePointer,
               OptionLength
               );

} // setsockopt


INT
MapOldIpMulticastOptionToBsdValue(
    INT OptionName
    )
{

    switch( OptionName ) {

    case OLD_IP_MULTICAST_IF :
        OptionName = IP_MULTICAST_IF;
        break;

    case OLD_IP_MULTICAST_TTL :
        OptionName = IP_MULTICAST_TTL;
        break;

    case OLD_IP_MULTICAST_LOOP :
        OptionName = IP_MULTICAST_LOOP;
        break;

    case OLD_IP_ADD_MEMBERSHIP :
        OptionName = IP_ADD_MEMBERSHIP;
        break;

    case OLD_IP_DROP_MEMBERSHIP :
        OptionName = IP_DROP_MEMBERSHIP;
        break;

    case OLD_IP_TTL :
        OptionName = IP_TTL;
        break;

    case OLD_IP_TOS :
        OptionName = IP_TOS;
        break;

    case OLD_IP_DONTFRAGMENT :
        OptionName = IP_DONTFRAGMENT;
        break;
    }

    return OptionName;

}   // MapOldIpMulticastOptionToBsdValue


int WSAAPI
recv(
     IN SOCKET s,
     OUT char FAR * buf,
     IN int len,
     IN int flags
     )
/*++
Routine Description:

    Receive data from a socket.

Arguments:

    s     - A descriptor identifying a connected socket.

    buf   - A buffer for the incoming data.

    len   - The length of buf.

    flags - Specifies the way in which the call is made.

Returns:

    The  number  of  bytes  received.   If  the  connection has been gracefully
    closed,  the  return  value  is  0.   Otherwise, a value of SOCKET_ERROR is
    returned, and a specific error code is stored with SetErrorCode().

--*/

{
    INT     ReturnValue;
    WSABUF  Buffers;
    DWORD   LocalFlags;
    INT     ErrorCode;

    Buffers.len = len;
    Buffers.buf = buf;
    LocalFlags = (DWORD) flags;

    ErrorCode = WSARecv(s,
            &Buffers,
            1, // Buffer count
            (LPDWORD)&ReturnValue,
            &LocalFlags,
            NULL,
            NULL);
    if (SOCKET_ERROR == ErrorCode) {
        ReturnValue = SOCKET_ERROR;
    } else if (LocalFlags & MSG_PARTIAL) {

        // If the receive was a partial message (won't happen on a
        // streams transport like TCP) set the last error to
        // WSAEMSGSIZE and negate ths number of bytes received.
        // This allows the app to know that the receive was partial
        // and also how many bytes were received.
        //

        ReturnValue *= -1;
        SetLastError (WSAEMSGSIZE);
    }

    return(ReturnValue);
}


int WSAAPI
recvfrom(
    IN SOCKET s,
    OUT char FAR * buf,
    IN int len,
    IN int flags,
    OUT struct sockaddr FAR *from,
    IN OUT int FAR * fromlen
    )
/*++
Routine Description:

    Receive a datagram and store the source address.

Arguments:

    s       - A descriptor identifying a bound socket.

    buf     - A buffer for the incoming data.

    len     - The length of buf.

    flags   - Specifies the way in which the call is made.

    from    - An  optional  pointer  to  a  buffer  which  will hold the source
              address upon return.

    fromlen - An optional pointer to the size of the from buffer.

Returns:

    The  number  of  bytes  received.   If  the  connection has been gracefully
    closed,  the  return  value  is  0.   Otherwise, a value of SOCKET_ERROR is
    returned, and a specific error code is stored with SetErrorCode().

--*/

{
    INT     ReturnValue;
    WSABUF  Buffers;
    DWORD   LocalFlags;
    INT     ErrorCode;

    Buffers.len = len;
    Buffers.buf = buf;
    LocalFlags = (DWORD) flags;

    ErrorCode = WSARecvFrom(s,
                &Buffers,
                1,
                (LPDWORD)&ReturnValue,
                &LocalFlags,
                from,
                fromlen,
                NULL,
                NULL);

    if (SOCKET_ERROR == ErrorCode) {
        ReturnValue = SOCKET_ERROR;
    } else if (LocalFlags & MSG_PARTIAL) {

        // If the receive was a partial message (won't happen on a
        // streams transport like TCP) set the last error to
        // WSAEMSGSIZE and negate ths number of bytes received.
        // This allows the app to know that the receive was partial
        // and also how many bytes were received.
        //

        ReturnValue *= -1;
        SetLastError (WSAEMSGSIZE);
    }

    return ReturnValue;
}
