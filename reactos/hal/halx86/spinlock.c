/* $Id: spinlock.c,v 1.8 2004/02/10 15:16:12 gvg Exp $
 *
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

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

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

  OldIrql = KfRaiseIrql(SYNCH_LEVEL);
  KiAcquireSpinLock(SpinLock);

  return OldIrql;
}

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

KIRQL FASTCALL
KfAcquireSpinLock (
	PKSPIN_LOCK	SpinLock
	)
{
   KIRQL OldIrql;

   assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);
   
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
   assert(KeGetCurrentIrql() == DISPATCH_LEVEL || KeGetCurrentIrql() == SYNCH_LEVEL);
   KiReleaseSpinLock(SpinLock);
   KfLowerIrql(NewIrql);
}

/* EOF */
