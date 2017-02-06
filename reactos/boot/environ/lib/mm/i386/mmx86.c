/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Library
* FILE:            boot/environ/lib/mm/i386/mmx86.c
* PURPOSE:         Boot Library Memory Manager x86-Specific Code
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"
#include "bcd.h"
#include "../../../../../ntoskrnl/include/internal/i386/mm.h"

/* DATA VARIABLES ************************************************************/

ULONG_PTR MmArchKsegBase;
ULONG_PTR MmArchKsegBias;
ULONG MmArchLargePageSize;
BL_ADDRESS_RANGE MmArchKsegAddressRange;
ULONG_PTR MmArchTopOfApplicationAddressSpace;
PHYSICAL_ADDRESS Mmx86SelfMapBase;
ULONG MmDeferredMappingCount;
PMMPTE MmPdpt;
PULONG MmArchReferencePage;
PVOID MmPteBase;
PVOID MmPdeBase;
ULONG MmArchReferencePageSize;

typedef VOID
(*PBL_MM_FLUSH_TLB) (
    VOID
    );

typedef VOID
(*PBL_MM_RELOCATE_SELF_MAP) (
    VOID
    );

typedef NTSTATUS
(*PBL_MM_MOVE_VIRTUAL_ADDRESS_RANGE) (
    _In_ PVOID DestinationAddress,
    _In_ PVOID SourceAddress,
    _In_ ULONGLONG Size
    );

typedef NTSTATUS
(*PBL_MM_ZERO_VIRTUAL_ADDRESS_RANGE) (
    _In_ PVOID DestinationAddress,
    _In_ ULONGLONG Size
    );

typedef VOID
(*PBL_MM_DESTROY_SELF_MAP) (
    VOID
    );

typedef VOID
(*PBL_MM_FLUSH_TLB_ENTRY) (
    _In_ PVOID VirtualAddress
    );

typedef VOID
(*PBL_MM_FLUSH_TLB) (
    VOID
    );

typedef NTSTATUS
(*PBL_MM_UNMAP_VIRTUAL_ADDRESS) (
    _In_ PVOID VirtualAddress,
    _In_ ULONG Size
    );

typedef NTSTATUS
(*PBL_MM_REMAP_VIRTUAL_ADDRESS) (
    _In_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID VirtualAddress,
    _In_ ULONG Size,
    _In_ ULONG CacheAttributes
    );

typedef NTSTATUS
(*PBL_MM_MAP_PHYSICAL_ADDRESS) (
    _In_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID VirtualAddress,
    _In_ ULONG Size,
    _In_ ULONG CacheAttributes
    );

typedef BOOLEAN
(*PBL_MM_TRANSLATE_VIRTUAL_ADDRESS) (
    _In_ PVOID VirtualAddress,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_opt_ PULONG CacheAttributes
    );

PBL_MM_TRANSLATE_VIRTUAL_ADDRESS Mmx86TranslateVirtualAddress;
PBL_MM_MAP_PHYSICAL_ADDRESS Mmx86MapPhysicalAddress;
PBL_MM_REMAP_VIRTUAL_ADDRESS Mmx86RemapVirtualAddress;
PBL_MM_UNMAP_VIRTUAL_ADDRESS Mmx86UnmapVirtualAddress;
PBL_MM_FLUSH_TLB Mmx86FlushTlb;
PBL_MM_FLUSH_TLB_ENTRY Mmx86FlushTlbEntry;
PBL_MM_DESTROY_SELF_MAP Mmx86DestroySelfMap;

PBL_MM_RELOCATE_SELF_MAP BlMmRelocateSelfMap;
PBL_MM_FLUSH_TLB BlMmFlushTlb;
PBL_MM_MOVE_VIRTUAL_ADDRESS_RANGE BlMmMoveVirtualAddressRange;
PBL_MM_ZERO_VIRTUAL_ADDRESS_RANGE BlMmZeroVirtualAddressRange;

PBL_MM_FLUSH_TLB Mmx86FlushTlb;

/* FUNCTIONS *****************************************************************/

VOID
MmArchNullFunction (
    VOID
    )
{
    /* Nothing to do */
    return;
}

VOID
MmDefRelocateSelfMap (
    VOID
    )
{
    if (MmPteBase != (PVOID)PTE_BASE)
    {
        EfiPrintf(L"Supposed to relocate CR3\r\n");
    }
}

NTSTATUS
MmDefMoveVirtualAddressRange (
    _In_ PVOID DestinationAddress,
    _In_ PVOID SourceAddress,
    _In_ ULONGLONG Size
    )
{
    EfiPrintf(L"Supposed to move shit\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmDefZeroVirtualAddressRange (
    _In_ PVOID DestinationAddress,
    _In_ ULONGLONG Size
    )
{
    EfiPrintf(L"Supposed to zero shit\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
Mmx86pMapMemoryRegions (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData
    )
{
    BOOLEAN DoDeferred;

    /* In phase 1 we don't initialize deferred mappings*/
    if (Phase == 1)
    {
        DoDeferred = 0;
    }
    else
    {
        /* Don't do anything if there's nothing to initialize */
        if (!MmDeferredMappingCount)
        {
            return STATUS_SUCCESS;
        }

        DoDeferred = 1;
    }

    if (DoDeferred)
    {
        EfiPrintf(L"Deferred todo\r\n");
    }

    EfiPrintf(L"Phase 1 TODO\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
MmArchTranslateVirtualAddress (
    _In_ PVOID VirtualAddress, 
    _Out_opt_ PPHYSICAL_ADDRESS PhysicalAddress, 
    _Out_opt_ PULONG CachingFlags
    )
{
    PBL_MEMORY_DESCRIPTOR Descriptor;

    /* Check if paging is on */
    if ((CurrentExecutionContext) &&
        (CurrentExecutionContext->ContextFlags & BL_CONTEXT_PAGING_ON))
    {
        /* Yes -- we have to translate this from virtual */
        return Mmx86TranslateVirtualAddress(VirtualAddress,
                                            PhysicalAddress,
                                            CachingFlags);
    }

    /* Look in all descriptors except truncated and firmware ones */
    Descriptor = MmMdFindDescriptor(BL_MM_INCLUDE_NO_FIRMWARE_MEMORY &
                                    ~BL_MM_INCLUDE_TRUNCATED_MEMORY,
                                    BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                    (ULONG_PTR)VirtualAddress >> PAGE_SHIFT);

    /* Return the virtual address as the physical address */
    if (PhysicalAddress)
    {
        PhysicalAddress->HighPart = 0;
        PhysicalAddress->LowPart = (ULONG_PTR)VirtualAddress;
    }

    /* There's no caching on physical memory */
    if (CachingFlags)
    {
        *CachingFlags = 0;
    }

    /* Success is if we found a descriptor */
    return Descriptor != NULL;
}

VOID
MmDefpDestroySelfMap (
    VOID
    )
{
    EfiPrintf(L"No destroy\r\n");
}

VOID
MmDefpFlushTlbEntry (
    _In_ PVOID VirtualAddress
    )
{
    /* Flush the TLB */
    __invlpg(VirtualAddress);
}

VOID
MmDefpFlushTlb (
    VOID
    )
{
    /* Flush the TLB */
    __writecr3(__readcr3());
}

NTSTATUS
MmDefpUnmapVirtualAddress (
    _In_ PVOID VirtualAddress,
    _In_ ULONG Size
    )
{
    EfiPrintf(L"No unmap\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmDefpRemapVirtualAddress (
    _In_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID VirtualAddress,
    _In_ ULONG Size,
    _In_ ULONG CacheAttributes
    )
{
    EfiPrintf(L"No remap\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmDefpMapPhysicalAddress (
    _In_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_ PVOID VirtualAddress,
    _In_ ULONG Size,
    _In_ ULONG CacheAttributes
    )
{
    EfiPrintf(L"No map\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
MmDefpTranslateVirtualAddress (
    _In_ PVOID VirtualAddress,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_opt_ PULONG CacheAttributes
    )
{
    EfiPrintf(L"No translate\r\n");
    return FALSE;
}

NTSTATUS
Mmx86MapInitStructure (
    _In_ PVOID VirtualAddress,
    _In_ ULONGLONG Size,
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    )
{
    NTSTATUS Status;
    
    /* Make a virtual mapping for this physical address */
    Status = MmMapPhysicalAddress(&PhysicalAddress, &VirtualAddress, &Size, 0);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Nothing else to do if we're not in paging mode */
    if (MmTranslationType == BlNone)
    {
        return STATUS_SUCCESS;
    }

    /* Otherwise, remove this region from the list of free virtual ranges */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       (ULONG_PTR)VirtualAddress >> PAGE_SHIFT,
                                       Size >> PAGE_SHIFT,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        /* Unmap the address if that failed */
        MmUnmapVirtualAddress(&VirtualAddress, &Size);
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
Mmx86InitializeMemoryMap (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData
    )
{
    ULONG ImageSize;
    PVOID ImageBase;
    KDESCRIPTOR Gdt, Idt;
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* If this is phase 2, map the memory regions */
    if (Phase != 1)
    {
        return Mmx86pMapMemoryRegions(Phase, MemoryData);
    }

    /* Get the application image base/size */
    Status = BlGetApplicationBaseAndSize(&ImageBase, &ImageSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Map the image back at the same place */
    PhysicalAddress.QuadPart = (ULONG_PTR)ImageBase;
    Status = Mmx86MapInitStructure(ImageBase, ImageSize, PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Map the first 4MB of memory */
    PhysicalAddress.QuadPart = 0;
    Status = Mmx86MapInitStructure(NULL, 4 * 1024 * 1024, PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Map the GDT */
    _sgdt(&Gdt.Limit);
    PhysicalAddress.QuadPart = Gdt.Base;
    Status = Mmx86MapInitStructure((PVOID)Gdt.Base, Gdt.Limit + 1, PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Map the IDT */
    __sidt(&Idt.Limit);
    PhysicalAddress.QuadPart = Idt.Base;
    Status = Mmx86MapInitStructure((PVOID)Idt.Base, Idt.Limit + 1, PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Map the reference page */
    PhysicalAddress.QuadPart = (ULONG_PTR)MmArchReferencePage;
    Status = Mmx86MapInitStructure(MmArchReferencePage,
                                   MmArchReferencePageSize,
                                   PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* More to do */
    EfiPrintf(L"VM more work\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MmDefInitializeTranslation (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG PdeIndex;

    /* Set the global function pointers for memory translation */
    Mmx86TranslateVirtualAddress = MmDefpTranslateVirtualAddress;
    Mmx86MapPhysicalAddress = MmDefpMapPhysicalAddress;
    Mmx86UnmapVirtualAddress = MmDefpUnmapVirtualAddress;
    Mmx86RemapVirtualAddress = MmDefpRemapVirtualAddress;
    Mmx86FlushTlb = MmDefpFlushTlb;
    Mmx86FlushTlbEntry = MmDefpFlushTlbEntry;
    Mmx86DestroySelfMap = MmDefpDestroySelfMap;

    /* Check what mode we're currently in */
    if (TranslationType == BlVirtual)
    {
        EfiPrintf(L"Virtual->Virtual not yet supported\r\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    else if (TranslationType != BlNone)
    {
        /* Not even Windows supports PAE->Virtual downgrade */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* The None->Virtual case */
    MmPdpt = NULL;
    Mmx86SelfMapBase.QuadPart = 0;
    MmArchReferencePage = NULL;

    /* Truncate all memory above 4GB so that we don't use it @TODO: FIXME */
    EfiPrintf(L"Warning: not truncating > 4GB memory. Don't boot with more than 4GB of RAM!\r\n");
    //Status = MmPaTruncateMemory(0x100000);
    Status = STATUS_SUCCESS;
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate a page directory */
    Status = MmPapAllocatePhysicalPagesInRange(&PhysicalAddress,
                                               BlLoaderPageDirectory,
                                               1,
                                               0,
                                               0,
                                               &MmMdlUnmappedAllocated,
                                               0,
                                               0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Zero out the page directory */
    MmPdpt = (PVOID)PhysicalAddress.LowPart;
    RtlZeroMemory(MmPdpt, PAGE_SIZE);

    /* Set the page size */
    MmArchReferencePageSize = PAGE_SIZE;

    /* Allocate the self-map page */
    Status = MmPapAllocatePhysicalPagesInRange(&PhysicalAddress,
                                               BlLoaderReferencePage,
                                               1,
                                               0,
                                               0,
                                               &MmMdlUnmappedAllocated,
                                               0,
                                               0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Set the reference page */
    MmArchReferencePage = (PVOID)PhysicalAddress.LowPart;

    /* Zero it out */
    RtlZeroMemory(MmArchReferencePage, MmArchReferencePageSize);

    /* Allocate 4MB worth of self-map pages */
    Status = MmPaReserveSelfMapPages(&Mmx86SelfMapBase, 
                                     (4 * 1024 * 1024) >> PAGE_SHIFT,
                                     (4 * 1024 * 1024) >> PAGE_SHIFT);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Zero them out */
    RtlZeroMemory((PVOID)Mmx86SelfMapBase.LowPart, 4 * 1024 * 1024);
    EfiPrintf(L"PDPT at 0x%p Reference Page at 0x%p Self-map at 0x%p\r\n",
              MmPdpt, MmArchReferencePage, Mmx86SelfMapBase.LowPart);

    /* Align PTE base to 4MB region */
    MmPteBase = (PVOID)(Mmx86SelfMapBase.LowPart & ~0x3FFFFF);

    /* The PDE is the PTE of the PTE base */
    MmPdeBase = MiAddressToPte(MmPteBase);
    PdeIndex = MiGetPdeOffset(MmPdeBase);
    MmPdpt[PdeIndex].u.Hard.Valid = 1;
    MmPdpt[PdeIndex].u.Hard.Write = 1;
    MmPdpt[PdeIndex].u.Hard.PageFrameNumber = (ULONG_PTR)MmPdpt >> PAGE_SHIFT;
    MmArchReferencePage[PdeIndex]++;

    /* Remove PTE_BASE from free virtual memory */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       PTE_BASE >> PAGE_SHIFT,
                                       (4 * 1024 * 1024) >> PAGE_SHIFT,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Remove HAL_HEAP from free virtual memory */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       MM_HAL_VA_START >> PAGE_SHIFT,
                                       (4 * 1024 * 1024) >> PAGE_SHIFT,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Initialize the virtual->physical memory mappings */
    Status = Mmx86InitializeMemoryMap(1, MemoryData);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    EfiPrintf(L"Ready to turn on motherfucking paging, brah!\r\n");
    Status = STATUS_NOT_IMPLEMENTED;

Quickie:
    /* Free reference page if we allocated it */
    if (MmArchReferencePage)
    {
        PhysicalAddress.QuadPart = (ULONG_PTR)MmArchReferencePage;
        BlMmFreePhysicalPages(PhysicalAddress);
    }

    /* Free page directory if we allocated it */
    if (MmPdpt)
    {
        PhysicalAddress.QuadPart = (ULONG_PTR)MmPdpt;
        BlMmFreePhysicalPages(PhysicalAddress);
    }

    /* Free the self map if we allocated it */
    if (Mmx86SelfMapBase.QuadPart)
    {
        MmPaReleaseSelfMapPages(Mmx86SelfMapBase);
    }

    /* All done */
    return Status;
}

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE RequestedTranslationType
    )
{
    NTSTATUS Status;
    ULONGLONG IncreaseUserVa, PerfCounter, CpuRandom;
    INT CpuInfo[4];

    /* For phase 2, just map deferred regions */
    if (Phase != 1)
    {
        return Mmx86pMapMemoryRegions(2, MemoryData);
    }

    /* What translation type are we switching to? */
    switch (RequestedTranslationType)
    {
        /* Physical memory */
        case BlNone:

            /* Initialize everything to default/null values */
            MmArchLargePageSize = 1;
            MmArchKsegBase = 0;
            MmArchKsegBias = 0;
            MmArchKsegAddressRange.Minimum = 0;
            MmArchKsegAddressRange.Maximum = (ULONGLONG)~0;
            MmArchTopOfApplicationAddressSpace = 0;
            Mmx86SelfMapBase.QuadPart = 0;

            /* Set stub functions */
            BlMmRelocateSelfMap = MmArchNullFunction;
            BlMmFlushTlb = MmArchNullFunction;

            /* Set success */
            Status = STATUS_SUCCESS;
            break;

        case BlVirtual:

            /* Set the large page size to 1024 pages (4MB) */
            MmArchLargePageSize = (4 * 1024 * 1024) / PAGE_SIZE;

            /* Check if /USERVA option was used */
            Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                            BcdOSLoaderInteger_IncreaseUserVa,
                                            &IncreaseUserVa);
            if (NT_SUCCESS(Status) && (IncreaseUserVa))
            {
                /* Yes -- load the kernel at 0xE0000000 instead */
                MmArchKsegBase = 0xE0000000;
            }
            else
            {
                /* Nope, load at the standard 2GB split */
                MmArchKsegBase = 0x80000000;
            }

            /* Check if CPUID 01h is supported */
            CpuRandom = 0;
            if (BlArchIsCpuIdFunctionSupported(1))
            {
                /* Call it */
                BlArchCpuId(1, 0, CpuInfo);

                /* Check if RDRAND is supported */
                if (CpuInfo[2] & 0x40000000)
                {
                    EfiPrintf(L"Your CPU can do RDRAND! Good for you!\r\n");
                    CpuRandom = 0;
                }
            }

            /* Read the TSC */
            PerfCounter = BlArchGetPerformanceCounter();
            PerfCounter >>= 4;
            _rotl16(PerfCounter, 5);

            /* Set the address range */
            MmArchKsegAddressRange.Minimum = 0;
            MmArchKsegAddressRange.Maximum = (ULONGLONG)~0;

            /* Set the KASLR bias */
            MmArchKsegBias = ((PerfCounter ^ CpuRandom) & 0xFFF) << 12;
            MmArchKsegBias = 0;
            MmArchKsegBase += MmArchKsegBias;

            /* Set the kernel range */
            MmArchKsegAddressRange.Minimum = MmArchKsegBase;
            MmArchKsegAddressRange.Maximum = (ULONGLONG)~0;

            /* Set the boot application top maximum */
            MmArchTopOfApplicationAddressSpace = 0x70000000;

            /* Initialize virtual address space translation */
            Status = MmDefInitializeTranslation(MemoryData, TranslationType);
            if (NT_SUCCESS(Status))
            {
                /* Set stub functions */
                BlMmRelocateSelfMap = MmDefRelocateSelfMap;
                BlMmFlushTlb = Mmx86FlushTlb;
                BlMmMoveVirtualAddressRange = MmDefMoveVirtualAddressRange;
                BlMmZeroVirtualAddressRange = MmDefZeroVirtualAddressRange;
            }
            break;

        case BlPae:

            Status = STATUS_NOT_SUPPORTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;

}
