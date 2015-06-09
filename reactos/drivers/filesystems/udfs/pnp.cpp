////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Pnp.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*    This module implements the Plug and Play routines for UDF called by
*    the dispatch driver.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_PNP


NTSTATUS
UDFPnpQueryRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UDFPnpRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UDFPnpSurpriseRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
UDFPnpCancelRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    );

NTSTATUS
NTAPI
UDFPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
UDFCommonPnp (
    PtrUDFIrpContext PtrIrpContext,
    IN PIRP Irp
    );

/*
    This routine implements the FSD part of PnP operations

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists
    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

 */
NTSTATUS
UDFPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS RC;
    PtrUDFIrpContext PtrIrpContext = NULL;
    BOOLEAN AreWeTopLevel;

    KdPrint(("UDFPnp\n"));
    ASSERT(FALSE);

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);

    _SEH2_TRY {
        //  We expect there to never be a fileobject, in which case we will always
        //  wait.  Since at the moment we don't have any concept of pending Pnp
        //  operations, this is a bit nitpicky.
        
        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonPnp(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter( PtrIrpContext, _SEH2_GetExceptionInformation() )) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);
        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return RC;
}

/*
    This is the common routine for doing PnP operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation
 */
NTSTATUS
UDFCommonPnp (
    PtrUDFIrpContext PtrIrpContext,
    IN PIRP Irp
    )
{
    NTSTATUS RC;
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;
    KdPrint(("UDFCommonPnp\n"));

    _SEH2_TRY {
        // Get the current Irp stack location.
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        // Make sure this device object really is big enough to be a volume device
        // object.  If it isn't, we need to get out before we try to reference some
        // field that takes us past the end of an ordinary device object.
        Vcb = (PVCB)(IrpSp->DeviceObject->DeviceExtension);

        if (Vcb->NodeIdentifier.NodeType != UDF_NODE_TYPE_VCB) {
            // We were called with something we don't understand.
            if(Irp->Flags & IRP_INPUT_OPERATION) {
                Irp->IoStatus.Information = 0;
            }
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            IoCompleteRequest( Irp, IO_DISK_INCREMENT );
            try_return (RC = STATUS_INVALID_PARAMETER);
        }

        // Force everything to wait.
        PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_CAN_BLOCK;
        
        // Case on the minor code.
        switch ( IrpSp->MinorFunction ) {

            case IRP_MN_QUERY_REMOVE_DEVICE:
                RC = UDFPnpQueryRemove( PtrIrpContext, Irp, Vcb );
                break;
            
            case IRP_MN_SURPRISE_REMOVAL:
                RC = UDFPnpSurpriseRemove( PtrIrpContext, Irp, Vcb );
                break;

            case IRP_MN_REMOVE_DEVICE:
                RC = UDFPnpRemove( PtrIrpContext, Irp, Vcb );
                break;

/*            case IRP_MN_CANCEL_REMOVE_DEVICE:
                RC = UDFPnpCancelRemove( PtrIrpContext, Irp, Vcb );
                break;*/

            default:
                KdPrint(("UDFCommonPnp: pass through\n"));
                //  Just pass the IRP on.  As we do not need to be in the
                //  way on return, ellide ourselves out of the stack.
                IoSkipCurrentIrpStackLocation( Irp );
                RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);
                ASSERT(RC != STATUS_PENDING);
                
                break;
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        UDFReleaseIrpContext(PtrIrpContext);
    } _SEH2_END;

    return RC;
}


/*
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
 */
NTSTATUS
UDFPnpQueryRemove(
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    )
{
    NTSTATUS RC;
    KEVENT Event;
    BOOLEAN VcbDeleted = FALSE;
    BOOLEAN GlobalHeld = FALSE;
    BOOLEAN VcbAcquired = FALSE;
    PPREVENT_MEDIA_REMOVAL_USER_IN Buf = NULL;

    //  Having said yes to a QUERY, any communication with the
    //  underlying storage stack is undefined (and may block)
    //  until the bounding CANCEL or REMOVE is sent.

    _SEH2_TRY {

        //  Acquire the global resource so that we can try to vaporize
        //  the volume, and the vcb resource itself.
        UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);
        GlobalHeld = TRUE;

        if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK))
            UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
#ifdef UDF_DELAYED_CLOSE
        UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

        UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
        VcbAcquired = TRUE;

        Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
        //  With the volume held locked, note that we must finalize as much
        //  as possible right now.
        UDFDoDismountSequence(Vcb, Buf, FALSE);

        // disable Eject Request Waiter if any
        UDFReleaseResource( &(Vcb->VCBResource) );
        VcbAcquired = FALSE;
        UDFStopEjectWaiter(Vcb);
        UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
        VcbAcquired = TRUE;

        //  We need to pass this down before starting the dismount, which
        //  could disconnect us immediately from the stack.

        //  Get the next stack location, and copy over the stack location
        IoCopyCurrentIrpStackLocationToNext( Irp );

        //  Set up the completion routine
        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( Irp,
                                UDFPnpCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );
        //  Send the request and wait.
        RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (RC == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            RC = Irp->IoStatus.Status;
        }

        //  Now if no one below us failed already, initiate the dismount
        //  on this volume, make it go away.  PnP needs to see our internal
        //  streams close and drop their references to the target device.

        //  Since we were able to lock the volume, we are guaranteed to
        //  move this volume into dismount state and disconnect it from
        //  the underlying storage stack.  The force on our part is actually
        //  unnecesary, though complete.

        //  What is not strictly guaranteed, though, is that the closes
        //  for the metadata streams take effect synchronously underneath
        //  of this call.  This would leave references on the target device
        //  even though we are disconnected!
        if (NT_SUCCESS( RC )) {
            VcbDeleted = !UDFCheckForDismount( PtrIrpContext, Vcb, TRUE );
            ASSERT( VcbDeleted );
        }

        //  Release the Vcb if it could still remain.

        //  Note: if everything else succeeded and the Vcb is persistent because the
        //  internal streams did not vaporize, we really need to pend this IRP off on
        //  the side until the dismount is completed.  I can't think of a reasonable
        //  case (in UDF) where this would actually happen, though it might still need
        //  to be implemented.
        //
        //  The reason this is the case is that handles/fileobjects place a reference
        //  on the device objects they overly.  In the filesystem case, these references
        //  are on our target devices.  PnP correcly thinks that if references remain
        //  on the device objects in the stack that someone has a handle, and that this
        //  counts as a reason to not succeed the query - even though every interrogated
        //  driver thinks that it is OK.
        ASSERT( !(NT_SUCCESS( RC ) && !VcbDeleted ));

    } _SEH2_FINALLY {
        
        if (!VcbDeleted && VcbAcquired) {
            UDFReleaseResource( &(Vcb->VCBResource) );
        }

        if (GlobalHeld) {
            UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );
        }

        if (!_SEH2_AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }
    } _SEH2_END;

    return RC;
} // end UDFPnpQueryRemove()


/*
Routine Description:
    This routine handles the PnP remove operation.  This is our notification
    that the underlying storage device for the volume we have is gone, and
    an excellent indication that the volume will never reappear. The filesystem
    is responsible for initiation or completion of the dismount.
    
Arguments:
    Irp - Supplies the Irp to process
    Vcb - Supplies the volume being removed.

Return Value:
    NTSTATUS - The return status for the operation

--*/
NTSTATUS
UDFPnpRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    )
{
    NTSTATUS RC;
    KEVENT Event;
    BOOLEAN VcbDeleted;
    BOOLEAN VcbAcquired;
    PPREVENT_MEDIA_REMOVAL_USER_IN Buf = NULL;
    
    //  REMOVE - a storage device is now gone.  We either got
    //  QUERY'd and said yes OR got a SURPRISE OR a storage
    //  stack failed to spin back up from a sleep/stop state
    //  (the only case in which this will be the first warning).
    //
    //  Note that it is entirely unlikely that we will be around
    //  for a REMOVE in the first two cases, as we try to intiate
    //  dismount.

    //  Acquire the global resource so that we can try to vaporize
    //  the volume, and the vcb resource itself.
    UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);

    if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK))
        UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
#ifdef UDF_DELAYED_CLOSE
    UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE
    
    UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
    VcbAcquired = TRUE;

    //  The device will be going away.  Remove our lock (benign
    //  if we never had it).
    if((Vcb->Vpb->Flags & VPB_LOCKED) ||
       (Vcb->VolumeLockPID != (ULONG)-1) ) {
        Vcb->Vpb->Flags &= ~VPB_LOCKED;
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_LOCKED;
        Vcb->VolumeLockFileObject = NULL;
        Vcb->VolumeLockPID = -1;
        RC = STATUS_SUCCESS;
    }

    //  We need to pass this down before starting the dismount, which
    //  could disconnect us immediately from the stack.
    
    //  Get the next stack location, and copy over the stack location
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //  Set up the completion routine
    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( Irp,
                            UDFPnpCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //  Send the request and wait.
    RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    if (RC == STATUS_PENDING) {

        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        RC = Irp->IoStatus.Status;
    }

    _SEH2_TRY {
        
        //  Knock as many files down for this volume as we can.

        //  Now make our dismount happen.  This may not vaporize the
        //  Vcb, of course, since there could be any number of handles
        //  outstanding if we were not preceeded by a QUERY.
        //
        //  PnP will take care of disconnecting this stack if we
        //  couldn't get off of it immediately.
        Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
        Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
        if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        UDFDoDismountSequence(Vcb, Buf, FALSE);
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
        Vcb->WriteSecurity = FALSE;
        // disable Eject Request Waiter if any
        UDFReleaseResource( &(Vcb->VCBResource) );
        VcbAcquired = FALSE;

        UDFStopEjectWaiter(Vcb);

        VcbDeleted = !UDFCheckForDismount( PtrIrpContext, Vcb, FALSE );

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        //  Release the Vcb if it could still remain.
        if (!VcbDeleted && VcbAcquired) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));

        if(Buf)
            MyFreePool__(Buf);

        if (!_SEH2_AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }
    } _SEH2_END;

    return RC;
}


NTSTATUS
UDFPnpSurpriseRemove (
    PtrUDFIrpContext PtrIrpContext,
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
    NTSTATUS RC;
    KEVENT Event;
    BOOLEAN VcbDeleted;
    BOOLEAN VcbAcquired;
    PPREVENT_MEDIA_REMOVAL_USER_IN Buf = NULL;
    
    //  SURPRISE - a device was physically yanked away without
    //  any warning.  This means external forces.
    
    UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);

    if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK))
        UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
#ifdef UDF_DELAYED_CLOSE
    UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE
    
    UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
    VcbAcquired = TRUE;
        
    //  We need to pass this down before starting the dismount, which
    //  could disconnect us immediately from the stack.
    
    //  Get the next stack location, and copy over the stack location
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //  Set up the completion routine
    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( Irp,
                            UDFPnpCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //  Send the request and wait.
    RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    if (RC == STATUS_PENDING) {

        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        RC = Irp->IoStatus.Status;
    }
    
    _SEH2_TRY {
        //  Knock as many files down for this volume as we can.
        Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
        Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
        if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        UDFDoDismountSequence(Vcb, Buf, FALSE);
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
        Vcb->WriteSecurity = FALSE;

        UDFReleaseResource(&(Vcb->VCBResource));
        VcbAcquired = FALSE;

        UDFStopEjectWaiter(Vcb);

        //  Now make our dismount happen.  This may not vaporize the
        //  Vcb, of course, since there could be any number of handles
        //  outstanding since this is an out of band notification.
        VcbDeleted = !UDFCheckForDismount( PtrIrpContext, Vcb, FALSE );

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        
        //  Release the Vcb if it could still remain.
        if (!VcbDeleted && VcbAcquired) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));

        if(Buf)
            MyFreePool__(Buf);

        if (!_SEH2_AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }
    } _SEH2_END;

    return RC;
}

/*
NTSTATUS
UDFPnpCancelRemove (
    PtrUDFIrpContext PtrIrpContext,
    PIRP Irp,
    PVCB Vcb
    )

*/
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

/*{
    NTSTATUS RC;

    //  CANCEL - a previous QUERY has been rescinded as a result
    //  of someone vetoing.  Since PnP cannot figure out who may
    //  have gotten the QUERY (think about it: stacked drivers),
    //  we must expect to deal with getting a CANCEL without having
    //  seen the QUERY.
    //
    //  For UDF, this is quite easy.  In fact, we can't get a
    //  CANCEL if the underlying drivers succeeded the QUERY since
    //  we disconnect the Vpb on our dismount initiation.  This is
    //  actually pretty important because if PnP could get to us
    //  after the disconnect we'd be thoroughly unsynchronized
    //  with respect to the Vcb getting torn apart - merely referencing
    //  the volume device object is insufficient to keep us intact.
    
    UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
    
    //  Unlock the volume.  This is benign if we never had seen
    //  a QUERY.
    if(Vcb->Vpb->Flags & VPB_LOCKED) {
        Vcb->Vpb->Flags &= ~VPB_LOCKED;
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_LOCKED;
        Vcb->VolumeLockFileObject = NULL;
        RC = STATUS_SUCCESS;
    } else {
        RC = STATUS_NOT_LOCKED;
    }

    try {
        
        //  We must re-enable allocation support if we got through
        //  the first stages of a QUERY_REMOVE; i.e., we decided we
        //  could place a lock on the volume.
        if (NT_SUCCESS( RC )) {
            FatSetupAllocationSupport( PtrIrpContext, Vcb );
        }
    
    } finally {
        UDFReleaseResource(&(Vcb->VCBResource));
    }

    //  Send the request.  The underlying driver will complete the
    //  IRP.  Since we don't need to be in the way, simply ellide
    //  ourselves out of the IRP stack.
    IoSkipCurrentIrpStackLocation( Irp );

    RC = IoCallDriver(Vcb->TargetDeviceObject, Irp);

//    if (!AbnormalTermination()) {
        Irp->IoStatus.Status = RC;
        // Free up the Irp Context
        UDFReleaseIrpContext(PtrIrpContext);
        // complete the IRP
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
//    }

    return RC;
} */


//  Local support routine
NTSTATUS
NTAPI
UDFPnpCompletionRoutine (
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


