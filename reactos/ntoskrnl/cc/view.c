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
/* $Id: view.c,v 1.55 2003/01/11 15:24:38 hbirr Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * PORTABILITY:     Checked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* NOTES **********************************************************************
 *
 * This is not the NT implementation of a file cache nor anything much like 
 * it. 
 *
 * The general procedure for a filesystem to implement a read or write 
 * dispatch routine is as follows
 * 
 * (1) If caching for the FCB hasn't been initiated then so do by calling
 * CcInitializeFileCache.
 * 
 * (2) For each 4k region which is being read or written obtain a cache page
 * by calling CcRequestCachePage. 
 *
 * (3) If either the page is being read or not completely written, and it is 
 * not up to date then read its data from the underlying medium. If the read
 * fails then call CcReleaseCachePage with VALID as FALSE and return a error.  
 * 
 * (4) Copy the data into or out of the page as necessary.
 * 
 * (5) Release the cache page
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/*
 * If CACHE_BITMAP is defined, the cache manager uses one large memory region 
 * within the kernel address space and allocate/deallocate space from this block 
 * over a bitmap. If CACHE_BITMAP is used, the size of the mdl mapping region 
 * must be reduced (ntoskrnl\mm\mdl.c, MI_MDLMAPPING_REGION_SIZE).
 */
// #define CACHE_BITMAP

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) % (S)) ? ROUND_UP(N, S) - S : N)

#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')
#define TAG_IBCB  TAG('i', 'B', 'C', 'B')

static LIST_ENTRY DirtySegmentListHead;
static LIST_ENTRY CacheSegmentListHead;
static LIST_ENTRY CacheSegmentLRUListHead;
static LIST_ENTRY ClosedListHead;

static FAST_MUTEX ViewLock;

#ifdef CACHE_BITMAP
#define	CI_CACHESEG_MAPPING_REGION_SIZE	(128*1024*1024)

static PVOID CiCacheSegMappingRegionBase = NULL;
static RTL_BITMAP CiCacheSegMappingRegionAllocMap;
static ULONG CiCacheSegMappingRegionHint;
static KSPIN_LOCK CiCacheSegMappingRegionLock;
#endif

NPAGED_LOOKASIDE_LIST iBcbLookasideList;
static NPAGED_LOOKASIDE_LIST BcbLookasideList;
static NPAGED_LOOKASIDE_LIST CacheSegLookasideList;

static ULONG CcTimeStamp;
static KEVENT LazyCloseThreadEvent;
static HANDLE LazyCloseThreadHandle;
static CLIENT_ID LazyCloseThreadId;
static volatile BOOLEAN LazyCloseThreadShouldTerminate;

void * alloca(size_t size);

NTSTATUS
CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg);

/* FUNCTIONS *****************************************************************/

NTSTATUS STATIC
CcRosFlushCacheSegment(PCACHE_SEGMENT CacheSegment)
{
  NTSTATUS Status;
  KIRQL oldIrql;
  Status = WriteCacheSegment(CacheSegment);
  if (NT_SUCCESS(Status))
    {
      ExAcquireFastMutex(&ViewLock);
      KeAcquireSpinLock(&CacheSegment->Bcb->BcbLock, &oldIrql);
      CacheSegment->Dirty = FALSE;
      RemoveEntryList(&CacheSegment->DirtySegmentListEntry);
      CacheSegment->ReferenceCount--;
      KeReleaseSpinLock(&CacheSegment->Bcb->BcbLock, oldIrql);
      ExReleaseFastMutex(&ViewLock);
    }
  return(Status);
}

VOID CcRosRemoveUnusedFiles(VOID);


NTSTATUS
CcRosFlushDirtyPages(ULONG Target, PULONG Count)
{
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  ULONG PagesPerSegment;
  BOOLEAN Locked;
  NTSTATUS Status;

  DPRINT("CcRosFlushDirtyPages(Target %d)\n", Target);

  (*Count) = 0;

  ExAcquireFastMutex(&ViewLock);
  current_entry = DirtySegmentListHead.Flink;
  if (current_entry == &DirtySegmentListHead)
  {
     DPRINT("No Dirty pages\n");
  }
  while (current_entry != &DirtySegmentListHead && Target > 0)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				  DirtySegmentListEntry);
      current_entry = current_entry->Flink;
      Locked = ExTryToAcquireFastMutex(&current->Lock);
      if (!Locked)
	{
	  continue;
	}
      assert(current->Dirty);
      if (current->ReferenceCount > 1)
	{
	  ExReleaseFastMutex(&current->Lock);
	  continue;
	}
      ExReleaseFastMutex(&ViewLock);
      PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;
      Status = CcRosFlushCacheSegment(current);      
      ExReleaseFastMutex(&current->Lock);
      if (!NT_SUCCESS(Status))
      {
	 DPRINT1("CC: Failed to flush cache segment.\n");
      }
      else
      {
         (*Count) += PagesPerSegment;
         Target -= PagesPerSegment;     
      }
      ExAcquireFastMutex(&ViewLock);
      current_entry = DirtySegmentListHead.Flink;
    }
  ExReleaseFastMutex(&ViewLock);
  DPRINT("CcRosFlushDirtyPages() finished\n");

  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosTrimCache(ULONG Target, ULONG Priority, PULONG NrFreed)
/*
 * FUNCTION: Try to free some memory from the file cache.
 * ARGUMENTS:
 *       Target - The number of pages to be freed.
 *       Priority - The priority of free (currently unused).
 *       NrFreed - Points to a variable where the number of pages 
 *                 actually freed is returned.
 */
{
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  ULONG PagesPerSegment;
  ULONG PagesFreed;
  KIRQL oldIrql;
  LIST_ENTRY FreeList;

  DPRINT("CcRosTrimCache(Target %d)\n", Target);

  *NrFreed = 0;
  
  InitializeListHead(&FreeList);
  
  ExAcquireFastMutex(&ViewLock);
  current_entry = CacheSegmentLRUListHead.Flink;
  while (current_entry != &CacheSegmentLRUListHead && Target > 0)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				  CacheSegmentLRUListEntry);
      current_entry = current_entry->Flink;
      
      KeAcquireSpinLock(&current->Bcb->BcbLock, &oldIrql);
      if (current->ReferenceCount == 0)
      {
         RemoveEntryList(&current->BcbSegmentListEntry);
         KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
         RemoveEntryList(&current->CacheSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentLRUListEntry);
	 InsertHeadList(&FreeList, &current->BcbSegmentListEntry);
      }
      else
      {
         KeReleaseSpinLock(&current->Bcb->BcbLock, oldIrql);
      }
  }
  ExReleaseFastMutex(&ViewLock);

  while (!IsListEmpty(&FreeList))
  {
     current_entry = RemoveHeadList(&FreeList);
     current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				 BcbSegmentListEntry);
     PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;
     PagesFreed = min(PagesPerSegment, Target);
     Target -= PagesFreed;
     (*NrFreed) += PagesFreed;
     CcRosInternalFreeCacheSegment(current);
  }

  DPRINT("CcRosTrimCache() finished\n");
  return(STATUS_SUCCESS);
}

NTSTATUS 
CcRosReleaseCacheSegment(PBCB Bcb,
			 PCACHE_SEGMENT CacheSeg,
			 BOOLEAN Valid,
			 BOOLEAN Dirty,
			 BOOLEAN Mapped)
{
  BOOLEAN WasDirty = CacheSeg->Dirty;
  KIRQL oldIrql;

  assert(Bcb);

  DPRINT("CcReleaseCacheSegment(Bcb %x, CacheSeg %x, Valid %d)\n",
	 Bcb, CacheSeg, Valid);

  CacheSeg->Valid = Valid;
  CacheSeg->Dirty = CacheSeg->Dirty || Dirty;

  ExAcquireFastMutex(&ViewLock);
  if (!WasDirty && CacheSeg->Dirty)
    {
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
    }
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  InsertTailList(&CacheSegmentLRUListHead, &CacheSeg->CacheSegmentLRUListEntry);

  if (Mapped)
  {
     CacheSeg->MappedCount++;
  }
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  CacheSeg->ReferenceCount--;
  if (Mapped && CacheSeg->MappedCount == 1)
  {
      CacheSeg->ReferenceCount++;
  }
  if (!WasDirty && CacheSeg->Dirty)
  {
      CacheSeg->ReferenceCount++;
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  ExReleaseFastMutex(&ViewLock);
  ExReleaseFastMutex(&CacheSeg->Lock);
  
  return(STATUS_SUCCESS);
}

PCACHE_SEGMENT 
CcRosLookupCacheSegment(PBCB Bcb, ULONG FileOffset)
{
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  KIRQL oldIrql;

  assert(Bcb);

  DPRINT("CcRosLookupCacheSegment(Bcb %x, FileOffset %d)\n", Bcb, FileOffset);

  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  current_entry = Bcb->BcbSegmentListHead.Flink;
  while (current_entry != &Bcb->BcbSegmentListHead)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				  BcbSegmentListEntry);
      if (current->FileOffset <= FileOffset &&
	  (current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
	{
	  current->ReferenceCount++;
	  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
	  return(current);
	}
      current_entry = current_entry->Flink;
    }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  return(NULL);
}

NTSTATUS
CcRosMarkDirtyCacheSegment(PBCB Bcb, ULONG FileOffset)
{
  PCACHE_SEGMENT CacheSeg;
  KIRQL oldIrql;

  assert(Bcb);

  DPRINT("CcRosMarkDirtyCacheSegment(Bcb %x, FileOffset %d)\n", Bcb, FileOffset);

  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      KeBugCheck(0);
    }
  ExAcquireFastMutex(&CacheSeg->Lock);
  if (!CacheSeg->Dirty)
    {
      ExAcquireFastMutex(&ViewLock);
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
      ExReleaseFastMutex(&ViewLock);
    }
  else
  {
     KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
     CacheSeg->ReferenceCount--;
     KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  }


  CacheSeg->Dirty = TRUE;
  ExReleaseFastMutex(&CacheSeg->Lock);

  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosUnmapCacheSegment(PBCB Bcb, ULONG FileOffset, BOOLEAN NowDirty)
{
  PCACHE_SEGMENT CacheSeg;
  BOOLEAN WasDirty;
  KIRQL oldIrql;

  assert(Bcb);

  DPRINT("CcRosUnmapCacheSegment(Bcb %x, FileOffset %d, NowDirty %d)\n",
          Bcb, FileOffset, NowDirty);

  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  ExAcquireFastMutex(&CacheSeg->Lock);

  WasDirty = CacheSeg->Dirty;
  CacheSeg->Dirty = CacheSeg->Dirty || NowDirty;

  CacheSeg->MappedCount--;

  if (!WasDirty && NowDirty)
  {
     ExAcquireFastMutex(&ViewLock);
     InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
     ExReleaseFastMutex(&ViewLock);
  }

  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  CacheSeg->ReferenceCount--;
  if (!WasDirty && NowDirty)
  {
     CacheSeg->ReferenceCount++;
  }
  if (CacheSeg->MappedCount == 0)
  {
     CacheSeg->ReferenceCount--;
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);

  ExReleaseFastMutex(&CacheSeg->Lock);
  return(STATUS_SUCCESS);
}

NTSTATUS STATIC
CcRosCreateCacheSegment(PBCB Bcb,
			ULONG FileOffset,
			PCACHE_SEGMENT* CacheSeg,
			BOOLEAN Lock)
{
  ULONG i;
  PCACHE_SEGMENT current;
  PLIST_ENTRY current_entry;
  NTSTATUS Status;
  KIRQL oldIrql;
#ifdef CACHE_BITMAP
  ULONG StartingOffset;
#endif

  assert(Bcb);

  DPRINT("CcRosCreateCacheSegment()\n");

  current = ExAllocateFromNPagedLookasideList(&CacheSegLookasideList);
  current->Valid = FALSE;
  current->Dirty = FALSE;
  current->FileOffset = ROUND_DOWN(FileOffset, Bcb->CacheSegmentSize);
  current->Bcb = Bcb;
  current->MappedCount = 0;
  current->DirtySegmentListEntry.Flink = NULL;
  current->DirtySegmentListEntry.Blink = NULL;
  current->ReferenceCount = 1;
  ExInitializeFastMutex(&current->Lock);
  ExAcquireFastMutex(&current->Lock);
  ExAcquireFastMutex(&ViewLock);

  *CacheSeg = current;
  /* There is window between the call to CcRosLookupCacheSegment
   * and CcRosCreateCacheSegment. We must check if a segment on
   * the fileoffset exist. If there exist a segment, we release
   * our new created segment and return the existing one. 
   */
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  current_entry = Bcb->BcbSegmentListHead.Flink;
  while (current_entry != &Bcb->BcbSegmentListHead)
  {
     current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				 BcbSegmentListEntry);
     if (current->FileOffset <= FileOffset &&
      	(current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
     {
	current->ReferenceCount++;
	KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
	ExReleaseFastMutex(&(*CacheSeg)->Lock);
	ExReleaseFastMutex(&ViewLock);
	ExFreeToNPagedLookasideList(&CacheSegLookasideList, *CacheSeg);
	*CacheSeg = current;
	if (Lock)
	{
          ExAcquireFastMutex(&current->Lock);
	}
	return STATUS_SUCCESS;
     }
     current_entry = current_entry->Flink;
  }
  /* There was no existing segment. */
  current = *CacheSeg;
  InsertTailList(&Bcb->BcbSegmentListHead, &current->BcbSegmentListEntry);
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  InsertTailList(&CacheSegmentListHead, &current->CacheSegmentListEntry);
  InsertTailList(&CacheSegmentLRUListHead, &current->CacheSegmentLRUListEntry);
  ExReleaseFastMutex(&ViewLock);
#ifdef CACHE_BITMAP
  KeAcquireSpinLock(&CiCacheSegMappingRegionLock, &oldIrql);

  StartingOffset = RtlFindClearBitsAndSet(&CiCacheSegMappingRegionAllocMap, Bcb->CacheSegmentSize / PAGE_SIZE, CiCacheSegMappingRegionHint);
  
  if (StartingOffset == 0xffffffff)
  {
     DPRINT1("Out of CacheSeg mapping space\n");
     KeBugCheck(0);
  }

  current->BaseAddress = CiCacheSegMappingRegionBase + StartingOffset * PAGE_SIZE;

  if (CiCacheSegMappingRegionHint == StartingOffset)
  {
     CiCacheSegMappingRegionHint += Bcb->CacheSegmentSize / PAGE_SIZE; 
  }

  KeReleaseSpinLock(&CiCacheSegMappingRegionLock, oldIrql);
#else
  MmLockAddressSpace(MmGetKernelAddressSpace());
  current->BaseAddress = NULL;
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_CACHE_SEGMENT,
			      &current->BaseAddress,
			      Bcb->CacheSegmentSize,
			      PAGE_READWRITE,
			      (PMEMORY_AREA*)&current->MemoryArea,
			      FALSE);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
  {
     KeBugCheck(0);
  }
#endif
  for (i = 0; i < (Bcb->CacheSegmentSize / PAGE_SIZE); i++)
  {
     PHYSICAL_ADDRESS Page;
      
     Status = MmRequestPageMemoryConsumer(MC_CACHE, TRUE, &Page);
     if (!NT_SUCCESS(Status))
     {
	KeBugCheck(0);
     }
      
     Status = MmCreateVirtualMapping(NULL,
				     current->BaseAddress + (i * PAGE_SIZE),
				     PAGE_READWRITE,
				     Page,
				     TRUE);
     if (!NT_SUCCESS(Status))
     {
	KeBugCheck(0);
     }
  }
  if (!Lock)
  {
     ExReleaseFastMutex(&current->Lock);
  }

  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosGetCacheSegmentChain(PBCB Bcb,
			  ULONG FileOffset,
			  ULONG Length,
			  PCACHE_SEGMENT* CacheSeg)
{
  PCACHE_SEGMENT current;
  ULONG i;
  PCACHE_SEGMENT* CacheSegList;
  PCACHE_SEGMENT Previous = NULL;

  assert(Bcb);

  DPRINT("CcRosGetCacheSegmentChain()\n");

  Length = ROUND_UP(Length, Bcb->CacheSegmentSize);

  CacheSegList = alloca(sizeof(PCACHE_SEGMENT) * 
			(Length / Bcb->CacheSegmentSize));

  /*
   * Look for a cache segment already mapping the same data.
   */
  for (i = 0; i < (Length / Bcb->CacheSegmentSize); i++)
    {
      ULONG CurrentOffset = FileOffset + (i * Bcb->CacheSegmentSize);
      current = CcRosLookupCacheSegment(Bcb, CurrentOffset);
      if (current != NULL)
	{
	  CacheSegList[i] = current;
	}
      else
	{
	  CcRosCreateCacheSegment(Bcb, CurrentOffset, &current, FALSE);
	  CacheSegList[i] = current;
	}
    }

  for (i = 0; i < (Length / Bcb->CacheSegmentSize); i++)
    {
      ExAcquireFastMutex(&CacheSegList[i]->Lock);
      if (i == 0)
	{
	  *CacheSeg = CacheSegList[i];
	  Previous = CacheSegList[i];
	}
      else
	{
	  Previous->NextInChain = CacheSegList[i];
	  Previous = CacheSegList[i];
	}
    }
  Previous->NextInChain = NULL;
  
  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosGetCacheSegment(PBCB Bcb,
		     ULONG FileOffset,
		     PULONG BaseOffset,
		     PVOID* BaseAddress,
		     PBOOLEAN UptoDate,
		     PCACHE_SEGMENT* CacheSeg)
{
   PCACHE_SEGMENT current;
   NTSTATUS Status;

   assert(Bcb);

   DPRINT("CcRosGetCacheSegment()\n");

   /*
    * Look for a cache segment already mapping the same data.
    */
   current = CcRosLookupCacheSegment(Bcb, FileOffset);
   if (current != NULL)
   {
      ExAcquireFastMutex(&current->Lock);
   }
   else
   {
     /*
      * Otherwise create a new segment.
      */
      Status = CcRosCreateCacheSegment(Bcb, FileOffset, &current, TRUE);
   }
   /*
    * Return information about the segment to the caller.
    */
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   DPRINT("*BaseAddress 0x%.8X\n", *BaseAddress);
   *CacheSeg = current;
   *BaseOffset = current->FileOffset;
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosRequestCacheSegment(PBCB Bcb,
		      ULONG FileOffset,
		      PVOID* BaseAddress,
		      PBOOLEAN UptoDate,
		      PCACHE_SEGMENT* CacheSeg)
/*
 * FUNCTION: Request a page mapping for a BCB
 */
{
  ULONG BaseOffset;

  assert(Bcb);

  if ((FileOffset % Bcb->CacheSegmentSize) != 0)
    {
      CPRINT("Bad fileoffset %x should be multiple of %x",
        FileOffset, Bcb->CacheSegmentSize);
      KeBugCheck(0);
    }

  return(CcRosGetCacheSegment(Bcb,
			   FileOffset,
			   &BaseOffset,
			   BaseAddress,
			   UptoDate,
			   CacheSeg));
}
#ifdef CACHE_BITMAP
#else
STATIC VOID 
CcFreeCachePage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
		PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
  assert(SwapEntry == 0);
  if (PhysAddr.QuadPart != 0)
    {
      MmReleasePageMemoryConsumer(MC_CACHE, PhysAddr);
    }
}
#endif
NTSTATUS 
CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{
#ifdef CACHE_BITMAP
  ULONG i;
  ULONG RegionSize;
  ULONG Base;
  PHYSICAL_ADDRESS PhysicalAddr;
  KIRQL oldIrql;
#endif
  DPRINT("Freeing cache segment %x\n", CacheSeg);
#ifdef CACHE_BITMAP
  RegionSize = CacheSeg->Bcb->CacheSegmentSize / PAGE_SIZE;

  /* Unmap all the pages. */
  for (i = 0; i < RegionSize; i++)
    {
      MmDeleteVirtualMapping(NULL, 
			     CacheSeg->BaseAddress + (i * PAGE_SIZE),
			     FALSE,
			     NULL,
			     &PhysicalAddr);
      MmReleasePageMemoryConsumer(MC_CACHE, PhysicalAddr);
    }

  KeAcquireSpinLock(&CiCacheSegMappingRegionLock, &oldIrql);
  /* Deallocate all the pages used. */
  Base = (ULONG)(CacheSeg->BaseAddress - CiCacheSegMappingRegionBase) / PAGE_SIZE;
  
  RtlClearBits(&CiCacheSegMappingRegionAllocMap, Base, RegionSize);

  CiCacheSegMappingRegionHint = min (CiCacheSegMappingRegionHint, Base);

  KeReleaseSpinLock(&CiCacheSegMappingRegionLock, oldIrql);
#else
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   CacheSeg->BaseAddress,
		   CacheSeg->Bcb->CacheSegmentSize,
		   CcFreeCachePage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
#endif
  ExFreeToNPagedLookasideList(&CacheSegLookasideList, CacheSeg);
  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosFreeCacheSegment(PBCB Bcb, PCACHE_SEGMENT CacheSeg)
{
  NTSTATUS Status;
  KIRQL oldIrql;

  assert(Bcb);

  DPRINT("CcRosFreeCacheSegment(Bcb %x, CacheSeg %x)\n",
         Bcb, CacheSeg);

  ExAcquireFastMutex(&ViewLock);
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  RemoveEntryList(&CacheSeg->BcbSegmentListEntry);
  RemoveEntryList(&CacheSeg->CacheSegmentListEntry);
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  if (CacheSeg->Dirty)
  {
     RemoveEntryList(&CacheSeg->DirtySegmentListEntry);
  }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  ExReleaseFastMutex(&ViewLock);

  Status = CcRosInternalFreeCacheSegment(CacheSeg);
  return(Status);
}

VOID STDCALL
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
	     IN PLARGE_INTEGER FileOffset OPTIONAL,
	     IN ULONG Length,
	     OUT PIO_STATUS_BLOCK IoStatus)
{
   PBCB Bcb;
   LARGE_INTEGER Offset;
   PCACHE_SEGMENT current;
   NTSTATUS Status;
   KIRQL oldIrql;

   DPRINT("CcFlushCache(SectionObjectPointers %x, FileOffset %x, Length %d, IoStatus %x)\n",
           SectionObjectPointers, FileOffset, Length, IoStatus);

   if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
   {
      Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
      assert(Bcb);
      if (FileOffset)
      {
	 Offset = *FileOffset;
      }
      else 
      {
	 Offset.QuadPart = 0LL;
	 Length = Bcb->FileSize.u.LowPart;
      }
   
      if (IoStatus)
      {
	 IoStatus->Status = STATUS_SUCCESS;
	 IoStatus->Information = 0;
      }

      while (Length > 0)
      {
	 current = CcRosLookupCacheSegment (Bcb, Offset.u.LowPart);
	 if (current != NULL)
	 {
	    ExAcquireFastMutex(&current->Lock);
	    if (current->Dirty)
	    {
	       Status = CcRosFlushCacheSegment(current);
	       if (!NT_SUCCESS(Status) && IoStatus != NULL)
	       {
		   IoStatus->Status = Status;
	       }
	    }
            KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
	    ExReleaseFastMutex(&current->Lock);
	    current->ReferenceCount--;
	    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
	 }

	 Offset.QuadPart += Bcb->CacheSegmentSize;
	 if (Length > Bcb->CacheSegmentSize)
	 {
	    Length -= Bcb->CacheSegmentSize;
	 }
	 else
	 {
	    Length = 0;
	 }
      }
   }
   else
   {
      if (IoStatus)
      {
	 IoStatus->Status = STATUS_INVALID_PARAMETER;
      }
   }
}

NTSTATUS 
CcRosDeleteFileCache(PFILE_OBJECT FileObject, PBCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   NTSTATUS Status;
   LIST_ENTRY FreeList;
   KIRQL oldIrql;

   assert(Bcb);
   
   Bcb->RefCount++;
   ExReleaseFastMutex(&ViewLock);

   CcFlushCache(FileObject->SectionObjectPointers, NULL, 0, NULL);

   ExAcquireFastMutex(&ViewLock);
   Bcb->RefCount--;
   if (Bcb->RefCount == 0)
   {
      if (Bcb->BcbRemoveListEntry.Flink != NULL)
      {
	 RemoveEntryList(&Bcb->BcbRemoveListEntry);
         Bcb->BcbRemoveListEntry.Flink = NULL;
      }

      FileObject->SectionObjectPointers->SharedCacheMap = NULL;  

      /*
       * Release all cache segments.
       */
      InitializeListHead(&FreeList);
      KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
      current_entry = Bcb->BcbSegmentListHead.Flink;
      while (!IsListEmpty(&Bcb->BcbSegmentListHead))
      {
         current_entry = RemoveTailList(&Bcb->BcbSegmentListHead);
         current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentListEntry);
         RemoveEntryList(&current->CacheSegmentLRUListEntry);
         if (current->Dirty)
	 {
            RemoveEntryList(&current->DirtySegmentListEntry);
	    DPRINT1("Freeing dirty segment\n");
	 }
         InsertHeadList(&FreeList, &current->BcbSegmentListEntry);
      }
      KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);	

      ExReleaseFastMutex(&ViewLock);
      ObDereferenceObject (Bcb->FileObject);

      while (!IsListEmpty(&FreeList))
      {
         current_entry = RemoveTailList(&FreeList);
         current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
         Status = CcRosInternalFreeCacheSegment(current);
      }
      ExFreeToNPagedLookasideList(&BcbLookasideList, Bcb);   
      ExAcquireFastMutex(&ViewLock);
   }
   return(STATUS_SUCCESS);
}

VOID CcRosReferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  ExAcquireFastMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointers->SharedCacheMap;
  assert(Bcb);
  Bcb->RefCount++;
  ExReleaseFastMutex(&ViewLock);
}

VOID CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
  PBCB Bcb;
//  DPRINT1("CcRosSetRemoveOnClose()\n");
  ExAcquireFastMutex(&ViewLock);
  Bcb = (PBCB)SectionObjectPointer->SharedCacheMap;
  if (Bcb)
  {
    Bcb->RemoveOnClose = TRUE;
    if (Bcb->RefCount == 0)
    {
      CcRosDeleteFileCache(Bcb->FileObject, Bcb);
    }
  }
  ExReleaseFastMutex(&ViewLock);
}


VOID CcRosDereferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  ExAcquireFastMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointers->SharedCacheMap;
  assert(Bcb);
  if (Bcb->RefCount > 0)
  {
    Bcb->RefCount--;
    if (Bcb->RefCount == 0)
    {
       MmFreeSectionSegments(Bcb->FileObject);
       if (Bcb->RemoveOnClose)
       {
          CcRosDeleteFileCache(FileObject, Bcb);
       }
       else
       {
	  Bcb->TimeStamp = CcTimeStamp;
          InsertHeadList(&ClosedListHead, &Bcb->BcbRemoveListEntry);
       }
    }
  }
  ExReleaseFastMutex(&ViewLock);
}

NTSTATUS STDCALL 
CcRosReleaseFileCache(PFILE_OBJECT FileObject, PBCB Bcb)
/*
 * FUNCTION: Called by the file system when a handle to a file object
 * has been closed.
 */
{
  assert(Bcb);

  ExAcquireFastMutex(&ViewLock);

  if (FileObject->SectionObjectPointers->SharedCacheMap != NULL)
  {
    if (FileObject->PrivateCacheMap != NULL)
    {
      FileObject->PrivateCacheMap = NULL;
      if (Bcb->RefCount > 0)
      {
         Bcb->RefCount--;
	 if (Bcb->RefCount == 0)
	 {
	    if (Bcb->RemoveOnClose)
	    {
	       CcRosDeleteFileCache(FileObject, Bcb);
	    }
	    else
	    {
	       Bcb->TimeStamp = CcTimeStamp;
	       InsertHeadList(&ClosedListHead, &Bcb->BcbRemoveListEntry);
	    }
	 }
      }
    }
  }
  ExReleaseFastMutex(&ViewLock);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosInitializeFileCache(PFILE_OBJECT FileObject,
			 PBCB* Bcb,
			 ULONG CacheSegmentSize)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
{
   DPRINT("CcRosInitializeFileCache(FileObject %x, *Bcb %x, CacheSegmentSize %d)\n",
           FileObject, Bcb, CacheSegmentSize);

   ExAcquireFastMutex(&ViewLock);

   if (*Bcb == NULL)
   {
      (*Bcb) = ExAllocateFromNPagedLookasideList(&BcbLookasideList);	
      if ((*Bcb) == NULL)
      {
        ExReleaseFastMutex(&ViewLock);
	return(STATUS_UNSUCCESSFUL);
      }
      memset((*Bcb), 0, sizeof(BCB));      
      ObReferenceObjectByPointer(FileObject,
			         FILE_ALL_ACCESS,
			         NULL,
			         KernelMode);
      (*Bcb)->FileObject = FileObject;
      (*Bcb)->CacheSegmentSize = CacheSegmentSize;
      if (FileObject->FsContext)
      {
         (*Bcb)->AllocationSize = 
	   ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->AllocationSize;
         (*Bcb)->FileSize = 
	   ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->FileSize;
      }
      KeInitializeSpinLock(&(*Bcb)->BcbLock);
      InitializeListHead(&(*Bcb)->BcbSegmentListHead);
      FileObject->SectionObjectPointers->SharedCacheMap = *Bcb;
   }
   if (FileObject->PrivateCacheMap == NULL)
   {
      FileObject->PrivateCacheMap = *Bcb;
      (*Bcb)->RefCount++;
   }
   if ((*Bcb)->BcbRemoveListEntry.Flink != NULL)
   {
      RemoveEntryList(&(*Bcb)->BcbRemoveListEntry);
      (*Bcb)->BcbRemoveListEntry.Flink = NULL;
   }
   ExReleaseFastMutex(&ViewLock);

   return(STATUS_SUCCESS);
}

PFILE_OBJECT STDCALL
CcGetFileObjectFromSectionPtrs(IN PSECTION_OBJECT_POINTERS SectionObjectPointers)
{
   PBCB Bcb;
   if (SectionObjectPointers && SectionObjectPointers->SharedCacheMap)
   {
      Bcb = (PBCB)SectionObjectPointers->SharedCacheMap;
      assert(Bcb);
      return Bcb->FileObject;
   }
   return NULL;
}

VOID STDCALL
CmLazyCloseThreadMain(PVOID Ignored)
{
   LARGE_INTEGER Timeout;
   PLIST_ENTRY current_entry;
   PBCB current;
   ULONG RemoveTimeStamp;
   NTSTATUS Status;

   KeQuerySystemTime (&Timeout);

   while (1)
   {
      Timeout.QuadPart += 100000000LL; // 10sec
      Status = KeWaitForSingleObject(&LazyCloseThreadEvent,
	                             0,
				     KernelMode,
				     FALSE,
				     &Timeout);

      DPRINT("LazyCloseThreadMain %d\n", CcTimeStamp);

      if (!NT_SUCCESS(Status))
      {
	  DbgPrint("LazyCloseThread: Wait failed\n");
	  KeBugCheck(0);
	  break;
      }
      if (LazyCloseThreadShouldTerminate)
      {
          DbgPrint("LazyCloseThread: Terminating\n");
	  break;
      }
      
      ExAcquireFastMutex(&ViewLock);
      CcTimeStamp++;
      if (CcTimeStamp >= 30)
      {
         RemoveTimeStamp = CcTimeStamp - 30; /* 5min = 10sec * 30 */
         while (!IsListEmpty(&ClosedListHead))
	 {
            current_entry = ClosedListHead.Blink;
            current = CONTAINING_RECORD(current_entry, BCB, BcbRemoveListEntry);
	    if (current->TimeStamp >= RemoveTimeStamp)
	    {
	       break;
	    }
            CcRosDeleteFileCache(current->FileObject, current);
	 }
      }
      ExReleaseFastMutex(&ViewLock);
   }
}

VOID
CcInitView(VOID)
{
#ifdef CACHE_BITMAP
  PMEMORY_AREA marea;
  PVOID Buffer;
#endif
  NTSTATUS Status;
  KPRIORITY Priority;

  DPRINT("CcInitView()\n");
#ifdef CACHE_BITMAP
  CiCacheSegMappingRegionHint = 0;
  CiCacheSegMappingRegionBase = NULL;

  MmLockAddressSpace(MmGetKernelAddressSpace());

  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_CACHE_SEGMENT,
			      &CiCacheSegMappingRegionBase,
			      CI_CACHESEG_MAPPING_REGION_SIZE,
			      0,
			      &marea,
			      FALSE);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(0);
    }

  Buffer = ExAllocatePool(NonPagedPool, CI_CACHESEG_MAPPING_REGION_SIZE / (PAGE_SIZE * 8));

  RtlInitializeBitMap(&CiCacheSegMappingRegionAllocMap, Buffer, CI_CACHESEG_MAPPING_REGION_SIZE / PAGE_SIZE);
  RtlClearAllBits(&CiCacheSegMappingRegionAllocMap);

  KeInitializeSpinLock(&CiCacheSegMappingRegionLock);
#endif  
  InitializeListHead(&CacheSegmentListHead);
  InitializeListHead(&DirtySegmentListHead);
  InitializeListHead(&CacheSegmentLRUListHead);
  InitializeListHead(&ClosedListHead);
  ExInitializeFastMutex(&ViewLock);
  ExInitializeNPagedLookasideList (&iBcbLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(INTERNAL_BCB),
				   TAG_IBCB,
				   20);
  ExInitializeNPagedLookasideList (&BcbLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(BCB),
				   TAG_BCB,
				   20);
  ExInitializeNPagedLookasideList (&CacheSegLookasideList,
	                           NULL,
				   NULL,
				   0,
				   sizeof(CACHE_SEGMENT),
				   TAG_CSEG,
				   20);

  MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);
  
  CcInitCacheZeroPage();

  CcTimeStamp = 0;  
  LazyCloseThreadShouldTerminate = FALSE;
  KeInitializeEvent (&LazyCloseThreadEvent, SynchronizationEvent, FALSE);
  Status = PsCreateSystemThread(&LazyCloseThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&LazyCloseThreadId,
				(PKSTART_ROUTINE)CmLazyCloseThreadMain,
				NULL);
  if (NT_SUCCESS(Status))
  {
     Priority = LOW_REALTIME_PRIORITY;
     NtSetInformationThread(LazyCloseThreadHandle,
			    ThreadPriority,
			    &Priority,
			    sizeof(Priority));
  }

}

/* EOF */







