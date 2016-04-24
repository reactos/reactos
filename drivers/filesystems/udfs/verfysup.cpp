////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 Module Name: VerfySup.cpp

 Abstract:

    This module implements the UDF verification routines.

 Environment:

    Kernel mode only

*/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID    UDF_FILE_VERIFY_FS_CONTROL

/*
Routine Description:
    This routine checks that the current Vcb is valid and currently mounted
    on the device.  It will raise on an error condition.
    We check whether the volume needs verification and the current state
    of the Vcb.
Arguments:

    Vcb - This is the volume to verify.
*/

NTSTATUS
UDFVerifyVcb(
    IN PtrUDFIrpContext IrpContext,
    IN PVCB Vcb
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    IO_STATUS_BLOCK Iosb;
    ULONG MediaChangeCount = 0;
    BOOLEAN Nop = TRUE;
    BOOLEAN UnsafeIoctl = (Vcb->VCBFlags & UDF_VCB_FLAGS_UNSAFE_IOCTL) ? TRUE : FALSE;

    KdPrint(("UDFVerifyVCB: Modified=%d\n", Vcb->Modified));
    //  Fail immediately if the volume is in the progress of being dismounted
    //  or has been marked invalid.
    if (Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED) { 
        return STATUS_FILE_INVALID;
    }

    //  If the media is removable and the verify volume flag in the
    //  device object is not set then we want to ping the device
    //  to see if it needs to be verified
    if ( (Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA) &&
        !(Vcb->Vpb->RealDevice->Flags & DO_VERIFY_VOLUME) &&
        (!(Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_LOCKED) || UnsafeIoctl) ) {
        KdPrint(("UDFVerifyVCB: UnsafeIoctl=%d, locked=%d\n", UnsafeIoctl, (Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_LOCKED) ? 0 : 1));
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_UNSAFE_IOCTL;
        RC = UDFTSendIOCTL( IOCTL_STORAGE_CHECK_VERIFY,
                                     Vcb,
                                     NULL,0,
                                     &MediaChangeCount,sizeof(ULONG),
                                     FALSE,&Iosb );
    
        //  Be safe about the count in case the driver didn't fill it in
        if (Iosb.Information != sizeof(ULONG))  MediaChangeCount = 0;
        KdPrint(("  MediaChangeCount %d -> %d\n", Vcb->MediaChangeCount, MediaChangeCount));

        //  If the volume is now an empty device, or we have receieved a
        //  bare STATUS_VERIFY_REQUIRED (various hardware conditions such
        //  as bus resets, etc., will trigger this in the drivers), or the
        //  media change count has moved since we last inspected the device,
        //  then mark the volume to be verified.

        if ( (RC == STATUS_VERIFY_REQUIRED) ||
             (UDFIsRawDevice(RC) && (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) ||
             (NT_SUCCESS(RC) && (Vcb->MediaChangeCount != MediaChangeCount)) ||
             UnsafeIoctl) {

            KdPrint(("  set DO_VERIFY_VOLUME\n"));
            Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;

            //  If the volume is not mounted and we got a media change count,
            //  update the Vcb so we do not trigger a verify again at this
            //  count value.  If the verify->mount path detects that the media
            //  has actually changed and this Vcb is valid again, this will have
            //  done nothing.  We are already synchronized since the caller has
            //  the Vcb.
            if (!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) &&
                NT_SUCCESS(RC) ) {
                Vcb->MediaChangeCount = MediaChangeCount;
            }

        } else if (!NT_SUCCESS(RC)) {
//            Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
            KdPrint(("  UDFNormalizeAndRaiseStatus(%x)\n", RC));
            UDFNormalizeAndRaiseStatus(IrpContext,RC);
            ASSERT(Nop);
        }
    }

    KdPrint(("UDFVerifyVCB: Modified=%d\n", Vcb->Modified));
    //  The Vcb may be mounted but the underlying real device may need to be verified.
    //  If it does then we'll set the Iosb in the irp to be our real device
    if (Vcb->Vpb->RealDevice->Flags & DO_VERIFY_VOLUME) {

        KdPrint(("  DO_VERIFY_VOLUME -> IoSetHardErrorOrVerifyDevice()\n"));
        IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                      Vcb->Vpb->RealDevice );

        RC = STATUS_VERIFY_REQUIRED;
        KdPrint(("  UDFRaiseStatus()\n"));
        UDFRaiseStatus(IrpContext, RC);
        ASSERT(Nop);
    }

    KdPrint(("UDFVerifyVCB: Modified=%d\n", Vcb->Modified));
    if (!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) {
        KdPrint(("  !UDF_VCB_FLAGS_VOLUME_MOUNTED -> IoSetHardErrorOrVerifyDevice()\n"));
        Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
        IoSetHardErrorOrVerifyDevice( IrpContext->Irp, Vcb->Vpb->RealDevice );
        RC = STATUS_WRONG_VOLUME;
        KdPrint(("  UDFRaiseStatus()\n"));
        UDFRaiseStatus(IrpContext, RC);
//        UDFRaiseStatus(IrpContext, STATUS_UNRECOGNIZED_VOLUME);
        ASSERT(Nop);
    }
    if ((Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED)) {
        KdPrint(("  UDF_VCB_FLAGS_BEING_DISMOUNTED\n"));
        RC = STATUS_FILE_INVALID;
        UDFRaiseStatus( IrpContext, RC );
        ASSERT(Nop);
    }
    KdPrint(("UDFVerifyVcb: RC = %x\n", RC));

    return RC;
} // end UDFVerifyVcb()

/*

Routine Description:
    This routine performs the verify volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:
    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
NTSTATUS
UDFVerifyVolume(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PVPB Vpb = IrpSp->Parameters.VerifyVolume.Vpb;
    PVCB Vcb = (PVCB)IrpSp->Parameters.VerifyVolume.DeviceObject->DeviceExtension;
    PVCB NewVcb = NULL;
    IO_STATUS_BLOCK Iosb;
    ULONG MediaChangeCount = 0;
    NTSTATUS RC;
    ULONG Mode;
    BOOLEAN UnsafeIoctl = (Vcb->VCBFlags & UDF_VCB_FLAGS_UNSAFE_IOCTL) ? TRUE : FALSE;

    //  Update the real device in the IrpContext from the Vpb.  There was no available
    //  file object when the IrpContext was created.
    //    IrpContext->RealDevice = Vpb->RealDevice;
    KdPrint(("UDFVerifyVolume:\n"));

    //  Acquire shared global access, the termination handler for the
    //  following try statement will free the access.

    UDFAcquireResourceShared(&(UDFGlobalData.GlobalDataResource),TRUE);
    UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);

    _SEH2_TRY {

        KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
        // Check if the real device still needs to be verified.  If it doesn't
        // then obviously someone beat us here and already did the work
        // so complete the verify irp with success.  Otherwise reenable
        // the real device and get to work.
        if( !(Vpb->RealDevice->Flags & DO_VERIFY_VOLUME) &&
            ((Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_LOCKED) && !UnsafeIoctl) ) {
            KdPrint(("UDFVerifyVolume: STATUS_SUCCESS (1)\n"));
            try_return(RC = STATUS_SUCCESS);
        }
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_UNSAFE_IOCTL;
        // Verify that there is a disk here.
        RC = UDFPhSendIOCTL( IOCTL_STORAGE_CHECK_VERIFY,
                                 Vcb->TargetDeviceObject,
                                 NULL,0,
                                 &MediaChangeCount,sizeof(ULONG),
                                 TRUE,&Iosb );

        if(!NT_SUCCESS( RC )) {
            // If we will allow a raw mount then return WRONG_VOLUME to
            // allow the volume to be mounted by raw.
            if(FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {
                KdPrint(("UDFVerifyVolume: STATUS_WRONG_VOLUME (1)\n"));
                RC = STATUS_WRONG_VOLUME;
            }

            if(UDFIsRawDevice(RC)) {
                KdPrint(("UDFVerifyVolume: STATUS_WRONG_VOLUME (2)\n"));
                RC = STATUS_WRONG_VOLUME;
            }
            try_return( RC );
        }

        if(Iosb.Information != sizeof(ULONG)) {
            // Be safe about the count in case the driver didn't fill it in
            MediaChangeCount = 0;
        }

        KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
        KdPrint(("UDFVerifyVolume: MediaChangeCount=%x, Vcb->MediaChangeCount=%x, UnsafeIoctl=%x\n",
            MediaChangeCount, Vcb->MediaChangeCount, UnsafeIoctl));
        // Verify that the device actually saw a change. If the driver does not
        // support the MCC, then we must verify the volume in any case.
        if(MediaChangeCount == 0 ||
            (Vcb->MediaChangeCount != MediaChangeCount) ||
           UnsafeIoctl ) {

            KdPrint(("UDFVerifyVolume: compare\n"));

            NewVcb = (PVCB)MyAllocatePool__(NonPagedPool,sizeof(VCB));
            if(!NewVcb)
                try_return(RC=STATUS_INSUFFICIENT_RESOURCES);
            RtlZeroMemory(NewVcb,sizeof(VCB));

            NewVcb->TargetDeviceObject = Vcb->TargetDeviceObject;
            NewVcb->Vpb = Vpb;

            // Set the removable media flag based on the real device's
            // characteristics
            if(Vpb->RealDevice->Characteristics & FILE_REMOVABLE_MEDIA) {
                UDFSetFlag( NewVcb->VCBFlags, UDF_VCB_FLAGS_REMOVABLE_MEDIA );
            }

            RC = UDFGetDiskInfo(NewVcb->TargetDeviceObject,NewVcb);
            if(!NT_SUCCESS(RC)) try_return(RC);
            // Prevent modification attempts durring Verify
            NewVcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY |
                                UDF_VCB_FLAGS_MEDIA_READ_ONLY;
            // Compare physical parameters (phase 1)
            KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
            RC = UDFCompareVcb(Vcb,NewVcb, TRUE);
            if(!NT_SUCCESS(RC)) try_return(RC);

            if((Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
                Vcb->MountPhErrorCount > MOUNT_ERR_THRESHOLD ) {
                KdPrint(("UDFVerifyVolume: it was very BAD volume. Do not perform Logical check\n"));
                goto skip_logical_check;
            }
            // Initialize internal cache
            // in *** READ ONLY *** mode
            Mode = WCACHE_MODE_ROM;

            RC = WCacheInit__(&(NewVcb->FastCache),
                              UDFGlobalData.WCacheMaxFrames,
                              UDFGlobalData.WCacheMaxBlocks,
                              NewVcb->WriteBlockSize,
                              5, NewVcb->BlockSizeBits,
                              UDFGlobalData.WCacheBlocksPerFrameSh,
                              0/*NewVcb->FirstLBA*/, NewVcb->LastPossibleLBA, Mode,
                                  /*WCACHE_CACHE_WHOLE_PACKET*/ 0 |
                                  (Vcb->DoNotCompareBeforeWrite ? WCACHE_DO_NOT_COMPARE : 0) |
                                  WCACHE_MARK_BAD_BLOCKS | WCACHE_RO_BAD_BLOCKS, // speed up mount on bad disks
                              UDFGlobalData.WCacheFramesToKeepFree,
                              UDFTWrite, UDFTRead,
#ifdef UDF_ASYNC_IO
                          UDFTWriteAsync, UDFTReadAsync,
#else  //UDF_ASYNC_IO
                          NULL, NULL,
#endif //UDF_ASYNC_IO
                              UDFIsBlockAllocated, UDFUpdateVAT,
                              UDFWCacheErrorHandler);
            if(!NT_SUCCESS(RC)) try_return(RC);

            KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
            RC = UDFGetDiskInfoAndVerify(NewVcb->TargetDeviceObject,NewVcb);
            KdPrint(("  NewVcb->NSRDesc=%x\n", NewVcb->NSRDesc));
            if(!NT_SUCCESS(RC)) {
                if((Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
                   (NewVcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
                   !(NewVcb->NSRDesc & VRS_ISO9660_FOUND)) {
                    KdPrint(("UDFVerifyVolume: both are RAW -> remount\n", Vcb->Modified));
                    RC = STATUS_SUCCESS;
                    goto skip_logical_check;
                }
                if(RC == STATUS_UNRECOGNIZED_VOLUME) {
                    try_return(RC = STATUS_WRONG_VOLUME);
                }
                try_return(RC);
            }

            WCacheChFlags__(&(Vcb->FastCache),
                            WCACHE_CACHE_WHOLE_PACKET, // enable cache whole packet
                            WCACHE_MARK_BAD_BLOCKS | WCACHE_RO_BAD_BLOCKS);  // let user retry request on Bad Blocks

            NewVcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_MOUNTED;
            // Compare logical parameters (phase 2)
            KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
            RC = UDFCompareVcb(Vcb,NewVcb, FALSE);
            if(!NT_SUCCESS(RC)) try_return(RC);
            // We have unitialized WCache, so it is better to
            // force MOUNT_VOLUME call
            if(!WCacheIsInitialized__(&(Vcb->FastCache)))
                try_return(RC = STATUS_WRONG_VOLUME);

skip_logical_check:;

        }

        KdPrint(("UDFVerifyVolume: compared\n"));
        KdPrint(("UDFVerifyVolume: Modified=%d\n", Vcb->Modified));
        if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_LOCKED)) {
            KdPrint(("UDFVerifyVolume: set UDF_VCB_FLAGS_VOLUME_MOUNTED\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_MOUNTED;
            Vcb->SoftEjectReq = FALSE;
        }
        UDFClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

try_exit: NOTHING;

    } _SEH2_FINALLY {

        // Update the media change count to note that we have verified the volume
        // at this value
        Vcb->MediaChangeCount = MediaChangeCount;

        // If we got the wrong volume, mark the Vcb as not mounted.
        if(RC == STATUS_WRONG_VOLUME) {
            KdPrint(("UDFVerifyVolume: clear UDF_VCB_FLAGS_VOLUME_MOUNTED\n"));
            Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
            Vcb->WriteSecurity = FALSE;
//            ASSERT(!(Vcb->EjectWaiter));
            if(Vcb->EjectWaiter) {
                UDFReleaseResource(&(Vcb->VCBResource));
                UDFStopEjectWaiter(Vcb);
                UDFAcquireResourceExclusive(&(Vcb->VCBResource),TRUE);
            }
        } else
        if(NT_SUCCESS(RC) &&
           (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)){
            BOOLEAN CacheInitialized = FALSE;
            KdPrint(("    !!! VerifyVolume - QUICK REMOUNT !!!\n"));
            // Initialize internal cache
            CacheInitialized = WCacheIsInitialized__(&(Vcb->FastCache));
            if(!CacheInitialized) {
                Mode = WCACHE_MODE_ROM;
                RC = WCacheInit__(&(Vcb->FastCache),
                                  Vcb->WCacheMaxFrames,
                                  Vcb->WCacheMaxBlocks,
                                  Vcb->WriteBlockSize,
                                  5, Vcb->BlockSizeBits,
                              Vcb->WCacheBlocksPerFrameSh,
                              0/*Vcb->FirstLBA*/, Vcb->LastPossibleLBA, Mode,
                                  /*WCACHE_CACHE_WHOLE_PACKET*/ 0 |
                                  (Vcb->DoNotCompareBeforeWrite ? WCACHE_DO_NOT_COMPARE : 0) |
                                  (Vcb->CacheChainedIo ? WCACHE_CHAINED_IO : 0),
                              Vcb->WCacheFramesToKeepFree,
//                              UDFTWrite, UDFTRead,
                              UDFTWriteVerify, UDFTReadVerify,
#ifdef UDF_ASYNC_IO
                                  UDFTWriteAsync, UDFTReadAsync,
#else  //UDF_ASYNC_IO
                                  NULL, NULL,
#endif //UDF_ASYNC_IO
                                  UDFIsBlockAllocated, UDFUpdateVAT,
                                  UDFWCacheErrorHandler);
            }
            if(NT_SUCCESS(RC)) {
                if(!Vcb->VerifyCtx.VInited) {
                    RC = UDFVInit(Vcb);
                }
            }
            if(NT_SUCCESS(RC)) {

                if(!CacheInitialized) {
                    if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY)) {
                        if(!Vcb->CDR_Mode) {
                            if((Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK) ||
                               CdrwMediaClassEx_IsRAM(Vcb->MediaClassEx)) {
                                KdPrint(("UDFMountVolume: RAM mode\n"));
                                Mode = WCACHE_MODE_RAM;
                            } else {
                                KdPrint(("UDFMountVolume: RW mode\n"));
                                Mode = WCACHE_MODE_RW;
                            }
        /*                    if(FsDeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) {
                            } else {
                                Vcb->WriteSecurity = TRUE;
                            }*/
                        } else {
                            Mode = WCACHE_MODE_R;
                        }
                    }
                    WCacheSetMode__(&(Vcb->FastCache), Mode);

                    WCacheChFlags__(&(Vcb->FastCache),
                                    WCACHE_CACHE_WHOLE_PACKET, // enable cache whole packet
                                    WCACHE_MARK_BAD_BLOCKS | WCACHE_RO_BAD_BLOCKS);  // let user retry request on Bad Blocks
                }
                // we can't record ACL on old format disks
                if(!UDFNtAclSupported(Vcb)) {
                    Vcb->WriteSecurity = FALSE;
                    Vcb->UseExtendedFE = FALSE;
                }
                KdPrint(("UDFVerifyVolume: try start EjectWaiter\n"));
                RC = UDFStartEjectWaiter(Vcb);
                if(!NT_SUCCESS(RC)) {
                    KdPrint(("UDFVerifyVolume: start EjectWaiter failed\n"));
                    Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_MOUNTED;
                    Vcb->WriteSecurity = FALSE;
                }
            }
        }

        if(NewVcb) {
            // Release internal cache
            KdPrint(("UDFVerifyVolume: delete NewVcb\n"));
            WCacheFlushAll__(&(NewVcb->FastCache),NewVcb);
            WCacheRelease__(&(NewVcb->FastCache));

            ASSERT(!(NewVcb->EjectWaiter));
            // Waiter thread should be already stopped
            // if MediaChangeCount have changed
            ASSERT(!(Vcb->EjectWaiter));

            UDFCleanupVCB(NewVcb);
            MyFreePool__(NewVcb);
        }
        UDFReleaseResource(&(Vcb->VCBResource));
        UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));
    } _SEH2_END;

    // Complete the request if no exception.
    Irp->IoStatus.Information = 0;

    Irp->IoStatus.Status = RC;
    IoCompleteRequest(Irp,IO_DISK_INCREMENT);

    KdPrint(("UDFVerifyVolume: RC = %x\n", RC));

    return RC;
} // end UDFVerifyVolume ()

/*
Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  If the verify is successful then we send the originating
    Irp off to an Ex Worker Thread.  This routine is called from the exception handler.
    No file system resources are held when this routine is called.

Arguments:

    Irp - The irp to send off after all is well and done.
    Device - The real device needing verification.

*/
NTSTATUS
UDFPerformVerify(
    IN PtrUDFIrpContext IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceToVerify
    )
{

    PVCB Vcb;
    NTSTATUS RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    KdPrint(("UDFPerformVerify:\n"));
    if(!IrpContext) return STATUS_INVALID_PARAMETER;
    if(!Irp) return STATUS_INVALID_PARAMETER;

    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    if ((IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
       ((IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
        (IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME))) {

        return UDFPostRequest(IrpContext, Irp);
    }

    //  Extract a pointer to the Vcb from the VolumeDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Vcb = (PVCB)IrpSp->DeviceObject->DeviceExtension;

    KdPrint(("UDFPerformVerify: check\n"));
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    _SEH2_TRY {

        if (DeviceToVerify->Flags & DO_VERIFY_VOLUME) {

            //  If the IopMount in IoVerifyVolume did something, and
            //  this is an absolute open, force a reparse.
            RC = IoVerifyVolume( DeviceToVerify, FALSE );

            // Bug?
/*            if (UDFIsRawDevice(RC)) {
                RC = STATUS_WRONG_VOLUME;
            }*/

            //  If the verify operation completed it will return
            //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
            if (RC == STATUS_SUCCESS) {
                IrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_EXCEPTION;
            }
            //  If UDFVerifyVolume encountered an error during
            //  processing, it will return that error.  If we got
            //  STATUS_WRONG_VOLUME from the verify, and our volume
            //  is now mounted, commute the status to STATUS_SUCCESS.
            if ((RC == STATUS_WRONG_VOLUME) &&
                (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED)) {
                RC = STATUS_SUCCESS;
            }

            //  Do a quick unprotected check here.  The routine will do
            //  a safe check.  After here we can release the resource.
            //  Note that if the volume really went away, we will be taking
            //  the Reparse path.

            //  If the device might need to go away then call our dismount routine.
            if ( (!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) ||
                   (Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED)) &&
                   (Vcb->VCBOpenCount <= UDF_RESIDUAL_REFERENCE) )
            {
                KdPrint(("UDFPerformVerify: UDFCheckForDismount\n"));
                UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);
                UDFCheckForDismount( IrpContext, Vcb, FALSE );
                UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));
            }

            //  If this is a create and the verify succeeded then complete the
            //  request with a REPARSE status.
            if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                (IrpSp->FileObject->RelatedFileObject == NULL) &&
                ((RC == STATUS_SUCCESS) || (RC == STATUS_WRONG_VOLUME)) ) {

                KdPrint(("UDFPerformVerify: IO_REMOUNT\n"));

                Irp->IoStatus.Information = IO_REMOUNT;

                Irp->IoStatus.Status = STATUS_REPARSE;
                IoCompleteRequest(Irp,IO_DISK_INCREMENT);

                UDFReleaseIrpContext(IrpContext);

                RC = STATUS_REPARSE;
                Irp = NULL;
                IrpContext = NULL;

            //  If there is still an error to process then call the Io system
            //  for a popup.
            } else if ((Irp != NULL) && !NT_SUCCESS( RC )) {

                KdPrint(("UDFPerformVerify: check IoIsErrorUserInduced\n"));
                //  Fill in the device object if required.
                if (IoIsErrorUserInduced( RC ) ) {
                    IoSetHardErrorOrVerifyDevice( Irp, DeviceToVerify );
                }
                KdPrint(("UDFPerformVerify: UDFNormalizeAndRaiseStatus\n"));
                UDFNormalizeAndRaiseStatus( IrpContext, RC );
            }
        }

        //  If there is still an Irp, send it off to an Ex Worker thread.
        if (IrpContext != NULL) {

            RC = UDFPostRequest( IrpContext, Irp );
        }

    } _SEH2_EXCEPT(UDFExceptionFilter( IrpContext, _SEH2_GetExceptionInformation())) {
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        RC = UDFExceptionHandler( IrpContext, Irp);
    } _SEH2_END;

    KdPrint(("UDFPerformVerify: RC = %x\n", RC));

    return RC;
    
} // end UDFPerformVerify()

/*

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

*/
BOOLEAN
UDFCheckForDismount(
    IN PtrUDFIrpContext IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN _VcbAcquired
    )
{
    BOOLEAN VcbPresent = TRUE;
    KIRQL SavedIrql;
    BOOLEAN VcbAcquired;
    ULONG ResidualReferenceCount;

    KdPrint(("UDFCheckForDismount:\n"));
    if(!Vcb) return FALSE;

    //  GlobalDataResource is already acquired
    if(!_VcbAcquired) {
        VcbAcquired = UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE/*FALSE*/ );
        if(!VcbAcquired)
            return TRUE;
    } else {
        VcbAcquired = TRUE;
    }

    if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
        (IrpContext->TargetDeviceObject == Vcb->TargetDeviceObject)) {

        ResidualReferenceCount = 2;

    } else {

        ResidualReferenceCount = 1;
    }

    //  If the dismount is not already underway then check if the
    //  user reference count has gone to zero.  If so start the teardown
    //  on the Vcb.
    if (!(Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED)) {
        if (Vcb->VCBOpenCount <= UDF_RESIDUAL_REFERENCE) {
            VcbPresent = UDFDismountVcb(Vcb, VcbAcquired);
        }
        VcbAcquired = VcbAcquired && VcbPresent;

    //  If the teardown is underway and there are absolutely no references
    //  remaining then delete the Vcb.  References here include the
    //  references in the Vcb and Vpb.
    } else if (!(Vcb->VCBOpenCount)) {

        IoAcquireVpbSpinLock( &SavedIrql );
        //  If there are no file objects and no reference counts in the
        //  Vpb we can delete the Vcb.  Don't forget that we have the
        //  last reference in the Vpb.
        if (Vcb->Vpb->ReferenceCount <= ResidualReferenceCount) {

            IoReleaseVpbSpinLock( SavedIrql );
            if(VcbAcquired)
                UDFReleaseResource(&(Vcb->VCBResource));
            UDFStopEjectWaiter(Vcb);
            UDFReleaseVCB(Vcb);
            VcbAcquired =
            VcbPresent = FALSE;

        } else {

            IoReleaseVpbSpinLock( SavedIrql );
        }
    }

    //  Release any resources still acquired.
    if (!_VcbAcquired && VcbAcquired) {
         UDFReleaseResource(&(Vcb->VCBResource));
    }

    return VcbPresent;
} // end UDFCheckForDismount()


/*

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

*/
BOOLEAN
UDFDismountVcb(
    IN PVCB Vcb,
    IN BOOLEAN VcbAcquired
    )
{

    PVPB OldVpb;
    PVPB NewVpb;
    BOOLEAN VcbPresent = TRUE;
    KIRQL SavedIrql;

    BOOLEAN FinalReference;

    KdPrint(("UDFDismountVcb:\n"));
    //  We should only take this path once.
    ASSERT( !(Vcb->VCBFlags & UDF_VCB_FLAGS_BEING_DISMOUNTED) );

    //  Mark the Vcb as DismountInProgress.
    Vcb->VCBFlags |= UDF_VCB_FLAGS_BEING_DISMOUNTED;

    //  Allocate a new Vpb in case we will need it.
    NewVpb = (PVPB)DbgAllocatePoolWithTag( NonPagedPool, sizeof( VPB ), 'bpvU' );
    if(!NewVpb) {
        Vcb->VCBFlags &= ~UDF_VCB_FLAGS_BEING_DISMOUNTED;
        return TRUE;
    }

    RtlZeroMemory( NewVpb, sizeof(VPB) );

    OldVpb = Vcb->Vpb;

    //  Remove the mount volume reference.
    UDFCloseResidual(Vcb);
    // the only residual reference is cleaned above

    //  Acquire the Vpb spinlock to check for Vpb references.
    IoAcquireVpbSpinLock(&SavedIrql);

    //  Remember if this is the last reference on this Vcb.  We incremented
    //  the count on the Vpb earlier so we get one last crack it.  If our
    //  reference has gone to zero but the vpb reference count is greater
    //  than zero then the Io system will be responsible for deleting the
    //  Vpb.
    FinalReference = (BOOLEAN)(OldVpb->ReferenceCount == 1);

    //  There is a reference count in the Vpb and in the Vcb.  We have
    //  incremented the reference count in the Vpb to make sure that
    //  we have last crack at it.  If this is a failed mount then we
    //  want to return the Vpb to the IO system to use for the next
    //  mount request.
    if (OldVpb->RealDevice->Vpb == OldVpb) {

        //  If not the final reference then swap out the Vpb.
        if (!FinalReference) {

            NewVpb->Type = IO_TYPE_VPB;
            NewVpb->Size = sizeof( VPB );
            NewVpb->RealDevice = OldVpb->RealDevice;

            NewVpb->RealDevice->Vpb = NewVpb;

            NewVpb = NULL;
            IoReleaseVpbSpinLock(SavedIrql);
        //  We want to leave the Vpb for the IO system.  Mark it
        //  as being not mounted.  Go ahead and delete the Vcb as
        //  well.
        } else {

            //  Make sure to remove the last reference on the Vpb.

            OldVpb->ReferenceCount--;

            OldVpb->DeviceObject = NULL;
            Vcb->Vpb->Flags &= ~VPB_MOUNTED;

            //  Clear the Vpb flag so we know not to delete it.
            Vcb->Vpb = NULL;

            IoReleaseVpbSpinLock(SavedIrql);
            if(VcbAcquired)
                UDFReleaseResource(&(Vcb->VCBResource));
            UDFStopEjectWaiter(Vcb);
            UDFReleaseVCB(Vcb);
            VcbPresent = FALSE;
        }

    //  Someone has already swapped in a new Vpb.  If this is the final reference
    //  then the file system is responsible for deleting the Vpb.
    } else if (FinalReference) {

        //  Make sure to remove the last reference on the Vpb.
        OldVpb->ReferenceCount--;

        IoReleaseVpbSpinLock( SavedIrql );
        if(VcbAcquired)
            UDFReleaseResource(&(Vcb->VCBResource));
        UDFStopEjectWaiter(Vcb);
        UDFReleaseVCB(Vcb);
        VcbPresent = FALSE;

    //  The current Vpb is no longer the Vpb for the device (the IO system
    //  has already allocated a new one).  We leave our reference in the
    //  Vpb and will be responsible for deleting it at a later time.
    } else {

        OldVpb->DeviceObject = NULL;
        Vcb->Vpb->Flags &= ~VPB_MOUNTED;

        IoReleaseVpbSpinLock( SavedIrql );
    }

    //  Deallocate the new Vpb if we don't need it.
    if (NewVpb != NULL) {
        DbgFreePool( NewVpb );
    }

    //  Let our caller know whether the Vcb is still present.
    return VcbPresent;
} // end UDFDismountVcb()


NTSTATUS
UDFCompareVcb(
    IN PVCB OldVcb,
    IN PVCB NewVcb,
    IN BOOLEAN PhysicalOnly
    )
{
    NTSTATUS RC;
    UDF_FILE_INFO    RootFileInfo;
    BOOLEAN SimpleLogicalCheck = FALSE;

    KdPrint(("UDFCompareVcb:\n"));
    if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_BEING_UNLOADED) {
        KdPrint(("  WRONG_VOLUME\n"));
        return STATUS_WRONG_VOLUME;
    }

#define VCB_NE(x)   (OldVcb->x != NewVcb->x)

    // compare physical parameters
    if(PhysicalOnly) {
        KdPrint(("  PhysicalOnly\n"));
        if(VCB_NE(FirstLBA) ||
           VCB_NE(LastLBA) ||
           VCB_NE(FirstTrackNum) ||
           VCB_NE(LastTrackNum) ||
           VCB_NE(NWA) ||
           VCB_NE(LastPossibleLBA) ||
           VCB_NE(PhSerialNumber) ||
           VCB_NE(PhErasable) ||
           VCB_NE(PhDiskType) ||
           VCB_NE(MediaClassEx) ||

          /* We cannot compare these flags, because NewVcb is in unconditional ReadOnly */

          /*((OldVcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) != (NewVcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)) ||
          ((OldVcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY)  != (NewVcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY)) ||*/

           VCB_NE(TargetDeviceObject) ||
    //       VCB_NE(xxx) ||
    //       VCB_NE(xxx) ||
           VCB_NE(LastSession) ) {

            KdPrint(("  WRONG_VOLUME (2)\n"));
            return STATUS_WRONG_VOLUME;
        }
        // Note, MRWStatus can change while media is mounted (stoppped/in-progress/complete)
        // We can compare only (Vcb->MRWStatus == 0) values
        if((OldVcb->MRWStatus == 0) != (NewVcb->MRWStatus == 0)) {
            KdPrint(("  WRONG_VOLUME (4), missmatch MRW status\n"));
        }
        for(uint32 i=OldVcb->FirstTrackNum; i<=OldVcb->LastTrackNum; i++) {
            if(VCB_NE(TrackMap[i].FirstLba) ||
               VCB_NE(TrackMap[i].LastLba) ||
               VCB_NE(TrackMap[i].PacketSize) ||
               VCB_NE(TrackMap[i].TrackParam) ||
               VCB_NE(TrackMap[i].DataParam) ||
               VCB_NE(TrackMap[i].NWA_V) ) {
                KdPrint(("  WRONG_VOLUME (3), missmatch trk %d\n", i));
                return STATUS_WRONG_VOLUME;
            }
        }
        KdPrint(("  Vcb compare Ok\n"));
        return STATUS_SUCCESS;
    }

    // Something is nasty!!! We perform verify for not flushed volume
    // This should never happen, but some devices/buses and their drivers
    // can lead us to such condition. For example with help of RESET.
    // Now, we hope, that nobody changed media.
    // We shall make simplified logical structure check
    if(OldVcb->Modified) {
        KdPrint(("  Vcb SIMPLE compare on !!!MODIFIED!!! volume\n"));
        ASSERT(FALSE);
        SimpleLogicalCheck = TRUE;
    }

    // compare logical structure
    if(!SimpleLogicalCheck && (OldVcb->InitVatCount != NewVcb->InitVatCount)) {
        KdPrint(("  InitVatCount %d != %d \n", OldVcb->InitVatCount, NewVcb->InitVatCount));
        return STATUS_WRONG_VOLUME;
    }

    // Compare volume creation time
    if(OldVcb->VolCreationTime != NewVcb->VolCreationTime) {
        KdPrint(("  VolCreationTime %I64x != %I64x \n", OldVcb->VolCreationTime, NewVcb->VolCreationTime));
        return STATUS_WRONG_VOLUME;
    }
    // Compare serial numbers 
    if(OldVcb->SerialNumber != NewVcb->SerialNumber) {
        KdPrint(("  SerialNumber %x != %x \n", OldVcb->SerialNumber, NewVcb->SerialNumber));
        return STATUS_WRONG_VOLUME;
    }   
    // Compare volume idents
    if(!SimpleLogicalCheck &&
       RtlCompareUnicodeString(&(OldVcb->VolIdent),&(NewVcb->VolIdent),FALSE)) {
        KdPrint(("  VolIdent missmatch \n"));
        return STATUS_WRONG_VOLUME;
    }
    if(SimpleLogicalCheck) {
        // do not touch RootDir. It can be partially recorded
        KdPrint(("  SimpleLogicalCheck Ok\n"));
        return STATUS_SUCCESS;
    }

    RC = UDFOpenRootFile__(NewVcb, &(NewVcb->RootLbAddr), &RootFileInfo);
    if(!NT_SUCCESS(RC)) {
        KdPrint(("  Can't open root file, status %x\n", RC));
        UDFCleanUpFile__(NewVcb, &RootFileInfo);
        return STATUS_WRONG_VOLUME;
    }
    // perform exhaustive check
    if(!(OldVcb->RootDirFCB)) {
        KdPrint(("  !(OldVcb->RootDirFCB)\n"));
wr_vol:
        UDFCloseFile__(NewVcb, &RootFileInfo);
        UDFCleanUpFile__(NewVcb, &RootFileInfo);
        return STATUS_WRONG_VOLUME;
    }

    if(!UDFCompareFileInfo(&RootFileInfo, OldVcb->RootDirFCB->FileInfo)) {
        KdPrint(("  !UDFCompareFileInfo\n"));
        goto wr_vol;
    }
    UDFCloseFile__(NewVcb, &RootFileInfo);
    UDFCleanUpFile__(NewVcb, &RootFileInfo);

    KdPrint(("UDFCompareVcb: Ok\n"));
    return STATUS_SUCCESS;

#undef VCB_NE

} // end UDFCompareVcb()

