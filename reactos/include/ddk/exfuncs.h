/* EXECUTIVE ROUTINES ******************************************************/

VOID ExAcquireFastMutex(PFAST_MUTEX FastMutex);
VOID ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex);

BOOLEAN ExAcquireResourceExclusive(PERESOURCE Resource, BOOLEAN Wait);
BOOLEAN ExAcquireResourceExclusiveLite(PERESOURCE Resource, BOOLEAN Wait);
BOOLEAN ExAcquireResourceSharedLite(PERESOURCE Resource, BOOLEAN Wait);
BOOLEAN ExAcquireSharedStarveExclusive(PERESOURCE Resource, BOOLEAN Wait);
BOOLEAN ExAcquireSharedWaitForExclusive(PERESOURCE Resource, BOOLEAN Wait);
PVOID ExAllocateFromNPagedLookasideList(PNPAGED_LOOKASIDE_LIST LookSide);
PVOID ExAllocateFromPagedLookasideList(PPAGED_LOOKASIDE_LIST LookSide);
PVOID ExAllocateFromZone(PZONE_HEADER Zone);

/*
 * FUNCTION: Allocates memory from the nonpaged pool
 * ARGUMENTS:
 *      size = minimum size of the block to be allocated
 *      PoolType = the type of memory to use for the block (ignored)
 * RETURNS:
 *      the address of the block if it succeeds
 */
PVOID ExAllocatePool(POOL_TYPE PoolType, ULONG size);

PVOID ExAllocatePoolWithQuota(POOL_TYPE PoolType, ULONG NumberOfBytes);
PVOID ExAllocatePoolWithQuotaTag(POOL_TYPE PoolType, ULONG NumberOfBytes,
				 ULONG Tag);
PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, ULONG NumberOfBytes,
			    ULONG Tag);
VOID ExConvertExclusiveToSharedLite(PERESOURCE Resource);
VOID ExDeleteNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside);
VOID ExDeletePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside);
NTSTATUS ExDeleteResource(PERESOURCE Resource);
NTSTATUS ExDeleteResourceLite(PERESOURCE Resource);
NTSTATUS ExExtendZone(PZONE_HEADER Zone, PVOID Segment, ULONG SegmentSize);

/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
VOID ExFreePool(PVOID block);

VOID ExFreeToNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
				 PVOID Entry);
VOID ExFreeToPagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside,
				 PVOID Entry);
PVOID ExFreeToZone(PZONE_HEADER Zone, PVOID Block);
ERESOURCE_THREAD ExGetCurrentResourceThread(VOID);
ULONG ExGetExclusiveWaiterCount(PERESOURCE Resource);
ULONG ExGetSharedWaiterCount(PERESOURCE Resource);
VOID ExInitializeFastMutex(PFAST_MUTEX FastMutex);
VOID ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
				     PALLOCATE_FUNCTION Allocate,
				     PFREE_FUNCTION Free,
				     ULONG Flags,
				     ULONG Size,
				     ULONG Tag,
				     USHORT Depth);
VOID ExInitializePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside,
				     PALLOCATE_FUNCTION Allocate,
				     PFREE_FUNCTION Free,
				     ULONG Flags,
				     ULONG Size,
				     ULONG Tag,
				     USHORT Depth);
NTSTATUS ExInitializeResource(PERESOURCE Resource);
NTSTATUS ExInitializeResourceLite(PERESOURCE Resource);
VOID ExInitializeSListHead(PSLIST_HEADER SListHead);
VOID ExInitializeWorkItem(PWORK_QUEUE_ITEM Item,
			  PWORKER_THREAD_ROUTINE Routine,
			  PVOID Context);
NTSTATUS ExInitializeZone(PZONE_HEADER Zone,
			  ULONG BlockSize,
			  PVOID InitialSegment,
			  ULONG InitialSegmentSize);
LARGE_INTEGER ExInterlockedAddLargeInteger(PLARGE_INTEGER Addend,
					   LARGE_INTEGER Increment,
					   PKSPIN_LOCK Lock);
ULONG ExInterlockedAddUlong(PULONG Addend, ULONG Increment, PKSPIN_LOCK Lock);

VOID ExInterlockedRemoveEntryList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry,
				  PKSPIN_LOCK Lock);
VOID RemoveEntryFromList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);   
PLIST_ENTRY ExInterlockedRemoveHeadList(PLIST_ENTRY Head, PKSPIN_LOCK Lock);

PLIST_ENTRY ExInterlockedInsertTailList(PLIST_ENTRY ListHead,
					PLIST_ENTRY ListEntry,
					PKSPIN_LOCK Lock);

PLIST_ENTRY ExInterlockedInsertHeadList(PLIST_ENTRY ListHead,
					PLIST_ENTRY ListEntry,
					PKSPIN_LOCK Lock);

VOID ExQueueWorkItem(PWORK_QUEUE_ITEM WorkItem,
		     WORK_QUEUE_TYPE QueueType);
