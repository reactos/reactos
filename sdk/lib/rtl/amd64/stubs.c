/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           AMD64 stubs
 * FILE:              lib/rtl/amd64/stubs.c
 * PROGRAMMERS:        Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>
#include "amd64/ketypes.h"

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
RtlInitializeContext(
    _Reserved_ HANDLE ProcessHandle,
    _Out_ PCONTEXT ThreadContext,
    _In_opt_ PVOID ThreadStartParam,
    _In_ PTHREAD_START_ROUTINE ThreadStartAddress,
    _In_ PVOID StackBase)
{
    /* Make sure the stack is aligned */
    if (((ULONG_PTR)StackBase & 0xF) != 0)
    {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }

    /* Initialize StartAddress and Stack */
    ThreadContext->Rip = (ULONG64)ThreadStartAddress;
    ThreadContext->Rsp = (ULONG64)StackBase;

    /* Enable Interrupts */
    ThreadContext->EFlags = EFLAGS_INTERRUPT_MASK;

    /* Set start parameter */
    ThreadContext->Rcx = (ULONG64)ThreadStartParam;

    /* Initialize floating point and SSE state */
    RtlZeroMemory(&ThreadContext->FltSave, sizeof(ThreadContext->FltSave));
    ThreadContext->MxCsr = INITIAL_MXCSR;
    ThreadContext->FltSave.MxCsr = INITIAL_MXCSR;
    ThreadContext->FltSave.ControlWord = INITIAL_FPCSR;

    /* Initialize integer registers */
    ThreadContext->Rbp = 0x0000000000000000ull;
    ThreadContext->Rax = 0x0000000000000000ull;
    ThreadContext->Rbx = 0x0000000000000001ull;
    ThreadContext->Rbx = 0x0000000000000001ull;
    ThreadContext->Rsi = 0x0000000000000004ull;
    ThreadContext->Rdi = 0x0000000000000005ull;
    ThreadContext->R8  = 0x0000000000000008ull;
    ThreadContext->R9  = 0xF0E0D0C0A0908070ull;
    ThreadContext->R10 = 0x000000000000000Aull;
    ThreadContext->R11 = 0x000000000000000Bull;
    ThreadContext->R12 = 0x000000000000000Cull;
    ThreadContext->R13 = 0x000000000000000Dull;
    ThreadContext->R14 = 0x000000000000000Eull;
    ThreadContext->R15 = 0x000000000000000Full;

    /* Set the context flags */
    ThreadContext->ContextFlags = CONTEXT_FULL;

    return;
}

NTSTATUS
NTAPI
RtlQueueApcWow64Thread(
    _In_ HANDLE ThreadHandle,
    _In_ PKNORMAL_ROUTINE ApcRoutine,
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

