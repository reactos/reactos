/* $Id: extypes.h,v 1.6 2001/08/30 23:50:53 ekohl Exp $ */

#ifndef __INCLUDE_DDK_EXTYPES_H
#define __INCLUDE_DDK_EXTYPES_H

#ifdef __NTOSKRNL__
extern POBJECT_TYPE EXPORTED ExDesktopObjectType;
extern POBJECT_TYPE EXPORTED ExEventObjectType;
extern POBJECT_TYPE EXPORTED ExWindowStationObjectType;
#else
extern POBJECT_TYPE IMPORTED ExDesktopObjectType;
extern POBJECT_TYPE IMPORTED ExEventObjectType;
extern POBJECT_TYPE IMPORTED ExWindowStationObjectType;
#endif

typedef ULONG INTERLOCKED_RESULT;
typedef ULONG WORK_QUEUE_TYPE;

typedef ULONG ERESOURCE_THREAD, *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY
{
   ERESOURCE_THREAD OwnerThread;
   union
     {
	LONG OwnerCount;
	ULONG TableSize;
     } a;
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE
{
   LIST_ENTRY SystemResourcesList;
   POWNER_ENTRY OwnerTable;
   SHORT ActiveCount;
   USHORT Flag;
   PKSEMAPHORE SharedWaiters;
   PKEVENT ExclusiveWaiters;
   OWNER_ENTRY OwnerThreads[2];
   ULONG ContentionCount;
   USHORT NumberOfSharedWaiters;
   USHORT NumberOfExclusiveWaiters;
   union
     {
	PVOID Address;
	ULONG CreatorBackTraceIndex;
     } a;
   KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;


typedef struct 
{
   LONG Count;
   struct _KTHREAD* Owner;
   ULONG Contention;
   KEVENT Event;
   ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef struct _ZONE_HEADER
{
   SINGLE_LIST_ENTRY FreeList;
   SINGLE_LIST_ENTRY SegmentList;
   ULONG BlockSize;
   ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

typedef struct _ZONE_SEGMENT
{
   SINGLE_LIST_ENTRY Entry;
   ULONG size;
} ZONE_SEGMENT, *PZONE_SEGMENT;

typedef struct _ZONE_ENTRY
{
   SINGLE_LIST_ENTRY Entry;
} ZONE_ENTRY, *PZONE_ENTRY;


typedef VOID STDCALL
(*PWORKER_THREAD_ROUTINE)(PVOID Parameter);

typedef struct _WORK_QUEUE_ITEM
{
   LIST_ENTRY Entry;
   PWORKER_THREAD_ROUTINE Routine;
   PVOID Context;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef PVOID STDCALL
(*PALLOCATE_FUNCTION)(POOL_TYPE PoolType,
		      ULONG NumberOfBytes,
		      ULONG Tag);

typedef VOID STDCALL
(*PFREE_FUNCTION)(PVOID Buffer);

typedef union _SLIST_HEADER
{
   ULONGLONG Alignment;
   struct
     {
	SINGLE_LIST_ENTRY Next;
	USHORT Depth;
	USHORT Sequence;	
     } s;
} SLIST_HEADER, *PSLIST_HEADER;

typedef struct _NPAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT MinimumDepth;
   USHORT MaximumDepth;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG FreeMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   ULONG LastTotalAllocates;
   ULONG LastAllocateMisses;
   ULONG Pad[2];
   KSPIN_LOCK Lock;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct _PAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT MinimumDepth;
   USHORT MaximumDepth;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG FreeMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   ULONG LastTotalAllocates;
   ULONG LastAllocateMisses;
   FAST_MUTEX Lock;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;


/* callback object (not functional in NT4)*/

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID STDCALL
(*PCALLBACK_FUNCTION)(PVOID CallbackContext,
		      PVOID Argument1,
		      PVOID Argument2);

#endif /* __INCLUDE_DDK_EXTYPES_H */

/* EOF */
