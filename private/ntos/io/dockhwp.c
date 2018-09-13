/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dock.c

Abstract:


Author:

    Kenneth D. Ray (kenray) Feb 1998

Revision History:

--*/

#include "iop.h"
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#include "..\config\cmp.h"
#include <string.h>
#include <profiles.h>
#include <wdmguid.h>

#if DBG

#define ASSERT_SEMA_NOT_SIGNALLED(SemaphoreObject) \
    ASSERT(KeReadStateSemaphore(SemaphoreObject) == 0) ;

#else // DBG

#define ASSERT_SEMA_NOT_SIGNALLED(SemaphoreObject)

#endif // DBG

//
// Internal functions to dockhwp.c
//

NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                    * ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT PHANDLE                     NewProfile,
    OUT PBOOLEAN                    ProfileChanged
    );

NTSTATUS
IopUpdateHardwareProfile (
    OUT PBOOLEAN   ProfileChanged
    );

VOID
IopHardwareProfileSendCommit(
    VOID
    );

VOID
IopHardwareProfileSendCancel(
    VOID
    );

//
// List of current dock devices, and the number of dockdevices.
// Must hold IopDockDeviceListLock to change these values.
//
LIST_ENTRY  IopDockDeviceListHead;
ULONG       IopDockDeviceCount;
FAST_MUTEX  IopDockDeviceListLock;
KSEMAPHORE  IopProfileChangeSemaphore;
BOOLEAN     IopProfileChangeCancelRequired;
LONG        IopDocksInTransition;

typedef struct {

    WORK_QUEUE_ITEM WorkItem;
    KEVENT          NotificationCompleted ;
    LPGUID          NotificationGuid ;
    NTSTATUS        FinalStatus ;
    PPNP_VETO_TYPE  VetoType ;
    PUNICODE_STRING VetoName ;

} PROFILE_WORK_ITEM, *PPROFILE_WORK_ITEM ;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopHardwareProfileBeginTransition)
#pragma alloc_text(PAGE, IopHardwareProfileMarkDock)
#pragma alloc_text(PAGE, IopHardwareProfileQueryChange)
#pragma alloc_text(PAGE, IopHardwareProfileCommitStartedDock)
#pragma alloc_text(PAGE, IopHardwareProfileCommitRemovedDock)
#pragma alloc_text(PAGE, IopHardwareProfileCancelRemovedDock)
#pragma alloc_text(PAGE, IopHardwareProfileCancelTransition)
#define alloc_text(PAGE, IopHardwareProfileSendCommit)
#pragma alloc_text(PAGE, IopHardwareProfileSendCancel)
#endif // ALLOC_PRAGMA


VOID
IopHardwareProfileBeginTransition(
    IN BOOLEAN SubsumeExistingDeparture
    )
/*++

Routine Description:

    This routine must be called before any dock devnodes can be marked for
    transition (ie arriving or departing). After calling this function,
    IopHardwareProfileMarkDock should be called for each dock that is appearing
    or disappearing.

    Functionally, this code acquires the profile change semaphore. Future
    changes in the life of the added dock devnodes cause it to be released.

Arguments:

    SubsumeExistingDeparture - Set if we are ejecting the parent of a
                               device that is still in the process of
                               ejecting...

Return Value:

    None.

--*/
{
    NTSTATUS status ;

    if (SubsumeExistingDeparture) {

        //
        // We will already have queried in this case. Also, enumeration is
        // locked right now, so the appropriate devices found cannot disappear.
        // Assert everything is consistant.
        //
        ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
        ASSERT(IopDocksInTransition != 0) ;
        return ;
    }

    //
    // Take the profile change semaphore. We do this whenever a dock is
    // in our list, even if no query is going to occur.
    //
    status = KeWaitForSingleObject(
        &IopProfileChangeSemaphore,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    ASSERT(status == STATUS_SUCCESS) ;
}

VOID
IopHardwareProfileMarkDock(
    PDEVICE_NODE    DeviceNode,
    PROFILE_STATUS  ChangeInPresence
    )
/*++

Routine Description:

    This routine is called to mark a dock as "in transition", ie it is either
    disappearing or appearing, the results of which determine our final
    hardware profile state. After all the docks that are transitioning have
    been passed into this function, IopHardwareProfileQueryChange is called.

Arguments:

    DeviceNode          - The dock devnode that is appearing or disappearing
    ChangeInPresence    - Either DOCK_DEPARTING or DOCK_ARRIVING

Return Value:

    Nope.

--*/
{
    PWCHAR          deviceSerialNumber;
    PDEVICE_OBJECT  deviceObject;
    NTSTATUS        status;

    //
    // Verify we are under semaphore, we aren't marking the dock twice, and
    // our parameters are sensable.
    //
    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT) ;
    ASSERT((ChangeInPresence == DOCK_DEPARTING)||
           (ChangeInPresence == DOCK_ARRIVING)) ;

    if (ChangeInPresence == DOCK_ARRIVING) {

        //
        // First, ensure this dock is a member of the dock list.
        // ADRIAO BUGBUG 11/12/98 -
        //     We should move this into IopProcessNewDeviceNode.
        //
        if (IsListEmpty(&DeviceNode->DockInfo.ListEntry)) {

            //
            // Acquire the lock on the list of dock devices
            //
            ExAcquireFastMutex(&IopDockDeviceListLock);

            //
            // Add this element to the head of the list
            //
            InsertHeadList(&IopDockDeviceListHead,
                           &DeviceNode->DockInfo.ListEntry);
            IopDockDeviceCount++;

            //
            // Release the lock on the list of dock devices
            //
            ExReleaseFastMutex(&IopDockDeviceListLock);
        }

        //
        // Retrieve the Serial Number from this dock device. We do this just
        // to test the BIOS today. Later we will be acquiring the information
        // to determine the profile we are *about* to enter.
        //
        deviceObject = DeviceNode->PhysicalDeviceObject;
        status = IopQueryDeviceSerialNumber(deviceObject,
                                            &deviceSerialNumber);

        if (NT_SUCCESS(status) && (deviceSerialNumber != NULL)) {

            ExFreePool(deviceSerialNumber) ;
        }

    } else {

        //
        // DOCK_DEPARTING case, we must be a member of the dock list...
        //
        ASSERT(!IsListEmpty(&DeviceNode->DockInfo.ListEntry)) ;
    }

    InterlockedIncrement(&IopDocksInTransition) ;
    DeviceNode->DockInfo.DockStatus = ChangeInPresence ;
}

NTSTATUS
IopHardwareProfileQueryChange(
    IN  BOOLEAN                     SubsumingExistingDeparture,
    IN  PROFILE_NOTIFICATION_TIME   InPnpEvent,
    OUT PPNP_VETO_TYPE              VetoType,
    OUT PUNICODE_STRING             VetoName OPTIONAL
    )
/*++

Routine Description:

    This function queries drivers to see if it is OK to exit the current
    hardware profile and enter next one (as determined by which docks have
    been marked). One of three functions should be used subsequently to this
    call:
        IopHardwareProfileCommitStartedDock (call when a dock has successfully
                                             started)
        IopHardwareProfileCommitRemovedDock (call when a dock is no longer
                                             present in the system)
        IopHardwareProfileCancelTransition  (call to abort a transition, say
                                             if a dock failed to start or a
                                             query returned failure for eject)

Arguments:

    InPnpEvent  - This argument indicates whether an operation is being done
                  within the context of another PnpEvent or not. If not, we
                  will queue such an event and block on it. If so, we cannot
                  queue&block (we'd deadlock), so we do the query manually.
    VetoType    - If this function returns false, this parameter will describe
                  who failed the query profile change. The below optional
                  parameter will contain the name of said vetoer.
    VetoName    - This optional parameter will get the name of the vetoer (ie
                  devinst, service name, application name, etc). If VetoName
                  is supplied, the caller must free the buffer returned.

Return Value:

    NTSTATUS.

--*/
{
    PROFILE_WORK_ITEM profileWorkItem;
    NTSTATUS status;
    BOOLEAN arrivingDockFound;
    PLIST_ENTRY listEntry ;
    PDEVICE_NODE devNode ;

    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;

    //
    // Acquire the lock on the list of dock devices and determine whether any
    // dock devnodes are arriving.
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    ASSERT(IopDocksInTransition) ;

    arrivingDockFound = FALSE ;
    for (listEntry  = IopDockDeviceListHead.Flink;
        listEntry != &(IopDockDeviceListHead);
        listEntry  = listEntry->Flink ) {

        devNode = CONTAINING_RECORD(listEntry,
                                    DEVICE_NODE,
                                    DockInfo.ListEntry);

        ASSERT((devNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE)&&
               (devNode->DockInfo.DockStatus != DOCK_EJECTIRP_COMPLETED)) ;

        if (devNode->DockInfo.DockStatus == DOCK_ARRIVING) {

            arrivingDockFound = TRUE ;
        }
    }

    //
    // Release the lock on the list of dock devices
    //
    ExReleaseFastMutex(&IopDockDeviceListLock);

    if (SubsumingExistingDeparture) {

        ASSERT(IopProfileChangeCancelRequired) ;
        //
        // We're nesting. Work off the last query, and don't requery.
        //
        return STATUS_SUCCESS ;
    }

    if (arrivingDockFound) {

        //
        // We currently don't actually query for hardware profile change on a
        // dock event as the user may have the lid closed. If we ever find a
        // piece of hardware that needs to be updated *prior* to actually
        // switching over, we will have to remove this bit of code.
        //
        IopProfileChangeCancelRequired = FALSE ;
        return STATUS_SUCCESS ;
    }

    PIDBGMSG(PIDBG_HWPROFILE, ("NTOSKRNL: Sending HW profile change [query]\n")) ;

    status = IopRequestHwProfileChangeNotification(
        (LPGUID) &GUID_HWPROFILE_QUERY_CHANGE,
        InPnpEvent,
        VetoType,
        VetoName
        );

    if (NT_SUCCESS(status)) {
        IopProfileChangeCancelRequired = TRUE ;
    } else {
        IopProfileChangeCancelRequired = FALSE ;
    }
    return status ;
}

VOID
IopHardwareProfileCommitStartedDock(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine adds the specified device from the list of current dock
    devices and requests a Hardware Profile change.

Arguments:

    DeviceNode  - Supplies a pointer to a device node which will be started and
                  enumerated.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PWCHAR  deviceSerialNumber;
    BOOLEAN profileChanged = FALSE ;

    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_ARRIVING) ;
    ASSERT(!IsListEmpty(&DeviceNode->DockInfo.ListEntry)) ;

    DeviceNode->DockInfo.DockStatus = DOCK_QUIESCENT ;
    InterlockedDecrement(&IopDocksInTransition) ;

    //
    // We only add one dock at a time. So this should have been the last!
    //
    ASSERT(!IopDocksInTransition) ;

    //
    // Retrieve the Serial Number from this dock device
    //
    if (DeviceNode->DockInfo.SerialNumber == NULL) {

        deviceObject = DeviceNode->PhysicalDeviceObject;

        status = IopQueryDeviceSerialNumber(deviceObject,
                                            &deviceSerialNumber);

        DeviceNode->DockInfo.SerialNumber = deviceSerialNumber;

        //
        // Update the current Hardware Profile after successfully starting this
        // device. This routine does two things for us:
        // 1) It determines whether the profile actually changed and updates
        //    the global flag IopProfileChangeOccured appropriately.
        // 2) If the profile changed, this routine updates the registry, but
        //    does *not* broadcast the profile change around.
        //
        status = IopUpdateHardwareProfile(&profileChanged);
        if (!NT_SUCCESS(status)) {

            PIDBGMSG(
                PIDBG_HWPROFILE,
                ("IopUpdateHardwareProfile failed with status == %lx\n", status)
                ) ;
        }

    } else {
        //
        // Couldn't get Serial Number for this dock device, or serial number was NULL
        //
        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status) && profileChanged) {

        IopHardwareProfileSendCommit() ;
        IopProcessNewProfile();

    } else if (IopProfileChangeCancelRequired) {

        IopHardwareProfileSendCancel() ;
    }

    KeReleaseSemaphore(
        &IopProfileChangeSemaphore,
        IO_NO_INCREMENT,
        1,
        FALSE
        );

    return ;
}

VOID
IopHardwareProfileCommitRemovedDock(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine removes the specified device from the list of current dock
    devices and requests a Hardware Profile change.

Arguments:

    DeviceNode - Supplies a pointer to a device node which has been listed as
                 missing in a previous enumeration, has had the final remove IRP
                 sent to it, and is about to be deleted.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    BOOLEAN  profileChanged ;
    LONG     remainingDockCount ;

    //
    // Acquire the lock on the list of dock devices
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    //
    // Since we are about to remove this dock device from the list of
    // all dock devices present, the list should not be empty.
    //
    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    ASSERT((DeviceNode->DockInfo.DockStatus == DOCK_DEPARTING)||
           (DeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)) ;
    ASSERT(!IsListEmpty(&DeviceNode->DockInfo.ListEntry)) ;

    //
    // Remove the current devnode from the list of docks
    //
    RemoveEntryList(&DeviceNode->DockInfo.ListEntry);
    InitializeListHead(&DeviceNode->DockInfo.ListEntry);
    if (DeviceNode->DockInfo.SerialNumber) {

        ExFreePool(DeviceNode->DockInfo.SerialNumber);
        DeviceNode->DockInfo.SerialNumber = NULL;
    }
    IopDockDeviceCount--;

    DeviceNode->DockInfo.DockStatus = DOCK_QUIESCENT ;
    remainingDockCount = InterlockedDecrement(&IopDocksInTransition) ;
    ASSERT(remainingDockCount >= 0) ;

    //
    // Release the lock on the list of dock devices
    //
    ExReleaseFastMutex(&IopDockDeviceListLock);

    if (remainingDockCount) {

        return ;
    }

    //
    // Update the current Hardware Profile after removing this device.
    //
    status = IopUpdateHardwareProfile(&profileChanged);

    if (!NT_SUCCESS(status)) {

        //
        // So we're there physically, but not mentally? Too bad, where broadcasting
        // change either way.
        //
        PIDBGMSG(
            PIDBG_HWPROFILE,
            ("IopUpdateHardwareProfile failed with status == %lx\n", status)
            ) ;

        ASSERT(NT_SUCCESS(status)) ;
    }

    if (NT_SUCCESS(status) && profileChanged) {

        IopHardwareProfileSendCommit() ;
        IopProcessNewProfile();

    } else {

        ASSERT(IopProfileChangeCancelRequired) ;
        IopHardwareProfileSendCancel() ;
    }

    KeReleaseSemaphore(
        &IopProfileChangeSemaphore,
        IO_NO_INCREMENT,
        1,
        FALSE
        );

    return ;
}

VOID
IopHardwareProfileCancelRemovedDock(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine is called when a dock that was marked to disappear didn't (ie,
    after the eject, the dock device still enumerated). We remove it from the
    transition list and complete/cancel the HW profile change as appropriate.
    See IopHardwareProfileSetMarkedDocksEjected.

Arguments:

    DeviceNode - Supplies a pointer to a device node which will be started and
                 enumerated.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    BOOLEAN  profileChanged ;
    LONG     remainingDockCount ;

    //
    // Acquire the lock on the list of dock devices
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    //
    // Since we are about to remove this dock device from the list of
    // all dock devices present, the list should not be empty.
    //
    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED) ;
    ASSERT(!IsListEmpty(&DeviceNode->DockInfo.ListEntry)) ;

    DeviceNode->DockInfo.DockStatus = DOCK_QUIESCENT ;
    remainingDockCount = InterlockedDecrement(&IopDocksInTransition) ;
    ASSERT(remainingDockCount >= 0) ;

    //
    // Release the lock on the list of dock devices
    //
    ExReleaseFastMutex(&IopDockDeviceListLock);

    if (remainingDockCount) {

        return ;
    }

    //
    // Update the current Hardware Profile after removing this device.
    //
    status = IopUpdateHardwareProfile(&profileChanged);

    if (!NT_SUCCESS(status)) {

        //
        // So we're there physically, but not mentally? Too bad, where broadcasting
        // change either way.
        //
        PIDBGMSG(
            PIDBG_HWPROFILE,
            ("IopUpdateHardwareProfile failed with status == %lx\n", status)
            ) ;
        ASSERT(NT_SUCCESS(status)) ;
    }

    if (NT_SUCCESS(status) && profileChanged) {

        IopHardwareProfileSendCommit() ;
        IopProcessNewProfile();

    } else {

        ASSERT(IopProfileChangeCancelRequired) ;
        IopHardwareProfileSendCancel() ;
    }

    KeReleaseSemaphore(
        &IopProfileChangeSemaphore,
        IO_NO_INCREMENT,
        1,
        FALSE
        );

    return ;
}

VOID
IopHardwareProfileCancelTransition(
    VOID
    )
/*++

Routine Description:

    This routine unmarks any marked devnodes (ie, sets them to no change,
    appearing or disappearing), and sends the CancelQueryProfileChange as
    appropriate. Once called, other profile changes can occur.

Arguments:

    None.

Return Value:

    Nodda.

--*/
{
    PLIST_ENTRY  listEntry;
    PDEVICE_NODE devNode;

    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;

    //
    // Acquire the lock on the list of dock devices
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    for (listEntry  = IopDockDeviceListHead.Flink;
        listEntry != &(IopDockDeviceListHead);
        listEntry  = listEntry->Flink ) {

        devNode = CONTAINING_RECORD(listEntry,
                                    DEVICE_NODE,
                                    DockInfo.ListEntry);

        ASSERT((devNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE)&&
               (devNode->DockInfo.DockStatus != DOCK_EJECTIRP_COMPLETED)) ;
        if (devNode->DockInfo.DockStatus != DOCK_QUIESCENT) {

            InterlockedDecrement(&IopDocksInTransition) ;
            devNode->DockInfo.DockStatus = DOCK_QUIESCENT ;
        }
    }

    ASSERT(!IopDocksInTransition) ;

    //
    // Release the lock on the list of dock devices
    //
    ExReleaseFastMutex(&IopDockDeviceListLock);

    if (IopProfileChangeCancelRequired) {

        IopHardwareProfileSendCancel() ;
    }

    //
    // No need to broadcast the cancels here, as IopQueryHardwareProfileChange
    // will have taken care of the cancels for us.
    //
    KeReleaseSemaphore(
        &IopProfileChangeSemaphore,
        IO_NO_INCREMENT,
        1,
        FALSE
        );
}

VOID
IopHardwareProfileSetMarkedDocksEjected(
    VOID
    )
/*++

Routine Description:

    This routine moves any departing devnodes to the ejected state. If any
    subsequent enumeration lists the device as present, we know the eject
    failed and we appropriately cancel that piece of the profile change.
    IopHardwareProfileCancelRemovedDock can only be called after this function
    is called.

Arguments:

    None.

Return Value:

    Nodda.

--*/
{
    PLIST_ENTRY  listEntry;
    PDEVICE_NODE devNode;

    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;

    //
    // Acquire the lock on the list of dock devices
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    for (listEntry  = IopDockDeviceListHead.Flink;
        listEntry != &(IopDockDeviceListHead);
        listEntry  = listEntry->Flink ) {

        devNode = CONTAINING_RECORD(listEntry,
                                    DEVICE_NODE,
                                    DockInfo.ListEntry);

        ASSERT((devNode->DockInfo.DockStatus == DOCK_QUIESCENT)||
               (devNode->DockInfo.DockStatus == DOCK_DEPARTING)) ;
        if (devNode->DockInfo.DockStatus != DOCK_QUIESCENT) {

            devNode->DockInfo.DockStatus = DOCK_EJECTIRP_COMPLETED ;
        }
    }

    //
    // Release the lock on the list of dock devices
    //
    ExReleaseFastMutex(&IopDockDeviceListLock);
}

VOID
IopHardwareProfileSendCommit(
    VOID
    )
/*++

Routine Description:

    This routine (internal to dockhwp.c) simply sends the change complete message.
    We do not wait for this, as it is asynchronous...

Arguments:

    None.

Return Value:

    Nodda.

--*/
{
    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    PIDBGMSG(PIDBG_HWPROFILE, ("NTOSKRNL: Sending HW profile change [commit]\n")) ;

    IopRequestHwProfileChangeNotification(
        (LPGUID) &GUID_HWPROFILE_CHANGE_COMPLETE,
        PROFILE_PERHAPS_IN_PNPEVENT,
        NULL,
        NULL
        );
}

VOID
IopHardwareProfileSendCancel(
    VOID
    )
/*++

Routine Description:

    This routine (internal to dockhwp.c) simply sends the cancel.

Arguments:

    None.

Return Value:

    Nodda.

--*/
{
    PROFILE_WORK_ITEM   profileWorkItem;
    PNP_VETO_TYPE       vetoType;
    NTSTATUS            status;

    ASSERT_SEMA_NOT_SIGNALLED(&IopProfileChangeSemaphore) ;
    PIDBGMSG(PIDBG_HWPROFILE, ("NTOSKRNL: Sending HW profile change [cancel]\n")) ;

    IopRequestHwProfileChangeNotification(
        (LPGUID) &GUID_HWPROFILE_CHANGE_CANCELLED,
        PROFILE_PERHAPS_IN_PNPEVENT,
        NULL,
        NULL
        ) ;
}

NTSTATUS
IopUpdateHardwareProfile (
    OUT PBOOLEAN   ProfileChanged
    )
/*++
Routine Description:

    This routine scans the list of current dock devices, builds a list of serial
    numbers from those devices, and calls for the Hardware Profile to be
    changed, based on that list.

Arguments:

    ProfileChanged - Supplies a variable to receive TRUE if the current hardware
                     profile changes as a result of calling this routine.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY  listEntry;
    PDEVICE_NODE devNode;
    PWCHAR  *profileSerialNumbers, *p;
    HANDLE  hProfileKey=NULL;
    ULONG   len, numProfiles;
    HANDLE  hCurrent, hIDConfigDB;
    UNICODE_STRING unicodeName;

    //
    // Acquire the lock on the list of dock devices
    //
    ExAcquireFastMutex(&IopDockDeviceListLock);

    //
    // Update the flag for Ejectable Docks
    //
    RtlInitUnicodeString(&unicodeName, CM_HARDWARE_PROFILE_STR_DATABASE);
    if(NT_SUCCESS(IopOpenRegistryKey(&hIDConfigDB,
                                     NULL,
                                     &unicodeName,
                                     KEY_READ,
                                     FALSE) )) {

        RtlInitUnicodeString(&unicodeName, CM_HARDWARE_PROFILE_STR_CURRENT_DOCK_INFO);
        if(NT_SUCCESS(IopOpenRegistryKey(&hCurrent,
                                         hIDConfigDB,
                                         &unicodeName,
                                         KEY_READ | KEY_WRITE,
                                         FALSE) )) {

            RtlInitUnicodeString(&unicodeName, REGSTR_VAL_EJECTABLE_DOCKS);
            ZwSetValueKey(hCurrent,
                          &unicodeName,
                          0,
                          REG_DWORD,
                          &IopDockDeviceCount,
                          sizeof(IopDockDeviceCount));
            ZwClose(hCurrent);
        }
        ZwClose(hIDConfigDB);
    }

    if (IopDockDeviceCount == 0) {
        //
        // if there are no dock devices, the list should
        // contain a single null entry, in addition to the null
        // termination.
        //
        numProfiles = 1;
        ASSERT(IsListEmpty(&IopDockDeviceListHead));
    } else {
        numProfiles = IopDockDeviceCount;
        ASSERT(!IsListEmpty(&IopDockDeviceListHead));
    }

    //
    // Allocate space for a null-terminated list of SerialNumber lists.
    //
    len = (numProfiles+1)*sizeof(PWCHAR);
    profileSerialNumbers = ExAllocatePool(NonPagedPool, len);

    if (profileSerialNumbers) {

        p = profileSerialNumbers;

        //
        // Create the list of Serial Numbers
        //
        for (listEntry  = IopDockDeviceListHead.Flink;
             listEntry != &(IopDockDeviceListHead);
             listEntry  = listEntry->Flink ) {

            devNode = CONTAINING_RECORD(listEntry,
                                        DEVICE_NODE,
                                        DockInfo.ListEntry);

            //
            // ADRIAO BUGBUG 11/11/98 -
            //     Is everything in the quiescent state here?
            //
            ASSERT(devNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) ;
            if (devNode->DockInfo.SerialNumber) {
                *p = devNode->DockInfo.SerialNumber;
                p++;
            }
        }

        ExReleaseFastMutex(&IopDockDeviceListLock);

        if (p == profileSerialNumbers) {
            //
            // Set a single list entry to NULL if we look to be in an "undocked"
            // profile
            //
            *p = NULL;
            p++;
        }

        //
        // Null-terminate the list
        //
        *p = NULL;

        numProfiles = (ULONG)(p - profileSerialNumbers);

        //
        // Change the current Hardware Profile based on the new Dock State
        // and perform notification that the Hardware Profile has changed
        //
        status = IopExecuteHardwareProfileChange(HardwareProfileBusTypeACPI,
                                                 profileSerialNumbers,
                                                 numProfiles,
                                                 &hProfileKey,
                                                 ProfileChanged);
        if (hProfileKey) {
            ZwClose(hProfileKey);
        }
        ExFreePool (profileSerialNumbers);

    } else {

        ExReleaseFastMutex(&IopDockDeviceListLock);

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS
IopExecuteHwpDefaultSelect (
    IN  PCM_HARDWARE_PROFILE_LIST ProfileList,
    OUT PULONG ProfileIndexToUse,
    IN  PVOID Context
    )
{
    UNREFERENCED_PARAMETER (Context);

    * ProfileIndexToUse = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                    * ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT PHANDLE                     NewProfile,
    OUT PBOOLEAN                    ProfileChanged
    )


/*++
Routine Description:
    A docking event has occured and now, given a list of Profile Serial Numbers
    that describe the new docking state:
    Transition to the given docking state.
    Set the Current Hardware Profile to based on the new state.
    (Possibly Prompt the user if there is ambiguity)
    Send Removes to those devices that are turned off in this profile,

Arguments:
    Bus - This is the bus that is supplying the hardware profile change
    (currently only HardwareProfileBusTypeAcpi is supported).

    ProfileSerialNumbers - A list of serial numbers (a list of null terminated
    UCHAR lists) representing this new docking state.  These can be listed in
    any order, and form a complete representation of the new docking state
    caused by a docking even on the given bus.  A Serial Number string of "\0"
    represents an "undocked state" and should not be listed with any other
    strings.  This list need not be sorted.

    SerialNumbersCount - The number of serial numbers listed.

    NewProfile - a handle to the registry key representing the new hardware
    profile (IE \CCS\HardwareProfiles\Current".)

    ProfileChanged - set to TRUE if new current profile (as a result of this
    docking event, is different that then old current profile.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    ULONG           len;
    ULONG           tmplen;
    ULONG           i, j;
    PWCHAR          tmpStr;
    UNICODE_STRING  tmpUStr;
    PUNICODE_STRING sortedSerials = NULL;

    PPROFILE_ACPI_DOCKING_STATE dockState = NULL;

    PIDBGMSG(
        PIDBG_HWPROFILE,
        ("Execute Profile (BusType %x), (SerialNumCount %x)\n", Bus, SerialNumbersCount)
        ) ;

    //
    // Sort the list of serial numbers
    //
    len = sizeof (UNICODE_STRING) * SerialNumbersCount;
    sortedSerials = ExAllocatePool (NonPagedPool, len);
    if (NULL == sortedSerials) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }
    for (i = 0; i < SerialNumbersCount; i++) {
        RtlInitUnicodeString (&sortedSerials[i], ProfileSerialNumbers[i]);
    }
    //
    // I do not anticipate getting more than a few serial numbers, and I am
    // just lasy enough to write this comment and use a buble sort.
    //
    for (i = 0; i < SerialNumbersCount; i++) {
        for (j = 0; j < SerialNumbersCount - 1; j++) {
            if (0 < RtlCompareUnicodeString (&sortedSerials[j],
                                             &sortedSerials[j+1],
                                             FALSE)) {
                tmpUStr = sortedSerials[j];
                sortedSerials[j] = sortedSerials[j+1];
                sortedSerials[j+1] = tmpUStr;
            }
        }
    }

    //
    // Construct the DockState ID
    //
    len = 0;
    for (i = 0; i < SerialNumbersCount; i++) {
        len += sortedSerials[i].Length;
    }
    len += sizeof (WCHAR); // NULL termination;

    dockState = (PPROFILE_ACPI_DOCKING_STATE)
                ExAllocatePool (NonPagedPool,
                                len + sizeof (PROFILE_ACPI_DOCKING_STATE));
    // BUGBUG wasted WCHAR here.  Oh well.

    if (NULL == dockState) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }
    for (i = 0, tmpStr = dockState->SerialNumber, tmplen = 0;
         i < SerialNumbersCount;
         i++) {

        tmplen = sortedSerials[i].Length;
        ASSERT (tmplen <= len - ((PCHAR)tmpStr - (PCHAR)dockState->SerialNumber));

        RtlCopyMemory (tmpStr, sortedSerials[i].Buffer, tmplen);
        (PCHAR) tmpStr += tmplen;
    }

    *(tmpStr++) = L'\0';

    ASSERT (len == (ULONG) ((PCHAR) tmpStr - (PCHAR) dockState->SerialNumber));
    dockState->SerialLength = (USHORT) len;

    if ((SerialNumbersCount > 1) || (L'\0' !=  dockState->SerialNumber[0])) {
        dockState->DockingState = HW_PROFILE_DOCKSTATE_DOCKED;
    } else {
        dockState->DockingState = HW_PROFILE_DOCKSTATE_UNDOCKED;
    }


    //
    // Set the new Profile
    //
    switch (Bus) {
    case HardwareProfileBusTypeACPI:

        status = CmSetAcpiHwProfile (dockState,
                                     IopExecuteHwpDefaultSelect,
                                     NULL,
                                     NewProfile,
                                     ProfileChanged);

        ASSERT(NT_SUCCESS(status) || (!(*ProfileChanged))) ;
        break;

    default:
        *ProfileChanged = FALSE ;
        status = STATUS_NOT_SUPPORTED;
        goto Clean;
    }

Clean:
    if (NULL != sortedSerials) {
        ExFreePool (sortedSerials);
    }
    if (NULL != dockState) {
        ExFreePool (dockState);
    }

    return status;
}



//
// Beyond here lie commented out code. This code is a sketch of what we will
// use when we do two-step profile changes (ie, using the serial numbers figure
// out what profile we are *about* to enter, and return a set of all existing
// profiles that we might transition into. Today this code is not used as not
// all BIOS's can tell us what profile we are entering prior to being there.
//

#if 0

struct _IOP_NEW_ACPI_STATE {
    ULONG Blah;
    PCM_HARDWARE_PROFILE_LIST    ProfileList;

} IOP_NEW_ACPI_STATE, *PIOP_NEW_ACPI_STATE;


NTSTATUS
IopProfileAcquisition (
    IN  PCM_HARDWARE_PROFILE_LIST   ProfileList,
    OUT PULONG                      ProfileIndexToUse,
    IN  PIOP_PROFILE_LIST           NewAcpiState
    )
/*++
Routine Description

--*/
{
    PCM_HARDWARE_PROFILE_LIST tmpList;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
    ULONG k;
    ULONG oldSize;
    ULONG newSize;
    BOOLEAN found;

    //
    // We need to add the entries from ProfileList to our new acpi state
    //
    // Use the most inefficient manner possible to see if we already have
    // this profile.  Hopefully there will not be that many profiles around.
    //
    ASSERT (ProfileList->CurrentProfileCount <= ProfileList->MaxProfileCount);
    for (i = 0; i < ProfileList->CurrentProfileCount; i++) {
        found = FALSE;

        for (k = 0; k < NewAcpiState->ProfileList->CurrentProfileCount; k++) {
            if (ProfileList->Profile[i].Id ==
                NewAcpiState->ProfileList->Profile[k]) {
                found = TRUE;
                break;
            }
        }

        if (!found) {
            //
            // Add it
            //
            if (NewAcpiState->MaxProfileCount <= NewAcpiState->CurrentProfileCount) {
                tmpList = NewAcpiState->ProfileList;

                oldSize = sizeof (CM_HARDWARE_PROFILE_LIST)
                        + (sizeof (CM_HARDWARE_PROFILE) *
                           (tmpList->MaxProfileCount - 1));

                newSize = tmpList->MaxProfileCount * 2;
                newSize = sizeof (CM_HARDWARE_PROFILE_LIST)
                        + (sizeof (CM_HAREWARE_PROFILE) * (newSize - 1));

                tmpList = ExAllocatePool (PagedPool, newSize);
                if (NULL == tmpList) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                RtlCopyMemory (tmpList, NewAcpiState->ProfileList, oldSize);
                // NB current count and Max count copied over as well
                tmpList->MaxProfileCount *= 2;

                ExFreePool (NewAcpiState->ProfileList);
                NewAcpiState->ProfileList = tmpList;
            }

            k = NewAcpiState->ProfileList->CurrentProfileCount++;
            NewAcpiState->ProfileList->Profile[k] = ProfileList->Profile[i].
        }

    }



    *ProfileIndexToUse = -1; // Don't select anything yet.
    return status;
}


NTSTATUS
IopFindPossibleProfiles (
    IN PPROFILE_ACPI_DOCKING_STATE * AcpiProfiles,
    IN ULONG PossibleDockingStates,
    OUT PCM_HARDWARE_PROFILE_LIST * ProfileList
    )
/*++
Routine Description:
    Based on the cononical list of possible acpi docking states, determine the
    cononical list of Hardware profiles that might result.

Arguments:
    AcpiProfiles - A list of pointers to AcpiProfiles that could be achieved
    in the next docking state.

    PossibleDockingStates - The total number of AcpiProfiles.

    ProfileList - the cononical list of hardware profiles that could result from
    the given list of acpi docking profiles.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       i;
    IOP_PROFILE_CONTEXT         context;
    PROFILE_ACPI_DOCKING_STATE  acpiDockState;

    context->ProfileList = ExAllocatePool (NonPagedPool,
                                           sizeof (PCM_HARDWARE_PROFILE_LIST));
    if (NULL == context->ProfileList) {
        status = STATUS_INSUFICIENT_RESOURCES;
        goto Clean;
    }
    context->ProfileList->MaxProfileCount = 1;
    context->ProfileList->CurrentProfileCount = 0;

    for (i = 0; i < PossibleDockingStates; i++) {

        status = CmSetAcpiHwProfile (AcpiProfiles[i],
                                     IopProfileAcquisition,
                                     &context);

        if (STATUS_MORE_PROCESSING_REQUIRED == status) {
            status = STATUS_SUCCESS;
        }
        if (!NT_SUCCESS (status)) {
            goto Clean;
        }
    }
    status = STATUS_SUCCESS

    *ProfileList = context->ProfileList;

Clean:

    if (NULL != context->ProfileList) {
        ExFreePool (context->ProfileList);
    }

    if (!NT_SUCCESS(status)) {
        if (possibleProfileList) {
            ExFreePool (possibleProfileList);
        }
    }

    return status;
}

typedef struct _IOP_ACPI_DOCKING_PROFILE_INFO {
    ULONG Blah;
} IOP_ACPI_DOCKING_PROFILE_INFO, * PIOP_ACPI_DOCKING_PROFILE_INFO;

NTSTATUS
IopAcpiQueryDockingProfileEvent (
    IN  PUCHAR    * AcpiDockSerialNumbers,
    IN  ULONG       PossibleSerialNumbers,
    IN  PFOOBAR     DockRelations,
    OUT PIOP_ACPI_DOCKING_PROFILE_INFO * AcpiDockInfo,
    )
/*++
Routine Description:
    Given a list of ACPI serial numbers that might appear once an ACPI docking
    event has completed, determine the cononical list of hardware profiles that
    could result from the docking serial numbers provided.  Query remove all
    devices listed as disabled across all of these profiles.  If all queries
    succeed, then remove all devices.  If not cancel the queries.

    Return success iff all query removes completed successfully.  Otherwise
    fail.

    Do not actually cause the remove of these devices.


Arguments
    AcpiDockSerialNumbers - a list of serial numbers for the possible docks
    that may be attached for this acpi docking event.

    PossibleSerialNumbers - number of elements in that list.

    BUGBUG
    DockRelations - the list of (help me out here I don't really know) which
                    may be removed when this docking state is executed.
    BUGBUG


    AcpiDockInfo - a context parameter that needs to be passed to
    IopAcpiExecuteDockingProfile.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    return status;
}

#endif


