/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/view.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>

//#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

#define CACHE_SEGMENT_SIZE (0x1000)

typedef struct _CACHE_SEGMENT
{
   PVOID BaseAddress;
   PMEMORY_AREA MemoryArea;
   BOOLEAN Valid;
   LIST_ENTRY ListEntry;
   ULONG FileOffset;
   KEVENT Lock;
   ULONG ReferenceCount;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

/* FUNCTIONS *****************************************************************/

NTSTATUS CcRequestCachePage(PBCB Bcb,
			    ULONG FileOffset,
			    PVOID* BaseAddress,
			    PBOOLEAN UptoDate)
{
   KIRQL oldirql;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   
   DPRINT("CcRequestCachePage(Bcb %x, FileOffset %x)\n");
   
   KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != &Bcb->CacheSegmentListHead)
     {
	current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, ListEntry);
	if (current->FileOffset == PAGE_ROUND_DOWN(FileOffset))
	  {
	     current->ReferenceCount++;
	     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	     KeWaitForSingleObject(&current->Lock,
				   Executive,
				   KernelMode,
				   FALSE,
				   NULL);
	     *UptoDate = current->Valid;
	     BaseAddress = current->BaseAddress;
	     return(STATUS_SUCCESS);
	  }
	current_entry = current_entry->Flink;
     }

   current = ExAllocatePool(NonPagedPool, sizeof(CACHE_SEGMENT));
   current->BaseAddress = NULL;
   MmCreateMemoryArea(KernelMode,
		      PsGetCurrentProcess(),
		      MEMORY_AREA_CACHE_SEGMENT,
		      &current->BaseAddress,
		      CACHE_SEGMENT_SIZE,
		      PAGE_READWRITE,
		      &current->MemoryArea);
   current->Valid = FALSE;
   current->FileOffset = PAGE_ROUND_DOWN(FileOffset);
   KeInitializeEvent(&current->Lock, SynchronizationEvent, TRUE);
   current->ReferenceCount = 1;
   InsertTailList(&Bcb->CacheSegmentListHead, &current->ListEntry);
   *UptoDate = current->Valid;
   *BaseAddress = current->BaseAddress;
   MmSetPage(PsGetCurrentProcess(),
	     current->BaseAddress, 
	     (ULONG)MmAllocPage(), 
	     PAGE_READWRITE);
   
   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
   
   return(STATUS_SUCCESS);
}

NTSTATUS CcInitializeFileCache(PFILE_OBJECT FileObject,
			       PBCB* Bcb)
{
   DPRINT("CcInitializeFileCache(FileObject %x)\n",FileObject);
   
   (*Bcb) = ExAllocatePool(NonPagedPool, sizeof(BCB));
   if ((*Bcb) == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   (*Bcb)->FileObject = FileObject;
   InitializeListHead(&(*Bcb)->CacheSegmentListHead);
   KeInitializeSpinLock(&(*Bcb)->BcbLock);
   
   DPRINT("Finished CcInitializeFileCache() = %x\n", *Bcb);
   
   return(STATUS_SUCCESS);
}



