/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    bind.c

Abstract:

    This module contains support for the bind( ) WinSock API.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "winsockp.h"


int
WSPAPI
WSPBind(
    IN SOCKET Handle,
    IN const struct sockaddr *SocketAddress,
    IN int SocketAddressLength,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine is used on an unconnected datagram or stream socket,
    before subsequent connect()s or listen()s.  When a socket is created
    with socket(), it exists in a name space (address family), but it
    has no name assigned.  bind() establishes the local association
    (host address/port number) of the socket by assigning a local name
    to an unnamed socket.

    If an application does not care what address is assigned to it, it
    may specify an Internet address and port equal to 0.  If this is the
    case, the Windows Sockets implementation will assign a unique
    address to the application.  The application may use getsockname()
    after bind() to learn the address that has been assigned to it.

    In the Internet address family, a name consists of several
    components.  For SOCK_DGRAM and SOCK_STREAM, the name consists of
    three parts: a host address, the protocol number (set implicitly to
    UDP or TCP, respectively), and a port number which identifies the
    application. The port is ignored for SOCK_RAW.

Arguments:

    s - A descriptor identifying an unbound socket.

    name - The address to assign to the socket.  The sockaddr structure
       is defined as follows:

              struct sockaddr {
                   u_short   sa_family;
                   char sa_data[14];
              };

    namelen - The length of the name.

Return Value:

--*/

{
    NTSTATUS status;
	PWINSOCK_TLS_DATA	tlsData;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    int err;
    SOCKADDR_INFO sockaddrInfo;
    UCHAR localBuffer[MAX_FAST_TDI_ADDRESS
                +FIELD_OFFSET(TDI_ADDRESS_INFO, Address)];
    PAFD_BIND_INFO  bindInfo;
    PTDI_ADDRESS_INFO tdiAddressInfo;
    ULONG bindInfoLength;
    ULONG tdiAddressInfoLength;


    WS_ENTER( "WSPBind", (PVOID)Handle, (PVOID)SocketAddress, (PVOID)SocketAddressLength, lpErrno );

    WS_ASSERT( lpErrno != NULL );

    IF_DEBUG(BIND) {

        WS_PRINT(( "WSPBind() on socket %lx addr %lx addrlen %ld\n"
                   "    req",
                       Handle, SocketAddress, SocketAddressLength ));

        WsPrintSockaddr( (PSOCKADDR)SocketAddress, &SocketAddressLength );

    }

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPBind", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // Set up local variables so that we know how to clean up on exit.
    //

    socket = NULL;
    tdiAddressInfo = NULL;
    bindInfo = NULL;

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
    // Acquire the lock that protects sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're binding.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If the socket has been initialized at all then this is not a
    // legal request.
    //

    if ( socket->State != SocketStateOpen ) {

        err = WSAEINVAL;
        goto exit;

    }

    //
    // If the buffer passed in is larger than needed for a socket address,
    // just truncate it.  Otherwise, the TDI address buffer we allocate
    // may not be large enough.
    //

    if ( SocketAddressLength > socket->HelperDll->MaxSockaddrLength ) {

        SocketAddressLength = socket->HelperDll->MaxSockaddrLength;

    }

    //
    // Make sure that the address structure passed in is legitimate,
    // and determine the type of address we're binding to.
    //

    err = socket->HelperDll->WSHGetSockaddrType(
                (PSOCKADDR)SocketAddress,
                SocketAddressLength,
                &sockaddrInfo
                );

    if ( err != NO_ERROR) {

        goto exit;

    }

    //
    // Hack?  If this is a wildcard address, and the caller set the
    // super secret "don't use wildcard" option, put non zero information
    // in the reserved field so that UDP knows that we really want to
    // bind to the zero address (0.0.0.0).
    //

    if ( socket->DontUseWildcard &&
         sockaddrInfo.AddressInfo == SockaddrAddressInfoWildcard ) {

        *(UNALIGNED ULONG *)(SocketAddress->sa_data + 6) = 0x12345678;

    }

    // !!! test for reserved port?

    //
    // Allocate enough space to hold the TDI address structure we'll pass
    // to AFD and get back in return
    //

    // This is used for input
    bindInfoLength =  FIELD_OFFSET(AFD_BIND_INFO, Address)
                        + socket->HelperDll->MaxTdiAddressLength;

    // This is used for output
    tdiAddressInfoLength = FIELD_OFFSET (TDI_ADDRESS_INFO,Address)
                            + socket->HelperDll->MaxTdiAddressLength;


    if( tdiAddressInfoLength <= sizeof(localBuffer) &&
        bindInfoLength <= sizeof(localBuffer)) {

        tdiAddressInfo = (PVOID)localBuffer;
        bindInfo = (PVOID)localBuffer;

    } else {

        tdiAddressInfo = ALLOCATE_HEAP( max(tdiAddressInfoLength,
                                            bindInfoLength));

        if ( tdiAddressInfo == NULL ) {

            err = WSAENOBUFS;
            goto exit;

        }

        bindInfo = (PVOID)tdiAddressInfo;

    }

    if (socket->ExclusiveAddressUse) {

        //
        // Ask for exclusive access if application wants it
        //
        bindInfo->ShareAccess = AFD_EXCLUSIVEADDRUSE;

    }
    else if ( sockaddrInfo.EndpointInfo == SockaddrEndpointInfoWildcard) {

        //
        // Wildcard address, no checking as it is not allocated yet
        //

        bindInfo->ShareAccess = AFD_WILDCARDADDRESS;
    }
    else if (socket->ReuseAddresses ) {
        
        //
        // Application asks to reuse someones (or its own) address
        //
        bindInfo->ShareAccess = AFD_REUSEADDRESS;

    } else {
        //
        // Normal case, ask for unique predefined address.
        //
        bindInfo->ShareAccess = AFD_NORMALADDRUSE;
    }


    //
    // Convert the address from the sockaddr structure to the appropriate
    // TDI structure.
    //

    err = SockBuildTdiAddress(
        &bindInfo->Address,
        (PSOCKADDR)SocketAddress,
        SocketAddressLength
        );

    if (err!=NO_ERROR) {

        goto exit;

    }

    //
    // Call AFD to perform the actual bind operation.  AFD will open a
    // TDI address object through the proper TDI provider for this
    // socket.
    //

    status = NtDeviceIoControlFile(
                 socket->HContext.Handle,
                 tlsData->EventHandle,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_BIND,
                 bindInfo,
                 bindInfoLength,
                 tdiAddressInfo,
                 tdiAddressInfoLength
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

    socket->TdiAddressHandle = (HANDLE)ioStatusBlock.Information;
#if DBG
    {
        DWORD flags;
        WS_ASSERT (socket->TdiAddressHandle!=INVALID_HANDLE_VALUE && // (NtCurrentProcess==(HANDLE)-1)
                GetHandleInformation( socket->TdiAddressHandle, &flags ));
    }
#endif

    //
    // Notify the helper DLL that the socket is now bound.
    //

    err = SockNotifyHelperDll( socket, WSH_NOTIFY_BIND );

    if ( err != NO_ERROR ) {

        goto exit;

    }


    err = SockBuildSockaddr(
        socket->LocalAddress,
        &SocketAddressLength,
        &tdiAddressInfo->Address
        );

    if ( err != NO_ERROR ) {

        goto exit;

    }

#if DBG
    if ( (
        socket->ExclusiveAddressUse || 
                (sockaddrInfo.EndpointInfo != SockaddrEndpointInfoWildcard &&
                     !socket->ReuseAddresses)) &&
          (tdiAddressInfo->ActivityCount > 1) ) {
        WS_PRINT (("WSPBind: Asked for unique address but tdi address info"
                    " activity count is %ld for ",
                    tdiAddressInfo->ActivityCount));
        WsPrintSockaddr( socket->LocalAddress, &SocketAddressLength );
    }
#endif

    //
    // Indicate that the socket is now bound to a specific address;
    //

    socket->State = SocketStateBound;

    //
    // Remember the changed state of this socket.
    //

    err = SockSetHandleContext( socket );

    if ( err != NO_ERROR ) {

        goto exit;

    }

    //
    // If the application changed the send or receive buffers and this
    // is a datagram socket, tell AFD about the new buffer sizes.
    //

    err = SockUpdateWindowSizes( socket, FALSE );

    if ( err != NO_ERROR ) {

        goto exit;

    }

exit:

    IF_DEBUG(BIND) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPBind on socket %lx (%lx) failed: %ld\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "    WSPBind on socket %lx (%lx), granted",
                           Handle, socket ));
            WsPrintSockaddr( socket->LocalAddress, &socket->LocalAddressLength );

        }

    }

    //
    // Perform cleanup--dereference the socket if it was referenced,
    // free allocated resources.
    //

    if ( socket != NULL ) {

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );

    }

    WS_ASSERT ((PVOID)tdiAddressInfo==(PVOID)bindInfo);
    if ( tdiAddressInfo != NULL && tdiAddressInfo != (PVOID)localBuffer ) {
        FREE_HEAP( tdiAddressInfo );

    }

    //
    // Return an error if appropriate.
    //

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPBind", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPBind", NO_ERROR, FALSE );
    return NO_ERROR;

} // WSPBind

