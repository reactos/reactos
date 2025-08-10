/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Exception and Trap Handlers
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <arm64/arm64.h>

/* Exception frame structure (must match KTRAP_FRAME) */
#define TF_X0       0x000
#define TF_X1       0x008
#define TF_X2       0x010
#define TF_X3       0x018
#define TF_X4       0x020
#define TF_X5       0x028
#define TF_X6       0x030
#define TF_X7       0x038
#define TF_X8       0x040
#define TF_X9       0x048
#define TF_X10      0x050
#define TF_X11      0x058
#define TF_X12      0x060
#define TF_X13      0x068
#define TF_X14      0x070
#define TF_X15      0x078
#define TF_X16      0x080
#define TF_X17      0x088
#define TF_X18      0x090
#define TF_X19      0x098
#define TF_X20      0x0A0
#define TF_X21      0x0A8
#define TF_X22      0x0B0
#define TF_X23      0x0B8
#define TF_X24      0x0C0
#define TF_X25      0x0C8
#define TF_X26      0x0D0
#define TF_X27      0x0D8
#define TF_X28      0x0E0
#define TF_X29      0x0E8    /* Frame pointer */
#define TF_X30      0x0F0    /* Link register */
#define TF_SP       0x0F8    /* Stack pointer */
#define TF_PC       0x100    /* Program counter (ELR_EL1) */
#define TF_PSTATE   0x108    /* Processor state (SPSR_EL1) */
#define TF_ESR      0x110    /* Exception syndrome register */
#define TF_FAR      0x118    /* Fault address register */
#define TF_SIZE     0x120    /* Total size */

.text

/**
 * ARM64 exception entry macro
 * Saves all registers and calls C handler
 */
.macro ARM64_EXCEPTION_ENTRY handler
    /* Allocate trap frame on stack */
    sub sp, sp, #TF_SIZE
    
    /* Save all general purpose registers */
    stp x0, x1, [sp, #TF_X0]
    stp x2, x3, [sp, #TF_X2]
    stp x4, x5, [sp, #TF_X4]
    stp x6, x7, [sp, #TF_X6]
    stp x8, x9, [sp, #TF_X8]
    stp x10, x11, [sp, #TF_X10]
    stp x12, x13, [sp, #TF_X12]
    stp x14, x15, [sp, #TF_X14]
    stp x16, x17, [sp, #TF_X16]
    stp x18, x19, [sp, #TF_X18]
    stp x20, x21, [sp, #TF_X20]
    stp x22, x23, [sp, #TF_X22]
    stp x24, x25, [sp, #TF_X24]
    stp x26, x27, [sp, #TF_X26]
    stp x28, x29, [sp, #TF_X28]
    str x30, [sp, #TF_X30]
    
    /* Save exception return address and state */
    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp, #TF_PC]
    
    /* Save exception syndrome and fault address */
    mrs x0, esr_el1
    mrs x1, far_el1
    stp x0, x1, [sp, #TF_ESR]
    
    /* Save original stack pointer */
    add x0, sp, #TF_SIZE
    str x0, [sp, #TF_SP]
    
    /* Call C handler with trap frame pointer */
    mov x0, sp
    bl \handler
    
    /* Restore from trap frame if we return */
    b KiExceptionExit
.endm

/**
 * ARM64 exception exit routine
 */
.globl KiExceptionExit
KiExceptionExit:
    /* Restore exception return address and state */
    ldp x0, x1, [sp, #TF_PC]
    msr elr_el1, x0
    msr spsr_el1, x1
    
    /* Restore general purpose registers */
    ldr x30, [sp, #TF_X30]
    ldp x28, x29, [sp, #TF_X28]
    ldp x26, x27, [sp, #TF_X26]
    ldp x24, x25, [sp, #TF_X24]
    ldp x22, x23, [sp, #TF_X22]
    ldp x20, x21, [sp, #TF_X20]
    ldp x18, x19, [sp, #TF_X18]
    ldp x16, x17, [sp, #TF_X16]
    ldp x14, x15, [sp, #TF_X14]
    ldp x12, x13, [sp, #TF_X12]
    ldp x10, x11, [sp, #TF_X10]
    ldp x8, x9, [sp, #TF_X8]
    ldp x6, x7, [sp, #TF_X6]
    ldp x4, x5, [sp, #TF_X4]
    ldp x2, x3, [sp, #TF_X2]
    ldp x0, x1, [sp, #TF_X0]
    
    /* Deallocate trap frame */
    add sp, sp, #TF_SIZE
    
    /* Return from exception */
    eret

/**
 * Kernel trap handler (synchronous exceptions from kernel mode)
 */
.globl KiTrapHandler
KiTrapHandler:
    ARM64_EXCEPTION_ENTRY KiTrapHandlerC

/**
 * Interrupt handler (IRQ)
 */
.globl KiInterruptHandler
KiInterruptHandler:
    ARM64_EXCEPTION_ENTRY KiInterruptHandlerC

/**
 * Fast interrupt handler (FIQ)
 */
.globl KiFiqHandler
KiFiqHandler:
    ARM64_EXCEPTION_ENTRY KiFiqHandlerC

/**
 * System error handler (SError)
 */
.globl KiSerrorHandler
KiSerrorHandler:
    ARM64_EXCEPTION_ENTRY KiSerrorHandlerC

/**
 * System call handler for AArch64
 */
.globl KiSystemCallHandler64
KiSystemCallHandler64:
    ARM64_EXCEPTION_ENTRY KiSystemCallHandler64C

/**
 * System call handler for AArch32 (compatibility)
 */
.globl KiSystemCallHandler32
KiSystemCallHandler32:
    ARM64_EXCEPTION_ENTRY KiSystemCallHandler32C

/**
 * Context switch routine
 * x0 = OldThread
 * x1 = NewThread
 */
.globl KiSwapContext
KiSwapContext:
    /* Save callee-saved registers for old thread */
    stp x19, x20, [x0, #THREAD_CONTEXT_X19]
    stp x21, x22, [x0, #THREAD_CONTEXT_X21]
    stp x23, x24, [x0, #THREAD_CONTEXT_X23]
    stp x25, x26, [x0, #THREAD_CONTEXT_X25]
    stp x27, x28, [x0, #THREAD_CONTEXT_X27]
    stp x29, x30, [x0, #THREAD_CONTEXT_X29]
    
    /* Save stack pointer */
    mov x2, sp
    str x2, [x0, #THREAD_CONTEXT_SP]
    
    /* Switch to new thread's address space if different process */
    ldr x2, [x0, #THREAD_APCSTATE_PROCESS]
    ldr x3, [x1, #THREAD_APCSTATE_PROCESS]
    cmp x2, x3
    b.eq 1f
    
    /* Load new page directory base */
    ldr x2, [x3, #PROCESS_DIRECTORYTABLEBASE]
    msr ttbr1_el1, x2
    isb
    
    /* Invalidate TLB */
    tlbi vmalle1
    dsb sy
    isb

1:
    /* Restore callee-saved registers for new thread */
    ldp x19, x20, [x1, #THREAD_CONTEXT_X19]
    ldp x21, x22, [x1, #THREAD_CONTEXT_X21]
    ldp x23, x24, [x1, #THREAD_CONTEXT_X23]
    ldp x25, x26, [x1, #THREAD_CONTEXT_X25]
    ldp x27, x28, [x1, #THREAD_CONTEXT_X27]
    ldp x29, x30, [x1, #THREAD_CONTEXT_X29]
    
    /* Restore stack pointer */
    ldr x2, [x1, #THREAD_CONTEXT_SP]
    mov sp, x2
    
    /* Update current thread in PCR */
    /* This would need to be implemented with PCR base */
    
    ret

/**
 * Thread startup routine
 */
.globl KiThreadStartup
KiThreadStartup:
    /* Enable interrupts */
    msr daifclr, #0xF
    
    /* Call thread start routine */
    /* Implementation depends on thread structure layout */
    bl KiThreadStartupC
    
    /* Should never reach here */
    brk #0

/**
 * User mode return routine
 */
.globl KiServiceExit
KiServiceExit:
    /* This is an alias to exception exit for ARM64 */
    b KiExceptionExit

.globl KiServiceExit2
KiServiceExit2:
    /* Same as KiServiceExit for ARM64 */
    b KiExceptionExit

/**
 * Debug interrupt handler
 */
.globl KiDebugService
KiDebugService:
    ARM64_EXCEPTION_ENTRY KiDebugServiceC

/**
 * Breakpoint exception handler
 */
.globl KiBreakpointTrap
KiBreakpointTrap:
    ARM64_EXCEPTION_ENTRY KiBreakpointTrapC

/**
 * Single step exception handler
 */
.globl KiSingleStepTrap
KiSingleStepTrap:
    ARM64_EXCEPTION_ENTRY KiSingleStepTrapC

/**
 * Unexpected interrupt handler
 */
.globl KiUnexpectedInterrupt
KiUnexpectedInterrupt:
    /* Save minimal context */
    stp x0, x1, [sp, #-16]!
    stp x2, x30, [sp, #-16]!
    
    /* Get exception syndrome */
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    
    /* Call C handler */
    bl KiUnexpectedInterruptC
    
    /* Restore context */
    ldp x2, x30, [sp], #16
    ldp x0, x1, [sp], #16
    
    /* Return - should not happen */
    eret

/**
 * Display unexpected interrupt information
 * x0 = interrupt type
 */
.globl KiDisplayUnexpectedInterrupt
KiDisplayUnexpectedInterrupt:
    /* Save link register */
    str x30, [sp, #-16]!
    
    /* Call C function to display info */
    bl KiDisplayUnexpectedInterruptC
    
    /* Restore and return */
    ldr x30, [sp], #16
    ret

/**
 * Save floating point state
 * x0 = FPU context buffer
 */
.globl KiSaveFpuState
KiSaveFpuState:
    /* Save FPCR and FPSR */
    mrs x1, fpcr
    mrs x2, fpsr
    stp x1, x2, [x0], #16
    
    /* Save SIMD/FP registers */
    stp q0, q1, [x0], #32
    stp q2, q3, [x0], #32
    stp q4, q5, [x0], #32
    stp q6, q7, [x0], #32
    stp q8, q9, [x0], #32
    stp q10, q11, [x0], #32
    stp q12, q13, [x0], #32
    stp q14, q15, [x0], #32
    stp q16, q17, [x0], #32
    stp q18, q19, [x0], #32
    stp q20, q21, [x0], #32
    stp q22, q23, [x0], #32
    stp q24, q25, [x0], #32
    stp q26, q27, [x0], #32
    stp q28, q29, [x0], #32
    stp q30, q31, [x0], #32
    
    ret

/**
 * Restore floating point state
 * x0 = FPU context buffer
 */
.globl KiRestoreFpuState
KiRestoreFpuState:
    /* Restore FPCR and FPSR */
    ldp x1, x2, [x0], #16
    msr fpcr, x1
    msr fpsr, x2
    
    /* Restore SIMD/FP registers */
    ldp q0, q1, [x0], #32
    ldp q2, q3, [x0], #32
    ldp q4, q5, [x0], #32
    ldp q6, q7, [x0], #32
    ldp q8, q9, [x0], #32
    ldp q10, q11, [x0], #32
    ldp q12, q13, [x0], #32
    ldp q14, q15, [x0], #32
    ldp q16, q17, [x0], #32
    ldp q18, q19, [x0], #32
    ldp q20, q21, [x0], #32
    ldp q22, q23, [x0], #32
    ldp q24, q25, [x0], #32
    ldp q26, q27, [x0], #32
    ldp q28, q29, [x0], #32
    ldp q30, q31, [x0], #32
    
    ret

.end