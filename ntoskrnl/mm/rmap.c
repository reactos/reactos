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

/* TYPES ********************************************************************/

/* GLOBALS ******************************************************************/

static NPAGED_LOOKASIDE_LIST RmapLookasideList;

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

CODE_SEG("INIT")
VOID
NTAPI
MmInitializeRmapList(VOID)
{
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
    PVOID Address;
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    PMM_SECTION_SEGMENT Segment;
    LARGE_INTEGER SegmentOffset;
    KIRQL OldIrql;

    OldIrql = MiAcquirePfnLock();

    entry = MmGetRmapListHeadPage(Page);

    while (entry && RMAP_IS_SEGMENT(entry->Address))
        entry = entry->Next;

    if (entry == NULL)
    {
        MiReleasePfnLock(OldIrql);
        goto WriteSegment;
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
            MiReleasePfnLock(OldIrql);
            return STATUS_PROCESS_IS_TERMINATING;
        }

        Status = ObReferenceObjectByPointer(Process, PROCESS_ALL_ACCESS, NULL, KernelMode);
        MiReleasePfnLock(OldIrql);
        if (!NT_SUCCESS(Status))
        {
            ExReleaseRundownProtection(&Process->RundownProtect);
            return Status;
        }
        AddressSpace = &Process->Vm;
    }
    else
    {
        MiReleasePfnLock(OldIrql);
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

    if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
    {
        ULONG_PTR Entry;
        BOOLEAN Dirty;
        PFN_NUMBER MapPage;
        LARGE_INTEGER Offset;
        BOOLEAN Released;

        Offset.QuadPart = MemoryArea->SectionData.ViewOffset.QuadPart +
                 ((ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea));

        Segment = MemoryArea->SectionData.Segment;

        MmLockSectionSegment(Segment);

        Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
        if (Entry && MM_IS_WAIT_PTE(Entry))
        {
            /* The segment is being read or something. Give up */
            MmUnlockSectionSegment(Segment);
            MmUnlockAddressSpace(AddressSpace);
            if (Address < MmSystemRangeStart)
            {
                ExReleaseRundownProtection(&Process->RundownProtect);
                ObDereferenceObject(Process);
            }
            return(STATUS_UNSUCCESSFUL);
        }

        /* Delete this virtual mapping in the process */
        MmDeleteVirtualMapping(Process, Address, &Dirty, &MapPage);
        ASSERT(MapPage == Page);

        if (Page != PFN_FROM_SSE(Entry))
        {
            SWAPENTRY SwapEntry;

            /* This page is private to the process */
            MmUnlockSectionSegment(Segment);

            /* Check if we should write it back to the page file */
            SwapEntry = MmGetSavedSwapEntryPage(Page);

            if ((SwapEntry == 0) && Dirty)
            {
                /* We don't have a Swap entry, yet the page is dirty. Get one */
                SwapEntry = MmAllocSwapPage();
                if (!SwapEntry)
                {
                    PMM_REGION Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                            &MemoryArea->SectionData.RegionListHead,
                            Address, NULL);

                    /* We can't, so let this page in the Process VM */
                    MmCreateVirtualMapping(Process, Address, Region->Protect, &Page, 1);
                    MmSetDirtyPage(Process, Address);

                    MmUnlockAddressSpace(AddressSpace);
                    if (Address < MmSystemRangeStart)
                    {
                        ExReleaseRundownProtection(&Process->RundownProtect);
                        ObDereferenceObject(Process);
                    }

                    return STATUS_UNSUCCESSFUL;
                }
            }

            if (Dirty)
            {
                Status = MmWriteToSwapPage(SwapEntry, Page);
                if (!NT_SUCCESS(Status))
                {
                    /* We failed at saving the content of this page. Keep it in */
                    PMM_REGION Region = MmFindRegion((PVOID)MA_GetStartingAddress(MemoryArea),
                            &MemoryArea->SectionData.RegionListHead,
                            Address, NULL);

                    /* This Swap Entry is useless to us */
                    MmSetSavedSwapEntryPage(Page, 0);
                    MmFreeSwapPage(SwapEntry);

                    /* We can't, so let this page in the Process VM */
                    MmCreateVirtualMapping(Process, Address, Region->Protect, &Page, 1);
                    MmSetDirtyPage(Process, Address);

                    MmUnlockAddressSpace(AddressSpace);
                    if (Address < MmSystemRangeStart)
                    {
                        ExReleaseRundownProtection(&Process->RundownProtect);
                        ObDereferenceObject(Process);
                    }
                    return STATUS_UNSUCCESSFUL;
                }
            }

            if (SwapEntry)
            {
                /* Keep this in the process VM */
                MmCreatePageFileMapping(Process, Address, SwapEntry);
                MmSetSavedSwapEntryPage(Page, 0);
            }

            MmUnlockAddressSpace(AddressSpace);

            /* We can finally let this page go */
            MmDeleteRmap(Page, Process, Address);
            MmReleasePageMemoryConsumer(MC_USER, Page);

            ASSERT(MmGetRmapListHeadPage(Page) == NULL);

            if (Address < MmSystemRangeStart)
            {
                ExReleaseRundownProtection(&Process->RundownProtect);
                ObDereferenceObject(Process);
            }
            return STATUS_SUCCESS;
        }

        /* Delete this RMAP */
        MmDeleteRmap(Page, Process, Address);

        /* One less mapping referencing this segment */
        Released = MmUnsharePageEntrySectionSegment(MemoryArea, Segment, &Offset, Dirty, FALSE, NULL);

        MmUnlockSectionSegment(Segment);
        MmUnlockAddressSpace(AddressSpace);

        if (Address < MmSystemRangeStart)
        {
            ExReleaseRundownProtection(&Process->RundownProtect);
            ObDereferenceObject(Process);
        }

        if (Released) return STATUS_SUCCESS;
    }
#ifdef NEWCC
    else if (Type == MEMORY_AREA_CACHE)
    {
        /* NEWCC does locking itself */
        MmUnlockAddressSpace(AddressSpace);
        Status = MmpPageOutPhysicalAddress(Page);
    }
#endif
    else
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

WriteSegment:
    /* Now write this page to file, if needed */
    Segment = MmGetSectionAssociation(Page, &SegmentOffset);
    if (Segment)
    {
        BOOLEAN Released;

        MmLockSectionSegment(Segment);

        Released = MmCheckDirtySegment(Segment, &SegmentOffset, FALSE, TRUE);

        MmUnlockSectionSegment(Segment);

        MmDereferenceSegment(Segment);

        if (Released)
        {
            return STATUS_SUCCESS;
        }
    }

    /* If we are here, then we didn't release the page */
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MmSetCleanAllRmaps(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry;
    KIRQL OldIrql;

    OldIrql = MiAcquirePfnLock();
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        DPRINT1("MmSetCleanAllRmaps: No rmaps.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    while (current_entry != NULL)
    {
        if (!RMAP_IS_SEGMENT(current_entry->Address))
            MmSetCleanPage(current_entry->Process, current_entry->Address);
        current_entry = current_entry->Next;
    }
    MiReleasePfnLock(OldIrql);
}

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_NUMBER Page)
{
    PMM_RMAP_ENTRY current_entry;
    KIRQL OldIrql;
    BOOLEAN Dirty = FALSE;

    OldIrql = MiAcquirePfnLock();
    current_entry = MmGetRmapListHeadPage(Page);
    if (current_entry == NULL)
    {
        DPRINT1("MmIsDirtyPageRmap: No rmaps.\n");
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    while (current_entry != NULL)
    {
        if (!RMAP_IS_SEGMENT(current_entry->Address))
        {
            if (MmIsDirtyPage(current_entry->Process, current_entry->Address))
            {
                Dirty = TRUE;
                break;
            }
        }
        current_entry = current_entry->Next;
    }
    MiReleasePfnLock(OldIrql);

    return Dirty;
}

VOID
NTAPI
MmInsertRmap(PFN_NUMBER Page, PEPROCESS Process,
             PVOID Address)
{
    PMM_RMAP_ENTRY current_entry;
    PMM_RMAP_ENTRY new_entry;
    ULONG PrevSize;
    KIRQL OldIrql;

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

    OldIrql = MiAcquirePfnLock();
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
    MiReleasePfnLock(OldIrql);

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
MmDeleteRmap(PFN_NUMBER Page, PEPROCESS Process,
             PVOID Address)
{
    PMM_RMAP_ENTRY current_entry, previous_entry;
    KIRQL OldIrql;

    OldIrql = MiAcquirePfnLock();
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
            MiReleasePfnLock(OldIrql);

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
    KIRQL OldIrql = MiAcquirePfnLock();

    //previous_entry = NULL;
    current_entry = MmGetRmapListHeadPage(Page);
    while (current_entry != NULL)
    {
        if (RMAP_IS_SEGMENT(current_entry->Address))
        {
            Result = (PCACHE_SECTION_PAGE_TABLE)current_entry->Process;
            *RawOffset = (ULONG_PTR)current_entry->Address & ~RMAP_SEGMENT_MASK;
            if (*Result->Segment->Flags & MM_SEGMENT_INDELETE)
            {
                MiReleasePfnLock(OldIrql);
                return NULL;
            }

            InterlockedIncrementUL(Result->Segment->ReferenceCount);
            MiReleasePfnLock(OldIrql);
            return Result;
        }
        //previous_entry = current_entry;
        current_entry = current_entry->Next;
    }
    MiReleasePfnLock(OldIrql);
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
    KIRQL OldIrql = MiAcquirePfnLock();

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
            MiReleasePfnLock(OldIrql);
            ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
            return;
        }
        previous_entry = current_entry;
        current_entry = current_entry->Next;
    }
    MiReleasePfnLock(OldIrql);
}
