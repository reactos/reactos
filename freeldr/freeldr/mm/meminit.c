/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include <arch.h>
#include <mm.h>
#include "mem.h"
#include <rtl.h>
#include <debug.h>
#include <ui.h>


#ifdef DEBUG
typedef struct
{
	ULONG	Type;
	UCHAR	TypeString[20];
} MEMORY_TYPE, *PMEMORY_TYPE;

ULONG			MemoryTypeCount = 5;
MEMORY_TYPE		MemoryTypeArray[] =
{
	{ 0, "Unknown Memory" },
	{ MEMTYPE_USABLE, "Usable Memory" },
	{ MEMTYPE_RESERVED, "Reserved Memory" },
	{ MEMTYPE_ACPI_RECLAIM, "ACPI Reclaim Memory" },
	{ MEMTYPE_ACPI_NVS, "ACPI NVS Memory" },
};
#endif

PVOID	PageLookupTableAddress = NULL;
ULONG	TotalPagesInLookupTable = 0;
ULONG	FreePagesInLookupTable = 0;
ULONG	LastFreePageHint = 0;

BOOL MmInitializeMemoryManager(VOID)
{
	BIOS_MEMORY_MAP	BiosMemoryMap[32];
	ULONG			BiosMemoryMapEntryCount;
	ULONG			ExtendedMemorySize;
	ULONG			ConventionalMemorySize;
	ULONG			Index;

	DbgPrint((DPRINT_MEMORY, "Initializing Memory Manager.\n"));

	RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);

	BiosMemoryMapEntryCount = GetBiosMemoryMap(BiosMemoryMap);
	ExtendedMemorySize = GetExtendedMemorySize();
	ConventionalMemorySize = GetConventionalMemorySize();

	// If we got the system memory map then fixup invalid entries
	if (BiosMemoryMapEntryCount != 0)
	{
		MmFixupSystemMemoryMap(BiosMemoryMap, &BiosMemoryMapEntryCount);
	}

#ifdef DEBUG
	// Dump the system memory map
	if (BiosMemoryMapEntryCount != 0)
	{
		DbgPrint((DPRINT_MEMORY, "System Memory Map (Base Address, Length, Type):\n"));
		for (Index=0; Index<BiosMemoryMapEntryCount; Index++)
		{
			DbgPrint((DPRINT_MEMORY, "%x%x\t %x%x\t %s\n", BiosMemoryMap[Index].BaseAddressHigh, BiosMemoryMap[Index].BaseAddressLow, BiosMemoryMap[Index].LengthHigh, BiosMemoryMap[Index].LengthLow, MmGetSystemMemoryMapTypeString(BiosMemoryMap[Index].Type)));
		}
	}
	else
	{
		DbgPrint((DPRINT_MEMORY, "GetBiosMemoryMap() not supported.\n"));
	}
#endif

	DbgPrint((DPRINT_MEMORY, "Extended memory size: %d KB\n", ExtendedMemorySize));
	DbgPrint((DPRINT_MEMORY, "Conventional memory size: %d KB\n", ConventionalMemorySize));

	// Since I don't feel like writing two sets of routines
	// one to handle the BiosMemoryMap structure and another
	// to handle just a flat extended memory size I'm going
	// to create a 'fake' memory map entry out of the
	// extended memory size if GetBiosMemoryMap() fails.
	if (BiosMemoryMapEntryCount == 0)
	{
		BiosMemoryMap[0].BaseAddressLow = 0x100000;		// Start at 1MB
		BiosMemoryMap[0].BaseAddressHigh = 0;
		BiosMemoryMap[0].LengthLow = ExtendedMemorySize * 1024;
		BiosMemoryMap[0].LengthHigh = 0;
		BiosMemoryMap[0].Type = MEMTYPE_USABLE;
		BiosMemoryMapEntryCount = 1;
	}

	TotalPagesInLookupTable = MmGetAddressablePageCountIncludingHoles(BiosMemoryMap, BiosMemoryMapEntryCount);
	PageLookupTableAddress = MmFindLocationForPageLookupTable(BiosMemoryMap, BiosMemoryMapEntryCount);
	LastFreePageHint = TotalPagesInLookupTable;

	if (PageLookupTableAddress == 0)
	{
		// If we get here then we probably couldn't
		// find a contigous chunk of memory big
		// enough to hold the page lookup table
		printf("Error initializing memory manager!\n");
		return FALSE;
	}

	MmInitPageLookupTable(PageLookupTableAddress, TotalPagesInLookupTable, BiosMemoryMap, BiosMemoryMapEntryCount);
	MmUpdateLastFreePageHint(PageLookupTableAddress, TotalPagesInLookupTable);

	FreePagesInLookupTable = MmCountFreePagesInLookupTable(PageLookupTableAddress, TotalPagesInLookupTable);

	DbgPrint((DPRINT_MEMORY, "Memory Manager initialized. %d pages available.\n", FreePagesInLookupTable));
	return TRUE;
}

#ifdef DEBUG
PUCHAR MmGetSystemMemoryMapTypeString(ULONG Type)
{
	ULONG	Index;

	for (Index=1; Index<MemoryTypeCount; Index++)
	{
		if (MemoryTypeArray[Index].Type == Type)
		{
			return MemoryTypeArray[Index].TypeString;
		}
	}
	
	return MemoryTypeArray[0].TypeString;
}
#endif

ULONG MmGetPageNumberFromAddress(PVOID Address)
{
	return ((ULONG)Address) / MM_PAGE_SIZE;
}

PVOID MmGetEndAddressOfAnyMemory(BIOS_MEMORY_MAP BiosMemoryMap[32], ULONG MapCount)
{
	ULONG	MaxStartAddressSoFar;
	UINT64	EndAddressOfMemory;
	ULONG	Index;

	MaxStartAddressSoFar = 0;
	EndAddressOfMemory = 0;
	for (Index=0; Index<MapCount; Index++)
	{
		if (MaxStartAddressSoFar < BiosMemoryMap[Index].BaseAddressLow)
		{
			MaxStartAddressSoFar = BiosMemoryMap[Index].BaseAddressLow;
			EndAddressOfMemory = ((UINT64)MaxStartAddressSoFar + (UINT64)BiosMemoryMap[Index].LengthLow);
			if (EndAddressOfMemory > 0xFFFFFFFF)
			{
				EndAddressOfMemory = 0xFFFFFFFF;
			}
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmGetEndAddressOfAnyMemory() returning 0x%x\n", (UINT32)EndAddressOfMemory));

	return (PVOID)(UINT32)EndAddressOfMemory;
}

ULONG MmGetAddressablePageCountIncludingHoles(BIOS_MEMORY_MAP BiosMemoryMap[32], ULONG MapCount)
{
	ULONG	PageCount;

	PageCount = MmGetPageNumberFromAddress(MmGetEndAddressOfAnyMemory(BiosMemoryMap, MapCount));

	DbgPrint((DPRINT_MEMORY, "MmGetAddressablePageCountIncludingHoles() returning %d\n", PageCount));

	return PageCount;
}

PVOID MmFindLocationForPageLookupTable(BIOS_MEMORY_MAP BiosMemoryMap[32], ULONG MapCount)
{
	ULONG				TotalPageCount;
	ULONG				PageLookupTableSize;
	PVOID				PageLookupTableAddress;
	int					Index;
	BIOS_MEMORY_MAP		TempBiosMemoryMap[32];

	TotalPageCount = MmGetAddressablePageCountIncludingHoles(BiosMemoryMap, MapCount);
	PageLookupTableSize = TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM);
	PageLookupTableAddress = 0;

	RtlCopyMemory(TempBiosMemoryMap, BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);
	MmSortBiosMemoryMap(TempBiosMemoryMap, MapCount);

	for (Index=(MapCount-1); Index>=0; Index--)
	{
		// If this is usable memory with a big enough length
		// then we'll put our page lookup table here
		if (TempBiosMemoryMap[Index].Type == MEMTYPE_USABLE && TempBiosMemoryMap[Index].LengthLow >= PageLookupTableSize)
		{
			PageLookupTableAddress = (PVOID)(TempBiosMemoryMap[Index].BaseAddressLow + (TempBiosMemoryMap[Index].LengthLow - PageLookupTableSize));
			break;
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmFindLocationForPageLookupTable() returning 0x%x\n", PageLookupTableAddress));

	return PageLookupTableAddress;
}

VOID MmSortBiosMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], ULONG MapCount)
{
	ULONG				Index;
	ULONG				LoopCount;
	BIOS_MEMORY_MAP		TempMapItem;

	// Loop once for each entry in the memory map minus one
	// On each loop iteration go through and sort the memory map
	for (LoopCount=0; LoopCount<(MapCount-1); LoopCount++)
	{
		for (Index=0; Index<(MapCount-1); Index++)
		{
			if (BiosMemoryMap[Index].BaseAddressLow > BiosMemoryMap[Index+1].BaseAddressLow)
			{
				TempMapItem = BiosMemoryMap[Index];
				BiosMemoryMap[Index] = BiosMemoryMap[Index+1];
				BiosMemoryMap[Index+1] = TempMapItem;
			}
		}
	}
}

VOID MmInitPageLookupTable(PVOID PageLookupTable, ULONG TotalPageCount, BIOS_MEMORY_MAP BiosMemoryMap[32], ULONG MapCount)
{
	ULONG	MemoryMapStartPage;
	ULONG	MemoryMapEndPage;
	ULONG	MemoryMapPageCount;
	ULONG	MemoryMapPageAllocated;
	ULONG	PageLookupTableStartPage;
	ULONG	PageLookupTablePageCount;
	ULONG	Index;

	DbgPrint((DPRINT_MEMORY, "MmInitPageLookupTable()\n"));

	// Mark every page as allocated initially
	// We will go through and mark pages again according to the memory map
	// But this will mark any holes not described in the map as allocated
	MmMarkPagesInLookupTable(PageLookupTable, 0, TotalPageCount, 1);

	for (Index=0; Index<MapCount; Index++)
	{
		MemoryMapStartPage = MmGetPageNumberFromAddress((PVOID)BiosMemoryMap[Index].BaseAddressLow);
		MemoryMapEndPage = MmGetPageNumberFromAddress((PVOID)(BiosMemoryMap[Index].BaseAddressLow + BiosMemoryMap[Index].LengthLow - 1));
		MemoryMapPageCount = (MemoryMapEndPage - MemoryMapStartPage) + 1;
		MemoryMapPageAllocated = (BiosMemoryMap[Index].Type == MEMTYPE_USABLE) ? 0 : BiosMemoryMap[Index].Type;
		DbgPrint((DPRINT_MEMORY, "Marking pages as type %d: StartPage: %d PageCount: %d\n", MemoryMapPageAllocated, MemoryMapStartPage, MemoryMapPageCount));
		MmMarkPagesInLookupTable(PageLookupTable, MemoryMapStartPage, MemoryMapPageCount, MemoryMapPageAllocated);
	}

	// Mark the low memory region below 1MB as reserved (256 pages in region)
	DbgPrint((DPRINT_MEMORY, "Marking the low 1MB region as reserved.\n"));
	MmMarkPagesInLookupTable(PageLookupTable, 0, 256, MEMTYPE_RESERVED);

	// Mark the pages that the lookup tabel occupies as reserved
	PageLookupTableStartPage = MmGetPageNumberFromAddress(PageLookupTable);
	PageLookupTablePageCount = MmGetPageNumberFromAddress(PageLookupTable + ROUND_UP(TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM), MM_PAGE_SIZE)) - PageLookupTableStartPage;
	DbgPrint((DPRINT_MEMORY, "Marking the page lookup table pages as reserved StartPage: %d PageCount: %d\n", PageLookupTableStartPage, PageLookupTablePageCount));
	MmMarkPagesInLookupTable(PageLookupTable, PageLookupTableStartPage, PageLookupTablePageCount, MEMTYPE_RESERVED);
}

VOID MmMarkPagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, ULONG PageAllocated)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG						Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		RealPageLookupTable[Index].PageAllocated = PageAllocated;
		RealPageLookupTable[Index].PageAllocationLength = PageAllocated ? 1 : 0;
	}
}

VOID MmAllocatePagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG						Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		RealPageLookupTable[Index].PageAllocated = 1;
		RealPageLookupTable[Index].PageAllocationLength = (Index == StartPage) ? PageCount : 0;
	}
}

ULONG MmCountFreePagesInLookupTable(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG						Index;
	ULONG						FreePageCount;

	FreePageCount = 0;
	for (Index=0; Index<TotalPageCount; Index++)
	{
		if (RealPageLookupTable[Index].PageAllocated == 0)
		{
			FreePageCount++;
		}
	}

	return FreePageCount;
}

ULONG MmFindAvailablePagesFromEnd(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG						AvailablePageStart;
	ULONG						AvailablePagesSoFar;
	ULONG						Index;

	if (LastFreePageHint > TotalPageCount)
	{
		LastFreePageHint = TotalPageCount;
	}

	AvailablePageStart = 0;
	AvailablePagesSoFar = 0;
	for (Index=LastFreePageHint-1; Index>=0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated != 0)
		{
			AvailablePagesSoFar = 0;
			continue;
		}
		else
		{
			AvailablePagesSoFar++;
		}

		if (AvailablePagesSoFar >= PagesNeeded)
		{
			return Index;
		}
	}

	return 0;
}

VOID MmFixupSystemMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], PULONG MapCount)
{
	int		Index;
	int		Index2;
	UINT64	RealLength;

	// Loop through each entry in the array
	for (Index=0; Index<*MapCount; Index++)
	{
		// If the base address for this entry starts at
		// or above 4G then remove this entry
		if (BiosMemoryMap[Index].BaseAddressHigh != 0)
		{
			// Slide every entry after this down one
			for (Index2=Index; Index2<(*MapCount - 1); Index2++)
			{
				BiosMemoryMap[Index2] = BiosMemoryMap[Index2 + 1];
			}
			(*MapCount)--;
			Index--;
		}

		// If the base address plus the length for this entry
		// extends beyond 4G then truncate this entry
		RealLength = BiosMemoryMap[Index].BaseAddressLow + BiosMemoryMap[Index].LengthLow;
		if ((BiosMemoryMap[Index].LengthHigh != 0) || (RealLength > 0xFFFFFFFF))
		{
			BiosMemoryMap[Index].LengthLow = 0xFFFFFFFF - BiosMemoryMap[Index].BaseAddressLow;
		}
	}
}

VOID MmUpdateLastFreePageHint(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG						Index;

	for (Index=TotalPageCount-1; Index>=0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated == 0)
		{
			LastFreePageHint = Index + 1;
			break;
		}
	}
}
