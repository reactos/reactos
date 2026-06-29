/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Helper functions for NtCreateThread test
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

.code

EXTERN _RtlCaptureContext@4:PROC
EXTERN _TestThreadProcPtr:DWORD
EXTERN _TestThreadStartupContext:DWORD

TempEflags:
    .long 0

PUBLIC _ThreadStartupThunk
.PROC _ThreadStartupThunk

    push ebp
    mov ebp, esp

    /* Save EFlags */
    pushfd

    /* Clear direction flag */
    cld

    /* Capture the context */
    push offset _TestThreadStartupContext
    call _RtlCaptureContext@4

    /* Fix up control registers */
    pop eax
    mov [_TestThreadStartupContext + CsEflags], eax
    lea eax, [esp + 4]
    mov [_TestThreadStartupContext + CsEsp], eax
    lea eax, _ThreadStartupThunk
    mov [_TestThreadStartupContext + CsEip], eax

    pop ebp
    jmp dword ptr [_TestThreadProcPtr]

.ENDP

END
