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
/* $Id: section.c,v 1.148 2004/05/01 00:25:41 tamlin Exp $
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
#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <internal/mm.h>
#include <internal/io.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/cc.h>
#include <ddk/ntifs.h>
#include <ntos/minmax.h>
#include <rosrtl/string.h>
#include <reactos/bugcodes.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct
{
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   ULONG Offset;
   BOOLEAN WasDirty;
   BOOLEAN Private;
}
MM_SECTION_PAGEOUT_CONTEXT;

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

/* Note: Mmsp prefix denotes "Memory Manager Section Private". */

/*
 * FUNCTION:  Waits in kernel mode up to ten seconds for an MM_PAGEOP event.
 * ARGUMENTS: PMM_PAGEOP which event we should wait for.
 * RETURNS:   Status of the wait.
 */
static NTSTATUS
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
static void
MmspCompleteAndReleasePageOp(PMM_PAGEOP PageOp)
{
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
}


/*
 * FUNCTION:  Waits in kernel mode indefinitely for a file object lock.
 * ARGUMENTS: PFILE_OBJECT to wait for.
 * RETURNS:   Status of the wait.
 */
static NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File)
{
   return KeWaitForSingleObject(&File->Lock, 0, KernelMode, FALSE, NULL);
}


VOID
MmFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG i;
   if (Segment->Length > NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      for (i = 0; i < NR_SECTION_PAGE_TABLES; i++)
      {
         if (Segment->PageDirectory.PageTables[i] != NULL)
         {
            ExFreePool(Segment->PageDirectory.PageTables[i]);
         }
      }
   }
}

VOID
MmFreeSectionSegments(PFILE_OBJECT FileObject)
{
   if (FileObject->SectionObjectPointer->ImageSectionObject != NULL)
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
            KEBUGCHECK(0);
         }
         MmFreePageTablesSectionSegment(&SectionSegments[i]);
      }
      ExFreePool(ImageSectionObject);
      FileObject->SectionObjectPointer->ImageSectionObject = NULL;
   }
   if (FileObject->SectionObjectPointer->DataSectionObject != NULL)
   {
      PMM_SECTION_SEGMENT Segment;

      Segment = (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
                DataSectionObject;

      if (Segment->ReferenceCount != 0)
      {
         DPRINT1("Data segment still referenced\n");
         KEBUGCHECK(0);
      }
      MmFreePageTablesSectionSegment(Segment);
      ExFreePool(Segment);
      FileObject->SectionObjectPointer->DataSectionObject = NULL;
   }
}

VOID
MmLockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExAcquireFastMutex(&Segment->Lock);
}

VOID
MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExReleaseFastMutex(&Segment->Lock);
}

VOID
MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset,
                             ULONG Entry)
{
   PSECTION_PAGE_TABLE Table;
   ULONG DirectoryOffset;
   ULONG TableOffset;

   if (Segment->Length <= NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      Table = (PSECTION_PAGE_TABLE)&Segment->PageDirectory;
   }
   else
   {
      DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
      Table = Segment->PageDirectory.PageTables[DirectoryOffset];
      if (Table == NULL)
      {
         Table =
            Segment->PageDirectory.PageTables[DirectoryOffset] =
               ExAllocatePoolWithTag(NonPagedPool, sizeof(SECTION_PAGE_TABLE),
                                     TAG_SECTION_PAGE_TABLE);
         if (Table == NULL)
         {
            KEBUGCHECK(0);
         }
         memset(Table, 0, sizeof(SECTION_PAGE_TABLE));
         DPRINT("Table %x\n", Table);
      }
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

   if (Segment->Length <= NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      Table = (PSECTION_PAGE_TABLE)&Segment->PageDirectory;
   }
   else
   {
      DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
      Table = Segment->PageDirectory.PageTables[DirectoryOffset];
      DPRINT("Table %x\n", Table);
      if (Table == NULL)
      {
         return(0);
      }
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
      KEBUGCHECK(0);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == MAX_SHARE_COUNT)
   {
      DPRINT1("Maximum share count reached\n");
      KEBUGCHECK(0);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      KEBUGCHECK(0);
   }
   Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) + 1);
   MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

BOOLEAN
MmUnsharePageEntrySectionSegment(PSECTION_OBJECT Section,
                                 PMM_SECTION_SEGMENT Segment,
                                 ULONG Offset,
                                 BOOLEAN Dirty,
                                 BOOLEAN PageOut)
{
   ULONG Entry;
   BOOLEAN IsDirectMapped = FALSE;

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("Entry == 0 for MmUnsharePageEntrySectionSegment\n");
      KEBUGCHECK(0);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == 0)
   {
      DPRINT1("Zero share count for unshare\n");
      KEBUGCHECK(0);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      KEBUGCHECK(0);
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
      BOOLEAN IsImageSection;
      ULONG FileOffset;

      FileOffset = Offset + Segment->FileOffset;

      IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

      Page.QuadPart = (LONGLONG)PAGE_FROM_SSE(Entry);
      FileObject = Section->FileObject;
      if (FileObject != NULL &&
            !(Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
      {

         if ((FileOffset % PAGE_SIZE) == 0 &&
               (Offset + PAGE_SIZE <= Segment->RawLength || !IsImageSection))
         {
            NTSTATUS Status;
            Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
            IsDirectMapped = TRUE;
            Status = CcRosUnmapCacheSegment(Bcb, FileOffset, Dirty);
            if (!NT_SUCCESS(Status))
            {
               DPRINT1("CcRosUnmapCacheSegment failed, status = %x\n", Status);
               KEBUGCHECK(0);
            }
         }
      }

      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
      if (SavedSwapEntry == 0)
      {
         if (!PageOut &&
               ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                (Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED)))
         {
            /*
             * FIXME:
             *   Try to page out this page and set the swap entry
             *   within the section segment. There exist no rmap entry
             *   for this page. The pager thread can't page out a
             *   page without a rmap entry.
             */
            MmSetPageEntrySectionSegment(Segment, Offset, Entry);
         }
         else
         {
            MmSetPageEntrySectionSegment(Segment, Offset, 0);
            if (!IsDirectMapped)
            {
               MmReleasePageMemoryConsumer(MC_USER, Page);
            }
         }
      }
      else
      {
         if ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
               (Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
         {
            if (!PageOut)
            {
               if (Dirty)
               {
                  /*
                   * FIXME:
                   *   We hold all locks. Nobody can do something with the current 
                   *   process and the current segment (also not within an other process).
                   */
                  NTSTATUS Status;
                  PMDL Mdl;
                  Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
                  MmBuildMdlFromPages(Mdl, (PULONG)&Page);
                  Status = MmWriteToSwapPage(SavedSwapEntry, Mdl);
                  if (!NT_SUCCESS(Status))
                  {
                     DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", Status);
                     KEBUGCHECK(0);
                  }
               }
               MmSetPageEntrySectionSegment(Segment, Offset, MAKE_SWAP_SSE(SavedSwapEntry));
               MmSetSavedSwapEntryPage(Page, 0);
            }
            MmReleasePageMemoryConsumer(MC_USER, Page);
         }
         else
         {
            DPRINT1("Found a swapentry for a non private page in an image or data file sgment\n");
            KEBUGCHECK(0);
         }
      }
   }
   else
   {
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
   }
   return(SHARE_COUNT_FROM_SSE(Entry) > 0);
}

BOOL MiIsPageFromCache(PMEMORY_AREA MemoryArea,
                       ULONG SegOffset)
{
   if (!(MemoryArea->Data.SectionData.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
   {
      PBCB Bcb;
      PCACHE_SEGMENT CacheSeg;
      Bcb = MemoryArea->Data.SectionData.Section->FileObject->SectionObjectPointer->SharedCacheMap;
      CacheSeg = CcRosLookupCacheSegment(Bcb, SegOffset + MemoryArea->Data.SectionData.Segment->FileOffset);
      if (CacheSeg)
      {
         CcRosReleaseCacheSegment(Bcb, CacheSeg, CacheSeg->Valid, FALSE, TRUE);
         return TRUE;
      }
   }
   return FALSE;
}

NTSTATUS
MiReadPage(PMEMORY_AREA MemoryArea,
           ULONG SegOffset,
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
   ULONG FileOffset;
   PVOID BaseAddress;
   BOOLEAN UptoDate;
   PCACHE_SEGMENT CacheSeg;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   ULONG RawLength;
   PBCB Bcb;
   BOOLEAN IsImageSection;
   ULONG Length;

   FileObject = MemoryArea->Data.SectionData.Section->FileObject;
   Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
   RawLength = MemoryArea->Data.SectionData.Segment->RawLength;
   FileOffset = SegOffset + MemoryArea->Data.SectionData.Segment->FileOffset;
   IsImageSection = MemoryArea->Data.SectionData.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   assert(Bcb);

   DPRINT("%S %x\n", FileObject->FileName.Buffer, FileOffset);

   /*
    * If the file system is letting us go directly to the cache and the
    * memory area was mapped at an offset in the file which is page aligned
    * then get the related cache segment.
    */
   if ((FileOffset % PAGE_SIZE) == 0 &&
         (SegOffset + PAGE_SIZE <= RawLength || !IsImageSection) &&
         !(MemoryArea->Data.SectionData.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
   {

      /*
       * Get the related cache segment; we use a lower level interface than
       * filesystems do because it is safe for us to use an offset with a
       * alignment less than the file system block size.
       */
      Status = CcRosGetCacheSegment(Bcb,
                                    FileOffset,
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
      (*Page) = MmGetPhysicalAddress((char*)BaseAddress +
                                     FileOffset - BaseOffset);

      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, TRUE);
   }
   else
   {
      PVOID PageAddr;
      ULONG CacheSegOffset;
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
                                    FileOffset,
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
      CacheSegOffset = BaseOffset + CacheSeg->Bcb->CacheSegmentSize - FileOffset;
      Length = RawLength - SegOffset;
      if (Length <= CacheSegOffset && Length <= PAGE_SIZE)
      {
         memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, Length);
      }
      else if (CacheSegOffset >= PAGE_SIZE)
      {
         memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, PAGE_SIZE);
      }
      else
      {
         memcpy(PageAddr, (char*)BaseAddress + FileOffset - BaseOffset, CacheSegOffset);
         CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
         Status = CcRosGetCacheSegment(Bcb,
                                       FileOffset + CacheSegOffset,
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
         if (Length < PAGE_SIZE)
         {
            memcpy((char*)PageAddr + CacheSegOffset, BaseAddress, Length - CacheSegOffset);
         }
         else
         {
            memcpy((char*)PageAddr + CacheSegOffset, BaseAddress, PAGE_SIZE - CacheSegOffset);
         }
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
   ULONG Offset;
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
   BOOL HasSwapEntry;

   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(AddressSpace->Process, Address))
   {
      if (Locked)
      {
         MmLockPage(MmGetPhysicalAddressForProcess(AddressSpace->Process, Address));
      }
      return(STATUS_SUCCESS);
   }

   PAddress = (ULONG)PAGE_ROUND_DOWN(((ULONG)Address));
   Offset = PAddress - (ULONG)MemoryArea->BaseAddress;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->BaseAddress,
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
   PageOp = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset, MM_PAGEOP_PAGEIN, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
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
         KEBUGCHECK(0);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
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
      if (!MmIsPagePresent(AddressSpace->Process, Address))
      {
         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
         HasSwapEntry = MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress);

         if (PAGE_FROM_SSE(Entry) == 0 || HasSwapEntry)
         {
            /*
             * The page was a private page in another or in our address space 
            */
            MmUnlockSectionSegment(Segment);
            MmspCompleteAndReleasePageOp(PageOp);
            return(STATUS_MM_RESTART_OPERATION);
         }

         Page.QuadPart = (LONGLONG)(PAGE_FROM_SSE(Entry));

         MmSharePageEntrySectionSegment(Segment, Offset);

         Status = MmCreateVirtualMapping(MemoryArea->Process,
                                         Address,
                                         Attributes,
                                         Page,
                                         FALSE);
         if (Status == STATUS_NO_MEMORY)
         {
            MmUnlockAddressSpace(AddressSpace);
            Status = MmCreateVirtualMapping(MemoryArea->Process,
                                            Address,
                                            Attributes,
                                            Page,
                                            TRUE);
            MmLockAddressSpace(AddressSpace);
         }

         if (!NT_SUCCESS(Status))
         {
            DbgPrint("Unable to create virtual mapping\n");
            KEBUGCHECK(0);
         }
         MmInsertRmap(Page, MemoryArea->Process, (PVOID)PAddress);
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

   HasSwapEntry = MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress);
   if (HasSwapEntry)
   {
      /*
       * Must be private page we have swapped out.
       */
      SWAPENTRY SwapEntry;
      PMDL Mdl;

      /*
       * Sanity check
      */
      if (Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         DPRINT1("Found a swaped out private page in a pagefile section.\n");
         KEBUGCHECK(0);
      }

      MmUnlockSectionSegment(Segment);
      MmDeletePageFileMapping(AddressSpace->Process, (PVOID)PAddress, &SwapEntry);

      MmUnlockAddressSpace(AddressSpace);
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
      MmBuildMdlFromPages(Mdl, (PULONG)&Page);
      Status = MmReadFromSwapPage(SwapEntry, Mdl);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
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
         DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
         KEBUGCHECK(0);
         return(Status);
      }

      /*
      * Store the swap entry for later use.
      */
      MmSetSavedSwapEntryPage(Page, SwapEntry);

      /*
      * Add the page to the process's working set
      */
      MmInsertRmap(Page, AddressSpace->Process, (PVOID)PAddress);

      /*
      * Finish the operation
      */
      if (Locked)
      {
         MmLockPage(MmGetPhysicalAddressForProcess(NULL, Address));
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Satisfying a page fault on a map of /Device/PhysicalMemory is easy
    */
   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      MmUnlockSectionSegment(Segment);
      /*
      * Just map the desired physical page
      */
      Page.QuadPart = Offset + MemoryArea->Data.SectionData.ViewOffset;
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
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
         DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
         KEBUGCHECK(0);
         return(Status);
      }
      /*
       * Don't add an rmap entry since the page mapped could be for
      * anything.
      */
      if (Locked)
      {
         MmLockPage(Page);
      }

      /*
      * Cleanup and release locks
      */
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Map anonymous memory for BSS sections
    */
   if (Segment->Characteristics & IMAGE_SECTION_CHAR_BSS)
   {
      MmUnlockSectionSegment(Segment);
      Status = MmRequestPageMemoryConsumer(MC_USER, FALSE, &Page);
      if (!NT_SUCCESS(Status))
      {
         MmUnlockAddressSpace(AddressSpace);
         Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
         MmLockAddressSpace(AddressSpace);
      }
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
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
         DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
         KEBUGCHECK(0);
         return(Status);
      }
      MmInsertRmap(Page, AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Page);
      }

      /*
      * Cleanup and release locks
      */
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Get the entry corresponding to the offset within the section
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

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

      if ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
            (Offset >= PAGE_ROUND_UP(Segment->RawLength) && Section->AllocationAttributes & SEC_IMAGE))
      {
         Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
         if (!NT_SUCCESS(Status))
         {
            DPRINT1("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
         }
      }
      else
      {
         Status = MiReadPage(MemoryArea, Offset, &Page);
         if (!NT_SUCCESS(Status))
         {
            DPRINT1("MiReadPage failed (Status %x)\n", Status);
         }
      }
      if (!NT_SUCCESS(Status))
      {
         /*
          * FIXME: What do we know in this case?
          */
         /*
          * Cleanup and release locks
          */
         MmLockAddressSpace(AddressSpace);
         PageOp->Status = Status;
         MmspCompleteAndReleasePageOp(PageOp);
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
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
      Entry1 = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry != Entry1)
      {
         DbgPrint("Someone changed ppte entry while we slept\n");
         KEBUGCHECK(0);
      }

      /*
      * Mark the offset within the section as having valid, in-memory
      * data
      */
      Entry = MAKE_SSE(Page.u.LowPart, 1);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Attributes,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
      {
         MmUnlockAddressSpace(AddressSpace);
         Status = MmCreateVirtualMapping(AddressSpace->Process,
                                         Address,
                                         Attributes,
                                         Page,
                                         TRUE);
         MmLockAddressSpace(AddressSpace);
      }
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Page, AddressSpace->Process, (PVOID)PAddress);

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
      SWAPENTRY SwapEntry;
      PMDL Mdl;

      SwapEntry = SWAPENTRY_FROM_SSE(Entry);

      /*
      * Release all our locks and read in the page from disk
      */
      MmUnlockSectionSegment(Segment);

      MmUnlockAddressSpace(AddressSpace);

      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      Mdl = MmCreateMdl(NULL, NULL, PAGE_SIZE);
      MmBuildMdlFromPages(Mdl, (PULONG)&Page);
      Status = MmReadFromSwapPage(SwapEntry, Mdl);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
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
      Entry1 = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry != Entry1)
      {
         DbgPrint("Someone changed ppte entry while we slept\n");
         KEBUGCHECK(0);
      }

      /*
      * Mark the offset within the section as having valid, in-memory
      * data
      */
      Entry = MAKE_SSE(Page.u.LowPart, 1);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);

      /*
      * Save the swap entry.
      */
      MmSetSavedSwapEntryPage(Page, SwapEntry);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
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
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Page, AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Page);
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   else
   {
      /*
       * If the section offset is already in-memory and valid then just
      * take another reference to the page
      */

      Page.QuadPart = (LONGLONG)PAGE_FROM_SSE(Entry);

      MmSharePageEntrySectionSegment(Segment, Offset);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Attributes,
                                      Page,
                                      FALSE);
      if (Status == STATUS_NO_MEMORY)
      {
         MmUnlockAddressSpace(AddressSpace);
         Status = MmCreateVirtualMapping(AddressSpace->Process,
                                         Address,
                                         Attributes,
                                         Page,
                                         TRUE);
         MmLockAddressSpace(AddressSpace);
      }
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Page, AddressSpace->Process, (PVOID)PAddress);
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
   ULONG Offset;
   PMM_PAGEOP PageOp;
   PMM_REGION Region;
   ULONG Entry;

   /*
    * Check if the page has been paged out or has already been set readwrite
    */
   if (!MmIsPagePresent(AddressSpace->Process, Address) ||
         MmGetPageProtect(AddressSpace->Process, Address) & PAGE_READWRITE)
   {
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Find the offset of the page
    */
   PAddress = (ULONG)PAGE_ROUND_DOWN(((ULONG)Address));
   Offset = PAddress - (ULONG)MemoryArea->BaseAddress;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->BaseAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   OldPage = MmGetPhysicalAddressForProcess(NULL, Address);
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

   MmUnlockSectionSegment(Segment);

   /*
    * Check if we are doing COW
    */
   if (!((Segment->WriteCopy || MemoryArea->Data.SectionData.WriteCopyView) &&
         (Region->Protect == PAGE_READWRITE ||
          Region->Protect == PAGE_EXECUTE_READWRITE)))
   {
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_UNSUCCESSFUL);
   }

   if (IS_SWAP_FROM_SSE(Entry) ||
         PAGE_FROM_SSE(Entry) != OldPage.u.LowPart)
   {
      /* This is a private page. We must only change the page protection. */
      MmSetPageProtect(AddressSpace->Process, (PVOID)PAddress, Region->Protect);
      return(STATUS_SUCCESS);
   }

   /*
    * Get or create a pageop
    */
   PageOp = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset,
                        MM_PAGEOP_ACCESSFAULT, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
   }

   /*
    * Wait for any other operations to complete
    */
   if (PageOp->Thread != PsGetCurrentThread())
   {
      MmUnlockAddressSpace(AddressSpace);
      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      /*
      * Check for various strange conditions
      */
      if (Status == STATUS_TIMEOUT)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
      }
      /*
      * Restart the operation
      */
      MmLockAddressSpace(AddressSpace);
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_MM_RESTART_OPERATION);
   }

   /*
    * Release locks now we have the pageop
    */
   MmUnlockAddressSpace(AddressSpace);

   /*
    * Allocate a page
    */
   Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &NewPage);
   if (!NT_SUCCESS(Status))
   {
      KEBUGCHECK(0);
   }

   /*
    * Copy the old page
    */

   NewAddress = ExAllocatePageWithPhysPage(NewPage);
   memcpy(NewAddress, (PVOID)PAddress, PAGE_SIZE);
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
   if (Status == STATUS_NO_MEMORY)
   {
      MmUnlockAddressSpace(AddressSpace);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      NewPage,
                                      TRUE);
      MmLockAddressSpace(AddressSpace);
   }
   if (!NT_SUCCESS(Status))
   {
      DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
      KEBUGCHECK(0);
      return(Status);
   }
   MmInsertRmap(NewPage, AddressSpace->Process, (PVOID)PAddress);
   if (!NT_SUCCESS(Status))
   {
      DbgPrint("Unable to create virtual mapping\n");
      KEBUGCHECK(0);
   }
   if (Locked)
   {
      MmLockPage(NewPage);
      MmUnlockPage(OldPage);
   }

   /*
    * Unshare the old page.
    */
   MmDeleteRmap(OldPage, AddressSpace->Process, (PVOID)PAddress);
   MmLockSectionSegment(Segment);
   MmUnsharePageEntrySectionSegment(Section, Segment, Offset, FALSE, FALSE);
   MmUnlockSectionSegment(Segment);

   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   DPRINT("Address 0x%.8X\n", Address);
   return(STATUS_SUCCESS);
}

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
{
   MM_SECTION_PAGEOUT_CONTEXT* PageOutContext;
   BOOL WasDirty;
   PHYSICAL_ADDRESS Page;

   PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
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
      MmUnsharePageEntrySectionSegment(PageOutContext->Section,
                                       PageOutContext->Segment,
                                       PageOutContext->Offset,
                                       PageOutContext->WasDirty,
                                       TRUE);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
   }

   DPRINT("PhysicalAddress %I64x, Address %x\n", Page, Address);
}

NTSTATUS
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
                     MEMORY_AREA* MemoryArea,
                     PVOID Address,
                     PMM_PAGEOP PageOp)
{
   PHYSICAL_ADDRESS PhysicalAddress;
   MM_SECTION_PAGEOUT_CONTEXT Context;
   SWAPENTRY SwapEntry;
   PMDL Mdl;
   ULONG Entry;
   ULONG FileOffset;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PBCB Bcb = NULL;
   BOOLEAN DirectMapped;
   BOOLEAN IsImageSection;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;

   Context.Offset = (ULONG)((char*)Address - (ULONG)MemoryArea->BaseAddress);
   FileOffset = Context.Offset + Context.Segment->FileOffset;

   IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Context.Section->FileObject;
   DirectMapped = FALSE;
   if (FileObject != NULL &&
         !(Context.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
   {
      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if ((FileOffset % PAGE_SIZE) == 0 &&
            (Context.Offset + PAGE_SIZE <= Context.Segment->RawLength || !IsImageSection))
      {
         DirectMapped = TRUE;
      }
   }


   /*
    * This should never happen since mappings of physical memory are never
    * placed in the rmap lists.
    */
   if (Context.Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      DPRINT1("Trying to page out from physical memory section address 0x%X "
              "process %d\n", Address,
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0);
      KEBUGCHECK(0);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Context.Segment, Context.Offset);
   if (!MmIsPagePresent(AddressSpace->Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0, Address);
      KEBUGCHECK(0);
   }
   PhysicalAddress =
      MmGetPhysicalAddressForProcess(AddressSpace->Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);

   /*
    * Prepare the context structure for the rmap delete call.
    */
   Context.WasDirty = FALSE;
   if (Context.Segment->Characteristics & IMAGE_SECTION_CHAR_BSS ||
         IS_SWAP_FROM_SSE(Entry) ||
         (LONGLONG)PAGE_FROM_SSE(Entry) != PhysicalAddress.QuadPart)
   {
      Context.Private = TRUE;
   }
   else
   {
      Context.Private = FALSE;
   }

   /*
    * Take an additional reference to the page or the cache segment.
    */
   if (DirectMapped && !Context.Private)
   {
      if(!MiIsPageFromCache(MemoryArea, Context.Offset))
      {
         DPRINT1("Direct mapped non private page is not associated with the cache.\n");
         KEBUGCHECK(0);
      }
   }
   else
   {
      MmReferencePage(PhysicalAddress);
   }

   MmDeleteAllRmaps(PhysicalAddress, (PVOID)&Context, MmPageOutDeleteMapping);

   /*
    * If this wasn't a private page then we should have reduced the entry to
    * zero by deleting all the rmaps.
    */
   if (!Context.Private && MmGetPageEntrySectionSegment(Context.Segment, Context.Offset) != 0)
   {
      if (!(Context.Segment->Flags & MM_PAGEFILE_SEGMENT) &&
            !(Context.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
      {
         KEBUGCHECK(0);
      }
   }

   /*
    * If the page wasn't dirty then we can just free it as for a readonly page.
    * Since we unmapped all the mappings above we know it will not suddenly
    * become dirty.
    * If the page is from a pagefile section and has no swap entry,
    * we can't free the page at this point.
    */
   SwapEntry = MmGetSavedSwapEntryPage(PhysicalAddress);
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a pagefile segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
         KEBUGCHECK(0);
      }
      if (!Context.WasDirty && SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(PhysicalAddress, 0);
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (Context.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a shared section segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
         KEBUGCHECK(0);
      }
      if (!Context.WasDirty || SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(PhysicalAddress, 0);
         if (SwapEntry != 0)
         {
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         }
         MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (!Context.Private && DirectMapped)
   {
      if (SwapEntry != 0)
      {
         DPRINT1("Found a swapentry for a non private and direct mapped page (address %x)\n",
                 Address);
         KEBUGCHECK(0);
      }
      Status = CcRosUnmapCacheSegment(Bcb, FileOffset, FALSE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("CCRosUnmapCacheSegment failed, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }
   else if (!Context.WasDirty && !DirectMapped && !Context.Private)
   {
      if (SwapEntry != 0)
      {
         DPRINT1("Found a swap entry for a non dirty, non private and not direct mapped page (address %x)\n",
                 Address);
         KEBUGCHECK(0);
      }
      MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }
   else if (!Context.WasDirty && Context.Private && SwapEntry != 0)
   {
      MmSetSavedSwapEntryPage(PhysicalAddress, 0);
      Status = MmCreatePageFileMapping(AddressSpace->Process,
                                       Address,
                                       SwapEntry);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
      MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
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

         /*
          * For private pages restore the old mappings.
          */
         if (Context.Private)
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
            Entry = MAKE_SSE(PhysicalAddress.u.LowPart, 1);
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
         }
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
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
      if (Context.Private)
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
         Entry = MAKE_SSE(PhysicalAddress.u.LowPart, 1);
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
      }
      PageOp->Status = STATUS_UNSUCCESSFUL;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", PhysicalAddress);
   MmSetSavedSwapEntryPage(PhysicalAddress, 0);
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT ||
         Context.Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED)
   {
      MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, PhysicalAddress);
   }

   if (Context.Private)
   {
      Status = MmCreatePageFileMapping(MemoryArea->Process,
                                       Address,
                                       SwapEntry);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
   }
   else
   {
      Entry = MAKE_SWAP_SSE(SwapEntry);
      MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
   }

   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

NTSTATUS
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
                       PMM_PAGEOP PageOp)
{
   ULONG Offset;
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
   BOOLEAN IsImageSection;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = (ULONG)((char*)Address - (ULONG)MemoryArea->BaseAddress);

   /*
    * Get the segment and section.
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Section->FileObject;
   DirectMapped = FALSE;
   if (FileObject != NULL &&
         !(Segment->Characteristics & IMAGE_SECTION_CHAR_SHARED))
   {
      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if ((Offset + MemoryArea->Data.SectionData.ViewOffset % PAGE_SIZE) == 0 &&
            (Offset + PAGE_SIZE <= Segment->RawLength || !IsImageSection))
      {
         DirectMapped = TRUE;
      }
   }

   /*
    * This should never happen since mappings of physical memory are never
    * placed in the rmap lists.
    */
   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      DPRINT1("Trying to write back page from physical memory mapped at %X "
              "process %d\n", Address,
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0);
      KEBUGCHECK(0);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (!MmIsPagePresent(AddressSpace->Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0, Address);
      KEBUGCHECK(0);
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
      CcRosMarkDirtyCacheSegment(Bcb, Offset + MemoryArea->Data.SectionData.ViewOffset);
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
         MmSetDirtyAllRmaps(PhysicalAddress);
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
      }
      MmSetSavedSwapEntryPage(PhysicalAddress, SwapEntry);
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
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", PhysicalAddress);
   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
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
         PVOID Address = (char*)BaseAddress + (i * PAGE_SIZE);
         ULONG Protect = NewProtect;

         /*
          * If we doing COW for this segment then check if the page is
          * already private.
          */
         if (DoCOW && MmIsPagePresent(AddressSpace->Process, Address))
         {
            ULONG Offset;
            ULONG Entry;
            LARGE_INTEGER PhysicalAddress;

            Offset =  (ULONG)Address - (ULONG)MemoryArea->BaseAddress;
            Entry = MmGetPageEntrySectionSegment(Segment, Offset);
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
      min(Length, (ULONG) ((char*)MemoryArea->BaseAddress + MemoryArea->Length - (char*)BaseAddress));
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
   if (Region == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }
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

   *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
   return(STATUS_SUCCESS);
}

VOID
MmpFreePageFileSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG Length;
   ULONG Offset;
   ULONG Entry;
   ULONG SavedSwapEntry;
   PHYSICAL_ADDRESS Page;

   Page.u.HighPart = 0;

   Length = PAGE_ROUND_UP(Segment->Length);
   for (Offset = 0; Offset < Length; Offset += PAGE_SIZE)
   {
      Entry = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry)
      {
         if (IS_SWAP_FROM_SSE(Entry))
         {
            MmFreeSwapPage(SWAPENTRY_FROM_SSE(Entry));
         }
         else
         {
            Page.u.LowPart = PAGE_FROM_SSE(Entry);
            SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
            if (SavedSwapEntry != 0)
            {
               MmSetSavedSwapEntryPage(Page, 0);
               MmFreeSwapPage(SavedSwapEntry);
            }
            MmReleasePageMemoryConsumer(MC_USER, Page);
         }
         MmSetPageEntrySectionSegment(Segment, Offset, 0);
      }
   }
}

VOID STDCALL
MmpDeleteSection(PVOID ObjectBody)
{
   PSECTION_OBJECT Section = (PSECTION_OBJECT)ObjectBody;

   DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      ULONG RefCount;
      PMM_SECTION_SEGMENT SectionSegments;

      SectionSegments = Section->ImageSection->Segments;
      NrSegments = Section->ImageSection->NrSegments;

      for (i = 0; i < NrSegments; i++)
      {
         if (SectionSegments[i].Characteristics & IMAGE_SECTION_CHAR_SHARED)
         {
            MmLockSectionSegment(&SectionSegments[i]);
         }
         RefCount = InterlockedDecrement((LONG *)&SectionSegments[i].ReferenceCount);
         if (SectionSegments[i].Characteristics & IMAGE_SECTION_CHAR_SHARED)
         {
            if (RefCount == 0)
            {
               MmpFreePageFileSegment(&SectionSegments[i]);
            }
            MmUnlockSectionSegment(&SectionSegments[i]);
         }
      }
   }
   else
   {
      if (Section->Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         MmpFreePageFileSegment(Section->Segment);
         MmFreePageTablesSectionSegment(Section->Segment);
         ExFreePool(Section->Segment);
         Section->Segment = NULL;
      }
      else
      {
         InterlockedDecrement((LONG *)&Section->Segment->ReferenceCount);
      }
   }
   if (Section->FileObject != NULL)
   {
      CcRosDereferenceCache(Section->FileObject);
      ObDereferenceObject(Section->FileObject);
      Section->FileObject = NULL;
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

NTSTATUS INIT_FUNCTION
MmCreatePhysicalMemorySection(VOID)
{
   HANDLE PhysSectionH;
   PSECTION_OBJECT PhysSection;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Obj;
   UNICODE_STRING Name = ROS_STRING_INITIALIZER(L"\\Device\\PhysicalMemory");
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
      KEBUGCHECK(0);
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
      KEBUGCHECK(0);
   }
   PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;
   ObDereferenceObject((PVOID)PhysSection);

   return(STATUS_SUCCESS);
}

NTSTATUS INIT_FUNCTION
MmInitSectionImplementation(VOID)
{
   MmSectionObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

   RtlRosInitUnicodeStringFromLiteral(&MmSectionObjectType->TypeName, L"Section");

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

   /*
    * NOTE: Do not register the section object type here because
    * the object manager it not initialized yet!
    * The section object type will be created in ObInit().
    */
   ObpCreateTypeObject(MmSectionObjectType);

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
   Status = ObCreateObject(ExGetPreviousMode(),
                           MmSectionObjectType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(SECTION_OBJECT),
                           0,
                           0,
                           (PVOID*)&Section);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Status = ObInsertObject ((PVOID)Section,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            SectionHandle);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Section);
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);
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
   Section->Segment = Segment;
   Segment->ReferenceCount = 1;
   ExInitializeFastMutex(&Segment->Lock);
   Segment->FileOffset = 0;
   Segment->Protection = SectionPageProtection;
   Segment->Attributes = AllocationAttributes;
   Segment->RawLength = MaximumSize.u.LowPart;
   Segment->Length = PAGE_ROUND_UP(MaximumSize.u.LowPart);
   Segment->Flags = MM_PAGEFILE_SEGMENT;
   Segment->WriteCopy = FALSE;
   RtlZeroMemory(&Segment->PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
   Segment->VirtualAddress = 0;
   Segment->Characteristics = 0;
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
    * Create the section
    */
   Status = ObCreateObject(ExGetPreviousMode(),
                           MmSectionObjectType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(SECTION_OBJECT),
                           0,
                           0,
                           (PVOID*)&Section);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Status = ObInsertObject ((PVOID)Section,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            SectionHandle);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Section);
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);

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
         ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize;
   }

   if (MaximumSize.QuadPart >
         ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->FileSize.QuadPart)
   {
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

   if (FileObject->SectionObjectPointer == NULL ||
         FileObject->SectionObjectPointer->SharedCacheMap == NULL)
   {
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
         ZwClose(*SectionHandle);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }
      if (FileObject->SectionObjectPointer == NULL ||
            FileObject->SectionObjectPointer->SharedCacheMap == NULL)
      {
         /* FIXME: handle this situation */
         ZwClose(*SectionHandle);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return STATUS_INVALID_PARAMETER;
      }
   }

   /*
    * Lock the file
    */
   Status = MmspWaitForFileLock(FileObject);
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
   if (FileObject->SectionObjectPointer->DataSectionObject == NULL)
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
      Section->Segment = Segment;
      Segment->ReferenceCount = 1;
      ExInitializeFastMutex(&Segment->Lock);
      /*
       * Set the lock before assigning the segment to the file object
       */
      ExAcquireFastMutex(&Segment->Lock);
      FileObject->SectionObjectPointer->DataSectionObject = (PVOID)Segment;

      Segment->FileOffset = 0;
      Segment->Protection = SectionPageProtection;
      Segment->Attributes = 0;
      Segment->Flags = MM_DATAFILE_SEGMENT;
      Segment->Characteristics = 0;
      Segment->WriteCopy = FALSE;
      if (AllocationAttributes & SEC_RESERVE)
      {
         Segment->Length = Segment->RawLength = 0;
      }
      else
      {
         Segment->RawLength = MaximumSize.u.LowPart;
         Segment->Length = PAGE_ROUND_UP(Segment->RawLength);
      }
      Segment->VirtualAddress = NULL;
      RtlZeroMemory(&Segment->PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
   }
   else
   {
      /*
       * If the file is already mapped as a data file then we may need
       * to extend it
       */
      Segment =
         (PMM_SECTION_SEGMENT)FileObject->SectionObjectPointer->
         DataSectionObject;
      Section->Segment = Segment;
      InterlockedIncrement((PLONG)&Segment->ReferenceCount);
      MmLockSectionSegment(Segment);

      if (MaximumSize.u.LowPart > Segment->RawLength &&
            !(AllocationAttributes & SEC_RESERVE))
      {
         Segment->RawLength = MaximumSize.u.LowPart;
         Segment->Length = PAGE_ROUND_UP(Segment->RawLength);
      }
   }
   MmUnlockSectionSegment(Segment);
   Section->FileObject = FileObject;
   Section->MaximumSize = MaximumSize;
   CcRosReferenceCache(FileObject);
   KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
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
   IMAGE_DOS_HEADER DosHeader;
   IO_STATUS_BLOCK Iosb;
   LARGE_INTEGER Offset;
   IMAGE_NT_HEADERS PEHeader;
   PMM_SECTION_SEGMENT SectionSegments;
   ULONG NrSegments;
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
   ULONG i;
   ULONG Size;
   ULONG Characteristics;
   ULONG FileAccess = 0;
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
      return Status;
   }

   /*
    * Initialized caching for this file object if previously caching
    * was initialized for the same on disk file
    */
   Status = CcTryToInitializeFileCache(FileObject);

   if (!NT_SUCCESS(Status) || FileObject->SectionObjectPointer->ImageSectionObject == NULL)
   {
      PIMAGE_SECTION_HEADER ImageSections;
      /*
       * Read the dos header and check the DOS signature
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
                          NULL);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(FileObject);
         return(Status);
      }

      /*
       * Check the DOS signature
       */
      if (Iosb.Information != sizeof(DosHeader) ||
            DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
      {
         ObDereferenceObject(FileObject);
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
                          NULL);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(FileObject);
         return(Status);
      }

      /*
       * Check the signature
       */
      if (Iosb.Information != sizeof(PEHeader) ||
            PEHeader.Signature != IMAGE_NT_SIGNATURE)
      {
         ObDereferenceObject(FileObject);
         return(STATUS_INVALID_IMAGE_FORMAT);
      }

      /*
       * Read in the section headers
       */
      Offset.QuadPart = DosHeader.e_lfanew + sizeof(PEHeader);
      ImageSections = ExAllocatePool(NonPagedPool,
                                     PEHeader.FileHeader.NumberOfSections *
                                     sizeof(IMAGE_SECTION_HEADER));
      if (ImageSections == NULL)
      {
         ObDereferenceObject(FileObject);
         return(STATUS_NO_MEMORY);
      }

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
         ObDereferenceObject(FileObject);
         ExFreePool(ImageSections);
         return(Status);
      }
      if (Iosb.Information != (PEHeader.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)))
      {
         ObDereferenceObject(FileObject);
         ExFreePool(ImageSections);
         return(STATUS_INVALID_IMAGE_FORMAT);
      }

      /*
       * Create the section
       */
      Status = ObCreateObject (ExGetPreviousMode(),
                               MmSectionObjectType,
                               ObjectAttributes,
                               ExGetPreviousMode(),
                               NULL,
                               sizeof(SECTION_OBJECT),
                               0,
                               0,
                               (PVOID*)&Section);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(FileObject);
         ExFreePool(ImageSections);
         return(Status);
      }

      Status = ObInsertObject ((PVOID)Section,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               SectionHandle);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
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

      /*
              * Check file access required
              */
      if (SectionPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
      {
         FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
      }
      else
      {
         FileAccess = FILE_READ_DATA;
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
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ZwClose(*SectionHandle);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         ExFreePool(ImageSections);
         return(Status);
      }

      /*
       * allocate the section segments to describe the mapping
       */
      NrSegments = PEHeader.FileHeader.NumberOfSections + 1;
      Size = sizeof(MM_IMAGE_SECTION_OBJECT) + sizeof(MM_SECTION_SEGMENT) * NrSegments;
      ImageSectionObject = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
      {
         KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
         ZwClose(*SectionHandle);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         ExFreePool(ImageSections);
         return(STATUS_NO_MEMORY);
      }
      Section->ImageSection = ImageSectionObject;
      ImageSectionObject->NrSegments = NrSegments;
      ImageSectionObject->ImageBase = (PVOID)PEHeader.OptionalHeader.ImageBase;
      ImageSectionObject->EntryPoint = (PVOID)PEHeader.OptionalHeader.AddressOfEntryPoint;
      ImageSectionObject->StackReserve = PEHeader.OptionalHeader.SizeOfStackReserve;
      ImageSectionObject->StackCommit = PEHeader.OptionalHeader.SizeOfStackCommit;
      ImageSectionObject->Subsystem = PEHeader.OptionalHeader.Subsystem;
      ImageSectionObject->MinorSubsystemVersion = PEHeader.OptionalHeader.MinorSubsystemVersion;
      ImageSectionObject->MajorSubsystemVersion = PEHeader.OptionalHeader.MajorSubsystemVersion;
      ImageSectionObject->ImageCharacteristics = PEHeader.FileHeader.Characteristics;
      ImageSectionObject->Machine = PEHeader.FileHeader.Machine;
      ImageSectionObject->Executable = (PEHeader.OptionalHeader.SizeOfCode != 0);

      SectionSegments = ImageSectionObject->Segments;
      SectionSegments[0].FileOffset = 0;
      SectionSegments[0].Characteristics = IMAGE_SECTION_CHAR_DATA;
      SectionSegments[0].Protection = PAGE_READONLY;
      SectionSegments[0].RawLength = PAGE_SIZE;
      SectionSegments[0].Length = PAGE_SIZE;
      SectionSegments[0].Flags = 0;
      SectionSegments[0].ReferenceCount = 1;
      SectionSegments[0].VirtualAddress = 0;
      SectionSegments[0].WriteCopy = TRUE;
      SectionSegments[0].Attributes = 0;
      ExInitializeFastMutex(&SectionSegments[0].Lock);
      RtlZeroMemory(&SectionSegments[0].PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
      for (i = 1; i < NrSegments; i++)
      {
         SectionSegments[i].FileOffset = ImageSections[i-1].PointerToRawData;
         SectionSegments[i].Characteristics = ImageSections[i-1].Characteristics;

         /*
          * Set up the protection and write copy variables.
          */
         Characteristics = ImageSections[i - 1].Characteristics;
         if (Characteristics & (IMAGE_SECTION_CHAR_READABLE|IMAGE_SECTION_CHAR_WRITABLE|IMAGE_SECTION_CHAR_EXECUTABLE))
         {
            SectionSegments[i].Protection = SectionCharacteristicsToProtect[Characteristics >> 28];
            SectionSegments[i].WriteCopy = !(Characteristics & IMAGE_SECTION_CHAR_SHARED);
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
         SectionSegments[i].Length = ImageSections[i-1].Misc.VirtualSize;
         SectionSegments[i].Flags = 0;
         SectionSegments[i].ReferenceCount = 1;
         SectionSegments[i].VirtualAddress = (PVOID)ImageSections[i-1].VirtualAddress;
         ExInitializeFastMutex(&SectionSegments[i].Lock);
         RtlZeroMemory(&SectionSegments[i].PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
      }
      if (0 != InterlockedCompareExchange((PLONG)&FileObject->SectionObjectPointer->ImageSectionObject,
                                          (LONG)ImageSectionObject, 0))
      {
         /*
          * An other thread has initialized the some image in the background
          */
         ExFreePool(ImageSectionObject);
         ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
         Section->ImageSection = ImageSectionObject;
         SectionSegments = ImageSectionObject->Segments;

         for (i = 0; i < NrSegments; i++)
         {
            InterlockedIncrement((LONG *)&SectionSegments[i].ReferenceCount);
         }
      }
      ExFreePool(ImageSections);
   }
   else
   {
      /*
       * Create the section
       */
      Status = ObCreateObject (ExGetPreviousMode(),
                               MmSectionObjectType,
                               ObjectAttributes,
                               ExGetPreviousMode(),
                               NULL,
                               sizeof(SECTION_OBJECT),
                               0,
                               0,
                               (PVOID*)&Section);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(FileObject);
         return(Status);
      }

      Status = ObInsertObject ((PVOID)Section,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               SectionHandle);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }

      /*
       * Initialize it
       */
      Section->SectionPageProtection = SectionPageProtection;
      Section->AllocationAttributes = AllocationAttributes;
      InitializeListHead(&Section->ViewListHead);
      KeInitializeSpinLock(&Section->ViewListLock);

      /*
       * Check file access required
       */
      if (SectionPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
      {
         FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
      }
      else
      {
         FileAccess = FILE_READ_DATA;
      }

      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ZwClose(*SectionHandle);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }

      ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
      Section->ImageSection = ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;
      NrSegments = ImageSectionObject->NrSegments;

      /*
       * Otherwise just reference all the section segments
       */
      for (i = 0; i < NrSegments; i++)
      {
         InterlockedIncrement((LONG *)&SectionSegments[i].ReferenceCount);
      }

   }
   Section->FileObject = FileObject;
   CcRosReferenceCache(FileObject);
   KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateSection (OUT PHANDLE SectionHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN PLARGE_INTEGER MaximumSize OPTIONAL,
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

   if (FileHandle != NULL)
   {
      return(MmCreateDataFileSection(SectionHandle,
                                     DesiredAccess,
                                     ObjectAttributes,
                                     MaximumSize,
                                     SectionPageProtection,
                                     AllocationAttributes,
                                     FileHandle));
   }

   return(MmCreatePageFileSection(SectionHandle,
                                  DesiredAccess,
                                  ObjectAttributes,
                                  MaximumSize,
                                  SectionPageProtection,
                                  AllocationAttributes));
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
NTSTATUS STDCALL
NtOpenSection(PHANDLE   SectionHandle,
              ACCESS_MASK  DesiredAccess,
              POBJECT_ATTRIBUTES ObjectAttributes)
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
                   ULONG ViewOffset,
                   BOOL TopDown)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   KIRQL oldIrql;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;

   Status = MmCreateMemoryArea(Process,
                               AddressSpace,
                               MEMORY_AREA_SECTION_VIEW,
                               BaseAddress,
                               ViewSize,
                               Protect,
                               &MArea,
                               FALSE,
                               TopDown,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Mapping between 0x%.8X and 0x%.8X failed.\n",
              (*BaseAddress), (char*)(*BaseAddress) + ViewSize);
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
   NTSTATUS Status;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;

   MArea = (PMEMORY_AREA)Context;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = ((ULONG)Address - (ULONG)MArea->BaseAddress);

   Section = MArea->Data.SectionData.Section;
   Segment = MArea->Data.SectionData.Segment;


   PageOp = MmCheckForPageOp(MArea, 0, NULL, Segment, Offset);

   while (PageOp)
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(&MArea->Process->AddressSpace);

      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }

      MmLockAddressSpace(&MArea->Process->AddressSpace);
      MmLockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp);
      PageOp = MmCheckForPageOp(MArea, 0, NULL, Segment, Offset);
   }

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

   /*
    * For a dirty, datafile, non-private page mark it as dirty in the
    * cache manager.
    */
   if (Segment->Flags & MM_DATAFILE_SEGMENT)
   {
      if (PhysAddr.QuadPart == PAGE_FROM_SSE(Entry) && Dirty)
      {
         FileObject = MemoryArea->Data.SectionData.Section->FileObject;
         Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
         CcRosMarkDirtyCacheSegment(Bcb, Offset);
         assert(SwapEntry == 0);
      }
   }

   if (SwapEntry != 0)
   {
      /*
       * Sanity check
       */
      if (Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         DPRINT1("Found a swap entry for a page in a pagefile section.\n");
         KEBUGCHECK(0);
      }
      MmFreeSwapPage(SwapEntry);
   }
   else if (PhysAddr.QuadPart != 0)
   {
      if (IS_SWAP_FROM_SSE(Entry) ||
            PhysAddr.QuadPart != (PAGE_FROM_SSE(Entry)))
      {
         /*
          * Sanity check
          */
         if (Segment->Flags & MM_PAGEFILE_SEGMENT)
         {
            DPRINT1("Found a private page in a pagefile section.\n");
            KEBUGCHECK(0);
         }
         /*
          * Just dereference private pages
          */
         SavedSwapEntry = MmGetSavedSwapEntryPage(PhysAddr);
         if (SavedSwapEntry != 0)
         {
            MmFreeSwapPage(SavedSwapEntry);
            MmSetSavedSwapEntryPage(PhysAddr, 0);
         }
         MmDeleteRmap(PhysAddr, MArea->Process, Address);
         MmReleasePageMemoryConsumer(MC_USER, PhysAddr);
      }
      else
      {
         MmDeleteRmap(PhysAddr, MArea->Process, Address);
         MmUnsharePageEntrySectionSegment(Section, Segment, Offset, Dirty, FALSE);
      }
   }
}

NTSTATUS
MmUnmapViewOfSegment(PMADDRESS_SPACE AddressSpace,
                     PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   KIRQL oldIrql;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   PLIST_ENTRY RegionListHead;

   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                          BaseAddress);
   if (MemoryArea == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   MemoryArea->DeleteInProgress = TRUE;
   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;

   MmLockSectionSegment(Segment);
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   RegionListHead = &MemoryArea->Data.SectionData.RegionListHead;
   while (!IsListEmpty(RegionListHead))
   {
      CurrentEntry = RemoveHeadList(RegionListHead);
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
      ExFreePool(CurrentRegion);
   }

   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                BaseAddress,
                                0,
                                NULL,
                                NULL);
   }
   else
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                BaseAddress,
                                0,
                                MmFreeSectionPage,
                                MemoryArea);
   }
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
MmUnmapViewOfSection(PEPROCESS Process,
                     PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   PSECTION_OBJECT Section;

   DPRINT("Opening memory area Process %x BaseAddress %x\n",
          Process, BaseAddress);

   assert(Process);

   AddressSpace = &Process->AddressSpace;
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                          BaseAddress);
   if (MemoryArea == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   Section = MemoryArea->Data.SectionData.Section;

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
      PMM_SECTION_SEGMENT SectionSegments;
      PVOID ImageBaseAddress = 0;
      PMM_SECTION_SEGMENT Segment;

      Segment = MemoryArea->Data.SectionData.Segment;
      ImageSectionObject = Section->ImageSection;
      SectionSegments = ImageSectionObject->Segments;
      NrSegments = ImageSectionObject->NrSegments;

      /* Search for the current segment within the section segments
       * and calculate the image base address */
      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SECTION_NOLOAD))
         {
            if (Segment == &SectionSegments[i])
            {
               ImageBaseAddress = (char*)BaseAddress - (ULONG_PTR)SectionSegments[i].VirtualAddress;
               break;
            }
         }
      }
      if (i >= NrSegments)
      {
         KEBUGCHECK(0);
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SECTION_NOLOAD))
         {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBaseAddress + (ULONG_PTR)SectionSegments[i].VirtualAddress);

            Status = MmUnmapViewOfSegment(AddressSpace, SBaseAddress);
         }
      }
   }
   else
   {
      Status = MmUnmapViewOfSegment(AddressSpace, BaseAddress);
   }
   return(STATUS_SUCCESS);
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
NTSTATUS STDCALL
NtUnmapViewOfSection (HANDLE ProcessHandle,
                      PVOID BaseAddress)
{
   PEPROCESS Process;
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
NtQuerySection (IN HANDLE SectionHandle,
                IN CINT SectionInformationClass,
                OUT PVOID SectionInformation,
                IN ULONG Length,
                OUT PULONG ResultLength)
/*
 * FUNCTION: Queries the information of a section object.
 * ARGUMENTS:
 *        SectionHandle = Handle to the section link object
 *   SectionInformationClass = Index to a certain information structure
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
            memset(Sii, 0, sizeof(SECTION_IMAGE_INFORMATION));
            if (Section->AllocationAttributes & SEC_IMAGE)
            {
               PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
               ImageSectionObject = Section->ImageSection;

               Sii->EntryPoint = ImageSectionObject->EntryPoint;
               Sii->StackReserve = ImageSectionObject->StackReserve;
               Sii->StackCommit = ImageSectionObject->StackCommit;
               Sii->Subsystem = ImageSectionObject->Subsystem;
               Sii->MinorSubsystemVersion = (USHORT)ImageSectionObject->MinorSubsystemVersion;
               Sii->MajorSubsystemVersion = (USHORT)ImageSectionObject->MajorSubsystemVersion;
               Sii->Characteristics = ImageSectionObject->ImageCharacteristics;
               Sii->ImageNumber = ImageSectionObject->Machine;
               Sii->Executable = ImageSectionObject->Executable;
            }
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
NtExtendSection(IN HANDLE SectionHandle,
                IN ULONG NewMaximumSize)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
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
PVOID STDCALL
MmAllocateSection (IN ULONG Length)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   DPRINT("MmAllocateSection(Length %x)\n",Length);

   BoundaryAddressMultiple.QuadPart = 0;

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
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(AddressSpace);

   if (!NT_SUCCESS(Status))
   {
      return (NULL);
   }
   DPRINT("Result %p\n",Result);
   for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
   {
      PHYSICAL_ADDRESS Page;

      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to allocate page\n");
         KEBUGCHECK(0);
      }
      Status = MmCreateVirtualMapping (NULL,
                                       ((char*)Result + (i * PAGE_SIZE)),
                                       PAGE_READWRITE,
                                       Page,
                                       TRUE);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
   }
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

   assert(Process);

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = &Process->AddressSpace;

   MmLockAddressSpace(AddressSpace);

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      PVOID ImageBase;
      ULONG ImageSize;
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
      PMM_SECTION_SEGMENT SectionSegments;

      ImageSectionObject = Section->ImageSection;
      SectionSegments = ImageSectionObject->Segments;
      NrSegments = ImageSectionObject->NrSegments;


      ImageBase = *BaseAddress;
      if (ImageBase == NULL)
      {
         ImageBase = ImageSectionObject->ImageBase;
      }

      ImageSize = 0;
      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SECTION_NOLOAD))
         {
            ULONG MaxExtent;
            MaxExtent = (ULONG)((char*)SectionSegments[i].VirtualAddress +
                                SectionSegments[i].Length);
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
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
         /* Otherwise find a gap to map the image. */
         ImageBase = MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), FALSE);
         if (ImageBase == NULL)
         {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SECTION_NOLOAD))
         {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBase + (ULONG_PTR)SectionSegments[i].VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MmMapViewOfSegment(Process,
                                        AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length,
                                        SectionSegments[i].Protection,
                                        (ULONG_PTR)SectionSegments[i].VirtualAddress,
                                        FALSE);
            MmUnlockSectionSegment(&SectionSegments[i]);
            if (!NT_SUCCESS(Status))
            {
               MmUnlockAddressSpace(AddressSpace);
               return(Status);
            }
         }
      }

      *BaseAddress = ImageBase;
   }
   else
   {
      if (ViewSize == NULL)
      {
         /* Following this pointer would lead to us to the dark side */
         /* What to do? Bugcheck? Return status? Do the mambo? */
         KEBUGCHECK(MEMORY_MANAGEMENT);
      }

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

      MmLockSectionSegment(Section->Segment);
      Status = MmMapViewOfSegment(Process,
                                  AddressSpace,
                                  Section,
                                  Section->Segment,
                                  BaseAddress,
                                  *ViewSize,
                                  Protect,
                                  ViewOffset,
                                  (AllocationType & MEM_TOP_DOWN));
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
BOOLEAN STDCALL
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN PLARGE_INTEGER   NewFileSize)
{
   UNIMPLEMENTED;
   return (FALSE);
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
MmDisableModifiedWriteOfSection (DWORD Unknown0)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
MmFlushImageSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN MMFLUSH_TYPE   FlushType)
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

/*
 * @unimplemented
 */
BOOLEAN STDCALL
MmForceSectionClosed (DWORD Unknown0,
                      DWORD Unknown1)
{
   UNIMPLEMENTED;
   return (FALSE);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
MmMapViewInSystemSpace (IN PVOID SectionObject,
                        OUT PVOID * MappedBase,
                        IN OUT PULONG ViewSize)
{
   PSECTION_OBJECT Section;
   PMADDRESS_SPACE AddressSpace;
   NTSTATUS Status;

   DPRINT("MmMapViewInSystemSpace() called\n");

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = MmGetKernelAddressSpace();

   MmLockAddressSpace(AddressSpace);


   if ((*ViewSize) == 0)
   {
      (*ViewSize) = Section->MaximumSize.u.LowPart;
   }
   else if ((*ViewSize) > Section->MaximumSize.u.LowPart)
   {
      (*ViewSize) = Section->MaximumSize.u.LowPart;
   }

   MmLockSectionSegment(Section->Segment);


   Status = MmMapViewOfSegment(NULL,
                               AddressSpace,
                               Section,
                               Section->Segment,
                               MappedBase,
                               *ViewSize,
                               PAGE_READWRITE,
                               0,
                               FALSE);

   MmUnlockSectionSegment(Section->Segment);
   MmUnlockAddressSpace(AddressSpace);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
MmUnmapViewInSystemSpace (IN PVOID MappedBase)
{
   PMADDRESS_SPACE AddressSpace;
   NTSTATUS Status;

   DPRINT("MmUnmapViewInSystemSpace() called\n");

   AddressSpace = MmGetKernelAddressSpace();

   Status = MmUnmapViewOfSegment(AddressSpace, MappedBase);

   return Status;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
MmSetBankedSection (DWORD Unknown0,
                    DWORD Unknown1,
                    DWORD Unknown2,
                    DWORD Unknown3,
                    DWORD Unknown4,
                    DWORD Unknown5)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME       EXPORTED
 *  MmCreateSection@
 *
 * DESCRIPTION
 *  Creates a section object.
 *
 * ARGUMENTS
 * SectionObjiect (OUT)
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
 * @unimplemented
 */
NTSTATUS STDCALL
MmCreateSection (OUT PSECTION_OBJECT  * SectionObject,
                 IN ACCESS_MASK  DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes     OPTIONAL,
                 IN PLARGE_INTEGER  MaximumSize,
                 IN ULONG   SectionPageProtection,
                 IN ULONG   AllocationAttributes,
                 IN HANDLE   FileHandle   OPTIONAL,
                 IN PFILE_OBJECT  File      OPTIONAL)
{
   return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
