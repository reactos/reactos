/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ReactOS AMD64 architecture asm macros
 * COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <macamd64.inc>

MACRO(GENERATE_EXCEPTION_FRAME)

    /* Allocate a KEXCEPTION_FRAME on the stack */
    /* -8 because the last field is the return address */
    sub rsp, KEXCEPTION_FRAME_LENGTH - 8
    .allocstack (KEXCEPTION_FRAME_LENGTH - 8)

    /* Save non-volatiles in KEXCEPTION_FRAME */
    mov [rsp + ExRbp], rbp
    .savereg rbp, ExRbp
    mov [rsp + ExRbx], rbx
    .savereg rbx, ExRbx
    mov [rsp +ExRdi], rdi
    .savereg rdi, ExRdi
    mov [rsp + ExRsi], rsi
    .savereg rsi, ExRsi
    mov [rsp + ExR12], r12
    .savereg r12, ExR12
    mov [rsp + ExR13], r13
    .savereg r13, ExR13
    mov [rsp + ExR14], r14
    .savereg r14, ExR14
    mov [rsp + ExR15], r15
    .savereg r15, ExR15
    movaps [rsp + ExXmm6], xmm6
    .savexmm128 xmm6, ExXmm6
    movaps [rsp + ExXmm7], xmm7
    .savexmm128 xmm7, ExXmm7
    movaps [rsp + ExXmm8], xmm8
    .savexmm128 xmm8, ExXmm8
    movaps [rsp + ExXmm9], xmm9
    .savexmm128 xmm9, ExXmm9
    movaps [rsp + ExXmm10], xmm10
    .savexmm128 xmm10, ExXmm10
    movaps [rsp + ExXmm11], xmm11
    .savexmm128 xmm11, ExXmm11
    movaps [rsp + ExXmm12], xmm12
    .savexmm128 xmm12, ExXmm12
    movaps [rsp + ExXmm13], xmm13
    .savexmm128 xmm13, ExXmm13
    movaps [rsp + ExXmm14], xmm14
    .savexmm128 xmm14, ExXmm14
    movaps [rsp + ExXmm15], xmm15
    .savexmm128 xmm15, ExXmm15
    .endprolog
ENDM

MACRO(RESTORE_EXCEPTION_STATE)

    /* Restore non-volatile registers */
    mov rbp, [rsp + ExRbp]
    mov rbx, [rsp + ExRbx]
    mov rdi, [rsp + ExRdi]
    mov rsi, [rsp + ExRsi]
    mov r12, [rsp + ExR12]
    mov r13, [rsp + ExR13]
    mov r14, [rsp + ExR14]
    mov r15, [rsp + ExR15]
    movaps xmm6, [rsp + ExXmm6]
    movaps xmm7, [rsp + ExXmm7]
    movaps xmm8, [rsp + ExXmm8]
    movaps xmm9, [rsp + ExXmm9]
    movaps xmm10, [rsp + ExXmm10]
    movaps xmm11, [rsp + ExXmm11]
    movaps xmm12, [rsp + ExXmm12]
    movaps xmm13, [rsp + ExXmm13]
    movaps xmm14, [rsp + ExXmm14]
    movaps xmm15, [rsp + ExXmm15]

    /* Clean stack and return */
    add rsp, KEXCEPTION_FRAME_LENGTH - 8

ENDM
