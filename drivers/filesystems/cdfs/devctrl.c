/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Cdfs
    called by the dispatch driver.


--*/

#include "cdprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_DEVCTRL)

//
//  Local support routines
//

//  Tell prefast this is a completion routine
IO_COMPLETION_ROUTINE CdDevCtrlCompletionRoutine;

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdDevCtrlCompletionRoutine (
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonDevControl)
#endif


NTSTATUS
CdCommonDevControl (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PAGED_CODE();

    //
    //  Extract and decode the file object.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TypeOfOpen = CdDecodeFileObject( IrpContext,
                                     IrpSp->FileObject,
                                     &Fcb,
                                     &Ccb );

    //
    //  The only type of opens we accept are user volume opens.
    //

    if (TypeOfOpen != UserVolumeOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_TOC) {

        //
        //  Verify the Vcb in this case to detect if the volume has changed.
        //

        CdVerifyVcb( IrpContext, Fcb->Vcb );

    //
    //  Handle the case of the disk type ourselves.
    //

    } else if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_DISK_TYPE) {

        //
        //  Verify the Vcb in this case to detect if the volume has changed.
        //

        CdVerifyVcb( IrpContext, Fcb->Vcb );

        //
        //  Check the size of the output buffer.
        //

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( CDROM_DISK_DATA )) {

            CdCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        //  Copy the data from the Vcb.
        //

        ((PCDROM_DISK_DATA) Irp->AssociatedIrp.SystemBuffer)->DiskData = Fcb->Vcb->DiskFlags;

        Irp->IoStatus.Information = sizeof( CDROM_DISK_DATA );
        CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Get the next stack location, and copy over the stack parameter
    //  information.
    //

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    *NextIrpSp = *IrpSp;

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            CdDevCtrlCompletionRoutine,
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

    CdCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdDevCtrlCompletionRoutine (
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
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


