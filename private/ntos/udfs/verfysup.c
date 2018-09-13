/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Udfs Verification routines.

Author:

    Dan Lovinger    [DanLo]     18-July-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_VERFYSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_VERFYSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfVerifyFcbOperation)
#pragma alloc_text(PAGE, UdfVerifyVcb)
#endif


NTSTATUS
UdfPerformVerify (
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

        return UdfFsdPostRequest( IrpContext, Irp );
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

    //
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    //

    try {

        if (FlagOn( DeviceToVerify->Flags, DO_VERIFY_VOLUME )) {

            BOOLEAN AllowRawMount = FALSE;

            //
            //  We will allow Raw to mount this volume if we were doing a
            //  an absolute DASD open.
            //

            if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                (IrpSp->FileObject->FileName.Length == 0) &&
                (IrpSp->FileObject->RelatedFileObject == NULL)) {

                AllowRawMount = TRUE;
            }

            //
            //  If the IopMount in IoVerifyVolume did something, and
            //  this is an absolute open, force a reparse.
            //

            Status = IoVerifyVolume( DeviceToVerify, AllowRawMount );

            //
            //  If the verify operation completed it will return
            //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
            //
            //  If UdfVerifyVolume encountered an error during
            //  processing, it will return that error.  If we got
            //  STATUS_WRONG_VOLUME from the verify, and our volume
            //  is now mounted, commute the status to STATUS_SUCCESS.
            //

            if ((Status == STATUS_WRONG_VOLUME) &&
                (Vcb->VcbCondition == VcbMounted)) {

                Status = STATUS_SUCCESS;
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
                (Vcb->VcbReference <= Vcb->VcbResidualReference)) {

                UdfAcquireUdfData( IrpContext );
                UdfCheckForDismount( IrpContext, Vcb, FALSE );
                UdfReleaseUdfData( IrpContext );
            }

            //
            //  If this is a create and the verify succeeded then complete the
            //  request with a REPARSE status.
            //

            if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                (IrpSp->FileObject->RelatedFileObject == NULL) &&
                ((Status == STATUS_SUCCESS) || (Status == STATUS_WRONG_VOLUME))) {

                Irp->IoStatus.Information = IO_REMOUNT;

                UdfCompleteRequest( IrpContext, Irp, STATUS_REPARSE );
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

                UdfNormalizeAndRaiseStatus( IrpContext, Status );
            }
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if (IrpContext != NULL) {

            Status = UdfFsdPostRequest( IrpContext, Irp );
        }

    } except(UdfExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = UdfProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    return Status;
}


BOOLEAN
UdfCheckForDismount (
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

    ASSERT_EXCLUSIVE_UDFDATA;

    //
    //  Acquire and lock this Vcb to check the dismount state.
    //

    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  Lets get rid of any pending closes for this volume.
    //

    UdfFspClose( Vcb );

    UdfLockVcb( IrpContext, Vcb );

    //
    //  If the dismount is not already underway then check if the
    //  user reference count has gone to zero or we are being forced
    //  to disconnect.  If so start the teardown on the Vcb.
    //

    if (Vcb->VcbCondition != VcbDismountInProgress) {

        if (Vcb->VcbUserReference <= Vcb->VcbResidualUserReference || Force) {

            UdfUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;
            VcbPresent = UdfDismountVcb( IrpContext, Vcb );
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
            UdfUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;
            UdfDeleteVcb( IrpContext, Vcb );
            VcbPresent = FALSE;

        } else {

            IoReleaseVpbSpinLock( SavedIrql );
        }
    }

    //
    //  Unlock the Vcb if still held.
    //

    if (UnlockVcb) {

        UdfUnlockVcb( IrpContext, Vcb );
    }

    //
    //  Release any resources still acquired.
    //

    if (VcbPresent) {

        UdfReleaseVcb( IrpContext, Vcb );
    }

    return VcbPresent;
}


BOOLEAN
UdfDismountVcb (
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
    PVPB NewVpb;
    BOOLEAN VcbPresent = TRUE;
    KIRQL SavedIrql;

    BOOLEAN FinalReference;

    ASSERT_EXCLUSIVE_UDFDATA;
    ASSERT_EXCLUSIVE_VCB( Vcb );

    UdfLockVcb( IrpContext, Vcb );

    //
    //  We should only take this path once.
    //

    ASSERT( Vcb->VcbCondition != VcbDismountInProgress );

    //
    //  Mark the Vcb as DismountInProgress.
    //

    Vcb->VcbCondition = VcbDismountInProgress;

    //
    //  Remove our reference to the internal Fcb's.  The Fcb's will then
    //  be removed in the purge path below.
    //

    if (Vcb->RootIndexFcb != NULL) {

        Vcb->RootIndexFcb->FcbReference -= 1;
        Vcb->RootIndexFcb->FcbUserReference -= 1;
    }

    if (Vcb->MetadataFcb != NULL) {

        Vcb->MetadataFcb->FcbReference -= 1;
        Vcb->MetadataFcb->FcbUserReference -= 1;
    }

    if (Vcb->VatFcb != NULL) {

        Vcb->VatFcb->FcbReference -= 1;
        Vcb->VatFcb->FcbUserReference -= 1;
    }

    if (Vcb->VolumeDasdFcb != NULL) {

        Vcb->VolumeDasdFcb->FcbReference -= 1;
        Vcb->VolumeDasdFcb->FcbUserReference -= 1;
    }

    UdfUnlockVcb( IrpContext, Vcb );

    //
    //  Purge the volume.
    //

    UdfPurgeVolume( IrpContext, Vcb, TRUE );

    //
    //  Empty the delayed and async close queues.
    //

    UdfFspClose( Vcb );

    //
    //  Allocate a new Vpb in case we will need it.
    //

    NewVpb = ExAllocatePoolWithTag( NonPagedPoolMustSucceed, sizeof( VPB ), TAG_VPB );
    RtlZeroMemory( NewVpb, sizeof( VPB ) );

    OldVpb = Vcb->Vpb;

    //
    //  Remove the mount volume reference.
    //

    UdfLockVcb( IrpContext, Vcb );
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

            NewVpb->Type = IO_TYPE_VPB;
            NewVpb->Size = sizeof( VPB );
            NewVpb->RealDevice = OldVpb->RealDevice;

            NewVpb->RealDevice->Vpb = NewVpb;

            NewVpb->Flags = FlagOn( OldVpb->Flags, VPB_REMOVE_PENDING );
            
            NewVpb = NULL;
            IoReleaseVpbSpinLock( SavedIrql );
            UdfUnlockVcb( IrpContext, Vcb );

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

            //
            //  Clear the Vpb flag so we know not to delete it.
            //

            Vcb->Vpb = NULL;

            IoReleaseVpbSpinLock( SavedIrql );
            UdfUnlockVcb( IrpContext, Vcb );
            UdfDeleteVcb( IrpContext, Vcb );
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
        UdfUnlockVcb( IrpContext, Vcb );
        UdfDeleteVcb( IrpContext, Vcb );
        VcbPresent = FALSE;

    //
    //  The current Vpb is no longer the Vpb for the device (the IO system
    //  has already allocated a new one).  We leave our reference in the
    //  Vpb and will be responsible for deleting it at a later time.
    //

    } else {

        OldVpb->DeviceObject = NULL;
        ClearFlag( Vcb->Vpb->Flags, VPB_MOUNTED );

        IoReleaseVpbSpinLock( SavedIrql );
        UdfUnlockVcb( IrpContext, Vcb );
    }

    //
    //  Deallocate the new Vpb if we don't need it.
    //

    if (NewVpb != NULL) {

        UdfFreePool( &NewVpb );
    }

    //
    //  Let our caller know whether the Vcb is still present.
    //

    return VcbPresent;
}


VOID
UdfVerifyVcb (
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
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    ULONG MediaChangeCount = 0;

    PAGED_CODE();

    //
    //  Fail immediately if the volume is in the progress of being dismounted
    //  or has been marked invalid.
    //

    if ((Vcb->VcbCondition == VcbInvalid) ||
        (Vcb->VcbCondition == VcbDismountInProgress)) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_INVALID );
    }

    //
    //  If the media is removable and the verify volume flag in the
    //  device object is not set then we want to ping the device
    //  to see if it needs to be verified
    //

    if ((Vcb->VcbCondition != VcbMountInProgress) &&
        FlagOn( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA ) &&
        !FlagOn( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        Status = UdfPerformDevIoCtrl( IrpContext,
                                      ( Vcb->Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
                                        IOCTL_CDROM_CHECK_VERIFY :
                                        IOCTL_DISK_CHECK_VERIFY ),
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
        //  If the volume is now an empty device, or we have receieved a
        //  bare STATUS_VERIFY_REQUIRED (various hardware conditions such
        //  as bus resets, etc., will trigger this in the drivers), or the
        //  media change count has moved since we last inspected the device,
        //  then mark the volume to be verified.
        //      

        if ((Vcb->VcbCondition == VcbMounted &&
             UdfIsRawDevice( IrpContext, Status )) ||
            (Status == STATUS_VERIFY_REQUIRED) ||
            (NT_SUCCESS(Status) &&
             (Vcb->MediaChangeCount != MediaChangeCount))) {

            SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

            //
            //  If the volume is not mounted and we got a media change count,
            //  update the Vcb so we do not trigger a verify again at this
            //  count value.  If the verify->mount path detects that the media
            //  has actually changed and this Vcb is valid again, this will have
            //  done nothing.  We are already synchronized since the caller has
            //  the Vcb.
            //

            if ((Vcb->VcbCondition == VcbNotMounted) &&
                NT_SUCCESS(Status)) {

                Vcb->MediaChangeCount = MediaChangeCount;
            }

        //
        //  Raise the error condition otherwise.
        //

        } else if (!NT_SUCCESS( Status )) {

            UdfNormalizeAndRaiseStatus( IrpContext, Status );
        }

    }

    //
    //  The Vcb may be mounted but the underlying real device may need to be verified.
    //  If it does then we'll set the Iosb in the irp to be our real device
    //  and raise Verify required
    //

    if (FlagOn( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                      Vcb->Vpb->RealDevice );

        UdfRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
    }

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Vcb->VcbCondition) {

    case VcbNotMounted:

        SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

        IoSetHardErrorOrVerifyDevice( IrpContext->Irp, Vcb->Vpb->RealDevice );

        UdfRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        break;

    case VcbInvalid:
    case VcbDismountInProgress :

        UdfRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        break;
    }

    return;
}


BOOLEAN
UdfVerifyFcbOperation (
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

    PAGED_CODE();

    //
    //  Fail immediately if the volume is in the progress of being dismounted
    //  or has been marked invalid.
    //

    if ((Vcb->VcbCondition == VcbInvalid) ||
        (Vcb->VcbCondition == VcbDismountInProgress)) {

        if (ARGUMENT_PRESENT( IrpContext )) {

            UdfRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        }

        return FALSE;
    }

    //
    //  Always fail if the volume needs to be verified.
    //

    if (FlagOn( RealDevice->Flags, DO_VERIFY_VOLUME )) {

        if (ARGUMENT_PRESENT( IrpContext )) {

            IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                          RealDevice );

            UdfRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
        }

        return FALSE;

    //
    //  All operations are allowed on mounted volumes.
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

            SetFlag(RealDevice->Flags, DO_VERIFY_VOLUME);

            IoSetHardErrorOrVerifyDevice( IrpContext->Irp, RealDevice );
            UdfRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        }

        return FALSE;
    }

    return TRUE;
}


