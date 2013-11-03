/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/i386/mach86.cpp
 * PURPOSE:         x86-specific implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "../memorymanager.hpp"
#include "mach86.hpp"

//#define NDEBUG
#include <debug.h>


MEMORY_MANAGER stMemoryManager;
MEMORY_MANAGER *MemoryManager = &stMemoryManager;

VOID
MEMORY_MANAGER::
MachineDependentInit0(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;

    // Initialize our sample PTEs and PDEs
    ValidKernelPte.u.Long = 0;
    ValidKernelPte.u.Hard.Valid = 1;
    ValidKernelPte.u.Hard.Write = 1;
    ValidKernelPte.u.Hard.Dirty = 1;
    ValidKernelPte.u.Hard.Accessed = 1;

    ZeroKernelPte.u.Long = 0;

    ValidKernelPde.u.Long = 0;
    ValidKernelPde.u.Hard.Valid = 1;
    ValidKernelPde.u.Hard.Write = 1;
    ValidKernelPde.u.Hard.Dirty = 1;
    ValidKernelPde.u.Hard.Accessed = 1;

    DemandZeroPte.u.Long  = MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS;

    // Initialize max working set size for this architecture
    WorkingSetLimit = (2 *_1GB  - 64 * _1MB) >> PAGE_SHIFT;

    // Default secondary colors value
    SecondaryColors = 64;

    /* Check for global bit */
#if 0
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Set it on the template PTE and PDE */
        ValidKernelPte.u.Hard.Global = TRUE;
        ValidKernelPde.u.Hard.Global = TRUE;
    }
#endif

    // Set CR3 for the system process
    PTENTRY *PointerPte = PTENTRY::AddressToPde(PDE_BASE);
    PageFrameIndex = PointerPte->GetPfn() << PAGE_SHIFT;
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameIndex;
    DPRINT1("System's process CR3 %p, PFN %x, PTE %p\n", PageFrameIndex, PointerPte->GetPfn(), PointerPte);

    // Blow away user-mode
    PTENTRY *StartPde = PTENTRY::AddressToPde(0);
    PTENTRY *EndPde = PTENTRY::AddressToPde(KSEG0_BASE);
    RtlZeroMemory(StartPde, (EndPde - StartPde) * sizeof(PTENTRY));

    // Initialize non-paged pool
    Pools.InitializeNonPagedPool(LbNumberOfFreePages);

    // Now calculate the nonpaged system VA region, which includes the
    // nonpaged pool expansion (above) and the system PTEs. Note that it is
    // then aligned to a PDE boundary (4MB).
    ULONG MiNonPagedSystemSize = (SystemPtes.GetSystemPtesNumber() + 1) * PAGE_SIZE;
    SystemPtes.NonPagedSystemStart = (PVOID)(Pools.NonPagedPoolPtr.Start - MiNonPagedSystemSize);
    SystemPtes.NonPagedSystemStart = (PVOID)((ULONG_PTR)SystemPtes.NonPagedSystemStart & ~(PDE_MAPPED_VA - 1));

    // Don't let it go below the minimum
    if (SystemPtes.NonPagedSystemStart < (PVOID)0xEB000000)
    {
        // This is a hard-coded limit in the Windows NT address space
        SystemPtes.NonPagedSystemStart = (PVOID)0xEB000000;

        // Reduce the amount of system PTEs to reach this point
        SystemPtes.SetSystemPtesNumber((((ULONG_PTR)Pools.NonPagedPoolPtr.Start - (ULONG_PTR)SystemPtes.NonPagedSystemStart) >> PAGE_SHIFT) - 1);
        ASSERT(SystemPtes.GetSystemPtesNumber() > 1000);
    }

    // Check if we are in a situation where the size of the paged pool
    // is so large that it overflows into nonpaged pool
    //if (MmSizeOfPagedPoolInBytes > ((ULONG_PTR)SystemPtes.NonPagedSystemStart - (ULONG_PTR)MmPagedPoolStart))
    //{
        // We need some recalculations here
        //DPRINT1("Paged pool is too big!\n");
    //}

    DPRINT("System NP start %p\n", SystemPtes.NonPagedSystemStart);
    DPRINT("NPP VA %p - %p\n", Pools.NonPagedPoolPtr.Start, Pools.NonPagedPoolPtr.End);

    // Now we need some pages to create the page tables for the NP system VA
    // which includes system PTEs and expansion NP
    StartPde = PTENTRY::AddressToPde((ULONG_PTR)SystemPtes.NonPagedSystemStart);
    EndPde = PTENTRY::AddressToPde((ULONG_PTR)Pools.NonPagedPoolPtr.End - 1);
    PTENTRY TempPde = ValidKernelPde;
    while (StartPde <= EndPde)
    {
        // Get a page
        TempPde.SetPfn(LbGetNextPage(1));
        StartPde->WriteValidPte(&TempPde);

        // Zero out the page table
        PointerPte = (PTENTRY *)StartPde->PteToAddress();
        RtlZeroMemory(PointerPte, PAGE_SIZE);

        // Next
        StartPde++;
    }

    // FIXME: Large pages mapping?
    LargePagesMapped = FALSE;

    // Last step is to actually map the nonpaged pool
    PTENTRY TempPte = ValidKernelPte;
    PointerPte = PTENTRY::AddressToPte(Pools.NonPagedPoolPtr.Start);
    PTENTRY *LastPte = PTENTRY::AddressToPte((ULONG_PTR)Pools.NonPagedPoolPtr.Start + Pools.NonPagedPoolPtr.SizeInBytes - 1);
    PageFrameIndex = LbGetNextPage(LastPte - PointerPte + 1);
    while (PointerPte <= LastPte)
    {
        // Use one of our contigous pages
        TempPte.SetPfn(PageFrameIndex++);
        PointerPte->WriteValidPte(&TempPte);

        // Next
        PointerPte++;
    }

    // Now remember where expansion starts
    Pools.NonPagedPoolPtr.ExpansionStart = Pools.NonPagedPoolPtr.Start + Pools.NonPagedPoolPtr.SizeInBytes;

    // Now go ahead and initialize the nonpaged pool
    Pools.NonPagedPoolPtr.Initialize();

    // Set number of colors and compute color mask
    //SecondaryColors = 64; // FIXME: Re-compute if not default
    SecondaryColorMask = SecondaryColors - 1;

    // Map the PFN database
    PfnDb.Map(LoaderBlock, LargePagesMapped);

    // Initialize free pages by color lists
    PfnDb.InitializeFreePagesByColor();

    // If non-paged pool is in KSEG0 - map it
    if (Pools.NonPagedPoolPtr.Start < MM_KSEG2_BASE)
    {
        UNIMPLEMENTED;
    }

    PfnDb.InitializePfnDatabase(LoaderBlock, LargePagesMapped);

    // Reset the descriptor back so we can create the correct memory blocks
    *LbFreeDescriptor = LbOldFreeDescriptor;

    // Initialize the nonpaged pool
    Pools.NonPagedPoolPtr.Initialize1(0);

    // We PDE-aligned the nonpaged system start VA, so haul some extra PTEs!
    PointerPte = PTENTRY::AddressToPte((ULONG_PTR)SystemPtes.NonPagedSystemStart);
    SystemPtes.SetSystemPtesNumber(PTENTRY::AddressToPte(Pools.NonPagedPoolPtr.Start) - PointerPte - 1);
    ASSERT(LargePagesMapped == FALSE);
    DPRINT("Final System PTE count: %d (%d bytes)\n", SystemPtes.GetSystemPtesNumber(), SystemPtes.GetSystemPtesNumber() * PAGE_SIZE);

    // Create the system PTE space
    SystemPtes.Initialize(PointerPte, SystemPtes.GetSystemPtesNumber(), SystemPteSpace);

    // Get the PDE for hyperspace
    StartPde = PTENTRY::AddressToPde(HYPER_SPACE);

    // Allocate a page for hyperspace and create it
    TempPte = ValidKernelPde;
    PageFrameIndex = PfnDb.RemoveAnyPage(0);
    TempPte.SetPfn(PageFrameIndex);
    StartPde->WriteValidPte(&TempPte);
    DPRINT1("Hyperspace PDE page 0x%x (PDE %p)\n", PageFrameIndex, StartPde);

    // Flush the TLB
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeFlushCurrentTb();
    KeLowerIrql(OldIrql);

    // Zero out the page table now
    PointerPte = PTENTRY::AddressToPte(HYPER_SPACE);
    RtlZeroMemory(PointerPte, PAGE_SIZE);
    DPRINT1("Hyperspace PTE page 0x%x (PTE %p)\n", PointerPte->GetPfn(), PointerPte);

    // Setup the mapping PTEs
    FirstReservedMappingPte = PTENTRY::AddressToPte(MI_MAPPING_RANGE_START);
    LastReservedMappingPte = PTENTRY::AddressToPte(MI_MAPPING_RANGE_END);
    //FirstReservedMappingPte->SetPfn(MI_HYPERSPACE_PTES);

    // Set the working set address
    WorkingSetList = (WORKING_SET_LIST *)MI_WORKING_SET_LIST;
    Wsle = (WS_LIST_ENTRY *)((PCHAR)WorkingSetList + sizeof(WORKING_SET_LIST));

    // Initialize PDE page reference and share counters so that InitializeProcessAddressSpace works
    PMMPFN PdePfn = PfnDb.GetEntry(PTENTRY::AddressToPde(PDE_BASE)->GetPfn());
    PdePfn->u3.e2.ReferenceCount = 0;
    PdePfn->u2.ShareCount = 0;

    // Get a page for the working set list
    PageFrameIndex = PfnDb.RemoveAnyPage(0);
    TempPte.SetPfn(PageFrameIndex);

    // Map the working set list
    PointerPte = PTENTRY::AddressToPte((ULONG_PTR)WorkingSetList);
    PointerPte->WriteValidPte(&TempPte);
    DPRINT1("WSL %p (PTE %p), Page 0x%x\n", WorkingSetList, PointerPte, PageFrameIndex);

    // Flush the TLB
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeFlushCurrentTb();
    KeLowerIrql(OldIrql);

    // Zero it out, and save the frame index
    RtlZeroMemory(PointerPte->PteToAddress(), PAGE_SIZE);
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    CurrentProcess->WorkingSetPage = PageFrameIndex;

    // Check for Pentium LOCK errata
    if (KiI386PentiumLockErrataPresent)
    {
        /* Mark the 1st IDT page as Write-Through to prevent a lockup
           on a F00F instruction.
           See http://www.rcollins.org/Errata/Dec97/F00FBug.html */
        PointerPte = PTENTRY::AddressToPte((ULONG_PTR)KeGetPcr()->IDT);
        PointerPte->u.Hard.WriteThrough = 1;
    }

    // Working set size of a system process
    // FIXME: Move into config storage
    CurrentProcess->Vm.MinimumWorkingSetSize = 50;
    CurrentProcess->Vm.MaximumWorkingSetSize = 500;

    // Initialize the bogus address space
    ULONG Flags = 0;
    InitializeProcessAddressSpace(CurrentProcess, NULL, NULL, &Flags, NULL);

    // Mark all BIOS pages as in use
    PMMPFN Pfn;
    for (Pfn = PfnDb.GetEntry(0xA000); Pfn <= PfnDb.GetEntry(0xFFFF); Pfn++)
    {
        if (!Pfn->PteAddress &&
            Pfn->u2.ShareCount == 0 &&
            Pfn->u3.e2.ReferenceCount == 0)
        {
            DPRINT1("Marking BIOS page in PFN as used!\n");
            Pfn->u3.e1.PageLocation = ActiveAndValid;
            Pfn->u3.e1.PageColor = 0;
            Pfn->u3.e2.ReferenceCount = 1;
            Pfn->PteAddress = (PTENTRY *)((ULONG_PTR)KSEG0_BASE - 1);
        }
    }

    DPRINT("Done\n");
}

VOID
MEMORY_MANAGER::
MachineDependentInit1(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
}

BOOLEAN
MEMORY_MANAGER::
IsAddressValid(IN PVOID Ptr)
{
    // Check if the PDE is valid
    PTENTRY *Pde = PTENTRY::AddressToPde((ULONG_PTR)Ptr);
    if (!Pde->IsValid()) return FALSE;

    // Check if the PTE is valid (don't check if it's a large page)
    PTENTRY *Pte = PTENTRY::AddressToPte((ULONG_PTR)Ptr);
    if (!Pte->IsValid() && !Pte->IsLargePage()) return FALSE;

    // This address is valid now, but it will only stay so if the caller holds
    // the PFN lock
    return TRUE;

}

// Pools machine dependent implementation
VOID
NONPAGED_POOL::
ComputeVirtualAddresses(PFN_NUMBER FreePages)
{
    PFN_NUMBER PoolPages;

    // Set initial end value
    End = 0xFFBE0000;

    /* Check if this is a machine with less than 256MB of RAM, and no overide */
    if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) && !SizeInBytes)
    {
        /* Force the non paged pool to be 2MB so we can reduce RAM usage */
        SizeInBytes = 2 * _1MB;
    }

    /* Hyperspace ends here */
    //MmHyperSpaceEnd = (PVOID)((ULONG_PTR)MmSystemCacheWorkingSetList - 1);

    /* Check if the user gave a ridicuously large nonpaged pool RAM size */
    if ((SizeInBytes >> PAGE_SHIFT) > (FreePages * 7 / 8))
    {
        /* More than 7/8ths of RAM was dedicated to nonpaged pool, ignore! */
        SizeInBytes = 0;
    }

    /* Check if no registry setting was set, or if the setting was too low */
    if (SizeInBytes < MinInBytes)
    {
        /* Start with the minimum (256 KB) and add 32 KB for each MB above 4 */
        SizeInBytes = MinInBytes;
        SizeInBytes += (FreePages - 1024) / 256 * 32 * 1024;
    }

    /* Check if the registy setting or our dynamic calculation was too high */
    if (SizeInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
    {
        /* Set it to the maximum */
        SizeInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
    }

    /* Page-align the nonpaged pool size */
    SizeInBytes &= ~(PAGE_SIZE - 1);

    /* Now, check if there was a registry size for the maximum size */
    if (!MaxInBytes)
    {
        /* Start with the default (1MB) */
        MaxInBytes = _1MB;

        /* Add space for PFN database */
        MaxInBytes += (ULONG)PAGE_ALIGN((MemoryManager->GetHighestPhysicalPage() +  1) * sizeof(MMPFN));

        /* Check if the machine has more than 512MB of free RAM */
        if (FreePages >= 0x1F000)
        {
            /* Add 200KB for each MB above 4 */
            MaxInBytes += (FreePages - 1024) / 256 * (400 * 1024 / 2);
            if (MaxInBytes < MI_MAX_NONPAGED_POOL_SIZE)
            {
                /* Make it at least 128MB since this machine has a lot of RAM */
                MaxInBytes = MI_MAX_NONPAGED_POOL_SIZE;
            }
        }
        else
        {
            /* Add 400KB for each MB above 4 */
            MaxInBytes += (FreePages - 1024) / 256 * 400 * 1024;
        }
    }

    /* Make sure there's at least 16 pages + the PFN available for expansion */
    PoolPages = SizeInBytes + (PAGE_SIZE * 16) +
                ((ULONG)PAGE_ALIGN(MemoryManager->GetHighestPhysicalPage() + 1) * sizeof(MMPFN));
    if (MaxInBytes < PoolPages)
    {
        /* The maximum should be at least high enough to cover all the above */
        MaxInBytes = PoolPages;
    }

    /* Systems with 2GB of kernel address space get double the size */
    PoolPages = MI_MAX_NONPAGED_POOL_SIZE * 2;

    /* On the other hand, make sure that PFN + nonpaged pool doesn't get too big */
    if (MaxInBytes > PoolPages)
    {
        /* Trim it down to the maximum architectural limit (256MB) */
        MaxInBytes = PoolPages;
    }

    /* Check if this is a system with > 128MB of non paged pool */
    if (MaxInBytes > MI_MAX_NONPAGED_POOL_SIZE)
    {
        /* Check if the initial size is less than the extra 128MB boost */
        if (SizeInBytes < (MaxInBytes - MI_MAX_NONPAGED_POOL_SIZE))
        {
            /* FIXME: Should check if the initial pool can be expanded */

            /* Assume no expansion possible, check ift he maximum is too large */
            if (MaxInBytes > (SizeInBytes + MI_MAX_NONPAGED_POOL_SIZE))
            {
                /* Set it to the initial value plus the boost */
                MaxInBytes = SizeInBytes + MI_MAX_NONPAGED_POOL_SIZE;
            }
        }
    }
}

PFN_NUMBER
PFN_DATABASE::
GetIndexFromPhysical(ULONG_PTR Addr)
{
    return (Addr << 3) >> 15;
}

