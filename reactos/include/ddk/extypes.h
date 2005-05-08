/* $Id$ */

#ifndef __INCLUDE_DDK_EXTYPES_H
#define __INCLUDE_DDK_EXTYPES_H

typedef ULONG INTERLOCKED_RESULT;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType,
    NonPagedPoolSession = 32,
    PagedPoolSession,
    NonPagedPoolMustSucceedSession,
    DontUseThisTypeSession,
    NonPagedPoolCacheAlignedSession,
    PagedPoolCacheAlignedSession,
    NonPagedPoolCacheAlignedMustSSession
} POOL_TYPE;

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef struct _EX_QUEUE_WORKER_INFO {
    UCHAR QueueDisabled:1;
    UCHAR MakeThreadsAsNecessary:1;
    UCHAR WaitMode:1;
    ULONG WorkerCount:29;
} EX_QUEUE_WORKER_INFO, *PEX_QUEUE_WORKER_INFO;

typedef struct _EX_WORK_QUEUE {
    KQUEUE WorkerQueue;
    ULONG DynamicThreadCount;
    ULONG WorkItemsProcessed;
    ULONG WorkItemsProcessedLastPass;
    ULONG QueueDepthLastPass;
    EX_QUEUE_WORKER_INFO Info;    
} EX_WORK_QUEUE, *PEX_WORK_QUEUE;

typedef ULONG_PTR ERESOURCE_THREAD, *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY
{
   ERESOURCE_THREAD OwnerThread;
   union
     {
	LONG OwnerCount;
	ULONG TableSize;
     }; /* anon */
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
     }; /* anon */
   KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

#define EX_RUNDOWN_ACTIVE       0x1
#define EX_RUNDOWN_COUNT_SHIFT 0x1
#define EX_RUNDOWN_COUNT_INC   (0x1 << EX_RUNDOWN_COUNT_SHIFT)

typedef struct _RUNDOWN_DESCRIPTOR {
    ULONG_PTR References;
    KEVENT RundownEvent;
} RUNDOWN_DESCRIPTOR, *PRUNDOWN_DESCRIPTOR;

typedef struct _EX_RUNDOWN_REF {
    union {
        ULONG_PTR Count;
        PRUNDOWN_DESCRIPTOR Ptr;
    };
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

typedef struct _EX_FAST_REF {
    union {
        PVOID Object;
        ULONG RefCnt:3;
        ULONG Value;
    };
} EX_FAST_REF, *PEX_FAST_REF;

typedef struct _EX_PUSH_LOCK {
    union {
        struct {
            ULONG Locked:1;
            ULONG Waiting:1;
            ULONG Waking:1;
            ULONG MultipleShared:1;
            ULONG Shared:28;
        };
        ULONG Value;
        PVOID Ptr;
    };
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

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

typedef struct _ZONE_SEGMENT_HEADER
{
   SINGLE_LIST_ENTRY SegmentList; /* was Entry */
   PVOID Reserved;  /* was ULONG Size; */
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;



typedef VOID STDCALL_FUNC
(*PWORKER_THREAD_ROUTINE)(PVOID Parameter);


/* Modified by Andrew Greenwood, 16th July 2003: */

typedef struct _WORK_QUEUE_ITEM
{
   LIST_ENTRY List;
   PWORKER_THREAD_ROUTINE WorkerRoutine;
   PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef PVOID STDCALL_FUNC
(*PALLOCATE_FUNCTION)(POOL_TYPE PoolType,
		      ULONG NumberOfBytes,
		      ULONG Tag);

typedef VOID STDCALL_FUNC
(*PFREE_FUNCTION)(PVOID Buffer);

typedef union _SLIST_HEADER
{
   ULONGLONG Alignment;
   struct
     {
	SLIST_ENTRY Next;
	USHORT Depth;
	USHORT Sequence;	
     }; /* now anonymous */
} SLIST_HEADER, *PSLIST_HEADER;

typedef struct _NPAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT Depth;
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
   KSPIN_LOCK Obsoleted;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct _PAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT Depth;
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
   FAST_MUTEX Obsoleted;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

typedef struct _PP_LOOKASIDE_LIST {
   struct _GENERAL_LOOKASIDE *P;
   struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

typedef enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41

    } EX_POOL_PRIORITY;

typedef enum _SUITE_TYPE {
    SmallBusiness,
    Enterprise,
    BackOffice,
    CommunicationServer,
    TerminalServer,
    SmallBusinessRestricted,
    EmbeddedNT,
    DataCenter,
    SingleUserTS,
    Personal,
    Blade,
    MaxSuiteType
} SUITE_TYPE;

typedef enum _HARDERROR_RESPONSE_OPTION {
	OptionAbortRetryIgnore,
	OptionOk,
	OptionOkCancel,
	OptionRetryCancel,
	OptionYesNo,
	OptionYesNoCancel,
	OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE {
	ResponseReturnToCaller,
	ResponseNotHandled,
	ResponseAbort,
	ResponseCancel,
	ResponseIgnore,
	ResponseNo,
	ResponseOk,
	ResponseRetry,
	ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

/* callback object (not functional in NT4)*/

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID STDCALL_FUNC
(*PCALLBACK_FUNCTION)(PVOID CallbackContext,
		      PVOID Argument1,
		      PVOID Argument2);

#endif /* __INCLUDE_DDK_EXTYPES_H */

/* EOF */
