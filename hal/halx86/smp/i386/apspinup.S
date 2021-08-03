/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for protected mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC _APSpinup
PUBLIC _APSpinupEnd
PUBLIC _BSPCr3
EXTERN _APEntry:DWORD

.code32
_APSpinup:

    /* TODO: replace this with something that's going to actually work */
    xor eax, eax
    mov eax, cs:[_BSPCr3 - _APSpinup]
    mov cr3, eax

    hlt

    mov eax, cr0
    or  eax, HEX(80000000)
    mov cr0, eax

    hlt
_BSPCr3:
    .long HEX(0)

_APSpinupEnd:
END
 