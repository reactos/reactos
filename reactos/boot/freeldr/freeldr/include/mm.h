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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

extern char __ImageBase;
#ifdef __GNUC__
#define FREELDR_SECTION_COUNT 3
#else
#ifdef _M_AMD64
/* .text and .pdata */
#define FREELDR_SECTION_COUNT 2
#else
#define FREELDR_SECTION_COUNT 1
#endif
#endif

typedef struct _FREELDR_MEMORY_DESCRIPTOR
{
    TYPE_OF_MEMORY MemoryType;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} FREELDR_MEMORY_DESCRIPTOR, *PFREELDR_MEMORY_DESCRIPTOR;


#if  defined(__i386__) || defined(_PPC_) || defined(_MIPS_) || defined(_ARM_)

#define MM_PAGE_SIZE    4096
#define MM_PAGE_MASK    0xFFF
#define MM_PAGE_SHIFT    12
#define MM_MAX_PAGE        0xFFFFF

#define MM_SIZE_TO_PAGES(a)  \
    ( ((a) >> MM_PAGE_SHIFT) + ((a) & MM_PAGE_MASK ? 1 : 0) )

#endif // defined __i386__ or _PPC_ or _MIPS_

#if defined (_AMD64_)

#define MM_PAGE_SIZE    4096
#define MM_PAGE_MASK    0xFFF
#define MM_PAGE_SHIFT    12
// FIXME: freeldr implementation uses ULONG for page numbers
#define MM_MAX_PAGE        0xFFFFFFFFFFFFF

#define MM_SIZE_TO_PAGES(a)  \
    ( ((a) >> MM_PAGE_SHIFT) + ((a) & MM_PAGE_MASK ? 1 : 0) )

#endif

// HEAP and STACK size
#define HEAP_PAGES    0x400
#define STACK_PAGES    0x00

#include <pshpack1.h>
typedef struct
{
    TYPE_OF_MEMORY    PageAllocated;                    // Type of allocated memory (LoaderFree if this memory is free)
    PFN_NUMBER            PageAllocationLength;            // Number of pages allocated (or zero if this isn't the first page in the chain)
} PAGE_LOOKUP_TABLE_ITEM, *PPAGE_LOOKUP_TABLE_ITEM;
#include <poppack.h>

//
// Define this to 1 if you want the entire contents
// of the memory allocation bitmap displayed
// when a chunk is allocated or freed
//
#define DUMP_MEM_MAP_ON_VERIFY    0

extern PVOID PageLookupTableAddress;
extern PFN_NUMBER TotalPagesInLookupTable;
extern PFN_NUMBER FreePagesInLookupTable;
extern PFN_NUMBER LastFreePageHint;

#if DBG
PCSTR MmGetSystemMemoryMapTypeString(TYPE_OF_MEMORY Type);
#endif

PFN_NUMBER MmGetPageNumberFromAddress(PVOID Address);    // Returns the page number that contains a linear address
PFN_NUMBER MmGetAddressablePageCountIncludingHoles(VOID);    // Returns the count of addressable pages from address zero including any memory holes and reserved memory regions
PVOID MmFindLocationForPageLookupTable(PFN_NUMBER TotalPageCount);    // Returns the address for a memory chunk big enough to hold the page lookup table (starts search from end of memory)
VOID MmInitPageLookupTable(PVOID PageLookupTable, PFN_NUMBER TotalPageCount);    // Inits the page lookup table according to the memory types in the memory map
VOID MmMarkPagesInLookupTable(PVOID PageLookupTable, PFN_NUMBER StartPage, PFN_NUMBER PageCount, TYPE_OF_MEMORY PageAllocated);    // Marks the specified pages as allocated or free in the lookup table
VOID MmAllocatePagesInLookupTable(PVOID PageLookupTable, PFN_NUMBER StartPage, PFN_NUMBER PageCount, TYPE_OF_MEMORY MemoryType);    // Allocates the specified pages in the lookup table
PFN_NUMBER MmCountFreePagesInLookupTable(PVOID PageLookupTable, PFN_NUMBER TotalPageCount);    // Returns the number of free pages in the lookup table
PFN_NUMBER MmFindAvailablePages(PVOID PageLookupTable, PFN_NUMBER TotalPageCount, PFN_NUMBER PagesNeeded, BOOLEAN FromEnd);    // Returns the page number of the first available page range from the beginning or end of memory
PFN_NUMBER MmFindAvailablePagesBeforePage(PVOID PageLookupTable, PFN_NUMBER TotalPageCount, PFN_NUMBER PagesNeeded, PFN_NUMBER LastPage);    // Returns the page number of the first available page range before the specified page
VOID MmUpdateLastFreePageHint(PVOID PageLookupTable, PFN_NUMBER TotalPageCount);    // Sets the LastFreePageHint to the last usable page of memory
BOOLEAN MmAreMemoryPagesAvailable(PVOID PageLookupTable, PFN_NUMBER TotalPageCount, PVOID PageAddress, PFN_NUMBER PageCount);    // Returns TRUE if the specified pages of memory are available, otherwise FALSE
VOID MmSetMemoryType(PVOID MemoryAddress, SIZE_T MemorySize, TYPE_OF_MEMORY NewType); // Use with EXTREME caution!

PPAGE_LOOKUP_TABLE_ITEM MmGetMemoryMap(PFN_NUMBER *NoEntries);            // Returns a pointer to the memory mapping table and a number of entries in it


//BOOLEAN    MmInitializeMemoryManager(ULONG LowMemoryStart, ULONG LowMemoryLength);
BOOLEAN    MmInitializeMemoryManager(VOID);
VOID    MmInitializeHeap(PVOID PageLookupTable);
PVOID    MmAllocateMemory(SIZE_T MemorySize);
PVOID    MmAllocateMemoryWithType(SIZE_T MemorySize, TYPE_OF_MEMORY MemoryType);
VOID    MmFreeMemory(PVOID MemoryPointer);
PVOID    MmAllocateMemoryAtAddress(SIZE_T MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType);
PVOID    MmAllocateHighestMemoryBelowAddress(SIZE_T MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType);

/* Heap */
#define DEFAULT_HEAP_SIZE (1024 * 1024)
#define TEMP_HEAP_SIZE (32 * 1024 * 1024)

extern PVOID FrLdrDefaultHeap;
extern PVOID FrLdrTempHeap;
extern SIZE_T FrLdrImageSize;

PVOID
FrLdrHeapCreate(
    SIZE_T MaximumSize,
    TYPE_OF_MEMORY MemoryType);

VOID
FrLdrHeapDestroy(
    PVOID HeapHandle);

VOID
FrLdrHeapRelease(
    PVOID HeapHandle);

VOID
FrLdrHeapVerify(
    PVOID HeapHandle);

VOID
FrLdrHeapCleanupAll(VOID);

PVOID
FrLdrHeapAllocateEx(
    PVOID HeapHandle,
    SIZE_T ByteSize,
    ULONG Tag);

VOID
FrLdrHeapFreeEx(
    PVOID HeapHandle,
    PVOID Pointer,
    ULONG Tag);

FORCEINLINE
PVOID
FrLdrHeapAlloc(SIZE_T MemorySize, ULONG Tag)
{
    return FrLdrHeapAllocateEx(FrLdrDefaultHeap, MemorySize, Tag);
}

FORCEINLINE
VOID
FrLdrHeapFree(PVOID MemoryPointer, ULONG Tag)
{
    FrLdrHeapFreeEx(FrLdrDefaultHeap, MemoryPointer, Tag);
}

FORCEINLINE
PVOID
FrLdrTempAlloc(
    ULONG Size, ULONG Tag)
{
    return FrLdrHeapAllocateEx(FrLdrTempHeap, Size, Tag);
}

FORCEINLINE
VOID
FrLdrTempFree(
    PVOID Allocation, ULONG Tag)
{
    FrLdrHeapFreeEx(FrLdrTempHeap, Allocation, Tag);
}

