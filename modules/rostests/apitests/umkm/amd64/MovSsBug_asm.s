/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x64 ASM helper functions for MOV/POP SS bug
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

.code64

PUBLIC RunMovSsAndSyscall
RunMovSsAndSyscall:

    // Load invalid syscall id
    mov rax, 100000

    mov ax, ss
    mov ss, ax

PUBLIC SyscallInstruction
SyscallInstruction:
    syscall
    ret

END
