/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
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

NTSTATUS
KiSystemCall(
    IN PVOID Handler,
    IN PULONG Arguments,
    IN ULONG ArgumentCount
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
            // FIXME: TODO
            //
            DPRINT1("DPC/Timer Delivery!\n");
            while (TRUE);
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
            while (TRUE);
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
        while (TRUE);
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
            while (TRUE);
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
        while (TRUE);
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
        while (TRUE);
    }
    
    //
    // Return
    //
    return FALSE;
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
        // FIXME: TODO
        //
        DPRINT1("DPC/Timer Delivery!\n");
        while (TRUE);
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
        // FIXME: TODO
        //
        DPRINT1("Quantum End!\n");
        while (TRUE);
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
        while (TRUE);
    }
}

VOID
KiInterruptHandler(IN PKTRAP_FRAME TrapFrame,
                   IN ULONG Reserved)
{
    KIRQL OldIrql, Irql;
    ULONG InterruptCause, InterruptMask;
    PKPCR Pcr;

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
//    DPRINT1("[ISR RETURN]\n");
    
    //
    // Re-enable interrupts and return IRQL
    //
    KeLowerIrql(OldIrql);
    _enable();
}

NTSTATUS
KiDataAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status;
    PVOID Address = (PVOID)KeArmFaultAddressRegisterGet();
    DPRINT1("[ABORT] (%x) @ %p/%p/%p\n",
            KeArmFaultStatusRegisterGet(), Address, TrapFrame->SvcLr, TrapFrame->Pc);
    
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
    
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
KiSystemService(IN PKTHREAD Thread,
                IN PKTRAP_FRAME TrapFrame,
                IN ULONG Instruction)
{
    ULONG Id, Number, ArgumentCount, i;
    PKPCR Pcr;
    ULONG_PTR ServiceTable, Offset;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    PVOID SystemCall;
    PULONG Argument;
    ULONG Arguments[16]; // Maximum 20 arguments
    
    //
    // Increase count of system calls
    //
    Pcr = (PKPCR)KeGetPcr();
    Pcr->Prcb->KeSystemCalls++;
    
    //
    // Get the system call ID
    //
    Id = Instruction & 0xFFFFF;
    DPRINT1("[SWI] (%x) %p (%d) \n", Id, Thread, Thread->PreviousMode);
    
    //
    // Get the descriptor table
    //
    ServiceTable = (ULONG_PTR)Thread->ServiceTable;
    Offset = ((Id >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK);
    ServiceTable += Offset;
    DescriptorTable = (PVOID)ServiceTable;
    
    //
    // Get the service call number and validate it
    //
    Number = Id & SERVICE_NUMBER_MASK;
    if (Number > DescriptorTable->Limit)
    {
        //
        // Check if this is a GUI call
        //
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    //
    // Save the function responsible for handling this system call
    //
    SystemCall = (PVOID)DescriptorTable->Base[Number];
    
    //
    // Check if this is a GUI call
    //
    if (Offset & SERVICE_TABLE_TEST)
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    //
    // Check how many arguments this system call takes
    //
    ArgumentCount = DescriptorTable->Number[Number] / 4;
    ASSERT(ArgumentCount <= 20);
    
    //
    // Copy the register-arguments first
    // First four arguments are in a1, a2, a3, a4
    //
    Argument = &TrapFrame->R0;
    for (i = 0; (i < ArgumentCount) && (i < 4); i++)
    {
        //
        // Copy them into the kernel stack
        //
        DPRINT1("Argument: %p\n", *Argument);
        Arguments[i] = *Argument;
        Argument++;
    }
    
    //
    // If more than four, we'll have some on the user stack
    //
    if (ArgumentCount > 4)
    {
        //
        // Check where the stack is
        //
        if (Thread->PreviousMode == UserMode)
        {
            //
            // FIXME: Validate the user stack
            //
            Argument = (PULONG)TrapFrame->UserSp;
        }
        else
        {
            //
            // We were called from the kernel
            //
            Argument = (PULONG)TrapFrame->SvcSp;
            
            //
            // Bias for the values we saved
            //
            Argument += 2;
        }

        //
        // Copy the rest
        //
        for (i = 4; i < ArgumentCount; i++)
        {
            //
            // Copy into kernel stack
            //
            DPRINT1("Argument: %p\n", *Argument);
            Arguments[i] = *Argument;
            Argument++;
        }
    }
    
    //
    // Do the system call and save result in EAX
    //
    TrapFrame->R0 = KiSystemCall(SystemCall, Arguments, ArgumentCount);
    DPRINT1("Returned: %lx\n", TrapFrame->R0);
}

VOID
KiSoftwareInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KPROCESSOR_MODE PreviousMode;
    ULONG Instruction;
    
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
    //TrapFrame->PreviousMode = PreviousMode;
    
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
