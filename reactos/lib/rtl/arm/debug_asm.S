/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            rtl/arm/debug_asm.S
 * PURPOSE:         Cross-privilege Debugging and Exception Support for ARM
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ksarm.h>

    TEXTAREA

    NESTED_ENTRY DbgBreakPoint
    PROLOG_END DbgBreakPoint

    //
    // Do a breakpoint and return
    //
    bkpt BREAKPOINT_BREAK
    bx lr
    ENTRY_END DbgBreakPoint

    NESTED_ENTRY DebugService
    PROLOG_END DebugService

    //
    // Do a breakpoint and return
    //
    bkpt BREAKPOINT_PRINT // Could be prompt too, we check this later
    bx lr
    ENTRY_END DebugService

    NESTED_ENTRY DebugService2
    PROLOG_END DebugService2

    //
    // FIXME-TODO: Do a breakpoint and return
    //
    //bkpt BREAKPOINT_LOAD_SYMBOLS // Could be unload too, we check this later
    bx lr
    ENTRY_END DebugService2

    NESTED_ENTRY RtlCaptureContext
    PROLOG_END RtlCaptureContext

    //
    // FIXME-PERF: Change to stmdb later
    //
    str r0, [a1, #CsR0]
    str r1, [a1, #CsR1]
    str r2, [a1, #CsR2]
    str r3, [a1, #CsR3]
    str r4, [a1, #CsR4]
    str r5, [a1, #CsR5]
    str r6, [a1, #CsR6]
    str r7, [a1, #CsR7]
    str r8, [a1, #CsR8]
    str r9, [a1, #CsR9]
    str r10, [a1, #CsR10]
    str r11, [a1, #CsR11]
    str r12, [a1, #CsR12]
    str sp, [a1, #CsSp]
    str lr, [a1, #CsLr]
//    str pc, [a1, #CsPc] // FIXME: error A2193: this instruction generates unpredictable behavior
    mrs ip, spsr
    str ip, [a1, #CsCpsr]

    //
    // Set flags
    //
    mov ip, #CONTEXT_FULL
    str ip, [a1, #CsContextFlags]

    //
    // Return
    //
    bx lr
    ENTRY_END RtlCaptureContext

    END
/* EOF */
