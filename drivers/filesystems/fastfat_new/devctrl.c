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


NTSTATUS
NTAPI
FatFsdDeviceControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
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
        //  execption code
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

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatCommonDeviceControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
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

        ASSERT( IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES );

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
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
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

