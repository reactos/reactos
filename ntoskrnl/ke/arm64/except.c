/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 exception dispatching
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* What the user mode dispatcher finds at its stack pointer */
typedef struct _KUSER_EXCEPTION_STACK
{
    CONTEXT Context;
    EXCEPTION_RECORD ExceptionRecord;
} KUSER_EXCEPTION_STACK, *PKUSER_EXCEPTION_STACK;

/* Implemented in ke/arm64/trap.s */
VOID
KiArm64ExceptionVectors(VOID);

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeInitExceptions(VOID)
{
    KeArm64WriteRegister(ARM64_VBAR_EL1, (ULONG_PTR)KiArm64ExceptionVectors);
    __isb(_ARM64_BARRIER_SY);
}

static
BOOLEAN
KiDispatchExceptionToUser(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _In_ PCONTEXT Context,
    _In_ PEXCEPTION_RECORD ExceptionRecord)
{
    PKUSER_EXCEPTION_STACK UserStack;
    ULONG_PTR UserSp;

    UserSp = (Context->Sp - sizeof(*UserStack)) & ~(ULONG_PTR)15;
    UserStack = (PKUSER_EXCEPTION_STACK)UserSp;

    _enable();

    _SEH2_TRY
    {
        ProbeForWrite(UserStack, sizeof(*UserStack), TYPE_ALIGNMENT(KUSER_EXCEPTION_STACK));
        UserStack->Context = *Context;
        UserStack->ExceptionRecord = *ExceptionRecord;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _disable();
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    /* KiUserExceptionDispatcher(ExceptionRecord, Context) */
    TrapFrame->X0 = (ULONG64)&UserStack->ExceptionRecord;
    TrapFrame->X1 = (ULONG64)&UserStack->Context;
    TrapFrame->Sp = UserSp;
    TrapFrame->Lr = 0;
    TrapFrame->Pc = (ULONG64)KeUserExceptionDispatcher;
    TrapFrame->Spsr = ARM64_PSR_MODE_EL0T;

    _disable();
    return TRUE;
}

VOID
NTAPI
KiDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Inout_ PKTRAP_FRAME TrapFrame,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ BOOLEAN FirstChance)
{
    CONTEXT Context;

    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    /* Nothing from the kernel stack may leak to user mode */
    RtlZeroMemory(&Context, sizeof(Context));
    Context.ContextFlags = CONTEXT_ALL;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    if (ExceptionRecord->ExceptionCode == KI_EXCEPTION_ACCESS_VIOLATION)
        ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;

    if (PreviousMode == KernelMode)
    {
        if (FirstChance)
        {
            if (KiDebugRoutine(TrapFrame, ExceptionFrame, ExceptionRecord, &Context, PreviousMode, FALSE))
                goto Handled;

            if (RtlDispatchException(ExceptionRecord, &Context))
                goto Handled;
        }

        if (KiDebugRoutine(TrapFrame, ExceptionFrame, ExceptionRecord, &Context, PreviousMode, TRUE))
            goto Handled;

        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }

    if (FirstChance)
    {
        /* Debug services always go to the kernel debugger */
        if ((!PsGetCurrentProcess()->DebugPort && !KdIgnoreUmExceptions) ||
            KdIsThisAKdTrap(ExceptionRecord, &Context, PreviousMode))
        {
            if (KiDebugRoutine(TrapFrame, ExceptionFrame, ExceptionRecord, &Context, PreviousMode, FALSE))
                goto Handled;
        }

        if (DbgkForwardException(ExceptionRecord, TRUE, FALSE))
            return;

        if (KiDispatchExceptionToUser(TrapFrame, &Context, ExceptionRecord))
            return;
    }

    if (DbgkForwardException(ExceptionRecord, TRUE, TRUE))
        return;

    if (DbgkForwardException(ExceptionRecord, FALSE, TRUE))
        return;

    DPRINT1("Kill %.16s, ExceptionCode: %lx, ExceptionAddress: %p, BaseAddress: %p\n",
            PsGetCurrentProcess()->ImageFileName,
            ExceptionRecord->ExceptionCode,
            ExceptionRecord->ExceptionAddress,
            PsGetCurrentProcess()->SectionBaseAddress);

    ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
    KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                 ExceptionRecord->ExceptionCode,
                 (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                 (ULONG_PTR)TrapFrame,
                 0);

Handled:
    KeContextToTrapFrame(&Context, ExceptionFrame, TrapFrame, Context.ContextFlags, PreviousMode);
}

/**
 * @brief Makes the current system call return into KiRaiseUserExceptionDispatcher.
 */
NTSTATUS
NTAPI
KeRaiseUserException(
    _In_ NTSTATUS ExceptionCode)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME TrapFrame = Thread->TrapFrame;
    PTEB Teb = Thread->Teb;

    if (!TrapFrame || !KiUserTrap(TrapFrame))
        return ExceptionCode;

    _SEH2_TRY
    {
        Teb->ExceptionCode = ExceptionCode;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return ExceptionCode);
    }
    _SEH2_END;

    /* The dispatcher returns to the code that made the call */
    TrapFrame->Lr = TrapFrame->Pc;
    TrapFrame->Pc = (ULONG64)KeRaiseUserExceptionDispatcher;
    return ExceptionCode;
}
