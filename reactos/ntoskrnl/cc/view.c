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
#include <internal/bitops.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

#define CACHE_SEGMENT_SIZE (0x10000)

typedef struct _CACHE_SEGMENT
{
   PVOID BaseAddress;
   PMEMORY_AREA MemoryArea;
   ULONG ValidPages;
   ULONG AllocatedPages;
   LIST_ENTRY ListEntry;
   ULONG FileOffset;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

typedef struct _BCB
{
   LIST_ENTRY CacheSegmentListHead;
   PFILE_OBJECT FileObject;
   KSPIN_LOCK BcbLock;
} BCB, *PBCB;

/* FUNCTIONS *****************************************************************/

NTSTATUS CcRequestCachePage(PBCB Bcb,
			    ULONG FileOffset,
			    PVOID* BaseAddress,
			    PBOOLEAN UptoDate)
{
   KIRQL oldirql;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   ULONG InternalOffset;
   
   KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
   
   current_entry = Bcb->CacheSegmentListHead.Flink;
   while (current_entry != &Bcb->CacheSegmentListHead)
     {
	current = CONTAING_RECORD(current, CACHE_SEGMENT, ListEntry);
	if (current->FileOffset <= FileOffset &&
	    (current->FileOffset + CACHE_SEGMENT_SIZE) > FileOffset)
	  {
	     InternalOffset = (FileOffset - current->FileOffset);
	     
	     if (!test_bit(InternalOffset / PAGESIZE,
			   current->AllocatedPages))
	       {
		  MmSetPageEntry(PsGetCurrentProcess(),
				 current->BaseAddress + InternalOffset,
				 PAGE_READWRITE,
				 get_free_page());
	       }
	     if (!test_bit(InternalOffset / PAGESIZE,
			   current->ValidPages))
	       {
		  UptoDate = False;
	       }
	     else
	       {
		  UptoDate = True;
	       }
	     (*BaseAddress) = current->BaseAddress + InternalOffset;
	     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	     return(STATUS_SUCCESS);
	  }
	current_entry = current_entry->Flink;
     }
   
   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
}

NTSTATUS CcInitializeFileCache(PFILE_OBJECT FileObject,
			       PBCB* Bcb)
{
   (*Bcb) = ExAllocatePool(NonPagedPool, sizeof(BCB));
   if ((*Bcb) == NULL)
     {
	return(STATUS_OUT_OF_MEMORY);
     }
   
   (*Bcb)->FileObject = FileObject;
   InitializeListHead(&(*Bcb)->CacheSegmentListHead);
   KeInitializeSpinLock(&(*Bcb)->BcbLock);
   
   return(STATUS_SUCCESS);
}



