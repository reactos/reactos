if NT_INST
else
        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements the routines for acquiring and releasing
;     spin locks.
;
;  Author:
;
;     Bryan Willman (bryanwi) 13 Dec 89
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;   Ken Reneris (kenr) 22-Jan-1991
;       Removed KeAcquireSpinLock macros, and made functions
;--

        PAGE

.486p

include ks386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc

        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeGetCurrentIrql,0,IMPORT
        EXTRNP  _KeBugCheckEx,5


_TEXT$00   SEGMENT  PARA PUBLIC 'CODE'
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
cPublicFpo 1,0
        mov     eax, dword ptr [esp+4]
        mov     dword ptr [eax], 0
        stdRET    _KeInitializeSpinLock
stdENDP _KeInitializeSpinLock



        PAGE
        SUBTTL "Ke Acquire Spin Lock At DPC Level"

;++
;
;  VOID
;  KefAcquireSpinLockAtDpcLevel (
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
;     (ecx) SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicFastCall KefAcquireSpinLockAtDpcLevel, 1
cPublicFpo 0, 0
if DBG
        push    ecx
        stdCall _KeGetCurrentIrql
        pop     ecx

        cmp     al, DISPATCH_LEVEL
        jne     short asld50
endif

ifdef NT_UP
        fstRET    KefAcquireSpinLockAtDpcLevel
else
;
;   Attempt to assert the lock
;

asld10: ACQUIRE_SPINLOCK    ecx,<short asld20>
        fstRET    KefAcquireSpinLockAtDpcLevel

;
;   Lock is owned, spin till it looks free, then go get it again.
;

align 4
asld20: SPIN_ON_SPINLOCK    ecx,<short asld10>

endif

if DBG
asld50: stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,eax,0,0>
        int       3                 ; help debugger backtrace.
endif

fstENDP KefAcquireSpinLockAtDpcLevel


;++
;
;  VOID
;  KeAcquireSpinLockAtDpcLevel (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;   Thunk for standard call callers
;
;--

cPublicProc _KeAcquireSpinLockAtDpcLevel, 1
cPublicFpo 1,0

ifndef NT_UP
        mov     ecx,[esp+4]         ; SpinLock

aslc10: ACQUIRE_SPINLOCK    ecx,<short aslc20>
        stdRET    _KeAcquireSpinLockAtDpcLevel

aslc20: SPIN_ON_SPINLOCK    ecx,<short aslc10>
endif
        stdRET    _KeAcquireSpinLockAtDpcLevel
stdENDP _KeAcquireSpinLockAtDpcLevel


        PAGE
        SUBTTL "Ke Release Spin Lock From Dpc Level"
;++
;
;  VOID
;  KefReleaseSpinLockFromDpcLevel (
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
;     (ecx) SpinLock - Supplies a pointer to an executive spin lock.
;
;  Return Value:
;
;     None.
;
;--
align 16
cPublicFastCall KefReleaseSpinLockFromDpcLevel  ,1
cPublicFpo 0,0
ifndef NT_UP
        RELEASE_SPINLOCK    ecx
endif
        fstRET    KefReleaseSpinLockFromDpcLevel

fstENDP KefReleaseSpinLockFromDpcLevel

;++
;
;  VOID
;  KeReleaseSpinLockFromDpcLevel (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;   Thunk for standard call callers
;
;--

cPublicProc _KeReleaseSpinLockFromDpcLevel, 1
cPublicFpo 1,0
ifndef NT_UP
        mov     ecx, [esp+4]            ; (ecx) = SpinLock
        RELEASE_SPINLOCK    ecx
endif
        stdRET    _KeReleaseSpinLockFromDpcLevel
stdENDP _KeReleaseSpinLockFromDpcLevel



        PAGE
        SUBTTL "Ki Acquire Kernel Spin Lock"

;++
;
;  VOID
;  FASTCALL
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
;     (ecx) SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicFastCall KiAcquireSpinLock  ,1
cPublicFpo 0,0
ifndef NT_UP

;
;   Attempt to assert the lock
;

asl10:  ACQUIRE_SPINLOCK    ecx,<short asl20>
        fstRET    KiAcquireSpinLock

;
;   Lock is owned, spin till it looks free, then go get it again.
;

align 4
asl20:  SPIN_ON_SPINLOCK    ecx,<short asl10>

else
        fstRET    KiAcquireSpinLock
endif

fstENDP KiAcquireSpinLock

        PAGE
        SUBTTL "Ki Release Kernel Spin Lock"
;++
;
;  VOID
;  FASTCALL
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
;     (ecx) SpinLock - Supplies a pointer to an executive spin lock.
;
;  Return Value:
;
;     None.
;
;--
align 16
cPublicFastCall KiReleaseSpinLock  ,1
cPublicFpo 0,0
ifndef NT_UP

        RELEASE_SPINLOCK    ecx

endif
        fstRET    KiReleaseSpinLock

fstENDP KiReleaseSpinLock

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
cPublicFpo 2,0

ifdef NT_UP
; UP Version of KeTryToAcquireSpinLock

        mov     ecx, DISPATCH_LEVEL
        fstCall KfRaiseIrql

        mov     ecx, [esp+8]        ; (ecx) -> ptr to OldIrql
        mov     [ecx], al           ; save OldIrql

        mov     eax, 1              ; Return TRUE
        stdRET    _KeTryToAcquireSpinLock

else
; MP Version of KeTryToAcquireSpinLock

        mov     edx,[esp+4]         ; (edx) -> spinlock

;
; First check the spinlock without asserting a lock
;

        TEST_SPINLOCK       edx,<short ttsl10>

;
; Spinlock looks free raise irql & try to acquire it
;

;
; raise to dispatch_level
;

        mov     ecx, DISPATCH_LEVEL
        fstCall KfRaiseIrql

        mov     edx, [esp+4]        ; (edx) -> spinlock
        mov     ecx, [esp+8]        ; (ecx) = Return OldIrql

        ACQUIRE_SPINLOCK    edx,<short ttsl20>

        mov     [ecx], al           ; save OldIrql
        mov     eax, 1              ; spinlock was acquired, return TRUE

        stdRET    _KeTryToAcquireSpinLock

ttsl10: xor     eax, eax            ; return FALSE
        YIELD
        stdRET    _KeTryToAcquireSpinLock

ttsl20:
        YIELD
        mov     cl, al              ; (cl) = OldIrql
        fstCall KfLowerIrql         ; spinlock was busy, restore irql
        xor     eax, eax            ; return FALSE
        stdRET    _KeTryToAcquireSpinLock
endif

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
cPublicFpo 1,0

ifndef NT_UP
        mov     eax,[esp+4]         ; (eax) -> spinlock

;
; First check the spinlock without asserting a lock
;

        TEST_SPINLOCK       eax,<short atsl20>

;
; Spinlock looks free try to acquire it
;

        ACQUIRE_SPINLOCK    eax,<short atsl20>
endif
        mov     eax, 1              ; spinlock was acquired, return TRUE
        stdRET    _KiTryToAcquireSpinLock

ifndef NT_UP
atsl20:
        YIELD
        xor     eax, eax            ; return FALSE
        stdRET    _KiTryToAcquireSpinLock
endif
stdENDP _KiTryToAcquireSpinLock

;++
;
;  BOOLEAN
;  KeTestSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function tests a kernel spin lock.  If the spinlock is
;     busy, FALSE is returned.  If not, TRUE is returned.  The spinlock
;     is never acquired.  This is provided to allow code to spin at low
;     IRQL, only raising the IRQL when there is a reasonable hope of
;     acquiring the lock.
;
;  Arguments:
;
;     SpinLock (ecx) - Supplies a pointer to a kernel spin lock.
;
;  Return Value:
;     TRUE  - Spinlock appears available
;     FALSE - SpinLock is busy
;
;--

cPublicFastCall KeTestSpinLock  ,1
        TEST_SPINLOCK       ecx,<short tso10>
        mov       eax, 1
        fstRET    KeTestSpinLock

tso10:  YIELD
        xor       eax, eax
        fstRET    KeTestSpinLock

fstENDP KeTestSpinLock

        page    ,132
        subttl  "Acquire Queued SpinLock"

;++
;
; VOID
; KiAcquireQueuedSpinLock (
;     IN PKSPIN_LOCK_QUEUE QueuedLock
;     )
;
; Routine Description:
;
;    This function acquires the specified queued spinlock.
;    No change to IRQL is made, IRQL is not returned.  It is
;    expected IRQL is sufficient to avoid context switch.
;
;    Unlike the equivalent Ke versions of these routines,
;    the argument to this routine is the address of the 
;    lock queue entry (for the lock to be acquired) in the
;    PRCB rather than the LockQueueNumber.  This saves us
;    a couple of instructions as the address can be calculated
;    at compile time.
;
;    NOTE: This code may be modified for use during textmode
;    setup if this is an MP kernel running with a UP HAL.
;
; Arguments:
;
;    LockQueueEntry (ecx) - Supplies the address of the queued
;                           spinlock intry in this processor's
;                           PRCB.
;
; Return Value:
;
;    None.
;
;    N.B. ecx is preserved, assembly code callers can take advantage
;    of this by avoiding setting up ecx for the call to release if
;    the caller can preserve the lock that long.
;
;--

        ; compile time assert sizeof(KSPIN_LOCK_QUEUE) == 8

.errnz  (LOCK_QUEUE_HEADER_SIZE - 8)

align 16
cPublicFastCall KiAcquireQueuedSpinLock,1
cPublicFpo 0,0

ifndef NT_UP

        ; Get address of the actual lock.

        mov     edx, [ecx].LqLock
        mov     eax, ecx                        ; save Lock Queue entry address

        ; Exchange the value of the lock with the address of this
        ; Lock Queue entry.

        xchg    [edx], eax

        cmp     eax, 0                          ; check if lock is held
        jnz     short @f                        ; jiff held

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      edx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [ecx].LqLock, edx

        ; lock has been acquired, return.

aqsl20: 

endif

        fstRET  KiAcquireQueuedSpinLock

ifndef NT_UP

@@:  

if DBG

        ; make sure it isn't already held by THIS processor.

        test    edx, LOCK_QUEUE_OWNER
        jz      short @f

        ; KeBugCheckEx(SPIN_LOCK_ALREADY_OWNED,
        ;             actual lock address,
        ;             my context,
        ;             previous acquirer,
        ;             2);

        stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED,edx,ecx,eax,2>
@@:

endif
        ; The lock is already held by another processor.  Set the wait
        ; bit in this processor's Lock Queue entry, then set the next
        ; field in the Lock Queue entry of the last processor to attempt
        ; to acquire the lock (this is the address returned by the xchg
        ; above) to point to THIS processor's lock queue entry.

        or      edx, LOCK_QUEUE_WAIT            ; set lock bit
        mov     [ecx].LqLock, edx

        mov     [eax].LqNext, ecx               ; set previous acquirer's
                                                ; next field.

        ; Wait.
@@: 
        test    [ecx].LqLock, LOCK_QUEUE_WAIT   ; check if still waiting
        jz      short aqsl20                    ; jif lock acquired
        YIELD                                   ; fire avoidance.
        jmp     short @b                        ; else, continue waiting

endif

fstENDP KiAcquireQueuedSpinLock


        page    ,132
        subttl  "Release Queued SpinLock"

;++
;
; VOID
; KiReleaseQueuedSpinLock (
;     IN PKSPIN_LOCK_QUEUE QueuedLock
;     )
;
; Routine Description:
;
;    This function releases a queued spinlock.
;    No change to IRQL is made, IRQL is not returned.  It is
;    expected IRQL is sufficient to avoid context switch.
;
;    NOTE: This code may be modified for use during textmode
;    setup if this is an MP kernel running with a UP HAL.
;
; Arguments:
;
;    LockQueueEntry (ecx) - Supplies the address of the queued
;                           spinlock intry in this processor's
;                           PRCB.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KiReleaseQueuedSpinLock,1
cPublicFpo 0,0

.errnz  (LOCK_QUEUE_OWNER - 2)           ; error if not bit 1 for btr 

ifndef NT_UP


        mov     eax, ecx                        ; need in eax for cmpxchg
        mov     edx, [ecx].LqNext
        mov     ecx, [ecx].LqLock


        ; Quick check: If Lock Queue entry's Next field is not NULL,
        ; there is another waiter.  Don't bother with ANY atomic ops
        ; in this case.
        ;
        ; N.B. Careful ordering, the test will clear the CF bit and set
        ; the ZF bit appropriately if the Next Field (in EDX) is zero.
        ; The BTR will set the CF bit to the previous value of the owner
        ; bit.

        test    edx, edx

        ; Clear the "I am owner" field in the Lock entry.

        btr     ecx, 1                          ; clear owner bit

if DBG
        jnc     short rqsl90                    ; bugcheck if was not set
                                                ; tests CF
endif

        mov     [eax].LqLock, ecx               ; clear lock bit in queue entry
        jnz     short rqsl40                    ; jif another processor waits
                                                ; tests ZF

        xor     edx, edx                        ; new lock owner will be NULL
        push    eax                             ; save &PRCB->LockQueue[Number]

        ; Use compare exchange to attempt to clear the actual lock.
        ; If there are still no processors waiting for the lock when
        ; the compare exchange happens, the old contents of the lock
        ; should be the address of this lock entry (eax).

        lock cmpxchg [ecx], edx                 ; store 0 if no waiters
        pop     eax                             ; restore lock queue address
        jnz     short rqsl60                    ; jif store failed

        ; The lock has been released.  Return to caller.

endif

        fstRET  KiReleaseQueuedSpinLock

ifndef NT_UP

        ; Another processor is waiting on this lock.   Hand the lock
        ; to that processor by getting the address of its LockQueue
        ; entry, turning ON its owner bit and OFF its wait bit.

rqsl40: xor     [edx].LqLock, (LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT)

        ; Done, the other processor now owns the lock, clear the next
        ; field in my LockQueue entry (to preserve the order for entering
        ; the queue again) and return.

        mov     [eax].LqNext, 0
        fstRET  KiReleaseQueuedSpinLock

        ; We get here if another processor is attempting to acquire
        ; the lock but had not yet updated the next field in this 
        ; processor's Queued Lock Next field.   Wait for the next
        ; field to be updated.

rqsl60: mov     edx, [eax].LqNext
        test    edx, edx                        ; check if still 0
        jnz     short rqsl40                    ; jif Next field now set.
        YIELD                                   ; wait a bit
        jmp     short rqsl60                    ; continue waiting

if DBG

rqsl90:
        stdCall _KeBugCheckEx,<SPIN_LOCK_NOT_OWNED,ecx,eax,0,0>
        int     3                               ; help debugger back trace.

endif

endif

fstENDP KiReleaseQueuedSpinLock

        page    ,132
        subttl  "Try to Acquire Queued SpinLock"

;++
;
; LOGICAL
; KiTryToAcquireQueuedSpinLock (
;     IN  KSPIN_LOCK_QUQUE_NUMBER Number
;     IN PKSPIN_LOCK_QUEUE QueuedLock
;     )
;
; Routine Description:
;
;    This function attempts to acquire the specified queued spinlock.
;    No change to IRQL is made, IRQL is not returned.  It is
;    expected IRQL is sufficient to avoid context switch.
;
;    NOTE: This code may be modified for use during textmode
;    setup if this is an MP kernel running with a UP HAL.
;
; Arguments:
;
;    LockQueueEntry (ecx) - Supplies the address of the queued
;                           spinlock intry in this processor's
;                           PRCB.
;
; Return Value:
;
;    TRUE if the lock was acquired, FALSE otherwise.
;    N.B. ZF is set if FALSE returned, clear otherwise.
;
;--


align 16
cPublicFastCall KiTryToAcquireQueuedSpinLock,1
cPublicFpo 0,0

ifndef NT_UP

        ; Get address of Lock Queue entry

        mov     edx, [ecx].LqLock

        ; Store the Lock Queue entry address in the lock ONLY if the
        ; current lock value is 0.

        xor     eax, eax                        ; old value must be 0
        lock cmpxchg [edx], ecx
        jnz     short taqsl60

        ; Lock has been acquired.

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      edx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [ecx].LqLock, edx

        or      eax, 1                          ; return TRUE

        fstRET  KiTryToAcquireQueuedSpinLock

taqsl60:  

if DBG

        ; make sure it isn't already held by THIS processor.

        test    edx, LOCK_QUEUE_OWNER
        jz      short @f

        stdCall _KeBugCheckEx,<SPIN_LOCK_ALREADY_OWNED, edx, ecx,0,1>
@@:

endif

        ; The lock is already held by another processor.  Indicate
        ; failure to the caller.

        xor     eax, eax                        ; return FALSE
        fstRET  KiTryToAcquireQueuedSpinLock

        ; In the event that this is an MP kernel running with a UP
        ; HAL, the following UP version is copied over the MP version
        ; during kernel initialization.

        public  _KiTryToAcquireQueuedSpinLockUP
_KiTryToAcquireQueuedSpinLockUP:

endif

        ; UP version, always succeed.

        xor     eax, eax
        or      eax, 1
        fstRet  KiTryToAcquireQueuedSpinLock

fstENDP KiTryToAcquireQueuedSpinLock


_TEXT$00   ends

endif   ; NT_INST
        end
