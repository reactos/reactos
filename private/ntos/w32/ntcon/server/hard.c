/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    NtHard.c

Abstract:

Author:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if defined(i386)


#define REGISTRY_HARDWARE_DESCRIPTION \
        TEXT("\\Registry\\Machine\\Hardware\\DESCRIPTION\\System")

#define REGISTRY_MACHINE_IDENTIFIER   \
        TEXT("Identifier")

#define FUJITSU_FMR_NAME    TEXT("FUJITSU FMR-")
#define NEC_PC98_NAME       TEXT("NEC PC-98")



#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 256)

NTSTATUS
NtGetMachineIdentifierValue(
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the machine identifier information and get the
    value.

Arguments:

    ValueName - the unicode name for the registry value located in the
                identifier location of the registry.
    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    //
    // Set default as PC/AT
    //

    *Value = MACHINEID_MS_PCAT;

    KeyName.Buffer = REGISTRY_HARDWARE_DESCRIPTION;
    KeyName.Length = sizeof(REGISTRY_HARDWARE_DESCRIPTION) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(REGISTRY_HARDWARE_DESCRIPTION);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    ValueName.Buffer = REGISTRY_MACHINE_IDENTIFIER;
    ValueName.Length = sizeof(REGISTRY_MACHINE_IDENTIFIER) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(REGISTRY_MACHINE_IDENTIFIER);

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    Status = NtQueryValueKey(Handle,
                             &ValueName,
                             KeyValueFullInformation,
                             KeyValueInformation,
                             RequestLength,
                             &ResultLength);

    ASSERT( Status != STATUS_BUFFER_OVERFLOW );

    NtClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PWCHAR DataPtr;
            UNICODE_STRING DetectedString, TargetString1, TargetString2;

            //
            // Return contents to the caller.
            //

            DataPtr = (PWCHAR)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);

            //
            // Initialize strings.
            //

            RtlInitUnicodeString( &DetectedString, DataPtr );
            RtlInitUnicodeString( &TargetString1, FUJITSU_FMR_NAME );
            RtlInitUnicodeString( &TargetString2, NEC_PC98_NAME );

            //
            // Check the hardware platform
            //

            if (RtlPrefixUnicodeString( &TargetString1 , &DetectedString , TRUE)) {

                //
                // Fujitsu FMR Series.
                //

                *Value = MACHINEID_FUJITSU_FMR;

            } else if (RtlPrefixUnicodeString( &TargetString2 , &DetectedString , TRUE)) {

                //
                // NEC PC-9800 Seriss
                //

                *Value = MACHINEID_NEC_PC98;

            } else {

                //
                // Standard PC/AT comapatibles
                //

                *Value = MACHINEID_MS_PCAT;

            }

        } else {

            //
            // Treat as if no value was found
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

    }

    return Status;
}
#endif // defined(i386)
