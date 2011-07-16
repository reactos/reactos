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
    PMEM_HEADER MemoryPage = NULL;
    ULONG PageCount = 0;
    ULONG NumberOfPages = hcd->CommonBufferSize / PAGE_SIZE;
    ULONG BlocksNeeded = 0;
    ULONG i,j, freeCount;
    ULONG RetAddr = 0;

    MemoryPage = (PMEM_HEADER)hcd->CommonBufferVA[0];
    Size = ((Size + SMALL_ALLOCATION_SIZE - 1) / SMALL_ALLOCATION_SIZE) * SMALL_ALLOCATION_SIZE;
    BlocksNeeded = Size / SMALL_ALLOCATION_SIZE;

    do
    {
        if (MemoryPage->IsFull)
        {
            PageCount++;
            
            if (!(PMEM_HEADER)hcd->CommonBufferVA[PageCount])
            {
                hcd->CommonBufferVA[PageCount] =
                    hcd->pDmaAdapter->DmaOperations->AllocateCommonBuffer(hcd->pDmaAdapter,
                                                                          PAGE_SIZE,
                                                                          &hcd->CommonBufferPA[PageCount],
                                                                          FALSE);
            }
            MemoryPage = (PMEM_HEADER)hcd->CommonBufferVA[PageCount];
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

            if ((i-freeCount+1 + BlocksNeeded) > sizeof(MemoryPage->Entry))
            {
                freeCount = 0;
                break;
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

                *(ULONG*)PhysicalAddress = (ULONG)hcd->CommonBufferPA[PageCount].LowPart + (RetAddr - (ULONG)hcd->CommonBufferVA[PageCount]);

                return RetAddr;
            }
        }

        PageCount++;
        if (!(PMEM_HEADER)hcd->CommonBufferVA[PageCount])
        {
            
            hcd->CommonBufferVA[PageCount] =
                hcd->pDmaAdapter->DmaOperations->AllocateCommonBuffer(hcd->pDmaAdapter,
                                                                      PAGE_SIZE,
                                                                      &hcd->CommonBufferPA[PageCount],
                                                                      FALSE);
            DPRINT1("Allocated CommonBuffer VA %x, PA %x\n", hcd->CommonBufferVA[PageCount], hcd->CommonBufferPA[PageCount]);
        }
        MemoryPage = (PMEM_HEADER)hcd->CommonBufferVA[PageCount];

    } while (PageCount < NumberOfPages);

    if (PageCount == NumberOfPages)
        ASSERT(FALSE);

    return 0;
}

VOID
ReleaseMemory(PEHCI_HOST_CONTROLLER hcd, ULONG Address)
{
    PMEM_HEADER MemoryPage;
    ULONG Index, i, BlockSize;

    MemoryPage = (PMEM_HEADER)(Address & ~(PAGE_SIZE - 1));

    Index = (Address - ((ULONG)MemoryPage + sizeof(MEM_HEADER))) / SMALL_ALLOCATION_SIZE;
    BlockSize = MemoryPage->Entry[Index].Blocks;

    for (i = 0; i < BlockSize; i++)
    {
        MemoryPage->Entry[Index + i].InUse = 0;
        MemoryPage->Entry[Index + i].Blocks = 0;
    }

    if (MemoryPage != (PMEM_HEADER)hcd->CommonBufferVA[0])
    {
        for (i=0; i < sizeof(MemoryPage->Entry) / 2; i++)
        {
            if ((MemoryPage->Entry[i].InUse) || (MemoryPage->Entry[sizeof(MemoryPage->Entry) - i].InUse))
                return;
        }
        DPRINT1("Freeing CommonBuffer VA %x, PA %x\n", MemoryPage, MmGetPhysicalAddress(MemoryPage));
        hcd->pDmaAdapter->DmaOperations->FreeCommonBuffer(hcd->pDmaAdapter,
                                                          PAGE_SIZE,
                                                          MmGetPhysicalAddress(MemoryPage),
                                                          MemoryPage,
                                                          FALSE);
    }
}
