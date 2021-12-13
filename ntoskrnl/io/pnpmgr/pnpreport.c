/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpreport.c
 * PURPOSE:         Device Changes Reporting Functions
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* TYPES *******************************************************************/

typedef struct _INTERNAL_WORK_QUEUE_ITEM
{
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_CHANGE_COMPLETE_CALLBACK Callback;
    PVOID Context;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure;
} INTERNAL_WORK_QUEUE_ITEM, *PINTERNAL_WORK_QUEUE_ITEM;

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
                         PDEVICE_NODE DeviceNode);

NTSTATUS
PpSetCustomTargetEvent(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PKEVENT SyncEvent OPTIONAL,
                       IN OUT PNTSTATUS SyncStatus OPTIONAL,
                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                       IN PVOID Context OPTIONAL,
                       IN PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure);

/* PRIVATE FUNCTIONS *********************************************************/

PWCHAR
IopGetInterfaceTypeString(INTERFACE_TYPE IfType)
{
    switch (IfType)
    {
        case Internal:
            return L"Internal";

        case Isa:
            return L"Isa";

        case Eisa:
            return L"Eisa";

        case MicroChannel:
            return L"MicroChannel";

        case TurboChannel:
            return L"TurboChannel";

        case PCIBus:
            return L"PCIBus";

        case VMEBus:
            return L"VMEBus";

        case NuBus:
            return L"NuBus";

        case PCMCIABus:
            return L"PCMCIABus";

        case CBus:
            return L"CBus";

        case MPIBus:
            return L"MPIBus";

        case MPSABus:
            return L"MPSABus";

        case ProcessorInternal:
            return L"ProcessorInternal";

        case PNPISABus:
            return L"PNPISABus";

        case PNPBus:
            return L"PNPBus";

        case Vmcs:
            return L"Vmcs";

        default:
            DPRINT1("Invalid bus type: %d\n", IfType);
            return NULL;
    }
}

VOID
NTAPI
IopReportTargetDeviceChangeAsyncWorker(PVOID Context)
{
    PINTERNAL_WORK_QUEUE_ITEM Item;

    Item = (PINTERNAL_WORK_QUEUE_ITEM)Context;
    PpSetCustomTargetEvent(Item->PhysicalDeviceObject, NULL, NULL, Item->Callback, Item->Context, Item->NotificationStructure);
    ObDereferenceObject(Item->PhysicalDeviceObject);
    ExFreePoolWithTag(Context, '  pP');
}

NTSTATUS
PpSetCustomTargetEvent(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PKEVENT SyncEvent OPTIONAL,
                       IN OUT PNTSTATUS SyncStatus OPTIONAL,
                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                       IN PVOID Context OPTIONAL,
                       IN PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure)
{
    ASSERT(NotificationStructure != NULL);
    ASSERT(DeviceObject != NULL);

    if (SyncEvent)
    {
        ASSERT(SyncStatus);
        *SyncStatus = STATUS_PENDING;
    }

    /* That call is totally wrong but notifications handler must be fixed first */
    PiNotifyTargetDeviceChange(&GUID_PNP_CUSTOM_NOTIFICATION, DeviceObject, NotificationStructure);

    if (SyncEvent)
    {
        KeSetEvent(SyncEvent, IO_NO_INCREMENT, FALSE);
        *SyncStatus = STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportDetectedDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ INTERFACE_TYPE LegacyBusType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    _In_ BOOLEAN ResourceAssigned,
    _Inout_ PDEVICE_OBJECT *DeviceObject)
{
    UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING DeviceReportedName = RTL_CONSTANT_STRING(L"DeviceReported");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_NODE DeviceNode;
    PDEVICE_OBJECT Pdo;
    NTSTATUS Status;
    HANDLE InstanceKey, ControlKey;
    UNICODE_STRING ValueName, ServiceLongName, ServiceName;
    WCHAR HardwareId[256];
    PWCHAR IfString;
    ULONG IdLength;
    ULONG LegacyValue;
    ULONG DeviceReported = 1;

    DPRINT("IoReportDetectedDevice (DeviceObject %p, *DeviceObject %p)\n",
           DeviceObject, DeviceObject ? *DeviceObject : NULL);

    ServiceLongName = DriverObject->DriverExtension->ServiceKeyName;
    ServiceName = ServiceLongName;

    /* If the interface type is unknown, treat it as internal */
    if (LegacyBusType == InterfaceTypeUndefined)
        LegacyBusType = Internal;

    /* Get the string equivalent of the interface type */
    IfString = IopGetInterfaceTypeString(LegacyBusType);

    /* If NULL is returned then it's a bad type */
    if (!IfString)
        return STATUS_INVALID_PARAMETER;

    /*
     * Drivers that have been created via a direct IoCreateDriver() call
     * have their ServiceKeyName set to \Driver\DriverName. We need to
     * strip everything up to the last path separator and keep what remains.
     */
    if (DriverObject->Flags & DRVO_BUILTIN_DRIVER)
    {
        /*
         * Find the last path separator.
         * NOTE: Since ServiceName is not necessarily NULL-terminated,
         * we cannot use wcsrchr().
         */
        if (ServiceName.Buffer && ServiceName.Length >= sizeof(WCHAR))
        {
            ValueName.Length = 1;
            ValueName.Buffer = ServiceName.Buffer + (ServiceName.Length / sizeof(WCHAR)) - 1;

            while ((ValueName.Buffer > ServiceName.Buffer) && (*ValueName.Buffer != L'\\'))
            {
                --ValueName.Buffer;
                ++ValueName.Length;
            }
            if (*ValueName.Buffer == L'\\')
            {
                ++ValueName.Buffer;
                --ValueName.Length;
            }
            ValueName.Length *= sizeof(WCHAR);

            /* Shorten the string */
            ServiceName.MaximumLength -= (ServiceName.Length - ValueName.Length);
            ServiceName.Length = ValueName.Length;
            ServiceName.Buffer = ValueName.Buffer;
        }
    }

    /* We use the caller's PDO if they supplied one */
    UNICODE_STRING instancePath;
    if (DeviceObject && *DeviceObject)
    {
        Pdo = *DeviceObject;
    }
    else
    {
        /* Create the PDO */
        Status = PnpRootCreateDevice(&ServiceName, NULL, &Pdo, &instancePath);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PnpRootCreateDevice() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    /* Create the device node for the new PDO */
    DeviceNode = PipAllocateDeviceNode(Pdo);
    if (!DeviceNode)
    {
        DPRINT("PipAllocateDeviceNode() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlDuplicateUnicodeString(0, &instancePath, &DeviceNode->InstancePath);

    /* Open a handle to the instance path key */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Save the driver name */
    RtlInitUnicodeString(&ValueName, L"Service");
    Status = ZwSetValueKey(InstanceKey, &ValueName, 0, REG_SZ, ServiceLongName.Buffer, ServiceLongName.Length + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to write the Service name value: 0x%x\n", Status);
    }

    /* Report as non-legacy driver */
    RtlInitUnicodeString(&ValueName, L"Legacy");
    LegacyValue = 0;
    Status = ZwSetValueKey(InstanceKey, &ValueName, 0, REG_DWORD, &LegacyValue, sizeof(LegacyValue));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to write the Legacy value: 0x%x\n", Status);
    }
    Status = ZwSetValueKey(InstanceKey, &DeviceReportedName, 0, REG_DWORD, &DeviceReported, sizeof(DeviceReported));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to write the DeviceReported value: 0x%x\n", Status);
    }

    /* Set DeviceReported=1 in Control subkey */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Control,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               InstanceKey,
                               NULL);
    Status = ZwCreateKey(&ControlKey,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (NT_SUCCESS(Status))
    {
        Status = ZwSetValueKey(ControlKey,
                               &DeviceReportedName,
                               0,
                               REG_DWORD,
                               &DeviceReported,
                               sizeof(DeviceReported));
        ZwClose(ControlKey);
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set ReportedDevice=1 for device %wZ (status 0x%08lx)\n", &instancePath, Status);
    }

    /* Add DETECTEDInterfaceType\DriverName */
    IdLength = 0;
    IdLength += swprintf(&HardwareId[IdLength],
                         L"DETECTED%ls\\%wZ",
                         IfString,
                         &ServiceName);
    IdLength++;

    /* Add DETECTED\DriverName */
    IdLength += swprintf(&HardwareId[IdLength],
                         L"DETECTED\\%wZ",
                         &ServiceName);
    IdLength++;

    /* Terminate the string with another null */
    HardwareId[IdLength++] = UNICODE_NULL;

    /* Store the value for CompatibleIDs */
    RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
    Status = ZwSetValueKey(InstanceKey, &ValueName, 0, REG_MULTI_SZ, HardwareId, IdLength * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to write the compatible IDs: 0x%x\n", Status);
        ZwClose(InstanceKey);
        return Status;
    }

    // Set the device's DeviceDesc and LocationInformation fields
    PiSetDevNodeText(DeviceNode, InstanceKey);

    /* Assign the resources to the device node */
    DeviceNode->BootResources = ResourceList;
    DeviceNode->ResourceRequirements = ResourceRequirements;

    /* Set appropriate flags */
    if (DeviceNode->BootResources)
        IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);

    if (!DeviceNode->ResourceRequirements && !DeviceNode->BootResources)
        IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);

    /* Write the resource information to the registry */
    IopSetDeviceInstanceData(InstanceKey, DeviceNode);

    /* If the caller didn't get the resources assigned for us, do it now */
    if (!ResourceAssigned)
    {
        Status = IopAssignDeviceResources(DeviceNode);

        /* See if we failed */
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Assigning resources failed: 0x%x\n", Status);
            ZwClose(InstanceKey);
            return Status;
        }
    }

    /* Close the instance key handle */
    ZwClose(InstanceKey);

    /* Register the given DO with PnP root if required */
    if (DeviceObject && *DeviceObject)
        PnpRootRegisterDevice(*DeviceObject);

    PiInsertDevNode(DeviceNode, IopRootDeviceNode);
    DeviceNode->Flags |= DNF_MADEUP | DNF_ENUMERATED;

    // we still need to query IDs, send events and reenumerate this node
    PiSetDevNodeState(DeviceNode, DeviceNodeStartPostWork);

    DPRINT("Reported device: %S (%wZ)\n", HardwareId, &DeviceNode->InstancePath);

    PiQueueDeviceAction(Pdo, PiActionEnumDeviceTree, NULL, NULL);

    /* Return the PDO */
    if (DeviceObject) *DeviceObject = Pdo;

    return STATUS_SUCCESS;
}

/*
 * @halfplemented
 */
NTSTATUS
NTAPI
IoReportResourceForDetection(IN PDRIVER_OBJECT DriverObject,
                             IN PCM_RESOURCE_LIST DriverList OPTIONAL,
                             IN ULONG DriverListSize OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                             IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
                             IN ULONG DeviceListSize OPTIONAL,
                             OUT PBOOLEAN ConflictDetected)
{
    PCM_RESOURCE_LIST ResourceList;
    NTSTATUS Status;

    *ConflictDetected = FALSE;

    if (!DriverList && !DeviceList)
        return STATUS_INVALID_PARAMETER;

    /* Find the real list */
    if (!DriverList)
        ResourceList = DeviceList;
    else
        ResourceList = DriverList;

    /* Look for a resource conflict */
    Status = IopDetectResourceConflict(ResourceList, TRUE, NULL);
    if (Status == STATUS_CONFLICTING_ADDRESSES)
    {
        /* Oh noes */
        *ConflictDetected = TRUE;
    }
    else if (NT_SUCCESS(Status))
    {
        /* Looks like we're good to go */

        /* TODO: Claim the resources in the ResourceMap */
    }

    return Status;
}

VOID
NTAPI
IopSetEvent(IN PVOID Context)
{
    PKEVENT Event = Context;

    /* Set the event */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChange(IN PDEVICE_OBJECT PhysicalDeviceObject,
                           IN PVOID NotificationStructure)
{
    KEVENT NotifyEvent;
    NTSTATUS Status, NotifyStatus;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    /* Check for valid PDO */
    if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
    {
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)PhysicalDeviceObject, 0, 0);
    }

    /* FileObject must be null. PnP will fill in it */
    ASSERT(notifyStruct->FileObject == NULL);

    /* Do not handle system PnP events */
    if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Version != 1)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Initialize even that will let us know when PnP will have finished notify */
    KeInitializeEvent(&NotifyEvent, NotificationEvent, FALSE);

    Status = PpSetCustomTargetEvent(PhysicalDeviceObject, &NotifyEvent, &NotifyStatus, NULL, NULL, notifyStruct);
    /* If no error, wait for the notify to end and return the status of the notify and not of the event */
    if (NT_SUCCESS(Status))
    {
        KeWaitForSingleObject(&NotifyEvent, Executive, KernelMode, FALSE, NULL);
        Status = NotifyStatus;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(IN PDEVICE_OBJECT PhysicalDeviceObject,
                                       IN PVOID NotificationStructure,
                                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                                       IN PVOID Context OPTIONAL)
{
    PINTERNAL_WORK_QUEUE_ITEM Item = NULL;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    /* Check for valid PDO */
    if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
    {
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)PhysicalDeviceObject, 0, 0);
    }

    /* FileObject must be null. PnP will fill in it */
    ASSERT(notifyStruct->FileObject == NULL);

    /* Do not handle system PnP events */
    if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
        (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Version != 1)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* We need to store all the data given by the caller with the WorkItem, so use our own struct */
    Item = ExAllocatePoolWithTag(NonPagedPool, sizeof(INTERNAL_WORK_QUEUE_ITEM), '  pP');
    if (!Item) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize all stuff */
    ObReferenceObject(PhysicalDeviceObject);
    Item->NotificationStructure = notifyStruct;
    Item->PhysicalDeviceObject = PhysicalDeviceObject;
    Item->Callback = Callback;
    Item->Context = Context;
    ExInitializeWorkItem(&(Item->WorkItem), IopReportTargetDeviceChangeAsyncWorker, Item);

    /* Finally, queue the item, our work here is done */
    ExQueueWorkItem(&(Item->WorkItem), DelayedWorkQueue);

    return STATUS_PENDING;
}
