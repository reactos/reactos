/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regsval.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    set value APIs. That is:

        - RegSetValueA
        - RegSetValueW
        - RegSetValueExA
        - RegSetValueExW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regsval.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#include <string.h>


LONG
RegSetValueA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwType,
    LPCSTR lpData,
    DWORD cbData
    )

/*++

Routine Description:

    Win 3.1 ANSI RPC wrapper for setting a value.

--*/

{
    HKEY        ChildKey;
    LONG        Error;
    HKEY        TempHandle = NULL;

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

    //
    // Check the value type for compatability w/Win 3.1
    //

    if( dwType != REG_SZ ) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Win3.1ism - Win 3.1 ignores the cbData parameter so it is computed
    // here instead as the length of the string plus the NUL character.
    //

    cbData = strlen( lpData ) + 1;


    //
    // If the sub-key is NULL or points to an empty string then the value is
    // set in this key (i.e.  hKey) otherwise the sub-key needs to be
    // opened/created.
    //

    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {

        ChildKey = hKey;

    } else {

        //
        // The sub-key was supplied so attempt to open/create it.
        //

        Error = RegCreateKeyExA(
                    hKey,
                    lpSubKey,
                    0,
                    WIN31_CLASS,
                    0,
                    KEY_SET_VALUE,
                    NULL,
                    &ChildKey,
                    NULL
                    );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }
    }

    //
    // ChildKey contains an HKEY which may be the one supplied (hKey) or
    // returned from RegCreateKeyA. Set the value using the special value
    // name NULL.
    //

    Error = RegSetValueExA(
                ChildKey,
                NULL,
                0,
                dwType,
                lpData,
                cbData
                );

    //
    // If the sub key was opened, close it.
    //

    if( ChildKey != hKey ) {

        Error = RegCloseKey( ChildKey );
        ASSERT( Error == ERROR_SUCCESS );
    }

    //
    // Return the results of setting the value.
    //

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
RegSetValueW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    )

/*++

Routine Description:

    Win 3.1 Unicode RPC wrapper for setting a value.

--*/

{
    HKEY        ChildKey;
    LONG        Error;
    HKEY        TempHandle = NULL;

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

    //
    // Check the value type for compatability w/Win 3.1
    //

    if( dwType != REG_SZ ) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle);
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Win3.1ism - Win 3.1 ignores the cbData parameter so it is computed
    // here instead as the length of the string plus the UNICODE_NUL
    // character.
    //

    cbData = wcslen( lpData ) * sizeof( WCHAR ) + sizeof( UNICODE_NULL );;

    //
    // If the sub-key is NULL or points to an empty string then the value is
    // set in this key (i.e.  hKey) otherwise the sub-key needs to be
    // opened/created.
    //

    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {

        ChildKey = hKey;

    } else {

        //
        // The sub-key was supplied attempt to open/create it.
        //

        Error = RegCreateKeyExW(
                    hKey,
                    lpSubKey,
                    0,
                    WIN31_CLASS,
                    0,
                    KEY_SET_VALUE,
                    NULL,
                    &ChildKey,
                    NULL
                    );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }
    }

    //
    // ChildKey contains an HKEY which may be the one supplied (hKey) or
    // returned from RegCreateKeyW. Set the value using the special value
    // name NULL.
    //

    Error = RegSetValueExW(
                ChildKey,
                NULL,
                0,
                dwType,
                ( LPBYTE ) lpData,
                cbData
                );

    //
    // If the sub key was opened/created, close it.
    //

    if( ChildKey != hKey ) {

        RegCloseKey( ChildKey );
    }

    //
    // Return the results of querying the value.
    //

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegSetValueExA (
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for setting a value.

    RegSetValueExA converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegSetValue.

--*/

{
    PUNICODE_STRING     ValueName;
    UNICODE_STRING      UnicodeString;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LPBYTE              ValueData;

    PSTR                AnsiValueBuffer;
    ULONG               AnsiValueLength;
    PWSTR               UnicodeValueBuffer;
    ULONG               UnicodeValueLength;
    ULONG               Index;

    LONG                Error;
    HKEY                TempHandle = NULL;

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

    //
    // Ensure Reserved is zero to avoid future compatability problems.
    //

    if( Reserved != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the value name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //
    if ( lpValueName ) {

        ValueName = &NtCurrentTeb( )->StaticUnicodeString;
        ASSERT( ValueName != NULL );
        RtlInitAnsiString( &AnsiString, lpValueName );
        Status = RtlAnsiStringToUnicodeString(
                    ValueName,
                    &AnsiString,
                    FALSE
                    );

        if( ! NT_SUCCESS( Status )) {
            Error = RtlNtStatusToDosError( Status );
            goto ExitCleanup;
        }


        //
        //  Add the NULL to the Length, so that RPC will transmit it
        //
        ValueName->Length += sizeof( UNICODE_NULL );

    } else {

        //
        //  No name was passed. Use our internal UNICODE string
        //  and set its fields to NULL. We don't use the static
        //  unicode string in the TEB in this case because we
        //  can't mess with its Buffer and MaximumLength fields.
        //
        ValueName = &UnicodeString;
        ValueName->Length           = 0;
        ValueName->MaximumLength    = 0;
        ValueName->Buffer           = NULL;
    }

    //
    // If type is one of the null terminated string types, then do the ANSI to
    // UNICODE translation into an allocated buffer.
    //
    ValueData = ( LPBYTE )lpData;
    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ || dwType == REG_MULTI_SZ) {

        //
        // Special hack to help out all the idiots who
        // believe the length of a NULL terminated string is
        // strlen(foo) instead of strlen(foo) + 1.
        //
        if ((cbData > 0) &&
            (lpData[cbData-1] != 0)) {
            //
            // Do this under an exception handler in case the last
            // little bit crosses a page boundary.
            //
            try {
                if (lpData[cbData] == 0) {
                    cbData += 1;        // increase string length to account for NULL terminator
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                ; // guess they really really did not want a NULL terminator

            }
        }
        AnsiValueBuffer        = ValueData;
        AnsiValueLength        = cbData;

        UnicodeValueLength = cbData * sizeof( WCHAR );
        UnicodeValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                              UnicodeValueLength
                                            );
        if (UnicodeValueBuffer == NULL) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            Status = RtlMultiByteToUnicodeN( UnicodeValueBuffer,
                                             UnicodeValueLength,
                                             &Index,
                                             AnsiValueBuffer,
                                             AnsiValueLength
                                           );
            if (!NT_SUCCESS( Status )) {
                Error = RtlNtStatusToDosError( Status );
            } else {
                ValueData   = (LPBYTE)UnicodeValueBuffer;
                cbData      = Index;
                Error       = ERROR_SUCCESS;
            }
        }
    } else {
        Error = ERROR_SUCCESS;
    }

    if ( Error == ERROR_SUCCESS ) {

        //
        // Call the Base API, passing it the supplied parameters and the
        // counted Unicode strings.
        //

        if( IsLocalHandle( hKey )) {

            Error =  (LONG)LocalBaseRegSetValue (
                                hKey,
                                ValueName,
                                dwType,
                                ValueData,
                                cbData
                                );

        } else {

            Error =  (LONG)BaseRegSetValue (
                                DereferenceRemoteHandle( hKey ),
                                ValueName,
                                dwType,
                                ValueData,
                                cbData
                                );
        }
    }

    if( ValueData != lpData ) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValueBuffer );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegSetValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for setting a value.

    RegSetValueExW converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegSetValue.

--*/

{
    UNICODE_STRING      ValueName;
    UNALIGNED WCHAR *String;
    DWORD StringLength;
    LONG                Error;
    HKEY                TempHandle = NULL;

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

    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(PerfRegSetValue(hKey,
                               ( LPWSTR )lpValueName,
                               Reserved,
                               dwType,
                               ( LPBYTE )lpData,
                               cbData));
    }

    //
    // Ensure Reserved is zero to avoid future compatability problems.
    //

    if( Reserved != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Special hack to help out all the idiots who
    // believe the length of a NULL terminated string is
    // strlen(foo) instead of strlen(foo) + 1.
    //
    String = (UNALIGNED WCHAR *)lpData;
    StringLength = cbData/sizeof(WCHAR);
    if (((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ) || (dwType == REG_MULTI_SZ)) &&
        (StringLength > 0) &&
        (String[StringLength-1] != 0)) {
        //
        // Do this under an exception handler in case the last
        // little bit crosses a page boundary.
        //
        try {
            if (String[StringLength] == 0) {
                cbData += sizeof(WCHAR);        // increase string length to account for NULL terminator
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ; // guess they really really did not want a NULL terminator

        }
    }

    //
    // Convert the value name to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    //
    //  Add the NULL to the Length, so that RPC will transmit it
    //
    ValueName.Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegSetValue (
                        hKey,
                        &ValueName,
                        dwType,
                        ( LPBYTE )lpData,
                        cbData
                        );

    } else {

        Error = (LONG)BaseRegSetValue (
                        DereferenceRemoteHandle( hKey ),
                        &ValueName,
                        dwType,
                        ( LPBYTE )lpData,
                        cbData
                        );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
