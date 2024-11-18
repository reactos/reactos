/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* NOTES **********************************************************************
 *
 * This is not the NT implementation of a file cache nor anything much like
 * it.
 *
 * The general procedure for a filesystem to implement a read or write
 * dispatch routine is as follows
 *
 * (1) If caching for the FCB hasn't been initiated then so do by calling
 * CcInitializeFileCache.
 *
 * (2) For each 4k region which is being read or written obtain a cache page
 * by calling CcRequestCachePage.
 *
 * (3) If either the page is being read or not completely written, and it is
 * not up to date then read its data from the underlying medium. If the read
 * fails then call CcReleaseCachePage with VALID as FALSE and return a error.
 *
 * (4) Copy the data into or out of the page as necessary.
 *
 * (5) Release the cache page
 */
/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static LIST_ENTRY VacbLruListHead;

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST SharedCacheMapLookasideList;
static NPAGED_LOOKASIDE_LIST VacbLookasideList;

/* Internal vars (MS):
 * - Threshold above which lazy writer will start action
 * - Amount of dirty pages
 * - List for deferred writes
 * - Spinlock when dealing with the deferred list
 * - List for "clean" shared cache maps
 * - List for "dirty" shared cache maps
 */
ULONG CcDirtyPageThreshold = 0;
ULONG CcTotalDirtyPages = 0;
LIST_ENTRY CcDeferredWrites;
KSPIN_LOCK CcDeferredWriteSpinLock;
LIST_ENTRY CcCleanSharedCacheMapList;
LIST_ENTRY CcDirtySharedCacheMapList;

#if DBG
ULONG CcRosVacbIncRefCount_(PROS_VACB vacb, PCSTR file, INT line)
{
    ULONG Refs;

    Refs = InterlockedIncrement((PLONG)&vacb->ReferenceCount);
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p ++RefCount=%lu, PageOut %lu\n",
                 file, line, vacb, Refs, vacb->PageOut);
    }

    return Refs;
}
ULONG CcRosVacbDecRefCount_(PROS_VACB vacb, PCSTR file, INT line)
{
    ULONG Refs;
    BOOLEAN VacbTrace = vacb->SharedCacheMap->Trace;
    BOOLEAN VacbPageOut = vacb->PageOut;

    Refs = InterlockedDecrement((PLONG)&vacb->ReferenceCount);
    if (VacbTrace)
    {
        DbgPrint("(%s:%i) VACB %p --RefCount=%lu, PageOut %lu\n",
                 file, line, vacb, Refs, VacbPageOut);
    }

    if (Refs == 0)
    {
        CcRosInternalFreeVacb(vacb);
    }

    return Refs;
}
ULONG CcRosVacbGetRefCount_(PROS_VACB vacb, PCSTR file, INT line)
{
    ULONG Refs;

    Refs = InterlockedCompareExchange((PLONG)&vacb->ReferenceCount, 0, 0);
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p ==RefCount=%lu, PageOut %lu\n",
                 file, line, vacb, Refs, vacb->PageOut);
    }

    return Refs;
}
#endif


/* FUNCTIONS *****************************************************************/

VOID
CcRosTraceCacheMap (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    BOOLEAN Trace )
{
#if DBG
    KIRQL oldirql;
    PLIST_ENTRY current_entry;
    PROS_VACB current;

    if (!SharedCacheMap)
        return;

    SharedCacheMap->Trace = Trace;

    if (Trace)
    {
        DPRINT1("Enabling Tracing for CacheMap 0x%p:\n", SharedCacheMap);

        oldirql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

        current_entry = SharedCacheMap->CacheMapVacbListHead.Flink;
        while (current_entry != &SharedCacheMap->CacheMapVacbListHead)
        {
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            current_entry = current_entry->Flink;

            DPRINT1("  VACB 0x%p enabled, RefCount %lu, PageOut %lu, BaseAddress %p, FileOffset %I64d\n",
                    current, current->ReferenceCount, current->PageOut, current->BaseAddress, current->FileOffset.QuadPart);
        }

        KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
        KeReleaseQueuedSpinLock(LockQueueMasterLock, oldirql);
    }
    else
    {
        DPRINT1("Disabling Tracing for CacheMap 0x%p:\n", SharedCacheMap);
    }

#else
    UNREFERENCED_PARAMETER(SharedCacheMap);
    UNREFERENCED_PARAMETER(Trace);
#endif
}

static
NTSTATUS
CcRosDeleteFileCache(
    _In_ PFILE_OBJECT FileObject,
    _In_ PROS_SHARED_CACHE_MAP SharedCacheMap,
    _Inout_ PKIRQL OldIrql)
/*
 * FUNCTION: Releases the shared cache map associated with a file object
 */
{
    PLIST_ENTRY CurrentEntry;

    ASSERT(SharedCacheMap);
    ASSERT(SharedCacheMap == FileObject->SectionObjectPointer->SharedCacheMap);
    ASSERT(SharedCacheMap->OpenCount == 0);

    KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

    /* Remove all VACBs from the global list */
    CurrentEntry = SharedCacheMap->CacheMapVacbListHead.Flink;
    while (CurrentEntry != &SharedCacheMap->CacheMapVacbListHead)
    {
        PROS_VACB Vacb = CONTAINING_RECORD(CurrentEntry, ROS_VACB, CacheMapVacbListEntry);

        RemoveEntryList(&Vacb->VacbLruListEntry);
        InitializeListHead(&Vacb->VacbLruListEntry);

        CurrentEntry = CurrentEntry->Flink;
    }

    /* Make sure there is no trace anymore of this map */
    FileObject->SectionObjectPointer->SharedCacheMap = NULL;
    RemoveEntryList(&SharedCacheMap->SharedCacheMapLinks);

    KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
    KeReleaseQueuedSpinLock(LockQueueMasterLock, *OldIrql);

    /* Flush dirty data to disk, if needed */
    if (SharedCacheMap->DirtyPages != 0)
    {
        IO_STATUS_BLOCK FlushIosb;

        CcpFlushFileCache(SharedCacheMap, NULL, 0, FALSE, &FlushIosb);

        if (!NT_SUCCESS(FlushIosb.Status))
        {
            /* Complain. There's not much we can do */
            DPRINT1("Failed to flush dirty data to disk while deleting the file cache. Status: 0x%08x\n", FlushIosb.Status);
        }
    }

    /* Now that we're out of the locks, free everything for real */
    while (!IsListEmpty(&SharedCacheMap->CacheMapVacbListHead))
    {
        PROS_VACB Vacb = CONTAINING_RECORD(RemoveHeadList(&SharedCacheMap->CacheMapVacbListHead), ROS_VACB, CacheMapVacbListEntry);
        ULONG RefCount;

        InitializeListHead(&Vacb->CacheMapVacbListEntry);

        RefCount = CcRosVacbDecRefCount(Vacb);
#if DBG // CORE-14578
        if (RefCount != 0)
        {
            DPRINT1("Leaking VACB %p attached to %p (%I64d)\n", Vacb, FileObject, Vacb->FileOffset.QuadPart);
            DPRINT1("There are: %d references left\n", RefCount);
            DPRINT1("Map: %d\n", Vacb->MappedCount);
            if (FileObject->FileName.Length != 0)
            {
                DPRINT1("File was: %wZ\n", &FileObject->FileName);
            }
            else
            {
                DPRINT1("No name for the file\n");
            }
        }
#else
        (void)RefCount;
#endif
    }

    /* Release the references we own */
    if (SharedCacheMap->Section)
    {
        ObDereferenceObject(SharedCacheMap->Section);
    }
    ObDereferenceObject(SharedCacheMap->FileObject);

    ExFreeToNPagedLookasideList(&SharedCacheMapLookasideList, SharedCacheMap);

    /* Acquire the lock again for our caller */
    *OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    return STATUS_SUCCESS;
}

NTSTATUS
CcRosFlushDirtyPages (
    ULONG Target,
    PULONG Count,
    BOOLEAN Wait,
    BOOLEAN CalledFromLazy)
{
    PLIST_ENTRY current_entry;
    KIRQL OldIrql;
    BOOLEAN FlushAll = (Target == MAXULONG);

    DPRINT("CcRosFlushDirtyPages(Target %lu)\n", Target);

    (*Count) = 0;

    KeEnterCriticalRegion();
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

#if DBG
    if (IsListEmpty(&CcDirtySharedCacheMapList))
    {
        DPRINT("No Dirty pages\n");
    }
#endif

    current_entry = CcDirtySharedCacheMapList.Flink;
    while (((current_entry != &CcDirtySharedCacheMapList) && (Target > 0)) || FlushAll)
    {
        PROS_SHARED_CACHE_MAP current;
        BOOLEAN Locked;
        LARGE_INTEGER FlushOffset;
        LONGLONG ValidDataGoal;
        ULONG FlushLength;
        IO_STATUS_BLOCK FlushIosb;
        ULONG FlushedPages;

        if (current_entry == &CcDirtySharedCacheMapList)
        {
            ASSERT(FlushAll);
            if (IsListEmpty(&CcDirtySharedCacheMapList))
                break;
            current_entry = CcDirtySharedCacheMapList.Flink;
        }

        current = CONTAINING_RECORD(current_entry,
                                    ROS_SHARED_CACHE_MAP,
                                    SharedCacheMapLinks);
        current_entry = current_entry->Flink;

        /* When performing lazy write, don't handle temporary files */
        if (CalledFromLazy && BooleanFlagOn(current->FileObject->Flags, FO_TEMPORARY_FILE))
            continue;

        /* Don't attempt to lazy write the files that asked not to */
        if (CalledFromLazy && BooleanFlagOn(current->Flags, WRITEBEHIND_DISABLED))
            continue;

        /* Do not lazy-write the same file concurrently. Fastfat ASSERTS on that */
        if (BooleanFlagOn(current->Flags, SHARED_CACHE_MAP_IN_LAZYWRITE))
            continue;

        KeAcquireSpinLockAtDpcLevel(&current->CacheMapLock);

        ASSERT(current->DirtyPages != 0);

        /* A file system may indicate that the Cache Manager should not attempt
         * to track valid data by using the special value of MAXLONGLONG as the
         * valid data length in a CcSetFileSizes call.
         */
        if (current->ValidDataLength.QuadPart == MAXLONGLONG)
        {
            FlushOffset.QuadPart = 0;
            ValidDataGoal = current->FileSize.QuadPart;
        }
        else
        {
            FlushOffset = current->ValidDataLength;
            ValidDataGoal = current->FileSize.QuadPart - current->ValidDataLength.QuadPart;
        }

        KeReleaseSpinLockFromDpcLevel(&current->CacheMapLock);

        /* Mark for lazy write and keep a ref on the shared cache map */
        SetFlag(current->Flags, SHARED_CACHE_MAP_IN_LAZYWRITE);
        current->OpenCount++;

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

        if (FlushAll)
        {
            FlushLength = min(ValidDataGoal, MAX_FLUSH_LENGTH);
        }
        else
        {
            FlushLength = min(min((LONGLONG)Target * PAGE_SIZE, ValidDataGoal), MAX_FLUSH_LENGTH);
        }

        Locked = current->Callbacks->AcquireForLazyWrite(current->LazyWriteContext, Wait);
        if (!Locked)
        {
            DPRINT("Not locked!");
            ASSERT(!Wait);

            OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

            /* Release the shared cache map */
            ClearFlag(current->Flags, SHARED_CACHE_MAP_IN_LAZYWRITE);
            if (--current->OpenCount == 0)
                CcRosDeleteFileCache(current->FileObject, current, &OldIrql);

            continue;
        }

        CcpFlushFileCache(current, &FlushOffset, FlushLength, TRUE, &FlushIosb);

        current->Callbacks->ReleaseFromLazyWrite(current->LazyWriteContext);

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

        /* Release the shared cache map */
        ClearFlag(current->Flags, SHARED_CACHE_MAP_IN_LAZYWRITE);
        if (--current->OpenCount == 0)
            CcRosDeleteFileCache(current->FileObject, current, &OldIrql);

        /* How many pages did we flush? */
        FlushedPages = FlushIosb.Information / PAGE_SIZE;
        if (FlushedPages != 0)
        {
            (*Count) += FlushedPages;

            if (!FlushAll)
            {
                /* Make sure we don't overflow target! */
                if (Target < FlushedPages)
                {
                    /* If we would have, jump to zero directly */
                    Target = 0;
                }
                else
                {
                    Target -= FlushedPages;
                }
            }
        }

        if (!NT_SUCCESS(FlushIosb.Status))
        {
            DPRINT1("CC: Failed to flush file cache. Status 0x%08X\n", FlushIosb.Status);
            continue;
        }

        current_entry = CcDirtySharedCacheMapList.Flink;
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    KeLeaveCriticalRegion();

    DPRINT("CcRosFlushDirtyPages() finished\n");
    return STATUS_SUCCESS;
}

VOID
CcRosTrimCache(
    _In_ ULONG Target,
    _Out_ PULONG NrFreed)
/*
 * FUNCTION: Try to free some memory from the file cache.
 * ARGUMENTS:
 *       Target - The number of pages to be freed.
 *       NrFreed - Points to a variable where the number of pages
 *                 actually freed is returned.
 */
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    ULONG PagesFreed;
    KIRQL oldIrql;
    LIST_ENTRY FreeList;
    BOOLEAN FlushedPages = FALSE;

    DPRINT("CcRosTrimCache(Target %lu)\n", Target);

    InitializeListHead(&FreeList);

    *NrFreed = 0;

retry:
    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    current_entry = VacbLruListHead.Flink;
    while (current_entry != &VacbLruListHead)
    {
        ULONG Refs;

        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    VacbLruListEntry);

        KeAcquireSpinLockAtDpcLevel(&current->SharedCacheMap->CacheMapLock);

        /* Only keep iterating though the loop while the lock is held */
        current_entry = current_entry->Flink;

        /* Check if file cache associated with it is not dirty */
        if (current->SharedCacheMap->DirtyPages != 0)
        {
            KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);
            continue;
        }

        /* Check if it's mapped */
        if (InterlockedCompareExchange((PLONG)&current->MappedCount, 0, 0) > 0)
        {
            /* This code is never executed. It is left for reference only. */
#if 1
            DPRINT1("MmPageOutPhysicalAddress unexpectedly called\n");
            ASSERT(FALSE);
#else
            ULONG i;
            PFN_NUMBER Page;

            /* Reference the VACB */
            CcRosVacbIncRefCount(current);

            /* We have to break these locks to call MmPageOutPhysicalAddress */
            KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);
            KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

            /* Page out the VACB */
            for (i = 0; i < VACB_MAPPING_GRANULARITY / PAGE_SIZE; i++)
            {
                Page = (PFN_NUMBER)(MmGetPhysicalAddress((PUCHAR)current->BaseAddress + (i * PAGE_SIZE)).QuadPart >> PAGE_SHIFT);

                MmPageOutPhysicalAddress(Page);
            }

            /* Reacquire the locks */
            oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
            KeAcquireSpinLockAtDpcLevel(&current->SharedCacheMap->CacheMapLock);

            /* Dereference the VACB */
            CcRosVacbDecRefCount(current);
#endif
        }

        /* Check if we can free this VACB now */
        Refs = CcRosVacbGetRefCount(current);
        if (Refs < 2)
        {
            ASSERT(current->SharedCacheMap->DirtyPages == 0);
            ASSERT(!current->MappedCount);
            ASSERT(Refs == 1);

            RemoveEntryList(&current->CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InitializeListHead(&current->VacbLruListEntry);
            InsertHeadList(&FreeList, &current->CacheMapVacbListEntry);

            /* Calculate how many pages we freed for Mm */
            PagesFreed = min(VACB_MAPPING_GRANULARITY / PAGE_SIZE, Target);
            Target -= PagesFreed;
            (*NrFreed) += PagesFreed;
        }

        KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    /* Try flushing pages if we haven't met our target */
    if ((Target > 0) && !FlushedPages)
    {
        /* Flush dirty pages to disk */
        CcRosFlushDirtyPages(Target, &PagesFreed, FALSE, FALSE);
        FlushedPages = TRUE;

        /* We can only swap as many pages as we flushed */
        if (PagesFreed < Target) Target = PagesFreed;

        /* Check if we flushed anything */
        if (PagesFreed != 0)
        {
            /* Try again after flushing dirty pages */
            DPRINT("Flushed %lu dirty cache pages to disk\n", PagesFreed);
            goto retry;
        }
    }

    while (!IsListEmpty(&FreeList))
    {
        ULONG Refs;

        current_entry = RemoveHeadList(&FreeList);
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    CacheMapVacbListEntry);
        InitializeListHead(&current->CacheMapVacbListEntry);
        Refs = CcRosVacbDecRefCount(current);
        ASSERT(Refs == 0);
    }

    DPRINT("Evicted %lu cache pages\n", (*NrFreed));
}

VOID
CcRosReleaseVacb(
    _In_ PROS_VACB Vacb,
    _In_ BOOLEAN Mapped)
{
    ULONG Refs;

    DPRINT("CcRosReleaseVacb(SharedCacheMap 0x%p, Vacb 0x%p)\n", Vacb->SharedCacheMap, Vacb);

    if (Mapped)
    {
        if (InterlockedIncrement((PLONG)&Vacb->MappedCount) == 1)
        {
            CcRosVacbIncRefCount(Vacb);
        }
    }

    Refs = CcRosVacbDecRefCount(Vacb);
    ASSERT(Refs > 0);
}

/* Returns with VACB Lock Held! */
PROS_VACB
CcRosLookupVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset)
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    KIRQL oldIrql;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosLookupVacb(SharedCacheMap 0x%p, FileOffset %I64u)\n",
           SharedCacheMap, FileOffset);

    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

    current_entry = SharedCacheMap->CacheMapVacbListHead.Flink;
    while (current_entry != &SharedCacheMap->CacheMapVacbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    CacheMapVacbListEntry);
        if (IsPointInRange(current->FileOffset.QuadPart,
                           VACB_MAPPING_GRANULARITY,
                           FileOffset))
        {
            CcRosVacbIncRefCount(current);
            KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
            KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);
            return current;
        }
        if (current->FileOffset.QuadPart > FileOffset)
            break;
        current_entry = current_entry->Flink;
    }

    KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    return NULL;
}

NTSTATUS
CcpMarkDirtyFileCache(
    _In_ PROS_SHARED_CACHE_MAP SharedCacheMap,
    _In_ PVOID BaseAddress,
    _In_ ULONG Length)
{
    NTSTATUS Status;
    ULONG MarkedPages;

    /* Tell MM */
    Status = MmMakePagesDirty(NULL, BaseAddress, Length, &MarkedPages);

    if (MarkedPages != 0)
    {
        KIRQL OldIrql;
        BOOLEAN MarkDirty;

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

        MarkDirty = (SharedCacheMap->DirtyPages == 0);

        /* Update number of dirty pages and check dirty status */
        CcTotalDirtyPages += MarkedPages;
        SharedCacheMap->DirtyPages += MarkedPages;
        if (MarkDirty)
        {
            /* The file cache is now dirty, add to dirty list */
            RemoveEntryList(&SharedCacheMap->SharedCacheMapLinks);
            InsertTailList(&CcDirtySharedCacheMapList, &SharedCacheMap->SharedCacheMapLinks);
        }

        KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

        /* Schedule a lazy writer run to now that we have dirty data */
        if (!LazyWriter.ScanActive)
        {
            CcScheduleLazyWriteScan(FALSE);
        }

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    }

    return Status;
}

BOOLEAN
CcRosFreeOneUnusedVacb(
    VOID)
{
    KIRQL oldIrql;
    PLIST_ENTRY current_entry;
    PROS_VACB to_free = NULL;

    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    /* Browse all the available VACB */
    current_entry = VacbLruListHead.Flink;
    while ((current_entry != &VacbLruListHead) && (to_free == NULL))
    {
        ULONG Refs;
        PROS_VACB current;

        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    VacbLruListEntry);

        KeAcquireSpinLockAtDpcLevel(&current->SharedCacheMap->CacheMapLock);

        /* Only keep iterating though the loop while the lock is held */
        current_entry = current_entry->Flink;

        /* Check if file cache associated with it is not dirty */
        if (current->SharedCacheMap->DirtyPages != 0)
        {
            KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);
            continue;
        }

        /* Check if we can free this VACB now */
        Refs = CcRosVacbGetRefCount(current);
        if (Refs < 2)
        {
            ASSERT(current->SharedCacheMap->DirtyPages == 0);
            ASSERT(!current->MappedCount);
            ASSERT(Refs == 1);

            /* Reset it, this is the one we want to free */
            RemoveEntryList(&current->CacheMapVacbListEntry);
            InitializeListHead(&current->CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InitializeListHead(&current->VacbLruListEntry);

            to_free = current;
        }

        KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    /* And now, free the VACB that we found, if any. */
    if (to_free == NULL)
    {
        return FALSE;
    }

    /* This must be its last ref */
    NT_VERIFY(CcRosVacbDecRefCount(to_free) == 0);

    return TRUE;
}

static
NTSTATUS
CcRosCreateVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PROS_VACB *Vacb)
{
    PROS_VACB current;
    PROS_VACB previous;
    PLIST_ENTRY current_entry;
    NTSTATUS Status;
    KIRQL oldIrql;
    ULONG Refs;
    SIZE_T ViewSize = VACB_MAPPING_GRANULARITY;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosCreateVacb()\n");

    current = ExAllocateFromNPagedLookasideList(&VacbLookasideList);
    if (!current)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    current->BaseAddress = NULL;
    current->PageOut = FALSE;
    current->FileOffset.QuadPart = ROUND_DOWN(FileOffset, VACB_MAPPING_GRANULARITY);
    current->SharedCacheMap = SharedCacheMap;
    current->MappedCount = 0;
    current->ReferenceCount = 0;
    InitializeListHead(&current->CacheMapVacbListEntry);
    InitializeListHead(&current->VacbLruListEntry);

    CcRosVacbIncRefCount(current);

    while (TRUE)
    {
        /* Map VACB in system space */
        Status = MmMapViewInSystemSpaceEx(SharedCacheMap->Section, &current->BaseAddress, &ViewSize, &current->FileOffset, 0);
        if (NT_SUCCESS(Status))
        {
            break;
        }

        /*
         * If no space left, try to prune one unused VACB to recover space to map our VACB.
         * If it succeeds, retry to map, otherwise just fail.
         */
        if (!CcRosFreeOneUnusedVacb())
        {
            ExFreeToNPagedLookasideList(&VacbLookasideList, current);
            return Status;
        }
    }

#if DBG
    if (SharedCacheMap->Trace)
    {
        DPRINT1("CacheMap 0x%p: new VACB: 0x%p, file offset %I64d, BaseAddress %p\n",
                SharedCacheMap, current, current->FileOffset.QuadPart, current->BaseAddress);
    }
#endif

    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    *Vacb = current;
    /* There is window between the call to CcRosLookupVacb
     * and CcRosCreateVacb. We must check if a VACB for the
     * file offset exist. If there is a VACB, we release
     * our newly created VACB and return the existing one.
     */
    KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
    current_entry = SharedCacheMap->CacheMapVacbListHead.Flink;
    previous = NULL;
    while (current_entry != &SharedCacheMap->CacheMapVacbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    CacheMapVacbListEntry);
        if (IsPointInRange(current->FileOffset.QuadPart,
                           VACB_MAPPING_GRANULARITY,
                           FileOffset))
        {
            CcRosVacbIncRefCount(current);
            KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
#if DBG
            if (SharedCacheMap->Trace)
            {
                DPRINT1("CacheMap 0x%p: deleting newly created VACB 0x%p ( found existing one 0x%p )\n",
                        SharedCacheMap,
                        (*Vacb),
                        current);
            }
#endif
            KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

            Refs = CcRosVacbDecRefCount(*Vacb);
            ASSERT(Refs == 0);

            *Vacb = current;
            return STATUS_SUCCESS;
        }
        if (current->FileOffset.QuadPart < FileOffset)
        {
            ASSERT(previous == NULL ||
                   previous->FileOffset.QuadPart < current->FileOffset.QuadPart);
            previous = current;
        }
        if (current->FileOffset.QuadPart > FileOffset)
            break;
        current_entry = current_entry->Flink;
    }
    /* There was no existing VACB. */
    current = *Vacb;
    if (previous)
    {
        InsertHeadList(&previous->CacheMapVacbListEntry, &current->CacheMapVacbListEntry);
    }
    else
    {
        InsertHeadList(&SharedCacheMap->CacheMapVacbListHead, &current->CacheMapVacbListEntry);
    }
    KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
    InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);

    /* Reference it to allow release */
    CcRosVacbIncRefCount(current);

    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    return Status;
}

BOOLEAN
CcRosEnsureVacbResident(
    _In_ PROS_VACB Vacb,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN NoRead,
    _In_ ULONG Offset,
    _In_ ULONG Length
)
{
    PVOID BaseAddress;

    ASSERT((Offset + Length) <= VACB_MAPPING_GRANULARITY);

#if 0
    if ((Vacb->FileOffset.QuadPart + Offset) > Vacb->SharedCacheMap->SectionSize.QuadPart)
    {
        DPRINT1("Vacb read beyond the file size!\n");
        return FALSE;
    }
#endif

    BaseAddress = (PVOID)((ULONG_PTR)Vacb->BaseAddress + Offset);

    /* Check if the pages are resident */
    if (!MmArePagesResident(NULL, BaseAddress, Length))
    {
        if (!Wait)
        {
            return FALSE;
        }

        if (!NoRead)
        {
            PROS_SHARED_CACHE_MAP SharedCacheMap = Vacb->SharedCacheMap;
            NTSTATUS Status = MmMakeDataSectionResident(SharedCacheMap->FileObject->SectionObjectPointer,
                                                        Vacb->FileOffset.QuadPart + Offset,
                                                        Length,
                                                        &SharedCacheMap->ValidDataLength);
            if (!NT_SUCCESS(Status))
                ExRaiseStatus(Status);
        }
    }

    return TRUE;
}


NTSTATUS
CcRosGetVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PROS_VACB *Vacb)
{
    PROS_VACB current;
    NTSTATUS Status;
    ULONG Refs;
    KIRQL OldIrql;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosGetVacb()\n");

    /*
     * Look for a VACB already mapping the same data.
     */
    current = CcRosLookupVacb(SharedCacheMap, FileOffset);
    if (current == NULL)
    {
        /*
         * Otherwise create a new VACB.
         */
        Status = CcRosCreateVacb(SharedCacheMap, FileOffset, &current);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    Refs = CcRosVacbGetRefCount(current);

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    /* Move to the tail of the LRU list */
    RemoveEntryList(&current->VacbLruListEntry);
    InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);

    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /*
     * Return the VACB to the caller.
     */
    *Vacb = current;

    ASSERT(Refs > 1);

    return STATUS_SUCCESS;
}

NTSTATUS
CcRosRequestVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PROS_VACB *Vacb)
/*
 * FUNCTION: Request a page mapping for a shared cache map
 */
{

    ASSERT(SharedCacheMap);

    if (FileOffset % VACB_MAPPING_GRANULARITY != 0)
    {
        DPRINT1("Bad fileoffset %I64x should be multiple of %x",
                FileOffset, VACB_MAPPING_GRANULARITY);
        KeBugCheck(CACHE_MANAGER);
    }

    return CcRosGetVacb(SharedCacheMap,
                        FileOffset,
                        Vacb);
}

NTSTATUS
CcRosInternalFreeVacb (
    PROS_VACB Vacb)
/*
 * FUNCTION: Releases a VACB associated with a shared cache map
 */
{
    NTSTATUS Status;

    DPRINT("Freeing VACB 0x%p\n", Vacb);
#if DBG
    if (Vacb->SharedCacheMap->Trace)
    {
        DPRINT1("CacheMap 0x%p: deleting VACB: 0x%p\n", Vacb->SharedCacheMap, Vacb);
    }
#endif

    if (Vacb->ReferenceCount != 0)
    {
        DPRINT1("Invalid free: %ld\n", Vacb->ReferenceCount);
        if (Vacb->SharedCacheMap->FileObject && Vacb->SharedCacheMap->FileObject->FileName.Length)
        {
            DPRINT1("For file: %wZ\n", &Vacb->SharedCacheMap->FileObject->FileName);
        }
    }

    ASSERT(Vacb->ReferenceCount == 0);
    ASSERT(IsListEmpty(&Vacb->CacheMapVacbListEntry));
    ASSERT(IsListEmpty(&Vacb->VacbLruListEntry));

    /* Delete the mapping */
    Status = MmUnmapViewInSystemSpace(Vacb->BaseAddress);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to unmap VACB from System address space! Status 0x%08X\n", Status);
        ASSERT(FALSE);
        /* Proceed with the deĺetion anyway */
    }

    RtlFillMemory(Vacb, sizeof(*Vacb), 0xfd);
    ExFreeToNPagedLookasideList(&VacbLookasideList, Vacb);
    return STATUS_SUCCESS;
}

static
VOID
CcpUpdateFlushedFileCache(
    _In_ PROS_SHARED_CACHE_MAP SharedCacheMap,
    _In_ ULONG FlushedPages,
    _In_ LONGLONG NewVdl)
{
    KIRQL OldIrql;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

    /* Update number of dirty pages and check dirty status */
    CcTotalDirtyPages -= FlushedPages;
    SharedCacheMap->DirtyPages -= FlushedPages;
    if (SharedCacheMap->DirtyPages == 0)
    {
        /* The file cache is no longer dirty, remove from dirty list */
        RemoveEntryList(&SharedCacheMap->SharedCacheMapLinks);
        InsertTailList(&CcCleanSharedCacheMapList, &SharedCacheMap->SharedCacheMapLinks);
    }

    /* Update VDL */
    if (SharedCacheMap->ValidDataLength.QuadPart < NewVdl)
        SharedCacheMap->ValidDataLength.QuadPart = NewVdl;

    KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}

/**
 * @brief
 * Flushes all or a portion of a file cache to disk.
 *
 * @param[in] SharedCacheMap
 * Pointer to the shared cache map.
 *
 * @param[in] FileOffset
 * Pointer to a LARGE_INTEGER structure that specifies the starting byte offset
 * within the cached file.
 * If this parameter is NULL, the entire file will be flushed from the cache.
 *
 * @param[in] Length
 * Number of bytes to flush, starting at FileOffset. If FileOffset is NULL,
 * this parameter is ignored.
 *
 * @param[in] UpdateCacheMap
 * Set to TRUE if relevant fields of the shared cache map will be updated after
 * flushing. Otherwise set to FALSE.
 *
 * @param[out] IoStatus
 * Pointer to a IO_STATUS_BLOCK structure that receives the completion status
 * and information.
 */
VOID
CcpFlushFileCache(
    _In_ PROS_SHARED_CACHE_MAP SharedCacheMap,
    _In_opt_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN UpdateCacheMap,
    _Out_opt_ PIO_STATUS_BLOCK IoStatus)
{
    LONGLONG FlushStart, FlushEnd;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG FlushedPages = 0;
    BOOLEAN HaveFileLock = FALSE;

    ASSERT(SharedCacheMap);
    if (FileOffset)
    {
        FlushStart = FileOffset->QuadPart;
        Status = RtlLongLongAdd(FlushStart, Length, &FlushEnd);
        if (!NT_SUCCESS(Status))
            goto Quit;
    }
    else
    {
        FlushStart = 0;
        FlushEnd = SharedCacheMap->FileSize.QuadPart;
    }

    /* Lock file for flush, if we are not already the top-level */
    if (IoGetTopLevelIrp() != (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
    {
        Status = FsRtlAcquireFileForCcFlushEx(SharedCacheMap->FileObject);
        if (!NT_SUCCESS(Status))
            goto Quit;

        HaveFileLock = TRUE;
    }

    while (FlushStart < FlushEnd)
    {
        LARGE_INTEGER MmOffset;
        ULONG MmLength;
        ULONG MmFlushedPages;

        MmOffset.QuadPart = FlushStart;

        if (FlushEnd - (FlushEnd % VACB_MAPPING_GRANULARITY) <= FlushStart)
        {
            /* The whole range fits within a VACB chunk */
            MmLength = FlushEnd - FlushStart;
        }
        else
        {
            MmLength = VACB_MAPPING_GRANULARITY - (FlushStart % VACB_MAPPING_GRANULARITY);
        }

        Status = MmFlushSegment(SharedCacheMap->FileObject->SectionObjectPointer, &MmOffset, MmLength, &MmFlushedPages);
        FlushedPages += MmFlushedPages;
        if (!NT_SUCCESS(Status))
            break;

        /* Go to the next VACB start */
        FlushStart += MmLength;
    }

    if (HaveFileLock)
        FsRtlReleaseFileForCcFlush(SharedCacheMap->FileObject);

    if (UpdateCacheMap)
    {
        CcpUpdateFlushedFileCache(SharedCacheMap, FlushedPages, FlushStart);
    }
    else if (FlushedPages != 0)
    {
        KIRQL OldIrql;

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

        /* Update total dirty pages */
        CcTotalDirtyPages -= FlushedPages;

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    }

Quit:
    if (IoStatus)
    {
        IoStatus->Status = Status;
        IoStatus->Information = (ULONG_PTR)FlushedPages * PAGE_SIZE;
    }
}

/**
 * @implemented
 *
 * @brief
 * Flushes all or a portion of a cached file to disk.
 *
 * @param[in] SectionObjectPointers
 * Pointer to a SECTION_OBJECT_POINTERS structure containing the file object's
 * section object pointers.
 *
 * @param[in] FileOffset
 * See CcpFlushFileCache.
 *
 * @param[in] Length
 * See CcpFlushFileCache.
 *
 * @param[out] IoStatus
 * See CcpFlushFileCache.
 *
 * @remarks
 * The caller must be able to enter a wait state until all the data has been
 * flushed.
 */
VOID
NTAPI
CcFlushCache(
    _In_ PSECTION_OBJECT_POINTERS SectionObjectPointers,
    _In_opt_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _Out_opt_ PIO_STATUS_BLOCK IoStatus)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG FlushedPages = 0;

    CCTRACE(CC_API_DEBUG, "SectionObjectPointers=%p FileOffset=0x%I64X Length=%lu\n",
            SectionObjectPointers, FileOffset ? FileOffset->QuadPart : 0LL, Length);

    if (!SectionObjectPointers)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quit;
    }

    SharedCacheMap = SectionObjectPointers->SharedCacheMap;
    if (SharedCacheMap)
    {
        /* Call internal function */
        CcpFlushFileCache(SharedCacheMap, FileOffset, Length, TRUE, IoStatus);
        return;
    }
    else
    {
        /* Forward this to MM */
        Status = MmFlushSegment(SectionObjectPointers, FileOffset, Length, &FlushedPages);
    }

Quit:
    if (IoStatus)
    {
        IoStatus->Status = Status;
        IoStatus->Information = (ULONG_PTR)FlushedPages * PAGE_SIZE;
    }
}

NTSTATUS
CcRosReleaseFileCache (
    PFILE_OBJECT FileObject)
/*
 * FUNCTION: Called by the file system when a handle to a file object
 * has been closed.
 */
{
    KIRQL OldIrql;
    PPRIVATE_CACHE_MAP PrivateMap;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
    {
        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

        /* Closing the handle, so kill the private cache map
         * Before you event try to remove it from FO, always
         * lock the master lock, to be sure not to race
         * with a potential read ahead ongoing!
         */
        PrivateMap = FileObject->PrivateCacheMap;
        FileObject->PrivateCacheMap = NULL;

        if (PrivateMap != NULL)
        {
            /* Remove it from the file */
            KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
            RemoveEntryList(&PrivateMap->PrivateLinks);
            KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

            /* And free it. */
            if (PrivateMap != &SharedCacheMap->PrivateCacheMap)
            {
                ExFreePoolWithTag(PrivateMap, TAG_PRIVATE_CACHE_MAP);
            }
            else
            {
                PrivateMap->NodeTypeCode = 0;
            }

            ASSERT(SharedCacheMap->OpenCount > 0);

            SharedCacheMap->OpenCount--;
            if (SharedCacheMap->OpenCount == 0)
            {
                CcRosDeleteFileCache(FileObject, SharedCacheMap, &OldIrql);
            }
        }
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
CcRosInitializeFileCache (
    PFILE_OBJECT FileObject,
    PCC_FILE_SIZES FileSizes,
    BOOLEAN PinAccess,
    PCACHE_MANAGER_CALLBACKS CallBacks,
    PVOID LazyWriterContext)
/*
 * FUNCTION: Initializes a shared cache map for a file object
 */
{
    KIRQL OldIrql;
    BOOLEAN Allocated;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    DPRINT("CcRosInitializeFileCache(FileObject 0x%p)\n", FileObject);

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    Allocated = FALSE;
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    if (SharedCacheMap == NULL)
    {
        Allocated = TRUE;
        SharedCacheMap = ExAllocateFromNPagedLookasideList(&SharedCacheMapLookasideList);
        if (SharedCacheMap == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(SharedCacheMap, sizeof(*SharedCacheMap));
        SharedCacheMap->NodeTypeCode = NODE_TYPE_SHARED_MAP;
        SharedCacheMap->NodeByteSize = sizeof(*SharedCacheMap);
        SharedCacheMap->FileObject = FileObject;
        SharedCacheMap->Callbacks = CallBacks;
        SharedCacheMap->LazyWriteContext = LazyWriterContext;
        SharedCacheMap->SectionSize = FileSizes->AllocationSize;
        SharedCacheMap->FileSize = FileSizes->FileSize;
        SharedCacheMap->ValidDataLength = FileSizes->ValidDataLength;
        SharedCacheMap->PinAccess = PinAccess;
        SharedCacheMap->DirtyPageThreshold = 0;
        SharedCacheMap->DirtyPages = 0;
        InitializeListHead(&SharedCacheMap->PrivateList);
        KeInitializeSpinLock(&SharedCacheMap->CacheMapLock);
        InitializeListHead(&SharedCacheMap->CacheMapVacbListHead);
        InitializeListHead(&SharedCacheMap->BcbList);

        SharedCacheMap->Flags = SHARED_CACHE_MAP_IN_CREATION;

        ObReferenceObjectByPointer(FileObject,
                                   FILE_ALL_ACCESS,
                                   NULL,
                                   KernelMode);

        FileObject->SectionObjectPointer->SharedCacheMap = SharedCacheMap;

        //CcRosTraceCacheMap(SharedCacheMap, TRUE);
    }
    else if (SharedCacheMap->Flags & SHARED_CACHE_MAP_IN_CREATION)
    {
        /* The shared cache map is being created somewhere else. Wait for that to happen */
        KEVENT Waiter;
        PKEVENT PreviousWaiter = SharedCacheMap->CreateEvent;

        KeInitializeEvent(&Waiter, NotificationEvent, FALSE);
        SharedCacheMap->CreateEvent = &Waiter;

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

        KeWaitForSingleObject(&Waiter, Executive, KernelMode, FALSE, NULL);

        if (PreviousWaiter)
            KeSetEvent(PreviousWaiter, IO_NO_INCREMENT, FALSE);

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    }

    if (FileObject->PrivateCacheMap == NULL)
    {
        PPRIVATE_CACHE_MAP PrivateMap;

        /* Allocate the private cache map for this handle */
        if (SharedCacheMap->PrivateCacheMap.NodeTypeCode != 0)
        {
            PrivateMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(PRIVATE_CACHE_MAP), TAG_PRIVATE_CACHE_MAP);
        }
        else
        {
            PrivateMap = &SharedCacheMap->PrivateCacheMap;
        }

        if (PrivateMap == NULL)
        {
            /* If we also allocated the shared cache map for this file, kill it */
            if (Allocated)
            {
                RemoveEntryList(&SharedCacheMap->SharedCacheMapLinks);

                FileObject->SectionObjectPointer->SharedCacheMap = NULL;
                ObDereferenceObject(FileObject);
                ExFreeToNPagedLookasideList(&SharedCacheMapLookasideList, SharedCacheMap);
            }

            KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Initialize it */
        RtlZeroMemory(PrivateMap, sizeof(PRIVATE_CACHE_MAP));
        PrivateMap->NodeTypeCode = NODE_TYPE_PRIVATE_MAP;
        PrivateMap->ReadAheadMask = PAGE_SIZE - 1;
        PrivateMap->FileObject = FileObject;
        KeInitializeSpinLock(&PrivateMap->ReadAheadSpinLock);

        /* Link it to the file */
        KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
        InsertTailList(&SharedCacheMap->PrivateList, &PrivateMap->PrivateLinks);
        KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

        FileObject->PrivateCacheMap = PrivateMap;
        SharedCacheMap->OpenCount++;
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /* Create the section */
    if (Allocated)
    {
        NTSTATUS Status;

        ASSERT(SharedCacheMap->Section == NULL);

        Status = MmCreateSection(
            &SharedCacheMap->Section,
            SECTION_ALL_ACCESS,
            NULL,
            &SharedCacheMap->SectionSize,
            PAGE_READWRITE,
            SEC_RESERVE,
            NULL,
            FileObject);

        ASSERT(NT_SUCCESS(Status));

        if (!NT_SUCCESS(Status))
        {
            CcRosReleaseFileCache(FileObject);
            return Status;
        }

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

        InsertTailList(&CcCleanSharedCacheMapList, &SharedCacheMap->SharedCacheMapLinks);
        SharedCacheMap->Flags &= ~SHARED_CACHE_MAP_IN_CREATION;

        if (SharedCacheMap->CreateEvent)
        {
            KeSetEvent(SharedCacheMap->CreateEvent, IO_NO_INCREMENT, FALSE);
            SharedCacheMap->CreateEvent = NULL;
        }

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointers)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "SectionObjectPointers=%p\n", SectionObjectPointers);

    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        SharedCacheMap = SectionObjectPointers->SharedCacheMap;
        ASSERT(SharedCacheMap);
        return SharedCacheMap->FileObject;
    }
    return NULL;
}

CODE_SEG("INIT")
VOID
NTAPI
CcInitView (
    VOID)
{
    DPRINT("CcInitView()\n");

    InitializeListHead(&VacbLruListHead);
    InitializeListHead(&CcDeferredWrites);
    InitializeListHead(&CcCleanSharedCacheMapList);
    InitializeListHead(&CcDirtySharedCacheMapList);
    KeInitializeSpinLock(&CcDeferredWriteSpinLock);
    ExInitializeNPagedLookasideList(&iBcbLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(INTERNAL_BCB),
                                    TAG_BCB,
                                    20);
    ExInitializeNPagedLookasideList(&SharedCacheMapLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(ROS_SHARED_CACHE_MAP),
                                    TAG_SHARED_CACHE_MAP,
                                    20);
    ExInitializeNPagedLookasideList(&VacbLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(ROS_VACB),
                                    TAG_VACB,
                                    20);

    CcInitCacheZeroPage();
}

#if DBG && defined(KDBG)

#include <kdbg/kdb.h>

BOOLEAN
ExpKdbgExtFileCache(ULONG Argc, PCHAR Argv[])
{
    PLIST_ENTRY ListEntry;
    UNICODE_STRING NoName = RTL_CONSTANT_STRING(L"No name for File");

    KdbpPrint("  Usage Summary (in kb)\n");
    KdbpPrint("Shared\t\tMapped\tDirty\tName\n");
    /* No need to lock the spin lock here, we're in DBG */
    for (ListEntry = CcCleanSharedCacheMapList.Flink;
         ListEntry != &CcCleanSharedCacheMapList;
         ListEntry = ListEntry->Flink)
    {
        PLIST_ENTRY Vacbs;
        ULONG Mapped = 0, Dirty = 0;
        PROS_SHARED_CACHE_MAP SharedCacheMap;
        PUNICODE_STRING FileName;
        PWSTR Extra = L"";

        SharedCacheMap = CONTAINING_RECORD(ListEntry, ROS_SHARED_CACHE_MAP, SharedCacheMapLinks);

        /* Dirty size */
        Dirty = (SharedCacheMap->DirtyPages * PAGE_SIZE) / 1024;

        /* First, count for all the associated VACB */
        for (Vacbs = SharedCacheMap->CacheMapVacbListHead.Flink;
             Vacbs != &SharedCacheMap->CacheMapVacbListHead;
             Vacbs = Vacbs->Flink)
        {
            Mapped += VACB_MAPPING_GRANULARITY / 1024;
        }

        /* Setup name */
        if (SharedCacheMap->FileObject != NULL &&
            SharedCacheMap->FileObject->FileName.Length != 0)
        {
            FileName = &SharedCacheMap->FileObject->FileName;
        }
        else if (SharedCacheMap->FileObject != NULL &&
                 SharedCacheMap->FileObject->FsContext != NULL &&
                 ((PFSRTL_COMMON_FCB_HEADER)(SharedCacheMap->FileObject->FsContext))->NodeTypeCode == 0x0502 &&
                 ((PFSRTL_COMMON_FCB_HEADER)(SharedCacheMap->FileObject->FsContext))->NodeByteSize == 0x1F8 &&
                 ((PUNICODE_STRING)(((PUCHAR)SharedCacheMap->FileObject->FsContext) + 0x100))->Length != 0)
        {
            FileName = (PUNICODE_STRING)(((PUCHAR)SharedCacheMap->FileObject->FsContext) + 0x100);
            Extra = L" (FastFAT)";
        }
        else
        {
            FileName = &NoName;
        }

        /* And print */
        KdbpPrint("%p\t%d\t%d\t%wZ%S\n", SharedCacheMap, Mapped, Dirty, FileName, Extra);
    }

    return TRUE;
}

BOOLEAN
ExpKdbgExtDefWrites(ULONG Argc, PCHAR Argv[])
{
    KdbpPrint("CcTotalDirtyPages:\t%lu (%lu Kb)\n", CcTotalDirtyPages,
              (CcTotalDirtyPages * PAGE_SIZE) / 1024);
    KdbpPrint("CcDirtyPageThreshold:\t%lu (%lu Kb)\n", CcDirtyPageThreshold,
              (CcDirtyPageThreshold * PAGE_SIZE) / 1024);
    KdbpPrint("MmAvailablePages:\t%lu (%lu Kb)\n", MmAvailablePages,
              (MmAvailablePages * PAGE_SIZE) / 1024);
    KdbpPrint("MmThrottleTop:\t\t%lu (%lu Kb)\n", MmThrottleTop,
              (MmThrottleTop * PAGE_SIZE) / 1024);
    KdbpPrint("MmThrottleBottom:\t%lu (%lu Kb)\n", MmThrottleBottom,
              (MmThrottleBottom * PAGE_SIZE) / 1024);
    KdbpPrint("MmModifiedPageListHead.Total:\t%lu (%lu Kb)\n", MmModifiedPageListHead.Total,
              (MmModifiedPageListHead.Total * PAGE_SIZE) / 1024);

    if (CcTotalDirtyPages >= CcDirtyPageThreshold)
    {
        KdbpPrint("CcTotalDirtyPages above the threshold, writes should be throttled\n");
    }
    else if (CcTotalDirtyPages + 64 >= CcDirtyPageThreshold)
    {
        KdbpPrint("CcTotalDirtyPages within 64 (max charge) pages of the threshold, writes may be throttled\n");
    }
    else
    {
        KdbpPrint("CcTotalDirtyPages below the threshold, writes should not be throttled\n");
    }

    return TRUE;
}

#endif // DBG && defined(KDBG)

/* EOF */
