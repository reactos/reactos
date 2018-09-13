/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains routines to perform device removal

Author:

    Robert B. Nelson (RobertN) Jun 1, 1998.

Revision History:

--*/

#include "iop.h"
#include "wdmguid.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'edpP')
#endif

//
// Kernel mode PNP specific routines.
//

NTSTATUS
IopCancelPendingEject(
    IN PPENDING_RELATIONS_LIST_ENTRY EjectEntry
    );

VOID
IopDelayedRemoveWorker(
    IN PVOID Context
    );

BOOLEAN
IopDeleteLockedDeviceNode(
    IN PDEVICE_NODE DeviceNode,
    IN ULONG IrpCode,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN IsKernelInitiated,
    IN ULONG Problem
    );

NTSTATUS
IopProcessRelation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN IsKernelInitiated,
    IN BOOLEAN IsDirectDescendant
    );

NTSTATUS
IopUnloadAttachedDriver(
    IN PDRIVER_OBJECT DriverObject
    );

WORK_QUEUE_ITEM IopDeviceRemovalWorkItem;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopChainDereferenceComplete)
#pragma alloc_text(PAGE, IopDelayedRemoveWorker)
#pragma alloc_text(PAGE, IopDeleteLockedDeviceNode)
#pragma alloc_text(PAGE, IopDeleteLockedDeviceNodes)
#pragma alloc_text(PAGE, IopLockDeviceRemovalRelations)
#pragma alloc_text(PAGE, IopProcessCompletedEject)
#pragma alloc_text(PAGE, IopProcessRelation)
#pragma alloc_text(PAGE, IopQueuePendingEject)
#pragma alloc_text(PAGE, IopQueuePendingSurpriseRemoval)
#pragma alloc_text(PAGE, IopRequestDeviceRemoval)
#pragma alloc_text(PAGE, IopUnloadAttachedDriver)
#pragma alloc_text(PAGE, IopUnlockDeviceRemovalRelations)
#endif

VOID
IopChainDereferenceComplete(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine is invoked when the reference count on a PDO and all its
    attached devices transitions to a zero.  It tags the devnode as ready for
    removal.  If all the devnodes are tagged then IopDelayedRemoveWorker is
    called to actually send the remove IRPs.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to the PDO whose references just
        went to zero.

Return Value:

    None.

--*/

{
    PPENDING_RELATIONS_LIST_ENTRY   entry;
    PLIST_ENTRY                     link;
    ULONG                           count;
    ULONG                           taggedCount;
    NTSTATUS                        status;

    PAGED_CODE();

    //
    // Lock the whole device node tree so no one can touch the tree.
    //

    IopAcquireDeviceTreeLock();

    //
    // Find the relation list this devnode is a member of.
    //
    for (link = IopPendingSurpriseRemovals.Flink;
         link != &IopPendingSurpriseRemovals;
         link = link->Flink) {

        entry = CONTAINING_RECORD(link, PENDING_RELATIONS_LIST_ENTRY, Link);

        //
        // Tag the devnode as ready for remove.  If it isn't in this list
        //
        status = IopSetRelationsTag( entry->RelationsList, PhysicalDeviceObject, TRUE );

        if (NT_SUCCESS(status)) {
            taggedCount = IopGetRelationsTaggedCount( entry->RelationsList );
            count = IopGetRelationsCount( entry->RelationsList );

            if (taggedCount == count) {
                //
                // Remove relations list from list of pending surprise removals.
                //
                RemoveEntryList( link );

                IopReleaseDeviceTreeLock();

                if (PsGetCurrentProcess() != PsInitialSystemProcess) {

                    //
                    // Queue a work item to do the removal so we call the driver
                    // in the system process context rather than the random one
                    // we're in now.
                    //

                    ExInitializeWorkItem( &entry->WorkItem,
                                        IopDelayedRemoveWorker,
                                        entry);

                    ExQueueWorkItem(&entry->WorkItem, DelayedWorkQueue);

                } else {

                    //
                    // We are already in the system process, so call the
                    // worker inline.
                    //

                    IopDelayedRemoveWorker( entry );

                }

                return;
            }

            break;
        }
    }

    IopReleaseDeviceTreeLock();

    ASSERT(link != &IopPendingSurpriseRemovals);
}

VOID
IopDelayedRemoveWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is usually called from a worker thread to actually send the
    remove IRPs once the reference count on a PDO and all its attached devices
    transitions to a zero.

Arguments:

    Context - Supplies a pointer to the pending relations list entry which has
        the relations list of PDOs we need to remove.

Return Value:

    None.

--*/

{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;

    PAGED_CODE();

    IopSetAllRelationsTags( entry->RelationsList, FALSE );

    IopDeleteLockedDeviceNodes( entry->DeviceObject,
                                entry->RelationsList,
                                RemoveDevice,           // OperationCode
                                TRUE,                   // IsKernelInitiated
                                FALSE,                  // ProcessIndirectDescendants
                                entry->Problem,         // Problem
                                NULL);                  // VetoingDevice

    IopFreeRelationList( entry->RelationsList );

    ExFreePool( entry );
}


BOOLEAN
IopDeleteLockedDeviceNode(
    IN PDEVICE_NODE DeviceNode,
    IN ULONG IrpCode,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN IsKernelInitiated,
    IN ULONG Problem
    )

/*++

Routine Description:

    This function assumes that the specified device is a bus and will
    recursively remove all its children.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be removed.

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    PDEVICE_OBJECT *attachedDevices, device1, *device2;
    PDRIVER_OBJECT *attachedDrivers, *driver;
    ULONG length = 0;
    BOOLEAN success = TRUE;
    NTSTATUS status;

    PAGED_CODE();

    PIDBGMSG( PIDBG_REMOVAL,
              ("IopDeleteLockedDeviceNode: Entered\n    DeviceNode = 0x%p\n    IrpCode = 0x%08X\n    RelationsList = 0x%p\n    IsKernelInitiated = %d\n   Problem = %d\n",
              DeviceNode,
              IrpCode,
              RelationsList,
              IsKernelInitiated,
              Problem));

    //
    // If this device has been deleted, simply return.
    //

    if (!IsKernelInitiated && !(DeviceNode->Flags & DNF_ADDED)) {
        PIDBGMSG(PIDBG_REMOVAL, ("IopDeleteLockedDeviceNode: Device already deleted\n"));
        if (IrpCode == IRP_MN_REMOVE_DEVICE) {
            while (deviceObject) {
                deviceObject->DeviceObjectExtension->ExtensionFlags &= ~(DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED);
                deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;
                deviceObject = deviceObject->AttachedDevice;
            }
        }
        return TRUE;
    }

    if (IrpCode == IRP_MN_SURPRISE_REMOVAL) {

        //
        // Send irp to remove the device...
        //

        PIDBGMSG( PIDBG_REMOVAL,
                  ("IopDeleteLockedDeviceNode: Sending surprise remove irp to device = 0x%p\n",
                  deviceObject));

        status = IopRemoveDevice(deviceObject, IRP_MN_SURPRISE_REMOVAL);

        if (NT_SUCCESS(status)) {

            PIDBGMSG(PIDBG_REMOVAL, ("IopDeleteLockedDeviceNode: Releasing devices resources\n"));

            IopReleaseDeviceResources(DeviceNode, FALSE);

        } else if (!(DeviceNode->Flags & DNF_NO_RESOURCE_REQUIRED)) {
            success = FALSE;
        }

        ASSERT(DeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);

    } else if (IrpCode == IRP_MN_REMOVE_DEVICE) {

        PDEVICE_NODE child, nextChild, parent;
        PDEVICE_OBJECT childPDO;

        //
        // Make sure we WILL drop our references to its children.
        //

        child = DeviceNode->Child;
        while (child) {
            if (child->Flags & DNF_ENUMERATED) {
                child->Flags &= ~DNF_ENUMERATED;
            }

            ASSERT(!(child->Flags & DNF_ADDED));

            if (child->Flags & (DNF_HAS_RESOURCE | DNF_RESOURCE_REPORTED | DNF_HAS_BOOT_CONFIG)) {

                //
                // Send irp to remove the about to be orphaned child device...
                //

                PIDBGMSG( PIDBG_REMOVAL,
                          ("IopDeleteLockedDeviceNode: Sending remove irp to orphaned child device = 0x%p\n",
                          child->PhysicalDeviceObject));

                IopRemoveDevice(child->PhysicalDeviceObject, IRP_MN_REMOVE_DEVICE);

                PIDBGMSG(PIDBG_REMOVAL,
                         ("IopDeleteLockedDeviceNode: Releasing resources for child device = 0x%p\n",
                         child->PhysicalDeviceObject));

                IopReleaseDeviceResources(child, FALSE);

                //
                // BUGBUG - what if the resources aren't released???
                //
            }

            //
            //
            nextChild = child->Sibling;
            parent = child->Parent;

            PIDBGMSG( PIDBG_REMOVAL,
                      ("IopDeleteLockedDeviceNode: Cleaning up registry values, instance = %wZ\n",
                      &child->InstancePath));

            KeEnterCriticalRegion();
            ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

            IopCleanupDeviceRegistryValues(&child->InstancePath, FALSE);

            PIDBGMSG( PIDBG_REMOVAL,
                      ("IopDeleteLockedDeviceNode: Removing DevNode tree, DevNode = 0x%p\n",
                      child));

            childPDO = child->PhysicalDeviceObject;
            IopRemoveTreeDeviceNode(child);
            IopRemoveRelationFromList(RelationsList, childPDO);
            ObDereferenceObject(childPDO); // Added during Enum

            ExReleaseResource(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();

            if (child->LockCount != 0) {

                ASSERT(child->LockCount == 1);

                child->LockCount = 0;

                KeSetEvent(&child->EnumerationMutex, 0, FALSE);
                ObDereferenceObject(childPDO);

                ASSERT(parent->LockCount > 0);

                if (--parent->LockCount == 0) {
                    KeSetEvent(&parent->EnumerationMutex, 0, FALSE);
                    ObDereferenceObject(parent->PhysicalDeviceObject);
                }
            }

            child = nextChild;
        }

        //
        // Add a reference to each FDO attached to the PDO such that the FDOs won't
        // actually go away until the removal operation is completed.
        // Note we need to make a copy of all the attached devices because we won't be
        // able to traverse the attached chain when the removal operation is done.
        //

        device1 = deviceObject->AttachedDevice;
        while (device1) {
            length++;
            device1 = device1->AttachedDevice;
        }

        attachedDevices = NULL;
        attachedDrivers = NULL;
        if (length != 0) {
            length = (length + 2) * sizeof(PDEVICE_OBJECT);
            attachedDevices = (PDEVICE_OBJECT *)ExAllocatePool (PagedPool, length);
            if (!attachedDevices) {
                return FALSE;
            }
            attachedDrivers = (PDRIVER_OBJECT *)ExAllocatePool (PagedPool, length);
            if (!attachedDrivers) {
                ExFreePool(attachedDevices);
                return FALSE;
            }
            RtlZeroMemory(attachedDevices, length);
            RtlZeroMemory(attachedDrivers, length);
            device1 = deviceObject->AttachedDevice;
            device2 = attachedDevices;
            driver = attachedDrivers;
            while (device1) {
                ObReferenceObject(device1);
                *device2++ = device1;
                *driver++ = device1->DriverObject;
                device1 = device1->AttachedDevice;
            }
        }

        //
        // Send irp to remove the device...
        //

        PIDBGMSG( PIDBG_REMOVAL,
                  ("IopDeleteLockedDeviceNode: Sending remove irp to device = 0x%p\n",
                  deviceObject));

        IopRemoveDevice(deviceObject, IRP_MN_REMOVE_DEVICE);

        PIDBGMSG(PIDBG_REMOVAL, ("IopDeleteLockedDeviceNode: Releasing devices resources\n"));

        IopReleaseDeviceResources(DeviceNode, (BOOLEAN)!(DeviceNode->Flags & DNF_DEVICE_GONE));

        if (!(DeviceNode->Flags & DNF_ENUMERATED)) {
            //
            // If the device is a dock, remove it from the list of dock devices
            // and change the current Hardware Profile, if necessary.
            //
            ASSERT(DeviceNode->DockInfo.DockStatus != DOCK_ARRIVING) ;
            if ((DeviceNode->DockInfo.DockStatus == DOCK_DEPARTING)||
                (DeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)) {
                IopHardwareProfileCommitRemovedDock(DeviceNode);
            }
        }

        //
        // Remove the reference to the attached FDOs to allow them to be actually
        // deleted.
        //

        if (device2 = attachedDevices) {
            driver = attachedDrivers;
            while (*device2) {
                (*device2)->DeviceObjectExtension->ExtensionFlags &= ~(DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED);
                (*device2)->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;
                IopUnloadAttachedDriver(*driver);
                ObDereferenceObject(*device2);
                device2++;
                driver++;
            }
            ExFreePool(attachedDevices);
            ExFreePool(attachedDrivers);
        }

        //
        // Now mark this one deleted.
        //

        if (!(DeviceNode->Flags & DNF_HAS_PRIVATE_PROBLEM)) {

            ASSERT(!IopDoesDevNodeHaveProblem(DeviceNode) ||
                    IopIsDevNodeProblem(DeviceNode, Problem) ||
                    Problem == CM_PROB_DEVICE_NOT_THERE ||
                    Problem == CM_PROB_DISABLED);

            IopClearDevNodeProblem(DeviceNode);
            IopSetDevNodeProblem(DeviceNode, Problem);
        }

        deviceObject->DeviceObjectExtension->ExtensionFlags &= ~(DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED);
        deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;

        if (DeviceNode->Flags & DNF_REMOVE_PENDING_CLOSES) {

            ASSERT(DeviceNode->LockCount == 0);

            DeviceNode->Flags &= ~DNF_REMOVE_PENDING_CLOSES;

            if ((DeviceNode->Flags & DNF_DEVICE_GONE) && DeviceNode->Parent != NULL) {

                ASSERT(DeviceNode->Child == NULL);

                PIDBGMSG( PIDBG_REMOVAL,
                          ("IopDeleteLockedDeviceNode: Cleaning up registry values, instance = %wZ\n",
                          &DeviceNode->InstancePath));

                KeEnterCriticalRegion();
                ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

                IopCleanupDeviceRegistryValues(&DeviceNode->InstancePath, FALSE);

                ExReleaseResource(&PpRegistryDeviceResource);
                KeLeaveCriticalRegion();

                PIDBGMSG( PIDBG_REMOVAL,
                          ("IopDeleteLockedDeviceNode: Removing DevNode tree, DevNode = 0x%p\n",
                          DeviceNode));

                IopRemoveTreeDeviceNode(DeviceNode);
                ObDereferenceObject(deviceObject); // Added during Enum
            }
        }

    } else {

        //
        // The request is QUERY or cancel.  If it is query return FALSE on failure.
        //

        PIDBGMSG( PIDBG_REMOVAL,
                  ("IopDeleteLockedDeviceNode: Sending QueryRemove/CancelRemove irp to device = 0x%p\n",
                  deviceObject));

        status = IopRemoveDevice(deviceObject, IrpCode);
        if (!NT_SUCCESS(status) && IrpCode == IRP_MN_QUERY_REMOVE_DEVICE) {
            PIDBGMSG( PIDBG_REMOVAL,
                      ("IopDeleteLockedDeviceNode: QueryRemove vetoed by device = 0x%p\n",
                      deviceObject));

            success = FALSE;
        }
    }
    return success;
}

NTSTATUS
IopDeleteLockedDeviceNodes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRELATION_LIST RelationsList,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    IN BOOLEAN IsKernelInitiated,
    IN BOOLEAN ProcessIndirectDescendants,
    IN ULONG Problem,
    OUT PDEVICE_OBJECT *VetoingDevice OPTIONAL
    )

/*++

Routine Description:

    This routine performs requested operation on the DeviceObject and
    the device objects specified in the DeviceRelations.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    DeviceRelations - supplies a pointer to the device's removal relations.

    OperationCode - Operation code, i.e., QueryRemove, CancelRemove, Remove...

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT deviceObject, relatedDeviceObject;
    ULONG i;
    ULONG marker;
    ULONG irpCode;
    BOOLEAN tagged, directDescendant;

    PAGED_CODE();

    PIDBGMSG( PIDBG_REMOVAL,
              ("IopDeleteLockedDeviceNodes: Entered\n    DeviceObject = 0x%p\n    RelationsList = 0x%p\n    OperationCode = %d\n",
              DeviceObject,
              RelationsList,
              OperationCode));

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;

    switch (OperationCode) {
    case QueryRemoveDevice:
        irpCode = IRP_MN_QUERY_REMOVE_DEVICE;
        break;

    case CancelRemoveDevice:
        irpCode = IRP_MN_CANCEL_REMOVE_DEVICE;
        break;

    case RemoveDevice:
        irpCode = IRP_MN_REMOVE_DEVICE;
        break;

    case SurpriseRemoveDevice:
        irpCode = IRP_MN_SURPRISE_REMOVAL;
        break;

    case EjectDevice:
    default:
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    marker = 0;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &deviceObject,
                                  &directDescendant,
                                  &tagged,
                                  TRUE)) {

        //
        // Depending on the operation we need to do different things.
        //
        //  QueryRemoveDevice / CancelRemoveDevice
        //      Ignore tagged relations - they were processed by a previous eject
        //      Process both direct and indirect descendants
        //
        //  SurpriseRemoveDevice / RemoveDevice
        //      None of the relations should be tagged
        //      Ignore indirect descendants
        //

        if (!tagged && (directDescendant || ProcessIndirectDescendants)) {
            deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

            if (OperationCode == SurpriseRemoveDevice) {
                deviceNode->Flags |= DNF_REMOVE_PENDING_CLOSES;
            }

            if (OperationCode == QueryRemoveDevice && !(deviceNode->Flags & DNF_STARTED)) {
                //
                // Don't send Queries to devices without FDOs (or filters)
                //
                continue;
            }

            if (!IopDeleteLockedDeviceNode( deviceNode,
                                            irpCode,
                                            RelationsList,
                                            IsKernelInitiated,
                                            Problem)) {

                if (OperationCode == QueryRemoveDevice) {

                    if (VetoingDevice != NULL) {
                        *VetoingDevice = deviceObject;
                    }

                    IopDeleteLockedDeviceNode( deviceNode,
                                               IRP_MN_CANCEL_REMOVE_DEVICE,
                                               RelationsList,
                                               IsKernelInitiated,
                                               Problem);

                    while (IopEnumerateRelations( RelationsList,
                                                  &marker,
                                                  &deviceObject,
                                                  NULL,
                                                  &tagged,
                                                  FALSE)) {

                        if (!tagged) {

                            deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

                            IopDeleteLockedDeviceNode( deviceNode,
                                                       IRP_MN_CANCEL_REMOVE_DEVICE,
                                                       RelationsList,
                                                       IsKernelInitiated,
                                                       Problem);
                        }
                    }

                    status = STATUS_UNSUCCESSFUL;
                    goto exit;
                }
            }
        }
    }

    //
    // As long as there is device removed, try satisfy the DNF_INSUFFICIENT_RESOURCES
    // devices.
    //

    if (OperationCode == RemoveDevice) {
        PIDBGMSG( PIDBG_REMOVAL,
                  ("IopDeleteLockedDeviceNodes: Calling IopRequestDeviceEnumeration\n"));

        IopRequestDeviceAction(NULL, AssignResources, NULL, NULL);
    }

exit:
    return status;
}

NTSTATUS
IopLockDeviceRemovalRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    OUT PRELATION_LIST *RelationsList,
    IN BOOLEAN IsKernelInitiated
    )

/*++

Routine Description:

    This routine locks the device subtrees for removal operation and returns
    a list of device objects which need to be removed with the specified
    DeviceObject.

    Caller must hold a reference to the DeviceObject.

Arguments:

    DeviceObject - Supplies a pointer to the device object to be removed.

    OperationCode - Operation code, i.e., QueryEject, CancelEject, Eject...

    DeviceRelations - supplies a pointer to a variable to receive the device's
                   removal relations.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PDEVICE_NODE            deviceNode, parent;
    PRELATION_LIST          relationsList;
    ULONG                   marker;
    BOOLEAN                 tagged;

    PAGED_CODE();

    *RelationsList = NULL;

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    if (!IsKernelInitiated && !(deviceNode->Flags & DNF_ADDED) && OperationCode != EjectDevice) {

        return STATUS_SUCCESS;
    }

    //
    // Obviously no one should try to delete the whole device node tree.
    //

    ASSERT(DeviceObject != IopRootDeviceNode->PhysicalDeviceObject);

    //
    // Lock the whole device node tree so no one can touch the tree.
    //

    IopAcquireDeviceTreeLock();

    if (IsKernelInitiated) {
        //
        // For kernel initiated removes it means that the device itself is
        // physically gone.
        //

        //
        // We'll take advantage of that signal to go and check if there
        // previously was a QueryRemove done on this devnode.  If
        // so, we need to reenumerate the parents of all the relations in
        // case some of them were speculative.
        //
    }

    if ((relationsList = IopAllocateRelationList()) == NULL) {

        IopReleaseDeviceTreeLock();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // First process the object itself
    //
    status = IopProcessRelation( DeviceObject,
                                 OperationCode,
                                 relationsList,
                                 IsKernelInitiated,
                                 TRUE);

    ASSERT(status != STATUS_INVALID_DEVICE_REQUEST);

    marker = 0;
    while (IopEnumerateRelations( relationsList,
                                  &marker,
                                  &deviceObject,
                                  NULL,
                                  &tagged,
                                  FALSE)) {

        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

        //
        // If we were successful we need to lock the parents of each of the
        // relations, otherwise we need to unlock each relation.
        //


        if (NT_SUCCESS(status)) {
            //
            // For all the device nodes in the DeviceRelations, we need to lock
            // their parents' enumeration mutex.
            //

            if (tagged) {

                //
                // If the tagged bit is set then the relation was merged from
                // a pending eject.  In this case the devnode isn't locked so
                // we do it now.
                //

                if (deviceNode->LockCount++ == 0) {
                    ObReferenceObject(deviceNode->PhysicalDeviceObject);
                    KeClearEvent(&deviceNode->EnumerationMutex);
                }
            }

            parent = deviceNode->Parent;

            if (parent->LockCount++ == 0) {
                ObReferenceObject(parent->PhysicalDeviceObject);
                KeClearEvent(&parent->EnumerationMutex);
            }

        } else {

            if (!tagged) {

                //
                // If the tagged bit is set then the relation was merged from
                // an pending eject.  In this case the devnode isn't locked so
                // we don't need to unlock it, all the others we unlock now.
                //

                ASSERT(deviceNode->LockCount > 0);

                if (--deviceNode->LockCount == 0) {
                    KeSetEvent(&deviceNode->EnumerationMutex, 0, FALSE);
                    ObDereferenceObject(deviceObject);
                }
            }
        }
    }

    //
    // Release the device tree.
    //

    IopReleaseDeviceTreeLock();

    if (NT_SUCCESS(status)) {
        IopCompressRelationList(&relationsList);
        *RelationsList = relationsList;

        //
        // At this point we have a list of all the relations, those that are
        // direct descendants of the original device we are ejecting or
        // removing have the DirectDescendant bit set.
        //
        // Relations which were merged from an existing eject have the tagged
        // bit set.
        //
        // All of the relations and their parents are locked.
        //
        // There is a reference on each device object by virtue of it being in
        // the list.  There is another one on each device object because it is
        // locked and the lock count is >= 1.
        //
        // There is also a reference on each relation's parent and it's lock
        // count is >= 1.
        //
    } else {
        IopFreeRelationList(relationsList);
    }

    return status;
}

NTSTATUS
IopProcessRelation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN IsKernelInitiated,
    IN BOOLEAN IsDirectDescendant
    )
/*++

Routine Description:

    This routine builds the list of device objects that need to be removed or
    examined when the passed in device object is torn down.

    Caller must hold the device tree lock.

Arguments:

    ADRIAO BUGBUG 01/07/1999 - fill this out.

Return Value:

    NTSTATUS code.

--*/
{
    PDEVICE_NODE                    deviceNode, relatedDeviceNode;
    PDEVICE_OBJECT                  relatedDeviceObject;
    PDEVICE_RELATIONS               deviceRelations;
    PLIST_ENTRY                     ejectLink;
    PPENDING_RELATIONS_LIST_ENTRY   ejectEntry;
    PRELATION_LIST                  pendingRelationList;
    PIRP                            pendingIrp;
    NTSTATUS                        status;
    ULONG                           i;

    PAGED_CODE();

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    if (deviceNode == NULL || deviceNode->Parent == NULL) {
        ASSERT(deviceNode != NULL);

        //
        // The device has already been removed
        //
        return STATUS_UNSUCCESSFUL;
    }

    if ((OperationCode == QueryRemoveDevice || OperationCode == EjectDevice) &&
        (deviceNode->Flags & DNF_REMOVE_PENDING_CLOSES)) {

        //
        // The device is in the process of being surprise removed, let it finish
        //
        return STATUS_UNSUCCESSFUL;
    }

    status = IopAddRelationToList( RelationsList,
                                   DeviceObject,
                                   IsDirectDescendant,
                                   FALSE);

    if (status == STATUS_SUCCESS) {

        ASSERT(deviceNode->LockCount == 0);

        if (!(deviceNode->Flags & DNF_LOCKED_FOR_EJECT)) {
            ObReferenceObject(DeviceObject);

            KeClearEvent(&deviceNode->EnumerationMutex);

            deviceNode->LockCount++;

            //
            // Then process the bus relations
            //
            for (relatedDeviceNode = deviceNode->Child;
                 relatedDeviceNode != NULL;
                 ) {

                relatedDeviceObject = relatedDeviceNode->PhysicalDeviceObject;

                relatedDeviceNode = relatedDeviceNode->Sibling;

                status = IopProcessRelation( relatedDeviceObject,
                                             OperationCode,
                                             RelationsList,
                                             IsKernelInitiated,
                                             IsDirectDescendant
                                             );

                ASSERT(status == STATUS_SUCCESS || status == STATUS_UNSUCCESSFUL);

                if (status == STATUS_INSUFFICIENT_RESOURCES ||
                    status == STATUS_INVALID_DEVICE_REQUEST) {
                    return status;
                }
            }

            //
            // Next the removal relations
            //


            if (deviceNode->Flags & DNF_STARTED) {
                status = IopQueryDeviceRelations( RemovalRelations,
                                                  DeviceObject,
                                                  FALSE,
                                                  &deviceRelations);

                if (NT_SUCCESS(status) && deviceRelations) {

                    for (i = 0; i < deviceRelations->Count; i++) {

                        relatedDeviceObject = deviceRelations->Objects[i];

                        status = IopProcessRelation( relatedDeviceObject,
                                                     OperationCode,
                                                     RelationsList,
                                                     IsKernelInitiated,
                                                     FALSE);


                        ObDereferenceObject( relatedDeviceObject );

                        ASSERT(status == STATUS_SUCCESS ||
                               status == STATUS_UNSUCCESSFUL);

                        if (status == STATUS_INSUFFICIENT_RESOURCES ||
                            status == STATUS_INVALID_DEVICE_REQUEST) {

                            ExFreePool(deviceRelations);

                            return status;
                        }
                    }

                    ExFreePool(deviceRelations);
                } else {
                    if (status != STATUS_NOT_SUPPORTED) {
                        PIDBGMSG( PIDBG_WARNING,
                                  ("IopProcessRelation: IopQueryDeviceRelations failed, DeviceObject = 0x%p, status = 0x%08X\n",
                                  DeviceObject, status));
                    }
                }
            }

            //
            // Finally the eject relations if we are doing an eject operation
            //

            if (OperationCode != QueryRemoveDevice &&
                OperationCode != RemoveFailedDevice &&
                OperationCode != RemoveUnstartedFailedDevice) {
                status = IopQueryDeviceRelations( EjectionRelations,
                                                  DeviceObject,
                                                  FALSE,
                                                  &deviceRelations);

                if (NT_SUCCESS(status) && deviceRelations) {

                    for (i = 0; i < deviceRelations->Count; i++) {

                        relatedDeviceObject = deviceRelations->Objects[i];

                        status = IopProcessRelation( relatedDeviceObject,
                                                     OperationCode,
                                                     RelationsList,
                                                     IsKernelInitiated,
                                                     FALSE);

                        ObDereferenceObject( relatedDeviceObject );

                        ASSERT(status == STATUS_SUCCESS ||
                               status == STATUS_UNSUCCESSFUL);

                        if (status == STATUS_INSUFFICIENT_RESOURCES ||
                            status == STATUS_INVALID_DEVICE_REQUEST) {

                            ExFreePool(deviceRelations);

                            return status;
                        }
                    }

                    ExFreePool(deviceRelations);
                } else {
                    if (status != STATUS_NOT_SUPPORTED) {
                        PIDBGMSG( PIDBG_WARNING,
                                  ("IopProcessRelation: IopQueryDeviceRelations failed, DeviceObject = 0x%p, status = 0x%08X\n",
                                  DeviceObject, status));
                    }
                }
            }

            status = STATUS_SUCCESS;

        } else {

            //
            // Look to see if this device is already part of a pending ejection.
            // If it is and we are doing an ejection then we will subsume it
            // within the larger ejection.  If we aren't doing an ejection then
            // we better be processing the removal of one of the ejected devices.
            //

            for (ejectLink = IopPendingEjects.Flink;
                 ejectLink != &IopPendingEjects;
                 ejectLink = ejectLink->Flink) {

                ejectEntry = CONTAINING_RECORD( ejectLink,
                                                PENDING_RELATIONS_LIST_ENTRY,
                                                Link);

                if (ejectEntry->RelationsList != NULL &&
                    IopIsRelationInList(ejectEntry->RelationsList, DeviceObject)) {


                    if (OperationCode == EjectDevice) {

                        status = IopRemoveRelationFromList(RelationsList, DeviceObject);

                        ASSERT(NT_SUCCESS(status));

                        pendingIrp = InterlockedExchangePointer(&ejectEntry->EjectIrp, NULL);
                        pendingRelationList = ejectEntry->RelationsList;
                        ejectEntry->RelationsList = NULL;

                        if (pendingIrp != NULL) {
                            IoCancelIrp(pendingIrp);
                        }

                        //
                        // ADRIAO BUGBUG 11/10/98 -
                        //     If a parent fails eject and it has a child that is
                        // infinitely pending an eject, this means the child now
                        // wakes up. One suggestion brought up that does not involve
                        // a code change is to amend the WDM spec to say if driver
                        // gets a start IRP for a device pending eject, it should
                        // cancel the eject IRP automatically.
                        //
                        IopMergeRelationLists(RelationsList, pendingRelationList, TRUE);

                        IopFreeRelationList(pendingRelationList);

                        if (IsDirectDescendant) {
                            //
                            // If IsDirectDescendant is specified then we need to
                            // get that bit set on the relation that caused us to
                            // do the merge.  IopAddRelationToList will fail with
                            // STATUS_OBJECT_NAME_COLLISION but the bit will still
                            // be set as a side effect.
                            //
                            IopAddRelationToList( RelationsList,
                                                  DeviceObject,
                                                  TRUE,
                                                  FALSE);
                        }
                    } else if (OperationCode == RemoveDevice) {

                        //
                        // We haven't processed the completion of the eject IRP
                        // before this device went away.  We'll remove it from
                        // the list in the pending ejection and return it.
                        //

                        status = IopRemoveRelationFromList( ejectEntry->RelationsList,
                                                            DeviceObject);

                        deviceNode->Flags &= ~DNF_LOCKED_FOR_EJECT;

                        ASSERT(NT_SUCCESS(status));

                        ObReferenceObject(DeviceObject);

                        KeClearEvent(&deviceNode->EnumerationMutex);

                        deviceNode->LockCount++;
                    } else {

                        ASSERT(0);

                        return STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;
                }
            }

            ASSERT(ejectLink != &IopPendingEjects);

            if (ejectLink == &IopPendingEjects) {
                KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                              PNP_ERR_DEVICE_MISSING_FROM_EJECT_LIST,
                              (ULONG_PTR)DeviceObject,
                              0,
                              0);
            }
        }
    } else if (status == STATUS_OBJECT_NAME_COLLISION) {
        PIDBGMSG( PIDBG_INFORMATION,
                  ("IopProcessRelation: Duplicate relation, DeviceObject = 0x%p\n",
                  DeviceObject));

        status = STATUS_SUCCESS;
    } else if (status != STATUS_INSUFFICIENT_RESOURCES) {
        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                      PNP_ERR_UNEXPECTED_ADD_RELATION_ERR,
                      (ULONG_PTR)DeviceObject,
                      (ULONG_PTR)RelationsList,
                      status);
    }

    return status;
}

BOOLEAN
IopQueuePendingEject(
    PPENDING_RELATIONS_LIST_ENTRY Entry
    )
{
    PAGED_CODE();

    IopAcquireDeviceTreeLock();

    InsertTailList(&IopPendingEjects, &Entry->Link);

    IopReleaseDeviceTreeLock();

    return TRUE;
}

NTSTATUS
IopInvalidateRelationsInList(
    PRELATION_LIST RelationsList,
    BOOLEAN OnlyIndirectDescendants,
    BOOLEAN UnlockDevNode,
    BOOLEAN RestartDevNode
    )

/*++

Routine Description:

    Iterate over the relations in the list creating a second list containing the
    parent of each entry skipping parents which are also in the list.  In other
    words, if the list contains node P and node C where node C is a child of node
    P then the parent of node P would be added but not node P itself.


Arguments:

    RelationsList           - List of relations

    OnlyIndirectDescendants - Indirect relations are those which aren't direct
                              descendants (bus relations) of the PDO originally
                              targetted for the operation or its direct
                              descendants.  This would include Removal or
                              Eject relations.

    UnlockDevNode           - If true then any node who's parent was invalidated
                              is unlocked.  In the case where the parent is also
                              in the list the node is still unlocked as will the
                              parent be once we process it.

    RestartDevNode          - If true then any node who's parent was invalidated
                              is restarted.  This flag requires that all the
                              relations in the list have been previously
                              sent a remove IRP.


Return Value:

    NTSTATUS code.

--*/

{
    PRELATION_LIST                  parentsList;
    PDEVICE_OBJECT                  deviceObject, parentObject;
    PDEVICE_NODE                    deviceNode, parentNode;
    ULONG                           marker;
    BOOLEAN                         directDescendant, tagged;

    PAGED_CODE();

    parentsList = IopAllocateRelationList();

    if (parentsList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IopSetAllRelationsTags( RelationsList, FALSE );

    //
    // Traverse the list creating a new list with the topmost parents of
    // each sublist contained in RelationsList.
    //

    marker = 0;

    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &deviceObject,
                                  &directDescendant,
                                  &tagged,
                                  TRUE)) {

        if (!OnlyIndirectDescendants || !directDescendant) {

            if (!tagged) {

                parentObject = deviceObject;

                while (IopSetRelationsTag( RelationsList, parentObject, TRUE ) == STATUS_SUCCESS) {

                    deviceNode = parentObject->DeviceObjectExtension->DeviceNode;

                    if (RestartDevNode)  {

                        deviceNode->Flags &= ~DNF_LOCKED_FOR_EJECT;

                        if ((deviceNode->Flags & (DNF_PROCESSED | DNF_ENUMERATED)) ==
                            (DNF_PROCESSED | DNF_ENUMERATED)) {

                            ASSERT(deviceNode->Child == NULL);
                            ASSERT(!(deviceNode->Flags & DNF_ADDED));

                            IopClearDevNodeProblem( deviceNode );
                            IopRestartDeviceNode( deviceNode );
                        }
                    }

                    if (UnlockDevNode) {
                        parentNode = deviceNode->Parent;

                        ASSERT(parentNode != NULL);

                        ASSERT(deviceNode->LockCount > 0);

                        if (--deviceNode->LockCount == 0) {
                            KeSetEvent(&deviceNode->EnumerationMutex, 0, FALSE);
                            ObDereferenceObject(deviceNode->PhysicalDeviceObject);
                        }

                        ASSERT(parentNode->LockCount > 0);

                        if (--parentNode->LockCount == 0) {
                            KeSetEvent(&parentNode->EnumerationMutex, 0, FALSE);
                            ObDereferenceObject(parentNode->PhysicalDeviceObject);
                        }
                    }

                    if (deviceNode->Parent != NULL) {

                        parentObject = deviceNode->Parent->PhysicalDeviceObject;

                    } else {
                        parentObject = NULL;
                        break;
                    }
                }

                if (parentObject != NULL)  {
                    IopAddRelationToList( parentsList, parentObject, FALSE, FALSE );
                }
            }

        }
    }

    //
    // Reenumerate each of the parents
    //

    marker = 0;

    while (IopEnumerateRelations( parentsList,
                                  &marker,
                                  &deviceObject,
                                  NULL,
                                  NULL,
                                  FALSE)) {

        IopRequestDeviceAction( deviceObject,
                                ReenumerateDeviceTree,
                                NULL,
                                NULL );
    }

    //
    // Free the parents list
    //

    IopFreeRelationList( parentsList );

    return STATUS_SUCCESS;
}

VOID
IopProcessCompletedEject(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called at passive level from a worker thread that was queued
    either when an eject IRP completed (see io\pnpirp.c - IopDeviceEjectComplete
    or io\pnpirp.c - IopEjectDevice), or when a warm eject needs to be performed.
    We also may need to fire off any enumerations of parents of ejected devices
    to verify they have indeed left.

Arguments:

    Context - Pointer to the pending relations list which contains the device
              to eject (warm) and the list of parents to reenumerate.

Return Value:

    None.

--*/
{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if ((entry->LightestSleepState != PowerSystemWorking) &&
        (entry->LightestSleepState != PowerSystemUnspecified)) {

        //
        // For docks, WinLogon gets to do the honors. For other devices, the
        // user must infer when it's safe to remove the device (if we've powered
        // up, it may not be safe now!)
        //
        entry->DisplaySafeRemovalDialog = FALSE;

        //
        // This is a warm eject request, initiate it here.
        //
        status = IopWarmEjectDevice(entry->DeviceObject, entry->LightestSleepState);

        //
        // We're back and we either succeeded or failed. Either way...
        //
    }

    if (entry->DockInterface) {

        entry->DockInterface->ProfileDepartureSetMode(
            entry->DockInterface->Context,
            PDS_UPDATE_DEFAULT
            );

        entry->DockInterface->InterfaceDereference(
            entry->DockInterface->Context
            );
    }

    IopAcquireDeviceTreeLock();

    RemoveEntryList( &entry->Link );

    //
    // Check if the RelationsList pointer in the context structure is NULL.  If
    // so, this means we were cancelled because this eject is part of a new
    // larger eject.  In that case all we want to do is unlink and free the
    // context structure.
    //

    //
    // Two interesting points about such code.
    //
    // 1) If you wait forever to complete an eject of a dock, we *wait* forever
    //    in the Query profile change state. No sneaky adding another dock. You
    //    must finish what you started...
    // 2) Let's say you are ejecting a dock, and it is taking a long time. If
    //    you try to eject the parent, that eject will *not* grab this lower
    //    eject as we will block on the profile change semaphore. Again, finish
    //    what you started...
    //

    if (entry->RelationsList != NULL)  {

        if (entry->ProfileChangingEject) {

            IopHardwareProfileSetMarkedDocksEjected();
        }

        IopInvalidateRelationsInList( entry->RelationsList, FALSE, FALSE, TRUE );

        //
        // Free the relations list
        //

        IopFreeRelationList( entry->RelationsList );

    } else {

        entry->DisplaySafeRemovalDialog = FALSE;
    }

    IopReleaseDeviceTreeLock();

    //
    // Complete the event
    //
    if (entry->DeviceEvent != NULL ) {

        PpCompleteDeviceEvent( entry->DeviceEvent, status );
    }

    if (entry->DisplaySafeRemovalDialog) {

        PpSetDeviceRemovalSafe(entry->DeviceObject, NULL, NULL);
    }

    ExFreePool( entry );
}

BOOLEAN
IopQueuePendingSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRELATION_LIST List,
    IN ULONG Problem
    )
{
    PPENDING_RELATIONS_LIST_ENTRY   entry;

    PAGED_CODE();

    entry = ExAllocatePool( NonPagedPool, sizeof(PENDING_RELATIONS_LIST_ENTRY) );

    if (entry != NULL) {

        entry->DeviceObject = DeviceObject;
        entry->RelationsList = List;
        entry->Problem = Problem;
        entry->ProfileChangingEject = FALSE ;

        IopAcquireDeviceTreeLock();

        InsertTailList(&IopPendingSurpriseRemovals, &entry->Link);

        IopReleaseDeviceTreeLock();

        return TRUE;
    }

    return FALSE;
}

NTSTATUS
IopUnlockDeviceRemovalRelations(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PRELATION_LIST       RelationsList,
    IN UNLOCK_UNLINK_ACTION UnlinkAction
    )

/*++

Routine Description:

    This routine unlocks the device tree deletion operation.
    If there is any pending kernel deletion, this routine initiates
    a worker thread to perform the work.

Arguments:

    DeviceObject - Supplies a pointer to the device object to which the remove
        was originally targetted (as opposed to one of the relations).

    DeviceRelations - supplies a pointer to the device's removal relations.

    UnlinkAction - Specifies which devnodes will be unlinked from the devnode
        tree.

        UnLinkRemovedDeviceNodes - Devnodes which are no longer enumerated and
            have been sent a REMOVE_DEVICE IRP are unlinked.

        UnlinkAllDeviceNodesPendingClose - This is used when a device is
            surprise removed.  Devnodes in RelationsList are unlinked from the
            tree if they don't have children and aren't consuming any resources.

        UnlinkOnlyChildDeviceNodesPendingClose - This is used when a device fails
            while started.  We unlink any child devnodes of the device which
            failed but not the failed device's devnode.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode, parent;
    PDEVICE_OBJECT deviceObject, relatedDeviceObject;
    ULONG i;
    ULONG marker;

    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    if (ARGUMENT_PRESENT(RelationsList)) {
        marker = 0;
        while (IopEnumerateRelations( RelationsList,
                                      &marker,
                                      &deviceObject,
                                      NULL,
                                      NULL,
                                      TRUE)) {

            deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
            parent = deviceNode->Parent;

            //
            // There are three different scenarios in which we want to unlink a
            // devnode from the tree.  The first case is tested in the first
            // part of the condition.  The other two in the second part.
            //
            // 1) A devnode is no longer enumerated and has been sent a
            //    remove IRP.
            //
            // 2) A devnode has been surprise removed, has no children, has
            //    no resources or they've been freed.  UnlinkAction will be
            //    UnlinkAllDeviceNodesPendingClose.
            //
            // 3) A devnode has failed and a surprise remove IRP has been sent.
            //    Then we want to remove children without resources but not the
            //    failed devnode itself.  UnlinkAction will be
            //    UnlinkOnlyChildDeviceNodesPendingClose.
            //

            if ((!(deviceNode->Flags & (DNF_ENUMERATED | DNF_REMOVE_PENDING_CLOSES)) &&
                    IopDoesDevNodeHaveProblem(deviceNode)) ||
                (UnlinkAction != UnlinkRemovedDeviceNodes &&
                    (deviceNode->Flags & DNF_REMOVE_PENDING_CLOSES) &&
                    !(deviceNode->Flags & DNF_RESOURCE_ASSIGNED) &&
                    deviceNode->Child == NULL &&
                    (UnlinkAction == UnlinkAllDeviceNodesPendingClose ||
                     deviceObject != DeviceObject))) {

                PIDBGMSG( PIDBG_REMOVAL,
                          ("IopUnlockDeviceRemovalRelations: Cleaning up registry values, instance = %wZ\n",
                          &deviceNode->InstancePath));

                IopCleanupDeviceRegistryValues(&deviceNode->InstancePath, FALSE);

                PIDBGMSG( PIDBG_REMOVAL,
                          ("IopUnlockDeviceRemovalRelations: Removing DevNode tree, DevNode = 0x%p\n",
                          deviceNode));

                IopRemoveTreeDeviceNode(deviceNode);

                if (!(deviceNode->Flags & DNF_REMOVE_PENDING_CLOSES)) {
                    IopRemoveRelationFromList(RelationsList, deviceObject);
                }
                ObDereferenceObject(deviceObject); // Added during enum
            }

            ASSERT(deviceNode->LockCount > 0);

            if (--deviceNode->LockCount == 0) {
                KeSetEvent(&deviceNode->EnumerationMutex, 0, FALSE);
                ObDereferenceObject(deviceObject);
            }

            ASSERT(parent->LockCount > 0);

            if (--parent->LockCount == 0) {
                KeSetEvent(&parent->EnumerationMutex, 0, FALSE);
                ObDereferenceObject(parent->PhysicalDeviceObject);
            }
        }
    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

//
// The routines below are specific to kernel mode PnP configMgr.
//
NTSTATUS
IopRequestDeviceRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Problem
    )

/*++

Routine Description:

    This routine queues a work item to delete a device.  (This is because
    to delete a device we need to lock device tree.  Most of the places where
    we want to delete a device have DeviceNode Enumeration lock acquired.)
    This is for IO internal use only.

Arguments:

    DeviceObject - Supplies a pointer to the device object to be eject.

Return Value:

    NTSTATUS code.

--*/

{
    PAGED_CODE();

    ASSERT(DeviceObject->DeviceObjectExtension->DeviceNode != NULL);

    ASSERT(((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->InstancePath.Length != 0);

    //
    // Queue the event, we'll return immediately after it's queued.
    //

    return PpSetTargetDeviceRemove( DeviceObject,
                                    TRUE,
                                    TRUE,
                                    FALSE,
                                    Problem,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
}

NTSTATUS
IopUnloadAttachedDriver(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This function unloads the driver for the specified device object if it does not
    control any other device object.

Arguments:

    DeviceObject - Supplies a pointer to a device object

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PWCHAR buffer;
    UNICODE_STRING unicodeName;
    PUNICODE_STRING serviceName = &DriverObject->DriverExtension->ServiceKeyName;

    PAGED_CODE();

    if (DriverObject->DriverSection != NULL) {
        if (DriverObject->DeviceObject == NULL) {
            buffer = (PWCHAR) ExAllocatePool(
                                 PagedPool,
                                 CmRegistryMachineSystemCurrentControlSetServices.Length +
                                     serviceName->Length + sizeof(WCHAR) +
                                     sizeof(L"\\"));
            if (!buffer) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            swprintf(buffer,
                     L"%s\\%s",
                     CmRegistryMachineSystemCurrentControlSetServices.Buffer,
                     serviceName->Buffer);
            RtlInitUnicodeString(&unicodeName, buffer);
            status = ZwUnloadDriver(&unicodeName);
#if DBG
            if (NT_SUCCESS(status)) {
                DbgPrint("****** Unloaded driver (%wZ)\n", serviceName);
            } else {
                DbgPrint("****** Error unloading driver (%wZ), status = 0x%08X\n", serviceName, status);
            }
#endif
            ExFreePool(unicodeName.Buffer);
        }
#if DBG
        else {
            DbgPrint("****** Skipping unload of driver (%wZ), DriverObject->DeviceObject != NULL\n", serviceName);
        }
#endif
    }
#if DBG
    else {
        //
        // This is a boot driver, can't be unloaded just return SUCCESS
        //
        DbgPrint("****** Skipping unload of boot driver (%wZ)\n", serviceName);
    }
#endif
    return STATUS_SUCCESS;
}
