/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/hal/x86/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  3/6/98: Created
 */

/*
 * NOTE: On a uniprocessor machine spinlocks are implemented by raising
 * the irq level
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN KeSynchronizeExecution(PKINTERRUPT Interrupt,
			       PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
			       PVOID SynchronizeContext)
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
   
   KeRaiseIrql(Interrupt->SynchLevel,&oldlvl);
   KeAcquireSpinLockAtDpcLevel(Interrupt->IrqLock);
   
   ret = SynchronizeRoutine(SynchronizeContext);
   
   KeReleaseSpinLockFromDpcLevel(Interrupt->IrqLock);
   KeLowerIrql(oldlvl);
   
   return(ret);
}

VOID KeInitializeSpinLock(PKSPIN_LOCK SpinLock)
/*
 * FUNCTION: Initalizes a spinlock
 * ARGUMENTS:
 *           SpinLock = Caller supplied storage for the spinlock
 */
{
   SpinLock->irql = DISPATCH_LEVEL;
}

VOID KeAcquireSpinLockAtDpcLevel(PKSPIN_LOCK SpinLock)
/*
 * FUNCTION: Acquires a spinlock when the caller is already running at 
 * dispatch level
 * ARGUMENTS:
 *        SpinLock = Spinlock to acquire
 */
{
}

VOID KeReleaseSpinLockFromDpcLevel(PKSPIN_LOCK SpinLock)
/*
 * FUNCTION: Releases a spinlock when the caller was running at dispatch
 * level before acquiring it
 * ARGUMENTS: 
 *         SpinLock = Spinlock to release
 */
{
}

VOID KeAcquireSpinLock(PKSPIN_LOCK SpinLock, PKIRQL OldIrql)
/*
 * FUNCTION: Acquires a spinlock
 * ARGUMENTS:
 *         SpinLock = Spinlock to acquire
 *         OldIrql (OUT) = Caller supplied storage for the previous irql
 */
{
   KeRaiseIrql(DISPATCH_LEVEL,OldIrql);
}

VOID KeReleaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL NewIrql)
/*
 * FUNCTION: Releases a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to release
 *        NewIrql = Irql level before acquiring the spinlock
 */
{
   KeLowerIrql(NewIrql);
}

