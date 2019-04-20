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

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, CcInitView)
#endif

/* GLOBALS *******************************************************************/

LIST_ENTRY DirtyVacbListHead;
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
 */
ULONG CcDirtyPageThreshold = 0;
ULONG CcTotalDirtyPages = 0;
LIST_ENTRY CcDeferredWrites;
KSPIN_LOCK CcDeferredWriteSpinLock;
LIST_ENTRY CcCleanSharedCacheMapList;

#if DBG
ULONG CcRosVacbIncRefCount_(PROS_VACB vacb, PCSTR file, INT line)
{
    ULONG Refs;

    Refs = InterlockedIncrement((PLONG)&vacb->ReferenceCount);
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p ++RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, Refs, vacb->Dirty, vacb->PageOut);
    }

    return Refs;
}
ULONG CcRosVacbDecRefCount_(PROS_VACB vacb, PCSTR file, INT line)
{
    ULONG Refs;

    Refs = InterlockedDecrement((PLONG)&vacb->ReferenceCount);
    ASSERT(!(Refs == 0 && vacb->Dirty));
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p --RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, Refs, vacb->Dirty, vacb->PageOut);
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
        DbgPrint("(%s:%i) VACB %p ==RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, Refs, vacb->Dirty, vacb->PageOut);
    }

    return Refs;
}
#endif


/* FUNCTIONS *****************************************************************/

VOID
NTAPI
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

            DPRINT1("  VACB 0x%p enabled, RefCount %lu, Dirty %u, PageOut %lu\n",
                    current, current->ReferenceCount, current->Dirty, current->PageOut );
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

NTSTATUS
NTAPI
CcRosFlushVacb (
    PROS_VACB Vacb)
{
    NTSTATUS Status;

    CcRosUnmarkDirtyVacb(Vacb, TRUE);

    Status = CcWriteVirtualAddress(Vacb);
    if (!NT_SUCCESS(Status))
    {
        CcRosMarkDirtyVacb(Vacb);
    }

    return Status;
}

NTSTATUS
NTAPI
CcRosFlushDirtyPages (
    ULONG Target,
    PULONG Count,
    BOOLEAN Wait,
    BOOLEAN CalledFromLazy)
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    BOOLEAN Locked;
    NTSTATUS Status;
    KIRQL OldIrql;

    DPRINT("CcRosFlushDirtyPages(Target %lu)\n", Target);

    (*Count) = 0;

    KeEnterCriticalRegion();
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    current_entry = DirtyVacbListHead.Flink;
    if (current_entry == &DirtyVacbListHead)
    {
        DPRINT("No Dirty pages\n");
    }

    while ((current_entry != &DirtyVacbListHead) && (Target > 0))
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    DirtyVacbListEntry);
        current_entry = current_entry->Flink;

        CcRosVacbIncRefCount(current);

        /* When performing lazy write, don't handle temporary files */
        if (CalledFromLazy &&
            BooleanFlagOn(current->SharedCacheMap->FileObject->Flags, FO_TEMPORARY_FILE))
        {
            CcRosVacbDecRefCount(current);
            continue;
        }

        /* Don't attempt to lazy write the files that asked not to */
        if (CalledFromLazy &&
            BooleanFlagOn(current->SharedCacheMap->Flags, WRITEBEHIND_DISABLED))
        {
            CcRosVacbDecRefCount(current);
            continue;
        }

        ASSERT(current->Dirty);

        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

        Locked = current->SharedCacheMap->Callbacks->AcquireForLazyWrite(
                     current->SharedCacheMap->LazyWriteContext, Wait);
        if (!Locked)
        {
            OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
            CcRosVacbDecRefCount(current);
            continue;
        }

        Status = CcRosFlushVacb(current);

        current->SharedCacheMap->Callbacks->ReleaseFromLazyWrite(
            current->SharedCacheMap->LazyWriteContext);

        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        CcRosVacbDecRefCount(current);

        if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE) &&
            (Status != STATUS_MEDIA_WRITE_PROTECTED))
        {
            DPRINT1("CC: Failed to flush VACB.\n");
        }
        else
        {
            ULONG PagesFreed;

            /* How many pages did we free? */
            PagesFreed = VACB_MAPPING_GRANULARITY / PAGE_SIZE;
            (*Count) += PagesFreed;

            /* Make sure we don't overflow target! */
            if (Target < PagesFreed)
            {
                /* If we would have, jump to zero directly */
                Target = 0;
            }
            else
            {
                Target -= PagesFreed;
            }
        }

        current_entry = DirtyVacbListHead.Flink;
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    KeLeaveCriticalRegion();

    DPRINT("CcRosFlushDirtyPages() finished\n");
    return STATUS_SUCCESS;
}

NTSTATUS
CcRosTrimCache (
    ULONG Target,
    ULONG Priority,
    PULONG NrFreed)
/*
 * FUNCTION: Try to free some memory from the file cache.
 * ARGUMENTS:
 *       Target - The number of pages to be freed.
 *       Priority - The priority of free (currently unused).
 *       NrFreed - Points to a variable where the number of pages
 *                 actually freed is returned.
 */
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    ULONG PagesFreed;
    KIRQL oldIrql;
    LIST_ENTRY FreeList;
    PFN_NUMBER Page;
    ULONG i;
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
        current_entry = current_entry->Flink;

        KeAcquireSpinLockAtDpcLevel(&current->SharedCacheMap->CacheMapLock);

        /* Reference the VACB */
        CcRosVacbIncRefCount(current);

        /* Check if it's mapped and not dirty */
        if (InterlockedCompareExchange((PLONG)&current->MappedCount, 0, 0) > 0 && !current->Dirty)
        {
            /* We have to break these locks because Cc sucks */
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
        }

        /* Dereference the VACB */
        Refs = CcRosVacbDecRefCount(current);

        /* Check if we can free this entry now */
        if (Refs < 2)
        {
            ASSERT(!current->Dirty);
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

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosReleaseVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    PROS_VACB Vacb,
    BOOLEAN Valid,
    BOOLEAN Dirty,
    BOOLEAN Mapped)
{
    ULONG Refs;
    ASSERT(SharedCacheMap);

    DPRINT("CcRosReleaseVacb(SharedCacheMap 0x%p, Vacb 0x%p, Valid %u)\n",
           SharedCacheMap, Vacb, Valid);

    Vacb->Valid = Valid;

    if (Dirty && !Vacb->Dirty)
    {
        CcRosMarkDirtyVacb(Vacb);
    }

    if (Mapped)
    {
        if (InterlockedIncrement((PLONG)&Vacb->MappedCount) == 1)
        {
            CcRosVacbIncRefCount(Vacb);
        }
    }

    Refs = CcRosVacbDecRefCount(Vacb);
    ASSERT(Refs > 0);

    return STATUS_SUCCESS;
}

/* Returns with VACB Lock Held! */
PROS_VACB
NTAPI
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

VOID
NTAPI
CcRosMarkDirtyVacb (
    PROS_VACB Vacb)
{
    KIRQL oldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    SharedCacheMap = Vacb->SharedCacheMap;

    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);

    ASSERT(!Vacb->Dirty);

    InsertTailList(&DirtyVacbListHead, &Vacb->DirtyVacbListEntry);
    CcTotalDirtyPages += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    Vacb->SharedCacheMap->DirtyPages += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    CcRosVacbIncRefCount(Vacb);

    /* Move to the tail of the LRU list */
    RemoveEntryList(&Vacb->VacbLruListEntry);
    InsertTailList(&VacbLruListHead, &Vacb->VacbLruListEntry);

    Vacb->Dirty = TRUE;

    KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

    /* Schedule a lazy writer run to now that we have dirty VACB */
    if (!LazyWriter.ScanActive)
    {
        CcScheduleLazyWriteScan(FALSE);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);
}

VOID
NTAPI
CcRosUnmarkDirtyVacb (
    PROS_VACB Vacb,
    BOOLEAN LockViews)
{
    KIRQL oldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    SharedCacheMap = Vacb->SharedCacheMap;

    if (LockViews)
    {
        oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
    }

    ASSERT(Vacb->Dirty);

    Vacb->Dirty = FALSE;

    RemoveEntryList(&Vacb->DirtyVacbListEntry);
    InitializeListHead(&Vacb->DirtyVacbListEntry);
    CcTotalDirtyPages -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    Vacb->SharedCacheMap->DirtyPages -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    CcRosVacbDecRefCount(Vacb);

    if (LockViews)
    {
        KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
        KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);
    }
}

NTSTATUS
NTAPI
CcRosMarkDirtyFile (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset)
{
    PROS_VACB Vacb;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosMarkDirtyVacb(SharedCacheMap 0x%p, FileOffset %I64u)\n",
           SharedCacheMap, FileOffset);

    Vacb = CcRosLookupVacb(SharedCacheMap, FileOffset);
    if (Vacb == NULL)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    CcRosReleaseVacb(SharedCacheMap, Vacb, Vacb->Valid, TRUE, FALSE);

    return STATUS_SUCCESS;
}

/*
 * Note: this is not the contrary function of
 * CcRosMapVacbInKernelSpace()
 */
NTSTATUS
NTAPI
CcRosUnmapVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    BOOLEAN NowDirty)
{
    PROS_VACB Vacb;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosUnmapVacb(SharedCacheMap 0x%p, FileOffset %I64u, NowDirty %u)\n",
           SharedCacheMap, FileOffset, NowDirty);

    Vacb = CcRosLookupVacb(SharedCacheMap, FileOffset);
    if (Vacb == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ASSERT(Vacb->MappedCount != 0);
    if (InterlockedDecrement((PLONG)&Vacb->MappedCount) == 0)
    {
        CcRosVacbDecRefCount(Vacb);
    }

    CcRosReleaseVacb(SharedCacheMap, Vacb, Vacb->Valid, NowDirty, FALSE);

    return STATUS_SUCCESS;
}

static
NTSTATUS
CcRosMapVacbInKernelSpace(
    PROS_VACB Vacb)
{
    ULONG i;
    NTSTATUS Status;
    ULONG_PTR NumberOfPages;
    PVOID BaseAddress = NULL;

    /* Create a memory area. */
    MmLockAddressSpace(MmGetKernelAddressSpace());
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                0, // nothing checks for VACB mareas, so set to 0
                                &BaseAddress,
                                VACB_MAPPING_GRANULARITY,
                                PAGE_READWRITE,
                                (PMEMORY_AREA*)&Vacb->MemoryArea,
                                0,
                                PAGE_SIZE);
    ASSERT(Vacb->BaseAddress == NULL);
    Vacb->BaseAddress = BaseAddress;
    MmUnlockAddressSpace(MmGetKernelAddressSpace());
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCreateMemoryArea failed with %lx for VACB %p\n", Status, Vacb);
        return Status;
    }

    ASSERT(((ULONG_PTR)Vacb->BaseAddress % PAGE_SIZE) == 0);
    ASSERT((ULONG_PTR)Vacb->BaseAddress > (ULONG_PTR)MmSystemRangeStart);
    ASSERT((ULONG_PTR)Vacb->BaseAddress + VACB_MAPPING_GRANULARITY - 1 > (ULONG_PTR)MmSystemRangeStart);

    /* Create a virtual mapping for this memory area */
    NumberOfPages = BYTES_TO_PAGES(VACB_MAPPING_GRANULARITY);
    for (i = 0; i < NumberOfPages; i++)
    {
        PFN_NUMBER PageFrameNumber;

        MI_SET_USAGE(MI_USAGE_CACHE);
        Status = MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &PageFrameNumber);
        if (PageFrameNumber == 0)
        {
            DPRINT1("Unable to allocate page\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        ASSERT(BaseAddress == Vacb->BaseAddress);
        ASSERT(i * PAGE_SIZE < VACB_MAPPING_GRANULARITY);
        ASSERT((ULONG_PTR)Vacb->BaseAddress + (i * PAGE_SIZE) >= (ULONG_PTR)BaseAddress);
        ASSERT((ULONG_PTR)Vacb->BaseAddress + (i * PAGE_SIZE) > (ULONG_PTR)MmSystemRangeStart);

        Status = MmCreateVirtualMapping(NULL,
                                        (PVOID)((ULONG_PTR)Vacb->BaseAddress + (i * PAGE_SIZE)),
                                        PAGE_READWRITE,
                                        &PageFrameNumber,
                                        1);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to create virtual mapping\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

    return STATUS_SUCCESS;
}

static
BOOLEAN
CcRosFreeUnusedVacb (
    PULONG Count)
{
    ULONG cFreed;
    BOOLEAN Freed;
    KIRQL oldIrql;
    PROS_VACB current;
    LIST_ENTRY FreeList;
    PLIST_ENTRY current_entry;

    cFreed = 0;
    Freed = FALSE;
    InitializeListHead(&FreeList);

    oldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);

    /* Browse all the available VACB */
    current_entry = VacbLruListHead.Flink;
    while (current_entry != &VacbLruListHead)
    {
        ULONG Refs;

        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    VacbLruListEntry);
        current_entry = current_entry->Flink;

        KeAcquireSpinLockAtDpcLevel(&current->SharedCacheMap->CacheMapLock);

        /* Only deal with unused VACB, we will free them */
        Refs = CcRosVacbGetRefCount(current);
        if (Refs < 2)
        {
            ASSERT(!current->Dirty);
            ASSERT(!current->MappedCount);
            ASSERT(Refs == 1);

            /* Reset and move to free list */
            RemoveEntryList(&current->CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InitializeListHead(&current->VacbLruListEntry);
            InsertHeadList(&FreeList, &current->CacheMapVacbListEntry);
        }

        KeReleaseSpinLockFromDpcLevel(&current->SharedCacheMap->CacheMapLock);

    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    /* And now, free any of the found VACB, that'll free memory! */
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
        ++cFreed;
    }

    /* If we freed at least one VACB, return success */
    if (cFreed != 0)
    {
        Freed = TRUE;
    }

    /* If caller asked for free count, return it */
    if (Count != NULL)
    {
        *Count = cFreed;
    }

    return Freed;
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
    BOOLEAN Retried;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosCreateVacb()\n");

    if (FileOffset >= SharedCacheMap->SectionSize.QuadPart)
    {
        *Vacb = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    current = ExAllocateFromNPagedLookasideList(&VacbLookasideList);
    current->BaseAddress = NULL;
    current->Valid = FALSE;
    current->Dirty = FALSE;
    current->PageOut = FALSE;
    current->FileOffset.QuadPart = ROUND_DOWN(FileOffset, VACB_MAPPING_GRANULARITY);
    current->SharedCacheMap = SharedCacheMap;
#if DBG
    if (SharedCacheMap->Trace)
    {
        DPRINT1("CacheMap 0x%p: new VACB: 0x%p\n", SharedCacheMap, current);
    }
#endif
    current->MappedCount = 0;
    current->ReferenceCount = 0;
    InitializeListHead(&current->CacheMapVacbListEntry);
    InitializeListHead(&current->DirtyVacbListEntry);
    InitializeListHead(&current->VacbLruListEntry);

    CcRosVacbIncRefCount(current);

    Retried = FALSE;
Retry:
    /* Map VACB in kernel space */
    Status = CcRosMapVacbInKernelSpace(current);
    if (!NT_SUCCESS(Status))
    {
        ULONG Freed;
        /* If no space left, try to prune unused VACB
         * to recover space to map our VACB
         * If it succeed, retry to map, otherwise
         * just fail.
         */
        if (!Retried && CcRosFreeUnusedVacb(&Freed))
        {
            DPRINT("Prunned %d VACB, trying again\n", Freed);
            Retried = TRUE;
            goto Retry;
        }

        ExFreeToNPagedLookasideList(&VacbLookasideList, current);
        return Status;
    }

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
    KeReleaseQueuedSpinLock(LockQueueMasterLock, oldIrql);

    MI_SET_USAGE(MI_USAGE_CACHE);
#if MI_TRACE_PFNS
    if ((SharedCacheMap->FileObject) && (SharedCacheMap->FileObject->FileName.Buffer))
    {
        PWCHAR pos;
        ULONG len = 0;
        pos = wcsrchr(SharedCacheMap->FileObject->FileName.Buffer, '\\');
        if (pos)
        {
            len = wcslen(pos) * sizeof(WCHAR);
            snprintf(MI_PFN_CURRENT_PROCESS_NAME, min(16, len), "%S", pos);
        }
        else
        {
            snprintf(MI_PFN_CURRENT_PROCESS_NAME, min(16, len), "%wZ", &SharedCacheMap->FileObject->FileName);
        }
    }
#endif

    /* Reference it to allow release */
    CcRosVacbIncRefCount(current);

    return Status;
}

NTSTATUS
NTAPI
CcRosGetVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PLONGLONG BaseOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
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
     * Return information about the VACB to the caller.
     */
    *UptoDate = current->Valid;
    *BaseAddress = current->BaseAddress;
    DPRINT("*BaseAddress %p\n", *BaseAddress);
    *Vacb = current;
    *BaseOffset = current->FileOffset.QuadPart;

    ASSERT(Refs > 1);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosRequestVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    PROS_VACB *Vacb)
/*
 * FUNCTION: Request a page mapping for a shared cache map
 */
{
    LONGLONG BaseOffset;

    ASSERT(SharedCacheMap);

    if (FileOffset % VACB_MAPPING_GRANULARITY != 0)
    {
        DPRINT1("Bad fileoffset %I64x should be multiple of %x",
                FileOffset, VACB_MAPPING_GRANULARITY);
        KeBugCheck(CACHE_MANAGER);
    }

    return CcRosGetVacb(SharedCacheMap,
                        FileOffset,
                        &BaseOffset,
                        BaseAddress,
                        UptoDate,
                        Vacb);
}

static
VOID
CcFreeCachePage (
    PVOID Context,
    MEMORY_AREA* MemoryArea,
    PVOID Address,
    PFN_NUMBER Page,
    SWAPENTRY SwapEntry,
    BOOLEAN Dirty)
{
    ASSERT(SwapEntry == 0);
    if (Page != 0)
    {
        ASSERT(MmGetReferenceCountPage(Page) == 1);
        MmReleasePageMemoryConsumer(MC_CACHE, Page);
    }
}

NTSTATUS
CcRosInternalFreeVacb (
    PROS_VACB Vacb)
/*
 * FUNCTION: Releases a VACB associated with a shared cache map
 */
{
    DPRINT("Freeing VACB 0x%p\n", Vacb);
#if DBG
    if (Vacb->SharedCacheMap->Trace)
    {
        DPRINT1("CacheMap 0x%p: deleting VACB: 0x%p\n", Vacb->SharedCacheMap, Vacb);
    }
#endif

    MmLockAddressSpace(MmGetKernelAddressSpace());
    MmFreeMemoryArea(MmGetKernelAddressSpace(),
                     Vacb->MemoryArea,
                     CcFreeCachePage,
                     NULL);
    MmUnlockAddressSpace(MmGetKernelAddressSpace());

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
    ASSERT(IsListEmpty(&Vacb->DirtyVacbListEntry));
    ASSERT(IsListEmpty(&Vacb->VacbLruListEntry));
    RtlFillMemory(Vacb, sizeof(*Vacb), 0xfd);
    ExFreeToNPagedLookasideList(&VacbLookasideList, Vacb);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
CcFlushCache (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    OUT PIO_STATUS_BLOCK IoStatus)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    LARGE_INTEGER Offset;
    LONGLONG RemainingLength;
    PROS_VACB current;
    NTSTATUS Status;

    CCTRACE(CC_API_DEBUG, "SectionObjectPointers=%p FileOffset=%p Length=%lu\n",
        SectionObjectPointers, FileOffset, Length);

    DPRINT("CcFlushCache(SectionObjectPointers 0x%p, FileOffset 0x%p, Length %lu, IoStatus 0x%p)\n",
           SectionObjectPointers, FileOffset, Length, IoStatus);

    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        SharedCacheMap = SectionObjectPointers->SharedCacheMap;
        ASSERT(SharedCacheMap);
        if (FileOffset)
        {
            Offset = *FileOffset;
            RemainingLength = Length;
        }
        else
        {
            Offset.QuadPart = 0;
            RemainingLength = SharedCacheMap->FileSize.QuadPart;
        }

        if (IoStatus)
        {
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
        }

        while (RemainingLength > 0)
        {
            current = CcRosLookupVacb(SharedCacheMap, Offset.QuadPart);
            if (current != NULL)
            {
                if (current->Dirty)
                {
                    Status = CcRosFlushVacb(current);
                    if (!NT_SUCCESS(Status) && IoStatus != NULL)
                    {
                        IoStatus->Status = Status;
                    }
                }

                CcRosReleaseVacb(SharedCacheMap, current, current->Valid, FALSE, FALSE);
            }

            Offset.QuadPart += VACB_MAPPING_GRANULARITY;
            RemainingLength -= min(RemainingLength, VACB_MAPPING_GRANULARITY);
        }
    }
    else
    {
        if (IoStatus)
        {
            IoStatus->Status = STATUS_INVALID_PARAMETER;
        }
    }
}

NTSTATUS
NTAPI
CcRosDeleteFileCache (
    PFILE_OBJECT FileObject,
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    PKIRQL OldIrql)
/*
 * FUNCTION: Releases the shared cache map associated with a file object
 */
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    LIST_ENTRY FreeList;

    ASSERT(SharedCacheMap);

    SharedCacheMap->OpenCount++;
    KeReleaseQueuedSpinLock(LockQueueMasterLock, *OldIrql);

    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, NULL);

    *OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    SharedCacheMap->OpenCount--;
    if (SharedCacheMap->OpenCount == 0)
    {
        FileObject->SectionObjectPointer->SharedCacheMap = NULL;

        /*
         * Release all VACBs
         */
        InitializeListHead(&FreeList);
        KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
        while (!IsListEmpty(&SharedCacheMap->CacheMapVacbListHead))
        {
            current_entry = RemoveTailList(&SharedCacheMap->CacheMapVacbListHead);
            KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InitializeListHead(&current->VacbLruListEntry);
            if (current->Dirty)
            {
                KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
                CcRosUnmarkDirtyVacb(current, FALSE);
                KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);
                DPRINT1("Freeing dirty VACB\n");
            }
            if (current->MappedCount != 0)
            {
                current->MappedCount = 0;
                NT_VERIFY(CcRosVacbDecRefCount(current) > 0);
                DPRINT1("Freeing mapped VACB\n");
            }
            InsertHeadList(&FreeList, &current->CacheMapVacbListEntry);

            KeAcquireSpinLockAtDpcLevel(&SharedCacheMap->CacheMapLock);
        }
#if DBG
        SharedCacheMap->Trace = FALSE;
#endif
        KeReleaseSpinLockFromDpcLevel(&SharedCacheMap->CacheMapLock);

        KeReleaseQueuedSpinLock(LockQueueMasterLock, *OldIrql);
        ObDereferenceObject(SharedCacheMap->FileObject);

        while (!IsListEmpty(&FreeList))
        {
            ULONG Refs;

            current_entry = RemoveTailList(&FreeList);
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            InitializeListHead(&current->CacheMapVacbListEntry);
            Refs = CcRosVacbDecRefCount(current);
#if DBG // CORE-14578
            if (Refs != 0)
            {
                DPRINT1("Leaking VACB %p attached to %p (%I64d)\n", current, FileObject, current->FileOffset.QuadPart);
                DPRINT1("There are: %d references left\n", Refs);
                DPRINT1("Map: %d\n", current->MappedCount);
                DPRINT1("Dirty: %d\n", current->Dirty);
                if (FileObject->FileName.Length != 0)
                {
                    DPRINT1("File was: %wZ\n", &FileObject->FileName);
                }
                else if (FileObject->FsContext != NULL &&
                         ((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->NodeTypeCode == 0x0502 &&
                         ((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->NodeByteSize == 0x1F8 &&
                         ((PUNICODE_STRING)(((PUCHAR)FileObject->FsContext) + 0x100))->Length != 0)
                {
                    DPRINT1("File was: %wZ (FastFAT)\n", (PUNICODE_STRING)(((PUCHAR)FileObject->FsContext) + 0x100));
                }
                else
                {
                    DPRINT1("No name for the file\n");
                }
            }
#else
            ASSERT(Refs == 0);
#endif
        }

        *OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        RemoveEntryList(&SharedCacheMap->SharedCacheMapLinks);
        KeReleaseQueuedSpinLock(LockQueueMasterLock, *OldIrql);

        ExFreeToNPagedLookasideList(&SharedCacheMapLookasideList, SharedCacheMap);
        *OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
CcRosReferenceCache (
    PFILE_OBJECT FileObject)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    ASSERT(SharedCacheMap->OpenCount != 0);
    SharedCacheMap->OpenCount++;
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}

VOID
NTAPI
CcRosRemoveIfClosed (
    PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;

    DPRINT("CcRosRemoveIfClosed()\n");
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    SharedCacheMap = SectionObjectPointer->SharedCacheMap;
    if (SharedCacheMap && SharedCacheMap->OpenCount == 0)
    {
        CcRosDeleteFileCache(SharedCacheMap->FileObject, SharedCacheMap, &OldIrql);
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}


VOID
NTAPI
CcRosDereferenceCache (
    PFILE_OBJECT FileObject)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    if (SharedCacheMap->OpenCount > 0)
    {
        SharedCacheMap->OpenCount--;
        if (SharedCacheMap->OpenCount == 0)
        {
            KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
            MmFreeSectionSegments(SharedCacheMap->FileObject);

            OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
            CcRosDeleteFileCache(FileObject, SharedCacheMap, &OldIrql);
            KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

            return;
        }
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
}

NTSTATUS
NTAPI
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

            if (SharedCacheMap->OpenCount > 0)
            {
                SharedCacheMap->OpenCount--;
                if (SharedCacheMap->OpenCount == 0)
                {
                    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
                    MmFreeSectionSegments(SharedCacheMap->FileObject);

                    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
                    CcRosDeleteFileCache(FileObject, SharedCacheMap, &OldIrql);
                    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

                    return STATUS_SUCCESS;
                }
            }
        }
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
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
        SharedCacheMap->PinAccess = PinAccess;
        SharedCacheMap->DirtyPageThreshold = 0;
        SharedCacheMap->DirtyPages = 0;
        InitializeListHead(&SharedCacheMap->PrivateList);
        KeInitializeSpinLock(&SharedCacheMap->CacheMapLock);
        InitializeListHead(&SharedCacheMap->CacheMapVacbListHead);
        InitializeListHead(&SharedCacheMap->BcbList);
    }

    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    if (Allocated)
    {
        if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
        {
            ObReferenceObjectByPointer(FileObject,
                                       FILE_ALL_ACCESS,
                                       NULL,
                                       KernelMode);
            FileObject->SectionObjectPointer->SharedCacheMap = SharedCacheMap;

            InsertTailList(&CcCleanSharedCacheMapList, &SharedCacheMap->SharedCacheMapLinks);
        }
        else
        {
            ExFreeToNPagedLookasideList(&SharedCacheMapLookasideList, SharedCacheMap);
            SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
        }
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

VOID
INIT_FUNCTION
NTAPI
CcInitView (
    VOID)
{
    DPRINT("CcInitView()\n");

    InitializeListHead(&DirtyVacbListHead);
    InitializeListHead(&VacbLruListHead);
    InitializeListHead(&CcDeferredWrites);
    InitializeListHead(&CcCleanSharedCacheMapList);
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

    MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);

    CcInitCacheZeroPage();
}

#if DBG && defined(KDBG)
BOOLEAN
ExpKdbgExtFileCache(ULONG Argc, PCHAR Argv[])
{
    PLIST_ENTRY ListEntry;
    UNICODE_STRING NoName = RTL_CONSTANT_STRING(L"No name for File");

    KdbpPrint("  Usage Summary (in kb)\n");
    KdbpPrint("Shared\t\tValid\tDirty\tName\n");
    /* No need to lock the spin lock here, we're in DBG */
    for (ListEntry = CcCleanSharedCacheMapList.Flink;
         ListEntry != &CcCleanSharedCacheMapList;
         ListEntry = ListEntry->Flink)
    {
        PLIST_ENTRY Vacbs;
        ULONG Valid = 0, Dirty = 0;
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
            PROS_VACB Vacb;

            Vacb = CONTAINING_RECORD(Vacbs, ROS_VACB, CacheMapVacbListEntry);
            if (Vacb->Valid)
            {
                Valid += VACB_MAPPING_GRANULARITY / 1024;
            }
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
        KdbpPrint("%p\t%d\t%d\t%wZ%S\n", SharedCacheMap, Valid, Dirty, FileName, Extra);
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
#endif

/* EOF */
