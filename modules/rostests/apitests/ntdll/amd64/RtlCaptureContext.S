;
; PROJECT:         ReactOS api tests
; LICENSE:         GPL - See COPYING in the top level directory
; PURPOSE:         Test helper for RtlCaptureContext
; PROGRAMMER:      Timo Kreuzer
;

#include <asm.inc>
#include <ksamd64.inc>

.code64

EXTERN RtlCaptureContext:PROC
EXTERN memset:PROC

FUNC ZeroContext

    pushfq
    .ALLOCSTACK 8
    push rax
    .PUSHREG rax
    push rcx
    .PUSHREG rcx
    push rdi
    .PUSHREG rdi
    .ENDPROLOG

    mov rdi, rcx
    mov rcx, CONTEXT_FRAME_LENGTH
    xor eax, eax
    cld
    rep stosb

    pop rdi
    pop rcx
    pop rax
    popfq
    ret

ENDFUNC

;
; VOID
; RtlCaptureContextWrapper (
;    _Inout_ PCONTEXT InOutContext,
;    _Out_ PCONTEXT CapturedContext);
;
PUBLIC RtlCaptureContextWrapper
FUNC RtlCaptureContextWrapper

    ; Generate a KEXCEPTION_FRAME on the stack
    GENERATE_EXCEPTION_FRAME

    ; Save parameters
    mov [rsp + ExP1Home], rcx
    mov [rsp + ExP2Home], rdx

    ; Load integer registers from InOutContext
    mov rax, [rcx + CxRax]
    mov rdx, [rcx + CxRdx]
    mov rbx, [rcx + CxRbx]
    mov rbp, [rcx + CxRbp]
    mov rsi, [rcx + CxRsi]
    mov rdi, [rcx + CxRdi]
    mov r8, [rcx + CxR8]
    mov r9, [rcx + CxR9]
    mov r10, [rcx + CxR10]
    mov r11, [rcx + CxR11]
    mov r12, [rcx + CxR12]
    mov r13, [rcx + CxR13]
    mov r14, [rcx + CxR14]
    mov r15, [rcx + CxR15]

    ; Load floating point registers from InOutContext
    fxrstor [rcx + CxFltSave]

    ; Load MxCsr (this overwrites the value from FltSave)
    ldmxcsr [rcx + CxMxCsr]

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

    ; Load EFlags
    push [rcx + CxEFlags]
    popfq

    ; Capture the context
    mov rcx, [rsp + ExP2Home]
    call RtlCaptureContext
ReturnAddress:

    ; Save the returned rcx
    mov [rsp + ExP3Home], rcx

    ; Save Eflags
    pushfq
    pop [rsp + ExP4Home]

    ; Restore original rcx
    mov rcx, [rsp + ExP1Home]

    ; Zero out the context (this doesn't clobber abny registers)
    call ZeroContext

    ; Save returned integer registers to InOutContext
    mov [rcx + CxRax], rax
    mov [rcx + CxRdx], rdx
    mov [rcx + CxRbx], rbx
    mov [rcx + CxRbp], rbp
    mov [rcx + CxRsi], rsi
    mov [rcx + CxRdi], rdi
    mov [rcx + CxR8], r8
    mov [rcx + CxR9], r9
    mov [rcx + CxR10], r10
    mov [rcx + CxR11], r11
    mov [rcx + CxR12], r12
    mov [rcx + CxR13], r13
    mov [rcx + CxR14], r14
    mov [rcx + CxR15], r15

    ; Save the returned rcx in the context
    mov rax, [rsp + ExP3Home]
    mov [rcx + CxRcx], rax

    ; Save the returned EFLags in the context    
    mov rax, [rsp + ExP4Home]
    mov [rcx + CxEFlags], rax

    ; Save returned floating point registers to InOutContext
    stmxcsr [rcx + CxMxCsr]
    fxsave [rcx + CxFltSave]
    movaps [rcx + CxXmm0], xmm0
    movaps [rcx + CxXmm1], xmm1
    movaps [rcx + CxXmm2], xmm2
    movaps [rcx + CxXmm3], xmm3
    movaps [rcx + CxXmm4], xmm4
    movaps [rcx + CxXmm5], xmm5
    movaps [rcx + CxXmm6], xmm6
    movaps [rcx + CxXmm7], xmm7
    movaps [rcx + CxXmm8], xmm8
    movaps [rcx + CxXmm9], xmm9
    movaps [rcx + CxXmm10], xmm10
    movaps [rcx + CxXmm11], xmm11
    movaps [rcx + CxXmm12], xmm12
    movaps [rcx + CxXmm13], xmm13
    movaps [rcx + CxXmm14], xmm14
    movaps [rcx + CxXmm15], xmm15

    ; Save the expected return address
    mov rax, offset ReturnAddress
    mov [rcx + CxRip], rax

    ; Save the expected stored rsp
    mov [rcx + CxRsp], rsp

    ; Restore the registers from the KEXCEPTION_FRAME
    RESTORE_EXCEPTION_STATE

    ret

ENDFUNC



END
