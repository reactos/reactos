/* EXECUTIVE ROUTINES ******************************************************/

//VOID ExAcquireFastMutex(PFAST_MUTEX FastMutex);


/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
VOID ExFreePool(PVOID block);
   
/*
 * FUNCTION: Allocates memory from the nonpaged pool
 * ARGUMENTS:
 *      size = minimum size of the block to be allocated
 *      PoolType = the type of memory to use for the block (ignored)
 * RETURNS:
 *      the address of the block if it succeeds
 */
PVOID ExAllocatePool(POOL_TYPE PoolType, ULONG size);

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

