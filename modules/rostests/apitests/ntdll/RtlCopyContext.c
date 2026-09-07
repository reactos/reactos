/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RTL extended context functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
NTSTATUS
NTAPI
FN_RtlCopyContext(
    _Out_ PVOID DstContext,
    _In_ ULONG ContextFlags,
    _In_ const VOID* SrcContext);

FN_RtlCopyContext* pRtlCopyContext;

static ULONG EqualBytes(const VOID* Buffer, SIZE_T BufferSize, ULONG_PTR Offset, UCHAR Value)
{
    PUCHAR Ptr = (PUCHAR)Buffer + Offset;
    SIZE_T Remaining = BufferSize - Offset;
    ULONG Lenth = 0;

    while (Lenth < Remaining && *Ptr == Value)
    {
        Ptr++;
        Lenth++;
    }

    return Lenth;
}

static void CheckContext_I386_(
    ULONG Line,
    const I386_CONTEXT* Context,
    ULONG ContextFlags,
    UCHAR ExpectedValue)
{
    ULONG ExpectedValue32 = ExpectedValue * 0x01010101;
    ULONG MatchingBytes;

    if ((ContextFlags & I386_CONTEXT_CONTROL) == I386_CONTEXT_CONTROL)
    {
        ok_eq_hex_(__FILE__, Line, Context->Eip, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->EFlags, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->SegCs, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->SegSs, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Esp, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Ebp, ExpectedValue32);
    }

    if ((ContextFlags & I386_CONTEXT_INTEGER) == I386_CONTEXT_INTEGER)
    {
        ok_eq_hex_(__FILE__, Line, Context->Eax, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Ebx, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Ecx, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Edx, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Esi, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Edi, ExpectedValue32);
    }

    if ((ContextFlags & I386_CONTEXT_SEGMENTS) == I386_CONTEXT_SEGMENTS)
    {
        ok_eq_hex_(__FILE__, Line, Context->SegDs, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->SegEs, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->SegFs, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->SegGs, ExpectedValue32);
    }

    if ((ContextFlags & I386_CONTEXT_DEBUG_REGISTERS) == I386_CONTEXT_DEBUG_REGISTERS)
    {
        ok_eq_hex_(__FILE__, Line, Context->Dr0, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Dr1, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Dr2, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Dr3, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Dr6, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Dr7, ExpectedValue32);
    }

    if ((ContextFlags & I386_CONTEXT_FLOATING_POINT) == I386_CONTEXT_FLOATING_POINT)
    {
        MatchingBytes = EqualBytes(&Context->FloatSave, sizeof(Context->FloatSave), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->FloatSave));
    }

    if ((ContextFlags & I386_CONTEXT_EXTENDED_REGISTERS) == I386_CONTEXT_EXTENDED_REGISTERS)
    {
        MatchingBytes = EqualBytes(&Context->ExtendedRegisters, 0x120, 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, 0x120);
        MatchingBytes = EqualBytes(&Context->ExtendedRegisters[0x120],
                                   sizeof(Context->ExtendedRegisters) - 0x120, 0, 0x00);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->ExtendedRegisters) - 0x120);
    }
}

#define CheckContext_I386(Context, ContextFlags, ExpectedValue) \
    CheckContext_I386_(__LINE__, Context, ContextFlags, ExpectedValue)

static void CheckContextAll_I386_(
    ULONG Line,
    const I386_CONTEXT* Context,
    UCHAR Expected_CONTROL,
    UCHAR Expected_INTEGER,
    UCHAR Expected_SEGMENTS,
    UCHAR Expected_DEBUG_REGISTERS,
    UCHAR Expected_FLOATING_POINT,
    UCHAR Expected_EXTENDED_REGISTERS)
{
    CheckContext_I386_(Line, Context, I386_CONTEXT_CONTROL, Expected_CONTROL);
    CheckContext_I386_(Line, Context, I386_CONTEXT_INTEGER, Expected_INTEGER);
    CheckContext_I386_(Line, Context, I386_CONTEXT_SEGMENTS, Expected_SEGMENTS);
    CheckContext_I386_(Line, Context, I386_CONTEXT_DEBUG_REGISTERS, Expected_DEBUG_REGISTERS);
    CheckContext_I386_(Line, Context, I386_CONTEXT_FLOATING_POINT, Expected_FLOATING_POINT);
    CheckContext_I386_(Line, Context, I386_CONTEXT_EXTENDED_REGISTERS, Expected_EXTENDED_REGISTERS);
}

#define CheckContextAll_I386(Context, Expected_CONTROL, Expected_INTEGER, Expected_SEGMENTS, Expected_DEBUG_REGISTERS, Expected_FLOATING_POINT, Expected_EXTENDED_REGISTERS) \
    CheckContextAll_I386_(__LINE__, Context, Expected_CONTROL, Expected_INTEGER, Expected_SEGMENTS, Expected_DEBUG_REGISTERS, Expected_FLOATING_POINT, Expected_EXTENDED_REGISTERS)

static void Test_RtlCopyContext_I386(void)
{
    I386_CONTEXT SrcContext = { 0 };
    I386_CONTEXT DstContext = { 0 };
    NTSTATUS Status, ExpectedStatus;

    /* Try to copy an empty context */
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_i386 in the source, but not in the destination */
    SrcContext.ContextFlags = CONTEXT_i386;
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_i386 in both source and destination */
    DstContext.ContextFlags = CONTEXT_i386;
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test invalid ContextFlags */   
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        Status = pRtlCopyContext(&DstContext, CONTEXT_i386 | TestFlag, &SrcContext);
        ExpectedStatus = (TestFlag & ~I386_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid source ContextFlags */   
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        SrcContext.ContextFlags = CONTEXT_i386 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
        ExpectedStatus = (TestFlag & ~I386_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid destination ContextFlags */   
    SrcContext.ContextFlags = CONTEXT_i386;
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        DstContext.ContextFlags = CONTEXT_i386 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
        ExpectedStatus = (TestFlag & ~I386_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Set invalid flag in the source */
    SrcContext.ContextFlags = CONTEXT_i386 | 0x200;
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set invalid flag in the destination */
    SrcContext.ContextFlags = CONTEXT_i386;
    DstContext.ContextFlags = CONTEXT_i386 | 0x200;
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Copy with only CONTEXT_i386 (should copy no contents) */
    RtlFillMemory(&SrcContext, sizeof(SrcContext), 0xAA);
    SrcContext.ContextFlags = CONTEXT_i386;
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    DstContext.ContextFlags = CONTEXT_i386;
    Status = pRtlCopyContext(&DstContext, CONTEXT_i386, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy I386_CONTEXT_CONTROL, but source doesn't have it */
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy I386_CONTEXT_CONTROL */
    SrcContext.ContextFlags = I386_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_CONTROL);

    /* Copy I386_CONTEXT_INTEGER onto dest with I386_CONTEXT_CONTROL */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = I386_CONTEXT_INTEGER;
    DstContext.ContextFlags = I386_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_CONTROL | I386_CONTEXT_INTEGER);

    /* Copy I386_CONTEXT_SEGMENTS onto dest with I386_CONTEXT_DEBUG_REGISTERS */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = I386_CONTEXT_ALL;
    DstContext.ContextFlags = I386_CONTEXT_DEBUG_REGISTERS;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_SEGMENTS, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_DEBUG_REGISTERS | I386_CONTEXT_SEGMENTS);

    /* Copy I386_CONTEXT_DEBUG_REGISTERS onto dest with I386_CONTEXT_INTEGER */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = I386_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = I386_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_ALL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_INTEGER | I386_CONTEXT_DEBUG_REGISTERS);

    /* Copy I386_CONTEXT_FLOATING_POINT */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = I386_CONTEXT_FLOATING_POINT | I386_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = I386_CONTEXT_CONTROL | I386_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_FLOATING_POINT | I386_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x00);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_CONTROL | I386_CONTEXT_INTEGER | I386_CONTEXT_FLOATING_POINT);

    /* Copy I386_CONTEXT_EXTENDED_REGISTERS */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = I386_CONTEXT_EXTENDED_REGISTERS;
    DstContext.ContextFlags = I386_CONTEXT_CONTROL | I386_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, I386_CONTEXT_EXTENDED_REGISTERS, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_I386(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA);
    ok_eq_hex(DstContext.ContextFlags, I386_CONTEXT_CONTROL | I386_CONTEXT_INTEGER | I386_CONTEXT_EXTENDED_REGISTERS);
}

static void CheckContext_AMD64_(
    ULONG Line,
    const AMD64_CONTEXT* Context,
    ULONG ContextFlags,
    UCHAR ExpectedValue)
{
    ULONG64 ExpectedValue64 = ExpectedValue * 0x0101010101010101ULL;
    ULONG ExpectedValue32 = ExpectedValue * 0x01010101;
    ULONG MatchingBytes;

    if ((ContextFlags & AMD64_CONTEXT_CONTROL) == AMD64_CONTEXT_CONTROL)
    {
        ok_eq_hex64_(__FILE__, Line, Context->Rsp, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rip, ExpectedValue64);
        ok_eq_hex_(__FILE__, Line, Context->EFlags, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegCs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegSs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
    }

    if ((ContextFlags & AMD64_CONTEXT_INTEGER) == AMD64_CONTEXT_INTEGER)
    {
        ok_eq_hex64_(__FILE__, Line, Context->Rax, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rcx, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rdx, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rbx, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rbp, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rsi, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Rdi, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R8,  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R9,  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R10, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R11, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R12, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R13, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R14, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->R15, ExpectedValue64);
    }

    if ((ContextFlags & AMD64_CONTEXT_SEGMENTS) == AMD64_CONTEXT_SEGMENTS)
    {
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegDs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegEs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegFs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
        ok_eq_hex_(__FILE__, Line, (ULONG)Context->SegGs, (ULONG)(USHORT)(ExpectedValue * 0x0101));
    }

    if ((ContextFlags & AMD64_CONTEXT_FLOATING_POINT) == AMD64_CONTEXT_FLOATING_POINT)
    {
        ok_eq_hex_(__FILE__, Line, Context->MxCsr, ExpectedValue32);
        MatchingBytes = EqualBytes(&Context->FltSave,
                                   FIELD_OFFSET(XSAVE_FORMAT64, Reserved4), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, FIELD_OFFSET(XSAVE_FORMAT64, Reserved4));
        MatchingBytes = EqualBytes(&Context->FltSave.Reserved4,
                                   sizeof(Context->FltSave.Reserved4), 0, 0);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->FltSave.Reserved4));
    }

    if ((ContextFlags & AMD64_CONTEXT_DEBUG_REGISTERS) == AMD64_CONTEXT_DEBUG_REGISTERS)
    {
        ok_eq_hex64_(__FILE__, Line, Context->Dr0, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Dr1, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Dr2, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Dr3, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Dr6, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Dr7, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->LastBranchToRip, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->LastBranchFromRip, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->LastExceptionToRip, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->LastExceptionFromRip, ExpectedValue64);
    }
}

#define CheckContext_AMD64(Context, ContextFlags, ExpectedValue) \
    CheckContext_AMD64_(__LINE__, Context, ContextFlags, ExpectedValue)

static void CheckContextAll_AMD64_(
    ULONG Line,
    const AMD64_CONTEXT* Context,
    UCHAR Expected_CONTROL,
    UCHAR Expected_INTEGER,
    UCHAR Expected_SEGMENTS,
    UCHAR Expected_FLOATING_POINT,
    UCHAR Expected_DEBUG_REGISTERS)
{
    ULONG MatchingBytes;

    CheckContext_AMD64_(Line, Context, AMD64_CONTEXT_CONTROL, Expected_CONTROL);
    CheckContext_AMD64_(Line, Context, AMD64_CONTEXT_INTEGER, Expected_INTEGER);
    CheckContext_AMD64_(Line, Context, AMD64_CONTEXT_SEGMENTS, Expected_SEGMENTS);
    CheckContext_AMD64_(Line, Context, AMD64_CONTEXT_FLOATING_POINT, Expected_FLOATING_POINT);
    CheckContext_AMD64_(Line, Context, AMD64_CONTEXT_DEBUG_REGISTERS, Expected_DEBUG_REGISTERS);

    /* These are never modified */
    ok_eq_hex64_(__FILE__, Line, Context->P1Home, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->P2Home, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->P3Home, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->P4Home, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->P5Home, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->P6Home, 0x0000000000000000);
    MatchingBytes = EqualBytes(&Context->VectorRegister, sizeof(Context->VectorRegister), 0, 0x00);
    ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->VectorRegister));
    ok_eq_hex64_(__FILE__, Line, Context->VectorControl, 0x0000000000000000);
    ok_eq_hex64_(__FILE__, Line, Context->DebugControl, 0x0000000000000000);
}

#define CheckContextAll_AMD64(Context, Expected_CONTROL, Expected_INTEGER, Expected_SEGMENTS, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS) \
    CheckContextAll_AMD64_(__LINE__, Context, Expected_CONTROL, Expected_INTEGER, Expected_SEGMENTS, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS)

static void Test_RtlCopyContext_AMD64(void)
{
    AMD64_CONTEXT SrcContext = { 0 };
    AMD64_CONTEXT DstContext = { 0 };
    NTSTATUS Status, ExpectedStatus;

    /* Try to copy an empty context */
    Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_AMD64 in the source, but not in the destination */
    SrcContext.ContextFlags = CONTEXT_AMD64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_AMD64 in both source and destination */
    DstContext.ContextFlags = CONTEXT_AMD64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test invalid ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64 | TestFlag, &SrcContext);
        ExpectedStatus = (TestFlag & ~AMD64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid source ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        SrcContext.ContextFlags = CONTEXT_AMD64 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
        ExpectedStatus = (TestFlag & ~AMD64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid destination ContextFlags */
    SrcContext.ContextFlags = CONTEXT_AMD64;
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        DstContext.ContextFlags = CONTEXT_AMD64 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
        ExpectedStatus = (TestFlag & ~AMD64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Copy with only CONTEXT_AMD64 (should copy no contents) */
    RtlFillMemory(&SrcContext, sizeof(SrcContext), 0xAA);
    SrcContext.ContextFlags = CONTEXT_AMD64;
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    DstContext.ContextFlags = CONTEXT_AMD64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_AMD64, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy AMD64_CONTEXT_CONTROL, but source doesn't have it */
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy AMD64_CONTEXT_CONTROL */
    SrcContext.ContextFlags = AMD64_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0xAA, 0x00, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, AMD64_CONTEXT_CONTROL);

    /* Copy AMD64_CONTEXT_INTEGER onto dest with AMD64_CONTEXT_CONTROL */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = AMD64_CONTEXT_INTEGER;
    DstContext.ContextFlags = AMD64_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0xAA, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, AMD64_CONTEXT_CONTROL | AMD64_CONTEXT_INTEGER);

    /* Copy AMD64_CONTEXT_SEGMENTS onto dest with AMD64_CONTEXT_DEBUG_REGISTERS */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = AMD64_CONTEXT_ALL;
    DstContext.ContextFlags = AMD64_CONTEXT_DEBUG_REGISTERS;
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_SEGMENTS, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0x00, 0xAA, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, AMD64_CONTEXT_DEBUG_REGISTERS | AMD64_CONTEXT_SEGMENTS);

    /* Copy AMD64_CONTEXT_DEBUG_REGISTERS onto dest with AMD64_CONTEXT_INTEGER */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = AMD64_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = AMD64_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_ALL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0xAA);
    ok_eq_hex(DstContext.ContextFlags, AMD64_CONTEXT_INTEGER | AMD64_CONTEXT_DEBUG_REGISTERS);

    /* Copy AMD64_CONTEXT_FLOATING_POINT */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = AMD64_CONTEXT_FLOATING_POINT | AMD64_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = AMD64_CONTEXT_CONTROL | AMD64_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, AMD64_CONTEXT_FLOATING_POINT | AMD64_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_AMD64(&DstContext, 0x00, 0x00, 0x00, 0xAA, 0x00);
    ok_eq_hex(DstContext.ContextFlags, AMD64_CONTEXT_CONTROL | AMD64_CONTEXT_INTEGER | AMD64_CONTEXT_FLOATING_POINT);
}

static void CheckContext_ARM32_(
    ULONG Line,
    const ARM32_CONTEXT* Context,
    ULONG ContextFlags,
    UCHAR ExpectedValue)
{
    ULONG ExpectedValue32 = ExpectedValue * 0x01010101;
    ULONG MatchingBytes;

    if ((ContextFlags & ARM32_CONTEXT_CONTROL) == ARM32_CONTEXT_CONTROL)
    {
        ok_eq_hex_(__FILE__, Line, Context->Sp,   ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Lr,   ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Pc,   ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Cpsr, ExpectedValue32);
    }

    if ((ContextFlags & ARM32_CONTEXT_INTEGER) == ARM32_CONTEXT_INTEGER)
    {
        ok_eq_hex_(__FILE__, Line, Context->R0,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R1,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R2,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R3,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R4,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R5,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R6,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R7,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R8,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R9,  ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R10, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R11, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->R12, ExpectedValue32);
    }

    if ((ContextFlags & ARM32_CONTEXT_FLOATING_POINT) == ARM32_CONTEXT_FLOATING_POINT)
    {
        ok_eq_hex_(__FILE__, Line, Context->Fpscr, ExpectedValue32);
        MatchingBytes = EqualBytes(&Context->Q, sizeof(Context->Q), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Q));
    }

    if ((ContextFlags & ARM32_CONTEXT_DEBUG_REGISTERS) == ARM32_CONTEXT_DEBUG_REGISTERS)
    {
        MatchingBytes = EqualBytes(&Context->Bvr, sizeof(Context->Bvr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Bvr));
        MatchingBytes = EqualBytes(&Context->Bcr, sizeof(Context->Bcr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Bcr));
        MatchingBytes = EqualBytes(&Context->Wvr, sizeof(Context->Wvr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Wvr));
        MatchingBytes = EqualBytes(&Context->Wcr, sizeof(Context->Wcr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Wcr));
    }
}

#define CheckContext_ARM32(Context, ContextFlags, ExpectedValue) \
    CheckContext_ARM32_(__LINE__, Context, ContextFlags, ExpectedValue)

static void CheckContextAll_ARM32_(
    ULONG Line,
    const ARM32_CONTEXT* Context,
    UCHAR Expected_CONTROL,
    UCHAR Expected_INTEGER,
    UCHAR Expected_FLOATING_POINT,
    UCHAR Expected_DEBUG_REGISTERS)
{
    CheckContext_ARM32_(Line, Context, ARM32_CONTEXT_CONTROL, Expected_CONTROL);
    CheckContext_ARM32_(Line, Context, ARM32_CONTEXT_INTEGER, Expected_INTEGER);
    CheckContext_ARM32_(Line, Context, ARM32_CONTEXT_FLOATING_POINT, Expected_FLOATING_POINT);
    CheckContext_ARM32_(Line, Context, ARM32_CONTEXT_DEBUG_REGISTERS, Expected_DEBUG_REGISTERS);

    /* These are never modified */
    ok_eq_hex_(__FILE__, Line, Context->Padding, 0);
    ok_eq_hex_(__FILE__, Line, Context->Padding2[0], 0);
    ok_eq_hex_(__FILE__, Line, Context->Padding2[1], 0);
}

#define CheckContextAll_ARM32(Context, Expected_CONTROL, Expected_INTEGER, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS) \
    CheckContextAll_ARM32_(__LINE__, Context, Expected_CONTROL, Expected_INTEGER, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS)

static void Test_RtlCopyContext_ARM32(void)
{
    ARM32_CONTEXT SrcContext = { 0 };
    ARM32_CONTEXT DstContext = { 0 };
    NTSTATUS Status, ExpectedStatus;

    /* Try to copy an empty context */
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_ARM32 in the source, but not in the destination */
    SrcContext.ContextFlags = CONTEXT_ARM32;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_ARM32 in both source and destination */
    DstContext.ContextFlags = CONTEXT_ARM32;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test invalid ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32 | TestFlag, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM32_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid source ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        SrcContext.ContextFlags = CONTEXT_ARM32 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM32_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid destination ContextFlags */
    SrcContext.ContextFlags = CONTEXT_ARM32;
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        DstContext.ContextFlags = CONTEXT_ARM32 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM32_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Copy with only CONTEXT_ARM32 (should copy no contents) */
    RtlFillMemory(&SrcContext, sizeof(SrcContext), 0xAA);
    SrcContext.ContextFlags = CONTEXT_ARM32;
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    DstContext.ContextFlags = CONTEXT_ARM32;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM32, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0x00, 0x00, 0x00, 0x00);

    /* Copy ARM32_CONTEXT_CONTROL, but source doesn't have it */
    Status = pRtlCopyContext(&DstContext, ARM32_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0x00, 0x00, 0x00, 0x00);

    /* Copy ARM32_CONTEXT_CONTROL */
    SrcContext.ContextFlags = ARM32_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, ARM32_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0xAA, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM32_CONTEXT_CONTROL);

    /* Copy ARM32_CONTEXT_INTEGER onto dest with ARM32_CONTEXT_CONTROL */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM32_CONTEXT_INTEGER;
    DstContext.ContextFlags = ARM32_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, ARM32_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0x00, 0xAA, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM32_CONTEXT_CONTROL | ARM32_CONTEXT_INTEGER);

    /* Copy ARM32_CONTEXT_FLOATING_POINT onto dest with ARM32_CONTEXT_DEBUG_REGISTERS */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM32_CONTEXT_ALL;
    DstContext.ContextFlags = ARM32_CONTEXT_DEBUG_REGISTERS;
    Status = pRtlCopyContext(&DstContext, ARM32_CONTEXT_FLOATING_POINT, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0x00, 0x00, 0xAA, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM32_CONTEXT_DEBUG_REGISTERS | ARM32_CONTEXT_FLOATING_POINT);

    /* Copy ARM32_CONTEXT_DEBUG_REGISTERS onto dest with ARM32_CONTEXT_INTEGER */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM32_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = ARM32_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, ARM32_CONTEXT_ALL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM32(&DstContext, 0x00, 0x00, 0x00, 0xAA);
    ok_eq_hex(DstContext.ContextFlags, ARM32_CONTEXT_INTEGER | ARM32_CONTEXT_DEBUG_REGISTERS);
}

static void CheckContext_ARM64_(
    ULONG Line,
    const ARM64_CONTEXT* Context,
    ULONG ContextFlags,
    UCHAR ExpectedValue)
{
    ULONG64 ExpectedValue64 = ExpectedValue * 0x0101010101010101ULL;
    ULONG ExpectedValue32 = ExpectedValue * 0x01010101;
    ULONG MatchingBytes;

    if ((ContextFlags & ARM64_CONTEXT_CONTROL) == ARM64_CONTEXT_CONTROL)
    {
        ok_eq_hex_(__FILE__, Line, Context->Cpsr, ExpectedValue32);
        ok_eq_hex64_(__FILE__, Line, Context->Sp, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Pc, ExpectedValue64);
    }

    if ((ContextFlags & ARM64_CONTEXT_INTEGER) == ARM64_CONTEXT_INTEGER)
    {
        ok_eq_hex64_(__FILE__, Line, Context->X[0],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[1],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[2],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[3],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[4],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[5],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[6],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[7],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[8],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[9],  ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[10], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[11], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[12], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[13], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[14], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[15], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[16], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[17], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[19], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[20], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[21], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[22], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[23], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[24], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[25], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[26], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[27], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->X[28], ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Fp, ExpectedValue64);
        ok_eq_hex64_(__FILE__, Line, Context->Lr, ExpectedValue64);
    }

    if ((ContextFlags & ARM64_CONTEXT_X18) == ARM64_CONTEXT_X18)
    {
        ok_eq_hex64_(__FILE__, Line, Context->X[18], ExpectedValue64);
    }

    if ((ContextFlags & ARM64_CONTEXT_FLOATING_POINT) == ARM64_CONTEXT_FLOATING_POINT)
    {
        ok_eq_hex_(__FILE__, Line, Context->Fpcr, ExpectedValue32);
        ok_eq_hex_(__FILE__, Line, Context->Fpsr, ExpectedValue32);
        MatchingBytes = EqualBytes(&Context->V, sizeof(Context->V), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->V));
    }

    if ((ContextFlags & ARM64_CONTEXT_DEBUG_REGISTERS) == ARM64_CONTEXT_DEBUG_REGISTERS)
    {
        MatchingBytes = EqualBytes(&Context->Bcr, sizeof(Context->Bcr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Bcr));
        MatchingBytes = EqualBytes(&Context->Bvr, sizeof(Context->Bvr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Bvr));
        MatchingBytes = EqualBytes(&Context->Wcr, sizeof(Context->Wcr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Wcr));
        MatchingBytes = EqualBytes(&Context->Wvr, sizeof(Context->Wvr), 0, ExpectedValue);
        ok_eq_hex_(__FILE__, Line, MatchingBytes, sizeof(Context->Wvr));
    }
}

#define CheckContext_ARM64(Context, ContextFlags, ExpectedValue) \
    CheckContext_ARM64_(__LINE__, Context, ContextFlags, ExpectedValue)

static void CheckContextAll_ARM64_(
    ULONG Line,
    const ARM64_CONTEXT* Context,
    UCHAR Expected_CONTROL,
    UCHAR Expected_INTEGER,
    UCHAR Expected_FLOATING_POINT,
    UCHAR Expected_DEBUG_REGISTERS,
    UCHAR Expected_X18)
{
    CheckContext_ARM64_(Line, Context, ARM64_CONTEXT_CONTROL, Expected_CONTROL);
    CheckContext_ARM64_(Line, Context, ARM64_CONTEXT_INTEGER, Expected_INTEGER);
    CheckContext_ARM64_(Line, Context, ARM64_CONTEXT_FLOATING_POINT, Expected_FLOATING_POINT);
    CheckContext_ARM64_(Line, Context, ARM64_CONTEXT_DEBUG_REGISTERS, Expected_DEBUG_REGISTERS);
    CheckContext_ARM64_(Line, Context, ARM64_CONTEXT_X18, Expected_X18);
}

#define CheckContextAll_ARM64(Context, Expected_CONTROL, Expected_INTEGER, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS, Expected_X18) \
    CheckContextAll_ARM64_(__LINE__, Context, Expected_CONTROL, Expected_INTEGER, Expected_FLOATING_POINT, Expected_DEBUG_REGISTERS, Expected_X18)

static void Test_RtlCopyContext_ARM64(void)
{
    ARM64_CONTEXT SrcContext = { 0 };
    ARM64_CONTEXT DstContext = { 0 };
    NTSTATUS Status, ExpectedStatus;

    /* Try to copy an empty context */
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_ARM64 in the source, but not in the destination */
    SrcContext.ContextFlags = CONTEXT_ARM64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Set CONTEXT_ARM64 in both source and destination */
    DstContext.ContextFlags = CONTEXT_ARM64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test invalid ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64 | TestFlag, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid source ContextFlags */
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        SrcContext.ContextFlags = CONTEXT_ARM64 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Test invalid destination ContextFlags */
    SrcContext.ContextFlags = CONTEXT_ARM64;
    for (ULONG i = 0; i < 32; i++)
    {
        ULONG TestFlag = 1 << i;
        DstContext.ContextFlags = CONTEXT_ARM64 | TestFlag;
        Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
        ExpectedStatus = (TestFlag & ~ARM64_CONTEXT_ALLOWED_FLAGS) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        ok(Status == ExpectedStatus, "TestFlag 0x%lx: Expected %08x, got %08x\n", TestFlag, ExpectedStatus, Status);
    }

    /* Copy with only CONTEXT_ARM64 (should copy no contents) */
    RtlFillMemory(&SrcContext, sizeof(SrcContext), 0xAA);
    SrcContext.ContextFlags = CONTEXT_ARM64;
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    DstContext.ContextFlags = CONTEXT_ARM64;
    Status = pRtlCopyContext(&DstContext, CONTEXT_ARM64, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy ARM64_CONTEXT_CONTROL, but source doesn't have it */
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0x00);

    /* Copy ARM64_CONTEXT_CONTROL */
    SrcContext.ContextFlags = ARM64_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_CONTROL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0xAA, 0x00, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM64_CONTEXT_CONTROL);

    /* Copy ARM64_CONTEXT_INTEGER onto dest with ARM64_CONTEXT_CONTROL */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM64_CONTEXT_INTEGER;
    DstContext.ContextFlags = ARM64_CONTEXT_CONTROL;
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_INTEGER, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0xAA, 0x00, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM64_CONTEXT_CONTROL | ARM64_CONTEXT_INTEGER);

    /* Copy ARM64_CONTEXT_FLOATING_POINT onto dest with ARM64_CONTEXT_DEBUG_REGISTERS */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM64_CONTEXT_ALL;
    DstContext.ContextFlags = ARM64_CONTEXT_DEBUG_REGISTERS;
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_FLOATING_POINT, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0x00, 0xAA, 0x00, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM64_CONTEXT_DEBUG_REGISTERS | ARM64_CONTEXT_FLOATING_POINT);

    /* Copy ARM64_CONTEXT_DEBUG_REGISTERS onto dest with ARM64_CONTEXT_INTEGER */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM64_CONTEXT_DEBUG_REGISTERS;
    DstContext.ContextFlags = ARM64_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_ALL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0x00, 0x00, 0xAA, 0x00);
    ok_eq_hex(DstContext.ContextFlags, ARM64_CONTEXT_INTEGER | ARM64_CONTEXT_DEBUG_REGISTERS);

    /* Copy ARM64_CONTEXT_X18 onto dest with ARM64_CONTEXT_INTEGER */
    RtlFillMemory(&DstContext, sizeof(DstContext), 0x00);
    SrcContext.ContextFlags = ARM64_CONTEXT_X18;
    DstContext.ContextFlags = ARM64_CONTEXT_INTEGER;
    Status = pRtlCopyContext(&DstContext, ARM64_CONTEXT_ALL, &SrcContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckContextAll_ARM64(&DstContext, 0x00, 0x00, 0x00, 0x00, 0xAA);
    ok_eq_hex(DstContext.ContextFlags, ARM64_CONTEXT_INTEGER | ARM64_CONTEXT_X18);
}


START_TEST(RtlCopyContext)
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    pRtlCopyContext = (FN_RtlCopyContext*)GetProcAddress(hNtdll, "RtlCopyContext");
    if (!pRtlCopyContext)
    {
        HMODULE hNtdll_vista = LoadLibraryA("ntdll_vista.dll");
        pRtlCopyContext = (FN_RtlCopyContext*)GetProcAddress(hNtdll_vista, "RtlCopyContext");
    }

    if (!pRtlCopyContext)
    {
        trace("RtlCopyContext not found in ntdll.dll or ntdll_vista.dll\n");
        return;
    }

    /* Test no architecture */
    CONTEXT SrcContext = { .ContextFlags = 0 };
    CONTEXT DstContext = { .ContextFlags = 0 };
    NTSTATUS Status = pRtlCopyContext(&DstContext, 0, &SrcContext);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    Test_RtlCopyContext_I386();
    Test_RtlCopyContext_AMD64();
    Test_RtlCopyContext_ARM32();
    Test_RtlCopyContext_ARM64();
}
