/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsRtlP.c

Abstract:

    This module declares the global data used by the FsRtl Module

Author:

    Gary Kimura     [GaryKi]    30-Jul-1990

Revision History:

--*/

#include "FsRtlP.h"

#define COMPATIBILITY_MODE_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define COMPATIBILITY_MODE_VALUE_NAME L"Win95TruncatedExtensions"

#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 64)

#ifdef FSRTLDBG

LONG FsRtlDebugTraceLevel = 0x0000000f;
LONG FsRtlDebugTraceIndent = 0;

#endif // FSRTLDBG

//
//  Local Support routine
//

NTSTATUS
FsRtlGetCompatibilityModeValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlAllocateResource)
#pragma alloc_text(INIT, FsRtlInitSystem)
#pragma alloc_text(INIT, FsRtlGetCompatibilityModeValue)
#endif

//
//  Define the number of resources, a pointer to them and a counter for
//  resource selection.
//

#define FSRTL_NUMBER_OF_RESOURCES (16)

PERESOURCE FsRtlPagingIoResources;

ULONG FsRtlPagingIoResourceSelector = 0;
BOOLEAN FsRtlSafeExtensions = TRUE;

//
//  The global static legal ANSI character array.  Wild characters
//  are not considered legal, they should be checked seperately if
//  allowed.
//

#define _FAT_  FSRTL_FAT_LEGAL
#define _HPFS_ FSRTL_HPFS_LEGAL
#define _NTFS_ FSRTL_NTFS_LEGAL
#define _OLE_  FSRTL_OLE_LEGAL
#define _WILD_ FSRTL_WILD_CHARACTER

static UCHAR LocalLegalAnsiCharacterArray[128] = {

    0                                   ,   // 0x00 ^@
                                   _OLE_,   // 0x01 ^A
                                   _OLE_,   // 0x02 ^B
                                   _OLE_,   // 0x03 ^C
                                   _OLE_,   // 0x04 ^D
                                   _OLE_,   // 0x05 ^E
                                   _OLE_,   // 0x06 ^F
                                   _OLE_,   // 0x07 ^G
                                   _OLE_,   // 0x08 ^H
                                   _OLE_,   // 0x09 ^I
                                   _OLE_,   // 0x0A ^J
                                   _OLE_,   // 0x0B ^K
                                   _OLE_,   // 0x0C ^L
                                   _OLE_,   // 0x0D ^M
                                   _OLE_,   // 0x0E ^N
                                   _OLE_,   // 0x0F ^O
                                   _OLE_,   // 0x10 ^P
                                   _OLE_,   // 0x11 ^Q
                                   _OLE_,   // 0x12 ^R
                                   _OLE_,   // 0x13 ^S
                                   _OLE_,   // 0x14 ^T
                                   _OLE_,   // 0x15 ^U
                                   _OLE_,   // 0x16 ^V
                                   _OLE_,   // 0x17 ^W
                                   _OLE_,   // 0x18 ^X
                                   _OLE_,   // 0x19 ^Y
                                   _OLE_,   // 0x1A ^Z
                                   _OLE_,   // 0x1B ESC
                                   _OLE_,   // 0x1C FS
                                   _OLE_,   // 0x1D GS
                                   _OLE_,   // 0x1E RS
                                   _OLE_,   // 0x1F US
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x20 space
    _FAT_ | _HPFS_ | _NTFS_              ,  // 0x21 !
                            _WILD_| _OLE_,  // 0x22 "
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x23 #
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x24 $
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x25 %
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x26 &
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x27 '
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x28 (
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x29 )
                            _WILD_| _OLE_,  // 0x2A *
            _HPFS_ | _NTFS_       | _OLE_,  // 0x2B +
            _HPFS_ | _NTFS_       | _OLE_,  // 0x2C ,
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x2D -
    _FAT_ | _HPFS_ | _NTFS_              ,  // 0x2E .
    0                                    ,  // 0x2F /
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x30 0
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x31 1
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x32 2
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x33 3
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x34 4
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x35 5
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x36 6
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x37 7
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x38 8
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x39 9
                     _NTFS_              ,  // 0x3A :
            _HPFS_ | _NTFS_       | _OLE_,  // 0x3B ;
                            _WILD_| _OLE_,  // 0x3C <
            _HPFS_ | _NTFS_       | _OLE_,  // 0x3D =
                            _WILD_| _OLE_,  // 0x3E >
                            _WILD_| _OLE_,  // 0x3F ?
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x40 @
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x41 A
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x42 B
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x43 C
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x44 D
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x45 E
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x46 F
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x47 G
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x48 H
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x49 I
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4A J
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4B K
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4C L
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4D M
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4E N
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4F O
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x50 P
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x51 Q
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x52 R
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x53 S
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x54 T
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x55 U
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x56 V
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x57 W
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x58 X
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x59 Y
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5A Z
            _HPFS_ | _NTFS_       | _OLE_,  // 0x5B [
    0                                    ,  // 0x5C backslash
            _HPFS_ | _NTFS_       | _OLE_,  // 0x5D ]
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5E ^
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5F _
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x60 `
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x61 a
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x62 b
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x63 c
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x64 d
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x65 e
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x66 f
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x67 g
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x68 h
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x69 i
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6A j
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6B k
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6C l
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6D m
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6E n
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6F o
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x70 p
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x71 q
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x72 r
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x73 s
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x74 t
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x75 u
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x76 v
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x77 w
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x78 x
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x79 y
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7A z
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7B {
    0                             | _OLE_,  // 0x7C |
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7D }
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7E ~
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7F 
};

PUCHAR FsRtlLegalAnsiCharacterArray = &LocalLegalAnsiCharacterArray[0];

//
//  This routine is called during phase one initialization.
//

BOOLEAN
FsRtlInitSystem (
    )
{
    ULONG i;

    ULONG Value;
    UNICODE_STRING ValueName;

    extern KSEMAPHORE FsRtlpUncSemaphore;

    PAGED_CODE();

    //
    //  Allocate and initialize all the paging Io resources
    //

    FsRtlPagingIoResources = FsRtlAllocatePool( NonPagedPool,
                                                FSRTL_NUMBER_OF_RESOURCES *
                                                sizeof(ERESOURCE) );

    for (i=0; i < FSRTL_NUMBER_OF_RESOURCES; i++) {

        ExInitializeResource( &FsRtlPagingIoResources[i] );
    }

    //
    //  Initialize the global tunneling structures.
    //

    FsRtlInitializeTunnels();

    //
    //  Initialize the global filelock structures.
    //

    FsRtlInitializeFileLocks();

    //
    //  Initialize the global largemcb structures.
    //

    FsRtlInitializeLargeMcbs();

    //
    // Initialize the semaphore used to guard loading of the MUP
    //

    KeInitializeSemaphore( &FsRtlpUncSemaphore, 1, MAXLONG );

    //
    // Pull the bit from the registry telling us whether to do a safe
    // or dangerous extension truncation.
    //

    ValueName.Buffer = COMPATIBILITY_MODE_VALUE_NAME;
    ValueName.Length = sizeof(COMPATIBILITY_MODE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(COMPATIBILITY_MODE_VALUE_NAME);

    if (NT_SUCCESS(FsRtlGetCompatibilityModeValue( &ValueName, &Value )) &&
        (Value != 0)) {

        FsRtlSafeExtensions = FALSE;
    }

    //
    // Initialize the FsRtl stack overflow work QueueObject and thread.
    //

    if (!NT_SUCCESS(FsRtlInitializeWorkerThread())) {

        return FALSE;
    }

    return TRUE;
}


PERESOURCE
FsRtlAllocateResource (
    )

/*++

Routine Description:

    This routine is used to allocate a resource from the FsRtl pool.

Arguments:

Return Value:

    PERESOURCE - A pointer to the provided resource.

--*/

{
    PAGED_CODE();

    return &FsRtlPagingIoResources[ FsRtlPagingIoResourceSelector++ %
                                    FSRTL_NUMBER_OF_RESOURCES];
}


//
//  Local Support routine
//

NTSTATUS
FsRtlGetCompatibilityModeValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the Chicago compatibilitymode information and get the
    value.

Arguments:

    ValueName - the unicode name for the registry value located in the
                double space configuration location of the registry.
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
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    KeyName.Buffer = COMPATIBILITY_MODE_KEY_NAME;
    KeyName.Length = sizeof(COMPATIBILITY_MODE_KEY_NAME) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(COMPATIBILITY_MODE_KEY_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    while (1) {

        Status = ZwQueryValueKey(Handle,
                                 ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

                ExFreePool(KeyValueInformation);
            }

            RequestLength += 256;

            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        ' taF');

            if (!KeyValueInformation) {
                return STATUS_NO_MEMORY;
            }

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PULONG DataPtr;

            //
            // Return contents to the caller.
            //

            DataPtr = (PULONG)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
            *Value = *DataPtr;

        } else {

            //
            // Treat as if no value was found
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

        ExFreePool(KeyValueInformation);
    }

    return Status;
}
