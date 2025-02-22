/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for x64 RtlCaptureContext
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtltests.h>

VOID
RtlCaptureContextWrapper(
    _Inout_ PCONTEXT InOutContext,
    _Out_ PCONTEXT CapturedContext);

START_TEST(RtlCaptureContext)
{
    CONTEXT OriginalContext, InOutContext, CapturedContext;
    SIZE_T Index, MaxIndex;
    PUSHORT Buffer;

    /* Initialize a pattern */
    MaxIndex = sizeof(OriginalContext) / sizeof(USHORT);
    Buffer = (PUSHORT)&OriginalContext;
    for (Index = 0; Index < MaxIndex; Index++)
    {
        Buffer[Index] = Index;
    }

    /* Set all valid bits in EFlags */
    OriginalContext.EFlags = 0xE7F;

    /* Set up a valid floating point state */
    OriginalContext.FltSave.ControlWord = 0x27f;
    OriginalContext.FltSave.StatusWord = 0x1234;
    OriginalContext.FltSave.TagWord = 0xab;
    OriginalContext.FltSave.Reserved1 = 0x75;
    OriginalContext.FltSave.MxCsr = 0x1f80; // Valid Mask is 2FFFF
    OriginalContext.FltSave.MxCsr_Mask = 0xabcde;

    /* Set up a unique MxCsr. This one will overwrite FltSave.MxCsr */
    OriginalContext.MxCsr = 0xffff; // Valid Mask is 0xFFFF

    /* Copy the original buffer */
    InOutContext = OriginalContext;

    /* Fill the output buffer with bogus */
    RtlFillMemory(&CapturedContext, sizeof(CapturedContext), 0xCC);

    /* Call the wrapper function */
    RtlCaptureContextWrapper(&InOutContext, &CapturedContext);

    /* These fields are not changed */
    ok_eq_hex64(CapturedContext.P1Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.P2Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.P3Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.P4Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.P5Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.P6Home, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr0, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr1, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr2, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr3, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr6, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.Dr7, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.VectorControl, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.DebugControl, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.LastBranchToRip, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.LastBranchFromRip, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.LastExceptionToRip, 0xccccccccccccccccULL);
    ok_eq_hex64(CapturedContext.LastExceptionFromRip, 0xccccccccccccccccULL);
    for (Index = 0; Index < ARRAYSIZE(CapturedContext.FltSave.Reserved4); Index++)
    {
        ok_eq_hex64(CapturedContext.FltSave.Reserved4[Index], 0xcc);
    }
    for (Index = 0; Index < ARRAYSIZE(CapturedContext.VectorRegister); Index++)
    {
        ok_eq_hex64(CapturedContext.VectorRegister[Index].Low, 0xccccccccccccccccULL);
        ok_eq_hex64(CapturedContext.VectorRegister[Index].High, 0xccccccccccccccccULL);
    }

    /* ContextFlags is set */
    ok_eq_hex(CapturedContext.ContextFlags, CONTEXT_FULL | CONTEXT_SEGMENTS);

    /* These vallues are as passed in */
    ok_eq_hex(CapturedContext.MxCsr, OriginalContext.MxCsr);
    ok_eq_hex64(CapturedContext.Rax, OriginalContext.Rax);
    ok_eq_hex64(CapturedContext.Rcx, (ULONG64)&CapturedContext);
    ok_eq_hex64(CapturedContext.Rdx, OriginalContext.Rdx);
    ok_eq_hex64(CapturedContext.Rbx, OriginalContext.Rbx);
    ok_eq_hex64(CapturedContext.Rbp, OriginalContext.Rbp);
    ok_eq_hex64(CapturedContext.Rsi, OriginalContext.Rsi);
    ok_eq_hex64(CapturedContext.Rdi, OriginalContext.Rdi);
    ok_eq_hex64(CapturedContext.R8, OriginalContext.R8);
    ok_eq_hex64(CapturedContext.R9, OriginalContext.R9);
    ok_eq_hex64(CapturedContext.R10, OriginalContext.R10);
    ok_eq_hex64(CapturedContext.R11, OriginalContext.R11);
    ok_eq_hex64(CapturedContext.R12, OriginalContext.R12);
    ok_eq_hex64(CapturedContext.R13, OriginalContext.R13);
    ok_eq_hex64(CapturedContext.R14, OriginalContext.R14);
    ok_eq_hex64(CapturedContext.R15, OriginalContext.R15);

    ok_eq_xmm(CapturedContext.Xmm0, OriginalContext.Xmm0);
    ok_eq_xmm(CapturedContext.Xmm1, OriginalContext.Xmm1);
    ok_eq_xmm(CapturedContext.Xmm2, OriginalContext.Xmm2);
    ok_eq_xmm(CapturedContext.Xmm3, OriginalContext.Xmm3);
    ok_eq_xmm(CapturedContext.Xmm4, OriginalContext.Xmm4);
    ok_eq_xmm(CapturedContext.Xmm5, OriginalContext.Xmm5);
    ok_eq_xmm(CapturedContext.Xmm6, OriginalContext.Xmm6);
    ok_eq_xmm(CapturedContext.Xmm7, OriginalContext.Xmm7);
    ok_eq_xmm(CapturedContext.Xmm8, OriginalContext.Xmm8);
    ok_eq_xmm(CapturedContext.Xmm9, OriginalContext.Xmm9);
    ok_eq_xmm(CapturedContext.Xmm10, OriginalContext.Xmm10);
    ok_eq_xmm(CapturedContext.Xmm11, OriginalContext.Xmm11);
    ok_eq_xmm(CapturedContext.Xmm12, OriginalContext.Xmm12);
    ok_eq_xmm(CapturedContext.Xmm13, OriginalContext.Xmm13);
    ok_eq_xmm(CapturedContext.Xmm14, OriginalContext.Xmm14);
    ok_eq_xmm(CapturedContext.Xmm15, OriginalContext.Xmm15);

    /* Some EFlags fields are cleared */
    ok_eq_hex64(CapturedContext.EFlags, OriginalContext.EFlags & ~0x28);

#ifndef KMT_KERNEL_MODE // User mode
    ok_eq_hex(CapturedContext.FltSave.ControlWord, 0x27f);
    ok_eq_hex(CapturedContext.FltSave.StatusWord, OriginalContext.FltSave.StatusWord);
    ok_eq_hex(CapturedContext.FltSave.TagWord, OriginalContext.FltSave.TagWord);
    ok_eq_hex(CapturedContext.FltSave.Reserved1, 0x00);
    ok_eq_hex(CapturedContext.FltSave.MxCsr_Mask, 0xffff);
    ok_eq_hex(CapturedContext.FltSave.ErrorOpcode, 0x00000083);
    ok_eq_hex(CapturedContext.FltSave.ErrorOffset, 0x00850084);
    ok_eq_hex(CapturedContext.FltSave.ErrorSelector, 0x0086);
    ok_eq_hex(CapturedContext.FltSave.Reserved2, 0x0000);
    ok_eq_hex(CapturedContext.FltSave.DataOffset, 0x00890088);
    ok_eq_hex(CapturedContext.FltSave.DataSelector, 0x008a);
    ok_eq_hex(CapturedContext.FltSave.Reserved3, 0x0000);

    /* We get the value from OriginalContext.MxCsr, since we set that later in the wrapper */
    ok_eq_hex(CapturedContext.FltSave.MxCsr, OriginalContext.MxCsr);

    /* Legacy floating point registers are truncated to 10 bytes */
    ok_eq_hex64(CapturedContext.Legacy[0].Low, OriginalContext.Legacy[0].Low);
    ok_eq_hex64(CapturedContext.Legacy[0].High, OriginalContext.Legacy[0].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[1].Low, OriginalContext.Legacy[1].Low);
    ok_eq_hex64(CapturedContext.Legacy[1].High, OriginalContext.Legacy[1].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[2].Low, OriginalContext.Legacy[2].Low);
    ok_eq_hex64(CapturedContext.Legacy[2].High, OriginalContext.Legacy[2].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[3].Low, OriginalContext.Legacy[3].Low);
    ok_eq_hex64(CapturedContext.Legacy[3].High, OriginalContext.Legacy[3].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[4].Low, OriginalContext.Legacy[4].Low);
    ok_eq_hex64(CapturedContext.Legacy[4].High, OriginalContext.Legacy[4].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[5].Low, OriginalContext.Legacy[5].Low);
    ok_eq_hex64(CapturedContext.Legacy[5].High, OriginalContext.Legacy[5].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[6].Low, OriginalContext.Legacy[6].Low);
    ok_eq_hex64(CapturedContext.Legacy[6].High, OriginalContext.Legacy[6].High & 0xFF);
    ok_eq_hex64(CapturedContext.Legacy[7].Low, OriginalContext.Legacy[7].Low);
    ok_eq_hex64(CapturedContext.Legacy[7].High, OriginalContext.Legacy[7].High & 0xFF);
#else
    ok_eq_hex(CapturedContext.FltSave.ControlWord, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.StatusWord, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.TagWord, 0xcc);
    ok_eq_hex(CapturedContext.FltSave.Reserved1, 0xcc);
    ok_eq_hex(CapturedContext.FltSave.MxCsr_Mask, 0xcccccccc);
    ok_eq_hex(CapturedContext.FltSave.ErrorOpcode, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.ErrorOffset, 0xcccccccc);
    ok_eq_hex(CapturedContext.FltSave.ErrorSelector, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.Reserved2, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.DataOffset, 0xcccccccc);
    ok_eq_hex(CapturedContext.FltSave.DataSelector, 0xcccc);
    ok_eq_hex(CapturedContext.FltSave.Reserved3, 0xcccc);

    /* We get the value from OriginalContext.MxCsr, since we set that later in the wrapper */
    ok_eq_hex(CapturedContext.FltSave.MxCsr, 0xcccccccc);

    /* Legacy floating point registers are truncated to 10 bytes */
    ok_eq_hex64(CapturedContext.Legacy[0].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[0].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[1].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[1].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[2].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[2].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[3].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[3].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[4].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[4].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[5].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[5].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[6].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[6].High, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[7].Low, 0xcccccccccccccccc);
    ok_eq_hex64(CapturedContext.Legacy[7].High, 0xcccccccccccccccc);
#endif

    /* We don't pass in segments, but expect the default values */
    ok_eq_hex(CapturedContext.SegDs, 0x2b);
    ok_eq_hex(CapturedContext.SegEs, 0x2b);
    ok_eq_hex(CapturedContext.SegFs, 0x53);
    ok_eq_hex(CapturedContext.SegGs, 0x2b);
#ifndef KMT_KERNEL_MODE // User mode
    ok_eq_hex(CapturedContext.SegCs, 0x33);
    ok_eq_hex(CapturedContext.SegSs, 0x2b);
#else
    ok_eq_hex(CapturedContext.SegCs, 0x10);
    ok_eq_hex(CapturedContext.SegSs, 0x18);
#endif

    /* For Rsp and Rip we get the expected value back from the asm wrapper */
    ok_eq_hex64(CapturedContext.Rsp, InOutContext.Rsp);
    ok_eq_hex64(CapturedContext.Rip, InOutContext.Rip);

    /* Check that these registers are not modified by RtlCaptureContext */
    ok_eq_xmm(InOutContext.Xmm0, OriginalContext.Xmm0);
    ok_eq_xmm(InOutContext.Xmm1, OriginalContext.Xmm1);
    ok_eq_xmm(InOutContext.Xmm2, OriginalContext.Xmm2);
    ok_eq_xmm(InOutContext.Xmm3, OriginalContext.Xmm3);
    ok_eq_xmm(InOutContext.Xmm4, OriginalContext.Xmm4);
    ok_eq_xmm(InOutContext.Xmm5, OriginalContext.Xmm5);
    ok_eq_xmm(InOutContext.Xmm6, OriginalContext.Xmm6);
    ok_eq_xmm(InOutContext.Xmm7, OriginalContext.Xmm7);
    ok_eq_xmm(InOutContext.Xmm8, OriginalContext.Xmm8);
    ok_eq_xmm(InOutContext.Xmm9, OriginalContext.Xmm9);
    ok_eq_xmm(InOutContext.Xmm10, OriginalContext.Xmm10);
    ok_eq_xmm(InOutContext.Xmm11, OriginalContext.Xmm11);
    ok_eq_xmm(InOutContext.Xmm12, OriginalContext.Xmm12);
    ok_eq_xmm(InOutContext.Xmm13, OriginalContext.Xmm13);
    ok_eq_xmm(InOutContext.Xmm14, OriginalContext.Xmm14);
    ok_eq_xmm(InOutContext.Xmm15, OriginalContext.Xmm15);
    ok_eq_hex64(InOutContext.Rdx, OriginalContext.Rdx);
    ok_eq_hex64(InOutContext.Rbx, OriginalContext.Rbx);
    ok_eq_hex64(InOutContext.Rbp, OriginalContext.Rbp);
    ok_eq_hex64(InOutContext.Rsi, OriginalContext.Rsi);
    ok_eq_hex64(InOutContext.Rdi, OriginalContext.Rdi);
    ok_eq_hex64(InOutContext.R8, OriginalContext.R8);
    ok_eq_hex64(InOutContext.R9, OriginalContext.R9);
    ok_eq_hex64(InOutContext.R10, OriginalContext.R10);
    ok_eq_hex64(InOutContext.R11, OriginalContext.R11);
    ok_eq_hex64(InOutContext.R12, OriginalContext.R12);
    ok_eq_hex64(InOutContext.R13, OriginalContext.R13);
    ok_eq_hex64(InOutContext.R14, OriginalContext.R14);
    ok_eq_hex64(InOutContext.R15, OriginalContext.R15);

    /* Eflags is changed (parity is flaky) */
#ifndef KMT_KERNEL_MODE // User mode
    ok_eq_hex64(InOutContext.EFlags & ~0x04, OriginalContext.EFlags & 0x782);
#endif

    /* MxCsr is the one we passed in in OriginalContext.MxCsr */
    ok_eq_hex(InOutContext.MxCsr, OriginalContext.MxCsr);
    ok_eq_hex(InOutContext.FltSave.MxCsr, OriginalContext.MxCsr);

    /* Rcx still points to the captured context */
    ok_eq_hex64(InOutContext.Rcx, (ULONG64)&CapturedContext);

    /* Rax contains eflags */
    ok_eq_hex64(InOutContext.Rax, CapturedContext.EFlags);

    /* Second run with minimal EFLags/MxCsr */
    OriginalContext.EFlags = 0x200;
    OriginalContext.MxCsr = 0x0000;
    InOutContext = OriginalContext;
    RtlFillMemory(&CapturedContext, sizeof(CapturedContext), 0xCC);
    RtlCaptureContextWrapper(&InOutContext, &CapturedContext);

    /* Captured Eflags has reserved flag set (which is always 1) */
    ok_eq_hex64(CapturedContext.EFlags, OriginalContext.EFlags | 2);

    /* Parity flag is flaky */
#ifndef KMT_KERNEL_MODE // User mode
    ok_eq_hex64(InOutContext.EFlags & ~4, CapturedContext.EFlags);
#endif

    /* MxCsr is captured/returned as passed in */
    ok_eq_hex64(InOutContext.MxCsr, CapturedContext.MxCsr);
    ok_eq_hex64(CapturedContext.MxCsr, OriginalContext.MxCsr);
}
