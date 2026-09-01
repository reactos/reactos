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

static
NTSTATUS
NTAPI
PciPdoEnterDevicePowerState(
    _Inout_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ DEVICE_POWER_STATE DeviceState);

static IO_WORKITEM_ROUTINE PciPdoDevicePowerWorker;

NTSTATUS
NTAPI
PciPdoWaitWake(IN PIRP Irp,
               IN PIO_STACK_LOCATION IoStackLocation,
               IN PPCI_PDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    /* PME is never armed, so the function cannot wake the system */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PciPdoSetPowerState(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    POWER_STATE State = IoStackLocation->Parameters.Power.State;
    PIO_WORKITEM WorkItem;

    /* The function driver follows a system state with a device state request of its own */
    if (IoStackLocation->Parameters.Power.Type == SystemPowerState)
    {
        DeviceExtension->PowerState.CurrentSystemState = State.SystemState;
        return STATUS_SUCCESS;
    }

    if ((State.DeviceState < PowerDeviceD0) || (State.DeviceState > PowerDeviceD3))
        return STATUS_INVALID_PARAMETER;

    if (State.DeviceState == DeviceExtension->PowerState.CurrentDeviceState)
        return STATUS_SUCCESS;

    /* Configuration cycles cannot reach a function on a bus that is not running */
    if (DeviceExtension->ParentFdoExtension->DeviceState != PciStarted)
        return STATUS_NO_SUCH_DEVICE;

    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        return PciPdoEnterDevicePowerState(DeviceExtension, State.DeviceState);

    /* D0 may be sent at DISPATCH_LEVEL, and reprogramming the function needs PASSIVE_LEVEL */
    WorkItem = IoAllocateWorkItem(DeviceExtension->PhysicalDeviceObject);
    if (!WorkItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->Tail.Overlay.DriverContext[0] = WorkItem;
    IoMarkIrpPending(Irp);
    IoQueueWorkItem(WorkItem, PciPdoDevicePowerWorker, DelayedWorkQueue, Irp);
    return STATUS_PENDING;
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

    /* Every device state is accepted when set, so none is refused here */
    return STATUS_SUCCESS;
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

/**
 * @brief
 * Checks whether a request for a low power device state may take power from a function.
 *
 * @param[in] DeviceExtension
 * The PDO extension of the function.
 *
 * @param[in] DeviceState
 * The low power state being entered.
 *
 * @return
 * FALSE if the function has to stay in D0 and keep decoding.
 */
static
BOOLEAN
NTAPI
PciPdoCanPowerDown(
    _In_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ DEVICE_POWER_STATE DeviceState)
{
    SYSTEM_POWER_STATE SystemState = DeviceExtension->PowerState.CurrentSystemState;

    /* Critical functions, VGA and the debug path keep decoding, so they keep power too */
    if (!PciPdoCanTurnOff(DeviceExtension))
        return FALSE;

    /* IDE controllers and legacy bridges stay in D0 */
    if (!PciCanDisableDecodes(DeviceExtension, NULL, 0, TRUE))
        return FALSE;

    /* The hibernation file and crash dump are written through this function */
    if ((SystemState == PowerSystemHibernate) &&
        ((DeviceExtension->PowerState.Hibernate) || (DeviceExtension->PowerState.CrashDump)))
    {
        return FALSE;
    }

    /* On a warm reboot firmware may not power a bridge up again before booting from behind it */
    if ((SystemState == PowerSystemShutdown) &&
        (DeviceState == PowerDeviceD3) &&
        (DeviceExtension->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
        (DeviceExtension->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Moves a function to a device power state and reports it to the power manager.
 *
 * @param[in,out] DeviceExtension
 * The PDO extension of the function.
 *
 * @param[in] DeviceState
 * The state to enter, D0 through D3.
 *
 * @return
 * STATUS_SUCCESS, or the error that kept the function from returning to D0.
 */
static
NTSTATUS
NTAPI
PciPdoEnterDevicePowerState(
    _Inout_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ DEVICE_POWER_STATE DeviceState)
{
    POWER_STATE PowerState;
    NTSTATUS Status;
    PAGED_CODE();

    if (DeviceState == DeviceExtension->PowerState.CurrentDeviceState)
        return STATUS_SUCCESS;

    PowerState.DeviceState = DeviceState;

    if (DeviceState == PowerDeviceD0)
    {
        /* After the settle delay this also writes the configuration and interrupt back */
        Status = PciSetPowerManagedDevicePowerState(DeviceExtension, PowerDeviceD0, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PCI (pdox %p) failed to return to D0 (0x%08lx)\n", DeviceExtension, Status);
            return Status;
        }

        DeviceExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
        PoSetPowerState(DeviceExtension->PhysicalDeviceObject, DevicePowerState, PowerState);
        return STATUS_SUCCESS;
    }

    /* The power manager learns of a power down before the function loses power */
    PoSetPowerState(DeviceExtension->PhysicalDeviceObject, DevicePowerState, PowerState);

    if (PciPdoCanPowerDown(DeviceExtension, DeviceState))
    {
        PciPdoTurnOff(DeviceExtension, FALSE);

        Status = PciSetPowerManagedDevicePowerState(DeviceExtension, DeviceState, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PCI (pdox %p) did not settle in device state %lu (0x%08lx)\n",
                    DeviceExtension,
                    DeviceState,
                    Status);
        }
    }

    /* A function kept running is still reprogrammed by the next D0 */
    DeviceExtension->PowerState.CurrentDeviceState = DeviceState;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Completes a device set power request that arrived at raised IRQL.
 *
 * @param[in] Context
 * The pending device power IRP.
 */
static
VOID
NTAPI
PciPdoDevicePowerWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PIRP Irp = Context;
    PIO_STACK_LOCATION IoStackLocation;
    DEVICE_POWER_STATE DeviceState;

    ASSERT(Irp != NULL);

    IoFreeWorkItem(Irp->Tail.Overlay.DriverContext[0]);

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceState = IoStackLocation->Parameters.Power.State.DeviceState;

    Irp->IoStatus.Status = PciPdoEnterDevicePowerState(DeviceObject->DeviceExtension,
                                                       DeviceState);
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
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

/**
 * @brief
 * Serves a driver's read or write of its own function's configuration space.
 *
 * @param[in] Read
 * TRUE to read configuration space, FALSE to write it.
 *
 * @return
 * STATUS_SUCCESS, or the error that kept the access from being made.
 */
static
NTSTATUS
NTAPI
PciPdoReadWriteConfig(
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _In_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ BOOLEAN Read)
{
    PUCHAR Bounce;
    ULONG Offset, Length, End, Limit, LineOffset;
    BOOLEAN CoversLine;
    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    /* No ROM image is kept, and a ROM can never be written */
    if (IoStackLocation->Parameters.ReadWriteConfig.WhichSpace == PCI_WHICHSPACE_ROM)
        return STATUS_INVALID_DEVICE_REQUEST;

    /* Any other space value is taken as configuration space */
    Offset = IoStackLocation->Parameters.ReadWriteConfig.Offset;
    Length = IoStackLocation->Parameters.ReadWriteConfig.Length;

    End = Offset + Length;
    if (End < Offset)
        return STATUS_INTEGER_OVERFLOW;

    Limit = DeviceExtension->IsExtendedConfigReachable ? PCI_EXTENDED_CONFIG_LENGTH :
                                                         PCI_LEGACY_CONFIG_LENGTH;
    if (End > Limit)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (!Length)
        return STATUS_SUCCESS;

    /* The caller's buffer may be pageable, and config cycles run at raised IRQL */
    Bounce = ExAllocatePoolWithTag(NonPagedPool, Length, PCI_POOL_TAG);
    if (!Bounce)
        return STATUS_INSUFFICIENT_RESOURCES;

    LineOffset = FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.InterruptLine);
    CoversLine = (DeviceExtension->InterruptPin != 0) &&
                 (LineOffset >= Offset) &&
                 (LineOffset < End);

    if (Read)
    {
        PciReadDeviceConfig(DeviceExtension, Bounce, Offset, Length);

        /* Drivers see the line they were assigned */
        if (CoversLine)
            Bounce[LineOffset - Offset] = DeviceExtension->AdjustedInterruptLine;

        RtlCopyMemory(IoStackLocation->Parameters.ReadWriteConfig.Buffer, Bounce, Length);
    }
    else
    {
        RtlCopyMemory(Bounce, IoStackLocation->Parameters.ReadWriteConfig.Buffer, Length);

        /* The register keeps its firmware value */
        if (CoversLine)
            Bounce[LineOffset - Offset] = DeviceExtension->RawInterruptLine;

        PciWriteDeviceConfig(DeviceExtension, Bounce, Offset, Length);
    }

    ExFreePoolWithTag(Bounce, PCI_POOL_TAG);

    Irp->IoStatus.Information = Length;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciPdoIrpReadConfig(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    return PciPdoReadWriteConfig(Irp, IoStackLocation, DeviceExtension, TRUE);
}

NTSTATUS
NTAPI
PciPdoIrpWriteConfig(IN PIRP Irp,
                     IN PIO_STACK_LOCATION IoStackLocation,
                     IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    return PciPdoReadWriteConfig(Irp, IoStackLocation, DeviceExtension, FALSE);
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
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    /* A bridge's FDO answers above this, so keep its bus number and buffer intact */
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
