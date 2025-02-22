/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests for system calls
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

#define EFLAGS_TF               0x100L
#define EFLAGS_INTERRUPT_MASK   0x200L

ULONG g_NoopSyscallNumber = 0;
ULONG g_HandlerCalled = 0;
ULONG g_RandomSeed = 0x63c28b49;

VOID
DoSyscallAndCaptureContext(
    _In_ ULONG SyscallNumber,
    _Out_ PCONTEXT PreContext,
    _Out_ PCONTEXT PostContext);

extern const UCHAR SyscallReturn;

ULONG_PTR
DoSyscallWithUnalignedStack(
    _In_ ULONG64 SyscallNumber);

#ifdef _M_IX86
__declspec(dllimport)
VOID
NTAPI
KiFastSystemCallRet(VOID);
#endif

static
BOOLEAN
InitSysCalls()
{
    /* Scan instructions in NtFlushWriteBuffer to find the syscall number
       for NtFlushWriteBuffer, which is a noop syscall on x86/x64 */
    PUCHAR Instructions = (PUCHAR)NtFlushWriteBuffer;
    for (ULONG i = 0; i < 32; i++)
    {
        if (Instructions[i] == 0xB8)
        {
            g_NoopSyscallNumber = *(PULONG)&Instructions[i + 1];
            return TRUE;
        }
    }

    return FALSE;
}

static
VOID
LoadUser32()
{
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    ok(hUser32 != NULL, "Failed to load user32.dll\n");
}

static
LONG
WINAPI
VectoredExceptionHandlerForUserModeCallback(
    struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    g_HandlerCalled++;

    /* Return from the callback */
    NtCallbackReturn(NULL, 0, 0xdeadbeef);

    /* If that failed, we were not in a callback, keep searching */
    return EXCEPTION_CONTINUE_SEARCH;
}

VOID
ValidateSyscall_(
    _In_ PCCH File,
    _In_ ULONG Line,
    _In_ ULONG_PTR SyscallId,
    _In_ ULONG_PTR Result)
{
    CONTEXT PreContext, PostContext;

#ifdef _M_IX86
    DoSyscallAndCaptureContext(SyscallId, &PreContext, &PostContext);

    /* Non-volatile registers and rsp are unchanged */
    ok_eq_hex_(File, Line, PostContext.Esp, PreContext.Esp);
    ok_eq_hex_(File, Line, PostContext.Ebx, PreContext.Ebx);
    ok_eq_hex_(File, Line, PostContext.Esi, PreContext.Esi);
    ok_eq_hex_(File, Line, PostContext.Edi, PreContext.Edi);
    ok_eq_hex_(File, Line, PostContext.Ebp, PreContext.Ebp);

    /* Special cases */
    ok_eq_hex_(File, Line, PostContext.Ecx, PreContext.Esp - 0x4C);
    ok_eq_hex_(File, Line, PostContext.Edx, (ULONG)KiFastSystemCallRet);
    ok_eq_hex_(File, Line, PostContext.Eax, Result);

#elif defined(_M_AMD64)
    /* Initiaize the pre-contex with random numbers */
    PULONG64 IntegerRegs = &PreContext.Rax;
    PM128A XmmRegs = &PreContext.Xmm0;
    for (ULONG Index = 0; Index < 16; Index++)
    {
        IntegerRegs[Index] = (ULONG64)RtlRandom(&g_RandomSeed) << 32 | RtlRandom(&g_RandomSeed);
        XmmRegs[Index].Low = (ULONG64)RtlRandom(&g_RandomSeed) << 32 | RtlRandom(&g_RandomSeed);
        XmmRegs[Index].High = (ULONG64)RtlRandom(&g_RandomSeed) << 32 | RtlRandom(&g_RandomSeed);
    }
    PreContext.EFlags = RtlRandom(&g_RandomSeed);
    PreContext.EFlags &= ~(EFLAGS_TF | 0x20 | 0x40000);
    PreContext.EFlags |= EFLAGS_INTERRUPT_MASK;

    PreContext.SegDs = 0; //0x0028;
    PreContext.SegEs = 0; //0x002B;
    PreContext.SegFs = 0; //0x0053;
    PreContext.SegGs = 0; //0x002B;
    PreContext.SegSs = 0; // 0x002B;

    DoSyscallAndCaptureContext(SyscallId, &PreContext, &PostContext);

    /* Non-volatile registers and rsp are unchanged */
    ok_eq_hex64_(File, Line, PostContext.Rsp, PreContext.Rsp);
    ok_eq_hex64_(File, Line, PostContext.Rbx, PreContext.Rbx);
    ok_eq_hex64_(File, Line, PostContext.Rsi, PreContext.Rsi);
    ok_eq_hex64_(File, Line, PostContext.Rdi, PreContext.Rdi);
    ok_eq_hex64_(File, Line, PostContext.Rbp, PreContext.Rbp);
    ok_eq_hex64_(File, Line, PostContext.R12, PreContext.R12);
    ok_eq_hex64_(File, Line, PostContext.R13, PreContext.R13);
    ok_eq_hex64_(File, Line, PostContext.R14, PreContext.R14);
    ok_eq_hex64_(File, Line, PostContext.R15, PreContext.R15);
    ok_eq_hex64_(File, Line, PostContext.Xmm6.Low, PreContext.Xmm6.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm6.High, PreContext.Xmm6.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm7.Low, PreContext.Xmm7.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm7.High, PreContext.Xmm7.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm8.Low, PreContext.Xmm8.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm8.High, PreContext.Xmm8.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm9.Low, PreContext.Xmm9.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm9.High, PreContext.Xmm9.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm10.Low, PreContext.Xmm10.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm10.High, PreContext.Xmm10.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm11.Low, PreContext.Xmm11.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm11.High, PreContext.Xmm11.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm12.Low, PreContext.Xmm12.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm12.High, PreContext.Xmm12.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm13.Low, PreContext.Xmm13.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm13.High, PreContext.Xmm13.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm14.Low, PreContext.Xmm14.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm14.High, PreContext.Xmm14.High);
    ok_eq_hex64_(File, Line, PostContext.Xmm15.Low, PreContext.Xmm15.Low);
    ok_eq_hex64_(File, Line, PostContext.Xmm15.High, PreContext.Xmm15.High);

    /* Parity flag is flaky */
    ok_eq_hex64_(File, Line, PostContext.EFlags & ~0x4, PreContext.EFlags & ~0x9F5);

    ok_eq_hex64_(File, Line, PostContext.SegCs, 0x0033);
    ok_eq_hex64_(File, Line, PostContext.SegSs, 0x002B);
    ok_(File, Line)(PostContext.SegDs == PreContext.SegDs || PostContext.SegDs == 0x002B,
        "Expected 0x002B, got 0x%04X\n", PostContext.SegDs);
    ok_(File, Line)(PostContext.SegEs == PreContext.SegEs || PostContext.SegEs == 0x002B,
        "Expected 0x002B, got 0x%04X\n", PostContext.SegEs);
    ok_(File, Line)(PostContext.SegFs == PreContext.SegFs || PostContext.SegFs == 0x0053,
        "Expected 0x002B, got 0x%04X\n", PostContext.SegFs);
    ok_(File, Line)(PostContext.SegGs == PreContext.SegGs || PostContext.SegGs == 0x002B,
        "Expected 0x002B, got 0x%04X\n", PostContext.SegGs);
    ok_eq_hex64_(File, Line, PostContext.SegSs, 0x002B);

    /* These volatile registers are zeroed */
    ok_eq_hex64_(File, Line, PostContext.Rdx, 0);
    ok_eq_hex64_(File, Line, PostContext.R10, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm0.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm0.High, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm1.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm1.High, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm2.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm2.High, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm3.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm3.High, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm4.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm4.High, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm5.Low, 0);
    ok_eq_hex64_(File, Line, PostContext.Xmm5.High, 0);

    /* Special cases */
    ok_eq_hex64_(File, Line, PostContext.Rax, Result);
    ok_eq_hex64_(File, Line, PostContext.Rcx, (ULONG64)&SyscallReturn);
    ok_eq_hex64_(File, Line, PostContext.R8, PreContext.Rsp);
    ok_eq_hex64_(File, Line, PostContext.R9, PreContext.Rbp);
    ok_eq_hex64_(File, Line, PostContext.R11, PostContext.EFlags);

    // TODO:Debug regs, mxcsr, floating point, etc.
#else
#error Unsupported architecture
#endif
}

#define ValidateSyscall(SyscallId, Result) ValidateSyscall_(__FILE__, __LINE__, SyscallId, Result)

static
VOID
Test_SyscallNumbers()
{
    BOOL Wow64Process;

    if (IsWow64Process(NtCurrentProcess(), &Wow64Process) && Wow64Process)
    {
        skip("Skipping syscall tests on WOW64\n");
        return;
    }

    /* Test valid syscall number */
    ValidateSyscall(g_NoopSyscallNumber, STATUS_SUCCESS);

    /* Test invalid syscall number */
    ValidateSyscall(0x0FFF, (ULONG)STATUS_INVALID_SYSTEM_SERVICE);

    /* Add a vectored exception handler to catch the exception we will get
       when KiUserCallbackDispatcher is called and user32.dll is not loaded
       We cannot use SEH here, because the exception is outside of the try block */
    PVOID hHandler = AddVectoredExceptionHandler(TRUE, VectoredExceptionHandlerForUserModeCallback);
    ok(hHandler != NULL, "Failed to add vectored exception handler\n");

    /* Test win32k syscall number without user32.dll loaded */
#ifdef _M_AMD64
    ValidateSyscall(0x1000, STATUS_SUCCESS);
#else
    ValidateSyscall(0x1000, (ULONG)STATUS_INVALID_SYSTEM_SERVICE);
#endif
    ok_eq_ulong(g_HandlerCalled, 1UL);

    /* Test invalid win32k syscall number without user32.dll loaded */
#ifdef _M_IX86
    ValidateSyscall(0x1FFF, 0xffffffbf);
#else
    ValidateSyscall(0x1FFF, (ULONG)STATUS_INVALID_SYSTEM_SERVICE);
#endif

    ok_eq_ulong(g_HandlerCalled, 2UL);

    RemoveVectoredExceptionHandler(hHandler);

    LoadUser32();

    /* Test invalid win32k syscall number */
#ifdef _M_IX86
    ValidateSyscall(0x1FFF, 0xffffffbf);
#else
    ValidateSyscall(0x1FFF, (ULONG)STATUS_INVALID_SYSTEM_SERVICE);
#endif

    /* Test invalid syscall table number */
    ValidateSyscall(0x2000 + g_NoopSyscallNumber, STATUS_SUCCESS);
    ValidateSyscall(0x3000 + g_NoopSyscallNumber, STATUS_SUCCESS);

#if 0 // This only happens, when running the test from VS, but not from the command line
    /* For some unknown reason the result gets sign extended in this case */
    ULONG64 Result = DoSyscallWithUnalignedStack(0x2000);
    ok_eq_hex64(Result, (LONG)STATUS_ACCESS_VIOLATION);
#endif

    /* Test invalid upper bits in syscall number */
    ValidateSyscall(0xFFFFFFFFFFF70000ULL + g_NoopSyscallNumber, STATUS_SUCCESS);
}

static
VOID
Test_SyscallPerformance()
{
    ULONG64 Start, End, Cycles;
    ULONG64 TotalCycles = 0, Min = -1, Max = 0;
    ULONG64 Count = 100000;
    ULONG Outliers = 0;
    ULONG_PTR OldAffinityMask;
    double AvgCycles;

    OldAffinityMask = SetThreadAffinityMask(GetCurrentThread(), 1);

    for (ULONG64 i = 0; i < Count; i++)
    {
        Start = __rdtsc();
        NtFlushWriteBuffer();
        End = __rdtsc();
        Cycles = End - Start;
        if (Cycles > 2000)
        {
            Outliers++;
            continue;
        }
        TotalCycles += Cycles;
        Min = min(Min, Cycles);
        Max = max(Max, Cycles);
    }

    AvgCycles = (double)TotalCycles / (Count - Outliers);

    trace("NtFlushWriteBuffer: avg %.2f cycles, min %I64u, max %I64u, Outliers %lu\n",
          AvgCycles, Min, Max, Outliers);

    SetThreadAffinityMask(GetCurrentThread(), OldAffinityMask);
}

START_TEST(SystemCall)
{
    if (!InitSysCalls())
    {
        skip("Failed to initialize.\n");
        return;
    }

    Test_SyscallNumbers();
    Test_SyscallPerformance();
}
