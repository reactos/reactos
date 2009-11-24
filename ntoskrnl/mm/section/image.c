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
#include <reactos/exeformat.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

VOID
NTAPI
MmFreeImageSectionSegments(PFILE_OBJECT FileObject)
{
	PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
	PMM_SECTION_SEGMENT SectionSegments;
	ULONG NrSegments;
	ULONG i;
	
	ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)FileObject->SectionObjectPointer->ImageSectionObject;
	NrSegments = ImageSectionObject->NrSegments;
	SectionSegments = ImageSectionObject->Segments;
	for (i = 0; i < NrSegments; i++)
	{
		if (SectionSegments[i].ReferenceCount != 0)
		{
            DPRINT1("Image segment %d still referenced (was %d)\n", i,
                    SectionSegments[i].ReferenceCount);
            ASSERT(FALSE);
		}
		MiFreePageTablesSectionSegment(&SectionSegments[i]);
	}
	ExFreePool(ImageSectionObject->Segments);
	ExFreePool(ImageSectionObject);
	FileObject->SectionObjectPointer->ImageSectionObject = NULL;
}

BOOLEAN
NTAPI
MmUnsharePageEntryImageSectionSegment
(PROS_SECTION_OBJECT Section,
 PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset)
{
   ULONG Entry;
   SWAPENTRY SwapEntry;

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
      SWAPENTRY SavedSwapEntry;
      PFN_TYPE Page;
      LARGE_INTEGER FileOffset;

      FileOffset.QuadPart = Offset->QuadPart + Segment->Image.FileOffset/*.QuadPart*/;

      Page = PFN_FROM_SSE(Entry);

      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
      if (SavedSwapEntry == 0)
      {
		 if (Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
         {
			 // XXX 
			 SwapEntry = MmAllocSwapPage();
			 if (!SwapEntry)
			 {
				 // Out of swap space
				 ASSERT(FALSE);
			 }
			 MmWriteToSwapPage(SwapEntry, Page);
			 MiSetPageEntrySectionSegment(Segment, Offset, MAKE_SWAP_SSE(SwapEntry));
         }
		 else
			 MiSetPageEntrySectionSegment(Segment, Offset, 0);
		 MmReleasePageMemoryConsumer(MC_USER, Page);
		 DPRINT("Release page %x\n", Page);
      }
      else
      {
		 if (Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
         {
			 /*
			  * FIXME:
			  *   We hold all locks. Nobody can do something with the current
			  *   process and the current segment (also not within an other process).
			  */
			 NTSTATUS Status;
			 Status = MmWriteToSwapPage(SavedSwapEntry, Page);
			 if (!NT_SUCCESS(Status))
			 {
				 DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", Status);
				 ASSERT(FALSE);
			 }
			 MiSetPageEntrySectionSegment(Segment, Offset, MAKE_SWAP_SSE(SavedSwapEntry));
			 MmSetSavedSwapEntryPage(Page, 0);
			 MmReleasePageMemoryConsumer(MC_USER, Page);
			 DPRINT("Release page %x\n", Page);
         }
         else
         {
            DPRINT1("Found a swapentry for a non private page in an image or data file sgment\n");
            ASSERT(FALSE);
         }
      }
   }
   else
   {
      MiSetPageEntrySectionSegment(Segment, Offset, Entry);
   }
   return(SHARE_COUNT_FROM_SSE(Entry) > 0);
}

NTSTATUS
NTAPI
MmNotPresentFaultImageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	LARGE_INTEGER Offset;
	PFN_TYPE Page;
	NTSTATUS Status;
	PVOID PAddress;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	ULONG Entry;
	ULONG Attributes;
	PMM_REGION Region;
	BOOLEAN HasSwapEntry;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
	PHYSICAL_ADDRESS BoundaryAddressMultiple;
	LARGE_INTEGER TotalOffset;

	BoundaryAddressMultiple.QuadPart = 0;

	//DPRINT("Not Present: %p %p (%p-%p)\n", AddressSpace, Address, MemoryArea->StartingAddress, MemoryArea->EndingAddress);
    
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
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);

	/*
	 * Lock the segment
	 */
	MmLockSectionSegment(Segment);

	Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
	HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);

	if (Entry == 0 && !HasSwapEntry && Offset.QuadPart < PAGE_ROUND_UP(Segment->RawLength.QuadPart))
	{
		TotalOffset.QuadPart = Offset.QuadPart + Segment->Image.FileOffset/*.QuadPart*/;

		MmUnlockSectionSegment(Segment);
		MmUnlockAddressSpace(AddressSpace);
		Status = MiReadFilePage(Section->FileObject, &TotalOffset, &Page);
		MmLockAddressSpace(AddressSpace);

		if (!NT_SUCCESS(Status))
		{
			return Status;
		}

		MmLockSectionSegment(Segment);
		Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
		if (Entry != 0) // Handled elsewhere
		{
			MmUnlockSectionSegment(Segment);
			MmReleasePageMemoryConsumer(MC_USER, Page);
			DPRINT("Release page %x\n", Page);
			return STATUS_SUCCESS; // We'll re-fault if we need more
		}
	}

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

	HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);
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
		DPRINT("Allocated page %x\n", Page);

		Status = MmReadFromSwapPage(SwapEntry, Page);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
			ASSERT(FALSE);
		}
		MmLockAddressSpace(AddressSpace);
		Status = MmCreateVirtualMapping(Process,
										Address,
										Region->Protect,
										&Page,
										1);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
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
		//DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}

	/*
	 * Map anonymous memory for BSS sections
	 */
	if (Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	{
		MmUnlockAddressSpace(AddressSpace);
		Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("Status %x\n", Status);
			MmUnlockSectionSegment(Segment);
			return Status;
		}
		DPRINT("Allocated page %x\n", Page);
		MmLockAddressSpace(AddressSpace);
		Status = MmCreateVirtualMapping(Process,
										Address,
										Region->Protect,
										&Page,
										1);
		if (!NT_SUCCESS(Status))
		{
			MmReleasePageMemoryConsumer(MC_USER, Page);
			DPRINT("Release page %x\n", Page);
			MmUnlockSectionSegment(Segment);
			return(Status);
		}
		MmInsertRmap(Page, Process, (PVOID)PAddress);
		if (Locked)
		{
			MmLockPage(Page);
		}

		/*
		 * Cleanup and release locks
		 */
		MmUnlockSectionSegment(Segment);
		//DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}

	/*
	 * Get the entry corresponding to the offset within the section
	 */
	Entry = MiGetPageEntrySectionSegment(Segment, &Offset);

	if (Entry == 0)
	{
		/*
		 * If the entry is zero (and it can't change because we have
		 * locked the segment) then we need to load the page.
		 */

		/*
		 * Release all our locks and read in the page from disk
		 */
		MmUnlockAddressSpace(AddressSpace);

		if (Offset.QuadPart >= PAGE_ROUND_UP(Segment->RawLength.QuadPart))
		{
			Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
				MmLockAddressSpace(AddressSpace);
				MmUnlockSectionSegment(Segment);
				return Status;
			}
			DPRINT("Allocated page %x\n", Page);
			Status = MmCreateVirtualMapping(Process,
											Address,
											Attributes,
											&Page,
											1);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("Unable to create virtual mapping\n");
				MmReleasePageMemoryConsumer(MC_USER, Page);
				DPRINT("Release page %x\n", Page);
				MmLockAddressSpace(AddressSpace);
				MmUnlockSectionSegment(Segment);
				return Status;
			}
		}
		else
		{
			Status = MmCreateVirtualMapping(Process,
											Address,
											Attributes,
											&Page,
											1);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("Unable to create virtual mapping\n");
				MmReleasePageMemoryConsumer(MC_USER, Page);
				DPRINT("Release page %x\n", Page);
				MmLockAddressSpace(AddressSpace);
				MmUnlockSectionSegment(Segment);
				return Status;
			}

			MmSetCleanPage(Process, Address);
		}

		/*
		 * Relock the address space and segment
		 */
		MmLockAddressSpace(AddressSpace);

		/*
		 * Mark the offset within the section as having valid, in-memory
		 * data
		 */
		Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
		MiSetPageEntrySectionSegment(Segment, &Offset, Entry);

		MmInsertRmap(Page, Process, (PVOID)PAddress);

		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		//DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
	else if (IS_SWAP_FROM_SSE(Entry))
	{
		Status = MiSwapInSectionPage(AddressSpace, MemoryArea, Segment, Address, &Page);
		// MiSwapInSectionPage unlocks the section segment
		if (Locked)
		{
			MmLockPage(Page);
		}
		//DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
	else
	{
		/*
		 * If the section offset is already in-memory and valid then just
		 * take another reference to the page
		 */

		Page = PFN_FROM_SSE(Entry);

		MmSharePageEntrySectionSegment(Segment, &Offset);

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
		//DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
}

NTSTATUS
NTAPI
MmPageOutImageFile(PMMSUPPORT AddressSpace,
				   MEMORY_AREA* MemoryArea,
				   PVOID Address,
				   PMM_PAGEOP PageOp)
{
   PFN_TYPE Page;
   MM_SECTION_PAGEOUT_CONTEXT Context;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   LARGE_INTEGER FileOffset;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   BOOLEAN DirectMapped;
   BOOLEAN IsImageSection;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;

   Context.Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;
   FileOffset.QuadPart = Context.Offset.QuadPart + Context.Segment->Image.FileOffset/*.QuadPart*/;

   IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Context.Section->FileObject;
   if (FileObject != NULL &&
       !(Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED))
   {
      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if ((FileOffset.QuadPart % PAGE_SIZE) == 0 &&
            (Context.Offset.QuadPart + PAGE_SIZE <= Context.Segment->RawLength.QuadPart || !IsImageSection))
      {
         DirectMapped = TRUE;
      }
   }

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
   if (Context.Segment->Image.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
         IS_SWAP_FROM_SSE(Entry) ||
         PFN_FROM_SSE(Entry) != Page)
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
   if (Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a shared section segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
         ASSERT(FALSE);
      }
      if (!Context.WasDirty || SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(Page, 0);
         if (SwapEntry != 0)
         {
            MiSetPageEntrySectionSegment(Context.Segment, &Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         }
         MmReleasePageMemoryConsumer(MC_USER, Page);
		 DPRINT("Release page %x\n", Page);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (!Context.WasDirty && !DirectMapped && !Context.Private)
   {
      if (SwapEntry != 0)
      {
         DPRINT1("Found a swap entry for a non dirty, non private and not direct mapped page (address %x)\n",
                 Address);
         ASSERT(FALSE);
      }
      MmReleasePageMemoryConsumer(MC_USER, Page);
	  DPRINT("Release page %x\n", Page);
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
	  DPRINT("Release page %x\n", Page);
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
   MmSetSavedSwapEntryPage(Page, 0);
   if (Context.Segment->Image.Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      MiSetPageEntrySectionSegment(Context.Segment, &Context.Offset, MAKE_SWAP_SSE(SwapEntry));
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
	  DPRINT("Release page %x\n", Page);
   }

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
MmAccessFaultImageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	return MiCowSectionPage(AddressSpace, MemoryArea, Address, Locked);
}

/*
 TODO: not that great (declaring loaders statically, having to declare all of
 them, having to keep them extern, etc.), will fix in the future
*/
extern NTSTATUS NTAPI PeFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

extern NTSTATUS NTAPI ElfFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

/* TODO: this is a standard DDK/PSDK macro */
#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(ARR_) (sizeof(ARR_) / sizeof((ARR_)[0]))
#endif

static PEXEFMT_LOADER ExeFmtpLoaders[] =
{
 PeFmtCreateSection,
#ifdef __ELF
 ElfFmtCreateSection
#endif
};

static
PMM_SECTION_SEGMENT
NTAPI
ExeFmtpAllocateSegments(IN ULONG NrSegments)
{
 SIZE_T SizeOfSegments;
 PMM_SECTION_SEGMENT Segments;

 /* TODO: check for integer overflow */
 SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * NrSegments;

 Segments = ExAllocatePoolWithTag(NonPagedPool,
                                  SizeOfSegments,
                                  TAG_MM_SECTION_SEGMENT);

 if(Segments)
  RtlZeroMemory(Segments, SizeOfSegments);

 return Segments;
}

static
NTSTATUS
NTAPI
ExeFmtpReadFile(IN PVOID File,
                IN PLARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PVOID * Data,
                OUT PVOID * AllocBase,
                OUT PULONG ReadSize)
{
   NTSTATUS Status;
   LARGE_INTEGER FileOffset;
   ULONG AdjustOffset;
   ULONG OffsetAdjustment;
   ULONG BufferSize;
   ULONG UsedSize;
   PVOID Buffer;
   IO_STATUS_BLOCK Iosb;

   ASSERT_IRQL_LESS(DISPATCH_LEVEL);

   if(Length == 0)
   {
      ASSERT(FALSE);
   }

   FileOffset = *Offset;

   /* Negative/special offset: it cannot be used in this context */
   if(FileOffset.u.HighPart < 0)
   {
      ASSERT(FALSE);
   }

   AdjustOffset = PAGE_ROUND_DOWN(FileOffset.u.LowPart);
   OffsetAdjustment = FileOffset.u.LowPart - AdjustOffset;
   FileOffset.u.LowPart = AdjustOffset;

   BufferSize = Length + OffsetAdjustment;
   BufferSize = PAGE_ROUND_UP(BufferSize);

   /*
    * It's ok to use paged pool, because this is a temporary buffer only used in
    * the loading of executables. The assumption is that MmCreateSection is
    * always called at low IRQLs and that these buffers don't survive a brief
    * initialization phase
    */
   Buffer = ExAllocatePoolWithTag(PagedPool,
                                  BufferSize,
                                  'MmXr');

   UsedSize = 0;

   Status = MiSimpleRead(File, &FileOffset, Buffer, BufferSize, &Iosb);

   if(NT_SUCCESS(Status) && UsedSize < OffsetAdjustment)
   {
      Status = STATUS_IN_PAGE_ERROR;
      ASSERT(!NT_SUCCESS(Status));
   }

   UsedSize = Iosb.Information;

   if(NT_SUCCESS(Status))
   {
      *Data = (PVOID)((ULONG_PTR)Buffer + OffsetAdjustment);
      *AllocBase = Buffer;
      *ReadSize = UsedSize - OffsetAdjustment;
   }
   else
   {
      ExFreePool(Buffer);
   }

   return Status;
}

#ifdef NASSERT
# define MmspAssertSegmentsSorted(OBJ_) ((void)0)
# define MmspAssertSegmentsNoOverlap(OBJ_) ((void)0)
# define MmspAssertSegmentsPageAligned(OBJ_) ((void)0)
#else
static
VOID
NTAPI
MmspAssertSegmentsSorted(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 1; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
             ImageSectionObject->Segments[i - 1].Image.VirtualAddress);
   }
}

static
VOID
NTAPI
MmspAssertSegmentsNoOverlap(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   MmspAssertSegmentsSorted(ImageSectionObject);

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].Length.QuadPart > 0);

      if(i > 0)
      {
         ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
                (ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
                 ImageSectionObject->Segments[i - 1].Length.QuadPart));
      }
   }
}

static
VOID
NTAPI
MmspAssertSegmentsPageAligned(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT((ImageSectionObject->Segments[i].Image.VirtualAddress % PAGE_SIZE) == 0);
      ASSERT((ImageSectionObject->Segments[i].Length.QuadPart % PAGE_SIZE) == 0);
   }
}
#endif

static
int
__cdecl
MmspCompareSegments(const void * x,
                    const void * y)
{
   const MM_SECTION_SEGMENT *Segment1 = (const MM_SECTION_SEGMENT *)x;
   const MM_SECTION_SEGMENT *Segment2 = (const MM_SECTION_SEGMENT *)y;

   return
      (Segment1->Image.VirtualAddress - Segment2->Image.VirtualAddress) >>
      ((sizeof(ULONG_PTR) - sizeof(int)) * 8);
}

/*
 * Ensures an image section's segments are sorted in memory
 */
static
VOID
NTAPI
MmspSortSegments(IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
                 IN ULONG Flags)
{
   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED)
   {
      MmspAssertSegmentsSorted(ImageSectionObject);
   }
   else
   {
      qsort(ImageSectionObject->Segments,
            ImageSectionObject->NrSegments,
            sizeof(ImageSectionObject->Segments[0]),
            MmspCompareSegments);
   }
}


/*
 * Ensures an image section's segments don't overlap in memory and don't have
 * gaps and don't have a null size. We let them map to overlapping file regions,
 * though - that's not necessarily an error
 */
static
BOOLEAN
NTAPI
MmspCheckSegmentBounds
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP)
   {
      MmspAssertSegmentsNoOverlap(ImageSectionObject);
      return TRUE;
   }

   ASSERT(ImageSectionObject->NrSegments >= 1);

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
	   if(ImageSectionObject->Segments[i].Length.QuadPart == 0)
      {
         return FALSE;
      }

      if(i > 0)
      {
         /*
          * TODO: relax the limitation on gaps. For example, gaps smaller than a
          * page could be OK (Windows seems to be OK with them), and larger gaps
          * could lead to image sections spanning several discontiguous regions
          * (NtMapViewOfSection could then refuse to map them, and they could
          * e.g. only be allowed as parameters to NtCreateProcess, like on UNIX)
          */
         if ((ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
              ImageSectionObject->Segments[i - 1].Length.QuadPart) !=
              ImageSectionObject->Segments[i].Image.VirtualAddress)
         {
            return FALSE;
         }
      }
   }

   return TRUE;
}

/*
 * Merges and pads an image section's segments until they all are page-aligned
 * and have a size that is a multiple of the page size
 */
static
BOOLEAN
NTAPI
MmspPageAlignSegments
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;
   ULONG LastSegment;
   BOOLEAN Initialized;
   PMM_SECTION_SEGMENT EffectiveSegment;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED)
   {
      MmspAssertSegmentsPageAligned(ImageSectionObject);
      return TRUE;
   }

   Initialized = FALSE;
   LastSegment = 0;
   EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      /*
       * The first segment requires special handling
       */
      if (i == 0)
      {
         ULONG_PTR VirtualAddress;
         ULONG_PTR VirtualOffset;

         VirtualAddress = EffectiveSegment->Image.VirtualAddress;

         /* Round down the virtual address to the nearest page */
         EffectiveSegment->Image.VirtualAddress = PAGE_ROUND_DOWN(VirtualAddress);

         /* Round up the virtual size to the nearest page */
         EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(VirtualAddress + EffectiveSegment->Length.QuadPart) -
                                    EffectiveSegment->Image.VirtualAddress;

         /* Adjust the raw address and size */
         VirtualOffset = VirtualAddress - EffectiveSegment->Image.VirtualAddress;

         if (EffectiveSegment->Image.FileOffset/*.QuadPart*/ < VirtualOffset)
         {
            return FALSE;
         }

         /*
          * Garbage in, garbage out: unaligned base addresses make the file
          * offset point in curious and odd places, but that's what we were
          * asked for
          */
         EffectiveSegment->Image.FileOffset/*.QuadPart*/ -= VirtualOffset;
         EffectiveSegment->RawLength.QuadPart += VirtualOffset;
      }
      else
      {
         PMM_SECTION_SEGMENT Segment = &ImageSectionObject->Segments[i];
         ULONG_PTR EndOfEffectiveSegment;

         EndOfEffectiveSegment = EffectiveSegment->Image.VirtualAddress + EffectiveSegment->Length.QuadPart;
         ASSERT((EndOfEffectiveSegment % PAGE_SIZE) == 0);

         /*
          * The current segment begins exactly where the current effective
          * segment ended, therefore beginning a new effective segment
          */
         if (EndOfEffectiveSegment == Segment->Image.VirtualAddress)
         {
            LastSegment ++;
            ASSERT(LastSegment <= i);
            ASSERT(LastSegment < ImageSectionObject->NrSegments);

            EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

            if (LastSegment != i)
            {
               /*
                * Copy the current segment. If necessary, the effective segment
                * will be expanded later
                */
               *EffectiveSegment = *Segment;
            }

            /*
             * Page-align the virtual size. We know for sure the virtual address
             * already is
             */
            ASSERT((EffectiveSegment->Image.VirtualAddress % PAGE_SIZE) == 0);
            EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(EffectiveSegment->Length.QuadPart);
         }
         /*
          * The current segment is still part of the current effective segment:
          * extend the effective segment to reflect this
          */
         else if (EndOfEffectiveSegment > Segment->Image.VirtualAddress)
         {
            static const ULONG FlagsToProtection[16] =
            {
               PAGE_NOACCESS,
               PAGE_READONLY,
               PAGE_READWRITE,
               PAGE_READWRITE,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READWRITE,
               PAGE_EXECUTE_READWRITE,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY
            };

            unsigned ProtectionFlags;

            /*
             * Extend the file size
             */

            /* Unaligned segments must be contiguous within the file */
            if (Segment->Image.FileOffset/*.QuadPart*/ != 
				(EffectiveSegment->Image.FileOffset/*.QuadPart*/ +
		 EffectiveSegment->RawLength.QuadPart))
            {
               return FALSE;
            }

            EffectiveSegment->RawLength.QuadPart += Segment->RawLength.QuadPart;

            /*
             * Extend the virtual size
             */
            ASSERT(PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) >= EndOfEffectiveSegment);

            EffectiveSegment->Length.QuadPart = 
				PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) -
				EffectiveSegment->Image.VirtualAddress;

            /*
             * Merge the protection
             */
            EffectiveSegment->Protection |= Segment->Protection;

            /* Clean up redundance */
            ProtectionFlags = 0;

            if(EffectiveSegment->Protection & PAGE_IS_READABLE)
               ProtectionFlags |= 1 << 0;

            if(EffectiveSegment->Protection & PAGE_IS_WRITABLE)
               ProtectionFlags |= 1 << 1;

            if(EffectiveSegment->Protection & PAGE_IS_EXECUTABLE)
               ProtectionFlags |= 1 << 2;

            if(EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
               ProtectionFlags |= 1 << 3;

            ASSERT(ProtectionFlags < 16);
            EffectiveSegment->Protection = FlagsToProtection[ProtectionFlags];

            /* If a segment was required to be shared and cannot, fail */
            if(!(Segment->Protection & PAGE_IS_WRITECOPY) &&
               EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
            {
               return FALSE;
            }
         }
         /*
          * We assume no holes between segments at this point
          */
         else
         {
            ASSERT(FALSE);
         }
      }
   }
   ImageSectionObject->NrSegments = LastSegment + 1;

   return TRUE;
}

NTSTATUS
ExeFmtpCreateImageSection(PFILE_OBJECT FileObject,
                          PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   LARGE_INTEGER Offset;
   PVOID FileHeader;
   PVOID FileHeaderBuffer;
   ULONG FileHeaderSize;
   ULONG Flags;
   ULONG OldNrSegments;
   NTSTATUS Status;
   ULONG i;

   /*
    * Read the beginning of the file (2 pages). Should be enough to contain
    * all (or most) of the headers
    */
   Offset.QuadPart = 0;

   /* FIXME: use FileObject instead of FileHandle */
   Status = ExeFmtpReadFile (FileObject,
                             &Offset,
                             PAGE_SIZE * 2,
                             &FileHeader,
                             &FileHeaderBuffer,
                             &FileHeaderSize);

   if (!NT_SUCCESS(Status))
      return Status;

   if (FileHeaderSize == 0)
   {
      ExFreePool(FileHeaderBuffer);
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Look for a loader that can handle this executable
    */
   for (i = 0; i < RTL_NUMBER_OF(ExeFmtpLoaders); ++ i)
   {
      RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject));
      Flags = 0;

      /* FIXME: use FileObject instead of FileHandle */
      Status = ExeFmtpLoaders[i](FileHeader,
                                 FileHeaderSize,
                                 FileObject,
                                 ImageSectionObject,
                                 &Flags,
                                 ExeFmtpReadFile,
                                 ExeFmtpAllocateSegments);

      if (!NT_SUCCESS(Status))
      {
         if (ImageSectionObject->Segments)
         {
            ExFreePool(ImageSectionObject->Segments);
            ImageSectionObject->Segments = NULL;
         }
      }

      if (Status != STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
         break;
   }

   ExFreePool(FileHeaderBuffer);

   /*
    * No loader handled the format
    */
   if (Status == STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
   {
      Status = STATUS_INVALID_IMAGE_NOT_MZ;
      ASSERT(!NT_SUCCESS(Status));
   }

   if (!NT_SUCCESS(Status))
      return Status;

   ASSERT(ImageSectionObject->Segments != NULL);

   /*
    * Some defaults
    */
   /* FIXME? are these values platform-dependent? */
   if(ImageSectionObject->StackReserve == 0)
      ImageSectionObject->StackReserve = 0x40000;

   if(ImageSectionObject->StackCommit == 0)
      ImageSectionObject->StackCommit = 0x1000;

   if(ImageSectionObject->ImageBase == 0)
   {
      if(ImageSectionObject->ImageCharacteristics & IMAGE_FILE_DLL)
         ImageSectionObject->ImageBase = 0x10000000;
      else
         ImageSectionObject->ImageBase = 0x00400000;
   }

   /*
    * And now the fun part: fixing the segments
    */

   /* Sort them by virtual address */
   MmspSortSegments(ImageSectionObject, Flags);

   /* Ensure they don't overlap in memory */
   if (!MmspCheckSegmentBounds(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Ensure they are aligned */
   OldNrSegments = ImageSectionObject->NrSegments;

   if (!MmspPageAlignSegments(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Trim them if the alignment phase merged some of them */
   if (ImageSectionObject->NrSegments < OldNrSegments)
   {
      PMM_SECTION_SEGMENT Segments;
      SIZE_T SizeOfSegments;

      SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * ImageSectionObject->NrSegments;

      Segments = ExAllocatePoolWithTag(PagedPool,
                                       SizeOfSegments,
                                       TAG_MM_SECTION_SEGMENT);

      if (Segments == NULL)
         return STATUS_INSUFFICIENT_RESOURCES;

      RtlCopyMemory(Segments, ImageSectionObject->Segments, SizeOfSegments);
      ExFreePool(ImageSectionObject->Segments);
      ImageSectionObject->Segments = Segments;
   }

   /* And finish their initialization */
   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ExInitializeFastMutex(&ImageSectionObject->Segments[i].Lock);
	  ImageSectionObject->Segments[i].Flags = MM_IMAGE_SEGMENT;
      ImageSectionObject->Segments[i].ReferenceCount = 1;
	  MiInitializeSectionPageTable(&ImageSectionObject->Segments[i]);
   }

   ASSERT(NT_SUCCESS(Status));
   return Status;
}

NTSTATUS
MmCreateImageSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes,
 PFILE_OBJECT FileObject)
{
   PROS_SECTION_OBJECT Section;
   NTSTATUS Status;
   PMM_SECTION_SEGMENT SectionSegments;
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
   ULONG i;
   ULONG FileAccess = 0;

   /*
    * Specifying a maximum size is meaningless for an image section
    */
   if (UMaximumSize != NULL)
   {
	  DPRINT1("STATUS_INVALID_PARAMETER_4\n");
      return(STATUS_INVALID_PARAMETER_4);
   }

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

   /*
    * Create the section
    */
   Status = ObCreateObject (ExGetPreviousMode(),
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
      ObDereferenceObject(FileObject);
	  DPRINT1("Failed - Status %x\n", Status);
      return(Status);
   }

   RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));

   /*
    * Initialize it
    */
   Section->FileObject = FileObject;
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;

   if (FileObject->SectionObjectPointer->ImageSectionObject == NULL)
   {
      NTSTATUS StatusExeFmt;

      ImageSectionObject = ExAllocatePoolWithTag(PagedPool, sizeof(MM_IMAGE_SECTION_OBJECT), TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
      {
         ObDereferenceObject(Section);
		 DPRINT1("STATUS_NO_MEMORY");
         return(STATUS_NO_MEMORY);
      }

      RtlZeroMemory(ImageSectionObject, sizeof(MM_IMAGE_SECTION_OBJECT));

      StatusExeFmt = ExeFmtpCreateImageSection(FileObject, ImageSectionObject);

      if (!NT_SUCCESS(StatusExeFmt))
      {
         ObDereferenceObject(Section);
		 DPRINT1("StatusExeFmt %x\n", StatusExeFmt);
         return(StatusExeFmt);
      }

      Section->ImageSection = ImageSectionObject;
      ASSERT(ImageSectionObject->Segments);

      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Section);
		 DPRINT1("Status %x\n", Status);
         return(Status);
      }

      if (NULL != InterlockedCompareExchangePointer(&FileObject->SectionObjectPointer->ImageSectionObject,
                                                    ImageSectionObject, NULL))
      {
         /*
          * An other thread has initialized the some image in the background
          */
         ExFreePool(ImageSectionObject->Segments);
         ExFreePool(ImageSectionObject);
         ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
         Section->ImageSection = ImageSectionObject;
         SectionSegments = ImageSectionObject->Segments;

         for (i = 0; i < ImageSectionObject->NrSegments; i++)
         {
            (void)InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
         }
      }

      Status = StatusExeFmt;
   }
   else
   {
      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ObDereferenceObject(Section);
		 DPRINT1("Status %x\n", Status);
         return(Status);
      }

      ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
      Section->ImageSection = ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;

      /*
       * Otherwise just reference all the section segments
       */
      for (i = 0; i < ImageSectionObject->NrSegments; i++)
      {
         (void)InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
      }

      Status = STATUS_SUCCESS;
   }
   //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   return(Status);
}

VOID
NTAPI
MmpFreeSharedSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG Length;
   ULONG Entry;
   ULONG SavedSwapEntry;
   PFN_TYPE Page;
   LARGE_INTEGER Offset;

   Page = 0;

   Length = PAGE_ROUND_UP(Segment->Length.QuadPart);
   for (Offset.QuadPart = 0; Offset.QuadPart < Length; Offset.QuadPart += PAGE_SIZE)
   {
      Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
      if (Entry)
      {
         if (IS_SWAP_FROM_SSE(Entry))
         {
            MmFreeSwapPage(SWAPENTRY_FROM_SSE(Entry));
         }
         else
         {
            Page = PFN_FROM_SSE(Entry);
            SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
            if (SavedSwapEntry != 0)
            {
               MmSetSavedSwapEntryPage(Page, 0);
               MmFreeSwapPage(SavedSwapEntry);
            }
            MmReleasePageMemoryConsumer(MC_USER, Page);
			DPRINT1("Release page %x\n", Page);
         }
         MiSetPageEntrySectionSegment(Segment,& Offset, 0);
      }
   }
}

VOID
NTAPI
MiDeleteImageSection(PROS_SECTION_OBJECT Section)
{
	ULONG i;
	ULONG NrSegments;
	ULONG RefCount;
	PMM_SECTION_SEGMENT SectionSegments;
	
	/*
	 * NOTE: Section->ImageSection can be NULL for short time
	 * during the section creating. If we fail for some reason
	 * until the image section is properly initialized we shouldn't
	 * process further here.
	 */
	if (Section->ImageSection == NULL)
		return;
	
	SectionSegments = Section->ImageSection->Segments;
	NrSegments = Section->ImageSection->NrSegments;
	
	for (i = 0; i < NrSegments; i++)
	{
		if (SectionSegments[i].Image.Characteristics & IMAGE_SCN_MEM_SHARED)
		{
            MmLockSectionSegment(&SectionSegments[i]);
		}
		RefCount = InterlockedDecrementUL(&SectionSegments[i].ReferenceCount);
		if (SectionSegments[i].Image.Characteristics & IMAGE_SCN_MEM_SHARED)
		{
            if (RefCount == 0)
            {
				MmpFreeSharedSegment(&SectionSegments[i]);
            }
            MmUnlockSectionSegment(&SectionSegments[i]);
		}
	}
}

NTSTATUS
NTAPI
MiMapImageFileSection
(PMMSUPPORT AddressSpace,
 PROS_SECTION_OBJECT Section,
 PVOID *BaseAddress)
{
	ULONG i;
	ULONG NrSegments;
	ULONG_PTR ImageBase;
	ULONG ImageSize;
	NTSTATUS Status = STATUS_SUCCESS;
	PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
	PMM_SECTION_SEGMENT SectionSegments;
	
	ImageSectionObject = Section->ImageSection;
	SectionSegments = ImageSectionObject->Segments;
	NrSegments = ImageSectionObject->NrSegments;	
	
	ImageBase = (ULONG_PTR)*BaseAddress;
	if (ImageBase == 0)
	{
		ImageBase = ImageSectionObject->ImageBase;
	}
	
	ImageSize = 0;
	for (i = 0; i < NrSegments; i++)
	{
		if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
		{
            ULONG_PTR MaxExtent;
            MaxExtent = (ULONG_PTR)SectionSegments[i].Image.VirtualAddress +
				SectionSegments[i].Length.QuadPart;
            ImageSize = max(ImageSize, MaxExtent);
		}
	}
	
	ImageSectionObject->ImageSize = ImageSize;
	
	/* Check there is enough space to map the section at that point. */
	if ((AddressSpace != MmGetKernelAddressSpace() &&
		 (ULONG_PTR)ImageBase >= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS) ||
		MmLocateMemoryAreaByRegion(AddressSpace, (PVOID)ImageBase,
								   PAGE_ROUND_UP(ImageSize)) != NULL)
	{
		/* Fail if the user requested a fixed base address. */
		if ((*BaseAddress) != NULL)
		{
            return(STATUS_UNSUCCESSFUL);
		}
		/* Otherwise find a gap to map the image. */
		ImageBase = (ULONG_PTR)MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), PAGE_SIZE, FALSE);
		if (ImageBase == 0)
		{
            return(STATUS_UNSUCCESSFUL);
		}
	}
	
	for (i = 0; i < NrSegments; i++)
	{
		if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
		{
            PVOID SBaseAddress = (PVOID)
				((char*)ImageBase + (ULONG_PTR)SectionSegments[i].Image.VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MiMapViewOfSegment(AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length.u.LowPart,
                                        SectionSegments[i].Protection,
                                        0,
                                        0);
            MmUnlockSectionSegment(&SectionSegments[i]);
		}
	}
	
	if (NT_SUCCESS(Status))
		*BaseAddress = (PVOID)ImageBase;

	return Status;
}

VOID static
MmFreeImagePage
(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
 PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   ULONG Entry;
   SWAPENTRY SavedSwapEntry;
   LARGE_INTEGER Offset;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMMSUPPORT AddressSpace;
   PEPROCESS Process;

   DPRINT("MmFreeImagePage(%x,%x,%x,%d)\n", Address, Page, SwapEntry, Dirty);

   AddressSpace = (PMMSUPPORT)Context;
   Process = MmGetAddressSpaceOwner(AddressSpace);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset.QuadPart = ((ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress) +
            MemoryArea->Data.SectionData.ViewOffset.QuadPart;

   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;

   Entry = MiGetPageEntrySectionSegment(Segment, &Offset);

   if (SwapEntry != 0)
   {
	  DPRINT("SwapEntry %x\n", SwapEntry);
      MmFreeSwapPage(SwapEntry);
   }
   if (Page != 0)
   {
	  DPRINT("Page %x vs %x\n", PFN_FROM_SSE(Entry), Page);
      if (IS_SWAP_FROM_SSE(Entry) ||
          Page != PFN_FROM_SSE(Entry))
      {
         /*
          * Just dereference private pages
          */
         SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
         if (SavedSwapEntry != 0)
         {
			DPRINT("SavedSwapEntry %x\n", SavedSwapEntry);
            MmFreeSwapPage(SavedSwapEntry);
            MmSetSavedSwapEntryPage(Page, 0);
         }
         MmDeleteRmap(Page, Process, Address);
         MmReleasePageMemoryConsumer(MC_USER, Page);
		 DPRINT("Release page %x\n", Page);
      }
      else
      {
		 DPRINT("Unshare %x -> %x @ %08x%08x\n", Address, Page, Offset.HighPart, Offset.LowPart);
         MmDeleteRmap(Page, Process, Address);
         MmUnsharePageEntryImageSectionSegment(Section, Segment, &Offset);
      }
   }
}

NTSTATUS
NTAPI
MiUnmapViewOfSectionSegment(PMMSUPPORT AddressSpace,
							PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   PLIST_ENTRY RegionListHead;

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            BaseAddress);
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

   Status = MmFreeMemoryArea
	   (AddressSpace, MemoryArea, MmFreeImagePage, AddressSpace);

   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(Status);
}

NTSTATUS
NTAPI
MiUnmapImageSection
(PMMSUPPORT AddressSpace, PMEMORY_AREA MemoryArea, PVOID BaseAddress)
{
	NTSTATUS Status = STATUS_SUCCESS;
    PVOID ImageBaseAddress = 0;
	PROS_SECTION_OBJECT Section;

	//DPRINT("MiUnmapImageSection @ %x\n", BaseAddress);
	MemoryArea->DeleteInProgress = TRUE;

	Section = MemoryArea->Data.SectionData.Section;

	if (Section->AllocationAttributes & SEC_IMAGE)
	{
		ULONG i;
		ULONG NrSegments;
		PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
		PMM_SECTION_SEGMENT SectionSegments;
		PMM_SECTION_SEGMENT Segment;

		Segment = MemoryArea->Data.SectionData.Segment;
		ImageSectionObject = Section->ImageSection;
		SectionSegments = ImageSectionObject->Segments;
		NrSegments = ImageSectionObject->NrSegments;

		/* Search for the current segment within the section segments
		 * and calculate the image base address */
		for (i = 0; i < NrSegments; i++)
		{
			if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
			{
				if (Segment == &SectionSegments[i])
				{
					ImageBaseAddress = (char*)BaseAddress - (ULONG_PTR)SectionSegments[i].Image.VirtualAddress;
					break;
				}
			}
		}
		if (i >= NrSegments)
		{
			ASSERT(FALSE);
		}

		for (i = 0; i < NrSegments; i++)
		{
			if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
			{
				PVOID SBaseAddress = (PVOID)
					((char*)ImageBaseAddress + (ULONG_PTR)SectionSegments[i].Image.VirtualAddress);
				Status = MiUnmapViewOfSectionSegment(AddressSpace, SBaseAddress);
			}
		}
	}	

	/* Notify debugger */
	if (ImageBaseAddress) DbgkUnMapViewOfSection(ImageBaseAddress);

	//DPRINT("MiUnmapImageSection Status %x ImageBase %x\n", Status, ImageBaseAddress);
	return Status;
}

