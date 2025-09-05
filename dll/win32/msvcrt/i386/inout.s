/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x86 asm implementation of _inp, _inpw, _inpd
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

.code

PUBLIC __inp
__inp:
    mov dx, [esp + 8]
    in al, dx
    ret

PUBLIC __inpw
__inpw:
    mov dx, [esp + 8]
    in ax, dx
    ret

PUBLIC __inpd
__inpd:
    mov dx, [esp + 8]
    in eax, dx
    ret

PUBLIC __outp
__outp:
    mov dx, [esp + 8]
    mov al, [esp + 12]
    out dx, al
    ret

PUBLIC __outpw
__outpw:
    mov dx, [esp + 8]
    mov ax, [esp + 12]
    out dx, ax
    ret

PUBLIC __outpd
__outpd:
    mov dx, [esp + 8]
    mov eax, [esp + 12]
    out dx, eax
    ret

END
