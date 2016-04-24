/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pdo.c
 * PURPOSE:         PDO Device Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LONG PciPdoSequenceNumber;

C_ASSERT(FIELD_OFFSET(PCI_FDO_EXTENSION, DeviceState) == FIELD_OFFSET(PCI_PDO_EXTENSION, DeviceState));
C_ASSERT(FIELD_OFFSET(PCI_FDO_EXTENSION, TentativeNextState) == FIELD_OFFSET(PCI_PDO_EXTENSION, TentativeNextState));
C_ASSERT(FIELD_OFFSET(PCI_FDO_EXTENSION, List) == FIELD_OFFSET(PCI_PDO_EXTENSION, Next));

PCI_MN_DISPATCH_TABLE PciPdoDispatchPowerTable[] =
{
    {IRP_DISPATCH, (PCI_DISPATCH_FUNCTION)PciPdoWaitWake},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoSetPowerState},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryPower},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported}
};

PCI_MN_DISPATCH_TABLE PciPdoDispatchPnpTable[] =
{
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpStartDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryRemoveDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpRemoveDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpCancelRemoveDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpStopDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryStopDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpCancelStopDevice},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryDeviceRelations},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryInterface},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryCapabilities},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryResources},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryResourceRequirements},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryDeviceText},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpReadConfig},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpWriteConfig},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryId},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryDeviceState},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryBusInformation},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpDeviceUsageNotification},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpSurpriseRemoval},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciPdoIrpQueryLegacyBusInformation},
    {IRP_COMPLETE, (PCI_DISPATCH_FUNCTION)PciIrpNotSupported}
};

PCI_MJ_DISPATCH_TABLE PciPdoDispatchTable =
{
    IRP_MN_QUERY_LEGACY_BUS_INFORMATION,
    PciPdoDispatchPnpTable,
    IRP_MN_QUERY_POWER,
    PciPdoDispatchPowerTable,
    IRP_COMPLETE,
    (PCI_DISPATCH_FUNCTION)PciIrpNotSupported,
    IRP_COMPLETE,
    (PCI_DISPATCH_FUNCTION)PciIrpInvalidDeviceRequest
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciPdoWaitWake(IN PIRP Irp,
               IN PIO_STACK_LOCATION IoStackLocation,
               IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoSetPowerState(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryPower(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpStartDevice(IN PIRP Irp,
                     IN PIO_STACK_LOCATION IoStackLocation,
                     IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    BOOLEAN Changed, DoReset;
    POWER_STATE PowerState;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);

    DoReset = FALSE;

    /* Begin entering the start phase */
    Status = PciBeginStateTransition((PVOID)DeviceExtension, PciStarted);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is a VGA device */
    if (((DeviceExtension->BaseClass == PCI_CLASS_PRE_20) &&
         (DeviceExtension->SubClass == PCI_SUBCLASS_PRE_20_VGA)) ||
        ((DeviceExtension->BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
         (DeviceExtension->SubClass == PCI_SUBCLASS_VID_VGA_CTLR)))
    {
        /* Always force it on */
        DeviceExtension->CommandEnables |= (PCI_ENABLE_IO_SPACE |
                                            PCI_ENABLE_MEMORY_SPACE);
    }

    /* Check if native IDE is enabled and it owns the I/O ports */
    if (DeviceExtension->IoSpaceUnderNativeIdeControl)
    {
        /* Then don't allow I/O access */
        DeviceExtension->CommandEnables &= ~PCI_ENABLE_IO_SPACE;
    }

    /* Always enable bus mastering */
    DeviceExtension->CommandEnables |= PCI_ENABLE_BUS_MASTER;

    /* Check if the OS assigned resources differ from the PCI configuration */
    Changed = PciComputeNewCurrentSettings(DeviceExtension,
                                           IoStackLocation->Parameters.
                                           StartDevice.AllocatedResources);
    if (Changed)
    {
        /* Remember this for later */
        DeviceExtension->MovedDevice = TRUE;
    }
    else
    {
        /* All good */
        DPRINT1("PCI - START not changing resource settings.\n");
    }

    /* Check if the device was sleeping */
    if (DeviceExtension->PowerState.CurrentDeviceState != PowerDeviceD0)
    {
        /* Power it up */
        Status = PciSetPowerManagedDevicePowerState(DeviceExtension,
                                                    PowerDeviceD0,
                                                    FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Powerup fail, fail the request */
            PciCancelStateTransition((PVOID)DeviceExtension, PciStarted);
            return STATUS_DEVICE_POWER_FAILURE;
        }

        /* Tell the power manager that the device is powered up */
        PowerState.DeviceState = PowerDeviceD0;
        PoSetPowerState(DeviceExtension->PhysicalDeviceObject,
                        DevicePowerState,
                        PowerState);

        /* Update internal state */
        DeviceExtension->PowerState.CurrentDeviceState = PowerDeviceD0;

        /* This device's resources and decodes will need to be reset */
        DoReset = TRUE;
    }

    /* Update resource information now that the device is powered up and active */
    Status = PciSetResources(DeviceExtension, DoReset, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* That failed, so cancel the transition */
        PciCancelStateTransition((PVOID)DeviceExtension, PciStarted);
    }
    else
    {
        /* Fully commit, as the device is now started up and ready to go */
        PciCommitStateTransition((PVOID)DeviceExtension, PciStarted);
    }

    /* Return the result of the start request */
    return Status;
}

NTSTATUS
NTAPI
PciPdoIrpQueryRemoveDevice(IN PIRP Irp,
                           IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpRemoveDevice(IN PIRP Irp,
                      IN PIO_STACK_LOCATION IoStackLocation,
                      IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpCancelRemoveDevice(IN PIRP Irp,
                            IN PIO_STACK_LOCATION IoStackLocation,
                            IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpStopDevice(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryStopDevice(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpCancelStopDevice(IN PIRP Irp,
                          IN PIO_STACK_LOCATION IoStackLocation,
                          IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryInterface(IN PIRP Irp,
                        IN PIO_STACK_LOCATION IoStackLocation,
                        IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceRelations(IN PIRP Irp,
                              IN PIO_STACK_LOCATION IoStackLocation,
                              IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Are ejection relations being queried? */
    if (IoStackLocation->Parameters.QueryDeviceRelations.Type == EjectionRelations)
    {
        /* Call the worker function */
        Status = PciQueryEjectionRelations(DeviceExtension,
                                           (PDEVICE_RELATIONS*)&Irp->
                                           IoStatus.Information);
    }
    else if (IoStackLocation->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
    {
        /* The only other relation supported is the target device relation */
        Status = PciQueryTargetDeviceRelations(DeviceExtension,
                                               (PDEVICE_RELATIONS*)&Irp->
                                               IoStatus.Information);
    }
    else
    {
        /* All other relations are unsupported */
        Status = STATUS_NOT_SUPPORTED;
    }

    /* Return either the result of the worker function, or unsupported status */
    return Status;
}

NTSTATUS
NTAPI
PciPdoIrpQueryCapabilities(IN PIRP Irp,
                           IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);

    /* Call the worker function */
    return PciQueryCapabilities(DeviceExtension,
                                IoStackLocation->
                                Parameters.DeviceCapabilities.Capabilities);
}

NTSTATUS
NTAPI
PciPdoIrpQueryResources(IN PIRP Irp,
                        IN PIO_STACK_LOCATION IoStackLocation,
                        IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IoStackLocation);

    /* Call the worker function */
    return PciQueryResources(DeviceExtension,
                            (PCM_RESOURCE_LIST*)&Irp->IoStatus.Information);
}

NTSTATUS
NTAPI
PciPdoIrpQueryResourceRequirements(IN PIRP Irp,
                                   IN PIO_STACK_LOCATION IoStackLocation,
                                   IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IoStackLocation);

    /* Call the worker function */
    return PciQueryRequirements(DeviceExtension,
                                (PIO_RESOURCE_REQUIREMENTS_LIST*)&Irp->
                                IoStatus.Information);
}

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceText(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    /* Call the worker function */
    return PciQueryDeviceText(DeviceExtension,
                              IoStackLocation->
                              Parameters.QueryDeviceText.DeviceTextType,
                              IoStackLocation->
                              Parameters.QueryDeviceText.LocaleId,
                              (PWCHAR*)&Irp->IoStatus.Information);
}

NTSTATUS
NTAPI
PciPdoIrpQueryId(IN PIRP Irp,
                 IN PIO_STACK_LOCATION IoStackLocation,
                 IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    /* Call the worker function */
    return PciQueryId(DeviceExtension,
                      IoStackLocation->Parameters.QueryId.IdType,
                      (PWCHAR*)&Irp->IoStatus.Information);
}

NTSTATUS
NTAPI
PciPdoIrpQueryBusInformation(IN PIRP Irp,
                             IN PIO_STACK_LOCATION IoStackLocation,
                             IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IoStackLocation);

    /* Call the worker function */
    return PciQueryBusInformation(DeviceExtension,
                                  (PPNP_BUS_INFORMATION*)&Irp->
                                  IoStatus.Information);
}

NTSTATUS
NTAPI
PciPdoIrpReadConfig(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpWriteConfig(IN PIRP Irp,
                     IN PIO_STACK_LOCATION IoStackLocation,
                     IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryDeviceState(IN PIRP Irp,
                          IN PIO_STACK_LOCATION IoStackLocation,
                          IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpDeviceUsageNotification(IN PIRP Irp,
                                 IN PIO_STACK_LOCATION IoStackLocation,
                                 IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpSurpriseRemoval(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoIrpQueryLegacyBusInformation(IN PIRP Irp,
                                   IN PIO_STACK_LOCATION IoStackLocation,
                                   IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoCreate(IN PPCI_FDO_EXTENSION DeviceExtension,
             IN PCI_SLOT_NUMBER Slot,
             OUT PDEVICE_OBJECT *PdoDeviceObject)
{
    WCHAR DeviceName[32];
    UNICODE_STRING DeviceString;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PPCI_PDO_EXTENSION PdoExtension;
    ULONG SequenceNumber;
    PAGED_CODE();

    /* Pick an atomically unique sequence number for this device */
    SequenceNumber = InterlockedIncrement(&PciPdoSequenceNumber);

    /* Create the standard PCI device name for a PDO */
    swprintf(DeviceName, L"\\Device\\NTPNP_PCI%04d", SequenceNumber);
    RtlInitUnicodeString(&DeviceString, DeviceName);

    /* Create the actual device now */
    Status = IoCreateDevice(DeviceExtension->FunctionalDeviceObject->DriverObject,
                            sizeof(PCI_PDO_EXTENSION),
                            &DeviceString,
                            FILE_DEVICE_BUS_EXTENDER,
                            0,
                            0,
                            &DeviceObject);
    ASSERT(NT_SUCCESS(Status));

    /* Get the extension for it */
    PdoExtension = (PPCI_PDO_EXTENSION)DeviceObject->DeviceExtension;
    DPRINT1("PCI: New PDO (b=0x%x, d=0x%x, f=0x%x) @ %p, ext @ %p\n",
            DeviceExtension->BaseBus,
            Slot.u.bits.DeviceNumber,
            Slot.u.bits.FunctionNumber,
            DeviceObject,
            DeviceObject->DeviceExtension);

    /* Configure the extension */
    PdoExtension->ExtensionType = PciPdoExtensionType;
    PdoExtension->IrpDispatchTable = &PciPdoDispatchTable;
    PdoExtension->PhysicalDeviceObject = DeviceObject;
    PdoExtension->Slot = Slot;
    PdoExtension->PowerState.CurrentSystemState = PowerDeviceD0;
    PdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    PdoExtension->ParentFdoExtension = DeviceExtension;

    /* Initialize the lock for arbiters and other interfaces */
    KeInitializeEvent(&PdoExtension->SecondaryExtLock, SynchronizationEvent, TRUE);

    /* Initialize the state machine */
    PciInitializeState((PPCI_FDO_EXTENSION)PdoExtension);

    /* Add the PDO to the parent's list */
    PdoExtension->Next = NULL;
    PciInsertEntryAtTail((PSINGLE_LIST_ENTRY)&DeviceExtension->ChildPdoList,
                         (PPCI_FDO_EXTENSION)PdoExtension,
                         &DeviceExtension->ChildListLock);

    /* And finally return it to the caller */
    *PdoDeviceObject = DeviceObject;
    return STATUS_SUCCESS;
}

/* EOF */
