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


#ifndef __MEMORY_H
#define __MEMORY_H

typedef enum
{
	BiosMemoryUsable=1,
	BiosMemoryReserved,
	BiosMemoryAcpiReclaim,
	BiosMemoryAcpiNvs
} BIOS_MEMORY_TYPE;

#include <pshpack1.h>
typedef struct
{
	ULONGLONG		BaseAddress;
	ULONGLONG		Length;
	ULONG		Type;
	ULONG		Reserved;
} BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;
#include <poppack.h>

#if  defined(__i386__) || defined(_PPC_) || defined(_MIPS_) || defined(_ARM_)

#define MM_PAGE_SIZE	4096
#define MM_PAGE_MASK	0xFFF
#define MM_PAGE_SHIFT	12

#define MM_SIZE_TO_PAGES(a)  \
	( ((a) >> MM_PAGE_SHIFT) + ((a) & MM_PAGE_MASK ? 1 : 0) )

#endif // defined __i386__ or _PPC_ or _MIPS_

#if defined (_AMD64_)

#define MM_PAGE_SIZE	4096
#define MM_PAGE_MASK	0xFFF
#define MM_PAGE_SHIFT	12

#define MM_SIZE_TO_PAGES(a)  \
	( ((a) >> MM_PAGE_SHIFT) + ((a) & MM_PAGE_MASK ? 1 : 0) )

#endif

// HEAP and STACK size
#define HEAP_PAGES	0x400
#define STACK_PAGES	0x00

#include <pshpack1.h>
typedef struct
{
	TYPE_OF_MEMORY	PageAllocated;					// Type of allocated memory (LoaderFree if this memory is free)
	ULONG			PageAllocationLength;			// Number of pages allocated (or zero if this isn't the first page in the chain)
} PAGE_LOOKUP_TABLE_ITEM, *PPAGE_LOOKUP_TABLE_ITEM;
#include <poppack.h>

//
// Define this to 1 if you want the entire contents
// of the memory allocation bitmap displayed
// when a chunk is allocated or freed
//
#define DUMP_MEM_MAP_ON_VERIFY	0



extern	PVOID	PageLookupTableAddress;
extern	ULONG		TotalPagesInLookupTable;
extern	ULONG		FreePagesInLookupTable;
extern	ULONG		LastFreePageHint;

#ifdef DBG
PCSTR MmGetSystemMemoryMapTypeString(MEMORY_TYPE Type);
#endif

ULONG		MmGetPageNumberFromAddress(PVOID Address);	// Returns the page number that contains a linear address
ULONG		MmGetAddressablePageCountIncludingHoles(VOID);	// Returns the count of addressable pages from address zero including any memory holes and reserved memory regions
PVOID	MmFindLocationForPageLookupTable(ULONG TotalPageCount);	// Returns the address for a memory chunk big enough to hold the page lookup table (starts search from end of memory)
VOID	MmInitPageLookupTable(PVOID PageLookupTable, ULONG TotalPageCount);	// Inits the page lookup table according to the memory types in the memory map
VOID	MmMarkPagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, TYPE_OF_MEMORY PageAllocated);	// Marks the specified pages as allocated or free in the lookup table
VOID	MmAllocatePagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, TYPE_OF_MEMORY MemoryType);	// Allocates the specified pages in the lookup table
ULONG		MmCountFreePagesInLookupTable(PVOID PageLookupTable, ULONG TotalPageCount);	// Returns the number of free pages in the lookup table
ULONG		MmFindAvailablePages(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, BOOLEAN FromEnd);	// Returns the page number of the first available page range from the beginning or end of memory
ULONG		MmFindAvailablePagesBeforePage(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, ULONG LastPage);	// Returns the page number of the first available page range before the specified page
VOID	MmUpdateLastFreePageHint(PVOID PageLookupTable, ULONG TotalPageCount);	// Sets the LastFreePageHint to the last usable page of memory
BOOLEAN	MmAreMemoryPagesAvailable(PVOID PageLookupTable, ULONG TotalPageCount, PVOID PageAddress, ULONG PageCount);	// Returns TRUE if the specified pages of memory are available, otherwise FALSE
VOID	MmSetMemoryType(PVOID MemoryAddress, ULONG MemorySize, TYPE_OF_MEMORY NewType); // Use with EXTREME caution!

ULONG		GetSystemMemorySize(VOID);								// Returns the amount of total memory in the system
PPAGE_LOOKUP_TABLE_ITEM MmGetMemoryMap(ULONG *NoEntries);			// Returns a pointer to the memory mapping table and a number of entries in it


//BOOLEAN	MmInitializeMemoryManager(ULONG LowMemoryStart, ULONG LowMemoryLength);
BOOLEAN	MmInitializeMemoryManager(VOID);
VOID	MmInitializeHeap(PVOID PageLookupTable);
PVOID	MmAllocateMemory(ULONG MemorySize);
PVOID	MmAllocateMemoryWithType(ULONG MemorySize, TYPE_OF_MEMORY MemoryType);
VOID	MmFreeMemory(PVOID MemoryPointer);
PVOID	MmAllocateMemoryAtAddress(ULONG MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType);
PVOID	MmAllocateHighestMemoryBelowAddress(ULONG MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType);

PVOID	MmHeapAlloc(ULONG MemorySize);
VOID	MmHeapFree(PVOID MemoryPointer);

#endif // defined __MEMORY_H
