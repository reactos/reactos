/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/pool.cpp
 * PURPOSE:         Pools implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"


VOID
NONPAGED_POOL::
Initialize()
{
    // Initialize one free pages list
    InitializeListHead(&FreeListHead);

    // FIXME: Add support for Must Succeed non-paged pool

    // Convert SizeInBytes to size in pages
    PFN_COUNT PoolPages = BYTES_TO_PAGES(SizeInBytes);

    // Initialize the first free entry
    PMMFREE_POOL_ENTRY FreeEntry = (PMMFREE_POOL_ENTRY)Start;
    PMMFREE_POOL_ENTRY FirstEntry = FreeEntry;

    FreeEntry->Size = PoolPages;
    FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
    FreeEntry->Owner = FreeEntry;

    // Insert it into the free list
    InsertHeadList(&FreeListHead, &FreeEntry->List);

    // Now create free entries for every single other page
    while (PoolPages-- > 1)
    {
        // Link them all back to the original entry
        FreeEntry = (PMMFREE_POOL_ENTRY)((ULONG_PTR)FreeEntry + PAGE_SIZE);
        FreeEntry->Owner = FirstEntry;
        FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
    }

    // Validate the first nonpaged pool expansion page (which is a guard page)
    PTENTRY *PointerPte = PTENTRY::AddressToPte(ExpansionStart);
    ASSERT(!PointerPte->IsValid());

    // Calculate the size of the expansion region alone
    PFN_COUNT ExpansionSize = (PFN_COUNT)BYTES_TO_PAGES(MaxInBytes - SizeInBytes);

    // Remove 1 page, since there's a guard page
    ExpansionSize -= 1;

    // Now initialize the nonpaged pool expansion PTE space
    MemoryManager->SystemPtes.Initialize(PointerPte, ExpansionSize, NonPagedPoolExpansion);

    // Create that guard page
    PointerPte += ExpansionSize;
    *PointerPte = MemoryManager->ZeroKernelPte;
}


VOID
NONPAGED_POOL::
Initialize1(ULONG Threshold)
{
    // Initialize non-paged pool spinlocks
    KeInitializeSpinLock (&TaggedPoolLock);
    KeInitializeSpinLock(&PoolLock);

    // Initialize the nonpaged pool descriptor
    MemoryManager->Pools.PoolVector[NonPagedPool] = &NonPagedPoolDescriptor;
    MemoryManager->Pools.InitializePoolDescriptor(MemoryManager->Pools.PoolVector[NonPagedPool], NonPagedPool, 0, Threshold, &PoolLock);
}
