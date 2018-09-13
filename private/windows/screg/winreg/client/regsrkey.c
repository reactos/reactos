/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regsrkey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    save/restore key APIs, that is:

        - RegRestoreKeyA
        - RegRestoreKeyW
        - RegSaveKeyA
        - RegSaveKeyW

Author:

    David J. Gilman (davegi) 23-Jan-1992

Notes:

    The RegSaveKey and RegRestoreKey APIs involve up to 3 machines:

    1.- CLIENT: The machine where the API is invoked.
    2.- SERVER: The machine where the Registry resides.
    3.- TARGET: The machine of the specified file.

    Note that both the client and the server will be running Windows NT,
    but that the target machine might not.

    Even though the target might be accessible from the client, it might
    not be accessible from the server (e.g. the share is protected).



Revision History:

    25-Mar-1992     Ramon J. San Andres (ramonsa)
                    Changed to use RPC.

--*/


#include <rpc.h>
#include "regrpc.h"
#include "client.h"




LONG
APIENTRY
RegRestoreKeyA (
    HKEY hKey,
    LPCSTR lpFile,
    DWORD dwFlags
    )

/*++

Routine Description:

    Win32 Ansi API for restoring a key.

--*/

{
    PUNICODE_STRING     FileName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;
    HKEY                TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    ASSERT( lpFile != NULL );

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }


    //
    // Convert the file name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    FileName = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( FileName != NULL );
    RtlInitAnsiString( &AnsiString, lpFile );
    Status = RtlAnsiStringToUnicodeString(
                FileName,
                &AnsiString,
                FALSE
                );

    //
    // If the file name could not be converted, map the results and return.
    //

    if( ! NT_SUCCESS( Status )) {
        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }


    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( FileName->Length > 0 ) {
        FileName->Length += sizeof( UNICODE_NULL );
    }

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegRestoreKey(
                            hKey,
                            FileName,
                            dwFlags
                            );

    } else {

        Error = (LONG)BaseRegRestoreKey(
                            DereferenceRemoteHandle( hKey ),
                            FileName,
                            dwFlags
                            );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}



LONG
APIENTRY
RegRestoreKeyW (
    HKEY hKey,
    LPCWSTR lpFile,
    DWORD dwFlags
    )

/*++

Routine Description:

    Restore the tree in the supplied file onto the key referenced by the
    supplied key handle. The restored tree will overwrite all of the
    contents of the supplied hKey except for its name. Pictorially, if
    the file contains:

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

    hKey - Supplies a handle to the key where the file is to be restored.

    lpFile - Supplies a pointer to an existing file name whose contents was
        created with RegSaveKey.

    dwFlags - Supplies an optional flag argument which can be:

                - REG_WHOLE_HIVE_VOLATILE

                    If specified this flag causes a new, volatile
                    (i.e. memory only) hive to be created. In this case
                    the hKey can only refer to a child of HKEY_USERS or
                    HKEY_LOCAL_MACHINE.

                    If not specified, hKey can refer to any key in the
                    Registry.


Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING  FileName;
    LONG            Error;
    HKEY            TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    ASSERT( lpFile != NULL );

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }


    RtlInitUnicodeString( &FileName, lpFile );


    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( FileName.Length > 0 ) {
        FileName.Length += sizeof( UNICODE_NULL );
    }

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegRestoreKey(
                                hKey,
                                &FileName,
                                dwFlags
                                );

    } else {

        Error = (LONG)BaseRegRestoreKey(
                                DereferenceRemoteHandle( hKey ),
                                &FileName,
                                dwFlags
                                );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}

LONG
APIENTRY
RegSaveKeyA (
    HKEY hKey,
    LPCSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    Win32 ANSI wrapper to RegSaveKeyW.

    Save the key (and all of its decsendants) to the non-existant file
    named by the supplied string.

Arguments:

    hKey    - Supplies a handle to the key where the save operation is to
              begin.

    lpFile  - Supplies a pointer to a non-existant file name where the tree
              rooted at the supplied key handle will be saved.

    lpSecurityAttributes - Supplies an optional pointer to a
        SECURITY_ATTRIBUTES structure for the newly created file.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    PUNICODE_STRING             FileName;
    ANSI_STRING                 AnsiString;
    NTSTATUS                    Status;
    PRPC_SECURITY_ATTRIBUTES    pRpcSA;
    RPC_SECURITY_ATTRIBUTES     RpcSA;
    LONG                        Error;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    ASSERT( lpFile != NULL );

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  Note that we must map the handle here even though RegSaveKeyW
    //  will map it again. This is so that the second map will not
    //  overwrite the static Unicode string in the TEB that will
    //  contain the file name.
    //

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }



    //
    // Convert the file name to a counted Unicode string using the static
    // Unicode string in the TEB.
    //
    FileName = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( FileName != NULL );
    RtlInitAnsiString( &AnsiString, lpFile );
    Status = RtlAnsiStringToUnicodeString(
                FileName,
                &AnsiString,
                FALSE
                );

    //
    // If the file name could not be converted, map the results and return.
    //
    if( ! NT_SUCCESS( Status )) {
        Error = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( FileName->Length > 0 ) {
        FileName->Length += sizeof( UNICODE_NULL );
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

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegSaveKey(
                                hKey,
                                FileName,
                                pRpcSA
                                );

    } else {

        Error = (LONG)BaseRegSaveKey(
                                DereferenceRemoteHandle( hKey ),
                                FileName,
                                pRpcSA
                                );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}


LONG
APIENTRY
RegSaveKeyW (
    HKEY hKey,
    LPCWSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++

Routine Description:

    Save the key (and all of its decsendants) to the non-existant file
    named by the supplied string.

Arguments:

    hKey - Supplies a handle to the key where the save operation is to
        begin.

    lpFile - Supplies a pointer to a non-existant file name where the tree
            rooted at the supplied key handle will be saved.

    lpSecurityAttributes - Supplies an optional pointer to a
        SECURITY_ATTRIBUTES structure for the newly created file.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING              FileName;
    PRPC_SECURITY_ATTRIBUTES    pRpcSA;
    RPC_SECURITY_ATTRIBUTES     RpcSA;
    LONG                        Error;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    ASSERT( lpFile != NULL );


    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    //  Note that we must map the handle here even though RegSaveKeyW
    //  will map it again. This is so that the second map will not
    //  overwrite the static Unicode string in the TEB that will
    //  contain the file name.
    //
    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }



    RtlInitUnicodeString( &FileName, lpFile );


    //
    //  Add the NULL to the length so that RPC will transmit the entire
    //  thing
    //
    if ( FileName.Length > 0 ) {
        FileName.Length += sizeof( UNICODE_NULL );
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


    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegSaveKey(
                                hKey,
                                &FileName,
                                pRpcSA
                                );

    } else {

        Error = (LONG)BaseRegSaveKey(
                                DereferenceRemoteHandle( hKey ),
                                &FileName,
                                pRpcSA
                                );
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
