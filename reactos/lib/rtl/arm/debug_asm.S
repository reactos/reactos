/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            rtl/arm/debug_asm.S
 * PURPOSE:         Cross-privilege Debugging and Exception Support for ARM
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ksarm.h>

    TEXTAREA

    LEAF_ENTRY DbgBreakPoint
    __debugbreak
    bx lr
    LEAF_END DbgBreakPoint

    LEAF_ENTRY DbgUserBreakPoint
    __debugbreak
    bx lr
    LEAF_END DbgUserBreakPoint

    LEAF_ENTRY DbgBreakPointWithStatus
    __debugbreak
    bx lr
    LEAF_END

    LEAF_ENTRY RtlpBreakWithStatusInstruction
    __debugbreak
    bx lr
    LEAF_END

    LEAF_ENTRY DebugService
    __debugservice
    //bkpt BREAKPOINT_PRINT // Could be prompt too, we check this later
    bx lr
    LEAF_END DebugService

    LEAF_ENTRY DebugService2
    //bkpt BREAKPOINT_LOAD_SYMBOLS // Could be unload too, we check this later
    bx lr
    LEAF_END DebugService2

    LEAF_ENTRY RtlCaptureContext

    // FIXME-PERF: Change to stmdb later
    str r0, [a1, #CxR0]
    str r1, [a1, #CxR1]
    str r2, [a1, #CxR2]
    str r3, [a1, #CxR3]
    str r4, [a1, #CxR4]
    str r5, [a1, #CxR5]
    str r6, [a1, #CxR6]
    str r7, [a1, #CxR7]
    str r8, [a1, #CxR8]
    str r9, [a1, #CxR9]
    str r10, [a1, #CxR10]
    str r11, [a1, #CxR11]
    str r12, [a1, #CxR12]

    str sp, [a1, #CxSp]
    str lr, [a1, #CxLr]

    /* Need to do this indirectly, since "str pc, [a1, #CxPc]" generates
       unpredictable behavior (error A2193) */
    mov ip, pc
    str ip, [a1, #CxPc]
    mrs ip, spsr
    str ip, [a1, #CxCpsr]

    // FIXME: Fpscr and remaining stuff

    /* Set flags */
    mov32 r0, #CONTEXT_FULL
    str ip, [a1, #CxContextFlags]

    /* Return */
    bx lr
    LEAF_END RtlCaptureContext

    END
/* EOF */
