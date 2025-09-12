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

#if DBG
VOID    DumpMemoryAllocMap(VOID);
#endif // DBG

DBG_DEFAULT_CHANNEL(MEMORY);

PFN_NUMBER LoaderPagesSpanned = 0;

PVOID MmAllocateMemoryWithType(SIZE_T MemorySize, TYPE_OF_MEMORY MemoryType)
{
    PFN_NUMBER    PagesNeeded;
    PFN_NUMBER    FirstFreePageFromEnd;
    PVOID    MemPointer;

    if (MemorySize == 0)
    {
        WARN("MmAllocateMemory() called for 0 bytes. Returning NULL.\n");
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
        ERR("Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes.\n", MemorySize);
        UiMessageBoxCritical("Memory allocation failed: out of memory.");
        return NULL;
    }

    FirstFreePageFromEnd = MmFindAvailablePages(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, FALSE);

    if (FirstFreePageFromEnd == 0)
    {
        ERR("Memory allocation failed in MmAllocateMemory(). Not enough free memory to allocate %d bytes.\n", MemorySize);
        UiMessageBoxCritical("Memory allocation failed: out of memory.");
        return NULL;
    }

    MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded, MemoryType);

    FreePagesInLookupTable -= PagesNeeded;
    MemPointer = (PVOID)((ULONG_PTR)FirstFreePageFromEnd * MM_PAGE_SIZE);

    TRACE("Allocated %d bytes (%d pages) of memory (type %ld) starting at page 0x%lx.\n",
          MemorySize, PagesNeeded, MemoryType, FirstFreePageFromEnd);
    TRACE("Memory allocation pointer: 0x%x\n", MemPointer);

    // Update LoaderPagesSpanned count
    if ((((ULONG_PTR)MemPointer + MemorySize + PAGE_SIZE - 1) >> PAGE_SHIFT) > LoaderPagesSpanned)
        LoaderPagesSpanned = (((ULONG_PTR)MemPointer + MemorySize + PAGE_SIZE - 1) >> PAGE_SHIFT);

    // Now return the pointer
    return MemPointer;
}

PVOID MmAllocateMemoryAtAddress(SIZE_T MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType)
{
    PFN_NUMBER        PagesNeeded;
    PFN_NUMBER        StartPageNumber;
    PVOID    MemPointer;

    if (MemorySize == 0)
    {
        WARN("MmAllocateMemoryAtAddress() called for 0 bytes. Returning NULL.\n");
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
        ERR("Memory allocation failed in MmAllocateMemoryAtAddress(). "
            "Not enough free memory to allocate %d bytes (requesting %d pages but have only %d). "
            "\n", MemorySize, PagesNeeded, FreePagesInLookupTable);
        UiMessageBoxCritical("Memory allocation failed: out of memory.");
        return NULL;
    }

    if (MmAreMemoryPagesAvailable(PageLookupTableAddress, TotalPagesInLookupTable, DesiredAddress, PagesNeeded) == FALSE)
    {
        WARN("Memory allocation failed in MmAllocateMemoryAtAddress(). "
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

    TRACE("Allocated %d bytes (%d pages) of memory starting at page %d.\n", MemorySize, PagesNeeded, StartPageNumber);
    TRACE("Memory allocation pointer: 0x%x\n", MemPointer);

    // Update LoaderPagesSpanned count
    if ((((ULONG_PTR)MemPointer + MemorySize + PAGE_SIZE - 1) >> PAGE_SHIFT) > LoaderPagesSpanned)
        LoaderPagesSpanned = (((ULONG_PTR)MemPointer + MemorySize + PAGE_SIZE - 1) >> PAGE_SHIFT);

    // Now return the pointer
    return MemPointer;
}

VOID MmSetMemoryType(PVOID MemoryAddress, SIZE_T MemorySize, TYPE_OF_MEMORY NewType)
{
    PFN_NUMBER        PagesNeeded;
    PFN_NUMBER        StartPageNumber;

    // Find out how many blocks it will take to
    // satisfy this allocation
    PagesNeeded = ROUND_UP(MemorySize, MM_PAGE_SIZE) / MM_PAGE_SIZE;

    // Get the starting page number
    StartPageNumber = MmGetPageNumberFromAddress(MemoryAddress);

    // Set new type for these pages
    MmAllocatePagesInLookupTable(PageLookupTableAddress, StartPageNumber, PagesNeeded, NewType);
}

PVOID MmAllocateHighestMemoryBelowAddress(SIZE_T MemorySize, PVOID DesiredAddress, TYPE_OF_MEMORY MemoryType)
{
    PFN_NUMBER        PagesNeeded;
    PFN_NUMBER        FirstFreePageFromEnd;
    PFN_NUMBER        DesiredAddressPageNumber;
    PVOID    MemPointer;

    if (MemorySize == 0)
    {
        WARN("MmAllocateHighestMemoryBelowAddress() called for 0 bytes. Returning NULL.\n");
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
        ERR("Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes.\n", MemorySize);
        UiMessageBoxCritical("Memory allocation failed: out of memory.");
        return NULL;
    }

    FirstFreePageFromEnd = MmFindAvailablePagesBeforePage(PageLookupTableAddress, TotalPagesInLookupTable, PagesNeeded, DesiredAddressPageNumber);

    if (FirstFreePageFromEnd == 0)
    {
        ERR("Memory allocation failed in MmAllocateHighestMemoryBelowAddress(). Not enough free memory to allocate %d bytes.\n", MemorySize);
        UiMessageBoxCritical("Memory allocation failed: out of memory.");
        return NULL;
    }

    MmAllocatePagesInLookupTable(PageLookupTableAddress, FirstFreePageFromEnd, PagesNeeded, MemoryType);

    FreePagesInLookupTable -= PagesNeeded;
    MemPointer = (PVOID)((ULONG_PTR)FirstFreePageFromEnd * MM_PAGE_SIZE);

    TRACE("Allocated %d bytes (%d pages) of memory starting at page %d.\n", MemorySize, PagesNeeded, FirstFreePageFromEnd);
    TRACE("Memory allocation pointer: 0x%x\n", MemPointer);

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
    PFN_NUMBER    Idx;
    PPAGE_LOOKUP_TABLE_ITEM        RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

    DbgPrint("----------- Memory Allocation Bitmap -----------\n");

    for (Idx=0; Idx<TotalPagesInLookupTable; Idx++)
    {
        if ((Idx % 32) == 0)
        {
            DbgPrint("\n");
            DbgPrint("%08x:\t", (Idx * MM_PAGE_SIZE));
        }
        else if ((Idx % 4) == 0)
        {
            DbgPrint(" ");
        }

        switch (RealPageLookupTable[Idx].PageAllocated)
        {
        case LoaderFree:
            DbgPrint("*");
            break;
        case LoaderBad:
            DbgPrint("-");
            break;
        case LoaderLoadedProgram:
            DbgPrint("O");
            break;
        case LoaderFirmwareTemporary:
            DbgPrint("T");
            break;
        case LoaderFirmwarePermanent:
            DbgPrint("P");
            break;
        case LoaderOsloaderHeap:
            DbgPrint("H");
            break;
        case LoaderOsloaderStack:
            DbgPrint("S");
            break;
        case LoaderSystemCode:
            DbgPrint("K");
            break;
        case LoaderHalCode:
            DbgPrint("L");
            break;
        case LoaderBootDriver:
            DbgPrint("B");
            break;
        case LoaderStartupPcrPage:
            DbgPrint("G");
            break;
        case LoaderRegistryData:
            DbgPrint("R");
            break;
        case LoaderMemoryData:
            DbgPrint("M");
            break;
        case LoaderNlsData:
            DbgPrint("N");
            break;
        case LoaderSpecialMemory:
            DbgPrint("C");
            break;
        default:
            DbgPrint("?");
            break;
        }
    }

    DbgPrint("\n");
}
#endif // DBG

PPAGE_LOOKUP_TABLE_ITEM MmGetMemoryMap(PFN_NUMBER *NoEntries)
{
    PPAGE_LOOKUP_TABLE_ITEM        RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

    *NoEntries = TotalPagesInLookupTable;

    return RealPageLookupTable;
}

PFN_NUMBER
MmGetLoaderPagesSpanned(VOID)
{
    return LoaderPagesSpanned;
}
