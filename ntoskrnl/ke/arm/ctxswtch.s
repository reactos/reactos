/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/ctxswtch.s
 * PURPOSE:         Context Switch and Idle Thread on ARM
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM Context Switching"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    TEXTAREA
    NESTED_ENTRY KiSwapContext
    PROLOG_END KiSwapContext
    //
    // a1 = Old Thread
    // a2 = New Thread
    //
    
    //
    // Make space for the trap frame
    //
    sub sp, sp, #ExceptionFrameLength
    
    //
    // Build exception frame
    // FIXME-PERF: Change to stmdb later
    //
    str r4, [sp, #ExR4]
    str r5, [sp, #ExR5]
    str r6, [sp, #ExR6]
    str r7, [sp, #ExR7]
    str r8, [sp, #ExR8]
    str r9, [sp, #ExR9]
    str r10, [sp, #ExR10]
    str r11, [sp, #ExR11]
    str lr, [sp, #ExLr]
    mrs r4, spsr_all
    str r4, [sp, #ExSpsr]

    //
    // Switch stacks
    //
    str sp, [a1, #ThKernelStack]
    ldr sp, [a2, #ThKernelStack]
    
    //
    // Call the C context switch code
    //
    bl KiSwapContextInternal

    //
    // Get the SPSR and restore it
    //
    ldr r4, [sp, #ExSpsr]
    msr spsr_all, r4
    
    //
    // Restore the registers
    // FIXME-PERF: Use LDMIA later
    //
    ldr r4, [sp, #ExR4]
    ldr r5, [sp, #ExR5]
    ldr r6, [sp, #ExR6]
    ldr r7, [sp, #ExR7]
    ldr r8, [sp, #ExR8]
    ldr r9, [sp, #ExR9]
    ldr r10, [sp, #ExR10]
    ldr r11, [sp, #ExR11]
    ldr lr, [sp, #ExLr]
    
    //
    // Restore stack
    //
    add sp, sp, #ExceptionFrameLength
    
    //
    // Jump to saved restore address
    //
    mov pc, lr

    ENTRY_END KiSwapContext

    NESTED_ENTRY KiThreadStartup
    PROLOG_END KiThreadStartup
        
    //
    // Lower to APC_LEVEL
    //
    mov a1, #1
    bl KeLowerIrql
    
    //
    // Set the start address and startup context
    //
    mov a1, r6
    mov a2, r5
    blx r7
    
    //
    // Oh noes, we are back!
    //
    b .
    
    ENTRY_END KiThreadStartup
