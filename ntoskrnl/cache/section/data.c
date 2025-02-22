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
 * FILE:            ntoskrnl/cache/section/data.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
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

A note on this code:

Unlike the previous section code, this code does not rely on an active map
for a page to exist in a data segment.  Each mapping contains a large integer
offset to map at, and the segment always represents the entire section space
from zero to the maximum long long.  This allows us to associate one single
page map with each file object, and to let each mapping view an offset into
the overall mapped file.  Temporarily unmapping the file has no effect on the
section membership.

This necessitates a change in the section page table implementation, which is
now an RtlGenericTable.  This will be elaborated more in sptab.c.  One upshot
of this change is that a mapping of a small files takes a bit more than 1/4
of the size in nonpaged kernel space as it did previously.

When we need other threads that may be competing for the same page fault to
wait, we have a mechanism separate from PageOps for dealing with that, which
was suggested by Travis Geiselbrecht after a conversation I had with Alex
Ionescu.  That mechanism is the MM_WAIT_ENTRY, which is the all-ones SWAPENTRY.

When we wish for other threads to know that we're waiting and will finish
handling a page fault, we place the swap entry MM_WAIT_ENTRY in the page table
at the fault address (this works on either the section page table or a process
address space), perform any blocking operations required, then replace the
entry.

*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#include <cache/newcc.h>
#define NDEBUG
#include <debug.h>
#include <mm/ARM3/miarm.h>

#define DPRINTC DPRINT

LIST_ENTRY MiSegmentList;

extern KEVENT MpwThreadEvent;
extern KSPIN_LOCK MiSectionPageTableLock;
extern PMMWSL MmWorkingSetList;

/* FUNCTIONS *****************************************************************/

/* Note: Mmsp prefix denotes "Memory Manager Section Private". */

VOID
NTAPI
_MmLockSectionSegment(PMM_SECTION_SEGMENT Segment, const char *file, int line)
{
    //DPRINT("MmLockSectionSegment(%p,%s:%d)\n", Segment, file, line);
    ExAcquireFastMutex(&Segment->Lock);
    Segment->Locked = TRUE;
}

VOID
NTAPI
_MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment, const char *file, int line)
{
    ASSERT(Segment->Locked);
    Segment->Locked = FALSE;
    ExReleaseFastMutex(&Segment->Lock);
    //DPRINT("MmUnlockSectionSegment(%p,%s:%d)\n", Segment, file, line);
}

#ifdef NEWCC
/*

MiFlushMappedSection

Called from cache code to cause dirty pages of a section
to be written back.  This doesn't affect the mapping.

BaseOffset is the base at which to start writing in file space.
FileSize is the length of the file as understood by the cache.

 */
NTSTATUS
NTAPI
_MiFlushMappedSection(PVOID BaseAddress,
                      PLARGE_INTEGER BaseOffset,
                      PLARGE_INTEGER FileSize,
                      BOOLEAN WriteData,
                      const char *File,
                      int Line)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR PageAddress;
    PMMSUPPORT AddressSpace = MmGetKernelAddressSpace();
    PMEMORY_AREA MemoryArea;
    PMM_SECTION_SEGMENT Segment;
    ULONG_PTR BeginningAddress, EndingAddress;
    LARGE_INTEGER ViewOffset;
    LARGE_INTEGER FileOffset;
    PFN_NUMBER Page;
    PPFN_NUMBER Pages;
    KIRQL OldIrql;

    DPRINT("MiFlushMappedSection(%p,%I64x,%I64x,%u,%s:%d)\n",
           BaseAddress,
           BaseOffset->QuadPart,
           FileSize ? FileSize->QuadPart : 0,
           WriteData,
           File,
           Line);

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
    if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_CACHE || MemoryArea->DeleteInProgress)
    {
        MmUnlockAddressSpace(AddressSpace);
        DPRINT("STATUS_NOT_MAPPED_DATA\n");
        return STATUS_NOT_MAPPED_DATA;
    }
    BeginningAddress = PAGE_ROUND_DOWN(MA_GetStartingAddress(MemoryArea));
    EndingAddress = PAGE_ROUND_UP(MA_GetEndingAddress(MemoryArea));
    Segment = MemoryArea->Data.SectionData.Segment;
    ViewOffset.QuadPart = MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    ASSERT(ViewOffset.QuadPart == BaseOffset->QuadPart);

    MmLockSectionSegment(Segment);

    Pages = ExAllocatePool(NonPagedPool,
                           sizeof(PFN_NUMBER) * ((EndingAddress - BeginningAddress) >> PAGE_SHIFT));

    if (!Pages)
    {
        ASSERT(FALSE);
    }

    //DPRINT("Getting pages in range %08x-%08x\n", BeginningAddress, EndingAddress);

    for (PageAddress = BeginningAddress;
         PageAddress < EndingAddress;
         PageAddress += PAGE_SIZE)
    {
        ULONG_PTR Entry;
        FileOffset.QuadPart = ViewOffset.QuadPart + PageAddress - BeginningAddress;
        Entry = MmGetPageEntrySectionSegment(MemoryArea->Data.SectionData.Segment,
                                             &FileOffset);
        Page = PFN_FROM_SSE(Entry);
        if (Entry != 0 && !IS_SWAP_FROM_SSE(Entry) &&
            (MmIsDirtyPageRmap(Page) || IS_DIRTY_SSE(Entry)) &&
            FileOffset.QuadPart < FileSize->QuadPart)
        {
            OldIrql = MiAcquirePfnLock();
            MmReferencePage(Page);
            MiReleasePfnLock(OldIrql);
            Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = Entry;
        }
        else
        {
            Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = 0;
        }
    }

    MmUnlockSectionSegment(Segment);
    MmUnlockAddressSpace(AddressSpace);

    for (PageAddress = BeginningAddress;
         PageAddress < EndingAddress;
         PageAddress += PAGE_SIZE)
    {
        ULONG_PTR Entry;
        FileOffset.QuadPart = ViewOffset.QuadPart + PageAddress - BeginningAddress;
        Entry = Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT];
        Page = PFN_FROM_SSE(Entry);
        if (Page)
        {
            if (WriteData) {
                //DPRINT("MiWriteBackPage(%wZ,addr %x,%08x%08x)\n", &Segment->FileObject->FileName, PageAddress, FileOffset.u.HighPart, FileOffset.u.LowPart);
                Status = MiWriteBackPage(Segment->FileObject, &FileOffset, PAGE_SIZE, Page);
            } else
                Status = STATUS_SUCCESS;

            if (NT_SUCCESS(Status)) {
                MmLockAddressSpace(AddressSpace);
                MmSetCleanAllRmaps(Page);

                MmSetPageProtect(MmGetAddressSpaceOwner(AddressSpace),
                                 (PVOID)PageAddress,
                                 PAGE_READONLY);

                MmLockSectionSegment(Segment);
                Entry = MmGetPageEntrySectionSegment(Segment, &FileOffset);

                if (Entry && !IS_SWAP_FROM_SSE(Entry) && PFN_FROM_SSE(Entry) == Page)
                    MmSetPageEntrySectionSegment(Segment, &FileOffset, CLEAN_SSE(Entry));

                MmUnlockSectionSegment(Segment);
                MmUnlockAddressSpace(AddressSpace);
            } else {
                DPRINT("Writeback from section flush %08x%08x (%x) %x@%x (%08x%08x:%wZ) failed %x\n",
                       FileOffset.u.HighPart,
                       FileOffset.u.LowPart,
                       (ULONG)(FileSize->QuadPart - FileOffset.QuadPart),
                       PageAddress,
                       Page,
                       FileSize->u.HighPart,
                       FileSize->u.LowPart,
                       &Segment->FileObject->FileName,
                       Status);
            }
            MmReleasePageMemoryConsumer(MC_CACHE, Page);
        }
    }

    ExFreePool(Pages);

    return Status;
}

/*

This deletes a segment entirely including its page map.
It must have been unmapped in every address space.

 */
VOID
NTAPI
MmFinalizeSegment(PMM_SECTION_SEGMENT Segment)
{
    KIRQL OldIrql = 0;

    DPRINT("Finalize segment %p\n", Segment);

    MmLockSectionSegment(Segment);
    RemoveEntryList(&Segment->ListOfSegments);
    if (Segment->Flags & MM_DATAFILE_SEGMENT) {
        KeAcquireSpinLock(&Segment->FileObject->IrpListLock, &OldIrql);
        if (Segment->Flags & MM_SEGMENT_FINALIZE) {
            KeReleaseSpinLock(&Segment->FileObject->IrpListLock, OldIrql);
            MmUnlockSectionSegment(Segment);
            return;
        }
        Segment->Flags |= MM_SEGMENT_FINALIZE;
        DPRINTC("Finalizing data file segment %p\n", Segment);

        Segment->FileObject->SectionObjectPointer->DataSectionObject = NULL;
        KeReleaseSpinLock(&Segment->FileObject->IrpListLock, OldIrql);
        MmFreePageTablesSectionSegment(Segment, MiFreeSegmentPage);
        MmUnlockSectionSegment(Segment);
        DPRINT("Dereference file object %wZ\n", &Segment->FileObject->FileName);
        ObDereferenceObject(Segment->FileObject);
        DPRINT("Done with %wZ\n", &Segment->FileObject->FileName);
        Segment->FileObject = NULL;
    } else {
        DPRINTC("Finalizing segment %p\n", Segment);
        MmFreePageTablesSectionSegment(Segment, MiFreeSegmentPage);
        MmUnlockSectionSegment(Segment);
    }
    DPRINTC("Segment %p destroy\n", Segment);
    ExFreePoolWithTag(Segment, TAG_MM_SECTION_SEGMENT);
}

NTSTATUS
NTAPI
MmCreateCacheSection(PROS_SECTION_OBJECT *SectionObject,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PLARGE_INTEGER UMaximumSize,
                     ULONG SectionPageProtection,
                     ULONG AllocationAttributes,
                     PFILE_OBJECT FileObject)
/*
 * Create a section backed by a data file.
 */
{
    PROS_SECTION_OBJECT Section;
    NTSTATUS Status;
    LARGE_INTEGER MaximumSize;
    PMM_SECTION_SEGMENT Segment;
    IO_STATUS_BLOCK Iosb;
    CC_FILE_SIZES FileSizes;
    FILE_STANDARD_INFORMATION FileInfo;
    KIRQL OldIrql;

    DPRINT("MmCreateDataFileSection\n");

    /* Create the section */
    Status = ObCreateObject(ExGetPreviousMode(),
                            MmSectionObjectType,
                            ObjectAttributes,
                            ExGetPreviousMode(),
                            NULL,
                            sizeof(ROS_SECTION_OBJECT),
                            0,
                            0,
                            (PVOID*)(PVOID)&Section);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed: %x\n", Status);
        ObDereferenceObject(FileObject);
        return Status;
    }

    /* Initialize it */
    RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));
    Section->Type = 'SC';
    Section->Size = 'TN';
    Section->SectionPageProtection = SectionPageProtection;
    Section->AllocationAttributes = AllocationAttributes;
    Section->Segment = NULL;

    Section->FileObject = FileObject;

    DPRINT("Getting original file size\n");
    /* A hack: If we're cached, we can overcome deadlocking with the upper
    * layer filesystem call by retriving the object sizes from the cache
    * which is made to keep track.  If I had to guess, they were figuring
    * out a similar problem.
    */
    if (!CcGetFileSizes(FileObject, &FileSizes))
    {
        ULONG Information;
        /*
        * FIXME: This is propably not entirely correct. We can't look into
        * the standard FCB header because it might not be initialized yet
        * (as in case of the EXT2FS driver by Manoj Paul Joseph where the
        * standard file information is filled on first request).
        */
        DPRINT("Querying info\n");
        Status = IoQueryFileInformation(FileObject,
                                        FileStandardInformation,
                                        sizeof(FILE_STANDARD_INFORMATION),
                                        &FileInfo,
                                        &Information);
        Iosb.Information = Information;
        DPRINT("Query => %x\n", Status);
        DBG_UNREFERENCED_LOCAL_VARIABLE(Iosb);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("Status %x\n", Status);
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return Status;
        }
        ASSERT(Status != STATUS_PENDING);

        FileSizes.ValidDataLength = FileInfo.EndOfFile;
        FileSizes.FileSize = FileInfo.EndOfFile;
    }
    DPRINT("Got %I64x\n", FileSizes.ValidDataLength.QuadPart);

    /*
    * FIXME: Revise this once a locking order for file size changes is
    * decided
    *
    * We're handed down a maximum size in every case.  Should we still check at all?
    */
    if (UMaximumSize != NULL && UMaximumSize->QuadPart)
    {
        DPRINT("Taking maximum %I64x\n", UMaximumSize->QuadPart);
        MaximumSize.QuadPart = UMaximumSize->QuadPart;
    }
    else
    {
        DPRINT("Got file size %I64x\n", FileSizes.FileSize.QuadPart);
        MaximumSize.QuadPart = FileSizes.FileSize.QuadPart;
    }

    /* Mapping zero-sized files isn't allowed. */
    if (MaximumSize.QuadPart == 0)
    {
        DPRINT("Zero size file\n");
        ObDereferenceObject(Section);
        ObDereferenceObject(FileObject);
        return STATUS_FILE_INVALID;
    }

    Segment = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(MM_SECTION_SEGMENT),
                                    TAG_MM_SECTION_SEGMENT);
    if (Segment == NULL)
    {
        DPRINT("Failed: STATUS_NO_MEMORY\n");
        ObDereferenceObject(Section);
        ObDereferenceObject(FileObject);
        return STATUS_NO_MEMORY;
    }

    DPRINT("Zeroing %p\n", Segment);
    RtlZeroMemory(Segment, sizeof(MM_SECTION_SEGMENT));
    ExInitializeFastMutex(&Segment->Lock);

    Segment->ReferenceCount = 1;
    Segment->Locked = TRUE;
    RtlZeroMemory(&Segment->Image, sizeof(Segment->Image));
    Section->Segment = Segment;

    KeAcquireSpinLock(&FileObject->IrpListLock, &OldIrql);
    /*
    * If this file hasn't been mapped as a data file before then allocate a
    * section segment to describe the data file mapping
    */
    if (FileObject->SectionObjectPointer->DataSectionObject == NULL)
    {
        FileObject->SectionObjectPointer->DataSectionObject = (PVOID)Segment;
        KeReleaseSpinLock(&FileObject->IrpListLock, OldIrql);

        /*
        * Set the lock before assigning the segment to the file object
        */
        ExAcquireFastMutex(&Segment->Lock);

        DPRINT("Filling out Segment info (No previous data section)\n");
        ObReferenceObject(FileObject);
        Segment->FileObject = FileObject;
        Segment->Protection = SectionPageProtection;
        Segment->Flags = MM_DATAFILE_SEGMENT;
        memset(&Segment->Image, 0, sizeof(Segment->Image));
        Segment->WriteCopy = FALSE;

        if (AllocationAttributes & SEC_RESERVE)
        {
            Segment->Length.QuadPart = Segment->RawLength.QuadPart = 0;
        }
        else
        {
            Segment->RawLength.QuadPart = MaximumSize.QuadPart;
            Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
        }
        MiInitializeSectionPageTable(Segment);
        InsertHeadList(&MiSegmentList, &Segment->ListOfSegments);
    }
    else
    {
        KeReleaseSpinLock(&FileObject->IrpListLock, OldIrql);
        DPRINTC("Free Segment %p\n", Segment);
        ExFreePoolWithTag(Segment, TAG_MM_SECTION_SEGMENT);

        DPRINT("Filling out Segment info (previous data section)\n");

        /*
        * If the file is already mapped as a data file then we may need
        * to extend it
        */
        Segment = (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->DataSectionObject;
        Section->Segment = Segment;
        (void)InterlockedIncrementUL(&Segment->ReferenceCount);

        MmLockSectionSegment(Segment);

        if (MaximumSize.QuadPart > Segment->RawLength.QuadPart &&
            !(AllocationAttributes & SEC_RESERVE))
        {
            Segment->RawLength.QuadPart = MaximumSize.QuadPart;
            Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
        }
    }

    MmUnlockSectionSegment(Segment);

    Section->MaximumSize.QuadPart = MaximumSize.QuadPart;

    /* Extend file if section is longer */
    DPRINT("MaximumSize %I64x ValidDataLength %I64x\n",
           MaximumSize.QuadPart,
           FileSizes.ValidDataLength.QuadPart);
    if (MaximumSize.QuadPart > FileSizes.ValidDataLength.QuadPart)
    {
        DPRINT("Changing file size to %I64x, segment %p\n",
               MaximumSize.QuadPart,
               Segment);

        Status = IoSetInformation(FileObject,
                                  FileEndOfFileInformation,
                                  sizeof(LARGE_INTEGER),
                                  &MaximumSize);

        DPRINT("Change: Status %x\n", Status);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Could not expand section\n");
            ObDereferenceObject(Section);
            return Status;
        }
    }

    DPRINTC("Segment %p created (%x)\n", Segment, Segment->Flags);

    *SectionObject = Section;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
_MiMapViewOfSegment(PMMSUPPORT AddressSpace,
                    PMM_SECTION_SEGMENT Segment,
                    PVOID* BaseAddress,
                    SIZE_T ViewSize,
                    ULONG Protect,
                    PLARGE_INTEGER ViewOffset,
                    ULONG AllocationType,
                    const char *file,
                    int line)
{
    PMEMORY_AREA MArea;
    NTSTATUS Status;

    Status = MmCreateMemoryArea(AddressSpace,
                                MEMORY_AREA_CACHE,
                                BaseAddress,
                                ViewSize,
                                Protect,
                                &MArea,
                                AllocationType,
                                *BaseAddress ?
                                PAGE_SIZE : MM_ALLOCATION_GRANULARITY);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Mapping between 0x%p and 0x%p failed (%X).\n",
               (*BaseAddress),
               (char*)(*BaseAddress) + ViewSize,
               Status);

        return Status;
    }

    DPRINTC("MiMapViewOfSegment %p %p %p %I64x %Ix %wZ %s:%d\n",
            MmGetAddressSpaceOwner(AddressSpace),
            *BaseAddress,
            Segment,
            ViewOffset ? ViewOffset->QuadPart : 0,
            ViewSize,
            Segment->FileObject ? &Segment->FileObject->FileName : NULL,
            file,
            line);

    MArea->Data.SectionData.Segment = Segment;
    if (ViewOffset)
        MArea->Data.SectionData.ViewOffset = *ViewOffset;
    else
        MArea->Data.SectionData.ViewOffset.QuadPart = 0;

#if 0
    MArea->NotPresent = MmNotPresentFaultPageFile;
    MArea->AccessFault = MiCowSectionPage;
    MArea->PageOut = MmPageOutPageFileView;
#endif

    MmInitializeRegion(&MArea->Data.SectionData.RegionListHead,
                       ViewSize,
                       0,
                       Protect);

    DPRINTC("MiMapViewOfSegment(P %p, A %p, T %x)\n",
            MmGetAddressSpaceOwner(AddressSpace),
            *BaseAddress,
            MArea->Type);

    return STATUS_SUCCESS;
}
#endif

/*

Completely remove the page at FileOffset in Segment.  The page must not
be mapped.

*/

VOID
NTAPI
MiFreeSegmentPage(PMM_SECTION_SEGMENT Segment,
                  PLARGE_INTEGER FileOffset)
{
    ULONG_PTR Entry;
    PFILE_OBJECT FileObject = Segment->FileObject;

    Entry = MmGetPageEntrySectionSegment(Segment, FileOffset);
    DPRINTC("MiFreeSegmentPage(%p:%I64x -> Entry %Ix\n",
            Segment,
            FileOffset->QuadPart,
            Entry);

    if (Entry && !IS_SWAP_FROM_SSE(Entry))
    {
        // The segment is carrying a dirty page.
        PFN_NUMBER OldPage = PFN_FROM_SSE(Entry);
        if (IS_DIRTY_SSE(Entry) && FileObject)
        {
            DPRINT("MiWriteBackPage(%p,%wZ,%I64x)\n",
                   Segment,
                   &FileObject->FileName,
                   FileOffset->QuadPart);

            MiWriteBackPage(FileObject, FileOffset, PAGE_SIZE, OldPage);
        }
        DPRINTC("Free page %Ix (off %I64x from %p) (ref ct %lu, ent %Ix, dirty? %s)\n",
                OldPage,
                FileOffset->QuadPart,
                Segment,
                MmGetReferenceCountPageWithoutLock(OldPage),
                Entry,
                IS_DIRTY_SSE(Entry) ? "true" : "false");

        MmSetPageEntrySectionSegment(Segment, FileOffset, 0);
        MmReleasePageMemoryConsumer(MC_CACHE, OldPage);
    }
    else if (IS_SWAP_FROM_SSE(Entry))
    {
        DPRINT("Free swap\n");
        MmFreeSwapPage(SWAPENTRY_FROM_SSE(Entry));
    }

    DPRINT("Done\n");
}

VOID
MmFreeCacheSectionPage(PVOID Context,
                       MEMORY_AREA* MemoryArea,
                       PVOID Address,
                       PFN_NUMBER Page,
                       SWAPENTRY SwapEntry,
                       BOOLEAN Dirty)
{
    ULONG_PTR Entry;
    PVOID *ContextData = Context;
    PMMSUPPORT AddressSpace;
    PEPROCESS Process;
    PMM_SECTION_SEGMENT Segment;
    LARGE_INTEGER Offset;

    DPRINT("MmFreeSectionPage(%p,%p,%Ix,%Ix,%u)\n",
           MmGetAddressSpaceOwner(ContextData[0]),
           Address,
           Page,
           SwapEntry,
           Dirty);

    AddressSpace = ContextData[0];
    Process = MmGetAddressSpaceOwner(AddressSpace);
    Address = (PVOID)PAGE_ROUND_DOWN(Address);
    Segment = ContextData[1];
    Offset.QuadPart = (ULONG_PTR)Address - MA_GetStartingAddress(MemoryArea) +
                      MemoryArea->Data.SectionData.ViewOffset.QuadPart;

    Entry = MmGetPageEntrySectionSegment(Segment, &Offset);

    if (Page != 0 && PFN_FROM_SSE(Entry) == Page && Dirty)
    {
        DPRINT("Freeing section page %p:%I64x -> %Ix\n", Segment, Offset.QuadPart, Entry);
        MmSetPageEntrySectionSegment(Segment, &Offset, DIRTY_SSE(Entry));
    }
    if (Page)
    {
        DPRINT("Removing page %p:%I64x -> %x\n", Segment, Offset.QuadPart, Entry);
        MmSetSavedSwapEntryPage(Page, 0);
        MmDeleteRmap(Page, Process, Address);
        MmDeleteVirtualMapping(Process, Address, NULL, NULL);
        MmReleasePageMemoryConsumer(MC_CACHE, Page);
    }
    if (SwapEntry != 0)
    {
        MmFreeSwapPage(SwapEntry);
    }
}

#ifdef NEWCC
NTSTATUS
NTAPI
MmUnmapViewOfCacheSegment(PMMSUPPORT AddressSpace,
                          PVOID BaseAddress)
{
    PVOID Context[2];
    PMEMORY_AREA MemoryArea;
    PMM_SECTION_SEGMENT Segment;

    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
    if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
    {
        ASSERT(MemoryArea);
        return STATUS_UNSUCCESSFUL;
    }

    MemoryArea->DeleteInProgress = TRUE;
    Segment = MemoryArea->Data.SectionData.Segment;
    MemoryArea->Data.SectionData.Segment = NULL;

    MmLockSectionSegment(Segment);

    Context[0] = AddressSpace;
    Context[1] = Segment;

    DPRINT("MmFreeMemoryArea(%p,%p)\n",
           MmGetAddressSpaceOwner(AddressSpace),
           MA_GetStartingAddress(MemoryArea));

    MmLockAddressSpace(AddressSpace);

    MmFreeMemoryArea(AddressSpace, MemoryArea, MmFreeCacheSectionPage, Context);

    MmUnlockAddressSpace(AddressSpace);

    MmUnlockSectionSegment(Segment);

    DPRINTC("MiUnmapViewOfSegment %p %p %p\n",
            MmGetAddressSpaceOwner(AddressSpace),
            BaseAddress,
            Segment);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmExtendCacheSection(PROS_SECTION_OBJECT Section,
                     PLARGE_INTEGER NewSize,
                     BOOLEAN ExtendFile)
{
    LARGE_INTEGER OldSize;
    PMM_SECTION_SEGMENT Segment = Section->Segment;
    DPRINT("Extend Segment %p\n", Segment);

    MmLockSectionSegment(Segment);
    OldSize.QuadPart = Segment->RawLength.QuadPart;
    MmUnlockSectionSegment(Segment);

    DPRINT("OldSize 0x%I64x NewSize 0x%I64x\n",
           OldSize.QuadPart,
           NewSize->QuadPart);

    if (ExtendFile && OldSize.QuadPart < NewSize->QuadPart)
    {
        NTSTATUS Status;

        Status = IoSetInformation(Segment->FileObject,
                                  FileEndOfFileInformation,
                                  sizeof(LARGE_INTEGER),
                                  NewSize);

        if (!NT_SUCCESS(Status)) return Status;
    }

    MmLockSectionSegment(Segment);
    Segment->RawLength.QuadPart = NewSize->QuadPart;
    Segment->Length.QuadPart = MAX(Segment->Length.QuadPart,
                                   (LONG64)PAGE_ROUND_UP(Segment->RawLength.QuadPart));
    MmUnlockSectionSegment(Segment);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmMapCacheViewInSystemSpaceAtOffset(IN PMM_SECTION_SEGMENT Segment,
                                    OUT PVOID *MappedBase,
                                    PLARGE_INTEGER FileOffset,
                                    IN OUT PULONG ViewSize)
{
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;

    DPRINT("MmMapViewInSystemSpaceAtOffset() called offset 0x%I64x\n",
           FileOffset->QuadPart);

    AddressSpace = MmGetKernelAddressSpace();

    MmLockAddressSpace(AddressSpace);
    MmLockSectionSegment(Segment);

    Status = MiMapViewOfSegment(AddressSpace,
                                Segment,
                                MappedBase,
                                *ViewSize,
                                PAGE_READWRITE,
                                FileOffset,
                                0);

    MmUnlockSectionSegment(Segment);
    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
MmUnmapCacheViewInSystemSpace (IN PVOID MappedBase)
{
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;

    DPRINT("MmUnmapViewInSystemSpace() called\n");

    AddressSpace = MmGetKernelAddressSpace();

    Status = MmUnmapViewOfCacheSegment(AddressSpace, MappedBase);

    return Status;
}
#endif /* NEWCC */

/* EOF */
