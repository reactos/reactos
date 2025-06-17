/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test helper for x86 RtlRemoteCall
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#include <asm.inc>
#include <ks386.inc>

EXTERN _RtlExitUserThread@4:PROC

#define RTL_REMOTECALL_MAX_ARG_COUNT 4

.code

PUBLIC _RtlRemoteCall_TestProc@0
FUNC _RtlRemoteCall_TestProc@0

    /* Check preset value in registers */
    cmp eax, HEX(EAEAEAEA)
    jne Fail
    cmp ebx, HEX(EBEBEBEB)
    jne Fail
    cmp ecx, HEX(ECECECEC)
    jne Fail
    cmp edx, HEX(EDEDEDED)
    jne Fail
    cmp esi, HEX(EFEFEFEF)
    jne Fail
    cmp edi, HEX(FEFEFEFE)
    jne Fail

    /* If PassContext (ebp) is set, the first parameter points to the context */
    test ebp, ebp
    je SkipVerifyContext
    mov ebx, [esp]
    cmp dword ptr [ebx + CONTEXT_FLAGS], CONTEXT_FULL
    jne Fail
    cmp dword ptr [ebx + CONTEXT_EAX], HEX(EAEAEAEA)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_EBX], HEX(EBEBEBEB)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_ECX], HEX(ECECECEC)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_EDX], HEX(EDEDEDED)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_ESI], HEX(EFEFEFEF)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_EDI], HEX(FEFEFEFE)
    jne Fail
    cmp dword ptr [ebx + CONTEXT_EBP], ebp
    jne Fail
    mov eax, [ebx + CONTEXT_ESP]
    sub eax, (RTL_REMOTECALL_MAX_ARG_COUNT + 1) * 4 + CONTEXT_FRAME_LENGTH
    cmp eax, esp
    jne Fail
    add esp, 4
SkipVerifyContext:

    /* Check input parameters */
    cmp dword ptr [esp], HEX(AAAAAAAA)
    jne Fail
    cmp dword ptr [esp + 4], HEX(BBBBBBBB)
    jne Fail
    cmp dword ptr [esp + 8], HEX(CCCCCCCC)
    jne Fail
    cmp dword ptr [esp + 12], HEX(DDDDDDDD)
    jne Fail

    /* Success */
    push STATUS_SUCCESS
    jmp Exit

Fail:
    push STATUS_INVALID_PARAMETER
Exit:
    call _RtlExitUserThread@4
    int 3

ENDFUNC

END
