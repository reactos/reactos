/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regqkey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    query key APIs.  That is:

        - RegQueryInfoKeyA
        - RegQueryInfoKeyW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regqkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

LONG
RegQueryInfoKeyA (
    HKEY hKey,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for querying information about a previously
    opened key.

--*/

{
    PUNICODE_STRING     Class;
    UNICODE_STRING      UnicodeString;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;
    DWORD               cSubKeys;
    DWORD               cbMaxSubKeyLen;
    DWORD               cbMaxClassLen;
    DWORD               cValues;
    DWORD               cbMaxValueNameLen;
    DWORD               cbMaxValueLen;
    DWORD               cbSecurityDescriptor;
    FILETIME            ftLastWriteTime;
    HKEY                TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpClass ) && ( ! ARGUMENT_PRESENT( lpcbClass )))) {
        return ERROR_INVALID_PARAMETER;
    }


    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    //  Make sure that the buffer size for lpClass is zero if lpClass is NULL
    //
    if( !ARGUMENT_PRESENT( lpClass ) && ARGUMENT_PRESENT( lpcbClass ) ) {
        *lpcbClass = 0;
    }

    //
    //  If the count of bytes in the class is 0, pass a NULL pointer
    //  instead of what was supplied.  This ensures that RPC won't
    //  attempt to copy data to a bogus pointer.  Note that in this
    //  case we use the unicode string allocated on the stack, because
    //  we must not change the Buffer or MaximumLength fields of the
    //  static unicode string in the TEB.
    //
    if ( !ARGUMENT_PRESENT( lpClass ) || *lpcbClass == 0 ) {

        Class = &UnicodeString;
        Class->Length           = 0;
        Class->MaximumLength    = 0;
        Class->Buffer           = NULL;

    } else {

        //
        // Use the static Unicode string in the TEB as a temporary for the
        // key's class.
        //
        Class = &NtCurrentTeb( )->StaticUnicodeString;
        ASSERT( Class != NULL );
        Class->Length = 0;
    }


    //
    // Call the Base API passing it a pointer to a counted Unicode string
    // for the class string.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryInfoKey(
                                hKey,
                                Class,
                                &cSubKeys,
                                &cbMaxSubKeyLen,
                                &cbMaxClassLen,
                                &cValues,
                                &cbMaxValueNameLen,
                                &cbMaxValueLen,
                                &cbSecurityDescriptor,
                                &ftLastWriteTime
                                );
    } else {

        Error = (LONG)BaseRegQueryInfoKey(
                                DereferenceRemoteHandle( hKey ),
                                Class,
                                &cSubKeys,
                                &cbMaxSubKeyLen,
                                &cbMaxClassLen,
                                &cValues,
                                &cbMaxValueNameLen,
                                &cbMaxValueLen,
                                &cbSecurityDescriptor,
                                &ftLastWriteTime
                                );
        if (Error == ERROR_SUCCESS) {
            DWORD dwVersion;

            //
            // Check for a downlevel Win95 server, which requires
            // us to work around their BaseRegQueryInfoKey bugs.
            // They do not account for Unicode correctly.
            //
            if (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion)) {
                //
                // This is a Win95 server.
                // Double the maximum value name length and
                // maximum value data length to account for
                // the Unicode translation that Win95 forgot
                // to account for.
                //
                cbMaxValueNameLen *= sizeof(WCHAR);
                cbMaxValueLen *= sizeof(WCHAR);
            }
        }
    }

    //
    //  MaxSubKeyLen, MaxClassLen, and MaxValueNameLen should be in
    //  number of characters, without counting the NULL.
    //  Note that the server side will return the number of bytes,
    //  without counting the NUL
    //

    cbMaxSubKeyLen /= sizeof( WCHAR );
    cbMaxClassLen /= sizeof( WCHAR );
    cbMaxValueNameLen /= sizeof( WCHAR );


    //
    //  Subtract the NULL from the Length. This was added on
    //  the server side so that RPC would transmit it.
    //
    if ( Class->Length > 0 ) {
        Class->Length -= sizeof( UNICODE_NULL );
    }

    //
    // If all the information was succesfully queried from the key
    // convert the class name to ANSI and update the class length value.
    //

    if( ( Error == ERROR_SUCCESS ) &&
        ARGUMENT_PRESENT( lpClass ) && ( *lpcbClass != 0 ) ) {

        if (*lpcbClass > (DWORD)0xFFFF) {
            AnsiString.MaximumLength    = ( USHORT ) 0xFFFF;
        } else {
            AnsiString.MaximumLength    = ( USHORT ) *lpcbClass;
        }

        AnsiString.Buffer           = lpClass;

        Status = RtlUnicodeStringToAnsiString(
                    &AnsiString,
                    Class,
                    FALSE
                    );
        ASSERTMSG( "Unicode->ANSI conversion of Class ",
                    NT_SUCCESS( Status ));

        //
        // Update the class length return parameter.
        //

        *lpcbClass = AnsiString.Length;

        Error = RtlNtStatusToDosError( Status );

    } else {

        //
        // Not all of the information was succesfully queried, or Class
        // doesn't have to be converted from UNICODE to ANSI
        //

        if( ARGUMENT_PRESENT( lpcbClass ) ) {
            if( Class->Length == 0 ) {

                *lpcbClass = 0;

            } else {

                *lpcbClass = ( Class->Length >> 1 );
            }
        }
    }

    if( ARGUMENT_PRESENT( lpcSubKeys ) ) {
        *lpcSubKeys = cSubKeys;
    }
    if( ARGUMENT_PRESENT( lpcbMaxSubKeyLen ) ) {
        *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
    }
    if( ARGUMENT_PRESENT( lpcbMaxClassLen ) ) {
        *lpcbMaxClassLen = cbMaxClassLen;
    }
    if( ARGUMENT_PRESENT( lpcValues ) ) {
        *lpcValues = cValues;
    }
    if( ARGUMENT_PRESENT( lpcbMaxValueNameLen ) ) {
        *lpcbMaxValueNameLen = cbMaxValueNameLen;
    }
    if( ARGUMENT_PRESENT( lpcbMaxValueLen ) ) {
        *lpcbMaxValueLen = cbMaxValueLen;
    }
    if( ARGUMENT_PRESENT( lpcbSecurityDescriptor ) ) {
        *lpcbSecurityDescriptor = cbSecurityDescriptor;
    }
    if( ARGUMENT_PRESENT( lpftLastWriteTime ) ) {
        *lpftLastWriteTime = ftLastWriteTime;
    }

ExitCleanup:
    
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
RegQueryInfoKeyW (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for querying information about a previously
    opened key.

--*/

{
    UNICODE_STRING  Class;
    LONG            Error;

    DWORD cbClass;
    DWORD cSubKeys;
    DWORD cbMaxSubKeyLen;
    DWORD cbMaxClassLen;
    DWORD cValues;
    DWORD cbMaxValueNameLen;
    DWORD cbMaxValueLen;
    DWORD cbSecurityDescriptor;
    FILETIME ftLastWriteTime;
    HKEY        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpClass ) && ( ! ARGUMENT_PRESENT( lpcbClass )))) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    //  Make sure that the buffer size for lpClass is zero if lpClass is NULL
    //
    if( !ARGUMENT_PRESENT( lpClass ) && ARGUMENT_PRESENT( lpcbClass ) ) {
        *lpcbClass = 0;
    }

    //
    // Use the supplied class Class buffer as the buffer in a counted
    // Unicode Class.
    //
    Class.Length = 0;
    if( ARGUMENT_PRESENT( lpcbClass ) && ( *lpcbClass != 0 ) ) {

        Class.MaximumLength = ( USHORT )( *lpcbClass << 1 );
        Class.Buffer        = lpClass;

    } else {

        //
        // If the count of bytes in the class is 0, pass a NULL pointer
        // instead of what was supplied.  This ensures that RPC won't
        // attempt to copy data to a bogus pointer.
        //
        Class.MaximumLength = 0;
        Class.Buffer        = NULL;
    }

    //
    // Call the Base API.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegQueryInfoKey(
                                hKey,
                                &Class,
                                &cSubKeys,
                                &cbMaxSubKeyLen,
                                &cbMaxClassLen,
                                &cValues,
                                &cbMaxValueNameLen,
                                &cbMaxValueLen,
                                &cbSecurityDescriptor,
                                &ftLastWriteTime
                                );
    } else {

        Error = (LONG)BaseRegQueryInfoKey(
                                DereferenceRemoteHandle( hKey ),
                                &Class,
                                &cSubKeys,
                                &cbMaxSubKeyLen,
                                &cbMaxClassLen,
                                &cValues,
                                &cbMaxValueNameLen,
                                &cbMaxValueLen,
                                &cbSecurityDescriptor,
                                &ftLastWriteTime
                                );
        if (Error == ERROR_SUCCESS) {
            DWORD dwVersion;
            //
            // Check for a downlevel Win95 server, which requires
            // us to work around their BaseRegQueryInfoKey bugs.
            // They do not account for Unicode correctly.
            //
            if (IsWin95Server(DereferenceRemoteHandle(hKey),dwVersion)) {
                //
                // This is a Win95 server.
                // Double the maximum value name length and
                // maximum value data length to account for
                // the Unicode translation that Win95 forgot
                // to account for.
                //
                cbMaxValueNameLen *= sizeof(WCHAR);
                cbMaxValueLen *= sizeof(WCHAR);
            }
        }
    }

    //
    //  MaxSubKeyLen, MaxClassLen, and MaxValueNameLen should be in
    //  number of characters, without counting the NULL.
    //  Note that the server side will return the number of bytes,
    //  without counting the NUL
    //

    cbMaxSubKeyLen /= sizeof( WCHAR );
    cbMaxClassLen /= sizeof( WCHAR );
    cbMaxValueNameLen /= sizeof( WCHAR );


    if( ARGUMENT_PRESENT( lpcbClass ) ) {
        if( Class.Length == 0 ) {
            *lpcbClass = 0;
        } else {
            *lpcbClass = ( Class.Length >> 1 ) - 1;
        }
    }

    if( ARGUMENT_PRESENT( lpcSubKeys ) ) {
        *lpcSubKeys = cSubKeys;
    }
    if( ARGUMENT_PRESENT( lpcbMaxSubKeyLen ) ) {
        *lpcbMaxSubKeyLen = cbMaxSubKeyLen;
    }
    if( ARGUMENT_PRESENT( lpcbMaxClassLen ) ) {
        *lpcbMaxClassLen = cbMaxClassLen;
    }
    if( ARGUMENT_PRESENT( lpcValues ) ) {
        *lpcValues = cValues;
    }
    if( ARGUMENT_PRESENT( lpcbMaxValueNameLen ) ) {
        *lpcbMaxValueNameLen = cbMaxValueNameLen;
    }
    if( ARGUMENT_PRESENT( lpcbMaxValueLen ) ) {
        *lpcbMaxValueLen = cbMaxValueLen;
    }
    if( ARGUMENT_PRESENT( lpcbSecurityDescriptor ) ) {
        *lpcbSecurityDescriptor = cbSecurityDescriptor;
    }
    if( ARGUMENT_PRESENT( lpftLastWriteTime ) ) {
        *lpftLastWriteTime = ftLastWriteTime;
    }

ExitCleanup:
    
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
