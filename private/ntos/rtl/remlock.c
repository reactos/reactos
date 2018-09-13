/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    remlock.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Peter Wieland
    Kenneth Ray

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <ntos.h>

#include <remlock.h>

#define MinutesToTicks(x) \
        (ULONGLONG) KeQueryTimeIncrement() * \
        10 * \
        1000 * \
        1000 * \
        60 * \
        x
// 10 -> microseconds, 1000 -> miliseconds, 1000 -> seconds, 60 -> minutes

// LIST_ENTRY RtlpRemoveLockList;

NTSYSAPI
PRTL_REMOVE_LOCK
NTAPI
RtlAllocateRemoveLock(
    IN  ULONG   MaxLockedMinutes,
    IN  ULONG   AllocateTag,
    IN  ULONG   HighWatermark
    )
/*++

Routine Description:

    This routine is called to initialize the remove lock for a device object.

--*/
{
    PRTL_REMOVE_LOCK lock;

    lock = ExAllocatePoolWithTag (NonPagedPool,
                                  sizeof (RTL_REMOVE_LOCK),
                                  AllocateTag);

    if (lock) {

        lock->Signature = RTL_REMOVE_LOCK_SIG;
        lock->Removed = FALSE;
        lock->IoCount = 1;
        KeInitializeEvent(&lock->RemoveEvent, SynchronizationEvent, FALSE);
#if DBG
        lock->HighWatermark = HighWatermark;
        lock->MaxLockedMinutes = MaxLockedMinutes;
        lock->AllocateTag = AllocateTag;
        KeInitializeSpinLock (&lock->Spin);
        lock->Blocks.Link = NULL;
#endif
    }

    return lock;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlAcquireRemoveLockEx(
    IN PRTL_REMOVE_LOCK RemoveLock,
    IN OPTIONAL PVOID   Tag,
    IN PCSTR            File,
    IN ULONG            Line
    )

/*++

Routine Description:

    This routine is called to acquire the remove lock for a device object.
    While the lock is held, the caller can assume that no pending pnp REMOVE
    requests will be completed.

    The lock should be acquired immediately upon entering a dispatch routine.
    It should also be acquired before creating any new reference to the
    device object if there's a chance of releasing the reference before the
    new one is done.

Arguments:

    RemoveLock - A pointer to an initialized REMOVE_LOCK structure.

    Tag - Used for tracking lock allocation and release.  If an irp is
          specified when acquiring the lock then the same Tag must be
          used to release the lock before the Tag is completed.

    File - set to __FILE__ as the location in the code where the lock was taken.

    Line - set to __LINE__.

Return Value:

    Returns whether or not the remove lock was obtained.
    If successful the caller should continue with work calling
    RtlReleaseRemoveLock when finished.

    If not successful the lock was not obtained.  The caller should abort the
    work but not call RtlReleaseRemoveLock.

--*/

{
    LONG        lockValue;
    NTSTATUS    status;

#if DBG
    PRTL_REMOVE_LOCK_TRACKING_BLOCK trackingBlock;
#endif

    //
    // Grab the remove lock
    //

    lockValue = InterlockedIncrement(&RemoveLock->IoCount);

    ASSERTMSG("RtlAcquireRemoveLock - lock value was negative : ",
              (lockValue > 0));

    ASSERTMSG("RemoveLock increased to meet LockHighWatermark",
              ((0 == RemoveLock->HighWatermark) ||
               (lockValue <= RemoveLock->HighWatermark)));

    if (! RemoveLock->Removed) {

#if DBG
        trackingBlock = ExAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof(RTL_REMOVE_LOCK_TRACKING_BLOCK),
                            RemoveLock->AllocateTag);

        RtlZeroMemory (trackingBlock,
                       sizeof (RTL_REMOVE_LOCK_TRACKING_BLOCK));

        if (NULL == trackingBlock) {

            ASSERTMSG ("insufficient resources", FALSE);

        } else {

            KIRQL oldIrql;

            trackingBlock->Tag = Tag;
            trackingBlock->File = File;
            trackingBlock->Line = Line;

            KeQueryTickCount(&trackingBlock->TimeLocked);

            KeAcquireSpinLock (&RemoveLock->Spin, &oldIrql);
            trackingBlock->Link = RemoveLock->Blocks.Link;
            RemoveLock->Blocks.Link = trackingBlock;
            KeReleaseSpinLock(&RemoveLock->Spin, oldIrql);
        }
#endif

        status = STATUS_SUCCESS;

    } else {

        if (0 == InterlockedDecrement (&RemoveLock->IoCount)) {
            KeSetEvent (&RemoveLock->RemoveEvent, 0, FALSE);
        }
        status = STATUS_DELETE_PENDING;
    }

    return status;
}


NTSYSAPI
VOID
NTAPI
RtlReleaseRemoveLock(
    IN PRTL_REMOVE_LOCK RemoveLock,
    IN PVOID            Tag
    )

/*++

Routine Description:

    This routine is called to release the remove lock on the device object.  It
    must be called when finished using a previously locked reference to the
    device object.  If an Tag was specified when acquiring the lock then the
    same Tag must be specified when releasing the lock.

    When the lock count reduces to zero, this routine will signal the waiting
    event to release the waiting thread deleting the device object protected
    by this lock.

Arguments:

    DeviceObject - the device object to lock

    Tag - The tag (if any) specified when acquiring the lock.  This is used
          for lock tracking purposes

Return Value:

    none

--*/

{
    LONG            lockValue;

#if DBG
    KIRQL           oldIrql;
    LARGE_INTEGER   difference;
    BOOLEAN         found;
    LONGLONG        maxTime;

    PRTL_REMOVE_LOCK_TRACKING_BLOCK last;
    PRTL_REMOVE_LOCK_TRACKING_BLOCK current;

    //
    // Check the tick count and make sure this thing hasn't been locked
    // for more than MaxLockedMinutes.
    //

    found = FALSE;
    KeAcquireSpinLock(&RemoveLock->Spin, &oldIrql);
    last = (&RemoveLock->Blocks);
    current = last->Link;
    //
    // Note the first one is the sentinal
    //

    while (NULL != current) {

        KeQueryTickCount((&difference));
        difference.QuadPart -= current->TimeLocked.QuadPart;
        maxTime = MinutesToTicks (RemoveLock->MaxLockedMinutes);

        if (maxTime && (maxTime < difference.QuadPart)) {

            KdPrint(("RtlReleaseRemoveLock: Lock %#08lx (tag %#08lx) locked "
                     "for %I64d ticks - TOO LONG\n",
                     RemoveLock,
                     current->Tag,
                     difference.QuadPart));

            KdPrint(("RtlReleaseRemoveLock: Lock acquired in file "
                     "%s on line %d\n",
                     current->File,
                     current->Line));
            ASSERT(FALSE);
        }

        if ((!found) && (current->Tag == Tag)) {
            found = TRUE;
            last->Link = current->Link;
            ExFreePool (current);
                          current = last->Link;
            continue;
        }

        last = current;
        current = current->Link;
    }

    KeReleaseSpinLock(&RemoveLock->Spin, oldIrql);

    if (!found) {

        KdPrint (("RtlReleaseRemoveLock: Couldn't find Tag %#08lx "
                  "in the lock tracking list\n",
                  Tag));
        ASSERT(FALSE);
    }
#endif

    lockValue = InterlockedDecrement(&RemoveLock->IoCount);

    ASSERT(0 <= lockValue);

    if (0 == lockValue) {

        ASSERT (RemoveLock->Removed);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //

        KeSetEvent(&RemoveLock->RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }
    return;
}


NTSYSAPI
VOID
NTAPI
RtlReleaseRemoveLockAndWait (
    IN PRTL_REMOVE_LOCK RemoveLock,
    IN PVOID            Tag
    )

/*++

Routine Description:

    This routine is called when the client would like to delete the remove-
    locked resource.
    This routine will block until all the remove locks have completed.

    This routine MUST be called after acquiring once more the lock.

Arguments:

    RemoveLock -

Return Value:

    none

--*/
{
    LONG    ioCount;

    PAGED_CODE ();

    RemoveLock->Removed = TRUE;

    ioCount = InterlockedDecrement (&RemoveLock->IoCount);
    ASSERT (0 < ioCount);

    if (0 < InterlockedDecrement (&RemoveLock->IoCount)) {
        KeWaitForSingleObject (&RemoveLock->RemoveEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
    }

#if DBG
    ASSERT (RemoveLock->Blocks.Link);
    if (Tag != RemoveLock->Blocks.Link->Tag) {
        KdPrint (("RtlRelaseRemoveLockAndWait last tag invalid %x %x\n",
                  Tag,
                  RemoveLock->Blocks.Link->Tag));

        ASSERT (Tag != RemoveLock->Blocks.Link->Tag);
    }

    ExFreePool (RemoveLock->Blocks.Link);
#endif

}


