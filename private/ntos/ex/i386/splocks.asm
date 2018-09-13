        title "Global SpinLock declerations"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   splocks.asm
;
;Abstract:
;
;   All global spinlocks in the kernel image are declared in this
;   module.  This is done so that each spinlock can be spaced out
;   sufficiently to guaarantee that the L2 cache does not thrash
;   by having a spinlock and another high use variable in the same
;   cache line.
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
        .xlist

PADLOCKS  equ   128


SPINLOCK macro  SpinLockName
    public  SpinLockName
SpinLockName    dd      0

ifndef NT_UP
                db      PADLOCKS-4 dup (0)
endif
endm

ULONG macro  VariableName
    public  VariableName
VariableName     dd      0

ifndef NT_UP
                db      PADLOCKS-4 dup (0)
endif
endm

_DATA   SEGMENT PARA PUBLIC 'DATA'

;
; Static SpinLocks from ntos\cc\cachedat.c
;

;;align PADLOCKS
            db      PADLOCKS dup (0)

SPINLOCK    _CcMasterSpinLock
SPINLOCK    _CcWorkQueueSpinlock
SPINLOCK    _CcVacbSpinLock
SPINLOCK    _CcDeferredWriteSpinLock
SPINLOCK    _CcDebugTraceLock
SPINLOCK    _CcBcbSpinLock


;
; Static SpinLocks from ntos\ex
;

SPINLOCK    _ExpLuidLock                ; luid.c
SPINLOCK    _NonPagedPoolLock           ; pool.c
SPINLOCK    _ExpResourceSpinLock        ; resource.c


;
; Static SpinLocks from ntos\io\iodata.c
;

SPINLOCK    _IopCompletionLock
SPINLOCK    _IopCancelSpinLock
SPINLOCK    _IopVpbSpinLock
SPINLOCK    _IopDatabaseLock
SPINLOCK    _IopErrorLogLock
SPINLOCK    _IopErrorLogAllocationLock
SPINLOCK    _IopTimerLock
SPINLOCK    _IoStatisticsLock
SPINLOCK    _IopFastLockSpinLock


;
; Static SpinLocks from ntos\kd\kdlock.c
;

SPINLOCK    _KdpDebuggerLock


;
; Static SpinLocks from ntos\ke\kernldat.c
;

SPINLOCK    _KiContextSwapLock
SPINLOCK    _KiDispatcherLock
SPINLOCK    _KiFreezeExecutionLock
SPINLOCK    _KiFreezeLockBackup
ULONG       _KiHardwareTrigger
SPINLOCK    _KiProfileLock

;
; Static SpinLocks from ntos\mm\miglobal.c
;

SPINLOCK    _MmPfnLock
SPINLOCK    _MmSystemSpaceLock
SPINLOCK    _MmChargeCommitmentLock

;
; Static SpinLocks from ntos\ps\psinit.c
;

SPINLOCK    _PspEventPairLock
SPINLOCK    _PsLoadedModuleSpinLock

;
; Static SpinLocks from ntos\fsrtl\fsrtlp.c
;

SPINLOCK    _FsRtlStrucSupSpinLock          ; fsrtlp.c

        db      PADLOCKS dup (0)

;
; IopLookasideIrpFloat - This is the number of IRPs that are currently
;      in progress that were allocated from a lookaside list.
;

        public  _IopLookasideIrpFloat
_IopLookasideIrpFloat       dd      0

;
; IopLookasideIrpLimit - This is the maximum number of IRPs that can be
;      in progress that were allocated from a lookaside list.
;

        public  _IopLookasideIrpLimit
_IopLookasideIrpLimit       dd      0

;
; KeTickCount - This is the number of clock ticks that have occurred since
;      the system was booted. This count is used to compute a millisecond
;      tick counter.
;

        public  _KeTickCount
_KeTickCount                dd      0, 0, 0

;
; KeMaximumIncrement - This is the maximum time between clock interrupts
;      in 100ns units that is supported by the host HAL.
;

        public  _KeMaximumIncrement
_KeMaximumIncrement         dd      0

;
; KeTimeAdjustment - This is the actual number of 100ns units that are to
;      be added to the system time at each interval timer interupt. This
;      value is copied from KeTimeIncrement at system start up and can be
;      later modified via the set system information service.
;      timer table entries.
;

        public  _KeTimeAdjustment
_KeTimeAdjustment           dd      0

;
; KiTickOffset - This is the number of 100ns units remaining before a tick
;      is added to the tick count and the system time is updated.
;

        public  _KiTickOffset
_KiTickOffset               dd      0

;
; KiMaximumDpcQueueDepth - This is used to control how many DPCs can be
;      queued before a DPC of medium importance will trigger a dispatch
;      interrupt.
;

        public  _KiMaximumDpcQueueDepth
_KiMaximumDpcQueueDepth     dd      4

;
; KiMinimumDpcRate - This is the rate of DPC requests per clock tick that
;      must be exceeded before DPC batching of medium importance DPCs
;      will occur.
;

        public  _KiMinimumDpcRate
_KiMinimumDpcRate           dd      3

;
; KiAdjustDpcThreshold - This is the threshold used by the clock interrupt
;      routine to control the rate at which the processor's DPC queue depth
;      is dynamically adjusted.
;

        public  _KiAdjustDpcThreshold
_KiAdjustDpcThreshold       dd      20

;
; KiIdealDpcRate - This is used to control the aggressiveness of the DPC
;      rate adjusting algorithm when decrementing the queue depth. As long
;      as the DPC rate for the last tick is greater than this rate, the
;      DPC queue depth will not be decremented.
;

        public  _KiIdealDpcRate
_KiIdealDpcRate             dd      20

_DATA   ends

end
