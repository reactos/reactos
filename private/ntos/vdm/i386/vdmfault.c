/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmfault.c

Abstract:

    This module contains the support for dispatching VDM page faults.

Author:

    Sudeep Bharati (sudeepb) 30-Aug-1992

Revision History:

--*/


#include "vdmp.h"

BOOLEAN
VdmDispatchPageFault(
    PKTRAP_FRAME TrapFrame,
    ULONG Mode,
    ULONG FaultAddr
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmDispatchPageFault)
#endif

BOOLEAN
VdmDispatchPageFault(
    PKTRAP_FRAME TrapFrame,
    ULONG Mode,
    ULONG FaultAddr
    )

/*++

Routine Description:

    This routine dispatches a v86 mode page fault to the VDM monitor.
    It verifies that the fault occurred below 1MB.


Arguments:
    TrapFrame
    Mode - 0 - if read
	   1 - if write
    FaultAddr - faulting address

Return Value:

    True if successfull, False otherwise

--*/
{
    PVDM_TIB VdmTib;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL   OldIrql;


    PAGED_CODE();

    //
    // Raise Irql to APC level...
    //
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KPROBE);
    if (!NT_SUCCESS(Status)) {
       KeLowerIrql(OldIrql);
       return(FALSE);
    }

    //
    // VdmTib is in user mode memory
    //
    try {
        //
        // Get a pointer to the VdmTib
        //
 //       VdmTib =
 //        ((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->VdmTib;

        if ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
	    (TrapFrame->SegCs != (KGDT_R3_CODE | RPL_MASK))) {

            //
	    // If the faulting address is above 1MB return failure
	    //
	    if (FaultAddr < 0x100000) {
                VdmTib->EventInfo.Event = VdmMemAccess;
                VdmTib->EventInfo.InstructionSize = 0;
                VdmTib->EventInfo.FaultInfo.FaultAddr = FaultAddr;
                VdmTib->EventInfo.FaultInfo.RWMode = Mode;
                VdmEndExecution(TrapFrame, VdmTib);
	    }
	    else {
                Status = STATUS_ILLEGAL_INSTRUCTION;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    } else {
        return TRUE;
    }

}
