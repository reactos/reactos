/*++



Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regkey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to open, create, flush and close keys.  That is:

        - RegCloseKey
        - RegCreateKeyA
        - RegCreateKeyW
        - RegCreateKeyExA
        - RegCreateKeyExW
        - RegFlushKey
        - RegOpenKeyA
        - RegOpenKeyW
        - RegOpenKeyExA
        - RegOpenKeyExW
        - RegOverridePredefKey
        - RegOpenCurrentUser

Author:

    David J. Gilman (davegi) 15-Nov-1991

Notes:

    See the notes in server\regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

#if defined(LEAK_TRACK)
NTSTATUS TrackObject(HKEY hKey);
#endif // defined(LEAK_TRACK)

NTSTATUS DisablePredefinedHandleTable(HKEY Handle);


LONG
APIENTRY
RegCloseKey (
    IN HKEY hKey
    )

/*++

Routine Description:

    Win32 RPC wrapper for closeing a key handle.

--*/

{

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    ConvertKey(&hKey);
    if( IsPredefinedRegistryHandle( hKey )) {
        return( ClosePredefinedHandle( hKey ) );
    }

    if( IsLocalHandle( hKey )) {

        return ( LONG ) LocalBaseRegCloseKey( &hKey );

    } else {

        hKey = DereferenceRemoteHandle( hKey );
        return ( LONG ) BaseRegCloseKey( &hKey );
    }
}

LONG
APIENTRY
RegOverridePredefKey (
    IN HKEY hKey,
	IN HKEY hNewKey
    )

/*++

Routine Description:

    Win32 wrapper to override the normal value for a predefined key.

--*/

{

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    if( hKey == NULL ) {
        return ERROR_INVALID_HANDLE;
    }

    ConvertKey(&hKey);
    if( !IsPredefinedRegistryHandle( hKey )) {
        return ERROR_INVALID_HANDLE;
    }

    {
	NTSTATUS Status;

	Status = RemapPredefinedHandle( hKey, hNewKey );

	return RtlNtStatusToDosError( Status );
    }
}

LONG
APIENTRY
RegCreateKeyA (
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win 3.1 ANSI RPC wrapper for opening an existing key or creating a new one.

--*/

{
    LONG    Error;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Win3.1ism - Win 3.1 allows the predefined handle to be opened by
    // specifying a pointer to an empty or NULL string for the sub-key.
    //

    //
    // If the subkey is NULL or points to a NUL string and the handle is
    // predefined, just return the predefined handle (a virtual open)
    // otherwise it's an error.
    //

    ConvertKey(&hKey);
    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {

        if( IsPredefinedRegistryHandle( hKey )) {

            *phkResult = hKey;
            return ERROR_SUCCESS;

        } else {

            return ERROR_BADKEY;
        }
    }

    Error = (LONG)RegCreateKeyExA(
                            hKey,
                            lpSubKey,
                            0,
                            WIN31_CLASS,
                            REG_OPTION_NON_VOLATILE,
                            WIN31_REGSAM,
                            NULL,
                            phkResult,
                            NULL
                            );

    return Error;

}

LONG
APIENTRY
RegCreateKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win 3.1 Unicode RPC wrapper for opening an existing key or creating a
    new one.

--*/

{
    LONG    Error;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    ConvertKey(&hKey);
    Error =  (LONG)RegCreateKeyExW(
                            hKey,
                            lpSubKey,
                            0,
                            WIN31_CLASS,
                            REG_OPTION_NON_VOLATILE,
                            WIN31_REGSAM,
                            NULL,
                            phkResult,
                            NULL
                            );

    return Error;

}

LONG
APIENTRY
RegCreateKeyExA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for opening an existing key or creating a new one.

    RegCreateKeyExA converts the LPSECURITY_ATTRIBUTES argument to a
    RPC_SECURITY_ATTRIBUTES argument and calls BaseRegCreateKeyExA.

--*/

{
    PUNICODE_STRING             SubKey;
    UNICODE_STRING              ClassUnicode;
    PUNICODE_STRING             Class;
    ANSI_STRING                 AnsiString;
    PRPC_SECURITY_ATTRIBUTES    pRpcSA;
    RPC_SECURITY_ATTRIBUTES     RpcSA;
    NTSTATUS                    Status;
    LONG                        Error;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        OutputDebugString( "In RegCreateKeyExA\n" );
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Ensure Reserved is zero to avoid future compatability problems.
    //

    if( Reserved != 0 ) {
            return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate that the sub key is not NULL.
    //

    ASSERT( lpSubKey != NULL );
    if( ! lpSubKey ) {
        return ERROR_BADKEY;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the subkey to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    SubKey = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( SubKey != NULL );
    RtlInitAnsiString( &AnsiString, lpSubKey );
    Status = RtlAnsiStringToUnicodeString(
                SubKey,
                &AnsiString,
                FALSE
                );

    if( ! NT_SUCCESS( Status )) {
        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add size of NULL so that RPC transmits the right
    //  stuff.
    //
    SubKey->Length += sizeof( UNICODE_NULL );

    if (ARGUMENT_PRESENT( lpClass )) {

        //
        // Convert the class name to a counted Unicode string using a counted
        // Unicode string dynamically allocated by RtlAnsiStringToUnicodeString.
        //

        RtlInitAnsiString( &AnsiString, lpClass );
        Status = RtlAnsiStringToUnicodeString(
                    &ClassUnicode,
                    &AnsiString,
                    TRUE
                    );

        if( ! NT_SUCCESS( Status )) {
            Error = RtlNtStatusToDosError( Status );
            goto ExitCleanup;
        }

        Class = &ClassUnicode;
        Class->Length += sizeof( UNICODE_NULL );

    } else {

        Class = &ClassUnicode;

        Class->Length        = 0;
        Class->MaximumLength = 0;
        Class->Buffer        = NULL;
    }

    //
    // If the caller supplied a LPSECURITY_ATTRIBUTES argument, map
    // it to the RPCable version.
    //

    if( ARGUMENT_PRESENT( lpSecurityAttributes )) {

        pRpcSA = &RpcSA;

        Error = MapSAToRpcSA( lpSecurityAttributes, pRpcSA );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }

    } else {

        //
        // No PSECURITY_ATTRIBUTES argument, therefore no mapping was done.
        //

        pRpcSA = NULL;
    }

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegCreateKey (
                            hKey,
                            SubKey,
                            Class,
                            dwOptions,
                            samDesired,
                            pRpcSA,
                            phkResult,
                            lpdwDisposition
                            );
    } else {

        Error = (LONG)BaseRegCreateKey (
                            DereferenceRemoteHandle( hKey ),
                            SubKey,
                            Class,
                            dwOptions,
                            samDesired,
                            pRpcSA,
                            phkResult,
                            lpdwDisposition
                            );

        if( Error == ERROR_SUCCESS) {

            TagRemoteHandle( phkResult );
        }
    }

    //
    // Free the counted Unicode string allocated by
    // RtlAnsiStringToUnicodeString.
    //

    if (Class != NULL) {
        RtlFreeUnicodeString( Class );
    }

    //
    // Free the RPC_SECURITY_DESCRIPTOR buffer and return the
    // Registry return value.
    //

    if( pRpcSA != NULL ) {

        RtlFreeHeap(
            RtlProcessHeap( ), 0,
            pRpcSA->RpcSecurityDescriptor.lpSecurityDescriptor
            );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegCreateKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for opening an existing key or creating a new one.

    RegCreateKeyExW converts the LPSECURITY_ATTRIBUTES argument to a
    RPC_SECURITY_ATTRIBUTES argument and calls BaseRegCreateKeyExW.

--*/

{
    UNICODE_STRING              SubKey;
    UNICODE_STRING              ClassUnicode;
    PUNICODE_STRING             Class;
    PRPC_SECURITY_ATTRIBUTES    pRpcSA;
    RPC_SECURITY_ATTRIBUTES     RpcSA;
    LONG                        Error;
    PWSTR                       AuxBuffer;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Ensure Reserved is zero to avoid future compatability problems.
    //

    if( Reserved != 0 ) {
            return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate that the sub key is not NULL.
    //

    ASSERT( lpSubKey != NULL );
    if( ! lpSubKey ) {
        return ERROR_BADKEY;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    //  Add terminating NULL to Length so that RPC transmits it.
    //
    SubKey.Length += sizeof( UNICODE_NULL );

    if (ARGUMENT_PRESENT( lpClass )) {

        //
        // Convert the class name to a counted Unicode string.
        //

        RtlInitUnicodeString( &ClassUnicode, lpClass );
        Class = &ClassUnicode;
        Class->Length += sizeof( UNICODE_NULL );

    } else {

        Class = &ClassUnicode;

        Class->Length        = 0;
        Class->MaximumLength = 0;
        Class->Buffer        = NULL;
    }


    //
    // If the caller supplied a LPSECURITY_ATTRIBUTES argument, map
    // it and call the private version of the create key API.
    //

    if( ARGUMENT_PRESENT( lpSecurityAttributes )) {

        pRpcSA = &RpcSA;

        Error = MapSAToRpcSA( lpSecurityAttributes, pRpcSA );

        if( Error != ERROR_SUCCESS ) {
            goto ExitCleanup;
        }

    } else {

        //
        // No PSECURITY_ATTRIBUTES argument, therefore no mapping was done.
        //

        pRpcSA = NULL;
    }

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegCreateKey (
                            hKey,
                            &SubKey,
                            Class,
                            dwOptions,
                            samDesired,
                            pRpcSA,
                            phkResult,
                            lpdwDisposition
                            );

    } else {

        Error = (LONG)BaseRegCreateKey (
                            DereferenceRemoteHandle( hKey ),
                            &SubKey,
                            Class,
                            dwOptions,
                            samDesired,
                            pRpcSA,
                            phkResult,
                            lpdwDisposition
                            );

        if( Error == ERROR_SUCCESS) {

            TagRemoteHandle( phkResult );
        }
    }

    //
    // Free the RPC_SECURITY_DESCRIPTOR buffer and return the
    // Registry return value.
    //

    if( pRpcSA != NULL ) {

        RtlFreeHeap(
            RtlProcessHeap( ), 0,
            pRpcSA->RpcSecurityDescriptor.lpSecurityDescriptor
            );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegFlushKey (
    IN HKEY hKey
    )

/*++

Routine Description:

    Win32 RPC wrapper for flushing changes to backing store.

--*/

{
    LONG                        Error;
    HKEY                        TempHandle = NULL;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Flush is a NO-OP for HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {
        return ERROR_SUCCESS;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegFlushKey( hKey );

    } else {

        Error = (LONG)BaseRegFlushKey( DereferenceRemoteHandle( hKey ));
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegOpenKeyA (
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win 3.1 ANSI RPC wrapper for opening an existing key.

--*/

{
    LONG    Error;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    if( phkResult == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Win3.1ism - Win 3.1 allows the predefined handle to be opened by
    // specifying a pointer to an empty or NULL string for the sub-key.
    //

    //
    // If the subkey is NULL or points to a NUL string and the handle is
    // predefined, just return the predefined handle (a virtual open)
    // otherwise return the same handle that was passed in.
    //

    ConvertKey(&hKey);
    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {
        if( !IsPredefinedRegistryHandle( hKey )) {
            *phkResult = hKey;
            return ERROR_SUCCESS;
        }

/*
        if( IsPredefinedRegistryHandle( hKey )) {

            *phkResult = hKey;
            return ERROR_SUCCESS;

        } else {

            return ERROR_BADKEY;
        }
*/
    }

    Error = (LONG)RegOpenKeyExA(
                        hKey,
                        lpSubKey,
                        REG_OPTION_RESERVED,
                        WIN31_REGSAM,
                        phkResult
                        );

    return Error;

}

LONG
APIENTRY
RegOpenKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win 3.1 Unicode RPC wrapper for opening an existing key.

--*/

{

    LONG    Error;

#if DBG
    if ( BreakPointOnEntry ) {
        OutputDebugString( "In RegOpenKeyW\n" );
        DbgBreakPoint();
    }
#endif

    if( phkResult == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Win3.1ism - Win 3.1 allows the predefined handle to be opened by
    // specifying a pointer to an empty or NULL string for the sub-key.
    //

    //
    // If the subkey is NULL or points to a NUL string and the handle is
    // predefined, just return the predefined handle (a virtual open)
    // otherwise return the handle passed in.
    //

    ConvertKey(&hKey);
    if(( lpSubKey == NULL ) || ( *lpSubKey == '\0' )) {
        if( !IsPredefinedRegistryHandle( hKey )) {
            *phkResult = hKey;
            return ERROR_SUCCESS;
        }

/*
        if( IsPredefinedRegistryHandle( hKey )) {

            *phkResult = hKey;
            return ERROR_SUCCESS;

        } else {

            return ERROR_BADKEY;
        }
*/
    }

    Error = (LONG)RegOpenKeyExW(
                         hKey,
                         lpSubKey,
                         REG_OPTION_RESERVED,
                         WIN31_REGSAM,
                         phkResult
                         );

    return Error;

}

LONG
APIENTRY
RegOpenKeyExA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for opening an existing key.

    RegOpenKeyExA converts the lpSubKey argument to a counted Unicode string
    and then calls BaseRegOpenKey.

--*/

{
    PUNICODE_STRING     SubKey;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;
    CHAR                NullString;
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
    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  Caller must pass pointer to the variable where the opened handle
    //  will be returned
    //

    if( phkResult == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  If lpSubKey is NULL, then assume NUL-string as subkey name
    //

    if( lpSubKey == NULL ) {
        NullString = ( CHAR )'\0';
        lpSubKey = &NullString;
    }

    //
    //  If hKey is a predefined key, and lpSubKey is either a NULL pointer or
    //  a NUL string, close the predefined key and clear the associated entry
    //  in the PredefinedHandleTable  (RegCloseKey will do the job).
    //
    if( IsPredefinedRegistryHandle( hKey ) &&
        ( ( lpSubKey == NULL ) || ( *lpSubKey == '\0' ) ) ) {

        if ( HKEY_CLASSES_ROOT != hKey ) {
            Error = RegCloseKey( hKey );
            if( Error != ERROR_SUCCESS ) {
                return( Error );
            }
            //
            //  Create a handle and save it in the appropriate entry  in
            //  PredefinedHandleTable.
            //  Notice that the client will be impersonated.
            //  (MapPredefinedHandle will do all this stuff).
            //
            if( MapPredefinedHandle( hKey, &TempHandle ) == NULL ) {
                Error = ERROR_INVALID_HANDLE;
                goto ExitCleanup;
            }
        }

        //
        //  Return to the user the handle passed in
        //
        *phkResult = hKey;
        Error = ERROR_SUCCESS;
        goto ExitCleanup;
    }


    //
    // Validate that the sub key is not NULL.
    //

    ASSERT( lpSubKey != NULL );
    if( ! lpSubKey ) {
        Error = ERROR_BADKEY;
        goto ExitCleanup;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the subkey to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    SubKey = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( SubKey != NULL );
    RtlInitAnsiString( &AnsiString, lpSubKey );
    Status = RtlAnsiStringToUnicodeString(
                SubKey,
                &AnsiString,
                FALSE
                );

    if( ! NT_SUCCESS( Status )) {
        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add terminating NULL to Length so that RPC transmits it.
    //
    SubKey->Length += sizeof( UNICODE_NULL );


    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegOpenKey (
                            hKey,
                            SubKey,
                            dwOptions,
                            samDesired,
                            phkResult
                            );
    } else {

        Error = (LONG)BaseRegOpenKey (
                            DereferenceRemoteHandle( hKey ),
                            SubKey,
                            dwOptions,
                            samDesired,
                            phkResult
                            );

        if( Error == ERROR_SUCCESS) {

            TagRemoteHandle( phkResult );
        }
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegOpenKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for opening an existing key.

    RegOpenKeyExW converts the lpSubKey argument to a counted Unicode string
    and then calls BaseRegOpenKey.

--*/

{
    UNICODE_STRING      SubKey;
    LONG                Error;
    WCHAR               NullString;
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
    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  Caller must pass pointer to the variable where the opened handle
    //  will be returned
    //

    if( phkResult == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  If lpSubKey is NULL, then assume NUL-string as subkey name
    //

    if( lpSubKey == NULL ) {
        NullString = UNICODE_NULL;
        lpSubKey = &NullString;
    }

    //
    //  If hKey is a predefined key, and lpSubKey is either a NULL pointer or
    //  a NUL string, close the predefined key and clear the associated entry
    //  in the PredefinedHandleTable  (RegCloseKey will do the job).
    //
    if( IsPredefinedRegistryHandle( hKey ) &&
        ( ( lpSubKey == NULL ) || ( *lpSubKey == ( WCHAR )'\0' ) ) ) {

        if ( HKEY_CLASSES_ROOT != hKey ) {
            Error = RegCloseKey( hKey );
            if( Error != ERROR_SUCCESS ) {
                return( Error );
            }
            //
            //  Create a handle and save it in the appropriate entry  in
            //  PredefinedHandleTable.
            //  Notice that the client will be impersonated.
            //  (MapPredefinedHandle will do all this stuff).
            //
            if( MapPredefinedHandle( hKey, &TempHandle ) == NULL ) {
                Error = ERROR_INVALID_HANDLE;
                goto ExitCleanup;
            }
        }

        //
        //  Return to the user the handle passed in
        //
        *phkResult = hKey;
        Error = ERROR_SUCCESS;
        goto ExitCleanup;
    }

    //
    // Validate that the sub key is not NULL.
    //

    ASSERT( lpSubKey != NULL );
    if( ! lpSubKey ) {
        Error = ERROR_BADKEY;
        goto ExitCleanup;
    }


    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    //  Add terminating NULL to Length so that RPC transmits it
    //
    SubKey.Length += sizeof (UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Error =  (LONG)LocalBaseRegOpenKey (
                            hKey,
                            &SubKey,
                            dwOptions,
                            samDesired,
                            phkResult
                            );
    } else {

        Error =  (LONG)BaseRegOpenKey (
                            DereferenceRemoteHandle( hKey ),
                            &SubKey,
                            dwOptions,
                            samDesired,
                            phkResult
                            );

        if( Error == ERROR_SUCCESS) {

            TagRemoteHandle( phkResult );
        }
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegOpenCurrentUser(
    REGSAM samDesired,
    PHKEY phkResult
    )
/*++

Routine Description:

    Win32 Client-Only function to open the key for HKEY_CURRENT_USER
    for the user that the thread is currently impersonating.  Since
    HKEY_CURRENT_USER is cached for all threads in a process, if the
    process is impersonating multiple users, this allows access to
    the appropriate key.

Arguments:

    samDesired - Supplies the requested security access mask.

    phkResult - Returns an open handle to the key.

Return Value:

    Returns 0 (ERROR_SUCCESS) for success, otherwise a windows error code.

--*/
{
    NTSTATUS Status ;

    Status = RtlOpenCurrentUser( samDesired, phkResult );

#if defined(LEAK_TRACK)

    if (NT_SUCCESS(Status)) {
        if (g_RegLeakTraceInfo.bEnableLeakTrack) {
            (void) TrackObject(*phkResult);
        }
    }
    
#endif // (LEAK_TRACK)

    return RtlNtStatusToDosError( Status );

}

LONG
APIENTRY
RegDisablePredefinedCache(
    )
/*++

Routine Description:

    Win32 Client-Only function to disable the predefined handle table
    for HKEY_CURRENT_USER for the calling process
    All references to HKEY_CURRENT_USER after this function is called
    will result in a open/close on HKU\<sid>

Arguments:


Return Value:

    Returns 0 (ERROR_SUCCESS) for success, otherwise a windows error code.

--*/
{
    NTSTATUS Status ;

    Status = DisablePredefinedHandleTable( HKEY_CURRENT_USER );

    return RtlNtStatusToDosError( Status );
}

