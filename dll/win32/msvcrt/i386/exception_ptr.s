/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x86 asm code for except_i386.c
 * COPYRIGHT:   Copyright 2017-2024 Wine team
 */

#include <asm.inc>

// See msvcrt/exception_ptr.c

.code

PUBLIC _call_copy_ctor
_call_copy_ctor:
    push ebp
    //.cfi_adjust_cfa_offset 4
    //.cfi_rel_offset ebp, 0
    mov ebp, esp
    //.cfi_def_cfa_register ebp
    push 1
    mov ecx, [ebp + 12]
    push dword ptr [ebp + 16]
    call dword ptr [ebp + 8]
    //.cfi_def_cfa esp, 4
    //.cfi_same_value ebp
    leave
    ret

PUBLIC _call_dtor
_call_dtor:
    mov ecx, [esp + 8]
    call dword ptr [esp + 4]
    ret

END
