/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Nspeprot.c

Abstract:

    This module contains support for the Name Space Provider API
    EnumProtocols().

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

--*/

#include "winsockp.h"



INT
SockLoadTransportList (
    OUT PWSTR *TransportList
    )
{
    DWORD transportListLength;
    INT error;
    HKEY winsockKey;
    ULONG type;

    //
    // Open the key that stores the list of transports that support
    // winsock.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\Winsock\\Parameters",
                0,
                KEY_READ,
                &winsockKey
                );
    if ( error != NO_ERROR ) {
        return error;
    }


    //
    // Determine the size of the mapping.  We need this so that we can
    // allocate enough memory to hold it.
    //

    transportListLength = 0;

    error = RegQueryValueExW(
                winsockKey,
                L"Transports",
                NULL,
                &type,
                NULL,
                &transportListLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        RegCloseKey( winsockKey );
        return error;
    }

    //
    // Allocate enough memory to hold the mapping.
    //

    *TransportList = ALLOCATE_HEAP( transportListLength );
    if ( *TransportList == NULL ) {
        RegCloseKey( winsockKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get the list of transports from the registry.
    //

    error = RegQueryValueExW(
                winsockKey,
                L"Transports",
                NULL,
                &type,
                (PVOID)*TransportList,
                &transportListLength
                );
    if ( error != NO_ERROR ) {
        RegCloseKey( winsockKey );
        FREE_HEAP( *TransportList );
        return error;
    }

    //
    // It worked!  The caller is responsible for freeing the memory
    // allocated to hold the list.
    //

    RegCloseKey( winsockKey );

    return NO_ERROR;

} // SockLoadTransportList


