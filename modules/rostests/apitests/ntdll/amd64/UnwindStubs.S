;
; PROJECT:         ReactOS api tests
; LICENSE:         GPL - See COPYING in the top level directory
; PURPOSE:         Test helper for RtlCaptureContext
; PROGRAMMER:      Timo Kreuzer
;

#include <asm.inc>
#include <ksamd64.inc>

EXTERN RtlCaptureContext:PROC
EXTERN __chkstk:PROC

.code64

; VOID
; LeafFunction (
;     PVOID* p,
;     PVOID x);
;
PUBLIC LeafFunction
FUNC LeafFunction
    .ENDPROLOG
    mov [rcx], rdx
    ret
ENDFUNC

;
; VOID
; UnwindCallWrapper (
;    _Out_ PCONTEXT UnwindContext,
;    _Out_ PCONTEXT ExpectedContext,
;    _In_ PHELPER_FUNCTION Function);
;
PUBLIC UnwindCallWrapper
FUNC UnwindCallWrapper

    mov [rsp + 8], rcx
    .SAVEREG rcx, 8
    mov [rsp + 16], rdx
    .SAVEREG rdx, 16
    sub rsp, 24
    .ALLOCSTACK 24
    .ENDPROLOG

    ; Call the stub function
    call r8

    ; Capture the post-context
    mov rcx, [rsp + 24 + 16]
    call RtlCaptureContext


    add rsp, 24
    ret

ENDFUNC

;
; VOID
; UnwindStub1 (
;    _Out_ PCONTEXT CapturedContext);
;
PUBLIC UnwindStub1
FUNC UnwindStub1

    sub rsp, HEX(18)    ; 0x00
    .ALLOCSTACK HEX(18)
    .ENDPROLOG

    call RtlCaptureContext ; 0x04
    xor rax, rax        ; 0x09

    add rsp, HEX(18)    ; 0x0B
    ret                 ; 0x0F

ENDFUNC

PUBLIC UnwindStub2
FUNC UnwindStub2

    mov [rsp + 8], rcx  ; 0x00
    .SAVEREG rcx, 8
    push r15            ; 0x05
    .PUSHREG r15
    push r14            ; 0x07
    .PUSHREG r14
    push r13            ; 0x09
    .PUSHREG r13
    mov rax, 128        ; 0x0B
    call __chkstk       ; 0x12
    sub rsp, rax        ; 0x17
    .ALLOCSTACK 128
    lea r13, [rsp + 128]; 0x1A
    .SETFRAME r13, 128
    .ENDPROLOG

    call RtlCaptureContext ; 0x22
    xor rax, rax        ; 0x27
    mov r13, 111h        ; 0x27
    mov r14, 112h        ; 0x27
    mov r15, 113h        ; 0x27

    add rsp, 128        ; 0x2A
    pop r13             ; 0x31
    pop r14             ; 0x33
    pop r15             ; 0x35
    ret                 ; 0x37

ENDFUNC

PUBLIC UnwindStub3
FUNC UnwindStub3

    ; Prolog
    mov [rsp + 8], rcx      ; 0x00
    .SAVEREG rcx, 8
    push rbp                ; 0x05
    .PUSHREG r15
    sub rsp, 48             ; 0x06
    .ALLOCSTACK 48
    lea rbp, [rsp + 32]     ; 0x0A
    .SETFRAME rbp, 32
    .ENDPROLOG

    ; alloca
    sub rsp, 16            ; 0x1B

    ; Store alloca buffer in stack variable
    lea rax, [rsp + 32]     ; 0x1E
    mov [rbp], rax          ; 0x23

    call RtlCaptureContext  ; 0x27

    mov rax, [rbp]          ; 0x2C
    mov byte ptr [rax], 1   ; 0x30

    ; Epilog (double stack cleanup)
    lea rsp, [rbp - 32]     ; 0x33
    add rsp, 48             ; 0x37
    pop rbp                 ; 0x3B
    ret                     ; 0x3C

ENDFUNC

;
; VOID
; UnwindStub4 (
;    _Out_ PCONTEXT CapturedContext);
;
PUBLIC UnwindStub4
FUNC UnwindStub4

    sub rsp, HEX(18)    ; 0x00
    .ALLOCSTACK HEX(18)
    .ENDPROLOG

    ; Capture the context
    call RtlCaptureContext ; 0x04

   ; mov 

    ; restore the context
    xor rdx, rdx
   ; call RtlRestoreContext

ENDFUNC

DB 0,0,0,0,0,0,0,0

END
