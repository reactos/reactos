/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pnppower.c

Abstract:

    This file contains the routines that integrate PnP and Power

Author:

    Adrian J. Oney (AdriaO) 01-19-1999

Revision History:

    Modified for nt kernel.

--*/

#include "iop.h"

//
// Internal References
//

PWCHAR
IopCaptureObjectName (
    IN PVOID    Object
    );

VOID
IopFreePoDeviceNotifyListHead (
    PLIST_ENTRY ListHead
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopWarmEjectDevice)
#pragma alloc_text(PAGELK, IoBuildPoDeviceNotifyList)
#pragma alloc_text(PAGELK, IoFreePoDeviceNotifyList)
#pragma alloc_text(PAGELK, IopFreePoDeviceNotifyListHead)
#pragma alloc_text(PAGELK, IopCaptureObjectName)
#endif

NTSTATUS
IoBuildPoDeviceNotifyList (
    IN OUT PPO_DEVICE_NOTIFY_ORDER  Order
    )
{
    PLIST_ENTRY             link;
    PPO_DEVICE_NOTIFY       notify;
    PDEVICE_NODE            node;
    PDEVICE_NODE            parent;
    ULONG                   noLists, listIndex;
    PLIST_ENTRY             notifyLists;
    LONG                    maxLevel, level;
    UCHAR                   orderLevel;
    PDEVICE_OBJECT          nonPaged;

    //
    // Acquire device node lock
    //

    IopAcquireDeviceTreeLock();
    RtlZeroMemory (Order, sizeof (*Order));
    Order->DevNodeSequence = IoDeviceNodeTreeSequence;
    InitializeListHead (&Order->Partial);
    InitializeListHead (&Order->Rebase);

    //
    // Allocate notification structures for all nodes, and determine
    // maximum depth.
    //

    level = -1;
    node = IopRootDeviceNode;
    while (node->Child) {
        node = node->Child;
        level += 1;
    }

    //
    // ADRIAO 01/12/1999 N.B. -
    // 
    // Note that we include devices without the started flag. Now, we acquire
    // the tree lock exclusively, which prevents people from being in the middle
    // of a rebalance (actually it doesn't today - this is a bug!). However, two
    // things prevent us from excluding devices that aren't started:
    // 1) We must be able to send power messages to a device we are warm 
    //    undocking.
    // 2) Many devices may not be started, that is no guarentee they are in D3!
    //    For example, they could easily have a boot config, and PNP still 
    //    relies heavily on BIOS boot configs to keep us from placing hardware
    //    ontop of other devices with boot configs we haven't found or started
    //    yet!
    //

    maxLevel = level;
    while (node != IopRootDeviceNode) {
        notify = ExAllocatePoolWithTag (
                      NonPagedPool,
                      sizeof(PO_DEVICE_NOTIFY),
                      IOP_DPWR_TAG
                      );

        if (!notify) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (notify, sizeof(PO_DEVICE_NOTIFY));
        ASSERT(node->Notify == NULL) ;
        node->Notify = notify;
        notify->Node = node;
        notify->DeviceObject = node->PhysicalDeviceObject;
        notify->TargetDevice = IoGetAttachedDevice(node->PhysicalDeviceObject);
        notify->DriverName   = IopCaptureObjectName(notify->TargetDevice->DriverObject);
        notify->DeviceName   = IopCaptureObjectName(notify->TargetDevice);
        ObReferenceObject (notify->DeviceObject);
        ObReferenceObject (notify->TargetDevice);

        orderLevel   = 0;

        if (notify->TargetDevice->DeviceType != FILE_DEVICE_SCREEN &&
            notify->TargetDevice->DeviceType != FILE_DEVICE_VIDEO) {
            orderLevel |= PO_ORDER_NOT_VIDEO;
        }

        if (notify->TargetDevice->Flags & DO_POWER_PAGABLE) {
            orderLevel |= PO_ORDER_PAGABLE;
        }

        notify->OrderLevel = orderLevel;
        notify->NodeLevel  = level;

        //
        // If this is a level 0 node it's in the root.  Look for
        // non-bus stuff in the root as those guys need to be re-based
        // below everything else
        //

        if (level == 0  &&
            node->InterfaceType != Internal &&
            !(node->Flags & DNF_HAL_NODE)) {

            InsertTailList (&Order->Rebase, &notify->Link);
        } else {
            InsertTailList (&Order->Partial, &notify->Link);
        }

        //
        // Next node
        //

        if (node->Sibling) {
            node = node->Sibling;
            while (node->Child) {
                node = node->Child;
                level += 1;
                if (level > maxLevel) {
                    maxLevel = level;
                }
            }

        } else {
            node = node->Parent;
            level -= 1;
        }
    }

    //
    // Rebase anything on the rebase list to be after the normal pnp stuff
    //

    while (!IsListEmpty(&Order->Rebase)) {
        link = Order->Rebase.Flink;
        notify = CONTAINING_RECORD (link, PO_DEVICE_NOTIFY, Link);
        RemoveEntryList (&notify->Link);
        InsertTailList (&Order->Partial, &notify->Link);

        //
        // Rebase this node
        //

        node = notify->Node;
        notify->OrderLevel |= PO_ORDER_ROOT_ENUM;

        //
        // Now rebase all the node's children
        //

        parent = node;
        while (node->Child) {
            node = node->Child;
        }

        while (node != parent) {
            notify = node->Notify;
            if (notify) {
                notify->OrderLevel |= PO_ORDER_ROOT_ENUM;
            }

            // next node
            if (node->Sibling) {
                node = node->Sibling;
                while (node->Child) {
                    node = node->Child;
                }
            } else {
                node = node->Parent;
            }
        }
    }

    //
    // Allocate ordered notifcation lists
    //

    maxLevel = maxLevel + 1;
    noLists  = maxLevel * (PO_ORDER_MAXIMUM + 1);
    notifyLists = ExAllocatePoolWithTag (
                        NonPagedPool,
                        sizeof(LIST_ENTRY) * noLists,
                        IOP_DPWR_TAG
                        );

    if (!notifyLists) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (listIndex=0; listIndex < noLists; listIndex++) {
        InitializeListHead (&notifyLists[listIndex]);
    }

    Order->Notify   = notifyLists;
    Order->MaxLevel = maxLevel;
    Order->NoLists  = noLists;

    //
    // Build ordered list
    //

    while (!IsListEmpty(&Order->Partial)) {
        link = Order->Partial.Flink;
        notify = CONTAINING_RECORD (link, PO_DEVICE_NOTIFY, Link);
        RemoveEntryList (&notify->Link);

        orderLevel = notify->OrderLevel;
        listIndex = orderLevel * maxLevel + notify->NodeLevel;
        InsertTailList (&notifyLists[listIndex], &notify->Link);

        //
        // Sanity check that the pagable bit is set the same on the top
        // of the PDO stack and the bottom of it
        //

        nonPaged = NULL;
        if (!(notify->TargetDevice->Flags & DO_POWER_PAGABLE)) {
            nonPaged = notify->TargetDevice;
        }

        if (nonPaged  &&  (notify->DeviceObject->Flags & DO_POWER_PAGABLE)) {
            KeBugCheckEx (
                DRIVER_POWER_STATE_FAILURE,
                0x100,
                (ULONG_PTR) nonPaged,
                (ULONG_PTR) notify->TargetDevice,
                (ULONG_PTR) notify->DeviceObject
                );
        }


        //
        // Make sure all parents are at least the level of this child
        // node or better
        //

        node = notify->Node;
        for (parent = node->Parent;
             parent != IopRootDeviceNode;
             parent = parent->Parent) {

            notify = parent->Notify;
            ASSERT (notify != NULL);
            if (notify->OrderLevel <= orderLevel) {
                break;
            }

            if (nonPaged  && (
                (notify->DeviceObject->Flags & DO_POWER_PAGABLE) ||
                (notify->TargetDevice->Flags & DO_POWER_PAGABLE) )) {

                KeBugCheckEx (
                    DRIVER_POWER_STATE_FAILURE,
                    0x100,
                    (ULONG_PTR) nonPaged,
                    (ULONG_PTR) notify->TargetDevice,
                    (ULONG_PTR) notify->DeviceObject
                    );
            }

            RemoveEntryList (&notify->Link);

            notify->OrderLevel = orderLevel;
            listIndex = orderLevel * maxLevel + notify->NodeLevel;
            InsertTailList (&notifyLists[listIndex], &notify->Link);
        }
    }

    Order->WarmEjectPdoPointer = &IopWarmEjectPdo;

    //
    // The device tree lock is release when the notify list is freed
    //

    return STATUS_SUCCESS;
}

VOID
IopFreePoDeviceNotifyListHead (
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY             Link;
    PPO_DEVICE_NOTIFY       Notify;
    PDEVICE_NODE            Node;

    if (ListHead->Flink) {
        while (!IsListEmpty(ListHead)) {
            Link = ListHead->Flink;
            Notify = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);
            RemoveEntryList(&Notify->Link);

            Node = (PDEVICE_NODE) Notify->Node;
            Node->Notify = NULL;

            ObDereferenceObject (Notify->DeviceObject);
            ObDereferenceObject (Notify->TargetDevice);
            if (Notify->DeviceName) {
                ExFreePool (Notify->DeviceName);
            }
            if (Notify->DriverName) {
                ExFreePool (Notify->DriverName);
            }
            ExFreePool(Notify);
        }
    }
}

VOID
IoFreePoDeviceNotifyList (
    IN OUT PPO_DEVICE_NOTIFY_ORDER  Order
    )
{
    ULONG                   i;


    if (Order->DevNodeSequence) {
        //
        // Release the device tree lock
        //

        IopReleaseDeviceTreeLock();
        Order->DevNodeSequence = 0;
    }

    //
    // Free the resources from the notify list
    //

    IopFreePoDeviceNotifyListHead (&Order->Partial);
    IopFreePoDeviceNotifyListHead (&Order->Rebase);
    if (Order->Notify) {
        for (i=0; i < Order->NoLists; i++) {
            IopFreePoDeviceNotifyListHead (&Order->Notify[i]);
        }
        ExFreePool (Order->Notify);
    }

    RtlZeroMemory (Order, sizeof (PO_DEVICE_NOTIFY_ORDER));
}


PWCHAR
IopCaptureObjectName (
    IN PVOID    Object
    )
{
    NTSTATUS                    Status;
    UCHAR                       Buffer[512];
    POBJECT_NAME_INFORMATION    ObName;
    ULONG                       len;
    PWCHAR                      Name;

    ObName = (POBJECT_NAME_INFORMATION) Buffer;
    Status = ObQueryNameString (
                Object,
                ObName,
                sizeof (Buffer),
                &len
                );

    Name = NULL;
    if (NT_SUCCESS(Status) && ObName->Name.Buffer) {
        Name = ExAllocatePoolWithTag (
                    NonPagedPool,
                    ObName->Name.Length + sizeof(WCHAR),
                    IOP_DPWR_TAG
                    );

        if (Name) {
            memcpy (Name, ObName->Name.Buffer, ObName->Name.Length);
            Name[ObName->Name.Length/sizeof(WCHAR)] = 0;
        }
    }

    return Name;
}

NTSTATUS
IopWarmEjectDevice(
   IN PDEVICE_OBJECT       DeviceToEject,
   IN SYSTEM_POWER_STATE   LightestSleepState
   )
/*++

Routine Description:

    This function is invoked to initiate a warm eject. The eject progresses
    from S1 to the passed in lightest sleep state.

Arguments:

    DeviceToEject       - The device to eject

    LightestSleepState  - The lightest S state (at least S1) that the device
                          may be ejected in. This might be S4 if we are truely
                          low on power.

Return Value:

    NTSTATUS value.

--*/
{
    NTSTATUS       status;

    PAGED_CODE();

    //
    // Acquire the warm eject device lock. A warm eject requires we enter a 
    // specific S-state, and two different devices may have conflicting options.
    // Therefore only one is allowed to occur at once.
    //
    status = KeWaitForSingleObject(
        &IopWarmEjectLock,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
  
    ASSERT(status == STATUS_SUCCESS) ;

    //
    // Acquire device node lock. We are not allowed to set or clear this field
    // unless we are under this lock.
    //
    IopAcquireDeviceTreeLock();

    //
    // Set the current Pdo to eject. 
    //
    ASSERT(IopWarmEjectPdo == NULL);
    IopWarmEjectPdo = DeviceToEject;

    //
    // Release the tree lock.
    //
    IopReleaseDeviceTreeLock();

    //
    // Attempt to invalidate Po's cached notification list. This should cause
    // IoBuildPoDeviceNotifyList to be called at which time it will in theory 
    // pickup the above placed warm eject Pdo. 
    //
    // ADRIAO NOTE 01/07/1999 -
    //     Actually, this whole IoDeviceNodeTreeSequence stuff isn't neccessary. 
    // PnP will make no changes to the tree while the device tree lock is owned,
    // and it's owned for the duration of a power notification.
    //
    IoDeviceNodeTreeSequence++;

    //
    // Sleep...
    //
    status = NtInitiatePowerAction(
        PowerActionWarmEject,
        LightestSleepState,
        POWER_ACTION_QUERY_ALLOWED |
        POWER_ACTION_UI_ALLOWED,
        FALSE // Asynchronous == FALSE
        );   

    //
    // Acquire device node lock. We are not allowed to set or clear this field
    // unless we are under this lock.
    //
    IopAcquireDeviceTreeLock();

    //
    // Clear the current PDO to eject, and see if the Pdo was actually picked
    // up.
    //
    if (IopWarmEjectPdo) {

        if (NT_SUCCESS(status)) {

            //
            // If our device wasn't picked up, the return of 
            // NtInitiatePowerAction should *not* be successful!
            //
            ASSERT(0);
            status = STATUS_UNSUCCESSFUL;
        }

        IopWarmEjectPdo = NULL;
    } 

    //
    // Release the tree lock.
    //
    IopReleaseDeviceTreeLock();
    
    //
    // Release the warm eject device lock
    //
    KeSetEvent(                                        
        &IopWarmEjectLock,
        IO_NO_INCREMENT,
        FALSE
        );        
   
    return status;
}


