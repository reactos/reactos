/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test helper for x64 RtlRemoteCall
 * COPYRIGHT:   Copyright 2025 Ratin Gao <ratin@knsoft.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

EXTERN RtlExitUserThread:PROC

.code64

PUBLIC RtlRemoteCall_TestProc
FUNC RtlRemoteCall_TestProc

    .ENDPROLOG

    /* Check preset value in registers */
    cmp rax, STATUS_ALERTED
    jne Fail
    mov rax, HEX(EBEBEBEBEBEBEBEB)
    cmp rbx, rax
    jne Fail
    mov rax, HEX(ECECECECECECECEC)
    cmp rcx, rax
    jne Fail
    mov rax, HEX(EDEDEDEDEDEDEDED)
    cmp rdx, rax
    jne Fail
    mov rax, HEX(EFEFEFEFEFEFEFEF)
    cmp rsi, rax
    jne Fail
    mov rax, HEX(FEFEFEFEFEFEFEFE)
    cmp rdi, rax
    jne Fail
    mov rax, HEX(0808080808080808)
    cmp r8, rax
    jne Fail
    mov rax, HEX(0909090909090909)
    cmp r9, rax
    jne Fail
    mov rax, HEX(1010101010101010)
    cmp r10, rax
    jne Fail

    /* Context is always written to the stack, whatever PassContext (rbp) is set or not */
    cmp dword ptr [rsp + CONTEXT_ContextFlags], CONTEXT_FULL
    jne Fail
    cmp qword ptr [rsp + CONTEXT_Rax], STATUS_ALERTED
    jne Fail
    mov rax, HEX(EBEBEBEBEBEBEBEB)
    cmp qword ptr [rsp + CONTEXT_Rbx], rax
    jne Fail
    mov rax, HEX(ECECECECECECECEC)
    cmp qword ptr [rsp + CONTEXT_Rcx], rax
    jne Fail
    mov rax, HEX(EDEDEDEDEDEDEDED)
    cmp qword ptr [rsp + CONTEXT_Rdx], rax
    jne Fail
    mov rax, HEX(EFEFEFEFEFEFEFEF)
    cmp qword ptr [rsp + CONTEXT_Rsi], rax
    jne Fail
    mov rax, HEX(FEFEFEFEFEFEFEFE)
    cmp qword ptr [rsp + CONTEXT_Rdi], rax
    jne Fail
    mov rax, HEX(0808080808080808)
    cmp qword ptr [rsp + CONTEXT_R8], rax
    jne Fail
    mov rax, HEX(0909090909090909)
    cmp qword ptr [rsp + CONTEXT_R9], rax
    jne Fail
    mov rax, HEX(1010101010101010)
    cmp qword ptr [rsp + CONTEXT_R10], rax
    jne Fail
    mov rax, HEX(1111111111111111)
    cmp qword ptr [rsp + CONTEXT_R11], rax
    jne Fail
    mov rax, HEX(1212121212121212)
    cmp qword ptr [rsp + CONTEXT_R12], rax
    jne Fail
    mov rax, HEX(1313131313131313)
    cmp qword ptr [rsp + CONTEXT_R13], rax
    jne Fail
    mov rax, HEX(1414141414141414)
    cmp qword ptr [rsp + CONTEXT_R14], rax
    jne Fail
    mov rax, HEX(1515151515151515)
    cmp qword ptr [rsp + CONTEXT_R15], rax
    jne Fail
    cmp qword ptr [rsp + CONTEXT_Rbp], rbp
    jne Fail
    mov rax, [rsp + CONTEXT_Rsp]
    sub rax, CONTEXT_FRAME_LENGTH
    cmp rax, rsp
    jne Fail

    /* Check input parameters, if PassContext (rbp) is set, the first parameter points to the context */
    test rbp, rbp
    je NotPassContext
    cmp r11, rsp
    jne Fail
    mov rax, HEX(AAAAAAAAAAAAAAAA)
    cmp r12, rax
    jne Fail
    mov rax, HEX(BBBBBBBBBBBBBBBB)
    cmp r13, rax
    jne Fail
    mov rax, HEX(CCCCCCCCCCCCCCCC)
    cmp r14, rax
    jne Fail
    mov rax, HEX(DDDDDDDDDDDDDDDD)
    cmp r15, rax
    jne Fail
    mov rcx, STATUS_SUCCESS
    jmp Exit
NotPassContext:
    mov rax, HEX(AAAAAAAAAAAAAAAA)
    cmp r11, rax
    jne Fail
    mov rax, HEX(BBBBBBBBBBBBBBBB)
    cmp r12, rax
    jne Fail
    mov rax, HEX(CCCCCCCCCCCCCCCC)
    cmp r13, rax
    jne Fail
    mov rax, HEX(DDDDDDDDDDDDDDDD)
    cmp r14, rax
    jne Fail
    mov rax, HEX(1515151515151515)
    cmp r15, rax
    jne Fail
    mov rcx, STATUS_SUCCESS
    jmp Exit

Fail:
    mov rcx, STATUS_INVALID_PARAMETER
Exit:
    call RtlExitUserThread
    int 3

ENDFUNC

END
