/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test helper for x86 setjmp
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

#ifdef TEST_UCRTBASE
#define __setjmp ___intrinsic_setjmp
#endif

.code

PUBLIC _get_sp
_get_sp:
    lea eax, [esp + 4]
    ret

EXTERN __setjmp:PROC
PUBLIC _setjmp_return_address
EXTERN __setjmp3:PROC

PUBLIC __setjmp1
__setjmp1:
    jmp __setjmp

PUBLIC _call_setjmp
_call_setjmp:
    push offset __setjmp
    jmp _call_setjmp_common

PUBLIC _call_setjmp3
_call_setjmp3:
    push offset __setjmp3
    jmp _call_setjmp_common


_call_setjmp_common:
    // Save non-volatile registers
    push ebp
    push ebx
    push edi
    push esi

    // Set up a register state
    mov ebp, HEX(A1A1A1A1)
    mov ebx, HEX(A2A2A2A2)
    mov edi, HEX(A3A3A3A3)
    mov esi, HEX(A4A4A4A4)

    // First push parameters for _setjmp3
    push HEX(3456789A) // Param 3
    push HEX(23456789) // Param 2
    push HEX(12345678) // Param 1
    push HEX(00000007) // TryLevel
    push HEX(00012345) // UnwindFunc
    push HEX(00000005) // Count

    // Common parameter for both _setjmp and _setjmp3
    push dword ptr [esp + 48]
    call dword ptr [esp + 44]
_setjmp_return_address:

    add esp, 28

    // Restore non-volatile registers
    pop esi
    pop edi
    pop ebx
    pop ebp

    // Clean up the stack
    add esp, 4

    ret

END
