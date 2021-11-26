/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/exp.c
 * PURPOSE:         Implements exception helper routines for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
    KIRQL OldIrql;

    //
    // Do this at APC_LEVEL
    //
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    //
    // Start with the Control flags
    //
    if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        //
        // So this basically means all the special stuff
        //

        //
        // ARM has register banks
        //
        TrapFrame->Sp = Context->Sp;
        TrapFrame->Lr = Context->Lr;

        //
        // The rest is already in the right mode
        //
        TrapFrame->Pc = Context->Pc;
        TrapFrame->Cpsr = Context->Cpsr;
    }

    //
    // Now do the integers
    //
    if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        //
        // Basically everything else but FPU
        //
        TrapFrame->R0 = Context->R0;
        TrapFrame->R1 = Context->R1;
        TrapFrame->R2 = Context->R2;
        TrapFrame->R3 = Context->R3;
        ExceptionFrame->R4 = Context->R4;
        ExceptionFrame->R5 = Context->R5;
        ExceptionFrame->R6 = Context->R6;
        ExceptionFrame->R7 = Context->R7;
        ExceptionFrame->R8 = Context->R8;
        ExceptionFrame->R9 = Context->R9;
        ExceptionFrame->R10 = Context->R10;
        ExceptionFrame->R11 = Context->R11;
        TrapFrame->R12 = Context->R12;
    }

    //
    // Restore IRQL
    //
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
KeTrapFrameToContext(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PCONTEXT Context)
{
    KIRQL OldIrql;

    //
    // Do this at APC_LEVEL
    //
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    //
    // Start with the Control flags
    //
    if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        //
        // So this basically means all the special stuff
        //

        //
        // ARM has register banks
        //
        Context->Sp = TrapFrame->Sp;
        Context->Lr = TrapFrame->Lr;

        //
        // The rest is already in the right mode
        //
        Context->Pc = TrapFrame->Pc;
        Context->Cpsr = TrapFrame->Cpsr;
    }

    //
    // Now do the integers
    //
    if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        //
        // Basically everything else but FPU
        //
        Context->R0 = TrapFrame->R0;
        Context->R1 = TrapFrame->R1;
        Context->R2 = TrapFrame->R2;
        Context->R3 = TrapFrame->R3;
        Context->R4 = ExceptionFrame->R4;
        Context->R5 = ExceptionFrame->R5;
        Context->R6 = ExceptionFrame->R6;
        Context->R7 = ExceptionFrame->R7;
        Context->R8 = ExceptionFrame->R8;
        Context->R9 = ExceptionFrame->R9;
        Context->R10 = ExceptionFrame->R10;
        Context->R11 = ExceptionFrame->R11;
        Context->R12 = TrapFrame->R12;
    }

    //
    // Restore IRQL
    //
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
    CONTEXT Context;

    /* Increase number of Exception Dispatches */
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    /* Set the context flags */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if User Mode or if the kernel debugger is enabled */
    if ((PreviousMode == UserMode) || (KeGetPcr()->KdVersionBlock))
    {
        /* FIXME-V6: VFP Support */
    }

    /* Get a Context */
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Look at our exception code */
    switch (ExceptionRecord->ExceptionCode)
    {
        /* Breakpoint */
        case STATUS_BREAKPOINT:

            /* Decrement PC by four */
            Context.Pc -= sizeof(ULONG);
            break;

        /* Internal exception */
        case KI_EXCEPTION_ACCESS_VIOLATION:

            /* Set correct code */
            ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
            if (PreviousMode == UserMode)
            {
                /* FIXME: Handle no execute */
            }
            break;
    }

    /* Handle kernel-mode first, it's simpler */
    if (PreviousMode == KernelMode)
    {
        /* Check if this is a first-chance exception */
        if (FirstChance != FALSE)
        {
            /* Break into the debugger for the first time */
            if (KiDebugRoutine(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &Context,
                               PreviousMode,
                               FALSE))
            {
                /* Exception was handled */
                goto Handled;
            }

            /* If the Debugger couldn't handle it, dispatch the exception */
            if (RtlDispatchException(ExceptionRecord, &Context)) goto Handled;
        }

        /* This is a second-chance exception, only for the debugger */
        if (KiDebugRoutine(TrapFrame,
                           ExceptionFrame,
                           ExceptionRecord,
                           &Context,
                           PreviousMode,
                           TRUE))
        {
            /* Exception was handled */
            goto Handled;
        }

        /* Third strike; you're out */
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }
    else
    {
        /* FIXME: TODO */
        /* 3rd strike, kill the process */
        DPRINT1("Kill %.16s, ExceptionCode: %lx, ExceptionAddress: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                ExceptionRecord->ExceptionCode,
                ExceptionRecord->ExceptionAddress);

        ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }

Handled:
    /* Convert the context back into Trap/Exception Frames */
    KeContextToTrapFrame(&Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
    return;
}

NTSTATUS
NTAPI
KeRaiseUserException(
    _In_ NTSTATUS ExceptionCode)
{
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
