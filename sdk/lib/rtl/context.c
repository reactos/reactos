/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Functions related to contexts
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

VOID
NTAPI
RtlpCopyContextI386Internal(
    _Inout_ PI386_CONTEXT DstContext,
    _In_ ULONG CopyFlags,
    _In_ const I386_CONTEXT* SrcContext)
{
    CopyFlags &= ~CONTEXT_i386;
    if (CopyFlags & I386_CONTEXT_CONTROL)
    {
        DstContext->Eip = SrcContext->Eip;
        DstContext->EFlags = SrcContext->EFlags;
        DstContext->SegCs = SrcContext->SegCs;
        DstContext->SegSs = SrcContext->SegSs;
        DstContext->Esp = SrcContext->Esp;
        DstContext->Ebp = SrcContext->Ebp;
    }

    if (CopyFlags & I386_CONTEXT_INTEGER)
    {
        DstContext->Eax = SrcContext->Eax;
        DstContext->Ebx = SrcContext->Ebx;
        DstContext->Ecx = SrcContext->Ecx;
        DstContext->Edx = SrcContext->Edx;
        DstContext->Esi = SrcContext->Esi;
        DstContext->Edi = SrcContext->Edi;
    }

    if (CopyFlags & I386_CONTEXT_SEGMENTS)
    {
        DstContext->SegDs = SrcContext->SegDs;
        DstContext->SegEs = SrcContext->SegEs;
        DstContext->SegFs = SrcContext->SegFs;
        DstContext->SegGs = SrcContext->SegGs;
    }

    if (CopyFlags & I386_CONTEXT_DEBUG_REGISTERS)
    {
        DstContext->Dr0 = SrcContext->Dr0;
        DstContext->Dr1 = SrcContext->Dr1;
        DstContext->Dr2 = SrcContext->Dr2;
        DstContext->Dr3 = SrcContext->Dr3;
        DstContext->Dr6 = SrcContext->Dr6;
        DstContext->Dr7 = SrcContext->Dr7;
    }

    if (CopyFlags & I386_CONTEXT_FLOATING_POINT)
    {
        DstContext->FloatSave = SrcContext->FloatSave;
    }

    if (CopyFlags & I386_CONTEXT_EXTENDED_REGISTERS)
    {
        RtlCopyMemory(DstContext->ExtendedRegisters,
                      SrcContext->ExtendedRegisters,
                      0x120);
    }
}

NTSTATUS
NTAPI
RtlpCopyContextI386(
    _Inout_ PI386_CONTEXT DstContext,
    _In_ ULONG ContextFlags,
    _In_ const I386_CONTEXT* SrcContext)
{
    ASSERT((ContextFlags & CONTEXT_i386) == CONTEXT_i386);

    /* Make sure the source and dest context have the correct architecture */
    if ((SrcContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_i386 ||
        (DstContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_i386)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate flags for i386 context */
    ULONG AllContextFlags = ContextFlags | SrcContext->ContextFlags | DstContext->ContextFlags;
    if (AllContextFlags & ~I386_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG CopyFlags = ContextFlags & SrcContext->ContextFlags & ~CONTEXT_i386;
    RtlpCopyContextI386Internal(DstContext, CopyFlags, SrcContext);

    DstContext->ContextFlags |= CopyFlags;
    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpCopyContextAmd64Internal(
    _Inout_ PAMD64_CONTEXT DstContext,
    _In_ ULONG CopyFlags,
    _In_ const AMD64_CONTEXT* SrcContext)
{
    CopyFlags &= ~CONTEXT_AMD64;
    if (CopyFlags & AMD64_CONTEXT_CONTROL)
    {
        DstContext->Rsp = SrcContext->Rsp;
        DstContext->Rip = SrcContext->Rip;
        DstContext->EFlags = SrcContext->EFlags;
        DstContext->SegCs = SrcContext->SegCs;
        DstContext->SegSs = SrcContext->SegSs;
    }

    if (CopyFlags & AMD64_CONTEXT_INTEGER)
    {
        DstContext->Rax = SrcContext->Rax;
        DstContext->Rcx = SrcContext->Rcx;
        DstContext->Rdx = SrcContext->Rdx;
        DstContext->Rbx = SrcContext->Rbx;
        DstContext->Rbp = SrcContext->Rbp;
        DstContext->Rsi = SrcContext->Rsi;
        DstContext->Rdi = SrcContext->Rdi;
        DstContext->R8  = SrcContext->R8;
        DstContext->R9  = SrcContext->R9;
        DstContext->R10 = SrcContext->R10;
        DstContext->R11 = SrcContext->R11;
        DstContext->R12 = SrcContext->R12;
        DstContext->R13 = SrcContext->R13;
        DstContext->R14 = SrcContext->R14;
        DstContext->R15 = SrcContext->R15;
    }

    if (CopyFlags & AMD64_CONTEXT_SEGMENTS)
    {
        DstContext->SegDs = SrcContext->SegDs;
        DstContext->SegEs = SrcContext->SegEs;
        DstContext->SegFs = SrcContext->SegFs;
        DstContext->SegGs = SrcContext->SegGs;
    }

    if (CopyFlags & AMD64_CONTEXT_FLOATING_POINT)
    {
        DstContext->MxCsr = SrcContext->MxCsr;
        RtlCopyMemory(&DstContext->FltSave,
                      &SrcContext->FltSave,
                      FIELD_OFFSET(XSAVE_FORMAT64, Reserved4));
    }

    if (CopyFlags & AMD64_CONTEXT_DEBUG_REGISTERS)
    {
        DstContext->Dr0 = SrcContext->Dr0;
        DstContext->Dr1 = SrcContext->Dr1;
        DstContext->Dr2 = SrcContext->Dr2;
        DstContext->Dr3 = SrcContext->Dr3;
        DstContext->Dr6 = SrcContext->Dr6;
        DstContext->Dr7 = SrcContext->Dr7;
        DstContext->LastBranchToRip = SrcContext->LastBranchToRip;
        DstContext->LastBranchFromRip = SrcContext->LastBranchFromRip;
        DstContext->LastExceptionToRip = SrcContext->LastExceptionToRip;
        DstContext->LastExceptionFromRip = SrcContext->LastExceptionFromRip;
    }
}

NTSTATUS
NTAPI
RtlpCopyContextAmd64(
    _Inout_ PAMD64_CONTEXT DstContext,
    _In_ ULONG ContextFlags,
    _In_ const AMD64_CONTEXT* SrcContext)
{
    ASSERT((ContextFlags & CONTEXT_AMD64) == CONTEXT_AMD64);

    /* Make sure the source and dest context have the correct architecture */
    if ((SrcContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_AMD64 ||
        (DstContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_AMD64)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate flags for AMD64 context */
    ULONG AllContextFlags = ContextFlags | SrcContext->ContextFlags | DstContext->ContextFlags;
    if (AllContextFlags & ~AMD64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG CopyFlags = ContextFlags & SrcContext->ContextFlags & ~CONTEXT_AMD64;
    RtlpCopyContextAmd64Internal(DstContext, CopyFlags, SrcContext);

    DstContext->ContextFlags |= CopyFlags;
    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpCopyContextArm32Internal(
    _Inout_ PARM32_CONTEXT DstContext,
    _In_ ULONG CopyFlags,
    _In_ const ARM32_CONTEXT* SrcContext)
{
    CopyFlags &= ~CONTEXT_ARM32;
    if (CopyFlags & ARM32_CONTEXT_CONTROL)
    {
        DstContext->Sp = SrcContext->Sp;
        DstContext->Lr = SrcContext->Lr;
        DstContext->Pc = SrcContext->Pc;
        DstContext->Cpsr = SrcContext->Cpsr;
    }

    if (CopyFlags & ARM32_CONTEXT_INTEGER)
    {
        DstContext->R0 = SrcContext->R0;
        DstContext->R1 = SrcContext->R1;
        DstContext->R2 = SrcContext->R2;
        DstContext->R3 = SrcContext->R3;
        DstContext->R4 = SrcContext->R4;
        DstContext->R5 = SrcContext->R5;
        DstContext->R6 = SrcContext->R6;
        DstContext->R7 = SrcContext->R7;
        DstContext->R8 = SrcContext->R8;
        DstContext->R9 = SrcContext->R9;
        DstContext->R10 = SrcContext->R10;
        DstContext->R11 = SrcContext->R11;
        DstContext->R12 = SrcContext->R12;
    }

    if (CopyFlags & ARM32_CONTEXT_FLOATING_POINT)
    {
        DstContext->Fpscr = SrcContext->Fpscr;
        RtlCopyMemory(&DstContext->Q, &SrcContext->Q, sizeof(DstContext->Q));
    }

    if (CopyFlags & ARM32_CONTEXT_DEBUG_REGISTERS)
    {
        RtlCopyMemory(&DstContext->Bvr, &SrcContext->Bvr, sizeof(DstContext->Bvr));
        RtlCopyMemory(&DstContext->Bcr, &SrcContext->Bcr, sizeof(DstContext->Bcr));
        RtlCopyMemory(&DstContext->Wvr, &SrcContext->Wvr, sizeof(DstContext->Wvr));
        RtlCopyMemory(&DstContext->Wcr, &SrcContext->Wcr, sizeof(DstContext->Wcr));
    }
}

NTSTATUS
NTAPI
RtlpCopyContextArm32(
    _Inout_ PARM32_CONTEXT DstContext,
    _In_ ULONG ContextFlags,
    _In_ const ARM32_CONTEXT* SrcContext)
{
    ASSERT((ContextFlags & CONTEXT_ARM32) == CONTEXT_ARM32);

    /* Make sure the source and dest context have the correct architecture */
    if ((SrcContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_ARM32 ||
        (DstContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_ARM32)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate flags for ARM32 context */
    ULONG AllContextFlags = ContextFlags | SrcContext->ContextFlags | DstContext->ContextFlags;
    if (AllContextFlags & ~ARM32_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG CopyFlags = ContextFlags & SrcContext->ContextFlags & ~CONTEXT_ARM32;
    RtlpCopyContextArm32Internal(DstContext, CopyFlags, SrcContext);

    DstContext->ContextFlags |= CopyFlags;
    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpCopyContextArm64Internal(
    _Inout_ PARM64_CONTEXT DstContext,
    _In_ ULONG CopyFlags,
    _In_ const ARM64_CONTEXT* SrcContext)
{
    CopyFlags &= ~CONTEXT_ARM64;
    if (CopyFlags & ARM64_CONTEXT_CONTROL)
    {
        DstContext->Cpsr = SrcContext->Cpsr;
        DstContext->Sp = SrcContext->Sp;
        DstContext->Pc = SrcContext->Pc;
    }

    if (CopyFlags & ARM64_CONTEXT_INTEGER)
    {
        /* Copy X0-X17 and X19-X30. X18 is handled separately. */
        RtlCopyMemory(&DstContext->X[0], &SrcContext->X[0], sizeof(DstContext->X[0]) * 18);
        RtlCopyMemory(&DstContext->X[19], &SrcContext->X[19], sizeof(DstContext->X[0]) * 12);
    }

    if (CopyFlags & ARM64_CONTEXT_X18)
    {
        DstContext->X[18] = SrcContext->X[18];
    }

    if (CopyFlags & ARM64_CONTEXT_FLOATING_POINT)
    {
        DstContext->Fpcr = SrcContext->Fpcr;
        DstContext->Fpsr = SrcContext->Fpsr;
        RtlCopyMemory(&DstContext->V, &SrcContext->V, sizeof(DstContext->V));
    }

    if (CopyFlags & ARM64_CONTEXT_DEBUG_REGISTERS)
    {
        RtlCopyMemory(&DstContext->Bcr, &SrcContext->Bcr, sizeof(DstContext->Bcr));
        RtlCopyMemory(&DstContext->Bvr, &SrcContext->Bvr, sizeof(DstContext->Bvr));
        RtlCopyMemory(&DstContext->Wcr, &SrcContext->Wcr, sizeof(DstContext->Wcr));
        RtlCopyMemory(&DstContext->Wvr, &SrcContext->Wvr, sizeof(DstContext->Wvr));
    }
}

NTSTATUS
NTAPI
RtlpCopyContextArm64(
    _Inout_ PARM64_CONTEXT DstContext,
    _In_ ULONG ContextFlags,
    _In_ const ARM64_CONTEXT* SrcContext)
{
    ASSERT((ContextFlags & CONTEXT_ARM64) == CONTEXT_ARM64);

    /* Make sure the source and dest context have the correct architecture */
    if ((SrcContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_ARM64 ||
        (DstContext->ContextFlags & CONTEXT_ARCHITECTURE_MASK) != CONTEXT_ARM64)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate flags for ARM64 context */
    ULONG AllContextFlags = ContextFlags | SrcContext->ContextFlags | DstContext->ContextFlags;
    if (AllContextFlags & ~ARM64_CONTEXT_ALLOWED_FLAGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG CopyFlags = ContextFlags & SrcContext->ContextFlags & ~CONTEXT_ARM64;
    RtlpCopyContextArm64Internal(DstContext, CopyFlags, SrcContext);
    
    DstContext->ContextFlags |= CopyFlags;
    return STATUS_SUCCESS;
}

/*!
* \@brief Copies the specified parts of a context from a source to a destination context.
*
* \param DstContext - A pointer to the destination context.
*     The ContextFlags field of this context must be set to the same value as ContextFlags parameter.
* 
* \param ContextFlags - A bitmask specifying which parts of the context to copy.
*     This must be a combination of CONTEXT_i386, CONTEXT_AMD64, CONTEXT_ARM32 or CONTEXT_ARM64
*     and the relevant flags for the architecture.
* 
* \param SrcContext - A pointer to the source context.
*     The ContextFlags field of this context must be set to the same value as ContextFlags parameter.
* 
* \return STATUS_SUCCESS if the context was copied successfully,
*     or STATUS_INVALID_PARAMETER if the parameters were invalid.
*/
NTSTATUS
NTAPI
RtlCopyContext(
    _Inout_ PVOID DstContext,
    _In_ ULONG ContextFlags,
    _In_ const VOID* SrcContext)
{
    ULONG Architecture;

    Architecture = ContextFlags & CONTEXT_ARCHITECTURE_MASK;
    switch (Architecture)
    {
        case CONTEXT_i386:
            return RtlpCopyContextI386(DstContext, ContextFlags, SrcContext);

        case CONTEXT_AMD64:
            return RtlpCopyContextAmd64(DstContext, ContextFlags, SrcContext);

        case CONTEXT_ARM32:
            return RtlpCopyContextArm32(DstContext, ContextFlags, SrcContext);

        case CONTEXT_ARM64:
            return RtlpCopyContextArm64(DstContext, ContextFlags, SrcContext);

        default:
            return STATUS_INVALID_PARAMETER;
    }
}
