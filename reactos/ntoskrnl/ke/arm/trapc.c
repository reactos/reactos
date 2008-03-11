/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/trapc.c
 * PURPOSE:         Implements the various trap handlers for ARM exceptions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

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
    if (KeArmFaultStatusRegisterGet() == 21)
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

