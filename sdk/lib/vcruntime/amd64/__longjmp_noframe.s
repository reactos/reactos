/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __longjmp_noframe for x64.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

.code64

//
// __declspec(noreturn) void __longjmp_noframe(_JUMP_BUFFER* _Env, int _Value);
//
PUBLIC __longjmp_noframe
__longjmp_noframe:

    // Load floating point state
    ldmxcsr [rcx + JbMxCsr]
    fldcw [rcx + JbFpCsr]

    // Clear legacy exception status
    fnclex

    // Load non-volatile registers
    mov rbx, [rcx + JbRbx]
    mov rbp, [rcx + JbRbp]
    mov rsi, [rcx + JbRsi]
    mov rdi, [rcx + JbRdi]
    mov r12, [rcx + JbR12]
    mov r13, [rcx + JbR13]
    mov r14, [rcx + JbR14]
    mov r15, [rcx + JbR15]
    movdqa xmm6, [rcx + JbXmm6]
    movdqa xmm7, [rcx + JbXmm7]
    movdqa xmm8, [rcx + JbXmm8]
    movdqa xmm9, [rcx + JbXmm9]
    movdqa xmm10, [rcx + JbXmm10]
    movdqa xmm11, [rcx + JbXmm11]
    movdqa xmm12, [rcx + JbXmm12]
    movdqa xmm13, [rcx + JbXmm13]
    movdqa xmm14, [rcx + JbXmm14]
    movdqa xmm15, [rcx + JbXmm15]

    // Load rip into r8
    mov r8, [rcx + JbRip]

    // return _Value or 1 if _Value is 0
    mov eax, 1
    test edx, edx
    cmovnz eax, edx

    // Load the stack pointer
    mov rsp, [rcx + JbRsp]

    // Jump to the saved instruction pointer
    jmp qword ptr [rcx + JbRip]

END
