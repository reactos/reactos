/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         CONTEXT related functions
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
    KIRQL OldIrql;

    /* Make sure we have an amd64 context, then remove the flag */
    ASSERT(ContextFlags & CONTEXT_AMD64);
    ContextFlags &= ~CONTEXT_AMD64;

    /* Do this at APC_LEVEL */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Handle integer registers */
    if (ContextFlags & CONTEXT_INTEGER)
    {
        TrapFrame->Rax = Context->Rax;
        TrapFrame->Rcx = Context->Rcx;
        TrapFrame->Rdx = Context->Rdx;
        TrapFrame->Rbp = Context->Rbp;
        TrapFrame->R8 = Context->R8;
        TrapFrame->R9 = Context->R9;
        TrapFrame->R10 = Context->R10;
        TrapFrame->R11 = Context->R11;
        if (ExceptionFrame)
        {
            ExceptionFrame->Rbx = Context->Rbx;
            ExceptionFrame->Rsi = Context->Rsi;
            ExceptionFrame->Rdi = Context->Rdi;
            ExceptionFrame->R12 = Context->R12;
            ExceptionFrame->R13 = Context->R13;
            ExceptionFrame->R14 = Context->R14;
            ExceptionFrame->R15 = Context->R15;
        }
    }

    /* Handle floating point registers */
    if (ContextFlags & CONTEXT_FLOATING_POINT)
    {
        TrapFrame->MxCsr = Context->MxCsr;
        TrapFrame->Xmm0 = Context->Xmm0;
        TrapFrame->Xmm1 = Context->Xmm1;
        TrapFrame->Xmm2 = Context->Xmm2;
        TrapFrame->Xmm3 = Context->Xmm3;
        TrapFrame->Xmm4 = Context->Xmm4;
        TrapFrame->Xmm5 = Context->Xmm5;
        if (ExceptionFrame)
        {
            ExceptionFrame->Xmm6 = Context->Xmm6;
            ExceptionFrame->Xmm7 = Context->Xmm7;
            ExceptionFrame->Xmm8 = Context->Xmm8;
            ExceptionFrame->Xmm9 = Context->Xmm9;
            ExceptionFrame->Xmm10 = Context->Xmm10;
            ExceptionFrame->Xmm11 = Context->Xmm11;
            ExceptionFrame->Xmm12 = Context->Xmm12;
            ExceptionFrame->Xmm13 = Context->Xmm13;
            ExceptionFrame->Xmm14 = Context->Xmm14;
            ExceptionFrame->Xmm15 = Context->Xmm15;
        }
    }

    /* Handle control registers */
    if (ContextFlags & CONTEXT_CONTROL)
    {
        /* RIP, RSP, EFLAGS */
        TrapFrame->Rip = Context->Rip;
        TrapFrame->Rsp = Context->Rsp;
        TrapFrame->EFlags = Context->EFlags;

        if ((Context->SegCs & MODE_MASK) == KernelMode)
        {
            /* Set valid selectors */
            TrapFrame->SegCs = KGDT64_R0_CODE;
            TrapFrame->SegSs = KGDT64_R0_DATA;

            /* Set valid EFLAGS */
            TrapFrame->EFlags &= (EFLAGS_USER_SANITIZE | EFLAGS_INTERRUPT_MASK);
        }
        else
        {
            /* Copy selectors */
            TrapFrame->SegCs = Context->SegCs;
            if (TrapFrame->SegCs != (KGDT64_R3_CODE | RPL_MASK))
            {
                TrapFrame->SegCs = (KGDT64_R3_CMCODE | RPL_MASK);
            }

            TrapFrame->SegSs = Context->SegSs;

            /* Set valid EFLAGS */
            TrapFrame->EFlags &= EFLAGS_USER_SANITIZE;
            TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;
        }
    }

    /* Handle segment selectors */
    if (ContextFlags & CONTEXT_SEGMENTS)
    {
        /* Check if this was a Kernel Trap */
        if ((Context->SegCs & MODE_MASK) == KernelMode)
        {
            /* Set valid selectors */
            TrapFrame->SegDs = KGDT64_R3_DATA | RPL_MASK;
            TrapFrame->SegEs = KGDT64_R3_DATA | RPL_MASK;
            TrapFrame->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
            TrapFrame->SegGs = KGDT64_R3_DATA | RPL_MASK;
        }
        else
        {
            /* Copy selectors */
            TrapFrame->SegDs = Context->SegDs;
            TrapFrame->SegEs = Context->SegEs;
            TrapFrame->SegFs = Context->SegFs;
            TrapFrame->SegGs = Context->SegGs;
        }
    }

    /* Handle debug registers */
    if (ContextFlags & CONTEXT_DEBUG_REGISTERS)
    {
        /* Copy the debug registers */
        TrapFrame->Dr0 = Context->Dr0;
        TrapFrame->Dr1 = Context->Dr1;
        TrapFrame->Dr2 = Context->Dr2;
        TrapFrame->Dr3 = Context->Dr3;
        TrapFrame->Dr6 = Context->Dr6;
        TrapFrame->Dr7 = Context->Dr7;

        if ((Context->SegCs & MODE_MASK) != KernelMode)
        {
            if (TrapFrame->Dr0 > (ULONG64)MmHighestUserAddress)
                TrapFrame->Dr0 = 0;
            if (TrapFrame->Dr1 > (ULONG64)MmHighestUserAddress)
                TrapFrame->Dr1 = 0;
            if (TrapFrame->Dr2 > (ULONG64)MmHighestUserAddress)
                TrapFrame->Dr2 = 0;
            if (TrapFrame->Dr3 > (ULONG64)MmHighestUserAddress)
                TrapFrame->Dr3 = 0;
        }
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
KeTrapFrameToContext(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PCONTEXT Context)
{
    ULONG ContextFlags;
    KIRQL OldIrql;

    /* Do this at APC_LEVEL */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Make sure we have an amd64 context, then remove the flag */
    ContextFlags = Context->ContextFlags;
    ASSERT(ContextFlags & CONTEXT_AMD64);
    ContextFlags &= ~CONTEXT_AMD64;

    /* Handle integer registers */
    if (ContextFlags & CONTEXT_INTEGER)
    {
        Context->Rax = TrapFrame->Rax;
        Context->Rcx = TrapFrame->Rcx;
        Context->Rdx = TrapFrame->Rdx;
        Context->Rbp = TrapFrame->Rbp;
        Context->R8 = TrapFrame->R8;
        Context->R9 = TrapFrame->R9;
        Context->R10 = TrapFrame->R10;
        Context->R11 = TrapFrame->R11;

        if (ExceptionFrame)
        {
            Context->Rbx = ExceptionFrame->Rbx;
            Context->Rsi = ExceptionFrame->Rsi;
            Context->Rdi = ExceptionFrame->Rdi;
            Context->R12 = ExceptionFrame->R12;
            Context->R13 = ExceptionFrame->R13;
            Context->R14 = ExceptionFrame->R14;
            Context->R15 = ExceptionFrame->R15;
        }
    }

    /* Handle floating point registers */
    if (ContextFlags & CONTEXT_FLOATING_POINT)
    {
        Context->MxCsr = TrapFrame->MxCsr;
        Context->Xmm0 = TrapFrame->Xmm0;
        Context->Xmm1 = TrapFrame->Xmm1;
        Context->Xmm2 = TrapFrame->Xmm2;
        Context->Xmm3 = TrapFrame->Xmm3;
        Context->Xmm4 = TrapFrame->Xmm4;
        Context->Xmm5 = TrapFrame->Xmm5;
        if (ExceptionFrame)
        {
            Context->Xmm6 = ExceptionFrame->Xmm6;
            Context->Xmm7 = ExceptionFrame->Xmm7;
            Context->Xmm8 = ExceptionFrame->Xmm8;
            Context->Xmm9 = ExceptionFrame->Xmm9;
            Context->Xmm10 = ExceptionFrame->Xmm10;
            Context->Xmm11 = ExceptionFrame->Xmm11;
            Context->Xmm12 = ExceptionFrame->Xmm12;
            Context->Xmm13 = ExceptionFrame->Xmm13;
            Context->Xmm14 = ExceptionFrame->Xmm14;
            Context->Xmm15 = ExceptionFrame->Xmm15;
        }
    }

    /* Handle control registers */
    if (ContextFlags & CONTEXT_CONTROL)
    {
        /* Check if this was a Kernel Trap */
        if ((TrapFrame->SegCs & MODE_MASK) == KernelMode)
        {
            /* Set valid selectors */
            Context->SegCs = KGDT64_R0_CODE;
            Context->SegSs = KGDT64_R0_DATA;
        }
        else
        {
            /* Copy selectors */
            Context->SegCs = TrapFrame->SegCs;
            Context->SegSs = TrapFrame->SegSs;
        }

        /* Copy RIP, RSP, EFLAGS */
        Context->Rip = TrapFrame->Rip;
        Context->Rsp = TrapFrame->Rsp;
        Context->EFlags = TrapFrame->EFlags;
    }

    /* Handle segment selectors */
    if (ContextFlags & CONTEXT_SEGMENTS)
    {
        /* Check if this was a Kernel Trap */
        if ((TrapFrame->SegCs & MODE_MASK) == KernelMode)
        {
            /* Set valid selectors */
            Context->SegDs = KGDT64_R3_DATA | RPL_MASK;
            Context->SegEs = KGDT64_R3_DATA | RPL_MASK;
            Context->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
            Context->SegGs = KGDT64_R3_DATA | RPL_MASK;
        }
        else
        {
            /* Copy selectors */
            Context->SegDs = TrapFrame->SegDs;
            Context->SegEs = TrapFrame->SegEs;
            Context->SegFs = TrapFrame->SegFs;
            Context->SegGs = TrapFrame->SegGs;
        }
    }

    /* Handle debug registers */
    if (ContextFlags & CONTEXT_DEBUG_REGISTERS)
    {
        /* Copy the debug registers */
        Context->Dr0 = TrapFrame->Dr0;
        Context->Dr1 = TrapFrame->Dr1;
        Context->Dr2 = TrapFrame->Dr2;
        Context->Dr3 = TrapFrame->Dr3;
        Context->Dr6 = TrapFrame->Dr6;
        Context->Dr7 = TrapFrame->Dr7;
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
RtlSetUnwindContext(
    _In_ PCONTEXT Context,
    _In_ DWORD64 TargetFrame);

VOID
KiSetTrapContextInternal(
    _Out_ PKTRAP_FRAME TrapFrame,
    _In_ PCONTEXT Context,
    _In_ KPROCESSOR_MODE RequestorMode)
{
    ULONG64 TargetFrame;

    /* Save the volatile register context in the trap frame */
    KeContextToTrapFrame(Context,
                         NULL,
                         TrapFrame,
                         Context->ContextFlags,
                         RequestorMode);

    /* The target frame is MAX_SYSCALL_PARAM_SIZE bytes before the trap frame */
    TargetFrame = (ULONG64)TrapFrame - MAX_SYSCALL_PARAM_SIZE ;

    /* Set the nonvolatiles on the stack */
    RtlSetUnwindContext(Context, TargetFrame);
}
