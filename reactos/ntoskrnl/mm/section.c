/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: section.c,v 1.108 2003/04/26 23:13:32 hyperion Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/cc.h>
#include <ddk/ntifs.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct
{
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  LARGE_INTEGER Offset;
  BOOLEAN WasDirty;
  BOOLEAN Private;
} MM_SECTION_PAGEOUT_CONTEXT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED MmSectionObjectType = NULL;

static GENERIC_MAPPING MmpSectionMapping = {
	STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
	STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
	STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
	SECTION_ALL_ACCESS};

#define TAG_MM_SECTION_SEGMENT   TAG('M', 'M', 'S', 'S')
#define TAG_SECTION_PAGE_TABLE   TAG('M', 'S', 'P', 'T')

#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFF000)
#define SHARE_COUNT_FROM_SSE(E)  (((E) & 0x00000FFE) >> 1)
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MAX_SHARE_COUNT          0x7FF
#define MAKE_SSE(P, C)           ((P) | ((C) << 1))
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((S) << 1) | 0x1)

/* FUNCTIONS *****************************************************************/

VOID
MmFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment)
{
  ULONG i;

  for (i = 0; i < NR_SECTION_PAGE_TABLES; i++)
    {
      if (Segment->PageDirectory.PageTables[i] != NULL)
	{
	  ExFreePool(Segment->PageDirectory.PageTables[i]);
	}
    }
}

VOID
MmFreeSectionSegments(PFILE_OBJECT FileObject)
{
  if (FileObject->SectionObjectPointers->ImageSectionObject != NULL)
    {
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;

      ULONG i;

      ImageSectionObject = 
	(PMM_IMAGE_SECTION_OBJECT)FileObject->SectionObjectPointers->
	ImageSectionObject;
      
      for (i = 0; i < ImageSectionObject->NrSegments; i++)
	{
	  if (ImageSectionObject->Segments[i].ReferenceCount != 0)
	    {
	      DPRINT1("Image segment %d still referenced (was %d)\n", i,
		      ImageSectionObject->Segments[i].ReferenceCount);
	      KeBugCheck(0);
	    }
	  MmFreePageTablesSectionSegment(&ImageSectionObject->Segments[i]);
	}
      ExFreePool(ImageSectionObject);
      FileObject->SectionObjectPointers->ImageSectionObject = NULL;
    }
  if (FileObject->SectionObjectPointers->DataSectionObject != NULL)
    {
      PMM_SECTION_SEGMENT Segment;

      Segment = (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointers->
	DataSectionObject;

      if (Segment->ReferenceCount != 0)
	{
	  DPRINT1("Data segment still referenced\n");
	  KeBugCheck(0);
	}
      MmFreePageTablesSectionSegment(Segment);
      ExFreePool(Segment);
      FileObject->SectionObjectPointers->DataSectionObject = NULL;
    }
}

VOID 
MmLockSection(PSECTION_OBJECT Section)
{
  KeWaitForSingleObject(&Section->Lock,
			UserRequest,
			KernelMode,
			FALSE,
			NULL);
}

VOID 
MmUnlockSection(PSECTION_OBJECT Section)
{
   KeReleaseMutex(&Section->Lock, FALSE);
}

VOID
MmLockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
  KeWaitForSingleObject(&Segment->Lock,
			UserRequest,
			KernelMode,
			FALSE,
			NULL);
}

VOID
MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
  KeReleaseMutex(&Segment->Lock, FALSE);
}

VOID 
MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
			     ULONG Offset,
			     ULONG Entry)
{
   PSECTION_PAGE_TABLE Table;
   ULONG DirectoryOffset;
   ULONG TableOffset;
   
   DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
   Table = Segment->PageDirectory.PageTables[DirectoryOffset];
   if (Table == NULL)
     {
	Table = 
	  Segment->PageDirectory.PageTables[DirectoryOffset] =
	  ExAllocatePoolWithTag(NonPagedPool, sizeof(SECTION_PAGE_TABLE),
				TAG_SECTION_PAGE_TABLE);
	memset(Table, 0, sizeof(SECTION_PAGE_TABLE));
	DPRINT("Table %x\n", Table);
     }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Table->Entry[TableOffset] = Entry;
}


ULONG 
MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
			     ULONG Offset)
{
   PSECTION_PAGE_TABLE Table;
   ULONG Entry;
   ULONG DirectoryOffset;
   ULONG TableOffset;
   
   DPRINT("MmGetPageEntrySection(Offset %x)\n", Offset);
   
   DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
   Table = Segment->PageDirectory.PageTables[DirectoryOffset];
   DPRINT("Table %x\n", Table);
   if (Table == NULL)
     {
	return(0);
     }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Entry = Table->Entry[TableOffset];
   return(Entry);
}

VOID
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
			       ULONG Offset)
{
  ULONG Entry;

  Entry = MmGetPageEntrySectionSegment(Segment, Offset);
  if (Entry == 0)
    {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
      KeBugCheck(0);
    }
  if (SHARE_COUNT_FROM_SSE(Entry) == MAX_SHARE_COUNT)
    {
      DPRINT1("Maximum share count reached\n");
      KeBugCheck(0);
    }
  if (IS_SWAP_FROM_SSE(Entry))
    {
      KeBugCheck(0);
    }
  Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) + 1);
  MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

BOOLEAN
MmUnsharePageEntrySectionSegment(PSECTION_OBJECT Section,
				 PMM_SECTION_SEGMENT Segment,
			         ULONG Offset,
				 BOOLEAN Dirty)
{
  ULONG Entry;

  Entry = MmGetPageEntrySectionSegment(Segment, Offset);
  if (Entry == 0)
    {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
      KeBugCheck(0);
    }
  if (SHARE_COUNT_FROM_SSE(Entry) == 0)
    {
      DPRINT1("Zero share count for unshare\n");
      KeBugCheck(0);
    }
  if (IS_SWAP_FROM_SSE(Entry))
    {
      KeBugCheck(0);
    }
  Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) - 1);
  /*
   * If we reducing the share count of this entry to zero then set the entry 
   * to zero and tell the cache the page is no longer mapped.
   */
  if (SHARE_COUNT_FROM_SSE(Entry) == 0)
    {
      PFILE_OBJECT FileObject;
      PBCB Bcb;
      SWAPENTRY SavedSwapEntry;
      PHYSICAL_ADDRESS Page;
      
      MmSetPageEntrySectionSegment(Segment, Offset, 0);
      FileObject = Section->FileObject;
      if (FileObject != NULL)
	{
	  Bcb = FileObject->SectionObjectPointers->SharedCacheMap;
      
	  if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
	      (Offset % PAGE_SIZE) == 0)
	    {
	      NTSTATUS Status;
	      Status = CcRosUnmapCacheSegment(Bcb, Offset, Dirty);
	      if (!NT_SUCCESS(Status))
		{
		  KeBugCheck(0);
		}
	    }
	}

      Page = (PHYSICAL_ADDRESS)(LONGLONG)PAGE_FROM_SSE(Entry);
      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
      if (SavedSwapEntry != 0)
	{
	  MmFreeSwapPage(SavedSwapEntry);
	  MmSetSavedSwapEntryPage(Page, 0);
	}
    }
  else
    {
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
    }
  return(SHARE_COUNT_FROM_SSE(Entry) > 1);
}

NTSTATUS
MiReadPage(PMEMORY_AREA MemoryArea,
	   PLARGE_INTEGER Offset,
	   PHYSICAL_ADDRESS* Page)
     /*
      * FUNCTION: Read a page for a section backed memory area.
      * PARAMETERS:
      *       MemoryArea - Memory area to read the page for.
      *       Offset - Offset of the page to read.
      *       Page - Variable that receives a page contains the read data.
      */
{
  ULONG BaseOffset;
  PVOID BaseAddress;
  BOOLEAN UptoDate;
  PCACHE_SEGMENT CacheSeg;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PBCB Bcb;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  Bcb = FileObject->SectionObjectPointers->SharedCacheMap;
  
  assert(Bcb);

  /*
   * If the file system is letting us go directly to the cache and the
   * memory area was mapped at an offset in the file which is page aligned
   * then get the related cache segment.
   */
  if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
      (Offset->QuadPart % PAGE_SIZE) == 0)
    {
      PHYSICAL_ADDRESS Addr;

      /*
       * Get the related cache segment; we use a lower level interface than
       * filesystems do because it is safe for us to use an offset with a
       * alignment less than the file system block size.
       */
      Status = CcRosGetCacheSegment(Bcb,
				    Offset->u.LowPart,
				    &BaseOffset,
				    &BaseAddress,
				    &UptoDate,
				    &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      if (!UptoDate)
	{
	  /*
	   * If the cache segment isn't up to date then call the file
	   * system to read in the data.
	   */
          Status = ReadCacheSegment(CacheSeg);
	  if (!NT_SUCCESS(Status))
	  {
	      CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	      return Status;
	  }
	}
      /*
       * Retrieve the page from the cache segment that we actually want.
       */
      Addr = MmGetPhysicalAddress(BaseAddress +
				  Offset->u.LowPart - BaseOffset);
      (*Page) = Addr;
      MmReferencePage((*Page));

      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, TRUE);
    }
  else
    {
      PVOID PageAddr;
      ULONG OffsetInPage;
      /*
       * Allocate a page, this is rather complicated by the possibility
       * we might have to move other things out of memory
       */
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, Page);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}

      Status = CcRosGetCacheSegment(Bcb,
				    Offset->u.LowPart,
				    &BaseOffset,
				    &BaseAddress,
				    &UptoDate,
				    &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      if (!UptoDate)
	{
	  /*
	   * If the cache segment isn't up to date then call the file
	   * system to read in the data.
	   */
          Status = ReadCacheSegment(CacheSeg);
	  if (!NT_SUCCESS(Status))
	  {
	      CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	      return Status;
	  }
	}
      PageAddr = ExAllocatePageWithPhysPage(*Page);
      OffsetInPage = BaseOffset + CacheSeg->Bcb->CacheSegmentSize - Offset->u.LowPart;
      if (OffsetInPage >= PAGE_SIZE)
      {
         memcpy(PageAddr, BaseAddress + Offset->u.LowPart - BaseOffset, PAGE_SIZE);
      }
      else
      {
	 memcpy(PageAddr, BaseAddress + Offset->u.LowPart - BaseOffset, OffsetInPage);
         CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
         Status = CcRosGetCacheSegment(Bcb,
				       Offset->u.LowPart + OffsetInPage,
				       &BaseOffset,
				       &BaseAddress,
				       &UptoDate,
				       &CacheSeg);
         if (!NT_SUCCESS(Status))
	   {
	     ExUnmapPage(PageAddr);
	     return(Status);
	   }
         if (!UptoDate)
	   {
	     /*
	      * If the cache segment isn't up to date then call the file
	      * system to read in the data.
	      */
             Status = ReadCacheSegment(CacheSeg);
	     if (!NT_SUCCESS(Status))
	       {
	         CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
		 ExUnmapPage(PageAddr);
	         return Status;
	       }
	   }
	 memcpy(PageAddr + OffsetInPage, BaseAddress, PAGE_SIZE - OffsetInPage);
      }
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
      ExUnmapPage(PageAddr);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
			     MEMORY_AREA* MemoryArea,
			     PVOID Address,
			     BOOLEAN Locked)
{
   LARGE_INTEGER Offset;
   LARGE_INTEGER Page;
   NTSTATUS Status;
   ULONG PAddress;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   ULONG Entry;
   ULONG Entry1;
   ULONG Attributes;
   PMM_PAGEOP PageOp;
   PMM_REGION Region;

   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(NULL, Address))
     {
	if (Locked)
	  {
	    MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	  }  
	return(STATUS_SUCCESS);
     }
   
   PAddress = (ULONG)PAGE_ROUND_DOWN(((ULONG)Address));
   Offset.QuadPart = (PAddress - (ULONG)MemoryArea->BaseAddress) +
     MemoryArea->Data.SectionData.ViewOffset;
   
   /*
    * Lock the segment
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->BaseAddress,
			 &MemoryArea->Data.SectionData.RegionListHead, 
			 Address, NULL);
   MmLockSection(Section);
   MmLockSectionSegment(Segment);

   /*
    * Check if this page needs to be mapped COW
    */
   if ((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
       (Region->Protect == PAGE_READWRITE ||
	Region->Protect == PAGE_EXECUTE_READWRITE))
     {
       Attributes = PAGE_READONLY;
     }
   else
     {
       Attributes = Region->Protect;
     }
   
   /*
    * Get or create a page operation descriptor
    */
   PageOp = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset.u.LowPart,
			MM_PAGEOP_PAGEIN);
   if (PageOp == NULL)
     {
       DPRINT1("MmGetPageOp failed\n");
       KeBugCheck(0);
     }

   /*
    * Check if someone else is already handling this fault, if so wait
    * for them
    */
   if (PageOp->Thread != PsGetCurrentThread())
     {
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       MmUnlockAddressSpace(AddressSpace);
       Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
				      0,
				      KernelMode,
				      FALSE,
				      NULL);
       /*
	* Check for various strange conditions
	*/
       if (Status != STATUS_SUCCESS)
	 {
	   DPRINT1("Failed to wait for page op\n");
	   KeBugCheck(0);
	 }
       if (PageOp->Status == STATUS_PENDING)
	 {
	   DPRINT1("Woke for page op before completion\n");
	   KeBugCheck(0);
	 }
       /*
	* If this wasn't a pagein then restart the operation
	*/
       if (PageOp->OpType != MM_PAGEOP_PAGEIN)
	 {
	   MmLockAddressSpace(AddressSpace);
           KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   DPRINT("Address 0x%.8X\n", Address);
	   return(STATUS_MM_RESTART_OPERATION);
	 }
       /*
	* If the thread handling this fault has failed then we don't retry
	*/
       if (!NT_SUCCESS(PageOp->Status))
	 {
	   MmLockAddressSpace(AddressSpace);
	   Status = PageOp->Status;
	   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   DPRINT("Address 0x%.8X\n", Address);
	   return(Status);
	 }
       MmLockAddressSpace(AddressSpace);
       MmLockSection(Section);
       MmLockSectionSegment(Segment);
       /*
	* If the completed fault was for another address space then set the 
	* page in this one.
	*/
       if (!MmIsPagePresent(NULL, Address))
	 {
	   Entry = MmGetPageEntrySectionSegment(Segment, Offset.u.LowPart);
	   if (Entry == 0)
	   {
		MmUnlockSectionSegment(Segment);
		MmUnlockSection(Section);
                KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	        MmReleasePageOp(PageOp);
	        return(STATUS_MM_RESTART_OPERATION);
	   } 

	   Page = (LARGE_INTEGER)(LONGLONG)(PAGE_FROM_SSE(Entry));
	   MmReferencePage(Page);	
	   MmSharePageEntrySectionSegment(Segment, Offset.u.LowPart);

	   Status = MmCreateVirtualMapping(MemoryArea->Process,
					   Address,
					   MemoryArea->Attributes,
					   Page,
					   FALSE);
	   if (!NT_SUCCESS(Status))
	     {
	       DbgPrint("Unable to create virtual mapping\n");
	       KeBugCheck(0);
	     }
	   MmInsertRmap(Page, MemoryArea->Process, 
			(PVOID)PAGE_ROUND_DOWN(Address));
	 }
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       DPRINT("Address 0x%.8X\n", Address);
       return(STATUS_SUCCESS);
     }

   /*
    * Must be private page we have swapped out.
    */
   if (MmIsPageSwapEntry(NULL, (PVOID)PAddress))
     {
       SWAPENTRY SwapEntry;
       PMDL Mdl;

       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       
       MmDeletePageFileMapping(NULL, (PVOID)PAddress, &SwapEntry);

       Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }

       Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
       MmBuildMdlFromPages(Mdl, (PULONG)&Page);
       Status = MmReadFromSwapPage(SwapEntry, Mdl);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }
       
       Status = MmCreateVirtualMapping(AddressSpace->Process,		      
				       Address,
				       Region->Protect,
				       Page,
				       FALSE);
       while (Status == STATUS_NO_MEMORY)
	 {
	   MmUnlockAddressSpace(AddressSpace);	   
	   Status = MmCreateVirtualMapping(AddressSpace->Process,
					   Address,
					   Region->Protect,
					   Page,
					   TRUE);
	   MmLockAddressSpace(AddressSpace);
	 }  
       if (!NT_SUCCESS(Status))
	 {
	   DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
	   KeBugCheck(0);
	   return(Status);
	 }

       /*
	* Store the swap entry for later use.
	*/
       MmSetSavedSwapEntryPage(Page, SwapEntry);
       
       /*
	* Add the page to the process's working set
	*/
       MmInsertRmap(Page, AddressSpace->Process, 
		    (PVOID)PAGE_ROUND_DOWN(Address));       
       
       /*
	* Finish the operation
	*/
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       DPRINT("Address 0x%.8X\n", Address);
       return(STATUS_SUCCESS);
     }

   /*
    * Satisfying a page fault on a map of /Device/PhysicalMemory is easy
    */
   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
     {
       /*
	* Just map the desired physical page 
	*/
       Status = MmCreateVirtualMapping(AddressSpace->Process,
				       Address,
				       Region->Protect,
				       Offset,
				       FALSE);
       /* 
        * Don't add an rmap entry since the page mapped could be for 
	* anything. 
	*/
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  

       /*
	* Cleanup and release locks
	*/
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       DPRINT("Address 0x%.8X\n", Address);
       return(STATUS_SUCCESS);
     }
   
   /*
    * Map anonymous memory for BSS sections
    */
   if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS)
     {
       Status = MmRequestPageMemoryConsumer(MC_USER, FALSE, &Page);
       if (!NT_SUCCESS(Status))
	 {
	    MmUnlockSectionSegment(Segment);
	    MmUnlockSection(Section);
	    MmUnlockAddressSpace(AddressSpace);	   
	    MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
	    MmLockAddressSpace(AddressSpace);
	    MmLockSection(Section);
	    MmLockSectionSegment(Segment);
	 }

       Status = MmCreateVirtualMapping(AddressSpace->Process,
				       Address,
				       Region->Protect,
				       Page,
				       FALSE);
       MmInsertRmap(Page, AddressSpace->Process, 
		    (PVOID)PAGE_ROUND_DOWN(Address));
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  

       /*
	* Cleanup and release locks
	*/
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       DPRINT("Address 0x%.8X\n", Address);
       return(STATUS_SUCCESS);
     }

   /*
    * Get the entry corresponding to the offset within the section
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset.u.LowPart);
   
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
       MmUnlockSection(Section);
       MmUnlockAddressSpace(AddressSpace);
	
       if (Segment->Flags & MM_PAGEFILE_SEGMENT)
	 {
	    Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
	 }
       else
	 {
	   Status = MiReadPage(MemoryArea, &Offset, &Page);
	 }
       if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
	 {
	   /*
	    * FIXME: What do we know in this case?
	    */
	   DPRINT1("IoPageRead failed (Status %x)\n", Status);
	   
	   /*
	    * Cleanup and release locks
	    */
	   PageOp->Status = Status;
	   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   MmLockAddressSpace(AddressSpace);
	   return(Status);
	 }
       
       /*
	* Relock the address space, section and segment
	*/
       MmLockAddressSpace(AddressSpace);
       MmLockSection(Section);
       MmLockSectionSegment(Segment);
       
       /*
	* Check the entry. No one should change the status of a page
	* that has a pending page-in.
	*/
       Entry1 = MmGetPageEntrySectionSegment(Segment, Offset.QuadPart);
       if (Entry != Entry1)
	 {
	   DbgPrint("Someone changed ppte entry while we slept\n");
	   KeBugCheck(0);
	 }
       
       /*
	* Mark the offset within the section as having valid, in-memory
	* data
	*/
       Entry = Page.u.LowPart;
       MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, Entry);
       MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
       
       Status = MmCreateVirtualMapping(AddressSpace->Process,
				       Address,
				       Attributes,
				       Page,
				       FALSE);
       if (!NT_SUCCESS(Status))
	 {
	   MmUnlockSectionSegment(Segment);
	   MmUnlockSection(Section);
	   MmUnlockAddressSpace(AddressSpace);
	   Status = MmCreateVirtualMapping(AddressSpace->Process,
					   Address,
					   Attributes,
					   Page,
					   TRUE);
	   if (!NT_SUCCESS(Status))
	     {
	       KeBugCheck(0);
	     }
	   MmLockAddressSpace(AddressSpace);
	   MmLockSection(Section);
	   MmLockSectionSegment(Segment);
	 }
       MmInsertRmap(Page, AddressSpace->Process, 
		    (PVOID)PAGE_ROUND_DOWN(Address));     
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("Unable to create virtual mapping\n");
	   KeBugCheck(0);
	 }
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       DPRINT("MmNotPresentFaultSectionView succeeded\n");
       return(STATUS_SUCCESS);
     }
   else if (IS_SWAP_FROM_SSE(Entry))
     {
       SWAPENTRY SwapEntry;
       PMDL Mdl;

       SwapEntry = SWAPENTRY_FROM_SSE(Entry);

       /*
	* Release all our locks and read in the page from disk
	*/
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       MmUnlockAddressSpace(AddressSpace);
	
       Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }

       Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
       MmBuildMdlFromPages(Mdl, (PULONG)&Page);
       Status = MmReadFromSwapPage(SwapEntry, Mdl);
       if (!NT_SUCCESS(Status))
	 {
	   KeBugCheck(0);
	 }

       /*
	* Relock the address space, section and segment
	*/
       MmLockAddressSpace(AddressSpace);
       MmLockSection(Section);
       MmLockSectionSegment(Segment);
       
       /*
	* Check the entry. No one should change the status of a page
	* that has a pending page-in.
	*/
       Entry1 = MmGetPageEntrySectionSegment(Segment, Offset.QuadPart);
       if (Entry != Entry1)
	 {
	   DbgPrint("Someone changed ppte entry while we slept\n");
	   KeBugCheck(0);
	 }
       
       /*
	* Mark the offset within the section as having valid, in-memory
	* data
	*/
       Entry = Page.u.LowPart;
       MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, Entry);
       MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);

       /*
	* Save the swap entry.
	*/
       MmSetSavedSwapEntryPage(Page, SwapEntry);
       
       Status = MmCreateVirtualMapping(AddressSpace->Process,
				       Address,
				       Attributes,
				       Page,
				       FALSE);
       MmInsertRmap(Page, AddressSpace->Process, 
		    (PVOID)PAGE_ROUND_DOWN(Address));
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("Unable to create virtual mapping\n");
	   KeBugCheck(0);
	 }
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       DPRINT("MmNotPresentFaultSectionView succeeded\n");
       return(STATUS_SUCCESS);
     }
   else
     {
	/*
	 * If the section offset is already in-memory and valid then just
	 * take another reference to the page 
	 */
	
	Page = (LARGE_INTEGER)(LONGLONG)PAGE_FROM_SSE(Entry);
	MmReferencePage(Page);	
	MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
	
	Status = MmCreateVirtualMapping(AddressSpace->Process,
					Address,
					Attributes,
					Page,
					FALSE);
	MmInsertRmap(Page, AddressSpace->Process, 
		     (PVOID)PAGE_ROUND_DOWN(Address));
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
       if (Locked)
	 {
	   MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
	 }  
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);	
       return(STATUS_SUCCESS);
     }
}

NTSTATUS 
MmAccessFaultSectionView(PMADDRESS_SPACE AddressSpace,
			 MEMORY_AREA* MemoryArea, 
			 PVOID Address,
			 BOOLEAN Locked)
{
  PMM_SECTION_SEGMENT Segment;
  PSECTION_OBJECT Section;
  PHYSICAL_ADDRESS OldPage;
  PHYSICAL_ADDRESS NewPage;
  PVOID NewAddress;
  NTSTATUS Status;
  ULONG PAddress;
  LARGE_INTEGER Offset;
  PMM_PAGEOP PageOp;
  PMM_REGION Region;

  /*
   * Check if the page has been paged out or has already been set readwrite
   */
   if (!MmIsPagePresent(NULL, Address) ||
       MmGetPageProtect(NULL, Address) & PAGE_READWRITE)
     {
	return(STATUS_SUCCESS);
     }  

   /*
    * Find the offset of the page
    */
   PAddress = (ULONG)PAGE_ROUND_DOWN(((ULONG)Address));
   Offset.QuadPart = (PAddress - (ULONG)MemoryArea->BaseAddress) +
     MemoryArea->Data.SectionData.ViewOffset;

   /*
    * Lock the segment
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->BaseAddress,
			 &MemoryArea->Data.SectionData.RegionListHead,
			 Address, NULL);
   MmLockSection(Section);
   MmLockSectionSegment(Segment);

   /*
    * Sanity check.
    */
   if (MmGetPageEntrySectionSegment(Segment, Offset.QuadPart) == 0)
     {
       DPRINT1("COW fault for page with PESS 0. Address was 0x%.8X\n",
	       Address);
     }

   /*
    * Check if we are doing COW
    */
   if (!((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
	 (Region->Protect == PAGE_READWRITE ||
	  Region->Protect == PAGE_EXECUTE_READWRITE)))
     {
       MmUnlockSection(Section);
       MmUnlockSectionSegment(Segment);
       return(STATUS_UNSUCCESSFUL);
     }

   /*
    * Get or create a pageop
    */
   PageOp = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset.u.LowPart,
			MM_PAGEOP_ACCESSFAULT);
   if (PageOp == NULL)
     {
       DPRINT1("MmGetPageOp failed\n");
       KeBugCheck(0);
     }

   /*
    * Wait for any other operations to complete
    */
   if (PageOp->Thread != PsGetCurrentThread())
     {
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       MmUnlockAddressSpace(AddressSpace);
       Status = KeWaitForSingleObject(&PageOp->CompletionEvent,
				      0,
				      KernelMode,
				      FALSE,
				      NULL);
       /*
	* Check for various strange conditions
	*/
       if (Status != STATUS_SUCCESS)
	 {
	   DPRINT1("Failed to wait for page op\n");
	   KeBugCheck(0);
	 }
       if (PageOp->Status == STATUS_PENDING)
	 {
	   DPRINT1("Woke for page op before completion\n");
	   KeBugCheck(0);
	 }
       /*
	* Restart the operation
	*/
       MmLockAddressSpace(AddressSpace);
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       return(STATUS_MM_RESTART_OPERATION);
     }

   /*
    * Release locks now we have the pageop
    */
   MmUnlockSectionSegment(Segment);
   MmUnlockSection(Section);
   MmUnlockAddressSpace(AddressSpace);

   /*
    * Allocate a page
    */
   Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &NewPage);

   /*
    * Copy the old page
    */
   OldPage = MmGetPhysicalAddressForProcess(NULL, Address);
 
   NewAddress = ExAllocatePageWithPhysPage(NewPage);
   memcpy(NewAddress, (PVOID)PAGE_ROUND_DOWN(Address), PAGE_SIZE);
   ExUnmapPage(NewAddress);

   /*
    * Delete the old entry.
    */
   MmDeleteVirtualMapping(AddressSpace->Process, Address, FALSE, NULL, NULL);

   /*
    * Set the PTE to point to the new page
    */
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateVirtualMapping(AddressSpace->Process,
				   Address,
				   Region->Protect,
				   NewPage,
				   FALSE);   
   MmInsertRmap(NewPage, AddressSpace->Process, 
		(PVOID)PAGE_ROUND_DOWN(Address));
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("Unable to create virtual mapping\n");
       KeBugCheck(0);
     }
   if (Locked)
     {
       MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
     }  

   /*
    * Unshare the old page.
    */
   MmUnsharePageEntrySectionSegment(Section, Segment, Offset.QuadPart, FALSE);
   MmDeleteRmap(OldPage, AddressSpace->Process,
		(PVOID)PAGE_ROUND_DOWN(Address));
   MmReleasePageMemoryConsumer(MC_USER, OldPage);

   PageOp->Status = STATUS_SUCCESS;
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
{
  MM_SECTION_PAGEOUT_CONTEXT* PageOutContext;
  BOOL WasDirty;
  PHYSICAL_ADDRESS PhysicalAddress;

  PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
  MmDeleteVirtualMapping(Process,
			 Address,
			 FALSE,
			 &WasDirty,
			 &PhysicalAddress);
  if (WasDirty)
    {
      PageOutContext->WasDirty = TRUE;
    }
  if (!PageOutContext->Private)
    {
      MmUnsharePageEntrySectionSegment(PageOutContext->Section,
				       PageOutContext->Segment,
				       PageOutContext->Offset.u.LowPart,
				       PageOutContext->WasDirty);
    }
  MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
}

NTSTATUS
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
		     MEMORY_AREA* MemoryArea, 
		     PVOID Address,
		     PMM_PAGEOP PageOp)
{
  LARGE_INTEGER Offset;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  PHYSICAL_ADDRESS PhysicalAddress;
  MM_SECTION_PAGEOUT_CONTEXT Context;
  SWAPENTRY SwapEntry;
  PMDL Mdl;
  ULONG Entry;
  BOOLEAN Private;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PREACTOS_COMMON_FCB_HEADER Fcb;
  BOOLEAN DirectMapped;

  Address = (PVOID)PAGE_ROUND_DOWN(Address);

  Offset.QuadPart = (ULONG)(Address - (ULONG)MemoryArea->BaseAddress) +
     MemoryArea->Data.SectionData.ViewOffset;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  DirectMapped = FALSE;
  if (FileObject != NULL)
    {
      Fcb = (PREACTOS_COMMON_FCB_HEADER)FileObject->FsContext;
  
      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
	  (Offset.QuadPart % PAGE_SIZE) == 0)
	{
	  DirectMapped = TRUE;
	}
    }
   
  /*
   * Get the segment and section.
   */
  Segment = MemoryArea->Data.SectionData.Segment;
  Section = MemoryArea->Data.SectionData.Section;

  /*
   * This should never happen since mappings of physical memory are never
   * placed in the rmap lists.
   */
  if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
      DPRINT1("Trying to page out from physical memory section address 0x%X "
	      "process %d\n", Address, AddressSpace->Process->UniqueProcessId);
      KeBugCheck(0);
    }

  /*
   * Get the section segment entry and the physical address.
   */
  Entry = MmGetPageEntrySectionSegment(Segment, Offset.QuadPart);
  if (!MmIsPagePresent(AddressSpace->Process, Address))
    {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
	      AddressSpace->Process->UniqueProcessId, Address);
      KeBugCheck(0);
    }
  PhysicalAddress = 
    MmGetPhysicalAddressForProcess(AddressSpace->Process, Address);
  SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);

  /*
   * Prepare the context structure for the rmap delete call.
   */
  Context.Section = Section;
  Context.Segment = Segment;
  Context.Offset = Offset;
  Context.WasDirty = FALSE;
  if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
      IS_SWAP_FROM_SSE(Entry) || 
      (LONGLONG)PAGE_FROM_SSE(Entry) != PhysicalAddress.QuadPart)
    {
      Context.Private = Private = TRUE;
    }
  else
    {
      Context.Private = Private = FALSE;
    }

  /*
   * Take an additional reference to the page.
   */
//  MmReferencePage(PhysicalAddress);

  /*
   * Paging out data mapped read-only is easy.
   */
  if (MemoryArea->Attributes & PAGE_READONLY ||
      MemoryArea->Attributes & PAGE_EXECUTE_READ)
    {
      /*
       * Read-only data should never be in the swapfile.
       */
      if (SwapEntry != 0)
	{
	  DPRINT1("SwapEntry != 0 was 0x%.8X at address 0x%.8X, "
		  "paddress 0x%.8X\n", SwapEntry, Address, 
		  PhysicalAddress);
	  KeBugCheck(0);
	}

      /*
       * Read-only data should never be COWed
       */
      if (Private)
	{
	  DPRINT1("Had private copy of read-only page.\n");
	  KeBugCheck(0);
	}
      
      /*
       * Delete all mappings of this page.
       */
      MmDeleteAllRmaps(PhysicalAddress, (PVOID)&Context, 
		       MmPageOutDeleteMapping);
      if (Context.WasDirty)
	{
	  KeBugCheck(0);
	}
      /*
       * If this page wasn't direct mapped then we have a private copy so 
       * release back to the system; otherwise the cache manager will have 
       * handled freeing the cache segment which we mapped from.
       */
      if (!DirectMapped)
	{
	  MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
	}
       
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
    }

  /*
   * Otherwise we have read-write data.
   */
  MmDeleteAllRmaps(PhysicalAddress, (PVOID)&Context, MmPageOutDeleteMapping);
  
  /*
   * If this wasn't a private page then we should have reduced the entry to
   * zero by deleting all the rmaps.
   */
  if (!Private && MmGetPageEntrySectionSegment(Segment, Offset.QuadPart) != 0)
    {
      KeBugCheck(0);
    }

  /*
   * If the page wasn't dirty then we can just free it as for a readonly page.
   * Since we unmapped all the mappings above we know it will not suddenly
   * become dirty.
   */
  if (!Context.WasDirty)
    {
      if (!DirectMapped || Private)
	{
	  MmSetSavedSwapEntryPage(PhysicalAddress, 0);
	  MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
	}
      if (Private)
	{
	  if (!(Segment->Characteristics & IMAGE_SECTION_CHAR_BSS) &&
	      SwapEntry == 0)
	    {
	      DPRINT1("Private page, non-dirty but not swapped out "
		      "process %d address 0x%.8X\n",
		      AddressSpace->Process->UniqueProcessId,
		      Address);	      
	      KeBugCheck(0);
	    }
	  else
	    {
	      Status = MmCreatePageFileMapping(AddressSpace->Process,
					       Address,
					       SwapEntry);
	      if (!NT_SUCCESS(Status))
		{
		  KeBugCheck(0);
		}
	    }
	}
      
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
    }

  /*
   * If this page was direct mapped from the cache then the cache manager
   * will already have taken care of writing it back.
   */
  if (DirectMapped && !Private)
    {
      assert(SwapEntry == 0);
      MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
    }

  /*
   * If necessary, allocate an entry in the paging file for this page
   */
  SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);
  if (SwapEntry == 0)
    {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
	{
	  MmShowOutOfSpaceMessagePagingFile();

	  /*
	   * For private pages restore the old mappings.
	   */
	  if (Private)
	    {
	      Status = MmCreateVirtualMapping(MemoryArea->Process,   
					      Address,
					      MemoryArea->Attributes,
					      PhysicalAddress,
					      FALSE);
	      MmSetDirtyPage(MemoryArea->Process, Address);
	      MmInsertRmap(PhysicalAddress, 
			   MemoryArea->Process,
			   Address);
	    }
	  else
	    {
	      /*
	       * For non-private pages if the page wasn't direct mapped then
	       * set it back into the section segment entry so we don't loose 
	       * our copy. Otherwise it will be handled by the cache manager.
	       */
	      Status = MmCreateVirtualMapping(MemoryArea->Process,   
					      Address,
					      MemoryArea->Attributes,
					      PhysicalAddress,
					      FALSE);
	      MmSetDirtyPage(MemoryArea->Process, Address);
	      MmInsertRmap(PhysicalAddress, 
			   MemoryArea->Process,
			   Address);
	      MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, 
					   PhysicalAddress.u.LowPart);
	      MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
	    }
	   PageOp->Status = STATUS_UNSUCCESSFUL;
	   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	   MmReleasePageOp(PageOp);
	   return(STATUS_UNSUCCESSFUL);
	}
    }

  /*
   * Write the page to the pagefile
   */
  Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
  MmBuildMdlFromPages(Mdl, (PULONG)&PhysicalAddress);
  Status = MmWriteToSwapPage(SwapEntry, Mdl);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", 
	      Status);
      /*
       * As above: undo our actions.
       * FIXME: Also free the swap page.
       */
      if (Private)
	{
	  Status = MmCreateVirtualMapping(MemoryArea->Process,   
					  Address,
					  MemoryArea->Attributes,
					  PhysicalAddress,
					  FALSE);
	  MmSetDirtyPage(MemoryArea->Process, Address);
	  MmInsertRmap(PhysicalAddress, 
		       MemoryArea->Process,
		       Address);
	}
      else
	{
	  Status = MmCreateVirtualMapping(MemoryArea->Process,   
					  Address,
					  MemoryArea->Attributes,
					  PhysicalAddress,
					  FALSE);
	  MmSetDirtyPage(MemoryArea->Process, Address);
	  MmInsertRmap(PhysicalAddress, 
			   MemoryArea->Process,
			   Address);
	  MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, 
				       PhysicalAddress.u.LowPart);
	  MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
	}
      PageOp->Status = STATUS_UNSUCCESSFUL;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
    }

  /*
   * Otherwise we have succeeded.
   */
  DPRINT("MM: Wrote section page 0x%.8X to swap!\n", PhysicalAddress);
  MmSetSavedSwapEntryPage(PhysicalAddress, 0);
  MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);

  if (Private)
    {
      Status = MmCreatePageFileMapping(MemoryArea->Process,   
				       Address,
				       SwapEntry);
      if (!NT_SUCCESS(Status))
	{
	  KeBugCheck(0);
	}
    }
  else
    {
      Entry = MAKE_SWAP_SSE(SwapEntry);
      MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, Entry);
    }

  PageOp->Status = STATUS_SUCCESS;
  KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
  MmReleasePageOp(PageOp);
  return(STATUS_SUCCESS);
}

NTSTATUS 
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MemoryArea,
		       PVOID Address,
		       PMM_PAGEOP PageOp)
{
  LARGE_INTEGER Offset;
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  PHYSICAL_ADDRESS PhysicalAddress;
  SWAPENTRY SwapEntry;
  PMDL Mdl;
  ULONG Entry;
  BOOLEAN Private;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PBCB Bcb = NULL;
  BOOLEAN DirectMapped;

  Address = (PVOID)PAGE_ROUND_DOWN(Address);

  Offset.QuadPart = (ULONG)(Address - (ULONG)MemoryArea->BaseAddress) +
     MemoryArea->Data.SectionData.ViewOffset;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  DirectMapped = FALSE;
  if (FileObject != NULL)
    {
      Bcb = FileObject->SectionObjectPointers->SharedCacheMap;
  
      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
	  (Offset.QuadPart % PAGE_SIZE) == 0)
	{
	  DirectMapped = TRUE;
	}
    }
   
  /*
   * Get the segment and section.
   */
  Segment = MemoryArea->Data.SectionData.Segment;
  Section = MemoryArea->Data.SectionData.Section;

  /*
   * This should never happen since mappings of physical memory are never
   * placed in the rmap lists.
   */
  if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
    {
      DPRINT1("Trying to write back page from physical memory mapped at %X "
	      "process %d\n", Address, AddressSpace->Process->UniqueProcessId);
      KeBugCheck(0);
    }

  /*
   * Get the section segment entry and the physical address.
   */
  Entry = MmGetPageEntrySectionSegment(Segment, Offset.QuadPart);
  if (!MmIsPagePresent(AddressSpace->Process, Address))
    {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
	      AddressSpace->Process->UniqueProcessId, Address);
      KeBugCheck(0);
    }
  PhysicalAddress = 
    MmGetPhysicalAddressForProcess(AddressSpace->Process, Address);
  SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);

  /*
   * Check for a private (COWed) page.
   */
  if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
      IS_SWAP_FROM_SSE(Entry) || 
      (LONGLONG)PAGE_FROM_SSE(Entry) != PhysicalAddress.QuadPart)
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
  MmSetCleanAllRmaps(PhysicalAddress);
  
  /*
   * If this page was direct mapped from the cache then the cache manager
   * will take care of writing it back to disk.
   */
  if (DirectMapped && !Private)
    {
      assert(SwapEntry == 0);
      CcRosMarkDirtyCacheSegment(Bcb, Offset.u.LowPart);
      PageOp->Status = STATUS_SUCCESS;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
    }

  /*
   * If necessary, allocate an entry in the paging file for this page
   */
  SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);
  if (SwapEntry == 0)
    {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
	{
	  MmSetDirtyAllRmaps(PhysicalAddress);
	  PageOp->Status = STATUS_UNSUCCESSFUL;
	  KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	  MmReleasePageOp(PageOp);
	  return(STATUS_UNSUCCESSFUL);
	}
    }

  /*
   * Write the page to the pagefile
   */
  Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
  MmBuildMdlFromPages(Mdl, (PULONG)&PhysicalAddress);
  Status = MmWriteToSwapPage(SwapEntry, Mdl);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", 
	      Status);
      MmSetDirtyAllRmaps(PhysicalAddress);
      PageOp->Status = STATUS_UNSUCCESSFUL;
      KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
      MmReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
    }

  /*
   * Otherwise we have succeeded.
   */
  DPRINT("MM: Wrote section page 0x%.8X to swap!\n", PhysicalAddress);
  MmSetSavedSwapEntryPage(PhysicalAddress, SwapEntry);
  PageOp->Status = STATUS_SUCCESS;
  KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
  MmReleasePageOp(PageOp);
  return(STATUS_SUCCESS);
}

VOID STATIC
MmAlterViewAttributes(PMADDRESS_SPACE AddressSpace,
		      PVOID BaseAddress,
		      ULONG RegionSize,
		      ULONG OldType,
		      ULONG OldProtect,
		      ULONG NewType,
		      ULONG NewProtect)
{
  PMEMORY_AREA MemoryArea;
  PMM_SECTION_SEGMENT Segment;
  BOOL DoCOW = FALSE;
  ULONG i;

  MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, BaseAddress);
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
	  PVOID Address = BaseAddress + (i * PAGE_SIZE);
	  ULONG Protect = NewProtect;

	  /* 
	   * If we doing COW for this segment then check if the page is
	   * already private.
	   */
	  if (DoCOW && MmIsPagePresent(AddressSpace->Process, Address))
	    {
	      LARGE_INTEGER Offset;
	      ULONG Entry;
	      LARGE_INTEGER PhysicalAddress;

	      Offset.QuadPart = 
		(ULONG)(Address - (ULONG)MemoryArea->BaseAddress) +
		MemoryArea->Data.SectionData.ViewOffset;
	      Entry = MmGetPageEntrySectionSegment(Segment, Offset.QuadPart);
	      PhysicalAddress = 
		MmGetPhysicalAddressForProcess(AddressSpace->Process, Address);

	      Protect = PAGE_READONLY;
	      if ((Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
		   IS_SWAP_FROM_SSE(Entry) || 
		   (LONGLONG)PAGE_FROM_SSE(Entry) != PhysicalAddress.QuadPart))
		{
		  Protect = NewProtect;
		}
	    }

	  if (MmIsPagePresent(AddressSpace->Process, Address))
	    {
	      MmSetPageProtect(AddressSpace->Process, BaseAddress, 
			       Protect);
	    }
	}
    }
}

NTSTATUS
MmProtectSectionView(PMADDRESS_SPACE AddressSpace,
		     PMEMORY_AREA MemoryArea,
		     PVOID BaseAddress,
		     ULONG Length,
		     ULONG Protect,
		     PULONG OldProtect)
{
  PMM_REGION Region;
  NTSTATUS Status;

  Length = 
    min(Length, (ULONG) (MemoryArea->BaseAddress + MemoryArea->Length - BaseAddress));
  Region = MmFindRegion(MemoryArea->BaseAddress,
			&MemoryArea->Data.SectionData.RegionListHead,
			BaseAddress, NULL);
  *OldProtect = Region->Protect;
  Status = MmAlterRegion(AddressSpace, MemoryArea->BaseAddress,
			 &MemoryArea->Data.SectionData.RegionListHead,
			 BaseAddress, Length, Region->Type, Protect,
			 MmAlterViewAttributes);

  return(Status);
}

NTSTATUS STDCALL
MmQuerySectionView(PMEMORY_AREA MemoryArea,
		   PVOID Address,
		   PMEMORY_BASIC_INFORMATION Info,
		   PULONG ResultLength)
{
  PMM_REGION Region;
  PVOID RegionBaseAddress;
  
  Region = MmFindRegion(MemoryArea->BaseAddress,
			&MemoryArea->Data.SectionData.RegionListHead,
			Address, &RegionBaseAddress);
  Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);
  Info->AllocationBase = MemoryArea->BaseAddress;
  Info->AllocationProtect = MemoryArea->Attributes;
  Info->RegionSize = MemoryArea->Length;
  Info->State = MEM_COMMIT;
  Info->Protect = Region->Protect;
  if (MemoryArea->Data.SectionData.Section->AllocationAttributes & SEC_IMAGE)
    {
      Info->Type = MEM_IMAGE;
    }
  else
    {
      Info->Type = MEM_MAPPED;
    }

  return(STATUS_SUCCESS);
}

VOID STDCALL
MmpDeleteSection(PVOID ObjectBody)
{
  PSECTION_OBJECT Section = (PSECTION_OBJECT)ObjectBody;

  DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
  if (Section->AllocationAttributes & SEC_IMAGE)
    {
      ULONG i;
      
      for (i = 0; i < Section->NrSegments; i++)
	{
	  InterlockedDecrement((LONG *)&Section->Segments[i].ReferenceCount);
	}
    }
  else
    {
      InterlockedDecrement((LONG *)&Section->Segments->ReferenceCount);
    }
  if (Section->FileObject != NULL)
    {
      CcRosDereferenceCache(Section->FileObject);
      ObDereferenceObject(Section->FileObject);
      Section->FileObject = NULL;
    }

  if (Section->Segments->Flags & MM_PAGEFILE_SEGMENT)
  {
     MmFreePageTablesSectionSegment(Section->Segments);
     ExFreePool(Section->Segments);
  }
}

VOID STDCALL
MmpCloseSection(PVOID ObjectBody,
		ULONG HandleCount)
{
   DPRINT("MmpCloseSection(OB %x, HC %d) RC %d\n",
	   ObjectBody, HandleCount, ObGetObjectPointerCount(ObjectBody));
   
}

NTSTATUS STDCALL
MmpCreateSection(PVOID ObjectBody,
		 PVOID Parent,
		 PWSTR RemainingPath,
		 POBJECT_ATTRIBUTES ObjectAttributes)
{
   DPRINT("MmpCreateSection(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath == NULL)
     {
	return(STATUS_SUCCESS);
     }
   
   if (wcschr(RemainingPath+1, L'\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS 
MmCreatePhysicalMemorySection(VOID)
{
  HANDLE PhysSectionH;
  PSECTION_OBJECT PhysSection;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES Obj;
  UNICODE_STRING Name = UNICODE_STRING_INITIALIZER(L"\\Device\\PhysicalMemory");
  LARGE_INTEGER SectionSize;
   
  /*
   * Create the section mapping physical memory 
   */
  SectionSize.QuadPart = 0xFFFFFFFF;
  InitializeObjectAttributes(&Obj,
			     &Name,
			     0,
			     NULL,
			     NULL);
  Status = NtCreateSection(&PhysSectionH,
			   SECTION_ALL_ACCESS,
			   &Obj,
			   &SectionSize,
			   PAGE_EXECUTE_READWRITE,
			   0,
			   NULL);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to create PhysicalMemory section\n");
      KeBugCheck(0);
    }
  Status = ObReferenceObjectByHandle(PhysSectionH,
				     SECTION_ALL_ACCESS,
				     NULL,
				     KernelMode,
				     (PVOID*)&PhysSection,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to reference PhysicalMemory section\n");
      KeBugCheck(0);
    }
  PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;
  ObDereferenceObject((PVOID)PhysSection);
   
  return(STATUS_SUCCESS);
}

NTSTATUS 
MmInitSectionImplementation(VOID)
{
   MmSectionObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeStringFromLiteral(&MmSectionObjectType->TypeName, L"Section");
   
   MmSectionObjectType->Tag = TAG('S', 'E', 'C', 'T');
   MmSectionObjectType->TotalObjects = 0;
   MmSectionObjectType->TotalHandles = 0;
   MmSectionObjectType->MaxObjects = ULONG_MAX;
   MmSectionObjectType->MaxHandles = ULONG_MAX;
   MmSectionObjectType->PagedPoolCharge = 0;
   MmSectionObjectType->NonpagedPoolCharge = sizeof(SECTION_OBJECT);
   MmSectionObjectType->Mapping = &MmpSectionMapping;
   MmSectionObjectType->Dump = NULL;
   MmSectionObjectType->Open = NULL;
   MmSectionObjectType->Close = MmpCloseSection;
   MmSectionObjectType->Delete = MmpDeleteSection;
   MmSectionObjectType->Parse = NULL;
   MmSectionObjectType->Security = NULL;
   MmSectionObjectType->QueryName = NULL;
   MmSectionObjectType->OkayToClose = NULL;
   MmSectionObjectType->Create = MmpCreateSection;
   MmSectionObjectType->DuplicationNotify = NULL;
   
   return(STATUS_SUCCESS);
}

NTSTATUS
MmCreatePageFileSection(PHANDLE SectionHandle,
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
  PSECTION_OBJECT Section;
  PMM_SECTION_SEGMENT Segment;
  NTSTATUS Status;

  if (UMaximumSize == NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  MaximumSize = *UMaximumSize;
  
  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) != 
      SectionPageProtection)
    {
      return(STATUS_INVALID_PAGE_PROTECTION);
    }
  
  /*
   * Create the section
   */
  Status = ObCreateObject(SectionHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  MmSectionObjectType,
			  (PVOID*)&Section);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocationAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->FileObject = NULL;
  Section->MaximumSize = MaximumSize;
  Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
				  TAG_MM_SECTION_SEGMENT);
  if (Segment == NULL)
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      return(STATUS_NO_MEMORY);
    }
  Section->Segments = Segment;
  Segment->ReferenceCount = 1;
  KeInitializeMutex(&Segment->Lock, 0);
  Segment->FileOffset = 0;
  Segment->Protection = SectionPageProtection;
  Segment->Attributes = AllocationAttributes;
  Segment->Length = MaximumSize.u.LowPart;
  Segment->Flags = MM_PAGEFILE_SEGMENT;
  Segment->WriteCopy = FALSE;
  ObDereferenceObject(Section);
  return(STATUS_SUCCESS);
}


NTSTATUS
MmCreateDataFileSection(PHANDLE SectionHandle,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			PLARGE_INTEGER UMaximumSize,
			ULONG SectionPageProtection,
			ULONG AllocationAttributes,
			HANDLE FileHandle)
     /*
      * Create a section backed by a data file
      */
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;
  LARGE_INTEGER MaximumSize;
  PFILE_OBJECT FileObject;
  PMM_SECTION_SEGMENT Segment;
  ULONG FileAccess;
  IO_STATUS_BLOCK Iosb;
  LARGE_INTEGER Offset;
  CHAR Buffer;
  
  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) != 
      SectionPageProtection)
    {
      return(STATUS_INVALID_PAGE_PROTECTION);
    }

  /*
   * Read a bit so caching is initiated for the file object.
   * This is only needed because MiReadPage currently cannot
   * handle non-cached streams.
   */
  Offset.QuadPart = 0;
  Status = ZwReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
		      &Buffer,
		      sizeof (Buffer),
		      &Offset,
		      0);
  if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
      return(Status);
    }

  /*
   * Create the section
   */
  Status = ObCreateObject(SectionHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  MmSectionObjectType,
			  (PVOID*)&Section);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocationAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->NrSegments = 1;
  Section->ImageBase = NULL;
  Section->EntryPoint = NULL;
  Section->StackReserve = 0;
  Section->StackCommit = 0;
  Section->Subsystem = 0;
  Section->MinorSubsystemVersion = 0;
  Section->MajorSubsystemVersion = 0;
  Section->ImageCharacteristics = 0;
  Section->Machine = 0;
  Section->Executable = FALSE;

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
  Status = ObReferenceObjectByHandle(FileHandle,
				     FileAccess,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      return(Status);
    }

  /*
   * We can't do memory mappings if the file system doesn't support the
   * standard FCB
   */
  if (!(FileObject->Flags & FO_FCB_IS_VALID))
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      ObDereferenceObject(FileObject);
      return(STATUS_INVALID_FILE_FOR_SECTION);
    }
  
  /*
   * FIXME: Revise this once a locking order for file size changes is
   * decided
   */
  if (UMaximumSize != NULL)
    {
      MaximumSize = *UMaximumSize;
    }
  else
    {
      MaximumSize = 
	((PREACTOS_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize;
    }

  if (MaximumSize.QuadPart > 
      ((PREACTOS_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize.QuadPart)
    {
      IO_STATUS_BLOCK Iosb;
      Status = NtSetInformationFile(FileHandle,
				    &Iosb,
				    &MaximumSize,
				    sizeof(LARGE_INTEGER),
				    FileAllocationInformation);
      if (!NT_SUCCESS(Status))
	{
	  ZwClose(*SectionHandle);
	  ObDereferenceObject(Section);
	  ObDereferenceObject(FileObject);
	  return(STATUS_SECTION_NOT_EXTENDED);
	}
    }

  /*
   * Lock the file
   */
  Status = KeWaitForSingleObject((PVOID)&FileObject->Lock,
				 0,
				 KernelMode,
				 FALSE,
				 NULL);
  if (Status != STATUS_SUCCESS)
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      ObDereferenceObject(FileObject);
      return(Status);
    }

  /*
   * If this file hasn't been mapped as a data file before then allocate a
   * section segment to describe the data file mapping
   */
  if (FileObject->SectionObjectPointers->DataSectionObject == NULL)
    {
      Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
				      TAG_MM_SECTION_SEGMENT);
      if (Segment == NULL)
	{
	  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose(*SectionHandle);
	  ObDereferenceObject(Section);
	  ObDereferenceObject(FileObject);
	  return(STATUS_NO_MEMORY);
	}
      Section->Segments = Segment;
      Segment->ReferenceCount = 1;
      KeInitializeMutex(&Segment->Lock, 0);

      /*
       * Set the lock before assigning the segment to the file object
       */
      Status = KeWaitForSingleObject((PVOID)&Segment->Lock,
				     0,
				     KernelMode,
				     FALSE,
				     NULL);
      if (Status != STATUS_SUCCESS)
	{
	  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ExFreePool(Segment);
	  ZwClose(*SectionHandle);
	  ObDereferenceObject(Section);
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
      FileObject->SectionObjectPointers->DataSectionObject = (PVOID)Segment;

      Segment->FileOffset = 0;
      Segment->Protection = 0;
      Segment->Attributes = 0;
      Segment->Flags = MM_DATAFILE_SEGMENT;
      Segment->Characteristics = 0;
      Segment->WriteCopy = FALSE;
      if (AllocationAttributes & SEC_RESERVE)
	{
	  Segment->Length = 0;
	}
      else
	{
	  Segment->Length = MaximumSize.u.LowPart;
	}
      Segment->VirtualAddress = NULL;
    }
  else
    {
      /*
       * If the file is already mapped as a data file then we may need
       * to extend it
       */      
      Segment = 
	(PMM_SECTION_SEGMENT)FileObject->SectionObjectPointers->
	DataSectionObject;
      Section->Segments = Segment;
      InterlockedIncrement((PLONG)&Segment->ReferenceCount);
      Status = KeWaitForSingleObject((PVOID)&Segment->Lock,
				     0,
				     KernelMode,
				     FALSE,
				     NULL);
      if (Status != STATUS_SUCCESS)
	{
	  InterlockedDecrement((PLONG)&Segment->ReferenceCount);
	  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose(*SectionHandle);
	  ObDereferenceObject(Section);
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
      if (MaximumSize.u.LowPart > Segment->Length && 
	  !(AllocationAttributes & SEC_RESERVE))
	{
	  Segment->Length = MaximumSize.u.LowPart;
	}
    }
  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
  CcRosReferenceCache(FileObject);
  Section->FileObject = FileObject; 
  Section->MaximumSize = MaximumSize;
  KeReleaseMutex(&Segment->Lock, FALSE);

  ObDereferenceObject(Section);
  return(STATUS_SUCCESS);
}

static ULONG SectionCharacteristicsToProtect[16] = 
{
  PAGE_NOACCESS,               // 0 = NONE
  PAGE_NOACCESS,               // 1 = SHARED
  PAGE_EXECUTE,                // 2 = EXECUTABLE
  PAGE_EXECUTE,                // 3 = EXECUTABLE, SHARED
  PAGE_READONLY,               // 4 = READABLE
  PAGE_READONLY,               // 5 = READABLE, SHARED
  PAGE_EXECUTE_READ,           // 6 = READABLE, EXECUTABLE
  PAGE_EXECUTE_READ,           // 7 = READABLE, EXECUTABLE, SHARED
  PAGE_READWRITE,              // 8 = WRITABLE
  PAGE_READWRITE,              // 9 = WRITABLE, SHARED
  PAGE_EXECUTE_READWRITE,      // 10 = WRITABLE, EXECUTABLE
  PAGE_EXECUTE_READWRITE,      // 11 = WRITABLE, EXECUTABLE, SHARED
  PAGE_READWRITE,              // 12 = WRITABLE, READABLE
  PAGE_READWRITE,              // 13 = WRITABLE, READABLE, SHARED
  PAGE_EXECUTE_READWRITE,      // 14 = WRITABLE, READABLE, EXECUTABLE,
  PAGE_EXECUTE_READWRITE,      // 15 = WRITABLE, READABLE, EXECUTABLE, SHARED
};

NTSTATUS
MmCreateImageSection(PHANDLE SectionHandle,
		     ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes,
		     PLARGE_INTEGER UMaximumSize,
		     ULONG SectionPageProtection,
		     ULONG AllocationAttributes,
		     HANDLE FileHandle)
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  ULONG FileAccess;
  IMAGE_DOS_HEADER DosHeader;
  IO_STATUS_BLOCK Iosb;
  LARGE_INTEGER Offset;
  IMAGE_NT_HEADERS PEHeader;
  PIMAGE_SECTION_HEADER ImageSections;
  PMM_SECTION_SEGMENT SectionSegments;
  ULONG NrSegments;
  PMM_IMAGE_SECTION_OBJECT ImageSectionObject;

  /*
   * Check the protection
   */
  if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) != 
      SectionPageProtection)
    {
      return(STATUS_INVALID_PAGE_PROTECTION);
    }
  
  /*
   * Specifying a maximum size is meaningless for an image section
   */
  if (UMaximumSize != NULL)
    {
      return(STATUS_INVALID_PARAMETER_4);
    } 

  /*
   * Read the dos header
   */
  Offset.QuadPart = 0;
  Status = ZwReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
		      &DosHeader,
		      sizeof(DosHeader),
		      &Offset,
		      0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (Iosb.Information != sizeof(DosHeader))
    {
      return(STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Check the DOS signature
   */
  if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
      return(STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Read the PE header
   */
  Offset.QuadPart = DosHeader.e_lfanew;
  Status = ZwReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
		      &PEHeader,
		      sizeof(PEHeader),
		      &Offset,
		      0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  if (Iosb.Information != sizeof(PEHeader))
    {
      return(STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Check the signature
   */
  if (PEHeader.Signature != IMAGE_NT_SIGNATURE)
    {
      return(STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Read in the section headers
   */
  Offset.QuadPart = DosHeader.e_lfanew + sizeof(PEHeader);  
  ImageSections = 
    ExAllocatePool(NonPagedPool,
		   PEHeader.FileHeader.NumberOfSections * 
		   sizeof(IMAGE_SECTION_HEADER));
  Status = ZwReadFile(FileHandle,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
		      ImageSections,
		      PEHeader.FileHeader.NumberOfSections * 
		      sizeof(IMAGE_SECTION_HEADER),
		      &Offset,
		      0);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(ImageSections);
      return(Status);
    }
  if (Iosb.Information != 
      (PEHeader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)))
    {
      ExFreePool(ImageSections);
      return(STATUS_INVALID_IMAGE_FORMAT);
    }

  /*
   * Create the section
   */
  Status = ObCreateObject(SectionHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  MmSectionObjectType,
			  (PVOID*)&Section);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(ImageSections);
      return(Status);
    }

  /*
   * Initialize it
   */
  Section->SectionPageProtection = SectionPageProtection;
  Section->AllocationAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->NrSegments = PEHeader.FileHeader.NumberOfSections + 1;
  Section->ImageBase = (PVOID)PEHeader.OptionalHeader.ImageBase;
  Section->EntryPoint = (PVOID)PEHeader.OptionalHeader.AddressOfEntryPoint;
  Section->StackReserve = PEHeader.OptionalHeader.SizeOfStackReserve;
  Section->StackCommit = PEHeader.OptionalHeader.SizeOfStackCommit;
  Section->Subsystem = PEHeader.OptionalHeader.Subsystem;
  Section->MinorSubsystemVersion = 
    PEHeader.OptionalHeader.MinorSubsystemVersion;
  Section->MajorSubsystemVersion = 
    PEHeader.OptionalHeader.MajorSubsystemVersion;
  Section->ImageCharacteristics = PEHeader.FileHeader.Characteristics;
  Section->Machine = PEHeader.FileHeader.Machine;
  Section->Executable = (PEHeader.OptionalHeader.SizeOfCode != 0); 

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
  Status = ObReferenceObjectByHandle(FileHandle,
				     FileAccess,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      ExFreePool(ImageSections);
      return(Status);
    }

  /*
   * We can't do memory mappings if the file system doesn't support the
   * standard FCB
   */
  if (!(FileObject->Flags & FO_FCB_IS_VALID))
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      ObDereferenceObject(FileObject);
      ExFreePool(ImageSections);
      return(STATUS_INVALID_FILE_FOR_SECTION);
    }
  
  /*
   * Lock the file
   */
  Status = KeWaitForSingleObject((PVOID)&FileObject->Lock,
				 0,
				 KernelMode,
				 FALSE,
				 NULL);
  if (Status != STATUS_SUCCESS)
    {
      ZwClose(*SectionHandle);
      ObDereferenceObject(Section);
      ObDereferenceObject(FileObject);
      ExFreePool(ImageSections);
      return(Status);
    }

  /*
   * If this file hasn't been mapped as a image file before then allocate the
   * section segments to describe the mapping
   */
  NrSegments = PEHeader.FileHeader.NumberOfSections + 1;
  if (FileObject->SectionObjectPointers->ImageSectionObject == NULL)
    {
      ULONG i;
      ULONG Size;
      ULONG Characteristics;

      Size = sizeof(MM_IMAGE_SECTION_OBJECT) + 
	(sizeof(MM_SECTION_SEGMENT) * NrSegments);
      ImageSectionObject = 
	ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
	{
	  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
	  ZwClose(*SectionHandle);
	  ObDereferenceObject(Section);
	  ObDereferenceObject(FileObject);
	  ExFreePool(ImageSections);
	  return(STATUS_NO_MEMORY);
	}
      ImageSectionObject->NrSegments = NrSegments;
      SectionSegments = ImageSectionObject->Segments;
      Section->Segments = SectionSegments;

      SectionSegments[0].FileOffset = 0;
      SectionSegments[0].Characteristics = IMAGE_SECTION_CHAR_DATA;
      SectionSegments[0].Protection = PAGE_READWRITE;
      SectionSegments[0].RawLength = PAGE_SIZE;
      SectionSegments[0].Length = PAGE_SIZE;
      SectionSegments[0].Flags = 0;
      SectionSegments[0].ReferenceCount = 1;
      SectionSegments[0].VirtualAddress = 0;
      SectionSegments[0].WriteCopy = TRUE;
      KeInitializeMutex(&SectionSegments[0].Lock, 0);

      for (i = 1; i < NrSegments; i++)
	{
	  SectionSegments[i].FileOffset = 
	    ImageSections[i-1].PointerToRawData;
	  SectionSegments[i].Characteristics = 
	    ImageSections[i-1].Characteristics;

	  /*
	   * Set up the protection and write copy variables.
	   */
	  Characteristics = ImageSections[i - 1].Characteristics;
	  if ((Characteristics & IMAGE_SECTION_CHAR_READABLE) ||
	      (Characteristics & IMAGE_SECTION_CHAR_WRITABLE) ||
	      (Characteristics & IMAGE_SECTION_CHAR_EXECUTABLE))
	    {
	      SectionSegments[i].Protection = 
		SectionCharacteristicsToProtect[Characteristics >> 28];
	      SectionSegments[i].WriteCopy = 
		!(Characteristics & IMAGE_SECTION_CHAR_SHARED);
	    }
	  else if (Characteristics & IMAGE_SECTION_CHAR_CODE)	    
	    {
	      SectionSegments[i].Protection = PAGE_EXECUTE_READ;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else if (Characteristics & IMAGE_SECTION_CHAR_DATA)	    
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else if (Characteristics & IMAGE_SECTION_CHAR_BSS)
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  else
	    {
	      SectionSegments[i].Protection = PAGE_NOACCESS;
	      SectionSegments[i].WriteCopy = TRUE;
	    }
	  
	  /*
	   * Set up the attributes.
	   */
	  if (Characteristics & IMAGE_SECTION_CHAR_CODE)	    
	    {
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (Characteristics & IMAGE_SECTION_CHAR_DATA)	    
	    {
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (Characteristics & IMAGE_SECTION_CHAR_BSS)
	    {
	      SectionSegments[i].Attributes = MM_SECTION_SEGMENT_BSS;
	    }
	  else
	    {
	      SectionSegments[i].Attributes = 0;
	    }

	  SectionSegments[i].RawLength = ImageSections[i-1].SizeOfRawData;
	  SectionSegments[i].Length = 
	    ImageSections[i-1].Misc.VirtualSize;
	  SectionSegments[i].Flags = 0;
	  SectionSegments[i].ReferenceCount = 1;
	  SectionSegments[i].VirtualAddress = 
	    (PVOID)ImageSections[i-1].VirtualAddress;
	  KeInitializeMutex(&SectionSegments[i].Lock, 0);
	}

      FileObject->SectionObjectPointers->ImageSectionObject = 
	(PVOID)ImageSectionObject;       
    }
  else
    {
      ULONG i;

      ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)
	FileObject->SectionObjectPointers->ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;
      Section->Segments = SectionSegments;

      /*
       * Otherwise just reference all the section segments
       */
      for (i = 0; i < NrSegments; i++)
	{
	  InterlockedIncrement((LONG *)&SectionSegments[i].ReferenceCount);
	}

    }
  ExFreePool(ImageSections);
  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
  CcRosReferenceCache(FileObject);
  Section->FileObject = FileObject;

  ObDereferenceObject(Section);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtCreateSection (OUT PHANDLE SectionHandle,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
		 IN PLARGE_INTEGER MaximumSize	OPTIONAL,
		 IN ULONG SectionPageProtection OPTIONAL,
		 IN ULONG AllocationAttributes,
		 IN HANDLE FileHandle OPTIONAL)
{
  if (AllocationAttributes & SEC_IMAGE)
    {
      return(MmCreateImageSection(SectionHandle,
				  DesiredAccess,
				  ObjectAttributes,
				  MaximumSize,
				  SectionPageProtection,
				  AllocationAttributes,
				  FileHandle));
    }
  else if (FileHandle != NULL)
    {
      return(MmCreateDataFileSection(SectionHandle,
				     DesiredAccess,
				     ObjectAttributes,
				     MaximumSize,
				     SectionPageProtection,
				     AllocationAttributes,
				     FileHandle));
    }
  else
    {
      return(MmCreatePageFileSection(SectionHandle,
				     DesiredAccess,
				     ObjectAttributes,
				     MaximumSize,
				     SectionPageProtection,
				     AllocationAttributes));
    }
}


/**********************************************************************
 * NAME
 * 	NtOpenSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	SectionHandle
 *
 * 	DesiredAccess
 *
 * 	ObjectAttributes
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtOpenSection(PHANDLE			SectionHandle,
	      ACCESS_MASK		DesiredAccess,
	      POBJECT_ATTRIBUTES	ObjectAttributes)
{
   NTSTATUS Status;
   
   *SectionHandle = 0;

   Status = ObOpenObjectByName(ObjectAttributes,
			       MmSectionObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       SectionHandle);

   return(Status);
}

NTSTATUS STATIC
MmMapViewOfSegment(PEPROCESS Process,
		   PMADDRESS_SPACE AddressSpace,
		   PSECTION_OBJECT Section,
		   PMM_SECTION_SEGMENT Segment,
		   PVOID* BaseAddress,
		   ULONG ViewSize,
		   ULONG Protect,
		   ULONG ViewOffset)
{
  PMEMORY_AREA MArea;
  NTSTATUS Status;
  KIRQL oldIrql;

  Status = MmCreateMemoryArea(Process,
			      &Process->AddressSpace,
			      MEMORY_AREA_SECTION_VIEW,
			      BaseAddress,
			      ViewSize,
			      Protect,
			      &MArea,
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Mapping between 0x%.8X and 0x%.8X failed.\n",
	      (*BaseAddress), (*BaseAddress) + ViewSize);
      return(Status);
    }
  
  KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
  InsertTailList(&Section->ViewListHead, 
		 &MArea->Data.SectionData.ViewListEntry);
  KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
  
  ObReferenceObjectByPointer((PVOID)Section,
			     SECTION_MAP_READ,
			     NULL,
			     ExGetPreviousMode());
  MArea->Data.SectionData.Segment = Segment;
  MArea->Data.SectionData.Section = Section;
  MArea->Data.SectionData.ViewOffset = ViewOffset;
  MArea->Data.SectionData.WriteCopyView = FALSE;
  MmInitialiseRegion(&MArea->Data.SectionData.RegionListHead,
		     ViewSize, 0, Protect);
  
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtMapViewOfSection
 *
 * DESCRIPTION
 *	Maps a view of a section into the virtual address space of a 
 *	process.
 *	
 * ARGUMENTS
 *	SectionHandle
 *		Handle of the section.
 *		
 *	ProcessHandle
 *		Handle of the process.
 *		
 *	BaseAddress
 *		Desired base address (or NULL) on entry;
 *		Actual base address of the view on exit.
 *		
 *	ZeroBits
 *		Number of high order address bits that must be zero.
 *		
 *	CommitSize
 *		Size in bytes of the initially committed section of 
 *		the view.
 *		
 *	SectionOffset
 *		Offset in bytes from the beginning of the section
 *		to the beginning of the view.
 *		
 *	ViewSize
 *		Desired length of map (or zero to map all) on entry
 *		Actual length mapped on exit.
 *		
 *	InheritDisposition
 *		Specified how the view is to be shared with
 *		child processes.
 *		
 *	AllocateType
 *		Type of allocation for the pages.
 *		
 *	Protect
 *		Protection for the committed region of the view.
 *
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL
NtMapViewOfSection(HANDLE SectionHandle,
		   HANDLE ProcessHandle,
		   PVOID* BaseAddress,
		   ULONG ZeroBits,
		   ULONG CommitSize,
		   PLARGE_INTEGER SectionOffset,
		   PULONG ViewSize,
		   SECTION_INHERIT InheritDisposition,
		   ULONG AllocationType,
		   ULONG Protect)
{
   PSECTION_OBJECT Section;
   PEPROCESS Process;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;

   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   AddressSpace = &Process->AddressSpace;
   
   Status = ObReferenceObjectByHandle(SectionHandle,
				      SECTION_MAP_READ,
				      MmSectionObjectType,
				      UserMode,
				      (PVOID*)&Section,
				      NULL);
   if (!(NT_SUCCESS(Status)))
     {
	DPRINT("ObReference failed rc=%x\n",Status);
	ObDereferenceObject(Process);
	return(Status);
     }
   
   Status = MmMapViewOfSection(Section,
			       Process,
			       BaseAddress,
			       ZeroBits,
			       CommitSize,
			       SectionOffset,
			       ViewSize,
			       InheritDisposition,
			       AllocationType,
			       Protect);
   
   ObDereferenceObject(Section);
   ObDereferenceObject(Process);
   
   return(Status);
}

VOID STATIC
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
		  PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, 
		  BOOLEAN Dirty)
{
  PMEMORY_AREA MArea;
  ULONG Entry;
  PFILE_OBJECT FileObject;
  PBCB Bcb;
  ULONG Offset;
  SWAPENTRY SavedSwapEntry;
  PMM_PAGEOP PageOp;

  MArea = (PMEMORY_AREA)Context;

  Offset = ((ULONG)PAGE_ROUND_DOWN(Address) - (ULONG)MArea->BaseAddress) + 
    MArea->Data.SectionData.ViewOffset;
  Entry = MmGetPageEntrySectionSegment(MArea->Data.SectionData.Segment,
				       Offset);

  PageOp = MmCheckForPageOp(MArea, 0, NULL, MArea->Data.SectionData.Segment,
			    Offset);
  
  if (PageOp)
  {  
     KeWaitForSingleObject(&PageOp->CompletionEvent,
			   0,
			   KernelMode,
			   FALSE,
			   NULL);
     MmReleasePageOp(PageOp);
     PageOp = MmCheckForPageOp(MArea, 0, NULL, MArea->Data.SectionData.Segment,
			       Offset);
  }
  
  assert(PageOp == NULL);

  /*
   * For a dirty, datafile, non-private page mark it as dirty in the
   * cache manager.
   */
  if (MArea->Data.SectionData.Segment->Flags & MM_DATAFILE_SEGMENT)
    {
      if (PhysAddr.QuadPart == PAGE_FROM_SSE(Entry) && Dirty)
	{
	  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
	  Bcb = FileObject->SectionObjectPointers->SharedCacheMap;
	  CcRosMarkDirtyCacheSegment(Bcb, Offset);
	  assert(SwapEntry == 0);
	}
    }

  if (SwapEntry != 0)
    {
      MmFreeSwapPage(SwapEntry);
    }
  else if (PhysAddr.QuadPart != 0)
    {      
      if (IS_SWAP_FROM_SSE(Entry) || 
	  PhysAddr.QuadPart != (PAGE_FROM_SSE(Entry)))
	{
	  /*
	   * Just dereference private pages
	   */
	  SavedSwapEntry = MmGetSavedSwapEntryPage(PhysAddr);
	  if (SavedSwapEntry != 0)
	    {
	      MmFreeSwapPage(SavedSwapEntry);
	      MmSetSavedSwapEntryPage(PhysAddr, 0);
	    }
	}
      else
	{
	  MmUnsharePageEntrySectionSegment(MArea->Data.SectionData.Section,
					   MArea->Data.SectionData.Segment,
					   Offset,
					   Dirty);
	}
      MmDeleteRmap(PhysAddr, MArea->Process, Address);
      MmReleasePageMemoryConsumer(MC_USER, PhysAddr);
    }
}

NTSTATUS STDCALL
MmUnmapViewOfSection(PEPROCESS Process,
		     PVOID BaseAddress)
{
   NTSTATUS	Status;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   KIRQL oldIrql;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   
   AddressSpace = &Process->AddressSpace;
   
   DPRINT("Opening memory area Process %x BaseAddress %x\n",
	  Process, BaseAddress);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }

   MemoryArea->DeleteInProgress = TRUE;
   
   MmLockSection(MemoryArea->Data.SectionData.Section);
   MmLockSectionSegment(MemoryArea->Data.SectionData.Segment);
   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   CurrentEntry = MemoryArea->Data.SectionData.RegionListHead.Flink;
   while (CurrentEntry != &MemoryArea->Data.SectionData.RegionListHead)
     {
       CurrentRegion = 
	 CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
       CurrentEntry = CurrentEntry->Flink;
       ExFreePool(CurrentRegion);
     }

   if (MemoryArea->Data.SectionData.Section->AllocationAttributes & 
       SEC_PHYSICALMEMORY)
     {
       Status = MmFreeMemoryArea(&Process->AddressSpace,
				 BaseAddress,
				 0,
				 NULL,
				 NULL);
     }
   else
     {
       Status = MmFreeMemoryArea(&Process->AddressSpace,
				 BaseAddress,
				 0,
				 MmFreeSectionPage,
				 MemoryArea);
      }
   MmUnlockSection(Section);
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 *	NtUnmapViewOfSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ProcessHandle
 *
 *	BaseAddress
 *
 * RETURN VALUE
 *	Status.
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtUnmapViewOfSection (HANDLE	ProcessHandle,
		      PVOID	BaseAddress)
{
   PEPROCESS	Process;
   NTSTATUS Status;
   
   DPRINT("NtUnmapViewOfSection(ProcessHandle %x, BaseAddress %x)\n",
	   ProcessHandle, BaseAddress);
   
   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObReferenceObjectByHandle failed (Status %x)\n", Status);
	return(Status);
     }

   MmLockAddressSpace(&Process->AddressSpace);
   Status = MmUnmapViewOfSection(Process, BaseAddress);
   MmUnlockAddressSpace(&Process->AddressSpace);

   ObDereferenceObject(Process);

   return Status;
}


NTSTATUS STDCALL
NtQuerySection (IN	HANDLE	SectionHandle,
		IN	CINT	SectionInformationClass,
		OUT	PVOID	SectionInformation,
		IN	ULONG	Length, 
		OUT	PULONG	ResultLength)
/*
 * FUNCTION: Queries the information of a section object.
 * ARGUMENTS: 
 *        SectionHandle = Handle to the section link object
 *	  SectionInformationClass = Index to a certain information structure
 *        SectionInformation (OUT)= Caller supplies storage for resulting 
 *                                  information
 *        Length =  Size of the supplied storage 
 *        ResultLength = Data written
 * RETURNS: Status
 *
 */
{
  PSECTION_OBJECT Section;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(SectionHandle,
				     SECTION_MAP_READ,
				     MmSectionObjectType,
				     UserMode,
				     (PVOID*)&Section,
				     NULL);
  if (!(NT_SUCCESS(Status)))
    {
      return(Status);
    }

  switch (SectionInformationClass)
    {
    case SectionBasicInformation:
      {
	PSECTION_BASIC_INFORMATION Sbi;

	if (Length != sizeof(SECTION_BASIC_INFORMATION))
	  {
	    ObDereferenceObject(Section);
	    return(STATUS_INFO_LENGTH_MISMATCH);
	  }

	Sbi = (PSECTION_BASIC_INFORMATION)SectionInformation;
	
	Sbi->BaseAddress = 0;
	Sbi->Attributes = 0;
	Sbi->Size.QuadPart = 0;

	*ResultLength = sizeof(SECTION_BASIC_INFORMATION);
	Status = STATUS_SUCCESS;
	break;
      }

      case SectionImageInformation:
	{
	  PSECTION_IMAGE_INFORMATION Sii;

	  if (Length != sizeof(SECTION_IMAGE_INFORMATION))
	    {
	      ObDereferenceObject(Section);
	      return(STATUS_INFO_LENGTH_MISMATCH);
	    }
	  Sii = (PSECTION_IMAGE_INFORMATION)SectionInformation; 
	  Sii->EntryPoint = Section->EntryPoint;
	  Sii->Unknown1 = 0;
	  Sii->StackReserve = Section->StackReserve;
	  Sii->StackCommit = Section->StackCommit;
	  Sii->Subsystem = Section->Subsystem;
	  Sii->MinorSubsystemVersion = Section->MinorSubsystemVersion;
	  Sii->MajorSubsystemVersion = Section->MajorSubsystemVersion;
	  Sii->Unknown2 = 0;
	  Sii->Characteristics = Section->ImageCharacteristics;
	  Sii->ImageNumber = Section->Machine;
	  Sii->Executable = Section->Executable;
	  Sii->Unknown3 = 0;
	  Sii->Unknown4[0] = 0;
	  Sii->Unknown4[1] = 0;
	  Sii->Unknown4[2] = 0;

	  *ResultLength = sizeof(SECTION_IMAGE_INFORMATION);
	  Status = STATUS_SUCCESS;
	  break;
	}

    default:
      *ResultLength = 0;
      Status = STATUS_INVALID_INFO_CLASS;
    }
  ObDereferenceObject(Section);
  return(Status);
}


NTSTATUS STDCALL 
NtExtendSection(IN	HANDLE	SectionHandle,
		IN	ULONG	NewMaximumSize)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							INTERNAL
 * 	MmAllocateSection@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	Length
 *
 * RETURN VALUE
 *
 * NOTE
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL 
MmAllocateSection (IN ULONG Length)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("MmAllocateSection(Length %x)\n",Length);
   
   AddressSpace = MmGetKernelAddressSpace();
   Result = NULL;
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateMemoryArea (NULL,
				AddressSpace,
				MEMORY_AREA_SYSTEM,
				&Result,
				Length,
				0,
				&marea,
				FALSE);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(AddressSpace);
	return (NULL);
     }
   MmUnlockAddressSpace(AddressSpace);
   DPRINT("Result %p\n",Result);
   for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
     {
       PHYSICAL_ADDRESS Page;

       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("Unable to allocate page\n");
	   KeBugCheck(0);
	 }
       Status = MmCreateVirtualMapping (NULL,
					(Result + (i * PAGE_SIZE)),
					PAGE_READWRITE,
					Page,
					TRUE);
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("Unable to create virtual mapping\n");	  
	   KeBugCheck(0);
	 }
     }
   return ((PVOID)Result);
}


/**********************************************************************
 * NAME							EXPORTED
 *	MmMapViewOfSection
 *
 * DESCRIPTION
 *	Maps a view of a section into the virtual address space of a 
 *	process.
 *	
 * ARGUMENTS
 *	Section
 *		Pointer to the section object.
 *		
 *	ProcessHandle
 *		Pointer to the process.
 *		
 *	BaseAddress
 *		Desired base address (or NULL) on entry;
 *		Actual base address of the view on exit.
 *		
 *	ZeroBits
 *		Number of high order address bits that must be zero.
 *		
 *	CommitSize
 *		Size in bytes of the initially committed section of 
 *		the view.
 *		
 *	SectionOffset
 *		Offset in bytes from the beginning of the section
 *		to the beginning of the view.
 *		
 *	ViewSize
 *		Desired length of map (or zero to map all) on entry
 *		Actual length mapped on exit.
 *		
 *	InheritDisposition
 *		Specified how the view is to be shared with
 *		child processes.
 *		
 *	AllocationType
 *		Type of allocation for the pages.
 *		
 *	Protect
 *		Protection for the committed region of the view.
 *
 * RETURN VALUE
 *	Status.
 */
NTSTATUS STDCALL
MmMapViewOfSection(IN PVOID SectionObject,
		   IN PEPROCESS Process,
		   IN OUT PVOID *BaseAddress,
		   IN ULONG ZeroBits,
		   IN ULONG CommitSize,
		   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		   IN OUT PULONG ViewSize,
		   IN SECTION_INHERIT InheritDisposition,
		   IN ULONG AllocationType,
		   IN ULONG Protect)
{
   PSECTION_OBJECT Section;
   PMADDRESS_SPACE AddressSpace;
   ULONG ViewOffset;
   NTSTATUS Status = STATUS_SUCCESS;

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = &Process->AddressSpace;

   MmLockSection(SectionObject);
   
   if (Section->AllocationAttributes & SEC_IMAGE)
     {
       ULONG i;
       PVOID ImageBase;
       ULONG ImageSize;	   
       
       MmLockAddressSpace(AddressSpace);

       ImageBase = *BaseAddress;
       if (ImageBase == NULL)
	 {
	   ImageBase = Section->ImageBase;
	 }
       
       ImageSize = 0;
       for (i = 0; i < Section->NrSegments; i++)
	 {
	   if (!(Section->Segments[i].Characteristics & IMAGE_SECTION_NOLOAD))
	     {
	       ULONG MaxExtent;
	       MaxExtent = (ULONG)(Section->Segments[i].VirtualAddress +
				   Section->Segments[i].Length);
	       ImageSize = max(ImageSize, MaxExtent);
	     }
	 }

       /* Check there is enough space to map the section at that point. */
       if (MmOpenMemoryAreaByRegion(AddressSpace, ImageBase, 
				    PAGE_ROUND_UP(ImageSize)) != NULL)
	 {
	   /* Fail if the user requested a fixed base address. */
	   if ((*BaseAddress) != NULL)
	     {
	       MmUnlockSection(Section);
	       MmUnlockAddressSpace(AddressSpace);
	       return(STATUS_UNSUCCESSFUL);
	     }
	   /* Otherwise find a gap to map the image. */
	   ImageBase = MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize));
	   if (ImageBase == NULL)
	     {
	       MmUnlockSection(Section);
	       MmUnlockAddressSpace(AddressSpace);
	       return(STATUS_UNSUCCESSFUL);
	     }
	 }
				    
       for (i = 0; i < Section->NrSegments; i++)
	 {
	   PVOID SBaseAddress;	   	   
	   
	   if (!(Section->Segments[i].Characteristics & IMAGE_SECTION_NOLOAD))
	     {
	       SBaseAddress = (PVOID)
		 (ImageBase + (ULONG_PTR)Section->Segments[i].VirtualAddress);

	       MmLockSectionSegment(&Section->Segments[i]);
	       Status = MmMapViewOfSegment(Process,
					   &Process->AddressSpace,
					   Section,
					   &Section->Segments[i],
					   &SBaseAddress,
					   Section->Segments[i].Length,
					   Section->Segments[i].Protection,
					   Section->Segments[i].FileOffset);
	       MmUnlockSectionSegment(&Section->Segments[i]);
	       if (!NT_SUCCESS(Status))
		 {
		   MmUnlockSection(Section);
		   MmUnlockAddressSpace(AddressSpace);
		   return(Status);
		 }
	     }
	 }

       *BaseAddress = ImageBase;

       MmUnlockAddressSpace(AddressSpace);

       /*
        * Zero-fill the end of initialized data segments which are not completely
        * present in the file
        */
       for (i = 0; i < Section->NrSegments; i++)
	 {
	   if (IMAGE_SECTION_INITIALIZED_DATA ==
               (Section->Segments[i].Characteristics &
	        (IMAGE_SECTION_NOLOAD | IMAGE_SECTION_INITIALIZED_DATA)) &&
	       Section->Segments[i].RawLength < Section->Segments[i].Length)
	     {
	       /* PsGetCurrentProcess() might not return our process at this moment,
	        * let's make sure our address space is being used anyway */
	       KeAttachProcess(Process);

	       RtlZeroMemory((PVOID) (ImageBase + (ULONG_PTR) Section->Segments[i].VirtualAddress) +
	                     Section->Segments[i].RawLength,
	                     Section->Segments[i].Length - Section->Segments[i].RawLength);

	       KeDetachProcess();
	     }
	 }
     }
   else
     {
       MmLockAddressSpace(AddressSpace);

       if (SectionOffset == NULL)
	 {
	   ViewOffset = 0;
	 }
       else
	 {
	   ViewOffset = SectionOffset->u.LowPart;
	 }
       
       if ((ViewOffset % PAGE_SIZE) != 0)
	 {
	   MmUnlockSection(Section);
	   MmUnlockAddressSpace(AddressSpace);
	   return(STATUS_MAPPED_ALIGNMENT);
	 }

       if ((*ViewSize) == 0)
	 {
	   (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
	 }
       else if (((*ViewSize)+ViewOffset) > Section->MaximumSize.u.LowPart)
	 {
	   (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
	 }
       
       MmLockSectionSegment(Section->Segments);
       Status = MmMapViewOfSegment(Process,
				   &Process->AddressSpace,
				   Section,
				   Section->Segments,
				   BaseAddress,
				   *ViewSize,
				   Protect,
				   ViewOffset);
       MmUnlockSectionSegment(Section->Segments);
       MmUnlockAddressSpace(AddressSpace);
       if (!NT_SUCCESS(Status))
	 {
	   MmUnlockSection(Section);
	   return(Status);
	 }

     }

   MmUnlockSection(Section);

   return(STATUS_SUCCESS);
}

BOOLEAN STDCALL
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS	SectionObjectPointer,
		      IN PLARGE_INTEGER			NewFileSize)
{
  UNIMPLEMENTED;
  return (FALSE);
}


BOOLEAN STDCALL
MmDisableModifiedWriteOfSection (DWORD	Unknown0)
{
  UNIMPLEMENTED;
  return (FALSE);
}

BOOLEAN STDCALL
MmFlushImageSection (IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
		     IN	MMFLUSH_TYPE			FlushType)
{
   switch(FlushType)
   {
      case MmFlushForDelete: 
         if (SectionObjectPointer->ImageSectionObject || 
	     SectionObjectPointer->DataSectionObject)
	 {
            return FALSE;
	 }
	 CcRosSetRemoveOnClose(SectionObjectPointer);
         return TRUE;
      case MmFlushForWrite:
	 break;
   }
   return FALSE;
}

BOOLEAN STDCALL
MmForceSectionClosed (DWORD	Unknown0,
		      DWORD	Unknown1)
{
  UNIMPLEMENTED;
  return (FALSE);
}


NTSTATUS STDCALL
MmMapViewInSystemSpace (IN	PVOID	Section,
			OUT	PVOID	* MappedBase,
			IN	PULONG	ViewSize)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL
MmUnmapViewInSystemSpace (DWORD	Unknown0)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
MmSetBankedSection (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2,
		    DWORD	Unknown3,
		    DWORD	Unknown4,
		    DWORD	Unknown5)
{
  UNIMPLEMENTED;
  return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	MmCreateSection@
 * 	
 * DESCRIPTION
 * 	Creates a section object.
 * 	
 * ARGUMENTS
 *	SectionObjiect (OUT)
 *		Caller supplied storage for the resulting pointer
 *		to a SECTION_BOJECT instance;
 *		
 *	DesiredAccess
 *		Specifies the desired access to the section can be a
 *		combination of:
 *			STANDARD_RIGHTS_REQUIRED	|
 *			SECTION_QUERY			|
 *			SECTION_MAP_WRITE		|
 *			SECTION_MAP_READ		|
 *			SECTION_MAP_EXECUTE
 *			
 *	ObjectAttributes [OPTIONAL]
 *		Initialized attributes for the object can be used 
 *		to create a named section;
 *
 *	MaximumSize
 *		Maximizes the size of the memory section. Must be 
 *		non-NULL for a page-file backed section. 
 *		If value specified for a mapped file and the file is 
 *		not large enough, file will be extended.
 *		
 *	SectionPageProtection
 *		Can be a combination of:
 *			PAGE_READONLY	| 
 *			PAGE_READWRITE	|
 *			PAGE_WRITEONLY	| 
 *			PAGE_WRITECOPY
 *			
 *	AllocationAttributes
 *		Can be a combination of:
 *			SEC_IMAGE	| 
 *			SEC_RESERVE
 *			
 *	FileHandle
 *		Handle to a file to create a section mapped to a file
 *		instead of a memory backed section;
 *
 *	File
 *		Unknown.
 *	
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL
MmCreateSection (OUT	PSECTION_OBJECT		* SectionObject,
		 IN	ACCESS_MASK		DesiredAccess,
		 IN	POBJECT_ATTRIBUTES	ObjectAttributes     OPTIONAL,
		 IN	PLARGE_INTEGER		MaximumSize,
		 IN	ULONG			SectionPageProtection,
		 IN	ULONG			AllocationAttributes,
		 IN	HANDLE			FileHandle	  OPTIONAL,
		 IN	PFILE_OBJECT		File		    OPTIONAL)
{
  return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */






