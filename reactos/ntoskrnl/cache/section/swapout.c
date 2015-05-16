/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section/fault.c
 * PURPOSE:         Consolidate fault handlers for sections
 *
 * PROGRAMMERS:     Arty
 *                  Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/*

This file implements page out infrastructure for cache type sections.  This
is implemented a little differently from the legacy mm because mapping in an
address space and membership in a segment are considered separate.

The general strategy here is to try to remove all mappings as gently as
possible, then to remove the page entry from the section itself as a final
step.  If at any time during the page out operation, the page is mapped in
a new address space by a competing thread, the operation will abort before
the segment page is finally removed, and the page will be naturally faulted
back into any address spaces required in the normal way.

*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

extern KEVENT MmWaitPageEvent;
extern FAST_MUTEX RmapListLock;
extern PMMWSL MmWorkingSetList;

FAST_MUTEX MiGlobalPageOperation;

/*

MmWithdrawSectionPage removes a page entry from the section segment, replacing
it with a wait entry.  The caller must replace the wait entry with a 0, when
any required writing is done.  The wait entry must remain until the page is
written to protect against cases where a fault brings a stale copy of the page
back before writing is complete.

*/
PFN_NUMBER
NTAPI
MmWithdrawSectionPage(PMM_SECTION_SEGMENT Segment,
                      PLARGE_INTEGER FileOffset,
                      BOOLEAN *Dirty)
{
    ULONG_PTR Entry;

    DPRINT("MmWithdrawSectionPage(%p,%08x%08x,%p)\n",
           Segment,
           FileOffset->HighPart,
           FileOffset->LowPart,
           Dirty);

    MmLockSectionSegment(Segment);
    Entry = MmGetPageEntrySectionSegment(Segment, FileOffset);

    *Dirty = !!IS_DIRTY_SSE(Entry);

    DPRINT("Withdraw %x (%x) of %wZ\n",
           FileOffset->LowPart,
           Entry,
           Segment->FileObject ? &Segment->FileObject->FileName : NULL);

    if (!Entry)
    {
        DPRINT("Stoeled!\n");
        MmUnlockSectionSegment(Segment);
        return 0;
    }
    else if (MM_IS_WAIT_PTE(Entry))
    {
        DPRINT("WAIT\n");
        MmUnlockSectionSegment(Segment);
        return MM_WAIT_ENTRY;
    }
    else if (Entry && !IS_SWAP_FROM_SSE(Entry))
    {
        DPRINT("Page %x\n", PFN_FROM_SSE(Entry));

        *Dirty |= (Entry & 2);

        MmSetPageEntrySectionSegment(Segment,
                                     FileOffset,
                                     MAKE_SWAP_SSE(MM_WAIT_ENTRY));

        MmUnlockSectionSegment(Segment);
        return PFN_FROM_SSE(Entry);
    }
    else
    {
        DPRINT1("SWAP ENTRY?! (%p:%08x%08x)\n",
                Segment,
                FileOffset->HighPart,
                FileOffset->LowPart);

        ASSERT(FALSE);
        MmUnlockSectionSegment(Segment);
        return 0;
    }
}

/*

This function determines whether the segment holds the very last reference to
the page being considered and if so, writes it back or discards it as
approriate.  One small niggle here is that we might be holding the last
reference to the section segment associated with this page.  That happens
when the segment is destroyed at the same time that an active swap operation
is occurring, and all maps were already withdrawn.  In that case, it's our
responsiblity for finalizing the segment.

Note that in the current code, WriteZero is always TRUE because the section
always backs a file.  In the ultimate form of this code, it also writes back
pages without necessarily evicting them.  In reactos' trunk, this is vestigal.

*/

NTSTATUS
NTAPI
MmFinalizeSectionPageOut(PMM_SECTION_SEGMENT Segment,
                         PLARGE_INTEGER FileOffset,
                         PFN_NUMBER Page,
                         BOOLEAN Dirty)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WriteZero = FALSE, WritePage = FALSE;
    SWAPENTRY Swap = MmGetSavedSwapEntryPage(Page);

    /* Bail early if the reference count isn't where we need it */
    if (MmGetReferenceCountPage(Page) != 1)
    {
        DPRINT1("Cannot page out locked page %x with ref count %lu\n",
                Page,
                MmGetReferenceCountPage(Page));
        return STATUS_UNSUCCESSFUL;
    }

    MmLockSectionSegment(Segment);
    (void)InterlockedIncrementUL(&Segment->ReferenceCount);

    if (Dirty)
    {
        DPRINT("Finalize (dirty) Segment %p Page %x\n", Segment, Page);
        DPRINT("Segment->FileObject %p\n", Segment->FileObject);
        DPRINT("Segment->Flags %x\n", Segment->Flags);

        WriteZero = TRUE;
        WritePage = TRUE;
    }
    else
    {
        WriteZero = TRUE;
    }

    DPRINT("Status %x\n", Status);

    MmUnlockSectionSegment(Segment);

    if (WritePage)
    {
        DPRINT("MiWriteBackPage(Segment %p FileObject %p Offset %x)\n",
               Segment,
               Segment->FileObject,
               FileOffset->LowPart);

        Status = MiWriteBackPage(Segment->FileObject,
                                 FileOffset,
                                 PAGE_SIZE,
                                 Page);
    }

    MmLockSectionSegment(Segment);

    if (WriteZero && NT_SUCCESS(Status))
    {
        DPRINT("Setting page entry in segment %p:%x to swap %x\n",
               Segment,
               FileOffset->LowPart,
               Swap);

        MmSetPageEntrySectionSegment(Segment,
                                     FileOffset,
                                     Swap ? MAKE_SWAP_SSE(Swap) : 0);
    }
    else
    {
        DPRINT("Setting page entry in segment %p:%x to page %x\n",
               Segment,
               FileOffset->LowPart,
               Page);

        MmSetPageEntrySectionSegment(Segment,
                                     FileOffset,
                                     Page ? (Dirty ? DIRTY_SSE(MAKE_PFN_SSE(Page)) : MAKE_PFN_SSE(Page)) : 0);
    }

    if (NT_SUCCESS(Status))
    {
        DPRINT("Removing page %x for real\n", Page);
        MmSetSavedSwapEntryPage(Page, 0);
        MmReleasePageMemoryConsumer(MC_CACHE, Page);
    }

    MmUnlockSectionSegment(Segment);

    if (InterlockedDecrementUL(&Segment->ReferenceCount) == 0)
    {
        MmFinalizeSegment(Segment);
    }

    /* Note: Writing may evict the segment... Nothing is guaranteed from here down */
    MiSetPageEvent(Segment, FileOffset->LowPart);

    DPRINT("Status %x\n", Status);
    return Status;
}

/*

The slightly misnamed MmPageOutCacheSection removes a page from an address
space in the manner of fault handlers found in fault.c.  In the ultimate form
of the code, this is one of the function pointers stored in a memory area
to control how pages in that memory area are managed.

Also misleading is the call to MmReleasePageMemoryConsumer, which releases
the reference held by this address space only.  After all address spaces
have had MmPageOutCacheSection succeed on them for the indicated page,
then paging out of a cache page can continue.

*/

NTSTATUS
NTAPI
MmPageOutCacheSection(PMMSUPPORT AddressSpace,
                      MEMORY_AREA* MemoryArea,
                      PVOID Address,
                      PBOOLEAN Dirty,
                      PMM_REQUIRED_RESOURCES Required)
{
    ULONG_PTR Entry;
    PFN_NUMBER OurPage;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    LARGE_INTEGER TotalOffset;
    PMM_SECTION_SEGMENT Segment;
    PVOID PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);

    TotalOffset.QuadPart = (ULONG_PTR)PAddress -
                           MA_GetStartingAddress(MemoryArea) +
                           MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    Segment = MemoryArea->Data.SectionData.Segment;

    MmLockSectionSegment(Segment);
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    Entry = MmGetPageEntrySectionSegment(Segment, &TotalOffset);
    DBG_UNREFERENCED_LOCAL_VARIABLE(Entry);

    if (MmIsPageSwapEntry(Process, PAddress))
    {
        SWAPENTRY SwapEntry;
        MmGetPageFileMapping(Process, PAddress, &SwapEntry);
        MmUnlockSectionSegment(Segment);
        return SwapEntry == MM_WAIT_ENTRY ? STATUS_SUCCESS + 1 : STATUS_UNSUCCESSFUL;
    }

    MmDeleteRmap(Required->Page[0], Process, Address);
    MmDeleteVirtualMapping(Process, Address, Dirty, &OurPage);
    ASSERT(OurPage == Required->Page[0]);

    /* Note: this releases the reference held by this address space only. */
    MmReleasePageMemoryConsumer(MC_CACHE, Required->Page[0]);

    MmUnlockSectionSegment(Segment);
    MiSetPageEvent(Process, Address);
    return STATUS_SUCCESS;
}

/*

This function is called by rmap when spare pages are needed by the blancer.
It attempts first to release the page from every address space in which it
appears, and, after a final check that no competing thread has mapped the
page again, uses MmFinalizeSectionPageOut to completely evict the page.  If
that's successful, then a suitable non-page map will be left in the segment
page table, otherwise, the original page is replaced in the section page
map.  Failure may result from a variety of conditions, but always leaves
the page mapped.

This code is like the other fault handlers, in that MmPageOutCacheSection has
the option of returning either STATUS_SUCCESS + 1 to wait for a wait entry
to disppear or to use the blocking callout facility by returning
STATUS_MORE_PROCESSING_REQUIRED and placing a pointer to a function from
reqtools.c in the MM_REQUIRED_RESOURCES struct.

*/

NTSTATUS
NTAPI
MmpPageOutPhysicalAddress(PFN_NUMBER Page)
{
    BOOLEAN ProcRef = FALSE, PageDirty;
    PFN_NUMBER SectionPage = 0;
    PMM_RMAP_ENTRY entry;
    PMM_SECTION_SEGMENT Segment = NULL;
    LARGE_INTEGER FileOffset;
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace = NULL;
    BOOLEAN Dirty = FALSE;
    PVOID Address = NULL;
    PEPROCESS Process = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    MM_REQUIRED_RESOURCES Resources = { 0 };

    DPRINTC("Page out %x (ref ct %x)\n", Page, MmGetReferenceCountPage(Page));

    ExAcquireFastMutex(&MiGlobalPageOperation);
    if ((Segment = MmGetSectionAssociation(Page, &FileOffset)))
    {
        DPRINTC("Withdrawing page (%x) %p:%x\n",
                Page,
                Segment,
                FileOffset.LowPart);

        SectionPage = MmWithdrawSectionPage(Segment, &FileOffset, &Dirty);
        DPRINTC("SectionPage %x\n", SectionPage);

        if (SectionPage == MM_WAIT_ENTRY || SectionPage == 0)
        {
            DPRINT1("In progress page out %x\n", SectionPage);
            ExReleaseFastMutex(&MiGlobalPageOperation);
            return STATUS_UNSUCCESSFUL;
        }
        else
        {
            ASSERT(SectionPage == Page);
        }
        Resources.State = Dirty ? 1 : 0;
    }
    else
    {
        DPRINT("No segment association for %x\n", Page);
    }

    Dirty = MmIsDirtyPageRmap(Page);

    DPRINTC("Trying to unmap all instances of %x\n", Page);
    ExAcquireFastMutex(&RmapListLock);
    entry = MmGetRmapListHeadPage(Page);

    // Entry and Segment might be null here in the case that the page
    // is new and is in the process of being swapped in
    if (!entry && !Segment)
    {
        Status = STATUS_UNSUCCESSFUL;
        DPRINT1("Page %x is in transit\n", Page);
        ExReleaseFastMutex(&RmapListLock);
        goto bail;
    }

    while (entry != NULL && NT_SUCCESS(Status))
    {
        Process = entry->Process;
        Address = entry->Address;

        DPRINTC("Process %p Address %p Page %x\n", Process, Address, Page);

        if (RMAP_IS_SEGMENT(Address))
        {
            entry = entry->Next;
            continue;
        }

        if (Process && Address < MmSystemRangeStart)
        {
            /* Make sure we don't try to page out part of an exiting process */
            if (PspIsProcessExiting(Process))
            {
                DPRINT("bail\n");
                ExReleaseFastMutex(&RmapListLock);
                goto bail;
            }
            ObReferenceObject(Process);
            ProcRef = TRUE;
            AddressSpace = &Process->Vm;
        }
        else
        {
            AddressSpace = MmGetKernelAddressSpace();
        }
        ExReleaseFastMutex(&RmapListLock);

        RtlZeroMemory(&Resources, sizeof(Resources));

        if ((((ULONG_PTR)Address) & 0xFFF) != 0)
        {
            KeBugCheck(MEMORY_MANAGEMENT);
        }

        do
        {
            MmLockAddressSpace(AddressSpace);

            MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
            if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
            {
                Status = STATUS_UNSUCCESSFUL;
                MmUnlockAddressSpace(AddressSpace);
                DPRINTC("bail\n");
                goto bail;
            }

            DPRINTC("Type %x (%p -> %p)\n",
                    MemoryArea->Type,
                    MA_GetStartingAddress(MemoryArea),
                    MemoryArea->EndingAddress);

            Resources.DoAcquisition = NULL;
            Resources.Page[0] = Page;

            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

            DPRINT("%p:%p, page %x %x\n",
                   Process,
                   Address,
                   Page,
                   Resources.Page[0]);

            PageDirty = FALSE;

            Status = MmPageOutCacheSection(AddressSpace,
                                           MemoryArea,
                                           Address,
                                           &PageDirty,
                                           &Resources);

            Dirty |= PageDirty;
            DPRINT("%x\n", Status);

            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

            MmUnlockAddressSpace(AddressSpace);

            if (Status == STATUS_SUCCESS + 1)
            {
                // Wait page ... the other guy has it, so we'll just fail for now
                DPRINT1("Wait entry ... can't continue\n");
                Status = STATUS_UNSUCCESSFUL;
                goto bail;
            }
            else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
            {
                DPRINTC("DoAcquisition %p\n", Resources.DoAcquisition);

                Status = Resources.DoAcquisition(AddressSpace,
                                                 MemoryArea,
                                                 &Resources);

                DPRINTC("Status %x\n", Status);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("bail\n");
                    goto bail;
                }
                else
                {
                    Status = STATUS_MM_RESTART_OPERATION;
                }
            }
        }
        while (Status == STATUS_MM_RESTART_OPERATION);

        if (ProcRef)
        {
            ObDereferenceObject(Process);
            ProcRef = FALSE;
        }

        ExAcquireFastMutex(&RmapListLock);
        ASSERT(!MM_IS_WAIT_PTE(MmGetPfnForProcess(Process, Address)));
        entry = MmGetRmapListHeadPage(Page);

        DPRINTC("Entry %p\n", entry);
    }

    ExReleaseFastMutex(&RmapListLock);

bail:
    DPRINTC("BAIL %x\n", Status);

    if (Segment)
    {
        ULONG RefCount;

        DPRINTC("About to finalize section page %x (%p:%x) Status %x %s\n",
                Page,
                Segment,
                FileOffset.LowPart,
                Status,
                Dirty ? "dirty" : "clean");

        if (!NT_SUCCESS(Status) ||
            !NT_SUCCESS(Status = MmFinalizeSectionPageOut(Segment,
                                                          &FileOffset,
                                                          Page,
                                                          Dirty)))
        {
            DPRINTC("Failed to page out %x, replacing %x at %x in segment %x\n",
                    SectionPage,
                    FileOffset.LowPart,
                    Segment);

            MmLockSectionSegment(Segment);

            MmSetPageEntrySectionSegment(Segment,
                                         &FileOffset,
                                         Dirty ? MAKE_PFN_SSE(Page) : DIRTY_SSE(MAKE_PFN_SSE(Page)));

            MmUnlockSectionSegment(Segment);
        }

        /* Alas, we had the last reference */
        if ((RefCount = InterlockedDecrementUL(&Segment->ReferenceCount)) == 0)
            MmFinalizeSegment(Segment);
    }

    if (ProcRef)
    {
        DPRINTC("Dereferencing process...\n");
        ObDereferenceObject(Process);
    }

    ExReleaseFastMutex(&MiGlobalPageOperation);

    DPRINTC("%s %x %x\n",
            NT_SUCCESS(Status) ? "Evicted" : "Spared",
            Page,
            Status);

    return NT_SUCCESS(Status) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
MiCacheEvictPages(PMM_SECTION_SEGMENT Segment,
                  ULONG Target)
{
    ULONG_PTR Entry;
    ULONG Result = 0, i, j;
    NTSTATUS Status;
    PFN_NUMBER Page;
    LARGE_INTEGER Offset;

    MmLockSectionSegment(Segment);

    for (i = 0; i < RtlNumberGenericTableElements(&Segment->PageTable); i++) {

        PCACHE_SECTION_PAGE_TABLE Element = RtlGetElementGenericTable(&Segment->PageTable,
                                                                      i);

        ASSERT(Element);

        Offset = Element->FileOffset;
        for (j = 0; j < ENTRIES_PER_ELEMENT; j++, Offset.QuadPart += PAGE_SIZE) {
            Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
            if (Entry && !IS_SWAP_FROM_SSE(Entry)) {
                Page = PFN_FROM_SSE(Entry);
                MmUnlockSectionSegment(Segment);
                Status = MmpPageOutPhysicalAddress(Page);
                if (NT_SUCCESS(Status))
                    Result++;
                MmLockSectionSegment(Segment);
            }
        }
    }

    MmUnlockSectionSegment(Segment);

    return Result;
}

extern LIST_ENTRY MiSegmentList;

// Interact with legacy balance manager for now
// This can fall away when our section implementation supports
// demand paging properly
NTSTATUS
MiRosTrimCache(ULONG Target,
               ULONG Priority,
               PULONG NrFreed)
{
    ULONG Freed;
    PLIST_ENTRY Entry;
    PMM_SECTION_SEGMENT Segment;
    *NrFreed = 0;

    DPRINT1("Need to trim %lu cache pages\n", Target);
    for (Entry = MiSegmentList.Flink;
         *NrFreed < Target && Entry != &MiSegmentList;
         Entry = Entry->Flink) {
        Segment = CONTAINING_RECORD(Entry, MM_SECTION_SEGMENT, ListOfSegments);
        /* Defer to MM to try recovering pages from it */
        Freed = MiCacheEvictPages(Segment, Target);
        *NrFreed += Freed;
    }
    DPRINT1("Evicted %lu cache pages\n", Target);

    if (!IsListEmpty(&MiSegmentList)) {
        Entry = MiSegmentList.Flink;
        RemoveEntryList(Entry);
        InsertTailList(&MiSegmentList, Entry);
    }

    return STATUS_SUCCESS;
}
