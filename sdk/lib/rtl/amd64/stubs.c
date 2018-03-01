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
RtlInitializeContext(IN HANDLE ProcessHandle,
                     OUT PCONTEXT ThreadContext,
                     IN PVOID ThreadStartParam  OPTIONAL,
                     IN PTHREAD_START_ROUTINE ThreadStartAddress,
                     IN PINITIAL_TEB StackBase)
{

    ThreadContext->Rax = 0;
    ThreadContext->Rbx = 0;
    ThreadContext->Rcx = (ULONG64)ThreadStartParam;
    ThreadContext->Rdx = 0;
    ThreadContext->Rsi = 0;
    ThreadContext->Rdi = 0;
    ThreadContext->Rbp = 0;
    ThreadContext->R8 = 0;
    ThreadContext->R9 = 0;
    ThreadContext->R10 = 0;
    ThreadContext->R11 = 0;
    ThreadContext->R12 = 0;

    /* Set the Selectors */
    if ((LONG64)ThreadStartAddress < 0)
    {
        /* Initialize kernel mode segments */
        ThreadContext->SegCs = KGDT64_R0_CODE;
        ThreadContext->SegDs = KGDT64_R3_DATA;
        ThreadContext->SegEs = KGDT64_R3_DATA;
        ThreadContext->SegFs = KGDT64_R3_CMTEB;
        ThreadContext->SegGs = KGDT64_R3_DATA;
        ThreadContext->SegSs = KGDT64_R0_DATA;
    }
    else
    {
        /* Initialize user mode segments */
        ThreadContext->SegCs = KGDT64_R3_CODE |  RPL_MASK;
        ThreadContext->SegDs = KGDT64_R3_DATA |  RPL_MASK;
        ThreadContext->SegEs = KGDT64_R3_DATA |  RPL_MASK;
        ThreadContext->SegFs = KGDT64_R3_CMTEB |  RPL_MASK;
        ThreadContext->SegGs = KGDT64_R3_DATA |  RPL_MASK;
        ThreadContext->SegSs = KGDT64_R3_DATA |  RPL_MASK;
    }

    /* Enable Interrupts */
    ThreadContext->EFlags = EFLAGS_INTERRUPT_MASK;

    /* Settings passed */
    ThreadContext->Rip = (ULONG64)ThreadStartAddress;
    ThreadContext->Rsp = (ULONG64)StackBase - 6 * sizeof(PVOID);

    /* Align stack by 16 and substract 8 (unaligned on function entry) */
    ThreadContext->Rsp &= ~15;
    ThreadContext->Rsp -= 8;

    /* Only the basic Context is initialized */
    ThreadContext->ContextFlags = CONTEXT_CONTROL |
                                  CONTEXT_INTEGER |
                                  CONTEXT_SEGMENTS;

    return;
}

NTSYSAPI
VOID
RtlRestoreContext(
    PCONTEXT ContextRecord,
    PEXCEPTION_RECORD ExceptionRecord)
{
    UNIMPLEMENTED;
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

