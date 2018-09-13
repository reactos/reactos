/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    Pnp.c

Abstract:

    This module implements the Plug and Play routines for UDFS called by
    the dispatch driver.

Author:

    Dan Lovinger    [DanLo]     23-Jul-1997

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_PNP)

NTSTATUS
UdfPnpQueryRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UdfPnpRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UdfPnpSurpriseRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UdfPnpCancelRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UdfPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonPnp)
#pragma alloc_text(PAGE, UdfPnpCancelRemove)
#pragma alloc_text(PAGE, UdfPnpQueryRemove)
#pragma alloc_text(PAGE, UdfPnpRemove)
#pragma alloc_text(PAGE, UdfPnpSurpriseRemove)
#endif


NTSTATUS
UdfCommonPnp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing PnP operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    
    PIO_STACK_LOCATION IrpSp;

    PVOLUME_DEVICE_OBJECT OurDeviceObject;
    PVCB Vcb;

    //
    //  Get the current Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Find our Vcb.  This is tricky since we have no file object in the Irp.
    //

    OurDeviceObject = (PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject;

    //
    //  Make sure this device object really is big enough to be a volume device
    //  object.  If it isn't, we need to get out before we try to reference some
    //  field that takes us past the end of an ordinary device object.
    //
    
    if (OurDeviceObject->DeviceObject.Size != sizeof(VOLUME_DEVICE_OBJECT) ||
        NodeType( &OurDeviceObject->Vcb ) != UDFS_NTC_VCB) {
        
        //
        //  We were called with something we don't understand.
        //
        
        Status = STATUS_INVALID_PARAMETER;
        UdfCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Force all PnP operations to be synchronous.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    Vcb = &OurDeviceObject->Vcb;

    //
    //  Case on the minor code.
    //
    
    switch ( IrpSp->MinorFunction ) {

        case IRP_MN_QUERY_REMOVE_DEVICE:
            
            Status = UdfPnpQueryRemove( IrpContext, Irp, Vcb );
            break;
        
        case IRP_MN_SURPRISE_REMOVAL:
        
            Status = UdfPnpSurpriseRemove( IrpContext, Irp, Vcb );
            break;

        case IRP_MN_REMOVE_DEVICE:

            Status = UdfPnpRemove( IrpContext, Irp, Vcb );
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
    
            Status = UdfPnpCancelRemove( IrpContext, Irp, Vcb );
            break;

        default:
    
            //
            //  Just pass the IRP on.  As we do not need to be in the
            //  way on return, ellide ourselves out of the stack.
            //
            
            IoSkipCurrentIrpStackLocation( Irp );
    
            Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);
            
            //
            //  Cleanup our Irp Context.  The driver has completed the Irp.
            //
        
            UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
            
            break;
    }
        
    return Status;
}


NTSTATUS
UdfPnpQueryRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine handles the PnP query remove operation.  The filesystem
    is responsible for answering whether there are any reasons it sees
    that the volume can not go away (and the device removed).  Initiation
    of the dismount begins when we answer yes to this question.
    
    Query will be followed by a Cancel or Remove.

Arguments:

    Irp - Supplies the Irp to process
    
    Vcb - Supplies the volume being queried.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KEVENT Event;
    BOOLEAN VcbPresent = TRUE;

    //
    //  Having said yes to a QUERY, any communication with the
    //  underlying storage stack is undefined (and may block)
    //  until the bounding CANCEL or REMOVE is sent.
    //

    //
    //  Acquire the global resource so that we can try to vaporize
    //  the volume, and the vcb resource itself.
    //
    
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    Status = UdfLockVolumeInternal( IrpContext, Vcb, NULL );

    UdfReleaseVcb( IrpContext, Vcb );
    UdfAcquireUdfData( IrpContext );
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    if (NT_SUCCESS( Status )) {

        //
        //  We need to pass this down before starting the dismount, which
        //  could disconnect us immediately from the stack.
        //
        
        //
        //  Get the next stack location, and copy over the stack location
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );

        //
        //  Set up the completion routine
        //
    
        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( Irp,
                                UdfPnpCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Send the request and wait.
        //

        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {

            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = Irp->IoStatus.Status;
        }

        //
        //  Now if no one below us failed already, initiate the dismount
        //  on this volume, make it go away.  PnP needs to see our internal
        //  streams close and drop their references to the target device.
        //
        //  Since we were able to lock the volume, we are guaranteed to
        //  move this volume into dismount state and disconnect it from
        //  the underlying storage stack.  The force on our part is actually
        //  unnecesary, though complete.
        //
        //  What is not strictly guaranteed, though, is that the closes
        //  for the metadata streams take effect synchronously underneath
        //  of this call.  This would leave references on the target device
        //  even though we are disconnected!
        //

        if (NT_SUCCESS( Status )) {
            
            VcbPresent = UdfCheckForDismount( IrpContext, Vcb, TRUE );
    
            ASSERT( !VcbPresent || Vcb->VcbCondition == VcbDismountInProgress );
        }
    }
    
    //
    //  Release the Vcb if it could still remain.
    //
    //  Note: if everything else succeeded and the Vcb is persistent because the
    //  internal streams did not vaporize, we really need to pend this IRP off on
    //  the side until the dismount is completed.  I can't think of a reasonable
    //  case (in UDFS) where this would actually happen, though it might still need
    //  to be implemented.
    //
    //  The reason this is the case is that handles/fileobjects place a reference
    //  on the device objects they overly.  In the filesystem case, these references
    //  are on our target devices.  PnP correcly thinks that if references remain
    //  on the device objects in the stack that someone has a handle, and that this
    //  counts as a reason to not succeed the query - even though every interrogated
    //  driver thinks that it is OK.
    //
    
    ASSERT( !(NT_SUCCESS( Status ) && VcbPresent && Vcb->VcbReference != 0));
    
    if (VcbPresent) {

        UdfReleaseVcb( IrpContext, Vcb );
    }

    UdfReleaseUdfData( IrpContext );
    
    //
    //  Cleanup our IrpContext and complete the IRP if neccesary.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
UdfPnpRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine handles the PnP remove operation.  This is our notification
    that the underlying storage device for the volume we have is gone, and
    an excellent indication that the volume will never reappear. The filesystem
    is responsible for initiation or completion the dismount.
    
Arguments:

    Irp - Supplies the Irp to process
    
    Vcb - Supplies the volume being removed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KEVENT Event;
    BOOLEAN VcbPresent = TRUE;
    
    //
    //  REMOVE - a storage device is now gone.  We either got
    //  QUERY'd and said yes OR got a SURPRISE OR a storage
    //  stack failed to spin back up from a sleep/stop state
    //  (the only case in which this will be the first warning).
    //
    //  Note that it is entirely unlikely that we will be around
    //  for a REMOVE in the first two cases, as we try to intiate
    //  dismount.
    //
    
    //
    //  Acquire the global resource so that we can try to vaporize
    //  the volume, and the vcb resource itself.
    //
    
    UdfAcquireUdfData( IrpContext );
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  The device will be going away.  Remove our lock and find
    //  out if we ever had one in the first place.
    //

    Status = UdfUnlockVolumeInternal( IrpContext, Vcb, NULL );

    //
    //  If the volume had not been locked, we must invalidate the
    //  volume to ensure it goes away properly.  The remove will
    //  succeed.
    //

    if (!NT_SUCCESS( Status )) {

        UdfLockVcb( IrpContext, Vcb );
        
        if (Vcb->VcbCondition != VcbDismountInProgress) {
            Vcb->VcbCondition = VcbInvalid;
        }
        
        UdfUnlockVcb( IrpContext, Vcb );
        
        Status = STATUS_SUCCESS;
    }
    
    //
    //  We need to pass this down before starting the dismount, which
    //  could disconnect us immediately from the stack.
    //
    
    //
    //  Get the next stack location, and copy over the stack location
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    //  Set up the completion routine
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( Irp,
                            UdfPnpCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request and wait.
    //

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        Status = Irp->IoStatus.Status;
    }

    //
    //  Now make our dismount happen.  This may not vaporize the
    //  Vcb, of course, since there could be any number of handles
    //  outstanding if we were not preceeded by a QUERY.
    //
    //  PnP will take care of disconnecting this stack if we
    //  couldn't get off of it immediately.
    //

    VcbPresent = UdfCheckForDismount( IrpContext, Vcb, TRUE );

    //
    //  Release the Vcb if it could still remain.
    //
    
    if (VcbPresent) {

        UdfReleaseVcb( IrpContext, Vcb );
    }

    UdfReleaseUdfData( IrpContext );
    
    //
    //  Cleanup our IrpContext and complete the IRP.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
UdfPnpSurpriseRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine handles the PnP surprise remove operation.  This is another
    type of notification that the underlying storage device for the volume we
    have is gone, and is excellent indication that the volume will never reappear.
    The filesystem is responsible for initiation or completion the dismount.
    
    For the most part, only "real" drivers care about the distinction of a
    surprise remove, which is a result of our noticing that a user (usually)
    physically reached into the machine and pulled something out.
    
    Surprise will be followed by a Remove when all references have been shut down.

Arguments:

    Irp - Supplies the Irp to process
    
    Vcb - Supplies the volume being removed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KEVENT Event;
    BOOLEAN VcbPresent = TRUE;
    
    //
    //  SURPRISE - a device was physically yanked away without
    //  any warning.  This means external forces.
    //
    
    UdfAcquireUdfData( IrpContext );
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );
        
    //
    //  Invalidate the volume right now.
    //
    //  The intent here is to make every subsequent operation
    //  on the volume fail and grease the rails toward dismount.
    //  By definition there is no going back from a SURPRISE.
    //
        
    UdfLockVcb( IrpContext, Vcb );
    
    if (Vcb->VcbCondition != VcbDismountInProgress) {
        Vcb->VcbCondition = VcbInvalid;
    }
    
    UdfUnlockVcb( IrpContext, Vcb );
    
    //
    //  We need to pass this down before starting the dismount, which
    //  could disconnect us immediately from the stack.
    //
    
    //
    //  Get the next stack location, and copy over the stack location
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    //  Set up the completion routine
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( Irp,
                            UdfPnpCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request and wait.
    //

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        Status = Irp->IoStatus.Status;
    }
    
    //
    //  Now make our dismount happen.  This may not vaporize the
    //  Vcb, of course, since there could be any number of handles
    //  outstanding since this is an out of band notification.
    //

    VcbPresent = UdfCheckForDismount( IrpContext, Vcb, TRUE );
    
    //
    //  Release the Vcb if it could still remain.
    //
    
    if (VcbPresent) {

        UdfReleaseVcb( IrpContext, Vcb );
    }

    UdfReleaseUdfData( IrpContext );
    
    //
    //  Cleanup our IrpContext and complete the IRP.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


NTSTATUS
UdfPnpCancelRemove (
    PIRP_CONTEXT IrpContext,
    PIRP Irp,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine handles the PnP cancel remove operation.  This is our
    notification that a previously proposed remove (query) was eventually
    vetoed by a component.  The filesystem is responsible for cleaning up
    and getting ready for more IO.
    
Arguments:

    Irp - Supplies the Irp to process
    
    Vcb - Supplies the volume being removed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    //
    //  CANCEL - a previous QUERY has been rescinded as a result
    //  of someone vetoing.  Since PnP cannot figure out who may
    //  have gotten the QUERY (think about it: stacked drivers),
    //  we must expect to deal with getting a CANCEL without having
    //  seen the QUERY.
    //
    //  For UDFS, this is quite easy.  In fact, we can't get a
    //  CANCEL if the underlying drivers succeeded the QUERY since
    //  we disconnect the Vpb on our dismount initiation.  This is
    //  actually pretty important because if PnP could get to us
    //  after the disconnect we'd be thoroughly unsynchronized
    //  with respect to the Vcb getting torn apart - merely referencing
    //  the volume device object is insufficient to keep us intact.
    //
    
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );
    
    //
    //  Unlock the volume.  This is benign if we never had seen
    //  a QUERY.
    //

    (VOID) UdfUnlockVolumeInternal( IrpContext, Vcb, NULL );

    UdfReleaseVcb( IrpContext, Vcb );

    //
    //  Send the request.  The underlying driver will complete the
    //  IRP.  Since we don't need to be in the way, simply ellide
    //  ourselves out of the IRP stack.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    PKEVENT Event = (PKEVENT) Contxt;

    KeSetEvent( Event, 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Contxt );
}

