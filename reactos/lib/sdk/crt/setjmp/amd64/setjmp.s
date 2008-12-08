/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _setjmp/longjmp
 * FILE:              lib/sdk/crt/setjmp/amd64/setjmp.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asm.h>
#include <ndk/amd64/asmmacro.S>

.intel_syntax noprefix

#define JUMP_BUFFER_Frame 0x00
#define JUMP_BUFFER_Rbx   0x08
#define JUMP_BUFFER_Rsp   0x10
#define JUMP_BUFFER_Rbp   0x18
#define JUMP_BUFFER_Rsi   0x20
#define JUMP_BUFFER_Rdi   0x28
#define JUMP_BUFFER_R12   0x30
#define JUMP_BUFFER_R13   0x38
#define JUMP_BUFFER_R14   0x40
#define JUMP_BUFFER_R15   0x48
#define JUMP_BUFFER_Rip   0x50
#define JUMP_BUFFER_Spare 0x58
#define JUMP_BUFFER_Xmm6  0x60
#define JUMP_BUFFER_Xmm7  0x70
#define JUMP_BUFFER_Xmm8  0x80
#define JUMP_BUFFER_Xmm9  0x90
#define JUMP_BUFFER_Xmm10 0xa0
#define JUMP_BUFFER_Xmm11 0xb0
#define JUMP_BUFFER_Xmm12 0xc0
#define JUMP_BUFFER_Xmm13 0xd0
#define JUMP_BUFFER_Xmm14 0xe0
#define JUMP_BUFFER_Xmm15 0xf0


/* FUNCTIONS ******************************************************************/

/*
 * int _setjmp(jmp_buf env);
 *
 * Parameters: <rcx> - jmp_buf env
 * Returns:    0
 * Notes:      Sets up the jmp_buf
 */
.proc _setjmp
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
.endproc

/*
 * int _setjmpex(jmp_buf _Buf,void *_Ctx);
 *
 * Parameters: <rcx> - jmp_buf env
 *             <rdx> - frame
 * Returns:    0
 * Notes:      Sets up the jmp_buf
 */
.proc _setjmpex
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
.endproc


/*
 * void longjmp(jmp_buf env, int value);
 *
 * Parameters: <rcx> - jmp_buf setup by _setjmp
 *             <rdx> - int     value to return
 * Returns:    Doesn't return
 * Notes:      Non-local goto
 */
.proc longjmp

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
    jnz 2f
    inc rax
2:  jmp r8
.endproc
