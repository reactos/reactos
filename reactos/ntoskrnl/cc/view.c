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

#define CACHE_SEGMENT_SIZE     (0x10000)

#define CACHE_SEGMENT_INVALID  (0)     // Isn't valid
#define CACHE_SEGMENT_WRITTEN  (1)     // Written
#define CACHE_SEGMENT_READ     (2)

typedef struct _CACHE_SEGMENT
{
   ULONG Type;                    // Debugging
   ULONG Size;
   LIST_ENTRY ListEntry;          // Entry in the per-open list of segments
   PVOID BaseAddress;             // Base address of the mapping   
   ULONG ValidLength;                  // Length of the mapping
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


 
NTSTATUS Cc1RequestView(PCC1_CCB CacheDesc,
			ULONG FileOffset,
			ULONG Length,
			PCACHE_SEGMENT ReturnedSegments[],
			PULONG NrSegments)
/*
 * FUNCTION: Request a view for caching data
 */
{
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;
   PCACHE_SEGMENT new_segment;
   ULONG MaxSegments;
   ULONG LengthDelta;
   
   MaxSegments = *NrSegments;
   (*NrSegments) = 0;
   
   KeAcquireSpinLock(&CacheDesc->CacheSegmentListLock);
   
   current_entry = CacheDesc->CacheSegmentListHead.Flink;
   while (current_entry != &(CacheDesc->CacheSegmentListHead))
     {
	current = CONTAING_RECORD(current_entry, CACHE_SEGMENT, ListEntry);
	
	if (current->FileOffset <= FileOffset &&
	    (current->FileOffset + current->ValidLength) > FileOffset)
	  {
	     ReturnedSegments[(*NrSegments)] = current;
	     (*NrSegments)++;
	     FileOffset = current->FileOffset + current->ValidLength;
	     LengthDelta = (FileOffset - current->FileOffset);
	     if (Length <= LengthDelta)
	       {
		  KeReleaseSpinLock(&CacheDesc->CacheSegmentListLock);
		  return(STATUS_SUCCESS);
	       }
	     Length = Length - LengthDelta;
	  }
	else if (current->FileOffset <= (FileOffset + Length) &&
                 (current->FileOffset + current->ValidLength) >
		 (FileOffset + Length))
	  {
	     ReturnedSegments[(*NrSegments)] = current;
	     (*NrSegments)++;
	     Length = Length - ((FileOffset + Length) - current->FileOffset);
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
   if (CacheDesc == NULL)
     {
	return(NULL);
     }
   
   CacheDesc->Type = CC1_CCB_ID;
   InitializeListHead(&CacheDesc->CacheSegmentListHead);
   KeInitializeSpinLock(&CacheDesc->CacheSegmentListLock);
   
   return(CacheDesc);
}
