/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 context switching and thread startup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

    IMPORT KiSwapContextResume
    IMPORT KiArm64StartThread
    IMPORT KiArm64TrapExit
    IMPORT KiSwitchKernelStack

    TEXTAREA

//
// BOOLEAN
// KiSwapContext(
//     _In_ KIRQL WaitIrql,
//     _In_ PKTHREAD OldThread);
//
// The new thread is already PRCB.CurrentThread. Returns TRUE when the new
// thread has kernel APCs to deliver.
//
    NESTED_ENTRY KiSwapContext
    sub     sp, sp, #KSWITCH_FRAME_LENGTH
    stp     x19, x20, [sp, #SwX19]
    stp     x21, x22, [sp, #SwX21]
    stp     x23, x24, [sp, #SwX23]
    stp     x25, x26, [sp, #SwX25]
    stp     x27, x28, [sp, #SwX27]
    stp     x29, x30, [sp, #SwFp]
    stp     d8, d9, [sp, #SwD8]
    stp     d10, d11, [sp, #SwD10]
    stp     d12, d13, [sp, #SwD12]
    stp     d14, d15, [sp, #SwD14]
    strb    w0, [sp, #SwApcBypass]

    ldr     x2, [x18, #PcCurrentThread]

    /* The new thread may still be leaving another processor */
    add     x3, x2, #ThSwapBusy
LABEL(KiSwapContextWait)
    ldarb   w4, [x3]
    cbz     w4, KiSwapContextReady
    yield
    b       KiSwapContextWait

LABEL(KiSwapContextReady)
    mov     x3, sp
    str     x3, [x1, #ThKernelStack]
    ldr     x3, [x2, #ThKernelStack]
    mov     sp, x3

    ldrb    w0, [sp, #SwApcBypass]
    bl      KiSwapContextResume

    ldp     x19, x20, [sp, #SwX19]
    ldp     x21, x22, [sp, #SwX21]
    ldp     x23, x24, [sp, #SwX23]
    ldp     x25, x26, [sp, #SwX25]
    ldp     x27, x28, [sp, #SwX27]
    ldp     x29, x30, [sp, #SwFp]
    ldp     d8, d9, [sp, #SwD8]
    ldp     d10, d11, [sp, #SwD10]
    ldp     d12, d13, [sp, #SwD12]
    ldp     d14, d15, [sp, #SwD14]
    add     sp, sp, #KSWITCH_FRAME_LENGTH
    ret
    NESTED_END KiSwapContext

//
// First code of every thread, the context switch returns here with sp at
// the KSTART_FRAME. Only user threads come back, their exception frame
// follows the start frame.
//
    NESTED_ENTRY KiThreadStartup
    mov     x29, #0
    mov     x0, sp
    bl      KiArm64StartThread
    add     sp, sp, #KSTART_FRAME_LENGTH
    b       KiArm64TrapExit
    NESTED_END KiThreadStartup

//
// PVOID
// KeSwitchKernelStack(
//     _In_ PVOID StackBase,
//     _In_ PVOID StackLimit);
//
// Returns the old stack base, the caller continues on the new stack.
//
    NESTED_ENTRY KeSwitchKernelStack
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    mov     x2, x29
    add     x3, sp, #16
    bl      KiSwitchKernelStack

    /* The old base still sits on the old stack */
    ldr     x1, [sp, #16]
    add     sp, sp, x0
    add     x29, x29, x0
    ldp     x29, x30, [sp], #32
    mov     x0, x1
    ret
    NESTED_END KeSwitchKernelStack

    END
