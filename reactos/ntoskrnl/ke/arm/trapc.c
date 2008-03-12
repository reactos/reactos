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

/* FUNCTIONS ******************************************************************/

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

NTSTATUS
KiSystemService(IN PKTHREAD Thread,
                IN PKTRAP_FRAME TrapFrame,
                IN ULONG Instruction)
{
    ULONG Id;
    PKPCR Pcr;
    
    //
    // Increase count of system calls
    //
    Pcr = (PKPCR)KeGetPcr();
    Pcr->Prcb->KeSystemCalls++;
    
    //
    // Get the system call ID
    //
    Id = Instruction & 0xFFFFF;
    DPRINT1("System call (%X) from thread: %p \n", Id, Thread);            
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
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
    return KiSystemService(Thread, TrapFrame, Instruction);
}
