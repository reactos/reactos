/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
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
    // Save volatile registers for the OLD thread
    //
    sub sp, sp, #(4*8)
    mrs ip, spsr_all
    stmia sp, {ip, lr}
    sub sp, sp, #(4*2)
    stmia sp, {r4-r11}
    
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
    // Restore volatile registers for the NEW thread
    //
    ldmia sp, {r4-r11}
    add sp, sp, #(4*8)
    ldmia sp, {ip, lr}
    msr spsr_all, ip
    add sp, sp, #(4*2)
    
    //
    // Jump to saved restore address
    //
    mov pc, lr

    ENTRY_END KiSwapContext

    NESTED_ENTRY KiThreadStartup
    PROLOG_END KiThreadStartup
    
    //
    // FIXME: Make space on stack and clean it up?
    //
    
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
