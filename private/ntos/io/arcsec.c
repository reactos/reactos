/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    arcsec.c

Abstract:

    This module contains subroutines for protecting the system
    partition on an ARC system.

Author:

    Jim Kelly  (JimK) 13-Jan-1993

Environment:

    Kernel mode - system initialization

Revision History:


--*/

#include "iop.h"

//
// Define procedures local to this module.
//

NTSTATUS
IopApplySystemPartitionProt(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IopProtectSystemPartition)
#pragma alloc_text(INIT,IopApplySystemPartitionProt)
#endif

//
// This name must match the name use by the DISK MANAGER utility.
// The Disk Manager creates and sets the value of this registry
// key.  We only look at it.
//

#define IOP_SYSTEM_PART_PROT_KEY    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
#define IOP_SYSTEM_PART_PROT_VALUE  L"Protect System Partition"

BOOLEAN
IopProtectSystemPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine assigns protection to the system partition of an
    ARC system, if necessary.  If this is not an ARC system, or
    the system partition does not need to be protected, then this
    routine does nothing.


Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not protection
    has been appropriately applied.   TRUE indicates no errors were
    encountered.  FALSE indicates an error was encountered.


--*/

{

    //
    // We only entertain the possibility of assigning protection
    // to the system partition if we are an ARC system.  For the
    // time being, the best way to determine if you are an ARC
    // system is to see if you aren't and X86 machine.  DavidRo
    // believes that at some point in the future we will have
    // ARC compliant X86 machines.  At that point in time, we
    // will need to change the following #ifdef's into something
    // that does a run-time determination.
    //

#ifdef i386  // if (!ARC-Compliant system)


    //
    // Nothing to do for non-ARC systems
    //

    return(TRUE);


#else // ARC-COMPLIANT system

    NTSTATUS status;
    NTSTATUS tmpStatus;
    HANDLE keyHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING keyName;
    UNICODE_STRING valueName;
    ULONG resultLength;
    ULONG keyBuffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG )];
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;

    //
    // This is an ARC system.  Attempt to retrieve information from the registry
    // indicating whether or not we should protect the system partition.
    //

    RtlInitUnicodeString( &keyName, IOP_SYSTEM_PART_PROT_KEY );
    InitializeObjectAttributes( &objectAttributes,
                                &keyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
    status = NtOpenKey( &keyHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS( status )) {

        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION) &keyBuffer[0];
        RtlInitUnicodeString( &valueName, IOP_SYSTEM_PART_PROT_VALUE );
        status = NtQueryValueKey( keyHandle,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  keyValue,
                                  sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG ),
                                  &resultLength );

        if (NT_SUCCESS( status )) {

            PBOOLEAN applyIt;

            //
            // The appropriate information was located in the registry.  Now
            // determine whether or not is indicates that protection is to be
            // applied.
            //

            applyIt = &(keyValue->Data[0]);

            if (*applyIt) {
                status = IopApplySystemPartitionProt( LoaderBlock );
            }
        }

        tmpStatus = NtClose( keyHandle );
        ASSERT(NT_SUCCESS( tmpStatus ));
    }


    return TRUE;

#endif // ARC-COMPLIANT system
}

NTSTATUS
IopApplySystemPartitionProt(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine applies protection to the system partition that
    prevents all users except administrators from accessing the
    partition.


    This routine is only used during system initialization.
    As such, all memory allocations are expected to succeed.
    Success is tested only with assertions.


Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    The function value is the final status from attempting to set the system
    partition protection.


--*/

{
    NTSTATUS status;
    PACL dacl;
    SECURITY_DESCRIPTOR securityDescriptor;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG length;
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';

    ASSERT( ARGUMENT_PRESENT( LoaderBlock ) );
    ASSERT( ARGUMENT_PRESENT( LoaderBlock->ArcHalDeviceName ) );

    //
    // Build an appropriate discretionary ACL.
    //

    length = (ULONG) sizeof( ACL ) +
             ( 2 * ((ULONG) sizeof( ACCESS_ALLOWED_ACE ))) +
             SeLengthSid( SeLocalSystemSid ) +
             SeLengthSid( SeAliasAdminsSid ) +
             8; // The 8 is just for good measure

    dacl = (PACL) ExAllocatePool( PagedPool, length );
    if (!dacl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl( dacl, length, ACL_REVISION2 );
    if (NT_SUCCESS( status )) {

        status = RtlAddAccessAllowedAce( dacl,
                                         ACL_REVISION2,
                                         GENERIC_ALL,
                                         SeLocalSystemSid );
        if (NT_SUCCESS( status )) {

            status = RtlAddAccessAllowedAce( dacl,
                                             ACL_REVISION2,
                                             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | READ_CONTROL,
                                             SeAliasAdminsSid );
            if (NT_SUCCESS( status )) {

                //
                // Put it in a security descriptor so that it may be applied to
                // the system partition device.
                //

                status = RtlCreateSecurityDescriptor( &securityDescriptor,
                                                      SECURITY_DESCRIPTOR_REVISION );
                if (NT_SUCCESS( status )) {

                    status = RtlSetDaclSecurityDescriptor( &securityDescriptor,
                                                           TRUE,
                                                           dacl,
                                                           FALSE );
                }
            }
        }
    }

    if (!NT_SUCCESS( status )) {
        ExFreePool( dacl );
        return status;
    }

    //
    // Open the ARC boot device and apply the ACL.
    //

    {
        NTSTATUS tmpStatus;
        UCHAR deviceNameBuffer[256];
        STRING deviceNameString;
        UNICODE_STRING deviceNameUnicodeString;
        HANDLE deviceHandle;
        IO_STATUS_BLOCK ioStatusBlock;

        //
        // Begin by formulating the ARC name of the boot device in the ARC
        // name space.
        //

        sprintf( deviceNameBuffer,
                 ArcNameFmt,
                 LoaderBlock->ArcHalDeviceName );

        RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

        status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                               &deviceNameString,
                                               TRUE );

        if (NT_SUCCESS( status )) {

            InitializeObjectAttributes( &objectAttributes,
                                        &deviceNameUnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            status = ZwOpenFile( &deviceHandle,
                                 WRITE_DAC,
                                 &objectAttributes,
                                 &ioStatusBlock,
                                 TRUE,
                                 0 );

            RtlFreeUnicodeString( &deviceNameUnicodeString );

            if (NT_SUCCESS( status )) {


                //
                // Apply the ACL built above to the system partition device
                // object.
                //

                status = ZwSetSecurityObject( deviceHandle,
                                              DACL_SECURITY_INFORMATION,
                                              &securityDescriptor );

                tmpStatus = NtClose( deviceHandle );
            }
        }
    }

    //
    // Free the memory used to hold the ACL.
    //

    ExFreePool( dacl );

    return status;
}
