/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpreport.c
 * PURPOSE:         Device Changes Reporting Functions
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IopCreateDeviceKeyPath(IN PCUNICODE_STRING RegistryPath,
                       OUT PHANDLE Handle);

NTSTATUS
IopAssignDeviceResources(
   IN PDEVICE_NODE DeviceNode,
   OUT ULONG *pRequiredSize);

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
                         PDEVICE_NODE DeviceNode);

NTSTATUS
IopTranslateDeviceResources(
   IN PDEVICE_NODE DeviceNode,
   IN ULONG RequiredSize);

NTSTATUS
IopActionInterrogateDeviceStack(PDEVICE_NODE DeviceNode,
                                PVOID Context);

NTSTATUS
IopUpdateResourceMapForPnPDevice(
   IN PDEVICE_NODE DeviceNode);

NTSTATUS
IopDetectResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList);

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

    /* Create the service name (eg. ACPI_HAL) */
    ServiceName.Buffer = DriverObject->DriverName.Buffer +
       sizeof(DRIVER_ROOT_NAME) / sizeof(WCHAR) - 1;
    ServiceName.Length = DriverObject->DriverName.Length -
       sizeof(DRIVER_ROOT_NAME) + sizeof(WCHAR);
    ServiceName.MaximumLength = ServiceName.Length;

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
        DeviceNode = IopGetDeviceNode(*DeviceObject);
    }
    else
    {
        /* Create the PDO */
        Status = PnpRootCreateDevice(&ServiceName,
                                     &Pdo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PnpRootCreateDevice() failed (Status 0x%08lx)\n", Status);
            return Status;
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
    }

    /* We don't call AddDevice for devices reported this way */
    IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);

    /* We don't send IRP_MN_START_DEVICE */
    IopDeviceNodeSetFlag(DeviceNode, DNF_STARTED);

    /* This is a legacy driver for this device */
    IopDeviceNodeSetFlag(DeviceNode, DNF_LEGACY_DRIVER);

    /* Perform a manual configuration of our device */
    IopActionInterrogateDeviceStack(DeviceNode, DeviceNode->Parent);
    IopActionConfigureChildServices(DeviceNode, DeviceNode->Parent);

    /* Open a handle to the instance path key */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, &InstanceKey);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add DETECTEDInterfaceType\DriverName */
    IdLength = 0;
    IdLength += swprintf(&HardwareId[IdLength],
                         L"DETECTED%ls\\%wZ",
                         IfString,
                         &ServiceName);
    HardwareId[IdLength++] = UNICODE_NULL;

    /* Add DETECTED\DriverName */
    IdLength += swprintf(&HardwareId[IdLength],
                         L"DETECTED\\%wZ",
                         &ServiceName);
    HardwareId[IdLength++] = UNICODE_NULL;

    /* Terminate the string with another null */
    HardwareId[IdLength] = UNICODE_NULL;

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
       HardwareId[++IdLength] = UNICODE_NULL;

       /* Write the value to the registry */
       Status = ZwSetValueKey(InstanceKey, &ValueName, 0, REG_SZ, HardwareId, IdLength * sizeof(WCHAR));
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

    if (DeviceNode->ResourceRequirements)
       IopDeviceNodeSetFlag(DeviceNode, DNF_RESOURCE_REPORTED);
    else
       IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);

    /* Write the resource information to the registry */
    IopSetDeviceInstanceData(InstanceKey, DeviceNode);

    /* Close the instance key handle */
    ZwClose(InstanceKey);

    /* If the caller didn't get the resources assigned for us, do it now */
    if (!ResourceAssigned)
    {
       IopDeviceNodeSetFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);
       Status = IopAssignDeviceResources(DeviceNode, &RequiredLength);
       if (NT_SUCCESS(Status))
       {
          Status = IopTranslateDeviceResources(DeviceNode, RequiredLength);
          if (NT_SUCCESS(Status))
              Status = IopUpdateResourceMapForPnPDevice(DeviceNode);
       }
       IopDeviceNodeClearFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);

       /* See if we failed */
       if (!NT_SUCCESS(Status))
       {
           DPRINT("Assigning resources failed: 0x%x\n", Status);
           return Status;
       }
    }

    /* Report the device's enumeration to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                              &DeviceNode->InstancePath);

    /* Report the device's arrival to umpnpmgr */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                              &DeviceNode->InstancePath);

    DPRINT1("Reported device: %S (%wZ)\n", HardwareId, &DeviceNode->InstancePath);

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
    Status = IopDetectResourceConflict(ResourceList);
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
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChange(IN PDEVICE_OBJECT PhysicalDeviceObject,
                           IN PVOID NotificationStructure)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(IN PDEVICE_OBJECT PhysicalDeviceObject,
                                       IN PVOID NotificationStructure,
                                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                                       IN PVOID Context OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
