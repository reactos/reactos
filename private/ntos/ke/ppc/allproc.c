/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1994  Motorola, IBM Corp.

Module Name:

    allproc.c

Abstract:

    This module allocates and intializes kernel resources required
    to start a new processor, and passes a complete processor state
    structure to the HAL to obtain a new processor.

Author:

    David N. Cutler 29-Apr-1993
    Joe Notarangelo 30-Nov-1993
    Pat Carr        16-Aug-1994

Environment:

    Kernel mode only.

Revision History:

--*/


#include "ki.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, KeStartAllProcessors)

#endif

//
// Define macro to round up to 64-byte boundary and define block sizes.
//

#define ROUND_UP(x) ((sizeof(x) + 63) & (~63))
#define BLOCK1_SIZE (3 * KERNEL_STACK_SIZE)
#define BLOCK2_SIZE (ROUND_UP(KPRCB) + ROUND_UP(ETHREAD) + 64)

//
// Define barrier wait static data.
//

#if !defined(NT_UP)

ULONG KiBarrierWait = 0;

#endif

#if !defined(NT_UP)
MEMORY_ALLOCATION_DESCRIPTOR KiFreePcrPagesDescriptor;
#endif

//
// Define forward referenced prototypes.
//

VOID
KiStartProcessor (
    IN PLOADER_PARAMETER_BLOCK Loaderblock
    );


VOID
KeStartAllProcessors(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialization on the master boot
    processor to start all of the other registered processors.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ULONG MemoryBlock1;
    ULONG MemoryBlock2;
    ULONG Number;
    ULONG PcrAddress;
    ULONG PcrPage;
    PKPRCB Prcb;
    KPROCESSOR_STATE ProcessorState;
    volatile PRESTART_BLOCK RestartBlock;
    BOOLEAN Started;
    PHYSICAL_ADDRESS PcrPhysicalAddress;
    PMEMORY_ALLOCATION_DESCRIPTOR KiPcrPagesDescriptor = KeLoaderBlock->u.Ppc.PcrPagesDescriptor;

    //
    // If the registered number of processors is greater than the maximum
    // number of processors supported, then only allow the maximum number
    // of supported processors.
    //

    if (KeRegisteredProcessors > MAXIMUM_PROCESSORS) {
        KeRegisteredProcessors = MAXIMUM_PROCESSORS;
    }

    //
    // Set barrier that will prevent any other processor from entering the
    // idle loop until all processors have been started.
    //

    KiBarrierWait = 1;

    //
    // Initialize the processor state that will be used to start each of
    // processors. Each processor starts in the system initialization code
    // with address of the loader parameter block as an argument.
    //

    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.Gpr3 = (ULONG)KeLoaderBlock;
    ProcessorState.ContextFrame.Iar  = *(PULONG)KiStartProcessor;

    Number = 0;

    while ((Number+1) < KeRegisteredProcessors) {

        //
        // Allocate a DPC stack, an idle thread kernel stack, a panic
        // stack, a PCR page, a processor block, and an executive thread
        // object. If the allocation fails or the allocation cannot be
        // made from nonpaged pool, then stop starting processors.
        //

        if (Number >= KiPcrPagesDescriptor->PageCount) {
            break;
        }

        MemoryBlock1 = (ULONG)ExAllocatePool(NonPagedPool, BLOCK1_SIZE);
        if ((PVOID)MemoryBlock1 == NULL) {
            break;
        }

        MemoryBlock2 = (ULONG)ExAllocatePool(NonPagedPool, BLOCK2_SIZE);
        if ((PVOID)MemoryBlock2 == NULL) {
            ExFreePool((PVOID)MemoryBlock1);
            break;
        }

        //
        // Zero both blocks of allocated memory.
        //

        RtlZeroMemory((PVOID)MemoryBlock1, BLOCK1_SIZE);
        RtlZeroMemory((PVOID)MemoryBlock2, BLOCK2_SIZE);

        //
        // Set address of interrupt stack in loader parameter block.
        //

        KeLoaderBlock->u.Ppc.InterruptStack = MemoryBlock1 + (1 * KERNEL_STACK_SIZE);

        //
        // Set address of idle thread kernel stack in loader parameter block.
        //

        KeLoaderBlock->KernelStack = MemoryBlock1 + (2 * KERNEL_STACK_SIZE);

        ProcessorState.ContextFrame.Gpr1 = (ULONG)KeLoaderBlock->KernelStack;

        //
        // Set address of panic stack in loader parameter block.
        //

        KeLoaderBlock->u.Ppc.PanicStack = MemoryBlock1 + (3 * KERNEL_STACK_SIZE);

        //
        // Set the page frame of the PCR page in the loader parameter block.
        //

        PcrPage = KiPcrPagesDescriptor->BasePage + Number;
        PcrAddress = KSEG0_BASE | (PcrPage << PAGE_SHIFT);
        RtlZeroMemory((PVOID)PcrAddress, PAGE_SIZE);
        ProcessorState.ContextFrame.Gpr4 = PcrAddress;
        KeLoaderBlock->u.Ppc.PcrPage = PcrPage;

        //
        // Copy the physical address of the PCR2 page from the current
        // processor's PCR into the loader parameter block for the new
        // processor.
        //
        // Note that in the PCR this is an address rather than a page
        // number.
        //

        KeLoaderBlock->u.Ppc.PcrPage2 = PCR->PcrPage2 >> PAGE_SHIFT;

        //
        // Set the address of the processor block and executive thread in the
        // loader parameter block.
        //

        KeLoaderBlock->Prcb = (MemoryBlock2  + 63) & ~63;
        KeLoaderBlock->Thread = KeLoaderBlock->Prcb + ROUND_UP(KPRCB);

        //
        // Attempt to start the next processor. If attempt is successful,
        // then wait for the processor to get initialized. Otherwise,
        // deallocate the processor resources and terminate the loop.
        //

        Started = HalStartNextProcessor(KeLoaderBlock, &ProcessorState);

        if (Started == FALSE) {

            ExFreePool((PVOID)MemoryBlock1);
            ExFreePool((PVOID)MemoryBlock2);
            break;

        } else {

            //
            // Wait until boot is finished on the target processor before
            // starting the next processor. Booting is considered to be
            // finished when a processor completes its initialization and
            // drops into the idle loop.
            //

            Prcb = (PKPRCB)(KeLoaderBlock->Prcb);
            RestartBlock = Prcb->RestartBlock;
            while (RestartBlock->BootStatus.BootFinished == 0) {
            }
        }

        Number += 1;

    }

    //
    // Allow all processor that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;

    if ( Number < KiPcrPagesDescriptor->PageCount ) {
        if ( Number == 0 ) {
            KiPcrPagesDescriptor->MemoryType = LoaderOsloaderHeap;
        } else {
            KiFreePcrPagesDescriptor.BasePage = KiPcrPagesDescriptor->BasePage + Number;
            KiFreePcrPagesDescriptor.PageCount = KiPcrPagesDescriptor->PageCount - Number;
            KiFreePcrPagesDescriptor.MemoryType = LoaderOsloaderHeap;
            InsertTailList(&KeLoaderBlock->MemoryDescriptorListHead,
                           &KiFreePcrPagesDescriptor.ListEntry);
        }
    }

#endif

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KiAdjustInterruptTime (0);
    return;
}
