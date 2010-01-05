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
#define NDEBUG
#include <debug.h>

extern KEVENT MmWaitPageEvent;

NTSTATUS
NTAPI
MmNotPresentFaultPageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required)
{
	NTSTATUS Status;
	PVOID PAddress;
	ULONG Consumer;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	LARGE_INTEGER FileOffset, TotalOffset;
	ULONG Entry;
	ULONG Attributes;
	PMM_REGION Region;
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
		DPRINT("Done\n");
		return(STATUS_SUCCESS);
	}

	PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
	TotalOffset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;

	Segment = MemoryArea->Data.SectionData.Segment;
	Section = MemoryArea->Data.SectionData.Section;
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);

	if (Segment->Flags & MM_IMAGE_SEGMENT)
	{
		FileOffset.QuadPart = TotalOffset.QuadPart + Segment->Image.FileOffset;
		DPRINT("SEG Flags %x File Offset %x\n", 
			   Segment->Image.Characteristics, FileOffset.LowPart);
	}
	else
	{
		TotalOffset.QuadPart += MemoryArea->Data.SectionData.ViewOffset.QuadPart;
		FileOffset = TotalOffset;
	}

	Consumer = (Segment->Flags & MM_DATAFILE_SEGMENT) ? MC_CACHE : MC_USER;

	if (Segment->FileObject)
	{
		DPRINT("FileName %wZ\n", &Segment->FileObject->FileName);
	}

	DPRINT("Total Offset %08x%08x\n", TotalOffset.HighPart, TotalOffset.LowPart);

	/*
	 * Lock the segment
	 */
	MmLockSectionSegment(Segment);

	/*
	 * Get the entry corresponding to the offset within the section
	 */
	Entry = MiGetPageEntrySectionSegment(Segment, &TotalOffset);

	if (MM_IS_WAIT_PTE(Entry) && !Required->Page[0] && !Required->Context)
	{
		DPRINT("Wait PTE\n");
		MmUnlockSectionSegment(Segment);
		return STATUS_SUCCESS + 1; // Wait
	}	

	/*
	 * Check if this page needs to be mapped COW
	 */
	if ((Segment->WriteCopy || 
		 MemoryArea->Data.SectionData.WriteCopyView) &&
		(Region->Protect == PAGE_READWRITE ||
		 Region->Protect == PAGE_EXECUTE_READWRITE))
	{
		DPRINT("Cow\n");
		Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
	}
	else
	{
		Attributes = Region->Protect;
		DPRINT("Normal %x\n", Attributes);
	}

	if (Required->State && Required->Page[0])
	{
		DPRINT("Have file and page, set page in section @ %x\n", TotalOffset.LowPart);
		Status = MiSetPageEntrySectionSegment
			(Segment, &TotalOffset, Entry = MAKE_PFN_SSE(Required->Page[0]));
		if (NT_SUCCESS(Status))
		{
			MmReferencePage(Required->Page[0]);
			Status = MmCreateVirtualMapping(Process, Address, Attributes, Required->Page, 1);
			if (NT_SUCCESS(Status))
			{
				MmInsertRmap(Required->Page[0], Process, Address);
			}
			else
			{
				MmDereferencePage(Required->Page[0]);
			}
		}
		MmUnlockSectionSegment(Segment);
		DPRINT("XXX Set Event %x\n", Status);
		KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
		DPRINT1("Status %x\n", Status);
		return Status;
	}
	else if (MmIsPageSwapEntry(Process, Address))
	{
		SWAPENTRY SwapEntry;
		MmGetPageFileMapping(Process, Address, &SwapEntry);
		if (SwapEntry == MM_WAIT_ENTRY)
		{
			DPRINT1("Wait for page entry in section\n");
			return STATUS_SUCCESS + 1;
		}
		else
		{
			DPRINT("Swap in\n");
			MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
			Required->State = 7;
			Required->Consumer = Consumer;
			Required->SwapEntry = SwapEntry;
			Required->DoAcquisition = MiSwapInPage;
			MmUnlockSectionSegment(Segment);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
	else if (IS_SWAP_FROM_SSE(Entry))
	{
		SWAPENTRY SwapEntry = SWAPENTRY_FROM_SSE(Entry);
		if (SwapEntry == MM_WAIT_ENTRY)
		{
			DPRINT1("Wait for page entry in section\n");
			return STATUS_SUCCESS + 1;
		}
		else
		{
			DPRINT("Swap in\n");
			MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
			MiSetPageEntrySectionSegment(Segment, &TotalOffset, MAKE_SWAP_SSE(MM_WAIT_ENTRY));
			Required->State = 7;
			Required->Consumer = Consumer;
			Required->SwapEntry = SwapEntry;
			Required->DoAcquisition = MiSwapInPage;
			MmUnlockSectionSegment(Segment);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
	else if (Entry && !IS_SWAP_FROM_SSE(Entry))
	{
		PFN_TYPE Page = PFN_FROM_SSE(Entry);
		DPRINT("Page %x\n", Page);
		MmReferencePage(Page);
		Status = MmCreateVirtualMapping(Process, Address, Attributes, &Page, 1);
		if (NT_SUCCESS(Status))
		{
			MmInsertRmap(Page, Process, Address);
			Required->Page[0] = 0;
		}
		DPRINT("XXX Set Event %x\n", Status);
		KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
		MmUnlockSectionSegment(Segment);
		DPRINT1("Status %x\n", Status);
		return Status;
	}
	else
	{
		DPRINT("Get page into section\n");
		/*
		 * If the entry is zero (and it can't change because we have
		 * locked the segment) then we need to load the page.
		 */
		if (Section->FileObject &&
			(!(Segment->Flags & MM_IMAGE_SEGMENT) ||
			 (Segment->Image.Characteristics &
			  (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA))))
		{
			DPRINT("Read from file %08x\n", FileOffset.LowPart);
			Required->State = 1;
			Required->Context = Section->FileObject;
			Required->Consumer = Consumer;
			Required->FileOffset = FileOffset;
			Required->DoAcquisition = MiReadFilePage;
			MmUnlockSectionSegment(Segment);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
		else
		{
			DPRINT("Get Page\n");
			Required->State = 1;
			Required->Amount = 1;
			Required->Consumer = Consumer;
			Required->DoAcquisition = MiGetOnePage;
			MmUnlockSectionSegment(Segment);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
}

NTSTATUS
NTAPI
MmNotPresentFaultImageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required)
{
	LARGE_INTEGER Offset;
	PFN_TYPE Page;
	NTSTATUS Status = STATUS_SUCCESS;
	PVOID PAddress;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	ULONG Entry;
	ULONG Attributes;
	PMM_REGION Region;
	BOOLEAN HasSwapEntry;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
	PHYSICAL_ADDRESS BoundaryAddressMultiple;

	BoundaryAddressMultiple.QuadPart = 0;

	DPRINT1("Not Present: %p %p (%p-%p)\n", AddressSpace, Address, MemoryArea->StartingAddress, MemoryArea->EndingAddress);
    
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
	Offset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;

	Segment = MemoryArea->Data.SectionData.Segment;
	Section = MemoryArea->Data.SectionData.Section;
	Region = MmFindRegion
		(MemoryArea->StartingAddress,
		 &MemoryArea->Data.SectionData.RegionListHead,
		 Address, NULL);

	/*
	 * Lock the segment
	 */
	MmLockSectionSegment(Segment);
	DPRINT("Acquired Lock %x\n", Segment);

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

	Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
	HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);
	DPRINT("Entry %x HasSwapEntry %x\n", Entry, HasSwapEntry);

	if (Required->Page[0] && Required->State)
	{
		Page = Required->Page[0];

		DPRINT("Have Page: %x %d\n", Page, Required->State);
	    if (Required->State & 1)
		{
			DPRINT("Set in section @ %x\n", Offset.LowPart);
			Status = MiSetPageEntrySectionSegment
				(Segment, &Offset, MAKE_PFN_SSE(Page));
			KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
		}

		if (Required->State & 2)
		{
			DPRINT("Set in address space @ %x\n", Address);
			Status = MmCreateVirtualMapping
				(Process, Address, Attributes, &Page, 1);
			if (NT_SUCCESS(Status))
			{
				MmInsertRmap(Page, Process, Address);
				if (Locked) MmLockPage(Page);
			}
			DPRINT("Set clean %x\n", Page);
			MmSetCleanAllRmaps(Page);
		}

		if (Required->State & 4)
		{
			DPRINT("Take a reference\n");
			Status = STATUS_SUCCESS;
			MmReferencePage(Page);
		}

		MmUnlockSectionSegment(Segment);
		DPRINT("Done: %x\n", Status);
		return Status;
	}

	if (Entry == 0 && !HasSwapEntry && Offset.QuadPart < PAGE_ROUND_UP(Segment->RawLength.QuadPart))
	{
		Required->State = 7;
		Required->Context = Section->FileObject;
		Required->Consumer = MC_USER;
		Required->FileOffset.QuadPart = Offset.QuadPart + Segment->Image.FileOffset;
		DPRINT("Normal page from disk @ %x\n", Required->FileOffset.LowPart);
		Required->DoAcquisition = MiReadFilePage;
		MmUnlockSectionSegment(Segment);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else if (HasSwapEntry)
	{
		MmGetPageFileMapping(Process, (PVOID)PAddress, &Required->SwapEntry);
		if (Required->SwapEntry == MM_WAIT_ENTRY)
		{
			DPRINT("Wait\n");
			MmUnlockSectionSegment(Segment);
			return STATUS_SUCCESS + 1;
		}
		/*
		 * Must be private page we have swapped out.
		 */
		DPRINT("Private swapped out page\n");
		MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
		Required->State = 2;
		Required->Consumer = MC_USER;
		Required->DoAcquisition = MiSwapInPage;
		MmUnlockSectionSegment(Segment);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else if (IS_SWAP_FROM_SSE(Entry) && SWAPENTRY_FROM_SSE(Entry) == MM_WAIT_ENTRY)
	{
		DPRINT("Waiting\n");
		MmUnlockSectionSegment(Segment);
		return STATUS_SUCCESS + 1;
	}
	else if (IS_SWAP_FROM_SSE(Entry))
	{
		DPRINT("Swap in section page\n");
		Required->State = 7;
		Required->Consumer = MC_USER;
		MmGetPageFileMapping(Process, (PVOID)PAddress, &Required->SwapEntry);
		MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
		Required->DoAcquisition = MiSwapInPage;
		MmUnlockSectionSegment(Segment);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	/*
	 * Map anonymous memory for BSS sections
	 */
	else if (Entry == 0 &&
			 ((Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||
			  Offset.QuadPart >= PAGE_ROUND_UP(Segment->RawLength.QuadPart))) 
	{
		DPRINT("BSS Page\n");
		Required->State = 2;
		Required->Amount = 1;
		Required->Consumer = MC_USER;
		Required->DoAcquisition = MiGetOnePage;
		MmUnlockSectionSegment(Segment);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else
	{
		/*
		 * If the section offset is already in-memory and valid then just
		 * take another reference to the page
		 */

		Page = PFN_FROM_SSE(Entry);
		DPRINT("Take reference to page %x\n", Page);

		MmReferencePage(Page);

		Status = MmCreateVirtualMapping
			(Process, Address, Attributes, &Page, 1);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("Unable to create virtual mapping\n");
			ASSERT(FALSE);
		}
		MmInsertRmap(Page, Process, (PVOID)PAddress);
		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
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
 PMEMORY_AREA MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required)
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

   if (!Required->Page[0])
   {
	   Required->Consumer = MC_PPOOL;
	   Required->Amount = 1;
	   Required->DoAcquisition = MiGetOnePage;
	   return STATUS_MORE_PROCESSING_REQUIRED;
   }

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);

   /*
    * Find the offset of the page
    */
   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   Offset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress +
	   MemoryArea->Data.SectionData.ViewOffset.QuadPart;

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

   DPRINT("Entry %x OldPage %x PFN_FROM_SSE(Entry) %x\n", Entry, OldPage, PFN_FROM_SSE(Entry));
   if (IS_SWAP_FROM_SSE(Entry) ||
       PFN_FROM_SSE(Entry) != OldPage)
   {
      /* This is a private page. We must only change the page protection. */
      MmSetPageProtect(Process, PAddress, Region->Protect);
	  MmUnlockSectionSegment(Segment);
	  DPRINT("Private\n");
      return(STATUS_SUCCESS);
   }

   if (!Required->Page[0])
   {
	   Required->Consumer = MC_USER;
	   Required->Amount = 1;
	   Required->DoAcquisition = MiGetOnePage;
	   MmUnlockSectionSegment(Segment);
	   return STATUS_MORE_PROCESSING_REQUIRED;
   }

   NewPage = Required->Page[0];
   DPRINT("Allocated page %x\n", NewPage);

   /*
    * Copy the old page
    */
   DPRINT("Copying\n");
   MiCopyFromUserPage(NewPage, PAddress);

   /*
    * Unshare the old page.
    */
   MmDeleteRmap(OldPage, Process, PAddress);

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
      DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
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

   MmInsertRmap(NewPage, Process, PAddress);
   MmDereferencePage(OldPage);
   MmUnlockSectionSegment(Segment);

   DPRINT("Address 0x%.8X\n", Address);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiSwapInSectionPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Required)
{
	NTSTATUS Status;
	Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Required->Page[0]);
	if (!NT_SUCCESS(Status))
	{
		ASSERT(FALSE);
	}
	
	return MmReadFromSwapPage(Required->SwapEntry, Required->Page[0]);
}

NTSTATUS
NTAPI
MmWritePageSectionView(PMMSUPPORT AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address)
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
    * If this page was direct mapped from the cache then the cache manager
    * will take care of writing it back to disk.
    */
   if (!Private)
   {
      ASSERT(SwapEntry == 0);
	  DPRINT1("MiWriteBackPage(%wZ,%08x%08x)\n", &FileObject->FileName, Offset.u.HighPart, Offset.u.LowPart);
      Status = MiWriteBackPage(FileObject, &Offset, PAGE_SIZE, Page);
	  MmSetCleanAllRmaps(Page);
	  MmUnlockSectionSegment(Segment);
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
		 MmUnlockSectionSegment(Segment);
         return(STATUS_PAGEFILE_QUOTA);
      }
      MmSetSavedSwapEntryPage(Page, SwapEntry);
   }

   /*
    * Write the page to the pagefile
    */
   DPRINT1("Writing swap entry: %x %x\n", SwapEntry, Page);
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
	  MmUnlockSectionSegment(Segment);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("Set clean %x\n", Page);
   MmSetCleanAllRmaps(Page);
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmPageOutPageFileView
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 PMM_REQUIRED_RESOURCES Required)
{
	NTSTATUS Status;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
	LARGE_INTEGER TotalOffset;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	PMM_REGION Region;
	PVOID PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
	ULONG Entry;
	PFN_TYPE OurPage, SectionPage;

	TotalOffset.QuadPart = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress + 
		MemoryArea->Data.SectionData.ViewOffset.QuadPart;

	Segment = MemoryArea->Data.SectionData.Segment;
	Section = MemoryArea->Data.SectionData.Section;
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);

	MmLockSectionSegment(Segment);
	OurPage = Required->Page[0];

	Entry = MiGetPageEntrySectionSegment(Segment, &TotalOffset);
	ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	SectionPage = PFN_FROM_SSE(Entry);

	if (MmIsPageSwapEntry(Process, Address))
	{
		SWAPENTRY SwapEntry;
		MmGetPageFileMapping(Process, Address, &SwapEntry);
		ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		if (SwapEntry == MM_WAIT_ENTRY)
		{
			DPRINT1
				("SwapEntry is a WAIT, our swap to is %x, State is %x for (%x:%x) on page %x\n",
				 Required->SwapEntry,
				 Required->State,
				 Process,
				 Address,
				 OurPage);				 
			if (Required->SwapEntry || (Required->State & 2))
			{
				Status = MmCreatePageFileMapping(Process, Address, Required->SwapEntry);
				MmSetSavedSwapEntryPage(OurPage, 0);
				ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
				MmDeleteRmap(OurPage, Process, Address);
				MmDereferencePage(OurPage);
				MmUnlockSectionSegment(Segment);
				KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
				ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
				DPRINT("XXX Set Event %x\n", Status);
				return Status;
			}
			else
			{
				MmUnlockSectionSegment(Segment);
				return STATUS_SUCCESS + 1;
			}
		}
	}

	DPRINT("Entry %x SectionPage %x OurPage %x\n", Entry, SectionPage, OurPage);
	if (!MM_IS_WAIT_PTE(Entry) && 
		(!Entry || IS_SWAP_FROM_SSE(Entry) || SectionPage != OurPage))
	{
		// We have a private page that differs from the parent
		DPRINT("Private Page %x vs %x\n", SectionPage, OurPage);
		if (!Required->SwapEntry && !(Required->State & 2))
		{
			Required->State |= 2;
			Required->SwapEntry = MmAllocSwapPage();
			if (!Required->SwapEntry)
			{
				MmUnlockSectionSegment(Segment);
				return STATUS_PAGEFILE_QUOTA;
			}
			DPRINT1("MiWriteSwapPage (%x -> %x)\n", OurPage, Required->SwapEntry);
			Required->DoAcquisition = MiWriteSwapPage;
			MmCreatePageFileMapping(Process, Address, MM_WAIT_ENTRY);
			MmUnlockSectionSegment(Segment);
			return STATUS_MORE_PROCESSING_REQUIRED;
		}
		else
		{
			DPRINT1("Out of swap space for page %x\n", OurPage);
			MmUnlockSectionSegment(Segment);
			return STATUS_PAGEFILE_QUOTA;
		}
	}
	else
	{
		BOOLEAN Dirty = !!MmIsDirtyPageRmap(OurPage);
		Required->State |= Dirty;
		DPRINT("Public page evicting %x (dirty %d)\n", OurPage, Required->State & 1);
		MmDeleteVirtualMapping(Process, Address, FALSE, NULL, NULL);
		MmDeleteRmap(OurPage, Process, Address);
		MmDereferencePage(OurPage);
		MmUnlockSectionSegment(Segment);
		KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
		return STATUS_SUCCESS;
	}
}

