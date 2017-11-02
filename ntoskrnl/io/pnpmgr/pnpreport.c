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
NTAPI
IopCreateDeviceKeyPath(IN PCUNICODE_STRING RegistryPath,
                       IN ULONG CreateOptions,
                       OUT PHANDLE Handle);

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
                         PDEVICE_NODE DeviceNode);

NTSTATUS
IopActionInterrogateDeviceStack(PDEVICE_NODE DeviceNode,
                                PVOID Context);

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
    IopNotifyPlugPlayNotification(DeviceObject,
                                  EventCategoryTargetDeviceChange,
                                  &GUID_PNP_CUSTOM_NOTIFICATION,
                                  NotificationStructure,
                                  NULL);

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
IoReportDetectedDevice(IN PDRIVER_OBJECT DriverObject,
                       IN INTERFACE_TYPE LegacyBusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN PCM_RESOURCE_LIST ResourceList,
                       IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
                       IN BOOLEAN ResourceAssigned,
                       IN OUT PDEVICE_OBJECT *DeviceObject OPTIONAL)
{
    PDEVICE_NODE DeviceNode;
    PDEVICE_OBJECT Pdo;
    NTSTATUS Status;
    HANDLE InstanceKey;
    ULONG RequiredLength;
    UNICODE_STRING ValueName, ServiceName;
    WCHAR HardwareId[256];
    PWCHAR IfString;
    ULONG IdLength;

    DPRINT("IoReportDetectedDevice (DeviceObject %p, *DeviceObject %p)\n",
      DeviceObject, DeviceObject ? *DeviceObject : NULL);

    ServiceName = DriverObject->DriverExtension->ServiceKeyName;

    /* If the interface type is unknown, treat it as internal */
    if (LegacyBusType == InterfaceTypeUndefined)
        LegacyBusType = Internal;

    /* Get the string equivalent of the interface type */
    IfString = IopGetInterfaceTypeString(LegacyBusType);

    /* If NULL is returned then it's a bad type */
    if (!IfString)
        return STATUS_INVALID_PARAMETER;

    /* We use the caller's PDO if they supplied one */
    if (DeviceObject && *DeviceObject)
    {
        Pdo = *DeviceObject;
    }
    else
    {
        /* Create the PDO */
        Status = PnpRootCreateDevice(&ServiceName,
                                     NULL,
                                     &Pdo,
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PnpRootCreateDevice() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    /* Create the device node for the new PDO */
    Status = IopCreateDeviceNode(IopRootDeviceNode,
                                 Pdo,
                                 NULL,
                                 &DeviceNode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopCreateDeviceNode() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* We're enumerated already */
    IopDeviceNodeSetFlag(DeviceNode, DNF_ENUMERATED);

    /* We don't call AddDevice for devices reported this way */
    IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);

    /* We don't send IRP_MN_START_DEVICE */
    IopDeviceNodeSetFlag(DeviceNode, DNF_STARTED);

    /* We need to get device IDs */
#if 0
    IopDeviceNodeSetFlag(DeviceNode, DNF_NEED_QUERY_IDS);
#endif

    /* This is a legacy driver for this device */
    IopDeviceNodeSetFlag(DeviceNode, DNF_LEGACY_DRIVER);

    /* Perform a manual configuration of our device */
    IopActionInterrogateDeviceStack(DeviceNode, DeviceNode->Parent);
    IopActionConfigureChildServices(DeviceNode, DeviceNode->Parent);

    /* Open a handle to the instance path key */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (!NT_SUCCESS(Status))
        return Status;

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

    /* Add a hardware ID if the driver didn't report one */
    RtlInitUnicodeString(&ValueName, L"HardwareID");
    if (ZwQueryValueKey(InstanceKey, &ValueName, KeyValueBasicInformation, NULL, 0, &RequiredLength) == STATUS_OBJECT_NAME_NOT_FOUND)
    {
       /* Just use our most specific compatible ID */
       IdLength = 0;
       IdLength += swprintf(&HardwareId[IdLength],
                            L"DETECTED%ls\\%wZ",
                            IfString,
                            &ServiceName);
       IdLength++;

       HardwareId[IdLength++] = UNICODE_NULL;

       /* Write the value to the registry */
       Status = ZwSetValueKey(InstanceKey, &ValueName, 0, REG_MULTI_SZ, HardwareId, IdLength * sizeof(WCHAR));
       if (!NT_SUCCESS(Status))
       {
          DPRINT("Failed to write the hardware ID: 0x%x\n", Status);
          ZwClose(InstanceKey);
          return Status;
       }
    }

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

    /* Report the device's enumeration to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                              &DeviceNode->InstancePath);

    /* Report the device's arrival to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                              &DeviceNode->InstancePath);

    DPRINT("Reported device: %S (%wZ)\n", HardwareId, &DeviceNode->InstancePath);

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
    Status = IopDetectResourceConflict(ResourceList, FALSE, NULL);
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
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG)PhysicalDeviceObject, 0, 0);
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
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG)PhysicalDeviceObject, 0, 0);
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
