/*
 * PROJECT:     ReactOS Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Low-Fragmentation Heap (LFH) front-end allocator
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

/* Useful references:
   http://illmatics.com/Understanding_the_LFH.pdf
*/

#include <rtl.h>
#include "heap.h"
#include "heap_lfh.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

static
VOID
RtlpBuildLFHBucketTable(
    _In_ PLFH_HEAP Lfh)
{
    ULONG Index;
    SIZE_T UserSize, Granularity;

    for (Index = 0; Index < LFH_BUCKET_COUNT; Index++)
    {
        if (Index < 32)
        {
            Granularity = 8;
            UserSize = (Index + 1) * Granularity;
        }
        else if (Index < 64)
        {
            Granularity = 16;
            UserSize = 256 + (Index - 31) * Granularity;
        }
        else if (Index < 96)
        {
            Granularity = 32;
            UserSize = 768 + (Index - 63) * Granularity;
        }
        else
        {
            Granularity = 64;
            UserSize = 1792 + (Index - 95) * Granularity;
        }

        Lfh->Buckets[Index].BlockSize = ALIGN_UP_BY(UserSize + sizeof(HEAP_ENTRY), HEAP_ENTRY_SIZE);
        InitializeListHead(&Lfh->Buckets[Index].SubSegmentList);
        Lfh->Buckets[Index].ActiveSubSegment = NULL;
    }

    /* Largest bucket must stay under the frontend's size maximum */
    ASSERT(Lfh->Buckets[LFH_BUCKET_COUNT - 1].BlockSize < LFH_MAX_BLOCK_SIZE);
}

static
PHEAP_BUCKET
RtlpFindLFHBucket(
    _In_ PLFH_HEAP Lfh,
    _In_ SIZE_T AllocationSize)
{
    ULONG Index;

    for (Index = 0; Index < LFH_BUCKET_COUNT; Index++)
    {
        if (Lfh->Buckets[Index].BlockSize >= AllocationSize)
            return &Lfh->Buckets[Index];
    }

    return NULL;
}

static
PHEAP_SUBSEGMENT
RtlpAllocateLFHSubSegment(
    _In_ PHEAP Heap,
    _In_ PHEAP_BUCKET Bucket)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PLFH_BLOCK_ZONE Zone;
    PHEAP_SUBSEGMENT SubSegment;
    PLFH_FREE_ENTRY FreeEntry, NextFree;
    SIZE_T ZoneSize, BlocksSize;
    UCHAR SavedFrontEndHeapType;
    ULONG Index;

    BlocksSize = Bucket->BlockSize * LFH_MIN_BLOCKS_PER_SUBSEGMENT;
    ZoneSize = ALIGN_UP_BY(sizeof(LFH_BLOCK_ZONE) + sizeof(HEAP_SUBSEGMENT) + BlocksSize, PAGE_SIZE);

    SavedFrontEndHeapType = Heap->FrontEndHeapType;
    Heap->FrontEndHeapType = 0;
    Zone = RtlAllocateHeap(Heap, HEAP_NO_SERIALIZE, ZoneSize);
    Heap->FrontEndHeapType = SavedFrontEndHeapType;

    if (!Zone)
        return NULL;

    Zone->Base = Zone;
    Zone->Size = ZoneSize;
    InsertTailList(&Lfh->BlockZones, &Zone->ListEntry);

    SubSegment = (PHEAP_SUBSEGMENT)(Zone + 1);
    SubSegment->Bucket = Bucket;
    SubSegment->BlockBase = (PUCHAR)(SubSegment + 1);
    SubSegment->BlockSize = Bucket->BlockSize;
    SubSegment->BlockCount = LFH_MIN_BLOCKS_PER_SUBSEGMENT;
    SubSegment->FreeBlockCount = LFH_MIN_BLOCKS_PER_SUBSEGMENT;

    FreeEntry = (PLFH_FREE_ENTRY)SubSegment->BlockBase;
    SubSegment->FreeList = FreeEntry;
    for (Index = 0; Index < SubSegment->BlockCount - 1; Index++)
    {
        NextFree = (PLFH_FREE_ENTRY)((PUCHAR)FreeEntry + SubSegment->BlockSize);
        FreeEntry->Next = NextFree;
        FreeEntry = NextFree;
    }
    FreeEntry->Next = NULL;

    InsertTailList(&Bucket->SubSegmentList, &SubSegment->ListEntry);
    return SubSegment;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
RtlpInitializeLFH(
    _In_ PHEAP Heap)
{
    PLFH_HEAP Lfh;

    /* Already enabled, nothing to do */
    if (Heap->FrontEndHeapType == 2)
        return STATUS_SUCCESS;

    Lfh = RtlAllocateHeap(Heap, HEAP_ZERO_MEMORY, sizeof(LFH_HEAP));
    if (!Lfh)
        return STATUS_NO_MEMORY;

    Lfh->Heap = Heap;
    InitializeListHead(&Lfh->BlockZones);
    RtlpBuildLFHBucketTable(Lfh);

    Heap->FrontEndHeap = Lfh;
    Heap->FrontEndHeapType = 2;

    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpDestroyLFH(
    _In_ PHEAP Heap)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PLIST_ENTRY Entry;
    PLFH_BLOCK_ZONE Zone;

    if (!Lfh)
        return;

    while (!IsListEmpty(&Lfh->BlockZones))
    {
        Entry = RemoveHeadList(&Lfh->BlockZones);
        Zone = CONTAINING_RECORD(Entry, LFH_BLOCK_ZONE, ListEntry);
        RtlFreeHeap(Heap, HEAP_NO_SERIALIZE, Zone->Base);
    }

    Heap->FrontEndHeap = NULL;
    Heap->FrontEndHeapType = 0;
    RtlFreeHeap(Heap, 0, Lfh);
}

BOOLEAN
NTAPI
RtlpLFHSizeFits(
    _In_ SIZE_T Size)
{
    return Size <= 3840;
}

SIZE_T
NTAPI
RtlpLFHSize(
    _In_ PHEAP Heap,
    _In_ PVOID BaseAddress)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PHEAP_ENTRY HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;
    ULONG BucketIndex = HeapEntry->PreviousSize;

    if (BucketIndex >= LFH_BUCKET_COUNT)
        return (SIZE_T)-1;

    return Lfh->Buckets[BucketIndex].BlockSize - sizeof(HEAP_ENTRY);
}

PVOID
NTAPI
RtlpLFHReAllocate(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ PVOID Ptr,
    _In_ SIZE_T Size)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PHEAP_ENTRY OldEntry = (PHEAP_ENTRY)Ptr - 1;
    ULONG OldBucketIndex = OldEntry->PreviousSize;
    SIZE_T OldUsableSize, NewAllocationSize, CopySize;
    PHEAP_BUCKET NewBucket;
    PVOID NewBaseAddress;

    if (OldBucketIndex >= LFH_BUCKET_COUNT)
    {
        DPRINT1("Corrupt LFH bucket index %lu on realloc %p\n", OldBucketIndex, Ptr);
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);
        return NULL;
    }
    OldUsableSize = Lfh->Buckets[OldBucketIndex].BlockSize - sizeof(HEAP_ENTRY);

    NewAllocationSize = ALIGN_UP_BY((Size ? Size : 1) + sizeof(HEAP_ENTRY), HEAP_ENTRY_SIZE);
    NewBucket = RtlpFindLFHBucket(Lfh, NewAllocationSize);

    if (NewBucket && ((ULONG)(NewBucket - Lfh->Buckets) == OldBucketIndex))
    {
        if ((Flags & HEAP_ZERO_MEMORY) && (Size > OldUsableSize))
            RtlZeroMemory((PUCHAR)Ptr + OldUsableSize, Size - OldUsableSize);

        return Ptr;
    }

    if (Flags & HEAP_REALLOC_IN_PLACE_ONLY)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
        return NULL;
    }

    if (NewBucket)
        NewBaseAddress = RtlpLFHAllocate(Heap, Flags, Size);
    else
        NewBaseAddress = RtlAllocateHeap(Heap, Flags, Size);

    if (!NewBaseAddress)
        return NULL;

    CopySize = (OldUsableSize < Size) ? OldUsableSize : Size;
    RtlCopyMemory(NewBaseAddress, Ptr, CopySize);

    if ((Flags & HEAP_ZERO_MEMORY) && (Size > CopySize))
        RtlZeroMemory((PUCHAR)NewBaseAddress + CopySize, Size - CopySize);

    RtlpLFHFree(Heap, Flags, Ptr);

    return NewBaseAddress;
}

PVOID
NTAPI
RtlpLFHAllocate(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ SIZE_T Size)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PHEAP_BUCKET Bucket;
    PHEAP_SUBSEGMENT SubSegment;
    PLFH_FREE_ENTRY FreeEntry;
    PHEAP_ENTRY HeapEntry;
    SIZE_T AllocationSize;
    ULONG BucketIndex;
    BOOLEAN HeapLocked = FALSE;

    AllocationSize = ALIGN_UP_BY((Size ? Size : 1) + sizeof(HEAP_ENTRY), HEAP_ENTRY_SIZE);

    Bucket = RtlpFindLFHBucket(Lfh, AllocationSize);
    if (!Bucket)
        return NULL;
    BucketIndex = (ULONG)(Bucket - Lfh->Buckets);

    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    SubSegment = Bucket->ActiveSubSegment;
    if (!SubSegment || !SubSegment->FreeBlockCount)
    {
        SubSegment = RtlpAllocateLFHSubSegment(Heap, Bucket);
        if (!SubSegment)
        {
            if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
            RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
            return NULL;
        }
        Bucket->ActiveSubSegment = SubSegment;
    }

    /* Pop a free block off the subsegment */
    FreeEntry = SubSegment->FreeList;
    SubSegment->FreeList = FreeEntry->Next;
    SubSegment->FreeBlockCount--;

    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    HeapEntry = (PHEAP_ENTRY)FreeEntry;
    HeapEntry->Size = (USHORT)(SubSegment->BlockSize >> HEAP_ENTRY_SHIFT);
    HeapEntry->Flags = HEAP_ENTRY_BUSY;
    HeapEntry->SmallTagIndex = (UCHAR)(LOBYTE(HeapEntry->Size) ^ HIBYTE(HeapEntry->Size) ^ HeapEntry->Flags);
    HeapEntry->PreviousSize = (USHORT)BucketIndex;
    HeapEntry->SegmentOffset = 0;
    HeapEntry->UnusedBytes = HEAP_ENTRY_LFH_FLAG;

    if (Flags & HEAP_ZERO_MEMORY)
        RtlZeroMemory(HeapEntry + 1, Size);
    else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
        RtlFillMemoryUlong(HeapEntry + 1, Size & ~0x3, ARENA_INUSE_FILLER);

    return HeapEntry + 1;
}

BOOLEAN
NTAPI
RtlpLFHFree(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress)
{
    PLFH_HEAP Lfh = (PLFH_HEAP)Heap->FrontEndHeap;
    PHEAP_ENTRY HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;
    PHEAP_BUCKET Bucket;
    PHEAP_SUBSEGMENT SubSegment;
    PLFH_FREE_ENTRY FreeEntry;
    ULONG BucketIndex = HeapEntry->PreviousSize;
    BOOLEAN HeapLocked = FALSE;

    if (BucketIndex >= LFH_BUCKET_COUNT)
    {
        DPRINT1("Corrupt LFH bucket index %lu for block %p\n", BucketIndex, BaseAddress);
        return FALSE;
    }
    Bucket = &Lfh->Buckets[BucketIndex];

    if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
        RtlFillMemory(BaseAddress, Bucket->BlockSize - sizeof(HEAP_ENTRY), ARENA_FREE_FILLER);

    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Walk the bucket's subsegments to find the one that owns this block */
    SubSegment = NULL;
    {
        PLIST_ENTRY Entry;
        for (Entry = Bucket->SubSegmentList.Flink;
             Entry != &Bucket->SubSegmentList;
             Entry = Entry->Flink)
        {
            PHEAP_SUBSEGMENT Candidate = CONTAINING_RECORD(Entry, HEAP_SUBSEGMENT, ListEntry);
            PUCHAR Base = Candidate->BlockBase;
            PUCHAR Limit = Base + Candidate->BlockCount * Candidate->BlockSize;

            if ((PUCHAR)HeapEntry >= Base && (PUCHAR)HeapEntry < Limit)
            {
                SubSegment = Candidate;
                break;
            }
        }
    }

    if (!SubSegment)
    {
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
        DPRINT1("LFH block %p does not belong to any subsegment\n", BaseAddress);
        return FALSE;
    }

    FreeEntry = (PLFH_FREE_ENTRY)HeapEntry;
    FreeEntry->Next = SubSegment->FreeList;
    SubSegment->FreeList = FreeEntry;
    SubSegment->FreeBlockCount++;

    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return TRUE;
}