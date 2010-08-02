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
    
    /* Put us in FIQ mode, set IRQ stack */
    msr cpsr_c, #CPSR_FIQ_MODE
    ldr sp, [a1, #LpbInterruptStack]
    
    /* Repeat for IRQ mode */
    msr cpsr_c, #CPSR_IRQ_MODE
    ldr sp, [a1, #LpbInterruptStack]

    /* Put us in ABORT mode and set the panic stack */
    msr cpsr_c, #CPSR_ABORT_MODE
    ldr sp, [a1, #LpbPanicStack]
    
    /* Repeat for UND (Undefined) mode */
    msr cpsr_c, #CPSR_UND_MODE
    ldr sp, [a1, #LpbPanicStack]
    
    /* Put us into SVC (Supervisor) mode and set the kernel stack */
    msr cpsr_c, #CPSR_SVC_MODE
    ldr sp, [a1, #LpbKernelStack]
    
    /* Go to C code */
    b KiInitializeSystem
    
    ENTRY_END KiSystemStartup

/* EOF */
