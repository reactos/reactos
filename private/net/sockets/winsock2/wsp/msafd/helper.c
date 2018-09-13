/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    helper.c

Abstract:

    This module contains routines for interacting and handling helper
    DLLs in the winsock DLL.

Author:

    David Treadwell (davidtr)    25-Jul-1992

Revision History:

--*/

#include "winsockp.h"


VOID
SockFreeHelperDll (
    PWINSOCK_HELPER_DLL_INFO helperDll
	)
{
    FreeLibrary( helperDll->DllHandle );
    FREE_HEAP( helperDll->Mapping );
    FREE_HEAP( helperDll );
}



VOID
SockFreeHelperDlls (
    VOID
    )
{

    PLIST_ENTRY listEntry;
    PWINSOCK_HELPER_DLL_INFO helperDll;

    //
    // Global lock must be owner when this routine is called
    //
    WS_ASSERT (SocketGlobalLock.WriterId==NtCurrentTeb()->ClientId.UniqueThread);

    while ( !IsListEmpty( &SockHelperDllListHead ) ) {

        listEntry = RemoveHeadList( &SockHelperDllListHead );
        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );
		SockDereferenceHelper (helperDll);
    }

    return;

} // SockFreeHelperDlls


INT
SockGetTdiName (
    IN PINT AddressFamily,
    IN PINT SocketType,
    IN PINT Protocol,
    IN GROUP Group,
    IN DWORD Flags,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll,
    OUT PDWORD NotificationEvents
    )
{
    PLIST_ENTRY listEntry;
    PWINSOCK_HELPER_DLL_INFO helperDll;
    INT error;
    BOOLEAN resourceShared = TRUE;
    BOOLEAN addressFamilyFound = FALSE;
    BOOLEAN socketTypeFound = FALSE;
    BOOLEAN protocolFound = FALSE;
    BOOLEAN invalidProtocolMatch = FALSE;
    PWSTR transportList;
    PWSTR currentTransport;
    PWINSOCK_MAPPING mapping;

    //
    // Acquire lock for shared access and search the list of helper
    // DLLs for one which supports this combination of address family,
    // socket type, and protocol.
    //

    SockAcquireRwLockShared(&SocketGlobalLock);

walkList:

    for ( listEntry = SockHelperDllListHead.Flink;
          listEntry != &SockHelperDllListHead;
          listEntry = listEntry->Flink ) {


        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );

        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockGetTdiName: examining DLL at %lx for AF %ld, "
                       "ST %ld, Proto %ld\n", helperDll, AddressFamily,
                           SocketType, Protocol ));
        }

        //
        // Check to see whether the DLL supports the socket we're
        // opening.
        //
        if ( SockIsTripleInMapping(
                 helperDll->Mapping,
                 *AddressFamily,
                 &addressFamilyFound,
                 *SocketType,
                 &socketTypeFound,
                 *Protocol,
                 &protocolFound,
                 &invalidProtocolMatch ) ) {

            //
            // Found a match.  Try to use this DLL.
            //

            if( helperDll->WSHOpenSocket2 == NULL ) {

                //
                // This helper doesn't support the new WinSock 2
                // WSHOpenSocket2 entrypoint. If the application is
                // creating a "normal" socket, then just call through
                // to the old WinSock 1 WSHOpenSocket entrypoint.
                // Otherwise, fail the call.
                //

                if( ( Flags & ALL_MULTIPOINT_FLAGS ) == 0 ) {

                    error = helperDll->WSHOpenSocket(
                                           AddressFamily,
                                           SocketType,
                                           Protocol,
                                           TransportDeviceName,
                                           HelperDllSocketContext,
                                           NotificationEvents
                                           );

                } else {

                    error = WSAEINVAL;

                }

            } else {

                error = helperDll->WSHOpenSocket2(
                                       AddressFamily,
                                       SocketType,
                                       Protocol,
                                       Group,
                                       Flags,
                                       TransportDeviceName,
                                       HelperDllSocketContext,
                                       NotificationEvents
                                       );

            }

            if ( error == NO_ERROR ) {

	            
				SockReferenceHelper (helperDll);

                IF_DEBUG(HELPER_DLL) {
                    WS_PRINT(( "WSHOpenSocket by DLL at %lx succeeded, "
                               "context = %lx\n", helperDll,
                               *HelperDllSocketContext ));
                }

                //
                // The DLL accepted the socket.  Return a pointer to the
                // helper DLL info.
                //

                if ( resourceShared )
                {
                    SockReleaseRwLockShared(&SocketGlobalLock);
                }
                else
                {
                    SockReleaseRwLockExclusive(&SocketGlobalLock);
                }

                *HelperDll = helperDll;
                return NO_ERROR;
            }

            if ( (*SocketType == SOCK_RAW) &&
                 (TransportDeviceName->Buffer != NULL)
               )
            {
                RtlFreeHeap( RtlProcessHeap(), 0, TransportDeviceName->Buffer );
                TransportDeviceName->Buffer = NULL;
            }

            //
            // The open failed.  Continue searching for a matching DLL.
            //

            IF_DEBUG(HELPER_DLL) {
                WS_PRINT(( "WSHOpenSocket by DLL %lx failed: %ld\n",
                               helperDll, error ));
            }
        }
    }

    //
    // If we've not already done so, try again with exclusive access
    // to the helper dll list (since another thread may have loaded
    // the helper dll we want after we've released the resource),
    // then if we still can't find a match drop thru to code below
    //

    if ( resourceShared ) {

        SockReleaseRwLockShared(&SocketGlobalLock);
        SockAcquireRwLockExclusive(&SocketGlobalLock);
        resourceShared = FALSE;
        goto walkList;
    }

    //
    // We don't have any loaded DLLs that can accept this socket.
    // Attempt to find a DLL in the registry that can handle the
    // specified triple.  First get the REG_MULTI_SZ that contains the
    // list of transports that have winsock support.
    //

    error = SockLoadTransportList( &transportList );
    if ( error != NO_ERROR ) {
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return error;
    }

    //
    // Loop through the transports looking for one which will support
    // the socket we're opening.
    //

    for ( currentTransport = transportList;
          *currentTransport != UNICODE_NULL;
          currentTransport += wcslen( currentTransport ) + 1 ) {

        //
        // Load the list of triples supported by this transport.
        //

        error = SockLoadTransportMapping( currentTransport, &mapping );
        if ( error != NO_ERROR ) {
            WS_PRINT((
                "SockLoadTransportMapping( %ws ) failed: %ld\n",
                currentTransport,
                error
                ));
            continue;
        }

        //
        // Determine whether the triple of the socket we're opening is
        // in this transport's mapping.
        //

        if ( SockIsTripleInMapping(
                 mapping,
                 *AddressFamily,
                 &addressFamilyFound,
                 *SocketType,
                 &socketTypeFound,
                 *Protocol,
                 &protocolFound,
                 &invalidProtocolMatch ) ) {

            //
            // The triple is supported.  Load the helper DLL for the
            // transport.
            //

            error = SockLoadHelperDll( currentTransport, mapping, &helperDll );

            //
            // If we couldn't load the DLL, continue looking for a helper
            // DLL that will support this triple.
            //

            if ( error == NO_ERROR ) {

	
                //
                // Found a match.  Try to use this DLL.
                //

                if( helperDll->WSHOpenSocket2 == NULL ) {

                    //
                    // This helper doesn't support the new WinSock 2
                    // WSHOpenSocket2 entrypoint. If the application is
                    // creating a "normal" socket, then just call through
                    // to the old WinSock 1 WSHOpenSocket entrypoint.
                    // Otherwise, fail the call.
                    //

                    if( ( Flags & ALL_MULTIPOINT_FLAGS ) == 0 ) {

                        error = helperDll->WSHOpenSocket(
                                               AddressFamily,
                                               SocketType,
                                               Protocol,
                                               TransportDeviceName,
                                               HelperDllSocketContext,
                                               NotificationEvents
                                               );

                    } else {

                        error = WSAEINVAL;

                    }

                } else {

                    error = helperDll->WSHOpenSocket2(
                                           AddressFamily,
                                           SocketType,
                                           Protocol,
                                           Group,
                                           Flags,
                                           TransportDeviceName,
                                           HelperDllSocketContext,
                                           NotificationEvents
                                           );

                }

                if ( error == NO_ERROR ) {

					SockReferenceHelper (helperDll);
                    IF_DEBUG(HELPER_DLL) {
                        WS_PRINT(( "WSHOpenSocket by DLL at %lx succeeded, "
                                   "context = %lx\n", helperDll,
                                   *HelperDllSocketContext ));
                    }

                    //
                    // The DLL accepted the socket.  Free resources and
                    // return a pointer to the helper DLL info.
                    //

                    SockReleaseRwLockExclusive(&SocketGlobalLock);
                    FREE_HEAP( transportList );

                    *HelperDll = helperDll;
                    return NO_ERROR;
                }

                //
                // The open failed.  Continue searching for a matching DLL.
                //

                IF_DEBUG(HELPER_DLL) {
                    WS_PRINT(( "WSHOpenSocket by DLL %lx failed: %ld\n",
                                   helperDll, error ));
                }

                if ( (*SocketType == SOCK_RAW) &&
                     (TransportDeviceName->Buffer != NULL)
                   )
                {
                    RtlFreeHeap(
                        RtlProcessHeap(),
                        0,
                        TransportDeviceName->Buffer
                        );
                    TransportDeviceName->Buffer = NULL;
                }

                continue;
            }
        }

        //
        // This transport does not support the socket we're opening.
        // Free the memory that held the mapping and try the next
        // transport in the list.
        //

        FREE_HEAP( mapping );
    }

    SockReleaseRwLockExclusive(&SocketGlobalLock);

    FREE_HEAP (transportList);

    //
    // We didn't find any matches.  Return an error based on the matches that
    // did occur.
    //

    if ( invalidProtocolMatch ) {
        return WSAEPROTOTYPE;
    }

    if ( !addressFamilyFound ) {
        return WSAEAFNOSUPPORT;
    }

    if ( !socketTypeFound ) {
        return WSAESOCKTNOSUPPORT;
    }

    if ( !protocolFound ) {
        return WSAEPROTONOSUPPORT;
    }

    //
    // All the individual numbers were found, it is just the particular
    // combination that was invalid.
    //

    return WSAEINVAL;

} // SockGetTdiName


INT
SockLoadTransportMapping (
    IN PWSTR TransportName,
    OUT PWINSOCK_MAPPING *Mapping
    )
{
    PWSTR winsockKeyName;
    HKEY winsockKey;
    INT error;
    ULONG mappingLength;
    ULONG type;

    //
    // Allocate space to hold the winsock key name for the transport
    // we're accessing.
    //

    winsockKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( winsockKeyName == NULL ) {
        WS_PRINT(( "SockLoadTransportMapping: ALLOCATE_HEAP(1) failed: %ld\n",
                       GetLastError() ));

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Build the name of the transport's winsock key.
    //

    wcscpy( winsockKeyName, L"System\\CurrentControlSet\\Services\\" );
    wcscat( winsockKeyName, TransportName );
    wcscat( winsockKeyName, L"\\Parameters\\Winsock" );

    //
    // Open the transport's winsock key.  This key holds all necessary
    // information about winsock should support the transport.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                winsockKeyName,
                0,
                KEY_READ,
                &winsockKey
                );
    FREE_HEAP( winsockKeyName );
    if ( error != NO_ERROR ) {
        WS_PRINT(( "SockLoadTransportMapping: RegOpenKeyExW failed: %ld\n", error ));
        return error;
    }

    //
    // Determine the length of the mapping.
    //

    mappingLength = 0;

    error = RegQueryValueExW(
                winsockKey,
                L"Mapping",
                NULL,
                &type,
                NULL,
                &mappingLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        WS_PRINT(( "SockLoadTransportMapping: RegQueryValueEx(1) failed: %ld\n",
                           error ));
        RegCloseKey( winsockKey );
        return error;
    }

    WS_ASSERT( mappingLength >= sizeof(WINSOCK_MAPPING) );
    //WS_ASSERT( type == REG_BINARY );

    //
    // Allocate enough memory to hold the mapping.
    //

    *Mapping = ALLOCATE_HEAP( mappingLength );
    if ( *Mapping == NULL ) {
        WS_PRINT(( "SockLoadTransportMapping: ALLOCATE_HEAP(2) failed: %ld\n",
                       GetLastError() ));

        RegCloseKey( winsockKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get the mapping from the registry.
    //

    error = RegQueryValueExW(
                winsockKey,
                L"Mapping",
                NULL,
                &type,
                (PVOID)*Mapping,
                &mappingLength
                );
    if ( error != NO_ERROR ) {
        WS_PRINT(( "SockLoadTransportMapping: RegQueryValueEx(2) failed: %ld\n",
                       error ));
        RegCloseKey( winsockKey );
        return error;
    }

    //
    // It worked, return.
    //

    RegCloseKey( winsockKey );

    return NO_ERROR;

} // SockLoadTransportMapping


BOOL
SockIsTripleInMapping (
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily,
    OUT PBOOLEAN AddressFamilyFound,
    IN INT SocketType,
    OUT PBOOLEAN SocketTypeFound,
    IN INT Protocol,
    OUT PBOOLEAN ProtocolFound,
    OUT PBOOLEAN InvalidProtocolMatch
    )
{
    ULONG i;
    BOOLEAN addressFamilyFound = FALSE;
    BOOLEAN socketTypeFound = FALSE;
    BOOLEAN protocolFound = FALSE;

    //
    // Loop through the mapping attempting to find an exact match of
    // the triple.
    //

    for ( i = 0; i < Mapping->Rows; i++ ) {
        //
        // Remember if any of the individual elements were found.
        //

        if ( (INT)Mapping->Mapping[i].AddressFamily == AddressFamily ) {
            addressFamilyFound = TRUE;
        }

        if ( (INT)Mapping->Mapping[i].SocketType == SocketType ) {
            socketTypeFound = TRUE;
        }

        //
        // Special hack for AF_NETBIOS: the protocol does not have to
        // match.  This allows for support of multiple lanas.
        //
        // Same hack for SOCK_RAW - any protocol will do.
        //

        if ( (INT)Mapping->Mapping[i].Protocol == Protocol ||
                 AddressFamily == AF_NETBIOS || SocketType == SOCK_RAW
           )
        {
            protocolFound = TRUE;
        }

        if ( addressFamilyFound && socketTypeFound && !protocolFound ) {
            *InvalidProtocolMatch = TRUE;
        }

        //
        // Check for a full match.
        //

        if ( addressFamilyFound && socketTypeFound && protocolFound ) {

            //
            // The triple matched.  Return.
            //

            *AddressFamilyFound = TRUE;
            *SocketTypeFound = TRUE;
            *ProtocolFound = TRUE;

            return TRUE;
        }
    }

    //
    // No triple matched completely.
    //

    if ( addressFamilyFound ) {
        *AddressFamilyFound = TRUE;
    }

    if ( socketTypeFound ) {
        *SocketTypeFound = TRUE;
    }

    if ( protocolFound ) {
        *ProtocolFound = TRUE;
    }

    return FALSE;

} // SockIsTripleInMapping


INT
SockLoadHelperDll (
    IN PWSTR TransportName,
    IN PWINSOCK_MAPPING Mapping,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll
    )
{

    PWINSOCK_HELPER_DLL_INFO helperDll;
    PLIST_ENTRY listEntry;
    PWSTR helperDllName;
    PWSTR helperDllExpandedName;
    DWORD helperDllExpandedNameLength;
    PWSTR winsockKeyName;
    HKEY winsockKey;
    ULONG entryLength;
    ULONG type;
    INT error;

    //
    // Note: Assumes caller has exclusive access to SockHelperDllListLock
    //
    WS_ASSERT (SocketGlobalLock.WriterId==NtCurrentTeb()->ClientId.UniqueThread);

    //
    // Allocate some memory to cache information about the helper DLL,
    // the helper DLL's name, and the name of the transport's winsock
    // key.
    //

    helperDll = ALLOCATE_HEAP( FIELD_OFFSET (WINSOCK_HELPER_DLL_INFO,
                                                TransportName[wcslen(TransportName)+1]));
    if ( helperDll == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    helperDllName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( helperDllName == NULL ) {
        FREE_HEAP( helperDll );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    helperDllExpandedName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( helperDllExpandedName == NULL ) {
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    winsockKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( winsockKeyName == NULL ) {
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Build the name of the transport's winsock key.
    //

    wcscpy( winsockKeyName, L"System\\CurrentControlSet\\Services\\" );
    wcscat( winsockKeyName, TransportName );
    wcscat( winsockKeyName, L"\\Parameters\\Winsock" );

    //
    // Open the transport's winsock key.  This key holds all necessary
    // information about winsock should support the transport.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                winsockKeyName,
                0,
                KEY_READ,
                &winsockKey
                );
    FREE_HEAP( winsockKeyName );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegOpenKeyExW failed: %ld\n", error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    //
    // Read the minimum and maximum sockaddr lengths from the registry.
    //

    entryLength = sizeof(helperDll->MaxSockaddrLength);

    error = RegQueryValueExW(
                winsockKey,
                L"MinSockaddrLength",
                NULL,
                &type,
                (PVOID)&helperDll->MinSockaddrLength,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW(min) failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    WS_ASSERT( entryLength == sizeof(helperDll->MaxSockaddrLength) );
    WS_ASSERT( type == REG_DWORD );

    entryLength = sizeof(helperDll->MaxSockaddrLength);

    error = RegQueryValueExW(
                winsockKey,
                L"MaxSockaddrLength",
                NULL,
                &type,
                (PVOID)&helperDll->MaxSockaddrLength,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW(max) failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    WS_ASSERT( entryLength == sizeof(helperDll->MaxSockaddrLength) );
    WS_ASSERT( type == REG_DWORD );

    helperDll->MinTdiAddressLength = helperDll->MinSockaddrLength + 6;
    helperDll->MaxTdiAddressLength = helperDll->MaxSockaddrLength + 6;

    //
    // Check if we should use delayed acceptance with this transport.
    //

    entryLength = sizeof(helperDll->UseDelayedAcceptance);

    error = RegQueryValueExW(
                winsockKey,
                L"UseDelayedAcceptance",
                NULL,
                &type,
                (PVOID)&helperDll->UseDelayedAcceptance,
                &entryLength
                );
    if ( error == NO_ERROR ) {
        WS_ASSERT( entryLength == sizeof(helperDll->UseDelayedAcceptance) );
        WS_ASSERT( type == REG_DWORD );
    }
    else {
        //
        // It is ok to fail this.  This means that transport does not
        // support delayed acceptance with Winsock apps
        //

        helperDll->UseDelayedAcceptance = -1;
    }


    //
    // Get the name of the helper DLL that this transport uses.
    //

    entryLength = DOS_MAX_PATH_LENGTH*sizeof(WCHAR);

    error = RegQueryValueExW(
                winsockKey,
                L"HelperDllName",
                NULL,
                &type,
                (PVOID)helperDllName,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }
    WS_ASSERT( type == REG_EXPAND_SZ );

    //
    // Expand the name of the DLL, converting environment variables to
    // their corresponding strings.
    //

    helperDllExpandedNameLength = ExpandEnvironmentStringsW(
                                      helperDllName,
                                      helperDllExpandedName,
                                      DOS_MAX_PATH_LENGTH
                                      );
    WS_ASSERT( helperDllExpandedNameLength <= DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    FREE_HEAP( helperDllName );

    //
    // Load the helper DLL so that we can get at it's entry points.
    //

    IF_DEBUG(HELPER_DLL) {
        WS_PRINT(( "SockLoadHelperDll: loading helper DLL %ws\n",
                       helperDllExpandedName ));
    }

    helperDll->DllHandle = LoadLibraryW( helperDllExpandedName );
    error = GetLastError( );
    FREE_HEAP( helperDllExpandedName );
    if ( helperDll->DllHandle == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: LoadLibrary failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        RegCloseKey( winsockKey );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    RegCloseKey( winsockKey );

    //
    // Get the addresses of the entry points for the relevant helper DLL
    // routines.
    //

    helperDll->WSHOpenSocket =
        (PWSH_OPEN_SOCKET)GetProcAddress( helperDll->DllHandle, "WSHOpenSocket" );
    helperDll->WSHOpenSocket2 =
        (PWSH_OPEN_SOCKET2)GetProcAddress( helperDll->DllHandle, "WSHOpenSocket2" );
    if ( helperDll->WSHOpenSocket == NULL && helperDll->WSHOpenSocket2 == NULL ) {
        error = GetLastError( );;
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHOpenSocket failed: %ld\n", error ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    helperDll->WSHJoinLeaf =
        (PWSH_JOIN_LEAF)GetProcAddress( helperDll->DllHandle, "WSHJoinLeaf" );
    if ( helperDll->WSHJoinLeaf == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHJoinLeaf failed: %ld (continuing)\n", error ));
        }

        //
        // It is OK if WSHJoinLeaf() is not present--it just
        // means that this helper DLL does not support multipoint.
        //
    }

    helperDll->WSHNotify =
        (PWSH_NOTIFY)GetProcAddress( helperDll->DllHandle, "WSHNotify" );
    if ( helperDll->WSHNotify == NULL ) {
        error = GetLastError( );;
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHNotify failed: %ld\n", error ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    helperDll->WSHGetSocketInformation =
        (PWSH_GET_SOCKET_INFORMATION)GetProcAddress( helperDll->DllHandle, "WSHGetSocketInformation" );
    if ( helperDll->WSHGetSocketInformation == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetSocketInformation failed: %ld\n", error ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    helperDll->WSHSetSocketInformation =
        (PWSH_SET_SOCKET_INFORMATION)GetProcAddress( helperDll->DllHandle, "WSHSetSocketInformation" );
    if ( helperDll->WSHSetSocketInformation == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHSetSocketInformation failed: %ld\n", error ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    helperDll->WSHGetSockaddrType =
        (PWSH_GET_SOCKADDR_TYPE)GetProcAddress( helperDll->DllHandle, "WSHGetSockaddrType" );
    if ( helperDll->WSHGetSockaddrType == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetSockaddrType failed: %ld\n", error ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
    }

    helperDll->WSHGetWildcardSockaddr =
        (PWSH_GET_WILDCARD_SOCKADDR)GetProcAddress( helperDll->DllHandle, "WSHGetWildcardSockaddr" );
    if ( helperDll->WSHGetWildcardSockaddr == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetWildcardAddress failed: %ld (continuing)\n", error ));
        }

        //
        // It is OK if WSHGetWildcardSockaddr() is not present--it just
        // means that this helper DLL does not support autobind.
        //
    }

    helperDll->WSHGetBroadcastSockaddr =
        (PWSH_GET_BROADCAST_SOCKADDR)GetProcAddress( helperDll->DllHandle, "WSHGetBroadcastSockaddr" );
    if ( helperDll->WSHGetBroadcastSockaddr == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetBroadcastAddress failed: %ld (continuing)\n", error ));
        }

        //
        // It is OK if WSHGetBroadcastSockaddr() is not present--it just
        // means that SIO_GET_BROADCAST_ADDRESS will fail on sockets managed
        // by this helper DLL.
        //
    }

    helperDll->WSHAddressToString =
        (PWSH_ADDRESS_TO_STRING)GetProcAddress( helperDll->DllHandle, "WSHAddressToString" );
    if ( helperDll->WSHAddressToString == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHAddressToString failed: %ld (continuing)\n", error ));
        }

        //
        // It is OK if WSHAddressToString() is not present--it just
        // means that this helper DLL does not support address conversions.
        //
    }

    helperDll->WSHStringToAddress =
        (PWSH_STRING_TO_ADDRESS)GetProcAddress( helperDll->DllHandle, "WSHStringToAddress" );
    if ( helperDll->WSHStringToAddress == NULL ) {
        error = GetLastError ();
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHStringToAddress failed: %ld (continuing)\n", error ));
        }

        //
        // It is OK if WSHStringToAddress() is not present--it just
        // means that this helper DLL does not support address conversions.
        //
    }

    helperDll->WSHIoctl =
        (PWSH_IOCTL)GetProcAddress( helperDll->DllHandle, "WSHIoctl" );

    if( helperDll->WSHIoctl == NULL ) {
        error = GetLastError ();

        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHIoctl failed: %ld (continuing)\n", error ));
        }
        //
        // It is OK if WSHIoctl is not present -- it just means that
        // this helper DLL does not support nonstandard/extended
        // IOCTL codes.
        //

    }

    //
    // Save a pointer to the mapping structure for use on future socket
    // opens.
    //

    helperDll->Mapping = Mapping;

    wcscpy (helperDll->TransportName, TransportName);

    //
	// Set initial helper dll reference count to 1.
	// It will be incremented for each socket that uses this
	// DLL. When all socket are destroyed and WSPCleanup is
	// called enough times the count will go to 0 and DLL
	// structure will get freed.
	//
	helperDll->RefCount = 1;

    //
    // The load of the helper DLL was successful.  Place the cached
    // information about the DLL in the process's global list.  This
    // list allows us to use the same helper DLL on future socket()
    // calls without accessing the registry.
    //

    InsertHeadList( &SockHelperDllListHead, &helperDll->HelperDllListEntry );

    *HelperDll = helperDll;

    //
    // Check if we already have the helper DLL structure for the same transport
    // and free it. We'll use the update one instead.
    //
    listEntry = helperDll->HelperDllListEntry.Flink;
    while (listEntry != &SockHelperDllListHead) {


        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );
        listEntry = listEntry->Flink;
        if (wcscmp (helperDll->TransportName, (*HelperDll)->TransportName)==0) {
            RemoveEntryList (&helperDll->HelperDllListEntry);
            SockDereferenceHelper (helperDll);
        }
    }

    return NO_ERROR;

} // SockLoadHelperDll


INT
SockNotifyHelperDll (
    IN PSOCKET_INFORMATION Socket,
    IN DWORD Event
    )
{
    INT error;

    if ( (Socket->HelperDllNotificationEvents & Event) == 0 ) {

        //
        // The helper DLL does not care about this state transition.
        // Just return.
        //

        return NO_ERROR;
    }

    if( Socket->HelperDllContext == NULL ) {

        //
        // There is no context associated with the socket, so just return.
        //

        return NO_ERROR;
    }

    //
    // Get the TDI handles for the socket.
    //

    error = SockGetTdiHandles( Socket );
    if ( error != NO_ERROR ) {
        return error;
    }

    // !!! If we're terminating, don't do the notification.  This is
    //     a hack because we don't have reference counts on helper DLL
    //     info structures.  Post-beta, add helper DLL refcnts.

    //if ( SockTerminating ) {
    //    return NO_ERROR;
    //}

    //
    // Call the help DLL's notification routine.
    //

    return Socket->HelperDll->WSHNotify(
               Socket->HelperDllContext,
               Socket->Handle,
               Socket->TdiAddressHandle,
               Socket->TdiConnectionHandle,
               Event
               );

} // SockNotifyHelperDll

BOOL
SockDefaultValidateAddressForConstrainedGroup(
    IN PSOCKADDR Sockaddr1,
    IN PSOCKADDR Sockaddr2,
    IN INT SockaddrLength
    )
{

    //
    // Just about the only thing we can do in a protocol indepenent manner
    // is to blindly byte-compare the addresses.
    //

    return RtlEqualMemory(
               Sockaddr1,
               Sockaddr2,
               (UINT)SockaddrLength
               );

}   // SockDefaultValidateAddressForConstrainedGroup

