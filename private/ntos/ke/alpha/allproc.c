/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and initializes kernel resources required
    to start a new processor, and passes a complete processor state
    structure to the HAL to obtain a new processor.

Author:

    David N. Cutler 29-Apr-1993
    Joe Notarangelo 30-Nov-1993

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

#define ROUND_UP(x) ((sizeof(x) + 64) & (~64))
#define BLOCK1_SIZE ((3 * KERNEL_STACK_SIZE) + PAGE_SIZE)
#define BLOCK2_SIZE (ROUND_UP(KPRCB) + ROUND_UP(ETHREAD) + 64)

//
// Macros to compute whether an address is physically addressable.
//

#if defined(_AXP64_)

#define IS_KSEG_ADDRESS(v)                                      \
    (((v) >= KSEG43_BASE) &&                                    \
     ((v) < KSEG43_LIMIT) &&                                    \
     (KSEG_PFN(v) < ((KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT)))

#define KSEG_PFN(v) ((ULONG)(((v) - KSEG43_BASE) >> PAGE_SHIFT))
#define KSEG0_ADDRESS(v) (KSEG0_BASE | ((v) - KSEG43_BASE))

#else

#define IS_KSEG_ADDRESS(v) (((v) >= KSEG0_BASE) && ((v) < KSEG2_BASE))
#define KSEG_PFN(v) ((ULONG)(((v) - KSEG0_BASE) >> PAGE_SHIFT))
#define KSEG0_ADDRESS(v) (v)

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

    ULONG_PTR MemoryBlock1;
    ULONG_PTR MemoryBlock2;
    ULONG Number;
    ULONG PcrPage;
    PKPRCB Prcb;
    KPROCESSOR_STATE ProcessorState;
    struct _RESTART_BLOCK *RestartBlock;
    BOOLEAN Started;
    LOGICAL SpecialPoolState;

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
    // Initialize the processor state that will be used to start each of
    // processors. Each processor starts in the system initialization code
    // with address of the loader parameter block as an argument.
    //

    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.IntA0 = (ULONGLONG)(LONG_PTR)KeLoaderBlock;
    ProcessorState.ContextFrame.Fir = (ULONGLONG)(LONG_PTR)KiStartProcessor;
    Number = 1;
    while (Number < KeRegisteredProcessors) {

        //
        // Allocate a DPC stack, an idle thread kernel stack, a panic
        // stack, a PCR page, a processor block, and an executive thread
        // object. If the allocation fails or the allocation cannot be
        // made from unmapped nonpaged pool, then stop starting processors.
        //
        // Disable any special pooling that the user may have set in the
        // registry as the next couple of allocations must come from KSEG0.
        //

        SpecialPoolState = MmSetSpecialPool(FALSE);
        MemoryBlock1 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK1_SIZE);
        if (IS_KSEG_ADDRESS(MemoryBlock1) == FALSE) {
            MmSetSpecialPool(SpecialPoolState);
            if ((PVOID)MemoryBlock1 != NULL) {
                ExFreePool((PVOID)MemoryBlock1);
            }

            break;
        }

        MemoryBlock2 = (ULONG_PTR)ExAllocatePool(NonPagedPool, BLOCK2_SIZE);
        if (IS_KSEG_ADDRESS(MemoryBlock2) == FALSE) {
            MmSetSpecialPool(SpecialPoolState);
            ExFreePool((PVOID)MemoryBlock1);
            if ((PVOID)MemoryBlock2 != NULL) {
                ExFreePool((PVOID)MemoryBlock2);
            }

            break;
        }

        MmSetSpecialPool(SpecialPoolState);

        //
        // Zero both blocks of allocated memory.
        //

        RtlZeroMemory((PVOID)MemoryBlock1, BLOCK1_SIZE);
        RtlZeroMemory((PVOID)MemoryBlock2, BLOCK2_SIZE);

        //
        // Set address of interrupt stack in loader parameter block.
        //

        KeLoaderBlock->u.Alpha.PanicStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (1 * KERNEL_STACK_SIZE));

        //
        // Set address of idle thread kernel stack in loader parameter block.
        //

        KeLoaderBlock->KernelStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (2 * KERNEL_STACK_SIZE));

        ProcessorState.ContextFrame.IntSp =
                            (ULONGLONG)(LONG_PTR)KeLoaderBlock->KernelStack;

        //
        // Set address of panic stack in loader parameter block.
        //

        KeLoaderBlock->u.Alpha.DpcStack =
                        KSEG0_ADDRESS(MemoryBlock1 + (3 * KERNEL_STACK_SIZE));

        //
        // Set the page frame of the PCR page in the loader parameter block.
        //

        PcrPage = KSEG_PFN(MemoryBlock1 + (3 * KERNEL_STACK_SIZE));
        KeLoaderBlock->u.Alpha.PcrPage = PcrPage;

        //
        // Set the address of the processor block and executive thread in the
        // loader parameter block.
        //

        KeLoaderBlock->Prcb = KSEG0_ADDRESS((MemoryBlock2  + 63) & ~63);
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
                KiMb();
            }
        }

        Number += 1;
    }

#endif

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KiAdjustInterruptTime(0);
    return;
}
