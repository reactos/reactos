/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <mm.h>
#include "mem.h"
#include <rtl.h>
#include <debug.h>
#include <ui.h>


#ifdef DEBUG
U32			AllocationCount = 0;

VOID		VerifyHeap(VOID);
VOID		DumpMemoryAllocMap(VOID);
VOID		IncrementAllocationCount(VOID);
VOID		DecrementAllocationCount(VOID);
VOID		MemAllocTest(VOID);
#endif // DEBUG

PVOID MmAllocateMemory(U32 MemorySize)
{
	U32		PagesNeeded;
	U32		FirstFreePageFromEnd;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DbgPrint((DPRINT_MEMORY, "MmAllocateMemory() called for 0 bytes. Returning NULL.\n"));
		UiMessageBoxCritical("Memory allocation failed: MmAllocateMemory() called for 0 bytes.");
		return NULL;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed. Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	FirstFreePageFromEnd = MmFindAvailablePagesFromEnd(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded);

	if (FirstFreePageFromEnd == 0)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed. Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)(FirstFreePageFromEnd * MM_PAGE_SIZE);

#ifdef DEBUG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d. AllocCount: %d\n", MemorySize, PagesNeeded, FirstFreePageFromEnd, AllocationCount));
	DbgPrint((DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer));
	//VerifyHeap();
#endif // DEBUG

	// Now return the pointer
	return MemPointer;
}

PVOID MmAllocateMemoryAtAddress(U32 MemorySize, PVOID DesiredAddress)
{
	U32		PagesNeeded;
	U32		StartPageNumber;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DbgPrint((DPRINT_MEMORY, "MmAllocateMemoryAtAddress() called for 0 bytes. Returning NULL.\n"));
		UiMessageBoxCritical("Memory allocation failed: MmAllocateMemoryAtAddress() called for 0 bytes.");
		return NULL;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// Get the starting page number
	StartPageNumber = MmGetPageNumberFromAddress(DesiredAddress);

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed. Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	if (MmAreMemoryPagesAvailable(PageLookupTableAddress, TotalPagesInLookupTable, DesiredAddress, PagesNeeded) == FALSE)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed. Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, StartPageNumber, PagesNeeded);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)(StartPageNumber * MM_PAGE_SIZE);

#ifdef DEBUG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d. AllocCount: %d\n", MemorySize, PagesNeeded, StartPageNumber, AllocationCount));
	DbgPrint((DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer));
	//VerifyHeap();
#endif // DEBUG

	// Now return the pointer
	return MemPointer;
}

VOID MmFreeMemory(PVOID MemoryPointer)
{
	U32							PageNumber;
	U32							PageCount;
	U32							Idx;
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

#ifdef DEBUG

	// Make sure we didn't get a bogus pointer
	if (MemoryPointer >= (PVOID)(TotalPagesInLookupTable * MM_PAGE_SIZE))
	{
		BugCheck((DPRINT_MEMORY, "Bogus memory pointer (0x%x) passed to MmFreeMemory()\n", MemoryPointer));
	}
#endif // DEBUG

	// Find out the page number of the first
	// page of memory they allocated
	PageNumber = MmGetPageNumberFromAddress(MemoryPointer);
	PageCount = RealPageLookupTable[PageNumber].PageAllocationLength;

#ifdef DEBUG
	// Make sure we didn't get a bogus pointer
	if ((PageCount < 1) || (PageCount > (TotalPagesInLookupTable - PageNumber)))
	{
		BugCheck((DPRINT_MEMORY, "Invalid page count in lookup table. PageLookupTable[%d].PageAllocationLength = %d\n", PageNumber, RealPageLookupTable[PageNumber].PageAllocationLength));
	}

	// Loop through our array check all the pages
	// to make sure they are allocated with a length of 0
	for (Idx=PageNumber+1; Idx<(PageNumber + PageCount); Idx++)
	{
		if ((RealPageLookupTable[Idx].PageAllocated != 1) ||
			(RealPageLookupTable[Idx].PageAllocationLength != 0))
		{
			BugCheck((DPRINT_MEMORY, "Invalid page entry in lookup table, PageAllocated should = 1 and PageAllocationLength should = 0 because this is not the first block in the run. PageLookupTable[%d].PageAllocated = %d PageLookupTable[%d].PageAllocationLength = %d\n", PageNumber, RealPageLookupTable[PageNumber].PageAllocated, PageNumber, RealPageLookupTable[PageNumber].PageAllocationLength));
		}
	}

#endif

	// Loop through our array and mark all the
	// blocks as free
	for (Idx=PageNumber; Idx<(PageNumber + PageCount); Idx++)
	{
		RealPageLookupTable[Idx].PageAllocated = 0;
		RealPageLookupTable[Idx].PageAllocationLength = 0;
	}

#ifdef DEBUG
	DecrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Freed %d pages of memory starting at page %d. AllocationCount: %d\n", PageCount, PageNumber, AllocationCount));
	//VerifyHeap();
#endif // DEBUG
}

#ifdef DEBUG
VOID VerifyHeap(VOID)
{
	U32							Idx;
	U32							Idx2;
	U32							Count;
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

	if (DUMP_MEM_MAP_ON_VERIFY)
	{
		DumpMemoryAllocMap();
	}

	// Loop through the array and verify that
	// everything is kosher
	for (Idx=0; Idx<TotalPagesInLookupTable; Idx++)
	{
		// Check if this block is allocated
		if (RealPageLookupTable[Idx].PageAllocated != 0)
		{
			// This is the first block in the run so it
			// had better have a length that is within range
			if ((RealPageLookupTable[Idx].PageAllocationLength < 1) || (RealPageLookupTable[Idx].PageAllocationLength > (TotalPagesInLookupTable - Idx)))
			{
				BugCheck((DPRINT_MEMORY, "Allocation length out of range in heap table. PageLookupTable[Idx].PageAllocationLength = %d\n", RealPageLookupTable[Idx].PageAllocationLength));
			}

			// Now go through and verify that the rest of
			// this run has the blocks marked allocated
			// with a length of zero but don't check the
			// first one because we already did
			Count = RealPageLookupTable[Idx].PageAllocationLength;
			for (Idx2=1; Idx2<Count; Idx2++)
			{
				// Make sure it's allocated
				if (RealPageLookupTable[Idx + Idx2].PageAllocated == 0)
				{
					BugCheck((DPRINT_MEMORY, "Lookup table indicates hole in memory allocation. RealPageLookupTable[Idx + Idx2].PageAllocated == 0\n"));
				}

				// Make sure the length is zero
				if (RealPageLookupTable[Idx + Idx2].PageAllocationLength != 0)
				{
					BugCheck((DPRINT_MEMORY, "Allocation chain has non-zero value in non-first block in lookup table. RealPageLookupTable[Idx + Idx2].PageAllocationLength != 0\n"));
				}
			}

			// Move on to the next run
			Idx += (Count - 1);
		}
		else
		{
			// Nope, not allocated so make sure the length is zero
			if (RealPageLookupTable[Idx].PageAllocationLength != 0)
			{
				BugCheck((DPRINT_MEMORY, "Free block is start of memory allocation. RealPageLookupTable[Idx].PageAllocationLength != 0\n"));
			}
		}
	}
}

VOID DumpMemoryAllocMap(VOID)
{
	U32							Idx;
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

	DbgPrint((DPRINT_MEMORY, "----------- Memory Allocation Bitmap -----------\n"));

	for (Idx=0; Idx<TotalPagesInLookupTable; Idx++)
	{
		if ((Idx % 32) == 0)
		{
			DbgPrint((DPRINT_MEMORY, "\n"));
			DbgPrint((DPRINT_MEMORY, "%x:\t", (Idx * MM_PAGE_SIZE)));
		}
		else if ((Idx % 4) == 0)
		{
			DbgPrint((DPRINT_MEMORY, " "));
		}

		switch (RealPageLookupTable[Idx].PageAllocated)
		{
		case 0:
			DbgPrint((DPRINT_MEMORY, "*"));
			break;
		case 1:
			DbgPrint((DPRINT_MEMORY, "A"));
			break;
		case MEMTYPE_RESERVED:
			DbgPrint((DPRINT_MEMORY, "R"));
			break;
		case MEMTYPE_ACPI_RECLAIM:
			DbgPrint((DPRINT_MEMORY, "M"));
			break;
		case MEMTYPE_ACPI_NVS:
			DbgPrint((DPRINT_MEMORY, "N"));
			break;
		default:
			DbgPrint((DPRINT_MEMORY, "X"));
			break;
		}
	}

	DbgPrint((DPRINT_MEMORY, "\n"));
}

VOID IncrementAllocationCount(VOID)
{
	AllocationCount++;
}

VOID DecrementAllocationCount(VOID)
{
	AllocationCount--;
}

VOID MemAllocTest(VOID)
{
	PVOID	MemPtr1;
	PVOID	MemPtr2;
	PVOID	MemPtr3;
	PVOID	MemPtr4;
	PVOID	MemPtr5;

	MemPtr1 = MmAllocateMemory(4096);
	printf("MemPtr1: 0x%x\n", (int)MemPtr1);
	getch();
	MemPtr2 = MmAllocateMemory(4096);
	printf("MemPtr2: 0x%x\n", (int)MemPtr2);
	getch();
	MemPtr3 = MmAllocateMemory(4096);
	printf("MemPtr3: 0x%x\n", (int)MemPtr3);
	DumpMemoryAllocMap();
	VerifyHeap();
	getch();

	MmFreeMemory(MemPtr2);
	getch();

	MemPtr4 = MmAllocateMemory(2048);
	printf("MemPtr4: 0x%x\n", (int)MemPtr4);
	getch();
	MemPtr5 = MmAllocateMemory(4096);
	printf("MemPtr5: 0x%x\n", (int)MemPtr5);
	getch();
}
#endif // DEBUG

U32 GetSystemMemorySize(VOID)
{
	return (TotalPagesInLookupTable * MM_PAGE_SIZE);
}
