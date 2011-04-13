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

VOID
NTAPI
DmaMemAllocator_Destroy(
    IN LPDMA_MEMORY_ALLOCATOR Allocator)
{
    /* is there a bitmap buffer */
    if (Allocator->BitmapBuffer)
    {
        /* free bitmap buffer */
        ExFreePool(Allocator->BitmapBuffer);
    }

    /* free struct */
    ExFreePool(Allocator);
}

NTSTATUS
NTAPI
DmaMemAllocator_Create(
    IN LPDMA_MEMORY_ALLOCATOR *OutMemoryAllocator)
{
    LPDMA_MEMORY_ALLOCATOR Allocator;

    /* sanity check */
    ASSERT(OutMemoryAllocator);

    /* allocate struct - must be non paged as it contains a spin lock */
    Allocator = ExAllocatePool(NonPagedPool, sizeof(DMA_MEMORY_ALLOCATOR));
    if (!Allocator)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero struct */
    RtlZeroMemory(Allocator, sizeof(DMA_MEMORY_ALLOCATOR));

    /* store result */
    *OutMemoryAllocator = Allocator;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DmaMemAllocator_Initialize(
    IN OUT LPDMA_MEMORY_ALLOCATOR Allocator,
    IN ULONG DefaultBlockSize,
    IN PKSPIN_LOCK Lock,
    IN PHYSICAL_ADDRESS PhysicalBase,
    IN PVOID VirtualBase,
    IN ULONG Length)
{
    PULONG BitmapBuffer;
    ULONG BitmapLength;

    /* sanity checks */
    ASSERT(Length >= PAGE_SIZE);
    ASSERT(Length % PAGE_SIZE == 0);
    ASSERT(DefaultBlockSize == 32 || DefaultBlockSize == 64 || DefaultBlockSize == 128);

    /* calculate bitmap length */
    BitmapLength = (Length / DefaultBlockSize) / sizeof(ULONG);

    /* allocate bitmap buffer from nonpaged pool */
    BitmapBuffer = ExAllocatePool(NonPagedPool, BitmapLength);
    if (!BitmapBuffer)
    {
        /* out of memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize bitmap */
    RtlInitializeBitMap(&Allocator->Bitmap, BitmapBuffer, BitmapLength);
    RtlClearAllBits(&Allocator->Bitmap);

    /* initialize rest of allocator */
    Allocator->PhysicalBase = PhysicalBase;
    Allocator->VirtualBase = VirtualBase;
    Allocator->Length = Length;
    Allocator->BitmapBuffer = BitmapBuffer;
    Allocator->Lock = Lock;
    Allocator->BlockSize = DefaultBlockSize;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DmaMemAllocator_Allocate(
    IN LPDMA_MEMORY_ALLOCATOR Allocator,
    IN ULONG Size,
    OUT PVOID *OutVirtualAddress,
    OUT PPHYSICAL_ADDRESS OutPhysicalAddress)
{
    ULONG Length, BlockCount, FreeIndex, StartPage, EndPage;
    KIRQL OldLevel;

    /* sanity check */
    ASSERT(Size < PAGE_SIZE);
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    /* align request size to block size */
    Length = (Size + Allocator->BlockSize -1) & ~(Allocator->BlockSize -1);

    /* sanity check */
    ASSERT(Length);

    /* convert to block count */
    BlockCount = Length / Allocator->BlockSize;

    /* acquire lock */
    KeAcquireSpinLock(Allocator->Lock, &OldLevel);


    /* start search */
    FreeIndex = 0;
    do
    {

        /* search for an free index */
        FreeIndex = RtlFindClearBits(&Allocator->Bitmap, BlockCount, FreeIndex);

        /* check if there were bits found */
        if (FreeIndex == MAXULONG)
           break;

        /* check that the allocation does not spawn over page boundaries */
        StartPage = (FreeIndex * Allocator->BlockSize);
        StartPage = (StartPage != 0 ? StartPage / PAGE_SIZE : 0);
        EndPage = ((FreeIndex + BlockCount) * Allocator->BlockSize) / PAGE_SIZE;


        if (StartPage == EndPage)
        {
            /* reserve bits */
            RtlSetBits(&Allocator->Bitmap, FreeIndex, BlockCount);

            /* done */
            break;
        }
        else
        {
            /* request spaws a page boundary */
            FreeIndex++;
        }
    }
    while(TRUE);

    /* release bitmap lock */
    KeReleaseSpinLock(Allocator->Lock, OldLevel);

    /* check if allocation failed */
    if (FreeIndex == MAXULONG)
    {
        /* allocation failed */
        return STATUS_UNSUCCESSFUL;
    }

    /* return result */
    *OutVirtualAddress = (PVOID)((ULONG_PTR)Allocator->VirtualBase + FreeIndex * Allocator->BlockSize);
    OutPhysicalAddress->QuadPart = Allocator->PhysicalBase.QuadPart + FreeIndex * Allocator->BlockSize;

    /* done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
DmaMemAllocator_Free(
    IN LPDMA_MEMORY_ALLOCATOR Allocator,
    IN OPTIONAL PVOID VirtualAddress,
    IN ULONG Size)
{
    KIRQL OldLevel;
    ULONG BlockOffset = 0, BlockLength;

    /* sanity check */
    ASSERT(VirtualAddress);

    /* calculate block length */
    BlockLength = ((ULONG_PTR)VirtualAddress - (ULONG_PTR)Allocator->VirtualBase);

    /* is block offset zero */
    if (BlockLength)
    {
        /* divide by base block size */
        BlockOffset = BlockLength / Allocator->BlockSize;
    }

    /* align size to base block */
    Size = (Size + Allocator->BlockSize - 1) & ~(Allocator->BlockSize - 1);

    /* acquire bitmap lock */
    KeAcquireSpinLock(Allocator->Lock, &OldLevel);

    /* clear bits */
    RtlClearBits(&Allocator->Bitmap, BlockOffset, Size);

    /* release bitmap lock */
    KeReleaseSpinLock(Allocator->Lock, OldLevel);
}
