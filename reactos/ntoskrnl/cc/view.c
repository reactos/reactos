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

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

#define CACHE_SEGMENT_INVALID  (0)     // Isn't valid
#define CACHE_SEGMENT_WRITTEN  (1)     // Written
#define CACHE_SEGMENT_READ     (2)

typedef struct _CACHE_SEGMENT
{
   ULONG Type;                    // Debugging
   ULONG Size;
   LIST_ENTRY ListEntry;          // Entry in the per-open list of segments
   PVOID BaseAddress;             // Base address of the mapping   
   ULONG Length;                  // Length of the mapping
   ULONG State;                   // Information
   MEMORY_AREA* MemoryArea;       // Memory area for the mapping
   ULONG FileOffset;              // Offset within the file of the mapping
   KEVENT Event;
   BOOLEAN Dirty;                 // Contains dirty data
} CACHE_SEGMENT, *PCACHE_SEGMENT;

typedef struct _CC1_CCB
{
   ULONG Type;
   ULONG Size;
   LIST_ENTRY CacheSegmentListHead;
   KSPIN_LOCK CacheSegmentListLock;
   LIST_ENTRY ListEntry;
} CC1_CCB, PCC1_CCB;

/* FUNCTIONS *****************************************************************/

PVOID Cc1FlushView(PCC1_CCB CacheDesc,
		   ULONG FileOffset,
		   ULONG Length)
{
}

PVOID Cc1PurgeView(PCC1_CCB CacheDesc,
		   ULONG FileOffset,
		   ULONG Length)
{
}

BOOLEAN Cc1AcquireCacheSegment(PCACHE_SEGMENT CacheSegment,
			       BOOLEAN AcquireForWrite,
			       BOOLEAN Wait)
{
   
}
 
PVOID Cc1RequestView(PCC1_CCB CacheDesc,
		     ULONG FileOffset,
		     ULONG Length,
		     BOOLEAN Wait,
		     BOOLEAN AcquireForWrite)
/*
 * FUNCTION: Request a view for caching data
 * ARGUMENTS:
 *          FileObject = File to have information cached in the view
 *          FileOffset = Offset within the file of the cached information
 *          Length = Length of the information to be cached
 *          Wait = If the view is being created then wait for the creater
 *                 to make the view valid
 *          AcquireForWrite = True if the view is being acquired for writing
 *          Buffer = Pointer to a variable to hold the base address of the view
 * RETURNS: True if the view contains valid data,
 *          False otherwise
 */
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   
   KeAcquireSpinLock(&CacheDesc->CacheSegmentListLock);
   
   current_entry = CacheDesc->CacheSegmentListHead.Flink;
   while (current_entry != &CacheDesc->CacheSegmentListHead)
     {
	current = CONTAING_RECORD(current_entry, CACHE_SEGMENT, ListEntry);       	
	
	if (current->FileOffset <= FileOffset &&
	    (current->FileOffset + current->length) >= (FileOffset + Length))
	  {
	     if (!Cc1AcquireCacheSegment(AcquireForWrite, Wait))
	       {
		  return(NULL);
	       }
	     return(current->BaseAddress + (FileOffset - current->FileOffset));
	  }
	current_entry = current_entry->Flink;
     }
   
   KeReleaseSpinLock(&CacheDesc->CacheSegmentListLock);
}

PCC1_CCB Cc1InitializeFileCache(PFILE_OBJECT FileObject)
/*
 * FUNCTION: Initialize caching for a file
 */
{
   PCC1_CCB CacheDesc;
   
   CacheDesc = ExAllocatePool(NonPagedPool, sizeof(CC1_CCB));
   InitializeListHead(&CacheDesc->CacheSegmentListHead);
   KeAcquireSpinLock(&CacheDesc->CacheSegmentListLock);
   
   return(CacheDesc);
}
