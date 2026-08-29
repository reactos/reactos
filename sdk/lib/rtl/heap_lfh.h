/*
 * PROJECT:     ReactOS Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Low-Fragmentation Heap (LFH) front-end allocator
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#ifndef RTL_HEAP_LFH_H
#define RTL_HEAP_LFH_H

#define LFH_BUCKET_COUNT 128
#define LFH_MAX_BLOCK_SIZE 0x1000
#define LFH_MIN_BLOCKS_PER_SUBSEGMENT 16
#define HEAP_ENTRY_LFH_FLAG 0x80

typedef struct _LFH_FREE_ENTRY
{
    struct _LFH_FREE_ENTRY *Next;
} LFH_FREE_ENTRY, *PLFH_FREE_ENTRY;

struct _HEAP_BUCKET;

typedef struct _HEAP_SUBSEGMENT
{
    LIST_ENTRY ListEntry;
    struct _HEAP_BUCKET *Bucket; // owning bucket
    PUCHAR BlockBase;
    SIZE_T BlockSize;
    ULONG BlockCount;
    ULONG FreeBlockCount;
    PLFH_FREE_ENTRY FreeList;
} HEAP_SUBSEGMENT, *PHEAP_SUBSEGMENT;

typedef struct _HEAP_BUCKET
{
    SIZE_T BlockSize; // rounded allocation size for this bucket
    LIST_ENTRY SubSegmentList;
    PHEAP_SUBSEGMENT ActiveSubSegment;
} HEAP_BUCKET, *PHEAP_BUCKET;

typedef struct _LFH_BLOCK_ZONE
{
    LIST_ENTRY ListEntry;
    PVOID Base;
    SIZE_T Size;
} LFH_BLOCK_ZONE, *PLFH_BLOCK_ZONE;

typedef struct _LFH_HEAP
{
    PHEAP Heap; // owning backend heap
    LIST_ENTRY BlockZones;
    HEAP_BUCKET Buckets[LFH_BUCKET_COUNT];
} LFH_HEAP, *PLFH_HEAP;

NTSTATUS
NTAPI
RtlpInitializeLFH(
    _In_ PHEAP Heap);

VOID
NTAPI
RtlpDestroyLFH(
    _In_ PHEAP Heap);

BOOLEAN
NTAPI
RtlpLFHSizeFits(
    _In_ SIZE_T Size);

PVOID
NTAPI
RtlpLFHAllocate(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ SIZE_T Size);

SIZE_T
NTAPI
RtlpLFHSize(
    _In_ PHEAP Heap,
    _In_ PVOID BaseAddress);

PVOID
NTAPI
RtlpLFHReAllocate(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ PVOID Ptr,
    _In_ SIZE_T Size);

BOOLEAN
NTAPI
RtlpLFHFree(
    _In_ PHEAP Heap,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress);

#endif /* RTL_HEAP_LFH_H */