/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/poolmanager.cpp
 * PURPOSE:         Pools manager class implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

VOID
POOL_MANAGER::
InitializeNonPagedPool(PFN_NUMBER FreePages)
{
    // Set min/max
    NonPagedPoolPtr.MinInBytes = 256 * 1024;
    NonPagedPoolPtr.MaxInBytes = 0;

    // Compute virtual addresses
    NonPagedPoolPtr.ComputeVirtualAddresses(FreePages);

    // Add up PFN database size to the max NPP size
    NonPagedPoolPtr.MaxInBytes += MemoryManager->PfnDb.GetAllocationSize() << PAGE_SHIFT;

    // Now calculate the nonpaged pool expansion VA region
    NonPagedPoolPtr.Start = ((ULONG_PTR)NonPagedPoolPtr.End - NonPagedPoolPtr.MaxInBytes);
    NonPagedPoolPtr.Start = (ULONG_PTR)PAGE_ALIGN(NonPagedPoolPtr.Start);

    DPRINT("NP Pool has been tuned to: current size %d bytes and max %d bytes\n",
        NonPagedPoolPtr.SizeInBytes, NonPagedPoolPtr.MaxInBytes);
}

VOID
POOL_MANAGER::
InitializePoolDescriptor(IN PPOOL_DESCRIPTOR PoolDescriptor, IN POOL_TYPE PoolType, IN ULONG PoolIndex, IN ULONG Threshold, IN PVOID PoolLock)
{
    PLIST_ENTRY NextEntry, LastEntry;

    // Setup the descriptor based on the caller's request
    PoolDescriptor->PoolType = PoolType;
    PoolDescriptor->PoolIndex = PoolIndex;
    PoolDescriptor->Threshold = Threshold;
    PoolDescriptor->LockAddress = PoolLock;

    // Initialize accounting data
    PoolDescriptor->RunningAllocs = 0;
    PoolDescriptor->RunningDeAllocs = 0;
    PoolDescriptor->TotalPages = 0;
    PoolDescriptor->TotalBytes = 0;
    PoolDescriptor->TotalBigPages = 0;

    // Loop all the descriptor's allocation lists and initialize them
    NextEntry = PoolDescriptor->ListHeads;
    LastEntry = NextEntry + POOL_LISTS_TOTAL;
    while (NextEntry < LastEntry)
    {
        InitializeListHead(NextEntry);
        NextEntry++;
    }
}

PVOID
POOL_MANAGER::
AllocateWithQuota(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
POOL_MANAGER::
AllocateWithQuotaTag(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
POOL_MANAGER::
AllocateWithTag(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag)
{
    UNIMPLEMENTED;
    return NULL;
}

PVOID
POOL_MANAGER::
AllocateWithTagPriority(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag, IN EX_POOL_PRIORITY Priority)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
POOL_MANAGER::
FreeWithTag(IN PVOID P, IN ULONG Tag)
{
    UNIMPLEMENTED;
}

SIZE_T
POOL_MANAGER::
QueryBlockSize(IN PVOID PoolBlock, OUT PBOOLEAN QuotaCharged)
{
    UNIMPLEMENTED;
    return FALSE;
}
