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


/* TYPES *********************************************************************/

typedef struct
{
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   ULONG Offset;
   BOOLEAN WasDirty;
   BOOLEAN Private;
}
MM_SECTION_PAGEOUT_CONTEXT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE MmSectionObjectType = NULL;

static GENERIC_MAPPING MmpSectionMapping = {
         STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
         STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
         STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
         SECTION_ALL_ACCESS};

#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFF000)
#define PFN_FROM_SSE(E)          ((E) >> PAGE_SHIFT)
#define SHARE_COUNT_FROM_SSE(E)  (((E) & 0x00000FFE) >> 1)
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MAX_SHARE_COUNT          0x7FF
#define MAKE_SSE(P, C)           ((P) | ((C) << 1))
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((S) << 1) | 0x1)

static const INFORMATION_CLASS_INFO ExSectionInfoClass[] =
{
  ICI_SQ_SAME( sizeof(SECTION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionBasicInformation */
  ICI_SQ_SAME( sizeof(SECTION_IMAGE_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionImageInformation */
};

/* FUNCTIONS *****************************************************************/

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(IN PROS_SECTION_OBJECT Section)
{
    PAGED_CODE();
    ASSERT(Section);

    /* Return the file object */
    return Section->FileObject; // Section->ControlArea->FileObject on NT
}

NTSTATUS
NTAPI
MmGetFileNameForSection(IN PROS_SECTION_OBJECT Section,
                        OUT POBJECT_NAME_INFORMATION *ModuleName)
{
    POBJECT_NAME_INFORMATION ObjectNameInfo;
    NTSTATUS Status;
    ULONG ReturnLength;

    /* Make sure it's an image section */
    *ModuleName = NULL;
    if (!(Section->AllocationAttributes & SEC_IMAGE))
    {
        /* It's not, fail */
        return STATUS_SECTION_NOT_IMAGE;
    }

    /* Allocate memory for our structure */
    ObjectNameInfo = ExAllocatePoolWithTag(PagedPool,
                                           1024,
                                           TAG('M', 'm', ' ', ' '));
    if (!ObjectNameInfo) return STATUS_NO_MEMORY;

    /* Query the name */
    Status = ObQueryNameString(Section->FileObject,
                               ObjectNameInfo,
                               1024,
                               &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, free memory */
        ExFreePoolWithTag(ObjectNameInfo, TAG('M', 'm', ' ', ' '));
        return Status;
    }

    /* Success */
    *ModuleName = ObjectNameInfo;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmGetFileNameForAddress(IN PVOID Address,
                        OUT PUNICODE_STRING ModuleName)
{
   PROS_SECTION_OBJECT Section;
   PMEMORY_AREA MemoryArea;
   PMM_AVL_TABLE AddressSpace;
   POBJECT_NAME_INFORMATION ModuleNameInformation;
   NTSTATUS Status = STATUS_ADDRESS_NOT_ASSOCIATED;

   /* Get the MM_AVL_TABLE from EPROCESS */
   if (Address >= MmSystemRangeStart)
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->VadRoot;
   }

   /* Lock address space */
   MmLockAddressSpace(AddressSpace);

   /* Locate the memory area for the process by address */
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);

   /* Make sure it's a section view type */
   if ((MemoryArea != NULL) && (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW))
   {
      /* Get the section pointer to the SECTION_OBJECT */
      Section = MemoryArea->Data.SectionData.Section;

      /* Unlock address space */
      MmUnlockAddressSpace(AddressSpace);

      /* Get the filename of the section */
      Status = MmGetFileNameForSection(Section,&ModuleNameInformation);

      if (NT_SUCCESS(Status))
      {
         /* Init modulename */
         RtlCreateUnicodeString(ModuleName,
                                ModuleNameInformation->Name.Buffer);

         /* Free temp taged buffer from MmGetFileNameForSection() */
         ExFreePoolWithTag(ModuleNameInformation, TAG('M', 'm', ' ', ' '));
         DPRINT("Found ModuleName %S by address %p\n",
                ModuleName->Buffer,Address);
      }
   }
   else
   {
      /* Unlock address space */
      MmUnlockAddressSpace(AddressSpace);
   }

   return Status;
}

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
    return STATUS_SUCCESS;
   //return KeWaitForSingleObject(&File->Lock, 0, KernelMode, FALSE, NULL);
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
NTAPI
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
            KeBugCheck(MEMORY_MANAGEMENT);
         }
         MmFreePageTablesSectionSegment(&SectionSegments[i]);
      }
      ExFreePool(ImageSectionObject->Segments);
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
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      MmFreePageTablesSectionSegment(Segment);
      ExFreePool(Segment);
      FileObject->SectionObjectPointer->DataSectionObject = NULL;
   }
}

VOID
NTAPI
MmLockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExAcquireFastMutex(&Segment->Lock);
}

VOID
NTAPI
MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExReleaseFastMutex(&Segment->Lock);
}

VOID
NTAPI
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
            KeBugCheck(MEMORY_MANAGEMENT);
         }
         memset(Table, 0, sizeof(SECTION_PAGE_TABLE));
         DPRINT("Table %x\n", Table);
      }
   }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Table->Entry[TableOffset] = Entry;
}


ULONG
NTAPI
MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset)
{
   PSECTION_PAGE_TABLE Table;
   ULONG Entry;
   ULONG DirectoryOffset;
   ULONG TableOffset;

   DPRINT("MmGetPageEntrySection(Segment %x, Offset %x)\n", Segment, Offset);

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
NTAPI
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                               ULONG Offset)
{
   ULONG Entry;

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == MAX_SHARE_COUNT)
   {
      DPRINT1("Maximum share count reached\n");
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   Entry = MAKE_SSE(PAGE_FROM_SSE(Entry), SHARE_COUNT_FROM_SSE(Entry) + 1);
   MmSetPageEntrySectionSegment(Segment, Offset, Entry);
}

BOOLEAN
NTAPI
MmUnsharePageEntrySectionSegment(PROS_SECTION_OBJECT Section,
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
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (SHARE_COUNT_FROM_SSE(Entry) == 0)
   {
      DPRINT1("Zero share count for unshare\n");
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
       KeBugCheck(MEMORY_MANAGEMENT);
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
      PFN_TYPE Page;
      BOOLEAN IsImageSection;
      ULONG FileOffset;

      FileOffset = Offset + Segment->FileOffset;

      IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

      Page = PFN_FROM_SSE(Entry);
      FileObject = Section->FileObject;
      if (FileObject != NULL &&
            !(Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
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
                KeBugCheck(MEMORY_MANAGEMENT);
            }
         }
      }

      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
      if (SavedSwapEntry == 0)
      {
         if (!PageOut &&
               ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                (Segment->Characteristics & IMAGE_SCN_MEM_SHARED)))
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
               (Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
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
                  Status = MmWriteToSwapPage(SavedSwapEntry, Page);
                  if (!NT_SUCCESS(Status))
                  {
                     DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", Status);
                      KeBugCheck(MEMORY_MANAGEMENT);
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
            KeBugCheck(MEMORY_MANAGEMENT);
         }
      }
   }
   else
   {
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
   }
   return(SHARE_COUNT_FROM_SSE(Entry) > 0);
}

BOOLEAN MiIsPageFromCache(PMEMORY_AREA MemoryArea,
                       ULONG SegOffset)
{
   if (!(MemoryArea->Data.SectionData.Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
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
NTAPI
MiReadPage(PMEMORY_AREA MemoryArea,
           ULONG SegOffset,
           PPFN_TYPE Page)
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

   ASSERT(Bcb);

   DPRINT("%S %x\n", FileObject->FileName.Buffer, FileOffset);

   /*
    * If the file system is letting us go directly to the cache and the
    * memory area was mapped at an offset in the file which is page aligned
    * then get the related cache segment.
    */
   if ((FileOffset % PAGE_SIZE) == 0 &&
       (SegOffset + PAGE_SIZE <= RawLength || !IsImageSection) &&
       !(MemoryArea->Data.SectionData.Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
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
                                     FileOffset - BaseOffset).LowPart >> PAGE_SHIFT;

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
      PageAddr = MmCreateHyperspaceMapping(*Page);
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
            MmDeleteHyperspaceMapping(PageAddr);
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
               MmDeleteHyperspaceMapping(PageAddr);
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
      MmDeleteHyperspaceMapping(PageAddr);
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmNotPresentFaultSectionView(PMM_AVL_TABLE AddressSpace,
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
   PMM_PAGEOP PageOp;
   PMM_REGION Region;
   BOOLEAN HasSwapEntry;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
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
   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress
            + MemoryArea->Data.SectionData.ViewOffset;

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
   PageOp = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset, MM_PAGEOP_PAGEIN, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
       KeBugCheck(MEMORY_MANAGEMENT);
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
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
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

         MmSharePageEntrySectionSegment(Segment, Offset);

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
            KeBugCheck(MEMORY_MANAGEMENT);
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

   HasSwapEntry = MmIsPageSwapEntry(Process, (PVOID)PAddress);
   if (HasSwapEntry)
   {
      /*
       * Must be private page we have swapped out.
       */
      SWAPENTRY SwapEntry;

      /*
       * Sanity check
       */
      if (Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         DPRINT1("Found a swaped out private page in a pagefile section.\n");
          KeBugCheck(MEMORY_MANAGEMENT);
      }

      MmUnlockSectionSegment(Segment);
      MmDeletePageFileMapping(Process, (PVOID)PAddress, &SwapEntry);

      MmUnlockAddressSpace(AddressSpace);
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
          KeBugCheck(MEMORY_MANAGEMENT);
      }

      Status = MmReadFromSwapPage(SwapEntry, Page);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      MmLockAddressSpace(AddressSpace);
      Status = MmCreateVirtualMapping(Process,
                                      Address,
                                      Region->Protect,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
      Page = Offset >> PAGE_SHIFT;
      Status = MmCreateVirtualMappingUnsafe(Process,
                                            Address,
                                            Region->Protect,
                                            &Page,
                                            1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("MmCreateVirtualMappingUnsafe failed, not out of memory\n");
          KeBugCheck(MEMORY_MANAGEMENT);
         return(Status);
      }
      /*
       * Don't add an rmap entry since the page mapped could be for
       * anything.
       */
      if (Locked)
      {
         MmLockPageUnsafe(Page);
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
   if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
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
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      Status = MmCreateVirtualMapping(Process,
                                      Address,
                                      Region->Protect,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
         DPRINT1("Someone changed ppte entry while we slept\n");
          KeBugCheck(MEMORY_MANAGEMENT);
      }

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(Process,
                                      Address,
                                      Attributes,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Unable to create virtual mapping\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
   else if (IS_SWAP_FROM_SSE(Entry))
   {
      SWAPENTRY SwapEntry;

      SwapEntry = SWAPENTRY_FROM_SSE(Entry);

      /*
      * Release all our locks and read in the page from disk
      */
      MmUnlockSectionSegment(Segment);

      MmUnlockAddressSpace(AddressSpace);

      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
          KeBugCheck(MEMORY_MANAGEMENT);
      }

      Status = MmReadFromSwapPage(SwapEntry, Page);
      if (!NT_SUCCESS(Status))
      {
          KeBugCheck(MEMORY_MANAGEMENT);
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
         DPRINT1("Someone changed ppte entry while we slept\n");
          KeBugCheck(MEMORY_MANAGEMENT);
      }

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = MAKE_SSE(Page << PAGE_SHIFT, 1);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);

      /*
       * Save the swap entry.
       */
      MmSetSavedSwapEntryPage(Page, SwapEntry);
      Status = MmCreateVirtualMapping(Process,
                                      Address,
                                      Region->Protect,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Unable to create virtual mapping\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
   else
   {
      /*
       * If the section offset is already in-memory and valid then just
       * take another reference to the page
       */

      Page = PFN_FROM_SSE(Entry);

      MmSharePageEntrySectionSegment(Segment, Offset);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(Process,
                                      Address,
                                      Attributes,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Unable to create virtual mapping\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
MmAccessFaultSectionView(PMM_AVL_TABLE AddressSpace,
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
   ULONG Offset;
   PMM_PAGEOP PageOp;
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
   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress
            + MemoryArea->Data.SectionData.ViewOffset;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   OldPage = MmGetPfnForProcess(NULL, Address);
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
      return(STATUS_ACCESS_VIOLATION);
   }

   if (IS_SWAP_FROM_SSE(Entry) ||
       PFN_FROM_SSE(Entry) != OldPage)
   {
      /* This is a private page. We must only change the page protection. */
      MmSetPageProtect(Process, PAddress, Region->Protect);
      return(STATUS_SUCCESS);
   }

   /*
    * Get or create a pageop
    */
   PageOp = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset,
                        MM_PAGEOP_ACCESSFAULT, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
       KeBugCheck(MEMORY_MANAGEMENT);
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
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
          KeBugCheck(MEMORY_MANAGEMENT);
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
       KeBugCheck(MEMORY_MANAGEMENT);
   }

   /*
    * Copy the old page
    */
   MiCopyFromUserPage(NewPage, PAddress);

   MmLockAddressSpace(AddressSpace);
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
       KeBugCheck(MEMORY_MANAGEMENT);
      return(Status);
   }
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Unable to create virtual mapping\n");
       KeBugCheck(MEMORY_MANAGEMENT);
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
   BOOLEAN WasDirty;
   PFN_TYPE Page;

   PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
   if (Process)
   {
      MmLockAddressSpace(&Process->VadRoot);
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
                                       PageOutContext->Offset,
                                       PageOutContext->WasDirty,
                                       TRUE);
      MmUnlockSectionSegment(PageOutContext->Segment);
   }
   if (Process)
   {
      MmUnlockAddressSpace(&Process->VadRoot);
   }

   if (PageOutContext->Private)
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
   }

   DPRINT("PhysicalAddress %x, Address %x\n", Page << PAGE_SHIFT, Address);
}

NTSTATUS
NTAPI
MmPageOutSectionView(PMM_AVL_TABLE AddressSpace,
                     MEMORY_AREA* MemoryArea,
                     PVOID Address,
                     PMM_PAGEOP PageOp)
{
   PFN_TYPE Page;
   MM_SECTION_PAGEOUT_CONTEXT Context;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   ULONG FileOffset;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PBCB Bcb = NULL;
   BOOLEAN DirectMapped;
   BOOLEAN IsImageSection;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;

   Context.Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress
                    + MemoryArea->Data.SectionData.ViewOffset;
   FileOffset = Context.Offset + Context.Segment->FileOffset;

   IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Context.Section->FileObject;
   DirectMapped = FALSE;
   if (FileObject != NULL &&
       !(Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
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
              Process ? Process->UniqueProcessId : 0);
       KeBugCheck(MEMORY_MANAGEMENT);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Context.Segment, Context.Offset);
   if (!MmIsPagePresent(Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              Process ? Process->UniqueProcessId : 0, Address);
       KeBugCheck(MEMORY_MANAGEMENT);
   }
   Page = MmGetPfnForProcess(Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Prepare the context structure for the rmap delete call.
    */
   Context.WasDirty = FALSE;
   if (Context.Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
         IS_SWAP_FROM_SSE(Entry) ||
         PFN_FROM_SSE(Entry) != Page)
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
          KeBugCheck(MEMORY_MANAGEMENT);
      }
   }
   else
   {
      MmReferencePage(Page);
   }

   MmDeleteAllRmaps(Page, (PVOID)&Context, MmPageOutDeleteMapping);

   /*
    * If this wasn't a private page then we should have reduced the entry to
    * zero by deleting all the rmaps.
    */
   if (!Context.Private && MmGetPageEntrySectionSegment(Context.Segment, Context.Offset) != 0)
   {
      if (!(Context.Segment->Flags & MM_PAGEFILE_SEGMENT) &&
            !(Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
      {
          KeBugCheck(MEMORY_MANAGEMENT);
      }
   }

   /*
    * If the page wasn't dirty then we can just free it as for a readonly page.
    * Since we unmapped all the mappings above we know it will not suddenly
    * become dirty.
    * If the page is from a pagefile section and has no swap entry,
    * we can't free the page at this point.
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a pagefile segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (!Context.WasDirty && SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(Page, 0);
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         MmReleasePageMemoryConsumer(MC_USER, Page);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a shared section segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (!Context.WasDirty || SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(Page, 0);
         if (SwapEntry != 0)
         {
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         }
         MmReleasePageMemoryConsumer(MC_USER, Page);
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
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      Status = CcRosUnmapCacheSegment(Bcb, FileOffset, FALSE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("CCRosUnmapCacheSegment failed, status = %x\n", Status);
         KeBugCheck(MEMORY_MANAGEMENT);
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
         KeBugCheck(MEMORY_MANAGEMENT);
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
         KeBugCheck(MEMORY_MANAGEMENT);
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
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
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
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
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
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT ||
         Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
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
         KeBugCheck(MEMORY_MANAGEMENT);
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
NTAPI
MmWritePageSectionView(PMM_AVL_TABLE AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
                       PMM_PAGEOP PageOp)
{
   ULONG Offset;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PFN_TYPE Page;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   BOOLEAN Private;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PBCB Bcb = NULL;
   BOOLEAN DirectMapped;
   BOOLEAN IsImageSection;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress
            + MemoryArea->Data.SectionData.ViewOffset;

   /*
    * Get the segment and section.
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Section->FileObject;
   DirectMapped = FALSE;
   if (FileObject != NULL &&
         !(Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
   {
      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if (((Offset + Segment->FileOffset) % PAGE_SIZE) == 0 &&
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
              Process ? Process->UniqueProcessId : 0);
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (!MmIsPagePresent(Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              Process ? Process->UniqueProcessId : 0, Address);
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   Page = MmGetPfnForProcess(Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Check for a private (COWed) page.
    */
   if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
         IS_SWAP_FROM_SSE(Entry) ||
         PFN_FROM_SSE(Entry) != Page)
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
   if (DirectMapped && !Private)
   {
      ASSERT(SwapEntry == 0);
      CcRosMarkDirtyCacheSegment(Bcb, Offset + Segment->FileOffset);
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

VOID static
MmAlterViewAttributes(PMM_AVL_TABLE AddressSpace,
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

         /*
          * If we doing COW for this segment then check if the page is
          * already private.
          */
         if (DoCOW && MmIsPagePresent(Process, Address))
         {
            ULONG Offset;
            ULONG Entry;
            PFN_TYPE Page;

            Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress
                     + MemoryArea->Data.SectionData.ViewOffset;
            Entry = MmGetPageEntrySectionSegment(Segment, Offset);
            Page = MmGetPfnForProcess(Process, Address);

            Protect = PAGE_READONLY;
            if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
                  IS_SWAP_FROM_SSE(Entry) ||
                  PFN_FROM_SSE(Entry) != Page)
            {
               Protect = NewProtect;
            }
         }

         if (MmIsPagePresent(Process, Address))
         {
            MmSetPageProtect(Process, Address,
                             Protect);
         }
      }
   }
}

NTSTATUS
NTAPI
MmProtectSectionView(PMM_AVL_TABLE AddressSpace,
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
      Info->AllocationBase = (PUCHAR)MemoryArea->StartingAddress - Segment->VirtualAddress;
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

VOID
NTAPI
MmpFreePageFileSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG Length;
   ULONG Offset;
   ULONG Entry;
   ULONG SavedSwapEntry;
   PFN_TYPE Page;

   Page = 0;

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
            Page = PFN_FROM_SSE(Entry);
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

VOID NTAPI
MmpDeleteSection(PVOID ObjectBody)
{
   PROS_SECTION_OBJECT Section = (PROS_SECTION_OBJECT)ObjectBody;

   DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
   if (Section->AllocationAttributes & SEC_IMAGE)
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
         if (SectionSegments[i].Characteristics & IMAGE_SCN_MEM_SHARED)
         {
            MmLockSectionSegment(&SectionSegments[i]);
         }
         RefCount = InterlockedDecrementUL(&SectionSegments[i].ReferenceCount);
         if (SectionSegments[i].Characteristics & IMAGE_SCN_MEM_SHARED)
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
      /*
       * NOTE: Section->Segment can be NULL for short time
       * during the section creating.
       */
      if (Section->Segment == NULL)
         return;

      if (Section->Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         MmpFreePageFileSegment(Section->Segment);
         MmFreePageTablesSectionSegment(Section->Segment);
         ExFreePool(Section->Segment);
         Section->Segment = NULL;
      }
      else
      {
         (void)InterlockedDecrementUL(&Section->Segment->ReferenceCount);
      }
   }
   if (Section->FileObject != NULL)
   {
      CcRosDereferenceCache(Section->FileObject);
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
MmCreatePhysicalMemorySection(VOID)
{
   PROS_SECTION_OBJECT PhysSection;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Obj;
   UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
   LARGE_INTEGER SectionSize;
   HANDLE Handle;

   /*
    * Create the section mapping physical memory
    */
   SectionSize.QuadPart = 0xFFFFFFFF;
   InitializeObjectAttributes(&Obj,
                              &Name,
                              OBJ_PERMANENT,
                              NULL,
                              NULL);
   Status = MmCreateSection((PVOID)&PhysSection,
                            SECTION_ALL_ACCESS,
                            &Obj,
                            &SectionSize,
                            PAGE_EXECUTE_READWRITE,
                            0,
                            NULL,
                            NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create PhysicalMemory section\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   Status = ObInsertObject(PhysSection,
                           NULL,
                           SECTION_ALL_ACCESS,
                           0,
                           NULL,
                           &Handle);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(PhysSection);
   }
   ObCloseHandle(Handle, KernelMode);
   PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;
   PhysSection->Segment->Flags &= ~MM_PAGEFILE_SEGMENT;

   return(STATUS_SUCCESS);
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

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreatePageFileSection(PROS_SECTION_OBJECT *SectionObject,
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
   Segment->FileOffset = 0;
   Segment->Protection = SectionPageProtection;
   Segment->RawLength = MaximumSize.u.LowPart;
   Segment->Length = PAGE_ROUND_UP(MaximumSize.u.LowPart);
   Segment->Flags = MM_PAGEFILE_SEGMENT;
   Segment->WriteCopy = FALSE;
   RtlZeroMemory(&Segment->PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
   Segment->VirtualAddress = 0;
   Segment->Characteristics = 0;
   *SectionObject = Section;
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
                        HANDLE FileHandle)
/*
 * Create a section backed by a data file
 */
{
   PROS_SECTION_OBJECT Section;
   NTSTATUS Status;
   LARGE_INTEGER MaximumSize;
   PFILE_OBJECT FileObject;
   PMM_SECTION_SEGMENT Segment;
   ULONG FileAccess;
   IO_STATUS_BLOCK Iosb;
   LARGE_INTEGER Offset;
   CHAR Buffer;
   FILE_STANDARD_INFORMATION FileInfo;

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
   Status = ObReferenceObjectByHandle(FileHandle,
                                      FileAccess,
                                      IoFileObjectType,
                                      ExGetPreviousMode(),
                                      (PVOID*)(PVOID)&FileObject,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Section);
      return(Status);
   }

   /*
    * FIXME: This is propably not entirely correct. We can't look into
    * the standard FCB header because it might not be initialized yet
    * (as in case of the EXT2FS driver by Manoj Paul Joseph where the
    * standard file information is filled on first request).
    */
   Status = IoQueryFileInformation(FileObject,
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

   /*
    * FIXME: Revise this once a locking order for file size changes is
    * decided
    */
   if ((UMaximumSize != NULL) && (UMaximumSize->QuadPart != 0))
   {
         MaximumSize = *UMaximumSize;
   }
   else
   {
      MaximumSize = FileInfo.EndOfFile;
      /* Mapping zero-sized files isn't allowed. */
      if (MaximumSize.QuadPart == 0)
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return STATUS_FILE_INVALID;
      }
   }

   if (MaximumSize.QuadPart > FileInfo.EndOfFile.QuadPart)
   {
      Status = IoSetInformation(FileObject,
                                FileAllocationInformation,
                                sizeof(LARGE_INTEGER),
                                &MaximumSize);
      if (!NT_SUCCESS(Status))
      {
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
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }
      if (FileObject->SectionObjectPointer == NULL ||
            FileObject->SectionObjectPointer->SharedCacheMap == NULL)
      {
         /* FIXME: handle this situation */
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
         //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
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
      Segment->VirtualAddress = 0;
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
      (void)InterlockedIncrementUL(&Segment->ReferenceCount);
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
   //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   return(STATUS_SUCCESS);
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

   ASSERT_IRQL_LESS(DISPATCH_LEVEL);

   if(Length == 0)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   FileOffset = *Offset;

   /* Negative/special offset: it cannot be used in this context */
   if(FileOffset.u.HighPart < 0)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
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
                                  TAG('M', 'm', 'X', 'r'));

   UsedSize = 0;

#if 0
   Status = MmspPageRead(File,
                         Buffer,
                         BufferSize,
                         &FileOffset,
                         &UsedSize);
#else
/*
 * FIXME: if we don't use ZwReadFile, caching is not enabled for the file and
 * nothing will work. But using ZwReadFile is wrong, and using its side effects
 * to initialize internal state is even worse. Our cache manager is in need of
 * professional help
 */
   {
      IO_STATUS_BLOCK Iosb;

      Status = ZwReadFile(File,
                          NULL,
                          NULL,
                          NULL,
                          &Iosb,
                          Buffer,
                          BufferSize,
                          &FileOffset,
                          NULL);

      if(NT_SUCCESS(Status))
      {
         UsedSize = Iosb.Information;
      }
   }
#endif

   if(NT_SUCCESS(Status) && UsedSize < OffsetAdjustment)
   {
      Status = STATUS_IN_PAGE_ERROR;
      ASSERT(!NT_SUCCESS(Status));
   }

   if(NT_SUCCESS(Status))
   {
      *Data = (PVOID)((ULONG_PTR)Buffer + OffsetAdjustment);
      *AllocBase = Buffer;
      *ReadSize = UsedSize - OffsetAdjustment;
   }
   else
   {
      ExFreePoolWithTag(Buffer, TAG('M', 'm', 'X', 'r'));
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
      ASSERT(ImageSectionObject->Segments[i].VirtualAddress >=
             ImageSectionObject->Segments[i - 1].VirtualAddress);
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
      ASSERT(ImageSectionObject->Segments[i].Length > 0);

      if(i > 0)
      {
         ASSERT(ImageSectionObject->Segments[i].VirtualAddress >=
                (ImageSectionObject->Segments[i - 1].VirtualAddress +
                 ImageSectionObject->Segments[i - 1].Length));
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
      ASSERT((ImageSectionObject->Segments[i].VirtualAddress % PAGE_SIZE) == 0);
      ASSERT((ImageSectionObject->Segments[i].Length % PAGE_SIZE) == 0);
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
      (Segment1->VirtualAddress - Segment2->VirtualAddress) >>
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
      if(ImageSectionObject->Segments[i].Length == 0)
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
         if ((ImageSectionObject->Segments[i - 1].VirtualAddress +
              ImageSectionObject->Segments[i - 1].Length) !=
              ImageSectionObject->Segments[i].VirtualAddress)
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

         VirtualAddress = EffectiveSegment->VirtualAddress;

         /* Round down the virtual address to the nearest page */
         EffectiveSegment->VirtualAddress = PAGE_ROUND_DOWN(VirtualAddress);

         /* Round up the virtual size to the nearest page */
         EffectiveSegment->Length = PAGE_ROUND_UP(VirtualAddress + EffectiveSegment->Length) -
                                    EffectiveSegment->VirtualAddress;

         /* Adjust the raw address and size */
         VirtualOffset = VirtualAddress - EffectiveSegment->VirtualAddress;

         if (EffectiveSegment->FileOffset < VirtualOffset)
         {
            return FALSE;
         }

         /*
          * Garbage in, garbage out: unaligned base addresses make the file
          * offset point in curious and odd places, but that's what we were
          * asked for
          */
         EffectiveSegment->FileOffset -= VirtualOffset;
         EffectiveSegment->RawLength += VirtualOffset;
      }
      else
      {
         PMM_SECTION_SEGMENT Segment = &ImageSectionObject->Segments[i];
         ULONG_PTR EndOfEffectiveSegment;

         EndOfEffectiveSegment = EffectiveSegment->VirtualAddress + EffectiveSegment->Length;
         ASSERT((EndOfEffectiveSegment % PAGE_SIZE) == 0);

         /*
          * The current segment begins exactly where the current effective
          * segment ended, therefore beginning a new effective segment
          */
         if (EndOfEffectiveSegment == Segment->VirtualAddress)
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
            ASSERT((EffectiveSegment->VirtualAddress % PAGE_SIZE) == 0);
            EffectiveSegment->Length = PAGE_ROUND_UP(EffectiveSegment->Length);
         }
         /*
          * The current segment is still part of the current effective segment:
          * extend the effective segment to reflect this
          */
         else if (EndOfEffectiveSegment > Segment->VirtualAddress)
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
            if (Segment->FileOffset != (EffectiveSegment->FileOffset +
                                        EffectiveSegment->RawLength))
            {
               return FALSE;
            }

            EffectiveSegment->RawLength += Segment->RawLength;

            /*
             * Extend the virtual size
             */
            ASSERT(PAGE_ROUND_UP(Segment->VirtualAddress + Segment->Length) >= EndOfEffectiveSegment);

            EffectiveSegment->Length = PAGE_ROUND_UP(Segment->VirtualAddress + Segment->Length) -
                                       EffectiveSegment->VirtualAddress;

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
            KeBugCheck(MEMORY_MANAGEMENT);
         }
      }
   }
   ImageSectionObject->NrSegments = LastSegment + 1;

   return TRUE;
}

NTSTATUS
ExeFmtpCreateImageSection(HANDLE FileHandle,
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
   Status = ExeFmtpReadFile (FileHandle,
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
                                 FileHandle,
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

   ExFreePoolWithTag(FileHeaderBuffer, TAG('M', 'm', 'X', 'r'));

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
      ImageSectionObject->Segments[i].ReferenceCount = 1;

      RtlZeroMemory(&ImageSectionObject->Segments[i].PageDirectory,
                    sizeof(ImageSectionObject->Segments[i].PageDirectory));
   }

   ASSERT(NT_SUCCESS(Status));
   return Status;
}

NTSTATUS
MmCreateImageSection(PROS_SECTION_OBJECT *SectionObject,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PLARGE_INTEGER UMaximumSize,
                     ULONG SectionPageProtection,
                     ULONG AllocationAttributes,
                     HANDLE FileHandle)
{
   PROS_SECTION_OBJECT Section;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PMM_SECTION_SEGMENT SectionSegments;
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
   ULONG i;
   ULONG FileAccess = 0;

   /*
    * Specifying a maximum size is meaningless for an image section
    */
   if (UMaximumSize != NULL)
   {
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
   Status = ObReferenceObjectByHandle(FileHandle,
                                      FileAccess,
                                      IoFileObjectType,
                                      ExGetPreviousMode(),
                                      (PVOID*)(PVOID)&FileObject,
                                      NULL);

   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

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
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;

   /*
    * Initialized caching for this file object if previously caching
    * was initialized for the same on disk file
    */
   Status = CcTryToInitializeFileCache(FileObject);

   if (!NT_SUCCESS(Status) || FileObject->SectionObjectPointer->ImageSectionObject == NULL)
   {
      NTSTATUS StatusExeFmt;

      ImageSectionObject = ExAllocatePoolWithTag(PagedPool, sizeof(MM_IMAGE_SECTION_OBJECT), TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
      {
         ObDereferenceObject(FileObject);
         ObDereferenceObject(Section);
         return(STATUS_NO_MEMORY);
      }

      RtlZeroMemory(ImageSectionObject, sizeof(MM_IMAGE_SECTION_OBJECT));

      StatusExeFmt = ExeFmtpCreateImageSection(FileHandle, ImageSectionObject);

      if (!NT_SUCCESS(StatusExeFmt))
      {
         if(ImageSectionObject->Segments != NULL)
            ExFreePool(ImageSectionObject->Segments);

         ExFreePool(ImageSectionObject);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
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
         ExFreePool(ImageSectionObject->Segments);
         ExFreePool(ImageSectionObject);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }

      if (NULL != InterlockedCompareExchangePointer(&FileObject->SectionObjectPointer->ImageSectionObject,
                                                    ImageSectionObject, NULL))
      {
         /*
          * An other thread has initialized the same image in the background
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
         ObDereferenceObject(FileObject);
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
   Section->FileObject = FileObject;
   CcRosReferenceCache(FileObject);
   //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   return(Status);
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

NTSTATUS static
MmMapViewOfSegment(PMM_AVL_TABLE AddressSpace,
                   PROS_SECTION_OBJECT Section,
                   PMM_SECTION_SEGMENT Segment,
                   PVOID* BaseAddress,
                   SIZE_T ViewSize,
                   ULONG Protect,
                   ULONG ViewOffset,
                   ULONG AllocationType)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;

   Status = MmCreateMemoryArea(AddressSpace,
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
   MArea->Data.SectionData.ViewOffset = ViewOffset;
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
                   IN ULONG_PTR ZeroBits  OPTIONAL,
                   IN SIZE_T CommitSize,
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

VOID static
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                  PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   ULONG Entry;
   PFILE_OBJECT FileObject;
   PBCB Bcb;
   ULONG Offset;
   SWAPENTRY SavedSwapEntry;
   PMM_PAGEOP PageOp;
   NTSTATUS Status;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMM_AVL_TABLE AddressSpace;
   PEPROCESS Process;

   AddressSpace = (PMM_AVL_TABLE)Context;
   Process = MmGetAddressSpaceOwner(AddressSpace);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = ((ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress) +
            MemoryArea->Data.SectionData.ViewOffset;

   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;

   PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL, Segment, Offset);

   while (PageOp)
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);

      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KeBugCheck(MEMORY_MANAGEMENT);
      }

      MmLockAddressSpace(AddressSpace);
      MmLockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp);
      PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL, Segment, Offset);
   }

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

   /*
    * For a dirty, datafile, non-private page mark it as dirty in the
    * cache manager.
    */
   if (Segment->Flags & MM_DATAFILE_SEGMENT)
   {
      if (Page == PFN_FROM_SSE(Entry) && Dirty)
      {
         FileObject = MemoryArea->Data.SectionData.Section->FileObject;
         Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
         CcRosMarkDirtyCacheSegment(Bcb, Offset + Segment->FileOffset);
         ASSERT(SwapEntry == 0);
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
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      MmFreeSwapPage(SwapEntry);
   }
   else if (Page != 0)
   {
      if (IS_SWAP_FROM_SSE(Entry) ||
          Page != PFN_FROM_SSE(Entry))
      {
         /*
          * Sanity check
          */
         if (Segment->Flags & MM_PAGEFILE_SEGMENT)
         {
            DPRINT1("Found a private page in a pagefile section.\n");
            KeBugCheck(MEMORY_MANAGEMENT);
         }
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
         MmUnsharePageEntrySectionSegment(Section, Segment, Offset, Dirty, FALSE);
      }
   }
}

static NTSTATUS
MmUnmapViewOfSegment(PMM_AVL_TABLE AddressSpace,
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
      ExFreePoolWithTag(CurrentRegion, TAG_MM_REGION);
   }

   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                MemoryArea,
                                NULL,
                                NULL);
   }
   else
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                MemoryArea,
                                MmFreeSectionPage,
                                AddressSpace);
   }
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
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
   PMM_AVL_TABLE AddressSpace;
   PROS_SECTION_OBJECT Section;
   PMM_PAGEOP PageOp;
   ULONG_PTR Offset;
    PVOID ImageBaseAddress = 0;

   DPRINT("Opening memory area Process %x BaseAddress %x\n",
          Process, BaseAddress);

   ASSERT(Process);

   AddressSpace = &Process->VadRoot;

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            BaseAddress);
   if (MemoryArea == NULL ||
       MemoryArea->Type != MEMORY_AREA_SECTION_VIEW ||
       MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      return STATUS_NOT_MAPPED_VIEW;
   }

   MemoryArea->DeleteInProgress = TRUE;

   while (MemoryArea->PageOpCount)
   {
      Offset = PAGE_ROUND_UP((ULONG_PTR)MemoryArea->EndingAddress - (ULONG_PTR)MemoryArea->StartingAddress);

      while (Offset)
      {
         Offset -= PAGE_SIZE;
         PageOp = MmCheckForPageOp(MemoryArea, NULL, NULL,
                                   MemoryArea->Data.SectionData.Segment,
                                   Offset + MemoryArea->Data.SectionData.ViewOffset);
         if (PageOp)
         {
            MmUnlockAddressSpace(AddressSpace);
            Status = MmspWaitForPageOpCompletionEvent(PageOp);
            if (Status != STATUS_SUCCESS)
            {
               DPRINT1("Failed to wait for page op, status = %x\n", Status);
               KeBugCheck(MEMORY_MANAGEMENT);
            }
            MmLockAddressSpace(AddressSpace);
            MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                                     BaseAddress);
            if (MemoryArea == NULL ||
                MemoryArea->Type != MEMORY_AREA_SECTION_VIEW)
            {
               MmUnlockAddressSpace(AddressSpace);
               return STATUS_NOT_MAPPED_VIEW;
            }
            break;
         }
      }
   }

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
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
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
         KeBugCheck(MEMORY_MANAGEMENT);
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
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

   /* Notify debugger */
   if (ImageBaseAddress) DbgkUnMapViewOfSection(ImageBaseAddress);

   MmUnlockAddressSpace(AddressSpace);
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
                  Sbi->BaseAddress = (PVOID)Section->Segment->VirtualAddress;
                  Sbi->Size.QuadPart = Section->Segment->Length;
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

   /*
    * - Acquire file extneding resource.
    * - Check if we're not resizing the section below it's actual size!
    * - Extend segments if needed.
    * - Set file information (FileAllocationInformation) to the new size.
    * - Release file extending resource.
    */

   ObDereferenceObject(Section);

   return STATUS_NOT_IMPLEMENTED;
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
   PMM_AVL_TABLE AddressSpace;
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
                   IN ULONG_PTR ZeroBits,
                   IN SIZE_T CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PSIZE_T ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG Protect)
{
   PROS_SECTION_OBJECT Section;
   PMM_AVL_TABLE AddressSpace;
   ULONG ViewOffset;
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
   AddressSpace = &Process->VadRoot;

   AllocationType |= (Section->AllocationAttributes & SEC_NO_CHANGE);

   MmLockAddressSpace(AddressSpace);

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      ULONG_PTR ImageBase;
      ULONG ImageSize;
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
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            ULONG_PTR MaxExtent;
            MaxExtent = (ULONG_PTR)SectionSegments[i].VirtualAddress +
                        SectionSegments[i].Length;
            ImageSize = max(ImageSize, MaxExtent);
         }
      }

      ImageSectionObject->ImageSize = ImageSize;

      /* Check there is enough space to map the section at that point. */
      if (MmLocateMemoryAreaByRegion(AddressSpace, (PVOID)ImageBase,
                                     PAGE_ROUND_UP(ImageSize)) != NULL)
      {
         /* Fail if the user requested a fixed base address. */
         if ((*BaseAddress) != NULL)
         {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
         /* Otherwise find a gap to map the image. */
         ImageBase = (ULONG_PTR)MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), PAGE_SIZE, FALSE);
         if (ImageBase == 0)
         {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBase + (ULONG_PTR)SectionSegments[i].VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MmMapViewOfSegment(AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length,
                                        SectionSegments[i].Protection,
                                        0,
                                        0);
            MmUnlockSectionSegment(&SectionSegments[i]);
            if (!NT_SUCCESS(Status))
            {
               MmUnlockAddressSpace(AddressSpace);
               return(Status);
            }
         }
      }

      *BaseAddress = (PVOID)ImageBase;
   }
   else
   {
      /* check for write access */
      if ((Protect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)) &&
          !(Section->SectionPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE)))
      {
         MmUnlockAddressSpace(AddressSpace);
         return STATUS_SECTION_PROTECTION;
      }
      /* check for read access */
      if ((Protect & (PAGE_READONLY|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY)) &&
          !(Section->SectionPageProtection & (PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
      {
         MmUnlockAddressSpace(AddressSpace);
         return STATUS_SECTION_PROTECTION;
      }
      /* check for execute access */
      if ((Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) &&
          !(Section->SectionPageProtection & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))
      {
         MmUnlockAddressSpace(AddressSpace);
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
      Status = MmMapViewOfSegment(AddressSpace,
                                  Section,
                                  Section->Segment,
                                  BaseAddress,
                                  *ViewSize,
                                  Protect,
                                  ViewOffset,
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
   UNIMPLEMENTED;
   return (FALSE);
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
   PMM_AVL_TABLE AddressSpace;
   NTSTATUS Status;

   DPRINT("MmMapViewInSystemSpace() called\n");

   Section = (PROS_SECTION_OBJECT)SectionObject;
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


   Status = MmMapViewOfSegment(AddressSpace,
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
   PMM_AVL_TABLE AddressSpace;
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

/*
 * @unimplemented
 */
NTSTATUS NTAPI
MmSetBankedSection (ULONG Unknown0,
                    ULONG Unknown1,
                    ULONG Unknown2,
                    ULONG Unknown3,
                    ULONG Unknown4,
                    ULONG Unknown5)
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
                 IN HANDLE   FileHandle   OPTIONAL,
                 IN PFILE_OBJECT  File      OPTIONAL)
{
   ULONG Protection;
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

   if (AllocationAttributes & SEC_IMAGE)
   {
      return(MmCreateImageSection(SectionObject,
                                  DesiredAccess,
                                  ObjectAttributes,
                                  MaximumSize,
                                  SectionPageProtection,
                                  AllocationAttributes,
                                  FileHandle));
   }

   if (FileHandle != NULL)
   {
      return(MmCreateDataFileSection(SectionObject,
                                     DesiredAccess,
                                     ObjectAttributes,
                                     MaximumSize,
                                     SectionPageProtection,
                                     AllocationAttributes,
                                     FileHandle));
   }

   return(MmCreatePageFileSection(SectionObject,
                                  DesiredAccess,
                                  ObjectAttributes,
                                  MaximumSize,
                                  SectionPageProtection,
                                  AllocationAttributes));
}

NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(IN HANDLE ProcessHandle,
                            IN OUT PULONG_PTR NumberOfPages,
                            IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtMapUserPhysicalPages(IN PVOID VirtualAddresses,
                       IN ULONG_PTR NumberOfPages,
                       IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(IN PVOID *VirtualAddresses,
                              IN ULONG_PTR NumberOfPages,
                              IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtFreeUserPhysicalPages(IN HANDLE ProcessHandle,
                        IN OUT PULONG_PTR NumberOfPages,
                        IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
