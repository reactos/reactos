/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/control.c
 * PURPOSE:     Program control routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>

VOID
EXPORT
NdisReinitializePacket(
     IN OUT  PNDIS_PACKET    Packet)
{
	(Packet)->Private.Head = (PNDIS_BUFFER)NULL;
	(Packet)->Private.ValidCounts = FALSE;
}

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
    UNIMPLEMENTED
}


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

    Status = KeWaitForSingleObject(&Event->Event,
				   Executive,
				   KernelMode,
				   TRUE,
				   &Timeout);

    return (Status == STATUS_SUCCESS);
}

/* EOF */
