/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/power.c
 * PURPOSE:         Bus/Device Power Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG PciPowerDelayTable[PowerDeviceD3 * PowerDeviceD3] =
{
    0,      // D0 -> D0
    0,      // D1 -> D0
    200,    // D2 -> D0
    10000,  // D3 -> D0

    0,      // D0 -> D1
    0,      // D1 -> D1
    200,    // D2 -> D1
    10000,  // D3 -> D1

    200,    // D0 -> D2
    200,    // D1 -> D2
    0,      // D2 -> D2
    10000,  // D3 -> D2

    10000,  // D0 -> D3
    10000,  // D1 -> D3
    10000,  // D2 -> D3
    0       // D3 -> D3
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
PciStallForPowerChange(IN PPCI_PDO_EXTENSION PdoExtension,
                       IN DEVICE_POWER_STATE PowerState,
                       IN ULONG_PTR CapOffset)
{
    ULONG PciState, TimeoutEntry, PmcsrOffset, TryCount;
    PPCI_VERIFIER_DATA VerifierData;
    LARGE_INTEGER Interval;
    PCI_PMCSR Pmcsr;
    KIRQL Irql;

    /* Make sure the power state is valid, and the device can support it */
    ASSERT((PdoExtension->PowerState.CurrentDeviceState >= PowerDeviceD0) &&
           (PdoExtension->PowerState.CurrentDeviceState <= PowerDeviceD3));
    ASSERT((PowerState >= PowerDeviceD0) && (PowerState <= PowerDeviceD3));
    ASSERT(!(PdoExtension->HackFlags & PCI_HACK_NO_PM_CAPS));

    /* Save the current IRQL */
    Irql = KeGetCurrentIrql();

    /* Pick the expected timeout for this transition */
    TimeoutEntry = PciPowerDelayTable[PowerState * PdoExtension->PowerState.CurrentDeviceState];

    /* PCI power states are one less than NT power states */
    PciState = PowerState - 1;

    /* The state status is stored in the PMCSR offset */
    PmcsrOffset = CapOffset + FIELD_OFFSET(PCI_PM_CAPABILITY, PMCSR);

    /* Try changing the power state up to 100 times */
    TryCount = 100;
    while (--TryCount)
    {
        /* Check if this state transition will take time */
        if (TimeoutEntry > 0)
        {
            /* Check if this is happening at high IRQL */
            if (Irql >= DISPATCH_LEVEL)
            {
                /* Can't wait at high IRQL, stall the processor */
                KeStallExecutionProcessor(TimeoutEntry);
            }
            else
            {
                /* Do a wait for the timeout specified instead */
                Interval.QuadPart = -10 * TimeoutEntry;
                Interval.QuadPart -= KeQueryTimeIncrement() - 1;
                KeDelayExecutionThread(KernelMode, FALSE, &Interval);
            }
        }

        /* Read the PMCSR and see if the state has changed */
        PciReadDeviceConfig(PdoExtension, &Pmcsr, PmcsrOffset, sizeof(PCI_PMCSR));
        if (Pmcsr.PowerState == PciState) return STATUS_SUCCESS;

        /* Try again, forcing a timeout of 1ms */
        TimeoutEntry = 1000;
    }

    /* Call verifier with this error */
    VerifierData = PciVerifierRetrieveFailureData(2);
    ASSERT(VerifierData);
    VfFailDeviceNode(PdoExtension->PhysicalDeviceObject,
                     PCI_VERIFIER_DETECTED_VIOLATION,
                     2, // The PMCSR register was not updated within the spec-mandated time.
                     VerifierData->FailureClass,
                     &VerifierData->AssertionControl,
                     VerifierData->DebuggerMessageText,
                     "%DevObj%Ulong",
                     PdoExtension->PhysicalDeviceObject,
                     PciState);

    return STATUS_DEVICE_PROTOCOL_ERROR;
}

NTSTATUS
NTAPI
PciSetPowerManagedDevicePowerState(IN PPCI_PDO_EXTENSION DeviceExtension,
                                   IN DEVICE_POWER_STATE DeviceState,
                                   IN BOOLEAN IrpSet)
{
    NTSTATUS Status;
    PCI_PM_CAPABILITY PmCaps;
    ULONG CapsOffset;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Check if this device can support low power states */
    if (!(PciCanDisableDecodes(DeviceExtension, NULL, 0, TRUE)) &&
         (DeviceState != PowerDeviceD0))
    {
        /* Simply return success, ignoring this request */
        DPRINT1("Cannot disable decodes on this device, ignoring PM request...\n");
        return Status;
    }

    /* Does the device support power management at all? */
    if (!(DeviceExtension->HackFlags & PCI_HACK_NO_PM_CAPS))
    {
        /* Get the PM capabilities register */
        CapsOffset = PciReadDeviceCapability(DeviceExtension,
                                             DeviceExtension->CapabilitiesPtr,
                                             PCI_CAPABILITY_ID_POWER_MANAGEMENT,
                                             &PmCaps.Header,
                                             sizeof(PCI_PM_CAPABILITY));
        ASSERT(CapsOffset);
        ASSERT(DeviceState != PowerDeviceUnspecified);

        /* Check if the device is being powered up */
        if (DeviceState == PowerDeviceD0)
        {
            /* Set full power state */
            PmCaps.PMCSR.ControlStatus.PowerState = 0;

            /* Check if the device supports Cold-D3 poweroff */
            if (PmCaps.PMC.Capabilities.Support.PMED3Cold)
            {
                /* If there was a pending PME, clear it */
                PmCaps.PMCSR.ControlStatus.PMEStatus = 1;
            }
        }
        else
        {
            /* Otherwise, just set the new power state, converting from NT */
            PmCaps.PMCSR.ControlStatus.PowerState = DeviceState - 1;
        }

        /* Write the new power state in the PMCSR */
        PciWriteDeviceConfig(DeviceExtension,
                             &PmCaps.PMCSR,
                             CapsOffset + FIELD_OFFSET(PCI_PM_CAPABILITY, PMCSR),
                             sizeof(PCI_PMCSR));

        /* Now wait for the change to "stick" based on the spec-mandated time */
        Status = PciStallForPowerChange(DeviceExtension, DeviceState, CapsOffset);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Nothing to do! */
        DPRINT1("No PM on this device, ignoring request\n");
    }

    /* Check if new resources have to be assigned */
    if (IrpSet)
    {
        /* Check if the new device state is lower (higher power) than now */
        if (DeviceState < DeviceExtension->PowerState.CurrentDeviceState)
        {
            /* We would normally re-assign resources after powerup */
            UNIMPLEMENTED_DBGBREAK();
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }

    /* Return the power state change status */
    return Status;
}

static REQUEST_POWER_COMPLETE PciFdoDevicePowerCompletion;

/**
 * @brief
 * Finishes a system set power request once the bus has been through the
 * device state that request asked for.
 *
 * @param[in] Context
 * The system power IRP that is still held.
 */
static
VOID
NTAPI
PciFdoDevicePowerCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PVOID Context,
    _In_ PIO_STATUS_BLOCK IoStatus)
{
    PIRP SystemIrp = Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MinorFunction);
    ASSERT(SystemIrp != NULL);

    /* The system transition goes ahead even if the bus could not follow it */
    if (!NT_SUCCESS(IoStatus->Status))
    {
        DPRINT1("PCI: Bus failed to enter device state %lu (0x%08lx)\n",
                PowerState.DeviceState,
                IoStatus->Status);
    }

    PoStartNextPowerIrp(SystemIrp);
    IoCompleteRequest(SystemIrp, IO_NO_INCREMENT);
}

static IO_COMPLETION_ROUTINE PciFdoSystemPowerCompletion;

/**
 * @brief
 * Asks for the device state that matches a system set power request once the
 * drivers below the bus have handled it.
 *
 * @param[in] Context
 * The FDO extension of the bus.
 *
 * @return
 * STATUS_MORE_PROCESSING_REQUIRED while the device state request is out.
 */
static
NTSTATUS
NTAPI
PciFdoSystemPowerCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PPCI_FDO_EXTENSION DeviceExtension = Context;
    PIO_STACK_LOCATION IoStackLocation;
    POWER_STATE Target;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);
    ASSERT(DeviceExtension != NULL);

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.Power.State.SystemState == PowerSystemWorking)
    {
        Target.DeviceState = PowerDeviceD0;
    }
    else
    {
        Target.DeviceState = PowerDeviceD3;
    }

    if ((NT_SUCCESS(Irp->IoStatus.Status)) &&
        (DeviceExtension->PowerState.CurrentDeviceState != Target.DeviceState))
    {
        Status = PoRequestPowerIrp(DeviceExtension->FunctionalDeviceObject,
                                   IRP_MN_SET_POWER,
                                   Target,
                                   PciFdoDevicePowerCompletion,
                                   Irp,
                                   NULL);
        if (NT_SUCCESS(Status))
            return STATUS_MORE_PROCESSING_REQUIRED;

        DPRINT1("PCI: Could not request device state %lu (0x%08lx)\n",
                Target.DeviceState,
                Status);
    }

    PoStartNextPowerIrp(Irp);
    return STATUS_CONTINUE_COMPLETION;
}

NTSTATUS
NTAPI
PciFdoWaitWake(IN PIRP Irp,
               IN PIO_STACK_LOCATION IoStackLocation,
               IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(IoStackLocation);

    /* The bus never arms itself, so whatever lies below answers this */
    return PciPassIrpFromFdoToPdo(DeviceExtension, Irp);
}

NTSTATUS
NTAPI
PciFdoSetPowerState(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_FDO_EXTENSION DeviceExtension)
{
    POWER_STATE State = IoStackLocation->Parameters.Power.State;

    /* Bridge hardware is handled by the PDO below, so only the state is kept */
    if (IoStackLocation->Parameters.Power.Type == DevicePowerState)
    {
        DeviceExtension->PowerState.CurrentDeviceState = State.DeviceState;
        return STATUS_SUCCESS;
    }

    DeviceExtension->PowerState.CurrentSystemState = State.SystemState;
    if (DeviceExtension->DeviceState != PciStarted)
        return STATUS_SUCCESS;

    /* The bus owns its power policy and follows up with a device state request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoMarkIrpPending(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           PciFdoSystemPowerCompletion,
                           DeviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);
    PoCallDriver(DeviceExtension->AttachedDeviceObject, Irp);
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
PciFdoIrpQueryPower(IN PIRP Irp,
                    IN PIO_STACK_LOCATION IoStackLocation,
                    IN PPCI_FDO_EXTENSION DeviceExtension)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IoStackLocation);
    UNREFERENCED_PARAMETER(DeviceExtension);

    /* The bus is never armed for wake, so no state is refused */
    return STATUS_SUCCESS;
}

/* EOF */
