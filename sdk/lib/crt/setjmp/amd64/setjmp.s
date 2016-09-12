/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _setjmp/longjmp
 * FILE:              lib/sdk/crt/setjmp/amd64/setjmp.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
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

    /* Load rsp as it was before the call into rax */
    lea rax, [rsp + 8]
    /* Load return address into r8 */
    mov r8, [rsp]
    mov qword ptr [rcx + JUMP_BUFFER_Frame], 0
    mov [rcx + JUMP_BUFFER_Rbx], rbx
    mov [rcx + JUMP_BUFFER_Rbp], rbp
    mov [rcx + JUMP_BUFFER_Rsi], rsi
    mov [rcx + JUMP_BUFFER_Rdi], rdi
    mov [rcx + JUMP_BUFFER_R12], r12
    mov [rcx + JUMP_BUFFER_R13], r13
    mov [rcx + JUMP_BUFFER_R14], r14
    mov [rcx + JUMP_BUFFER_R15], r15
    mov [rcx + JUMP_BUFFER_Rsp], rax
    mov [rcx + JUMP_BUFFER_Rip], r8
    movdqa [rcx + JUMP_BUFFER_Xmm6], xmm6
    movdqa [rcx + JUMP_BUFFER_Xmm7], xmm7
    movdqa [rcx + JUMP_BUFFER_Xmm8], xmm8
    movdqa [rcx + JUMP_BUFFER_Xmm9], xmm9
    movdqa [rcx + JUMP_BUFFER_Xmm10], xmm10
    movdqa [rcx + JUMP_BUFFER_Xmm11], xmm11
    movdqa [rcx + JUMP_BUFFER_Xmm12], xmm12
    movdqa [rcx + JUMP_BUFFER_Xmm13], xmm13
    movdqa [rcx + JUMP_BUFFER_Xmm14], xmm14
    movdqa [rcx + JUMP_BUFFER_Xmm15], xmm15
    xor rax, rax
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

    /* Load rsp as it was before the call into rax */
    lea rax, [rsp + 8]
    /* Load return address into r8 */
    mov r8, [rsp]
    mov [rcx + JUMP_BUFFER_Frame], rdx
    mov [rcx + JUMP_BUFFER_Rbx], rbx
    mov [rcx + JUMP_BUFFER_Rbp], rbp
    mov [rcx + JUMP_BUFFER_Rsi], rsi
    mov [rcx + JUMP_BUFFER_Rdi], rdi
    mov [rcx + JUMP_BUFFER_R12], r12
    mov [rcx + JUMP_BUFFER_R13], r13
    mov [rcx + JUMP_BUFFER_R14], r14
    mov [rcx + JUMP_BUFFER_R15], r15
    mov [rcx + JUMP_BUFFER_Rsp], rax
    mov [rcx + JUMP_BUFFER_Rip], r8
    movdqa [rcx + JUMP_BUFFER_Xmm6], xmm6
    movdqa [rcx + JUMP_BUFFER_Xmm7], xmm7
    movdqa [rcx + JUMP_BUFFER_Xmm8], xmm8
    movdqa [rcx + JUMP_BUFFER_Xmm9], xmm9
    movdqa [rcx + JUMP_BUFFER_Xmm10], xmm10
    movdqa [rcx + JUMP_BUFFER_Xmm11], xmm11
    movdqa [rcx + JUMP_BUFFER_Xmm12], xmm12
    movdqa [rcx + JUMP_BUFFER_Xmm13], xmm13
    movdqa [rcx + JUMP_BUFFER_Xmm14], xmm14
    movdqa [rcx + JUMP_BUFFER_Xmm15], xmm15
    xor rax, rax
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

    // FIXME: handle frame

    mov rbx, [rcx + JUMP_BUFFER_Rbx]
    mov rbp, [rcx + JUMP_BUFFER_Rbp]
    mov rsi, [rcx + JUMP_BUFFER_Rsi]
    mov rdi, [rcx + JUMP_BUFFER_Rdi]
    mov r12, [rcx + JUMP_BUFFER_R12]
    mov r13, [rcx + JUMP_BUFFER_R13]
    mov r14, [rcx + JUMP_BUFFER_R14]
    mov r15, [rcx + JUMP_BUFFER_R15]
    mov rsp, [rcx + JUMP_BUFFER_Rsp]
    mov r8, [rcx + JUMP_BUFFER_Rip]
    movdqa xmm6, [rcx + JUMP_BUFFER_Xmm6]
    movdqa xmm7, [rcx + JUMP_BUFFER_Xmm7]
    movdqa xmm8, [rcx + JUMP_BUFFER_Xmm8]
    movdqa xmm9, [rcx + JUMP_BUFFER_Xmm9]
    movdqa xmm10, [rcx + JUMP_BUFFER_Xmm10]
    movdqa xmm11, [rcx + JUMP_BUFFER_Xmm11]
    movdqa xmm12, [rcx + JUMP_BUFFER_Xmm12]
    movdqa xmm13, [rcx + JUMP_BUFFER_Xmm13]
    movdqa xmm14, [rcx + JUMP_BUFFER_Xmm14]
    movdqa xmm15, [rcx + JUMP_BUFFER_Xmm15]

    /* return param2 or 1 if it was 0 */
    mov rax, rdx
    test rax, rax
    jnz l2
    inc rax
l2: jmp r8
ENDFUNC

END
