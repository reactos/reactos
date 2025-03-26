/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for Cdfs called by
    the dispatch driver.


--*/

#include "cdprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_VOLINFO)

//
//  Local support routines
//

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdQueryFsVolumeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_VOLUME_INFORMATION Buffer,
    _Inout_ PULONG Length
    );

NTSTATUS
CdQueryFsSizeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_SIZE_INFORMATION Buffer,
    _Inout_ PULONG Length
    );

NTSTATUS
CdQueryFsDeviceInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_DEVICE_INFORMATION Buffer,
    _Inout_ PULONG Length
    );

NTSTATUS
CdQueryFsAttributeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    _Inout_ PULONG Length
    );

#ifdef __REACTOS__
#define PFILE_FS_SECTOR_SIZE_INFORMATION PVOID
#endif

NTSTATUS
CdQueryFsSectorSizeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_writes_bytes_(*Length) PFILE_FS_SECTOR_SIZE_INFORMATION Buffer,
    _Inout_ PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonQueryVolInfo)
#pragma alloc_text(PAGE, CdQueryFsAttributeInfo)
#pragma alloc_text(PAGE, CdQueryFsDeviceInfo)
#pragma alloc_text(PAGE, CdQueryFsSizeInfo)
#pragma alloc_text(PAGE, CdQueryFsVolumeInfo)
#pragma alloc_text(PAGE, CdQueryFsSectorSizeInfo)
#endif


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonQueryVolInfo (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ULONG Length;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;

    //
    //  Decode the file object and fail if this an unopened file object.
    //

    TypeOfOpen = CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    if (TypeOfOpen == UnopenedFileObject) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the Vcb for this volume.
    //

    CdAcquireVcbShared( IrpContext, Fcb->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    _SEH2_TRY {

        //
        //  Verify the Vcb.
        //

        CdVerifyVcb( IrpContext, Fcb->Vcb );

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

#ifdef __REACTOS__
        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, Length);
#endif // __REACTOS__

        switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {

        case FileFsSizeInformation:

            Status = CdQueryFsSizeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsVolumeInformation:

            Status = CdQueryFsVolumeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = CdQueryFsDeviceInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = CdQueryFsAttributeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

#if (NTDDI_VERSION >= NTDDI_WIN8)
        case FileFsSectorSizeInformation:

            Status = CdQueryFsSectorSizeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;
#endif

        /* ReactOS Change: GCC "enumeration value not handled in switch" */
        default: break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    } _SEH2_FINALLY {

        //
        //  Release the Vcb.
        //

        CdReleaseVcb( IrpContext, Fcb->Vcb );
    } _SEH2_END;

    //
    //  Complete the request if we didn't raise.
    //

    CdCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local support routine
//

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdQueryFsVolumeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_VOLUME_INFORMATION Buffer,
    _Inout_ PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Fill in the data from the Vcb.
    //

    Buffer->VolumeCreationTime = *((PLARGE_INTEGER) &Vcb->VolumeDasdFcb->CreationTime);
    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET( FILE_FS_VOLUME_INFORMATION, VolumeLabel[0] );

    //
    //  Check if the buffer we're given is long enough
    //

    if (*Length >= (ULONG) Vcb->Vpb->VolumeLabelLength) {

        BytesToCopy = Vcb->Vpb->VolumeLabelLength;

    } else {

        BytesToCopy = *Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    Buffer->VolumeLabelLength = BytesToCopy;

    if (BytesToCopy) {

        RtlCopyMemory( &Buffer->VolumeLabel[0],
                       &Vcb->Vpb->VolumeLabel[0],
                       BytesToCopy );
    }

    *Length -= BytesToCopy;

    //
    //  Set our status and return to our caller
    //

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdQueryFsSizeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_SIZE_INFORMATION Buffer,
    _Inout_ PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume size call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Fill in the output buffer.
    //

    Buffer->TotalAllocationUnits.QuadPart = LlSectorsFromBytes( Vcb->VolumeDasdFcb->AllocationSize.QuadPart );

    Buffer->AvailableAllocationUnits.QuadPart = 0;
    Buffer->SectorsPerAllocationUnit = 1;
    Buffer->BytesPerSector = SECTOR_SIZE;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof( FILE_FS_SIZE_INFORMATION );

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdQueryFsDeviceInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_DEVICE_INFORMATION Buffer,
    _Inout_ PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Update the output buffer.
    //

    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;
#ifndef __REACTOS__
    Buffer->DeviceType = FILE_DEVICE_CD_ROM;
#else
    Buffer->DeviceType = Vcb->TargetDeviceObject->DeviceType;
#endif

    //
    //  Adjust the length variable
    //

    *Length -= sizeof( FILE_FS_DEVICE_INFORMATION );

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdQueryFsAttributeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_ PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    _Inout_ PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call.

Arguments:

    Vcb - Vcb for this volume.

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Vcb );

    //
    //  Fill out the fixed portion of the buffer.
    //

    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_READ_ONLY_VOLUME |
                                   FILE_SUPPORTS_OPEN_BY_FILE_ID;

    if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_JOLIET )) {

        SetFlag( Buffer->FileSystemAttributes, FILE_UNICODE_ON_DISK );

        Buffer->MaximumComponentNameLength = 110;

    } else {

        Buffer->MaximumComponentNameLength = 221;
    }

    *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName );

    //
    //  Make sure we can copy full unicode characters.
    //

    ClearFlag( *Length, 1 );

    //
    //  Determine how much of the file system name will fit.
    //

    if (*Length >= 8) {

        BytesToCopy = 8;

    } else {

        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    *Length -= BytesToCopy;

    //
    //  Do the file system name.
    //

    Buffer->FileSystemNameLength = BytesToCopy;

    RtlCopyMemory( &Buffer->FileSystemName[0], L"CDFS", BytesToCopy );

    //
    //  And return to our caller
    //

    return Status;
}

#if (NTDDI_VERSION >= NTDDI_WIN8)

NTSTATUS
CdQueryFsSectorSizeInfo (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Out_writes_bytes_(*Length) PFILE_FS_SECTOR_SIZE_INFORMATION Buffer,
    _Inout_ PULONG Length
    )

/*++

Routine Description:

    This routine implements the query sector size information call
    This operation will work on any handle and requires no privilege.

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return receives the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    NTSTATUS Status;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Sufficient buffer size is guaranteed by the I/O manager or the
    //  originating kernel mode driver.
    //

    ASSERT( *Length >= sizeof( FILE_FS_SECTOR_SIZE_INFORMATION ));
    _Analysis_assume_( *Length >= sizeof( FILE_FS_SECTOR_SIZE_INFORMATION ));

    //
    //  Retrieve the sector size information
    //

    Status = FsRtlGetSectorSizeInformation( Vcb->Vpb->RealDevice,
                                            Buffer );

    //
    //  Adjust the length variable
    //

    if (NT_SUCCESS( Status )) {

        *Length -= sizeof( FILE_FS_SECTOR_SIZE_INFORMATION );
    }

    return Status;
}

#endif

