
extern POBJECT_TYPE ExEventType;

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


typedef VOID (*PWORKER_THREAD_ROUTINE)(PVOID Parameter);

typedef struct _WORK_QUEUE_ITEM
{
   LIST_ENTRY Entry;
   PWORKER_THREAD_ROUTINE Routine;
   PVOID Context;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef PVOID (*PALLOCATE_FUNCTION)(POOL_TYPE PoolType,
				   ULONG NumberOfBytes,
				   ULONG Tag);
typedef VOID (*PFREE_FUNCTION)(PVOID Buffer);

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

typedef struct
{
   SLIST_HEADER ListHead;
   USHORT Depth;
   USHORT Pad;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG TotalMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   KSPIN_LOCK Lock;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct
{
   SLIST_HEADER ListHead;
   USHORT Depth;
   USHORT Pad;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG TotalMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   FAST_MUTEX Lock;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;
