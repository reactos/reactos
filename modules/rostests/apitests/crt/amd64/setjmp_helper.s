/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test helper for x64 setjmp
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

#ifdef TEST_UCRTBASE
#define _setjmp __intrinsic_setjmp
#define _setjmpex __intrinsic_setjmpex
#endif

.const

.align 16
xmm6_data:
    .quad HEX(0606060606060606), HEX(1616161616161616)
xmm7_data:
    .quad HEX(0707070707070707), HEX(1717171717171717)
xmm8_data:
    .quad HEX(0808080808080808), HEX(1818181818181818)
xmm9_data:
    .quad HEX(0909090909090909), HEX(1919191919191919)
xmm10_data:
    .quad HEX(0A0A0A0A0A0A0A0A), HEX(1A1A1A1A1A1A1A1A)
xmm11_data:
    .quad HEX(0B0B0B0B0B0B0B0B), HEX(1B1B1B1B1B1B1B1B)
xmm12_data:
    .quad HEX(0C0C0C0C0C0C0C0C), HEX(1C1C1C1C1C1C1C1C)
xmm13_data:
    .quad HEX(0D0D0D0D0D0D0D0D), HEX(1D1D1D1D1D1D1D1D)
xmm14_data:
    .quad HEX(0E0E0E0E0E0E0E0E), HEX(1E1E1E1E1E1E1E1E)
xmm15_data:
    .quad HEX(0F0F0F0F0F0F0F0F), HEX(1F1F1F1F1F1F1F1F)


.code64

PUBLIC get_sp
get_sp:
    lea rax, [rsp + 8]
    ret


EXTERN _setjmp:PROC
EXTERN _setjmpex:PROC
PUBLIC setjmp_return_address

PUBLIC call_setjmp
call_setjmp:
    lea r10, _setjmp[rip]
    jmp call_setjmp_common

PUBLIC call_setjmpex
call_setjmpex:
    lea r10, _setjmpex[rip]
    jmp call_setjmp_common

.PROC call_setjmp_common

    // Allocate space for non-volatile (normal+xmm) registers on the stack
    sub rsp, 8 * 8 + 10 * 16 + 8 // +8 to align the stack to 16 bytes

    // Save non-volatile registers
    mov [rsp + 0 * 8], rbx
    mov [rsp + 1 * 8], rbp
    mov [rsp + 2 * 8], rsi
    mov [rsp + 3 * 8], rdi
    mov [rsp + 4 * 8], r12
    mov [rsp + 5 * 8], r13
    mov [rsp + 6 * 8], r14
    mov [rsp + 7 * 8], r15

    // Save non-volatile xmm registers
    movdqa xmmword ptr [rsp + 8 * 8 + 0 * 16], xmm6
    movdqa xmmword ptr [rsp + 8 * 8 + 1 * 16], xmm7
    movdqa xmmword ptr [rsp + 8 * 8 + 2 * 16], xmm8
    movdqa xmmword ptr [rsp + 8 * 8 + 3 * 16], xmm9
    movdqa xmmword ptr [rsp + 8 * 8 + 4 * 16], xmm10
    movdqa xmmword ptr [rsp + 8 * 8 + 5 * 16], xmm11
    movdqa xmmword ptr [rsp + 8 * 8 + 6 * 16], xmm12
    movdqa xmmword ptr [rsp + 8 * 8 + 7 * 16], xmm13
    movdqa xmmword ptr [rsp + 8 * 8 + 8 * 16], xmm14
    movdqa xmmword ptr [rsp + 8 * 8 + 9 * 16], xmm15

    .ENDPROLOG

    // Set up a register state
    mov rbx, HEX(A1A1A1A1A1A1A1A1)
    mov rbp, HEX(A2A2A2A2A2A2A2A2)
    mov rsi, HEX(A3A3A3A3A3A3A3A3)
    mov rdi, HEX(A4A4A4A4A4A4A4A4)
    mov r12, HEX(ACACACACACACACAC)
    mov r13, HEX(ADADADADADADADAD)
    mov r14, HEX(AEAEAEAEAEAEAEAE)
    mov r15, HEX(AFAFAFAFAFAFAFAF)
    movdqa xmm6,  xmmword ptr xmm6_data[rip]
    movdqa xmm7,  xmmword ptr xmm7_data[rip]
    movdqa xmm8,  xmmword ptr xmm8_data[rip]
    movdqa xmm9,  xmmword ptr xmm9_data[rip]
    movdqa xmm10, xmmword ptr xmm10_data[rip]
    movdqa xmm11, xmmword ptr xmm11_data[rip]
    movdqa xmm12, xmmword ptr xmm12_data[rip]
    movdqa xmm13, xmmword ptr xmm13_data[rip]
    movdqa xmm14, xmmword ptr xmm14_data[rip]
    movdqa xmm15, xmmword ptr xmm15_data[rip]

    mov rdx, rsp
    call r10 // _setjmp or _setjmpex
GLOBAL_LABEL setjmp_return_address

    // Restore non-volatile registers
    mov rbx, [rsp + 0 * 8]
    mov rbp, [rsp + 1 * 8]
    mov rsi, [rsp + 2 * 8]
    mov rdi, [rsp + 3 * 8]
    mov r12, [rsp + 4 * 8]
    mov r13, [rsp + 5 * 8]
    mov r14, [rsp + 6 * 8]
    mov r15, [rsp + 7 * 8]

    // Restore non-volatile xmm registers
    movdqa xmm6,  xmmword ptr [rsp + 8 * 8 + 0 * 16]
    movdqa xmm7,  xmmword ptr [rsp + 8 * 8 + 1 * 16]
    movdqa xmm8,  xmmword ptr [rsp + 8 * 8 + 2 * 16]
    movdqa xmm9,  xmmword ptr [rsp + 8 * 8 + 3 * 16]
    movdqa xmm10, xmmword ptr [rsp + 8 * 8 + 4 * 16]
    movdqa xmm11, xmmword ptr [rsp + 8 * 8 + 5 * 16]
    movdqa xmm12, xmmword ptr [rsp + 8 * 8 + 6 * 16]
    movdqa xmm13, xmmword ptr [rsp + 8 * 8 + 7 * 16]
    movdqa xmm14, xmmword ptr [rsp + 8 * 8 + 8 * 16]
    movdqa xmm15, xmmword ptr [rsp + 8 * 8 + 9 * 16]

    add rsp, 8 * 8 + 10 * 16 + 8 // Restore stack pointer
    ret

.ENDP

END
