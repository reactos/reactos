/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/trapc.c
 * PURPOSE:         Implements the various trap handlers for ARM exceptions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <internal/arm/ksarm.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define KiGetPreviousMode(tf) \
    ((tf->Spsr & CPSR_MODES) == CPSR_USER_MODE) ? UserMode: KernelMode

VOID
FASTCALL
KiRetireDpcList(
    IN PKPRCB Prcb
);

VOID
FASTCALL
KiQuantumEnd(
    VOID
);

VOID
KiSystemService(
    IN PKTHREAD Thread,
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Instruction
);

/* FUNCTIONS ******************************************************************/

VOID
KiIdleLoop(VOID)
{
    PKPCR Pcr = (PKPCR)KeGetPcr();
    PKPRCB Prcb = Pcr->Prcb;
    PKTHREAD OldThread, NewThread;
    
    //
    // Loop forever... that's why this is an idle loop
    //
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
            DPRINT1("Swapping context!\n");
            KiSwapContext(OldThread, NewThread);
            DPRINT1("Back\n");
            ASSERT(FALSE);
        }
        else
        {
            //
            // FIXME: Wait-For-Interrupt ARM Opcode
            //
        }
    }
}

BOOLEAN
KiSwapContextInternal(IN PKTHREAD OldThread,
                      IN PKTHREAD NewThread)
{
    PKEXCEPTION_FRAME ExFrame = NewThread->KernelStack;
    PKPCR Pcr = (PKPCR)KeGetPcr();
    PKPRCB Prcb = Pcr->Prcb;
    PKPROCESS OldProcess, NewProcess;
    DPRINT1("Switching from: %p to %p\n", OldThread, NewThread);
    DPRINT1("Stacks: %p %p\n", OldThread->KernelStack, NewThread->KernelStack);
    DPRINT1("Thread Registers:\n"
            "R4: %lx\n"
            "R5: %lx\n"
            "R6: %lx\n"
            "R7: %lx\n"
            "R8: %lx\n"
            "R9: %lx\n"
            "R10: %lx\n"
            "R11: %lx\n"
            "Psr: %lx\n"
            "Lr: %lx\n",
            ExFrame->R4,
            ExFrame->R5,
            ExFrame->R6,
            ExFrame->R7,
            ExFrame->R8,
            ExFrame->R9,
            ExFrame->R10,
            ExFrame->R11,
            ExFrame->Psr,
            ExFrame->Lr);
    DPRINT1("Old priority: %lx\n", OldThread->Priority);
    
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
        // FIXME: TODO
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
        if (OldProcess->DirectoryTableBase.LowPart !=
            NewProcess->DirectoryTableBase.LowPart)
        {
            //
            // FIXME: TODO
            //
            DPRINT1("Address space switch not implemented\n");
            ASSERT(FALSE);
        }
    }
    
    //
    // Increase thread context switches
    //
    NewThread->ContextSwitches++;
    
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
    
    //
    // DPCs shouldn't be active
    //
    if (Prcb->DpcRoutineActive)
    {
        //
        // FIXME: FAIL
        //
        DPRINT1("DPCS ACTIVE!!!\n");
        ASSERT(FALSE);
    }
    
    //
    // Kernel APCs may be pending
    //
    if (NewThread->ApcState.KernelApcPending)
    {
        //
        // FIXME: TODO
        //
        DPRINT1("APCs pending!\n");
        ASSERT(FALSE);
    }
    
    //
    // Return
    //
    return FALSE;
}

VOID
KiApcInterrupt(VOID)
{
    KPROCESSOR_MODE PreviousMode;
    KEXCEPTION_FRAME ExceptionFrame;
    PKTRAP_FRAME TrapFrame = KeGetCurrentThread()->TrapFrame;
    //DPRINT1("[APC]\n");
       
    //
    // Isolate previous mode
    //
    PreviousMode = KiGetPreviousMode(TrapFrame);
    
    //
    // FIXME No use-mode support
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

VOID
KiDispatchInterrupt(VOID)
{
    PKPCR Pcr;
    PKPRCB Prcb;
    PKTHREAD NewThread, OldThread;
    
    //
    // Get the PCR and disable interrupts
    //
    Pcr = (PKPCR)KeGetPcr();
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
        DPRINT1("Swapping context!\n");
        KiSwapContext(OldThread, NewThread);
        DPRINT1("Back\n");
        ASSERT(FALSE);
    }
}

VOID
KiInterruptHandler(IN PKTRAP_FRAME TrapFrame,
                   IN ULONG Reserved)
{
    KIRQL OldIrql, Irql;
    ULONG InterruptCause, InterruptMask;
    PKPCR Pcr;
    PKTRAP_FRAME OldTrapFrame;
    ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00);

    //
    // Increment interrupt count
    //
    Pcr = (PKPCR)KeGetPcr();
    Pcr->Prcb->InterruptCount++;
    
    //
    // Get the old IRQL
    //
    OldIrql = KeGetCurrentIrql();
    TrapFrame->OldIrql = OldIrql;
    
    //
    // Get the interrupt source
    //
    InterruptCause = HalGetInterruptSource();
//    DPRINT1("[INT] (%x) @ %p %p\n", InterruptCause, TrapFrame->SvcLr, TrapFrame->Pc);

    //
    // Get the new IRQL and Interrupt Mask
    //
    Irql = Pcr->IrqlMask[InterruptCause];
    InterruptMask = Pcr->IrqlTable[Irql];
    
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
        // FIXME: Switch to interrupt stack
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
    Pcr->InterruptRoutine[Irql]();
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
    ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00);
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
KiDataAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status;
    PVOID Address = (PVOID)KeArmFaultAddressRegisterGet();
    ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00);
   
    //
    // Check if this is a page fault
    //
    if (KeArmFaultStatusRegisterGet() == 21 || KeArmFaultStatusRegisterGet() == 23)
    {
        Status = MmAccessFault(FALSE,
                               Address,
                               KernelMode,
                               TrapFrame);
        if (Status == STATUS_SUCCESS) return Status;
    }

    DPRINT1("[ABORT] (%x) @ %p/%p/%p\n",
            KeArmFaultStatusRegisterGet(), Address, TrapFrame->SvcLr, TrapFrame->Pc);
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
    ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00);
    
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
    // FIXME: Enable interrupts?
    //
    
    //
    // Call the service call dispatcher
    //
    KiSystemService(Thread, TrapFrame, Instruction);
}
