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

NTSTATUS
NTAPI
KeUserModeCallback(
    _In_ ULONG RoutineIndex,
    _In_ PVOID Argument,
    _In_ ULONG ArgumentLength,
    _Out_ PVOID *Result,
    _Out_ PULONG ResultLength)
{
    UNREFERENCED_PARAMETER(RoutineIndex);
    UNREFERENCED_PARAMETER(Argument);
    UNREFERENCED_PARAMETER(ArgumentLength);

    /* Needs a kernel stack switch and the callback frame, not done yet */
    UNIMPLEMENTED;
    *Result = NULL;
    *ResultLength = 0;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtCallbackReturn(
    _In_ PVOID Result,
    _In_ ULONG ResultLength,
    _In_ NTSTATUS CallbackStatus)
{
    UNREFERENCED_PARAMETER(Result);
    UNREFERENCED_PARAMETER(ResultLength);
    UNREFERENCED_PARAMETER(CallbackStatus);
    return STATUS_NO_CALLBACK_ACTIVE;
}

/* The debugger data block names this routine, callouts are not done yet */
NTSTATUS
NTAPI
KiCallUserMode(
    _In_ PVOID *OutputBuffer,
    _In_ PULONG OutputLength)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputLength);
    return STATUS_NOT_IMPLEMENTED;
}
