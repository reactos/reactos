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

;
; VOID NTAPI
; RtlCaptureContext(
;     _In_ PCONTEXT ContextRecord); <rcx>
;
PUBLIC RtlCaptureContext
.PROC RtlCaptureContext

    ; Push rflags
    pushfq
    .ALLOCSTACK 8
    .ENDPROLOG

    ; Save rax first, we use it later to copy some data
    mov [rcx + CxRax], rax

    ; Set ContextFlags
    mov dword ptr [rcx + CxContextFlags], (CONTEXT_FULL or CONTEXT_SEGMENTS)

    ; Store the basic register context
    mov [rcx + CxRcx], rcx
    mov [rcx + CxRdx], rdx
    mov [rcx + CxRbx], rbx
    mov [rcx + CxRsi], rsi

    ; Load return address in rax
    mov rax, [rsp + 8]

    mov [rcx + CxRdi], rdi
    mov [rcx + CxRbp], rbp
    mov [rcx + CxR8], r8
    mov [rcx + CxR9], r9
    mov [rcx + CxR10], r10

    ; Store the return address
    mov [rcx + CxRip], rax

    mov [rcx + CxR11], r11
    mov [rcx + CxR12], r12
    mov [rcx + CxR13], r13
    mov [rcx + CxR14], r14
    mov [rcx + CxR15], r15

    ; Load former stack pointer in rax
    lea rax, [rsp + 16]

    ; Store segment selectors
    mov [rcx + CxSegCs], cs
    mov [rcx + CxSegDs], ds
    mov [rcx + CxSegEs], es
    mov [rcx + CxSegFs], fs
    mov [rcx + CxSegGs], gs
    mov [rcx + CxSegSs], ss

    ; Store stack pointer
    mov [rcx + CxRsp], rax

    ; Store xmm registers
    movaps [rcx + CxXmm0], xmm0
    movaps [rcx + CxXmm1], xmm1
    movaps [rcx + CxXmm2], xmm2
    movaps [rcx + CxXmm3], xmm3
    movaps [rcx + CxXmm4], xmm4
    movaps [rcx + CxXmm5], xmm5
    movaps [rcx + CxXmm6], xmm6
    movaps [rcx + CxXmm7], xmm7

    ; Load rflags into eax
    mov eax, [rsp]

    movaps [rcx + CxXmm8], xmm8
    movaps [rcx + CxXmm9], xmm9
    movaps [rcx + CxXmm10], xmm10
    movaps [rcx + CxXmm11], xmm11
    movaps [rcx + CxXmm12], xmm12
    movaps [rcx + CxXmm13], xmm13
    movaps [rcx + CxXmm14], xmm14
    movaps [rcx + CxXmm15], xmm15

    ; Store legacy floating point registers
    fxsave [rcx + CxFltSave]
    stmxcsr [rcx + CxMxCsr]

    ; Store rflags
    mov [rcx + CxEFlags], eax

    ; Cleanup stack and return
    add rsp, 8
    ret
.ENDP

EXTERN RtlpCheckForActiveDebugger:PROC
EXTERN RtlDispatchException:PROC
EXTERN ZwContinue:PROC
EXTERN ZwRaiseException:PROC
EXTERN RtlRaiseStatus:PROC

;
; VOID
; RtlRaiseException (
;     _In_ PEXCEPTION_RECORD ExceptionRecord);
;
PUBLIC RtlRaiseException
.PROC RtlRaiseException

    ; Allocate stack space for a CONTEXT record
    sub rsp, CONTEXT_FRAME_LENGTH + 8
    .allocstack CONTEXT_FRAME_LENGTH + 8

    ; Save the ExceptionRecord pointer
    mov [rsp + CxP1Home], rcx
    .savereg rcx, CxP1Home
    .endprolog

    ; Save the return address in EXCEPTION_RECORD.ExceptionAddress
    mov rdx, [rsp + CONTEXT_FRAME_LENGTH + 8]
    mov [rcx + ErExceptionAddress], rdx

    ; Capture the current context
    mov rcx, rsp
    call RtlCaptureContext

    ; Fix up CONTEXT.Rip for the caller (RtlCaptureContext doesn't change rdx!)
    mov [rsp + CxRip], rdx

    ; Fix up CONTEXT.Rsp for the caller (+8 for the return address)
    lea rdx, [rsp + CONTEXT_FRAME_LENGTH + 8 + 8]
    mov [rsp + CxRsp], rdx

    ; Check if a user mode debugger is active
    call RtlpCheckForActiveDebugger
    test al, al
    mov r8b, 1
    jnz RaiseException

    ; Dispatch the exception
    mov rcx, [rsp + CxP1Home]
    mov rdx, rsp
    call RtlDispatchException

    ; Check if we should continue
    test al, al
    mov r8b, 0
    jz RaiseException

    ; Continue, go back to previous context
    mov rcx, rsp
    mov dl, 0
    call  ZwContinue
    jmp RaiseStatus

RaiseException:

    mov rcx, [rsp + CxP1Home]
    mov rdx, rsp
    call ZwRaiseException

RaiseStatus:

    mov rcx, rax
    mov rdx, rsp
    call RtlRaiseStatus

.ENDP


END


