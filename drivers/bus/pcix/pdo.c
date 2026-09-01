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

/**
 * @brief
 * Checks the uses of a function that keep it from being stopped or removed.
 *
 * @param[in] DeviceExtension
 * The PDO extension of the function.
 *
 * @return
 * STATUS_SUCCESS if the function may be given up.
 */
static
NTSTATUS
NTAPI
PciPdoValidateRelease(
    _In_ PPCI_PDO_EXTENSION DeviceExtension)
{
    /* Paging, hibernation and dump files, and the debugger, cannot lose their hardware */
    if ((DeviceExtension->PowerState.Paging) ||
        (DeviceExtension->PowerState.Hibernate) ||
        (DeviceExtension->PowerState.CrashDump) ||
        (DeviceExtension->OnDebugPath))
    {
        return STATUS_DEVICE_BUSY;
    }

    /* A driver that claimed the function outside of PnP will not let go of it */
    if (DeviceExtension->LegacyDriver)
        return STATUS_INVALID_DEVICE_REQUEST;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Checks whether a function may stop decoding for as long as it is stopped or removed.
 *
 * @param[in] DeviceExtension
 * The PDO extension of the function.
 *
 * @return
 * TRUE if the decodes and interrupts of the function can be turned off.
 */
static
BOOLEAN
NTAPI
PciPdoCanTurnOff(
    _In_ PPCI_PDO_EXTENSION DeviceExtension)
{
    /* Critical functions, VGA, legacy bridges and the debug path keep decoding */
    return PciCanDisableDecodes(DeviceExtension, NULL, 0, FALSE);
}

/**
 * @brief
 * Reads the identity back from the slot to learn whether the function is still there.
 *
 * @param[in,out] DeviceExtension
 * The PDO extension of the function. Marked not present if the identity differs.
 *
 * @return
 * TRUE if the same function still answers in the slot.
 */
static
BOOLEAN
NTAPI
PciPdoIsPresent(
    _Inout_ PPCI_PDO_EXTENSION DeviceExtension)
{
    PCI_COMMON_HEADER PciData;

    if (DeviceExtension->NotPresent)
        return FALSE;

    PciReadDeviceConfig(DeviceExtension, &PciData, 0, PCI_COMMON_HDR_LENGTH);
    if (!PcipIsSameDevice(DeviceExtension, &PciData))
    {
        DPRINT1("PCI (pdox %p) no longer answers in its slot\n", DeviceExtension);
        DeviceExtension->NotPresent = TRUE;
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Stops a function from decoding, mastering and raising interrupts, and
 * optionally moves it to D3.
 *
 * @param[in,out] DeviceExtension
 * The PDO extension of the function.
 *
 * @param[in] PowerDown
 * TRUE to also power the function down.
 */
static
VOID
NTAPI
PciPdoTurnOff(
    _Inout_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ BOOLEAN PowerDown)
{
    POWER_STATE PowerState;
    USHORT Command;

    if (!PciPdoCanTurnOff(DeviceExtension))
        return;

    /* The wired line is masked in the same write that clears the decodes */
    PciReadDeviceConfig(DeviceExtension,
                        &Command,
                        FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                        sizeof(Command));
    Command |= PCI_DISABLE_LEVEL_INTERRUPT;
    PciDecodeEnable(DeviceExtension, FALSE, &Command);

    if ((!PowerDown) ||
        (DeviceExtension->PowerState.CurrentDeviceState == PowerDeviceD3) ||
        !(PciCanDisableDecodes(DeviceExtension, NULL, 0, TRUE)))
    {
        return;
    }

    /* Start sees D3, powers the function back up and reprograms it */
    PciSetPowerManagedDevicePowerState(DeviceExtension, PowerDeviceD3, FALSE);
    DeviceExtension->PowerState.CurrentDeviceState = PowerDeviceD3;

    PowerState.DeviceState = PowerDeviceD3;
    PoSetPowerState(DeviceExtension->PhysicalDeviceObject, DevicePowerState, PowerState);
}

NTSTATUS
NTAPI
PciPdoIrpQueryRemoveDevice(IN PIRP Irp,
                           IN PIO_STACK_LOCATION IoStackLocation,
                           IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    if (DeviceExtension->HackFlags & PCI_HACK_FAIL_QUERY_REMOVE)
        return STATUS_DEVICE_BUSY;

    Status = PciPdoValidateRelease(DeviceExtension);
    if (!NT_SUCCESS(Status))
        return Status;

    /* A function that was never started has no state to leave */
    if (DeviceExtension->DeviceState == PciNotStarted)
        return STATUS_SUCCESS;

    return PciBeginStateTransition((PVOID)DeviceExtension, PciNotStarted);
}

NTSTATUS
NTAPI
PciPdoIrpRemoveDevice(IN PIRP Irp,
                      IN PIO_STACK_LOCATION IoStackLocation,
                      IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    if (PciPdoIsPresent(DeviceExtension))
        PciPdoTurnOff(DeviceExtension, TRUE);

    /* A remove without a query still has to take a started function out of that state */
    if ((DeviceExtension->DeviceState == PciStarted) &&
        (DeviceExtension->TentativeNextState == PciStarted))
    {
        PciBeginStateTransition((PVOID)DeviceExtension, PciNotStarted);
    }

    if ((DeviceExtension->DeviceState != PciNotStarted) &&
        (DeviceExtension->TentativeNextState == PciNotStarted))
    {
        PciCommitStateTransition((PVOID)DeviceExtension, PciNotStarted);
    }

    /* A function that left the bus will not come back through this PDO */
    if ((DeviceExtension->ReportedMissing) &&
        (NT_SUCCESS(PciBeginStateTransition((PVOID)DeviceExtension, PciDeleted))))
    {
        PciCommitStateTransition((PVOID)DeviceExtension, PciDeleted);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpCancelRemoveDevice(IN PIRP Irp,
                            IN PIO_STACK_LOCATION IoStackLocation,
                            IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    /* The remove is off, so the device stays where it was */
    PciCancelStateTransition((PVOID)DeviceExtension, PciNotStarted);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpStopDevice(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    /* The device is losing its resources, so it must stop using them first */
    PciPdoTurnOff(DeviceExtension, FALSE);

    /* The query stop that has to come first already began the transition */
    PciCommitStateTransition((PVOID)DeviceExtension, PciStopped);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpQueryStopDevice(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    Status = PciPdoValidateRelease(DeviceExtension);
    if (!NT_SUCCESS(Status))
        return Status;

    /* New resources are useless if the old ranges keep decoding */
    if (!PciPdoCanTurnOff(DeviceExtension))
        return STATUS_INVALID_DEVICE_REQUEST;

    return PciBeginStateTransition((PVOID)DeviceExtension, PciStopped);
}

NTSTATUS
NTAPI
PciPdoIrpCancelStopDevice(IN PIRP Irp,
                          IN PIO_STACK_LOCATION IoStackLocation,
                          IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    /* The stop is off, so the device keeps what it has */
    PciCancelStateTransition((PVOID)DeviceExtension, PciStopped);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpQueryInterface(IN PIRP Irp,
                        IN PIO_STACK_LOCATION IoStackLocation,
                        IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);

    /* Query the interface from the PCI driver */
    Status = PciQueryInterface(
                (PPCI_FDO_EXTENSION)DeviceExtension,
                IoStackLocation->Parameters.QueryInterface.InterfaceType,
                IoStackLocation->Parameters.QueryInterface.Size,
                IoStackLocation->Parameters.QueryInterface.Version,
                IoStackLocation->Parameters.QueryInterface.InterfaceSpecificData,
                IoStackLocation->Parameters.QueryInterface.Interface,
                FALSE);

    /* TODO: There's more to this story, but NOT TODAY! */

    return Status;
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
    PNP_DEVICE_STATE State;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IoStackLocation);

    /* Whatever the rest of the stack decided about this device still holds */
    State = (PNP_DEVICE_STATE)Irp->IoStatus.Information;

    /* Host bridges, and anything a query remove would refuse, must not be offered for disabling */
    if (((DeviceExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
         (DeviceExtension->SubClass == PCI_SUBCLASS_BR_HOST)) ||
        (!NT_SUCCESS(PciPdoValidateRelease(DeviceExtension))))
    {
        State |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    Irp->IoStatus.Information = State;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpDeviceUsageNotification(IN PIRP Irp,
                                 IN PIO_STACK_LOCATION IoStackLocation,
                                 IN PPCI_PDO_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);

    /* Every bus up to the root has to hold on to the path, so it goes up first */
    Status = PciSendDeviceUsageToParent(DeviceExtension->ParentFdoExtension,
                                        IoStackLocation);
    if (!NT_SUCCESS(Status))
        return Status;

    return PciUpdateDeviceUsage(&DeviceExtension->PowerState, IoStackLocation);
}

NTSTATUS
NTAPI
PciPdoIrpSurpriseRemoval(IN PIRP Irp,
                         IN PIO_STACK_LOCATION IoStackLocation,
                         IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);

    /* This also comes after a failed start, with the function still in its slot */
    if (PciPdoIsPresent(DeviceExtension))
        PciPdoTurnOff(DeviceExtension, TRUE);

    if (DeviceExtension->ReportedMissing)
    {
        /* The slot is empty, so only the remove is left to come */
        if (NT_SUCCESS(PciBeginStateTransition((PVOID)DeviceExtension, PciSurpriseRemoved)))
            PciCommitStateTransition((PVOID)DeviceExtension, PciSurpriseRemoved);
    }
    else if (DeviceExtension->DeviceState != PciNotStarted)
    {
        /* The remove that follows commits this */
        PciBeginStateTransition((PVOID)DeviceExtension, PciNotStarted);
    }

    return STATUS_SUCCESS;
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
    _swprintf(DeviceName, L"\\Device\\NTPNP_PCI%04d", SequenceNumber);
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
    PdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
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
