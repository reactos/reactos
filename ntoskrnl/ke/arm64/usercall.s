/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 user mode callout entry and return
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ksarm64.h>

/* The callout frame is an exception frame followed by the saved initial and callback stacks */
#define CALLOUT_FRAME_LENGTH    (KEXCEPTION_FRAME_LENGTH + 0x10)

/* Room for the frames a trap from user mode leaves on the stack */
#define USER_TRAP_LENGTH        (KEXCEPTION_FRAME_LENGTH + KARM64_VFP_STATE_LENGTH + KTRAP_FRAME_LENGTH)

    IMPORT KiArm64UserModeCallout

    TEXTAREA

//
// NTSTATUS
// KiCallUserMode(
//     _In_ PVOID *OutputBuffer,
//     _In_ PULONG OutputLength);
//
// Returns through KiArm64CallbackReturn once user mode finishes, or right
// away when the callout cannot be set up.
//
    NESTED_ENTRY KiCallUserMode
    sub     sp, sp, #CALLOUT_FRAME_LENGTH
    stp     x19, x20, [sp, #ExX19]
    stp     x21, x22, [sp, #ExX21]
    stp     x23, x24, [sp, #ExX23]
    stp     x25, x26, [sp, #ExX25]
    stp     x27, x28, [sp, #ExX27]
    stp     x29, x30, [sp, #ExFp]
    stp     d8, d9, [sp, #ExD8]
    stp     d10, d11, [sp, #ExD10]
    stp     d12, d13, [sp, #ExD12]
    stp     d14, d15, [sp, #ExD14]
    str     x0, [sp, #ExOutputBuffer]
    str     x1, [sp, #ExOutputLength]
    add     x29, sp, #ExFp

    /* The callback trap frames go right below, the C code runs under them */
    mov     x0, sp
    sub     sp, sp, #USER_TRAP_LENGTH
    bl      KiArm64UserModeCallout

    /* Only a failure comes back here */
    add     sp, sp, #USER_TRAP_LENGTH
    ldp     x29, x30, [sp, #ExFp]
    add     sp, sp, #CALLOUT_FRAME_LENGTH
    ret
    NESTED_END KiCallUserMode

//
// DECLSPEC_NORETURN
// VOID
// KiArm64CallbackReturn(
//     _In_ PKCALLOUT_FRAME CalloutFrame,
//     _In_ NTSTATUS Status);
//
// Drops everything above the callout frame and returns from KiCallUserMode.
//
    LEAF_ENTRY KiArm64CallbackReturn
    mov     sp, x0
    ldp     x19, x20, [sp, #ExX19]
    ldp     x21, x22, [sp, #ExX21]
    ldp     x23, x24, [sp, #ExX23]
    ldp     x25, x26, [sp, #ExX25]
    ldp     x27, x28, [sp, #ExX27]
    ldp     x29, x30, [sp, #ExFp]
    ldp     d8, d9, [sp, #ExD8]
    ldp     d10, d11, [sp, #ExD10]
    ldp     d12, d13, [sp, #ExD12]
    ldp     d14, d15, [sp, #ExD14]
    mov     w0, w1
    add     sp, sp, #CALLOUT_FRAME_LENGTH
    ret
    LEAF_END KiArm64CallbackReturn

    END
