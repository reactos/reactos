/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Udfs called
    by the Fsd/Fsp dispatch drivers.

Author:

    Dan Lovinger    [DanLo]   11-Jun-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_FSCTRL)

//
//  Local constants
//

BOOLEAN UdfDisable = FALSE;

//
//  Local macros
//

INLINE
VOID
UdfStoreFileSetDescriptorIfPrevailing (
    IN OUT PNSR_FSD *StoredFSD,
    IN OUT PNSR_FSD *NewFSD
    )
{
    PNSR_FSD TempFSD;

    //
    //  If we haven't stored a fileset descriptor or the fileset number
    //  of the stored descriptor is less than the new descriptor, swap the
    //  pointers around.
    //

    if (*StoredFSD == NULL || (*StoredFSD)->FileSet < (*NewFSD)->FileSet) {

        TempFSD = *StoredFSD;
        *StoredFSD = *NewFSD;
        *NewFSD = TempFSD;
    }
}

//
//  Local support routines
//

VOID
UdfDetermineVolumeBounding ( 
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PULONG S,
    IN PULONG N
    );

NTSTATUS
UdfDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfFindAnchorVolumeDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PNSR_ANCHOR *AnchorVolumeDescriptor
    );

NTSTATUS
UdfFindFileSetDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLONGAD LongAd,
    IN OUT PNSR_FSD *FileSetDescriptor
    );

NTSTATUS
UdfFindVolumeDescriptors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PEXTENTAD Extent,
    IN OUT PPCB *Pcb,
    IN OUT PNSR_PVD *PrimaryVolumeDescriptor,
    IN OUT PNSR_LVOL *LogicalVolumeDescriptor
    );

NTSTATUS
UdfInvalidateVolumes (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
UdfIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PVCB *OldVcb
    );

UdfIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfMountVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
UdfRecognizeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN OUT PBOOLEAN Bridge
    );

VOID
UdfScanForDismountedVcb (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
UdfUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
UdfUpdateVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PWCHAR VolumeLabel,
    IN OUT PUSHORT VolumeLabelLength,
    IN PUCHAR Dstring,
    IN UCHAR FieldLength
    );

VOID
UdfUpdateVolumeSerialNumber (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PULONG VolumeSerialNumber,
    IN PNSR_FSD Fsd
    );

NTSTATUS
UdfUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonFsControl)
#pragma alloc_text(PAGE, UdfDetermineVolumeBounding)
#pragma alloc_text(PAGE, UdfDismountVolume)
#pragma alloc_text(PAGE, UdfFindAnchorVolumeDescriptor)
#pragma alloc_text(PAGE, UdfFindFileSetDescriptor)
#pragma alloc_text(PAGE, UdfFindVolumeDescriptors)
#pragma alloc_text(PAGE, UdfIsPathnameValid)
#pragma alloc_text(PAGE, UdfIsRemount)
#pragma alloc_text(PAGE, UdfIsVolumeDirty)
#pragma alloc_text(PAGE, UdfIsVolumeMounted)
#pragma alloc_text(PAGE, UdfLockVolume)
#pragma alloc_text(PAGE, UdfLockVolumeInternal)
#pragma alloc_text(PAGE, UdfMountVolume)
#pragma alloc_text(PAGE, UdfOplockRequest)
#pragma alloc_text(PAGE, UdfRecognizeVolume)
#pragma alloc_text(PAGE, UdfScanForDismountedVcb)
#pragma alloc_text(PAGE, UdfStoreVolumeDescriptorIfPrevailing)
#pragma alloc_text(PAGE, UdfUnlockVolume)
#pragma alloc_text(PAGE, UdfUnlockVolumeInternal)
#pragma alloc_text(PAGE, UdfUpdateVolumeLabel)
#pragma alloc_text(PAGE, UdfUpdateVolumeSerialNumber)
#pragma alloc_text(PAGE, UdfUserFsctl)
#pragma alloc_text(PAGE, UdfVerifyVolume)
#endif


VOID
UdfStoreVolumeDescriptorIfPrevailing (
    IN OUT PNSR_VD_GENERIC *StoredVD,
    IN OUT PNSR_VD_GENERIC NewVD
    )

/*++

Routine Description:

    This routine updates Volume Descriptor if the new descriptor
    is more prevailing than the one currently stored.

Arguments:

    StoredVD - pointer to a currently stored descriptor

    NewVD - pointer to a candidate descriptor

Return Value:

    None.

--*/

{
    PNSR_VD_GENERIC TempVD;

    //
    //  If we haven't stored a volume descriptor or the sequence number
    //  of the stored descriptor is less than the new descriptor, make a copy
    //  of it and store it.
    //

    if ((NULL == *StoredVD) || ((*StoredVD)->Sequence < NewVD->Sequence)) {

        if ( NULL == *StoredVD)  {

            *StoredVD = (PNSR_VD_GENERIC) FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                                    sizeof(NSR_VD_GENERIC),
                                                                    TAG_NSR_VDSD );
        }

        RtlCopyMemory( *StoredVD,  NewVD,  sizeof( NSR_VD_GENERIC));
    }
}


NTSTATUS
UdfCommonFsControl (
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

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_MOUNT_VOLUME:

        Status = UdfMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = UdfVerifyVolume( IrpContext, Irp );
        break;

    case IRP_MN_USER_FS_REQUEST:

        Status = UdfUserFsctl( IrpContext, Irp );
        break;

    default:

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfUserFsctl (
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

        Status = UdfOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME :

        Status = UdfLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME :

        Status = UdfUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME :

        Status = UdfDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_DIRTY :

        Status = UdfIsVolumeDirty( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED :

        Status = UdfIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID :

        Status = UdfIsPathnameValid( IrpContext, Irp );
        break;

    case FSCTL_INVALIDATE_VOLUMES :

        Status = UdfInvalidateVolumes( IrpContext, Irp );
        break;


    //
    //  We don't support any of the known or unknown requests.
    //

    case FSCTL_MARK_VOLUME_DIRTY :
    case FSCTL_QUERY_RETRIEVAL_POINTERS :
    case FSCTL_GET_COMPRESSION :
    case FSCTL_SET_COMPRESSION :
    case FSCTL_MARK_AS_SYSTEM_HIVE :
    case FSCTL_QUERY_FAT_BPB :
    default:

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfOplockRequest (
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

    if (UdfDecodeFileObject( IrpSp->FileObject,
                             &Fcb,
                             &Ccb ) != UserFileOpen ) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
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

        UdfAcquireFcbExclusive( IrpContext, Fcb, FALSE );

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

        UdfAcquireFcbShared( IrpContext, Fcb, FALSE );
        break;

    default:

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Verify the Fcb.
        //

        UdfVerifyFcbOperation( IrpContext, Fcb );

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        UdfLockFcb( IrpContext, Fcb );
        Fcb->IsFastIoPossible = UdfIsFastIoPossible( Fcb );
        UdfUnlockFcb( IrpContext, Fcb );

        //
        //  The oplock package will complete the Irp.
        //

        Irp = NULL;

    } finally {

        //
        //  Release all of our resources
        //

        UdfReleaseFcb( IrpContext, Fcb );
    }

    //
    //  Complete the request if there was no exception.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfLockVolumeInternal (
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
    NTSTATUS FinalStatus = (FileObject? STATUS_ACCESS_DENIED: STATUS_DEVICE_BUSY);
    ULONG RemainingUserReferences = (FileObject? 1: 0);

    PAGED_CODE();

    ASSERT_EXCLUSIVE_VCB( Vcb );
    
    //
    //  If the volume is already locked then complete with success if this file
    //  object has the volume locked, fail otherwise.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

        if (FileObject && Vcb->VolumeLockFileObject == FileObject) {

            FinalStatus = STATUS_SUCCESS;
        }

    } else if (Vcb->VcbCleanup == RemainingUserReferences) {

        //
        //  The cleanup count for the volume only reflects the fileobject that
        //  will lock the volume.  Otherwise, we must fail the request.
        //
        //  Since the only cleanup is for the provided fileobject, we will try
        //  to get rid of all of the other user references.  If there is only one
        //  remaining after the purge then we can allow the volume to be locked.
        //
        
        UdfPurgeVolume( IrpContext, Vcb, FALSE );

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

        UdfReleaseVcb( IrpContext, Vcb );

        Status = CcWaitForCurrentLazyWriterActivity();

        //
        //  This is intentional. If we were able to get the Vcb before, just
        //  wait for it and take advantage of knowing that it is OK to leave
        //  the flag up.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
        UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );
        
        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        UdfFspClose( Vcb );

        if (Vcb->VcbUserReference == Vcb->VcbResidualUserReference + RemainingUserReferences) {

            SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            Vcb->VolumeLockFileObject = FileObject;
            FinalStatus = STATUS_SUCCESS;
        }
    }
    
    return FinalStatus;
}


NTSTATUS
UdfUnlockVolumeInternal (
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
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    
    if (FlagOn(Vcb->VcbState, VCB_STATE_LOCKED) && FileObject == Vcb->VolumeLockFileObject) {

        ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
        Vcb->VolumeLockFileObject = NULL;
        Status = STATUS_SUCCESS;
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfLockVolume (
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

    if (UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

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
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    try {

        //
        //  Verify the Vcb.
        //

        UdfVerifyVcb( IrpContext, Vcb );

        Status = UdfLockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );

    } finally {

        //
        //  Release the Vcb.
        //

        UdfReleaseVcb( IrpContext, Vcb );

        if (AbnormalTermination() || !NT_SUCCESS( Status )) {

            FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_LOCK_FAILED );
        }
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfUnlockVolume (
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

    if (UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;

    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  We won't check for a valid Vcb for this request.  An unlock will always
    //  succeed on a locked volume.
    //

    Status = UdfUnlockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );    
    
    //
    //  Release all of our resources
    //

    UdfReleaseVcb( IrpContext, Vcb );

    //
    //  Send notification that the volume is avaliable.
    //

    if (NT_SUCCESS( Status )) {

        FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_UNLOCK );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}



//
//  Local support routine
//

NTSTATUS
UdfDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.  We only dismount a volume which
    has been locked.  The intent here is that someone has locked the volume (they are the
    only remaining handle).  We set the volume state to invalid so that it will be torn
    down quickly.

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

    if (UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb ) != UserVolumeOpen ) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Send notification.
    //
    
    FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_DISMOUNT );

    //
    //  Acquire exclusive access to the Vcb.
    //

    Vcb = Fcb->Vcb;

    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    //
    //  Mark the volume as invalid, but only do it if the vcb is locked
    //  by this handle and the volume is currently mounted.  No more
    //  operations will occur on this vcb except cleanup/close.
    //

    if ((Vcb->VcbCondition != VcbMounted) &&
        (Vcb->VolumeLockFileObject != IrpSp->FileObject)) {

        Status = STATUS_NOT_IMPLEMENTED;

    } else {

        SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        SetFlag( Vcb->VcbState, VCB_STATE_NOTIFY_REMOUNT );

        Status = STATUS_SUCCESS;
    }

    //
    //  Release all of our resources
    //

    UdfReleaseVcb( IrpContext, Vcb );

    if (!NT_SUCCESS( Status )) {

        FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_DISMOUNT_FAILED );
    }

    //
    //  Complete the request if there haven't been any exceptions.
    //

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

UdfIsVolumeDirty (
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
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PULONG VolumeState;
    
    //
    //  Get the current stack location and extract the output
    //  buffer information.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in the
    //  irp first.  Then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        VolumeState = Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        VolumeState = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (NULL == VolumeState)  {
        
            UdfCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the volume isn't dirty.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    *VolumeState = 0;

    //
    //  Decode the file object
    //

    TypeOfOpen = UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (Fcb->Vcb->VcbCondition != VcbMounted) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Now set up to return the clean state.  If we paid attention to the dirty
    //  state of the media we could be more accurate, but since this is a readonly
    //  implementation at the moment we think it is clean all of the time.
    //
    
    Irp->IoStatus.Information = sizeof( ULONG );

    UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
UdfIsVolumeMounted (
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

    UdfDecodeFileObject( IrpSp->FileObject, &Fcb, &Ccb );

    if (Fcb != NULL) {

        //
        //  Disable PopUps, we want to return any error.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS );

        //
        //  Verify the Vcb.  This will raise in the error condition.
        //

        UdfVerifyVcb( IrpContext, Fcb->Vcb );
    }

    UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
UdfIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if pathname is a valid UDFS pathname.
    We always succeed this request.

Arguments:

    Irp - Supplies the Irp to process.

Return Value:

    None

--*/

{
    PAGED_CODE();

    UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
UdfInvalidateVolumes (
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

    LUID TcbPrivilege = {SE_TCB_PRIVILEGE, 0};

    HANDLE Handle;

    PVPB NewVpb;
    PVCB Vcb;

    PLIST_ENTRY Links;

    PFILE_OBJECT FileToMarkBad;
    PDEVICE_OBJECT DeviceToMarkBad;

    //
    //  Check for the correct security access.
    //  The caller must have the SeTcbPrivilege.
    //

    if (!SeSinglePrivilegeCheck( TcbPrivilege, Irp->RequestorMode )) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_PRIVILEGE_NOT_HELD );

        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    //  Try to get a pointer to the device object from the handle passed in.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof( HANDLE )) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    Handle = *((PHANDLE) Irp->AssociatedIrp.SystemBuffer);

    Status = ObReferenceObjectByHandle( Handle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        &FileToMarkBad,
                                        NULL );

    if (!NT_SUCCESS(Status)) {

        UdfCompleteRequest( IrpContext, Irp, Status );
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
    //  Create a new Vpb for this device so that any new opens will mount
    //  a new volume.
    //

    NewVpb = ExAllocatePoolWithTag( NonPagedPoolMustSucceed, sizeof( VPB ), TAG_VPB );
    RtlZeroMemory( NewVpb, sizeof( VPB ) );

    NewVpb->Type = IO_TYPE_VPB;
    NewVpb->Size = sizeof( VPB );
    NewVpb->RealDevice = DeviceToMarkBad;
    NewVpb->Flags = FlagOn( DeviceToMarkBad->Vpb->Flags, VPB_REMOVE_PENDING );

    //
    //  Make sure this request can wait.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

    UdfAcquireUdfData( IrpContext );

    //
    //  Nothing can go wrong now.
    //

    IoAcquireVpbSpinLock( &SavedIrql );
    DeviceToMarkBad->Vpb = NewVpb;
    IoReleaseVpbSpinLock( SavedIrql );

    //
    //  Now walk through all the mounted Vcb's looking for candidates to
    //  mark invalid.
    //
    //  On volumes we mark invalid, check for dismount possibility (which is
    //  why we have to get the next link so early).
    //

    Links = UdfData.VcbQueue.Flink;

    while (Links != &UdfData.VcbQueue) {

        Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks);

        Links = Links->Flink;

        //
        //  If we get a match, mark the volume Bad, and also check to
        //  see if the volume should go away.
        //

        UdfLockVcb( IrpContext, Vcb );

        if (Vcb->Vpb->RealDevice == DeviceToMarkBad) {

            if (Vcb->VcbCondition != VcbDismountInProgress) {
                
                Vcb->VcbCondition = VcbInvalid;
            }

            UdfUnlockVcb( IrpContext, Vcb );

            UdfPurgeVolume( IrpContext, Vcb, FALSE );

            UdfCheckForDismount( IrpContext, Vcb, FALSE );

        } else {

            UdfUnlockVcb( IrpContext, Vcb );
        }
    }

    UdfReleaseUdfData( IrpContext );

    UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
UdfMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is a UDF volume,
    and create the VCB and root directory FCB structures.  The algorithm it
    uses is essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do I/O
       through the on-disk volume descriptors.

    2. Read the disk and check if it is a UDF volume.

    3. If it is not a UDF volume then delete the Vcb and
       complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves deleting the VCB, hook in the
       old VCB, and complete the IRP.

    5. Otherwise create a Vcb and root directory FCB

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PVOLUME_DEVICE_OBJECT VolDo = NULL;
    PVCB Vcb = NULL;
    PVCB OldVcb = NULL;
    PPCB Pcb = NULL;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    PVPB Vpb = IrpSp->Parameters.MountVolume.Vpb;

    PFILE_OBJECT FileObjectToNotify = NULL;

    ULONG MediaChangeCount = 0;

    DISK_GEOMETRY DiskGeometry;

    PNSR_ANCHOR AnchorVolumeDescriptor = NULL;
    PNSR_PVD PrimaryVolumeDescriptor = NULL;
    PNSR_LVOL LogicalVolumeDescriptor = NULL;
    PNSR_FSD FileSetDescriptor = NULL;

    BOOLEAN BridgeMedia;

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Check that we are talking to a Cdrom or Disk device.  This request should
    //  always be waitable.
    //

    ASSERT( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ||
            Vpb->RealDevice->DeviceType == FILE_DEVICE_DISK );
    ASSERT( FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT ));

    DebugTrace(( +1, Dbg, "UdfMountVolume\n" ));

    //
    //  Update the real device in the IrpContext from the Vpb.  There was no available
    //  file object when the IrpContext was created.
    //

    IrpContext->RealDevice = Vpb->RealDevice;

    //
    //  Check if we have disabled the mount process.
    //

    if (UdfDisable) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
        DebugTrace(( 0, Dbg, "UdfMountVolume, disabled\n" ));
        DebugTrace(( -1, Dbg, "UdfMountVolume -> STATUS_UNRECOGNIZED_VOLUME\n" ));

        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Do a CheckVerify here to lift the MediaChange ticker from the driver
    //

    Status = UdfPerformDevIoCtrl( IrpContext,
                                  ( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
                                    IOCTL_CDROM_CHECK_VERIFY :
                                    IOCTL_DISK_CHECK_VERIFY ),
                                  DeviceObjectWeTalkTo,
                                  &MediaChangeCount,
                                  sizeof(ULONG),
                                  FALSE,
                                  TRUE,
                                  NULL );

    if (!NT_SUCCESS( Status )) {
        
        UdfCompleteRequest( IrpContext, Irp, Status );
        DebugTrace(( 0, Dbg,
                     "UdfMountVolume, CHECK_VERIFY handed back status %08x (so don't continue)\n",
                     Status ));
        DebugTrace(( -1, Dbg,
                     "UdfMountVolume -> %08x\n",
                     Status ));

        return Status;
    }
    
    //
    //  Now let's make Jeff delirious and call to get the disk geometry.  This
    //  will fix the case where the first change line is swallowed.
    //
    //  This IOCTL does not have a generic STORAGE equivalent, so we must figure
    //  our which variant to pass down from the real underlying device object (as
    //  opposed to the top of the driver filter stack we will really be attaching
    //  on top of).
    //

    Status = UdfPerformDevIoCtrl( IrpContext,
                                  ( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
                                    IOCTL_CDROM_GET_DRIVE_GEOMETRY :
                                    IOCTL_DISK_GET_DRIVE_GEOMETRY ),
                                  DeviceObjectWeTalkTo,
                                  &DiskGeometry,
                                  sizeof( DISK_GEOMETRY ),
                                  FALSE,
                                  TRUE,
                                  NULL );

    //
    //  If this call failed, we might be able to get away with a heuristic guess as to
    //  what the sector size is (per CDFS), but that is playing with fire.  Nearly every
    //  failure here will be a permanent problem of some form.
    //

    if (!NT_SUCCESS( Status )) {

        UdfCompleteRequest( IrpContext, Irp, Status );
        DebugTrace(( 0, Dbg, "UdfMountVolume, GET_DRIVE_GEOMETRY failed\n" ));
        DebugTrace(( -1, Dbg,
                     "UdfMountVolume -> %08x\n",
                     Status ));

        return Status;
    }

    //
    //  Acquire the global resource to do mount operations.
    //

    UdfAcquireUdfData( IrpContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Do a quick check to see if there any Vcb's which can be removed.
        //

        UdfScanForDismountedVcb( IrpContext );

        //
        //  Make sure that the driver/drive is not screwing up underneath of us by
        //  feeding us garbage for the sector size.
        //

        if (DiskGeometry.BytesPerSector == 0 ||
            (DiskGeometry.BytesPerSector & ~( 1 << UdfHighBit( DiskGeometry.BytesPerSector ))) != 0) {

            DebugTrace(( 0, 0,
                         "UdfMountVolume, bad DiskGeometry (%08x) .BytesPerSector == %08x\n",
                         &DiskGeometry,
                         DiskGeometry.BytesPerSector ));

            ASSERT( FALSE );

            try_leave( Status = STATUS_DRIVER_INTERNAL_ERROR );
        }

        //
        //  Now go confirm that this volume may be a UDF image by looking for a
        //  valid ISO 13346 Volume Recognition Sequence.
        //

        if (!UdfRecognizeVolume( IrpContext,
                                 DeviceObjectWeTalkTo,
                                 DiskGeometry.BytesPerSector,
                                 &BridgeMedia )) {

            DebugTrace(( 0, Dbg, "UdfMountVolume, recognition failed so not mounting\n" ));

            try_leave( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Create the DeviceObject for this mount attempt
        //

        Status = IoCreateDevice( UdfData.DriverObject,
                                 sizeof( VOLUME_DEVICE_OBJECT ) - sizeof( DEVICE_OBJECT ),
                                 NULL,
                                 FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                                 0,
                                 FALSE,
                                 (PDEVICE_OBJECT *) &VolDo );

        if (!NT_SUCCESS( Status )) {

            DebugTrace(( 0, Dbg, "UdfMountVolume, couldn't get voldo! (%08x)\n", Status ));
            try_leave( Status );
        }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

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

        UdfInitializeVcb( IrpContext,
                          &VolDo->Vcb,
                          DeviceObjectWeTalkTo,
                          Vpb,
                          &DiskGeometry,
                          MediaChangeCount );

        //
        //  We must initialize the stack size in our device object before
        //  the following reads, because the I/O system has not done it yet.
        //

        ((PDEVICE_OBJECT) VolDo)->StackSize = (CCHAR) (DeviceObjectWeTalkTo->StackSize + 1);

        //
        //  Pick up a local pointer to the new Vcb.  Here is where we start
        //  thinking about cleanup of structures if the mount is failed.
        //

        Vcb = &VolDo->Vcb;
        Vpb = NULL;
        VolDo = NULL;

        //
        //  Store the Vcb in the IrpContext as we didn't have one before.
        //

        IrpContext->Vcb = Vcb;

        UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

        //
        //  Let's reference the Vpb to make sure we are the one to
        //  have the last dereference.
        //

        Vcb->Vpb->ReferenceCount += 1;

        //
        //  Clear the verify bit for the start of mount.
        //

        ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

        //
        //  Now find the multi-session bounds on this media.
        //

        UdfDetermineVolumeBounding( IrpContext,
                                    Vcb,
                                    &Vcb->BoundS,
                                    &Vcb->BoundN );

        //
        //  Now find the Anchor Volume Descriptor so we can discover the Volume Set
        //  Descriptor Sequence extent.
        //

        Status = UdfFindAnchorVolumeDescriptor( IrpContext,
                                                Vcb,
                                                &AnchorVolumeDescriptor );

        if (!NT_SUCCESS(Status)) {

            DebugTrace(( 0, Dbg, "UdfMountVolume, couldn't find anchor descriptors\n" ));
            try_leave( Status );
        }

        //
        //  Now search for the prevailing copies of the PVD, LVD, and related PD in the
        //  extents indicated by the AVD.
        //

        Status = UdfFindVolumeDescriptors( IrpContext,
                                           Vcb,
                                           &AnchorVolumeDescriptor->Main,
                                           &Pcb,
                                           &PrimaryVolumeDescriptor,
                                           &LogicalVolumeDescriptor );

        //
        //  If we discovered invalid structures on the main extent, we may still
        //  be able to use the reserve extent.  By definition the two extents
        //  must be logically equal, so just plow into it on any error.
        //

        if (!NT_SUCCESS( Status )) {

            Status = UdfFindVolumeDescriptors( IrpContext,
                                               Vcb,
                                               &AnchorVolumeDescriptor->Reserve,
                                               &Pcb,
                                               &PrimaryVolumeDescriptor,
                                               &LogicalVolumeDescriptor );
        }

        if (!NT_SUCCESS(Status)) {

            DebugTrace(( 0, Dbg, "UdfMountVolume, couldn't find good VSD descriptors (PVD/LVD/PD)\n" ));
            try_leave( Status );
        }

        //
        //  Now go complete initialization of the Pcb.  After this point, we can perform
        //  physical partition mappings and know that the partition table is good.
        //

        Status = UdfCompletePcb( IrpContext,
                                 Vcb,
                                 Pcb );

        if (!NT_SUCCESS(Status)) {

            DebugTrace(( 0, Dbg, "UdfMountVolume, Pcb completion failed\n" ));
            try_leave( Status );
        }

        Vcb->Pcb = Pcb;
        Pcb = NULL;

        //
        //  Set up all the support we need to do reads into the volume.
        //

        UdfUpdateVcbPhase0( IrpContext, Vcb );

        //
        //  Now go get the fileset descriptor that will finally reveal the location
        //  of the root directory on this volume.
        //

        Status = UdfFindFileSetDescriptor( IrpContext,
                                           Vcb,
                                           &LogicalVolumeDescriptor->FSD,
                                           &FileSetDescriptor );

        if (!NT_SUCCESS(Status)) {

            try_leave( NOTHING );
        }

        //
        //  Now that we have everything together, update the Vpb with identification
        //  of this volume.
        //

        UdfUpdateVolumeLabel( IrpContext,
                              Vcb->Vpb->VolumeLabel,
                              &Vcb->Vpb->VolumeLabelLength,
                              LogicalVolumeDescriptor->VolumeID,
                              sizeof( LogicalVolumeDescriptor->VolumeID ));

        UdfUpdateVolumeSerialNumber( IrpContext,
                                     &Vcb->Vpb->SerialNumber,
                                     FileSetDescriptor );

        //
        //  Check if this is a remount operation.  If so then clean up
        //  the data structures passed in and created here.
        //

        if (UdfIsRemount( IrpContext, Vcb, &OldVcb )) {

            //
            //  Link the old Vcb to point to the new device object that we
            //  should be talking to, dereferencing the previous.
            //

            ObDereferenceObject( OldVcb->TargetDeviceObject );

            Vcb->Vpb->RealDevice->Vpb = OldVcb->Vpb;

            OldVcb->Vpb->RealDevice = Vcb->Vpb->RealDevice;

            OldVcb->TargetDeviceObject = DeviceObjectWeTalkTo;
            OldVcb->VcbCondition = VcbMounted;

            OldVcb->MediaChangeCount = Vcb->MediaChangeCount;

            //
            //  Push the state of the method 2 bit across.  In changing the device,
            //  we may now be on one with a different requirement.
            //

            ClearFlag( OldVcb->VcbState, VCB_STATE_METHOD_2_FIXUP );
            SetFlag( OldVcb->VcbState, FlagOn( Vcb->VcbState, VCB_STATE_METHOD_2_FIXUP ));
            
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
            
            DebugTrace(( 0, Dbg, "UdfMountVolume, remounted old Vcb %08x\n", OldVcb ));

            try_leave( Status = STATUS_SUCCESS );
        }

        //
        //  Initialize the Vcb and associated structures from our volume descriptors
        //

        UdfUpdateVcbPhase1( IrpContext,
                            Vcb,
                            FileSetDescriptor );

        //
        //  Drop an extra reference on the root dir file so we'll be able to send
        //  notification.
        //

        if (Vcb->RootIndexFcb) {

            FileObjectToNotify = Vcb->RootIndexFcb->FileObject;
            ObReferenceObject( FileObjectToNotify );
        }

        //
        //  The new mount is complete.  Remove the additional references on this
        //  Vcb since, at this point, we have added the real references this volume
        //  will have during its lifetime.  We also need to drop the additional
        //  reference on the device we mounted.
        //

        Vcb->VcbReference -= Vcb->VcbResidualReference;
        ASSERT( Vcb->VcbReference == Vcb->VcbResidualReference );

        ObDereferenceObject( Vcb->TargetDeviceObject );

        Vcb->VcbCondition = VcbMounted;

        UdfReleaseVcb( IrpContext, Vcb );
        Vcb = NULL;

        Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( "UdfMountVolume" );

        //
        //  If we didn't complete the mount then cleanup any remaining structures.
        //

        if (Vpb != NULL) { Vpb->DeviceObject = NULL; }

        if (Pcb != NULL) {

            UdfDeletePcb( Pcb );
        }

        if (Vcb != NULL) {

            //
            //  Make sure there is no Vcb in the IrpContext since it could go away
            //

            IrpContext->Vcb = NULL;

            Vcb->VcbReference -= Vcb->VcbResidualReference;

            if (UdfDismountVcb( IrpContext, Vcb )) {

                UdfReleaseVcb( IrpContext, Vcb );
            }

        } else if (VolDo != NULL) {

            IoDeleteDevice( (PDEVICE_OBJECT)VolDo );
            Vpb->DeviceObject = NULL;
        }

        //
        //  Release the global resource.
        //

        UdfReleaseUdfData( IrpContext );

        //
        //  Free any structures we may have been allocated
        //

        UdfFreePool( &AnchorVolumeDescriptor );
        UdfFreePool( &PrimaryVolumeDescriptor );
        UdfFreePool( &LogicalVolumeDescriptor );
        UdfFreePool( &FileSetDescriptor );
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

    UdfCompleteRequest( IrpContext, Irp, Status );
    DebugTrace(( -1, Dbg, "UdfMountVolume -> %08x\n", Status ));

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfVerifyVolume (
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

    PPCB Pcb = NULL;

    PNSR_ANCHOR AnchorVolumeDescriptor = NULL;
    PNSR_PVD PrimaryVolumeDescriptor = NULL;
    PNSR_LVOL LogicalVolumeDescriptor = NULL;
    PNSR_FSD FileSetDescriptor = NULL;

    ULONG MediaChangeCount = 0;
    ULONG Index;

    PFILE_OBJECT FileObjectToNotify = NULL;

    BOOLEAN ReturnError;
    BOOLEAN ReleaseVcb;

    IO_STATUS_BLOCK Iosb;

    WCHAR VolumeLabel[ MAXIMUM_VOLUME_LABEL_LENGTH / sizeof( WCHAR )];
    USHORT VolumeLabelLength;
    ULONG VolumeSerialNumber;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Check that we are talking to a Cdrom or Disk device.  This request should
    //  always be waitable.
    //

    ASSERT( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ||
            Vpb->RealDevice->DeviceType == FILE_DEVICE_DISK );

    ASSERT_VCB( Vcb );

    //
    //  Update the real device in the IrpContext from the Vpb.  There was no available
    //  file object when the IrpContext was created.
    //

    IrpContext->RealDevice = Vpb->RealDevice;

    //
    //  Acquire shared global access, the termination handler for the
    //  following try statement will free the access.
    //

    UdfAcquireUdfData( IrpContext );
    UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );
    ReleaseVcb = TRUE;

    DebugTrace(( +1, Dbg, "UdfVerifyVolume, Vcb %08x\n", Vcb ));

    try {

        //
        //  Check if the real device still needs to be verified.  If it doesn't
        //  then obviously someone beat us here and already did the work
        //  so complete the verify irp with success.  Otherwise reenable
        //  the real device and get to work.
        //

        if (!FlagOn( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

            DebugTrace(( 0, Dbg, "UdfVerifyVolume, verify bit was cleared out ahead of us\n" ));

            MediaChangeCount = Vcb->MediaChangeCount;
            try_leave( Status = STATUS_SUCCESS );
        }

        //
        //  Verify that there is a disk here.
        //

        Status = UdfPerformDevIoCtrl( IrpContext,
                                      ( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
                                        IOCTL_CDROM_CHECK_VERIFY :
                                        IOCTL_DISK_CHECK_VERIFY ),
                                      Vcb->TargetDeviceObject,
                                      &MediaChangeCount,
                                      sizeof(ULONG),
                                      FALSE,
                                      TRUE,
                                      &Iosb );

        if (!NT_SUCCESS( Status )) {

            DebugTrace(( 0, Dbg, "UdfVerifyVolume, CHECK_VERIFY failed\n" ));

            //
            //  If we will allow a raw mount then return WRONG_VOLUME to
            //  allow the volume to be mounted by raw.
            //

            if (FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {

                DebugTrace(( 0, Dbg, "UdfVerifyVolume, ... allowing raw mount\n" ));

                Status = STATUS_WRONG_VOLUME;
            }

            try_leave( Status );
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

        if (MediaChangeCount == 0 || Vcb->MediaChangeCount != MediaChangeCount) {

            //
            //  Now we need to navigate the disc to find the relavent decriptors.  This is
            //  much the same as the mount process.
            //

            //
            //  Find the AVD.
            //

            Status = UdfFindAnchorVolumeDescriptor( IrpContext,
                                                    Vcb,
                                                    &AnchorVolumeDescriptor );

            if (!NT_SUCCESS(Status)) {
                
                DebugTrace(( 0, Dbg, "UdfVerifyVolume, No AVD visible\n" ));
                try_leave( Status = STATUS_WRONG_VOLUME );
            }
            
            //
            //  Get the prevailing descriptors out of the VDS, building a fresh Pcb.
            //

            Status = UdfFindVolumeDescriptors( IrpContext,
                                               Vcb,
                                               &AnchorVolumeDescriptor->Main,
                                               &Pcb,
                                               &PrimaryVolumeDescriptor,
                                               &LogicalVolumeDescriptor );

            //
            //  Try the reserve sequence in case of error.
            //

            if (Status == STATUS_DISK_CORRUPT_ERROR) {

                Status = UdfFindVolumeDescriptors( IrpContext,
                                                   Vcb,
                                                   &AnchorVolumeDescriptor->Reserve,
                                                   &Pcb,
                                                   &PrimaryVolumeDescriptor,
                                                   &LogicalVolumeDescriptor );
            }

            //
            //  If we're totally unable to find a VDS, give up.
            //

            if (!NT_SUCCESS(Status)) {

                DebugTrace(( 0, Dbg, "UdfVerifyVolume, PVD/LVD/PD pickup failed\n" ));

                try_leave( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  Now go complete initialization of the Pcb so we can compare it.
            //

            Status = UdfCompletePcb( IrpContext,
                                     Vcb,
                                     Pcb );

            if (!NT_SUCCESS(Status)) {

                DebugTrace(( 0, Dbg, "UdfVerifyVolume, Pcb completion failed\n" ));

                try_leave( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  Now let's compare this new Pcb to the previous Vcb's Pcb to see if they
            //  appear to be equivalent.
            //

            if (!UdfEquivalentPcb( IrpContext,
                                   Pcb,
                                   Vcb->Pcb)) {

                DebugTrace(( 0, Dbg, "UdfVerifyVolume, Pcbs are not equivalent\n" ));

                try_leave( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  At this point we know that the Vcb's Pcb is OK for mapping to find the fileset
            //  descriptor, so we can drop the new one we built for comparison purposes.
            //

            UdfDeletePcb( Pcb );
            Pcb = NULL;

            //
            //  Go pick up the fileset descriptor.
            //

            Status = UdfFindFileSetDescriptor( IrpContext,
                                               Vcb,
                                               &LogicalVolumeDescriptor->FSD,
                                               &FileSetDescriptor );

            if (!NT_SUCCESS(Status)) {

                try_leave( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  Now that everything is in place, build a volume label and serial number from these
            //  descriptors and perform the final check that this Vcb is (or is not) the right one
            //  for the media now in the drive.
            //

            UdfUpdateVolumeLabel( IrpContext,
                                  VolumeLabel,
                                  &VolumeLabelLength,
                                  LogicalVolumeDescriptor->VolumeID,
                                  sizeof( LogicalVolumeDescriptor->VolumeID ));

            UdfUpdateVolumeSerialNumber( IrpContext,
                                         &VolumeSerialNumber,
                                         FileSetDescriptor );

            if (Vcb->Vpb->SerialNumber != VolumeSerialNumber ||
                Vcb->Vpb->VolumeLabelLength != VolumeLabelLength ||
                RtlCompareMemory( Vcb->Vpb->VolumeLabel,
                                  VolumeLabel,
                                  VolumeLabelLength )) {

                DebugTrace(( 0, Dbg, "UdfVerifyVolume, volume label/sn mismatch\n" ));

                try_leave( Status = STATUS_WRONG_VOLUME );
            }
        }

        //
        //  The volume is OK, clear the verify bit.
        //

        DebugTrace(( 0, Dbg, "UdfVerifyVolume, looks like the same volume\n" ));

        Vcb->VcbCondition = VcbMounted;

        ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

        //
        //  See if we will need to provide notification of the remount.  This is the readonly
        //  filesystem's form of dismount/mount notification.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_NOTIFY_REMOUNT )) {

            ClearFlag( Vcb->VcbState, VCB_STATE_NOTIFY_REMOUNT );
            
            FileObjectToNotify = Vcb->RootIndexFcb->FileObject;
            ObReferenceObject( FileObjectToNotify );
        }
        
    } finally {

        //
        //  If we did not raise an exception, update the current Vcb.
        //

        if (!AbnormalTermination()) {

            //
            //  Update the media change count to note that we have verified the volume
            //  at this value
            //

            Vcb->MediaChangeCount = MediaChangeCount;

            //
            //  Mark the Vcb as not mounted.
            //

            if (Status == STATUS_WRONG_VOLUME) {

                Vcb->VcbCondition = VcbNotMounted;
		
		//
		//  Now, if there are no user handles to the volume, try to spark
		//  teardown by purging the volume.
		//

		if (Vcb->VcbCleanup == 0) {

		    if (NT_SUCCESS( UdfPurgeVolume( IrpContext, Vcb, FALSE ))) {

			ReleaseVcb = UdfCheckForDismount( IrpContext, Vcb, FALSE );
		    }
		}
            }
        }

        DebugTrace(( -1, Dbg, "UdfVerifyVolume -> %08x\n", Status ));

        if (ReleaseVcb) {
	    
	    UdfReleaseVcb( IrpContext, Vcb );
	}

        UdfReleaseUdfData( IrpContext );

	//
        //  Delete the Pcb if built.
        //

        if (Pcb != NULL) {

            UdfDeletePcb( Pcb );
        }

        UdfFreePool( &AnchorVolumeDescriptor );
        UdfFreePool( &PrimaryVolumeDescriptor );
        UdfFreePool( &LogicalVolumeDescriptor );
        UdfFreePool( &FileSetDescriptor );
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

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

BOOLEAN
UdfIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PVCB *OldVcb
    )

/*++

Routine Description:

    This routine walks through the links of the Vcb chain in the global
    data structure.  The remount condition is met when the following
    conditions are all met:

            1 - The 32 serial for this VPB matches that in a previous
                VPB.

            2 - The volume label for this VPB matches that in the previous
                VPB.

            3 - The system pointer to the real device object in the current
                VPB matches that in the same previous VPB.

            4 - Finally the previous Vcb cannot be invalid or have a dismount
                underway.

    If a VPB is found which matches these conditions, then the address of
    the Vcb for that VPB is returned via the pointer OldVcb.

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
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace(( +1, Dbg, "UdfIsRemount, Vcb %08x\n", Vcb ));

    for (Link = UdfData.VcbQueue.Flink;
         Link != &UdfData.VcbQueue;
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
            //  Go ahead and compare serial numbers and volume label.
            //

            if ((OldVpb->SerialNumber == Vpb->SerialNumber) &&
                       (Vpb->VolumeLabelLength == OldVpb->VolumeLabelLength) &&
                       (RtlEqualMemory( OldVpb->VolumeLabel,
                                        Vpb->VolumeLabel,
                                        Vpb->VolumeLabelLength ))) {

                //
                //  Got it.
                //

                DebugTrace(( 0, Dbg, "UdfIsRemount, matched OldVcb %08x\n", *OldVcb ));

                Remount = TRUE;
                break;
            }
        }
    }

    DebugTrace(( -1, Dbg, "UdfIsRemount -> %c\n", (Remount? 'T' : 'F' )));

    return Remount;
}


//
//  Local support routine
//

NTSTATUS
UdfFindFileSetDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLONGAD LongAd,
    IN OUT PNSR_FSD *FileSetDescriptor
    )

/*++

Routine Description:

    This routine walks a Fileset Descriptor Sequence looking for the default
    descriptor.  This will reveal the location of the root directory on the
    volume.

Arguments:

    Vcb - Vcb of volume to search

    LongAd - Long allocation descriptor describing the start of the sequence

    FileSetDescriptor - Address of caller's pointer to an FSD

Return Value:

    STATUS_SUCCESS if all descriptors are found, read, and are valid.

    STATUS_DISK_CORRUPT_ERROR if corrupt/bad descriptors are found (may be raised)

--*/

{
    PNSR_FSD FSD = NULL;
    ULONGLONG Offset;
    ULONG Lbn, Len;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( *FileSetDescriptor == NULL );

    DebugTrace(( +1, Dbg,
                 "UdfFindFileSetDescriptor, Vcb %08x, LongAd %08x %x/%08x +%08x (type %x)\n",
                 Vcb,
                 LongAd,
                 LongAd->Start.Partition,
                 LongAd->Start.Lbn,
                 LongAd->Length.Length,
                 LongAd->Length.Type ));
    
    //
    //  If the extent we begin from is not a whole number of recorded logical blocks,
    //  we can't continue.
    //

#ifndef UDF_SUPPORT_NONSTANDARD_ALLSTOR
    
    //
    //  Disable checking the sanity of the longad here.
    //
    //  Reason: first drop of Allstor media recorded the type as unrecorded (!)
    //

    if (LongAd->Length.Length == 0 ||
        LongAd->Length.Type != NSRLENGTH_TYPE_RECORDED ||
        BlockOffset( Vcb, LongAd->Length.Length )) {

        DebugTrace(( +0, Dbg,
                     "UdfFindFileSetDescriptor, bad longad length\n" ));
        DebugTrace(( -1, Dbg,
                     "UdfFindFileSetDescriptor ->  STATUS_DISK_CORRUPT_ERROR\n" ));
        
        return STATUS_DISK_CORRUPT_ERROR;
    }

#endif

    //
    //  Use a try-finally for cleanup
    //

    try {

        try {
            
            for ( //
                  //  Home ourselves in the search and make a pass through the sequence.
                  //

                  Len = LongAd->Length.Length,
                  Lbn = LongAd->Start.Lbn,
                  Offset = LlBytesFromSectors( Vcb, UdfLookupPsnOfExtent( IrpContext,
                                                                          Vcb,
                                                                          LongAd->Start.Partition,
                                                                          Lbn,
                                                                          Len ));

                  Len;

                  //
                  //  Advance to the next descriptor offset in the sequence.
                  //

                  Len -= BlockSize( Vcb ),
                  Lbn++,
                  Offset += BlockSize( Vcb )) {

                //
                //  Allocate a buffer to read fileset descriptors.
                //

                if (FSD == NULL) {

                    FSD = FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                    UdfRawBufferSize( Vcb, sizeof(NSR_FSD) ),
                                                    TAG_NSR_FSD );
                }

                Status = UdfReadSectors( IrpContext,
                                         Offset,
                                         UdfRawReadSize( Vcb, sizeof(NSR_FSD) ),
                                         TRUE,
                                         FSD,
                                         Vcb->TargetDeviceObject );

                if (!NT_SUCCESS( Status ) ||
                    FSD->Destag.Ident == DESTAG_ID_NOTSPEC) {

                    //
                    //  These are both an excellent sign that this is an unrecorded sector, which
                    //  is defined to terminate the sequence. (3/8.4.2)
                    //

                    break;
                }

                if ((FSD->Destag.Ident != DESTAG_ID_NSR_FSD &&
                     FSD->Destag.Ident != DESTAG_ID_NSR_TERM) ||

                    !UdfVerifyDescriptor( IrpContext,
                                          &FSD->Destag,
                                          FSD->Destag.Ident,
                                          sizeof(NSR_FSD),
                                          Lbn,
                                          TRUE)) {

                    //
                    //  If we spot an illegal descriptor type in the stream, there is no reasonable
                    //  way to guess that we can continue (the disc may be trash beyond this point).
                    //  Clearly, we also cannot trust the next extent pointed to by a corrupt
                    //  descriptor.
                    //

                    try_leave( Status = STATUS_DISK_CORRUPT_ERROR );
                }

                if (FSD->Destag.Ident == DESTAG_ID_NSR_TERM) {

                    //
                    //  This is a way to terminate the sequence.
                    //

                    break;
                }

                //
                //  Reset the pointers to the possible next extent
                //

                LongAd = &FSD->NextExtent;

                if (LongAd->Length.Length) {

                    //
                    //  A fileset descriptor containing a nonzero next extent pointer also
                    //  terminates this extent of the FSD sequence. (4/8.3.1)
                    //
                    //  If the extent referred to is not fully recorded, this will
                    //  terminate the sequence.
                    //

                    if (LongAd->Length.Type != NSRLENGTH_TYPE_RECORDED) {

                        break;
                    }

                    Len = LongAd->Length.Length;

                    //
                    //  The extent must be a multiple of a block size.
                    //

                    if (BlockOffset( Vcb, Len )) {

                        DebugTrace(( +0, Dbg,
                                     "UdfFindFileSetDescriptor, interior extent not blocksize in length\n" ));
                        try_leave ( Status = STATUS_DISK_CORRUPT_ERROR );
                    }

                    Lbn = LongAd->Start.Lbn;

                    Offset = LlBytesFromBlocks( Vcb, UdfLookupPsnOfExtent( IrpContext,
                                                                           Vcb,
                                                                           LongAd->Start.Partition,
                                                                           Lbn,
                                                                           Len ));

                }

                UdfStoreFileSetDescriptorIfPrevailing( FileSetDescriptor, &FSD );
            }
        
        } finally {
            
            //
            //  Free up the buffer space we may have allocated
            //

            UdfFreePool( &FSD );

        }
    
    } except( UdfExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  Transmute raised apparent file corruption to disk corruption - we are not
        //  yet touching the visible filesystem.
        //

        Status = IrpContext->ExceptionStatus;
        
        DebugTrace(( +0, Dbg,
                     "UdfFindFileSetDescriptor, exception %08x thrown\n", Status ));

        if (Status == STATUS_FILE_CORRUPT_ERROR) {

            DebugTrace(( +0, Dbg,
                         "UdfFindFileSetDescriptor, translating file corrupt to disk corrupt\n" ));
            Status = STATUS_DISK_CORRUPT_ERROR;
        }
    }

    //
    //  Success is when we've really found something.  If we failed to find the
    //  descriptor, commute whatever intermediate status was involved and clean up.
    //

    if (*FileSetDescriptor == NULL) {
        
        Status = STATUS_UNRECOGNIZED_VOLUME;
    }

    if (!NT_SUCCESS( Status )) {

        UdfFreePool( FileSetDescriptor );
    }
    
    DebugTrace(( -1, Dbg,
                 "UdfFindFileSetDescriptor -> %08x\n", Status ));
    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfFindVolumeDescriptors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PEXTENTAD Extent,
    IN OUT PPCB *Pcb,
    IN OUT PNSR_PVD *PrimaryVolumeDescriptor,
    IN OUT PNSR_LVOL *LogicalVolumeDescriptor
    )

/*++

Routine Description:

    This routine walks the indicated Volume Descriptor Sequence searching for the
    active descriptors for this volume and generates an initializing Pcb from the
    referenced partitions.  No updating of the Vcb occurs.

Arguments:

    Vcb - Vcb of volume to search

    Extent - Extent to search

    Pcb - Address of a caller's pointer to a Pcb

    PrimaryVolumeDescriptor - Address of caller's pointer to a PVD

    LogicalVolumeDescriptor - Address of caller's pointer to an LVD

Return Value:

    STATUS_SUCCESS if all descriptors are found, read, and are valid.

    STATUS_DISK_CORRUPT_ERROR if corrupt descriptors are found.

    STATUS_UNRECOGNIZED_VOLUME if noncompliant descriptors are found.
    
    Descriptors are only returned on success.

--*/

{
    PNSR_VD_GENERIC GenericVD = NULL;
    ULONGLONG Offset;
    ULONG Len;
    ULONG UnitSize = UdfRawReadSize( Vcb, sizeof(NSR_VD_GENERIC) );

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ThisPass = 1;

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_IRP_CONTEXT( IrpContext);
    ASSERT_VCB( Vcb );
    ASSERT_OPTIONAL_PCB( *Pcb );

    DebugTrace(( +1, Dbg,
                 "UdfFindVolumeDescriptors, Vcb %08x, Extent %08x +%08x\n",
                 Vcb,
                 Extent->Lsn,
                 Extent->Len ));

    //
    //  If the extent we begin from is not at least the size of an aligned descriptor
    //  or is sized in base units other than aligned descriptors, we can't continue.
    //

    if (Extent->Len < UnitSize ||
        Extent->Len % UnitSize) {

        DebugTrace(( 0, Dbg,
                     "UdfFindVolumeDescriptors, Base extent length %08x is mismatched with read size %08x\n",
                     Extent->Len,
                     UnitSize ));

        DebugTrace(( -1, Dbg,
                     "UdfFindVolumeDescriptors -> STATUS_DISK_CORRUPT_ERROR\n" ));

        return STATUS_DISK_CORRUPT_ERROR;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        DebugTrace(( 0, Dbg,
                     "UdfFindVolumeDescriptors, starting pass 1, find LVD/PVD\n" ));

        //
        //  We will make at least one pass through the Volume Descriptor Sequence to find
        //  the prevailing versions of the two controlling descriptors - the PVD and LVD.
        //  In order to avoid picking up partition descriptors that aren't actually going
        //  to be referenced by the LVD, we will pick them up in a second pass if we find
        //  a PVD and LVD that look reasonable and then stick them in a Pcb.
        //

        for (ThisPass = 1; ThisPass <= 2; ThisPass++) {

            for ( //
                  //  Home ourselves in the search and make a pass through the sequence.
                  //

                  Offset = LlBytesFromSectors( Vcb, Extent->Lsn ),
                  Len = Extent->Len;

                  //
                  //  If we have reached the end of the extent's indicated valid
                  //  length, we are done. This usually will not happen.
                  //

                  Len;

                  //
                  //  Advance to the next descriptor offset in the sequence.
                  //

                  Offset += UnitSize,
                  Len -= UnitSize ) {

                //
                //  Allocate a buffer to read generic volume descriptors.
                //

                if (GenericVD == NULL) {

                    GenericVD = (PNSR_VD_GENERIC) FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                                            UdfRawBufferSize( Vcb, sizeof(NSR_VD_GENERIC) ),
                                                                            TAG_NSR_VDSD );
                }

                Status = UdfReadSectors( IrpContext,
                                         Offset,
                                         UnitSize,
                                         TRUE,
                                         GenericVD,
                                         Vcb->TargetDeviceObject );

                //
                //  Thise is a decent sign that this is an unrecorded sector and is
                //  defined to terminate the sequence.
                //

                if (!NT_SUCCESS( Status )) {

                    break;
                }

                if (GenericVD->Destag.Ident > DESTAG_ID_MAXIMUM_PART3 ||

                    !UdfVerifyDescriptor( IrpContext,
                                          &GenericVD->Destag,
                                          GenericVD->Destag.Ident,
                                          sizeof(NSR_VD_GENERIC),
                                          (ULONG) SectorsFromBytes( Vcb, Offset ),
                                          TRUE)) {

                    //
                    //  If we spot an illegal descriptor type in the stream, there is no reasonable
                    //  way to guess that we can continue (the disc may be trash beyond this point).
                    //  Likewise, even if we have a single corrupt descriptor we cannot continue because
                    //  this may be corruption of a descriptor we may have otherwise required for operation
                    //  (i.e., one of the prevailing descriptors).
                    //

                    DebugTrace(( 0, Dbg,
                                 "UdfFindVolumeDescriptors, descriptor didn't verify\n" ));

                    try_leave( Status = STATUS_DISK_CORRUPT_ERROR );
                }

                if (GenericVD->Destag.Ident == DESTAG_ID_NSR_TERM) {

                    //
                    //  The Terminating Descriptor (3/10.9) is the usual way to stop a search.
                    //

                    break;
                }

                if (GenericVD->Destag.Ident == DESTAG_ID_NSR_VDP) {

                    //
                    //  Follow a Volume Desciptor Pointer (3/10.3) to the next extent of the sequence.
                    //

                    Offset = LlBytesFromSectors( Vcb, ((PNSR_VDP) GenericVD)->Next.Lsn );
                    Len = ((PNSR_VDP) GenericVD)->Next.Len;

                    //
                    //  We cannot do anything if the extent is invalid
                    //

                    if (Len < UnitSize ||
                        Len % UnitSize) {

                        DebugTrace(( 0, Dbg,
                                     "UdfFindVolumeDescriptors, following extent length %08x is mismatched with read size %08x\n",
                                     Extent->Len,
                                     UnitSize ));

                        try_leave( Status = STATUS_DISK_CORRUPT_ERROR );
                    }
                }

                DebugTrace(( 0, Dbg,
                             "UdfFindVolumeDescriptors, descriptor tag %08x\n",
                             GenericVD->Destag.Ident ));

                if (ThisPass == 1) {

                    //
                    //  Our first pass is to find prevailing LVD and PVD.
                    //

                    switch (GenericVD->Destag.Ident) {

                        case DESTAG_ID_NSR_PVD:

                            UdfStoreVolumeDescriptorIfPrevailing( (PNSR_VD_GENERIC *) PrimaryVolumeDescriptor,
                                                                  GenericVD );
                            break;

                        case DESTAG_ID_NSR_LVOL:

                            UdfStoreVolumeDescriptorIfPrevailing( (PNSR_VD_GENERIC *) LogicalVolumeDescriptor,
                                                                  GenericVD );
                            break;

                        default:

                            break;
                    }

                } else {

                    PNSR_PART PartitionDescriptor = (PNSR_PART) GenericVD;

                    //
                    //  Our second pass is to pick up all relavent NSR02 PD
                    //

                    if (PartitionDescriptor->Destag.Ident != DESTAG_ID_NSR_PART ||
                        !UdfEqualEntityId( &PartitionDescriptor->ContentsID, &UdfNSR02Identifier, NULL )) {

                        continue;
                    }

                    UdfAddToPcb( *Pcb, (PNSR_PART) GenericVD );
                }
            }

            //
            //  Now that a pass through the VDS has been completed, analyze the results.
            //

            if (ThisPass == 1) {

                PNSR_PVD PVD;
                PNSR_LVOL LVD;

                //
                //  Reference the descriptors for ease of use
                //

                PVD = *PrimaryVolumeDescriptor;
                LVD = *LogicalVolumeDescriptor;

                //
                //  Check that the descriptors indicate a logical volume which appears to
                //  be a valid UDF volume.
                //

                if ((PVD == NULL &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, don't have a PVD\n" ))) ||
                    (LVD == NULL &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, don't have an LVD\n" ))) ||

                    //
                    //  Now check the PVD
                    //

                    //
                    //  The Volume Set Sequence fields indicates how many volumes form
                    //  the volume set and what number this volume is in that sequence.
                    //  We are a level 2 implementation, meaning that the volumes we read
                    //  consist of a single volume. (3/11)
                    //

                    (PVD->VolSetSeq > 1 &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, PVD VolSetSeq %08x - not volume 1 of a volume set\n",
                                  PVD->VolSetSeq ))) ||
                    (PVD->VolSetSeqMax > 1 &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, PVD VolSetSeqMax %08x - volume in a non-unit volume set\n",
                                  PVD->VolSetSeqMax ))) ||

                    
#ifndef UDF_SUPPORT_NONSTANDARD_ALLSTOR

                    //
                    //  Disable checking of character set lists.
                    //
                    //  Reason: first drop of Allstor media recorded these fields as 0x0.
                    //

                    //
                    //  Insure that Character Set Lists conform to UDF
                    //

                    (PVD->CharSetList != UDF_CHARSETLIST &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, PVD CharSetList %08x != CS0 only\n",
                                  PVD->CharSetList ))) ||
                    (PVD->CharSetListMax != UDF_CHARSETLIST &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, PVD CharSetListMax %08x != CS0 only\n",
                                  PVD->CharSetListMax ))) ||

                    //
                    //  Disable checking of character set lists.
                    //
                    //  Reason: first drop of Allstor media misspelled "Compressed" as "Copmressed"
                    //

                    //
                    //  The two character sets must be UDF CS0.  CS0 is a "by convention"
                    //  character set in ISO 13346, which UDF specifies for our domain.
                    //

                    (!UdfEqualCharspec( &PVD->CharsetDesc, &UdfCS0Identifier, CHARSPEC_T_CS0 ) &&
                     DebugTrace(( 0, Dbg,
                                 "UdfFindVolumeDescriptors, PVD CharsetDesc != CS0 only\n" ))) ||
                    (!UdfEqualCharspec( &PVD->CharsetExplan, &UdfCS0Identifier, CHARSPEC_T_CS0 ) &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, PVD CharsetExplan != CS0 only\n" ))) ||

#endif
                    //
                    //  Now check the LVD
                    //

                    //
                    //  The LVD is a variant sized structure.  Check that the claimed size fits in a single
                    //  logical sector.  Although an LVD may legally exceed a single sector, we will never
                    //  want to deal with such a volume.
                    //

                    (ISONsrLvolSize( LVD ) > SectorSize( Vcb ) &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, LVD is bigger than a sector\n" ))) ||

#ifndef UDF_SUPPORT_NONSTANDARD_ALLSTOR
                    
                    //
                    //  Disable checking of character set lists.
                    //
                    //  Reason: first drop of Allstor media recorded these fields as 0x0.
                    //

                    //
                    //  The character set used in the LVD must be UDF CS0 as well.
                    //

                    (!UdfEqualCharspec( &LVD->Charset, &UdfCS0Identifier, CHARSPEC_T_CS0 ) &&
                     DebugTrace(( 0, Dbg,
                                 "UdfFindVolumeDescriptors, LVD Charset != CS0 only\n" ))) ||
#endif

                    //
                    //  The specified block size must equal the physical sector size.
                    //

                    (LVD->BlockSize != SectorSize( Vcb ) &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, LVD BlockSize %08x != SectorSize %08x\n" ))) ||

                    //
                    //  The domain must be within the version we read
                    //

                    (!UdfDomainIdentifierContained( &LVD->DomainID,
                                                    &UdfDomainIdentifier,
                                                    UDF_VERSION_MINIMUM,
                                                    UDF_VERSION_RECOGNIZED ) &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, domain ID indicates unreadable volume\n" ))) ||

                    //
                    //  Although we can handle any number of partitions, UDF only specifies
                    //  a single partition or special dual partition formats.
                    //

                    (LVD->MapTableCount > 2 &&
                     DebugTrace(( 0, Dbg,
                                  "UdfFindVolumeDescriptors, LVD MapTableCount %08x greater than allowed (2)\n",
                                  LVD->MapTableCount )))
                    ) {

                    DebugTrace(( 0, Dbg,
                                 "UdfFindVolumeDescriptors, ... so returning STATUS_UNRECOGNIZED_VOLUME\n" ));

                    try_leave( Status = STATUS_UNRECOGNIZED_VOLUME );
                }

                //
                //  Now that we have performed the simple field checks, build a Pcb.
                //

                Status = UdfInitializePcb( IrpContext, Vcb, Pcb, LVD );

                if (!NT_SUCCESS(Status)) {

                    DebugTrace(( 0, Dbg,
                                 "UdfFindVolumeDescriptors, Pcb intialization failed (!)\n" ));

                    try_leave( Status );
                }
            }

            //
            //  Go onto Pass 2 to find the Partition Descriptors
            //

            DebugTrace(( 0, Dbg,
                         "UdfFindVolumeDescriptors, starting pass 2, find associated PD\n" ));
        }

    } finally {

        DebugUnwind( "UdfFindVolumeDescriptors" );

        //
        //  Free up the buffer space we may have allocated
        //

        UdfFreePool( &GenericVD );
    }

    DebugTrace(( -1, Dbg,
                 "UdfFindVolumeDescriptors -> %08x\n", Status ));

    //
    //  Success is when we've really found something.  If we failed to find both
    //  descriptors, commute whatever intermediate status was involved and clean up.
    //

    if (*PrimaryVolumeDescriptor == NULL || *LogicalVolumeDescriptor == NULL) {
        
        Status = STATUS_UNRECOGNIZED_VOLUME;
    }

    if (!NT_SUCCESS( Status )) {
        
        UdfFreePool(PrimaryVolumeDescriptor);
        UdfFreePool(LogicalVolumeDescriptor);
    }
    
    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfFindAnchorVolumeDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PNSR_ANCHOR *AnchorVolumeDescriptor
    )

/*++

Routine Description:

    This routine will find the Anchor Volume Descriptor for a piece of media

Arguments:

    Vcb - Vcb of volume to search

    AnchorVolumeDescriptor - Caller's pointer to an AVD

Return Value:

    Boolean TRUE if AVD is discovered, FALSE otherwise.

--*/

{
    ULONG ThisPass;
    ULONG ReadLsn;
    ULONG Lsn;
    BOOLEAN Found = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_IRP_CONTEXT( IrpContext);
    ASSERT_VCB( Vcb );

    ASSERT(*AnchorVolumeDescriptor == NULL);

    //
    //  Discover the Anchor Volume Descriptor, which will point towards the
    //  Volume Set Descriptor Sequence.  The AVD may exist at sector 256 or
    //  in the last sector of the volume.
    //

    *AnchorVolumeDescriptor = (PNSR_ANCHOR) FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                                      UdfRawBufferSize( Vcb, sizeof(NSR_ANCHOR) ),
                                                                      TAG_NSR_VDSD );


    //
    //  Search the three possible locations for an AVD to exist on the volume,
    //  plus check for the possibility of a method 2 fixup requirement.
    //

    for ( ThisPass = 1; ThisPass <= 4; ThisPass++ ) {

        if (ThisPass == 1) {

            ReadLsn = Lsn = ANCHOR_SECTOR + Vcb->BoundS;

        } else if (ThisPass == 2) {

            //
            //  It is so unlikely that we will get a disk that doesn't have
            //  an anchor at 256 that this is a pretty good indication we
            //  have a CD-RW here and the drive is method 2 goofy.  Take
            //  a shot.
            //

            ReadLsn = UdfMethod2TransformSector( Vcb, ANCHOR_SECTOR );
            Lsn = ANCHOR_SECTOR;

        } else if (ThisPass == 3) {

            //
            //  Our remaining two chances depend on being able to determine
            //  the last recorded sector for the volume.  If we were unable
            //  to do this, stop.
            //

            if (!Vcb->BoundN) {

                break;
            }

            ReadLsn = Lsn = Vcb->BoundN;

        } else if (ThisPass == 4) {

            ReadLsn = Lsn = Vcb->BoundN - ANCHOR_SECTOR;
        }

        //
        //  We may have more chances to succeed if failure occurs.
        //

        Status = UdfReadSectors( IrpContext,
                                 LlBytesFromSectors( Vcb, ReadLsn ),
                                 UdfRawReadSize( Vcb, sizeof(NSR_ANCHOR) ),
                                 TRUE,
                                 *AnchorVolumeDescriptor,
                                 Vcb->TargetDeviceObject );

        if (!NT_SUCCESS( Status )) {
            continue;
        }

        if (!UdfVerifyDescriptor( IrpContext,
                                  &(*AnchorVolumeDescriptor)->Destag,
                                  DESTAG_ID_NSR_ANCHOR,
                                  sizeof(NSR_ANCHOR),
                                  Lsn,
                                  TRUE)) {

            continue;
        }
        
        //
        //  Got one!  Set the method 2 fixup appropriately.
        //

        if (ThisPass == 2) {

            DebugTrace(( 0, Dbg, "************************************************\n"));
            DebugTrace(( 0, Dbg, "METHOD 2 FIXUPS ACTIVATED FOR Vcb @ %08x\n", Vcb ));
            DebugTrace(( 0, Dbg, "************************************************\n"));

            SetFlag( Vcb->VcbState, VCB_STATE_METHOD_2_FIXUP );
        
        } else {
            
            ClearFlag( Vcb->VcbState, VCB_STATE_METHOD_2_FIXUP );
        }
        
        return STATUS_SUCCESS;
    }

    return STATUS_UNRECOGNIZED_VOLUME;
}


//
//  Local support routine
//

BOOLEAN
UdfRecognizeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN OUT PBOOLEAN Bridge
    )

/*++

Routine Description:

    This routine walks the Volume Recognition Sequence to determine
    whether this volume contains an NSR02 (ISO 13346 Section 4) image.

Arguments:

    DeviceObject - device we are checking

    SectorSize - size of a physical sector on this device

    Bridge - will return whether there appear to be ISO 9660 structures
        on the media

Return Value:

    Boolean TRUE if we found NSR02, FALSE otherwise.

--*/

{
    NTSTATUS Status;

    BOOLEAN FoundBEA = FALSE;
    BOOLEAN FoundNSR = FALSE;
    BOOLEAN Resolved = FALSE;

    PVSD_GENERIC VolumeStructureDescriptor;
    ULONGLONG Offset = SectorAlignN( SectorSize, VRA_BOUNDARY_LOCATION );

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_IRP_CONTEXT( IrpContext);

    VolumeStructureDescriptor = (PVSD_GENERIC) FsRtlAllocatePoolWithTag( UdfNonPagedPool,
                                                                         UdfRawBufferSizeN( SectorSize,
                                                                                            sizeof(VSD_GENERIC) ),
                                                                         TAG_NSR_VSD );

    DebugTrace(( +1, Dbg,
                 "UdfRecognizeVolume, DevObj %08x SectorSize %08x\n",
                 DeviceObject,
                 SectorSize ));

    //
    //  Use try-finally to facilitate cleanup
    //

    try {

#ifdef UDF_SUPPORT_NONSTANDARD_ADAPTEC

        //
        //  Disable checking the recognition area.
        //
        //  Reasons:
        //
        //      ADAPTEC - early CDUDF did not bound NSR02 with BEA/TEA, instead
        //              sticking it in the middle of the ISO 9660 sequence.
        //

        Resolved = TRUE;
#endif

        while (!Resolved) {

            Status = UdfReadSectors( IrpContext,
                                     Offset,
                                     UdfRawReadSizeN( SectorSize,
                                                      sizeof(VSD_GENERIC) ),
                                     FALSE,
                                     VolumeStructureDescriptor,
                                     DeviceObject );

            if (!NT_SUCCESS( Status )) {
                break;
            }
            
            //
            //  Now check the type of the descriptor. All ISO 13346 VSDs are
            //  of Type 0, 9660 PVDs are Type 1, 9660 SVDs are Type 2, and 9660
            //  terminating descriptors are Type 255.
            //

            if (VolumeStructureDescriptor->Type == 0) {

                //
                //  In order to properly recognize the volume, we must know all of the
                //  Structure identifiers in ISO 13346 so that we can terminate if a
                //  badly formatted (or, shockingly, non 13346) volume is presented to us.
                //

                switch (UdfFindInParseTable( VsdIdentParseTable,
                                             VolumeStructureDescriptor->Ident,
                                             VSD_LENGTH_IDENT )) {
                    case VsdIdentBEA01:

                        //
                        //  Only one BEA may exist and its version must be 1 (2/9.2.3)
                        //

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got a BEA01\n" ));


                        if ((FoundBEA &&
                             DebugTrace(( 0, Dbg,
                                          "UdfRecognizeVolume, ... but it is a duplicate!\n" ))) ||

                            (VolumeStructureDescriptor->Version != 1 &&
                             DebugTrace(( 0, Dbg,
                                          "UdfRecognizeVolume, ... but it has a wacky version number %02x != 1!\n",
                                          VolumeStructureDescriptor->Version )))) {

                            Resolved = TRUE;
                            break;
                        }

                        FoundBEA = TRUE;
                        break;

                    case VsdIdentTEA01:

                        //
                        //  If we reach the TEA it must be the case that we don't recognize
                        //

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got a TEA01\n" ));
                        Resolved = TRUE;
                        break;

                    case VsdIdentNSR02:

                        //
                        //  We recognize NSR02 version 1 embedded after a BEA (3/9.1.3).  For
                        //  simplicity we will not bother being a complete nitpick and check
                        //  for a bounding TEA, although we will be optimistic in the case where
                        //  we fail to match the version.
                        //

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got an NSR02\n" ));

                        if ((FoundBEA ||
                             !DebugTrace(( 0, Dbg, "UdfRecognizeVolume, ... but we haven't seen a BEA01 yet!\n" ))) &&

                            (VolumeStructureDescriptor->Version == 1 ||
                             !DebugTrace(( 0, Dbg, "UdfRecognizeVolume, ... but it has a wacky version number %02x != 1\n",
                                           VolumeStructureDescriptor->Version )))) {

                            
                            FoundNSR = Resolved = TRUE;
                            break;
                        }

                        break;

                    case VsdIdentCD001:
                    case VsdIdentCDW01:
                    case VsdIdentNSR01:
                    case VsdIdentCDW02:
                    case VsdIdentBOOT2:

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got a valid but uninteresting 13346 descriptor\n" ));

                        //
                        //  Valid but uninteresting (to us) descriptors
                        //

                        break;

                    default:

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got an invalid 13346 descriptor\n" ));

                        //
                        //  Stumbling across something we don't know, it must be that this
                        //  is not a valid 13346 image
                        //

                        Resolved = TRUE;
                        break;

                }

            } else if (!FoundBEA && (VolumeStructureDescriptor->Type < 3 ||
                                     VolumeStructureDescriptor->Type == 255)) {

                DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got a 9660 descriptor\n" ));

                //
                //  Only HSG (CDROM) and 9660 (CD001) are possible, and they are only legal
                //  before the ISO 13346 BEA/TEA extent.  By design, an ISO 13346 VSD precisely
                //  overlaps a 9660 PVD/SVD in the appropriate fields.
                //
                //  Note that we aren't being strict about the structure of the 9660 descriptors
                //  since that really isn't very interesting.  We care more about the 13346.
                //
                //

                switch (UdfFindInParseTable( VsdIdentParseTable,
                                             VolumeStructureDescriptor->Ident,
                                             VSD_LENGTH_IDENT )) {
                    case VsdIdentCDROM:
                    case VsdIdentCD001:

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, ... seems we have 9660 here\n" ));

                        //
                        //  Note to our caller that we seem to have ISO 9660 here
                        //

                        *Bridge = TRUE;

                        break;

                    default:

                        DebugTrace(( 0, Dbg, "UdfRecognizeVolume, ... but it looks wacky\n" ));

                        //
                        //  This probably was a false alert, but in any case there is nothing
                        //  on this volume for us.
                        //

                        Resolved = TRUE;
                        break;
                }

            } else {

                //
                //  Something else must be recorded on this volume.
                //

                DebugTrace(( 0, Dbg, "UdfRecognizeVolume, got an unrecognizeable descriptor, probably not 13346/9660\n" ));
                break;
            }

            //
            //  Align our next read with the sector following the current descriptor
            //

            Offset += SectorAlignN( SectorSize, sizeof(VSD_GENERIC) );
        }

    } finally {

        DebugUnwind( "UdfRecognizeVolume" );

        //
        //  Free up our temporary buffer
        //

        UdfFreePool( &VolumeStructureDescriptor );

        if (AbnormalTermination()) {

            //
            //  Commute a status we raised for empty devices so that other filesystems
            //  can have a crack at this.
            //

            if (UdfIsRawDevice(IrpContext, IrpContext->ExceptionStatus)) {

                IrpContext->ExceptionStatus = STATUS_UNRECOGNIZED_VOLUME;
            }
        }
    }

    DebugTrace(( -1, Dbg, "UdfRecognizeVolume -> %u\n", FoundNSR ));

    return FoundNSR;
}


//
//  Local support routine
//

VOID
UdfScanForDismountedVcb (
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
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    ASSERT_EXCLUSIVE_UDFDATA;

    //
    //  Walk through all of the Vcb's attached to the global data.
    //

    Links = UdfData.VcbQueue.Flink;

    while (Links != &UdfData.VcbQueue) {

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
            ((Vcb->VcbCondition == VcbNotMounted) && (Vcb->VcbReference <= Vcb->VcbResidualReference))) {

            UdfCheckForDismount( IrpContext, Vcb, FALSE );
        }
    }

    return;
}


VOID
UdfDetermineVolumeBounding (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PULONG S,
    IN PULONG N
    )

/*++

Routine Description:

    This routine will figure out where the base offset to discover volume descriptors
    lies and where the end of the disc is.  In the case where this is a non-CD media,
    this will tend to not to set the end bound since there is no uniform way to figure
    that piece of information out.
    
    The bounding information is used to start the hunt for CD-UDF (UDF 1.5) volumes.
    Anyone who puts CD-UDF on non-CD media deserves what they get.

Arguments:

    Vcb - the volume we are operating on
    
    S - an address to store the start of the volume for the purposes of finding descriptors
    
    N - an address to store the end of the volume for the purposes of finding descriptors

Return Value:

    None.
    
    Benign inability find the S/N information will result in 0/0 being returned.

--*/

{
    NTSTATUS Status;
    PCDROM_TOC CdromToc;
    PTRACK_DATA TrackData;

    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    //
    //  Allocate a buffer for the last session information.
    //

    CdromToc = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                         sizeof( CDROM_TOC ),
                                         TAG_CDROM_TOC );

    RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

    DebugTrace(( +1, Dbg,
               "UdfDetermineVolumeBounding, Vcb %08x S %08x N %08x\n",
               Vcb,
               S,
               N ));
        
    //
    //  Whack the inputs to the benign state.
    //
    
    *S = *N = 0;

    //
    //  Try to retrieve the CDROM last session information.
    //

    try {

        //
        //  Pull up the TOC.  The information for track AA (start of leadout)
        //  will get us the end of disc within some tolerance dependent on how
        //  much the device manufacturer paid attention to specifications.
        //  (-152, -150, -2, and 0 are possible offsets to the real end).
        //
        
        Status = UdfPerformDevIoCtrl( IrpContext,
                                      IOCTL_CDROM_READ_TOC,
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

            DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, READ_TOC failed INSUFFICIENT_RESOURCES\n" ));
            UdfRaiseStatus( IrpContext, Status );
        }

        //
        //  For other errors, just fail.  Perhaps this will turn out to be benign, in any case
        //  the mount will rapidly and correctly fail if it really was dependant on this work.
        //
        
        if (!NT_SUCCESS( Status )) {

            try_leave( NOTHING );
        }

        //
        //  Sanity chck that the TOC is well-bounded.
        //
        
        if (CdromToc->LastTrack - CdromToc->FirstTrack >= MAXIMUM_NUMBER_TRACKS) {

            DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, TOC malf (too many tracks)\n" ));
            try_leave( NOTHING );
        }

        TrackData = &CdromToc->TrackData[(CdromToc->LastTrack - CdromToc->FirstTrack + 1)];

#if 0
        //
        //  Better be AA ...
        //
        
        if (TrackData->TrackNumber != 0xaa) {

            DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, TOC malf (aa not last)\n" ));
            try_leave( NOTHING );
        }
#endif

        //
        //  Now, find the AA info and convert MSF to a logical block address.  75 frames/sectors
        //  per second, 60 seconds per minute.  The MSF address is stored LSB (the F byte) high
        //  in the word.
        //

        //
        //  NOTE: MSF is only capable of representing 256*(256+256*60)*75 = 0x11ce20 sectors.
        //  This is 2.3gb, much less than the size of DVD media, which will respond to CDROM_TOC.
        //  Caveat user.
        //

        *N = (TrackData->Address[3] + (TrackData->Address[2] + TrackData->Address[1] * 60) * 75) - 1;

        //
        //  We must bias back by 0/2/0 MSF since that is the defined location of sector 0.  This
        //  works out to 150 sectors.
        //
        
        if (*N <= 150) {

            *N = 0;
            try_leave( NOTHING );
        }

        *N -= 150;

        //
        //  Query the last session information from the driver.
        //

        Status = UdfPerformDevIoCtrl( IrpContext,
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

            DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, GET_LAST_SESSION failed INSUFFICIENT_RESOURCES\n" ));
            UdfRaiseStatus( IrpContext, Status );
        }

        //
        //  Now, if we got anything interesting out of this try, return it.  If this                                                
        //  failed for any other reason, we don't really care - it just means that
        //  if this was CDUDF media, we're gonna fail to figure it out pretty quickly.
        //
        //  Life is tough.
        //

        if (NT_SUCCESS( Status ) &&
            CdromToc->FirstTrack != CdromToc->LastTrack) {

            //
            //  The 0 entry in TrackData tells us about the start of the session as a
            //  logical block address.
            //

            SwapCopyUchar4( S, &CdromToc->TrackData[0].Address );

            //
            //  Save grief if the session info is screwed up.
            //
            
            if (*N <= *S) {

                DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, N before S, whacking both back!\n" ));
                *S = *N = 0;
            }
        }

        DebugTrace(( 0, Dbg, "UdfDetermineVolumeBounding, S %08x N %08x\n", *S, *N ));

    } finally {

        DebugUnwind( "UdfDetermineVolumeBounding" );
        
        if (CdromToc != NULL) {
            
            UdfFreePool( &CdromToc );
        }
    }

    DebugTrace(( -1, Dbg, "UdfDetermineVolumeBounding -> VOID\n" ));

    return;
}


//
//  Local support routine
//

VOID
UdfUpdateVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PWCHAR VolumeLabel,
    IN OUT PUSHORT VolumeLabelLength,
    IN PUCHAR Dstring,
    IN UCHAR FieldLength
    )

/*++

Routine Description:

    This routine will retrieve an NT volume label from a logical volume descriptor.

Arguments:

    VolumeLabel - a volume label to fill in.

    VolumeLabelLength - returns the length of the returned volume label.

    Dstring - the dstring field containing the volume id.

    FieldLength - the length of the dstring field.

Return Value:

    None.

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace(( +1, Dbg,
                 "UdfUpdateVolumeLabel, Label %08x, Dstring %08x FieldLength %02x\n",
                 VolumeLabel,
                 Dstring,
                 FieldLength ));

    //
    //  Check that the dstring is usable as a volume identification.
    //

    Result = UdfCheckLegalCS0Dstring( IrpContext,
                                      Dstring,
                                      0,
                                      FieldLength,
                                      TRUE );


    //
    //  Update the label directly if the dstring is good.
    //

    if (Result) {

        UNICODE_STRING TemporaryUnicodeString;

        TemporaryUnicodeString.Buffer = VolumeLabel;
        TemporaryUnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        TemporaryUnicodeString.Length = 0;

        UdfConvertCS0DstringToUnicode( IrpContext,
                                       Dstring,
                                       0,
                                       FieldLength,
                                       &TemporaryUnicodeString );

        //
        //  Now retrieve the name for return to the caller.
        //

        RtlCopyMemory( VolumeLabel, TemporaryUnicodeString.Buffer, TemporaryUnicodeString.Length );
        *VolumeLabelLength = TemporaryUnicodeString.Length;

        DebugTrace(( 0, Dbg,
                     "UdfUpdateVolumeLabel, Labeled as \"%wZ\"\n",
                     &TemporaryUnicodeString ));

    //
    //  Treat as label.
    //

    } else {

        *VolumeLabelLength = 0;

        DebugTrace(( 0, Dbg,
                     "UdfUpdateVolumeLabel, invalid label.\n" ));
    }

    DebugTrace(( -1, Dbg,
                 "UdfUpdateVolumeLabel -> VOID\n" ));
}


//
//  Local support routine
//

VOID
UdfUpdateVolumeSerialNumber (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PULONG VolumeSerialNumber,
    IN PNSR_FSD Fsd
    )

/*++

Routine Description:

    This routine will compute the volume serial number for a set of descriptors.

Arguments:

    VolumeSerialNumber - returns the volume serial number corresponding to these descriptors.

    Fsd - the fileset descriptor to examine.

Return Value:

    None.

--*/

{
    ULONG VsnLe;
    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  The serial number is just off of the FSD. This matches Win9x.
    //

    VsnLe = UdfSerial32( (PCHAR) Fsd, sizeof( NSR_FSD ));
    SwapCopyUchar4( VolumeSerialNumber, &VsnLe );
}

