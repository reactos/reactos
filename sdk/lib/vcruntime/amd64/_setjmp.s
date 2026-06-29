/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __intrinsic_setjmp for x64.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

.code64

//
// int __intrinsic_setjmp(_JUMP_BUFFER* _Env, void* _Frame);
//
PUBLIC _setjmp
_setjmp:
PUBLIC _setjmpex
_setjmpex:
PUBLIC __intrinsic_setjmp
__intrinsic_setjmp:
PUBLIC __intrinsic_setjmpex
__intrinsic_setjmpex:

    // Load rsp as it was before the call into rax
    lea rax, [rsp + 8]

    // Load return address into r8
    mov r8, [rsp]

    // Save the Frame passed in rdx
    mov [rcx + JbFrame], rdx

    // Save floating point state
    stmxcsr [rcx + JbMxCsr]
    fnstcw [rcx + JbFpCsr]

    // Save the non-volatile registers
    mov [rcx + JbRbx], rbx
    mov [rcx + JbRbp], rbp
    mov [rcx + JbRsi], rsi
    mov [rcx + JbRdi], rdi
    mov [rcx + JbR12], r12
    mov [rcx + JbR13], r13
    mov [rcx + JbR14], r14
    mov [rcx + JbR15], r15
    mov [rcx + JbRsp], rax
    mov [rcx + JbRip], r8
    movdqa [rcx + JbXmm6], xmm6
    movdqa [rcx + JbXmm7], xmm7
    movdqa [rcx + JbXmm8], xmm8
    movdqa [rcx + JbXmm9], xmm9
    movdqa [rcx + JbXmm10], xmm10
    movdqa [rcx + JbXmm11], xmm11
    movdqa [rcx + JbXmm12], xmm12
    movdqa [rcx + JbXmm13], xmm13
    movdqa [rcx + JbXmm14], xmm14
    movdqa [rcx + JbXmm15], xmm15

    // Return 0
    xor rax, rax
    ret

END
