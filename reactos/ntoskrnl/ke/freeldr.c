/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/freeldr.c
 * PURPOSE:         FreeLDR Bootstrap Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _BIOS_MEMORY_DESCRIPTOR
{
    ULONG BlockBase;
    ULONG BlockSize;
} BIOS_MEMORY_DESCRIPTOR, *PBIOS_MEMORY_DESCRIPTOR;

/* GLOBALS *******************************************************************/

/* FreeLDR Memory Data */
ULONG_PTR MmFreeLdrFirstKrnlPhysAddr, MmFreeLdrLastKrnlPhysAddr;
ULONG_PTR MmFreeLdrLastKernelAddress;
ULONG MmFreeLdrMemHigher;
ULONG MmFreeLdrPageDirectoryEnd;

/* FreeLDR Loader Data */
PROS_LOADER_PARAMETER_BLOCK KeRosLoaderBlock;
BOOLEAN AcpiTableDetected;
ADDRESS_RANGE KeMemoryMap[64];
ULONG KeMemoryMapRangeCount;

/* NT Loader Module/Descriptor Count */
ULONG BldrCurrentMd;
ULONG BldrCurrentMod;

/* NT Loader Data. Eats up about 80KB! */
LOADER_PARAMETER_BLOCK BldrLoaderBlock;                 // 0x0000
LOADER_PARAMETER_EXTENSION BldrExtensionBlock;          // 0x0060
CHAR BldrCommandLine[256];                              // 0x00DC
CHAR BldrArcBootPath[64];                               // 0x01DC
CHAR BldrArcHalPath[64];                                // 0x021C
CHAR BldrNtHalPath[64];                                 // 0x025C
CHAR BldrNtBootPath[64];                                // 0x029C
LDR_DATA_TABLE_ENTRY BldrModules[64];                   // 0x02DC
MEMORY_ALLOCATION_DESCRIPTOR BldrMemoryDescriptors[60]; // 0x14DC
WCHAR BldrModuleStrings[64][260];                       // 0x19DC
WCHAR BldrModuleStringsFull[64][260];                   // 0x9BDC
NLS_DATA_BLOCK BldrNlsDataBlock;                        // 0x11DDC
SETUP_LOADER_BLOCK BldrSetupBlock;                      // 0x11DE8
ARC_DISK_INFORMATION BldrArcDiskInfo;                   // 0x12134
CHAR BldrArcNames[32][256];                             // 0x1213C
ARC_DISK_SIGNATURE BldrDiskInfo[32];                    // 0x1413C
                                                        // 0x1443C

/* BIOS Memory Map */
BIOS_MEMORY_DESCRIPTOR BiosMemoryDescriptors[16] = {{0}};
PBIOS_MEMORY_DESCRIPTOR BiosMemoryDescriptorList = BiosMemoryDescriptors;

/* ARC Memory Map */
ULONG NumberDescriptors = 0;
MEMORY_DESCRIPTOR MDArray[60] = {{0}};

/* FUNCTIONS *****************************************************************/

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
KiRosGetMdFromArray(VOID)
{
    /* Return the next MD from the list, but make sure we don't overflow */
    if (BldrCurrentMd > 60) KEBUGCHECK(0);
    return &BldrMemoryDescriptors[BldrCurrentMd++];
}

VOID
NTAPI
KiRosAddBiosBlock(ULONG Address,
                  ULONG Size)
{
    PBIOS_MEMORY_DESCRIPTOR BiosBlock = BiosMemoryDescriptorList;

    /* Loop our BIOS Memory Descriptor List */
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
KiRosBuildBiosMemoryMap(VOID)
{
    ULONG j;
    ULONG BlockBegin, BlockEnd;

    /* Loop the BIOS Memory Map */
    for (j = 0; j < KeMemoryMapRangeCount; j++)
    {
        /* Get the start and end addresses */
        BlockBegin = KeMemoryMap[j].BaseAddrLow;
        BlockEnd = KeMemoryMap[j].BaseAddrLow + KeMemoryMap[j].LengthLow - 1;

        /* Make sure this isn't a > 4GB descriptor */
        if (!KeMemoryMap[j].BaseAddrHigh)
        {
            /* Make sure we don't overflow */
            if (BlockEnd < BlockBegin) BlockEnd = 0xFFFFFFFF;

            /* Check if this is free memory */
            if (KeMemoryMap[j].Type == 1)
            {
                /* Add it to our BIOS descriptors */
                KiRosAddBiosBlock(BlockBegin, BlockEnd - BlockBegin + 1);
            }
        }
    }
}

NTSTATUS
NTAPI
KiRosAllocateArcDescriptor(IN ULONG PageBegin,
                           IN ULONG PageEnd,
                           IN MEMORY_TYPE MemoryType)
{
    ULONG i;

    /* Loop all our descriptors */
    for (i = 0; i < NumberDescriptors; i++)
    {
        /* Attempt to fing a free block that describes our region */
        if ((MDArray[i].MemoryType == MemoryFree) &&
            (MDArray[i].BasePage <= PageBegin) &&
            (MDArray[i].BasePage + MDArray[i].PageCount > PageBegin) &&
            (MDArray[i].BasePage + MDArray[i].PageCount >= PageEnd))
        {
            /* Found one! */
            break;
        }
    }

    /* Check if we found no free blocks, and fail if so */
    if (i == NumberDescriptors) return ENOMEM;

    /* Check if the block has our base address */
    if (MDArray[i].BasePage == PageBegin)
    {
        /* Check if it also has our ending address */
        if ((MDArray[i].BasePage + MDArray[i].PageCount) == PageEnd)
        {
            /* Then convert this region into our new memory type */
            MDArray[i].MemoryType = MemoryType;
        }
        else
        {
            /* Otherwise, make sure we have enough descriptors */
            if (NumberDescriptors == 60) return ENOMEM;

            /* Cut this descriptor short */
            MDArray[i].BasePage = PageEnd;
            MDArray[i].PageCount -= (PageEnd - PageBegin);

            /* And allocate a new descriptor for our memory range */
            MDArray[NumberDescriptors].BasePage = PageBegin;
            MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
            MDArray[NumberDescriptors].MemoryType = MemoryType;
            NumberDescriptors++;
        }
    }
    else if ((MDArray[i].BasePage + MDArray[i].PageCount) == PageEnd)
    {
        /* This block has our end address, make sure we have a free block */
        if (NumberDescriptors == 60) return ENOMEM;

        /* Rebase this descriptor */
        MDArray[i].PageCount = PageBegin - MDArray[i].BasePage;

        /* And allocate a new descriptor for our memory range */
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        NumberDescriptors++;
    }
    else
    {
        /* We'll need two descriptors, make sure they're available */
        if ((NumberDescriptors + 1) >= 60) return ENOMEM;

        /* Allocate a free memory descriptor for what follows us */
        MDArray[NumberDescriptors].BasePage = PageEnd;
        MDArray[NumberDescriptors].PageCount = MDArray[i].PageCount -
                                               (PageEnd - MDArray[i].BasePage);
        MDArray[NumberDescriptors].MemoryType = MemoryFree;
        NumberDescriptors++;

        /* Cut down the current free descriptor */
        MDArray[i].PageCount = PageBegin - MDArray[i].BasePage;
        
        /* Allocate a new memory descriptor for our memory range */
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        NumberDescriptors++;
    }

    /* Everything went well */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KiRosConfigureArcDescriptor(IN ULONG PageBegin,
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
KiRosBuildOsMemoryMap(VOID)
{
    PBIOS_MEMORY_DESCRIPTOR MdBlock;
    ULONG BlockStart, BlockEnd, BiasedStart, BiasedEnd, PageStart, PageEnd;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BiosPage = 0xA0;

    /* Loop the BIOS Memory Descriptor List */
    MdBlock = BiosMemoryDescriptorList;
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

        /* If we're starting at page 0, then put the BIOS page at the end */
        if (!PageStart) BiosPage = PageEnd;

        /* Check if we did any alignment */
        if (BiasedStart)
        {
            /* Mark that region as reserved */
            Status = KiRosConfigureArcDescriptor(PageStart - 1,
                                                 PageStart,
                                                 MemorySpecialMemory);
            if (Status != STATUS_SUCCESS) break;
        }

        /* Check if we did any alignment */
        if (BiasedEnd)
        {
            /* Mark that region as reserved */
            Status = KiRosConfigureArcDescriptor(PageEnd - 1,
                                                 PageEnd,
                                                 MemorySpecialMemory);
            if (Status != STATUS_SUCCESS) break;

            /* If the bios page was the last page, use the next one instead */
            if (BiosPage == PageEnd) BiosPage += 1;
        }

        /* Check if the page is below the 16MB Memory hole */
        if (PageEnd <= 0xFC0)
        {
            /* It is, mark the memory a free */
            Status = KiRosConfigureArcDescriptor(PageStart,
                                                 PageEnd,
                                                 LoaderFree);
        }
        else if (PageStart >= 0x1000)
        {
            /* It's over 16MB, so that memory gets marked as reserve */
            Status = KiRosConfigureArcDescriptor(PageStart,
                                                 PageEnd,
                                                 LoaderFree);
        }
        else
        {
            /* Check if it starts below the memory hole */
            if (PageStart < 0xFC0)
            {
                /* Mark that part as free */
                Status = KiRosConfigureArcDescriptor(PageStart,
                                                     0xFC0,
                                                     MemoryFree);
                if (Status != STATUS_SUCCESS) break;

                /* And update the page start for the code below */
                PageStart = 0xFC0;
            }

            /* Any code in the memory hole region ends up as reserve */
            Status = KiRosConfigureArcDescriptor(PageStart,
                                                 PageEnd,
                                                 LoaderFree);
        }

        /* If we failed, break out, otherwise, go to the next BIOS block */
        if (Status != STATUS_SUCCESS) break;
        MdBlock++;
    }

    /* If anything failed until now, return error code */
    if (Status != STATUS_SUCCESS) return Status;

    /* Set the top 16MB region as reserved */
    Status = KiRosConfigureArcDescriptor(0xFC0, 0x1000, MemorySpecialMemory);
    if (Status != STATUS_SUCCESS) return Status;

    /* Setup the BIOS region as reserved */
    KiRosConfigureArcDescriptor(0xA0, 0x100, LoaderMaximum);
    KiRosConfigureArcDescriptor(BiosPage, 0x100, MemoryFirmwarePermanent);

    /* Build an entry for the IVT */
    Status = KiRosAllocateArcDescriptor(0, 1, MemoryFirmwarePermanent);
    if (Status != STATUS_SUCCESS) return Status;

    /* Build an entry for the KPCR and KUSER_SHARED_DATA */
    Status = KiRosAllocateArcDescriptor(1, 3, LoaderMemoryData);
    if (Status != STATUS_SUCCESS) return Status;

    /* Build an entry for the PDE and return the status */
    Status = KiRosAllocateArcDescriptor(KeRosLoaderBlock->
                                        PageDirectoryStart >> PAGE_SHIFT,
                                        KeRosLoaderBlock->
                                        PageDirectoryEnd >> PAGE_SHIFT,
                                        LoaderMemoryData);
    return Status;
}

VOID
NTAPI
KiRosBuildReservedMemoryMap(VOID)
{
    ULONG j;
    ULONG BlockBegin, BlockEnd, BiasedPage;

    /* Loop the BIOS Memory Map */
    for (j = 0; j < KeMemoryMapRangeCount; j++)
    {
        /* Get the start and end addresses */
        BlockBegin = KeMemoryMap[j].BaseAddrLow;
        BlockEnd = BlockBegin + KeMemoryMap[j].LengthLow - 1;

        /* Make sure it wasn't a > 4GB descriptor */
        if (!KeMemoryMap[j].BaseAddrHigh)
        {
            /* Make sure it doesn't overflow */
            if (BlockEnd < BlockBegin) BlockEnd = 0xFFFFFFFF;

            /* Check if this was free memory */
            if (KeMemoryMap[j].Type == 1)
            {
                /* Get the page-aligned addresses */
                BiasedPage = BlockBegin & (PAGE_SIZE - 1);
                BlockBegin >>= PAGE_SHIFT;
                if (BiasedPage) BlockBegin++;
                BlockEnd = (BlockEnd >> PAGE_SHIFT) + 1;

                /* Check if the block is within the 16MB memory hole */
                if ((BlockBegin < 0xFC0) && (BlockEnd >= 0xFC0))
                {
                    /* Don't allow it to cross this boundary */
                    BlockBegin = 0xFC0;
                }

                /* Check if the boundary is across 16MB */
                if ((BlockEnd > 0xFFF) && (BlockBegin <= 0xFFF))
                {
                    /* Don't let it cross */
                    BlockEnd = 0xFFF;
                }

                /* Check if the block describes the memory hole */
                if ((BlockBegin >= 0xFC0) && (BlockEnd <= 0xFFF))
                {
                    /* Set this region as temporary */
                    KiRosConfigureArcDescriptor(BlockBegin,
                                                BlockEnd,
                                                MemoryFirmwareTemporary);
                }
            }
            else
            {
                /* Get the page-aligned addresses */
                BlockBegin >>= PAGE_SHIFT;
                BiasedPage = (BlockEnd + 1) & (PAGE_SIZE - 1);
                BlockEnd >>= PAGE_SHIFT;
                if (BiasedPage) BlockEnd++;

                /* Set this memory as reserved */
                KiRosConfigureArcDescriptor(BlockBegin,
                                            BlockEnd + 1,
                                            MemorySpecialMemory);
            }
        }
    }
}

VOID
NTAPI
KiRosInsertNtDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
    PLIST_ENTRY ListHead, PreviousEntry, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor = NULL, NextDescriptor = NULL;

    /* Loop the memory descriptor list */
    ListHead = &KeLoaderBlock->MemoryDescriptorListHead;
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
KiRosBuildNtDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
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
        Descriptor = KiRosGetMdFromArray();
        if (!Descriptor) return STATUS_INSUFFICIENT_RESOURCES;

        /* Check if we are using another descriptor */
        if (UseNext)
        {
            /* Allocate that one too */
            NextDescriptor = KiRosGetMdFromArray();
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
            KiRosInsertNtDescriptor(NextDescriptor);
        }

        /* Insert the descriptor we allocated */
        KiRosInsertNtDescriptor(Descriptor);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

PMEMORY_ALLOCATION_DESCRIPTOR
NTAPI
KiRosFindNtDescriptor(IN ULONG BasePage)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock = NULL;
    PLIST_ENTRY NextEntry, ListHead;

    /* Scan the memory descriptor list */
    ListHead = &KeLoaderBlock->MemoryDescriptorListHead;
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
KiRosAllocateNtDescriptor(IN TYPE_OF_MEMORY MemoryType,
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
        MdBlock = KiRosFindNtDescriptor(BasePage);
        if (MdBlock)
        {
            /* If it contains our limit as well, break out early */
            if ((MdBlock->PageCount + MdBlock->BasePage) > AlignedLimit) break;
        }

        /* Loop the memory list */
        ActiveMdBlock = NULL;
        ListHead = &KeLoaderBlock->MemoryDescriptorListHead;
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
            return KiRosBuildNtDescriptor(ActiveMdBlock,
                                          MemoryType,
                                          ActiveAlignedBase,
                                          PageCount);
        }
    } while (TRUE);

    /* We found a matching block, generate a descriptor with it */
    *ReturnedBase = BasePage;
    return KiRosBuildNtDescriptor(MdBlock, MemoryType, BasePage, PageCount);
}

NTSTATUS
NTAPI
KiRosBuildArcMemoryList(VOID)
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
        Descriptor = KiRosGetMdFromArray();
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
        if (Descriptor->PageCount) KiRosInsertNtDescriptor(Descriptor);
    }

    /* All went well */
    return STATUS_SUCCESS;
}

VOID
NTAPI
KiRosFrldrLpbToNtLpb(IN PROS_LOADER_PARAMETER_BLOCK RosLoaderBlock,
                     IN PLOADER_PARAMETER_BLOCK *NtLoaderBlock)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLOADER_MODULE RosEntry;
    ULONG i, j, ModSize;
    PVOID ModStart;
    PCHAR DriverName;
    PCHAR BootPath, HalPath;
    CHAR CommandLine[256];
    PARC_DISK_SIGNATURE RosDiskInfo, ArcDiskInfo;
    PIMAGE_NT_HEADERS NtHeader;
    WCHAR PathToDrivers[] = L"\\SystemRoot\\System32\\drivers\\";
    WCHAR PathToSystem32[] = L"\\SystemRoot\\System32\\";
    CHAR DriverNameLow[256];
    ULONG Base;

    /* First get some kernel-loader globals */
    AcpiTableDetected = (RosLoaderBlock->Flags & MB_FLAGS_ACPI_TABLE) ? TRUE : FALSE;
    MmFreeLdrMemHigher = RosLoaderBlock->MemHigher;
    MmFreeLdrPageDirectoryEnd = RosLoaderBlock->PageDirectoryEnd;
    if (!MmFreeLdrPageDirectoryEnd) MmFreeLdrPageDirectoryEnd = 0x40000;

    /* Set the NT Loader block and initialize it */
    *NtLoaderBlock = KeLoaderBlock = LoaderBlock = &BldrLoaderBlock;
    RtlZeroMemory(LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

    /* Set the NLS Data block */
    LoaderBlock->NlsData = &BldrNlsDataBlock;

    /* Set the ARC Data block */
    LoaderBlock->ArcDiskInformation = &BldrArcDiskInfo;

    /* Assume this is from FreeLDR's SetupLdr */
    LoaderBlock->SetupLdrBlock = &BldrSetupBlock;

    /* Setup the list heads */
    InitializeListHead(&LoaderBlock->LoadOrderListHead);
    InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
    InitializeListHead(&LoaderBlock->BootDriverListHead);
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    /* Build the free memory map, which uses BIOS Descriptors */
    KiRosBuildBiosMemoryMap();

    /* Build entries for ReactOS memory ranges, which uses ARC Descriptors */
    KiRosBuildOsMemoryMap();

    /* Build entries for the reserved map, which uses ARC Descriptors */
    KiRosBuildReservedMemoryMap();

    /* Now convert the BIOS and ARC Descriptors into NT Memory Descirptors */
    KiRosBuildArcMemoryList();

    /* Loop boot driver list */
    for (i = 0; i < RosLoaderBlock->ModsCount; i++)
    {
        /* Get the ROS loader entry */
        RosEntry = &RosLoaderBlock->ModsAddr[i];
        DriverName = (PCHAR)RosEntry->String;
        ModStart = (PVOID)RosEntry->ModStart;
        ModSize = RosEntry->ModEnd - (ULONG_PTR)ModStart;

        /* Check if this is any of the NLS files */
        if (!_stricmp(DriverName, "ansi.nls"))
        {
            /* ANSI Code page */
            ModStart = RVA(ModStart, KSEG0_BASE);
            LoaderBlock->NlsData->AnsiCodePageData = ModStart;

            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderNlsData,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
            continue;
        }
        else if (!_stricmp(DriverName, "oem.nls"))
        {
            /* OEM Code page */
            ModStart = RVA(ModStart, KSEG0_BASE);
            LoaderBlock->NlsData->OemCodePageData = ModStart;

            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderNlsData,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
            continue;
        }
        else if (!_stricmp(DriverName, "casemap.nls"))
        {
            /* Unicode Code page */
            ModStart = RVA(ModStart, KSEG0_BASE);
            LoaderBlock->NlsData->UnicodeCodePageData = ModStart;

            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderNlsData,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
            continue;
        }

        /* Check if this is the SYSTEM hive */
        if (!(_stricmp(DriverName, "system")) ||
            !(_stricmp(DriverName, "system.hiv")))
        {
            /* Save registry data */
            ModStart = RVA(ModStart, KSEG0_BASE);
            LoaderBlock->RegistryBase = ModStart;
            LoaderBlock->RegistryLength = ModSize;

            /* Disable setup mode */
            LoaderBlock->SetupLdrBlock = NULL;

            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderRegistryData,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
            continue;
        }

        /* Check if this is the HARDWARE hive */
        if (!(_stricmp(DriverName, "hardware")) ||
            !(_stricmp(DriverName, "hardware.hiv")))
        {
            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderRegistryData,
                                      (ULONG_PTR)ModStart >> PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
            continue;
        }

        /* Check if this is the kernel */
        if (!(_stricmp(DriverName, "ntoskrnl.exe")))
        {
            /* Create an MD for it */
            KiRosAllocateNtDescriptor(LoaderSystemCode,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
        }
        else if (!(_stricmp(DriverName, "hal.dll")))
        {
            /* Create an MD for the HAL */
            KiRosAllocateNtDescriptor(LoaderHalCode,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
        }
        else
        {
            /* Create an MD for any driver */
            KiRosAllocateNtDescriptor(LoaderBootDriver,
                                      ((ULONG_PTR)ModStart &~ KSEG0_BASE) >>
                                      PAGE_SHIFT,
                                      (ModSize + PAGE_SIZE - 1)>> PAGE_SHIFT,
                                      0,
                                      &Base);
        }

        /* Lowercase the drivername so we can check its extension later */
        strcpy(DriverNameLow, DriverName);
        _strlwr(DriverNameLow);

        /* Setup the loader entry */
        LdrEntry = &BldrModules[i];
        RtlZeroMemory(LdrEntry, sizeof(LDR_DATA_TABLE_ENTRY));

        /* Convert driver name from ANSI to Unicode */
        for (j = 0; j < strlen(DriverName); j++)
        {
            BldrModuleStrings[i][j] = DriverName[j];
        }

        /* Setup driver name */
        RtlInitUnicodeString(&LdrEntry->BaseDllName, BldrModuleStrings[i]);

        /* Construct a correct full name */
        BldrModuleStringsFull[i][0] = 0;
        LdrEntry->FullDllName.MaximumLength = 260 * sizeof(WCHAR);
        LdrEntry->FullDllName.Length = 0;
        LdrEntry->FullDllName.Buffer = BldrModuleStringsFull[i];

        /* Guess the path */
        if (strstr(DriverNameLow, ".dll") || strstr(DriverNameLow, ".exe"))
        {
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&TempString, PathToSystem32);
            RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &TempString);
        }
        else /* .sys */
        {
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&TempString, PathToDrivers);
            RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &TempString);
        }

        /* Append base name of the driver */
        RtlAppendUnicodeStringToString(&LdrEntry->FullDllName, &LdrEntry->BaseDllName);

        /* Copy data from Freeldr Module Entry */
        LdrEntry->DllBase = ModStart;
        LdrEntry->SizeOfImage = ModSize;

        /* Copy additional data */
        NtHeader = RtlImageNtHeader(ModStart);
        LdrEntry->EntryPoint = RVA(ModStart,
                                   NtHeader->
                                   OptionalHeader.AddressOfEntryPoint);

        /* Initialize other data */
        LdrEntry->LoadCount = 1;
        LdrEntry->Flags = LDRP_IMAGE_DLL |
                          LDRP_ENTRY_PROCESSED;
        if (RosEntry->Reserved) LdrEntry->Flags |= LDRP_ENTRY_INSERTED;

        /* Insert it into the loader block */
        InsertTailList(&LoaderBlock->LoadOrderListHead,
                       &LdrEntry->InLoadOrderLinks);
    }

    /* Setup command line */
    LoaderBlock->LoadOptions = BldrCommandLine;
    strcpy(BldrCommandLine, RosLoaderBlock->CommandLine);

    /* Setup the extension block */
    LoaderBlock->Extension = &BldrExtensionBlock;
    LoaderBlock->Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    LoaderBlock->Extension->MajorVersion = 5;
    LoaderBlock->Extension->MinorVersion = 2;

    /* FreeLDR hackllocates 1536 static pages for the initial boot images */
    LoaderBlock->Extension->LoaderPagesSpanned = 1536 * PAGE_SIZE;

    /* ReactOS always boots the kernel at 0x80800000 (just like NT 5.2) */
    LoaderBlock->Extension->LoaderPagesSpanned += 0x80800000 - KSEG0_BASE;

    /* Now convert to pages */
    LoaderBlock->Extension->LoaderPagesSpanned /= PAGE_SIZE;

    /* Now setup the setup block if we have one */
    if (LoaderBlock->SetupLdrBlock)
    {
        /* All we'll setup right now is the flag for text-mode setup */
        LoaderBlock->SetupLdrBlock->Flags = 1;
    }

    /* Make a copy of the command line */
    strcpy(CommandLine, LoaderBlock->LoadOptions);

    /* Find the first \, separating the ARC path from NT path */
    BootPath = strchr(CommandLine, '\\');
    *BootPath = ANSI_NULL;
    strncpy(BldrArcBootPath, CommandLine, 63);
    LoaderBlock->ArcBootDeviceName = BldrArcBootPath;

    /* The rest of the string is the NT path */
    HalPath = strchr(BootPath + 1, ' ');
    *HalPath = ANSI_NULL;
    BldrNtBootPath[0] = '\\';
    strncat(BldrNtBootPath, BootPath + 1, 63);
    strcat(BldrNtBootPath,"\\");
    LoaderBlock->NtBootPathName = BldrNtBootPath;

    /* Set the HAL paths */
    strncpy(BldrArcHalPath, BldrArcBootPath, 63);
    LoaderBlock->ArcHalDeviceName = BldrArcHalPath;
    strcpy(BldrNtHalPath, "\\");
    LoaderBlock->NtHalPathName = BldrNtHalPath;

    /* Use this new command line */
    strncpy(LoaderBlock->LoadOptions, HalPath + 2, 255);

    /* Parse it and change every slash to a space */
    BootPath = LoaderBlock->LoadOptions;
    do {if (*BootPath == '/') *BootPath = ' ';} while (*BootPath++);

    /* Now let's loop ARC disk information */
    for (i = 0; i < RosLoaderBlock->DrivesCount; i++)
    {
        /* Get the ROS loader entry */
        RosDiskInfo = &RosLoaderBlock->DrivesAddr[i];

        /* Get the ARC structure */
        ArcDiskInfo = &BldrDiskInfo[i];

        /* Copy the data over */
        ArcDiskInfo->Signature = RosDiskInfo->Signature;
        ArcDiskInfo->CheckSum = RosDiskInfo->CheckSum;

        /* Copy the ARC Name */
        strcpy(BldrArcNames[i], RosDiskInfo->ArcName);
        ArcDiskInfo->ArcName = BldrArcNames[i];

        /* Insert into the list */
        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                       &ArcDiskInfo->ListEntry);
    }
}

VOID
FASTCALL
KiRosPrepareForSystemStartup(IN ULONG Dummy,
                             IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_BLOCK NtLoaderBlock;
    ULONG size, i;
#if defined(_M_IX86)
    PKTSS Tss;
    PKGDTENTRY TssEntry;

    /* Load the GDT and IDT */
    Ke386SetGlobalDescriptorTable(*(PKDESCRIPTOR)&KiGdtDescriptor.Limit);
    Ke386SetInterruptDescriptorTable(*(PKDESCRIPTOR)&KiIdtDescriptor.Limit);

    /* Initialize the boot TSS */
    Tss = &KiBootTss;
    TssEntry = &KiBootGdt[KGDT_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);
#endif

    /* Save pointer to ROS Block */
    KeRosLoaderBlock = LoaderBlock;
    MmFreeLdrLastKernelAddress = PAGE_ROUND_UP(KeRosLoaderBlock->
                                               ModsAddr[KeRosLoaderBlock->
                                                        ModsCount - 1].
                                               ModEnd);
    MmFreeLdrFirstKrnlPhysAddr = KeRosLoaderBlock->ModsAddr[0].ModStart -
                                 KSEG0_BASE;
    MmFreeLdrLastKrnlPhysAddr = MmFreeLdrLastKernelAddress - KSEG0_BASE;

    /* Save memory manager data */
    KeMemoryMapRangeCount = 0;
    if (LoaderBlock->Flags & MB_FLAGS_MMAP_INFO)
    {
        /* We have a memory map from the nice BIOS */
        size = *((PULONG)(LoaderBlock->MmapAddr - sizeof(ULONG)));
        i = 0;

        /* Map it until we run out of size */
        while (i < LoaderBlock->MmapLength)
        {
            /* Copy into the Kernel Memory Map */
            memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
                    (PVOID)(LoaderBlock->MmapAddr + i),
                    sizeof(ADDRESS_RANGE));

            /* Increase Memory Map Count */
            KeMemoryMapRangeCount++;

            /* Increase Size */
            i += size;
        }

        /* Save data */
        LoaderBlock->MmapLength = KeMemoryMapRangeCount * sizeof(ADDRESS_RANGE);
        LoaderBlock->MmapAddr = (ULONG)KeMemoryMap;
    }
    else
    {
        /* Nothing from BIOS */
        LoaderBlock->MmapLength = 0;
        LoaderBlock->MmapAddr = (ULONG)KeMemoryMap;
    }

#if defined(_M_IX86)
    /* Set up the VDM Data */
    NtEarlyInitVdm();
#endif

    /* Convert the loader block */
    KiRosFrldrLpbToNtLpb(KeRosLoaderBlock, &NtLoaderBlock);

    /* Do general System Startup */
    KiSystemStartup(NtLoaderBlock);
}

/* EOF */
