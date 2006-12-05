/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  09/06/2000 Created
 */

/*
 * NOTE: On a uniprocessor machine spinlocks are implemented by raising
 * the irq level
 */

/* INCLUDES ****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* Hmm, needed for KDBG := 1. Why? */
#undef KeGetCurrentIrql

/* FUNCTIONS ***************************************************************/

#undef KeAcquireSpinLock
VOID STDCALL
KeAcquireSpinLock (
	PKSPIN_LOCK	SpinLock,
	PKIRQL		OldIrql
	)
/*
 * FUNCTION: Acquires a spinlock
 * ARGUMENTS:
 *         SpinLock = Spinlock to acquire
 *         OldIrql (OUT) = Caller supplied storage for the previous irql
 */
{
  *OldIrql = KfAcquireSpinLock(SpinLock);
}

KIRQL FASTCALL
KeAcquireSpinLockRaiseToSynch (
	PKSPIN_LOCK	SpinLock
	)
{
  KIRQL OldIrql;

  OldIrql = KfRaiseIrql(CLOCK2_LEVEL);
  KiAcquireSpinLock(SpinLock);

  return OldIrql;
}

#undef KeReleaseSpinLock
VOID STDCALL
KeReleaseSpinLock (
	PKSPIN_LOCK	SpinLock,
	KIRQL		NewIrql
	)
/*
 * FUNCTION: Releases a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to release
 *        NewIrql = Irql level before acquiring the spinlock
 */
{
   KfReleaseSpinLock(SpinLock, NewIrql);
}

LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
  KSPIN_LOCK_QUEUE_NUMBER LockNumber,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return FALSE;
}


BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(
  KSPIN_LOCK_QUEUE_NUMBER LockNumber,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return FALSE;
}

KIRQL FASTCALL
KfAcquireSpinLock (
	PKSPIN_LOCK	SpinLock
	)
{
   KIRQL OldIrql;

   ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
   
   OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
   KiAcquireSpinLock(SpinLock);

   return OldIrql;
}

VOID FASTCALL
KfReleaseSpinLock (
	PKSPIN_LOCK	SpinLock,
	KIRQL		NewIrql
	)
/*
 * FUNCTION: Releases a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to release
 *        NewIrql = Irql level before acquiring the spinlock
 */
{
   ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL || KeGetCurrentIrql() == SYNCH_LEVEL);
   KiReleaseSpinLock(SpinLock);
   KfLowerIrql(NewIrql);
}


/*
 * @unimplemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
   UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
   UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
   UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
   UNIMPLEMENTED;
   return 0;
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle,
                        IN KIRQL OldIrql)
{
   UNIMPLEMENTED;
}

/* EOF */
