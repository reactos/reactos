/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1998  Intel Corporation

Module Name:

    intsupc.c

Abstract:

    This module implements ruotines for interrupt support.

Author:

    Bernard Lint 5-May-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiLowerIrqlSpecial(KIRQL);

VOID
KiDispatchSoftwareInterrupt (
    KIRQL Irql
    )

/*++

Routine Description:

    Dispatch pending software interrupt

Arguments:

    Irql (a0) - Software interrupt to dispatch

Return Value:

    None.

Notes:

    Interrupts disabled on entry/return.
    The function is only called by KiCheckForSoftwareInterrupt that passes an
    Irql value of APC_LEVEL or DISPATCH_LEVEL.

    
--*/

{
    PKPRCB Prcb = KeGetCurrentPrcb();

    KiLowerIrqlSpecial(Irql); // set IRQL

    if (Irql == APC_LEVEL) {

        Prcb->ApcBypassCount += 1;
        PCR->ApcInterrupt = 0;
        
        _enable();
        
        //
        // Dispatch APC Interrupt via direct call to KiDeliverApc
        //

        KiDeliverApc(KernelMode,NULL,NULL);
    
        _disable();

    } else {

        Prcb->DpcBypassCount += 1;
        PCR->DispatchInterrupt = 0;
        
        _enable();
        
        //
        // Dispatch DPC Interrupt
        //

        KiDispatchInterrupt();
    
        _disable();

    }
}

VOID
KiCheckForSoftwareInterrupt (
    KIRQL RequestIrql
    )

/*++

Routine Description:

    Check for and dispatch pending software interrupts

Arguments:

    Irql (a0) - New, lower IRQL

Return Value:

    None.

Notes:

    Caller must check IRQL has dropped below s/w IRQL level
    
--*/

{
    BOOLEAN InterruptState;
    
    InterruptState = KiDisableInterrupts();
    
    if (RequestIrql == APC_LEVEL) {

        //
        // Dispatch only DPC requests
        //

        while (PCR->DispatchInterrupt) {
            KiDispatchSoftwareInterrupt(DISPATCH_LEVEL);
        }

    } else {

        //
        // Dispatch either APC or DPC
        //

        while (PCR->SoftwareInterruptPending) {
            KIRQL Irql;

            if (PCR->DispatchInterrupt) {
                Irql = DISPATCH_LEVEL;
            } else if (PCR->ApcInterrupt) {
                Irql = APC_LEVEL;
            }
            KiDispatchSoftwareInterrupt(Irql);
        }
    }

    KiRestoreInterrupts(InterruptState);
}

VOID
KiRequestSoftwareInterrupt (
    KIRQL RequestIrql
    )

/*++

Routine Description:

   This function requests a software interrupt at the specified IRQL
   level.

Arguments:

   RequestIrql (a0) - Supplies the request IRQL value.

Return Value:

   None.

--*/

{
#if DEBUG
    if ((RequestIrql < APC_LEVEL) || (RequestIrql > DISPATCH_LEVEL))
        KeBugCheckEx(INVALID_SOFTWARE_INTERRUPT, RequestIrql, 0, 0, 0);
#endif
    ((PUCHAR)&PCR->SoftwareInterruptPending)[RequestIrql-APC_LEVEL] = 1;
}
