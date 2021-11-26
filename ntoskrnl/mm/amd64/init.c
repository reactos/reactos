/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/init.c
 * PURPOSE:         Memory Manager Initialization for amd64
 *
 * PROGRAMMERS:     Timo kreuzer (timo.kreuzer@reactos.org)
 *                  ReactOS Portable Systems Group
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

#include <mm/ARM3/miarm.h>
#include <fltkernel.h>

extern PMMPTE MmDebugPte;

/* Helper macros */
#define IS_PAGE_ALIGNED(addr) IS_ALIGNED(addr, PAGE_SIZE)

/* GLOBALS *****************************************************************/

/* Template PTE and PDE for a kernel page */
MMPTE ValidKernelPde = {{PTE_VALID|PTE_EXECUTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};
MMPTE ValidKernelPte = {{PTE_VALID|PTE_EXECUTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};

/* The same, but for local pages */
MMPTE ValidKernelPdeLocal = {{PTE_VALID|PTE_EXECUTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};
MMPTE ValidKernelPteLocal = {{PTE_VALID|PTE_EXECUTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};

/* Template PDE for a demand-zero page */
MMPDE DemandZeroPde  = {{MM_EXECUTE_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};
MMPTE DemandZeroPte  = {{MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};

/* Template PTE for prototype page */
MMPTE PrototypePte = {{(MM_EXECUTE_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS) |
                      PTE_PROTOTYPE | (MI_PTE_LOOKUP_NEEDED << 32)}};

/* Template PTE for decommited page */
MMPTE MmDecommittedPte = {{MM_DECOMMIT << MM_PTE_SOFTWARE_PROTECTION_BITS}};

/* Address ranges */
PVOID MiSessionViewEnd;
PVOID MiSystemPteSpaceStart;
PVOID MiSystemPteSpaceEnd;

ULONG64 MxPfnSizeInBytes;
BOOLEAN MiIncludeType[LoaderMaximum];
PFN_NUMBER MxFreePageBase;
ULONG64 MxFreePageCount = 0;

BOOLEAN MiPfnsInitialized = FALSE;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeSessionSpaceLayout(VOID)
{
    /* This is the entire size */
    MmSessionSize = MI_SESSION_SIZE;

    /* Start with session space end */
    MiSessionSpaceEnd = (PVOID)MI_SESSION_SPACE_END;

    /* The highest range is the session image range */
    MmSessionImageSize = MI_SESSION_IMAGE_SIZE;
    MiSessionImageEnd = MiSessionSpaceEnd;
    MiSessionImageStart = (PUCHAR)MiSessionImageEnd - MmSessionImageSize;
    ASSERT(IS_PAGE_ALIGNED(MiSessionImageStart));

    /* Session working set is below the session image range */
    MiSessionSpaceWs = (PUCHAR)MiSessionImageStart - MI_SESSION_WORKING_SET_SIZE;

    /* Session view is below the session working set */
    MmSessionViewSize = MI_SESSION_VIEW_SIZE;
    MiSessionViewEnd = MiSessionSpaceWs;
    MiSessionViewStart = (PUCHAR)MiSessionViewEnd - MmSessionViewSize;
    ASSERT(IS_PAGE_ALIGNED(MiSessionViewStart));

    /* Session pool is below session view */
    MmSessionPoolSize = MI_SESSION_POOL_SIZE;
    MiSessionPoolEnd = MiSessionViewStart;
    MiSessionPoolStart = (PUCHAR)MiSessionPoolEnd - MmSessionPoolSize;
    ASSERT(IS_PAGE_ALIGNED(MiSessionPoolStart));

    /* And it all begins here */
    MmSessionBase = MiSessionPoolStart;

    /* System view space ends at session space, so now that we know where
     * this is, we can compute the base address of system view space itself. */
    MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;
    MiSystemViewStart = (PUCHAR)MmSessionBase - MmSystemViewSize;
    ASSERT(IS_PAGE_ALIGNED(MiSystemViewStart));

    /* Sanity checks */
    ASSERT(Add2Ptr(MmSessionBase, MmSessionSize) == MiSessionSpaceEnd);
    ASSERT(MiSessionViewEnd <= MiSessionImageStart);
    ASSERT(MmSessionBase <= MiSessionPoolStart);

    /* Compute the PTE addresses for all the addresses we carved out */
    MiSessionImagePteStart = MiAddressToPte(MiSessionImageStart);
    MiSessionImagePteEnd = MiAddressToPte(MiSessionImageEnd);
    MiSessionBasePte = MiAddressToPte(MmSessionBase);
    MiSessionLastPte = MiAddressToPte(MiSessionSpaceEnd);

    /* Initialize the pointer to the session space structure */
    MmSessionSpace = (PMM_SESSION_SPACE)Add2Ptr(MiSessionImageStart, 0x10000);
}

VOID
NTAPI
MiMapPPEs(
    PVOID StartAddress,
    PVOID EndAddress)
{
    PMMPDE PointerPpe;
    MMPDE TmplPde = ValidKernelPde;

    /* Loop the PPEs */
    for (PointerPpe = MiAddressToPpe(StartAddress);
         PointerPpe <= MiAddressToPpe(EndAddress);
         PointerPpe++)
    {
        /* Check if its already mapped */
        if (!PointerPpe->u.Hard.Valid)
        {
            /* No, map it! */
            TmplPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
            MI_WRITE_VALID_PTE(PointerPpe, TmplPde);

            /* Zero out the page table */
            RtlZeroMemory(MiPteToAddress(PointerPpe), PAGE_SIZE);
        }
    }
}

VOID
NTAPI
MiMapPDEs(
    PVOID StartAddress,
    PVOID EndAddress)
{
    PMMPDE PointerPde;
    MMPDE TmplPde = ValidKernelPde;

    /* Loop the PDEs */
    for (PointerPde = MiAddressToPde(StartAddress);
         PointerPde <= MiAddressToPde(EndAddress);
         PointerPde++)
    {
        /* Check if its already mapped */
        if (!PointerPde->u.Hard.Valid)
        {
            /* No, map it! */
            TmplPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
            MI_WRITE_VALID_PTE(PointerPde, TmplPde);

            /* Zero out the page table */
            RtlZeroMemory(MiPteToAddress(PointerPde), PAGE_SIZE);
        }
    }
}

VOID
NTAPI
MiMapPTEs(
    PVOID StartAddress,
    PVOID EndAddress)
{
    PMMPTE PointerPte;
    MMPTE TmplPte = ValidKernelPte;

    /* Loop the PTEs */
    for (PointerPte = MiAddressToPte(StartAddress);
         PointerPte <= MiAddressToPte(EndAddress);
         PointerPte++)
    {
        /* Check if its already mapped */
        if (!PointerPte->u.Hard.Valid)
        {
            /* No, map it! */
            TmplPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
            MI_WRITE_VALID_PTE(PointerPte, TmplPte);

            /* Zero out the page (FIXME: not always neccessary) */
            RtlZeroMemory(MiPteToAddress(PointerPte), PAGE_SIZE);
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiInitializePageTable(VOID)
{
    ULONG64 PxePhysicalAddress;
    MMPTE TmplPte, *PointerPxe;
    PFN_NUMBER PxePfn;

    /* Get current directory base */
    PxePfn = ((PMMPTE)PXE_SELFMAP)->u.Hard.PageFrameNumber;
    PxePhysicalAddress = PxePfn << PAGE_SHIFT;
    ASSERT(PxePhysicalAddress == __readcr3());

    /* Set directory base for the system process */
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PxePhysicalAddress;

    /* Enable global pages */
    __writecr4(__readcr4() | CR4_PGE);
    ASSERT(__readcr4() & CR4_PGE);

    /* Loop the user mode PXEs */
    for (PointerPxe = MiAddressToPxe(0);
         PointerPxe <= MiAddressToPxe(MmHighestUserAddress);
         PointerPxe++)
    {
        /* Zero the PXE, clear all mappings */
        PointerPxe->u.Long = 0;
    }

    /* Flush the TLB */
    KeFlushCurrentTb();

    /* Set up a template PTE */
    TmplPte.u.Long = 0;
    TmplPte.u.Flush.Valid = 1;
    TmplPte.u.Flush.Write = 1;
    HyperTemplatePte = TmplPte;

    /* Create PDPTs (72 KB) for shared system address space,
     * skip page tables TODO: use global pages. */

    /* Loop the PXEs */
    for (PointerPxe = MiAddressToPxe((PVOID)HYPER_SPACE);
         PointerPxe <= MiAddressToPxe(MI_HIGHEST_SYSTEM_ADDRESS);
         PointerPxe++)
    {
        /* Is the PXE already valid? */
        if (!PointerPxe->u.Hard.Valid)
        {
            /* It's not Initialize it */
            TmplPte.u.Flush.PageFrameNumber = MxGetNextPage(1);
            *PointerPxe = TmplPte;

            /* Zero the page. The PXE is the PTE for the PDPT. */
            RtlZeroMemory(MiPteToAddress(PointerPxe), PAGE_SIZE);
        }
    }
    PxePfn = PFN_FROM_PXE(MiAddressToPxe((PVOID)HYPER_SPACE));
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[1] = PxePfn << PAGE_SHIFT;

    /* Map PPEs for paged pool */
    MiMapPPEs(MmPagedPoolStart, MmPagedPoolEnd);

    /* Setup 1 PPE for hyper space */
    MiMapPPEs((PVOID)HYPER_SPACE, (PVOID)HYPER_SPACE_END);

    /* Setup PPEs for system space view */
    MiMapPPEs(MiSystemViewStart, (PCHAR)MiSystemViewStart + MmSystemViewSize);

    /* Setup the mapping PDEs */
    MiMapPDEs((PVOID)MI_MAPPING_RANGE_START, (PVOID)MI_MAPPING_RANGE_END);

    /* Setup the mapping PTEs */
    MmFirstReservedMappingPte = MiAddressToPte((PVOID)MI_MAPPING_RANGE_START);
    MmLastReservedMappingPte = MiAddressToPte((PVOID)MI_MAPPING_RANGE_END);
    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;

    /* Setup debug mapping PTE */
    MiMapPPEs((PVOID)MI_DEBUG_MAPPING, (PVOID)MI_DEBUG_MAPPING);
    MiMapPDEs((PVOID)MI_DEBUG_MAPPING, (PVOID)MI_DEBUG_MAPPING);
    MmDebugPte = MiAddressToPte((PVOID)MI_DEBUG_MAPPING);

    /* Setup PDE and PTEs for VAD bitmap and working set list */
    MiMapPDEs((PVOID)MI_VAD_BITMAP, (PVOID)(MI_WORKING_SET_LIST + PAGE_SIZE - 1));
    MiMapPTEs((PVOID)MI_VAD_BITMAP, (PVOID)(MI_WORKING_SET_LIST + PAGE_SIZE - 1));
}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildNonPagedPool(VOID)
{
    /* Check if this is a machine with less than 256MB of RAM, and no overide */
    if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) &&
        !(MmSizeOfNonPagedPoolInBytes))
    {
        /* Force the non paged pool to be 2MB so we can reduce RAM usage */
        MmSizeOfNonPagedPoolInBytes = 2 * 1024 * 1024;
    }

    /* Check if the user gave a ridicuously large nonpaged pool RAM size */
    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
        (MmNumberOfPhysicalPages * 7 / 8))
    {
        /* More than 7/8ths of RAM was dedicated to nonpaged pool, ignore! */
        MmSizeOfNonPagedPoolInBytes = 0;
    }

    /* Check if no registry setting was set, or if the setting was too low */
    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize)
    {
        /* Start with the minimum (256 KB) and add 32 KB for each MB above 4 */
        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
        MmSizeOfNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                       256 * MmMinAdditionNonPagedPoolPerMb;
    }

    /* Check if the registy setting or our dynamic calculation was too high */
    if (MmSizeOfNonPagedPoolInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
    {
        /* Set it to the maximum */
        MmSizeOfNonPagedPoolInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
    }

    /* Check if a percentage cap was set through the registry */
    if (MmMaximumNonPagedPoolPercent)
    {
        /* Don't feel like supporting this right now */
        UNIMPLEMENTED;
    }

    /* Page-align the nonpaged pool size */
    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    /* Now, check if there was a registry size for the maximum size */
    if (!MmMaximumNonPagedPoolInBytes)
    {
        /* Start with the default (1MB) and add 400 KB for each MB above 4 */
        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;
        MmMaximumNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                         256 * MmMaxAdditionNonPagedPoolPerMb;
    }

    /* Don't let the maximum go too high */
    if (MmMaximumNonPagedPoolInBytes > MI_MAX_NONPAGED_POOL_SIZE)
    {
        /* Set it to the upper limit */
        MmMaximumNonPagedPoolInBytes = MI_MAX_NONPAGED_POOL_SIZE;
    }

    /* Convert nonpaged pool size from bytes to pages */
    MmMaximumNonPagedPoolInPages = MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT;

    /* Non paged pool starts after the PFN database */
    MmNonPagedPoolStart = MmPfnDatabase + MxPfnAllocation * PAGE_SIZE;

    /* Calculate the nonpaged pool expansion start region */
    MmNonPagedPoolExpansionStart = (PCHAR)MmNonPagedPoolStart +
                                          MmSizeOfNonPagedPoolInBytes;
    ASSERT(IS_PAGE_ALIGNED(MmNonPagedPoolExpansionStart));

    /* And this is where the none paged pool ends */
    MmNonPagedPoolEnd = (PCHAR)MmNonPagedPoolStart + MmMaximumNonPagedPoolInBytes;
    ASSERT(MmNonPagedPoolEnd < (PVOID)MM_HAL_VA_START);

    /* Map PPEs and PDEs for non paged pool (including expansion) */
    MiMapPPEs(MmNonPagedPoolStart, MmNonPagedPoolEnd);
    MiMapPDEs(MmNonPagedPoolStart, MmNonPagedPoolEnd);

    /* Map the nonpaged pool PTEs (without expansion) */
    MiMapPTEs(MmNonPagedPoolStart, (PCHAR)MmNonPagedPoolExpansionStart - 1);

    /* Initialize the ARM3 nonpaged pool */
    MiInitializeNonPagedPool();
    MiInitializeNonPagedPoolThresholds();

}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildSystemPteSpace(VOID)
{
    PMMPTE PointerPte;
    SIZE_T NonPagedSystemSize;

    /* Use the default number of system PTEs */
    MmNumberOfSystemPtes = MI_NUMBER_SYSTEM_PTES;
    NonPagedSystemSize = (MmNumberOfSystemPtes + 1) * PAGE_SIZE;

    /* Put system PTEs at the start of the system VA space */
    MiSystemPteSpaceStart = MmNonPagedSystemStart;
    MiSystemPteSpaceEnd = (PUCHAR)MiSystemPteSpaceStart + NonPagedSystemSize;

    /* Map the PPEs and PDEs for the system PTEs */
    MiMapPPEs(MiSystemPteSpaceStart, MiSystemPteSpaceEnd);
    MiMapPDEs(MiSystemPteSpaceStart, MiSystemPteSpaceEnd);

    /* Initialize the system PTE space */
    PointerPte = MiAddressToPte(MiSystemPteSpaceStart);
    MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    /* Reserve system PTEs for zeroing PTEs and clear them */
    MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES + 1,
                                                    SystemPteSpace);
    RtlZeroMemory(MiFirstReservedZeroingPte, (MI_ZERO_PTES + 1) * sizeof(MMPTE));

    /* Set the counter to maximum */
    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES;
}

static
VOID
MiSetupPfnForPageTable(
    PFN_NUMBER PageFrameIndex,
    PMMPTE PointerPte)
{
    PMMPFN Pfn;
    PMMPDE PointerPde;

    /* Get the pfn entry for this page */
    Pfn = MiGetPfnEntry(PageFrameIndex);

    /* Check if it's valid memory */
    if ((PageFrameIndex <= MmHighestPhysicalPage) &&
        (MmIsAddressValid(Pfn)) &&
        (Pfn->u3.e1.PageLocation == ActiveAndValid))
    {
        /* Setup the PFN entry */
        Pfn->u1.WsIndex = 0;
        Pfn->u2.ShareCount++;
        Pfn->PteAddress = PointerPte;
        Pfn->OriginalPte = *PointerPte;
        Pfn->u3.e1.PageLocation = ActiveAndValid;
        Pfn->u3.e1.CacheAttribute = MiNonCached;
        Pfn->u3.e2.ReferenceCount = 1;
        Pfn->u4.PteFrame = PFN_FROM_PTE(MiAddressToPte(PointerPte));
    }

    /* Increase the shared count of the PFN entry for the PDE */
    PointerPde = MiAddressToPde(MiPteToAddress(PointerPte));
    Pfn = MiGetPfnEntry(PFN_FROM_PTE(PointerPde));
    Pfn->u2.ShareCount++;
}

CODE_SEG("INIT")
static
VOID
MiBuildPfnDatabaseFromPageTables(VOID)
{
    PVOID Address = NULL;
    PFN_NUMBER PageFrameIndex;
    PMMPDE PointerPde;
    PMMPTE PointerPte;
    ULONG k, l;
    PMMPFN Pfn;
#if (_MI_PAGING_LEVELS >= 3)
    PMMPDE PointerPpe;
    ULONG j;
#endif
#if (_MI_PAGING_LEVELS == 4)
    PMMPDE PointerPxe;
    ULONG i;
#endif

    /* Manual setup of the top level page directory */
#if (_MI_PAGING_LEVELS == 4)
    PageFrameIndex = PFN_FROM_PTE(MiAddressToPte(PXE_BASE));
#elif (_MI_PAGING_LEVELS == 3)
    PageFrameIndex = PFN_FROM_PTE(MiAddressToPte(PPE_BASE));
#else
    PageFrameIndex = PFN_FROM_PTE(MiAddressToPte(PDE_BASE));
#endif
    Pfn = MiGetPfnEntry(PageFrameIndex);
    ASSERT(Pfn->u3.e1.PageLocation == ActiveAndValid);
    Pfn->u1.WsIndex = 0;
    Pfn->u2.ShareCount = 1;
    Pfn->PteAddress = NULL;
    Pfn->u3.e1.CacheAttribute = MiNonCached;
    Pfn->u3.e2.ReferenceCount = 1;
    Pfn->u4.PteFrame = 0;

#if (_MI_PAGING_LEVELS == 4)
    /* Loop all PXEs in the PML4 */
    PointerPxe = MiAddressToPxe(Address);
    for (i = 0; i < PXE_PER_PAGE; i++, PointerPxe++)
    {
        /* Skip invalid PXEs */
        if (!PointerPxe->u.Hard.Valid) continue;

        /* Handle the PFN */
        PageFrameIndex = PFN_FROM_PXE(PointerPxe);
        MiSetupPfnForPageTable(PageFrameIndex, PointerPxe);

        /* Get starting VA for this PXE */
        Address = MiPxeToAddress(PointerPxe);
#endif
#if (_MI_PAGING_LEVELS >= 3)
        /* Loop all PPEs in this PDP */
        PointerPpe = MiAddressToPpe(Address);
        for (j = 0; j < PPE_PER_PAGE; j++, PointerPpe++)
        {
            /* Skip invalid PPEs */
            if (!PointerPpe->u.Hard.Valid) continue;

            /* Handle the PFN */
            PageFrameIndex = PFN_FROM_PPE(PointerPpe);
            MiSetupPfnForPageTable(PageFrameIndex, PointerPpe);

            /* Get starting VA for this PPE */
            Address = MiPpeToAddress(PointerPpe);
#endif
            /* Loop all PDEs in this PD */
            PointerPde = MiAddressToPde(Address);
            for (k = 0; k < PDE_PER_PAGE; k++, PointerPde++)
            {
                /* Skip invalid PDEs */
                if (!PointerPde->u.Hard.Valid) continue;

                /* Handle the PFN */
                PageFrameIndex = PFN_FROM_PDE(PointerPde);
                MiSetupPfnForPageTable(PageFrameIndex, PointerPde);

                /* Get starting VA for this PDE */
                Address = MiPdeToAddress(PointerPde);

                /* Loop all PTEs in this PT */
                PointerPte = MiAddressToPte(Address);
                for (l = 0; l < PTE_PER_PAGE; l++, PointerPte++)
                {
                    /* Skip invalid PTEs */
                    if (!PointerPte->u.Hard.Valid) continue;

                    /* Handle the PFN */
                    PageFrameIndex = PFN_FROM_PTE(PointerPte);
                    MiSetupPfnForPageTable(PageFrameIndex, PointerPte);
                }
            }
#if (_MI_PAGING_LEVELS >= 3)
        }
#endif
#if (_MI_PAGING_LEVELS == 4)
    }
#endif
}

CODE_SEG("INIT")
static
VOID
MiAddDescriptorToDatabase(
    PFN_NUMBER BasePage,
    PFN_NUMBER PageCount,
    TYPE_OF_MEMORY MemoryType)
{
    PMMPFN Pfn;

    ASSERT(!MiIsMemoryTypeInvisible(MemoryType));

    /* Check if the memory is free */
    if (MiIsMemoryTypeFree(MemoryType))
    {
        /* Get the last pfn of this descriptor. Note we loop backwards */
        Pfn = &MmPfnDatabase[BasePage + PageCount - 1];

        /* Loop all pages */
        while (PageCount--)
        {
            /* Add it to the free list */
            Pfn->u3.e1.CacheAttribute = MiNonCached; // FIXME: Windows ASSERTs MiChached, but why not MiNotMapped?
            MiInsertPageInFreeList(BasePage + PageCount);

            /* Go to the previous page */
            Pfn--;
        }
    }
    else if (MemoryType == LoaderXIPRom)
    {
        Pfn = &MmPfnDatabase[BasePage];
        while (PageCount--)
        {
            /* Make it a pseudo-I/O ROM mapping */
            Pfn->PteAddress = 0;
            Pfn->u1.Flink = 0;
            Pfn->u2.ShareCount = 0;
            Pfn->u3.e1.PageLocation = 0;
            Pfn->u3.e1.CacheAttribute = MiNonCached;
            Pfn->u3.e1.Rom = 1;
            Pfn->u3.e1.PrototypePte = 1;
            Pfn->u3.e2.ReferenceCount = 0;
            Pfn->u4.InPageError = 0;
            Pfn->u4.PteFrame = 0;

            /* Advance one */
            Pfn++;
        }
    }
    else if (MemoryType == LoaderBad)
    {
        // FIXME: later
        ASSERT(FALSE);
    }
    else
    {
        /* For now skip it */
        Pfn = &MmPfnDatabase[BasePage];
        while (PageCount--)
        {
            /* Make an active PFN */
            Pfn->u3.e1.PageLocation = ActiveAndValid;

            /* Advance one */
            Pfn++;
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPfnDatabase(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PFN_NUMBER BasePage, PageCount;
    KIRQL OldIrql;

    /* Lock the PFN Database */
    OldIrql = MiAcquirePfnLock();

    /* Map the PDEs and PPEs for the pfn database (ignore holes) */
#if (_MI_PAGING_LEVELS >= 3)
    MiMapPPEs(MmPfnDatabase, (PUCHAR)MmPfnDatabase + (MxPfnAllocation * PAGE_SIZE) - 1);
#endif
    MiMapPDEs(MmPfnDatabase, (PUCHAR)MmPfnDatabase + (MxPfnAllocation * PAGE_SIZE) - 1);

    /* First initialize the color tables */
    MiInitializeColorTables();

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Skip invisible memory */
        if (MiIsMemoryTypeInvisible(Descriptor->MemoryType)) continue;

        /* If this is the free descriptor, use the copy instead */
        if (Descriptor == MxFreeDescriptor) Descriptor = &MxOldFreeDescriptor;

        /* Get the range for this descriptor */
        BasePage = Descriptor->BasePage;
        PageCount = Descriptor->PageCount;

        /* Map the pages for the database */
        MiMapPTEs(&MmPfnDatabase[BasePage],
                  (PUCHAR)(&MmPfnDatabase[BasePage + PageCount]) - 1);

        /* If this was the free descriptor, skip the next step */
        if (Descriptor == &MxOldFreeDescriptor) continue;

        /* Add this descriptor to the database */
        MiAddDescriptorToDatabase(BasePage, PageCount, Descriptor->MemoryType);
    }

    /* At this point the whole pfn database is mapped. We are about to add the
       pages from the free descriptor to the database, so from now on we cannot
       use it anymore. */

    /* Now add the free descriptor */
    BasePage = MxFreeDescriptor->BasePage;
    PageCount = MxFreeDescriptor->PageCount;
    MiAddDescriptorToDatabase(BasePage, PageCount, LoaderFree);

    /* And finally the memory we used */
    BasePage = MxOldFreeDescriptor.BasePage;
    PageCount = MxFreeDescriptor->BasePage - BasePage;
    MiAddDescriptorToDatabase(BasePage, PageCount, LoaderMemoryData);

    /* Reset the descriptor back so we can create the correct memory blocks */
    *MxFreeDescriptor = MxOldFreeDescriptor;

    /* Now process the page tables */
    MiBuildPfnDatabaseFromPageTables();

    /* PFNs are initialized now! */
    MiPfnsInitialized = TRUE;

    /* Release PFN database */
    MiReleasePfnLock(OldIrql);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MiInitMachineDependent(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    ULONG Flags;

    ASSERT(MxPfnAllocation != 0);

    /* Set some hardcoded addresses */
    MmHyperSpaceEnd = (PVOID)HYPER_SPACE_END;
    MmNonPagedSystemStart = (PVOID)MM_SYSTEM_SPACE_START;
    MmPfnDatabase = (PVOID)MI_PFN_DATABASE;
    MmWorkingSetList = (PVOID)MI_WORKING_SET_LIST;


//    PrototypePte.u.Proto.Valid = 1
//    PrototypePte.u.ReadOnly
//    PrototypePte.u.Prototype
//    PrototypePte.u.Protection = MM_READWRITE;
//    PrototypePte.u.ProtoAddress
    PrototypePte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;

    MiInitializePageTable();

    MiBuildNonPagedPool();

    MiBuildSystemPteSpace();

    /* Map the PFN database pages */
    MiBuildPfnDatabase(LoaderBlock);

    /* Reset the ref/share count so that MmInitializeProcessAddressSpace works */
    PMMPFN Pfn = MiGetPfnEntry(PFN_FROM_PTE((PMMPTE)PXE_SELFMAP));
    Pfn->u2.ShareCount = 0;
    Pfn->u3.e2.ReferenceCount = 0;

    Pfn = MiGetPfnEntry(PFN_FROM_PDE(MiAddressToPde((PVOID)HYPER_SPACE)));
    Pfn->u2.ShareCount = 0;
    Pfn->u3.e2.ReferenceCount = 0;

    Pfn = MiGetPfnEntry(PFN_FROM_PPE(MiAddressToPpe((PVOID)HYPER_SPACE)));
    Pfn->u2.ShareCount = 0;
    Pfn->u3.e2.ReferenceCount = 0;

    Pfn = MiGetPfnEntry(PFN_FROM_PXE(MiAddressToPxe((PVOID)HYPER_SPACE)));
    Pfn->u2.ShareCount = 0;
    Pfn->u3.e2.ReferenceCount = 0;

    Pfn = MiGetPfnEntry(PFN_FROM_PTE(MiAddressToPte(MmWorkingSetList)));
    Pfn->u2.ShareCount = 0;
    Pfn->u3.e2.ReferenceCount = 0;

    /* Initialize the nonpaged pool */
    InitializePool(NonPagedPool, 0);

    /* Initialize the bogus address space */
    Flags = 0;
    Status = MmInitializeProcessAddressSpace(PsGetCurrentProcess(), NULL, NULL, &Flags, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmInitializeProcessAddressSpace(9 failed: 0x%lx\n", Status);
        return Status;
    }

    /* Initialize the balancer */
    MmInitializeBalancer((ULONG)MmAvailablePages, 0);

    /* Make sure we have everything we need */
    ASSERT(MmPfnDatabase);
    ASSERT(MmNonPagedSystemStart);
    ASSERT(MmNonPagedPoolStart);
    ASSERT(MmSizeOfNonPagedPoolInBytes);
    ASSERT(MmMaximumNonPagedPoolInBytes);
    ASSERT(MmNonPagedPoolExpansionStart);
    ASSERT(MmHyperSpaceEnd);
    ASSERT(MmNumberOfSystemPtes);
    ASSERT(MiAddressToPde(MmNonPagedPoolStart)->u.Hard.Valid);
    ASSERT(MiAddressToPte(MmNonPagedPoolStart)->u.Hard.Valid);

    return STATUS_SUCCESS;
}
