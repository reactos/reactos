/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x64 asm code for except_x86_64.c
 * COPYRIGHT:   Copyright 2017-2024 Wine team
 */

#include <asm.inc>

// See msvcrt/except_x86_64.c

.code64

// void *call_exc_handler( void *handler, ULONG_PTR frame, UINT flags );
PUBLIC call_exc_handler
.PROC call_exc_handler
    sub rsp, 40
    .allocstack 40
    .endprolog

    mov [rsp], rcx
    mov [rsp + 8], r8d
    mov [rsp + 16], rdx
    call rcx
    add rsp, 40
    ret
.ENDP

END
