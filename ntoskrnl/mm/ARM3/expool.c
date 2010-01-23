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

#line 15 "ARM³::EXPOOL"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag

/* GLOBALS ********************************************************************/

ULONG ExpNumberOfPagedPools;
POOL_DESCRIPTOR NonPagedPoolDescriptor;
PPOOL_DESCRIPTOR ExpPagedPoolDescriptor[16 + 1];
PPOOL_DESCRIPTOR PoolVector[2];
PVOID PoolTrackTable;
PKGUARDED_MUTEX ExpPagedPoolMutex;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
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
    while (NextEntry < LastEntry) InitializeListHead(NextEntry++);
}

VOID
NTAPI
InitializePool(IN POOL_TYPE PoolType,
               IN ULONG Threshold)
{
    PPOOL_DESCRIPTOR Descriptor;
    
    //
    // Check what kind of pool this is
    //
    if (PoolType == NonPagedPool)
    {
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
        KeInitializeGuardedMutex(ExpPagedPoolMutex);
        ExInitializePoolDescriptor(Descriptor,
                                   PagedPool,
                                   0,
                                   Threshold,
                                   ExpPagedPoolMutex);
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
    ULONG BlockSize, i;
    
    //
    // Check for paged pool
    //
    if (PoolType == PagedPool) return ExAllocatePagedPoolWithTag(PagedPool, NumberOfBytes, Tag);
    
    //
    // Some sanity checks
    //
    ASSERT(Tag != 0);
    ASSERT(Tag != ' GIB');
    ASSERT(NumberOfBytes != 0);
    
    //
    // Get the pool type and its corresponding vector for this request
    //
    PoolType = PoolType & BASE_POOL_TYPE_MASK;
    PoolDesc = PoolVector[PoolType];
    ASSERT(PoolDesc != NULL);
    
    //
    // Check if this is a big page allocation
    //
    if (NumberOfBytes > POOL_MAX_ALLOC)
    {
        //
        // Then just return the number of pages requested
        //
        return MiAllocatePoolPages(PoolType, NumberOfBytes);
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
    i = (NumberOfBytes + sizeof(POOL_HEADER) + sizeof(LIST_ENTRY) - 1) /
        sizeof(POOL_HEADER);

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
        if (!IsListEmpty(ListHead))
        {
            //
            // Acquire the pool lock now
            //
            OldIrql = ExLockPool(PoolDesc);
            
            //
            // And make sure the list still has entries
            //
            if (IsListEmpty(ListHead))
            {
                //
                // Someone raced us (and won) before we had a chance to acquire
                // the lock.
                //
                // Try again!
                //
                ExUnlockPool(PoolDesc, OldIrql);
                ListHead++;
                continue;
            }
            
            //
            // Remove a free entry from the list
            // Note that due to the way we insert free blocks into multiple lists
            // there is a guarantee that any block on this list will either be
            // of the correct size, or perhaps larger.
            //
            Entry = (PPOOL_HEADER)RemoveHeadList(ListHead) - 1;            
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
                    FragmentEntry = Entry + i;
                    FragmentEntry->BlockSize = Entry->BlockSize - i;
                    
                    //
                    // And make it point back to us
                    //
                    FragmentEntry->PreviousSize = i;
                    
                    //
                    // Now get the block that follows the new fragment and check
                    // if it's still on the same page as us (and not at the end)
                    //
                    NextEntry = FragmentEntry + FragmentEntry->BlockSize;
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
                    Entry += Entry->BlockSize;
                    Entry->PreviousSize = FragmentEntry->BlockSize;
                    
                    //
                    // And now let's go to the entry after that one and check if
                    // it's still on the same page, and not at the end
                    //
                    NextEntry = Entry + i;
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
                if (BlockSize != 1)
                {
                    //
                    // Insert the free entry into the free list for this size
                    //
                    InsertTailList(&PoolDesc->ListHeads[BlockSize - 1],
                                   (PLIST_ENTRY)FragmentEntry + 1);
                }
            }
            
            //
            // We have found an entry for this allocation, so set the pool type
            // and release the lock since we're done
            //
            Entry->PoolType = PoolType + 1;
            ExUnlockPool(PoolDesc, OldIrql);

            //
            // Return the pool allocation
            //
            Entry->PoolTag = Tag;
            return ++Entry;
        }
    } while (++ListHead != &PoolDesc->ListHeads[POOL_LISTS_PER_PAGE]);
    
    //
    // There were no free entries left, so we have to allocate a new fresh page
    //
    Entry = MiAllocatePoolPages(PoolType, PAGE_SIZE);
    Entry->Ulong1 = 0;
    Entry->BlockSize = i;
    Entry->PoolType = PoolType + 1;
    
    //
    // This page will have two entries -- one for the allocation (which we just
    // created above), and one for the remaining free bytes, which we're about
    // to create now. The free bytes are the whole page minus what was allocated
    // and then converted into units of block headers.
    //
    BlockSize = (PAGE_SIZE / sizeof(POOL_HEADER)) - i;
    FragmentEntry = Entry + i;
    FragmentEntry->Ulong1 = 0;
    FragmentEntry->BlockSize = BlockSize;
    FragmentEntry->PreviousSize = i;
    
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
        InsertTailList(&PoolDesc->ListHeads[BlockSize - 1],
                       (PLIST_ENTRY)FragmentEntry + 1);
        
        //
        // Release the pool lock
        //
        ExUnlockPool(PoolDesc, OldIrql);
    }

    //
    // And return the pool allocation
    //
    Entry->PoolTag = Tag;
    return ++Entry;
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePool(POOL_TYPE PoolType,
               SIZE_T NumberOfBytes)
{
    //
    // Use a default tag of "None"
    //
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, 'enoN');
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
    ULONG BlockSize;
    KIRQL OldIrql;
    POOL_TYPE PoolType;
    PPOOL_DESCRIPTOR PoolDesc;
    BOOLEAN Combined = FALSE;
    
    //
    // Check for paged pool
    //
    if ((P >= MmPagedPoolBase) &&
        (P <= (PVOID)((ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize)))
    {
        //
        // Use old allocator
        //
        ExFreePagedPool(P);
        return;
    }
   
    //
    // Quickly deal with big page allocations
    //
    if (PAGE_ALIGN(P) == P)
    {
        MiFreePoolPages(P);
        return;
    }
    
    //
    // Get the entry for this pool allocation
    // The pointer math here may look wrong or confusing, but it is quite right
    //
    Entry = P;
    Entry--;
    
    //
    // Get the size of the entry, and it's pool type, then load the descriptor
    // for this pool type
    //
    BlockSize = Entry->BlockSize;
    PoolType = (Entry->PoolType - 1) & BASE_POOL_TYPE_MASK;
    PoolDesc = PoolVector[PoolType];

    //
    // Get the pointer to the next entry
    //
    NextEntry = Entry + BlockSize;

    //
    // Acquire the pool lock
    //
    OldIrql = ExLockPool(PoolDesc);

    //
    // Check if the next allocation is at the end of the page
    //
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
                RemoveEntryList((PLIST_ENTRY)NextEntry + 1);
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
        NextEntry = Entry - Entry->PreviousSize;
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
                RemoveEntryList((PLIST_ENTRY)NextEntry + 1);
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
        (PAGE_ALIGN(Entry + Entry->BlockSize) == Entry + Entry->BlockSize))
    {
        //
        // In this case, release the pool lock, and free the page
        //
        ExUnlockPool(PoolDesc, OldIrql);
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
        NextEntry = Entry + BlockSize;
        
        //
        // As long as the next block isn't on a page boundary, have it point
        // back to us
        //
        if (PAGE_ALIGN(NextEntry) != NextEntry) NextEntry->PreviousSize = BlockSize;
    }
    
    //
    // Insert this new free block, and release the pool lock
    //
    InsertHeadList(&PoolDesc->ListHeads[BlockSize - 1], (PLIST_ENTRY)Entry + 1);
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
    return ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, 'enoN');
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
    //
    // Allocate the pool
    //
    UNIMPLEMENTED;
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/* EOF */
