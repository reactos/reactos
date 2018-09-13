/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Udfs
    called by the dispatch driver.

Author:

    Dan Lovinger    {DanLo]     28-Jan-1997

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_DEVCTRL)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_DEVCTRL)

//
//  Local support routines
//

NTSTATUS
UdfDvdReadStructure (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
UdfDvdTransferKey (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
UdfDevCtrlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonDevControl)
#pragma alloc_text(PAGE, UdfDvdReadStructure)
#pragma alloc_text(PAGE, UdfDvdTransferKey)
#endif


NTSTATUS
UdfCommonDevControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PIO_STACK_LOCATION IrpSp;

    PVOID TargetBuffer;

    PAGED_CODE();

    //
    //  Extract and decode the file object.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = UdfDecodeFileObject( IrpSp->FileObject,
                                      &Fcb,
                                      &Ccb );

    //
    //  A few IOCTLs actually require some intervention on our part to
    //  translate some information from file-based to device-based units.
    //

    if (TypeOfOpen == UserFileOpen) {

        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_DVD_READ_KEY:
            case IOCTL_DVD_SEND_KEY:

                Status = UdfDvdTransferKey( IrpContext, Irp, Fcb );
                break;

            case IOCTL_DVD_READ_STRUCTURE:

                Status = UdfDvdReadStructure( IrpContext, Irp, Fcb );
                break;

            case IOCTL_STORAGE_SET_READ_AHEAD:

                //
                //  We're just going to no-op this for now.
                //
                
                Status = STATUS_SUCCESS;
                UdfCompleteRequest( IrpContext, Irp, Status );
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                UdfCompleteRequest( IrpContext, Irp, Status );
                break;
        }

        return Status;
    }

    //
    //  Now the only type of opens we accept are user volume opens.
    //

    if (TypeOfOpen != UserVolumeOpen) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Handle the case of the disk type ourselves.  We're really just going to
    //  lie about this, but it is a good lie.
    //

    if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_DISK_TYPE) {

        //
        //  Verify the Vcb in this case to detect if the volume has changed.
        //

        UdfVerifyVcb( IrpContext, Fcb->Vcb );

        //
        //  Check the size of the output buffer.
        //

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( CDROM_DISK_DATA )) {

            UdfCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        //  Copy the data from the Vcb.
        //

        ((PCDROM_DISK_DATA) Irp->AssociatedIrp.SystemBuffer)->DiskData = CDROM_DISK_DATA_TRACK;

        Irp->IoStatus.Information = sizeof( CDROM_DISK_DATA );
        UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Copy the arguments and set up the completion routine
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );
    
    IoSetCompletionRoutine( Irp,
                            UdfDevCtrlCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver( IrpContext->Vcb->TargetDeviceObject, Irp );

    //
    //  Cleanup our Irp Context.  The driver has completed the Irp.
    //

    UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return Status;
}


NTSTATUS
UdfDvdTransferKey (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine handles the special form of the Dvd key negotiation IOCTLs
    performed in the context of a file.  For these IOCTLs, the incoming parameter
    is in file-relative form, which must be translated to a device-relatvie form
    before it can continue.

Arguments:

    Irp - Supplies the Irp to process
    
    Fcb - Supplies the file being operated with

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PDVD_COPY_PROTECT_KEY TransferKey;

    LARGE_INTEGER Offset;
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;

    //
    //  Grab the input buffer and confirm basic validity.
    //
    
    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TransferKey = (PDVD_COPY_PROTECT_KEY) Irp->AssociatedIrp.SystemBuffer;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(DVD_COPY_PROTECT_KEY) ||
        TransferKey->Parameters.TitleOffset.QuadPart > Fcb->FileSize.QuadPart) {

        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Now, convert the file byte offset in the structure to a physical sector.
    //

    Result = FsRtlLookupLargeMcbEntry( &Fcb->Mcb,
                                       LlSectorsFromBytes( Fcb->Vcb, TransferKey->Parameters.TitleOffset.QuadPart ),
                                       &Offset.QuadPart,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL );

    //
    //  If we failed the lookup, we know that this must be some form of unrecorded
    //  extent on the media.  This IOCTL is ill-defined at this point, so we have
    //  to give up.
    //
    
    if (!Result || Offset.QuadPart == -1) {
        
        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }
    
    //
    //  The input is buffered from user space, so we know we can just rewrite it.
    //

    TransferKey->Parameters.TitleOffset.QuadPart = LlBytesFromSectors( Fcb->Vcb, Offset.QuadPart );

    //
    //  Copy the arguments and set up the completion routine
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine( Irp,
                            UdfDevCtrlCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver( IrpContext->Vcb->TargetDeviceObject, Irp );

    //
    //  Cleanup our Irp Context.  The driver has completed the Irp.
    //

    UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return Status;
}


NTSTATUS
UdfDvdReadStructure (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb

    )

/*++

Routine Description:

    This routine handles the special form of the Dvd structure reading IOCTLs
    performed in the context of a file.  For these IOCTLs, the incoming parameter
    is in file-relative form, which must be translated to a device-relatvie form
    before it can continue.

Arguments:

    Irp - Supplies the Irp to process
    
    Fcb - Supplies the file being operated with

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PDVD_READ_STRUCTURE ReadStructure;

    LARGE_INTEGER Offset;
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;
    
    //
    //  Grab the input buffer and confirm basic validity.
    //
    
    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    ReadStructure = (PDVD_READ_STRUCTURE) Irp->AssociatedIrp.SystemBuffer;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_READ_STRUCTURE)) {

        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Now, convert the file byte offset in the structure to a physical sector.
    //

    Result = FsRtlLookupLargeMcbEntry( &Fcb->Mcb,
                                       LlSectorsFromBytes( Fcb->Vcb, ReadStructure->BlockByteOffset.QuadPart ),
                                       &Offset.QuadPart,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL );

    //
    //  If we failed the lookup, we know that this must be some form of unrecorded
    //  extent on the media.  This IOCTL is ill-defined at this point, so we have
    //  to give up.
    //
    
    if (!Result || Offset.QuadPart == -1) {
        
        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }
    
    //
    //  The input is buffered from user space, so we know we can just rewrite it.
    //

    ReadStructure->BlockByteOffset.QuadPart = LlBytesFromSectors( Fcb->Vcb, Offset.QuadPart );

    //
    //  Copy the arguments and set up the completion routine
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine( Irp,
                            UdfDevCtrlCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver( IrpContext->Vcb->TargetDeviceObject, Irp );

    //
    //  Cleanup our Irp Context.  The driver has completed the Irp.
    //

    UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfDevCtrlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Add the hack-o-ramma to fix formats.
    //

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Contxt );
}

