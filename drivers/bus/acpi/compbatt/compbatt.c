/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG CompBattDebug;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CompBattOpenClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING OpenClose\n");

    /* Complete the IRP with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* Return success */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: Exiting OpenClose\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattSystemControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING System Control\n");

    /* Are we attached yet? */
    if (DeviceExtension->AttachedDevice)
    {
        /* Send it up the stack */
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    else
    {
        /* We don't support WMI */
        Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattMonitorIrpComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    PIO_STACK_LOCATION IoStackLocation;
    PCOMPBATT_BATTERY_DATA BatteryData;

    /* We do not care about the device object */
    UNREFERENCED_PARAMETER(DeviceObject);

    /* Grab the composite battery data from the I/O stack packet */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    BatteryData = IoStackLocation->Parameters.Others.Argument2;

    /* Request the IRP complete worker to do the deed */
    ExQueueWorkItem(&BatteryData->WorkItem, DelayedWorkQueue);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
CompBattMonitorIrpCompleteWorker(
    _In_ PCOMPBATT_BATTERY_DATA BatteryData)
{
    NTSTATUS Status;
    PIRP Irp;
    UCHAR Mode;
    ULONG PrevPowerState;
    PDEVICE_OBJECT DeviceObject;
    BATTERY_STATUS BatteryStatus;
    PIO_STACK_LOCATION IoStackLocation;
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension;

    /* Cache the necessary battery data */
    Irp = BatteryData->Irp;
    DeviceObject = BatteryData->DeviceObject;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = IoStackLocation->Parameters.Others.Argument1;

    /* Switch to the next stack as we have to setup the control function data there */
    IoStackLocation = IoGetNextIrpStackLocation(Irp);

    /* Has the I/O composite battery request succeeded? */
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status) && Status != STATUS_CANCELLED)
    {
        /*
         * This battery is being removed from the composite, perform
         * cleanups and do not inquire I/O requests again on this battery.
         */
        if (Status == STATUS_DEVICE_REMOVED)
        {
            if (CompBattDebug & COMPBATT_DEBUG_WARN)
                DbgPrint("CompBatt: Battery (0x%p) is being removed from composite battery\n", BatteryData);

            IoFreeIrp(Irp);
            CompBattRemoveBattery(&BatteryData->BatteryName, DeviceExtension);
            return;
        }

        /*
         * This is the first time a battery is being added into the composite
         * (we understand that if Status was STATUS_DEVICE_NOT_CONNECTED).
         * We must invalidate the composite tag and request a recalculation
         * of the battery tag.
         */
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Battery arrived for first time or disappeared (Status 0x%08lx)\n", Status);

        BatteryData->Tag = BATTERY_TAG_INVALID;

        /*
         * Invalidate the last read status interrupt time as well since the last
         * battery status data no longer applies. Same for the composite battery
         * as well.
         */
        BatteryData->InterruptTime = 0;
        DeviceExtension->InterruptTime = 0;

        /* Notify Battery Class the battery status incurs in a change */
        BatteryClassStatusNotify(DeviceExtension->ClassData);

        /* Setup the necessary I/O data to query the battery tag */
        IoStackLocation->Parameters.DeviceIoControl.IoControlCode = IOCTL_BATTERY_QUERY_TAG;
        IoStackLocation->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ULONG);
        BatteryData->Mode = COMPBATT_QUERY_TAG;
        BatteryData->WorkerBuffer.WorkerTag = 0xFFFFFFFF;

        /* Dispatch our request now to the battery's driver */
        goto DispatchRequest;
    }

    /* Our I/O request has been completed successfully, check what did we get */
    Mode = BatteryData->Mode;
    switch (Mode)
    {
        case COMPBATT_QUERY_TAG:
        {
            /*
             * This battery has just gotten a tag, acknowledge the composite battery
             * about that so it can recalculate its own composite tag.
             */
            if (CompBattDebug & COMPBATT_DEBUG_WARN)
                DbgPrint("CompBatt: Battery (Device 0x%p) has a tag of %lu\n", DeviceObject, BatteryData->WorkerBuffer.WorkerTag);

            /* Ensure the battery tag is not bogus, getting a tag of 0 is illegal */
            ASSERT(BatteryData->WorkerBuffer.WorkerTag != 0);

            /* Assign the battery tag */
            BatteryData->Tag = BatteryData->WorkerBuffer.WorkerTag;
            BatteryData->Flags |= COMPBATT_TAG_ASSIGNED;

            /* Punt the composite battery flags, as the previous cached data no longer applies */
            DeviceExtension->Flags = 0;

            /* Notify the Battery Class driver this battery has got a tag */
            BatteryClassStatusNotify(DeviceExtension->ClassData);
            break;
        }

        case COMPBATT_READ_STATUS:
        {
            /*
             * Read the battery status only if the IRP has not been cancelled,
             * otherwise the request must be re-issued again. This typically
             * happens if the wait values are in conflict which it might
             * end up in inconsistent battery status results.
             */
            if (Status != STATUS_CANCELLED && !Irp->Cancel)
            {
                /*
                 * If we reach here then the battery has entered into a change of
                 * power state or its charge capacity has changed.
                 */
                if (CompBattDebug & COMPBATT_DEBUG_WARN)
                    DbgPrint("CompBatt: Battery state (Device 0x%p) has changed\n", DeviceObject);

                /* Copy the battery status of this battery */
                RtlCopyMemory(&BatteryData->BatteryStatus,
                              &BatteryData->WorkerBuffer.WorkerStatus,
                              sizeof(BatteryData->BatteryStatus));

                /* Update the interrupt time as this is the most recent read of the battery status */
                BatteryData->InterruptTime = KeQueryInterruptTime();

                /*
                 * Ensure we have not gotten unknown capacities while we waited for new
                 * battery status. The battery might have malfunctioned or something.
                 */
                if (BatteryData->WorkerBuffer.WorkerStatus.Capacity == BATTERY_UNKNOWN_CAPACITY)
                {
                    /* We do not know the capacity of this battery, default the low and high capacities */
                    BatteryData->WaitStatus.LowCapacity = BATTERY_UNKNOWN_CAPACITY;
                    BatteryData->WaitStatus.HighCapacity = BATTERY_UNKNOWN_CAPACITY;
                }
                else
                {
                    /* We know the capacity, adjust the low and high capacities accordingly */
                    if (BatteryData->WaitStatus.LowCapacity >
                        BatteryData->WorkerBuffer.WorkerStatus.Capacity)
                    {
                        BatteryData->WaitStatus.LowCapacity = BatteryData->WorkerBuffer.WorkerStatus.Capacity;
                    }

                    if (BatteryData->WaitStatus.HighCapacity <
                        BatteryData->WorkerBuffer.WorkerStatus.Capacity)
                    {
                        BatteryData->WaitStatus.HighCapacity = BatteryData->WorkerBuffer.WorkerStatus.Capacity;
                    }
                }

                /* Copy the current last read power state for the next wait */
                BatteryData->WaitStatus.PowerState = BatteryData->WorkerBuffer.WorkerStatus.PowerState;

                /*
                 * Cache the previous power state of the composite battery and invalidate
                 * the last computed battery status interrupt time. This is because,
                 * logically, this specific battery incurred in a state change therefore
                 * the previous composite status is no longer consistent.
                 */
                PrevPowerState = DeviceExtension->BatteryStatus.PowerState;
                DeviceExtension->InterruptTime = 0;

                /* Compute a new battery status for the composite battery */
                Status = CompBattQueryStatus(DeviceExtension,
                                             DeviceExtension->Tag,
                                             &BatteryStatus);

                /* Print out the current battery status of the composite to the debugger */
                if ((CompBattDebug & COMPBATT_DEBUG_INFO) && NT_SUCCESS(Status))
                    DbgPrint("CompBatt: Latest composite battery status\n"
                             "          PowerState -> 0x%lx\n"
                             "          Capacity -> %u\n"
                             "          Voltage -> %u\n"
                             "          Rate -> %d\n",
                             BatteryStatus.PowerState,
                             BatteryStatus.Capacity,
                             BatteryStatus.Voltage,
                             BatteryStatus.Rate);

                /*
                 * Now determine whether should we notify the Battery Class driver due to
                 * changes in power state settings in the composite battery. This could
                 * happen in two following conditions:
                 *
                 * 1. The status notify flag was set for the respective power notification
                 *    settings, and the composite battery incurred in a change of such
                 *    settings. In this case we have to probe the current settings that
                 *    they have changed.
                 *
                 * 2. The status notify flag was not set, therefore we do not know the
                 *    exact configuration of the notification settings. We only care that
                 *    the power state has changed at this point.
                 *
                 * Why do we have to do this is because we have to warn the Battery Class
                 * about the data that has changed.
                 */
                if (!(DeviceExtension->Flags & COMPBATT_STATUS_NOTIFY_SET))
                {
                    if (PrevPowerState != DeviceExtension->BatteryStatus.PowerState)
                    {
                        /* The previous power state is no longer valid, notify Battery Class */
                        BatteryClassStatusNotify(DeviceExtension->ClassData);
                    }
                }
                else
                {
                    /*
                     * Unlike the condition above, we check for power state change against
                     * the current notify wait set since the notify set flag bit is assigned.
                     */
                    if (DeviceExtension->WaitNotifyStatus.PowerState != DeviceExtension->BatteryStatus.PowerState ||
                        DeviceExtension->WaitNotifyStatus.LowCapacity > DeviceExtension->BatteryStatus.Capacity ||
                        DeviceExtension->WaitNotifyStatus.HighCapacity < DeviceExtension->BatteryStatus.Capacity)
                    {
                        /* The following configuration settings have changed, notify Battery Class */
                        BatteryClassStatusNotify(DeviceExtension->ClassData);
                    }
                }
            }

            break;
        }

        default:
        {
            ASSERTMSG("CompBatt: BAD!!! WE SHOULD NOT BE HERE!\n", FALSE);
            UNREACHABLE;
        }
    }

    /* Setup the necessary data to read battery status */
    BatteryData->WaitStatus.BatteryTag = BatteryData->Tag;
    BatteryData->WaitStatus.Timeout = 3000;  // FIXME: Hardcoded (wait for 3 seconds) because we do not have ACPI notifications implemented yet...

    RtlCopyMemory(&BatteryData->WorkerBuffer.WorkerWaitStatus,
                  &BatteryData->WaitStatus,
                  sizeof(BatteryData->WaitStatus));

    IoStackLocation->Parameters.DeviceIoControl.IoControlCode = IOCTL_BATTERY_QUERY_STATUS;
    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength = sizeof(BatteryData->WorkerBuffer.WorkerWaitStatus);
    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = sizeof(BatteryData->WorkerBuffer.WorkerStatus);
    BatteryData->Mode = COMPBATT_READ_STATUS;

DispatchRequest:
    /* Setup the system buffer to that of the battery data which it will hold the returned data */
    IoStackLocation->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    Irp->AssociatedIrp.SystemBuffer = &BatteryData->WorkerBuffer;
    Irp->Cancel = FALSE;
    Irp->PendingReturned = FALSE;

    /* Setup the worker completion routine which it will invoke the worker later on */
    IoSetCompletionRoutine(Irp,
                           CompBattMonitorIrpComplete,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Dispatch the I/O request now */
    IoCallDriver(DeviceObject, Irp);
}

VOID
NTAPI
CompBattRecalculateTag(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    ULONG Tag;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING CompBattRecalculateTag\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and check if it has a tag */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (BatteryData->Flags & COMPBATT_TAG_ASSIGNED)
        {
            /* Generate the next tag and exit */
            Tag = DeviceExtension->NextTag;
            DeviceExtension->Flags |= COMPBATT_TAG_ASSIGNED;
            DeviceExtension->Tag = Tag;
            DeviceExtension->NextTag = Tag + 1;
            break;
       }

       /* No tag for this device extension, clear it */
       DeviceExtension->Tag = BATTERY_TAG_INVALID;
       NextEntry = NextEntry->Flink;
    }

    /* We're done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING CompBattRecalculateTag\n");
}

NTSTATUS
NTAPI
CompBattIoctl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING Ioctl\n");

    /* Let the class driver handle it */
    Status = BatteryClassIoctl(DeviceExtension->ClassData, Irp);
    if (Status == STATUS_NOT_SUPPORTED)
    {
        /* It failed, try the next driver up the stack */
        Irp->IoStatus.Status = Status;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }

    /* Return status */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING Ioctl\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattQueryTag(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG Tag)
{
    NTSTATUS Status;
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING QueryTag\n");

    /* Was a tag assigned? */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED))
    {
        /* Assign one */
        CompBattRecalculateTag(DeviceExtension);
    }

    /* Do we have a tag now? */
    if ((DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) && DeviceExtension->Tag != BATTERY_TAG_INVALID)
    {
        /* Return the tag */
        *Tag = DeviceExtension->Tag;
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No tag */
        *Tag = BATTERY_TAG_INVALID;
        Status = STATUS_NO_SUCH_DEVICE;
    }

    /* Return status */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING QueryTag\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattDisableStatusNotify(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING DisableStatusNotify\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and clear capacity data */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        BatteryData->WaitStatus.LowCapacity = 0;
        BatteryData->WaitStatus.HighCapacity = 0x7FFFFFFF;
        NextEntry = NextEntry->Flink;
    }

    /* Done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING DisableStatusNotify\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattSetStatusNotify(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG BatteryTag,
    _In_ PBATTERY_NOTIFY BatteryNotify)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattQueryStatus(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Tag,
    _Out_ PBATTERY_STATUS BatteryStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteryInformation(
    _Out_ PBATTERY_INFORMATION BatteryInfo,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BATTERY_QUERY_INFORMATION InputBuffer;
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING GetBatteryInformation\n");

    /* Set defaults */
    BatteryInfo->DefaultAlert1 = 0;
    BatteryInfo->DefaultAlert2 = 0;
    BatteryInfo->CriticalBias = 0;

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Try to acquire the remove lock */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
        {
            /* Now release the device lock since the battery can't go away */
            ExReleaseFastMutex(&DeviceExtension->Lock);

            /* Build the query */
            InputBuffer.BatteryTag = BatteryData->Tag;
            InputBuffer.InformationLevel = BatteryInformation;
            InputBuffer.AtRate = 0;

            /* Make sure the battery has a tag */
            if (BatteryData->Tag != BATTERY_TAG_INVALID)
            {
                /* Do we already have the data? */
                if (!(BatteryData->Flags & COMPBATT_BATTERY_INFORMATION_PRESENT))
                {
                    /* Send the IOCTL to query the information */
                    RtlZeroMemory(&BatteryData->BatteryInformation,
                                  sizeof(BatteryData->BatteryInformation));
                    Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                          BatteryData->DeviceObject,
                                          &InputBuffer,
                                          sizeof(InputBuffer),
                                          &BatteryData->BatteryInformation,
                                          sizeof(BatteryData->BatteryInformation),
                                          FALSE);
                    if (!NT_SUCCESS(Status))
                    {
                        /* Fail if the query had a problem */
                        if (Status == STATUS_DEVICE_REMOVED) Status = STATUS_NO_SUCH_DEVICE;
                        ExAcquireFastMutex(&DeviceExtension->Lock);
                        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
                        break;
                    }

                    /* Next time we can use the static copy */
                    BatteryData->Flags |= COMPBATT_BATTERY_INFORMATION_PRESENT;
                    if (CompBattDebug & COMPBATT_DEBUG_INFO)
                        DbgPrint("CompBattGetBatteryInformation: Read individual BATTERY_INFORMATION\n"
                                 "--------  Capabilities = %x\n--------  Technology = %x\n"
                                 "--------  Chemistry[4] = %x\n--------  DesignedCapacity = %x\n"
                                 "--------  FullChargedCapacity = %x\n--------  DefaultAlert1 = %x\n"
                                 "--------  DefaultAlert2 = %x\n--------  CriticalBias = %x\n"
                                 "--------  CycleCount = %x\n",
                                 BatteryData->BatteryInformation.Capabilities,
                                 BatteryData->BatteryInformation.Technology,
                                 BatteryData->BatteryInformation.Chemistry,
                                 BatteryData->BatteryInformation.DesignedCapacity,
                                 BatteryData->BatteryInformation.FullChargedCapacity,
                                 BatteryData->BatteryInformation.DefaultAlert1,
                                 BatteryData->BatteryInformation.DefaultAlert2,
                                 BatteryData->BatteryInformation.CriticalBias,
                                 BatteryData->BatteryInformation.CycleCount);
                }

                /* Combine capabilities */
                BatteryInfo->Capabilities |= BatteryData->BatteryInformation.Capabilities;

                /* Add-on capacity */
                if (BatteryData->BatteryInformation.DesignedCapacity != BATTERY_UNKNOWN_CAPACITY)
                {
                    BatteryInfo->DesignedCapacity += BatteryData->BatteryInformation.DesignedCapacity;
                }

                /* Add on fully charged capacity */
                if (BatteryData->BatteryInformation.FullChargedCapacity != BATTERY_UNKNOWN_CAPACITY)
                {
                    BatteryInfo->FullChargedCapacity += BatteryData->BatteryInformation.FullChargedCapacity;
                }

                /* Choose the highest alert */
                BatteryInfo->DefaultAlert1 = max(BatteryInfo->DefaultAlert1,
                                                 BatteryData->BatteryInformation.DefaultAlert1);

                /* Choose the highest alert */
                BatteryInfo->DefaultAlert2 = max(BatteryInfo->DefaultAlert2,
                                                 BatteryData->BatteryInformation.DefaultAlert2);

                /* Choose the highest critical bias */
                BatteryInfo->CriticalBias = max(BatteryInfo->CriticalBias,
                                                BatteryData->BatteryInformation.CriticalBias);
            }

            /* Re-acquire the device extension lock and release the remove lock */
            ExAcquireFastMutex(&DeviceExtension->Lock);
            IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* We are done with the list, check if the information was queried okay */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (NT_SUCCESS(Status))
    {
        /* If there's no fully charged capacity, use the design capacity */
        if (!BatteryInfo->FullChargedCapacity)
        {
            BatteryInfo->FullChargedCapacity = BatteryInfo->DesignedCapacity;
        }

        /* Print out final combined data */
        if (CompBattDebug & COMPBATT_DEBUG_INFO)
            DbgPrint("CompBattGetBatteryInformation: Returning BATTERY_INFORMATION\n"
                     "--------  Capabilities = %x\n--------  Technology = %x\n"
                     "--------  Chemistry[4] = %x\n--------  DesignedCapacity = %x\n"
                     "--------  FullChargedCapacity = %x\n--------  DefaultAlert1 = %x\n"
                     "--------  DefaultAlert2 = %x\n--------  CriticalBias = %x\n"
                     "--------  CycleCount = %x\n",
                     BatteryInfo->Capabilities,
                     BatteryInfo->Technology,
                     BatteryInfo->Chemistry,
                     BatteryInfo->DesignedCapacity,
                     BatteryInfo->FullChargedCapacity,
                     BatteryInfo->DefaultAlert1,
                     BatteryInfo->DefaultAlert2,
                     BatteryInfo->CriticalBias,
                     BatteryInfo->CycleCount);

        /* Copy the data into the device extension */
        RtlCopyMemory(&DeviceExtension->BatteryInformation,
                      BatteryInfo,
                      sizeof(DeviceExtension->BatteryInformation));
        DeviceExtension->Flags |= COMPBATT_BATTERY_INFORMATION_PRESENT;
    }

    /* We are done */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING GetBatteryInformation\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattGetBatteryGranularity(
    _Out_ PBATTERY_REPORTING_SCALE ReportingScale,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BATTERY_QUERY_INFORMATION InputBuffer;
    PCOMPBATT_BATTERY_DATA BatteryData;
    BATTERY_REPORTING_SCALE BatteryScale[4];
    PLIST_ENTRY ListHead, NextEntry;
    ULONG i;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING GetBatteryGranularity\n");

    /* Set defaults */
    ReportingScale[0].Granularity = -1;
    ReportingScale[1].Granularity = -1;
    ReportingScale[2].Granularity = -1;
    ReportingScale[3].Granularity = -1;

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Try to acquire the remove lock */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
        {
            /* Now release the device lock since the battery can't go away */
            ExReleaseFastMutex(&DeviceExtension->Lock);

            /* Build the query */
            InputBuffer.BatteryTag = BatteryData->Tag;
            InputBuffer.InformationLevel = BatteryGranularityInformation;

            /* Make sure the battery has a tag */
            if (BatteryData->Tag != BATTERY_TAG_INVALID)
            {
                /* Send the IOCTL to query the information */
                RtlZeroMemory(&BatteryData->BatteryInformation,
                              sizeof(BatteryData->BatteryInformation));
                Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                      BatteryData->DeviceObject,
                                      &InputBuffer,
                                      sizeof(InputBuffer),
                                      &BatteryScale,
                                      sizeof(BatteryScale),
                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    /* Fail if the query had a problem */
                    ExAcquireFastMutex(&DeviceExtension->Lock);
                    IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
                    break;
                }

                /* Loop all 4 scales */
                for (i = 0; i < 4; i++)
                {
                    /* Check for valid granularity */
                    if (BatteryScale[i].Granularity)
                    {
                        /* If it's smaller, use it instead */
                        ReportingScale[i].Granularity = min(BatteryScale[i].Granularity,
                                                            ReportingScale[i].Granularity);
                    }

                }
            }

            /* Re-acquire the device extension lock and release the remove lock */
            ExAcquireFastMutex(&DeviceExtension->Lock);
            IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* All done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING GetBatteryGranularity\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattGetEstimatedTime(
    _Out_ PULONG Time,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattQueryInformation(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Tag,
    _In_ BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
    _In_opt_ LONG AtRate,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnedLength)
{
    BATTERY_INFORMATION BatteryInfo;
    BATTERY_REPORTING_SCALE BatteryGranularity[4];
    PWCHAR BatteryName = L"Composite Battery";
    //BATTERY_MANUFACTURE_DATE Date;
    ULONG Dummy, Time;
    PVOID QueryData = NULL;
    ULONG QueryLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_TRACE)  DbgPrint("CompBatt: ENTERING QueryInformation\n");

    /* Check for valid/correct tag */
    if ((Tag != DeviceExtension->Tag) ||
        (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED)))
    {
        /* Not right, so fail */
        return STATUS_NO_SUCH_DEVICE;
    }

    /* Check what caller wants */
    switch (InfoLevel)
    {
        case BatteryInformation:

            /* Query combined battery information */
            RtlZeroMemory(&BatteryInfo, sizeof(BatteryInfo));
            Status = CompBattGetBatteryInformation(&BatteryInfo, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryData = &BatteryInfo;
                QueryLength = sizeof(BatteryInfo);
            }
            break;

        case BatteryGranularityInformation:

            /* Query combined granularity information */
            RtlZeroMemory(&BatteryGranularity, sizeof(BatteryGranularity));
            Status = CompBattGetBatteryGranularity(BatteryGranularity, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryLength = sizeof(BatteryGranularity);
                QueryData = &BatteryGranularity;
            }
            break;

        case BatteryEstimatedTime:

            /* Query combined time estimate information */
            RtlZeroMemory(&Time, sizeof(Time));
            Status = CompBattGetEstimatedTime(&Time, DeviceExtension);
            if (NT_SUCCESS(Status))
            {
                /* Return the data if successful */
                QueryLength = sizeof(Time);
                QueryData = &Time;
            }
            break;

        case BatteryManufactureName:
        case BatteryDeviceName:

            /* Return the static buffer */
            QueryData = BatteryName;
            QueryLength = sizeof(L"Composite Battery");
            break;

        case BatteryManufactureDate:

            /* Static data */
            //Date.Day = 26;
            //Date.Month = 06;
            //Date.Year = 1997;
            break;

        case BatteryTemperature:
        case BatteryUniqueID:

            /* Return zero */
            Dummy = 0;
            QueryData = &Dummy;
            QueryLength = sizeof(Dummy);
            break;

        default:
            /* Everything else is unknown */
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* Return the required length and check if the caller supplied enough */
    *ReturnedLength = QueryLength;
    if (BufferLength < QueryLength) Status = STATUS_BUFFER_TOO_SMALL;

    /* Copy the data if there's enough space and it exists */
    if ((NT_SUCCESS(Status)) && (QueryData)) RtlCopyMemory(Buffer, QueryData, QueryLength);

    /* Return function result */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING QueryInformation\n");
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    /* Register add device routine */
    DriverObject->DriverExtension->AddDevice = CompBattAddDevice;

    /* Register other handlers */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CompBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CompBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = CompBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CompBattSystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = CompBattPnpDispatch;
    return STATUS_SUCCESS;
}

/* EOF */
