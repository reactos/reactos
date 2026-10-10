/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Test for NtCreateThread
 * PROGRAMMER:  Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 * PROGRAMMER:  Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

PVOID TestThreadProcPtr;
CONTEXT TestThreadStartupContext;
DECLSPEC_ALIGN(16) UCHAR TestThreadStartupStack[PAGE_SIZE];
BOOLEAN TestThreadFunctionCalled;
extern void ThreadStartupThunk(void*);

#define RPL_MASK                0x0003
#define MODE_MASK               0x0001
#define EFLAGS_INTERRUPT_MASK   0x200L
#define KGDT_R3_CODE            0x0018
#define KGDT_R3_DATA            0x0020
#define KGDT_R3_TEB             0x0038
#define KGDT64_R3_CMCODE        0x0020
#define KGDT64_R3_DATA          0x0028
#define KGDT64_R3_CODE          0x0030
#define KGDT64_R3_CMTEB         0x0050

static
VOID
InitializeTestContext(PCONTEXT Context)
{
    RtlFillMemory(Context, sizeof(*Context), 0xAA);

    Context->ContextFlags = CONTEXT_ALL;

#ifdef _M_AMD64

    /* Control */
    Context->Rsp = (ULONG64)(TestThreadStartupStack + PAGE_SIZE - 16);
    Context->Rip = (ULONG64)ThreadStartupThunk;

    /* Set Eflags. These get sanitized, but 0x100 (trap flag) makes it trap instantly. */
    Context->EFlags = 0xFFFFFFFF;
    Context->EFlags &= ~0x100;

    /* Integer (these are copied) */
    Context->Rax = 0xF000000000000000;
    Context->Rcx = 0xF000000000000001;
    Context->Rdx = 0xF000000000000002;
    Context->Rbx = 0xF000000000000003;
    Context->Rbp = 0xF000000000000005;
    Context->Rsi = 0xF000000000000006;
    Context->Rdi = 0xF000000000000007;
    Context->R8  = 0xF000000000000008;
    Context->R9  = 0xF000000000000009;
    Context->R10 = 0xF00000000000000A;
    Context->R11 = 0xF00000000000000B;
    Context->R12 = 0xF00000000000000C;
    Context->R13 = 0xF00000000000000D;
    Context->R14 = 0xF00000000000000E;
    Context->R15 = 0xF00000000000000F;

#elif defined(_M_IX86)

    /* Control */
    Context->Esp = (ULONG)(TestThreadStartupStack + PAGE_SIZE - 16);
    Context->Eip = (ULONG)ThreadStartupThunk;

    /* Set Eflags. These get sanitized, but 0x100 (trap flag) makes it trap instantly. */
    Context->EFlags = 0xFFFFFFFF;
    Context->EFlags &= ~0x100; // Disable trap flag, because it would make the thread trap instantly
    Context->EFlags &= ~0x00020000; // Disable V86 flag

    /* Integer (these are copied) */
    Context->Eax = 0xF0000000;
    Context->Ecx = 0xF0000001;
    Context->Edx = 0xF0000002;
    Context->Ebx = 0xF0000003;
    Context->Ebp = 0xF0000005;
    Context->Esi = 0xF0000006;
    Context->Edi = 0xF0000007;

    /* FIXME: Initialize segments to prevent ReactOS from crashing in the kernel */
    Context->SegCs = KGDT_R3_CODE | RPL_MASK;
    Context->SegSs = KGDT_R3_DATA | RPL_MASK;
    Context->SegDs = KGDT_R3_DATA | RPL_MASK;
    Context->SegEs = KGDT_R3_DATA | RPL_MASK;
    Context->SegFs = KGDT_R3_TEB | RPL_MASK;
    Context->SegGs = 0;

#else

#pragma message("Missing CONTEXT setup in NtCreateThread test")
    ASSERT(FALSE);

#endif
}

VOID
NTAPI
TestThreadProc(IN PVOID StartContext)
{
    TestThreadFunctionCalled = TRUE;

#if defined(_M_IX86) || defined(_M_AMD64)
    CONTEXT Context;

    /* Get the debug registers and floating point */
    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT;
    GetThreadContext(NtCurrentThread(), &Context);
    TestThreadStartupContext.Dr0 = Context.Dr0;
    TestThreadStartupContext.Dr1 = Context.Dr1;
    TestThreadStartupContext.Dr2 = Context.Dr2;
    TestThreadStartupContext.Dr3 = Context.Dr3;
    TestThreadStartupContext.Dr6 = Context.Dr6;
    TestThreadStartupContext.Dr7 = Context.Dr7;
#if defined(_M_IX86)
    TestThreadStartupContext.FloatSave = Context.FloatSave;
#endif
#endif

    /* Terminate current thread */
    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
}

START_TEST(NtCreateThread)
{
    NTSTATUS Status;
    INITIAL_TEB InitialTeb;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES Attributes;
    DECLSPEC_ALIGN(16) ULONG_PTR Stack[128];
    PULONG_PTR StackPtr = &Stack[ARRAYSIZE(Stack) - 2];
    CONTEXT Context;
    CLIENT_ID ClientId;

    BOOL IsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &IsWow64);

    InitializeObjectAttributes(&Attributes, NULL, 0, NULL, NULL);
    ZeroMemory(&InitialTeb, sizeof(INITIAL_TEB));

    Status = NtCreateThread(&ThreadHandle,
                            0,
                            &Attributes,
                            NtCurrentProcess(),
                            NULL,
                            (PCONTEXT)0x70000000, /* Aligned usermode address */
                            &InitialTeb,
                            FALSE);

    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    InitialTeb.PreviousStackBase = NULL;
    InitialTeb.PreviousStackLimit = NULL;
    InitialTeb.StackBase = StackPtr;
    InitialTeb.StackLimit = Stack;
    InitialTeb.AllocatedStackBase = Stack;

    TestThreadProcPtr = TestThreadProc;

    RtlFillMemory(&TestThreadStartupContext, sizeof(TestThreadStartupContext), 0xCC);
    InitializeTestContext(&Context);

    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            NULL,
                            NtCurrentProcess(),
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    NtResumeThread(ThreadHandle, NULL);
    WaitForSingleObject(ThreadHandle, INFINITE);
    CloseHandle(ThreadHandle);

    ok_eq_bool(TestThreadFunctionCalled, TRUE);
    TestThreadFunctionCalled = FALSE;

#ifdef _M_AMD64

    /* Control */
    ok_eq_hex64(TestThreadStartupContext.Rsp, Context.Rsp - 0x28);
    ok_eq_hex64(TestThreadStartupContext.SegCs, 0x0033);
    ok_eq_hex64(TestThreadStartupContext.EFlags, 0x200ED7);
    ok_eq_hex(TestThreadStartupContext.SegSs, 0x0002B);

    /* Integer (copied) */
    ok_eq_hex64(TestThreadStartupContext.Rax, Context.Rax);
    ok_eq_hex64(TestThreadStartupContext.Rcx, Context.Rcx);
    ok_eq_hex64(TestThreadStartupContext.Rdx, Context.Rdx);
    ok_eq_hex64(TestThreadStartupContext.Rbx, Context.Rbx);
    ok_eq_hex64(TestThreadStartupContext.Rbp, (GetNTVersion() >= _WIN32_WINNT_WIN10) ? Context.Rbp : 0);
    ok_eq_hex64(TestThreadStartupContext.Rsi, Context.Rsi);
    ok_eq_hex64(TestThreadStartupContext.Rdi, Context.Rdi);
    ok_eq_hex64(TestThreadStartupContext.R8,  Context.R8);
    ok_eq_hex64(TestThreadStartupContext.R9,  Context.R9);
    ok_eq_hex64(TestThreadStartupContext.R10, Context.R10);
    ok_eq_hex64(TestThreadStartupContext.R11, Context.R11);
    ok_eq_hex64(TestThreadStartupContext.R12, Context.R12);
    ok_eq_hex64(TestThreadStartupContext.R13, Context.R13);
    ok_eq_hex64(TestThreadStartupContext.R14, Context.R14);
    ok_eq_hex64(TestThreadStartupContext.R15, Context.R15);

    /* Segments (hardcoded) */
    ok_eq_hex(TestThreadStartupContext.SegDs, KGDT64_R3_DATA | RPL_MASK);
    ok_eq_hex(TestThreadStartupContext.SegEs, KGDT64_R3_DATA | RPL_MASK);
    ok_eq_hex(TestThreadStartupContext.SegFs, KGDT64_R3_CMTEB | RPL_MASK);
    ok_eq_hex(TestThreadStartupContext.SegGs, KGDT64_R3_DATA | RPL_MASK);

    /* Floating point (hardcoded) */
    ok_eq_hex64(TestThreadStartupContext.MxCsr, INITIAL_MXCSR);
    ok_eq_hex(TestThreadStartupContext.FltSave.ControlWord, INITIAL_FPCSR);
    ok_eq_hex(TestThreadStartupContext.FltSave.StatusWord, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.TagWord, 0x00);
    ok_eq_hex(TestThreadStartupContext.FltSave.Reserved1, 0x00);
    ok_eq_hex(TestThreadStartupContext.FltSave.ErrorOpcode, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.ErrorOffset, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.FltSave.ErrorSelector, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.Reserved2, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.DataOffset, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.FltSave.DataSelector, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.Reserved3, 0x0000);
    ok_eq_hex(TestThreadStartupContext.FltSave.MxCsr, INITIAL_MXCSR);
    ok_eq_hex(TestThreadStartupContext.FltSave.MxCsr_Mask, 0x0002FFFF);
    for (ULONG i = 0; i < ARRAYSIZE(TestThreadStartupContext.FltSave.FloatRegisters); i++)
    {
        ok_eq_hex64(TestThreadStartupContext.FltSave.FloatRegisters[i].Low, 0x0000000000000000ull);
        ok_eq_hex64(TestThreadStartupContext.FltSave.FloatRegisters[i].High, 0x0000000000000000ull);
    }
    PM128A XmmRegisters = &TestThreadStartupContext.Xmm0;
    for (ULONG i = 0; i < 16; i++)
    {
        ok_eq_hex64(XmmRegisters[i].Low, 0x0000000000000000ull);
        ok_eq_hex64(XmmRegisters[i].High, 0x0000000000000000ull);
    }
    for (ULONG i = 0; i < ARRAYSIZE(TestThreadStartupContext.VectorRegister); i++)
    {
        ok_eq_hex64(TestThreadStartupContext.VectorRegister[i].Low, 0xCCCCCCCCCCCCCCCCull);
        ok_eq_hex64(TestThreadStartupContext.VectorRegister[i].High, 0xCCCCCCCCCCCCCCCCull);
    }

    /* Debug registers (reset to 0) */
    ok_eq_hex64(TestThreadStartupContext.Dr0, 0x0000000000000000ull);
    ok_eq_hex64(TestThreadStartupContext.Dr1, 0x0000000000000000ull);
    ok_eq_hex64(TestThreadStartupContext.Dr2, 0x0000000000000000ull);
    ok_eq_hex64(TestThreadStartupContext.Dr3, 0x0000000000000000ull);
    ok_eq_hex64(TestThreadStartupContext.Dr6, 0x0000000000000000ull);
    ok_eq_hex64(TestThreadStartupContext.Dr7, 0x0000000000000000ull);

#elif defined(_M_IX86)

    /* Control */
    ok_eq_hex(TestThreadStartupContext.Esp, Context.Esp);
    ok_eq_hex(TestThreadStartupContext.EFlags, IsWow64 ? 0x00240ED7 : 0x003C4ED7);

    /* Integer (copied) */
    ok_eq_hex(TestThreadStartupContext.Eax, Context.Eax);
    ok_eq_hex(TestThreadStartupContext.Ecx, Context.Ecx);
    ok_eq_hex(TestThreadStartupContext.Edx, Context.Edx);
    ok_eq_hex(TestThreadStartupContext.Ebx, Context.Ebx);
    ok_eq_hex(TestThreadStartupContext.Ebp, Context.Ebp);
    ok_eq_hex(TestThreadStartupContext.Esi, Context.Esi);
    ok_eq_hex(TestThreadStartupContext.Edi, Context.Edi);

    /* Segments (hardcoded, the upper 16 bits are left uninitialized by RtlCaptureContext) */
    ok_eq_hex((USHORT)TestThreadStartupContext.SegCs, IsWow64 ? KGDT64_R3_CMCODE | RPL_MASK : KGDT_R3_CODE | RPL_MASK);
    ok_eq_hex((USHORT)TestThreadStartupContext.SegDs, IsWow64 ? KGDT64_R3_DATA | RPL_MASK : KGDT_R3_DATA | RPL_MASK);
    ok_eq_hex((USHORT)TestThreadStartupContext.SegEs, IsWow64 ? KGDT64_R3_DATA | RPL_MASK : KGDT_R3_DATA | RPL_MASK);
    ok_eq_hex((USHORT)TestThreadStartupContext.SegFs, IsWow64 ? KGDT64_R3_CMTEB | RPL_MASK : KGDT_R3_TEB | RPL_MASK);
    ok_eq_hex((USHORT)TestThreadStartupContext.SegGs, IsWow64 ? KGDT64_R3_DATA | RPL_MASK : 0x0000);
    ok_eq_hex((USHORT)TestThreadStartupContext.SegSs, IsWow64 ? KGDT64_R3_DATA | RPL_MASK : KGDT_R3_DATA | RPL_MASK);

    /* Floating point (hardcoded) */
    ok_eq_hex(TestThreadStartupContext.FloatSave.ControlWord, IsWow64 ? 0x27F : 0xFFFF0A6A);
    ok_eq_hex(TestThreadStartupContext.FloatSave.StatusWord, IsWow64 ? 0 : 0xFFFF2A2A);
    ok_eq_hex(TestThreadStartupContext.FloatSave.TagWord, IsWow64 ? 0x0000FFFF : 0xFFFF0000);
    ok_eq_hex(TestThreadStartupContext.FloatSave.ErrorOffset, IsWow64 ? 0 : 0xAAAAAAAA);
    ok_eq_hex(TestThreadStartupContext.FloatSave.ErrorSelector, IsWow64 ? 0 : 0x02AAAAAA);
    ok_eq_hex(TestThreadStartupContext.FloatSave.DataOffset, IsWow64 ? 0 : 0xAAAAAAAA);
    ok_eq_hex(TestThreadStartupContext.FloatSave.DataSelector, IsWow64 ? 0 : 0xFFFFAAAA);

    /* Debug registers (reset to 0) */
    ok_eq_hex(TestThreadStartupContext.Dr0, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.Dr1, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.Dr2, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.Dr3, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.Dr6, 0x00000000);
    ok_eq_hex(TestThreadStartupContext.Dr7, 0x00000000);

#else

#pragma message("Missing CONTEXT validation in NtCreateThread test")
    ASSERT(FALSE);

#endif

#if defined(_M_AMD64) || defined(_M_IX86)
    /* Test Eflags set to 0 */
    Context.EFlags = 0;
    TestThreadStartupContext.EFlags = 0xCCCCCCCC;
    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            NULL,
                            NtCurrentProcess(),
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    NtResumeThread(ThreadHandle, NULL);
    WaitForSingleObject(ThreadHandle, INFINITE);
    CloseHandle(ThreadHandle);
    ok_eq_hex(TestThreadStartupContext.EFlags, 0x202);
#endif

#if !defined(_M_IX86) // FIXME: This crashes on x86
    /* Test different InitialTeb values */
    InitialTeb.PreviousStackBase = Stack;
    InitialTeb.PreviousStackLimit = NULL;
    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            NULL,
                            NtCurrentProcess(),
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            FALSE);
    ok_eq_hex(Status, (GetNTVersion() >= _WIN32_WINNT_WIN10) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS);

    InitialTeb.PreviousStackBase = NULL;
    InitialTeb.PreviousStackLimit = Stack;
    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            NULL,
                            NtCurrentProcess(),
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            FALSE);
    ok_eq_hex(Status, (GetNTVersion() >= _WIN32_WINNT_WIN10) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS);
#endif
}
