/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    devnode.c

Abstract:

    This file contains routines to maintain our private device node list.

Author:

    Forrest Foltz (forrestf) 27-Mar-1996

Revision History:

    Modified for nt kernel.

--*/

#include "iop.h"

//
// Internal definitions
//

typedef struct _ENUM_CONTEXT{
    PENUM_CALLBACK CallersCallback;
    PVOID CallersContext;
} ENUM_CONTEXT, *PENUM_CONTEXT;

//
// Internal References
//

NTSTATUS
IopForAllDeviceNodesCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAllocateDeviceNode)
#pragma alloc_text(PAGE, IopForAllDeviceNodes)
#pragma alloc_text(PAGE, IopForAllChildDeviceNodes)
#pragma alloc_text(PAGE, IopForAllDeviceNodesCallback)
#pragma alloc_text(PAGE, IopDestroyDeviceNode)
#pragma alloc_text(PAGE, IopInsertTreeDeviceNode)
#pragma alloc_text(PAGE, IopRemoveTreeDeviceNode)
#endif


PDEVICE_NODE
IopAllocateDeviceNode(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This function allocates a device node from nonpaged pool and initializes
    the fields which do not require to hold lock to do so.  Since adding
    the device node to pnp mgr's device node tree requires acquiring lock,
    this routine does not add the device node to device node tree.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to its corresponding physical device
        object.

Return Value:

    a pointer to the newly created device node. Null is returned if failed.

--*/
{
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    deviceNode = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(DEVICE_NODE),
                    IOP_DNOD_TAG
                    );

    if (deviceNode == NULL ){
        return NULL;
    }

    InterlockedIncrement (&IopNumberDeviceNodes);

    RtlZeroMemory(deviceNode, sizeof(DEVICE_NODE));
    deviceNode->InterfaceType = InterfaceTypeUndefined;
    deviceNode->BusNumber = (ULONG)-1;
    deviceNode->ChildInterfaceType = InterfaceTypeUndefined;
    deviceNode->ChildBusNumber = (ULONG)-1;
    deviceNode->ChildBusTypeIndex = (USHORT)-1;

    KeInitializeEvent( &deviceNode->EnumerationMutex,
                       SynchronizationEvent,
                       TRUE );

    InitializeListHead(&deviceNode->DeviceArbiterList);
    InitializeListHead(&deviceNode->DeviceTranslatorList);

    if (PhysicalDeviceObject){

        deviceNode->PhysicalDeviceObject = PhysicalDeviceObject;
        PhysicalDeviceObject->DeviceObjectExtension->DeviceNode = (PVOID)deviceNode;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    InitializeListHead(&deviceNode->TargetDeviceNotify);

    InitializeListHead(&deviceNode->DockInfo.ListEntry);

    InitializeListHead(&deviceNode->PendedSetInterfaceState);

    return deviceNode;
}

NTSTATUS
IopForAllDeviceNodes(
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    )

/*++

Routine Description:

    This function walks the device node tree and perform caller specified
    'Callback' function for each device node.

    Note, this routine (or its worker routine) traverses the tree in a top
    down manner.

Arguments:

    Callback - Supplies the call back routine for each device node.

    Context - Supplies a parameter/context for the callback function.

Return Value:

    Status returned from Callback, if not successfull then the tree walking stops.

--*/
{
    ENUM_CONTEXT enumContext;
    NTSTATUS status;

    PAGED_CODE();

    enumContext.CallersCallback = Callback;
    enumContext.CallersContext = Context;

    //
    // Start with a pointer to the root device node, recursively examine all the
    // children until we the callback function says stop or we've looked at all
    // of them.
    //

    IopAcquireEnumerationLock(IopRootDeviceNode);

    status = IopForAllChildDeviceNodes(IopRootDeviceNode,
                                       IopForAllDeviceNodesCallback,
                                       (PVOID)&enumContext );

    IopReleaseEnumerationLock(IopRootDeviceNode);
    return status;
}

NTSTATUS
IopForAllChildDeviceNodes(
    IN PDEVICE_NODE Parent,
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    )

/*++

Routine Description:

    This function walks the Parent's device node subtree and perform caller specified
    'Callback' function for each device node under Parent.

    Note, befor calling this rotuine, callers must acquire the enumeration mutex
    of the 'Parent' device node to make sure its children won't go away unless the
    call tells them to.

Arguments:

    Parent - Supplies a pointer to the device node whose subtree is to be walked.

    Callback - Supplies the call back routine for each device node.

    Context - Supplies a parameter/context for the callback function.

Return Value:

    NTSTATUS value.

--*/

{
    PDEVICE_NODE nextChild = Parent->Child;
    PDEVICE_NODE child;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Process siblings until we find the end of the sibling list or
    // the Callback() returns FALSE.  Set result = TRUE at the top of
    // the loop so that if there are no siblings we will return TRUE,
    // e.g. Keep Enumerating.
    //
    // Note, we need to find next child before calling Callback function
    // in case the current child is deleted by the Callback function.
    //

    while (nextChild && NT_SUCCESS(status)) {
        child = nextChild;
        nextChild = child->Sibling;
        status = Callback(child, Context);
    }

    return status;
}

NTSTATUS
IopForAllDeviceNodesCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the worker routine for IopForAllChildDeviceNodes routine.

Arguments:

    DeviceNode - Supplies a pointer to the device node whose subtree is to be walked.

    Context - Supplies a context which contains the caller specified call back
              function and parameter.

Return Value:

    NTSTATUS value.

--*/

{
    PENUM_CONTEXT enumContext;
    NTSTATUS status;

    PAGED_CODE();

    enumContext = (PENUM_CONTEXT)Context;

    //
    // First call the caller's callback for this devnode
    //

    status =
        enumContext->CallersCallback(DeviceNode, enumContext->CallersContext);

    if (NT_SUCCESS(status)) {

        //
        // Now enumerate the children, if any.
        //

        IopAcquireEnumerationLock(DeviceNode);

        if( DeviceNode->Child) {

            status = IopForAllChildDeviceNodes(
                                        DeviceNode,
                                        IopForAllDeviceNodesCallback,
                                        Context);
        }
        IopReleaseEnumerationLock(DeviceNode);
    }

    return status;
}
VOID
IopDestroyDeviceNode(
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This function is invoked by IopDeleteDevice to clean up the device object's
    device node structure.

Arguments:

    DeviceNode - Supplies a pointer to the device node whose subtree is to be walked.

    Context - Supplies a context which contains the caller specified call back
              function and parameter.

Return Value:

    NTSTATUS value.

--*/

{
    PLIST_ENTRY listHead, nextEntry, entry;
    PPI_RESOURCE_TRANSLATOR_ENTRY handlerEntry;
    PINTERFACE interface;

    PAGED_CODE();

    if (DeviceNode) {

        if ((DeviceNode->PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) &&
            DeviceNode->Parent != NULL)  {

            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                          PNP_ERR_ACTIVE_PDO_FREED,
                          (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                          0,
                          0);
        }

#if DBG

        //
        // If Only Parent is NOT NULL, most likely the driver forgot to
        // release resources before deleting its FDO.  (The driver previously
        // call legacy assign resource interface.)
        //

        ASSERT(DeviceNode->Child == NULL &&
               DeviceNode->Sibling == NULL &&
               DeviceNode->LastChild == NULL
               );

        ASSERT(DeviceNode->DockInfo.SerialNumber == NULL &&
               IsListEmpty(&DeviceNode->DockInfo.ListEntry));

        if (DeviceNode->PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) {
            ASSERT (DeviceNode->Parent == 0);
        }

        if (DeviceNode->PreviousResourceList) {
            ExFreePool(DeviceNode->PreviousResourceList);
        }
        if (DeviceNode->PreviousResourceRequirements) {
            ExFreePool(DeviceNode->PreviousResourceRequirements);
        }

        //
        // device should not appear to be not-disableable if/when we get here
        // if either of these two lines ASSERT, email: jamiehun
        //

        ASSERT((DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE) == 0);
        ASSERT(DeviceNode->DisableableDepends == 0);

#endif
        //
        // If this devicenode is our internal one used for legacy
        // resource allocation, then clean up. Find this devicenode 
        // in the list of legacy resource devnodes hanging off the
        // legacy devicenode in the tree and remove it.
        //
        // BUGBUG: SantoshJ 10/15/99: We should be releasing
        // resources assigned to this device.
        //
        
        if (DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE) {

            PDEVICE_NODE    resourceDeviceNode;

            for (   resourceDeviceNode = (PDEVICE_NODE)DeviceNode->OverUsed1.LegacyDeviceNode;
                    resourceDeviceNode;
                    resourceDeviceNode = resourceDeviceNode->OverUsed2.NextResourceDeviceNode) {

                    if (resourceDeviceNode->OverUsed2.NextResourceDeviceNode == DeviceNode) {

                        resourceDeviceNode->OverUsed2.NextResourceDeviceNode = DeviceNode->OverUsed2.NextResourceDeviceNode;
                        break;

                    }
            }
        }

        if (DeviceNode->DuplicatePDO) {
            ObDereferenceObject(DeviceNode->DuplicatePDO);
        }
        if (DeviceNode->ServiceName.Length != 0) {
            ExFreePool(DeviceNode->ServiceName.Buffer);
        }
        if (DeviceNode->InstancePath.Length != 0) {
            ExFreePool(DeviceNode->InstancePath.Buffer);
        }
        if (DeviceNode->ResourceRequirements) {
            ExFreePool(DeviceNode->ResourceRequirements);
        }

        //
        // Dereference all the arbiters and translators on this PDO.
        //
        IopUncacheInterfaceInformation(DeviceNode->PhysicalDeviceObject) ;

        //
        // Release any pended IoSetDeviceInterface structures
        //

        while (!IsListEmpty(&DeviceNode->PendedSetInterfaceState)) {

            PPENDING_SET_INTERFACE_STATE entry;

            entry = (PPENDING_SET_INTERFACE_STATE)RemoveHeadList(&DeviceNode->PendedSetInterfaceState);

            ExFreePool(entry->LinkName.Buffer);

            ExFreePool(entry);
        }

        DeviceNode->PhysicalDeviceObject->DeviceObjectExtension->DeviceNode = NULL;
        ExFreePool(DeviceNode);
        IopNumberDeviceNodes--;
    }
}
//
// Code to support Power manager's need to traverse the device node tree in inverse order,
// (from the leaves towards the root.)
//

VOID
IopInsertTreeDeviceNode (
    IN PDEVICE_NODE     ParentNode,
    IN PDEVICE_NODE     DeviceNode
    )
//    N.B. Caller must own the device tree lock
{
    PDEVICE_NODE    deviceNode;
    PLIST_ENTRY     *p;
    LONG            i;

    //
    // Put this devnode at the end of the parent's list of children.
    //

    DeviceNode->Parent = ParentNode;
    if (ParentNode->LastChild) {
        ASSERT(ParentNode->LastChild->Sibling == NULL);
        ParentNode->LastChild->Sibling = DeviceNode;
        ParentNode->LastChild = DeviceNode;
    } else {
        ASSERT(ParentNode->Child == NULL);
        ParentNode->Child = ParentNode->LastChild = DeviceNode;
    }

    //
    // Determine the depth of the devnode.
    //

    for (deviceNode = DeviceNode;
         deviceNode != IopRootDeviceNode;
         deviceNode = deviceNode->Parent) {
        DeviceNode->Level++;
    }

    if (DeviceNode->Level > IopMaxDeviceNodeLevel) {
        IopMaxDeviceNodeLevel = DeviceNode->Level;
    }

    //
    // Tree has changed
    //

    IoDeviceNodeTreeSequence += 1;
}


VOID
IopRemoveTreeDeviceNode (
    IN PDEVICE_NODE     DeviceNode
    )
/*++

Routine Description:

    This function removes the device node from the device node tree

    N.B. The caller must own the device tree lock of the parent's enumeration lock

Arguments:

    DeviceNode      - Device node to remove

Return Value:


--*/
{
    PDEVICE_NODE        *Node;

    //
    // Ulink the pointer to this device node.  (If this is the
    // first entry, unlink it from the parents child pointer, else
    // remove it from the sibling list)
    //

    Node = &DeviceNode->Parent->Child;
    while (*Node != DeviceNode) {
        Node = &(*Node)->Sibling;
    }
    *Node = DeviceNode->Sibling;

    if (DeviceNode->Parent->Child == NULL) {
        DeviceNode->Parent->LastChild = NULL;
    } else {
        while (*Node) {
            Node = &(*Node)->Sibling;
        }
        DeviceNode->Parent->LastChild = CONTAINING_RECORD(Node, DEVICE_NODE, Sibling);
    }


    //
    // Orphan any outstanding device change notifications on these nodes.
    //
    IopOrphanNotification(DeviceNode);

    //
    // No longer linked
    //

    DeviceNode->Parent    = NULL;
    DeviceNode->Child     = NULL;
    DeviceNode->Sibling   = NULL;
    DeviceNode->LastChild = NULL;
}

#if DBG

VOID
IopCheckForTargetDevice (
    IN PDEVICE_OBJECT   TargetDevice,
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    if (!TargetDevice || !DeviceObject) {
        return ;
    }

    ASSERT (DeviceObject != TargetDevice);

    while (DeviceObject->AttachedDevice) {
        DeviceObject = DeviceObject->AttachedDevice;
        ASSERT (DeviceObject != TargetDevice);
    }
}

VOID
IopCheckDeviceNodeTree (
    IN PDEVICE_OBJECT TargetDevice OPTIONAL,
    IN PDEVICE_NODE   TargetNode OPTIONAL
    )
// scan the device node tree and make sure this TargetDevice and TargetNode
// are not in the tree
{
    KIRQL               OldIrql;
    PDEVICE_NODE        Node;

    return ;        // bugbug: not tested

    IopAcquireDeviceTreeLock();
    ExAcquireSpinLock (&IopDatabaseLock, &OldIrql);

    //
    // Find left most node
    //

    Node = IopRootDeviceNode;
    while (Node->Child) {
        Node = Node->Child;
    }

    //
    // Run the entire tree
    //

    while (Node != IopRootDeviceNode) {

        //
        // Verify this isn't the target node
        //

        ASSERT (Node != TargetNode);

        //
        // Verify target device isn't on the node somehow
        //

        IopCheckForTargetDevice (TargetDevice, Node->PhysicalDeviceObject);
        IopCheckForTargetDevice (TargetDevice, Node->DuplicatePDO);

        //
        // Next node
        //

        if (Node->Sibling) {
            Node = Node->Sibling;
            while (Node->Child) {
                Node = Node->Child;
            }
        } else {
            Node = Node->Parent;
        }
    }

    ExReleaseSpinLock (&IopDatabaseLock, OldIrql);
    IopReleaseDeviceTreeLock ();

}
#endif

