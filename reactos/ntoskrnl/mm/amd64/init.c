/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/init.c
 * PURPOSE:         Memory Manager Initialization for amd64
 *
 * PROGRAMMERS:     Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "../ARM3/miarm.h"

#define MI_SESSION_SPACE_END (PVOID)0xFFFFF98000000000ULL
#define MI_SESSION_VIEW_END 0xFFFFF97FFF000000ULL
#define MI_NON_PAGED_SYSTEM_START_MIN 0x0FFFFFAA000000000ULL

/* GLOBALS *****************************************************************/

ULONG64 MmUserProbeAddress = 0x7FFFFFF0000ULL;
PVOID MmHighestUserAddress = (PVOID)0x7FFFFFEFFFFULL;
PVOID MmSystemRangeStart = (PVOID)KSEG0_BASE; // FFFF080000000000

/* Size of session view, pool, and image */
ULONG64 MmSessionSize = MI_SESSION_SIZE;
ULONG64 MmSessionViewSize = MI_SESSION_VIEW_SIZE;
ULONG64 MmSessionPoolSize = MI_SESSION_POOL_SIZE;
ULONG64 MmSessionImageSize = MI_SESSION_IMAGE_SIZE;

/* Session space addresses */
PVOID MiSessionSpaceEnd = MI_SESSION_SPACE_END; // FFFFF98000000000
PVOID MiSessionImageEnd;    // FFFFF98000000000 = MiSessionSpaceEnd
PVOID MiSessionImageStart;  // ?FFFFF97FFF000000 = MiSessionImageEnd - MmSessionImageSize
PVOID MiSessionViewEnd;     // FFFFF97FFF000000
PVOID MiSessionViewStart;   //  = MiSessionViewEnd - MmSessionViewSize
PVOID MiSessionPoolEnd;     //  = MiSessionViewStart
PVOID MiSessionPoolStart;   // FFFFF90000000000 = MiSessionPoolEnd - MmSessionPoolSize
PVOID MmSessionBase;        // FFFFF90000000000 = MiSessionPoolStart

/* System view */
ULONG64 MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;
PVOID MiSystemViewStart;

ULONG64 MmMinimumNonPagedPoolSize = 256 * 1024;
ULONG64 MmSizeOfNonPagedPoolInBytes;
ULONG64 MmMaximumNonPagedPoolInBytes;
ULONG64 MmMaximumNonPagedPoolPercent;
ULONG64 MmMinAdditionNonPagedPoolPerMb = 32 * 1024;
ULONG64 MmMaxAdditionNonPagedPoolPerMb = 400 * 1024;
ULONG64 MmDefaultMaximumNonPagedPool = 1024 * 1024; 
PVOID MmNonPagedSystemStart;
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
PVOID MmNonPagedPoolEnd = MI_NONPAGED_POOL_END;

ULONG64 MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;
PVOID MmPagedPoolStart = MI_PAGED_POOL_START;
PVOID MmPagedPoolEnd;


ULONG64 MmBootImageSize;
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;
RTL_BITMAP MiPfnBitMap;
ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage = -1;
ULONG64 MmNumberOfSystemPtes;
PMMPTE MmSystemPagePtes;
ULONG64 MxPfnAllocation;

///////////////////////////////////////////////

PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;

PFN_NUMBER MxFreePageBase;
ULONG64 MxFreePageCount = 0;

VOID
NTAPI
MxSetupFreePageList(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    PLIST_ENTRY ListEntry;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the memory block */
        MdBlock = CONTAINING_RECORD(ListEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Check if this is free memory */
        if ((MdBlock->MemoryType == LoaderFree) ||
            (MdBlock->MemoryType == LoaderLoadedProgram) ||
            (MdBlock->MemoryType == LoaderFirmwareTemporary) ||
            (MdBlock->MemoryType == LoaderOsloaderStack))
        {
            /* Check if this is the largest memory descriptor */
            if (MdBlock->PageCount > MxFreePageCount)
            {
                /* For now, it is */
                MxFreeDescriptor = MdBlock;
                MxFreePageBase = MdBlock->BasePage;
                MxFreePageCount = MdBlock->PageCount;
            }
        }
    }
}

PFN_NUMBER
NTAPI
MxGetNextPage(IN PFN_NUMBER PageCount)
{
    PFN_NUMBER Pfn;

    /* Make sure we have enough pages */
    if (PageCount > MxFreePageCount)
    {
        /* Crash the system */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MxFreeDescriptor->PageCount,
                     MxOldFreeDescriptor.PageCount,
                     PageCount);
    }

    /* Use our lowest usable free pages */
    Pfn = MxFreePageBase;
    MxFreePageBase += PageCount;
    MxFreePageCount -= PageCount;
    return Pfn;
}

VOID
MxMapPage(PVOID Address)
{
    PMMPTE Pte;
    MMPTE TmpPte;

    TmpPte.u.Long = 0;
    TmpPte.u.Hard.Valid = 1;

    /* Get a pointer to the PXE */
    Pte = MiAddressToPxe(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmpPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
        *Pte = TmpPte;
    }

    /* Get a pointer to the PPE */
    Pte = MiAddressToPpe(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmpPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
        *Pte = TmpPte;
    }

    /* Get a pointer to the PDE */
    Pte = MiAddressToPde(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmpPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
        *Pte = TmpPte;
    }

    /* Get a pointer to the PTE */
    Pte = MiAddressToPte(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmpPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
        *Pte = TmpPte;
    }
}

VOID
MxMapPageRange(PVOID Address, ULONG64 PageCount)
{
    ULONG64 i;

    for (i = 0; i < PageCount; i++)
    {
        MxMapPage(Address);
        Address = (PVOID)((ULONG64)Address + PAGE_SIZE);
    }
}


VOID
NTAPI
MiArmIninializeMemoryLayout(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Get the size of the boot loader's image allocations */
    MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
    MmBootImageSize *= PAGE_SIZE;
    MmBootImageSize = (MmBootImageSize + (4 * 1024 * 1024) - 1) & ~((4 * 1024 * 1024) - 1);
    ASSERT((MmBootImageSize % (4 * 1024 * 1024)) == 0);

    MiSessionSpaceEnd = (PVOID)MI_SESSION_SPACE_END;

    /* This is where we will load Win32k.sys and the video driver */
    MiSessionImageEnd = MiSessionSpaceEnd;
    MiSessionImageStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                  MmSessionImageSize);

    /* The view starts right below the session working set (itself below
     * the image area) */
    MiSessionViewEnd = (PVOID)MI_SESSION_VIEW_END;
    MiSessionViewStart = (PVOID)((ULONG_PTR)MiSessionViewStart -
                                 MmSessionViewSize);

    /* Session pool follows */
    MiSessionPoolEnd = MiSessionViewStart;
    MiSessionPoolStart = (PVOID)((ULONG_PTR)MiSessionPoolEnd -
                                 MmSessionPoolSize);

    /* And it all begins here */
    MmSessionBase = MiSessionPoolStart;

    // Sanity check that our math is correct
    //ASSERT((ULONG_PTR)MmSessionBase + MmSessionSize == PTE_BASE);

    /* System view space ends at session space, so now that we know where
     * this is, we can compute the base address of system view space itself. */
    MiSystemViewStart = (PVOID)((ULONG_PTR)MmSessionBase -
                                MmSystemViewSize);

    /* Use the default */
    MmNumberOfSystemPtes = 22000;

}

VOID
MiArmInitializePageTable()
{
    ULONG64 PageFrameOffset;
    PMMPTE StartPte, EndPte;

    /* Set CR3 for the system process */
    PageFrameOffset = ((PMMPTE)PXE_BASE)->u.Hard.PageFrameNumber << PAGE_SHIFT;
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameOffset;

    /* Clear user mode mappings in PML4 */
    StartPte = MiAddressToPxe(0);
    EndPte = MiAddressToPxe(MmSystemRangeStart);
    RtlZeroMemory(StartPte, (EndPte - StartPte) * sizeof(MMPTE));
}


VOID
NTAPI
MiArmEvaluateMemoryDescriptors(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    PLIST_ENTRY ListEntry;
    PFN_NUMBER BasePage, LastPage, PageCount;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        MdBlock = CONTAINING_RECORD(ListEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Skip pages that are not part of the PFN database */
        if ((MdBlock->MemoryType == LoaderFirmwarePermanent) ||
            (MdBlock->MemoryType == LoaderBBTMemory) ||
            (MdBlock->MemoryType == LoaderHALCachedMemory) || // ???
            (MdBlock->MemoryType == LoaderSpecialMemory))
        {
            continue;
        }

        /* Check if BURNMEM was used */
        if (MdBlock->MemoryType != LoaderBad)
        {
            /* Count this in the total of pages */
            MmNumberOfPhysicalPages += MdBlock->PageCount;
        }

        BasePage = MdBlock->BasePage;
        LastPage = MdBlock->BasePage + MdBlock->PageCount - 1;

        /* Check if this is the new lowest page */
        if (BasePage < MmLowestPhysicalPage)
        {
            /* Update the lowest page */
            MmLowestPhysicalPage = BasePage;
        }

        /* Check if this is the new highest page */
        if (LastPage > MmHighestPhysicalPage)
        {
            /* Update the highest page */
            MmHighestPhysicalPage = LastPage;
        }

        /* Map pages for the PFN database */
        PageCount = (MdBlock->PageCount * sizeof(MMPFN)) / PAGE_SIZE;
        MxMapPageRange(&MmPfnDatabase[BasePage], PageCount);

        /* Zero out the pages */
        RtlZeroMemory(&MmPfnDatabase[BasePage], PageCount * PAGE_SIZE);
    }

    /* Calculate the number of bytes, and then convert to pages */
    MxPfnAllocation = (MmHighestPhysicalPage + 1) * sizeof(MMPFN);
    MxPfnAllocation >>= PAGE_SHIFT;
    MxPfnAllocation++;

}

VOID
NTAPI
MiArmPrepareNonPagedPool()
{
    PFN_NUMBER PageCount;

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
        // Set it to the maximum */
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

    /* Calculate the nonpaged pool expansion start region */
    MmNonPagedPoolExpansionStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd -
                                  MmMaximumNonPagedPoolInBytes +
                                  MmSizeOfNonPagedPoolInBytes);
    MmNonPagedPoolExpansionStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolExpansionStart);

    DPRINT("NP Pool has been tuned to: %d bytes and %d bytes\n",
           MmSizeOfNonPagedPoolInBytes, MmMaximumNonPagedPoolInBytes);

    /* Now calculate the nonpaged system VA region, which includes the
     * nonpaged pool expansion (above) and the system PTEs. Note that it is
     * then aligned to a PDE boundary (4MB). */
    MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedPoolExpansionStart -
                                    (MmNumberOfSystemPtes + 1) * PAGE_SIZE);
    MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart &
                                    ~((4 * 1024 * 1024) - 1));

    /* Don't let it go below the minimum */
    if (MmNonPagedSystemStart < (PVOID)MI_NON_PAGED_SYSTEM_START_MIN)
    {
        /* This is a hard-coded limit in the Windows NT address space */
        MmNonPagedSystemStart = (PVOID)MI_NON_PAGED_SYSTEM_START_MIN;

        /* Reduce the amount of system PTEs to reach this point */
        MmNumberOfSystemPtes = ((ULONG_PTR)MmNonPagedPoolExpansionStart -
                                (ULONG_PTR)MmNonPagedSystemStart) >>
                                PAGE_SHIFT;
        MmNumberOfSystemPtes--;
        ASSERT(MmNumberOfSystemPtes > 1000);
    }

    /* Non paged pool comes after the PFN database */
    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmPfnDatabase +
                                  (MxPfnAllocation << PAGE_SHIFT));

    /* Map the nonpaged pool */
    PageCount = (MmSizeOfNonPagedPoolInBytes + PAGE_SIZE - 1) / PAGE_SIZE;
    MxMapPageRange(MmNonPagedPoolStart, PageCount);

    /* Sanity check: make sure we have properly defined the system PTE space */
    ASSERT(MiAddressToPte(MmNonPagedSystemStart) <
           MiAddressToPte(MmNonPagedPoolExpansionStart));

}

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (Phase == 0)
    {
        /* Get a continuous range of physical pages */
        MxSetupFreePageList(LoaderBlock);

        /* Initialize the memory layout */
        MiArmIninializeMemoryLayout(LoaderBlock);
        DPRINT1("MmArmInitSystem 3\n");

        /* Loop descriptors and prepare PFN database */
        MiArmEvaluateMemoryDescriptors(LoaderBlock);
        DPRINT1("MmArmInitSystem 4\n");

        MiArmInitializePageTable();
        DPRINT1("MmArmInitSystem 5\n");

        /* Configure size of the non paged pool */
        MiArmPrepareNonPagedPool();

        /* Initialize the ARM3 nonpaged pool */
        MiInitializeArmPool();

        /* Update the memory descriptor, to make sure the pages we used
           won't get inserted into the PFN database */
        MxOldFreeDescriptor = *MxFreeDescriptor;
        MxFreeDescriptor->BasePage = MxFreePageBase;
        MxFreeDescriptor->PageCount = MxFreePageCount;
    }
    else if (Phase == 1)
    {
        /* The PFN database was created, restore the free descriptor */
        *MxFreeDescriptor = MxOldFreeDescriptor;


    }

    return STATUS_SUCCESS;
}

