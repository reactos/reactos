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

--*/

#include "ki.h"

//
// Put all code for kernel initialization in the INIT section. It will be
// deallocated by memory management when phase 1 initialization is completed.
//

#if defined(ALLOC_PRAGMA)

#pragma alloc_text(INIT, KiInitializeKernel)

#endif

#if !defined(NT_UP)
VOID
KiPhase0SyncIoMap (
    VOID
    );
#endif

VOID
KiSetDbatInvalid(
    IN ULONG Number
    );

VOID
KiSetDbat(
    IN ULONG Number,
    IN ULONG PhysicalAddress,
    IN ULONG VirtualAddress,
    IN ULONG Length,
    IN ULONG Coherence
    );

ULONG
KiInitExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
#if DBG
    DbgPrint("KE: Phase0 Exception Pointers = %x\n",ExceptionPointers);
    DbgPrint("Code %x Addr %lx Info0 %x Info1 %x Info2 %x Info3 %x\n",
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG)ExceptionPointers->ExceptionRecord->ExceptionAddress,
        ExceptionPointers->ExceptionRecord->ExceptionInformation[0],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[1],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[2],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[3]
        );
    DbgBreakPoint();
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}

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
    PRESTART_BLOCK RestartBlock;
    KPCR *Pcr;

    //
    // Before the hashed page table is set up, the PCR must be referred
    // to by its SPRG.1 address, not by its 0xffffd000 address.
    //

    Pcr = (PKPCR)PCRsprg1;

#if !defined(NT_UP)

    if (Number != 0) {
        KiPhase0SyncIoMap();
    }

#endif

    //
    // Set processor version and revision in PCR
    //

    Pcr->ProcessorRevision = KeGetPvr() & 0xFFFF;
    Pcr->ProcessorVersion = (KeGetPvr() >> 16);

    //
    // Set global processor architecture, level and revision.  The
    // latter two are the least common denominator on an MP system.
    //

    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_PPC;
    KeFeatureBits = 0;
    if (KeProcessorLevel == 0 ||
        KeProcessorLevel > (USHORT)Pcr->ProcessorVersion
       ) {
        KeProcessorLevel = (USHORT)Pcr->ProcessorVersion;
    }
    if (KeProcessorRevision == 0 ||
        KeProcessorRevision > (USHORT)Pcr->ProcessorRevision
       ) {
        KeProcessorRevision = (USHORT)Pcr->ProcessorRevision;
    }

    //
    // Perform platform dependent processor initialization.
    //

    HalInitializeProcessor(Number);
    HalSweepIcache();

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
    Prcb->SetMember = 1 << Number;
    Prcb->PcrPage = LoaderBlock->u.Ppc.PcrPage;
    Prcb->ProcessorState.SpecialRegisters.Sprg0 =
                    LoaderBlock->u.Ppc.PcrPage << PAGE_SHIFT;
    Prcb->ProcessorState.SpecialRegisters.Sprg1 = (ULONG)Pcr;

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
    // Initialize the idle thread initial kernel stack value.
    //

    Pcr->InitialStack = IdleStack;
    Pcr->StackLimit = (PVOID)((ULONG)IdleStack - KERNEL_STACK_SIZE);

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
        // Initial the address of the interrupt dispatch routine.
        //

        KxUnexpectedInterrupt.DispatchAddress = KiUnexpectedInterrupt;

        //
        // Copy the interrupt dispatch function descriptor into the interrupt
        // object.
        //

        KxUnexpectedInterrupt.DispatchCode[0] =
                *(PULONG)(KxUnexpectedInterrupt.DispatchAddress);
        KxUnexpectedInterrupt.DispatchCode[1] =
                *(((PULONG)(KxUnexpectedInterrupt.DispatchAddress))+1);

        //
        // Initialize the context swap spinlock.
        //

        KeInitializeSpinLock(&KiContextSwapLock);

        //
        // Set the default DMA I/O coherency attributes.  PowerPC
        // architecture dictates that the D-Cache is fully coherent
        // but the I-Cache doesn't snoop.
        //

        KiDmaIoCoherency = DMA_READ_DCACHE_INVALIDATE | DMA_WRITE_DCACHE_SNOOP;
    }

    for (Index = 0; Index < MAXIMUM_VECTOR; Index += 1) {
        Pcr->InterruptRoutine[Index] =
                    (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode);
    }

    //
    // Initialize the profile count and interval.
    //

    Pcr->ProfileCount = 0;
    Pcr->ProfileInterval = 0x200000;

    //
    // Initialize the passive release, APC, and DPC interrupt vectors.
    //

    Pcr->InterruptRoutine[0] = KiUnexpectedInterrupt;
//  Pcr->InterruptRoutine[APC_LEVEL] = KiApcInterrupt;
//  Pcr->InterruptRoutine[DISPATCH_LEVEL] = KiDispatchInterrupt;

    // on PowerPC APC and Dispatch Level interrupts are handled explicitly
    // so handle calls thru the dispatch table as no-ops.

    Pcr->InterruptRoutine[APC_LEVEL] = KiUnexpectedInterrupt;
    Pcr->InterruptRoutine[DISPATCH_LEVEL] = KiUnexpectedInterrupt;

    Pcr->ReservedVectors = (1 << PASSIVE_LEVEL) |
                           (1 << APC_LEVEL)     |
                           (1 << DISPATCH_LEVEL);

    //
    // Initialize the set member for the current processor, set IRQL to
    // APC_LEVEL, and set the processor number.
    //

    Pcr->CurrentIrql = APC_LEVEL;
    Pcr->SetMember = 1 << Number;
    Pcr->NotMember = ~Pcr->SetMember;
    Pcr->Number = Number;

    //
    // Set the initial stall execution scale factor. This value will be
    // recomputed later by the HAL.
    //

    Pcr->StallScaleFactor = 50;

    //
    // Set address of process object in thread object.
    //

    Thread->ApcState.Process = Process;

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
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        //
        // Initialize the kernel debugger.
        //

        if (KdInitSystem(LoaderBlock, FALSE) == FALSE) {
            KeBugCheck(PHASE0_INITIALIZATION_FAILED);
        }

        //
        // Sweep both the D and the I caches.
        //

        HalSweepDcache();
        HalSweepIcache();

        //
        // Ensure there are NO stale entries in the TLB by
        // flushing the HPT/TLB even though the HPT is fresh.
        //

        KeFlushCurrentTb();

#if DBG
        if ((PCR->IcacheMode) || (PCR->DcacheMode))
        {
                DbgPrint("******  Dynamic Cache Mode Kernel Invocation\n");
                if (PCR->IcacheMode)
                    DbgPrint("******  Icache is OFF\n");
                if (PCR->DcacheMode)
                    DbgPrint("******  Dcache is OFF\n");
        }
#endif

        //
        // Initialize the address of the restart block for the boot master.
        //

        Prcb->RestartBlock = SYSTEM_BLOCK->RestartBlock;

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

        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(0xffffffff),
                            (PULONG)(PDE_BASE + ((PDE_BASE >> PDI_SHIFT - 2) & 0xffc)),
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

    KeInitializeThread(Thread, (PVOID)((ULONG)IdleStack - PAGE_SIZE),
                       (PKSYSTEM_ROUTINE)KeBugCheck,
                       (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);

    Thread->InitialStack = IdleStack;
    Thread->StackBase = IdleStack;
    Thread->StackLimit = (PVOID)((ULONG)IdleStack - KERNEL_STACK_SIZE);
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

    } except (KiInitExceptionFilter(GetExceptionInformation())) {
        KeBugCheck (PHASE0_EXCEPTION);
    }

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

    KeRaiseIrqlToDpcLevel(&OldIrql);
    KeSetPriorityThread(Thread, (KPRIORITY)0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // If a restart block exists for the current process, then set boot
    // completed.
    //
    // N.B. Firmware on uniprocessor machines configured for MP operation
    //      can have a restart block address of NULL.
    //

#if !defined(NT_UP)

    RestartBlock = Prcb->RestartBlock;
    if (RestartBlock != NULL) {
        RestartBlock->BootStatus.BootFinished = 1;
    }

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

#define NumBats                 3
#define RoundDownTo8MB(x)       ((x) & ~(0x7fffff))
#define VirtBase                0xb0000000
#define EightMeg(x)             ((x) << 23)


static struct {
    ULONG   PhysBase;
    ULONG   RefCount;
} AllocatedBats[NumBats];

PVOID
KePhase0MapIo (
    IN PVOID PhysicalAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    Assign a Virtual to Physical address translation using PowerPC
    Block Address Translation registers for use by the HAL prior to
    being able to achieve the same thing using MmMapIo.

    Up to three BATs can be used for this function.  This routine is
    extremely simplistic.  It will attempt to assign the required
    address range within an existing BAT register if possible.  In
    deference to the 601, the maximum range covered by a BAT is
    limited to 8MB.  On processors that support seperate Instruction
    and Data BATs, only the Data BAT will be set.  Caching in the
    region is disabled.

    In the first pass at this, a full 8MB will be allocated.

    The HAL should call KePhase0DeleteIoMap with the same parameters
    to release the BAT when it is able to aquire the same physical
    memory using MM.

    WARNING:  This code is NOT applicable for an MP solution.  Further
    study of this problem is required, specifically, a way of ensuring
    a change to BATs on one processor is reflected on other processors.


Arguments:

    PhysicalAddress - Address to which the HAL needs access.

    Length - Number of bytes.

Return Value:

    Virtual address corresponding to the requested physical address or 0
    if unable to allocate.

--*/

{
    ULONG Base, Offset;
    LONG i, FreeBat = -1;

    //  The maximum allocation we allow is 8MB starting at an 8MB
    //  boundary.

    Base = RoundDownTo8MB((ULONG)PhysicalAddress);
    Offset = (ULONG)PhysicalAddress - Base;

    //  Chack length is acceptable

    if ( (Offset + Length) > 0x800000 ) {
        return (PVOID)0;
    }

    for ( i = 0 ; i < NumBats ; i++ ) {
        if ( AllocatedBats[i].RefCount ) {
            if ( Base == AllocatedBats[i].PhysBase ) {
                //  We have a match, reuse this bat
                AllocatedBats[i].RefCount++;
                return (PVOID)(VirtBase + EightMeg(i) + Offset);
            }
        } else {
            //  This index is not allocated, remember.
            FreeBat = i;
        }
    }

    //  No match, we need to allocate another bat (if one is available).

    if ( FreeBat == -1 ) {
        return (PVOID)0;
    }

    AllocatedBats[FreeBat].PhysBase = Base;
    AllocatedBats[FreeBat].RefCount = 1;
    KiSetDbat(FreeBat + 1, Base, VirtBase + EightMeg(FreeBat), 0x800000, 6);
    return (PVOID)(VirtBase + EightMeg(FreeBat) + Offset);
}

VOID
KePhase0DeleteIoMap (
    IN PVOID PhysicalAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    Removes a Virtual to Physical address translation that was
    established with KePhase0MapIo.

Arguments:

    PhysicalAddress - Address to which the HAL needs access.

    Length - Number of bytes.  (Ignored)

Return Value:

    None.

--*/

{
    ULONG Base;
    LONG i;

    Base = RoundDownTo8MB((ULONG)PhysicalAddress);

    for ( i = 0 ; i < NumBats ; i++ ) {
        if ( AllocatedBats[i].RefCount ) {
            if ( Base == AllocatedBats[i].PhysBase ) {
                //  We have a match, detach from this bat
                if ( --AllocatedBats[i].RefCount == 0 ) {
                    KiSetDbatInvalid(i+1);
                }
                return;
            }
        }
    }

    // if we get here we were called to detach from memory
    // we don't have.  This is insane.

    KeBugCheck(MEMORY_MANAGEMENT);
}


#if !defined(NT_UP)

VOID
KiPhase0SyncIoMap (
    VOID
    )

/*++

Routine Description:

    This routine runs on all processors other than zero to
    ensure that all processors have the same Block Address
    Translations (BATs) established during phase 0.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LONG i;

    for ( i = 0 ; i < NumBats ; i++ ) {
        if ( AllocatedBats[i].RefCount ) {
            KiSetDbat(i + 1,
                      AllocatedBats[i].PhysBase,
                      VirtBase + EightMeg(i),
                      0x800000,
                      6);
        }
    }
}

#endif

