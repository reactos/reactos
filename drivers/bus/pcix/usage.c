/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/usage.c
 * PURPOSE:         Bus/Device Usage Reporting
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Counts a paging, hibernation or dump file arriving on or leaving a device.
 *
 * @param[in,out] PowerState
 * The power state of the extension that holds the counts.
 *
 * @param[in] IoStackLocation
 * The stack location of the device usage notification.
 *
 * @return
 * STATUS_SUCCESS for a usage the driver accepts, STATUS_NOT_SUPPORTED otherwise.
 */
NTSTATUS
NTAPI
PciUpdateDeviceUsage(
    _Inout_ PPCI_POWER_STATE PowerState,
    _In_ PIO_STACK_LOCATION IoStackLocation)
{
    PLONG Count;

    PAGED_CODE();

    switch (IoStackLocation->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Count = &PowerState->Paging;
            break;

        case DeviceUsageTypeHibernation:
            Count = &PowerState->Hibernate;
            break;

        case DeviceUsageTypeDumpFile:
            Count = &PowerState->CrashDump;
            break;

        /* Accepted, but nothing is held back for the boot device */
        case DeviceUsageTypeBoot:
            return STATUS_SUCCESS;

        default:
            return STATUS_NOT_SUPPORTED;
    }

    /* Several files can share a device, so these are counts and not flags */
    if (IoStackLocation->Parameters.UsageNotification.InPath)
    {
        InterlockedIncrement(Count);
    }
    else
    {
        ASSERT(*Count > 0);
        InterlockedDecrement(Count);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Sends a device usage notification to the top of the stack of a parent bus.
 *
 * @param[in] ParentFdoExtension
 * The FDO extension of the bus the function is on.
 *
 * @param[in] IoStackLocation
 * The stack location of the notification the function received.
 *
 * @return
 * The status the parent stack completed the notification with.
 */
NTSTATUS
NTAPI
PciSendDeviceUsageToParent(
    _In_ PPCI_FDO_EXTENSION ParentFdoExtension,
    _In_ PIO_STACK_LOCATION IoStackLocation)
{
    PDEVICE_OBJECT TopDevice;
    PIO_STACK_LOCATION NextStack;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;

    PAGED_CODE();

    TopDevice = IoGetAttachedDeviceReference(ParentFdoExtension->PhysicalDeviceObject);

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       TopDevice,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (!Irp)
    {
        ObDereferenceObject(TopDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* A PnP IRP nobody handles must come back as not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    NextStack = IoGetNextIrpStackLocation(Irp);
    NextStack->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    NextStack->Parameters.UsageNotification = IoStackLocation->Parameters.UsageNotification;

    Status = IoCallDriver(TopDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    ObDereferenceObject(TopDevice);
    return Status;
}

/* EOF */
