/*++

Copyright (c) 1997-2006 Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module implements the shutdown routine for CDFS called by
    the dispatch driver.


--*/

#include "cdprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_SHUTDOWN)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonShutdown)
#endif

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonShutdown (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for handling shutdown operation called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    KEVENT Event;
    PLIST_ENTRY Links;
    PVCB Vcb;
    PIRP NewIrp;
    IO_STATUS_BLOCK Iosb;
    BOOLEAN VcbPresent;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Make sure we don't get any pop-ups.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS );

    //
    //  Initialize an event for doing calls down to
    //  our target device objects.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Indicate that shutdown has started.
    //

    SetFlag( CdData.Flags, CD_FLAGS_SHUTDOWN );

    //
    //  Get everyone else out of the way
    //

    CdAcquireCdData( IrpContext );

    //
    //  Now walk through all the mounted Vcb's and shutdown the target
    //  device objects.
    //

    Links = CdData.VcbQueue.Flink;

    while (Links != &CdData.VcbQueue) {

        Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks );

        //
        //  Move to the next link now since the current Vcb may be deleted.
        //

        Links = Links->Flink;

        //
        //  If we have already been called before for this volume
        //  (and yes this does happen), skip this volume as no writes
        //  have been allowed since the first shutdown.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_SHUTDOWN ) ||
            (Vcb->VcbCondition != VcbMounted)) {

            continue;
        }

        CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

        CdPurgeVolume( IrpContext, Vcb, FALSE );

        //
        //  Build an irp for this volume stack - our own irp is probably too small and
        //  each stack may have a different stack size.
        //

        NewIrp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                               Vcb->TargetDeviceObject,
                                               NULL,
                                               0,
                                               NULL,
                                               &Event,
                                               &Iosb );

        if (NewIrp != NULL) {

            Status = IoCallDriver( Vcb->TargetDeviceObject, NewIrp );

            if (Status == STATUS_PENDING) {

                (VOID)KeWaitForSingleObject( &Event,
                                             Executive,
                                             KernelMode,
                                             FALSE,
                                             NULL );
            }

            KeClearEvent( &Event );
        }

        SetFlag( Vcb->VcbState, VCB_STATE_SHUTDOWN );

        //
        //  Attempt to punch the volume down.
        //

        VcbPresent = CdCheckForDismount( IrpContext, Vcb, FALSE );

        if (VcbPresent) {

            CdReleaseVcb( IrpContext, Vcb );
        }
    }


    CdReleaseCdData( IrpContext );

    IoUnregisterFileSystem( CdData.FileSystemDeviceObject );
    IoDeleteDevice( CdData.FileSystemDeviceObject );
#ifdef __REACTOS__
    IoUnregisterFileSystem( CdData.HddFileSystemDeviceObject );
    IoDeleteDevice( CdData.HddFileSystemDeviceObject );
#endif

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


