/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __longjmp_nounwind for x86
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

.code

PUBLIC ___longjmp_nounwind
___longjmp_nounwind:

    // Load Buf into edx
    mov edx, [esp + 4]

    // Load return value into eax
    mov eax, [esp + 8]

    // Restore non-volatile registers from the jump buffer
    mov ebp, [edx + JbEbp]
    mov ebx, [edx + JbEbx]
    mov edi, [edx + JbEdi]
    mov esi, [edx + JbEsi]

    // Restore stack (buffer contains callee stack pointer)
    mov esp, [edx + JbEsp]
    add esp, 4

    // Jump to the saved instruction pointer
    jmp dword ptr [edx + JbEip]

END
