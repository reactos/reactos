/* $Id: zone.c,v 1.3 2002/09/07 15:12:50 chorns Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/mm/zone.c
 * PURPOSE:       Implements zone buffers
 * PROGRAMMER:    David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* VARIABLES ***************************************************************/

typedef struct _ZONE_SEGMENT
{
   SINGLE_LIST_ENTRY Entry;
   ULONG size;
} ZONE_SEGMENT, *PZONE_SEGMENT;

typedef struct _ZONE_ENTRY
{
   SINGLE_LIST_ENTRY Entry;
} ZONE_ENTRY, *PZONE_ENTRY;

/* FUNCTIONS ***************************************************************/

NTSTATUS
STDCALL
ExExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize
	)
{
   PZONE_ENTRY entry;
   PZONE_SEGMENT seg;
   unsigned int i;
   
   seg = (PZONE_SEGMENT)Segment;
   seg->size = SegmentSize;
   
   PushEntryList(&Zone->SegmentList,&seg->Entry);
   
   entry = (PZONE_ENTRY)( ((PVOID)seg) + sizeof(ZONE_SEGMENT) );
   
   for (i=0;i<(SegmentSize / Zone->BlockSize);i++)
     {
	PushEntryList(&Zone->FreeList,&entry->Entry);
	entry = (PZONE_ENTRY)(((PVOID)entry) + sizeof(ZONE_ENTRY) + 
			      Zone->BlockSize);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
ExInterlockedExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize,
	PKSPIN_LOCK	Lock
	)
{
   NTSTATUS ret;
   KIRQL oldlvl;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret = ExExtendZone(Zone,Segment,SegmentSize);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}


NTSTATUS
STDCALL
ExInitializeZone (
	PZONE_HEADER	Zone,
	ULONG		BlockSize,
	PVOID		InitialSegment,
	ULONG		InitialSegmentSize
	)
/*
 * FUNCTION: Initalizes a zone header
 * ARGUMENTS:
 *          Zone = zone header to be initialized
 *          BlockSize = Size (in bytes) of the allocation size of the zone
 *          InitialSegment = Initial segment of storage allocated by the 
 *                           caller
 *          InitialSegmentSize = Initial size of the segment
 */
{
   unsigned int i;
   PZONE_SEGMENT seg;
   PZONE_ENTRY entry;
   
   Zone->FreeList.Next=NULL;
   Zone->SegmentList.Next=NULL;
   Zone->BlockSize=BlockSize;
   Zone->TotalSegmentSize = InitialSegmentSize;
   
   seg = (PZONE_SEGMENT)InitialSegment;
   seg->size = InitialSegmentSize;
   
   PushEntryList(&Zone->SegmentList,&seg->Entry);
   
   entry = (PZONE_ENTRY)( ((PVOID)seg) + sizeof(ZONE_SEGMENT) );
   
   for (i=0;i<(InitialSegmentSize / BlockSize);i++)
     {
	PushEntryList(&Zone->FreeList,&entry->Entry);
	entry = (PZONE_ENTRY)(((PVOID)entry) + sizeof(ZONE_ENTRY) + BlockSize);
     }

   return(STATUS_SUCCESS);
}

/* EOF */
