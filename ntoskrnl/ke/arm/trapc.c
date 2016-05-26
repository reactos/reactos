/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/trapc.c
 * PURPOSE:         Implements the various trap handlers for ARM exceptions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

#if 0
VOID
KiIdleLoop(VOID)
{
    PKPCR Pcr = (PKPCR)KeGetPcr();
    PKPRCB Prcb = Pcr->Prcb;
    PKTHREAD OldThread, NewThread;

    //
    // Loop forever... that's why this is an idle loop
    //
    DPRINT1("[IDLE LOOP]\n");
    while (TRUE);

    while (TRUE)
    {
        //
        // Cycle interrupts
        //
        _disable();
        _enable();

        //
        // Check if there's DPC work to do
        //
        if ((Prcb->DpcData[0].DpcQueueDepth) ||
            (Prcb->TimerRequest) ||
            (Prcb->DeferredReadyListHead.Next))
        {
            //
            // Clear the pending interrupt
            //
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);

            //
            // Retire DPCs
            //
            KiRetireDpcList(Prcb);
        }

        //
        // Check if there's a thread to schedule
        //
        if (Prcb->NextThread)
        {
            //
            // Out with the old, in with the new...
            //
            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;
            Prcb->CurrentThread = NewThread;
            Prcb->NextThread = NULL;

            //
            // Update thread state
            //
            NewThread->State = Running;

            //
            // Swap to the new thread
            // On ARM we call KiSwapContext instead of KiSwapContextInternal,
            // because we're calling this from C code and not assembly.
            // This is similar to how it gets called for unwaiting, on x86
            //
            KiSwapContext(OldThread, NewThread);
        }
        else
        {
            //
            // Go into WFI (sleep more)
            //
            KeArmWaitForInterrupt();
        }
    }
}
#endif

VOID
NTAPI
KiSwapProcess(IN PKPROCESS NewProcess,
              IN PKPROCESS OldProcess)
{
    ARM_TTB_REGISTER TtbRegister;
    DPRINT1("Swapping from: %p (%16s) to %p (%16s)\n",
            OldProcess, ((PEPROCESS)OldProcess)->ImageFileName,
            NewProcess, ((PEPROCESS)NewProcess)->ImageFileName);

    //
    // Update the page directory base
    //
    TtbRegister.AsUlong = NewProcess->DirectoryTableBase[0];
    ASSERT(TtbRegister.Reserved == 0);
    KeArmTranslationTableRegisterSet(TtbRegister);

    //
    // FIXME: Flush the TLB
    //


    DPRINT1("Survived!\n");
    while (TRUE);
}

#if 0
BOOLEAN
KiSwapContextInternal(IN PKTHREAD OldThread,
                      IN PKTHREAD NewThread)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPRCB Prcb = Pcr->Prcb;
    PKPROCESS OldProcess, NewProcess;

    DPRINT1("SWAP\n");
    while (TRUE);

    //
    // Increase context switch count
    //
    Pcr->ContextSwitches++;

    //
    // Check if WMI tracing is enabled
    //
    if (Pcr->PerfGlobalGroupMask)
    {
        //
        // We don't support this yet on x86 either
        //
        DPRINT1("WMI Tracing not supported\n");
        ASSERT(FALSE);
    }

    //
    // Check if the processes are also different
    //
    OldProcess = OldThread->ApcState.Process;
    NewProcess = NewThread->ApcState.Process;
    if (OldProcess != NewProcess)
    {
        //
        // Check if address space switch is needed
        //
        if (OldProcess->DirectoryTableBase[0] !=
            NewProcess->DirectoryTableBase[0])
        {
            //
            // FIXME-USER: Support address space switch
            //
            DPRINT1("Address space switch not implemented\n");
            ASSERT(FALSE);
        }
    }

    //
    // Increase thread context switches
    //
    NewThread->ContextSwitches++;
#if 0 // I don't buy this
    //
    // Set us as the current thread
    // NOTE: On RISC Platforms, there is both a KPCR CurrentThread, and a
    // KPRCB CurrentThread.
    // The latter is set just like on x86-based builds, the former is only set
    // when actually doing the context switch (here).
    // Recall that the reason for the latter is due to the fact that the KPCR
    // is shared with user-mode (read-only), so that information is exposed
    // there as well.
    //
    Pcr->CurrentThread = NewThread;
#endif
    //
    // DPCs shouldn't be active
    //
    if (Prcb->DpcRoutineActive)
    {
        //
        // Crash the machine
        //
        KeBugCheckEx(ATTEMPTED_SWITCH_FROM_DPC,
                     (ULONG_PTR)OldThread,
                     (ULONG_PTR)NewThread,
                     (ULONG_PTR)OldThread->InitialStack,
                     0);
    }

    //
    // Kernel APCs may be pending
    //
    if (NewThread->ApcState.KernelApcPending)
    {
        //
        // Are APCs enabled?
        //
        if (NewThread->SpecialApcDisable == 0)
        {
            //
            // Request APC delivery
            //
            HalRequestSoftwareInterrupt(APC_LEVEL);
            return TRUE;
        }
    }

    //
    // Return
    //
    return FALSE;
}
#endif

VOID
KiApcInterrupt(VOID)
{
    KPROCESSOR_MODE PreviousMode;
    KEXCEPTION_FRAME ExceptionFrame;
    PKTRAP_FRAME TrapFrame = KeGetCurrentThread()->TrapFrame;

    DPRINT1("[APC TRAP]\n");
    while (TRUE);

    //
    // Isolate previous mode
    //
    PreviousMode = KiGetPreviousMode(TrapFrame);

    //
    // FIXME-USER: Handle APC interrupt while in user-mode
    //
    if (PreviousMode == UserMode) ASSERT(FALSE);

    //
    // Disable interrupts
    //
    _disable();

    //
    // Clear APC interrupt
    //
    HalClearSoftwareInterrupt(APC_LEVEL);

    //
    // Re-enable interrupts
    //
    _enable();

    //
    // Deliver APCs
    //
    KiDeliverApc(PreviousMode, &ExceptionFrame, TrapFrame);
}

#if 0
VOID
KiDispatchInterrupt(VOID)
{
    PKIPCR Pcr;
    PKPRCB Prcb;
    PKTHREAD NewThread, OldThread;

    DPRINT1("[DPC TRAP]\n");
    while (TRUE);

    //
    // Get the PCR and disable interrupts
    //
    Pcr = (PKIPCR)KeGetPcr();
    Prcb = Pcr->Prcb;
    _disable();

    //
    //Check if we have to deliver DPCs, timers, or deferred threads
    //
    if ((Prcb->DpcData[0].DpcQueueDepth) ||
        (Prcb->TimerRequest) ||
        (Prcb->DeferredReadyListHead.Next))
    {
        //
        // Retire DPCs
        //
        KiRetireDpcList(Prcb);
    }

    //
    // Re-enable interrupts
    //
    _enable();

    //
    // Check for quantum end
    //
    if (Prcb->QuantumEnd)
    {
        //
        // Handle quantum end
        //
        Prcb->QuantumEnd = FALSE;
        KiQuantumEnd();
        return;
    }

    //
    // Check if we have a thread to swap to
    //
    if (Prcb->NextThread)
    {
        //
        // Next is now current
        //
        OldThread = Prcb->CurrentThread;
        NewThread = Prcb->NextThread;
        Prcb->CurrentThread = NewThread;
        Prcb->NextThread = NULL;

        //
        // Update thread states
        //
        NewThread->State = Running;
        OldThread->WaitReason = WrDispatchInt;

        //
        // Make the old thread ready
        //
        KxQueueReadyThread(OldThread, Prcb);

        //
        // Swap to the new thread
        // On ARM we call KiSwapContext instead of KiSwapContextInternal,
        // because we're calling this from C code and not assembly.
        // This is similar to how it gets called for unwaiting, on x86
        //
        KiSwapContext(OldThread, NewThread);
    }
}
#endif

VOID
KiInterruptHandler(IN PKTRAP_FRAME TrapFrame,
                   IN ULONG Reserved)
{
    KIRQL OldIrql, Irql;
    ULONG InterruptCause;//, InterruptMask;
    PKIPCR Pcr;
    PKTRAP_FRAME OldTrapFrame;
    ASSERT(TrapFrame->Reserved == 0xBADB0D00);

    //
    // Increment interrupt count
    //
    Pcr = (PKIPCR)KeGetPcr();
    Pcr->Prcb.InterruptCount++;

    //
    // Get the old IRQL
    //
    OldIrql = KeGetCurrentIrql();
    TrapFrame->PreviousIrql = OldIrql;

    //
    // Get the interrupt source
    //
    InterruptCause = HalGetInterruptSource();
    //DPRINT1("[INT] (%x) @ %p %p\n", InterruptCause, TrapFrame->SvcLr, TrapFrame->Pc);

    //
    // Get the new IRQL and Interrupt Mask
    //
    /// FIXME: use a global table in ntoskrnl instead of HAL?
    //Irql = Pcr->IrqlMask[InterruptCause];
    //InterruptMask = Pcr->IrqlTable[Irql];
    Irql = 0;
    __debugbreak();

    //
    // Raise to the new IRQL
    //
    KfRaiseIrql(Irql);

    //
    // The clock ISR wants the trap frame as a parameter
    //
    OldTrapFrame = KeGetCurrentThread()->TrapFrame;
    KeGetCurrentThread()->TrapFrame = TrapFrame;

    //
    // Check if this interrupt is at DISPATCH or higher
    //
    if (Irql > DISPATCH_LEVEL)
    {
        //
        // FIXME-TODO: Switch to interrupt stack
        //
        //DPRINT1("[ISR]\n");
    }
    else
    {
        //
        // We know this is APC or DPC.
        //
        //DPRINT1("[DPC/APC]\n");
        HalClearSoftwareInterrupt(Irql);
    }

    //
    // Call the registered interrupt routine
    //
    /// FIXME: this should probably go into a table in ntoskrnl
    //Pcr->InterruptRoutine[Irql]();
    __debugbreak();
    ASSERT(KeGetCurrentThread()->TrapFrame == TrapFrame);
    KeGetCurrentThread()->TrapFrame = OldTrapFrame;
//    DPRINT1("[ISR RETURN]\n");

    //
    // Restore IRQL and interrupts
    //
    KeLowerIrql(OldIrql);
    _enable();
}

NTSTATUS
KiPrefetchAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    PVOID Address = (PVOID)KeArmFaultAddressRegisterGet();
    ASSERT(TrapFrame->Reserved == 0xBADB0D00);
    ULONG Instruction = *(PULONG)TrapFrame->Pc;
    ULONG DebugType, Parameter0;
    EXCEPTION_RECORD ExceptionRecord;

    DPRINT1("[PREFETCH ABORT] (%x) @ %p/%p/%p\n",
            KeArmInstructionFaultStatusRegisterGet(), Address, TrapFrame->Lr, TrapFrame->Pc);
    while (TRUE);

    //
    // What we *SHOULD* do is look at the instruction fault status register
    // and see if it's equal to 2 (debug trap). Unfortunately QEMU doesn't seem
    // to emulate this behaviour properly, so we use a workaround.
    //
    //if (KeArmInstructionFaultStatusRegisterGet() == 2)
    if (Instruction & 0xE1200070) // BKPT
    {
        //
        // Okay, we know this is a breakpoint, extract the index
        //
        DebugType = Instruction & 0xF;
        if (DebugType == BREAKPOINT_PRINT)
        {
            //
            // Debug Service
            //
            Parameter0 = TrapFrame->R0;
            TrapFrame->Pc += sizeof(ULONG);
        }
        else
        {
            //
            // Standard INT3 (emulate x86 behavior)
            //
            Parameter0 = STATUS_SUCCESS;
        }

        //
        // Build the exception record
        //
        ExceptionRecord.ExceptionCode = STATUS_BREAKPOINT;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.ExceptionAddress = (PVOID)TrapFrame->Pc;
        ExceptionRecord.NumberParameters = 3;

        //
        // Build the parameters
        //
        ExceptionRecord.ExceptionInformation[0] = Parameter0;
        ExceptionRecord.ExceptionInformation[1] = TrapFrame->R1;
        ExceptionRecord.ExceptionInformation[2] = TrapFrame->R2;

        //
        // Dispatch the exception
        //
        KiDispatchException(&ExceptionRecord,
                            NULL,
                            TrapFrame,
                            KiGetPreviousMode(TrapFrame),
                            TRUE);

        //
        // We're done
        //
        return STATUS_SUCCESS;
    }

    //
    // Unhandled
    //
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
KiDataAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status;
    PVOID Address = (PVOID)KeArmFaultAddressRegisterGet();
    ASSERT(TrapFrame->Reserved == 0xBADB0D00);

    DPRINT1("[ABORT] (%x) @ %p/%p/%p\n",
            KeArmFaultStatusRegisterGet(), Address, TrapFrame->Lr, TrapFrame->Pc);
    while (TRUE);

    //
    // Check if this is a page fault
    //
    if (KeArmFaultStatusRegisterGet() == 21 || KeArmFaultStatusRegisterGet() == 23)
    {
        Status = MmAccessFault(FALSE,
                               Address,
                               KiGetPreviousMode(TrapFrame),
                               TrapFrame);
        if (NT_SUCCESS(Status)) return Status;
    }

    //
    // Unhandled
    //
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

VOID
KiSoftwareInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KPROCESSOR_MODE PreviousMode;
    ULONG Instruction;
    ASSERT(TrapFrame->Reserved == 0xBADB0D00);

    DPRINT1("[SWI] @ %p/%p\n", TrapFrame->Lr, TrapFrame->Pc);
    while (TRUE);

    //
    // Get the current thread
    //
    Thread = KeGetCurrentThread();

    //
    // Isolate previous mode
    //
    PreviousMode = KiGetPreviousMode(TrapFrame);

    //
    // Save old previous mode
    //
    TrapFrame->PreviousMode = PreviousMode;
    TrapFrame->TrapFrame = (ULONG_PTR)Thread->TrapFrame;

    //
    // Save previous mode and trap frame
    //
    Thread->TrapFrame = TrapFrame;
    Thread->PreviousMode = PreviousMode;

    //
    // Read the opcode
    //
    Instruction = *(PULONG)(TrapFrame->Pc - sizeof(ULONG));

    //
    // Call the service call dispatcher
    //
    KiSystemService(Thread, TrapFrame, Instruction);
}

NTSTATUS
KiUndefinedExceptionHandler(IN PKTRAP_FRAME TrapFrame)
{
    ASSERT(TrapFrame->Reserved == 0xBADB0D00);

    //
    // This should never happen
    //
    DPRINT1("[UNDEF] @ %p/%p\n", TrapFrame->Lr, TrapFrame->Pc);
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}
