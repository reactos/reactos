        title  "Raise Exception"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    raise.asm
;
; Abstract:
;
;    This module implements the function to raise a software exception.
;
; Author:
;
;    Bryan Willman  11 april 90
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

        EXTRNP  _ZwRaiseException,3

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; Context flags definition.
;

CONTEXT_SETTING EQU CONTEXT_INTEGER OR CONTEXT_CONTROL OR CONTEXT_SEGMENTS


;
; Exception record length definition.
;

EXCEPTION_RECORD_LENGTH EQU (ErExceptionInformation + 16) AND 0fffffff0H

        page
        subttl  "Raise Software Exception"
;++
;
; VOID
; RtlRaiseException (
;    IN PEXCEPTION_RECORD ExceptionRecord
;    )
;
; Routine Description:
;
;    This function raises a software exception by building a context record,
;    establishing the stack limits of the current processor mode, and calling
;    the exception dispatcher. If the exception dispatcher finds a handler
;    to process the exception, then control is returned to the caller using
;    the NtContinue system service. Otherwise the NtLastChance system service
;    is called to provide default handing.
;
;   N.B.    On the 386, floating point state is not defined for non-fp
;           exceptions.  Therefore, this routine does not attempt to
;           capture it.
;
;           This means this routine cannot be used to report fp exceptions.
;
; Arguments:
;
;    ExceptionRecord (ebp+8) - Supplies a pointer to an exception record.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _RtlRaiseException  ,1

        push    ebp
        mov     ebp,esp
        pushfd                          ; save flags before sub
        sub     esp,ContextFrameLength  ; Allocate a context record

;
;   Save regs we use in context record
;

        mov     [(ebp-ContextFrameLength-4)+CsEax],eax
        mov     [(ebp-ContextFrameLength-4)+CsEcx],ecx

;
;   Get pointer to exception report record, and set the exceptionaddress
;   field to be our return address
;

        mov     eax,[ebp+8]             ; (eax) -> ExceptionReportRecord

        mov     ecx,[ebp+4]
        mov     [eax.ErExceptionAddress],ecx

;
;   Copy machine context into the context record
;


        lea     eax,[ebp-ContextFrameLength-4]  ; (eax) -> Context record

        mov     [eax.CsEip],ecx

        mov     [eax.CsEbx],ebx
        mov     [eax.CsEdx],edx

        mov     [eax.CsEsi],esi
        mov     [eax.CsEdi],edi

;
; context record's ESP must have the argument popped off the stack
;

        lea     ecx,[ebp+12]

        mov     [eax.CsEsp],ecx

        mov     ecx,[ebp]
        mov     [eax.CsEbp],ecx

        mov     ecx,[ebp-4]
        mov     [eax.CsEflags],ecx

        mov     dword ptr [eax.CsSegCs],cs
        mov     dword ptr [eax.CsSegDs],ds
        mov     dword ptr [eax.CsSegEs],es
        mov     dword ptr [eax.CsSegFs],fs
        mov     dword ptr [eax.CsSegGs],gs
        mov     dword ptr [eax.CsSegSs],ss

;
;   Set Context flags, note that FLOATING_POINT is NOT set.
;

        mov     dword ptr [eax.CsContextFlags],CONTEXT_SETTING

;
;   _ZwRaiseException(ExceptionRecord, ContextRecord, FirstChance=TRUE)
;

; 1 - TRUE
; eax - Context Record
; [ebp+8] - Exception Report Record

        stdCall   _ZwRaiseException,<[ebp+8],eax,1>

;
;   We came back, suggesting some sort of error in the call.  Raise
;   a status exception to report this, return from ZwRaiseException is type.
;

        sub     esp,EXCEPTION_RECORD_LENGTH ; allocate record on stack, esp is base
        mov     [esp.ErExceptionCode],eax   ; set exception type
        mov     dword ptr [esp.ErExceptionFlags],EXCEPTION_NONCONTINUABLE
        mov     dword ptr [esp.ErNumberParameters],0  ; no parms
        mov     eax,[ebp+8]
        mov     [esp.ErExceptionRecord],eax ; point back to first exception
        mov     eax,esp
        stdCall   _RtlRaiseException,<eax>

;
;   We will never come here, because RtlRaiseException will not allow
;   return if exception is non-continuable.
;

stdENDP _RtlRaiseException

_TEXT   ends
        end
