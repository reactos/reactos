/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for x64 and x86 RtlRemoteCall
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

EXTERN_C void _stdcall RtlRemoteCall_TestProc(void);

static
ULONG
NTAPI
ThreadStartStub(
    _In_ PVOID ThreadParameter)
{
    UNREFERENCED_PARAMETER(ThreadParameter);
    return STATUS_SUCCESS;
}

static
NTSTATUS
RemoteCall(
    _In_ PVOID CallSite,
    _In_ ULONG ArgumentCount,
    _In_reads_(ArgumentCount) PULONG_PTR Arguments,
    _In_ BOOLEAN PassContext)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    CONTEXT Context;
    THREAD_BASIC_INFORMATION tbi;

    /* Create new thread in suspended state */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 ThreadStartStub,
                                 NULL,
                                 &ThreadHandle,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Preset some values into the context */
    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Fail;
    }
#if defined(_M_X64)
    Context.Rax = 0xEAEAEAEAEAEAEAEA;
    Context.Rbx = 0xEBEBEBEBEBEBEBEB;
    Context.Rcx = 0xECECECECECECECEC;
    Context.Rdx = 0xEDEDEDEDEDEDEDED;
    Context.Rsi = 0xEFEFEFEFEFEFEFEF;
    Context.Rdi = 0xFEFEFEFEFEFEFEFE;
    Context.R8  = 0x0808080808080808;
    Context.R9  = 0x0909090909090909;
    Context.R10 = 0x1010101010101010;
    Context.R11 = 0x1111111111111111;
    Context.R12 = 0x1212121212121212;
    Context.R13 = 0x1313131313131313;
    Context.R14 = 0x1414141414141414;
    Context.R15 = 0x1515151515151515;
    Context.Rbp = PassContext;
#elif defined(_M_IX86)
    Context.Eax = 0xEAEAEAEA;
    Context.Ebx = 0xEBEBEBEB;
    Context.Ecx = 0xECECECEC;
    Context.Edx = 0xEDEDEDED;
    Context.Esi = 0xEFEFEFEF;
    Context.Edi = 0xFEFEFEFE;
    Context.Ebp = PassContext;
#endif
    Status = NtSetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Fail;
    }

    /* Call remote procedure and wait for exit code */
    Status = RtlRemoteCall(NtCurrentProcess(),
                           ThreadHandle,
                           CallSite,
                           ArgumentCount,
                           Arguments,
                           PassContext,
                           TRUE);
    if (!NT_SUCCESS(Status))
    {
        goto _Fail;
    }
    Status = NtResumeThread(ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto _Fail;
    }
    Status = NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (Status != STATUS_WAIT_0)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto _Fail;
    }
    Status = NtQueryInformationThread(ThreadHandle, ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
    if (NT_SUCCESS(Status))
    {
        NtClose(ThreadHandle);
        return tbi.ExitStatus;
    }

_Fail:
    NtTerminateThread(ThreadHandle, Status);
    NtClose(ThreadHandle);
    return Status;
}

static ULONG_PTR Params[] = {
#if defined(_M_X64)
    0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE
#elif defined(_M_IX86)
    0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE
#endif
};

START_TEST(RtlRemoteCall)
{
    NTSTATUS Status;

    /* PassContext is set */
    Status = RemoteCall(RtlRemoteCall_TestProc, 4, Params, TRUE);
    ok(Status == STATUS_SUCCESS, "RemoteCall failed with 0x%08lX\n", Status);

    /* PassContext is not set */
    Status = RemoteCall(RtlRemoteCall_TestProc, 4, Params, FALSE);
    ok(Status == STATUS_SUCCESS, "RemoteCall failed with 0x%08lX\n", Status);

    /* The maximum number of parameters is 4 */
    Status = RemoteCall(RtlRemoteCall_TestProc, 5, Params, TRUE);
    ok(Status == STATUS_INVALID_PARAMETER,
       "RtlRemoteCall returns status 0x%08lX, expect 0x%08lX\n", Status, STATUS_INVALID_PARAMETER);
}
