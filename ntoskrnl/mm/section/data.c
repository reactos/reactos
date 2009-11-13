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
 * FILE:            ntoskrnl/mm/section.c
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

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern KEVENT MpwThreadEvent;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

/* GLOBALS *******************************************************************/

POBJECT_TYPE MmSectionObjectType = NULL;

ULONG_PTR MmSubsectionBase;

NTSTATUS
NTAPI
MiSimpleRead
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset,
 PVOID Buffer, 
 ULONG Length,
 PIO_STATUS_BLOCK ReadStatus);

static GENERIC_MAPPING MmpSectionMapping = {
         STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
         STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
         STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
         SECTION_ALL_ACCESS};

static const INFORMATION_CLASS_INFO ExSectionInfoClass[] =
{
  ICI_SQ_SAME( sizeof(SECTION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionBasicInformation */
  ICI_SQ_SAME( sizeof(SECTION_IMAGE_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionImageInformation */
};

/* FUNCTIONS *****************************************************************/

/* Note: Mmsp prefix denotes "Memory Manager Section Private". */

/*
 * FUNCTION:  Waits in kernel mode up to ten seconds for an MM_PAGEOP event.
 * ARGUMENTS: PMM_PAGEOP which event we should wait for.
 * RETURNS:   Status of the wait.
 */
NTSTATUS
MmspWaitForPageOpCompletionEvent(PMM_PAGEOP PageOp)
{
   LARGE_INTEGER Timeout;
#ifdef __GNUC__ /* TODO: Use other macro to check for suffix to use? */

   Timeout.QuadPart = -100000000LL; // 10 sec
#else

   Timeout.QuadPart = -100000000; // 10 sec
#endif

   return KeWaitForSingleObject(&PageOp->CompletionEvent, 0, KernelMode, FALSE, &Timeout);
}


/*
 * FUNCTION:  Sets the page op completion event and releases the page op.
 * ARGUMENTS: PMM_PAGEOP.
 * RETURNS:   In shorter time than it takes you to even read this
 *            description, so don't even think about geting a mug of coffee.
 */
VOID
MmspCompleteAndReleasePageOp(PMM_PAGEOP PageOp)
{
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
}

VOID
NTAPI
MmLockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   DPRINT("MmLockSectionSegment(%p)\n", Segment);
   ExAcquireFastMutex(&Segment->Lock);
}

VOID
NTAPI
MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExReleaseFastMutex(&Segment->Lock);
   DPRINT("MmUnlockSectionSegment(%p)\n", Segment);
}

VOID
NTAPI
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                               PLARGE_INTEGER Offset)
{
   ULONG Entry;

   Entry = MiGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
      ASSERT(FALSE);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == MAX_SHARE_COUNT)
   {
      DPRINT1("Maximum share count reached\n");
      ASSERT(FALSE);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      ASSERT(FALSE);
   }
   Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) + 1);
   MiSetPageEntrySectionSegment(Segment, Offset, Entry);
}

BOOLEAN
NTAPI
MmUnsharePageEntrySectionSegment(PROS_SECTION_OBJECT Section,
                                 PMM_SECTION_SEGMENT Segment,
                                 PLARGE_INTEGER Offset,
                                 BOOLEAN Dirty,
                                 BOOLEAN PageOut)
{
   ULONG Entry;

   DPRINT("MmUnsharePageEntrySectionSegment(%p,%x,Type %x)\n", Segment, Offset, Segment->Flags);

   Entry = MiGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("Entry == 0 for MmUnsharePageEntrySectionSegment\n");
      ASSERT(FALSE);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == 0)
   {
      DPRINT1("Zero share count for unshare\n");
      ASSERT(FALSE);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      ASSERT(FALSE);
   }
   Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) - 1);
   /*
    * If we reducing the share count of this entry to zero then set the entry
    * to zero and tell the cache the page is no longer mapped.
    */
   if (SHARE_COUNT_FROM_SSE(Entry) == 0)
   {
      PFILE_OBJECT FileObject;
      SWAPENTRY SavedSwapEntry;
      PFN_TYPE Page;
	  NTSTATUS Status;

      Page = PFN_FROM_SSE(Entry);
      FileObject = Section->FileObject;

	  if (MmIsDirtyPageRmap(Page))
	  {
		  DPRINT("MiWriteBackPage(%wZ,%08x%08x)\n", &FileObject->FileName, Offset->u.HighPart, Offset->u.LowPart);
		  Status = MiWriteBackPage(FileObject, Offset, PAGE_SIZE, Page);
		  if (!NT_SUCCESS(Status))
		  {
			  DPRINT1("CcRosUnmapCacheSegment failed, status = %x\n", Status);
			  ASSERT(FALSE);
		  }
		  MmSetCleanAllRmaps(Page);
	  }

	  SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
	  if (SavedSwapEntry == 0)
	  {
		  MiSetPageEntrySectionSegment(Segment, Offset, 0);
		  MmReleasePageMemoryConsumer(MC_USER, Page);
	  }
   }
   else
   {
      MiSetPageEntrySectionSegment(Segment, Offset, Entry);
   }
   DPRINT("MmUnsharePageEntrySectionSegment(%p,%x) -> Done\n", Segment, Offset);
   return(SHARE_COUNT_FROM_SSE(Entry) > 0);
}

NTSTATUS
NTAPI
MiReadFilePage(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PPFN_TYPE Page)
{
	PFN_TYPE XPage;
	NTSTATUS Status;
	PVOID PageBuf = NULL;
	PMEMORY_AREA TmpArea;
	IO_STATUS_BLOCK IOSB;
	PHYSICAL_ADDRESS BoundaryAddressMultiple;

	BoundaryAddressMultiple.QuadPart = 0;

	DPRINT
		("Pulling page %08x%08x from disk\n", 
		 FileOffset->u.HighPart, FileOffset->u.LowPart);

	Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, Page);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Status: %x\n", Status);
		return Status;
	}
	
	MmLockAddressSpace(MmGetKernelAddressSpace());
	Status = MmCreateMemoryArea
		(MmGetKernelAddressSpace(),
		 MEMORY_AREA_VIRTUAL_MEMORY, 
		 &PageBuf,
		 PAGE_SIZE,
		 PAGE_READWRITE,
		 &TmpArea,
		 FALSE,
		 MEM_TOP_DOWN,
		 BoundaryAddressMultiple);
	
	DPRINT("Status %x, PageBuf %x\n", Status, PageBuf);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("STATUS_NO_MEMORY: %x\n", Status);
		MmUnlockAddressSpace(MmGetKernelAddressSpace());
		MmReleasePageMemoryConsumer(MC_USER, *Page);
		return STATUS_NO_MEMORY;
	}
	
	Status = MmCreateVirtualMapping(NULL, PageBuf, PAGE_READWRITE, Page, 1);
	if (!NT_SUCCESS(Status))
	{
		MmFreeMemoryArea(MmGetKernelAddressSpace(), TmpArea, NULL, NULL);
		MmUnlockAddressSpace(MmGetKernelAddressSpace());
		MmReleasePageMemoryConsumer(MC_USER, *Page);
		DPRINT1("Status: %x\n", Status);
		return Status;
	}
	
	MmUnlockAddressSpace(MmGetKernelAddressSpace());

	MiZeroPage(*Page);
	Status = MiSimpleRead
		(FileObject, 
		 FileOffset,
		 PageBuf,
		 PAGE_SIZE,
		 &IOSB);
	
	DPRINT("Read Status %x (Page %x)\n", Status, *Page);

	MmLockAddressSpace(MmGetKernelAddressSpace());
	MmDeleteVirtualMapping(NULL, PageBuf, FALSE, NULL, &XPage);
	ASSERT(XPage == *Page);
	MmFreeMemoryArea(MmGetKernelAddressSpace(), TmpArea, NULL, NULL);
	MmUnlockAddressSpace(MmGetKernelAddressSpace());

	if (!NT_SUCCESS(Status))
	{
		MmReleasePageMemoryConsumer(MC_USER, *Page);
		DPRINT("Status: %x\n", Status);
		return Status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(PMMSUPPORT AddressSpace,
                             MEMORY_AREA* MemoryArea,
                             PVOID Address,
                             BOOLEAN Locked)
{
	ULONG Offset;
	PFN_TYPE Page;
	NTSTATUS Status;
	PVOID PAddress;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	ULONG Entry;
	ULONG Entry1;
	ULONG Attributes;
	PMM_REGION Region;
	BOOLEAN HasSwapEntry;
	LARGE_INTEGER TotalOffset;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

	DPRINT("Not Present: %p %p (%p-%p)\n", AddressSpace, Address, MemoryArea->StartingAddress, MemoryArea->EndingAddress);
    
	/*
	 * There is a window between taking the page fault and locking the
	 * address space when another thread could load the page so we check
	 * that.
	 */
	if (MmIsPagePresent(Process, Address))
	{
		if (Locked)
		{
			MmLockPage(MmGetPfnForProcess(Process, Address));
		}
		return(STATUS_SUCCESS);
	}

	PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
	Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;

	Segment = MemoryArea->Data.SectionData.Segment;
	Section = MemoryArea->Data.SectionData.Section;
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);
	TotalOffset.QuadPart = MemoryArea->Data.SectionData.ViewOffset.QuadPart + Offset;

	/*
	 * Lock the segment
	 */
	MmLockSectionSegment(Segment);

	Entry = MiGetPageEntrySectionSegment(Segment, &TotalOffset);
	HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);
	DPRINT("Entry %x HasSwapEntry %x Offset %08x%08x\n", Entry, HasSwapEntry, TotalOffset.u.HighPart, TotalOffset.u.LowPart);

	if (Entry == 0 && !HasSwapEntry)
	{
		DPRINT("Segment->RawLength %08x%08x\n", Segment->RawLength.u.HighPart, Segment->RawLength.u.LowPart);
		DPRINT("Segment->Length    %08x%08x\n", Segment->Length.u.HighPart, Segment->Length.u.LowPart);
		DPRINT("Reading at offset %08x%08x (relative %x)\n", TotalOffset.HighPart, TotalOffset.LowPart, Offset);
	
		MmUnlockSectionSegment(Segment);
		MmUnlockAddressSpace(AddressSpace);
		Status = MiReadFilePage(Segment->FileObject, &TotalOffset, &Page);
		MmLockAddressSpace(AddressSpace);

		if (!NT_SUCCESS(Status))
		{
			return Status;
		}

		MmLockSectionSegment(Segment);
		Entry = MiGetPageEntrySectionSegment(Segment, &TotalOffset);

		if (Entry != 0) // Handled elsewhere
		{
			MmReleasePageMemoryConsumer(MC_USER, Page);
			DPRINT("Handled elsewhere\n");
			MmUnlockSectionSegment(Segment);
			return STATUS_SUCCESS;
		}
	}

	DPRINT("Entry %x\n", Entry);

	/*
	 * Check if this page needs to be mapped COW
	 */
	if ((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
		(Region->Protect == PAGE_READWRITE ||
		 Region->Protect == PAGE_EXECUTE_READWRITE))
	{
		Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
	}
	else
	{
		Attributes = Region->Protect;
	}

	if (HasSwapEntry)
	{
		/*
		 * Must be private page we have swapped out.
		 */
		SWAPENTRY SwapEntry;

		MmDeletePageFileMapping(Process, (PVOID)PAddress, &SwapEntry);
		Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
		if (!NT_SUCCESS(Status))
		{
			ASSERT(FALSE);
		}

		Status = MmReadFromSwapPage(SwapEntry, Page);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
			ASSERT(FALSE);
		}
		DPRINT("CreateVirtualMapping: %x -> %x\n", Address, Attributes);
		Status = MmCreateVirtualMapping(Process,
										Address,
										Attributes,
										&Page,
										1);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
			ASSERT(FALSE);
			return(Status);
		}

		/*
		 * Store the swap entry for later use.
		 */
		MmSetSavedSwapEntryPage(Page, SwapEntry);

		/*
		 * Add the page to the process's working set
		 */
		MmInsertRmap(Page, Process, (PVOID)PAddress);

		/*
		 * Finish the operation
		 */
		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}

	if (Entry == 0)
	{
		Status = MmCreateVirtualMapping(Process,
										Address,
										Attributes,
										&Page,
										1);

		if (!NT_SUCCESS(Status))
		{
			/*
			 * FIXME: What do we know in this case?
			 */
			/*
			 * Cleanup and release locks
			 */
			MmUnlockSectionSegment(Segment);
			DPRINT("Address 0x%.8X\n", Address);
			return(Status);
		}

		/*
		 * Check the entry. No one should change the status of a page
		 * that has a pending page-in.
		 */
		Entry1 = MiGetPageEntrySectionSegment(Segment, &TotalOffset);
		if (Entry != Entry1)
		{
			DPRINT1("Someone changed ppte entry while we slept\n");
			ASSERT(FALSE);
		}

		/*
		 * Mark the offset within the section as having valid, in-memory
		 * data
		 */
		Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
		MiSetPageEntrySectionSegment(Segment, &TotalOffset, Entry);

		MmInsertRmap(Page, Process, (PVOID)PAddress);
		MmSetCleanAllRmaps(Page);
		ASSERT(!MmIsDirtyPageRmap(Page));

		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
	else if (IS_SWAP_FROM_SSE(Entry))
	{
		Status = MiSwapInSectionPage(AddressSpace, MemoryArea, Segment, Address, &Page);
		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
	else
	{
		/*
		 * If the section offset is already in-memory and valid then just
		 * take another reference to the page
		 */

		Page = PFN_FROM_SSE(Entry);

		MmSharePageEntrySectionSegment(Segment, &TotalOffset);

		Status = MmCreateVirtualMapping(Process,
										Address,
										Attributes,
										&Page,
										1);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("Unable to create virtual mapping\n");
			ASSERT(FALSE);
		}
		MmInsertRmap(Page, Process, (PVOID)PAddress);
		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
}

NTSTATUS
NTAPI
MiCopyFromUserPage(PFN_TYPE DestPage, PVOID SourceAddress)
{
    PEPROCESS Process;
    KIRQL Irql;
    PVOID TempAddress;
    
    Process = PsGetCurrentProcess();
    TempAddress = MiMapPageInHyperSpace(Process, DestPage, &Irql);
    if (TempAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    memcpy(TempAddress, SourceAddress, PAGE_SIZE);
    MiUnmapPageInHyperSpace(Process, TempAddress, Irql);
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiCowSectionPage
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
   PMM_SECTION_SEGMENT Segment;
   PROS_SECTION_OBJECT Section;
   PFN_TYPE OldPage;
   PFN_TYPE NewPage;
   NTSTATUS Status;
   PVOID PAddress;
   LARGE_INTEGER Offset;
   PMM_REGION Region;
   ULONG Entry;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   DPRINT("MmAccessFaultSectionView(%x, %x, %x, %x)\n", AddressSpace, MemoryArea, Address, Locked);

   /*
    * Check if the page has been paged out or has already been set readwrite
    */
   if (!MmIsPagePresent(Process, Address) ||
         MmGetPageProtect(Process, Address) & PAGE_READWRITE)
   {
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Find the offset of the page
    */
   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   Offset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress +
	   MemoryArea->Data.SectionData.ViewOffset.QuadPart;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   OldPage = MmGetPfnForProcess(Process, Address);
   Entry = MiGetPageEntrySectionSegment(Segment, &Offset);

   /*
    * Check if we are doing COW
    */
   if (!((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
         (Region->Protect == PAGE_READWRITE ||
          Region->Protect == PAGE_EXECUTE_READWRITE)))
   {
      DPRINT("Address 0x%.8X\n", Address);
	  MmUnlockSectionSegment(Segment);
      return(STATUS_ACCESS_VIOLATION);
   }

   if (IS_SWAP_FROM_SSE(Entry) ||
       PFN_FROM_SSE(Entry) != OldPage)
   {
      /* This is a private page. We must only change the page protection. */
      MmSetPageProtect(Process, PAddress, Region->Protect);
	  MmUnlockSectionSegment(Segment);
      return(STATUS_SUCCESS);
   }

   /*
    * Allocate a page
    */
   Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &NewPage);
   if (!NT_SUCCESS(Status))
   {
      ASSERT(FALSE);
   }

   /*
    * Copy the old page
    */
   MiCopyFromUserPage(NewPage, PAddress);

   /*
    * Delete the old entry.
    */
   MmDeleteVirtualMapping(Process, Address, FALSE, NULL, NULL);

   /*
    * Set the PTE to point to the new page
    */
   Status = MmCreateVirtualMapping(Process,
                                   Address,
                                   Region->Protect,
                                   &NewPage,
                                   1);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
      ASSERT(FALSE);
	  MmUnlockSectionSegment(Segment);
      return(Status);
   }
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Unable to create virtual mapping\n");
      ASSERT(FALSE);
   }
   if (Locked)
   {
      MmLockPage(NewPage);
      MmUnlockPage(OldPage);
   }

   /*
    * Unshare the old page.
    */
   MmDeleteRmap(OldPage, Process, PAddress);
   MmInsertRmap(NewPage, Process, PAddress);
   MmUnsharePageEntrySectionSegment(Section, Segment, &Offset, FALSE, FALSE);
   MmUnlockSectionSegment(Segment);

   DPRINT("Address 0x%.8X\n", Address);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiSwapInSectionPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_SECTION_SEGMENT Segment,
 PVOID Address,
 PPFN_TYPE Page)
{
	NTSTATUS Status;
	ULONG Entry, Entry1;
	SWAPENTRY SwapEntry;
	PMM_REGION Region;
	LARGE_INTEGER Offset;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
	
	Address = MM_ROUND_DOWN(Address, PAGE_SIZE);
	Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress +
		MemoryArea->Data.SectionData.ViewOffset.QuadPart;
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);
	Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
	SwapEntry = SWAPENTRY_FROM_SSE(Entry);
	
	/*
	 * Release all our locks and read in the page from disk
	 */
	MmUnlockSectionSegment(Segment);
	
	MmUnlockAddressSpace(AddressSpace);
	
	Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, Page);
	if (!NT_SUCCESS(Status))
	{
		ASSERT(FALSE);
	}
	
	Status = MmReadFromSwapPage(SwapEntry, *Page);
	if (!NT_SUCCESS(Status))
	{
		ASSERT(FALSE);
	}
	
	/*
	 * Relock the address space and segment
	 */
	MmLockAddressSpace(AddressSpace);
	MmLockSectionSegment(Segment);
	
	/*
	 * Check the entry. No one should change the status of a page
	 * that has a pending page-in.
	 */
	Entry1 = MiGetPageEntrySectionSegment(Segment, &Offset);
	if (Entry != Entry1)
	{
		DPRINT1("Someone changed ppte entry while we slept\n");
		ASSERT(FALSE);
	}
	
	/*
	 * Mark the offset within the section as having valid, in-memory
	 * data
	 */
	Entry = MAKE_SSE(*Page << PAGE_SHIFT, 1);
	MiSetPageEntrySectionSegment(Segment, &Offset, Entry);
	MmUnlockSectionSegment(Segment);
	
	/*
	 * Save the swap entry.
	 */
	MmSetSavedSwapEntryPage(*Page, SwapEntry);
	Status = MmCreateVirtualMapping(Process,
									Address,
									Region->Protect,
									Page,
									1);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Unable to create virtual mapping\n");
		ASSERT(FALSE);
	}
	MmInsertRmap(*Page, Process, (PVOID)Address);
	return Status;
}

NTSTATUS
NTAPI
MiZeroFillSection
(PVOID Address,
 PLARGE_INTEGER FileOffsetPtr,
 ULONG Length)
{
	PFN_TYPE Page;
	PMMSUPPORT AddressSpace;
	PMEMORY_AREA MemoryArea;
	PMM_SECTION_SEGMENT Segment;
	LARGE_INTEGER FileOffset = *FileOffsetPtr, End, FirstMapped;
	AddressSpace = MmGetKernelAddressSpace();
	MmLockAddressSpace(AddressSpace);
	MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
	if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_SECTION_VIEW) 
	{
		MmUnlockAddressSpace(AddressSpace);
		return STATUS_NOT_MAPPED_DATA;
	}

	Segment = MemoryArea->Data.SectionData.Segment;
	End.QuadPart = FileOffset.QuadPart + Length;
	End.LowPart = PAGE_ROUND_DOWN(End.LowPart);
	FileOffset.LowPart = PAGE_ROUND_UP(FileOffset.LowPart);
	FirstMapped.QuadPart = MemoryArea->Data.SectionData.ViewOffset.QuadPart;
	DPRINT1
		("Pulling zero pages for %08x%08x-%08x%08x\n",
		 FileOffset.u.HighPart, FileOffset.u.LowPart,
		 End.u.HighPart, End.u.LowPart);
	MmLockSectionSegment(Segment);
	while (FileOffset.QuadPart < End.QuadPart)
	{
		PVOID Address;
		ULONG Entry = MiGetPageEntrySectionSegment(Segment, &FileOffset);
		if (Entry == 0)
		{
			if (!NT_SUCCESS(MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page)))
			{
				MmUnlockSectionSegment(Segment);
				MmUnlockAddressSpace(AddressSpace);
				return STATUS_NO_MEMORY;
			}
			Address = ((PCHAR)MemoryArea->StartingAddress) + FileOffset.QuadPart - FirstMapped.QuadPart;
			MmCreateVirtualMapping(NULL, Address, PAGE_READWRITE, &Page, 1);
			MmInsertRmap(Page, NULL, Address);
			MiSetPageEntrySectionSegment(Segment, &FileOffset, MAKE_SSE(Page << PAGE_SHIFT, 1));
		}
		FileOffset.QuadPart += PAGE_SIZE;
	}
	MmUnlockSectionSegment(Segment);
	MmUnlockAddressSpace(AddressSpace);
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmAccessFaultSectionView(PMMSUPPORT AddressSpace,
                         MEMORY_AREA* MemoryArea,
                         PVOID Address,
                         BOOLEAN Locked)
{
	return MiCowSectionPage(AddressSpace, MemoryArea, Address, Locked);
}

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
{
   MM_SECTION_PAGEOUT_CONTEXT* PageOutContext;
   BOOLEAN WasDirty;
   PFN_TYPE Page;

   PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
   if (Process)
   {
      MmLockAddressSpace(&Process->Vm);
   }

   MmDeleteVirtualMapping(Process,
                          Address,
                          FALSE,
                          &WasDirty,
                          &Page);
   if (WasDirty)
   {
      PageOutContext->WasDirty = TRUE;
   }
   if (!PageOutContext->Private)
   {
      MmLockSectionSegment(PageOutContext->Segment);
      MmUnsharePageEntrySectionSegment((PROS_SECTION_OBJECT)PageOutContext->Section,
                                       PageOutContext->Segment,
                                       &PageOutContext->Offset,
                                       PageOutContext->WasDirty,
                                       TRUE);
      MmUnlockSectionSegment(PageOutContext->Segment);
   }
   if (Process)
   {
      MmUnlockAddressSpace(&Process->Vm);
   }

   if (PageOutContext->Private)
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
   }

   DPRINT("PhysicalAddress %x, Address %x\n", Page << PAGE_SHIFT, Address);
}

NTSTATUS
NTAPI
MmPageOutSectionView(PMMSUPPORT AddressSpace,
                     MEMORY_AREA* MemoryArea,
                     PVOID Address,
                     PMM_PAGEOP PageOp)
{
   PFN_TYPE Page;
   MM_SECTION_PAGEOUT_CONTEXT Context;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;
   Context.Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress +
	   MemoryArea->Data.SectionData.ViewOffset.QuadPart;

   FileObject = Context.Section->FileObject;

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MiGetPageEntrySectionSegment(Context.Segment, &Context.Offset);
   if (!MmIsPagePresent(Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              Process ? Process->UniqueProcessId : 0, Address);
      ASSERT(FALSE);
   }
   Page = MmGetPfnForProcess(Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Prepare the context structure for the rmap delete call.
    */
   Context.WasDirty = FALSE;
   if (IS_SWAP_FROM_SSE(Entry) || PFN_FROM_SSE(Entry) != Page)
   {
      Context.Private = TRUE;
   }
   else
   {
      Context.Private = FALSE;
   }

   MmReferencePage(Page);
   MmDeleteAllRmaps(Page, (PVOID)&Context, MmPageOutDeleteMapping);

   /*
    * If the page wasn't dirty then we can just free it as for a readonly page.
    * Since we unmapped all the mappings above we know it will not suddenly
    * become dirty.
    * If the page is from a pagefile section and has no swap entry,
    * we can't free the page at this point.
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (!Context.Private)
   {

      if (SwapEntry != 0)
      {
         DPRINT1("Found a swapentry for a non private and direct mapped page (address %x)\n",
                 Address);
         ASSERT(FALSE);
      }

	  if (Context.WasDirty)
	  {
		  DPRINT("MiWriteBackPage(%wZ,%08x%08x)\n", &FileObject->FileName, Context.Offset.u.HighPart, Context.Offset.u.LowPart);
		  Status = MiWriteBackPage(FileObject, &Context.Offset, PAGE_SIZE, Page);
		  if (!NT_SUCCESS(Status))
		  {
			  DPRINT1("CCRosUnmapCacheSegment failed, status = %x\n", Status);
			  ASSERT(FALSE);
		  }
	  }

      MmReleasePageMemoryConsumer(MC_USER, Page);
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }
   else if (!Context.WasDirty && Context.Private && SwapEntry != 0)
   {
      MmSetSavedSwapEntryPage(Page, 0);
      MmLockAddressSpace(AddressSpace);
      Status = MmCreatePageFileMapping(Process,
                                       Address,
                                       SwapEntry);
      MmUnlockAddressSpace(AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         ASSERT(FALSE);
      }
      MmReleasePageMemoryConsumer(MC_USER, Page);
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmShowOutOfSpaceMessagePagingFile();
         MmLockAddressSpace(AddressSpace);
         /*
          * For private pages restore the old mappings.
          */
         if (Context.Private)
         {
            Status = MmCreateVirtualMapping(Process,
                                            Address,
                                            MemoryArea->Protect,
                                            &Page,
                                            1);
            MmSetDirtyPage(Process, Address);
            MmInsertRmap(Page,
                         Process,
                         Address);
         }
         else
         {
            /*
             * For non-private pages if the page wasn't direct mapped then
             * set it back into the section segment entry so we don't loose
             * our copy. Otherwise it will be handled by the cache manager.
             */
            Status = MmCreateVirtualMapping(Process,
                                            Address,
                                            MemoryArea->Protect,
                                            &Page,
                                            1);
            MmSetDirtyPage(Process, Address);
            MmInsertRmap(Page,
                         Process,
                         Address);
            Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
            MiSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
         }
         MmUnlockAddressSpace(AddressSpace);
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
      }
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
      /*
       * As above: undo our actions.
       * FIXME: Also free the swap page.
       */
      MmLockAddressSpace(AddressSpace);
      if (Context.Private)
      {
         Status = MmCreateVirtualMapping(Process,
                                         Address,
                                         MemoryArea->Protect,
                                         &Page,
                                         1);
         MmSetDirtyPage(Process, Address);
         MmInsertRmap(Page,
                      Process,
                      Address);
      }
      else
      {
         Status = MmCreateVirtualMapping(Process,
                                         Address,
                                         MemoryArea->Protect,
                                         &Page,
                                         1);
         MmSetDirtyPage(Process, Address);
         MmInsertRmap(Page,
                      Process,
                      Address);
         Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
         MiSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
      }
      MmUnlockAddressSpace(AddressSpace);
      PageOp->Status = STATUS_UNSUCCESSFUL;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
   MmSetSavedSwapEntryPage(Page, 0);
   MmReleasePageMemoryConsumer(MC_USER, Page);

   if (Context.Private)
   {
      MmLockAddressSpace(AddressSpace);
      Status = MmCreatePageFileMapping(Process,
                                       Address,
                                       SwapEntry);
      MmUnlockAddressSpace(AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         ASSERT(FALSE);
      }
   }
   else
   {
      Entry = MAKE_SWAP_SSE(SwapEntry);
      MiSetPageEntrySectionSegment(Context.Segment, &Context.Offset, Entry);
   }

   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmWritePageSectionView(PMMSUPPORT AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
                       PMM_PAGEOP PageOp)
{
   LARGE_INTEGER Offset;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PFN_TYPE Page;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   BOOLEAN Private;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress +
	   MemoryArea->Data.SectionData.ViewOffset.QuadPart;

   /*
    * Get the segment and section.
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;

   FileObject = Section->FileObject;
   ASSERT(FileObject);

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
   if (!MmIsPagePresent(Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              Process ? Process->UniqueProcessId : 0, Address);
      ASSERT(FALSE);
   }
   Page = MmGetPfnForProcess(Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Check for a private (COWed) page.
    */
   if (IS_SWAP_FROM_SSE(Entry) || PFN_FROM_SSE(Entry) != Page)
   {
      Private = TRUE;
   }
   else
   {
      Private = FALSE;
   }

   /*
    * Speculatively set all mappings of the page to clean.
    */
   MmSetCleanAllRmaps(Page);

   /*
    * If this page was direct mapped from the cache then the cache manager
    * will take care of writing it back to disk.
    */
   if (!Private)
   {
      ASSERT(SwapEntry == 0);
	  DPRINT("MiWriteBackPage(%wZ,%08x%08x)\n", &FileObject->FileName, Offset.u.HighPart, Offset.u.LowPart);
      Status = PageOp->Status = MiWriteBackPage(FileObject, &Offset, PAGE_SIZE, Page);
      MmspCompleteAndReleasePageOp(PageOp);
      return(Status);
   }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmSetDirtyAllRmaps(Page);
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
      }
      MmSetSavedSwapEntryPage(Page, SwapEntry);
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
      MmSetDirtyAllRmaps(Page);
      PageOp->Status = STATUS_UNSUCCESSFUL;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiFlushMappedSection
(PVOID BaseAddress,
 PLARGE_INTEGER FileSize)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG_PTR PageAddress;
	PMMSUPPORT AddressSpace = MmGetKernelAddressSpace();
	PMEMORY_AREA MemoryArea;
	PMM_SECTION_SEGMENT Segment;
	ULONG_PTR BeginningAddress, EndingAddress;
	LARGE_INTEGER ViewOffset;
	LARGE_INTEGER FileOffset;
	PFN_TYPE Page;
	PPFN_TYPE Pages;

	MmLockAddressSpace(AddressSpace);
	MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
	if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_SECTION_VIEW) 
	{
		MmUnlockAddressSpace(AddressSpace);
		return STATUS_NOT_MAPPED_DATA;
	}
	BeginningAddress = PAGE_ROUND_DOWN((ULONG_PTR)MemoryArea->StartingAddress);
	EndingAddress = PAGE_ROUND_UP((ULONG_PTR)MemoryArea->EndingAddress);
	Segment = MemoryArea->Data.SectionData.Segment;
	ViewOffset.QuadPart = MemoryArea->Data.SectionData.ViewOffset.QuadPart;

	MmLockSectionSegment(Segment);

	Pages = ExAllocatePool
		(NonPagedPool, 
		 sizeof(PFN_TYPE) * 
		 ((EndingAddress - BeginningAddress) >> PAGE_SHIFT));

	if (!Pages)
	{
		ASSERT(FALSE);
	}

	for (PageAddress = BeginningAddress;
		 PageAddress < EndingAddress;
		 PageAddress += PAGE_SIZE)
	{
		ULONG Entry;
		FileOffset.QuadPart = PageAddress - BeginningAddress + ViewOffset.QuadPart;
		Entry =
			MiGetPageEntrySectionSegment
			(MemoryArea->Data.SectionData.Segment, 
			 &FileOffset);
		Page = PFN_FROM_SSE(Entry);
		if (Entry != 0 && !IS_SWAP_FROM_SSE(Entry) && MmIsDirtyPageRmap(Page) &&
			FileOffset.QuadPart < FileSize->QuadPart)
		{
			MmLockPage(Page);
			Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = Page;
		}
		else
			Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = 0;
	}
	MmUnlockSectionSegment(Segment);
	MmUnlockAddressSpace(AddressSpace);

	for (PageAddress = BeginningAddress;
		 PageAddress < EndingAddress;
		 PageAddress += PAGE_SIZE)
	{
		FileOffset.QuadPart = PageAddress - BeginningAddress + ViewOffset.QuadPart;
		Page = Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT];
		if (Page)
		{
			DPRINT("MiWriteBackPage(%wZ,%08x%08x)\n", &Segment->FileObject->FileName, FileOffset.u.HighPart, FileOffset.u.LowPart);
			Status = MiWriteBackPage(Segment->FileObject, &FileOffset, PAGE_SIZE, Page);
			MmUnlockPage(Page);
			MmSetCleanAllRmaps(Page);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1
					("Writeback from section flush %08x%08x (%x) %x@%x (%08x%08x:%wZ) failed %x\n",
					 FileOffset.u.HighPart, FileOffset.u.LowPart,
					 (ULONG)(FileSize->QuadPart - FileOffset.QuadPart),
					 PageAddress,
					 Page,
					 FileSize->u.HighPart,
					 FileSize->u.LowPart,
					 &Segment->FileObject->FileName,
					 Status);
			}
		}
	}

	ExFreePool(Pages);

	return Status;
}

VOID static
MmAlterViewAttributes(PMMSUPPORT AddressSpace,
                      PVOID BaseAddress,
                      ULONG RegionSize,
                      ULONG OldType,
                      ULONG OldProtect,
                      ULONG NewType,
                      ULONG NewProtect)
{
   PMEMORY_AREA MemoryArea;
   PMM_SECTION_SEGMENT Segment;
   BOOLEAN DoCOW = FALSE;
   ULONG i;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   Segment = MemoryArea->Data.SectionData.Segment;

   if ((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
         (NewProtect == PAGE_READWRITE || NewProtect == PAGE_EXECUTE_READWRITE))
   {
      DoCOW = TRUE;
   }

   if (OldProtect != NewProtect)
   {
      for (i = 0; i < PAGE_ROUND_UP(RegionSize) / PAGE_SIZE; i++)
      {
         PVOID Address = (char*)BaseAddress + (i * PAGE_SIZE);
         ULONG Protect = NewProtect;
		 BOOLEAN Present;

         /*
          * If we doing COW for this segment then check if the page is
          * already private.
          */

		 Present = MmIsPagePresent(Process, Address);
         if (DoCOW && Present)
         {
            LARGE_INTEGER Offset;
            ULONG Entry;
            PFN_TYPE Page;

            Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress
                     + MemoryArea->Data.SectionData.ViewOffset.QuadPart;
            Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
            Page = MmGetPfnForProcess(Process, Address);

            Protect = PAGE_READONLY;
            if (IS_SWAP_FROM_SSE(Entry) || PFN_FROM_SSE(Entry) != Page)
            {
               Protect = NewProtect;
            }
         }

         if (Present)
         {
            MmSetPageProtect(Process, Address, Protect);
         }
      }
   }
}

NTSTATUS
NTAPI
MmProtectSectionView(PMMSUPPORT AddressSpace,
                     PMEMORY_AREA MemoryArea,
                     PVOID BaseAddress,
                     ULONG Length,
                     ULONG Protect,
                     PULONG OldProtect)
{
   PMM_REGION Region;
   NTSTATUS Status;
   ULONG_PTR MaxLength;

   MaxLength = (ULONG_PTR)MemoryArea->EndingAddress - (ULONG_PTR)BaseAddress;
   if (Length > MaxLength)
      Length = MaxLength;

   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         BaseAddress, NULL);
   if ((MemoryArea->Flags & SEC_NO_CHANGE) &&
       Region->Protect != Protect)
   {
	  DPRINT1("STATUS_INVALID_PAGE_PROTECTION\n");
      return STATUS_INVALID_PAGE_PROTECTION;
   }

   *OldProtect = Region->Protect;
   Status = MmAlterRegion(AddressSpace, MemoryArea->StartingAddress,
                          &MemoryArea->Data.SectionData.RegionListHead,
                          BaseAddress, Length, Region->Type, Protect,
                          MmAlterViewAttributes);

   return(Status);
}

NTSTATUS NTAPI
MmQuerySectionView(PMEMORY_AREA MemoryArea,
                   PVOID Address,
                   PMEMORY_BASIC_INFORMATION Info,
                   PULONG ResultLength)
{
   PMM_REGION Region;
   PVOID RegionBaseAddress;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;

   Region = MmFindRegion((PVOID)MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, &RegionBaseAddress);
   if (Region == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Section = MemoryArea->Data.SectionData.Section;
   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      Segment = MemoryArea->Data.SectionData.Segment;
      Info->AllocationBase = (PUCHAR)MemoryArea->StartingAddress - Segment->Image.VirtualAddress;
      Info->Type = MEM_IMAGE;
   }
   else
   {
      Info->AllocationBase = MemoryArea->StartingAddress;
      Info->Type = MEM_MAPPED;
   }
   Info->BaseAddress = RegionBaseAddress;
   Info->AllocationProtect = MemoryArea->Protect;
   Info->RegionSize = Region->Length;
   Info->State = MEM_COMMIT;
   Info->Protect = Region->Protect;

   *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
   return(STATUS_SUCCESS);
}

VOID NTAPI
MmpDeleteSection(PVOID ObjectBody)
{
   PROS_SECTION_OBJECT Section = (PROS_SECTION_OBJECT)ObjectBody;

   DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
   if (Section->AllocationAttributes & SEC_IMAGE)
   {
	   MiDeleteImageSection(Section);
   }
   else
   {
      /*
       * NOTE: Section->Segment can be NULL for short time
       * during the section creating.
       */
      if (Section->Segment == NULL)
         return;

	  (void)InterlockedDecrementUL(&Section->Segment->ReferenceCount);
   }
   if (Section->FileObject != NULL)
   {
      ObDereferenceObject(Section->FileObject);
      Section->FileObject = NULL;
   }
}

VOID NTAPI
MmpCloseSection(IN PEPROCESS Process OPTIONAL,
                IN PVOID Object,
                IN ACCESS_MASK GrantedAccess,
                IN ULONG ProcessHandleCount,
                IN ULONG SystemHandleCount)
{
   DPRINT("MmpCloseSection(OB %x, HC %d)\n",
          Object, ProcessHandleCount);
}

NTSTATUS
INIT_FUNCTION
NTAPI
MmInitSectionImplementation(VOID)
{
   OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
   UNICODE_STRING Name;

   DPRINT("Creating Section Object Type\n");

   /* Initialize the Section object type  */
   RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
   RtlInitUnicodeString(&Name, L"Section");
   ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
   ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(ROS_SECTION_OBJECT);
   ObjectTypeInitializer.PoolType = PagedPool;
   ObjectTypeInitializer.UseDefaultObject = TRUE;
   ObjectTypeInitializer.GenericMapping = MmpSectionMapping;
   ObjectTypeInitializer.DeleteProcedure = MmpDeleteSection;
   ObjectTypeInitializer.CloseProcedure = MmpCloseSection;
   ObjectTypeInitializer.ValidAccessMask = SECTION_ALL_ACCESS;
   ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &MmSectionObjectType);

   MmCreatePhysicalMemorySection();

   return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
MmCreateDataFileSection(PROS_SECTION_OBJECT *SectionObject,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PLARGE_INTEGER UMaximumSize,
                        ULONG SectionPageProtection,
                        ULONG AllocationAttributes,
                        PFILE_OBJECT FileObject)
/*
 * Create a section backed by a data file
 */
{
   PROS_SECTION_OBJECT Section;
   NTSTATUS Status;
   ULARGE_INTEGER MaximumSize;
   PMM_SECTION_SEGMENT Segment;
   ULONG FileAccess;
   IO_STATUS_BLOCK Iosb;
   CC_FILE_SIZES FileSizes;
   FILE_STANDARD_INFORMATION FileInfo;
   KIRQL OldIrql;

   /*
    * Create the section
    */
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
      return(Status);
   }
   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   Section->Segment = NULL;

   /*
    * Check file access required
    */
   if (SectionPageProtection & PAGE_READWRITE ||
         SectionPageProtection & PAGE_EXECUTE_READWRITE)
   {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
   }
   else
   {
      FileAccess = FILE_READ_DATA;
   }

   /*
    * Reference the file handle
    */
   ObReferenceObject(FileObject);

   DPRINT("Getting original file size\n");
   /* A hack: If we're cached, we can overcome deadlocking with the upper
    * layer filesystem call by retriving the object sizes from the cache
    * which is made to keep track.  If I had to guess, they were figuring
    * out a similar problem.
    */
   if (!CcGetFileSizes(FileObject, &FileSizes))
   {
       /*
		* FIXME: This is propably not entirely correct. We can't look into
		* the standard FCB header because it might not be initialized yet
		* (as in case of the EXT2FS driver by Manoj Paul Joseph where the
		* standard file information is filled on first request).
		*/
       Status = IoQueryFileInformation
	   (FileObject,
	    FileStandardInformation,
	    sizeof(FILE_STANDARD_INFORMATION),
	    &FileInfo,
	    &Iosb.Information);

       if (!NT_SUCCESS(Status))
       {
		   ObDereferenceObject(Section);
		   ObDereferenceObject(FileObject);
		   return Status;
       }
	   ASSERT(Status != STATUS_PENDING);

       FileSizes.ValidDataLength = FileInfo.EndOfFile;
       FileSizes.FileSize = FileInfo.EndOfFile;
   }
   DPRINT("Got %08x\n", FileSizes.ValidDataLength.u.LowPart);

   /*
    * FIXME: Revise this once a locking order for file size changes is
    * decided
    */
   if (UMaximumSize != NULL)
   {
	   MaximumSize.QuadPart = UMaximumSize->QuadPart;
   }
   else
   {
	   DPRINT("Got file size %08x%08x\n", FileSizes.FileSize.u.HighPart, FileSizes.FileSize.u.LowPart);
	   MaximumSize.QuadPart = FileSizes.FileSize.QuadPart;
   }

   /* Mapping zero-sized files isn't allowed. */
   if (MaximumSize.QuadPart == 0)
   {
	   ObDereferenceObject(Section);
	   ObDereferenceObject(FileObject);
	   return STATUS_FILE_INVALID;
   }

   Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
				   TAG_MM_SECTION_SEGMENT);
   if (Segment == NULL)
   {
       ObDereferenceObject(Section);
       ObDereferenceObject(FileObject);
       return(STATUS_NO_MEMORY);
   }

   ExInitializeFastMutex(&Segment->Lock);

   Segment->ReferenceCount = 1;
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
         Segment->RawLength = MaximumSize;
         Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
      }
	  MiInitializeSectionPageTable(Segment);
   }
   else
   {
      KeReleaseSpinLock(&FileObject->IrpListLock, OldIrql);
      ExFreePool(Segment);

      DPRINT("Filling out Segment info (previous data section)\n");

      /*
       * If the file is already mapped as a data file then we may need
       * to extend it
       */
      Segment =
         (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
         DataSectionObject;
      Section->Segment = Segment;
      (void)InterlockedIncrementUL(&Segment->ReferenceCount);

      MmLockSectionSegment(Segment);

      if (MaximumSize.QuadPart > Segment->RawLength.QuadPart &&
            !(AllocationAttributes & SEC_RESERVE))
      {
         Segment->RawLength = MaximumSize;
         Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
      }
   }

   MmUnlockSectionSegment(Segment);

   Section->FileObject = FileObject;
   Section->MaximumSize.QuadPart = MaximumSize.QuadPart;

   /* Extend file if section is longer */
   DPRINT("MaximumSize %08x%08x ValidDataLength %08x%08x\n",
		  MaximumSize.u.HighPart, MaximumSize.u.LowPart,
		  FileSizes.ValidDataLength.u.HighPart, FileSizes.ValidDataLength.u.LowPart);
   if (MaximumSize.QuadPart > FileSizes.ValidDataLength.QuadPart)
   {
	   DPRINT("Changing file size to %08x%08x, segment %x\n", MaximumSize.u.HighPart, MaximumSize.u.LowPart, Segment);
	   Status = IoSetInformation(FileObject, FileEndOfFileInformation, sizeof(LARGE_INTEGER), &MaximumSize);
	   DPRINT("Change: Status %x\n", Status);
	   if (!NT_SUCCESS(Status))
	   {
		   DPRINT1("Could not expand section\n");
		   ObDereferenceObject(Section);
		   ObDereferenceObject(FileObject);
		   return Status;
	   }
   }

   //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);

   *SectionObject = Section;
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
NtCreateSection (OUT PHANDLE SectionHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN PLARGE_INTEGER MaximumSize OPTIONAL,
                 IN ULONG SectionPageProtection OPTIONAL,
                 IN ULONG AllocationAttributes,
                 IN HANDLE FileHandle OPTIONAL)
{
   LARGE_INTEGER SafeMaximumSize;
   PVOID SectionObject;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(MaximumSize != NULL && PreviousMode != KernelMode)
   {
     _SEH2_TRY
     {
       /* make a copy on the stack */
       SafeMaximumSize = ProbeForReadLargeInteger(MaximumSize);
       MaximumSize = &SafeMaximumSize;
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = MmCreateSection(&SectionObject,
                            DesiredAccess,
                            ObjectAttributes,
                            MaximumSize,
                            SectionPageProtection,
                            AllocationAttributes,
                            FileHandle,
                            NULL);
   if (NT_SUCCESS(Status))
   {
      Status = ObInsertObject ((PVOID)SectionObject,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               SectionHandle);
   }

   return Status;
}


/**********************************************************************
 * NAME
 *  NtOpenSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *  SectionHandle
 *
 *  DesiredAccess
 *
 *  ObjectAttributes
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS NTAPI
NtOpenSection(PHANDLE   SectionHandle,
              ACCESS_MASK  DesiredAccess,
              POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hSection;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH2_TRY
     {
       ProbeForWriteHandle(SectionHandle);
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObOpenObjectByName(ObjectAttributes,
                               MmSectionObjectType,
                               PreviousMode,
                               NULL,
                               DesiredAccess,
                               NULL,
                               &hSection);

   if(NT_SUCCESS(Status))
   {
     _SEH2_TRY
     {
       *SectionHandle = hSection;
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;
   }

   return(Status);
}

NTSTATUS
NTAPI
MiMapViewOfSegment(PMMSUPPORT AddressSpace,
                   PROS_SECTION_OBJECT Section,
                   PMM_SECTION_SEGMENT Segment,
                   PVOID* BaseAddress,
                   SIZE_T ViewSize,
                   ULONG Protect,
                   PLARGE_INTEGER ViewOffset,
                   ULONG AllocationType)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;

   Status = MmCreateMemoryArea(AddressSpace,
                               (Segment->Flags & MM_PHYSIMEM_SEGMENT) ? 
							   MEMORY_AREA_PHYSICAL_MEMORY_SECTION : 
							   (Segment->Flags & MM_PAGEFILE_SEGMENT) ?
							   MEMORY_AREA_PAGE_FILE_SECTION :
							   (Segment->Flags & MM_IMAGE_SEGMENT) ? 
							   MEMORY_AREA_IMAGE_SECTION :
							   MEMORY_AREA_SECTION_VIEW,
                               BaseAddress,
                               ViewSize,
                               Protect,
                               &MArea,
                               FALSE,
                               AllocationType,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Mapping between 0x%.8X and 0x%.8X failed (%X).\n",
              (*BaseAddress), (char*)(*BaseAddress) + ViewSize, Status);
      return(Status);
   }

   ObReferenceObject((PVOID)Section);

   MArea->Data.SectionData.Segment = Segment;
   MArea->Data.SectionData.Section = Section;
   if (ViewOffset)
	   MArea->Data.SectionData.ViewOffset = *ViewOffset;
   else
	   MArea->Data.SectionData.ViewOffset.QuadPart = 0;
   MArea->Data.SectionData.WriteCopyView = FALSE;
   MmInitializeRegion(&MArea->Data.SectionData.RegionListHead,
                      ViewSize, 0, Protect);

   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME       EXPORTED
 * NtMapViewOfSection
 *
 * DESCRIPTION
 * Maps a view of a section into the virtual address space of a
 * process.
 *
 * ARGUMENTS
 * SectionHandle
 *  Handle of the section.
 *
 * ProcessHandle
 *  Handle of the process.
 *
 * BaseAddress
 *  Desired base address (or NULL) on entry;
 *  Actual base address of the view on exit.
 *
 * ZeroBits
 *  Number of high order address bits that must be zero.
 *
 * CommitSize
 *  Size in bytes of the initially committed section of
 *  the view.
 *
 * SectionOffset
 *  Offset in bytes from the beginning of the section
 *  to the beginning of the view.
 *
 * ViewSize
 *  Desired length of map (or zero to map all) on entry
 *  Actual length mapped on exit.
 *
 * InheritDisposition
 *  Specified how the view is to be shared with
 *  child processes.
 *
 * AllocateType
 *  Type of allocation for the pages.
 *
 * Protect
 *  Protection for the committed region of the view.
 *
 * RETURN VALUE
 *  Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
NtMapViewOfSection(IN HANDLE SectionHandle,
                   IN HANDLE ProcessHandle,
                   IN OUT PVOID* BaseAddress  OPTIONAL,
                   IN ULONG ZeroBits  OPTIONAL,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset  OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType  OPTIONAL,
                   IN ULONG Protect)
{
   PVOID SafeBaseAddress;
   LARGE_INTEGER SafeSectionOffset;
   SIZE_T SafeViewSize;
   PROS_SECTION_OBJECT Section;
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   PMM_AVL_TABLE AddressSpace;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG tmpProtect;

   /*
    * Check the protection
    */
   if (Protect & ~PAGE_FLAGS_VALID_FROM_USER_MODE)
   {
     return STATUS_INVALID_PARAMETER_10;
   }

   tmpProtect = Protect & ~(PAGE_GUARD|PAGE_NOCACHE);
   if (tmpProtect != PAGE_NOACCESS &&
       tmpProtect != PAGE_READONLY &&
       tmpProtect != PAGE_READWRITE &&
       tmpProtect != PAGE_WRITECOPY &&
       tmpProtect != PAGE_EXECUTE &&
       tmpProtect != PAGE_EXECUTE_READ &&
       tmpProtect != PAGE_EXECUTE_READWRITE &&
       tmpProtect != PAGE_EXECUTE_WRITECOPY)
   {
     return STATUS_INVALID_PAGE_PROTECTION;
   }

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     SafeBaseAddress = NULL;
     SafeSectionOffset.QuadPart = 0;
     SafeViewSize = 0;

     _SEH2_TRY
     {
       if(BaseAddress != NULL)
       {
         ProbeForWritePointer(BaseAddress);
         SafeBaseAddress = *BaseAddress;
       }
       if(SectionOffset != NULL)
       {
         ProbeForWriteLargeInteger(SectionOffset);
         SafeSectionOffset = *SectionOffset;
       }
       ProbeForWriteSize_t(ViewSize);
       SafeViewSize = *ViewSize;
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }
   else
   {
     SafeBaseAddress = (BaseAddress != NULL ? *BaseAddress : NULL);
     SafeSectionOffset.QuadPart = (SectionOffset != NULL ? SectionOffset->QuadPart : 0);
     SafeViewSize = (ViewSize != NULL ? *ViewSize : 0);
   }

   SafeSectionOffset.LowPart = PAGE_ROUND_DOWN(SafeSectionOffset.LowPart);

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Process,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   AddressSpace = &Process->VadRoot;

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_MAP_READ,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Section,
                                      NULL);
   if (!(NT_SUCCESS(Status)))
   {
      DPRINT("ObReference failed rc=%x\n",Status);
      ObDereferenceObject(Process);
      return(Status);
   }

   Status = MmMapViewOfSection(Section,
                               (PEPROCESS)Process,
                               (BaseAddress != NULL ? &SafeBaseAddress : NULL),
                               ZeroBits,
                               CommitSize,
                               (SectionOffset != NULL ? &SafeSectionOffset : NULL),
                               (ViewSize != NULL ? &SafeViewSize : NULL),
                               InheritDisposition,
                               AllocationType,
                               Protect);

   /* Check if this is an image for the current process */
   if ((Section->AllocationAttributes & SEC_IMAGE) &&
       (Process == PsGetCurrentProcess()) &&
       (Status != STATUS_IMAGE_NOT_AT_BASE))
   {
        /* Notify the debugger */
       DbgkMapViewOfSection(Section,
                            SafeBaseAddress,
                            SafeSectionOffset.LowPart,
                            SafeViewSize);
   }

   ObDereferenceObject(Section);
   ObDereferenceObject(Process);

   if(NT_SUCCESS(Status))
   {
     /* copy parameters back to the caller */
     _SEH2_TRY
     {
       if(BaseAddress != NULL)
       {
         *BaseAddress = SafeBaseAddress;
       }
       if(SectionOffset != NULL)
       {
         *SectionOffset = SafeSectionOffset;
       }
       if(ViewSize != NULL)
       {
         *ViewSize = SafeViewSize;
       }
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;
   }

   return(Status);
}

VOID
NTAPI
MiFreeDataSectionSegment(PFILE_OBJECT FileObject)
{
	PMM_SECTION_SEGMENT Segment;
	
	Segment = (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
		DataSectionObject;
	
	if (Segment->ReferenceCount != 0)
	{
		DPRINT1("Data segment still referenced\n");
		ASSERT(FALSE);
	}
	MiFreePageTablesSectionSegment(Segment);
	ExFreePool(Segment);

	FileObject->SectionObjectPointer->DataSectionObject = NULL;
}

VOID static
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                  PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   ULONG Entry;
   PFILE_OBJECT FileObject;
   LARGE_INTEGER Offset;
   SWAPENTRY SavedSwapEntry;
   PMM_PAGEOP PageOp;
   NTSTATUS Status;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMMSUPPORT AddressSpace;
   PEPROCESS Process;

   AddressSpace = (PMMSUPPORT)Context;
   Process = MmGetAddressSpaceOwner(AddressSpace);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset.QuadPart = ((ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress) +
            MemoryArea->Data.SectionData.ViewOffset.QuadPart;

   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;
   FileObject = Section->FileObject;

   PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL, Segment, Offset.QuadPart);

   while (PageOp)
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);

      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         ASSERT(FALSE);
      }

      MmLockAddressSpace(AddressSpace);
      MmLockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp);
      PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL, Segment, Offset.QuadPart);
   }

   Entry = MiGetPageEntrySectionSegment(Segment, &Offset);

   /*
    * For a dirty, datafile, non-private page mark it as dirty in the
    * cache manager.
    */
   if (Page == PFN_FROM_SSE(Entry) && Dirty)
   {
	   DPRINT("Writing back dirty section page %08x%08x\n", Offset.u.HighPart, Offset.u.LowPart);
	   Status = MiWriteBackPage(FileObject, &Offset, PAGE_SIZE, Page);
   }

   if (SwapEntry != 0)
   {
      MmFreeSwapPage(SwapEntry);
   }
   else if (Page != 0)
   {
      if (IS_SWAP_FROM_SSE(Entry) ||
          Page != PFN_FROM_SSE(Entry))
      {
         /*
          * Just dereference private pages
          */
         SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
         if (SavedSwapEntry != 0)
         {
            MmFreeSwapPage(SavedSwapEntry);
            MmSetSavedSwapEntryPage(Page, 0);
         }
         MmDeleteRmap(Page, Process, Address);
         MmReleasePageMemoryConsumer(MC_USER, Page);
      }
      else
      {
         MmDeleteRmap(Page, Process, Address);
         MmUnsharePageEntrySectionSegment(Section, Segment, &Offset, Dirty, FALSE);
      }
   }
}

NTSTATUS
NTAPI
MmUnmapViewOfSegment(PMMSUPPORT AddressSpace,
                     PVOID BaseAddress)
{
   PMEMORY_AREA MemoryArea;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   PLIST_ENTRY RegionListHead;

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   if (MemoryArea == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   MemoryArea->DeleteInProgress = TRUE;
   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;

   MmLockSectionSegment(Segment);

   RegionListHead = &MemoryArea->Data.SectionData.RegionListHead;
   while (!IsListEmpty(RegionListHead))
   {
      CurrentEntry = RemoveHeadList(RegionListHead);
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
      ExFreePool(CurrentRegion);
   }

   if (MemoryArea->Type == MEMORY_AREA_PHYSICAL_MEMORY_SECTION)
   {
	   MmUnmapPhysicalMemorySegment(AddressSpace, MemoryArea, Section, Segment);
   }
   else if (MemoryArea->Type == MEMORY_AREA_PAGE_FILE_SECTION)
   {
	   MmUnmapPageFileSegment(AddressSpace, MemoryArea, Section, Segment);
   }
   else
   {
	   MmFreeMemoryArea(AddressSpace, MemoryArea, MmFreeSectionPage, AddressSpace);
   }
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiAwaitPageOps(PMMSUPPORT AddressSpace, PMEMORY_AREA MemoryArea, PVOID BaseAddress)
{
	NTSTATUS Status;
	PMM_PAGEOP PageOp;
	ULONG Offset;

	while (MemoryArea->PageOpCount)
	{
		Offset = PAGE_ROUND_UP((ULONG_PTR)MemoryArea->EndingAddress - (ULONG_PTR)MemoryArea->StartingAddress);
		
		while (Offset)
		{
			Offset -= PAGE_SIZE;
			PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL,
									  MemoryArea->Data.SectionData.Segment,
									  Offset);
			if (PageOp)
			{
				MmUnlockAddressSpace(AddressSpace);
				Status = MmspWaitForPageOpCompletionEvent(PageOp);
				if (Status != STATUS_SUCCESS)
				{
					DPRINT1("Failed to wait for page op, status = %x\n", Status);
					ASSERT(FALSE);
				}
				MmLockAddressSpace(AddressSpace);
				MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
														 BaseAddress);
				if (MemoryArea == NULL ||
					MemoryArea->Type != MEMORY_AREA_SECTION_VIEW)
				{
					MmUnlockAddressSpace(AddressSpace);
					DPRINT("STATUS_NOT_MAPPED_VIEW\n");
					return STATUS_NOT_MAPPED_VIEW;
				}
				break;
			}
		}
	}
	return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
MmUnmapViewOfSection(PEPROCESS Process,
                     PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PMMSUPPORT AddressSpace;

   DPRINT("Opening memory area Process %x BaseAddress %x\n",
          Process, BaseAddress);

   ASSERT(Process);

   AddressSpace = &Process->Vm;

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);

   if (MemoryArea == NULL ||
       MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      DPRINT("STATUS_NOT_MAPPED_VIEW\n");
      return STATUS_NOT_MAPPED_VIEW;
   }

   if (MemoryArea->Type == MEMORY_AREA_IMAGE_SECTION)
   {
	   MemoryArea->DeleteInProgress = TRUE;
	   Status = MiAwaitPageOps(AddressSpace, MemoryArea, BaseAddress);
	   if (NT_SUCCESS(Status))
		   Status = MiUnmapImageSection(AddressSpace, MemoryArea, BaseAddress);
   }
   else if (MemoryArea->Type == MEMORY_AREA_PHYSICAL_MEMORY_SECTION ||
			MemoryArea->Type == MEMORY_AREA_PAGE_FILE_SECTION ||
			MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
	   MemoryArea->DeleteInProgress = TRUE;
	   Status = MiAwaitPageOps(AddressSpace, MemoryArea, BaseAddress);
	   if (NT_SUCCESS(Status))
		   Status = MmUnmapViewOfSegment(AddressSpace, BaseAddress);
   }
   else
   {
	   Status = STATUS_NOT_MAPPED_VIEW;
   }

   MmUnlockAddressSpace(AddressSpace);
   return Status;
}

/**********************************************************************
 * NAME       EXPORTED
 * NtUnmapViewOfSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * ProcessHandle
 *
 * BaseAddress
 *
 * RETURN VALUE
 * Status.
 *
 * REVISIONS
 */
NTSTATUS NTAPI
NtUnmapViewOfSection (HANDLE ProcessHandle,
                      PVOID BaseAddress)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtUnmapViewOfSection(ProcessHandle %x, BaseAddress %x)\n",
          ProcessHandle, BaseAddress);

   PreviousMode = ExGetPreviousMode();

   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Process,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ObReferenceObjectByHandle failed (Status %x)\n", Status);
      return(Status);
   }

   Status = MmUnmapViewOfSection(Process, BaseAddress);

   ObDereferenceObject(Process);

   return Status;
}


/**
 * Queries the information of a section object.
 *
 * @param SectionHandle
 *        Handle to the section object. It must be opened with SECTION_QUERY
 *        access.
 * @param SectionInformationClass
 *        Index to a certain information structure. Can be either
 *        SectionBasicInformation or SectionImageInformation. The latter
 *        is valid only for sections that were created with the SEC_IMAGE
 *        flag.
 * @param SectionInformation
 *        Caller supplies storage for resulting information.
 * @param Length
 *        Size of the supplied storage.
 * @param ResultLength
 *        Data written.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
NtQuerySection(IN HANDLE SectionHandle,
               IN SECTION_INFORMATION_CLASS SectionInformationClass,
               OUT PVOID SectionInformation,
               IN ULONG SectionInformationLength,
               OUT PULONG ResultLength  OPTIONAL)
{
   PROS_SECTION_OBJECT Section;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   Status = DefaultQueryInfoBufferCheck(SectionInformationClass,
                                        ExSectionInfoClass,
                                        sizeof(ExSectionInfoClass) / sizeof(ExSectionInfoClass[0]),
                                        SectionInformation,
                                        SectionInformationLength,
                                        ResultLength,
                                        PreviousMode);

   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQuerySection() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_QUERY,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Section,
                                      NULL);
   if (NT_SUCCESS(Status))
   {
      switch (SectionInformationClass)
      {
         case SectionBasicInformation:
         {
            PSECTION_BASIC_INFORMATION Sbi = (PSECTION_BASIC_INFORMATION)SectionInformation;

            _SEH2_TRY
            {
               Sbi->Attributes = Section->AllocationAttributes;
               if (Section->AllocationAttributes & SEC_IMAGE)
               {
                  Sbi->BaseAddress = 0;
                  Sbi->Size.QuadPart = 0;
               }
               else
               {
                  Sbi->BaseAddress = (PVOID)Section->Segment->Image.VirtualAddress;
                  RtlCopyMemory(&Sbi->Size, &Section->Segment->Length, sizeof(ULARGE_INTEGER));
               }

               if (ResultLength != NULL)
               {
                  *ResultLength = sizeof(SECTION_BASIC_INFORMATION);
               }
               Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
               Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
         }

         case SectionImageInformation:
         {
            PSECTION_IMAGE_INFORMATION Sii = (PSECTION_IMAGE_INFORMATION)SectionInformation;

            _SEH2_TRY
            {
               memset(Sii, 0, sizeof(SECTION_IMAGE_INFORMATION));
               if (Section->AllocationAttributes & SEC_IMAGE)
               {
                  PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
                  ImageSectionObject = Section->ImageSection;

                  Sii->TransferAddress = (PVOID)ImageSectionObject->EntryPoint;
                  Sii->MaximumStackSize = ImageSectionObject->StackReserve;
                  Sii->CommittedStackSize = ImageSectionObject->StackCommit;
                  Sii->SubSystemType = ImageSectionObject->Subsystem;
                  Sii->SubSystemMinorVersion = ImageSectionObject->MinorSubsystemVersion;
                  Sii->SubSystemMajorVersion = ImageSectionObject->MajorSubsystemVersion;
                  Sii->ImageCharacteristics = ImageSectionObject->ImageCharacteristics;
                  Sii->Machine = ImageSectionObject->Machine;
                  Sii->ImageContainsCode = ImageSectionObject->Executable;
               }

               if (ResultLength != NULL)
               {
                  *ResultLength = sizeof(SECTION_IMAGE_INFORMATION);
               }
               Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
               Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
         }
      }

      ObDereferenceObject(Section);
   }

   return(Status);
}

NTSTATUS
NTAPI
MmExtendSection
(PROS_SECTION_OBJECT Section,
 PLARGE_INTEGER NewSize,
 BOOLEAN ExtendFile)
{
	PMM_SECTION_SEGMENT Segment = Section->Segment;
	LARGE_INTEGER OldSize;
	DPRINT("Extend Segment %x\n", Segment);

	MmLockSectionSegment(Segment);
	OldSize.QuadPart = Segment->RawLength.QuadPart;
	MmUnlockSectionSegment(Segment);

	DPRINT("OldSize %08x%08x NewSize %08x%08x\n",
		   OldSize.u.HighPart, OldSize.u.LowPart,
		   NewSize->u.HighPart, NewSize->u.LowPart);

	if (ExtendFile && OldSize.QuadPart < NewSize->QuadPart)
	{
		NTSTATUS Status;
		Status = IoSetInformation(Segment->FileObject, FileEndOfFileInformation, sizeof(LARGE_INTEGER), NewSize);
		if (!NT_SUCCESS(Status)) return Status;
	}

	MmLockSectionSegment(Segment);
	Segment->RawLength.QuadPart = NewSize->QuadPart;
	Segment->Length.QuadPart = MAX(Segment->Length.QuadPart, PAGE_ROUND_UP(Segment->RawLength.LowPart));
	MmUnlockSectionSegment(Segment);
	Section->MaximumSize = *NewSize;
	return STATUS_SUCCESS;
}

/**
 * Extends size of file backed section.
 *
 * @param SectionHandle
 *        Handle to the section object. It must be opened with
 *        SECTION_EXTEND_SIZE access.
 * @param NewMaximumSize
 *        New maximum size of the section in bytes.
 *
 * @return Status.
 *
 * @todo Move the actual code to internal function MmExtendSection.
 * @unimplemented
 */
NTSTATUS NTAPI
NtExtendSection(IN HANDLE SectionHandle,
                IN PLARGE_INTEGER NewMaximumSize)
{
   LARGE_INTEGER SafeNewMaximumSize;
   PROS_SECTION_OBJECT Section;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH2_TRY
     {
       /* make a copy on the stack */
       SafeNewMaximumSize = ProbeForReadLargeInteger(NewMaximumSize);
       NewMaximumSize = &SafeNewMaximumSize;
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
       Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_EXTEND_SIZE,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)&Section,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   if (!(Section->AllocationAttributes & SEC_FILE))
   {
      ObfDereferenceObject(Section);
      return STATUS_INVALID_PARAMETER;
   }

   Status = MmExtendSection(Section, NewMaximumSize, TRUE);
   ObDereferenceObject(Section);

   return Status;
}


/**********************************************************************
 * NAME       INTERNAL
 *  MmAllocateSection@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *  Length
 *
 * RETURN VALUE
 *
 * NOTE
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 */
PVOID NTAPI
MmAllocateSection (IN ULONG Length, PVOID BaseAddress)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   PMMSUPPORT AddressSpace;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   DPRINT("MmAllocateSection(Length %x)\n",Length);

   BoundaryAddressMultiple.QuadPart = 0;

   AddressSpace = MmGetKernelAddressSpace();
   Result = BaseAddress;
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateMemoryArea (AddressSpace,
                                MEMORY_AREA_SYSTEM,
                                &Result,
                                Length,
                                0,
                                &marea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(AddressSpace);

   if (!NT_SUCCESS(Status))
   {
      return (NULL);
   }
   DPRINT("Result %p\n",Result);

   /* Create a virtual mapping for this memory area */
   MmMapMemoryArea(Result, Length, MC_NPPOOL, PAGE_READWRITE);

   return ((PVOID)Result);
}


/**********************************************************************
 * NAME       EXPORTED
 * MmMapViewOfSection
 *
 * DESCRIPTION
 * Maps a view of a section into the virtual address space of a
 * process.
 *
 * ARGUMENTS
 * Section
 *  Pointer to the section object.
 *
 * ProcessHandle
 *  Pointer to the process.
 *
 * BaseAddress
 *  Desired base address (or NULL) on entry;
 *  Actual base address of the view on exit.
 *
 * ZeroBits
 *  Number of high order address bits that must be zero.
 *
 * CommitSize
 *  Size in bytes of the initially committed section of
 *  the view.
 *
 * SectionOffset
 *  Offset in bytes from the beginning of the section
 *  to the beginning of the view.
 *
 * ViewSize
 *  Desired length of map (or zero to map all) on entry
 *  Actual length mapped on exit.
 *
 * InheritDisposition
 *  Specified how the view is to be shared with
 *  child processes.
 *
 * AllocationType
 *  Type of allocation for the pages.
 *
 * Protect
 *  Protection for the committed region of the view.
 *
 * RETURN VALUE
 * Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
MmMapViewOfSection(IN PVOID SectionObject,
                   IN PEPROCESS Process,
                   IN OUT PVOID *BaseAddress,
                   IN ULONG ZeroBits,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG Protect)
{
   PROS_SECTION_OBJECT Section;
   PMMSUPPORT AddressSpace;
   LARGE_INTEGER ViewOffset;
   NTSTATUS Status = STATUS_SUCCESS;

   ASSERT(Process);

   if (Protect != PAGE_READONLY &&
       Protect != PAGE_READWRITE &&
       Protect != PAGE_WRITECOPY &&
       Protect != PAGE_EXECUTE &&
       Protect != PAGE_EXECUTE_READ &&
       Protect != PAGE_EXECUTE_READWRITE &&
       Protect != PAGE_EXECUTE_WRITECOPY)
   {
      return STATUS_INVALID_PAGE_PROTECTION;
   }


   Section = (PROS_SECTION_OBJECT)SectionObject;
   AddressSpace = &Process->Vm;

   AllocationType |= (Section->AllocationAttributes & SEC_NO_CHANGE);

   MmLockAddressSpace(AddressSpace);

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
	   Status = MiMapImageFileSection(AddressSpace, Section, BaseAddress);
   }
   else
   {
      /* check for write access */
      if ((Protect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)) &&
          !(Section->SectionPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)))
      {
         return STATUS_SECTION_PROTECTION;
      }
      /* check for read access */
      if ((Protect & (PAGE_READONLY|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY)) &&
          !(Section->SectionPageProtection & (PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
      {
         return STATUS_SECTION_PROTECTION;
      }
      /* check for execute access */
      if ((Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
          !(Section->SectionPageProtection & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
      {
         return STATUS_SECTION_PROTECTION;
      }

      if (ViewSize == NULL)
      {
         /* Following this pointer would lead to us to the dark side */
         /* What to do? Bugcheck? Return status? Do the mambo? */
         KeBugCheck(MEMORY_MANAGEMENT);
      }

      if (SectionOffset == NULL)
      {
         ViewOffset.QuadPart = 0;
      }
      else
      {
         ViewOffset = *SectionOffset;
      }

      if ((ViewOffset.QuadPart % PAGE_SIZE) != 0)
      {
         MmUnlockAddressSpace(AddressSpace);
         return(STATUS_MAPPED_ALIGNMENT);
      }

      if ((*ViewSize) == 0)
      {
         (*ViewSize) = Section->MaximumSize.QuadPart - ViewOffset.QuadPart;
      }
      else if (((*ViewSize)+ViewOffset.QuadPart) > Section->MaximumSize.QuadPart)
      {
         (*ViewSize) = Section->MaximumSize.QuadPart - ViewOffset.QuadPart;
      }

      MmLockSectionSegment(Section->Segment);
      Status = MiMapViewOfSegment(AddressSpace,
                                  Section,
                                  Section->Segment,
                                  BaseAddress,
                                  *ViewSize,
                                  Protect,
                                  &ViewOffset,
                                  AllocationType & (MEM_TOP_DOWN|SEC_NO_CHANGE));
      MmUnlockSectionSegment(Section->Segment);
      if (!NT_SUCCESS(Status))
      {
         MmUnlockAddressSpace(AddressSpace);
         return(Status);
      }
   }

   MmUnlockAddressSpace(AddressSpace);

   return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN PLARGE_INTEGER   NewFileSize)
{
   CC_FILE_SIZES FileSizes;

   /* Check whether an ImageSectionObject exists */
   if (SectionObjectPointer->ImageSectionObject != NULL)
   {
      DPRINT1("ERROR: File can't be truncated because it has an image section\n");
      return FALSE;
   }

   if (SectionObjectPointer->DataSectionObject != NULL)
   {
      PMM_SECTION_SEGMENT Segment;

      Segment = (PMM_SECTION_SEGMENT)SectionObjectPointer->
                DataSectionObject;

	  CcpLock();
      if (SectionObjectPointer->SharedCacheMap && (Segment->ReferenceCount > CcpCountCacheSections((PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap)))
      {
		  CcpUnlock();
          /* Check size of file */
          if (SectionObjectPointer->SharedCacheMap)
          {
			 if (!CcGetFileSizes(Segment->FileObject, &FileSizes))
			 {
				return FALSE;
			 }

             if (NewFileSize->QuadPart <= FileSizes.FileSize.QuadPart)
             {
                return FALSE;
             }
          }
      }
	  else
		  CcpUnlock();

	  // Otherwise, all sections are cache sections, and don't count
   }

   DPRINT1("FIXME: didn't check for outstanding write probes\n");

   return TRUE;
}


/*
 * @unimplemented
 */
BOOLEAN NTAPI
MmDisableModifiedWriteOfSection (ULONG Unknown0)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/*
 * @implemented
 */
BOOLEAN NTAPI
MmFlushImageSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN MMFLUSH_TYPE   FlushType)
{
    return CcFlushImageSection(SectionObjectPointer, FlushType);
}

/*
 * @unimplemented
 */
BOOLEAN NTAPI
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN                  DelayClose)
{
   UNIMPLEMENTED;
   return (FALSE);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
MmMapViewInSystemSpace (IN PVOID SectionObject,
                        OUT PVOID * MappedBase,
                        IN OUT PULONG ViewSize)
{
   PROS_SECTION_OBJECT Section;
   PMMSUPPORT AddressSpace;
   NTSTATUS Status;

   DPRINT("MmMapViewInSystemSpace() called\n");

   Section = (PROS_SECTION_OBJECT)SectionObject;
   AddressSpace = MmGetKernelAddressSpace();

   MmLockAddressSpace(AddressSpace);
   MmLockSectionSegment(Section->Segment);

   if ((*ViewSize) == 0)
   {
	   (*ViewSize) = Section->MaximumSize.u.LowPart;
   }
   else if ((*ViewSize) > Section->MaximumSize.u.LowPart)
   {
	   (*ViewSize) = Section->MaximumSize.u.LowPart;
   }

   Status = MiMapViewOfSegment(AddressSpace,
                               Section,
                               Section->Segment,
                               MappedBase,
                               *ViewSize,
                               PAGE_READWRITE,
                               0,
                               0);

   MmUnlockSectionSegment(Section->Segment);
   MmUnlockAddressSpace(AddressSpace);

   return Status;
}

NTSTATUS NTAPI
MmMapViewInSystemSpaceAtOffset
(IN PVOID SectionObject,
 OUT PVOID *MappedBase,
 PLARGE_INTEGER FileOffset,
 IN OUT PULONG ViewSize)
{
    PROS_SECTION_OBJECT Section;
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;
    
    DPRINT("MmMapViewInSystemSpaceAtOffset() called offset %08x%08x\n", FileOffset->HighPart, FileOffset->LowPart);
    
    Section = (PROS_SECTION_OBJECT)SectionObject;
    AddressSpace = MmGetKernelAddressSpace();
    
    MmLockAddressSpace(AddressSpace);
    
    MmLockSectionSegment(Section->Segment);
    
    Status = MiMapViewOfSegment(AddressSpace,
				Section,
				Section->Segment,
				MappedBase,
				*ViewSize,
				Section->SectionPageProtection,
				FileOffset,
				0);
    
    MmUnlockSectionSegment(Section->Segment);
    MmUnlockAddressSpace(AddressSpace);
    
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
MmUnmapViewInSystemSpace (IN PVOID MappedBase)
{
   PMMSUPPORT AddressSpace;
   NTSTATUS Status;

   DPRINT("MmUnmapViewInSystemSpace() called\n");

   AddressSpace = MmGetKernelAddressSpace();

   Status = MmUnmapViewOfSegment(AddressSpace, MappedBase);

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 * NAME       EXPORTED
 *  MmCreateSection@
 *
 * DESCRIPTION
 *  Creates a section object.
 *
 * ARGUMENTS
 * SectionObject (OUT)
 *  Caller supplied storage for the resulting pointer
 *  to a SECTION_OBJECT instance;
 *
 * DesiredAccess
 *  Specifies the desired access to the section can be a
 *  combination of:
 *   STANDARD_RIGHTS_REQUIRED |
 *   SECTION_QUERY   |
 *   SECTION_MAP_WRITE  |
 *   SECTION_MAP_READ  |
 *   SECTION_MAP_EXECUTE
 *
 * ObjectAttributes [OPTIONAL]
 *  Initialized attributes for the object can be used
 *  to create a named section;
 *
 * MaximumSize
 *  Maximizes the size of the memory section. Must be
 *  non-NULL for a page-file backed section.
 *  If value specified for a mapped file and the file is
 *  not large enough, file will be extended.
 *
 * SectionPageProtection
 *  Can be a combination of:
 *   PAGE_READONLY |
 *   PAGE_READWRITE |
 *   PAGE_WRITEONLY |
 *   PAGE_WRITECOPY
 *
 * AllocationAttributes
 *  Can be a combination of:
 *   SEC_IMAGE |
 *   SEC_RESERVE
 *
 * FileHandle
 *  Handle to a file to create a section mapped to a file
 *  instead of a memory backed section;
 *
 * File
 *  Unknown.
 *
 * RETURN VALUE
 *  Status.
 *
 * @implemented
 */
NTSTATUS NTAPI
MmCreateSection (OUT PVOID  * Section,
                 IN ACCESS_MASK  DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes     OPTIONAL,
                 IN PLARGE_INTEGER  MaximumSize,
                 IN ULONG   SectionPageProtection,
                 IN ULONG   AllocationAttributes,
                 IN HANDLE FileHandle OPTIONAL,
                 IN PFILE_OBJECT FileObject OPTIONAL)
{
    PFILE_OBJECT OriginalFileObject = FileObject;
    ULONG Protection;
    NTSTATUS Status;
    ULONG FileAccess = 0;
    BOOLEAN ReferencedFile = FALSE;
    PROS_SECTION_OBJECT *SectionObject = (PROS_SECTION_OBJECT *)Section;
    
    /*
     * Check the protection
     */
    Protection = SectionPageProtection & ~(PAGE_GUARD|PAGE_NOCACHE);
    if (Protection != PAGE_NOACCESS &&
		Protection != PAGE_READONLY &&
		Protection != PAGE_READWRITE &&
		Protection != PAGE_WRITECOPY &&
		Protection != PAGE_EXECUTE &&
		Protection != PAGE_EXECUTE_READ &&
		Protection != PAGE_EXECUTE_READWRITE &&
		Protection != PAGE_EXECUTE_WRITECOPY)
    {
		return STATUS_INVALID_PAGE_PROTECTION;
    }
    
    if (((DesiredAccess == SECTION_ALL_ACCESS ||
		  (DesiredAccess & SECTION_MAP_WRITE)) &&
		 (Protection == PAGE_READWRITE ||
		  Protection == PAGE_EXECUTE_READWRITE)) &&
		!(AllocationAttributes & SEC_IMAGE))
    {
		DPRINT("Creating a section with WRITE access\n");
		FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    }
    else 
    {
		DPRINT("Creating a section with READ access\n");
		FileAccess = FILE_READ_DATA;
    }
    
    /*
     * Reference the file handle
     */
    if (!FileObject && FileHandle)
    {
		Status = ObReferenceObjectByHandle
			(FileHandle,
			 FileAccess,
			 IoFileObjectType,
			 ExGetPreviousMode(),
			 (PVOID*)(PVOID)&FileObject,
			 NULL);
		if (!NT_SUCCESS(Status)) 
		{
			DPRINT("Failed: %x\n", Status);
			return Status;
		}
		else
			ReferencedFile = TRUE;
    }
    
    if (AllocationAttributes & SEC_IMAGE)
    {
		DPRINT("Creating an image section\n");
		Status = MmCreateImageSection
			(SectionObject,
			 DesiredAccess,
			 ObjectAttributes,
			 MaximumSize,
			 SectionPageProtection,
			 AllocationAttributes,
			 FileObject);
		if (!NT_SUCCESS(Status))
			DPRINT("Failed: %x\n", Status);
    }
    else if (FileHandle != NULL || OriginalFileObject != NULL)
    {
		DPRINT("Creating a data section\n");
		Status = MmCreateDataFileSection
			(SectionObject,
			 DesiredAccess,
			 ObjectAttributes,
			 MaximumSize,
			 SectionPageProtection,
			 AllocationAttributes,
			 FileObject);
		if (!NT_SUCCESS(Status))
			DPRINT("Failed: %x\n", Status);
    }
    else
    {
		DPRINT("Creating a page file section\n");
		Status = MmCreatePageFileSection
			(SectionObject,
			 DesiredAccess,
			 ObjectAttributes,
			 MaximumSize,
			 SectionPageProtection,
			 AllocationAttributes);
		if (!NT_SUCCESS(Status))
			DPRINT("Failed: %x\n", Status);
    }

    return(Status);   
}

NTSTATUS
NTAPI
NtAreMappedFilesTheSame(IN PVOID File1MappedAsAnImage,
                        IN PVOID File2MappedAsFile)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
