/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _setjmp3 for x86
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

#define _JUMP_BUFFER_COOKIE HEX(56433230)

.code

// See
// - MSDN: https://learn.microsoft.com/en-us/cpp/c-runtime-library/setjmp3?view=msvc-170
// - LLVM https://llvm.org/doxygen/X86WinEHState_8cpp_source.html

//
// int
// _setjmp3(
//     _JUMP_BUFFER* Buf,
//     unsigned Count,
//     void* UnwindFunc,
//     unsigned TryLevel,
//     ...);
//
PUBLIC __setjmp3
__setjmp3:

    // Load Buf into edx
    mov edx, [esp + 4]

    // Load Count into ecx
    mov ecx, [esp + 8]

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

    // Store cookie
    mov dword ptr [edx + JbCookie], _JUMP_BUFFER_COOKIE

    // Initialize UnwindFunc to NULL and TryLevel to -1
    xor eax, eax
    mov [edx + JbUnwindFunc], eax
    dec eax
    mov [edx + JbTryLevel], eax

    // Save the SEH registration
    mov eax, fs:[0]
    mov [edx + JbRegistration], eax

    // Check if we have an SEH registration
    cmp eax, HEX(FFFFFFFF)
    je .done

    // Save the TryLevel from the registration frame
    mov eax, [eax + 12] // fs:[0]->TryLevel
    mov [edx + JbTryLevel], eax

    // If we have 0 parameters we are done
    cmp ecx, 0
    je .done

    // Save UnwindFunc parameter
    mov eax, [esp + 12]
    mov [edx + JbUnwindFunc], eax

    // Decrement Count, check if we have more parameters
    dec ecx
    jnz .more_than_1

    // This is likely a bug in the MS implementation, we emulate it
    mov eax, [eax + 12]
    mov [edx + JbTryLevel], eax
    jmp .done

.more_than_1:
    // Save TryLevel parameter
    mov eax, [esp + 16]
    mov [edx + JbTryLevel], eax

    // Decrement Count again, if it is now 0, we are done
    dec ecx
    jz .done

    // Make sure the count does not exceed the maximum number of parameters (6)
    cmp ecx, 6
    jbe .copy_data
    mov ecx, 6

.copy_data:
    // Copy the remaining parameters to the jump buffer
    push esi
    push edi
    lea esi, [esp + 28]
    lea edi, [edx + JbUnwindData]
    rep movsd
    pop edi
    pop esi

.done:
    // Return value is 0
    xor eax, eax
    ret

END
