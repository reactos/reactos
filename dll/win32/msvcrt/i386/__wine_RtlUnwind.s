/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x86 asm implementation of __wine__RtlUnwind
 * COPYRIGHT:   Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

.code

EXTERN _RtlUnwind@16:PROC

// ASM wrapper for Wine code. This is needed, because Wine code expects
// RtlUnwind to restore the non-volatile registers, before returning, but
// ours / the native one does not do that.
//
// void
// WINAPI
// __wine__RtlUnwind(
//     PVOID TargetFrame,
//     PVOID TargetIp ,
//     PEXCEPTION_RECORD ExceptionRecord ,
//     PVOID ReturnValue);
//
PUBLIC ___wine_RtlUnwind@16
___wine_RtlUnwind@16:

    push ebp
    mov ebp, esp

    /* Save non-volatile registers */
    push ebx
    push esi
    push edi

    /* Call the native function */
    push dword ptr [ebp + 20] // ReturnValue
    push dword ptr [ebp + 16] // ExceptionRecord
    push dword ptr [ebp + 12] // TargetIp
    push dword ptr [ebp + 8]  // TargetFrame
    call _RtlUnwind@16

    /* Restore non-volatile registers */
    pop edi
    pop esi
    pop ebx

    mov esp, ebp
    pop ebp
    ret 16


PUBLIC __imp____wine_RtlUnwind@16
__imp____wine_RtlUnwind@16:
    .long ___wine_RtlUnwind@16

END
