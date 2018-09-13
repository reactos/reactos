/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psctx.c

Abstract:

    This procedure implements Get/Set Context Thread

Author:

    Mark Lucovsky (markl) 25-May-1989

Notes:

    There IS NO NonVolatileContext stored outside of the trap
    frame on a 386, with the exception of floating point.  Hence,
    the NonVolatileContextPointers argument to Get/SetContext is
    always NULL on the 386.

Revision History:

     8-Jan-90   bryanwi

        Port to 386

--*/

#include "psp.h"

#define PSPALIGN_DOWN(address,amt) ((ULONG)(address) & ~(( amt ) - 1))

#define PSPALIGN_UP(address,amt) (PSPALIGN_DOWN( (address + (amt) - 1), (amt) ))

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PspGetContext )
#pragma alloc_text(PAGE,PspGetSetContextSpecialApc )
#pragma alloc_text(PAGE,PspSetContext)
#endif


VOID
PspGetContext(
    IN PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS NonVolatileContext,
    IN OUT PCONTEXT Context
    )

/*++

Routine Description:

    This function moves the contents of the specified trap and NonVolatile
    context into the specified context record. It's primary user will
    be NtGetContextThread.

    N.B. - NonVolatileContext is IGNORED on the 386.

Arguments:

    TrapFrame - Supplies the contents of a trap frame that should be
                restored copied into the proper location in the context
                record.

    Context - Returns the threads current context.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( NonVolatileContext );

    PAGED_CODE();

    ASSERT(((TrapFrame->SegCs & MODE_MASK) != KernelMode) ||
           (TrapFrame->EFlags & EFLAGS_V86_MASK));

    KeContextFromKframes(TrapFrame, NULL, Context);
}

VOID
PspSetContext(
    OUT PKTRAP_FRAME TrapFrame,
    OUT PKNONVOLATILE_CONTEXT_POINTERS NonVolatileContext,
    IN PCONTEXT Context,
    KPROCESSOR_MODE Mode
    )

/*++

Routine Description:

    This function moves the contents of the specified context record
    into the specified trap frame, and modifies the thread's non volatile
    context by storing through the thread's nonvolatile context pointers.

    N.B. - NonVolatileContext is IGNORED on the 386.

Arguments:

    TrapFrame - Returns selected pieces of the context record.

    Context - Supplies a context record to be copied in the trap and
              nonvolatile context.

    Mode - Supplies the mode to be used when sanitizing the psr, epsr and fsr

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( NonVolatileContext );
    ASSERT(((TrapFrame->SegCs & MODE_MASK) != KernelMode) ||
           (TrapFrame->EFlags & EFLAGS_V86_MASK));

    PAGED_CODE();

    KeContextToKframes(TrapFrame, NULL, Context, Context->ContextFlags, Mode);
}

VOID
PspGetSetContextSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function either captures the usermode state of the current
    thread, or sets the usermode state of the current thread. The
    operation type is determined by the value of SystemArgument1. A
    NULL value is used for get context, and a non-NULL value is used
    for set context.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to a pointer to the normal routine
        function that was specifed when the APC was initialized.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
        arguments that contain untyped data.

Return Value:

    None.

--*/

{
    PGETSETCONTEXT Ctx;
    PKTRAP_FRAME TrapFrame;
    PETHREAD Thread;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( NormalRoutine );
    UNREFERENCED_PARAMETER( NormalContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    Ctx = CONTAINING_RECORD(Apc,GETSETCONTEXT,Apc);
    Thread = Apc->SystemArgument2;

    TrapFrame = (PKTRAP_FRAME)((PUCHAR)Thread->Tcb.InitialStack -
                PSPALIGN_UP(sizeof(KTRAP_FRAME),KTRAP_FRAME_ALIGN) -
                sizeof(FX_SAVE_AREA));

    if ( Apc->SystemArgument1 ) {

        //
        // Set Context
        //

        PspSetContext(TrapFrame,NULL,&Ctx->Context,Ctx->Mode);

    } else {

        //
        // Get Context
        //

        PspGetContext(TrapFrame,NULL,&Ctx->Context);
    }

    KeSetEvent(&Ctx->OperationComplete,0,FALSE);

}
