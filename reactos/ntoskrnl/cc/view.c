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
/* $Id: view.c,v 1.48 2002/09/07 15:12:47 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

extern void * alloca(size_t);

/* GLOBALS *******************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) % (S)) ? ROUND_UP(N, S) - S : N)

#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')

static LIST_ENTRY DirtySegmentListHead;
static LIST_ENTRY CacheSegmentListHead;
static LIST_ENTRY CacheSegmentLRUListHead;

static FAST_MUTEX ViewLock;

NTSTATUS STDCALL
CcRosInternalFreeCacheSegment(PROS_BCB Bcb, PROS_CACHE_SEGMENT CacheSeg);

/* FUNCTIONS *****************************************************************/

NTSTATUS STATIC
CcRosFlushCacheSegment(PROS_CACHE_SEGMENT CacheSegment)
{
  NTSTATUS Status;
  Status = WriteCacheSegment(CacheSegment);
  if (NT_SUCCESS(Status))
    {
      CacheSegment->Dirty = FALSE;
      RemoveEntryList(&CacheSegment->DirtySegmentListEntry);
    }
  return(Status);
}

NTSTATUS
CcRosFlushDirtyPages(ULONG Target, PULONG Count)
{
  PLIST_ENTRY current_entry;
  PROS_CACHE_SEGMENT current;
  ULONG PagesPerSegment;
  BOOLEAN Locked;
  NTSTATUS Status;

  DPRINT("CcRosFlushDirtyPages(Target %d)\n", Target);

  (*Count) = 0;

  ExAcquireFastMutex(&ViewLock);
  current_entry = DirtySegmentListHead.Flink;
  while (current_entry != &DirtySegmentListHead && Target > 0)
    {
      current = CONTAINING_RECORD(current_entry, ROS_CACHE_SEGMENT, 
				  DirtySegmentListEntry);
      current_entry = current_entry->Flink;
      Locked = ExTryToAcquireFastMutex(&current->Lock);
      if (!Locked)
	{
	  continue;
	}
      assert(current->Dirty);
      if (current->ReferenceCount > 0)
	{
	  ExReleaseFastMutex(&current->Lock);
	  continue;
	}
      current->ReferenceCount++;
      ExReleaseFastMutex(&ViewLock);
      PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;
      Status = CcRosFlushCacheSegment(current);      
      current->ReferenceCount--;
      ExReleaseFastMutex(&current->Lock);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CC: Failed to flush cache segment.\n");
	}      
      (*Count) += PagesPerSegment;
      Target -= PagesPerSegment;     

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
  PROS_CACHE_SEGMENT current;
  ULONG PagesPerSegment;
  ULONG PagesFreed;
  BOOLEAN Locked;

  DPRINT("CcRosTrimCache(Target %d)\n", Target);

  *NrFreed = 0;

  ExAcquireFastMutex(&ViewLock);
  current_entry = CacheSegmentLRUListHead.Flink;
  while (current_entry != &CacheSegmentLRUListHead && Target > 0)
    {
      current = CONTAINING_RECORD(current_entry, ROS_CACHE_SEGMENT, 
				  CacheSegmentLRUListEntry);
      current_entry = current_entry->Flink;
      Locked = ExTryToAcquireFastMutex(&current->Lock);
      if (!Locked)
	{
	  continue;
	}
      if (current->MappedCount > 0 || current->Dirty || 
	  current->ReferenceCount > 0)
	{
	  ExReleaseFastMutex(&current->Lock);
	  continue;
	}
      ExReleaseFastMutex(&current->Lock);
      DPRINT("current->Bcb->CacheSegmentSize %d\n", 
	     current->Bcb->CacheSegmentSize);
      PagesPerSegment = current->Bcb->CacheSegmentSize / PAGE_SIZE;
      CcRosInternalFreeCacheSegment(current->Bcb, current);      
      DPRINT("CcRosTrimCache(): Freed %d\n", PagesPerSegment);
      PagesFreed = min(PagesPerSegment, Target);
      Target = Target - PagesFreed;
      (*NrFreed) = (*NrFreed) + PagesFreed;
    }
  ExReleaseFastMutex(&ViewLock);
  DPRINT("CcRosTrimCache() finished\n");
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosReleaseCacheSegment(PROS_BCB Bcb,
			 PROS_CACHE_SEGMENT CacheSeg,
			 BOOLEAN Valid,
			 BOOLEAN Dirty,
			 BOOLEAN Mapped)
{
  BOOLEAN WasDirty = CacheSeg->Dirty;

  DPRINT("CcReleaseCachePage(Bcb %x, CacheSeg %x, Valid %d)\n",
	 Bcb, CacheSeg, Valid);
  
  CacheSeg->Valid = Valid;
  CacheSeg->Dirty = CacheSeg->Dirty || Dirty;
  if (Mapped)
    {
      CacheSeg->MappedCount++;
    }
  ExReleaseFastMutex(&CacheSeg->Lock);
  ExAcquireFastMutex(&ViewLock);
  if (!WasDirty && CacheSeg->Dirty)
    {
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
    }
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  InsertTailList(&CacheSegmentLRUListHead, 
		 &CacheSeg->CacheSegmentLRUListEntry);
  ExReleaseFastMutex(&ViewLock);
  InterlockedDecrement(&CacheSeg->ReferenceCount);
  
  DPRINT("CcReleaseCachePage() finished\n");
  
  return(STATUS_SUCCESS);
}

PROS_CACHE_SEGMENT CcRosLookupCacheSegment(PROS_BCB Bcb, ULONG FileOffset)
{
  PLIST_ENTRY current_entry;
  PROS_CACHE_SEGMENT current;
  KIRQL oldIrql;

  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  current_entry = Bcb->BcbSegmentListHead.Flink;
  while (current_entry != &Bcb->BcbSegmentListHead)
    {
      current = CONTAINING_RECORD(current_entry, ROS_CACHE_SEGMENT, 
				  BcbSegmentListEntry);
      if (current->FileOffset <= FileOffset &&
	  (current->FileOffset + Bcb->CacheSegmentSize) > FileOffset)
	{
	  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
	  return(current);
	}
      current_entry = current_entry->Flink;
    }
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  return(NULL);
}

NTSTATUS
CcRosMarkDirtyCacheSegment(PROS_BCB Bcb, ULONG FileOffset)
{
  PROS_CACHE_SEGMENT CacheSeg;

  ExAcquireFastMutex(&ViewLock);
  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      KeBugCheck(0);
    }
  ExAcquireFastMutex(&CacheSeg->Lock);
  if (!CacheSeg->Dirty)
    {
      InsertTailList(&DirtySegmentListHead, &CacheSeg->DirtySegmentListEntry);
    }
  CacheSeg->Dirty = TRUE;
  ExReleaseFastMutex(&CacheSeg->Lock);
  ExReleaseFastMutex(&ViewLock);
  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosSuggestFreeCacheSegment(PROS_BCB Bcb, ULONG FileOffset, BOOLEAN NowDirty)
{
  PROS_CACHE_SEGMENT CacheSeg;

  ExAcquireFastMutex(&ViewLock);
  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      KeBugCheck(0);
    }
  ExAcquireFastMutex(&CacheSeg->Lock);
  if (CacheSeg->MappedCount > 0)
    {
      KeBugCheck(0);
    }
  CacheSeg->Dirty = CacheSeg->Dirty || NowDirty;
  if (CacheSeg->Dirty || CacheSeg->ReferenceCount > 0)
    {
      ExReleaseFastMutex(&CacheSeg->Lock);
      ExReleaseFastMutex(&ViewLock);
      return(STATUS_UNSUCCESSFUL);
    }
  ExReleaseFastMutex(&CacheSeg->Lock);
  CcRosInternalFreeCacheSegment(CacheSeg->Bcb, CacheSeg);
  ExReleaseFastMutex(&ViewLock);
  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosUnmapCacheSegment(PROS_BCB Bcb, ULONG FileOffset, BOOLEAN NowDirty)
{
  PROS_CACHE_SEGMENT CacheSeg;

  ExAcquireFastMutex(&ViewLock);
  CacheSeg = CcRosLookupCacheSegment(Bcb, FileOffset);
  if (CacheSeg == NULL)
    {
      ExReleaseFastMutex(&ViewLock);
      return(STATUS_UNSUCCESSFUL);
    }
  CacheSeg->ReferenceCount++;
  ExReleaseFastMutex(&ViewLock);
  ExAcquireFastMutex(&CacheSeg->Lock);
  CacheSeg->MappedCount--;
  CacheSeg->Dirty = CacheSeg->Dirty || NowDirty;
  CacheSeg->ReferenceCount--;
  ExReleaseFastMutex(&CacheSeg->Lock);
  return(STATUS_SUCCESS);
}

NTSTATUS STATIC
CcRosCreateCacheSegment(PROS_BCB Bcb,
			ULONG FileOffset,
			PROS_CACHE_SEGMENT* CacheSeg,
			BOOLEAN Lock)
{
  ULONG i;
  PROS_CACHE_SEGMENT current;
  NTSTATUS Status;
  KIRQL oldIrql;

  current = ExAllocatePoolWithTag(NonPagedPool, sizeof(ROS_CACHE_SEGMENT), 
				  TAG_CSEG);

  MmLockAddressSpace(MmGetKernelAddressSpace());
  current->BaseAddress = NULL;
  Status = MmCreateMemoryArea(KernelMode,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_CACHE_SEGMENT,
			      &current->BaseAddress,
			      Bcb->CacheSegmentSize,
			      PAGE_READWRITE,
			      (PMEMORY_AREA*)&current->MemoryArea,
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      KeBugCheck(0);
    }
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  current->Valid = FALSE;
  current->Dirty = FALSE;
  current->FileOffset = ROUND_DOWN(FileOffset, Bcb->CacheSegmentSize);
  current->Bcb = Bcb;
  current->MappedCount = 0;
  ExInitializeFastMutex(&current->Lock);
  current->ReferenceCount = 1;
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  InsertTailList(&Bcb->BcbSegmentListHead, &current->BcbSegmentListEntry);
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  InsertTailList(&CacheSegmentListHead, &current->CacheSegmentListEntry);
  InsertTailList(&CacheSegmentLRUListHead, 
		 &current->CacheSegmentLRUListEntry);
  current->DirtySegmentListEntry.Flink = 
    current->DirtySegmentListEntry.Blink = NULL;
  if (Lock)
    {
      ExAcquireFastMutex(&current->Lock);
    }
  ExReleaseFastMutex(&ViewLock);
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
  *CacheSeg = current;
  return(STATUS_SUCCESS);
}

NTSTATUS
CcRosGetCacheSegmentChain(PROS_BCB Bcb,
			  ULONG FileOffset,
			  ULONG Length,
			  PROS_CACHE_SEGMENT* CacheSeg)
{
  PROS_CACHE_SEGMENT current;
  ULONG i;
  PROS_CACHE_SEGMENT* CacheSegList;
  PROS_CACHE_SEGMENT Previous;

  Length = ROUND_UP(Length, Bcb->CacheSegmentSize);

  CacheSegList = alloca(sizeof(PROS_CACHE_SEGMENT) * 
			(Length / Bcb->CacheSegmentSize));

  /*
   * Acquire the global lock.
   */
  ExAcquireFastMutex(&ViewLock);
  
  /*
   * Look for a cache segment already mapping the same data.
   */
  for (i = 0; i < (Length / Bcb->CacheSegmentSize); i++)
    {
      ULONG CurrentOffset = FileOffset + (i * Bcb->CacheSegmentSize);
      current = CcRosLookupCacheSegment(Bcb, CurrentOffset);
      if (current != NULL)
	{
	  /*
	   * Make sure the cache segment can't go away outside of our control.
	   */
	  current->ReferenceCount++;
	  CacheSegList[i] = current;
	}
      else
	{
	  CcRosCreateCacheSegment(Bcb, CurrentOffset, &current, FALSE);
	  CacheSegList[i] = current;
	  ExAcquireFastMutex(&ViewLock);
	}
    }
  ExReleaseFastMutex(&ViewLock);

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
CcRosGetCacheSegment(PROS_BCB Bcb,
		     ULONG FileOffset,
		     PULONG BaseOffset,
		     PVOID* BaseAddress,
		     PBOOLEAN UptoDate,
		     PROS_CACHE_SEGMENT* CacheSeg)
{
   PROS_CACHE_SEGMENT current;
   NTSTATUS Status;

   /*
    * Acquire the global lock.
    */
   ExAcquireFastMutex(&ViewLock);

   /*
    * Look for a cache segment already mapping the same data.
    */
   current = CcRosLookupCacheSegment(Bcb, FileOffset);
   if (current != NULL)
     {
       /*
	* Make sure the cache segment can't go away outside of our control.
	*/
       current->ReferenceCount++;
       /*
	* Release the global lock and lock the cache segment.
	*/
       ExReleaseFastMutex(&ViewLock);
       ExAcquireFastMutex(&current->Lock);
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

   /*
    * Otherwise create a new segment.
    */
   Status = CcRosCreateCacheSegment(Bcb, FileOffset, &current, TRUE);
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   DPRINT("*BaseAddress 0x%.8X\n", *BaseAddress);
   *CacheSeg = current;
   *BaseOffset = current->FileOffset;
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosRequestCacheSegment(PROS_BCB Bcb,
		      ULONG FileOffset,
		      PVOID* BaseAddress,
		      PBOOLEAN UptoDate,
		      PROS_CACHE_SEGMENT* CacheSeg)
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
CcRosInternalFreeCacheSegment(PROS_BCB Bcb, PROS_CACHE_SEGMENT CacheSeg)
/*
 * FUNCTION: Releases a cache segment associated with a BCB
 */
{
  DPRINT("Freeing cache segment %x\n", CacheSeg);
  RemoveEntryList(&CacheSeg->CacheSegmentListEntry);
  RemoveEntryList(&CacheSeg->CacheSegmentLRUListEntry);
  RemoveEntryList(&CacheSeg->BcbSegmentListEntry);
  MmLockAddressSpace(MmGetKernelAddressSpace());
  MmFreeMemoryArea(MmGetKernelAddressSpace(),
		   CacheSeg->BaseAddress,
		   Bcb->CacheSegmentSize,
		   CcFreeCachePage,
		   NULL);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());
  ExFreePool(CacheSeg);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosFreeCacheSegment(PROS_BCB Bcb, PROS_CACHE_SEGMENT CacheSeg)
{
  NTSTATUS Status;
  ExAcquireFastMutex(&ViewLock);
  Status = CcRosInternalFreeCacheSegment(Bcb, CacheSeg);
  ExReleaseFastMutex(&ViewLock);
  return(Status);
}

NTSTATUS STDCALL 
CcRosDeleteFileCache(PFILE_OBJECT FileObject, PROS_BCB Bcb)
/*
 * FUNCTION: Releases the BCB associated with a file object
 */
{
   PLIST_ENTRY current_entry;
   PROS_CACHE_SEGMENT current;
   NTSTATUS Status;
   
   DPRINT("CcRosDeleteFileCache(FileObject %x, Bcb %x)\n", Bcb->FileObject, 
	  Bcb);

   MmFreeSectionSegments(Bcb->FileObject);

   /*
    * Write back dirty cache segments.
    */
   current_entry = Bcb->BcbSegmentListHead.Flink;
   while (current_entry != &Bcb->BcbSegmentListHead)
     {
       current = 
	  CONTAINING_RECORD(current_entry, ROS_CACHE_SEGMENT, BcbSegmentListEntry);
       if (current->Dirty)
	 {
	   Status = WriteCacheSegment(current);
	   if (!NT_SUCCESS(Status))
	     {
	       DPRINT1("Failed to write cache segment (Status %X)\n", Status);
	     }
	   ExAcquireFastMutex(&ViewLock);
	   RemoveEntryList(&current->DirtySegmentListEntry);
	   ExReleaseFastMutex(&ViewLock);
	 }
       current_entry = current_entry->Flink;
     }
   
   /*
    * Release all cache segments.
    */
   current_entry = Bcb->BcbSegmentListHead.Flink;
   while (current_entry != &Bcb->BcbSegmentListHead)
     {
	current = 
	  CONTAINING_RECORD(current_entry, ROS_CACHE_SEGMENT, BcbSegmentListEntry);
	current_entry = current_entry->Flink;
	CcRosFreeCacheSegment(Bcb, current);
     }
   FileObject->SectionObjectPointer->SharedCacheMap = NULL;  
   ObDereferenceObject (Bcb->FileObject);
   ExFreePool(Bcb);

   return(STATUS_SUCCESS);
}

VOID CcRosReferenceCache(PFILE_OBJECT FileObject)
{
  KIRQL oldIrql;
  PROS_BCB Bcb = (PROS_BCB)FileObject->SectionObjectPointer->SharedCacheMap;
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  Bcb->RefCount++;
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
}

VOID CcRosDereferenceCache(PFILE_OBJECT FileObject)
{
  KIRQL oldIrql;
  PROS_BCB Bcb = (PROS_BCB)FileObject->SectionObjectPointer->SharedCacheMap;
  KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
  Bcb->RefCount--;
  KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
  if (Bcb->RefCount == 0)
  {
     CcRosDeleteFileCache(FileObject, Bcb);
  }
}

NTSTATUS STDCALL 
CcRosReleaseFileCache(PFILE_OBJECT FileObject, PROS_BCB Bcb)
/*
 * FUNCTION: Called by the file system when a handle to a file object
 * has been closed.
 */
{
  KIRQL oldIrql;
  if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
  {
    KeAcquireSpinLock(&Bcb->BcbLock, &oldIrql);
    if (FileObject->PrivateCacheMap != NULL)
    {
      FileObject->PrivateCacheMap = NULL;
      Bcb->RefCount--;
    }
    KeReleaseSpinLock(&Bcb->BcbLock, oldIrql);
    if (Bcb->RefCount == 0)
    {
      CcRosDeleteFileCache(FileObject, Bcb);
    }
  }
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
CcRosInitializeFileCache(PFILE_OBJECT FileObject,
			 PROS_BCB* Bcb,
			 ULONG CacheSegmentSize)
/*
 * FUNCTION: Initializes a BCB for a file object
 */
{
   KIRQL oldIrql;
   if (*Bcb == NULL)
   {
      (*Bcb) = ExAllocatePoolWithTag(NonPagedPool, sizeof(ROS_BCB), TAG_BCB);
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
      FileObject->SectionObjectPointer->SharedCacheMap = *Bcb;
   }
   KeAcquireSpinLock(&(*Bcb)->BcbLock, &oldIrql);
   if (FileObject->PrivateCacheMap == NULL)
   {
      FileObject->PrivateCacheMap = *Bcb;
      (*Bcb)->RefCount++;
   }
   KeReleaseSpinLock(&(*Bcb)->BcbLock, oldIrql);

   return(STATUS_SUCCESS);
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







