/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/systemcache.cpp
 * PURPOSE:         System cache implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

NTSTATUS
SYSTEM_CACHE::
Initialize()
{
    NTSTATUS Status = STATUS_SUCCESS;

    LargeSystemCache = 0;
    SystemCacheWsMin = 700;
    SystemCacheWsMax = 1200;
    SizeOfSystemCacheInPages = (64 * _1MB) >> PAGE_SHIFT;

    // Set start values
    SystemCacheWorkingSetList = (WORKING_SET_LIST *)MI_SYSTEM_CACHE_WS_START;
    SystemCacheStart = MI_SYSTEM_CACHE_START;

    // Update system cache size if the system has more than 16Mb of physical RAM
    ULONG Pages1024 = (MmNumberOfPhysicalPages + 65) / 1024;
    if (Pages1024 >= 4)
    {
        // If there is so much of physical memory (what a surprise), then
        // cache size becomes 128Mb plus 64Mb for each 1024 pages increase
        SizeOfSystemCacheInPages = ((128 * _1MB) >> PAGE_SHIFT) + ((Pages1024 - 4) * ((64 * _1MB) >> PAGE_SHIFT));

        // Limit size to the max size
        if (SizeOfSystemCacheInPages > (((ULONG_PTR)SystemCacheEnd - (ULONG_PTR)SystemCacheStart) >> PAGE_SHIFT))
            SizeOfSystemCacheInPages = (((ULONG_PTR)SystemCacheEnd - (ULONG_PTR)SystemCacheStart) >> PAGE_SHIFT);
    }

    // Calculate end of the system cache area
    SystemCacheEnd = (PVOID)(((ULONG_PTR)SystemCacheStart + SizeOfSystemCacheInPages * PAGE_SIZE) - 1);

    // Map PDE range for system cache
    PTENTRY TempPte = MemoryManager->ValidKernelPte;
    PTENTRY *StartPde = PTENTRY::AddressToPde((ULONG_PTR)SystemCacheWorkingSetList);
    PTENTRY *StartPte = PTENTRY::AddressToPte((ULONG_PTR)SystemCacheWorkingSetList);
    PTENTRY *EndPde = PTENTRY::AddressToPde((ULONG_PTR)SystemCacheEnd);

    // Consistency check for SystemCacheWorkingSetList and SystemCacheStart
    ASSERT((StartPde + 1) == PTENTRY::AddressToPde((ULONG_PTR)SystemCacheStart));

    KIRQL OldIrql = MemoryManager->PfnDb.AcquireLock();
    while (StartPde <= EndPde)
    {
        // Make sure current Pde is invalid
        ASSERT(!StartPde->IsValid());

        // Get a new page from the PFN db
        ULONG PageFrameIndex = MemoryManager->PfnDb.RemoveAnyPage(MemoryManager->GetPageColor());
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        *StartPde = TempPte;

        // Set up the PFN entry for this page
        PMMPFN Pfn1 = MemoryManager->PfnDb.GetEntry(PageFrameIndex);
        Pfn1->u4.PteFrame = PTENTRY::AddressToPde(PDE_BASE)->GetPfn();
        Pfn1->PteAddress = StartPde;
        Pfn1->OriginalPte.u.Long = 0;
        Pfn1->u2.ShareCount++;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e2.ReferenceCount = 1;

        // Initialize it with zero kernel PTEs
        RtlFillMemoryUlong(StartPte, PAGE_SIZE, MemoryManager->ZeroKernelPte.u.Long);

        // Move to the next one
        StartPde++;
        StartPte += PTE_PER_PAGE;
    }
    MemoryManager->PfnDb.ReleaseLock(OldIrql);

    // Calculate system cache max and min using a simple logic in case of a large system cache
    if (LargeSystemCache)
    {
        if ((MemoryManager->PfnDb.GetAvailablePages() > SystemCacheWsMax + ((6 * _1MB) >> PAGE_SHIFT)))
        {
            SystemCacheWsMax = MemoryManager->PfnDb.GetAvailablePages() - ((4 * _1MB) >> PAGE_SHIFT);
        }
    }

    // "Reserve" 5 pages in the working set
    if (SystemCacheWsMax > (MemoryManager->GetWorkingSetLimit() - 5))
        SystemCacheWsMax = MemoryManager->GetWorkingSetLimit() - 5;

    // If max ws size is larger than the total size of the system cache - clip it
    if (SystemCacheWsMax > SizeOfSystemCacheInPages)
    {
        SystemCacheWsMax = SizeOfSystemCacheInPages;

        // If the WS range is less than 500 pages, set it to 500 pages
        if ((SystemCacheWsMin + 500) > SystemCacheWsMax)
            SystemCacheWsMin = SystemCacheWsMax - 500;
    }

    // Finally perform the actual initialization of the system cache
    Status = InitializeInternal();
    if (!NT_SUCCESS(Status)) return Status;

    return Status;
}


NTSTATUS
SYSTEM_CACHE::
InitializeInternal()
{
    // Get a zeroed page
    ULONG Pfn = MemoryManager->PfnDb.RemoveZeroPage(MemoryManager->GetPageColor());

    // Get PTE address of a system cache working set list
    PTENTRY *PointerPte = PTENTRY::AddressToPte((ULONG_PTR)SystemCacheWorkingSetList);

    // Initialize this PFN
    *PointerPte = MemoryManager->ValidKernelPte;
    PointerPte->SetPfn(Pfn);
    MemoryManager->PfnDb.InitializeElement(Pfn, PointerPte, 1);

    // Initialize system cache WS and WSLE structures
    SystemCacheWs.VmWorkingSetList = SystemCacheWorkingSetList;
    SystemCacheWs.CurrentSize = 0;
    SystemCacheWs.MinimumSize = SystemCacheWsMin;
    SystemCacheWs.MaximumSize = SystemCacheWsMax;
    SystemCacheWs.AllowAdjustment = TRUE;
    InsertTailList(&MemoryManager->WorkingSetExpansionHead, &SystemCacheWs.ExpansionLinks);

    // Initialize system cache working set list structure
    SystemCacheWorkingSetList->FirstFree = 1;
    SystemCacheWorkingSetList->FirstDynamic = 1;
    SystemCacheWorkingSetList->NextSlot = 1;
    SystemCacheWorkingSetList->LastEntry = SystemCacheWsMin;
    SystemCacheWorkingSetList->Quota = SystemCacheWsMin;
    SystemCacheWorkingSetList->HashTable = NULL;
    SystemCacheWorkingSetList->HashTableSize = 0;
    SystemCacheWsle = (WS_LIST_ENTRY *)SystemCacheWorkingSetList->UsedPageTableEntries;
    SystemCacheWorkingSetList->Wsle = SystemCacheWsle;

    // Calculate number of mapped entries and map till maximum
    ULONG EntriesMapped = ((WS_LIST_ENTRY *)((ULONG_PTR)SystemCacheWorkingSetList + PAGE_SIZE)) - SystemCacheWsle;
    while (EntriesMapped < SystemCacheWsMax)
    {
        PointerPte++;
        Pfn = MemoryManager->PfnDb.RemoveZeroPage(MemoryManager->GetPageColor());
        *PointerPte = MemoryManager->ValidKernelPte;
        PointerPte->SetPfn(Pfn);
        MemoryManager->PfnDb.InitializeElement(Pfn, PointerPte, 1);
        EntriesMapped += PAGE_SIZE / sizeof(WS_LIST_ENTRY);
    }

    // If anything is left, initialize the list of free entries
    WS_LIST_ENTRY *Entry = SystemCacheWsle + 1;
    for (Pfn = 1; Pfn < EntriesMapped; Pfn++)
    {
        Entry->SetFree(Pfn + 1);
        Entry++;
    }

    // Mark end of the list
    Entry--;
    Entry->SetEndOfList();
    SystemCacheWorkingSetList->LastInitializedWsle = EntriesMapped - 1;

    // Build PTEs for free system cache 256KB blocks
    FirstFreeSystemCache = 0;
    SystemCachePtes = PTENTRY::AddressToPte((ULONG_PTR)SystemCacheStart);
    ULONG CacheBlocks = SizeOfSystemCacheInPages / (256 * _1KB / PAGE_SIZE);
    ULONG NextFree = 0;
    for (Pfn = 0; Pfn < CacheBlocks; Pfn++)
    {
        SystemCachePtes[NextFree].SetPfn(NextFree + (256 * _1KB / PAGE_SIZE));
        NextFree += (256 * _1KB / PAGE_SIZE);
    }
    LastFreeSystemCache = NextFree - (256 * _1KB / PAGE_SIZE);
    SystemCachePtes[LastFreeSystemCache].SetEmpty();

    // Grow WS list if needed
    if (SystemCacheWsMax > ((1536 * 1024) >> PAGE_SHIFT))
    {
        MemoryManager->GrowWsleHash(&SystemCacheWs, FALSE);
    }

    return STATUS_SUCCESS;
}
