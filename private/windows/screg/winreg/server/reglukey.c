/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Reglukey.c

Abstract:

    This module contains the server side Win32 Registry
    APIs to load, unload and replace keys. That is:

        - BaseRegLoadKeyA
        - BaseRegLoadKeyW
        - BaseRegUnLoadKeyA
        - BaseRegUnLoadKeyW
        - BaseRegReplaceKeyA
        - BaseRegReplaceKeyW

Author:


    Ramon J. San Andres (ramonsa) 16-Apr-1992



--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"

error_status_t
BaseRegLoadKey(
    IN  HKEY            hKey,
    IN  PUNICODE_STRING lpSubKey OPTIONAL,
    IN  PUNICODE_STRING lpFile
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
    OBJECT_ATTRIBUTES   ObjaKey;
    OBJECT_ATTRIBUTES   ObjaFile;
    BOOLEAN             ErrorFlag;
    UNICODE_STRING      FileName;
    RTL_RELATIVE_NAME   RelativeName;
    PVOID               FreeBuffer;
    NTSTATUS            NtStatus;
    PUNICODE_STRING     SubKey;

#if DBG
    //OutputDebugString( "WINREG: Entering BaseRegLoadKey\n" );
#endif


    ASSERT( (hKey != NULL) && (lpFile != NULL) && (lpFile->Buffer != NULL));
    if ( (hKey == NULL) || (lpFile == NULL) || (lpFile->Buffer == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // check for oddly formed UNICODE_STRINGs passed by malicious clients
    // check also for zero-length strings
    //
    if ((!lpFile->Length)    ||
        (lpFile->Length & 1) ||
        (lpFile->Buffer[(lpFile->Length-1)/sizeof(WCHAR)] != UNICODE_NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    if ((lpSubKey) &&
        ((!lpSubKey->Length)    ||
         (lpSubKey->Length & 1) ||
         (lpSubKey->Buffer[(lpSubKey->Length-1)/sizeof(WCHAR)] != UNICODE_NULL))) {
        return ERROR_INVALID_PARAMETER;
    }

    RPC_IMPERSONATE_CLIENT( NULL );


    //
    //  Remove terminating NULLs from Length counts. These were added
    //  on the client side so that RPC would transmit the whole thing.
    //
    if ( lpSubKey && lpSubKey->Length > 0 ) {
        lpSubKey->Length -= sizeof( UNICODE_NULL );
        SubKey = lpSubKey;
    } else {
        SubKey = NULL;
    }

    if ( lpFile->Length > 0 ) {
        lpFile->Length -= sizeof( UNICODE_NULL );
    }


    InitializeObjectAttributes(
                &ObjaKey,
                SubKey,
                OBJ_CASE_INSENSITIVE,
                hKey,
                NULL
                );

    //
    // Convert the DOS path name to a canonical Nt path name.
    //
    ErrorFlag = RtlDosPathNameToNtPathName_U(
                    lpFile->Buffer,
                    &FileName,
                    NULL,
                    &RelativeName
                    );

    //
    // If the name was not succesfully converted assume it was invalid.
    //
    if ( !ErrorFlag ) {
        RPC_REVERT_TO_SELF();
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Remember the buffer allocatted by RtlDosPathNameToNtPathName_U.
    //
    FreeBuffer = FileName.Buffer;

    //
    // If a relative name and directory handle will work, use those.
    //
    if ( RelativeName.RelativeName.Length ) {

        //
        // Replace the full path with the relative path.
        //
        FileName = *( PUNICODE_STRING ) &RelativeName.RelativeName;

    } else {

        //
        // Using the full path - no containing directory.
        //
        RelativeName.ContainingDirectory = NULL;
    }

    //
    // Initialize the Obja structure for the file.
    //
    InitializeObjectAttributes(
            &ObjaFile,
            &FileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );

#if DBG
    //OutputDebugString( "WINREG: Before NtLoadKey\n" );
#endif


    NtStatus = NtLoadKey(
                    &ObjaKey,
                    &ObjaFile
                    );

#if DBG
    //OutputDebugString( "WINREG: After RegLoadKey\n" );
#endif

    RPC_REVERT_TO_SELF();

    //
    // Free the buffer allocatted by RtlDosPathNameToNtPathName_U.
    //
    RtlFreeHeap( RtlProcessHeap( ), 0, FreeBuffer );

#if DBG
    //OutputDebugString( "WINREG: Leaving BaseRegLoadKey\n" );
#endif

    return (error_status_t)RtlNtStatusToDosError( NtStatus );
}




error_status_t
BaseRegUnLoadKey(
    IN  HKEY            hKey,
    IN  PUNICODE_STRING lpSubKey OPTIONAL
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

    OBJECT_ATTRIBUTES   ObjaKey;
    NTSTATUS            NtStatus;


    ASSERT( hKey != NULL );
    if ( hKey == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }


    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Remove terminating NULLs from Length counts. These were added
    //  on the client side so that RPC would transmit the whole thing.
    //
    if ( lpSubKey && lpSubKey->Length > 0 ) {
        lpSubKey->Length -= sizeof( UNICODE_NULL );
    }


    InitializeObjectAttributes(
                &ObjaKey,
                lpSubKey,
                OBJ_CASE_INSENSITIVE,
                hKey,
                NULL
                );

    NtStatus = NtUnloadKey( &ObjaKey );

    RPC_REVERT_TO_SELF();
    return (error_status_t)RtlNtStatusToDosError( NtStatus );
}









error_status_t
BaseRegReplaceKey(
    HKEY             hKey,
    PUNICODE_STRING  lpSubKey,
    PUNICODE_STRING  lpNewFile,
    PUNICODE_STRING  lpOldFile
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

    UNICODE_STRING      NewFileName;
    UNICODE_STRING      OldFileName;
    RTL_RELATIVE_NAME   RelativeName;
    PVOID               NewFreeBuffer;
    PVOID               OldFreeBuffer;
    HANDLE              HiveHandle;
    OBJECT_ATTRIBUTES   ObjaKey;
    OBJECT_ATTRIBUTES   ObjaNewFile;
    OBJECT_ATTRIBUTES   ObjaOldFile;
    BOOLEAN             ErrorFlag;
    NTSTATUS            NtStatus;


    ErrorFlag = (BOOLEAN)( (hKey   == NULL)            ||
                           (lpNewFile == NULL)         ||
                           (lpNewFile->Buffer == NULL) ||
                           (lpOldFile == NULL)         ||
                           (lpOldFile->Buffer == NULL) );

    ASSERT( !ErrorFlag );

    if ( ErrorFlag ) {
        return ERROR_INVALID_PARAMETER;
    }


    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Remove terminating NULLs from Length counts. These were added
    //  on the client side so that RPC would transmit the whole thing.
    //
    if ( lpSubKey && lpSubKey->Length > 0 ) {
        lpSubKey->Length -= sizeof( UNICODE_NULL );
    }

    if ( lpNewFile->Length > 0 ) {
        lpNewFile->Length -= sizeof( UNICODE_NULL );
    }

    if ( lpOldFile->Length > 0 ) {
        lpOldFile->Length -= sizeof( UNICODE_NULL );
    }


    InitializeObjectAttributes(
                &ObjaKey,
                lpSubKey,
                OBJ_CASE_INSENSITIVE,
                hKey,
                NULL
                );

    //
    //  Get a handle to the hive root
    //
    NtStatus = NtCreateKey(
                    &HiveHandle,
                    MAXIMUM_ALLOWED,
                    &ObjaKey,
                    0,
                    NULL,
                    REG_OPTION_BACKUP_RESTORE,
                    NULL
                    );


    if ( !NT_SUCCESS( NtStatus ) ) {
        RPC_REVERT_TO_SELF();
        return (error_status_t)RtlNtStatusToDosError( NtStatus );
    }



    //
    // Convert the new DOS path name to a canonical Nt path name.
    //
    ErrorFlag = RtlDosPathNameToNtPathName_U(
                    lpNewFile->Buffer,
                    &NewFileName,
                    NULL,
                    &RelativeName
                    );

    //
    // If the name was not succesfully converted assume it was invalid.
    //
    if ( !ErrorFlag ) {
        NtClose( HiveHandle );
        RPC_REVERT_TO_SELF();
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Remember the buffer allocatted by RtlDosPathNameToNtPathName_U.
    //
    NewFreeBuffer = NewFileName.Buffer;

    //
    // If a relative name and directory handle will work, use those.
    //
    if ( RelativeName.RelativeName.Length ) {

        //
        // Replace the full path with the relative path.
        //
        NewFileName = *( PUNICODE_STRING ) &RelativeName.RelativeName;

    } else {

        //
        // Using the full path - no containing directory.
        //
        RelativeName.ContainingDirectory = NULL;
    }

    //
    // Initialize the Obja structure for the new file.
    //
    InitializeObjectAttributes(
            &ObjaNewFile,
            &NewFileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );


    //
    // Convert the old DOS path name to a canonical Nt path name.
    //
    ErrorFlag = RtlDosPathNameToNtPathName_U(
                    lpOldFile->Buffer,
                    &OldFileName,
                    NULL,
                    &RelativeName
                    );

    //
    // If the name was not succesfully converted assume it was invalid.
    //
    if ( !ErrorFlag ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, NewFreeBuffer );
        NtClose( HiveHandle );
        RPC_REVERT_TO_SELF();
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Remember the buffer allocatted by RtlDosPathNameToNtPathName_U.
    //
    OldFreeBuffer = OldFileName.Buffer;

    //
    // If a relative name and directory handle will work, use those.
    //
    if ( RelativeName.RelativeName.Length ) {

        //
        // Replace the full path with the relative path.
        //
        OldFileName = *( PUNICODE_STRING ) &RelativeName.RelativeName;

    } else {

        //
        // Using the full path - no containing directory.
        //
        RelativeName.ContainingDirectory = NULL;
    }

    //
    // Initialize the Obja structure for the new file.
    //
    InitializeObjectAttributes(
            &ObjaOldFile,
            &OldFileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );


    NtStatus = NtReplaceKey(
                    &ObjaNewFile,
                    HiveHandle,
                    &ObjaOldFile
                    );

    //
    // Free the buffers allocatted by RtlDosPathNameToNtPathName_U.
    //
    RtlFreeHeap( RtlProcessHeap( ), 0, NewFreeBuffer );
    RtlFreeHeap( RtlProcessHeap( ), 0, OldFreeBuffer );

    NtClose( HiveHandle );

    RPC_REVERT_TO_SELF();
    return (error_status_t)RtlNtStatusToDosError( NtStatus );
}
