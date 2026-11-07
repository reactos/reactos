/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ARM64 asm code for except_arm64.c
 * COPYRIGHT:   Copyright 2017-2024 Wine team
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

// See msvcrt/except_arm64.c

    TEXTAREA

//
// void *call_exc_handler( void *handler, ULONG_PTR frame, UINT flags, BYTE *nonvol_regs );
//
// The epilog is the exact reverse of the prolog, so one set of unwind codes covers both.
//
    NESTED_ENTRY call_exc_handler

    stp     x29, x30, [sp, #-96]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    stp     x25, x26, [sp, #64]
    stp     x27, x28, [sp, #80]
    str     x1, [sp, #-16]!

    // Nonvolatile registers of the frame the handler belongs to
    ldp     x19, x20, [x3, #0]
    ldp     x21, x22, [x3, #16]
    ldp     x23, x24, [x3, #32]
    ldp     x25, x26, [x3, #48]
    ldp     x27, x28, [x3, #64]
    ldr     x29, [x3, #80]
    blr     x0

    add     sp, sp, #16
    ldp     x27, x28, [sp, #80]
    ldp     x25, x26, [sp, #64]
    ldp     x23, x24, [sp, #48]
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #96
    ret

    NESTED_END call_exc_handler

    XDATAAREA
LABEL(call_exc_handler_xdata)
    // 22 instructions, one epilog starting at code 0, 4 code words
    dcd     0x20200016
    dcb     0x01                // alloc_s 16
    dcb     0xCA, 0x0A          // save_regp x27, 80
    dcb     0xC9, 0x88          // save_regp x25, 64
    dcb     0xC9, 0x06          // save_regp x23, 48
    dcb     0xC8, 0x84          // save_regp x21, 32
    dcb     0xC8, 0x02          // save_regp x19, 16
    dcb     0x8B                // save_fplr_x 96
    dcb     0xE4                // end
    dcb     0xE3, 0xE3, 0xE3    // nop padding

    PDATAAREA
    DCRVA   call_exc_handler
    DCRVA   call_exc_handler_xdata

    END
