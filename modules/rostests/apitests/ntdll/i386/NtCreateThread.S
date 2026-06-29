/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Helper functions for NtCreateThread test
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>
#include <ks386.inc>

.code

EXTERN _TestThreadProcPtr:DWORD
EXTERN _TestThreadStartupContext:DWORD

TempEflags:
    .long 0

PUBLIC _ThreadStartupThunk
.PROC _ThreadStartupThunk

    pushfd
    pop dword ptr [_TestThreadStartupContext + CsEflags]

    /* Integer registers */
    mov [_TestThreadStartupContext + CsEax], eax
    mov [_TestThreadStartupContext + CsEbx], ebx
    mov [_TestThreadStartupContext + CsEcx], ecx
    mov [_TestThreadStartupContext + CsEdx], edx
    mov [_TestThreadStartupContext + CsEsi], esi
    mov [_TestThreadStartupContext + CsEdi], edi
    mov [_TestThreadStartupContext + CsEbp], ebp

    /* Control */
    mov [_TestThreadStartupContext + CsEsp], esp
    lea eax, _ThreadStartupThunk
    mov [_TestThreadStartupContext + CsEip], eax


    /* Segments */
    xor eax, eax
    mov ax, cs
    mov [_TestThreadStartupContext + CsSegCs], eax
    mov ax, ds
    mov [_TestThreadStartupContext + CsSegDs], eax
    mov ax, es
    mov [_TestThreadStartupContext + CsSegEs], eax
    mov ax, fs
    mov [_TestThreadStartupContext + CsSegFs], eax
    mov ax, gs
    mov [_TestThreadStartupContext + CsSegGs], eax
    mov ax, ss
    mov [_TestThreadStartupContext + CsSegSs], eax

    // TODO: floating point registers

    jmp dword ptr [_TestThreadProcPtr]

.ENDP

END
