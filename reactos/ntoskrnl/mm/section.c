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
/* $Id: section.c,v 1.66 2001/11/13 22:46:49 ekohl Exp $
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
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/cc.h>
#include <ddk/ntifs.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED MmSectionObjectType = NULL;

static GENERIC_MAPPING MmpSectionMapping = {
	STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
	STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
	STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
	SECTION_ALL_ACCESS};

#define TAG_MM_SECTION_SEGMENT   TAG('M', 'M', 'S', 'S')
#define TAG_SECTION_PAGE_TABLE   TAG('M', 'S', 'P', 'T')

#define SHARE_COUNT(E)           ((E) & 0xFFF)
#define MAX_SHARE_COUNT          0xFFF

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

NTSTATUS 
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
		       PMEMORY_AREA MArea,
		       PVOID Address)
{
   return(STATUS_UNSUCCESSFUL);
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
   Table->Pages[TableOffset] = Entry;
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
   Entry = Table->Pages[TableOffset];
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
  if (SHARE_COUNT(Entry) == MAX_SHARE_COUNT)
    {
      DPRINT1("Maximum share count reached\n");
      KeBugCheck(0);
    }
  Entry++;
  MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

VOID 
MmUnsharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
			       ULONG Offset)
{
  ULONG Entry;

  Entry = MmGetPageEntrySectionSegment(Segment, Offset);
  if (Entry == 0)
    {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
      KeBugCheck(0);
    }
  if (SHARE_COUNT(Entry) == 0)
    {
      DPRINT1("Zero share count for unshare\n");
      KeBugCheck(0);
    }
  Entry--;
  MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

NTSTATUS
MiReadPage(PMEMORY_AREA MemoryArea,
	   PLARGE_INTEGER Offset,
	   PVOID* Page)
     /*
      * FUNCTION: Read a page for a section backed memory area.
      * PARAMETERS:
      *       MemoryArea - Memory area to read the page for.
      *       Offset - Offset of the page to read.
      *       Page - Variable that receives a page contains the read data.
      */
{
  IO_STATUS_BLOCK IoStatus;
  PFILE_OBJECT FileObject;
  PMDL Mdl;
  NTSTATUS Status;
  PREACTOS_COMMON_FCB_HEADER Fcb;

  FileObject = MemoryArea->Data.SectionData.Section->FileObject;
  Fcb = (PREACTOS_COMMON_FCB_HEADER)FileObject->FsContext;
  
  /*
   * If the file system is letting us go directly to the cache and the
   * memory area was mapped at an offset in the file which is page aligned
   * then get the related cache segment.
   */
  if (FileObject->Flags & FO_DIRECT_CACHE_PAGING_READ &&
      (Offset->QuadPart % PAGESIZE) == 0)
    {
      ULONG BaseOffset;
      PVOID BaseAddress;
      BOOLEAN UptoDate;
      PCACHE_SEGMENT CacheSeg;
      LARGE_INTEGER SegOffset;
      PHYSICAL_ADDRESS Addr;

      /*
       * Get the related cache segment; we use a lower level interface than
       * filesystems do because it is safe for us to use an offset with a
       * alignment less than the file system block size.
       */
      Status = CcRosGetCacheSegment(Fcb->Bcb,
				 (ULONG)Offset->QuadPart,
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

	  Mdl = MmCreateMdl(NULL, BaseAddress, Fcb->Bcb->CacheSegmentSize);
	  MmBuildMdlForNonPagedPool(Mdl);
	  SegOffset.QuadPart = BaseOffset;
	  Status = IoPageRead(FileObject,
			      Mdl,
			      &SegOffset,
			      &IoStatus,
			      TRUE);
	  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
	    {
	      CcRosReleaseCacheSegment(Fcb->Bcb, CacheSeg, FALSE);
	      return(Status);
	    }
	}
      /*
       * Retrieve the page from the cache segment that we actually want.
       */
      Addr = MmGetPhysicalAddress(BaseAddress +
				  Offset->QuadPart - BaseOffset);
      (*Page) = (PVOID)(ULONG)Addr.QuadPart;
      MmReferencePage((*Page));

      CcRosReleaseCacheSegment(Fcb->Bcb, CacheSeg, TRUE);
      return(STATUS_SUCCESS);
    }
  else
    {
      /*
       * Allocate a page, this is rather complicated by the possibility
       * we might have to move other things out of memory
       */
      (*Page) = MmAllocPage(0);
      while ((*Page) == NULL)
	{
	  MmWaitForFreePages();
	  (*Page) = MmAllocPage(0);
	}

      /*
       * Create an mdl to hold the page we are going to read data into.
       */
      Mdl = MmCreateMdl(NULL, NULL, PAGESIZE);
      MmBuildMdlFromPages(Mdl, (PULONG)Page);
      /*
       * Call the FSD to read the page
       */
      Status = IoPageRead(FileObject,
			  Mdl,
			  Offset,
			  &IoStatus,
			  FALSE);
      return(Status);
    }
}

NTSTATUS
MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
			     MEMORY_AREA* MemoryArea,
			     PVOID Address,
			     BOOLEAN Locked)
{
   LARGE_INTEGER Offset;
   PVOID Page;
   NTSTATUS Status;
   ULONG PAddress;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   ULONG Entry;
   ULONG Entry1;
   ULONG Attributes;
   PMM_PAGEOP PageOp;
   PVOID NewAddress;
   
   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(NULL, Address))
     {
	if (Locked)
	  {
	    MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
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
   MmLockSection(Section);
   MmLockSectionSegment(Segment);
   
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
	   MmReleasePageOp(PageOp);
	   return(STATUS_MM_RESTART_OPERATION);
	 }
       /*
	* If the thread handling this fault has failed then we don't retry
	*/
       if (!NT_SUCCESS(PageOp->Status))
	 {
	   MmLockAddressSpace(AddressSpace);
	   return(PageOp->Status);
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

	   Page = (PVOID)(Entry & 0xFFFFF000);
	   MmReferencePage(Page);	
	   //	   MmSharePageEntrySectionSegment(Segment, Offset.u.LowPart);

	   Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
					   Address,
					   Attributes,
					   (ULONG)Page);
	   if (!NT_SUCCESS(Status))
	     {
	       DbgPrint("Unable to create virtual mapping\n");
	       KeBugCheck(0);
	     }
	 }
       if (Locked)
	 {
	   MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
	 }
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       MmReleasePageOp(PageOp);
       return(STATUS_SUCCESS);
     }

   /*
    * Satisfying a page fault on a map of /Device/PhysicalMemory is easy
    */
   if (Section->Flags & SO_PHYSICAL_MEMORY)
     {
       /*
	* Just map the desired physical page 
	*/
       Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
				       Address,
				       MemoryArea->Attributes,
				       Offset.QuadPart);
       if (Locked)
	 {
	   MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
	 }  

       /*
	* Cleanup and release locks
	*/
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       return(STATUS_SUCCESS);
     }
   
   /*
    * Map anonymous memory for BSS sections
    */
   if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS)
     {
       Page = MmAllocPage(0);
       while (Page == NULL)
	 {
	    MmUnlockSectionSegment(Segment);
	    MmUnlockSection(Section);
	    MmUnlockAddressSpace(AddressSpace);	   
	    MmWaitForFreePages();
	    MmLockAddressSpace(AddressSpace);
	    MmLockSection(Section);
	    MmLockSectionSegment(Segment);
	    Page = MmAllocPage(0);
	 }

       // clear the page	
       NewAddress = ExAllocatePageWithPhysPage((ULONG)Page);
       memset(NewAddress, 0, PAGESIZE);
       ExUnmapPage(NewAddress);

       Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
				       Address,
				       MemoryArea->Attributes,
				       (ULONG)Page);
       if (Locked)
	 {
	   MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
	 }  

       /*
	* Cleanup and release locks
	*/
       PageOp->Status = STATUS_SUCCESS;
       KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
       MmReleasePageOp(PageOp);
       MmUnlockSectionSegment(Segment);
       MmUnlockSection(Section);
       return(STATUS_SUCCESS);
     }

   /*
    * Check if this page needs to be mapped COW
    */
   if (Segment->Characteristics & IMAGE_SECTION_CHAR_DATA)
     {
       Attributes = PAGE_READONLY;
     }
   else
     {
       Attributes = MemoryArea->Attributes;
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

	Status = MiReadPage(MemoryArea, &Offset, &Page);
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
	Entry = (ULONG)Page;
	MmSetPageEntrySectionSegment(Segment, Offset.QuadPart, Entry);
	//	MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
	
	Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
					Address,
					Attributes,
					(ULONG)Page);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
       if (Locked)
	 {
	   MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
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
	
	Page = (PVOID)Entry;
	MmReferencePage(Page);	
	//	MmSharePageEntrySectionSegment(Segment, Offset.QuadPart);
	
	Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
					Address,
					Attributes,
					(ULONG)Page);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
       if (Locked)
	 {
	   MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
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
  ULONG OldPage;
  PVOID NewPage;
  PVOID NewAddress;
  NTSTATUS Status;
  ULONG PAddress;
  LARGE_INTEGER Offset;
  PMM_PAGEOP PageOp;

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
   MmLockSection(Section);
   MmLockSectionSegment(Segment);

   /*
    * Check if we are doing COW
    */
   if (!(Segment->Characteristics & IMAGE_SECTION_CHAR_DATA))
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
   NewPage = MmAllocPage(0);
   while (NewPage == NULL)
     {
       MmWaitForFreePages();
       NewPage = MmAllocPage(0);
     }

   /*
    * Copy the old page
    */
   OldPage = MmGetPhysicalAddressForProcess(NULL, Address);
 
   NewAddress = ExAllocatePageWithPhysPage((ULONG)NewPage);
   memcpy(NewAddress, (PVOID)PAGE_ROUND_DOWN(Address), PAGESIZE);
   ExUnmapPage(NewAddress);

   /*
    * Set the PTE to point to the new page
    */
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateVirtualMapping(PsGetCurrentProcess(),
				   Address,
				   MemoryArea->Attributes,
				   (ULONG)NewPage);   
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("Unable to create virtual mapping\n");
       KeBugCheck(0);
     }
   if (Locked)
     {
       MmLockPage((PVOID)MmGetPhysicalAddressForProcess(NULL, Address));
     }  
   PageOp->Status = STATUS_SUCCESS;
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

ULONG 
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
		     MEMORY_AREA* MemoryArea, 
		     PVOID Address,
		     PBOOLEAN Ul)
{
   (*Ul) = FALSE;
   return(0);
}

VOID STDCALL
MmpDeleteSection(PVOID ObjectBody)
{
  PSECTION_OBJECT Section = (PSECTION_OBJECT)ObjectBody;

  DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
   if (Section->Flags & MM_IMAGE_SECTION)
     {
       ULONG i;

       for (i = 0; i < Section->NrSegments; i++)
	 {
	   InterlockedDecrement(&Section->Segments[i].ReferenceCount);
	 }
     }
   else
     {
       InterlockedDecrement(&Section->Segments->ReferenceCount);
     }
  if (Section->FileObject != NULL)
    {
      ObDereferenceObject(Section->FileObject);
      Section->FileObject = NULL;
    }
}

VOID STDCALL
MmpCloseSection(PVOID ObjectBody,
		ULONG HandleCount)
{
   DPRINT("MmpCloseSection(OB %x, HC %d) RC %d\n",
	   ObjectBody, HandleCount, ObGetReferenceCount(ObjectBody));
   
}

NTSTATUS STDCALL
MmpCreateSection(PVOID ObjectBody,
		 PVOID Parent,
		 PWSTR RemainingPath,
		 POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   DPRINT("MmpCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath == NULL)
     {
	return(STATUS_SUCCESS);
     }
   
   if (wcschr(RemainingPath+1, L'\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = ObReferenceObjectByPointer(Parent,
				       STANDARD_RIGHTS_REQUIRED,
				       ObDirectoryType,
				       UserMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
   ObDereferenceObject(Parent);
   
   return(STATUS_SUCCESS);
}

NTSTATUS 
MmCreatePhysicalMemorySection(VOID)
{
  HANDLE PhysSectionH;
  PSECTION_OBJECT PhysSection;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES Obj;
  UNICODE_STRING Name;
  LARGE_INTEGER SectionSize;
   
  /*
   * Create the section mapping physical memory 
   */
  SectionSize.QuadPart = 0xFFFFFFFF;
  RtlInitUnicodeString(&Name, L"\\Device\\PhysicalMemory");
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
  PhysSection->Flags = PhysSection->Flags | SO_PHYSICAL_MEMORY;
  ObDereferenceObject((PVOID)PhysSection);
   
  return(STATUS_SUCCESS);
}

NTSTATUS 
MmInitSectionImplementation(VOID)
{
   MmSectionObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&MmSectionObjectType->TypeName, L"Section");
   
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
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->Flags = 0;
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
  Segment->Flags = MM_PAGEFILE_SECTION;
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
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->Flags = 0;
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
      Segment->Flags = 0;
      Segment->Characteristics = 0;
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
      Status = KeWaitForSingleObject((PVOID)&Section->Lock,
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
  Section->FileObject = FileObject; 
  KeReleaseMutex(&Segment->Lock, FALSE);

  ObDereferenceObject(Section);
  return(STATUS_SUCCESS);
}

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
  Section->AllocateAttributes = AllocationAttributes;
  InitializeListHead(&Section->ViewListHead);
  KeInitializeSpinLock(&Section->ViewListLock);
  KeInitializeMutex(&Section->Lock, 0);
  Section->Flags = MM_IMAGE_SECTION;
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
      SectionSegments[0].RawLength = PAGESIZE;
      SectionSegments[0].Length = PAGESIZE;
      SectionSegments[0].Flags = 0;
      SectionSegments[0].ReferenceCount = 1;
      SectionSegments[0].VirtualAddress = 0;
      KeInitializeMutex(&SectionSegments[0].Lock, 0);

      for (i = 1; i < NrSegments; i++)
	{
	  SectionSegments[i].FileOffset = 
	    ImageSections[i-1].PointerToRawData;
	  SectionSegments[i].Characteristics = 
	    ImageSections[i-1].Characteristics;
	  if (ImageSections[i-1].Characteristics & IMAGE_SECTION_CHAR_CODE)
	    {
	      SectionSegments[i].Protection = PAGE_EXECUTE_READ;
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (ImageSections[i-1].Characteristics & 
		   IMAGE_SECTION_CHAR_DATA)
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].Attributes = 0;
	    }
	  else if (ImageSections[i-1].Characteristics & IMAGE_SECTION_CHAR_BSS)
	    {
	      SectionSegments[i].Protection = PAGE_READWRITE;
	      SectionSegments[i].Attributes = MM_SECTION_SEGMENT_BSS;
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
      ExFreePool(ImageSections);
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
	  InterlockedIncrement(&SectionSegments[i].ReferenceCount);
	}

    }
  KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
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

NTSTATUS
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
  MmLockAddressSpace(&Process->AddressSpace);
  Status = MmCreateMemoryArea(Process,
			      &Process->AddressSpace,
			      MEMORY_AREA_SECTION_VIEW_COMMIT,
			      BaseAddress,
			      ViewSize,
			      Protect,
			      &MArea,
			      FALSE);
   MmUnlockAddressSpace(&Process->AddressSpace);
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
MmFreeSectionPage(PVOID Context, PVOID Address, ULONG PhysAddr)
{
  PMEMORY_AREA MArea;
  ULONG Entry;

  MArea = (PMEMORY_AREA)Context;

  if (PhysAddr != 0)
    {
      ULONG Offset;
      
      Offset = 
	((ULONG)PAGE_ROUND_DOWN(Address) - (ULONG)MArea->BaseAddress) + 
	MArea->Data.SectionData.ViewOffset;

      Entry = MmGetPageEntrySectionSegment(MArea->Data.SectionData.Segment,
					   Offset);
      /*
       * Just dereference private pages
       */
      if (PhysAddr != (Entry & 0xFFFFF000))
	{
	  MmDereferencePage((PVOID)PhysAddr);
	}
      else
	{
#if 0
	  MmUnsharePageEntrySectionSegment(MArea->Data.SectionData.Segment,
					   Offset);
	  MmDereferencePage((PVOID)PhysAddr);
#endif
	}
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

   AddressSpace = &Process->AddressSpace;
   
   DPRINT("Opening memory area Process %x BaseAddress %x\n",
	   Process, BaseAddress);
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	return(STATUS_UNSUCCESSFUL);
     }
   
   MmLockSection(MemoryArea->Data.SectionData.Section);
   MmLockSectionSegment(MemoryArea->Data.SectionData.Segment);
   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   if (MemoryArea->Data.SectionData.Section->Flags & SO_PHYSICAL_MEMORY)
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
   MmUnlockAddressSpace(AddressSpace);
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

   Status = MmUnmapViewOfSection(Process, BaseAddress);

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
	  
	  Status = STATUS_SUCCESS;
	  break;
	}

    default:
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
   DPRINT("Result %p\n",Result);
   for (i = 0; (i <= (Length / PAGESIZE)); i++)
     {
	Status = MmCreateVirtualMapping (NULL,
					 (Result + (i * PAGESIZE)),
					 PAGE_READWRITE,
					 (ULONG)MmAllocPage(0));
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     MmUnlockAddressSpace(AddressSpace);
		 KeBugCheck(0);
	  }
     }
   MmUnlockAddressSpace(AddressSpace);
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
   NTSTATUS Status;

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = &Process->AddressSpace;

   MmLockAddressSpace(AddressSpace);
   MmLockSection(SectionObject);
   
   if (Section->Flags & MM_IMAGE_SECTION)
     {
       ULONG i;

       for (i = 0; i < Section->NrSegments; i++)
	 {
	   PVOID SBaseAddress;

	   if (!(Section->Segments[i].Characteristics & IMAGE_SECTION_NOLOAD))
	     {
	       SBaseAddress = (PVOID)
		 ((ULONG)Section->ImageBase + 
		  (ULONG)Section->Segments[i].VirtualAddress);

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
		   ObDereferenceObject(Section);
		   ObDereferenceObject(Process);
		   return(Status);
		 }
	     }
	 }
       *BaseAddress = Section->ImageBase;
     }
   else
     {
       if (SectionOffset == NULL)
	 {
	   ViewOffset = 0;
	 }
       else
	 {
	   ViewOffset = SectionOffset->u.LowPart;
	 }
       
       if ((ViewOffset % PAGESIZE) != 0)
	 {
	   MmUnlockSection(Section);
	   MmUnlockAddressSpace(AddressSpace);
	   ObDereferenceObject(Section);
	   ObDereferenceObject(Process);
	   return(STATUS_MAPPED_ALIGNMENT);
	 }

       if (((*ViewSize)+ViewOffset) > Section->MaximumSize.u.LowPart)
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
       if (!NT_SUCCESS(Status))
	 {
	   MmUnlockSection(Section);
	   MmUnlockAddressSpace(AddressSpace);
	   ObDereferenceObject(Section);
	   ObDereferenceObject(Process);
	   return(Status);
	 }
     }

   MmUnlockSection(Section);
   MmUnlockAddressSpace(AddressSpace);

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
  UNIMPLEMENTED;
  return (FALSE);
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






