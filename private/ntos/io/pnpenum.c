/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains routines to perform device enumeration

Author:

    Shie-Lin Tzong (shielint) Sept. 5, 1996.

Revision History:

    James Cavalaris (t-jcaval) July 29, 1997.
    Added IopProcessCriticalDeviceRoutine.

--*/

#include "iop.h"
#pragma hdrstop
#include <setupblk.h>

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'nepP')
#endif

typedef struct _DRIVER_LIST_ENTRY DRIVER_LIST_ENTRY, *PDRIVER_LIST_ENTRY;

struct _DRIVER_LIST_ENTRY {
    PDRIVER_OBJECT DriverObject;
    PDRIVER_LIST_ENTRY NextEntry;
};

typedef enum _ADD_DRIVER_STAGE {
    LowerDeviceFilters,
    LowerClassFilters,
    DeviceService,
    UpperDeviceFilters,
    UpperClassFilters,
    MaximumAddStage
} ADD_DRIVER_STAGE;

typedef struct {
    PDEVICE_NODE DeviceNode;

    BOOLEAN LoadDriver;

    PADD_CONTEXT AddContext;

    PDRIVER_LIST_ENTRY DriverLists[MaximumAddStage];
} QUERY_CONTEXT, *PQUERY_CONTEXT;

#if 0
#define ASSERT_INITED(x) \
        ASSERTMSG("DO_DEVICE_INITIALIZING not cleared on device object",       \
                  ((((x)->Flags) & DO_DEVICE_INITIALIZING) == 0))
#else
#if DBG
#define ASSERT_INITED(x) \
        if (((x)->Flags & DO_DEVICE_INITIALIZING) != 0)    \
            DbgPrint("DO_DEVICE_INITIALIZING flag not cleared on DO %#08lx\n", x);
#else
#define ASSERT_INITED(x) /* nothing */
#endif
#endif


NTSTATUS
IopBusCheck(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN LoadDriver,
    IN BOOLEAN AsyncOk
    );

VOID
IopDeviceActionWorker (
    PVOID Context
    );

NTSTATUS
IopEnumerateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTART_CONTEXT StartContext,
    IN BOOLEAN AsyncOk
    );

NTSTATUS
IopCallDriverAddDeviceQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PWCHAR ValueData,
    IN ULONG ValueLength,
    IN PQUERY_CONTEXT Context,
    IN ULONG ServiceType
    );

NTSTATUS
IopProcessCriticalDeviceRoutine(
    IN HANDLE hDevInstance,
    IN PBOOLEAN FoundMatch,
    IN PUNICODE_STRING ServiceName,
    IN PUNICODE_STRING ClassGuid,
    IN PUNICODE_STRING LowerFilters,
    IN PUNICODE_STRING UpperFilters
    );

BOOLEAN
IopProcessCriticalDevice(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
IopProcessNewChildren(
    IN PDEVICE_NODE     DeviceNode,
    IN PSTART_CONTEXT   StartContext
    );

NTSTATUS
IopProcessStartDevicesWorker (
   IN PDEVICE_NODE DeviceNode,
   OUT PVOID Context
   );

USHORT
IopGetBusTypeGuidIndex(
    IN LPGUID busTypeGuid
    );

BOOLEAN
IopFixupDeviceId(
    PWCHAR DeviceId
    );

BOOLEAN
IopFixupIds(
    IN PWCHAR Ids,
    IN ULONG Length
    );

BOOLEAN
IopGetRegistryDwordWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey,
    IN OUT PULONG Value);

PSECURITY_DESCRIPTOR
IopGetRegistrySecurityWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey);

NTSTATUS
IopChangeDeviceObjectFromRegistryProperties(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN HANDLE DeviceClassPropKey,
    IN HANDLE DevicePropKey,
    IN BOOLEAN UsePdoCharacteristics
    );

#if DBG_SCOPE
ULONG PnpEnumDebugLevel = 0;
#define DebugPrint(level, x) \
    if (level <= PnpEnumDebugLevel) { \
        DbgPrint x;                  \
    }
#else
#define DebugPrint(level, x) /* x */
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopBusCheck)
#pragma alloc_text(PAGE, IopEnumerateDevice)
#pragma alloc_text(PAGE, IopProcessNewDeviceNode)
#pragma alloc_text(PAGE, IopCallDriverAddDevice)
#pragma alloc_text(PAGE, IopGetBusTypeGuidIndex)
#pragma alloc_text(PAGE, IopFixupDeviceId)
#pragma alloc_text(PAGE, IopFixupIds)
#pragma alloc_text(PAGE, IopProcessStartDevices)
#pragma alloc_text(PAGE, IopProcessStartDevicesWorker)
#pragma alloc_text(PAGE, IopStartAndEnumerateDevice)
#endif

//
// This flag indicates if the device's InvalidateDeviceRelation is in progress.
// To read or write this flag, callers must get IopPnpSpinlock.
//

BOOLEAN IopEnumerationInProgress = FALSE;
WORK_QUEUE_ITEM IopDeviceEnumerationWorkItem;


//
// Internal constant strings
//

#define DEVICE_PREFIX_STRING                TEXT("\\Device\\")
#define DOSDEVICES_PREFIX_STRING            TEXT("\\DosDevices\\")

NTSTATUS
IopRequestDeviceAction(
    IN PDEVICE_OBJECT DeviceObject      OPTIONAL,
    IN DEVICE_REQUEST_TYPE RequestType,
    IN PKEVENT CompletionEvent          OPTIONAL,
    IN PNTSTATUS CompletionStatus       OPTIONAL
    )

/*++

Routine Description:

    This routine queues a work item to enumerate a device. This is for IO
    internal use only.

Arguments:

    DeviceObject - Supplies a pointer to the device object to be enumerated.
                   if NULL, this is a request to retry resources allocation
                   failed devices.

    Request - the reason for the enumeration.

Return Value:

    NTSTATUS code.

--*/

{
    PPI_DEVICE_REQUEST  request;
    PDEVICE_NODE        deviceNode;
    KIRQL               oldIrql;

    //
    // If this node is ready for enumeration, enqueue it
    //

    request = ExAllocatePool(NonPagedPool, sizeof(PI_DEVICE_REQUEST));

    if (request) {
        //
        // Put this request onto the pending list
        //

        if (DeviceObject) {
            ObReferenceObject(DeviceObject);
        }

        request->DeviceObject = DeviceObject;
        request->RequestType = RequestType;
        request->CompletionEvent = CompletionEvent;
        request->CompletionStatus = (RequestType == ReenumerateBootDevices)? NULL : CompletionStatus;

        InitializeListHead(&request->ListEntry);

        //
        // Insert the  request to the request queue.  If the request queue is
        // not currently being worked on, request a worker thread to start it.
        //

        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

        InsertTailList(&IopPnpEnumerationRequestList, &request->ListEntry);

        if (    RequestType == ReenumerateBootDevices ||
                RequestType == ReenumerateRootDevices) {
            //
            // This is a special request used when booting the system.  Instead
            // of queuing a work item it synchronously calls the work routine.
            //

            IopEnumerationInProgress = TRUE;
            KeClearEvent(&PiEnumerationLock);
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            IopDeviceActionWorker((PVOID)CompletionStatus);

        } else if (PnPBootDriversLoaded && !IopEnumerationInProgress) {
            IopEnumerationInProgress = TRUE;
            KeClearEvent(&PiEnumerationLock);
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            //
            // Queue a work item to do the enumeration
            //

            ExInitializeWorkItem(&IopDeviceEnumerationWorkItem, IopDeviceActionWorker, NULL);
            ExQueueWorkItem(&IopDeviceEnumerationWorkItem, DelayedWorkQueue);
        } else {
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        }
    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


VOID
IopDeviceActionWorker(
    PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine of ZwLoadDriver.
    Its main purpose is to start and enumerate the device controlled by the newly
    loaded drivers.

    Caller must obtain a reference of the *new* device object.

Parameters:

    Context - Supplies a pointer to the BUS_CHECK_WORK_ITEM.

ReturnValue:

    None.

--*/

{
    PPI_DEVICE_REQUEST  request;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_NODE        deviceNode;
    PLIST_ENTRY         entry;
    BOOLEAN             assignResources = FALSE, allocateResources = FALSE;
    BOOLEAN             bootConfigsOK = TRUE, bootProcess = FALSE;
    BOOLEAN             newDevice, moreProcessing;
    START_CONTEXT       startContext;
    KIRQL               oldIrql;
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();

    for (; ;) {

        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
        entry = RemoveHeadList(&IopPnpEnumerationRequestList);
        if (entry == &IopPnpEnumerationRequestList) {
            entry = NULL;
        }
        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

        if (!entry) {

            //
            // BUGBUG
            //
            // This code will get run whenever we remove a device (amongst other
            // situations).  However, if we are trying to start boot devices, we
            // don't want to start other devices.  The startContext should
            // probably be based on some global state that knows what devices
            // should be (and more importantly should not be) started.
            //
            if ((allocateResources && IopResourcesReleased) || assignResources || bootProcess) {

                //
                // Retry resource allocation for the DNF_INSUFFICIENT_RESOURCES devices
                // if there are new resources become available.
                //

                newDevice = TRUE;

                startContext.LoadDriver = PnPBootDriversInitialized;
                startContext.AddContext.GroupsToStart = NO_MORE_GROUP;
                startContext.AddContext.GroupToStartNext = NO_MORE_GROUP;
                startContext.AddContext.DriverStartType = SERVICE_DEMAND_START;

                do {

                    startContext.NewDevice = FALSE;

                    //
                    // Process the whole device tree to assign resources to those devices who
                    // have been successfully added to their drivers.
                    //

                    moreProcessing = IopProcessAssignResources(IopRootDeviceNode, newDevice, bootConfigsOK);

                    //
                    // Process the device subtree to start those devices who have been allocated
                    // resources and waiting to be started.
                    // Note, the IopProcessStartDevices routine may enumerate new devices.
                    //

                    IopProcessStartDevices(IopRootDeviceNode, &startContext);
                    newDevice = startContext.NewDevice;

                } while ((moreProcessing || newDevice) && !bootProcess);

                allocateResources = FALSE;
                assignResources = FALSE;
                bootProcess = FALSE;
                IopResourcesReleased = FALSE;                  // This flag is set on device removal

            } else {

                ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

                if (IsListEmpty(&IopPnpEnumerationRequestList)) {
                    IopEnumerationInProgress = FALSE;
                    KeSetEvent(&PiEnumerationLock, 0, FALSE);
                    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
                    return;
                }

                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            }

            continue;
        }

        request = CONTAINING_RECORD(entry, PI_DEVICE_REQUEST, ListEntry);
        if (request->DeviceObject == NULL) {

            //
            // This is a request to retry resource allocation for the
            // DNF_INSUFFICIENT_RESOURCES or ResourceRequirementsChanged devices
            //

            if (request->RequestType == ReenumerateBootDevices) {

                //
                // Indicate that this is during boot driver initialization phase.
                //

                bootProcess = TRUE;

                //
                // Get whether this driver allows BOOT config assignment at its level.
                //

                if (Context) {

                    bootConfigsOK = *(PBOOLEAN)Context;

                }

            } else if (request->RequestType == ResourceRequirementsChanged) {
                //
                // The device wasn't started when IopResourceRequirementsChanged
                // was called.
                //
                assignResources = TRUE;
            } else {
                //
                // Resources were freed we want to try to satisfy any
                // DNF_INSUFFICIENT_RESOURCES devices.
                //
                allocateResources = TRUE;
            }
            ExFreePool(request);

            //
            // We've set the flags, we'll do the actual work once we've
            // processed any other requests in the queue.
            //
            continue;
        }

        if (request->RequestType == ResourceRequirementsChanged) {

            deviceObject = request->DeviceObject;

            //
            // Enumerate this object
            //

            IopReallocateResources(deviceObject);

        } else if (request->RequestType == StartDevice) {

            PNEW_DEVICE_WORK_ITEM   newDeviceWorkItem;
            PDEVICE_NODE            parentNode;

            deviceObject = request->DeviceObject;
            deviceNode = deviceObject->DeviceObjectExtension->DeviceNode;

            //
            // Start the specific device (reenumeration of parent not required in
            // this case) by first registering it then calling kernel-mode new
            // dev routine (note that this routine is normally called via a work
            // item but we're already in a work item now so call it directly.
            //

            ASSERT(deviceNode);

            //
            // Make sure we don't step on another enumerator's toes
            //

            IopAcquireDeviceTreeLock();

            KeEnterCriticalRegion();
            ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

            parentNode = deviceNode->Parent;
            if (parentNode != NULL) {
                ObReferenceObject(parentNode->PhysicalDeviceObject);
            }

            ExReleaseResource(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();

            IopReleaseDeviceTreeLock();

            if (parentNode == NULL) {
                status = STATUS_UNSUCCESSFUL;
                goto Clean0;
            }

            IopAcquireEnumerationLock(parentNode);

            if (deviceNode->Flags & DNF_STARTED) {

                IopReleaseEnumerationLock(parentNode);
                ObDereferenceObject(parentNode->PhysicalDeviceObject);
                status = STATUS_SUCCESS;
                goto Clean0;
            }

            if ((parentNode->Parent == NULL && parentNode != IopRootDeviceNode) ||
                !(parentNode->Flags & DNF_STARTED) ||
                !(deviceNode->Flags & DNF_ENUMERATED) ||
                IopDoesDevNodeHaveProblem(deviceNode) ||
                (deviceNode->Flags & DNF_ADDED) ||
                deviceNode->LockCount != 0)  {

                //
                // If the parent or child is going away bail now.
                //
                IopReleaseEnumerationLock(parentNode);
                ObDereferenceObject(parentNode->PhysicalDeviceObject);
                status = STATUS_UNSUCCESSFUL;
                goto Clean0;
            }

            IopRestartDeviceNode(deviceNode);

            status = IopProcessNewDeviceNode(deviceNode);

            KeSetEvent( &parentNode->EnumerationMutex, 0, FALSE );

            if (NT_SUCCESS(status) && !IopDoesDevNodeHaveProblem(deviceNode)) {
                PIDBGMSG( PIDBG_EVENTS,
                        ("IopDeviceActionWorker: START_REQUEST - calling IopNewDevice\n"));

                IopNewDevice(deviceObject);
            }

            IopReleaseDeviceTreeLock();

            ObDereferenceObject(parentNode->PhysicalDeviceObject);
        } else {

            PDEVICE_NODE    RootNode, parentNode;

            //
            // Acquire the tree lock so no new removals get processed.
            // Note that devnodes already locked might still get removed while we hold the tree lock.
            //

            ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);

            //
            // Reenumerate the target devnode.
            //

            deviceNode = RootNode = request->DeviceObject->DeviceObjectExtension->DeviceNode;
            while(1) {

                //
                // Validate that the devnode is not already removed.
                //

                KeEnterCriticalRegion();
                ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

                parentNode = deviceNode->Parent;
                if (parentNode) {
                    ObReferenceObject(parentNode->PhysicalDeviceObject);
                }
                ObReferenceObject(deviceNode->PhysicalDeviceObject);

                ExReleaseResource(&PpRegistryDeviceResource);
                KeLeaveCriticalRegion();

                if (parentNode) {

                    PDEVICE_NODE tempNode;

                    IopAcquireEnumerationLock(parentNode);
                    tempNode = deviceNode->Parent;
                    IopReleaseEnumerationLock(parentNode);
                    if (!tempNode) {
                        ObDereferenceObject(parentNode->PhysicalDeviceObject);
                    }
                    parentNode = tempNode;
                }

                if (!parentNode && deviceNode != IopRootDeviceNode) {
                    ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                    break;
                }

                //
                // Make sure that the devnode is ready for enumeration.
                //

                if (!IopDoesDevNodeHaveProblem(deviceNode) &&
                    (deviceNode->Flags & (DNF_ENUMERATED | DNF_STARTED)) == (DNF_ENUMERATED | DNF_STARTED)) {

                    //
                    // Enumerate this object
                    //

                    status = IopBusCheck( deviceNode->PhysicalDeviceObject,
                                          PnPBootDriversInitialized,     // LoadDriver
                                          (BOOLEAN)(request->CompletionEvent != NULL ? FALSE : PnpAsyncOk));
                    if (status == STATUS_PNP_RESTART_ENUMERATION) {

                        if (parentNode) {
                            ObDereferenceObject(parentNode->PhysicalDeviceObject);
                        }
                        ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                        ExReleaseResource(&IopDeviceTreeLock);
                        PpSynchronizeDeviceEventQueue();
                        ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);
                        deviceNode = RootNode;
                        continue;
                    }
                }

                //
                // Process the whole subtree unless specified otherwise.
                //

                if (request->RequestType != ReenumerateDeviceOnly) {

                    if (deviceNode->Child) {

                        PDEVICE_NODE child = deviceNode->Child;

                        if (parentNode) {
                            ObDereferenceObject(parentNode->PhysicalDeviceObject);
                        }
                        ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                        deviceNode = child;

                    } else {

                        while (deviceNode != RootNode) {

                            if (deviceNode->Sibling) {

                                PDEVICE_NODE sibling = deviceNode->Sibling;

                                if (parentNode) {
                                    ObDereferenceObject(parentNode->PhysicalDeviceObject);
                                }
                                ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                                deviceNode = sibling;
                                break;

                            } else {

                                ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                                deviceNode = parentNode;
                                parentNode = deviceNode->Parent;
                                if (parentNode) {
                                    ObReferenceObject(parentNode->PhysicalDeviceObject);
                                }
                            }
                        }
                    }
                }

                //
                // We are done if we are back where we started.
                //

                if (deviceNode == RootNode) {

                    if (parentNode) {
                        ObDereferenceObject(parentNode->PhysicalDeviceObject);
                    }
                    ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                    break;
                }
            }

            //
            // Unlock the tree so removals can proceed.
            //

            ExReleaseResource(&IopDeviceTreeLock);
            status = STATUS_SUCCESS;
        }

Clean0:
        //
        // Done with this enumeration request
        //

        if (request->CompletionStatus) {
            *request->CompletionStatus = status;
        }

        if (request->CompletionEvent) {
            KeSetEvent(request->CompletionEvent, 0, FALSE);
        }
        ObDereferenceObject(request->DeviceObject);
        ExFreePool(request);
    }
}

NTSTATUS
IopBusCheck(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN LoadDriver,
    IN BOOLEAN AsyncOk
    )

/*++

Routine Description:

    This routine performs bus check operation on the specified device/bus.

Arguments:

    DeviceObject - Supplies a pointer to a device object which will be enumerated.

    LoadDriver - Supplies a BOOLEAN value to indicate should a driver be loaded
                 to complete a enumeration.  If false, the enumeration will stop
                 once the device's controlling service is not loaded yet.  This is
                 mainly for boot device driver initialization.

    ParentLockOwned - Specifies if caller already owns the device's parent's lock.

    AsyncOk - Specifies can QueryDeviceRelation be done at Async way.

Return Value:

    None.

--*/

{
    BOOLEAN newDevice;
    PDEVICE_NODE deviceNode, parent = NULL;
    NTSTATUS status;
    START_CONTEXT startContext;

    PAGED_CODE();

    //
    // First get a reference to the PDO to make sure it won't go away.
    //

    ObReferenceObject(DeviceObject);
    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    //
    // Enumerate the specified device node.
    // If any new device node is found, its controlling driver is loaded and
    // its AddDevice entry is called.
    //

    startContext.LoadDriver = LoadDriver;
    startContext.NewDevice = FALSE;
    startContext.AddContext.GroupsToStart = NO_MORE_GROUP;
    startContext.AddContext.GroupToStartNext = NO_MORE_GROUP;
    startContext.AddContext.DriverStartType = SERVICE_DEMAND_START;

    status = IopEnumerateDevice( deviceNode->PhysicalDeviceObject,
                                 &startContext,
                                 AsyncOk);

    if (status != STATUS_PNP_RESTART_ENUMERATION) {

        do {

            startContext.NewDevice = FALSE;

            //
            // Process the whole device tree to assign resources to those devices who
            // have been successfully added to their drivers.
            //

            newDevice = IopProcessAssignResources(deviceNode, FALSE, TRUE);

            //
            // Process the device subtree to start those devices who have been allocated
            // resources and waiting to be started.
            // Note, the IopProcessStartDevices routine may enumerate new devices.
            //

            IopProcessStartDevices(deviceNode, &startContext);
            newDevice |= startContext.NewDevice;

        } while (newDevice);
    }

    ObDereferenceObject(DeviceObject);

    return status;
}

VOID
IopProcessStartDevices(
   IN PDEVICE_NODE DeviceNode,
   IN PSTART_CONTEXT StartContext
   )

/*++

Routine Description:

    This function is used by Pnp manager to start the devices which have been
    allocated resources and waiting to be started.

Parameters:

    DeviceNode - Specifies the device node whose subtree is to be checked for StartDevice.

    StartContext - specifies if new driver should be loaded to complete the enumeration.

Return Value:

    NONE.

--*/
{
    NTSTATUS status;
    PDEVICE_NODE deviceNode, nextDeviceNode;

    PAGED_CODE();

    //
    // Parse the device node subtree to determine which devices need to be started
    //

    ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);
    if (DeviceNode->LockCount == 0) {

        KeWaitForSingleObject( &DeviceNode->EnumerationMutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        deviceNode = DeviceNode->Child;
        while (deviceNode) {
            nextDeviceNode = deviceNode->Sibling;
            status = IopProcessStartDevicesWorker(deviceNode, StartContext);

            if (status == STATUS_PNP_RESTART_ENUMERATION) {

                IopReleaseEnumerationLock(DeviceNode);

                PpSynchronizeDeviceEventQueue();

                ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);
                if (DeviceNode->LockCount == 0) {

                    KeWaitForSingleObject( &DeviceNode->EnumerationMutex,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL );


                    if (!(DeviceNode->Flags & DNF_STARTED)) {
                        break;
                    }

                    deviceNode = DeviceNode->Child;

                    continue;

                } else {

                    ExReleaseResource(&IopDeviceTreeLock);
                    return;

                }
            }

            deviceNode = nextDeviceNode;
        }

        KeSetEvent( &DeviceNode->EnumerationMutex,
                    0,
                    FALSE );
    }

    ExReleaseResource(&IopDeviceTreeLock);
}

NTSTATUS
IopProcessStartDevicesWorker(
   IN PDEVICE_NODE DeviceNode,
   OUT PVOID Context
   )

/*++

Routine Description:

    This function is used by Pnp manager to start the devices which have been allocated
    resources and waiting to be started.

Parameters:

    DeviceNode - Specifies the device node whose subtree is to be checked for StartDevice.

    Context - specifies a pointer to START_CONTEXT to pass start device info.

Return Value:

    TRUE.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    if ( (DeviceNode->Flags & DNF_NEED_QUERY_IDS) ||   // Reported devices are Started but not enumerated
         ((DeviceNode->Flags & DNF_ADDED) &&
          !(DeviceNode->Flags & DNF_START_PHASE) &&
          (DeviceNode->Flags & DNF_HAS_RESOURCE)) ) {

        //
        // If device has been added and resources acquired, we will start it by
        // sending StartDevice irp and enumerate the device.
        //

        status = IopStartAndEnumerateDevice(DeviceNode, (PSTART_CONTEXT)Context);

    } else {

        //
        // Acquire enumeration mutex to make sure its children won't change by
        // someone else.  Note, the current device node is protected by its parent's
        // Enumeration mutex and it won't disappear either.
        //

        ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);

        if (DeviceNode->LockCount == 0) {

            KeWaitForSingleObject( &DeviceNode->EnumerationMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );


            //
            // Recursively mark all of our children deleted.
            //

            IopForAllChildDeviceNodes(DeviceNode, IopProcessStartDevicesWorker, Context);

            //
            // Release the enumeration mutex of the device node.
            //

            KeSetEvent( &DeviceNode->EnumerationMutex,
                        0,
                        FALSE );

        }

        ExReleaseResource(&IopDeviceTreeLock);
    }

    return status;
}

NTSTATUS
IopStartAndEnumerateDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PSTART_CONTEXT StartContext
    )

/*++

Routine Description:

    This routine starts the specified device and enumerates it.

    NOTE: The resources for the device should already been allocated.

Arguments:

    DeviceNode - Supplies a pointer to a device node which will be started and
                 enumerated.

    StartContext - Supplies a pointer to START_CONTEXT structure to control
                 how the start should be handled.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeName;
    PDEVICE_OBJECT deviceObject;
    HANDLE         handle;

    PAGED_CODE();

    //
    // If no driver is loaded or add device failed, don't start it.
    //

    ASSERT((DeviceNode->Flags & DNF_ADDED) &&
           (DeviceNode->Flags & (DNF_HAS_RESOURCE | DNF_NO_RESOURCE_REQUIRED)) &&
           (!(DeviceNode->Flags & DNF_START_PHASE) || (DeviceNode->Flags & DNF_NEED_QUERY_IDS))
           );

    deviceObject = DeviceNode->PhysicalDeviceObject;

    //
    // First start the device, if it hasn't
    //

    if (!(DeviceNode->Flags & DNF_STARTED)) {

        IopStartDevice(deviceObject);

        //
        // ADRIAO BUGBUG 11/11/98 -
        //     Everything in this if clause expects the start call to
        // returns synchronously. When AsyncOk == TRUE functionality is fixed,
        // code in here should be moved into IopStartDevice.
        //
        if (DeviceNode->Flags & DNF_STARTED) {

            IopDeviceNodeCapabilitiesToRegistry(DeviceNode);
            IopQueryDeviceState(deviceObject);
        }
    }

    if (DeviceNode->Flags & DNF_NEED_QUERY_IDS) {

        PWCHAR compatibleIds, hwIds;
        ULONG hwIdLength, compatibleIdLength;

        //
        // If the DNF_NEED_QUERY_IDS is set, the device is a reported device.
        // It should already be started.  We need to enumerate its children and ask
        // the HardwareId and the Compatible ids of the detected device.
        //

        DeviceNode->Flags &= ~DNF_NEED_QUERY_IDS;
        status = IopDeviceObjectToDeviceInstance (deviceObject,
                                                  &handle,
                                                  KEY_READ
                                                  );
        if (NT_SUCCESS(status)) {
            status = IopQueryCompatibleIds(deviceObject,
                                           BusQueryHardwareIDs,
                                           &hwIds,
                                           &hwIdLength);

            if (!NT_SUCCESS(status)) {
                hwIds = NULL;
            }

            status = IopQueryCompatibleIds(deviceObject,
                                           BusQueryCompatibleIDs,
                                           &compatibleIds,
                                           &compatibleIdLength);
            if (!NT_SUCCESS(status)) {
                compatibleIds = NULL;
            }

            if (hwIds || compatibleIds) {
                KeEnterCriticalRegion();
                ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

                if (hwIds) {

                    if (!IopFixupIds(hwIds, hwIdLength)) {
                        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                                      PNP_ERR_BOGUS_ID,
                                      (ULONG_PTR)deviceObject,
                                      (ULONG_PTR)hwIds,
                                      3);
                    }
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_HARDWAREID);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_MULTI_SZ,
                                  hwIds,
                                  hwIdLength
                                  );
                    ExFreePool(hwIds);
                }

                //
                // create CompatibleId value name.  It is a MULTI_SZ,
                //

                if (compatibleIds) {

                    if (!IopFixupIds(compatibleIds, compatibleIdLength)) {
                        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                                      PNP_ERR_BOGUS_ID,
                                      (ULONG_PTR)deviceObject,
                                      (ULONG_PTR)compatibleIds,
                                      4);
                    }

                    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_COMPATIBLEIDS);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_MULTI_SZ,
                                  compatibleIds,
                                  compatibleIdLength
                                  );
                    ExFreePool(compatibleIds);
                }

                ExReleaseResource(&PpRegistryDeviceResource);
                KeLeaveCriticalRegion();
            }
            ZwClose(handle);
        }
    }

    if ((DeviceNode->Flags & DNF_STARTED) &&
        (DeviceNode->Flags & DNF_NEED_ENUMERATION_ONLY)) {

        status = IopEnumerateDevice(deviceObject, StartContext, PnpAsyncOk);
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IopEnumerateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTART_CONTEXT StartContext,
    IN BOOLEAN AsyncOk
    )

/*++

Routine Description:

    This function assumes that the specified physical device object is
    a bus and will enumerate all of the children PDOs on the bus.

Arguments:

    DeviceObject - Supplies a pointer to the physical device object to be
                   enumerated.

    StartContext - supplies a pointer to the START_CONTEXT to control how to
                   add/start new devices.

    AsyncOk - Specifies can QueryDeviceRelation be done at Async way.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;
    PDEVICE_NODE childDeviceNode, nextChildDeviceNode;
    PDEVICE_OBJECT childDeviceObject;
    PDEVICE_RELATIONS deviceRelations;
    ULONG i;
    BOOLEAN childRemoved, newChildPostponed;

    PAGED_CODE();

    //
    // First get a reference to the PDO to make sure it won't go away.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    if (deviceNode->Flags & DNF_NEED_ENUMERATION_ONLY) {

        //
        // This means this device was just started and needed enumeration.
        // So, we will report the device arrival.
        //

        deviceNode->Flags &= ~DNF_NEED_ENUMERATION_ONLY;
        //
        // The device has been started, attempt to enumerate the device.
        //

        PpSetPlugPlayEvent( &GUID_DEVICE_ARRIVAL,
                            deviceNode->PhysicalDeviceObject);

        IOP_DIAG_THROW_CHAFF_AT_STARTED_PDO_STACK(DeviceObject);
#if DBG
        {

            IO_STACK_LOCATION irpSp;
            ULONG dummy;

            //
            // Initialize the stack location to pass to IopSynchronousCall()
            //

            RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

            //
            // Set the function codes.
            //

            irpSp.MajorFunction = IRP_MJ_PNP;
            irpSp.MinorFunction = 0xff;

            //
            // Make the call and return.
            //

            status = IopSynchronousCall(DeviceObject, &irpSp, (PVOID)&dummy);
            if (NT_SUCCESS(status) || dummy != 0) {
                if (deviceNode->ServiceName.Buffer) {
                    DbgPrint("*** BugBug : Driver %wZ returned status = %lx and Information = %lx\n",
                             &deviceNode->ServiceName, status, dummy);
                    DbgPrint("    for IRP_MN_BOGUS.  ");
                    ASSERT(0);
                } else {
                    DbgPrint("*** BugBug : Driver returned status = %lx and Information = %lx\n", status, dummy);
                    DbgPrint("    for IRP_MN_BOGUS.  ");
                    ASSERT(0);
                }
            }
        }

#endif
    }

    //
    // Make sure we don't step on another enumerator's toes
    //

    IopAcquireEnumerationLock(deviceNode);

    if ((deviceNode->Flags & (DNF_STARTED | DNF_REMOVE_PENDING_CLOSES)) != DNF_STARTED) {
        status = STATUS_UNSUCCESSFUL;
    } else if (deviceNode->Flags & DNF_ENUMERATION_REQUEST_PENDING) {
        if (!(deviceNode->Flags & DNF_BEING_ENUMERATED)) {

            //
            // The enumeration lock of the device must be acquired already
            // before performing following instructions.
            //

            deviceRelations = deviceNode->OverUsed1.PendingDeviceRelations;
            deviceNode->OverUsed1.PendingDeviceRelations = NULL;
            deviceNode->Flags &= ~DNF_ENUMERATION_REQUEST_PENDING;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
    } else {
        status = IopQueryDeviceRelations(BusRelations, DeviceObject, AsyncOk, &deviceRelations);
    }
    if (!NT_SUCCESS(status) || (status == STATUS_PENDING) || !deviceRelations) {
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // Walk all the child device nodes and mark them as not present
    //

    childDeviceNode = deviceNode->Child;
    while (childDeviceNode) {
        childDeviceNode->Flags &= ~DNF_ENUMERATED;
        childDeviceNode = childDeviceNode->Sibling;
    }

    //
    // Check all the PDOs returned see if any new one or any one disappeared.
    //

    for (i = 0; i < deviceRelations->Count; i++) {

        childDeviceObject = deviceRelations->Objects[i];

        ASSERT_INITED(childDeviceObject);

        if (childDeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_DELETE_PENDING) {

            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                          PNP_ERR_PDO_ENUMERATED_AFTER_DELETION,
                          (ULONG_PTR)childDeviceObject,
                          0,
                          0);
        }

        //
        // We've found another physical device, see if there is
        // already a devnode for it.
        //

        childDeviceNode = (PDEVICE_NODE)childDeviceObject->DeviceObjectExtension->DeviceNode;
        if (childDeviceNode == NULL) {

            //
            // Device node doesn't exist, create one.
            //

            childDeviceNode = IopAllocateDeviceNode(childDeviceObject);

            if (childDeviceNode != NULL) {

                //
                // We've found or created a devnode for the PDO that the
                // bus driver just enumerated.
                //

                childDeviceNode->Flags |= DNF_ENUMERATED;

                //
                // Mark the device object a bus enumerated device
                // BUGBUG - should be an ASSERT.  This should be set by bus drivers.
                //

                childDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

                //
                // Put this new device node at the head of the parent's list
                // of children.
                //

                IopInsertTreeDeviceNode (
                    deviceNode,
                    childDeviceNode
                    );

            } else {

                //
                // Had a problem creating a devnode.  Pretend we've never
                // seen it.
                //
                KdPrint(("IopEnumerateDevice: Failed to allocate device node space\n"));
                ObDereferenceObject(childDeviceObject);
            }
        } else {

            //
            // The device is alreay enumerated.  Remark it and release the
            // device object reference.
            //
            childDeviceNode->Flags |= DNF_ENUMERATED;

            if (childDeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED) {

                //
                // A dock that was listed as departing in an eject relation
                // didn't actually leave. Remove it from the profile transition
                // list...
                //
                IopHardwareProfileCancelRemovedDock(childDeviceNode);
            }

            ASSERT(!(childDeviceNode->Flags & DNF_DEVICE_GONE));

            ObDereferenceObject(childDeviceObject);
        }
    }

    ExFreePool(deviceRelations);

    //
    // If we get here, the enumeration was successful.  First process
    // any missing devnodes.
    //

    childRemoved = FALSE;

    for (childDeviceNode = deviceNode->Child;
         childDeviceNode != NULL;
         childDeviceNode = nextChildDeviceNode) {

        //
        // First, we need to remember the 'next child' because the 'child' will be
        // removed and we won't be able to find the 'next child.'
        //

        nextChildDeviceNode = childDeviceNode->Sibling;

        if (!(childDeviceNode->Flags & DNF_ENUMERATED)) {

            if (!(childDeviceNode->Flags & DNF_DEVICE_GONE)) {

                childDeviceNode->Flags |= DNF_DEVICE_GONE;

                IopRequestDeviceRemoval( childDeviceNode->PhysicalDeviceObject,
                                         CM_PROB_DEVICE_NOT_THERE);

                childRemoved = TRUE;
            }
        }
    }

    //
    // BUGBUG - currently the root enumerator gets confused if we reenumerate it
    // before we process newly reported PDOs.  Since it can't possibly create
    // the scenario we are trying to fix, we won't bother waiting for the
    // removes to complete before processing the new devnodes.
    //

    if (deviceNode->Parent != NULL && childRemoved) {

        status = STATUS_PNP_RESTART_ENUMERATION;

        goto exit;
    }

    //
    // Reserve legacy resources for the legacy interface and bus number.
    //

    if (!IopBootConfigsReserved && deviceNode->InterfaceType != InterfaceTypeUndefined) {

        //
        // EISA = ISA.
        //

        if (deviceNode->InterfaceType == Isa) {

            IopReserveLegacyBootResources(Eisa, deviceNode->BusNumber);

        }

        IopReserveLegacyBootResources(deviceNode->InterfaceType, deviceNode->BusNumber);

    }

    //
    // Now process new devnodes that have just appeared.
    //
    // Walk all of the children to perform driver loading and device addition operations.
    //

    IopProcessNewChildren(deviceNode, StartContext);

    status = STATUS_SUCCESS;

exit:

    IopReleaseEnumerationLock(deviceNode);
    ObDereferenceObject(DeviceObject);

    return status;
}

NTSTATUS
IopProcessNewChildren(
    IN PDEVICE_NODE     DeviceNode,
    IN PSTART_CONTEXT   StartContext
    )
{
    NTSTATUS        status;
    PDEVICE_NODE    childDeviceNode;

    for (childDeviceNode = DeviceNode->Child;
         childDeviceNode != NULL;
         childDeviceNode = childDeviceNode->Sibling) {


        if (childDeviceNode->Flags & DNF_ENUMERATED) {

            //
            // Make sure they aren't resurrecting dead PDOs.
            //

            ASSERT(!(childDeviceNode->Flags & DNF_DEVICE_GONE));

            //
            // If the device is not processed ...
            //

            if (!(childDeviceNode->Flags & DNF_PROCESSED)) {

                //
                // Setup registry key for the newly enumerated device
                //

                IopProcessNewDeviceNode(childDeviceNode);
            }

            if (OK_TO_ADD_DEVICE(childDeviceNode)) {
                status = IopCallDriverAddDevice(childDeviceNode,
                                                StartContext->LoadDriver,
                                                &StartContext->AddContext);

                if (NT_SUCCESS(status)) {
                    StartContext->NewDevice = TRUE;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopProcessNewDeviceNode(
    IN OUT PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This function creates a device instance key for the specified device.
    If LoadDriver is true and the device driver for the device is installed,
    this routine will load the driver to start enumerating its children.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be processed.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PWCHAR deviceId, busId, uniqueId, compatibleIds, id, hwIds, deviceText, globallyUniqueId;
    HANDLE handle, enumHandle, busIdHandle, deviceIdHandle;
    HANDLE uniqueIdHandle, logConfHandle;
    UNICODE_STRING unicodeName, unicodeString, unicodeDeviceInstance;
    ULONG disposition, tmpValue, length, cmLength, ioLength, hwIdLength, compatibleIdLength;
    PCM_RESOURCE_LIST cmResource;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResource;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PWCHAR buffer = NULL;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    IO_STACK_LOCATION irpSp;
    DEVICE_CAPABILITIES capabilities;
    PVOID dummy = NULL;
    BOOLEAN globallyUnique = FALSE;
    PWCHAR location = NULL, description = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    UCHAR CLSIDBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + 128];

    BOOLEAN processCriticalDevice = FALSE;
    BOOLEAN isRemoteBootCard = FALSE;
    BOOLEAN configuredBySetup;
    PWCHAR wp;
    GUID busTypeGuid;

    PAGED_CODE();

    deviceObject = DeviceNode->PhysicalDeviceObject;

    //
    // First open HKLM\System\CCS\Enum key.
    //

    status = IopOpenRegistryKeyEx( &enumHandle,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_ALL_ACCESS
                                   );
    if (!NT_SUCCESS(status)) {
        KdPrint(("IopProcessNewDeviceNode: Unable to open HKLM\\SYSTEM\\CCS\\ENUM\n"));
        return status;
    }

    //
    // First, get the device id and this will be the device key name
    //

    status = IopQueryDeviceId(deviceObject, &id);

    if (!NT_SUCCESS(status) || id == NULL) {
        ZwClose(enumHandle);
        return status;
    }

    //
    // Fix up the id if necessary
    //
    if (!IopFixupDeviceId(id)) {
        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                      PNP_ERR_BOGUS_ID,
                      (ULONG_PTR)deviceObject,
                      (ULONG_PTR)id,
                      1);


    }

    //
    // Extract  bus id out of the returned id
    //

    for (wp = id; *wp != UNICODE_NULL; wp++) {
        if (*wp == OBJ_NAME_PATH_SEPARATOR) {
            deviceId = wp + 1;
            busId = id;
            break;
        }
    }

    if (*wp != OBJ_NAME_PATH_SEPARATOR) {
        ZwClose(enumHandle);
        ExFreePool(id);
        KdPrint(("IopProcessNewDevice: Invalid device id return by driver (not in bus\\device format)\n"));
        return STATUS_UNSUCCESSFUL;
    }

    *wp = UNICODE_NULL;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    //
    // Open/Create enumerator key under HKLM\CCS\System\Enum
    //

    RtlInitUnicodeString(&unicodeName, busId);

    status = IopCreateRegistryKeyEx( &busIdHandle,
                                     enumHandle,
                                     &unicodeName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {
        ExFreePool(id);
        ZwClose(enumHandle);
        goto exit;
    }

    //
    // Open/create this registry path under HKLM\CCS\System\Enum\<Enumerator>
    //

    RtlInitUnicodeString(&unicodeName, deviceId);
    status = IopCreateRegistryKeyEx( &deviceIdHandle,
                                     busIdHandle,
                                     &unicodeName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    ZwClose(busIdHandle);
    if (!NT_SUCCESS(status)) {
        ExFreePool(id);
        ZwClose(enumHandle);
        goto exit;
    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    //
    // Query the device's capabilities
    // we will add this stuff to registry once we've processed it a bit
    //

    status = IopQueryDeviceCapabilities(DeviceNode, &capabilities);

    if (capabilities.NoDisplayInUI) {

        DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
    }

    //
    // From the query capabilities call, determine if this a globally unique ID?
    //

    if (NT_SUCCESS(status) && (capabilities.UniqueID)) {
        globallyUnique = TRUE;
    }

    //
    // Record, is this a dock?
    //
    DeviceNode->DockInfo.DockStatus =
        capabilities.DockDevice ? DOCK_QUIESCENT : DOCK_NOTDOCKDEVICE;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Query the device's description.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_TEXT;
    irpSp.Parameters.QueryDeviceText.DeviceTextType = DeviceTextDescription;
    irpSp.Parameters.QueryDeviceText.LocaleId = PsDefaultSystemLocaleId;
    status = IopSynchronousCall(deviceObject, &irpSp, &description);

    if (!NT_SUCCESS(status)) {
        description = NULL;
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Query the device's location information.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_TEXT;
    irpSp.Parameters.QueryDeviceText.DeviceTextType = DeviceTextLocationInformation;
    irpSp.Parameters.QueryDeviceText.LocaleId = PsDefaultSystemLocaleId;
    status = IopSynchronousCall(deviceObject, &irpSp, &location);

    if (!NT_SUCCESS(status)) {
        location = NULL;
    }

    //
    // Query the unique id for the device
    //

    IopQueryUniqueId(deviceObject, &uniqueId);

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

    if (!globallyUnique && deviceNode->Parent != IopRootDeviceNode) {
        globallyUniqueId = NULL;

        status = IopMakeGloballyUniqueId(deviceObject, uniqueId, &globallyUniqueId);

        if (uniqueId != NULL) {
            ExFreePool(uniqueId);
        }

        uniqueId = globallyUniqueId;

    } else {

        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status) || uniqueId == NULL) {
        if (description) {
            ExFreePool(description);
        }
        if (location) {
            ExFreePool(location);
        }
        ZwClose(deviceIdHandle);
        ZwClose(enumHandle);
        ExFreePool(id);
        return status;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

RetryDuplicateId:

    //
    // Fixup the unique instance id if necessary
    //
    if (!IopFixupDeviceId(uniqueId)) {
        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                      PNP_ERR_BOGUS_ID,
                      (ULONG_PTR)deviceObject,
                      (ULONG_PTR)uniqueId,
                      2);
    }

    length = (wcslen(busId) + wcslen(deviceId) + wcslen(uniqueId) + 5) * sizeof(WCHAR);
    buffer = (PWCHAR)ExAllocatePool(PagedPool, length);
    if (!buffer) {

        ExReleaseResource(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        if (description) {
            ExFreePool(description);
        }
        if (location) {
            ExFreePool(location);
        }
        ZwClose(deviceIdHandle);
        ZwClose(enumHandle);
        ExFreePool(id);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    swprintf(buffer, L"%s\\%s\\%s", busId, deviceId, uniqueId);
    RtlInitUnicodeString(&unicodeDeviceInstance, buffer);

    if (DeviceNode->InstancePath.Buffer != NULL) {

        ExFreePool(DeviceNode->InstancePath.Buffer);
        RtlInitUnicodeString(&DeviceNode->InstancePath, NULL);
    }

    IopConcatenateUnicodeStrings(&DeviceNode->InstancePath, &unicodeDeviceInstance, NULL);

    //
    // Open/create this registry device instance path under
    // HKLM\System\Enum\<Enumerator>\deviceId
    //

    RtlInitUnicodeString(&unicodeName, uniqueId);
    status = IopCreateRegistryKeyEx( &uniqueIdHandle,
                                     deviceIdHandle,
                                     &unicodeName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     &disposition
                                     );

    if (!NT_SUCCESS(status)) {
        ZwClose(enumHandle);
        ExFreePool(id);
        ZwClose(deviceIdHandle);
        ExFreePool(uniqueId);
        goto exit;
    }

    deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;

    DeviceNode->Flags |= DNF_PROCESSED;

    //
    // Check if the device instance is already reported.  If yes fail this request.
    //

    if (disposition != REG_CREATED_NEW_KEY) {

        PDEVICE_OBJECT dupCheckDeviceObject;

        //
        // Retrieve the device node associated with this device instance name (if
        // there is one).  If it's different from the device node we're currently
        // working with, then we have a duplicate, and we want to remove it.
        //

        dupCheckDeviceObject = IopDeviceObjectFromDeviceInstance(uniqueIdHandle, NULL);

        if (dupCheckDeviceObject) {

            //
            // Go ahead and dereference the device object now--we only need the
            // value for comparison.
            //

            ObDereferenceObject(dupCheckDeviceObject);

            if (dupCheckDeviceObject != deviceObject) {

                if (globallyUnique) {
                    globallyUnique = FALSE;

                    IopMakeGloballyUniqueId(deviceObject, uniqueId, &globallyUniqueId);

                    if (uniqueId != NULL) {
                        ExFreePool(uniqueId);
                    }

                    uniqueId = globallyUniqueId;

                    ExFreePool(buffer);
                    buffer = NULL;
                    ZwClose(uniqueIdHandle);
                    IopSetDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART);
                    goto RetryDuplicateId;
                }

                KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                              PNP_ERR_DUPLICATE_PDO,
                              (ULONG_PTR)deviceObject,
                              (ULONG_PTR)dupCheckDeviceObject,
                              0);

#if 0
                ZwClose(enumHandle);
                ZwClose(uniqueIdHandle);

                if (DeviceNode->InstancePath.Length != 0) {
                    ExFreePool(DeviceNode->InstancePath.Buffer);
                    DeviceNode->InstancePath.Length = 0;
                    DeviceNode->InstancePath.Buffer = NULL;
                }

                IopRequestDeviceRemoval(deviceObject, CM_PROB_DEVICE_NOT_THERE);
                goto exit;
#endif
            }
        }
    } else {

        if (description) {
            PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_DEVDESC);
            ZwSetValueKey(uniqueIdHandle,
                            &unicodeName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            description,
                            (wcslen(description)+1) * sizeof(WCHAR)
                            );
            ExFreePool(description);
            description = NULL;
        }
    }

    ExFreePool(id);
    ZwClose(deviceIdHandle);
    ExFreePool(uniqueId);

    if (location) {
        PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_LOCATION_INFORMATION);
        ZwSetValueKey(uniqueIdHandle,
                      &unicodeName,
                      TITLE_INDEX_VALUE,
                      REG_SZ,
                      location,
                      (wcslen(location)+1) * sizeof(WCHAR)
                      );
        ExFreePool(location);
        location = NULL;
    }

    //
    // now add the capabilities and UI_NUMBER into registry
    //
    status = IopDeviceCapabilitiesToRegistry(DeviceNode, &capabilities);

#if DBG
    ASSERT(status == STATUS_SUCCESS);
#endif

    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
    status = IopCreateRegistryKeyEx( &logConfHandle,
                                     uniqueIdHandle,
                                     &unicodeName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );
    if (!NT_SUCCESS(status)) {
        logConfHandle = NULL;          // just to make sure
    }

    if (disposition == REG_CREATED_NEW_KEY) {    // disposition from uniqueId

        //
        // "new registry key" device installation case
        //
        // Set flags to control whether device installation needs to
        // happen later. This means setting the devnode flag to
        // DNF_NOT_CONFIGURED and setting the ConfigFlag ONLY if it's
        // raw.
        //

        if (!IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {
            if (capabilities.RawDeviceOK) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);
                tmpValue = CONFIGFLAG_FINISH_INSTALL;
                ZwSetValueKey(uniqueIdHandle,
                                &unicodeName,
                                TITLE_INDEX_VALUE,
                                REG_DWORD,
                                &tmpValue,
                                sizeof(tmpValue)
                                );
            } else {
                IopSetDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED);
            }

            //
            // Process this as a critical device node.  This will setup
            // the service, if necessary, so that the system can boot far
            // enough to get to the config manager
            //

            processCriticalDevice = TRUE;
        }

    } else {

        UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];
        PKEY_VALUE_PARTIAL_INFORMATION keyInfo =
            (PKEY_VALUE_PARTIAL_INFORMATION) buffer;

        ULONG length;

        UNICODE_STRING valueName;

        NTSTATUS tmpStatus;

        if (!IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {

            RtlInitUnicodeString(&valueName, REGSTR_VALUE_CONFIG_FLAGS);
            tmpStatus = ZwQueryValueKey(uniqueIdHandle,
                                        &valueName,
                                        KeyValuePartialInformation,
                                        keyInfo,
                                        sizeof(buffer),
                                        &length);

            if (NT_SUCCESS(tmpStatus)) {

                ULONG configFlags = *(PULONG)keyInfo->Data;

                //
                // The ConfigFlags value exists in the registry
                // If DNF_REINSTALL is set.  We mark it as DNF_DELETED such that we will not
                // start the device.  Later when the reinstallation completes, the
                // DNF_RESTART_OK bit will be set and DNF_DELETED will be deleted.
                //

                if (configFlags & CONFIGFLAG_REINSTALL) {
                    IopSetDevNodeProblem(DeviceNode, CM_PROB_REINSTALL);
                    processCriticalDevice = TRUE;  // to install critical driver
                } else if (configFlags & CONFIGFLAG_FAILEDINSTALL) {
                    IopSetDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL);
                    processCriticalDevice = TRUE;  // to install critical driver
                }
            } else {
                //
                // The ConfigFlag value does not exist in the registry
                //
                IopSetDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED);
            }
        }

        RtlInitUnicodeString(&valueName, REGSTR_VALUE_SERVICE);

        tmpStatus = ZwQueryValueKey(uniqueIdHandle,
                                    &valueName,
                                    KeyValuePartialInformation,
                                    keyInfo,
                                    sizeof(buffer),
                                    &length);

        //
        // if there's no service setup then check to see if this should
        // be processed as a critical device.
        //

        if (NT_SUCCESS(tmpStatus) && (keyInfo->DataLength <= sizeof(L'\0'))) {
            processCriticalDevice = TRUE;
        } else if (tmpStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
            processCriticalDevice = TRUE;
        }
    }

    if (capabilities.HardwareDisabled &&
        !IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) &&
        !IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {
        //
        // mark the node as hardware disabled, if no configuration problems
        //
        IopClearDevNodeProblem(DeviceNode);
        IopSetDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED);
        //
        // Issue a PNP REMOVE_DEVICE Irp so when we query resources
        // we have those required after boot
        //
        status = IopRemoveDevice(deviceObject, IRP_MN_REMOVE_DEVICE);
        ASSERT(NT_SUCCESS(status));

    } else {
        //
        // these are the only problems I expect at this point
        //
        ASSERT(!IopDoesDevNodeHaveProblem(DeviceNode) ||
               IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
               IopIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL) ||
               IopIsDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL) ||
               IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART));
    }

    //
    // Create all the default value entry for the newly created key.
    // Configuration = REG_RESOURCE_LIST
    // ConfigurationVector = REG_RESOUCE_REQUIREMENTS_LIST
    // HardwareID = MULTI_SZ
    // CompatibleIDs = MULTI_SZ
    // Create "Control" volatile subkey.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopCreateRegistryKeyEx( &handle,
                                     uniqueIdHandle,
                                     &unicodeName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_VOLATILE,
                                     NULL
                                     );
    if (NT_SUCCESS(status)) {

        //
        // Write DeviceObject reference ...
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REFERENCE);
        status = ZwSetValueKey( handle,
                                &unicodeName,
                                TITLE_INDEX_VALUE,
                                REG_DWORD,
                                (PULONG_PTR)&deviceObject,
                                sizeof(ULONG_PTR)
                                );
        ZwClose(handle);
    }

    if (!NT_SUCCESS(status)) {
        ZwClose(enumHandle);
        ZwClose(uniqueIdHandle);
        if (logConfHandle) {
            ZwClose(logConfHandle);
        }
        goto exit;
    }

    ZwClose(enumHandle);

    //
    // Release registry lock before calling device driver
    //

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    status = IopQueryCompatibleIds( deviceObject,
                                    BusQueryHardwareIDs,
                                    &hwIds,
                                    &hwIdLength);

    if (!NT_SUCCESS(status)) {
        hwIds = NULL;
    }

    status = IopQueryCompatibleIds( deviceObject,
                                    BusQueryCompatibleIDs,
                                    &compatibleIds,
                                    &compatibleIdLength);
    if (!NT_SUCCESS(status)) {
        compatibleIds = NULL;
    }

    //
    // Query the device's basic config vector. This needs to be done before we check if
    // this is a remote BOOT card.
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_RESOURCE_REQUIREMENTS;
    status = IopSynchronousCall(deviceObject, &irpSp, &ioResource);

    if (!NT_SUCCESS(status)) {
        ioResource = NULL;
    }
    if (ioResource) {
        ioLength = ioResource->ListSize;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    //
    // Write resource requirements to registry
    //

    if (logConfHandle) {
        PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
        if (ioResource) {
            ZwSetValueKey(logConfHandle,
                            &unicodeName,
                            TITLE_INDEX_VALUE,
                            REG_RESOURCE_REQUIREMENTS_LIST,
                            ioResource,
                            ioLength
                            );
            DeviceNode->ResourceRequirements = ioResource;
            DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
        } else {
            ZwDeleteValueKey(logConfHandle, &unicodeName);
        }
    }

    //
    // While we have the hwIds, check if this is the device node
    // for the remote boot net card. If IopLoaderBlock is NULL
    // then we are not initilizing boot drivers so we don't have
    // to check for this.
    //

    if (IoRemoteBootClient && (IopLoaderBlock != NULL)) {

        if (hwIds) {
            isRemoteBootCard = IopIsRemoteBootCard(
                                    DeviceNode,
                                    (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                                    hwIds);
        }
        if (!isRemoteBootCard && compatibleIds) {
            isRemoteBootCard = IopIsRemoteBootCard(
                                    DeviceNode,
                                    (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                                    compatibleIds);
        }
    }

    //
    // create HardwareId value name.  It is a MULTI_SZ,
    //

    if (hwIds) {

        if (!IopFixupIds(hwIds, hwIdLength)) {
            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                          PNP_ERR_BOGUS_ID,
                          (ULONG_PTR)deviceObject,
                          (ULONG_PTR)hwIds,
                          3);
        }
        PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_HARDWAREID);
        ZwSetValueKey(uniqueIdHandle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_MULTI_SZ,
                        hwIds,
                        hwIdLength
                        );
        ExFreePool(hwIds);
    }

    //
    // create CompatibleId value name.  It is a MULTI_SZ,
    //

    if (compatibleIds) {

        if (!IopFixupIds(compatibleIds, compatibleIdLength)) {
            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                            PNP_ERR_BOGUS_ID,
                            (ULONG_PTR)deviceObject,
                            (ULONG_PTR)compatibleIds,
                            4);
        }
        PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_COMPATIBLEIDS);
        ZwSetValueKey(uniqueIdHandle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_MULTI_SZ,
                        compatibleIds,
                        compatibleIdLength
                        );
        ExFreePool(compatibleIds);
    }

    //
    // If this is the devnode for the remote boot card, do
    // special setup for it.
    //

    if (isRemoteBootCard) {

        status = IopSetupRemoteBootCard(
                        (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                        uniqueIdHandle,
                        &unicodeDeviceInstance);

        if (status != STATUS_SUCCESS) {
            goto exit;
        }

        //
        // HACK BUGBUG: Need to turn this off, or else the device won't
        // be allowed to be opened until the PNP start IRP is done. Unfortunately
        // that is exactly what NDIS does in the start IRP handler - adamba 3/31/99
        //

        deviceObject->DeviceObjectExtension->ExtensionFlags &= ~DOE_START_PENDING;

    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    //
    // we've pretty much got the PDO information ready, apart from Child bus information
    // get that now, because class-installer may want it
    //
    status = IopQueryPnpBusInformation(
                 deviceObject,
                 &busTypeGuid,
                 &DeviceNode->ChildInterfaceType,
                 &DeviceNode->ChildBusNumber
             );

    if (NT_SUCCESS(status)) {

        DeviceNode->ChildBusTypeIndex = IopGetBusTypeGuidIndex(&busTypeGuid);

    } else {

        DeviceNode->ChildBusTypeIndex = 0xffff;
        DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
        DeviceNode->ChildBusNumber = 0xfffffff0;

    }

    //
    // we check HardwareDisabled directly in case it's a new device
    //
    if (processCriticalDevice && !capabilities.HardwareDisabled &&
        !IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {

        IopProcessCriticalDevice(DeviceNode);
    }

    //
    // Set DNF_DISABLED flag if the device instance is disabled.
    //

    ASSERT(!IopDoesDevNodeHaveProblem(DeviceNode) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_PARTIAL_LOG_CONF) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED) ||
           IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART));
    if (!IopIsDevNodeProblem(DeviceNode, CM_PROB_DISABLED) &&
        !IopIsDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED) &&
        !IopIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {

        IopIsDeviceInstanceEnabled(uniqueIdHandle, &unicodeDeviceInstance, TRUE);
    }

    //
    // This code HAS to be after we have checked for DISABLED device.
    // We could end up here from an ENABLE following a DISABLE.
    // Query for BOOT config if there is none already present.
    //

    cmResource = NULL;
    if (DeviceNode->BootResources == NULL) {
        status = IopQueryDeviceResources( deviceObject,
                                          QUERY_RESOURCE_LIST,
                                          &cmResource,
                                          &cmLength );
        if (!NT_SUCCESS(status)) {
            cmResource = NULL;
        }
    } else {

        DebugPrint(1,
                   ("PNPENUM: %ws already has BOOT config in IopProcessNewDeviceNode!\n",
                   DeviceNode->InstancePath.Buffer));
    }

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    //
    // Write boot resources to registry
    //
    if (logConfHandle && DeviceNode->BootResources == NULL) {
        PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
        if (cmResource) {
            ZwSetValueKey(
                        logConfHandle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_RESOURCE_LIST,
                        cmResource,
                        cmLength
                        );

            ExReleaseResource(&PpRegistryDeviceResource);

            //
            // This device consumes BOOT resources.  Reserve its boot resources
            //

            status = (*IopReserveResourcesRoutine)(ArbiterRequestPnpEnumerated,
                                                    deviceObject,
                                                    cmResource);

            ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

            if (NT_SUCCESS(status)) {
                DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
            }
            ExFreePool(cmResource);
        } else {
            ZwDeleteValueKey(logConfHandle, &unicodeName);
        }
    }

    //
    // SurpriseRemovalOK bits may have changed due to DNF_HAS_BOOT_CONFIG.
    //
    status = IopDeviceCapabilitiesToRegistry(DeviceNode,&capabilities);

#if DBG
    ASSERT(status == STATUS_SUCCESS);
#endif

    //
    // Clean up
    //

    if (logConfHandle) {
        ZwClose(logConfHandle);
    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    //
    // Create new value entry under ServiceKeyName\Enum to reflect the newly
    // added made-up device instance node.
    //

    status = IopNotifySetupDeviceArrival( deviceObject,
                                          uniqueIdHandle,
                                          TRUE);

    configuredBySetup = NT_SUCCESS(status);

    status = PpDeviceRegistration(
                 &unicodeDeviceInstance,
                 TRUE,
                 &DeviceNode->ServiceName
                 );

    if (NT_SUCCESS(status) && (configuredBySetup || isRemoteBootCard) &&
        IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED)) {

        IopClearDevNodeProblem(DeviceNode);
    }

    //
    // Add an event so user-mode will attempt to install this device later.
    //
    PpSetPlugPlayEvent( &GUID_DEVICE_ENUMERATED,
                        deviceObject);

    ZwClose(uniqueIdHandle);

    if (buffer) {
        ExFreePool(buffer);
    }
    if (description) {
        ExFreePool(description);
    }
    if (location) {
        ExFreePool(location);
    }
    return STATUS_SUCCESS;

exit:

    //
    // In case of failure, we don't set the DNF_PROCESSED flags.  So the device
    // will be processed again later.
    //

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    if (buffer) {
        ExFreePool(buffer);
    }
    if (description) {
        ExFreePool(description);
    }
    if (location) {
        ExFreePool(location);
    }
    return status;
}

NTSTATUS
IopCallDriverAddDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN LoadDriver,
    IN PADD_CONTEXT Context
    )

/*++

Routine Description:

    This function checks if the driver for the DeviceNode is present and loads
    the driver if necessary.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be enumerated.

    LoadDriver - Supplies a BOOLEAN value to indicate should a driver be loaded
                 to complete enumeration.

    Context - Supplies a pointer to ADD_CONTEXT to control how the device be added.

Return Value:

    NTSTATUS code.

--*/

{
    HANDLE enumKey;
    HANDLE instanceKey;
    HANDLE classKey = NULL;
    HANDLE classPropsKey = NULL;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation = NULL;
    RTL_QUERY_REGISTRY_TABLE queryTable[3];
    QUERY_CONTEXT queryContext;
    BOOLEAN deviceRaw = FALSE;
    BOOLEAN usePdoCharacteristics = TRUE;
    NTSTATUS status;
    DEVICE_CAPABILITIES capabilities;
    ULONG index;
    PDEVICE_OBJECT deviceObject;
#ifndef NO_SPECIAL_IRP
    PDEVICE_OBJECT fdoDeviceObject, topOfPdoStack, topOfLowerFilterStack;
    BOOLEAN deviceObjectHasBeenAttached = FALSE;
#endif

    DebugPrint(1, ("IopCallDriverAddDevice: Processing devnode %#08lx\n",
                   DeviceNode));

    DebugPrint(1, ("IopCallDriverAddDevice: DevNode flags going in = %#08lx\n",
                   DeviceNode->Flags));

    //
    // The device node may have been started at this point.  This is because
    // some ill-behaved miniport drivers call IopReportedDetectedDevice at
    // DriverEntry for the devices which we already know about.
    //

    if (!OK_TO_ADD_DEVICE(DeviceNode)) {
        return STATUS_SUCCESS;
    }

    ASSERT_INITED(DeviceNode->PhysicalDeviceObject);

    DebugPrint(1, ("IopCallDriverAddDevice:\t%s load driver\n",
                   (LoadDriver ? "Will" : "Won't")));

    DebugPrint(1, ("IopCallDriverAddDevice:\tOpening registry key %wZ\n",
                   &DeviceNode->InstancePath));

    //
    // Open the HKLM\System\CCS\Enum key.
    //

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {
        DebugPrint(1, ("IopCallDriverAddDevice:\tUnable to open "
                       "HKLM\\SYSTEM\\CCS\\ENUM\n"));
        return status;
    }

    //
    // Open the instance key for this devnode
    //

    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   &DeviceNode->InstancePath,
                                   KEY_READ
                                   );

    ZwClose(enumKey);

    if (!NT_SUCCESS(status)) {

        DebugPrint(1, ("IopCallDriverAddDevice:\t\tError %#08lx opening enum key\n",
                    status));
        return status;
    }

    //
    // Get the class value to locate the class key for this devnode
    //

    status = IopGetRegistryValue(instanceKey,
                                 REGSTR_VALUE_CLASSGUID,
                                 &keyValueInformation);

    if (NT_SUCCESS(status) && ((keyValueInformation->Type == REG_SZ) &&
                              (keyValueInformation->DataLength != 0))) {

        HANDLE controlKey;
        UNICODE_STRING unicodeClassGuid;

        IopRegistryDataToUnicodeString(
            &unicodeClassGuid,
            (PWSTR) KEY_VALUE_DATA(keyValueInformation),
            keyValueInformation->DataLength);

        DebugPrint(1, ("IopCallDriverAddDevice:\t\tClass GUID is %wZ\n",
                       &unicodeClassGuid));

        if (InitSafeBootMode) {
            if (!IopSafebootDriverLoad(&unicodeClassGuid)) {
                PKEY_VALUE_FULL_INFORMATION ClassValueInformation = NULL;
                NTSTATUS s;

                //
                // don't load the driver
                //
                DbgPrint("SAFEBOOT: skipping device = %wZ\n",&unicodeClassGuid);
                s = IopGetRegistryValue(instanceKey,
                                        REGSTR_VAL_DEVDESC,
                                        &ClassValueInformation);
                if (NT_SUCCESS(s)) {
                    UNICODE_STRING ClassString;

                    RtlInitUnicodeString(&ClassString, (PCWSTR) KEY_VALUE_DATA(ClassValueInformation));

                    IopBootLog(&ClassString, FALSE);
                } else {
                    IopBootLog(&unicodeClassGuid, FALSE);
                }
                return STATUS_UNSUCCESSFUL;
            }
        }

        //
        // Open the class key
        //

        status = IopOpenRegistryKeyEx( &controlKey,
                                       NULL,
                                       &CmRegistryMachineSystemCurrentControlSetControlClass,
                                       KEY_READ
                                       );

        if (!NT_SUCCESS(status)) {

            DebugPrint(1, ("IopCallDriverAddDevice:\tUnable to open "
                        "HKLM\\SYSTEM\\CCS\\CONTROL\\CLASS - %#08lx\n",
                        status));
            classKey = NULL;
        } else {

            status = IopOpenRegistryKeyEx( &classKey,
                                           controlKey,
                                           &unicodeClassGuid,
                                           KEY_READ
                                           );

            ZwClose(controlKey);

            if (!NT_SUCCESS(status)) {

                DebugPrint(1, ("IopCallDriverAddDevice:\tUnable to open GUID key "
                            "%wZ - %#08lx\n",
                            &unicodeClassGuid,
                            status));

                classKey = NULL;
            }
        }

        if (classKey != NULL) {

            UNICODE_STRING unicodeProperties;

            RtlInitUnicodeString(&unicodeProperties, REGSTR_KEY_DEVICE_PROPERTIES );

            status = IopOpenRegistryKeyEx( &classPropsKey,
                                           classKey,
                                           &unicodeProperties,
                                           KEY_READ
                                           );

            if (!NT_SUCCESS(status)) {

                DebugPrint(2, ("IopCallDriverAddDevice:\tUnable to open GUID\\Properties key "
                            "%wZ - %#08lx\n",
                            &unicodeClassGuid,
                            status));

                classPropsKey = NULL;
            }
        }

        ExFreePool(keyValueInformation);
        keyValueInformation = NULL;

    }

    //
    // Check to see if there's a service assigned to this device node.  If
    // there's not then we can bail out without wasting too much time.
    //

    RtlZeroMemory(&queryContext, sizeof(queryContext));

    queryContext.DeviceNode = DeviceNode;
    queryContext.LoadDriver = LoadDriver;

    queryContext.AddContext = Context;

    RtlZeroMemory(queryTable, sizeof(queryTable));

    queryTable[0].QueryRoutine =
        (PRTL_QUERY_REGISTRY_ROUTINE) IopCallDriverAddDeviceQueryRoutine;
    queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
    queryTable[0].EntryContext = (PVOID) UIntToPtr(LowerDeviceFilters);

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR) instanceKey,
                                    queryTable,
                                    &queryContext,
                                    NULL);
    if (NT_SUCCESS(status)) {

        if (classKey != NULL) {

            queryTable[0].QueryRoutine =
                (PRTL_QUERY_REGISTRY_ROUTINE) IopCallDriverAddDeviceQueryRoutine;
            queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
            queryTable[0].EntryContext = (PVOID) UIntToPtr(LowerClassFilters);
            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR) classKey,
                                            queryTable,
                                            &queryContext,
                                            NULL);
        }

        if (NT_SUCCESS(status)) {
            queryTable[0].QueryRoutine = (PRTL_QUERY_REGISTRY_ROUTINE) IopCallDriverAddDeviceQueryRoutine;
            queryTable[0].Name = REGSTR_VALUE_SERVICE;
            queryTable[0].EntryContext = (PVOID) UIntToPtr(DeviceService);
            queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR) instanceKey,
                                            queryTable,
                                            &queryContext,
                                            NULL);
        }
    }

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER) {

        //
        // One of the services for this device is a legacy driver.  Don't try
        // to add any filters since we'll just screw up the device stack.
        //

        status = STATUS_SUCCESS;
        goto Cleanup;

    } else if (NT_SUCCESS(status)) {

        //
        // Call was successful so we must have been able to reference the
        // driver object.
        //

        ASSERT(queryContext.DriverLists[DeviceService] != NULL);

        if (queryContext.DriverLists[DeviceService]->NextEntry != NULL) {

            //
            // There's more than one service assigned to this device.  Configuration
            // error

            DebugPrint(1, ("IopCallDriverAddDevice: Configuration Error - more "
                        "than one service in driver list\n"));

            IopSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);

            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }
        //
        // this is the only case (FDO specified) where we can ignore PDO's characteristics
        //
        usePdoCharacteristics = FALSE;

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        DebugPrint(1, ("IopCallDriverAddDevice\t\tError %#08lx reading service "
                       "value for devnode %#08lx\n", status, DeviceNode));

        if (!IopDeviceNodeFlagsToCapabilities(DeviceNode)->RawDeviceOK) {

            //
            // The device cannot be used raw.  Bail out now.
            //

            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;

        } else {

            //
            // Raw device access is okay.
            //

            IopClearDevNodeProblem(DeviceNode);

            usePdoCharacteristics = TRUE; // shouldn't need to do this, but better be safe than sorry
            deviceRaw = TRUE;
            status = STATUS_SUCCESS;

        }

    } else {

        //
        // something else went wrong while parsing the service key.  The
        // query routine will have set the flags appropriately so we can
        // just bail out.
        //

        goto Cleanup;

    }

    //
    // For each type of filter driver we want to build a list of the driver
    // objects to be loaded.  We'll build all the driver lists if we can
    // and deal with error conditions afterwards.
    //

     //
     // First get all the information we have to out of the instance key and
     // the device node.
     //

     RtlZeroMemory(queryTable, sizeof(queryTable));

     queryTable[0].QueryRoutine =
         (PRTL_QUERY_REGISTRY_ROUTINE) IopCallDriverAddDeviceQueryRoutine;
     queryTable[0].Name = REGSTR_VAL_UPPERFILTERS;
     queryTable[0].EntryContext = (PVOID) UIntToPtr(UpperDeviceFilters);
     status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                     (PWSTR) instanceKey,
                                     queryTable,
                                     &queryContext,
                                     NULL);

     if (NT_SUCCESS(status) && classKey) {
         queryTable[0].QueryRoutine =
             (PRTL_QUERY_REGISTRY_ROUTINE) IopCallDriverAddDeviceQueryRoutine;
         queryTable[0].Name = REGSTR_VAL_UPPERFILTERS;
         queryTable[0].EntryContext = (PVOID) UIntToPtr(UpperClassFilters);

         status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                         (PWSTR) classKey,
                                         queryTable,
                                         &queryContext,
                                         NULL);
    }

    if (NT_SUCCESS(status)) {

        UCHAR serviceType = 0;
        PDRIVER_LIST_ENTRY listEntry = queryContext.DriverLists[serviceType];

        //
        // Make sure there's no more than one device service.  Anything else is
        // a configuration error.
        //

        ASSERT(!(DeviceNode->Flags & DNF_LEGACY_DRIVER));
        ASSERT(!(DeviceNode->Flags & DNF_ADDED));

        ASSERTMSG(
            "Error - Device has no service but cannot be run RAW\n",
            ((queryContext.DriverLists[DeviceService] != NULL) || (deviceRaw)));

#ifndef NO_SPECIAL_IRP
        //
        // Grab the top of PDO stack
        //
        topOfPdoStack = IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject);
#endif

        //
        // It's okay to try adding all the drivers.
        //
        for (serviceType = 0; serviceType < MaximumAddStage; serviceType++) {

            DebugPrint(1, ("IopCallDriverAddDevice: Adding Services (type %d)\n",
                        serviceType));

            if (serviceType == DeviceService) {

                if (deviceRaw&&(queryContext.DriverLists[serviceType]==NULL)) {

                    //
                    // Mark the devnode as added, as it has no service.
                    //

                    ASSERT(queryContext.DriverLists[serviceType] == NULL);
                    DeviceNode->Flags |= DNF_ADDED;

#ifndef NO_SPECIAL_IRP
                    //
                    // For the purpose of asserting IRPs, we mark the FDO. We
                    // don't mark a raw PDO as the BOTTOM of the FDO stack as
                    // that would be the lower filter in this case.
                    //
                    DeviceNode->PhysicalDeviceObject->DeviceObjectExtension->ExtensionFlags |=
                        DOE_DESIGNATED_FDO | DOE_RAW_FDO;

                } else {

                    //
                    // Since we are going to see a service, grab a pointer to
                    // the current top of the stack. While here, assert there
                    // is exactly one service driver to load...
                    //
                    ASSERT(queryContext.DriverLists[serviceType]);
                    ASSERT(!queryContext.DriverLists[serviceType]->NextEntry);
                    topOfLowerFilterStack = IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject);
#endif
                }
            }

            for (listEntry = queryContext.DriverLists[serviceType];
                listEntry != NULL;
                listEntry = listEntry->NextEntry) {

                PDRIVER_ADD_DEVICE addDeviceRoutine;

                DebugPrint(1, ("IopCallDriverAddDevice:\tAdding driver %#08lx\n",
                            listEntry->DriverObject));

                ASSERT(listEntry->DriverObject);
                ASSERT(listEntry->DriverObject->DriverExtension);
                ASSERT(listEntry->DriverObject->DriverExtension->AddDevice);

                //
                // Invoke the driver's AddDevice() entry point.
                //
                addDeviceRoutine =
                    listEntry->DriverObject->DriverExtension->AddDevice;

                status = (addDeviceRoutine)(listEntry->DriverObject,
                                            DeviceNode->PhysicalDeviceObject);

                DebugPrint(1, ("IopCallDriverAddDevice:\t\tRoutine returned "
                            "%#08lx\n", status));

                if (NT_SUCCESS(status)) {

#ifndef NO_SPECIAL_IRP
                   if (!deviceObjectHasBeenAttached) {

                       //
                       // Mark the first driver loaded by this routine as the
                       // bottom of the FDO stack. These must detach on a remove.
                       // Note we can't simply flag the top of the stack, as
                       // someone might attach in AddDevice...
                       //
                       fdoDeviceObject = topOfPdoStack->AttachedDevice;
                       if (fdoDeviceObject) {

                          fdoDeviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_BOTTOM_OF_FDO_STACK;
                          deviceObjectHasBeenAttached = TRUE;
                       }
                   }

                   //
                   // Also note it is legal for a filter to succeed AddDevice
                   // but fail to attach anything to the top of the stack.
                   //
                   if (serviceType == DeviceService) {

                       //
                       // ADRIAO BUGBUG 10/07/98 - Since we are temporarily
                       // letting successful but Noop'd AddDevice's through,
                       // mark the stack appropriately. We will make it look
                       // like a RAW FDO
                       //
                       fdoDeviceObject = topOfLowerFilterStack->AttachedDevice;

                       //ASSERT(fdoDeviceObject != NULL);

                       if (!fdoDeviceObject) {

                           //
                           // Nope, didn't get an FDO. Mark the PDO raw.
                           // ADRIAO BUGBUG 10/07/98 -
                           //      Another reason to complain about the legality
                           // of succeeding a FDO AddDevice without attaching
                           // anything - how does the PDO know he also has to
                           // respond as an FDO????
                           //
                           fdoDeviceObject = DeviceNode->PhysicalDeviceObject;
                           fdoDeviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_RAW_FDO;
                       }

                       //
                       // Mark appropriate node "FDO".
                       //
                       fdoDeviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_DESIGNATED_FDO;
                   }
#endif

                   DeviceNode->Flags |= DNF_ADDED;
                } else if (serviceType == DeviceService) {

                    //
                    // If filter drivers return failure, keep going.
                    //

                    DeviceNode->Flags &= ~DNF_ADDED;
                    IopSetDevNodeProblem(DeviceNode, CM_PROB_FAILED_ADD);
                    IopRequestDeviceRemoval(DeviceNode->PhysicalDeviceObject, CM_PROB_FAILED_ADD);
                    goto Cleanup;
                }

                if (IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject)->Flags & DO_DEVICE_INITIALIZING) {
                    DebugPrint(1, ("***************** DO_DEVICE_INITIALIZING not cleared on %#08lx\n",
                                IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject)));
                }

                ASSERT_INITED(IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject));
            }
        }

        //
        // change PDO and all attached objects
        // to have properties specified in the registry
        //

        IopChangeDeviceObjectFromRegistryProperties(DeviceNode->PhysicalDeviceObject, classPropsKey, instanceKey, usePdoCharacteristics);

        //
        // CapabilityFlags are refreshed with call to IopDeviceCapabilitiesToRegistry after device is started
        //

    } else {

        DebugPrint(1, ("IopCallDriverAddDevice: Error %#08lx while building "
                    "driver load list\n", status));
    }

    deviceObject = DeviceNode->PhysicalDeviceObject;

    status = IopQueryLegacyBusInformation(
                 deviceObject,
                 NULL,
                 &DeviceNode->InterfaceType,
                 &DeviceNode->BusNumber
             );

    if (!NT_SUCCESS(status)) {

        DeviceNode->InterfaceType = InterfaceTypeUndefined;
        DeviceNode->BusNumber = 0xfffffff0;

    }

    status = STATUS_SUCCESS;

Cleanup:
    {

        UCHAR i;

        DebugPrint(1, ("IopCallDriverAddDevice: DevNode flags leaving = %#08lx\n",
                    DeviceNode->Flags));

        DebugPrint(1, ("IopCallDriverAddDevice: Cleaning up\n"));

        //
        // Free the entries in the driver load list & release the references on
        // their driver objects.
        //

        for (i = 0; i < MaximumAddStage; i++) {

            PDRIVER_LIST_ENTRY listHead = queryContext.DriverLists[i];

            while(listHead != NULL) {

                PDRIVER_LIST_ENTRY tmp = listHead;

                listHead = listHead->NextEntry;

                ASSERT(tmp->DriverObject != NULL);

                ObDereferenceObject(tmp->DriverObject);

                ExFreePool(tmp);
            }
        }
    }

    ZwClose(instanceKey);

    if (classKey != NULL) {
        ZwClose(classKey);
    }

    if (classPropsKey != NULL) {
        ZwClose(classPropsKey);
    }

    DebugPrint(1, ("IopCallDriverAddDevice: Returning status %#08lx\n", status));

    return status;
}

NTSTATUS
IopCallDriverAddDeviceQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PWCHAR ValueData,
    IN ULONG ValueLength,
    IN PQUERY_CONTEXT Context,
    IN ULONG ServiceType
    )

/*++

Routine Description:

    This routine is called to build a list of driver objects which need to
    be Added to a physical device object.  Each time it is called with a
    service name it will locate a driver object for that device and append
    it to the proper driver list for the device node.

    In the event a driver object cannot be located or that it cannot be loaded
    at this time, this routine will return an error and will set the flags
    in the device node in the context appropriately.

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - a structure which contains the device node, the context passed
              to IopCallDriverAddDevice and the driver lists for the device
              node.

    EntryContext - the index of the driver list the routine should append
                   nodes to.

Return Value:

    STATUS_SUCCESS if the driver was located and added to the list
    successfully or if there was a non-fatal error while handling the
    driver.

    an error value indicating why the driver could not be added to the list.

--*/

{
    UNICODE_STRING unicodeServiceName;
    UNICODE_STRING unicodeDriverName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    ULONG i;
    ULONG loadType;

    PWSTR prefixString = L"\\Driver\\";
    BOOLEAN madeupService;

    USHORT groupIndex;
    PDRIVER_OBJECT driverObject = NULL;

    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN freeDriverName = FALSE;

    DebugPrint(1, ("IopCallDriverAddDevice:\t\tValue %ws [Type %d, Len %d] @ "
                "%#08lx\n",
                ValueName, ValueType, ValueLength, ValueData));

    //
    // First check and make sure that the value type is okay.  An invalid type
    // is not a fatal error.
    //

    if (ValueType != REG_SZ) {

        DebugPrint(1, ("IopCallDriverAddDevice:\t\tValueType %d invalid for "
                    "ServiceType %d\n",
                    ValueType,
                    ServiceType));

        return STATUS_SUCCESS;
    }

    //
    // Make sure the string is a reasonable length.
    //

    if (ValueLength <= sizeof(WCHAR)) {

        DebugPrint(1, ("IopCallDriverAddDevice:\t\tValueLength %d is too short\n",
                    ValueLength));

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&unicodeServiceName, ValueData);

    DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tService Name %wZ\n",
                &unicodeServiceName));

    //
    // Check the service name to see if it should be used directly to reference
    // the driver object.  If the string begins with "\Driver", make sure the
    // madeupService flag is set.
    //

    madeupService = TRUE;
    i = 0;

    while(*prefixString != L'\0') {

        if (unicodeServiceName.Buffer[i] != *prefixString) {

            madeupService = FALSE;
            break;
        }

        i++;
        prefixString++;
    }

    if (madeupService) {

        RtlInitUnicodeString(&unicodeDriverName, unicodeServiceName.Buffer);

        groupIndex = 0;
        loadType = SERVICE_BOOT_START;

    } else {

        HANDLE serviceKey;

        //
        // BUGBUG - (RBN) Hack to set the service name in the devnode if it
        //      isn't already set.
        //
        //      This probably should be done earlier somewhere else after the
        //      INF is run, but if we don't do it now we'll blow up when we
        //      call IopGetDriverLoadType below.
        //

        if (Context->DeviceNode->ServiceName.Length == 0) {

            Context->DeviceNode->ServiceName = unicodeServiceName;
            Context->DeviceNode->ServiceName.Buffer = ExAllocatePool( NonPagedPool,
                                                                      unicodeServiceName.MaximumLength );

            if (Context->DeviceNode->ServiceName.Buffer != NULL) {
                RtlCopyMemory( Context->DeviceNode->ServiceName.Buffer,
                               unicodeServiceName.Buffer,
                               unicodeServiceName.MaximumLength );
            } else {
                RtlInitUnicodeString( &Context->DeviceNode->ServiceName, NULL );

                DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tCannot allocate memory for service name in devnode\n"));

                status = STATUS_UNSUCCESSFUL;

                goto Cleanup;
            }
        }

        //
        // Check in the registry to find the name of the driver object
        // for this device.
        //

        status = IopOpenServiceEnumKeys(&unicodeServiceName,
                                        KEY_READ,
                                        &serviceKey,
                                        NULL,
                                        FALSE);

        if (!NT_SUCCESS(status)) {

            //
            // Cannot open the service key for this driver.  This is a
            // fatal error.
            //

            DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tStatus %#08lx "
                        "opening service key\n",
                        status));

            IopSetDevNodeProblem(Context->DeviceNode, CM_PROB_REGISTRY);

            goto Cleanup;
        }

        groupIndex = IopGetGroupOrderIndex(serviceKey);

        status = IopGetDriverNameFromKeyNode(serviceKey, &unicodeDriverName);

        if (!NT_SUCCESS(status)) {

            ZwClose(serviceKey);

            //
            // Can't get the driver name from the service key.  This is a
            // fatal error.
            //

            DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tStatus %#08lx "
                        "getting driver name\n",
                        status));

            IopSetDevNodeProblem(Context->DeviceNode, CM_PROB_REGISTRY);
            goto Cleanup;
        } else {
            freeDriverName = TRUE;
        }

        loadType = SERVICE_DISABLED;

        status = IopGetRegistryValue(serviceKey, L"Start", &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if (keyValueInformation->Type == REG_DWORD) {
                if (keyValueInformation->DataLength == sizeof(ULONG)) {
                    loadType = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
            }
            ExFreePool(keyValueInformation);
        }

        ZwClose(serviceKey);
    }

    DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tDriverName is %wZ\n",
                &unicodeDriverName));

    driverObject = IopReferenceDriverObjectByName(&unicodeDriverName);

    if (driverObject == NULL) {

        PWCHAR buffer;
        UNICODE_STRING unicodeServicePath;

        //
        // We couldn't find a driver object.  It's possible the driver isn't
        // loaded & initialized so check to see if we can try to load it
        // now.
        //

        if (madeupService) {

            //
            // The madeup service's driver doesn't seem to exist yet.
            // We will fail the request without setting DNF_ADD_FAILED such that
            // we will try it again later.  (Root Enumerated devices...)
            //

            DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tCannot find driver "
                        "object for madeup service\n"));

            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }

        if (ServiceType != DeviceService && !PnPBootDriversInitialized) {

            //
            // If we are in BootDriverInitialization phase and trying to load a filter driver
            //

            driverObject = IopLoadBootFilterDriver(&unicodeDriverName, groupIndex);
            if (driverObject == NULL) {
                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }
        } else {
            if (!Context->LoadDriver) {

                //
                // We're not supposed to try and load a driver - most likely our
                // disk drivers aren't initialized yet.  We need to stop the add
                // process but we can't mark the devnode as failed or we won't
                // be called again when we can load the drivers.
                //

                DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tNot allowed to load "
                            "drivers yet\n"));

                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

            if (groupIndex > Context->AddContext->GroupsToStart) {

                //
                // We're not allowed to initialize this driver until drivers in
                // previous groups are loaded.  Leave the devnode flags untouched
                // so we get called again, but stop the add process now.
                //

                DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tLower group drivers "
                            "have not been initialized yet\n"));
                if (groupIndex < Context->AddContext->GroupToStartNext) {
                    Context->AddContext->GroupToStartNext = groupIndex;
                }

                DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tGroup = %d, To Start = "
                            "Start Next = %d\n",
                            groupIndex,
                            Context->AddContext->GroupsToStart,
                            Context->AddContext->GroupToStartNext));

                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }



            if (loadType > Context->AddContext->DriverStartType) {

                if (loadType == SERVICE_DISABLED &&
                    !IopDoesDevNodeHaveProblem(Context->DeviceNode)) {
                    IopSetDevNodeProblem(Context->DeviceNode, CM_PROB_DISABLED_SERVICE);
                }

                //
                // The service is either disabled or we are not at the right
                // time to load it.  Don't load it, but make sure we can get
                // called again.  If a service is marked as demand start, we
                // always load it.
                //

                DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tService is disabled or not at right time to load it\n"));
                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

            {
                HANDLE handle;

                //
                // Check in the registry to find the name of the driver object
                // for this device.
                //

                status = IopOpenServiceEnumKeys(&unicodeServiceName,
                                                KEY_READ,
                                                &handle,
                                                NULL,
                                                FALSE);

                if (!NT_SUCCESS(status)) {

                    //
                    // Cannot open the service key for this driver.  This is a
                    // fatal error.
                    //

                    DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tStatus %#08lx "
                                "opening service key\n",
                                status));
                } else {
                    status = IopLoadDriver(handle,FALSE);              // handle will be closed by IopLoadDriver

                    if (PnPInitialized) {

                        PLIST_ENTRY entry;
                        PREINIT_PACKET reinitEntry;

                        //
                        // Walk the list reinitialization list in case this driver, or
                        // some other driver, has requested to be invoked at a re-
                        // initialization entry point.
                        //

                        while (entry = ExInterlockedRemoveHeadList( &IopDriverReinitializeQueueHead, &IopDatabaseLock )) {
                            reinitEntry = CONTAINING_RECORD( entry, REINIT_PACKET, ListEntry );
                            reinitEntry->DriverObject->DriverExtension->Count++;
                            reinitEntry->DriverObject->Flags &= ~DRVO_REINIT_REGISTERED;
                            reinitEntry->DriverReinitializationRoutine( reinitEntry->DriverObject,
                                                                        reinitEntry->Context,
                                                                        reinitEntry->DriverObject->DriverExtension->Count );
                            ExFreePool( reinitEntry );
                        }
                    }
                }

            }
            if (!NT_SUCCESS(status)) {

                if (PnPBootDriversInitialized) {
                    DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tStatus %#08lx "
                                "from loading driver\n", status));
                }
            }

        }
    } else {
        ObDereferenceObject(driverObject);
        if (!(driverObject->Flags & DRVO_INITIALIZED)) {
            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }
    }

    //
    // Ignore the return value from the driver load - just try and get a
    // pointer to the driver object for the service.
    //

    driverObject = IopReferenceDriverObjectByName(&unicodeDriverName);

    if (driverObject == NULL) {

        if (PnPBootDriversInitialized) {

            //
            // Apparently the load didn't work out very well.  This is a
            // fatal error.
            //

            DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tUnable to reference "
                        "driver %wZ\n", &unicodeDriverName));

            if (!IopDoesDevNodeHaveProblem(Context->DeviceNode)) {
                IopSetDevNodeProblem(Context->DeviceNode, CM_PROB_FAILED_ADD);
            }
        }

        status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    } else if (!(driverObject->Flags & DRVO_INITIALIZED)) {
        ObDereferenceObject(driverObject);
        status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tDriver Reference %#08lx\n",
                driverObject));

    //
    // Check to see if the driver is a legacy driver rather than a Pnp one.
    //

    if (IopIsLegacyDriver(driverObject)) {

        //
        // It is.  Since the legacy driver may have already obtained a
        // handle to the device object, we need to assume this device
        // has been added and started.
        //

        DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tDriver is a legacy "
                    "driver\n"));

        if (ServiceType == DeviceService) {
            Context->DeviceNode->Flags |= DNF_ADDED +
                                          DNF_STARTED +
                                          DNF_LEGACY_DRIVER;

            status = STATUS_UNSUCCESSFUL;
        } else {

            //
            // We allow someone to plug in a legacy driver as a filter driver.
            // In this case, the legacy driver will be loaded but will not be part
            // of our pnp driver stack.
            //

            status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // There's a chance the driver detected this PDO during it's driver entry
    // routine.  If it did then just bail out.
    //

    if (!OK_TO_ADD_DEVICE(Context->DeviceNode)) {

        DebugPrint(1, ("IopCallDriverAddDevice\t\t\tDevNode was reported "
                       "as detected during driver entry\n"));
        status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Add the driver to the list.
    //

    {
        PDRIVER_LIST_ENTRY listEntry;
        PDRIVER_LIST_ENTRY *runner = &(Context->DriverLists[ServiceType]);

        status = STATUS_SUCCESS;

        //
        // Allocate a new list entry to queue this driver object for the caller
        //

        listEntry = ExAllocatePool(PagedPool, sizeof(DRIVER_LIST_ENTRY));

        if (listEntry == NULL) {

            DebugPrint(1, ("IopCallDriverAddDevice:\t\t\tUnable to allocate list "
                        "entry\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        listEntry->DriverObject = driverObject;
        listEntry->NextEntry = NULL;

        while(*runner != NULL) {
            runner = &((*runner)->NextEntry);
        }

        *runner = listEntry;
    }

Cleanup:

    if (freeDriverName) {
        RtlFreeUnicodeString(&unicodeDriverName);
    }
    return status;
}


NTSTATUS
IopQueryDeviceCapabilities(
    IN PDEVICE_NODE DeviceNode,
    OUT PDEVICE_CAPABILITIES Capabilities
    )

/*++

Routine Description:

    This routine will issue an irp to the DeviceObject to retrieve the
    pnp device capabilities.
    Should only be called twice - first from IopProcessNewDeviceNode,
    and second from IopDeviceNodeCapabilitiesToRegistry, called after
    device is started. If you consider calling this device, see if
    DeviceNode->CapabilityFlags does what you need instead (accessed
    via IopDeviceNodeFlagsToCapabilities(...).

Arguments:

    DeviceNode - the device object the request should be sent to.

    Capabilities - a capabilities structure to be filled in by the driver.

Return Value:

    status

--*/

{
    IO_STACK_LOCATION irpStack;
    PVOID dummy;

    NTSTATUS status;

    //
    // Initialize the capabilities structure.
    //

    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));
    Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities->Version = 1;
    Capabilities->Address = Capabilities->UINumber = (ULONG)-1;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpStack, sizeof(IO_STACK_LOCATION));

    //
    // Query the device's capabilities
    //

    irpStack.MajorFunction = IRP_MJ_PNP;
    irpStack.MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack.Parameters.DeviceCapabilities.Capabilities = Capabilities;

    status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject,
                                &irpStack,
                                &dummy);

    ASSERT(status != STATUS_PENDING);

    return status;
}

BOOLEAN
IopProcessCriticalDevice(
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine will check whether the device is a "critical" one (see the
    top of this file for a description of critical).  If the device is critical
    then it will be assigned a service based on the contents of
    IopCriticalDeviceList.

Arguments:

    DeviceNode - the device node to process

Return Value:

    TRUE if the device is critical
    FALSE otherwise

--*/

{
    HANDLE enumKey;
    HANDLE instanceKey;

    UNICODE_STRING service, classGuid, lowerFilters, upperFilters;
    BOOLEAN foundMatch = FALSE;

    RTL_QUERY_REGISTRY_TABLE queryTable[2];

    NTSTATUS status;

#if DBG_SCOPE
    PWCHAR str;
    ULONG length;
#endif

    DebugPrint(1, ("IopIsCriticalPnpDevice called for devnode %#08lx\n", DeviceNode));

    //
    // Open the HKLM\System\CCS\Enum key.
    //

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {
        DebugPrint(1, ("IICPD: couldn't open enum key %#08lx\n", status));
        return FALSE;
    }

    //
    // Open the instance key for this devnode
    //

    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   &DeviceNode->InstancePath,
                                   KEY_ALL_ACCESS
                                   );

    ZwClose(enumKey);

    //
    // First determine if this is a critical device type
    //

    if (!NT_SUCCESS(status)) {
        DebugPrint(1, ("IICPD: couldn't open instance path key %wZ [%#08lx]\n",
                       &(DeviceNode->InstancePath)));
        ZwClose(instanceKey);
        return FALSE;

    } else {
        //
        // Call IopProcessCriticalDeviceRoutine to
        // enumerate entries in the CriticalDeviceDatabase
        // and compare with HardwareId and CompatibleIds
        // value data from instanceKey.
        //
        RtlInitUnicodeString(&service, NULL);
        RtlInitUnicodeString(&classGuid, NULL);
        RtlInitUnicodeString(&lowerFilters, NULL);
        RtlInitUnicodeString(&upperFilters, NULL);
        status = IopProcessCriticalDeviceRoutine(instanceKey,
                                                 &foundMatch,
                                                 &service,
                                                 &classGuid,
                                                 &lowerFilters,
                                                 &upperFilters);
        if (!NT_SUCCESS(status)) {
            DebugPrint(1, ("IICPD: IopProcessCriticalDeviceRoutine failed with status %#08lx\n", status));
            return FALSE;
        }

    }

    //
    // If we get here then this is a "critical" device and we know the service
    // to setup for it.  Set the service value in the registry.
    //

    if (!foundMatch) {
        ZwClose(instanceKey);
        return FALSE;

    } else {
        UNICODE_STRING serviceValue, classGuidValue, lowerFiltersValue,
                       upperFiltersValue;

        DebugPrint(1, ("IopProcessCriticalDevice: Setting up critical service\n"));

        RtlInitUnicodeString(&serviceValue, REGSTR_VALUE_SERVICE);
        RtlInitUnicodeString(&classGuidValue, REGSTR_VALUE_CLASSGUID);
        RtlInitUnicodeString(&lowerFiltersValue, REGSTR_VALUE_LOWERFILTERS);
        RtlInitUnicodeString(&upperFiltersValue, REGSTR_VALUE_UPPERFILTERS);

        if (classGuid.Buffer) {
            DebugPrint(1, ("IopProcessCriticalDevice: classGuid is %wZ\n",
                           &classGuid));
            status = ZwSetValueKey(instanceKey,
                                   &classGuidValue,
                                   0L,
                                   REG_SZ,
                                   classGuid.Buffer,
                                   classGuid.Length + sizeof(UNICODE_NULL));
        }
        if (lowerFilters.Buffer) {
#if DBG_SCOPE
            str = lowerFilters.Buffer;
            while ((length = wcslen(str)) != 0) {
                DebugPrint(1, ("IopProcessCriticalDevice: lower filter is %ws\n",
                               str));
                str += (length + 1);
            }
#endif
            status = ZwSetValueKey(instanceKey,
                                   &lowerFiltersValue,
                                   0L,
                                   REG_MULTI_SZ,
                                   lowerFilters.Buffer,
                                   lowerFilters.Length); // + sizeof(UNICODE_NULL));
        }

        if (upperFilters.Buffer) {
#if DBG_SCOPE
            str = upperFilters.Buffer;
            while ((length = wcslen(str)) != 0) {
                DebugPrint(1, ("IopProcessCriticalDevice: upper filter is %ws\n",
                               str));
                str += (length + 1);
            }
#endif
            status = ZwSetValueKey(instanceKey,
                                   &upperFiltersValue,
                                   0L,
                                   REG_MULTI_SZ,
                                   upperFilters.Buffer,
                                   upperFilters.Length); // + sizeof(UNICODE_NULL));
        }

        DebugPrint(1, ("IopProcessCriticalDevice: service is %wZ\n",
                       &service));
        status = ZwSetValueKey(instanceKey,
                               &serviceValue,
                               0L,
                               REG_SZ,
                               service.Buffer,
                               service.Length + sizeof(UNICODE_NULL));

        //
        // If the service was set properly set the CONFIGFLAG_FINISH_INSTALL so
        // we will still get a new hw found popup and go through the class
        // installer.
        //

        if (NT_SUCCESS(status)) {

            UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
            UNICODE_STRING valueName;
            PKEY_VALUE_PARTIAL_INFORMATION keyInfo =
                (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
            ULONG flags = 0;

            ULONG length;

            NTSTATUS tmpStatus;

            RtlInitUnicodeString(&valueName, REGSTR_VALUE_CONFIG_FLAGS);

            tmpStatus = ZwQueryValueKey(instanceKey,
                                        &valueName,
                                        KeyValuePartialInformation,
                                        keyInfo,
                                        sizeof(buffer),
                                        &length);

            if (NT_SUCCESS(tmpStatus) && (keyInfo->Type == REG_DWORD)) {

                flags = *(PULONG)keyInfo->Data;
            }

            flags &= ~(CONFIGFLAG_REINSTALL | CONFIGFLAG_FAILEDINSTALL);
            flags |= CONFIGFLAG_FINISH_INSTALL;

            ZwSetValueKey(instanceKey,
                          &valueName,
                          0L,
                          REG_DWORD,
                          &flags,
                          sizeof(ULONG));

            ASSERT(!IopDoesDevNodeHaveProblem(DeviceNode) ||
                   IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
                   IopIsDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL) ||
                   IopIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL));

            IopClearDevNodeProblem(DeviceNode);
        }
        ZwClose(instanceKey);
    }

    IopFreeAllocatedUnicodeString(&service);
    IopFreeAllocatedUnicodeString(&classGuid);
    IopFreeAllocatedUnicodeString(&lowerFilters);
    IopFreeAllocatedUnicodeString(&upperFilters);

    return (NT_SUCCESS(status));
}


NTSTATUS
IopProcessCriticalDeviceRoutine(
    IN  HANDLE HDevInstance,
    IN  PBOOLEAN FoundMatch,
    IN  PUNICODE_STRING ServiceName,
    IN  PUNICODE_STRING ClassGuid,
    IN  PUNICODE_STRING LowerFilters,
    IN  PUNICODE_STRING UpperFilters
    )

/*++


Routine Description:

    This routine will enumerate all values of the CriticalDeviceDatabase
    registry key, and compare each with entries within
    the HardwareId and CompatibleIds values of key associated
    with the current device instance.

Arguments:

    HDevInstance - HANDLE to device instance.

    FoundMatch - receives TRUE if a match was found.

    ServiceName - receives name of service to be assigned to
                  the device instance pointed to by
          HDevInstance.

Return Value:

    NTSTATUS code

         STATUS_SUCCESS
     STATUS_INVALID_PARAMETER

--*/

{
    NTSTATUS                    status;
    HANDLE                      hRegistryMachine, hCriticalDeviceKey,
                                hCriticalEntry;
    PWSTR                       keyValueInfoTag[2];
    PKEY_VALUE_FULL_INFORMATION keyValueInfo[2], matchedKeyValFullInfo;
    BUFFER_INFO                 infoBuffer;
    ULONG                       enumIndex, idIndex, resultSize, stringLength;
    UNICODE_STRING              tmpUnicodeString, unicodeCriticalEntry,
                                unicodeCriticalDeviceKeyName;
    PWCHAR                      stringStart, bufferEnd, ptr;
    PRTL_QUERY_REGISTRY_TABLE   parameters = NULL;

#define INITIAL_INFOBUFFER_SIZE sizeof(KEY_VALUE_FULL_INFORMATION) + 8*sizeof(WCHAR) + 255*sizeof(WCHAR)

    hRegistryMachine = NULL;
    hCriticalDeviceKey = NULL;
    infoBuffer.Buffer = NULL;

    //
    //  Read Key Value Information
    //  from HardwareId and CompatibleIds values
    //
    // keyValueInfo[idIndex == 0] == REGSTR_VAL_HARDWAREID == "HardwareId"
    // keyValueInfo[idIndex == 1] == REGSTR_VAL_COMPATIBLEIDS == "CompatibleIds"
    //
    keyValueInfoTag[0] = REGSTR_VAL_HARDWAREID;
    keyValueInfoTag[1] = REGSTR_VAL_COMPATIBLEIDS;

    for (idIndex = 0; idIndex < 2; idIndex++) {

        keyValueInfo[idIndex] = NULL;
        status = IopGetRegistryValue(HDevInstance,
                                     keyValueInfoTag[idIndex],
                                     &keyValueInfo[idIndex]);
        if (!NT_SUCCESS( status )) {

            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: Unable to get some "
                           "%ws value info, %#08lx. continuing.\n",
                           keyValueInfoTag[idIndex], status));

        } else if ((keyValueInfo[idIndex]->Type != REG_MULTI_SZ) ||
                   (keyValueInfo[idIndex]->DataLength == 0)) {

            ExFreePool(keyValueInfo[idIndex]);
            keyValueInfo[idIndex] = NULL;
            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: some %ws"
                           "Key Value Info not in expected format\n",
                           keyValueInfoTag[idIndex]));
        } else {

            //
            // It is good, make sure all of the IDs match '\' replacement policy
            //
            tmpUnicodeString.Buffer = (PWCHAR) KEY_VALUE_DATA(keyValueInfo[idIndex]);
            tmpUnicodeString.Length = (USHORT) keyValueInfo[idIndex]->DataLength;
            tmpUnicodeString.MaximumLength = tmpUnicodeString.Length;

            IopReplaceSeperatorWithPound(&tmpUnicodeString,
                                         &tmpUnicodeString);
        }
    }

    //
    // Get handle to \REGISTRY\MACHINE registry key.
    //
    status = IopOpenRegistryKeyEx( &hRegistryMachine,
                                   NULL,
                                   &CmRegistryMachineName,
                                   KEY_READ
                                   );
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open CriticalDeviceDatabase registry key to enumerate through.
    //
    // This key contains hardware id's for so called "critical" devices.  These
    // are devices for which, for one reason or another, we cannot wait for
    // config manager to bring them on line.  These are primarily devices which
    // are necessary in order to bring the system up into user mode so that
    // config manager can be run (disks, keyboards, video, etc...)
    //
    PiWstrToUnicodeString(&unicodeCriticalDeviceKeyName, REGSTR_PATH_CRITICALDEVICEDATABASE);
    status = IopOpenRegistryKeyEx( &hCriticalDeviceKey,
                                   hRegistryMachine,
                                   &unicodeCriticalDeviceKeyName,
                                   KEY_READ
                                   );
    //
    // Close handle to \REGISTRY\MACHINE.
    //
    ZwClose(hRegistryMachine);

    //
    // Check success in opening CriticalDeviceDatabase key.
    //
    if ( !NT_SUCCESS(status) ) {
        DebugPrint(1, ("IopProcessCriticalDeviceRoutine: Unable to open %wZ key.  exiting...\n",
                       &unicodeCriticalDeviceKeyName));
        goto cleanup;
    }
    DebugPrint(1, ("IopProcessCriticalDeviceRoutine: Successfully opened %wZ key.\n",
                   &unicodeCriticalDeviceKeyName));

    //
    // Allocate a buffer to store KeyValueFullInformation
    // of values from CriticalDeviceDatabase key.
    //
    status = IopAllocateBuffer( &infoBuffer,
                                INITIAL_INFOBUFFER_SIZE );
    if (!NT_SUCCESS(status)) {
        DebugPrint(1, ("IopProcessCriticalDeviceRoutine: Unable to allocate buffer to hold key values.  exiting...\n",
                       &unicodeCriticalDeviceKeyName));
        goto cleanup;
    }

    //
    // Enumerate through Critical Device entries.
    //
    // For each CriticalDeviceDatabase value entry compare with all entries of HardwareId
    // and CompatibleIds REG_MULTI_SZ for a match
    //
    enumIndex = 0;
    while (((status = ZwEnumerateKey( hCriticalDeviceKey,
                                      enumIndex,
                                      KeyBasicInformation,
                                      (PVOID) infoBuffer.Buffer,
                                      infoBuffer.MaxSize,
                                      &resultSize)) != STATUS_NO_MORE_ENTRIES)) {
        if (status == STATUS_BUFFER_OVERFLOW) {
            //
            // Buffer allocated to hold value was too small;
            // resize to specified length, and try again.
            //
            status = IopResizeBuffer( &infoBuffer,
                                      resultSize,
                                      FALSE );
            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: ZwEnumerateKey returned STATUS_BUFFER_OVERFLOW, %#08lx\n",
                           status));
            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: (resizing buffer...)\n"));
            continue;

        } else if (!NT_SUCCESS(status)) {
            //
            // ZwEnumerateKey returned failure status other than
            // STATUS_NO_MORE_ENTRIES or STATUS_BUFFER_OVERFLOW.
            //
            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: ZwEnumerateValueKey failed, %#08lx  exiting...\n",
                           status));
            goto cleanup;
        }
        //
        // Store CriticalDeviceDatabase entry in a unicode string to do
        // case-insensitive comparisons with HardwareId/CompatibleIds entries
        // from the new device's instance key.
        //

        unicodeCriticalEntry.Buffer = ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->Name;
        unicodeCriticalEntry.Length = (USHORT) ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->NameLength;
        unicodeCriticalEntry.MaximumLength = unicodeCriticalEntry.Length;

        DebugPrint(1, ("IopProcessCriticalDeviceRoutine: \t key (%u) enumerated: %wZ\n",
                       enumIndex,
                       &unicodeCriticalEntry));


        //
        // Look at HardwareId and CompatibleIds of new device...
        //
        for ( idIndex=0; idIndex < 2; idIndex++) {

            if (!keyValueInfo[idIndex]){
                //
                // keyValueInfo[idIndex == 0] == REGSTR_VAL_HARDWAREID
                // keyValueInfo[idIndex == 1] == REGSTR_VAL_COMPATIBLEIDS
                //
                // Corresponding HardwareId or CompatibleIds value entry was invalid or not found.
                //      keyValueInfo[idIndex] set to NULL above when
                //      Type != REG_MULTI_SZ or DataLength == 0
                // Just skip, and move on.
                //
                DebugPrint(1, ("IopProcessCriticalDeviceRoutine: No %ws value was found, move on to next\n",
                           keyValueInfoTag[idIndex]));
                continue;
            }

            //
            // Find start and end of this REG_MULTI_SZ
            //
            ptr = (PWCHAR)KEY_VALUE_DATA(keyValueInfo[idIndex]);
            stringStart = ptr;
            bufferEnd = (PWCHAR)((PUCHAR)ptr + keyValueInfo[idIndex]->DataLength);

            while(ptr != bufferEnd) {

                if (!*ptr) {
                    //
                    // Found null-terminated end of a single SZ within the MULTI_SZ.
                    //
                    stringLength = (ULONG)((PUCHAR)ptr - (PUCHAR)stringStart);
                    tmpUnicodeString.Buffer = stringStart;
                    tmpUnicodeString.Length = (USHORT)stringLength;
                    tmpUnicodeString.MaximumLength = (USHORT)stringLength  + sizeof(UNICODE_NULL);

                    DebugPrint(2, ("IopProcessCriticalDeviceRoutine: comparing %wZ with %wZ\n",
                             &tmpUnicodeString, &unicodeCriticalEntry));
                    //
                    // Check for a case-insenitive unicode string match.
                    //
                    if (RtlEqualUnicodeString(&tmpUnicodeString,
                                              &unicodeCriticalEntry,
                                              TRUE)) {

                        DebugPrint(1, ("IopProcessCriticalDeviceRoutine: ***** Critical Device %wZ: Matched to Device %wZ.\n",
                                 &tmpUnicodeString,
                                 &unicodeCriticalEntry));

                        #define NUM_QUERIES 4
                        status = IopOpenRegistryKeyEx( &hCriticalEntry,
                                                       hCriticalDeviceKey,
                                                       &unicodeCriticalEntry,
                                                       KEY_READ
                                                       );

                        if (!NT_SUCCESS(status)) {
                            goto cleanup;
                        }

                        parameters = (PRTL_QUERY_REGISTRY_TABLE)
                            ExAllocatePool(NonPagedPool,
                                           sizeof(RTL_QUERY_REGISTRY_TABLE)*(NUM_QUERIES+1));

                        if (!parameters) {
                            ZwClose(hCriticalEntry);
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            goto cleanup;
                        }

                        RtlZeroMemory(parameters,
                                      sizeof(RTL_QUERY_REGISTRY_TABLE) * (NUM_QUERIES + 1));

                        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
                        parameters[0].Name = REGSTR_VALUE_SERVICE;
                        parameters[0].EntryContext = ServiceName;
                        parameters[0].DefaultType = REG_SZ;
                        parameters[0].DefaultData = L"";
                        parameters[0].DefaultLength = 0;

                        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
                        parameters[1].Name = REGSTR_VALUE_CLASSGUID;
                        parameters[1].EntryContext = ClassGuid;
                        parameters[1].DefaultType = REG_SZ;
                        parameters[1].DefaultData = L"";
                        parameters[1].DefaultLength = 0;

                        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
                        parameters[2].Name = REGSTR_VALUE_LOWERFILTERS;
                        parameters[2].EntryContext = LowerFilters;
                        parameters[2].DefaultType = REG_MULTI_SZ;
                        parameters[2].DefaultData = L"";
                        parameters[2].DefaultLength = 0;

                        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
                        parameters[3].Name = REGSTR_VALUE_UPPERFILTERS;
                        parameters[3].EntryContext = UpperFilters;
                        parameters[3].DefaultType = REG_MULTI_SZ;
                        parameters[3].DefaultData = L"";
                        parameters[3].DefaultLength = 0;

                        status = RtlQueryRegistryValues(
                                     RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                     (PWSTR) hCriticalEntry,
                                     parameters,
                                     NULL,
                                     NULL
                                     );

                        ExFreePool(parameters);
                        ZwClose(hCriticalEntry);

                        if (NT_SUCCESS(status)) {

                            //
                            // Sanity check all of the values...
                            // 1)  There is a service name
                            // 2)  If there is a class guid, it is of the proper length
                            //
                            if (!ServiceName->Buffer || !ServiceName->Length ||
                                (ClassGuid->Length && (ClassGuid->Length < 38 * sizeof (WCHAR)))) {
                                goto FreeStrings;
                            }

                            //
                            // Caller expects XxxFilters->Buffer == NULL, so make
                            // the default case look like that
                            //
                            if (UpperFilters->Length <= 2 && UpperFilters->Buffer) {
                                RtlFreeUnicodeString(UpperFilters);
                            }
                            if (LowerFilters->Length <= 2 && LowerFilters->Buffer) {
                                RtlFreeUnicodeString(LowerFilters);
                            }
                            if (ClassGuid->Length == 0 && ClassGuid->Buffer) {
                                RtlFreeUnicodeString(ClassGuid);
                            }

#if DBG
                            DebugPrint(1, ("IopProcessCriticalDeviceRoutine: ***** Using ServiceName %wZ.\n",
                                     ServiceName));

                            if (ClassGuid->Buffer) {
                                DebugPrint(1, ("IopProcessCriticalDeviceRoutine: ***** Using ClassGuid %wZ.\n",
                                         ClassGuid));
                            }
#endif

                        } else {
FreeStrings:
                            //
                            // Free any strings that may have been allocated by
                            // RtlQueryRegistryValues
                            //
                            RtlFreeUnicodeString(ServiceName);
                            RtlFreeUnicodeString(ClassGuid);
                            RtlFreeUnicodeString(LowerFilters);
                            RtlFreeUnicodeString(UpperFilters);
                        }

                        if (ServiceName->Buffer != NULL) {
                            *FoundMatch = TRUE;
                            goto exit;
                        }
                    }

                    //
                    // See if we're at the end of the MULTI_SZ
                    //
                    if (((ptr + 1) == bufferEnd) || !*(ptr + 1)) {
                        break;
                    } else {
                        stringStart = ptr + 1;
                    }

                }
                //
                // advance to next character.
                //
                ptr++;
            }
        }
        //
        // enumerate next key value.
        //
        enumIndex++;
    }


    //
    // No match found, and no more values to enumerate.
    //
    if (status == STATUS_NO_MORE_ENTRIES) {
        DebugPrint(1, ("IopProcessCriticalDeviceRoutine: No match found for this device.\n"));
    }


exit:
    status = STATUS_SUCCESS;

cleanup:
    if (infoBuffer.Buffer) {
        IopFreeBuffer(&infoBuffer);
    }
    if (hCriticalDeviceKey) {
        ZwClose(hCriticalDeviceKey);
    }
    if (keyValueInfo[0]) {
        ExFreePool(keyValueInfo[0]);
    }
    if (keyValueInfo[1]) {
        ExFreePool(keyValueInfo[1]);
    }
    return status;
}

USHORT
IopGetBusTypeGuidIndex(
    IN LPGUID BusTypeGuid
    )

/*++

Routine Description:

    This routine returns an index into the global table of bus type guids for
    guid specified. If this guid is not already in the table, then it will be
    added and the index for the new table entry will be returned. This index
    is later used by IoGetDeviceProperty to retrieve and return the bus type
    guid back to callers but allows us not to bother storing the entire guid
    in each devnode (just once per guid in the table).

Arguments:

    BusTypeGuid - specifies the guid to retrieve an index for

Return Value:

    The index (into the IopBusTypeGuidList table) for the specified guid.

--*/

{
    PCHAR p;
    USHORT i, busTypeIndex = 0xffff;   // unreported
    ULONG size;

    ASSERT(IopBusTypeGuidList != NULL);

    ExAcquireFastMutex(&IopBusTypeGuidList->Lock);

    //
    // Search the guid list for a match
    //
    for (i = 0; i < (USHORT)IopBusTypeGuidList->Count; i++) {
        if (IopCompareGuid(BusTypeGuid, &IopBusTypeGuidList->Guid[i])) {
            busTypeIndex = i;
            goto Clean0;
        }
    }

    //
    // Bus type guid doesn't exist in the list.
    //

    if (IopBusTypeGuidList->Count > 0) {

        //
        // Reallocate to hold one more guid.
        //

        size = sizeof(BUS_TYPE_GUID_LIST) +
               sizeof(GUID) * (IopBusTypeGuidList->Count);

        p = ExAllocatePool(PagedPool, size);
        if (!p) {
            goto Clean0;    // oops, no more room, return unreported
        }

        size = sizeof(BUS_TYPE_GUID_LIST) +
               sizeof(GUID) * (IopBusTypeGuidList->Count - 1);

        RtlCopyMemory(p, IopBusTypeGuidList, size);

        ExFreePool(IopBusTypeGuidList);
        IopBusTypeGuidList = (PBUS_TYPE_GUID_LIST)p;
    }

    //
    // Add the new guid to the list.
    //
    RtlCopyMemory(&IopBusTypeGuidList->Guid[IopBusTypeGuidList->Count],
                  BusTypeGuid,
                  sizeof(GUID));

    busTypeIndex = (USHORT)IopBusTypeGuidList->Count;
    IopBusTypeGuidList->Count += 1;

Clean0:

    ExReleaseFastMutex(&IopBusTypeGuidList->Lock);

    return busTypeIndex;
}

BOOLEAN
IopFixupDeviceId(
    PWCHAR DeviceId
    )

/*++

Routine Description:

    This routine parses the device instance string and replaces any invalid
    characters (not allowed in a "device instance") with an underscore
    character.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    DeviceId - specifies a device instance string (or part of one), must be
               null-terminated.

Return Value:

    None.

--*/

{
    PWCHAR p;

    // BUGBUG - do we need to uppercase these!?

    for (p = DeviceId; *p; p++) {
        if (*p == L' ') {
            *p = L'_';
        } else if ((*p < L' ')  || (*p > (WCHAR)0x7F) || (*p == L',')) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
IopFixupIds(
    IN PWCHAR Ids,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine parses multiple IDs and replaces any invalid
    characters (not allowed in a "device instance") with an underscore
    character.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    Ids - specifies MULTI_SZ ids.

    Length - Specifies the lenght of Ids

Return Value:

    None.

--*/

{
    PWCHAR p, end;

    p = Ids;
    end = p + Length / sizeof(WCHAR);

    while ((p < end) && (*p)) {
        if (!IopFixupDeviceId(p)) {
            return FALSE;
        }

        while (*p) {
            p++;
        }
        p++;
    }
    return TRUE;
}


BOOLEAN
IopGetRegistryDwordWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey,
    IN OUT PULONG Value)
/*++

Routine Description:

    If
        (1) Primary key has a value named "ValueName" that is REG_DWORD, return it
    Else If
        (2) Secondary key has a value named "ValueName" that is REG_DWORD, return it
    Else
        (3) Leave Value untouched and return error

Arguments:

    ValueName          - Unicode name of value to query
    PrimaryKey         - If non-null, check this first
    SecondaryKey       - If non-null, check this second
    Value              - IN = default value, OUT = actual value

Return Value:

    TRUE if value found

--*/
{
    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;
    NTSTATUS status;
    HANDLE Keys[3];
    int count = 0;
    int index;
    BOOLEAN set = FALSE;

    if (PrimaryKey != NULL) {
        Keys[count++] = PrimaryKey;
    }
    if (SecondaryKey != NULL) {
        Keys[count++] = SecondaryKey;
    }
    Keys[count] = NULL;

    for (index = 0; index < count && !set; index ++) {
        info = NULL;
        try {
            status = IopGetRegistryValue(Keys[index],
                                         valueName->Buffer,
                                         &info);
            if (NT_SUCCESS(status) && info->Type == REG_DWORD) {
                data = ((PUCHAR) info) + info->DataOffset;
                *Value = *((PULONG) data);
                set = TRUE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing
            //
        }
        if (info) {
            ExFreePool(info);
        }
    }
    return set;
}

PSECURITY_DESCRIPTOR
IopGetRegistrySecurityWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey)
/*++

Routine Description:

    If
        (1) Primary key has a binary value named "ValueName" that is
        REG_BINARY and appears to be a valid security descriptor, return it
    Else
        (2) do same for Secondary key
    Else
        (3) Return NULL

Arguments:

    ValueName          - Unicode name of value to query
    PrimaryKey         - If non-null, check this first
    SecondaryKey       - If non-null, check this second

Return Value:

    Security Descriptor if found, else NULL

--*/
{
    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;
    NTSTATUS status;
    HANDLE Keys[3];
    int count = 0;
    int index;
    BOOLEAN set = FALSE;
    PSECURITY_DESCRIPTOR secDesc = NULL;
    PSECURITY_DESCRIPTOR allocDesc = NULL;

    if (PrimaryKey != NULL) {
        Keys[count++] = PrimaryKey;
    }
    if (SecondaryKey != NULL) {
        Keys[count++] = SecondaryKey;
    }
    Keys[count] = NULL;

    for (index = 0; index < count && !set; index ++) {
        info = NULL;
        try {
            status = IopGetRegistryValue(Keys[index],
                                         valueName->Buffer,
                                         &info);
            if (NT_SUCCESS(status) && info->Type == REG_BINARY) {
                data = ((PUCHAR) info) + info->DataOffset;
                secDesc = (PSECURITY_DESCRIPTOR)data;
                status = SeCaptureSecurityDescriptor(secDesc,
                                             KernelMode,
                                             PagedPool,
                                             TRUE,
                                             &allocDesc);
                if (NT_SUCCESS(status)) {
                    set = TRUE;
                }
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing
            //
        }
        if (info) {
            ExFreePool(info);
        }
    }
    if (set) {
        return allocDesc;
    }
    return NULL;
}

NTSTATUS
IopChangeDeviceObjectFromRegistryProperties(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN HANDLE DeviceClassPropKey,
    IN HANDLE DevicePropKey,
    IN BOOLEAN UsePdoCharacteristics
    )
/*++

Routine Description:

    This routine will obtain settings from either
    (1) DevNode settings (via DevicePropKey) or
    (2) Class settings (via DeviceClassPropKey)
    applying to PDO and all attached device objects

    Properties set/ changed are:

        * DeviceType - the I/O system type for the device object
        * DeviceCharacteristics - the I/O system characteristic flags to be
                                  set for the device object
        * Exclusive - the device can only be accessed exclusively
        * Security - security for the device

    The routine will then use the DeviceType and DeviceCharacteristics specified
    to determine whether a VPB should be allocated as well as to set default
    security if none is specified in the registry.

Arguments:

    PhysicalDeviceObject - the PDO we are to configure

    DeviceClassPropKey - a handle to Control\<Class>\Properties protected key
    DevicePropKey      - a handle to Enum\<Instance>  protected key

Return Value:

    status

--*/

{
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;
    NTSTATUS status;

    BOOLEAN deviceTypeSpec = FALSE;
    BOOLEAN characteristicsSpec = FALSE;
    BOOLEAN exclusiveSpec = FALSE;
    BOOLEAN securityForce = FALSE;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_INFORMATION securityInformation = 0;

    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    PACL allocatedAcl = NULL;
    ULONG deviceType = 0;
    ULONG characteristics = 0;
    ULONG exclusive = 0;
    ULONG prevCharacteristics = 0;
    ULONG prevExclusive = 0;
    PDEVICE_OBJECT StackIterator = NULL;
    PDEVICE_NODE deviceNode = NULL;

    ASSERT(PhysicalDeviceObject);
    deviceNode = PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);

    DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Modifying device stack for PDO: %08x\n",PhysicalDeviceObject));

    //
    // Iterate through all device objects to get our starting settings (OR everyone together)
    // generally, a PDO should take on the characteristics of whoever is above the PDO, and not used in the equation
    // the exception being if it's being used RAW
    // we detect this by absense of service name, or it's the only Device Object.
    //
    StackIterator = PhysicalDeviceObject;
    if (UsePdoCharacteristics || StackIterator->AttachedDevice == NULL) {
        DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Assuming PDO is being used RAW\n"));
    } else {
        StackIterator = StackIterator->AttachedDevice;
        DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Ignoring PDO's settings\n"));
    }
    for ( ; StackIterator != NULL; StackIterator = StackIterator->AttachedDevice) {
#if 0 // BUGBUG (jamiehun) we were breaking serial, I'd like to put this back in though...
        prevExclusive |= StackIterator->Flags & DO_EXCLUSIVE;
#endif
        prevCharacteristics |= StackIterator->Characteristics;
    }

    //
    // 1) Get Device type, DevicePropKey preferred over DeviceClassPropKey
    //
    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_TYPE);
    deviceTypeSpec = IopGetRegistryDwordWithFallback(&valueName,DevicePropKey,DeviceClassPropKey,&deviceType);
    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_CHARACTERISTICS);
    characteristicsSpec = IopGetRegistryDwordWithFallback(&valueName,DevicePropKey,DeviceClassPropKey,&characteristics);
    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_EXCLUSIVE);
    exclusiveSpec = IopGetRegistryDwordWithFallback(&valueName,DevicePropKey,DeviceClassPropKey,&exclusive);

    if (exclusive) {
        //
        // make sure a TRUE maps to a bit-mask
        //
        exclusive = DO_EXCLUSIVE;
    }

    if (exclusiveSpec) {
        exclusive |= prevExclusive;
    } else {
        exclusive = prevExclusive;
    }
    if (!characteristicsSpec) {
        characteristics = 0;
    }
    characteristics = (characteristics | prevCharacteristics) & FILE_CHARACTERISTICS_PROPAGATED; // mask only applicable characteristics

    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR);
    securityDescriptor = IopGetRegistrySecurityWithFallback(&valueName,DevicePropKey,DeviceClassPropKey);

    if (securityDescriptor == NULL) {
        //
        // determine if we should create internal default
        //
        if (deviceTypeSpec) {
            BOOLEAN hasName = (PhysicalDeviceObject->Flags & DO_DEVICE_HAS_NAME) ? TRUE : FALSE;

            securityDescriptor = IopCreateDefaultDeviceSecurityDescriptor(
                                    (DEVICE_TYPE)deviceType,
                                    characteristics,
                                    hasName,
                                    buffer,
                                    &allocatedAcl,
                                    &securityInformation
                                    );
            if (securityDescriptor) {
                securityForce = TRUE; // forced default security descriptor
            } else {
                DebugPrint(1, ("IopChangeDeviceObjectFromRegistryProperties: Was not able to get default security descriptor\n"));
            }
        }
    } else {
        //
        // further process the security information we're given to set "securityInformation"
        //
        PSID sid;
        PACL acl;
        BOOLEAN present, tmp;

        securityInformation = 0;

        //
        // See what information is in the captured descriptor so we can build
        // up a securityInformation block to go with it.
        //

        status = RtlGetOwnerSecurityDescriptor(securityDescriptor, &sid, &tmp);

        if (NT_SUCCESS(status) && (sid != NULL)) {
            securityInformation |= OWNER_SECURITY_INFORMATION;
        }

        status = RtlGetGroupSecurityDescriptor(securityDescriptor, &sid, &tmp);

        if (NT_SUCCESS(status) && (sid != NULL)) {
            securityInformation |= GROUP_SECURITY_INFORMATION;
        }

        status = RtlGetSaclSecurityDescriptor(securityDescriptor,
                                              &present,
                                              &acl,
                                              &tmp);

        if (NT_SUCCESS(status) && (present)) {
            securityInformation |= SACL_SECURITY_INFORMATION;
        }

        status = RtlGetDaclSecurityDescriptor(securityDescriptor,
                                              &present,
                                              &acl,
                                              &tmp);

        if (NT_SUCCESS(status) && (present)) {
            securityInformation |= DACL_SECURITY_INFORMATION;
        }

    }

#if DBG
    if (deviceTypeSpec == FALSE && characteristicsSpec == FALSE && exclusiveSpec == FALSE && securityDescriptor == NULL) {
        DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: No property changes\n"));
    } else {
        if (deviceTypeSpec) {
            DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Overide DeviceType=%08x\n",
                           deviceType));
        }
        if (characteristicsSpec) {
            DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Overide DeviceCharacteristics=%08x\n",
                           characteristics));
        }
        if (exclusiveSpec) {
            DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Overide Exclusive=%d\n",(exclusive?1:0)));
        }
        if (securityForce) {
            DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Overide Security based on DeviceType & DeviceCharacteristics\n"));
        }
        if (securityDescriptor == NULL) {
            DebugPrint(2, ("IopChangeDeviceObjectFromRegistryProperties: Overide Security\n"));
        }
    }
#endif
    //
    // modify apropriate characteristics of PDO to be the same as those of rest of stack
    // eg, PDO may be initialized as Raw-Capable Secure Open, but then be modified to be more lax
    //
    PhysicalDeviceObject->Characteristics = (PhysicalDeviceObject->Characteristics & ~FILE_CHARACTERISTICS_PROPAGATED) | characteristics;
    ASSERT((PhysicalDeviceObject->Characteristics & FILE_CHARACTERISTICS_PROPAGATED) == characteristics); // sanity (checks bit bounds)
    //
    // modify exclusivity of PDO to be the same as those of rest of stack
    // eg, PDO may be initialized as Exclusive open, but be modified to be more lax
    //
#if 0 // BUGBUG (jamiehun) we were breaking serial, I'd like to put this back in though...
    PhysicalDeviceObject->Flags = (PhysicalDeviceObject->Flags & ~DO_EXCLUSIVE) | exclusive;
    ASSERT((PhysicalDeviceObject->Flags & DO_EXCLUSIVE) == exclusive); // sanity (checks bit bounds)
#endif

    //
    // iterate through rest of objects
    // these flags were used to create characteristics & deviceType, so
    // we will only end up setting flags, not clearing them
    //
    for ( StackIterator = PhysicalDeviceObject->AttachedDevice ; StackIterator != NULL ; StackIterator = StackIterator->AttachedDevice) {
        //
        // modify characteristics (set only)
        //
        StackIterator->Characteristics |= characteristics;
        ASSERT((StackIterator->Characteristics & FILE_CHARACTERISTICS_PROPAGATED) == characteristics); // sanity (checks we only needed to set)
        //
        // modify exclusivity flag (set only)
        //
        StackIterator->Flags |= exclusive;
#if 0 // BUGBUG (jamiehun) we were breaking serial, I'd like to put this back in though...
        ASSERT((StackIterator->Flags & DO_EXCLUSIVE) == exclusive); // sanity (checks we only needed to set)
#endif
    }
    if (deviceTypeSpec) {
        //
        // modify device type - PDO only
        //
        PhysicalDeviceObject->DeviceType = deviceType;
    }
    if (securityDescriptor != NULL) {
        //
        // modify security (applied to whole stack)
        //
        status = ObSetSecurityObjectByPointer(PhysicalDeviceObject,
                                              securityInformation,
                                              securityDescriptor);
        if (NT_SUCCESS(status) == FALSE) {
            DebugPrint(1, ("IopChangeDeviceObjectFromRegistryProperties: Set security failed (%08x)\n",status));
        }
    }
    //
    // cleanup
    //
    if ((securityDescriptor != NULL) && !securityForce) {
        ExFreePool(securityDescriptor);
    }
    if (allocatedAcl) {
        ExFreePool(allocatedAcl);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopProcessNewProfile(
    VOID
    )

/*++

Routine Description:

    This function is called after the system has transitioned into a new
    hardware profile. The thread from which it is called may be holding an
    enumeration lock. Calling this function does two tasks:

    1) If a disabled devnode in the tree should be enabled in this new hardware
       profile state, it will be started.

    2) If an enabled devnode in the tree should be disabled in this new hardware
       profile state, it will be (surprise) removed.

    ADRIAO N.B. 02/19/1999 -
        Why surprise remove? There are four cases to be handled:
        a) Dock disappearing, need to enable device in new profile
        b) Dock appearing, need to enable device in new profile
        c) Dock disappearing, need to disable device in new profile
        d) Dock appearing, need to disable device in new profile

        a) and b) are trivial. c) involves treating the appropriate devices as
        if they were in the removal relation lists for the dock. d) is another
        matter altogether as we need to query-remove/remove devices before
        starting another. NT5's PnP state machine cannot handle this, so for
        this release we cleanup rather hastily after the profile change.

Parameters:

    NONE.

Return Value:

    NTSTATUS.

--*/
{
    PWORK_QUEUE_ITEM workQueueItem;

    PAGED_CODE();

    workQueueItem = (PWORK_QUEUE_ITEM) ExAllocatePool(
        NonPagedPool,
        sizeof(WORK_QUEUE_ITEM)
        );

    if (workQueueItem) {

        //
        // Queue this up so we can walk the tree outside of the enumeration lock.
        //
        ExInitializeWorkItem(
            workQueueItem,
            IopProcessNewProfileWorker,
            workQueueItem
            );

        ExQueueWorkItem(
            workQueueItem,
            CriticalWorkQueue
            );

        return STATUS_SUCCESS;

    } else {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

VOID
IopProcessNewProfileWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called for each devnode after the system has transitioned
    to a new hardware profile.

Parameters:

    NONE.

Return Value:

    NONE.

--*/
{
    PAGED_CODE();

    IopForAllDeviceNodes(IopProcessNewProfileStateCallback, NULL);

    ExFreePool(Context);
}

NTSTATUS
IopProcessNewProfileStateCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called for each devnode after the system has transitioned
    hardware profile states.

Parameters:

    NONE.

Return Value:

    NONE.

--*/
{
    PDEVICE_NODE parentDevNode;

    PAGED_CODE();

    if (DeviceNode->Flags & DNF_STARTED) {

        //
        // Calling this function will disable the device if it is appropriate
        // to do so.
        //
        if (!IopIsDeviceInstanceEnabled(NULL, &DeviceNode->InstancePath, FALSE)) {

            IopRequestDeviceRemoval(
                DeviceNode->PhysicalDeviceObject,
                CM_PROB_DISABLED
                );
        }

    } else if (IopIsDevNodeProblem(DeviceNode, CM_PROB_DISABLED)) {

        //
        // We might be turning on the device. So we will clear the problem
        // flags iff the device problem was CM_PROB_DISABLED.
        //
        IopClearDevNodeProblem(DeviceNode);

        //
        // Make sure the device stays down iff appropriate.
        //
        if (IopIsDeviceInstanceEnabled(NULL, &DeviceNode->InstancePath, FALSE)) {

            //
            // This device should come back online. Queue up an enumeration
            // at the parent level for him.
            //
            parentDevNode = DeviceNode->Parent;

            IoInvalidateDeviceRelations(
                parentDevNode->PhysicalDeviceObject,
                BusRelations
                );
        }
    }

    return STATUS_SUCCESS;
}
