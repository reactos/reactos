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
	U32		Type;
	UCHAR	TypeString[20];
} MEMORY_TYPE, *PMEMORY_TYPE;

U32				MemoryTypeCount = 5;
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
U32		TotalPagesInLookupTable = 0;
U32		FreePagesInLookupTable = 0;
U32		LastFreePageHint = 0;

BOOL MmInitializeMemoryManager(VOID)
{
	BIOS_MEMORY_MAP	BiosMemoryMap[32];
	U32				BiosMemoryMapEntryCount;
	U32				ExtendedMemorySize;
	U32				ConventionalMemorySize;
#ifdef DEBUG
	U32				Index;
#endif

	DbgPrint((DPRINT_MEMORY, "Initializing Memory Manager.\n"));

	RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);

	BiosMemoryMapEntryCount = GetBiosMemoryMap(BiosMemoryMap, 32);
	ExtendedMemorySize = GetExtendedMemorySize();
	ConventionalMemorySize = GetConventionalMemorySize();

#ifdef DEBUG
	// Dump the system memory map
	if (BiosMemoryMapEntryCount != 0)
	{
		DbgPrint((DPRINT_MEMORY, "System Memory Map (Base Address, Length, Type):\n"));
		for (Index=0; Index<BiosMemoryMapEntryCount; Index++)
		{
			DbgPrint((DPRINT_MEMORY, "%x%x\t %x%x\t %s\n", BiosMemoryMap[Index].BaseAddress, BiosMemoryMap[Index].Length, MmGetSystemMemoryMapTypeString(BiosMemoryMap[Index].Type)));
		}
	}
	else
	{
		DbgPrint((DPRINT_MEMORY, "GetBiosMemoryMap() not supported.\n"));
	}

	DbgPrint((DPRINT_MEMORY, "Extended memory size: %d KB\n", ExtendedMemorySize));
	DbgPrint((DPRINT_MEMORY, "Conventional memory size: %d KB\n", ConventionalMemorySize));
#endif

	// If we got the system memory map then fixup invalid entries
	if (BiosMemoryMapEntryCount != 0)
	{
		MmFixupSystemMemoryMap(BiosMemoryMap, &BiosMemoryMapEntryCount);
	}

	// Since I don't feel like writing two sets of routines
	// one to handle the BiosMemoryMap structure and another
	// to handle just a flat extended memory size I'm going
	// to create a 'fake' memory map entry out of the
	// extended memory size if GetBiosMemoryMap() fails.
	//if (BiosMemoryMapEntryCount == 0)
	{
		BiosMemoryMap[0].BaseAddress = 0x100000;		// Start at 1MB
		BiosMemoryMap[0].Length = ExtendedMemorySize * 1024;
		BiosMemoryMap[0].Type = MEMTYPE_USABLE;
		BiosMemoryMapEntryCount = 1;
	}

	TotalPagesInLookupTable = MmGetAddressablePageCountIncludingHoles(BiosMemoryMap, BiosMemoryMapEntryCount);
	PageLookupTableAddress = MmFindLocationForPageLookupTable(BiosMemoryMap, BiosMemoryMapEntryCount);
	LastFreePageHint = TotalPagesInLookupTable;

	if (PageLookupTableAddress == 0)
	{
		// If we get here then we probably couldn't
		// find a contiguous chunk of memory big
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
PUCHAR MmGetSystemMemoryMapTypeString(U32 Type)
{
	U32		Index;

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

U32 MmGetPageNumberFromAddress(PVOID Address)
{
	return ((U32)Address) / MM_PAGE_SIZE;
}

PVOID MmGetEndAddressOfAnyMemory(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount)
{
	U64		MaxStartAddressSoFar;
	U64		EndAddressOfMemory;
	U32		Index;

	MaxStartAddressSoFar = 0;
	EndAddressOfMemory = 0;
	for (Index=0; Index<MapCount; Index++)
	{
		if (MaxStartAddressSoFar < BiosMemoryMap[Index].BaseAddress)
		{
			MaxStartAddressSoFar = BiosMemoryMap[Index].BaseAddress;
			EndAddressOfMemory = (MaxStartAddressSoFar + BiosMemoryMap[Index].Length);
			if (EndAddressOfMemory > 0xFFFFFFFF)
			{
				EndAddressOfMemory = 0xFFFFFFFF;
			}
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmGetEndAddressOfAnyMemory() returning 0x%x\n", (U32)EndAddressOfMemory));

	return (PVOID)(U32)EndAddressOfMemory;
}

U32 MmGetAddressablePageCountIncludingHoles(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount)
{
	U32		PageCount;
	U64		EndAddress;

	EndAddress = (U64)(U32)MmGetEndAddressOfAnyMemory(BiosMemoryMap, MapCount);

	// Since MmGetEndAddressOfAnyMemory() won't
	// return addresses higher than 0xFFFFFFFF
	// then we need to adjust the end address
	// to 0x100000000 so we don't get an
	// off-by-one error
	if (EndAddress >= 0xFFFFFFFF)
	{
		EndAddress = 0x100000000;

		DbgPrint((DPRINT_MEMORY, "MmGetEndAddressOfAnyMemory() returned 0xFFFFFFFF, correcting to be 0x100000000.\n"));
	}

	PageCount = (EndAddress / MM_PAGE_SIZE);

	DbgPrint((DPRINT_MEMORY, "MmGetAddressablePageCountIncludingHoles() returning %d\n", PageCount));

	return PageCount;
}

PVOID MmFindLocationForPageLookupTable(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount)
{
	U32					TotalPageCount;
	U32					PageLookupTableSize;
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
		if (TempBiosMemoryMap[Index].Type == MEMTYPE_USABLE && TempBiosMemoryMap[Index].Length >= PageLookupTableSize)
		{
			PageLookupTableAddress = (PVOID)(U32)(TempBiosMemoryMap[Index].BaseAddress + (TempBiosMemoryMap[Index].Length - PageLookupTableSize));
			break;
		}
	}

	DbgPrint((DPRINT_MEMORY, "MmFindLocationForPageLookupTable() returning 0x%x\n", PageLookupTableAddress));

	return PageLookupTableAddress;
}

VOID MmSortBiosMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount)
{
	U32					Index;
	U32					LoopCount;
	BIOS_MEMORY_MAP		TempMapItem;

	// Loop once for each entry in the memory map minus one
	// On each loop iteration go through and sort the memory map
	for (LoopCount=0; LoopCount<(MapCount-1); LoopCount++)
	{
		for (Index=0; Index<(MapCount-1); Index++)
		{
			if (BiosMemoryMap[Index].BaseAddress > BiosMemoryMap[Index+1].BaseAddress)
			{
				TempMapItem = BiosMemoryMap[Index];
				BiosMemoryMap[Index] = BiosMemoryMap[Index+1];
				BiosMemoryMap[Index+1] = TempMapItem;
			}
		}
	}
}

VOID MmInitPageLookupTable(PVOID PageLookupTable, U32 TotalPageCount, BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount)
{
	U32		MemoryMapStartPage;
	U32		MemoryMapEndPage;
	U32		MemoryMapPageCount;
	U32		MemoryMapPageAllocated;
	U32		PageLookupTableStartPage;
	U32		PageLookupTablePageCount;
	U32		Index;

	DbgPrint((DPRINT_MEMORY, "MmInitPageLookupTable()\n"));

	// Mark every page as allocated initially
	// We will go through and mark pages again according to the memory map
	// But this will mark any holes not described in the map as allocated
	MmMarkPagesInLookupTable(PageLookupTable, 0, TotalPageCount, 1);

	for (Index=0; Index<MapCount; Index++)
	{
		MemoryMapStartPage = MmGetPageNumberFromAddress((PVOID)(U32)BiosMemoryMap[Index].BaseAddress);
		MemoryMapEndPage = MmGetPageNumberFromAddress((PVOID)(U32)(BiosMemoryMap[Index].BaseAddress + BiosMemoryMap[Index].Length - 1));
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

VOID MmMarkPagesInLookupTable(PVOID PageLookupTable, U32 StartPage, U32 PageCount, U32 PageAllocated)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		if ((Index <= (StartPage + 16)) || (Index >= (StartPage+PageCount-16)))
		{
			DbgPrint((DPRINT_MEMORY, "Index = %d StartPage = %d PageCount = %d\n", Index, StartPage, PageCount));
		}
		RealPageLookupTable[Index].PageAllocated = PageAllocated;
		RealPageLookupTable[Index].PageAllocationLength = PageAllocated ? 1 : 0;
	}
	DbgPrint((DPRINT_MEMORY, "MmMarkPagesInLookupTable() Done\n"));
}

VOID MmAllocatePagesInLookupTable(PVOID PageLookupTable, U32 StartPage, U32 PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							Index;

	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		RealPageLookupTable[Index].PageAllocated = 1;
		RealPageLookupTable[Index].PageAllocationLength = (Index == StartPage) ? PageCount : 0;
	}
}

U32 MmCountFreePagesInLookupTable(PVOID PageLookupTable, U32 TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							Index;
	U32							FreePageCount;

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

U32 MmFindAvailablePagesFromEnd(PVOID PageLookupTable, U32 TotalPageCount, U32 PagesNeeded)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							AvailablePageStart;
	U32							AvailablePagesSoFar;
	U32							Index;

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

VOID MmFixupSystemMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], U32* MapCount)
{
	int		Index;
	int		Index2;

	// Loop through each entry in the array
	for (Index=0; Index<*MapCount; Index++)
	{
		// If the entry type isn't usable then remove
		// it from the memory map (this will help reduce
		// the size of our lookup table)
		if (BiosMemoryMap[Index].Type != MEMTYPE_USABLE)
		{
			// Slide every entry after this down one
			for (Index2=Index; Index2<(*MapCount - 1); Index2++)
			{
				BiosMemoryMap[Index2] = BiosMemoryMap[Index2 + 1];
			}
			(*MapCount)--;
			Index--;
		}
	}
}

VOID MmUpdateLastFreePageHint(PVOID PageLookupTable, U32 TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							Index;

	for (Index=TotalPageCount-1; Index>=0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated == 0)
		{
			LastFreePageHint = Index + 1;
			break;
		}
	}
}

BOOL MmAreMemoryPagesAvailable(PVOID PageLookupTable, U32 TotalPageCount, PVOID PageAddress, U32 PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	U32							StartPage;
	U32							Index;

	StartPage = MmGetPageNumberFromAddress(PageAddress);

	// Make sure they aren't trying to go past the
	// end of availabe memory
	if ((StartPage + PageCount) > TotalPageCount)
	{
		return FALSE;
	}

	for (Index=StartPage; Index<(StartPage + PageCount); Index++)
	{
		// If this page is allocated then there obviously isn't
		// memory availabe so return FALSE
		if (RealPageLookupTable[Index].PageAllocated != 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}
