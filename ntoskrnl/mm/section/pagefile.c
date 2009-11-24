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

NTSTATUS
NTAPI
MmNotPresentFaultPageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	PFN_TYPE Page;
	NTSTATUS Status;
	PVOID PAddress;
	LARGE_INTEGER Offset;
	PROS_SECTION_OBJECT Section;
	PMM_SECTION_SEGMENT Segment;
	ULONG Entry;
	ULONG Entry1;
	ULONG Attributes;
	PMM_PAGEOP PageOp;
	PMM_REGION Region;
	BOOLEAN HasSwapEntry;
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

	/*
	 * Get or create a page operation descriptor
	 */
	PageOp = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset.QuadPart, MM_PAGEOP_PAGEIN, FALSE);
	if (PageOp == NULL)
	{
		DPRINT1("MmGetPageOp failed\n");
		ASSERT(FALSE);
	}

	/*
	 * Check if someone else is already handling this fault, if so wait
	 * for them
	 */
	if (PageOp->Thread != PsGetCurrentThread())
	{
		MmUnlockSectionSegment(Segment);
		MmUnlockAddressSpace(AddressSpace);
		Status = MmspWaitForPageOpCompletionEvent(PageOp);
		/*
		 * Check for various strange conditions
		 */
		if (Status != STATUS_SUCCESS)
		{
			DPRINT1("Failed to wait for page op, status = %x\n", Status);
			ASSERT(FALSE);
		}
		if (PageOp->Status == STATUS_PENDING)
		{
			DPRINT1("Woke for page op before completion\n");
			ASSERT(FALSE);
		}
		MmLockAddressSpace(AddressSpace);
		/*
		 * If this wasn't a pagein then restart the operation
		 */
		if (PageOp->OpType != MM_PAGEOP_PAGEIN)
		{
			MmspCompleteAndReleasePageOp(PageOp);
			DPRINT("Address 0x%.8X\n", Address);
			return(STATUS_MM_RESTART_OPERATION);
		}

		/*
		 * If the thread handling this fault has failed then we don't retry
		 */
		if (!NT_SUCCESS(PageOp->Status))
		{
			Status = PageOp->Status;
			MmspCompleteAndReleasePageOp(PageOp);
			DPRINT("Address 0x%.8X\n", Address);
			return(Status);
		}
		MmLockSectionSegment(Segment);
		/*
		 * If the completed fault was for another address space then set the
		 * page in this one.
		 */
		if (!MmIsPagePresent(Process, Address))
		{
			DPRINT("!MmIsPagePresent(%p, %p)\n", Process, Address);

			Entry = MiGetPageEntrySectionSegment(Segment, &Offset);
			HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);

			if (PAGE_FROM_SSE(Entry) == 0 || HasSwapEntry)
			{
				/*
				 * The page was a private page in another or in our address space
				 */
				MmUnlockSectionSegment(Segment);
				MmspCompleteAndReleasePageOp(PageOp);
				return(STATUS_MM_RESTART_OPERATION);
			}

			Page = PFN_FROM_SSE(Entry);

			MmSharePageEntrySectionSegment(Segment, &Offset);

			/* FIXME: Should we call MmCreateVirtualMappingUnsafe if
			 * (Section->AllocationAttributes & SEC_PHYSICALMEMORY) is true?
			 */
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
		}
		if (Locked)
		{
			MmLockPage(Page);
		}
		MmUnlockSectionSegment(Segment);
		PageOp->Status = STATUS_SUCCESS;
		MmspCompleteAndReleasePageOp(PageOp);
		DPRINT("Address 0x%.8X\n", Address);
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
		MmUnlockSectionSegment(Segment);
		MmUnlockAddressSpace(AddressSpace);

		Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
		}
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
		Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
		MiSetPageEntrySectionSegment(Segment, &Offset, Entry);
		MmUnlockSectionSegment(Segment);

		MmInsertRmap(Page, Process, (PVOID)PAddress);

		if (Locked)
		{
			MmLockPage(Page);
		}
		PageOp->Status = STATUS_SUCCESS;
		MmspCompleteAndReleasePageOp(PageOp);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
	else if (IS_SWAP_FROM_SSE(Entry))
	{
		Status = MiSwapInSectionPage(AddressSpace, MemoryArea, Segment, Address, &Page);
		// MiSwapInSectionPage unlocks the section segment
		if (NT_SUCCESS(Status) && Locked)
		{
			MmLockPage(Page);
		}
		PageOp->Status = Status;
		MmspCompleteAndReleasePageOp(PageOp);
		return Status;
	}
	else
	{
		/*
		 * If the section offset is already in-memory and valid then just
		 * take another reference to the page
		 */

		Page = PFN_FROM_SSE(Entry);

		MmSharePageEntrySectionSegment(Segment, &Offset);
		MmUnlockSectionSegment(Segment);

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
		PageOp->Status = STATUS_SUCCESS;
		MmspCompleteAndReleasePageOp(PageOp);
		DPRINT("Address 0x%.8X\n", Address);
		return(STATUS_SUCCESS);
	}
}

NTSTATUS
NTAPI
MmAccessFaultPageFile
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	return MiCowSectionPage(AddressSpace, MemoryArea, Address, Locked);
}

NTSTATUS
NTAPI
MmCreatePageFileSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes)
/*
 * Create a section which is backed by the pagefile
 */
{
   LARGE_INTEGER MaximumSize;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   NTSTATUS Status;

   if (UMaximumSize == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }
   MaximumSize = *UMaximumSize;

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
   Section->FileObject = NULL;
   Section->MaximumSize = MaximumSize;
   Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                   TAG_MM_SECTION_SEGMENT);
   if (Segment == NULL)
   {
      ObDereferenceObject(Section);
      return(STATUS_NO_MEMORY);
   }
   Section->Segment = Segment;
   Segment->ReferenceCount = 1;
   ExInitializeFastMutex(&Segment->Lock);
   Segment->Protection = SectionPageProtection;
   Segment->RawLength.QuadPart = MaximumSize.QuadPart;
   Segment->Length.QuadPart = PAGE_ROUND_UP(MaximumSize.QuadPart);
   Segment->Flags = MM_PAGEFILE_SEGMENT;
   Segment->WriteCopy = FALSE;
   MiInitializeSectionPageTable(Segment);
   *SectionObject = Section;
   return(STATUS_SUCCESS);
}

VOID
MmPageOutPageFileDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
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
MmPageOutPageFileView
(PMMSUPPORT AddressSpace,
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
   BOOLEAN IsImageSection;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;

   Context.Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;
   FileOffset = Context.Offset;

   IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Context.Section->FileObject;

   /*
    * Get the section segment entry and the physical address.
    */
   DPRINT("MmPageOutSectionView -> MmGetPageEntrySectionSegment(%p, %x)\n", Context.Segment, Context.Offset);
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
   MmDeleteAllRmaps(Page, (PVOID)&Context, MmPageOutPageFileDeleteMapping);

   /*
    * If the page wasn't dirty then we can just free it as for a readonly page.
    * Since we unmapped all the mappings above we know it will not suddenly
    * become dirty.
    * If the page is from a pagefile section and has no swap entry,
    * we can't free the page at this point.
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (!Context.WasDirty && !Context.Private)
   {
      if (SwapEntry != 0)
      {
         DPRINT1("Found a swap entry for a non dirty, non private and not direct mapped page (address %x)\n",
                 Address);
         ASSERT(FALSE);
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

VOID static
MmFreePageFilePage
(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
 PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   ULONG Entry;
   PMM_PAGEOP PageOp;
   NTSTATUS Status;
   LARGE_INTEGER Offset;
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

   DPRINT("MmFreeSectionPage -> MmGetPageEntrySectionSegment(%p, %x)\n", Segment, Offset.u.LowPart);
   Entry = MiGetPageEntrySectionSegment(Segment, &Offset);

   if (Page != 0)
   {
	   MmDeleteRmap(Page, Process, Address);
	   MmUnsharePageEntrySectionSegment(Section, Segment, &Offset, Dirty, FALSE);
   }
}

VOID
NTAPI
MmUnmapPageFileSegment
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea,
 PROS_SECTION_OBJECT Section,
 PMM_SECTION_SEGMENT Segment)
{
	MmFreeMemoryArea
		(AddressSpace,
		 MemoryArea,
		 MmFreePageFilePage,
		 AddressSpace);
}
