/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 thread context setup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*! RtlInitializeContext
 * \brief Sets up the initial context of a new thread.
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
    ULONG i;

    UNREFERENCED_PARAMETER(ProcessHandle);

    if ((ULONG_PTR)StackBase & 0xF)
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);

    ThreadContext->ContextFlags = CONTEXT_FULL;
    ThreadContext->Cpsr = 0;

    /* The start parameter, then recognizable junk in the other registers */
    ThreadContext->X0 = (ULONG64)ThreadStartParam;
    for (i = 1; i < 28; i++)
        ThreadContext->X[i] = i;
    ThreadContext->X9 = 0xF0E0D0C0A0908070ull;

    ThreadContext->Fp = 0;
    ThreadContext->Lr = 0;
    ThreadContext->Sp = (ULONG64)StackBase;
    ThreadContext->Pc = (ULONG64)ThreadStartAddress;

    RtlZeroMemory(ThreadContext->V, sizeof(ThreadContext->V));
    ThreadContext->Fpcr = 0;
    ThreadContext->Fpsr = 0;
}
