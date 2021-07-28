/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for protected mode AP code
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#include <asm.inc>

PUBLIC _APSpinup
PUBLIC _APSpinupEnd

EXTERN _APEntry:DWORD
EXTERN _BSPCr3:DWORD
.code32
_APSpinup:
    mov   bx, HEX(10)
    mov   ds, bx
    mov   es, bx
    mov   ss, bx
    mov   fs, bx

    hlt
    ;xor eax, eax
    ;mov eax, HEX(4000) + [_BSPCr3]
    ;sub eax, [_APEntry]
    ;mov cr3, eax 

    ;xor   eax, eax
    ;mov   eax, HEX(3000000)
    ;mov   cr3, eax

    mov eax, cr0
    or  eax, HEX(80000000)
    mov cr0, eax
    hlt

    hlt
_APSpinupEnd:
END
