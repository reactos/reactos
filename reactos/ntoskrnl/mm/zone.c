/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/mm/zone.c
 * PURPOSE:       Implements zone buffers
 * PROGRAMMER:    David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

/* FUNCTIONS ***************************************************************/

inline static PZONE_ENTRY block_to_entry(PVOID Block)
{
   return( (PZONE_ENTRY)(Block - sizeof(ZONE_ENTRY)) );
}

inline static PVOID entry_to_block(PZONE_ENTRY Entry)
{
   return( (PVOID)( ((PVOID)Entry) + sizeof(ZONE_ENTRY)));
}

BOOLEAN ExIsObjectInFirstZoneSegment(PZONE_HEADER Zone, PVOID Object)
{
   PVOID Base = (PVOID)(Zone + sizeof(ZONE_HEADER) + sizeof(ZONE_SEGMENT) + 
			sizeof(ZONE_ENTRY));
   PZONE_SEGMENT seg = (PZONE_SEGMENT)(Zone + sizeof(ZONE_HEADER));
   ULONG length = seg->size;
   return( (Object > Base) && (Object < (Base + length)));
}

NTSTATUS ExExtendZone(PZONE_HEADER Zone, PVOID Segment, ULONG SegmentSize)
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

NTSTATUS ExInterlockedExtendZone(PZONE_HEADER Zone, PVOID Segment,
				 ULONG SegmentSize, PKSPIN_LOCK Lock)
{
   NTSTATUS ret;
   KIRQL oldlvl;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret = ExExtendZone(Zone,Segment,SegmentSize);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

BOOLEAN ExIsFullZone(PZONE_HEADER Zone)
{
   return(Zone->FreeList.Next==NULL);
}

PVOID ExAllocateFromZone(PZONE_HEADER Zone)
/*
 * FUNCTION: Allocate a block from a zone
 * ARGUMENTS:
 *         Zone = Zone to allocate from
 * RETURNS: The base address of the block allocated
 */
{
   PSINGLE_LIST_ENTRY list_entry = PopEntryList(&Zone->FreeList);
   PZONE_ENTRY entry = CONTAINING_RECORD(list_entry,ZONE_ENTRY,Entry);
   return(entry_to_block(entry));
}

PVOID ExFreeToZone(PZONE_HEADER Zone, PVOID Block)
/*
 * FUNCTION: Frees a block from a zone
 * ARGUMENTS:
 *        Zone = Zone the block was allocated from
 *        Block = Block to free
 */
{
   PZONE_ENTRY entry  = block_to_entry(Block);
   PZONE_ENTRY ret = entry_to_block((PZONE_ENTRY)Zone->FreeList.Next);
   PushEntryList(&Zone->FreeList,&entry->Entry);
   return(ret);
}

NTSTATUS ExInitializeZone(PZONE_HEADER Zone, ULONG BlockSize, 
			  PVOID InitialSegment, ULONG InitialSegmentSize)
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
}

PVOID ExInterlockedFreeToZone(PZONE_HEADER Zone, PVOID Block,
			      PKSPIN_LOCK Lock)
{
   KIRQL oldlvl;
   PVOID ret;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret=ExFreeToZone(Zone,Block);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

PVOID ExInterlockedAllocateFromZone(PZONE_HEADER Zone, PKSPIN_LOCK Lock)
{
   PVOID ret;
   KIRQL oldlvl;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret=ExAllocateFromZone(Zone);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}
