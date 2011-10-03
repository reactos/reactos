/*
 *  FreeLoader
 *  Copyright (C) 2006-2008     Aleksey Bragin  <aleksey@reactos.org>
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

//#define MM_DBG 1 // needs #define BufStats 1 in bget.c

ULONG MmMaximumHeapAlloc;

DBG_DEFAULT_CHANNEL(MEMORY);

VOID MmInitializeHeap(PVOID PageLookupTable)
{
	ULONG PagesNeeded = 0;
	ULONG HeapStart = 0;

	// Find contigious memory block for HEAP:STACK
	PagesNeeded = HEAP_PAGES + STACK_PAGES;
	HeapStart = MmFindAvailablePages(PageLookupTable, TotalPagesInLookupTable, PagesNeeded, FALSE);

	if (HeapStart == 0)
	{
		UiMessageBox("Critical error: Can't allocate heap!");
		return;
	}

	// Initialize BGET
	bpool(HeapStart << MM_PAGE_SHIFT, PagesNeeded << MM_PAGE_SHIFT);

	// Mark those pages as used
	MmMarkPagesInLookupTable(PageLookupTableAddress, HeapStart, PagesNeeded, LoaderOsloaderHeap);

	TRACE("Heap initialized, base 0x%08x, pages %d\n", (HeapStart << MM_PAGE_SHIFT), PagesNeeded);
}

PVOID MmHeapAlloc(ULONG MemorySize)
{
	PVOID Result;

	if (MemorySize > MM_PAGE_SIZE)
	{
		WARN("Consider using other functions to allocate %d bytes of memory!\n", MemorySize);
	}

	// Get the buffer from BGET pool
	Result = bget(MemorySize);

	if (Result == NULL)
	{
		ERR("Heap allocation for %d bytes failed\n", MemorySize);
	}
#ifdef MM_DBG
	{
		LONG CurAlloc, TotalFree, MaxFree, NumberOfGets, NumberOfRels;

		// Gather some stats
		bstats(&CurAlloc, &TotalFree, &MaxFree, &NumberOfGets, &NumberOfRels);
		if (CurAlloc > MmMaximumHeapAlloc) MmMaximumHeapAlloc = CurAlloc;

		TRACE("Current alloc %d, free %d, max alloc %lx, allocs %d, frees %d\n",
		    CurAlloc, TotalFree, MmMaximumHeapAlloc, NumberOfGets, NumberOfRels);
	}
#endif
	return Result;
}

VOID MmHeapFree(PVOID MemoryPointer)
{
	// Release the buffer to the pool
	brel(MemoryPointer);
}


PVOID
NTAPI
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    return MmHeapAlloc(NumberOfBytes);
}

#undef ExAllocatePoolWithTag
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    return MmHeapAlloc(NumberOfBytes);
}

#undef ExFreePool
VOID
NTAPI
ExFreePool(
    IN PVOID P)
{
    MmHeapFree(P);
}

#undef ExFreePoolWithTag
VOID
NTAPI
ExFreePoolWithTag(
  IN PVOID P,
  IN ULONG Tag)
{
    ExFreePool(P);
}

PVOID
NTAPI
RtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size)
{
    PVOID ptr;

    ptr = MmHeapAlloc(Size);
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
    MmHeapFree(HeapBase);
    return TRUE;
}
