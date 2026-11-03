/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 UEFI stack switching helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

    IMPORT UefiServiceStack
    IMPORT BasicStack
    IMPORT ExecuteLoaderCleanly
    IMPORT UefiExitBootServices

    TEXTAREA

//
// VOID
// _exituefi(VOID);
//
// Boot services are left on the stack the firmware gave us.
//
    LEAF_ENTRY _exituefi
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    ldr     x16, =UefiServiceStack
    ldr     x16, [x16]
    and     x16, x16, #~15
    mov     sp, x16

    bl      UefiExitBootServices

    mov     sp, x29
    ldp     x29, x30, [sp], #16
    ret
    LEAF_END _exituefi

//
// DECLSPEC_NORETURN
// VOID
// _changestack(VOID);
//
// Continues the loader on its own stack, the firmware stack is kept for
// leaving boot services.
//
    LEAF_ENTRY _changestack
    mov     x0, sp

    ldr     x16, =BasicStack
    ldr     x16, [x16]
    and     x16, x16, #~15
    mov     sp, x16
    mov     x29, xzr

    bl      ExecuteLoaderCleanly
    brk     #0xF000
    LEAF_END _changestack

    END
