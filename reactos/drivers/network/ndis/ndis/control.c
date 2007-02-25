/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/control.c
 * PURPOSE:     Program control routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   3 Oct 2003 Vizzini - Formatting and minor bugfixes
 */

#include "ndissys.h"


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock)
/*
 * FUNCTION: Initialize a NDIS_RW_LOCK
 * ARGUMENTS:
 *     Lock: pointer to the lock to initialize
 * NOTES:
 *    NDIS 5.0
 */
{
  RtlZeroMemory(Lock, sizeof(NDIS_RW_LOCK));
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAcquireReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  BOOLEAN         fWrite,
    IN  PLOCK_STATE     LockState)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  ULONG RefCount;
  UCHAR ProcessorNumber;
  volatile UCHAR BusyLoop;

  ASSERT_IRQL(DISPATCH_LEVEL);

  if (fWrite) {
    if (Lock->Context == PsGetCurrentThread()) {
      LockState->LockState = 2;
    } else {
      KeAcquireSpinLock(&Lock->SpinLock, &LockState->OldIrql);
      /* Check if any other processor helds a shared lock. */
      for (ProcessorNumber = KeNumberProcessors; ProcessorNumber--; ) {
        if (ProcessorNumber != KeGetCurrentProcessorNumber()) {
          /* Wait till the shared lock is released. */
          while (Lock->RefCount[ProcessorNumber].RefCount != 0) {
            for (BusyLoop = 32; BusyLoop--; )
              ;
          }
        }
      }
      Lock->Context = PsGetCurrentThread();
      LockState->LockState = 4;
    }
  } else {
    KeRaiseIrql(DISPATCH_LEVEL, &LockState->OldIrql);
    RefCount = InterlockedIncrement((PLONG)&Lock->RefCount[KeGetCurrentProcessorNumber()].RefCount);
    /* Racing with a exclusive write lock case. */
    if (Lock->SpinLock != 0) {
      if (RefCount == 1) {
        if (Lock->Context != PsGetCurrentThread()) {
          /* Wait for the exclusive lock to be released. */
          Lock->RefCount[KeGetCurrentProcessorNumber()].RefCount--;
          KefAcquireSpinLockAtDpcLevel(&Lock->SpinLock);
          Lock->RefCount[KeGetCurrentProcessorNumber()].RefCount++;
          KefReleaseSpinLockFromDpcLevel(&Lock->SpinLock);
        }
      }
    }
    LockState->LockState = 3;
  }
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReleaseReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  PLOCK_STATE     LockState)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  switch (LockState->LockState) {
    case 2: /* Exclusive write lock, recursive */
      return;

    case 3: /* Shared read lock */
      Lock->RefCount[KeGetCurrentProcessorNumber()].RefCount--;
      LockState->LockState = -1;
      if (LockState->OldIrql < DISPATCH_LEVEL)
        KeLowerIrql(LockState->OldIrql);
      return;

    case 4: /* Exclusive write lock */
      Lock->Context = NULL;
      LockState->LockState = -1;
      KfReleaseSpinLock(&Lock->SpinLock, LockState->OldIrql);
      return;
  }  
}


/*
 * @implemented
 */
#undef NdisAcquireSpinLock
VOID
EXPORT
NdisAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Acquires a spin lock for exclusive access to a resource
 * ARGUMENTS:
 *     SpinLock = Pointer to the initialized NDIS spin lock to be acquired
 */
{
  KeAcquireSpinLock(&SpinLock->SpinLock, &SpinLock->OldIrql);
}


/*
 * @implemented
 */
#undef NdisAllocateSpinLock
VOID
EXPORT
NdisAllocateSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Initializes for an NDIS spin lock
 * ARGUMENTS:
 *     SpinLock = Pointer to an NDIS spin lock structure
 */
{
  KeInitializeSpinLock(&SpinLock->SpinLock);
}


/*
 * @implemented
 */
#undef NdisDprAcquireSpinLock
VOID
EXPORT
NdisDprAcquireSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Acquires a spin lock from IRQL DISPATCH_LEVEL
 * ARGUMENTS:
 *     SpinLock = Pointer to the initialized NDIS spin lock to be acquired
 */
{
  KeAcquireSpinLockAtDpcLevel(&SpinLock->SpinLock);
  SpinLock->OldIrql = DISPATCH_LEVEL;
}


/*
 * @implemented
 */
#undef NdisDprReleaseSpinLock
VOID
EXPORT
NdisDprReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Releases an acquired spin lock from IRQL DISPATCH_LEVEL
 * ARGUMENTS:
 *     SpinLock = Pointer to the acquired NDIS spin lock to be released
 */
{
  KeReleaseSpinLockFromDpcLevel(&SpinLock->SpinLock);
}


/*
 * @implemented
 */
#undef NdisFreeSpinLock
VOID
EXPORT
NdisFreeSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Releases a spin lock initialized with NdisAllocateSpinLock
 * ARGUMENTS:
 *     SpinLock = Pointer to an initialized NDIS spin lock
 */
{
  /* Nothing to do here! */
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisGetCurrentProcessorCpuUsage(
    PULONG  pCpuUsage)
/*
 * FUNCTION: Returns how busy the current processor is as a percentage
 * ARGUMENTS:
 *     pCpuUsage = Pointer to a buffer to place CPU usage
 */
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeEvent(
    IN  PNDIS_EVENT Event)
/*
 * FUNCTION: Initializes an event to be used for synchronization
 * ARGUMENTS:
 *     Event = Pointer to an NDIS event structure to be initialized
 */
{
  KeInitializeEvent(&Event->Event, NotificationEvent, FALSE);
}


/*
 * @implemented
 */
#undef NdisReleaseSpinLock
VOID
EXPORT
NdisReleaseSpinLock(
    IN  PNDIS_SPIN_LOCK SpinLock)
/*
 * FUNCTION: Releases a spin lock previously acquired with NdisAcquireSpinLock
 * ARGUMENTS:
 *     SpinLock = Pointer to the acquired NDIS spin lock to be released
 */
{
  KeReleaseSpinLock(&SpinLock->SpinLock, SpinLock->OldIrql);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisResetEvent(
    IN  PNDIS_EVENT Event)
/*
 * FUNCTION: Clears the signaled state of an event
 * ARGUMENTS:
 *     Event = Pointer to the initialized event object to be reset
 */
{
  KeResetEvent(&Event->Event);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisSetEvent(
    IN  PNDIS_EVENT Event)
/*
 * FUNCTION: Sets an event to a signaled state if not already signaled
 * ARGUMENTS:
 *     Event = Pointer to the initialized event object to be set
 */
{
  KeSetEvent(&Event->Event, IO_NO_INCREMENT, FALSE);
}


/*
 * @implemented
 */
BOOLEAN
EXPORT
NdisWaitEvent(
    IN  PNDIS_EVENT Event,
    IN  UINT        MsToWait)
/*
 * FUNCTION: Waits for an event to become signaled
 * ARGUMENTS:
 *     Event    = Pointer to the initialized event object to wait for
 *     MsToWait = Maximum milliseconds to wait for the event to become signaled
 * RETURNS:
 *     TRUE if the event is in the signaled state
 */
{
  LARGE_INTEGER Timeout;
  NTSTATUS Status;

  Timeout.QuadPart = MsToWait * -10000LL;

  Status = KeWaitForSingleObject(&Event->Event, Executive, KernelMode, TRUE, &Timeout);

  return (Status == STATUS_SUCCESS);
}

/* EOF */

