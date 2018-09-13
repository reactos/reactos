/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initkr.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, and the processor control
    block.

Author:

    David N. Cutler (davec) 11-Apr-1990

Environment:

    Kernel mode only.

Revision History:

    Joe Notarangelo  21-April-1992
        very minor changes for ALPHA
             - system time to 64bit integer
             - some data moved out of pcr
--*/

#include "ki.h"

//
// External data.
//

extern KSPIN_LOCK CcMasterSpinLock;
extern KSPIN_LOCK CcVacbSpinLock;
extern KSPIN_LOCK MmPfnLock;
extern KSPIN_LOCK MmSystemSpaceLock;

//
// Define forward referenced prototypes.
//

ULONG
KiGetFeatureBits (
    VOID
    );

__inline
ULONG
amask(
    IN ULONG Feature
    )

{
    return(__asm("amask %0, v0",Feature));
}
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, KiInitializeKernel)
#endif

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function gains control after the system has been bootstrapped and
    before the system has been initialized. Its function is to initialize
    the kernel data structures, initialize the idle thread and process objects,
    initialize the processor control block, call the executive initialization
    routine, and then return to the system startup routine. This routine is
    also called to initialize the processor specific structures when a new
    processor is brought on line.

Arguments:

    Process - Supplies a pointer to a control object of type process for
        the specified processor.

    Thread - Supplies a pointer to a dispatcher object of type thread for
        the specified processor.

    IdleStack - Supplies a pointer the base of the real kernel stack for
        idle thread on the specified processor.

    Prcb - Supplies a pointer to a processor control block for the specified
        processor.

    Number - Supplies the number of the processor that is being
        initialized.

    LoaderBlock - Supplies a pointer to the loader parameter block.

Return Value:

    None.

--*/

{

    UCHAR DataByte;
    ULONG DataLong;
    LONG Index;
    KIRQL OldIrql;
    PKPCR Pcr = PCR;
    struct _RESTART_BLOCK *RestartBlock;

    //
    // Save the address of the loader parameter block.
    //

    KeLoaderBlock = LoaderBlock;

    //
    // Set the appropriate member in the active processors set.
    //

    SetMember(Number, KeActiveProcessors);

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if ((Number + 1) > KeNumberProcessors) {
        KeNumberProcessors = Number + 1;
    }

    //
    // Set the maximum address space number to the minimum of all maximum
    // address space numbers passed via the loader block.
    //

    if (Number == 0) {
        KiMaximumAsn = LoaderBlock->u.Alpha.MaximumAddressSpaceNumber;

    } else if (KiMaximumAsn > LoaderBlock->u.Alpha.MaximumAddressSpaceNumber) {
        KiMaximumAsn = LoaderBlock->u.Alpha.MaximumAddressSpaceNumber;
    }

    //
    // Initialize the passive release, APC, and DPC interrupt vectors.
    //

    Pcr->InterruptRoutine[0] = KiPassiveRelease;
    Pcr->InterruptRoutine[APC_LEVEL] = KiApcInterrupt;
    Pcr->InterruptRoutine[DISPATCH_LEVEL] = KiDispatchInterrupt;
    Pcr->ReservedVectors =
        (1 << PASSIVE_LEVEL) | (1 << APC_LEVEL) | (1 << DISPATCH_LEVEL);

    //
    // Initialize the processor id fields in the PCR.
    //

    Pcr->Number = Number;
    Pcr->SetMember = 1 << Number;
    Pcr->NotMember = ~Pcr->SetMember;

    //
    // Initialize the processor block.
    //

    Prcb->MinorVersion = PRCB_MINOR_VERSION;
    Prcb->MajorVersion = PRCB_MAJOR_VERSION;
    Prcb->BuildType = 0;

#if DBG

    Prcb->BuildType |= PRCB_BUILD_DEBUG;

#endif

#ifdef NT_UP

    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;

#endif

    Prcb->CurrentThread = Thread;
    Prcb->NextThread = (PKTHREAD)NULL;
    Prcb->IdleThread = Thread;
    Prcb->Number = Number;
    Prcb->SetMember = 1 << Number;

    KeInitializeDpc(&Prcb->QuantumEndDpc,
                    (PKDEFERRED_ROUTINE)KiQuantumEnd,
                    NIL);

    //
    // initialize the per processor lock queue entry for implemented locks.
    //

#if !defined(NT_UP)

    Prcb->LockQueue[LockQueueDispatcherLock].Next = NULL;
    Prcb->LockQueue[LockQueueDispatcherLock].Lock = &KiDispatcherLock;
    Prcb->LockQueue[LockQueueContextSwapLock].Next = NULL;
    Prcb->LockQueue[LockQueueContextSwapLock].Lock = &KiContextSwapLock;
    Prcb->LockQueue[LockQueuePfnLock].Next = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Lock = &MmPfnLock;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Next = NULL;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Lock = &MmSystemSpaceLock;
    Prcb->LockQueue[LockQueueMasterLock].Next = NULL;
    Prcb->LockQueue[LockQueueMasterLock].Lock = &CcMasterSpinLock;
    Prcb->LockQueue[LockQueueVacbLock].Next = NULL;
    Prcb->LockQueue[LockQueueVacbLock].Lock = &CcVacbSpinLock;

#endif

    //
    // Set address of PCR in PRCB.
    //

    Prcb->Pcr = Pcr;

    //
    // Initialize the interprocessor communication packet.
    //

#if !defined(NT_UP)

    Prcb->TargetSet = 0;
    Prcb->WorkerRoutine = NULL;
    Prcb->RequestSummary = 0;
    Prcb->IpiFrozen = 0;

#if NT_INST

    Prcb->IpiCounts = &KiIpiCounts[Number];

#endif //NT_INST

#endif //NT_UP

    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

    //
    // Initialize DPC listhead and lock.
    //

    InitializeListHead(&Prcb->DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcLock);

    //
    // Set address of processor block.
    //

    KiProcessorBlock[Number] = Prcb;

    //
    // Set address of process object in thread object.
    //

    Thread->ApcState.Process = Process;

    //
    // Set the appropriate member in the active processors set.
    //

    SetMember( Number, KeActiveProcessors );

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if( (Number+1) > KeNumberProcessors ){
        KeNumberProcessors = Number + 1;
    }

    //
    // Initialize processors PowerState
    //

    PoInitializePrcb (Prcb);

    //
    // Set global processor architecture, level and revision.  The
    // latter two are the least common denominator on an MP system.
    //

#ifdef _AXP64_
    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA64;
#else
    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA;
#endif

    if ((KeProcessorLevel == 0) ||
        (KeProcessorLevel > (USHORT)Pcr->ProcessorType)) {
        KeProcessorLevel = (USHORT)Pcr->ProcessorType;
    }

    if ((KeProcessorRevision == 0) ||
        (KeProcessorRevision > (USHORT)Pcr->ProcessorRevision)) {
        KeProcessorRevision = (USHORT)Pcr->ProcessorRevision;
    }

    //
    // Initialize all interrupt vectors to transfer control to the unexpected
    // interrupt routine.
    //
    // N.B. This interrupt object is never actually "connected" to an interrupt
    //      vector via KeConnectInterrupt. It is initialized and then connected
    //      by simply storing the address of the dispatch code in the interrupt
    //      vector.
    //

    if (Number == 0) {

        KeFeatureBits = KiGetFeatureBits();

        //
        // Initial the address of the interrupt dispatch routine.
        //

        KxUnexpectedInterrupt.DispatchAddress = KiUnexpectedInterrupt;

        //
        // Initialize the context swap spinlock.
        //

        KeInitializeSpinLock(&KiContextSwapLock);

        //
        // Copy the interrupt dispatch code template into the interrupt object
        // and flush the dcache on all processors that the current thread can
        // run on to ensure that the code is actually in memory.
        //

        for (Index = 0; Index < DISPATCH_LENGTH; Index += 1) {
            KxUnexpectedInterrupt.DispatchCode[Index] = KiInterruptTemplate[Index];
        }

        //
        // Sweep the instruction cache on the current processor.
        //

        KiImb();

    } else {

        //
        // Mask off feature bits that are not supported on all processors.
        //

        KeFeatureBits &= KiGetFeatureBits();
    }

    //
    // Update processor features
    //

    SharedUserData->ProcessorFeatures[PF_ALPHA_BYTE_INSTRUCTIONS] =
        (KeFeatureBits & KF_BYTE) ? TRUE : FALSE;

    for (Index = DISPATCH_LEVEL+1; Index < MAXIMUM_VECTOR; Index += 1) {
        Pcr->InterruptRoutine[Index] =  (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode);
    }

    //
    // Raise IRQL to APC level.
    //

    KeRaiseIrql(APC_LEVEL, &OldIrql);

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        //
        // Initialize the address of the restart block for the boot master.
        //

        Prcb->RestartBlock = SYSTEM_BLOCK->RestartBlock;

        //
        // Initialize the kernel debugger if enabled by the load options.
        //

        if (KdInitSystem(LoaderBlock, FALSE) == FALSE) {
            KeBugCheck(PHASE0_INITIALIZATION_FAILED);
        }

#if DBG

        //
        // Allow a breakin to the kernel debugger if one is pending.
        //

        if (KdPollBreakIn() != FALSE){
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
        }

#endif //DBG

        //
        // Initialize processor block array.
        //

        for (Index = 1; Index < MAXIMUM_PROCESSORS; Index += 1) {
            KiProcessorBlock[Index] = (PKPRCB)NULL;
        }

        //
        // Initialize default DMA coherency value for Alpha.
        //

        KiDmaIoCoherency = DMA_READ_DCACHE_INVALIDATE | DMA_WRITE_DCACHE_SNOOP;

        //
        // Perform architecture independent initialization.
        //

        KiInitSystem();

        //
        // Initialize idle thread process object and then set:
        //
        //      1. all the quantum values to the maximum possible.
        //      2. the process in the balance set.
        //      3. the active processor mask to the specified processor.
        //

        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(0xffffffff),
                            (PULONG_PTR)PDE_SELFMAP,
                            FALSE);

        Process->ThreadQuantum = MAXCHAR;
    }

    //
    // Initialize idle thread object and then set:
    //
    //      1. the initial kernel stack to the specified idle stack.
    //      2. the next processor number to the specified processor.
    //      3. the thread priority to the highest possible value.
    //      4. the state of the thread to running.
    //      5. the thread affinity to the specified processor.
    //      6. the specified processor member in the process active processors
    //          set.
    //

    KeInitializeThread(Thread,
                       (PVOID)((ULONG_PTR)IdleStack - PAGE_SIZE),
                       (PKSYSTEM_ROUTINE)NULL,
                       (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL,
                       (PCONTEXT)NULL,
                       (PVOID)NULL,
                       Process);

    Thread->InitialStack = IdleStack;
    Thread->StackBase = IdleStack;
    Thread->StackLimit = (PVOID)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);
    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (KAFFINITY)(1 << Number);
    Thread->WaitIrql = DISPATCH_LEVEL;

    //
    // If the current processor is the boot master then set the appropriate
    // bit in the active summary of the idle process.
    //

    if (Number == 0) {
        SetMember(Number, Process->ActiveProcessors);
    }

    //
    // call the executive initialization routine.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except(KeBugCheckEx(PHASE0_EXCEPTION,
                          (ULONG)GetExceptionCode(),
                          (ULONG_PTR)GetExceptionInformation(),
                          0,0), EXCEPTION_EXECUTE_HANDLER) {
        ; // should never get here
    }

    //
    // If the initial processor is being initialized, then compute the
    // timer table reciprocal value and reset the PRCB values for
    // the controllable DPC behavior in order to reflect any registry
    // overrides.
    //

    if (Number == 0) {
        KiTimeIncrementReciprocal = KiComputeReciprocal((LONG)KeMaximumIncrement,
                                                        &KiTimeIncrementShiftCount);

        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    }

    //
    // Try to enable automatic PAL code fixups on this processor.
    // This must be done after the configuration values are read
    // out of the registry in ExpInitializeExecutive.
    //

    KiDisableAlignmentExceptions();

    //
    //
    // Raise IRQL to dispatch level and set the priority of the idle thread
    // to zero. This will have the effect of immediately causing the phase
    // one initialization thread to get scheduled for execution. The idle
    // thread priority is then set to the lowest realtime priority.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, (KPRIORITY)0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // If a restart block exists for the current processor then set boot
    // completed.
    //

#if !defined(NT_UP)

    RestartBlock = Prcb->RestartBlock;
    if (RestartBlock != NULL) {
        RestartBlock->BootStatus.BootFinished = 1;
    }

    //
    // If the current processor is a secondary processor and a thread has
    // not been selected for execution, then set the appropriate bit in the
    // idle summary.
    //

    if ((Number != 0) && (Prcb->NextThread == NULL)) {
        SetMember(Number, KiIdleSummary);
    }

#endif //NT_UP

    return;
}

ULONG
KiGetFeatureBits(
    VOID
    )

/*++

    Return the NT feature bits supported by this processors

--*/

{

    ULONG Features = 0;

    //
    // Check for byte instructions.
    //

    if (amask(1) == 0) {
        Features |= KF_BYTE;
    }

    return Features;
}

#if defined(_AXP64_)


VOID
KiStartHalThread (
    IN PKTHREAD Thread,
    IN PVOID Stack,
    IN PKSTART_ROUTINE StartRoutine,
    IN ULONG_PTR Process
    )

/*++

Routine Description:

    This function is called to initialize and start a thread from the
    HAL to emulate BIOS calls.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Stack - Supplies a pointer the base of the real kernel stack for
        thread.

    StartRoutine - Supplies the address of the start routine.

Return Value:

    None.

--*/

{

    //
    // Initialize the specified thread object.
    //

    KeInitializeThread(Thread,
                       Stack,
                       (PKSYSTEM_ROUTINE)StartRoutine,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (PKPROCESS)Process);

    //
    // Set thread priority.
    //

    Thread->Priority = (KPRIORITY) NORMAL_BASE_PRIORITY - 1;
    KeReadyThread(Thread);
    return;
}

#endif
