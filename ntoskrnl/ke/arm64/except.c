/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Exception and Trap Handler C routines
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* ARM64 Exception Syndrome Register (ESR) definitions */
#define ESR_ELx_EC_SHIFT        26
#define ESR_ELx_EC_MASK         (0x3F << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)         (((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)

/* Exception Classes */
#define ESR_ELx_EC_UNKNOWN      0x00
#define ESR_ELx_EC_WFx          0x01    /* WFI/WFE instruction */
#define ESR_ELx_EC_CP15_32      0x03    /* CP15 MCR/MRC (AArch32) */
#define ESR_ELx_EC_CP15_64      0x04    /* CP15 MCRR/MRRC (AArch32) */
#define ESR_ELx_EC_CP14_MR      0x05    /* CP14 MCR/MRC (AArch32) */
#define ESR_ELx_EC_CP14_LS      0x06    /* CP14 LDC/STC (AArch32) */
#define ESR_ELx_EC_FP_ASIMD     0x07    /* FP/ASIMD access */
#define ESR_ELx_EC_CP10_ID      0x08    /* CP10 MCR/MRC (AArch32) */
#define ESR_ELx_EC_PAC          0x09    /* Pointer Authentication */
#define ESR_ELx_EC_CP14_64      0x0C    /* CP14 MCRR/MRRC (AArch32) */
#define ESR_ELx_EC_BTI          0x0D    /* Branch Target Exception */
#define ESR_ELx_EC_ILL          0x0E    /* Illegal Execution State */
#define ESR_ELx_EC_SVC32        0x11    /* SVC instruction (AArch32) */
#define ESR_ELx_EC_HVC32        0x12    /* HVC instruction (AArch32) */
#define ESR_ELx_EC_SMC32        0x13    /* SMC instruction (AArch32) */
#define ESR_ELx_EC_SVC64        0x15    /* SVC instruction (AArch64) */
#define ESR_ELx_EC_HVC64        0x16    /* HVC instruction (AArch64) */
#define ESR_ELx_EC_SMC64        0x17    /* SMC instruction (AArch64) */
#define ESR_ELx_EC_SYS64        0x18    /* System register access */
#define ESR_ELx_EC_SVE          0x19    /* SVE access */
#define ESR_ELx_EC_IMP_DEF      0x1F    /* Implementation defined */
#define ESR_ELx_EC_IABT_LOW     0x20    /* Instruction Abort (lower EL) */
#define ESR_ELx_EC_IABT_CUR     0x21    /* Instruction Abort (current EL) */
#define ESR_ELx_EC_PC_ALIGN     0x22    /* PC alignment fault */
#define ESR_ELx_EC_DABT_LOW     0x24    /* Data Abort (lower EL) */
#define ESR_ELx_EC_DABT_CUR     0x25    /* Data Abort (current EL) */
#define ESR_ELx_EC_SP_ALIGN     0x26    /* SP alignment fault */
#define ESR_ELx_EC_FP_EXC32     0x28    /* FP exception (AArch32) */
#define ESR_ELx_EC_FP_EXC64     0x2C    /* FP exception (AArch64) */
#define ESR_ELx_EC_SERROR       0x2F    /* SError interrupt */
#define ESR_ELx_EC_BREAKPT_LOW  0x30    /* Breakpoint (lower EL) */
#define ESR_ELx_EC_BREAKPT_CUR  0x31    /* Breakpoint (current EL) */
#define ESR_ELx_EC_SOFTSTP_LOW  0x32    /* Software step (lower EL) */
#define ESR_ELx_EC_SOFTSTP_CUR  0x33    /* Software step (current EL) */
#define ESR_ELx_EC_WATCHPT_LOW  0x34    /* Watchpoint (lower EL) */
#define ESR_ELx_EC_WATCHPT_CUR  0x35    /* Watchpoint (current EL) */
#define ESR_ELx_EC_BKPT32       0x38    /* BKPT instruction (AArch32) */
#define ESR_ELx_EC_VECTOR32     0x3A    /* Vector catch (AArch32) */
#define ESR_ELx_EC_BRK64        0x3C    /* BRK instruction (AArch64) */

/* Data Abort ISS fields */
#define ESR_ELx_ISV             (1ULL << 24)
#define ESR_ELx_SAS_SHIFT       22
#define ESR_ELx_SAS_MASK        (3ULL << ESR_ELx_SAS_SHIFT)
#define ESR_ELx_SSE             (1ULL << 21)
#define ESR_ELx_SRT_SHIFT       16
#define ESR_ELx_SRT_MASK        (0x1F << ESR_ELx_SRT_SHIFT)
#define ESR_ELx_SF              (1ULL << 15)
#define ESR_ELx_AR              (1ULL << 14)
#define ESR_ELx_VNCR            (1ULL << 13)
#define ESR_ELx_SET_SHIFT       11
#define ESR_ELx_SET_MASK        (3ULL << ESR_ELx_SET_SHIFT)
#define ESR_ELx_FnV             (1ULL << 10)
#define ESR_ELx_EA              (1ULL << 9)
#define ESR_ELx_S1PTW           (1ULL << 7)
#define ESR_ELx_WnR             (1ULL << 6)
#define ESR_ELx_DFSC_MASK       0x3F

/* GLOBALS *******************************************************************/

/* ARM64 trap frame structure (must match assembly definitions) */
typedef struct _ARM64_TRAP_FRAME
{
    ULONGLONG X0, X1, X2, X3, X4, X5, X6, X7;
    ULONGLONG X8, X9, X10, X11, X12, X13, X14, X15;
    ULONGLONG X16, X17, X18, X19, X20, X21, X22, X23;
    ULONGLONG X24, X25, X26, X27, X28, X29, X30;
    ULONGLONG Sp;       /* Stack pointer */
    ULONGLONG Pc;       /* Program counter (ELR_EL1) */
    ULONGLONG Pstate;   /* Processor state (SPSR_EL1) */
    ULONGLONG Esr;      /* Exception syndrome register */
    ULONGLONG Far;      /* Fault address register */
} ARM64_TRAP_FRAME, *PARM64_TRAP_FRAME;

/* FUNCTIONS *****************************************************************/

/**
 * @brief Handle kernel trap (synchronous exception from kernel mode)
 */
VOID
NTAPI
KiTrapHandlerC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    ULONG ExceptionClass = ESR_ELx_EC(TrapFrame->Esr);
    
    DPRINT("ARM64: Kernel Trap - EC=0x%02X, ESR=0x%llX, FAR=0x%llX, PC=0x%llX\n",
           ExceptionClass, TrapFrame->Esr, TrapFrame->Far, TrapFrame->Pc);
    
    switch (ExceptionClass)
    {
        case ESR_ELx_EC_DABT_CUR:
            /* Data abort in current EL (kernel mode) */
            KiKernelDataAbort(TrapFrame);
            break;
            
        case ESR_ELx_EC_IABT_CUR:
            /* Instruction abort in current EL */
            KiKernelInstructionAbort(TrapFrame);
            break;
            
        case ESR_ELx_EC_PC_ALIGN:
            /* PC alignment fault */
            KiAlignmentFault(TrapFrame);
            break;
            
        case ESR_ELx_EC_SP_ALIGN:
            /* Stack pointer alignment fault */
            KiStackAlignmentFault(TrapFrame);
            break;
            
        case ESR_ELx_EC_BRK64:
            /* Software breakpoint */
            KiBreakpointTrapC(TrapFrame);
            break;
            
        case ESR_ELx_EC_ILL:
            /* Illegal execution state */
            KiIllegalInstruction(TrapFrame);
            break;
            
        default:
            DPRINT1("ARM64: Unhandled kernel trap - EC=0x%02X\n", ExceptionClass);
            KiBugCheck(TrapFrame);
            break;
    }
}

/**
 * @brief Handle interrupt (IRQ)
 */
VOID
NTAPI
KiInterruptHandlerC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: IRQ at PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement GIC interrupt handling */
    /* For now, just acknowledge and return */
}

/**
 * @brief Handle fast interrupt (FIQ)
 */
VOID
NTAPI
KiFiqHandlerC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: FIQ at PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement FIQ handling */
}

/**
 * @brief Handle system error (SError)
 */
VOID
NTAPI
KiSerrorHandlerC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: SError - ESR=0x%llX, FAR=0x%llX, PC=0x%llX\n",
            TrapFrame->Esr, TrapFrame->Far, TrapFrame->Pc);
    
    /* SError is typically fatal */
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle AArch64 system call
 */
VOID
NTAPI
KiSystemCallHandler64C(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    ULONG SystemCallNumber = (ULONG)TrapFrame->X8;
    
    DPRINT("ARM64: System call %u from PC=0x%llX\n", SystemCallNumber, TrapFrame->Pc);
    
    /* TODO: Implement system call dispatch */
    /* For now, return error */
    TrapFrame->X0 = STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Handle AArch32 system call (compatibility)
 */
VOID
NTAPI
KiSystemCallHandler32C(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: AArch32 system call from PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement AArch32 system call compatibility */
    TrapFrame->X0 = STATUS_NOT_SUPPORTED;
}

/**
 * @brief Handle kernel data abort
 */
VOID
NTAPI
KiKernelDataAbort(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    ULONG FaultStatus = TrapFrame->Esr & ESR_ELx_DFSC_MASK;
    BOOLEAN IsWrite = (TrapFrame->Esr & ESR_ELx_WnR) != 0;
    
    DPRINT1("ARM64: Kernel Data Abort - Address=0x%llX, %s, Status=0x%02X\n",
            TrapFrame->Far, IsWrite ? "Write" : "Read", FaultStatus);
    
    /* This is likely fatal in kernel mode */
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle kernel instruction abort
 */
VOID
NTAPI
KiKernelInstructionAbort(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: Kernel Instruction Abort at PC=0x%llX\n", TrapFrame->Pc);
    
    /* This is fatal */
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle alignment fault
 */
VOID
NTAPI
KiAlignmentFault(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: PC Alignment Fault at PC=0x%llX\n", TrapFrame->Pc);
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle stack alignment fault
 */
VOID
NTAPI
KiStackAlignmentFault(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: Stack Alignment Fault - SP=0x%llX, PC=0x%llX\n",
            TrapFrame->Sp, TrapFrame->Pc);
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle illegal instruction
 */
VOID
NTAPI
KiIllegalInstruction(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: Illegal Instruction at PC=0x%llX\n", TrapFrame->Pc);
    KiBugCheck(TrapFrame);
}

/**
 * @brief Handle breakpoint exception
 */
VOID
NTAPI
KiBreakpointTrapC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: Breakpoint at PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement kernel debugger integration */
    /* For now, just continue */
    TrapFrame->Pc += 4;  /* Skip BRK instruction */
}

/**
 * @brief Handle single step exception
 */
VOID
NTAPI
KiSingleStepTrapC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: Single step at PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement single stepping support */
}

/**
 * @brief Handle debug service
 */
VOID
NTAPI
KiDebugServiceC(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT("ARM64: Debug service at PC=0x%llX\n", TrapFrame->Pc);
    
    /* TODO: Implement debug services */
}

/**
 * @brief Handle unexpected interrupt
 */
VOID
NTAPI
KiUnexpectedInterruptC(
    IN ULONGLONG Esr,
    IN ULONGLONG Elr,
    IN ULONGLONG Far
)
{
    DPRINT1("ARM64: Unexpected Interrupt - ESR=0x%llX, ELR=0x%llX, FAR=0x%llX\n",
            Esr, Elr, Far);
}

/**
 * @brief Display unexpected interrupt information
 */
VOID
NTAPI
KiDisplayUnexpectedInterruptC(
    IN ULONG InterruptType
)
{
    DPRINT1("ARM64: Unexpected interrupt type %u\n", InterruptType);
}

/**
 * @brief Thread startup C routine
 */
VOID
NTAPI
KiThreadStartupC(VOID)
{
    /* TODO: Implement thread startup */
    DPRINT("ARM64: Thread startup\n");
}

/**
 * @brief Bug check with trap frame information
 */
VOID
NTAPI
KiBugCheck(
    IN PARM64_TRAP_FRAME TrapFrame
)
{
    DPRINT1("ARM64: KERNEL BUG CHECK\n");
    DPRINT1("PC=0x%llX, SP=0x%llX, PSTATE=0x%llX\n",
            TrapFrame->Pc, TrapFrame->Sp, TrapFrame->Pstate);
    DPRINT1("ESR=0x%llX, FAR=0x%llX\n", TrapFrame->Esr, TrapFrame->Far);
    
    /* Call system bug check */
    KeBugCheckWithoutLog(KERNEL_SECURITY_CHECK_FAILURE);
}