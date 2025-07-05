/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _seh_longjmp_unwind sor x86
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

EXTERN __local_unwind2:PROC

.code

//
// void __stdcall _seh_longjmp_unwind(_JUMP_BUFFER* _Buf);
//
PUBLIC __seh_longjmp_unwind@4
__seh_longjmp_unwind@4:

    // Save ebp
    push ebp

    // Load the jump buffer address into eax
    mov eax, [esp + 8]

    // Call _local_unwind2 to run the termination handlers
    push dword ptr [eax + JbTryLevel] // _Buf->TryLevel
    push dword ptr [eax + JbRegistration] // _Buf->Registration
    mov ebp, [eax + JbEbp] // _Buf->Ebp
    call __local_unwind2

    // Clean up the stack and restore ebp
    add esp, 8
    pop ebp
    ret 4

END
