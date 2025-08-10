/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 exception and interrupt handlers
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <asm.inc>

/* ARM64 register context structure size */
#define ARM64_CONTEXT_SIZE  (34 * 8)  /* 31 GPRs + SP + PC + PSTATE + alignment */

/* Exception syndrome register bits */
#define ESR_ELx_EC_SHIFT    26
#define ESR_ELx_EC_MASK     0x3F
#define ESR_ELx_IL          (1 << 25)
#define ESR_ELx_ISS_MASK    0x1FFFFFF

/* Exception classes */
#define ESR_ELx_EC_UNKNOWN      0x00
#define ESR_ELx_EC_WFx          0x01
#define ESR_ELx_EC_CP15_32      0x03
#define ESR_ELx_EC_CP15_64      0x04
#define ESR_ELx_EC_CP14_MR      0x05
#define ESR_ELx_EC_CP14_LS      0x06
#define ESR_ELx_EC_FP_ASIMD     0x07
#define ESR_ELx_EC_CP10_ID      0x08
#define ESR_ELx_EC_CP14_64      0x0C
#define ESR_ELx_EC_SVC64        0x15
#define ESR_ELx_EC_HVC64        0x16
#define ESR_ELx_EC_SMC64        0x17
#define ESR_ELx_EC_SYS64        0x18
#define ESR_ELx_EC_IMP_DEF      0x1F
#define ESR_ELx_EC_IABT_LOW     0x20
#define ESR_ELx_EC_IABT_CUR     0x21
#define ESR_ELx_EC_PC_ALIGN     0x22
#define ESR_ELx_EC_DABT_LOW     0x24
#define ESR_ELx_EC_DABT_CUR     0x25
#define ESR_ELx_EC_SP_ALIGN     0x26
#define ESR_ELx_EC_FP_EXC32     0x28
#define ESR_ELx_EC_FP_EXC64     0x2C
#define ESR_ELx_EC_SERROR       0x2F
#define ESR_ELx_EC_BREAKPT_LOW  0x30
#define ESR_ELx_EC_BREAKPT_CUR  0x31
#define ESR_ELx_EC_SOFTSTP_LOW  0x32
#define ESR_ELx_EC_SOFTSTP_CUR  0x33
#define ESR_ELx_EC_WATCHPT_LOW  0x34
#define ESR_ELx_EC_WATCHPT_CUR  0x35
#define ESR_ELx_EC_BKPT32       0x38
#define ESR_ELx_EC_BRK64        0x3C

EXTERN Arm64HandleSynchronousException
EXTERN Arm64HandleIrq
EXTERN Arm64HandleFiq
EXTERN Arm64HandleSerror

.text

/* Macro to save ARM64 register context */
.macro SAVE_ARM64_CONTEXT
    /* Make room for the context structure */
    sub sp, sp, #ARM64_CONTEXT_SIZE
    
    /* Save general purpose registers X0-X30 */
    stp x0, x1, [sp, #(0 * 8)]
    stp x2, x3, [sp, #(2 * 8)]
    stp x4, x5, [sp, #(4 * 8)]
    stp x6, x7, [sp, #(6 * 8)]
    stp x8, x9, [sp, #(8 * 8)]
    stp x10, x11, [sp, #(10 * 8)]
    stp x12, x13, [sp, #(12 * 8)]
    stp x14, x15, [sp, #(14 * 8)]
    stp x16, x17, [sp, #(16 * 8)]
    stp x18, x19, [sp, #(18 * 8)]
    stp x20, x21, [sp, #(20 * 8)]
    stp x22, x23, [sp, #(22 * 8)]
    stp x24, x25, [sp, #(24 * 8)]
    stp x26, x27, [sp, #(26 * 8)]
    stp x28, x29, [sp, #(28 * 8)]
    
    /* Save X30 (LR) and original SP */
    add x1, sp, #ARM64_CONTEXT_SIZE
    stp x30, x1, [sp, #(30 * 8)]
    
    /* Save PC and PSTATE from exception frame */
    mrs x1, elr_el1
    mrs x2, spsr_el1
    stp x1, x2, [sp, #(32 * 8)]
.endm

/* Macro to restore ARM64 register context */
.macro RESTORE_ARM64_CONTEXT
    /* Restore PC and PSTATE */
    ldp x1, x2, [sp, #(32 * 8)]
    msr elr_el1, x1
    msr spsr_el1, x2
    
    /* Restore X30 and skip SP (will be restored by add) */
    ldp x30, x1, [sp, #(30 * 8)]
    
    /* Restore general purpose registers X0-X29 */
    ldp x28, x29, [sp, #(28 * 8)]
    ldp x26, x27, [sp, #(26 * 8)]
    ldp x24, x25, [sp, #(24 * 8)]
    ldp x22, x23, [sp, #(22 * 8)]
    ldp x20, x21, [sp, #(20 * 8)]
    ldp x18, x19, [sp, #(18 * 8)]
    ldp x16, x17, [sp, #(16 * 8)]
    ldp x14, x15, [sp, #(14 * 8)]
    ldp x12, x13, [sp, #(12 * 8)]
    ldp x10, x11, [sp, #(10 * 8)]
    ldp x8, x9, [sp, #(8 * 8)]
    ldp x6, x7, [sp, #(6 * 8)]
    ldp x4, x5, [sp, #(4 * 8)]
    ldp x2, x3, [sp, #(2 * 8)]
    ldp x0, x1, [sp, #(0 * 8)]
    
    /* Restore stack pointer */
    add sp, sp, #ARM64_CONTEXT_SIZE
.endm

/* Common synchronous exception handler */
PUBLIC Arm64SyncExceptionHandler
Arm64SyncExceptionHandler:
    SAVE_ARM64_CONTEXT
    
    /* Get exception syndrome */
    mrs x1, esr_el1
    mrs x2, far_el1    /* Fault address */
    mrs x3, elr_el1    /* Exception link register (PC) */
    
    /* Call C handler with context pointer and syndrome */
    mov x0, sp
    bl Arm64HandleSynchronousException
    
    RESTORE_ARM64_CONTEXT
    eret

/* Common IRQ handler */
PUBLIC Arm64IrqHandler
Arm64IrqHandler:
    SAVE_ARM64_CONTEXT
    
    /* Call C IRQ handler */
    mov x0, sp
    bl Arm64HandleIrq
    
    RESTORE_ARM64_CONTEXT
    eret

/* Common FIQ handler */
PUBLIC Arm64FiqHandler
Arm64FiqHandler:
    SAVE_ARM64_CONTEXT
    
    /* Call C FIQ handler */
    mov x0, sp
    bl Arm64HandleFiq
    
    RESTORE_ARM64_CONTEXT
    eret

/* Common SError handler */
PUBLIC Arm64SerrorHandler
Arm64SerrorHandler:
    SAVE_ARM64_CONTEXT
    
    /* Get error syndrome */
    mrs x1, esr_el1
    
    /* Call C SError handler */
    mov x0, sp
    bl Arm64HandleSerror
    
    /* SError usually doesn't return, but if it does... */
    RESTORE_ARM64_CONTEXT
    eret

/* Install exception handlers into vector table */
PUBLIC Arm64InstallExceptionHandlers
Arm64InstallExceptionHandlers:
    /* Get vector table base */
    ldr x0, =arm64_exception_vectors
    
    /* Install sync exception handlers */
    ldr x1, =Arm64SyncExceptionHandler
    
    /* Update vector table entries to jump to our handlers */
    /* This is a simplified approach - real implementation would need */
    /* to modify the actual vector table entries */
    
    /* For now, the handlers are already linked in the vector table */
    /* defined in entry.S */
    
    ret

/* Debug: Print ARM64 register context */
PUBLIC Arm64PrintContext
Arm64PrintContext:
    /* x0 = pointer to ARM64_CONTEXT structure */
    /* This would print register values for debugging */
    /* Implementation depends on console output availability */
    ret

END