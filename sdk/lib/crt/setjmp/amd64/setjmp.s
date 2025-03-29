/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _setjmp/longjmp
 * FILE:              lib/sdk/crt/setjmp/amd64/setjmp.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 *                    Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

#define JUMP_BUFFER_Frame   0 /* 0x00 */
#define JUMP_BUFFER_Rbx     8 /* 0x08 */
#define JUMP_BUFFER_Rsp    16 /* 0x10 */
#define JUMP_BUFFER_Rbp    24 /* 0x18 */
#define JUMP_BUFFER_Rsi    32 /* 0x20 */
#define JUMP_BUFFER_Rdi    40 /* 0x28 */
#define JUMP_BUFFER_R12    48 /* 0x30 */
#define JUMP_BUFFER_R13    56 /* 0x38 */
#define JUMP_BUFFER_R14    64 /* 0x40 */
#define JUMP_BUFFER_R15    72 /* 0x48 */
#define JUMP_BUFFER_Rip    80 /* 0x50 */
#define JUMP_BUFFER_Spare  88 /* 0x58 */
#define JUMP_BUFFER_Xmm6   96 /* 0x60 */
#define JUMP_BUFFER_Xmm7  112 /* 0x70 */
#define JUMP_BUFFER_Xmm8  128 /* 0x80 */
#define JUMP_BUFFER_Xmm9  144 /* 0x90 */
#define JUMP_BUFFER_Xmm10 160 /* 0xa0 */
#define JUMP_BUFFER_Xmm11 176 /* 0xb0 */
#define JUMP_BUFFER_Xmm12 192 /* 0xc0 */
#define JUMP_BUFFER_Xmm13 208 /* 0xd0 */
#define JUMP_BUFFER_Xmm14 224 /* 0xe0 */
#define JUMP_BUFFER_Xmm15 240 /* 0xf0 */


/* FUNCTIONS ******************************************************************/
.code64

/*!
 * int _setjmp(jmp_buf env);
 *
 * \param   <rcx> - jmp_buf env
 * \return  0
 * \note    Sets up the jmp_buf
 */
PUBLIC _setjmp
FUNC _setjmp

    .endprolog

    push rbp                                    /* Save rbp */
    mov rbp, rsp                                /* rbp = rsp */
    and rsp, -16                                /* Align rsp to 16-byte boundary */
    mov [rcx + JUMP_BUFFER_Rbx], rbx            /* Store rbx */
    mov [rcx + JUMP_BUFFER_Rsp], rsp            /* Store rsp (aligned) */
    mov [rcx + JUMP_BUFFER_Rbp], rbp            /* Store rbp (aligned) */
    mov [rcx + JUMP_BUFFER_Rsi], rsi            /* Store rsi (non-volatile on windows) */
    mov [rcx + JUMP_BUFFER_Rdi], rdi            /* Store rdi (non-volatile on windows) */
    mov [rcx + JUMP_BUFFER_R12], r12            /* Store r12 */
    mov [rcx + JUMP_BUFFER_R13], r13            /* Store r13 */
    mov [rcx + JUMP_BUFFER_R14], r14            /* Store r14 */
    mov [rcx + JUMP_BUFFER_R15], r15            /* Store r15 */
    lea rax, [rip + LABEL1]                     /* Get the return address (LABEL1) */
    mov [rcx + JUMP_BUFFER_Rip], rax            /* Store rip (return address) */
    mov rax, [rsp + 8]
    mov [rcx + JUMP_BUFFER_Frame], rax          /* Store frame pointer */
    movdqu [rcx + JUMP_BUFFER_Xmm6], xmm6       /* Store xmm6 */
    movdqu [rcx + JUMP_BUFFER_Xmm7], xmm7       /* Store xmm7 */
    movdqu [rcx + JUMP_BUFFER_Xmm8], xmm8       /* Store xmm8 */
    movdqu [rcx + JUMP_BUFFER_Xmm9], xmm9       /* Store xmm9 */
    movdqu [rcx + JUMP_BUFFER_Xmm10], xmm10     /* Store xmm10 */
    movdqu [rcx + JUMP_BUFFER_Xmm11], xmm11     /* Store xmm11 */
    movdqu [rcx + JUMP_BUFFER_Xmm12], xmm12     /* Store xmm12 */
    movdqu [rcx + JUMP_BUFFER_Xmm13], xmm13     /* Store xmm13 */
    movdqu [rcx + JUMP_BUFFER_Xmm14], xmm14     /* Store xmm14 */
    movdqu [rcx + JUMP_BUFFER_Xmm15], xmm15     /* Store xmm15 */
    mov rsp, rbp                                /* Restore original rsp */
    pop rbp                                     /* Restore original rbp */
    xor eax, eax                                /* Return 0 */
LABEL1:
    ret
ENDFUNC

/*!
 * int _setjmpex(jmp_buf _Buf,void *_Ctx);
 *
 * \param   <rcx> - jmp_buf env
 * \param   <rdx> - frame
 * \return  0
 * \note    Sets up the jmp_buf
 */
PUBLIC _setjmpex
FUNC _setjmpex

    .endprolog

    push rbp                                    /* Save rbp */
    mov rbp, rsp                                /* rbp = rsp */
    and rsp, -16                                /* Align rsp to 16-byte boundary */
    mov [rcx + JUMP_BUFFER_Rbx], rbx            /* Store rbx */
    mov [rcx + JUMP_BUFFER_Rsp], rsp            /* Store rsp */
    mov [rcx + JUMP_BUFFER_Rbp], rbp            /* Store rbp */
    mov [rcx + JUMP_BUFFER_Rsi], rsi            /* Store rsi (non-volatile on windows) */
    mov [rcx + JUMP_BUFFER_Rdi], rdi            /* Store rdi (non-volatile on windows) */
    mov [rcx + JUMP_BUFFER_R12], r12            /* Store r12 */
    mov [rcx + JUMP_BUFFER_R13], r13            /* Store r13 */
    mov [rcx + JUMP_BUFFER_R14], r14            /* Store r14 */
    mov [rcx + JUMP_BUFFER_R15], r15            /* Store r15 */
    lea rax, [rip + LABEL2]                     /* Get the return address (LABEL2) */
    mov [rcx + JUMP_BUFFER_Rip], rax            /* Store rip (return address) */
    mov [rcx + JUMP_BUFFER_Frame], rdx          /* Store frame */
    movdqu [rcx + JUMP_BUFFER_Xmm6], xmm6       /* Store xmm6 */
    movdqu [rcx + JUMP_BUFFER_Xmm7], xmm7       /* Store xmm7 */
    movdqu [rcx + JUMP_BUFFER_Xmm8], xmm8       /* Store xmm8 */
    movdqu [rcx + JUMP_BUFFER_Xmm9], xmm9       /* Store xmm9 */
    movdqu [rcx + JUMP_BUFFER_Xmm10], xmm10     /* Store xmm10 */
    movdqu [rcx + JUMP_BUFFER_Xmm11], xmm11     /* Store xmm11 */
    movdqu [rcx + JUMP_BUFFER_Xmm12], xmm12     /* Store xmm12 */
    movdqu [rcx + JUMP_BUFFER_Xmm13], xmm13     /* Store xmm13 */
    movdqu [rcx + JUMP_BUFFER_Xmm14], xmm14     /* Store xmm14 */
    movdqu [rcx + JUMP_BUFFER_Xmm15], xmm15     /* Store xmm15 */
    mov rsp, rbp                                /* Restore original rsp */
    pop rbp                                     /* Restore original rbp */
    xor eax, eax                                /* Return 0 */
LABEL2:
    ret
ENDFUNC


/*!
 * void longjmp(jmp_buf env, int value);
 *
 * \param    <rcx> - jmp_buf setup by _setjmp
 * \param    <rdx> - int     value to return
 * \return   Doesn't return
 * \note     Non-local goto
 */
PUBLIC longjmp
FUNC longjmp

    .endprolog

    mov rbx, [rcx + JUMP_BUFFER_Rbx]            /* Restore rbx */
    mov rsp, [rcx + JUMP_BUFFER_Rsp]            /* Restore rsp */
    mov rbp, [rcx + JUMP_BUFFER_Rbp]            /* Restore rbp */
    mov rsi, [rcx + JUMP_BUFFER_Rsi]            /* Restore rsi */
    mov rdi, [rcx + JUMP_BUFFER_Rdi]            /* Restore rdi */
    mov r12, [rcx + JUMP_BUFFER_R12]            /* Restore r12 */
    mov r13, [rcx + JUMP_BUFFER_R13]            /* Restore r13 */
    mov r14, [rcx + JUMP_BUFFER_R14]            /* Restore r14 */
    mov r15, [rcx + JUMP_BUFFER_R15]            /* Restore r15 */
    mov rax, [rcx + JUMP_BUFFER_Frame]          /* Restore frame pointer */
    mov [rsp + 8], rax                          /* Restore frame pointer */
    movdqu xmm6, [rcx + JUMP_BUFFER_Xmm6]       /* Restore xmm6 */
    movdqu xmm7, [rcx + JUMP_BUFFER_Xmm7]       /* Restore xmm7 */
    movdqu xmm8, [rcx + JUMP_BUFFER_Xmm8]       /* Restore xmm8 */
    movdqu xmm9, [rcx + JUMP_BUFFER_Xmm9]       /* Restore xmm9 */
    movdqu xmm10, [rcx + JUMP_BUFFER_Xmm10]     /* Restore xmm10 */
    movdqu xmm11, [rcx + JUMP_BUFFER_Xmm11]     /* Restore xmm11 */
    movdqu xmm12, [rcx + JUMP_BUFFER_Xmm12]     /* Restore xmm12 */
    movdqu xmm13, [rcx + JUMP_BUFFER_Xmm13]     /* Restore xmm13 */
    movdqu xmm14, [rcx + JUMP_BUFFER_Xmm14]     /* Restore xmm14 */
    movdqu xmm15, [rcx + JUMP_BUFFER_Xmm15]     /* Restore xmm15 */
    mov rax, rdx                                /* Move val into rax (return value) */
    test rax, rax                               /* Check if val is 0 */
    jz LABEL3                                   /* If val is 0, jump to LABEL3 */
    jmp qword ptr [rcx + JUMP_BUFFER_Rip]       /* Jump to the stored return address (rip) */
LABEL3:
    mov rax, 1                                  /* If val was 0, return 1 */
    jmp qword ptr [rcx + JUMP_BUFFER_Rip]       /* Jump to the stored return address (rip) */
ENDFUNC

END
