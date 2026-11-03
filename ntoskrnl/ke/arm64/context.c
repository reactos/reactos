/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 context and trap frame conversion
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* Only the condition flags of CPSR are under the caller's control */
#define CPSR_FLAGS_MASK     0xF0000000

/* FUNCTIONS *****************************************************************/

/**
 * @brief Writes a context into the frames of a trap.
 *
 * X0 to X18, FP, LR, SP and PC live in the trap frame, X19 to X28 in the
 * exception frame and the vector state behind the trap frame's VfpState.
 * In kernel mode X18 holds the PCR and is never taken from the context.
 */
VOID
NTAPI
KeContextToTrapFrame(
    _In_ PCONTEXT Context,
    _Inout_opt_ PKEXCEPTION_FRAME ExceptionFrame,
    _Inout_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG ContextFlags,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PKARM64_VFP_STATE VfpState;
    KIRQL OldIrql;
    ULONG i;

    /* The frames belong to a thread that may run, keep APCs out */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL)
        KeRaiseIrql(APC_LEVEL, &OldIrql);

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        TrapFrame->Fp = Context->Fp;
        TrapFrame->Lr = Context->Lr;
        TrapFrame->Sp = Context->Sp;
        TrapFrame->Pc = Context->Pc;

        if (PreviousMode == UserMode)
        {
            TrapFrame->Spsr = (Context->Cpsr & CPSR_FLAGS_MASK) | ARM64_PSR_MODE_EL0T;
        }
        else
        {
            TrapFrame->Spsr = (TrapFrame->Spsr & ~CPSR_FLAGS_MASK) | (Context->Cpsr & CPSR_FLAGS_MASK);
        }
    }

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        for (i = 0; i < 18; i++)
            TrapFrame->X[i] = Context->X[i];

        if (ExceptionFrame)
        {
            ExceptionFrame->X19 = Context->X19;
            ExceptionFrame->X20 = Context->X20;
            ExceptionFrame->X21 = Context->X21;
            ExceptionFrame->X22 = Context->X22;
            ExceptionFrame->X23 = Context->X23;
            ExceptionFrame->X24 = Context->X24;
            ExceptionFrame->X25 = Context->X25;
            ExceptionFrame->X26 = Context->X26;
            ExceptionFrame->X27 = Context->X27;
            ExceptionFrame->X28 = Context->X28;
        }
    }

    /* The user TEB register only changes when asked for explicitly */
    if ((PreviousMode == UserMode) && ((ContextFlags & CONTEXT_X18) == CONTEXT_X18))
        TrapFrame->X18 = Context->X18;

    VfpState = TrapFrame->VfpState;
    if (((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) && VfpState)
    {
        VfpState->Fpcr = Context->Fpcr;
        VfpState->Fpsr = Context->Fpsr;
        for (i = 0; i < RTL_NUMBER_OF(VfpState->V); i++)
        {
            VfpState->V[i].Low = Context->V[i].Low;
            VfpState->V[i].High = Context->V[i].High;
        }
    }

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
    {
        RtlCopyMemory(TrapFrame->Bcr, Context->Bcr, sizeof(TrapFrame->Bcr));
        RtlCopyMemory(TrapFrame->Bvr, Context->Bvr, sizeof(TrapFrame->Bvr));
        RtlCopyMemory(TrapFrame->Wcr, Context->Wcr, sizeof(TrapFrame->Wcr));
        RtlCopyMemory(TrapFrame->Wvr, Context->Wvr, sizeof(TrapFrame->Wvr));
        TrapFrame->DebugRegistersValid = TRUE;
    }

    if (OldIrql < APC_LEVEL)
        KeLowerIrql(OldIrql);
}

VOID
NTAPI
KeTrapFrameToContext(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_opt_ PKEXCEPTION_FRAME ExceptionFrame,
    _Inout_ PCONTEXT Context)
{
    PKARM64_VFP_STATE VfpState;
    KIRQL OldIrql;
    ULONG i;

    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL)
        KeRaiseIrql(APC_LEVEL, &OldIrql);

    if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        Context->Fp = TrapFrame->Fp;
        Context->Lr = TrapFrame->Lr;
        Context->Sp = TrapFrame->Sp;
        Context->Pc = TrapFrame->Pc;
        Context->Cpsr = TrapFrame->Spsr;
    }

    if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        for (i = 0; i < 19; i++)
            Context->X[i] = TrapFrame->X[i];

        if (ExceptionFrame)
        {
            Context->X19 = ExceptionFrame->X19;
            Context->X20 = ExceptionFrame->X20;
            Context->X21 = ExceptionFrame->X21;
            Context->X22 = ExceptionFrame->X22;
            Context->X23 = ExceptionFrame->X23;
            Context->X24 = ExceptionFrame->X24;
            Context->X25 = ExceptionFrame->X25;
            Context->X26 = ExceptionFrame->X26;
            Context->X27 = ExceptionFrame->X27;
            Context->X28 = ExceptionFrame->X28;
        }
    }

    VfpState = TrapFrame->VfpState;
    if (((Context->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) && VfpState)
    {
        Context->Fpcr = VfpState->Fpcr;
        Context->Fpsr = VfpState->Fpsr;
        for (i = 0; i < RTL_NUMBER_OF(VfpState->V); i++)
        {
            Context->V[i].Low = VfpState->V[i].Low;
            Context->V[i].High = VfpState->V[i].High;
        }
    }

    if ((Context->ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
    {
        if (TrapFrame->DebugRegistersValid)
        {
            RtlCopyMemory(Context->Bcr, TrapFrame->Bcr, sizeof(Context->Bcr));
            RtlCopyMemory(Context->Bvr, TrapFrame->Bvr, sizeof(Context->Bvr));
            RtlCopyMemory(Context->Wcr, TrapFrame->Wcr, sizeof(Context->Wcr));
            RtlCopyMemory(Context->Wvr, TrapFrame->Wvr, sizeof(Context->Wvr));
        }
        else
        {
            RtlZeroMemory(Context->Bcr, sizeof(Context->Bcr));
            RtlZeroMemory(Context->Bvr, sizeof(Context->Bvr));
            RtlZeroMemory(Context->Wcr, sizeof(Context->Wcr));
            RtlZeroMemory(Context->Wvr, sizeof(Context->Wvr));
        }
    }

    if (OldIrql < APC_LEVEL)
        KeLowerIrql(OldIrql);
}

/* Every trap keeps its exception frame right below the floating point state */
static
PKEXCEPTION_FRAME
KiGetTrapExceptionFrame(
    _In_ PKTRAP_FRAME TrapFrame)
{
    return (PKEXCEPTION_FRAME)((ULONG_PTR)TrapFrame -
                               sizeof(KARM64_VFP_STATE) -
                               sizeof(KEXCEPTION_FRAME));
}

VOID
KiGetTrapContext(
    _In_ PKTRAP_FRAME TrapFrame,
    _Out_ PCONTEXT Context)
{
    KeTrapFrameToContext(TrapFrame, KiGetTrapExceptionFrame(TrapFrame), Context);
}

VOID
KiSetTrapContext(
    _Out_ PKTRAP_FRAME TrapFrame,
    _In_ PCONTEXT Context,
    _In_ KPROCESSOR_MODE RequestorMode)
{
    KeContextToTrapFrame(Context,
                         KiGetTrapExceptionFrame(TrapFrame),
                         TrapFrame,
                         Context->ContextFlags,
                         RequestorMode);
}
