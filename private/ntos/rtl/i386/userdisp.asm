        title  "User Mode Dispatcher Code"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    userdisp.asm
;
; Abstract:
;
;    The module contains procedures to do user mode dispatching
;    ("trampolining") of user apcs and user exceptions.
;
; Author:
;
;    Bryan M Willman (bryanwi) 31-Aug-90
;
; Environment:
;
;    User mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

ifndef WX86_i386
        EXTRNP  _ZwCallbackReturn,3
endif

        EXTRNP  _ZwContinue,2
        EXTRNP  _RtlDispatchException,2
        EXTRNP  _RtlRaiseStatus,1
        EXTRNP  _ZwRaiseException,3
        EXTRNP  _RtlRaiseException,1
;
; Exception record size definition.
;

ExceptionRecordSize = (ErNumberParameters + 4 + 3) AND 0fffffffcH ;

        page ,132
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

ifndef WX86_i386
        page
        subttl  "User APC Dispatcher"
;++
;
; VOID
; KiUserApcDispatcher (
;    IN PKNORMAL_ROUTINE NormalRoutine,
;    IN PVOID NormalContext,
;    IN PVOID SystemArgument1,
;    IN PVOID SystemArgument2,
;    IN CONTEXT ContinueContext
;    )
;
; Routine Description:
;
;    This routine is entered on return from kernel mode to deliver an APC
;    in user mode. The context frame for this routine was built when the
;    APC interrupt was processed and contains the entire machine state of
;    the current thread. The specified APC routine is called and then the
;    machine state is restored and execution is continued.
;
; Arguments:
;
;    NormalRoutine - Supplies that address of the function that is to be called.
;
;    NormalContext] - Supplies the normal context parameter that was specified
;       when the APC was initialized.
;
;    SystemArgument1 - Supplies the first argument that was provied by the
;       executive when the APC was queued.
;
;    SystemArgument2 - Supplies the second argument that was provided by
;       the executive when the APC was queued.
;
;    ContinueContext - Context record to pass to Continue call.
;
;
; Return Value:
;
;    None.
;
;--
cPublicProc _KiUserApcDispatcher ,5

        lea     edi, [esp+16]           ; (edi)->context frame
        pop     eax                     ; (eax)->specified function
        call    eax                     ; call the specified function

; 1 - set alert argument true
; ebp - addr of context frame
; execute system service to continue
        stdCall   _ZwContinue, <edi, 1>

stdENDP _KiUserApcDispatcher


        page
        subttl  "User Callback Dispatcher"
;++
;
; VOID
; KiUserCallbackDispatcher (
;    IN ULONG ApiNumber,
;    IN PVOID InputBuffer,
;    IN ULONG INputLength
;    )
;
; Routine Description:
;
;    This routine is entered on a callout from kernel mode to execute a
;    user mode callback function. All arguments for this function have
;    been placed on the stack.
;
; Arguments:
;
;    ApiNumber - Supplies the API number of the callback function that is
;        executed.
;
;    InputBuffer - Supplies a pointer to the input buffer.
;
;    InputLength - Supplies the input buffer length.
;
; Return Value:
;
;    This function returns to kernel mode.
;
;--
cPublicProc _KiUserCallbackDispatcher, 3
.FPO (0, 0, 0, 0, 0, 0)

        add     esp,4                   ; skip over return address
        pop     edx                     ; get address of callback function

                                        ; get peb pointer from teb
        mov     eax,fs:[PcTeb]
        mov     eax,[eax].TebPeb
        mov     eax,[eax].PebKernelCallbackTable    ; get address of callback table

        call    [eax+edx*4]             ; call specified function

;
; If a return from the callback function occurs, then the output buffer
; address and length are returned as NULL.
;

        xor     ecx,ecx                 ; clear output buffer address
ifdef BUILD_WOW6432
        stdCall _ZwCallbackReturn, <ecx, ecx, eax>
else
        xor     edx,edx                 ; clear output buffer length
        int     02bH                    ; return from callback
endif
        int     3                       ; break if return occurs

stdENDP _KiUserCallbackDispatcher

endif  ;; ndef WX86_i386

        page
        subttl  "User Exception Dispatcher"
;++
;
; VOID
; KiUserExceptionDispatcher (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PCONTEXT ContextRecord
;    )
;
; Routine Description:
;
;    This routine is entered on return from kernel mode to dispatch a user
;    mode exception. If a frame based handler handles the exception, then
;    the execution is continued. Else last chance processing is performed.
;
;    NOTE:  This procedure is not called, but rather dispatched to.
;           It depends on there not being a return address on the stack
;           (assumption w.r.t. argument offsets.)
;
; Arguments:
;
;    ExceptionRecord (esp+0) - Supplies a pointer to an exception record.
;
;    ContextRecord (esp+4) - Supplies a pointer to a context frame.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiUserExceptionDispatcher      ,2
.FPO (0, 2, 0, 0, 0, 0)

        mov     ecx, [esp+4]            ; (ecx)->context record
        mov     ebx, [esp]              ; (ebx)->exception record

; attempt to dispatch the exception
        stdCall   _RtlDispatchException, <ebx, ecx>

;
; If the return status is TRUE, then the exception was handled and execution
; should be continued with the NtContinue service in case the context was
; changed. If the return statusn is FALSE, then the exception was not handled
; and ZwRaiseException is called to perform last chance exception processing.
;

        or      al,al
        je      short kued10

;
; Continue execution.
;

        pop     ebx                     ; (ebx)->exception record
        pop     ecx                     ; (ecx)->context record

; continue execution
        stdCall   _ZwContinue, <ecx, 0>
        jmp     short kued20            ; join common code

;
; Last chance processing.
;
;   (esp+0) = ExceptionRecord
;   (esp+4) = ContextRecord
;

kued10: pop     ebx                     ; (ebx)->exception record
        pop     ecx                     ; (ecx)->context record

; ecx - context record
; ebx - exception record
; perform last chance processiong
        stdCall   _ZwRaiseException, <ebx, ecx, 0>

;
; Common code for nonsuccessful completion of the continue or raiseexception
; services. Use the return status as the exception code, set noncontinuable
; exception and attempt to raise another exception. Note the stack grows
; and eventually this loop will end.
;

.FPO(0, 0, 0, 0, 0, 0)

kued20: add     esp, -ExceptionRecordSize ; allocate stack space
        mov     [esp]+ErExceptionCode, eax ; set exception code
        mov     dword ptr [esp]+ErExceptionFlags, EXCEPTION_NONCONTINUABLE
        mov     [esp]+ErExceptionRecord,ebx ; set associated exception record
        mov     dword ptr [esp]+ErNumberParameters, 0
                                        ; set number of parameters
; esp - addr of exception record
        stdCall   _RtlRaiseException, <esp>
; never return
        stdRET    _KiUserExceptionDispatcher

stdENDP _KiUserExceptionDispatcher

        page
        subttl  "Raise User Exception Dispatcher"
ifndef WX86_i386
;++
;
; NTSTATUS
; KiUserExceptionDispatcher (
;    IN PVOID ReturnAddress
;    IN NTSTATUS ExceptionCode
;    )
;
; Routine Description:
;
;    This routine is entered on return from kernel mode to raise a user
;    mode exception.
;
;    NOTE:  This procedure is not called, but rather dispatched to.
;
;    The address this routine must return to is passed in EAX.
;    The exception code to be raised is passed in the TEB.
;
; Arguments:
;
;    ReturnAddress (eax) - Supplies the address to return to
;
;    ExceptionCode (TEB->ExceptionCode) - Supplies the exception code to be raised
;
; Return Value:
;
;    The exception code that was raised.
;
;--

cPublicProc _KiRaiseUserExceptionDispatcher

        push    eax                     ; push address to return to
        push    ebp                     ; make the debugger happy
        mov     ebp, esp
        sub     esp, ExceptionRecordLength      ; allocate exception record
        mov     [esp].ErExceptionAddress, eax   ; set exception address
        mov     eax,fs:[PcTeb]                  ; get exception code to be raised
        mov     eax,[eax].TbExceptionCode       ;
        mov     [esp].ErExceptionCode, eax      ; store exception code
        mov     [esp].ErExceptionFlags, 0       ; set exception flags
        mov     [esp].ErExceptionRecord, 0      ; set exception record
        mov     [esp].ErNumberParameters, 0     ; set number of parameters
; raise the exception
        stdCall   _RtlRaiseException, <esp>
        mov     eax, [esp].ErExceptionCode
        mov     esp,ebp
        pop     ebp                     ; restore return code
        ret

stdENDP _KiRaiseUserExceptionDispatcher
endif  ;; ndef WX86_i386


_TEXT   ENDS

        END
