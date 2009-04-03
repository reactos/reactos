/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/boot.s
 * PURPOSE:         Implements the kernel entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM Kernel Entry Point"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    TEXTAREA
    NESTED_ENTRY KiSystemStartup
    PROLOG_END KiSystemStartup
    
    //
    // Put us in FIQ mode
    //
    mrs r3, cpsr
    orr r3, r1, #CPSR_FIQ_MODE
    msr cpsr, r3
    
    //
    // Set FIQ stack and registers
    //
    ldr sp, [a2, #LpbInterruptStack]
    mov r8, #0
    mov r9, #0
    mov r10, #0
    
    //
    // Put us in ABORT mode
    //
    mrs r3, cpsr
    orr r3, r1, #CPSR_ABORT_MODE
    msr cpsr, r3
       
    //
    // Set panic stack
    //
    ldr sp, [a2, #LpbPanicStack]
    
    //
    // Put us in UND (Undefined) mode
    //
    mrs r3, cpsr
    orr r3, r1, #CPSR_UND_MODE
    msr cpsr, r3
       
    //
    // Set panic stack
    //
    ldr sp, [a2, #LpbPanicStack]
    
    //
    // Put us into SVC (Supervisor) mode
    //
    mrs r3, cpsr
    orr r3, r1, #CPSR_SVC_MODE
    msr cpsr, r3

    //
    // Switch to boot kernel stack
    //
    ldr sp, [a2, #LpbKernelStack]
    
    //
    // Go to C code
    //
    b KiInitializeSystem
    
    ENTRY_END KiSystemStartup
