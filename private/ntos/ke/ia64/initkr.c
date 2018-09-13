/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initkr.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, and the processor control
    block.

Author:

    Bernard Lint 8-Aug-1996

Environment:

    Kernel mode only.

Revision History:

    Based on MIPS version (David N. Cutler (davec) 11-Apr-1990)
--*/

#include "ki.h"

//
// Put all code for kernel initialization in the INIT section. It will be
// deallocated by memory management when phase 1 initialization is completed.
//

#if defined(ALLOC_PRAGMA)

#pragma alloc_text(INIT, KiInitializeKernel)

#endif

//
// KiTbBroadcastLock - This is the spin lock that prevents the other processors
// from issuing PTC.G (TB purge broadcast) operations.
//

KSPIN_LOCK KiTbBroadcastLock;

//
// KiMasterRidLock - This is the spin lock that prevents the other processors 
// from updating KiMasterRid.
//

KSPIN_LOCK KiMasterRidLock;

//
// KiCacheFlushLock - This is the spin lock that ensures cache flush is only 
// done on one processor at a time. (SAL cache flush not yet MP safe).
//

KSPIN_LOCK KiCacheFlushLock;

//
// KiUserSharedDataPage - This holds the page number of UserSharedDataPage for 
// MP boot
//

ULONG_PTR KiUserSharedDataPage;

//
// KiKernelPcrPage - This holds the page number of per-processor PCR page for
// MP boot
//

ULONG_PTR KiKernelPcrPage = 0i64;


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

    LONG Index;
    KIRQL OldIrql;
    ULONG_PTR DirectoryTableBase[2];

    //
    // Perform platform dependent processor initialization.
    //

    HalInitializeProcessor(Number, LoaderBlock);

    //
    // Save the address of the loader parameter block.
    //

    KeLoaderBlock = LoaderBlock;

    //
    // Initialize the processor block.
    //

    Prcb->MinorVersion = PRCB_MINOR_VERSION;
    Prcb->MajorVersion = PRCB_MAJOR_VERSION;
    Prcb->BuildType = 0;

#if DBG

    Prcb->BuildType |= PRCB_BUILD_DEBUG;

#endif

#if defined(NT_UP)

    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;

#endif

    Prcb->CurrentThread = Thread;
    Prcb->NextThread = (PKTHREAD)NULL;
    Prcb->IdleThread = Thread;
    Prcb->Number = Number;
    Prcb->SetMember = 1 << Number;
    Prcb->PcrPage = LoaderBlock->u.Ia64.PcrPage;

#if !defined(NT_UP)

    Prcb->TargetSet = 0;
    Prcb->WorkerRoutine = NULL;
    Prcb->RequestSummary = 0;
    Prcb->IpiFrozen = 0;

#if NT_INST

    Prcb->IpiCounts = &KiIpiCounts[Number];

#endif

#endif

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
    // Initialize processors PowerState
    //

    PoInitializePrcb (Prcb);

    //
    // Set global processor architecture, level and revision.  The
    // latter two are the least common denominator on an MP system.
    //

    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
    KeFeatureBits = 0;
    // *** TBD when processor info available
    //    if ( KeProcessorLevel == 0 ||
    //     KeProcessorLevel > (USHORT)(PCR->ProcessorId >> 8)
    //   ) {
    //     KeProcessorLevel = (USHORT)(PCR->ProcessorId >> 8);
    // }
    // if ( KeProcessorRevision == 0 ||
    //     KeProcessorRevision > (USHORT)(PCR->ProcessorId & 0xFF)
    //   ) {
    //    KeProcessorRevision = (USHORT)(PCR->ProcessorId & 0xFF);
    // }

    KeProcessorLevel = 1;
    KeProcessorRevision = 1;

    //
    // Initialize the address of the bus error routines / machine check
    //
    // **** TBD

    //
    // Initialize the idle thread initial kernel stack and limit address value.
    //

    PCR->InitialStack = (ULONGLONG)IdleStack;  
    PCR->InitialBStore = (ULONGLONG)IdleStack; 
    PCR->StackLimit = (ULONGLONG)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);  
    PCR->BStoreLimit = (ULONGLONG)((ULONG_PTR)IdleStack + KERNEL_BSTORE_SIZE);

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

        //
        // Initialize the context swap spinlock.
        //
 
        KeInitializeSpinLock(&KiContextSwapLock);

        //
        // Initialize the Tb Broadcast spinlock.
        //

        KeInitializeSpinLock(&KiTbBroadcastLock);

        //
        // Initialize the Master Rid spinlock.
        //

        KeInitializeSpinLock(&KiMasterRidLock);

        //
        // Initialize the cache flush spinlock.
        //

        KeInitializeSpinLock(&KiCacheFlushLock);

        //
        // Initial the address of the interrupt dispatch routine.
        //

        KxUnexpectedInterrupt.DispatchAddress = KiUnexpectedInterrupt;

        //
        // Copy the interrupt dispatch function descriptor into the interrupt
        // object.
        //

        for (Index = 0; Index < DISPATCH_LENGTH; Index += 1) {
            KxUnexpectedInterrupt.DispatchCode[Index] =
                *(((PULONG)(KxUnexpectedInterrupt.DispatchAddress))+Index);
        }

        //
        // Set the default DMA I/O coherency attributes.  IA64
        // architecture dictates that the D-Cache is fully coherent.
        //

        KiDmaIoCoherency = DMA_READ_DCACHE_INVALIDATE | DMA_WRITE_DCACHE_SNOOP;

        //
        // Set KiSharedUserData for MP boot
        //

        KiUserSharedDataPage = LoaderBlock->u.Ia64.PcrPage2;

    }

    //
    // Point to UnexpectedInterrupt function pointer
    //

    for (Index = 0; Index < MAXIMUM_VECTOR; Index += 1) {
        PCR->InterruptRoutine[Index] =
                    (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode);
    }

    //
    // Initialize the profile count and interval.
    //

    PCR->ProfileCount = 0;
    PCR->ProfileInterval = 0x200000;

    //
    // Initialize the passive release, APC, and DPC interrupt vectors.
    //

    PCR->InterruptRoutine[0] = KiPassiveRelease;
    PCR->InterruptRoutine[APC_VECTOR] = KiApcInterrupt;
    PCR->InterruptRoutine[DISPATCH_VECTOR] = KiDispatchInterrupt;

    //
    // N.B. Reserve levels, not vectors
    //

    PCR->ReservedVectors = (1 << PASSIVE_LEVEL) | (1 << APC_LEVEL) | (1 << DISPATCH_LEVEL);

    //
    // Initialize the set member for the current processor, set IRQL to
    // APC_LEVEL, and set the processor number.
    //

    KeLowerIrql(APC_LEVEL);
    PCR->SetMember = 1 << Number;
    PCR->NotMember = ~PCR->SetMember;
    PCR->Number = Number;

    //
    // Set the initial stall execution scale factor. This value will be
    // recomputed later by the HAL.
    //

    PCR->StallScaleFactor = 50;

    //
    // Set address of process object in thread object.
    //

    Thread->ApcState.Process = Process;


    //
    // Set idle process id (region id) to STARTID
    //

    Process->ProcessRegion.RegionId = START_PROCESS_RID;
    Process->ProcessRegion.SequenceNumber = START_SEQUENCE;
    Process->SessionRegion.RegionId = START_SESSION_RID;
    Process->SessionRegion.RegionId = START_SEQUENCE;

    //
    // Set the appropriate member in the active processors set.
    //

    SetMember(Number, KeActiveProcessors);

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if ((Number + 1) > KeNumberProcessors) {
        KeNumberProcessors = (CCHAR)(Number + 1);
    }

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        Prcb->RestartBlock = NULL;

        //
        // Initialize the kernel debugger.
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

        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;

        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(0x7f),
                            &DirectoryTableBase[0],
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

    KeInitializeThread(Thread, (PVOID)((ULONG_PTR)IdleStack - PAGE_SIZE),
                       (PKSYSTEM_ROUTINE)KeBugCheck,
                       (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);

    Thread->InitialStack = IdleStack;
    Thread->InitialBStore = IdleStack;
    Thread->StackBase = IdleStack;
    Thread->StackLimit = (PVOID)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);
    Thread->BStoreLimit = (PVOID)((ULONG_PTR)IdleStack + KERNEL_BSTORE_SIZE);
    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (KAFFINITY)(1 << Number);
    Thread->WaitIrql = DISPATCH_LEVEL;

    //
    // If the current processor is 0, then set the appropriate bit in the
    // active summary of the idle process.
    //

    if (Number == 0) {
        SetMember(Number, Process->ActiveProcessors);
    }

    //
    // Execute the executive initialization.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeBugCheck (PHASE0_EXCEPTION);
    }

#if 0
    if (Number == 0) {
        RUNTIME_FUNCTION LocalEntry;
        PRUNTIME_FUNCTION FunctionTable, FunctionEntry = NULL;
        ULONGLONG ControlPc;
        ULONG SizeOfExceptionTable;
        ULONG Size;
        LONG High;
        LONG Middle;
        LONG Low;
        extern VOID KiNormalSystemCall(VOID);

        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                            (PVOID) PsNtosImageBase,
                            TRUE,
                            IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                            &SizeOfExceptionTable);

        if (FunctionTable != NULL) {

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;
            ControlPc = ((PPLABEL_DESCRIPTOR)KiNormalSystemCall)->EntryPoint - (ULONG_PTR)PsNtosImageBase;

            while (High >= Low) {

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];

                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;
                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;
                } else {
                    break;
                }
            }
        }

        LocalEntry = *FunctionEntry;
        ControlPc = MM_EPC_VA - ((PPLABEL_DESCRIPTOR)KiNormalSystemCall)->EntryPoint;
        LocalEntry.BeginAddress += (ULONG)ControlPc;
        LocalEntry.EndAddress += (ULONG)ControlPc;
        Size = SizeOfExceptionTable - (ULONG)((ULONG_PTR)FunctionEntry - (ULONG_PTR)FunctionTable) - sizeof(RUNTIME_FUNCTION);
        
        RtlMoveMemory((PVOID)FunctionEntry, (PVOID)(FunctionEntry+1), Size);
        FunctionEntry = (PRUNTIME_FUNCTION)((ULONG_PTR)FunctionTable + SizeOfExceptionTable - sizeof(RUNTIME_FUNCTION));
        *FunctionEntry = LocalEntry;
    }
#endif // 0

    //
    // If the initial processor is being initialized, then compute the
    // timer table reciprocal value and reset the PRCB values for the
    // controllable DPC behavior in order to reflect any registry
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
    // Raise IRQL to dispatch level and set the priority of the idle thread
    // to zero. This will have the effect of immediately causing the phase
    // one initialization thread to get scheduled for execution. The idle
    // thread priority is then set to the lowest realtime priority. This is
    // necessary so that mutexes aquired at DPC level do not cause the active
    // matrix to get corrupted.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, (KPRIORITY)0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

#if !defined(NT_UP)

    //
    // Indicate boot complete on secondary processor
    //

    LoaderBlock->Prcb = 0;

    //
    // If the current processor is not 0, then set the appropriate bit in
    // idle summary.
    //

    if (Number != 0) {
        SetMember(Number, KiIdleSummary);
    }

#endif

    return;
}
