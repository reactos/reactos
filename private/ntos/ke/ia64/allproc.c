/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and intializes kernel resources required
    to start a new processor, and passes a complete processor state
    structure to the HAL to obtain a new processor.

Author:

    Bernard Lint 31-Jul-96

Environment:

    Kernel mode only.

Revision History:

    Based on MIPS original (David N. Cutler 29-Apr-1993)

--*/


#include "ki.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, KeStartAllProcessors)

#endif

//
// Define macro to round up to 64-byte boundary and define block sizes.
//

#define ROUND_UP(x) ((sizeof(x) + 63) & (~63))
#define BLOCK1_SIZE (2 * (KERNEL_BSTORE_SIZE + KERNEL_STACK_SIZE) + PAGE_SIZE)
#define BLOCK2_SIZE (ROUND_UP(KPRCB) + ROUND_UP(ETHREAD) + 64)

#if !defined(NT_UP)

//
// Define barrier wait static data.
//

ULONG KiBarrierWait = 0;

#endif

extern ULONG_PTR KiUserSharedDataPage;
extern ULONG_PTR KiKernelPcrPage;

//
// Define forward referenced prototypes.
//

VOID
KiCalibratePerformanceCounter(
    VOID
    );

VOID
KiCalibratePerformanceCounterTarget (
    IN PULONG SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiOSRendezvous (
    VOID
    );

VOID
KeStartAllProcessors(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialize on the master boot
    processor to start all of the other registered processors.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG_PTR MemoryBlock1;
    ULONG_PTR MemoryBlock2;
    ULONG_PTR MemoryBlock3;
    ULONG_PTR PcrAddress;
    ULONG Number;
    PHYSICAL_ADDRESS PcrPage;
    PKPRCB Prcb;
    BOOLEAN Started;
    KPROCESSOR_STATE ProcessorState;

#if !defined(NT_UP)

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

    Number = 1;
    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.StIIP = ((PPLABEL_DESCRIPTOR)KiOSRendezvous)->EntryPoint;
    while (Number < KeRegisteredProcessors) {

        // **** TBD check this for MP case

        //
        // Allocate a DPC stack, an idle thread kernel stack, a panic
        // stack, a PCR page, a processor block, and an executive thread
        // object. If the allocation fails or the allocation cannot be
        // made from unmapped nonpaged pool, then stop starting processors.
        //

        MemoryBlock1 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK1_SIZE);
        if ((PVOID)MemoryBlock1 == NULL) {
            break;
        }

        MemoryBlock2 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK2_SIZE);
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
        // Set address of idle thread kernel stack in loader parameter block.
        //

        KeLoaderBlock->KernelStack = MemoryBlock1 + KERNEL_STACK_SIZE;

        //
        // Set address of panic stack in loader parameter block.
        //

        KeLoaderBlock->u.Ia64.PanicStack = MemoryBlock1 + KERNEL_BSTORE_SIZE +
                                               (2 * KERNEL_STACK_SIZE);

        //
        // Set the address of the processor block and executive thread in the
        // loader parameter block.
        //

        KeLoaderBlock->Prcb = MemoryBlock2;
        KeLoaderBlock->Thread = KeLoaderBlock->Prcb + ROUND_UP(KPRCB);
        ((PKPRCB)KeLoaderBlock->Prcb)->Number = (UCHAR)Number;

        //
        // Set the page frame of the PCR page in the loader parameter block.
        //

        PcrAddress = MemoryBlock1 + KERNEL_BSTORE_SIZE + (2*KERNEL_STACK_SIZE);
        PcrPage = MmGetPhysicalAddress((PVOID)PcrAddress);
        KeLoaderBlock->u.Ia64.PcrPage = PcrPage.QuadPart >> PAGE_SHIFT;
        KeLoaderBlock->u.Ia64.PcrPage2 = KiUserSharedDataPage;
        KiKernelPcrPage = KeLoaderBlock->u.Ia64.PcrPage;
        
        //
        // Attempt to start the next processor. If attempt is successful,
        // then wait for the processor to get initialized. Otherwise,
        // deallocate the processor resources and terminate the loop.
        //

        Started = HalStartNextProcessor(KeLoaderBlock, &ProcessorState);

        if (Started) {

            //
            //  Wait for processor to initialize in kernel, 
            //  then loop for another
            //
    
            while (*((volatile ULONG_PTR *) &KeLoaderBlock->Prcb) != 0) {
                KeYieldProcessor();
            }

        } else {

            ExFreePool((PVOID)MemoryBlock1);
            ExFreePool((PVOID)MemoryBlock2);
            break;

        }

        Number += 1;
    }

    //
    // Allow all processor that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;

#endif

    //
    // Reset and synchronize the performance counters of all processors.
    //

    KiAdjustInterruptTime (0);
    return;
}
