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

#define PTE_BASE                0xC0000000

//
// Specific PDE/PTE macros to be used inside the boot library environment
//
#define MiAddressToPte(x)       ((PMMPTE)(((((ULONG)(x)) >> 12) << 2) + (ULONG_PTR)MmPteBase))
#define MiAddressToPde(x)       ((PMMPDE)(((((ULONG)(x)) >> 22) << 2) + (ULONG_PTR)MmPdeBase))
#define MiAddressToPteOffset(x) ((((ULONG)(x)) << 10) >> 22)
#define MiAddressToPdeOffset(x) (((ULONG)(x)) / (1024 * PAGE_SIZE))

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

BOOLEAN
BlMmIsTranslationEnabled (
    VOID
    )
{
    /* Return if paging is on */
    return ((CurrentExecutionContext) &&
            (CurrentExecutionContext->ContextFlags & BL_CONTEXT_PAGING_ON));
}

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
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ PVOID VirtualAddress,
    _In_ ULONG Size,
    _In_ ULONG CacheAttributes
    )
{
    BOOLEAN Enabled;
    ULONG i, PageCount, PdeOffset;
    ULONGLONG CurrentAddress;
    PMMPDE Pde;
    PMMPTE Pte;
    PMMPTE PageTable;
    PHYSICAL_ADDRESS PageTableAddress;
    NTSTATUS Status;

    /* Check if paging is on yet */
    Enabled = BlMmIsTranslationEnabled();

    /* Get the physical address aligned */
    CurrentAddress = (PhysicalAddress.QuadPart >> PAGE_SHIFT) << PAGE_SHIFT;

    /* Get the number of pages and loop through each one */
    PageCount = Size >> PAGE_SHIFT;
    for (i = 0; i < PageCount; i++)
    {
        /* Check if translation already exists for this page */
        if (Mmx86TranslateVirtualAddress(VirtualAddress, NULL, NULL))
        {
            /* Ignore it and move to the next one */
            VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);
            CurrentAddress += PAGE_SIZE;
            continue;
        }

        /* Get the PDE offset */
        PdeOffset = MiAddressToPdeOffset(VirtualAddress);

        /* Check if paging is actually turned on */
        if (Enabled)
        {
            /* Get the PDE entry using the self-map */
            Pde = MiAddressToPde(VirtualAddress);
        }
        else
        {
            /* Get it using our physical mappings */
            Pde = &MmPdpt[PdeOffset];
            PageTable = (PMMPDE)(Pde->u.Hard.PageFrameNumber << PAGE_SHIFT);
        }

        /* Check if we don't yet have a PDE */
        if (!Pde->u.Hard.Valid)
        {
            /* Allocate a page table */
            Status = MmPapAllocatePhysicalPagesInRange(&PageTableAddress,
                                                       BlLoaderPageDirectory,
                                                       1,
                                                       0,
                                                       0,
                                                       &MmMdlUnmappedAllocated,
                                                       0,
                                                       0);
            if (!NT_SUCCESS(Status))
            {
                EfiPrintf(L"PDE alloc failed!\r\n");
                EfiStall(1000000);
                return STATUS_NO_MEMORY;
            }

            /* This is our page table */
            PageTable = (PVOID)(ULONG_PTR)PageTableAddress.QuadPart;

            /* Build the PDE for it */
            Pde->u.Hard.PageFrameNumber = PageTableAddress.QuadPart >> PAGE_SHIFT;
            Pde->u.Hard.Write = 1;
            Pde->u.Hard.CacheDisable = 1;
            Pde->u.Hard.WriteThrough = 1;
            Pde->u.Hard.Valid = 1;

            /* Check if paging is enabled */
            if (Enabled)
            {
                /* Then actually, get the page table's virtual address */
                PageTable = (PVOID)PAGE_ROUND_DOWN(MiAddressToPte(VirtualAddress));

                /* Flush the TLB */
                Mmx86FlushTlb();
            }

            /* Zero out the page table */
            RtlZeroMemory(PageTable, PAGE_SIZE);

            /* Reset caching attributes now */
            Pde->u.Hard.CacheDisable = 0;
            Pde->u.Hard.WriteThrough = 0;

            /* Check for paging again */
            if (Enabled)
            {
                /* Flush the TLB entry for the page table only */
                Mmx86FlushTlbEntry(PageTable);
            }
        }

        /* Add a reference to this page table */
        MmArchReferencePage[PdeOffset]++;

        /* Check if a physical address was given */
        if (PhysicalAddress.QuadPart != -1)
        {
            /* Check if paging is turned on */
            if (Enabled)
            {
                /* Get the PTE using the self-map */
                Pte = MiAddressToPte(VirtualAddress);
            }
            else
            {
                /* Get the PTE using physical addressing */
                Pte = &PageTable[MiAddressToPteOffset(VirtualAddress)];
            }

            /* Build a valid PTE for it */
            Pte->u.Hard.PageFrameNumber = CurrentAddress >> PAGE_SHIFT;
            Pte->u.Hard.Write = 1;
            Pte->u.Hard.Valid = 1;

            /* Check if this is uncached */
            if (CacheAttributes == BlMemoryUncached)
            {
                /* Set the flags */
                Pte->u.Hard.CacheDisable = 1;
                Pte->u.Hard.WriteThrough = 1;
            }
            else if (CacheAttributes == BlMemoryWriteThrough)
            {
                /* It's write-through, set the flag */
                Pte->u.Hard.WriteThrough = 1;
            }
        }

        /* Move to the next physical/virtual address */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);
        CurrentAddress += PAGE_SIZE;
    }

    /* All done! */
    return STATUS_SUCCESS;
}

BOOLEAN
MmDefpTranslateVirtualAddress (
    _In_ PVOID VirtualAddress,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress,
    _Out_opt_ PULONG CacheAttributes
    )
{
    PMMPDE Pde;
    PMMPTE Pte;
    PMMPTE PageTable;
    BOOLEAN Enabled;

    /* Is there no page directory yet? */
    if (!MmPdpt)
    {
        return FALSE;
    }

    /* Is paging enabled? */
    Enabled = BlMmIsTranslationEnabled();

    /* Check if paging is actually turned on */
    if (Enabled)
    {
        /* Get the PDE entry using the self-map */
        Pde = MiAddressToPde(VirtualAddress);
    }
    else
    {
        /* Get it using our physical mappings */
        Pde = &MmPdpt[MiAddressToPdeOffset(VirtualAddress)];
    }

    /* Is the PDE valid? */
    if (!Pde->u.Hard.Valid)
    {
        return FALSE;
    }

    /* Check if paging is turned on */
    if (Enabled)
    {
        /* Get the PTE using the self-map */
        Pte = MiAddressToPte(VirtualAddress);
    }
    else
    {
        /* Get the PTE using physical addressing */
        PageTable = (PMMPTE)(Pde->u.Hard.PageFrameNumber << PAGE_SHIFT);
        Pte = &PageTable[MiAddressToPteOffset(VirtualAddress)];
    }

    /* Is the PTE valid? */
    if (!Pte->u.Hard.Valid)
    {
        return FALSE;
    }

    /* Does caller want the physical address?  */
    if (PhysicalAddress)
    {
        /* Return it */
        PhysicalAddress->QuadPart = (Pte->u.Hard.PageFrameNumber << PAGE_SHIFT) +
                                     BYTE_OFFSET(VirtualAddress);
    }

    /* Does caller want cache attributes? */
    if (CacheAttributes)
    {
        /* Not yet -- lie and say it's cached */
        EfiPrintf(L"Cache checking not yet enabled\r\n");
        *CacheAttributes = BlMemoryWriteBack;
    }

    /* It exists! */
    return TRUE;
}

NTSTATUS
MmMapPhysicalAddress (
    _Inout_ PPHYSICAL_ADDRESS PhysicalAddressPtr,
    _Inout_ PVOID* VirtualAddressPtr,
    _Inout_ PULONGLONG SizePtr,
    _In_ ULONG CacheAttributes
    )
{
    ULONGLONG Size;
    ULONGLONG PhysicalAddress;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS TranslatedAddress;
    ULONG_PTR CurrentAddress, VirtualAddressEnd;
    NTSTATUS Status;

    /* Fail if any parameters are missing */
    if (!(PhysicalAddressPtr) || !(VirtualAddressPtr) || !(SizePtr))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Fail if the size is over 32-bits */
    Size = *SizePtr;
    if (Size > 0xFFFFFFFF)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Nothing to do if we're in physical mode */
    if (MmTranslationType == BlNone)
    {
        return STATUS_SUCCESS;
    }

    /* Can't use virtual memory in real mode */
    if (CurrentExecutionContext->Mode == BlRealMode)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Capture the current virtual and physical addresses */
    VirtualAddress = *VirtualAddressPtr;
    PhysicalAddress = PhysicalAddressPtr->QuadPart;

    /* Check if a physical address was requested */
    if (PhysicalAddress != 0xFFFFFFFF)
    {
        /* Round down the base addresses */
        PhysicalAddress = PAGE_ROUND_DOWN(PhysicalAddress);
        VirtualAddress = (PVOID)PAGE_ROUND_DOWN(VirtualAddress);

        /* Round up the size */
        Size = ROUND_TO_PAGES(PhysicalAddressPtr->QuadPart -
                              PhysicalAddress +
                              Size);

        /* Loop every virtual page */
        CurrentAddress = (ULONG_PTR)VirtualAddress;
        VirtualAddressEnd = CurrentAddress + Size - 1;
        while (CurrentAddress < VirtualAddressEnd)
        {
            /* Get the physical page of this virtual page */
            if (MmArchTranslateVirtualAddress((PVOID)CurrentAddress,
                                              &TranslatedAddress,
                                              &CacheAttributes))
            {
                /* Make sure the physical page of the virtual page, matches our page */
                if (TranslatedAddress.QuadPart !=
                    (PhysicalAddress +
                     (CurrentAddress - (ULONG_PTR)VirtualAddress)))
                {
                    /* There is an existing virtual mapping for a different address */
                    EfiPrintf(L"Existing mapping exists: %lx vs %lx\r\n",
                              TranslatedAddress.QuadPart,
                              PhysicalAddress + (CurrentAddress - (ULONG_PTR)VirtualAddress));
                    EfiStall(10000000);
                    return STATUS_INVALID_PARAMETER;
                }
            }

            /* Try the next one */
            CurrentAddress += PAGE_SIZE;
        }
    }

    /* Aactually do the mapping */
    TranslatedAddress.QuadPart = PhysicalAddress;
    Status = Mmx86MapPhysicalAddress(TranslatedAddress,
                                     VirtualAddress,
                                     Size,
                                     CacheAttributes);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Failed to map!: %lx\r\n", Status);
        EfiStall(1000000);
        return Status;
    }

    /* Return aligned/fixed up output parameters */
    PhysicalAddressPtr->QuadPart = PhysicalAddress;
    *VirtualAddressPtr = VirtualAddress;
    *SizePtr = Size;

    /* Flush the TLB if paging is enabled */
    if (BlMmIsTranslationEnabled())
    {
        Mmx86FlushTlb();
    }

    /* All good! */
    return STATUS_SUCCESS;
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

VOID
MmMdDbgDumpList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST DescriptorList,
    _In_opt_ ULONG MaxCount
    )
{
    ULONGLONG EndPage, VirtualEndPage;
    PBL_MEMORY_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextEntry;

    /* If no maximum was provided, use essentially infinite */
    if (MaxCount == 0)
    {
        MaxCount = 0xFFFFFFFF;
    }

    /* Loop the list as long as there's entries and max isn't reached */
    NextEntry = DescriptorList->First->Flink;
    while ((NextEntry != DescriptorList->First) && (MaxCount--))
    {
        /* Get the descriptor */
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             BL_MEMORY_DESCRIPTOR,
                                             ListEntry);

        /* Get the descriptor end page, and see if it was virtually mapepd */
        EndPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
        if (MemoryDescriptor->VirtualPage)
        {
            /* Get the virtual end page too, then */
            VirtualEndPage = MemoryDescriptor->VirtualPage +
                             MemoryDescriptor->PageCount;
        }
        else
        {
            VirtualEndPage = 0;
        }

        /* Print out the descriptor, physical range, virtual range, and type */
        EfiPrintf(L"%p - [%08llx-%08llx @ %08llx-%08llx]:%x\r\n",
                    MemoryDescriptor,
                    MemoryDescriptor->BasePage << PAGE_SHIFT,
                    (EndPage << PAGE_SHIFT) - 1,
                    MemoryDescriptor->VirtualPage << PAGE_SHIFT,
                    VirtualEndPage ? (VirtualEndPage << PAGE_SHIFT) - 1 : 0,
                    (ULONG)MemoryDescriptor->Type);

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }
}

NTSTATUS
Mmx86pMapMemoryRegions (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData
    )
{
    BOOLEAN DoDeferred;
    ULONG DescriptorCount;
    PBL_MEMORY_DESCRIPTOR Descriptor;
    ULONG FinalOffset;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONGLONG Size;
    NTSTATUS Status;
    PVOID VirtualAddress;
    BL_MEMORY_DESCRIPTOR_LIST FirmwareMdl;
    PLIST_ENTRY Head, NextEntry;

    /* Check which phase this is */
    if (Phase == 1)
    {
        /* In phase 1 we don't initialize deferred mappings */
        DoDeferred = FALSE;
    }
    else
    {
        /* Don't do anything if there's nothing to initialize */
        if (!MmDeferredMappingCount)
        {
            return STATUS_SUCCESS;
        }

        /* We'll do deferred descriptors in phase 2 */
        DoDeferred = TRUE;
    }

    /*
    * Because BL supports cross x86-x64 application launches and a LIST_ENTRY
    * is of variable size, care must be taken here to ensure that we see a
    * consistent view of descriptors. BL uses some offset magic to figure out
    * where the data actually starts, since everything is ULONGLONG past the
    * LIST_ENTRY itself
    */
    FinalOffset = MemoryData->MdListOffset + MemoryData->DescriptorOffset;
    Descriptor = (PBL_MEMORY_DESCRIPTOR)((ULONG_PTR)MemoryData + FinalOffset -
                                         FIELD_OFFSET(BL_MEMORY_DESCRIPTOR, BasePage));

    /* Scan all of them */
    DescriptorCount = MemoryData->DescriptorCount;
    while (DescriptorCount != 0)
    {
        /* Ignore application data */
        if (Descriptor->Type != BlApplicationData)
        {
            /* If this is a ramdisk, do it in phase 2 */
            if ((Descriptor->Type == BlLoaderRamDisk) == DoDeferred)
            {
                /* Get the current physical address and size */
                PhysicalAddress.QuadPart = Descriptor->BasePage << PAGE_SHIFT;
                Size = Descriptor->PageCount << PAGE_SHIFT;

                /* Check if it was already mapped */
                if (Descriptor->VirtualPage)
                {
                    /* Use the existing address */
                    VirtualAddress = (PVOID)(ULONG_PTR)(Descriptor->VirtualPage << PAGE_SHIFT);
                }
                else
                {
                    /* Use the physical address */
                    VirtualAddress = (PVOID)(ULONG_PTR)PhysicalAddress.QuadPart;
                }

                /* Crete the mapping */
                Status = Mmx86MapInitStructure(VirtualAddress,
                                               Size,
                                               PhysicalAddress);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }

            /* Check if we're in phase 1 and deferring RAM disk */
            if ((Phase == 1) && (Descriptor->Type == BlLoaderRamDisk))
            {
                MmDeferredMappingCount++;
            }
        }

        /* Move on to the next descriptor */
        DescriptorCount--;
        Descriptor = (PBL_MEMORY_DESCRIPTOR)((ULONG_PTR)Descriptor + MemoryData->DescriptorSize);
    }

    /* In phase 1, also do UEFI mappings */
    if (Phase != 2)
    {
        /* Get the memory map */
        MmMdInitializeListHead(&FirmwareMdl);
        Status = MmFwGetMemoryMap(&FirmwareMdl, BL_MM_FLAG_REQUEST_COALESCING);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Iterate over it */
        Head = FirmwareMdl.First;
        NextEntry = Head->Flink;
        while (NextEntry != Head)
        {
            /* Check if this is a UEFI-related descriptor, unless it's the self-map page */
            Descriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);
            if (((Descriptor->Type == BlEfiBootMemory) ||
                 (Descriptor->Type == BlEfiRuntimeCodeMemory) ||
                 (Descriptor->Type == BlEfiRuntimeDataMemory) || // WINBUG?
                 (Descriptor->Type == BlLoaderMemory)) &&
                ((Descriptor->BasePage << PAGE_SHIFT) != Mmx86SelfMapBase.QuadPart))
            {
                /* Identity-map it */
                PhysicalAddress.QuadPart = Descriptor->BasePage << PAGE_SHIFT;
                Status = Mmx86MapInitStructure((PVOID)((ULONG_PTR)Descriptor->BasePage << PAGE_SHIFT),
                                               Descriptor->PageCount << PAGE_SHIFT,
                                               PhysicalAddress);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }

            /* Move to the next descriptor */
            NextEntry = NextEntry->Flink;
        }

        /* Reset */
        NextEntry = Head->Flink;
        while (NextEntry != Head)
        {
            /* Get the descriptor */
            Descriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

            /* Skip to the next entry before we free */
            NextEntry = NextEntry->Flink;

            /* Remove and free it */
            MmMdRemoveDescriptorFromList(&FirmwareMdl, Descriptor);
            MmMdFreeDescriptor(Descriptor);
        }
    }

    /* All library mappings identity mapped now */
    return STATUS_SUCCESS;
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
    return Mmx86pMapMemoryRegions(Phase, MemoryData);
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

    /* Truncate all memory above 4GB so that we don't use it */
    Status = MmPaTruncateMemory(0x100000);
    if (!NT_SUCCESS(Status))
    {
        goto Failure;
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
        goto Failure;
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
        goto Failure;
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
        goto Failure;
    }

    /* Zero them out */
    RtlZeroMemory((PVOID)Mmx86SelfMapBase.LowPart, 4 * 1024 * 1024);
    EfiPrintf(L"PDPT at 0x%p Reference Page at 0x%p Self-map at 0x%p\r\n",
              MmPdpt, MmArchReferencePage, Mmx86SelfMapBase.LowPart);

    /* Align PTE base to 4MB region */
    MmPteBase = (PVOID)(Mmx86SelfMapBase.LowPart & ~0x3FFFFF);

    /* The PDE is the PTE of the PTE base */
    MmPdeBase = MiAddressToPte(MmPteBase);
    PdeIndex = MiAddressToPdeOffset(MmPdeBase);
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
        goto Failure;
    }

    /* Remove HAL_HEAP from free virtual memory */
    Status = MmMdRemoveRegionFromMdlEx(&MmMdlFreeVirtual,
                                       BL_MM_REMOVE_VIRTUAL_REGION_FLAG,
                                       MM_HAL_VA_START >> PAGE_SHIFT,
                                       (4 * 1024 * 1024) >> PAGE_SHIFT,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        goto Failure;
    }

    /* Initialize the virtual->physical memory mappings */
    Status = Mmx86InitializeMemoryMap(1, MemoryData);
    if (!NT_SUCCESS(Status))
    {
        goto Failure;
    }

    /* Turn on paging with the new CR3 */
    __writecr3((ULONG_PTR)MmPdpt);
    BlpArchEnableTranslation();
    EfiPrintf(L"Paging... %d\r\n", BlMmIsTranslationEnabled());

    /* Return success */
    return Status;

Failure:
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
    CPU_INFO CpuInfo;

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
                BlArchCpuId(1, 0, &CpuInfo);

                /* Check if RDRAND is supported */
                if (CpuInfo.Ecx & 0x40000000)
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
            MmArchTopOfApplicationAddressSpace = 0x70000000 - 1; // Windows bug

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

            /* We don't support PAE */
            Status = STATUS_NOT_SUPPORTED;
            break;

        default:

            /* Invalid architecture type*/
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Back to caller */
    return Status;
}
