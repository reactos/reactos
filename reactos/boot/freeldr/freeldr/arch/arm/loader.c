/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/loader.c
 * PURPOSE:         ARM Kernel Loader
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <internal/arm/ke.h>
#include <internal/arm/mm.h>
#include <internal/arm/intrin_i.h>

#define KERNEL_DESCRIPTOR_PAGE(x) (((ULONG_PTR)x &~ KSEG0_BASE) >> PAGE_SHIFT)

/* GLOBALS ********************************************************************/

typedef struct _BIOS_MEMORY_DESCRIPTOR
{
    ULONG BlockBase;
    ULONG BlockSize;
} BIOS_MEMORY_DESCRIPTOR, *PBIOS_MEMORY_DESCRIPTOR;

ULONG PageDirectoryStart, PageDirectoryEnd;
PLOADER_PARAMETER_BLOCK ArmLoaderBlock;
CHAR ArmCommandLine[256];
CHAR ArmArcBootPath[64];
CHAR ArmArcHalPath[64];
CHAR ArmNtHalPath[64];
CHAR ArmNtBootPath[64];
PNLS_DATA_BLOCK ArmNlsDataBlock;
PLOADER_PARAMETER_EXTENSION ArmExtension;
BIOS_MEMORY_DESCRIPTOR ArmBoardMemoryDescriptors[16] = {{0}};
PBIOS_MEMORY_DESCRIPTOR ArmBoardMemoryList = ArmBoardMemoryDescriptors;
ULONG NumberDescriptors = 0;
MEMORY_DESCRIPTOR MDArray[16] = {{0}};
ULONG ArmSharedHeapSize;
PCHAR ArmSharedHeap;

extern PAGE_DIRECTORY_ARM startup_pagedirectory;
extern PAGE_TABLE_ARM kernel_pagetable;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;
extern ULONG_PTR KernelBase;

extern ADDRESS_RANGE ArmBoardMemoryMap[16];
extern ULONG ArmBoardMemoryMapRangeCount;
extern ULONG_PTR AnsiData, OemData, UnicodeData, RegistryData, KernelData, HalData, DriverData[16];
extern ULONG RegistrySize, AnsiSize, OemSize, UnicodeSize, KernelSize, HalSize, DriverSize[16];
extern PCHAR DriverName[16];
extern ULONG Drivers;
extern ULONG BootStack;

ULONG SizeBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    1 << 12, // 4KB
    1 << 13, // 8KB
    1 << 14, // 16KB
    1 << 15, // 32KB
    1 << 16, // 64KB
    1 << 17  // 128KB
};

ULONG AssocBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    4        // 4-way associative
};

ULONG LenBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    8        // 8 words per line (32 bytes)
};

//
// Where to map the serial port
//
#define UART_VIRTUAL 0xE0000000

/* FUNCTIONS ******************************************************************/

PVOID
ArmAllocateFromSharedHeap(IN ULONG Size)
{
    PVOID Buffer;

    //
    // Allocate from the shared heap
    //
    Buffer = &ArmSharedHeap[ArmSharedHeapSize];
    ArmSharedHeapSize += Size;
    return Buffer;
}

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
ArmAllocateMemoryDescriptor(VOID)
{
    //
    // Allocate a descriptor from the heap
    //
    return ArmAllocateFromSharedHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
}

VOID
NTAPI
ArmAddBoardMemoryDescriptor(IN ULONG Address,
                            IN ULONG Size)
{
    PBIOS_MEMORY_DESCRIPTOR BiosBlock = ArmBoardMemoryList;
    
    //
    // Loop board DRAM configuration
    //
    while (BiosBlock->BlockSize > 0)
    {
        /* Check if we've found a matching head block */
        if (Address + Size == BiosBlock->BlockBase)
        {
            /* Simply enlarge and rebase it */
            BiosBlock->BlockBase = Address;
            BiosBlock->BlockSize += Size;
            break;
        }
        
        /* Check if we've found a matching tail block */
        if (Address == (BiosBlock->BlockBase + BiosBlock->BlockSize))
        {
            /* Simply enlarge it */
            BiosBlock->BlockSize += Size;
            break;
        }
        
        /* Nothing suitable found, try the next block */
        BiosBlock++;
    }
    
    /* No usable blocks found, found a free block instead */
    if (!BiosBlock->BlockSize)
    {
        /* Write our data */
        BiosBlock->BlockBase = Address;
        BiosBlock->BlockSize = Size;
        
        /* Create a new block and mark it as the end of the array */
        BiosBlock++;
        BiosBlock->BlockBase = BiosBlock->BlockSize = 0L;
    }
}

VOID
NTAPI
ArmBuildBoardMemoryMap(VOID)
{
    ULONG BlockBegin, BlockEnd;
    ULONG j;
    
    /* Loop the BIOS Memory Map */
    for (j = 0; j < ArmBoardMemoryMapRangeCount; j++)
    {
        /* Get the start and end addresses */
        BlockBegin = ArmBoardMemoryMap[j].BaseAddrLow;
        BlockEnd = ArmBoardMemoryMap[j].BaseAddrLow + ArmBoardMemoryMap[j].LengthLow - 1;
        
        /* Make sure this isn't a > 4GB descriptor */
        if (!ArmBoardMemoryMap[j].BaseAddrHigh)
        {
            /* Make sure we don't overflow */
            if (BlockEnd < BlockBegin) BlockEnd = 0xFFFFFFFF;
            
            /* Check if this is free memory */
            if (ArmBoardMemoryMap[j].Type == 1)
            {
                /* Add it to our BIOS descriptors */
                ArmAddBoardMemoryDescriptor(BlockBegin, BlockEnd - BlockBegin + 1);
            }
        }
    }
}

NTSTATUS
NTAPI
ArmConfigureArcDescriptor(IN ULONG PageBegin,
                          IN ULONG PageEnd,
                          IN TYPE_OF_MEMORY MemoryType)
{
    ULONG i;
    ULONG BlockBegin, BlockEnd;
    MEMORY_TYPE BlockType;
    BOOLEAN Combined = FALSE;
    
    /* If this descriptor seems bogus, just return */
    if (PageEnd <= PageBegin) return STATUS_SUCCESS;
    
    /* Loop every ARC descriptor, trying to find one we can modify */
    for (i = 0; i < NumberDescriptors; i++)
    {
        /* Get its settings */
        BlockBegin = MDArray[i].BasePage;
        BlockEnd = MDArray[i].BasePage + MDArray[i].PageCount;
        BlockType = MDArray[i].MemoryType;
        
        /* Check if we can fit inside this block */
        if (BlockBegin < PageBegin)
        {
            /* Check if we are larger then it */
            if ((BlockEnd > PageBegin) && (BlockEnd <= PageEnd))
            {
                /* Make it end where we start */
                BlockEnd = PageBegin;
            }
            
            /* Check if it ends after we do */
            if (BlockEnd > PageEnd)
            {
                /* Make sure we can allocate a descriptor */
                if (NumberDescriptors == 60) return ENOMEM;
                
                /* Create a descriptor for whatever memory we're not part of */
                MDArray[NumberDescriptors].MemoryType = BlockType;
                MDArray[NumberDescriptors].BasePage = PageEnd;
                MDArray[NumberDescriptors].PageCount  = BlockEnd - PageEnd;
                NumberDescriptors++;
                
                /* The next block ending is now where we begin */
                BlockEnd = PageBegin;
            }
        }
        else
        {
            /* Check if the blog begins inside our range */
            if (BlockBegin < PageEnd)
            {
                /* Check if it ends before we do */
                if (BlockEnd < PageEnd)
                {
                    /* Then make it disappear */
                    BlockEnd = BlockBegin;
                }
                else
                {
                    /* Otherwise make it start where we end */
                    BlockBegin = PageEnd;
                }
            }
        }
        
        /* Check if the block matches us, and we haven't tried combining yet */
        if ((BlockType == MemoryType) && !(Combined))
        {
            /* Check if it starts where we end */
            if (BlockBegin == PageEnd)
            {
                /* Make it start with us, and combine us */
                BlockBegin = PageBegin;
                Combined = TRUE;
            }
            else if (BlockEnd == PageBegin)
            {
                /* Otherwise, it ends where we begin, combine its ending */
                BlockEnd = PageEnd;
                Combined = TRUE;
            }
        }
        
        /* Check the original block data matches with what we came up with */
        if ((MDArray[i].BasePage == BlockBegin) &&
            (MDArray[i].PageCount == BlockEnd - BlockBegin))
        {
            /* Then skip it */
            continue;
        }
        
        /* Otherwise, set our new settings for this block */
        MDArray[i].BasePage  = BlockBegin;
        MDArray[i].PageCount = BlockEnd - BlockBegin;
        
        /* Check if we are killing the block */
        if (BlockBegin == BlockEnd)
        {
            /* Delete this block and restart the loop properly */
            NumberDescriptors--;
            if (i < NumberDescriptors) MDArray[i] = MDArray[NumberDescriptors];
            i--;
        }
    }
    
    /* If we got here without combining, we need to allocate a new block */
    if (!(Combined) && (MemoryType < LoaderMaximum))
    {
        /* Make sure there's enough descriptors */
        if (NumberDescriptors == 60) return ENOMEM;
        
        /* Allocate a new block with our data */
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount  = PageEnd - PageBegin;
        NumberDescriptors++;
    }
    
    /* Changes complete, return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ArmBuildOsMemoryMap(VOID)
{
    PBIOS_MEMORY_DESCRIPTOR MdBlock;
    ULONG BlockStart, BlockEnd, BiasedStart, BiasedEnd, PageStart, PageEnd;
    NTSTATUS Status = STATUS_SUCCESS;
    
    /* Loop the BIOS Memory Descriptor List */
    MdBlock = ArmBoardMemoryList;
    while (MdBlock->BlockSize)
    {
        /* Get the statrt and end addresses */
        BlockStart = MdBlock->BlockBase;
        BlockEnd = BlockStart + MdBlock->BlockSize - 1;
        
        /* Align them to page boundaries */
        BiasedStart = BlockStart & (PAGE_SIZE - 1);
        if (BiasedStart) BlockStart = BlockStart + PAGE_SIZE - BiasedStart;
        BiasedEnd = (BlockEnd + 1) & (ULONG)(PAGE_SIZE - 1);
        if (BiasedEnd) BlockEnd -= BiasedEnd;
        
        /* Get the actual page numbers */
        PageStart = BlockStart >> PAGE_SHIFT;
        PageEnd = (BlockEnd + 1) >> PAGE_SHIFT;

        /* Check if we did any alignment */
        if (BiasedStart)
        {
            /* Mark that region as reserved */
            Status = ArmConfigureArcDescriptor(PageStart - 1,
                                               PageStart,
                                               MemorySpecialMemory);
            if (Status != STATUS_SUCCESS) break;
        }
        
        /* Check if we did any alignment */
        if (BiasedEnd)
        {
            /* Mark that region as reserved */
            Status = ArmConfigureArcDescriptor(PageEnd - 1,
                                               PageEnd,
                                               MemorySpecialMemory);
            if (Status != STATUS_SUCCESS) break;
        }
        
        /* It is, mark the memory a free */
        Status = ArmConfigureArcDescriptor(PageStart,
                                           PageEnd,
                                           LoaderFree);
        
        /* If we failed, break out, otherwise, go to the next BIOS block */
        if (Status != STATUS_SUCCESS) break;
        MdBlock++;
    }
    
    /* Return error code */
    return Status;
}

VOID
NTAPI
ArmInsertMemoryDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
    PLIST_ENTRY ListHead, PreviousEntry, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor = NULL, NextDescriptor = NULL;
    
    /* Loop the memory descriptor list */
    ListHead = &ArmLoaderBlock->MemoryDescriptorListHead;
    PreviousEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current descriptor and check if it's below ours */
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);
        if (NewDescriptor->BasePage < NextDescriptor->BasePage) break;
        
        /* It isn't, save the previous entry and descriptor, and try again */
        PreviousEntry = NextEntry;
        Descriptor = NextDescriptor;
        NextEntry = NextEntry->Flink;
    }
    
    /* So we found the right spot to insert. Is this free memory? */
    if (NewDescriptor->MemoryType != LoaderFree)
    {
        /* It isn't, so insert us before the last descriptor */
        InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
    }
    else
    {
        /* We're free memory. Check if the entry we found is also free memory */
        if ((PreviousEntry != ListHead) &&
            ((Descriptor->MemoryType == LoaderFree) ||
             (Descriptor->MemoryType == LoaderReserve)) &&
            ((Descriptor->BasePage + Descriptor->PageCount) ==
             NewDescriptor->BasePage))
        {
            /* It's free memory, and we're right after it. Enlarge that block */
            Descriptor->PageCount += NewDescriptor->PageCount;
            NewDescriptor = Descriptor;
        }
        else
        {
            /* Our range scan't be combined, so just insert us separately */
            InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
        }
        
        /* Check if we merged with an existing free memory block */
        if ((NextEntry != ListHead) &&
            ((NextDescriptor->MemoryType == LoaderFree) ||
             (NextDescriptor->MemoryType == LoaderReserve)) &&
            ((NewDescriptor->BasePage + NewDescriptor->PageCount) ==
             NextDescriptor->BasePage))
        {
            /* Update our own block */
            NewDescriptor->PageCount += NextDescriptor->PageCount;
            
            /* Remove the next block */
            RemoveEntryList(&NextDescriptor->ListEntry);
        }
    }
}

NTSTATUS
NTAPI
ArmBuildMemoryDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
                         IN MEMORY_TYPE MemoryType,
                         IN ULONG BasePage,
                         IN ULONG PageCount)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor, NextDescriptor = NULL;
    LONG Delta;
    TYPE_OF_MEMORY CurrentType;
    BOOLEAN UseNext;
    
    /* Check how many pages we'll be consuming */
    Delta = BasePage - MemoryDescriptor->BasePage;
    if (!(Delta) && (PageCount == MemoryDescriptor->PageCount))
    {
        /* We can simply convert the current descriptor into our new type */
        MemoryDescriptor->MemoryType = MemoryType;
    }
    else
    {
        /* Get the current memory type of the descriptor, and reserve it */
        CurrentType = MemoryDescriptor->MemoryType;
        MemoryDescriptor->MemoryType = LoaderSpecialMemory;
        
        /* Check if we'll need another descriptor for what's left of memory */
        UseNext = ((BasePage != MemoryDescriptor->BasePage) &&
                   (Delta + PageCount != MemoryDescriptor->PageCount));
        
        /* Get a descriptor */
        Descriptor = ArmAllocateMemoryDescriptor();
        if (!Descriptor) return STATUS_INSUFFICIENT_RESOURCES;
        
        /* Check if we are using another descriptor */
        if (UseNext)
        {
            /* Allocate that one too */
            NextDescriptor = ArmAllocateMemoryDescriptor();
            if (!NextDescriptor) return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        /* Build the descriptor we got */
        Descriptor->MemoryType = MemoryType;
        Descriptor->BasePage = BasePage;
        Descriptor->PageCount = PageCount;
        
        /* Check if we're starting at the same place as the old one */
        if (BasePage == MemoryDescriptor->BasePage)
        {
            /* Simply decrease the old descriptor and rebase it */
            MemoryDescriptor->BasePage += PageCount;
            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = CurrentType;
        }
        else if (Delta + PageCount == MemoryDescriptor->PageCount)
        {
            /* We finish where the old one did, shorten it */
            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = CurrentType;
        }
        else
        {
            /* We're inside the current block, mark our free region */
            NextDescriptor->MemoryType = LoaderFree;
            NextDescriptor->BasePage = BasePage + PageCount;
            NextDescriptor->PageCount = MemoryDescriptor->PageCount -
            (PageCount + Delta);
            
            /* And cut down the current descriptor */
            MemoryDescriptor->PageCount = Delta;
            MemoryDescriptor->MemoryType = CurrentType;
            
            /* Finally, insert our new free descriptor into the list */
            ArmInsertMemoryDescriptor(NextDescriptor);
        }
        
        /* Insert the descriptor we allocated */
        ArmInsertMemoryDescriptor(Descriptor);
    }
    
    /* Return success */
    return STATUS_SUCCESS;
}

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
ArmFindMemoryDescriptor(IN ULONG BasePage)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock = NULL;
    PLIST_ENTRY NextEntry, ListHead;
    
    /* Scan the memory descriptor list */
    ListHead = &ArmLoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current descriptor */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Check if it can contain our memory range */
        if ((MdBlock->BasePage <= BasePage) &&
            (MdBlock->BasePage + MdBlock->PageCount > BasePage))
        {
            /* It can, break out */
            break;
        }
        
        /* Go to the next descriptor */
        NextEntry = NextEntry->Flink;
    }
    
    /* Return the descriptor we found, if any */
    return MdBlock;
}

NTSTATUS
NTAPI
ArmCreateMemoryDescriptor(IN TYPE_OF_MEMORY MemoryType,
                      IN ULONG BasePage,
                      IN ULONG PageCount,
                      IN ULONG Alignment,
                      OUT PULONG ReturnedBase)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG AlignedBase, AlignedLimit;
    PMEMORY_ALLOCATION_DESCRIPTOR ActiveMdBlock;
    ULONG ActiveAlignedBase = 0;
    PLIST_ENTRY NextEntry, ListHead;
    
    /* If no information was given, make some assumptions */
    if (!Alignment) Alignment = 1;
    if (!PageCount) PageCount = 1;
    
    /* Start looking for a matching descvriptor */
    do
    {
        /* Calculate the limit of the range */
        AlignedLimit = PageCount + BasePage;
        
        /* Find a descriptor that already contains our base address */
        MdBlock = ArmFindMemoryDescriptor(BasePage);
        if (MdBlock)
        {
            /* If it contains our limit as well, break out early */
            if ((MdBlock->PageCount + MdBlock->BasePage) >= AlignedLimit) break;
        }
        
        /* Loop the memory list */
        AlignedBase = 0;
        ActiveMdBlock = NULL;
        ListHead = &ArmLoaderBlock->MemoryDescriptorListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the current descriptors */
            MdBlock = CONTAINING_RECORD(NextEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry);
            
            /* Align the base address and our limit */
            AlignedBase = (MdBlock->BasePage + (Alignment - 1)) &~ Alignment;
            AlignedLimit = MdBlock->PageCount -
                           AlignedBase +
                           MdBlock->BasePage;
            
            /* Check if this is a free block that can satisfy us */
            if ((MdBlock->MemoryType == LoaderFree) &&
                (AlignedLimit <= MdBlock->PageCount) &&
                (PageCount <= AlignedLimit))
            {
                /* It is, stop searching */
                ActiveMdBlock = MdBlock;
                ActiveAlignedBase = AlignedBase;
                break;
            }
            
            /* Try the next block */
            NextEntry = NextEntry->Flink;
        }
        
        /* See if we came up with an adequate block */
        if (ActiveMdBlock)
        {
            /* Generate a descriptor in it */
            *ReturnedBase = AlignedBase;
            return ArmBuildMemoryDescriptor(ActiveMdBlock,
                                          MemoryType,
                                          ActiveAlignedBase,
                                          PageCount);
        }
    } while (TRUE);
    
    /* We found a matching block, generate a descriptor with it */
    *ReturnedBase = BasePage;
    return ArmBuildMemoryDescriptor(MdBlock, MemoryType, BasePage, PageCount);
}

NTSTATUS
NTAPI
ArmBuildLoaderMemoryList(VOID)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    MEMORY_DESCRIPTOR *Memory;
    ULONG i;
    
    /* Loop all BIOS Memory Descriptors */
    for (i = 0; i < NumberDescriptors; i++)
    {
        /* Get the current descriptor */
        Memory = &MDArray[i];
        
        /* Allocate an NT Memory Descriptor */
        Descriptor = ArmAllocateMemoryDescriptor();
        if (!Descriptor) return ENOMEM;
        
        /* Copy the memory type */
        Descriptor->MemoryType = Memory->MemoryType;
        if (Memory->MemoryType == MemoryFreeContiguous)
        {
            /* Convert this to free */
            Descriptor->MemoryType = LoaderFree;
        }
        else if (Memory->MemoryType == MemorySpecialMemory)
        {
            /* Convert this to special memory */
            Descriptor->MemoryType = LoaderSpecialMemory;
        }
        
        /* Copy the range data */
        Descriptor->BasePage = Memory->BasePage;
        Descriptor->PageCount = Memory->PageCount;
        
        /* Insert the descriptor */
        if (Descriptor->PageCount) ArmInsertMemoryDescriptor(Descriptor);
    }
    
    /* All went well */
    return STATUS_SUCCESS;
}

#define PFN_SHIFT                   12
#define LARGE_PFN_SHIFT             20

#define PTE_BASE                    0xC0000000
#define PDE_BASE                    0xC0400000
#define HAL_BASE                    0xFFC00000
#define MMIO_BASE                   0x10000000

#define LowMemPageTableIndex        0
#define StartupPtePageTableIndex    (PTE_BASE >> PDE_SHIFT)
#define StartupPdePageTableIndex    (PDE_BASE >> PDE_SHIFT)
#define MmioPageTableIndex          (MMIO_BASE >> PDE_SHIFT)
#define HalPageTableIndex           (HAL_BASE >> PDE_SHIFT)

/* Converts a Physical Address into a Page Frame Number */
#define PaToPfn(p)                  ((p) >> PFN_SHIFT)
#define PaToLargePfn(p)             ((p) >> LARGE_PFN_SHIFT)
#define PaPtrToPfn(p)               (((ULONG_PTR)(p)) >> PFN_SHIFT)

/* Converts a Physical Address into a Coarse Page Table PFN */
#define PaPtrToPdePfn(p)            (((ULONG_PTR)(p)) >> CPT_SHIFT)

HARDWARE_PTE_ARMV6 TempPte;
HARDWARE_LARGE_PTE_ARMV6 TempLargePte;
HARDWARE_PDE_ARMV6 TempPde;

PVOID
ArmSetupPageDirectory(VOID)
{
    PPAGE_DIRECTORY_ARM PageDir;
    PPAGE_TABLE_ARM PageTable, KernelPageTable;
    ULONG KernelPageTableIndex;
    ULONG i;
    PHARDWARE_PTE_ARMV6 PointerPte;
    PHARDWARE_PDE_ARMV6 PointerPde;
    PHARDWARE_LARGE_PTE_ARMV6 LargePte;
    PFN_NUMBER Pfn;
    
    /* Setup templates */
    TempPte.Accessed = TempPte.Valid = TempLargePte.LargePage = TempLargePte.Accessed = TempPde.Valid = 1;
        
    /* Get the Kernel Table Index */
    KernelPageTableIndex = KernelBase >> PDE_SHIFT;
    printf("Kernel Base: 0x%p (PDE Index: %lx)\n", KernelBase, KernelPageTableIndex);
    
    /* Allocate 1MB PDE_BASE and HYPER_SPACE. This will be improved later. Must be 1MB aligned */
    PageDir = MmAllocateMemoryAtAddress(1 * 1024 * 1024, (PVOID)0x700000, LoaderMemoryData);
    if (!PageDir) { printf("FATAL: No memory!\n"); while (TRUE); }
    printf("Initial Page Directory: 0x%p\n", PageDir);
    
    /* Setup the Low Memory PDE as an identity-mapped Large Page (1MB) */
    LargePte = &PageDir->Pte[LowMemPageTableIndex];
    *LargePte = TempLargePte;
    
    /* Setup the MMIO PDE as two identity mapped large pages -- the kernel will blow these away later */
    LargePte = &PageDir->Pte[MmioPageTableIndex];
    Pfn = PaToLargePfn(0x10000000);
    for (i = 0; i < 2; i++)
    {
        TempLargePte.PageFrameNumber = Pfn++;
        *LargePte++ = TempLargePte;
    }
    
    /* Allocate 8 page tables (8KB) to describe the 8MB initial kernel region */
    KernelPageTable = MmAllocateMemoryWithType(8192, LoaderMemoryData);
    if (!KernelPageTable) { printf("FATAL: No memory!\n"); while (TRUE); }
    printf("Kernel Page Tables: 0x%p\n", KernelPageTable);
    
    /* Setup the Kernel PDEs */
    PointerPde = &PageDir->Pde[KernelPageTableIndex];
    Pfn = PaPtrToPdePfn(KernelPageTable);
    for (i = 0; i < 8; i++)
    {
        TempPde.PageFrameNumber = Pfn++;
        *PointerPde++ = TempPde;
    }

    /* Setup the Startup PDE */
    printf("PAGEDIR: %p IDX: %lx PPDE: %p PFN: %lx \n", PageDir, StartupPdePageTableIndex, &PageDir->Pte[StartupPdePageTableIndex], PaToLargePfn((ULONG_PTR)PageDir));
    LargePte = &PageDir->Pte[StartupPdePageTableIndex];
    TempLargePte.PageFrameNumber = PaToLargePfn((ULONG_PTR)PageDir);
    printf("PAGEDIR: %p IDX: %lx PPDE: %p PFN: %lx \n", PageDir, StartupPdePageTableIndex, LargePte, TempLargePte.PageFrameNumber);
    *LargePte = TempLargePte;
    
    /* After this point, any MiAddressToPde is guaranteed not to fault */
        
    /* Allocate 4 page tables (4KB) to describe the 4MB PTE_BASE region */
    PageTable = MmAllocateMemoryWithType(4096, LoaderMemoryData);
    if (!PageTable) { printf("FATAL: No memory!\n"); while (TRUE); }
    printf("Initial Page Tables: 0x%p\n", PageTable);
    
    /*
     * Link them in the Startup PDE.
     * Note these are the entries in the PD at (MiAddressToPde(PTE_BASE)).
     */
    PointerPde = &PageDir->Pde[StartupPtePageTableIndex];
    Pfn = PaPtrToPdePfn(PageTable);
    for (i = 0; i < 4; i++)
    {
        TempPde.PageFrameNumber = Pfn++;
        *PointerPde++ = TempPde;
    }
    
    /* 
     * Now map these page tables in PTE space (MiAddressToPte(PTE_BASE)).
     * Note that they all live on a single page, since each is 1KB.
     */
    PointerPte = &PageTable->Pte[0x300];
    TempPte.PageFrameNumber = PaPtrToPfn(PageTable);
    *PointerPte = TempPte;
    
    /*
     * After this point, MiAddressToPte((PDE_BASE) to MiAddressToPte(PDE_TOP))
     * is guaranteed not to fault.
     * Any subsequent page allocation will first need its page table created
     * and mapped in the PTE_BASE first, then the page table itself will be
     * editable through its flat PTE address.
     */
    printf("Paging init done\n");
    return PageDir;
}

VOID
ArmSetupPagingAndJump(IN PVOID PageDirectoryBaseAddress)
{
    ARM_CONTROL_REGISTER ControlRegister;
    ARM_TTB_REGISTER TtbRegister;
    ARM_DOMAIN_REGISTER DomainRegister;
    
    /* Set the TTBR */
    TtbRegister.AsUlong = (ULONG_PTR)PageDirectoryBaseAddress;
    ASSERT(TtbRegister.Reserved == 0);
    KeArmTranslationTableRegisterSet(TtbRegister);

    /* Disable domains and simply use access bits on PTEs */
    DomainRegister.AsUlong = 0;
    DomainRegister.Domain0 = ClientDomain;
    KeArmDomainRegisterSet(DomainRegister);

    /* Enable ARMv6+ paging (MMU), caches and the access bit */
    ControlRegister = KeArmControlRegisterGet();
    ControlRegister.MmuEnabled = TRUE;
    ControlRegister.ICacheEnabled = TRUE;
    ControlRegister.DCacheEnabled = TRUE;
    ControlRegister.ForceAp = TRUE;
    ControlRegister.ExtendedPageTables = TRUE;
    KeArmControlRegisterSet(ControlRegister);
	
    /* Jump to Kernel */
    TuiPrintf("Hello from MMU Enabled!\n");
    while (TRUE);
    (*KernelEntryPoint)((PVOID)((ULONG_PTR)ArmLoaderBlock | KSEG0_BASE));
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{   
    ARM_CACHE_REGISTER CacheReg;
    PVOID Base, MemBase;
    PCHAR BootPath, HalPath;
    NTSTATUS Status;
    ULONG Dummy, i;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry, OldEntry;
    PARC_DISK_INFORMATION ArcDiskInformation;
    PARC_DISK_SIGNATURE ArcDiskSignature;
    ULONG ArcDiskCount = 0;
#if 0
    ULONG Checksum = 0;
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
#endif
    PWCHAR ArmModuleName;

    //
    // Allocate the ARM Shared Heap
    //
    ArmSharedHeap = MmAllocateMemoryWithType(PAGE_SIZE, LoaderOsloaderHeap);
    ArmSharedHeapSize = 0;
    if (!ArmSharedHeap) return;
    
    //
    // Allocate the loader block and extension
    //
    ArmLoaderBlock = ArmAllocateFromSharedHeap(sizeof(LOADER_PARAMETER_BLOCK));
    if (!ArmLoaderBlock) return;
    ArmExtension = ArmAllocateFromSharedHeap(sizeof(LOADER_PARAMETER_EXTENSION));
    if (!ArmExtension) return;
    
    //
    // Initialize the loader block
    //
    InitializeListHead(&ArmLoaderBlock->BootDriverListHead);
    InitializeListHead(&ArmLoaderBlock->LoadOrderListHead);
    InitializeListHead(&ArmLoaderBlock->MemoryDescriptorListHead);
    
    //
    // Setup the extension and setup block
    //
    ArmLoaderBlock->Extension = (PVOID)((ULONG_PTR)ArmExtension | KSEG0_BASE);
    ArmLoaderBlock->SetupLdrBlock = NULL;
    
    //
    // Add the Board Memory Map from U-Boot into the STARTUP.COM-style 
    // BIOS descriptor format -- this needs to be removed later.
    //
    ArmBuildBoardMemoryMap();
    
    //
    // Now basically convert these entries to the ARC format, so that we can
    // get a good map of free (usable) memory
    //
    ArmBuildOsMemoryMap();

    //
    // NT uses an extended ARC format, with slightly different memory types.
    // We also want to link the ARC descriptors together into a linked list,
    // instead of the array, and allocate the semi-permanent storage in which
    // these entries will be stored so that the kernel can read them.
    //
    ArmBuildLoaderMemoryList();
    
    //
    // Setup descriptor for the shared heap
    //
    Status = ArmCreateMemoryDescriptor(LoaderOsloaderHeap,
                                       (ULONG_PTR)ArmSharedHeap >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(ArmSharedHeap,
                                                                      ArmSharedHeapSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;

    //
    // Setup descriptor for the boot stack
    //
    Status = ArmCreateMemoryDescriptor(LoaderOsloaderStack,
                                       (ULONG_PTR)&BootStack >> PAGE_SHIFT,
                                       4,
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
#if 0
    //
    // Setup descriptor for the boot page tables
    //
    Status = ArmCreateMemoryDescriptor(LoaderMemoryData,
                                       (ULONG_PTR)&TranslationTableStart >> PAGE_SHIFT,
                                       ((ULONG_PTR)&TranslationTableEnd -
                                        (ULONG_PTR)&TranslationTableStart) / PAGE_SIZE,
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
#endif
    //
    // Setup descriptor for the kernel
    //
    Status = ArmCreateMemoryDescriptor(LoaderSystemCode,
                                       KernelData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(KernelData,
                                                                      KernelSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Setup descriptor for the HAL
    //
    Status = ArmCreateMemoryDescriptor(LoaderHalCode,
                                       HalData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(HalData,
                                                                      HalSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Setup registry data
    //
    ArmLoaderBlock->RegistryBase = (PVOID)((ULONG_PTR)RegistryData | KSEG0_BASE);
    ArmLoaderBlock->RegistryLength = RegistrySize;
    
    //
    // Create an MD for it
    //
    Status = ArmCreateMemoryDescriptor(LoaderRegistryData,
                                       RegistryData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(RegistryData,
                                                                      RegistrySize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // TODO: Setup ARC Hardware tree data
    //
    
    //
    // Setup NLS data
    //
    ArmNlsDataBlock = ArmAllocateFromSharedHeap(sizeof(NLS_DATA_BLOCK));
    ArmLoaderBlock->NlsData = ArmNlsDataBlock;
    ArmLoaderBlock->NlsData->AnsiCodePageData = (PVOID)(AnsiData | KSEG0_BASE);
    ArmLoaderBlock->NlsData->OemCodePageData = (PVOID)(OemData | KSEG0_BASE);
    ArmLoaderBlock->NlsData->UnicodeCodePageData = (PVOID)(UnicodeData | KSEG0_BASE);
    ArmLoaderBlock->NlsData = (PVOID)((ULONG_PTR)ArmLoaderBlock->NlsData | KSEG0_BASE);
    
    //
    // Setup ANSI NLS Memory Descriptor
    //
    Status = ArmCreateMemoryDescriptor(LoaderNlsData,
                                       AnsiData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(AnsiData,
                                                                      AnsiSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Setup OEM NLS Memory Descriptor
    //
    Status = ArmCreateMemoryDescriptor(LoaderNlsData,
                                       OemData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(OemData,
                                                                      OemSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Setup Unicode NLS Memory Descriptor
    //
    Status = ArmCreateMemoryDescriptor(LoaderNlsData,
                                       UnicodeData >> PAGE_SHIFT,
                                       ADDRESS_AND_SIZE_TO_SPAN_PAGES(UnicodeData,
                                                                      UnicodeSize),
                                       0,
                                       &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Setup loader entry for the kernel
    //
    ArmModuleName = ArmAllocateFromSharedHeap(64 * sizeof(WCHAR));
    wcscpy(ArmModuleName, L"ntoskrnl.exe");
    LdrEntry = ArmAllocateFromSharedHeap(sizeof(LDR_DATA_TABLE_ENTRY));
    RtlZeroMemory(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY));
    LdrEntry->DllBase = (PVOID)KernelBase;
    LdrEntry->SizeOfImage = KernelSize;
    LdrEntry->EntryPoint = KernelEntryPoint;
    LdrEntry->LoadCount = 1;
    LdrEntry->Flags = LDRP_IMAGE_DLL | LDRP_ENTRY_PROCESSED;
    RtlInitUnicodeString(&LdrEntry->FullDllName, ArmModuleName);
    RtlInitUnicodeString(&LdrEntry->BaseDllName, ArmModuleName);
    LdrEntry->FullDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->FullDllName.Buffer | KSEG0_BASE);
    LdrEntry->BaseDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->BaseDllName.Buffer | KSEG0_BASE);
    InsertTailList(&ArmLoaderBlock->LoadOrderListHead, &LdrEntry->InLoadOrderLinks);

    //
    // Setup loader entry for the HAL
    //
    ArmModuleName = ArmAllocateFromSharedHeap(64 * sizeof(WCHAR));
    wcscpy(ArmModuleName, L"hal.dll");
    LdrEntry = ArmAllocateFromSharedHeap(sizeof(LDR_DATA_TABLE_ENTRY));
    RtlZeroMemory(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY));
    LdrEntry->DllBase = (PVOID)(HalData | KSEG0_BASE);
    LdrEntry->SizeOfImage = HalSize;
    LdrEntry->EntryPoint = (PVOID)RtlImageNtHeader((PVOID)HalData)->
                                  OptionalHeader.AddressOfEntryPoint;
    LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)LdrEntry->EntryPoint | KSEG0_BASE);
    LdrEntry->LoadCount = 1;
    LdrEntry->Flags = LDRP_IMAGE_DLL | LDRP_ENTRY_PROCESSED;
    RtlInitUnicodeString(&LdrEntry->FullDllName, ArmModuleName);
    RtlInitUnicodeString(&LdrEntry->BaseDllName, ArmModuleName);
    LdrEntry->FullDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->FullDllName.Buffer | KSEG0_BASE);
    LdrEntry->BaseDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->BaseDllName.Buffer | KSEG0_BASE);
    InsertTailList(&ArmLoaderBlock->LoadOrderListHead, &LdrEntry->InLoadOrderLinks);
    
    //
    // Build descriptors for the drivers loaded
    //
    for (i = 0; i < Drivers; i++)
    {
        //
        // Setup loader entry for the driver
        //
        LdrEntry = ArmAllocateFromSharedHeap(sizeof(LDR_DATA_TABLE_ENTRY));
        RtlZeroMemory(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY));
        LdrEntry->DllBase = (PVOID)(DriverData[i] | KSEG0_BASE);
        LdrEntry->SizeOfImage = DriverSize[i];
        LdrEntry->EntryPoint = (PVOID)RtlImageNtHeader((PVOID)DriverData[i])->
                                      OptionalHeader.AddressOfEntryPoint;
        LdrEntry->EntryPoint = (PVOID)((ULONG_PTR)LdrEntry->EntryPoint | KSEG0_BASE);
        LdrEntry->LoadCount = 1;
        LdrEntry->Flags = LDRP_IMAGE_DLL | LDRP_ENTRY_PROCESSED;
        ArmModuleName = ArmAllocateFromSharedHeap(64 * sizeof(WCHAR));
        RtlZeroMemory(ArmModuleName, 64 * sizeof(WCHAR));
        LdrEntry->FullDllName.Length = strlen(DriverName[i]) * sizeof(WCHAR);
        LdrEntry->FullDllName.MaximumLength = LdrEntry->FullDllName.Length;
        LdrEntry->FullDllName.Buffer = ArmModuleName;
        LdrEntry->BaseDllName = LdrEntry->FullDllName;
        while (*DriverName[i]) *ArmModuleName++ = *DriverName[i]++;
        LdrEntry->FullDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->FullDllName.Buffer | KSEG0_BASE);
        LdrEntry->BaseDllName.Buffer = (PVOID)((ULONG_PTR)LdrEntry->BaseDllName.Buffer | KSEG0_BASE);
        InsertTailList(&ArmLoaderBlock->LoadOrderListHead, &LdrEntry->InLoadOrderLinks);

        //
        // Build a descriptor for the driver
        //
        Status = ArmCreateMemoryDescriptor(LoaderBootDriver,
                                           DriverData[i] >> PAGE_SHIFT,
                                           ADDRESS_AND_SIZE_TO_SPAN_PAGES(DriverData[i],
                                                                          DriverSize[i]),
                                           0,
                                           &Dummy);
        if (Status != STATUS_SUCCESS) return;
    }
    
    //
    // Loop driver list
    //    
    NextEntry = ArmLoaderBlock->LoadOrderListHead.Flink;
    while (NextEntry != &ArmLoaderBlock->LoadOrderListHead)
    {
        //
        // Remember the physical entry
        //
        OldEntry = NextEntry->Flink;
        
        //
        // Edit the data
        //
        NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
        NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
        
        //
        // Keep looping
        //
        NextEntry = OldEntry;
    }

    //
    // Now edit the root itself
    //
    NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
    NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
    
    //
    // Setup extension parameters
    //
    ArmExtension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    ArmExtension->MajorVersion = 5;
    ArmExtension->MinorVersion = 2;
    
    //
    // Make a copy of the command line
    //
    ArmLoaderBlock->LoadOptions = ArmCommandLine;
    strcpy(ArmCommandLine, reactos_kernel_cmdline);
    
    //
    // Find the first \, separating the ARC path from NT path
    //
    BootPath = strchr(ArmCommandLine, '\\');
    *BootPath = ANSI_NULL;
    
    //
    // Set the ARC Boot Path
    //
    strncpy(ArmArcBootPath, ArmCommandLine, 63);
    ArmLoaderBlock->ArcBootDeviceName = (PVOID)((ULONG_PTR)ArmArcBootPath | KSEG0_BASE);
    
    //
    // The rest of the string is the NT path
    //
    HalPath = strchr(BootPath + 1, ' ');
    *HalPath = ANSI_NULL;
    ArmNtBootPath[0] = '\\';
    strncat(ArmNtBootPath, BootPath + 1, 63);
    strcat(ArmNtBootPath,"\\");
    ArmLoaderBlock->NtBootPathName = (PVOID)((ULONG_PTR)ArmNtBootPath | KSEG0_BASE);
    
    //
    // Set the HAL paths
    //
    strncpy(ArmArcHalPath, ArmArcBootPath, 63);
    ArmLoaderBlock->ArcHalDeviceName = (PVOID)((ULONG_PTR)ArmArcHalPath | KSEG0_BASE);
    strcpy(ArmNtHalPath, "\\");
    ArmLoaderBlock->NtHalPathName = (PVOID)((ULONG_PTR)ArmNtHalPath | KSEG0_BASE);
    
    //
    // Use this new command line
    //
    strncpy(ArmLoaderBlock->LoadOptions, HalPath + 2, 255);
    
    //
    // Parse it and change every slash to a space
    //
    BootPath = ArmLoaderBlock->LoadOptions;
    do {if (*BootPath == '/') *BootPath = ' ';} while (*BootPath++);

    //
    // Fixup command-line pointer
    //
    ArmLoaderBlock->LoadOptions = (PVOID)((ULONG_PTR)ArmLoaderBlock->LoadOptions | KSEG0_BASE);

    //
    // Setup cache information
    //
    CacheReg = KeArmCacheRegisterGet();   
    ArmLoaderBlock->u.Arm.FirstLevelDcacheSize = SizeBits[CacheReg.DSize];
    ArmLoaderBlock->u.Arm.FirstLevelDcacheFillSize = LenBits[CacheReg.DLength];
    ArmLoaderBlock->u.Arm.FirstLevelDcacheFillSize <<= 2;
    ArmLoaderBlock->u.Arm.FirstLevelIcacheSize = SizeBits[CacheReg.ISize];
    ArmLoaderBlock->u.Arm.FirstLevelIcacheFillSize = LenBits[CacheReg.ILength];
    ArmLoaderBlock->u.Arm.FirstLevelIcacheFillSize <<= 2;
    ArmLoaderBlock->u.Arm.SecondLevelDcacheSize =
    ArmLoaderBlock->u.Arm.SecondLevelDcacheFillSize =
    ArmLoaderBlock->u.Arm.SecondLevelIcacheSize =
    ArmLoaderBlock->u.Arm.SecondLevelIcacheFillSize = 0;
    
    //
    // Allocate the Interrupt stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupDpcStack);
    ArmLoaderBlock->u.Arm.InterruptStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->u.Arm.InterruptStack += KERNEL_STACK_SIZE;
    
    //
    // Build an entry for it
    //
    Status = ArmCreateMemoryDescriptor(LoaderStartupDpcStack,
                                         (ULONG_PTR)Base >> PAGE_SHIFT,
                                         KERNEL_STACK_SIZE / PAGE_SIZE,
                                         0,
                                         &Dummy);
    if (Status != STATUS_SUCCESS) return;
        
    //
    // Allocate the Kernel Boot stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupKernelStack);
    ArmLoaderBlock->KernelStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->KernelStack += KERNEL_STACK_SIZE;
    
    //
    // Build an entry for it
    //
    Status = ArmCreateMemoryDescriptor(LoaderStartupKernelStack,
                                         (ULONG_PTR)Base >> PAGE_SHIFT,
                                         KERNEL_STACK_SIZE / PAGE_SIZE,
                                         0,
                                         &Dummy);
    if (Status != STATUS_SUCCESS) return;

    //
    // Allocate the Abort stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupPanicStack);
    ArmLoaderBlock->u.Arm.PanicStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->u.Arm.PanicStack += KERNEL_STACK_SIZE;
    
    //
    // Build an entry for it
    //
    Status = ArmCreateMemoryDescriptor(LoaderStartupPanicStack,
                                         (ULONG_PTR)Base >> PAGE_SHIFT,
                                         KERNEL_STACK_SIZE / PAGE_SIZE,
                                         0,
                                         &Dummy);
    if (Status != STATUS_SUCCESS) return;

    //
    // Allocate the PCR/KUSER_SHARED page -- align it to 1MB (we only need 2x4KB)
    //
    Base = MmAllocateMemoryWithType(2 * 1024 * 1024, LoaderStartupPcrPage);
    MemBase = Base;
    Base = (PVOID)ROUND_UP(Base, 1 * 1024 * 1024);
    ArmLoaderBlock->u.Arm.PcrPage = (ULONG)Base >> PDE_SHIFT;
    
    //
    // Build an entry for the KPCR and KUSER_SHARED_DATA
    //
    Status = ArmCreateMemoryDescriptor(LoaderStartupPcrPage,
                                         (ULONG_PTR)MemBase >> PAGE_SHIFT,
                                         (2 * 1024 * 1024) / PAGE_SIZE,
                                         0,
                                         &Dummy);
    if (Status != STATUS_SUCCESS) return;
    
    //
    // Allocate PDR pages -- align them to 1MB (we only need 3x4KB)
    //
    Base = MmAllocateMemoryWithType(4 * 1024 * 1024, LoaderStartupPdrPage);
    MemBase = Base;
    Base = (PVOID)ROUND_UP(Base, 1 * 1024 * 1024);
    ArmLoaderBlock->u.Arm.PdrPage = (ULONG)Base >> PDE_SHIFT;

    //
    // Build an entry for the PDR, PRCB and initial KPROCESS/KTHREAD
    //
    Status = ArmCreateMemoryDescriptor(LoaderStartupPdrPage,
                                         (ULONG_PTR)MemBase >> PAGE_SHIFT,
                                         (4 * 1024 * 1024) / PAGE_SIZE,
                                         0,
                                         &Dummy);
    if (Status != STATUS_SUCCESS) return;

    //
    // Set initial PRCB, Thread and Process on the last PDR page
    //
    Base = (PVOID)((ULONG)Base + 2 * 1024 * 1024);
    ArmLoaderBlock->Prcb = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->Process = ArmLoaderBlock->Prcb + sizeof(KPRCB);
    ArmLoaderBlock->Thread = ArmLoaderBlock->Process + sizeof(EPROCESS);
    
    //
    // Check if we're booting from RAM disk
    //
    if ((gRamDiskBase) && (gRamDiskSize))
    {
        //
        // Allocate a descriptor to describe it
        //
        Status = ArmCreateMemoryDescriptor(LoaderXIPRom,
                                           (ULONG_PTR)gRamDiskBase >> PAGE_SHIFT,
                                           gRamDiskSize / PAGE_SIZE,
                                           0,
                                           &Dummy);
        if (Status != STATUS_SUCCESS) return;
    }
    
    //
    // Loop memory list
    //    
    NextEntry = ArmLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &ArmLoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Remember the physical entry
        //
        OldEntry = NextEntry->Flink;
        
        //
        // Edit the data
        //
        NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
        NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
        
        //
        // Keep looping
        //
        NextEntry = OldEntry;
    }
    
    //
    // Now edit the root itself
    //
    NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
    NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
    
    //
    // Allocate ARC disk structure
    //
    ArcDiskInformation = ArmAllocateFromSharedHeap(sizeof(ARC_DISK_INFORMATION));
    InitializeListHead(&ArcDiskInformation->DiskSignatureListHead);
    ArmLoaderBlock->ArcDiskInformation = (PVOID)((ULONG_PTR)ArcDiskInformation | KSEG0_BASE);
    
#if 0
    //
    // Read the MBR
    //
    MachDiskReadLogicalSectors(0x49, 0ULL, 1, (PVOID)DISKREADBUFFER);
    Buffer = (ULONG*)DISKREADBUFFER;
    Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;
        
    //
    // Calculate the MBR checksum
    //
    for (i = 0; i < 128; i++) Checksum += Buffer[i];
    Checksum = ~Checksum + 1;
        
#endif
    //
    // Allocate a disk signature and fill it out
    //
    ArcDiskSignature = ArmAllocateFromSharedHeap(sizeof(ARC_DISK_SIGNATURE));
    ArcDiskSignature->Signature = 0xBADAB00B;// Mbr->Signature;
    ArcDiskSignature->CheckSum = 0xFAB4BEEF; //Checksum;
    
    //
    // Allocare a string for the name and fill it out
    //
    ArcDiskSignature->ArcName = ArmAllocateFromSharedHeap(256);
    sprintf(ArcDiskSignature->ArcName, "multi(0)disk(0)rdisk(%lu)", ArcDiskCount++);
    ArcDiskSignature->ArcName = (PVOID)((ULONG_PTR)ArcDiskSignature->ArcName | KSEG0_BASE);
        
    //
    // Insert the descriptor into the list
    //
    InsertTailList(&ArcDiskInformation->DiskSignatureListHead,
                   &ArcDiskSignature->ListEntry);

    //
    // Loop ARC disk list
    //    
    NextEntry = ArcDiskInformation->DiskSignatureListHead.Flink;
    while (NextEntry != &ArcDiskInformation->DiskSignatureListHead)
    {
        //
        // Remember the physical entry
        //
        OldEntry = NextEntry->Flink;
        
        //
        // Edit the data
        //
        NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
        NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
        
        //
        // Keep looping
        //
        NextEntry = OldEntry;
    }
    
    //
    // Now edit the root itself
    //
    NextEntry->Flink = (PVOID)((ULONG_PTR)NextEntry->Flink | KSEG0_BASE);
    NextEntry->Blink = (PVOID)((ULONG_PTR)NextEntry->Blink | KSEG0_BASE);
}

VOID
FrLdrStartup(IN ULONG Magic)
{
    PVOID PageDir;
    //
    // Disable interrupts (already done)
    //

    //
    // Set proper CPSR (already done)
    //

    //
    // Initialize the page directory
    //
    PageDir = ArmSetupPageDirectory();

    //
    // Initialize paging and load NTOSKRNL
    //
    ArmSetupPagingAndJump(PageDir);
}
