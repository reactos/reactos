/* $Id$
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

#include <ntoskrnl.h>
#define NDEBUG
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
   
   oldlvl = KeAcquireInterruptSpinLock(Interrupt);
   
   ret = SynchronizeRoutine(SynchronizeContext);
   
   KeReleaseInterruptSpinLock(Interrupt, oldlvl);
   
   return(ret);
}

/*
 * @implemented
 */
KIRQL
STDCALL
KeAcquireInterruptSpinLock(
    IN PKINTERRUPT Interrupt
    )
{
   KIRQL oldIrql;
        
   KeRaiseIrql(Interrupt->SynchLevel, &oldIrql);
   KiAcquireSpinLock(Interrupt->ActualLock);
   return oldIrql;
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
  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
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


/*
 * @unimplemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
	UNIMPLEMENTED;
}


#undef KefReleaseSpinLockFromDpcLevel

/*
 * @implemented
 */
VOID FASTCALL
KefReleaseSpinLockFromDpcLevel(PKSPIN_LOCK SpinLock)
{
  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
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
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
	UNIMPLEMENTED;
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
   
  while ((i = InterlockedExchangeUL(SpinLock, 1)) == 1)
  {
#ifdef CONFIG_SMP
    /* Avoid reading the value again too fast */
#if 1
    __asm__ __volatile__ ("1:\n\t"
	                  "cmpl	$0,(%0)\n\t"
			  "jne	1b\n\t"
			  :
                          : "r" (SpinLock));
#else	                  
    while (0 != *(volatile PKSPIN_LOCK)SpinLock)
    {
    }
#endif
#else
    DbgPrint("Spinning on spinlock %x current value %x\n", SpinLock, i);
    KEBUGCHECK(0);
#endif /* CONFIG_SMP */
  }
}

/*
 * @implemented
 */
VOID
STDCALL
KeReleaseInterruptSpinLock(
	IN PKINTERRUPT Interrupt,
	IN KIRQL OldIrql
	)
{
   KiReleaseSpinLock(Interrupt->ActualLock);
   KeLowerIrql(OldIrql);
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
  (void)InterlockedExchangeUL(SpinLock, 0);
}

/* EOF */
