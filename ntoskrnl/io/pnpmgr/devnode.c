/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP manager device tree functions
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              2007 Hervé Poussineau (hpoussin@reactos.org)
 *              2010-2012 Cameron Gutman (cameron.gutman@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopRootDeviceNode;
KSPIN_LOCK IopDeviceTreeLock;

LONG IopNumberDeviceNodes;

/* FUNCTIONS *****************************************************************/

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    return ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;
}

PDEVICE_NODE
PipAllocateDeviceNode(
    _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode;
    PAGED_CODE();

    /* Allocate it */
    DeviceNode = ExAllocatePoolZero(NonPagedPool, sizeof(DEVICE_NODE), TAG_IO_DEVNODE);
    if (!DeviceNode)
    {
        return NULL;
    }

    /* Statistics */
    InterlockedIncrement(&IopNumberDeviceNodes);

    /* Set it up */
    DeviceNode->InterfaceType = InterfaceTypeUndefined;
    DeviceNode->BusNumber = -1;
    DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
    DeviceNode->ChildBusNumber = -1;
    DeviceNode->ChildBusTypeIndex = -1;
    DeviceNode->State = DeviceNodeUninitialized;
//    KeInitializeEvent(&DeviceNode->EnumerationMutex, SynchronizationEvent, TRUE);
    InitializeListHead(&DeviceNode->DeviceArbiterList);
    InitializeListHead(&DeviceNode->DeviceTranslatorList);
    InitializeListHead(&DeviceNode->TargetDeviceNotify);
    InitializeListHead(&DeviceNode->DockInfo.ListEntry);
    InitializeListHead(&DeviceNode->PendedSetInterfaceState);

    /* Check if there is a PDO */
    if (PhysicalDeviceObject)
    {
        /* Link it and remove the init flag */
        DeviceNode->PhysicalDeviceObject = PhysicalDeviceObject;
        ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = DeviceNode;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    DPRINT("Allocated devnode 0x%p\n", DeviceNode);

    /* Return the node */
    return DeviceNode;
}

VOID
PiInsertDevNode(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PDEVICE_NODE ParentNode)
{
    KIRQL oldIrql;

    ASSERT(DeviceNode->Parent == NULL);

    KeAcquireSpinLock(&IopDeviceTreeLock, &oldIrql);
    DeviceNode->Parent = ParentNode;
    DeviceNode->Sibling = NULL;
    if (ParentNode->LastChild == NULL)
    {
        ParentNode->Child = DeviceNode;
        ParentNode->LastChild = DeviceNode;
    }
    else
    {
        ParentNode->LastChild->Sibling = DeviceNode;
        ParentNode->LastChild = DeviceNode;
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, oldIrql);
    DeviceNode->Level = ParentNode->Level + 1;

    DPRINT("Inserted devnode 0x%p to parent 0x%p\n", DeviceNode, ParentNode);
}

PNP_DEVNODE_STATE
PiSetDevNodeState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PNP_DEVNODE_STATE NewState)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &oldIrql);

    PNP_DEVNODE_STATE prevState = DeviceNode->State;
    if (prevState != NewState)
    {
        DeviceNode->State = NewState;
        DeviceNode->PreviousState = prevState;
        DeviceNode->StateHistory[DeviceNode->StateHistoryEntry++] = prevState;
        DeviceNode->StateHistoryEntry %= DEVNODE_HISTORY_SIZE;
    }

    KeReleaseSpinLock(&IopDeviceTreeLock, oldIrql);

    DPRINT("%wZ Changed state 0x%x => 0x%x\n", &DeviceNode->InstancePath, prevState, NewState);
    return prevState;
}

VOID
PiSetDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UINT32 Problem)
{
    DeviceNode->Flags |= DNF_HAS_PROBLEM;
    DeviceNode->Problem = Problem;
}

VOID
PiClearDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode)
{
    DeviceNode->Flags &= ~DNF_HAS_PROBLEM;
    DeviceNode->Problem = 0;
}

/**
 * @brief      Creates a device node
 *
 * @param[in]  ParentNode           Pointer to parent device node
 * @param[in]  PhysicalDeviceObject Pointer to PDO for device object. Pass NULL to have
 *                                  the root device node create one (eg. for legacy drivers)
 * @param[in]  ServiceName          The service (driver) name for a node. Pass NULL
 *                                  to set UNKNOWN as a service
 * @param[out] DeviceNode           Pointer to storage for created device node
 *
 * @return     Status, indicating the result of an operation
 */
#if 0
NTSTATUS
IopCreateDeviceNode(
    _In_ PDEVICE_NODE ParentNode,
    _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_opt_ PUNICODE_STRING ServiceName,
    _Out_ PDEVICE_NODE *DeviceNode)
{
    PDEVICE_NODE Node;
    NTSTATUS Status;
    UNICODE_STRING FullServiceName;
    UNICODE_STRING LegacyPrefix = RTL_CONSTANT_STRING(L"LEGACY_");
    UNICODE_STRING UnknownDeviceName = RTL_CONSTANT_STRING(L"UNKNOWN");
    UNICODE_STRING KeyName, ClassName;
    PUNICODE_STRING ServiceName1;
    ULONG LegacyValue;
    UNICODE_STRING ClassGUID;
    HANDLE InstanceHandle;

    DPRINT("ParentNode 0x%p PhysicalDeviceObject 0x%p ServiceName %wZ\n",
           ParentNode, PhysicalDeviceObject, ServiceName);

    Node = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_NODE), TAG_IO_DEVNODE);
    if (!Node)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Node, sizeof(DEVICE_NODE));
    InitializeListHead(&Node->TargetDeviceNotify);

    if (!ServiceName)
        ServiceName1 = &UnknownDeviceName;
    else
        ServiceName1 = ServiceName;

    if (!PhysicalDeviceObject)
    {
        FullServiceName.MaximumLength = LegacyPrefix.Length + ServiceName1->Length + sizeof(UNICODE_NULL);
        FullServiceName.Length = 0;
        FullServiceName.Buffer = ExAllocatePool(PagedPool, FullServiceName.MaximumLength);
        if (!FullServiceName.Buffer)
        {
            ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlAppendUnicodeStringToString(&FullServiceName, &LegacyPrefix);
        RtlAppendUnicodeStringToString(&FullServiceName, ServiceName1);
        RtlUpcaseUnicodeString(&FullServiceName, &FullServiceName, FALSE);

        Status = PnpRootCreateDevice(&FullServiceName, NULL, &PhysicalDeviceObject, &Node->InstancePath);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PnpRootCreateDevice() failed with status 0x%08X\n", Status);
            ExFreePool(FullServiceName.Buffer);
            ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
            return Status;
        }

        /* Create the device key for legacy drivers */
        Status = IopCreateDeviceKeyPath(&Node->InstancePath, REG_OPTION_VOLATILE, &InstanceHandle);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(FullServiceName.Buffer);
            ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
            return Status;
        }

        Node->ServiceName.MaximumLength = ServiceName1->Length + sizeof(UNICODE_NULL);
        Node->ServiceName.Length = 0;
        Node->ServiceName.Buffer = ExAllocatePool(PagedPool, Node->ServiceName.MaximumLength);
        if (!Node->ServiceName.Buffer)
        {
            ZwClose(InstanceHandle);
            ExFreePool(FullServiceName.Buffer);
            ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
            return Status;
        }

        RtlCopyUnicodeString(&Node->ServiceName, ServiceName1);

        if (ServiceName)
        {
            RtlInitUnicodeString(&KeyName, L"Service");
            Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ServiceName->Buffer, ServiceName->Length + sizeof(UNICODE_NULL));
        }

        if (NT_SUCCESS(Status))
        {
            RtlInitUnicodeString(&KeyName, L"Legacy");
            LegacyValue = 1;
            Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_DWORD, &LegacyValue, sizeof(LegacyValue));

            RtlInitUnicodeString(&KeyName, L"ConfigFlags");
            LegacyValue = 0;
            ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_DWORD, &LegacyValue, sizeof(LegacyValue));

            if (NT_SUCCESS(Status))
            {
                RtlInitUnicodeString(&KeyName, L"Class");
                RtlInitUnicodeString(&ClassName, L"LegacyDriver");
                Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ClassName.Buffer, ClassName.Length + sizeof(UNICODE_NULL));
                if (NT_SUCCESS(Status))
                {
                    RtlInitUnicodeString(&KeyName, L"ClassGUID");
                    RtlInitUnicodeString(&ClassGUID, L"{8ECC055D-047F-11D1-A537-0000F8753ED1}");
                    Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ClassGUID.Buffer, ClassGUID.Length + sizeof(UNICODE_NULL));
                    if (NT_SUCCESS(Status))
                    {
                        // FIXME: Retrieve the real "description" by looking at the "DisplayName" string
                        // of the corresponding CurrentControlSet\Services\xxx entry for this driver.
                        RtlInitUnicodeString(&KeyName, L"DeviceDesc");
                        Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ServiceName1->Buffer, ServiceName1->Length + sizeof(UNICODE_NULL));
                    }
                }
            }
        }

        ZwClose(InstanceHandle);
        ExFreePool(FullServiceName.Buffer);

        if (!NT_SUCCESS(Status))
        {
            ExFreePool(Node->ServiceName.Buffer);
            ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
            return Status;
        }

        IopDeviceNodeSetFlag(Node, DNF_LEGACY_DRIVER);
        IopDeviceNodeSetFlag(Node, DNF_PROCESSED);
        IopDeviceNodeSetFlag(Node, DNF_ADDED);
        IopDeviceNodeSetFlag(Node, DNF_STARTED);
    }

    Node->PhysicalDeviceObject = PhysicalDeviceObject;

    ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = Node;

    if (ParentNode)
    {
        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
        Node->Parent = ParentNode;
        Node->Sibling = NULL;
        if (ParentNode->LastChild == NULL)
        {
            ParentNode->Child = Node;
            ParentNode->LastChild = Node;
        }
        else
        {
            ParentNode->LastChild->Sibling = Node;
            ParentNode->LastChild = Node;
        }
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
        Node->Level = ParentNode->Level + 1;
    }

    PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    *DeviceNode = Node;

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
IopFreeDeviceNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    KIRQL OldIrql;
    PDEVICE_NODE PrevSibling = NULL;

    ASSERT(DeviceNode->PhysicalDeviceObject);
    /* All children must be deleted before a parent is deleted */
    ASSERT(DeviceNode->Child == NULL);
    /* This is the only state where we are allowed to remove the node */
    ASSERT(DeviceNode->State == DeviceNodeRemoved);
    /* No notifications should be registered for this device */
    ASSERT(IsListEmpty(&DeviceNode->TargetDeviceNotify));

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

    /* Get previous sibling */
    if (DeviceNode->Parent && DeviceNode->Parent->Child != DeviceNode)
    {
        PrevSibling = DeviceNode->Parent->Child;
        while (PrevSibling->Sibling != DeviceNode)
            PrevSibling = PrevSibling->Sibling;
    }

    /* Unlink from parent if it exists */
    if (DeviceNode->Parent)
    {
        if (DeviceNode->Parent->LastChild == DeviceNode)
        {
            DeviceNode->Parent->LastChild = PrevSibling;
            if (PrevSibling)
                PrevSibling->Sibling = NULL;
        }
        if (DeviceNode->Parent->Child == DeviceNode)
            DeviceNode->Parent->Child = DeviceNode->Sibling;
    }

    /* Unlink from sibling list */
    if (PrevSibling)
        PrevSibling->Sibling = DeviceNode->Sibling;

    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    RtlFreeUnicodeString(&DeviceNode->InstancePath);

    RtlFreeUnicodeString(&DeviceNode->ServiceName);

    if (DeviceNode->ResourceList)
    {
        ExFreePool(DeviceNode->ResourceList);
    }

    if (DeviceNode->ResourceListTranslated)
    {
        ExFreePool(DeviceNode->ResourceListTranslated);
    }

    if (DeviceNode->ResourceRequirements)
    {
        ExFreePool(DeviceNode->ResourceRequirements);
    }

    if (DeviceNode->BootResources)
    {
        ExFreePool(DeviceNode->BootResources);
    }

    ((PEXTENDED_DEVOBJ_EXTENSION)DeviceNode->PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = NULL;
    ExFreePoolWithTag(DeviceNode, TAG_IO_DEVNODE);

    return STATUS_SUCCESS;
}

static
NTSTATUS
IopFindNextDeviceNodeForTraversal(
    _In_ PDEVICETREE_TRAVERSE_CONTEXT Context)
{
    /* If we have a child, simply go down the tree */
    if (Context->DeviceNode->Child != NULL)
    {
        ASSERT(Context->DeviceNode->Child->Parent == Context->DeviceNode);
        Context->DeviceNode = Context->DeviceNode->Child;
        return STATUS_SUCCESS;
    }

    while (Context->DeviceNode != Context->FirstDeviceNode)
    {
        /* All children processed -- go sideways */
        if (Context->DeviceNode->Sibling != NULL)
        {
            ASSERT(Context->DeviceNode->Sibling->Parent == Context->DeviceNode->Parent);
            Context->DeviceNode = Context->DeviceNode->Sibling;
            return STATUS_SUCCESS;
        }

        /* We're the last sibling -- go back up */
        ASSERT(Context->DeviceNode->Parent->LastChild == Context->DeviceNode);
        Context->DeviceNode = Context->DeviceNode->Parent;

        /* We already visited the parent and all its children, so keep looking */
    }

    /* Done with all children of the start node -- stop enumeration */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IopTraverseDeviceTree(
    _In_ PDEVICETREE_TRAVERSE_CONTEXT Context)
{
    NTSTATUS Status;
    PDEVICE_NODE DeviceNode;

    DPRINT("Context 0x%p\n", Context);

    DPRINT("IopTraverseDeviceTree(DeviceNode 0x%p  FirstDeviceNode 0x%p  Action %p  Context 0x%p)\n",
           Context->DeviceNode, Context->FirstDeviceNode, Context->Action, Context->Context);

    /* Start from the specified device node */
    Context->DeviceNode = Context->FirstDeviceNode;

    /* Traverse the device tree */
    do
    {
        DeviceNode = Context->DeviceNode;

        /* HACK: Keep a reference to the PDO so we can keep traversing the tree
         * if the device is deleted. In a perfect world, children would have to be
         * deleted before their parents, and we'd restart the traversal after
         * deleting a device node. */
        ObReferenceObject(DeviceNode->PhysicalDeviceObject);

        /* Call the action routine */
        Status = (Context->Action)(DeviceNode, Context->Context);
        if (NT_SUCCESS(Status))
        {
            /* Find next device node */
            ASSERT(Context->DeviceNode == DeviceNode);
            Status = IopFindNextDeviceNodeForTraversal(Context);
        }

        /* We need to either abort or make progress */
        ASSERT(!NT_SUCCESS(Status) || Context->DeviceNode != DeviceNode);

        ObDereferenceObject(DeviceNode->PhysicalDeviceObject);
    } while (NT_SUCCESS(Status));

    if (Status == STATUS_UNSUCCESSFUL)
    {
        /* The action routine just wanted to terminate the traversal with status
        code STATUS_SUCCESS */
        Status = STATUS_SUCCESS;
    }

    return Status;
}
