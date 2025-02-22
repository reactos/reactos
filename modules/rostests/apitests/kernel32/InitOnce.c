/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for One-Time initialization APIs
 * COPYRIGHT:   Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

typedef
VOID
WINAPI
FN_InitOnceInitialize(_Out_ PINIT_ONCE InitOnce);

typedef
BOOL
WINAPI
FN_InitOnceExecuteOnce(
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID *Context);

typedef
BOOL
WINAPI
FN_InitOnceBeginInitialize(
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _Out_ PBOOL fPending,
    _Outptr_opt_result_maybenull_ LPVOID *lpContext);

typedef
BOOL
WINAPI
FN_InitOnceComplete(
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _In_opt_ LPVOID lpContext);

static ULONG g_ulRandom;

static
VOID
InitWorker(_Inout_ PULONG InitCount, _Out_ PULONG_PTR Context)
{
    /* Increase the initialization count */
    (*InitCount)++;

    /* Output context data */
    *Context = (ULONG_PTR)g_ulRandom;
}

static
_Success_(return != FALSE)
BOOL
WINAPI
InitOnceProc(
    _Inout_ PINIT_ONCE InitOnce,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID* Context)
{
    if (!Parameter || !Context)
    {
        return FALSE;
    }

    InitWorker(Parameter, (PULONG_PTR)Context);
    return TRUE;
}

START_TEST(InitOnce)
{
    BOOL bRet, fPending;
    ULONG i, ulInitCount, ulSeed;
    ULONG_PTR ulContextData, ulTempContext;
    DWORD dwError;

    HMODULE hKernel32;
    FN_InitOnceInitialize* pfnInitOnceInitialize;
    FN_InitOnceExecuteOnce* pfnInitOnceExecuteOnce;
    FN_InitOnceBeginInitialize* pfnInitOnceBeginInitialize;
    FN_InitOnceComplete* pfnInitOnceComplete;

    /* Load functions */
    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32)
    {
        skip("Module kernel32 not found\n");
        return;
    }
    pfnInitOnceInitialize = (FN_InitOnceInitialize*)GetProcAddress(hKernel32, "InitOnceInitialize");
    pfnInitOnceExecuteOnce = (FN_InitOnceExecuteOnce*)GetProcAddress(hKernel32, "InitOnceExecuteOnce");
    pfnInitOnceBeginInitialize = (FN_InitOnceBeginInitialize*)GetProcAddress(hKernel32, "InitOnceBeginInitialize");
    pfnInitOnceComplete = (FN_InitOnceComplete*)GetProcAddress(hKernel32, "InitOnceComplete");

    /*
     * Use a random as output context data,
     * which the low-order INIT_ONCE_CTX_RESERVED_BITS bits should be zero.
     */
    ulSeed = (ULONG)(ULONG_PTR)&ulSeed ^ GetTickCount();
    g_ulRandom = RtlRandom(&ulSeed);
    for (i = 0; i < INIT_ONCE_CTX_RESERVED_BITS; i++)
    {
        g_ulRandom &= (~(1 << i));
    }

    /* Initialize One-Time initialization structure */
    INIT_ONCE InitOnce = { (PVOID)(ULONG_PTR)0xDEADBEEF };
    if (pfnInitOnceInitialize)
    {
        pfnInitOnceInitialize(&InitOnce);
    }
    else
    {
        skip("InitOnceInitialize not found\n");
        InitOnce = (INIT_ONCE)INIT_ONCE_STATIC_INIT;
    }

    if (!pfnInitOnceExecuteOnce)
    {
        skip("InitOnceExecuteOnce not found\n");
        goto _test_sync;
    }

    /*
     * Perform synchronous initialization by using InitOnceExecuteOnce,
     * which executes user-defined callback to initialize.
     * Call InitOnceExecuteOnce twice will success,
     * initialization count should be 1 and retrieve correct context data.
     */
    ulInitCount = 0;
    ulContextData = MAXULONG;
    bRet = pfnInitOnceExecuteOnce(&InitOnce, InitOnceProc, &ulInitCount, (LPVOID*)&ulContextData);
    ok(bRet, "InitOnceExecuteOnce failed with %lu\n", GetLastError());
    if (bRet)
    {
        /* Call InitOnceExecuteOnce again and check output values if the first call succeeded */
        bRet = pfnInitOnceExecuteOnce(&InitOnce,
                                      InitOnceProc,
                                      &ulInitCount,
                                      (LPVOID*)&ulContextData);
        ok(bRet, "InitOnceExecuteOnce failed with %lu\n", GetLastError());
        ok(ulInitCount == 1, "ulInitCount is not 1\n");
        ok(ulContextData == g_ulRandom, "Output ulContextData is incorrect\n");
    }

_test_sync:
    if (!pfnInitOnceBeginInitialize || !pfnInitOnceComplete)
    {
        skip("InitOnceBeginInitialize or InitOnceComplete not found\n");
        return;
    }

    /* Re-initialize One-Time initialization structure by using INIT_ONCE_STATIC_INIT */
    InitOnce = (INIT_ONCE)INIT_ONCE_STATIC_INIT;
    ulContextData = 0xdeadbeef;

    /* Perform synchronous initialization by using InitOnceBeginInitialize */
    fPending = FALSE;
    bRet = pfnInitOnceBeginInitialize(&InitOnce, 0, &fPending, (LPVOID*)&ulContextData);
    ok(bRet, "InitOnceBeginInitialize failed with %lu\n", GetLastError());
    if (!bRet)
    {
        goto _test_async;
    }
    ok(fPending, "fPending is not TRUE after the first success InitOnceBeginInitialize\n");
    if (!fPending)
    {
        goto _test_async;
    }

    /* Call again to check whether initialization has completed */
    fPending = 0xdeadbeef;
    bRet = pfnInitOnceBeginInitialize(&InitOnce,
                                      INIT_ONCE_CHECK_ONLY,
                                      &fPending,
                                      (LPVOID*)&ulContextData);
    ok(bRet == FALSE, "InitOnceBeginInitialize should fail\n");
    ok(fPending == 0xdeadbeef, "fPending should be unmodified\n");
    ok(ulContextData == 0xdeadbeef, "ulContextData should be unmodified\n");

    /* Complete the initialization */
    InitWorker(&ulInitCount, &ulTempContext);
    bRet = pfnInitOnceComplete(&InitOnce, 0, (LPVOID)ulTempContext);
    ok(bRet, "InitOnceComplete failed with %lu\n", GetLastError());
    if (!bRet)
    {
        goto _test_async;
    }

    /*
     * Initialization is completed, call InitOnceBeginInitialize with
     * INIT_ONCE_CHECK_ONLY should retrieve status and context data successfully
     */
    bRet = pfnInitOnceBeginInitialize(&InitOnce,
                                      INIT_ONCE_CHECK_ONLY,
                                      &fPending,
                                      (LPVOID*)&ulContextData);
    ok(bRet && !fPending && ulContextData == g_ulRandom,
       "InitOnceBeginInitialize returns incorrect result for a completed initialization\n");

_test_async:
    InitOnce = (INIT_ONCE)INIT_ONCE_STATIC_INIT;

    /* Perform asynchronous initialization */
    fPending = FALSE;
    bRet = pfnInitOnceBeginInitialize(&InitOnce,
                                      INIT_ONCE_ASYNC,
                                      &fPending,
                                      (LPVOID*)&ulContextData);
    ok(bRet, "InitOnceBeginInitialize failed with %lu\n", GetLastError());
    if (!bRet)
    {
        return;
    }
    ok(fPending, "fPending is not TRUE after a success InitOnceBeginInitialize\n");
    if (!fPending)
    {
        return;
    }

    /*
     * Now the initialization is in progress but not completed yet,
     * call InitOnceBeginInitialize again without INIT_ONCE_ASYNC is invalid,
     * should fail with ERROR_INVALID_PARAMETER
     */
    bRet = pfnInitOnceBeginInitialize(&InitOnce, 0, &fPending, (LPVOID*)&ulContextData);
    ok(!bRet, "InitOnceBeginInitialize should not success\n");
    if (!bRet)
    {
        dwError = GetLastError();
        ok(dwError == ERROR_INVALID_PARAMETER,
           "Last error is %lu, but %u is expected\n",
           dwError,
           ERROR_INVALID_PARAMETER);
    }

    /*
     * Call InitOnceBeginInitialize again with INIT_ONCE_ASYNC
     * should success because initialization could be executed in parallel
     */
    bRet = pfnInitOnceBeginInitialize(&InitOnce,
                                      INIT_ONCE_ASYNC,
                                      &fPending,
                                      (LPVOID*)&ulContextData);
    ok(bRet, "InitOnceBeginInitialize failed with %lu\n", GetLastError());
    if (!bRet)
    {
        return;
    }
    ok(fPending, "fPending is not TRUE after a success InitOnceBeginInitialize\n");
    if (!fPending)
    {
        return;
    }

    /* Complete the initialization once */
    InitWorker(&ulInitCount, &ulTempContext);
    bRet = pfnInitOnceComplete(&InitOnce, INIT_ONCE_ASYNC, (LPVOID)ulTempContext);
    ok(bRet, "InitOnceComplete failed with %lu\n", GetLastError());
    if (!bRet)
    {
        return;
    }

    /* Subsequent InitOnceComplete should fail with ERROR_GEN_FAILURE */
    bRet = pfnInitOnceComplete(&InitOnce, INIT_ONCE_ASYNC, (LPVOID)ulTempContext);
    ok(!bRet, "InitOnceComplete should not success\n");
    if (!bRet)
    {
        dwError = GetLastError();
        ok(dwError == ERROR_GEN_FAILURE,
           "Last error is %lu, but %u is expected\n",
           dwError,
           ERROR_GEN_FAILURE);
    }

    /*
     * Initialization is completed, call InitOnceBeginInitialize with
     * INIT_ONCE_CHECK_ONLY should retrieve status and context data successfully
     */
    bRet = pfnInitOnceBeginInitialize(&InitOnce,
                                      INIT_ONCE_CHECK_ONLY,
                                      &fPending,
                                      (LPVOID*)&ulContextData);
    ok(bRet && !fPending && ulContextData == g_ulRandom,
       "InitOnceBeginInitialize returns incorrect result for a completed initialization\n");

    return;
}
