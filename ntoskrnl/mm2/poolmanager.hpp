/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/poolmanager.hpp
 * PURPOSE:         Pools manager class declaration
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

class POOL_MANAGER
{
public:
    // Operations
    PVOID Allocate(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes) { return AllocateWithTag(PoolType, NumberOfBytes, TAG_NONE); };
    PVOID AllocateWithQuota(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes);
    PVOID AllocateWithQuotaTag(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag);
    PVOID AllocateWithTag(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag);
    PVOID AllocateWithTagPriority(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag, IN EX_POOL_PRIORITY Priority);
    VOID Free(IN PVOID P) { FreeWithTag(P, 0); };
    VOID FreeWithTag(IN PVOID P, IN ULONG Tag);
    SIZE_T QueryBlockSize(IN PVOID PoolBlock, OUT PBOOLEAN QuotaCharged);

    VOID InitializeNonPagedPool(PFN_NUMBER FreePages);
    VOID InitializePoolDescriptor(IN PPOOL_DESCRIPTOR PoolDescriptor, IN POOL_TYPE PoolType, IN ULONG PoolIndex, IN ULONG Threshold, IN PVOID PoolLock);

    PAGED_POOL PagedPoolPtr;
    NONPAGED_POOL NonPagedPoolPtr;

    POOL_DESCRIPTOR *PoolVector[2];

protected:
};