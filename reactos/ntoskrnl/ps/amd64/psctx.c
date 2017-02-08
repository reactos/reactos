/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/amd64/psctx.c
 * PURPOSE:         Process Manager: Set/Get Context for i386
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PspGetContext(IN PKTRAP_FRAME TrapFrame,
              IN PVOID NonVolatileContext,
              IN OUT PCONTEXT Context)
{   
    PAGED_CODE();
 
    /* Convert the trap frame to a context */
    KeTrapFrameToContext(TrapFrame, NULL, Context);
}

VOID
NTAPI
PspSetContext(OUT PKTRAP_FRAME TrapFrame,
              OUT PVOID NonVolatileContext,
              IN PCONTEXT Context,
              IN KPROCESSOR_MODE Mode)
{   
    PAGED_CODE();
    
    /* Convert the context to a trap frame structure */
    KeContextToTrapFrame(Context, NULL, TrapFrame, Context->ContextFlags, Mode);
}

VOID
NTAPI
PspGetOrSetContextKernelRoutine(IN PKAPC Apc,
                                IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                                IN OUT PVOID* NormalContext,
                                IN OUT PVOID* SystemArgument1,
                                IN OUT PVOID* SystemArgument2)
{
    PGET_SET_CTX_CONTEXT GetSetContext;
    PKEVENT Event;
    PCONTEXT Context;
    PKTHREAD Thread;
    KPROCESSOR_MODE Mode;
    PKTRAP_FRAME TrapFrame = NULL;
    PAGED_CODE();

    /* Get the Context Structure */
    GetSetContext = CONTAINING_RECORD(Apc, GET_SET_CTX_CONTEXT, Apc);
    Context = &GetSetContext->Context;
    Event = &GetSetContext->Event;
    Mode = GetSetContext->Mode;
    Thread = Apc->SystemArgument2;
    
    /* If this is a kernel-mode request, grab the saved trap frame */
    if (Mode == KernelMode) TrapFrame = Thread->TrapFrame;
    
    /* If we don't have one, grab it from the stack */
    if (!TrapFrame)
    {
        DPRINT1("FIXME!!!!\n");
        /* Trap frame is right under our initial stack */
//        TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)Thread->InitialStack -
//                                   ROUND_UP(sizeof(KTRAP_FRAME), KTRAP_FRAME_ALIGN) -
//                                   sizeof(FX_SAVE_AREA));
    }

    /* Check if it's a set or get */
    if (Apc->SystemArgument1)
    {
        /* Get the Context */
        PspSetContext(TrapFrame, NULL, Context, Mode);
    }
    else
    {
        /* Set the Context */
        PspGetContext(TrapFrame, NULL, Context);
    }

    /* Notify the Native API that we are done */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

/* EOF */
