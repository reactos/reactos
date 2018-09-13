        TITLE  "Interrupt Object Support Routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    intsup.asm
;
; Abstract:
;
;    This module implements the code necessary to support interrupt objects.
;    It contains the interrupt dispatch code and the code template that gets
;    copied into an interrupt object.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 20-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--
.386p
        .xlist
KERNELONLY  equ     1
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeBugCheck,1
        EXTRNP  _KiDeliverApc,3
        EXTRNP  _HalBeginSystemInterrupt,3,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2,IMPORT
        EXTRNP  Kei386EoiHelper
if DBG
        extrn   _DbgPrint:near
        extrn   _MsgISRTimeout:BYTE
        extrn   _MsgISROverflow:BYTE
        extrn   _KeTickCount:DWORD
        extrn   _KiISRTimeout:DWORD
        extrn   _KiISROverflow:DWORD
endif

MI_MOVEDI       EQU     0BFH            ; op code for mov  edi, constant
MI_DIRECTJMP    EQU     0E9H            ; op code for indirect jmp
                                        ; or index registers

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

if DBG
        public  KiInterruptCounts
KiInterruptCounts   dd  256*2 dup (0)
endif

_DATA   ends

        page ,132
        subttl  "Synchronize Execution"

_TEXT$00   SEGMENT PARA PUBLIC 'CODE'

;++
;
; BOOLEAN
; KeSynchronizeExecution (
;    IN PKINTERRUPT Interrupt,
;    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
;    IN PVOID SynchronizeContext
;    )
;
; Routine Description:
;
;    This function synchronizes the execution of the specified routine with the
;    execution of the service routine associated with the specified interrupt
;    object.
;
; Arguments:
;
;    Interrupt - Supplies a pointer to a control object of type interrupt.
;
;    SynchronizeRoutine - Supplies a pointer to a function whose execution
;       is to be synchronized with the execution of the service routine associated
;       with the specified interrupt object.
;
;    SynchronizeContext - Supplies a pointer to an arbitrary data structure
;       which is to be passed to the function specified by the SynchronizeRoutine
;       parameter.
;
; Return Value:
;
;    The value returned by the SynchronizeRoutine function is returned as the
;    function value.
;
;--
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
cPublicProc _KeSynchronizeExecution ,3

; equates of Local variables

KsePreviousIrql equ [ebp - 4]                ; previous IRQL
KseStackSize = 4 * 1

; equates for arguments

KseInterrupt equ [ebp +8]
KseSynchronizeRoutine equ [ebp + 12]
KseSynchronizeContext equ [ebp + 16]

        push    ebp                     ; save ebp
        mov     ebp, esp                ; (ebp)-> base of local variable frame
        sub     esp, KseStackSize       ; allocate local variables space
        push    ebx                     ; save ebx
        push    esi                     ; save esi

; Acquire the associated spin lock and raise IRQL to the interrupting source.

        mov     ebx, KseInterrupt       ; (ebx)->interrupt object

        mov     ecx, InSynchronizeIrql[ebx] ; (ecx) = Synchronize Irql
        fstCall KfRaiseIrql
        mov     KsePreviousIrql, al

kse10:  mov     esi,[ebx+InActualLock]  ; (esi)->Spin lock variable
        ACQUIRE_SPINLOCK esi,<short kse20>

; Call specified routine passing the specified context parameter.
        mov     eax,KseSynchronizeContext
        push    eax
        call    KseSynchronizeRoutine
        mov     ebx, eax                ; save function returned value

; Unlock spin lock, lower IRQL to its previous level, and return the value
; returned by the specified routine.

        RELEASE_SPINLOCK esi

        mov     ecx, KsePreviousIrql
        fstCall KfLowerIrql

        mov     eax, ebx                ; (eax) = returned value
        pop     esi                     ; restore esi
        pop     ebx                     ; restore ebx
        leave                           ; will clear stack
        stdRET    _KeSynchronizeExecution

; Lock is already owned; spin until free and then attempt to acquire lock
; again.

ifndef NT_UP
kse20:  SPIN_ON_SPINLOCK esi,<short kse10>,,DbgMp
endif

stdENDP _KeSynchronizeExecution

        page ,132
        subttl  "Chained Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to more than one interrupt object.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--


align 16
cPublicProc _KiChainedDispatch      ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;

        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

;
; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level
;

        mov     eax, [edi].InVector     ; save vector
        push    eax
        sub     esp, 4                  ; make room for OldIrql
        mov     ecx, [edi].InIrql       ; Irql

;
; esp - pointer to OldIrql
; eax - vector
; ecx - Irql
;

        stdCall   _HalBeginSystemInterrupt, <ecx, eax, esp>
        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

        stdCall _KiChainedDispatch2ndLvl

        INTERRUPT_EXIT                  ; will do an iret

stdENDP _KiChainedDispatch


        page ,132
        subttl  "Chained Dispatch 2nd Level"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is either connected to more than one interrupt object,
;    or is being 2nd level dispatched.  Its function is to walk the list
;    of connected interrupt objects and call each interrupt service routine.
;    If the mode of the interrupt is latched, then a complete traversal of
;    the chain must be performed. If any of the routines require saving the
;    floating point machine state, then it is only saved once.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;
; Return Value:
;
;   None.
;   Uses all registers
;
;--


public _KiInterruptDispatch2ndLvl@0
_KiInterruptDispatch2ndLvl@0:
        nop

cPublicProc _KiChainedDispatch2ndLvl,0
cPublicFpo 0, 3

        push    ebp
        sub     esp, 8                  ; Make room for scratch value
        xor     ebp, ebp                ; init (ebp) = Interrupthandled = FALSE
        lea     ebx, [edi].InInterruptListEntry
                                        ; (ebx)->Interrupt Head List

;
; Walk the list of connected interrupt objects and call the appropriate dispatch
; routine.
;

kcd40:

;
; Raise irql level to the SynchronizeIrql level if it is not equal to current
; irql.
;

        mov     cl, [edi+InIrql]        ; [cl] = Current Irql
        mov     esi,[edi+InActualLock]
        cmp     [edi+InSynchronizeIrql], cl ; Is SyncIrql > current IRQL?
        je      short kcd50             ; if e, no, go kcd50

;
; Arg2 esp : Address of OldIrql
; Arg1 eax : Irql to raise to
;

        mov     ecx, [edi+InSynchronizeIrql] ; (eax) = Irql to raise to
        fstCall KfRaiseIrql
        mov     [esp], eax              ; Save OldIrql


;
; Acquire the service routine spin lock and call the service routine.
;

kcd50:
        ACQUIRE_SPINLOCK esi,kcd110
if DBG
        mov     eax, _KeTickCount       ; Grab ISR start time
        mov     [esp+4], eax            ; save to local varible
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        call    InServiceRoutine[edi]   ; call specified routine

if DBG
        mov     ecx, [esp+4]            ; (ecx) = time isr started
        add     ecx, _KiISRTimeout      ; adjust for timeout
        cmp     _KeTickCount, ecx       ; Did ISR timeout?
        jnc     kcd200
kcd51:
endif

;
; Release the service routine spin lock and check to determine if end of loop.
;

        RELEASE_SPINLOCK esi

;
; Lower IRQL to earlier level if we raised it to SynchronizedLevel.
;

        mov     cl, [edi+InIrql]
        cmp     [edi+InSynchronizeIrql], cl ; Is SyncIrql > current IRQL?
        je      short kcd55             ; if e, no, go kcd55

        mov     esi, eax                ; save ISR returned value

;
; Arg1 : Irql to Lower to
;

        mov     ecx, [esp]
        fstCall KfLowerIrql

        mov     eax, esi                ; [eax] = ISR returned value
kcd55:
        or      al,al                   ; Is interrupt handled?
        je      short kcd60             ; if eq, interrupt not handled
        cmp     word ptr InMode[edi], InLevelSensitive
        je      short kcd70             ; if eq, level sensitive interrupt

        mov     ebp, eax                ; else edge shared int is handled. Remember it.
kcd60:  mov     edi, [edi].InInterruptListEntry
                                        ; (edi)->next obj's addr of listentry
        cmp     ebx, edi                ; Are we at end of interrupt list?
        je      short kcd65             ; if eq, reach end of list
        sub     edi, InInterruptListEntry; (edi)->addr of next interrupt object
        jmp     kcd40

kcd65:
;
; If this is edge shared interrupt, we need to loop till no one handle the
; interrupt.  In theory only shared edge triggered interrupts come here.
;

        sub     edi, InInterruptListEntry; (edi)->addr of next interrupt object
        cmp     word ptr InMode[edi], InLevelSensitive
        je      short kcd70             ; if level, exit.  No one handle the interrupt?

        test    ebp, 0fh                ; does anyone handle the interrupt?
        je      short kcd70             ; if e, no one, we can exit.

        xor     ebp, ebp                ; init local var to no one handle the int
        jmp     kcd40                   ; restart the loop.

;
; Either the interrupt is level sensitive and has been handled or the end of
; the interrupt object chain has been reached.
;

; restore frame pointer, and deallocate trap frame.

kcd70:
        add     esp, 8                  ; clear local variable space
        pop     ebp
        stdRet  _KiChainedDispatch2ndLvl


; Service routine Lock is currently owned, spin until free and then
; attempt to acquire lock again.

ifndef NT_UP
kcd110: SPIN_ON_SPINLOCK esi, kcd50,,DbgMp
endif

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kcd200: push    eax                     ; save return code
        push    InServiceRoutine[edi]
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        pop     eax
        int     3
        jmp     kcd51                   ; continue
endif

stdENDP _KiChainedDispatch2ndLvl


        page ,132
        subttl  "Floating Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to an interrupt object. Its function is
;    to save the machine state and floating state and then call the specified
;    interrupt service routine.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--

align 16
cPublicProc _KiFloatingDispatch     ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;
        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level to interrupt obj's SyncIrql
;
        mov     eax, [edi].InVector
        mov     ecx, [edi].InSynchronizeIrql ; Irql
        push    eax                     ; save vector
        sub     esp, 4                  ; make room for OldIrql

; arg3 - ptr to OldIrql
; arg2 - vector
; arg1 - Irql
        stdCall   _HalBeginSystemInterrupt, <ecx, eax, esp>

        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

;
; Acquire the service routine spin lock and call the service routine.
;

kfd30:  mov     esi,[edi+InActualLock]
        ACQUIRE_SPINLOCK esi,kfd100

if DBG
        mov     ebx, _KeTickCount       ; Grab current tick time
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        call    InServiceRoutine[edi]   ; call specified routine
if DBG
        add     ebx, _KiISRTimeout      ; adjust for ISR timeout
        cmp     _KeTickCount, ebx       ; Did ISR timeout?
        jnc     kfd200
kfd31:
endif

;
; Release the service routine spin lock.
;

        RELEASE_SPINLOCK esi

;
; Do interrupt exit processing
;
        INTERRUPT_EXIT                  ; will do an iret

;
; Service routine Lock is currently owned; spin until free and
; then attempt to acquire lock again.
;

ifndef NT_UP
kfd100: SPIN_ON_SPINLOCK esi,kfd30,,DbgMp
endif

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kfd200: push    InServiceRoutine[edi]   ; timed out
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        int     3
        jmp     kfd31                   ; continue
endif

stdENDP _KiFloatingDispatch

        page ,132
        subttl  "Interrupt Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to an interrupt object. Its function is
;    to directly call the specified interrupt service routine.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--

align 16
cPublicProc _KiInterruptDispatch    ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;
        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

;
; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level
;
        mov     eax, [edi].InVector     ; save vector
        mov     ecx, [edi].InSynchronizeIrql ; Irql to raise to
        push    eax
        sub     esp, 4                  ; make room for OldIrql

        stdCall   _HalBeginSystemInterrupt,<ecx, eax, esp>

        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

;
; Acquire the service routine spin lock and call the service routine.
;

kid30:  mov     esi,[edi+InActualLock]
        ACQUIRE_SPINLOCK esi,kid100
if DBG
        mov     ebx, [edi].InVector             ; this vector
        mov     eax, _KeTickCount               ; current time
        and     eax, NOT 31                     ; mask to closest 1/2 second
        shl     ebx, 3                          ; eax = eax * 8
        cmp     eax, [KiInterruptCounts+ebx]        ; in same 1/2 range?
        jne     kid_overflowreset

        mov     eax, _KiISROverflow
        inc     [KiInterruptCounts+ebx+4]
        cmp     [KiInterruptCounts+ebx+4], eax
        jnc     kid_interruptoverflow
kid_dbg2:
        mov     ebx, _KeTickCount
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        call    InServiceRoutine[edi]   ; call specified routine

if DBG
        add     ebx, _KiISRTimeout      ; adjust for ISR timeout
        cmp     _KeTickCount, ebx       ; Did ISR timeout?
        jnc     kid200
kid31:
endif

;
; Release the service routine spin lock, retrieve the return address,
; deallocate stack storage, and return.
;

        RELEASE_SPINLOCK esi

;
; Do interrupt exit processing
;

        INTERRUPT_EXIT                  ; will do an iret

        add     esp, 8                  ; clean stack

kid_spuriousinterrupt:
        add     esp, 8                  ; Irql wasn't raised, exit interrupt
        SPURIOUS_INTERRUPT_EXIT         ; without eoi or lower irql

;
; Lock is currently owned; spin until free and then attempt to acquire
; lock again.
;

ifndef NT_UP
kid100: SPIN_ON_SPINLOCK esi,kid30,,DbgMp
endif

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kid200: push    InServiceRoutine[edi]   ; timed out
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        int     3
        jmp     kid31                   ; continue

kid_interruptoverflow:
        push    [KiInterruptCounts+ebx+4]
        push    InServiceRoutine[edi]
        push    offset FLAT:_MsgISROverflow
        call    _DbgPrint
        add     esp,12
        int 3

        mov     eax, _KeTickCount               ; current time
        and     eax, NOT 31                     ; mask to closest 1/2 second

kid_overflowreset:
        mov     [KiInterruptCounts+ebx], eax        ; initialize time
        mov     [KiInterruptCounts+ebx+4], 0        ; reset count
        jmp     kid_dbg2
endif



stdENDP _KiInterruptDispatch

        page ,132
        subttl  "Interrupt Template"
;++
;
; Routine Description:
;
;    This routine is a template that is copied into each interrupt object. Its
;    function is to save machine state and pass the address of the respective
;    interrupt object and transfer control to the appropriate interrupt
;    dispatcher.
;
;    Control comes here through i386 interrupt gate and, upon entry, the
;    interrupt is disabled.
;
;    Note: If the length of this template changed, the corresponding constant
;          defined in Ki.h needs to be updated accordingly.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    edi - addr of interrupt object
;    esp - top of trap frame
;    interrupts are disabled
;
;--

_KiShutUpAssembler      proc

        public  _KiInterruptTemplate
_KiInterruptTemplate    label   byte

; Save machine state on trap frame

        ENTER_INTERRUPT kit_a,  kit_t

;
; the following instruction gets the addr of associated interrupt object.
; the value ? will be replaced by REAL interrupt object address at
; interrupt object initialization time.
;       mov     edi, addr of interrupt object
; 
; Template modifications made to support BBT, include replacing bogus
; insructions (created by db and dd) with real instructions.   
; This stuff gets overwritten anyway.  BBT just needs to see real instructions.

        public  _KiInterruptTemplate2ndDispatch
_KiInterruptTemplate2ndDispatch equ     this dword
        mov      edi,0	

        public  _KiInterruptTemplateObject
_KiInterruptTemplateObject      equ     this dword


; the following instruction transfers control to the appropriate dispatcher
; code.  The value ? will be replaced by real InterruptObj.DispatchAddr
; at interrupt initialization time.  The dispatcher routine will be any one
; of _KiInterruptDispatch, _KiFloatingDispatch, or _KiChainDispatch.
;       jmp     [IntObj.DispatchAddr]

        jmp _KeSynchronizeExecution

        public  _KiInterruptTemplateDispatch
_KiInterruptTemplateDispatch    equ     this dword

        ENTER_DR_ASSIST kit_a,  kit_t

; end of _KiInterruptTemplate

if  ($ - _KiInterruptTemplate) GT DISPATCH_LENGTH
    .err
    %out    <InterruptTemplate greater than dispatch_length>
endif

_KiShutUpAssembler      endp

        page ,132
        subttl  "Unexpected Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is not connected to an interrupt object.
;
;    For any unconnected vector, its associated 8259 irq is masked out at
;    Initialization time.  So, this routine should NEVER be called.
;    If somehow, this routine gets control we simple raise a BugCheck and
;    stop the system.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    None.
;
;--
        public _KiUnexpectedInterrupt
_KiUnexpectedInterrupt  proc
cPublicFpo 0,0

; stop the system
        stdCall   _KeBugCheck, <TRAP_CAUSE_UNKNOWN>
        nop

_KiUnexpectedInterrupt endp

_TEXT$00   ends
        end
