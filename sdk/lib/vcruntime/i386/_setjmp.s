/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _setjmp for x86
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

.code

// See
// - MSDN: https://learn.microsoft.com/en-us/cpp/c-runtime-library/setjmp3?view=msvc-170
// - LLVM https://llvm.org/doxygen/X86WinEHState_8cpp_source.html

//
// int
// ___intrinsic_setjmp(
//     _JUMP_BUFFER* Buf);
//
PUBLIC ___intrinsic_setjmp
___intrinsic_setjmp:
PUBLIC __setjmp
__setjmp:

    // Load Buf into edx
    mov edx, [esp + 4]

    // Save non-volatile registers in the jump buffer
    mov [edx + JbEbp], ebp
    mov [edx + JbEbx], ebx
    mov [edx + JbEsi], esi
    mov [edx + JbEdi], edi

    // Save the return address
    mov eax, [esp]
    mov [edx + JbEip], eax

    // Save the *current* stack pointer
    mov [edx + JbEsp], esp

    // Save the SEH registration
    mov eax, fs:[0]
    mov [edx + JbRegistration], eax

    // Check if we have an SEH registration
    cmp eax, HEX(FFFFFFFF)
    je .no_seh_registration

    // Get the TryLevel from the registration frame
    mov eax, [eax + 12] // fs:[0]->TryLevel

.no_seh_registration:

    // Save the current try level or 0xFFFFFFFF if no SEH registration
    mov [edx + JbTryLevel], eax

    // Return value is 0
    xor eax, eax
    ret

END
