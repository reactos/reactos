/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/expool.c
 * PURPOSE:         ARM Memory Manager Executive Pool Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag

/* GLOBALS ********************************************************************/

#define POOL_BIG_TABLE_ENTRY_FREE 0x1

typedef struct _POOL_DPC_CONTEXT
{
    PPOOL_TRACKER_TABLE PoolTrackTable;
    SIZE_T PoolTrackTableSize;
    PPOOL_TRACKER_TABLE PoolTrackTableExpansion;
    SIZE_T PoolTrackTableSizeExpansion;
} POOL_DPC_CONTEXT, *PPOOL_DPC_CONTEXT;

ULONG ExpNumberOfPagedPools;
POOL_DESCRIPTOR NonPagedPoolDescriptor;
PPOOL_DESCRIPTOR ExpPagedPoolDescriptor[16 + 1];
PPOOL_DESCRIPTOR PoolVector[2];
PKGUARDED_MUTEX ExpPagedPoolMutex;
SIZE_T PoolTrackTableSize, PoolTrackTableMask;
SIZE_T PoolBigPageTableSize, PoolBigPageTableHash;
PPOOL_TRACKER_TABLE PoolTrackTable;
PPOOL_TRACKER_BIG_PAGES PoolBigPageTable;
KSPIN_LOCK ExpTaggedPoolLock;
ULONG PoolHitTag;
BOOLEAN ExStopBadTags;
KSPIN_LOCK ExpLargePoolTableLock;
ULONG ExpPoolBigEntriesInUse;
ULONG ExpPoolFlags;
ULONG ExPoolFailures;

/* Pool block/header/list access macros */
#define POOL_ENTRY(x)       (PPOOL_HEADER)((ULONG_PTR)(x) - sizeof(POOL_HEADER))
#define POOL_FREE_BLOCK(x)  (PLIST_ENTRY)((ULONG_PTR)(x)  + sizeof(POOL_HEADER))
#define POOL_BLOCK(x, i)    (PPOOL_HEADER)((ULONG_PTR)(x) + ((i) * POOL_BLOCK_SIZE))
#define POOL_NEXT_BLOCK(x)  POOL_BLOCK((x), (x)->BlockSize)
#define POOL_PREV_BLOCK(x)  POOL_BLOCK((x), -((x)->PreviousSize))

/*
 * Pool list access debug macros, similar to Arthur's pfnlist.c work.
 * Microsoft actually implements similar checks in the Windows Server 2003 SP1
 * pool code, but only for checked builds.
 *
 * As of Vista, however, an MSDN Blog entry by a Security Team Manager indicates
 * that these checks are done even on retail builds, due to the increasing
 * number of kernel-mode attacks which depend on dangling list pointers and other
 * kinds of list-based attacks.
 *
 * For now, I will leave these checks on all the time, but later they are likely
 * to be DBG-only, at least until there are enough kernel-mode security attacks
 * against ReactOS to warrant the performance hit.
 *
 * For now, these are not made inline, so we can get good stack traces.
 */
PLIST_ENTRY
NTAPI
ExpDecodePoolLink(IN PLIST_ENTRY Link)
{
    return (PLIST_ENTRY)((ULONG_PTR)Link & ~1);
}

PLIST_ENTRY
NTAPI
ExpEncodePoolLink(IN PLIST_ENTRY Link)
{
    return (PLIST_ENTRY)((ULONG_PTR)Link | 1);
}

VOID
NTAPI
ExpCheckPoolLinks(IN PLIST_ENTRY ListHead)
{
    if ((ExpDecodePoolLink(ExpDecodePoolLink(ListHead->Flink)->Blink) != ListHead) ||
        (ExpDecodePoolLink(ExpDecodePoolLink(ListHead->Blink)->Flink) != ListHead))
    {
        KeBugCheckEx(BAD_POOL_HEADER,
                     3,
                     (ULONG_PTR)ListHead,
                     (ULONG_PTR)ExpDecodePoolLink(ExpDecodePoolLink(ListHead->Flink)->Blink),
                     (ULONG_PTR)ExpDecodePoolLink(ExpDecodePoolLink(ListHead->Blink)->Flink));
    }
}

VOID
NTAPI
ExpInitializePoolListHead(IN PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ExpEncodePoolLink(ListHead);
}

BOOLEAN
NTAPI
ExpIsPoolListEmpty(IN PLIST_ENTRY ListHead)
{
    return (ExpDecodePoolLink(ListHead->Flink) == ListHead);
}

VOID
NTAPI
ExpRemovePoolEntryList(IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink, Flink;
    Flink = ExpDecodePoolLink(Entry->Flink);
    Blink = ExpDecodePoolLink(Entry->Blink);
    Flink->Blink = ExpEncodePoolLink(Blink);
    Blink->Flink = ExpEncodePoolLink(Flink);
}

PLIST_ENTRY
NTAPI
ExpRemovePoolHeadList(IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Entry, Flink;
    Entry = ExpDecodePoolLink(ListHead->Flink);
    Flink = ExpDecodePoolLink(Entry->Flink);
    ListHead->Flink = ExpEncodePoolLink(Flink);
    Flink->Blink = ExpEncodePoolLink(ListHead);
    return Entry;
}

PLIST_ENTRY
NTAPI
ExpRemovePoolTailList(IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Entry, Blink;
    Entry = ExpDecodePoolLink(ListHead->Blink);
    Blink = ExpDecodePoolLink(Entry->Blink);
    ListHead->Blink = ExpEncodePoolLink(Blink);
    Blink->Flink = ExpEncodePoolLink(ListHead);
    return Entry;
}

VOID
NTAPI
ExpInsertPoolTailList(IN PLIST_ENTRY ListHead,
                      IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    ExpCheckPoolLinks(ListHead);
    Blink = ExpDecodePoolLink(ListHead->Blink);
    Entry->Flink = ExpEncodePoolLink(ListHead);
    Entry->Blink = ExpEncodePoolLink(Blink);
    Blink->Flink = ExpEncodePoolLink(Entry);
    ListHead->Blink = ExpEncodePoolLink(Entry);
    ExpCheckPoolLinks(ListHead);
}

VOID
NTAPI
ExpInsertPoolHeadList(IN PLIST_ENTRY ListHead,
                      IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink;
    ExpCheckPoolLinks(ListHead);
    Flink = ExpDecodePoolLink(ListHead->Flink);
    Entry->Flink = ExpEncodePoolLink(Flink);
    Entry->Blink = ExpEncodePoolLink(ListHead);
    Flink->Blink = ExpEncodePoolLink(Entry);
    ListHead->Flink = ExpEncodePoolLink(Entry);
    ExpCheckPoolLinks(ListHead);
}

VOID
NTAPI
ExpCheckPoolHeader(IN PPOOL_HEADER Entry)
{
    PPOOL_HEADER PreviousEntry, NextEntry;

    /* Is there a block before this one? */
    if (Entry->PreviousSize)
    {
        /* Get it */
        PreviousEntry = POOL_PREV_BLOCK(Entry);

        /* The two blocks must be on the same page! */
        if (PAGE_ALIGN(Entry) != PAGE_ALIGN(PreviousEntry))
        {
            /* Something is awry */
            KeBugCheckEx(BAD_POOL_HEADER,
                         6,
                         (ULONG_PTR)PreviousEntry,
                         __LINE__,
                         (ULONG_PTR)Entry);
        }

        /* This block should also indicate that it's as large as we think it is */
        if (PreviousEntry->BlockSize != Entry->PreviousSize)
        {
            /* Otherwise, someone corrupted one of the sizes */
            DPRINT1("PreviousEntry BlockSize %lu, tag %.4s. Entry PreviousSize %lu, tag %.4s\n",
                    PreviousEntry->BlockSize, (char *)&PreviousEntry->PoolTag,
                    Entry->PreviousSize, (char *)&Entry->PoolTag);
            KeBugCheckEx(BAD_POOL_HEADER,
                         5,
                         (ULONG_PTR)PreviousEntry,
                         __LINE__,
                         (ULONG_PTR)Entry);
        }
    }
    else if (PAGE_ALIGN(Entry) != Entry)
    {
        /* If there's no block before us, we are the first block, so we should be on a page boundary */
        KeBugCheckEx(BAD_POOL_HEADER,
                     7,
                     0,
                     __LINE__,
                     (ULONG_PTR)Entry);
    }

    /* This block must have a size */
    if (!Entry->BlockSize)
    {
        /* Someone must've corrupted this field */
        if (Entry->PreviousSize)
        {
            PreviousEntry = POOL_PREV_BLOCK(Entry);
            DPRINT1("PreviousEntry tag %.4s. Entry tag %.4s\n",
                    (char *)&PreviousEntry->PoolTag,
                    (char *)&Entry->PoolTag);
        }
        else
        {
            DPRINT1("Entry tag %.4s\n",
                    (char *)&Entry->PoolTag);
        }
        KeBugCheckEx(BAD_POOL_HEADER,
                     8,
                     0,
                     __LINE__,
                     (ULONG_PTR)Entry);
    }

    /* Okay, now get the next block */
    NextEntry = POOL_NEXT_BLOCK(Entry);

    /* If this is the last block, then we'll be page-aligned, otherwise, check this block */
    if (PAGE_ALIGN(NextEntry) != NextEntry)
    {
        /* The two blocks must be on the same page! */
        if (PAGE_ALIGN(Entry) != PAGE_ALIGN(NextEntry))
        {
            /* Something is messed up */
            KeBugCheckEx(BAD_POOL_HEADER,
                         9,
                         (ULONG_PTR)NextEntry,
                         __LINE__,
                         (ULONG_PTR)Entry);
        }

        /* And this block should think we are as large as we truly are */
        if (NextEntry->PreviousSize != Entry->BlockSize)
        {
            /* Otherwise, someone corrupted the field */
            DPRINT1("Entry BlockSize %lu, tag %.4s. NextEntry PreviousSize %lu, tag %.4s\n",
                    Entry->BlockSize, (char *)&Entry->PoolTag,
                    NextEntry->PreviousSize, (char *)&NextEntry->PoolTag);
            KeBugCheckEx(BAD_POOL_HEADER,
                         5,
                         (ULONG_PTR)NextEntry,
                         __LINE__,
                         (ULONG_PTR)Entry);
        }
    }
}

VOID
NTAPI
ExpCheckPoolAllocation(
    PVOID P,
    POOL_TYPE PoolType,
    ULONG Tag)
{
    PPOOL_HEADER Entry;
    ULONG i;
    KIRQL OldIrql;
    POOL_TYPE RealPoolType;

    /* Get the pool header */
    Entry = ((PPOOL_HEADER)P) - 1;

    /* Check if this is a large allocation */
    if (PAGE_ALIGN(P) == P)
    {
        /* Lock the pool table */
        KeAcquireSpinLock(&ExpLargePoolTableLock, &OldIrql);

        /* Find the pool tag */
        for (i = 0; i < PoolBigPageTableSize; i++)
        {
            /* Check if this is our allocation */
            if (PoolBigPageTable[i].Va == P)
            {
                /* Make sure the tag is ok */
                if (PoolBigPageTable[i].Key != Tag)
                {
                    KeBugCheckEx(BAD_POOL_CALLER, 0x0A, (ULONG_PTR)P, PoolBigPageTable[i].Key, Tag);
                }

                break;
            }
        }

        /* Release the lock */
        KeReleaseSpinLock(&ExpLargePoolTableLock, OldIrql);

        if (i == PoolBigPageTableSize)
        {
            /* Did not find the allocation */
            //ASSERT(FALSE);
        }

        /* Get Pool type by address */
        RealPoolType = MmDeterminePoolType(P);
    }
    else
    {
        /* Verify the tag */
        if (Entry->PoolTag != Tag)
        {
            DPRINT1("Allocation has wrong pool tag! Expected '%.4s', got '%.4s' (0x%08lx)\n",
                    &Tag, &Entry->PoolTag, Entry->PoolTag);
            KeBugCheckEx(BAD_POOL_CALLER, 0x0A, (ULONG_PTR)P, Entry->PoolTag, Tag);
        }

        /* Check the rest of the header */
        ExpCheckPoolHeader(Entry);

        /* Get Pool type from entry */
        RealPoolType = (Entry->PoolType - 1);
    }

    /* Should we check the pool type? */
    if (PoolType != -1)
    {
        /* Verify the pool type */
        if (RealPoolType != PoolType)
        {
            DPRINT1("Wrong pool type! Expected %s, got %s\n",
                    PoolType & BASE_POOL_TYPE_MASK ? "PagedPool" : "NonPagedPool",
                    (Entry->PoolType - 1) & BASE_POOL_TYPE_MASK ? "PagedPool" : "NonPagedPool");
            KeBugCheckEx(BAD_POOL_CALLER, 0xCC, (ULONG_PTR)P, Entry->PoolTag, Tag);
        }
    }
}

VOID
NTAPI
ExpCheckPoolBlocks(IN PVOID Block)
{
    BOOLEAN FoundBlock = FALSE;
    SIZE_T Size = 0;
    PPOOL_HEADER Entry;

    /* Get the first entry for this page, make sure it really is the first */
    Entry = PAGE_ALIGN(Block);
    ASSERT(Entry->PreviousSize == 0);

    /* Now scan each entry */
    while (TRUE)
    {
        /* When we actually found our block, remember this */
        if (Entry == Block) FoundBlock = TRUE;

        /* Now validate this block header */
        ExpCheckPoolHeader(Entry);

        /* And go to the next one, keeping track of our size */
        Size += Entry->BlockSize;
        Entry = POOL_NEXT_BLOCK(Entry);

        /* If we hit the last block, stop */
        if (Size >= (PAGE_SIZE / POOL_BLOCK_SIZE)) break;

        /* If we hit the end of the page, stop */
        if (PAGE_ALIGN(Entry) == Entry) break;
    }

    /* We must've found our block, and we must have hit the end of the page */
    if ((PAGE_ALIGN(Entry) != Entry) || !(FoundBlock))
    {
        /* Otherwise, the blocks are messed up */
        KeBugCheckEx(BAD_POOL_HEADER, 10, (ULONG_PTR)Block, __LINE__, (ULONG_PTR)Entry);
    }
}

FORCEINLINE
VOID
ExpCheckPoolIrqlLevel(IN POOL_TYPE PoolType,
                      IN SIZE_T NumberOfBytes,
                      IN PVOID Entry)
{
    //
    // Validate IRQL: It must be APC_LEVEL or lower for Paged Pool, and it must
    // be DISPATCH_LEVEL or lower for Non Paged Pool
    //
    if (((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) ?
        (KeGetCurrentIrql() > APC_LEVEL) :
        (KeGetCurrentIrql() > DISPATCH_LEVEL))
    {
        //
        // Take the system down
        //
        KeBugCheckEx(BAD_POOL_CALLER,
                     !Entry ? POOL_ALLOC_IRQL_INVALID : POOL_FREE_IRQL_INVALID,
                     KeGetCurrentIrql(),
                     PoolType,
                     !Entry ? NumberOfBytes : (ULONG_PTR)Entry);
    }
}

FORCEINLINE
ULONG
ExpComputeHashForTag(IN ULONG Tag,
                     IN SIZE_T BucketMask)
{
    //
    // Compute the hash by multiplying with a large prime number and then XORing
    // with the HIDWORD of the result.
    //
    // Finally, AND with the bucket mask to generate a valid index/bucket into
    // the table
    //
    ULONGLONG Result = (ULONGLONG)40543 * Tag;
    return (ULONG)BucketMask & ((ULONG)Result ^ (Result >> 32));
}

FORCEINLINE
ULONG
ExpComputePartialHashForAddress(IN PVOID BaseAddress)
{
    ULONG Result;
    //
    // Compute the hash by converting the address into a page number, and then
    // XORing each nibble with the next one.
    //
    // We do *NOT* AND with the bucket mask at this point because big table expansion
    // might happen. Therefore, the final step of the hash must be performed
    // while holding the expansion pushlock, and this is why we call this a
    // "partial" hash only.
    //
    Result = (ULONG)((ULONG_PTR)BaseAddress >> PAGE_SHIFT);
    return (Result >> 24) ^ (Result >> 16) ^ (Result >> 8) ^ Result;
}

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
INIT_SECTION
ExpSeedHotTags(VOID)
{
    ULONG i, Key, Hash, Index;
    PPOOL_TRACKER_TABLE TrackTable = PoolTrackTable;
    ULONG TagList[] =
    {
        '  oI',
        ' laH',
        'PldM',
        'LooP',
        'tSbO',
        ' prI',
        'bdDN',
        'LprI',
        'pOoI',
        ' ldM',
        'eliF',
        'aVMC',
        'dSeS',
        'CFtN',
        'looP',
        'rPCT',
        'bNMC',
        'dTeS',
        'sFtN',
        'TPCT',
        'CPCT',
        ' yeK',
        'qSbO',
        'mNoI',
        'aEoI',
        'cPCT',
        'aFtN',
        '0ftN',
        'tceS',
        'SprI',
        'ekoT',
        '  eS',
        'lCbO',
        'cScC',
        'lFtN',
        'cAeS',
        'mfSF',
        'kWcC',
        'miSF',
        'CdfA',
        'EdfA',
        'orSF',
        'nftN',
        'PRIU',
        'rFpN',
        'RFpN',
        'aPeS',
        'sUeS',
        'FpcA',
        'MpcA',
        'cSeS',
        'mNbO',
        'sFpN',
        'uLeS',
        'DPcS',
        'nevE',
        'vrqR',
        'ldaV',
        '  pP',
        'SdaV',
        ' daV',
        'LdaV',
        'FdaV',
        ' GIB',
    };

    //
    // Loop all 64 hot tags
    //
    ASSERT((sizeof(TagList) / sizeof(ULONG)) == 64);
    for (i = 0; i < sizeof(TagList) / sizeof(ULONG); i++)
    {
        //
        // Get the current tag, and compute its hash in the tracker table
        //
        Key = TagList[i];
        Hash = ExpComputeHashForTag(Key, PoolTrackTableMask);

        //
        // Loop all the hashes in this index/bucket
        //
        Index = Hash;
        while (TRUE)
        {
            //
            // Find an empty entry, and make sure this isn't the last hash that
            // can fit.
            //
            // On checked builds, also make sure this is the first time we are
            // seeding this tag.
            //
            ASSERT(TrackTable[Hash].Key != Key);
            if (!(TrackTable[Hash].Key) && (Hash != PoolTrackTableSize - 1))
            {
                //
                // It has been seeded, move on to the next tag
                //
                TrackTable[Hash].Key = Key;
                break;
            }

            //
            // This entry was already taken, compute the next possible hash while
            // making sure we're not back at our initial index.
            //
            ASSERT(TrackTable[Hash].Key != Key);
            Hash = (Hash + 1) & PoolTrackTableMask;
            if (Hash == Index) break;
        }
    }
}

VOID
NTAPI
ExpRemovePoolTracker(IN ULONG Key,
                     IN SIZE_T NumberOfBytes,
                     IN POOL_TYPE PoolType)
{
    ULONG Hash, Index;
    PPOOL_TRACKER_TABLE Table, TableEntry;
    SIZE_T TableMask, TableSize;

    //
    // Remove the PROTECTED_POOL flag which is not part of the tag
    //
    Key &= ~PROTECTED_POOL;

    //
    // With WinDBG you can set a tag you want to break on when an allocation is
    // attempted
    //
    if (Key == PoolHitTag) DbgBreakPoint();

    //
    // Why the double indirection? Because normally this function is also used
    // when doing session pool allocations, which has another set of tables,
    // sizes, and masks that live in session pool. Now we don't support session
    // pool so we only ever use the regular tables, but I'm keeping the code this
    // way so that the day we DO support session pool, it won't require that
    // many changes
    //
    Table = PoolTrackTable;
    TableMask = PoolTrackTableMask;
    TableSize = PoolTrackTableSize;
    DBG_UNREFERENCED_LOCAL_VARIABLE(TableSize);

    //
    // Compute the hash for this key, and loop all the possible buckets
    //
    Hash = ExpComputeHashForTag(Key, TableMask);
    Index = Hash;
    while (TRUE)
    {
        //
        // Have we found the entry for this tag? */
        //
        TableEntry = &Table[Hash];
        if (TableEntry->Key == Key)
        {
            //
            // Decrement the counters depending on if this was paged or nonpaged
            // pool
            //
            if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool)
            {
                InterlockedIncrement(&TableEntry->NonPagedFrees);
                InterlockedExchangeAddSizeT(&TableEntry->NonPagedBytes,
                                            -(SSIZE_T)NumberOfBytes);
                return;
            }
            InterlockedIncrement(&TableEntry->PagedFrees);
            InterlockedExchangeAddSizeT(&TableEntry->PagedBytes,
                                        -(SSIZE_T)NumberOfBytes);
            return;
        }

        //
        // We should have only ended up with an empty entry if we've reached
        // the last bucket
        //
        if (!TableEntry->Key)
        {
            DPRINT1("Empty item reached in tracker table. Hash=0x%lx, TableMask=0x%lx, Tag=0x%08lx, NumberOfBytes=%lu, PoolType=%d\n",
                    Hash, TableMask, Key, (ULONG)NumberOfBytes, PoolType);
            ASSERT(Hash == TableMask);
        }

        //
        // This path is hit when we don't have an entry, and the current bucket
        // is full, so we simply try the next one
        //
        Hash = (Hash + 1) & TableMask;
        if (Hash == Index) break;
    }

    //
    // And finally this path is hit when all the buckets are full, and we need
    // some expansion. This path is not yet supported in ReactOS and so we'll
    // ignore the tag
    //
    DPRINT1("Out of pool tag space, ignoring...\n");
}

VOID
NTAPI
ExpInsertPoolTracker(IN ULONG Key,
                     IN SIZE_T NumberOfBytes,
                     IN POOL_TYPE PoolType)
{
    ULONG Hash, Index;
    KIRQL OldIrql;
    PPOOL_TRACKER_TABLE Table, TableEntry;
    SIZE_T TableMask, TableSize;

    //
    // Remove the PROTECTED_POOL flag which is not part of the tag
    //
    Key &= ~PROTECTED_POOL;

    //
    // With WinDBG you can set a tag you want to break on when an allocation is
    // attempted
    //
    if (Key == PoolHitTag) DbgBreakPoint();

    //
    // There is also an internal flag you can set to break on malformed tags
    //
    if (ExStopBadTags) ASSERT(Key & 0xFFFFFF00);

    //
    // ASSERT on ReactOS features not yet supported
    //
    ASSERT(!(PoolType & SESSION_POOL_MASK));
    ASSERT(KeGetCurrentProcessorNumber() == 0);

    //
    // Why the double indirection? Because normally this function is also used
    // when doing session pool allocations, which has another set of tables,
    // sizes, and masks that live in session pool. Now we don't support session
    // pool so we only ever use the regular tables, but I'm keeping the code this
    // way so that the day we DO support session pool, it won't require that
    // many changes
    //
    Table = PoolTrackTable;
    TableMask = PoolTrackTableMask;
    TableSize = PoolTrackTableSize;
    DBG_UNREFERENCED_LOCAL_VARIABLE(TableSize);

    //
    // Compute the hash for this key, and loop all the possible buckets
    //
    Hash = ExpComputeHashForTag(Key, TableMask);
    Index = Hash;
    while (TRUE)
    {
        //
        // Do we already have an entry for this tag? */
        //
        TableEntry = &Table[Hash];
        if (TableEntry->Key == Key)
        {
            //
            // Increment the counters depending on if this was paged or nonpaged
            // pool
            //
            if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool)
            {
                InterlockedIncrement(&TableEntry->NonPagedAllocs);
                InterlockedExchangeAddSizeT(&TableEntry->NonPagedBytes, NumberOfBytes);
                return;
            }
            InterlockedIncrement(&TableEntry->PagedAllocs);
            InterlockedExchangeAddSizeT(&TableEntry->PagedBytes, NumberOfBytes);
            return;
        }

        //
        // We don't have an entry yet, but we've found a free bucket for it
        //
        if (!(TableEntry->Key) && (Hash != PoolTrackTableSize - 1))
        {
            //
            // We need to hold the lock while creating a new entry, since other
            // processors might be in this code path as well
            //
            ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);
            if (!PoolTrackTable[Hash].Key)
            {
                //
                // We've won the race, so now create this entry in the bucket
                //
                ASSERT(Table[Hash].Key == 0);
                PoolTrackTable[Hash].Key = Key;
                TableEntry->Key = Key;
            }
            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

            //
            // Now we force the loop to run again, and we should now end up in
            // the code path above which does the interlocked increments...
            //
            continue;
        }

        //
        // This path is hit when we don't have an entry, and the current bucket
        // is full, so we simply try the next one
        //
        Hash = (Hash + 1) & TableMask;
        if (Hash == Index) break;
    }

    //
    // And finally this path is hit when all the buckets are full, and we need
    // some expansion. This path is not yet supported in ReactOS and so we'll
    // ignore the tag
    //
    DPRINT1("Out of pool tag space, ignoring...\n");
}

VOID
NTAPI
INIT_SECTION
ExInitializePoolDescriptor(IN PPOOL_DESCRIPTOR PoolDescriptor,
                           IN POOL_TYPE PoolType,
                           IN ULONG PoolIndex,
                           IN ULONG Threshold,
                           IN PVOID PoolLock)
{
    PLIST_ENTRY NextEntry, LastEntry;

    //
    // Setup the descriptor based on the caller's request
    //
    PoolDescriptor->PoolType = PoolType;
    PoolDescriptor->PoolIndex = PoolIndex;
    PoolDescriptor->Threshold = Threshold;
    PoolDescriptor->LockAddress = PoolLock;

    //
    // Initialize accounting data
    //
    PoolDescriptor->RunningAllocs = 0;
    PoolDescriptor->RunningDeAllocs = 0;
    PoolDescriptor->TotalPages = 0;
    PoolDescriptor->TotalBytes = 0;
    PoolDescriptor->TotalBigPages = 0;

    //
    // Nothing pending for now
    //
    PoolDescriptor->PendingFrees = NULL;
    PoolDescriptor->PendingFreeDepth = 0;

    //
    // Loop all the descriptor's allocation lists and initialize them
    //
    NextEntry = PoolDescriptor->ListHeads;
    LastEntry = NextEntry + POOL_LISTS_PER_PAGE;
    while (NextEntry < LastEntry)
    {
        ExpInitializePoolListHead(NextEntry);
        NextEntry++;
    }

    //
    // Note that ReactOS does not support Session Pool Yet
    //
    ASSERT(PoolType != PagedPoolSession);
}

VOID
NTAPI
INIT_SECTION
InitializePool(IN POOL_TYPE PoolType,
               IN ULONG Threshold)
{
    PPOOL_DESCRIPTOR Descriptor;
    SIZE_T TableSize;
    ULONG i;

    //
    // Check what kind of pool this is
    //
    if (PoolType == NonPagedPool)
    {
        //
        // Compute the track table size and convert it from a power of two to an
        // actual byte size
        //
        // NOTE: On checked builds, we'll assert if the registry table size was
        // invalid, while on retail builds we'll just break out of the loop at
        // that point.
        //
        TableSize = min(PoolTrackTableSize, MmSizeOfNonPagedPoolInBytes >> 8);
        for (i = 0; i < 32; i++)
        {
            if (TableSize & 1)
            {
                ASSERT((TableSize & ~1) == 0);
                if (!(TableSize & ~1)) break;
            }
            TableSize >>= 1;
        }

        //
        // If we hit bit 32, than no size was defined in the registry, so
        // we'll use the default size of 2048 entries.
        //
        // Otherwise, use the size from the registry, as long as it's not
        // smaller than 64 entries.
        //
        if (i == 32)
        {
            PoolTrackTableSize = 2048;
        }
        else
        {
            PoolTrackTableSize = max(1 << i, 64);
        }

        //
        // Loop trying with the biggest specified size first, and cut it down
        // by a power of two each iteration in case not enough memory exist
        //
        while (TRUE)
        {
            //
            // Do not allow overflow
            //
            if ((PoolTrackTableSize + 1) > (MAXULONG_PTR / sizeof(POOL_TRACKER_TABLE)))
            {
                PoolTrackTableSize >>= 1;
                continue;
            }

            //
            // Allocate the tracker table and exit the loop if this worked
            //
            PoolTrackTable = MiAllocatePoolPages(NonPagedPool,
                                                 (PoolTrackTableSize + 1) *
                                                 sizeof(POOL_TRACKER_TABLE));
            if (PoolTrackTable) break;

            //
            // Otherwise, as long as we're not down to the last bit, keep
            // iterating
            //
            if (PoolTrackTableSize == 1)
            {
                KeBugCheckEx(MUST_SUCCEED_POOL_EMPTY,
                             TableSize,
                             0xFFFFFFFF,
                             0xFFFFFFFF,
                             0xFFFFFFFF);
            }
            PoolTrackTableSize >>= 1;
        }

        //
        // Add one entry, compute the hash, and zero the table
        //
        PoolTrackTableSize++;
        PoolTrackTableMask = PoolTrackTableSize - 2;

        RtlZeroMemory(PoolTrackTable,
                      PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE));

        //
        // Finally, add the most used tags to speed up those allocations
        //
        ExpSeedHotTags();

        //
        // We now do the exact same thing with the tracker table for big pages
        //
        TableSize = min(PoolBigPageTableSize, MmSizeOfNonPagedPoolInBytes >> 8);
        for (i = 0; i < 32; i++)
        {
            if (TableSize & 1)
            {
                ASSERT((TableSize & ~1) == 0);
                if (!(TableSize & ~1)) break;
            }
            TableSize >>= 1;
        }

        //
        // For big pages, the default tracker table is 4096 entries, while the
        // minimum is still 64
        //
        if (i == 32)
        {
            PoolBigPageTableSize = 4096;
        }
        else
        {
            PoolBigPageTableSize = max(1 << i, 64);
        }

        //
        // Again, run the exact same loop we ran earlier, but this time for the
        // big pool tracker instead
        //
        while (TRUE)
        {
            if ((PoolBigPageTableSize + 1) > (MAXULONG_PTR / sizeof(POOL_TRACKER_BIG_PAGES)))
            {
                PoolBigPageTableSize >>= 1;
                continue;
            }

            PoolBigPageTable = MiAllocatePoolPages(NonPagedPool,
                                                   PoolBigPageTableSize *
                                                   sizeof(POOL_TRACKER_BIG_PAGES));
            if (PoolBigPageTable) break;

            if (PoolBigPageTableSize == 1)
            {
                KeBugCheckEx(MUST_SUCCEED_POOL_EMPTY,
                             TableSize,
                             0xFFFFFFFF,
                             0xFFFFFFFF,
                             0xFFFFFFFF);
            }

            PoolBigPageTableSize >>= 1;
        }

        //
        // An extra entry is not needed for for the big pool tracker, so just
        // compute the hash and zero it
        //
        PoolBigPageTableHash = PoolBigPageTableSize - 1;
        RtlZeroMemory(PoolBigPageTable,
                      PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES));
        for (i = 0; i < PoolBigPageTableSize; i++) PoolBigPageTable[i].Va = (PVOID)1;

        //
        // During development, print this out so we can see what's happening
        //
        DPRINT("EXPOOL: Pool Tracker Table at: 0x%p with 0x%lx bytes\n",
                PoolTrackTable, PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE));
        DPRINT("EXPOOL: Big Pool Tracker Table at: 0x%p with 0x%lx bytes\n",
                PoolBigPageTable, PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES));

        //
        // Insert the generic tracker for all of big pool
        //
        ExpInsertPoolTracker('looP',
                             ROUND_TO_PAGES(PoolBigPageTableSize *
                                            sizeof(POOL_TRACKER_BIG_PAGES)),
                             NonPagedPool);

        //
        // No support for NUMA systems at this time
        //
        ASSERT(KeNumberNodes == 1);

        //
        // Initialize the tag spinlock
        //
        KeInitializeSpinLock(&ExpTaggedPoolLock);

        //
        // Initialize the nonpaged pool descriptor
        //
        PoolVector[NonPagedPool] = &NonPagedPoolDescriptor;
        ExInitializePoolDescriptor(PoolVector[NonPagedPool],
                                   NonPagedPool,
                                   0,
                                   Threshold,
                                   NULL);
    }
    else
    {
        //
        // No support for NUMA systems at this time
        //
        ASSERT(KeNumberNodes == 1);

        //
        // Allocate the pool descriptor
        //
        Descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(KGUARDED_MUTEX) +
                                           sizeof(POOL_DESCRIPTOR),
                                           'looP');
        if (!Descriptor)
        {
            //
            // This is really bad...
            //
            KeBugCheckEx(MUST_SUCCEED_POOL_EMPTY,
                         0,
                         -1,
                         -1,
                         -1);
        }

        //
        // Setup the vector and guarded mutex for paged pool
        //
        PoolVector[PagedPool] = Descriptor;
        ExpPagedPoolMutex = (PKGUARDED_MUTEX)(Descriptor + 1);
        ExpPagedPoolDescriptor[0] = Descriptor;
        KeInitializeGuardedMutex(ExpPagedPoolMutex);
        ExInitializePoolDescriptor(Descriptor,
                                   PagedPool,
                                   0,
                                   Threshold,
                                   ExpPagedPoolMutex);

        //
        // Insert the generic tracker for all of nonpaged pool
        //
        ExpInsertPoolTracker('looP',
                             ROUND_TO_PAGES(PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE)),
                             NonPagedPool);
    }
}

FORCEINLINE
KIRQL
ExLockPool(IN PPOOL_DESCRIPTOR Descriptor)
{
    //
    // Check if this is nonpaged pool
    //
    if ((Descriptor->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool)
    {
        //
        // Use the queued spin lock
        //
        return KeAcquireQueuedSpinLock(LockQueueNonPagedPoolLock);
    }
    else
    {
        //
        // Use the guarded mutex
        //
        KeAcquireGuardedMutex(Descriptor->LockAddress);
        return APC_LEVEL;
    }
}

FORCEINLINE
VOID
ExUnlockPool(IN PPOOL_DESCRIPTOR Descriptor,
             IN KIRQL OldIrql)
{
    //
    // Check if this is nonpaged pool
    //
    if ((Descriptor->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool)
    {
        //
        // Use the queued spin lock
        //
        KeReleaseQueuedSpinLock(LockQueueNonPagedPoolLock, OldIrql);
    }
    else
    {
        //
        // Use the guarded mutex
        //
        KeReleaseGuardedMutex(Descriptor->LockAddress);
    }
}

VOID
NTAPI
ExpGetPoolTagInfoTarget(IN PKDPC Dpc,
                        IN PVOID DeferredContext,
                        IN PVOID SystemArgument1,
                        IN PVOID SystemArgument2)
{
    PPOOL_DPC_CONTEXT Context = DeferredContext;
    UNREFERENCED_PARAMETER(Dpc);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Make sure we win the race, and if we did, copy the data atomically
    //
    if (KeSignalCallDpcSynchronize(SystemArgument2))
    {
        RtlCopyMemory(Context->PoolTrackTable,
                      PoolTrackTable,
                      Context->PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE));

        //
        // This is here because ReactOS does not yet support expansion
        //
        ASSERT(Context->PoolTrackTableSizeExpansion == 0);
    }

    //
    // Regardless of whether we won or not, we must now synchronize and then
    // decrement the barrier since this is one more processor that has completed
    // the callback.
    //
    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
}

NTSTATUS
NTAPI
ExGetPoolTagInfo(IN PSYSTEM_POOLTAG_INFORMATION SystemInformation,
                 IN ULONG SystemInformationLength,
                 IN OUT PULONG ReturnLength OPTIONAL)
{
    ULONG TableSize, CurrentLength;
    ULONG EntryCount;
    NTSTATUS Status = STATUS_SUCCESS;
    PSYSTEM_POOLTAG TagEntry;
    PPOOL_TRACKER_TABLE Buffer, TrackerEntry;
    POOL_DPC_CONTEXT Context;
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Keep track of how much data the caller's buffer must hold
    //
    CurrentLength = FIELD_OFFSET(SYSTEM_POOLTAG_INFORMATION, TagInfo);

    //
    // Initialize the caller's buffer
    //
    TagEntry = &SystemInformation->TagInfo[0];
    SystemInformation->Count = 0;

    //
    // Capture the number of entries, and the total size needed to make a copy
    // of the table
    //
    EntryCount = (ULONG)PoolTrackTableSize;
    TableSize = EntryCount * sizeof(POOL_TRACKER_TABLE);

    //
    // Allocate the "Generic DPC" temporary buffer
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, TableSize, 'ofnI');
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Do a "Generic DPC" to atomically retrieve the tag and allocation data
    //
    Context.PoolTrackTable = Buffer;
    Context.PoolTrackTableSize = PoolTrackTableSize;
    Context.PoolTrackTableExpansion = NULL;
    Context.PoolTrackTableSizeExpansion = 0;
    KeGenericCallDpc(ExpGetPoolTagInfoTarget, &Context);

    //
    // Now parse the results
    //
    for (TrackerEntry = Buffer; TrackerEntry < (Buffer + EntryCount); TrackerEntry++)
    {
        //
        // If the entry is empty, skip it
        //
        if (!TrackerEntry->Key) continue;

        //
        // Otherwise, add one more entry to the caller's buffer, and ensure that
        // enough space has been allocated in it
        //
        SystemInformation->Count++;
        CurrentLength += sizeof(*TagEntry);
        if (SystemInformationLength < CurrentLength)
        {
            //
            // The caller's buffer is too small, so set a failure code. The
            // caller will know the count, as well as how much space is needed.
            //
            // We do NOT break out of the loop, because we want to keep incrementing
            // the Count as well as CurrentLength so that the caller can know the
            // final numbers
            //
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            //
            // Small sanity check that our accounting is working correctly
            //
            ASSERT(TrackerEntry->PagedAllocs >= TrackerEntry->PagedFrees);
            ASSERT(TrackerEntry->NonPagedAllocs >= TrackerEntry->NonPagedFrees);

            //
            // Return the data into the caller's buffer
            //
            TagEntry->TagUlong = TrackerEntry->Key;
            TagEntry->PagedAllocs = TrackerEntry->PagedAllocs;
            TagEntry->PagedFrees = TrackerEntry->PagedFrees;
            TagEntry->PagedUsed = TrackerEntry->PagedBytes;
            TagEntry->NonPagedAllocs = TrackerEntry->NonPagedAllocs;
            TagEntry->NonPagedFrees = TrackerEntry->NonPagedFrees;
            TagEntry->NonPagedUsed = TrackerEntry->NonPagedBytes;
            TagEntry++;
        }
    }

    //
    // Free the "Generic DPC" temporary buffer, return the buffer length and status
    //
    ExFreePoolWithTag(Buffer, 'ofnI');
    if (ReturnLength) *ReturnLength = CurrentLength;
    return Status;
}

BOOLEAN
NTAPI
ExpAddTagForBigPages(IN PVOID Va,
                     IN ULONG Key,
                     IN ULONG NumberOfPages,
                     IN POOL_TYPE PoolType)
{
    ULONG Hash, i = 0;
    PVOID OldVa;
    KIRQL OldIrql;
    SIZE_T TableSize;
    PPOOL_TRACKER_BIG_PAGES Entry, EntryEnd, EntryStart;
    ASSERT(((ULONG_PTR)Va & POOL_BIG_TABLE_ENTRY_FREE) == 0);
    ASSERT(!(PoolType & SESSION_POOL_MASK));

    //
    // As the table is expandable, these values must only be read after acquiring
    // the lock to avoid a teared access during an expansion
    //
    Hash = ExpComputePartialHashForAddress(Va);
    KeAcquireSpinLock(&ExpLargePoolTableLock, &OldIrql);
    Hash &= PoolBigPageTableHash;
    TableSize = PoolBigPageTableSize;

    //
    // We loop from the current hash bucket to the end of the table, and then
    // rollover to hash bucket 0 and keep going from there. If we return back
    // to the beginning, then we attempt expansion at the bottom of the loop
    //
    EntryStart = Entry = &PoolBigPageTable[Hash];
    EntryEnd = &PoolBigPageTable[TableSize];
    do
    {
        //
        // Make sure that this is a free entry and attempt to atomically make the
        // entry busy now
        //
        OldVa = Entry->Va;
        if (((ULONG_PTR)OldVa & POOL_BIG_TABLE_ENTRY_FREE) &&
            (InterlockedCompareExchangePointer(&Entry->Va, Va, OldVa) == OldVa))
        {
            //
            // We now own this entry, write down the size and the pool tag
            //
            Entry->Key = Key;
            Entry->NumberOfPages = NumberOfPages;

            //
            // Add one more entry to the count, and see if we're getting within
            // 25% of the table size, at which point we'll do an expansion now
            // to avoid blocking too hard later on.
            //
            // Note that we only do this if it's also been the 16th time that we
            // keep losing the race or that we are not finding a free entry anymore,
            // which implies a massive number of concurrent big pool allocations.
            //
            InterlockedIncrementUL(&ExpPoolBigEntriesInUse);
            if ((i >= 16) && (ExpPoolBigEntriesInUse > (TableSize / 4)))
            {
                DPRINT("Should attempt expansion since we now have %lu entries\n",
                        ExpPoolBigEntriesInUse);
            }

            //
            // We have our entry, return
            //
            KeReleaseSpinLock(&ExpLargePoolTableLock, OldIrql);
            return TRUE;
        }

        //
        // We don't have our entry yet, so keep trying, making the entry list
        // circular if we reach the last entry. We'll eventually break out of
        // the loop once we've rolled over and returned back to our original
        // hash bucket
        //
        i++;
        if (++Entry >= EntryEnd) Entry = &PoolBigPageTable[0];
    } while (Entry != EntryStart);

    //
    // This means there's no free hash buckets whatsoever, so we would now have
    // to attempt expanding the table
    //
    DPRINT1("Big pool expansion needed, not implemented!\n");
    KeReleaseSpinLock(&ExpLargePoolTableLock, OldIrql);
    return FALSE;
}

ULONG
NTAPI
ExpFindAndRemoveTagBigPages(IN PVOID Va,
                            OUT PULONG_PTR BigPages,
                            IN POOL_TYPE PoolType)
{
    BOOLEAN FirstTry = TRUE;
    SIZE_T TableSize;
    KIRQL OldIrql;
    ULONG PoolTag, Hash;
    PPOOL_TRACKER_BIG_PAGES Entry;
    ASSERT(((ULONG_PTR)Va & POOL_BIG_TABLE_ENTRY_FREE) == 0);
    ASSERT(!(PoolType & SESSION_POOL_MASK));

    //
    // As the table is expandable, these values must only be read after acquiring
    // the lock to avoid a teared access during an expansion
    //
    Hash = ExpComputePartialHashForAddress(Va);
    KeAcquireSpinLock(&ExpLargePoolTableLock, &OldIrql);
    Hash &= PoolBigPageTableHash;
    TableSize = PoolBigPageTableSize;

    //
    // Loop while trying to find this big page allocation
    //
    while (PoolBigPageTable[Hash].Va != Va)
    {
        //
        // Increment the size until we go past the end of the table
        //
        if (++Hash >= TableSize)
        {
            //
            // Is this the second time we've tried?
            //
            if (!FirstTry)
            {
                //
                // This means it was never inserted into the pool table and it
                // received the special "BIG" tag -- return that and return 0
                // so that the code can ask Mm for the page count instead
                //
                KeReleaseSpinLock(&ExpLargePoolTableLock, OldIrql);
                *BigPages = 0;
                return ' GIB';
            }

            //
            // The first time this happens, reset the hash index and try again
            //
            Hash = 0;
            FirstTry = FALSE;
        }
    }

    //
    // Now capture all the information we need from the entry, since after we
    // release the lock, the data can change
    //
    Entry = &PoolBigPageTable[Hash];
    *BigPages = Entry->NumberOfPages;
    PoolTag = Entry->Key;

    //
    // Set the free bit, and decrement the number of allocations. Finally, release
    // the lock and return the tag that was located
    //
    InterlockedIncrement((PLONG)&Entry->Va);
    InterlockedDecrementUL(&ExpPoolBigEntriesInUse);
    KeReleaseSpinLock(&ExpLargePoolTableLock, OldIrql);
    return PoolTag;
}

VOID
NTAPI
ExQueryPoolUsage(OUT PULONG PagedPoolPages,
                 OUT PULONG NonPagedPoolPages,
                 OUT PULONG PagedPoolAllocs,
                 OUT PULONG PagedPoolFrees,
                 OUT PULONG PagedPoolLookasideHits,
                 OUT PULONG NonPagedPoolAllocs,
                 OUT PULONG NonPagedPoolFrees,
                 OUT PULONG NonPagedPoolLookasideHits)
{
    ULONG i;
    PPOOL_DESCRIPTOR PoolDesc;

    //
    // Assume all failures
    //
    *PagedPoolPages = 0;
    *PagedPoolAllocs = 0;
    *PagedPoolFrees = 0;

    //
    // Tally up the totals for all the apged pool
    //
    for (i = 0; i < ExpNumberOfPagedPools + 1; i++)
    {
        PoolDesc = ExpPagedPoolDescriptor[i];
        *PagedPoolPages += PoolDesc->TotalPages + PoolDesc->TotalBigPages;
        *PagedPoolAllocs += PoolDesc->RunningAllocs;
        *PagedPoolFrees += PoolDesc->RunningDeAllocs;
    }

    //
    // The first non-paged pool has a hardcoded well-known descriptor name
    //
    PoolDesc = &NonPagedPoolDescriptor;
    *NonPagedPoolPages = PoolDesc->TotalPages + PoolDesc->TotalBigPages;
    *NonPagedPoolAllocs = PoolDesc->RunningAllocs;
    *NonPagedPoolFrees = PoolDesc->RunningDeAllocs;

    //
    // If the system has more than one non-paged pool, copy the other descriptor
    // totals as well
    //
#if 0
    if (ExpNumberOfNonPagedPools > 1)
    {
        for (i = 0; i < ExpNumberOfNonPagedPools; i++)
        {
            PoolDesc = ExpNonPagedPoolDescriptor[i];
            *NonPagedPoolPages += PoolDesc->TotalPages + PoolDesc->TotalBigPages;
            *NonPagedPoolAllocs += PoolDesc->RunningAllocs;
            *NonPagedPoolFrees += PoolDesc->RunningDeAllocs;
        }
    }
#endif

    //
    // FIXME: Not yet supported
    //
    *NonPagedPoolLookasideHits += 0;
    *PagedPoolLookasideHits += 0;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePoolWithTag(IN POOL_TYPE PoolType,
                      IN SIZE_T NumberOfBytes,
                      IN ULONG Tag)
{
    PPOOL_DESCRIPTOR PoolDesc;
    PLIST_ENTRY ListHead;
    PPOOL_HEADER Entry, NextEntry, FragmentEntry;
    KIRQL OldIrql;
    USHORT BlockSize, i;
    ULONG OriginalType;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PGENERAL_LOOKASIDE LookasideList;

    //
    // Some sanity checks
    //
    ASSERT(Tag != 0);
    ASSERT(Tag != ' GIB');
    ASSERT(NumberOfBytes != 0);
    ExpCheckPoolIrqlLevel(PoolType, NumberOfBytes, NULL);

    //
    // Not supported in ReactOS
    //
    ASSERT(!(PoolType & SESSION_POOL_MASK));

    //
    // Check if verifier or special pool is enabled
    //
    if (ExpPoolFlags & (POOL_FLAG_VERIFIER | POOL_FLAG_SPECIAL_POOL))
    {
        //
        // For verifier, we should call the verification routine
        //
        if (ExpPoolFlags & POOL_FLAG_VERIFIER)
        {
            DPRINT1("Driver Verifier is not yet supported\n");
        }

        //
        // For special pool, we check if this is a suitable allocation and do
        // the special allocation if needed
        //
        if (ExpPoolFlags & POOL_FLAG_SPECIAL_POOL)
        {
            //
            // Check if this is a special pool allocation
            //
            if (MmUseSpecialPool(NumberOfBytes, Tag))
            {
                //
                // Try to allocate using special pool
                //
                Entry = MmAllocateSpecialPool(NumberOfBytes, Tag, PoolType, 2);
                if (Entry) return Entry;
            }
        }
    }

    //
    // Get the pool type and its corresponding vector for this request
    //
    OriginalType = PoolType;
    PoolType = PoolType & BASE_POOL_TYPE_MASK;
    PoolDesc = PoolVector[PoolType];
    ASSERT(PoolDesc != NULL);

    //
    // Check if this is a big page allocation
    //
    if (NumberOfBytes > POOL_MAX_ALLOC)
    {
        //
        // Allocate pages for it
        //
        Entry = MiAllocatePoolPages(OriginalType, NumberOfBytes);
        if (!Entry)
        {
            //
            // Must succeed pool is deprecated, but still supported. These allocation
            // failures must cause an immediate bugcheck
            //
            if (OriginalType & MUST_SUCCEED_POOL_MASK)
            {
                KeBugCheckEx(MUST_SUCCEED_POOL_EMPTY,
                             NumberOfBytes,
                             NonPagedPoolDescriptor.TotalPages,
                             NonPagedPoolDescriptor.TotalBigPages,
                             0);
            }

            //
            // Internal debugging
            //
            ExPoolFailures++;

            //
            // This flag requests printing failures, and can also further specify
            // breaking on failures
            //
            if (ExpPoolFlags & POOL_FLAG_DBGPRINT_ON_FAILURE)
            {
                DPRINT1("EX: ExAllocatePool (%lu, 0x%x) returning NULL\n",
                        NumberOfBytes,
                        OriginalType);
                if (ExpPoolFlags & POOL_FLAG_CRASH_ON_FAILURE) DbgBreakPoint();
            }

            //
            // Finally, this flag requests an exception, which we are more than
            // happy to raise!
            //
            if (OriginalType & POOL_RAISE_IF_ALLOCATION_FAILURE)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            return NULL;
        }

        //
        // Increment required counters
        //
        InterlockedExchangeAdd((PLONG)&PoolDesc->TotalBigPages,
                               (LONG)BYTES_TO_PAGES(NumberOfBytes));
        InterlockedExchangeAddSizeT(&PoolDesc->TotalBytes, NumberOfBytes);
        InterlockedIncrement((PLONG)&PoolDesc->RunningAllocs);

        //
        // Add a tag for the big page allocation and switch to the generic "BIG"
        // tag if we failed to do so, then insert a tracker for this alloation.
        //
        if (!ExpAddTagForBigPages(Entry,
                                  Tag,
                                  (ULONG)BYTES_TO_PAGES(NumberOfBytes),
                                  OriginalType))
        {
            Tag = ' GIB';
        }
        ExpInsertPoolTracker(Tag, ROUND_TO_PAGES(NumberOfBytes), OriginalType);
        return Entry;
    }

    //
    // Should never request 0 bytes from the pool, but since so many drivers do
    // it, we'll just assume they want 1 byte, based on NT's similar behavior
    //
    if (!NumberOfBytes) NumberOfBytes = 1;

    //
    // A pool allocation is defined by its data, a linked list to connect it to
    // the free list (if necessary), and a pool header to store accounting info.
    // Calculate this size, then convert it into a block size (units of pool
    // headers)
    //
    // Note that i cannot overflow (past POOL_LISTS_PER_PAGE) because any such
    // request would've been treated as a POOL_MAX_ALLOC earlier and resulted in
    // the direct allocation of pages.
    //
    i = (USHORT)((NumberOfBytes + sizeof(POOL_HEADER) + (POOL_BLOCK_SIZE - 1))
                 / POOL_BLOCK_SIZE);
    ASSERT(i < POOL_LISTS_PER_PAGE);

    //
    // Handle lookaside list optimization for both paged and nonpaged pool
    //
    if (i <= NUMBER_POOL_LOOKASIDE_LISTS)
    {
        //
        // Try popping it from the per-CPU lookaside list
        //
        LookasideList = (PoolType == PagedPool) ?
                         Prcb->PPPagedLookasideList[i - 1].P :
                         Prcb->PPNPagedLookasideList[i - 1].P;
        LookasideList->TotalAllocates++;
        Entry = (PPOOL_HEADER)InterlockedPopEntrySList(&LookasideList->ListHead);
        if (!Entry)
        {
            //
            // We failed, try popping it from the global list
            //
            LookasideList = (PoolType == PagedPool) ?
                             Prcb->PPPagedLookasideList[i - 1].L :
                             Prcb->PPNPagedLookasideList[i - 1].L;
            LookasideList->TotalAllocates++;
            Entry = (PPOOL_HEADER)InterlockedPopEntrySList(&LookasideList->ListHead);
        }

        //
        // If we were able to pop it, update the accounting and return the block
        //
        if (Entry)
        {
            LookasideList->AllocateHits++;

            //
            // Get the real entry, write down its pool type, and track it
            //
            Entry--;
            Entry->PoolType = OriginalType + 1;
            ExpInsertPoolTracker(Tag,
                                 Entry->BlockSize * POOL_BLOCK_SIZE,
                                 OriginalType);

            //
            // Return the pool allocation
            //
            Entry->PoolTag = Tag;
            (POOL_FREE_BLOCK(Entry))->Flink = NULL;
            (POOL_FREE_BLOCK(Entry))->Blink = NULL;
            return POOL_FREE_BLOCK(Entry);
        }
    }

    //
    // Loop in the free lists looking for a block if this size. Start with the
    // list optimized for this kind of size lookup
    //
    ListHead = &PoolDesc->ListHeads[i];
    do
    {
        //
        // Are there any free entries available on this list?
        //
        if (!ExpIsPoolListEmpty(ListHead))
        {
            //
            // Acquire the pool lock now
            //
            OldIrql = ExLockPool(PoolDesc);

            //
            // And make sure the list still has entries
            //
            if (ExpIsPoolListEmpty(ListHead))
            {
                //
                // Someone raced us (and won) before we had a chance to acquire
                // the lock.
                //
                // Try again!
                //
                ExUnlockPool(PoolDesc, OldIrql);
                continue;
            }

            //
            // Remove a free entry from the list
            // Note that due to the way we insert free blocks into multiple lists
            // there is a guarantee that any block on this list will either be
            // of the correct size, or perhaps larger.
            //
            ExpCheckPoolLinks(ListHead);
            Entry = POOL_ENTRY(ExpRemovePoolHeadList(ListHead));
            ExpCheckPoolLinks(ListHead);
            ExpCheckPoolBlocks(Entry);
            ASSERT(Entry->BlockSize >= i);
            ASSERT(Entry->PoolType == 0);

            //
            // Check if this block is larger that what we need. The block could
            // not possibly be smaller, due to the reason explained above (and
            // we would've asserted on a checked build if this was the case).
            //
            if (Entry->BlockSize != i)
            {
                //
                // Is there an entry before this one?
                //
                if (Entry->PreviousSize == 0)
                {
                    //
                    // There isn't anyone before us, so take the next block and
                    // turn it into a fragment that contains the leftover data
                    // that we don't need to satisfy the caller's request
                    //
                    FragmentEntry = POOL_BLOCK(Entry, i);
                    FragmentEntry->BlockSize = Entry->BlockSize - i;

                    //
                    // And make it point back to us
                    //
                    FragmentEntry->PreviousSize = i;

                    //
                    // Now get the block that follows the new fragment and check
                    // if it's still on the same page as us (and not at the end)
                    //
                    NextEntry = POOL_NEXT_BLOCK(FragmentEntry);
                    if (PAGE_ALIGN(NextEntry) != NextEntry)
                    {
                        //
                        // Adjust this next block to point to our newly created
                        // fragment block
                        //
                        NextEntry->PreviousSize = FragmentEntry->BlockSize;
                    }
                }
                else
                {
                    //
                    // There is a free entry before us, which we know is smaller
                    // so we'll make this entry the fragment instead
                    //
                    FragmentEntry = Entry;

                    //
                    // And then we'll remove from it the actual size required.
                    // Now the entry is a leftover free fragment
                    //
                    Entry->BlockSize -= i;

                    //
                    // Now let's go to the next entry after the fragment (which
                    // used to point to our original free entry) and make it
                    // reference the new fragment entry instead.
                    //
                    // This is the entry that will actually end up holding the
                    // allocation!
                    //
                    Entry = POOL_NEXT_BLOCK(Entry);
                    Entry->PreviousSize = FragmentEntry->BlockSize;

                    //
                    // And now let's go to the entry after that one and check if
                    // it's still on the same page, and not at the end
                    //
                    NextEntry = POOL_BLOCK(Entry, i);
                    if (PAGE_ALIGN(NextEntry) != NextEntry)
                    {
                        //
                        // Make it reference the allocation entry
                        //
                        NextEntry->PreviousSize = i;
                    }
                }

                //
                // Now our (allocation) entry is the right size
                //
                Entry->BlockSize = i;

                //
                // And the next entry is now the free fragment which contains
                // the remaining difference between how big the original entry
                // was, and the actual size the caller needs/requested.
                //
                FragmentEntry->PoolType = 0;
                BlockSize = FragmentEntry->BlockSize;

                //
                // Now check if enough free bytes remained for us to have a
                // "full" entry, which contains enough bytes for a linked list
                // and thus can be used for allocations (up to 8 bytes...)
                //
                ExpCheckPoolLinks(&PoolDesc->ListHeads[BlockSize - 1]);
                if (BlockSize != 1)
                {
                    //
                    // Insert the free entry into the free list for this size
                    //
                    ExpInsertPoolTailList(&PoolDesc->ListHeads[BlockSize - 1],
                                          POOL_FREE_BLOCK(FragmentEntry));
                    ExpCheckPoolLinks(POOL_FREE_BLOCK(FragmentEntry));
                }
            }

            //
            // We have found an entry for this allocation, so set the pool type
            // and release the lock since we're done
            //
            Entry->PoolType = OriginalType + 1;
            ExpCheckPoolBlocks(Entry);
            ExUnlockPool(PoolDesc, OldIrql);

            //
            // Increment required counters
            //
            InterlockedExchangeAddSizeT(&PoolDesc->TotalBytes, Entry->BlockSize * POOL_BLOCK_SIZE);
            InterlockedIncrement((PLONG)&PoolDesc->RunningAllocs);

            //
            // Track this allocation
            //
            ExpInsertPoolTracker(Tag,
                                 Entry->BlockSize * POOL_BLOCK_SIZE,
                                 OriginalType);

            //
            // Return the pool allocation
            //
            Entry->PoolTag = Tag;
            (POOL_FREE_BLOCK(Entry))->Flink = NULL;
            (POOL_FREE_BLOCK(Entry))->Blink = NULL;
            return POOL_FREE_BLOCK(Entry);
        }
    } while (++ListHead != &PoolDesc->ListHeads[POOL_LISTS_PER_PAGE]);

    //
    // There were no free entries left, so we have to allocate a new fresh page
    //
    Entry = MiAllocatePoolPages(OriginalType, PAGE_SIZE);
    if (!Entry)
    {
        //
        // Must succeed pool is deprecated, but still supported. These allocation
        // failures must cause an immediate bugcheck
        //
        if (OriginalType & MUST_SUCCEED_POOL_MASK)
        {
            KeBugCheckEx(MUST_SUCCEED_POOL_EMPTY,
                         PAGE_SIZE,
                         NonPagedPoolDescriptor.TotalPages,
                         NonPagedPoolDescriptor.TotalBigPages,
                         0);
        }

        //
        // Internal debugging
        //
        ExPoolFailures++;

        //
        // This flag requests printing failures, and can also further specify
        // breaking on failures
        //
        if (ExpPoolFlags & POOL_FLAG_DBGPRINT_ON_FAILURE)
        {
            DPRINT1("EX: ExAllocatePool (%lu, 0x%x) returning NULL\n",
                    NumberOfBytes,
                    OriginalType);
            if (ExpPoolFlags & POOL_FLAG_CRASH_ON_FAILURE) DbgBreakPoint();
        }

        //
        // Finally, this flag requests an exception, which we are more than
        // happy to raise!
        //
        if (OriginalType & POOL_RAISE_IF_ALLOCATION_FAILURE)
        {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // Return NULL to the caller in all other cases
        //
        return NULL;
    }

    //
    // Setup the entry data
    //
    Entry->Ulong1 = 0;
    Entry->BlockSize = i;
    Entry->PoolType = OriginalType + 1;

    //
    // This page will have two entries -- one for the allocation (which we just
    // created above), and one for the remaining free bytes, which we're about
    // to create now. The free bytes are the whole page minus what was allocated
    // and then converted into units of block headers.
    //
    BlockSize = (PAGE_SIZE / POOL_BLOCK_SIZE) - i;
    FragmentEntry = POOL_BLOCK(Entry, i);
    FragmentEntry->Ulong1 = 0;
    FragmentEntry->BlockSize = BlockSize;
    FragmentEntry->PreviousSize = i;

    //
    // Increment required counters
    //
    InterlockedIncrement((PLONG)&PoolDesc->TotalPages);
    InterlockedExchangeAddSizeT(&PoolDesc->TotalBytes, Entry->BlockSize * POOL_BLOCK_SIZE);

    //
    // Now check if enough free bytes remained for us to have a "full" entry,
    // which contains enough bytes for a linked list and thus can be used for
    // allocations (up to 8 bytes...)
    //
    if (FragmentEntry->BlockSize != 1)
    {
        //
        // Excellent -- acquire the pool lock
        //
        OldIrql = ExLockPool(PoolDesc);

        //
        // And insert the free entry into the free list for this block size
        //
        ExpCheckPoolLinks(&PoolDesc->ListHeads[BlockSize - 1]);
        ExpInsertPoolTailList(&PoolDesc->ListHeads[BlockSize - 1],
                              POOL_FREE_BLOCK(FragmentEntry));
        ExpCheckPoolLinks(POOL_FREE_BLOCK(FragmentEntry));

        //
        // Release the pool lock
        //
        ExpCheckPoolBlocks(Entry);
        ExUnlockPool(PoolDesc, OldIrql);
    }
    else
    {
        //
        // Simply do a sanity check
        //
        ExpCheckPoolBlocks(Entry);
    }

    //
    // Increment performance counters and track this allocation
    //
    InterlockedIncrement((PLONG)&PoolDesc->RunningAllocs);
    ExpInsertPoolTracker(Tag,
                         Entry->BlockSize * POOL_BLOCK_SIZE,
                         OriginalType);

    //
    // And return the pool allocation
    //
    ExpCheckPoolBlocks(Entry);
    Entry->PoolTag = Tag;
    return POOL_FREE_BLOCK(Entry);
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePool(POOL_TYPE PoolType,
               SIZE_T NumberOfBytes)
{
    ULONG Tag = TAG_NONE;
#if 0 && DBG
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    /* Use the first four letters of the driver name, or "None" if unavailable */
    LdrEntry = KeGetCurrentIrql() <= APC_LEVEL
                ? MiLookupDataTableEntry(_ReturnAddress())
                : NULL;
    if (LdrEntry)
    {
        ULONG i;
        Tag = 0;
        for (i = 0; i < min(4, LdrEntry->BaseDllName.Length / sizeof(WCHAR)); i++)
            Tag = Tag >> 8 | (LdrEntry->BaseDllName.Buffer[i] & 0xff) << 24;
        for (; i < 4; i++)
            Tag = Tag >> 8 | ' ' << 24;
    }
#endif
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
VOID
NTAPI
ExFreePoolWithTag(IN PVOID P,
                  IN ULONG TagToFree)
{
    PPOOL_HEADER Entry, NextEntry;
    USHORT BlockSize;
    KIRQL OldIrql;
    POOL_TYPE PoolType;
    PPOOL_DESCRIPTOR PoolDesc;
    ULONG Tag;
    BOOLEAN Combined = FALSE;
    PFN_NUMBER PageCount, RealPageCount;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PGENERAL_LOOKASIDE LookasideList;
    PEPROCESS Process;

    //
    // Check if any of the debug flags are enabled
    //
    if (ExpPoolFlags & (POOL_FLAG_CHECK_TIMERS |
                        POOL_FLAG_CHECK_WORKERS |
                        POOL_FLAG_CHECK_RESOURCES |
                        POOL_FLAG_VERIFIER |
                        POOL_FLAG_CHECK_DEADLOCK |
                        POOL_FLAG_SPECIAL_POOL))
    {
        //
        // Check if special pool is enabled
        //
        if (ExpPoolFlags & POOL_FLAG_SPECIAL_POOL)
        {
            //
            // Check if it was allocated from a special pool
            //
            if (MmIsSpecialPoolAddress(P))
            {
                //
                // Was deadlock verification also enabled? We can do some extra
                // checks at this point
                //
                if (ExpPoolFlags & POOL_FLAG_CHECK_DEADLOCK)
                {
                    DPRINT1("Verifier not yet supported\n");
                }

                //
                // It is, so handle it via special pool free routine
                //
                MmFreeSpecialPool(P);
                return;
            }
        }

        //
        // For non-big page allocations, we'll do a bunch of checks in here
        //
        if (PAGE_ALIGN(P) != P)
        {
            //
            // Get the entry for this pool allocation
            // The pointer math here may look wrong or confusing, but it is quite right
            //
            Entry = P;
            Entry--;

            //
            // Get the pool type
            //
            PoolType = (Entry->PoolType - 1) & BASE_POOL_TYPE_MASK;

            //
            // FIXME: Many other debugging checks go here
            //
            ExpCheckPoolIrqlLevel(PoolType, 0, P);
        }
    }

    //
    // Check if this is a big page allocation
    //
    if (PAGE_ALIGN(P) == P)
    {
        //
        // We need to find the tag for it, so first we need to find out what
        // kind of allocation this was (paged or nonpaged), then we can go
        // ahead and try finding the tag for it. Remember to get rid of the
        // PROTECTED_POOL tag if it's found.
        //
        // Note that if at insertion time, we failed to add the tag for a big
        // pool allocation, we used a special tag called 'BIG' to identify the
        // allocation, and we may get this tag back. In this scenario, we must
        // manually get the size of the allocation by actually counting through
        // the PFN database.
        //
        PoolType = MmDeterminePoolType(P);
        ExpCheckPoolIrqlLevel(PoolType, 0, P);
        Tag = ExpFindAndRemoveTagBigPages(P, &PageCount, PoolType);
        if (!Tag)
        {
            DPRINT1("We do not know the size of this allocation. This is not yet supported\n");
            ASSERT(Tag == ' GIB');
            PageCount = 1; // We are going to lie! This might screw up accounting?
        }
        else if (Tag & PROTECTED_POOL)
        {
            Tag &= ~PROTECTED_POOL;
        }

        //
        // Check block tag
        //
        if (TagToFree && TagToFree != Tag)
        {
            DPRINT1("Freeing pool - invalid tag specified: %.4s != %.4s\n", (char*)&TagToFree, (char*)&Tag);
            KeBugCheckEx(BAD_POOL_CALLER, 0x0A, (ULONG_PTR)P, Tag, TagToFree);
        }

        //
        // We have our tag and our page count, so we can go ahead and remove this
        // tracker now
        //
        ExpRemovePoolTracker(Tag, PageCount << PAGE_SHIFT, PoolType);

        //
        // Check if any of the debug flags are enabled
        //
        if (ExpPoolFlags & (POOL_FLAG_CHECK_TIMERS |
                            POOL_FLAG_CHECK_WORKERS |
                            POOL_FLAG_CHECK_RESOURCES |
                            POOL_FLAG_CHECK_DEADLOCK))
        {
            //
            // Was deadlock verification also enabled? We can do some extra
            // checks at this point
            //
            if (ExpPoolFlags & POOL_FLAG_CHECK_DEADLOCK)
            {
                DPRINT1("Verifier not yet supported\n");
            }

            //
            // FIXME: Many debugging checks go here
            //
        }

        //
        // Update counters
        //
        PoolDesc = PoolVector[PoolType];
        InterlockedIncrement((PLONG)&PoolDesc->RunningDeAllocs);
        InterlockedExchangeAddSizeT(&PoolDesc->TotalBytes,
                                    -(LONG_PTR)(PageCount << PAGE_SHIFT));

        //
        // Do the real free now and update the last counter with the big page count
        //
        RealPageCount = MiFreePoolPages(P);
        ASSERT(RealPageCount == PageCount);
        InterlockedExchangeAdd((PLONG)&PoolDesc->TotalBigPages,
                               -(LONG)RealPageCount);
        return;
    }

    //
    // Get the entry for this pool allocation
    // The pointer math here may look wrong or confusing, but it is quite right
    //
    Entry = P;
    Entry--;
    ASSERT((ULONG_PTR)Entry % POOL_BLOCK_SIZE == 0);

    //
    // Get the size of the entry, and it's pool type, then load the descriptor
    // for this pool type
    //
    BlockSize = Entry->BlockSize;
    PoolType = (Entry->PoolType - 1) & BASE_POOL_TYPE_MASK;
    PoolDesc = PoolVector[PoolType];

    //
    // Make sure that the IRQL makes sense
    //
    ExpCheckPoolIrqlLevel(PoolType, 0, P);

    //
    // Get the pool tag and get rid of the PROTECTED_POOL flag
    //
    Tag = Entry->PoolTag;
    if (Tag & PROTECTED_POOL) Tag &= ~PROTECTED_POOL;

    //
    // Check block tag
    //
    if (TagToFree && TagToFree != Tag)
    {
        DPRINT1("Freeing pool - invalid tag specified: %.4s != %.4s\n", (char*)&TagToFree, (char*)&Tag);
        KeBugCheckEx(BAD_POOL_CALLER, 0x0A, (ULONG_PTR)P, Tag, TagToFree);
    }

    //
    // Track the removal of this allocation
    //
    ExpRemovePoolTracker(Tag,
                         BlockSize * POOL_BLOCK_SIZE,
                         Entry->PoolType - 1);

    //
    // Release pool quota, if any
    //
    if ((Entry->PoolType - 1) & QUOTA_POOL_MASK)
    {
        Process = ((PVOID *)POOL_NEXT_BLOCK(Entry))[-1];
        ASSERT(Process != NULL);
        if (Process)
        {
            if (Process->Pcb.Header.Type != ProcessObject)
            {
                DPRINT1("Object %p is not a process. Type %u, pool type 0x%x, block size %u\n",
                        Process, Process->Pcb.Header.Type, Entry->PoolType, BlockSize);
                KeBugCheckEx(BAD_POOL_CALLER,
                             0x0D,
                             (ULONG_PTR)P,
                             Tag,
                             (ULONG_PTR)Process);
            }
            PsReturnPoolQuota(Process, PoolType, BlockSize * POOL_BLOCK_SIZE);
            ObDereferenceObject(Process);
        }
    }

    //
    // Is this allocation small enough to have come from a lookaside list?
    //
    if (BlockSize <= NUMBER_POOL_LOOKASIDE_LISTS)
    {
        //
        // Try pushing it into the per-CPU lookaside list
        //
        LookasideList = (PoolType == PagedPool) ?
                         Prcb->PPPagedLookasideList[BlockSize - 1].P :
                         Prcb->PPNPagedLookasideList[BlockSize - 1].P;
        LookasideList->TotalFrees++;
        if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth)
        {
            LookasideList->FreeHits++;
            InterlockedPushEntrySList(&LookasideList->ListHead, P);
            return;
        }

        //
        // We failed, try to push it into the global lookaside list
        //
        LookasideList = (PoolType == PagedPool) ?
                         Prcb->PPPagedLookasideList[BlockSize - 1].L :
                         Prcb->PPNPagedLookasideList[BlockSize - 1].L;
        LookasideList->TotalFrees++;
        if (ExQueryDepthSList(&LookasideList->ListHead) < LookasideList->Depth)
        {
            LookasideList->FreeHits++;
            InterlockedPushEntrySList(&LookasideList->ListHead, P);
            return;
        }
    }

    //
    // Get the pointer to the next entry
    //
    NextEntry = POOL_BLOCK(Entry, BlockSize);

    //
    // Update performance counters
    //
    InterlockedIncrement((PLONG)&PoolDesc->RunningDeAllocs);
    InterlockedExchangeAddSizeT(&PoolDesc->TotalBytes, -BlockSize * POOL_BLOCK_SIZE);

    //
    // Acquire the pool lock
    //
    OldIrql = ExLockPool(PoolDesc);

    //
    // Check if the next allocation is at the end of the page
    //
    ExpCheckPoolBlocks(Entry);
    if (PAGE_ALIGN(NextEntry) != NextEntry)
    {
        //
        // We may be able to combine the block if it's free
        //
        if (NextEntry->PoolType == 0)
        {
            //
            // The next block is free, so we'll do a combine
            //
            Combined = TRUE;

            //
            // Make sure there's actual data in the block -- anything smaller
            // than this means we only have the header, so there's no linked list
            // for us to remove
            //
            if ((NextEntry->BlockSize != 1))
            {
                //
                // The block is at least big enough to have a linked list, so go
                // ahead and remove it
                //
                ExpCheckPoolLinks(POOL_FREE_BLOCK(NextEntry));
                ExpRemovePoolEntryList(POOL_FREE_BLOCK(NextEntry));
                ExpCheckPoolLinks(ExpDecodePoolLink((POOL_FREE_BLOCK(NextEntry))->Flink));
                ExpCheckPoolLinks(ExpDecodePoolLink((POOL_FREE_BLOCK(NextEntry))->Blink));
            }

            //
            // Our entry is now combined with the next entry
            //
            Entry->BlockSize = Entry->BlockSize + NextEntry->BlockSize;
        }
    }

    //
    // Now check if there was a previous entry on the same page as us
    //
    if (Entry->PreviousSize)
    {
        //
        // Great, grab that entry and check if it's free
        //
        NextEntry = POOL_PREV_BLOCK(Entry);
        if (NextEntry->PoolType == 0)
        {
            //
            // It is, so we can do a combine
            //
            Combined = TRUE;

            //
            // Make sure there's actual data in the block -- anything smaller
            // than this means we only have the header so there's no linked list
            // for us to remove
            //
            if ((NextEntry->BlockSize != 1))
            {
                //
                // The block is at least big enough to have a linked list, so go
                // ahead and remove it
                //
                ExpCheckPoolLinks(POOL_FREE_BLOCK(NextEntry));
                ExpRemovePoolEntryList(POOL_FREE_BLOCK(NextEntry));
                ExpCheckPoolLinks(ExpDecodePoolLink((POOL_FREE_BLOCK(NextEntry))->Flink));
                ExpCheckPoolLinks(ExpDecodePoolLink((POOL_FREE_BLOCK(NextEntry))->Blink));
            }

            //
            // Combine our original block (which might've already been combined
            // with the next block), into the previous block
            //
            NextEntry->BlockSize = NextEntry->BlockSize + Entry->BlockSize;

            //
            // And now we'll work with the previous block instead
            //
            Entry = NextEntry;
        }
    }

    //
    // By now, it may have been possible for our combined blocks to actually
    // have made up a full page (if there were only 2-3 allocations on the
    // page, they could've all been combined).
    //
    if ((PAGE_ALIGN(Entry) == Entry) &&
        (PAGE_ALIGN(POOL_NEXT_BLOCK(Entry)) == POOL_NEXT_BLOCK(Entry)))
    {
        //
        // In this case, release the pool lock, update the performance counter,
        // and free the page
        //
        ExUnlockPool(PoolDesc, OldIrql);
        InterlockedExchangeAdd((PLONG)&PoolDesc->TotalPages, -1);
        MiFreePoolPages(Entry);
        return;
    }

    //
    // Otherwise, we now have a free block (or a combination of 2 or 3)
    //
    Entry->PoolType = 0;
    BlockSize = Entry->BlockSize;
    ASSERT(BlockSize != 1);

    //
    // Check if we actually did combine it with anyone
    //
    if (Combined)
    {
        //
        // Get the first combined block (either our original to begin with, or
        // the one after the original, depending if we combined with the previous)
        //
        NextEntry = POOL_NEXT_BLOCK(Entry);

        //
        // As long as the next block isn't on a page boundary, have it point
        // back to us
        //
        if (PAGE_ALIGN(NextEntry) != NextEntry) NextEntry->PreviousSize = BlockSize;
    }

    //
    // Insert this new free block, and release the pool lock
    //
    ExpInsertPoolHeadList(&PoolDesc->ListHeads[BlockSize - 1], POOL_FREE_BLOCK(Entry));
    ExpCheckPoolLinks(POOL_FREE_BLOCK(Entry));
    ExUnlockPool(PoolDesc, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
ExFreePool(PVOID P)
{
    //
    // Just free without checking for the tag
    //
    ExFreePoolWithTag(P, 0);
}

/*
 * @unimplemented
 */
SIZE_T
NTAPI
ExQueryPoolBlockSize(IN PVOID PoolBlock,
                     OUT PBOOLEAN QuotaCharged)
{
    //
    // Not implemented
    //
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */

PVOID
NTAPI
ExAllocatePoolWithQuota(IN POOL_TYPE PoolType,
                        IN SIZE_T NumberOfBytes)
{
    //
    // Allocate the pool
    //
    return ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, TAG_NONE);
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePoolWithTagPriority(IN POOL_TYPE PoolType,
                              IN SIZE_T NumberOfBytes,
                              IN ULONG Tag,
                              IN EX_POOL_PRIORITY Priority)
{
    //
    // Allocate the pool
    //
    UNIMPLEMENTED;
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePoolWithQuotaTag(IN POOL_TYPE PoolType,
                           IN SIZE_T NumberOfBytes,
                           IN ULONG Tag)
{
    BOOLEAN Raise = TRUE;
    PVOID Buffer;
    PPOOL_HEADER Entry;
    NTSTATUS Status;
    PEPROCESS Process = PsGetCurrentProcess();

    //
    // Check if we should fail instead of raising an exception
    //
    if (PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE)
    {
        Raise = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }

    //
    // Inject the pool quota mask
    //
    PoolType += QUOTA_POOL_MASK;

    //
    // Check if we have enough space to add the quota owner process, as long as
    // this isn't the system process, which never gets charged quota
    //
    ASSERT(NumberOfBytes != 0);
    if ((NumberOfBytes <= (PAGE_SIZE - POOL_BLOCK_SIZE - sizeof(PVOID))) &&
        (Process != PsInitialSystemProcess))
    {
        //
        // Add space for our EPROCESS pointer
        //
        NumberOfBytes += sizeof(PEPROCESS);
    }
    else
    {
        //
        // We won't be able to store the pointer, so don't use quota for this
        //
        PoolType -= QUOTA_POOL_MASK;
    }

    //
    // Allocate the pool buffer now
    //
    Buffer = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

    //
    // If the buffer is page-aligned, this is a large page allocation and we
    // won't touch it
    //
    if (PAGE_ALIGN(Buffer) != Buffer)
    {
        //
        // Also if special pool is enabled, and this was allocated from there,
        // we won't touch it either
        //
        if ((ExpPoolFlags & POOL_FLAG_SPECIAL_POOL) &&
            (MmIsSpecialPoolAddress(Buffer)))
        {
            return Buffer;
        }

        //
        // If it wasn't actually allocated with quota charges, ignore it too
        //
        if (!(PoolType & QUOTA_POOL_MASK)) return Buffer;

        //
        // If this is the system process, we don't charge quota, so ignore
        //
        if (Process == PsInitialSystemProcess) return Buffer;

        //
        // Actually go and charge quota for the process now
        //
        Entry = POOL_ENTRY(Buffer);
        Status = PsChargeProcessPoolQuota(Process,
                                          PoolType & BASE_POOL_TYPE_MASK,
                                          Entry->BlockSize * POOL_BLOCK_SIZE);
        if (!NT_SUCCESS(Status))
        {
            //
            // Quota failed, back out the allocation, clear the owner, and fail
            //
            ((PVOID *)POOL_NEXT_BLOCK(Entry))[-1] = NULL;
            ExFreePoolWithTag(Buffer, Tag);
            if (Raise) RtlRaiseStatus(Status);
            return NULL;
        }

        //
        // Quota worked, write the owner and then reference it before returning
        //
        ((PVOID *)POOL_NEXT_BLOCK(Entry))[-1] = Process;
        ObReferenceObject(Process);
    }
    else if (!(Buffer) && (Raise))
    {
        //
        // The allocation failed, raise an error if we are in raise mode
        //
        RtlRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Return the allocated buffer
    //
    return Buffer;
}

#if DBG && defined(KDBG)

BOOLEAN
ExpKdbgExtPool(
    ULONG Argc,
    PCHAR Argv[])
{
    ULONG_PTR Address = 0, Flags = 0;
    PVOID PoolPage;
    PPOOL_HEADER Entry;
    BOOLEAN ThisOne;
    PULONG Data;

    if (Argc > 1)
    {
        /* Get address */
        if (!KdbpGetHexNumber(Argv[1], &Address))
        {
            KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            return TRUE;
        }
    }

    if (Argc > 2)
    {
        /* Get address */
        if (!KdbpGetHexNumber(Argv[1], &Flags))
        {
            KdbpPrint("Invalid parameter: %s\n", Argv[0]);
            return TRUE;
        }
    }

    /* Check if we got an address */
    if (Address != 0)
    {
        /* Get the base page */
        PoolPage = PAGE_ALIGN(Address);
    }
    else
    {
        KdbpPrint("Heap is unimplemented\n");
        return TRUE;
    }

    /* No paging support! */
    if (!MmIsAddressValid(PoolPage))
    {
        KdbpPrint("Address not accessible!\n");
        return TRUE;
    }

    /* Get pool type */
    if ((Address >= (ULONG_PTR)MmPagedPoolStart) && (Address <= (ULONG_PTR)MmPagedPoolEnd))
        KdbpPrint("Allocation is from PagedPool region\n");
    else if ((Address >= (ULONG_PTR)MmNonPagedPoolStart) && (Address <= (ULONG_PTR)MmNonPagedPoolEnd))
        KdbpPrint("Allocation is from NonPagedPool region\n");
    else
    {
        KdbpPrint("Address 0x%p is not within any pool!\n", (PVOID)Address);
        return TRUE;
    }

    /* Loop all entries of that page */
    Entry = PoolPage;
    do
    {
        /* Check if the address is within that entry */
        ThisOne = ((Address >= (ULONG_PTR)Entry) &&
                   (Address < (ULONG_PTR)(Entry + Entry->BlockSize)));

        if (!(Flags & 1) || ThisOne)
        {
            /* Print the line */
            KdbpPrint("%c%p size: %4d previous size: %4d  %s  %.4s\n",
                     ThisOne ? '*' : ' ', Entry, Entry->BlockSize, Entry->PreviousSize,
                     (Flags & 0x80000000) ? "" : (Entry->PoolType ? "(Allocated)" : "(Free)     "),
                     (Flags & 0x80000000) ? "" : (PCHAR)&Entry->PoolTag);
        }

        if (Flags & 1)
        {
            Data = (PULONG)(Entry + 1);
            KdbpPrint("    %p  %08lx %08lx %08lx %08lx\n"
                     "    %p  %08lx %08lx %08lx %08lx\n",
                     &Data[0], Data[0], Data[1], Data[2], Data[3],
                     &Data[4], Data[4], Data[5], Data[6], Data[7]);
        }

        /* Go to next entry */
        Entry = POOL_BLOCK(Entry, Entry->BlockSize);
    }
    while ((Entry->BlockSize != 0) && ((ULONG_PTR)Entry < (ULONG_PTR)PoolPage + PAGE_SIZE));

    return TRUE;
}

#endif // DBG && KDBG

/* EOF */
