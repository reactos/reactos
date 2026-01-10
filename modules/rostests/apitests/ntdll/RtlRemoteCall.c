/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for x64 and x86 RtlRemoteCall
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

#define RTL_REMOTECALL_MAX_ARG_COUNT 4

/* High 16 bits should be zero */
#define TEST_VALUE_CALLSITE ((PVOID)(ULONG_PTR)0x00001C1C1C1C1C1CULL)

#define PRESET_VALUE_A ((ULONG_PTR)0xEAEAEAEAEAEAEAEAULL)
#define PRESET_VALUE_B ((ULONG_PTR)0xEBEBEBEBEBEBEBEBULL)
#define PRESET_VALUE_C ((ULONG_PTR)0xECECECECECECECECULL)
#define PRESET_VALUE_D ((ULONG_PTR)0xEDEDEDEDEDEDEDEDULL)
#define PRESET_VALUE_E ((ULONG_PTR)0xEFEFEFEFEFEFEFEFULL)
#define PRESET_VALUE_F ((ULONG_PTR)0xFEFEFEFEFEFEFEFEULL)
#define PRESET_VALUE_G ((ULONG_PTR)0xFDFDFDFDFDFDFDFDULL)
#define PRESET_VALUE_08 ((ULONG_PTR)0x0808080808080808ULL)
#define PRESET_VALUE_09 ((ULONG_PTR)0x0909090909090909ULL)
#define PRESET_VALUE_10 ((ULONG_PTR)0x1010101010101010ULL)
#define PRESET_VALUE_11 ((ULONG_PTR)0x1111111111111111ULL)
#define PRESET_VALUE_12 ((ULONG_PTR)0x1212121212121212ULL)
#define PRESET_VALUE_13 ((ULONG_PTR)0x1313131313131313ULL)
#define PRESET_VALUE_14 ((ULONG_PTR)0x1414141414141414ULL)
#define PRESET_VALUE_15 ((ULONG_PTR)0x1515151515151515ULL)

static ULONG_PTR g_Params[RTL_REMOTECALL_MAX_ARG_COUNT + 1] = {
    (ULONG_PTR)0xAAAAAAAAAAAAAAAAULL,
    (ULONG_PTR)0xBBBBBBBBBBBBBBBBULL,
    (ULONG_PTR)0xCCCCCCCCCCCCCCCCULL,
    (ULONG_PTR)0xDDDDDDDDDDDDDDDDULL,
    (ULONG_PTR)0xEEEEEEEEEEEEEEEEULL
};

static
void
RemoteCall(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ ULONG ArgumentCount,
    _In_reads_(ArgumentCount) PULONG_PTR Arguments,
    _In_ BOOLEAN PassContext)
{
    NTSTATUS Status;
    CONTEXT Context, *PassedContextPtr, PassedContext;
    ULONG_PTR OldPc, OldSp, ArgSize, Params[RTL_REMOTECALL_MAX_ARG_COUNT + 1], *ParamsPtr;

    /* Preset some values into the context */
    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Skip;
    }
#if defined(_M_X64)
    OldPc = Context.Rip;
    OldSp = Context.Rsp; 
    Context.Rax = PRESET_VALUE_A;
    Context.Rbx = PRESET_VALUE_B;
    Context.Rcx = PRESET_VALUE_C;
    Context.Rdx = PRESET_VALUE_D;
    Context.Rsi = PRESET_VALUE_E;
    Context.Rdi = PRESET_VALUE_F;
    Context.Rbp = PRESET_VALUE_G;
    Context.R8  = PRESET_VALUE_08;
    Context.R9  = PRESET_VALUE_09;
    Context.R10 = PRESET_VALUE_10;
    Context.R11 = PRESET_VALUE_11;
    Context.R12 = PRESET_VALUE_12;
    Context.R13 = PRESET_VALUE_13;
    Context.R14 = PRESET_VALUE_14;
    Context.R15 = PRESET_VALUE_15;
#elif defined(_M_IX86)
    OldPc = Context.Eip;
    OldSp = Context.Esp; 
    Context.Eax = PRESET_VALUE_A;
    Context.Ebx = PRESET_VALUE_B;
    Context.Ecx = PRESET_VALUE_C;
    Context.Edx = PRESET_VALUE_D;
    Context.Esi = PRESET_VALUE_E;
    Context.Edi = PRESET_VALUE_F;
    Context.Ebp = PRESET_VALUE_G;
#endif
    Status = NtSetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Skip;
    }

    /* Call RtlRemoteCall and get context */
    Status = RtlRemoteCall(ProcessHandle,
                           ThreadHandle,
                           TEST_VALUE_CALLSITE,
                           ArgumentCount,
                           Arguments,
                           PassContext,
                           TRUE);
    if (ArgumentCount > RTL_REMOTECALL_MAX_ARG_COUNT)
    {
        ok(Status == STATUS_INVALID_PARAMETER, "RtlRemoteCall returned wrong status 0x%08lX\n", Status);
    }
    else
    {
        ok(Status == STATUS_SUCCESS, "RtlRemoteCall failed with 0x%08lX\n", Status);
    }
    if (!NT_SUCCESS(Status))
    {
        return;
    }
    Status = NtGetContextThread(ThreadHandle, &Context);
    if (!NT_SUCCESS(Status))
    {
        goto _Skip;
    }

    /* Check preset values untouched */
#if defined(_M_X64)
    ok_size_t(Context.Rax, STATUS_ALERTED);
    ok_size_t(Context.Rbx, PRESET_VALUE_B);
    ok_size_t(Context.Rcx, PRESET_VALUE_C);
    ok_size_t(Context.Rdx, PRESET_VALUE_D);
    ok_size_t(Context.Rsi, PRESET_VALUE_E);
    ok_size_t(Context.Rdi, PRESET_VALUE_F);
    ok_size_t(Context.Rbp, PRESET_VALUE_G);
    ok_size_t(Context.R8 , PRESET_VALUE_08);
    ok_size_t(Context.R9 , PRESET_VALUE_09);
    ok_size_t(Context.R10, PRESET_VALUE_10);
#elif defined(_M_IX86)
    ok_size_t(Context.Eax, PRESET_VALUE_A);
    ok_size_t(Context.Ebx, PRESET_VALUE_B);
    ok_size_t(Context.Ecx, PRESET_VALUE_C);
    ok_size_t(Context.Edx, PRESET_VALUE_D);
    ok_size_t(Context.Esi, PRESET_VALUE_E);
    ok_size_t(Context.Edi, PRESET_VALUE_F);
    ok_size_t(Context.Ebp, PRESET_VALUE_G);
#endif

    /* Check new stack pointer, program counter, and input parameters */
    ArgSize = ArgumentCount * sizeof(*Arguments);
#if defined(_M_X64)
    ok_size_t(Context.Rsp, OldSp - sizeof(CONTEXT));
    ok_size_t(Context.Rip, (ULONG_PTR)TEST_VALUE_CALLSITE);
    Params[0] = Context.R11;
    Params[1] = Context.R12;
    Params[2] = Context.R13;
    Params[3] = Context.R14;
    Params[4] = Context.R15;
    if (PassContext)
    {
        ok_size_t(Context.R11, Context.Rsp);
        ParamsPtr = &Params[1];
    }
    else
    {
        ParamsPtr = &Params[0];
        ok_size_t(Context.R15, PRESET_VALUE_15);
    }
#elif defined(_M_IX86)
    ULONG_PTR NewSp = OldSp - ArgSize;
    if (PassContext)
    {
        NewSp -= sizeof(PCONTEXT) + sizeof(CONTEXT);
    }
    ok_size_t(Context.Esp, NewSp);
    ok_size_t(Context.Eip, (ULONG_PTR)TEST_VALUE_CALLSITE);
    Status = NtReadVirtualMemory(ProcessHandle, (PVOID)Context.Esp, Params, sizeof(Params), NULL);
    if (!NT_SUCCESS(Status))
    {
        goto _Skip;
    }
    ParamsPtr = &Params[PassContext ? 1 : 0];
#else
    ParamsPtr = NULL;
#endif
    if (ParamsPtr != NULL)
    {
        ok(memcmp(ParamsPtr, Arguments, ArgSize) == 0, "Passed parameters are incorrect\n");
    }

    /* Check passed context, which was written to stack */
#if defined(_M_X64)
    PassedContextPtr = (PCONTEXT)Context.Rsp;
#elif defined(_M_IX86)
    PassedContextPtr = PassContext ? (PCONTEXT)(Context.Esp + ArgSize + sizeof(PCONTEXT)) : NULL;
#else
    PassedContextPtr = NULL;
#endif
    if (PassedContextPtr != NULL)
    {
        /* If PassContext, the first parameter passed is the pointer to the context passed */
        if (PassContext)
        {
            ok_size_t((ULONG_PTR)PassedContextPtr, Params[0]);
        }
        Status = NtReadVirtualMemory(ProcessHandle,
                                     PassedContextPtr,
                                     &PassedContext,
                                     sizeof(PassedContext),
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            goto _Skip;
        }
#if defined(_M_X64)
        ok_eq_ulong(PassedContext.ContextFlags, CONTEXT_FULL);
        ok_size_t(PassedContext.Rax, STATUS_ALERTED);
        ok_size_t(PassedContext.Rbx, PRESET_VALUE_B);
        ok_size_t(PassedContext.Rcx, PRESET_VALUE_C);
        ok_size_t(PassedContext.Rdx, PRESET_VALUE_D);
        ok_size_t(PassedContext.Rsi, PRESET_VALUE_E);
        ok_size_t(PassedContext.Rdi, PRESET_VALUE_F);
        ok_size_t(PassedContext.Rbp, PRESET_VALUE_G);
        ok_size_t(PassedContext.R8 , PRESET_VALUE_08);
        ok_size_t(PassedContext.R9 , PRESET_VALUE_09);
        ok_size_t(PassedContext.R10, PRESET_VALUE_10);
        ok_size_t(PassedContext.R11, PRESET_VALUE_11);
        ok_size_t(PassedContext.R12, PRESET_VALUE_12);
        ok_size_t(PassedContext.R13, PRESET_VALUE_13);
        ok_size_t(PassedContext.R14, PRESET_VALUE_14);
        ok_size_t(PassedContext.R15, PRESET_VALUE_15);
        ok_size_t(PassedContext.Rip, OldPc);
        ok_size_t(PassedContext.Rsp, OldSp);
#elif defined(_M_IX86)
        ok_eq_ulong(PassedContext.ContextFlags, CONTEXT_FULL);
        ok_size_t(PassedContext.Eax, PRESET_VALUE_A);
        ok_size_t(PassedContext.Ebx, PRESET_VALUE_B);
        ok_size_t(PassedContext.Ecx, PRESET_VALUE_C);
        ok_size_t(PassedContext.Edx, PRESET_VALUE_D);
        ok_size_t(PassedContext.Esi, PRESET_VALUE_E);
        ok_size_t(PassedContext.Edi, PRESET_VALUE_F);
        ok_size_t(PassedContext.Ebp, PRESET_VALUE_G);
        ok_size_t(PassedContext.Eip, OldPc);
        ok_size_t(PassedContext.Esp, OldSp);
#endif
    }

    return;

_Skip:
    skip("API failed with 0x%08lX, test skipped", Status);
}

START_TEST(RtlRemoteCall)
{
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    /*
     * Create a process in a suspended state to test,
     * set DEBUG_PROCESS flag to make sure the process will be terminated if this test went wrong.
     */
    if (!CreateProcessW(NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer,
                        NULL,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_SUSPENDED | DEBUG_PROCESS,
                        NULL,
                        NULL,
                        &si,
                        &pi))
    {
        skip("CreateProcessW failed with 0x%08lX, test cannot continue\n", GetLastError());
        return;
    }

    /* PassContext is set */
    RemoteCall(pi.hProcess, pi.hThread, RTL_REMOTECALL_MAX_ARG_COUNT, g_Params, TRUE);

    /* PassContext is not set */
    RemoteCall(pi.hProcess, pi.hThread, RTL_REMOTECALL_MAX_ARG_COUNT, g_Params, FALSE);

    /* The maximum number of parameters is 4 */
    RemoteCall(pi.hProcess, pi.hThread, RTL_REMOTECALL_MAX_ARG_COUNT + 1, g_Params, TRUE);

    /* Cleanup */
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
