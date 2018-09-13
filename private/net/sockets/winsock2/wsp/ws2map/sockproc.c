/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sockproc.c

Abstract:

    This module contains socket management code for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockFindAndReferenceWS2Socket()
        SockFindAndReferenceWS1Socket()
        SockReferenceSocket()
        SockDereferenceSocket()
        SockCreateSocket()
        SockPrepareForBlockingHook()
        SockPreApiCallout()
        SockPostApiCallout()
        SockBuildProtocolInfoForSocket()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private prototypes.
//

INT
WINAPI
SockBlockingHook(
    VOID
    );


//
// Public functions.
//


PSOCKET_INFORMATION
SockFindAndReferenceWS2Socket(
    IN SOCKET Socket
    )

/*++

Routine Description:

    This routine maps an "API visible" (i.e. WinSock 2) handle to the
    corresponding SOCKET_INFORMATION structure.

Arguments:

    Socket - The WinSock 2 socket handle to map.

Return Value:

    PSOCKET_INFORMATION - A pointer to the sockets SOCKET_INFORMATION
        structure if successful, NULL otherwise.

--*/

{

    INT result;
    INT err;
    PSOCKET_INFORMATION socketInfo;

    //
    // Acquire the lock that protects the socket list.
    //

    SockAcquireGlobalLock();

    try {

        //
        // Try to get the context from WS2_32.DLL.
        //

        result = SockUpcallTable.lpWPUQuerySocketHandleContext(
                     Socket,
                     (PULONG_PTR)&socketInfo,
                     &err
                     );

        if( result != SOCKET_ERROR ) {

            SOCK_ASSERT( socketInfo->ReferenceCount > 0 );
            socketInfo->ReferenceCount++;

        } else {

            socketInfo = NULL;

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        socketInfo = NULL;

    }

    //
    // Release the global lock before returning.
    //

    SockReleaseGlobalLock();

    return socketInfo;

}   // SockFindAndReferenceWS2Socket



PSOCKET_INFORMATION
SockFindAndReferenceWS1Socket(
    IN SOCKET Socket
    )

/*++

Routine Description:

    This routine maps a "downlevel" (i.e. WinSock 1) handle to the
    corresponding SOCKET_INFORMATION structure.

Arguments:

    Socket - The WinSock 1 socket handle to map.

Return Value:

    PSOCKET_INFORMATION - A pointer to the sockets SOCKET_INFORMATION
        structure if successful, NULL otherwise.

--*/

{

    PLIST_ENTRY listEntry;
    PSOCKET_INFORMATION socketScan;
    PSOCKET_INFORMATION socketInfo = NULL;

    //
    // Acquire the lock that protects the socket list.
    //

    SockAcquireGlobalLock();

    try {

        //
        // We cannot use the WS2_32.DLL upcalls to map the WinSock 1 handle,
        // so we'll need to scan the global linked list.
        //

        for( listEntry = SockGlobalSocketListHead.Flink ;
             listEntry != &SockGlobalSocketListHead ;
             listEntry = listEntry->Flink ) {

            socketScan = CONTAINING_RECORD(
                             listEntry,
                             SOCKET_INFORMATION,
                             SocketListEntry
                             );

            if( socketScan->WS1Handle == Socket ) {

                socketInfo = socketScan;

                SOCK_ASSERT( socketInfo->ReferenceCount > 0 );
                socketInfo->ReferenceCount++;

                break;

            }

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        socketInfo = NULL;

    }

    //
    // Release the global lock before returning.
    //

    SockReleaseGlobalLock();

    return socketInfo;

}   // SockFindAndReferenceWS1Socket



VOID
SockReferenceSocket(
    IN PSOCKET_INFORMATION SocketInfo
    )

/*++

Routine Description:

    References the specified socket.

Arguments:

    SocketInfo - The socket to reference.

--*/

{

    INT err;

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( SocketInfo->ReferenceCount > 0 );

    //
    // Acquire the lock that protects the socket list.
    //

    SockAcquireGlobalLock();

    try {

        SocketInfo->ReferenceCount++;

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    //
    // Release the global lock before returning.
    //

    SockReleaseGlobalLock();

}   // SockReferenceSocket



VOID
SockDereferenceSocket(
    IN PSOCKET_INFORMATION SocketInfo
    )

/*++

Routine Description:

    Dereferences the specified socket. If the reference count drops to
    zero, removes the socket from the global list and frees its resources.

Arguments:

    SocketInfo - The socket to dereference.

--*/

{

    INT err;

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( SocketInfo->ReferenceCount > 0 );

    //
    // Acquire the lock that protects the socket list.
    //

    SockAcquireGlobalLock();

    try {

        SocketInfo->ReferenceCount--;

        if( SocketInfo->ReferenceCount == 0 ) {

            //
            // Free the socket's resources.
            //

            RemoveEntryList( &SocketInfo->SocketListEntry );
            DeleteCriticalSection( &SocketInfo->SocketLock );

            if( SocketInfo->Hooker != NULL ) {

                SockDereferenceHooker( SocketInfo->Hooker );

            }

            if( SocketInfo->DeferSocket != INVALID_SOCKET ) {

                WSPCloseSocket(
                    SocketInfo->DeferSocket,
                    &err
                    );

            }

            if( SocketInfo->GroupID != 0 ) {

                SockDereferenceGroup( SocketInfo->GroupID );

            }

            if( SocketInfo->ShutdownEvent != NULL ) {

                CloseHandle( SocketInfo->ShutdownEvent );

            }

            if( SocketInfo->EventSelectEvent != NULL ) {

                CloseHandle( SocketInfo->EventSelectEvent );

            }

            if( SocketInfo->OverlappedRecv.WakeupEvent != NULL ) {

                CloseHandle( SocketInfo->OverlappedRecv.WakeupEvent );

            }

            SOCK_ASSERT( IsListEmpty( &SocketInfo->OverlappedRecv.QueueListHead ) );

            if( SocketInfo->OverlappedSend.WakeupEvent != NULL ) {

                CloseHandle( SocketInfo->OverlappedSend.WakeupEvent );

            }

            SOCK_ASSERT( IsListEmpty( &SocketInfo->OverlappedSend.QueueListHead ) );

            SOCK_FREE_HEAP( SocketInfo );

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    //
    // Release the global lock before returning.
    //

    SockReleaseGlobalLock();

}   // SockDereferenceSocket



PSOCKET_INFORMATION
SockCreateSocket(
    IN PHOOKER_INFORMATION HookerInfo,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol,
    IN INT MaxSockaddrLength,
    IN INT MinSockaddrLength,
    IN DWORD Flags,
    IN DWORD CatalogEntryId,
    IN DWORD ServiceFlags1,
    IN DWORD ServiceFlags2,
    IN DWORD ServiceFlags3,
    IN DWORD ServiceFlags4,
    IN DWORD ProviderFlags,
    IN GROUP Group,
    IN SOCK_GROUP_TYPE GroupType,
    IN SOCKET WS1Handle,
    OUT LPINT Error
    )

/*++

Routine Description:

    Creates a new socket.

Arguments:

    HookerInfo - The hooker that will manage the new socket.

    AddressFamily - The new socket's address family.

    SocketType - The new socket's type.

    Protocol - The new socket's protocol.

    Flags - Snapshot of the creation flags passed (WSA_FLAG_*).

    CatalogEntryId - The protocol's catalog entry ID.

    WS1Handle - The Winsock 1.1 handle for this socket.

    Error - Will receive any error code.

Return Value:

    PSOCKET_INFORMATION - Pointer to the newly created socket if
        successful, NULL if not. If NULL, then an error code may be
        found in *Error.

--*/

{

    PSOCKET_INFORMATION socketInfo;
    SOCKET ws2Handle;
    INT err;

    //
    // Sanity check.
    //

    SOCK_ASSERT( HookerInfo != NULL );
    SOCK_ASSERT( WS1Handle != INVALID_SOCKET );
    SOCK_ASSERT( Error != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    err = NO_ERROR;
    socketInfo = NULL;
    ws2Handle = INVALID_SOCKET;

    //
    // Grab the global lock. We must hold this until the socket is
    // created and put on the global list.
    //

    SockAcquireGlobalLock();

    try {

        //
        // Create a new socket structure.
        //

        socketInfo = SOCK_ALLOCATE_HEAP(
                         sizeof(*socketInfo) + MaxSockaddrLength
                         );

        if( socketInfo == NULL ) {

            err = WSAENOBUFS;
            leave;

        }

        //
        // Initialize it.
        //

        ZeroMemory(
            socketInfo,
            sizeof(*socketInfo) + MaxSockaddrLength
            );

        socketInfo->ReferenceCount = 2;
        socketInfo->WS1Handle = WS1Handle;
        socketInfo->State = SocketStateOpen;

        socketInfo->AddressFamily = AddressFamily;
        socketInfo->SocketType = SocketType;
        socketInfo->Protocol = Protocol;

        InitializeCriticalSection( &socketInfo->SocketLock );

        socketInfo->OverlappedRecv.WakeupEvent = NULL;
        InitializeListHead( &socketInfo->OverlappedRecv.QueueListHead );

        socketInfo->OverlappedSend.WakeupEvent = NULL;
        InitializeListHead( &socketInfo->OverlappedSend.QueueListHead );

        socketInfo->Hooker = HookerInfo;

        socketInfo->MaxSockaddrLength = MaxSockaddrLength;
        socketInfo->MinSockaddrLength = MinSockaddrLength;
        socketInfo->CreationFlags = Flags;
        socketInfo->CatalogEntryId = CatalogEntryId;
        socketInfo->ServiceFlags1 = ServiceFlags1;
        socketInfo->ServiceFlags2 = ServiceFlags2;
        socketInfo->ServiceFlags3 = ServiceFlags3;
        socketInfo->ServiceFlags4 = ServiceFlags4;
        socketInfo->ProviderFlags = ProviderFlags;

        socketInfo->AsyncSelectIssuedToHooker = FALSE;
        socketInfo->SelectEventsEnabled = 0;
        socketInfo->SelectEventsActive = 0;
        socketInfo->AsyncSelectWindow = NULL;
        socketInfo->AsyncSelectMessage = 0;
        socketInfo->EventSelectEvent = NULL;

        socketInfo->GroupID = Group;
        socketInfo->GroupType = GroupType;
        socketInfo->GroupPriority = 0;

        socketInfo->DeferSocket = INVALID_SOCKET;

        //
        // Create the shutdown event.
        //

        socketInfo->ShutdownEvent = CreateEvent(
                                        NULL,
                                        FALSE,
                                        FALSE,
                                        NULL
                                        );

        if( socketInfo->ShutdownEvent == NULL ) {

            err = WSAENOBUFS;
            leave;

        }

        //
        // Create the WS2 socket.
        //

        ws2Handle = SockUpcallTable.lpWPUCreateSocketHandle(
                        CatalogEntryId,
                        (ULONG_PTR)socketInfo,
                        &err
                        );

        if( ws2Handle == INVALID_SOCKET ) {

            SOCK_ASSERT( err != NO_ERROR );
            leave;

        }

        //
        // Finish initialization and put it on the global list.
        //

        socketInfo->WS2Handle = ws2Handle;

        InsertHeadList(
            &SockGlobalSocketListHead,
            &socketInfo->SocketListEntry
            );

        SOCK_ASSERT( err == NO_ERROR );

    } except( SOCK_EXCEPTION_FILTER() ) {

        err = GetExceptionCode();

    }

    try {

        if( err == NO_ERROR ) {

            IF_DEBUG(SOCKET) {

                SOCK_PRINT((
                    "Created socket %lx:%lx (%08lx) from hooker %08lx\n",
                    ws2Handle,
                    WS1Handle,
                    socketInfo,
                    HookerInfo
                    ));

            }

        } else {

            if( ws2Handle != INVALID_SOCKET ) {

                INT dummy;

                SockUpcallTable.lpWPUCloseSocketHandle(
                    ws2Handle,
                    &dummy
                    );

            }

            if( socketInfo->ShutdownEvent != NULL ) {

                CloseHandle( socketInfo->ShutdownEvent );

            }

            if( socketInfo != NULL ) {

                SOCK_FREE_HEAP( socketInfo );
                socketInfo = NULL;

            }

            *Error = err;

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        socketInfo = NULL;

    }

    SockReleaseGlobalLock();

    return socketInfo;

}   // SockCreateSocket



VOID
SockPrepareForBlockingHook(
    IN PSOCKET_INFORMATION SocketInfo
    )

/*++

Routine Description:

    Prepares a hooker for possible blocking hook use. This routine is
    called before any hooker entrypoint that may invoke the blocking
    hook.

Arguments:

    SocketInfo - The target socket.

Return Value:

    None.

--*/

{

    INT err;
    INT result;
    FARPROC result2;
    PSOCK_TLS_DATA tlsData;

    SOCK_ASSERT( SocketInfo != NULL );

    tlsData = SOCK_GET_THREAD_DATA();
    SOCK_ASSERT( tlsData != NULL );

    //
    // Query the blocking callback for this thread.
    //

    result = SockUpcallTable.lpWPUQueryBlockingCallback(
                 SocketInfo->CatalogEntryId,
                 &tlsData->BlockingCallback,
                 &tlsData->BlockingContext,
                 &err
                 );

    if( result == SOCKET_ERROR ) {

        SOCK_PRINT((
            "SockPrepareForBlockingHook: cannot query blocking callback: %d\n",
            err
            ));

        //
        // Assume there is no callback.
        //

        tlsData->BlockingCallback = NULL;
        tlsData->BlockingContext = 0;

    }

    //
    // If there's been a change in state (meaning we have not previously
    // set a blocking hook for this thread and now we need one, OR we
    // have previously set a blocking hook for this thread and now we don't
    // need one) the send the update request to the hooker.
    //

    if( tlsData->BlockingHookInstalled &&
        tlsData->BlockingCallback == NULL ) {

        //
        // Need to unhook the blocking hook.
        //

        SockPreApiCallout();

        result = SocketInfo->Hooker->WSAUnhookBlockingHook();

        if( result == SOCKET_ERROR ) {

            err = SocketInfo->Hooker->WSAGetLastError();
            SOCK_ASSERT( err != NO_ERROR );
            SockPostApiCallout();

            SOCK_PRINT((
                "SockPrepareForBlockingCallback: cannot unhook blocking hook: %d\n",
                err
                ));

        } else {

            SockPostApiCallout();
            tlsData->BlockingHookInstalled = FALSE;
            tlsData->BlockingSocketInfo = NULL;

        }

    }
    else
    if( !tlsData->BlockingHookInstalled &&
        tlsData->BlockingCallback != NULL ) {

        //
        // Need to set the blocking hook
        //

        SockPreApiCallout();

        result2 = SocketInfo->Hooker->WSASetBlockingHook(
                      (FARPROC)SockBlockingHook
                      );

        if( result2 == NULL ) {

            err = SocketInfo->Hooker->WSAGetLastError();
            SOCK_ASSERT( err != NO_ERROR );
            SockPostApiCallout();

            SOCK_PRINT((
                "SockPrepareForBlockingCallback: cannot set blocking hook: %d\n",
                err
                ));

        } else {

            SockPostApiCallout();
            tlsData->BlockingHookInstalled = TRUE;
            tlsData->BlockingSocketInfo = SocketInfo;

        }

    }

}   // SockPrepareForBlockingHook



VOID
SockPreApiCallout(
    VOID
    )

/*++

Routine Description:

    Prepares the current thread for a callout to a hooker entrypoint. This
    routine is called before every hooker entrypoint. Currently, the only
    thing done here is to set the per-thread reentrancy flag.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PSOCK_TLS_DATA tlsData;

    tlsData = SOCK_GET_THREAD_DATA();
    SOCK_ASSERT( tlsData != NULL );

    SOCK_ASSERT( !tlsData->ReentrancyFlag );
    tlsData->ReentrancyFlag = TRUE;

}   // SockPreApiCallout



VOID
SockPostApiCallout(
    VOID
    )

/*++

Routine Description:

    Undoes any preparation performed in SockPreApiCallout(). This routine
    is called after every hooker entrypoint. Currently, the only thing done
    here is to clear the per-thread reentrancy flag.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PSOCK_TLS_DATA tlsData;

    tlsData = SOCK_GET_THREAD_DATA();
    SOCK_ASSERT( tlsData != NULL );

    SOCK_ASSERT( tlsData->ReentrancyFlag );
    tlsData->ReentrancyFlag = FALSE;

}   // SockPostApiCallout


//
// Private functions.
//


INT
WINAPI
SockBlockingHook(
    VOID
    )

/*++

Routine Description:

    Blocking hook routine set in the hooker. If the current thread
    really has blocking hook installed, then we set the hooker's
    blocking hook to point to this routine. This allows us to make
    the appropriate upcall to the WS2_32.DLL for blocking hook
    processing.

Arguments:

    None.

Return Value:

    INT - TRUE if there are more messages to process, FALSE otherwise.
        (We always return FALSE.)

--*/

{

    INT result;
    PSOCK_TLS_DATA tlsData;

    tlsData = SOCK_GET_THREAD_DATA();
    SOCK_ASSERT( tlsData != NULL );
    SOCK_ASSERT( tlsData->BlockingCallback != NULL );
    SOCK_ASSERT( tlsData->BlockingSocketInfo != NULL );

    //
    // Call the blocking callback.
    //

    tlsData->IsBlocking = TRUE;
    result = (tlsData->BlockingCallback)( tlsData->BlockingContext );
    tlsData->IsBlocking = FALSE;

    if( !result ) {

        SOCK_ASSERT( tlsData->IoCancelled );

    }

    return FALSE;

}   // SockBlockingHook



VOID
SockBuildProtocolInfoForSocket(
    IN PSOCKET_INFORMATION SocketInfo,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo
    )

/*++

Routine Description:

    Builds an appropriate WSAPROTOCOL_INFOW structure for the
    given socket.

Arguments:

    SocketInfo - The socket.

    ProtocolInfo - Will receive the WSAPROTOCOL_INFOW structure.

Return Value:

    None.

--*/

{

    //
    // Sanity check.
    //

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( ProtocolInfo != NULL );

    //
    // Put it into a known state.
    //

    ZeroMemory(
        ProtocolInfo,
        sizeof(*ProtocolInfo)
        );

    //
    // Initialize the fixed components.
    //

    ProtocolInfo->dwCatalogEntryId = SocketInfo->CatalogEntryId;
    ProtocolInfo->iVersion = 2;
    ProtocolInfo->iAddressFamily = SocketInfo->AddressFamily;
    ProtocolInfo->iMaxSockAddr = SocketInfo->MaxSockaddrLength;
    ProtocolInfo->iMinSockAddr = SocketInfo->MinSockaddrLength;
    ProtocolInfo->iSocketType = SocketInfo->SocketType;
    ProtocolInfo->iProtocol = SocketInfo->Protocol;
    ProtocolInfo->iNetworkByteOrder = BIGENDIAN;
    ProtocolInfo->iSecurityScheme = SECURITY_PROTOCOL_NONE;
    ProtocolInfo->dwServiceFlags1 = SocketInfo->ServiceFlags1;
    ProtocolInfo->dwServiceFlags2 = SocketInfo->ServiceFlags2;
    ProtocolInfo->dwServiceFlags3 = SocketInfo->ServiceFlags3;
    ProtocolInfo->dwServiceFlags4 = SocketInfo->ServiceFlags4;
    ProtocolInfo->dwProviderFlags = SocketInfo->ProviderFlags;
    ProtocolInfo->dwMessageSize = (DWORD)SocketInfo->Hooker->MaxUdpDatagramSize;
    ProtocolInfo->ProviderId = SocketInfo->Hooker->ProviderId;

}   // SockBuildProtocolInfoForSocket

