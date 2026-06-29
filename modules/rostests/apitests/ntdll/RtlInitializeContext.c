/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RtlInitializeContext
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlInitializeContext)
{
    CONTEXT Context;
    DECLSPEC_ALIGN(16) ULONG_PTR Stack[4];
    PULONG_PTR StackPtr = &Stack[ARRAYSIZE(Stack) - 2];

    RtlFillMemory(Stack, sizeof(Stack), 0xCC);
    RtlFillMemory(&Context, sizeof(CONTEXT), 0xCC);
    RtlInitializeContext(NtCurrentProcess(),
                         &Context,
                         (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                         (PTHREAD_START_ROUTINE)(ULONG_PTR)0xCAFEF00DCAFEF00Dull,
                         StackPtr);

#ifdef _M_IX86
    #define INITIAL_FPCSR 0x000027F0
    ok_eq_hex(Context.ContextFlags, CONTEXT_FULL);
    ok_eq_hex(Context.Dr0, 0xCCCCCCCC);
    ok_eq_hex(Context.Dr1, 0xCCCCCCCC);
    ok_eq_hex(Context.Dr2, 0xCCCCCCCC);
    ok_eq_hex(Context.Dr3, 0xCCCCCCCC);
    ok_eq_hex(Context.Dr6, 0xCCCCCCCC);
    ok_eq_hex(Context.Dr7, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.ControlWord, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.StatusWord, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.TagWord, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.ErrorOffset, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.ErrorSelector, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.DataOffset, 0xCCCCCCCC);
    ok_eq_hex(Context.FloatSave.DataSelector, 0xCCCCCCCC);
    for (ULONG i = 0; i < ARRAYSIZE(Context.FloatSave.RegisterArea); i++)
    {
        ok_eq_hex(Context.FloatSave.RegisterArea[i], 0xCC);
    }
    ok_eq_hex(Context.FloatSave.Cr0NpxState, 0xCCCCCCCC);
    ok_eq_hex(Context.SegGs, 0x00000000);
    ok_eq_hex(Context.SegFs, 0x00000038);
    ok_eq_hex(Context.SegEs, 0x00000020);
    ok_eq_hex(Context.SegDs, 0x00000020);
    ok_eq_hex(Context.Edi, 0x00000005);
    ok_eq_hex(Context.Esi, 0x00000004);
    ok_eq_hex(Context.Ebx, 0x00000001);
    ok_eq_hex(Context.Edx, 0x00000003);
    ok_eq_hex(Context.Ecx, 0x00000002);
    ok_eq_hex(Context.Eax, 0x00000000);
    ok_eq_hex(Context.Ebp, 0x00000000);
    ok_eq_hex(Context.Eip, 0xCAFEF00D);
    ok_eq_hex(Context.SegCs, 0x00000018);
    ok_eq_hex(Context.EFlags, 0x00000200);
    ok_eq_hex(Context.Esp, (ULONG)(StackPtr - 2));
    ok_eq_hex(Context.SegSs, 0x00000020);
    for (ULONG i = 0; i < ARRAYSIZE(Context.ExtendedRegisters); i++)
    {
        ok_eq_hex(Context.ExtendedRegisters[i], 0xCC);
    }

    ok_eq_hex(Stack[0], 0xCCCCCCCC);
    ok_eq_hex(Stack[1], 0xDEADBEEF);
    ok_eq_hex(Stack[2], 0xCCCCCCCC);
    ok_eq_hex(Stack[3], 0xCCCCCCCC);
#elif defined(_M_AMD64)
    ok_eq_hex64(Context.P1Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.P2Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.P3Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.P4Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.P5Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.P6Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex(Context.ContextFlags, CONTEXT_FULL);
    ok_eq_hex(Context.MxCsr, INITIAL_MXCSR);
    ok_eq_hex(Context.SegCs, 0xCCCC);
    ok_eq_hex(Context.SegDs, 0xCCCC);
    ok_eq_hex(Context.SegEs, 0xCCCC);
    ok_eq_hex(Context.SegFs, 0xCCCC);
    ok_eq_hex(Context.SegGs, 0xCCCC);
    ok_eq_hex(Context.SegSs, 0xCCCC);
    ok_eq_hex(Context.EFlags, 0x200);
    ok_eq_hex64(Context.Dr0, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Dr1, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Dr2, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Dr3, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Dr6, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Dr7, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Rax, 0x0000000000000000ull);
    ok_eq_hex64(Context.Rcx, 0xDEADBEEFDEADBEEFull);
    ok_eq_hex64(Context.Rdx, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.Rbx, 0x0000000000000001ull);
    ok_eq_hex64(Context.Rsp, (ULONG64)StackPtr);
    ok_eq_hex64(Context.Rbp, 0x0000000000000000ull);
    ok_eq_hex64(Context.Rsi, 0x0000000000000004ull);
    ok_eq_hex64(Context.Rdi, 0x0000000000000005ull);
    ok_eq_hex64(Context.R8,  0x0000000000000008ull);
    ok_eq_hex64(Context.R9,  0xF0E0D0C0A0908070ull);
    ok_eq_hex64(Context.R10, 0x000000000000000Aull);
    ok_eq_hex64(Context.R11, 0x000000000000000Bull);
    ok_eq_hex64(Context.R12, 0x000000000000000Cull);
    ok_eq_hex64(Context.R13, 0x000000000000000Dull);
    ok_eq_hex64(Context.R14, 0x000000000000000Eull);
    ok_eq_hex64(Context.R15, 0x000000000000000Full);
    ok_eq_hex64(Context.Rip, 0xCAFEF00DCAFEF00Dull);
    ok_eq_hex(Context.FltSave.ControlWord, INITIAL_FPCSR);
    ok_eq_hex(Context.FltSave.StatusWord, 0x0000);
    ok_eq_hex(Context.FltSave.TagWord, 0x00);
    ok_eq_hex(Context.FltSave.Reserved1, 0x00);
    ok_eq_hex(Context.FltSave.ErrorOpcode, 0x0000);
    ok_eq_hex(Context.FltSave.ErrorOffset, 0x00000000);
    ok_eq_hex(Context.FltSave.ErrorSelector, 0x0000);
    ok_eq_hex(Context.FltSave.Reserved2, 0x0000);
    ok_eq_hex(Context.FltSave.DataOffset, 0x00000000);
    ok_eq_hex(Context.FltSave.DataSelector, 0x0000);
    ok_eq_hex(Context.FltSave.Reserved3, 0x0000);
    ok_eq_hex(Context.FltSave.MxCsr, INITIAL_MXCSR);
    ok_eq_hex(Context.FltSave.MxCsr_Mask, 0x00000000);
    for (ULONG i = 0; i < ARRAYSIZE(Context.FltSave.FloatRegisters); i++)
    {
        ok_eq_hex64(Context.FltSave.FloatRegisters[i].Low, 0x0000000000000000ull);
        ok_eq_hex64(Context.FltSave.FloatRegisters[i].High, 0x0000000000000000ull);
    }
    PM128A XmmRegisters = &Context.Xmm0;
    for (ULONG i = 0; i < 16; i++)
    {
        ok_eq_hex64(XmmRegisters[i].Low, 0x0000000000000000ull);
        ok_eq_hex64(XmmRegisters[i].High, 0x0000000000000000ull);
    }
    for (ULONG i = 0; i < ARRAYSIZE(Context.VectorRegister); i++)
    {
        ok_eq_hex64(Context.VectorRegister[i].Low, 0xCCCCCCCCCCCCCCCCull);
        ok_eq_hex64(Context.VectorRegister[i].High, 0xCCCCCCCCCCCCCCCCull);
    }
    ok_eq_hex64(Context.VectorControl, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.DebugControl, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.LastBranchToRip, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.LastBranchFromRip, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.LastExceptionToRip, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Context.LastExceptionFromRip, 0xCCCCCCCCCCCCCCCCull);

    ok_eq_hex64(Stack[0], 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Stack[1], 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Stack[2], 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(Stack[3], 0xCCCCCCCCCCCCCCCCull);
#endif

    /* Test NULL Context */
    StartSeh()
    {
        RtlInitializeContext(NtCurrentProcess(),
                             NULL,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                             (PTHREAD_START_ROUTINE)(ULONG_PTR)0xCAFEF00DCAFEF00Dull,
                             StackPtr);
    }
    EndSeh(STATUS_ACCESS_VIOLATION)

    /* Test NULL StartRoutine */
    StartSeh()
    {
        RtlInitializeContext(NtCurrentProcess(),
                             &Context,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                             NULL,
                             StackPtr);
    }
    EndSeh(STATUS_SUCCESS)

    /* Test NULL Stack pointer */
    StartSeh()
    {
        RtlInitializeContext(NtCurrentProcess(),
                             &Context,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                             (PTHREAD_START_ROUTINE)(ULONG_PTR)0xCAFEF00DCAFEF00Dull,
                             NULL);
    }
    EndSeh(STATUS_SUCCESS)

    /* Test invalid Stack pointer */
    StartSeh()
    {
        RtlInitializeContext(NtCurrentProcess(),
                             &Context,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                             (PTHREAD_START_ROUTINE)(ULONG_PTR)0xCAFEF00DCAFEF00Dull,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEE0ull);
    }
    EndSeh(STATUS_SUCCESS)

    /* Test unaligned stack */
    StartSeh()
    {
        RtlInitializeContext(NtCurrentProcess(),
                             &Context,
                             (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull,
                             (PTHREAD_START_ROUTINE)(ULONG_PTR)0xCAFEF00DCAFEF00Dull,
                             (PUCHAR)StackPtr + 1);
    }
#ifdef _M_AMD64
    EndSeh(STATUS_BAD_INITIAL_STACK)
#else
    EndSeh(STATUS_SUCCESS)
#endif

}
