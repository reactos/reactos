/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/workingset.cpp
 * PURPOSE:         Working set implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

VOID
WORKING_SET_LIST::
Initialize(PEPROCESS Process)
{
    // Setup some generic list data
    DPRINT1("VM structure pointer %p, Process %p\n", GET_WS_OBJECT(Process), Process);
    LastEntry = GET_WS_OBJECT(Process)->MinimumSize;
    HashTable = NULL;
    HashTableSize = 0;
    Wsle = MemoryManager->GetWsle();
    Quota = LastEntry;
    NumberOfImageWaiters = 0;
    ImageMappingEvent = (PKEVENT)NULL;

    // Create 3 working set list entries which always exist
    WS_LIST_ENTRY *CurrentEntry = Wsle;
    PVOID Addr;
    PFN_NUMBER PfnNr;
    PMMPFN Pfn;
    ULONG WsEntries = 3;

    // Entry 0 - for PDE
    Addr = (PVOID)PDE_BASE;
    CurrentEntry->SetVirtualAddress(Addr);
    CurrentEntry->InitValidDirect();

    // Set back the ws index for that page
    PfnNr = PTENTRY::AddressToPte((ULONG_PTR)Addr)->GetPfn();
    Pfn = MemoryManager->PfnDb.GetEntry(PfnNr);

    // Mark zero index as current process
    DPRINT("Entry 0: VA %p (PTE %p), Page 0x%x, Pfn %p\n", CurrentEntry->GetVirtualAddress(), PTENTRY::AddressToPte((ULONG_PTR)Addr), PfnNr, Pfn);
    Pfn->u1.WsIndex = (ULONG)Process;

    // Entry 1 - for hyper space PTE
    CurrentEntry++;
    Addr = (PVOID)PTENTRY::AddressToPte((ULONG_PTR)HYPER_SPACE);
    CurrentEntry->SetVirtualAddress(Addr);
    CurrentEntry->InitValidDirect();

    // Set back the ws index for that page
    PfnNr = PTENTRY::AddressToPte((ULONG_PTR)Addr)->GetPfn();
    Pfn = MemoryManager->PfnDb.GetEntry(PfnNr);

    // Make sure previous WS index was zero and set the new one
    DPRINT("Entry 1: VA %p (PTE %p), Page 0x%x, Pfn %p\n", CurrentEntry->GetVirtualAddress(), PTENTRY::AddressToPte((ULONG_PTR)Addr), PfnNr, Pfn);
    ASSERT(Pfn->u1.WsIndex == 0);
    Pfn->u1.WsIndex = 1;

    // Entry 2 - for working set itself, one page
    CurrentEntry++;
    Addr = (PVOID)MemoryManager->GetWorkingSetList();
    CurrentEntry->SetVirtualAddress(Addr);
    CurrentEntry->InitValidDirect();

    // Set back the ws index for that page
    PfnNr = PTENTRY::AddressToPte((ULONG_PTR)Addr)->GetPfn();
    Pfn = MemoryManager->PfnDb.GetEntry(PfnNr);

    // Make sure previous WS index was zero and set the new one
    DPRINT("Entry 2: VA %p (PTE %p), Page 0x%x, Pfn %p\n", CurrentEntry->GetVirtualAddress(), PTENTRY::AddressToPte((ULONG_PTR)Addr), PfnNr, Pfn);
    ASSERT(Pfn->u1.WsIndex == 0);
    Pfn->u1.WsIndex = 2;

    // Check if more pages are needed to map this working set
    ULONG MaxEntries = (WS_LIST_ENTRY *)((ULONG)MI_WORKING_SET_LIST + PAGE_SIZE) - MemoryManager->GetWsle();
    if (GET_WS_OBJECT(Process)->MaximumSize >= MaxEntries)
    {
        // FIXME: The working set does not fit into one page
        UNIMPLEMENTED;
    }

    // Set current working set list size
    GET_WS_OBJECT(Process)->CurrentSize = WsEntries;

    // Initialize first free entries indices
    FirstFree = WsEntries;
    FirstDynamic = WsEntries;
    NextSlot = WsEntries;

    // Mark the rest as free
    ULONG k;
    for (k = WsEntries + 1; k <= MaxEntries; k++)
    {
        CurrentEntry++;
        CurrentEntry->SetFree(k);
    }

    // And the last entry
    CurrentEntry->SetEndOfList();
    LastInitializedWsle = MaxEntries - 1;

    if (GET_WS_OBJECT(Process)->MaximumSize > 384)
    {
        // FIXME: The hash needs to be used, as WsIndex field in PFN entry would overflow
        UNIMPLEMENTED;
    }
}
