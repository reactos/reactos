/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    addrconv.c

Abstract:

    This module contains address conversion routines.

Author:

    Keith Moore (keithmo)        8-Jan-1996

Revision History:

--*/

#include "winsockp.h"


PWINSOCK_HELPER_DLL_INFO
SockFindHelperDllForAddressFamily(
    INT AddressFamily
    );

BOOL
SockIsAddressFamilySupported(
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily
    );



int
WSPAPI
WSPAddressToString(
    LPSOCKADDR lpsaAddress,
    DWORD dwAddressLength,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPWSTR lpszAddressString,
    LPDWORD lpdwAddressStringLength,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine converts all components of a SOCKADDR structure into a human-
    readable string representation of the address. This is used mainly for
    display purposes.

Arguments:

    lpsaAddress - Points to a SOCKADDR structure to translate into a string.

    dwAddressLength - The length of the Address SOCKADDR.

    lpProtocolInfo - The WSAPROTOCOL_INFOW struct for a particular provider.

    lpszAddressString - A buffer which receives the human-readable address
        string.

    lpdwAddressStringLength - The length of the AddressString buffer. Returns
        the length of the string actually copied into the buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPAddressToString() returns 0. Otherwise, it returns
        SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/

{

    PWINSOCK_HELPER_DLL_INFO helperDll;
    INT addressFamily;

    __try {
        addressFamily = (INT)(INT)lpsaAddress->sa_family;
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    //
    // Find a helper DLL supporting this address family.
    //

    helperDll = SockFindHelperDllForAddressFamily(addressFamily);

    if( helperDll == NULL ||
        helperDll->WSHAddressToString == NULL ) {

        *lpErrno = WSA_INVALID_PARAMETER;
        return SOCKET_ERROR;

    }

    if( dwAddressLength < (DWORD)helperDll->MinSockaddrLength ) {

        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;

    }

    *lpErrno = helperDll->WSHAddressToString(
                   lpsaAddress,
                   dwAddressLength,
                   lpProtocolInfo,
                   lpszAddressString,
                   lpdwAddressStringLength
                   );

    return ( *lpErrno == 0 ) ? NO_ERROR : SOCKET_ERROR;

}   // WSPAddressToString


int
WSPAPI
WSPStringToAddress(
    LPWSTR AddressString,
    INT AddressFamily,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPSOCKADDR lpAddress,
    LPINT lpAddressLength,
    LPINT lpErrno
    )

/*++

Routine Description:

    This routine converts a human-readable string to a socket address
    structure (SOCKADDR) suitable for pass to Windows Sockets routines which
    take such a structure. Any missing components of the address will be
    defaulted to a reasonable value if possible. For example, a missing port
    number will be defaulted to zero.

Arguments:

    AddressString - Points to the zero-terminated human-readable string to
        convert.

    AddressFamily - The address family to which the string belongs, or
        AF_UNSPEC if it is unknown.

    lpProtocolInfo - The provider's WSAPROTOCOL_INFOW struct.

    lpAddress - A buffer which is filled with a single SOCKADDR structure.

    lpAddressLength - The length of the Address buffer. Returns the size of
        the resultant SOCKADDR structure.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPStringToAddress() returns 0. Otherwise, a value
        of SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    PWINSOCK_HELPER_DLL_INFO helperDll;

    //
    // Find a helper DLL supporting this address family.
    //

    helperDll = SockFindHelperDllForAddressFamily( AddressFamily );

    if( helperDll == NULL ||
        helperDll->WSHStringToAddress == NULL ) {

        *lpErrno = WSA_INVALID_PARAMETER;
        return SOCKET_ERROR;

    }

    __try {
        if( *lpAddressLength < helperDll->MinSockaddrLength ) {

            *lpErrno = WSAEFAULT;
            return SOCKET_ERROR;

        }
    }
    __except (SOCK_EXCEPTION_FILTER()) {
        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    *lpErrno = helperDll->WSHStringToAddress(
                   AddressString,
                   AddressFamily,
                   lpProtocolInfo,
                   lpAddress,
                   lpAddressLength
                   );

    return ( *lpErrno == 0 ) ? NO_ERROR : SOCKET_ERROR;

}   // WSPStringToAddress


PWINSOCK_HELPER_DLL_INFO
SockFindHelperDllForAddressFamily(
    INT AddressFamily
    )
{
    BOOLEAN resourceShared = TRUE;
    PLIST_ENTRY listEntry;
    PWINSOCK_HELPER_DLL_INFO helperDll;
    INT error;
    PWSTR transportList;
    PWSTR currentTransport;
    PWINSOCK_MAPPING mapping;

    //
    // Acquire lock for shared access and search the list of helper
    // DLLs for one which supports this address family.
    //

    SockAcquireRwLockShared(&SocketGlobalLock);

walkList:

    for( listEntry = SockHelperDllListHead.Flink;
         listEntry != &SockHelperDllListHead;
         listEntry = listEntry->Flink ) {

        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );

        IF_DEBUG( HELPER_DLL ) {

            WS_PRINT(( "SockFindHelperDllForAddressFamily: examining DLL at %lx for AF %ld\n",
                       helperDll, AddressFamily ));

        }

        //
        // Check to see whether the DLL supports the address family.
        //

        if( SockIsAddressFamilySupported(
                helperDll->Mapping,
                AddressFamily
                ) ) {

            //
            // Found a match.
            //

            if ( resourceShared )
            {
                SockReleaseRwLockShared(&SocketGlobalLock);
            }
            else
            {
                SockReleaseRwLockExclusive(&SocketGlobalLock);
            }

            return helperDll;
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
    // We don't have any loaded DLLs that support this address family.
    // Attempt to find a DLL in the registry that can handle the
    // specified address family.  First get the REG_MULTI_SZ that contains
    // the list of transports that have winsock support.
    //
    error = SockLoadTransportList( &transportList );

    if( error != NO_ERROR ) {

        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return NULL;

    }

    helperDll = NULL;

    //
    // Loop through the transports looking for one which will support
    // the socket we're opening.
    //

    for( currentTransport = transportList;
         *currentTransport != UNICODE_NULL;
         currentTransport += wcslen( currentTransport ) + 1 ) {

        //
        // Load the list of triples supported by this transport.
        //

        error = SockLoadTransportMapping( currentTransport, &mapping );

        if( error != NO_ERROR ) {

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

        if( SockIsAddressFamilySupported(
                mapping,
                AddressFamily
                ) ) {

            //
            // The address family is supported.  Load the helper DLL for the
            // transport.
            //

            error = SockLoadHelperDll( currentTransport, mapping, &helperDll );

            //
            // If we couldn't load the DLL, continue looking for a helper
            // DLL that will support this triple.
            //

            if( error == NO_ERROR ) {

                //
                // Success!
                //

                break;

            }

        }

        //
        // This transport does not support the socket we're opening.
        // Free the memory that held the mapping and try the next
        // transport in the list.
        //

        FREE_HEAP( mapping );
        WS_ASSERT ( helperDll==NULL );

    }

    SockReleaseRwLockExclusive(&SocketGlobalLock);

    FREE_HEAP (transportList);

    return helperDll;

}   // SockFindHelperDllForAddressFamily


BOOL
SockIsAddressFamilySupported(
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily
    )
{
    ULONG i;

    //
    // Loop through the mapping attempting to find the address family.
    //

    for ( i = 0; i < Mapping->Rows; i++ ) {

        if ( (INT)Mapping->Mapping[i].AddressFamily == AddressFamily ) {

            return TRUE;

        }

    }

    return FALSE;

} // SockIsAddressFamilySupported
