/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Fat in-memory data structure manipulation
    routines


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#define FillMemory(BUF,SIZ,MASK) {                          \
    ULONG i;                                                \
    for (i = 0; i < (((SIZ)/4) - 1); i += 2) {              \
        ((PULONG)(BUF))[i] = (MASK);                        \
        ((PULONG)(BUF))[i+1] = (ULONG)PsGetCurrentThread(); \
    }                                                       \
}

#define IRP_CONTEXT_HEADER (sizeof( IRP_CONTEXT ) * 0x10000 + FAT_NTC_IRP_CONTEXT)

//
//  Local macros.
//
//  Define our lookaside list allocators.  For the time being, and perhaps
//  permanently, the paged structures don't come off of lookasides.  This
//  is due to complications with clean unload as FAT can be in the paging
//  path, making it really hard to find the right time to empty them.
//
//  Fortunately, the hit rates on the Fcb/Ccb lists weren't stunning.
//

#define FAT_FILL_FREE 0

INLINE
PCCB
FatAllocateCcb (
    )
{
    return (PCCB) FsRtlAllocatePoolWithTag( PagedPool, sizeof(CCB), TAG_CCB );
}

INLINE
VOID
FatFreeCcb (
    IN PCCB Ccb
    )
{
#if FAT_FILL_FREE
    RtlFillMemoryUlong(Ccb, sizeof(CCB), FAT_FILL_FREE);
#endif

    ExFreePool( Ccb );
}

INLINE
PFCB
FatAllocateFcb (
    )
{
    return (PFCB) FsRtlAllocatePoolWithTag( PagedPool, sizeof(FCB), TAG_FCB );
}

INLINE
VOID
FatFreeFcb (
    IN PFCB Fcb
    )
{
#if FAT_FILL_FREE
    RtlFillMemoryUlong(Fcb, sizeof(FCB), FAT_FILL_FREE);
#endif

    ExFreePool( Fcb );
}

#ifdef __REACTOS__
static
#endif
INLINE
PNON_PAGED_FCB
FatAllocateNonPagedFcb (
    )
{
    return (PNON_PAGED_FCB) ExAllocateFromNPagedLookasideList( &FatNonPagedFcbLookasideList );
}

#ifdef __REACTOS__
static
#endif
INLINE
VOID
FatFreeNonPagedFcb (
    PNON_PAGED_FCB NonPagedFcb
    )
{
#if FAT_FILL_FREE
    RtlFillMemoryUlong(NonPagedFcb, sizeof(NON_PAGED_FCB), FAT_FILL_FREE);
#endif

    ExFreeToNPagedLookasideList( &FatNonPagedFcbLookasideList, (PVOID) NonPagedFcb );
}

#ifdef __REACTOS__
static
#endif
INLINE
PERESOURCE
FatAllocateResource (
    )
{
    PERESOURCE Resource;

    Resource = (PERESOURCE) ExAllocateFromNPagedLookasideList( &FatEResourceLookasideList );

    ExInitializeResourceLite( Resource );

    return Resource;
}

#ifdef __REACTOS__
static
#endif
INLINE
VOID
FatFreeResource (
    IN PERESOURCE Resource
    )
{
    ExDeleteResourceLite( Resource );

#if FAT_FILL_FREE
    RtlFillMemoryUlong(Resource, sizeof(ERESOURCE), FAT_FILL_FREE);
#endif

    ExFreeToNPagedLookasideList( &FatEResourceLookasideList, (PVOID) Resource );
}

#ifdef __REACTOS__
static
#endif
INLINE
PIRP_CONTEXT
FatAllocateIrpContext (
    )
{
    return (PIRP_CONTEXT) ExAllocateFromNPagedLookasideList( &FatIrpContextLookasideList );
}

#ifdef __REACTOS__
static
#endif
INLINE
VOID
FatFreeIrpContext (
    IN PIRP_CONTEXT IrpContext
    )
{
#if FAT_FILL_FREE
    RtlFillMemoryUlong(IrpContext, sizeof(IRP_CONTEXT), FAT_FILL_FREE);
#endif

    ExFreeToNPagedLookasideList( &FatIrpContextLookasideList, (PVOID) IrpContext );
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatInitializeVcb)
#pragma alloc_text(PAGE, FatTearDownVcb)
#pragma alloc_text(PAGE, FatDeleteVcb)
#pragma alloc_text(PAGE, FatCreateRootDcb)
#pragma alloc_text(PAGE, FatCreateFcb)
#pragma alloc_text(PAGE, FatCreateDcb)
#pragma alloc_text(PAGE, FatDeleteFcb)
#pragma alloc_text(PAGE, FatCreateCcb)
#pragma alloc_text(PAGE, FatDeallocateCcbStrings)
#pragma alloc_text(PAGE, FatDeleteCcb)
#pragma alloc_text(PAGE, FatGetNextFcbTopDown)
#pragma alloc_text(PAGE, FatGetNextFcbBottomUp)
#pragma alloc_text(PAGE, FatConstructNamesInFcb)
#pragma alloc_text(PAGE, FatCheckFreeDirentBitmap)
#pragma alloc_text(PAGE, FatCreateIrpContext)
#pragma alloc_text(PAGE, FatDeleteIrpContext_Real)
#pragma alloc_text(PAGE, FatIsHandleCountZero)
#pragma alloc_text(PAGE, FatAllocateCloseContext)
#pragma alloc_text(PAGE, FatPreallocateCloseContext)
#pragma alloc_text(PAGE, FatEnsureStringBufferEnough)
#pragma alloc_text(PAGE, FatFreeStringBuffer)
#pragma alloc_text(PAGE, FatScanForDataTrack)
#endif


_Requires_lock_held_(_Global_critical_region_)
VOID
FatInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDEVICE_OBJECT FsDeviceObject
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

    FsDeviceObject - The filesystem device object that the mount was directed
                     too.

Return Value:

    None.

--*/

{
    CC_FILE_SIZES FileSizes;
    PDEVICE_OBJECT RealDevice;
    ULONG i;

    STORAGE_HOTPLUG_INFO HotplugInfo;
    STORAGE_DEVICE_NUMBER StorDeviceNumber;
    NTSTATUS Status;

    //
    //  The following variables are used for abnormal unwind
    //

    PLIST_ENTRY UnwindEntryList = NULL;
    PERESOURCE UnwindResource = NULL;
    PERESOURCE UnwindResource2 = NULL;
    PFILE_OBJECT UnwindFileObject = NULL;
    PFILE_OBJECT UnwindCacheMap = NULL;
    BOOLEAN UnwindWeAllocatedMcb = FALSE;
    PFILE_SYSTEM_STATISTICS UnwindStatistics = NULL;
    BOOLEAN UnwindWeAllocatedBadBlockMap = FALSE;
    BOOLEAN CloseContextAllocated = FALSE;
    
    PAGED_CODE();
    UNREFERENCED_PARAMETER( FsDeviceObject );

    DebugTrace(+1, Dbg, "FatInitializeVcb, Vcb = %p\n", Vcb);

    _SEH2_TRY {

        //
        //  We start by first zeroing out all of the VCB, this will guarantee
        //  that any stale data is wiped clean
        //

        RtlZeroMemory( Vcb, sizeof(VCB) );

        //
        //  Set the proper node type code and node byte size
        //

        Vcb->VolumeFileHeader.NodeTypeCode = FAT_NTC_VCB;
        Vcb->VolumeFileHeader.NodeByteSize = sizeof(VCB);

        //
        //  Initialize the tunneling cache
        //

        FsRtlInitializeTunnelCache(&Vcb->Tunnel);

        //
        //  Insert this Vcb record on the FatData.VcbQueue
        //

        NT_ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );


#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable: 28137, "prefast wants the wait to be a constant, but that isn't possible for the way fastfat is designed" )
#pragma prefast( disable: 28193, "this will always wait" )
#endif

        (VOID)FatAcquireExclusiveGlobal( IrpContext );

#ifdef _MSC_VER
#pragma prefast( pop )
#endif

        InsertTailList( &FatData.VcbQueue, &Vcb->VcbLinks );
        FatReleaseGlobal( IrpContext );
        UnwindEntryList = &Vcb->VcbLinks;

        //
        //  Set the Target Device Object, Vpb, and Vcb State fields
        //


        ObReferenceObject( TargetDeviceObject );
        Vcb->TargetDeviceObject = TargetDeviceObject;
        Vcb->Vpb = Vpb;

        Vcb->CurrentDevice = Vpb->RealDevice;

        //
        //  Set the removable media and defflush flags based on the storage
        //  inquiry and the old characteristic bits.
        //

        Status = FatPerformDevIoCtrl( IrpContext,
                                      IOCTL_STORAGE_GET_HOTPLUG_INFO,
                                      TargetDeviceObject,
                                      NULL,
                                      0,
                                      &HotplugInfo,
                                      sizeof(HotplugInfo),
                                      FALSE,
                                      TRUE,
                                      NULL );

        if (NT_SUCCESS( Status )) {

            if (HotplugInfo.MediaRemovable) {

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA );
            }

            //
            //  If the media or device is hot-pluggable, then set this flag.
            //

            if (HotplugInfo.MediaHotplug || HotplugInfo.DeviceHotplug) {

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_HOTPLUGGABLE );
            }

            if (!HotplugInfo.WriteCacheEnableOverride) {

                //
                //  If the device or media is hotplug and the override is not
                //  set, force defflush behavior for the device.
                //

                if (HotplugInfo.MediaHotplug || HotplugInfo.DeviceHotplug) {

                    NT_ASSERT( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_HOTPLUGGABLE ));

                    SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH );

                //
                //  Now, for removables that claim to be lockable, lob a lock
                //  request and see if it works.  There can unfortunately be
                //  transient, media dependent reasons that it can fail.  If
                //  it does not, we must force defflush on.
                //

                } else if (HotplugInfo.MediaRemovable &&
                           !HotplugInfo.MediaHotplug) {

                    Status = FatToggleMediaEjectDisable( IrpContext, Vcb, TRUE );

                    if (!NT_SUCCESS( Status )) {

                        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH );

                    }

                    (VOID)FatToggleMediaEjectDisable( IrpContext, Vcb, FALSE );
                }
            }
        }

        if (FlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA );
        }

        //
        //  Make sure we turn on deferred flushing for floppies like we always
        //  have.
        //

        if (FlagOn(Vpb->RealDevice->Characteristics, FILE_FLOPPY_DISKETTE)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH );
        }

        //
        //  Query the storage device number.
        //

        Status = FatPerformDevIoCtrl( IrpContext,
                                      IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                      TargetDeviceObject,
                                      NULL,
                                      0,
                                      &StorDeviceNumber,
                                      sizeof(StorDeviceNumber),
                                      FALSE,
                                      TRUE,
                                      NULL );

        if (NT_SUCCESS( Status )) {

            Vcb->DeviceNumber = StorDeviceNumber.DeviceNumber;

        } else {

            Vcb->DeviceNumber = (ULONG)(-1);
        }

        FatSetVcbCondition( Vcb, VcbGood);

        //
        //  Initialize the resource variable for the Vcb
        //

        ExInitializeResourceLite( &Vcb->Resource );
        UnwindResource = &Vcb->Resource;

        ExInitializeResourceLite( &Vcb->ChangeBitMapResource );
        UnwindResource2 = &Vcb->ChangeBitMapResource;

        //
        //  Initialize the free cluster bitmap mutex.
        //

        ExInitializeFastMutex( &Vcb->FreeClusterBitMapMutex );

        //
        //  Create the special file object for the virtual volume file with a close
        //  context, its pointers back to the Vcb and the section object pointer.
        //
        //  We don't have to unwind the close context.  That will happen in the close
        //  path automatically.
        //

        RealDevice = Vcb->CurrentDevice;

        FatPreallocateCloseContext(Vcb);
        CloseContextAllocated = TRUE;

        Vcb->VirtualVolumeFile = UnwindFileObject = IoCreateStreamFileObject( NULL, RealDevice );

        FatSetFileObject( Vcb->VirtualVolumeFile,
                          VirtualVolumeFile,
                          Vcb,
                          NULL );

        //
        //  Remember this internal, residual open.
        //

        InterlockedIncrement( (LONG*)&(Vcb->InternalOpenCount) );
        InterlockedIncrement( (LONG*)&(Vcb->ResidualOpenCount) );

        Vcb->VirtualVolumeFile->SectionObjectPointer = &Vcb->SectionObjectPointers;

        Vcb->VirtualVolumeFile->ReadAccess = TRUE;
        Vcb->VirtualVolumeFile->WriteAccess = TRUE;
        Vcb->VirtualVolumeFile->DeleteAccess = TRUE;

        //
        //  Initialize the notify structures.
        //

        InitializeListHead( &Vcb->DirNotifyList );

        FsRtlNotifyInitializeSync( &Vcb->NotifySync );

        //
        //  Initialize the Cache Map for the volume file.  The size is
        //  initially set to that of our first read.  It will be extended
        //  when we know how big the Fat is.
        //

        FileSizes.AllocationSize.QuadPart =
        FileSizes.FileSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
        FileSizes.ValidDataLength = FatMaxLarge;

        FatInitializeCacheMap( Vcb->VirtualVolumeFile,
                               &FileSizes,
                               TRUE,
                               &FatData.CacheManagerNoOpCallbacks,
                               Vcb );

        UnwindCacheMap = Vcb->VirtualVolumeFile;

        //
        //  Initialize the structure that will keep track of dirty fat sectors.
        //  The largest possible Mcb structures are less than 1K, so we use
        //  non paged pool.
        //

        FsRtlInitializeLargeMcb( &Vcb->DirtyFatMcb, PagedPool );

        UnwindWeAllocatedMcb = TRUE;

        //
        //  Initialize the structure that will keep track of bad clusters on the volume.
        //
        //  It will be empty until it is populated by FSCTL_GET_RETRIEVAL_POINTERS with a volume handle.
        //

        FsRtlInitializeLargeMcb( &Vcb->BadBlockMcb, PagedPool );
        UnwindWeAllocatedBadBlockMap = TRUE;

        //
        //  Set the cluster index hint to the first valid cluster of a fat: 2
        //

        Vcb->ClusterHint = 2;

        //
        //  Initialize the directory stream file object creation event.
        //  This event is also "borrowed" for async non-cached writes.
        //

        ExInitializeFastMutex( &Vcb->DirectoryFileCreationMutex );

        //
        //  Initialize the clean volume callback Timer and DPC.
        //

        KeInitializeTimer( &Vcb->CleanVolumeTimer );

        KeInitializeDpc( &Vcb->CleanVolumeDpc, FatCleanVolumeDpc, Vcb );

        //
        //  Initialize the performance counters.
        //

#ifndef __REACTOS__
        Vcb->Statistics = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
        Vcb->Statistics = FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                    sizeof(FILE_SYSTEM_STATISTICS) * FatData.NumberProcessors,
                                                    TAG_VCB_STATS );
        UnwindStatistics = Vcb->Statistics;

        RtlZeroMemory( Vcb->Statistics, sizeof(FILE_SYSTEM_STATISTICS) * FatData.NumberProcessors );

        for (i = 0; i < FatData.NumberProcessors; i += 1) {
            Vcb->Statistics[i].Common.FileSystemType = FILESYSTEM_STATISTICS_TYPE_FAT;
            Vcb->Statistics[i].Common.Version = 1;
            Vcb->Statistics[i].Common.SizeOfCompleteStructure =
                sizeof(FILE_SYSTEM_STATISTICS);
        }

        //
        //  Pick up a VPB right now so we know we can pull this filesystem stack off
        //  of the storage stack on demand.
        //

#ifndef __REACTOS__
        Vcb->SwapVpb = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
        Vcb->SwapVpb = FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                 sizeof( VPB ),
                                                 TAG_VPB );

        RtlZeroMemory( Vcb->SwapVpb, sizeof( VPB ) );

        //
        //  Initialize the close queue listheads.
        //

        InitializeListHead( &Vcb->AsyncCloseList );
        InitializeListHead( &Vcb->DelayedCloseList );

        //
        //  Initialize the Advanced FCB Header
        //

        ExInitializeFastMutex( &Vcb->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &Vcb->VolumeFileHeader,
                                  &Vcb->AdvancedFcbHeaderMutex );


        //
        //  With the Vcb now set up, set the IrpContext Vcb field.
        //

        IrpContext->Vcb = Vcb;

    } _SEH2_FINALLY {

        DebugUnwind( FatInitializeVcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (_SEH2_AbnormalTermination()) {

            if (UnwindCacheMap != NULL) { FatSyncUninitializeCacheMap( IrpContext, UnwindCacheMap ); }
            if (UnwindFileObject != NULL) { ObDereferenceObject( UnwindFileObject ); }
            if (UnwindResource != NULL) { FatDeleteResource( UnwindResource ); }
            if (UnwindResource2 != NULL) { FatDeleteResource( UnwindResource2 ); }
            if (UnwindWeAllocatedMcb) { FsRtlUninitializeLargeMcb( &Vcb->DirtyFatMcb ); }
            if (UnwindWeAllocatedBadBlockMap) { FsRtlUninitializeLargeMcb(&Vcb->BadBlockMcb ); }
            if (UnwindEntryList != NULL) {
#ifdef _MSC_VER
#pragma prefast( suppress: 28137, "prefast wants the wait to be a constant, but that isn't possible for the way fastfat is designed" )          
#endif      
                (VOID)FatAcquireExclusiveGlobal( IrpContext );
                RemoveEntryList( UnwindEntryList );
                FatReleaseGlobal( IrpContext );
            }
            if (UnwindStatistics != NULL) { ExFreePool( UnwindStatistics ); }

            //
            // Cleanup the close context we preallocated above.
            //
            
            if (CloseContextAllocated && (Vcb->VirtualVolumeFile == NULL)) {

                //
                // FatAllocateCloseContext does not allocate memory, it
                // pulls a close context off the preallocated slist queue.
                //
                // Doing this here is necessary to balance out the one we 
                // preallocated for the Vcb earlier in this function, but 
                // only if we failed to create the virtual volume file.
                //
                // If VirtualVolumeFile is not NULL, then this CloseContext
                // will get cleaned up when the close comes in for it during
                // Vcb teardown.
                //
                
                PCLOSE_CONTEXT CloseContext = FatAllocateCloseContext(Vcb);

                ExFreePool( CloseContext );
                CloseContextAllocated = FALSE;
            }
        }

        DebugTrace(-1, Dbg, "FatInitializeVcb -> VOID\n", 0);
    } _SEH2_END;

    //
    //  and return to our caller
    //

    UNREFERENCED_PARAMETER( IrpContext );

    return;
}


VOID
FatTearDownVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine tries to remove all internal opens from the volume.

Arguments:

    IrpContext - Supplies the context for the overall request.

    Vcb - Supplies the Vcb to be torn down.

Return Value:

    None

--*/

{
    PFILE_OBJECT DirectoryFileObject;


    PAGED_CODE();

    //
    //  Get rid of the virtual volume file, if we need to.
    //

    if (Vcb->VirtualVolumeFile != NULL) {

        //
        //  Uninitialize the cache
        //

        CcUninitializeCacheMap( Vcb->VirtualVolumeFile,
                                &FatLargeZero,
                                NULL );

        FsRtlTeardownPerStreamContexts( &Vcb->VolumeFileHeader );

        ObDereferenceObject( Vcb->VirtualVolumeFile );

        Vcb->VirtualVolumeFile = NULL;
    }

    //
    //  Close down the EA file.
    //

    FatCloseEaFile( IrpContext, Vcb, FALSE );

    //
    //  Close down the root directory stream..
    //

    if (Vcb->RootDcb != NULL) {

        DirectoryFileObject = Vcb->RootDcb->Specific.Dcb.DirectoryFile;

        if (DirectoryFileObject != NULL) {

            //
            //  Tear down this directory file.
            //

            CcUninitializeCacheMap( DirectoryFileObject,
                                    &FatLargeZero,
                                    NULL );

            Vcb->RootDcb->Specific.Dcb.DirectoryFile = NULL;
            ObDereferenceObject( DirectoryFileObject );
        }
    }

    //
    //  The VCB can no longer be used.
    //

    FatSetVcbCondition( Vcb, VcbBad );
}


VOID
FatDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes the Vcb record from Fat's in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    None

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteVcb, Vcb = %p\n", Vcb);

    //
    //  If the IrpContext points to the VCB being deleted NULL out the stail
    //  pointer.
    //

    if (IrpContext->Vcb == Vcb) {

        IrpContext->Vcb = NULL;

    }

    //
    //  Chuck the backpocket Vpb we kept just in case.
    //

    if (Vcb->SwapVpb) {

        ExFreePool( Vcb->SwapVpb );

    }

    //
    //  Free the VPB, if we need to.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED )) {

        //
        //  We swapped the VPB, so we need to free the main one.
        //

        ExFreePool( Vcb->Vpb );
    }

    if (Vcb->VolumeGuidPath.Buffer) {
        ExFreePool( Vcb->VolumeGuidPath.Buffer );
        Vcb->VolumeGuidPath.Buffer = NULL;
    }

    //
    //  Remove this record from the global list of all Vcb records.
    //  Note that the global lock must already be held when calling
    //  this function.
    //

    RemoveEntryList( &(Vcb->VcbLinks) );

    //
    //  Make sure the direct access open count is zero, and the open file count
    //  is also zero.
    //

    if ((Vcb->DirectAccessOpenCount != 0) || (Vcb->OpenFileCount != 0)) {

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
        FatBugCheck( 0, 0, 0 );
    }

    //
    //  Remove the EaFcb and dereference the Fcb for the Ea file if it
    //  exists.
    //

    if (Vcb->EaFcb != NULL) {

        Vcb->EaFcb->OpenCount = 0;
        FatDeleteFcb( IrpContext, &Vcb->EaFcb );
    }

    //
    //  Remove the Root Dcb
    //

    if (Vcb->RootDcb != NULL) {

        //
        //  Rundown stale child Fcbs that may be hanging around.  Yes, this
        //  can happen.  No, the create path isn't perfectly defensive about
        //  tearing down branches built up on creates that don't wind up
        //  succeeding.  Normal system operation usually winds up having
        //  cleaned them out through re-visiting, but ...
        //
        //  Just pick off Fcbs from the bottom of the tree until we run out.
        //  Then we delete the root Dcb.
        //

        while( (Fcb = FatGetNextFcbBottomUp( IrpContext, NULL, Vcb->RootDcb )) != Vcb->RootDcb ) {

            FatDeleteFcb( IrpContext, &Fcb );
        }

        FatDeleteFcb( IrpContext, &Vcb->RootDcb );
    }

    //
    //  Uninitialize the notify sychronization object.
    //

    FsRtlNotifyUninitializeSync( &Vcb->NotifySync );

    //
    //  Uninitialize the resource variable for the Vcb
    //

    FatDeleteResource( &Vcb->Resource );
    FatDeleteResource( &Vcb->ChangeBitMapResource );

    //
    //  If allocation support has been setup, free it.
    //

    if (Vcb->FreeClusterBitMap.Buffer != NULL) {

        FatTearDownAllocationSupport( IrpContext, Vcb );
    }

    //
    //  UnInitialize the Mcb structure that kept track of dirty fat sectors.
    //

    FsRtlUninitializeLargeMcb( &Vcb->DirtyFatMcb );

    //
    //  Uninitialize the Mcb structure that kept track of bad sectors.
    //

    FsRtlUninitializeLargeMcb( &Vcb->BadBlockMcb );

    //
    //  Free the pool for the stached copy of the boot sector
    //

    if ( Vcb->First0x24BytesOfBootSector ) {

        ExFreePool( Vcb->First0x24BytesOfBootSector );
        Vcb->First0x24BytesOfBootSector = NULL;
    }

    //
    //  Cancel the CleanVolume Timer and Dpc
    //

    (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );

    (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );

    //
    //  Free the performance counters memory
    //

    ExFreePool( Vcb->Statistics );

    //
    //  Clean out the tunneling cache
    //

    FsRtlDeleteTunnelCache(&Vcb->Tunnel);

    //
    // Dereference the target device object.
    //

    ObDereferenceObject( Vcb->TargetDeviceObject );

    //
    //  We better have used all the close contexts we allocated. There could be
    //  one remaining if we're doing teardown due to a final close coming in on
    //  a directory file stream object.  It will be freed on the way back up.
    //

    NT_ASSERT( Vcb->CloseContextCount <= 1);

    //
    //  And zero out the Vcb, this will help ensure that any stale data is
    //  wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatDeleteVcb -> VOID\n", 0);

    return;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

Return Value:

    None. The Vcb is modified in-place.

--*/

{
    PDCB Dcb = NULL;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[2] = { NULL, NULL };
    PERESOURCE UnwindResource = NULL;
    PERESOURCE UnwindResource2 = NULL;
    PLARGE_MCB UnwindMcb = NULL;
    PFILE_OBJECT UnwindFileObject = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateRootDcb, Vcb = %p\n", Vcb);

    _SEH2_TRY {

        //
        //  Make sure we don't already have a root dcb for this vcb
        //

        if (Vcb->RootDcb != NULL) {

            DebugDump("Error trying to create multiple root dcbs\n", 0, Vcb);
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )          
#endif  
            FatBugCheck( 0, 0, 0 );
        }

        //
        //  Allocate a new DCB and zero it out, we use Dcb locally so we don't
        //  have to continually reference through the Vcb
        //

#ifndef __REACTOS__
        UnwindStorage[0] = Dcb = Vcb->RootDcb = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
        UnwindStorage[0] = Dcb = Vcb->RootDcb = FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                                          sizeof(DCB),
                                                                          TAG_FCB );

        RtlZeroMemory( Dcb, sizeof(DCB));

        UnwindStorage[1] =
        Dcb->NonPaged = FatAllocateNonPagedFcb();

        RtlZeroMemory( Dcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size, and call backs
        //

        Dcb->Header.NodeTypeCode = FAT_NTC_ROOT_DCB;
        Dcb->Header.NodeByteSize = sizeof(DCB);

        Dcb->FcbCondition = FcbGood;

        //
        //  The parent Dcb, initial state, open count, dirent location
        //  information, and directory change count fields are already zero so
        //  we can skip setting them
        //

        //
        //  Initialize the resource variable
        //

        UnwindResource =
        Dcb->Header.Resource = FatAllocateResource();

        //
        //  Initialize the PagingIo Resource.  We no longer use the FsRtl common
        //  shared pool because this led to a) deadlocks due to cases where files
        //  and their parent directories shared a resource and b) there is no way
        //  to anticipate inter-driver induced deadlock via recursive operation.
        //

        UnwindResource2 =
        Dcb->Header.PagingIoResource = FatAllocateResource();

        //
        //  The root Dcb has an empty parent dcb links field
        //

        InitializeListHead( &Dcb->ParentDcbLinks );

        //
        //  Set the Vcb
        //

        Dcb->Vcb = Vcb;

        //
        //  initialize the parent dcb queue.
        //

        InitializeListHead( &Dcb->Specific.Dcb.ParentDcbQueue );

        //
        //  Set the full file name up.
        //

        Dcb->FullFileName.Buffer = L"\\";
        Dcb->FullFileName.Length = (USHORT)2;
        Dcb->FullFileName.MaximumLength = (USHORT)4;

        Dcb->ShortName.Name.Oem.Buffer = "\\";
        Dcb->ShortName.Name.Oem.Length = (USHORT)1;
        Dcb->ShortName.Name.Oem.MaximumLength = (USHORT)2;

        //
        //  Construct a lie about file properties since we don't
        //  have a proper "." entry to look at.
        //

        Dcb->DirentFatFlags = FILE_ATTRIBUTE_DIRECTORY;

        //
        //  Initialize Advanced FCB Header fields
        //

        ExInitializeFastMutex( &Dcb->NonPaged->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &Dcb->Header,
                                  &Dcb->NonPaged->AdvancedFcbHeaderMutex );

        //
        //  Initialize the Mcb, and setup its mapping.  Note that the root
        //  directory is a fixed size so we can set it everything up now.
        //

#ifndef __REACTOS__
        FsRtlInitializeLargeMcb( &Dcb->Mcb, NonPagedPoolNx );
#else
        FsRtlInitializeLargeMcb( &Dcb->Mcb, NonPagedPool );
#endif
        UnwindMcb = &Dcb->Mcb;

        if (FatIsFat32(Vcb)) {

            //
            //  The first cluster of fat32 roots comes from the BPB
            //

            Dcb->FirstClusterOfFile = Vcb->Bpb.RootDirFirstCluster;

        } else {

            FatAddMcbEntry( Vcb, &Dcb->Mcb,
                            0,
                            FatRootDirectoryLbo( &Vcb->Bpb ),
                            FatRootDirectorySize( &Vcb->Bpb ));
        }

        if (FatIsFat32(Vcb)) {

            //
            //  Find the size of the fat32 root. As a side-effect, this will create
            //  MCBs for the entire root. In the process of doing this, we may
            //  discover that the FAT chain is bogus and raise corruption.
            //

            Dcb->Header.AllocationSize.LowPart = 0xFFFFFFFF;
            FatLookupFileAllocationSize( IrpContext, Dcb);

            Dcb->Header.FileSize.QuadPart =
                    Dcb->Header.AllocationSize.QuadPart;
        } else {

            //
            //  set the allocation size to real size of the root directory
            //

            Dcb->Header.FileSize.QuadPart =
            Dcb->Header.AllocationSize.QuadPart = FatRootDirectorySize( &Vcb->Bpb );

        }

        //
        //  Set our two create dirent aids to represent that we have yet to
        //  enumerate the directory for never used or deleted dirents.
        //

        Dcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Dcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;

        //
        //  Setup the free dirent bitmap buffer.
        //

        RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                             NULL,
                             0 );

        FatCheckFreeDirentBitmap( IrpContext, Dcb );

#if (NTDDI_VERSION >= NTDDI_WIN8)
        //
        //  Initialize the oplock structure.
        //
        
        FsRtlInitializeOplock( FatGetFcbOplock(Dcb) );
#endif

    } _SEH2_FINALLY {

        DebugUnwind( FatCreateRootDcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (_SEH2_AbnormalTermination()) {

            ULONG i;

            if (UnwindFileObject != NULL) { ObDereferenceObject( UnwindFileObject ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindMcb ); }
            if (UnwindResource != NULL) { FatFreeResource( UnwindResource ); }
            if (UnwindResource2 != NULL) { FatFreeResource( UnwindResource2 ); }

            for (i = 0; i < sizeof(UnwindStorage)/sizeof(PVOID); i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }

            //
            //  Re-zero the entry in the Vcb.
            //

            Vcb->RootDcb = NULL;
        }

        DebugTrace(-1, Dbg, "FatCreateRootDcb -> %p\n", Dcb);
    } _SEH2_END;

    return;
}


PFCB
FatCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN PUNICODE_STRING OrigLfn OPTIONAL,    
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN SingleResource
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in-memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    ParentDcb - Supplies the parent dcb that the new FCB is under.

    LfnOffsetWithinDirectory - Supplies the offset of the LFN.  If there is
        no LFN associated with this file then this value is same as
        DirentOffsetWithinDirectory.

    DirentOffsetWithinDirectory - Supplies the offset, in bytes from the
        start of the directory file where the dirent for the fcb is located

    Dirent - Supplies the dirent for the fcb being created

    Lfn - Supplies a long UNICODE name associated with this file.

    IsPagingFile - Indicates if we are creating an FCB for a paging file
        or some other type of file.

    SingleResource - Indicates if this Fcb should share a single resource
        as both main and paging.

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    PFCB Fcb = NULL;
    POOL_TYPE PoolType;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[2] = { NULL, NULL };
    PERESOURCE UnwindResource = NULL;
    PERESOURCE UnwindResource2 = NULL;
    PLIST_ENTRY UnwindEntryList = NULL;
    PLARGE_MCB UnwindMcb = NULL;
    PFILE_LOCK UnwindFileLock = NULL;
    POPLOCK UnwindOplock = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( OrigLfn );
    
    DebugTrace(+1, Dbg, "FatCreateFcb\n", 0);

    _SEH2_TRY {

        //
        //  Determine the pool type we should be using for the fcb and the
        //  mcb structure
        //

        if (IsPagingFile) {

#ifndef __REACTOS__
            PoolType = NonPagedPoolNx;
            Fcb = UnwindStorage[0] = FsRtlAllocatePoolWithTag( NonPagedPoolNx,
#else
            PoolType = NonPagedPool;
            Fcb = UnwindStorage[0] = FsRtlAllocatePoolWithTag( NonPagedPool,
#endif
                                                               sizeof(FCB),
                                                               TAG_FCB );
        } else {

            PoolType = PagedPool;
            Fcb = UnwindStorage[0] = FatAllocateFcb();

        }

        //
        //  ... and zero it out
        //

        RtlZeroMemory( Fcb, sizeof(FCB) );

        UnwindStorage[1] =
        Fcb->NonPaged = FatAllocateNonPagedFcb();

        RtlZeroMemory( Fcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size, and call backs
        //

        Fcb->Header.NodeTypeCode = FAT_NTC_FCB;
        Fcb->Header.NodeByteSize = sizeof(FCB);

        Fcb->FcbCondition = FcbGood;

        //
        //  Check to see if we need to set the Fcb state to indicate that this
        //  is a paging/system file.  This will prevent it from being opened
        //  again.
        //

        if (IsPagingFile) {

            SetFlag( Fcb->FcbState, FCB_STATE_PAGING_FILE | FCB_STATE_SYSTEM_FILE );
        }

        //
        //  The initial state, open count, and segment objects fields are already
        //  zero so we can skip setting them
        //

        //
        //  Initialize the resource variable
        //


        UnwindResource =
        Fcb->Header.Resource = FatAllocateResource();

        //
        //  Initialize the PagingIo Resource.  We no longer use the FsRtl common
        //  shared pool because this led to a) deadlocks due to cases where files
        //  and their parent directories shared a resource and b) there is no way
        //  to anticipate inter-driver induced deadlock via recursive operation.
        //

        if (SingleResource) {

            Fcb->Header.PagingIoResource = Fcb->Header.Resource;

        } else {

            UnwindResource2 =
            Fcb->Header.PagingIoResource = FatAllocateResource();
        }

        //
        //  Insert this fcb into our parent dcb's queue.
        //
        //  There is a deep reason why this goes on the tail, to allow us
        //  to easily enumerate all child directories before child files.
        //  This is important to let us maintain whole-volume lockorder
        //  via BottomUp enumeration.
        //
            
        InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                        &Fcb->ParentDcbLinks );
        UnwindEntryList = &Fcb->ParentDcbLinks;
        
        //
        //  Point back to our parent dcb
        //

        Fcb->ParentDcb = ParentDcb;

        //
        //  Set the Vcb
        //

        Fcb->Vcb = Vcb;

        //
        //  Set the dirent offset within the directory
        //

        Fcb->LfnOffsetWithinDirectory = LfnOffsetWithinDirectory;
        Fcb->DirentOffsetWithinDirectory = DirentOffsetWithinDirectory;

        //
        //  Set the DirentFatFlags and LastWriteTime
        //

        Fcb->DirentFatFlags = Dirent->Attributes;

        Fcb->LastWriteTime = FatFatTimeToNtTime( IrpContext,
                                                 Dirent->LastWriteTime,
                                                 0 );

        //
        //  These fields are only non-zero when in Chicago mode.
        //

        if (FatData.ChicagoMode) {

#ifndef __REACTOS__
            LARGE_INTEGER FatSystemJanOne1980 = {0};
#else
            LARGE_INTEGER FatSystemJanOne1980 = {{0}};
#endif

            //
            //  If either date is possibly zero, get the system
            //  version of 1/1/80.
            //

            if ((((PUSHORT)Dirent)[9] & ((PUSHORT)Dirent)[8]) == 0) {

                ExLocalTimeToSystemTime( &FatJanOne1980,
                                         &FatSystemJanOne1980 );
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[9] != 0) {

                Fcb->LastAccessTime =
                    FatFatDateToNtTime( IrpContext,
                                        Dirent->LastAccessDate );

            } else {

                Fcb->LastAccessTime = FatSystemJanOne1980;
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[8] != 0) {

                Fcb->CreationTime =
                    FatFatTimeToNtTime( IrpContext,
                                        Dirent->CreationTime,
                                        Dirent->CreationMSec );

            } else {

                Fcb->CreationTime = FatSystemJanOne1980;
            }
        }

        //
        //  Initialize Advanced FCB Header fields
        //

        ExInitializeFastMutex( &Fcb->NonPaged->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &Fcb->Header,
                                  &Fcb->NonPaged->AdvancedFcbHeaderMutex );

        //
        //  To make FAT match the present functionality of NTFS, disable
        //  stream contexts on paging files
        //

        if (IsPagingFile) {

            SetFlag( Fcb->Header.Flags2, FSRTL_FLAG2_IS_PAGING_FILE );
            ClearFlag( Fcb->Header.Flags2, FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS );
        }

        //
        //  Initialize the Mcb
        //

        FsRtlInitializeLargeMcb( &Fcb->Mcb, PoolType );
        UnwindMcb = &Fcb->Mcb;

        //
        //  Set the file size, valid data length, first cluster of file,
        //  and allocation size based on the information stored in the dirent
        //

        Fcb->Header.FileSize.LowPart = Dirent->FileSize;

        Fcb->Header.ValidDataLength.LowPart = Dirent->FileSize;

        Fcb->ValidDataToDisk = Dirent->FileSize;

        Fcb->FirstClusterOfFile = (ULONG)Dirent->FirstClusterOfFile;

        if ( FatIsFat32(Vcb) ) {

            Fcb->FirstClusterOfFile += Dirent->FirstClusterOfFileHi << 16;
        }

        if ( Fcb->FirstClusterOfFile == 0 ) {

            Fcb->Header.AllocationSize.QuadPart = 0;

        } else {

            Fcb->Header.AllocationSize.QuadPart = FCB_LOOKUP_ALLOCATIONSIZE_HINT;
        }


        //
        //  Initialize the Fcb's file lock record
        //

        FsRtlInitializeFileLock( &Fcb->Specific.Fcb.FileLock, NULL, NULL );
        UnwindFileLock = &Fcb->Specific.Fcb.FileLock;

        //
        //  Initialize the oplock structure.
        //

        FsRtlInitializeOplock( FatGetFcbOplock(Fcb) );
        UnwindOplock = FatGetFcbOplock(Fcb);

        //
        //  Indicate that Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = TRUE;

        //
        //  Set the file names.  This must be the last thing we do.
        //

        FatConstructNamesInFcb( IrpContext,
                                Fcb,
                                Dirent,
                                Lfn );

        //
        //  Drop the shortname hint so prefix searches can figure out
        //  what they found
        //

        Fcb->ShortName.FileNameDos = TRUE;

    } _SEH2_FINALLY {

        DebugUnwind( FatCreateFcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (_SEH2_AbnormalTermination()) {

            ULONG i;

            if (UnwindOplock != NULL) { FsRtlUninitializeOplock( UnwindOplock ); }
            if (UnwindFileLock != NULL) { FsRtlUninitializeFileLock( UnwindFileLock ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindMcb ); }
            if (UnwindEntryList != NULL) { RemoveEntryList( UnwindEntryList ); }
            if (UnwindResource != NULL) { FatFreeResource( UnwindResource ); }
            if (UnwindResource2 != NULL) { FatFreeResource( UnwindResource2 ); }

            for (i = 0; i < sizeof(UnwindStorage)/sizeof(PVOID); i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }
        }

        DebugTrace(-1, Dbg, "FatCreateFcb -> %p\n", Fcb);
    } _SEH2_END;

    //
    //  return and tell the caller
    //

    return Fcb;
}


PDCB
FatCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Dcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under.

    ParentDcb - Supplies the parent dcb that the new DCB is under.

    LfnOffsetWithinDirectory - Supplies the offset of the LFN.  If there is
        no LFN associated with this file then this value is same as
        DirentOffsetWithinDirectory.

    DirentOffsetWithinDirectory - Supplies the offset, in bytes from the
        start of the directory file where the dirent for the fcb is located

    Dirent - Supplies the dirent for the dcb being created

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    Lfn - Supplies a long UNICODE name associated with this directory.

Return Value:

    PDCB - Returns a pointer to the newly allocated DCB

--*/

{
    PDCB Dcb = NULL;

    //
    //  The following variables are used for abnormal unwind
    //

    PVOID UnwindStorage[2] = { NULL, NULL  };
    PERESOURCE UnwindResource = NULL;
    PERESOURCE UnwindResource2 = NULL;
    PLIST_ENTRY UnwindEntryList = NULL;
    PLARGE_MCB UnwindMcb = NULL;
    POPLOCK UnwindOplock = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateDcb\n", 0);

    _SEH2_TRY {

        //
        //  assert that the only time we are called is if wait is true
        //

        NT_ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        //
        //  Allocate a new DCB, and zero it out
        //

        UnwindStorage[0] = Dcb = FatAllocateFcb();

        RtlZeroMemory( Dcb, sizeof(DCB) );

        UnwindStorage[1] =
        Dcb->NonPaged = FatAllocateNonPagedFcb();

        RtlZeroMemory( Dcb->NonPaged, sizeof( NON_PAGED_FCB ) );

        //
        //  Set the proper node type code, node byte size and call backs
        //

        Dcb->Header.NodeTypeCode = FAT_NTC_DCB;
        Dcb->Header.NodeByteSize = sizeof(DCB);

        Dcb->FcbCondition = FcbGood;

        //
        //  The initial state, open count, and directory change count fields are
        //  already zero so we can skip setting them
        //

        //
        //  Initialize the resource variable
        //


        UnwindResource =
        Dcb->Header.Resource = FatAllocateResource();

        //
        //  Initialize the PagingIo Resource.  We no longer use the FsRtl common
        //  shared pool because this led to a) deadlocks due to cases where files
        //  and their parent directories shared a resource and b) there is no way
        //  to anticipate inter-driver induced deadlock via recursive operation.
        //

        UnwindResource2 =
        Dcb->Header.PagingIoResource = FatAllocateResource();

        //
        //  Insert this Dcb into our parent dcb's queue
        //
        //  There is a deep reason why this goes on the head, to allow us
        //  to easily enumerate all child directories before child files.
        //  This is important to let us maintain whole-volume lockorder
        //  via BottomUp enumeration.
        //

        InsertHeadList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                        &Dcb->ParentDcbLinks );
        UnwindEntryList = &Dcb->ParentDcbLinks;

        //
        //  Point back to our parent dcb
        //

        Dcb->ParentDcb = ParentDcb;

        //
        //  Set the Vcb
        //

        Dcb->Vcb = Vcb;

        //
        //  Set the dirent offset within the directory
        //

        Dcb->LfnOffsetWithinDirectory = LfnOffsetWithinDirectory;
        Dcb->DirentOffsetWithinDirectory = DirentOffsetWithinDirectory;

        //
        //  Set the DirentFatFlags and LastWriteTime
        //

        Dcb->DirentFatFlags = Dirent->Attributes;

        Dcb->LastWriteTime = FatFatTimeToNtTime( IrpContext,
                                                 Dirent->LastWriteTime,
                                                 0 );

        //
        //  These fields are only non-zero when in Chicago mode.
        //

        if (FatData.ChicagoMode) {

#ifndef __REACTOS__
            LARGE_INTEGER FatSystemJanOne1980 = {0};
#else
            LARGE_INTEGER FatSystemJanOne1980 = {{0}};
#endif

            //
            //  If either date is possibly zero, get the system
            //  version of 1/1/80.
            //

            if ((((PUSHORT)Dirent)[9] & ((PUSHORT)Dirent)[8]) == 0) {

                ExLocalTimeToSystemTime( &FatJanOne1980,
                                         &FatSystemJanOne1980 );
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[9] != 0) {

                Dcb->LastAccessTime =
                    FatFatDateToNtTime( IrpContext,
                                        Dirent->LastAccessDate );

            } else {

                Dcb->LastAccessTime = FatSystemJanOne1980;
            }

            //
            //  Only do the really hard work if this field is non-zero.
            //

            if (((PUSHORT)Dirent)[8] != 0) {

                Dcb->CreationTime =
                    FatFatTimeToNtTime( IrpContext,
                                        Dirent->CreationTime,
                                        Dirent->CreationMSec );

            } else {

                Dcb->CreationTime = FatSystemJanOne1980;
            }
        }

        //
        //  Initialize Advanced FCB Header fields
        //

        ExInitializeFastMutex( &Dcb->NonPaged->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &Dcb->Header,
                                  &Dcb->NonPaged->AdvancedFcbHeaderMutex );

        //
        //  Initialize the Mcb
        //

        FsRtlInitializeLargeMcb( &Dcb->Mcb, PagedPool );
        UnwindMcb = &Dcb->Mcb;

        //
        //  Set the file size, first cluster of file, and allocation size
        //  based on the information stored in the dirent
        //

        Dcb->FirstClusterOfFile = (ULONG)Dirent->FirstClusterOfFile;

        if ( FatIsFat32(Dcb->Vcb) ) {

            Dcb->FirstClusterOfFile += Dirent->FirstClusterOfFileHi << 16;
        }

        if ( Dcb->FirstClusterOfFile == 0 ) {

            Dcb->Header.AllocationSize.QuadPart = 0;

        } else {

            Dcb->Header.AllocationSize.QuadPart = FCB_LOOKUP_ALLOCATIONSIZE_HINT;
        }


        //  initialize the notify queues, and the parent dcb queue.
        //

        InitializeListHead( &Dcb->Specific.Dcb.ParentDcbQueue );

        //
        //  Setup the free dirent bitmap buffer.  Since we don't know the
        //  size of the directory, leave it zero for now.
        //

        RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                             NULL,
                             0 );

        //
        //  Set our two create dirent aids to represent that we have yet to
        //  enumerate the directory for never used or deleted dirents.
        //

        Dcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Dcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;

#if (NTDDI_VERSION >= NTDDI_WIN8)
        //
        //  Initialize the oplock structure.
        //

        FsRtlInitializeOplock( FatGetFcbOplock(Dcb) );
        UnwindOplock = FatGetFcbOplock(Dcb);
#endif

        //
        //  Postpone initializing the cache map until we need to do a read/write
        //  of the directory file.


        //
        //  set the file names.  This must be the last thing we do.
        //

        FatConstructNamesInFcb( IrpContext,
                                Dcb,
                                Dirent,
                                Lfn );

        Dcb->ShortName.FileNameDos = TRUE;

    } _SEH2_FINALLY {

        DebugUnwind( FatCreateDcb );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (_SEH2_AbnormalTermination()) {

            ULONG i;

            if (UnwindOplock != NULL) { FsRtlUninitializeOplock( UnwindOplock ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindMcb ); }
            if (UnwindEntryList != NULL) { RemoveEntryList( UnwindEntryList ); }
            if (UnwindResource != NULL) { FatFreeResource( UnwindResource ); }
            if (UnwindResource2 != NULL) { FatFreeResource( UnwindResource2 ); }

            for (i = 0; i < sizeof(UnwindStorage)/sizeof(PVOID); i += 1) {
                if (UnwindStorage[i] != NULL) { ExFreePool( UnwindStorage[i] ); }
            }
        }

        DebugTrace(-1, Dbg, "FatCreateDcb -> %p\n", Dcb);
    } _SEH2_END;

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatCreateDcb -> %p\n", Dcb);

    return Dcb;
}


VOID
FatDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB *FcbPtr
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB, DCB, or ROOT DCB record
    from Fat's in-memory data structures.  It also will remove all
    associated underlings (i.e., Notify irps, and child FCB/DCB records).

Arguments:

    Fcb - Supplies the FCB/DCB/ROOT DCB to be removed

Return Value:

    None

--*/

{
    PFCB Fcb = *FcbPtr;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteFcb, Fcb = %p\n", Fcb);

    //
    //  We can only delete this record if the open count is zero.
    //

    if (Fcb->OpenCount != 0) {

        DebugDump("Error deleting Fcb, Still Open\n", 0, Fcb);
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )        
#endif
        FatBugCheck( 0, 0, 0 );
    }

    //
    //  Better be an FCB/DCB.
    //

    if ((Fcb->Header.NodeTypeCode != FAT_NTC_DCB) &&
        (Fcb->Header.NodeTypeCode != FAT_NTC_ROOT_DCB) &&
        (Fcb->Header.NodeTypeCode != FAT_NTC_FCB)) {

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
        FatBugCheck( 0, 0, 0 );
    }

    //
    //  If this is a DCB then remove every Notify record from the two
    //  notify queues
    //

    if ((Fcb->Header.NodeTypeCode == FAT_NTC_DCB) ||
        (Fcb->Header.NodeTypeCode == FAT_NTC_ROOT_DCB)) {

        //
        //  If we allocated a free dirent bitmap buffer, free it.
        //

        if ((Fcb->Specific.Dcb.FreeDirentBitmap.Buffer != NULL) &&
            (Fcb->Specific.Dcb.FreeDirentBitmap.Buffer !=
             &Fcb->Specific.Dcb.FreeDirentBitmapBuffer[0])) {

            ExFreePool(Fcb->Specific.Dcb.FreeDirentBitmap.Buffer);
        }

#if (NTDDI_VERSION >= NTDDI_WIN8)
        //
        //  Uninitialize the oplock.
        //
        
        FsRtlUninitializeOplock( FatGetFcbOplock(Fcb) );
#endif

        NT_ASSERT( Fcb->Specific.Dcb.DirectoryFileOpenCount == 0 );
        NT_ASSERT( IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) );
        NT_ASSERT( NULL == Fcb->Specific.Dcb.DirectoryFile);

    } else {

        //
        //  Uninitialize the byte range file locks and opportunistic locks
        //

        FsRtlUninitializeFileLock( &Fcb->Specific.Fcb.FileLock );
        FsRtlUninitializeOplock( FatGetFcbOplock(Fcb) );
    }


    //
    //  Release any Filter Context structures associated with this FCB
    //

    FsRtlTeardownPerStreamContexts( &Fcb->Header );

    //
    //  Uninitialize the Mcb
    //

    FsRtlUninitializeLargeMcb( &Fcb->Mcb );

    //
    //  If this is not the root dcb then we need to remove ourselves from
    //  our parents Dcb queue
    //

    if (Fcb->Header.NodeTypeCode != FAT_NTC_ROOT_DCB) {

        RemoveEntryList( &(Fcb->ParentDcbLinks) );
    }

    //
    //  Remove the entry from the splay table if there is still is one.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE )) {

        FatRemoveNames( IrpContext, Fcb );
    }

    //
    //  Free the file name pool if allocated.
    //

    if (Fcb->Header.NodeTypeCode != FAT_NTC_ROOT_DCB) {

        //
        //  If we blew up at inconvenient times, the shortname
        //  could be null even though you will *never* see this
        //  normally.  Rename is a good example of this case.
        //

        if (Fcb->ShortName.Name.Oem.Buffer) {

            ExFreePool( Fcb->ShortName.Name.Oem.Buffer );
        }

        if (Fcb->FullFileName.Buffer) {

            ExFreePool( Fcb->FullFileName.Buffer );
        }
    }

    if (Fcb->ExactCaseLongName.Buffer) {

        ExFreePool(Fcb->ExactCaseLongName.Buffer);
    }

#ifdef SYSCACHE_COMPILE

    if (Fcb->WriteMask) {

        ExFreePool( Fcb->WriteMask );
    }

#endif

    //
    //  Finally deallocate the Fcb and non-paged fcb records
    //

    FatFreeResource( Fcb->Header.Resource );

    if (Fcb->Header.PagingIoResource != Fcb->Header.Resource) {

        FatFreeResource( Fcb->Header.PagingIoResource );
    }

    //
    //  If an Event was allocated, get rid of it.
    //

    if (Fcb->NonPaged->OutstandingAsyncEvent) {

        ExFreePool( Fcb->NonPaged->OutstandingAsyncEvent );
    }

    FatFreeNonPagedFcb( Fcb->NonPaged );

    Fcb->Header.NodeTypeCode = NTC_UNDEFINED;

    FatFreeFcb( Fcb );
    *FcbPtr = NULL;

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "FatDeleteFcb -> VOID\n", 0);
}


PCCB
FatCreateCcb (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

Return Value:

    CCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateCcb\n", 0);

    //
    //  Allocate a new CCB Record
    //

    Ccb = FatAllocateCcb();

    RtlZeroMemory( Ccb, sizeof(CCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = FAT_NTC_CCB;
    Ccb->NodeByteSize = sizeof(CCB);

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatCreateCcb -> %p\n", Ccb);

    UNREFERENCED_PARAMETER( IrpContext );

    return Ccb;
}



VOID
FatDeallocateCcbStrings(
    IN PCCB Ccb
    )
/*++

Routine Description:

    This routine deallocates CCB query templates

Arguments:

    Ccb - Supplies the CCB

Return Value:

    None

--*/
{
    PAGED_CODE();

    //
    //  If we allocated query template buffers, deallocate them now.
    //

    if (FlagOn(Ccb->Flags, CCB_FLAG_FREE_UNICODE)) {

        NT_ASSERT( Ccb->UnicodeQueryTemplate.Buffer);
        NT_ASSERT( !FlagOn( Ccb->Flags, CCB_FLAG_CLOSE_CONTEXT));
        RtlFreeUnicodeString( &Ccb->UnicodeQueryTemplate );
    }

    if (FlagOn(Ccb->Flags, CCB_FLAG_FREE_OEM_BEST_FIT)) {

        NT_ASSERT( Ccb->OemQueryTemplate.Wild.Buffer );
        NT_ASSERT( !FlagOn( Ccb->Flags, CCB_FLAG_CLOSE_CONTEXT));
        RtlFreeOemString( &Ccb->OemQueryTemplate.Wild );
    }

    ClearFlag( Ccb->Flags, CCB_FLAG_FREE_OEM_BEST_FIT | CCB_FLAG_FREE_UNICODE);
}



VOID
FatDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB *Ccb
    )

/*++

Routine Description:

    This routine deallocates and removes the specified CCB record
    from the Fat in memory data structures

Arguments:

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteCcb, Ccb = %p\n", *Ccb);

    FatDeallocateCcbStrings( *Ccb);

    //
    //  Deallocate the Ccb record
    //

    FatFreeCcb( *Ccb );
    *Ccb = NULL;

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatDeleteCcb -> VOID\n", 0);

    UNREFERENCED_PARAMETER( IrpContext );
}


PIRP_CONTEXT
FatCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine creates a new IRP_CONTEXT record

Arguments:

    Irp - Supplies the originating Irp.

    Wait - Supplies the wait value to store in the context

Return Value:

    PIRP_CONTEXT - returns a pointer to the newly allocate IRP_CONTEXT Record

--*/

{
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateIrpContext\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  The only operations a filesystem device object should ever receive
    //  are create/teardown of fsdo handles and operations which do not
    //  occur in the context of fileobjects (i.e., mount).
    //

    if (FatDeviceIsFatFsdo( IrpSp->DeviceObject))  {

        if (IrpSp->FileObject != NULL &&
            IrpSp->MajorFunction != IRP_MJ_CREATE &&
            IrpSp->MajorFunction != IRP_MJ_CLEANUP &&
            IrpSp->MajorFunction != IRP_MJ_CLOSE) {

            ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
        }

        NT_ASSERT( IrpSp->FileObject != NULL ||

                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                 IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES) ||

                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME ) ||

                IrpSp->MajorFunction == IRP_MJ_SHUTDOWN );
    }

    //
    //  Attemtp to allocate from the region first and failing that allocate
    //  from pool.
    //

    DebugDoit( FatFsdEntryCount += 1);

    IrpContext = FatAllocateIrpContext();

    //
    //  Zero out the irp context.
    //

    RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

    //
    //  Set the originating Irp field
    //

    IrpContext->OriginatingIrp = Irp;

    //
    //  Major/Minor Function codes
    //

    IrpContext->MajorFunction = IrpSp->MajorFunction;
    IrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Copy RealDevice for workque algorithms, and also set Write Through
    //  and Removable Media if there is a file object.  Only file system
    //  control Irps won't have a file object, and they should all have
    //  a Vpb as the first IrpSp location.
    //

    if (IrpSp->FileObject != NULL) {

        PFILE_OBJECT FileObject = IrpSp->FileObject;

        IrpContext->RealDevice = FileObject->DeviceObject;
        IrpContext->Vcb = &((PVOLUME_DEVICE_OBJECT)(IrpSp->DeviceObject))->Vcb;

        //
        //  See if the request is Write Through. Look for both FileObjects opened
        //  as write through, and non-cached requests with the SL_WRITE_THROUGH flag set.
        //
        //  The latter can only originate from kernel components. (Note - NtWriteFile()
        //  does redundantly set the SL_W_T flag for all requests it issues on write
        //  through file objects)
        //

        if (IsFileWriteThrough( FileObject, IrpContext->Vcb ) ||
            ( (IrpSp->MajorFunction == IRP_MJ_WRITE) &&
              BooleanFlagOn( Irp->Flags, IRP_NOCACHE) &&
              BooleanFlagOn( IrpSp->Flags, SL_WRITE_THROUGH))) {

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
        }
    } else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) {

        IrpContext->RealDevice = IrpSp->Parameters.MountVolume.Vpb->RealDevice;
    }

    //
    //  Set the wait parameter
    //

    if (Wait) { SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT); }

    //
    //  Set the recursive file system call parameter.  We set it true if
    //  the TopLevelIrp field in the thread local storage is not the current
    //  irp, otherwise we leave it as FALSE.
    //

    if ( IoGetTopLevelIrp() != Irp) {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL);
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatCreateIrpContext -> %p\n", IrpContext);

    return IrpContext;
}



VOID
FatDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine deallocates and removes the specified IRP_CONTEXT record
    from the Fat in memory data structures.  It should only be called
    by FatCompleteRequest.

Arguments:

    IrpContext - Supplies the IRP_CONTEXT to remove

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteIrpContext, IrpContext = %p\n", IrpContext);

    NT_ASSERT( IrpContext->NodeTypeCode == FAT_NTC_IRP_CONTEXT );
    NT_ASSERT( IrpContext->PinCount == 0 );


    //
    //  If there is a FatIoContext that was allocated, free it.
    //

    if (IrpContext->FatIoContext != NULL) {

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_STACK_IO_CONTEXT)) {

            if (IrpContext->FatIoContext->ZeroMdl) {
                IoFreeMdl( IrpContext->FatIoContext->ZeroMdl );
            }

            ExFreePool( IrpContext->FatIoContext );
        }
    }

    //
    //  Drop the IrpContext.
    //

    FatFreeIrpContext( IrpContext );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "FatDeleteIrpContext -> VOID\n", 0);

    return;
}


PFCB
FatGetNextFcbBottomUp (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb OPTIONAL,
    IN PFCB TerminationFcb
    )

/*++

Routine Description:

    This routine is used to iterate through Fcbs in a tree.  In order to match
    the lockorder for getting multiple Fcbs (so this can be used for acquiring
    all Fcbs), this version does a bottom-up enumeration.

    This is different than the old one, now called TopDown. The problem with
    lockorder was very well hidden.

    The transition rule is still pretty simple:

        A) If you have an adjacent sibling, go to it
            1) Descend to its leftmost child
        B) Else go to your parent

    If this routine is called with in invalid TerminationFcb it will fail,
    badly.

    The TerminationFcb is the last Fcb returned in the enumeration.

    This method is incompatible with the possibility that ancestors may vanish
    based on operations done on the last returned node.  For instance,
    FatPurgeReferencedFileObjects cannot use BottomUp enumeration.

Arguments:

    Fcb - Supplies the current Fcb.  This is NULL if enumeration is starting.

    TerminationFcb - The root Fcb of the tree in which the enumeration starts
        and at which it inclusively stops.

Return Value:

    The next Fcb in the enumeration, or NULL if Fcb was the final one.

--*/

{
    PFCB NextFcb;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    NT_ASSERT( FatVcbAcquiredExclusive( IrpContext, TerminationFcb->Vcb ) ||
            FlagOn( TerminationFcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) );

    //
    //  Do we need to begin the enumeration?
    //

    if (Fcb != NULL) {

        //
        //  Did we complete?
        //

        if (Fcb == TerminationFcb) {

            return NULL;
        }

        //
        //  Do we have a sibling to return?
        //

        NextFcb = FatGetNextSibling( Fcb );

        //
        //  If not, return our parent.  We are done with this branch.
        //

        if (NextFcb == NULL) {

            return Fcb->ParentDcb;
        }

    } else {

        NextFcb = TerminationFcb;
    }

    //
    //  Decend to its furthest child (if it exists) and return it.
    //

    for (;
         NodeType( NextFcb ) != FAT_NTC_FCB && FatGetFirstChild( NextFcb ) != NULL;
         NextFcb = FatGetFirstChild( NextFcb )) {
    }

    return NextFcb;
}

PFCB
FatGetNextFcbTopDown (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    )

/*++

Routine Description:

    This routine is used to iterate through Fcbs in a tree, from the top down.

    The rule is very simple:

        A) If you have a child, go to it, else
        B) If you have an older sibling, go to it, else
        C) Go to your parent's older sibling.

    If this routine is called with in invalid TerminationFcb it will fail,
    badly.

    The Termination Fcb is never returned.  If it is the root of the tree you
    are traversing, visit it first.

    This routine never returns direct ancestors of Fcb, and thus is useful when
    making Fcb's go away (which may tear up the tree).

Arguments:

    Fcb - Supplies the current Fcb

    TerminationFcb - The Fcb at which the enumeration should (non-inclusivly)
        stop.  Assumed to be a directory.

Return Value:

    The next Fcb in the enumeration, or NULL if Fcb was the final one.

--*/

{
    PFCB Sibling;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    NT_ASSERT( FatVcbAcquiredExclusive( IrpContext, Fcb->Vcb ) ||
            FlagOn( Fcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) );

    //
    //  If this was a directory (ie. not a file), get the child.  If
    //  there aren't any children and this is our termination Fcb,
    //  return NULL.
    //

    if ( ((NodeType(Fcb) == FAT_NTC_DCB) ||
          (NodeType(Fcb) == FAT_NTC_ROOT_DCB)) &&
         !IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) ) {

        return FatGetFirstChild( Fcb );
    }

    //
    //  Were we only meant to do one iteration?
    //

    if ( Fcb == TerminationFcb ) {

        return NULL;
    }

    Sibling = FatGetNextSibling(Fcb);

    while (TRUE) {

        //
        //  Do we still have an "older" sibling in this directory who is
        //  not the termination Fcb?
        //

        if ( Sibling != NULL ) {

            return (Sibling != TerminationFcb) ? Sibling : NULL;
        }

        //
        //  OK, let's move on to out parent and see if he is the termination
        //  node or has any older siblings.
        //

        if ( Fcb->ParentDcb == TerminationFcb ) {

            return NULL;
        }

        Fcb = Fcb->ParentDcb;

        Sibling = FatGetNextSibling(Fcb);
    }
}


BOOLEAN
FatSwapVpb (
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine swaps the VPB for this VCB if it has not been done already.
    This means the device object will get our spare VPB and we will cleanup
    the one used while the volume was mounted.

Arguments:

    IrpContext - Supplies the context for the overall request.

    Vcb - Supplies the VCB to swap the VPB on.

Return Value:

    TRUE - If the VPB was actually swapped.

    FALSE - If the VPB was already swapped.

--*/

{
    BOOLEAN Result = FALSE;
    PVPB OldVpb;
    PIO_STACK_LOCATION IrpSp;


    //
    //  Make sure we have not already swapped it.
    //

    OldVpb = Vcb->Vpb;

#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable: 28175, "touching Vpb is ok for a filesystem" )
#endif

    if (!FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED ) && OldVpb->RealDevice->Vpb == OldVpb) {

        //
        //  If not the final reference and we are forcing the disconnect,
        //  then swap out the Vpb.  We must preserve the REMOVE_PENDING flag
        //  so that the device is not remounted in the middle of a PnP remove
        //  operation.
        //

        NT_ASSERT( Vcb->SwapVpb != NULL );

        Vcb->SwapVpb->Type = IO_TYPE_VPB;
        Vcb->SwapVpb->Size = sizeof( VPB );
        Vcb->SwapVpb->RealDevice = OldVpb->RealDevice;

        Vcb->SwapVpb->RealDevice->Vpb = Vcb->SwapVpb;

        Vcb->SwapVpb->Flags = FlagOn( OldVpb->Flags, VPB_REMOVE_PENDING );

        //
        //  If we are working on a mount request, we need to make sure we update
        //  the VPB in the IRP, since the one it points to may no longer be valid.
        //

        if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL && IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) {

            //
            //  Get the IRP stack.
            //

            IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

            NT_ASSERT( IrpSp->FileObject == NULL );

            //
            //  Check the VPB in the IRP to see if it is the one we are swapping.
            //

            if (IrpSp->Parameters.MountVolume.Vpb == OldVpb) {

                //
                //  Change the IRP to point to the swap VPB.
                //

                IrpSp->Parameters.MountVolume.Vpb = Vcb->SwapVpb;
            }
        }

#ifdef _MSC_VER
#pragma prefast( pop )
#endif

        //
        //  We place the volume in the Bad state (as opposed to NotMounted) so
        //  that it is not eligible for a remount.  Also indicate we used up
        //  the swap.
        //

        Vcb->SwapVpb = NULL;
        FatSetVcbCondition( Vcb, VcbBad);
        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED );

        Result = TRUE;
    }

    return Result;
}


_Requires_lock_held_(_Global_critical_region_)    
BOOLEAN
FatCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    IN BOOLEAN Force
    )

/*++

Routine Description:

    This routine determines if a volume is ready for deletion.  It
    correctly synchronizes with creates en-route to the file system.

Arguments:

    Vcb - Supplies the volume to examine

    Force - Specifies whether we want this Vcb forcibly disconnected
        from the driver stack if it will not be deleted (a new vpb will
        be installed if neccesary).  Caller is responsible for making
        sure that the volume has been marked in such a way that attempts
        to operate through the realdevice are blocked (i.e., move the vcb
        out of the mounted state).

Return Value:

    BOOLEAN - TRUE if the volume was deleted, FALSE otherwise.

--*/

{
    KIRQL SavedIrql;
    BOOLEAN VcbDeleted = FALSE;


    //
    //  If the VCB condition is good and we are not forcing, just return.
    //

    if (Vcb->VcbCondition == VcbGood && !Force) {

        return FALSE;
    }

    //
    //  Now check for a zero Vpb count on an unmounted volume.  These
    //  volumes will be deleted as they now have no file objects and
    //  there are no creates en route to this volume.
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if (Vcb->Vpb->ReferenceCount == Vcb->ResidualOpenCount && Vcb->OpenFileCount == 0) {

        PVPB Vpb = Vcb->Vpb;

#if DBG
        UNICODE_STRING VolumeLabel;

        //
        //  Setup the VolumeLabel string
        //

        VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
        VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

        KdPrintEx((DPFLTR_FASTFAT_ID,
                   DPFLTR_INFO_LEVEL,
                   "FASTFAT: Dismounting Volume %Z\n",
                   &VolumeLabel));
#endif // DBG

        //
        //  Swap this VCB's VPB.
        //

        FatSwapVpb( IrpContext,
                    Vcb );

        //
        //  Clear the VPB_MOUNTED bit.  New opens should not come in due
        //  to the swapped VPB, but having the flag cleared helps debugging.
        //  Note that we must leave the Vpb->DeviceObject field set until
        //  after the FatTearDownVcb call as closes will have to make their
        //  way back to us.
        //

        ClearFlag( Vpb->Flags, VPB_MOUNTED );

        //
        //  If this Vpb was locked, clear this flag now.
        //

        ClearFlag( Vpb->Flags, (VPB_LOCKED | VPB_DIRECT_WRITES_ALLOWED) );

        IoReleaseVpbSpinLock( SavedIrql );

        //
        //  We are going to attempt the dismount, so mark the VCB as having
        //  a dismount in progress.
        //

        NT_ASSERT( !FlagOn( Vcb->VcbState, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS ) );
        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS );

        //
        //  Close down our internal opens.
        //

        FatTearDownVcb( IrpContext,
                        Vcb );

        //
        //  Process any delayed closes.
        //

        FatFspClose( Vcb );

        //
        //  Grab the VPB lock again so that we can recheck our counts.
        //

        IoAcquireVpbSpinLock( &SavedIrql );

        //
        //  See if we can delete this VCB.
        //

        if (Vcb->Vpb->ReferenceCount == 0 && Vcb->InternalOpenCount == 0) {

            Vpb->DeviceObject = NULL;

            IoReleaseVpbSpinLock( SavedIrql );

            FatDeleteVcb( IrpContext, Vcb );

            IoDeleteDevice( (PDEVICE_OBJECT)
                            CONTAINING_RECORD( Vcb,
                                               VOLUME_DEVICE_OBJECT,
                                               Vcb ) );

            VcbDeleted = TRUE;

        } else {

            IoReleaseVpbSpinLock( SavedIrql );

            NT_ASSERT( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS ) );
            ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS );
        }

    } else if (Force) {

        //
        //  The requester is forcing the issue.  We need to swap the VPB with our spare.
        //

        FatSwapVpb( IrpContext,
                    Vcb );

        IoReleaseVpbSpinLock( SavedIrql );

    } else {

        //
        //  Just drop the Vpb spinlock.
        //

        IoReleaseVpbSpinLock( SavedIrql );
    }

    return VcbDeleted;
}


VOID
FatConstructNamesInFcb (
    IN PIRP_CONTEXT IrpContext,
    PFCB Fcb,
    PDIRENT Dirent,
    PUNICODE_STRING Lfn OPTIONAL
    )

/*++

Routine Description:

    This routine places the short name in the dirent in the first set of
    STRINGs in the Fcb.  If a long file name (Lfn) was specified, then
    we must decide whether we will store its Oem equivolent in the same
    prefix table as the short name, or rather just save the upcased
    version of the UNICODE string in the FCB.

    For looking up Fcbs, the first approach will be faster, so we want to
    do this as much as possible.  Here are the rules that I have thought
    through extensively to determine when it is safe to store only Oem
    version of the UNICODE name.

    - If the UNICODE name contains no extended characters (>0x80), use Oem.

    - Let U be the upcased version of the UNICODE name.
      Let Up(x) be the function that upcases a UNICODE string.
      Let Down(x) be the function that upcases a UNICODE string.
      Let OemToUni(x) be the function that converts an Oem string to Unicode.
      Let UniToOem(x) be the function that converts a Unicode string to Oem.
      Let BestOemFit(x) be the function that creates the Best uppercase Oem
        fit for the UNICODE string x.

      BestOemFit(x) = UniToOem(Up(OemToUni(UniToOem(x))))   <1>

      if (BestOemFit(U) == BestOemFit(Down(U))              <2>

          then I know that there exists no UNICODE string Y such that:

              Up(Y) == Up(U)                                <3>

              AND

              BestOemFit(U) != BestOemFit(Y)                <4>

      Consider string U as a collection of one character strings.  The
      conjecture is clearly true for each sub-string, thus it is true
      for the entire string.

      Equation <1> is what we use to convert an incoming unicode name in
      FatCommonCreate() to Oem.  The double conversion is done to provide
      better visual best fitting for characters in the Ansi code page but
      not in the Oem code page.  A single Nls routine is provided to do
      this conversion efficiently.

      The idea is that with U, I only have to worry about a case varient Y
      matching it in a unicode compare, and I have shown that any case varient
      of U (the set Y defined in equation <3>), when filtered through <1>
      (as in create), will match the Oem string defined in <1>.

      Thus I do not have to worry about another UNICODE string missing in
      the prefix lookup, but matching when comparing LFNs in the directory.

Arguments:

    Fcb - The Fcb we are supposed to fill in.  Note that ParentDcb must
        already be filled in.

    Dirent - The gives up the short name.

    Lfn - If provided, this gives us the long name.

Return Value:

    None

--*/

{
#ifndef __REACTOS__
    NTSTATUS Status;
#endif
    ULONG i;

#ifndef __REACTOS__
    OEM_STRING OemA;
    OEM_STRING OemB;
#endif
    UNICODE_STRING Unicode;
    POEM_STRING ShortName;
    POEM_STRING LongOemName;
    PUNICODE_STRING LongUniName;

    PAGED_CODE();

    ShortName = &Fcb->ShortName.Name.Oem;

    NT_ASSERT( ShortName->Buffer == NULL );

    _SEH2_TRY {

        //
        //  First do the short name.
        //

        //
        //  Copy over the case flags for the short name of the file
        //

        if (FlagOn(Dirent->NtByte, FAT_DIRENT_NT_BYTE_8_LOWER_CASE)) {

            SetFlag(Fcb->FcbState, FCB_STATE_8_LOWER_CASE);

        } else {

            ClearFlag(Fcb->FcbState, FCB_STATE_8_LOWER_CASE);
        }

        if (FlagOn(Dirent->NtByte, FAT_DIRENT_NT_BYTE_3_LOWER_CASE)) {

            SetFlag(Fcb->FcbState, FCB_STATE_3_LOWER_CASE);

        } else {

            ClearFlag(Fcb->FcbState, FCB_STATE_3_LOWER_CASE);
        }

        ShortName->MaximumLength = 16;
        ShortName->Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                      16,
                                                      TAG_FILENAME_BUFFER );

        Fat8dot3ToString( IrpContext, Dirent, FALSE, ShortName );

        Fcb->ShortName.FileNameDos = TRUE;

        //
        //  If no Lfn was specified, we are done.  In either case, set the
        //  final name length.
        //

        NT_ASSERT( Fcb->ExactCaseLongName.Buffer == NULL );

        if (!ARGUMENT_PRESENT(Lfn) || (Lfn->Length == 0)) {

            Fcb->FinalNameLength = (USHORT) RtlOemStringToCountedUnicodeSize( ShortName );
            Fcb->ExactCaseLongName.Length = Fcb->ExactCaseLongName.MaximumLength = 0;

            try_return( NOTHING );
        }

        //
        //  If we already set up the full filename, we could be in trouble.  If the fast
        //  path for doing it already fired, FatSetFullFileNameInFcb, it will have missed
        //  this and could have built the full filename out of the shortname of the file.
        //
        //  At that point, disaster could be inevitable since the final name length will not
        //  match.  We use this to tell the notify package what to do - FatNotifyReportChange.
        //

        NT_ASSERT( Fcb->FullFileName.Buffer == NULL );

        //
        //  We know now we have an Lfn, save away a copy.
        //

        Fcb->FinalNameLength = Lfn->Length;

        Fcb->ExactCaseLongName.Length = Fcb->ExactCaseLongName.MaximumLength = Lfn->Length;
        Fcb->ExactCaseLongName.Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                                  Lfn->Length,
                                                                  TAG_FILENAME_BUFFER );
        RtlCopyMemory(Fcb->ExactCaseLongName.Buffer, Lfn->Buffer, Lfn->Length);

        //
        //  First check for no extended characters.
        //

        for (i=0; i < Lfn->Length/sizeof(WCHAR); i++) {

            if (Lfn->Buffer[i] >= 0x80) {

                break;
            }
        }

        if (i == Lfn->Length/sizeof(WCHAR)) {

            //
            //  Cool, I can go with the Oem, upcase it fast by hand.
            //

            LongOemName = &Fcb->LongName.Oem.Name.Oem;


            LongOemName->Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                            Lfn->Length/sizeof(WCHAR),
                                                            TAG_FILENAME_BUFFER );
            LongOemName->Length =
            LongOemName->MaximumLength = Lfn->Length/sizeof(WCHAR);

            for (i=0; i < Lfn->Length/sizeof(WCHAR); i++) {

                WCHAR c;

                c = Lfn->Buffer[i];

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable:4244 )
#endif
                LongOemName->Buffer[i] = c < 'a' ?
                                         (UCHAR)c :
                                         c <= 'z' ?
                                         c - (UCHAR)('a'-'A') :
                                         (UCHAR)c;
#ifdef _MSC_VER
#pragma warning( pop )
#endif
            }

            //
            //  If this name happens to be exactly the same as the short
            //  name, don't add it to the splay table.
            //

            if (FatAreNamesEqual(IrpContext, *ShortName, *LongOemName) ||
                (FatFindFcb( IrpContext,
                             &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                             LongOemName,
                             NULL) != NULL)) {

                ExFreePool( LongOemName->Buffer );

                LongOemName->Buffer = NULL;
                LongOemName->Length =
                LongOemName->MaximumLength = 0;

            } else {

                SetFlag( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME );
            }

            try_return( NOTHING );
        }

        //
        //  Now we have the fun part.  Make a copy of the Lfn.
        //

#ifndef __REACTOS__
        OemA.Buffer = NULL;
        OemB.Buffer = NULL;
#endif
        Unicode.Buffer = NULL;

        Unicode.Length =
        Unicode.MaximumLength = Lfn->Length;
        Unicode.Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                   Lfn->Length,
                                                   TAG_FILENAME_BUFFER );

        RtlCopyMemory( Unicode.Buffer, Lfn->Buffer, Lfn->Length );

#ifndef __REACTOS__
        Status = STATUS_SUCCESS;
#endif

#if TRUE
        //
        //  Unfortunately, this next block of code breaks down when you have
        //  two long Unicode filenames that both map to the same Oem (and are,
        //  well, long, i.e. are not the short names). In this case, with one
        //  in the prefix table first, the other will hit the common Oem
        //  representation.  This leads to several forms of user astonishment.
        //
        //  It isn't worth it, or probably even possible, to try to figure out
        //  when this is really safe to go through.  Simply omit the attempt.
        //
        //  Ex: ANSI 0x82 and 0x84 in the 1252 ANSI->UNI and 437 UNI->OEM codepages.
        //
        //      0x82 => 0x201a => 0x2c
        //      0x84 => 0x201e => 0x2c
        //
        //  0x2c is comma, so is FAT Oem illegal and forces shortname generation.
        //  Since it is otherwise well-formed by the rules articulated previously,
        //  we would have put 0x2c in the Oem prefix tree.  In terms of the
        //  argument given above, even though there exist no Y and U s.t.
        //
        //  Up(Y) == Up(U) && BestOemFit(U) != BestOemFit(Y)
        //
        //  there most certainly exist Y and U s.t.
        //
        //  Up(Y) != Up(U) && BestOemFit(U) == BestOemFit(Y)
        //
        //  and that is enough to keep us from doing this.  Note that the < 0x80
        //  case is OK since we know that the mapping in the OEM codepages are
        //  the identity in that range.
        //
        //  We still need to monocase it, though.  Do this through a full down/up
        //  transition.
        //

        (VOID)RtlDowncaseUnicodeString( &Unicode, &Unicode, FALSE );
        (VOID)RtlUpcaseUnicodeString( &Unicode, &Unicode, FALSE );
#else
        //
        //  Downcase and convert to upcased Oem.  Only continue if we can
        //  convert without error.  Any error other than UNMAPPABLE_CHAR
        //  is a fatal error and we raise.
        //
        //  Note that even if the conversion fails, we must leave Unicode
        //  in an upcased state.
        //
        //  NB: The Rtl doesn't NULL .Buffer on error.
        //

        (VOID)RtlDowncaseUnicodeString( &Unicode, &Unicode, FALSE );
        Status = RtlUpcaseUnicodeStringToCountedOemString( &OemA, &Unicode, TRUE );
        (VOID)RtlUpcaseUnicodeString( &Unicode, &Unicode, FALSE );

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_UNMAPPABLE_CHARACTER) {

                NT_ASSERT( Status == STATUS_NO_MEMORY );
                ExFreePool(Unicode.Buffer);
                FatNormalizeAndRaiseStatus( IrpContext, Status );
            }

        } else {

            //
            //  The same as above except upcase.
            //

            Status = RtlUpcaseUnicodeStringToCountedOemString( &OemB, &Unicode, TRUE );

            if (!NT_SUCCESS(Status)) {

                RtlFreeOemString( &OemA );

                if (Status != STATUS_UNMAPPABLE_CHARACTER) {

                    NT_ASSERT( Status == STATUS_NO_MEMORY );
                    ExFreePool(Unicode.Buffer);
                    FatNormalizeAndRaiseStatus( IrpContext, Status );
                }
            }
        }

        //
        //  If the final OemNames are equal, I can use save only the Oem
        //  name.  If the name did not map, then I have to go with the UNICODE
        //  name because I could get a case varient that didn't convert
        //  in create, but did match the LFN.
        //

        if (NT_SUCCESS(Status) && FatAreNamesEqual( IrpContext, OemA, OemB )) {

            //
            //  Cool, I can go with the Oem.  If we didn't convert correctly,
            //  get a fresh convert from the original LFN.
            //

            ExFreePool(Unicode.Buffer);

            RtlFreeOemString( &OemB );

            Fcb->LongName.Oem.Name.Oem = OemA;

            //
            //  If this name happens to be exactly the same as the short
            //  name, or a similar short name already exists don't add it
            //  to the splay table (note the final condition implies a
            //  corrupt disk.
            //

            if (FatAreNamesEqual(IrpContext, *ShortName, OemA) ||
                (FatFindFcb( IrpContext,
                             &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                             &OemA,
                             NULL) != NULL)) {

                RtlFreeOemString( &OemA );

            } else {

                SetFlag( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME );
            }

            try_return( NOTHING );
        }

        //
        //  The long name must be left in UNICODE.  Free the two Oem strings
        //  if we got here just because they weren't equal.
        //

        if (NT_SUCCESS(Status)) {

            RtlFreeOemString( &OemA );
            RtlFreeOemString( &OemB );
        }
#endif

        LongUniName = &Fcb->LongName.Unicode.Name.Unicode;

        LongUniName->Length =
        LongUniName->MaximumLength = Unicode.Length;
        LongUniName->Buffer = Unicode.Buffer;

        SetFlag(Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME);

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        if (_SEH2_AbnormalTermination()) {

            if (ShortName->Buffer != NULL) {

                ExFreePool( ShortName->Buffer );
                ShortName->Buffer = NULL;
            }

        } else {

            //
            //  Creating all the names worked, so add all the names
            //  to the splay tree.
            //

            FatInsertName( IrpContext,
                           &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                           &Fcb->ShortName );

            Fcb->ShortName.Fcb = Fcb;

            if (FlagOn(Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME)) {

                FatInsertName( IrpContext,
                               &Fcb->ParentDcb->Specific.Dcb.RootOemNode,
                               &Fcb->LongName.Oem );

                Fcb->LongName.Oem.Fcb = Fcb;
            }

            if (FlagOn(Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME)) {

                FatInsertName( IrpContext,
                               &Fcb->ParentDcb->Specific.Dcb.RootUnicodeNode,
                               &Fcb->LongName.Unicode );

                Fcb->LongName.Unicode.Fcb = Fcb;
            }

            SetFlag(Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE);
        }
    } _SEH2_END;

    return;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatCheckFreeDirentBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine checks if the size of the free dirent bitmap is
    sufficient to for the current directory size.  It is called
    whenever we grow a directory.

Arguments:

    Dcb -  Supplies the directory in question.

Return Value:

    None

--*/

{
    ULONG OldNumberOfDirents;
    ULONG NewNumberOfDirents;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Setup the Bitmap buffer if it is not big enough already
    //

    NT_ASSERT( Dcb->Header.AllocationSize.QuadPart != FCB_LOOKUP_ALLOCATIONSIZE_HINT );

    OldNumberOfDirents = Dcb->Specific.Dcb.FreeDirentBitmap.SizeOfBitMap;
    NewNumberOfDirents = Dcb->Header.AllocationSize.LowPart / sizeof(DIRENT);

    //
    //  Do the usual unsync/sync check.
    //

    if (NewNumberOfDirents > OldNumberOfDirents) {

        FatAcquireDirectoryFileMutex( Dcb->Vcb );

        _SEH2_TRY {

            PULONG OldBitmapBuffer;
            PULONG BitmapBuffer;

            ULONG BytesInBitmapBuffer;
            ULONG BytesInOldBitmapBuffer;

            OldNumberOfDirents = Dcb->Specific.Dcb.FreeDirentBitmap.SizeOfBitMap;
            NewNumberOfDirents = Dcb->Header.AllocationSize.LowPart / sizeof(DIRENT);

            if (NewNumberOfDirents > OldNumberOfDirents) {

                //
                //  Remember the old bitmap
                //

                OldBitmapBuffer = Dcb->Specific.Dcb.FreeDirentBitmap.Buffer;

                //
                //  Now make a new bitmap bufffer
                //

                BytesInBitmapBuffer = NewNumberOfDirents / 8;

                BytesInOldBitmapBuffer = OldNumberOfDirents / 8;

                if (DCB_UNION_SLACK_SPACE >= BytesInBitmapBuffer) {

                    BitmapBuffer = &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0];

                } else {

                    BitmapBuffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                             BytesInBitmapBuffer,
                                                             TAG_DIRENT_BITMAP );
                }

                //
                //  Copy the old buffer to the new buffer, free the old one, and zero
                //  the rest of the new one.  Only do the first two steps though if
                //  we moved out of the initial buffer.
                //

                if ((OldNumberOfDirents != 0) &&
                    (BitmapBuffer != &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0])) {

                    RtlCopyMemory( BitmapBuffer,
                                   OldBitmapBuffer,
                                   BytesInOldBitmapBuffer );

                    if (OldBitmapBuffer != &Dcb->Specific.Dcb.FreeDirentBitmapBuffer[0]) {

                        ExFreePool( OldBitmapBuffer );
                    }
                }

                NT_ASSERT( BytesInBitmapBuffer > BytesInOldBitmapBuffer );

                RtlZeroMemory( (PUCHAR)BitmapBuffer + BytesInOldBitmapBuffer,
                               BytesInBitmapBuffer - BytesInOldBitmapBuffer );

                //
                //  Now initialize the new bitmap.
                //

                RtlInitializeBitMap( &Dcb->Specific.Dcb.FreeDirentBitmap,
                                     BitmapBuffer,
                                     NewNumberOfDirents );
            }

        } _SEH2_FINALLY {

            FatReleaseDirectoryFileMutex( Dcb->Vcb );
        } _SEH2_END;
    }
}


BOOLEAN
FatIsHandleCountZero (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine decides if the handle count on the volume is zero.

Arguments:

    Vcb - The volume in question

Return Value:

    BOOLEAN - TRUE if there are no open handles on the volume, FALSE
              otherwise.

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = Vcb->RootDcb;

    while (Fcb != NULL) {

        if (Fcb->UncleanCount != 0) {

            return FALSE;
        }

        Fcb = FatGetNextFcbTopDown(IrpContext, Fcb, Vcb->RootDcb);
    }

    return TRUE;
}


PCLOSE_CONTEXT

FatAllocateCloseContext(
    OPTIONAL PVCB Vcb
    )
/*++

Routine Description:

    This routine preallocates a close context, presumeably on behalf
    of a fileobject which does not have a structure we can embed one
    in.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER( Vcb );

#if DBG
    if (ARGUMENT_PRESENT(Vcb)) {

        NT_ASSERT( 0 != Vcb->CloseContextCount);
        InterlockedDecrement( (LONG*)&Vcb->CloseContextCount);
    }
#endif
    return (PCLOSE_CONTEXT)ExInterlockedPopEntrySList( &FatCloseContextSList,
                                                       &FatData.GeneralSpinLock );
}


VOID
FatPreallocateCloseContext (
    PVCB Vcb
    )

/*++

Routine Description:

    This routine preallocates a close context, presumeably on behalf
    of a fileobject which does not have a structure we can embed one
    in.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PCLOSE_CONTEXT CloseContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Vcb );

    CloseContext = FsRtlAllocatePoolWithTag( PagedPool,
                                             sizeof(CLOSE_CONTEXT),
                                             TAG_FAT_CLOSE_CONTEXT );

    ExInterlockedPushEntrySList( &FatCloseContextSList,
                                 (PSLIST_ENTRY) CloseContext,
                                 &FatData.GeneralSpinLock );

    DbgDoit( InterlockedIncrement( (LONG*)&Vcb->CloseContextCount));
}



VOID
FatEnsureStringBufferEnough (
    _Inout_ PVOID String,
    _In_ USHORT DesiredBufferSize
    )

/*++

Routine Description:

    Ensures that a string string (STRING, UNICODE_STRING, ANSI_STRING, OEM_STRING)
    has a buffer >= DesiredBufferSize,  allocating from pool if neccessary.  Any
    existing pool buffer will be freed if a new one is allocated.

    NOTE: No copy of old buffer contents is performed on reallocation.

    Will raise on allocation failure.

Arguments:

    String - pointer to string structure

    DesiredBufferSize - (bytes) minimum required buffer size

--*/

{
    PSTRING LocalString = String;

    PAGED_CODE();

    if (LocalString->MaximumLength < DesiredBufferSize)  {

        FatFreeStringBuffer( LocalString);

        LocalString->Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                        DesiredBufferSize,
                                                        TAG_DYNAMIC_NAME_BUFFER);
        NT_ASSERT( LocalString->Buffer);

        LocalString->MaximumLength = DesiredBufferSize;
    }
}


VOID
FatFreeStringBuffer (
    _Inout_ PVOID String
    )

/*++

Routine Description:

    Frees the buffer of an string (STRING, UNICODE_STRING, ANSI_STRING, OEM_STRING)
    structure if it is not within the current thread's stack limits.

    Regardless of action performed,  on exit String->Buffer will be set to NULL and
    String->MaximumLength to zero.

Arguments:

    String - pointer to string structure

--*/

{
    ULONG_PTR High, Low;
    PSTRING LocalString = String;

    PAGED_CODE();

    if (NULL != LocalString->Buffer)  {

        IoGetStackLimits( &Low, &High );

        if (((ULONG_PTR)(LocalString->Buffer) < Low) ||
            ((ULONG_PTR)(LocalString->Buffer) > High))  {

            ExFreePool( LocalString->Buffer);
        }

        LocalString->Buffer = NULL;
    }

    LocalString->MaximumLength = LocalString->Length = 0;
}


BOOLEAN
FatScanForDataTrack(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called to verify and process the TOC for this disk.

    FAT queries for the TOC to avoid trying to mount on CD-DA/CD-E media,  Doing data reads on
    audio/leadin of that media sends a lot of drives into what could charitably be called
    "conniptions" which take a couple seconds to clear and would also convince FAT that the
    device was busted, and fail the mount (not letting CDFS get its crack).

    There is special handling of PD media.  These things fail the TOC read, but return
    a special error code so FAT knows to continue to try the mount anyway.

Arguments:

    TargetDeviceObject - Device object to send TOC request to.

Return Value:

    BOOLEAN - TRUE if we found a TOC with a single data track.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    ULONG LocalTrackCount;
    ULONG LocalTocLength;

    PCDROM_TOC CdromToc;
    BOOLEAN Result = FALSE;

    PAGED_CODE();

    CdromToc = FsRtlAllocatePoolWithTag( PagedPool,
                                         sizeof( CDROM_TOC ),
                                         TAG_IO_BUFFER );

    RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

    _SEH2_TRY {

        //
        //  Go ahead and read the table of contents
        //

        Status = FatPerformDevIoCtrl( IrpContext,
                                     IOCTL_CDROM_READ_TOC,
                                     TargetDeviceObject,
                                     NULL,
                                     0,
                                     CdromToc,
                                     sizeof( CDROM_TOC ),
                                     FALSE,
                                     TRUE,
                                     &Iosb );

        //
        //  Nothing to process if this request fails.
        //

        if (Status != STATUS_SUCCESS) {

            //
            //  If we get the special error indicating a failed TOC read on PD media just
            //  plow ahead with the mount (see comments above).
            //

            if ((Status == STATUS_IO_DEVICE_ERROR) || (Status == STATUS_INVALID_DEVICE_REQUEST)) {

                Result = TRUE;

            }

            try_leave( NOTHING );
        }

        //
        //  Get the number of tracks and stated size of this structure.
        //

        LocalTrackCount = CdromToc->LastTrack - CdromToc->FirstTrack + 1;
        LocalTocLength = PtrOffset( CdromToc, &CdromToc->TrackData[LocalTrackCount + 1] );

        //
        //  Get out if there is an immediate problem with the TOC,  or more than
        //  one track.
        //

        if ((LocalTocLength > Iosb.Information) ||
            (CdromToc->FirstTrack > CdromToc->LastTrack) ||
            (LocalTrackCount != 1)) {

            try_leave( NOTHING);
        }

        //
        //  Is it a data track?  DVD-RAM reports single,  data,  track.
        //

        Result = BooleanFlagOn( CdromToc->TrackData[ 0].Control, 0x04 );
    }
    _SEH2_FINALLY {

        ExFreePool( CdromToc);
    } _SEH2_END;

    return Result;
}


