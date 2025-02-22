/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test helper for x86 RtlUnwind
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

.code

EXTERN _g_InContext:DWORD
EXTERN _g_OutContext:DWORD
EXTERN _RtlUnwind@16:PROC

//
// VOID
// WINAPI
// RtlUnwindWrapper(
//     _In_ PVOID TargetFrame,
//     _In_ PVOID TargetIp,
//     _In_ PEXCEPTION_RECORD ExceptionRecord,
//     _In_ PVOID ReturnValue);
//
PUBLIC _RtlUnwindWrapper@16
FUNC _RtlUnwindWrapper@16

    push ebp
    mov ebp, esp

    /* Save non-volatile registers */
    push ebx
    push esi
    push edi

    /* Load registers from the in-context */
    mov eax, _g_InContext[CONTEXT_EAX]
    mov ebx, _g_InContext[CONTEXT_EBX]
    mov ecx, _g_InContext[CONTEXT_ECX]
    mov edx, _g_InContext[CONTEXT_EDX]
    mov esi, _g_InContext[CONTEXT_ESI]
    mov edi, _g_InContext[CONTEXT_EDI]

    /* Call the native function */
    push dword ptr [ebp + 20] // ReturnValue
    push dword ptr [ebp + 16] // ExceptionRecord
    push dword ptr [ebp + 12] // TargetIp
    push dword ptr [ebp + 8]  // TargetFrame
    call _RtlUnwind@16

    /* Save registers in the out-context */
    mov _g_OutContext[CONTEXT_EAX], eax
    mov _g_OutContext[CONTEXT_EBX], ebx
    mov _g_OutContext[CONTEXT_ECX], ecx
    mov _g_OutContext[CONTEXT_EDX], edx
    mov _g_OutContext[CONTEXT_ESI], esi
    mov _g_OutContext[CONTEXT_EDI], edi
    mov word ptr _g_OutContext[CONTEXT_SEGCS], cs
    mov word ptr _g_OutContext[CONTEXT_SEGDS], ds
    mov word ptr _g_OutContext[CONTEXT_SEGES], es
    mov word ptr _g_OutContext[CONTEXT_SEGFS], fs

    /* Restore non-volatile registers */
    pop edi
    pop esi
    pop ebx

    mov esp, ebp
    pop ebp
    ret 16

ENDFUNC


END
