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
/* $Id: view.c,v 1.53 2002/10/02 19:20:51 hbirr Exp $
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

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) % (S)) ? ROUND_UP(N, S) - S : N)

#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')

static LIST_ENTRY DirtySegmentListHead;
static LIST_ENTRY CacheSegmentListHead;
static LIST_ENTRY CacheSegmentLRUListHead;

static FAST_MUTEX ViewLock;

void * alloca(size_t size);

NTSTATUS STDCALL
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
  DPRINT("CcRosTrimCache() finished\n");
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

NTSTATUS STDCALL 
CcRosReleaseCacheSegment(PBCB Bcb,
			 PCACHE_SEGMENT CacheSeg,
			 BOOLEAN Valid,
			 BOOLEAN Dirty,
			 BOOLEAN Mapped)
{
  BOOLEAN WasDirty = CacheSeg->Dirty;
  KIRQL oldIrql;

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

PCACHE_SEGMENT CcRosLookupCacheSegment(PBCB Bcb, ULONG FileOffset)
{
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  KIRQL oldIrql;

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

  DPRINT("CcRosCreateCacheSegment()\n");

  current = ExAllocatePoolWithTag(NonPagedPool, sizeof(CACHE_SEGMENT), 
			          TAG_CSEG);
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
	ExFreePool(*CacheSeg);
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

NTSTATUS STDCALL 
CcRosInternalFreeCacheSegment(PCACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{

  DPRINT("Freeing cache segment %x\n", CacheSeg);

  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   CacheSeg->BaseAddress,
		   CacheSeg->Bcb->CacheSegmentSize,
		   CcFreeCachePage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  ExFreePool(CacheSeg);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosFreeCacheSegment(PBCB Bcb, PCACHE_SEGMENT CacheSeg)
{
  NTSTATUS Status;
  KIRQL oldIrql;

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
	    ExReleaseFastMutex(&current->Lock);
            KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
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

NTSTATUS STDCALL 
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
   
   DPRINT("CcRosDeleteFileCache(FileObject %x, Bcb %x)\n", 
          Bcb->FileObject, Bcb);

   ExReleaseFastMutex(&ViewLock);

   CcFlushCache(FileObject->SectionObjectPointers, NULL, 0, NULL);

   ExAcquireFastMutex(&ViewLock);

   if (Bcb->RefCount == 0)
   {
      MmFreeSectionSegments(Bcb->FileObject);

      /*
       * Release all cache segments.
       */
      InitializeListHead(&FreeList);

      FileObject->SectionObjectPointers->SharedCacheMap = NULL;  
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
	 }
         InsertHeadList(&FreeList, &current->BcbSegmentListEntry);

      }
      KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);	
      while (!IsListEmpty(&FreeList))
      {
         current_entry = RemoveTailList(&FreeList);
         current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
         Status = CcRosInternalFreeCacheSegment(current);
      }
      
      ObDereferenceObject (Bcb->FileObject);
      ExFreePool(Bcb);
   }
   return(STATUS_SUCCESS);
}

VOID CcRosReferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  ExAcquireFastMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointers->SharedCacheMap;
  Bcb->RefCount++;
  ExReleaseFastMutex(&ViewLock);
}

VOID CcRosDereferenceCache(PFILE_OBJECT FileObject)
{
  PBCB Bcb;
  ExAcquireFastMutex(&ViewLock);
  Bcb = (PBCB)FileObject->SectionObjectPointers->SharedCacheMap;
  Bcb->RefCount--;
  if (Bcb->RefCount == 0)
  {
     CcRosDeleteFileCache(FileObject, Bcb);
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
  ExAcquireFastMutex(&ViewLock);

  if (FileObject->SectionObjectPointers->SharedCacheMap != NULL)
  {
    if (FileObject->PrivateCacheMap != NULL)
    {
      FileObject->PrivateCacheMap = NULL;
      Bcb->RefCount--;
    }
    if (Bcb->RefCount == 0)
    {
      CcRosDeleteFileCache(FileObject, Bcb);
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
      (*Bcb) = ExAllocatePoolWithTag(NonPagedPool, sizeof(BCB), TAG_BCB);
      if ((*Bcb) == NULL)
      {
	return(STATUS_UNSUCCESSFUL);
      }
   
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
      return Bcb->FileObject;
   }
   return NULL;
}

VOID
CcInitView(VOID)
{
  DPRINT("CcInitView()\n");
  InitializeListHead(&CacheSegmentListHead);
  InitializeListHead(&DirtySegmentListHead);
  InitializeListHead(&CacheSegmentLRUListHead);
  ExInitializeFastMutex(&ViewLock);
  MmInitializeMemoryConsumer(MC_CACHE, CcRosTrimCache);
  CcInitCacheZeroPage();
}

/* EOF */







