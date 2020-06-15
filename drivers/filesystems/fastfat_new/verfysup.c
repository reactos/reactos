/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Fat Verify volume and fcb/dcb support
    routines


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_VERFYSUP)

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_VERFYSUP)

//
//  Local procedure prototypes
//

VOID
FatResetFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

BOOLEAN
FatMatchFileSize (
    __in PIRP_CONTEXT IrpContext,
    __in PDIRENT Dirent,
    __in PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDetermineAndMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

WORKER_THREAD_ROUTINE FatDeferredCleanVolume;

VOID
NTAPI
FatDeferredCleanVolume (
    _In_ PVOID Parameter
    );

IO_COMPLETION_ROUTINE FatMarkVolumeCompletionRoutine;

NTSTATUS
NTAPI
FatMarkVolumeCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCheckDirtyBit)
#pragma alloc_text(PAGE, FatVerifyOperationIsLegal)
#pragma alloc_text(PAGE, FatDeferredCleanVolume)
#pragma alloc_text(PAGE, FatMatchFileSize)
#pragma alloc_text(PAGE, FatDetermineAndMarkFcbCondition)
#pragma alloc_text(PAGE, FatQuickVerifyVcb)
#pragma alloc_text(PAGE, FatPerformVerify)
#pragma alloc_text(PAGE, FatMarkFcbCondition)
#pragma alloc_text(PAGE, FatResetFcb)
#pragma alloc_text(PAGE, FatVerifyVcb)
#pragma alloc_text(PAGE, FatVerifyFcb)
#endif


VOID
FatMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FCB_CONDITION FcbCondition,
    IN BOOLEAN Recursive
    )

/*++

Routine Description:

    This routines marks the entire Fcb/Dcb structure from Fcb down with
    FcbCondition.

Arguments:

    Fcb - Supplies the Fcb/Dcb being marked

    FcbCondition - Supplies the setting to use for the Fcb Condition

    Recursive - Specifies whether this condition should be applied to
        all children (see the case where we are invalidating a live volume
        for a case where this is now desireable).

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatMarkFcbCondition, Fcb = %p\n", Fcb );

    //
    //  If we are marking this Fcb something other than Good, we will need
    //  to have the Vcb exclusive.
    //

    NT_ASSERT( FcbCondition != FcbNeedsToBeVerified ? TRUE :
            FatVcbAcquiredExclusive(IrpContext, Fcb->Vcb) );

    //
    //  If this is a PagingFile it has to be good unless media underneath is
    //  removable. The "removable" check was added specifically for ReadyBoost,
    //  which opens its cache file on a removable device as a paging file and
    //  relies on the file system to validate its mapping information after a
    //  power transition.
    //

    if (!FlagOn(Fcb->Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
        FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

        Fcb->FcbCondition = FcbGood;
        return;
    }

    //
    //  Update the condition of the Fcb.
    //

    Fcb->FcbCondition = FcbCondition;

    DebugTrace(0, Dbg, "MarkFcb: %wZ\n", &Fcb->FullFileName);

    //
    //  This FastIo flag is based on FcbCondition, so update it now.  This only
    //  applies to regular FCBs, of course.
    //

    if (Fcb->Header.NodeTypeCode == FAT_NTC_FCB) {

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );
    }

    if (FcbCondition == FcbNeedsToBeVerified) {
        FatResetFcb( IrpContext, Fcb );
    }

    //
    //  Now if we marked NeedsVerify or Bad a directory then we also need to
    //  go and mark all of our children with the same condition.
    //

    if ( ((FcbCondition == FcbNeedsToBeVerified) ||
          (FcbCondition == FcbBad)) &&
         Recursive &&
         ((Fcb->Header.NodeTypeCode == FAT_NTC_DCB) ||
          (Fcb->Header.NodeTypeCode == FAT_NTC_ROOT_DCB)) ) {

        PFCB OriginalFcb = Fcb;

        while ( (Fcb = FatGetNextFcbTopDown(IrpContext, Fcb, OriginalFcb)) != NULL ) {

            DebugTrace(0, Dbg, "MarkFcb: %wZ\n", &Fcb->FullFileName);

            Fcb->FcbCondition = FcbCondition;

            //
            //  We already know that FastIo is not possible since we are propagating
            //  a parent's bad/verify flag down the tree - IO to the children must
            //  take the long route for now.
            //

            Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

            //
            //  Leave all the Fcbs in a condition to be verified.
            //

            if (FcbCondition == FcbNeedsToBeVerified) {
                FatResetFcb( IrpContext, Fcb );
            }
            
        }
    }

    DebugTrace(-1, Dbg, "FatMarkFcbCondition -> VOID\n", 0);

    return;
}

BOOLEAN
FatMarkDevForVerifyIfVcbMounted(
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

#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable: 28175, "touching Vpb is ok for a filesystem" )
#endif

    if (Vcb->Vpb->RealDevice->Vpb == Vcb->Vpb)  {

        SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
        Marked = TRUE;
    }
    else {

        //
        //  Flag this to avoid the VPB spinlock in future passes.
        //

        SetFlag( Vcb->VcbState, VCB_STATE_VPB_NOT_ON_DEVICE);
    }

#ifdef _MSC_VER
#pragma prefast( pop )
#endif

    IoReleaseVpbSpinLock( SavedIrql );

    return Marked;
}


VOID
FatVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routines verifies that the Vcb still denotes a valid Volume
    If the Vcb is bad it raises an error condition.

Arguments:

    Vcb - Supplies the Vcb being verified

Return Value:

    None.

--*/

{
    BOOLEAN DevMarkedForVerify;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatVerifyVcb, Vcb = %p\n", Vcb );

    //
    //  If the verify volume flag in the device object is set
    //  this means the media has potentially changed.
    //
    //  Note that we only force this ping for create operations.
    //  For others we take a sporting chance.  If in the end we
    //  have to physically access the disk, the right thing will happen.
    //

    DevMarkedForVerify = BooleanFlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);
    
    //
    //  We ALWAYS force CREATE requests on unmounted volumes through the
    //  verify path.  These requests could have been in limbo between
    //  IoCheckMountedVpb and us, when a verify/mount took place and caused
    //  a completely different fs/volume to be mounted.  In this case the
    //  checks above may not have caught the condition, since we may already
    //  have verified (wrong volume) and decided that we have nothing to do.
    //  We want the requests to be re routed to the currently mounted volume,
    //  since they were directed at the 'drive',  not our volume.  So we take
    //  the verify path for synchronisation,  and the request will eventually
    //  be bounced back to IO with STATUS_REPARSE by our verify handler.
    //

    if (!DevMarkedForVerify &&
        (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
        (IrpContext->OriginatingIrp != NULL)) {

        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp);

        if ((IrpSp->FileObject->RelatedFileObject == NULL) &&
            (Vcb->VcbCondition == VcbNotMounted)) {

            DevMarkedForVerify = TRUE;
        }
    }

    //
    //  Raise any error condition otherwise.
    //

    if (DevMarkedForVerify) {

        DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      Vcb->Vpb->RealDevice );

        FatNormalizeAndRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
    }

    //
    //  Check the operation is legal for current Vcb state.
    //

    FatQuickVerifyVcb( IrpContext, Vcb );

    DebugTrace(-1, Dbg, "FatVerifyVcb -> VOID\n", 0);
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routines verifies that the Fcb still denotes the same file.
    If the Fcb is bad it raises a error condition.

Arguments:

    Fcb - Supplies the Fcb being verified

Return Value:

    None.

--*/

{
    PFCB CurrentFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatVerifyFcb, Vcb = %p\n", Fcb );

    //
    //  Always refuse operations on dismounted volumes.
    //

    if (FlagOn( Fcb->Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DISMOUNTED )) {

        FatRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED );
    }

    //
    //  If this is the Fcb of a deleted dirent or our parent is deleted,
    //  no-op this call with the hope that the caller will do the right thing.
    //  The only caller we really have to worry about is the AdvanceOnly
    //  callback for setting valid data length from Cc, this will happen after
    //  cleanup (and file deletion), just before the SCM is ripped down.
    //

    if (IsFileDeleted( IrpContext, Fcb ) ||
        ((NodeType(Fcb) != FAT_NTC_ROOT_DCB) &&
         IsFileDeleted( IrpContext, Fcb->ParentDcb ))) {

        return;
    }

    //
    //  If we are not in the process of doing a verify,
    //  first do a quick spot check on the Vcb.
    //

    if ( Fcb->Vcb->VerifyThread != KeGetCurrentThread() ) {

        FatQuickVerifyVcb( IrpContext, Fcb->Vcb );
    }

    //
    //  Now based on the condition of the Fcb we'll either return
    //  immediately to the caller, raise a condition, or do some work
    //  to verify the Fcb.
    //

    switch (Fcb->FcbCondition) {

    case FcbGood:

        DebugTrace(0, Dbg, "The Fcb is good\n", 0);
        break;

    case FcbBad:

        FatRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        break;

    case FcbNeedsToBeVerified:

        //
        //  We loop here checking our ancestors until we hit an Fcb which
        //  is either good or bad.
        //

        CurrentFcb = Fcb;

        while (CurrentFcb->FcbCondition == FcbNeedsToBeVerified) {

            FatDetermineAndMarkFcbCondition(IrpContext, CurrentFcb);

            //
            //  If this Fcb didn't make it, or it was the Root Dcb, exit
            //  the loop now, else continue with out parent.
            //

            if ( (CurrentFcb->FcbCondition != FcbGood) ||
                 (NodeType(CurrentFcb) == FAT_NTC_ROOT_DCB) ) {

                break;
            }

            CurrentFcb = CurrentFcb->ParentDcb;
        }

        //
        //  Now we can just look at ourselves to see how we did.
        //

        if (Fcb->FcbCondition != FcbGood) {

            FatRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        }

        break;

    default:

        DebugDump("Invalid FcbCondition\n", 0, Fcb);
        
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )     
#endif   
        FatBugCheck( Fcb->FcbCondition, 0, 0 );
    }

    DebugTrace(-1, Dbg, "FatVerifyFcb -> VOID\n", 0);

    return;
}


VOID
NTAPI
FatDeferredCleanVolume (
    _In_ PVOID Parameter
    )

/*++

Routine Description:

    This is the routine that performs the actual FatMarkVolumeClean call.
    It assures that the target volume still exists as there ia a race
    condition between queueing the ExWorker item and volumes going away.

Arguments:

    Parameter - Points to a clean volume packet that was allocated from pool

Return Value:

    None.

--*/

{
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;
    PLIST_ENTRY Links;
    PVCB Vcb;
    IRP_CONTEXT IrpContext;
    BOOLEAN VcbExists = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeferredCleanVolume\n", 0);

    Packet = (PCLEAN_AND_DIRTY_VOLUME_PACKET)Parameter;

    Vcb = Packet->Vcb;

    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the operation.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    //
    //  Dummy up and Irp Context so we can call our worker routines
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT));

    SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  Acquire shared access to the global lock and make sure this volume
    //  still exists.
    //

#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable: 28193, "this will always wait" )     
#endif   
    FatAcquireSharedGlobal( &IrpContext );
#ifdef _MSC_VER
#pragma prefast( pop )
#endif

    for (Links = FatData.VcbQueue.Flink;
         Links != &FatData.VcbQueue;
         Links = Links->Flink) {

        PVCB ExistingVcb;

        ExistingVcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

        if ( Vcb == ExistingVcb ) {

            VcbExists = TRUE;
            break;
        }
    }

    //
    //  If the vcb is good then mark it clean.  Ignore any problems.
    //

    if ( VcbExists &&
         (Vcb->VcbCondition == VcbGood) &&
         !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ) {

        _SEH2_TRY {

            if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                FatMarkVolume( &IrpContext, Vcb, VolumeClean );
            }

            //
            //  Check for a pathological race condition, and fix it.
            //

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY)) {

                FatMarkVolume( &IrpContext, Vcb, VolumeDirty );

            } else {

                //
                //  Unlock the volume if it is removable.
                //

                if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
                    !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE)) {

                    FatToggleMediaEjectDisable( &IrpContext, Vcb, FALSE );
                }
            }

        } _SEH2_EXCEPT( FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

              NOTHING;
        } _SEH2_END;
    }

    //
    //  Release the global resource, unpin and repinned Bcbs and return.
    //

    FatReleaseGlobal( &IrpContext );

    _SEH2_TRY {

        FatUnpinRepinnedBcbs( &IrpContext );

    } _SEH2_EXCEPT( FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

          NOTHING;
    } _SEH2_END;

    IoSetTopLevelIrp( NULL );

    //
    //  and finally free the packet.
    //

    ExFreePool( Packet );

    return;
}



VOID
NTAPI
FatCleanVolumeDpc (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched 5 seconds after the last disk structure was
    modified in a specific volume, and exqueues an execuative worker thread
    to perform the actual task of marking the volume dirty.

Arguments:

    DefferedContext - Contains the Vcb to process.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;

    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );
    UNREFERENCED_PARAMETER( Dpc );

    Vcb = (PVCB)DeferredContext;


    //
    //  If there is still dirty data (highly unlikely), set the timer for a
    //  second in the future.
    //

    if (CcIsThereDirtyData(Vcb->Vpb)) {

        LARGE_INTEGER TwoSecondsFromNow;

        TwoSecondsFromNow.QuadPart = (LONG)-2*1000*1000*10;

        KeSetTimer( &Vcb->CleanVolumeTimer,
                    TwoSecondsFromNow,
                    &Vcb->CleanVolumeDpc );

        return;
    }

    //
    //  If we couldn't get pool, oh well....
    //

    Packet = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(CLEAN_AND_DIRTY_VOLUME_PACKET), ' taF');

    if ( Packet ) {

        Packet->Vcb = Vcb;
        Packet->Irp = NULL;

        //
        //  Clear the dirty flag now since we cannot synchronize after this point.
        //

        ClearFlag( Packet->Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );

        ExInitializeWorkItem( &Packet->Item, &FatDeferredCleanVolume, Packet );
        
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "prefast indicates this is an obsolete API, but it is ok for fastfat to keep using it" )
#endif
        ExQueueWorkItem( &Packet->Item, CriticalWorkQueue );
    }

    return;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatMarkVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FAT_VOLUME_STATE VolumeState
    )

/*++

Routine Description:

    This routine moves the physically marked volume state between the clean
    and dirty states.  For compatibility with Win9x, we manipulate both the
    historical DOS (on==clean in index 1 of the FAT) and NT (on==dirty in
    the CurrentHead field of the BPB) dirty bits.

Arguments:

    Vcb - Supplies the Vcb being modified

    VolumeState - Supplies the state the volume is transitioning to

Return Value:

    None.

--*/

{
    PCHAR Sector;
    PBCB Bcb = NULL;
    KEVENT Event;
    PIRP Irp = NULL;
    NTSTATUS Status;
    BOOLEAN FsInfoUpdate = FALSE;
    ULONG FsInfoOffset = 0;
    ULONG ThisPass;
    LARGE_INTEGER Offset;
    BOOLEAN abort = FALSE;

    DebugTrace(+1, Dbg, "FatMarkVolume, Vcb = %p\n", Vcb);

    //
    //  We had best not be trying to scribble dirty/clean bits if the
    //  volume is write protected.  The responsibility lies with the
    //  callers to make sure that operations that could cause a state
    //  change cannot happen.  There are a few, though, that show it
    //  just doesn't make sense to force everyone to do the dinky
    //  check.
    //

    //
    //  If we were called for FAT12 or readonly media, return immediately.
    //

    if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED) ||
        FatIsFat12( Vcb )) {

        return;
    }

    //
    //  We have two possible additional tasks to do to mark a volume
    //
    //      Pass 0) Flip the dirty bit in the Bpb
    //      Pass 1) Rewrite the FsInfo sector for FAT32 if needed
    //
    //  In most cases we can collapse these two either because the volume
    //  is either not FAT32 or the FsInfo sector is adjacent to the boot sector.
    //

    for (ThisPass = 0; ThisPass < 2; ThisPass++) {

        //
        //  If this volume is being dirtied, or isn't FAT32, or if it is and
        //  we were able to perform the fast update, or the bpb lied to us
        //  about where the FsInfo went, we're done - no FsInfo to update in
        //  a seperate write.
        //

        if (ThisPass == 1 && (!FatIsFat32( Vcb ) ||
                              VolumeState != VolumeClean ||
                              FsInfoUpdate ||
                              Vcb->Bpb.FsInfoSector == 0)) {

            break;
        }

        //
        //  Bail if we get an IO error.
        //

        _SEH2_TRY {

            ULONG PinLength;
            ULONG WriteLength;

            //
            // If the FAT table is 12-bit then our strategy is to pin the entire
            // thing when any of it is modified.  Here we're going to pin the
            // first page, so in the 12-bit case we also want to pin the rest
            // of the FAT table.
            //

            Offset.QuadPart = 0;

            if (Vcb->AllocationSupport.FatIndexBitSize == 12) {

                //
                //  But we only write back the first sector.
                //

                PinLength = FatReservedBytes(&Vcb->Bpb) + FatBytesPerFat(&Vcb->Bpb);
                WriteLength = Vcb->Bpb.BytesPerSector;

            } else {

                WriteLength = PinLength = Vcb->Bpb.BytesPerSector;

                //
                //  If this is a FAT32 volume going into the clean state,
                //  see about doing the FsInfo sector.
                //

                if (FatIsFat32( Vcb ) && VolumeState == VolumeClean) {

                    //
                    //  If the FsInfo sector immediately follows the boot sector,
                    //  we can do this in a single operation by rewriting both
                    //  sectors at once.
                    //

                    if (Vcb->Bpb.FsInfoSector == 1) {

                        NT_ASSERT( ThisPass == 0 );

                        FsInfoUpdate = TRUE;
                        FsInfoOffset = Vcb->Bpb.BytesPerSector;
                        WriteLength = PinLength = Vcb->Bpb.BytesPerSector * 2;

                    } else if (ThisPass == 1) {

                        //
                        //  We are doing an explicit write to the FsInfo sector.
                        //

                        FsInfoUpdate = TRUE;
                        FsInfoOffset = 0;

                        Offset.QuadPart = Vcb->Bpb.BytesPerSector * Vcb->Bpb.FsInfoSector;
                    }
                }
            }

            //
            //  Call Cc directly here so that we can avoid overhead and push this
            //  right down to the disk.
            //

            CcPinRead( Vcb->VirtualVolumeFile,
                       &Offset,
                       PinLength,
                       TRUE,
                       &Bcb,
                       (PVOID *)&Sector );

            DbgDoit( IrpContext->PinCount += 1 )

            //
            //  Set the Bpb on Pass 0 always
            //

            if (ThisPass == 0) {

                PCHAR CurrentHead;

                //
                //  Before we do anything, doublecheck that this still looks like a
                //  FAT bootsector.  If it doesn't, something remarkable happened
                //  and we should avoid touching the volume.
                //
                //  THIS IS TEMPORARY (but may last a while)
                //

                if (!FatIsBootSectorFat( (PPACKED_BOOT_SECTOR) Sector )) {
                    abort = TRUE;
                    _SEH2_LEAVE;
                }

                if (FatIsFat32( Vcb )) {

                    CurrentHead = (PCHAR)&((PPACKED_BOOT_SECTOR_EX) Sector)->CurrentHead;

                } else {

                    CurrentHead = (PCHAR)&((PPACKED_BOOT_SECTOR) Sector)->CurrentHead;
                }

                if (VolumeState == VolumeClean) {

                    ClearFlag( *CurrentHead, FAT_BOOT_SECTOR_DIRTY );

                } else {

                    SetFlag( *CurrentHead, FAT_BOOT_SECTOR_DIRTY );

                    //
                    //  In addition, if this request received an error that may indicate
                    //  media corruption, have autochk perform a surface test.
                    //

                    if ( VolumeState == VolumeDirtyWithSurfaceTest ) {

                        SetFlag( *CurrentHead, FAT_BOOT_SECTOR_TEST_SURFACE );
                    }
                }
            }

            //
            //  Update the FsInfo as appropriate.
            //

            if (FsInfoUpdate) {

                PFSINFO_SECTOR FsInfoSector = (PFSINFO_SECTOR) ((PCHAR)Sector + FsInfoOffset);

                //
                //  We just rewrite all of the spec'd fields.  Note that we don't
                //  care to synchronize with the allocation package - this will be
                //  quickly taken care of by a re-dirtying of the volume if a change
                //  is racing with us.  Remember that this is all a compatibility
                //  deference for Win9x FAT32 - NT will never look at this information.
                //

                FsInfoSector->SectorBeginSignature = FSINFO_SECTOR_BEGIN_SIGNATURE;
                FsInfoSector->FsInfoSignature = FSINFO_SIGNATURE;
                FsInfoSector->FreeClusterCount = Vcb->AllocationSupport.NumberOfFreeClusters;
                FsInfoSector->NextFreeCluster = Vcb->ClusterHint;
                FsInfoSector->SectorEndSignature = FSINFO_SECTOR_END_SIGNATURE;
            }

            //
            //  Initialize the event we're going to use
            //

            KeInitializeEvent( &Event, NotificationEvent, FALSE );

            //
            //  Build the irp for the operation and also set the override flag.
            //  Note that we may be at APC level, so do this asyncrhonously and
            //  use an event for synchronization as normal request completion
            //  cannot occur at APC level.
            //

            Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_WRITE,
                                                 Vcb->TargetDeviceObject,
                                                 (PVOID)Sector,
                                                 WriteLength,
                                                 &Offset,
                                                 NULL );

            if ( Irp == NULL ) {

                try_return(NOTHING);
            }

            //
            //  Make this operation write-through.  It never hurts to try to be
            //  safer about this, even though we aren't logged.
            //

            SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_WRITE_THROUGH );

            //
            //  Set up the completion routine
            //

            IoSetCompletionRoutine( Irp,
                                    FatMarkVolumeCompletionRoutine,
                                    &Event,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Call the device to do the write and wait for it to finish.
            //  Igmore any return status.
            //

            Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );

            if (Status == STATUS_PENDING) {

                (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
            }

        try_exit: NOTHING;
        } _SEH2_FINALLY {

            //
            //  Clean up the Irp and Mdl
            //


            if (Irp) {

                //
                //  If there is an MDL (or MDLs) associated with this I/O
                //  request, Free it (them) here.  This is accomplished by
                //  walking the MDL list hanging off of the IRP and deallocating
                //  each MDL encountered.
                //

                while (Irp->MdlAddress != NULL) {

                    PMDL NextMdl;

                    NextMdl = Irp->MdlAddress->Next;

                    MmUnlockPages( Irp->MdlAddress );

                    IoFreeMdl( Irp->MdlAddress );

                    Irp->MdlAddress = NextMdl;
                }

                IoFreeIrp( Irp );
            }

            if (Bcb != NULL) {

                FatUnpinBcb( IrpContext, Bcb );
            }
        } _SEH2_END;
    }

    if (!abort) {

        //
        //  Flip the dirty bit in the FAT
        //

        if (VolumeState == VolumeDirty) {

           FatSetFatEntry( IrpContext, Vcb, FAT_DIRTY_BIT_INDEX, FAT_DIRTY_VOLUME);

        } else {

           FatSetFatEntry( IrpContext, Vcb, FAT_DIRTY_BIT_INDEX, FAT_CLEAN_VOLUME);
        }
    }

    DebugTrace(-1, Dbg, "FatMarkVolume -> VOID\n", 0);

    return;
}


VOID
NTAPI
FatFspMarkVolumeDirtyWithRecover(
    PVOID Parameter
    )

/*++

Routine Description:

    This is the routine that performs the actual FatMarkVolume Dirty call
    on a paging file Io that encounters a media error.  It is responsible
    for completing the PagingIo Irp as soon as this is done.

    Note:  this routine (and thus FatMarkVolume()) must be resident as
           the paging file might be damaged at this point.

Arguments:

    Parameter - Points to a dirty volume packet that was allocated from pool

Return Value:

    None.

--*/

{
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;
    PVCB Vcb;
    IRP_CONTEXT IrpContext;
    PIRP Irp;

    DebugTrace(+1, Dbg, "FatFspMarkVolumeDirtyWithRecover\n", 0);

    Packet = (PCLEAN_AND_DIRTY_VOLUME_PACKET)Parameter;

    Vcb = Packet->Vcb;
    Irp = Packet->Irp;

    //
    //  Dummy up the IrpContext so we can call our worker routines
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT));

    SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    IrpContext.OriginatingIrp = Irp;

    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the operation.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    //
    //  Try to write out the dirty bit.  If something goes wrong, we
    //  tried.
    //

    _SEH2_TRY {

        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

        FatMarkVolume( &IrpContext, Vcb, VolumeDirtyWithSurfaceTest );

    } _SEH2_EXCEPT(FatExceptionFilter( &IrpContext, _SEH2_GetExceptionInformation() )) {

        NOTHING;
    } _SEH2_END;

    IoSetTopLevelIrp( NULL );

    //
    //  Now complete the originating Irp or set the synchronous event.
    //

    if (Packet->Event) {
        KeSetEvent( Packet->Event, 0, FALSE );
    } else {
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }

    DebugTrace(-1, Dbg, "FatFspMarkVolumeDirtyWithRecover -> VOID\n", 0);
}


VOID
FatCheckDirtyBit (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine looks at the volume dirty bit, and depending on the state of
    VCB_STATE_FLAG_MOUNTED_DIRTY, the appropriate action is taken.

Arguments:

    Vcb - Supplies the Vcb being queried.

Return Value:

    None.

--*/

{
    BOOLEAN Dirty;

    PPACKED_BOOT_SECTOR BootSector;
    PBCB BootSectorBcb;

    UNICODE_STRING VolumeLabel;

    PAGED_CODE();

    //
    //  Look in the boot sector
    //

    FatReadVolumeFile( IrpContext,
                       Vcb,
                       0,
                       sizeof(PACKED_BOOT_SECTOR),
                       &BootSectorBcb,
                       (PVOID *)&BootSector );

    _SEH2_TRY {

        //
        //  Check if the magic bit is set
        //

        if (IsBpbFat32(&BootSector->PackedBpb)) {
            Dirty = BooleanFlagOn( ((PPACKED_BOOT_SECTOR_EX)BootSector)->CurrentHead,
                                   FAT_BOOT_SECTOR_DIRTY );
        } else {
            Dirty = BooleanFlagOn( BootSector->CurrentHead, FAT_BOOT_SECTOR_DIRTY );
        }

        //
        //  Setup the VolumeLabel string
        //

        VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
        VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

        if ( Dirty ) {

            //
            //  Do not trigger the mounted dirty bit if this is a verify
            //  and the volume is a boot or paging device.  We know that
            //  a boot or paging device cannot leave the system, and thus
            //  that on its mount we will have figured this out correctly.
            //
            //  This logic is a reasonable change.  Why?
            //  'cause setup cracked a non-exclusive DASD handle near the
            //  end of setup, wrote some data, closed the handle and we
            //  set the verify bit ... came back around and saw that other
            //  arbitrary activity had left the volume in a temporarily dirty
            //  state.
            //
            //  Of course, the real problem is that we don't have a journal.
            //

            if (!(IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                  IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME &&
                  FlagOn( Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE))) {

                KdPrintEx((DPFLTR_FASTFAT_ID,
                           DPFLTR_INFO_LEVEL,
                           "FASTFAT: WARNING! Mounting Dirty Volume %Z\n",
                           &VolumeLabel));

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );
            }

        } else {

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                KdPrintEx((DPFLTR_FASTFAT_ID,
                           DPFLTR_INFO_LEVEL,
                           "FASTFAT: Volume %Z has been cleaned.\n",
                           &VolumeLabel));

                ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

            } else {

                (VOID)FsRtlBalanceReads( Vcb->TargetDeviceObject );
            }
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, BootSectorBcb );
    } _SEH2_END;
}


VOID
FatVerifyOperationIsLegal (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine determines is the requested operation should be allowed to
    continue.  It either returns to the user if the request is Okay, or
    raises an appropriate status.

Arguments:

    Irp - Supplies the Irp to check

Return Value:

    None.

--*/

{
    PIRP Irp;
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    Irp = IrpContext->OriginatingIrp;

    //
    //  If the Irp is not present, then we got here via close.
    //
    //

    if ( Irp == NULL ) {

        return;
    }

    FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    //
    //  If there is not a file object, we cannot continue.
    //

    if ( FileObject == NULL ) {

        return;
    }

    //
    //  If the file object has already been cleaned up, and
    //
    //  A) This request is a paging io read or write, or
    //  B) This request is a close operation, or
    //  C) This request is a set or query info call (for Lou)
    //  D) This is an MDL complete
    //
    //  let it pass, otherwise return STATUS_FILE_CLOSED.
    //

    if ( FlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE) ) {

        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

        if ( (FlagOn(Irp->Flags, IRP_PAGING_IO)) ||
             (IrpSp->MajorFunction == IRP_MJ_CLOSE ) ||
             (IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
             ( ( (IrpSp->MajorFunction == IRP_MJ_READ) ||
                 (IrpSp->MajorFunction == IRP_MJ_WRITE) ) &&
               FlagOn(IrpSp->MinorFunction, IRP_MN_COMPLETE) ) ) {

            NOTHING;

        } else {

            FatRaiseStatus( IrpContext, STATUS_FILE_CLOSED );
        }
    }

    return;
}



//
//  Internal support routine
//

VOID
FatResetFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called when an Fcb has been marked as needs to be verified.

    It does the following tasks:

        - Reset Mcb mapping information
        - For directories, reset dirent hints
        - Set allocation size to unknown

Arguments:

    Fcb - Supplies the Fcb to reset

Return Value:

    None.

--*/

{
    LOGICAL IsRealPagingFile;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );
    
    //
    //  Don't do the two following operations for the Root Dcb
    //  of a non FAT32 volume or paging files.  Paging files!?
    //  Yes, if someone diddles a volume we try to reverify all
    //  of the Fcbs just in case; however, there is no safe way
    //  to chuck and retrieve the mapping pair information for
    //  a real paging file. Lose it and die.
    //
    //  An exception is made for ReadyBoost cache files, which
    //  are created as paging files on removable devices and
    //  require validation after a power transition.
    //

    if (!FlagOn(Fcb->Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
        FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

        IsRealPagingFile = TRUE;

    } else {

        IsRealPagingFile = FALSE;
    }

    if ( (NodeType(Fcb) != FAT_NTC_ROOT_DCB ||
          FatIsFat32( Fcb->Vcb ))  &&
         !IsRealPagingFile ) {

        //
        //  Reset the mcb mapping.
        //

        FsRtlRemoveLargeMcbEntry( &Fcb->Mcb, 0, 0xFFFFFFFF );

        //
        //  Reset the allocation size to 0 or unknown
        //

        if ( Fcb->FirstClusterOfFile == 0 ) {

            Fcb->Header.AllocationSize.QuadPart = 0;

        } else {

            Fcb->Header.AllocationSize.QuadPart = FCB_LOOKUP_ALLOCATIONSIZE_HINT;
        }
    }

    //
    //  If this is a directory, reset the hints.
    //

    if ( (NodeType(Fcb) == FAT_NTC_DCB) ||
         (NodeType(Fcb) == FAT_NTC_ROOT_DCB) ) {

        //
        //  Force a rescan of the directory
        //

        Fcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Fcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;
    }
}



BOOLEAN
FatMatchFileSize (
    __in PIRP_CONTEXT IrpContext,
    __in PDIRENT Dirent,
    __in PFCB Fcb
    )
{

    UNREFERENCED_PARAMETER(IrpContext);

    if (NodeType(Fcb) != FAT_NTC_FCB) {
        return TRUE;
    }


        if (Fcb->Header.FileSize.LowPart != Dirent->FileSize) {
            return FALSE;
        }


    return TRUE;
}

//
//  Internal support routine
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDetermineAndMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine checks a specific Fcb to see if it is different from what's
    on the disk.  The following things are checked:

        - File Name
        - File Size (if not directory)
        - First Cluster Of File
        - Dirent Attributes

Arguments:

    Fcb - Supplies the Fcb to examine

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;
    ULONG FirstClusterOfFile;

    OEM_STRING Name;
    CHAR Buffer[16];

    PAGED_CODE();

    //
    //  If this is the Root Dcb, special case it.  That is, we know
    //  by definition that it is good since it is fixed in the volume
    //  structure.
    //

    if ( NodeType(Fcb) == FAT_NTC_ROOT_DCB ) {

        FatMarkFcbCondition( IrpContext, Fcb, FcbGood, FALSE );

        return;
    }

    //  The first thing we need to do to verify ourselves is
    //  locate the dirent on the disk.
    //

    FatGetDirentFromFcbOrDcb( IrpContext,
                              Fcb,
                              TRUE,
                              &Dirent,
                              &DirentBcb );
    //
    //  If we couldn't get the dirent, this fcb must be bad (case of
    //  enclosing directory shrinking during the time it was ejected).
    //

    if (DirentBcb == NULL) {

        FatMarkFcbCondition( IrpContext, Fcb, FcbBad, FALSE );
        
        return;
    }

    //
    //  We located the dirent for ourselves now make sure it
    //  is really ours by comparing the Name and FatFlags.
    //  Then for a file we also check the file size.
    //
    //  Note that we have to unpin the Bcb before calling FatResetFcb
    //  in order to avoid a deadlock in CcUninitializeCacheMap.
    //

    _SEH2_TRY {

        Name.MaximumLength = 16;
        Name.Buffer = &Buffer[0];

        Fat8dot3ToString( IrpContext, Dirent, FALSE, &Name );

        //
        //  We need to calculate the first cluster 'cause FAT32 splits
        //  this field across the dirent.
        //

        FirstClusterOfFile = Dirent->FirstClusterOfFile;

        if (FatIsFat32( Fcb->Vcb )) {

            FirstClusterOfFile += Dirent->FirstClusterOfFileHi << 16;
        }

        if (!RtlEqualString( &Name, &Fcb->ShortName.Name.Oem, TRUE )

                ||

             !FatMatchFileSize(IrpContext, Dirent, Fcb )
                
                ||

             (FirstClusterOfFile != Fcb->FirstClusterOfFile)

                ||

              (Dirent->Attributes != Fcb->DirentFatFlags) ) {

            FatMarkFcbCondition( IrpContext, Fcb, FcbBad, FALSE );

        } else {

            //
            //  We passed.  Get the Fcb ready to use again.
            //

            FatMarkFcbCondition( IrpContext, Fcb, FcbGood, FALSE );
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, DirentBcb );
    } _SEH2_END;

    return;
}



//
//  Internal support routine
//

VOID
FatQuickVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routines just checks the verify bit in the real device and the
    Vcb condition and raises an appropriate exception if so warented.
    It is called when verifying both Fcbs and Vcbs.

Arguments:

    Vcb - Supplies the Vcb to check the condition of.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  If the real device needs to be verified we'll set the
    //  DeviceToVerify to be our real device and raise VerifyRequired.
    //

    if (FlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

        DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      Vcb->Vpb->RealDevice );

        FatRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
    }

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Vcb->VcbCondition) {

    case VcbGood:

        DebugTrace(0, Dbg, "The Vcb is good\n", 0);

        //
        //  Do a check here of an operation that would try to modify a
        //  write protected media.
        //

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED) &&
            ((IrpContext->MajorFunction == IRP_MJ_WRITE) ||
             (IrpContext->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (IrpContext->MajorFunction == IRP_MJ_SET_EA) ||
             (IrpContext->MajorFunction == IRP_MJ_FLUSH_BUFFERS) ||
             (IrpContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION) ||
             (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
              IrpContext->MinorFunction == IRP_MN_USER_FS_REQUEST &&
              IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->Parameters.FileSystemControl.FsControlCode ==
                FSCTL_MARK_VOLUME_DIRTY))) {

            //
            //  Set the real device for the pop-up info, and set the verify
            //  bit in the device object, so that we will force a verify
            //  in case the user put the correct media back in.
            //


            IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                          Vcb->Vpb->RealDevice );

            FatMarkDevForVerifyIfVcbMounted(Vcb);

            FatRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED );
        }

        break;

    case VcbNotMounted:

        DebugTrace(0, Dbg, "The Vcb is not mounted\n", 0);

        //
        //  Set the real device for the pop-up info, and set the verify
        //  bit in the device object, so that we will force a verify
        //  in case the user put the correct media back in.
        //

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      Vcb->Vpb->RealDevice );

        FatRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );

        break;

    case VcbBad:

        DebugTrace(0, Dbg, "The Vcb is bad\n", 0);

        if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DISMOUNTED )) {

            FatRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED );

        } else {

            FatRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        }
        break;

    default:

        DebugDump("Invalid VcbCondition\n", 0, Vcb);
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )        
#endif
        FatBugCheck( Vcb->VcbCondition, 0, 0 );
    }
}

_Requires_lock_held_(_Global_critical_region_)    
NTSTATUS
FatPerformVerify (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp,
    _In_ PDEVICE_OBJECT Device
    )

/*++

Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  After the Verify is complete the originating
    Irp is sent off to an Ex Worker Thread.  This routine is called
    from the exception handler.

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
    PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
    BOOLEAN AllowRawMount = FALSE;
    BOOLEAN VcbDeleted = FALSE;

    PAGED_CODE();

    //
    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    //

    if ( (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
         ((IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
          (IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME)) ) {

        return FatFsdPostRequest( IrpContext, Irp );
    }

    DebugTrace(0, Dbg, "Verify Required, DeviceObject = %p\n", Device);

    //
    //  Extract a pointer to the Vcb from the VolumeDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Vcb = &CONTAINING_RECORD( IrpSp->DeviceObject,
                              VOLUME_DEVICE_OBJECT,
                              DeviceObject )->Vcb;

    //
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    //

    _SEH2_TRY {

        //
        //  We will allow Raw to mount this volume if we were doing a
        //  a DASD open.
        //

        if ( (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
             (IrpSp->FileObject->FileName.Length == 0) &&
             (IrpSp->FileObject->RelatedFileObject == NULL) ) {

            AllowRawMount = TRUE;
        }

        //
        //  Send down the verify.  This could be going to a different
        //  filesystem.
        //

        Status = IoVerifyVolume( Device, AllowRawMount );

        //
        //  If the verify operation completed it will return
        //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
        //
        //  If FatVerifyVolume encountered an error during
        //  processing, it will return that error.  If we got
        //  STATUS_WRONG_VOLUME from the verfy, and our volume
        //  is now mounted, commute the status to STATUS_SUCCESS.
        //
        //  Acquire the Vcb so we're working with a stable Vcb condition.
        //

        FatAcquireSharedVcb(IrpContext, Vcb);

        if ( (Status == STATUS_WRONG_VOLUME) &&
             (Vcb->VcbCondition == VcbGood) ) {

            Status = STATUS_SUCCESS;
        }
        else if ((STATUS_SUCCESS == Status) && (Vcb->VcbCondition != VcbGood)) {

            Status = STATUS_WRONG_VOLUME;
        }

        //
        //  Do a quick unprotected check here.  The routine will do
        //  a safe check.  After here we can release the resource.
        //  Note that if the volume really went away, we will be taking
        //  the Reparse path.
        //

        if ((VcbGood != Vcb->VcbCondition) &&
            (0 == Vcb->OpenFileCount) ) {

            FatReleaseVcb( IrpContext, Vcb);

#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable: 28137, "prefast wants the wait to be a constant, but that isn't possible for the way fastfat is designed" )            
#pragma prefast( disable: 28193 )
#endif
            FatAcquireExclusiveGlobal( IrpContext );
#ifdef _MSC_VER
#pragma prefast( pop )
#endif

            FatAcquireExclusiveVcb( IrpContext,
                                    Vcb );

            VcbDeleted = FatCheckForDismount( IrpContext,
                                              Vcb,
                                              FALSE );

            if (!VcbDeleted) {

                FatReleaseVcb( IrpContext,
                               Vcb );
            }

            FatReleaseGlobal( IrpContext );
        }
        else {

            FatReleaseVcb( IrpContext, Vcb);
        }

        //
        //  If the IopMount in IoVerifyVolume did something, and
        //  this is an absolute open, force a reparse.
        //

        if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
            (FileObject->RelatedFileObject == NULL) &&
            ((Status == STATUS_SUCCESS) || (Status == STATUS_WRONG_VOLUME))) {

            Irp->IoStatus.Information = IO_REMOUNT;

            FatCompleteRequest( IrpContext, Irp, STATUS_REPARSE );
            Status = STATUS_REPARSE;
            Irp = NULL;
        }

        if ( (Irp != NULL) && !NT_SUCCESS(Status) ) {

            //
            //  Fill in the device object if required.
            //

            if ( IoIsErrorUserInduced( Status ) ) {

                IoSetHardErrorOrVerifyDevice( Irp, Device );
            }

            NT_ASSERT( STATUS_VERIFY_REQUIRED != Status);

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if ( Irp != NULL ) {

            Status = FatFsdPostRequest( IrpContext, Irp );
        }

    }
    _SEH2_EXCEPT (FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

    return Status;
}

//
//  Local support routine
//

NTSTATUS
NTAPI
FatMarkVolumeCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Contxt
    )

{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Contxt, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


