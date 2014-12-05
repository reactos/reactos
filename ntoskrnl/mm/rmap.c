/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/rmap.c
 * PURPOSE:         Kernel memory managment functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <cache/section/newmm.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeRmapList)
#endif

/* TYPES ********************************************************************/

/* GLOBALS ******************************************************************/

static NPAGED_LOOKASIDE_LIST RmapLookasideList;
FAST_MUTEX RmapListLock;

/* FUNCTIONS ****************************************************************/

_IRQL_requires_max_(DISPATCH_LEVEL)
static
VOID
NTAPI
RmapListFree(
    _In_ __drv_freesMem(Mem) PVOID P)
{
    ExFreePoolWithTag(P, TAG_RMAP);
}

VOID
INIT_FUNCTION
NTAPI
MmInitializeRmapList(VOID)
{
    ExInitializeFastMutex(&RmapListLock);
    ExInitializeNPagedLookasideList (&RmapLookasideList,
                                     NULL,
                                     RmapListFree,
                                     0,
                                     sizeof(MM_RMAP_ENTRY),
                                     TAG_RMAP,
                                     50);
}

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY entry;
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    ULONG Type;
    PVOID Address;
    PEPROCESS Process;
    ULONGLONG Offset;
    NTSTATUS Status = STATUS_SUCCESS;

    ExAcquireFastMutex(&RmapListLock);
    entry = MmGetRmapListHeadPage(Page);

#ifdef NEWCC
    // Special case for NEWCC: we can have a page that's only in a segment
    // page table
    if (entry && RMAP_IS_SEGMENT(entry->Address) && entry->Next == NULL)
    {
        /* NEWCC does locking itself */
        ExReleaseFastMutex(&RmapListLock);
        return MmpPageOutPhysicalAddress(Page);
    }
#endif

    while (entry && RMAP_IS_SEGMENT(entry->Address))
        entry = entry->Next;

    if (entry == NULL)
    {
        ExReleaseFastMutex(&RmapListLock);
        return(STATUS_UNSUCCESSFUL);
    }

    Process = entry->Process;

    Address = entry->Address;

    if ((((ULONG_PTR)Address) & 0xFFF) != 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if (Address < MmSystemRangeStart)
    {
        if (!ExAcquireRundownProtection(&Process->RundownProtect))
        {
            ExReleaseFastMutex(&RmapListLock);
            return STATUS_PROCESS_IS_TERMINATING;
        }

        Status = ObReferenceObjectByPointer(Process, PROCESS_ALL_ACCESS, NULL, KernelMode);
        ExReleaseFastMutex(&RmapListLock);
        if (!NT_SUCCESS(Status))
        {
            ExReleaseRundownProtection(&Process->RundownProtect);
            return Status;
        }
        AddressSpace = &Process->Vm;
    }
    else
    {
        ExReleaseFastMutex(&RmapListLock);
        AddressSpace = MmGetKernelAddressSpace();
    }

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
    if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
    {
        MmUnlockAddressSpace(AddressSpace);
        if (Address < MmSystemRangeStart)
        {
            ExReleaseRundownProtection(&Process->RundownProtect);
            ObDereferenceObject(Process);
        }
        return(STATUS_UNSUCCESSFUL);
    }
    Type = MemoryArea->Type;
    if (Type == MEMORY_AREA_SECTION_VIEW)
    {
        ULONG_PTR Entry;
        Offset = MemoryArea->Data.SectionData.ViewOffset.QuadPart +
                 ((ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress);

        MmLockSectionSegment(MemoryArea->Data.SectionData.Segment);

        /*
         * Get or create a pageop
         */
        Entry = MmGetPageEntrySectionSegment(MemoryArea->Data.SectionData.Segment,
                                             (PLARGE_INTEGER)&Offset);
        if (Entry && IS_SWAP_FROM_SSE(Entry) && SWAPENTRY_FROM_SSE(Entry) == MM_WAIT_ENTRY)
        {
            MmUnlockSectionSegment(MemoryArea->Data.SectionData.Segment);
            MmUnlockAddressSpace(AddressSpace);
            if (Address < MmSystemRangeStart)
            {
                ExReleaseRundownProtection(&Process->RundownProtect);
                ObDereferenceObject(Process);
            }
            return(STATUS_UNSUCCESSFUL);
        }

        MmSetPageEntrySectionSegment(MemoryArea->Data.SectionData.Segment, (PLARGE_INTEGER)&Offset, MAKE_SWAP_SSE(MM_WAIT_ENTRY));

        /*
         * Release locks now we have a page op.
         */
        MmUnlockSectionSegment(MemoryArea->Data.SectionData.Segment);
        MmUnlockAddressSpace(AddressSpace);

        /*
         * Do the actual page out work.
         */
        Status = MmPageOutSectionView(AddressSpace, MemoryArea, Address, Entry);
    }
    else if (Type == MEMORY_AREA_CACHE)
    {
        /* NEWCC does locking itself */
        MmUnlockAddressSpace(AddressSpace);
        Status = MmpPageOutPhysicalAddress(Page);
    }
    else
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    if (Address < MmSystemRangeStart)
    {
        ExReleaseRundownProtection(&Process->RundownProtect);
        ObDereferenceObject(Process);
    }
    return(Status);
}

VOID
NTAPI
MmSetCleanAllRmaps(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry;

    ExAcquireFastMutex(&RmapListLock);
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        DPRINT1("MmIsDirtyRmap: No rmaps.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    while (current_entry != NULL)
    {
        if (!RMAP_IS_SEGMENT(current_entry->Address))
            MmSetCleanPage(current_entry->Process, current_entry->Address);
        current_entry = current_entry->Next;
    }
    ExReleaseFastMutex(&RmapListLock);
}

VOID
NTAPI
MmSetDirtyAllRmaps(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry;

    ExAcquireFastMutex(&RmapListLock);
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        DPRINT1("MmIsDirtyRmap: No rmaps.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    while (current_entry != NULL)
    {
        if (!RMAP_IS_SEGMENT(current_entry->Address))
            MmSetDirtyPage(current_entry->Process, current_entry->Address);
        current_entry = current_entry->Next;
    }
    ExReleaseFastMutex(&RmapListLock);
}

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry;

    ExAcquireFastMutex(&RmapListLock);
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        ExReleaseFastMutex(&RmapListLock);
        return(FALSE);
    }
    while (current_entry != NULL)
    {
        if (
            !RMAP_IS_SEGMENT(current_entry->Address) &&
            MmIsDirtyPage(current_entry->Process, current_entry->Address))
        {
            ExReleaseFastMutex(&RmapListLock);
            return(TRUE);
        }
        current_entry = current_entry->Next;
    }
    ExReleaseFastMutex(&RmapListLock);
    return(FALSE);
}

VOID
NTAPI
MmInsertRmap(PFN_NUMBER Page, PEPROCESS Process,
             PVOID Address)
{
    PMM_RMAP_ENTRY current_entry;
    PMM_RMAP_ENTRY new_entry;
    ULONG PrevSize;
    if (!RMAP_IS_SEGMENT(Address))
        Address = (PVOID)PAGE_ROUND_DOWN(Address);

    new_entry = ExAllocateFromNPagedLookasideList(&RmapLookasideList);
    if (new_entry == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    new_entry->Address = Address;
    new_entry->Process = (PEPROCESS)Process;
#if DBG
#ifdef __GNUC__
    new_entry->Caller = __builtin_return_address(0);
#else
    new_entry->Caller = _ReturnAddress();
#endif
#endif

    if (
        !RMAP_IS_SEGMENT(Address) &&
        MmGetPfnForProcess(Process, Address) != Page)
    {
        DPRINT1("Insert rmap (%d, 0x%.8X) 0x%.8X which doesn't match physical "
                "address 0x%.8X\n", Process ? Process->UniqueProcessId : 0,
                Address,
                MmGetPfnForProcess(Process, Address) << PAGE_SHIFT,
                Page << PAGE_SHIFT);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    ExAcquireFastMutex(&RmapListLock);
    current_entry = MmGetRmapListHeadPage(Page);
    new_entry->Next = current_entry;
#if DBG
    while (current_entry)
    {
        if (current_entry->Address == new_entry->Address && current_entry->Process == new_entry->Process)
        {
            DbgPrint("MmInsertRmap tries to add a second rmap entry for address %p\n    current caller ",
                     current_entry->Address);
            DbgPrint("%p", new_entry->Caller);
            DbgPrint("\n    previous caller ");
            DbgPrint("%p", current_entry->Caller);
            DbgPrint("\n");
            KeBugCheck(MEMORY_MANAGEMENT);
        }
        current_entry = current_entry->Next;
    }
#endif
    MmSetRmapListHeadPage(Page, new_entry);
    ExReleaseFastMutex(&RmapListLock);
    if (!RMAP_IS_SEGMENT(Address))
    {
        if (Process == NULL)
        {
            Process = PsInitialSystemProcess;
        }
        if (Process)
        {
            PrevSize = InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, PAGE_SIZE);
            if (PrevSize >= Process->Vm.PeakWorkingSetSize)
            {
                Process->Vm.PeakWorkingSetSize = PrevSize + PAGE_SIZE;
            }
        }
    }
}

VOID
NTAPI
MmDeleteAllRmaps(PFN_NUMBER Page, PVOID Context,
                 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process,
                                       PVOID Address))
{
    PMM_RMAP_ENTRY current_entry;
    PMM_RMAP_ENTRY previous_entry;
    PEPROCESS Process;

    ExAcquireFastMutex(&RmapListLock);
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        DPRINT1("MmDeleteAllRmaps: No rmaps.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    MmSetRmapListHeadPage(Page, NULL);
    ExReleaseFastMutex(&RmapListLock);

    while (current_entry != NULL)
    {
        previous_entry = current_entry;
        current_entry = current_entry->Next;
        if (!RMAP_IS_SEGMENT(previous_entry->Address))
        {
            if (DeleteMapping)
            {
                DeleteMapping(Context, previous_entry->Process,
                              previous_entry->Address);
            }
            Process = previous_entry->Process;
            ExFreeToNPagedLookasideList(&RmapLookasideList, previous_entry);
            if (Process == NULL)
            {
                Process = PsInitialSystemProcess;
            }
            if (Process)
            {
                (void)InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
            }
        }
        else
        {
            ExFreeToNPagedLookasideList(&RmapLookasideList, previous_entry);
        }
    }
}

VOID
NTAPI
MmDeleteRmap(PFN_NUMBER Page, PEPROCESS Process,
             PVOID Address)
{
    PMM_RMAP_ENTRY current_entry, previous_entry;

    ExAcquireFastMutex(&RmapListLock);
    previous_entry = NULL;
    current_entry = MmGetRmapListHeadPage(Page);

    while (current_entry != NULL)
    {
        if (current_entry->Process == (PEPROCESS)Process &&
                current_entry->Address == Address)
        {
            if (previous_entry == NULL)
            {
                MmSetRmapListHeadPage(Page, current_entry->Next);
            }
            else
            {
                previous_entry->Next = current_entry->Next;
            }
            ExReleaseFastMutex(&RmapListLock);
            ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
            if (!RMAP_IS_SEGMENT(Address))
            {
                if (Process == NULL)
                {
                    Process = PsInitialSystemProcess;
                }
                if (Process)
                {
                    (void)InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
                }
            }
            return;
        }
        previous_entry = current_entry;
        current_entry = current_entry->Next;
    }
    KeBugCheck(MEMORY_MANAGEMENT);
}

/*

Return the process pointer given when a previous call to MmInsertRmap was
called with a process and address pointer that conform to the segment rmap
schema.  In short, this requires the address part to be 0xffffff00 + n
where n is between 0 and 255.  When such an rmap exists, it specifies a
segment rmap in which the process part is a pointer to a slice of a section
page table, and the low 8 bits of the address represent a page index in the
page table slice.  Together, this information is used by
MmGetSectionAssociation to determine which page entry points to this page in
the segment page table.

*/

PVOID
NTAPI
MmGetSegmentRmap(PFN_NUMBER Page, PULONG RawOffset)
{
    PCACHE_SECTION_PAGE_TABLE Result = NULL;
    PMM_RMAP_ENTRY current_entry;//, previous_entry;

    ExAcquireFastMutex(&RmapListLock);
    //previous_entry = NULL;
    current_entry = MmGetRmapListHeadPage(Page);
    while (current_entry != NULL)
    {
        if (RMAP_IS_SEGMENT(current_entry->Address))
        {
            Result = (PCACHE_SECTION_PAGE_TABLE)current_entry->Process;
            *RawOffset = (ULONG_PTR)current_entry->Address & ~RMAP_SEGMENT_MASK;
            InterlockedIncrementUL(&Result->Segment->ReferenceCount);
            ExReleaseFastMutex(&RmapListLock);
            return Result;
        }
        //previous_entry = current_entry;
        current_entry = current_entry->Next;
    }
    ExReleaseFastMutex(&RmapListLock);
    return NULL;
}

/*

Remove the section rmap associated with the indicated page, if it exists.

*/

VOID
NTAPI
MmDeleteSectionAssociation(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry, previous_entry;

    ExAcquireFastMutex(&RmapListLock);
    previous_entry = NULL;
    current_entry = MmGetRmapListHeadPage(Page);
    while (current_entry != NULL)
    {
        if (RMAP_IS_SEGMENT(current_entry->Address))
        {
            if (previous_entry == NULL)
            {
                MmSetRmapListHeadPage(Page, current_entry->Next);
            }
            else
            {
                previous_entry->Next = current_entry->Next;
            }
            ExReleaseFastMutex(&RmapListLock);
            ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
            return;
        }
        previous_entry = current_entry;
        current_entry = current_entry->Next;
    }
    ExReleaseFastMutex(&RmapListLock);
}
