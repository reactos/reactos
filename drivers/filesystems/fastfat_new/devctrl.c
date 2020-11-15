/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Fat
    called by the dispatch driver.


--*/

#include "fatprocs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

//
//  Local procedure prototypes
//

//
//  Tell prefast this is an IO_COMPLETION_ROUTINE
//
IO_COMPLETION_ROUTINE FatDeviceControlCompletionRoutine;

NTSTATUS
NTAPI
FatDeviceControlCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCommonDeviceControl)
#pragma alloc_text(PAGE, FatFsdDeviceControl)
#endif


_Function_class_(IRP_MJ_DEVICE_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdDeviceControl (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Device control operations

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatFsdDeviceControl\n", 0);

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    _SEH2_TRY {

        IrpContext = FatCreateIrpContext( Irp, CanFsdWait( Irp ));

        Status = FatCommonDeviceControl( IrpContext, Irp );

    } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  exception code
        //

        Status = FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdDeviceControl -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    KEVENT WaitEvent;
    PVOID CompletionContext = NULL;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatCommonDeviceControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %p\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatCommonDeviceControl -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  A few IOCTLs actually require some intervention on our part
    //

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:

        //
        //  This is sent by the Volume Snapshot driver (Lovelace).
        //  We flush the volume, and hold all file resources
        //  to make sure that nothing more gets dirty. Then we wait
        //  for the IRP to complete or cancel.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
        FatAcquireExclusiveVolume( IrpContext, Vcb );

        FatFlushAndCleanVolume( IrpContext,
                                Irp,
                                Vcb,
                                FlushWithoutPurge );

        KeInitializeEvent( &WaitEvent, NotificationEvent, FALSE );
        CompletionContext = &WaitEvent;

        //
        //  Get the next stack location, and copy over the stack location
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );

        //
        //  Set up the completion routine
        //

        IoSetCompletionRoutine( Irp,
                                FatDeviceControlCompletionRoutine,
                                CompletionContext,
                                TRUE,
                                TRUE,
                                TRUE );
        break;

    case IOCTL_DISK_COPY_DATA:

        //
        //  We cannot allow this IOCTL to be sent unless the volume is locked,
        //  since this IOCTL allows direct writing of data to the volume.
        //  We do allow kernel callers to force access via a flag.  A handle that
        //  issued a dismount can send this IOCTL as well.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) &&
            !FlagOn( IrpSp->Flags, SL_FORCE_DIRECT_WRITE ) &&
            !FlagOn( Ccb->Flags, CCB_FLAG_COMPLETE_DISMOUNT )) {

            FatCompleteRequest( IrpContext,
                                Irp,
                                STATUS_ACCESS_DENIED );

            DebugTrace(-1, Dbg, "FatCommonDeviceControl -> %08lx\n", STATUS_ACCESS_DENIED);
            return STATUS_ACCESS_DENIED;
        }

        break;

    case IOCTL_SCSI_PASS_THROUGH:
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
    case IOCTL_SCSI_PASS_THROUGH_EX:
    case IOCTL_SCSI_PASS_THROUGH_DIRECT_EX:

        //
        //  If someone is issuing a format unit command underneath us, then make
        //  sure we mark the device as needing verification when they close their
        //  handle.
        //

        if ((!FlagOn( IrpSp->FileObject->Flags, FO_FILE_MODIFIED ) ||
             !FlagOn( Ccb->Flags, CCB_FLAG_SENT_FORMAT_UNIT )) &&
             (Irp->AssociatedIrp.SystemBuffer != NULL)) {

            PCDB  Cdb = NULL;

            //
            //  If this is a 32 bit application running on 64 bit then thunk the
            //  input structures to grab the Cdb.
            //

#if defined (_WIN64) && defined(BUILD_WOW64_ENABLED)
            if (IoIs32bitProcess(Irp)) {

                if ( (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH) ||
                     (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT )) {

                    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof( SCSI_PASS_THROUGH32 )) {

                        Cdb = (PCDB)((PSCSI_PASS_THROUGH32)(Irp->AssociatedIrp.SystemBuffer))->Cdb;
                    }
                } else {

                    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof( SCSI_PASS_THROUGH32_EX )) {

                        Cdb = (PCDB)((PSCSI_PASS_THROUGH32_EX)(Irp->AssociatedIrp.SystemBuffer))->Cdb;
                    }
                }

            } else {
#endif
                if ( (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH) ||
                     (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT )) {

                    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof( SCSI_PASS_THROUGH )) {

                        Cdb = (PCDB)((PSCSI_PASS_THROUGH)(Irp->AssociatedIrp.SystemBuffer))->Cdb;
                    }
                } else {

                    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof( SCSI_PASS_THROUGH_EX )) {

                        Cdb = (PCDB)((PSCSI_PASS_THROUGH_EX)(Irp->AssociatedIrp.SystemBuffer))->Cdb;
                    }
                }

#if defined (_WIN64) && defined(BUILD_WOW64_ENABLED)
            }
#endif

            if ((Cdb != NULL) && (Cdb->AsByte[0] == SCSIOP_FORMAT_UNIT)) {

                SetFlag( Ccb->Flags, CCB_FLAG_SENT_FORMAT_UNIT );
                SetFlag( IrpSp->FileObject->Flags, FO_FILE_MODIFIED );
            }
        }

        //
        //  Fall through as we do not need to know the outcome of this operation.
        //

    default:

        //
        //  FAT doesn't need to see this on the way back, so skip ourselves.
        //

        IoSkipCurrentIrpStackLocation( Irp );
        break;
    }

    //
    //  Send the request.
    //

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING && CompletionContext) {

        KeWaitForSingleObject( &WaitEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        Status = Irp->IoStatus.Status;
    }

    //
    //  If we had a context, the IRP remains for us and we will complete it.
    //  Handle it appropriately.
    //

    if (CompletionContext) {

        //
        //  Release all the resources that we held because of a
        //  VOLSNAP_FLUSH_AND_HOLD. 
        //

        NT_ASSERT( IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES );

        FatReleaseVolume( IrpContext, Vcb );

        //
        //  If we had no context, the IRP will complete asynchronously.
        //

    } else {

        Irp = NULL;
    }

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatCommonDeviceControl -> %08lx\n", Status);

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NTAPI
FatDeviceControlCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
    )

{
    PKEVENT Event = (PKEVENT) Contxt;
    
    //
    //  If there is an event, this is a synch request. Signal and
    //  let I/O know this isn't done yet.
    //

    if (Event) {

        KeSetEvent( Event, 0, FALSE );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_SUCCESS;
}

