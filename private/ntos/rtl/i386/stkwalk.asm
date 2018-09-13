        TITLE   "Capture Stack Back Trace"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    stkwalk.asm
;
; Abstract:
;
;    This module implements the routine RtlCaptureStackBackTrace.  It will
;    walker the stack back trace and capture a portion of it.
;
; Author:
;
;    Steve Wood (stevewo) 29-Jan-1992
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

IFDEF NTOS_KERNEL_RUNTIME
        EXTRNP  _MmIsAddressValid,1
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
ENDIF

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

IFDEF NTOS_KERNEL_RUNTIME
        page ,132
        subttl  "RtlGetCallersAddress"
;++
;
; VOID
; RtlGetCallersAddress(
;    OUT PVOID *CallersAddress,
;    OUT PVOID *CallersCaller
;    )
;
; Routine Description:
;
;    This routine walks up the stack frames, capturing the first two return
;    addresses
;
;
; Arguments:
;
;    OUT PVOID CallersAddress - returns callers return address
;    OUT PVOID CallersCaller - returns callers caller return address
;
; Return Value:
;
;     None.
;
;
;--
RgcaCallersAddress      EQU [ebp+08h]
RgcaCallersCaller       EQU [ebp+0Ch]
cPublicProc _RtlGetCallersAddress ,2
        push    ebp
        mov     ebp, esp
        push    ebx                     ; Save EBX
        push    esi                     ; Save ESI
        push    edi                     ; Save EDI
        mov     eax, PCR[PcPrcbData+PbCurrentThread] ; (eax)->current thread
        push    ThInitialStack[eax]     ;  RcbtInitialStack = base of kernel stack
        push    esp                     ; Save current esp for handler
        push    offset RgcaFault        ; Address of handler
        push    PCR[PcExceptionList]    ; Save current list head
        mov     PCR[PcExceptionList],esp; Put us on list
        xor     esi,esi                 ; (ESI) will contain callers return address
        xor     edi,edi                 ; (EDI) will contain callers caller return address

        mov     edx,ebp                 ; (EDX) = current frame pointer
        mov     edx,[edx]               ; (EDX) = callers frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     short RgcaExit          ; ...then exit
        cmp     edx,RcbtInitialStack
        jae     short RgcaCheckForDpcStack
        cmp     edx,ThStackLimit[eax]
        jb      short RgcaCheckForDpcStack

Rgca20:
        mov     esi,[edx].4             ; Get callers return address

        mov     edx,[edx]               ; (EDX) = callers caller frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     short RgcaExit          ; ...then exit
        cmp     edx,RcbtInitialStack
        jae     short RgcaExit

        mov     edi,[edx].4             ; Get callers caller return address
RgcaExit:
        mov     ecx,RgcaCallersAddress
        jecxz   @F
        mov     [ecx],esi
@@:
        mov     ecx,RgcaCallersCaller
        jecxz   @F
        mov     [ecx],edi
@@:
        pop     PCR[PcExceptionList]    ; Restore Exception list
        pop     edi                     ; discard handler address
        pop     edi                     ; discard saved esp
        pop     edi                     ; discard RcbtInitialStack
        pop     edi                     ; Restore EDI
        pop     esi                     ; Restore ESI
        pop     ebx                     ; Restore EBX
        pop     ebp
        stdRET  _RtlGetCallersAddress

;
; We may be executing on the DPC stack for this processor which is ok.
;

RgcaCheckForDpcStack:

        ; Check if DPC active.

        cmp     dword ptr PCR[PcPrcbData+PbDpcRoutineActive], 0
        mov     eax, PCR[PcPrcbData+PbDpcStack]
        je      short RgcaExit          ; if no DPC active, must be bad stack.

        ; Check if address if below DPC stack upper bound
        ;
        ; Note: If we ARE on the DPC stack, we need to adjust this funtion's
        ; idea of the initial stack pointer so it will succeed the check at
        ; the next level.   We do not support transitioning across stacks in
        ; this function.

        cmp     edx, eax
        mov     RcbtInitialStack, eax
        jae     short RgcaExit          ; exit if above upper bound

        ; Check if below DPC stack lower bound.

        sub     eax, KERNEL_STACK_SIZE
        cmp     edx, eax
        ja      short Rgca20            ; jif on DPC stack
        jmp     short RgcaExit          ; not on DPC stack.


RgcaFault:
;
; Cheap and sleazy exception handler.  This will not unwind properly, which
; is ok because this function is a leaf except for calling KeGetCurrentIrql,
; which has no exception handler.
;
        mov     eax,[esp+0Ch]           ; (esp)->Context
        mov     edi,CsEdi[eax]          ; restore buffer pointer
        mov     esp,[esp+8]             ; (esp)->ExceptionList
        jmp     RgcaExit                ;
stdENDP _RtlGetCallersAddress
ENDIF

IFDEF NTOS_KERNEL_RUNTIME
RcbtInitialStack        EQU [ebp-10h]
ENDIF

IF 0
        page ,132
        subttl  "RtlCaptureStackBackTrace"
;++
;
; USHORT
; RtlCaptureStackBackTrace(
;    IN ULONG FramesToSkip,
;    IN ULONG FramesToCapture,
;    OUT PVOID *BackTrace,
;    OUT PULONG BackTraceHash
;    )
;
; Routine Description:
;
;    This routine walks up the stack frames, capturing the return address from
;    each frame requested.
;
;
; Arguments:
;
;    OUT PVOID BackTrace (eps+0x10) - Returns the caller of the caller.
;
;
; Return Value:
;
;     Number of return addresses returned in the BackTrace buffer.
;
;
;--
RcbtFramesToSkip        EQU [ebp+08h]
RcbtFramesToCapture     EQU [ebp+0Ch]
RcbtBackTrace           EQU [ebp+010h]
RcbtBackTraceHash       EQU [ebp+014h]

cPublicProc _RtlCaptureStackBackTrace ,4
IFDEF NTOS_KERNEL_RUNTIME
IF FPO
        xor     eax,eax
        stdRET  _RtlCaptureStackBackTrace
ENDIF
;
; This code is dangerous; it can fault if the stack does not have a well formed
; EBP chain.  This uses a simple exception handler which just does a shortcut
; unwind and returns 0.
;
RtlNeededBbtLabel:		;  BBT needed label
        push    ebp
        mov     ebp, esp
        push    ebx                     ; Save EBX
        push    esi                     ; Save ESI
        push    edi                     ; Save EDI
        mov     eax, PCR[PcPrcbData+PbCurrentThread] ; (eax)->current thread
        push    ThInitialStack[eax]     ;  RcbtInitialStack = base of kernel stack
        push    esp                     ; Save current esp for handler
        push    offset RcbtFault        ; Address of handler
        push    PCR[PcExceptionList]    ; Save current list head
        mov     PCR[PcExceptionList],esp; Put us on list
        mov     esi,RcbtBackTraceHash   ; (ESI) -> where to accumulate hash sum
        mov     edi,RcbtBackTrace       ; (EDI) -> where to put backtrace
        mov     edx,ebp                 ; (EDX) = current frame pointer
        mov     ecx,RcbtFramesToSkip    ; (ECX) = number of frames to skip
        jecxz   RcbtSkipLoopDone        ; Done if nothing to skip
RcbtSkipLoop:
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     RcbtCheckSkipU          ; ...then exit
        cmp     edx,RcbtInitialStack
        jae     RcbtCheckSkipU
        loop    RcbtSkipLoop
RcbtSkipLoopDone:
        mov     ecx,RcbtFramesToCapture ; (ECX) = number of frames to capture
        jecxz   RcbtExit                ; Bail out if nothing to capture
RcbtCaptureLoop:
        mov     eax,[edx].4             ; Get next return address
        stosd                           ; Store it in the callers buffer
        add     [esi],eax               ; Accumulate hash sum
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     RcbtCheckCaptureU       ; ...then exit
        cmp     edx,RcbtInitialStack
        jae     RcbtCheckCaptureU
        loop    RcbtCaptureLoop         ; Otherwise get next frame
RcbtExit:
        mov     eax,edi                 ; (EAX) -> next unused dword in buffer
        sub     eax,RcbtBackTrace       ; (EAX) = number of bytes stored
        shr     eax,2                   ; (EAX) = number of dwords stored
        pop     PCR[PcExceptionList]    ; Restore Exception list
        pop     edi                     ; discard handler address
        pop     edi                     ; discard saved esp
        pop     edi                     ; discard RcbtInitialStack
        pop     edi                     ; Restore EDI
        pop     esi                     ; Restore ESI
        pop     ebx                     ; Restore EBX
        pop     ebp
        stdRET    _RtlCaptureStackBackTrace

RcbtCheckSkipU:
        stdCall _KeGetCurrentIrql       ; (al) = CurrentIrql
        cmp     al, 0                   ; Bail out if not at IRQL 0
        ja      RcbtExit
        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; (ebx)->current thread
        cmp     byte ptr ThApcStateIndex[ebx],1 ; Bail out if attached.
        je      RcbtExit
        mov     ebx, ThTeb[ebx]         ; (EBX) -> User mode TEB
        or      ebx, ebx
        jnz     @F
        jmp short RcbtExit
RcbtSkipLoopU:
        mov     edx,[edx]               ; (EDX) = next frame pointer
@@:
        cmp     edx,PcStackLimit[ebx]   ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,PcInitialStack[ebx]
        jae     RcbtExit
        loop    RcbtSkipLoopU
        mov     ecx,RcbtFramesToCapture ; (ECX) = number of frames to capture
        jecxz   RcbtExit                ; Bail out if nothing to capture
RcbtCaptureLoopU:
        mov     eax,[edx].4             ; Get next return address
        stosd                           ; Store it in the callers buffer
        add     [esi],eax               ; Accumulate hash sum
        mov     edx,[edx]               ; (EDX) = next frame pointer
@@:
        cmp     edx,PcStackLimit[ebx]   ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,PcInitialStack[ebx]
        jae     RcbtExit
        loop    RcbtCaptureLoopU        ; Otherwise get next frame
        jmp     short RcbtExit

RcbtCheckCaptureU:
        stdCall _KeGetCurrentIrql       ; (al) = CurrentIrql
        cmp     al, 0                   ; Bail out if not at IRQL 0
        ja      RcbtExit
        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; (ebx)->current thread
        cmp     byte ptr ThApcStateIndex[ebx],1 ; Bail out if attached.
        je      RcbtExit
        mov     ebx, ThTeb[ebx]         ; (EBX) -> User mode TEB
        or      ebx, ebx
        jnz     @B
        jmp     RcbtExit                ; Bail out if TEB == NULL

RcbtFault:
;
; Cheap and sleazy exception handler.  This will not unwind properly, which
; is ok because this function is a leaf except for calling KeGetCurrentIrql,
; which has no exception handler.
;
        mov     eax,[esp+0Ch]           ; (esp)->Context
        mov     edi,CsEdi[eax]          ; restore buffer pointer
        mov     esp,[esp+8]             ; (esp)->ExceptionList
        jmp     RcbtExit                ;


ELSE
;
; This is defined always for user mode code, although it can be
; unreliable if called from code compiled with FPO enabled.
; In fact, it can fault, so we have a simple exception handler which
; will return 0 if this code takes an exception.
;
        push    ebp
        mov     ebp, esp
        push    esi                     ; Save ESI
        push    edi                     ; Save EDI
        push    esp                     ; Save current esp for handler
        push    offset RcbtFault        ; Address of handler
        push    fs:PcExceptionList      ; Save current list head
        mov     fs:PcExceptionList,esp  ; Put us on list
        mov     esi,RcbtBackTraceHash   ; (ESI) -> where to accumulate hash sum
        mov     edi,RcbtBackTrace       ; (EDI) -> where to put backtrace
        mov     edx,ebp                 ; (EDX) = current frame pointer
        mov     ecx,RcbtFramesToSkip    ; (ECX) = number of frames to skip
        jecxz   RcbtSkipLoopDone        ; Done if nothing to skip
RcbtSkipLoop:
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,fs:PcStackLimit     ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,fs:PcInitialStack
        jae     RcbtExit
        loop    RcbtSkipLoop
RcbtSkipLoopDone:
        mov     ecx,RcbtFramesToCapture ; (ECX) = number of frames to capture
        jecxz   RcbtExit                ; Bail out if nothing to capture
RcbtCaptureLoop:
        mov     eax,[edx].4             ; Get next return address
        stosd                           ; Store it in the callers buffer
        add     [esi],eax               ; Accumulate hash sum
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,fs:PcStackLimit     ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,fs:PcInitialStack
        jae     RcbtExit
        loop    RcbtCaptureLoop         ; Otherwise get next frame
RcbtExit:
        mov     eax,edi                 ; (EAX) -> next unused dword in buffer
        sub     eax,RcbtBackTrace       ; (EAX) = number of bytes stored
        shr     eax,2                   ; (EAX) = number of dwords stored
        pop     fs:PcExceptionList      ; Restore Exception list
        pop     edi                     ; discard handler address
        pop     edi                     ; discard saved esp
        pop     edi                     ; Restore EDI
        pop     esi                     ; Restore ESI
        pop     ebp
        stdRET    _RtlCaptureStackBackTrace

RcbtFault:
;
; Cheap and sleazy exception handler.  This function is a leaf, so it is
; safe to do this.
;
        mov     eax,[esp+0Ch]           ; (esp)->Context
        mov     edi,CsEdi[eax]          ; restore buffer pointer
        mov     esp,[esp+8]             ; (esp)->ExceptionList
        jmp     RcbtExit                ;

ENDIF

stdENDP _RtlCaptureStackBackTrace

ENDIF

_TEXT   ends
        end
