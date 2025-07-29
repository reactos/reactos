/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x64 asm implementation of ___wine_longjmp
 * COPYRIGHT:   Copyright 1999, 2010 Alexandre Julliard
 */

#include <asm.inc>

// See wine: dlls/winecrt0/setjmp.c

.code64

PUBLIC __wine_longjmp
__wine_longjmp:
    mov rax, rdx                   /* retval */
    mov rbx, [rcx + HEX(8)]        /* jmp_buf->Rbx */
    mov rbp, [rcx + HEX(18)]       /* jmp_buf->Rbp */
    mov rsi, [rcx + HEX(20)]       /* jmp_buf->Rsi */
    mov rdi, [rcx + HEX(28)]       /* jmp_buf->Rdi */
    mov r12, [rcx + HEX(30)]       /* jmp_buf->R12 */
    mov r13, [rcx + HEX(38)]       /* jmp_buf->R13 */
    mov r14, [rcx + HEX(40)]       /* jmp_buf->R14 */
    mov r15, [rcx + HEX(48)]       /* jmp_buf->R15 */
    ldmxcsr [rcx + HEX(58)]        /* jmp_buf->MxCsr */
    fnclex
    fldcw [rcx + HEX(5c)]          /* jmp_buf->FpCsr */
    movdqa xmm6, [rcx + HEX(60)]   /* jmp_buf->Xmm6 */
    movdqa xmm7, [rcx + HEX(70)]   /* jmp_buf->Xmm7 */
    movdqa xmm8, [rcx + HEX(80)]   /* jmp_buf->Xmm8 */
    movdqa xmm9, [rcx + HEX(90)]   /* jmp_buf->Xmm9 */
    movdqa xmm10, [rcx + HEX(a0)]  /* jmp_buf->Xmm10 */
    movdqa xmm11, [rcx + HEX(b0)]  /* jmp_buf->Xmm11 */
    movdqa xmm12, [rcx + HEX(c0)]  /* jmp_buf->Xmm12 */
    movdqa xmm13, [rcx + HEX(d0)]  /* jmp_buf->Xmm13 */
    movdqa xmm14, [rcx + HEX(e0)]  /* jmp_buf->Xmm14 */
    movdqa xmm15, [rcx + HEX(f0)]  /* jmp_buf->Xmm15 */
    mov rdx, [rcx + HEX(50)]       /* jmp_buf->Rip */
    mov rsp, [rcx + HEX(10)]       /* jmp_buf->Rsp */
    jmp rdx

END
