/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt handling
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#include <asm.inc>

#define PIC1_CONTROL_PORT   HEX(20)
#define PIC_EOI             HEX(20)

.code32

PUBLIC _HwIrqHandler
_HwIrqHandler:
    push ax

    /* Increment the interrupt count */
    inc dword ptr ds:[_HwIrqCount]

    /* Dismiss the interrupt */
    mov al, PIC_EOI
    out PIC1_CONTROL_PORT, al

    pop ax
    iret

PUBLIC _HwIrqCount
_HwIrqCount:
    .long 0

END
