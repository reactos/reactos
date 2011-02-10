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

VOID
NTAPI
HalInitializeBios(ULONG Unknown, PLOADER_PARAMETER_BLOCK LoaderBlock);

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
///SIZE_T MmSessionSize = MI_SESSION_SIZE;
SIZE_T MmSessionViewSize = MI_SESSION_VIEW_SIZE;
SIZE_T MmSessionPoolSize = MI_SESSION_POOL_SIZE;
SIZE_T MmSessionImageSize = MI_SESSION_IMAGE_SIZE;
SIZE_T MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;
SIZE_T MiNonPagedSystemSize;

/* Address ranges */
ULONG64 MmUserProbeAddress = 0x7FFFFFF0000ULL;
PVOID MmHighestUserAddress = (PVOID)0x7FFFFFEFFFFULL;
PVOID MmSystemRangeStart = (PVOID)0xFFFF080000000000ULL;
PVOID MmSessionBase;                            // FFFFF90000000000 = MiSessionPoolStart
PVOID MiSessionPoolStart;                       // FFFFF90000000000 = MiSessionPoolEnd - MmSessionPoolSize
PVOID MiSessionPoolEnd;                         //                  = MiSessionViewStart
PVOID MiSessionViewStart;                       //                  = MiSessionViewEnd - MmSessionViewSize
PVOID MiSessionViewEnd;                         // FFFFF97FFF000000
PVOID MiSessionImageStart;                      // ?FFFFF97FFF000000 = MiSessionImageEnd - MmSessionImageSize
PVOID MiSessionImageEnd;                        // FFFFF98000000000 = MiSessionSpaceEnd
PVOID MiSessionSpaceEnd = MI_SESSION_SPACE_END; // FFFFF98000000000
PVOID MmSystemCacheStart;                       // FFFFF98000000000
PVOID MmSystemCacheEnd;                         // FFFFFA8000000000
/// PVOID MmPagedPoolStart = MI_PAGED_POOL_START;   // FFFFFA8000000000
PVOID MmPagedPoolEnd;                           // FFFFFAA000000000
PVOID MiSystemViewStart;
PVOID MmNonPagedSystemStart;                    // FFFFFAA000000000
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
///PVOID MmNonPagedPoolEnd = MI_NONPAGED_POOL_END; // 0xFFFFFAE000000000
PVOID MmHyperSpaceEnd = (PVOID)HYPER_SPACE_END;

MMSUPPORT MmSystemCacheWs;

ULONG64 MxPfnSizeInBytes;

PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;
ULONG MiNumberDescriptors = 0;
PFN_NUMBER MiSystemPages = 0;
BOOLEAN MiIncludeType[LoaderMaximum];

PFN_NUMBER MxFreePageBase;
ULONG64 MxFreePageCount = 0;

extern PFN_NUMBER MmSystemPageDirectory[PD_COUNT];

BOOLEAN MiPfnsInitialized = FALSE;

/* FUNCTIONS *****************************************************************/

ULONG
NoDbgPrint(const char *Format, ...)
{
    return 0;
}

VOID
NTAPI
MiEvaluateMemoryDescriptors(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PFN_NUMBER LastPage;
    ULONG i;

    /* Get the size of the boot loader's image allocations */
    MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned * PAGE_SIZE;
    MmBootImageSize = ROUND_UP(MmBootImageSize, 4 * 1024 * 1024);

    /* Instantiate memory that we don't consider RAM/usable */
    for (i = 0; i < LoaderMaximum; i++) MiIncludeType[i] = TRUE;
    MiIncludeType[LoaderBad] = FALSE;
    MiIncludeType[LoaderFirmwarePermanent] = FALSE;
    MiIncludeType[LoaderSpecialMemory] = FALSE;
    MiIncludeType[LoaderBBTMemory] = FALSE;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the memory descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Count it */
        MiNumberDescriptors++;

        /* Skip pages that are not part of the PFN database */
        if (!MiIncludeType[Descriptor->MemoryType])
        {
            continue;
        }

        /* Add this to the total of pages */
        MmNumberOfPhysicalPages += Descriptor->PageCount;

        /* Check if this is the new lowest page */
        if (Descriptor->BasePage < MmLowestPhysicalPage)
        {
            /* Update the lowest page */
            MmLowestPhysicalPage = Descriptor->BasePage;
        }

        /* Check if this is the new highest page */
        LastPage = Descriptor->BasePage + Descriptor->PageCount - 1;
        if (LastPage > MmHighestPhysicalPage)
        {
            /* Update the highest page */
            MmHighestPhysicalPage = LastPage;
        }

        /* Check if this is currently free memory */
        if ((Descriptor->MemoryType == LoaderFree) ||
            (Descriptor->MemoryType == LoaderLoadedProgram) ||
            (Descriptor->MemoryType == LoaderFirmwareTemporary) ||
            (Descriptor->MemoryType == LoaderOsloaderStack))
        {
            /* Check if this is the largest memory descriptor */
            if (Descriptor->PageCount > MxFreePageCount)
            {
                /* For now, it is */
                MxFreeDescriptor = Descriptor;
                MxFreePageBase = Descriptor->BasePage;
                MxFreePageCount = Descriptor->PageCount;
            }
        }
        else
        {
            /* Add it to the amount of system used pages */
            MiSystemPages += Descriptor->PageCount;
        }
    }
}

PFN_NUMBER
NTAPI
MiEarlyAllocPage()
{
    PFN_NUMBER Pfn;

    if (MiPfnsInitialized)
    {
        return MmAllocPage(MC_SYSTEM);
    }

    /* Make sure we have enough pages */
    if (!MxFreePageCount)
    {
        /* Crash the system */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MxFreeDescriptor->PageCount,
                     MxOldFreeDescriptor.PageCount,
                     1);
    }

    /* Use our lowest usable free pages */
    Pfn = MxFreePageBase;
    MxFreePageBase++;
    MxFreePageCount--;
    return Pfn;
}

PMMPTE
NTAPI
MxGetPte(PVOID Address)
{
    PMMPTE Pte;
    MMPTE TmplPte;

    /* Setup template pte */
    TmplPte.u.Long = 0;
    TmplPte.u.Flush.Valid = 1;
    TmplPte.u.Flush.Write = 1;

    /* Get a pointer to the PXE */
    Pte = MiAddressToPxe(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmplPte.u.Hard.PageFrameNumber = MiEarlyAllocPage();
        *Pte = TmplPte;

        /* Zero the page */
        RtlZeroMemory(MiPteToAddress(Pte), PAGE_SIZE);
    }

    /* Get a pointer to the PPE */
    Pte = MiAddressToPpe(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmplPte.u.Hard.PageFrameNumber = MiEarlyAllocPage();
        *Pte = TmplPte;

        /* Zero the page */
        RtlZeroMemory(MiPteToAddress(Pte), PAGE_SIZE);
    }

    /* Get a pointer to the PDE */
    Pte = MiAddressToPde(Address);
    if (!Pte->u.Hard.Valid)
    {
        /* It's not valid, map it! */
        TmplPte.u.Hard.PageFrameNumber = MiEarlyAllocPage();
        *Pte = TmplPte;

        /* Zero the page */
        RtlZeroMemory(MiPteToAddress(Pte), PAGE_SIZE);
    }

    /* Get a pointer to the PTE */
    Pte = MiAddressToPte(Address);
    return Pte;
}

VOID
NTAPI
MxMapPage(PVOID Address)
{
    MMPTE TmplPte, *Pte;

    /* Setup template pte */
    TmplPte.u.Long = 0;
    TmplPte.u.Flush.Valid = 1;
    TmplPte.u.Flush.Write = 1;
    TmplPte.u.Hard.PageFrameNumber = MiEarlyAllocPage();

    /* Get the PTE for that page */
    Pte = MxGetPte(Address);
    ASSERT(Pte->u.Hard.Valid == 0);

    /* Map a physical page */
    *Pte = TmplPte;
}

VOID
MxMapPageRange(PVOID Address, ULONG64 PageCount)
{
    while (PageCount--)
    {
        /* Map the page */
        MxMapPage(Address);

        /* Goto next page */
        Address = (PVOID)((ULONG64)Address + PAGE_SIZE);
    }
}

VOID
NTAPI
MiPreparePfnDatabse(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PUCHAR Page, FirstPage;
    SIZE_T Size;

    /* Calculate the size of the PFN database and convert to pages */
    MxPfnSizeInBytes = ROUND_TO_PAGES((MmHighestPhysicalPage + 1) * sizeof(MMPFN));
    MxPfnAllocation = MxPfnSizeInBytes >> PAGE_SHIFT;

    /* Simply start at hardcoded address */
    MmPfnDatabase = MI_PFN_DATABASE;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the memory descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Skip pages that are not part of the PFN database */
        if (MiIncludeType[Descriptor->MemoryType])
        {
            /* Get the base and size of this pfn database entry */
            FirstPage = PAGE_ALIGN(&MmPfnDatabase[Descriptor->BasePage]);
            Size = ROUND_TO_PAGES(Descriptor->PageCount * sizeof(MMPFN));

            /* Loop the pages of this Pfn database entry */
            for (Page = FirstPage; Page < FirstPage + Size; Page += PAGE_SIZE)
            {
                /* Is the page already mapped? */
                if (!MmIsAddressValid(Page))
                {
                    /* It's not, map it now */
                    MxMapPage(Page);
                    RtlZeroMemory(Page, PAGE_SIZE);
                }
            }

            /* Zero out the pages */
            RtlZeroMemory(FirstPage, Size);
        }
    }
}


VOID
NTAPI
MiInitializeSessionSpace(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
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
MiInitializePageTable()
{
    ULONG64 PxePhysicalAddress;
    MMPTE TmplPte, *Pte;
    PFN_NUMBER PxePfn;

    /* HACK: don't use freeldr debug print anymore */
    //FrLdrDbgPrint = NoDbgPrint;

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
    for (Pte = MiAddressToPxe(0);
         Pte <= MiAddressToPxe(MmHighestUserAddress);
         Pte++)
    {
        /* Zero the PXE, clear all mappings */
        Pte->u.Long = 0;
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
    for (Pte = MiAddressToPxe((PVOID)(HYPER_SPACE_END + 1));
         Pte <= MiAddressToPxe(MI_HIGHEST_SYSTEM_ADDRESS);
         Pte++)
    {
        /* Is the PXE already valid? */
        if (!Pte->u.Hard.Valid)
        {
            /* It's not Initialize it */
            TmplPte.u.Flush.PageFrameNumber = MiEarlyAllocPage(0);
            *Pte = TmplPte;

            /* Zero the page. The PXE is the PTE for the PDPT. */
            RtlZeroMemory(MiPteToAddress(Pte), PAGE_SIZE);
        }
    }

    /* Setup the mapping PTEs */
    MmFirstReservedMappingPte = MxGetPte((PVOID)MI_MAPPING_RANGE_START);
    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;
    MmLastReservedMappingPte = MiAddressToPte((PVOID)MI_MAPPING_RANGE_END);

#ifdef _WINKD_
    /* Setup debug mapping PTE */
    MmDebugPte = MxGetPte(MI_DEBUG_MAPPING);
#endif
}

VOID
NTAPI
MiBuildNonPagedPool(VOID)
{
    PMMPTE Pte;
    PFN_COUNT PageCount;

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

    /* Map the nonpaged pool */
    PageCount = (MmSizeOfNonPagedPoolInBytes + PAGE_SIZE - 1) / PAGE_SIZE;
    MxMapPageRange(MmNonPagedPoolStart, PageCount);

    /* Loop the non paged pool extension PTEs */
    for (Pte = MiAddressToPte(MmNonPagedPoolExpansionStart);
         Pte <= MiAddressToPte(MmNonPagedPoolEnd);
         Pte++)
    {
        /* Create PXE, PPE, PDE and zero the PTE */
        MxGetPte(MiPteToAddress(Pte))->u.Long = 0;
    }

    /* Initialize the ARM3 nonpaged pool */
    MiInitializeNonPagedPool();

    /* Initialize the nonpaged pool */
    InitializePool(NonPagedPool, 0);
}

VOID
NTAPI
MiBuildSystemPteSpace()
{
    PMMPTE Pte, StartPte, EndPte;

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
        MmNumberOfSystemPtes = ((ULONG64)MmPfnDatabase -
                                (ULONG64)MmNonPagedSystemStart) >>
                                PAGE_SHIFT;
        MmNumberOfSystemPtes--;
        ASSERT(MmNumberOfSystemPtes > 1000);
    }

    /* Set the range of system PTEs */
    StartPte = MiAddressToPte(MI_SYSTEM_PTE_START);
    EndPte = StartPte + MmNumberOfSystemPtes - 1;

    /* Loop the system PTEs */
    for (Pte = StartPte; Pte <= EndPte; Pte++)
    {
        /* Create PXE, PPE, PDE and zero the PTE */
        MxGetPte(MiPteToAddress(Pte))->u.Long = 0;
    }

    /* Create the system PTE space */
    Pte = MiAddressToPte(MI_SYSTEM_PTE_START);
    MiInitializeSystemPtes(Pte, MmNumberOfSystemPtes, SystemPteSpace);

    /* Reserve system PTEs for zeroing PTEs and clear them */
    MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES, SystemPteSpace);
    RtlZeroMemory(MiFirstReservedZeroingPte, MI_ZERO_PTES * sizeof(MMPTE));

    /* Set the counter to maximum */
    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES - 1;
}

VOID
NTAPI
MiBuildPhysicalMemoryBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PPHYSICAL_MEMORY_DESCRIPTOR Buffer;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PFN_NUMBER NextPage = -1;
    PULONG Bitmap;
    ULONG Runs = 0;
    ULONG Size;

    /* Calculate size for the PFN bitmap */
    Size = ROUND_UP(MmHighestPhysicalPage + 1, sizeof(ULONG));

    /* Allocate the PFN bitmap */
    Bitmap = ExAllocatePoolWithTag(NonPagedPool, Size, '  mM');

    /* Allocate enough memory for the physical memory block */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                                   sizeof(PHYSICAL_MEMORY_RUN) *
                                   (MiNumberDescriptors - 1),
                                   'lMmM');
    if (!Bitmap || !Buffer)
    {
        /* This is critical */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MmLowestPhysicalPage,
                     MmHighestPhysicalPage,
                     0x101);
    }

    /* Initialize the bitmap and clear all bits */
    RtlInitializeBitMap(&MiPfnBitMap, Bitmap, MmHighestPhysicalPage + 1);
    RtlClearAllBits(&MiPfnBitMap);

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the memory descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Skip pages that are not part of the PFN database */
        if (!MiIncludeType[Descriptor->MemoryType])
        {
            continue;
        }

        /* Does the memory block begin where the last ended? */
        if (Descriptor->BasePage == NextPage)
        {
            /* Add it to the current run */
            Buffer->Run[Runs - 1].PageCount += Descriptor->PageCount;
        }
        else
        {
            /* Create a new run */
            Runs++;
            Buffer->Run[Runs - 1].BasePage = Descriptor->BasePage;
            Buffer->Run[Runs - 1].PageCount = Descriptor->PageCount;
        }

        /* Set the bits in the PFN bitmap */
        RtlSetBits(&MiPfnBitMap, Descriptor->BasePage, Descriptor->PageCount);

        /* Set the next page */
        NextPage = Descriptor->BasePage + Descriptor->PageCount;
    }

    // FIXME: allocate a buffer of better size

    Buffer->NumberOfRuns = Runs;
    Buffer->NumberOfPages = MmNumberOfPhysicalPages;
    MmPhysicalMemoryBlock = Buffer;
}

VOID
NTAPI
MiBuildPagedPool_x(VOID)
{
    PMMPTE Pte;
    MMPTE TmplPte;
    ULONG Size, BitMapSize;

    /* Default size for paged pool is 4 times non paged pool */
    MmSizeOfPagedPoolInBytes = 4 * MmMaximumNonPagedPoolInBytes;

    /* Make sure it doesn't overflow */
    if (MmSizeOfPagedPoolInBytes > ((ULONG64)MmNonPagedSystemStart -
                                    (ULONG64)MmPagedPoolStart))
    {
        MmSizeOfPagedPoolInBytes = (ULONG64)MmNonPagedSystemStart -
                                   (ULONG64)MmPagedPoolStart;
    }

    /* Make sure paged pool is big enough */
    if (MmSizeOfPagedPoolInBytes < MI_MIN_INIT_PAGED_POOLSIZE)
    {
        MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;
    }

    /* Align down to a PDE boundary */
    MmSizeOfPagedPoolInBytes = ROUND_DOWN(MmSizeOfPagedPoolInBytes,
                                          512 * PAGE_SIZE);
    MmSizeOfPagedPoolInPages = MmSizeOfPagedPoolInBytes >> PAGE_SHIFT;

    /* This is where paged pool ends */
    MmPagedPoolEnd = (PCHAR)MmPagedPoolStart + MmSizeOfPagedPoolInBytes - 1;

    /* Sanity check */
    ASSERT(MmPagedPoolEnd < MmNonPagedSystemStart);

    /* setup a template PTE */
    TmplPte.u.Long = 0;
    TmplPte.u.Flush.Valid = 1;
    TmplPte.u.Flush.Write = 1;

    /* Make sure the PXE is valid */
    Pte = MiAddressToPxe(MmPagedPoolStart);
    if (!Pte->u.Flush.Valid)
    {
        /* Map it! */
        TmplPte.u.Flush.PageFrameNumber = MmAllocPage(MC_SYSTEM);
        *Pte = TmplPte;
    }

    /* Map all page directories (max 128) */
    for (Pte = MiAddressToPpe(MmPagedPoolStart);
         Pte <= MiAddressToPpe(MmPagedPoolEnd);
         Pte++)
    {
        if (!Pte->u.Flush.Valid)
        {
            /* Map it! */
            TmplPte.u.Flush.PageFrameNumber = MiEarlyAllocPage();
            *Pte = TmplPte;
        }
    }

    /* Create and map the first PTE for paged pool */
    Pte = MxGetPte(MmPagedPoolStart);
    TmplPte.u.Flush.PageFrameNumber = MiEarlyAllocPage();
    *Pte = TmplPte;

    /* Save the first and last paged pool PTE */
    MmPagedPoolInfo.FirstPteForPagedPool = MiAddressToPte(MmPagedPoolStart);
    MmPagedPoolInfo.LastPteForPagedPool = MiAddressToPte(MmPagedPoolEnd);
    MmPagedPoolInfo.NextPdeForPagedPoolExpansion = MiAddressToPde(MmPagedPoolStart) + 1;

    // We keep track of each page via a bit, so check how big the bitmap will
    // have to be (make sure to align our page count such that it fits nicely
    // into a 4-byte aligned bitmap.

    /* The size of the bitmap in bits is the size in pages */
    BitMapSize = MmSizeOfPagedPoolInPages;

    /* Calculate buffer size in bytes, aligned to 32 bits */
    Size = sizeof(RTL_BITMAP) + ROUND_UP(BitMapSize, 32) / 8;

    // Allocate the allocation bitmap, which tells us which regions have not yet
    // been mapped into memory

    MmPagedPoolInfo.PagedPoolAllocationMap =
        ExAllocatePoolWithTag(NonPagedPool, Size, '  mM');
    ASSERT(MmPagedPoolInfo.PagedPoolAllocationMap);

    // Initialize it such that at first, only the first page's worth of PTEs is
    // marked as allocated (incidentially, the first PDE we allocated earlier).
    RtlInitializeBitMap(MmPagedPoolInfo.PagedPoolAllocationMap,
                        (PULONG)(MmPagedPoolInfo.PagedPoolAllocationMap + 1),
                        BitMapSize);
    RtlSetAllBits(MmPagedPoolInfo.PagedPoolAllocationMap);
    RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap, 0, 512);

    // We have a second bitmap, which keeps track of where allocations end.
    // Given the allocation bitmap and a base address, we can therefore figure
    // out which page is the last page of that allocation, and thus how big the
    // entire allocation is.
    MmPagedPoolInfo.EndOfPagedPoolBitmap =
        ExAllocatePoolWithTag(NonPagedPool, Size, '  mM');
    ASSERT(MmPagedPoolInfo.EndOfPagedPoolBitmap);

    /* Initialize the bitmap */
    RtlInitializeBitMap(MmPagedPoolInfo.EndOfPagedPoolBitmap,
                        (PULONG)(MmPagedPoolInfo.EndOfPagedPoolBitmap + 1),
                        BitMapSize);

    /* No allocations, no allocation ends; clear all bits. */
    RtlClearAllBits(MmPagedPoolInfo.EndOfPagedPoolBitmap);

    /* Initialize the paged pool mutex */
    KeInitializeGuardedMutex(&MmPagedPoolMutex);

    /* Initialize the paged pool */
    InitializePool(PagedPool, 0);
}


NTSTATUS
NTAPI
MmArmInitSystem_x(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    if (Phase == 0)
    {
        MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned * PAGE_SIZE;
        MmBootImageSize = ROUND_UP(MmBootImageSize, PAGE_SIZE);

        /* Parse memory descriptors, find free pages */
        MiEvaluateMemoryDescriptors(LoaderBlock);

        /* Start PFN database at hardcoded address */
        MmPfnDatabase = MI_PFN_DATABASE;

        /* Prepare PFN database mappings */
        MiPreparePfnDatabse(LoaderBlock);

        /* Initialize the session space */
        MiInitializeSessionSpace(LoaderBlock);

        /* Initialize some mappings */
        MiInitializePageTable();

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

        /* The pfn database is ready now */
        MiPfnsInitialized = TRUE;

        /* Initialize the nonpaged pool */
        MiBuildNonPagedPool();

        /* Initialize system PTE handling */
        MiBuildSystemPteSpace();

        /* Build the physical memory block */
        MiBuildPhysicalMemoryBlock(LoaderBlock);

        /* Size up paged pool and build the shadow system page directory */
        //MiBuildPagedPool();

        // This is the old stuff:
        //MmPagedPoolBase = (PVOID)((PCHAR)MmPagedPoolEnd + 1);
        //MmPagedPoolSize = MM_PAGED_POOL_SIZE;
        //ASSERT((PCHAR)MmPagedPoolBase + MmPagedPoolSize < (PCHAR)MmNonPagedSystemStart);


        HalInitializeBios(0, LoaderBlock);
    }

    return STATUS_SUCCESS;
}

VOID
FASTCALL
MiSyncARM3WithROS(IN PVOID AddressStart,
                  IN PVOID AddressEnd)
{

}

NTSTATUS
NTAPI
MiInitMachineDependent(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
