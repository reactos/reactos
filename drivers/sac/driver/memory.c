/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/memory.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

LONG TotalFrees, TotalBytesFreed, TotalAllocations, TotalBytesAllocated;
KSPIN_LOCK MemoryLock;
PSAC_MEMORY_LIST GlobalMemoryList;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
InitializeMemoryManagement(VOID)
{
    PSAC_MEMORY_ENTRY Entry;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");

    /* Allocate a nonpaged heap for us to use */
    GlobalMemoryList = ExAllocatePoolWithTagPriority(NonPagedPool,
                                                     SAC_MEMORY_LIST_SIZE,
                                                     INITIAL_BLOCK_TAG,
                                                     HighPoolPriority);
    if (GlobalMemoryList)
    {
        /* Initialize a lock for it */
        KeInitializeSpinLock(&MemoryLock);

        /* Initialize the head of the list */
        GlobalMemoryList->Signature = GLOBAL_MEMORY_SIGNATURE;
        GlobalMemoryList->LocalDescriptor = (PSAC_MEMORY_ENTRY)(GlobalMemoryList + 1);
        GlobalMemoryList->Size = SAC_MEMORY_LIST_SIZE - sizeof(SAC_MEMORY_LIST);
        GlobalMemoryList->Next = NULL;

        /* Initialize the first free entry */
        Entry = GlobalMemoryList->LocalDescriptor;
        Entry->Signature = LOCAL_MEMORY_SIGNATURE;
        Entry->Tag = FREE_POOL_TAG;
        Entry->Size = GlobalMemoryList->Size - sizeof(SAC_MEMORY_ENTRY);

        /* All done */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with TRUE.\n");
        return TRUE;
    }

    /* No pool available to manage our heap */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with FALSE. No pool.\n");
    return FALSE;
}

VOID
NTAPI
FreeMemoryManagement(VOID)
{
    PSAC_MEMORY_LIST Next;
    KIRQL OldIrql;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");

    /* Acquire the memory lock while freeing the list(s) */
    KeAcquireSpinLock(&MemoryLock, &OldIrql);
    while (GlobalMemoryList)
    {
        ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

        /* While outside of the lock, save the next list and free this one */
        KeReleaseSpinLock(&MemoryLock, OldIrql);
        Next = GlobalMemoryList->Next;
        ExFreePoolWithTag(GlobalMemoryList, 0);

        /* Reacquire the lock and see if there was another list to free */
        KeAcquireSpinLock(&MemoryLock, &OldIrql);
        GlobalMemoryList = Next;
    }

    /* All done */
    KeReleaseSpinLock(&MemoryLock, OldIrql);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting\n");
}

PVOID
NTAPI
MyAllocatePool(IN SIZE_T PoolSize,
               IN ULONG Tag,
               IN PCHAR File,
               IN ULONG Line)
{
    PVOID p;
    p = ExAllocatePoolWithTag(NonPagedPool, PoolSize, 'HACK');
    RtlZeroMemory(p, PoolSize);
    SAC_DBG(SAC_DBG_MM, "Returning block 0x%X.\n", p);
    return p;
#if 0
    KIRQL OldIrql;
    PSAC_MEMORY_LIST GlobalDescriptor, NewDescriptor;
    PSAC_MEMORY_ENTRY LocalDescriptor, NextDescriptor;
    ULONG GlobalSize, ActualSize;
    PVOID Buffer;

    ASSERT(Tag != FREE_POOL_TAG);
    SAC_DBG(SAC_DBG_MM, "Entering.\n");

    /* Acquire the memory allocation lock and align the size request */
    KeAcquireSpinLock(&MemoryLock, &OldIrql);
    PoolSize = ALIGN_UP(PoolSize, ULONGLONG);

#if _USE_SAC_HEAP_ALLOCATOR_
    GlobalDescriptor = GlobalMemoryList;
    KeAcquireSpinLock(&MemoryLock, &OldIrql);
    while (GlobalDescriptor)
    {
        ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

        LocalDescriptor = GlobalDescriptor->LocalDescriptor;

        GlobalSize = GlobalDescriptor->Size;
        while (GlobalSize)
        {
            ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

            if ((LocalDescriptor->Tag == FREE_POOL_TAG) &&
                (LocalDescriptor->Size >= PoolSize))
            {
                break;
            }

            GlobalSize -= (LocalDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));

            LocalDescriptor =
                (PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
                LocalDescriptor->Size +
                sizeof(SAC_MEMORY_ENTRY));
        }

        GlobalDescriptor = GlobalDescriptor->Next;
    }

    if (!GlobalDescriptor)
    {
        KeReleaseSpinLock(&MemoryLock, OldIrql);

        ActualSize = min(
            PAGE_SIZE,
            PoolSize + sizeof(SAC_MEMORY_ENTRY) + sizeof(SAC_MEMORY_LIST));

        SAC_DBG(SAC_DBG_MM, "Allocating new space.\n");

        NewDescriptor = ExAllocatePoolWithTagPriority(NonPagedPool,
                                                      ActualSize,
                                                      ALLOC_BLOCK_TAG,
                                                      HighPoolPriority);
        if (!NewDescriptor)
        {
            SAC_DBG(SAC_DBG_MM, "No more memory, returning NULL.\n");
            return NULL;
        }

        KeAcquireSpinLock(&MemoryLock, &OldIrql);

        NewDescriptor->Signature = GLOBAL_MEMORY_SIGNATURE;
        NewDescriptor->LocalDescriptor = (PSAC_MEMORY_ENTRY)(NewDescriptor + 1);
        NewDescriptor->Size = ActualSize - 16;
        NewDescriptor->Next = GlobalMemoryList;

        GlobalMemoryList = NewDescriptor;

        LocalDescriptor = NewDescriptor->LocalDescriptor;
        LocalDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
        LocalDescriptor->Tag = FREE_POOL_TAG;
        LocalDescriptor->Size =
            GlobalMemoryList->Size - sizeof(SAC_MEMORY_ENTRY);
    }

    SAC_DBG(SAC_DBG_MM, "Found a good sized block.\n");
    ASSERT(LocalDescriptor->Tag == FREE_POOL_TAG);
    ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

    if (LocalDescriptor->Size > (PoolSize + sizeof(SAC_MEMORY_ENTRY)))
    {
        NextDescriptor =
            (PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
            PoolSize +
            sizeof(SAC_MEMORY_ENTRY));
        if (NextDescriptor->Tag == FREE_POOL_TAG)
        {
            NextDescriptor->Tag = FREE_POOL_TAG;
            NextDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
            NextDescriptor->Size =
                (LocalDescriptor->Size - PoolSize - sizeof(SAC_MEMORY_ENTRY));

            LocalDescriptor->Size = PoolSize;
        }
    }
#else
    /* Shut the compiler up */
    NewDescriptor = GlobalDescriptor = NULL;
    GlobalSize = (ULONG)NewDescriptor;
    ActualSize = GlobalSize;
    NextDescriptor = (PVOID)ActualSize;
    NewDescriptor = (PVOID)NextDescriptor;

    /* Use the NT pool allocator */
    LocalDescriptor = ExAllocatePoolWithTag(NonPagedPool,
                                            PoolSize + sizeof(*LocalDescriptor),
                                            Tag);
    LocalDescriptor->Size = PoolSize;
#endif
    /* Set the tag, and release the lock */
    LocalDescriptor->Tag = Tag;
    KeReleaseSpinLock(&MemoryLock, OldIrql);

    /* Update our performance counters */
    InterlockedIncrement(&TotalAllocations);
    InterlockedExchangeAdd(&TotalBytesAllocated, LocalDescriptor->Size);

    /* Return the buffer and zero it */
    SAC_DBG(SAC_DBG_MM, "Returning block 0x%X.\n", LocalDescriptor);
    Buffer = LocalDescriptor + 1;
    RtlZeroMemory(Buffer, PoolSize);
    return Buffer;
#endif
}

VOID
NTAPI
MyFreePool(IN PVOID *Block)
{
#if 0
    PSAC_MEMORY_ENTRY NextDescriptor;
    PSAC_MEMORY_ENTRY ThisDescriptor, FoundDescriptor;
    PSAC_MEMORY_ENTRY LocalDescriptor = (PVOID)((ULONG_PTR)(*Block) - sizeof(SAC_MEMORY_ENTRY));
    ULONG GlobalSize, LocalSize;
    PSAC_MEMORY_LIST GlobalDescriptor;
    KIRQL OldIrql;
    SAC_DBG(SAC_DBG_MM, "Entering with block 0x%X.\n", LocalDescriptor);

    /* Make sure this was a valid entry */
    ASSERT(LocalDescriptor->Size > 0);
    ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

    /* Update performance counters */
    InterlockedIncrement(&TotalFrees);
    InterlockedExchangeAdd(&TotalBytesFreed, LocalDescriptor->Size);

    /* Acquire the memory allocation lock */
    GlobalDescriptor = GlobalMemoryList;
    KeAcquireSpinLock(&MemoryLock, &OldIrql);

#if _USE_SAC_HEAP_ALLOCATOR_
    while (GlobalDescriptor)
    {
        ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

        FoundDescriptor = NULL;

        ThisDescriptor = GlobalDescriptor->LocalDescriptor;

        GlobalSize = GlobalDescriptor->Size;
        while (GlobalSize)
        {
            ASSERT(ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

            if (ThisDescriptor == LocalDescriptor) break;

            GlobalSize -= (ThisDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));

            ThisDescriptor =
                (PSAC_MEMORY_ENTRY)((ULONG_PTR)ThisDescriptor +
                ThisDescriptor->Size +
                sizeof(SAC_MEMORY_ENTRY));
        }

        if (ThisDescriptor == LocalDescriptor) break;

        GlobalDescriptor = GlobalDescriptor->Next;
    }

    if (!GlobalDescriptor)
    {
        KeReleaseSpinLock(&MemoryLock, OldIrql);
        SAC_DBG(SAC_DBG_MM, "Could not find block.\n");
        return;
    }

    ASSERT(ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

    if (LocalDescriptor->Tag == FREE_POOL_TAG)
    {
        KeReleaseSpinLock(&MemoryLock, OldIrql);
        SAC_DBG(SAC_DBG_MM, "Attempted to free something twice.\n");
        return;
    }

    LocalSize = LocalDescriptor->Size;
    LocalDescriptor->Tag = FREE_POOL_TAG;

    if (GlobalSize > (LocalSize + sizeof(SAC_MEMORY_ENTRY)))
    {
        NextDescriptor =
            (PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
            LocalSize +
            sizeof(SAC_MEMORY_ENTRY));
        if (NextDescriptor->Tag == FREE_POOL_TAG)
        {
            NextDescriptor->Tag = 0;
            NextDescriptor->Signature = 0;

            LocalDescriptor->Size +=
                (NextDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));
        }
    }

    if ((FoundDescriptor) && (FoundDescriptor->Tag == FREE_POOL_TAG))
    {
        LocalDescriptor->Signature = 0;
        LocalDescriptor->Tag = 0;

        FoundDescriptor->Size +=
            (LocalDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));
    }
#else
    /* Shut the compiler up */
    LocalSize = GlobalSize = 0;
    ThisDescriptor = (PVOID)LocalSize;
    NextDescriptor = (PVOID)GlobalSize;
    GlobalDescriptor = (PVOID)ThisDescriptor;
    FoundDescriptor = (PVOID)GlobalDescriptor;
    GlobalDescriptor = (PVOID)NextDescriptor;
    NextDescriptor = (PVOID)FoundDescriptor;

    /* Use the NT pool allocator*/
    ExFreePool(LocalDescriptor);
#endif

    /* Release the lock, delete the address, and return */
    KeReleaseSpinLock(&MemoryLock, OldIrql);
#endif
    SAC_DBG(SAC_DBG_MM, "exiting: 0x%p.\n", *Block);
    ExFreePoolWithTag(*Block, 'HACK');
    *Block = NULL;
}
