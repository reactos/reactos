/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for RtlUnhandledExceptionFilter(2)
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

static NTSTATUS KnownExceptionCodes[] =
{
/* Well-known exception codes, see include/psdk/minwinbase.h */
    EXCEPTION_ACCESS_VIOLATION,
    EXCEPTION_DATATYPE_MISALIGNMENT,
    EXCEPTION_BREAKPOINT,
    EXCEPTION_SINGLE_STEP,
    EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
    EXCEPTION_FLT_DENORMAL_OPERAND,
    EXCEPTION_FLT_DIVIDE_BY_ZERO,
    EXCEPTION_FLT_INEXACT_RESULT,
    EXCEPTION_FLT_INVALID_OPERATION,
    EXCEPTION_FLT_OVERFLOW,
    EXCEPTION_FLT_STACK_CHECK,
    EXCEPTION_FLT_UNDERFLOW,
    EXCEPTION_INT_DIVIDE_BY_ZERO,
    EXCEPTION_INT_OVERFLOW,
    EXCEPTION_PRIV_INSTRUCTION,
    EXCEPTION_IN_PAGE_ERROR,
    EXCEPTION_ILLEGAL_INSTRUCTION,
    EXCEPTION_NONCONTINUABLE_EXCEPTION,
    EXCEPTION_STACK_OVERFLOW,
    EXCEPTION_INVALID_DISPOSITION,
    EXCEPTION_GUARD_PAGE,
    EXCEPTION_INVALID_HANDLE,
    EXCEPTION_POSSIBLE_DEADLOCK,
/* Additional ones */
    STATUS_STACK_BUFFER_OVERRUN,
};

START_TEST(RtlUnhandledExceptionFilter)
{
    LONG (NTAPI* pRtlUnhandledExceptionFilter)(_In_ PEXCEPTION_POINTERS ExceptionInfo);
    LONG (NTAPI* pRtlUnhandledExceptionFilter2)(_In_ PEXCEPTION_POINTERS ExceptionInfo, _In_ PCSTR Function);
    PPEB Peb = NtCurrentPeb();
    CONTEXT ctx = {0};
    EXCEPTION_RECORD er = {0};
    EXCEPTION_POINTERS ep = {&er, &ctx};

    /* Load functions */
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll)
    {
        skip("GetModuleHandleW(\"ntdll.dll\") failed with 0x%08lX\n", GetLastError());
        return;
    }
    pRtlUnhandledExceptionFilter = (VOID*)GetProcAddress(hNtDll, "RtlUnhandledExceptionFilter");
    pRtlUnhandledExceptionFilter2 = (VOID*)GetProcAddress(hNtDll, "RtlUnhandledExceptionFilter2");
    if (!pRtlUnhandledExceptionFilter || !pRtlUnhandledExceptionFilter2)
    {
        skip("ntdll!RtlUnhandledExceptionFilter(2) not found\n");
        return;
    }

    /* Build a dummy context/exception record. Not really valid on self, don't care. */
    ctx.ContextFlags = CONTEXT_CONTROL;
    GetThreadContext(GetCurrentThread(), &ctx);

    /* Disable BeingDebugged so that RtlUnhandledExceptionFilter(2) doesn't
     * unnecessarily break into the debugger with the debugging instructions.
     * If you want to see them, comment the following line! */
    Peb->BeingDebugged = FALSE;

    /* Test the filter routine return value under different exception codes */
    // for (er.ExceptionCode = 0; er.ExceptionCode != ~0; ++er.ExceptionCode)
    for (UINT i = 0; i < _countof(KnownExceptionCodes); ++i)
    {
        LONG r = EXCEPTION_CONTINUE_SEARCH;
        BOOLEAN fail = TRUE;
        er.ExceptionCode = KnownExceptionCodes[i];

        /* Skip test if stack overflow code; the filter would invoke another crash handler */
        if (er.ExceptionCode == STATUS_STACK_BUFFER_OVERRUN)
            continue;

        _SEH2_TRY
        {
            _SEH2_TRY
            {
                r = pRtlUnhandledExceptionFilter2(&ep, __FUNCTION__);
                fail = FALSE;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            _SEH2_END;
        }
        _SEH2_FINALLY
        {
        }
        _SEH2_END;

        // trace("%#x ret %lu\n", er.ExceptionCode, r);
        if (er.ExceptionCode == STATUS_POSSIBLE_DEADLOCK)
            ok_long(r, EXCEPTION_CONTINUE_EXECUTION);
        else
            ok_long(r, EXCEPTION_CONTINUE_SEARCH);
        if (fail)
            trace("SEH on %#x\n", er.ExceptionCode);
    }
}
