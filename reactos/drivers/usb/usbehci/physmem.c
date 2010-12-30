/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/physmem.c
 * PURPOSE:     Common Buffer routines.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "physmem.h"
#include "debug.h"

#define SMALL_ALLOCATION_SIZE 32

VOID
DumpPages()
{
    //PMEM_HEADER MemBlock = (PMEM_HEADER)EhciSharedMemory.VirtualAddr;
}

// Returns Virtual Address of Allocated Memory
ULONG
AllocateMemory(PEHCI_HOST_CONTROLLER hcd, ULONG Size, ULONG *PhysicalAddress)
{
    PMEM_HEADER MemoryPage = (PMEM_HEADER)hcd->CommonBufferVA;
    ULONG PageCount = 0;
    ULONG NumberOfPages = hcd->CommonBufferSize / PAGE_SIZE;
    ULONG BlocksNeeded;
    ULONG i,j, freeCount;
    ULONG RetAddr = 0;

    Size = ((Size + SMALL_ALLOCATION_SIZE - 1) / SMALL_ALLOCATION_SIZE) * SMALL_ALLOCATION_SIZE;
    BlocksNeeded = Size / SMALL_ALLOCATION_SIZE;

    do
    {
        if (MemoryPage->IsFull)
        {
            PageCount++;
            MemoryPage = (PMEM_HEADER)((ULONG)MemoryPage + PAGE_SIZE);
            continue;
        }
        freeCount = 0;
        for (i = 0; i < sizeof(MemoryPage->Entry); i++)
        {
            if (!MemoryPage->Entry[i].InUse)
            {
                freeCount++;
            }
            else
            {
                freeCount = 0;
            }

            if (freeCount == BlocksNeeded)
            {
                for (j = 0; j < freeCount; j++)
                {
                    MemoryPage->Entry[i-j].InUse = 1;
                    MemoryPage->Entry[i-j].Blocks = 0;
                }

                MemoryPage->Entry[i-freeCount + 1].Blocks = BlocksNeeded;

                RetAddr = (ULONG)MemoryPage + (SMALL_ALLOCATION_SIZE * (i - freeCount + 1)) + sizeof(MEM_HEADER);

                *PhysicalAddress = (ULONG)hcd->CommonBufferPA.LowPart + (RetAddr - (ULONG)hcd->CommonBufferVA);
                return RetAddr;
            }
        }

        PageCount++;
        MemoryPage = (PMEM_HEADER)((ULONG)MemoryPage + PAGE_SIZE);
    } while (PageCount < NumberOfPages);

    return 0;
}

VOID
ReleaseMemory(ULONG Address)
{
    PMEM_HEADER MemoryPage;
    ULONG Index, i;

    MemoryPage = (PMEM_HEADER)(Address & ~(PAGE_SIZE - 1));

    Index = (Address - ((ULONG)MemoryPage + sizeof(MEM_HEADER))) / SMALL_ALLOCATION_SIZE;

    for (i = 0; i < MemoryPage->Entry[Index].Blocks; i++)
    {
        MemoryPage->Entry[Index + i].InUse = 0;
        MemoryPage->Entry[Index + i].Blocks = 0;
    }
}
