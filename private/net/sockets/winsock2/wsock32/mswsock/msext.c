/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    msext.c

Abstract:

    This module contains Microsoft WinSock extension APIs.

Author:

    Keith Moore (keithmo)        01-Jul-1996

Revision History:
    Vadim Eydelman (vadime)      Jan 1999
        Reimplemented to account for multiple providers and 
        dynamic provider loading/unloading

--*/


#include "winsockp.h"



//
// Private globals.
//

// GUIDs of MS extensions.
const GUID SockTransmitFileGuid = WSAID_TRANSMITFILE;
const GUID SockAcceptExGuid = WSAID_ACCEPTEX;
const GUID SockGetAcceptExSockaddrsGuid = WSAID_GETACCEPTEXSOCKADDRS;

//
// Globals for AcceptEx and GetAcceptExSockaddrs
// for the first (or the only) provider.
//
LPFN_ACCEPTEX SockAcceptExPointer=NULL;
LPFN_GETACCEPTEXSOCKADDRS SockGetAcceptExSockaddrsPointer=NULL;


//
// Hash table to resolve buffer address to provider entry point
// address if there is more than one provider.
//
#define DEFAULT_BUFFER_KEY_TABLE_SIZE   2047
UINT_PTR    *SockBufferKeyTable;
UINT_PTR    SockBufferKeyTableSize = DEFAULT_BUFFER_KEY_TABLE_SIZE;
INT         SockBufferKeyTableDoubleHashMax = 4;

//
// The following constants are chosen based on the assumption that
// the buffer for AcceptEx should at least be 16 bytes long
//
#define SOCK_ACCEPTEX_BUFFER_MASK   (((UINT_PTR)-1)<<4)
#define SOCK_ACCEPTEX_PVDIDX_MASK   (~(SOCK_ACCEPTEX_BUFFER_MASK))
#define SOCK_MAX_ACCEPTEX_PROVIDERS (1<<4)

#define SOCK_PVDIDX_FROM_KEY(_k)    ((_k)&SOCK_ACCEPTEX_PVDIDX_MASK)
#define SOCK_MAKE_BUFFER_KEY(_b,_i) \
            ((((UINT_PTR)(_b))&SOCK_ACCEPTEX_BUFFER_MASK)+(_i))
#define SOCK_SAME_BUFFERS(_k1,_k2) \
            ((((UINT_PTR)(_k1))&SOCK_ACCEPTEX_BUFFER_MASK)==\
                (((UINT_PTR)(_k2))&SOCK_ACCEPTEX_BUFFER_MASK))

//
// Addresses of the provider's entry points.
//
LPFN_GETACCEPTEXSOCKADDRS SockGSAEAFuncTable[SOCK_MAX_ACCEPTEX_PROVIDERS];

//
// Private prototypes.
//

BOOL
SockRememberAcceptExBuffer (
    PVOID                       Buffer,
    LPFN_GETACCEPTEXSOCKADDRS   FuncAddress
    );

LPFN_GETACCEPTEXSOCKADDRS
SockGetGAESAFuncAddress (
    PVOID   Buffer
    );

BOOL
SockInitializeBufferKeyTable (
    VOID
    );
//
// Public functions.
//


BOOL
TransmitFile (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nNumberOfBytesToWrite,
    IN DWORD nNumberOfBytesPerSend,
    IN LPOVERLAPPED lpOverlapped,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Transmits file data over a connected socket handle by interfacing
    with the cache manager to retrieve the file data.  Use this routine
    when high performance file data transfer over sockets is required.

Arguments:

    hSocket - a connected socket handle.  The socket may be a datagram
        socket or a virtual circuit socket.

    hFile - an open file handle.  Since the file data is read
        sequentially, it is recommended that the handle be opened with
        FILE_FLAG_SEQUENTIAL_SCAN to improve caching performance.

    nNumberOfBytesToWrite - the count of bytes to send from the file.
        This request will not complete until the entire requested amount
        has been sent, or until an error is encountered.  If this
        parameter is equal to zero, then the entire file is transmited.

    nNumberOfBytesPerSend - informs the sockets layer of the size to use
        for each send it performs.  The value 0 indicates that an
        intelligent default should be used.  This is mainly useful for
        datagram or message protocols that have limitations of the size
        of individual send requests.

    lpOverlapped - specify this parameter when the socket handle is
        opened as overlapped to achieve overlapped I/O.  Note that by
        default sockets are opened as overlapped.

        Also use this parameter to specify an offset within the file at
        which to start the file data transfer.  The file pointer is
        ignored by this API, so if lpOverlapped is NULL the transmission
        always starts at offset 0 in the file.

        When lpOverlapped is non-NULL, TransmitFile() may return
        ERROR_IO_PENDING to allow the calling function to continue
        processing while the operation completes.  The event (or hSocket
        if hEvent is NULL) will be set to the signalled state upon
        completion of the request.

    lpTransmitBuffers - an optional pointer to a TRANSMIT_FILE_BUFFERS
        structure which contains pointers to data to send before the
        file (the head buffer) and after the file (the tail buffer).  If
        only the file is to be sent, this parameter may be specified as
        NULL.

    dwFlags - any combination of the following:

        TF_DISCONNECT - start a transport-level disconnect after all the
            file data has been queued to the transport.

        TF_REUSE_SOCKET - prepare the socket handle to be reused.  When
            the TransmitFile request completes, the socket handle may be
            passed to the SuperAccept API.  Only valid if TF_DISCONNECT
            is also specified.

        TF_WRITE_BEHIND - complete the TransmitFile request immediately,
            without pending.  If this flag is specified and TransmitFile
            succeeds, then the data has been accepted by the system but not
            necessarily acknowledged by the remote end.  Do not use this
            setting with the other two settings.

Return Value:

    TRUE - The operation was successul.

    FALSE - The operation failed.  Extended error status is available
        using GetLastError().

--*/

{
    INT res;
    DWORD   returned;
    LPFN_TRANSMITFILE sockTransmitFilePointer;

    //
    // Query the entry point every time since different providers
    // have different entry points
    //

    res = WSAIoctl(
                 hSocket,
                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                 (LPVOID)&SockTransmitFileGuid,
                 sizeof(SockTransmitFileGuid),
                 (LPVOID)&sockTransmitFilePointer,
                 sizeof(sockTransmitFilePointer),
                 &returned,
                 NULL,
                 NULL
                 );
    if( res == SOCKET_ERROR ) {
        return FALSE;
    }

    //
    // Let the service provider do the dirty work.
    //

    WS_ASSERT( sockTransmitFilePointer != NULL );

    return sockTransmitFilePointer(
               hSocket,
               hFile,
               nNumberOfBytesToWrite,
               nNumberOfBytesPerSend,
               lpOverlapped,
               lpTransmitBuffers,
               dwFlags
               );

}   // TransmitFile



BOOL
AcceptEx (
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    Combines several socket functions into a single API/kernel
    transition.  A new connection is accepted, both the local and remote
    addresses for the connection are returned, and a receive is done for
    the first chunk of data sent by the remote.  By combining all these
    functions into a sigle call, the performance of connection
    acceptance is significantly improved.

    Unlike the normal winsock accept() call, AcceptEx() is
    asynchronous, which lets it fit better with servers which use thread
    pooling to improve scalability.  As with all overlapped Win32
    functions, either Win32 events or completion ports may be used
    as a completion notification mechanism.

    Another key difference between this function and the normal accept()
    function is that this function requires the caller to specify
    both the listening socket AND the socket on which to accept the
    connection.  The sAcceptSocket must be an opened socket which
    is neither bound nor connected.  Note that calling TransmitFile()
    with both the TF_DISCONNECT and TF_REUSE_SOCKET flags will result
    in the specified socket being returned to the "open" state, so
    such a socket may be passed to AcceptEx() as sAcceptSocket.

    Note that only a single output buffer receives the data as well as
    the local and remote socket addresses.  Use of a single buffer
    improves performance, but the caller should call th
    GetAcceptExSockaddrs() function to locate the addresses in the
    output buffer.

    Because the addresses are written in an internal format, the caller
    must specify sisteen more bytes for them than the size of the
    SOCKADDR structure for the transport protocol in use.  For example,
    th size of a SOCKADDR_IN, the address structure for TCP/IP, is 16
    bytes, so at least 24 bytes must be specified as the buffer sizes
    for the local and remote addresses.

    The lpNumberOfBytesTransferred parameters of the
    GetQueuedCompletionStatus() and GetOverlappedResult() APIs indicates
    the number of bytes received in the request.

    When this operation completes successfully, sAcceptHandle may
    be used for only the following routines:

        ReadFile
        WriteFile
        send
        recv
        TransmitFile
        closesocket

    In order to use it with othr winsock routines, a setsockopt() with
    the SO_UPDATE_ACCEPT_CONTEXT option must be performed on the socket.
    This socket option initializes some user-mode state on the socket
    which allows other winsock routines to access the socket correctly.

    sAcceptSocket will not inherit the properties of sListenSocket until
    SO_UPDATE_ACCEPT_CONTEXT is set on the socket.  When AcceptEx()
    returns, sAcceptSocket is in the default state for a connected
    socket.

    To use the SO_UPDATE_ACCEPT_CONTEXT option, call the setsockopt()
    function, specifying sAcceptSocket as the socket handle to setsockopt()
    and specify sListenSocket as the option value.  For example,

        err = setsockopt( sAcceptSocket,
                          SOL_SOCKET,
                          SO_UPDATE_ACCEPT_CONTEXT,
                          (char *)&sListenSocket,
                          sizeof(sListenSocket) );

Arguments:

    sListenSocket - a listening socket on which to accept a connection.
        The listen() socket API must have been previously called on this
        handle.

    sAcceptSocket - an open socket handle on which to accept an incoming
        connection.  This socket must not be bound or connected.

    lpOutputBuffer - a pointer to a buffer which receives the first
        chunk of data sent on the connection and the local and remote
        addresses for the new connection.  The receive data is written
        to the first part of the buffer (starting at offset 0) and the
        addresses are written later in the buffer.  This parameter must
        be specified.

    dwReceiveDataLength - the number of bytes in the buffer that should
        be used for receiving data.  If this parameter is specified as
        0, then no receive operation is performed in conjunction with
        the accept--the accept completes as soon as a connection arrives
        without waiting for any data to arrive.

    dwLocalAddressLength - the number of bytes reserved for the local
        address information.  This must be at least sixteen bytes more
        than the maximum sockaddr length for the transport protocol in
        use.

    dwRemoteAddressLength - the number of bytes reserved for the remote
        address information.  This must be at least sixteen bytes more
        than the maximum sockaddr length for the transport protocol in
        use.

    lpdwBytesReceived - points to a DWORD which receives the count of
        bytes received.  This is only set if the operation completes
        synchronously; if it returns ERROR_IO_PENDING and completes
        later, then this DWORD is never set and the count of bytes read
        may be obtained from the completion notification mechnaism.

    lpOverlapped - an OVERLAPPED structure to use in processing the request.
        This parameter MUST be specified; it may not be NULL.


Return Value:

    TRUE if the operation completed successfully.  FALSE if there was an
    error, in which case GetLastError() may be called to return extended
    error information.  If GetLastError() returns ERROR_IO_PENDING then
    the operation was successfully initiated and is still in progress.

--*/

{

    INT     res;
    DWORD   returned;
    LPFN_ACCEPTEX sockAcceptExPointer;
    LPFN_GETACCEPTEXSOCKADDRS sockGetAcceptExSockaddrsPointer;


    //
    // Query the entry point every time since different providers
    // have different entry points
    //
    res = WSAIoctl(
                 sListenSocket,
                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                 (LPVOID)&SockAcceptExGuid,
                 sizeof(SockAcceptExGuid),
                 (LPVOID)&sockAcceptExPointer,
                 sizeof(sockAcceptExPointer),
                 &returned,
                 NULL,
                 NULL
                 );
    if( res == SOCKET_ERROR ) {
        return FALSE;
    }

    WS_ASSERT( sockAcceptExPointer != NULL );

    //
    // If entry point is different that what we had before,
    // requery GetAcceptExSockaddrs too since it does not
    // have socket parameter and we do not know who to call.
    //
    if (sockAcceptExPointer != SockAcceptExPointer) {

        //
        // New provider, get corresponding GetAcceptExSockaddrs
        //

        res = WSAIoctl(
                     sListenSocket,
                     SIO_GET_EXTENSION_FUNCTION_POINTER,
                     (LPVOID)&SockGetAcceptExSockaddrsGuid,
                     sizeof(SockGetAcceptExSockaddrsGuid),
                     (LPVOID)&sockGetAcceptExSockaddrsPointer,
                     sizeof(sockGetAcceptExSockaddrsPointer),
                     &returned,
                     NULL,
                     NULL
                     );
        if (res==SOCKET_ERROR) {
            return FALSE;
        }

        ASSERT (sockGetAcceptExSockaddrsPointer!=NULL);

        if (SockAcceptExPointer==NULL) {
            //
            // If we never initialized the pointers, do it now
            //
            SockAcquireGlobalLockExclusive ();
            if (SockAcceptExPointer==NULL) {
                SockGetAcceptExSockaddrsPointer = sockGetAcceptExSockaddrsPointer;
                SockAcceptExPointer = sockAcceptExPointer;
                SockReleaseGlobalLock();
            }
            else if (SockAcceptExPointer==sockAcceptExPointer) {
                ASSERT (SockGetAcceptExSockaddrsPointer==sockGetAcceptExSockaddrsPointer);
                SockReleaseGlobalLock();
            }
            else {
                goto NewAcceptEx;
            }
        }
        else {
        NewAcceptEx:
            //
            // New provider, we'll have to remember the buffer
            // to find corresponding GetAcceptExSockaddrs pointer
            // when presented with it.
            //
            if (!SockRememberAcceptExBuffer (
                    lpOutputBuffer,
                    sockGetAcceptExSockaddrsPointer)) {
                return FALSE;
            }
        }
    }
            
    ASSERT (SockGetAcceptExSockaddrsPointer!=NULL);

    //
    // Let the service provider do the dirty work.
    //


    return sockAcceptExPointer(
               sListenSocket,
               sAcceptSocket,
               lpOutputBuffer,
               dwReceiveDataLength,
               dwLocalAddressLength,
               dwRemoteAddressLength,
               lpdwBytesReceived,
               lpOverlapped
               );


}   // AcceptEx


VOID
GetAcceptExSockaddrs (
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPSOCKADDR *LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT LPSOCKADDR *RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    )

/*++

Routine Description:

    Processes the lpOutputBuffer parameter after a successful AcceptEx()
    operation.  Because AcceptEx() writes address information in an
    internal (TDI) format, this routine is required to locate the
    SOCKADDR structures in the buffer.

Arguments:

    lpOutputBuffer - the same lpOutputBuffer parameter that was passed
        to AcceptEx().

    dwReceiveDataLength - must be equal to the dwReceiveDataLength
        parameter which was passed to AcceptEx().

    dwLocalAddressLength - must be equal to the dwLocalAddressLength
        parameter which was passed to AcceptEx().

    dwRemoteAddressLength - must be equal to the dwRemoteAddressLength
        parameter which was passed to AcceptEx().

    LocalSockaddr - receives a pointer to the SOCKADDR which describes
        the local address of the connection (the same information as
        would be returned by getsockname()).  This parameter must be
        specified.

    LocalSockaddrLength - receives the size of the local address.  This
        parameter must be specified.

    RemoteSockaddr - receives a pointer to the SOCKADDR which describes
        the remote address of the connection (the same information as
        would be returned by getpeername()).  This parameter must be
        specified.

    RemoteSockaddrLength - receives the size of the local address.  This
        parameter must be specified.

Return Value:

    None.

--*/

{

    LPFN_GETACCEPTEXSOCKADDRS sockGetAcceptExSockaddrsPointer;

    //
    // Since GetAcceptExSockaddrs() doesn't take a socket parameter, we
    // cannot initialize the extension table here. This is OK, as it
    // would be totally lame to call this API before calling AcceptEx().
    //
    // Also, since GetAcceptExSockaddrs() has no return value, we cannot
    // directly indicate an error to the caller, so we'll just NULL out
    // everything passed in and let the app explode.
    //

    if( SockGetAcceptExSockaddrsPointer==NULL ) {

        LocalSockaddr = NULL;
        LocalSockaddrLength = 0;
        RemoteSockaddr = NULL;
        RemoteSockaddrLength = 0;

        return;

    }

    if (SockBufferKeyTable==NULL) {
        //
        // We only have one provider in the process, pick 
        // function address from the global.
        //
        sockGetAcceptExSockaddrsPointer = SockGetAcceptExSockaddrsPointer;
    }
    else {
        //
        // More then one provider, get it from the hash table.
        //
        sockGetAcceptExSockaddrsPointer = 
            SockGetGAESAFuncAddress (lpOutputBuffer);
    }

    //
    // Let the service provider do the dirty work.
    //

    sockGetAcceptExSockaddrsPointer(
        lpOutputBuffer,
        dwReceiveDataLength,
        dwLocalAddressLength,
        dwRemoteAddressLength,
        LocalSockaddr,
        LocalSockaddrLength,
        RemoteSockaddr,
        RemoteSockaddrLength
        );

}   // GetAcceptExSockaddrs


BOOL
SockRememberAcceptExBuffer (
    PVOID                       Buffer,
    LPFN_GETACCEPTEXSOCKADDRS   FuncAddress
    )
/*
    Stores the address of GetAcceptExSockaddrs entry point
    that corresponds to buffer passed in AcceptEx
*/
{
    UINT_PTR    idx,key,oldKey;
    INT         i,j;

    if (SockBufferKeyTable==NULL) {
        if (!SockInitializeBufferKeyTable ()) {
            return FALSE;
        }
        ASSERT (SockBufferKeyTable!=NULL);
    }

    for (i=0;i<SOCK_MAX_ACCEPTEX_PROVIDERS;i++) {
        if (SockGSAEAFuncTable[i]==FuncAddress) {
            break;
        }
        else if (SockGSAEAFuncTable[i]==NULL) {
            SockGSAEAFuncTable[i] = FuncAddress;
            break;
        }
    }
    if (i>=SOCK_MAX_ACCEPTEX_PROVIDERS) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    idx = ((UINT_PTR)Buffer)%SockBufferKeyTableSize;
    key = SOCK_MAKE_BUFFER_KEY(Buffer,i);
    //
    // We use double hashing with quadratic probing algorithm
    //
    for (i=0,j=1; i<SockBufferKeyTableDoubleHashMax; i++,j+=2) {
        do {
            oldKey = (volatile UINT_PTR)SockBufferKeyTable[idx];
            if ((oldKey==0) || 
                    SOCK_SAME_BUFFERS(oldKey,Buffer)) { 
                if (InterlockedCompareExchangePointer (
                            (PVOID *)&SockBufferKeyTable[idx],
                            (PVOID)key,
                            (PVOID)oldKey)==(PVOID)oldKey) {
                    return TRUE;
                }
            }
            else
                break;
        }
        while (1);

        idx = (idx+j)%SockBufferKeyTableSize;
    }

    //
    // Store the key anyway, application must not be calling 
    // GetAcceptExSockaddrs.
    //

    SockBufferKeyTable[((UINT_PTR)Buffer)%SockBufferKeyTableSize] = key;
    return TRUE;
}

LPFN_GETACCEPTEXSOCKADDRS
SockGetGAESAFuncAddress (
    PVOID   Buffer
    )
/*
    Get address of the GetAcceptExSockaddrs entry point
    based on the AcceptEx buffer address
*/
{
    UINT_PTR    idx,key;
    INT         i,j;
    LPFN_GETACCEPTEXSOCKADDRS funcAddr;

    idx = ((UINT_PTR)Buffer)%SockBufferKeyTableSize;
    //
    // We use double hashing with quadratic probing algorithm
    //
    for (i=0,j=1; i<SockBufferKeyTableDoubleHashMax; i++,j+=2) {
        do {
            key = (volatile UINT_PTR)SockBufferKeyTable[idx];
            if (SOCK_SAME_BUFFERS(key,Buffer)) { 
                if (InterlockedCompareExchangePointer (
                            (PVOID *)&SockBufferKeyTable[idx],
                            (PVOID)0,
                            (PVOID)key)==(PVOID)key) {
                    funcAddr = SockGSAEAFuncTable[SOCK_PVDIDX_FROM_KEY(key)];
                    if (funcAddr!=NULL) {
                        return funcAddr;
                    }
                    goto DefaultExit;
                }
            }
            else
                break;
        }
        while (1);

        idx = (idx+j)%SockBufferKeyTableSize;
    }

DefaultExit:
    return SockGetAcceptExSockaddrsPointer;
}


BOOL
SockInitializeBufferKeyTable (
    VOID
    )
/*
    Initializes hash table that stores information about
    providers based on the buffer address passed in AcceptEx
*/
{
    HKEY    hKey;
    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Services\\Winsock\\Parameters"),
            0,
            KEY_QUERY_VALUE,
            &hKey
            ) == ERROR_SUCCESS) {
        DWORD   dwDataSize, dwDataType;
        ULONG   tableSize;
        dwDataSize = sizeof (tableSize);
        if (RegQueryValueEx(
            hKey,
            TEXT("AcceptExBufferTableSize"),
            0,
            &dwDataType,
            (LPBYTE) &tableSize,
            &dwDataSize
            )==ERROR_SUCCESS) {

            if (tableSize<DEFAULT_BUFFER_KEY_TABLE_SIZE) {
                tableSize = DEFAULT_BUFFER_KEY_TABLE_SIZE;
            }
            SockBufferKeyTableSize = tableSize;
        }
        RegCloseKey (hKey);
    }

    SockBufferKeyTable = GlobalAlloc (
                            GPTR,
                            sizeof (SockBufferKeyTable[0])*SockBufferKeyTableSize);
    if (SockBufferKeyTable!=NULL) {
        ZeroMemory (SockBufferKeyTable,
                    sizeof (SockBufferKeyTable[0])*SockBufferKeyTableSize);
        return TRUE;
    }
    else {
        return FALSE;
    }
}
