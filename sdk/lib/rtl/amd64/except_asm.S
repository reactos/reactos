/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library (RTL)
 * FILE:            lib/rtl/amd64/except_asm.S
 * PURPOSE:         Exception support for AMD64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* FUNCTIONS *****************************************************************/

.code64

/*
 * VOID
 * RtlCaptureContext(
 *     _Out_ PCONTEXT ContextRecord@<rcx>);
 */
PUBLIC RtlCaptureContext
.PROC RtlCaptureContext

    /* Push rflags */
    pushfq
    .ALLOCSTACK 8
    .ENDPROLOG

    /* Save rax first, we use it later to copy some data */
    mov [rcx + CxRax], rax

    /* Set ContextFlags */
    mov dword ptr [rcx + CxContextFlags], (CONTEXT_FULL or CONTEXT_SEGMENTS)

    /* Store the basic register context */
    mov [rcx + CxRcx], rcx
    mov [rcx + CxRdx], rdx
    mov [rcx + CxRbx], rbx
    mov [rcx + CxRsi], rsi

    /* Load return address in rax */
    mov rax, [rsp + 8]

    mov [rcx + CxRdi], rdi
    mov [rcx + CxRbp], rbp
    mov [rcx + CxR8], r8
    mov [rcx + CxR9], r9
    mov [rcx + CxR10], r10

    /* Store the return address */
    mov [rcx + CxRip], rax

    mov [rcx + CxR11], r11
    mov [rcx + CxR12], r12
    mov [rcx + CxR13], r13
    mov [rcx + CxR14], r14
    mov [rcx + CxR15], r15

    /* Load former stack pointer in rax */
    lea rax, [rsp + 16]

    /* Store segment selectors */
    mov [rcx + CxSegCs], cs
    mov [rcx + CxSegDs], ds
    mov [rcx + CxSegEs], es
    mov [rcx + CxSegFs], fs
    mov [rcx + CxSegGs], gs
    mov [rcx + CxSegSs], ss

    /* Store stack pointer */
    mov [rcx + CxRsp], rax

    /* Store xmm registers */
    movaps [rcx + CxXmm0], xmm0
    movaps [rcx + CxXmm1], xmm1
    movaps [rcx + CxXmm2], xmm2
    movaps [rcx + CxXmm3], xmm3
    movaps [rcx + CxXmm4], xmm4
    movaps [rcx + CxXmm5], xmm5
    movaps [rcx + CxXmm6], xmm6
    movaps [rcx + CxXmm7], xmm7

    /* Load rflags into eax */
    mov eax, [rsp]

    movaps [rcx + CxXmm8], xmm8
    movaps [rcx + CxXmm9], xmm9
    movaps [rcx + CxXmm10], xmm10
    movaps [rcx + CxXmm11], xmm11
    movaps [rcx + CxXmm12], xmm12
    movaps [rcx + CxXmm13], xmm13
    movaps [rcx + CxXmm14], xmm14
    movaps [rcx + CxXmm15], xmm15

    /* Store legacy floating point registers */
    fxsave [rcx + CxFltSave]
    stmxcsr [rcx + CxMxCsr]

    /* Store rflags */
    mov [rcx + CxEFlags], eax

    /* Cleanup stack and return */
    add rsp, 8
    ret
.ENDP

/*
 * VOID
 * RtlpRestoreContextInternal(
 *     _In_ PCONTEXT ContextRecord@<rcx>);
 */
PUBLIC RtlpRestoreContextInternal
.PROC RtlpRestoreContextInternal

    /* Allocate space */
    sub rsp, HEX(8)
    .ALLOCSTACK 8
    .ENDPROLOG

    /* Restore legacy floating point registers (It is slow, so do it first) */
    ldmxcsr [rcx + CxMxCsr]
    fxrstor [rcx + CxFltSave]

    /* Load the target stack pointer into rdx */
    mov rdx, [rcx + CxRsp]

    /* Load EFlags into rax (zero extended) */
    mov eax, [rcx + CxEFlags]

    /* Load the return address into r8 */
    mov r8, [rcx + CxRip]

    /* Load rdx restore value into r9 */
    mov r9, [rcx + CxRdx]

    /* Restore xmm registers */
    movaps xmm0, [rcx + CxXmm0]
    movaps xmm1, [rcx + CxXmm1]
    movaps xmm2, [rcx + CxXmm2]
    movaps xmm3, [rcx + CxXmm3]
    movaps xmm4, [rcx + CxXmm4]
    movaps xmm5, [rcx + CxXmm5]
    movaps xmm6, [rcx + CxXmm6]
    movaps xmm7, [rcx + CxXmm7]
    movaps xmm8, [rcx + CxXmm8]
    movaps xmm9, [rcx + CxXmm9]
    movaps xmm10, [rcx + CxXmm10]
    movaps xmm11, [rcx + CxXmm11]
    movaps xmm12, [rcx + CxXmm12]
    movaps xmm13, [rcx + CxXmm13]
    movaps xmm14, [rcx + CxXmm14]
    movaps xmm15, [rcx + CxXmm15]

    /* Copy Return address (now in r8) to the target stack */
    mov [rdx - 2 * 8], r8

    /* Copy Eflags (now in rax) to the target stack */
    mov [rdx - 3 * 8], rax

    /* Copy the value for rdx (now in r9) to the target stack */
    mov [rdx - 4 * 8], r9

    /* Restore the integer registers */
    mov rbx, [rcx + CxRbx]
    mov rsi, [rcx + CxRsi]
    mov rdi, [rcx + CxRdi]
    mov rbp, [rcx + CxRbp]
    mov r10, [rcx + CxR10]
    mov r11, [rcx + CxR11]
    mov r12, [rcx + CxR12]
    mov r13, [rcx + CxR13]
    mov r14, [rcx + CxR14]
    mov r15, [rcx + CxR15]

    /* Restore segment selectors */
    mov ds, [rcx + CxSegDs] // FIXME: WOW64 context?
    mov es, [rcx + CxSegEs]
    mov fs, [rcx + CxSegFs]
    //mov gs, [rcx + CxSegGs]
    //mov ss, [rcx + CxSegSs]

    /* Restore the registers we used */
    mov r8,  [rcx + CxR8]
    mov r9,  [rcx + CxR9]

    /* Check if we go to a different cs */
    mov ax, cs
    cmp [rcx + CxSegCs], ax
    jne ReturnFar

    /* Restore rax and rcx */
    mov rax, [rcx + CxRax]
    mov rcx, [rcx + CxRcx]

    /* Switch to the target stack */
    lea rsp, [rdx - 4 * 8]

    /* Pop rdx from the stack */
    pop rdx

    /* Pop Eflags from the stack */
    popfq

    /* Return and fix up the stack */
    ret 8

ReturnFar:

    /* Put cs on the stack for the far return */
    mov ax, [rcx + CxSegCs]
    mov [rdx - 1 * 8], ax

    /* Load ss */
    mov ss, [rcx + CxSegSs]

    /* Restore rax and rcx */
    mov rax, [rcx + CxRax]
    mov rcx, [rcx + CxRcx]

    /* Switch to the target stack */
    lea rsp, [rdx - 4 * 8]

    /* Pop rdx from the stack */
    pop rdx

    /* Pop Eflags from the stack */
    popfq

    /* Return far */
    retf

.ENDP

END


