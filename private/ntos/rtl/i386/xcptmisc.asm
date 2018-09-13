        title   "Miscellaneous Exception Handling"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    xcptmisc.asm
;
; Abstract:
;
;    This module implements miscellaneous routines that are required to
;    support exception handling. Functions are provided to call an exception
;    handler for an exception, call an exception handler for unwinding, get
;    the caller's stack pointer, get the caller's frame pointer, get the
;    caller's floating status, get the caller's processor state, get the
;    caller's extended processor status, and get the current stack limits.
;
; Author:
;
;    David N. Cutler (davec) 14-Aug-1989
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;   6 April 90  bryanwi
;
;           386 version created
;
;--
.386p

        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

;
; Unwind flags.
;

Unwind  equ EXCEPTION_UNWINDING OR EXCEPTION_EXIT_UNWIND

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page
        subttl  "Execute Handler for Exception"
;++
;
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForException (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext,
;    IN PEXCEPTION_ROUTINE ExceptionRoutine
;    )
;
; Routine Description:
;
;    This function allocates a call frame, stores the handler address and
;    establisher frame pointer in the frame, establishes an exception
;    handler, and then calls the specified exception handler as an exception
;    handler. If a nested exception occurs, then the exception handler of
;    of this function is called and the handler address and establisher
;    frame pointer are returned to the exception dispatcher via the dispatcher
;    context parameter. If control is returned to this routine, then the
;    frame is deallocated and the disposition status is returned to the
;    exception dispatcher.
;
; Arguments:
;
;    ExceptionRecord (ebp+8) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (ebp+12) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;    ContextRecord (ebp+16) - Supplies a pointer to a context record.
;
;    DispatcherContext (ebp+20) - Supplies a pointer to the dispatcher context
;       record.
;
;    ExceptionRoutine (ebp+24) - supplies a pointer to the exception handler
;       that is to be called.
;
; Return Value:
;
;    The disposition value returned by the specified exception handler is
;    returned as the function value.
;
;--

cPublicProc _RtlpExecuteHandlerForException,5

        mov     edx,offset FLAT:ExceptionHandler    ; Set who to register
        jmp     ExecuteHandler                      ; jump to common code

stdENDP _RtlpExecuteHandlerForException


        page
        subttl  "Execute Handler for Unwind"
;++
;
; EXCEPTION_DISPOSITION
; RtlpExecuteHandlerForUnwind (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext,
;    IN PEXCEPTION_ROUTINE ExceptionRoutine
;    )
;
; Routine Description:
;
;    This function allocates a call frame, stores the handler address and
;    establisher frame pointer in the frame, establishes an exception
;    handler, and then calls the specified exception handler as an unwind
;    handler. If a collided unwind occurs, then the exception handler of
;    of this function is called and the handler address and establisher
;    frame pointer are returned to the unwind dispatcher via the dispatcher
;    context parameter. If control is returned to this routine, then the
;    frame is deallocated and the disposition status is returned to the
;    unwind dispatcher.
;
; Arguments:
;
;    ExceptionRecord (ebp+8) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (ebp+12) - Supplies the frame pointer of the establisher
;       of the exception handler that is to be called.
;
;    ContextRecord (ebp+16) - Supplies a pointer to a context record.
;
;    DispatcherContext (ebp+20) - Supplies a pointer to the dispatcher context
;       record.
;
;    ExceptionRoutine (ebp+24) - supplies a pointer to the exception handler
;       that is to be called.
;
; Return Value:
;
;    The disposition value returned by the specified exception handler is
;    returned as the function value.
;
;--

cPublicProc _RtlpExecuteHandlerForUnwind    ,5

        mov     edx,offset FLAT:UnwindHandler

;;  N.B. - FALL into ExecuteHandler

stdENDP _RtlpExecuteHandlerForUnwind



;
;   ExecuteHandler is the common tail for RtlpExecuteHandlerForException
;   and RtlpExecuteHandlerForUnwind.
;
;   (edx) = handler (Exception or Unwind) address
;


ExceptionRecord     equ [ebp+8]
EstablisherFrame    equ [ebp+12]
ContextRecord       equ [ebp+16]
DispatcherContext   equ [ebp+20]
ExceptionRoutine    equ [ebp+24]


cPublicProc   ExecuteHandler,5

        push    ebp
        mov     ebp,esp

        push    EstablisherFrame        ; Save context of exception handler
                                        ; that we're about to call.

    .errnz   ErrHandler-4
        push    edx                     ; Set Handler address

    .errnz   ErrNext-0
        push    fs:PcExceptionList                      ; Set next pointer


        mov     fs:PcExceptionList,esp                  ; Link us on

; Call the specified exception handler.

        push    DispatcherContext
        push    ContextRecord
        push    EstablisherFrame
        push    ExceptionRecord

        mov     ecx,ExceptionRoutine
        call    ecx
        mov     esp,fs:PcExceptionList

; Don't clean stack here, code in front of ret will blow it off anyway

; Disposition is in eax, so all we do is deregister handler and return

    .errnz  ErrNext-0
        pop     fs:PcExceptionList

        mov     esp,ebp
        pop     ebp
        stdRET  ExecuteHandler

stdENDP ExecuteHandler

        page
        subttl  "Local Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; ExceptionHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext
;    )
;
; Routine Description:
;
;    This function is called when a nested exception occurs. Its function
;    is to retrieve the establisher frame pointer and handler address from
;    its establisher's call frame, store this information in the dispatcher
;    context record, and return a disposition value of nested exception.
;
; Arguments:
;
;    ExceptionRecord (exp+4) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (esp+8) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;    ContextRecord (esp+12) - Supplies a pointer to a context record.
;
;    DispatcherContext (esp+16) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;    A disposition value ExceptionNestedException is returned if an unwind
;    is not in progress. Otherwise a value of ExceptionContinueSearch is
;    returned.
;
;--

stdPROC   ExceptionHandler,4

        mov     ecx,dword ptr [esp+4]           ; (ecx) -> ExceptionRecord
        test    dword ptr [ecx.ErExceptionFlags],Unwind
        mov     eax,ExceptionContinueSearch     ; Assume unwind
        jnz     eh10                            ; unwind, go return

;
; Unwind is not in progress - return nested exception disposition.
;

        mov     ecx,[esp+8]             ; (ecx) -> EstablisherFrame
        mov     edx,[esp+16]            ; (edx) -> DispatcherContext
        mov     eax,[ecx+8]             ; (eax) -> EstablisherFrame for the
                                        ;          handler active when we
                                        ;          nested.
        mov     [edx],eax               ; Set DispatcherContext field.
        mov     eax,ExceptionNestedException

eh10:   stdRET    ExceptionHandler

stdENDP ExceptionHandler

        page
        subttl  "Local Unwind Handler"
;++
;
; EXCEPTION_DISPOSITION
; UnwindHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PVOID DispatcherContext
;    )
;
; Routine Description:
;
;    This function is called when a collided unwind occurs. Its function
;    is to retrieve the establisher frame pointer and handler address from
;    its establisher's call frame, store this information in the dispatcher
;    context record, and return a disposition value of nested unwind.
;
; Arguments:
;
;    ExceptionRecord (esp+4) - Supplies a pointer to an exception record.
;
;    EstablisherFrame (esp+8) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;    ContextRecord (esp+12) - Supplies a pointer to a context record.
;
;    DispatcherContext (esp+16) - Supplies a pointer to the dispatcher context
;       record.
;
; Return Value:
;
;    A disposition value ExceptionCollidedUnwind is returned if an unwind is
;    in progress. Otherwise a value of ExceptionContinueSearch is returned.
;
;--

stdPROC   UnwindHandler,4

        mov     ecx,dword ptr [esp+4]           ; (ecx) -> ExceptionRecord
        test    dword ptr [ecx.ErExceptionFlags],Unwind
        mov     eax,ExceptionContinueSearch     ; Assume NOT unwind
        jz      uh10                            ; not unwind, go return


;
; Unwind is in progress - return collided unwind disposition.
;

        mov     ecx,[esp+8]             ; (ecx) -> EstablisherFrame
        mov     edx,[esp+16]            ; (edx) -> DispatcherContext
        mov     eax,[ecx+8]             ; (eax) -> EstablisherFrame for the
                                        ;          handler active when we
                                        ;          nested.
        mov     [edx],eax               ; Set DispatcherContext field.
        mov     eax,ExceptionCollidedUnwind

uh10:   stdRET    UnwindHandler

stdENDP UnwindHandler

        page
        subttl  "Unlink Exception Registration Record & Handler"
;++
;
; VOID
; RtlpUnlinkHandler(PEXCEPTION_REGISTRATION_RECORD UnlinkPointer)
;
; Routine Description:
;
;   This function removes the specified exception registration record
;   (and thus the relevent handler) from the exception traversal
;   chain.
;
; Arguments:
;
;    UnlinkPointer (esp+4) - Address of registration record to unlink.
;
; Return Value:
;
;    The caller's return address.
;
;--

cPublicProc _RtlpUnlinkHandler ,1

        mov     ecx,dword ptr [esp+4]
        mov     ecx,[ecx.ErrNext]
        mov     fs:PcExceptionList,ecx
        stdRET    _RtlpUnlinkHandler

stdENDP _RtlpUnlinkHandler

        page
        subttl  "Capture Context"
;++
;
; VOID
; RtlCaptureContext (PCONTEXT ContextRecord)
; RtlpCaptureContext (PCONTEXT ContextRecord)
;
; Routine Description:
;
;   This fucntion fills in the specified context record with the
;   current state of the machine, except that the values of EBP
;   and ESP are computed to be those of the caller's caller.
;
;   N.B.  This function assumes it is called from a 'C' procedure with
;         the old ebp at [ebp], the return address at [ebp+4], and
;         old esp = ebp + 8.
;
;         Certain 'C' optimizations may cause this to not be true.
;
;   N.B.  This function does NOT adjust ESP to pop the arguments off
;         the caller's stack.  In other words, it provides a __cdecl ESP,
;         NOT a __stdcall ESP.  This is mainly because we can't figure
;         out how many arguments the caller takes.
;
;   N.B.  Floating point state is NOT captured.
;
;   RtlpCaptureContext does not capture volitales.
;   RtlCaptureContext captures volitales.
;
; Arguments:
;
;    ContextRecord  (esp+4) - Address of context record to fill in.
;
; Return Value:
;
;    The caller's return address.
;
;--

cPublicProc _RtlCaptureContext ,1

        push    ebx
        mov     ebx,[esp+8]         ; (ebx) -> ContextRecord

        mov     dword ptr [ebx.CsEax],eax
        mov     dword ptr [ebx.CsEcx],ecx
        mov     dword ptr [ebx.CsEdx],edx
        mov     eax, [esp]
        mov     dword ptr [ebx.CsEbx],eax

        mov     dword ptr [ebx.CsEsi],esi
        mov     dword ptr [ebx.CsEdi],edi
        jmp     RtlpCaptureCommon
stdENDP _RtlCaptureContext

cPublicProc _RtlpCaptureContext ,1

        push    ebx
        mov     ebx,[esp+8]         ; (ebx) -> ContextRecord

        mov     dword ptr [ebx.CsEax],0
        mov     dword ptr [ebx.CsEcx],0
        mov     dword ptr [ebx.CsEdx],0
        mov     dword ptr [ebx.CsEbx],0

        mov     dword ptr [ebx.CsEsi],0
        mov     dword ptr [ebx.CsEdi],0

RtlpCaptureCommon:
        mov     [ebx.CsSegCs],cs
        mov     [ebx.CsSegDs],ds
        mov     [ebx.CsSegEs],es
        mov     [ebx.CsSegFs],fs
        mov     [ebx.CsSegGs],gs
        mov     [ebx.CsSegSs],ss

        pushfd
        pop     [ebx.CsEflags]

        mov     eax,[ebp+4]
        mov     [ebx.CsEip],eax

        mov     eax,[ebp]
        mov     [ebx.CsEbp],eax

        lea     eax,[ebp+8]
        mov     [ebx.CsEsp],eax

        pop     ebx
        stdRET    _RtlpCaptureContext

stdENDP _RtlpCaptureContext

        page
        subttl  "Capture Context (private)"
;++
;
; VOID
; RtlCaptureContext (PCONTEXT ContextRecord)
;
; Routine Description:
;
;   This function is similiar too RtlpCaptureContext expect that
;   volitales are captured as well.
;
;   This fucntion fills in the specified context record with the
;   current state of the machine, except that the values of EBP
;   and ESP are computed to be those of the caller's caller.
;
;   N.B.  This function does NOT adjust ESP to pop the arguments off
;         the caller's stack.  In other words, it provides a __cdecl ESP,
;         NOT a __stdcall ESP.  This is mainly because we can't figure
;         out how many arguments the caller takes.
;
;   N.B.  Floating point state is NOT captured.
;
; Arguments:
;
;    ContextRecord  (esp+4) - Address of context record to fill in.
;
; Return Value:
;
;    The caller's return address.
;
;--


ifndef WX86_i386
        page
        subttl  "Get Stack Limits"
;++
;
; VOID
; RtlpGetStackLimits (
;    OUT PULONG LowLimit,
;    OUT PULONG HighLimit
;    )
;
; Routine Description:
;
;    This function returns the current stack limits based on the current
;    processor mode.
;
;    On the 386 we always store the stack limits in the PCR, and address
;    both PCR and TEB the same way, so this code is mode independent.
;
; Arguments:
;
;    LowLimit (esp+4) - Supplies a pointer to a variable that is to receive
;       the low limit of the stack.
;
;    HighLimit (esp+8) - Supplies a pointer to a variable that is to receive
;       the high limit of the stack.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _RtlpGetStackLimits ,2
;cPublicFpo 2,0

        mov     eax,fs:PcStackLimit
        mov     ecx,[esp+4]
        mov     [ecx],eax               ; Save low limit

        mov     eax,fs:PcInitialStack
        mov     ecx,[esp+8]
        mov     [ecx],eax               ; Save high limit

        stdRET    _RtlpGetStackLimits

stdENDP _RtlpGetStackLimits

endif
        page
        subttl  "Get Exception Registration List Head"
;++
;
;   PVOID
;   RtlpGetRegistrationHead()
;
;   Routine Description:
;
;       This function returns the address of the first Exception
;       registration record for the current context.
;
;   Arguments:
;
;       None.
;
;   Return Value:
;
;       The address of the first registration record.
;
;--

cPublicProc _RtlpGetRegistrationHead    ,0
;cPublicFpo 0,0

        mov     eax,fs:PcExceptionList
        stdRET    _RtlpGetRegistrationHead

stdENDP _RtlpGetRegistrationHead

        page
        subttl  "Get Return Address"
;++
;
;   PVOID
;   RtlpGetReturnAddress()
;
;   Routine Description:
;
;       This function returns the caller's return address.  It assumes
;       that a standard 'C' entry sequence has been used, be warned
;       that certain optimizations might invalidate that assumption.
;
;   Arguments:
;
;       None.
;
;   Return Value:
;
;       The address of the first registration record.
;
;--

cPublicProc _RtlpGetReturnAddress ,0
;cPublicFpo 0,0

        mov     eax,[ebp+4]
        stdRET    _RtlpGetReturnAddress

stdENDP _RtlpGetReturnAddress
_TEXT$01   ends
        end
