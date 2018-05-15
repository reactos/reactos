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

VOID
KiSetTrapContext(
    _Out_ PKTRAP_FRAME TrapFrame,
    _In_ PCONTEXT Context,
    _In_ KPROCESSOR_MODE RequestorMode);

/* FUNCTIONS ******************************************************************/


_IRQL_requires_(APC_LEVEL)
VOID
NTAPI
PspGetOrSetContextKernelRoutine(
    _In_ PKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ PVOID* NormalContext,
    _Inout_ PVOID* SystemArgument1,
    _Inout_ PVOID* SystemArgument2)
{
    PGET_SET_CTX_CONTEXT GetSetContext;
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame = NULL;

    PAGED_CODE();

    /* Get the Context Structure */
    GetSetContext = CONTAINING_RECORD(Apc, GET_SET_CTX_CONTEXT, Apc);
    Thread = Apc->SystemArgument2;
    NT_ASSERT(KeGetCurrentThread() == Thread);
    
    /* If this is a kernel-mode request, grab the saved trap frame */
    if (GetSetContext->Mode == KernelMode)
    {
        TrapFrame = Thread->TrapFrame;
    }
    
    /* If we don't have one, grab it from the stack */
    if (TrapFrame == NULL)
    {
        /* Get the thread's base trap frame */
        TrapFrame = KeGetTrapFrame(KeGetCurrentThread());
    }

    /* Check if it's a set or get */
    if (Apc->SystemArgument1 != 0)
    {
        /* Set the nonvolatiles on the stack, target frame is the trap frame */
        KiSetTrapContext(TrapFrame, &GetSetContext->Context, GetSetContext->Mode);
    }
    else
    {
        /* Convert the trap frame to a context */
        KeTrapFrameToContext(TrapFrame,
                             NULL,
                             &GetSetContext->Context);
    }

    /* Notify the Native API that we are done */
    KeSetEvent(&GetSetContext->Event, IO_NO_INCREMENT, FALSE);
}

/* EOF */
