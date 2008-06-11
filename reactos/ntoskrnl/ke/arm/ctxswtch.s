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
    
    //
    // Jump to saved restore address
    //
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiSwapContext

