/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 user mode APCs and callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* What KiUserApcDispatcher in ntdll finds at its stack pointer */
typedef struct _KUSER_APC_FRAME
{
    ULONG64 NormalRoutine;
    ULONG64 NormalContext;
    ULONG64 SystemArgument1;
    ULONG64 SystemArgument2;
    ULONG64 TestAlert;
    ULONG64 Reserved;
    CONTEXT Context;
} KUSER_APC_FRAME, *PKUSER_APC_FRAME;

C_ASSERT(FIELD_OFFSET(KUSER_APC_FRAME, Context) == 0x30);

/* KiCallUserMode keeps the nonvolatile registers here, see usercall.s */
typedef struct _KI_CALLOUT_STATE
{
    KCALLOUT_FRAME Frame;
    ULONG64 InitialStack;
    ULONG64 CallbackStack;
} KI_CALLOUT_STATE, *PKI_CALLOUT_STATE;

C_ASSERT(sizeof(KI_CALLOUT_STATE) == sizeof(KEXCEPTION_FRAME) + 0x10);

/* The frames a trap from user mode leaves below the initial stack */
typedef struct _KI_USER_TRAP_STACK
{
    KEXCEPTION_FRAME ExceptionFrame;
    KARM64_VFP_STATE VfpState;
    KTRAP_FRAME TrapFrame;
} KI_USER_TRAP_STACK, *PKI_USER_TRAP_STACK;

DECLSPEC_NORETURN
VOID
KiArm64CallbackReturn(
    _In_ PKI_CALLOUT_STATE Callout,
    _In_ NTSTATUS Status);

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitializeUserApc(
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Inout_ PKTRAP_FRAME TrapFrame,
    _In_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PKUSER_APC_FRAME ApcFrame;
    EXCEPTION_RECORD ExceptionRecord;

    ASSERT(KiUserTrap(TrapFrame));

    ApcFrame = (PKUSER_APC_FRAME)ALIGN_DOWN_BY(TrapFrame->Sp - sizeof(*ApcFrame), 16);

    _SEH2_TRY
    {
        ProbeForWrite(ApcFrame, sizeof(*ApcFrame), 16);

        /* The dispatcher continues with this context once the routine returns */
        ApcFrame->Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
        KeTrapFrameToContext(TrapFrame, ExceptionFrame, &ApcFrame->Context);

        ApcFrame->NormalRoutine = (ULONG64)NormalRoutine;
        ApcFrame->NormalContext = (ULONG64)NormalContext;
        ApcFrame->SystemArgument1 = (ULONG64)SystemArgument1;
        ApcFrame->SystemArgument2 = (ULONG64)SystemArgument2;
        ApcFrame->TestAlert = TRUE;
        ApcFrame->Reserved = 0;
    }
    _SEH2_EXCEPT(ExceptionRecord = *_SEH2_GetExceptionInformation()->ExceptionRecord, EXCEPTION_EXECUTE_HANDLER)
    {
        ExceptionRecord.ExceptionAddress = (PVOID)TrapFrame->Pc;
        KiDispatchException(&ExceptionRecord, ExceptionFrame, TrapFrame, UserMode, TRUE);
        _SEH2_YIELD(return);
    }
    _SEH2_END;

    TrapFrame->Sp = (ULONG64)ApcFrame;
    TrapFrame->Lr = 0;
    TrapFrame->Pc = (ULONG64)KeUserApcDispatcher;
    TrapFrame->Spsr = ARM64_PSR_MODE_EL0T;
}

/**
 * @brief Enters KiUserCallbackDispatcher, called by KiCallUserMode.
 *
 * The callback trap frames are built in the space KiCallUserMode left right
 * below the callout state, which becomes the initial stack of the callback.
 * Only returns when the stack cannot be grown.
 */
NTSTATUS
NTAPI
KiArm64UserModeCallout(
    _Inout_ PKI_CALLOUT_STATE Callout)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME TrapFrame = Thread->TrapFrame;
    PKI_USER_TRAP_STACK Stack;
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT((Thread->ApcStateIndex == OriginalApcEnvironment) &&
           (Thread->CombinedApcDisable == 0));

    /* The callback gets a full kernel stack of its own */
    if (((ULONG_PTR)Callout - KERNEL_STACK_SIZE) < (ULONG_PTR)Thread->StackLimit)
    {
        Status = MmGrowKernelStack(Callout);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Callout->InitialStack = (ULONG64)Thread->InitialStack;
    Callout->CallbackStack = (ULONG64)Thread->CallbackStack;
    Callout->Frame.TrapFrame = (ULONG64)TrapFrame;

    /* User mode resumes with its own nonvolatile and floating point state */
    Stack = (PKI_USER_TRAP_STACK)Callout - 1;
    Stack->TrapFrame = *TrapFrame;
    Stack->VfpState = *TrapFrame->VfpState;
    Stack->ExceptionFrame = *((PKEXCEPTION_FRAME)TrapFrame->VfpState - 1);
    Stack->TrapFrame.VfpState = &Stack->VfpState;
    Stack->TrapFrame.Lr = 0;
    Stack->TrapFrame.Pc = (ULONG64)KeUserCallbackDispatcher;

    _disable();
    Thread->CallbackStack = Callout;
    Thread->InitialStack = Callout;

    KiExceptionExit(&Stack->TrapFrame, &Stack->ExceptionFrame);
}

NTSTATUS
NTAPI
KeUserModeCallback(
    _In_ ULONG RoutineIndex,
    _In_ PVOID Argument,
    _In_ ULONG ArgumentLength,
    _Out_ PVOID *Result,
    _Out_ PULONG ResultLength)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME TrapFrame = Thread->TrapFrame;
    ULONG64 OldSp, OldX0, OldX1, OldX2;
    ULONG_PTR UserArguments;
    ULONG GdiBatchCount;
    NTSTATUS Status;

    ASSERT(Thread->ApcState.KernelApcInProgress == FALSE);
    ASSERT(KeGetPreviousMode() == UserMode);

    OldSp = TrapFrame->Sp;
    OldX0 = TrapFrame->X0;
    OldX1 = TrapFrame->X1;
    OldX2 = TrapFrame->X2;

    /* The arguments go on the user stack */
    UserArguments = ALIGN_DOWN_BY(OldSp - ArgumentLength, 16);
    _SEH2_TRY
    {
        ProbeForWrite((PVOID)UserArguments, ArgumentLength, sizeof(ULONG64));
        RtlCopyMemory((PVOID)UserArguments, Argument, ArgumentLength);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* KiUserCallbackDispatcher(RoutineIndex, Argument, ArgumentLength) */
    TrapFrame->Sp = UserArguments;
    TrapFrame->X0 = RoutineIndex;
    TrapFrame->X1 = UserArguments;
    TrapFrame->X2 = ArgumentLength;

    Status = KiCallUserMode(Result, ResultLength);

    /* A callback that popped the stack hands back its user state */
    TrapFrame = Thread->TrapFrame;
    if (Status != STATUS_CALLBACK_POP_STACK)
    {
        TrapFrame->Sp = OldSp;
        TrapFrame->X0 = OldX0;
        TrapFrame->X1 = OldX1;
        TrapFrame->X2 = OldX2;
    }

    _SEH2_TRY
    {
        GdiBatchCount = ((PTEB)Thread->Teb)->GdiBatchCount;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        GdiBatchCount = 0;
    }
    _SEH2_END;

    /* The flush may call back again, keep it off what the caller sees */
    if (GdiBatchCount)
    {
        TrapFrame->Sp -= 256;
        KeGdiFlushUserBatch();
        TrapFrame->Sp += 256;
    }

    return Status;
}

NTSTATUS
NTAPI
NtCallbackReturn(
    _In_ PVOID Result,
    _In_ ULONG ResultLength,
    _In_ NTSTATUS CallbackStatus)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKI_CALLOUT_STATE Callout = Thread->CallbackStack;
    PKTRAP_FRAME CallbackTrapFrame, TrapFrame;
    PKARM64_VFP_STATE VfpState;

    if (!Callout)
        return STATUS_NO_CALLBACK_ACTIVE;

    *(PVOID *)Callout->Frame.OutputBuffer = Result;
    *(PULONG)Callout->Frame.OutputLength = ResultLength;

    CallbackTrapFrame = Thread->TrapFrame;
    TrapFrame = (PKTRAP_FRAME)Callout->Frame.TrapFrame;

    _disable();

    if (CallbackStatus == STATUS_CALLBACK_POP_STACK)
    {
        VfpState = TrapFrame->VfpState;
        *TrapFrame = *CallbackTrapFrame;
        TrapFrame->VfpState = VfpState;
        *VfpState = *CallbackTrapFrame->VfpState;
        *((PKEXCEPTION_FRAME)VfpState - 1) = *((PKEXCEPTION_FRAME)CallbackTrapFrame->VfpState - 1);
    }
    else if (Thread->Header.DebugActive & 0xFF)
    {
        RtlCopyMemory(TrapFrame->Bcr, CallbackTrapFrame->Bcr, sizeof(TrapFrame->Bcr));
        RtlCopyMemory(TrapFrame->Bvr, CallbackTrapFrame->Bvr, sizeof(TrapFrame->Bvr));
        RtlCopyMemory(TrapFrame->Wcr, CallbackTrapFrame->Wcr, sizeof(TrapFrame->Wcr));
        RtlCopyMemory(TrapFrame->Wvr, CallbackTrapFrame->Wvr, sizeof(TrapFrame->Wvr));
        TrapFrame->DebugRegistersValid = CallbackTrapFrame->DebugRegistersValid;
    }

    Thread->InitialStack = (PVOID)Callout->InitialStack;
    Thread->TrapFrame = TrapFrame;
    Thread->CallbackStack = (PVOID)Callout->CallbackStack;

    _enable();

    KiArm64CallbackReturn(Callout, CallbackStatus);
}
