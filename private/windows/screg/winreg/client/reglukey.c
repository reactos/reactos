/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Reglukey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to load, unload and replace keys. That is:

        - RegLoadKeyA
        - RegLoadKeyW
        - RegUnLoadKeyA
        - RegUnLoadKeyW
        - RegReplaceKeyA
        - RegReplaceKeyW

Author:


    Ramon J. San Andres (ramonsa) 16-Apr-1992



--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"


LONG
APIENTRY
RegLoadKeyA(
    HKEY   hKey,
    LPCSTR  lpSubKey,
    LPCSTR  lpFile
    )

/*++

Routine Description:

    Win32 Ansi API for loading a key.

--*/

{

    HKEY                Handle;
    PUNICODE_STRING     SubKey;
    UNICODE_STRING      File;
    WCHAR               UnicodeBuffer[ MAX_PATH ];
    ANSI_STRING         AnsiSubKey;
    ANSI_STRING         AnsiFile;
    NTSTATUS            NtStatus;
    LONG                Error;
    HKEY                TempHandle = NULL;

#if DBG
    // OutputDebugString( "Winreg: Entering RegLoadKeyA\n" );
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

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the SubKey name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //
    SubKey = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( SubKey != NULL );
    RtlInitAnsiString( &AnsiSubKey, lpSubKey );
    NtStatus = RtlAnsiStringToUnicodeString(
                    SubKey,
                    &AnsiSubKey,
                    FALSE
                    );

    //
    // If the SubKey name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey->Length > 0 ) {
        SubKey->Length += sizeof( UNICODE_NULL );
    }


    //
    // Convert the file name to a counted Unicode string using the
    // Unicode string on the stack.
    //
    File.Buffer        = UnicodeBuffer;
    File.MaximumLength = sizeof( UnicodeBuffer );
    RtlInitAnsiString( &AnsiFile, lpFile );
    NtStatus = RtlAnsiStringToUnicodeString(
                    &File,
                    &AnsiFile,
                    FALSE
                    );

    //
    // If the file name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( File.Length > 0 ) {
        File.Length += sizeof( UNICODE_NULL );
    }

    //
    // Call the server
    //

    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegLoadKey(
                            Handle,
                            SubKey,
                            &File
                            );

    } else {

        Error = (LONG)BaseRegLoadKey(
                            DereferenceRemoteHandle( Handle ),
                            SubKey,
                            &File
                            );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}




LONG
APIENTRY
RegLoadKeyW(
    HKEY    hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpFile
    )

/*++

Routine Description:

    Load the tree in the supplied file into the key referenced by the
    supplied key handle and sub-key.  The loaded tree will overwrite all
    of the contents of the supplied sub-key except for its name.
    Pictorially, if the file contains:

                    A
                   / \
                  /   \
                 B     C

    and the supplied key refers to a key name X, the resultant tree would
    look like:

                    X
                   / \
                  /   \
                 B     C

Arguments:

    hKey - Supplies the predefined handle HKEY_USERS or HKEY_LOCAL_MACHINE.
        lpSubKey is relative to this handle.

    lpSubKey - Supplies a path name to a new (i.e.  non-existant) key
        where the supplied file will be loaded.

    lpFile - Supplies a pointer to an existing file name whose contents was
        created with RegSaveKey. The file name may not have an extension.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    The difference between RegRestoreKey and RegLoadKey is that in the
    latter case the supplied file is used as the actual backing store
    whereas in the former case the information in the file is copied into
    the Registry.

    RegLoadKey requires SeRestorePrivilege.

--*/

{

    HKEY                Handle;
    UNICODE_STRING      SubKey;
    UNICODE_STRING      File;
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

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }


    RtlInitUnicodeString(
            &SubKey,
            lpSubKey
            );

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey.Length > 0 ) {
        SubKey.Length += sizeof( UNICODE_NULL );
    }


    RtlInitUnicodeString(
            &File,
            lpFile
            );

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( File.Length > 0 ) {
        File.Length += sizeof( UNICODE_NULL );
    }


    //
    // Call the server
    //

    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegLoadKey(
                            Handle,
                            &SubKey,
                            &File
                            );

    } else {

        Error = (LONG)BaseRegLoadKey(
                            DereferenceRemoteHandle( Handle ),
                            &SubKey,
                            &File
                            );
    }
ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}




LONG
APIENTRY
RegUnLoadKeyA(
    HKEY   hKey,
    LPCSTR  lpSubKey
    )
/*++

Routine Description:

    Win32 Ansi API for unloading a key.

--*/

{

    HKEY                Handle;
    PUNICODE_STRING     SubKey;
    ANSI_STRING         AnsiSubKey;
    NTSTATUS            NtStatus;
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

    Handle = MapPredefinedHandle( hKey,&TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }



    //
    // Convert the SubKey name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //
    SubKey = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( SubKey != NULL );
    RtlInitAnsiString( &AnsiSubKey, lpSubKey );
    NtStatus = RtlAnsiStringToUnicodeString(
                    SubKey,
                    &AnsiSubKey,
                    FALSE
                    );

    //
    // If the SubKey name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey->Length > 0 ) {
        SubKey->Length += sizeof( UNICODE_NULL );
    }


    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegUnLoadKey(
                                Handle,
                                SubKey
                                );

    } else {

        Error = (LONG)BaseRegUnLoadKey(
                                DereferenceRemoteHandle( Handle ),
                                SubKey
                                );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}




LONG
APIENTRY
RegUnLoadKeyW(
    HKEY   hKey,
    LPCWSTR lpSubKey
    )

/*++

Routine Description:

    Unload the specified tree (hive) from the Registry.

Arguments:

    hKey - Supplies a handle to an open key. lpSubKey is relative to this
        handle.

    lpSubKey - Supplies a path name to the key that is to be unloaded.
        The combination of hKey and lpSubKey must refer to a hive in the
        Registry created with RegRestoreKey or RegLoadKey.  This parameter may
        be NULL.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    RegUnLoadKey requires SeRestorePrivilege.

--*/

{
    HKEY                Handle;
    UNICODE_STRING      SubKey;
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

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }


    RtlInitUnicodeString(
            &SubKey,
            lpSubKey
            );

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey.Length > 0 ) {
        SubKey.Length += sizeof( UNICODE_NULL );
    }


    //
    // Call the server
    //
    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegUnLoadKey(
                                Handle,
                                &SubKey
                                );

    } else {

        Error = (LONG)BaseRegUnLoadKey(
                                DereferenceRemoteHandle( Handle ),
                                &SubKey
                                );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}






LONG
APIENTRY
RegReplaceKeyA(
    HKEY   hKey,
    LPCSTR  lpSubKey,
    LPCSTR  lpNewFile,
    LPCSTR  lpOldFile
    )
/*++

Routine Description:

    Win32 Ansi API for replacing a key.

--*/
{
    HKEY                Handle;
    PUNICODE_STRING     SubKey;
    UNICODE_STRING      NewFile;
    UNICODE_STRING      OldFile;
    WCHAR               NewUnicodeBuffer[ MAX_PATH ];
    WCHAR               OldUnicodeBuffer[ MAX_PATH ];
    ANSI_STRING         AnsiSubKey;
    ANSI_STRING         AnsiFile;
    NTSTATUS            NtStatus;
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

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the SubKey name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //
    SubKey = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( SubKey != NULL );
    RtlInitAnsiString( &AnsiSubKey, lpSubKey );
    NtStatus = RtlAnsiStringToUnicodeString(
                    SubKey,
                    &AnsiSubKey,
                    FALSE
                    );

    //
    // If the SubKey name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }


    //
    // Convert the new file name to a counted Unicode string using the
    // Unicode string on the stack.
    //
    NewFile.Buffer        = NewUnicodeBuffer;
    NewFile.MaximumLength = sizeof( NewUnicodeBuffer );
    RtlInitAnsiString( &AnsiFile, lpNewFile );
    NtStatus = RtlAnsiStringToUnicodeString(
                    &NewFile,
                    &AnsiFile,
                    FALSE
                    );

    //
    // If the file name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }


    //
    // Convert the old file name to a counted Unicode string using the
    // Unicode string on the stack.
    //
    OldFile.Buffer        = OldUnicodeBuffer;
    OldFile.MaximumLength = sizeof( OldUnicodeBuffer );
    RtlInitAnsiString( &AnsiFile, lpOldFile );
    NtStatus = RtlAnsiStringToUnicodeString(
                    &OldFile,
                    &AnsiFile,
                    FALSE
                    );

    //
    // If the file name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( NtStatus )) {
        Error = RtlNtStatusToDosError( NtStatus );
        goto ExitCleanup;
    }

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey->Length > 0 ) {
        SubKey->Length += sizeof( UNICODE_NULL );
    }

    if ( NewFile.Length > 0 ) {
        NewFile.Length += sizeof( UNICODE_NULL );
    }

    if ( OldFile.Length > 0 ) {
        OldFile.Length += sizeof( UNICODE_NULL );
    }

    //
    //  Call the server
    //

    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegReplaceKey(
                                Handle,
                                SubKey,
                                &NewFile,
                                &OldFile
                                );

    } else {

        Error = (LONG)BaseRegReplaceKey(
                                DereferenceRemoteHandle( Handle ),
                                SubKey,
                                &NewFile,
                                &OldFile
                                );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}




LONG
APIENTRY
RegReplaceKeyW(
    HKEY    hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpNewFile,
    LPCWSTR  lpOldFile
    )

/*++

Routine Description:

    Replace an existing tree (hive) in the Registry. The new tree will
    take effect the next time the system is rebooted.

Arguments:

    hKey - Supplies a handle to an open key. lpSubKey is relative to this
        handle.

    lpSubKey - Supplies a path name to the key that is to be replaced.
        The combination of hKey and lpSubKey must refer to a hive in the
        Registry.  This parameter may be NULL.

    lpNewFile - Supplies a file name for the new hive file.

    lpOldFile - Supplies a backup file name for the old (existing) hive file.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    lpNewFile will remain open until after the system is rebooted.

    RegUnLoadKey requires SeRestorePrivilege.

--*/

{

    HKEY    Handle;
    UNICODE_STRING      SubKey;
    UNICODE_STRING      NewFile;
    UNICODE_STRING      OldFile;
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

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }



    RtlInitUnicodeString(
                &SubKey,
                lpSubKey
                );

    RtlInitUnicodeString(
                &NewFile,
                lpNewFile
                );

    RtlInitUnicodeString(
                &OldFile,
                lpOldFile
                );


    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( SubKey.Length > 0 ) {
        SubKey.Length += sizeof( UNICODE_NULL );
    }

    if ( NewFile.Length > 0 ) {
        NewFile.Length += sizeof( UNICODE_NULL );
    }

    if ( OldFile.Length > 0 ) {
        OldFile.Length += sizeof( UNICODE_NULL );
    }


    //
    //  Call the server
    //

    if( IsLocalHandle( Handle )) {

        Error = (LONG)LocalBaseRegReplaceKey(
                                Handle,
                                &SubKey,
                                &NewFile,
                                &OldFile
                                );

    } else {

        Error = (LONG)BaseRegReplaceKey(
                                DereferenceRemoteHandle( Handle ),
                                &SubKey,
                                &NewFile,
                                &OldFile
                                );
    }

ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
