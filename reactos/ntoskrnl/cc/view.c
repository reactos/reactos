/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
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

static LIST_ENTRY DirtyVacbListHead;
static LIST_ENTRY VacbLruListHead;
ULONG DirtyPageCount = 0;

KGUARDED_MUTEX ViewLock;

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST SharedCacheMapLookasideList;
static NPAGED_LOOKASIDE_LIST VacbLookasideList;

#if DBG
static void CcRosVacbIncRefCount_(PROS_VACB vacb, const char* file, int line)
{
    ++vacb->ReferenceCount;
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p ++RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, vacb->ReferenceCount, vacb->Dirty, vacb->PageOut);
    }
}
static void CcRosVacbDecRefCount_(PROS_VACB vacb, const char* file, int line)
{
    --vacb->ReferenceCount;
    if (vacb->SharedCacheMap->Trace)
    {
        DbgPrint("(%s:%i) VACB %p --RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, vacb->ReferenceCount, vacb->Dirty, vacb->PageOut);
    }
}
#define CcRosVacbIncRefCount(vacb) CcRosVacbIncRefCount_(vacb,__FILE__,__LINE__)
#define CcRosVacbDecRefCount(vacb) CcRosVacbDecRefCount_(vacb,__FILE__,__LINE__)
#else
#define CcRosVacbIncRefCount(vacb) (++((vacb)->ReferenceCount))
#define CcRosVacbDecRefCount(vacb) (--((vacb)->ReferenceCount))
#endif

NTSTATUS
CcRosInternalFreeVacb(PROS_VACB Vacb);


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

        KeAcquireGuardedMutex(&ViewLock);
        KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldirql);

        current_entry = SharedCacheMap->CacheMapVacbListHead.Flink;
        while (current_entry != &SharedCacheMap->CacheMapVacbListHead)
        {
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            current_entry = current_entry->Flink;

            DPRINT1("  VACB 0x%p enabled, RefCount %lu, Dirty %u, PageOut %lu\n",
                    current, current->ReferenceCount, current->Dirty, current->PageOut );
        }
        KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldirql);
        KeReleaseGuardedMutex(&ViewLock);
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
    KIRQL oldIrql;

    Status = CcWriteVirtualAddress(Vacb);
    if (NT_SUCCESS(Status))
    {
        KeAcquireGuardedMutex(&ViewLock);
        KeAcquireSpinLock(&Vacb->SharedCacheMap->CacheMapLock, &oldIrql);

        Vacb->Dirty = FALSE;
        RemoveEntryList(&Vacb->DirtyVacbListEntry);
        DirtyPageCount -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
        CcRosVacbDecRefCount(Vacb);

        KeReleaseSpinLock(&Vacb->SharedCacheMap->CacheMapLock, oldIrql);
        KeReleaseGuardedMutex(&ViewLock);
    }

    return Status;
}

NTSTATUS
NTAPI
CcRosFlushDirtyPages (
    ULONG Target,
    PULONG Count,
    BOOLEAN Wait)
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    BOOLEAN Locked;
    NTSTATUS Status;
    LARGE_INTEGER ZeroTimeout;

    DPRINT("CcRosFlushDirtyPages(Target %lu)\n", Target);

    (*Count) = 0;
    ZeroTimeout.QuadPart = 0;

    KeEnterCriticalRegion();
    KeAcquireGuardedMutex(&ViewLock);

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

        Locked = current->SharedCacheMap->Callbacks->AcquireForLazyWrite(
                     current->SharedCacheMap->LazyWriteContext, Wait);
        if (!Locked)
        {
            CcRosVacbDecRefCount(current);
            continue;
        }

        Status = CcRosAcquireVacbLock(current,
                                      Wait ? NULL : &ZeroTimeout);
        if (Status != STATUS_SUCCESS)
        {
            current->SharedCacheMap->Callbacks->ReleaseFromLazyWrite(
                current->SharedCacheMap->LazyWriteContext);
            CcRosVacbDecRefCount(current);
            continue;
        }

        ASSERT(current->Dirty);

        /* One reference is added above */
        if (current->ReferenceCount > 2)
        {
            CcRosReleaseVacbLock(current);
            current->SharedCacheMap->Callbacks->ReleaseFromLazyWrite(
                current->SharedCacheMap->LazyWriteContext);
            CcRosVacbDecRefCount(current);
            continue;
        }

        KeReleaseGuardedMutex(&ViewLock);

        Status = CcRosFlushVacb(current);

        CcRosReleaseVacbLock(current);
        current->SharedCacheMap->Callbacks->ReleaseFromLazyWrite(
            current->SharedCacheMap->LazyWriteContext);

        KeAcquireGuardedMutex(&ViewLock);
        CcRosVacbDecRefCount(current);

        if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE) &&
            (Status != STATUS_MEDIA_WRITE_PROTECTED))
        {
            DPRINT1("CC: Failed to flush VACB.\n");
        }
        else
        {
            (*Count) += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
            Target -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
        }

        current_entry = DirtyVacbListHead.Flink;
    }

    KeReleaseGuardedMutex(&ViewLock);
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
    KeAcquireGuardedMutex(&ViewLock);

    current_entry = VacbLruListHead.Flink;
    while (current_entry != &VacbLruListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    VacbLruListEntry);
        current_entry = current_entry->Flink;

        KeAcquireSpinLock(&current->SharedCacheMap->CacheMapLock, &oldIrql);

        /* Reference the VACB */
        CcRosVacbIncRefCount(current);

        /* Check if it's mapped and not dirty */
        if (current->MappedCount > 0 && !current->Dirty)
        {
            /* We have to break these locks because Cc sucks */
            KeReleaseSpinLock(&current->SharedCacheMap->CacheMapLock, oldIrql);
            KeReleaseGuardedMutex(&ViewLock);

            /* Page out the VACB */
            for (i = 0; i < VACB_MAPPING_GRANULARITY / PAGE_SIZE; i++)
            {
                Page = (PFN_NUMBER)(MmGetPhysicalAddress((PUCHAR)current->BaseAddress + (i * PAGE_SIZE)).QuadPart >> PAGE_SHIFT);

                MmPageOutPhysicalAddress(Page);
            }

            /* Reacquire the locks */
            KeAcquireGuardedMutex(&ViewLock);
            KeAcquireSpinLock(&current->SharedCacheMap->CacheMapLock, &oldIrql);
        }

        /* Dereference the VACB */
        CcRosVacbDecRefCount(current);

        /* Check if we can free this entry now */
        if (current->ReferenceCount == 0)
        {
            ASSERT(!current->Dirty);
            ASSERT(!current->MappedCount);

            RemoveEntryList(&current->CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InsertHeadList(&FreeList, &current->CacheMapVacbListEntry);

            /* Calculate how many pages we freed for Mm */
            PagesFreed = min(VACB_MAPPING_GRANULARITY / PAGE_SIZE, Target);
            Target -= PagesFreed;
            (*NrFreed) += PagesFreed;
        }

        KeReleaseSpinLock(&current->SharedCacheMap->CacheMapLock, oldIrql);
    }

    KeReleaseGuardedMutex(&ViewLock);

    /* Try flushing pages if we haven't met our target */
    if ((Target > 0) && !FlushedPages)
    {
        /* Flush dirty pages to disk */
        CcRosFlushDirtyPages(Target, &PagesFreed, FALSE);
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
        current_entry = RemoveHeadList(&FreeList);
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    CacheMapVacbListEntry);
        CcRosInternalFreeVacb(current);
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
    BOOLEAN WasDirty;
    KIRQL oldIrql;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosReleaseVacb(SharedCacheMap 0x%p, Vacb 0x%p, Valid %u)\n",
           SharedCacheMap, Vacb, Valid);

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);

    Vacb->Valid = Valid;

    WasDirty = Vacb->Dirty;
    Vacb->Dirty = Vacb->Dirty || Dirty;

    if (!WasDirty && Vacb->Dirty)
    {
        InsertTailList(&DirtyVacbListHead, &Vacb->DirtyVacbListEntry);
        DirtyPageCount += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    }

    if (Mapped)
    {
        Vacb->MappedCount++;
    }
    CcRosVacbDecRefCount(Vacb);
    if (Mapped && (Vacb->MappedCount == 1))
    {
        CcRosVacbIncRefCount(Vacb);
    }
    if (!WasDirty && Vacb->Dirty)
    {
        CcRosVacbIncRefCount(Vacb);
    }

    KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    CcRosReleaseVacbLock(Vacb);

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

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);

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
            KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
            KeReleaseGuardedMutex(&ViewLock);
            CcRosAcquireVacbLock(current, NULL);
            return current;
        }
        if (current->FileOffset.QuadPart > FileOffset)
            break;
        current_entry = current_entry->Flink;
    }

    KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);

    return NULL;
}

NTSTATUS
NTAPI
CcRosMarkDirtyVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset)
{
    PROS_VACB Vacb;
    KIRQL oldIrql;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosMarkDirtyVacb(SharedCacheMap 0x%p, FileOffset %I64u)\n",
           SharedCacheMap, FileOffset);

    Vacb = CcRosLookupVacb(SharedCacheMap, FileOffset);
    if (Vacb == NULL)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);

    if (!Vacb->Dirty)
    {
        InsertTailList(&DirtyVacbListHead, &Vacb->DirtyVacbListEntry);
        DirtyPageCount += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    }
    else
    {
        CcRosVacbDecRefCount(Vacb);
    }

    /* Move to the tail of the LRU list */
    RemoveEntryList(&Vacb->VacbLruListEntry);
    InsertTailList(&VacbLruListHead, &Vacb->VacbLruListEntry);

    Vacb->Dirty = TRUE;

    KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    CcRosReleaseVacbLock(Vacb);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosUnmapVacb (
    PROS_SHARED_CACHE_MAP SharedCacheMap,
    LONGLONG FileOffset,
    BOOLEAN NowDirty)
{
    PROS_VACB Vacb;
    BOOLEAN WasDirty;
    KIRQL oldIrql;

    ASSERT(SharedCacheMap);

    DPRINT("CcRosUnmapVacb(SharedCacheMap 0x%p, FileOffset %I64u, NowDirty %u)\n",
           SharedCacheMap, FileOffset, NowDirty);

    Vacb = CcRosLookupVacb(SharedCacheMap, FileOffset);
    if (Vacb == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);

    WasDirty = Vacb->Dirty;
    Vacb->Dirty = Vacb->Dirty || NowDirty;

    Vacb->MappedCount--;

    if (!WasDirty && NowDirty)
    {
        InsertTailList(&DirtyVacbListHead, &Vacb->DirtyVacbListEntry);
        DirtyPageCount += VACB_MAPPING_GRANULARITY / PAGE_SIZE;
    }

    CcRosVacbDecRefCount(Vacb);
    if (!WasDirty && NowDirty)
    {
        CcRosVacbIncRefCount(Vacb);
    }
    if (Vacb->MappedCount == 0)
    {
        CcRosVacbDecRefCount(Vacb);
    }

    KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    CcRosReleaseVacbLock(Vacb);

    return STATUS_SUCCESS;
}

static
NTSTATUS
CcRosMapVacb(
    PROS_VACB Vacb)
{
    ULONG i;
    NTSTATUS Status;
    ULONG_PTR NumberOfPages;

    /* Create a memory area. */
    MmLockAddressSpace(MmGetKernelAddressSpace());
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                0, // nothing checks for VACB mareas, so set to 0
                                &Vacb->BaseAddress,
                                VACB_MAPPING_GRANULARITY,
                                PAGE_READWRITE,
                                (PMEMORY_AREA*)&Vacb->MemoryArea,
                                0,
                                PAGE_SIZE);
    MmUnlockAddressSpace(MmGetKernelAddressSpace());
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCreateMemoryArea failed with %lx for VACB %p\n", Status, Vacb);
        return Status;
    }

    ASSERT(((ULONG_PTR)Vacb->BaseAddress % PAGE_SIZE) == 0);
    ASSERT((ULONG_PTR)Vacb->BaseAddress > (ULONG_PTR)MmSystemRangeStart);

    /* Create a virtual mapping for this memory area */
    NumberOfPages = BYTES_TO_PAGES(VACB_MAPPING_GRANULARITY);
    for (i = 0; i < NumberOfPages; i++)
    {
        PFN_NUMBER PageFrameNumber;

        Status = MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &PageFrameNumber);
        if (PageFrameNumber == 0)
        {
            DPRINT1("Unable to allocate page\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }

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
    current->DirtyVacbListEntry.Flink = NULL;
    current->DirtyVacbListEntry.Blink = NULL;
    current->ReferenceCount = 1;
    current->PinCount = 0;
    KeInitializeMutex(&current->Mutex, 0);
    CcRosAcquireVacbLock(current, NULL);
    KeAcquireGuardedMutex(&ViewLock);

    *Vacb = current;
    /* There is window between the call to CcRosLookupVacb
     * and CcRosCreateVacb. We must check if a VACB for the
     * file offset exist. If there is a VACB, we release
     * our newly created VACB and return the existing one.
     */
    KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);
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
            KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
#if DBG
            if (SharedCacheMap->Trace)
            {
                DPRINT1("CacheMap 0x%p: deleting newly created VACB 0x%p ( found existing one 0x%p )\n",
                        SharedCacheMap,
                        (*Vacb),
                        current);
            }
#endif
            CcRosReleaseVacbLock(*Vacb);
            KeReleaseGuardedMutex(&ViewLock);
            ExFreeToNPagedLookasideList(&VacbLookasideList, *Vacb);
            *Vacb = current;
            CcRosAcquireVacbLock(current, NULL);
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
    KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
    InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);
    KeReleaseGuardedMutex(&ViewLock);

    MI_SET_USAGE(MI_USAGE_CACHE);
#if MI_TRACE_PFNS
    if ((SharedCacheMap->FileObject) && (SharedCacheMap->FileObject->FileName.Buffer))
    {
        PWCHAR pos = NULL;
        ULONG len = 0;
        pos = wcsrchr(SharedCacheMap->FileObject->FileName.Buffer, '\\');
        len = wcslen(pos) * sizeof(WCHAR);
        if (pos) snprintf(MI_PFN_CURRENT_PROCESS_NAME, min(16, len), "%S", pos);
    }
#endif

    Status = CcRosMapVacb(current);
    if (!NT_SUCCESS(Status))
    {
        RemoveEntryList(&current->CacheMapVacbListEntry);
        RemoveEntryList(&current->VacbLruListEntry);
        CcRosReleaseVacbLock(current);
        ExFreeToNPagedLookasideList(&VacbLookasideList, current);
    }

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

    KeAcquireGuardedMutex(&ViewLock);

    /* Move to the tail of the LRU list */
    RemoveEntryList(&current->VacbLruListEntry);
    InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);

    KeReleaseGuardedMutex(&ViewLock);

    /*
     * Return information about the VACB to the caller.
     */
    *UptoDate = current->Valid;
    *BaseAddress = current->BaseAddress;
    DPRINT("*BaseAddress %p\n", *BaseAddress);
    *Vacb = current;
    *BaseOffset = current->FileOffset.QuadPart;
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
    KIRQL oldIrql;

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

                CcRosReleaseVacbLock(current);

                KeAcquireGuardedMutex(&ViewLock);
                KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);
                CcRosVacbDecRefCount(current);
                KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);
                KeReleaseGuardedMutex(&ViewLock);
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
    PROS_SHARED_CACHE_MAP SharedCacheMap)
/*
 * FUNCTION: Releases the shared cache map associated with a file object
 */
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    LIST_ENTRY FreeList;
    KIRQL oldIrql;

    ASSERT(SharedCacheMap);

    SharedCacheMap->OpenCount++;
    KeReleaseGuardedMutex(&ViewLock);

    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, NULL);

    KeAcquireGuardedMutex(&ViewLock);
    SharedCacheMap->OpenCount--;
    if (SharedCacheMap->OpenCount == 0)
    {
        FileObject->SectionObjectPointer->SharedCacheMap = NULL;

        /*
         * Release all VACBs
         */
        InitializeListHead(&FreeList);
        KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldIrql);
        while (!IsListEmpty(&SharedCacheMap->CacheMapVacbListHead))
        {
            current_entry = RemoveTailList(&SharedCacheMap->CacheMapVacbListHead);
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            if (current->Dirty)
            {
                RemoveEntryList(&current->DirtyVacbListEntry);
                DirtyPageCount -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
                DPRINT1("Freeing dirty VACB\n");
            }
            InsertHeadList(&FreeList, &current->CacheMapVacbListEntry);
        }
#if DBG
        SharedCacheMap->Trace = FALSE;
#endif
        KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldIrql);

        KeReleaseGuardedMutex(&ViewLock);
        ObDereferenceObject(SharedCacheMap->FileObject);

        while (!IsListEmpty(&FreeList))
        {
            current_entry = RemoveTailList(&FreeList);
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            CcRosInternalFreeVacb(current);
        }
        ExFreeToNPagedLookasideList(&SharedCacheMapLookasideList, SharedCacheMap);
        KeAcquireGuardedMutex(&ViewLock);
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
CcRosReferenceCache (
    PFILE_OBJECT FileObject)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    KeAcquireGuardedMutex(&ViewLock);
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    ASSERT(SharedCacheMap->OpenCount != 0);
    SharedCacheMap->OpenCount++;
    KeReleaseGuardedMutex(&ViewLock);
}

VOID
NTAPI
CcRosRemoveIfClosed (
    PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    DPRINT("CcRosRemoveIfClosed()\n");
    KeAcquireGuardedMutex(&ViewLock);
    SharedCacheMap = SectionObjectPointer->SharedCacheMap;
    if (SharedCacheMap && SharedCacheMap->OpenCount == 0)
    {
        CcRosDeleteFileCache(SharedCacheMap->FileObject, SharedCacheMap);
    }
    KeReleaseGuardedMutex(&ViewLock);
}


VOID
NTAPI
CcRosDereferenceCache (
    PFILE_OBJECT FileObject)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    KeAcquireGuardedMutex(&ViewLock);
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    if (SharedCacheMap->OpenCount > 0)
    {
        SharedCacheMap->OpenCount--;
        if (SharedCacheMap->OpenCount == 0)
        {
            MmFreeSectionSegments(SharedCacheMap->FileObject);
            CcRosDeleteFileCache(FileObject, SharedCacheMap);
        }
    }
    KeReleaseGuardedMutex(&ViewLock);
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
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    KeAcquireGuardedMutex(&ViewLock);

    if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
    {
        SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
        if (FileObject->PrivateCacheMap != NULL)
        {
            FileObject->PrivateCacheMap = NULL;
            if (SharedCacheMap->OpenCount > 0)
            {
                SharedCacheMap->OpenCount--;
                if (SharedCacheMap->OpenCount == 0)
                {
                    MmFreeSectionSegments(SharedCacheMap->FileObject);
                    CcRosDeleteFileCache(FileObject, SharedCacheMap);
                }
            }
        }
    }
    KeReleaseGuardedMutex(&ViewLock);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcTryToInitializeFileCache (
    PFILE_OBJECT FileObject)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    NTSTATUS Status;

    KeAcquireGuardedMutex(&ViewLock);

    ASSERT(FileObject->SectionObjectPointer);
    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    if (SharedCacheMap == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        if (FileObject->PrivateCacheMap == NULL)
        {
            FileObject->PrivateCacheMap = SharedCacheMap;
            SharedCacheMap->OpenCount++;
        }
        Status = STATUS_SUCCESS;
    }
    KeReleaseGuardedMutex(&ViewLock);

    return Status;
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
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    DPRINT("CcRosInitializeFileCache(FileObject 0x%p, SharedCacheMap 0x%p)\n",
           FileObject, SharedCacheMap);

    KeAcquireGuardedMutex(&ViewLock);
    if (SharedCacheMap == NULL)
    {
        SharedCacheMap = ExAllocateFromNPagedLookasideList(&SharedCacheMapLookasideList);
        if (SharedCacheMap == NULL)
        {
            KeReleaseGuardedMutex(&ViewLock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(SharedCacheMap, sizeof(*SharedCacheMap));
        ObReferenceObjectByPointer(FileObject,
                                   FILE_ALL_ACCESS,
                                   NULL,
                                   KernelMode);
        SharedCacheMap->FileObject = FileObject;
        SharedCacheMap->Callbacks = CallBacks;
        SharedCacheMap->LazyWriteContext = LazyWriterContext;
        SharedCacheMap->SectionSize = FileSizes->AllocationSize;
        SharedCacheMap->FileSize = FileSizes->FileSize;
        SharedCacheMap->PinAccess = PinAccess;
        KeInitializeSpinLock(&SharedCacheMap->CacheMapLock);
        InitializeListHead(&SharedCacheMap->CacheMapVacbListHead);
        FileObject->SectionObjectPointer->SharedCacheMap = SharedCacheMap;
    }
    if (FileObject->PrivateCacheMap == NULL)
    {
        FileObject->PrivateCacheMap = SharedCacheMap;
        SharedCacheMap->OpenCount++;
    }
    KeReleaseGuardedMutex(&ViewLock);

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
    KeInitializeGuardedMutex(&ViewLock);
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

/* EOF */
