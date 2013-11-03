/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/pool.hpp
 * PURPOSE:         Pools header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

#ifdef _M_AMD64
#define POOL_BLOCK_SIZE 16
#else
#define POOL_BLOCK_SIZE  8
#endif

#define POOL_LARGE_BLOCK_SIZE 512
#define POOL_LISTS 8

#define POOL_LISTS_TOTAL (PAGE_SIZE / POOL_LARGE_BLOCK_SIZE + POOL_LISTS + 1)

typedef struct _POOL_DESCRIPTOR
{
    POOL_TYPE PoolType;
    ULONG PoolIndex;
    ULONG RunningAllocs;
    ULONG RunningDeAllocs;
    ULONG TotalPages;
    ULONG TotalBigPages;
    ULONG Threshold;
    PVOID LockAddress;
    PVOID PendingFrees;
    LONG PendingFreeDepth;
    SIZE_T TotalBytes;
    SIZE_T Spare0;
    LIST_ENTRY ListHeads[POOL_LISTS_TOTAL];
} POOL_DESCRIPTOR, *PPOOL_DESCRIPTOR;

typedef struct _POOL_HEADER
{
    union
    {
        struct
        {
#ifdef _M_AMD64
            USHORT PreviousSize:8;
            USHORT PoolIndex:8;
            USHORT BlockSize:8;
            USHORT PoolType:8;
#else
            USHORT PreviousSize:9;
            USHORT PoolIndex:7;
            USHORT BlockSize:9;
            USHORT PoolType:7;
#endif
        };
        ULONG Ulong1;
    };
#ifdef _M_AMD64
    ULONG PoolTag;
#endif
    union
    {
#ifdef _M_AMD64
        PEPROCESS ProcessBilled;
#else
        ULONG PoolTag;
#endif
        struct
        {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

C_ASSERT(sizeof(POOL_HEADER) == POOL_BLOCK_SIZE);
C_ASSERT(POOL_BLOCK_SIZE == sizeof(LIST_ENTRY));

class POOL
{
public:
    PVOID Allocate(IN POOL_TYPE PoolType, IN ULONG NumberOfBytes, IN ULONG Tag);
    VOID Free(PVOID Ptr);

    ULONG_PTR Start;
    ULONG_PTR End;
    SIZE_T SizeInBytes;

    // Size limitations
    SIZE_T MinInBytes;
    SIZE_T MaxInBytes;
protected:
};

class NONPAGED_POOL : public POOL
{
public:
    VOID ComputeVirtualAddresses(PFN_NUMBER FreePages);
    VOID Initialize();
    VOID Initialize1(ULONG Threshold);

    ULONG_PTR ExpansionStart;
    LIST_ENTRY FreeListHead;

private:
    KSPIN_LOCK TaggedPoolLock;
    KSPIN_LOCK PoolLock;
    POOL_DESCRIPTOR NonPagedPoolDescriptor;
};

class PAGED_POOL : public POOL
{
public:
private:
};
