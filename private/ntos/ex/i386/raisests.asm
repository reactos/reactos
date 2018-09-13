        title  "Raise Exception"
;++
;
; Copyright (c) 1990  Microsoft Corporation
;
; Module Name:
;
;    raisests.asm
;
; Abstract:
;
;    This module implements the function to raise a software exception.
;
; Author:
;
;    Bryan Willman  11 Nov 90
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
include callconv.inc                    ; calling convention macros
        .list

        EXTRNP  _RtlDispatchException,2
        EXTRNP  _ZwContinue,2
        EXTRNP  _ZwRaiseException,3

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
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
; ExRaiseException (
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

cPublicProc _ExRaiseException  , 1

        push    ebp
        mov     ebp,esp
        pushfd                          ; save flags before sub
        sub     esp,ContextFrameLength  ; Allocate a context record

;
; Save regs we use in context record
;

        mov     [(ebp-ContextFrameLength-4)+CsEax],eax
        mov     [(ebp-ContextFrameLength-4)+CsEcx],ecx

;
; Get pointer to exception report record, and set the exceptionaddress
; field to be our return address
;

        mov     eax,[ebp+8]             ; (eax) -> ExceptionReportRecord

        mov     ecx,[ebp+4]
        mov     [eax.ErExceptionAddress],ecx

;
; Copy machine context into the context record
;

        lea     eax,[ebp-ContextFrameLength-4]  ; (eax) -> Context record

        mov     [eax.CsEip],ecx

        mov     [eax.CsEbx],ebx
        mov     [eax.CsEdx],edx

        mov     [eax.CsEsi],esi
        mov     [eax.CsEdi],edi

        lea     ecx,[ebp+8]
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
; Set Context flags, note that FLOATING_POINT is NOT set.
;

        mov     dword ptr [eax.CsContextFlags],CONTEXT_SETTING

;
; _RtlDispatchException(ExceptionRecord, ContextRecord)
;
        stdCall    _RtlDispatchException, <[ebp+8],eax>
;
; If the exception is successfully dispatched, then continue execution.
; Otherwise, give the kernel debugger a chance to handle the exception.
;
        lea     ecx,[ebp-ContextFrameLength-4]  ; (eax) -> Context record

        or      eax, eax
        jz      short ere10

        stdCall    _ZwContinue, <ecx,0>
        jmp     short ere20

ere10:
        stdCall    _ZwRaiseException, <[ebp+8],ecx,0>

ere20:
;
; Either the attempt to continue execution or the attempt to give
; the kernel debugger a chance to handle the exception failed. Raise
; a noncontinuable exception.
;
        stdCall    _ExRaiseStatus, <eax>


stdENDP _ExRaiseException

        page
        subttl  "Raise Software Exception"
;++
;
; VOID
; ExRaiseStatus (
;     IN NTSTATUS Status
;     )
;
; Routine Description:
;
;    This function raises a software exception with the specified status value
;    by building a context record, establishing the stack limits of the current
;    processor mode, and calling the exception dispatcher. If the exception
;    dispatcher finds a handler to process the exception, then control is
;    returned to the caller using the NtContinue system service. Otherwise
;    the NtLastChance system service is called to provide default handing.
;
;   N.B.    On the 386, floating point state is not defined for non-fp
;           exceptions.  Therefore, this routine does not attempt to
;           capture it.
;
;           This means this routine cannot be used to report fp exceptions.
;
; Arguments:
;
;     Status - Supplies the status value to be used as the exception code
;         for the exception that is to be raised.
;
; Return Value:
;
;     None.

; Arguments:
;
;--

cPublicProc     _ExRaiseStatus,1

        push    ebp
        mov     ebp,esp
        pushfd                          ; save flags before sub
        sub     esp,ContextFrameLength+ExceptionRecordLength

;
; Save regs we use in context record
;

        mov     [(ebp-ContextFrameLength-4)+CsEax],eax
        mov     [(ebp-ContextFrameLength-4)+CsEcx],ecx

;
; Copy machine context into the context record
;


        lea     eax,[ebp-ContextFrameLength-4]  ; (eax) -> Context record

        mov     ecx,[ebp+4]                     ; [ecx] = returned address
        mov     [eax.CsEip],ecx

        mov     [eax.CsEbx],ebx
        mov     [eax.CsEdx],edx

        mov     [eax.CsEsi],esi
        mov     [eax.CsEdi],edi

        lea     ecx,[ebp+8]
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
; Set Context flags, note that FLOATING_POINT is NOT set.
;

        mov     dword ptr [eax.CsContextFlags],CONTEXT_SETTING

;
; Get pointer to exception report record, and set the exceptionaddress
; field to be our return address
;

        lea     eax,[ebp-ContextFrameLength-ExceptionRecordLength-4]
                                        ; (eax) -> ExceptionRecord
        mov     ecx,[ebp+4]
        mov     dword ptr [eax.ErExceptionAddress],ecx
        mov     ecx,[ebp+8]
        mov     dword ptr [eax.ErExceptionCode],ecx
        mov     dword ptr [eax.ErNumberParameters], 0
        mov     dword ptr [eax.ErExceptionRecord], 0
        mov     dword ptr [eax.ErExceptionFlags], EXCEPTION_NONCONTINUABLE

;
; _RtlDispatchException(ExceptionRecord, ContextRecord)
;

        lea     ecx,[ebp-ContextFrameLength-4]  ; (eax) -> Context record

; ecx - Context record
; eax - Exception record
        stdCall _RtlDispatchException, <eax, ecx>

;
; An unwind was not initiated during the dispatching of a noncontinuable
; exception. Give the kernel debugger a chance to handle the exception.
;

;
; _ZwRaiseException(ExceptionRecord, ContextRecord, FirstChance=TRUE)
;

        lea     ecx,[ebp-ContextFrameLength-4]  ; (eax) -> Context record
        lea     eax,[ebp-ContextFrameLength-ExceptionRecordLength-4]
; 1 - TRUE
; ecx - Context Record
; eax - Exception Report Record
        stdCall   _ZwRaiseException, <eax, ecx, 1>

;
; We came back, suggesting some sort of error in the call.  Raise
; a status exception to report this, return from ZwRaiseException is type.
;

        stdCall    _ExRaiseStatus, <eax>


stdENDP _ExRaiseStatus

_TEXT$01   ends
        end

