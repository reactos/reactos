/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    wspmisc.c

Abstract:

    This module contains support for the following WinSock APIs;

        WSPCancelBlockingCall()
        WSPDuplicateSocket()
        WSPGetOverlappedResult()

Author:

    David Treadwell (davidtr)    15-May-1992

Revision History:

--*/

#include "winsockp.h"


int
WSPAPI
WSPCancelBlockingCall(
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This function cancels any outstanding blocking operation for this
    task.  It is normally used in two situations:

        (1) An application is processing a message which has been
          received while a blocking call is in progress.  In this case,
          WSAIsBlocking() will be true.

        (2) A blocking call is in progress, and Windows Sockets has
          called back to the application's "blocking hook" function (as
          established by WSASetBlockingHook()).

    In each case, the original blocking call will terminate as soon as
    possible with the error WSAEINTR.  (In (1), the termination will not
    take place until Windows message scheduling has caused control to
    revert to the blocking routine in Windows Sockets.  In (2), the
    blocking call will be terminated as soon as the blocking hook
    function completes.)

    In the case of a blocking connect() operation, the Windows Sockets
    implementation will terminate the blocking call as soon as possible,
    but it may not be possible for the socket resources to be released
    until the connection has completed (and then been reset) or timed
    out.  This is likely to be noticeable only if the application
    immediately tries to open a new socket (if no sockets are
    available), or to connect() to the same peer.

Arguments:

    None.

Return Value:

    The value returned by WSACancelBlockingCall() is 0 if the operation
    was successfully canceled.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number is availalbe in lpErrno.

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;

    WS_ENTER( "WSPCancelBlockingCall", NULL, NULL, NULL, NULL );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPCancelBlockingCall", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    //
    // This call is only valid when we are in a blocking call.
    //
	// This is now checked by ws2_32.dll, no point in
	// duplicating this check here

    //if ( !SockThreadIsBlocking ) {

    //    WS_EXIT( "WSPCancelBlockingCall", SOCKET_ERROR, TRUE );
    //    *lpErrno = WSAEINVAL;
    //    return SOCKET_ERROR;

    //}

    //
    // Note that because we disable the blocking hook callback below,
    // the IO should not have already been cancelled.
    //

    WS_ASSERT( !tlsData->IoCancelled );
    WS_ASSERT( tlsData->SocketHandle != INVALID_SOCKET );

    //
    // Cancel all the IO initiated in this thread for the socket handle
    // we're blocking on.
    //

    IF_DEBUG(CANCEL) {

        WS_PRINT(( "WSPCancelBlockingCall: cancelling IO on socket handle %lx\n",
                       tlsData->SocketHandle ));

    }

    SockCancelIo( tlsData->SocketHandle );

    //
    // Remember that we've cancelled the IO that we're blocking on.
    // This prevents the blocking hook from being called any more.
    //

    tlsData->IoCancelled = TRUE;


    WS_EXIT( "WSPCancelBlockingCall", NO_ERROR, FALSE );
    return NO_ERROR;

}   // WSPCancelBlockingCall



int
WSPAPI
WSPDuplicateSocket (
    SOCKET Handle,
    DWORD ProcessId,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPINT lpErrno
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;
    PSOCKET_INFORMATION socket;
    HANDLE processHandle;
    HANDLE dupedHandle;

    WS_ENTER( "WSPDuplicateSocket", (PVOID)Handle, (PVOID)ProcessId, lpProtocolInfo, lpErrno );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPDuplicateSocket", SOCKET_ERROR, TRUE );
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

    if( lpProtocolInfo == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Open the target process.
    //

    processHandle = OpenProcess(
                        PROCESS_DUP_HANDLE,         // fdwAccess
                        FALSE,                      // fInherit
                        ProcessId                   // IDProcess
                        );

    if( processHandle == NULL ) {

        err = WSAEINVAL;

        goto exit;

    }

    //
    // Duplicate the handle into the target process.
    //

    if( !DuplicateHandle(
            GetCurrentProcess(),                    // hSourceProcessHandle
            (HANDLE)Handle,                         // hSourceHandle
            processHandle,                          // hTargetProcessHandle
            &dupedHandle,                           // lpTargetHandle
            0,                                      // dwDesiredAccess
            TRUE,                                   // bInheritHandle
            DUPLICATE_SAME_ACCESS                   // dwOptions
            ) ) {

        err = WSAEMFILE;
        CloseHandle( processHandle );
        goto exit;

    }

    err = SockBuildProtocolInfoForSocket(
        socket,
        lpProtocolInfo
        );

    if (err!=NO_ERROR) {
        CloseHandle (processHandle);
        CloseHandle (dupedHandle);
        goto exit;
    }

    //
    // Success!
    //

    __try {   
        //
        // This cast is safe as handle is guaranteed to take only 
        // first 32 bits.
        //

        lpProtocolInfo->dwProviderReserved = HandleToUlong (dupedHandle);
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
        CloseHandle (processHandle);
        CloseHandle (dupedHandle);
        goto exit;
    }

    CloseHandle( processHandle );
    err = NO_ERROR;

exit:

    IF_DEBUG(RECEIVE) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPDuplicateSocket on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, err ));

        } else {

            WS_PRINT(( "WSPDuplicateSocket on socket %lx (%lx) succeeded\n",
                           Handle, socket ));

        }

    }

    if ( socket != NULL ) {

        SockDereferenceSocket( socket );

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPDuplicateSocket", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return SOCKET_ERROR;

    }

    WS_EXIT( "WSPDuplicateSocket", 0, FALSE );
    return 0;

}   // WSPDuplicateSocket


BOOL
WSPAPI
WSPGetOverlappedResult (
    SOCKET Handle,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags,
    LPINT lpErrno
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;

    WS_ENTER( "WSPGetOverlappedResult", (PVOID)Handle, lpOverlapped, lpcbTransfer, (PVOID)fWait );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPGetOverlappedResult", SOCKET_ERROR, TRUE );
        *lpErrno = err;
        return FALSE;

    }

    __try {
        //
        // Let KERNEL32.DLL do the hard part (GetOverlappedResult returns FALSE
        // if NT_SUCCESS(lpOverlapped->Internal) is FALSE)
        //

        if( GetOverlappedResult(
                (HANDLE)Handle,
                lpOverlapped,
                lpcbTransfer,
                fWait
                ) ) {

            //
            // Determine what flags to return.
            //

            switch( lpOverlapped->Internal ) {

            case STATUS_RECEIVE_PARTIAL :
                *lpdwFlags = MSG_PARTIAL;
                break;

            case STATUS_RECEIVE_EXPEDITED :
                *lpdwFlags = MSG_OOB;
                break;

            case STATUS_RECEIVE_PARTIAL_EXPEDITED :
                *lpdwFlags = MSG_PARTIAL | MSG_OOB;
                break;

            default :
                *lpdwFlags = 0;
                break;
            }

            //
            // Success!
            //

            WS_ASSERT( err == NO_ERROR );

            IF_DEBUG(RECEIVE) {
                WS_PRINT(( "WSPGetOverlappedResult on socket %lx succeeded\n",
                               Handle ));
            }

            WS_EXIT( "WSPGetOverlappedResult", TRUE, FALSE );
            *lpErrno = NO_ERROR;
            return TRUE;
        }
        else {

            if (lpOverlapped->Internal == STATUS_CANCELLED) {

                err = WSA_OPERATION_ABORTED;

            } else if  (!fWait && (GetLastError ()==ERROR_IO_INCOMPLETE)) {

                err = WSA_IO_INCOMPLETE;

            } else {

                err = SockNtStatusToSocketError((NTSTATUS)lpOverlapped->Internal);
            }
        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        err = WSAEFAULT;
    }

    IF_DEBUG(RECEIVE) {
        WS_PRINT(( "WSPGetOverlappedResult on socket %lx failed: %ld.\n",
                       Handle, err ));
    }
    WS_EXIT( "WSPGetOverlappedResult", FALSE, TRUE );
    *lpErrno = err;
    return FALSE;
}   // WSPGetOverlappedResult


BOOL
WSPAPI
WSPGetQOSByName (
    SOCKET Handle,
    LPWSABUF lpQOSName,
    LPQOS lpQOS,
    LPINT lpErrno
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

	PWINSOCK_TLS_DATA	tlsData;
    int err;
    DWORD bytesReturned;

    WS_ENTER( "WSPGetQOSByName", (PVOID)Handle, lpQOSName, lpQOS, lpErrno );

    WS_ASSERT( lpErrno != NULL );

    err = SockEnterApi( &tlsData );

    if( err != NO_ERROR ) {

        WS_EXIT( "WSPGetQOSByName", FALSE, TRUE );
        *lpErrno = err;
        return FALSE;

    }

    //
    // We'll take the totally cheesy way out and just return the
    // default QOS structure associated with the incoming socket.
    //

    if( WSPIoctl(
            Handle,
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

        WS_ASSERT( err != NO_ERROR );
        goto exit;

    }

    WS_ASSERT( err == NO_ERROR );

exit:

    IF_DEBUG(RECEIVE) {

        if ( err != NO_ERROR ) {

            WS_PRINT(( "WSPGetQOSByName on socket %lx failed: %ld.\n",
                           Handle, err ));

        } else {

            WS_PRINT(( "WSPGetQOSByName on socket %lx succeeded\n",
                           Handle ));

        }

    }

    if ( err != NO_ERROR ) {

        WS_EXIT( "WSPGetQOSByName", FALSE, TRUE );
        *lpErrno = err;
        return FALSE;

    }

    WS_EXIT( "WSPGetQOSByName", TRUE, FALSE );
    return TRUE;

}   // WSPGetQOSByName
