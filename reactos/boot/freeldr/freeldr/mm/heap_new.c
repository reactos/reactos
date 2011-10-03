/*
 *  FreeLoader
 *  Copyright (C) 2011 Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(HEAP);

#define DEFAULT_HEAP_SIZE (1024 * 1024)
#define TEMP_HEAP_SIZE (1024 * 1024)

PVOID FrLdrDefaultHeap;
PVOID FrLdrTempHeap;

typedef struct _HEAP_BLOCK
{
    USHORT Size;
    USHORT PreviousSize;
    ULONG Tag;
    UCHAR Data[];
} HEAP_BLOCK, *PHEAP_BLOCK;

typedef struct _HEAP
{
    ULONG MaximumSize;
    ULONG CurrentAllocBytes;
    ULONG MaxAllocBytes;
    ULONG NumAllocs;
    ULONG NumFrees;
    ULONG LargestAllocation;
    HEAP_BLOCK Blocks;
} HEAP, *PHEAP;

PVOID
HeapCreate(
    ULONG MaximumSize,
    TYPE_OF_MEMORY MemoryType)
{
    PHEAP Heap;
    PHEAP_BLOCK Block;
    ULONG Remaining;
    USHORT PreviousSize;
    TRACE("HeapCreate(MemoryType=%ld)\n", MemoryType);

    /* Allocate some memory for the heap */
    Heap = MmAllocateMemoryWithType(MaximumSize, MemoryType);
    if (!Heap)
    {
        ERR("HEAP: Failed to allocate heap of size 0x%lx, Type\n",
            MaximumSize, MemoryType);
        return NULL;
    }

    /* Initialize the heap header */
    Heap->MaximumSize = ALIGN_UP_BY(MaximumSize, MM_PAGE_SIZE);
    Heap->CurrentAllocBytes = 0;
    Heap->MaxAllocBytes = 0;
    Heap->NumAllocs = 0;
    Heap->NumFrees = 0;
    Heap->LargestAllocation = 0;

    /* Calculate what's left to process */
    Remaining = (MaximumSize - sizeof(HEAP)) / sizeof(HEAP_BLOCK);
    TRACE("Remaining = %ld\n", Remaining);

    /* Substract one for the terminating entry */
    Remaining--;

    Block = &Heap->Blocks;
    PreviousSize = 0;

    /* Create free blocks */
    while (Remaining > 1)
    {
        /* Initialize this free block */
        Block->Size = (USHORT)min(MAXUSHORT, Remaining - 1);
        Block->PreviousSize = PreviousSize;
        Block->Tag = 0;

        /* Substract current block size from remainder */
        Remaining -= (Block->Size + 1);

        /* Go to next block */
        PreviousSize = Block->Size;
        Block = Block + Block->Size + 1;

        TRACE("Remaining = %ld\n", Remaining);
    }

    /* Now finish with a terminating block */
    Block->Size = 0;
    Block->PreviousSize = PreviousSize;
    Block->Tag = 'dnE#';

    return Heap;
}

VOID
HeapDestroy(
    PVOID HeapHandle)
{
    PHEAP Heap = HeapHandle;

    /* Mark all pages free */
    MmMarkPagesInLookupTable(PageLookupTableAddress,
                             (ULONG_PTR)Heap / MM_PAGE_SIZE,
                             Heap->MaximumSize / MM_PAGE_SIZE,
                             LoaderFree);
}

VOID
HeapRelease(
    PVOID HeapHandle)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block;
    PUCHAR StartAddress, EndAddress;
    ULONG FreePages, AllFreePages = 0;
    TRACE("HeapRelease(%p)\n", HeapHandle);

    /* Loop all heap chunks */
    for (Block = &Heap->Blocks;
         Block->Size != 0;
         Block = Block + 1 + Block->Size)
    {
        /* Continue, if its not free */
        if (Block->Tag != 0) continue;

        /* Calculate page aligned start address of the free region */
        StartAddress = ALIGN_UP_POINTER_BY(Block->Data, PAGE_SIZE);

        /* Walk over adjacent free blocks */
        while (Block->Tag == 0) Block = Block + Block->Size + 1;

        /* Check if this was the last block */
        if (Block->Size == 0)
        {
            /* Align the end address up to cover the end of the heap */
            EndAddress = ALIGN_DOWN_POINTER_BY(Block->Data, PAGE_SIZE);
        }
        else
        {
            /* Align the end address down to not cover any allocations */
            EndAddress = ALIGN_DOWN_POINTER_BY(Block->Data, PAGE_SIZE);
        }

        FreePages = (EndAddress - StartAddress) / MM_PAGE_SIZE;
        AllFreePages += FreePages;

        /* Now mark the pages free */
        MmMarkPagesInLookupTable(PageLookupTableAddress,
                                 (ULONG_PTR)StartAddress / MM_PAGE_SIZE,
                                 FreePages,
                                 LoaderFree);

        /* bail out, if it was the last block */
        if (Block->Size == 0) break;
    }

    TRACE("HeapRelease() done, freed %ld pages\n", AllFreePages);
}

VOID
HeapCleanupAll(VOID)
{
    PHEAP Heap;

    Heap = FrLdrDefaultHeap;
    TRACE("Heap statistics for default heap:\n"
          "CurrentAlloc=0x%lx, MaxAlloc=0x%lx, LargestAllocation=0x%lx\n"
          "NumAllocs=%ld, NumFrees=%ld\n",
          Heap->CurrentAllocBytes, Heap->MaxAllocBytes, Heap->LargestAllocation,
          Heap->NumAllocs, Heap->NumFrees);

    /* Release fre pages */
    HeapRelease(FrLdrDefaultHeap);

    Heap = FrLdrTempHeap;
    TRACE("Heap statistics for temp heap:\n"
          "CurrentAlloc=0x%lx, MaxAlloc=0x%lx, LargestAllocation=0x%lx\n"
          "NumAllocs=%ld, NumFrees=%ld\n",
          Heap->CurrentAllocBytes, Heap->MaxAllocBytes, Heap->LargestAllocation,
          Heap->NumAllocs, Heap->NumFrees);

    /* Destroy the heap */
    HeapDestroy(FrLdrTempHeap);
}

PVOID
HeapAllocate(
    PVOID HeapHandle,
    SIZE_T ByteSize,
    ULONG Tag)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block, NextBlock;
    USHORT BlockSize, Remaining;

    /* Check if the allocation is too large */
    if ((ByteSize +  sizeof(HEAP_BLOCK)) > MAXUSHORT * sizeof(HEAP_BLOCK))
    {
        ERR("HEAP: Allocation of 0x%lx bytes too large\n", ByteSize);
        return NULL;
    }

    /* We need a proper tag */
    if (Tag == 0) Tag = 'enoN';

    /* Calculate alloc size */
    BlockSize = (USHORT)(ByteSize + sizeof(HEAP_BLOCK) - 1) / sizeof(HEAP_BLOCK);

    /* Loop all heap chunks */
    for (Block = &Heap->Blocks;
         Block->Size != 0;
         Block = Block + 1 + Block->Size)
    {
        /* Continue, if its not free */
        if (Block->Tag != 0) continue;

        /* Continue, if its too small */
        if (Block->Size < BlockSize) continue;

        /* This block is just fine, use it */
        Block->Tag = Tag;

        /* Calculate the remaining size */
        Remaining = Block->Size - BlockSize;

        /* Check if the remaining space is large enough for a new block */
        if (Remaining > 1)
        {
            /* Make the allocated block as large as neccessary */
            Block->Size = BlockSize;

            /* Get pointer to the new block */
            NextBlock = Block + 1 + BlockSize;

            /* Make it a free block */
            NextBlock->Size = Remaining - 1;
            NextBlock->PreviousSize = BlockSize;
            NextBlock->Tag = 0;

            /* Advance to the next block */
            BlockSize = NextBlock->Size;
            NextBlock = NextBlock + 1 + BlockSize;
        }
        else
        {
            /* Not enough left, use the full block */
            NextBlock = Block + 1 + BlockSize;
        }

        /* Update the next blocks back link */
        NextBlock->PreviousSize = Block->Size;

        /* Update heap usage */
        Heap->NumAllocs++;
        Heap->CurrentAllocBytes += Block->Size * sizeof(HEAP_BLOCK);
        Heap->MaxAllocBytes = max(Heap->MaxAllocBytes, Heap->CurrentAllocBytes);
        Heap->LargestAllocation = max(Heap->LargestAllocation,
                                      Block->Size * sizeof(HEAP_BLOCK));

        TRACE("HeapAllocate(%p, %ld, %.4s) -> return %p\n",
              HeapHandle, ByteSize, &Tag, Block->Data);

        /* Return pointer to the data */
        return Block->Data;
    }

    /* We found nothing */
    WARN("HEAP: nothing suitable found for 0x%lx bytes\n", ByteSize);
    return NULL;
}

VOID
HeapFree(
    PVOID HeapHandle,
    PVOID Pointer,
    ULONG Tag)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block, PrevBlock, NextBlock;
    TRACE("HeapFree(%p, %p)\n", HeapHandle, Pointer);
    ASSERT(Tag != 'dnE#');

    /* Check if the block is really inside this heap */
    if ((Pointer < (PVOID)(Heap + 1)) ||
        (Pointer > (PVOID)((PUCHAR)Heap + Heap->MaximumSize)))
    {
        ERR("Bad free of %p from heap %p\n", Pointer, Heap);
        ASSERT(FALSE);
    }

    Block = ((PHEAP_BLOCK)Pointer) - 1;

    /* Check if the tag matches */
    if (Tag && Block->Tag != Tag)
    {
        ERR("HEAP: Bad tag! Pointer = %p: block tag '%.4s', requested '%.4s'\n",
            Pointer, &Block->Tag, &Tag);
        ASSERT(FALSE);
    }

    /* Mark as free */
    Block->Tag = 0;

    /* Update heap usage */
    Heap->NumFrees++;
    Heap->CurrentAllocBytes -= Block->Size * sizeof(HEAP_BLOCK);

    /* Get pointers to the next and previous block */
    PrevBlock = Block - Block->PreviousSize - 1;
    NextBlock = Block + Block->Size + 1;

    /* Check if next block is free */
    if (NextBlock->Tag == 0)
    {
        /* Check if their combined size if small enough */
        if ((Block->Size + NextBlock->Size + 1) <= MAXUSHORT)
        {
            /* Merge next block into current */
            Block->Size += NextBlock->Size + 1;
            NextBlock = Block + Block->Size + 1;
            NextBlock->PreviousSize = Block->Size;
        }
    }

    /* Check if there is a block before and is free */
    if ((Block->PreviousSize != 0) && (PrevBlock->Tag == 0))
    {
        /* Check if their combined size if small enough */
        if ((PrevBlock->Size + Block->Size + 2) < MAXUSHORT)
        {
            /* Merge current block into previous */
            PrevBlock->Size += Block->Size + 1;
            NextBlock->PreviousSize = PrevBlock->Size;
        }
    }
}


/* Wrapper functions *********************************************************/

VOID
MmInitializeHeap(PVOID PageLookupTable)
{
    TRACE("MmInitializeHeap()\n");

    /* Create the default heap */
    FrLdrDefaultHeap = HeapCreate(DEFAULT_HEAP_SIZE, LoaderOsloaderHeap);
    ASSERT(FrLdrDefaultHeap);

    /* Create a temporary heap */
    FrLdrTempHeap = HeapCreate(TEMP_HEAP_SIZE, LoaderFirmwareTemporary);
    ASSERT(FrLdrTempHeap);

    TRACE("MmInitializeHeap() done, default heap %p, temp heap %p\n",
          FrLdrDefaultHeap, FrLdrTempHeap);
}

PVOID
MmHeapAlloc(ULONG MemorySize)
{
    return HeapAllocate(FrLdrDefaultHeap, MemorySize, 'pHmM');
}

VOID
MmHeapFree(PVOID MemoryPointer)
{
    HeapFree(FrLdrDefaultHeap, MemoryPointer, 'pHmM');
}


#undef ExAllocatePoolWithTag
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    return HeapAllocate(FrLdrDefaultHeap, NumberOfBytes, Tag);
}

PVOID
NTAPI
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    return HeapAllocate(FrLdrDefaultHeap, NumberOfBytes, 0);
}

#undef ExFreePool
VOID
NTAPI
ExFreePool(
    IN PVOID P)
{
    HeapFree(FrLdrDefaultHeap, P, 0);
}

#undef ExFreePoolWithTag
VOID
NTAPI
ExFreePoolWithTag(
  IN PVOID P,
  IN ULONG Tag)
{
    HeapFree(FrLdrDefaultHeap, P, Tag);
}

PVOID
NTAPI
RtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size)
{
    PVOID ptr;

    ptr = HeapAllocate(FrLdrDefaultHeap, Size, ' ltR');
    if (ptr && (Flags & HEAP_ZERO_MEMORY))
    {
        RtlZeroMemory(ptr, Size);
    }

    return ptr;
}

BOOLEAN
NTAPI
RtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID HeapBase)
{
    HeapFree(FrLdrDefaultHeap, HeapBase, ' ltR');
    return TRUE;
}

