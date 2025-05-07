/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x86 asm implementation of ___wine_rtl_unwind
 * COPYRIGHT:   Copyright 1999, 2010 Alexandre Julliard
 */

#include <asm.inc>

// See wine: dlls/winecrt0/exception.c

.code

//
// DECLSPEC_NORETURN
// void
// __cdecl
// __wine_rtl_unwind(
//     EXCEPTION_REGISTRATION_RECORD* frame,
//     EXCEPTION_RECORD *record,
//     void (*target)(void) );
//
EXTERN _RtlUnwind@16:PROC
PUBLIC ___wine_rtl_unwind
___wine_rtl_unwind:
    push ebp
    //.cfi_adjust_cfa_offset 4
    //.cfi_rel_offset %ebp,0
    mov ebp, esp
    //.cfi_def_cfa_register ebp
    sub esp, 8
    push 0          /* retval */
    push [ebp + 12] /* record */
    push [ebp + 16] /* target */
    push [ebp + 8]  /* frame */
    call _RtlUnwind@16
    call dword ptr [ebp + 16]

    int HEX(2C)

END
