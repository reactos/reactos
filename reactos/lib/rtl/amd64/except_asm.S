/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library (RTL)
 * FILE:            lib/rtl/amd64/except_asm.S
 * PURPOSE:         Exception support for AMD64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

/* FUNCTIONS *****************************************************************/

/* 
 * VOID NTAPI
 * RtlCaptureContext(
 *     PCONTEXT ContextRecord); <rcx>
 */
.func RtlCaptureContext
.global _RtlCaptureContext
_RtlCaptureContext:
    .cfi_startproc

    /* Push rflags */
    pushfq
    .cfi_adjust_cfa_offset 8

    /* Save the basic register context */
    mov [rcx + CONTEXT_Rax], rax
    mov [rcx + CONTEXT_Rcx], rcx
    mov [rcx + CONTEXT_Rdx], rdx

    /* Load rflags into rax */
    mov rax, [rsp]

    mov [rcx + CONTEXT_Rbx], rbx
    mov [rcx + CONTEXT_Rsi], rsi
    mov [rcx + CONTEXT_Rdi], rdi

    /* Store rflags */
    mov [rcx + CONTEXT_EFlags], rax

    mov [rcx + CONTEXT_Rbp], rbp
    mov [rcx + CONTEXT_R8], r8
    mov [rcx + CONTEXT_R9], r9

    /* Load former stack pointer in rax */
    lea rax, [rsp + 0x10]

    mov [rcx + CONTEXT_R10], r10
    mov [rcx + CONTEXT_R11], r11
    mov [rcx + CONTEXT_R12], r12

    /* Store stack pointer */
    mov [rcx + CONTEXT_Rsp], rax

    mov [rcx + CONTEXT_R13], r13
    mov [rcx + CONTEXT_R14], r14
    mov [rcx + CONTEXT_R15], r15

    /* Load return address in rax */
    mov rax, [rsp + 8]

    /* Safe segment selectors */
    mov [rcx + CONTEXT_SegCs], cs
    mov [rcx + CONTEXT_SegDs], ds
    mov [rcx + CONTEXT_SegEs], es
    mov [rcx + CONTEXT_SegFs], fs
    mov [rcx + CONTEXT_SegGs], gs
    mov [rcx + CONTEXT_SegSs], ss

    /* Store return address */
    mov [rcx + CONTEXT_Rip], rax

    /* Safe mmx registers */
/*    movdqa [rcx + CONTEXT_Xmm0], xmm0
    movdqa [rcx + CONTEXT_Xmm1], xmm1
    movdqa [rcx + CONTEXT_Xmm2], xmm2
    movdqa [rcx + CONTEXT_Xmm3], xmm3
    movdqa [rcx + CONTEXT_Xmm4], xmm4
    movdqa [rcx + CONTEXT_Xmm5], xmm5
    movdqa [rcx + CONTEXT_Xmm6], xmm6
    movdqa [rcx + CONTEXT_Xmm7], xmm7
    movdqa [rcx + CONTEXT_Xmm8], xmm8
    movdqa [rcx + CONTEXT_Xmm9], xmm9
    movdqa [rcx + CONTEXT_Xmm10], xmm10
    movdqa [rcx + CONTEXT_Xmm11], xmm11
    movdqa [rcx + CONTEXT_Xmm12], xmm12
    movdqa [rcx + CONTEXT_Xmm13], xmm13
    movdqa [rcx + CONTEXT_Xmm14], xmm14
    movdqa [rcx + CONTEXT_Xmm15], xmm15
*/
    /* Cleanup stack and return */
    add rsp, 8
    ret
    .cfi_endproc
.endfunc

