/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/halinit.c
 * PURPOSE:         HAL Entrypoint and Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Share with Mm headers? */
#define MM_HAL_VA_START     (PVOID)0xFFC00000
#define MM_HAL_HEAP_START   (PVOID)((ULONG_PTR)MM_HAL_VA_START + (1024 * 1024))

BOOLEAN HalpPciLockSettings;
ULONG HalpUsedAllocDescriptors;
MEMORY_ALLOCATION_DESCRIPTOR HalpAllocationDescriptorArray[64];
PVOID HalpHeapStart = MM_HAL_HEAP_START;

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
NTAPI
HalpAllocPhysicalMemory(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                        IN ULONG MaxAddress,
                        IN ULONG PageCount,
                        IN BOOLEAN Aligned)
{
    ULONG UsedDescriptors, Alignment, PhysicalAddress;
    PFN_NUMBER MaxPage, BasePage;
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock, NewBlock, FreeBlock;
    
    /* Highest page we'll go */
    MaxPage = MaxAddress >> PAGE_SHIFT;
    
    /* We need at least two blocks */
    if ((HalpUsedAllocDescriptors + 2) > 64) return 0;
    
    /* Remember how many we have now */
    UsedDescriptors = HalpUsedAllocDescriptors;
        
    /* Loop the loader block memory descriptors */
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Get the block */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        
        /* No alignment by default */
        Alignment = 0;
        
        /* Unless requested, in which case we use a 64KB block alignment */
        if (Aligned) Alignment = ((MdBlock->BasePage + 0x0F) & ~0x0F) - MdBlock->BasePage;
        
        /* Search for free memory */
        if ((MdBlock->MemoryType == LoaderFree) ||
            (MdBlock->MemoryType == MemoryFirmwareTemporary))
        {
            /* Make sure the page is within bounds, including alignment */
            BasePage = MdBlock->BasePage;
            if ((BasePage) &&
                (MdBlock->PageCount >= PageCount + Alignment) &&
                (BasePage + PageCount + Alignment < MaxPage))
            {
                
                /* We found an address */
                PhysicalAddress = (BasePage + Alignment) << PAGE_SHIFT;
                break;
            }
        }
        
        /* Keep trying */
        NextEntry = NextEntry->Flink;
    }
    
    /* If we didn't find anything, get out of here */
    if (NextEntry == &LoaderBlock->MemoryDescriptorListHead) return 0;
    
    /* Okay, now get a descriptor */
    NewBlock = &HalpAllocationDescriptorArray[HalpUsedAllocDescriptors];
    NewBlock->PageCount = PageCount;
    NewBlock->BasePage = MdBlock->BasePage + Alignment;
    NewBlock->MemoryType = LoaderHALCachedMemory;
    
    /* Update count */
    UsedDescriptors++;
    HalpUsedAllocDescriptors = UsedDescriptors;
    
    /* Check if we had any alignment */
    if (Alignment)
    {
        /* Check if we had leftovers */
        if ((MdBlock->PageCount - Alignment) != PageCount)
        {
            /* Get the next descriptor */
            FreeBlock = &HalpAllocationDescriptorArray[UsedDescriptors];
            FreeBlock->PageCount = MdBlock->PageCount - Alignment - PageCount;
            FreeBlock->BasePage = MdBlock->BasePage + Alignment + PageCount;
            
            /* One more */
            HalpUsedAllocDescriptors++;
            
            /* Insert it into the list */
            InsertHeadList(&MdBlock->ListEntry, &FreeBlock->ListEntry);
        }
        
        /* Use this descriptor */
        NewBlock->PageCount = Alignment;
        InsertHeadList(&MdBlock->ListEntry, &NewBlock->ListEntry);
    }
    else
    {
        /* Consume memory from this block */
        MdBlock->BasePage += PageCount;
        MdBlock->PageCount -= PageCount;
        
        /* Insert the descriptor */
        InsertTailList(&MdBlock->ListEntry, &NewBlock->ListEntry);

        /* Remove the entry if the whole block was allocated */
        if (!MdBlock->PageCount == 0) RemoveEntryList(&MdBlock->ListEntry);
    }

    /* Return the address */
    return PhysicalAddress;
}

PVOID
NTAPI
HalpMapPhysicalMemory64(IN PHYSICAL_ADDRESS PhysicalAddress,
                        IN ULONG PageCount)
{
    PHARDWARE_PTE PointerPte;
    ULONG UsedPages = 0;
    PVOID VirtualAddress, BaseAddress;

    /* Start at the current HAL heap base */
    BaseAddress = HalpHeapStart;
    VirtualAddress = BaseAddress;

    /* Loop until we have all the pages required */
    while (UsedPages < PageCount)
    {
        /* If this overflows past the HAL heap, it means there's no space */
        if (VirtualAddress == NULL) return NULL;

        /* Get the PTE for this address */
        PointerPte = HalAddressToPte(VirtualAddress);

        /* Go to the next page */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);

        /* Check if the page is available */
        if (PointerPte->Valid)
        {
            /* PTE has data, skip it and start with a new base address */
            BaseAddress = VirtualAddress;
            UsedPages = 0;
            continue;
        }

        /* PTE is available, keep going on this run */
        UsedPages++;
    }

    /* Take the base address of the page plus the actual offset in the address */
    VirtualAddress = (PVOID)((ULONG_PTR)BaseAddress +
                             BYTE_OFFSET(PhysicalAddress.LowPart));

    /* If we are starting at the heap, move the heap */
    if (BaseAddress == HalpHeapStart)
    {
        /* Past this allocation */
        HalpHeapStart = (PVOID)((ULONG_PTR)BaseAddress + (PageCount * PAGE_SIZE));
    }

    /* Loop pages that can be mapped */
    while (UsedPages--)
    {
        /* Fill out the PTE */
        PointerPte = HalAddressToPte(BaseAddress);
        PointerPte->PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;
        PointerPte->Valid = 1;
        PointerPte->Write = 1;

        /* Move to the next address */
        PhysicalAddress.QuadPart += PAGE_SIZE;
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PAGE_SIZE);
    }

    /* Flush the TLB and return the address */
    HalpFlushTLB();
    return VirtualAddress;
}

VOID
NTAPI
HalpUnmapVirtualAddress(IN PVOID VirtualAddress,
                        IN ULONG PageCount)
{
    PHARDWARE_PTE PointerPte;
    ULONG i;

    /* Only accept valid addresses */
    if (VirtualAddress < MM_HAL_VA_START) return;

    /* Align it down to page size */
    VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress & ~(PAGE_SIZE - 1));

    /* Loop PTEs */
    PointerPte = HalAddressToPte(VirtualAddress);
    for (i = 0; i < PageCount; i++)
    {
        *(PULONG)PointerPte = 0;
        PointerPte++;
    }

    /* Flush the TLB */
    HalpFlushTLB();

    /* Put the heap back */
    if (HalpHeapStart > VirtualAddress) HalpHeapStart = VirtualAddress;
}

VOID
NTAPI
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;

    /* Make sure we have a loader block and command line */
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        /* Read the command line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Check if PCI is locked */
        if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Check the boot phase */
    if (!BootPhase)
    {
        /* Phase 0... save bus type */
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xFF;

        /* Get command-line parameters */
        HalpGetParameters(LoaderBlock);

        /* Checked HAL requires checked kernel */
#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG))
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 1, 0);
        }
#else
        /* Release build requires release HAL */
        if (Prcb->BuildType & PRCB_BUILD_DEBUG)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

#ifdef CONFIG_SMP
        /* SMP HAL requires SMP kernel */
        if (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

        /* Validate the PRCB */
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* Validation failed, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, 1, 0);
        }

#ifndef _MINIHAL_
        /* Initialize ACPI */
        HalpSetupAcpiPhase0(LoaderBlock);

        /* Initialize the PICs */
        HalpInitializePICs(TRUE);
#endif

        /* Force initial PIC state */
        KfRaiseIrql(KeGetCurrentIrql());

        /* Initialize CMOS lock */
        KeInitializeSpinLock(&HalpSystemHardwareLock);

        /* Initialize CMOS */
        HalpInitializeCmos();

        /* Fill out the dispatch tables */
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = NULL; // FIXME: TODO
#ifndef _MINIHAL_
        HalGetDmaAdapter = HalpGetDmaAdapter;
#else
        HalGetDmaAdapter = NULL;
#endif
        HalGetInterruptTranslator = NULL;  // FIXME: TODO
#ifndef _MINIHAL_
        HalResetDisplay = HalpBiosDisplayReset;
#else
        HalResetDisplay = NULL;
#endif
        HalHaltSystem = HaliHaltSystem;

        /* Register IRQ 2 */
        HalpRegisterVector(IDT_INTERNAL,
                           PRIMARY_VECTOR_BASE + 2,
                           PRIMARY_VECTOR_BASE + 2,
                           HIGH_LEVEL);

        /* Setup I/O space */
        HalpDefaultIoSpace.Next = HalpAddressUsageList;
        HalpAddressUsageList = &HalpDefaultIoSpace;

        /* Setup busy waiting */
        HalpCalibrateStallExecution();

#ifndef _MINIHAL_
        /* Initialize the clock */
        HalpInitializeClock();
#endif

        /*
         * We could be rebooting with a pending profile interrupt,
         * so clear it here before interrupts are enabled
         */
        HalStopProfileInterrupt(ProfileTime);

        /* Do some HAL-specific initialization */
        HalpInitPhase0(LoaderBlock);
    }
    else if (BootPhase == 1)
    {
        /* Initialize bus handlers */
        HalpInitBusHandler();

#ifndef _MINIHAL_
        /* Enable IRQ 0 */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE,
                                   CLOCK2_LEVEL,
                                   HalpClockInterrupt,
                                   Latched);

        /* Enable IRQ 8 */
        HalpEnableInterruptHandler(IDT_DEVICE,
                                   0,
                                   PRIMARY_VECTOR_BASE + 8,
                                   PROFILE_LEVEL,
                                   HalpProfileInterrupt,
                                   Latched);

        /* Initialize DMA. NT does this in Phase 0 */
        HalpInitDma();
#endif

        /* Do some HAL-specific initialization */
        HalpInitPhase1();
    }

    /* All done, return */
    return TRUE;
}
