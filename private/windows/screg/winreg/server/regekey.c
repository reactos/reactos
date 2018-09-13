/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regekey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry API to enumerate keys.  That is:

        - BaseRegEnumKey

Author:

    David J. Gilman (davegi) 23-Dec-1991

Notes:

    See the Notes in Regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regecls.h"
#include <malloc.h>

#define DEFAULT_KEY_NAME_SIZE         128
#define DEFAULT_CLASS_SIZE            128



error_status_t
BaseRegEnumKey (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpName,
    OUT PUNICODE_STRING lpClass OPTIONAL,
    OUT PFILETIME lpftLastWriteTime OPTIONAL
    )

/*++

Routine Description:

    Used to enumerate subkeys of an open key.  This function copies the
    dwIndex-th subkey of hKey.

Arguments:

    hKey - A handle to the open key.  The keys returned are relative to
        the key pointed to by this key handle.  Any of the predefined reserved
        handles or a previously opened key handle may be used for hKey.

    dwIndex - The index of the subkey to return.  Note that this is for
        convenience, subkeys are not ordered (a new subkey has an arbitrary
        index).  Indexes start at 0.

    lpName - Provides a pointer to a buffer to receive the name of the
        key.

    lpClass - If present, provides a pointer to a buffer to receive the
        class of the key.

    lpftLastWriteTime - The time when the value was last written (set or
        created).

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    This function is guaranteed to operate correctly only if dwIndex
    starts at 0 and is incremented on successive calls without intervening
    calls to other registration APIs that will change the key.
    KEY_ENUMERATE_SUB_KEYS access is required.


--*/

{
    NTSTATUS                        Status;
    ULONG                           BufferLength;
    KEY_INFORMATION_CLASS           KeyInformationClass;
    PVOID                           KeyInformation;
    ULONG                           ResultLength;
    BOOL                            fClassKey;

    BYTE         PrivateKeyInformation[ sizeof( KEY_NODE_INFORMATION ) +
                                        DEFAULT_KEY_NAME_SIZE +
                                        DEFAULT_CLASS_SIZE ];

    ASSERT( lpName != NULL );

    //
    // Protect ourselves against malicious callers passing NULL
    // pointers.
    //
    if (lpClass == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Call out to Perflib if the HKEY is HKEY_PERFOMANCE_DATA or
    // HKEY_PERFORMANCE_TEXT or HKEY_PERFORMANCE_NLSTEXT

    if (hKey == HKEY_PERFORMANCE_DATA ||
        hKey == HKEY_PERFORMANCE_TEXT ||
        hKey == HKEY_PERFORMANCE_NLSTEXT ) {
//    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return (error_status_t)PerfRegEnumKey (
                                        hKey,
                                        dwIndex,
                                        lpName,
                                        NULL,
                                        lpClass,
                                        lpftLastWriteTime
                                        );
    }


    //
    //  First we assume that the information we want will fit on
    //  PrivateKeyValueInformattion
    //

    KeyInformationClass = (ARGUMENT_PRESENT( lpClass->Buffer ))?
                               KeyNodeInformation :
                               KeyBasicInformation;


    KeyInformation = PrivateKeyInformation;
    BufferLength = sizeof( PrivateKeyInformation );

    fClassKey = FALSE;
    Status = STATUS_SUCCESS;

    //
    // Query for the necessary information about the supplied key.
    //

#ifdef LOCAL
    //
    // For hkcr, we need to do special enumeration
    //
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {
        
        Status = EnumTableGetNextEnum( &gClassesEnumTable,
                                       hKey,
                                       dwIndex,
                                       KeyInformationClass,
                                       KeyInformation,
                                       BufferLength,
                                       &ResultLength);

        if (!NT_SUCCESS(Status) || (NT_SUCCESS(Status) && ResultLength)) {
            fClassKey = TRUE;
        }
    }
#endif // LOCAL

    if (!fClassKey) {

        Status = NtEnumerateKey( hKey,
                                 dwIndex,
                                 KeyInformationClass,
                                 KeyInformation,
                                 BufferLength,
                                 &ResultLength
            );
    }

    //
    // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
    // was not enough room for even the fixed portions of the structure.
    //

    ASSERT( Status != STATUS_BUFFER_TOO_SMALL );


    if( Status == STATUS_BUFFER_OVERFLOW ) {
        //
        //  The buffer defined in the stack wasn't big enough to hold
        //  the Key information.
        //  If the caller's buffer are big enough to hold the key name
        //  and key class, then allocate a new buffer, and call the
        //  NT API again.
        //
        if( ( ( KeyInformationClass == KeyBasicInformation ) &&
              ( (ULONG)( lpName->MaximumLength ) >=
                 (( PKEY_BASIC_INFORMATION )
                 KeyInformation )->NameLength + sizeof(UNICODE_NULL)
              )
            ) ||
            ( ( KeyInformationClass == KeyNodeInformation ) &&
              ( (ULONG)(lpName->MaximumLength) >=
                 (( PKEY_NODE_INFORMATION )
                 KeyInformation )->NameLength + sizeof(UNICODE_NULL)
              ) &&
              (
                ARGUMENT_PRESENT( lpClass->Buffer )
              ) &&
              (
                (ULONG)(lpClass->MaximumLength) >= (( PKEY_NODE_INFORMATION )
                        KeyInformation )->ClassLength + sizeof(UNICODE_NULL)
              )
            )
          ) {
            BufferLength = ResultLength;

            KeyInformation = RtlAllocateHeap( RtlProcessHeap( ), 0,
                                              BufferLength
                                            );
            //
            // If the memory allocation fails, return a Registry error.
            //

            if( ! KeyInformation ) {
                return ERROR_OUTOFMEMORY;
            }

            //
            // Query for the necessary information about the supplied key.
            // This may or may not include the class depending on lpClass->Buffer
            // as determined above.
            //

#ifdef LOCAL
            if (fClassKey) {
                //
                // For hkcr, we need to do special enumeration
                //
                Status = EnumTableGetNextEnum( &gClassesEnumTable,
                                               hKey,
                                               dwIndex,
                                               KeyInformationClass,
                                               KeyInformation,
                                               BufferLength,
                                               &ResultLength);

            } else
#endif // LOCAL
            {
                Status = NtEnumerateKey( hKey,
                                         dwIndex,
                                         KeyInformationClass,
                                         KeyInformation,
                                         BufferLength,
                                         &ResultLength
                    );
            }

        }
    }

    if( NT_SUCCESS( Status ) ) {
        //
        //  Copy key name
        //

        if( KeyInformationClass == KeyBasicInformation ) {
            //
            // Return the name length and the name of the key.
            // Note that the NUL byte is included so that RPC copies the
            // correct number of bytes. It is decremented on the client
            // side.
            //

            if( (ULONG)(lpName->MaximumLength) >=
                 (( PKEY_BASIC_INFORMATION )
                  KeyInformation )->NameLength + sizeof( UNICODE_NULL ) ) {

                lpName->Length = ( USHORT )
                                 (( PKEY_BASIC_INFORMATION )
                                 KeyInformation )->NameLength;

                RtlMoveMemory( lpName->Buffer,
                               (( PKEY_BASIC_INFORMATION )
                               KeyInformation )->Name,
                               lpName->Length
                             );

                //
                // NUL terminate the value name.
                //

                lpName->Buffer[ lpName->Length >> 1 ] = UNICODE_NULL;
                lpName->Length += sizeof( UNICODE_NULL );

            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // If requested, return the last write time.
            //

            if( ARGUMENT_PRESENT( lpftLastWriteTime )) {

                *lpftLastWriteTime
                = *( PFILETIME )
                &(( PKEY_BASIC_INFORMATION ) KeyInformation )
                ->LastWriteTime;
            }

        } else {
            //
            // Return the name length and the name of the key.
            // Note that the NUL byte is included so that RPC copies the
            // correct number of bytes. It is decremented on the client
            // side.
            //

            if( ( (ULONG)(lpName->MaximumLength) >=
                  (( PKEY_NODE_INFORMATION )
                   KeyInformation )->NameLength + sizeof( UNICODE_NULL ) ) &&
                ( (ULONG)(lpClass->MaximumLength) >=
                  (( PKEY_NODE_INFORMATION )
                   KeyInformation )->ClassLength + sizeof( UNICODE_NULL) )
              ) {
                //
                //  Copy the key name
                //
                lpName->Length = ( USHORT )
                                 (( PKEY_NODE_INFORMATION )
                                 KeyInformation )->NameLength;

                RtlMoveMemory( lpName->Buffer,
                               (( PKEY_NODE_INFORMATION )
                               KeyInformation )->Name,
                               lpName->Length
                             );

                //
                // NUL terminate the key name.
                //

                lpName->Buffer[ lpName->Length >> 1 ] = UNICODE_NULL;
                lpName->Length += sizeof( UNICODE_NULL );


                //
                //  Copy the key class
                //

                lpClass->Length = (USHORT)
                    ((( PKEY_NODE_INFORMATION ) KeyInformation )->ClassLength );

                RtlMoveMemory(
                    lpClass->Buffer,
                    ( PBYTE ) KeyInformation
                    + (( PKEY_NODE_INFORMATION ) KeyInformation )->ClassOffset,
                    (( PKEY_NODE_INFORMATION ) KeyInformation )->ClassLength
                    );

                //
                // NUL terminate the class.
                //

                lpClass->Buffer[ lpClass->Length >> 1 ] = UNICODE_NULL;

                lpClass->Length += sizeof( UNICODE_NULL );


            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // If requested, return the last write time.
            //

            if( ARGUMENT_PRESENT( lpftLastWriteTime )) {

                *lpftLastWriteTime
                = *( PFILETIME )
                &(( PKEY_NODE_INFORMATION ) KeyInformation )
                ->LastWriteTime;
            }

        }

    }

    if( KeyInformation != PrivateKeyInformation ) {
        //
        // Free the buffer allocated.
        //

        RtlFreeHeap( RtlProcessHeap( ), 0, KeyInformation );
    }

    return (error_status_t)RtlNtStatusToDosError( Status );
}
