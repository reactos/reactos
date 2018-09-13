/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for Udfs called by
    the dispatch driver.

Author:

    Dan Lovinger    [DanLo]     20-Jan-1997

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_VOLINFO)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_VOLINFO)

//
//  Local support routines
//

NTSTATUS
UdfQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UdfQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UdfQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UdfQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonQueryVolInfo)
#pragma alloc_text(PAGE, UdfQueryFsAttributeInfo)
#pragma alloc_text(PAGE, UdfQueryFsDeviceInfo)
#pragma alloc_text(PAGE, UdfQueryFsSizeInfo)
#pragma alloc_text(PAGE, UdfQueryFsVolumeInfo)
#endif


NTSTATUS
UdfCommonQueryVolInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
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

    TypeOfOpen = UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb );

    if (TypeOfOpen == UnopenedFileObject) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the Vcb for this volume.
    //

    UdfAcquireVcbShared( IrpContext, Fcb->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify the Vcb.
        //

        UdfVerifyVcb( IrpContext, Fcb->Vcb );

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {

        case FileFsSizeInformation:

            Status = UdfQueryFsSizeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsVolumeInformation:

            Status = UdfQueryFsVolumeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = UdfQueryFsDeviceInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = UdfQueryFsAttributeInfo( IrpContext, Fcb->Vcb, Irp->AssociatedIrp.SystemBuffer, &Length );
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    } finally {

        //
        //  Release the Vcb.
        //

        UdfReleaseVcb( IrpContext, Fcb->Vcb );
    }

    //
    //  Complete the request if we didn't raise.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
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

    //
    //  Fill in the data from the Vcb.
    //

    Buffer->VolumeCreationTime = Vcb->VolumeDasdFcb->Timestamps.CreationTime;
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
UdfQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
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

    //
    //  Fill in the output buffer.
    //

    Buffer->TotalAllocationUnits.QuadPart = LlBlocksFromBytes( Vcb, Vcb->VolumeDasdFcb->AllocationSize.QuadPart );

    Buffer->AvailableAllocationUnits.QuadPart = 0;
    Buffer->SectorsPerAllocationUnit = SectorsFromBytes( Vcb, BlockSize( Vcb ));
    Buffer->BytesPerSector = SectorSize( Vcb );

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
UdfQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
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

    //
    //  Update the output buffer.
    //

    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;
    Buffer->DeviceType = FILE_DEVICE_CD_ROM;

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
UdfQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
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

    //
    //  Fill out the fixed portion of the buffer.
    //

    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_UNICODE_ON_DISK;

    Buffer->MaximumComponentNameLength = 255;

    *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName );

    //
    //  Make sure we can copy full unicode characters.
    //

    ClearFlag( *Length, 1 );

    //
    //  Determine how much of the file system name will fit.
    //

    if (*Length >= 6) {

        BytesToCopy = 6;

    } else {

        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    *Length -= BytesToCopy;

    //
    //  Do the file system name.  We explicitly share this designation with all
    //  Microsoft implementations of the UDF filesystem - DO NOT CHANGE!
    //

    Buffer->FileSystemNameLength = BytesToCopy;

    RtlCopyMemory( &Buffer->FileSystemName[0], L"UDF", BytesToCopy );

    //
    //  And return to our caller
    //

    return Status;
}
