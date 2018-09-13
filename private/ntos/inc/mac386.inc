;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    mac386.inc - 386 machine specific assembler macros
;
; Abstract:
;
;   This module contains 386 machine specific (assembler) macros
;   applicable to code outside the kernel.  Note that
;   ACQUIRE_SPINLOCK_DIRECT assumes the PCR is handy, so it won't
;   work in user mode (with debugging turned on.)
;
; Author:
;
;   Bryan Willman (bryanwi)  1 Aug 90
;

;++
;
;  YIELD
;
;  Macro Description:
;
;     This macro implements the yield instruction
;--

YIELD macro
ifndef NT_UP
    db  0f3h
    db  090h
endif
endm


if NT_INST
else

;++
;
;  ACQUIRE_SPINLOCK LockAddress, SpinLabel
;
;  Macro Description:
;
;     This macro acquires a kernel spin lock.
;
;     N.B. This macro assumes that the current IRQL is set properly.
;        It neither raises nor lowers IRQL.
;
;  Arguments:
;
;     (KSPIN_LOCK) LockAddress - address of SpinLock value
;     SpinLabel   - if acquire spinlock fail, the label to perform the
;                   spin checking.  It could be simply a "label" or
;                   "short label" which means the label is within 128
;                   bytes in distant.
;
;     NoChecking - Not blank, if no debugging code should be generated.
;--

ACQUIRE_SPINLOCK macro   LockAddress, SpinLabel, NoChecking

.errb <LockAddress>
.errb <SpinLabel>

ifndef NT_UP

;
; Attempt to assert the lock
;

        lock bts dword ptr [LockAddress], 0     ; test and set the spinlock
        jc      SpinLabel               ; spinlock owned, go SpinLabe

if    DBG
ifb   <NoChecking>
        push    edi                     ; save edi
        mov     edi,fs:PcPrcb
        mov     edi, [edi].PbCurrentThread
        or      edi, 1                  ; spinlock owned
        mov     [LockAddress], edi      ; remember current thread
        pop     edi                     ; restore edi
endif ; NoChecking
endif ; DBG
endif ; NT_UP

endm

;++
;
;  SPIN_ON_SPINLOCK   LockAddress, AcquireLabel
;
;  Macro Description:
;
;     This macro spins on a kernel spin lock.
;
;     N.B. This macro assumes that the current IRQL is set properly.
;        It neither raises nor lowers IRQL.
;
;  Arguments:
;
;     (KSPIN_LOCK) LockAddress - address of a SpinLock value
;
;     SpinLabel    - if the test on cleared spinlock sucess, the label
;                    to assert the spin lock.  It could be simply a
;                    "label" or  "short label" which means the label is
;                    within 128 bytes in distance.
;
;     NoChecking - Not blank, if no debugging code should be generated.
;--

SPIN_ON_SPINLOCK macro   LockAddress, AcquireLabel, NoChecking, PollDebugger, NoTimeout
local   a,flag                          ; define a local label

.errb <LockAddress>
.errb <AcquireLabel>

ifndef NT_UP
if    DBG

EXTRNP  Kii386SpinOnSpinLock,2
        flag = 0

ifb   <NoChecking>
        flag = flag + 1
endif

ifnb  <Polldebugger>
        flag = flag + 2
endif

ifb   <NoTimeout>
        flag = flag + 4
endif
        stdCall Kii386SpinOnSpinLock,<LockAddress,flag>
        jmp     AcquireLabel

else    ; DBG

;
; Non-Debug version
;

a:      test    dword ptr [LockAddress], 1 ; Was spinlock cleared?
        jz      AcquireLabel            ; Yes, go get it
        YIELD
        jmp     short a

endif   ; DBG
endif   ; NT_UP

endm


;++
;
;  TEST_SPINLOCK   LockAddress, BusyLabel
;
;  Macro Description:
;
;     This macro tests a kernel spin lock to see if it's busy.
;     If it's not busy, ACQUIRE_SPINLOCK still needs to be called
;     to obtain the spinlock in a locked manner.
;
;  Arguments:
;
;     (KSPIN_LOCK) LockAddress - address of a SpinLock value


TEST_SPINLOCK macro   LockAddress, BusyLabel
        test    dword ptr [LockAddress], 1 ; spinlock clear?
        jnz     BusyLabel                  ; No, then busy
endm




;++
;
;  RELEASE_SPINLOCK  LockAddress
;
;  Macro Description:
;
;     This macro releases a kernel spin lock.
;
;     N.B. This macro assumes that the current IRQL is set properly.
;        It neither raises nor lowers IRQL.
;
;  Arguments:
;
;     (KSPIN_LOCK) LockAddress - Supplies an address to a spin lock value
;     NoChecking - Not blank, if no debugging code should be generated.
;--

RELEASE_SPINLOCK macro   LockAddress, NoChecking
local a
.errb <LockAddress>
ifndef NT_UP
if    DBG
ifb   <NoChecking>
EXTRNP  _KeBugCheckEx,5

        push    edi                     ; save edi
        mov     edi,fs:PcPrcb
        mov     edi,[edi].PbCurrentThread
        or      edi, 1                  ; assume current thread owns the lock
        cmp     edi, [LockAddress]      ; Does current thread own the lock?
        pop     edi                     ; restore edi
        jz      short a                 ; if z, yes, goto a and release lock
        stdCall _KeBugCheckEx,<SPIN_LOCK_NOT_OWNED,LockAddress,0,0,0>
a:
endif
        mov     dword ptr [LockAddress], 0
else
        mov     byte ptr [LockAddress], 0

endif   ; DBG
endif   ; NT_UP
endm


endif
if NT_INST

;
; These are the instrumentation version of the above functions.
; internal use only
;

ACQUIRE_SPINLOCK macro   LockAddress, SpinLabel, NoChecking
EXTRNP  KiInst_AcquireSpinLock,0
ifidni  <&LockAddress>, <eax>
        stdCall KiInst_AcquireSpinLock
else
        push    eax
        mov     eax, LockAddress
        stdCall KiInst_AcquireSpinLock
        pop     eax
endif
        jc      SpinLabel
endm

SPIN_ON_SPINLOCK macro   LockAddress, AcquireLabel, NoChecking, PollDebugger
EXTRNP  KiInst_SpinOnSpinLock,0
ifidni  <&LockAddress>, <eax>
        stdCall KiInst_SpinOnSpinLock
else
        push    eax
        mov     eax, LockAddress
        stdCall KiInst_SpinOnSpinLock
        pop     eax
endif
        jmp     AcquireLabel
endm

TEST_SPINLOCK macro   LockAddress, BusyLabel
EXTRNP  KiInst_TestSpinLock,0
ifidni  <&LockAddress>, <eax>
        stdCall KiInst_TestSpinLock
else
        push    eax
        mov     eax, LockAddress
        stdCall KiInst_TestSpinLock
        pop     eax
endif
        jnc     AcquireLabel
endm

RELEASE_SPINLOCK macro   LockAddress, NoChecking
EXTRNP  KiInst_ReleaseSpinLock,0
ifidni  <&LockAddress>, <eax>
        stdCall KiInst_ReleaseSpinLock
else
        push    eax
        mov     eax, LockAddress
        stdCall KiInst_ReleaseSpinLock
        pop     eax
endif
endm

endif

