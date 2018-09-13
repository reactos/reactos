        title   "Critical Section Support"
;++
;
;  Copyright (c) 1991  Microsoft Corporation
;
;  Module Name:
;
;     critsect.asm
;
;  Abstract:
;
;     This module implements functions to support user mode critical sections.
;
;  Author:
;
;     Bryan M. Willman (bryanwi) 2-Oct-91
;
;  Environment:
;
;     Any mode.
;
;  Revision History:
;
;--

.486p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list

_DATA   SEGMENT DWORD PUBLIC 'DATA'
    public _LdrpLockPrefixTable
_LdrpLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock2
        dd offset FLAT:Lock3
        dd offset FLAT:Lock4
        dd offset FLAT:Lock5
        dd offset FLAT:Lock6
        dd offset FLAT:Lock7
        dd 0
_DATA   ENDS

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        EXTRNP  _RtlpWaitForCriticalSection,1
        EXTRNP  _RtlpUnWaitCriticalSection,1
if DEVL
        EXTRNP  _RtlpNotOwnerCriticalSection,1
endif
if DBG
        EXTRNP  _RtlpCriticalSectionIsOwned,1
endif

CriticalSection equ     [esp + 4]

        page , 132
        subttl  "RtlEnterCriticalSection"

;++
;
; NTSTATUS
; RtlEnterCriticalSection(
;    IN PRTL_CRITICAL_SECTION CriticalSection
;    )
;
; Routine Description:
;
;    This function enters a critical section.
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;   STATUS_SUCCESS or raises an exception if an error occured.
;
;--

        align   16
cPublicProc _RtlEnterCriticalSection,1
cPublicFpo 1,0

        mov     ecx,fs:PcTeb            ; get current TEB address
        mov     edx,CriticalSection     ; get address of critical section
        cmp     CsSpinCount[edx],0      ; check if spin count is zero
        jne     short Ent40             ; if ne, spin count specified

;
; Attempt to acquire critical section.
;

Lock1:                                  ;
   lock inc     dword ptr CsLockCount[edx] ; increment lock count
        jnz     short Ent20             ; if nz, already owned

;
; Set critical section owner and initialize recursion count.
;

Ent10:
if DBG
        cmp     CsOwningThread[edx],0
        je      @F
        stdCall _RtlpCriticalSectionIsOwned, <edx>
        mov     ecx,fs:PcTeb            ; get current TEB address
        mov     edx,CriticalSection     ; get address of critical section
@@:
endif ; DBG
        mov     eax,TbClientId + 4[ecx] ; get current client ID
        mov     CsOwningThread[edx],eax ; set critical section owner
        mov     dword ptr CsRecursionCount[edx],1 ; set recursion count

if DBG

        inc     dword ptr TbCountOfOwnedCriticalSections[ecx] ; increment owned count
        mov     eax,CsDebugInfo[edx]    ; get debug information address
        inc     dword ptr CsEntryCount[eax] ; increment entry count

endif ; DBG

        xor     eax,eax                 ; set success status

        stdRET  _RtlEnterCriticalSection

;
; The critical section is already owned, but may be owned by the current thread.
;

        align   16
Ent20:  mov     eax,TbClientId + 4[ecx] ; get current client ID
        cmp     CsOwningThread[edx],eax ; check if current thread is owner
        jne     short Ent30             ; if ne, current thread not owner
        inc     dword ptr CsRecursionCount[edx] ; increment recursion count

if DBG

        mov     eax,CsDebugInfo[edx]    ; get debug information address
        inc     dword ptr CsEntryCount[eax] ; increment entry count

endif ; DBG

        xor     eax,eax                 ; set success status

        stdRET  _RtlEnterCriticalSection

;
; The critcal section is owned by another thread and the current thread must
; wait for ownership.
;

Ent30:  stdCall _RtlpWaitForCriticalSection, <edx> ; wait for ownership
        mov     ecx,fs:PcTeb            ; get current TEB address
        mov     edx,CriticalSection     ; get address of critical section
        jmp     Ent10                   ; set owner and recursion count

;
; A nonzero spin count is specified.
;

        align   16
Ent40:  mov     eax,TbClientId + 4[ecx] ; get current client ID
        cmp     CsOwningThread[edx],eax ; check if current thread is owner
        jne     short Ent50             ; if ne, current thread not owner

;
; The critical section is owned by the current thread. Increment the lock
; count and the recursion count.
;

Lock6:                                  ;
   lock inc     dword ptr CsLockCount[edx] ; increment lock count
        inc     dword ptr CsRecursionCount[edx] ; increment recursion count

if DBG

        mov     eax,CsDebugInfo[edx]    ; get debug information address
        inc     dword ptr CsEntryCount[eax] ; increment entry count

endif ; DBG

        xor     eax,eax                 ; set success status

        stdRET  _RtlEnterCriticalSection

;
; A nonzero spin count is specified and the current thread is not the owner.
;

        align   16
Ent50:  push    CsSpinCount[edx]        ; get spin count value
Ent60:  mov     eax,-1                  ; set comparand value
        mov     ecx,0                   ; set exchange value

Lock7:
   lock cmpxchg dword ptr CsLockCount[edx],ecx ; attempt to acquire critical section
        jnz     short Ent70             ; if nz, critical section not acquired

;
; The critical section has been acquired. Set the owning thread and the initial
; recursion count.
;

        add     esp,4                   ; remove spin count from stack
        mov     ecx,fs:PcTeb            ; get current TEB address
        mov     eax,TbClientId + 4[ecx] ; get current client ID
        mov     CsOwningThread[edx],eax ; set critical section owner
        mov     dword ptr CsRecursionCount[edx],1 ; set recursion count

if DBG

        inc     dword ptr TbCountOfOwnedCriticalSections[ecx] ; increment owned count
        mov     eax,CsDebugInfo[edx]    ; get debug information address
        inc     dword ptr CsEntryCount[eax] ; increment entry count

endif ; DBG

        xor     eax,eax                 ; set success status

        stdRET  _RtlEnterCriticalSection

;
; The critical section is currently owned. Spin until it is either unowned
; or the spin count has reached zero.
;
; If waiters are present, don't spin on the lock since we will never see it go free
;

Ent70:  cmp     CsLockCount[edx],1      ; check if waiters are present,
        jge     short Ent76             ; if ge 1, then do not spin

Ent75:  YIELD
        cmp     CsLockCount[edx],-1     ; check if lock is owned
        je      short Ent60             ; if e, lock is not owned
        dec     dword ptr [esp]         ; decrement spin count
        jnz     short Ent75             ; if nz, continue spinning
Ent76:  add     esp,4                   ; remove spin count from stack
        mov     ecx,fs:PcTeb            ; get current TEB address
        jmp     Lock1                   ;

stdENDP _RtlEnterCriticalSection

        page , 132
        subttl  "RtlLeaveCriticalSection"
;++
;
; NTSTATUS
; RtlLeaveCriticalSection(
;    IN PRTL_CRITICAL_SECTION CriticalSection
;    )
;
; Routine Description:
;
;    This function leaves a critical section.
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;   STATUS_SUCCESS or raises an exception if an error occured.
;
;--

        align   16
cPublicProc _RtlLeaveCriticalSection,1
cPublicFpo 1,0

        mov     edx,CriticalSection
if DBG
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        mov     eax,TbClientId+4[ecx]       ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        cmp     eax,CsOwningThread[edx]
        je      @F
        stdCall _RtlpNotOwnerCriticalSection, <edx>
        mov     eax,STATUS_INVALID_OWNER
        stdRET  _RtlLeaveCriticalSection
@@:
endif ; DBG
        xor     eax,eax                     ; Assume STATUS_SUCCESS
        dec     dword ptr CsRecursionCount[edx]
        jnz     leave_recurs                ; skip if only leaving recursion

        mov     CsOwningThread[edx],eax     ; clear owning thread id

if DBG
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        dec     dword ptr TbCountOfOwnedCriticalSections[ecx]
endif ; DBG

Lock2:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        jge     @F
        stdRET  _RtlLeaveCriticalSection

@@:
        stdCall _RtlpUnWaitCriticalSection, <edx>
        xor     eax,eax                     ; return STATUS_SUCCESS
        stdRET  _RtlLeaveCriticalSection

        align   16
leave_recurs:
Lock3:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        stdRET  _RtlLeaveCriticalSection

_RtlLeaveCriticalSection    endp

        page    ,132
        subttl  "RtlTryEnterCriticalSection"
;++
;
; BOOL
; RtlTryEnterCriticalSection(
;    IN PRTL_CRITICAL_SECTION CriticalSection
;    )
;
; Routine Description:
;
;    This function attempts to enter a critical section without blocking.
;
; Arguments:
;
;    CriticalSection (a0) - Supplies a pointer to a critical section.
;
; Return Value:
;
;    If the critical section was successfully entered, then a value of TRUE
;    is returned as the function value. Otherwise, a value of FALSE is returned.
;
;--

CriticalSection equ     [esp + 4]

cPublicProc _RtlTryEnterCriticalSection,1
cPublicFpo 1,0

        mov     ecx,CriticalSection         ; interlocked inc of
        mov     eax, -1                     ; set value to compare against
        mov     edx, 0                      ; set value to set
Lock4:
   lock cmpxchg dword ptr CsLockCount[ecx],edx  ; Attempt to acquire critsect
        jnz     short tec10                 ; if nz, critsect already owned

        mov     eax,fs:TbClientId+4         ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        mov     CsOwningThread[ecx],eax
        mov     dword ptr CsRecursionCount[ecx],1

if DBG
        mov     eax,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        inc     dword ptr TbCountOfOwnedCriticalSections[eax]
endif ; DBG

        mov     eax, 1                      ; set successful status

        stdRET  _RtlTryEnterCriticalSection

tec10:
;
; The critical section is already owned. If it is owned by another thread,
; return FALSE immediately. If it is owned by this thread, we must increment
; the lock count here.
;
        mov     eax, fs:TbClientId+4        ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        cmp     CsOwningThread[ecx], eax
        jz      tec20                       ; if eq, this thread is already the owner
        xor     eax, eax                    ; set failure status
        YIELD
        stdRET  _RtlTryEnterCriticalSection

tec20:
;
; This thread is already the owner of the critical section. Perform an atomic
; increment of the LockCount and a normal increment of the RecursionCount and
; return success.
;
Lock5:
   lock inc     dword ptr CsLockCount[ecx]
        inc     dword ptr CsRecursionCount[ecx]
        mov     eax, 1
        stdRET  _RtlTryEnterCriticalSection

stdENDP _RtlTryEnterCriticalSection


_TEXT   ends
        end
