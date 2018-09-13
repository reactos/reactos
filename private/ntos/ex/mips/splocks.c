/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   splocks.c

Abstract:

   All global spinlocks in the kernel image are declared in this
   module.  This is done so that each spinlock can be spaced out
   sufficiently to guaarantee that the L2 cache does not thrash
   by having a spinlock and another high use varible in the same
   cache line.


Author:

    Ken Reneris (kenr) 13-Jan-1992

Revision History:

--*/

#include "exp.h"

//
// Static SpinLocks from ntos\cc\cachedat.c
//

KSPIN_LOCK  CcWorkQueueSpinlock = 0;
KSPIN_LOCK  CcDeferredWriteSpinLock = 0;
KSPIN_LOCK  CcDebugTraceLock = 0;
KSPIN_LOCK  CcBcbSpinLock = 0;

//
// Static SpinLocks from ntos\ex
//

KSPIN_LOCK  ExpLuidLock = 0;
KSPIN_LOCK  PoolTraceLock = 0;

//
// Static SpinLocks from ntos\io\iodata.c
//

KSPIN_LOCK  IopErrorLogLock = 0;
KSPIN_LOCK  IopErrorLogAllocationLock = 0;
KSPIN_LOCK  IopTimerLock = 0;

//
// Static SpinLocks from ntos\kd\kdlock.c
//

KSPIN_LOCK  KdpDebuggerLock = 0;

//
// Static SpinLocks from ntos\ke\kernldat.c
//

KSPIN_LOCK  KiFreezeExecutionLock = 0;
KSPIN_LOCK  KiFreezeLockBackup = 0;
ULONG       KiHardwareTrigger = 0;
KSPIN_LOCK  KiPowerNotifyLock = 0;
KSPIN_LOCK  KiProfileLock = 0;

//
// Static SpinLocks from ntos\mm\miglobal.c
//

KSPIN_LOCK  MmSystemSpaceLock = 0;
KSPIN_LOCK  MmAllowWSExpansionLock = 0;

//
// Static SpinLocks from ntos\ps\psinit.c
//

KSPIN_LOCK  PspEventPairLock = 0;
KSPIN_LOCK  PsLoadedModuleSpinLock = 0;

//
// Static SpinLocks from ntos\fsrtl\fsrtlp.c
//

KSPIN_LOCK  FsRtlStrucSupSpinLock = 0;
