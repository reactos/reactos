/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x86 ASM helper functions for MOV/POP SS bug
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

.code

PUBLIC _RunMovSsAndSyscall
_RunMovSsAndSyscall:

    // Load invalid syscall id
    mov eax, 100000

    mov ax, ss
    mov ss, ax

PUBLIC _SyscallInstruction
_SyscallInstruction:
    int HEX(2E)
    ret

END
