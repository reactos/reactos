/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fssup.c

Abstract:

    This module implements the File System support routines for the
    Cache subsystem.

Author:

    Tom Miller      [TomM]      4-May-1990

Revision History:

--*/

#include "cc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CACHE_BUG_CHECK_FSSUP)

//
//  Define our debug constant
//

#define me 0x00000001

//
//  For your debugging pleasure, if the flag doesn't move!  (Currently not used)
//

#define IsSyscacheFile(FO) (((FO) != NULL) &&                                               \
                            (*(PUSHORT)(FO)->FsContext == 0X705) &&                         \
                            FlagOn(*(PULONG)((PCHAR)(FO)->FsContext + 0x48), 0x80000000))

extern POBJECT_TYPE IoFileObjectType;
extern ULONG MmLargeSystemCache;

VOID
CcUnmapAndPurge(
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

VOID
CcDeleteMbcb(
    IN PSHARED_CACHE_MAP SharedCacheMap
    );

VOID
CcPurgeAndClearCacheSection (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CcInitializeCacheManager)
#endif


BOOLEAN
CcInitializeCacheManager (
    )

/*++

Routine Description:

    This routine must be called during system initialization before the
    first call to any file system, to allow the Cache Manager to initialize
    its global data structures.  This routine has no dependencies on other
    system components being initialized.

Arguments:

    None

Return Value:

    TRUE if initialization was successful

--*/

{
    CLONG i;
    ULONG Index;
    PNPAGED_LOOKASIDE_LIST Lookaside;
    USHORT NumberOfItems;
    PKPRCB Prcb;
    PWORK_QUEUE_ITEM WorkItem;

#ifdef CCDBG_LOCK
    KeInitializeSpinLock( &CcDebugTraceLock );
#endif

#if DBG
    CcBcbCount = 0;
    InitializeListHead( &CcBcbList );
    KeInitializeSpinLock( &CcBcbSpinLock );
#endif

    //
    //  Figure out the timeout clock tick for the lazy writer.
    //

    CcIdleDelayTick = LAZY_WRITER_IDLE_DELAY / KeQueryTimeIncrement();
    
    //
    //  Initialize shared cache map list structures
    //

    KeInitializeSpinLock( &CcMasterSpinLock );
    InitializeListHead( &CcCleanSharedCacheMapList );
    InitializeListHead( &CcDirtySharedCacheMapList.SharedCacheMapLinks );
    CcDirtySharedCacheMapList.Flags = IS_CURSOR;
    InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                    &CcLazyWriterCursor.SharedCacheMapLinks );
    CcLazyWriterCursor.Flags = IS_CURSOR;

    //
    //  Initialize worker thread structures
    //

    KeInitializeSpinLock( &CcWorkQueueSpinlock );
    InitializeListHead( &CcIdleWorkerThreadList );
    InitializeListHead( &CcExpressWorkQueue );
    InitializeListHead( &CcRegularWorkQueue );
    InitializeListHead( &CcPostTickWorkQueue );

    //
    //  Set the number of worker threads based on the system size.
    //

    CcCapturedSystemSize = MmQuerySystemSize();
    if (CcNumberWorkerThreads == 0) {

        switch (CcCapturedSystemSize) {
        case MmSmallSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 8;
            CcAggressiveZeroThreshold = 1;
            break;

        case MmMediumSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 4;
            CcAggressiveZeroThreshold = 2;
            break;

        case MmLargeSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 2;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 4 +
                                    MmNumberOfPhysicalPages / 8;
            CcAggressiveZeroThreshold = 4;
#if 0
            //
            //  Use more memory if we are a large server.
            //

            if ((MmLargeSystemCache != 0) &&
                (CcDirtyPageThreshold < (MmNumberOfPhysicalPages - (0xE00000 / PAGE_SIZE)))) {

                CcDirtyPageThreshold = MmNumberOfPhysicalPages - (0xE00000 / PAGE_SIZE);
            }
#endif
            break;

        default:
            CcNumberWorkerThreads = 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 8;
        }

//        CcDirtyPageThreshold = (2*1024*1024)/PAGE_SIZE;

        if (MmSystemCacheWs.MaximumWorkingSetSize > ((4*1024*1024)/PAGE_SIZE)) {
            CcDirtyPageThreshold = MmSystemCacheWs.MaximumWorkingSetSize -
                                                    ((2*1024*1024)/PAGE_SIZE);
        }

        CcDirtyPageTarget = CcDirtyPageThreshold / 2 +
                            CcDirtyPageThreshold / 4;
    }

    CcAggressiveZeroCount = 0;

    //
    //  Now allocate and initialize the above number of worker thread
    //  items.
    //

    for (i = 0; i < CcNumberWorkerThreads; i++) {

        WorkItem = ExAllocatePoolWithTag( NonPagedPool, sizeof(WORK_QUEUE_ITEM), 'qWcC' );

        if (WorkItem == NULL) {
            
            CcBugCheck( 0, 0, 0 );
        }

        //
        //  Initialize the work queue item and insert in our queue
        //  of potential worker threads.
        //

        ExInitializeWorkItem( WorkItem, CcWorkerThread, WorkItem );
        InsertTailList( &CcIdleWorkerThreadList, &WorkItem->List );
    }

    //
    //  Initialize the Lazy Writer thread structure, and start him up.
    //

    RtlZeroMemory( &LazyWriter, sizeof(LAZY_WRITER) );

    KeInitializeSpinLock( &CcWorkQueueSpinlock );
    InitializeListHead( &LazyWriter.WorkQueue );

    //
    //  Store process address
    //

    LazyWriter.OurProcess = PsGetCurrentProcess();

    //
    //  Initialize the Scan Dpc and Timer.
    //

    KeInitializeDpc( &LazyWriter.ScanDpc, &CcScanDpc, NULL );
    KeInitializeTimer( &LazyWriter.ScanTimer );

    //
    //  Now initialize the lookaside list for allocating Work Queue entries.
    //

    switch ( CcCapturedSystemSize ) {

        //
        // ~512 bytes
        //

    case MmSmallSystem :
        NumberOfItems = 32;
        break;

        //
        // ~1k bytes
        //

    case MmMediumSystem :
        NumberOfItems = 64;
        break;

        //
        // ~2k bytes
        //

    case MmLargeSystem :
        NumberOfItems = 128;
        if (MmIsThisAnNtAsSystem()) {
            NumberOfItems += 128;
        }

        break;
    }

    ExInitializeNPagedLookasideList( &CcTwilightLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( WORK_QUEUE_ENTRY ),
                                     'kWcC',
                                     NumberOfItems );

    //
    // Initialize the per processor nonpaged lookaside lists and descriptors.
    //

    for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
        Prcb = KiProcessorBlock[Index];

        //
        // Initialize the large IRP per processor lookaside pointers.
        //

        Prcb->PPLookasideList[LookasideTwilightList].L = &CcTwilightLookasideList;
        Lookaside = (PNPAGED_LOOKASIDE_LIST)ExAllocatePoolWithTag( NonPagedPool,
                                                                   sizeof(NPAGED_LOOKASIDE_LIST),
                                                                   'KWcC');

        if (Lookaside != NULL) {
            ExInitializeNPagedLookasideList( Lookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             sizeof( WORK_QUEUE_ENTRY ),
                                             'KWcC',
                                             NumberOfItems );

        } else {
            Lookaside = &CcTwilightLookasideList;
        }

        Prcb->PPLookasideList[LookasideTwilightList].P = Lookaside;
    }

    //
    //  Now initialize the Bcb zone
    //

    {
        PVOID InitialSegment;
        ULONG InitialSegmentSize;
        ULONG RoundedBcbSize = (sizeof(BCB) + 7) & ~7;

        switch ( CcCapturedSystemSize ) {

                //
                // ~1.5k bytes
                //

            case MmSmallSystem :
                InitialSegmentSize = sizeof(ZONE_SEGMENT_HEADER) + RoundedBcbSize * 8;
                break;

                //
                // 1 Page
                //

            case MmMediumSystem :
                InitialSegmentSize = PAGE_SIZE;
                break;

                //
                // 3 Pages
                //

            case MmLargeSystem :
                InitialSegmentSize = 3 * PAGE_SIZE;
                break;
            }

        //
        //  Allocate the initial allocation for the zone.  If we cannot get it,
        //  something must really be wrong, so we will just bugcheck.
        //

        if ((InitialSegment = ExAllocatePoolWithTag( NonPagedPool,
                                                     InitialSegmentSize,
                                                     'zBcC' )) == NULL) {

            CcBugCheck( 0, 0, 0 );
        }

        if (!NT_SUCCESS(ExInitializeZone( &LazyWriter.BcbZone,
                                          RoundedBcbSize,
                                          InitialSegment,
                                          InitialSegmentSize ))) {
            CcBugCheck( 0, 0, 0 );
        }
    }

    //
    //  Initialize the Deferred Write List.
    //

    KeInitializeSpinLock( &CcDeferredWriteSpinLock );
    InitializeListHead( &CcDeferredWrites );

    //
    //  Initialize the Vacbs.
    //

    CcInitializeVacbs();

    return TRUE;
}


VOID
CcInitializeCacheMap (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes,
    IN BOOLEAN PinAccess,
    IN PCACHE_MANAGER_CALLBACKS Callbacks,
    IN PVOID LazyWriteContext
    )

/*++

Routine Description:

    This routine is intended to be called by File Systems only.  It
    initializes the cache maps for data caching.  It should be called
    every time a file is open or created, and NO_INTERMEDIATE_BUFFERING
    was specified as FALSE.

Arguments:

    FileObject - A pointer to the newly-created file object.

    FileSizes - A pointer to AllocationSize, FileSize and ValidDataLength
                for the file.  ValidDataLength should contain MAXLONGLONG if
                valid data length tracking and callbacks are not desired.

    PinAccess - FALSE if file will be used exclusively for Copy and Mdl
                access, or TRUE if file will be used for Pin access.
                (Files for Pin access are not limited in size as the caller
                must access multiple areas of the file at once.)

    Callbacks - Structure of callbacks used by the Lazy Writer

    LazyWriteContext - Parameter to be passed in to above routine.

Return Value:

    None.  If an error occurs, this routine will Raise the status.

--*/

{
    KIRQL OldIrql;
    PSHARED_CACHE_MAP SharedCacheMap = NULL;
    PVOID CacheMapToFree = NULL;
    CC_FILE_SIZES LocalSizes;
    BOOLEAN WeSetBeingCreated = FALSE;
    BOOLEAN SharedListOwned = FALSE;
    BOOLEAN MustUninitialize = FALSE;
    BOOLEAN WeCreated = FALSE;

    DebugTrace(+1, me, "CcInitializeCacheMap:\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace( 0, me, "    FileSizes = %08lx\n", FileSizes );

    //
    //  Make a local copy of the passed in file sizes before acquiring
    //  the spin lock.
    //

    LocalSizes = *FileSizes;

    //
    //  If no FileSize was given, set to one byte before maximizing below.
    //

    if (LocalSizes.AllocationSize.QuadPart == 0) {
        LocalSizes.AllocationSize.LowPart += 1;
    }

    //
    //  If caller has Write access or will allow write, then round
    //  size to next create modulo.  (***Temp*** there may be too many
    //  apps that end up allowing shared write, thanks to our Dos heritage,
    //  to keep that part of the check in.)
    //

    if (FileObject->WriteAccess /*|| FileObject->SharedWrite */) {

        LocalSizes.AllocationSize.QuadPart = LocalSizes.AllocationSize.QuadPart + (LONGLONG)(DEFAULT_CREATE_MODULO - 1);
        LocalSizes.AllocationSize.LowPart &= ~(DEFAULT_CREATE_MODULO - 1);

    } else {

        LocalSizes.AllocationSize.QuadPart = LocalSizes.AllocationSize.QuadPart + (LONGLONG)(VACB_MAPPING_GRANULARITY - 1);
        LocalSizes.AllocationSize.LowPart &= ~(VACB_MAPPING_GRANULARITY - 1);
    }

    //
    //  Do the allocate of the SharedCacheMap, based on an unsafe test,
    //  while not holding a spinlock.  Allocation failures look like we
    //  never decided to allocate one here!
    //

    if (FileObject->SectionObjectPointer->SharedCacheMap == NULL) {
        CacheMapToFree = ExAllocatePoolWithTag( NonPagedPool, sizeof(SHARED_CACHE_MAP), 'cScC' );
    }

    //
    //  Serialize Creation/Deletion of all Shared CacheMaps
    //

    CcAcquireMasterLock( &OldIrql );
    SharedListOwned = TRUE;

    //
    //  Insure release of our global resource
    //

    try {

        //
        //  Check for second initialization of same file object
        //

        if (FileObject->PrivateCacheMap != NULL) {

            DebugTrace( 0, 0, "CacheMap already initialized\n", 0 );
            try_return( NOTHING );
        }

        //
        //  Get current Shared Cache Map pointer indirectly off of the file object.
        //  (The actual pointer is typically in a file system data structure, such
        //  as an Fcb.)
        //

        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

        //
        //  If there is no SharedCacheMap, then we must create a section and
        //  the SharedCacheMap structure.
        //

        if (SharedCacheMap == NULL) {

            //
            //  After successfully creating the section, allocate the SharedCacheMap.
            //

            WeCreated = TRUE;

            if (CacheMapToFree == NULL) {
                CacheMapToFree = (PSHARED_CACHE_MAP)ExAllocatePoolWithTag( NonPagedPool,
                                                                           sizeof(SHARED_CACHE_MAP),
                                                                           'cScC' );
            }

            SharedCacheMap = CacheMapToFree;
            CacheMapToFree = NULL;

            if (SharedCacheMap == NULL) {

                DebugTrace( 0, 0, "Failed to allocate SharedCacheMap\n", 0 );

                CcReleaseMasterLock( OldIrql );
                SharedListOwned = FALSE;

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Zero the SharedCacheMap and fill in the nonzero portions later.
            //

            RtlZeroMemory( SharedCacheMap, sizeof(SHARED_CACHE_MAP) );

#if DANLO
            SharedCacheMap->OpenCountLog.Size = sizeof(SharedCacheMap->OpenCountLog.Log)/sizeof(CC_LOG_ENTRY);
#endif

            //
            //  Now initialize the Shared Cache Map.
            //

            SharedCacheMap->NodeTypeCode = CACHE_NTC_SHARED_CACHE_MAP;
            SharedCacheMap->NodeByteSize = sizeof(SHARED_CACHE_MAP);
            SharedCacheMap->FileObject = FileObject;
            SharedCacheMap->FileSize = LocalSizes.FileSize;
            SharedCacheMap->ValidDataLength = LocalSizes.ValidDataLength;
            SharedCacheMap->ValidDataGoal = LocalSizes.ValidDataLength;
            //  SharedCacheMap->Section set below

            //
            //  Initialize the spin locks.
            //

            KeInitializeSpinLock( &SharedCacheMap->ActiveVacbSpinLock );
            KeInitializeSpinLock( &SharedCacheMap->BcbSpinLock );

            if (PinAccess) {
                SetFlag(SharedCacheMap->Flags, PIN_ACCESS);
            }

            //
            //  Initialize our allocation hint for the local event.
            //

            SharedCacheMap->LocalEvent = &SharedCacheMap->Event;

            //
            //  If this file has FO_SEQUENTIAL_ONLY set, then remember that
            //  in the SharedCacheMap.
            //

            if (FlagOn(FileObject->Flags, FO_SEQUENTIAL_ONLY)) {
                SetFlag(SharedCacheMap->Flags, ONLY_SEQUENTIAL_ONLY_SEEN);
            }

            //
            //  Do the round-robin allocation of the spinlock for the shared
            //  cache map.  Note the manipulation of the next
            //  counter is safe, since we have the CcMasterSpinLock
            //  exclusive.
            //

            InitializeListHead( &SharedCacheMap->BcbList );
            SharedCacheMap->Callbacks = Callbacks;
            SharedCacheMap->LazyWriteContext = LazyWriteContext;

            //
            //  Initialize listhead for all PrivateCacheMaps
            //

            InitializeListHead( &SharedCacheMap->PrivateList );

            //
            //  Insert the new Shared Cache Map in the global list
            //

            InsertTailList( &CcCleanSharedCacheMapList,
                            &SharedCacheMap->SharedCacheMapLinks );

            //
            //  Finally, store the pointer to the Shared Cache Map back
            //  via the indirect pointer in the File Object.
            //

            FileObject->SectionObjectPointer->SharedCacheMap = SharedCacheMap;

            //
            //  We must reference this file object so that it cannot go away
            //  until we do CcUninitializeCacheMap below.  Note we cannot
            //  find or rely on the FileObject that Memory Management has,
            //  although normally it will be this same one anyway.
            //

            ObReferenceObject ( FileObject );

        } else {

            //
            //  If this file has FO_SEQUENTIAL_ONLY clear, then remember that
            //  in the SharedCacheMap.
            //

            if (!FlagOn(FileObject->Flags, FO_SEQUENTIAL_ONLY)) {
                ClearFlag(SharedCacheMap->Flags, ONLY_SEQUENTIAL_ONLY_SEEN);
            }
        }

        //
        //  If this file is opened for random access, remember this in
        //  the SharedCacheMap.
        //

        if (FlagOn(FileObject->Flags, FO_RANDOM_ACCESS)) {
            SetFlag(SharedCacheMap->Flags, RANDOM_ACCESS_SEEN);
        }

        //
        //  Make sure that no one is trying to lazy delete it in the case
        //  that the Cache Map was already there.
        //

        ClearFlag(SharedCacheMap->Flags, TRUNCATE_REQUIRED);

        //
        //  In case there has been a CcUnmapAndPurge call, we check here if we
        //  if we need to recreate the section and map it.
        //

        if ((SharedCacheMap->Vacbs == NULL) &&
            !FlagOn(SharedCacheMap->Flags, BEING_CREATED)) {

            //
            //  Increment the OpenCount on the CacheMap.
            //

            CcIncrementOpenCount( SharedCacheMap, 'onnI' );
            MustUninitialize = TRUE;

            //
            //  We still want anyone else to wait.
            //

            SetFlag(SharedCacheMap->Flags, BEING_CREATED);
            WeSetBeingCreated = TRUE;

            //
            //  If there is a create event, then this must be the path where we
            //  we were only unmapped.  We will just clear it here again in case
            //  someone needs to wait again this time too.
            //

            if (SharedCacheMap->CreateEvent != NULL) {

                KeInitializeEvent( SharedCacheMap->CreateEvent,
                                   NotificationEvent,
                                   FALSE );
            }

            //
            //  Release global resource
            //

            CcReleaseMasterLock( OldIrql );
            SharedListOwned = FALSE;

            //
            //  We have to test this, because the section may only be unmapped.
            //

            if (SharedCacheMap->Section == NULL) {

                LARGE_INTEGER LargeZero = {0,0};

                //
                //  Call MM to create a section for this file, for the calculated
                //  section size.  Note that we have the choice in this service to
                //  pass in a FileHandle or a FileObject pointer, but not both.
                //  Naturally we want to pass in the handle.
                //

                DebugTrace( 0, mm, "MmCreateSection:\n", 0 );
                DebugTrace2(0, mm, "    MaximumSize = %08lx, %08lx\n",
                            LocalSizes.AllocationSize.LowPart,
                            LocalSizes.AllocationSize.HighPart );
                DebugTrace( 0, mm, "    FileObject = %08lx\n", FileObject );

                SharedCacheMap->Status = MmCreateSection( &SharedCacheMap->Section,
                                                          SECTION_MAP_READ
                                                            | SECTION_MAP_WRITE
                                                            | SECTION_QUERY,
                                                          NULL,
                                                          &LocalSizes.AllocationSize,
                                                          PAGE_READWRITE,
                                                          SEC_COMMIT,
                                                          NULL,
                                                          FileObject );

                DebugTrace( 0, mm, "    <Section = %08lx\n", SharedCacheMap->Section );

                if (!NT_SUCCESS( SharedCacheMap->Status )){
                    DebugTrace( 0, 0, "Error from MmCreateSection = %08lx\n",
                                SharedCacheMap->Status );

                    SharedCacheMap->Section = NULL;
                    ExRaiseStatus( FsRtlNormalizeNtstatus( SharedCacheMap->Status,
                                                           STATUS_UNEXPECTED_MM_CREATE_ERR ));
                }

                ObDeleteCapturedInsertInfo(SharedCacheMap->Section);

                //
                //  If this is a stream file object, then no user can map it,
                //  and we should keep the modified page writer out of it.
                //

                if (!FlagOn(((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->Flags2,
                            FSRTL_FLAG2_DO_MODIFIED_WRITE) &&
                    (FileObject->FsContext2 == NULL)) {

                    BOOLEAN Disabled;

                    Disabled = MmDisableModifiedWriteOfSection( FileObject->SectionObjectPointer );
                    CcAcquireMasterLock( &OldIrql );
                    SetFlag(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED);
                    CcReleaseMasterLock( OldIrql );

                    //**** ASSERT( Disabled );
                }

                //
                //  Create the Vacb array.
                //

                CcCreateVacbArray( SharedCacheMap, LocalSizes.AllocationSize );
            }

            //
            //  If the section already exists, we still have to call MM to
            //  extend, in case it is not large enough.
            //

            else {

                if ( LocalSizes.AllocationSize.QuadPart > SharedCacheMap->SectionSize.QuadPart ) {

                    NTSTATUS Status;

                    DebugTrace( 0, mm, "MmExtendSection:\n", 0 );
                    DebugTrace( 0, mm, "    Section = %08lx\n", SharedCacheMap->Section );
                    DebugTrace2(0, mm, "    Size = %08lx, %08lx\n",
                                LocalSizes.AllocationSize.LowPart,
                                LocalSizes.AllocationSize.HighPart );

                    Status = MmExtendSection( SharedCacheMap->Section,
                                              &LocalSizes.AllocationSize,
                                              TRUE );

                    if (!NT_SUCCESS(Status)) {

                        DebugTrace( 0, 0, "Error from MmExtendSection, Status = %08lx\n",
                                    Status );

                        ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                               STATUS_UNEXPECTED_MM_EXTEND_ERR ));
                    }
                }

                //
                //  Extend the Vacb array.
                //

                CcExtendVacbArray( SharedCacheMap, LocalSizes.AllocationSize );
            }

            //
            //  Now show that we are all done and resume any waiters.
            //

            CcAcquireMasterLock( &OldIrql );
            ClearFlag(SharedCacheMap->Flags, BEING_CREATED);
            WeSetBeingCreated = FALSE;
            if (SharedCacheMap->CreateEvent != NULL) {
                KeSetEvent( SharedCacheMap->CreateEvent, 0, FALSE );
            }
            CcReleaseMasterLock( OldIrql );
        }

        //
        //  Else if the section is already there, we make sure it is large
        //  enough by calling CcExtendCacheSection.
        //

        else {

            //
            //  If the SharedCacheMap is currently being created we have
            //  to optionally create and wait on an event for it.  Note that
            //  the only safe time to delete the event is in
            //  CcUninitializeCacheMap, because we otherwise have no way of
            //  knowing when everyone has reached the KeWaitForSingleObject.
            //

            if (FlagOn(SharedCacheMap->Flags, BEING_CREATED)) {

                if (SharedCacheMap->CreateEvent == NULL) {

                    //
                    //  If the local event is not being used then we can grab it.
                    //  (Should be quite rare that it is in use.)
                    //

                    SharedCacheMap->CreateEvent = InterlockedExchangePointer( &SharedCacheMap->LocalEvent, NULL );
                    
                    if (SharedCacheMap->CreateEvent == NULL) {

                        SharedCacheMap->CreateEvent = (PKEVENT)ExAllocatePoolWithTag( NonPagedPool,
                                                                                      sizeof(KEVENT),
                                                                                      'vEcC' );
                    }

                    if (SharedCacheMap->CreateEvent == NULL) {
                        DebugTrace( 0, 0, "Failed to allocate CreateEvent\n", 0 );

                        CcReleaseMasterLock( OldIrql );
                        SharedListOwned = FALSE;

                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    KeInitializeEvent( SharedCacheMap->CreateEvent,
                                       NotificationEvent,
                                       FALSE );
                }

                //
                //  Increment the OpenCount on the CacheMap.
                //

                CcIncrementOpenCount( SharedCacheMap, 'ecnI' );
                MustUninitialize = TRUE;

                //
                //  Release global resource before waiting
                //

                CcReleaseMasterLock( OldIrql );
                SharedListOwned = FALSE;

                DebugTrace( 0, 0, "Waiting on CreateEvent\n", 0 );

                KeWaitForSingleObject( SharedCacheMap->CreateEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)NULL);

                //
                //  If the real creator got an error, then we must bomb
                //  out too.
                //

                if (!NT_SUCCESS(SharedCacheMap->Status)) {
                    ExRaiseStatus( FsRtlNormalizeNtstatus( SharedCacheMap->Status,
                                                           STATUS_UNEXPECTED_MM_CREATE_ERR ));
                }
            }
            else {

                PCACHE_UNINITIALIZE_EVENT CUEvent, EventNext;

                //
                //  Increment the OpenCount on the CacheMap.
                //

                CcIncrementOpenCount( SharedCacheMap, 'esnI' );
                MustUninitialize = TRUE;

                //
                //  If there is a process waiting on an uninitialize on this
                //  cache map to complete, let the thread that is waiting go,
                //  since the uninitialize is now complete.
                //
                CUEvent = SharedCacheMap->UninitializeEvent;

                while (CUEvent != NULL) {
                    EventNext = CUEvent->Next;
                    KeSetEvent(&CUEvent->Event, 0, FALSE);
                    CUEvent = EventNext;
                }

                SharedCacheMap->UninitializeEvent = NULL;

                //
                //  Release global resource
                //

                CcReleaseMasterLock( OldIrql );
                SharedListOwned = FALSE;
            }
        }

        {
            PPRIVATE_CACHE_MAP PrivateCacheMap;

            //
            //  Now allocate (if local one already in use) and initialize
            //  the Private Cache Map.
            //

            PrivateCacheMap = &SharedCacheMap->PrivateCacheMap;

            //
            //  See if we should allocate a PrivateCacheMap while not holding
            //  a spinlock.
            //

            if (CacheMapToFree != NULL) {
                ExFreePool( CacheMapToFree );
                CacheMapToFree = NULL;
            }

            if (PrivateCacheMap->NodeTypeCode != 0) {
                CacheMapToFree = ExAllocatePoolWithTag( NonPagedPool, sizeof(PRIVATE_CACHE_MAP), 'cPcC' );
            }

            //
            //  Insert the new PrivateCacheMap in the list off the SharedCacheMap.
            //

            CcAcquireMasterLock( &OldIrql );
            SharedListOwned = TRUE;

            //
            //  Now make sure there is still no PrivateCacheMap, and if so just get out.
            //

            if (FileObject->PrivateCacheMap == NULL) {

                //
                //  Is the local one already in use?
                //

                if (PrivateCacheMap->NodeTypeCode != 0) {

                    //
                    //  Use the one allocated above, if there is one, else go to pool now.
                    //

                    if (CacheMapToFree == NULL) {
                        CacheMapToFree =
                            (PPRIVATE_CACHE_MAP)ExAllocatePoolWithTag( NonPagedPool,
                                                                       sizeof(PRIVATE_CACHE_MAP),
                                                                       'cPcC' );
                    }
                    PrivateCacheMap = CacheMapToFree;
                    CacheMapToFree = NULL;
                }

                if (PrivateCacheMap == NULL) {

                    DebugTrace( 0, 0, "Failed to allocate PrivateCacheMap\n", 0 );

                    CcReleaseMasterLock( OldIrql );
                    SharedListOwned = FALSE;

                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                RtlZeroMemory( PrivateCacheMap, sizeof(PRIVATE_CACHE_MAP) );

                PrivateCacheMap->NodeTypeCode = CACHE_NTC_PRIVATE_CACHE_MAP;
                PrivateCacheMap->NodeByteSize = sizeof(PRIVATE_CACHE_MAP);
                PrivateCacheMap->FileObject = FileObject;
                PrivateCacheMap->ReadAheadMask = PAGE_SIZE - 1;

                //
                //  Initialize the spin lock.
                //

                KeInitializeSpinLock( &PrivateCacheMap->ReadAheadSpinLock );

                InsertTailList( &SharedCacheMap->PrivateList, &PrivateCacheMap->PrivateLinks );

                FileObject->PrivateCacheMap = PrivateCacheMap;
            
            } else {

                //
                //  We raced with another initializer for the same fileobject and must
                //  drop our (to this point speculative) opencount.
                //

                ASSERT( SharedCacheMap->OpenCount > 1 );
                
                CcDecrementOpenCount( SharedCacheMap, 'rpnI' );
                SharedCacheMap = NULL;
            }
        }  

        MustUninitialize = FALSE;
    try_exit: NOTHING;
    }
    finally {

        //
        //  See if we got an error and must uninitialize the SharedCacheMap
        //

        if (MustUninitialize) {

            if (!SharedListOwned) {
                CcAcquireMasterLock( &OldIrql );
            }
            if (WeSetBeingCreated) {
                if (SharedCacheMap->CreateEvent != NULL) {
                    KeSetEvent( SharedCacheMap->CreateEvent, 0, FALSE );
                }
                ClearFlag(SharedCacheMap->Flags, BEING_CREATED);
            }

            //
            //  Now release our open count.
            //

            CcDecrementOpenCount( SharedCacheMap, 'umnI' );

            if ((SharedCacheMap->OpenCount == 0) &&
                !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
                (SharedCacheMap->DirtyPages == 0)) {

                //
                //  On PinAccess it is safe and necessary to eliminate
                //  the structure immediately.
                //

                if (PinAccess) {

                    CcDeleteSharedCacheMap( SharedCacheMap, OldIrql, FALSE );

                //
                //  If it is not PinAccess, we must lazy delete, because
                //  we could get into a deadlock trying to acquire the
                //  stream exclusive when we dereference the file object.
                //

                } else {

                    //
                    //  Move it to the dirty list so the lazy write scan will
                    //  see it.
                    //

                    RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
                    InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                                    &SharedCacheMap->SharedCacheMapLinks );

                    //
                    //  Make sure the Lazy Writer will wake up, because we
                    //  want him to delete this SharedCacheMap.
                    //

                    LazyWriter.OtherWork = TRUE;
                    if (!LazyWriter.ScanActive) {
                        CcScheduleLazyWriteScan();
                    }

                    CcReleaseMasterLock( OldIrql );
                }

            } else {

                CcReleaseMasterLock( OldIrql );
            }

            SharedListOwned = FALSE;

        //
        //  If we did not create this SharedCacheMap, then there is a
        //  possibility that it is in the dirty list.  Once we are sure
        //  we have the spinlock, just make sure it is in the clean list
        //  if there are no dirty bytes and the open count is nonzero.
        //  (The latter test is almost guaranteed, of course, but we check
        //  it to be safe.)
        //

        } else if (!WeCreated &&
                   (SharedCacheMap != NULL)) {

            if (!SharedListOwned) {

                CcAcquireMasterLock( &OldIrql );
                SharedListOwned = TRUE;
            }

            if ((SharedCacheMap->DirtyPages == 0) &&
                (SharedCacheMap->OpenCount != 0)) {

                RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
                InsertTailList( &CcCleanSharedCacheMapList,
                                &SharedCacheMap->SharedCacheMapLinks );
            }
        }

        //
        //  Release global resource
        //

        if (SharedListOwned) {
            CcReleaseMasterLock( OldIrql );
        }

        if (CacheMapToFree != NULL) {
            ExFreePool(CacheMapToFree);
        }

    }

    DebugTrace(-1, me, "CcInitializeCacheMap -> VOID\n", 0 );

    return;
}


BOOLEAN
CcUninitializeCacheMap (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER TruncateSize OPTIONAL,
    IN PCACHE_UNINITIALIZE_EVENT UninitializeEvent OPTIONAL
    )

/*++

Routine Description:

    This routine uninitializes the previously initialized Shared and Private
    Cache Maps.  This routine is only intended to be called by File Systems.
    It should be called when the File System receives a cleanup call on the
    File Object.

    A File System which supports data caching must always call this routine
    whenever it closes a file, whether the caller opened the file with
    NO_INTERMEDIATE_BUFFERING as FALSE or not.  This is because the final
    cleanup of a file related to truncation or deletion of the file, can
    only occur on the last close, whether the last closer cached the file
    or not.  When CcUnitializeCacheMap is called on a file object for which
    CcInitializeCacheMap was never called, the call has a benign effect
    iff no one has truncated or deleted the file; otherwise the necessary
    cleanup relating to the truncate or close is performed.

    In summary, CcUnitializeCacheMap does the following:

        If the caller had Write or Delete access, the cache is flushed.
        (This could change with lazy writing.)

        If a Cache Map was initialized on this File Object, it is
        unitialized (unmap any views, delete section, and delete
        Cache Map structures).

        On the last Cleanup, if the file has been deleted, the
        Section is forced closed.  If the file has been truncated, then
        the truncated pages are purged from the cache.

Arguments:

    FileObject - File Object which was previously supplied to
                 CcInitializeCacheMap.

    TruncateSize - If specified, the file was truncated to the specified
                   size, and the cache should be purged accordingly.

    UninitializeEvent - If specified, then the provided event
                   will be set to the signalled state when the actual flush is
                   completed.  This is only of interest to file systems that
                   require that they be notified when a cache flush operation
                   has completed.  Due to network protocol restrictions, it
                   is critical that network file systems know exactly when
                   a cache flush operation completes, by specifying this
                   event, they can be notified when the cache section is
                   finally purged if the section is "lazy-deleted".

ReturnValue:

    FALSE if Section was not closed.
    TRUE if Section was closed.

--*/

{
    KIRQL OldIrql;
    PSHARED_CACHE_MAP SharedCacheMap;
    ULONG ActivePage;
    ULONG PageIsDirty;
    PVACB ActiveVacb = NULL;
    BOOLEAN SectionClosed = FALSE;
    BOOLEAN SharedListAcquired = FALSE;
    PPRIVATE_CACHE_MAP PrivateCacheMap;

    DebugTrace(+1, me, "CcUninitializeCacheMap:\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace( 0, me, "    &TruncateSize = %08lx\n", TruncateSize );

    //
    //  Insure release of resources
    //

    try {

        //
        //  Serialize Creation/Deletion of all Shared CacheMaps
        //

        CcAcquireMasterLock( &OldIrql );
        SharedListAcquired = TRUE;

        //
        //  Get pointer to SharedCacheMap via File Object.
        //

        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
        PrivateCacheMap = FileObject->PrivateCacheMap;

        //
        //  Decrement Open Count on SharedCacheMap, if we did a cached open.
        //  Also unmap PrivateCacheMap if it is mapped and deallocate it.
        //

        if (PrivateCacheMap != NULL) {

            ASSERT( PrivateCacheMap->FileObject == FileObject );
            
            CcDecrementOpenCount( SharedCacheMap, 'ninU' );

            //
            //  Remove PrivateCacheMap from list in SharedCacheMap.
            //

            RemoveEntryList( &PrivateCacheMap->PrivateLinks );

            //
            //  Free local or allocated PrivateCacheMap
            //

            if (PrivateCacheMap == &SharedCacheMap->PrivateCacheMap) {
                PrivateCacheMap->NodeTypeCode = 0;
                PrivateCacheMap = NULL;
            }

            FileObject->PrivateCacheMap = (PPRIVATE_CACHE_MAP)NULL;
        }

        //
        //  Now if we have a SharedCacheMap whose Open Count went to 0, we
        //  have some additional cleanup.
        //

        if (SharedCacheMap != NULL) {

            //
            //  If a Truncate Size was specified, then remember that we want to
            //  truncate the FileSize and purge the unneeded pages when OpenCount
            //  goes to 0.
            //

            if (ARGUMENT_PRESENT(TruncateSize)) {

                if ( (TruncateSize->QuadPart == 0) && (SharedCacheMap->FileSize.QuadPart != 0) ) {
                    
                    SetFlag(SharedCacheMap->Flags, TRUNCATE_REQUIRED);
                
                } else

                //
                //  If this is the last guy, I can drop the file size down
                //  now.
                //

                if (IsListEmpty(&SharedCacheMap->PrivateList)) {
                    SharedCacheMap->FileSize = *TruncateSize;
                }
            }

            //
            //  If other file objects are still using this SharedCacheMap,
            //  then we are done now.
            //

            if (SharedCacheMap->OpenCount != 0) {

                DebugTrace(-1, me, "SharedCacheMap OpenCount != 0\n", 0);

                //
                //  If the caller specified an event to be set when
                //  the cache uninitialize is completed, set the event
                //  now, because the uninitialize is complete for this file.
                //  (Note, we make him wait if he is the last guy.)
                //

                if (ARGUMENT_PRESENT(UninitializeEvent)) {

                    if (!IsListEmpty(&SharedCacheMap->PrivateList)) {
                        KeSetEvent(&UninitializeEvent->Event, 0, FALSE);
                    } else {
                        UninitializeEvent->Next = SharedCacheMap->UninitializeEvent;
                        SharedCacheMap->UninitializeEvent = UninitializeEvent;
                    }
                }

                try_return( SectionClosed = FALSE );
            }

            //
            //  The private cache map list better be empty!
            //

            ASSERT(IsListEmpty(&SharedCacheMap->PrivateList));

            //
            //  Set the "uninitialize complete" in the shared cache map
            //  so that CcDeleteSharedCacheMap will delete it.
            //

            if (ARGUMENT_PRESENT(UninitializeEvent)) {
                UninitializeEvent->Next = SharedCacheMap->UninitializeEvent;
                SharedCacheMap->UninitializeEvent = UninitializeEvent;
            }

            //
            //  We are in the process of deleting this cache map.  If the
            //  Lazy Writer is active or the Bcb list is not empty or the Lazy
            //  Writer will hit this SharedCacheMap because we are purging
            //  the file to 0, then get out and let the Lazy Writer clean
            //  up.
            //

            if ((!FlagOn(SharedCacheMap->Flags, PIN_ACCESS) &&
                 !ARGUMENT_PRESENT(UninitializeEvent))

                    ||

                FlagOn(SharedCacheMap->Flags, WRITE_QUEUED)

                    ||

                (SharedCacheMap->DirtyPages != 0)) {

                //
                //  Move it to the dirty list so the lazy write scan will
                //  see it.
                //

                if (!FlagOn(SharedCacheMap->Flags, WRITE_QUEUED)) {
                    RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
                    InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                                    &SharedCacheMap->SharedCacheMapLinks );
                }

                //
                //  Make sure the Lazy Writer will wake up, because we
                //  want him to delete this SharedCacheMap.
                //

                LazyWriter.OtherWork = TRUE;
                if (!LazyWriter.ScanActive) {
                    CcScheduleLazyWriteScan();
                }

                //
                //  Get the active Vacb if we are going to lazy delete, to
                //  free it for someone who can use it.
                //

                GetActiveVacbAtDpcLevel( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

                DebugTrace(-1, me, "SharedCacheMap has Bcbs and not purging to 0\n", 0);

                try_return( SectionClosed = FALSE );
            }

            //
            //  Now we can delete the SharedCacheMap.  If there are any Bcbs,
            //  then we must be truncating to 0, and they will also be deleted.
            //  On return the Shared Cache Map List Spinlock will be released.
            //

            CcDeleteSharedCacheMap( SharedCacheMap, OldIrql, FALSE );

            SharedListAcquired = FALSE;

            try_return( SectionClosed = TRUE );
        }

        //
        //  No Shared Cache Map.  To make the file go away, we still need to
        //  purge the section, if one exists.  (And we still need to release
        //  our global list first to avoid deadlocks.)
        //

        else {
            if (ARGUMENT_PRESENT(TruncateSize) &&
                ( TruncateSize->QuadPart == 0 ) &&
                (*(PCHAR *)FileObject->SectionObjectPointer != NULL)) {

                CcReleaseMasterLock( OldIrql );
                SharedListAcquired = FALSE;

                DebugTrace( 0, mm, "MmPurgeSection:\n", 0 );
                DebugTrace( 0, mm, "    SectionObjectPointer = %08lx\n",
                            FileObject->SectionObjectPointer );
                DebugTrace2(0, mm, "    Offset = %08lx\n",
                            TruncateSize->LowPart,
                            TruncateSize->HighPart );

                //
                //  0 Length means to purge from the TruncateSize on.
                //

                CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                     TruncateSize,
                                     0,
                                     FALSE );
            }

            //
            //  If the caller specified an event to be set when
            //  the cache uninitialize is completed, set the event
            //  now, because the uninitialize is complete for this file.
            //

            if (ARGUMENT_PRESENT(UninitializeEvent)) {
                KeSetEvent(&UninitializeEvent->Event, 0, FALSE);
            }

        }

    try_exit: NOTHING;
    }
    finally {

        //
        //  Release global resources
        //

        if (SharedListAcquired) {
            CcReleaseMasterLock( OldIrql );
        }

        //
        //  Free the active vacb, if we found one.
        //

        if (ActiveVacb != NULL) {

            CcFreeActiveVacb( ActiveVacb->SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
        }

        //
        //  Free PrivateCacheMap now that we no longer have the spinlock.
        //

        if (PrivateCacheMap != NULL) {
            ExFreePool( PrivateCacheMap );
        }
    }

    DebugTrace(-1, me, "CcUnitializeCacheMap -> %02lx\n", SectionClosed );

    return SectionClosed;

}


//
//  Internal support routine.
//

VOID
FASTCALL
CcDeleteSharedCacheMap (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN KIRQL ListIrql,
    IN ULONG ReleaseFile
    )

/*++

Routine Description:

    The specified SharedCacheMap is removed from the global list of
    SharedCacheMap's and deleted with all of its related structures.
    Other objects which were referenced in CcInitializeCacheMap are
    dereferenced here.

    NOTE:   The CcMasterSpinLock must already be acquired
            on entry.  It is released on return.

Arguments:

    SharedCacheMap - Pointer to Cache Map to delete

    ListIrql - priority to restore to when releasing shared cache map list

    ReleaseFile - Supplied as nonzero if file was acquired exclusive and
                  should be released.

ReturnValue:

    None.

--*/

{
    LIST_ENTRY LocalList;
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT FileObject;
    PVACB ActiveVacb;
    ULONG ActivePage;
    ULONG PageIsDirty;
    KIRQL OldIrql;
    PMBCB Mbcb;

    DebugTrace(+1, me, "CcDeleteSharedCacheMap:\n", 0 );
    DebugTrace( 0, me, "    SharedCacheMap = %08lx\n", SharedCacheMap );

    //
    //  Remove it from the global list and clear the pointer to it via
    //  the File Object.
    //

    RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );

    //
    //  Zero pointer to SharedCacheMap.  Once we have cleared the pointer,
    //  we can/must release the global list to avoid deadlocks.
    //

    FileObject = SharedCacheMap->FileObject;

    FileObject->SectionObjectPointer->SharedCacheMap = (PSHARED_CACHE_MAP)NULL;
    SetFlag( SharedCacheMap->Flags, WRITE_QUEUED );

    //
    //  The OpenCount is 0, but we still need to flush out any dangling
    //  cache read or writes.
    //

    if ((SharedCacheMap->VacbActiveCount != 0) || (SharedCacheMap->NeedToZero != NULL)) {

        //
        //  We will put it in a local list and set a flag
        //  to keep the Lazy Writer away from it, so that we can wrip it out
        //  below if someone manages to sneak in and set something dirty, etc.
        //  If the file system does not synchronize cleanup calls with an
        //  exclusive on the stream, then this case is possible.
        //

        InitializeListHead( &LocalList );
        InsertTailList( &LocalList, &SharedCacheMap->SharedCacheMapLinks );

        //
        //  If there is an active Vacb, then nuke it now (before waiting!).
        //

        GetActiveVacbAtDpcLevel( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

        CcReleaseMasterLock( ListIrql );

        //
        //  No point in saying the page is dirty (which can cause an allocation
        //  failure), since we are deleting this SharedCacheMap anyway.
        //

        CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, FALSE );

        while (SharedCacheMap->VacbActiveCount != 0) {
            CcWaitOnActiveCount( SharedCacheMap );
        }

        //
        //  Now in case we hit the rare path where someone moved the
        //  SharedCacheMap again, do a remove again now.  It may be
        //  from our local list or it may be from the dirty list,
        //  but who cares?  The important thing is to remove it in
        //  the case it was the dirty list, since we will delete it
        //  below.
        //

        CcAcquireMasterLock( &ListIrql );
        RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
    }

    CcReleaseMasterLock( ListIrql );

    //
    //  If there are Bcbs, then empty the list, asserting that none of them
    //  can be pinned now if we have gotten this far!
    //

    NextEntry = SharedCacheMap->BcbList.Flink;
    while (NextEntry != &SharedCacheMap->BcbList) {

        PBCB Bcb;

        Bcb = (PBCB)CONTAINING_RECORD( NextEntry,
                                       BCB,
                                       BcbLinks );
        NextEntry = Bcb->BcbLinks.Flink;

        //
        //  Skip over the pendaflex entries, only removing true Bcbs
        //  so that level teardown doesn't need to special case unhooking
        //  the pendaflex.  This has the side benefit of dramatically
        //  reducing write traffic to memory on teardown of large files.
        //
        //  I really wonder how often we have Bcbs at teardown.  This is
        //  a lot of work that could be avoided otherwise.
        //

        if (Bcb->NodeTypeCode == CACHE_NTC_BCB) {

            ASSERT( Bcb->PinCount == 0 );

            RemoveEntryList( &Bcb->BcbLinks );

            //
            //  For large metadata streams we unlock the Vacb level when removing.
            //  We do not need spinlocks since no other thread can be accessing
            //  this list when we are deleting the SharedCacheMap.
            //

            CcUnlockVacbLevel( SharedCacheMap, Bcb->FileOffset.QuadPart );

            //
            //  There is a small window where the data could still be mapped
            //  if (for example) the Lazy Writer collides with a CcCopyWrite
            //  in the foreground, and then someone calls CcUninitializeCacheMap
            //  while the Lazy Writer is active.  This is because the Lazy
            //  Writer biases the pin count.  Deal with that here.
            //

            if (Bcb->BaseAddress != NULL) {
                CcFreeVirtualAddress( Bcb->Vacb );
            }

            //
            //  Debug routines used to remove Bcbs from the global list
            //

#if LIST_DBG

            {
                KIRQL OldIrql;

                ExAcquireSpinLock( &CcBcbSpinLock, &OldIrql );

                if (Bcb->CcBcbLinks.Flink != NULL) {

                    RemoveEntryList( &Bcb->CcBcbLinks );
                    CcBcbCount -= 1;
                }

                ExReleaseSpinLock( &CcBcbSpinLock, OldIrql );
            }

#endif

            //
            //  If the Bcb is dirty, we have to synchronize with the Lazy Writer
            //  and reduce the total number of dirty.
            //

            CcAcquireMasterLock( &ListIrql );
            if (Bcb->Dirty) {

                SharedCacheMap->DirtyPages -= Bcb->ByteLength >> PAGE_SHIFT;
                CcTotalDirtyPages -= Bcb->ByteLength >> PAGE_SHIFT;
            }

            CcReleaseMasterLock( ListIrql );

            CcDeallocateBcb( Bcb );
        }
    }

    //
    //  Call local routine to unmap, and purge if necessary.
    //

    CcUnmapAndPurge( SharedCacheMap );

    //
    //  Now release the file now that the purge is done.
    //

    if (ReleaseFile) {
        FsRtlReleaseFile( SharedCacheMap->FileObject );
    }

    //
    //  Dereference our pointer to the Section and FileObject
    //  (We have to test the Section pointer since CcInitializeCacheMap
    //  calls this routine for error recovery.  Release our global
    //  resource before dereferencing the FileObject to avoid deadlocks.
    //

    if (SharedCacheMap->Section != NULL) {
        ObDereferenceObject( SharedCacheMap->Section );
    }
    ObDereferenceObject( FileObject );

    //
    //  If there is an Mbcb, deduct any dirty pages and deallocate.
    //

    if (SharedCacheMap->Mbcb != NULL) {
        CcDeleteMbcb( SharedCacheMap );
    }

    //
    //  If there was an uninitialize event specified for this shared cache
    //  map, then set it to the signalled state, indicating that we are
    //  removing the section and deleting the shared cache map.
    //

    if (SharedCacheMap->UninitializeEvent != NULL) {
        PCACHE_UNINITIALIZE_EVENT CUEvent, EventNext;

        CUEvent = SharedCacheMap->UninitializeEvent;
        while (CUEvent != NULL) {
            EventNext = CUEvent->Next;
            KeSetEvent(&CUEvent->Event, 0, FALSE);
            CUEvent = EventNext;
        }
    }

    //
    //  Now delete the Vacb vector.
    //

    if ((SharedCacheMap->Vacbs != &SharedCacheMap->InitialVacbs[0])

            &&

        (SharedCacheMap->Vacbs != NULL)) {

        //
        //  If there are Vacb levels, then the Vacb Array better be in an empty state.
        //

        ASSERT((SharedCacheMap->SectionSize.QuadPart <= VACB_SIZE_OF_FIRST_LEVEL) ||
               !IsVacbLevelReferenced( SharedCacheMap, SharedCacheMap->Vacbs, 1 ));

        ExFreePool( SharedCacheMap->Vacbs );
    }

    //
    //  If an event had to be allocated for this SharedCacheMap,
    //  deallocate it.
    //

    if ((SharedCacheMap->CreateEvent != NULL) && (SharedCacheMap->CreateEvent != &SharedCacheMap->Event)) {
        ExFreePool( SharedCacheMap->CreateEvent );
    }

    if ((SharedCacheMap->WaitOnActiveCount != NULL) && (SharedCacheMap->WaitOnActiveCount != &SharedCacheMap->Event)) {
        ExFreePool( SharedCacheMap->WaitOnActiveCount );
    }

    //
    //  Deallocate the storeage for the SharedCacheMap.
    //

    ExFreePool( SharedCacheMap );

    DebugTrace(-1, me, "CcDeleteSharedCacheMap -> VOID\n", 0 );

    return;

}


VOID
CcSetFileSizes (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes
    )

/*++

Routine Description:

    This routine must be called whenever a file has been extended to reflect
    this extension in the cache maps and underlying section.  Calling this
    routine has a benign effect if the current size of the section is
    already greater than or equal to the new AllocationSize.

    This routine must also be called whenever the FileSize for a file changes
    to reflect these changes in the Cache Manager.

    This routine seems rather large, but in the normal case it only acquires
    a spinlock, updates some fields, and exits.  Less often it will either
    extend the section, or truncate/purge the file, but it would be unexpected
    to do both.  On the other hand, the idea of this routine is that it does
    "everything" required when AllocationSize or FileSize change.

Arguments:

    FileObject - A file object for which CcInitializeCacheMap has been
                 previously called.

    FileSizes - A pointer to AllocationSize, FileSize and ValidDataLength
                for the file.  AllocationSize is ignored if it is not larger
                than the current section size (i.e., it is ignored unless it
                has grown).  ValidDataLength is not used.


Return Value:

    None

--*/

{
    LARGE_INTEGER NewSectionSize;
    LARGE_INTEGER NewFileSize;
    LARGE_INTEGER NewValidDataLength;
    IO_STATUS_BLOCK IoStatus;
    PSHARED_CACHE_MAP SharedCacheMap;
    NTSTATUS Status;
    KIRQL OldIrql;
    PVACB ActiveVacb;
    ULONG ActivePage;
    ULONG PageIsDirty;

    DebugTrace(+1, me, "CcSetFileSizes:\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace( 0, me, "    FileSizes = %08lx\n", FileSizes );

    //
    //  Make a local copy of the new file size and section size.
    //

    NewSectionSize = FileSizes->AllocationSize;
    NewFileSize = FileSizes->FileSize;
    NewValidDataLength = FileSizes->ValidDataLength;

    //
    //  Serialize Creation/Deletion of all Shared CacheMaps
    //

    CcAcquireMasterLock( &OldIrql );

    //
    //  Get pointer to SharedCacheMap via File Object.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  If the file is not cached, just get out.
    //

    if ((SharedCacheMap == NULL) || (SharedCacheMap->Section == NULL)) {

        CcReleaseMasterLock( OldIrql );

        //
        //  Let's try to purge the file incase this is a truncate.  In the
        //  vast majority of cases when there is no shared cache map, there
        //  is no data section either, so this call will eventually be
        //  no-oped in Mm.
        //

        //
        //  First flush the first page we are keeping, if it has data, before
        //  we throw it away.
        //

        if (NewFileSize.LowPart & (PAGE_SIZE - 1)) {
            MmFlushSection( FileObject->SectionObjectPointer, &NewFileSize, 1, &IoStatus, FALSE );
        }

        CcPurgeCacheSection( FileObject->SectionObjectPointer,
                             &NewFileSize,
                             0,
                             FALSE );

        DebugTrace(-1, me, "CcSetFileSizes -> VOID\n", 0 );

        return;
    }

    //
    //  Make call a Noop if file is not mapped, or section already big enough.
    //

    if ( NewSectionSize.QuadPart > SharedCacheMap->SectionSize.QuadPart ) {

        //
        //  Increment open count to make sure the SharedCacheMap stays around,
        //  then release the spinlock so that we can call Mm.
        //

        CcIncrementOpenCount( SharedCacheMap, '1fSS' );
        CcReleaseMasterLock( OldIrql );

        //
        //  Round new section size to pages.
        //

        NewSectionSize.QuadPart = NewSectionSize.QuadPart + (LONGLONG)(DEFAULT_EXTEND_MODULO - 1);
        NewSectionSize.LowPart &= ~(DEFAULT_EXTEND_MODULO - 1);

        //
        //  Use try-finally to make sure we get the open count decremented.
        //

        try {

            //
            //  Call MM to extend the section.
            //

            DebugTrace( 0, mm, "MmExtendSection:\n", 0 );
            DebugTrace( 0, mm, "    Section = %08lx\n", SharedCacheMap->Section );
            DebugTrace2(0, mm, "    Size = %08lx, %08lx\n",
                        NewSectionSize.LowPart, NewSectionSize.HighPart );

            Status = MmExtendSection( SharedCacheMap->Section, &NewSectionSize, TRUE );

            if (!NT_SUCCESS(Status)) {

                DebugTrace( 0, 0, "Error from MmExtendSection, Status = %08lx\n",
                            Status );

                ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                       STATUS_UNEXPECTED_MM_EXTEND_ERR ));
            }

            //
            //  Extend the Vacb array.
            //

            CcExtendVacbArray( SharedCacheMap, NewSectionSize );

        } finally {

            //
            //  Serialize again to decrement the open count.
            //

            CcAcquireMasterLock( &OldIrql );

            CcDecrementOpenCount( SharedCacheMap, '1fSF' );

            if ((SharedCacheMap->OpenCount == 0) &&
                !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
                (SharedCacheMap->DirtyPages == 0)) {

                //
                //  Move to the dirty list.
                //

                RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
                InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                                &SharedCacheMap->SharedCacheMapLinks );

                //
                //  Make sure the Lazy Writer will wake up, because we
                //  want him to delete this SharedCacheMap.
                //

                LazyWriter.OtherWork = TRUE;
                if (!LazyWriter.ScanActive) {
                    CcScheduleLazyWriteScan();
                }
            }

            CcReleaseMasterLock( OldIrql );
        }

        //
        //  It is now very unlikely that we have any more work to do, but just
        //  in case we reacquire the spinlock and check again if we are cached.
        //

        CcAcquireMasterLock( &OldIrql );

        //
        //  Get pointer to SharedCacheMap via File Object.
        //

        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

        //
        //  If the file is not cached, just get out.
        //

        if (SharedCacheMap == NULL) {

            CcReleaseMasterLock( OldIrql );

            DebugTrace(-1, me, "CcSetFileSizes -> VOID\n", 0 );

            return;
        }
    }

    //
    //  If we are shrinking either of these two sizes, then we must free the
    //  active page, since it may be locked.
    //

    CcIncrementOpenCount( SharedCacheMap, '2fSS' );

    try {

        if ( ( NewFileSize.QuadPart < SharedCacheMap->ValidDataGoal.QuadPart ) ||
             ( NewFileSize.QuadPart < SharedCacheMap->FileSize.QuadPart )) {

            GetActiveVacbAtDpcLevel( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

            if ((ActiveVacb != NULL) || (SharedCacheMap->NeedToZero != NULL)) {

                CcReleaseMasterLock( OldIrql );

                CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

                //
                //  Serialize again to reduce ValidDataLength.  It cannot change
                //  because the caller must have the file exclusive.
                //

                CcAcquireMasterLock( &OldIrql );
            }
        }

        //
        //  If the section did not grow, see if the file system supports ValidDataLength,
        //  then update the valid data length in the file system.
        //

        if ( SharedCacheMap->ValidDataLength.QuadPart != MAXLONGLONG ) {

            if ( NewFileSize.QuadPart < SharedCacheMap->ValidDataLength.QuadPart ) {
                SharedCacheMap->ValidDataLength = NewFileSize;
            }

            //
            //  Update our notion of ValidDataGoal (how far the file has been written
            //  in the cache) with caller's ValidDataLength.  (Our ValidDataLength controls
            //  when we issue ValidDataLength callbacks.)  *** For now play it safe by
            //  only growing here, which is the historical problem at hand, as with
            //  compressed and uncompressed stream caches.
            //

            if (NewValidDataLength.QuadPart > SharedCacheMap->ValidDataGoal.QuadPart) {
                SharedCacheMap->ValidDataGoal = NewValidDataLength;
            }
        }

        //
        //  On truncate, be nice guys and actually purge away user data from
        //  the cache.  However, the PinAccess check is important to avoid deadlocks
        //  in Ntfs.
        //
        //  It is also important to check the Vacb Active count.  The caller
        //  must have the file exclusive, therefore, no one else can be actively
        //  doing anything in the file.  Normally the Active count will be zero
        //  (like in a normal call from Set File Info), and we can go ahead and truncate.
        //  However, if the active count is nonzero, chances are this very thread has
        //  something pinned or mapped, and we will deadlock if we try to purge and
        //  wait for the count to go zero.  A rare case of this which deadlocked DaveC
        //  on Christmas Day of 1992, is where Ntfs was trying to convert an attribute
        //  from resident to nonresident - which is a good example of a case where the
        //  purge was not needed.
        //

        if ( (NewFileSize.QuadPart < SharedCacheMap->FileSize.QuadPart ) &&
            !FlagOn(SharedCacheMap->Flags, PIN_ACCESS) &&
            (SharedCacheMap->VacbActiveCount == 0)) {

            //
            //  Release the spinlock so that we can call Mm.
            //

            CcReleaseMasterLock( OldIrql );

            //
            //  If we are actually truncating to zero (a size which has particular
            //  meaning to the Lazy Writer scan!), then we must reset the Mbcb if
            //  there is one, so that we do not keep dirty pages around forever.
            //

            if ((NewFileSize.QuadPart == 0) && (SharedCacheMap->Mbcb != NULL)) {
                CcDeleteMbcb( SharedCacheMap );
            }

            CcPurgeAndClearCacheSection( SharedCacheMap, &NewFileSize );

            //
            //  Serialize again to decrement the open count.
            //

            CcAcquireMasterLock( &OldIrql );
        }

    } finally {

        //
        //  We should only be raising without owning the spinlock.
        //

        if (AbnormalTermination()) {

            CcAcquireMasterLock( &OldIrql );
        }

        CcDecrementOpenCount( SharedCacheMap, '2fSF' );

        SharedCacheMap->FileSize = NewFileSize;

        if ((SharedCacheMap->OpenCount == 0) &&
            !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
            (SharedCacheMap->DirtyPages == 0)) {

            //
            //  Move to the dirty list.
            //

            RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
            InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                            &SharedCacheMap->SharedCacheMapLinks );

            //
            //  Make sure the Lazy Writer will wake up, because we
            //  want him to delete this SharedCacheMap.
            //

            LazyWriter.OtherWork = TRUE;
            if (!LazyWriter.ScanActive) {
                CcScheduleLazyWriteScan();
            }
        }

        CcReleaseMasterLock( OldIrql );
    }

    DebugTrace(-1, me, "CcSetFileSizes -> VOID\n", 0 );

    return;
}


VOID
CcPurgeAndClearCacheSection (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset
    )

/*++

Routine Description:

    This routine calls CcPurgeCacheSection after zeroing the end any
    partial page at the start of the range.  If the file is not cached
    it flushes this page before the purge.

Arguments:

    SectionObjectPointer - A pointer to the Section Object Pointers
                           structure in the nonpaged Fcb.

    FileOffset - Offset from which file should be purged - rounded down
               to page boundary.  If NULL, purge the entire file.

ReturnValue:

    FALSE - if the section was not successfully purged
    TRUE - if the section was successfully purged

--*/

{
    ULONG TempLength, Length;
    LARGE_INTEGER LocalFileOffset;
    IO_STATUS_BLOCK IoStatus;
    PVOID TempVa;
    PVACB Vacb;

    //
    //  If a range was specified, then we have to see if we need to
    //  save any user data before purging.
    //

    if ((FileOffset->LowPart & (PAGE_SIZE - 1)) != 0) {

        //
        //  Switch to LocalFileOffset.  We do it this way because we
        //  still pass it on as an optional parameter.
        //

        LocalFileOffset = *FileOffset;
        FileOffset = &LocalFileOffset;

        //
        //  If the file is cached, then we can actually zero the data to
        //  be purged in memory, and not purge those pages.  This is a huge
        //  savings, because sometimes the flushes in the other case cause
        //  us to kill lots of stack, time and I/O doing CcZeroData in especially
        //  large user-mapped files.
        //

        if ((SharedCacheMap->Section != NULL) &&
            (SharedCacheMap->Vacbs != NULL)) {

            //
            //  First zero the first page we are keeping, if it has data, and
            //  adjust FileOffset and Length to allow it to stay.
            //

            TempLength = PAGE_SIZE - (FileOffset->LowPart & (PAGE_SIZE - 1));

            TempVa = CcGetVirtualAddress( SharedCacheMap, *FileOffset, &Vacb, &Length );

            try {

                //
                //  Do not map and zero the page if we are not reducing our notion
                //  of Valid Data, because that does two bad things.  First CcSetDirtyInMask
                //  will arbitrarily smash up ValidDataGoal (causing a potential invalid
                //  CcSetValidData call).  Secondly, if the Lazy Writer writes the last
                //  page ahead of another flush through MM, then the file system will
                //  never see a write from MM, and will not include the last page in
                //  ValidDataLength on disk.
                //

                RtlZeroMemory( TempVa, TempLength );

                if (FileOffset->QuadPart <= SharedCacheMap->ValidDataGoal.QuadPart) {

                    //
                    //  Make sure the Lazy Writer writes it.
                    //

                    CcSetDirtyInMask( SharedCacheMap, FileOffset, TempLength );

                //
                //  Otherwise, we are mapped, so make sure at least that Mm
                //  knows the page is dirty since we zeroed it.
                //

                } else {

                    MmSetAddressRangeModified( TempVa, 1 );
                }

                FileOffset->QuadPart += (LONGLONG)TempLength;

            //
            //  If we get any kind of error, like failing to read the page from
            //  the network, just charge on.  Note that we only read it in order
            //  to zero it and avoid the flush below, so if we cannot read it
            //  there is really no stale data problem.
            //

            } except(EXCEPTION_EXECUTE_HANDLER) {

                NOTHING;
            }

            CcFreeVirtualAddress( Vacb );

        } else {

            //
            //  First flush the first page we are keeping, if it has data, before
            //  we throw it away.
            //

            MmFlushSection( SharedCacheMap->FileObject->SectionObjectPointer, FileOffset, 1, &IoStatus, FALSE );
        }
    }

    CcPurgeCacheSection( SharedCacheMap->FileObject->SectionObjectPointer,
                         FileOffset,
                         0,
                         FALSE );
}


BOOLEAN
CcPurgeCacheSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN UninitializeCacheMaps
    )

/*++

Routine Description:

    This routine may be called to force a purge of the cache section,
    even if it is cached.  Note, if a user has the file mapped, then the purge
    will *not* take effect, and this must be considered part of normal application
    interaction.  The purpose of purge is to throw away potentially nonzero
    data, so that it will be read in again and presumably zeroed.  This is
    not really a security issue, but rather an effort to not confuse the
    application when it sees nonzero data.  We cannot help the fact that
    a user-mapped view forces us to hang on to stale data.

    This routine is intended to be called whenever previously written
    data is being truncated from the file, and the file is not being
    deleted.

    The file must be acquired exclusive in order to call this routine.

Arguments:

    SectionObjectPointer - A pointer to the Section Object Pointers
                           structure in the nonpaged Fcb.

    FileOffset - Offset from which file should be purged - rounded down
               to page boundary.  If NULL, purge the entire file.

    Length - Defines the length of the byte range to purge, starting at
             FileOffset.  This parameter is ignored if FileOffset is
             specified as NULL.  If FileOffset is specified and Length
             is 0, then purge from FileOffset to the end of the file.

    UninitializeCacheMaps - If TRUE, we should uninitialize all the private
                            cache maps before purging the data.

ReturnValue:

    FALSE - if the section was not successfully purged
    TRUE - if the section was successfully purged

--*/

{
    KIRQL OldIrql;
    PSHARED_CACHE_MAP SharedCacheMap;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    ULONG ActivePage;
    ULONG PageIsDirty;
    BOOLEAN PurgeWorked = TRUE;
    PVACB Vacb = NULL;

    DebugTrace(+1, me, "CcPurgeCacheSection:\n", 0 );
    DebugTrace( 0, mm, "    SectionObjectPointer = %08lx\n", SectionObjectPointer );
    DebugTrace2(0, me, "    FileOffset = %08lx, %08lx\n",
                            ARGUMENT_PRESENT(FileOffset) ? FileOffset->LowPart
                                                         : 0,
                            ARGUMENT_PRESENT(FileOffset) ? FileOffset->HighPart
                                                         : 0 );
    DebugTrace( 0, me, "    Length = %08lx\n", Length );


    //
    //  If you want us to uninitialize cache maps, the RtlZeroMemory paths
    //  below depend on actually having to purge something after zeroing.
    //

    ASSERT(!UninitializeCacheMaps || (Length == 0) || (Length >= PAGE_SIZE * 2));

    //
    //  Serialize Creation/Deletion of all Shared CacheMaps
    //

    CcAcquireMasterLock( &OldIrql );

    //
    //  Get pointer to SharedCacheMap via File Object.
    //

    SharedCacheMap = SectionObjectPointer->SharedCacheMap;

    //
    //  Increment open count to make sure the SharedCacheMap stays around,
    //  then release the spinlock so that we can call Mm.
    //

    if (SharedCacheMap != NULL) {

        CcIncrementOpenCount( SharedCacheMap, 'scPS' );

        //
        //  If there is an active Vacb, then nuke it now (before waiting!).
        //

        GetActiveVacbAtDpcLevel( SharedCacheMap, Vacb, ActivePage, PageIsDirty );
    }

    CcReleaseMasterLock( OldIrql );

    if (Vacb != NULL) {

        CcFreeActiveVacb( SharedCacheMap, Vacb, ActivePage, PageIsDirty );
    }

    //
    //  Use try-finally to insure cleanup of the Open Count and Vacb on the
    //  way out.
    //

    try {

        //
        //  Increment open count to make sure the SharedCacheMap stays around,
        //  then release the spinlock so that we can call Mm.
        //

        if (SharedCacheMap != NULL) {

            //
            // Now loop to make sure that no one is currently caching the file.
            //

            if (UninitializeCacheMaps) {

                while (!IsListEmpty( &SharedCacheMap->PrivateList )) {

                    PrivateCacheMap = CONTAINING_RECORD( SharedCacheMap->PrivateList.Flink,
                                                         PRIVATE_CACHE_MAP,
                                                         PrivateLinks );

                    CcUninitializeCacheMap( PrivateCacheMap->FileObject, NULL, NULL );
                }
            }

            //
            //  Now, let's unmap and purge here.
            //
            //  We still need to wait for any dangling cache read or writes.
            //
            //  In fact we have to loop and wait because the lazy writer can
            //  sneak in and do an CcGetVirtualAddressIfMapped, and we are not
            //  synchronized.
            //

            while ((SharedCacheMap->Vacbs != NULL) &&
                   !CcUnmapVacbArray( SharedCacheMap, FileOffset, Length, FALSE )) {

                CcWaitOnActiveCount( SharedCacheMap );
            }
        }

        //
        //  Purge failures are extremely rare if there are no user mapped sections.
        //  However, it is possible that we will get one from our own mapping, if
        //  the file is being lazy deleted from a previous open.  For that case
        //  we wait here until the purge succeeds, so that we are not left with
        //  old user file data.  Although Length is actually invariant in this loop,
        //  we do need to keep checking that we are allowed to truncate in case a
        //  user maps the file during a delay.
        //

        while (!(PurgeWorked = MmPurgeSection(SectionObjectPointer,
                                              FileOffset,
                                              Length,
                                              (BOOLEAN)((SharedCacheMap !=NULL) &&
                                                        ARGUMENT_PRESENT(FileOffset)))) &&
               (Length == 0) &&
               MmCanFileBeTruncated(SectionObjectPointer, FileOffset)) {

            (VOID)KeDelayExecutionThread( KernelMode, FALSE, &CcCollisionDelay );
        }
    
    } finally {

        //
        //  Reduce the open count on the SharedCacheMap if there was one.
        //

        if (SharedCacheMap != NULL) {

            //
            //  Serialize again to decrement the open count.
            //

            CcAcquireMasterLock( &OldIrql );

            CcDecrementOpenCount( SharedCacheMap, 'scPF' );

            if ((SharedCacheMap->OpenCount == 0) &&
                !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
                (SharedCacheMap->DirtyPages == 0)) {

                //
                //  Move to the dirty list.
                //

                RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
                InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                                &SharedCacheMap->SharedCacheMapLinks );

                //
                //  Make sure the Lazy Writer will wake up, because we
                //  want him to delete this SharedCacheMap.
                //

                LazyWriter.OtherWork = TRUE;
                if (!LazyWriter.ScanActive) {
                    CcScheduleLazyWriteScan();
                }
            }

            CcReleaseMasterLock( OldIrql );
        }
    }

    DebugTrace(-1, me, "CcPurgeCacheSection -> %02lx\n", PurgeWorked );

    return PurgeWorked;
}


//
//  Internal support routine.
//

VOID
CcUnmapAndPurge(
    IN PSHARED_CACHE_MAP SharedCacheMap
    )

/*++

Routine Description:

    This routine may be called to unmap and purge a section, causing Memory
    Management to throw the pages out and reset his notion of file size.

Arguments:

    SharedCacheMap - Pointer to SharedCacheMap of section to purge.

Return Value:

    None.

--*/

{
    PFILE_OBJECT FileObject;
    KIRQL OldIrql;

    FileObject = SharedCacheMap->FileObject;

    //
    //  Unmap all Vacbs
    //

    if (SharedCacheMap->Vacbs != NULL) {
        (VOID)CcUnmapVacbArray( SharedCacheMap, NULL, 0, FALSE );
    }

    //
    //  Now that the file is unmapped, we can purge the truncated
    //  pages from memory, if TRUNCATE_REQUIRED.  Note that since the
    //  entire section is being purged (FileSize == NULL), the purge
    //  and subsequent delete  of the SharedCacheMap should drop
    //  all references on the section and file object clearing the
    //  way for the Close Call and actual file delete to occur
    //  immediately.
    //

    if (FlagOn(SharedCacheMap->Flags, TRUNCATE_REQUIRED)) {

        DebugTrace( 0, mm, "MmPurgeSection:\n", 0 );
        DebugTrace( 0, mm, "    SectionObjectPointer = %08lx\n",
                    FileObject->SectionObjectPointer );
        DebugTrace2(0, mm, "    Offset = %08lx\n",
                    SharedCacheMap->FileSize.LowPart,
                    SharedCacheMap->FileSize.HighPart );

        CcPurgeCacheSection( FileObject->SectionObjectPointer,
                             NULL,
                             0,
                             FALSE );
    }
}


VOID
CcDeleteMbcb(
    IN PSHARED_CACHE_MAP SharedCacheMap
    )

/*++

Routine Description:

    This routine may be called to reset the Mbcb for a stream to say
    there are no dirty pages, and free all auxillary allocation.

Arguments:

    SharedCacheMap - Pointer to SharedCacheMap.

Return Value:

    None.

--*/

{
    PMBCB Mbcb;
    PBITMAP_RANGE BitmapRange;
    KIRQL OldIrql;
    ULONG DoDrain = FALSE;

    ExAcquireSpinLock( &SharedCacheMap->BcbSpinLock, &OldIrql );

    Mbcb = SharedCacheMap->Mbcb;

    //
    //  Is there an Mbcb?
    //

    if (Mbcb != NULL) {

        //
        //  First deduct the dirty pages we are getting rid of.
        //

        CcAcquireMasterLockAtDpcLevel();
        CcTotalDirtyPages -= Mbcb->DirtyPages;
        SharedCacheMap->DirtyPages -= Mbcb->DirtyPages;
        CcReleaseMasterLockFromDpcLevel();

        //
        //  Now loop through all of the ranges.
        //

        while (!IsListEmpty(&Mbcb->BitmapRanges)) {

            //
            //  Get next range and remove it from the list.
            //

            BitmapRange = (PBITMAP_RANGE)CONTAINING_RECORD( Mbcb->BitmapRanges.Flink,
                                                            BITMAP_RANGE,
                                                            Links );

            RemoveEntryList( &BitmapRange->Links );

            //
            //  If there is a bitmap, and it is not the initial embedded one, then
            //  delete it.
            //

            if ((BitmapRange->Bitmap != NULL) &&
                (BitmapRange->Bitmap != (PULONG)&Mbcb->BitmapRange2)) {

                DoDrain = TRUE;

                //
                //  Usually the bitmap is all zeros at this point, but it may not be.
                //

                if (BitmapRange->DirtyPages != 0) {
                    RtlZeroMemory( BitmapRange->Bitmap, MBCB_BITMAP_BLOCK_SIZE );
                }
                CcAcquireVacbLockAtDpcLevel();
                CcDeallocateVacbLevel( (PVACB *)BitmapRange->Bitmap, FALSE );
                CcReleaseVacbLockFromDpcLevel();
            }

            //
            //  If the range is not one of the initial embedded ranges, then delete it.
            //

            if ((BitmapRange < (PBITMAP_RANGE)Mbcb) &&
                (BitmapRange > (PBITMAP_RANGE)((PCHAR)Mbcb + sizeof(MBCB)))) {

                ExFreePool( BitmapRange );
            }
        }

        //
        //  Zero the pointer and get out.
        //

        SharedCacheMap->Mbcb = NULL;

        ExReleaseSpinLock( &SharedCacheMap->BcbSpinLock, OldIrql );

        //
        //  Now delete the Mbcb.
        //

        CcDeallocateBcb( (PBCB)Mbcb );

    } else {

        ExReleaseSpinLock( &SharedCacheMap->BcbSpinLock, OldIrql );
    }

    if (DoDrain) {
        CcDrainVacbLevelZone();
    }
}


VOID
CcSetDirtyPageThreshold (
    IN PFILE_OBJECT FileObject,
    IN ULONG DirtyPageThreshold
    )

/*++

Routine Description:

    This routine may be called to set a dirty page threshold for this
    stream.  The write throttling will kick in whenever the file system
    attempts to exceed the dirty page threshold for this file.

Arguments:

    FileObject - Supplies file object for the stream

    DirtyPageThreshold - Supplies the dirty page threshold for this stream,
                         or 0 for no threshold.

Return Value:

    None

Environment:

    The caller must guarantee exclusive access to the FsRtl header flags,
    for example, by calling this routine once during create of the structure
    containing the header.  Then it would call the routine again when actually
    caching the stream.

--*/

{
    KIRQL OldIrql;
    PSHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    if (SharedCacheMap != NULL) {

        SharedCacheMap->DirtyPageThreshold = DirtyPageThreshold;
    }

    //
    //  Test the flag before setting, in case the caller is no longer properly
    //  synchronized.
    //

    if (!FlagOn(((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->Flags,
                FSRTL_FLAG_LIMIT_MODIFIED_PAGES)) {

        SetFlag(((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->Flags,
                FSRTL_FLAG_LIMIT_MODIFIED_PAGES);
    }
}


VOID
CcZeroEndOfLastPage (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is only called by Mm before mapping a user view to
    a section.  If there is an uninitialized page at the end of the
    file, we zero it by freeing that page.

Parameters:

    FileObject - File object for section to be mapped

Return Value:

    None
--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    ULONG ActivePage;
    ULONG PageIsDirty;
    KIRQL OldIrql;
    PVOID NeedToZero = NULL;
    PVACB ActiveVacb = NULL;
    IO_STATUS_BLOCK Iosb;
    BOOLEAN PurgeResult;

    //
    //  See if we have an active Vacb, that we need to free.
    //

    FsRtlAcquireFileExclusive( FileObject );
    CcAcquireMasterLock( &OldIrql );
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    if (SharedCacheMap != NULL) {

        //
        //  See if there is an active vacb.
        //

        if ((SharedCacheMap->ActiveVacb != NULL) || ((NeedToZero = SharedCacheMap->NeedToZero) != NULL)) {

            CcIncrementOpenCount( SharedCacheMap, 'peZS' );
            GetActiveVacbAtDpcLevel( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
        }
    }

    CcReleaseMasterLock( OldIrql );

    //
    //  Remember in FsRtl header is there is a user section.
    //  If this is an advanced header then also acquire the mutex to access
    //  this field.
    //

    if (FlagOn( ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->Flags,
                FSRTL_FLAG_ADVANCED_HEADER )) {

        ExAcquireFastMutex( ((PFSRTL_ADVANCED_FCB_HEADER)FileObject->FsContext)->FastMutex );

        SetFlag( ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->Flags,
                 FSRTL_FLAG_USER_MAPPED_FILE );

        ExReleaseFastMutex( ((PFSRTL_ADVANCED_FCB_HEADER)FileObject->FsContext)->FastMutex );

    } else {

        SetFlag( ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->Flags,
                 FSRTL_FLAG_USER_MAPPED_FILE );
    }

    //
    //  Free the active vacb now so we don't deadlock if we have to purge
    //


    if ((ActiveVacb != NULL) || (NeedToZero != NULL)) {
        CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
    }


    if (FlagOn( ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->Flags2, FSRTL_FLAG2_PURGE_WHEN_MAPPED )) {

        if (FileObject->SectionObjectPointer->SharedCacheMap) {
            ASSERT( ((PSHARED_CACHE_MAP)(FileObject->SectionObjectPointer->SharedCacheMap))->VacbActiveCount == 0 );
        }

        CcFlushCache( FileObject->SectionObjectPointer, NULL, 0, &Iosb );
        PurgeResult = CcPurgeCacheSection( FileObject->SectionObjectPointer, NULL, 0, FALSE );

        if (FileObject->SectionObjectPointer->SharedCacheMap) {
            ASSERT( ((PSHARED_CACHE_MAP)(FileObject->SectionObjectPointer->SharedCacheMap))->VacbActiveCount == 0 );
        }
    }


    FsRtlReleaseFile( FileObject );

    //
    //  If the file is cached and we have a Vacb to free, we need to
    //  use the lazy writer callback to synchronize so no one will be
    //  extending valid data.
    //

    if ((ActiveVacb != NULL) || (NeedToZero != NULL)) {

        //
        //  Serialize again to decrement the open count.
        //

        CcAcquireMasterLock( &OldIrql );

        CcDecrementOpenCount( SharedCacheMap, 'peZF' );

        if ((SharedCacheMap->OpenCount == 0) &&
            !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
            (SharedCacheMap->DirtyPages == 0)) {

            //
            //  Move to the dirty list.
            //

            RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
            InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                            &SharedCacheMap->SharedCacheMapLinks );

            //
            //  Make sure the Lazy Writer will wake up, because we
            //  want him to delete this SharedCacheMap.
            //

            LazyWriter.OtherWork = TRUE;
            if (!LazyWriter.ScanActive) {
                CcScheduleLazyWriteScan();
            }
        }

        CcReleaseMasterLock( OldIrql );
    }
}


BOOLEAN
CcZeroData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER StartOffset,
    IN PLARGE_INTEGER EndOffset,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine attempts to zero the specified file data and deliver the
    correct I/O status.

    If the caller does not want to block (such as for disk I/O), then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to zero all of the requested data without
    blocking, then this routine will return FALSE.  However, if the
    required space is immediately accessible in the cache and no blocking is
    required, this routine zeros the data and returns TRUE.

    If the caller supplies Wait as TRUE, then this routine is guaranteed
    to zero the data and return TRUE.  If the correct space is immediately
    accessible in the cache, then no blocking will occur.  Otherwise,
    the necessary work will be initiated to read and/or free cache data,
    and the caller will be blocked until the data can be received.

    File system Fsd's should typically supply Wait = TRUE if they are
    processing a synchronous I/O requests, or Wait = FALSE if they are
    processing an asynchronous request.

    File system threads should supply Wait = TRUE.

    IMPORTANT NOTE: File systems which call this routine must be prepared
    to handle a special form of a write call where the Mdl is already
    supplied.  Namely, if Irp->MdlAddress is supplied, the file system
    must check the low order bit of Irp->MdlAddress->ByteOffset.  If it
    is set, that means that the Irp was generated in this routine and
    the file system must do two things:

        Decrement Irp->MdlAddress->ByteOffset and Irp->UserBuffer

        Clear Irp->MdlAddress immediately prior to completing the
        request, as this routine expects to reuse the Mdl and
        ultimately deallocate the Mdl itself.

Arguments:

    FileObject - pointer to the FileObject for which a range of bytes
                 is to be zeroed.  This FileObject may either be for
                 a cached file or a noncached file.  If the file is
                 not cached, then WriteThrough must be TRUE and
                 StartOffset and EndOffset must be on sector boundaries.

    StartOffset - Start offset in file to be zeroed.

    EndOffset - End offset in file to be zeroed.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not zeroed.

    TRUE - if the data has been zeroed.

Raises:

    STATUS_INSUFFICIENT_RESOURCES - If a pool allocation failure occurs.
        This can only occur if Wait was specified as TRUE.  (If Wait is
        specified as FALSE, and an allocation failure occurs, this
        routine simply returns FALSE.)

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    LARGE_INTEGER ToGo;
    ULONG ZeroBytes, ZeroTransfer;
    ULONG SectorMask;
    ULONG i;
    BOOLEAN WriteThrough;
    BOOLEAN AggressiveZero = FALSE;
    ULONG SavedState = 0;
    ULONG MaxZerosInCache = MAX_ZEROS_IN_CACHE;
    ULONG NumberOfColors = 1;

    PBCB Bcb = NULL;
    PCHAR Zeros = NULL;
    PMDL ZeroMdl = NULL;
    ULONG MaxBytesMappedInMdl = 0;
    BOOLEAN Result = TRUE;

    PPFN_NUMBER Page;
    ULONG SavedByteCount;
    LARGE_INTEGER SizeLeft;

    DebugTrace(+1, me, "CcZeroData\n", 0 );

    WriteThrough = (BOOLEAN)(((FileObject->Flags & FO_WRITE_THROUGH) != 0) ||
                   (FileObject->PrivateCacheMap == NULL));

    //
    //  If the caller specified Wait, but the FileObject is WriteThrough,
    //  then we need to just get out.
    //

    if (WriteThrough && !Wait) {

        DebugTrace(-1, me, "CcZeroData->FALSE (WriteThrough && !Wait)\n", 0 );

        return FALSE;
    }

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    SectorMask = IoGetRelatedDeviceObject(FileObject)->SectorSize - 1;

    FOffset = *StartOffset;

    //
    //  Calculate how much to zero this time.
    //

    ToGo.QuadPart = EndOffset->QuadPart - FOffset.QuadPart;

    //
    //  This magic number is what the fastpaths throttle on, and they will present
    //  non-sector aligned zeroing requests. As long as we will always handle them
    //  on the cached path, we are OK.
    //
    //  If we will not make the cached path, the request must be aligned.
    //
    
    ASSERT( ToGo.QuadPart <= 0x2000 ||
	    ((ToGo.LowPart & SectorMask) == 0  &&
	     (FOffset.LowPart & SectorMask) == 0));

    //
    //  We will only do zeroing in the cache if the caller is using a
    //  cached file object, and did not specify WriteThrough.  We are
    //  willing to zero some data in the cache if our total is not too
    //  much, or there is sufficient available pages.
    //

    if (((ToGo.QuadPart <= 0x2000) ||
         (MmAvailablePages >= ((MAX_ZEROS_IN_CACHE / PAGE_SIZE) * 4))) && !WriteThrough) {

        try {

            while (MaxZerosInCache != 0) {

                ULONG ReceivedLength;
                LARGE_INTEGER BeyondLastByte;

                if ( ToGo.QuadPart > (LONGLONG)MaxZerosInCache ) {

                    //
                    //  If Wait == FALSE, then there is no point in getting started,
                    //  because we would have to start all over again zeroing with
                    //  Wait == TRUE, since we would fall out of this loop and
                    //  start synchronously writing pages to disk.
                    //

                    if (!Wait) {

                        DebugTrace(-1, me, "CcZeroData -> FALSE\n", 0 );

                        try_return( Result = FALSE );
                    }
                }
                else {
                    MaxZerosInCache = ToGo.LowPart;
                }

                //
                //  Call local routine to Map or Access the file data, then zero the data,
                //  then call another local routine to free the data.  If we cannot map
                //  the data because of a Wait condition, return FALSE.
                //
                //  Note that this call may result in an exception, however, if it
                //  does no Bcb is returned and this routine has absolutely no
                //  cleanup to perform.  Therefore, we do not have a try-finally
                //  and we allow the possibility that we will simply be unwound
                //  without notice.
                //

                if (!CcPinFileData( FileObject,
                                    &FOffset,
                                    MaxZerosInCache,
                                    FALSE,
                                    TRUE,
                                    Wait,
                                    &Bcb,
                                    &CacheBuffer,
                                    &BeyondLastByte )) {

                    DebugTrace(-1, me, "CcZeroData -> FALSE\n", 0 );

                    try_return( Result = FALSE );
                }

                //
                //  Calculate how much data is described by Bcb starting at our desired
                //  file offset.  If it is more than we need, we will zero the whole thing
                //  anyway.
                //

                ReceivedLength = (ULONG)(BeyondLastByte.QuadPart - FOffset.QuadPart );

                //
                //  Now attempt to allocate an Mdl to describe the mapped data.
                //

                ZeroMdl = IoAllocateMdl( CacheBuffer,
                                         ReceivedLength,
                                         FALSE,
                                         FALSE,
                                         NULL );

                if (ZeroMdl == NULL) {

                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                //  It is necessary to probe and lock the pages, or else
                //  the pages may not still be in memory when we do the
                //  MmSetAddressRangeModified for the dirty Bcb.
                //

                MmDisablePageFaultClustering(&SavedState);
                MmProbeAndLockPages( ZeroMdl, KernelMode, IoReadAccess );
                MmEnablePageFaultClustering(SavedState);
                SavedState = 0;

                //
                //  Assume we did not get all the data we wanted, and set FOffset
                //  to the end of the returned data, and advance buffer pointer.
                //

                FOffset = BeyondLastByte;

                //
                //  Figure out how many bytes we are allowed to zero in the cache.
                //  Note it is possible we have zeroed a little more than our maximum,
                //  because we hit an existing Bcb that extended beyond the range.
                //

                if (MaxZerosInCache <= ReceivedLength) {
                    MaxZerosInCache = 0;
                }
                else {
                    MaxZerosInCache -= ReceivedLength;
                }

                //
                //  Now set the Bcb dirty.  We have to explicitly set the address
                //  range modified here, because that work otherwise gets deferred
                //  to the Lazy Writer.
                //

                MmSetAddressRangeModified( CacheBuffer, ReceivedLength );
                CcSetDirtyPinnedData( Bcb, NULL );

                //
                //  Unmap the data now
                //

                CcUnpinFileData( Bcb, FALSE, UNPIN );
                Bcb = NULL;

                //
                //  Unlock and free the Mdl (we only loop back if we crossed
                //  a 256KB boundary.
                //

                MmUnlockPages( ZeroMdl );
                IoFreeMdl( ZeroMdl );
                ZeroMdl = NULL;
            }

        try_exit: NOTHING;
        } finally {

            if (SavedState != 0) {
                MmEnablePageFaultClustering(SavedState);
            }

            //
            //  Clean up only necessary in abnormal termination.
            //

            if (Bcb != NULL) {

                CcUnpinFileData( Bcb, FALSE, UNPIN );
            }

            //
            //  Since the last thing in the above loop which can
            //  fail is the MmProbeAndLockPages, we only need to
            //  free the Mdl here.
            //

            if (ZeroMdl != NULL) {

                IoFreeMdl( ZeroMdl );
            }
        }

        //
        //  If hit a wait condition above, return it now.
        //

        if (!Result) {
            return FALSE;
        }

        //
        //  If we finished, get out nbow.
        //

        if ( FOffset.QuadPart >= EndOffset->QuadPart ) {
            return TRUE;
        }
    }

    //
    //  We either get here because we decided above not to zero anything in
    //  the cache directly, or else we zeroed up to our maximum and still
    //  have some left to zero direct to the file on disk.  In either case,
    //  we will now zero from FOffset to *EndOffset, and then flush this
    //  range in case the file is cached/mapped, and there are modified
    //  changes in memory.
    //

    //
    //  Round FOffset and EndOffset up to sector boundaries, since
    //  we will be doing disk I/O, and calculate size left.
    //

    ASSERT( (FOffset.LowPart & SectorMask) == 0 );

    FOffset.QuadPart += (LONGLONG)SectorMask;
    FOffset.LowPart &= ~SectorMask;
    SizeLeft.QuadPart = EndOffset->QuadPart + (LONGLONG)SectorMask;
    SizeLeft.LowPart &= ~SectorMask;
    SizeLeft.QuadPart -= FOffset.QuadPart;

    ASSERT( (FOffset.LowPart & SectorMask) == 0 );
    ASSERT( (SizeLeft.LowPart & SectorMask) == 0 );

    if (SizeLeft.QuadPart == 0) {
	return TRUE;
    }

    //
    //  try-finally to guarantee cleanup.
    //

    try {
        
        //
        //  Allocate a page to hold the zeros we will write, and
        //  zero it.
        //

        ZeroBytes = NumberOfColors * PAGE_SIZE;

        if (SizeLeft.HighPart == 0 && SizeLeft.LowPart < ZeroBytes) {
	    ZeroBytes = SizeLeft.LowPart;
        }

        Zeros = (PCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned, ZeroBytes, 'eZcC' );

        if (Zeros != NULL) {

            //
            //  Allocate and initialize an Mdl to describe the zeros
            //  we need to transfer.  Allocate to cover the maximum
            //  size required, and we will use and reuse it in the
            //  loop below, initialized correctly.
            //

            if (SizeLeft.HighPart == 0 && SizeLeft.LowPart < MAX_ZERO_TRANSFER) {

                ZeroTransfer = SizeLeft.LowPart;

            } else {

                //
                //  See how aggressive we can afford to be.
                //

                if (InterlockedIncrement( &CcAggressiveZeroCount ) <= CcAggressiveZeroThreshold) {
                    AggressiveZero = TRUE;
                    ZeroTransfer = MAX_ZERO_TRANSFER;
                } else {
                    InterlockedDecrement( &CcAggressiveZeroCount );
                    ZeroTransfer = MIN_ZERO_TRANSFER;
                }
            }

            //
            //  Since the maximum zero may start at a very aggresive level, fall back
            //  until we really have to give up.  Since filter drivers, filesystems and
            //  even storage drivers may need to map this Mdl, we have to pre-map it
            //  into system space so that we know enough PTEs are avaliable.  We also
            //  need to throttle our consumption of virtual addresses based on the size
            //  of the system and the number of parallel instances of this work outstanding.
            //  This may be a bit of overkill, but since running out of PTEs is a fatal
            //  event for the rest of the system, try to help out while still being fast.
            //

            while (TRUE) {

		//
		//  Spin down trying to get an MDL which can describe our operation.
		//
		
		while (TRUE) {

                    ZeroMdl = IoAllocateMdl( Zeros, ZeroTransfer, FALSE, FALSE, NULL );
		    
		    //
		    //  Throttle ourselves to what we've physically allocated.  Note that
		    //  we could have started with an odd multiple of this number.  If we
		    //  tried for exactly that size and failed, we're toast.
		    //
                    
		    if (ZeroMdl || ZeroTransfer == ZeroBytes) {
                        
			break;
                    }
		    
		    Fall_Back:
		    
		    //
		    //  Fallback by half and round down to a sector multiple.
		    //
			
		    ZeroTransfer /= 2;
		    ZeroTransfer &= ~SectorMask;
		    if (ZeroTransfer < ZeroBytes) {
			ZeroTransfer = ZeroBytes;
		    }

                    ASSERT( (ZeroTransfer & SectorMask) == 0 && ZeroTransfer != 0);
		}

                if (ZeroMdl == NULL) {
                    
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                //  If we have throttled all the way down, stop and just build a
		//  simple MDL describing our previous allocation.
                //

                if (ZeroTransfer == ZeroBytes) {

		    MmBuildMdlForNonPagedPool( ZeroMdl );
                    break;
                }

                //
                //  Now we will temporarily lock the allocated pages
                //  only, and then replicate the page frame numbers through
                //  the entire Mdl to keep writing the same pages of zeros.
                //
                //  It would be nice if Mm exported a way for us to not have
                //  to pull the Mdl apart and rebuild it ourselves, but this
                //  is so bizzare a purpose as to be tolerable.
                //

                SavedByteCount = ZeroMdl->ByteCount;
                ZeroMdl->ByteCount = ZeroBytes;
                MmBuildMdlForNonPagedPool( ZeroMdl );

                ZeroMdl->MdlFlags &= ~MDL_SOURCE_IS_NONPAGED_POOL;
                ZeroMdl->MdlFlags |= MDL_PAGES_LOCKED;
                ZeroMdl->MappedSystemVa = NULL;
                ZeroMdl->ByteCount = SavedByteCount;
                Page = MmGetMdlPfnArray( ZeroMdl );
                for (i = NumberOfColors;
                     i < (COMPUTE_PAGES_SPANNED( 0, SavedByteCount ));
                     i++) {

                    *(Page + i) = *(Page + i - NumberOfColors);
                }

                if (MmGetSystemAddressForMdlSafe( ZeroMdl, LowPagePriority ) == NULL) {

                    //
                    //  Blow away this Mdl and trim for the retry.  Since it didn't
                    //  get mapped, there is nothing fancy to do. 
                    //

                    IoFreeMdl( ZeroMdl );
		    goto Fall_Back;
                }

                break;
            }

        //
        //  We failed to allocate the space we wanted, so we will go to
        //  half of a page and limp along.
        //

        } else {

            //
	    //  Of course, if we have a device which has large sectors, that defines
	    //  the lower limit of our attempt.
	    //

	    if (IoGetRelatedDeviceObject(FileObject)->SectorSize < PAGE_SIZE / 2) {
		
		ZeroBytes = PAGE_SIZE / 2;
		Zeros = (PCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned, ZeroBytes, 'eZcC' );
	    }

            //
            //  If we cannot get even that much, then let's write a sector at a time.
            //

            if (Zeros == NULL) {
                
		ZeroBytes = IoGetRelatedDeviceObject(FileObject)->SectorSize;
                Zeros = (PCHAR)ExAllocatePoolWithTag( NonPagedPoolCacheAligned, ZeroBytes, 'eZcC' );

                //
                //  If we cannot get even the minimum, we have to give up.
                //

                if (Zeros == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
            }

            //
            //  Allocate and initialize an Mdl to describe the zeros
            //  we need to transfer.  Allocate to cover the maximum
            //  size required, and we will use and reuse it in the
            //  loop below, initialized correctly.
            //

            ZeroTransfer = ZeroBytes;
            ZeroMdl = IoAllocateMdl( Zeros, ZeroBytes, FALSE, FALSE, NULL );

	    ASSERT( (ZeroTransfer & SectorMask) == 0 );

            if (ZeroMdl == NULL) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Now we will lock and map the allocated pages.
            //

            MmBuildMdlForNonPagedPool( ZeroMdl );

            ASSERT( ZeroMdl->MappedSystemVa == Zeros );
        }

#ifdef MIPS
#ifdef MIPS_PREFILL
        RtlFillMemory( Zeros, ZeroBytes, 0xDD );
        KeSweepDcache( TRUE );
#endif
#endif

        //
        //  Zero the buffer now.
        //

        RtlZeroMemory( Zeros, ZeroBytes );

        //
        //  We have a mapped and zeroed range back by an MDL to use.  Note the
        //  size we have for cleanup, since we will possibly wind this down
        //  over the operation.
        //

        ASSERT( MmGetSystemAddressForMdl(ZeroMdl) );
        MaxBytesMappedInMdl = ZeroMdl->ByteCount;

        //
        //  Now loop to write buffers full of zeros through to the file
        //  until we reach the starting Vbn for the transfer.
        //

        ASSERT( ZeroTransfer != 0 &&
		(ZeroTransfer & SectorMask) == 0 &&
		(SizeLeft.LowPart & SectorMask) == 0 );

        while ( SizeLeft.QuadPart != 0 ) {

            IO_STATUS_BLOCK IoStatus;
            NTSTATUS Status;
            KEVENT Event;

            //
            //  See if we really need to write that many zeros, and
            //  trim the size back if not.
            //

            if ( (LONGLONG)ZeroTransfer > SizeLeft.QuadPart ) {

                ZeroTransfer = SizeLeft.LowPart;
            }

            //
            //  (Re)initialize the kernel event to FALSE.
            //

            KeInitializeEvent( &Event, NotificationEvent, FALSE );

            //
            //  Initiate and wait for the synchronous transfer.
            //

            ZeroMdl->ByteCount = ZeroTransfer;

            Status = IoSynchronousPageWrite( FileObject,
                                             ZeroMdl,
                                             &FOffset,
                                             &Event,
                                             &IoStatus );

            //
            //  If pending is returned (which is a successful status),
            //  we must wait for the request to complete.
            //

            if (Status == STATUS_PENDING) {
                KeWaitForSingleObject( &Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)NULL);
            }


            //
            //  If we got an error back in Status, then the Iosb
            //  was not written, so we will just copy the status
            //  there, then test the final status after that.
            //

            if (!NT_SUCCESS(Status)) {
                ExRaiseStatus( Status );
            }

            if (!NT_SUCCESS(IoStatus.Status)) {
                ExRaiseStatus( IoStatus.Status );
            }

            //
            //  If we succeeded, then update where we are at by how much
            //  we wrote, and loop back to see if there is more.
            //

            FOffset.QuadPart = FOffset.QuadPart + (LONGLONG)ZeroTransfer;
            SizeLeft.QuadPart = SizeLeft.QuadPart - (LONGLONG)ZeroTransfer;
        }
    }
    finally{

        //
        //  Clean up anything from zeroing pages on a noncached
        //  write.
        //

        if (ZeroMdl != NULL) {

            if ((MaxBytesMappedInMdl != 0) &&
                !FlagOn(ZeroMdl->MdlFlags, MDL_SOURCE_IS_NONPAGED_POOL)) {
                ZeroMdl->ByteCount = MaxBytesMappedInMdl;
                MmUnmapLockedPages (ZeroMdl->MappedSystemVa, ZeroMdl);
            }

            IoFreeMdl( ZeroMdl );
        }

        if (AggressiveZero) {
            InterlockedDecrement( &CcAggressiveZeroCount );
        }

        if (Zeros != NULL) {
            ExFreePool( Zeros );
        }

        DebugTrace(-1, me, "CcZeroData -> TRUE\n", 0 );
    }

    return TRUE;
}


PFILE_OBJECT
CcGetFileObjectFromSectionPtrs (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    )

/*++

This routine may be used to retrieve a pointer to the FileObject that the
Cache Manager is using for a given file from the Section Object Pointers
in the nonpaged File System structure Fcb.  The use of this function is
intended for exceptional use unrelated to the processing of user requests,
when the File System would otherwise not have a FileObject at its disposal.
An example is for mount verification.

Note that the File System is responsible for insuring that the File
Object does not go away while in use.  It is impossible for the Cache
Manager to guarantee this.

Arguments:

    SectionObjectPointer - A pointer to the Section Object Pointers
                           structure in the nonpaged Fcb.

Return Value:

    Pointer to the File Object, or NULL if the file is not cached or no
    longer cached

--*/

{
    KIRQL OldIrql;
    PFILE_OBJECT FileObject = NULL;

    //
    //  Serialize with Creation/Deletion of all Shared CacheMaps
    //

    CcAcquireMasterLock( &OldIrql );

    if (SectionObjectPointer->SharedCacheMap != NULL) {

        FileObject = ((PSHARED_CACHE_MAP)SectionObjectPointer->SharedCacheMap)->FileObject;
    }

    CcReleaseMasterLock( OldIrql );

    return FileObject;
}


PFILE_OBJECT
CcGetFileObjectFromBcb (
    IN PVOID Bcb
    )

/*++

This routine may be used to retrieve a pointer to the FileObject that the
Cache Manager is using for a given file from a Bcb of that file.

Note that the File System is responsible for insuring that the File
Object does not go away while in use.  It is impossible for the Cache
Manager to guarantee this.

Arguments:

    Bcb - A pointer to the pinned Bcb.

Return Value:

    Pointer to the File Object, or NULL if the file is not cached or no
    longer cached

--*/

{
    return ((PBCB)Bcb)->SharedCacheMap->FileObject;
}
