/* $Id$
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/mm/zone.c
 * PURPOSE:       Implements zone buffers
 * PROGRAMMER:    David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

/* FUNCTIONS ***************************************************************/

// undocumented? from extypes.h in here for now...

//typedef struct _ZONE_ENTRY
//{
//   SINGLE_LIST_ENTRY Entry;
//} ZONE_ENTRY, *PZONE_ENTRY;



/*
 * @implemented
 */
NTSTATUS
STDCALL
ExExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize
	)
{
   PZONE_SEGMENT_HEADER entry;
   PZONE_SEGMENT_HEADER seg;
   unsigned int i;
   
   seg = (PZONE_SEGMENT_HEADER)Segment;
   seg->Reserved = (PVOID) SegmentSize;
   
   PushEntryList(&Zone->SegmentList,&seg->SegmentList);
   
   entry = (PZONE_SEGMENT_HEADER)( ((char*)seg) + sizeof(ZONE_SEGMENT_HEADER) );
   
   for (i=0;i<(SegmentSize / Zone->BlockSize);i++)
     {
	PushEntryList(&Zone->FreeList,&entry->SegmentList);
	entry = (PZONE_SEGMENT_HEADER)(((char*)entry) + sizeof(PZONE_SEGMENT_HEADER) + 
			      Zone->BlockSize);
     }
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
   PZONE_SEGMENT_HEADER seg;
   PZONE_SEGMENT_HEADER entry;
   
   Zone->FreeList.Next=NULL;
   Zone->SegmentList.Next=NULL;
   Zone->BlockSize=BlockSize;
   Zone->TotalSegmentSize = InitialSegmentSize;
   
   seg = (PZONE_SEGMENT_HEADER)InitialSegment;
   seg->Reserved = (PVOID*) InitialSegmentSize;
   
   PushEntryList(&Zone->SegmentList,&seg->SegmentList);
   
   entry = (PZONE_SEGMENT_HEADER)( ((char*)seg) + sizeof(ZONE_SEGMENT_HEADER) );
   
   for (i=0;i<(InitialSegmentSize / BlockSize);i++)
     {
	PushEntryList(&Zone->FreeList,&entry->SegmentList);
	entry = (PZONE_SEGMENT_HEADER)(((char*)entry) + sizeof(PZONE_SEGMENT_HEADER) + BlockSize);
     }

   return(STATUS_SUCCESS);
}

/* EOF */
