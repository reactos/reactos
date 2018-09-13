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



Revision History:

--*/

#include "iop.h"

#include <remlock.h>

#pragma alloc_text(PAGE, IoInitializeRemoveLockEx)
#pragma alloc_text(PAGE, IoReleaseRemoveLockAndWaitEx)

#define MinutesToTicks(x) \
        (ULONGLONG) KeQueryTimeIncrement() * \
        10 * \
        1000 * \
        1000 * \
        60 * \
        x

// 10 -> microseconds, 1000 -> miliseconds, 1000 -> seconds, 60 -> minutes


typedef struct _IO_PRIVATE_REMOVE_LOCK {
    IO_REMOVE_LOCK_COMMON_BLOCK Common;
    IO_REMOVE_LOCK_DBG_BLOCK Dbg;
} IO_PRIVATE_REMOVE_LOCK, *PIO_PRIVATE_REMOVE_LOCK;


#define FREESIZE sizeof (IO_REMOVE_LOCK_COMMON_BLOCK)
#define CHECKEDSIZE sizeof (IO_PRIVATE_REMOVE_LOCK)


NTSYSAPI
VOID
NTAPI
IoInitializeRemoveLockEx(
    IN  PIO_REMOVE_LOCK PublicLock,
    IN  ULONG   AllocateTag, // Used only on checked kernels
    IN  ULONG   MaxLockedMinutes, // Used only on checked kernels
    IN  ULONG   HighWatermark, // Used only on checked kernels
    IN  ULONG   RemlockSize // are we checked or free
    )
/*++

Routine Description:

    This routine is called to initialize the remove lock for a device object.

--*/
{
    PIO_PRIVATE_REMOVE_LOCK Lock = (PIO_PRIVATE_REMOVE_LOCK) PublicLock;

    PAGED_CODE ();

    if (Lock) {

        switch (RemlockSize) {

        case CHECKEDSIZE:
            Lock->Dbg.Signature = IO_REMOVE_LOCK_SIG;
            Lock->Dbg.HighWatermark = HighWatermark;
            Lock->Dbg.MaxLockedTicks = MinutesToTicks (MaxLockedMinutes);
            Lock->Dbg.AllocateTag = AllocateTag;
            KeInitializeSpinLock (&Lock->Dbg.Spin);
            Lock->Dbg.LowMemoryCount = 0;
            Lock->Dbg.Blocks = NULL;

            //
            // fall through
            //
        case FREESIZE:
            Lock->Common.Removed = FALSE;
            Lock->Common.IoCount = 1;
            KeInitializeEvent(&Lock->Common.RemoveEvent,
                              SynchronizationEvent,
                              FALSE);
            break;

        default:
            break;
        }
    }
}


NTSYSAPI
NTSTATUS
NTAPI
IoAcquireRemoveLockEx(
    IN PIO_REMOVE_LOCK PublicLock,
    IN OPTIONAL PVOID   Tag,
    IN PCSTR            File,
    IN ULONG            Line,
    IN ULONG            RemlockSize // are we checked or free
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
    IoReleaseRemoveLock when finished.

    If not successful the lock was not obtained.  The caller should abort the
    work but not call IoReleaseRemoveLock.

--*/

{
    PIO_PRIVATE_REMOVE_LOCK Lock = (PIO_PRIVATE_REMOVE_LOCK) PublicLock;
    LONG        lockValue;
    NTSTATUS    status;

    PIO_REMOVE_LOCK_TRACKING_BLOCK trackingBlock;

    //
    // Grab the remove lock
    //

    lockValue = InterlockedIncrement(&Lock->Common.IoCount);

    ASSERTMSG("IoAcquireRemoveLock - lock value was negative : ",
              (lockValue > 0));

    if (! Lock->Common.Removed) {

        switch (RemlockSize) {
        case CHECKEDSIZE:

            ASSERTMSG("RemoveLock increased to meet LockHighWatermark",
                      ((0 == Lock->Dbg.HighWatermark) ||
                       (lockValue <= Lock->Dbg.HighWatermark)));

            trackingBlock = ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(IO_REMOVE_LOCK_TRACKING_BLOCK),
                                Lock->Dbg.AllocateTag);

            if (NULL == trackingBlock) {

                // ASSERTMSG ("insufficient resources", FALSE);
                InterlockedIncrement (& Lock->Dbg.LowMemoryCount);
                //
                // Let the acquire go through but without adding the
                // tracking block.
                // When we are later releasing the lock, but the tracking
                // block does not exist, deduct from this value to see if the
                // release was still valuable.
                //

            } else {

                KIRQL oldIrql;

                RtlZeroMemory (trackingBlock,
                               sizeof (IO_REMOVE_LOCK_TRACKING_BLOCK));

                trackingBlock->Tag = Tag;
                trackingBlock->File = File;
                trackingBlock->Line = Line;

                KeQueryTickCount(&trackingBlock->TimeLocked);

                ExAcquireSpinLock (&Lock->Dbg.Spin, &oldIrql);
                trackingBlock->Link = Lock->Dbg.Blocks;
                Lock->Dbg.Blocks = trackingBlock;
                ExReleaseSpinLock(&Lock->Dbg.Spin, oldIrql);
            }
            break;

        case FREESIZE:
            break;

        default:
            break;
        }

        status = STATUS_SUCCESS;

    } else {

        if (0 == InterlockedDecrement (&Lock->Common.IoCount)) {
            KeSetEvent (&Lock->Common.RemoveEvent, 0, FALSE);
        }
        status = STATUS_DELETE_PENDING;
    }

    return status;
}


NTSYSAPI
VOID
NTAPI
IoReleaseRemoveLockEx(
    IN PIO_REMOVE_LOCK PublicLock,
    IN PVOID            Tag,
    IN ULONG            RemlockSize // are we checked or free
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
    PIO_PRIVATE_REMOVE_LOCK Lock = (PIO_PRIVATE_REMOVE_LOCK) PublicLock;
    LONG            lockValue;
    KIRQL           oldIrql;
    LARGE_INTEGER   ticks;
    LONGLONG        difference;
    BOOLEAN         found;

    PIO_REMOVE_LOCK_TRACKING_BLOCK last;
    PIO_REMOVE_LOCK_TRACKING_BLOCK current;

    switch (RemlockSize) {
    case CHECKEDSIZE:

        //
        // Check the tick count and make sure this thing hasn't been locked
        // for more than MaxLockedMinutes.
        //

        found = FALSE;
        ExAcquireSpinLock(&Lock->Dbg.Spin, &oldIrql);
        last = (Lock->Dbg.Blocks);
        current = last;

        KeQueryTickCount((&ticks));

        while (NULL != current) {

            if (Lock->Dbg.MaxLockedTicks) {
                difference = ticks.QuadPart - current->TimeLocked.QuadPart;

                if (Lock->Dbg.MaxLockedTicks < difference) {

                    KdPrint(("IoReleaseRemoveLock: Lock %#08lx (tag %#08lx) "
                             "locked for %I64d ticks - TOO LONG\n",
                             Lock,
                             current->Tag,
                             difference));

                    KdPrint(("IoReleaseRemoveLock: Lock acquired in file "
                             "%s on line %d\n",
                             current->File,
                             current->Line));
                    ASSERT(FALSE);
                }
            }

            if ((!found) && (current->Tag == Tag)) {
                found = TRUE;
                if (current == Lock->Dbg.Blocks) {
                    Lock->Dbg.Blocks = current->Link;
                    ExFreePool (current);
                    current = Lock->Dbg.Blocks;
                } else {
                    last->Link = current->Link;
                    ExFreePool (current);
                    current = last->Link;
                }
                continue;
            }

            last = current;
            current = current->Link;
        }

        ExReleaseSpinLock(&Lock->Dbg.Spin, oldIrql);

        if (!found) {
            //
            // Check to see if we have any credits in our Low Memory Count.
            // In this fassion we can tell if we have acquired any locks without
            // the memory for adding tracking blocks.
            //
            if (InterlockedDecrement (& Lock->Dbg.LowMemoryCount) < 0) {
                //
                // We have just released a lock that neither had a corresponding
                // tracking block, nor a credit in LowMemoryCount.
                //
                InterlockedIncrement (& Lock->Dbg.LowMemoryCount);
                KdPrint (("IoReleaseRemoveLock: Couldn't find Tag %#08lx "
                          "in the lock tracking list\n",
                          Tag));
                ASSERT(FALSE);
            }
        }
        break;

    case FREESIZE:
        break;

    default:
        break;
    }

    lockValue = InterlockedDecrement(&Lock->Common.IoCount);

    ASSERT(0 <= lockValue);

    if (0 == lockValue) {

        ASSERT (Lock->Common.Removed);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //

        KeSetEvent(&Lock->Common.RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }
    return;
}


NTSYSAPI
VOID
NTAPI
IoReleaseRemoveLockAndWaitEx (
    IN PIO_REMOVE_LOCK PublicLock,
    IN PVOID            Tag,
    IN ULONG            RemlockSize // are we checked or free
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
    PIO_PRIVATE_REMOVE_LOCK Lock = (PIO_PRIVATE_REMOVE_LOCK) PublicLock;
    LONG    ioCount;

    PAGED_CODE ();

    Lock->Common.Removed = TRUE;

    ioCount = InterlockedDecrement (&Lock->Common.IoCount);
    ASSERT (0 < ioCount);

    if (0 < InterlockedDecrement (&Lock->Common.IoCount)) {
        KeWaitForSingleObject (&Lock->Common.RemoveEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
    }

    switch (RemlockSize) {
    case CHECKEDSIZE:

        ASSERT (Lock->Dbg.Blocks);
        if (Tag != Lock->Dbg.Blocks->Tag) {
            KdPrint (("IoRelaseRemoveLockAndWait last tag invalid %x %x\n",
                      Tag,
                      Lock->Dbg.Blocks->Tag));

            ASSERT (Tag != Lock->Dbg.Blocks->Tag);
        }

        ExFreePool (Lock->Dbg.Blocks);
        break;

    case FREESIZE:
        break;

    default:
        break;

    }
}


