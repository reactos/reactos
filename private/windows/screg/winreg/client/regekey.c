/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regekey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    enumerate key APIs.  That is:

        - RegEnumKeyA
        - RegEnumKeyW
        - RegEnumKeyExA
        - RegEnumKeyExW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regekey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

LONG
APIENTRY
RegEnumKeyA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    )

/*++

Routine Description:

    Win 3.1 ANSI RPC wrapper for enumerating keys.

--*/

{
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

    return RegEnumKeyExA (
        hKey,
        dwIndex,
        lpName,
        &cbName,
        NULL,
        NULL,
        NULL,
        NULL
        );
}

LONG
APIENTRY
RegEnumKeyW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    )

/*++

Routine Description:

    Win 3.1 Unicode RPC wrapper for enumerating keys.

--*/

{
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

    return RegEnumKeyExW (
        hKey,
        dwIndex,
        lpName,
        &cbName,
        NULL,
        NULL,
        NULL,
        NULL
        );
}

LONG
APIENTRY
RegEnumKeyExA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD  lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 ANSI API for enumerating keys.

--*/

{
    PUNICODE_STRING     Name;
    UNICODE_STRING      Class;
    WCHAR               ClassBuffer[ MAX_PATH ];
    PUNICODE_STRING     ClassPointer;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error = ERROR_SUCCESS;
    HKEY                TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Validate dependency between lpClass and lpcbClass parameters.
    //

    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpClass ) && ( ! ARGUMENT_PRESENT( lpcbClass )))) {
        return ERROR_INVALID_PARAMETER;
    }

    ConvertKey(&hKey);
    hKey = MapPredefinedHandle( hKey,&TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Use the static Unicode string in the TEB as a temporary for the
    // key's name.
    //

    Name = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( Name != NULL );
    Name->Length        = 0;

    //
    // If the class string is to be returned, initialize a UNICODE_STRING
    //

    ClassPointer           = &Class;
    ClassPointer->Length   = 0;

    if( ARGUMENT_PRESENT( lpClass )) {

        ClassPointer->MaximumLength = MAX_PATH;
        ClassPointer->Buffer        = ( PVOID ) ClassBuffer;

    } else {

        ClassPointer->MaximumLength = 0;
        ClassPointer->Buffer        = NULL;
    }




    //
    // Call the Base API passing it a pointer to the counted Unicode
    // strings for the name and class.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegEnumKey (
                            hKey,
                            dwIndex,
                            Name,
                            ClassPointer,
                            lpftLastWriteTime
                            );
    } else {

        Error = (LONG)BaseRegEnumKey (
                            DereferenceRemoteHandle( hKey ),
                            dwIndex,
                            Name,
                            ClassPointer,
                            lpftLastWriteTime
                            );
    }

    //
    // If the information was not succesfully queried return the error.
    //

    if( Error != ERROR_SUCCESS ) {
        goto ExitCleanup;
    }

    //
    //  Subtact the NULL from Length, which was added by the server
    //  so that RPC would transmit it.
    //

    Name->Length -= sizeof( UNICODE_NULL );

    if ( ClassPointer->Length > 0 ) {
        ClassPointer->Length -= sizeof( UNICODE_NULL );
    }

    //
    // Convert the name to ANSI.
    //
    // If somebody passed in a really big buffer, pretend it's
    // not quite so big so that it doesn't get truncated to zero.
    //
    if (*lpcbName > 0xFFFF) {
        AnsiString.MaximumLength    = ( USHORT ) 0xFFFF;
    } else {
        AnsiString.MaximumLength    = ( USHORT ) *lpcbName;
    }

    AnsiString.Buffer           = lpName;

    Status = RtlUnicodeStringToAnsiString(
                &AnsiString,
                Name,
                FALSE
                );

    //
    // If the name conversion failed, map and return the error.
    //

    if( ! NT_SUCCESS( Status )) {
        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    // Update the name length return parameter.
    //

    *lpcbName = AnsiString.Length;

    //
    // If requested, convert the class to ANSI.
    //

    if( ARGUMENT_PRESENT( lpClass )) {

        AnsiString.MaximumLength    = ( USHORT ) *lpcbClass;
        AnsiString.Buffer           = lpClass;

        Status = RtlUnicodeStringToAnsiString(
                    &AnsiString,
                    ClassPointer,
                    FALSE
                    );

        //
        // If the class conversion failed, map and return the error.
        //

        if( ! NT_SUCCESS( Status )) {
            Error = RtlNtStatusToDosError( Status );
            goto ExitCleanup;
        }

        //
        // If requested, return the class length parameter w/o the NUL.
        //

        if( ARGUMENT_PRESENT( lpcbClass )) {
            *lpcbClass = AnsiString.Length;
        }

    //
    // It is possible to ask for the size of the class w/o asking for the
    // class itself.
    //

    } else if( ARGUMENT_PRESENT( lpcbClass )) {
        *lpcbClass = ( ClassPointer->Length >> 1 );
    }


ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegEnumKeyExW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD  lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for enumerating keys.

--*/


{
    LONG                Error;
    UNICODE_STRING      Name;
    UNICODE_STRING      Class;
    PUNICODE_STRING     ClassPointer;
    HKEY                TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    //
    // Validate dependency between lpClass and lpcbClass parameters.
    //
    if( ARGUMENT_PRESENT( lpReserved ) ||
        (ARGUMENT_PRESENT( lpClass ) && ( ! ARGUMENT_PRESENT( lpcbClass )))) {
        return ERROR_INVALID_PARAMETER;
    }

    ConvertKey(&hKey);
    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Use the supplied name string buffer as the buffer in a counted
    // Unicode string.
    //

    Name.Length           = 0;
    if ((*lpcbName << 1) > 0xFFFE) {
        Name.MaximumLength    = ( USHORT ) 0xFFFE;
    } else {
        Name.MaximumLength    = ( USHORT )( *lpcbName << 1 );
    }
    Name.Buffer           = lpName;

    //
    // If supplied use the supplied name string buffer as the buffer in a
    // counted Unicode string.
    //
    ClassPointer        = &Class;

    if( ARGUMENT_PRESENT( lpClass )) {

        Class.Length        = 0;
        Class.MaximumLength = ( USHORT )( *lpcbClass << 1 );
        Class.Buffer        = lpClass;

    } else {

        Class.Length        = 0;
        Class.MaximumLength = 0;
        Class.Buffer        = NULL;
    }

    //
    // Call the Base API passing it a pointer to the counted Unicode
    // strings for the name and class and return the results.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegEnumKey (
                            hKey,
                            dwIndex,
                            &Name,
                            ClassPointer,
                            lpftLastWriteTime
                            );
    } else {

        Error = (LONG)BaseRegEnumKey (
                            DereferenceRemoteHandle( hKey ),
                            dwIndex,
                            &Name,
                            ClassPointer,
                            lpftLastWriteTime
                            );
    }

    //
    //  Subtact the NULL from Length, which was added by the server
    //  so that RPC would transmit it.
    //

    if ( Name.Length > 0 ) {
        Name.Length -= sizeof( UNICODE_NULL );
    }

    if ( ClassPointer->Length > 0 ) {
        ClassPointer->Length -= sizeof( UNICODE_NULL );
    }

    //
    // Return the name length parameter w/o the NUL.
    //

    if( Error == ERROR_SUCCESS ) {

        *lpcbName = ( Name.Length >> 1 );
    }

    //
    // If requested, return the class length parameter w/o the NUL.
    //

    if( ARGUMENT_PRESENT( lpcbClass )) {
        *lpcbClass = ( Class.Length >> 1 );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
