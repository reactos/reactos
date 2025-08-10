/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 Boot Code and Startup Trampoline
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES ******************************************************************/

#include <arm64/arm64.h>

/* EXTERN FUNCTIONS **********************************************************/

.extern KiSystemStartupBootStack

/* FUNCTIONS *****************************************************************/

.text

/**
 * ARM64 Kernel Entry Point from FreeLDR
 * Called by FreeLDR after setting up initial page tables
 * 
 * Parameters:
 * x0 - LoaderBlock pointer
 * x1 - Initial stack pointer
 */
.globl _start
_start:
    /* Disable interrupts */
    msr daifset, #0xF
    
    /* Set up initial stack */
    mov sp, x1
    
    /* Align stack to 16 bytes */
    and sp, sp, #~0xF
    
    /* Store LoaderBlock pointer in preserved register */
    mov x19, x0
    
    /* Initialize exception vectors early */
    bl KiInitializeExceptionVectors
    
    /* Jump to kernel startup with LoaderBlock */
    mov x0, x19
    bl KiSystemStartupBootStack
    
    /* Should never reach here */
    b .

/**
 * ARM64 Context Switch Entry Point
 * Called when switching to boot stack
 */
.globl KiSwitchToBootStack
KiSwitchToBootStack:
    /* Set new stack pointer */
    mov sp, x0
    
    /* Align stack to 16 bytes */
    and sp, sp, #~0xF
    
    /* Jump to boot stack initialization */
    b KiSystemStartupBootStack

/**
 * ARM64 Exception Vector Table
 * Must be aligned to 2KB boundary
 */
.align 11
.globl KiExceptionVectors
KiExceptionVectors:
    /* Current EL with SP_EL0 */
    .org KiExceptionVectors + 0x000
    b KiSynchronousExceptionSP0
    .org KiExceptionVectors + 0x080
    b KiIrqExceptionSP0
    .org KiExceptionVectors + 0x100
    b KiFiqExceptionSP0
    .org KiExceptionVectors + 0x180
    b KiSerrorExceptionSP0
    
    /* Current EL with SP_ELx */
    .org KiExceptionVectors + 0x200
    b KiSynchronousExceptionSPx
    .org KiExceptionVectors + 0x280
    b KiIrqExceptionSPx
    .org KiExceptionVectors + 0x300
    b KiFiqExceptionSPx
    .org KiExceptionVectors + 0x380
    b KiSerrorExceptionSPx
    
    /* Lower EL using AArch64 */
    .org KiExceptionVectors + 0x400
    b KiSynchronousExceptionLowerAArch64
    .org KiExceptionVectors + 0x480
    b KiIrqExceptionLowerAArch64
    .org KiExceptionVectors + 0x500
    b KiFiqExceptionLowerAArch64
    .org KiExceptionVectors + 0x580
    b KiSerrorExceptionLowerAArch64
    
    /* Lower EL using AArch32 */
    .org KiExceptionVectors + 0x600
    b KiSynchronousExceptionLowerAArch32
    .org KiExceptionVectors + 0x680
    b KiIrqExceptionLowerAArch32
    .org KiExceptionVectors + 0x700
    b KiFiqExceptionLowerAArch32
    .org KiExceptionVectors + 0x780
    b KiSerrorExceptionLowerAArch32

/**
 * Initialize ARM64 Exception Vectors
 */
.globl KiInitializeExceptionVectors
KiInitializeExceptionVectors:
    /* Set VBAR_EL1 to point to our exception vector table */
    adrp x0, KiExceptionVectors
    add x0, x0, #:lo12:KiExceptionVectors
    msr vbar_el1, x0
    isb
    ret

/**
 * Exception Handler Stubs
 * These will be implemented in trap.S
 */
KiSynchronousExceptionSP0:
    b KiUnexpectedInterrupt

KiIrqExceptionSP0:
    b KiUnexpectedInterrupt

KiFiqExceptionSP0:
    b KiUnexpectedInterrupt

KiSerrorExceptionSP0:
    b KiUnexpectedInterrupt

KiSynchronousExceptionSPx:
    b KiTrapHandler

KiIrqExceptionSPx:
    b KiInterruptHandler

KiFiqExceptionSPx:
    b KiFiqHandler

KiSerrorExceptionSPx:
    b KiSerrorHandler

KiSynchronousExceptionLowerAArch64:
    b KiSystemCallHandler64

KiIrqExceptionLowerAArch64:
    b KiInterruptHandler

KiFiqExceptionLowerAArch64:
    b KiFiqHandler

KiSerrorExceptionLowerAArch64:
    b KiSerrorHandler

KiSynchronousExceptionLowerAArch32:
    b KiSystemCallHandler32

KiIrqExceptionLowerAArch32:
    b KiInterruptHandler

KiFiqExceptionLowerAArch32:
    b KiFiqHandler

KiSerrorExceptionLowerAArch32:
    b KiSerrorHandler

/**
 * Temporary interrupt handler for unimplemented exceptions
 */
KiUnexpectedInterrupt:
    /* Save some context */
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    
    /* Print debug message */
    mov x0, #0  /* Unexpected interrupt */
    bl KiDisplayUnexpectedInterrupt
    
    /* Restore context */
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    
    /* Hang the system */
1:  wfi
    b 1b

.end