/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psctxmip.c

Abstract:

    This module implements function to get and set the context of a thread.

Author:

    David N. Cutler (davec) 1-Oct-1990

Revision History:

    Tom Wood (twood) 19-Aug-1994
    Update to use RtlVirtualUnwind even when there isn't a function table
    entry.  Add stack limit parameters to RtlVirtualUnwind.

--*/

#include "psp.h"
#pragma hdrstop
#define         STK_MIN_FRAME   56
extern  ULONG   KiBreakPoints;

VOID
PspGetContext (
    IN PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified trap frame
    and nonvolatile context to the specified context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextPointers - Supplies the address of context pointers record.

    ContextRecord - Supplies the address of a context record.

Return Value:

    None.

--*/

{

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Get machine state, instr address, link, count registers
        //

        ContextRecord->Msr = TrapFrame->Msr;
        ContextRecord->Iar = TrapFrame->Iar;
        ContextRecord->Lr  = TrapFrame->Lr;
        ContextRecord->Ctr = TrapFrame->Ctr;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Get volatile integer regs in trap frame are 0..12
        //

        RtlMoveMemory (&ContextRecord->Gpr0, &TrapFrame->Gpr0,
                       sizeof (ULONG) * 13);

        //
        // Get non-volatile integer regs in exception frame are 13..31
        //

        ContextRecord->Gpr13 = *ContextPointers->IntegerContext[13];
        ContextRecord->Gpr14 = *ContextPointers->IntegerContext[14];
        ContextRecord->Gpr15 = *ContextPointers->IntegerContext[15];
        ContextRecord->Gpr16 = *ContextPointers->IntegerContext[16];
        ContextRecord->Gpr17 = *ContextPointers->IntegerContext[17];
        ContextRecord->Gpr18 = *ContextPointers->IntegerContext[18];
        ContextRecord->Gpr19 = *ContextPointers->IntegerContext[19];
        ContextRecord->Gpr20 = *ContextPointers->IntegerContext[20];
        ContextRecord->Gpr21 = *ContextPointers->IntegerContext[21];
        ContextRecord->Gpr22 = *ContextPointers->IntegerContext[22];
        ContextRecord->Gpr23 = *ContextPointers->IntegerContext[23];
        ContextRecord->Gpr24 = *ContextPointers->IntegerContext[24];
        ContextRecord->Gpr25 = *ContextPointers->IntegerContext[25];
        ContextRecord->Gpr26 = *ContextPointers->IntegerContext[26];
        ContextRecord->Gpr27 = *ContextPointers->IntegerContext[27];
        ContextRecord->Gpr28 = *ContextPointers->IntegerContext[28];
        ContextRecord->Gpr29 = *ContextPointers->IntegerContext[29];
        ContextRecord->Gpr30 = *ContextPointers->IntegerContext[30];
        ContextRecord->Gpr31 = *ContextPointers->IntegerContext[31];

        //
        // The CR is made up of volatile and non-volatile fields,
        // but the entire CR is saved in the trap frame
        //

        ContextRecord->Cr = TrapFrame->Cr;

        //
        // Fixed Point Exception Register (XER) is part of the
        // integer state
        //

        ContextRecord->Xer = TrapFrame->Xer;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) ==
         CONTEXT_FLOATING_POINT) {

        //
        // Get volatile floating point regs in trap frame are 0..13
        //

        RtlMoveMemory(&ContextRecord->Fpr0, &TrapFrame->Fpr0,
                     sizeof(DOUBLE) * (14));

        //
        // Get non-volatile floating point regs 14..31
        //

        ContextRecord->Fpr14 = *ContextPointers->FloatingContext[14];
        ContextRecord->Fpr15 = *ContextPointers->FloatingContext[15];
        ContextRecord->Fpr16 = *ContextPointers->FloatingContext[16];
        ContextRecord->Fpr17 = *ContextPointers->FloatingContext[17];
        ContextRecord->Fpr18 = *ContextPointers->FloatingContext[18];
        ContextRecord->Fpr19 = *ContextPointers->FloatingContext[19];
        ContextRecord->Fpr20 = *ContextPointers->FloatingContext[20];
        ContextRecord->Fpr21 = *ContextPointers->FloatingContext[21];
        ContextRecord->Fpr22 = *ContextPointers->FloatingContext[22];
        ContextRecord->Fpr23 = *ContextPointers->FloatingContext[23];
        ContextRecord->Fpr24 = *ContextPointers->FloatingContext[24];
        ContextRecord->Fpr25 = *ContextPointers->FloatingContext[25];
        ContextRecord->Fpr26 = *ContextPointers->FloatingContext[26];
        ContextRecord->Fpr27 = *ContextPointers->FloatingContext[27];
        ContextRecord->Fpr28 = *ContextPointers->FloatingContext[28];
        ContextRecord->Fpr29 = *ContextPointers->FloatingContext[29];
        ContextRecord->Fpr30 = *ContextPointers->FloatingContext[30];
        ContextRecord->Fpr31 = *ContextPointers->FloatingContext[31];

        //
        // Get floating point status and control register.
        //

        ContextRecord->Fpscr = TrapFrame->Fpscr;
    }

    //
    // Fetch Dr register contents if requested.  Values may be trash.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) ==
        CONTEXT_DEBUG_REGISTERS) {


        ContextRecord->Dr0 = TrapFrame->Dr0;
        ContextRecord->Dr1 = TrapFrame->Dr1;
        ContextRecord->Dr2 = TrapFrame->Dr2;
        ContextRecord->Dr3 = TrapFrame->Dr3;
        ContextRecord->Dr6 = TrapFrame->Dr6;
        ContextRecord->Dr5 = 0;           // Zero initialize unused regs
        ContextRecord->Dr4 = 0;

        //
        // If it's a user mode frame, and the thread doesn't have DRs set,
        // and we just return the trash in the frame, we risk accidentally
        // making the thread active with trash values on a set.  Therefore,
        // Dr7 must be set to the number of available data address breakpoint
        // registers if we get a non-active user mode frame.
        //
        if (((TrapFrame->PreviousMode) != KernelMode) &&
            (KeGetCurrentThread()->DebugActive)) {

            ContextRecord->Dr7 = TrapFrame->Dr7;
            ContextRecord->Dr6 |= KiBreakPoints;
        } else {

            ContextRecord->Dr7 = 0;
            ContextRecord->Dr6 = KiBreakPoints;
        }
    }

    return;
}

VOID
PspSetContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE ProcessorMode
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified context
    record to the specified trap frame and nonvolatile context.

Arguments:

    TrapFrame - Supplies the address of a trap frame.

    ContextPointers - Supplies the address of a context pointers record.

    ContextRecord - Supplies the address of a context record.

    ProcessorMode - Supplies the processor mode to use when sanitizing
        the PSR and FSR.

Return Value:

    None.

--*/

{

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set instruction address, link, count, and machine state registers
        //

        TrapFrame->Lr  = ContextRecord->Lr;
        TrapFrame->Ctr = ContextRecord->Ctr;
        TrapFrame->Msr = SANITIZE_MSR(ContextRecord->Msr, ProcessorMode);

        //
        // If this is a remote call dereference the function descritor in
        // the remote threads context.
        //
        if (((ContextRecord->ContextFlags & CONTEXT_INTEGER) ==
              CONTEXT_INTEGER) &&
            (ContextRecord->Gpr2 == 0)) {
           try {

               //
               // Make sure we have read access to the function descriptor.
               //
               ProbeForRead(ContextRecord->Iar,
                            (sizeof(ULONG) * 2), sizeof(ULONG));
               TrapFrame->Iar = *((PULONG)(ContextRecord->Iar))++;
               ContextRecord->Gpr2 = *(PULONG)(ContextRecord->Iar);

           } except(EXCEPTION_EXECUTE_HANDLER) {

               //
               // Remote thread doesn't have access to the function
               // descriptor. Just set the IAR and let the thread take
               // the exception.
               //
               TrapFrame->Iar = ContextRecord->Iar;
               return;
           }
        } else {
           TrapFrame->Iar = ContextRecord->Iar;
        }
    }

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Volatile integer regs are 0..12
        //

        RtlMoveMemory(&TrapFrame->Gpr0, &ContextRecord->Gpr0,
                     sizeof(ULONG) * (13));

        //
        // Non-volatile integer regs are 13..31
        //

        *ContextPointers->IntegerContext[13] = ContextRecord->Gpr13;
        *ContextPointers->IntegerContext[14] = ContextRecord->Gpr14;
        *ContextPointers->IntegerContext[15] = ContextRecord->Gpr15;
        *ContextPointers->IntegerContext[16] = ContextRecord->Gpr16;
        *ContextPointers->IntegerContext[17] = ContextRecord->Gpr17;
        *ContextPointers->IntegerContext[18] = ContextRecord->Gpr18;
        *ContextPointers->IntegerContext[19] = ContextRecord->Gpr19;
        *ContextPointers->IntegerContext[20] = ContextRecord->Gpr20;
        *ContextPointers->IntegerContext[21] = ContextRecord->Gpr21;
        *ContextPointers->IntegerContext[22] = ContextRecord->Gpr22;
        *ContextPointers->IntegerContext[23] = ContextRecord->Gpr23;
        *ContextPointers->IntegerContext[24] = ContextRecord->Gpr24;
        *ContextPointers->IntegerContext[25] = ContextRecord->Gpr25;
        *ContextPointers->IntegerContext[26] = ContextRecord->Gpr26;
        *ContextPointers->IntegerContext[27] = ContextRecord->Gpr27;
        *ContextPointers->IntegerContext[28] = ContextRecord->Gpr28;
        *ContextPointers->IntegerContext[29] = ContextRecord->Gpr29;
        *ContextPointers->IntegerContext[30] = ContextRecord->Gpr30;
        *ContextPointers->IntegerContext[31] = ContextRecord->Gpr31;

        //
        // Copy the Condition Reg and Fixed Point Exception Reg
        //

        TrapFrame->Cr = ContextRecord->Cr;
        TrapFrame->Xer = ContextRecord->Xer;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) ==
         CONTEXT_FLOATING_POINT) {

        //
        // Volatile floating point regs are 0..13
        //

        RtlMoveMemory(&TrapFrame->Fpr0, &ContextRecord->Fpr0,
                     sizeof(DOUBLE) * (14));

        //
        // Non-volatile floating point regs are 14..31
        //

        *ContextPointers->FloatingContext[14] = ContextRecord->Fpr14;
        *ContextPointers->FloatingContext[15] = ContextRecord->Fpr15;
        *ContextPointers->FloatingContext[16] = ContextRecord->Fpr16;
        *ContextPointers->FloatingContext[17] = ContextRecord->Fpr17;
        *ContextPointers->FloatingContext[18] = ContextRecord->Fpr18;
        *ContextPointers->FloatingContext[19] = ContextRecord->Fpr19;
        *ContextPointers->FloatingContext[20] = ContextRecord->Fpr20;
        *ContextPointers->FloatingContext[21] = ContextRecord->Fpr21;
        *ContextPointers->FloatingContext[22] = ContextRecord->Fpr22;
        *ContextPointers->FloatingContext[23] = ContextRecord->Fpr23;
        *ContextPointers->FloatingContext[24] = ContextRecord->Fpr24;
        *ContextPointers->FloatingContext[25] = ContextRecord->Fpr25;
        *ContextPointers->FloatingContext[26] = ContextRecord->Fpr26;
        *ContextPointers->FloatingContext[27] = ContextRecord->Fpr27;
        *ContextPointers->FloatingContext[28] = ContextRecord->Fpr28;
        *ContextPointers->FloatingContext[29] = ContextRecord->Fpr29;
        *ContextPointers->FloatingContext[30] = ContextRecord->Fpr30;
        *ContextPointers->FloatingContext[31] = ContextRecord->Fpr31;

        //
        // Set floating point status and control register.
        //

        TrapFrame->Fpscr = SANITIZE_FPSCR(ContextRecord->Fpscr, ProcessorMode);
    }

    //
    // Set debug register state if specified.  If previous mode is user
    // mode (i.e. it's a user frame we're setting) and if effect will be to
    // cause at least one of the debug register enable bits in Dr7
    // to be set then set DebugActive to the enable bit mask.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) ==
         CONTEXT_DEBUG_REGISTERS) {

        //
        // Set the debug control register for the 601 and 604
        // indicating the number of address breakpoints supported.
        //
        TrapFrame->Dr0 = SANITIZE_DRADDR(ContextRecord->Dr0, ProcessorMode);
        TrapFrame->Dr1 = SANITIZE_DRADDR(ContextRecord->Dr1, ProcessorMode);
        TrapFrame->Dr2 = SANITIZE_DRADDR(ContextRecord->Dr2, ProcessorMode);
        TrapFrame->Dr3 = SANITIZE_DRADDR(ContextRecord->Dr3, ProcessorMode);
        TrapFrame->Dr6 = SANITIZE_DR6(ContextRecord->Dr6, ProcessorMode);
        TrapFrame->Dr7 = SANITIZE_DR7(ContextRecord->Dr7, ProcessorMode);

        if (ProcessorMode != KernelMode) {
              KeGetPcr()->DebugActive = KeGetCurrentThread()->DebugActive =
                                        (UCHAR)(TrapFrame->Dr7 & DR7_ACTIVE);
        }
    }

    return;
}

VOID
PspGetSetContextApc (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function either captures the user mode state of the current
    thread, or sets the user mode state of the current thread. The
    operation type is determined by the value of SystemArgument1. A
    zero value is used for get context, and a nonzero value is used
    for set context.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to the normal routine function that
        was specified when the APC was initialized. This parameter is not
        used.

    NormalContext - Supplies a pointer to an arbitrary data structure that
        was specified when the APC was initialized. This parameter is not
        used.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
        arguments that contain untyped data. These parameters are not used.

Return Value:

    None.

--*/

{

    PGETSETCONTEXT ContextBlock;
    KNONVOLATILE_CONTEXT_POINTERS ContextPointers;
    CONTEXT ContextRecord;
    ULONG ControlPc;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    PETHREAD Thread;
    ULONG TrapFrame1;
    ULONG TrapFrame2;

    //
    // Get the address of the context block and compute the address of the
    // system entry trap frame.
    //

    ContextBlock = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);
    Thread = PsGetCurrentThread();
    TrapFrame1 = (ULONG)Thread->Tcb.InitialStack - (KTRAP_FRAME_LENGTH +
                 sizeof(KEXCEPTION_FRAME) + (2 * sizeof(ULONG)));
    TrapFrame2 = (ULONG)Thread->Tcb.InitialStack - (KTRAP_FRAME_LENGTH +
                 sizeof(KEXCEPTION_FRAME) + STK_MIN_FRAME +
                 (10 * sizeof(ULONG)));

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = ContextRecord.Lr;

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.IntegerContext[13] = &ContextRecord.Gpr13;
    ContextPointers.IntegerContext[14] = &ContextRecord.Gpr14;
    ContextPointers.IntegerContext[15] = &ContextRecord.Gpr15;
    ContextPointers.IntegerContext[16] = &ContextRecord.Gpr16;
    ContextPointers.IntegerContext[17] = &ContextRecord.Gpr17;
    ContextPointers.IntegerContext[18] = &ContextRecord.Gpr18;
    ContextPointers.IntegerContext[19] = &ContextRecord.Gpr19;
    ContextPointers.IntegerContext[20] = &ContextRecord.Gpr20;
    ContextPointers.IntegerContext[21] = &ContextRecord.Gpr21;
    ContextPointers.IntegerContext[22] = &ContextRecord.Gpr22;
    ContextPointers.IntegerContext[23] = &ContextRecord.Gpr23;
    ContextPointers.IntegerContext[24] = &ContextRecord.Gpr24;
    ContextPointers.IntegerContext[25] = &ContextRecord.Gpr25;
    ContextPointers.IntegerContext[26] = &ContextRecord.Gpr26;
    ContextPointers.IntegerContext[27] = &ContextRecord.Gpr27;
    ContextPointers.IntegerContext[28] = &ContextRecord.Gpr28;
    ContextPointers.IntegerContext[29] = &ContextRecord.Gpr29;
    ContextPointers.IntegerContext[30] = &ContextRecord.Gpr30;
    ContextPointers.IntegerContext[31] = &ContextRecord.Gpr31;

    ContextPointers.FloatingContext[14] = &ContextRecord.Fpr14;
    ContextPointers.FloatingContext[15] = &ContextRecord.Fpr15;
    ContextPointers.FloatingContext[16] = &ContextRecord.Fpr16;
    ContextPointers.FloatingContext[17] = &ContextRecord.Fpr17;
    ContextPointers.FloatingContext[18] = &ContextRecord.Fpr18;
    ContextPointers.FloatingContext[19] = &ContextRecord.Fpr19;
    ContextPointers.FloatingContext[20] = &ContextRecord.Fpr20;
    ContextPointers.FloatingContext[21] = &ContextRecord.Fpr21;
    ContextPointers.FloatingContext[22] = &ContextRecord.Fpr22;
    ContextPointers.FloatingContext[23] = &ContextRecord.Fpr23;
    ContextPointers.FloatingContext[24] = &ContextRecord.Fpr24;
    ContextPointers.FloatingContext[25] = &ContextRecord.Fpr25;
    ContextPointers.FloatingContext[26] = &ContextRecord.Fpr26;
    ContextPointers.FloatingContext[27] = &ContextRecord.Fpr27;
    ContextPointers.FloatingContext[28] = &ContextRecord.Fpr28;
    ContextPointers.FloatingContext[29] = &ContextRecord.Fpr29;
    ContextPointers.FloatingContext[30] = &ContextRecord.Fpr30;
    ContextPointers.FloatingContext[31] = &ContextRecord.Fpr31;

    //
    // Start with the frame specified by the context record and virtually
    // unwind call frames until the system entry trap frame is encountered.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // Virtually unwind to the caller of the current routine to
        // obtain the address where control left the caller.
        //

        ControlPc = RtlVirtualUnwind(ControlPc,
                                     FunctionEntry,
                                     &ContextRecord,
                                     &InFunction,
                                     &EstablisherFrame,
                                     &ContextPointers,
                                     (ULONG)Thread->Tcb.StackLimit,
                                     (ULONG)Thread->Tcb.InitialStack);

    } while ((ContextRecord.Gpr1 >= (ULONG)Thread->Tcb.StackLimit) &&
             (ContextRecord.Gpr1 < (ULONG)Thread->Tcb.InitialStack));

    //
    // If system argument one is nonzero, then set the context of the current
    // thread. Otherwise, get the context of the current thread.
    //

    if (Apc->SystemArgument1 != 0) {

        //
        // Set context of current thread.
        //

        PspSetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context,
                      ContextBlock->Mode);

    } else {

        //
        // Get context of current thread.
        //

        PspGetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context);
    }

    KeSetEvent(&ContextBlock->OperationComplete, 0, FALSE);
    return;
}
