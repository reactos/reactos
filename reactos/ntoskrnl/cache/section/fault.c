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

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>
#include "../mm/ARM3/miarm.h"

#define DPRINTC DPRINT

extern KEVENT MmWaitPageEvent;
extern PMMWSL MmWorkingSetList;

NTSTATUS
NTAPI
MmNotPresentFaultCachePage(PMMSUPPORT AddressSpace,
                           MEMORY_AREA* MemoryArea,
                           PVOID Address,
                           BOOLEAN Locked,
                           PMM_REQUIRED_RESOURCES Required)
{
    NTSTATUS Status;
    PVOID PAddress;
    ULONG Consumer;
    PMM_SECTION_SEGMENT Segment;
    LARGE_INTEGER FileOffset, TotalOffset;
    ULONG Entry;
    ULONG Attributes;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    KIRQL OldIrql;

    DPRINT("Not Present: %p %p (%p-%p)\n",
           AddressSpace,
           Address,
           MemoryArea->StartingAddress,
           MemoryArea->EndingAddress);

    /*
     * There is a window between taking the page fault and locking the
     * address space when another thread could load the page so we check
     * that.
     */
    if (MmIsPagePresent(Process, Address))
    {
        DPRINT("Done\n");
        return STATUS_SUCCESS;
    }

    PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
    TotalOffset.QuadPart = (ULONG_PTR)PAddress -
                           (ULONG_PTR)MemoryArea->StartingAddress;

    Segment = MemoryArea->Data.SectionData.Segment;

    TotalOffset.QuadPart += MemoryArea->Data.SectionData.ViewOffset.QuadPart;
    FileOffset = TotalOffset;

    //Consumer = (Segment->Flags & MM_DATAFILE_SEGMENT) ? MC_CACHE : MC_USER;
    Consumer = MC_CACHE;

    if (Segment->FileObject)
    {
        DPRINT("FileName %wZ\n", &Segment->FileObject->FileName);
    }

    DPRINT("Total Offset %08x%08x\n", TotalOffset.HighPart, TotalOffset.LowPart);

    /* Lock the segment */
    MmLockSectionSegment(Segment);

    /* Get the entry corresponding to the offset within the section */
    Entry = MmGetPageEntrySectionSegment(Segment, &TotalOffset);

    Attributes = PAGE_READONLY;

    if (Required->State && Required->Page[0])
    {
        DPRINT("Have file and page, set page %x in section @ %x #\n",
               Required->Page[0],
               TotalOffset.LowPart);

        if (Required->SwapEntry)
            MmSetSavedSwapEntryPage(Required->Page[0], Required->SwapEntry);

        if (Required->State & 2)
        {
            DPRINT("Set in section @ %x\n", TotalOffset.LowPart);
            Status = MmSetPageEntrySectionSegment(Segment,
                                                  &TotalOffset,
                                                  Entry = MAKE_PFN_SSE(Required->Page[0]));
            if (!NT_SUCCESS(Status))
            {
                MmReleasePageMemoryConsumer(MC_CACHE, Required->Page[0]);
            }
            MmUnlockSectionSegment(Segment);
            MiSetPageEvent(Process, Address);
            DPRINT("Status %x\n", Status);
            return STATUS_MM_RESTART_OPERATION;
        }
        else
        {
            DPRINT("Set %x in address space @ %x\n", Required->Page[0], Address);
            Status = MmCreateVirtualMapping(Process,
                                            Address,
                                            Attributes,
                                            Required->Page,
                                            1);
            if (NT_SUCCESS(Status))
            {
                MmInsertRmap(Required->Page[0], Process, Address);
            }
            else
            {
                /* Drop the reference for our address space ... */
                MmReleasePageMemoryConsumer(MC_CACHE, Required->Page[0]);
            }
            MmUnlockSectionSegment(Segment);
            DPRINTC("XXX Set Event %x\n", Status);
            MiSetPageEvent(Process, Address);
            DPRINT("Status %x\n", Status);
            return Status;
        }
    }
    else if (MM_IS_WAIT_PTE(Entry))
    {
        MmUnlockSectionSegment(Segment);
        return STATUS_SUCCESS + 1;
    }
    else if (Entry)
    {
        PFN_NUMBER Page = PFN_FROM_SSE(Entry);
        DPRINT("Take reference to page %x #\n", Page);

        if (MiGetPfnEntry(Page) == NULL)
        {
            DPRINT1("Found no PFN entry for page 0x%x in page entry 0x%x (segment: 0x%p, offset: %08x%08x)\n",
                    Page,
                    Entry,
                    Segment,
                    TotalOffset.HighPart,
                    TotalOffset.LowPart);
            KeBugCheck(CACHE_MANAGER);
        }

        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
        MmReferencePage(Page);
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

        Status = MmCreateVirtualMapping(Process, Address, Attributes, &Page, 1);
        if (NT_SUCCESS(Status))
        {
            MmInsertRmap(Page, Process, Address);
        }
        DPRINT("XXX Set Event %x\n", Status);
        MiSetPageEvent(Process, Address);
        MmUnlockSectionSegment(Segment);
        DPRINT("Status %x\n", Status);
        return Status;
    }
    else
    {
        DPRINT("Get page into section\n");
        /*
         * If the entry is zero (and it can't change because we have
         * locked the segment) then we need to load the page.
         */
        //DPRINT1("Read from file %08x %wZ\n", FileOffset.LowPart, &Section->FileObject->FileName);
        Required->State = 2;
        Required->Context = Segment->FileObject;
        Required->Consumer = Consumer;
        Required->FileOffset = FileOffset;
        Required->Amount = PAGE_SIZE;
        Required->DoAcquisition = MiReadFilePage;

        MmSetPageEntrySectionSegment(Segment,
                                     &TotalOffset,
                                     MAKE_SWAP_SSE(MM_WAIT_ENTRY));

        MmUnlockSectionSegment(Segment);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    ASSERT(FALSE);
    return STATUS_ACCESS_VIOLATION;
}

NTSTATUS
NTAPI
MiCopyPageToPage(PFN_NUMBER DestPage, PFN_NUMBER SrcPage)
{
    PEPROCESS Process;
    KIRQL Irql, Irql2;
    PVOID TempAddress, TempSource;

    Process = PsGetCurrentProcess();
    TempAddress = MiMapPageInHyperSpace(Process, DestPage, &Irql);
    if (TempAddress == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    TempSource = MiMapPageInHyperSpace(Process, SrcPage, &Irql2);
    if (!TempSource) {
        MiUnmapPageInHyperSpace(Process, TempAddress, Irql);
        return STATUS_NO_MEMORY;
    }

    memcpy(TempAddress, TempSource, PAGE_SIZE);

    MiUnmapPageInHyperSpace(Process, TempSource, Irql2);
    MiUnmapPageInHyperSpace(Process, TempAddress, Irql);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiCowCacheSectionPage(PMMSUPPORT AddressSpace,
                      PMEMORY_AREA MemoryArea,
                      PVOID Address,
                      BOOLEAN Locked,
                      PMM_REQUIRED_RESOURCES Required)
{
    PMM_SECTION_SEGMENT Segment;
    PFN_NUMBER NewPage, OldPage;
    NTSTATUS Status;
    PVOID PAddress;
    LARGE_INTEGER Offset;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

    DPRINT("MmAccessFaultSectionView(%x, %x, %x, %x)\n",
           AddressSpace,
           MemoryArea,
           Address,
           Locked);

    Segment = MemoryArea->Data.SectionData.Segment;

   /* Lock the segment */
    MmLockSectionSegment(Segment);

   /* Find the offset of the page */
    PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
    Offset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress +
                      MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    if (!Segment->WriteCopy /*&&
        !MemoryArea->Data.SectionData.WriteCopyView*/ ||
        Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
    {
#if 0
        if (Region->Protect == PAGE_READWRITE || 
            Region->Protect == PAGE_EXECUTE_READWRITE)
#endif
        {
            ULONG Entry;
            DPRINTC("setting non-cow page %x %x:%x offset %x (%x) to writable\n",
                    Segment,
                    Process,
                    PAddress,
                    Offset.u.LowPart,
                    MmGetPfnForProcess(Process, Address));
            if (Segment->FileObject)
            {
                DPRINTC("file %wZ\n", &Segment->FileObject->FileName);
            }
            Entry = MmGetPageEntrySectionSegment(Segment, &Offset);
            DPRINT("Entry %x\n", Entry);
            if (Entry &&
                !IS_SWAP_FROM_SSE(Entry) &&
                PFN_FROM_SSE(Entry) == MmGetPfnForProcess(Process, Address)) {

                MmSetPageEntrySectionSegment(Segment,
                                             &Offset,
                                             DIRTY_SSE(Entry));
            }
            MmSetPageProtect(Process, PAddress, PAGE_READWRITE);
            MmSetDirtyPage(Process, PAddress);
            MmUnlockSectionSegment(Segment);
            DPRINT("Done\n");
            return STATUS_SUCCESS;
        }
#if 0
        else
        {
            DPRINT("Not supposed to be writable\n");
            MmUnlockSectionSegment(Segment);
            return STATUS_ACCESS_VIOLATION;
        }
#endif
    }

    if (!Required->Page[0])
    {
        SWAPENTRY SwapEntry;
        if (MmIsPageSwapEntry(Process, Address))
        {
            MmGetPageFileMapping(Process, Address, &SwapEntry);
            MmUnlockSectionSegment(Segment);
            if (SwapEntry == MM_WAIT_ENTRY)
                return STATUS_SUCCESS + 1; // Wait ... somebody else is getting it right now
            else
                return STATUS_SUCCESS; // Nonwait swap entry ... handle elsewhere
        }
        Required->Page[1] = MmGetPfnForProcess(Process, Address);
        Required->Consumer = MC_CACHE;
        Required->Amount = 1;
        Required->File = __FILE__;
        Required->Line = __LINE__;
        Required->DoAcquisition = MiGetOnePage;
        MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
        MmUnlockSectionSegment(Segment);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    NewPage = Required->Page[0];
    OldPage = Required->Page[1];

    DPRINT("Allocated page %x\n", NewPage);

   /* Unshare the old page */
   MmDeleteRmap(OldPage, Process, PAddress);

   /* Copy the old page */
    DPRINT("Copying\n");
    MiCopyPageToPage(NewPage, OldPage);

   /* Set the PTE to point to the new page */
    Status = MmCreateVirtualMapping(Process,
                                    Address,
                                    PAGE_READWRITE,
                                    &NewPage,
                                    1);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
        ASSERT(FALSE);
        MmUnlockSectionSegment(Segment);
        return Status;
    }

    MmInsertRmap(NewPage, Process, PAddress);
    MmReleasePageMemoryConsumer(MC_CACHE, OldPage);
    MmUnlockSectionSegment(Segment);

    DPRINT("Address 0x%.8X\n", Address);
    return STATUS_SUCCESS;
}

KEVENT MmWaitPageEvent;

typedef struct _WORK_QUEUE_WITH_CONTEXT
{
    WORK_QUEUE_ITEM WorkItem;
    PMMSUPPORT AddressSpace;
    PMEMORY_AREA MemoryArea;
    PMM_REQUIRED_RESOURCES Required;
    NTSTATUS Status;
    KEVENT Wait;
    AcquireResource DoAcquisition;
} WORK_QUEUE_WITH_CONTEXT, *PWORK_QUEUE_WITH_CONTEXT;

VOID
NTAPI
MmpFaultWorker(PWORK_QUEUE_WITH_CONTEXT WorkItem)
{
    DPRINT("Calling work\n");
    WorkItem->Status = WorkItem->Required->DoAcquisition(WorkItem->AddressSpace,
                                                         WorkItem->MemoryArea,
                                                         WorkItem->Required);
    DPRINT("Status %x\n", WorkItem->Status);
    KeSetEvent(&WorkItem->Wait, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
NTAPI
MmpSectionAccessFaultInner(KPROCESSOR_MODE Mode,
                           PMMSUPPORT AddressSpace,
                           ULONG_PTR Address,
                           BOOLEAN FromMdl,
                           PETHREAD Thread)
{
    MEMORY_AREA* MemoryArea;
    NTSTATUS Status;
    BOOLEAN Locked = FromMdl;
    MM_REQUIRED_RESOURCES Resources = { 0 };
    WORK_QUEUE_WITH_CONTEXT Context;

    RtlZeroMemory(&Context, sizeof(WORK_QUEUE_WITH_CONTEXT));

    DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
        return STATUS_UNSUCCESSFUL;
    }

    /* Find the memory area for the faulting address */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /* Check permissions */
        if (Mode != KernelMode)
        {
            DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
            return STATUS_ACCESS_VIOLATION;
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }

    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL ||
            MemoryArea->DeleteInProgress)
        {
            if (!FromMdl)
            {
                MmUnlockAddressSpace(AddressSpace);
            }
            DPRINT("Address: %x\n", Address);
            return STATUS_ACCESS_VIOLATION;
        }

        DPRINT("Type %x (%x -> %x)\n",
               MemoryArea->Type,
               MemoryArea->StartingAddress,
               MemoryArea->EndingAddress);

        Resources.DoAcquisition = NULL;

        // Note: fault handlers are called with address space locked
        // We return STATUS_MORE_PROCESSING_REQUIRED if anything is needed
        Status = MiCowCacheSectionPage(AddressSpace,
                                       MemoryArea,
                                       (PVOID)Address,
                                       Locked,
                                       &Resources);

        if (!FromMdl)
        {
            MmUnlockAddressSpace(AddressSpace);
        }

        if (Status == STATUS_SUCCESS + 1)
        {
            /* Wait page ... */
            DPRINT("Waiting for %x\n", Address);
            MiWaitForPageEvent(MmGetAddressSpaceOwner(AddressSpace), Address);
            DPRINT("Restarting fault %x\n", Address);
            Status = STATUS_MM_RESTART_OPERATION;
        }
        else if (Status == STATUS_MM_RESTART_OPERATION)
        {
            /* Clean slate */
            RtlZeroMemory(&Resources, sizeof(Resources));
        }
        else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            if (Thread->ActiveFaultCount > 0)
            {
                DPRINT("Already fault handling ... going to work item (%x)\n",
                       Address);
                Context.AddressSpace = AddressSpace;
                Context.MemoryArea = MemoryArea;
                Context.Required = &Resources;
                KeInitializeEvent(&Context.Wait, NotificationEvent, FALSE);

                ExInitializeWorkItem(&Context.WorkItem,
                                     (PWORKER_THREAD_ROUTINE)MmpFaultWorker,
                                     &Context);

                DPRINT("Queue work item\n");
                ExQueueWorkItem(&Context.WorkItem, DelayedWorkQueue);
                DPRINT("Wait\n");
                KeWaitForSingleObject(&Context.Wait, 0, KernelMode, FALSE, NULL);
                Status = Context.Status;
                DPRINT("Status %x\n", Status);
            }
            else
            {
                Status = Resources.DoAcquisition(AddressSpace, MemoryArea, &Resources);
            }

            if (NT_SUCCESS(Status))
            {
                Status = STATUS_MM_RESTART_OPERATION;
            }
        }

        if (!FromMdl)
        {
            MmLockAddressSpace(AddressSpace);
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    if (!NT_SUCCESS(Status) && MemoryArea->Type == 1)
    {
        DPRINT1("Completed page fault handling %x %x\n", Address, Status);
        DPRINT1("Type %x (%x -> %x)\n",
                MemoryArea->Type,
                MemoryArea->StartingAddress,
                MemoryArea->EndingAddress);
    }

    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }

    return Status;
}

NTSTATUS
NTAPI
MmAccessFaultCacheSection(KPROCESSOR_MODE Mode,
                          ULONG_PTR Address,
                          BOOLEAN FromMdl)
{
    PETHREAD Thread;
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;

    DPRINT("MmpAccessFault(Mode %d, Address %x)\n", Mode, Address);

    Thread = PsGetCurrentThread();

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL %d, address %x\n",
                KeGetCurrentIrql(),
                Address);
        return STATUS_UNSUCCESSFUL;
    }

    /* Find the memory area for the faulting address */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /* Check permissions */
        if (Mode != KernelMode)
        {
            DPRINT1("Address: %x:%x\n", PsGetCurrentProcess(), Address);
            return STATUS_ACCESS_VIOLATION;
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    Thread->ActiveFaultCount++;
    Status = MmpSectionAccessFaultInner(Mode,
                                        AddressSpace,
                                        Address,
                                        FromMdl,
                                        Thread);
    Thread->ActiveFaultCount--;

    return Status;
}

NTSTATUS
NTAPI
MmNotPresentFaultCacheSectionInner(KPROCESSOR_MODE Mode,
                                   PMMSUPPORT AddressSpace,
                                   ULONG_PTR Address,
                                   BOOLEAN FromMdl,
                                   PETHREAD Thread)
{
    BOOLEAN Locked = FromMdl;
    PMEMORY_AREA MemoryArea;
    MM_REQUIRED_RESOURCES Resources = { 0 };
    WORK_QUEUE_WITH_CONTEXT Context;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlZeroMemory(&Context, sizeof(WORK_QUEUE_WITH_CONTEXT));

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }

    /* Call the memory area specific fault handler */
    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
        {
            Status = STATUS_ACCESS_VIOLATION;
            if (MemoryArea)
            {
                DPRINT1("Type %x DIP %x\n",
                        MemoryArea->Type,
                        MemoryArea->DeleteInProgress);
            }
            else
            {
                DPRINT1("No memory area\n");
            }
            DPRINT1("Process %x, Address %x\n",
                    MmGetAddressSpaceOwner(AddressSpace),
                    Address);
            break;
        }

        DPRINTC("Type %x (%x -> %x -> %x) in %x\n",
                MemoryArea->Type,
                MemoryArea->StartingAddress,
                Address,
                MemoryArea->EndingAddress,
                PsGetCurrentThread());

        Resources.DoAcquisition = NULL;

        // Note: fault handlers are called with address space locked
        // We return STATUS_MORE_PROCESSING_REQUIRED if anything is needed

        Status = MmNotPresentFaultCachePage(AddressSpace,
                                            MemoryArea,
                                            (PVOID)Address,
                                            Locked,
                                            &Resources);

        if (!FromMdl)
        {
            MmUnlockAddressSpace(AddressSpace);
        }

        if (Status == STATUS_SUCCESS)
        {
            ; // Nothing
        }
        else if (Status == STATUS_SUCCESS + 1)
        {
            /* Wait page ... */
            DPRINT("Waiting for %x\n", Address);
            MiWaitForPageEvent(MmGetAddressSpaceOwner(AddressSpace), Address);
            DPRINT("Done waiting for %x\n", Address);
            Status = STATUS_MM_RESTART_OPERATION;
        }
        else if (Status == STATUS_MM_RESTART_OPERATION)
        {
            /* Clean slate */
            DPRINT("Clear resource\n");
            RtlZeroMemory(&Resources, sizeof(Resources));
        }
        else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            if (Thread->ActiveFaultCount > 2)
            {
                DPRINTC("Already fault handling ... going to work item (%x)\n", Address);
                Context.AddressSpace = AddressSpace;
                Context.MemoryArea = MemoryArea;
                Context.Required = &Resources;
                KeInitializeEvent(&Context.Wait, NotificationEvent, FALSE);

                ExInitializeWorkItem(&Context.WorkItem,
                                     (PWORKER_THREAD_ROUTINE)MmpFaultWorker,
                                     &Context);

                DPRINT("Queue work item\n");
                ExQueueWorkItem(&Context.WorkItem, DelayedWorkQueue);
                DPRINT("Wait\n");
                KeWaitForSingleObject(&Context.Wait, 0, KernelMode, FALSE, NULL);
                Status = Context.Status;
                DPRINTC("Status %x\n", Status);
            }
            else
            {
                DPRINT("DoAcquisition %x\n", Resources.DoAcquisition);

                Status = Resources.DoAcquisition(AddressSpace,
                                                 MemoryArea,
                                                 &Resources);

                DPRINT("DoAcquisition %x -> %x\n",
                       Resources.DoAcquisition,
                       Status);
            }

            if (NT_SUCCESS(Status))
            {
                Status = STATUS_MM_RESTART_OPERATION;
            }
        }
        else if (NT_SUCCESS(Status))
        {
            ASSERT(FALSE);
        }

        if (!FromMdl)
        {
            MmLockAddressSpace(AddressSpace);
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINTC("Completed page fault handling: %x:%x %x\n",
            MmGetAddressSpaceOwner(AddressSpace),
            Address,
            Status);

    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }

    MiSetPageEvent(MmGetAddressSpaceOwner(AddressSpace), Address);
    DPRINT("Done %x\n", Status);

    return Status;
}

NTSTATUS
NTAPI
MmNotPresentFaultCacheSection(KPROCESSOR_MODE Mode,
                              ULONG_PTR Address,
                              BOOLEAN FromMdl)
{
    PETHREAD Thread;
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;

    Address &= ~(PAGE_SIZE - 1);
    DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

    Thread = PsGetCurrentThread();

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL %d, address %x\n",
                KeGetCurrentIrql(),
                Address);

        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    /* Find the memory area for the faulting address */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /* Check permissions */
        if (Mode != KernelMode)
        {
            DPRINTC("Address: %x\n", Address);
            return STATUS_ACCESS_VIOLATION;
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    Thread->ActiveFaultCount++;
    Status = MmNotPresentFaultCacheSectionInner(Mode,
                                                AddressSpace,
                                                Address,
                                                FromMdl,
                                                Thread);
    Thread->ActiveFaultCount--;

    ASSERT(Status != STATUS_UNSUCCESSFUL);
    ASSERT(Status != STATUS_INVALID_PARAMETER);
    DPRINT("MmAccessFault %x:%x -> %x\n",
           MmGetAddressSpaceOwner(AddressSpace),
           Address,
           Status);

    return Status;
}
