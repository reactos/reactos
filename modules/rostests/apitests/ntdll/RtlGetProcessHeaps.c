/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for RtlGetProcessHeaps
 * COPYRIGHT:   Copyright 2023 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlGetProcessHeaps)
{
    HANDLE HeapArray[40] = {0};
    ULONG MaxHeapArraySize = _countof(HeapArray);

    // Can call it with NULL
    ULONG InitialNumHeaps = RtlGetProcessHeaps(0, NULL);
    ok(InitialNumHeaps > 0, "Expected at least one heap\n");
    ok(InitialNumHeaps < MaxHeapArraySize, "Expected less heaps, got %lu\n", InitialNumHeaps);

    // Grab all heaps
    ULONG NumHeaps = RtlGetProcessHeaps(MaxHeapArraySize, HeapArray);
    ok_eq_ulong(NumHeaps, InitialNumHeaps);
    for (ULONG n = 0; n < min(NumHeaps, MaxHeapArraySize); ++n)
    {
        ok(HeapArray[n] != NULL, "Heap[%lu] == NULL\n", n);
    }

    PVOID HeapPtr = RtlCreateHeap(HEAP_GROWABLE, NULL, 0, 0x10000, NULL, NULL);

    memset(HeapArray, 0, sizeof(HeapArray));
    NumHeaps = RtlGetProcessHeaps(MaxHeapArraySize, HeapArray);
    ok_eq_ulong(InitialNumHeaps + 1, NumHeaps);
    if (NumHeaps > 0 && NumHeaps <= MaxHeapArraySize)
    {
        // A new heap is added at the end
        ok_ptr(HeapArray[NumHeaps - 1], HeapPtr);
    }

    RtlDestroyHeap(HeapPtr);

    // Just ask for one heap, the number of heaps available is returned, but only one heap ptr filled!
    memset(HeapArray, 0xff, sizeof(HeapArray));
    NumHeaps = RtlGetProcessHeaps(1, HeapArray);
    ok_eq_ulong(NumHeaps, InitialNumHeaps);

    ok_ptr(HeapArray[0], RtlGetProcessHeap());  // First item is the process heap
    ok_ptr(HeapArray[1], INVALID_HANDLE_VALUE); // Next item is not touched!
}
