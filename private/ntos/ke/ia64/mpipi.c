/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    mpipi.c

Abstract:

    This module implements MIPS specific MP routine.

Author:

    Bernard Lint 26-Jun-1996

Environment:

    Kernel mode only.

Revision History:

    Based on version of David N. Cutler 24-Apr-1993

--*/

#include "ki.h"

VOID
KiFlushUserRseState (
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the current
    processor context structure in the processor block to the
    specified trap and exception frames.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from the processor context structure
    // to the specified trap and exception frames
    //

    Prcb = KeGetCurrentPrcb();
    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &Prcb->ProcessorState.ContextFrame,
                       CONTEXT_FULL,
                       (KPROCESSOR_MODE)TrapFrame->PreviousMode);

#endif

    return;
}

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the specified trap
    and exception frames to the processor context structure in the current
    processor block.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from specified trap and exception
    // frames to the current processor context structure.
    //

    Prcb = KeGetCurrentPrcb();
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame,
                         ExceptionFrame,
                         &Prcb->ProcessorState.ContextFrame);

    if (TrapFrame->PreviousMode == UserMode)
        KiFlushUserRseState(TrapFrame);

    //
    // Save ISR in special registers
    //

    Prcb->ProcessorState.SpecialRegisters.StISR = TrapFrame->StISR;

    //
    // Save the current processor control state.
    //

    KiSaveProcessorControlState(&Prcb->ProcessorState);

#endif

    return;
}

BOOLEAN
KiIpiServiceRoutine (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:


    This function is called at IPI_LEVEL to process any outstanding
    interprocess request for the current processor.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame

Return Value:

    A value of TRUE is returned, if one of more requests were service.
    Otherwise, FALSE is returned.

--*/

{
    ULONG RequestSummary;

    //
    // Process any outstanding IPI requests
    //

    RequestSummary = KiIpiProcessRequests();

    //
    // If freeze is requested, then freeze target execution.
    //

    if ((RequestSummary & IPI_FREEZE) != 0) {
        KiFreezeTargetExecution(TrapFrame, ExceptionFrame);
    }

    return ((RequestSummary & ~IPI_FREEZE) != 0);
}

ULONG
KiIpiProcessRequests (
    VOID
    )

/*++

Routine Description:

    This routine processes interprocessor requests and returns a summary
    of the requests that were processed.

Arguments:

    None.

Return Value:

    The request summary is returned as the function value.

--*/
{
    ULONG RequestSummary;
    PKPRCB SignalDone;
    PKPRCB Prcb = KeGetCurrentPrcb();

    RequestSummary = (ULONG)InterlockedExchange((PLONG)&Prcb->RequestSummary, 0);

    //
    // If a packet is ready, then get the address of the requested function
    // and call the function passing the address of the packet address as a
    // parameter.
    //

    SignalDone = (PKPRCB)Prcb->SignalDone;

    if (SignalDone != 0) {
     
        Prcb->SignalDone = 0;

        (*SignalDone->WorkerRoutine) ((PKIPI_CONTEXT)SignalDone,
                                      SignalDone->CurrentPacket[0], 
                                      SignalDone->CurrentPacket[1], 
                                      SignalDone->CurrentPacket[2]);

    } 

    if ((RequestSummary & IPI_APC) != 0) {
        KiRequestSoftwareInterrupt (APC_LEVEL);
    } else if ((RequestSummary & IPI_DPC) != 0) {
        KiRequestSoftwareInterrupt (DISPATCH_LEVEL);
    }

    return RequestSummary;
}


VOID
KiIpiSend (
    IN KAFFINITY TargetProcessors,
    IN KIPI_REQUEST IpiRequest
    )

/*++

Routine Description:

    This routine requests the specified operation on the target set of
    processors.

Arguments:

    TargetProcessors (a0) - Supplies the set of processors on which the
        specified operation is to be executed.

    IpiRequest (a1) - Supplies the request operation mask.

Return Value:

    None.

--*/

{
#if !defined(NT_UP)
    ULONG RequestSummary;
    KAFFINITY NextProcessors;
    ULONG Next;

    //
    // Loop through the target processors and send the packet to the specified
    // recipients.
    //

    NextProcessors = TargetProcessors;
    Next = 0;

    while (NextProcessors != 0) {

        if ((NextProcessors & 1) != 0) {

            do {
            
                RequestSummary = KiProcessorBlock[Next]->RequestSummary;

            } while(InterlockedCompareExchange(
                (PLONG) &KiProcessorBlock[Next]->RequestSummary, 
                (LONG) (RequestSummary | IpiRequest),
                (LONG) RequestSummary) != (LONG) RequestSummary);  
        }

        NextProcessors = NextProcessors >> 1;
        
        Next = Next + 1;

    }
    HalRequestIpi (TargetProcessors);
#endif

    return;
}


VOID
KiIpiSendPacket (
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This routine executes the specified worker function on the specified
    set of processors.

Arguments:

    TargetProcessors (a0) - Supplies the set of processors on which the
        specified operation is to be executed.

    WorkerFunction (a1) - Supplies the address of the worker function.

    Parameter1 - Parameter3 (a2, a3, 4 * 4(sp)) - Supplies worker
        function specific parameters.

Return Value:

    None.

--*/
{
#if !defined(NT_UP)
    PKPRCB Prcb;
    KAFFINITY NextProcessors;
    ULONG Next;

    Prcb = KeGetCurrentPrcb();
    Prcb->TargetSet = TargetProcessors;
    Prcb->WorkerRoutine = WorkerFunction;
    Prcb->CurrentPacket[0] = Parameter1;
    Prcb->CurrentPacket[1] = Parameter2;
    Prcb->CurrentPacket[2] = Parameter3;

    //
    // synchronize memory access
    // 

    __mf();
    
    //
    // Loop through the target processors and send the packet to the specified
    // recipients.
    //

    NextProcessors = TargetProcessors;
    Next = 0;

    while (NextProcessors != 0) {

        if ((NextProcessors & 1) != 0) {
            
            while(InterlockedCompareExchangePointer(
                (PVOID)&KiProcessorBlock[Next]->SignalDone, 
                (PVOID)Prcb,
                (PVOID)0) != (PVOID)0);

        }
            
        NextProcessors = NextProcessors >> 1;
        
        Next = Next + 1;

    }
    HalRequestIpi (TargetProcessors);
#endif    
}


#if !defined(NT_UP)

VOID
KiIpiSignalPacketDone (
   IN PKPRCB SignalDone
   )

/*++

Routine Description:

    This routine signals that a processor has completed a packet by
    clearing the calling processor's set member of the requesting
    processor's packet.

Arguments:

    SignalDone (a0) - Supplies a pointer to the processor block of the
        sending processor.

Return Value:

    None.

--*/
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    KAFFINITY TargetSet; 

    do {

        TargetSet = SignalDone->TargetSet; 

    } while (InterlockedCompareExchange(
        (PLONG) &SignalDone->TargetSet, 
        (LONG) (TargetSet ^ Prcb->SetMember), 
        (LONG) TargetSet) != (LONG)TargetSet);
}

#endif // !defined(NT_UP)
