/*++

Copyright (C) Microsoft Corporation, 1990 - 1998

Module Name:

    lock.c

Abstract:

    This is the NT SCSI port driver.

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "classp.h"

LONG LockHighWatermark = 0;
LONG LockLowWatermark = 0;
LONG MaxLockedMinutes = 5;

//
// Structure used for tracking remove lock allocations in checked builds
//
typedef struct _REMOVE_TRACKING_BLOCK {
    struct _REMOVE_TRACKING_BLOCK *NextBlock;
    PVOID Tag;
    LARGE_INTEGER TimeLocked;
    PCSTR File;
    ULONG Line;
} REMOVE_TRACKING_BLOCK, *PREMOVE_TRACKING_BLOCK;


/*++////////////////////////////////////////////////////////////////////////////

ClassAcquireRemoveLockEx()

Routine Description:

    This routine is called to acquire the remove lock on the device object.
    While the lock is held, the caller can assume that no pending pnp REMOVE
    requests will be completed.

    The lock should be acquired immediately upon entering a dispatch routine.
    It should also be acquired before creating any new reference to the
    device object if there's a chance of releasing the reference before the
    new one is done.

    This routine will return TRUE if the lock was successfully acquired or
    FALSE if it cannot be because the device object has already been removed.

Arguments:

    DeviceObject - the device object to lock

    Tag - Used for tracking lock allocation and release.  If an irp is
          specified when acquiring the lock then the same Tag must be
          used to release the lock before the Tag is completed.

Return Value:

    The value of the IsRemoved flag in the device extension.  If this is
    non-zero then the device object has received a Remove irp and non-cleanup
    IRP's should fail.

    If the value is REMOVE_COMPLETE, the caller should not even release the
    lock.

--*/
ULONG
NTAPI
ClassAcquireRemoveLockEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag,
    IN PCSTR File,
    IN ULONG Line
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;



    //
    // Grab the remove lock
    //
    lockValue = InterlockedIncrement(&commonExtension->RemoveLock);

    #if DBG

        DebugPrint((ClassDebugRemoveLock, "ClassAcquireRemoveLock: "
                    "Acquired for Object %p & irp %p - count is %d\n",
                    DeviceObject, Tag, lockValue));

        ASSERTMSG("ClassAcquireRemoveLock - lock value was negative : ",
                  (lockValue > 0));

        ASSERTMSG("RemoveLock increased to meet LockHighWatermark",
                  ((LockHighWatermark == 0) ||
                   (lockValue != LockHighWatermark)));

        if (commonExtension->IsRemoved != REMOVE_COMPLETE){
            PREMOVE_TRACKING_BLOCK trackingBlock;

            trackingBlock = ExAllocatePool(NonPagedPool,
                                           sizeof(REMOVE_TRACKING_BLOCK));

            if(trackingBlock == NULL) {

                KIRQL oldIrql;

                KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                                  &oldIrql);

                commonExtension->RemoveTrackingUntrackedCount++;
            
                DebugPrint((ClassDebugWarning, ">>>>>ClassAcquireRemoveLock: "
                            "Cannot track Tag %p - currently %d untracked requests\n",
                            Tag, commonExtension->RemoveTrackingUntrackedCount));

                KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                                  oldIrql);
            } 
            else {
                PREMOVE_TRACKING_BLOCK *removeTrackingList =
                    (PREMOVE_TRACKING_BLOCK*)&commonExtension->RemoveTrackingList;

                KIRQL oldIrql;

                trackingBlock->Tag = Tag;

                trackingBlock->File = File;
                trackingBlock->Line = Line;

                KeQueryTickCount((&trackingBlock->TimeLocked));

                KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                                  &oldIrql);

                while(*removeTrackingList != NULL) {

                    if((*removeTrackingList)->Tag > Tag) {
                        break;
                    }

                    if((*removeTrackingList)->Tag == Tag) {

                        DebugPrint((ClassDebugError, ">>>>>ClassAcquireRemoveLock: "
                                    "already tracking Tag %p\n", Tag));
                        DebugPrint((ClassDebugError, ">>>>>ClassAcquireRemoveLock: "
                                    "acquired in file %s on line %d\n",
                                    (*removeTrackingList)->File,
                                    (*removeTrackingList)->Line));
                        ASSERT(FALSE);
                    }

                    removeTrackingList = &((*removeTrackingList)->NextBlock);
                }

                trackingBlock->NextBlock = *removeTrackingList;
                *removeTrackingList = trackingBlock;

                KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                                  oldIrql);

            }
        }

    #endif

    return (commonExtension->IsRemoved);
}

/*++////////////////////////////////////////////////////////////////////////////

ClassReleaseRemoveLock()

Routine Description:

    This routine is called to release the remove lock on the device object.  It
    must be called when finished using a previously locked reference to the
    device object.  If an Tag was specified when acquiring the lock then the
    same Tag must be specified when releasing the lock.

    When the lock count reduces to zero, this routine will signal the waiting
    remove Tag to delete the device object.  As a result the DeviceObject
    pointer should not be used again once the lock has been released.

Arguments:

    DeviceObject - the device object to lock

    Tag - The irp (if any) specified when acquiring the lock.  This is used
          for lock tracking purposes

Return Value:

    none

--*/
VOID
NTAPI
ClassReleaseRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PIRP Tag
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;

    #if DBG
        PREMOVE_TRACKING_BLOCK *listEntry =
            (PREMOVE_TRACKING_BLOCK*)&commonExtension->RemoveTrackingList;

        BOOLEAN found = FALSE;

        LONGLONG maxCount;

        BOOLEAN isRemoved = (commonExtension->IsRemoved == REMOVE_COMPLETE);

        KIRQL oldIrql;

        if(isRemoved) {
            DBGTRAP(("ClassReleaseRemoveLock: REMOVE_COMPLETE set; this should never happen"));
            InterlockedDecrement(&(commonExtension->RemoveLock));
            return;
        }

        //
        // Check the tick count and make sure this thing hasn't been locked
        // for more than MaxLockedMinutes.
        //

        maxCount = KeQueryTimeIncrement() * 10;     // microseconds
        maxCount *= 1000;                           // milliseconds
        maxCount *= 1000;                           // seconds
        maxCount *= 60;                             // minutes
        maxCount *= MaxLockedMinutes;

        DebugPrint((ClassDebugRemoveLock, "ClassReleaseRemoveLock: "
                    "maxCount = %0I64x\n", maxCount));

        KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                          &oldIrql);

        while(*listEntry != NULL) {

            PREMOVE_TRACKING_BLOCK block;
            LARGE_INTEGER difference;

            block = *listEntry;

            KeQueryTickCount((&difference));

            difference.QuadPart -= block->TimeLocked.QuadPart;

            DebugPrint((ClassDebugRemoveLock, "ClassReleaseRemoveLock: "
                        "Object %p (tag %p) locked for %I64d ticks\n",
                        DeviceObject, block->Tag, difference.QuadPart));

            if(difference.QuadPart >= maxCount) {

                DebugPrint((ClassDebugError, ">>>>>ClassReleaseRemoveLock: "
                            "Object %p (tag %p) locked for %I64d ticks - TOO LONG\n",
                            DeviceObject, block->Tag, difference.QuadPart));
                DebugPrint((ClassDebugError, ">>>>>ClassReleaseRemoveLock: "
                            "Lock acquired in file %s on line %d\n",
                            block->File, block->Line));
                ASSERT(FALSE);
            }

            if((found == FALSE) && ((*listEntry)->Tag == Tag)) {

                *listEntry = block->NextBlock;
                ExFreePool(block);
                found = TRUE;

            } else {

                listEntry = &((*listEntry)->NextBlock);

            }
        }

        if(!found) {
            if(commonExtension->RemoveTrackingUntrackedCount == 0) {
                DebugPrint((ClassDebugError, ">>>>>ClassReleaseRemoveLock: "
                            "Couldn't find Tag %p in the lock tracking list\n",
                            Tag));
                ASSERT(FALSE);
            } else {
                DebugPrint((ClassDebugError, ">>>>>ClassReleaseRemoveLock: "
                            "Couldn't find Tag %p in the lock tracking list - "
                            "may be one of the %d untracked requests still "
                            "outstanding\n",
                            Tag,
                            commonExtension->RemoveTrackingUntrackedCount));

                commonExtension->RemoveTrackingUntrackedCount--;
                ASSERT(commonExtension->RemoveTrackingUntrackedCount >= 0);
            }
        }

        KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                          oldIrql);

    #endif

    lockValue = InterlockedDecrement(&commonExtension->RemoveLock);

    DebugPrint((ClassDebugRemoveLock, "ClassReleaseRemoveLock: "
                "Released for Object %p & irp %p - count is %d\n",
                DeviceObject, Tag, lockValue));

    ASSERT(lockValue >= 0);

    ASSERTMSG("RemoveLock decreased to meet LockLowWatermark",
              ((LockLowWatermark == 0) || !(lockValue == LockLowWatermark)));

    if(lockValue == 0) {

        ASSERT(commonExtension->IsRemoved);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //

        DebugPrint((ClassDebugRemoveLock, "ClassReleaseRemoveLock: "
                    "Release for object %p & irp %p caused lock to go to zero\n",
                    DeviceObject, Tag));

        KeSetEvent(&commonExtension->RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);

    }
    return;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassCompleteRequest()

Routine Description:

    This routine is a wrapper around (and should be used instead of)
    IoCompleteRequest.  It is used primarily for debugging purposes.
    The routine will assert if the Irp being completed is still holding
    the release lock.

Arguments:

    DeviceObject - the device object that was handling this request
    
    Irp - the irp to be completed by IoCompleteRequest
    
    PriorityBoost - the priority boost to pass to IoCompleteRequest

Return Value:

    none

--*/
VOID
NTAPI
ClassCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{

    #if DBG
        PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
        PREMOVE_TRACKING_BLOCK *listEntry =
            (PREMOVE_TRACKING_BLOCK*)&commonExtension->RemoveTrackingList;

        KIRQL oldIrql;

        KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                          &oldIrql);

        while(*listEntry != NULL) {

            if((*listEntry)->Tag == Irp) {
                break;
            }

            listEntry = &((*listEntry)->NextBlock);
        }

        if(*listEntry != NULL) {

            DebugPrint((ClassDebugError, ">>>>>ClassCompleteRequest: "
                        "Irp %p completed while still holding the remove lock\n",
                        Irp));
            DebugPrint((ClassDebugError, ">>>>>ClassCompleteRequest: "
                        "Lock acquired in file %s on line %d\n",
                        (*listEntry)->File, (*listEntry)->Line));
            ASSERT(FALSE);
        }

        KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);
    #endif

    IoCompleteRequest(Irp, PriorityBoost);
    return;
} // end ClassCompleteRequest()

