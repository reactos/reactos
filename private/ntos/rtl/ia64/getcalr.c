/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    getcalr.c

Abstract:

    This module implements the routine RtlGetCallerAddress. It will
    return the address of the caller, and the callers caller to the
    specified procedure.

Author:

    William K. Cheung (wcheung) 17-Jan-1996

    based on version by Larry Osterman (larryo) 18-Mar-1991

Revision History:

--*/
#include "ntrtlp.h"

//
// Undefine get callers address since it is defined as a macro.
//

#undef RtlGetCallersAddress

VOID
RtlGetCallersAddress (
    OUT PVOID *CallersPc,
    OUT PVOID *CallersCallersPc
    )

/*++

Routine Description:

    This routine returns the address of the routine that called the routine
    that called this routine, and the routine that called the routine that
    called this routine. For example, if A called B called C which called
    this routine, the return addresses in A and B would be returned.

Arguments:

    CallersPc - Supplies a pointer to a variable that receives the address
        of the caller of the caller of this routine (B).

    CallersCallersPc - Supplies a pointer to a variable that receives the
        address of the caller of the caller of the caller of this routine
        (A).

Return Value:

    None.

Note:

    If either of the calling stack frames exceeds the limits of the stack,
    they are set to NULL.

--*/

{
#ifdef REALLY_GET_CALLERS_CALLER
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc;
    ULONGLONG HighStackLimit, LowStackLimit;
    ULONGLONG HighBStoreLimit, LowBStoreLimit;
    ULONGLONG ImageBase;
    ULONGLONG TargetGp;

    //
    // Assume the function table entries for the various routines cannot be
    // found or there are not four procedure activation records on the stack.
    //

    *CallersPc = NULL;
    *CallersCallersPc = NULL;

    //
    // Capture the current context.
    //

    RtlCaptureContext(&ContextRecord);
    NextPc = (ULONG_PTR)ContextRecord.BrRp;

    //
    // Get the high and low limits of the current thread's stack.
    //

    Rtlp64GetStackLimits(&LowStackLimit, &HighStackLimit);
    Rtlp64GetBStoreLimits(&LowBStoreLimit, &HighBStoreLimit);

    //
    // Attempt to unwind to the caller of this routine (C).
    //

    FunctionEntry = RtlLookupFunctionEntry(NextPc, &ImageBase, &TargetGp);
    if (FunctionEntry != NULL) {

        //
        // A function entry was found for this routine. Virtually unwind
        // to the caller of this routine (C).
        //

        NextPc = RtlVirtualUnwind(NextPc,
                                  FunctionEntry,
                                  &ContextRecord,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL);

        //
        // Attempt to unwind to the caller of the caller of this routine (B).
        //

        FunctionEntry = RtlLookupFunctionEntry(NextPc);
        if ((FunctionEntry != NULL) && (((ULONG_PTR)ContextRecord.IntSp < HighStackLimit) && ((ULONG_PTR)ContextRecord.RsBSP > LowBStoreLimit))) {

            //
            // A function table entry was found for the caller of the caller
            // of this routine (B). Virtually unwind to the caller of the
            // caller of this routine (B).
            //

            NextPc = RtlVirtualUnwind(NextPc,
                                      FunctionEntry,
                                      &ContextRecord,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL);

            *CallersPc = (PVOID)NextPc;

            //
            // Attempt to unwind to the caller of the caller of the caller
            // of the caller of this routine (A).
            //

            FunctionEntry = RtlLookupFunctionEntry(NextPc);
            if ((FunctionEntry != NULL) && (((ULONG_PTR)ContextRecord.IntSp < HighStackLimit) && ((ULONG_PTR)ContextRecord.RsBSP > LowBStoreLimit))) {

                //
                // A function table entry was found for the caller of the
                // caller of the caller of this routine (A). Virtually unwind
                // to the caller of the caller of the caller of this routine
                // (A).
                //

                NextPc = RtlVirtualUnwind(NextPc,
                                          FunctionEntry,
                                          &ContextRecord,
                                          &InFunction,
                                          &EstablisherFrame,
                                          NULL);

                *CallersCallersPc = (PVOID)NextPc;
            }
        }
    }
#else
    *CallersPc = NULL;
    *CallersCallersPc = NULL;
#endif // REALLY_GET_CALLERS_CALLER
    return;
}

USHORT
RtlCaptureStackBackTrace(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   )
{
    return 0;
}
