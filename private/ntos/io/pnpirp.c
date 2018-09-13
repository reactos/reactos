/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains IRP related routines.

Author:

    Shie-Lin Tzong (shielint) 13-Sept-1996

Revision History:

--*/

#include "iop.h"
#pragma hdrstop

#if DBG_SCOPE

#define PnpIrpStatusTracking(Status, IrpCode, Device)                    \
    if (PnpIrpMask & (1 << IrpCode)) {                                   \
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING) {           \
            DbgPrint(" ++ %s Driver ( %wZ ) return status %08lx\n",      \
                     IrpName[IrpCode],                                   \
                     &Device->DriverObject->DriverName,                  \
                     Status);                                            \
        }                                                                \
    }

ULONG PnpIrpMask;
PCHAR IrpName[] = {
    "IRP_MN_START_DEVICE - ",                 // 0x00
    "IRP_MN_QUERY_REMOVE_DEVICE - ",          // 0x01
    "IRP_MN_REMOVE_DEVICE - ",                // 0x02
    "IRP_MN_CANCEL_REMOVE_DEVICE - ",         // 0x03
    "IRP_MN_STOP_DEVICE - ",                  // 0x04
    "IRP_MN_QUERY_STOP_DEVICE - ",            // 0x05
    "IRP_MN_CANCEL_STOP_DEVICE - ",           // 0x06
    "IRP_MN_QUERY_DEVICE_RELATIONS - ",       // 0x07
    "IRP_MN_QUERY_INTERFACE - ",              // 0x08
    "IRP_MN_QUERY_CAPABILITIES - ",           // 0x09
    "IRP_MN_QUERY_RESOURCES - ",              // 0x0A
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS - ",  // 0x0B
    "IRP_MN_QUERY_DEVICE_TEXT - ",            // 0x0C
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS - ", // 0x0D
    "INVALID_IRP_CODE - ",                    //
    "IRP_MN_READ_CONFIG - ",                  // 0x0F
    "IRP_MN_WRITE_CONFIG - ",                 // 0x10
    "IRP_MN_EJECT - ",                        // 0x11
    "IRP_MN_SET_LOCK - ",                     // 0x12
    "IRP_MN_QUERY_ID - ",                     // 0x13
    "IRP_MN_QUERY_PNP_DEVICE_STATE - ",       // 0x14
    "IRP_MN_QUERY_BUS_INFORMATION - ",        // 0x15
    "IRP_MN_DEVICE_USAGE_NOTIFICATION - ",    // 0x16
    NULL
};
#else
#define PnpIrpStatusTracking(Status, IrpCode, Device)
#endif

//
// Hash routine from CNTFS (see cntfs\prefxsup.c)
// (used here in the construction of unique ids)
//

#define HASH_UNICODE_STRING( _pustr, _phash ) {                             \
    PWCHAR _p = (_pustr)->Buffer;                                           \
    PWCHAR _ep = _p + ((_pustr)->Length/sizeof(WCHAR));                     \
    ULONG _chHolder =0;                                                     \
                                                                            \
    while( _p < _ep ) {                                                     \
        _chHolder = 37 * _chHolder + (unsigned int) (*_p++);                \
    }                                                                       \
                                                                            \
    *(_phash) = abs(314159269 * _chHolder) % 1000000007;                    \
}

// Parent prefixes are of the form %x&%x&%x
#define MAX_PARENT_PREFIX (8 + 8 + 8 + 2)

//
// Internal definitions
//

typedef struct _DEVICE_COMPLETION_CONTEXT {
    PDEVICE_NODE DeviceNode;
    ERESOURCE_THREAD Thread;
    ULONG IrpMinorCode;
#if DBG
    PVOID Id;
#endif
} DEVICE_COMPLETION_CONTEXT, *PDEVICE_COMPLETION_CONTEXT;

typedef struct _LOCK_MOUNTABLE_DEVICE_CONTEXT{
    PDEVICE_OBJECT MountedDevice;
} LOCK_MOUNTABLE_DEVICE_CONTEXT, *PLOCK_MOUNTABLE_DEVICE_CONTEXT;

//
// Internal references
//

NTSTATUS
IopAsynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN PDEVICE_COMPLETION_CONTEXT CompletionContext,
    IN PVOID CompletionRoutine
    );

NTSTATUS
IopDeviceEjectComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IopDeviceStartComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IopDeviceRelationsComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PDEVICE_OBJECT
IopFindMountableDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

PDEVICE_OBJECT
IopLockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    OUT PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    );

VOID
IopUnlockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    IN PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    );

NTSTATUS
IopFilterResourceRequirementsCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResReqList,
    OUT PVOID *Information
    );

//
// External reference
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAsynchronousCall)
#pragma alloc_text(PAGE, IopSynchronousCall)
//#pragma alloc_text(PAGE, IopStartDevice)
#pragma alloc_text(PAGE, IopEjectDevice)
#pragma alloc_text(PAGE, IopRemoveDevice)
//#pragma alloc_text(PAGE, IopQueryDeviceRelations)
#pragma alloc_text(PAGE, IopQueryDeviceId)
#pragma alloc_text(PAGE, IopQueryUniqueId)
#pragma alloc_text(PAGE, IopQueryCompatibleIds)
#pragma alloc_text(PAGE, IopQueryDeviceResources)
#pragma alloc_text(PAGE, IopQueryDockRemovalInterface)
#pragma alloc_text(PAGE, IopQueryLegacyBusInformation)
#pragma alloc_text(PAGE, IopQueryPnpBusInformation)
#pragma alloc_text(PAGE, IopQueryResourceHandlerInterface)
#pragma alloc_text(PAGE, IopQueryReconfiguration)
#pragma alloc_text(PAGE, IopUncacheInterfaceInformation)
#pragma alloc_text(PAGE, IopFindMountableDevice)
#pragma alloc_text(PAGE, IopFilterResourceRequirementsCall)
#pragma alloc_text(PAGE, IopMakeGloballyUniqueId)

#endif  // ALLOC_PRAGMA

VOID
IopUncacheInterfaceInformation(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function removes all the cached translators and arbiters information
    from the device object.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PLIST_ENTRY listHead, nextEntry, entry;
    PPI_RESOURCE_TRANSLATOR_ENTRY handlerEntry;
    PINTERFACE interface;

    //
    // Dereference all the arbiters on this PDO.
    //

    listHead = &deviceNode->DeviceArbiterList;
    nextEntry = listHead->Flink;
    while (nextEntry != listHead) {
        entry = nextEntry;
        nextEntry = nextEntry->Flink;
        handlerEntry = CONTAINING_RECORD(entry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);
        if (interface = (PINTERFACE)handlerEntry->TranslatorInterface) {
            (interface->InterfaceDereference)(interface->Context);
            ExFreePool(interface);
        }
        ExFreePool(entry);
    }
    InitializeListHead(&deviceNode->DeviceArbiterList);

    //
    // Dereference all the translators on this PDO.
    //

    listHead = &deviceNode->DeviceTranslatorList;
    nextEntry = listHead->Flink;
    while (nextEntry != listHead) {
        entry = nextEntry;
        nextEntry = nextEntry->Flink;
        handlerEntry = CONTAINING_RECORD(entry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);
        if (interface = (PINTERFACE)handlerEntry->TranslatorInterface) {
            (interface->InterfaceDereference)(interface->Context);
            ExFreePool(interface);
        }
        ExFreePool(entry);
    }

    InitializeListHead(&deviceNode->DeviceTranslatorList);

    deviceNode->NoArbiterMask = 0;
    deviceNode->QueryArbiterMask = 0;
    deviceNode->NoTranslatorMask = 0;
    deviceNode->QueryTranslatorMask = 0;
}

NTSTATUS
IopAsynchronousCall(
    IN PDEVICE_OBJECT TargetDevice,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN PDEVICE_COMPLETION_CONTEXT CompletionContext,
    IN PVOID CompletionRoutine
    )

/*++

Routine Description:

    This function sends an  Asynchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

    CompletionContext -

    CompletionRoutine -

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    //
    // If the target device object has a VPB associated with it and a file
    // system is mounted, make the filesystem's volume device object the
    // irp target.
    //
    // BUGBUG - I don't think we need to do this.  The filesystem
    // driver should attach to the PDO chain or registered for device
    // change registration.
    //

    if ((TargetDevice->Vpb != NULL) && (TargetDevice->Vpb->Flags & VPB_MOUNTED)) {
        deviceObject = TargetDevice->Vpb->DeviceObject;
    } else {
        deviceObject = TargetDevice;
    }

    ASSERT(deviceObject != NULL);

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(deviceObject);

    //
    // Allocate an I/O Request Packet (IRP) for this device removal operation.
    //

    irp = IoAllocateIrp( (CCHAR) (deviceObject->StackSize), FALSE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation (irp);

    //
    // Fill in the IRP according to this request.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;
#if DBG
    CompletionContext->Id = irp;
#endif
    IoSetCompletionRoutine(irp,
                           CompletionRoutine,
                           CompletionContext,  /* Completion context */
                           TRUE,               /* Invoke on success  */
                           TRUE,               /* Invoke on error    */
                           TRUE                /* Invoke on cancel   */
                           );

    status = IoCallDriver( deviceObject, irp );
    return status;
}

NTSTATUS
IopSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    OUT PVOID *Information
    )

/*++

Routine Description:

    This function sends a synchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

    Information - Supplies a pointer to a variable to receive the returned
                  information of the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PULONG_PTR returnInfo = (PULONG_PTR)Information;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = statusBlock.Information = 0;

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    irp->UserIosb = &statusBlock;
    irp->UserEvent = &event;

    //
    // Set the address of the current thread
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue this irp onto the current thread
    //

    IopQueueThreadIrp(irp);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;

    //
    // Call the driver
    //

    status = IoCallDriver(deviceObject, irp);

    PnpIrpStatusTracking(status, TopStackLocation->MinorFunction, deviceObject);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    *returnInfo = (ULONG_PTR) statusBlock.Information;

    return status;
}

VOID
IopStartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function sends a start device irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the pointer to the device object of the device
                   being removed.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    PVOID dummy;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PDEVICE_COMPLETION_CONTEXT completionContext;
    NTSTATUS status;
    PNP_VETO_TYPE  vetoType;

//    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_START_DEVICE;

    //
    // Set the pointers for the raw and translated resource lists
    //

    if (!(deviceNode->Flags & DNF_RESOURCE_REPORTED)) {
        irpSp.Parameters.StartDevice.AllocatedResources = deviceNode->ResourceList;
        irpSp.Parameters.StartDevice.AllocatedResourcesTranslated = deviceNode->ResourceListTranslated;
    }

    if (!(deviceNode->Flags & DNF_STOPPED)) {
        IopUncacheInterfaceInformation(DeviceObject);
    }

    if (PnpAsyncOk) {

        //
        // ADRIAO BUGBUG 11/18/98 -
        //     The dock code has not been duplicated in the async case, but as
        // PnpAsync support is totally broken and disabled anyway, this should
        // not matter right now.
        //
        ASSERT(0);
        completionContext = (PDEVICE_COMPLETION_CONTEXT) ExAllocatePool(
                              NonPagedPool,
                              sizeof(DEVICE_COMPLETION_CONTEXT));
        if (completionContext == NULL) {
            return;   // BUGBUG - Should try it again *explicitly*
        }

        completionContext->DeviceNode = deviceNode;
        completionContext->IrpMinorCode = IRP_MN_START_DEVICE;
        completionContext->Thread = ExGetCurrentResourceThread();

        //
        // Make the call and return.
        //

        IopAcquireEnumerationLock(NULL);  // To block IopAcquireTreeLock();
        status = IopAsynchronousCall(DeviceObject, &irpSp, completionContext, IopDeviceStartComplete);
        if (status == STATUS_PENDING) {

            KIRQL oldIrql;

            //
            // Set DNF_START_REQUEST_PENDING flag such that the completion routine knows it
            // needs to request enumeration when the start completed successfully.
            //

            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

            if (!(IopDoesDevNodeHaveProblem(deviceNode) || (deviceNode->Flags & DNF_STARTED))) {
                deviceNode->Flags |= DNF_START_REQUEST_PENDING;
            }
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        }
    } else {

        if ((deviceNode->Flags & DNF_STOPPED)||
            (deviceNode->DockInfo.DockStatus == DOCK_NOTDOCKDEVICE)) {


            //
            // This is either the rebalance case in which we are restarting a
            // temporarily stopped device, or we are starting a non-dock device,
            // and it was previously off.
            //
            status = IopSynchronousCall(DeviceObject, &irpSp, &dummy);

        } else {

            //
            // This is a dock so we a little bit of work before starting it.
            // Take the profile change semaphore. We do this whenever a dock
            // is in our list, even if no query is going to occur.
            //
            IopHardwareProfileBeginTransition(FALSE);

            //
            // Tell the profile code what dock device object may be bringing the
            // new hardware profile online.
            //
            IopHardwareProfileMarkDock(deviceNode, DOCK_ARRIVING);

            //
            // Ask everyone if this is really a good idea right now. Note that
            // PiProcessStart calls IopNewDevice calls IopStartAndEnumerateDevice
            // who calls this function on that thread. Therefore we may indded be
            // in an event, although this would probably be a fairly rare event
            // for a dock.
            //
            status = IopHardwareProfileQueryChange(
                FALSE,
                PROFILE_PERHAPS_IN_PNPEVENT,
                &vetoType,
                NULL
                );

            if (NT_SUCCESS(status)) {

                status = IopSynchronousCall(DeviceObject, &irpSp, &dummy);
            }

            if (NT_SUCCESS(status)) {

                //
                // Commit the current Hardware Profile as necessary.
                //
                IopHardwareProfileCommitStartedDock(deviceNode);

            } else {

                IopHardwareProfileCancelTransition();
            }
        }

        if (!NT_SUCCESS(status)) {
            ULONG Problem = CM_PROB_FAILED_START;

            SAVE_FAILURE_INFO(deviceNode, status);

            //
            // Handle certain problems determined by the status code
            //
            switch (status) {
                case STATUS_PNP_REBOOT_REQUIRED:
                    Problem = CM_PROB_NEED_RESTART;
                    break;

                default:
                    Problem = CM_PROB_FAILED_START;
            }
            IopSetDevNodeProblem(deviceNode, Problem);

            if (deviceNode->DockInfo.DockStatus == DOCK_NOTDOCKDEVICE) {

                IopRequestDeviceRemoval(deviceNode->PhysicalDeviceObject, CM_PROB_FAILED_START);
            } else {

                ASSERT(deviceNode->DockInfo.DockStatus == DOCK_QUIESCENT);

                PpSetTargetDeviceRemove( deviceNode->PhysicalDeviceObject,
                                         FALSE,
                                         TRUE,
                                         TRUE,
                                         CM_PROB_DEVICE_NOT_THERE,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);
            }

        } else {

            IopDoDeferredSetInterfaceState(deviceNode);

            deviceNode->Flags |= DNF_STARTED;

            if (deviceNode->Flags & DNF_STOPPED) {

                //
                // If the start is initiated by rebalancing, do NOT do enumeration
                //

                deviceNode->Flags &= ~DNF_STOPPED;

            } else {

                deviceNode->Flags |= DNF_NEED_ENUMERATION_ONLY;

            }
        }
    }
}

NTSTATUS
IopEjectDevice(
    IN      PDEVICE_OBJECT                  DeviceObject,
    IN OUT  PPENDING_RELATIONS_LIST_ENTRY   PendingEntry
    )

/*++

Routine Description:

    This function sends an eject device irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies a pointer to the device object of the device being
                   removed.

Return Value:

    NTSTATUS code.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;

    PAGED_CODE();

    if (PendingEntry->LightestSleepState != PowerSystemWorking) {

        //
        // We have to warm eject.
        //
        if (PendingEntry->DockInterface) {

            PendingEntry->DockInterface->ProfileDepartureSetMode(
                PendingEntry->DockInterface->Context,
                PDS_UPDATE_ON_EJECT
                );
        }

        PendingEntry->EjectIrp = NULL;

        InitializeListHead( &PendingEntry->Link );

        IopQueuePendingEject(PendingEntry);

        ExInitializeWorkItem( &PendingEntry->WorkItem,
                              IopProcessCompletedEject,
                              PendingEntry);

        ExQueueWorkItem( &PendingEntry->WorkItem, DelayedWorkQueue );
        return STATUS_SUCCESS;
    }

    if (PendingEntry->DockInterface) {

        //
        // Notify dock that now is a good time to update it's hardware profile.
        //
        PendingEntry->DockInterface->ProfileDepartureSetMode(
            PendingEntry->DockInterface->Context,
            PDS_UPDATE_ON_INTERFACE
            );

        PendingEntry->DockInterface->ProfileDepartureUpdate(
            PendingEntry->DockInterface->Context
            );

        if (PendingEntry->DisplaySafeRemovalDialog) {

            PpNotifyUserModeRemovalSafe(DeviceObject);
            PendingEntry->DisplaySafeRemovalDialog = FALSE;
        }
    }

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Allocate an I/O Request Packet (IRP) for this device removal operation.
    //

    irp = IoAllocateIrp( (CCHAR) (deviceObject->StackSize), FALSE );
    if (!irp) {

        PendingEntry->EjectIrp = NULL;

        InitializeListHead( &PendingEntry->Link );

        IopQueuePendingEject(PendingEntry);

        ExInitializeWorkItem( &PendingEntry->WorkItem,
                              IopProcessCompletedEject,
                              PendingEntry);

        ExQueueWorkItem( &PendingEntry->WorkItem, DelayedWorkQueue );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_EJECT;

    //
    // Fill in the IRP according to this request.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    PendingEntry->EjectIrp = irp;

    IopQueuePendingEject(PendingEntry);

    IoSetCompletionRoutine(irp,
                           IopDeviceEjectComplete,
                           PendingEntry,       /* Completion context */
                           TRUE,               /* Invoke on success  */
                           TRUE,               /* Invoke on error    */
                           TRUE                /* Invoke on cancel   */
                           );

    status = IoCallDriver( deviceObject, irp );

    return status;
}

NTSTATUS
IopDeviceEjectComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;
    PIRP ejectIrp;

    ejectIrp = InterlockedExchangePointer(&entry->EjectIrp, NULL);

    ASSERT(ejectIrp == NULL || ejectIrp == Irp);

    //
    // Queue a work item to finish up the eject.  We queue a work item because
    // we are probably running at dispatch level in some random context.
    //

    ExInitializeWorkItem( &entry->WorkItem,
                          IopProcessCompletedEject,
                          entry);

    ExQueueWorkItem( &entry->WorkItem, DelayedWorkQueue );

    IoFreeIrp( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
IopRemoveDevice (
    IN PDEVICE_OBJECT TargetDevice,
    IN ULONG IrpMinorCode
    )

/*++

Routine Description:

    This function sends a requested DeviceRemoval related irp to the top level device
    object which roots on TargetDevice.  If there is a VPB associated with the
    TargetDevice, the corresponding filesystem's VDO will be used.  Otherwise
    the irp will be sent directly to the target device/ or its assocated device
    object.

Parameters:

    TargetDevice - Supplies the device object of the device being removed.

    Operation - Specifies the operation requested.
        The following IRP codes are used with IRP_MJ_DEVICE_CHANGE for removing
        devices:
            IRP_MN_QUERY_REMOVE_DEVICE
            IRP_MN_CANCEL_REMOVE_DEVICE
            IRP_MN_REMOVE_DEVICE
            IRP_MN_EJECT
Return Value:

    NTSTATUS code.

--*/
{
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    BOOLEAN isMountable = FALSE;
    PDEVICE_OBJECT mountedDevice;

    PVOID dummy;
    LOCK_MOUNTABLE_DEVICE_CONTEXT lockContext;

    PAGED_CODE();

    ASSERT(IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_CANCEL_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_SURPRISE_REMOVAL ||
           IrpMinorCode == IRP_MN_EJECT);

    if (IrpMinorCode == IRP_MN_REMOVE_DEVICE ||
        IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE) {
        IopUncacheInterfaceInformation(TargetDevice);
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = (UCHAR)IrpMinorCode;

    //
    // BUGBUG - this should probably go at a higher level but then it goes
    // in way too many places.  The only thing we do to the VPB is make sure
    // that it won't go away while the operation is in the file system and
    // that no one new can mount on the device if the FS decides to bail out.
    //

    //
    // Check to see if there's a VPB anywhere in the device stack.  If there
    // is then we'll have to lock the stack.
    //

    mountedDevice = IopFindMountableDevice(TargetDevice);

    if (mountedDevice != NULL) {

        //
        // This routine will cause any mount operations on the VPB to fail.
        // It will also release the VPB spinlock.
        //

        mountedDevice = IopLockMountedDeviceForRemove(TargetDevice,
                                                      IrpMinorCode,
                                                      &lockContext);

        isMountable = TRUE;

    } else {
        ASSERTMSG("Mass storage device does not have VPB - this is odd",
                  !((TargetDevice->Type == FILE_DEVICE_DISK) ||
                    (TargetDevice->Type == FILE_DEVICE_CD_ROM) ||
                    (TargetDevice->Type == FILE_DEVICE_TAPE) ||
                    (TargetDevice->Type == FILE_DEVICE_VIRTUAL_DISK)));

        mountedDevice = TargetDevice;
    }

    //
    // Make the call and return.
    //

    if (IrpMinorCode == IRP_MN_SURPRISE_REMOVAL || IrpMinorCode == IRP_MN_REMOVE_DEVICE) {
        //
        // if device was not disableable, we cleanup the tree
        // and debug-trace that we surprise-removed a non-disableable device
        //
        PDEVICE_NODE deviceNode = TargetDevice->DeviceObjectExtension->DeviceNode;

        if (deviceNode->UserFlags & DNUF_NOT_DISABLEABLE) {
            //
            // this device was marked as disableable, update the depends
            // before this device disappears
            // (by momentarily marking this node as disableable)
            //
            deviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;
            IopDecDisableableDepends(deviceNode);
        }
    }

    status = IopSynchronousCall(mountedDevice, &irpSp, &dummy);

    if (isMountable) {

        IopUnlockMountedDeviceForRemove(TargetDevice,
                                        IrpMinorCode,
                                        &lockContext);

        //
        // Succesful query should follow up with invalidation of all volumes
        // which have been on this device but which are not currently mounted.
        //

        if (IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE && NT_SUCCESS( status )) {

            status = IopInvalidateVolumesForDevice( TargetDevice );
        }
    }

    if (IrpMinorCode == IRP_MN_REMOVE_DEVICE) {
        ((PDEVICE_NODE)TargetDevice->DeviceObjectExtension->DeviceNode)->Flags &=
            ~(DNF_ADDED | DNF_STARTED | DNF_LEGACY_DRIVER | DNF_NEED_QUERY_IDS | DNF_NEED_ENUMERATION_ONLY);
    }

    return status;
}


PDEVICE_OBJECT
IopLockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    OUT PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    )

/*++

Routine Description:

    This routine will scan up the device stack and mark each unmounted VPB it
    finds with the VPB_REMOVE_PENDING bit (or clear it in the case of cancel)
    and increment (or decrement in the case of cancel) the reference count
    in the VPB.  This is to ensure that no new file system can get mounted on
    the device stack while the remove operation is in place.

    The search will terminate once all the attached device objects have been
    marked, or once a mounted device object has been marked.

Arguments:

    DeviceObject - the PDO we are attempting to remove

    IrpMinorCode - the remove-type operation we are going to perform

    Context - a context block which must be passed in to the unlock operation

Return Value:

    A pointer to the device object stack which the remove request should be
    sent to.  If a mounted file system was found, this will be the lowest
    file system device object in the mounted stack.  Otherwise this will be
    the PDO which was passed in.

--*/

{
    PVPB vpb;

    PDEVICE_OBJECT device = DeviceObject;
    PDEVICE_OBJECT fsDevice = NULL;

    BOOLEAN isRemovePendingSet;
    KIRQL oldIrql;

    RtlZeroMemory(Context, sizeof(LOCK_MOUNTABLE_DEVICE_CONTEXT));
    Context->MountedDevice = DeviceObject;

    do {

        //
        // Walk up each device object in the stack.  For each one, if a VPB
        // exists, grab the database resource exclusive followed by the
        // device lock.  Then acquire the Vpb spinlock and perform the
        // appropriate magic on the device object.
        //

        //
        // NOTE - Its unfortunate that the locking order includes grabbing
        // the device specific lock first followed by the global lock.
        //

        if(device->Vpb != NULL) {

            //
            // Grab the device lock.  This will ensure that there are no mount
            // or verify operations in progress.
            //

            KeWaitForSingleObject(&(device->DeviceLock),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Now set the remove pending flag, which will prevent new mounts
            // from occuring on this stack once the current (if existant)
            // filesystem dismounts. Filesystems will preserve the flag across
            // vpb swaps.
            //

            IoAcquireVpbSpinLock(&oldIrql);

            vpb = device->Vpb;

            ASSERT(vpb != NULL);

            switch(IrpMinorCode) {

                case IRP_MN_QUERY_REMOVE_DEVICE:
                case IRP_MN_SURPRISE_REMOVAL:
                case IRP_MN_REMOVE_DEVICE: {

                    vpb->Flags |= VPB_REMOVE_PENDING;
                    break;
                }

                case IRP_MN_CANCEL_REMOVE_DEVICE: {

                    vpb->Flags &= ~VPB_REMOVE_PENDING;
                    break;
                }

                default:
                    break;
            }

            //
            // Note the device object that has the filesystem stack attached.
            // We must remember the vpb we referenced that had the fs because
            // it may be swapped off of the storage device during a dismount
            // operation.
            //

            if(vpb->Flags & VPB_MOUNTED) {

                Context->MountedDevice = device;
                fsDevice = vpb->DeviceObject;
            }

            IoReleaseVpbSpinLock(oldIrql);
            KeSetEvent(&(device->DeviceLock), IO_NO_INCREMENT, FALSE);

            //
            // Stop if we hit a device with a mounted filesystem.
            //

            if (Context->MountedDevice != NULL) {

                //
                // We found and setup a mounted device.  Time to return.
                //

                break;
            }
        }

        ExAcquireFastLock( &IopDatabaseLock, &oldIrql );
        device = device->AttachedDevice;
        ExReleaseFastLock( &IopDatabaseLock, oldIrql );

    } while (device != NULL);

    if(fsDevice != NULL) {

        return fsDevice;
    }

    return Context->MountedDevice;
}

VOID
IopUnlockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    IN PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    )
{
    PVPB vpb;
    BOOLEAN removePendingSet;

    PDEVICE_OBJECT device = DeviceObject;

    do {

        KIRQL oldIrql;

        //
        // Walk up each device object in the stack.  For each one, if a VPB
        // exists, grab the database resource exclusive followed by the
        // device lock.  Then acquire the Vpb spinlock and perform the
        // appropriate magic on the device object.
        //

        //
        // NOTE - It's unfortunate that the locking order includes grabing
        // the device specific lock first followed by the global lock.
        //

        if (device->Vpb != NULL) {

            //
            // Grab the device lock.  This will ensure that there are no mount
            // or verify operations in progress, which in turn will ensure
            // that any mounted file system won't go away.
            //

            KeWaitForSingleObject(&(device->DeviceLock),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Now decrement the reference count in the VPB.  If the remove
            // pending flag has been set in the VPB (if this is a QUERY or a
            // REMOVE) then even on a dismount no new file system will be
            // allowed onto the device.
            //

            IoAcquireVpbSpinLock(&oldIrql);

            if (IrpMinorCode == IRP_MN_REMOVE_DEVICE) {

                device->Vpb->Flags &= ~VPB_REMOVE_PENDING;
            }

            IoReleaseVpbSpinLock(oldIrql);

            KeSetEvent(&(device->DeviceLock), IO_NO_INCREMENT, FALSE);
        }

        //
        // Continue up the chain until we know we hit the device the fs
        // mounted on, if any.
        //

        if (Context->MountedDevice == device) {

            break;

        } else {

            ExAcquireFastLock( &IopDatabaseLock, &oldIrql );
            device = device->AttachedDevice;
            ExReleaseFastLock( &IopDatabaseLock, oldIrql );
        }

    } while (device != NULL);

    return;
}


PDEVICE_OBJECT
IopFindMountableDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine will scan up the device stack and find a device which could
    finds with the VPB_REMOVE_PENDING bit (or clear it in the case of cancel)
    and increment (or decrement in the case of cancel) the reference count
    in the VPB.  This is to ensure that no new file system can get mounted on
    the device stack while the remove operation is in place.

    The search will terminate once all the attached device objects have been
    marked, or once a mounted device object has been marked.

Arguments:

    DeviceObject - the PDO we are attempting to remove

    IrpMinorCode - the remove-type operation we are going to perform

    Context - a context block which must be passed in to the unlock operation

Return Value:

    A pointer to the device object stack which the remove request should be
    sent to.  If a mounted file system was found, this will be the lowest
    file system device object in the mounted stack.  Otherwise this will be
    the PDO which was passed in.

--*/

{
    PVPB vpb;

    PDEVICE_OBJECT mountableDevice = DeviceObject;

    while (mountableDevice != NULL) {

        if ((mountableDevice->Flags & DO_DEVICE_HAS_NAME) &&
           (mountableDevice->Vpb != NULL)) {

            return mountableDevice;
        }

        mountableDevice = mountableDevice->AttachedDevice;
    }

    return NULL;
}


NTSTATUS
IopDeviceStartComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Completion function for an Async Start IRP.

Arguments:

    DeviceObject - NULL.
    Irp          - SetPower irp which has completed
    Context      - a pointer to the DEVICE_CHANGE_COMPLETION_CONTEXT.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED is returned to IoCompleteRequest
    to signify that IoCompleteRequest should not continue processing
    the IRP.

--*/
{
    PDEVICE_NODE deviceNode = ((PDEVICE_COMPLETION_CONTEXT)Context)->DeviceNode;
    ERESOURCE_THREAD LockingThread = ((PDEVICE_COMPLETION_CONTEXT)Context)->Thread;
    ULONG oldFlags;
    KIRQL oldIrql;

    //
    // Read state from Irp.
    //

#if DBG
    if (((PDEVICE_COMPLETION_CONTEXT)Context)->Id != (PVOID)Irp) {
        ASSERT(0);
        IoFreeIrp (Irp);
        ExFreePool(Context);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
#endif

    PnpIrpStatusTracking(Irp->IoStatus.Status, IRP_MN_START_DEVICE, deviceNode->PhysicalDeviceObject);
    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    oldFlags = deviceNode->Flags;
    deviceNode->Flags &= ~DNF_START_REQUEST_PENDING;
    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        if (deviceNode->Flags & DNF_STOPPED) {

            //
            // If the start is initiated by rebalancing, do NOT do enumeration
            //

            deviceNode->Flags &= ~DNF_STOPPED;
            deviceNode->Flags |= DNF_STARTED;

            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        } else {

            //
            // Otherwise, we need to queue a request to enumerate the device if DNF_START_REQUEST_PENDING
            // is set.  (IopStartDevice sets the flag if status of start irp returns pending.)
            //
            //

            deviceNode->Flags |= DNF_NEED_ENUMERATION_ONLY + DNF_STARTED;

            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            if (oldFlags & DNF_START_REQUEST_PENDING) {
                IopRequestDeviceAction( deviceNode->PhysicalDeviceObject,
                                        ReenumerateDeviceTree,
                                        NULL,
                                        NULL );
            }
        }

    } else {

        //
        // The start failed.  We will remove the device
        //

        deviceNode->Flags &= ~(DNF_STOPPED | DNF_STARTED);

        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        SAVE_FAILURE_INFO(deviceNode, Irp->IoStatus.Status);

        IopSetDevNodeProblem(deviceNode, CM_PROB_FAILED_START);

        if (deviceNode->DockInfo.DockStatus == DOCK_NOTDOCKDEVICE) {

            IopRequestDeviceRemoval(deviceNode->PhysicalDeviceObject, CM_PROB_FAILED_START);
        } else {

            ASSERT(deviceNode->DockInfo.DockStatus == DOCK_ARRIVING);

            //
            // ADRIAO BUGBUG 05/12/1999 -
            //     We never go down this path, but if we fix it a function
            // similar to IopRequestDeviceEjectWorker needs to be queued, one
            // that passes in "FALSE" for the kernel initiated parameter.
            //
            ASSERT(0);
            IoRequestDeviceEject(deviceNode->PhysicalDeviceObject);
        }
    }
    IopReleaseEnumerationLockForThread(NULL, LockingThread);

    //
    // Irp processing is complete, free the irp and then return
    // more_processing_required which causes IoCompleteRequest to
    // stop "completing" this irp any future.
    //

    IoFreeIrp (Irp);
    ExFreePool(Context);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IopDeviceRelationsComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Completion function for a QueryDeviceRelations IRP. T

Arguments:

    DeviceObject - NULL.
    Irp          - SetPower irp which has completed
    Context      - a pointer to the DEVICE_CHANGE_COMPLETION_CONTEXT.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED is returned to IoCompleteRequest
    to signify that IoCompleteRequest should not continue processing
    the IRP.

--*/
{
    PDEVICE_NODE deviceNode = ((PDEVICE_COMPLETION_CONTEXT)Context)->DeviceNode;
    ERESOURCE_THREAD LockingThread = ((PDEVICE_COMPLETION_CONTEXT)Context)->Thread;
    KIRQL oldIrql;
    BOOLEAN requestEnumeration = FALSE;

#if DBG
    if (((PDEVICE_COMPLETION_CONTEXT)Context)->Id != (PVOID)Irp) {
        ASSERT(0);
        IoFreeIrp (Irp);
        ExFreePool(Context);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
#endif

    PnpIrpStatusTracking(Irp->IoStatus.Status, IRP_MN_QUERY_DEVICE_RELATIONS, deviceNode->PhysicalDeviceObject);
    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
    deviceNode->Flags  &= ~DNF_BEING_ENUMERATED;

    //
    // Read state from Irp.
    //

    if (NT_SUCCESS(Irp->IoStatus.Status) && (Irp->IoStatus.Information)) {

        //
        // It is VERY important that we set PendingDeviceRealtions field before
        // checking DNF_ENUMERATION_REQUEST_PENDING.
        //

        ASSERT(deviceNode->OverUsed1.PendingDeviceRelations == NULL);
        deviceNode->OverUsed1.PendingDeviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    }

    if (deviceNode->Flags & DNF_ENUMERATION_REQUEST_PENDING) {
        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

        if (deviceNode->OverUsed1.PendingDeviceRelations) {

            //
            // If the query_device_relations irp returns something, we will
            // make a request to process the childrens.
            //

            IopRequestDeviceAction( deviceNode->PhysicalDeviceObject,
                                    RestartEnumeration,
                                    NULL,
                                    NULL );
        } else {
            deviceNode->Flags &= ~DNF_ENUMERATION_REQUEST_PENDING;
        }
    } else {
        deviceNode->Flags |= DNF_ENUMERATION_REQUEST_PENDING;
        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
    }

    IopReleaseEnumerationLockForThread(NULL, LockingThread);

    //
    // Irp processing is complete, free the irp and then return
    // more_processing_required which causes IoCompleteRequest to
    // stop "completing" this irp any future.
    //

    IoFreeIrp (Irp);
    ExFreePool(Context);

    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
    if (deviceNode->Flags & DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING) {
        deviceNode->Flags &= ~DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING;
        requestEnumeration = TRUE;
    }
    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
    if (requestEnumeration) {
        IopRequestDeviceAction( DeviceObject,
                                ReenumerateDeviceTree,
                                NULL,
                                NULL );
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IopQueryDeviceRelations(
    IN DEVICE_RELATION_TYPE Relations,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN AsyncOk,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:

    This routine sends query device relation irp to the specified device object.

Parameters:

    Relations - specifies the type of relation interested.

    DeviceObjet - Supplies the device object of the device being queried.

    AsyncOk - Specifies if we can perform Async QueryDeviceRelations

    DeviceRelations - Supplies a pointer to a variable to receive the returned
                      relation information. This must be freed by the caller.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PDEVICE_COMPLETION_CONTEXT completionContext;
    BOOLEAN requestEnumeration = FALSE;
    KIRQL oldIrql;

    //
    // Do not allow two bus relations at the same time
    //

    if (Relations == BusRelations) {
        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
        if (deviceNode->Flags & DNF_BEING_ENUMERATED) {
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            return STATUS_UNSUCCESSFUL;
        }
        deviceNode->Flags &= ~DNF_ENUMERATION_REQUEST_QUEUED;
        deviceNode->Flags  |= DNF_BEING_ENUMERATED;
        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryDeviceRelations.Type = Relations;

    //
    // Make the call and return.
    //

    if (AsyncOk && Relations == BusRelations) {
        completionContext = (PDEVICE_COMPLETION_CONTEXT) ExAllocatePool(
                              NonPagedPool,
                              sizeof(DEVICE_COMPLETION_CONTEXT));
        if (completionContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;   // BUGBUG - Should try it again.
        }

        completionContext->DeviceNode = deviceNode;
        completionContext->IrpMinorCode = IRP_MN_QUERY_DEVICE_RELATIONS;
        completionContext->Thread = ExGetCurrentResourceThread();

        //
        // Make the call and return.
        //

        IopAcquireEnumerationLock(NULL);  // To block IopAcquireTreeLock();
        status = IopAsynchronousCall(DeviceObject, &irpSp, completionContext, IopDeviceRelationsComplete);
        if (status == STATUS_PENDING) {
            KIRQL oldIrql;

            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

            //
            // Check if the completion routine completes before we setting
            // the DNF_ENUMERATION_REQUEST_PENDING flags.
            //

            if (deviceNode->Flags & DNF_ENUMERATION_REQUEST_PENDING) {
                deviceNode->Flags &= ~DNF_ENUMERATION_REQUEST_PENDING;
                *DeviceRelations = deviceNode->OverUsed1.PendingDeviceRelations;
                deviceNode->OverUsed1.PendingDeviceRelations = NULL;
                status = STATUS_SUCCESS;
            } else {

                //
                // Set DNF_ENUMERATION_REQUEST_PENDING such that the completion routine knows it
                // needs to request enumeration when the Q_bus_relations completed successfully.
                //

                deviceNode->Flags |= DNF_ENUMERATION_REQUEST_PENDING;
            }

            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        } else {
            deviceNode->Flags &= ~DNF_ENUMERATION_REQUEST_PENDING;
            *DeviceRelations = deviceNode->OverUsed1.PendingDeviceRelations;
            deviceNode->OverUsed1.PendingDeviceRelations = NULL;
        }
        return status;
    } else {
        status = IopSynchronousCall(DeviceObject, &irpSp, DeviceRelations);

        //
        // To prevent the scenario that a driver calls IoInvalidateDeviceRelations while servicing
        // an Async Q-D-R irp, and receives another q-d-r irp before first one completed, we will
        // set a flag when it calls IoInvalidateDeviceRelations and delay queuing the request till
        // the original one is completed.
        //

        if (Relations == BusRelations) {
            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
            deviceNode->Flags  &= ~DNF_BEING_ENUMERATED;
            if (deviceNode->Flags & DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING) {
                deviceNode->Flags &= ~DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING;
                requestEnumeration = TRUE;
            }
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            if (requestEnumeration) {
                IopRequestDeviceAction( DeviceObject,
                                        ReenumerateDeviceTree,
                                        NULL,
                                        NULL );
            }
        }
    }

    return status;
}

NTSTATUS
IopQueryDeviceId (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *DeviceId
    )

/*++

Routine Description:

    This routine sends a query device id irp to the specified device object.

Parameters:

    DeviceObject - Supplies the device object of the device being queried/

    DeviceId - Supplies a pointer to a variable to receive the returned Id.
               This must be freed by the caller.

Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryId.IdType = BusQueryDeviceID;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, DeviceId);

    return status;
}

NTSTATUS
IopQueryUniqueId (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *UniqueId
    )

/*++

Routine Description:


    This routine generates a unique id for the specified device object.

Parameters:

    DeviceObject - Supplies the device object of the device being queried

    UniqueId - Supplies a pointer to a variable to receive the returned Id.
               This must be freed by the caller.

    GloballyUnique - Indicates (from a previous call to query capabilities
                     whether the id is globally unique.
Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    *UniqueId = NULL;

    //
    // First ask for for InstanceId.
    //

    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryId.IdType = BusQueryInstanceID;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, UniqueId);

    if (!NT_SUCCESS(status)) {
        *UniqueId = NULL;
    }

    return status;

}

NTSTATUS
IopMakeGloballyUniqueId(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWCHAR           UniqueId,
    OUT PWCHAR         *GloballyUniqueId
    )
{
    NTSTATUS status;
    ULONG length;
    PWSTR id, Prefix = NULL;
    HANDLE enumKey;
    HANDLE instanceKey;
    UCHAR keyBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION keyValue, stringValueBuffer = NULL;
    UNICODE_STRING valueName;
    ULONG uniqueIdValue, Hash, hashInstance;
    PDEVICE_NODE parentNode;

    PAGED_CODE();

    //
    // We need to build an instance id to uniquely identify this
    // device.  We will accomplish this by producing a prefix that will be
    // prepended to the non-unique device id supplied.
    //

    //
    // To 'unique-ify' the child's instance ID, we will retrieve
    // the unique "UniqueParentID" number that has been assigned
    // to the parent and use it to construct a prefix.  This is
    // the legacy mechanism supported here so that existing device
    // settings are not lost on upgrade.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    parentNode = ((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Parent;

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ | KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {
        DbgPrint("IopQueryUniqueId:\tUnable to open HKLM\\SYSTEM\\CCS\\ENUM (status %08lx)\n",
                 status
                );
        goto clean0;
    }

    //
    // Open the instance key for this devnode
    //
    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   &parentNode->InstancePath,
                                   KEY_READ | KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {
        DbgPrint("IopQueryUniqueId:\tUnable to open registry key for %wZ (status %08lx)\n",
                 &parentNode->InstancePath,
                 status
                );
        goto clean1;
    }

    //
    // Attempt to retrieve the "UniqueParentID" value from the device
    // instance key.
    //
    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)keyBuffer;
    PiWstrToUnicodeString(&valueName, REGSTR_VALUE_UNIQUE_PARENT_ID);

    status = ZwQueryValueKey(instanceKey,
                             &valueName,
                             KeyValuePartialInformation,
                             keyValue,
                             sizeof(keyBuffer),
                             &length
                             );

    if (NT_SUCCESS(status)) {
        ASSERT(keyValue->Type == REG_DWORD);
        ASSERT(keyValue->DataLength == sizeof(ULONG));
        if ((keyValue->Type != REG_DWORD) ||
            (keyValue->DataLength != sizeof(ULONG))) {
            status = STATUS_INVALID_PARAMETER;
            goto clean2;
        }

        uniqueIdValue = *(PULONG)(keyValue->Data);

        //
        // OK, we have a unique parent ID number to prefix to the
        // instance ID.
        Prefix = (PWSTR)ExAllocatePool(PagedPool, 9 * sizeof(WCHAR));
        if (!Prefix) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto clean2;
        }
        swprintf(Prefix, L"%x", uniqueIdValue);
    } else {

        //
        // This is the current mechanism for finding existing
        // device instance prefixes and calculating new ones if
        // required.
        //

        //
        // Attempt to retrieve the "ParentIdPrefix" value from the device
        // instance key.
        //

        PiWstrToUnicodeString(&valueName, REGSTR_VALUE_PARENT_ID_PREFIX);
        length = (MAX_PARENT_PREFIX + 1) * sizeof(WCHAR) +
            FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
        stringValueBuffer = ExAllocatePool(PagedPool,
                                           length);
        if (stringValueBuffer) {
            status = ZwQueryValueKey(instanceKey,
                                     &valueName,
                                     KeyValuePartialInformation,
                                     stringValueBuffer,
                                     length,
                                     &length);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto clean2;
        }

        if (NT_SUCCESS(status)) {

            ASSERT(stringValueBuffer->Type == REG_SZ);
            if (stringValueBuffer->Type != REG_SZ) {
                status = STATUS_INVALID_PARAMETER;
                goto clean2;
            }

            //
            // Parent has already been assigned a "ParentIdPrefix".
            //

            Prefix = (PWSTR) ExAllocatePool(PagedPool,
                                            stringValueBuffer->DataLength);
            if (!Prefix)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean2;
            }
            wcscpy(Prefix, (PWSTR) stringValueBuffer->Data);
        }
        else
        {
            //
            // Parent has not been assigned a "ParentIdPrefix".
            // Compute the prefix:
            //    * Compute Hash
            //    * Look for value of the form:
            //        NextParentId.<level>.<hash>:REG_DWORD: <NextInstance>
            //      under CCS\Enum.  If not present, create it.
            //    * Assign the new "ParentIdPrefix" which will be of
            //      of the form:
            //        <level>&<hash>&<instance>
            //

            // Allocate a buffer once for the NextParentId... value
            // and for the prefix.
            length = max(wcslen(REGSTR_VALUE_NEXT_PARENT_ID) + 2 + 8 + 8,
                         MAX_PARENT_PREFIX) + 1;

            // Device instances are case in-sensitive.  Upcase before
            // performing hash to ensure that the hash is case-insensitve.
            status = RtlUpcaseUnicodeString(&valueName,
                                            &parentNode->InstancePath,
                                            TRUE);
            if (!NT_SUCCESS(status))
            {
                goto clean2;
            }
            HASH_UNICODE_STRING(&valueName, &Hash);
            RtlFreeUnicodeString(&valueName);

            Prefix = (PWSTR) ExAllocatePool(PagedPool,
                                            length * sizeof(WCHAR));
            if (!Prefix) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean2;
            }

            // Check for existence of "NextParentId...." value and update.
            swprintf(Prefix, L"%s.%x.%x", REGSTR_VALUE_NEXT_PARENT_ID,
                     Hash, parentNode->Level);
            RtlInitUnicodeString(&valueName, Prefix);
            keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)keyBuffer;
            status = ZwQueryValueKey(enumKey,
                                     &valueName,
                                     KeyValuePartialInformation,
                                     keyValue,
                                     sizeof(keyBuffer),
                                     &length
                                     );
            if (NT_SUCCESS(status) && (keyValue->Type == REG_DWORD) &&
                (keyValue->DataLength == sizeof(ULONG))) {
                hashInstance = *(PULONG)(keyValue->Data);
            }
            else {
                hashInstance = 0;
            }

            hashInstance++;

            status = ZwSetValueKey(enumKey,
                                   &valueName,
                                   TITLE_INDEX_VALUE,
                                   REG_DWORD,
                                   &hashInstance,
                                   sizeof(hashInstance)
                                   );

            if (!NT_SUCCESS(status)) {
                goto clean2;
            }

            hashInstance--;

            // Create actual ParentIdPrefix string
            PiWstrToUnicodeString(&valueName, REGSTR_VALUE_PARENT_ID_PREFIX);
            length = swprintf(Prefix, L"%x&%x&%x", parentNode->Level,
                     Hash, hashInstance) + 1;
            status = ZwSetValueKey(instanceKey,
                                   &valueName,
                                   TITLE_INDEX_VALUE,
                                   REG_SZ,
                                   Prefix,
                                   length * sizeof(WCHAR)
                                   );
            if (!NT_SUCCESS(status))
            {
                goto clean2;
            }
        }
    }

    // Construct the instance id from the non-unique id (if any)
    // provided by the child and the prefix we've constructed.
    length = wcslen(Prefix) + (UniqueId ? wcslen(UniqueId) : 0) + 2;
    id = (PWSTR)ExAllocatePool(PagedPool, length * sizeof(WCHAR));
    if (!id) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    } else if (UniqueId) {
        swprintf(id, L"%s&%s", Prefix, UniqueId);
    } else {
        wcscpy(id, Prefix);
    }

clean2:
    ZwClose(instanceKey);

clean1:
    ZwClose(enumKey);

clean0:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (stringValueBuffer) {
        ExFreePool(stringValueBuffer);
    }

    if (Prefix) {
        ExFreePool(Prefix);
    }

    *GloballyUniqueId = id;
    return status;
}

NTSTATUS
IopQueryCompatibleIds (
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE IdType,
    OUT PWCHAR *CompatibleIds,
    OUT ULONG *Length
    )

/*++

Routine Description:

    This routine sends irp to query HardwareIds or CompatibleIds.  This rotine
    queries MULTISZ Ids.

Parameters:

    DeviceObject - Supplies the device object of the device being queried/

    IdType - Specifies the Id type interested.  Only HardwareIDs and CompatibleIDs
             are supported by this routine.

    CompatibleId - Supplies a pointer to a variable to receive the returned Ids.
                   This must be freed by the caller.

    Length - Supplies a pointer to a variable to receive the length of the IDs.

Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    *Length = 0;
    if ((IdType != BusQueryHardwareIDs) && (IdType != BusQueryCompatibleIDs)) {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryId.IdType = IdType;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, CompatibleIds);

    if (NT_SUCCESS(status) && *CompatibleIds) {

        //
        // The Compatible IDs and Hardware IDs are multi_sz,
        // try to determine its size.
        //

        PWCHAR wp;

        for (wp = *CompatibleIds;
             (*wp != UNICODE_NULL) || (*(wp + 1) != UNICODE_NULL);
             wp++) {

            *Length += 2;
        }
        *Length += 4;
    }

    return status;
}

NTSTATUS
IopQueryDeviceResources (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ResourceType,
    OUT PVOID *Resource,
    OUT ULONG *Length
    )

/*++

Routine Description:

    This routine sends irp to queries resources or resource requirements list
    of the specified device object.

    If the device object is a detected device, its resources will be read from
    registry.  Otherwise, an irp is sent to the bus driver to query its resources.

Parameters:

    DeviceObject - Supplies the device object of the device being queries.

    ResourceType - 0 for device resources and 1 for resource requirements list.

    Resource - Supplies a pointer to a variable to receive the returned resources

    Length - Supplies a pointer to a variable to receive the length of the returned
             resources or resource requirements list.

Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    PDEVICE_NODE deviceNode;
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST resReqList, newResources;
    ULONG junk;
    PCM_RESOURCE_LIST cmList;
    PIO_RESOURCE_REQUIREMENTS_LIST filteredList, mergedList;
    BOOLEAN exactMatch;

    PAGED_CODE();

#if DBG

    if ((ResourceType != QUERY_RESOURCE_LIST) &&
        (ResourceType != QUERY_RESOURCE_REQUIREMENTS)) {
        return STATUS_INVALID_PARAMETER_2;
    }
#endif

    *Resource = NULL;
    *Length = 0;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;

    if (ResourceType == QUERY_RESOURCE_LIST) {

        //
        // caller is asked for RESOURCE_LIST.  If this is a madeup device, we will
        // read it from registry.  Otherwise, we ask drivers.
        //

        if (deviceNode->Flags & DNF_MADEUP) {

            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             ResourceType,
                             REGISTRY_ALLOC_CONFIG + REGISTRY_FORCED_CONFIG + REGISTRY_BOOT_CONFIG,
                             Resource,
                             Length);
            if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                status = STATUS_SUCCESS;
            }
            return status;
        } else {
            irpSp.MinorFunction = IRP_MN_QUERY_RESOURCES;
            irpSp.MajorFunction = IRP_MJ_PNP;
            status = IopSynchronousCall(DeviceObject, &irpSp, Resource);
            if (status == STATUS_NOT_SUPPORTED) {

                //
                // If driver doesn't implement this request, it
                // doesn't consume any resources.
                //

                *Resource = NULL;
                status = STATUS_SUCCESS;
            }
            if (NT_SUCCESS(status)) {
                *Length = IopDetermineResourceListSize((PCM_RESOURCE_LIST)*Resource);
            }
            return status;
        }
    } else {

        //
        // Caller is asked for resource requirements list.  We will check:
        // if there is a ForcedConfig, it will be converted to resource requirements
        //     list and return.  Otherwise,
        // If there is an OVerrideConfigVector, we will use it as our
        //     FilterConfigVector.  Otherwise we ask driver for the config vector and
        //     use it as our FilterConfigVector.
        //     Finaly, we pass the FilterConfigVector to driver stack to let drivers
        //     filter the requirements.
        //

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_LIST,
                         REGISTRY_FORCED_CONFIG,
                         Resource,
                         &junk);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             QUERY_RESOURCE_REQUIREMENTS,
                             REGISTRY_OVERRIDE_CONFIGVECTOR,
                             &resReqList,
                             &junk);
            if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                if (deviceNode->Flags & DNF_MADEUP) {
                    status = IopGetDeviceResourcesFromRegistry(
                                     DeviceObject,
                                     QUERY_RESOURCE_REQUIREMENTS,
                                     REGISTRY_BASIC_CONFIGVECTOR,
                                     &resReqList,
                                     &junk);
                    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                        status = STATUS_SUCCESS;
                        resReqList = NULL;
                    }
                } else {

                    //
                    // We are going to ask the bus driver ...
                    //

                    if (deviceNode->ResourceRequirements) {
                        ASSERT(deviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED);
                        resReqList = ExAllocatePool(PagedPool, deviceNode->ResourceRequirements->ListSize);
                        if (resReqList) {
                            RtlMoveMemory(resReqList,
                                         deviceNode->ResourceRequirements,
                                         deviceNode->ResourceRequirements->ListSize
                                         );
                            status = STATUS_SUCCESS;
                        } else {
                            return STATUS_NO_MEMORY;
                        }
                    } else {
                        irpSp.MinorFunction = IRP_MN_QUERY_RESOURCE_REQUIREMENTS;
                        irpSp.MajorFunction = IRP_MJ_PNP;
                        status = IopSynchronousCall(DeviceObject, &irpSp, &resReqList);
                        if (status == STATUS_NOT_SUPPORTED) {

                            //
                            // If driver doesn't implement this request, it
                            // doesn't require any resources.
                            //

                            status = STATUS_SUCCESS;
                            resReqList = NULL;

                        }
                    }
                }
                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }

            //
            // For devices with boot config, we need to filter the resource requirements
            // list against boot config.
            //

            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             QUERY_RESOURCE_LIST,
                             REGISTRY_BOOT_CONFIG,
                             &cmList,
                             &junk);
            if (NT_SUCCESS(status) &&
                (!cmList || cmList->Count == 0 || cmList->List[0].InterfaceType != PCIBus)) {
                status = IopFilterResourceRequirementsList (
                             resReqList,
                             cmList,
                             &filteredList,
                             &exactMatch);
                if (cmList) {
                    ExFreePool(cmList);
                }
                if (!NT_SUCCESS(status)) {
                    if (resReqList) {
                        ExFreePool(resReqList);
                    }
                    return status;
                } else {

                    //
                    // For non-root-enumerated devices, we merge filtered config with basic config
                    // vectors to form a new res req list.  For root-enumerated devices, we don't
                    // consider Basic config vector.
                    //

                    if (!(deviceNode->Flags & DNF_MADEUP) &&
                        (exactMatch == FALSE || resReqList->AlternativeLists > 1)) {
                        status = IopMergeFilteredResourceRequirementsList (
                                 filteredList,
                                 resReqList,
                                 &mergedList
                                 );
                        if (resReqList) {
                            ExFreePool(resReqList);
                        }
                        if (filteredList) {
                            ExFreePool(filteredList);
                        }
                        if (NT_SUCCESS(status)) {
                            resReqList = mergedList;
                        } else {
                            return status;
                        }
                    } else {
                        if (resReqList) {
                            ExFreePool(resReqList);
                        }
                        resReqList = filteredList;
                    }
                }
            }

        } else {
            ASSERT(NT_SUCCESS(status));

            //
            // We have Forced Config.  Convert it to resource requirements and return it.
            //

            if (*Resource) {
                resReqList = IopCmResourcesToIoResources (0, (PCM_RESOURCE_LIST)*Resource, LCPRI_FORCECONFIG);
                ExFreePool(*Resource);
                if (resReqList) {
                    *Resource = (PVOID)resReqList;
                    *Length = resReqList->ListSize;
                } else {
                    *Resource = NULL;
                    *Length = 0;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }
            } else {
                resReqList = NULL;
            }
        }

        //
        // If we are here, we have a resource requirements list for drivers to examine ...
        // NOTE: Per Lonny's request, we let drivers filter ForcedConfig
        //

        status = IopFilterResourceRequirementsCall(
            DeviceObject,
            resReqList,
            &newResources
            );

        if (NT_SUCCESS(status)) {
            UNICODE_STRING unicodeName;
            HANDLE handle, handlex;

#if DBG
            if (newResources == NULL && resReqList) {
                DbgPrint("PnpMgr: Non-NULL resource requirements list filtered to NULL\n");
            }
#endif
            if (newResources) {

                *Length = newResources->ListSize;
                ASSERT(*Length);

                //
                // Make our own copy of the allocation. We do this so that the
                // verifier doesn't believe the driver has leaked memory if
                // unloaded.
                //

                *Resource = (PVOID) ExAllocatePool(PagedPool, *Length);
                if (*Resource == NULL) {

                    ExFreePool(newResources);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(*Resource, newResources, *Length);
                ExFreePool(newResources);

            } else {
                *Length = 0;
                *Resource = NULL;
            }

            //
            // Write filtered res req to registry
            //

            status = IopDeviceObjectToDeviceInstance(DeviceObject, &handlex, KEY_ALL_ACCESS);
            if (NT_SUCCESS(status)) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &handle,
                                               handlex,
                                               &unicodeName,
                                               KEY_READ
                                               );
                if (NT_SUCCESS(status)) {
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_FILTERED_CONFIG_VECTOR);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_RESOURCE_REQUIREMENTS_LIST,
                                  *Resource,
                                  *Length
                                  );
                    ZwClose(handle);
                    ZwClose(handlex);
                }
            }
        } else {

            //
            // ADRIAO BUGBUG 05/26/1999 -
            //     Why do we not bubble up non-STATUS_NOT_SUPPORTED failure
            // codes?
            //
            *Resource = resReqList;
            if (resReqList) {
                *Length = resReqList->ListSize;
            } else {
                *Length = 0;
            }
        }
        return STATUS_SUCCESS;
    }
}

NTSTATUS
IopQueryResourceHandlerInterface(
    IN RESOURCE_HANDLER_TYPE HandlerType,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR ResourceType,
    IN OUT PVOID *Interface
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID dummy;
    PINTERFACE interface;
    USHORT size;
    GUID interfaceType;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    PAGED_CODE();

    //
    // If this device object is created by pnp mgr for legacy resource allocation,
    // skip it.
    //

    if ((deviceNode->DuplicatePDO == (PDEVICE_OBJECT) DeviceObject->DriverObject) ||
        !(DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)) {
        return STATUS_NOT_SUPPORTED;
    }

    switch (HandlerType) {
    case ResourceTranslator:
        size = sizeof(TRANSLATOR_INTERFACE) + 4;  // Pnptest
        //size = sizeof(TRANSLATOR_INTERFACE);
        interfaceType = GUID_TRANSLATOR_INTERFACE_STANDARD;
        break;

    case ResourceArbiter:
        size = sizeof(ARBITER_INTERFACE);
        interfaceType = GUID_ARBITER_INTERFACE_STANDARD;
        break;

    case ResourceLegacyDeviceDetection:
        size = sizeof(LEGACY_DEVICE_DETECTION_INTERFACE);
        interfaceType = GUID_LEGACY_DEVICE_DETECTION_STANDARD;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    interface = (PINTERFACE) ExAllocatePool(PagedPool, size);
    if (interface == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(interface, size);
    interface->Size = size;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryInterface.InterfaceType = &interfaceType;
    irpSp.Parameters.QueryInterface.Size = interface->Size;
    irpSp.Parameters.QueryInterface.Version = interface->Version = 0;
    irpSp.Parameters.QueryInterface.Interface = interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = (PVOID) ResourceType;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &dummy);
    if (NT_SUCCESS(status)) {
        *Interface = interface;
    } else {
        ExFreePool(interface);
    }
    return status;
}

NTSTATUS
IopQueryReconfiguration(
    IN UCHAR Request,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID dummy;

    PAGED_CODE();

    ASSERT (Request == IRP_MN_QUERY_STOP_DEVICE ||
            Request == IRP_MN_STOP_DEVICE ||
            Request == IRP_MN_CANCEL_STOP_DEVICE);

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = Request;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &dummy);
    return status;
}

NTSTATUS
IopQueryLegacyBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid,          OPTIONAL
    OUT INTERFACE_TYPE *InterfaceType, OPTIONAL
    OUT ULONG *BusNumber               OPTIONAL
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for its legacy bus
    information.

Parameters:

    DeviceObject - The device object to be queried.

    InterfaceGuid = Supplies a pointer to receive the device's interface type
        GUID.

    Interface = Supplies a pointer to receive the device's interface type.

    BusNumber = Supplies a pointer to receive the device's bus number.

Return Value:

    Returns NTSTATUS.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PLEGACY_BUS_INFORMATION busInfo;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_LEGACY_BUS_INFORMATION;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &busInfo);
    if (NT_SUCCESS(status)) {

        if (busInfo == NULL) {

            //
            // The device driver LIED to us.  Bad, bad, bad device driver.
            //

            PDEVICE_NODE deviceNode;

            deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode && deviceNode->ServiceName.Buffer) {

                DbgPrint("*** IopQueryLegacyBusInformation - Driver %wZ returned STATUS_SUCCESS\n", &deviceNode->ServiceName);
                DbgPrint("    for IRP_MN_QUERY_LEGACY_BUS_INFORMATION, and a NULL POINTER.\n");
            }

            ASSERT(busInfo != NULL);

        } else {
            if (ARGUMENT_PRESENT(InterfaceGuid)) {
                *InterfaceGuid = busInfo->BusTypeGuid;
            }
            if (ARGUMENT_PRESENT(InterfaceType)) {
                *InterfaceType = busInfo->LegacyBusType;
            }
            if (ARGUMENT_PRESENT(BusNumber)) {
                *BusNumber = busInfo->BusNumber;
            }
            ExFreePool(busInfo);
        }
    }
    return status;
}

NTSTATUS
IopQueryPnpBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid,         OPTIONAL
    OUT INTERFACE_TYPE *InterfaceType, OPTIONAL
    OUT ULONG *BusNumber               OPTIONAL
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PPNP_BUS_INFORMATION busInfo;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_BUS_INFORMATION;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &busInfo);
    if (NT_SUCCESS(status)) {

        if (busInfo == NULL) {

            //
            // The device driver LIED to us.  Bad, bad, bad device driver.
            //

            PDEVICE_NODE deviceNode;

            deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode && deviceNode->ServiceName.Buffer) {

                DbgPrint("*** IopQueryPnpBusInformation - Driver %wZ returned STATUS_SUCCESS\n", &deviceNode->ServiceName);
                DbgPrint("    for IRP_MN_QUERY_BUS_INFORMATION, and a NULL POINTER.\n");
            }

            ASSERT(busInfo != NULL);

        } else {
            if (ARGUMENT_PRESENT(InterfaceGuid)) {
                *InterfaceGuid = busInfo->BusTypeGuid;
            }
            if (ARGUMENT_PRESENT(InterfaceType)) {
                *InterfaceType = busInfo->LegacyBusType;
            }
            if (ARGUMENT_PRESENT(BusNumber)) {
                *BusNumber = busInfo->BusNumber;
            }
            ExFreePool(busInfo);
        }
    }
    return status;
}

NTSTATUS
IopQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine sends query device state irp to the specified device object.

Parameters:

    DeviceObjet - Supplies the device object of the device being queried.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    PDEVICE_NODE deviceNode;
    PNP_DEVICE_STATE deviceState;
    PDEVICE_RELATIONS deviceRelations;
    KEVENT userEvent;
    ULONG eventResult;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_PNP_DEVICE_STATE;

    //
    // Make the call.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, (PVOID *)&deviceState);

    //
    // Now perform the appropriate action based on the returned state
    //

    if (NT_SUCCESS(status)) {

        deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

        if (deviceState != 0) {

            //
            // everything here can only be turned on (state set)
            //

            if (deviceState & PNP_DEVICE_DONT_DISPLAY_IN_UI) {

                deviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
            }

            if (deviceState & PNP_DEVICE_NOT_DISABLEABLE) {

                if ((deviceNode->UserFlags & DNUF_NOT_DISABLEABLE)==0) {
                    //
                    // this node itself is not disableable
                    //
                    deviceNode->UserFlags |= DNUF_NOT_DISABLEABLE;
                    //
                    // propagate up tree
                    //
                    IopIncDisableableDepends(deviceNode);

                }
            }

            if (deviceState & (PNP_DEVICE_DISABLED | PNP_DEVICE_REMOVED)) {

                IopRequestDeviceRemoval( DeviceObject,
                                         (deviceState & PNP_DEVICE_DISABLED) ?
                                            CM_PROB_HARDWARE_DISABLED : CM_PROB_DEVICE_NOT_THERE
                                         );

            } else if (deviceState & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED) {

                if (deviceState & PNP_DEVICE_FAILED) {

                    IopResourceRequirementsChanged(DeviceObject, TRUE);

                } else {

                    IopResourceRequirementsChanged(DeviceObject, FALSE);

                }
            } else if (deviceState & PNP_DEVICE_FAILED) {

                deviceNode->Flags |= DNF_HAS_PRIVATE_PROBLEM;

                IopRequestDeviceRemoval(DeviceObject, 0);
            }
        } else {

            //
            // handle things that can be turned off (state cleared)
            //

            if (deviceNode->UserFlags & DNUF_NOT_DISABLEABLE) {
                //
                // this node itself is now disableable
                //
                //
                // check tree
                //
                IopDecDisableableDepends(deviceNode);
            }

            deviceNode->UserFlags &= ~(DNUF_DONT_SHOW_IN_UI | DNUF_NOT_DISABLEABLE);
        }
    }

    return status;
}


VOID
IopIncDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Increments the DisableableDepends field of this devicenode
    and potentially every parent device node up the tree
    A parent devicenode is only incremented if the child in question
    is incremented from 0 to 1

Parameters:

    DeviceNode - Supplies the device node where the depends is to be incremented

Return Value:

    none.

--*/
{

    while (DeviceNode != NULL) {

        LONG newval;

        newval = InterlockedIncrement(& DeviceNode->DisableableDepends);
        if (newval != 1) {
            //
            // we were already non-disableable, so we don't have to bother parent
            //
            break;
        }

        DeviceNode = DeviceNode ->Parent;

    }

}


VOID
IopDecDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Decrements the DisableableDepends field of this devicenode
    and potentially every parent device node up the tree
    A parent devicenode is only decremented if the child in question
    is decremented from 1 to 0

Parameters:

    DeviceNode - Supplies the device node where the depends is to be decremented

Return Value:

    none.

--*/
{

    while (DeviceNode != NULL) {

        LONG newval;

        newval = InterlockedDecrement(& DeviceNode->DisableableDepends);
        if (newval != 0) {
            //
            // we are still non-disableable, so we don't have to bother parent
            //
            break;
        }

        DeviceNode = DeviceNode ->Parent;

    }

}


NTSTATUS
IopQueryDeviceSerialNumber (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWCHAR *SerialNumber
    )

/*++

Routine Description:

    This routine retrieves a hardware serial number for the specified device object.
    If the routine fails, SerialNumber is guarenteed to be to NULL.

Parameters:

    DeviceObject - Supplies the device object of the device being queried

    SerialNumber - Supplies a pointer to a variable to receive the returned Id.
                   This must be freed by the caller.

Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    *SerialNumber = NULL;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;
    irpSp.Parameters.QueryId.IdType = BusQueryDeviceSerialNumber;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, SerialNumber);

    ASSERT(NT_SUCCESS(status) || (*SerialNumber == NULL));
    if (!NT_SUCCESS(status)) {
        *SerialNumber = NULL;
    }
    return status;
}

NTSTATUS
IopFilterResourceRequirementsCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResReqList OPTIONAL,
    OUT PVOID *Information
    )

/*++

Routine Description:

    This function sends a synchronous filter resource requirements irp to the
    top level device object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    ResReqList   - Supplies a pointer to the resource requirements requiring
                   filtering.

    Information  - Supplies a pointer to a variable that receives the returned
                   information of the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PULONG_PTR returnInfo = (PULONG_PTR)Information;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to success. This is a special hack for WDM (ie 9x)
    // compatibility. The driver verifier is in on this one.
    //

    if (ResReqList) {

        irp->IoStatus.Status = statusBlock.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = statusBlock.Information = (ULONG_PTR) ResReqList;

    } else {

        irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    }

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    irp->UserIosb = &statusBlock;
    irp->UserEvent = &event;

    //
    // Set the address of the current thread
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue this irp onto the current thread
    //

    IopQueueThreadIrp(irp);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Setup the stack location contents
    //

    irpSp->MinorFunction = IRP_MN_FILTER_RESOURCE_REQUIREMENTS;
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList = ResReqList;

    //
    // Call the driver
    //

    status = IoCallDriver(deviceObject, irp);

    PnpIrpStatusTracking(status, IRP_MN_FILTER_RESOURCE_REQUIREMENTS, deviceObject);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    *returnInfo = (ULONG_PTR) statusBlock.Information;

    return status;
}

NTSTATUS
IopQueryDockRemovalInterface(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PDOCK_INTERFACE *DockInterface
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the dock removal
    interface. We use this interface to send pseudo-remove's. We use this
    to solve the removal orderings problem.

Parameters:

    DeviceObject - Supplies a pointer to the Device object to be queried.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID dummy;
    PINTERFACE interface;
    USHORT size;
    GUID interfaceType;

    PAGED_CODE();

    size = sizeof(DOCK_INTERFACE);
    interfaceType = GUID_DOCK_INTERFACE;
    interface = (PINTERFACE) ExAllocatePool(PagedPool, size);
    if (interface == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(interface, size);
    interface->Size = size;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryInterface.InterfaceType = &interfaceType;
    irpSp.Parameters.QueryInterface.Size = interface->Size;
    irpSp.Parameters.QueryInterface.Version = interface->Version = 0;
    irpSp.Parameters.QueryInterface.Interface = interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &dummy);
    if (NT_SUCCESS(status)) {
        *DockInterface = (PDOCK_INTERFACE) interface;
    } else {
        ExFreePool(interface);
    }
    return status;
}

