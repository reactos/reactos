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

ULONG
HalGetInterruptSource(VOID);

VOID FASTCALL
HalClearSoftwareInterrupt(IN KIRQL Request);

VOID
KiSwapContextInternal(IN PKTHREAD OldThread,
                      IN PKTHREAD NewThread)
{
    //
    // FIXME: TODO
    //
    DPRINT1("Switching from: %p to %p\n", OldThread, NewThread);
    DPRINT1("Stacks: %p %p\n", OldThread->KernelStack, NewThread->KernelStack);
    while (TRUE);
}

VOID
KiDispatchInterrupt(VOID)
{
    PKPCR Pcr;
    PKTHREAD NewThread, OldThread;
    
    //
    // Get the PCR and disable interrupts
    //
    Pcr = (PKPCR)KeGetPcr();
    _disable();
    
    //
    //Check if we have to deliver DPCs, timers, or deferred threads
    //
    if ((Pcr->Prcb->DpcData[0].DpcQueueDepth) ||
        (Pcr->Prcb->TimerRequest) ||
        (Pcr->Prcb->DeferredReadyListHead.Next))
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
    if (Pcr->Prcb->QuantumEnd)
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
    if (Pcr->Prcb->NextThread)
    {
        DPRINT1("Switching threads!\n");
        
        //
        // Next is now current
        //
        OldThread = Pcr->Prcb->CurrentThread;
        NewThread = Pcr->Prcb->NextThread;
        Pcr->Prcb->CurrentThread = NewThread;
        Pcr->Prcb->NextThread = NULL;
        
        //
        // Update thread states
        //
        NewThread->State = Running;
        OldThread->WaitReason = WrDispatchInt;
        
        //
        // Make the old thread ready
        //
        DPRINT1("Queueing the ready thread\n");
        KxQueueReadyThread(OldThread, Pcr->Prcb);
        
        //
        // Swap to the new thread
        // On ARM we call KiSwapContext instead of KiSwapContextInternal,
        // because we're calling this from C code and not assembly.
        // This is similar to how it gets called for unwaiting, on x86
        //
        DPRINT1("Swapping context!\n");
        KiSwapContext(OldThread, NewThread);
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
    // Get the old IRQL
    //
    OldIrql = KeGetCurrentIrql();
    TrapFrame->OldIrql = OldIrql;
    
    //
    // Get the interrupt source
    //
    InterruptCause = HalGetInterruptSource();
    DPRINT1("Interrupt (%x) @ %p %p\n", InterruptCause, TrapFrame->SvcLr, TrapFrame->Pc);
    DPRINT1("OLD IRQL: %x\n", OldIrql);

    //
    // Get the new IRQL and Interrupt Mask
    //
    Pcr = (PKPCR)KeGetPcr();
    Irql = Pcr->IrqlMask[InterruptCause];
    InterruptMask = Pcr->IrqlTable[Irql];
    DPRINT1("IRQL (%x) MASK (%x)\n", Irql, InterruptMask);
    
    //
    // Make sure the IRQL is valid
    //
    if (OldIrql < Irql)
    {
        //
        // We should just return, probably
        //
        DPRINT1("IRQL Race!\n");
        while (TRUE);
    }
    
    //
    // Check if this interrupt is at DISPATCH or higher
    //
    if (Irql > DISPATCH_LEVEL)
    {
        //
        // ISR Handling Code
        //
        DPRINT1("ISR!\n");
        while (TRUE);
    }
    
    //
    // We know this is APC or DPC.
    // Clear the software interrupt.
    // Reenable interrupts and update the IRQL
    //
    HalClearSoftwareInterrupt(Irql);
    _enable();
    Pcr->CurrentIrql = Irql;
    
    //
    // Increment interrupt count
    //
    Pcr->Prcb->InterruptCount++;
    
    //
    // Call the registered interrupt routine
    //
    DPRINT1("Calling handler\n");
    Pcr->InterruptRoutine[Irql]();
    DPRINT1("Done!\n");
    while (TRUE);
}

NTSTATUS
KiDataAbortHandler(IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status;
    PVOID Address = (PVOID)KeArmFaultAddressRegisterGet();
    DPRINT1("Data Abort (%x) @ %p %p\n", Address, TrapFrame->SvcLr, TrapFrame->Pc);
    DPRINT1("Abort Reason: %d\n", KeArmFaultStatusRegisterGet());
    
    //
    // Check if this is a page fault
    //
    if (KeArmFaultStatusRegisterGet() == 21 || KeArmFaultStatusRegisterGet() == 23)
    {
        Status = MmAccessFault(FALSE,
                               Address,
                               KernelMode,
                               TrapFrame);
        DPRINT1("Status: %x\n", Status);
        if (Status == STATUS_SUCCESS) return Status;
    }
    
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
    DPRINT1("System call (%X) from thread: %p (%d) \n", Id, Thread, Thread->PreviousMode);
    
    //
    // Get the descriptor table
    //
    ServiceTable = (ULONG_PTR)Thread->ServiceTable;
    Offset = ((Id >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK);
    ServiceTable += Offset;
    DescriptorTable = (PVOID)ServiceTable;
    DPRINT1("Descriptor Table: %p (Count %d)\n", DescriptorTable, DescriptorTable->Limit);
    
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
    DPRINT1("Handler: %p\n", SystemCall);
    DPRINT1("NtClose: %p\n", NtClose);
    
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
    DPRINT1("Number: %d\n", Number);
    ArgumentCount = DescriptorTable->Number[Number] / 4;
    ASSERT(ArgumentCount <= 20);
    DPRINT1("Argument Count: %d\n", ArgumentCount);
    
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
        Arguments[i] = *Argument;
        Argument++;
        DPRINT1("Argument %d: %x\n", i, Arguments[i]);
    }
    
    //
    // If more than four, we'll have some on the user stack
    //
    if (ArgumentCount > 4)
    {
        //
        // FIXME: Validate the user stack
        //
        DPRINT1("User stack: %p\n", TrapFrame->UserSp);
        
        //
        // Copy the rest
        //
        Argument = (PULONG)TrapFrame->UserSp;
        for (i = 4; i < ArgumentCount; i++)
        {
            //
            // Copy into kernel stack
            //
            Arguments[i] = *Argument;
            Argument++;
            DPRINT1("Argument %d: %x\n", i, Arguments[i]);
        }
    }
    
    //
    // Do the system call and save result in EAX
    //
    TrapFrame->R0 = KiSystemCall(SystemCall, Arguments, ArgumentCount);
}

VOID
KiSoftwareInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KPROCESSOR_MODE PreviousMode;
    ULONG Instruction;
    DPRINT1("SWI @ %p %p \n", TrapFrame->Pc, TrapFrame->UserLr);
    
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
