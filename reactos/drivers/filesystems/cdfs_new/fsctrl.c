/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Cdfs called
    by the Fsd/Fsp dispatch drivers.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FSCTRL)

//
//  Local constants
//

BOOLEAN CdDisable = FALSE;
BOOLEAN CdNoJoliet = FALSE;

//
//  Local support routines
//

NTSTATUS
CdUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdReMountOldVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB OldVcb,
    IN PVCB NewVcb,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo
    );

NTSTATUS
CdMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

CdIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdInvalidateVolumes (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdScanForDismountedVcb (
    IN PIRP_CONTEXT IrpContext
    );

BOOLEAN
CdFindPrimaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCHAR RawIsoVd,
    IN ULONG BlockFactor,
    IN BOOLEAN ReturnOnError,
    IN BOOLEAN VerifyVolume
    );

BOOLEAN
CdIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PVCB *OldVcb
    );

VOID
CdFindActiveVolDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PCHAR RawIsoVd,
    IN BOOLEAN VerifyVolume
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonFsControl)
#pragma alloc_text(PAGE, CdDismountVolume)
#pragma alloc_text(PAGE, CdFindActiveVolDescriptor)
#pragma alloc_text(PAGE, CdFindPrimaryVd)
#pragma alloc_text(PAGE, CdIsPathnameValid)
#pragma alloc_text(PAGE, CdIsRemount)
#pragma alloc_text(PAGE, CdIsVolumeDirty)
#pragma alloc_text(PAGE, CdIsVolumeMounted)
#pragma alloc_text(PAGE, CdLockVolume)
#pragma alloc_text(PAGE, CdMountVolume)
#pragma alloc_text(PAGE, CdOplockRequest)
#pragma alloc_text(PAGE, CdScanForDismountedVcb)
#pragma alloc_text(PAGE, CdUnlockVolume)
#pragma alloc_text(PAGE, CdUserFsctl)
#pragma alloc_text(PAGE, CdVerifyVolume)
#endif


//
//  Local support routine
//

NTSTATUS
CdLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine performs the actual lock volume operation.  It will be called
    by anyone wishing to try to protect the volume for a long duration.  PNP
    operations are such a user.
    
    The volume must be held exclusive by the caller.

Arguments:

    Vcb - The volume being locked.
    
    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KIRQL SavedIrql;
    NTSTATUS FinalStatus = (FileObject? STATUS_ACCESS_DENIED: STATUS_DEVICE_BUSY);
    ULONG RemainingUserReferences = (FileObject? 1: 0);

    //
    //  The cleanup count for the volume only reflects the fileobject that
    //  will lock the volume.  Otherwise, we must fail the request.
    //
    //  Since the only cleanup is for the provided fileobject, we will try
    //  to get rid of all of the other user references.  If there is only one
    //  remaining after the purge then we can allow the volume to be locked.
    //
    
    CdPurgeVolume( IrpContext, Vcb, FALSE );

    //
    //  Now back out of our synchronization and wait for the lazy writer
    //  to finish off any lazy closes that could have been outstanding.
    //
    //  Since we purged, we know that the lazy writer will issue all
    //  possible lazy closes in the next tick - if we hadn't, an otherwise
    //  unopened file with a large amount of dirty data could have hung
    //  around for a while as the data trickled out to the disk.
    //
    //  This is even more important now since we send notification to
    //  alert other folks that this style of check is about to happen so
    //  that they can close their handles.  We don't want to enter a fast
    //  race with the lazy writer tearing down his references to the file.
    //

    CdReleaseVcb( IrpContext, Vcb );

    Status = CcWaitForCurrentLazyWriterActivity();

    //
    //  This is intentional. If we were able to get the Vcb before, just
    //  wait for it and take advantage of knowing that it is OK to leave
    //  the flag up.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );
    
    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    CdFspClose( Vcb );

    //
    //  If the volume is already explicitly locked then fail.  We use the
    //  Vpb locked flag as an 'explicit lock' flag in the same way as Fat.
    //

    IoAcquireVpbSpinLock( &SavedIrql ); 
        
    if (!FlagOn( Vcb->Vpb->Flags, VPB_LOCKED ) && 
        (Vcb->VcbCleanup == RemainingUserReferences) &&
        (Vcb->VcbUserReference == CDFS_RESIDUAL_USER_REFERENCE + RemainingUserReferences))  {

        SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
        SetFlag( Vcb->Vpb->Flags, VPB_LOCKED);
        Vcb->VolumeLockFileObject = FileObject;
        FinalStatus = STATUS_SUCCESS;
    }
    
    IoReleaseVpbSpinLock( SavedIrql );  
    
    return FinalStatus;
}


NTSTATUS
CdUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine performs the actual unlock volume operation. 
    
    The volume must be held exclusive by the caller.

Arguments:

    Vcb - The volume being locked.
    
    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:

    NTSTATUS - The return status for the operation
    
    Attempting to remove a system lock that did not exist is OK.

--*/

{
    NTSTATUS Status = STATUS_NOT_LOCKED;
    KIRQL SavedIrql;

    //
    //  Note that we check the VPB_LOCKED flag here rather than the Vcb
    //  lock flag.  The Vpb flag is only set for an explicit lock request,  not
    //  for the implicit lock obtained on a volume open with zero share mode.
    //
    
    IoAcquireVpbSpinLock( &SavedIrql ); 
 
    if (FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) && 
        (FileObject == Vcb->VolumeLockFileObject))  {

        ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
        ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED);
        Vcb->VolumeLockFileObject = NULL;
        Status = STATUS_SUCCESS;
    }
    
    IoReleaseVpbSpinLock( SavedIrql );  

    return Status;
}


NTSTATUS
CdCommonFsControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = CdUserFsctl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = CdMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = CdVerifyVolume( IrpContext, Irp );
        break;

    default:

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  Case on the control code.
    //

    switch ( IrpSp->Parameters.FileSystemControl.FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY :
    case FSCTL_OPLOCK_BREAK_ACK_NO_2 :
    case FSCTL_REQUEST_FILTER_OPLOCK :

        Status = CdOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME :

        Status = CdLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME :

        Status = CdUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME :

        Status = CdDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_DIRTY :

        Status = CdIsVolumeDirty( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED :

        Status = CdIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID :

        Status = CdIsPathnameValid( IrpContext, Irp );
        break;

    case FSCTL_INVALIDATE_VOLUMES :

        Status = CdInvalidateVolumes( IrpContext, Irp );
        break;


    //
    //  We don't support any of the known or unknown requests.
    //

    default:

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


VOID
CdReMountOldVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB OldVcb,
    IN PVCB NewVcb,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo
    )
{
    KIRQL SavedIrql;
    
    ObDereferenceObject( OldVcb->TargetDeviceObject );

    IoAcquireVpbSpinLock( &SavedIrql );

    NewVcb->Vpb->RealDevice->Vpb = OldVcb->Vpb;
    
    OldVcb->Vpb->RealDevice = NewVcb->Vpb->RealDevice;
    OldVcb->TargetDeviceObject = DeviceObjectWeTalkTo;
    
    CdUpdateVcbCondition( OldVcb, VcbMounted);
    CdUpdateMediaChangeCount( OldVcb, NewVcb->MediaChangeCount);

    ClearFlag( OldVcb->VcbState, VCB_STATE_VPB_NOT_ON_DEVICE);

    IoReleaseVpbSpinLock( SavedIrql );
}


//
//  Local support routine
//

NTSTATUS
CdMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is a Cdrom volume,
    and create the VCB and root DCB structures.  The algorithm it
    uses is essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do I/O
       through the on-disk volume descriptors.

    2. Read the disk and check if it is a Cdrom volume.

    3. If it is not a Cdrom volume then delete the Vcb and
       complete the IRP back with an appropriate status.

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves deleting the VCB, hook in the
       old VCB, and complete the IRP.

    5. Otherwise create a Vcb and root DCB for each valid volume descriptor.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PVOLUME_DEVICE_OBJECT VolDo = NULL;
    PVCB Vcb = NULL;
    PVCB OldVcb;
    
    BOOLEAN FoundPvd = FALSE;
    BOOLEAN SetDoVerifyOnFail;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    PVPB Vpb = IrpSp->Parameters.MountVolume.Vpb;

    PFILE_OBJECT FileObjectToNotify = NULL;

    ULONG BlockFactor;
    DISK_GEOMETRY DiskGeometry;

    IO_SCSI_CAPABILITIES Capabilities;

    IO_STATUS_BLOCK Iosb;

    PCHAR RawIsoVd = NULL;

    PCDROM_TOC CdromToc = NULL;
    ULONG TocLength = 0;
    ULONG TocTrackCount = 0;
    ULONG TocDiskFlags = 0;
    ULONG MediaChangeCount = 0;

    PAGED_CODE();

    //
    //  Check that we are talking to a Cdrom device.  This request should
    //  always be waitable.
    //

    ASSERT( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM );
    ASSERT( FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT ));

    //
    //  Update the real device in the IrpContext from the Vpb.  There was no available
    //  file object when the IrpContext was created.
    //

    IrpContext->RealDevice = Vpb->RealDevice;

    SetDoVerifyOnFail = CdRealDevNeedsVerify( IrpContext->RealDevice);

    //
    //  Check if we have disabled the mount process.
    //

    if (CdDisable) {

        CdCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Do a CheckVerify here to lift the MediaChange ticker from the driver
    //

    Status = CdPerformDevIoCtrl( IrpContext,
                                 IOCTL_CDROM_CHECK_VERIFY,
                                 DeviceObjectWeTalkTo,
                                 &MediaChangeCount,
                                 sizeof(ULONG),
                                 FALSE,
                                 TRUE,
                                 &Iosb );

    if (!NT_SUCCESS( Status )) {
        
        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }
    
    if (Iosb.Information != sizeof(ULONG)) {

        //
        //  Be safe about the count in case the driver didn't fill it in
        //

        MediaChangeCount = 0;
    }

    //
    //  Now let's make Jeff delirious and call to get the disk geometry.  This
    //  will fix the case where the first change line is swallowed.
    //

    Status = CdPerformDevIoCtrl( IrpContext,
                                 IOCTL_CDROM_GET_DRIVE_GEOMETRY,
                                 DeviceObjectWeTalkTo,
                                 &DiskGeometry,
                                 sizeof( DISK_GEOMETRY ),
                                 FALSE,
                                 TRUE,
                                 NULL );

    //
    //  Return insufficient sources to our caller.
    //

    if (Status == STATUS_INSUFFICIENT_RESOURCES) {

        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Now check the block factor for addressing the volume descriptors.
    //  If the call for the disk geometry failed then assume there is one
    //  block per sector.
    //

    BlockFactor = 1;

    if (NT_SUCCESS( Status ) &&
        (DiskGeometry.BytesPerSector != 0) &&
        (DiskGeometry.BytesPerSector < SECTOR_SIZE)) {

        BlockFactor = SECTOR_SIZE / DiskGeometry.BytesPerSector;
    }

    //
    //  Acquire the global resource to do mount operations.
    //

    CdAcquireCdData( IrpContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate a buffer to query the TOC.
        //

        CdromToc = FsRtlAllocatePoolWithTag( CdPagedPool,
                                             sizeof( CDROM_TOC ),
                                             TAG_CDROM_TOC );

        RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

        //
        //  Do a quick check to see if there any Vcb's which can be removed.
        //

        CdScanForDismountedVcb( IrpContext );

        //
        //  Get our device object and alignment requirement.
        //

        Status = IoCreateDevice( CdData.DriverObject,
                                 sizeof( VOLUME_DEVICE_OBJECT ) - sizeof( DEVICE_OBJECT ),
                                 NULL,
                                 FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                                 0,
                                 FALSE,
                                 (PDEVICE_OBJECT *) &VolDo );

        if (!NT_SUCCESS( Status )) { try_leave( Status ); }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        //
        //  We must initialize the stack size in our device object before
        //  the following reads, because the I/O system has not done it yet.
        //

        ((PDEVICE_OBJECT) VolDo)->StackSize = (CCHAR) (DeviceObjectWeTalkTo->StackSize + 1);

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

        //
        //  Let's query for the Toc now and handle any error we get from this operation.
        //

        Status = CdProcessToc( IrpContext,
                               DeviceObjectWeTalkTo,
                               CdromToc,
                               &TocLength,
                               &TocTrackCount,
                               &TocDiskFlags );

        //
        //  If we failed to read the TOC, then bail out.  Probably blank media.
        //

        if (Status != STATUS_SUCCESS)  { 

            try_leave( Status ); 
        }

        //
        //  Now before we can initialize the Vcb we need to set up the
        //  device object field in the VPB to point to our new volume device
        //  object.
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT) VolDo;

        //
        //  Initialize the Vcb.  This routine will raise on an allocation
        //  failure.
        //

        CdInitializeVcb( IrpContext,
                         &VolDo->Vcb,
                         DeviceObjectWeTalkTo,
                         Vpb,
                         CdromToc,
                         TocLength,
                         TocTrackCount,
                         TocDiskFlags,
                         BlockFactor,
                         MediaChangeCount );

        //
        //  Show that we initialized the Vcb and can cleanup with the Vcb.
        //

        Vcb = &VolDo->Vcb;
        VolDo = NULL;
        Vpb = NULL;
        CdromToc = NULL;

        //
        //  Store the Vcb in the IrpContext as we didn't have one before.
        //

        IrpContext->Vcb = Vcb;

        CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

        //
        //  Let's reference the Vpb to make sure we are the one to
        //  have the last dereference.
        //

        Vcb->Vpb->ReferenceCount += 1;

        //
        //  Clear the verify bit for the start of mount.
        //

        CdMarkRealDevVerifyOk( Vcb->Vpb->RealDevice);

        if (!FlagOn( Vcb->VcbState, VCB_STATE_AUDIO_DISK))  {
            
            //
            //  Allocate a buffer to read in the volume descriptors.  We allocate a full
            //  page to make sure we don't hit any alignment problems.
            //

            RawIsoVd = FsRtlAllocatePoolWithTag( CdNonPagedPool,
                                                 ROUND_TO_PAGES( SECTOR_SIZE ),
                                                 TAG_VOL_DESC );

            //
            //  Try to find the primary volume descriptor.
            //

            FoundPvd = CdFindPrimaryVd(   IrpContext,
                                          Vcb,
                                          RawIsoVd,
                                          BlockFactor,
                                          TRUE,
                                          FALSE );

            if (!FoundPvd)  {

                //
                //  We failed to find a valid VD in the data track,  but there were also
                //  audio tracks on this disc,  so we'll try to mount it as an audio CD.
                //  Since we're always last in the mount order,  we won't be preventing
                //  any other FS from trying to mount the data track.  However if the 
                //  data track was at the start of the disc,  then we abort,  to avoid
                //  having to filter it from our synthesised directory listing later.  We
                //  already filtered off any data track at the end.
                //

                if (!(TocDiskFlags & CDROM_DISK_AUDIO_TRACK) ||
                     BooleanFlagOn( Vcb->CdromToc->TrackData[0].Control, TOC_DATA_TRACK))  {
                
                    try_leave( Status = STATUS_UNRECOGNIZED_VOLUME);
                }

                SetFlag( Vcb->VcbState, VCB_STATE_AUDIO_DISK | VCB_STATE_CDXA );

                CdFreePool( &RawIsoVd );
                RawIsoVd = NULL;
            }
        }
        
        //
        //  Look and see if there is a secondary volume descriptor we want to
        //  use.
        //

        if (FoundPvd) {

            //
            //  Store the primary volume descriptor in the second half of
            //  RawIsoVd.  Then if our search for a secondary fails we can
            //  recover this immediately.
            //

            RtlCopyMemory( Add2Ptr( RawIsoVd, SECTOR_SIZE, PVOID ),
                           RawIsoVd,
                           SECTOR_SIZE );

            //
            //  We have the initial volume descriptor.  Locate a secondary
            //  volume descriptor if present.
            //

            CdFindActiveVolDescriptor( IrpContext,
                                       Vcb,
                                       RawIsoVd,
                                       FALSE);
        }

        //
        //  Check if this is a remount operation.  If so then clean up
        //  the data structures passed in and created here.
        //

        if (CdIsRemount( IrpContext, Vcb, &OldVcb )) {

            KIRQL SavedIrql;

            ASSERT( NULL != OldVcb->SwapVpb );

            //
            //  Link the old Vcb to point to the new device object that we
            //  should be talking to, dereferencing the previous.  Call a 
            //  nonpaged routine to do this since we take the Vpb spinlock.
            //

            CdReMountOldVcb( IrpContext, 
                             OldVcb, 
                             Vcb, 
                             DeviceObjectWeTalkTo);

            //
            //  See if we will need to provide notification of the remount.  This is the readonly
            //  filesystem's form of dismount/mount notification - we promise that whenever a
            //  volume is "dismounted", that a mount notification will occur when it is revalidated.
            //  Note that we do not send mount on normal remounts - that would duplicate the media
            //  arrival notification of the device driver.
            //
    
            if (FlagOn( OldVcb->VcbState, VCB_STATE_NOTIFY_REMOUNT )) {
    
                ClearFlag( OldVcb->VcbState, VCB_STATE_NOTIFY_REMOUNT );
                
                FileObjectToNotify = OldVcb->RootIndexFcb->FileObject;
                ObReferenceObject( FileObjectToNotify );
            }
            
            try_leave( Status = STATUS_SUCCESS );
        }

        //
        //  This is a new mount.  Go ahead and initialize the
        //  Vcb from the volume descriptor.
        //

        CdUpdateVcbFromVolDescriptor( IrpContext,
                                      Vcb,
                                      RawIsoVd );

        //
        //  Drop an extra reference on the root dir file so we'll be able to send
        //  notification.
        //

        if (Vcb->RootIndexFcb) {

            FileObjectToNotify = Vcb->RootIndexFcb->FileObject;
            ObReferenceObject( FileObjectToNotify );
        }

        //
        //  Now check the maximum transfer limits on the device in case we
        //  get raw reads on this volume.
        //

        Status = CdPerformDevIoCtrl( IrpContext,
                                     IOCTL_SCSI_GET_CAPABILITIES,
                                     DeviceObjectWeTalkTo,
                                     &Capabilities,
                                     sizeof( IO_SCSI_CAPABILITIES ),
                                     FALSE,
                                     TRUE,
                                     NULL );

        if (NT_SUCCESS(Status)) {

            Vcb->MaximumTransferRawSectors = Capabilities.MaximumTransferLength / RAW_SECTOR_SIZE;
            Vcb->MaximumPhysicalPages = Capabilities.MaximumPhysicalPages;

        } else {

            //
            //  This should never happen, but we can safely assume 64k and 16 pages.
            //

            Vcb->MaximumTransferRawSectors = (64 * 1024) / RAW_SECTOR_SIZE;
            Vcb->MaximumPhysicalPages = 16;
        }

        //
        //  The new mount is complete.  Remove the additional references on this
        //  Vcb and the device we are mounted on top of.
        //

        Vcb->VcbReference -= CDFS_RESIDUAL_REFERENCE;
        ASSERT( Vcb->VcbReference == CDFS_RESIDUAL_REFERENCE );

        ObDereferenceObject( Vcb->TargetDeviceObject );

        CdUpdateVcbCondition( Vcb, VcbMounted);

        CdReleaseVcb( IrpContext, Vcb );
        Vcb = NULL;

        Status = STATUS_SUCCESS;

    } finally {

        //
        //  Free the TOC buffer if not in the Vcb.
        //

        if (CdromToc != NULL) {

            CdFreePool( &CdromToc );
        }

        //
        //  Free the sector buffer if allocated.
        //

        if (RawIsoVd != NULL) {

            CdFreePool( &RawIsoVd );
        }

        //
        //  If we are not mounting the device,  then set the verify bit again.
        //
        
        if ((AbnormalTermination() || (Status != STATUS_SUCCESS)) && 
            SetDoVerifyOnFail)  {

            CdMarkRealDevForVerify( IrpContext->RealDevice);
        }

        //
        //  If we didn't complete the mount then cleanup any remaining structures.
        //

        if (Vpb != NULL) { Vpb->DeviceObject = NULL; }

        if (Vcb != NULL) {

            //
            //  Make sure there is no Vcb in the IrpContext since it could go away
            //

            IrpContext->Vcb = NULL;

            Vcb->VcbReference -= CDFS_RESIDUAL_REFERENCE;

            if (CdDismountVcb( IrpContext, Vcb )) {

                CdReleaseVcb( IrpContext, Vcb );
            }

        } else if (VolDo != NULL) {

            IoDeleteDevice( (PDEVICE_OBJECT) VolDo );
        }

        //
        //  Release the global resource.
        //

        CdReleaseCdData( IrpContext );
    }

    //
    //  Now send mount notification.
    //
    
    if (FileObjectToNotify) {

        FsRtlNotifyVolumeEvent( FileObjectToNotify, FSRTL_VOLUME_MOUNT );
        ObDereferenceObject( FileObjectToNotify );
    }

    //
    //  Complete the request if no exception.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the verify volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PVPB Vpb = IrpSp->Parameters.VerifyVolume.Vpb;
    PVCB Vcb = &((PVOLUME_DEVICE_OBJECT) IrpSp->Parameters.VerifyVolume.DeviceObject)->Vcb;

    PCHAR RawIsoVd = NULL;

    PCDROM_TOC CdromToc = NULL;
    ULONG TocLength = 0;
    ULONG TocTrackCount = 0;
    ULONG TocDiskFlags = 0;

    ULONG MediaChangeCount = Vcb->MediaChangeCount;

    PFILE_OBJECT FileObjectToNotify = NULL;

    BOOLEAN ReturnError;
    BOOLEAN ReleaseVcb = FALSE;

    IO_STATUS_BLOCK Iosb;

    STRING AnsiLabel;
    UNICODE_STRING UnicodeLabel;

    WCHAR VolumeLabel[ VOLUME_ID_LENGTH ];
    ULONG VolumeLabelLength;

    ULONG Index;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  We check that we are talking to a Cdrom device.
    //

    ASSERT( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM );
    ASSERT( FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT ));

    //
    //  Update the real device in the IrpContext from the Vpb.  There was no available
    //  file object when the IrpContext was created.
    //

    IrpContext->RealDevice = Vpb->RealDevice;

    //
    //  Acquire the global resource to synchronise against mounts and teardown,
    //  finally clause releases.
    //

    CdAcquireCdData( IrpContext );

    try {

        CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );
        ReleaseVcb = TRUE;

        //
        //  Verify that there is a disk here.
        //

        Status = CdPerformDevIoCtrl( IrpContext,
                                     IOCTL_CDROM_CHECK_VERIFY,
                                     Vcb->TargetDeviceObject,
                                     &MediaChangeCount,
                                     sizeof(ULONG),
                                     FALSE,
                                     TRUE,
                                     &Iosb );

        if (!NT_SUCCESS( Status )) {

            //
            //  If we will allow a raw mount then return WRONG_VOLUME to
            //  allow the volume to be mounted by raw.
            //

            if (FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {

                Status = STATUS_WRONG_VOLUME;
            }

            try_return( Status );
        }
        
        if (Iosb.Information != sizeof(ULONG)) {

            //
            //  Be safe about the count in case the driver didn't fill it in
            //

            MediaChangeCount = 0;
        }

        //
        //  Verify that the device actually saw a change. If the driver does not
        //  support the MCC, then we must verify the volume in any case.
        //

        if (MediaChangeCount == 0 ||
            (Vcb->MediaChangeCount != MediaChangeCount)) {

            //
            //  Allocate a buffer to query the TOC.
            //

            CdromToc = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                 sizeof( CDROM_TOC ),
                                                 TAG_CDROM_TOC );

            RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

            //
            //  Let's query for the Toc now and handle any error we get from this operation.
            //

            Status = CdProcessToc( IrpContext,
                                   Vcb->TargetDeviceObject,
                                   CdromToc,
                                   &TocLength,
                                   &TocTrackCount,
                                   &TocDiskFlags );

            //
            //  If we failed to read the TOC,  then give up now.  Drives will fail
            //  a TOC read on,  for example,  erased CD-RW media.
            //

            if (Status != STATUS_SUCCESS) {

                //
                //  For any errors other than no media and not ready,  commute the
                //  status to ensure that the current VPB is kicked off the device
                //  below - there is probably blank media in the drive,  since we got
                //  further than the check verify.
                //

                if (!CdIsRawDevice( IrpContext, Status )) {

                    Status = STATUS_WRONG_VOLUME;
                }

                try_return( Status );

            //
            //  We got a TOC.  Verify that it matches the previous Toc.
            //

            } else if ((Vcb->TocLength != TocLength) ||
                       (Vcb->TrackCount != TocTrackCount) ||
                       (Vcb->DiskFlags != TocDiskFlags) ||
                       !RtlEqualMemory( CdromToc,
                                        Vcb->CdromToc,
                                        TocLength )) {

                try_return( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  If the disk to verify is an audio disk then we already have a
            //  match.  Otherwise we need to check the volume descriptor.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_AUDIO_DISK )) {

                //
                //  Allocate a buffer for the sector buffer.
                //

                RawIsoVd = FsRtlAllocatePoolWithTag( CdNonPagedPool,
                                                     ROUND_TO_PAGES( 2 * SECTOR_SIZE ),
                                                     TAG_VOL_DESC );

                //
                //  Read the primary volume descriptor for this volume.  If we
                //  get an io error and this verify was a the result of DASD open,
                //  commute the Io error to STATUS_WRONG_VOLUME.  Note that if we currently
                //  expect a music disk then this request should fail.
                //

                ReturnError = FALSE;

                if (FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {

                    ReturnError = TRUE;
                }

                if (!CdFindPrimaryVd( IrpContext,
                                      Vcb,
                                      RawIsoVd,
                                      Vcb->BlockFactor,
                                      ReturnError,
                                      TRUE )) {

                    //
                    //  If the previous Vcb did not represent a raw disk
                    //  then show this volume was dismounted.
                    //

                    try_return( Status = STATUS_WRONG_VOLUME );

                } 
                else {

                    //
                    //  Look for a supplementary VD.
                    //
                    //  Store the primary volume descriptor in the second half of
                    //  RawIsoVd.  Then if our search for a secondary fails we can
                    //  recover this immediately.
                    //

                    RtlCopyMemory( Add2Ptr( RawIsoVd, SECTOR_SIZE, PVOID ),
                                   RawIsoVd,
                                   SECTOR_SIZE );

                    //
                    //  We have the initial volume descriptor.  Locate a secondary
                    //  volume descriptor if present.
                    //

                    CdFindActiveVolDescriptor( IrpContext,
                                               Vcb,
                                               RawIsoVd,
                                               TRUE);
                    //
                    //  Compare the serial numbers.  If they don't match, set the
                    //  status to wrong volume.
                    //

                    if (Vpb->SerialNumber != CdSerial32( RawIsoVd, SECTOR_SIZE )) {

                        try_return( Status = STATUS_WRONG_VOLUME );
                    }

                    //
                    //  Verify the volume labels.
                    //

                    if (!FlagOn( Vcb->VcbState, VCB_STATE_JOLIET )) {

                        //
                        //  Compute the length of the volume name
                        //

                        AnsiLabel.Buffer = CdRvdVolId( RawIsoVd, Vcb->VcbState );
                        AnsiLabel.MaximumLength = AnsiLabel.Length = VOLUME_ID_LENGTH;

                        UnicodeLabel.MaximumLength = VOLUME_ID_LENGTH * sizeof( WCHAR );
                        UnicodeLabel.Buffer = VolumeLabel;

                        //
                        //  Convert this to unicode.  If we get any error then use a name
                        //  length of zero.
                        //

                        VolumeLabelLength = 0;

                        if (NT_SUCCESS( RtlOemStringToCountedUnicodeString( &UnicodeLabel,
                                                                            &AnsiLabel,
                                                                            FALSE ))) {

                            VolumeLabelLength = UnicodeLabel.Length;
                        }

                    //
                    //  We need to convert from big-endian to little endian.
                    //

                    } else {

                        CdConvertBigToLittleEndian( IrpContext,
                                                    CdRvdVolId( RawIsoVd, Vcb->VcbState ),
                                                    VOLUME_ID_LENGTH,
                                                    (PCHAR) VolumeLabel );

                        VolumeLabelLength = VOLUME_ID_LENGTH;
                    }

                    //
                    //  Strip the trailing spaces or zeroes from the name.
                    //

                    Index = VolumeLabelLength / sizeof( WCHAR );

                    while (Index > 0) {

                        if ((VolumeLabel[ Index - 1 ] != L'\0') &&
                            (VolumeLabel[ Index - 1 ] != L' ')) {

                            break;
                        }

                        Index -= 1;
                    }

                    //
                    //  Now set the final length for the name.
                    //

                    VolumeLabelLength = (USHORT) (Index * sizeof( WCHAR ));

                    //
                    //  Now check that the label matches.
                    //
                    if ((Vpb->VolumeLabelLength != VolumeLabelLength) ||
                        !RtlEqualMemory( Vpb->VolumeLabel,
                                         VolumeLabel,
                                         VolumeLabelLength )) {

                        try_return( Status = STATUS_WRONG_VOLUME );
                    }
                }
            }
        }

        //
        //  The volume is OK, clear the verify bit.
        //

        CdUpdateVcbCondition( Vcb, VcbMounted);

        CdMarkRealDevVerifyOk( Vpb->RealDevice);

        //
        //  See if we will need to provide notification of the remount.  This is the readonly
        //  filesystem's form of dismount/mount notification.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_NOTIFY_REMOUNT )) {

            ClearFlag( Vcb->VcbState, VCB_STATE_NOTIFY_REMOUNT );
            
            FileObjectToNotify = Vcb->RootIndexFcb->FileObject;
            ObReferenceObject( FileObjectToNotify );
        }
        
    try_exit: NOTHING;

        //
        //  Update the media change count to note that we have verified the volume
        //  at this value - regardless of the outcome.
        //

        CdUpdateMediaChangeCount( Vcb, MediaChangeCount);

        //
        //  If we got the wrong volume then free any remaining XA sector in
        //  the current Vcb.  Also mark the Vcb as not mounted.
        //

        if (Status == STATUS_WRONG_VOLUME) {

            CdUpdateVcbCondition( Vcb, VcbNotMounted);

            if (Vcb->XASector != NULL) {

                CdFreePool( &Vcb->XASector );
                Vcb->XASector = 0;
                Vcb->XADiskOffset = 0;
            }

            //
            //  Now, if there are no user handles to the volume, try to spark
            //  teardown by purging the volume.
            //

            if (Vcb->VcbCleanup == 0) {

                if (NT_SUCCESS( CdPurgeVolume( IrpContext, Vcb, FALSE ))) {
                    
                    ReleaseVcb = CdCheckForDismount( IrpContext, Vcb, FALSE );
                }
            }
        }

    } finally {

        //
        //  Free the TOC buffer if allocated.
        //

        if (CdromToc != NULL) {

            CdFreePool( &CdromToc );
        }

        if (RawIsoVd != NULL) {

            CdFreePool( &RawIsoVd );
        }

        if (ReleaseVcb) {
            
            CdReleaseVcb( IrpContext, Vcb );
        }

        CdReleaseCdData( IrpContext );
    }

    //
    //  Now send mount notification.
    //
    
    if (FileObjectToNotify) {

        FsRtlNotifyVolumeEvent( FileObjectToNotify, FSRTL_VOLUME_MOUNT );
        ObDereferenceObject( FileObjectToNotify );
    }
    
    //
    //  Complete the request if no exception.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    PCCB Ccb;

    ULONG OplockCount = 0;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  We only permit oplock requests on files.
    //

    if (CdDecodeFileObject( IrpContext,
                            IrpSp->FileObject,
                            &Fcb,
                            &Ccb ) != UserFileOpen ) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Make this a waitable Irpcontext so we don't fail to acquire
    //  the resources.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch (IrpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :
    case FSCTL_REQUEST_FILTER_OPLOCK :

        CdAcquireFcbExclusive( IrpContext, Fcb, FALSE );

        if (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            if (Fcb->FileLock != NULL) {

                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( Fcb->FileLock );
            }

        } else {

            OplockCount = Fcb->FcbCleanup;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        CdAcquireFcbShared( IrpContext, Fcb, FALSE );
        break;

    default:

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Verify the Fcb.
        //

        CdVerifyFcbOperation( IrpContext, Fcb );

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        CdLockFcb( IrpContext, Fcb );
        Fcb->IsFastIoPossible = CdIsFastIoPossible( Fcb );
        CdUnlockFcb( IrpContext, Fcb );

        //
        //  The oplock package will complete the Irp.
        //

        Irp = NULL;

    } finally {

        //
        //  Release all of our resources
        //

        CdReleaseFcb( IrpContext, Fcb );
    }

    //
    //  Complete the request if there was no exception.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Send our notification so that folks that like to hold handles on
    //  volumes can get out of the way.
    //

    FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_LOCK );

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;
    CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    try {

        //
        //  Verify the Vcb.
        //

        CdVerifyVcb( IrpContext, Vcb );

        Status = CdLockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );

    } finally {

        //
        //  Release the Vcb.
        //

        CdReleaseVcb( IrpContext, Vcb );
        
        if (AbnormalTermination() || !NT_SUCCESS( Status )) {

            FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_LOCK_FAILED );
        }
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;

    CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  We won't check for a valid Vcb for this request.  An unlock will always
    //  succeed on a locked volume.
    //

    Status = CdUnlockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );

    //
    //  Release all of our resources
    //

    CdReleaseVcb( IrpContext, Vcb );

    //
    //  Send notification that the volume is avaliable.
    //

    if (NT_SUCCESS( Status )) {

        FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_UNLOCK );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}



//
//  Local support routine
//

NTSTATUS
CdDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.  We only dismount a volume which
    has been locked.  The intent here is that someone has locked the volume (they are the
    only remaining handle).  We set the verify bit here and the user will close his handle.
    We will dismount a volume with no user's handles in the verify path.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    if (CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    //
    //  Make this request waitable.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    
    //
    //  Acquire exclusive access to the Vcb,  and take the global resource to
    //  sync. against mounts,  verifies etc.
    //

    CdAcquireCdData( IrpContext );
    CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  Mark the volume as needs to be verified, but only do it if
    //  the vcb is locked by this handle and the volume is currently mounted.
    //

    if (Vcb->VcbCondition != VcbMounted) {

        Status = STATUS_VOLUME_DISMOUNTED;

    } else {

        //
        //  Invalidate the volume right now.
        //
        //  The intent here is to make every subsequent operation
        //  on the volume fail and grease the rails toward dismount.
        //  By definition there is no going back from a SURPRISE.
        //
            
        CdLockVcb( IrpContext, Vcb );
        
        if (Vcb->VcbCondition != VcbDismountInProgress) {
        
            CdUpdateVcbCondition( Vcb, VcbInvalid);
        }
        
        CdUnlockVcb( IrpContext, Vcb );

        //
        //  Set flag to tell the close path that we want to force dismount
        //  the volume when this handle is closed.
        //
        
        SetFlag( Ccb->Flags, CCB_FLAG_DISMOUNT_ON_CLOSE);
        
        Status = STATUS_SUCCESS;
    }

    //
    //  Release all of our resources
    //

    CdReleaseVcb( IrpContext, Vcb );
    CdReleaseCdData( IrpContext);

    //
    //  Complete the request if there haven't been any exceptions.
    //

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

CdIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;

    PULONG VolumeState;
    
    //
    //  Get the current stack location and extract the output
    //  buffer information.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        VolumeState = Irp->AssociatedIrp.SystemBuffer;

    } else {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the volume isn't dirty.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    *VolumeState = 0;

    //
    //  Decode the file object
    //

    TypeOfOpen = CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (Fcb->Vcb->VcbCondition != VcbMounted) {

        CdCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Now set up to return the clean state.  CDs obviously can never be dirty
    //  but we want to make sure we have enforced the full semantics of this call.
    //
    
    Irp->IoStatus.Information = sizeof( ULONG );

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently mounted.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object.
    //

    CdDecodeFileObject( IrpContext, IrpSp->FileObject, &Fcb, &Ccb );

    if (Fcb != NULL) {

        //
        //  Disable PopUps, we want to return any error.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS );

        //
        //  Verify the Vcb.  This will raise in the error condition.
        //

        CdVerifyVcb( IrpContext, Fcb->Vcb );
    }

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if pathname is a valid CDFS pathname.
    We always succeed this request.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    None

--*/

{
    PAGED_CODE();

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdInvalidateVolumes (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine searches for all the volumes mounted on the same real device
    of the current DASD handle, and marks them all bad.  The only operation
    that can be done on such handles is cleanup and close.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    KIRQL SavedIrql;
    
    BOOLEAN UnlockVcb = FALSE;
    
    LUID TcbPrivilege = {SE_TCB_PRIVILEGE, 0};

    HANDLE Handle;

    PVCB Vcb;

    PLIST_ENTRY Links;

    PFILE_OBJECT FileToMarkBad;
    PDEVICE_OBJECT DeviceToMarkBad;

    //
    //  We only allow the invalidate call to come in on our file system devices.
    //
    
    if (IrpSp->DeviceObject != CdData.FileSystemDeviceObject)  {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Check for the correct security access.
    //  The caller must have the SeTcbPrivilege.
    //

    if (!SeSinglePrivilegeCheck( TcbPrivilege, Irp->RequestorMode )) {

        CdCompleteRequest( IrpContext, Irp, STATUS_PRIVILEGE_NOT_HELD );

        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    //  Try to get a pointer to the device object from the handle passed in.
    //

#if defined(_WIN64)
    if (IoIs32bitProcess( Irp )) {
        
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof( UINT32 )) {

            CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        Handle = (HANDLE) LongToHandle( *((PUINT32) Irp->AssociatedIrp.SystemBuffer) );
    
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof( HANDLE )) {

            CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }
        Handle = *((PHANDLE) Irp->AssociatedIrp.SystemBuffer);
#if defined(_WIN64)
    }
#endif

    Status = ObReferenceObjectByHandle( Handle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        &FileToMarkBad,
                                        NULL );

    if (!NT_SUCCESS(Status)) {

        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Grab the DeviceObject from the FileObject.
    //

    DeviceToMarkBad = FileToMarkBad->DeviceObject;

    //
    //  We only needed the device object involved, not a reference to the file.
    //

    ObDereferenceObject( FileToMarkBad );

    //
    //  Make sure this request can wait.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

    //
    //  Synchronise with pnp/mount/verify paths.
    //
    
    CdAcquireCdData( IrpContext );

    //
    //  Nothing can go wrong now.
    //

    //
    //  Now walk through all the mounted Vcb's looking for candidates to
    //  mark invalid.
    //
    //  On volumes we mark invalid, check for dismount possibility (which is
    //  why we have to get the next link so early).
    //

    Links = CdData.VcbQueue.Flink;

    while (Links != &CdData.VcbQueue) {

        Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks);

        Links = Links->Flink;

        //
        //  If we get a match, mark the volume Bad, and also check to
        //  see if the volume should go away.
        //

        CdLockVcb( IrpContext, Vcb );

        if (Vcb->Vpb->RealDevice == DeviceToMarkBad) {

            //
            //  Take the VPB spinlock,  and look to see if this volume is the 
            //  one currently mounted on the actual device.  If it is,  pull it 
            //  off immediately.
            //
            
            IoAcquireVpbSpinLock( &SavedIrql );

            if (DeviceToMarkBad->Vpb == Vcb->Vpb)  {
            
                PVPB NewVpb = Vcb->SwapVpb;

                ASSERT( FlagOn( Vcb->Vpb->Flags, VPB_MOUNTED));
                ASSERT( NULL != NewVpb);

                RtlZeroMemory( NewVpb, sizeof( VPB ) );

                NewVpb->Type = IO_TYPE_VPB;
                NewVpb->Size = sizeof( VPB );
                NewVpb->RealDevice = DeviceToMarkBad;
                NewVpb->Flags = FlagOn( DeviceToMarkBad->Vpb->Flags, VPB_REMOVE_PENDING );

                DeviceToMarkBad->Vpb = NewVpb;
                Vcb->SwapVpb = NULL;
            }

            IoReleaseVpbSpinLock( SavedIrql );

            if (Vcb->VcbCondition != VcbDismountInProgress) {
                
                CdUpdateVcbCondition( Vcb, VcbInvalid);
            }

            CdUnlockVcb( IrpContext, Vcb );

            CdAcquireVcbExclusive( IrpContext, Vcb, FALSE);
            
            CdPurgeVolume( IrpContext, Vcb, FALSE );

            UnlockVcb = CdCheckForDismount( IrpContext, Vcb, FALSE );

            if (UnlockVcb)  {

                CdReleaseVcb( IrpContext, Vcb);
            }

        } else {

            CdUnlockVcb( IrpContext, Vcb );
        }
    }

    CdReleaseCdData( IrpContext );

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
CdScanForDismountedVcb (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine walks through the list of Vcb's looking for any which may
    now be deleted.  They may have been left on the list because there were
    outstanding references.

Arguments:

Return Value:

    None

--*/

{
    PVCB Vcb;
    PLIST_ENTRY Links;

    PAGED_CODE();

    //
    //  Walk through all of the Vcb's attached to the global data.
    //

    Links = CdData.VcbQueue.Flink;

    while (Links != &CdData.VcbQueue) {

        Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks );

        //
        //  Move to the next link now since the current Vcb may be deleted.
        //

        Links = Links->Flink;

        //
        //  If dismount is already underway then check if this Vcb can
        //  go away.
        //

        if ((Vcb->VcbCondition == VcbDismountInProgress) ||
            (Vcb->VcbCondition == VcbInvalid) ||
            ((Vcb->VcbCondition == VcbNotMounted) && (Vcb->VcbReference <= CDFS_RESIDUAL_REFERENCE))) {

            CdCheckForDismount( IrpContext, Vcb, FALSE );
        }
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
CdFindPrimaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCHAR RawIsoVd,
    IN ULONG BlockFactor,
    IN BOOLEAN ReturnOnError,
    IN BOOLEAN VerifyVolume
    )

/*++

Routine Description:

    This routine is called to walk through the volume descriptors looking
    for a primary volume descriptor.  When/if a primary is found a 32-bit
    serial number is generated and stored into the Vpb.  We also store the
    location of the primary volume descriptor in the Vcb.

Arguments:

    Vcb - Pointer to the VCB for the volume.

    RawIsoVd - Pointer to a sector buffer which will contain the primary
               volume descriptor on exit, if successful.

    BlockFactor - Block factor used by the current device for the TableOfContents.

    ReturnOnError - Indicates that we should raise on I/O errors rather than
        returning a FALSE value.

    VerifyVolume - Indicates if we were called from the verify path.  We
        do a few things different in this path.  We don't update the Vcb in
        the verify path.

Return Value:

    BOOLEAN - TRUE if a valid primary volume descriptor found, FALSE
              otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ThisPass = 1;
    BOOLEAN FoundVd = FALSE;

    ULONG BaseSector;
    ULONG SectorOffset;

    PCDROM_TOC CdromToc;

    ULONG VolumeFlags;

    PAGED_CODE();

    //
    //  If there are no data tracks, don't even bother hunting for descriptors.
    //
    //  This explicitly breaks various non-BlueBook compliant CDs that scribble
    //  an ISO filesystem on media claiming only audio tracks.  Since these
    //  disks can cause serious problems in some CDROM units, fail fast.  I admit
    //  that it is possible that someone can still record the descriptors in the
    //  audio track, record a data track (but fail to record descriptors there)
    //  and still have the disk work.  As this form of error worked in NT 4.0, and
    //  since these disks really do exist, I don't want to change them.
    //
    //  If we wished to support all such media (we don't), it would be neccesary
    //  to clear this flag on finding ISO or HSG descriptors below.
    //

    if (FlagOn(Vcb->VcbState, VCB_STATE_AUDIO_DISK)) {

        return FALSE;
    }
    
    //
    //  We will make at most two passes through the volume descriptor sequence.
    //
    //  On the first pass we will query for the last session.  Using this
    //  as a starting offset we will attempt to mount the volume.  On any failure
    //  we will go to the second pass and try without using any multi-session
    //  information.
    //
    //  On the second pass we will start offset from sector zero.
    //

    while (!FoundVd && (ThisPass <= 2)) {

        //
        //  If we aren't at pass 1 then we start at sector 0.  Otherwise we
        //  try to look up the multi-session information.
        //

        BaseSector = 0;

        if (ThisPass == 1) {

            CdromToc = NULL;

            //
            //  Check for whether this device supports XA and multi-session.
            //

            try {

                //
                //  Allocate a buffer for the last session information.
                //

                CdromToc = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                     sizeof( CDROM_TOC ),
                                                     TAG_CDROM_TOC );

                RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

                //
                //  Query the last session information from the driver.
                //

                Status = CdPerformDevIoCtrl( IrpContext,
                                             IOCTL_CDROM_GET_LAST_SESSION,
                                             Vcb->TargetDeviceObject,
                                             CdromToc,
                                             sizeof( CDROM_TOC ),
                                             FALSE,
                                             TRUE,
                                             NULL );

                //
                //  Raise an exception if there was an allocation failure.
                //

                if (Status == STATUS_INSUFFICIENT_RESOURCES) {

                    CdRaiseStatus( IrpContext, Status );
                }

                //
                //  We don't handle any errors yet.  We will hit that below
                //  as we try to scan the disk.  If we have last session information
                //  then modify the base sector.
                //

                if (NT_SUCCESS( Status ) &&
                    (CdromToc->FirstTrack != CdromToc->LastTrack)) {

                    PCHAR Source, Dest;
                    ULONG Count;

                    Count = 4;

                    //
                    //  The track address is BigEndian, we need to flip the bytes.
                    //

                    Source = (PUCHAR) &CdromToc->TrackData[0].Address[3];
                    Dest = (PUCHAR) &BaseSector;

                    do {

                        *Dest++ = *Source--;

                    } while (--Count);

                    //
                    //  Now adjust the base sector by the block factor of the
                    //  device.
                    //

                    BaseSector /= BlockFactor;

                //
                //  Make this look like the second pass since we are only using the
                //  first session.  No reason to retry on error.
                //

                } else {

                    ThisPass += 1;
                }

            } finally {

                if (CdromToc != NULL) { CdFreePool( &CdromToc ); }
            }
        }

        //
        //  Compute the starting sector offset from the start of the session.
        //

        SectorOffset = FIRST_VD_SECTOR;

        //
        //  Start by assuming we have neither Hsg or Iso volumes.
        //

        VolumeFlags = 0;

        //
        //  Loop until either error encountered, primary volume descriptor is
        //  found or a terminal volume descriptor is found.
        //

        while (TRUE) {

            //
            //  Attempt to read the desired sector. Exit directly if operation
            //  not completed.
            //
            //  If this is pass 1 we will ignore errors in read sectors and just
            //  go to the next pass.
            //

            if (!CdReadSectors( IrpContext,
                                LlBytesFromSectors( BaseSector + SectorOffset ),
                                SECTOR_SIZE,
                                (BOOLEAN) ((ThisPass == 1) || ReturnOnError),
                                RawIsoVd,
                                Vcb->TargetDeviceObject )) {

                break;
            }

            //
            //  Check if either an ISO or HSG volume.
            //

            if (RtlEqualMemory( CdIsoId,
                                CdRvdId( RawIsoVd, VCB_STATE_ISO ),
                                VOL_ID_LEN )) {

                SetFlag( VolumeFlags, VCB_STATE_ISO );

            } else if (RtlEqualMemory( CdHsgId,
                                       CdRvdId( RawIsoVd, VCB_STATE_HSG ),
                                       VOL_ID_LEN )) {

                SetFlag( VolumeFlags, VCB_STATE_HSG );

            //
            //  We have neither so break out of the loop.
            //

            } else {

                 break;
            }

            //
            //  Break out if the version number is incorrect or this is
            //  a terminator.
            //

            if ((CdRvdVersion( RawIsoVd, VolumeFlags ) != VERSION_1) ||
                (CdRvdDescType( RawIsoVd, VolumeFlags ) == VD_TERMINATOR)) {

                break;
            }

            //
            //  If this is a primary volume descriptor then our search is over.
            //

            if (CdRvdDescType( RawIsoVd, VolumeFlags ) == VD_PRIMARY) {

                //
                //  If we are not in the verify path then initialize the
                //  fields in the Vcb with basic information from this
                //  descriptor.
                //

                if (!VerifyVolume) {

                    //
                    //  Set the flag for the volume type.
                    //

                    SetFlag( Vcb->VcbState, VolumeFlags );

                    //
                    //  Store the base sector and sector offset for the
                    //  primary volume descriptor.
                    //

                    Vcb->BaseSector = BaseSector;
                    Vcb->VdSectorOffset = SectorOffset;
                    Vcb->PrimaryVdSectorOffset = SectorOffset;
                }

                FoundVd = TRUE;
                break;
            }

            //
            //  Indicate that we're at the next sector.
            //

            SectorOffset += 1;
        }

        ThisPass += 1;
    }

    return FoundVd;
}


//
//  Local support routine
//

BOOLEAN
CdIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PVCB *OldVcb
    )
/*++

Routine Description:

    This routine walks through the links of the Vcb chain in the global
    data structure.  The remount condition is met when the following
    conditions are all met:

        If the new Vcb is a device only Mvcb and there is a previous
        device only Mvcb.

        Otherwise following conditions must be matched.

            1 - The 32 serial in the current VPB matches that in a previous
                VPB.

            2 - The volume label in the Vpb matches that in the previous
                Vpb.

            3 - The system pointer to the real device object in the current
                VPB matches that in the same previous VPB.

            4 - Finally the previous Vcb cannot be invalid or have a dismount
                underway.

    If a VPB is found which matches these conditions, then the address of
    the VCB for that VPB is returned via the pointer Vcb.

    Skip over the current Vcb.

Arguments:

    Vcb - This is the Vcb we are checking for a remount.

    OldVcb -  A pointer to the address to store the address for the Vcb
              for the volume if this is a remount.  (This is a pointer to
              a pointer)

Return Value:

    BOOLEAN - TRUE if this is in fact a remount, FALSE otherwise.

--*/

{
    PLIST_ENTRY Link;

    PVPB Vpb = Vcb->Vpb;
    PVPB OldVpb;

    BOOLEAN Remount = FALSE;

    PAGED_CODE();

    //
    //  Check whether we are looking for a device only Mvcb.
    //

    for (Link = CdData.VcbQueue.Flink;
         Link != &CdData.VcbQueue;
         Link = Link->Flink) {

        *OldVcb = CONTAINING_RECORD( Link, VCB, VcbLinks );

        //
        //  Skip ourselves.
        //

        if (Vcb == *OldVcb) { continue; }

        //
        //  Look at the Vpb and state of the previous Vcb.
        //

        OldVpb = (*OldVcb)->Vpb;

        if ((OldVpb != Vpb) &&
            (OldVpb->RealDevice == Vpb->RealDevice) &&
            ((*OldVcb)->VcbCondition == VcbNotMounted)) {

            //
            //  If the current disk is a raw disk then it can match a previous music or
            //  raw disk.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_AUDIO_DISK)) {

                if (FlagOn( (*OldVcb)->VcbState, VCB_STATE_AUDIO_DISK )) {

                    //
                    //  If we have both TOC then fail the remount if the lengths
                    //  are different or they don't match.
                    //

                    if ((Vcb->TocLength != (*OldVcb)->TocLength) ||
                        ((Vcb->TocLength != 0) &&
                         !RtlEqualMemory( Vcb->CdromToc,
                                          (*OldVcb)->CdromToc,
                                          Vcb->TocLength ))) {

                        continue;
                    }

                    Remount = TRUE;
                    break;
                }

            //
            //  The current disk is not a raw disk.  Go ahead and compare
            //  serial numbers and volume label.
            //

            } else if ((OldVpb->SerialNumber == Vpb->SerialNumber) &&
                       (Vpb->VolumeLabelLength == OldVpb->VolumeLabelLength) &&
                       (RtlEqualMemory( OldVpb->VolumeLabel,
                                        Vpb->VolumeLabel,
                                        Vpb->VolumeLabelLength ))) {

                //
                //  Remember the old mvcb.  Then set the return value to
                //  TRUE and break.
                //

                Remount = TRUE;
                break;
            }
        }
    }

    return Remount;
}


//
//  Local support routine
//

VOID
CdFindActiveVolDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PCHAR RawIsoVd,
    IN BOOLEAN VerifyVolume
    )

/*++

Routine Description:

    This routine is called to search for a valid secondary volume descriptor that
    we will support.  Right now we only support Joliet escape sequences for
    the secondary descriptor.

    If we don't find the secondary descriptor then we will reread the primary.

    This routine will update the serial number and volume label in the Vpb.

Arguments:

    Vcb - This is the Vcb for the volume being mounted.

    RawIsoVd - Sector buffer used to read the volume descriptors from the disks, but
               on input should contain the PVD (ISO) in the SECOND 'sector' of the
               buffer.

    VerifyVolume - indicates we are being called by the verify path, and should
                   not modify the Vcb fields.

Return Value:

    None

--*/

{
    BOOLEAN FoundSecondaryVd = FALSE;
    ULONG SectorOffset = FIRST_VD_SECTOR;

    ULONG Length;

    ULONG Index;

    PAGED_CODE();

    //
    //  We only look for secondary volume descriptors on an Iso disk.
    //

    if ((FlagOn( Vcb->VcbState, VCB_STATE_ISO) || VerifyVolume) && !CdNoJoliet) {

        //
        //  Scan the volume descriptors from the beginning looking for a valid
        //  secondary or a terminator.
        //

        SectorOffset = FIRST_VD_SECTOR;

        while (TRUE) {

            //
            //  Read the next sector.  We should never have an error in this
            //  path.
            //

            CdReadSectors( IrpContext,
                           LlBytesFromSectors( Vcb->BaseSector + SectorOffset ),
                           SECTOR_SIZE,
                           FALSE,
                           RawIsoVd,
                           Vcb->TargetDeviceObject );

            //
            //  Break out if the version number or standard Id is incorrect.
            //  Also break out if this is a terminator.
            //

            if (!RtlEqualMemory( CdIsoId, CdRvdId( RawIsoVd, VCB_STATE_JOLIET ), VOL_ID_LEN ) ||
                (CdRvdVersion( RawIsoVd, VCB_STATE_JOLIET ) != VERSION_1) ||
                (CdRvdDescType( RawIsoVd, VCB_STATE_JOLIET ) == VD_TERMINATOR)) {

                break;
            }

            //
            //  We have a match if this is a secondary descriptor with a matching
            //  escape sequence.
            //

            if ((CdRvdDescType( RawIsoVd, VCB_STATE_JOLIET ) == VD_SECONDARY) &&
                (RtlEqualMemory( CdRvdEsc( RawIsoVd, VCB_STATE_JOLIET ),
                                 CdJolietEscape[0],
                                 ESC_SEQ_LEN ) ||
                 RtlEqualMemory( CdRvdEsc( RawIsoVd, VCB_STATE_JOLIET ),
                                 CdJolietEscape[1],
                                 ESC_SEQ_LEN ) ||
                 RtlEqualMemory( CdRvdEsc( RawIsoVd, VCB_STATE_JOLIET ),
                                 CdJolietEscape[2],
                                 ESC_SEQ_LEN ))) {

                if (!VerifyVolume)  {
                        
                    //
                    //  Update the Vcb with the new volume descriptor.
                    //

                    ClearFlag( Vcb->VcbState, VCB_STATE_ISO );
                    SetFlag( Vcb->VcbState, VCB_STATE_JOLIET );

                    Vcb->VdSectorOffset = SectorOffset;
                }
                
                FoundSecondaryVd = TRUE;
                break;
            }

            //
            //  Otherwise move on to the next sector.
            //

            SectorOffset += 1;
        }

        //
        //  If we didn't find the secondary then recover the original volume
        //  descriptor stored in the second half of the RawIsoVd.
        //

        if (!FoundSecondaryVd) {

            RtlCopyMemory( RawIsoVd,
                           Add2Ptr( RawIsoVd, SECTOR_SIZE, PVOID ),
                           SECTOR_SIZE );
        }
    }

    //
    //  If we're in the verify path,  our work is done,  since we don't want
    //  to update any Vcb/Vpb values.
    //
    
    if (VerifyVolume)  {

        return;
    }
        
    //
    //  Compute the serial number and volume label from the volume descriptor.
    //

    Vcb->Vpb->SerialNumber = CdSerial32( RawIsoVd, SECTOR_SIZE );

    //
    //  Make sure the CD label will fit in the Vpb.
    //

    ASSERT( VOLUME_ID_LENGTH * sizeof( WCHAR ) <= MAXIMUM_VOLUME_LABEL_LENGTH );

    //
    //  If this is not a Unicode label we must convert it to unicode.
    //

    if (!FlagOn( Vcb->VcbState, VCB_STATE_JOLIET )) {

        //
        //  Convert the label to unicode.  If we get any error then use a name
        //  length of zero.
        //

        Vcb->Vpb->VolumeLabelLength = 0;

        if (NT_SUCCESS( RtlOemToUnicodeN( &Vcb->Vpb->VolumeLabel[0],
                                          MAXIMUM_VOLUME_LABEL_LENGTH,
                                          &Length,
                                          CdRvdVolId( RawIsoVd, Vcb->VcbState ),
                                          VOLUME_ID_LENGTH ))) {

            Vcb->Vpb->VolumeLabelLength = (USHORT) Length;
        }

    //
    //  We need to convert from big-endian to little endian.
    //

    } else {

        CdConvertBigToLittleEndian( IrpContext,
                                    CdRvdVolId( RawIsoVd, Vcb->VcbState ),
                                    VOLUME_ID_LENGTH,
                                    (PCHAR) Vcb->Vpb->VolumeLabel );

        Vcb->Vpb->VolumeLabelLength = VOLUME_ID_LENGTH * sizeof( WCHAR );
    }

    //
    //  Strip the trailing spaces or zeroes from the name.
    //

    Index = Vcb->Vpb->VolumeLabelLength / sizeof( WCHAR );

    while (Index > 0) {

        if ((Vcb->Vpb->VolumeLabel[ Index - 1 ] != L'\0') &&
            (Vcb->Vpb->VolumeLabel[ Index - 1 ] != L' ')) {

            break;
        }

        Index -= 1;
    }

    //
    //  Now set the final length for the name.
    //

    Vcb->Vpb->VolumeLabelLength = (USHORT) (Index * sizeof( WCHAR ));
}



