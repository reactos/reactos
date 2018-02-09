;++
; PROJECT:     ReactOS SDK
; LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
; PURPOSE:     ReactOS AMD64 architecture asm macros
; COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
;--

#include <ksamd64.inc>

MACRO(GENERATE_EXCEPTION_FRAME)

    ; Allocate a KEXCEPTION_FRAME on the stack
    ; -8 because the last field is the return address
    sub rsp, KEXCEPTION_FRAME_LENGTH - 8
    .allocstack KEXCEPTION_FRAME_LENGTH - 8

    /* save non-volatiles in KEXCEPTION_FRAME */
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
    movdqa [rsp + ExXmm6], xmm6
    .savexmm128 xmm6, ExXmm6
    movdqa [rsp + ExXmm7], xmm7
    .savexmm128 xmm7, ExXmm7
    movdqa [rsp + ExXmm8], xmm8
    .savexmm128 xmm8, ExXmm8
    movdqa [rsp + ExXmm9], xmm9
    .savexmm128 xmm9, ExXmm9
    movdqa [rsp + ExXmm10], xmm10
    .savexmm128 xmm10, ExXmm10
    movdqa [rsp + ExXmm11], xmm11
    .savexmm128 xmm11, ExXmm11
    movdqa [rsp + ExXmm12], xmm12
    .savexmm128 xmm12, ExXmm12
    movdqa [rsp + ExXmm13], xmm13
    .savexmm128 xmm13, ExXmm13
    movdqa [rsp + ExXmm14], xmm14
    .savexmm128 xmm14, ExXmm14
    movdqa [rsp + ExXmm15], xmm15
    .savexmm128 xmm15, ExXmm15
    .endprolog
ENDM

MACRO(RESTORE_EXCEPTION_STATE)

    /* restore non-volatile registers */
    mov rbp, [rsp + KEXCEPTION_FRAME_Rbp]
    mov rbx, [rsp + KEXCEPTION_FRAME_Rbx]
    mov rdi, [rsp + KEXCEPTION_FRAME_Rdi]
    mov rsi, [rsp + KEXCEPTION_FRAME_Rsi]
    mov r12, [rsp + KEXCEPTION_FRAME_R12]
    mov r13, [rsp + KEXCEPTION_FRAME_R13]
    mov r14, [rsp + KEXCEPTION_FRAME_R14]
    mov r15, [rsp + KEXCEPTION_FRAME_R15]
    movdqa xmm6, [rsp + KEXCEPTION_FRAME_Xmm6]
    movdqa xmm7, [rsp + KEXCEPTION_FRAME_Xmm7]
    movdqa xmm8, [rsp + KEXCEPTION_FRAME_Xmm8]
    movdqa xmm9, [rsp + KEXCEPTION_FRAME_Xmm9]
    movdqa xmm10, [rsp + KEXCEPTION_FRAME_Xmm10]
    movdqa xmm11, [rsp + KEXCEPTION_FRAME_Xmm11]
    movdqa xmm12, [rsp + KEXCEPTION_FRAME_Xmm12]
    movdqa xmm13, [rsp + KEXCEPTION_FRAME_Xmm13]
    movdqa xmm14, [rsp + KEXCEPTION_FRAME_Xmm14]
    movdqa xmm15, [rsp + KEXCEPTION_FRAME_Xmm15]

    /* Clean stack and return */
    add rsp, KEXCEPTION_FRAME_LENGTH - 8

ENDM
