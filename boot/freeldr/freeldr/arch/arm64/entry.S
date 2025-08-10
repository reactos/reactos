/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 entry point for FreeLoader
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <asm.inc>

EXTERN BootMain
EXTERN FrldrBootDrive
EXTERN FrldrBootPartition
EXTERN Arm64InitializeExceptions
EXTERN Arm64InitializeMMU

.text

/* ARM64 UEFI entry point */
PUBLIC RealEntryPoint
RealEntryPoint:
    /* x0 = ImageHandle, x1 = SystemTable (UEFI parameters) */
    
    /* Save UEFI parameters on stack */
    stp x29, x30, [sp, #-32]!
    stp x0, x1, [sp, #16]
    mov x29, sp
    
    /* Disable interrupts during initialization */
    msr daifset, #0xF
    
    /* Initialize exception vectors */
    bl Arm64SetupExceptionVectors
    
    /* Initialize BSS section */
    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    sub x2, x1, x0
    cbz x2, bss_done
    
clear_bss:
    str xzr, [x0], #8
    subs x2, x2, #8
    b.ne clear_bss
    
bss_done:
    /* Store BootDrive and BootPartition (defaults for UEFI) */
    mov w0, #0x80  /* Default boot drive for UEFI */
    ldr x1, =FrldrBootDrive
    strb w0, [x1]
    
    mov w0, #0xFF  /* UEFI boot partition marker */
    ldr x1, =FrldrBootPartition
    str w0, [x1]
    
    /* Initialize cache and memory management */
    bl Arm64InitializeCache
    bl Arm64InitializeMMU
    
    /* Set up parameters for BootMain (command line = NULL for now) */
    mov x0, #0
    
    /* Enable interrupts before calling BootMain */
    msr daifclr, #0xF
    
    /* Call BootMain */
    bl BootMain
    
    /* Should never return, but if it does, loop */
stop:
    b stop

/* ARM64 cache and MMU initialization */
PUBLIC Arm64InitializeCache
Arm64InitializeCache:
    /* Ensure all previous instructions complete */
    dsb sy
    isb
    
    /* Invalidate instruction cache */
    ic iallu
    dsb sy
    isb
    
    /* Clean and invalidate data cache */
    mrs x0, clidr_el1
    and w3, w0, #0x07000000
    lsr w3, w3, #23
    cbz w3, cache_done
    
    mov w10, #0
    mov w8, #1
    
cache_loop1:
    add w2, w10, w10, lsr #1
    lsr w1, w0, w2
    and w1, w1, #0x7
    cmp w1, #2
    b.lt cache_skip
    
    msr csselr_el1, x10
    isb
    mrs x1, ccsidr_el1
    and w2, w1, #7
    add w2, w2, #4
    ubfx w4, w1, #3, #10
    clz w5, w4
    lsl w9, w4, w5
    lsl w16, w8, w5
    
cache_loop2:
    ubfx w7, w1, #13, #15
    lsl w7, w7, w2
    lsl w17, w8, w2
    
cache_loop3:
    orr w11, w10, w9
    orr w11, w11, w7
    dc cisw, x11
    subs w7, w7, w17
    b.ge cache_loop3
    subs x9, x9, x16
    b.ge cache_loop2
    
cache_skip:
    add w10, w10, #2
    cmp w3, w10
    dsb sy
    b.gt cache_loop1
    
cache_done:
    isb
    ret

/* Setup ARM64 exception vectors */
PUBLIC Arm64SetupExceptionVectors
Arm64SetupExceptionVectors:
    /* Install exception vector table */
    ldr x0, =arm64_exception_vectors
    msr vbar_el1, x0
    isb
    ret

PUBLIC Reboot
Reboot:
    /* Disable interrupts */
    msr daifset, #0xF
    
    /* System reset via PSCI call if available, otherwise loop */
    ldr x0, =0x84000009  /* PSCI SYSTEM_RESET */
    hvc #0
    
    /* If PSCI fails, infinite loop */
1:  wfi
    b 1b

/* ARM64 equivalent functions - not applicable but needed for compatibility */
PUBLIC Relocator16Boot
Relocator16Boot:
    /* Not supported on ARM64 UEFI */
    b stop

PUBLIC PxeCallApi
PxeCallApi:
    /* PXE not supported on ARM64 for now */
    mov x0, #0
    ret

PUBLIC CallRealMode
CallRealMode:
    /* Real mode not applicable on ARM64 */
    ret

/* Exception vector table for ARM64 */
.align 11
PUBLIC arm64_exception_vectors
arm64_exception_vectors:
    /* Current EL with SP0 */
    .align 7
    b arm64_sync_exception_sp0      /* Synchronous */
    .align 7
    b arm64_irq_exception_sp0       /* IRQ */
    .align 7
    b arm64_fiq_exception_sp0       /* FIQ */
    .align 7
    b arm64_serror_exception_sp0    /* SError */
    
    /* Current EL with SPx */
    .align 7
    b arm64_sync_exception_spx      /* Synchronous */
    .align 7
    b arm64_irq_exception_spx       /* IRQ */
    .align 7
    b arm64_fiq_exception_spx       /* FIQ */
    .align 7
    b arm64_serror_exception_spx    /* SError */
    
    /* Lower EL using AArch64 */
    .align 7
    b arm64_sync_exception_lower64  /* Synchronous */
    .align 7
    b arm64_irq_exception_lower64   /* IRQ */
    .align 7
    b arm64_fiq_exception_lower64   /* FIQ */
    .align 7
    b arm64_serror_exception_lower64 /* SError */
    
    /* Lower EL using AArch32 */
    .align 7
    b arm64_sync_exception_lower32  /* Synchronous */
    .align 7
    b arm64_irq_exception_lower32   /* IRQ */
    .align 7
    b arm64_fiq_exception_lower32   /* FIQ */
    .align 7
    b arm64_serror_exception_lower32 /* SError */

/* Exception handlers - for now just hang */
arm64_sync_exception_sp0:
arm64_irq_exception_sp0:
arm64_fiq_exception_sp0:
arm64_serror_exception_sp0:
arm64_sync_exception_spx:
arm64_irq_exception_spx:
arm64_fiq_exception_spx:
arm64_serror_exception_spx:
arm64_sync_exception_lower64:
arm64_irq_exception_lower64:
arm64_fiq_exception_lower64:
arm64_serror_exception_lower64:
arm64_sync_exception_lower32:
arm64_irq_exception_lower32:
arm64_fiq_exception_lower32:
arm64_serror_exception_lower32:
    /* For now, just loop on exceptions */
1:  wfi
    b 1b

PUBLIC FrldrBootDrive
FrldrBootDrive:
    .byte 0

PUBLIC FrldrBootPartition
FrldrBootPartition:
    .long 0

END