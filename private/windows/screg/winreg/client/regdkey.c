/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regdkey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to delete a key.  That is:

        - RegDeleteKeyA
        - RegDeleteKeyW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regdkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"


LONG
APIENTRY
RegDeleteKeyA (
    HKEY hKey,
    LPCSTR lpKeyName
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for deleting a Key.

    RegDeleteKeyA converts the lpKeyName argument to a counted Unicode string
    and then calls BaseRegDeleteKey.

--*/

{
    PUNICODE_STRING     KeyName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    HKEY                TempHandle = NULL;
    LONG                Result;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle);
    if( hKey == NULL ) {
        Result = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the Key name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    KeyName = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( KeyName != NULL );
    RtlInitAnsiString( &AnsiString, lpKeyName );
    Status = RtlAnsiStringToUnicodeString(
                KeyName,
                &AnsiString,
                FALSE
                );

    if( ! NT_SUCCESS( Status )) {
        Result = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add terminating NULL to Length so that RPC transmits it
    //
    KeyName->Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Result = (LONG)LocalBaseRegDeleteKey (
                    hKey,
                    KeyName
                    );
    } else {

        Result = (LONG)BaseRegDeleteKey (
                    DereferenceRemoteHandle( hKey ),
                    KeyName
                    );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Result;
}

LONG
APIENTRY
RegDeleteKeyW (
    HKEY hKey,
    LPCWSTR lpKeyName
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for deleting a Key.

    RegDeleteKeyW converts the lpKeyName argument to a counted Unicode string
    and then calls BaseRegDeleteKey.

--*/

{
    UNICODE_STRING      KeyName;
    HKEY                TempHandle = NULL;
    LONG                Result;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    // ASSERT( hKey != NULL );
    if( hKey == NULL ) {
        Result = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the Key name to a counted Unicode string.
    //

    RtlInitUnicodeString( &KeyName, lpKeyName );

    //
    //  Add terminating NULL to Length so that RPC transmits it
    //

    KeyName.Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Result = (LONG)LocalBaseRegDeleteKey (
                    hKey,
                    &KeyName
                    );
    } else {

        Result = (LONG)BaseRegDeleteKey (
                    DereferenceRemoteHandle( hKey ),
                    &KeyName
                    );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Result;
}
