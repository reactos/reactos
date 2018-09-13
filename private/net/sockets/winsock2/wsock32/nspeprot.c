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

#if defined(CHICAGO)
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspapi.h>
#include <nspapip.h>
#include <nspmisc.h>

INT
SockLoadTransportList (
    OUT PTSTR *TransportList
    );

#if defined(CHICAGO)
#include <wsahelp.h>

PTSTR
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    );
#endif


INT
APIENTRY
EnumProtocols (
    IN     LPINT           lpiProtocols,
    IN OUT LPVOID          lpProtocolBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT error;
    PTSTR transportList;
    PTSTR currentTransport;
    PTSTR helperDllName;
    PTSTR helperDllExpandedName;
    DWORD helperDllExpandedNameLength;
    PTSTR winsockKeyName;
    DWORD entryLength;
    HMODULE dllHandle;
    PWSH_ENUM_PROTOCOLS enumProtocols;
    HKEY winsockKey;
    DWORD type;
    INT count;
    INT totalCount;
    DWORD bytesUsed;
    DWORD bytesLeft;

    //
    // Initialize locals.
    //

    totalCount = 0;
    bytesUsed = 0;
    bytesLeft = *lpdwBufferLength;

    //
    // First get the list of winsock transports.
    //

    error = SockLoadTransportList( &transportList );
    if ( error != NO_ERROR ) {
        SetLastError( error );
        return -1;
    }

    //
    // For each transport, load the appropriate helper DLL.
    //

    for ( currentTransport = transportList;
          *currentTransport != UNICODE_NULL;
          currentTransport += _tcslen( currentTransport ) + 1 ) {

        //
        // Allocate some memory to hold the helper DLL name, etc.
        //

        helperDllName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
        if ( helperDllName == NULL ) {
            FREE_HEAP( transportList );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return -1;
        }

        helperDllExpandedName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
        if ( helperDllExpandedName == NULL ) {
            FREE_HEAP( transportList );
            FREE_HEAP( helperDllName );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return -1;
        }

        winsockKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(TCHAR) );
        if ( winsockKeyName == NULL ) {
            FREE_HEAP( transportList );
            FREE_HEAP( helperDllName );
            FREE_HEAP( helperDllExpandedName );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return -1;
        }

        //
        // Build the name of the transport's winsock key.
        //

        _tcscpy( winsockKeyName, REG_SERVICES_ROOT );
        _tcscat( winsockKeyName, currentTransport );
        _tcscat( winsockKeyName, TEXT("\\Parameters\\Winsock") );

        //
        // Open the transport's winsock key.  This key holds all necessary
        // information about winsock should support the transport.
        //

        error = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    winsockKeyName,
                    0,
                    KEY_READ,
                    &winsockKey
                    );
        FREE_HEAP( winsockKeyName );
        if ( error != NO_ERROR ) {
            FREE_HEAP( transportList );
            FREE_HEAP( helperDllName );
            FREE_HEAP( helperDllExpandedName );
            SetLastError( error );
            return -1;
        }

        //
        // Get the name of the helper DLL that this transport uses.
        //

        entryLength = DOS_MAX_PATH_LENGTH*sizeof(TCHAR);

        error = RegQueryValueEx(
                    winsockKey,
                    TEXT("HelperDllName"),
                    NULL,
                    &type,
                    (PVOID)helperDllName,
                    &entryLength
                    );
        if ( error != NO_ERROR ) {
            FREE_HEAP( transportList );
            FREE_HEAP( helperDllName );
            FREE_HEAP( helperDllExpandedName );
            RegCloseKey( winsockKey );
            SetLastError( error );
            return -1;
        }

        //
        // Expand the name of the DLL, converting environment variables to
        // their corresponding strings.
        //

        helperDllExpandedNameLength = ExpandEnvironmentStrings(
                                          helperDllName,
                                          helperDllExpandedName,
                                          DOS_MAX_PATH_LENGTH
                                          );
        FREE_HEAP( helperDllName );

        //
        // Load the helper DLL so that we can get at it's entry points.
        //

        dllHandle = LoadLibrary( helperDllExpandedName );

        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );

        if ( dllHandle == NULL ) {
            FREE_HEAP( transportList );
            return -1;
        }

        //
        // Get the addresses of the WSHEnumProtocols entry point for the
        // helper DLL.  If the helper DLL does not export this routine,
        // just skip it.
        //

        enumProtocols =
            (PWSH_ENUM_PROTOCOLS)GetProcAddress( dllHandle, "WSHEnumProtocols" );
        if ( enumProtocols == NULL ) {
            FreeLibrary( dllHandle );
            continue;
        }

        //
        // Calculate how many bytes are still available in the user
        // buffer.
        //

        if ( bytesUsed <= *lpdwBufferLength ) {
            bytesLeft = *lpdwBufferLength - bytesUsed;
        } else {
            bytesLeft = 0;
        }

        //
        // Call the helper DLL so that it fills in information about
        // the transport protocols which it supports.
        //

        count = enumProtocols(
                    lpiProtocols,
                    currentTransport,
                    lpProtocolBuffer,
                    &bytesLeft
                    );

        bytesUsed += bytesLeft;

        //
        // Unload the helper DLL--we're done with it.
        //

        FreeLibrary( dllHandle );

        //
        // If the helper DLL returned any entries then fix up pointers
        // and counts.
        //

        if ( count > 0 ) {
            totalCount += count;

            lpProtocolBuffer =
                (PBYTE)lpProtocolBuffer + (count * sizeof(PROTOCOL_INFO));
        }
    }

    //
    // Determine whether we overflowed the user buffer.
    //

    FREE_HEAP( transportList );

    if ( bytesUsed > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesUsed;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return -1;
    }

    *lpdwBufferLength = bytesUsed;
    return totalCount;

} // EnumProtocols

#if defined(UNICODE)


INT
APIENTRY
EnumProtocolsA (
    IN     LPINT           lpiProtocols,
    IN OUT LPVOID          lpProtocolBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT count;
    INT i;
    PPROTOCOL_INFO protocolInfo;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;

    //
    // Get the protocol information in Unicode format.
    //

    count = EnumProtocolsW( lpiProtocols, lpProtocolBuffer, lpdwBufferLength );
    if ( count <= 0 ) {
        return count;
    }

    //
    // Convert each of the Unicode protocol names to Ansi.
    //

    protocolInfo = lpProtocolBuffer;

    for ( i = 0; i < count; i++ ) {

        RtlInitUnicodeString( &unicodeString, protocolInfo[i].lpProtocol );
        ansiString.MaximumLength = unicodeString.MaximumLength;
        ansiString.Buffer = (PCHAR)unicodeString.Buffer;
        RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
    }

    return count;

} // EnumProtocolsA

#else // defined(UNICODE)

INT
APIENTRY
EnumProtocolsW (
    IN     LPINT           lpiProtocols,
    IN OUT LPVOID          lpProtocolBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // EnumProtocolsW

#endif // defined(UNICODE)


INT
SockLoadTransportList (
    OUT PTSTR *TransportList
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

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Services\\Winsock\\Parameters"),
                0,
                KEY_READ,
                &winsockKey
                );
    if ( error != NO_ERROR ) {
        return error;
    }

#if defined(CHICAGO)
    *TransportList = KludgeMultiSz(winsockKey, &transportListLength);

    if( *TransportList == NULL ) {
        RegCloseKey( winsockKey );
        return GetLastError();
    }
#else   // !CHICAGO

    //
    // Determine the size of the mapping.  We need this so that we can
    // allocate enough memory to hold it.
    //

    transportListLength = 0;

    error = RegQueryValueEx(
                winsockKey,
                TEXT("Transports"),
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

    error = RegQueryValueEx(
                winsockKey,
                TEXT("Transports"),
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

#endif  // !CHICAGO
    //
    // It worked!  The caller is responsible for freeing the memory
    // allocated to hold the list.
    //

    RegCloseKey( winsockKey );

    return NO_ERROR;

} // SockLoadTransportList

