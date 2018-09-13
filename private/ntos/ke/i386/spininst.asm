if NT_INST
        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     spininst.asm
;
;  Abstract:
;
;     This module implements the instrumentation versions of the routines
;     for acquiring and releasing spin locks.
;
;  Author:
;
;     Ken Reneris
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;--

        PAGE

.386p

include ks386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc

        EXTRNP  _KeRaiseIrql,2,IMPORT
        EXTRNP  _KeLowerIrql,1,IMPORT
        EXTRNP  _KeBugCheckEx,5

ifdef NT_UP
        .err    SpinLock instrutmentation requires MP build
endif

s_SpinLock struc
        SpinLock        dd  ?   ; Back pointer to spinlock
        InitAddr        dd  ?   ; Address of KeInitializeSpinLock caller
        LockValue       db  ?   ; Actual lock varible
        LockFlags       db  ?   ; Various flags
                        dw  ?
        NoAcquires      dd  ?   ; # of times acquired
        NoCollides      dd  ?   ; # of times busy on acquire attempt
        TotalSpinHigh   dd  ?   ; number spins spent waiting on this spinlock
        TotalSpinLow    dd  ?
        HighestSpin     dd  ?   ; max spin ever waited for on this spinlock
s_SpinLock ends

LOCK_LAZYINIT   equ     1h
LOCK_NOTTRACED  equ     2h



_DATA   SEGMENT  DWORD PUBLIC 'DATA'

MAXSPINLOCKS    equ     1000h
SYSTEM_ADDR     equ     80000000h

        public  _KiNoOfSpinLocks, _KiSpinLockBogus, _KiSpinLockArray
        public  _KiSpinLockFreeList
_KiNoOfSpinLocks    dd      1       ; skip first one
_KiSpinLockBogus    dd      0
_KiSpinLockLock     dd      0

_KiSpinLockArray    db ((size s_SpinLock) * MAXSPINLOCKS) dup (0)

_KiSpinLockFreeList dd  0

_DATA   ends



_TEXT$00   SEGMENT  DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        PAGE
        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  VOID
;  KeInializeSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;
;  Routine Description:
;
;     This function initializes a SpinLock
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     None.
;
;--
cPublicProc _KeInitializeSpinLock  ,1
        pushfd
        cli
@@: lock bts    _KiSpinLockLock, 0
        jc      short @b

        mov     eax, _KiSpinLockFreeList
        or      eax, eax
        jz      short isl10

        mov     ecx, [eax].InitAddr
        mov     _KiSpinLockFreeList, ecx
        jmp     short isl20

isl10:
        mov     eax, _KiNoOfSpinLocks
        cmp     eax, MAXSPINLOCKS
        jnc     isl_overflow

        inc     _KiNoOfSpinLocks

.errnz (size s_SpinLock - (8*4))
        shl     eax, 5
        add     eax, offset _KiSpinLockArray

isl20:
; (eax) = address of spinlock structure
        mov     ecx, [esp+8]
        mov     [ecx], eax

        mov     [eax].SpinLock, ecx
        mov     ecx, [esp+4]
        mov     [eax].InitAddr, ecx

        mov     _KiSpinLockLock, 0
        popfd

        stdRET    _KeInitializeSpinLock

isl_overflow:
    ; Just use non-tracing locks from now on
        mov     eax, [esp+4]
        mov     dword ptr [eax], LOCK_NOTTRACED
        popfd
        stdRET    _KeInitializeSpinLock

stdENDP _KeInitializeSpinLock

;++
;   VOID
;   SpinLockLazyInit (
;     IN PKSPIN_LOCK SpinLock,
;   )
;
;  Routine Description:
;
;     Used internaly to initialize a spinlock which is being used without
;     first being initialized   (bad! bad!)
;
;--

cPublicProc SpinLockLazyInit,1
        push    eax
        mov     eax, [esp+8]        ; Get SpinLock addr
        test    dword ptr [eax], SYSTEM_ADDR
        jnz     slz_10

        push    ecx
        push    edx
        inc     _KiSpinLockBogus
        stdCall _KeInitializeSpinLock, <eax>
        pop     edx
        pop     ecx

        mov     eax, [esp+8]        ; Get SpinLock addr
        mov     eax, [eax]
        or      [eax].LockFlags, LOCK_LAZYINIT
        pop     eax
        stdRet  SpinLockLazyInit

slz_10:
        stdCall _KeBugCheckEx,<SPIN_LOCK_INIT_FAILURE,eax,0,0,0>

stdENDP SpinLockLazyInit

;++
;   VOID
;   SpinLockInit (VOID)
;
cPublicProc SpinLockInit,0
        pushad
        pushf
        cli

        mov     ecx, MAXSPINLOCKS-1
        mov     eax, offset FLAT:_KiSpinLockArray
        xor     edx, edx

@@:     mov     [eax].NoAcquires, edx
        mov     [eax].NoCollides, edx
        mov     [eax].TotalSpinHigh, edx
        mov     [eax].TotalSpinLow, edx
        mov     [eax].HighestSpin, edx

        add     eax, size s_SpinLock
        dec     ecx
        jnz     short @b

        popf
        popad
@@:     int 3
        jmp     short @b

stdENDP SpinLockInit



;++
;
;  VOID
;  KeFreeSpinLock (
;       )
;
;  Routine Description:
;       Used in instrumentation build to allow spinlocks to be
;       de-allocated if needed.
;
;--

cPublicProc _KeFreeSpinLock,1
        pushfd
        cli
@@: lock bts    _KiSpinLockLock, 0
        jc      short @b

        mov     eax, [esp+8]
        mov     edx, [eax]
        test    edx, SYSTEM_ADDR
        jz      short @f

        mov     dword ptr [eax], 0

;
; Acculate old SpinLock's totals to misc bucket
;
        mov     eax, [edx].NoAcquires
        add     _KiSpinLockArray.NoAcquires, eax

        mov     eax, [edx].NoCollides
        add     _KiSpinLockArray.NoCollides, eax

        mov     eax, [edx].TotalSpinLow
        add     _KiSpinLockArray.TotalSpinLow, eax
        mov     eax, [edx].TotalSpinHigh
        adc     _KiSpinLockArray.TotalSpinLow, eax

        mov     eax, [edx].HighestSpin
        cmp     _KiSpinLockArray.HighestSpin, eax
        jnc     @f
        mov     _KiSpinLockArray.HighestSpin, eax
@@:
        push    edi
        mov     edi, edx
        mov     ecx, size s_SpinLock / 4
        xor     eax, eax
        rep     stosd
        pop     edi

        mov     ecx, _KiSpinLockFreeList
        mov     [edx].InitAddr, ecx
        mov     _KiSpinLockFreeList, edx

@@:
        mov     _KiSpinLockLock, 0
        popfd
        stdRET    _KeFreeSpinLock
stdENDP _KeFreeSpinLock

;++
;
;  VOID
;  KeInializeSpinLock2 (
;     IN PKSPIN_LOCK SpinLock,
;
;  Routine Description:
;
;     This function initializes a non-tracing SpinLock.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     None.
;
;--
cPublicProc _KeInitializeSpinLock2,1
        mov     eax, [esp+4]
        mov     dword ptr [eax], LOCK_NOTTRACED
        stdRET  _KeInitializeSpinLock2
stdENDP _KeInitializeSpinLock2,1


        PAGE
        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  VOID
;  KeAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     OUT PKIRQL     OldIrql
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and then acquires a the
;     kernel spin lock.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;     OldIrql  (TOS+8) - pointer to place old irql
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicProc _KeAcquireSpinLock  ,2
        sub     esp, 4              ; Make room for OldIrql
        stdCall   _KeRaiseIrql, <DISPATCH_LEVEL, esp>

sl00:   mov     eax,[esp+8]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]           ; (eax) -> Spin structure
        test    eax, SYSTEM_ADDR
        jz      short sl_bogus


        xor     ecx, ecx            ; Initialize spin count
        xor     edx, edx            ; Initialize collide count

;
;   Attempt to obtain the lock
;

sl10:   lock    bts [eax].LockValue, 0
        jc      short sl30          ; If lock is busy, go wait

;
; SpinLock is now owned
;
        inc     [eax].NoAcquires    ; accumulate statistic
        add     [eax].NoCollides, edx
   lock add     [eax].TotalSpinLow, ecx
        adc     [eax].TotalSpinHigh, 0

        cmp     [eax].HighestSpin, ecx
        jc      short sl20

sl15:   mov     eax, [esp+12]       ; pOldIrql
        pop     ecx                 ; OldIrql
        mov     byte ptr [eax], cl

        stdRet  _KeAcquireSpinLock

align 4
sl20:   mov     [eax].HighestSpin, ecx  ; set new highest spin mark
        jmp     short sl15

sl30:   inc     edx                 ; one more collide

;
; SpinLoop is kept small in order to get counts based on PcStallCount
;
align 4
sl50:   inc     ecx                 ; one more spin
        test    [eax].LockValue, 1  ; is it free?
        jnz     short sl50          ; no, loop

        jmp     short sl10          ; Go try again

;
; SpinLock was bogus - it's either a lock being used without being
; initialized, or it's a lock we don't care to trace
;

sl_bogus:
        mov     eax, [esp+8]
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short sl_lazyinit

sl60:   lock    bts dword ptr [eax], 0  ; attempt to acquire non-traced lock
        jnc     short sl15              ; if got it, return

        xor     ecx, ecx
sl65:   inc     ecx
        test    dword ptr [eax], 1      ; wait for lock to be un-busy
        jnz     short sl65

   lock add     _KiSpinLockArray.TotalSpinLow, ecx
        adc     _KiSpinLockArray.TotalSpinHigh, 0
        jmp     short sl60

;
; Someone is using a lock which was not properly initialized, go do it now
;

sl_lazyinit:
        stdCall SpinLockLazyInit,<eax>
        jmp     short sl00

stdENDP _KeAcquireSpinLock


        PAGE
        SUBTTL "Release Kernel Spin Lock"
;++
;
;  VOID
;  KeReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an executive spin lock.
;     NewIrql  (TOS+8) - New irql value to set
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicProc _KeReleaseSpinLock  ,2

        mov     eax,[esp+4]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]           ; SpinLock structure
        test    eax, SYSTEM_ADDR
        jz      short rsl_bogus

        mov     [eax].LockValue, 0  ; clear busy bit

rsl10:  pop     eax                 ; (eax) = ret. address
        mov     [esp],eax           ; set stack so we can jump directly
        jmp     _KeLowerIrql@4      ; to KeLowerIrql

rsl_bogus:
        mov     eax, [esp+4]
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short rsl_lazyinit

        btr     dword ptr [eax], 0  ; clear lock bit on non-tracing lock
        jmp     short rsl10

rsl_lazyinit:                       ; go initialize lock now
        stdCall SpinLockLazyInit, <eax>
        jmp     short _KeReleaseSpinLock
stdENDP _KeReleaseSpinLock

        PAGE
        SUBTTL "Ki Acquire Kernel Spin Lock"

;++
;
;  VOID
;  KiAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function acquires a kernel spin lock.
;
;     N.B. This function assumes that the current IRQL is set properly.
;        It neither raises nor lowers IRQL.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicProc _KiAcquireSpinLock  ,1
        mov     eax,[esp+4]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]           ; (eax) -> Spin structure
        test    eax, SYSTEM_ADDR
        jz      short asl_bogus

        xor     ecx, ecx            ; Initialize spin count
        xor     edx, edx            ; Initialize collide count

;
;   Attempt to obtain the lock
;

asl10:  lock    bts [eax].LockValue, 0
        jc      short asl40         ; If lock is busy, go wait

;
; SpinLock is owned
;
        inc     [eax].NoAcquires    ; accumulate statistics
        add     [eax].NoCollides, edx
   lock add     [eax].TotalSpinLow, ecx
        adc     [eax].TotalSpinHigh, 0

        cmp     [eax].HighestSpin, ecx
        jc      short asl20

        stdRet  _KiAcquireSpinLock

align 4
asl20:  mov     [eax].HighestSpin, ecx    ; set new highest spin mark
asl30:  stdRet  _KiAcquireSpinLock

asl40:  inc     edx                 ; one more collide

;
; SpinLoop is kept small in order to get counts based on PcStallCount
;
align 4
asl50:  inc     ecx                 ; one more spin
        test    [eax].LockValue, 1  ; is it free?
        jnz     short asl50         ; no, loop
        jmp     short asl10         ; Go try again

;
; This is a non-initialized lock.
;
asl_bogus:
        mov     eax, [esp+4]
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      asl_lazyinit

asl60:  lock    bts dword ptr [eax], 0  ; attempt to acquire non-traced lock
        jnc     short asl30             ; if got it, return

        xor     ecx, ecx
asl65:  inc     ecx
        test    dword ptr [eax], 1      ; wait for lock to be un-busy
        jnz     short asl65

   lock add     _KiSpinLockArray.TotalSpinLow, eax
        adc     _KiSpinLockArray.TotalSpinHigh, 0
        jmp     short asl60

asl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        jmp     short _KiAcquireSpinLock
stdENDP _KiAcquireSpinLock

        PAGE
        SUBTTL "Ki Release Kernel Spin Lock"
;++
;
;  VOID
;  KiReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock.
;
;     N.B. This function assumes that the current IRQL is set properly.
;        It neither raises nor lowers IRQL.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an executive spin lock.
;
;  Return Value:
;
;     None.
;
;--
align 16
cPublicProc _KiReleaseSpinLock  ,1
        mov     eax,[esp+4]             ; (eax) -> ptr -> spinlock
        mov     eax,[eax]
        test    eax, SYSTEM_ADDR
        jz      short irl_bogus

        mov     [eax].LockValue, 0
        stdRET  _KiReleaseSpinLock

irl_bogus:
        mov     eax,[esp+4]
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short irl_lazyinit

        btr     dword ptr [eax], 0      ; clear busy bit on non-traced lock
        stdRET  _KiReleaseSpinLock

irl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        stdRet  _KiReleaseSpinLock
stdENDP _KiReleaseSpinLock

        PAGE
        SUBTTL "Try to acquire Kernel Spin Lock"
;++
;
;  BOOLEAN
;  KeTryToAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     OUT PKIRQL     OldIrql
;     )
;
;  Routine Description:
;
;     This function attempts acquires a kernel spin lock.  If the
;     spinlock is busy, it is not acquire and FALSE is returned.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;     OldIrql  (TOS+8) = Location to store old irql
;
;  Return Value:
;     TRUE  - Spinlock was acquired & irql was raise
;     FALSE - SpinLock was not acquired - irql is unchanged.
;
;--

align dword
cPublicProc _KeTryToAcquireSpinLock  ,2

;
; This function is currently only used by the debugger, so we don't
; keep stats on it
;

        mov     eax,[esp+4]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]
        test    eax, SYSTEM_ADDR
        jz      short tts_bogus


;
; First check the spinlock without asserting a lock
;

        test    [eax].LockValue, 1
        jnz     short ttsl10

;
; Spinlock looks free raise irql & try to acquire it
;

        mov     eax, [esp+8]        ; (eax) -> ptr to OldIrql

;
; raise to dispatch_level
;

        stdCall   _KeRaiseIrql, <DISPATCH_LEVEL, eax>

        mov     eax,[esp+4]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]
        lock bts  [eax].LockValue, 0
        jc      short ttsl20

        mov     eax, 1              ; spinlock was acquired, return TRUE
        stdRET  _KeTryToAcquireSpinLock

ttsl10:
        xor     eax, eax            ; return FALSE
        stdRET  _KeTryToAcquireSpinLock

ttsl20:
        mov     eax, [esp+8]        ; spinlock was busy, restore irql
        stdCall _KeLowerIrql, <dword ptr [eax]>

        xor     eax, eax            ; return FALSE
        stdRET  _KeTryToAcquireSpinLock

tts_bogus:
        mov     eax,[esp+4]
        test    dword ptr [eax], LOCK_NOTTRACED
        jnz     short tts_bogus2

        stdCall SpinLockLazyInit, <eax>
        jmp     short _KeTryToAcquireSpinLock

tts_bogus2:
        stdCall _KeBugCheckEx,<SPIN_LOCK_INIT_FAILURE,eax,0,0,0>   ; Not supported for now

stdENDP _KeTryToAcquireSpinLock

        PAGE
        SUBTTL "Ki Try to acquire Kernel Spin Lock"
;++
;
;  BOOLEAN
;  KiTryToAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function attempts acquires a kernel spin lock.  If the
;     spinlock is busy, it is not acquire and FALSE is returned.
;
;  Arguments:
;
;     SpinLock (TOS+4) - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;     TRUE  - Spinlock was acquired
;     FALSE - SpinLock was not acquired
;
;--
align dword
cPublicProc _KiTryToAcquireSpinLock  ,1
;
; This function is currently only used by the debugger, so we don't
; keep stats on it
;

        mov     eax,[esp+4]         ; (eax) -> ptr -> spinlock
        mov     eax,[eax]
        test    eax, SYSTEM_ADDR
        jz      short atsl_bogus


;
; First check the spinlock without asserting a lock
;

        test    [eax].LockValue, 1
        jnz     short atsl10

;
        lock bts  [eax].LockValue, 0
        jc      short atsl10

atsl05:
        mov     eax, 1              ; spinlock was acquired, return TRUE
        stdRET  _KiTryToAcquireSpinLock

atsl10:
        xor     eax, eax            ; return FALSE
        stdRET  _KiTryToAcquireSpinLock

atsl_bogus:
        mov     eax,[esp+4]
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short atsl_lazyinit

        test    dword ptr [eax], 1
        jnz     short atsl10

        lock    bts dword ptr [eax], 0
        jnc     short atsl05
        jmp     short atsl10

atsl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        jmp     short _KiTryToAcquireSpinLock

stdENDP _KiTryToAcquireSpinLock


;++
;
; KiInst_AcquireSpinLock
;
;  Routine Description:
;     The NT_INST version of the macro ACQUIRE_SPINLOCK.
;     The macro thunks to this function so stats can be kept
;
;  Arguments:
;   (eax) - SpinLock to acquire
;
;  Return value:
;    CY - SpinLock was not acquired
;    NC - SpinLock was acquired
;
;--
align dword
cPublicProc KiInst_AcquireSpinLock, 0
        test    dword ptr [eax], SYSTEM_ADDR
        jz      short iasl_bogus

        mov     eax, [eax]          ; Get SpinLock structure
   lock bts [eax].LockValue, 0
        jc      short iasl_10       ; was busy, return CY

        inc     [eax].NoAcquires
        mov     eax, [eax].SpinLock
        stdRET  KiInst_AcquireSpinLock

iasl_10:
        inc     [eax].NoCollides
        mov     eax, [eax].SpinLock
        stdRET  KiInst_AcquireSpinLock

iasl_bogus:
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short iasl_lazyinit

   lock bts     dword ptr [eax], 0
        stdRET  KiInst_AcquireSpinLock

iasl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        jmp     short KiInst_AcquireSpinLock

stdENDP KiInst_AcquireSpinLock


;++
;
; KiInst_SpinOnSpinLock
;
;  Routine Description:
;     The NT_INST version of the macro SPIN_ON_SPINLOCK.
;     The macro thunks to this function so stats can be kept
;
;  Arguments:
;   (eax) - SpinLock to acquire
;
;  Return value:
;    Returns when spinlock appears to be free
;
;--
align dword
cPublicProc KiInst_SpinOnSpinLock, 0
        test    dword ptr [eax], SYSTEM_ADDR
        jz      short issl_bogus

        push    ecx
        mov     eax, [eax]          ; Get SpinLock structure
        xor     ecx, ecx            ; initialize spincount

align 4
issl10: inc     ecx                 ; one more spin
        test    [eax].LockValue, 1  ; is it free?
        jnz     short issl10        ; no, loop

   lock add     [eax].TotalSpinLow, ecx    ; accumulate spin
        adc     [eax].TotalSpinHigh, 0

        cmp     [eax].HighestSpin, ecx
        jc      short issl20

        mov     eax, [eax].SpinLock     ; restore eax
        pop     ecx
        stdRet  KiInst_SpinOnSpinLock

issl20:
        mov     [eax].HighestSpin, ecx  ; set new highest spin mark
        mov     eax, [eax].SpinLock     ; restore eax
        pop     ecx
        stdRet  KiInst_SpinOnSpinLock

issl_bogus:
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short issl_lazyinit

        push    ecx
        xor     ecx, ecx

issl30: inc     ecx
        test    dword ptr [eax], 1
        jnz     short issl30

   lock add     _KiSpinLockArray.TotalSpinLow, ecx
   lock adc     _KiSpinLockArray.TotalSpinHigh, 0
        pop     ecx

        stdRet  KiInst_SpinOnSpinLock

issl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        stdRet  KiInst_SpinOnSpinLock


stdENDP KiInst_SpinOnSpinLock


;++
;
; KiInst_ReleaseSpinLock
;
;  Routine Description:
;     The NT_INST version of the macro ACQUIRE_SPINLOCK.
;     The macro thunks to this function so stats can be kept
;
;  Arguments:
;   (eax) - SpinLock to acquire
;
;  Return value:
;
;--
align dword
cPublicProc KiInst_ReleaseSpinLock, 0
        test    dword ptr [eax], SYSTEM_ADDR
        jz      short rssl_bogus

        mov     eax, [eax]              ; Get SpinLock structure
        mov     [eax].LockValue, 0      ; Free it
        mov     eax, [eax].SpinLock     ; Restore eax
        stdRET  KiInst_ReleaseSpinLock

rssl_bogus:
        test    dword ptr [eax], LOCK_NOTTRACED
        jz      short rssl_lazyinit

        btr     dword ptr [eax], 0

rssl_lazyinit:
        stdCall SpinLockLazyInit, <eax>
        stdRET  KiInst_ReleaseSpinLock
stdENDP KiInst_ReleaseSpinLock

_TEXT$00   ends

endif

        end
