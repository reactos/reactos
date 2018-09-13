/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regsrkey.c

Abstract:

    This module contains the save\restore key APIs, that is:

        - RegRestoreKeyW
        - RegSaveKeyW

Author:

    David J. Gilman (davegi) 23-Jan-1992

Notes:



Revision History:

    25-Mar-1992     Ramon J. San Andres (ramonsa)
                    Changed to use RPC.

--*/


#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#ifdef LOCAL
#include "tsappcmp.h"
#include "regclass.h"
#endif


error_status_t
BaseRegRestoreKey(
    IN  HKEY            hKey,
    IN  PUNICODE_STRING lpFile,
    IN  DWORD           dwFlags
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
    UNICODE_STRING      FileName;
    RTL_RELATIVE_NAME   RelativeName;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK     IoStatusBlock;
    PVOID               FreeBuffer;
    BOOLEAN             ErrorFlag;
    NTSTATUS            NtStatus;
    NTSTATUS            NtStatus1;
    HANDLE              Handle;


    ASSERT( (hKey != NULL) && (lpFile != NULL) && (lpFile->Buffer != NULL));
    if ( (hKey == NULL) || (lpFile == NULL) || (lpFile->Buffer == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Remove the NULL from the Length. This was added by the client
    //  so that RPC would transmit the entire thing.
    //
    if ( lpFile->Length > 0 ) {
        lpFile->Length -= sizeof( UNICODE_NULL );
    }


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
    // Remember the buffer allocated by RtlDosPathNameToNtPathName_U.
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
    // Initialize the Obja structure for the save file.
    //
    InitializeObjectAttributes(
            &Obja,
            &FileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );


    //
    // Open the existing file.
    //
    NtStatus = NtOpenFile(
                    &Handle,
                    GENERIC_READ | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ,
                    FILE_SYNCHRONOUS_IO_NONALERT
                    );

    //
    // Free the buffer allocated by RtlDosPathNameToNtPathName_U.
    //
    RtlFreeHeap( RtlProcessHeap( ), 0, FreeBuffer );

    //
    // Check the results of the NtOpenFile.
    //
    if( NT_SUCCESS( NtStatus )) {

#ifdef LOCAL
        if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

            HKEY           hkRestoreKey;
            UNICODE_STRING EmptyString = {0, 0, NULL};

            //
            // We need to restore to to user if it exists, 
            // machine if not
            //
            NtStatus = BaseRegOpenClassKey(
                hKey,
                &EmptyString,
                0,
                MAXIMUM_ALLOWED,
                &hkRestoreKey);
            
            if (NT_SUCCESS(NtStatus)) {

                //
                // Now restore to the highest precedence key
                //
                NtStatus = NtRestoreKey( hkRestoreKey, Handle, dwFlags );

                NtClose(hkRestoreKey);
            }
        } else {
            //
            // If this isn't in hkcr, then just restore to the supplied object
            //
            NtStatus = NtRestoreKey( hKey, Handle, dwFlags );
        }

#else // LOCAL
        //
        //  Now call the NT API
        //
        NtStatus = NtRestoreKey( hKey, Handle, dwFlags );
#endif // LOCAL

        //
        // Close the file.
        //
        NtStatus1 = NtClose(Handle);
        ASSERT( NT_SUCCESS( NtStatus1 ));

    }

    RPC_REVERT_TO_SELF();

#ifdef LOCAL
    if (NT_SUCCESS(NtStatus) && !(dwFlags & REG_WHOLE_HIVE_VOLATILE) && gpfnTermsrvRestoreKey) {
        gpfnTermsrvRestoreKey(hKey, Handle, dwFlags); 
    }
#endif

    //
    // Map the result of NtRestoreKey and return.
    //
    return (error_status_t)RtlNtStatusToDosError( NtStatus );
}




error_status_t
BaseRegSaveKey(
    IN  HKEY                     hKey,
    IN  PUNICODE_STRING          lpFile,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSecurityAttributes OPTIONAL
    )
/*++

Routine Description:

    Saves the given key to the specified file.

Arguments:

    hKey                    -   Supplies a handle to the open key.

    lpFile                  -   Supplies the name of the file to save the key to.

    pRpcSecurityAttributes  -   Supplies the security attributes of
                                the file.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.



--*/
{

    BOOLEAN             ErrorFlag;
    UNICODE_STRING      FileName;
    RTL_RELATIVE_NAME   RelativeName;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK     IoStatusBlock;
    PVOID               FreeBuffer;
    NTSTATUS            NtStatus;
    NTSTATUS            NtStatus1;
    HANDLE              Handle;

    ASSERT( (hKey != NULL) && (lpFile != NULL) && (lpFile->Buffer != NULL));
    if ( (hKey == NULL) || (lpFile == NULL) || (lpFile->Buffer == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Remove the NULL from the Length. This was added by the client
    //  so that RPC would transmit the entire thing.
    //
    if ( lpFile->Length > 0 ) {
        lpFile->Length -= sizeof( UNICODE_NULL );
    }

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
    if( ! ErrorFlag ) {
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
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;

    } else {

        //
        // Using the full path - no containing directory.
        //
        RelativeName.ContainingDirectory = NULL;
    }

    //
    // Initialize the Obja structure for the save file.
    //
    InitializeObjectAttributes(
                &Obja,
                &FileName,
                OBJ_CASE_INSENSITIVE,
                RelativeName.ContainingDirectory,
                ARGUMENT_PRESENT( pRpcSecurityAttributes )
                        ? pRpcSecurityAttributes
                                ->RpcSecurityDescriptor.lpSecurityDescriptor
                        : NULL
                );



    //
    // Create the file - fail if the file exists.
    //
    NtStatus = NtCreateFile(
                    &Handle,
                    GENERIC_WRITE | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_CREATE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                    NULL,
                    0
                    );

    //
    // Free the buffer allocated by RtlDosPathNameToNtPathName_U.
    //
    RtlFreeHeap( RtlProcessHeap( ), 0, FreeBuffer );

    //
    // Check the results of the NtCreateFile.
    //
    if ( NT_SUCCESS( NtStatus )) {

#ifdef LOCAL

        if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

            HKEY hkMachineClass;
            HKEY hkUserClass;

            NtStatus = BaseRegGetUserAndMachineClass(
                NULL,
                hKey,
                MAXIMUM_ALLOWED,
                &hkMachineClass,
                &hkUserClass);

            if (NT_SUCCESS(NtStatus)) {

                //
                // We only need to merge keys if we have
                // more than one key
                //
                if (hkMachineClass && hkUserClass) {
                
                    NtStatus = NtSaveMergedKeys(
                        hkUserClass,
                        hkMachineClass,
                        Handle);

                    //
                    // Clean up the extra handle we opened
                    //
                    if (hkUserClass != hKey) {
                        NtClose(hkUserClass);
                    } else {
                        NtClose(hkMachineClass);
                    }

                } else {
                    //
                    // If there's only one key, use the regular
                    // api
                    //
                    NtStatus = NtSaveKey( hKey, Handle );
                }
            }
        } else {
            //
            // If this isn't in hkcr, just save the regular way
            //
            NtStatus = NtSaveKey( hKey, Handle );
        }
#else // LOCAL
        NtStatus = NtSaveKey( hKey, Handle );
#endif // LOCAL

        //
        // Close the file.
        //
        NtStatus1 = NtClose( Handle );
        ASSERT( NT_SUCCESS( NtStatus1 ));

    }

    RPC_REVERT_TO_SELF();

    //
    // Map the result of NtSaveKey and return.
    //
    return (error_status_t)RtlNtStatusToDosError( NtStatus );
}
