;++
;
; Copyright (c) 1991  Microsoft Corporation
;
; Module Name:
;
;    ntnap.asm
;
; Abstract:
;
;    This module implements the system service dispatch procedure.
;    It also creates a "profile" of each service by counting and
;    timing calls.
;
; Author:
;
;    Russ Blake (russbl) 22-Apr-1991
;
; Environment:
;
;    User or kernel mode.
;
; Revision History:
;
;--

include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
include ntnap.inc

.386

EXTRN           _NapDllInit:near
EXTRN           _NapRecordInfo:near

NapStart        equ     [ebp - 08h]
NapEnd          equ     [ebp - 010h]
NapServiceNum   equ     [ebp - 014h]

NapLocalSize    equ     4 * 5

NapCalSrvNum    equ     0FFFFFFFFh

;++
;
; Routine Description:
;
;    This routine is called to save registers during API profiling.
;    The objecttive is to preserve the caller's environment
;    while timing takes place and, once, while dll initialization
;    takes place.  This routine svaes registers on the stack to
;    permit recursivce calls.
;
;    There should be a matching call to NapRestoreRegs to restore
;    the registers.
;
; Arguments:
;
;    All registers.
;
; Return Value:
;
;    None.  All registers are preserved on the stack.
;
;--


.386p

_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

cPublicProc _NapSaveRegs

    ;
    ; This is how the stack looks like upon entering this routine:
    ;
    ;    ---+----+----+----+----+----
    ;       |   Return Address  |
    ;    ---+----+----+----+----+----
    ;        esp+                esp+
    ;        0                   4
    ;
    ;
    ; -> popping makes esp go ->
    ; <- pushing makes esp go <-
    ;

        push    ebp
        mov     ebp,esp         ; Remember where we are during this stuff
                                ; ebp = Original esp - 4
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        pushfd
        push    ds
        push    es
        push    ss
        push    fs
        push    gs

        mov     eax,[ebp+4]     ; Grab Return Address
        push    eax             ; Put Return Address on Stack
        mov     ebp,[ebp+0]     ; Restore original ebp

    ;
    ; This is how the stack looks like just before executing RET:
    ;
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |  Return  Address  |        g s        |        f s        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     0
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |        s s        |        e s        |        d s        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     c
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |      eflags       |        edi        |        esi        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |        edx        |        ecx        |        ebx        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+----
    ;    |        eax        |   original  ebp   |   Return Address  |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+----
    ;                         was
    ;                         ebp+
    ;                         0
    ;

        stdRET    _NapSaveRegs

stdENDP _NapSaveRegs

cPublicProc _NapRestoreRegs,,near

    ;
    ; This is how the stack looks like upon entering this routine:
    ;
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |  Return  Address  |        g s        |        f s        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     0
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |        s s        |        e s        |        d s        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     c
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |      eflags       |        edi        |        esi        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     18
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;    |        edx        |        ecx        |        ebx        |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+
    ;     esp+
    ;     24
    ;
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+----
    ;    |        eax        |   original  ebp   |   Return Address  |
    ;    +----+----+----+----+----+----+----+----+----+----+----+----+----
    ;     esp+                esp+                esp+
    ;     30                  34                  38
    ;
        pop     eax             ; Get Return Address
        push    ebp             ; Save a temporary copy of original BP
        mov     ebp,esp         ; BP = Original SP + 4

        mov     [ebp+038h],eax  ; Put Return Address on Stack
        pop     eax             ; Get Original BP
        mov     [ebp+034h],eax  ; Put it in the original BP place

        pop     gs
        pop     fs
        pop     ss
        pop     es
        pop     ds
        popfd
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp

        stdRET    _NapRestoreRegs

stdENDP _NapRestoreRegs


;++
;
; Routine Description:
;
;    This routine is called by the initialization code in the
;    Nt Api Profiler to calibrate the cost of profiling.
;    It simulates the overhead of a profiled call to a system
;    service, but carefully avoids doing any of the normal
;    work associated with such a call.
;
;    NOTE:  This routine's code should exactly parallel that of
;           _NapDispatch, except for any operation normally
;           (i.e., when not profiling) executed to call a system service.
;           This amounts to an "int 2Eh" in the middle of the routine.
;
; Arguments:
;
;    eax - Service Number of the routine being called.  Must be -1
;          for all calls to this routine.  The routine
;          _NapRecordInfo notes this value and discards
;          the call.
;
;    edx - Pointer to the parameters to the Service; ignored by
;          this routine.
;
; Return Value:
;
;    None.
;
;--


cPublicProc _NapCalibrate   , ,near


        push    ebp                     ; Locals: the value of
        mov     ebp, esp                ; the perf counter before and
        sub     esp, NapLocalSize       ; after the API call

        mov     eax, NapCalSrvNum       ; special routine number
        mov     NapServicenum, eax      ; is used for calibration
                                        ; can't be passed in eax from
                                        ; C routine, so load it here
                                        ; save the service routine number


        stdCall    _NapSaveRegs           ; save register state so call to
                                        ; get counter does not destroy them

        stdCall    _NapDllInit            ; initialize dll if necessary

; Now call NtQueryPerformanceCounter to get the starting count;
; Store this locally

        push    0                       ; don't need frequency: pass 0
        lea     eax, NapStart           ; (eax) = pointer to counter
        push    eax                     ; pass pointer to counter
        mov     eax, NapCounterServiceNumber
        lea     edx, [esp]              ; (edx) -> arguments
        int     2Eh                     ; get the current counter value
        add     esp, 08h                ; remove counter parameters

; Restore caller's registers

        stdCall   _NapRestoreRegs

; We're just calibrating the overhead, so we don't call the system
; service here.

; Save regsiters so we can complete the profile accounting.

        stdCall   _NapSaveRegs

; Now get the ending counter.

        push    0                       ; don't need frequency: pass 0
        lea     eax, NapEnd             ; (eax) = pointer to counter
        push    eax                     ; pass pointer to counter
        mov     eax, NapCounterServiceNumber
        lea     edx, [esp]              ; (edx) -> arguments
        int     2Eh                     ; get the current counter value
        add     esp, 08h                ; remove counter parameters

; Compute the time for this call and increment the nukmber of calls.

        lea     eax, NapEnd             ; pointer to start/end counters
                                        ; ID of this routine
        stdCall   _NapRecordInfo, <NapServiceNum, eax>

        stdCall   _NapRestoreRegs
                                        ; restore caller's registers
        leave                           ; we needed this for pseudo locals
        stdRET    _NapCalibrate
stdENDP _NapCalibrate


;++
;
; Routine Description:
;
;    This routine is called by the USRSTUBS_ENTRY1 MACRO in the
;    services.prf to carry out profiling on an Nt system api call.
;
; Arguments:
;
;    eax - Service Number of the routine being called.  This number
;          is assigned by genprof.c from the table in services.tab.
;
;    edx - Pointer to the parameters to the Service.
;
; Return Value:
;
;    Whatever the system service returns.
;
;--



cPublicProc _NapProfileDispatch , ,near

        push    ebp                     ; Locals: the value of
        mov     ebp, esp                ; the perf counter before and
        sub     esp, NapLocalSize       ; after the API call

        mov     NapServicenum, eax
                                        ; save the service routine number

        stdCall   _NapSaveRegs            ; save register state so call to
                                        ; get counter does not destroy them

        stdCall   _NapDllInit             ; initialize dll if necessary

; Now call NtQueryPerformanceCounter to get the starting count;
; Store this locally

        push    0                       ; don't need frequency: pass 0
        lea     eax, NapStart           ; (eax) = pointer to counter
        push    eax                     ; pass pointer to counter
        mov     eax, NapCounterServiceNumber
        lea     edx, [esp]              ; (edx) -> arguments
        int     2Eh                     ; get the current counter value
        add     esp, 08h                ; remove counter parameters

; Restore caller's registers

        stdCall   _NapRestoreRegs

        INT     2Eh                     ; invoke system service

; Save regsiters so we can complete the profile accounting.

        stdCall   _NapSaveRegs

; Now get the ending counter.

        push    0                       ; don't need frequency: pass 0
        lea     eax, NapEnd             ; (eax) = pointer to counter
        push    eax                     ; pass pointer to counter
        mov     eax, NapCounterServiceNumber
        lea     edx, [esp]              ; (edx) -> arguments
        int     2Eh                     ; get the current counter value
        add     esp, 08h                ; remove counter parameters

; Compute the time for this call and increment the number of calls.

        lea     eax, NapEnd             ; pointer to start/end counters
                                        ; ID of this routine
        stdCall   _NapRecordInfo, <NapServiceNum, eax>

        stdCall   _NapRestoreRegs
                                        ; restore caller's registers
        leave                           ; we needed this for pseudo locals
        stdRET    _NapProfileDispatch
stdENDP _NapProfileDispatch

;++
;
; Routine Description:
;
;    This routine is claled to get the spin lock associated with
;    a particular api.  It prevents the simultaneous update
;    from multiple threads in this or other processors of the
;    profiling data for the api.
;
; Arguments:
;
;    SpinLockAddr - address of the spin lock within the data
;                   for the api being updated.
;
; Return Value:
;
;    None.
;
;--


cPublicProc _NapAcquireSpinLock      , ,near

        push    eax
        mov     eax, [esp+8]            ; get address of lock
WaitForLock:
        lock bts dword ptr [eax], 0     ; test and set the spinlock
        jc      SHORT WaitForLock       ; spinlock owned: go to SpinLabel
        pop     eax

        stdRET    _NapAcquireSpinLock

stdENDP _NapAcquireSpinLock


;++
;
; Routine Description:
;
;    This routine is called to release the spin lock associated with
;    a particular api.
;
; Arguments:
;
;    SpinLockAddr - address of the spin lock within the data
;                   for the api being updated.
;
; Return Value:
;
;    None.
;
;--


cPublicProc _NapReleaseSpinLock     , ,near

        push    eax
        mov     eax, [esp+8]            ; get address of lock
        lock btr dword ptr [eax], 0     ; release spinlock
        pop     eax
        stdRET    _NapReleaseSpinLock

stdENDP _NapReleaseSpinLock


_TEXT           ends

                end
