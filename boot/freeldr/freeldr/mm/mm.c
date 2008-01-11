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
#include <debug.h>

ULONG			AllocationCount = 0;

#ifdef DBG
VOID		VerifyHeap(VOID);
VOID		DumpMemoryAllocMap(VOID);
VOID		IncrementAllocationCount(VOID);
VOID		DecrementAllocationCount(VOID);
VOID		MemAllocTest(VOID);
#endif // DBG

/*
 * Hack alert
 * Normally, we allocate whole pages. This is ofcourse wastefull for small
 * allocations (a few bytes). So, for small allocations (smaller than a page)
 * we sub-allocate. When the first small allocation is done, a page is
 * requested. We keep a pointer to that page in SubAllocationPage. The alloc
 * is satisfied by returning a pointer to the beginning of the page. We also
 * keep track of how many bytes are still available in the page in SubAllocationRest.
 * When the next small request comes in, we try to allocate it just after the
 * memory previously allocated. If it won't fit, we allocate a new page and
 * the whole process starts again.
 * Note that suballocations are done back-to-back, there's no bookkeeping at all.
 * That also means that we cannot really free suballocations. So, when a free is
 * done and it is determined that this might be a free of a sub-allocation, we
 * just no-op the free.
 * Perhaps we should use the heap routines from ntdll here.
 */
static PVOID    SubAllocationPage = NULL;
static unsigned SubAllocationRest = 0;

BOOLEAN AllocateFromEnd = TRUE;

VOID MmChangeAllocationPolicy(BOOLEAN PolicyAllocatePagesFromEnd)
{
	AllocateFromEnd = PolicyAllocatePagesFromEnd;
}

PVOID MmAllocateMemory(ULONG MemorySize)
{
	ULONG	PagesNeeded;
	ULONG	FirstFreePageFromEnd;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DbgPrint((DPRINT_MEMORY, "MmAllocateMemory() called for 0 bytes. Returning NULL.\n"));
		UiMessageBoxCritical("Memory allocation failed: MmAllocateMemory() called for 0 bytes.");
		return NULL;
	}

	MemorySize = ROUND_UP(MemorySize, 4);
	if (MemorySize <= SubAllocationRest)
	{
		MemPointer = (PVOID)((ULONG_PTR)SubAllocationPage + MM_PAGE_SIZE - SubAllocationRest);
		SubAllocationRest -= MemorySize;
		return MemPointer;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	FirstFreePageFromEnd = MmFindAvailablePages(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, AllocateFromEnd);

	if (FirstFreePageFromEnd == (ULONG)-1)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)(FirstFreePageFromEnd * MM_PAGE_SIZE);

	if (MemorySize < MM_PAGE_SIZE)
	{
		SubAllocationPage = MemPointer;
		SubAllocationRest = MM_PAGE_SIZE - MemorySize;
	}


#ifdef DBG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d. AllocCount: %d\n", MemorySize, PagesNeeded, FirstFreePageFromEnd, AllocationCount));
	DbgPrint((DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer));
	//VerifyHeap();
#endif // DBG

	// Now return the pointer
	return MemPointer;
}

PVOID MmAllocateMemoryAtAddress(ULONG MemorySize, PVOID DesiredAddress)
{
	ULONG		PagesNeeded;
	ULONG		StartPageNumber;
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
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemoryAtAddress(). "
			"Not enough free memory to allocate %d bytes (requesting %d pages but have only %d). "
			"AllocationCount: %d\n", MemorySize, PagesNeeded, FreePagesInLookupTable, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	if (MmAreMemoryPagesAvailable(PageLookupTableAddress, TotalPagesInLookupTable, DesiredAddress, PagesNeeded) == FALSE)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemoryAtAddress(). "
			"Not enough free memory to allocate %d bytes at address %p. AllocationCount: %d\n",
			MemorySize, DesiredAddress, AllocationCount));

		// Don't tell this to user since caller should try to alloc this memory
		// at a different address
		//UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, StartPageNumber, PagesNeeded);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)(StartPageNumber * MM_PAGE_SIZE);

#ifdef DBG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d. AllocCount: %d\n", MemorySize, PagesNeeded, StartPageNumber, AllocationCount));
	DbgPrint((DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer));
	//VerifyHeap();
#endif // DBG

	// Now return the pointer
	return MemPointer;
}

PVOID MmAllocateHighestMemoryBelowAddress(ULONG MemorySize, PVOID DesiredAddress)
{
	ULONG		PagesNeeded;
	ULONG		FirstFreePageFromEnd;
	ULONG		DesiredAddressPageNumber;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DbgPrint((DPRINT_MEMORY, "MmAllocateHighestMemoryBelowAddress() called for 0 bytes. Returning NULL.\n"));
		UiMessageBoxCritical("Memory allocation failed: MmAllocateHighestMemoryBelowAddress() called for 0 bytes.");
		return NULL;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// Get the page number for their desired address
	DesiredAddressPageNumber = (ULONG)DesiredAddress / MM_PAGE_SIZE;

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	FirstFreePageFromEnd = MmFindAvailablePagesBeforePage(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, DesiredAddressPageNumber);

	if (FirstFreePageFromEnd == 0)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes. AllocationCount: %d\n", MemorySize, AllocationCount));
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)(FirstFreePageFromEnd * MM_PAGE_SIZE);

#ifdef DBG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d. AllocCount: %d\n", MemorySize, PagesNeeded, FirstFreePageFromEnd, AllocationCount));
	DbgPrint((DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer));
	//VerifyHeap();
#endif // DBG

	// Now return the pointer
	return MemPointer;
}

VOID MmFreeMemory(PVOID MemoryPointer)
{
	ULONG							PageNumber;
	ULONG							PageCount;
	ULONG							Idx;
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

#ifdef DBG

	// Make sure we didn't get a bogus pointer
	if (MemoryPointer >= (PVOID)(TotalPagesInLookupTable * MM_PAGE_SIZE))
	{
		BugCheck((DPRINT_MEMORY, "Bogus memory pointer (0x%x) passed to MmFreeMemory()\n", MemoryPointer));
	}
#endif // DBG

	// Find out the page number of the first
	// page of memory they allocated
	PageNumber = MmGetPageNumberFromAddress(MemoryPointer);
	PageCount = RealPageLookupTable[PageNumber].PageAllocationLength;

#ifdef DBG
	// Make sure we didn't get a bogus pointer
	if ((PageCount < 1) || (PageCount > (TotalPagesInLookupTable - PageNumber)))
	{
		BugCheck((DPRINT_MEMORY, "Invalid page count in lookup table. PageLookupTable[%d].PageAllocationLength = %d\n", PageNumber, RealPageLookupTable[PageNumber].PageAllocationLength));
	}

	// Loop through our array check all the pages
	// to make sure they are allocated with a length of 0
	for (Idx=PageNumber+1; Idx<(PageNumber + PageCount); Idx++)
	{
		if ((RealPageLookupTable[Idx].PageAllocated == LoaderFree) ||
			(RealPageLookupTable[Idx].PageAllocationLength != 0))
		{
			BugCheck((DPRINT_MEMORY, "Invalid page entry in lookup table, PageAllocated should = 1 and PageAllocationLength should = 0 because this is not the first block in the run. PageLookupTable[%d].PageAllocated = %d PageLookupTable[%d].PageAllocationLength = %d\n", PageNumber, RealPageLookupTable[PageNumber].PageAllocated, PageNumber, RealPageLookupTable[PageNumber].PageAllocationLength));
		}
	}

#endif

	/* If this allocation is only a single page, it could be a sub-allocated page.
	 * Just don't free it */
	if (1 == PageCount)
	{
		return;
	}

	// Loop through our array and mark all the
	// blocks as free
	for (Idx=PageNumber; Idx<(PageNumber + PageCount); Idx++)
	{
		RealPageLookupTable[Idx].PageAllocated = LoaderFree;
		RealPageLookupTable[Idx].PageAllocationLength = 0;
	}

	FreePagesInLookupTable += PageCount;

#ifdef DBG
	DecrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Freed %d pages of memory starting at page %d. AllocationCount: %d\n", PageCount, PageNumber, AllocationCount));
	//VerifyHeap();
#endif // DBG
}

#ifdef DBG
VOID VerifyHeap(VOID)
{
	ULONG							Idx;
	ULONG							Idx2;
	ULONG							Count;
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
		if (RealPageLookupTable[Idx].PageAllocated != LoaderFree)
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
				if (RealPageLookupTable[Idx + Idx2].PageAllocated == LoaderFree)
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
	ULONG							Idx;
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
		case LoaderFree:
			DbgPrint((DPRINT_MEMORY, "*"));
			break;
		case LoaderBad:
			DbgPrint((DPRINT_MEMORY, "-"));
			break;
		case LoaderLoadedProgram:
			DbgPrint((DPRINT_MEMORY, "O"));
			break;
		case LoaderFirmwareTemporary:
			DbgPrint((DPRINT_MEMORY, "T"));
			break;
		case LoaderFirmwarePermanent:
			DbgPrint((DPRINT_MEMORY, "P"));
			break;
		case LoaderOsloaderHeap:
			DbgPrint((DPRINT_MEMORY, "H"));
			break;
		case LoaderOsloaderStack:
			DbgPrint((DPRINT_MEMORY, "S"));
			break;
		case LoaderSystemCode:
			DbgPrint((DPRINT_MEMORY, "K"));
			break;
		case LoaderHalCode:
			DbgPrint((DPRINT_MEMORY, "L"));
			break;
		case LoaderBootDriver:
			DbgPrint((DPRINT_MEMORY, "B"));
			break;
		case LoaderStartupPcrPage:
			DbgPrint((DPRINT_MEMORY, "G"));
			break;
		case LoaderRegistryData:
			DbgPrint((DPRINT_MEMORY, "R"));
			break;
		case LoaderMemoryData:
			DbgPrint((DPRINT_MEMORY, "M"));
			break;
		case LoaderNlsData:
			DbgPrint((DPRINT_MEMORY, "N"));
			break;
		case LoaderSpecialMemory:
			DbgPrint((DPRINT_MEMORY, "C"));
			break;
		default:
			DbgPrint((DPRINT_MEMORY, "?"));
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
	MachConsGetCh();
	MemPtr2 = MmAllocateMemory(4096);
	printf("MemPtr2: 0x%x\n", (int)MemPtr2);
	MachConsGetCh();
	MemPtr3 = MmAllocateMemory(4096);
	printf("MemPtr3: 0x%x\n", (int)MemPtr3);
	DumpMemoryAllocMap();
	VerifyHeap();
	MachConsGetCh();

	MmFreeMemory(MemPtr2);
	MachConsGetCh();

	MemPtr4 = MmAllocateMemory(2048);
	printf("MemPtr4: 0x%x\n", (int)MemPtr4);
	MachConsGetCh();
	MemPtr5 = MmAllocateMemory(4096);
	printf("MemPtr5: 0x%x\n", (int)MemPtr5);
	MachConsGetCh();
}
#endif // DBG

ULONG GetSystemMemorySize(VOID)
{
	return (TotalPagesInLookupTable * MM_PAGE_SIZE);
}

PPAGE_LOOKUP_TABLE_ITEM MmGetMemoryMap(ULONG *NoEntries)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

	*NoEntries = TotalPagesInLookupTable;

	return RealPageLookupTable;
}
