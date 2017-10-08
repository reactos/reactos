/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/fdo.c
 * PURPOSE:         FDO Device Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

SINGLE_LIST_ENTRY PciFdoExtensionListHead;
BOOLEAN PciBreakOnDefault;

PCI_MN_DISPATCH_TABLE PciFdoDispatchPowerTable[] =
{
    {IRP_DISPATCH, (PCI_DISPATCH_FUNCTION)PciFdoWaitWake},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoSetPowerState},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryPower},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported}
};

PCI_MN_DISPATCH_TABLE PciFdoDispatchPnpTable[] =
{
    {IRP_UPWARD,   (PCI_DISPATCH_FUNCTION)PciFdoIrpStartDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryRemoveDevice},
    {IRP_DISPATCH, (PCI_DISPATCH_FUNCTION)PciFdoIrpRemoveDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpCancelRemoveDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpStopDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryStopDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpCancelStopDevice},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryDeviceRelations},
    {IRP_DISPATCH, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryInterface},
    {IRP_UPWARD,   (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryCapabilities},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_UPWARD,   (PCI_DISPATCH_FUNCTION)PciFdoIrpDeviceUsageNotification},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpSurpriseRemoval},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciFdoIrpQueryLegacyBusInformation},
    {IRP_DOWNWARD, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported}
};

PCI_MJ_DISPATCH_TABLE PciFdoDispatchTable =
{
    IRP_MN_QUERY_LEGACY_BUS_INFORMATION,
    PciFdoDispatchPnpTable,
    IRP_MN_QUERY_POWER,
    PciFdoDispatchPowerTable,
    IRP_DOWNWARD,
    (PCI_DISPATCH_FUNCTION)PciIrpNotSupported,
    IRP_DOWNWARD,
    (PCI_DISPATCH_FUNCTION)PciIrpNotSupported
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciFdoIrpStartDevice(IN PIRP Irp,
                     IN PIO_STACK_LOCATION IoStackLocation,
                     IN PPCI_FDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PCM_RESOURCE_LIST Resources;
    PAGED_CODE();

    /* The device stack must be starting the FDO in a success path */
    if (!NT_SUCCESS(Irp->IoStatus.Status)) return STATUS_NOT_SUPPORTED;

    /* Attempt to switch the state machine to the started state */
    Status = PciBeginStateTransition(DeviceExtension, PciStarted);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check for any boot-provided resources */
    Resources = IoStackLocation->Parameters.StartDevice.AllocatedResources;
    if ((Resources) && !(PCI_IS_ROOT_FDO(DeviceExtension)))
    {
        /* These resources would only be for non-root FDOs, unhandled for now */
        ASSERT(Resources->Count == 1);
        UNIMPLEMENTED_DBGBREAK();
    }

    /* Initialize the arbiter for this FDO */
    Status = PciInitializeArbiterRanges(DeviceExtension, Resources);
    if (!NT_SUCCESS(Status))
    {
        /* Cancel the transition if this failed */
        PciCancelStateTransition(DeviceExtension, PciStarted);
        return Status;
    }

    /* Again, check for boot-provided resources for non-root FDO */
    if ((Resources) && !(PCI_IS_ROOT_FDO(DeviceExtension)))
    {
        /* Unhandled for now */
        ASSERT(Resources->Count == 1);
        UNIMPLEMENTED_DBGBREAK();
    }

    /* Commit the transition to the started state */
    PciCommitStateTransition(DeviceExtension, PciStarted);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciFdoIrpQueryRemoveDevice(IN PIRP Irp,
                           IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpRemoveDevice(IN PIRP Irp,
                      IN PIO_STACK_LOCATION IoStackLocation,
                      IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpCancelRemoveDevice(IN PIRP Irp,
                            IN PIO_STACK_LOCATION IoStackLocation,
                            IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpStopDevice(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpQueryStopDevice(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpCancelStopDevice(IN PIRP Irp,
                          IN PIO_STACK_LOCATION IoStackLocation,
                          IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpQueryDeviceRelations(IN PIRP Irp,
                              IN PIO_STACK_LOCATION IoStackLocation,
                              IN PPCI_FDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Are bus relations being queried? */
    if (IoStackLocation->Parameters.QueryDeviceRelations.Type != BusRelations)
    {
        /* The FDO is a bus, so only bus relations can be obtained */
        Status = STATUS_NOT_SUPPORTED;
    }
    else
    {
        /* Scan the PCI bus and build the device relations for the caller */
        Status = PciQueryDeviceRelations(DeviceExtension,
                                         (PDEVICE_RELATIONS*)
                                         &Irp->IoStatus.Information);
    }

    /* Return the enumeration status back */
    return Status;
}

NTSTATUS
NTAPI
PciFdoIrpQueryInterface(IN PIRP Irp,
                        IN PIO_STACK_LOCATION IoStackLocation,
                        IN PPCI_FDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();
    ASSERT(DeviceExtension->ExtensionType == PciFdoExtensionType);

    /* Deleted extensions don't respond to IRPs */
    if (DeviceExtension->DeviceState == PciDeleted)
    {
        /* Hand it back to try to deal with it */
        return PciPassIrpFromFdoToPdo(DeviceExtension, Irp);
    }

    /* Query our driver for this interface */
    Status = PciQueryInterface(DeviceExtension,
                               IoStackLocation->Parameters.QueryInterface.
                               InterfaceType,
                               IoStackLocation->Parameters.QueryInterface.
                               Size,
                               IoStackLocation->Parameters.QueryInterface.
                               Version,
                               IoStackLocation->Parameters.QueryInterface.
                               InterfaceSpecificData,
                               IoStackLocation->Parameters.QueryInterface.
                               Interface,
                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* We found it, let the PDO handle it */
        Irp->IoStatus.Status = Status;
        return PciPassIrpFromFdoToPdo(DeviceExtension, Irp);
    }
    else if (Status == STATUS_NOT_SUPPORTED)
    {
        /* Otherwise, we can't handle it, let someone else down the stack try */
        Status = PciCallDownIrpStack(DeviceExtension, Irp);
        if (Status == STATUS_NOT_SUPPORTED)
        {
            /* They can't either, try a last-resort interface lookup */
            Status = PciQueryInterface(DeviceExtension,
                                       IoStackLocation->Parameters.QueryInterface.
                                       InterfaceType,
                                       IoStackLocation->Parameters.QueryInterface.
                                       Size,
                                       IoStackLocation->Parameters.QueryInterface.
                                       Version,
                                       IoStackLocation->Parameters.QueryInterface.
                                       InterfaceSpecificData,
                                       IoStackLocation->Parameters.QueryInterface.
                                       Interface,
                                       TRUE);
        }
    }

    /* Has anyone claimed this interface yet? */
    if (Status == STATUS_NOT_SUPPORTED)
    {
        /* No, return the original IRP status */
        Status = Irp->IoStatus.Status;
    }
    else
    {
        /* Yes, set the new IRP status */
        Irp->IoStatus.Status = Status;
    }

    /* Complete this IRP */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
PciFdoIrpQueryCapabilities(IN PIRP Irp,
                           IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_FDO_EXTENSION DeviceExtension)
{
    PDEVICE_CAPABILITIES Capabilities;
    PAGED_CODE();
    ASSERT_FDO(DeviceExtension);

    UNREFERENCED_PARAMETER(Irp);

    /* Get the capabilities */
    Capabilities = IoStackLocation->Parameters.DeviceCapabilities.Capabilities;

    /* Inherit wake levels and power mappings from the higher-up capabilities */
    DeviceExtension->PowerState.SystemWakeLevel = Capabilities->SystemWake;
    DeviceExtension->PowerState.DeviceWakeLevel = Capabilities->DeviceWake;
    RtlCopyMemory(DeviceExtension->PowerState.SystemStateMapping,
                  Capabilities->DeviceState,
                  sizeof(DeviceExtension->PowerState.SystemStateMapping));

    /* Dump the capabilities and return success */
    PciDebugDumpQueryCapabilities(Capabilities);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciFdoIrpDeviceUsageNotification(IN PIRP Irp,
                                 IN PIO_STACK_LOCATION IoStackLocation,
                                 IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpSurpriseRemoval(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciFdoIrpQueryLegacyBusInformation(IN PIRP Irp,
                                   IN PIO_STACK_LOCATION IoStackLocation,
                                   IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

VOID
NTAPI
PciGetHotPlugParameters(IN PPCI_FDO_EXTENSION FdoExtension)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ULONG Length;
    NTSTATUS Status;
    PAGED_CODE();

    /* We should receive 4 parameters, per the HPP specification */
    Length = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 4 * sizeof(ACPI_METHOD_ARGUMENT);

    /* Allocate the buffer to hold the parameters */
    OutputBuffer = ExAllocatePoolWithTag(PagedPool, Length, PCI_POOL_TAG);
    if (!OutputBuffer) return;

    /* Initialize the output and input buffers. The method is _HPP */
    RtlZeroMemory(OutputBuffer, Length);
    *(PULONG)InputBuffer.MethodName = 'PPH_';
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    do
    {
        /* Send the IOCTL to the ACPI driver */
        Status = PciSendIoctl(FdoExtension->PhysicalDeviceObject,
                              IOCTL_ACPI_EVAL_METHOD,
                              &InputBuffer,
                              sizeof(InputBuffer),
                              OutputBuffer,
                              Length);
        if (!NT_SUCCESS(Status))
        {
            /* The method failed, check if we can salvage data from parent */
            if (!PCI_IS_ROOT_FDO(FdoExtension))
            {
                /* Copy the root bus' hot plug parameters */
                FdoExtension->HotPlugParameters = FdoExtension->ParentFdoExtension->HotPlugParameters;
            }

            /* Nothing more to do on this path */
            break;
        }

        /* ACPI sent back some data. 4 parameters are expected in the output */
        if (OutputBuffer->Count != 4) break;

        /* HotPlug PCI Support not yet implemented */
        UNIMPLEMENTED_DBGBREAK();
    } while (FALSE);

    /* Free the buffer and return */
    ExFreePoolWithTag(OutputBuffer, 0);
}

VOID
NTAPI
PciInitializeFdoExtensionCommonFields(PPCI_FDO_EXTENSION FdoExtension,
                                      IN PDEVICE_OBJECT DeviceObject,
                                      IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    /* Initialize the extension */
    RtlZeroMemory(FdoExtension, sizeof(PCI_FDO_EXTENSION));

    /* Setup the common fields */
    FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    FdoExtension->FunctionalDeviceObject = DeviceObject;
    FdoExtension->ExtensionType = PciFdoExtensionType;
    FdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
    FdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    FdoExtension->IrpDispatchTable = &PciFdoDispatchTable;

    /* Initialize the extension locks */
    KeInitializeEvent(&FdoExtension->SecondaryExtLock, SynchronizationEvent, TRUE);
    KeInitializeEvent(&FdoExtension->ChildListLock, SynchronizationEvent, TRUE);

    /* Initialize the default state */
    PciInitializeState(FdoExtension);
}

NTSTATUS
NTAPI
PciAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PCM_RESOURCE_LIST Descriptor;
    PDEVICE_OBJECT AttachedTo;
    PPCI_FDO_EXTENSION FdoExtension;
    PPCI_FDO_EXTENSION ParentExtension;
    PPCI_PDO_EXTENSION PdoExtension;
    PDEVICE_OBJECT DeviceObject;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
    ULONG ResultLength;
    PAGED_CODE();
    DPRINT1("PCI - AddDevice (a new bus). PDO: %p (Driver: %wZ)\n",
            PhysicalDeviceObject, &PhysicalDeviceObject->DriverObject->DriverName);

    /* Zero out variables so failure path knows what to do */
    AttachedTo = NULL;
    FdoExtension = NULL;
    PdoExtension = NULL;
    DeviceObject = NULL;

    do
    {
        /* Check if there's already a device extension for this bus */
        ParentExtension = PciFindParentPciFdoExtension(PhysicalDeviceObject,
                                                       &PciGlobalLock);
        if (ParentExtension)
        {
            /* Make sure we find a real PDO */
            PdoExtension = PhysicalDeviceObject->DeviceExtension;
            ASSERT_PDO(PdoExtension);

            /* Make sure it's a PCI-to-PCI bridge */
            if ((PdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
                (PdoExtension->SubClass != PCI_SUBCLASS_BR_PCI_TO_PCI))
            {
                /* This should never happen */
                DPRINT1("PCI - PciAddDevice for Non-Root/Non-PCI-PCI bridge,\n"
                        "      Class %02x, SubClass %02x, will not add.\n",
                        PdoExtension->BaseClass,
                        PdoExtension->SubClass);
                ASSERT((PdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                       (PdoExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI));

                /* Enter the failure path */
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            /* Subordinate bus on the bridge */
            DPRINT1("PCI - AddDevice (new bus is child of bus 0x%x).\n",
                    ParentExtension->BaseBus);

            /* Make sure PCI bus numbers are configured */
            if (!PciAreBusNumbersConfigured(PdoExtension))
            {
                /* This is a critical failure */
                DPRINT1("PCI - Bus numbers not configured for bridge (0x%x.0x%x.0x%x)\n",
                        ParentExtension->BaseBus,
                        PdoExtension->Slot.u.bits.DeviceNumber,
                        PdoExtension->Slot.u.bits.FunctionNumber);

                /* Enter the failure path */
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
        }

        /* Create the FDO for the bus */
        Status = IoCreateDevice(DriverObject,
                                sizeof(PCI_FDO_EXTENSION),
                                NULL,
                                FILE_DEVICE_BUS_EXTENDER,
                                0,
                                0,
                                &DeviceObject);
        if (!NT_SUCCESS(Status)) break;

        /* Initialize the extension for the FDO */
        FdoExtension = DeviceObject->DeviceExtension;
        PciInitializeFdoExtensionCommonFields(DeviceObject->DeviceExtension,
                                              DeviceObject,
                                              PhysicalDeviceObject);

        /* Attach to the root PDO */
        Status = STATUS_NO_SUCH_DEVICE;
        AttachedTo = IoAttachDeviceToDeviceStack(DeviceObject,
                                                 PhysicalDeviceObject);
        ASSERT(AttachedTo != NULL);
        if (!AttachedTo) break;
        FdoExtension->AttachedDeviceObject = AttachedTo;

        /* Check if this is a child bus, or the root */
        if (ParentExtension)
        {
            /* The child inherits root data */
            FdoExtension->BaseBus = PdoExtension->Dependent.type1.SecondaryBus;
            FdoExtension->BusRootFdoExtension = ParentExtension->BusRootFdoExtension;
            PdoExtension->BridgeFdoExtension = FdoExtension;
            FdoExtension->ParentFdoExtension = ParentExtension;
        }
        else
        {
            /* Query the boot configuration */
            Status = PciGetDeviceProperty(PhysicalDeviceObject,
                                          DevicePropertyBootConfiguration,
                                          (PVOID*)&Descriptor);
            if (!NT_SUCCESS(Status))
            {
                /* No configuration has been set */
                Descriptor = NULL;
            }
            else
            {
                /* Root PDO in ReactOS does not assign boot resources */
                UNIMPLEMENTED_DBGBREAK("Encountered during setup\n");
                Descriptor = NULL;
            }

            if (Descriptor)
            {
                /* Root PDO in ReactOS does not assign boot resources */
                UNIMPLEMENTED_DBGBREAK();
            }
            else
            {
                /* Default configuration isn't the normal path on Windows */
                if (PciBreakOnDefault)
                {
                    /* If a second bus is found and there's still no data, crash */
                    KeBugCheckEx(PCI_BUS_DRIVER_INTERNAL,
                                 0xDEAD0010u,
                                 (ULONG_PTR)DeviceObject,
                                 0,
                                 0);
                }

                /* Warn that a default configuration will be used, and set bus 0 */
                DPRINT1("PCI   Will use default configuration.\n");
                PciBreakOnDefault = TRUE;
                FdoExtension->BaseBus = 0;
            }

            /* This is the root bus */
            FdoExtension->BusRootFdoExtension = FdoExtension;
        }

        /* Get the HAL or ACPI Bus Handler Callbacks for Configuration Access */
        Status = PciGetConfigHandlers(FdoExtension);
        if (!NT_SUCCESS(Status)) break;

        /* Initialize all the supported PCI arbiters */
        Status = PciInitializeArbiters(FdoExtension);
        if (!NT_SUCCESS(Status)) break;

        /* This is a real FDO, insert it into the list */
        FdoExtension->Fake = FALSE;
        PciInsertEntryAtTail(&PciFdoExtensionListHead,
                             FdoExtension,
                             &PciGlobalLock);

        /* Open the device registry key so that we can query the errata flags */
        IoOpenDeviceRegistryKey(DeviceObject,
                                PLUGPLAY_REGKEY_DEVICE,
                                KEY_ALL_ACCESS,
                                &KeyHandle),

        /* Open the value that contains errata flags for this bus instance */
        RtlInitUnicodeString(&ValueName, L"HackFlags");
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(Buffer),
                                 &ResultLength);
        ZwClose(KeyHandle);
        if (NT_SUCCESS(Status))
        {
            /* Make sure the data is of expected type and size */
            if ((ValueInfo->Type == REG_DWORD) &&
                (ValueInfo->DataLength == sizeof(ULONG)))
            {
                /* Read the flags for this bus */
                FdoExtension->BusHackFlags = *(PULONG)&ValueInfo->Data;
            }
        }

        /* Query ACPI for PCI HotPlug Support */
        PciGetHotPlugParameters(FdoExtension);

        /* The Bus FDO is now initialized */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    } while (FALSE);

    /* This is the failure path */
    ASSERT(!NT_SUCCESS(Status));

    /* Check if the FDO extension exists */
    if (FdoExtension) DPRINT1("Should destroy secondaries\n");

    /* Delete device objects */
    if (AttachedTo) IoDetachDevice(AttachedTo);
    if (DeviceObject) IoDeleteDevice(DeviceObject);
    return Status;
}

/* EOF */
