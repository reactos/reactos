        page    ,132
        subttl  emerror.asm - Emulator error handler
;***
;emerror.asm - Emulator error handler
;
;        Microsoft Confidential
;
;        Copyright (c) Microsoft Corporation 1987, 1991
;
;        All Rights Reserved
;
;Purpose:
;       Emulator error handler
;
;Revision History:  (also see emulator.hst)
;
;   10/30/89  WAJ   Added this header.
;   11/15/89  WAJ   Major changes for Dos32RaiseExcpetion().
;   12/01/89  WAJ   Now set cbExceptionInfo correctly.
;   02/08/90  WAJ   Fixed GP fault in 32 bit exception handler.
;   09/03/91  JWM   Modified entry/exit sequence for DOSX32.
;   02/15/92  JWM   Adapted for NT.
;
;*******************************************************************************

ifdef   _DOS32EXT
include except32.inc
endif

;***    error_return - return to user code (regardless of error)
;
;       This macro returns to user code.  It goes to some lengths
;       to restore the flags on the instruction immediately before
;       the return so that any pending trace trap will be
;       acknowledged immediately after the retfd (and before the
;       next user instruction) instead of after the instruction
;       following the return as would be the case if we returned
;       using iretd.
;
;       ENTRY   ((SS:ESP)) = user's EAX
;               ((SS:ESP)+4) = return EIP
;               ((SS:ESP)+8) = return CS
;               ((SS:ESP)+12) = user's EFLAGS
;       EXIT    return to user program, above arguments
;               popped off stack, user's EAX and EFLAGS
;               restored.

error_return    macro   noerror
ifdef   _DOS32EXT
        sti                                     ; JWM, 9/3/91
        push    dword ptr [esp+8]               ; JWM, 9/6/91
        popfd                                   ; JWM, 9/6/91
endif                                           ; DOS32EXT

ifdef NT386
if DBG
        push    dword ptr [esp+8]               ; On checked build, allow
        popfd                                   ; single step to work
endif
endif
        iretd
        endm


TESTif  macro   nam
        mov     bl,err&nam      ; default error number
   if (nam ge 100h)
        test    ah,nam/256
   else ;not (nam ge 100h)
        test    al,nam
   endif ;(nam ge 100h)
        JSNZ    signalerror
        endm

EM_ENTRY eCommonExceptions
CommonExceptions:
        mov     ebx,[esp].[OldLongStatus]
        and		ebx,LongSavedFlags		;preserve condition codes, error flags
        or		EMSEG:[LongStatusWord],ebx					;merge saved status word, condition codes
        pop     eax
        pop     ecx
        pop     edx
        pop     ebx
        add     esp,4                   ; toss esp value
        pop     ebp
        pop     esi
        pop     edi
        add     esp,8                   ;toss old PrevCodeOff and StatusWord
        pop     ds
        call    Emexcept
        error_return    noerror

ifdef _DOS32EXT

EmExcept PROC C, OldEIP:DWORD, OldCS:DWORD, OldFlags:DWORD

LOCAL   SSAR:DWORD
LOCAL   ec:_DX32_CONTEXT

    ;*
    ;*  Set up SS access rights.
    ;*

        push    ds
        mov     [ec.R_Eax], eax
        GetEmData   ds,ax

        mov     eax, ss
        lar     eax, eax
        mov     [SSAR], eax

    ;*
    ;*  Fill in ExceptionContext structure.
    ;*


        mov     [ec.NPXContextFlags], NPX_CONTEXT_FULL
        mov     [ec.R_Edi], edi
        mov     [ec.R_Esi], esi

        mov     eax, [ebp]
        mov     [ec.R_Ebp], eax

        lea     eax, [OldFlags+4]
        mov     [ec.R_Esp], eax

        mov     [ec.R_Ebx], ebx
        mov     [ec.R_Edx], edx
        mov     [ec.R_Ecx], ecx

        mov     eax, EMSEG:[PrevCodeOff]

        mov     [ec.R_Eip], eax
        mov     eax, [OldFlags]
        mov     [ec.EFlags], eax

        mov     eax, [OldCS]
        movzx   eax,ax
        mov     [ec.SegCs], eax
        mov     ax,ss
        movzx   eax,ax
        mov     [ec.SegSs], eax

        pop     eax
        movzx   eax,ax
        mov     [ec.SegDs], eax         ; ds was pushed on entry.

        mov     ax,es
        movzx   eax,ax
        mov     [ec.SegEs], eax

        mov     ax,fs
        movzx   eax,ax
        mov     [ec.SegFs], eax

        mov     ax,gs
        movzx   eax,ax
        mov     [ec.SegGs], eax

        lea     esi, [ec]
        add     esi, 4

        push    ebp
        call    SaveState
        pop     ebp

        lea     eax, [ec]
        push    ds
        push    es

        mov     bx, seg FLAT:CURstk
        mov     ds, ebx
        mov     es, ebx
        push    eax

        call    DOS32RAISEEXCEPTION

        add     esp, 4

        pop     es
        pop     ds

RaiseExceptRet:
        or      eax, eax
        JZ      ExceptNotHandled

    ;*
    ;* Copy new flags, cs, eip to new stack.
    ;*

        mov     ds, [ec.SegSs]
        mov     esi, [ec.R_Esp]     ; ds:esi == new ss:esp

        mov     eax, [ec.Eflags]            ; set up iretd frame
        mov     [esi-4], eax

        mov     eax, [ec.SegCs]
        mov     [esi-8], eax

        mov     eax, [ec.R_Eip]
        mov     [esi-12], eax

    ;*
    ;*  Put new stack pointer on stack.
    ;*

        push    ds
        sub     esi, 12
        push    esi

    ;*
    ;*  Reset other registers.
    ;*

        mov     edi, [ec.R_Edi]
        mov     esi, [ec.R_Esi]
        mov     ebx, [ec.R_Ebx]
        mov     edx, [ec.R_Edx]
        mov     ecx, [ec.R_Ecx]
        mov     eax, [ec.R_Eax]
        mov     ds, [ec.SegDs]
        mov     es, [ec.SegEs]
        mov     fs, [ec.SegFs]
        mov     gs, [ec.SegGs]

        mov     ebp, [ec.R_Ebp]    ; must do this last.

        lss     esp, fword ptr [esp] ; reset ss:esp

        sti                             ; JWM, 9/3/91
        push    [esp+8]                 ; JWM, 9/6/91
        popfd                           ; JWM, 9/6/91

        iretd                       ; reset flags, cs, eip

ExceptNotHandled:
EmExcept        ENDP

endif                   ; ifdef _DOS32EXT

ifdef NT386

ISIZE                   equ     4
ISizeEC                 equ     (ContextFrameLength + ISIZE - 1) and (not (ISIZE - 1))
ISizeExceptStruct       equ     (ExceptionRecordLength + ISIZE - 1) and (not (ISIZE - 1))

ec_off          EQU     4+ISizeEc
estruct_off     EQU     ec_off+ISizeExceptStruct

SSAR            EQU     <[ebp][-4]>
ec              EQU     <[ebp][-ec_off]>
eStruct         EQU     <[ebp][-estruct_off]>

OldEIP          EQU     <ebp+8>
OldCS           EQU     <ebp+12>
OldFlags        EQU     <ebp+16>


EmExcept PROC   NEAR

        push    ebp
        mov     ebp,esp
        sub     esp,estruct_off


    ;*
    ;*  Set up SS access rights.
    ;*

        push    ds
        mov     [ec.ctx_RegEax], eax
        GetEmData   ds,ax

        mov     eax, ss
        lar     eax, eax
        mov     [SSAR], eax

    ;*
    ;*  Fill in ExceptionContext structure.
    ;*


        mov     dword ptr [ec.ContextFlags], NPX_CONTEXT_FULL
        mov     dword ptr [ec.ctx_Cr0NpxState], CR0_EM
        mov     [ec.ctx_RegEdi], edi
        mov     [ec.ctx_RegEsi], esi

        mov     eax, [ebp]
        mov     [ec.ctx_RegEbp], eax

        lea     eax, [OldFlags+4]
        mov     [ec.ctx_RegEsp], eax

        mov     [ec.ctx_RegEbx], ebx
        mov     [ec.ctx_RegEdx], edx
        mov     [ec.ctx_RegEcx], ecx

        mov     eax, [OldEIP]

        mov     [ec.ctx_RegEip], eax
        mov     eax, [OldFlags]
        mov     [ec.ctx_EFlags], eax

        mov     eax, [OldCS]
        movzx   eax,ax
        mov     [ec.ctx_SegCs], eax
        mov     ax,ss
        movzx   eax,ax
        mov     [ec.ctx_SegSs], eax

        pop     eax
        movzx   eax,ax
        mov     [ec.ctx_SegDs], eax             ; ds was pushed on entry.

        mov     ax,es
        movzx   eax,ax
        mov     [ec.ctx_SegEs], eax

        mov     ax,fs
        movzx   eax,ax
        mov     [ec.ctx_SegFs], eax

        mov     ax,gs
        movzx   eax,ax
        mov     [ec.ctx_SegGs], eax

        lea     esi, [ec]
        add     esi, ctx_env

        or      EMSEG:[StatusWord], 8000H		; set 'busy' bit
        or      EMSEG:[SWerr], Summary                  ; set Summary bit
        or      EMSEG:[CURerr], Summary

        mov     cl, EMSEG:[ErrMask]
        push    ecx
        push    ebp
        call    SaveState
        pop     ebp
        pop     ecx

        call    GetEMSEGStatusWord                      ; EAX = status word
        test    al, cl                          ; test status word against mask
        jne     short Err00

ifdef TRACENPX
        mov     edx, 0C1020304h                 ; Raise bogus exception code, to trace with
        jmp     short Err50
endif
        mov     al, Invalid

;
; According to the floating error priority, we test what is the cause of
; the NPX error and raise an appropriate exception.
;

Err00:
        test    al, Invalid                     ; Invalid Op?
        jz      short Err10                     ; No, go check next

        mov     edx, XCPT_FLOAT_INVALID_OPERATION
        test    al, StackFlag                   ; Stack fault?
        jz      short Err50                     ; No, go raise invalid op
        mov     edx, XCPT_FLOAT_STACK_CHECK
        jmp     short Err50                     ; Go raise stack fault

Err10:  mov     edx, XCPT_FLOAT_DIVIDE_BY_ZERO
        test    al, ZeroDivide
        jnz     short Err50
        mov     edx, XCPT_FLOAT_DENORMAL_OPERAND
        test    al, Denormal
        jnz     short Err50
        mov     edx, XCPT_FLOAT_OVERFLOW
        test    al, Overflow
        jnz     short Err50
        mov     edx, XCPT_FLOAT_UNDERFLOW
        test    al, Underflow
        jnz     short Err50
        mov     edx, XCPT_FLOAT_INEXACT_RESULT

Err50:  mov     [eStruct.ExceptionNum], edx

        xor     eax,eax
        mov     [eStruct.fHandlerFlags], eax
        mov     [eStruct.NestedExceptionReportRecord], eax
        mov     dword ptr [eStruct.CParameters], 1      ; GeorgioP convention
        mov     [eStruct.ErExceptionInformation], eax   ; GeorgioP convention

        mov     eax, EMSEG:[PrevCodeOff]
        mov     [eStruct.ExceptionAddress], eax

        lea     edx, [eStruct]

        lea     eax, [ec]
        push    ds
        push    es


;TRUE, this is a first-chance exception

        stdCall _NtRaiseException,<edx, eax, 1>
        stdCall _RtlRaiseStatus, <eax>

        pop     es
        pop     ds

RaiseExceptRet:
        or      eax, eax
        JZ      ExceptNotHandled

    ;*
    ;* Copy new flags, cs, eip to new stack.
    ;*

        mov     ds, [ec.ctx_SegSs]
        mov     esi, [ec.ctx_RegEsp]        ; ds:esi == new ss:esp

        mov     eax, [ec.ctx_Eflags]        ; set up iretd frame
        mov     [esi-4], eax

        mov     eax, [ec.ctx_SegCs]
        mov     [esi-8], eax

        mov     eax, [ec.ctx_RegEip]
        mov     [esi-12], eax

    ;*
    ;*  Put new stack pointer on stack.
    ;*

        push    ds
        sub     esi, 12
        push    esi

    ;*
    ;*  Reset other registers.
    ;*

        mov     edi, [ec.ctx_RegEdi]
        mov     esi, [ec.ctx_RegEsi]
        mov     ebx, [ec.ctx_RegEbx]
        mov     edx, [ec.ctx_RegEdx]
        mov     ecx, [ec.ctx_RegEcx]
        mov     eax, [ec.ctx_RegEax]
        mov     ds, [ec.ctx_SegDs]
        mov     es, [ec.ctx_SegEs]
        mov     fs, [ec.ctx_SegFs]
        mov     gs, [ec.ctx_SegGs]

        mov     ebp, [ec.ctx_RegEbp]    ; must do this last.

        lss     esp, fword ptr [esp] ; reset ss:esp

        sti                             ; JWM, 9/3/91
        push    [esp+8]                 ; JWM, 9/6/91
        popfd                           ; JWM, 9/6/91

        iretd                       ; reset flags, cs, eip

ExceptNotHandled:
EmExcept        ENDP

endif                   ; ifdef NT386

        int 3	                ; Added For BBT, a return here is needed or BBT
        ret	                ; has a flow problem.
ifdef  DEBUG

lab PageFault
        mov     al, byte ptr cs:[iax]
        ret
endif
