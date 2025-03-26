/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/cacheman.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

BOOLEAN CcPfEnablePrefetcher;
PFSN_PREFETCHER_GLOBALS CcPfGlobals;
MM_SYSTEMSIZE CcCapturedSystemSize;

static ULONG BugCheckFileId = 0x4 << 16;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
CcPfInitializePrefetcher(VOID)
{
    /* Notify debugger */
    DbgPrintEx(DPFLTR_PREFETCHER_ID,
               DPFLTR_TRACE_LEVEL,
               "CCPF: InitializePrefetecher()\n");

    /* Setup the Prefetcher Data */
    InitializeListHead(&CcPfGlobals.ActiveTraces);
    InitializeListHead(&CcPfGlobals.CompletedTraces);
    ExInitializeFastMutex(&CcPfGlobals.CompletedTracesLock);

    /* FIXME: Setup the rest of the prefetecher */
}

CODE_SEG("INIT")
BOOLEAN
CcInitializeCacheManager(VOID)
{
    ULONG Thread;

    CcInitView();

    /* Initialize lazy-writer lists */
    InitializeListHead(&CcIdleWorkerThreadList);
    InitializeListHead(&CcExpressWorkQueue);
    InitializeListHead(&CcRegularWorkQueue);
    InitializeListHead(&CcPostTickWorkQueue);

    /* Define lazy writer threshold and the amount of workers,
      * depending on the system type
      */
    CcCapturedSystemSize = MmQuerySystemSize();
    switch (CcCapturedSystemSize)
    {
        case MmSmallSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 8;
            break;

        case MmMediumSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 4;
            break;

        case MmLargeSystem:
            CcNumberWorkerThreads = ExCriticalWorkerThreads - 2;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 8 + MmNumberOfPhysicalPages / 4;
            break;

        default:
            CcNumberWorkerThreads = 1;
            CcDirtyPageThreshold = MmNumberOfPhysicalPages / 8;
            break;
    }

    /* Allocate a work item for all our threads */
    for (Thread = 0; Thread < CcNumberWorkerThreads; ++Thread)
    {
        PWORK_QUEUE_ITEM Item;

        Item = ExAllocatePoolWithTag(NonPagedPool, sizeof(WORK_QUEUE_ITEM), 'qWcC');
        if (Item == NULL)
        {
            CcBugCheck(0, 0, 0);
        }

        /* By default, it's obviously idle */
        ExInitializeWorkItem(Item, CcWorkerThread, Item);
        InsertTailList(&CcIdleWorkerThreadList, &Item->List);
    }

    /* Initialize our lazy writer */
    RtlZeroMemory(&LazyWriter, sizeof(LazyWriter));
    InitializeListHead(&LazyWriter.WorkQueue);
    /* Delay activation of the lazy writer */
    KeInitializeDpc(&LazyWriter.ScanDpc, CcScanDpc, NULL);
    KeInitializeTimer(&LazyWriter.ScanTimer);

    /* Lookaside list for our work items */
    ExInitializeNPagedLookasideList(&CcTwilightLookasideList, NULL, NULL, 0, sizeof(WORK_QUEUE_ENTRY), 'KWcC', 0);

    return TRUE;
}

VOID
NTAPI
CcShutdownSystem(VOID)
{
    /* NOTHING TO DO */
}

/*
 * @unimplemented
 */
LARGE_INTEGER
NTAPI
CcGetFlushedValidData (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN BcbListHeld
    )
{
	LARGE_INTEGER i;

	UNIMPLEMENTED;

	i.QuadPart = 0;
	return i;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
CcRemapBcb (
    IN PVOID Bcb
    )
{
	UNIMPLEMENTED;

    return 0;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcScheduleReadAhead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length
	)
{
    KIRQL OldIrql;
    LARGE_INTEGER NewOffset;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    PPRIVATE_CACHE_MAP PrivateCacheMap;

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    PrivateCacheMap = FileObject->PrivateCacheMap;

    /* If file isn't cached, or if read ahead is disabled, this is no op */
    if (SharedCacheMap == NULL || PrivateCacheMap == NULL ||
        BooleanFlagOn(SharedCacheMap->Flags, READAHEAD_DISABLED))
    {
        return;
    }

    /* Round read length with read ahead mask */
    Length = ROUND_UP(Length, PrivateCacheMap->ReadAheadMask + 1);
    /* Compute the offset we'll reach */
    NewOffset.QuadPart = FileOffset->QuadPart + Length;

    /* Lock read ahead spin lock */
    KeAcquireSpinLock(&PrivateCacheMap->ReadAheadSpinLock, &OldIrql);
    /* Easy case: the file is sequentially read */
    if (BooleanFlagOn(FileObject->Flags, FO_SEQUENTIAL_ONLY))
    {
        /* If we went backward, this is no go! */
        if (NewOffset.QuadPart < PrivateCacheMap->ReadAheadOffset[1].QuadPart)
        {
            KeReleaseSpinLock(&PrivateCacheMap->ReadAheadSpinLock, OldIrql);
            return;
        }

        /* FIXME: hackish, but will do the job for now */
        PrivateCacheMap->ReadAheadOffset[1].QuadPart = NewOffset.QuadPart;
        PrivateCacheMap->ReadAheadLength[1] = Length;
    }
    /* Other cases: try to find some logic in that mess... */
    else
    {
        /* Let's check if we always read the same way (like going down in the file)
         * and pretend it's enough for now
         */
        if (PrivateCacheMap->FileOffset2.QuadPart >= PrivateCacheMap->FileOffset1.QuadPart &&
            FileOffset->QuadPart >= PrivateCacheMap->FileOffset2.QuadPart)
        {
            /* FIXME: hackish, but will do the job for now */
            PrivateCacheMap->ReadAheadOffset[1].QuadPart = NewOffset.QuadPart;
            PrivateCacheMap->ReadAheadLength[1] = Length;
        }
        else
        {
            /* FIXME: handle the other cases */
            KeReleaseSpinLock(&PrivateCacheMap->ReadAheadSpinLock, OldIrql);
            UNIMPLEMENTED_ONCE;
            return;
        }
    }

    /* If read ahead isn't active yet */
    if (!PrivateCacheMap->Flags.ReadAheadActive)
    {
        PWORK_QUEUE_ENTRY WorkItem;

        /* It's active now!
         * Be careful with the mask, you don't want to mess with node code
         */
        InterlockedOr((volatile long *)&PrivateCacheMap->UlongFlags, PRIVATE_CACHE_MAP_READ_AHEAD_ACTIVE);
        KeReleaseSpinLock(&PrivateCacheMap->ReadAheadSpinLock, OldIrql);

        /* Get a work item */
        WorkItem = ExAllocateFromNPagedLookasideList(&CcTwilightLookasideList);
        if (WorkItem != NULL)
        {
            /* Reference our FO so that it doesn't go in between */
            ObReferenceObject(FileObject);

            /* We want to do read ahead! */
            WorkItem->Function = ReadAhead;
            WorkItem->Parameters.Read.FileObject = FileObject;

            /* Queue in the read ahead dedicated queue */
            CcPostWorkQueue(WorkItem, &CcExpressWorkQueue);

            return;
        }

        /* Fail path: lock again, and revert read ahead active */
        KeAcquireSpinLock(&PrivateCacheMap->ReadAheadSpinLock, &OldIrql);
        InterlockedAnd((volatile long *)&PrivateCacheMap->UlongFlags, ~PRIVATE_CACHE_MAP_READ_AHEAD_ACTIVE);
    }

    /* Done (fail) */
    KeReleaseSpinLock(&PrivateCacheMap->ReadAheadSpinLock, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
CcSetAdditionalCacheAttributes (
	IN	PFILE_OBJECT	FileObject,
	IN	BOOLEAN		DisableReadAhead,
	IN	BOOLEAN		DisableWriteBehind
	)
{
    KIRQL OldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p DisableReadAhead=%d DisableWriteBehind=%d\n",
        FileObject, DisableReadAhead, DisableWriteBehind);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    if (DisableReadAhead)
    {
        SetFlag(SharedCacheMap->Flags, READAHEAD_DISABLED);
    }
    else
    {
        ClearFlag(SharedCacheMap->Flags, READAHEAD_DISABLED);
    }

    if (DisableWriteBehind)
    {
        /* FIXME: also set flag 0x200 */
        SetFlag(SharedCacheMap->Flags, WRITEBEHIND_DISABLED);
    }
    else
    {
        ClearFlag(SharedCacheMap->Flags, WRITEBEHIND_DISABLED);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetBcbOwnerPointer (
	IN	PVOID	Bcb,
	IN	PVOID	Owner
	)
{
    PINTERNAL_BCB iBcb = CONTAINING_RECORD(Bcb, INTERNAL_BCB, PFCB);

    CCTRACE(CC_API_DEBUG, "Bcb=%p Owner=%p\n",
        Bcb, Owner);

    if (!ExIsResourceAcquiredExclusiveLite(&iBcb->Lock) && !ExIsResourceAcquiredSharedLite(&iBcb->Lock))
    {
        DPRINT1("Current thread doesn't own resource!\n");
        return;
    }

    ExSetResourceOwnerPointer(&iBcb->Lock, Owner);
}

/*
 * @implemented
 */
VOID
NTAPI
CcSetDirtyPageThreshold (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		DirtyPageThreshold
	)
{
    PFSRTL_COMMON_FCB_HEADER Fcb;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p DirtyPageThreshold=%lu\n",
        FileObject, DirtyPageThreshold);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    if (SharedCacheMap != NULL)
    {
        SharedCacheMap->DirtyPageThreshold = DirtyPageThreshold;
    }

    Fcb = FileObject->FsContext;
    if (!BooleanFlagOn(Fcb->Flags, FSRTL_FLAG_LIMIT_MODIFIED_PAGES))
    {
        SetFlag(Fcb->Flags, FSRTL_FLAG_LIMIT_MODIFIED_PAGES);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
CcSetReadAheadGranularity (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		Granularity
	)
{
    PPRIVATE_CACHE_MAP PrivateMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p Granularity=%lu\n",
        FileObject, Granularity);

    PrivateMap = FileObject->PrivateCacheMap;
    PrivateMap->ReadAheadMask = Granularity - 1;
}
