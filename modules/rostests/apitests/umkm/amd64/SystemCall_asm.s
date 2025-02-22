/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     x64 ASM helper functions for syscall tests
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

.data
g_Rcx:
    .quad 0
g_SegSs:
    .short 0

.code64

EXTERN RtlCaptureContext:PROC
EXTERN g_NoopSyscallNumber:DWORD

#define STACK_ARGUMENT_SPACE 16*8

LoadContext:
    push qword ptr [rcx + CxEFlags]
    popfq
    movdqu xmm0, [rcx + CxXmm0]
    movdqu xmm1, [rcx + CxXmm1]
    movdqu xmm2, [rcx + CxXmm2]
    movdqu xmm3, [rcx + CxXmm3]
    movdqu xmm4, [rcx + CxXmm4]
    movdqu xmm5, [rcx + CxXmm5]
    movdqu xmm6, [rcx + CxXmm6]
    movdqu xmm7, [rcx + CxXmm7]
    movdqu xmm8, [rcx + CxXmm8]
    movdqu xmm9, [rcx + CxXmm9]
    movdqu xmm10, [rcx + CxXmm10]
    movdqu xmm11, [rcx + CxXmm11]
    movdqu xmm12, [rcx + CxXmm12]
    movdqu xmm13, [rcx + CxXmm13]
    movdqu xmm14, [rcx + CxXmm14]
    movdqu xmm15, [rcx + CxXmm15]
    mov rax, [rcx + CxRax]
    mov rbx, [rcx + CxRbx]
    mov rdx, [rcx + CxRdx]
    mov rsi, [rcx + CxRsi]
    mov rdi, [rcx + CxRdi]
    mov rbp, [rcx + CxRbp]
    mov r8, [rcx + CxR8]
    mov r9, [rcx + CxR9]
    mov r10, [rcx + CxR10]
    mov r11, [rcx + CxR11]
    mov r12, [rcx + CxR12]
    mov r13, [rcx + CxR13]
    mov r14, [rcx + CxR14]
    mov r15, [rcx + CxR15]
    //mov rcx, [rcx + CxRcx]

    mov ax, [rcx + CxSegDs]
    mov ds, ax
    mov ax, [rcx + CxSegEs]
    mov es, ax
    mov ax, [rcx + CxSegFs]
    mov fs, ax
    mov ax, [rcx + CxSegGs]
    //mov gs, ax // FIXME: ReactOS does not like this

    ret

PUBLIC SyscallReturn

.PROC DoSyscallAndCaptureContext2
    /* Allocate enough space for the system call handler */
    sub rsp, STACK_ARGUMENT_SPACE + 5*8
    .ALLOCSTACK STACK_ARGUMENT_SPACE + 5*8
    .ENDPROLOG

    /* Save rcx and r8 in the home space */
    mov [rsp + STACK_ARGUMENT_SPACE + 6*8], rcx
    mov [rsp + STACK_ARGUMENT_SPACE + 8*8], r8

    /* Load the pre-context */
    mov rcx, rdx
    call LoadContext
    call RtlCaptureContext

    mov ax, word ptr [rcx + CxSegSs]
    mov ss, ax

    /* Do the syscall */
    mov rax, [rsp + STACK_ARGUMENT_SPACE + 6*8]
    syscall

GLOBAL_LABEL SyscallReturn

    /* Save returned ss */
    mov qword ptr g_Rcx[rip], rcx
    mov cx, ss
    mov word ptr g_SegSs[rip], cx

    mov cx, HEX(2B)
    mov ss, cx

    /* Save the post-context */
    mov rcx, [rsp + STACK_ARGUMENT_SPACE + 8*8]
    call RtlCaptureContext

    mov rcx, qword ptr g_Rcx[rip]
    mov rax, [rsp + STACK_ARGUMENT_SPACE + 8*8]
    mov [rax + CxRcx], rcx

    mov ax, HEX(2B)
    mov ds, ax
    mov es, ax
    //mov gs, ax // FIXME: ReactOS does not like this
    mov ss, ax
    mov ax, HEX(53)
    mov fs, ax

    cld

    mov rcx, [rsp + STACK_ARGUMENT_SPACE + 8*8]
    mov ax, word ptr g_SegSs[rip]
    mov [rcx + CxSegSs], ax

    add rsp, STACK_ARGUMENT_SPACE + 5*8
    ret
.ENDP

/*
 * VOID
 * DoSyscallAndCaptureContext(
 *     _In_ ULONG64 SyscallNumber,
 *     _Out_ PCONTEXT PreContext,
 *     _Out_ PCONTEXT PostContext);
 */
PUBLIC DoSyscallAndCaptureContext
.PROC DoSyscallAndCaptureContext
    GENERATE_EXCEPTION_FRAME

    call DoSyscallAndCaptureContext2

    RESTORE_EXCEPTION_STATE
    ret

.ENDP

/*
 * ULONG64
 * DoSyscallWithUnalignedStack(
 *     _In_ ULONG64 SyscallNumber);
 */
PUBLIC DoSyscallWithUnalignedStack
.PROC DoSyscallWithUnalignedStack
    /* Allocate enough space for the system call handler */
    sub rsp, STACK_ARGUMENT_SPACE + 6*8
    .ALLOCSTACK STACK_ARGUMENT_SPACE + 6*8
    .ENDPROLOG

    mov rax, rcx
    syscall

    add rsp, STACK_ARGUMENT_SPACE + 6*8
    ret
.ENDP

END
