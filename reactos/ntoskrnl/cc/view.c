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

typedef struct _CACHE_SEGMENT
{
   ULONG Type;
   ULONG Size;
   LIST_ENTRY ListEntry;
   PVOID BaseAddress;
   ULONG Length;
   ULONG State;
   MEMORY_AREA* MemoryArea;
   ULONG FileOffset;
   ULONG InternalOffset;
} CACHE_SEGMENT, *PCACHE_SEGMENT;

typedef struct _CC1_CCB
{
   ULONG Type;
   ULONG Size;
   LIST_ENTRY CacheSegmentListHead;
   LIST_ENTRY ListEntry;
} CC1_CCB, PCC1_CCB;

/* FUNCTIONS *****************************************************************/

PVOID Cc1FlushView(PFILE_OBJECT FileObject,
		   ULONG FileOffset,
		   ULONG Length)
{
}

PVOID Cc1PurgeView(PFILE_OBJECT FileObject,
		   ULONG FileOffset,
		   ULONG Length)
{
}

VOID Cc1ViewIsUpdated(PFILE_OBJECT FileObject,
		      ULONG FileOffset)
{
}

typedef 

BOOLEAN Cc1RequestView(PFILE_OBJECT FileObject,
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
}

BOOLEAN Cc1InitializeFileCache(PFILE_OBJECT FileObject)
/*
 * FUNCTION: Initialize caching for a file
 */
{
}
