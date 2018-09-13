/*++


Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regeval.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry API to enumerate values. That is:

        - BaseRegEnumValue

Author:

    David J. Gilman (davegi) 23-Dec-1991

Notes:

    See the Notes in Regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regvcls.h"

#define DEFAULT_VALUE_SIZE          128
#define DEFAULT_VALUE_NAME_SIZE     64


error_status_t
BaseRegEnumValue(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData OPTIONAL,
    IN OUT LPDWORD lpcbData OPTIONAL,
    IN OUT LPDWORD lpcbLen  OPTIONAL
    )

/*++

Routine Description:

    Used to enumerate the ValueNames of an open key.  This function copies
    the dwIndex-th ValueName of hKey.  This function is guaranteed to
    operate correctly only if dwIndex starts at 0 and is incremented on
    successive calls without intervening calls to other registration APIs
    that will change the key.  The ValueName (only the ValueName, not the
    full path) is copied to lpBuffer.  The size of lpBuffer is specified
    by dwBufferSize.

Arguments:

    hKey - A handle to the open key.  The value entries returned are
        contained in the key pointed to by this key handle.  Any of the
        predefined reserved handles or a previously opened key handle may be
        used for hKey.

    dwIndex - The index of the ValueName to return.  Note that this is for
        convenience, ValueNames are not ordered (a new ValueName has an
        arbitrary index).  Indexes start at 0.

    lpValueName - Provides a pointer to a buffer to receive the name of
        the value (it's Id)

    lpType - If present, supplies pointer to variable to receive the type
        code of value entry.

    lpData - If present, provides a pointer to a buffer to receive the
        data of the value entry.

    lpcbData - Must be present if lpDatais.  Provides pointer to a
        variable which on input contains the size of the buffer lpDatapoints
        to.  On output, the variable will receive the number of bytes returned
        in lpData.

    lpcbLen - Return the number of bytes to transmit to the client (used
        by RPC).


Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    hKey must have been opened for KEY_QUERY_VALUE access.


--*/

{
    NTSTATUS                        Status;
    ULONG                           BufferLength;
    KEY_VALUE_INFORMATION_CLASS     KeyValueInformationClass;
    PVOID                           KeyValueInformation;
    ULONG                           ResultLength;

    BYTE    PrivateKeyValueInformation[ sizeof( KEY_VALUE_FULL_INFORMATION ) +
                                        DEFAULT_VALUE_NAME_SIZE +
                                        DEFAULT_VALUE_SIZE ];
    HKEY                            hkEnum;
#ifdef LOCAL
    ValueState*                     pValState;

    pValState = NULL;
#endif // LOCAL    
    hkEnum = hKey;

    //
    //  If the client gave us a bogus size, patch it.
    //
    if ( ARGUMENT_PRESENT( lpcbData ) && !ARGUMENT_PRESENT( lpData ) ) {
        *lpcbData = 0;
    }

    //
    // Call out to Perflib if the HKEY is HKEY_PERFOMANCE_DATA.
    //

    if(( hKey == HKEY_PERFORMANCE_DATA ) ||
       ( hKey == HKEY_PERFORMANCE_TEXT ) ||
       ( hKey == HKEY_PERFORMANCE_NLSTEXT )) {

        return (error_status_t)PerfRegEnumValue (
                                    hKey,
                                    dwIndex,
                                    lpValueName,
                                    NULL,
                                    lpType,
                                    lpData,
                                    lpcbData,
                                    lpcbLen
                                    );
    }

#ifdef LOCAL
    //
    // If we are in HKEY_CLASSES_ROOT, then we need to remap
    // the key / index pair to take into account merging
    //

    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {
        
        //
        // Find a key state for this key
        //
        Status = BaseRegGetClassKeyValueState(
            hKey,
            dwIndex,
            &pValState);

        if (!NT_SUCCESS(Status)) {
            return (error_status_t)RtlNtStatusToDosError(Status);
        }

        //
        // Now remap to the appropriate key / index 
        //
        ValStateGetPhysicalIndexFromLogical(
            pValState,
            hKey,
            dwIndex,
            &hkEnum,
            &dwIndex);

    }
#endif // LOCAL

    //
    //  First we assume that the information we want will fit on
    //  PrivateKeyValueInformattion
    //

    KeyValueInformationClass = ( ARGUMENT_PRESENT( lpcbData ))?
                               KeyValueFullInformation :
                               KeyValueBasicInformation;


    KeyValueInformation = PrivateKeyValueInformation;
    BufferLength = sizeof( PrivateKeyValueInformation );

    //
    // Query for the necessary information about the supplied value.
    //

    Status = NtEnumerateValueKey( hkEnum,
                                  dwIndex,
                                  KeyValueInformationClass,
                                  KeyValueInformation,
                                  BufferLength,
                                  &ResultLength
                                );

    //
    // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
    // was not enough room for even the known (i.e. fixed length portion)
    // of the structure.
    //

    ASSERT( Status != STATUS_BUFFER_TOO_SMALL );


    if( Status == STATUS_BUFFER_OVERFLOW ) {
        //
        //  The buffer defined in the stack wasn't big enough to hold
        //  the Value information.
        //  If the caller's buffer are big enough to hold the value name
        //  and value data, then allocate a new buffer, and call the
        //  NT API again.
        //
        if( ( ( KeyValueInformationClass == KeyValueBasicInformation ) &&
              ( (ULONG)(lpValueName->MaximumLength) >=
                 (( PKEY_VALUE_BASIC_INFORMATION )
                 KeyValueInformation )->NameLength + sizeof(UNICODE_NULL)
              )
            ) ||
            ( ( KeyValueInformationClass == KeyValueFullInformation ) &&
              ( (ULONG)(lpValueName->MaximumLength) >=
                 (( PKEY_VALUE_FULL_INFORMATION )
                 KeyValueInformation )->NameLength + sizeof(UNICODE_NULL)
              ) &&
              ( !ARGUMENT_PRESENT( lpData ) ||
                ( ARGUMENT_PRESENT( lpData ) &&
                  ARGUMENT_PRESENT( lpcbData ) &&
                  ( *lpcbData >= (( PKEY_VALUE_FULL_INFORMATION )
                        KeyValueInformation )->DataLength
                  )
                )
              )
            )
          ) {
            BufferLength = ResultLength;

            KeyValueInformation = RtlAllocateHeap( RtlProcessHeap( ), 0,
                                                   BufferLength
                                                 );
            //
            // If the memory allocation fails, return a Registry error.
            //

            if( ! KeyValueInformation ) {
#ifdef LOCAL
                ValStateRelease(pValState);
#endif // LOCAL
                return ERROR_OUTOFMEMORY;
            }

            //
            // Query for the necessary information about the supplied value. This
            // may or may not include the data depending on lpcbData as determined
            // above.
            //

            Status = NtEnumerateValueKey( hkEnum,
                                          dwIndex,
                                          KeyValueInformationClass,
                                          KeyValueInformation,
                                          BufferLength,
                                          &ResultLength
                                        );
        }
    }

#ifdef LOCAL
    ValStateRelease(pValState);
#endif // LOCAL

    //
    //  If the API succeeded, try to copy the value name to the client's buffer
    //

    if( NT_SUCCESS( Status ) ) {
        //
        //  Copy value name
        //

        if( KeyValueInformationClass == KeyValueBasicInformation ) {
            //
            // Return the name length and the name of the value.
            // Note that the NUL byte is included so that RPC copies the
            // correct number of bytes. It is decremented on the client
            // side.
            //

            if( (ULONG)(lpValueName->MaximumLength) >=
                 (( PKEY_VALUE_BASIC_INFORMATION )
                  KeyValueInformation )->NameLength + sizeof( UNICODE_NULL )) {

                //
                // If client's buffer is big enough for the name,
                // copy the value name and NUL terminate it
                //
                lpValueName->Length = ( USHORT )
                                      (( PKEY_VALUE_BASIC_INFORMATION )
                                          KeyValueInformation )->NameLength;

                RtlMoveMemory( lpValueName->Buffer,
                               (( PKEY_VALUE_BASIC_INFORMATION )
                               KeyValueInformation )->Name,
                               lpValueName->Length
                             );

                lpValueName->Buffer[ lpValueName->Length >> 1 ] = UNICODE_NULL;

                //
                // Value name length must include size of UNICODE_NULL.
                // It will be decremented in the client side
                //

                lpValueName->Length += sizeof( UNICODE_NULL );

            } else {
                //
                //  If the client's buffer for the value name is not big
                //  enough, then set status to STATUS_BUFFER_OVERFLOW.
                //
                //  Note that in the remote case, RPC will transmit garbage
                //  in the buffer back to the client.
                //  We cannot set the buffer to prevent this transmission,
                //  because in the local case we would be destroying the
                //  buffer in the &NtCurrectTeb->StaticUnicodeString.
                //

                Status = STATUS_BUFFER_OVERFLOW;
            }

        } else {
            //
            // Here if KeyValueInformation == KeyValueFullInformation
            //
            // Return the name length and the name of the value.
            // Note that the NUL byte is included so that RPC copies the
            // correct number of bytes. It is decremented on the client
            // side.
            //

            if( (ULONG)(lpValueName->MaximumLength) >=
                 (( PKEY_VALUE_FULL_INFORMATION )
                  KeyValueInformation )->NameLength + sizeof( UNICODE_NULL )) {

                //
                // If client's buffer is big enough for the name,
                // copy the value name and NUL terminate it
                //
                lpValueName->Length = ( USHORT )
                                      (( PKEY_VALUE_FULL_INFORMATION )
                                          KeyValueInformation )->NameLength;

                RtlMoveMemory( lpValueName->Buffer,
                               (( PKEY_VALUE_FULL_INFORMATION )
                               KeyValueInformation )->Name,
                               lpValueName->Length
                             );

                lpValueName->Buffer[ lpValueName->Length >> 1 ] = UNICODE_NULL;

                //
                // Value name length must include size of UNICODE_NULL.
                // It will be decremented in the client side
                //

                lpValueName->Length += sizeof( UNICODE_NULL );

            } else {
                //
                //  If the client's buffer for the value name is not big
                //  enough, then set status to STATUS_BUFFER_OVERFLOW.
                //
                //  Note that in the remote case, RPC will transmit garbage
                //  in the buffer back to the client.
                //  We cannot set the buffer to prevent this transmission,
                //  because in the local case we would be destroying the
                //  buffer in the &NtCurrectTeb->StaticUnicodeString.
                //

                Status = STATUS_BUFFER_OVERFLOW;
            }

        }
    }



    if( NT_SUCCESS( Status ) &&
        ARGUMENT_PRESENT( lpData ) ) {

        //
        //  If we were able to copy the value name to the client's buffer
        //  and the value data is also requested, then try to copy it
        //  to the client's buffer
        //

        if( *lpcbData >= (( PKEY_VALUE_FULL_INFORMATION )
                           KeyValueInformation )->DataLength ) {
            //
            // If the buffer is big enough to hold the data, copy the data
            //
            RtlMoveMemory( lpData,
                           ( PBYTE ) KeyValueInformation
                             + (( PKEY_VALUE_FULL_INFORMATION )
                                KeyValueInformation )->DataOffset,
                           (( PKEY_VALUE_FULL_INFORMATION )
                                     KeyValueInformation )->DataLength
                         );
        } else {
            //
            // If buffer is not big enough to hold the data, then return
            // STATUS_BUFFER_OVERFLOW.
            //
            //  Note that in the remote case, RPC will transmit garbage
            //  in the buffer back to the client.
            //  We cannot set the buffer to prevent this transmission,
            //  because in the local case we would be destroying the
            //  buffer in the &NtCurrectTeb->StaticUnicodeString.
            //
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }



    //
    // Certain information is returned on success or in the case of
    // NtEnumerateValueKey returning STATUS_BUFFER_OVERFLOW.  This information
    // is always available because we always pass the minimum size required for
    // the NtEnumerateValueKey API.
    //

    if( NT_SUCCESS( Status ) ||
        ( Status == STATUS_BUFFER_OVERFLOW ) ) {

        if( KeyValueInformationClass == KeyValueBasicInformation ) {

            //
            // If requested, return the value type.
            //

            if( ARGUMENT_PRESENT( lpType )) {

                *lpType = (( PKEY_VALUE_BASIC_INFORMATION )
                            KeyValueInformation )->Type;
            }

//            lpValueName->Length
//                = ( USHORT ) ((( PKEY_VALUE_BASIC_INFORMATION )
//                KeyValueInformation )->NameLength + sizeof( UNICODE_NULL ) );

        } else {
            //
            // Here if KeyValueInformationClass == KeyValueFullInformation
            //

            //
            // If requested, return the value type.
            //

            if( ARGUMENT_PRESENT( lpType )) {

                *lpType = (( PKEY_VALUE_FULL_INFORMATION )
                            KeyValueInformation )->Type;
            }

//            lpValueName->Length
//                = ( USHORT ) ((( PKEY_VALUE_FULL_INFORMATION )
//                KeyValueInformation )->NameLength + sizeof( UNICODE_NULL ) );

            *lpcbData = (( PKEY_VALUE_FULL_INFORMATION )
                            KeyValueInformation )->DataLength;
        }
    }

    //
    // Transmit all of the value data back to the client.
    //

    if( NT_SUCCESS( Status ) ) {
        if( ARGUMENT_PRESENT( lpcbLen  ) &&
            ARGUMENT_PRESENT( lpcbData ) ) {
            *lpcbLen = *lpcbData;
        }
    } else {
        //
        // If something failed, don't transmit any data back to the client
        //
        if( ARGUMENT_PRESENT( lpcbLen ) ) {
            *lpcbLen = 0;
        }
    }

    //
    //  Free memory if it was allocated
    //
    if( KeyValueInformation != PrivateKeyValueInformation ) {

        RtlFreeHeap( RtlProcessHeap( ), 0, KeyValueInformation );

    }

    return (error_status_t)RtlNtStatusToDosError( Status );
}

