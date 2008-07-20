/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Cdfs Verification routines.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_VERFYSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdVerifyFcbOperation)
#pragma alloc_text(PAGE, CdVerifyVcb)
#endif


NTSTATUS
CdPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceToVerify
    )

/*++

Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  If the verify is successful then we send the originating
    Irp off to an Ex Worker Thread.  This routine is called from the exception handler.

    No file system resources are held when this routine is called.

Arguments:

    Irp - The irp to send off after all is well and done.

    Device - The real device needing verification.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    //

    if ((IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
        ((IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
         (IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME))) {

        return CdFsdPostRequest( IrpContext, Irp );
    }

    //
    //  Extract a pointer to the Vcb from the VolumeDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Vcb = &CONTAINING_RECORD( IrpSp->DeviceObject,
                              VOLUME_DEVICE_OBJECT,
                              DeviceObject )->Vcb;
    try {

        //
        //  Send down the verify FSCTL.  Note that this is sent to the
        //  currently mounted volume,  which may not be this one.
        //
        //  We will allow Raw to mount this volume if we were doing a
        //  an absolute DASD open.
        //

        Status = IoVerifyVolume( DeviceToVerify, CdOperationIsDasdOpen( IrpContext));

        //
        //  Acquire the Vcb so we're working with a stable VcbCondition.
        //
        
        CdAcquireVcbShared( IrpContext, Vcb, FALSE);
        
        //
        //  If the verify operation completed it will return
        //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
        //
        //  If CdVerifyVolume encountered an error during
        //  processing, it will return that error.  If we got
        //  STATUS_WRONG_VOLUME from the verify, and our volume
        //  is now mounted, commute the status to STATUS_SUCCESS.
        //

        if ((Status == STATUS_WRONG_VOLUME) &&
            (Vcb->VcbCondition == VcbMounted)) {

            Status = STATUS_SUCCESS;
        }
        else if ((STATUS_SUCCESS == Status) && (Vcb->VcbCondition != VcbMounted))  {

            //
            //  If the verify succeeded,  but our volume is not mounted,
            //  then some other volume is on the device. 
            //

            Status = STATUS_WRONG_VOLUME;
        } 
        
        //
        //  Do a quick unprotected check here.  The routine will do
        //  a safe check.  After here we can release the resource.
        //  Note that if the volume really went away, we will be taking
        //  the Reparse path.
        //

        //
        //  If the device might need to go away then call our dismount routine.
        //

        if (((Vcb->VcbCondition == VcbNotMounted) ||
             (Vcb->VcbCondition == VcbInvalid) ||
             (Vcb->VcbCondition == VcbDismountInProgress)) &&
            (Vcb->VcbReference <= CDFS_RESIDUAL_REFERENCE)) {

            CdReleaseVcb( IrpContext, Vcb);

            CdAcquireCdData( IrpContext );
            CdCheckForDismount( IrpContext, Vcb, FALSE );
            CdReleaseCdData( IrpContext );
        }
        else {

            CdReleaseVcb( IrpContext, Vcb);
        }

        //
        //  If this is a create and the verify succeeded then complete the
        //  request with a REPARSE status.
        //

        if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
            (IrpSp->FileObject->RelatedFileObject == NULL) &&
            ((Status == STATUS_SUCCESS) || (Status == STATUS_WRONG_VOLUME))) {

            Irp->IoStatus.Information = IO_REMOUNT;

            CdCompleteRequest( IrpContext, Irp, STATUS_REPARSE );
            Status = STATUS_REPARSE;
            Irp = NULL;
            IrpContext = NULL;

        //
        //  If there is still an error to process then call the Io system
        //  for a popup.
        //

        } else if ((Irp != NULL) && !NT_SUCCESS( Status )) {

            //
            //  Fill in the device object if required.
            //
            
            if (IoIsErrorUserInduced( Status ) ) {

                IoSetHardErrorOrVerifyDevice( Irp, DeviceToVerify );
            }

            CdNormalizeAndRaiseStatus( IrpContext, Status );
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if (IrpContext != NULL) {

            Status = CdFsdPostRequest( IrpContext, Irp );
        }

    } except(CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    return Status;
}


BOOLEAN
CdCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Force
    )

/*++

Routine Description:

    This routine is called to check if a volume is ready for dismount.  This
    occurs when only file system references are left on the volume.

    If the dismount is not currently underway and the user reference count
    has gone to zero then we can begin the dismount.

    If the dismount is in progress and there are no references left on the
    volume (we check the Vpb for outstanding references as well to catch
    any create calls dispatched to the file system) then we can delete
    the Vcb.

Arguments:

    Vcb - Vcb for the volume to try to dismount.
    
    Force - Whether we will force this volume to be dismounted.

Return Value:

    BOOLEAN - True if the Vcb was not gone by the time this function finished,
        False if it was deleted.
        
    This is only a trustworthy indication to the caller if it had the vcb
    exclusive itself.

--*/

{
    BOOLEAN UnlockVcb = TRUE;
    BOOLEAN VcbPresent = TRUE;
    KIRQL SavedIrql;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    ASSERT_EXCLUSIVE_CDDATA;

    //
    //  Acquire and lock this Vcb to check the dismount state.
    //

    CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  Lets get rid of any pending closes for this volume.
    //

    CdFspClose( Vcb );

    CdLockVcb( IrpContext, Vcb );

    //
    //  If the dismount is not already underway then check if the
    //  user reference count has gone to zero or we are being forced
    //  to disconnect.  If so start the teardown on the Vcb.
    //

    if (Vcb->VcbCondition != VcbDismountInProgress) {

        if (Vcb->VcbUserReference <= CDFS_RESIDUAL_USER_REFERENCE || Force) {

            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;
            VcbPresent = CdDismountVcb( IrpContext, Vcb );
        }

    //
    //  If the teardown is underway and there are absolutely no references
    //  remaining then delete the Vcb.  References here include the
    //  references in the Vcb and Vpb.
    //

    } else if (Vcb->VcbReference == 0) {

        IoAcquireVpbSpinLock( &SavedIrql );

        //
        //  If there are no file objects and no reference counts in the
        //  Vpb we can delete the Vcb.  Don't forget that we have the
        //  last reference in the Vpb.
        //

        if (Vcb->Vpb->ReferenceCount == 1) {

            IoReleaseVpbSpinLock( SavedIrql );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;
            CdDeleteVcb( IrpContext, Vcb );
            VcbPresent = FALSE;

        } else {

            IoReleaseVpbSpinLock( SavedIrql );
        }
    }

    //
    //  Unlock the Vcb if still held.
    //

    if (UnlockVcb) {

        CdUnlockVcb( IrpContext, Vcb );
    }

    //
    //  Release any resources still acquired.
    //

    if (VcbPresent) {

        CdReleaseVcb( IrpContext, Vcb );
    }

    return VcbPresent;
}


BOOLEAN
CdMarkDevForVerifyIfVcbMounted(
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine checks to see if the specified Vcb is currently mounted on
    the device or not.  If it is,  it sets the verify flag on the device, if
    not then the state is noted in the Vcb.

Arguments:

    Vcb - This is the volume to check.

Return Value:

    TRUE if the device has been marked for verify here,  FALSE otherwise.

--*/

{
    BOOLEAN Marked = FALSE;
    KIRQL SavedIrql;

    IoAcquireVpbSpinLock( &SavedIrql );
    
    if (Vcb->Vpb->RealDevice->Vpb == Vcb->Vpb)  {

        CdMarkRealDevForVerify( Vcb->Vpb->RealDevice);
        Marked = TRUE;
    }
    else {

        //
        //  Flag this to avoid the VPB spinlock in future passes.
        //
        
        SetFlag( Vcb->VcbState, VCB_STATE_VPB_NOT_ON_DEVICE);
    }
    
    IoReleaseVpbSpinLock( SavedIrql );

    return Marked;
}


VOID
CdVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine checks that the current Vcb is valid and currently mounted
    on the device.  It will raise on an error condition.

    We check whether the volume needs verification and the current state
    of the Vcb.

Arguments:

    Vcb - This is the volume to verify.

Return Value:

    None

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK Iosb;
    ULONG MediaChangeCount = 0;
    BOOLEAN ForceVerify = FALSE;
    BOOLEAN DevMarkedForVerify;
    KIRQL SavedIrql;

    PAGED_CODE();

    //
    //  Fail immediately if the volume is in the progress of being dismounted
    //  or has been marked invalid.
    //

    if ((Vcb->VcbCondition == VcbInvalid) ||
        ((Vcb->VcbCondition == VcbDismountInProgress) && 
         (IrpContext->MajorFunction != IRP_MJ_CREATE))) {

        CdRaiseStatus( IrpContext, STATUS_FILE_INVALID );
    }
    
    if (FlagOn( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA ))  {
        
        //
        //  Capture the real device verify state.
        //
        
        DevMarkedForVerify = CdRealDevNeedsVerify( Vcb->Vpb->RealDevice);

        //
        //  If the media is removable and the verify volume flag in the
        //  device object is not set then we want to ping the device
        //  to see if it needs to be verified.
        //

        if (Vcb->VcbCondition != VcbMountInProgress) {

            Status = CdPerformDevIoCtrl( IrpContext,
                                         IOCTL_CDROM_CHECK_VERIFY,
                                         Vcb->TargetDeviceObject,
                                         &MediaChangeCount,
                                         sizeof(ULONG),
                                         FALSE,
                                         FALSE,
                                         &Iosb );

            if (Iosb.Information != sizeof(ULONG)) {
        
                //
                //  Be safe about the count in case the driver didn't fill it in
                //
        
                MediaChangeCount = 0;
            }

            //
            //  There are four cases when we want to do a verify.  These are the
            //  first three.
            //
            //  1. We are mounted,  and the device has become empty
            //  2. The device has returned verify required (=> DO_VERIFY_VOL flag is
            //     set, but could be due to hardware condition)
            //  3. Media change count doesn't match the one in the Vcb
            //
            
            if (((Vcb->VcbCondition == VcbMounted) &&
                 CdIsRawDevice( IrpContext, Status )) 
                ||
                (Status == STATUS_VERIFY_REQUIRED)
                ||
                (NT_SUCCESS(Status) &&
                 (Vcb->MediaChangeCount != MediaChangeCount))) {

                //
                //  If we are currently the volume on the device then it is our
                //  responsibility to set the verify flag.  If we're not on the device,
                //  then we shouldn't touch the flag.
                //

                if (!FlagOn( Vcb->VcbState, VCB_STATE_VPB_NOT_ON_DEVICE) &&
                    !DevMarkedForVerify)  {

                     DevMarkedForVerify = CdMarkDevForVerifyIfVcbMounted( Vcb);
                }

                ForceVerify = TRUE;

                //
                //  NOTE that we no longer update the media change count here. We
                //  do so only when we've actually completed a verify at a particular
                //  change count value.
                //
            } 
        }

        //
        //  This is the 4th verify case.
        //
        //  We ALWAYS force CREATE requests on unmounted volumes through the 
        //  verify path.  These requests could have been in limbo between
        //  IoCheckMountedVpb and us when a verify/mount took place and caused
        //  a completely different fs/volume to be mounted.  In this case the
        //  checks above may not have caught the condition,  since we may already
        //  have verified (wrong volume) and decided that we have nothing to do.
        //  We want the requests to be re routed to the currently mounted volume,
        //  since they were directed at the 'drive',  not our volume.
        //

        if (NT_SUCCESS( Status) && !ForceVerify && 
            (IrpContext->MajorFunction == IRP_MJ_CREATE))  {

            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->Irp);

            ForceVerify = (IrpSp->FileObject->RelatedFileObject == NULL) &&
                          ((Vcb->VcbCondition == VcbDismountInProgress) ||
                           (Vcb->VcbCondition == VcbNotMounted));

            //
            //  Note that we don't touch the device verify flag here.  It required
            //  it would have been caught and set by the first set of checks.
            //
        }

        //
        //  Raise the verify / error if neccessary.
        //
        
        if (ForceVerify || !NT_SUCCESS( Status)) {

            IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                          Vcb->Vpb->RealDevice );
           
            CdRaiseStatus( IrpContext, ForceVerify ? STATUS_VERIFY_REQUIRED : Status);
        }
    }

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Vcb->VcbCondition) {

    case VcbNotMounted:

        IoSetHardErrorOrVerifyDevice( IrpContext->Irp, Vcb->Vpb->RealDevice );

        CdRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        break;

    case VcbInvalid:
    case VcbDismountInProgress :

        CdRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        break;
    }
}


BOOLEAN
CdVerifyFcbOperation (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to verify that the state of the Fcb is valid
    to allow the current operation to continue.  We use the state of the
    Vcb, target device and type of operation to determine this.

Arguments:

    IrpContext - IrpContext for the request.  If not present then we
        were called from the fast IO path.

    Fcb - Fcb to perform the request on.

Return Value:

    BOOLEAN - TRUE if the request can continue, FALSE otherwise.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVCB Vcb = Fcb->Vcb;
    PDEVICE_OBJECT RealDevice = Vcb->Vpb->RealDevice;
    PIRP Irp;

    PAGED_CODE();
    
    //
    //  Check that the fileobject has not been cleaned up.
    //
    
    if ( ARGUMENT_PRESENT( IrpContext ))  {

        PFILE_OBJECT FileObject;

        Irp = IrpContext->Irp;
        FileObject = IoGetCurrentIrpStackLocation( Irp)->FileObject;
        
        if ( FileObject && FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE))  {

            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  Following FAT,  we allow certain operations even on cleaned up
            //  file objects.  Everything else,  we fail.
            //
            
            if ( (FlagOn(Irp->Flags, IRP_PAGING_IO)) ||
                 (IrpSp->MajorFunction == IRP_MJ_CLOSE ) ||
                 (IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
                 ( (IrpSp->MajorFunction == IRP_MJ_READ) &&
                   FlagOn(IrpSp->MinorFunction, IRP_MN_COMPLETE) ) ) {

                NOTHING;

            } else {

                CdRaiseStatus( IrpContext, STATUS_FILE_CLOSED );
            }
        }
    }

    //
    //  Fail immediately if the volume is in the progress of being dismounted
    //  or has been marked invalid.
    //

    if ((Vcb->VcbCondition == VcbInvalid) ||
        (Vcb->VcbCondition == VcbDismountInProgress)) {

        if (ARGUMENT_PRESENT( IrpContext )) {

            CdRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        }

        return FALSE;
    }

    //
    //  Always fail if the volume needs to be verified.
    //

    if (CdRealDevNeedsVerify( RealDevice)) {

        if (ARGUMENT_PRESENT( IrpContext )) {

            IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                          RealDevice );

            CdRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
        }

        return FALSE;

    //
    //
    //  All operations are allowed on mounted.
    //

    } else if ((Vcb->VcbCondition == VcbMounted) ||
               (Vcb->VcbCondition == VcbMountInProgress)) {

        return TRUE;

    //
    //  Fail all requests for fast Io on other Vcb conditions.
    //

    } else if (!ARGUMENT_PRESENT( IrpContext )) {

        return FALSE;

    //
    //  The remaining case is VcbNotMounted.
    //  Mark the device to be verified and raise WRONG_VOLUME.
    //

    } else if (Vcb->VcbCondition == VcbNotMounted) {

        if (ARGUMENT_PRESENT( IrpContext )) {

            IoSetHardErrorOrVerifyDevice( IrpContext->Irp, RealDevice );
            CdRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        }

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
CdDismountVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called when all of the user references to a volume are
    gone.  We will initiate all of the teardown any system resources.

    If all of the references to this volume are gone at the end of this routine
    then we will complete the teardown of this Vcb and mark the current Vpb
    as not mounted.  Otherwise we will allocated a new Vpb for this device
    and keep the current Vpb attached to the Vcb.

Arguments:

    Vcb - Vcb for the volume to dismount.

Return Value:

    BOOLEAN - TRUE if we didn't delete the Vcb, FALSE otherwise.

--*/

{
    PVPB OldVpb;
    BOOLEAN VcbPresent = TRUE;
    KIRQL SavedIrql;

    BOOLEAN FinalReference;

    ASSERT_EXCLUSIVE_CDDATA;
    ASSERT_EXCLUSIVE_VCB( Vcb );

    CdLockVcb( IrpContext, Vcb );

    //
    //  We should only take this path once.
    //

    ASSERT( Vcb->VcbCondition != VcbDismountInProgress );

    //
    //  Mark the Vcb as DismountInProgress.
    //

    Vcb->VcbCondition = VcbDismountInProgress;

    if (Vcb->XASector != NULL) {

        CdFreePool( &Vcb->XASector );
        Vcb->XASector = 0;
        Vcb->XADiskOffset = 0;
    }

    //
    //  Remove our reference to the internal Fcb's.  The Fcb's will then
    //  be removed in the purge path below.
    //

    if (Vcb->RootIndexFcb != NULL) {

        Vcb->RootIndexFcb->FcbReference -= 1;
        Vcb->RootIndexFcb->FcbUserReference -= 1;
    }

    if (Vcb->PathTableFcb != NULL) {

        Vcb->PathTableFcb->FcbReference -= 1;
        Vcb->PathTableFcb->FcbUserReference -= 1;
    }

    if (Vcb->VolumeDasdFcb != NULL) {

        Vcb->VolumeDasdFcb->FcbReference -= 1;
        Vcb->VolumeDasdFcb->FcbUserReference -= 1;
    }

    CdUnlockVcb( IrpContext, Vcb );

    //
    //  Purge the volume.
    //

    CdPurgeVolume( IrpContext, Vcb, TRUE );

    //
    //  Empty the delayed and async close queues.
    //

    CdFspClose( Vcb );

    OldVpb = Vcb->Vpb;

    //
    //  Remove the mount volume reference.
    //

    CdLockVcb( IrpContext, Vcb );
    Vcb->VcbReference -= 1;

    //
    //  Acquire the Vpb spinlock to check for Vpb references.
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    //
    //  Remember if this is the last reference on this Vcb.  We incremented
    //  the count on the Vpb earlier so we get one last crack it.  If our
    //  reference has gone to zero but the vpb reference count is greater
    //  than zero then the Io system will be responsible for deleting the
    //  Vpb.
    //

    FinalReference = (BOOLEAN) ((Vcb->VcbReference == 0) &&
                                (OldVpb->ReferenceCount == 1));

    //
    //  There is a reference count in the Vpb and in the Vcb.  We have
    //  incremented the reference count in the Vpb to make sure that
    //  we have last crack at it.  If this is a failed mount then we
    //  want to return the Vpb to the IO system to use for the next
    //  mount request.
    //

    if (OldVpb->RealDevice->Vpb == OldVpb) {

        //
        //  If not the final reference then swap out the Vpb.  We must
        //  preserve the REMOVE_PENDING flag so that the device is
        //  not remounted in the middle of a PnP remove operation.
        //

        if (!FinalReference) {

            ASSERT( Vcb->SwapVpb != NULL );

            Vcb->SwapVpb->Type = IO_TYPE_VPB;
            Vcb->SwapVpb->Size = sizeof( VPB );
            Vcb->SwapVpb->RealDevice = OldVpb->RealDevice;

            Vcb->SwapVpb->RealDevice->Vpb = Vcb->SwapVpb;

            Vcb->SwapVpb->Flags = FlagOn( OldVpb->Flags, VPB_REMOVE_PENDING );

            IoReleaseVpbSpinLock( SavedIrql );

            //
            //  Indicate we used up the swap.
            //

            Vcb->SwapVpb = NULL;            

            CdUnlockVcb( IrpContext, Vcb );

        //
        //  We want to leave the Vpb for the IO system.  Mark it
        //  as being not mounted.  Go ahead and delete the Vcb as
        //  well.
        //

        } else {

            //
            //  Make sure to remove the last reference on the Vpb.
            //

            OldVpb->ReferenceCount -= 1;

            OldVpb->DeviceObject = NULL;
            ClearFlag( Vcb->Vpb->Flags, VPB_MOUNTED );
            ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED );

            //
            //  Clear the Vpb flag so we know not to delete it.
            //

            Vcb->Vpb = NULL;

            IoReleaseVpbSpinLock( SavedIrql );
            CdUnlockVcb( IrpContext, Vcb );
            CdDeleteVcb( IrpContext, Vcb );
            VcbPresent = FALSE;
        }

    //
    //  Someone has already swapped in a new Vpb.  If this is the final reference
    //  then the file system is responsible for deleting the Vpb.
    //

    } else if (FinalReference) {

        //
        //  Make sure to remove the last reference on the Vpb.
        //

        OldVpb->ReferenceCount -= 1;

        IoReleaseVpbSpinLock( SavedIrql );
        CdUnlockVcb( IrpContext, Vcb );
        CdDeleteVcb( IrpContext, Vcb );
        VcbPresent = FALSE;

    //
    //  The current Vpb is no longer the Vpb for the device (the IO system
    //  has already allocated a new one).  We leave our reference in the
    //  Vpb and will be responsible for deleting it at a later time.
    //

    } else {

        IoReleaseVpbSpinLock( SavedIrql );
        CdUnlockVcb( IrpContext, Vcb );
    }

    //
    //  Let our caller know whether the Vcb is still present.
    //

    return VcbPresent;
}

