/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    hookproc.c

Abstract:

    This module contains hooker management code for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockFindAndReferenceHooker()
        SockReferenceHooker()
        SockDereferenceHooker()
        SockFreeAllHookers()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


PHOOKER_INFORMATION
SockFindAndReferenceHooker(
    IN LPGUID ProviderId
    )

/*++

Routine Description:

    This routine maps the given provider GUID to the corresponding
    HOOKER_INFORMATION structure. If the specified hooker has already
    been loaded (and therefore has an entry in the global list) its
    structure pointer is simply returned. Otherwise, an attempt is made
    to load the necessary information from the registry.

Arguments:

    ProviderId - A pointer to the GUID identifying a hooker.

Return Value:

    PHOOKER_INFORMATION - Pointer to the hooker structure if successful,
        NULL otherwise.

--*/

{

    PLIST_ENTRY listEntry;
    PHOOKER_INFORMATION hookerInfo;
    RPC_STATUS status;
    UCHAR * str;
    LONG err;
    DWORD type;
    DWORD length;
    HINSTANCE dllHandle;
    CHAR dllPath[MAX_PATH];
    CHAR expandedDllPath[MAX_PATH];
    WSADATA wsaData;
    INT result;

    //
    // Sanity check.
    //

    SOCK_ASSERT( ProviderId != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    str = NULL;
    dllHandle = NULL;

    //
    // Scan the in-memory hooker list.
    //

    SockAcquireGlobalLock();

    try {

        for( listEntry = SockHookerListHead.Flink ;
             listEntry != &SockHookerListHead ;
             listEntry = listEntry->Flink ) {

            hookerInfo = CONTAINING_RECORD(
                             listEntry,
                             HOOKER_INFORMATION,
                             HookerListEntry
                             );

            if( GUIDS_ARE_EQUAL(
                    ProviderId,
                    &hookerInfo->ProviderId
                    ) ) {

                if( hookerInfo->Initialized ) {

                    hookerInfo->ReferenceCount++;

                } else {

                    hookerInfo = NULL;

                }

                leave;

            }

        }

        //
        // Not found in memory, try to load it from the registry.
        //

        //
        // Prepare for early exit.
        //

        hookerInfo = NULL;

        //
        // Open the registry key if not already open.
        //

        if( SockHookerRegistryKey == NULL ) {

            err = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      SOCK_HOOKER_GUID_MAPPER_KEY,
                      0,
                      KEY_READ,
                      &SockHookerRegistryKey
                      );

            if( err != NO_ERROR ) {

                SOCK_PRINT((
                    "SockFindAndReferenceHooker: cannot open %s, error %ld\n",
                    SOCK_HOOKER_GUID_MAPPER_KEY,
                    err
                    ));

                leave;

            }

        }

        //
        // Map the GUID to a string.
        //

        status = UuidToString( ProviderId, &str );

        if( status != RPC_S_OK ) {

            SOCK_PRINT((
                "SockFindAndReferenceHooker: cannot map GUID to string, error %d\n",
                status
                ));

            leave;

        }

        //
        // Try to load the DLL path from the registry.
        //

        length = sizeof(dllPath);

        err = RegQueryValueEx(
                  SockHookerRegistryKey,
                  str,
                  NULL,
                  &type,
                  (LPBYTE)dllPath,
                  &length
                  );

        if( err != NO_ERROR ) {

            SOCK_PRINT((
                "SockFindAndReferenceHooker: cannot query %s, error %ld\n",
                str,
                err
                ));

            leave;

        }

        //
        // Expand any embedded environment strings.
        //

        length = ExpandEnvironmentStrings(
                     dllPath,
                     expandedDllPath,
                     sizeof(expandedDllPath)
                     );

        if( length == 0 ) {

            err = GetLastError();

            SOCK_PRINT((
                "SockFindAndReferenceHooker: cannot expand %s, error %ld\n",
                dllPath,
                err
                ));

            leave;

        }

        //
        // Load the DLL.
        //

        dllHandle = LoadLibrary( expandedDllPath );

        if( dllHandle == NULL ) {

            err = GetLastError();

            SOCK_PRINT((
                "SockFindAndReferenceHooker: cannot load %s, error %ld\n",
                expandedDllPath,
                err
                ));

            leave;

        }

        //
        // Allocate a new hooker.
        //

        hookerInfo = SOCK_ALLOCATE_HEAP( sizeof(*hookerInfo) );

        if( hookerInfo == NULL ) {

            leave;

        }

        ZeroMemory(
            hookerInfo,
            sizeof(*hookerInfo)
            );

        //
        // Find the entrypoints.
        //

        hookerInfo->accept = (PVOID)GetProcAddress( dllHandle, "accept" );
        hookerInfo->bind = (PVOID)GetProcAddress( dllHandle, "bind" );
        hookerInfo->closesocket = (PVOID)GetProcAddress( dllHandle, "closesocket" );
        hookerInfo->connect = (PVOID)GetProcAddress( dllHandle, "connect" );
        hookerInfo->ioctlsocket = (PVOID)GetProcAddress( dllHandle, "ioctlsocket" );
        hookerInfo->getpeername = (PVOID)GetProcAddress( dllHandle, "getpeername" );
        hookerInfo->getsockname = (PVOID)GetProcAddress( dllHandle, "getsockname" );
        hookerInfo->getsockopt = (PVOID)GetProcAddress( dllHandle, "getsockopt" );
        hookerInfo->listen = (PVOID)GetProcAddress( dllHandle, "listen" );
        hookerInfo->recv = (PVOID)GetProcAddress( dllHandle, "recv" );
        hookerInfo->recvfrom = (PVOID)GetProcAddress( dllHandle, "recvfrom" );
        hookerInfo->select = (PVOID)GetProcAddress( dllHandle, "select" );
        hookerInfo->send = (PVOID)GetProcAddress( dllHandle, "send" );
        hookerInfo->sendto = (PVOID)GetProcAddress( dllHandle, "sendto" );
        hookerInfo->setsockopt = (PVOID)GetProcAddress( dllHandle, "setsockopt" );
        hookerInfo->shutdown = (PVOID)GetProcAddress( dllHandle, "shutdown" );
        hookerInfo->socket = (PVOID)GetProcAddress( dllHandle, "socket" );
        hookerInfo->WSAStartup = (PVOID)GetProcAddress( dllHandle, "WSAStartup" );
        hookerInfo->WSACleanup = (PVOID)GetProcAddress( dllHandle, "WSACleanup" );
        hookerInfo->WSAGetLastError = (PVOID)GetProcAddress( dllHandle, "WSAGetLastError" );
        hookerInfo->WSAIsBlocking = (PVOID)GetProcAddress( dllHandle, "WSAIsBlocking" );
        hookerInfo->WSAUnhookBlockingHook = (PVOID)GetProcAddress( dllHandle, "WSAUnhookBlockingHook" );
        hookerInfo->WSASetBlockingHook = (PVOID)GetProcAddress( dllHandle, "WSASetBlockingHook" );
        hookerInfo->WSACancelBlockingCall = (PVOID)GetProcAddress( dllHandle, "WSACancelBlockingCall" );
        hookerInfo->WSAAsyncSelect = (PVOID)GetProcAddress( dllHandle, "WSAAsyncSelect" );

        hookerInfo->gethostname = (PVOID)GetProcAddress( dllHandle, "gethostname" );
        hookerInfo->gethostbyname = (PVOID)GetProcAddress( dllHandle, "gethostbyname" );
        hookerInfo->gethostbyaddr = (PVOID)GetProcAddress( dllHandle, "gethostbyaddr" );
        hookerInfo->getservbyname = (PVOID)GetProcAddress( dllHandle, "getservbyname" );
        hookerInfo->getservbyport = (PVOID)GetProcAddress( dllHandle, "getservbyport" );

        if( hookerInfo->accept == NULL ||
            hookerInfo->bind == NULL ||
            hookerInfo->closesocket == NULL ||
            hookerInfo->connect == NULL ||
            hookerInfo->ioctlsocket == NULL ||
            hookerInfo->getpeername == NULL ||
            hookerInfo->getsockname == NULL ||
            hookerInfo->getsockopt == NULL ||
            hookerInfo->listen == NULL ||
            hookerInfo->recv == NULL ||
            hookerInfo->recvfrom == NULL ||
            hookerInfo->select == NULL ||
            hookerInfo->send == NULL ||
            hookerInfo->sendto == NULL ||
            hookerInfo->setsockopt == NULL ||
            hookerInfo->shutdown == NULL ||
            hookerInfo->socket == NULL ||
            hookerInfo->WSAStartup == NULL ||
            hookerInfo->WSACleanup == NULL ||
            hookerInfo->WSAGetLastError == NULL ||
            hookerInfo->WSAIsBlocking == NULL ||
            hookerInfo->WSAUnhookBlockingHook == NULL ||
            hookerInfo->WSASetBlockingHook == NULL ||
            hookerInfo->WSACancelBlockingCall == NULL ||
            hookerInfo->WSAAsyncSelect == NULL ||
            hookerInfo->gethostname == NULL ||
            hookerInfo->gethostbyname == NULL ||
            hookerInfo->gethostbyaddr == NULL ||
            hookerInfo->getservbyname == NULL ||
            hookerInfo->getservbyname == NULL
            ) {

            SOCK_PRINT((
                "SockFindAndReferenceHooker: cannot find entrypoints in %s\n",
                expandedDllPath
                ));

            SOCK_FREE_HEAP( hookerInfo );
            hookerInfo = NULL;
            leave;

        }

        //
        // Go ahead and put it on the global list, but with the "Initialized"
        // flag set to FALSE. We don't set this to TRUE until after we've
        // completed the hooker's initialization.
        //

        hookerInfo->ProviderId = *ProviderId;
        SOCK_ASSERT( !hookerInfo->Initialized );

        InsertHeadList(
            &SockHookerListHead,
            &hookerInfo->HookerListEntry
            );

        //
        // Initialize it.
        //
        // Note that the hooker starts out with a reference count of two;
        // one for the "active" reference, and one for the reference added
        // by this routine.
        //

        SockPreApiCallout();

        result = hookerInfo->WSAStartup(
                     MAKEWORD(1,1),
                     &wsaData
                     );

        if( result != NO_ERROR ) {

            SOCK_PRINT((
                "SockFindAndReferenceHooker: %s:WSAStartup() failed, error %d\n",
                expandedDllPath,
                result
                ));

            RemoveEntryList(
                &hookerInfo->HookerListEntry
                );

            SOCK_FREE_HEAP( hookerInfo );
            hookerInfo = NULL;
            SockPostApiCallout();
            leave;

        }

        //
        // Save the max UDP datagram size for future reference.
        //

        hookerInfo->MaxUdpDatagramSize = (INT)wsaData.iMaxUdpDg;

        //
        // Create and bind the anchor socket.
        //

        hookerInfo->AnchorSocket = hookerInfo->socket(
                                       AF_INET,
                                       SOCK_STREAM,
                                       IPPROTO_TCP
                                       );

        if( hookerInfo->AnchorSocket != INVALID_SOCKET ) {

            SOCKADDR_IN addr;

            ZeroMemory(
                &addr,
                sizeof(addr)
                );

            addr.sin_family = AF_INET;

            hookerInfo->bind(
                hookerInfo->AnchorSocket,
                (SOCKADDR *)&addr,
                sizeof(addr)
                );

        }

        SockPostApiCallout();

        //
        // Finish initializing the hooker.
        //

        hookerInfo->DllHandle = dllHandle;
        hookerInfo->ReferenceCount = 2;
        hookerInfo->Initialized = TRUE;

        //
        // Success!
        //

        IF_DEBUG(HOOKER) {

            SOCK_PRINT((
                "SockFindAndReferenceHooker: loaded %s [%s] @ %08lx\n",
                str,
                dllPath,
                hookerInfo
                ));

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        hookerInfo = NULL;

    }

    SockReleaseGlobalLock();

    if( str != NULL ) {

        RpcStringFree( &str );

    }

    if( hookerInfo == NULL && dllHandle != NULL ) {

        FreeLibrary( dllHandle );

    }

    return hookerInfo;

}   // SockFindAndReferenceHooker



VOID
SockReferenceHooker(
    IN PHOOKER_INFORMATION HookerInfo
    )

/*++

Routine Description:

    This routine references the given HOOKER_INFORMATION structure.

Arguments:

    HookerInfo - A pointer to the HOOKER_INFORMATION structure to
        reference.

--*/

{

    //
    // Sanity check.
    //

    SOCK_ASSERT( HookerInfo != NULL );

    //
    // Reference the hooker, protected by a lock.
    //

    SockAcquireGlobalLock();

    try {

        HookerInfo->ReferenceCount++;
        SOCK_ASSERT( HookerInfo->ReferenceCount != 0 );

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    SockReleaseGlobalLock();

}   // SockReferenceHooker



VOID
SockDereferenceHooker(
    IN PHOOKER_INFORMATION HookerInfo
    )

/*++

Routine Description:

    This routine dereferences the given HOOKER_INFORMATION structure.
    If the reference count drops to zero, all resources associated with
    the structure are released.

Arguments:

    HookerInfo - A pointer to the HOOKER_INFORMATION structure to
        dereference.

--*/

{

    INT result;
    INT err;

    //
    // Sanity check.
    //

    SOCK_ASSERT( HookerInfo != NULL );

    //
    // Dereference the hooker, protected by a lock.
    //

    SockAcquireGlobalLock();

    try {

        HookerInfo->ReferenceCount--;

        if( HookerInfo->ReferenceCount == 0 ) {

            //
            // Shut 'er down, Scotty.
            //

            if( HookerInfo->AnchorSocket != INVALID_SOCKET ) {

                result = HookerInfo->closesocket(
                             HookerInfo->AnchorSocket
                             );

                if( result == SOCKET_ERROR ) {

                    err = HookerInfo->WSAGetLastError();

                    SOCK_PRINT((
                        "SockDereferenceHooker: closesocket() failed, error %d\n",
                        err
                        ));

                    //
                    // Press on regardless...
                    //

                }

            }

            result = HookerInfo->WSACleanup();

            if( result == SOCKET_ERROR ) {

                err = HookerInfo->WSAGetLastError();

                SOCK_PRINT((
                    "SockDereferenceHooker: WSACleanup() failed, error %d\n",
                    err
                    ));

                //
                // Press on regardless...
                //

            }

            //
            // Remove the hooker from the global list, then free
            // the hooker's resources.
            //

            RemoveEntryList( &HookerInfo->HookerListEntry );
            FreeLibrary( HookerInfo->DllHandle );
            SOCK_FREE_HEAP( HookerInfo );

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    SockReleaseGlobalLock();

}   // SockDereferenceHooker



VOID
SockFreeAllHookers(
    VOID
    )

/*++

Routine Description:

    This routine frees all allocated HOOKER_INFORMATION structures
    and closes the hooker registry key.

--*/

{

    PLIST_ENTRY listEntry;
    PHOOKER_INFORMATION hookerInfo;

    //
    // Scan the in-memory hooker list and free 'em.
    //

    SockAcquireGlobalLock();

    try {

        for( listEntry = SockHookerListHead.Flink ;
             listEntry != &SockHookerListHead ; ) {

            hookerInfo = CONTAINING_RECORD(
                             listEntry,
                             HOOKER_INFORMATION,
                             HookerListEntry
                             );

            //
            // Dereference the hooker.
            //

            SockDereferenceHooker( hookerInfo );

        }

        //
        // Close the registry key if necessary.
        //

        if( SockHookerRegistryKey != NULL ) {

            RegCloseKey( SockHookerRegistryKey );
            SockHookerRegistryKey = NULL;

        }

    } except( SOCK_EXCEPTION_FILTER() ) {

        NOTHING;

    }

    SockReleaseGlobalLock();

}   // SockFreeAllHookers

