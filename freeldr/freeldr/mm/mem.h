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


#ifndef __MEM_H
#define __MEM_H


#define MM_PAGE_SIZE	4096

typedef struct
{
	U32		PageAllocated;					// Zero = free, non-zero = allocated
	U32		PageAllocationLength;			// Number of pages allocated (or zero if this isn't the first page in the chain)
} PAGE_LOOKUP_TABLE_ITEM, *PPAGE_LOOKUP_TABLE_ITEM;

//
// Define this to 1 if you want the entire contents
// of the memory allocation bitmap displayed
// when a chunk is allocated or freed
//
#define DUMP_MEM_MAP_ON_VERIFY	0



extern	PVOID	PageLookupTableAddress;
extern	U32		TotalPagesInLookupTable;
extern	U32		FreePagesInLookupTable;
extern	U32		LastFreePageHint;

#ifdef DEBUG
PUCHAR	MmGetSystemMemoryMapTypeString(U32 Type);
#endif

U32		MmGetPageNumberFromAddress(PVOID Address);	// Returns the page number that contains a linear address
PVOID	MmGetEndAddressOfAnyMemory(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount);	// Returns the last address of memory from the memory map
U32		MmGetAddressablePageCountIncludingHoles(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount);	// Returns the count of addressable pages from address zero including any memory holes and reserved memory regions
PVOID	MmFindLocationForPageLookupTable(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount);	// Returns the address for a memory chunk big enough to hold the page lookup table (starts search from end of memory)
VOID	MmSortBiosMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount);	// Sorts the BIOS_MEMORY_MAP array so the first element corresponds to the first address in memory
VOID	MmInitPageLookupTable(PVOID PageLookupTable, U32 TotalPageCount, BIOS_MEMORY_MAP BiosMemoryMap[32], U32 MapCount);	// Inits the page lookup table according to the memory types in the memory map
VOID	MmMarkPagesInLookupTable(PVOID PageLookupTable, U32 StartPage, U32 PageCount, U32 PageAllocated);	// Marks the specified pages as allocated or free in the lookup table
VOID	MmAllocatePagesInLookupTable(PVOID PageLookupTable, U32 StartPage, U32 PageCount);	// Allocates the specified pages in the lookup table
U32		MmCountFreePagesInLookupTable(PVOID PageLookupTable, U32 TotalPageCount);	// Returns the number of free pages in the lookup table
U32		MmFindAvailablePagesFromEnd(PVOID PageLookupTable, U32 TotalPageCount, U32 PagesNeeded);	// Returns the page number of the first available page range from the end of memory
VOID	MmFixupSystemMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32], U32* MapCount);	// Removes entries in the memory map that describe memory above 4G
VOID	MmUpdateLastFreePageHint(PVOID PageLookupTable, U32 TotalPageCount);	// Sets the LastFreePageHint to the last usable page of memory
BOOL	MmAreMemoryPagesAvailable(PVOID PageLookupTable, U32 TotalPageCount, PVOID PageAddress, U32 PageCount);	// Returns TRUE if the specified pages of memory are available, otherwise FALSE

#endif // defined __MEM_H
