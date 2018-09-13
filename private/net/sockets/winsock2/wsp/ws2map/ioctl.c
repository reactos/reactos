*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module contains ioctl routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPIoctl()

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
WSPIoctl(
    IN SOCKET s,
    IN DWORD dwIoControlCode,
    IN LPVOID lpvInBuffer,
    IN DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno
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

    PSOCKET_INFORMATION socketInfo;
    INT err;
    INT result;

    SOCK_ENTER( "WSPIoctl", (PVOID)s, (PVOID)dwIoControlCode, lpvInBuffer, (PVOID)cbInBuffer );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPIoctl", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Attempt to find the socket in our lookup table.
    //

    socketInfo = SockFindAndReferenceWS2Socket( s );

    if( socketInfo == NULL || socketInfo->State == SocketStateClosing ) {

        IF_DEBUG(IOCTL) {

            SOCK_PRINT((
                "WSPIoctl failed on %s handle: %lx\n",
                socketInfo == NULL ? "unknown" : "closed",
                s
                ));

        }

        if( socketInfo != NULL ) {

            SockDereferenceSocket( socketInfo );

        }

        *lpErrno = WSAENOTSOCK;
        SOCK_EXIT( "WSPIoctl", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Validate the parameters.
    //

    if( lpcbBytesReturned == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Special case the new 2.x ioctls.
    //

    switch( dwIoControlCode ) {

    case SIO_GET_BROADCAST_ADDRESS : {

        LPSOCKADDR_IN addr;

        if( lpvOutBuffer == NULL || cbOutBuffer < sizeof(*addr) ) {

            err = WSAEFAULT;
            goto exit;

        }

        addr = (LPSOCKADDR_IN)lpvOutBuffer;

        ZeroMemory(
            addr,
            sizeof(*addr)
            );

        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = INADDR_BROADCAST;

        *lpcbBytesReturned = sizeof(*addr);
        err = NO_ERROR;
        goto exit;

        }
        break;

    case SIO_GET_EXTENSION_FUNCTION_POINTER :

        err = WSAEINVAL;
        goto exit;

        break;

    case SIO_GET_QOS :
    case SIO_GET_GROUP_QOS : {

        LPQOS qos;

        if( lpvOutBuffer == NULL || cbOutBuffer < sizeof(*qos) ) {

            err = WSAEFAULT;
            goto exit;

        }

        qos = (LPQOS)lpvOutBuffer;

        CopyMemory(
            qos,
            &SockDefaultQos,
            sizeof(*qos)
            );

        *lpcbBytesReturned = sizeof(*qos);
        err = NO_ERROR;
        goto exit;

        }
        break;

    case SIO_SET_QOS :
    case SIO_SET_GROUP_QOS : {

        LPQOS qos;

        if( lpvInBuffer == NULL || cbInBuffer < sizeof(*qos) ) {

            err = WSAEFAULT;
            goto exit;

        }

        qos = (LPQOS)lpvInBuffer;

        if( memcmp( qos, &SockDefaultQos, sizeof(*qos) ) != 0 ) {

            //
            // CODEWORK: SIGNAL FD_QOS OR FD_GROUP_QOS AS APPROPRIATE!
            //

        }

        *lpcbBytesReturned = 0;
        err = NO_ERROR;
        goto exit;

        }
        break;

    }

    if( lpvInBuffer != lpvOutBuffer ||
        cbInBuffer != cbOutBuffer ) {

        err = WSAENOPROTOOPT;
        goto exit;

    }

    //
    // Let the hooker do its thang.
    //

    SockPreApiCallout();

    result = socketInfo->Hooker->ioctlsocket(
                 socketInfo->WS1Handle,
                 (long)dwIoControlCode,
                 (u_long FAR *)lpvInBuffer
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
    SOCK_ASSERT( result == NO_ERROR );

    *lpcbBytesReturned = sizeof(u_long);

exit:

    if( err == NO_ERROR ) {

        //
        // The IOCTL succeeded. We may need to do some completion magic
        // if this is an overlapped request on an overlapped socket.
        //

        SockCompleteRequest(
            socketInfo,
            NO_ERROR,
            *lpcbBytesReturned,
            lpOverlapped,
            lpCompletionRoutine,
            lpThreadId
            );

    }

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SockDereferenceSocket( socketInfo );

    SOCK_EXIT( "WSPIoctl", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPIoctl

