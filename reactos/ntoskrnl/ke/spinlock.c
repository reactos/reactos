/* $Id: spinlock.c,v 1.20 2004/01/18 22:41:53 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  3/6/98: Created
 */

/*
 * NOTE: On a uniprocessor machine spinlocks are implemented by raising
 * the irq level
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOLEAN STDCALL
KeSynchronizeExecution (PKINTERRUPT		Interrupt,
			PKSYNCHRONIZE_ROUTINE	SynchronizeRoutine,
			PVOID			SynchronizeContext)
/*
 * FUNCTION: Synchronizes the execution of a given routine with the ISR
 * of a given interrupt object
 * ARGUMENTS:
 *       Interrupt = Interrupt object to synchronize with
 *       SynchronizeRoutine = Routine to call whose execution is 
 *                            synchronized with the ISR
 *       SynchronizeContext = Parameter to pass to the synchronized routine
 * RETURNS: TRUE if the operation succeeded
 */
{
   KIRQL oldlvl;
   BOOLEAN ret;
   
   KeRaiseIrql(Interrupt->SynchLevel, &oldlvl);
   KiAcquireSpinLock(Interrupt->IrqLock);
   
   ret = SynchronizeRoutine(SynchronizeContext);
   
   KiReleaseSpinLock(Interrupt->IrqLock);
   KeLowerIrql(oldlvl);
   
   return(ret);
}

/*
 * @implemented
 */
VOID STDCALL
KeInitializeSpinLock (PKSPIN_LOCK	SpinLock)
/*
 * FUNCTION: Initalizes a spinlock
 * ARGUMENTS:
 *           SpinLock = Caller supplied storage for the spinlock
 */
{
   *SpinLock = 0;
}

#undef KefAcquireSpinLockAtDpcLevel

/*
 * @implemented
 */
VOID FASTCALL
KefAcquireSpinLockAtDpcLevel(PKSPIN_LOCK SpinLock)
{
  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);
  KiAcquireSpinLock(SpinLock);
}

#undef KeAcquireSpinLockAtDpcLevel

/*
 * @implemented
 */
VOID STDCALL
KeAcquireSpinLockAtDpcLevel (PKSPIN_LOCK	SpinLock)
/*
 * FUNCTION: Acquires a spinlock when the caller is already running at 
 * dispatch level
 * ARGUMENTS:
 *        SpinLock = Spinlock to acquire
 */
{
  KefAcquireSpinLockAtDpcLevel(SpinLock);
}

#undef KefReleaseSpinLockFromDpcLevel

/*
 * @implemented
 */
VOID FASTCALL
KefReleaseSpinLockFromDpcLevel(PKSPIN_LOCK SpinLock)
{
  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);
  KiReleaseSpinLock(SpinLock);  
}

#undef KeReleaseSpinLockFromDpcLevel

/*
 * @implemented
 */
VOID STDCALL
KeReleaseSpinLockFromDpcLevel (PKSPIN_LOCK	SpinLock)
/*
 * FUNCTION: Releases a spinlock when the caller was running at dispatch
 * level before acquiring it
 * ARGUMENTS: 
 *         SpinLock = Spinlock to release
 */
{
  KefReleaseSpinLockFromDpcLevel(SpinLock);
}


/*
 * @implemented
 */
VOID FASTCALL
KiAcquireSpinLock(PKSPIN_LOCK SpinLock)
{
  ULONG i;

  /*
   * FIXME: This depends on gcc assembling this test to a single load from
   * the spinlock's value.
   */
  if (*SpinLock >= 2)
  {
    DbgPrint("Lock %x has bad value %x\n", SpinLock, *SpinLock);
    KEBUGCHECK(0);
  }
   
  while ((i = InterlockedExchange((LONG *)SpinLock, 1)) == 1)
  {
#ifndef MP
    DbgPrint("Spinning on spinlock %x current value %x\n", SpinLock, i);
    KEBUGCHECK(0);
#else /* not MP */
       /* Avoid reading the value again too fast */
#endif /* MP */
  }
}


/*
 * @implemented
 */
VOID FASTCALL
KiReleaseSpinLock(PKSPIN_LOCK SpinLock)
{
  if (*SpinLock != 1)
  {
    DbgPrint("Releasing unacquired spinlock %x\n", SpinLock);
    KEBUGCHECK(0);
  }
  (void)InterlockedExchange((LONG *)SpinLock, 0);
}

/* EOF */
