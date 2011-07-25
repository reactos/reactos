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
#include "newmm.h"
#include "../newcc.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

extern KEVENT MpwThreadEvent;
extern KSPIN_LOCK MiSectionPageTableLock;

/* GLOBALS *******************************************************************/

ULONG_PTR MmSubsectionBase;

NTSTATUS
NTAPI
MiSimpleRead
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset,
 PVOID Buffer, 
 ULONG Length,
 PIO_STATUS_BLOCK ReadStatus);

static const INFORMATION_CLASS_INFO ExSectionInfoClass[] =
{
  ICI_SQ_SAME( sizeof(SECTION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionBasicInformation */
  ICI_SQ_SAME( sizeof(SECTION_IMAGE_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionImageInformation */
};

/* FUNCTIONS *****************************************************************/

/* Note: Mmsp prefix denotes "Memory Manager Section Private". */

VOID
NTAPI
_MmLockCacheSectionSegment(PMM_CACHE_SECTION_SEGMENT Segment, const char *file, int line)
{
	DPRINT("MmLockSectionSegment(%p,%s:%d)\n", Segment, file, line);
	ExAcquireFastMutex(&Segment->Lock);
}

VOID
NTAPI
_MmUnlockCacheSectionSegment(PMM_CACHE_SECTION_SEGMENT Segment, const char *file, int line)
{
	ExReleaseFastMutex(&Segment->Lock);
	DPRINT("MmUnlockSectionSegment(%p,%s:%d)\n", Segment, file, line);
}

NTSTATUS
NTAPI
MiZeroFillSection
(PVOID Address,
 PLARGE_INTEGER FileOffsetPtr,
 ULONG Length)
{
	PFN_NUMBER Page;
	PMMSUPPORT AddressSpace;
	PMEMORY_AREA MemoryArea;
	PMM_CACHE_SECTION_SEGMENT Segment;
	LARGE_INTEGER FileOffset = *FileOffsetPtr, End, FirstMapped;
	DPRINT("MiZeroFillSection(Address %x,Offset %x,Length %x)\n", Address, FileOffset.LowPart, Length);
	AddressSpace = MmGetKernelAddressSpace();
	MmLockAddressSpace(AddressSpace);
	MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
	MmUnlockAddressSpace(AddressSpace);
	if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_SECTION_VIEW) 
	{
		return STATUS_NOT_MAPPED_DATA;
	}

	Segment = MemoryArea->Data.CacheData.Segment;
	End.QuadPart = FileOffset.QuadPart + Length;
	End.LowPart = PAGE_ROUND_DOWN(End.LowPart);
	FileOffset.LowPart = PAGE_ROUND_UP(FileOffset.LowPart);
	FirstMapped.QuadPart = MemoryArea->Data.CacheData.ViewOffset.QuadPart;
	DPRINT
		("Pulling zero pages for %08x%08x-%08x%08x\n",
		 FileOffset.u.HighPart, FileOffset.u.LowPart,
		 End.u.HighPart, End.u.LowPart);
	while (FileOffset.QuadPart < End.QuadPart)
	{
		PVOID Address;
		ULONG Entry;

		if (!NT_SUCCESS(MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &Page)))
			break;

		MmLockAddressSpace(AddressSpace);
		MmLockCacheSectionSegment(Segment);

		Entry = MiGetPageEntryCacheSectionSegment(Segment, &FileOffset);
		if (Entry == 0)
		{
			MiSetPageEntryCacheSectionSegment(Segment, &FileOffset, MAKE_PFN_SSE(Page));
			Address = ((PCHAR)MemoryArea->StartingAddress) + FileOffset.QuadPart - FirstMapped.QuadPart;
			MmReferencePage(Page);
			MmCreateVirtualMapping(NULL, Address, PAGE_READWRITE, &Page, 1);
			MmInsertRmap(Page, NULL, Address);
		}
		else
			MmReleasePageMemoryConsumer(MC_CACHE, Page);

		MmUnlockCacheSectionSegment(Segment);
		MmUnlockAddressSpace(AddressSpace);

		FileOffset.QuadPart += PAGE_SIZE;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
_MiFlushMappedSection
(PVOID BaseAddress,
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
	PMM_CACHE_SECTION_SEGMENT Segment;
	ULONG_PTR BeginningAddress, EndingAddress;
	LARGE_INTEGER ViewOffset;
	LARGE_INTEGER FileOffset;
	PFN_NUMBER Page;
	PPFN_NUMBER Pages;

	DPRINT("MiFlushMappedSection(%x,%08x,%x,%d,%s:%d)\n", BaseAddress, BaseOffset->LowPart, FileSize, WriteData, File, Line);

	MmLockAddressSpace(AddressSpace);
	MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
	if (!MemoryArea || MemoryArea->Type != MEMORY_AREA_CACHE) 
	{
		MmUnlockAddressSpace(AddressSpace);
		DPRINT("STATUS_NOT_MAPPED_DATA\n");
		return STATUS_NOT_MAPPED_DATA;
	}
	BeginningAddress = PAGE_ROUND_DOWN((ULONG_PTR)MemoryArea->StartingAddress);
	EndingAddress = PAGE_ROUND_UP((ULONG_PTR)MemoryArea->EndingAddress);
	Segment = MemoryArea->Data.CacheData.Segment;
	ViewOffset.QuadPart = MemoryArea->Data.CacheData.ViewOffset.QuadPart;

	ASSERT(ViewOffset.QuadPart == BaseOffset->QuadPart);

	MmLockCacheSectionSegment(Segment);

	Pages = ExAllocatePool
		(NonPagedPool, 
		 sizeof(PFN_NUMBER) * 
		 ((EndingAddress - BeginningAddress) >> PAGE_SHIFT));

	if (!Pages)
	{
		ASSERT(FALSE);
	}

	DPRINT("Getting pages in range %08x-%08x\n", BeginningAddress, EndingAddress);

	for (PageAddress = BeginningAddress;
		 PageAddress < EndingAddress;
		 PageAddress += PAGE_SIZE)
	{
		ULONG Entry;
		FileOffset.QuadPart = ViewOffset.QuadPart + PageAddress - BeginningAddress;
		Entry =
			MiGetPageEntryCacheSectionSegment
			(MemoryArea->Data.CacheData.Segment, 
			 &FileOffset);
		Page = PFN_FROM_SSE(Entry);
		if (Entry != 0 && !IS_SWAP_FROM_SSE(Entry) && 
			(MmIsDirtyPageRmap(Page) || IS_DIRTY_SSE(Entry)) &&
			FileOffset.QuadPart < FileSize->QuadPart)
		{
			Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = Page;
		}
		else
			Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT] = 0;
	}

	MmUnlockCacheSectionSegment(Segment);
	MmUnlockAddressSpace(AddressSpace);

	for (PageAddress = BeginningAddress;
		 PageAddress < EndingAddress;
		 PageAddress += PAGE_SIZE)
	{
		FileOffset.QuadPart = ViewOffset.QuadPart + PageAddress - BeginningAddress;
		Page = Pages[(PageAddress - BeginningAddress) >> PAGE_SHIFT];
		if (Page)
		{
			ULONG Entry;
			if (WriteData) {
				DPRINT("MiWriteBackPage(%wZ,addr %x,%08x%08x)\n", &Segment->FileObject->FileName, PageAddress, FileOffset.u.HighPart, FileOffset.u.LowPart);
				Status = MiWriteBackPage(Segment->FileObject, &FileOffset, PAGE_SIZE, Page);
			} else
				Status = STATUS_SUCCESS;

			if (NT_SUCCESS(Status)) {
				MmLockAddressSpace(AddressSpace);
				MmSetCleanAllRmaps(Page);
				MmLockCacheSectionSegment(Segment);
				Entry = MiGetPageEntryCacheSectionSegment(Segment, &FileOffset);
				if (Entry && !IS_SWAP_FROM_SSE(Entry) && PFN_FROM_SSE(Entry) == Page)
					MiSetPageEntryCacheSectionSegment(Segment, &FileOffset, CLEAN_SSE(Entry));
				MmUnlockCacheSectionSegment(Segment);
				MmUnlockAddressSpace(AddressSpace);
			} else {
				DPRINT
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

VOID
NTAPI
MmFinalizeSegment(PMM_CACHE_SECTION_SEGMENT Segment)
{
	KIRQL OldIrql = 0;

	MmLockCacheSectionSegment(Segment);
	if (Segment->Flags & MM_DATAFILE_SEGMENT) {
		KeAcquireSpinLock(&Segment->FileObject->IrpListLock, &OldIrql);
		if (Segment->Flags & MM_SEGMENT_FINALIZE) {
			KeReleaseSpinLock(&Segment->FileObject->IrpListLock, OldIrql);
			MmUnlockCacheSectionSegment(Segment);
			return;
		} else {
			Segment->Flags |= MM_SEGMENT_FINALIZE;
		}
	}
	DPRINTC("Finalizing segment %x\n", Segment);
	if (Segment->Flags & MM_DATAFILE_SEGMENT)
	{
		//Segment->FileObject->SectionObjectPointer->DataSectionObject = NULL;
		KeReleaseSpinLock(&Segment->FileObject->IrpListLock, OldIrql);
		MiFreePageTablesSectionSegment(Segment, MiFreeSegmentPage);
		MmUnlockCacheSectionSegment(Segment);
		ObDereferenceObject(Segment->FileObject);
	} else {
		MiFreePageTablesSectionSegment(Segment, MiFreeSegmentPage);
		MmUnlockCacheSectionSegment(Segment);		
	}
	DPRINTC("Segment %x destroy\n", Segment);
	ExFreePoolWithTag(Segment, TAG_MM_SECTION_SEGMENT);
}

NTSTATUS
NTAPI
MmCreateCacheSection
(PMM_CACHE_SECTION_SEGMENT *SegmentObject,
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
   NTSTATUS Status;
   ULARGE_INTEGER MaximumSize;
   PMM_CACHE_SECTION_SEGMENT Segment;
   ULONG FileAccess;
   IO_STATUS_BLOCK Iosb;
   CC_FILE_SIZES FileSizes;
   FILE_STANDARD_INFORMATION FileInfo;

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

   Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_CACHE_SECTION_SEGMENT),
				   TAG_MM_SECTION_SEGMENT);
   if (Segment == NULL)
   {
       return(STATUS_NO_MEMORY);
   }

   ExInitializeFastMutex(&Segment->Lock);

   Segment->ReferenceCount = 1;
   
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
	   Segment->RawLength = MaximumSize;
	   Segment->Length.QuadPart = PAGE_ROUND_UP(Segment->RawLength.QuadPart);
   }

   MiInitializeSectionPageTable(Segment);
   MmUnlockCacheSectionSegment(Segment);

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
		   DPRINT("Could not expand section\n");
		   return Status;
	   }
   }

   DPRINTC("Segment %x created (%x)\n", Segment, Segment->Flags);

   *SegmentObject = Segment;

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
_MiMapViewOfSegment
(PMMSUPPORT AddressSpace,
 PMM_CACHE_SECTION_SEGMENT Segment,
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
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;

   Status = MmCreateMemoryArea
	   (AddressSpace,
		MEMORY_AREA_CACHE,
		BaseAddress,
		ViewSize,
		Protect,
		&MArea,
		FALSE,
		AllocationType,
		BoundaryAddressMultiple);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Mapping between 0x%.8X and 0x%.8X failed (%X).\n",
			  (*BaseAddress), (char*)(*BaseAddress) + ViewSize, Status);
      return(Status);
   }

   DPRINTC("MiMapViewOfSegment %x %x %x %x %x %wZ %s:%d\n", MmGetAddressSpaceOwner(AddressSpace), *BaseAddress, Segment, ViewOffset ? ViewOffset->LowPart : 0, ViewSize, Segment->FileObject ? &Segment->FileObject->FileName : NULL, file, line);

   MArea->Data.CacheData.Segment = Segment;
   if (ViewOffset)
	   MArea->Data.CacheData.ViewOffset = *ViewOffset;
   else
	   MArea->Data.CacheData.ViewOffset.QuadPart = 0;

#if 0
   MArea->NotPresent = MmNotPresentFaultPageFile;
   MArea->AccessFault = MiCowSectionPage;
   MArea->PageOut = MmPageOutPageFileView;
#endif

   DPRINTC
	   ("MiMapViewOfSegment(P %x, A %x, T %x)\n",
		MmGetAddressSpaceOwner(AddressSpace), *BaseAddress, MArea->Type);

   return(STATUS_SUCCESS);
}

VOID
NTAPI
MiFreeSegmentPage
(PMM_CACHE_SECTION_SEGMENT Segment,
 PLARGE_INTEGER FileOffset)
{
	ULONG Entry;
	PFILE_OBJECT FileObject = Segment->FileObject;

	Entry = MiGetPageEntryCacheSectionSegment(Segment, FileOffset);
	DPRINTC("MiFreeSegmentPage(%x:%08x%08x -> Entry %x\n",
			Segment, FileOffset->HighPart, FileOffset->LowPart, Entry);

	if (Entry && !IS_SWAP_FROM_SSE(Entry))
	{
		// The segment is carrying a dirty page.
		PFN_NUMBER OldPage = PFN_FROM_SSE(Entry);
		if (IS_DIRTY_SSE(Entry) && FileObject)
		{
			DPRINT("MiWriteBackPage(%x,%wZ,%08x%08x)\n", Segment, &FileObject->FileName, FileOffset->u.HighPart, FileOffset->u.LowPart);
			MiWriteBackPage(FileObject, FileOffset, PAGE_SIZE, OldPage);
		}
		DPRINTC("Free page %x (off %x from %x) (ref ct %d, ent %x, dirty? %s)\n", OldPage, FileOffset->LowPart, Segment, MmGetReferenceCountPage(OldPage), Entry, IS_DIRTY_SSE(Entry) ? "true" : "false");

		MiSetPageEntryCacheSectionSegment(Segment, FileOffset, 0);
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
MmFreeCacheSectionPage
(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
 PFN_NUMBER Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   ULONG Entry;
   PVOID *ContextData = Context;
   PMMSUPPORT AddressSpace;
   PEPROCESS Process;
   PMM_CACHE_SECTION_SEGMENT Segment;
   LARGE_INTEGER Offset;

   DPRINT("MmFreeSectionPage(%x,%x,%x,%x,%d)\n", MmGetAddressSpaceOwner(ContextData[0]), Address, Page, SwapEntry, Dirty);

   AddressSpace = ContextData[0];
   Process = MmGetAddressSpaceOwner(AddressSpace);
   Address = (PVOID)PAGE_ROUND_DOWN(Address);
   Segment = ContextData[1];
   Offset.QuadPart = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress +
	   MemoryArea->Data.CacheData.ViewOffset.QuadPart;

   Entry = MiGetPageEntryCacheSectionSegment(Segment, &Offset);

   if (Page)
   {
	   DPRINT("Removing page %x:%x -> %x\n", Segment, Offset.LowPart, Entry);
	   MmSetSavedSwapEntryPage(Page, 0);
	   MmDeleteRmap(Page, Process, Address);
	   MmDeleteVirtualMapping(Process, Address, FALSE, NULL, NULL);
	   MmReleasePageMemoryConsumer(MC_CACHE, Page);
   }
   if (Page != 0 && PFN_FROM_SSE(Entry) == Page && Dirty)
   {
	   DPRINT("Freeing section page %x:%x -> %x\n", Segment, Offset.LowPart, Entry);
	   MiSetPageEntryCacheSectionSegment(Segment, &Offset, DIRTY_SSE(Entry));
   }
   else if (SwapEntry != 0)
   {
      MmFreeSwapPage(SwapEntry);
   }
}

NTSTATUS
NTAPI
MmUnmapViewOfCacheSegment
(PMMSUPPORT AddressSpace,
 PVOID BaseAddress)
{
   PVOID Context[2];
   PMEMORY_AREA MemoryArea;
   PMM_CACHE_SECTION_SEGMENT Segment;

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   if (MemoryArea == NULL)
   {
	  ASSERT(MemoryArea);
      return(STATUS_UNSUCCESSFUL);
   }

   MemoryArea->DeleteInProgress = TRUE;
   Segment = MemoryArea->Data.CacheData.Segment;
   MemoryArea->Data.CacheData.Segment = NULL;
   
   MmLockCacheSectionSegment(Segment);

   Context[0] = AddressSpace;
   Context[1] = Segment;
   DPRINT("MmFreeMemoryArea(%x,%x)\n", MmGetAddressSpaceOwner(AddressSpace), MemoryArea->StartingAddress);
   MmFreeMemoryArea(AddressSpace, MemoryArea, MmFreeCacheSectionPage, Context);

   MmUnlockCacheSectionSegment(Segment);

   DPRINTC("MiUnmapViewOfSegment %x %x %x\n", MmGetAddressSpaceOwner(AddressSpace), BaseAddress, Segment);

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmExtendCacheSection
(PMM_CACHE_SECTION_SEGMENT Segment,
 PLARGE_INTEGER NewSize,
 BOOLEAN ExtendFile)
{
	LARGE_INTEGER OldSize;
	DPRINT("Extend Segment %x\n", Segment);

	MmLockCacheSectionSegment(Segment);
	OldSize.QuadPart = Segment->RawLength.QuadPart;
	MmUnlockCacheSectionSegment(Segment);

	DPRINT("OldSize %08x%08x NewSize %08x%08x\n",
		   OldSize.u.HighPart, OldSize.u.LowPart,
		   NewSize->u.HighPart, NewSize->u.LowPart);

	if (ExtendFile && OldSize.QuadPart < NewSize->QuadPart)
	{
		NTSTATUS Status;
		Status = IoSetInformation(Segment->FileObject, FileEndOfFileInformation, sizeof(LARGE_INTEGER), NewSize);
		if (!NT_SUCCESS(Status)) return Status;
	}

	MmLockCacheSectionSegment(Segment);
	Segment->RawLength.QuadPart = NewSize->QuadPart;
	Segment->Length.QuadPart = MAX(Segment->Length.QuadPart, PAGE_ROUND_UP(Segment->RawLength.LowPart));
	MmUnlockCacheSectionSegment(Segment);
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI
MmMapCacheViewInSystemSpaceAtOffset
(IN PMM_CACHE_SECTION_SEGMENT Segment,
 OUT PVOID *MappedBase,
 PLARGE_INTEGER FileOffset,
 IN OUT PULONG ViewSize)
{
    PMMSUPPORT AddressSpace;
    NTSTATUS Status;
    
    DPRINT("MmMapViewInSystemSpaceAtOffset() called offset %08x%08x\n", FileOffset->HighPart, FileOffset->LowPart);
    
    AddressSpace = MmGetKernelAddressSpace();
    
    MmLockAddressSpace(AddressSpace);
    MmLockCacheSectionSegment(Segment);
    
    Status = MiMapViewOfSegment
		(AddressSpace,
		 Segment,
		 MappedBase,
		 *ViewSize,
		 PAGE_READWRITE,
		 FileOffset,
		 0);
    
    MmUnlockCacheSectionSegment(Segment);
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

/* EOF */
