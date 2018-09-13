/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    prodtype.c

Abstract:

    This module defines a function to determine the product type.

Author:

    Cliff Van Dyke (CliffV) 20-Mar-1992

Revision History:


--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlGetNtProductType)
#endif


BOOLEAN
RtlGetNtProductType(
    OUT PNT_PRODUCT_TYPE    NtProductType
    )

/*++

Routine Description:

    Returns the product type of the current system.

Arguments:

    NtProductType - Returns the product type.  Either NtProductWinNt or
        NtProductLanManNt.

Return Value:

    TRUE on success, FALSE on failure
    The product type will be set to WinNt on failure

--*/

{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG KeyValueInfoLength;
    ULONG ResultLength;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;
    UNICODE_STRING Value;
    UNICODE_STRING WinNtValue;
    UNICODE_STRING LanmanNtValue;
    UNICODE_STRING ServerNtValue;
    BOOLEAN Result;

    RTL_PAGED_CODE();

    //
    // if we are in gui setup mode, product type is read from the registry since
    // gui setup mode is the only time product type can be changed.
    // All other times, the "captured at boot" version of product type is used
    //

    if ( USER_SHARED_DATA->ProductTypeIsValid ) {
        *NtProductType = USER_SHARED_DATA->NtProductType;
        return TRUE;
        }

    //
    // Prepare default value for failure case
    //

    *NtProductType = NtProductWinNt;
    Result = FALSE;

    RtlInitUnicodeString( &KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ProductOptions" );
    RtlInitUnicodeString( &ValueName, L"ProductType" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &KeyHandle,
                        MAXIMUM_ALLOWED,
                        &ObjectAttributes
                      );
    KeyValueInformation = NULL;
    if (NT_SUCCESS( Status )) {
        KeyValueInfoLength = 256;
        KeyValueInformation = RtlAllocateHeap( RtlProcessHeap(), 0,
                                               KeyValueInfoLength
                                             );
        if (KeyValueInformation == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {
            Status = NtQueryValueKey( KeyHandle,
                                      &ValueName,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      KeyValueInfoLength,
                                      &ResultLength
                                    );
        }
    } else {
        KeyHandle = NULL;
    }

    if (NT_SUCCESS( Status ) && KeyValueInformation->Type == REG_SZ) {

        //
        // Decide which product we are installed as
        //

        Value.Buffer = (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        Value.Length = (USHORT)(KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
        Value.MaximumLength = (USHORT)(KeyValueInformation->DataLength);
        RtlInitUnicodeString(&WinNtValue, L"WinNt");
        RtlInitUnicodeString(&LanmanNtValue, L"LanmanNt");
        RtlInitUnicodeString(&ServerNtValue, L"ServerNt");

        if (RtlEqualUnicodeString(&Value, &WinNtValue, TRUE)) {
            *NtProductType = NtProductWinNt;
            Result = TRUE;
        } else if (RtlEqualUnicodeString(&Value, &LanmanNtValue, TRUE)) {
            *NtProductType = NtProductLanManNt;
            Result = TRUE;
        } else if (RtlEqualUnicodeString(&Value, &ServerNtValue, TRUE)) {
            *NtProductType = NtProductServer;
            Result = TRUE;
        } else {
#if DBG
            DbgPrint("RtlGetNtProductType: Product type unrecognised <%wZ>\n", &Value);
#endif // DBG
        }
    } else {
#if DBG
        DbgPrint("RtlGetNtProductType: %wZ\\%wZ not found or invalid type\n", &KeyPath, &ValueName );
#endif // DBG
    }

    //
    // Clean up our resources.
    //

    if (KeyValueInformation != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInformation );
    }

    if (KeyHandle != NULL) {
        NtClose( KeyHandle );
    }

    //
    // Return result.
    //

    return(Result);












}
