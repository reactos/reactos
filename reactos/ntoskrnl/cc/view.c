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
static LIST_ENTRY VacbListHead;
static LIST_ENTRY VacbLruListHead;
static LIST_ENTRY ClosedListHead;
ULONG DirtyPageCount = 0;

KGUARDED_MUTEX ViewLock;

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST BcbLookasideList;
static NPAGED_LOOKASIDE_LIST VacbLookasideList;

#if DBG
static void CcRosVacbIncRefCount_(PROS_VACB vacb, const char* file, int line)
{
    ++vacb->ReferenceCount;
    if (vacb->Bcb->Trace)
    {
        DbgPrint("(%s:%i) VACB %p ++RefCount=%lu, Dirty %u, PageOut %lu\n",
                 file, line, vacb, vacb->ReferenceCount, vacb->Dirty, vacb->PageOut);
    }
}
static void CcRosVacbDecRefCount_(PROS_VACB vacb, const char* file, int line)
{
    --vacb->ReferenceCount;
    if (vacb->Bcb->Trace)
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
    PBCB Bcb,
    BOOLEAN Trace )
{
#if DBG
    KIRQL oldirql;
    PLIST_ENTRY current_entry;
    PROS_VACB current;

    if ( !Bcb )
        return;

    Bcb->Trace = Trace;

    if ( Trace )
    {
        DPRINT1("Enabling Tracing for CacheMap 0x%p:\n", Bcb );

        KeAcquireGuardedMutex(&ViewLock);
        KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);

        current_entry = Bcb->BcbVacbListHead.Flink;
        while (current_entry != &Bcb->BcbVacbListHead)
        {
            current = CONTAINING_RECORD(current_entry, ROS_VACB, BcbVacbListEntry);
            current_entry = current_entry->Flink;

            DPRINT1("  VACB 0x%p enabled, RefCount %lu, Dirty %u, PageOut %lu\n",
                    current, current->ReferenceCount, current->Dirty, current->PageOut );
        }
        KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
        KeReleaseGuardedMutex(&ViewLock);
    }
    else
    {
        DPRINT1("Disabling Tracing for CacheMap 0x%p:\n", Bcb );
    }

#else
    Bcb = Bcb;
    Trace = Trace;
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
        KeAcquireSpinLock(&Vacb->Bcb->BcbLock, &oldIrql);

        Vacb->Dirty = FALSE;
        RemoveEntryList(&Vacb->DirtyVacbListEntry);
        DirtyPageCount -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
        CcRosVacbDecRefCount(Vacb);

        KeReleaseSpinLock(&Vacb->Bcb->BcbLock, oldIrql);
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

        Locked = current->Bcb->Callbacks->AcquireForLazyWrite(
                     current->Bcb->LazyWriteContext, Wait);
        if (!Locked)
        {
            CcRosVacbDecRefCount(current);
            continue;
        }

        Status = KeWaitForSingleObject(&current->Mutex,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       Wait ? NULL : &ZeroTimeout);
        if (Status != STATUS_SUCCESS)
        {
            current->Bcb->Callbacks->ReleaseFromLazyWrite(
                current->Bcb->LazyWriteContext);
            CcRosVacbDecRefCount(current);
            continue;
        }

        ASSERT(current->Dirty);

        /* One reference is added above */
        if (current->ReferenceCount > 2)
        {
            KeReleaseMutex(&current->Mutex, FALSE);
            current->Bcb->Callbacks->ReleaseFromLazyWrite(
                current->Bcb->LazyWriteContext);
            CcRosVacbDecRefCount(current);
            continue;
        }

        KeReleaseGuardedMutex(&ViewLock);

        Status = CcRosFlushVacb(current);

        KeReleaseMutex(&current->Mutex, FALSE);
        current->Bcb->Callbacks->ReleaseFromLazyWrite(
            current->Bcb->LazyWriteContext);

        KeAcquireGuardedMutex(&ViewLock);
        CcRosVacbDecRefCount(current);

        if (!NT_SUCCESS(Status) &&  (Status != STATUS_END_OF_FILE))
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

        KeAcquireSpinLock(&current->Bcb->BcbLock, &oldIrql);

        /* Reference the VACB */
        CcRosVacbIncRefCount(current);

        /* Check if it's mapped and not dirty */
        if (current->MappedCount > 0 && !current->Dirty)
        {
            /* We have to break these locks because Cc sucks */
            KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
            KeReleaseGuardedMutex(&ViewLock);

            /* Page out the VACB */
            for (i = 0; i < VACB_MAPPING_GRANULARITY / PAGE_SIZE; i++)
            {
                Page = (PFN_NUMBER)(MmGetPhysicalAddress((PUCHAR)current->BaseAddress + (i * PAGE_SIZE)).QuadPart >> PAGE_SHIFT);

                MmPageOutPhysicalAddress(Page);
            }

            /* Reacquire the locks */
            KeAcquireGuardedMutex(&ViewLock);
            KeAcquireSpinLock(&current->Bcb->BcbLock, &oldIrql);
        }

        /* Dereference the VACB */
        CcRosVacbDecRefCount(current);

        /* Check if we can free this entry now */
        if (current->ReferenceCount == 0)
        {
            ASSERT(!current->Dirty);
            ASSERT(!current->MappedCount);

            RemoveEntryList(&current->BcbVacbListEntry);
            RemoveEntryList(&current->VacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            InsertHeadList(&FreeList, &current->BcbVacbListEntry);

            /* Calculate how many pages we freed for Mm */
            PagesFreed = min(VACB_MAPPING_GRANULARITY / PAGE_SIZE, Target);
            Target -= PagesFreed;
            (*NrFreed) += PagesFreed;
        }

        KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
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
                                    BcbVacbListEntry);
        CcRosInternalFreeVacb(current);
    }

    DPRINT("Evicted %lu cache pages\n", (*NrFreed));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosReleaseVacb (
    PBCB Bcb,
    PROS_VACB Vacb,
    BOOLEAN Valid,
    BOOLEAN Dirty,
    BOOLEAN Mapped)
{
    BOOLEAN WasDirty;
    KIRQL oldIrql;

    ASSERT(Bcb);

    DPRINT("CcRosReleaseVacb(Bcb 0x%p, Vacb 0x%p, Valid %u)\n",
           Bcb, Vacb, Valid);

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);

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

    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    KeReleaseMutex(&Vacb->Mutex, FALSE);

    return STATUS_SUCCESS;
}

/* Returns with VACB Lock Held! */
PROS_VACB
NTAPI
CcRosLookupVacb (
    PBCB Bcb,
    ULONG FileOffset)
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    KIRQL oldIrql;

    ASSERT(Bcb);

    DPRINT("CcRosLookupVacb(Bcb -x%p, FileOffset %lu)\n", Bcb, FileOffset);

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);

    current_entry = Bcb->BcbVacbListHead.Flink;
    while (current_entry != &Bcb->BcbVacbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    BcbVacbListEntry);
        if (IsPointInRange(current->FileOffset, VACB_MAPPING_GRANULARITY,
                           FileOffset))
        {
            CcRosVacbIncRefCount(current);
            KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
            KeReleaseGuardedMutex(&ViewLock);
            KeWaitForSingleObject(&current->Mutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            return current;
        }
        if (current->FileOffset > FileOffset)
            break;
        current_entry = current_entry->Flink;
    }

    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);

    return NULL;
}

NTSTATUS
NTAPI
CcRosMarkDirtyVacb (
    PBCB Bcb,
    ULONG FileOffset)
{
    PROS_VACB Vacb;
    KIRQL oldIrql;

    ASSERT(Bcb);

    DPRINT("CcRosMarkDirtyVacb(Bcb 0x%p, FileOffset %lu)\n", Bcb, FileOffset);

    Vacb = CcRosLookupVacb(Bcb, FileOffset);
    if (Vacb == NULL)
    {
        KeBugCheck(CACHE_MANAGER);
    }

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);

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

    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    KeReleaseMutex(&Vacb->Mutex, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosUnmapVacb (
    PBCB Bcb,
    ULONG FileOffset,
    BOOLEAN NowDirty)
{
    PROS_VACB Vacb;
    BOOLEAN WasDirty;
    KIRQL oldIrql;

    ASSERT(Bcb);

    DPRINT("CcRosUnmapVacb(Bcb 0x%p, FileOffset %lu, NowDirty %u)\n",
           Bcb, FileOffset, NowDirty);

    Vacb = CcRosLookupVacb(Bcb, FileOffset);
    if (Vacb == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KeAcquireGuardedMutex(&ViewLock);
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);

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

    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    KeReleaseGuardedMutex(&ViewLock);
    KeReleaseMutex(&Vacb->Mutex, FALSE);

    return STATUS_SUCCESS;
}

static
NTSTATUS
CcRosCreateVacb (
    PBCB Bcb,
    ULONG FileOffset,
    PROS_VACB *Vacb)
{
    PROS_VACB current;
    PROS_VACB previous;
    PLIST_ENTRY current_entry;
    NTSTATUS Status;
    KIRQL oldIrql;

    ASSERT(Bcb);

    DPRINT("CcRosCreateVacb()\n");

    if (FileOffset >= Bcb->FileSize.u.LowPart)
    {
        *Vacb = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    current = ExAllocateFromNPagedLookasideList(&VacbLookasideList);
    current->Valid = FALSE;
    current->Dirty = FALSE;
    current->PageOut = FALSE;
    current->FileOffset = ROUND_DOWN(FileOffset, VACB_MAPPING_GRANULARITY);
    current->Bcb = Bcb;
#if DBG
    if ( Bcb->Trace )
    {
        DPRINT1("CacheMap 0x%p: new VACB: 0x%p\n", Bcb, current );
    }
#endif
    current->MappedCount = 0;
    current->DirtyVacbListEntry.Flink = NULL;
    current->DirtyVacbListEntry.Blink = NULL;
    current->ReferenceCount = 1;
    KeInitializeMutex(&current->Mutex, 0);
    KeWaitForSingleObject(&current->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    KeAcquireGuardedMutex(&ViewLock);

    *Vacb = current;
    /* There is window between the call to CcRosLookupVacb
     * and CcRosCreateVacb. We must check if a VACB for the
     * file offset exist. If there is a VACB, we release
     * our newly created VACB and return the existing one.
     */
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
    current_entry = Bcb->BcbVacbListHead.Flink;
    previous = NULL;
    while (current_entry != &Bcb->BcbVacbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    ROS_VACB,
                                    BcbVacbListEntry);
        if (IsPointInRange(current->FileOffset, VACB_MAPPING_GRANULARITY,
                           FileOffset))
        {
            CcRosVacbIncRefCount(current);
            KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
#if DBG
            if ( Bcb->Trace )
            {
                DPRINT1("CacheMap 0x%p: deleting newly created VACB 0x%p ( found existing one 0x%p )\n",
                        Bcb,
                        (*Vacb),
                        current );
            }
#endif
            KeReleaseMutex(&(*Vacb)->Mutex, FALSE);
            KeReleaseGuardedMutex(&ViewLock);
            ExFreeToNPagedLookasideList(&VacbLookasideList, *Vacb);
            *Vacb = current;
            KeWaitForSingleObject(&current->Mutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            return STATUS_SUCCESS;
        }
        if (current->FileOffset < FileOffset)
        {
            ASSERT(previous == NULL ||
                   previous->FileOffset < current->FileOffset);
            previous = current;
        }
        if (current->FileOffset > FileOffset)
            break;
        current_entry = current_entry->Flink;
    }
    /* There was no existing VACB. */
    current = *Vacb;
    if (previous)
    {
        InsertHeadList(&previous->BcbVacbListEntry, &current->BcbVacbListEntry);
    }
    else
    {
        InsertHeadList(&Bcb->BcbVacbListHead, &current->BcbVacbListEntry);
    }
    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    InsertTailList(&VacbListHead, &current->VacbListEntry);
    InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);
    KeReleaseGuardedMutex(&ViewLock);

    MmLockAddressSpace(MmGetKernelAddressSpace());
    current->BaseAddress = NULL;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                0, // nothing checks for VACB mareas, so set to 0
                                &current->BaseAddress,
                                VACB_MAPPING_GRANULARITY,
                                PAGE_READWRITE,
                                (PMEMORY_AREA*)&current->MemoryArea,
                                FALSE,
                                0,
                                PAGE_SIZE);
    MmUnlockAddressSpace(MmGetKernelAddressSpace());
    if (!NT_SUCCESS(Status))
    {
        KeBugCheck(CACHE_MANAGER);
    }

    /* Create a virtual mapping for this memory area */
    MI_SET_USAGE(MI_USAGE_CACHE);
#if MI_TRACE_PFNS
    PWCHAR pos = NULL;
    ULONG len = 0;
    if ((Bcb->FileObject) && (Bcb->FileObject->FileName.Buffer))
    {
        pos = wcsrchr(Bcb->FileObject->FileName.Buffer, '\\');
        len = wcslen(pos) * sizeof(WCHAR);
        if (pos) snprintf(MI_PFN_CURRENT_PROCESS_NAME, min(16, len), "%S", pos);
    }
#endif

    MmMapMemoryArea(current->BaseAddress, VACB_MAPPING_GRANULARITY,
                    MC_CACHE, PAGE_READWRITE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosGetVacbChain (
    PBCB Bcb,
    ULONG FileOffset,
    ULONG Length,
    PROS_VACB *Vacb)
{
    PROS_VACB current;
    ULONG i;
    PROS_VACB *VacbList;
    PROS_VACB Previous = NULL;

    ASSERT(Bcb);

    DPRINT("CcRosGetVacbChain()\n");

    Length = ROUND_UP(Length, VACB_MAPPING_GRANULARITY);

    VacbList = _alloca(sizeof(PROS_VACB) *
                       (Length / VACB_MAPPING_GRANULARITY));

    /*
     * Look for a VACB already mapping the same data.
     */
    for (i = 0; i < (Length / VACB_MAPPING_GRANULARITY); i++)
    {
        ULONG CurrentOffset = FileOffset + (i * VACB_MAPPING_GRANULARITY);
        current = CcRosLookupVacb(Bcb, CurrentOffset);
        if (current != NULL)
        {
            KeAcquireGuardedMutex(&ViewLock);

            /* Move to tail of LRU list */
            RemoveEntryList(&current->VacbLruListEntry);
            InsertTailList(&VacbLruListHead, &current->VacbLruListEntry);

            KeReleaseGuardedMutex(&ViewLock);

            VacbList[i] = current;
        }
        else
        {
            CcRosCreateVacb(Bcb, CurrentOffset, &current);
            VacbList[i] = current;
        }
    }

    for (i = 0; i < Length / VACB_MAPPING_GRANULARITY; i++)
    {
        if (i == 0)
        {
            *Vacb = VacbList[i];
            Previous = VacbList[i];
        }
        else
        {
            Previous->NextInChain = VacbList[i];
            Previous = VacbList[i];
        }
    }
    ASSERT(Previous);
    Previous->NextInChain = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosGetVacb (
    PBCB Bcb,
    ULONG FileOffset,
    PULONG BaseOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    PROS_VACB *Vacb)
{
    PROS_VACB current;
    NTSTATUS Status;

    ASSERT(Bcb);

    DPRINT("CcRosGetVacb()\n");

    /*
     * Look for a VACB already mapping the same data.
     */
    current = CcRosLookupVacb(Bcb, FileOffset);
    if (current == NULL)
    {
        /*
         * Otherwise create a new VACB.
         */
        Status = CcRosCreateVacb(Bcb, FileOffset, &current);
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
    *BaseOffset = current->FileOffset;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CcRosRequestVacb (
    PBCB Bcb,
    ULONG FileOffset,
    PVOID* BaseAddress,
    PBOOLEAN UptoDate,
    PROS_VACB *Vacb)
/*
 * FUNCTION: Request a page mapping for a BCB
 */
{
    ULONG BaseOffset;

    ASSERT(Bcb);

    if (FileOffset % VACB_MAPPING_GRANULARITY != 0)
    {
        DPRINT1("Bad fileoffset %x should be multiple of %x",
                FileOffset, VACB_MAPPING_GRANULARITY);
        KeBugCheck(CACHE_MANAGER);
    }

    return CcRosGetVacb(Bcb,
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
 * FUNCTION: Releases a VACB associated with a BCB
 */
{
    DPRINT("Freeing VACB 0x%p\n", Vacb);
#if DBG
    if (Vacb->Bcb->Trace)
    {
        DPRINT1("CacheMap 0x%p: deleting VACB: 0x%p\n", Vacb->Bcb, Vacb);
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
    PBCB Bcb;
    LARGE_INTEGER Offset;
    PROS_VACB current;
    NTSTATUS Status;
    KIRQL oldIrql;

    DPRINT("CcFlushCache(SectionObjectPointers 0x%p, FileOffset 0x%p, Length %lu, IoStatus 0x%p)\n",
           SectionObjectPointers, FileOffset, Length, IoStatus);

    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
        ASSERT(Bcb);
        if (FileOffset)
        {
            Offset = *FileOffset;
        }
        else
        {
            Offset.QuadPart = (LONGLONG)0;
            Length = Bcb->FileSize.u.LowPart;
        }

        if (IoStatus)
        {
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
        }

        while (Length > 0)
        {
            current = CcRosLookupVacb(Bcb, Offset.u.LowPart);
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
                KeReleaseMutex(&current->Mutex, FALSE);

                KeAcquireGuardedMutex(&ViewLock);
                KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
                CcRosVacbDecRefCount(current);
                KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
                KeReleaseGuardedMutex(&ViewLock);
            }

            Offset.QuadPart += VACB_MAPPING_GRANULARITY;
            if (Length > VACB_MAPPING_GRANULARITY)
            {
                Length -= VACB_MAPPING_GRANULARITY;
            }
            else
            {
                Length = 0;
            }
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
    PBCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
{
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    LIST_ENTRY FreeList;
    KIRQL oldIrql;

    ASSERT(Bcb);

    Bcb->RefCount++;
    KeReleaseGuardedMutex(&ViewLock);

    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, NULL);

    KeAcquireGuardedMutex(&ViewLock);
    Bcb->RefCount--;
    if (Bcb->RefCount == 0)
    {
        if (Bcb->BcbRemoveListEntry.Flink != NULL)
        {
            RemoveEntryList(&Bcb->BcbRemoveListEntry);
            Bcb->BcbRemoveListEntry.Flink = NULL;
        }

        FileObject->SectionObjectPointer->SharedCacheMap = NULL;

        /*
         * Release all VACBs
         */
        InitializeListHead(&FreeList);
        KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
        while (!IsListEmpty(&Bcb->BcbVacbListHead))
        {
            current_entry = RemoveTailList(&Bcb->BcbVacbListHead);
            current = CONTAINING_RECORD(current_entry, ROS_VACB, BcbVacbListEntry);
            RemoveEntryList(&current->VacbListEntry);
            RemoveEntryList(&current->VacbLruListEntry);
            if (current->Dirty)
            {
                RemoveEntryList(&current->DirtyVacbListEntry);
                DirtyPageCount -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
                DPRINT1("Freeing dirty VACB\n");
            }
            InsertHeadList(&FreeList, &current->BcbVacbListEntry);
        }
#if DBG
        Bcb->Trace = FALSE;
#endif
        KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);

        KeReleaseGuardedMutex(&ViewLock);
        ObDereferenceObject (Bcb->FileObject);

        while (!IsListEmpty(&FreeList))
        {
            current_entry = RemoveTailList(&FreeList);
            current = CONTAINING_RECORD(current_entry, ROS_VACB, BcbVacbListEntry);
            CcRosInternalFreeVacb(current);
        }
        ExFreeToNPagedLookasideList(&BcbLookasideList, Bcb);
        KeAcquireGuardedMutex(&ViewLock);
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
CcRosReferenceCache (
    PFILE_OBJECT FileObject)
{
    PBCB Bcb;
    KeAcquireGuardedMutex(&ViewLock);
    Bcb = (PBCB)FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(Bcb);
    if (Bcb->RefCount == 0)
    {
        ASSERT(Bcb->BcbRemoveListEntry.Flink != NULL);
        RemoveEntryList(&Bcb->BcbRemoveListEntry);
        Bcb->BcbRemoveListEntry.Flink = NULL;

    }
    else
    {
        ASSERT(Bcb->BcbRemoveListEntry.Flink == NULL);
    }
    Bcb->RefCount++;
    KeReleaseGuardedMutex(&ViewLock);
}

VOID
NTAPI
CcRosSetRemoveOnClose (
    PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
    PBCB Bcb;
    DPRINT("CcRosSetRemoveOnClose()\n");
    KeAcquireGuardedMutex(&ViewLock);
    Bcb = (PBCB)SectionObjectPointer->SharedCacheMap;
    if (Bcb)
    {
        Bcb->RemoveOnClose = TRUE;
        if (Bcb->RefCount == 0)
        {
            CcRosDeleteFileCache(Bcb->FileObject, Bcb);
        }
    }
    KeReleaseGuardedMutex(&ViewLock);
}


VOID
NTAPI
CcRosDereferenceCache (
    PFILE_OBJECT FileObject)
{
    PBCB Bcb;
    KeAcquireGuardedMutex(&ViewLock);
    Bcb = (PBCB)FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(Bcb);
    if (Bcb->RefCount > 0)
    {
        Bcb->RefCount--;
        if (Bcb->RefCount == 0)
        {
            MmFreeSectionSegments(Bcb->FileObject);
            CcRosDeleteFileCache(FileObject, Bcb);
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
    PBCB Bcb;

    KeAcquireGuardedMutex(&ViewLock);

    if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
    {
        Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
        if (FileObject->PrivateCacheMap != NULL)
        {
            FileObject->PrivateCacheMap = NULL;
            if (Bcb->RefCount > 0)
            {
                Bcb->RefCount--;
                if (Bcb->RefCount == 0)
                {
                    MmFreeSectionSegments(Bcb->FileObject);
                    CcRosDeleteFileCache(FileObject, Bcb);
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
    PBCB Bcb;
    NTSTATUS Status;

    KeAcquireGuardedMutex(&ViewLock);

    ASSERT(FileObject->SectionObjectPointer);
    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    if (Bcb == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        if (FileObject->PrivateCacheMap == NULL)
        {
            FileObject->PrivateCacheMap = Bcb;
            Bcb->RefCount++;
        }
        if (Bcb->BcbRemoveListEntry.Flink != NULL)
        {
            RemoveEntryList(&Bcb->BcbRemoveListEntry);
            Bcb->BcbRemoveListEntry.Flink = NULL;
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
    PCACHE_MANAGER_CALLBACKS CallBacks,
    PVOID LazyWriterContext)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
{
    PBCB Bcb;

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    DPRINT("CcRosInitializeFileCache(FileObject 0x%p, Bcb 0x%p)\n",
           FileObject, Bcb);

    KeAcquireGuardedMutex(&ViewLock);
    if (Bcb == NULL)
    {
        Bcb = ExAllocateFromNPagedLookasideList(&BcbLookasideList);
        if (Bcb == NULL)
        {
            KeReleaseGuardedMutex(&ViewLock);
            return STATUS_UNSUCCESSFUL;
        }
        RtlZeroMemory(Bcb, sizeof(*Bcb));
        ObReferenceObjectByPointer(FileObject,
                                   FILE_ALL_ACCESS,
                                   NULL,
                                   KernelMode);
        Bcb->FileObject = FileObject;
        Bcb->Callbacks = CallBacks;
        Bcb->LazyWriteContext = LazyWriterContext;
        if (FileObject->FsContext)
        {
            Bcb->AllocationSize =
                ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->AllocationSize;
            Bcb->FileSize =
                ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize;
        }
        KeInitializeSpinLock(&Bcb->BcbLock);
        InitializeListHead(&Bcb->BcbVacbListHead);
        FileObject->SectionObjectPointer->SharedCacheMap = Bcb;
    }
    if (FileObject->PrivateCacheMap == NULL)
    {
        FileObject->PrivateCacheMap = Bcb;
        Bcb->RefCount++;
    }
    if (Bcb->BcbRemoveListEntry.Flink != NULL)
    {
        RemoveEntryList(&Bcb->BcbRemoveListEntry);
        Bcb->BcbRemoveListEntry.Flink = NULL;
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
    PBCB Bcb;
    if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
    {
        Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
        ASSERT(Bcb);
        return Bcb->FileObject;
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

    InitializeListHead(&VacbListHead);
    InitializeListHead(&DirtyVacbListHead);
    InitializeListHead(&VacbLruListHead);
    InitializeListHead(&ClosedListHead);
    KeInitializeGuardedMutex(&ViewLock);
    ExInitializeNPagedLookasideList (&iBcbLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(INTERNAL_BCB),
                                     TAG_IBCB,
                                     20);
    ExInitializeNPagedLookasideList (&BcbLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(BCB),
                                     TAG_BCB,
                                     20);
    ExInitializeNPagedLookasideList (&VacbLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(ROS_VACB),
                                     TAG_CSEG,
                                     20);

    MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);

    CcInitCacheZeroPage();

}

/* EOF */
