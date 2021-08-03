/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for real mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC APEntry
PUBLIC APEntryEnd
PUBLIC APJumpOffset
PUBLIC BSPCr3

.code
APEntry:
    cli

    xor ax, ax
	mov ds, ax
	mov ss, ax
    mov fs, ax
    mov gs, ax

    hlt

APJumpOffset:
    .word HEX(0)

BSPCr3:
    .long HEX(0)

APEntryEnd:
END
