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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <debug.h>

#if DBG
VOID		DumpMemoryAllocMap(VOID);
VOID		MemAllocTest(VOID);
#endif // DBG

ULONG LoaderPagesSpanned = 0;

PVOID MmAllocateMemoryWithType(ULONG MemorySize, TYPE_OF_MEMORY MemoryType)
{
	ULONG	PagesNeeded;
	ULONG	FirstFreePageFromEnd;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DPRINTM(DPRINT_MEMORY, "MmAllocateMemory() called for 0 bytes. Returning NULL.\n");
		UiMessageBoxCritical("Memory allocation failed: MmAllocateMemory() called for 0 bytes.");
		return NULL;
	}

	MemorySize = ROUND_UP(MemorySize, 4);

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes.\n", MemorySize);
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	FirstFreePageFromEnd = MmFindAvailablePages(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, FALSE);

	if (FirstFreePageFromEnd == 0)
	{
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes.\n", MemorySize);
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded, MemoryType);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)((ULONG_PTR)FirstFreePageFromEnd * MM_PAGE_SIZE);

#if DBG
	DPRINTM(DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d.\n", MemorySize, PagesNeeded, FirstFreePageFromEnd);
	DPRINTM(DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer);
#endif // DBG

	// Update LoaderPagesSpanned count
	if ((((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT) > LoaderPagesSpanned)
		LoaderPagesSpanned = (((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT);

	// Now return the pointer
	return MemPointer;
}

PVOID MmHeapAlloc(ULONG MemorySize)
{
	PVOID Result;
	LONG CurAlloc, TotalFree, MaxFree, NumberOfGets, NumberOfRels;

	if (MemorySize > MM_PAGE_SIZE)
	{
		DPRINTM(DPRINT_MEMORY, "Consider using other functions to allocate %d bytes of memory!\n", MemorySize);
	}

	// Get the buffer from BGET pool
	Result = bget(MemorySize);

	if (Result == NULL)
	{
		DPRINTM(DPRINT_MEMORY, "Heap allocation for %d bytes failed\n", MemorySize);
	}

	// Gather some stats
	bstats(&CurAlloc, &TotalFree, &MaxFree, &NumberOfGets, &NumberOfRels);

	DPRINTM(DPRINT_MEMORY, "Current alloced %d bytes, free %d bytes, allocs %d, frees %d\n",
		CurAlloc, TotalFree, NumberOfGets, NumberOfRels);

	return Result;
}

VOID MmHeapFree(PVOID MemoryPointer)
{
	// Release the buffer to the pool
	brel(MemoryPointer);
}

PVOID MmAllocateMemory(ULONG MemorySize)
{
	// Temporary forwarder...
	return MmAllocateMemoryWithType(MemorySize, LoaderOsloaderHeap);
}

PVOID MmAllocateMemoryAtAddress(ULONG MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType)
{
	ULONG		PagesNeeded;
	ULONG		StartPageNumber;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DPRINTM(DPRINT_MEMORY, "MmAllocateMemoryAtAddress() called for 0 bytes. Returning NULL.\n");
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
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemoryAtAddress(). "
			"Not enough free memory to allocate %d bytes (requesting %d pages but have only %d). "
			"\n", MemorySize, PagesNeeded, FreePagesInLookupTable);
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	if (MmAreMemoryPagesAvailable(PageLookupTableAddress, TotalPagesInLookupTable, DesiredAddress, PagesNeeded) == FALSE)
	{
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateMemoryAtAddress(). "
			"Not enough free memory to allocate %d bytes at address %p.\n",
			MemorySize, DesiredAddress);

		// Don't tell this to user since caller should try to alloc this memory
		// at a different address
		//UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, StartPageNumber, PagesNeeded, MemoryType);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)((ULONG_PTR)StartPageNumber * MM_PAGE_SIZE);

#if DBG
	DPRINTM(DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d.\n", MemorySize, PagesNeeded, StartPageNumber);
	DPRINTM(DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer);
#endif // DBG

	// Update LoaderPagesSpanned count
	if ((((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT) > LoaderPagesSpanned)
		LoaderPagesSpanned = (((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT);

	// Now return the pointer
	return MemPointer;
}

VOID MmSetMemoryType(PVOID MemoryAddress, ULONG MemorySize, TYPE_OF_MEMORY NewType)
{
	ULONG		PagesNeeded;
	ULONG		StartPageNumber;

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// Get the starting page number
	StartPageNumber = MmGetPageNumberFromAddress(MemoryAddress);

	// Set new type for these pages
	MmAllocatePagesInLookupTable(PageLookupTableAddress, StartPageNumber, PagesNeeded, NewType);
}

PVOID MmAllocateHighestMemoryBelowAddress(ULONG MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType)
{
	ULONG		PagesNeeded;
	ULONG		FirstFreePageFromEnd;
	ULONG		DesiredAddressPageNumber;
	PVOID	MemPointer;

	if (MemorySize == 0)
	{
		DPRINTM(DPRINT_MEMORY, "MmAllocateHighestMemoryBelowAddress() called for 0 bytes. Returning NULL.\n");
		UiMessageBoxCritical("Memory allocation failed: MmAllocateHighestMemoryBelowAddress() called for 0 bytes.");
		return NULL;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

	// Get the page number for their desired address
	DesiredAddressPageNumber = (ULONG_PTR)DesiredAddress / MM_PAGE_SIZE;

	// If we don't have enough available mem
	// then return NULL
	if (FreePagesInLookupTable < PagesNeeded)
	{
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes.\n", MemorySize);
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	FirstFreePageFromEnd = MmFindAvailablePagesBeforePage(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, DesiredAddressPageNumber);

	if (FirstFreePageFromEnd == 0)
	{
		DPRINTM(DPRINT_MEMORY, "Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes.\n", MemorySize);
		UiMessageBoxCritical("Memory allocation failed: out of memory.");
		return NULL;
	}

	MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded, MemoryType);

	FreePagesInLookupTable -= PagesNeeded;
	MemPointer = (PVOID)((ULONG_PTR)FirstFreePageFromEnd * MM_PAGE_SIZE);

#if DBG
	DPRINTM(DPRINT_MEMORY, "Allocated %d bytes (%d pages) of memory starting at page %d.\n", MemorySize, PagesNeeded, FirstFreePageFromEnd);
	DPRINTM(DPRINT_MEMORY, "Memory allocation pointer: 0x%x\n", MemPointer);
#endif // DBG

	// Update LoaderPagesSpanned count
	if ((((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT) > LoaderPagesSpanned)
		LoaderPagesSpanned = (((ULONG_PTR)MemPointer + MemorySize) >> PAGE_SHIFT);

	// Now return the pointer
	return MemPointer;
}

VOID MmFreeMemory(PVOID MemoryPointer)
{
}

#if DBG

VOID DumpMemoryAllocMap(VOID)
{
	ULONG							Idx;
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

	DPRINTM(DPRINT_MEMORY, "----------- Memory Allocation Bitmap -----------\n");

	for (Idx=0; Idx<TotalPagesInLookupTable; Idx++)
	{
		if ((Idx % 32) == 0)
		{
			DPRINTM(DPRINT_MEMORY, "\n");
			DPRINTM(DPRINT_MEMORY, "%08x:\t", (Idx * MM_PAGE_SIZE));
		}
		else if ((Idx % 4) == 0)
		{
			DPRINTM(DPRINT_MEMORY, " ");
		}

		switch (RealPageLookupTable[Idx].PageAllocated)
		{
		case LoaderFree:
			DPRINTM(DPRINT_MEMORY, "*");
			break;
		case LoaderBad:
			DPRINTM(DPRINT_MEMORY, "-");
			break;
		case LoaderLoadedProgram:
			DPRINTM(DPRINT_MEMORY, "O");
			break;
		case LoaderFirmwareTemporary:
			DPRINTM(DPRINT_MEMORY, "T");
			break;
		case LoaderFirmwarePermanent:
			DPRINTM(DPRINT_MEMORY, "P");
			break;
		case LoaderOsloaderHeap:
			DPRINTM(DPRINT_MEMORY, "H");
			break;
		case LoaderOsloaderStack:
			DPRINTM(DPRINT_MEMORY, "S");
			break;
		case LoaderSystemCode:
			DPRINTM(DPRINT_MEMORY, "K");
			break;
		case LoaderHalCode:
			DPRINTM(DPRINT_MEMORY, "L");
			break;
		case LoaderBootDriver:
			DPRINTM(DPRINT_MEMORY, "B");
			break;
		case LoaderStartupPcrPage:
			DPRINTM(DPRINT_MEMORY, "G");
			break;
		case LoaderRegistryData:
			DPRINTM(DPRINT_MEMORY, "R");
			break;
		case LoaderMemoryData:
			DPRINTM(DPRINT_MEMORY, "M");
			break;
		case LoaderNlsData:
			DPRINTM(DPRINT_MEMORY, "N");
			break;
		case LoaderSpecialMemory:
			DPRINTM(DPRINT_MEMORY, "C");
			break;
		default:
			DPRINTM(DPRINT_MEMORY, "?");
			break;
		}
	}

	DPRINTM(DPRINT_MEMORY, "\n");
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
