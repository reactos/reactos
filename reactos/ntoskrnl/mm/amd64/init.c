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

#include "../ARM3/miarm.h"

#ifdef _WINKD_
extern PMMPTE MmDebugPte;
#endif

/* GLOBALS *****************************************************************/

/* Template PTE and PDE for a kernel page */
MMPTE ValidKernelPde = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};
MMPTE ValidKernelPte = {{PTE_VALID|PTE_READWRITE|PTE_DIRTY|PTE_ACCESSED}};

/* Template PDE for a demand-zero page */
MMPDE DemandZeroPde  = {{MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};
MMPTE DemandZeroPte  = {{MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS}};

/* Template PTE for prototype page */
MMPTE PrototypePte = {{(MM_READWRITE << MM_PTE_SOFTWARE_PROTECTION_BITS) |
                      PTE_PROTOTYPE | (MI_PTE_LOOKUP_NEEDED << PAGE_SHIFT)}};

/* Sizes */
SIZE_T MiNonPagedSystemSize;

/* Address ranges */
PVOID MiSessionViewEnd;

ULONG64 MxPfnSizeInBytes;
BOOLEAN MiIncludeType[LoaderMaximum];
PFN_NUMBER MxFreePageBase;
ULONG64 MxFreePageCount = 0;

BOOLEAN MiPfnsInitialized = FALSE;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
INIT_FUNCTION
MiInitializeSessionSpaceLayout()
{
    MmSessionSize = MI_SESSION_SIZE;
    MmSessionViewSize = MI_SESSION_VIEW_SIZE;
    MmSessionPoolSize = MI_SESSION_POOL_SIZE;
    MmSessionImageSize = MI_SESSION_IMAGE_SIZE;
    MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;

    /* Set up session space */
    MiSessionSpaceEnd = (PVOID)MI_SESSION_SPACE_END;

    /* This is where we will load Win32k.sys and the video driver */
    MiSessionImageEnd = MiSessionSpaceEnd;
    MiSessionImageStart = (PCHAR)MiSessionImageEnd - MmSessionImageSize;

    /* The view starts right below the session working set (itself below
     * the image area) */
    MiSessionViewEnd = MI_SESSION_VIEW_END;
    MiSessionViewStart = (PCHAR)MiSessionViewEnd - MmSessionViewSize;
    ASSERT(IS_PAGE_ALIGNED(MiSessionViewStart));

    /* Session pool follows */
    MiSessionPoolEnd = MiSessionViewStart;
    MiSessionPoolStart = (PCHAR)MiSessionPoolEnd - MmSessionPoolSize;
    ASSERT(IS_PAGE_ALIGNED(MiSessionPoolStart));

    /* And it all begins here */
    MmSessionBase = MiSessionPoolStart;

    /* System view space ends at session space, so now that we know where
     * this is, we can compute the base address of system view space itself. */
    MiSystemViewStart = (PCHAR)MmSessionBase - MmSystemViewSize;
    ASSERT(IS_PAGE_ALIGNED(MiSystemViewStart));

    /* Sanity checks */
    ASSERT(MiSessionViewEnd <= MiSessionImageStart);
    ASSERT(MmSessionBase <= MiSessionPoolStart);
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
            *PointerPpe = TmplPde;

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
            *PointerPde = TmplPde;

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
            *PointerPte = TmplPte;
        }
    }
}

VOID
NTAPI
INIT_FUNCTION
MiInitializePageTable()
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

    /* Enable no execute */
    __writemsr(X86_MSR_EFER, __readmsr(X86_MSR_EFER) | EFER_NXE);

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
     * skip page tables and hyperspace */

    /* Loop the PXEs */
    for (PointerPxe = MiAddressToPxe((PVOID)(HYPER_SPACE_END + 1));
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

    /* Setup the mapping PPEs and PDEs */
    MiMapPPEs((PVOID)MI_MAPPING_RANGE_START, (PVOID)MI_MAPPING_RANGE_END);
    MiMapPDEs((PVOID)MI_MAPPING_RANGE_START, (PVOID)MI_MAPPING_RANGE_END);

    /* Setup the mapping PTEs */
    MmFirstReservedMappingPte = MiAddressToPte((PVOID)MI_MAPPING_RANGE_START);
    MmLastReservedMappingPte = MiAddressToPte((PVOID)MI_MAPPING_RANGE_END);
    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;

#ifdef _WINKD_
    /* Setup debug mapping PTE */
    MiMapPPEs((PVOID)MI_DEBUG_MAPPING, (PVOID)MI_DEBUG_MAPPING);
    MiMapPDEs((PVOID)MI_DEBUG_MAPPING, (PVOID)MI_DEBUG_MAPPING);
    MmDebugPte = MiAddressToPte((PVOID)MI_DEBUG_MAPPING);
#endif
}

VOID
NTAPI
INIT_FUNCTION
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

    /* Put non paged pool to the end of the region */
    MmNonPagedPoolStart = (PCHAR)MmNonPagedPoolEnd - MmMaximumNonPagedPoolInBytes;

    /* Make sure it doesn't collide with the PFN database */
    if ((PCHAR)MmNonPagedPoolStart < (PCHAR)MmPfnDatabase + MxPfnSizeInBytes)
    {
        /* Put non paged pool after the PFN database */
        MmNonPagedPoolStart = (PCHAR)MmPfnDatabase + MxPfnSizeInBytes;
        MmMaximumNonPagedPoolInBytes = (ULONG64)MmNonPagedPoolEnd -
                                       (ULONG64)MmNonPagedPoolStart;
    }

    ASSERT(IS_PAGE_ALIGNED(MmNonPagedPoolStart));

    /* Calculate the nonpaged pool expansion start region */
    MmNonPagedPoolExpansionStart = (PCHAR)MmNonPagedPoolStart +
                                          MmSizeOfNonPagedPoolInBytes;
    ASSERT(IS_PAGE_ALIGNED(MmNonPagedPoolExpansionStart));

    /* Map PPEs and PDEs for non paged pool (including expansion) */
    MiMapPPEs(MmNonPagedPoolStart, MmNonPagedPoolEnd);
    MiMapPDEs(MmNonPagedPoolStart, MmNonPagedPoolEnd);

    /* Map the nonpaged pool PTEs (without expansion) */
    MiMapPTEs(MmNonPagedPoolStart, (PUCHAR)MmNonPagedPoolExpansionStart - 1);

    /* Initialize the ARM3 nonpaged pool */
    MiInitializeNonPagedPool();

    /* Initialize the nonpaged pool */
    InitializePool(NonPagedPool, 0);
}

VOID
NTAPI
INIT_FUNCTION
MiBuildSystemPteSpace()
{
    PMMPTE PointerPte;

    /* Use the default numer of system PTEs */
    MmNumberOfSystemPtes = MI_NUMBER_SYSTEM_PTES;

    /* System PTE pool is below the PFN database */
    MiNonPagedSystemSize = (MmNumberOfSystemPtes + 1) * PAGE_SIZE;
    MmNonPagedSystemStart = (PCHAR)MmPfnDatabase - MiNonPagedSystemSize;
    MmNonPagedSystemStart = MM_ROUND_DOWN(MmNonPagedSystemStart, 512 * PAGE_SIZE);

    /* Don't let it go below the minimum */
    if (MmNonPagedSystemStart < (PVOID)MI_NON_PAGED_SYSTEM_START_MIN)
    {
        /* This is a hard-coded limit in the Windows NT address space */
        MmNonPagedSystemStart = (PVOID)MI_NON_PAGED_SYSTEM_START_MIN;

        /* Reduce the amount of system PTEs to reach this point */
        MmNumberOfSystemPtes = (ULONG)(((ULONG64)MmPfnDatabase -
                                (ULONG64)MmNonPagedSystemStart) >>
                                PAGE_SHIFT);
        MmNumberOfSystemPtes--;
        ASSERT(MmNumberOfSystemPtes > 1000);
    }

    /* Map the PDEs and PPEs for the system PTEs */
    MiMapPPEs(MI_SYSTEM_PTE_START, MI_SYSTEM_PTE_END);
    MiMapPDEs(MI_SYSTEM_PTE_START, MI_SYSTEM_PTE_END);

    /* Create the system PTE space */
    PointerPte = MiAddressToPte(MI_SYSTEM_PTE_START);
    MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    /* Reserve system PTEs for zeroing PTEs and clear them */
    MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES, SystemPteSpace);
    RtlZeroMemory(MiFirstReservedZeroingPte, MI_ZERO_PTES * sizeof(MMPTE));

    /* Set the counter to maximum */
    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES - 1;
}

NTSTATUS
NTAPI
INIT_FUNCTION
MiInitMachineDependent(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{

    MmHyperSpaceEnd = (PVOID)HYPER_SPACE_END;

    MiInitializePageTable();

    MiBuildNonPagedPool();

    MiBuildSystemPteSpace();

    return STATUS_SUCCESS;
}
