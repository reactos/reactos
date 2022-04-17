/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/plugplay.c
 * PURPOSE:         Plug-and-play interface routines
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _PNP_EVENT_ENTRY
{
    LIST_ENTRY ListEntry;
    PLUGPLAY_EVENT_BLOCK Event;
} PNP_EVENT_ENTRY, *PPNP_EVENT_ENTRY;


/* GLOBALS *******************************************************************/

static LIST_ENTRY IopPnpEventQueueHead;
static KEVENT IopPnpNotifyEvent;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
NTSTATUS
IopInitPlugPlayEvents(VOID)
{
    InitializeListHead(&IopPnpEventQueueHead);

    KeInitializeEvent(&IopPnpNotifyEvent,
                      SynchronizationEvent,
                      FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
IopQueueTargetDeviceEvent(const GUID *Guid,
                          PUNICODE_STRING DeviceIds)
{
    PPNP_EVENT_ENTRY EventEntry;
    UNICODE_STRING Copy;
    ULONG TotalSize;
    NTSTATUS Status;

    ASSERT(DeviceIds);

    /* Allocate a big enough buffer */
    Copy.Length = 0;
    Copy.MaximumLength = DeviceIds->Length + sizeof(UNICODE_NULL);
    TotalSize =
        FIELD_OFFSET(PLUGPLAY_EVENT_BLOCK, TargetDevice.DeviceIds) +
        Copy.MaximumLength;

    EventEntry = ExAllocatePool(NonPagedPool,
                                TotalSize + FIELD_OFFSET(PNP_EVENT_ENTRY, Event));
    if (!EventEntry)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(EventEntry, TotalSize + FIELD_OFFSET(PNP_EVENT_ENTRY, Event));

    /* Fill the buffer with the event GUID */
    RtlCopyMemory(&EventEntry->Event.EventGuid,
                  Guid,
                  sizeof(GUID));
    EventEntry->Event.EventCategory = TargetDeviceChangeEvent;
    EventEntry->Event.TotalSize = TotalSize;

    /* Fill the device id */
    Copy.Buffer = EventEntry->Event.TargetDevice.DeviceIds;
    Status = RtlAppendUnicodeStringToString(&Copy, DeviceIds);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(EventEntry);
        return Status;
    }

    InsertHeadList(&IopPnpEventQueueHead,
                   &EventEntry->ListEntry);
    KeSetEvent(&IopPnpNotifyEvent,
               0,
               FALSE);

    return STATUS_SUCCESS;
}


static PDEVICE_OBJECT
IopTraverseDeviceNode(PDEVICE_NODE Node, PUNICODE_STRING DeviceInstance)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE ChildNode;

    if (RtlEqualUnicodeString(&Node->InstancePath,
                              DeviceInstance, TRUE))
    {
        ObReferenceObject(Node->PhysicalDeviceObject);
        return Node->PhysicalDeviceObject;
    }

    /* Traversal of all children nodes */
    for (ChildNode = Node->Child;
         ChildNode != NULL;
         ChildNode = ChildNode->Sibling)
    {
        DeviceObject = IopTraverseDeviceNode(ChildNode, DeviceInstance);
        if (DeviceObject != NULL)
        {
            return DeviceObject;
        }
    }

    return NULL;
}


PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance)
{
    if (IopRootDeviceNode == NULL)
        return NULL;

    if (DeviceInstance == NULL ||
        DeviceInstance->Length == 0)
    {
        if (IopRootDeviceNode->PhysicalDeviceObject)
        {
            ObReferenceObject(IopRootDeviceNode->PhysicalDeviceObject);
            return IopRootDeviceNode->PhysicalDeviceObject;
        }
        else
            return NULL;
    }

    return IopTraverseDeviceNode(IopRootDeviceNode, DeviceInstance);
}

static NTSTATUS
IopCaptureUnicodeString(PUNICODE_STRING DstName, PUNICODE_STRING SrcName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    volatile UNICODE_STRING Name;

    Name.Buffer = NULL;
    _SEH2_TRY
    {
        Name.Length = SrcName->Length;
        Name.MaximumLength = SrcName->MaximumLength;
        if (Name.Length > Name.MaximumLength)
        {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        if (Name.MaximumLength)
        {
            ProbeForRead(SrcName->Buffer,
                         Name.MaximumLength,
                         sizeof(WCHAR));
            Name.Buffer = ExAllocatePool(NonPagedPool, Name.MaximumLength);
            if (Name.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            memcpy(Name.Buffer, SrcName->Buffer, Name.MaximumLength);
        }

        *DstName = Name;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (Name.Buffer)
        {
            ExFreePool(Name.Buffer);
        }
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

/*
 * Remove the current PnP event from the tail of the event queue
 * and signal IopPnpNotifyEvent if there is yet another event in the queue.
 */
static
NTSTATUS
IopRemovePlugPlayEvent(
    _In_ PPLUGPLAY_CONTROL_USER_RESPONSE_DATA ResponseData)
{
    /* Remove a pnp event entry from the tail of the queue */
    if (!IsListEmpty(&IopPnpEventQueueHead))
    {
        ExFreePool(CONTAINING_RECORD(RemoveTailList(&IopPnpEventQueueHead), PNP_EVENT_ENTRY, ListEntry));
    }

    /* Signal the next pnp event in the queue */
    if (!IsListEmpty(&IopPnpEventQueueHead))
    {
        KeSetEvent(&IopPnpNotifyEvent,
                   0,
                   FALSE);
    }

    return STATUS_SUCCESS;
}


static NTSTATUS
IopGetInterfaceDeviceList(PPLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA DeviceList)
{
    NTSTATUS Status;
    PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA StackList;
    UNICODE_STRING DeviceInstance;
    PDEVICE_OBJECT DeviceObject = NULL;
    GUID FilterGuid;
    PZZWSTR SymbolicLinkList = NULL, LinkList;
    SIZE_T TotalLength;

    _SEH2_TRY
    {
        RtlCopyMemory(&StackList, DeviceList, sizeof(PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA));

        ProbeForRead(StackList.FilterGuid, sizeof(GUID), sizeof(UCHAR));
        RtlCopyMemory(&FilterGuid, StackList.FilterGuid, sizeof(GUID));

        if (StackList.Buffer != NULL && StackList.BufferSize != 0)
        {
            ProbeForWrite(StackList.Buffer, StackList.BufferSize, sizeof(UCHAR));
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = IopCaptureUnicodeString(&DeviceInstance, &StackList.DeviceInstance);
    if (NT_SUCCESS(Status))
    {
        /* Get the device object */
        DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
        if (DeviceInstance.Buffer != NULL)
        {
            ExFreePool(DeviceInstance.Buffer);
        }
    }

    Status = IoGetDeviceInterfaces(&FilterGuid, DeviceObject, StackList.Flags, &SymbolicLinkList);
    ObDereferenceObject(DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    LinkList = SymbolicLinkList;
    while (*SymbolicLinkList != UNICODE_NULL)
    {
        SymbolicLinkList += wcslen(SymbolicLinkList) + (sizeof(UNICODE_NULL) / sizeof(WCHAR));
    }
    TotalLength = ((SymbolicLinkList - LinkList + 1) * sizeof(WCHAR));

    _SEH2_TRY
    {
        if (StackList.Buffer != NULL &&
            StackList.BufferSize >= TotalLength)
        {
            // We've already probed the buffer for writing above.
            RtlCopyMemory(StackList.Buffer, LinkList, TotalLength);
        }

        DeviceList->BufferSize = TotalLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePool(LinkList);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    ExFreePool(LinkList);
    return STATUS_SUCCESS;
}

static NTSTATUS
IopGetDeviceProperty(PPLUGPLAY_CONTROL_PROPERTY_DATA PropertyData)
{
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_NODE DeviceNode;
    UNICODE_STRING DeviceInstance;
    ULONG BufferSize;
    ULONG Property;
    DEVICE_REGISTRY_PROPERTY DeviceProperty;
    PVOID Buffer;
    NTSTATUS Status;

    DPRINT("IopGetDeviceProperty() called\n");
    DPRINT("Device name: %wZ\n", &PropertyData->DeviceInstance);

    Status = IopCaptureUnicodeString(&DeviceInstance, &PropertyData->DeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    _SEH2_TRY
    {
        Property = PropertyData->Property;
        BufferSize = PropertyData->BufferSize;
        ProbeForWrite(PropertyData->Buffer,
                      BufferSize,
                      sizeof(UCHAR));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (DeviceInstance.Buffer != NULL)
        {
            ExFreePool(DeviceInstance.Buffer);
        }
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Get the device object */
    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
    if (DeviceInstance.Buffer != NULL)
    {
        ExFreePool(DeviceInstance.Buffer);
    }
    if (DeviceObject == NULL)
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    Buffer = ExAllocatePool(NonPagedPool, BufferSize);
    if (Buffer == NULL)
    {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    DeviceNode = ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;

    if (Property == PNP_PROPERTY_POWER_DATA)
    {
        if (BufferSize < sizeof(CM_POWER_DATA))
        {
            BufferSize = 0;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            DEVICE_CAPABILITIES DeviceCapabilities;
            PCM_POWER_DATA PowerData;
            IO_STACK_LOCATION Stack;
            IO_STATUS_BLOCK IoStatusBlock;

            PowerData = (PCM_POWER_DATA)Buffer;
            RtlZeroMemory(PowerData, sizeof(CM_POWER_DATA));
            PowerData->PD_Size = sizeof(CM_POWER_DATA);

            RtlZeroMemory(&DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
            DeviceCapabilities.Size = sizeof(DEVICE_CAPABILITIES);
            DeviceCapabilities.Version = 1;
            DeviceCapabilities.Address = -1;
            DeviceCapabilities.UINumber = -1;

            Stack.Parameters.DeviceCapabilities.Capabilities = &DeviceCapabilities;

            Status = IopInitiatePnpIrp(DeviceObject,
                                       &IoStatusBlock,
                                       IRP_MN_QUERY_CAPABILITIES,
                                       &Stack);
            if (NT_SUCCESS(Status))
            {
                DPRINT("Got device capabiliities\n");

                PowerData->PD_MostRecentPowerState = PowerDeviceD0; // FIXME
                if (DeviceCapabilities.DeviceD1)
                    PowerData->PD_Capabilities |= PDCAP_D1_SUPPORTED;
                if (DeviceCapabilities.DeviceD2)
                    PowerData->PD_Capabilities |= PDCAP_D2_SUPPORTED;
                if (DeviceCapabilities.WakeFromD0)
                    PowerData->PD_Capabilities |= PDCAP_WAKE_FROM_D0_SUPPORTED;
                if (DeviceCapabilities.WakeFromD1)
                    PowerData->PD_Capabilities |= PDCAP_WAKE_FROM_D1_SUPPORTED;
                if (DeviceCapabilities.WakeFromD2)
                    PowerData->PD_Capabilities |= PDCAP_WAKE_FROM_D2_SUPPORTED;
                if (DeviceCapabilities.WakeFromD3)
                    PowerData->PD_Capabilities |= PDCAP_WAKE_FROM_D3_SUPPORTED;
                if (DeviceCapabilities.WarmEjectSupported)
                    PowerData->PD_Capabilities |= PDCAP_WARM_EJECT_SUPPORTED;
                PowerData->PD_D1Latency = DeviceCapabilities.D1Latency;
                PowerData->PD_D2Latency = DeviceCapabilities.D2Latency;
                PowerData->PD_D3Latency = DeviceCapabilities.D3Latency;
                RtlCopyMemory(&PowerData->PD_PowerStateMapping,
                              &DeviceCapabilities.DeviceState,
                              sizeof(DeviceCapabilities.DeviceState));
                PowerData->PD_DeepestSystemWake = DeviceCapabilities.SystemWake;
            }
            else
            {
                DPRINT("IRP_MN_QUERY_CAPABILITIES failed (Status 0x%08lx)\n", Status);

                PowerData->PD_Capabilities = PDCAP_D0_SUPPORTED | PDCAP_D3_SUPPORTED;
                PowerData->PD_MostRecentPowerState = PowerDeviceD0;
            }
        }
    }
    else if (Property == PNP_PROPERTY_REMOVAL_POLICY_OVERRIDE)
    {
        UNIMPLEMENTED;
        BufferSize = 0;
        Status = STATUS_NOT_IMPLEMENTED;
    }
    else if (Property == PNP_PROPERTY_REMOVAL_POLICY_HARDWARE_DEFAULT)
    {
        if (BufferSize < sizeof(DeviceNode->HardwareRemovalPolicy))
        {
            BufferSize = 0;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            BufferSize = sizeof(DeviceNode->HardwareRemovalPolicy);
            RtlCopyMemory(Buffer,
                          &DeviceNode->HardwareRemovalPolicy,
                          BufferSize);
        }
    }
    else
    {
        switch (Property)
        {
            case PNP_PROPERTY_UI_NUMBER:
                DeviceProperty = DevicePropertyUINumber;
                break;

            case PNP_PROPERTY_PHYSICAL_DEVICE_OBJECT_NAME:
                DeviceProperty = DevicePropertyPhysicalDeviceObjectName;
                break;

            case PNP_PROPERTY_BUSTYPEGUID:
                DeviceProperty = DevicePropertyBusTypeGuid;
                break;

            case PNP_PROPERTY_LEGACYBUSTYPE:
                DeviceProperty = DevicePropertyLegacyBusType;
                break;

            case PNP_PROPERTY_BUSNUMBER:
                DeviceProperty = DevicePropertyBusNumber;
                break;

            case PNP_PROPERTY_REMOVAL_POLICY:
                DeviceProperty = DevicePropertyRemovalPolicy;
                break;

            case PNP_PROPERTY_ADDRESS:
                DeviceProperty = DevicePropertyAddress;
                break;

            case PNP_PROPERTY_ENUMERATOR_NAME:
                DeviceProperty = DevicePropertyEnumeratorName;
                break;

            case PNP_PROPERTY_INSTALL_STATE:
                DeviceProperty = DevicePropertyInstallState;
                break;

#if (WINVER >= _WIN32_WINNT_WS03)
            case PNP_PROPERTY_LOCATION_PATHS:
                UNIMPLEMENTED;
                BufferSize = 0;
                Status = STATUS_NOT_IMPLEMENTED;
                break;
#endif

#if (WINVER >= _WIN32_WINNT_WIN7)
            case PNP_PROPERTY_CONTAINERID:
                DeviceProperty = DevicePropertyContainerID;
                break;
#endif

            default:
                BufferSize = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
        }

        if (Status == STATUS_SUCCESS)
        {
            Status = IoGetDeviceProperty(DeviceObject,
                                         DeviceProperty,
                                         BufferSize,
                                         Buffer,
                                         &BufferSize);
        }
    }

    ObDereferenceObject(DeviceObject);

    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            RtlCopyMemory(PropertyData->Buffer, Buffer, BufferSize);
            PropertyData->BufferSize = BufferSize;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    ExFreePool(Buffer);
    return Status;
}


static NTSTATUS
IopGetRelatedDevice(PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA RelatedDeviceData)
{
    UNICODE_STRING RootDeviceName;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_NODE DeviceNode = NULL;
    PDEVICE_NODE RelatedDeviceNode;
    UNICODE_STRING TargetDeviceInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Relation = 0;
    ULONG MaximumLength = 0;

    DPRINT("IopGetRelatedDevice() called\n");
    DPRINT("Device name: %wZ\n", &RelatedDeviceData->TargetDeviceInstance);

    Status = IopCaptureUnicodeString(&TargetDeviceInstance, &RelatedDeviceData->TargetDeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    _SEH2_TRY
    {
        Relation = RelatedDeviceData->Relation;
        MaximumLength = RelatedDeviceData->RelatedDeviceInstanceLength;
        ProbeForWrite(RelatedDeviceData->RelatedDeviceInstance,
                      MaximumLength,
                      sizeof(WCHAR));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (TargetDeviceInstance.Buffer != NULL)
        {
            ExFreePool(TargetDeviceInstance.Buffer);
        }
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    RtlInitUnicodeString(&RootDeviceName,
                         L"HTREE\\ROOT\\0");
    if (RtlEqualUnicodeString(&TargetDeviceInstance,
                              &RootDeviceName,
                              TRUE))
    {
        DeviceNode = IopRootDeviceNode;
        if (TargetDeviceInstance.Buffer != NULL)
        {
            ExFreePool(TargetDeviceInstance.Buffer);
        }
    }
    else
    {
        /* Get the device object */
        DeviceObject = IopGetDeviceObjectFromDeviceInstance(&TargetDeviceInstance);
        if (TargetDeviceInstance.Buffer != NULL)
        {
            ExFreePool(TargetDeviceInstance.Buffer);
        }
        if (DeviceObject == NULL)
            return STATUS_NO_SUCH_DEVICE;

        DeviceNode = ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;
    }

    switch (Relation)
    {
        case PNP_GET_PARENT_DEVICE:
            RelatedDeviceNode = DeviceNode->Parent;
            break;

        case PNP_GET_CHILD_DEVICE:
            RelatedDeviceNode = DeviceNode->Child;
            break;

        case PNP_GET_SIBLING_DEVICE:
            RelatedDeviceNode = DeviceNode->Sibling;
            break;

        default:
            if (DeviceObject != NULL)
            {
                ObDereferenceObject(DeviceObject);
            }

            return STATUS_INVALID_PARAMETER;
    }

    if (RelatedDeviceNode == NULL)
    {
        if (DeviceObject)
        {
            ObDereferenceObject(DeviceObject);
        }

        return STATUS_NO_SUCH_DEVICE;
    }

    if (RelatedDeviceNode->InstancePath.Length > MaximumLength)
    {
        if (DeviceObject)
        {
            ObDereferenceObject(DeviceObject);
        }

        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy related device instance name */
    _SEH2_TRY
    {
        RtlCopyMemory(RelatedDeviceData->RelatedDeviceInstance,
                      RelatedDeviceNode->InstancePath.Buffer,
                      RelatedDeviceNode->InstancePath.Length);
        RelatedDeviceData->RelatedDeviceInstanceLength = RelatedDeviceNode->InstancePath.Length;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (DeviceObject != NULL)
    {
        ObDereferenceObject(DeviceObject);
    }

    DPRINT("IopGetRelatedDevice() done\n");

    return Status;
}

static
BOOLEAN
PiIsDevNodeStarted(
    _In_ PDEVICE_NODE DeviceNode)
{
    return (DeviceNode->State == DeviceNodeStartPending ||
            DeviceNode->State == DeviceNodeStartCompletion ||
            DeviceNode->State == DeviceNodeStartPostWork ||
            DeviceNode->State == DeviceNodeStarted ||
            DeviceNode->State == DeviceNodeQueryStopped ||
            DeviceNode->State == DeviceNodeEnumeratePending ||
            DeviceNode->State == DeviceNodeEnumerateCompletion ||
            DeviceNode->State == DeviceNodeStopped ||
            DeviceNode->State == DeviceNodeRestartCompletion);
}

static ULONG
IopGetDeviceNodeStatus(PDEVICE_NODE DeviceNode)
{
    ULONG Output = DN_NT_ENUMERATOR | DN_NT_DRIVER;

    if (DeviceNode->Parent == IopRootDeviceNode)
        Output |= DN_ROOT_ENUMERATED;

    // FIXME: review for deleted and removed states
    if (DeviceNode->State >= DeviceNodeDriversAdded)
        Output |= DN_DRIVER_LOADED;

    if (PiIsDevNodeStarted(DeviceNode))
        Output |= DN_STARTED;

    if (DeviceNode->UserFlags & DNUF_WILL_BE_REMOVED)
        Output |= DN_WILL_BE_REMOVED;

    if (DeviceNode->Flags & DNF_HAS_PROBLEM)
        Output |= DN_HAS_PROBLEM;

    if (DeviceNode->Flags & DNF_HAS_PRIVATE_PROBLEM)
        Output |= DN_PRIVATE_PROBLEM;

    if (DeviceNode->Flags & DNF_DRIVER_BLOCKED)
        Output |= DN_DRIVER_BLOCKED;

    if (DeviceNode->Flags & DNF_CHILD_WITH_INVALID_ID)
        Output |= DN_CHILD_WITH_INVALID_ID;

    if (DeviceNode->Flags & DNF_HAS_PRIVATE_PROBLEM)
        Output |= DN_PRIVATE_PROBLEM;

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER)
        Output |= DN_LEGACY_DRIVER;

    if (DeviceNode->UserFlags & DNUF_DONT_SHOW_IN_UI)
        Output |= DN_NO_SHOW_IN_DM;

    if (!(DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE))
        Output |= DN_DISABLEABLE;

    return Output;
}

static NTSTATUS
IopDeviceStatus(PPLUGPLAY_CONTROL_STATUS_DATA StatusData)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    ULONG Operation = 0;
    ULONG DeviceStatus = 0;
    ULONG DeviceProblem = 0;
    UNICODE_STRING DeviceInstance;
    NTSTATUS Status;

    DPRINT("IopDeviceStatus() called\n");

    Status = IopCaptureUnicodeString(&DeviceInstance, &StatusData->DeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("Device name: '%wZ'\n", &DeviceInstance);

    _SEH2_TRY
    {
        Operation = StatusData->Operation;
        if (Operation == PNP_SET_DEVICE_STATUS)
        {
            DeviceStatus = StatusData->DeviceStatus;
            DeviceProblem = StatusData->DeviceProblem;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (DeviceInstance.Buffer != NULL)
        {
            ExFreePool(DeviceInstance.Buffer);
        }
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Get the device object */
    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
    if (DeviceInstance.Buffer != NULL)
    {
        ExFreePool(DeviceInstance.Buffer);
    }
    if (DeviceObject == NULL)
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    DeviceNode = IopGetDeviceNode(DeviceObject);

    switch (Operation)
    {
        case PNP_GET_DEVICE_STATUS:
            DPRINT("Get status data\n");
            DeviceStatus = IopGetDeviceNodeStatus(DeviceNode);
            DeviceProblem = DeviceNode->Problem;
            break;

        case PNP_SET_DEVICE_STATUS:
            DPRINT1("Set status data is NOT SUPPORTED\n");
            break;

        case PNP_CLEAR_DEVICE_STATUS:
            DPRINT1("FIXME: Clear status data!\n");
            break;
    }

    ObDereferenceObject(DeviceObject);

    if (Operation == PNP_GET_DEVICE_STATUS)
    {
        _SEH2_TRY
        {
            StatusData->DeviceStatus = DeviceStatus;
            StatusData->DeviceProblem = DeviceProblem;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

static
NTSTATUS
IopGetDeviceRelations(PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA RelationsData)
{
    UNICODE_STRING DeviceInstance;
    PDEVICE_OBJECT DeviceObject = NULL;
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    PDEVICE_OBJECT ChildDeviceObject;
    PDEVICE_NODE ChildDeviceNode;
    ULONG i;
    ULONG Relations;
    ULONG BufferSize, RequiredSize;
    ULONG BufferLeft;
    PWCHAR Buffer, Ptr;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopGetDeviceRelations() called\n");
    DPRINT("Device name: %wZ\n", &RelationsData->DeviceInstance);
    DPRINT("Relations: %lu\n", RelationsData->Relations);
    DPRINT("BufferSize: %lu\n", RelationsData->BufferSize);
    DPRINT("Buffer: %p\n", RelationsData->Buffer);

    _SEH2_TRY
    {
        Relations = RelationsData->Relations;
        BufferSize = RelationsData->BufferSize;
        Buffer = RelationsData->Buffer;

        ProbeForWrite(Buffer, BufferSize, sizeof(CHAR));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = IopCaptureUnicodeString(&DeviceInstance, &RelationsData->DeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopCaptureUnicodeString() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Get the device object */
    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
    if (DeviceObject == NULL)
    {
        DPRINT1("IopGetDeviceObjectFromDeviceInstance() returned NULL\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto done;
    }

    switch (Relations)
    {
        case PNP_EJECT_RELATIONS:
            Stack.Parameters.QueryDeviceRelations.Type = EjectionRelations;
            break;

        case PNP_REMOVAL_RELATIONS:
            Stack.Parameters.QueryDeviceRelations.Type = RemovalRelations;
            break;

        case PNP_POWER_RELATIONS:
            Stack.Parameters.QueryDeviceRelations.Type = PowerRelations;
            break;

        case PNP_BUS_RELATIONS:
            Stack.Parameters.QueryDeviceRelations.Type = BusRelations;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            goto done;
    }

    Status = IopInitiatePnpIrp(DeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopInitiatePnpIrp() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;

    DPRINT("Found %d device relations\n", DeviceRelations->Count);

    _SEH2_TRY
    {
        RequiredSize = 0;
        BufferLeft = BufferSize;
        Ptr = Buffer;

        for (i = 0; i < DeviceRelations->Count; i++)
        {
            ChildDeviceObject = DeviceRelations->Objects[i];

            ChildDeviceNode = IopGetDeviceNode(ChildDeviceObject);
            if (ChildDeviceNode)
            {
                DPRINT("Device instance: %wZ\n", &ChildDeviceNode->InstancePath);
                DPRINT("RequiredSize: %hu\n", ChildDeviceNode->InstancePath.Length + sizeof(WCHAR));

                if (Ptr != NULL)
                {
                    if (BufferLeft < ChildDeviceNode->InstancePath.Length + 2 * sizeof(WCHAR))
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                    }

                    RtlCopyMemory(Ptr,
                                  ChildDeviceNode->InstancePath.Buffer,
                                  ChildDeviceNode->InstancePath.Length);
                    Ptr = (PWCHAR)((ULONG_PTR)Ptr + ChildDeviceNode->InstancePath.Length);
                    *Ptr = UNICODE_NULL;
                    Ptr = (PWCHAR)((ULONG_PTR)Ptr + sizeof(WCHAR));

                    BufferLeft -= (ChildDeviceNode->InstancePath.Length + sizeof(WCHAR));
                }

                RequiredSize += (ChildDeviceNode->InstancePath.Length + sizeof(WCHAR));
            }
        }

        if (Ptr != NULL && BufferLeft >= sizeof(WCHAR))
            *Ptr = UNICODE_NULL;

        if (RequiredSize > 0)
            RequiredSize += sizeof(WCHAR);

        DPRINT("BufferSize: %lu  RequiredSize: %lu\n", RelationsData->BufferSize, RequiredSize);

        RelationsData->BufferSize = RequiredSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

done:
    if (DeviceRelations != NULL)
        ExFreePool(DeviceRelations);

    if (DeviceObject != NULL)
        ObDereferenceObject(DeviceObject);

    if (DeviceInstance.Buffer != NULL)
        ExFreePool(DeviceInstance.Buffer);

    return Status;
}

static NTSTATUS
IopGetDeviceDepth(PPLUGPLAY_CONTROL_DEPTH_DATA DepthData)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    UNICODE_STRING DeviceInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopGetDeviceDepth() called\n");
    DPRINT("Device name: %wZ\n", &DepthData->DeviceInstance);

    Status = IopCaptureUnicodeString(&DeviceInstance, &DepthData->DeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the device object */
    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
    if (DeviceInstance.Buffer != NULL)
    {
        ExFreePool(DeviceInstance.Buffer);
    }
    if (DeviceObject == NULL)
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    DeviceNode = IopGetDeviceNode(DeviceObject);

    _SEH2_TRY
    {
        DepthData->Depth = DeviceNode->Level;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(DeviceObject);

    return Status;
}

static
NTSTATUS
PiControlSyncDeviceAction(
    _In_ PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA DeviceData,
    _In_ PLUGPLAY_CONTROL_CLASS ControlClass)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    UNICODE_STRING DeviceInstance;

    ASSERT(ControlClass == PlugPlayControlEnumerateDevice ||
           ControlClass == PlugPlayControlStartDevice ||
           ControlClass == PlugPlayControlResetDevice);

    Status = IopCaptureUnicodeString(&DeviceInstance, &DeviceData->DeviceInstance);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);
    if (DeviceInstance.Buffer != NULL)
    {
        ExFreePool(DeviceInstance.Buffer);
    }
    if (DeviceObject == NULL)
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    DEVICE_ACTION Action;

    switch (ControlClass)
    {
        case PlugPlayControlEnumerateDevice:
            Action = PiActionEnumDeviceTree;
            break;
        case PlugPlayControlStartDevice:
            Action = PiActionStartDevice;
            break;
        case PlugPlayControlResetDevice:
            Action = PiActionResetDevice;
            break;
        default:
            UNREACHABLE;
            break;
    }

    Status = PiPerformSyncDeviceAction(DeviceObject, Action);

    ObDereferenceObject(DeviceObject);

    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * Plug and Play event structure used by NtGetPlugPlayEvent.
 *
 * EventGuid
 *    Can be one of the following values:
 *       GUID_HWPROFILE_QUERY_CHANGE
 *       GUID_HWPROFILE_CHANGE_CANCELLED
 *       GUID_HWPROFILE_CHANGE_COMPLETE
 *       GUID_TARGET_DEVICE_QUERY_REMOVE
 *       GUID_TARGET_DEVICE_REMOVE_CANCELLED
 *       GUID_TARGET_DEVICE_REMOVE_COMPLETE
 *       GUID_PNP_CUSTOM_NOTIFICATION
 *       GUID_PNP_POWER_NOTIFICATION
 *       GUID_DEVICE_* (see above)
 *
 * EventCategory
 *    Type of the event that happened.
 *
 * Result
 *    ?
 *
 * Flags
 *    ?
 *
 * TotalSize
 *    Size of the event block including the device IDs and other
 *    per category specific fields.
 */

/*
 * NtGetPlugPlayEvent
 *
 * Returns one Plug & Play event from a global queue.
 *
 * Parameters
 *    Reserved1
 *    Reserved2
 *       Always set to zero.
 *
 *    Buffer
 *       The buffer that will be filled with the event information on
 *       successful return from the function.
 *
 *    BufferSize
 *       Size of the buffer pointed by the Buffer parameter. If the
 *       buffer size is not large enough to hold the whole event
 *       information, error STATUS_BUFFER_TOO_SMALL is returned and
 *       the buffer remains untouched.
 *
 * Return Values
 *    STATUS_PRIVILEGE_NOT_HELD
 *    STATUS_BUFFER_TOO_SMALL
 *    STATUS_SUCCESS
 *
 * Remarks
 *    This function isn't multi-thread safe!
 *
 * @implemented
 */
NTSTATUS
NTAPI
NtGetPlugPlayEvent(IN ULONG Reserved1,
                   IN ULONG Reserved2,
                   OUT PPLUGPLAY_EVENT_BLOCK Buffer,
                   IN ULONG BufferSize)
{
    PPNP_EVENT_ENTRY Entry;
    NTSTATUS Status;

    DPRINT("NtGetPlugPlayEvent() called\n");

    /* Function can only be called from user-mode */
    if (KeGetPreviousMode() == KernelMode)
    {
        DPRINT1("NtGetPlugPlayEvent cannot be called from kernel mode!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check for Tcb privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                UserMode))
    {
        DPRINT1("NtGetPlugPlayEvent: Caller does not hold the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Wait for a PnP event */
    DPRINT("Waiting for pnp notification event\n");
    Status = KeWaitForSingleObject(&IopPnpNotifyEvent,
                                   UserRequest,
                                   UserMode,
                                   FALSE,
                                   NULL);
    if (!NT_SUCCESS(Status) || Status == STATUS_USER_APC)
    {
        DPRINT("KeWaitForSingleObject() failed (Status %lx)\n", Status);
        ASSERT(Status == STATUS_USER_APC);
        return Status;
    }

    /* Get entry from the tail of the queue */
    Entry = CONTAINING_RECORD(IopPnpEventQueueHead.Blink,
                              PNP_EVENT_ENTRY,
                              ListEntry);

    /* Check the buffer size */
    if (BufferSize < Entry->Event.TotalSize)
    {
        DPRINT1("Buffer is too small for the pnp-event\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy event data to the user buffer */
    _SEH2_TRY
    {
        ProbeForWrite(Buffer,
                      Entry->Event.TotalSize,
                      sizeof(UCHAR));
        RtlCopyMemory(Buffer,
                      &Entry->Event,
                      Entry->Event.TotalSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DPRINT("NtGetPlugPlayEvent() done\n");

    return STATUS_SUCCESS;
}

/*
 * NtPlugPlayControl
 *
 * A function for doing various Plug & Play operations from user mode.
 *
 * Parameters
 *    PlugPlayControlClass
 *       0x00   Reenumerate device tree
 *
 *              Buffer points to UNICODE_STRING decribing the instance
 *              path (like "HTREE\ROOT\0" or "Root\ACPI_HAL\0000"). For
 *              more information about instance paths see !devnode command
 *              in kernel debugger or look at "Inside Windows 2000" book,
 *              chapter "Driver Loading, Initialization, and Installation".
 *
 *       0x01   Register new device
 *       0x02   Deregister device
 *       0x03   Initialize device
 *       0x04   Start device
 *       0x06   Query and remove device
 *       0x07   User response
 *
 *              Called after processing the message from NtGetPlugPlayEvent.
 *
 *       0x08   Generate legacy device
 *       0x09   Get interface device list
 *       0x0A   Get property data
 *       0x0B   Device class association (Registration)
 *       0x0C   Get related device
 *       0x0D   Get device interface alias
 *       0x0E   Get/set/clear device status
 *       0x0F   Get device depth
 *       0x10   Query device relations
 *       0x11   Query target device relation
 *       0x12   Query conflict list
 *       0x13   Retrieve dock data
 *       0x14   Reset device
 *       0x15   Halt device
 *       0x16   Get blocked driver data
 *
 *    Buffer
 *       The buffer contains information that is specific to each control
 *       code. The buffer is read-only.
 *
 *    BufferSize
 *       Size of the buffer pointed by the Buffer parameter. If the
 *       buffer size specifies incorrect value for specified control
 *       code, error ??? is returned.
 *
 * Return Values
 *    STATUS_PRIVILEGE_NOT_HELD
 *    STATUS_SUCCESS
 *    ...
 *
 * @unimplemented
 */
NTSTATUS
NTAPI
NtPlugPlayControl(IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
                  IN OUT PVOID Buffer,
                  IN ULONG BufferLength)
{
    DPRINT("NtPlugPlayControl(%d %p %lu) called\n",
           PlugPlayControlClass, Buffer, BufferLength);

    /* Function can only be called from user-mode */
    if (KeGetPreviousMode() == KernelMode)
    {
        DPRINT1("NtGetPlugPlayEvent cannot be called from kernel mode!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check for Tcb privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                UserMode))
    {
        DPRINT1("NtGetPlugPlayEvent: Caller does not hold the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Probe the buffer */
    _SEH2_TRY
    {
        ProbeForWrite(Buffer,
                      BufferLength,
                      sizeof(ULONG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    switch (PlugPlayControlClass)
    {
        case PlugPlayControlEnumerateDevice:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_ENUMERATE_DEVICE_DATA))
                return STATUS_INVALID_PARAMETER;
            // the Flags field is not used anyway
            return PiControlSyncDeviceAction((PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)Buffer,
                                             PlugPlayControlClass);

//        case PlugPlayControlRegisterNewDevice:
//        case PlugPlayControlDeregisterDevice:
//        case PlugPlayControlInitializeDevice:

        case PlugPlayControlStartDevice:
        case PlugPlayControlResetDevice:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA))
                return STATUS_INVALID_PARAMETER;
            return PiControlSyncDeviceAction((PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)Buffer,
                                             PlugPlayControlClass);

//        case PlugPlayControlUnlockDevice:
//        case PlugPlayControlQueryAndRemoveDevice:

        case PlugPlayControlUserResponse:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_USER_RESPONSE_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopRemovePlugPlayEvent((PPLUGPLAY_CONTROL_USER_RESPONSE_DATA)Buffer);

//        case PlugPlayControlGenerateLegacyDevice:

        case PlugPlayControlGetInterfaceDeviceList:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetInterfaceDeviceList((PPLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA)Buffer);

        case PlugPlayControlProperty:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetDeviceProperty((PPLUGPLAY_CONTROL_PROPERTY_DATA)Buffer);

//        case PlugPlayControlDeviceClassAssociation:

        case PlugPlayControlGetRelatedDevice:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetRelatedDevice((PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA)Buffer);

//        case PlugPlayControlGetInterfaceDeviceAlias:

        case PlugPlayControlDeviceStatus:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_STATUS_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopDeviceStatus((PPLUGPLAY_CONTROL_STATUS_DATA)Buffer);

        case PlugPlayControlGetDeviceDepth:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_DEPTH_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetDeviceDepth((PPLUGPLAY_CONTROL_DEPTH_DATA)Buffer);

        case PlugPlayControlQueryDeviceRelations:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetDeviceRelations((PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA)Buffer);

//        case PlugPlayControlTargetDeviceRelation:
//        case PlugPlayControlQueryConflictList:
//        case PlugPlayControlRetrieveDock:
//        case PlugPlayControlHaltDevice:
//        case PlugPlayControlGetBlockedDriverList:

        default:
            return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_NOT_IMPLEMENTED;
}
