#ifndef _NTOS_EXFUNCS_H
#define _NTOS_EXFUNCS_H

/* EXECUTIVE ROUTINES ******************************************************/

VOID
FASTCALL
ExAcquireFastMutex (
	PFAST_MUTEX	FastMutex
	);
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
	PFAST_MUTEX	FastMutex
	);
BOOLEAN
STDCALL
ExAcquireResourceExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireResourceExclusiveLite (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireResourceSharedLite (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireSharedStarveExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireSharedWaitForExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
PVOID
STDCALL
ExAllocateFromNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	LookSide
	);
PVOID
STDCALL
ExAllocateFromPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	LookSide
	);
PVOID
STDCALL
ExAllocateFromZone (
	PZONE_HEADER	Zone
	);
/*
 * FUNCTION: Allocates memory from the nonpaged pool
 * ARGUMENTS:
 *      NumberOfBytes = minimum size of the block to be allocated
 *      PoolType = the type of memory to use for the block (ignored)
 * RETURNS:
 *      the address of the block if it succeeds
 */
PVOID
STDCALL
ExAllocatePool (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);

PVOID
STDCALL
ExAllocatePoolWithQuota (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);
PVOID
STDCALL
ExAllocatePoolWithQuotaTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
PVOID
STDCALL
ExAllocatePoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
VOID
STDCALL
ExConvertExclusiveToSharedLite (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExDeleteNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	);
VOID
STDCALL
ExDeletePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	);
NTSTATUS
STDCALL
ExDeleteResource (
	PERESOURCE	Resource
	);
NTSTATUS
STDCALL
ExDeleteResourceLite (
	PERESOURCE	Resource
	);
NTSTATUS
STDCALL
ExExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize
	);

/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
VOID
STDCALL
ExFreePool (
	PVOID	block
	);

VOID
STDCALL
ExFreeToNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	);
VOID
STDCALL
ExFreeToPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	);
PVOID
STDCALL
ExFreeToZone (
	PZONE_HEADER	Zone,
	PVOID		Block
	);
ERESOURCE_THREAD
STDCALL
ExGetCurrentResourceThread (
	VOID
	);
ULONG
STDCALL
ExGetExclusiveWaiterCount (
	PERESOURCE	Resource
	);
ULONG
STDCALL
ExGetSharedWaiterCount (
	PERESOURCE	Resource
	);
/* ReactOS Specific: begin */
VOID
FASTCALL
ExInitializeFastMutex (
	PFAST_MUTEX	FastMutex
	);
/* ReactOS Specific: end */
VOID
STDCALL
ExInitializeNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	);
VOID
STDCALL
ExInitializePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	);
NTSTATUS
STDCALL
ExInitializeResource (
	PERESOURCE	Resource
	);
NTSTATUS
STDCALL
ExInitializeResourceLite (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExInitializeSListHead (
	PSLIST_HEADER	SListHead
	);
VOID
STDCALL
ExInitializeWorkItem (
	PWORK_QUEUE_ITEM	Item,
	PWORKER_THREAD_ROUTINE	Routine,
	PVOID			Context
	);
NTSTATUS
STDCALL
ExInitializeZone (
	PZONE_HEADER	Zone,
	ULONG		BlockSize,
	PVOID		InitialSegment,
	ULONG		InitialSegmentSize
	);
LARGE_INTEGER
STDCALL
ExInterlockedAddLargeInteger (
	PLARGE_INTEGER	Addend,
	LARGE_INTEGER	Increment,
	PKSPIN_LOCK	Lock
	);
ULONG
STDCALL
ExInterlockedAddUlong (
	PULONG		Addend,
	ULONG		Increment,
	PKSPIN_LOCK	Lock
	);
PVOID
STDCALL
ExInterlockedAllocateFromZone (
	PZONE_HEADER	Zone,
	PKSPIN_LOCK	Lock
	);
INTERLOCKED_RESULT
STDCALL
ExInterlockedDecrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	);
ULONG
STDCALL
ExInterlockedExchangeUlong (
	PULONG		Target,
	ULONG		Value,
	PKSPIN_LOCK	Lock
	);
NTSTATUS
STDCALL
ExInterlockedExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize,
	PKSPIN_LOCK	Lock
	);
PVOID
STDCALL
ExInterlockedFreeToZone (
	PZONE_HEADER	Zone,
	PVOID		Block,
	PKSPIN_LOCK	Lock
	);
INTERLOCKED_RESULT
STDCALL
ExInterlockedIncrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	);
PLIST_ENTRY
STDCALL
ExInterlockedInsertHeadList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	);
PLIST_ENTRY
STDCALL
ExInterlockedInsertTailList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPopEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PKSPIN_LOCK		Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPopEntrySList (
	PSLIST_HEADER	ListHead,
	PKSPIN_LOCK	Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPushEntrySList (
	PSLIST_HEADER		ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	);

VOID
ExInterlockedRemoveEntryList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry,
	PKSPIN_LOCK	Lock
	);

PLIST_ENTRY
STDCALL
ExInterlockedRemoveHeadList (
	PLIST_ENTRY	Head,
	PKSPIN_LOCK	Lock
	);
BOOLEAN
STDCALL
ExIsFullZone (
	PZONE_HEADER	Zone
	);
BOOLEAN
STDCALL
ExIsObjectInFirstZoneSegment (
	PZONE_HEADER	Zone,
	PVOID		Object
	);
BOOLEAN
STDCALL
ExIsResourceAcquiredExclusiveLite (
	PERESOURCE	Resource
	);
ULONG
STDCALL
ExIsResourceAcquiredSharedLite (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	);
USHORT
STDCALL
ExQueryDepthSListHead (
	PSLIST_HEADER	SListHead
	);
VOID
STDCALL
ExQueueWorkItem (
	PWORK_QUEUE_ITEM	WorkItem,
	WORK_QUEUE_TYPE		QueueType
	);
VOID
STDCALL
ExRaiseAccessViolation (
	VOID
	);
VOID
STDCALL
ExRaiseDatatypeMisalignment (
	VOID
	);
VOID
STDCALL
ExRaiseStatus (
	NTSTATUS	Status
	);
VOID
STDCALL
ExReinitializeResourceLite (
	PERESOURCE	Resource
	);
/* ReactOS Specific: begin */
VOID
FASTCALL
ExReleaseFastMutex (
	PFAST_MUTEX	Mutex
	);
/* ReactOS Specific: end */
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
	PFAST_MUTEX	Mutex
	);
VOID
STDCALL
ExReleaseResource (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExReleaseResourceLite (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExReleaseResourceForThread (
	PERESOURCE		Resource,
	ERESOURCE_THREAD	ResourceThreadId
	);
VOID
STDCALL
ExReleaseResourceForThreadLite (
	PERESOURCE		Resource,
	ERESOURCE_THREAD	ResourceThreadId
	);
VOID
STDCALL
ExSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	);
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
	PFAST_MUTEX	FastMutex
	);
BOOLEAN
STDCALL
ExTryToAcquireResourceExclusiveLite (
	PERESOURCE	Resource
	);
/*
LONG
FASTCALL
InterlockedCompareExchange (
	PLONG	Target,
	LONG	Value,
	LONG	Reference
	);
*/
PVOID
FASTCALL
InterlockedCompareExchange (
	PVOID	* Destination,
	PVOID	Exchange,
	PVOID	Comperand
	);
#ifdef _GNU_H_WINDOWS_H
#ifdef InterlockedDecrement
#undef InterlockedDecrement
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedIncrement
#endif /* def InterlockedDecrement */
#endif /* def _GNU_H_WINDOWS_H */
LONG
FASTCALL
InterlockedDecrement (
	PLONG	Addend
	);
LONG
FASTCALL
InterlockedExchange (
	PLONG	Target,
	LONG	Value
	);
LONG
FASTCALL
InterlockedExchangeAdd (
	PLONG	Addend,
	LONG	Value
	);
LONG
FASTCALL
InterlockedIncrement (
	PLONG	Addend
	);

VOID
RemoveEntryFromList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	Entry
	);
/*---*/

typedef
unsigned int
(exception_hook) (
	CONTEXT		* c,
	unsigned int	exp
	);
unsigned int
ExHookException (
	exception_hook	fn,
	unsigned int	exp
	);

#endif /* ndef _NTOS_EXFUNCS_H */
