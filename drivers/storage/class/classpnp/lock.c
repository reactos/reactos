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
#include "debug.h"

#ifdef DEBUG_USE_WPP
#include "lock.tmh"
#endif


LONG LockHighWatermark = 0;
LONG LockLowWatermark = 0;
LONG MaxLockedMinutes = 5;

//
// Structure used for tracking remove lock allocations in checked builds
//
typedef struct _REMOVE_TRACKING_BLOCK {
    PVOID Tag;
    LARGE_INTEGER TimeLocked;
    PCSTR File;
    ULONG Line;
} REMOVE_TRACKING_BLOCK, *PREMOVE_TRACKING_BLOCK;

/*++////////////////////////////////////////////////////////////////////////////

Classpnp RemoveLockRundown

RemoveLockRundown is a cacheaware rundown protection for the classpnp device object. While this
rundown protection is held successfully, the caller can assume that no pending pnp REMOVE
requests will be completed.

The RemoveLockRundown is a replacement of the original RemoveLock to improve the scalability.
For backward compatibility, we still keep the RemoveLock field in the device common extension structure.
However, the old RemoveLock is only being used in the DBG build.

The usage of the RemoveLockRundown is slightly different from the normal rundown protection usage.
The RemoveLockRundown is acquired via ClassAcquireRemoveLockEx() function
and released via ClassReleaseRemoveLock() function. Usually, we bail out when the acquisition
of rundown protection fails (calls to ExAcquireRundownProtectionCacheAware returns FALSE) and
will not release the rundown protection in acquisition failure. For the RemoveLockRundown,
the caller will always call ClassAcquireRemoveLockEx() and ClassReleaseRemoveLock() in a pair no
matter the return value of ClassAcquireRemoveLockEx(). Therefore, a thread may still call
ClassReleaseRemoveLock() even the previous acquisition RemoveLockRundown protection failed.

To deal with the previous acquisition failure case, we introduced a new field RemoveLockFailAcquire
as a counter for rundown acquisition failures. In the ClassReleaseRemoveLock() function, we only
release the rundown protection when this counter is decremented to zero. Since the change of RemoveLockFailAcquire
and release rundown protection is not protected by a lock as an atomic operation, we use a while loop over
InterlockedCompareExchange operation to make sure when we release the rundown protection, this counter is
actually zero.

--*/

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassAcquireRemoveLockEx(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID Tag,
    _In_ PCSTR File,
    _In_ ULONG Line
    )
// This function implements the acquisition of Tag
#ifdef _MSC_VER
#pragma warning(suppress:28104)
#endif
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    BOOLEAN rundownAcquired;
    PEX_RUNDOWN_REF_CACHE_AWARE removeLockRundown = NULL;

    //
    // Grab the remove lock
    //

    #if DBG

        LONG lockValue;

        lockValue = InterlockedIncrement(&commonExtension->RemoveLock);


        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_LOCK,  "ClassAcquireRemoveLock: "
                    "Acquired for Object %p & irp %p - count is %d\n",
                    DeviceObject, Tag, lockValue));

        NT_ASSERTMSG("ClassAcquireRemoveLock - lock value was negative : ",
                  (lockValue > 0));

        NT_ASSERTMSG("RemoveLock increased to meet LockHighWatermark",
                  ((LockHighWatermark == 0) ||
                   (lockValue != LockHighWatermark)));

        if (commonExtension->IsRemoved != REMOVE_COMPLETE) {
            PRTL_GENERIC_TABLE removeTrackingList = NULL;
            REMOVE_TRACKING_BLOCK trackingBlock;
            PREMOVE_TRACKING_BLOCK insertedTrackingBlock = NULL;
            BOOLEAN newElement = FALSE;

            KIRQL oldIrql;

            trackingBlock.Tag = Tag;

            trackingBlock.File = File;
            trackingBlock.Line = Line;

            KeQueryTickCount((&trackingBlock.TimeLocked));

            KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                              &oldIrql);

            removeTrackingList = commonExtension->RemoveTrackingList;

            if (removeTrackingList != NULL) {
                insertedTrackingBlock = RtlInsertElementGenericTable(removeTrackingList,
                                                                     &trackingBlock,
                                                                     sizeof(REMOVE_TRACKING_BLOCK),
                                                                     &newElement);
            }

            if (insertedTrackingBlock != NULL) {
                if (!newElement) {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassAcquireRemoveLock: "
                                "already tracking Tag %p\n", Tag));
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassAcquireRemoveLock: "
                                "acquired in file %s on line %d\n",
                                insertedTrackingBlock->File, insertedTrackingBlock->Line));
//                  NT_ASSERT(FALSE);

                }
            } else {
                commonExtension->RemoveTrackingUntrackedCount++;

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_LOCK, ">>>>>ClassAcquireRemoveLock: "
                            "Cannot track Tag %p - currently %d untracked requsts\n",
                            Tag, commonExtension->RemoveTrackingUntrackedCount));
            }

            KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);
        }
    #else

        UNREFERENCED_PARAMETER(Tag);
        UNREFERENCED_PARAMETER(File);
        UNREFERENCED_PARAMETER(Line);

    #endif

    removeLockRundown = (PEX_RUNDOWN_REF_CACHE_AWARE)
        ((PCHAR)commonExtension->PrivateCommonData + sizeof(CLASS_PRIVATE_COMMON_DATA));
    rundownAcquired = ExAcquireRundownProtectionCacheAware(removeLockRundown);
    if (!rundownAcquired) {
        InterlockedIncrement((volatile LONG*) &(commonExtension->PrivateCommonData->RemoveLockFailAcquire));
        TracePrint((TRACE_LEVEL_VERBOSE,
                    TRACE_FLAG_LOCK,
                    "ClassAcquireRemoveLockEx: RemoveLockRundown acquisition failed"
                    "RemoveLockFailAcquire = %d\n",
                    commonExtension->PrivateCommonData->RemoveLockFailAcquire));
    }

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReleaseRemoveLock(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIRP Tag
    )
// This function implements the release of Tag
#ifdef _MSC_VER
#pragma warning(suppress:28103)
#endif
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    LONG lockValue;
    LONG oldValue;
    PEX_RUNDOWN_REF_CACHE_AWARE removeLockRundown = NULL;

    #if DBG
        PRTL_GENERIC_TABLE removeTrackingList = NULL;
        REMOVE_TRACKING_BLOCK searchDataBlock;

        BOOLEAN found = FALSE;

        BOOLEAN isRemoved = (commonExtension->IsRemoved == REMOVE_COMPLETE);

        KIRQL oldIrql;

        if (isRemoved) {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_LOCK, "ClassReleaseRemoveLock: REMOVE_COMPLETE set; this should never happen"));
            InterlockedDecrement(&(commonExtension->RemoveLock));
            return;
        }

        KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock,
                          &oldIrql);

        removeTrackingList = commonExtension->RemoveTrackingList;

        if (removeTrackingList != NULL) {
            searchDataBlock.Tag = Tag;
            found = RtlDeleteElementGenericTable(removeTrackingList, &searchDataBlock);
        }

        if (!found) {
            if(commonExtension->RemoveTrackingUntrackedCount == 0) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassReleaseRemoveLock: "
                            "Couldn't find Tag %p in the lock tracking list\n", Tag));
                //
                // This might happen if the device is being removed and the tracking list
                // has already been freed. Don't assert if that is the case.
                //
                NT_ASSERT((removeTrackingList == NULL) && (commonExtension->IsRemoved != NO_REMOVE));
            } else {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassReleaseRemoveLock: "
                            "Couldn't find Tag %p in the lock tracking list - "
                            "may be one of the %d untracked requests still outstanding\n",
                            Tag, commonExtension->RemoveTrackingUntrackedCount));

                commonExtension->RemoveTrackingUntrackedCount--;
                NT_ASSERT(commonExtension->RemoveTrackingUntrackedCount >= 0);
            }
        }

        KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock,
                          oldIrql);

        lockValue = InterlockedDecrement(&commonExtension->RemoveLock);

        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_LOCK,  "ClassReleaseRemoveLock: "
                    "Released for Object %p & irp %p - count is %d\n",
                    DeviceObject, Tag, lockValue));

        NT_ASSERT(lockValue >= 0);

        NT_ASSERTMSG("RemoveLock decreased to meet LockLowWatermark",
                  ((LockLowWatermark == 0) || !(lockValue == LockLowWatermark)));

        if (lockValue == 0) {

            NT_ASSERT(commonExtension->IsRemoved);

            //
            // The device needs to be removed.  Signal the remove event
            // that it's safe to go ahead.
            //

            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_LOCK,  "ClassReleaseRemoveLock: "
                        "Release for object %p & irp %p caused lock to go to zero\n",
                        DeviceObject, Tag));

        }

    #else

        UNREFERENCED_PARAMETER(Tag);

    #endif

    //
    // Decrement the RemoveLockFailAcquire by 1 when RemoveLockFailAcquire is non-zero.
    // Release the RemoveLockRundown only when RemoveLockFailAcquire is zero.
    //

    oldValue = 1;
    lockValue = commonExtension->PrivateCommonData->RemoveLockFailAcquire;
    while (lockValue != 0) {
        oldValue =
            InterlockedCompareExchange((volatile LONG *) &commonExtension->PrivateCommonData->RemoveLockFailAcquire,
            lockValue - 1,
            lockValue);

        if (oldValue == lockValue) {
            break;
        }

        lockValue = oldValue;
    }

    if (lockValue == 0) {
        removeLockRundown = (PEX_RUNDOWN_REF_CACHE_AWARE)
            ((PCHAR)commonExtension->PrivateCommonData + sizeof(CLASS_PRIVATE_COMMON_DATA));
        ExReleaseRundownProtectionCacheAware(removeLockRundown);
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassCompleteRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ CCHAR PriorityBoost
    )
{
    #if DBG
        PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

        PRTL_GENERIC_TABLE removeTrackingList = NULL;
        REMOVE_TRACKING_BLOCK searchDataBlock;
        PREMOVE_TRACKING_BLOCK foundTrackingBlock;

        KIRQL oldIrql;

        KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock, &oldIrql);

        removeTrackingList = commonExtension->RemoveTrackingList;

        if (removeTrackingList != NULL)
        {
            searchDataBlock.Tag = Irp;

            foundTrackingBlock = RtlLookupElementGenericTable(removeTrackingList, &searchDataBlock);

            if(foundTrackingBlock != NULL) {

                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassCompleteRequest: "
                            "Irp %p completed while still holding the remove lock\n", Irp));
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_LOCK, ">>>>>ClassCompleteRequest: "
                            "Lock acquired in file %s on line %d\n",
                            foundTrackingBlock->File, foundTrackingBlock->Line));
                NT_ASSERT(FALSE);
            }
        }

        KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);
    #endif


    UNREFERENCED_PARAMETER(DeviceObject);

    IoCompleteRequest(Irp, PriorityBoost);
    return;
} // end ClassCompleteRequest()


RTL_GENERIC_COMPARE_RESULTS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RemoveTrackingCompareRoutine(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
{
    PVOID tag1, tag2;

    UNREFERENCED_PARAMETER(Table);

    tag1 = ((PREMOVE_TRACKING_BLOCK)FirstStruct)->Tag;
    tag2 = ((PREMOVE_TRACKING_BLOCK)SecondStruct)->Tag;

    if (tag1 < tag2)
    {
        return GenericLessThan;
    }
    else if (tag1 > tag2)
    {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

PVOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RemoveTrackingAllocateRoutine(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )
{
    UNREFERENCED_PARAMETER(Table);

    return ExAllocatePoolWithTag(NonPagedPoolNx, ByteSize, CLASS_TAG_LOCK_TRACKING);
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RemoveTrackingFreeRoutine(
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    )
{
    UNREFERENCED_PARAMETER(Table);

    FREE_POOL(Buffer);
}

VOID
ClasspInitializeRemoveTracking(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    #if DBG
        KeInitializeSpinLock(&commonExtension->RemoveTrackingSpinlock);

        commonExtension->RemoveTrackingList = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(RTL_GENERIC_TABLE), CLASS_TAG_LOCK_TRACKING);

        if (commonExtension->RemoveTrackingList != NULL)
        {
            RtlInitializeGenericTable(commonExtension->RemoveTrackingList,
                                      RemoveTrackingCompareRoutine,
                                      RemoveTrackingAllocateRoutine,
                                      RemoveTrackingFreeRoutine,
                                      NULL);
        }
    #else

        UNREFERENCED_PARAMETER(DeviceObject);

        commonExtension->RemoveTrackingSpinlock = (ULONG_PTR) -1;
        commonExtension->RemoveTrackingList = NULL;
    #endif
}

VOID
ClasspUninitializeRemoveTracking(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    #if DBG
        PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
        PRTL_GENERIC_TABLE removeTrackingList = commonExtension->RemoveTrackingList;

        ASSERTMSG("Removing the device while still holding remove locks",
                   commonExtension->RemoveTrackingUntrackedCount == 0 &&
                   removeTrackingList != NULL ? RtlNumberGenericTableElements(removeTrackingList) == 0 : TRUE);

        if (removeTrackingList != NULL)
        {
            KIRQL oldIrql;
            KeAcquireSpinLock(&commonExtension->RemoveTrackingSpinlock, &oldIrql);

            FREE_POOL(removeTrackingList);
            commonExtension->RemoveTrackingList = NULL;

            KeReleaseSpinLock(&commonExtension->RemoveTrackingSpinlock, oldIrql);
        }

    #else

        UNREFERENCED_PARAMETER(DeviceObject);
    #endif
}




