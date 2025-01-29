/*
 * PROJECT:     ReactOS Composite Battery Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main Initialization Code and IRP Handling
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group <ros.arm@reactos.org>
 *              Copyright 2024 George Bișoc <george.bisoc@reactos.org>
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

/**
 * @brief
 * Queues a work item thread worker which is bound to the individual
 * CM (Control Method) ACPI battery to handle the IRP.
 *
 * @param[in] DeviceObject
 * A pointer to a device object, this parameter is unused.
 *
 * @param[in] Irp
 * A pointer to an I/O request packet. It is used to gather the I/O stack
 * location which contains the data of the individual battery.
 *
 * @param[in] Context
 * An aribtrary pointer that points to context data, this paramater
 * is unused.
 *
 * @return
 * Returns STATUS_MORE_PROCESSING_REQUIRED to indicate the I/O request
 * is still in action, therefore the IRP is not freed.
 */
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

/**
 * @brief
 * The brains of the battery IRP worker. It monitors the state of the
 * IRP as well as sends the IRP down the device stack to gather battery
 * related data, such tag and status. It also serves as the I/O
 * completion routine of which it elaborates the gathered data.
 *
 * @param[in] BatteryData
 * A pointer to battery data of an individual battery that contains
 * the IRP to be send down the device stack.
 */
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

/**
 * @brief
 * Calculates the total discharging/charging rate flow of each individual
 * battery linked with the composite battery and determines whether at
 * least one battery is behaving improperly.
 *
 * @param[in] DeviceExtension
 * A pointer to a device extension which describes the composite battery
 * itself. It is used to gather each connected battery in the list with
 * the composite battery.
 *
 * @param[out] TotalRate
 * A pointer returned to caller that describes the total accumulated
 * rate flow of all batteries.
 *
 * @param[out] BatteriesCount
 * A pointer returned to caller that describes the batteries present.
 *
 * @return
 * Returns TRUE if at least one battery is behaving improperly, FALSE
 * otherwise. This is determined by the fact if a battery has a negative
 * rate but is charging, or if it has a positive rate but is discharging.
 */
static
BOOLEAN
CompBattCalculateTotalRateAndLinkedBatteries(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG TotalRate,
    _Out_ PULONG BatteriesCount)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;
    BOOLEAN BadBattery = FALSE;
    ULONG LinkedBatteries = 0;
    ULONG BadBatteriesCount = 0;
    ULONG ComputedRate = 0;

    /* Loop over the linked batteries and sum up the total capacity rate */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Acquire the remove lock so this battery does not disappear under us */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
            continue;

        /*
         * Ensure this battery has a valid tag and that its rate capacity
         * is not unknown. Reject unknown rates when calculating the total rate.
         */
        if ((BatteryData->Tag != BATTERY_TAG_INVALID) &&
            (BatteryData->BatteryStatus.Rate != BATTERY_UNKNOWN_RATE))
        {
            /*
             * Now the ultimate judgement for this battery is to determine
             * if the battery behaves optimally based on its current power
             * state it is and the rate flow of the battery.
             *
             * If the rate flow is positive the battery is receiving power
             * which increases the chemical potential energy as electrons
             * move around, THIS MEANS the battery is CHARGING. If the rate
             * flow is negative the battery cells are producing way less
             * electrical energy, thus the battery is DISCHARGING.
             *
             * A consistent battery is a battery of which power state matches
             * the rate flow. If that were the case, then we have found a bad
             * battery. The worst case is that a battery is physically damanged.
             */
            if ((BatteryData->BatteryStatus.PowerState & BATTERY_DISCHARGING) &&
                (BatteryData->BatteryStatus.Rate >= 0))
            {
                if (CompBattDebug & COMPBATT_DEBUG_WARN)
                    DbgPrint("CompBatt: The battery is discharging but in reality it is charging... (Rate %d)\n",
                             BatteryData->BatteryStatus.Rate);

                BadBattery = TRUE;
                BadBatteriesCount++;
            }

            if ((BatteryData->BatteryStatus.PowerState & BATTERY_CHARGING) &&
                (BatteryData->BatteryStatus.Rate <= 0))
            {
                if (CompBattDebug & COMPBATT_DEBUG_WARN)
                    DbgPrint("CompBatt: The battery is charging but in reality it is discharging... (Rate %d)\n",
                             BatteryData->BatteryStatus.Rate);

                BadBattery = TRUE;
                BadBatteriesCount++;
            }

            if (((BatteryData->BatteryStatus.PowerState & (BATTERY_CHARGING | BATTERY_DISCHARGING)) == 0) &&
                (BatteryData->BatteryStatus.Rate != 0))
            {
                if (CompBattDebug & COMPBATT_DEBUG_WARN)
                    DbgPrint("CompBatt: The battery is neither charging or discharging but has a contradicting rate... (Rate %d)\n",
                             BatteryData->BatteryStatus.Rate);

                BadBattery = TRUE;
                BadBatteriesCount++;
            }

            /*
             * Sum up the rate of this battery to make up the total, even if that means
             * the battery may have incosistent rate. This is because it is still a linked
             * battery to the composite battery and it is used to power up the system nonetheless.
             */
            ComputedRate += BatteryData->BatteryStatus.Rate;
        }

        /* We are done with this individual battery */
        LinkedBatteries++;
        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
    }

    /* Release the lock as we are no longer poking through the batteries list */
    ExReleaseFastMutex(&DeviceExtension->Lock);

    /* Print out the total count of bad batteries we have found */
    if (BadBatteriesCount > 0)
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: %lu bad batteries have been found!\n", BadBatteriesCount);
    }

    *TotalRate = ComputedRate;
    *BatteriesCount = LinkedBatteries;
    return BadBattery;
}

/**
 * @brief
 * Sets a new configuration battery wait status settings of each battery.
 * The purpose of this is so that the composite battery gets notified
 * of new battery status as if it was a single battery.
 *
 * @param[in] DeviceExtension
 * A pointer to a device extension which describes the composite battery
 * itself. It is used to gather each connected battery in the list with
 * the composite battery.
 *
 * @param[in] BatteryTag
 * A battery tag supplied by the caller. This is typically the tag of
 * the composite battery which is used to check against the cached tag
 * of the composite battery if it has changed or not.
 *
 * @param[in] BatteryNotify
 * A pointer to a structure filled with battery notification settings,
 * supplied by the caller. It is used as the new values for the
 * configuration wait settings.
 *
 * @return
 * Returns STATUS_NO_SUCH_DEVICE if the supplied battery tag does not match
 * with that of the cached composite battery's tag or if the composite
 * battery currently does not have a tag assigned. Otherwise STATUS_SUCCESS
 * is returned.
 */
NTSTATUS
NTAPI
CompBattSetStatusNotify(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG BatteryTag,
    _In_ PBATTERY_NOTIFY BatteryNotify)
{
    NTSTATUS Status;
    BOOLEAN BadBattery;
    ULONG TotalRate;
    ULONG BatteriesCount;
    ULONG HighestCapacity;
    ULONG LowCapDifference, HighCapDifference, LowDelta, HighDelta;
    BATTERY_STATUS BatteryStatus;
    PCOMPBATT_BATTERY_DATA BatteryData;
    PLIST_ENTRY ListHead, NextEntry;

    /*
     * The caller wants to set new status notification settings but the composite
     * battery does not have a valid tag assigned, or the tag does not actually match.
     */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) ||
        (DeviceExtension->Tag != BatteryTag))
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Composite battery tag not assigned or not matching (Tag -> %lu, Composite Tag -> %lu)\n",
                     BatteryTag, DeviceExtension->Tag);

        return STATUS_NO_SUCH_DEVICE;
    }

    /*
     * Before we are setting up new status wait notification points we need to
     * refresh the composite status so that we get to know what values should be
     * set for the current notification wait status.
     */
    Status = CompBattQueryStatus(DeviceExtension,
                                 BatteryTag,
                                 &BatteryStatus);
    if (!NT_SUCCESS(Status))
    {
        if (CompBattDebug & COMPBATT_DEBUG_ERR)
            DbgPrint("CompBatt: Failed to refresh composite battery's status (Status 0x%08lx)\n", Status);

        return Status;
    }

    /* Print out battery status data that has been polled */
    if (CompBattDebug & COMPBATT_DEBUG_INFO)
        DbgPrint("CompBatt: Latest composite battery status (when setting notify status)\n"
                 "          PowerState -> 0x%lx\n"
                 "          Capacity -> %u\n"
                 "          Voltage -> %u\n"
                 "          Rate -> %d\n",
                 BatteryStatus.PowerState,
                 BatteryStatus.Capacity,
                 BatteryStatus.Voltage,
                 BatteryStatus.Rate);

    /* Calculate the high and low capacity differences based on the real summed capacity of the composite */
    LowCapDifference = DeviceExtension->BatteryStatus.Capacity - BatteryNotify->LowCapacity;
    HighCapDifference = BatteryNotify->HighCapacity - DeviceExtension->BatteryStatus.Capacity;

    /* Cache the notification parameters provided for later usage when polling for battery status */
    DeviceExtension->WaitNotifyStatus.PowerState = BatteryNotify->PowerState;
    DeviceExtension->WaitNotifyStatus.LowCapacity = BatteryNotify->LowCapacity;
    DeviceExtension->WaitNotifyStatus.HighCapacity = BatteryNotify->HighCapacity;

    /* Toggle the valid notify flag as these are the newer notification settings */
    DeviceExtension->Flags |= COMPBATT_STATUS_NOTIFY_SET;

    /*
     * Get the number of currently linked batteries to composite and total rate,
     * we will use these counters later to determine the wait values for each
     * individual battery.
     */
    BadBattery = CompBattCalculateTotalRateAndLinkedBatteries(DeviceExtension,
                                                              &TotalRate,
                                                              &BatteriesCount);

    /*
     * Of course we have to be sure that we have at least one battery linked
     * with the composite battery at this time of getting invoked to set new
     * notification wait settings.
     */
    ASSERT(BatteriesCount != 0);

    /* Walk over the linked batteries list and set up new wait configuration settings */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Acquire the remove lock so this battery does not disappear under us */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
            continue;

        /* Now release the device lock since the battery can't go away */
        ExReleaseFastMutex(&DeviceExtension->Lock);

        /* Make sure this battery has a tag before setting new wait values */
        if (BatteryData->Tag != BATTERY_TAG_INVALID)
        {
            /*
             * And also make sure this battery does not have an unknown
             * capacity, we cannot set up new configuration wait settings
             * based on that. Default the low and high wait capacities.
             */
            if (BatteryData->BatteryStatus.Capacity != BATTERY_UNKNOWN_CAPACITY)
            {
                /*
                 * Calculate the low capacity wait setting. If at least one
                 * bad battery was found while we computed the total composite
                 * rate, then divide the difference between the total batteries.
                 * Otherwise compute the battery deltas of the composite based
                 * on total summed capacity rate. Otherwise if the total rate
                 * is 0, then the real wait low and high capacities will be on
                 * par with the real capacity.
                 */
                if (BadBattery)
                {
                    LowDelta = LowCapDifference / BatteriesCount;
                    HighDelta = HighCapDifference / BatteriesCount;
                }
                else
                {
                    if (TotalRate)
                    {
                        LowDelta = COMPUTE_BATT_CAP_DELTA(LowCapDifference, BatteryData, TotalRate);
                        HighDelta = COMPUTE_BATT_CAP_DELTA(HighCapDifference, BatteryData, TotalRate);
                    }
                    else
                    {
                        LowDelta = 0;
                        HighDelta = 0;
                    }
                }

                /*
                 * Assign the wait low capacity setting ONLY if the battery delta
                 * is not high. Otherwise it has overflowed and we cannot use that
                 * for low capacity, of which we have to default it to 0.
                 */
                if (BatteryData->BatteryStatus.Capacity > LowDelta)
                {
                    BatteryData->WaitStatus.LowCapacity = BatteryData->BatteryStatus.Capacity - LowDelta;
                }
                else
                {
                    BatteryData->WaitStatus.LowCapacity = COMPBATT_WAIT_MIN_LOW_CAPACITY;
                }

                /*
                 * Assign the wait high capacity setting ONLY if the real capacity
                 * is not above the maximum highest capacity constant.
                 */
                HighestCapacity = COMPBATT_WAIT_MAX_HIGH_CAPACITY - HighDelta;
                if (HighestCapacity < BatteryData->BatteryStatus.Capacity)
                {
                    BatteryData->WaitStatus.HighCapacity = HighestCapacity;
                }
                else
                {
                    BatteryData->WaitStatus.HighCapacity = BatteryData->BatteryStatus.Capacity + HighDelta;
                }

                /*
                 * We have set up the wait values but they are in conflict with the
                 * ones set up by the IRP complete worker. We have to cancel the IRP
                 * so the worker will copy our wait configuration values.
                 */
                if ((BatteryData->Mode == COMPBATT_READ_STATUS) &&
                    (BatteryData->WaitStatus.PowerState != BatteryData->WorkerBuffer.WorkerWaitStatus.PowerState ||
                     BatteryData->WaitStatus.LowCapacity != BatteryData->WorkerBuffer.WorkerWaitStatus.LowCapacity ||
                     BatteryData->WaitStatus.HighCapacity != BatteryData->WorkerBuffer.WorkerWaitStatus.HighCapacity))
                {
                    if (CompBattDebug & COMPBATT_DEBUG_INFO)
                        DbgPrint("CompBatt: Configuration wait values are in conflict\n"
                                 "          BatteryData->WaitStatus.PowerState -> 0x%lx\n"
                                 "          BatteryData->WorkerBuffer.WorkerWaitStatus.PowerState -> 0x%lx\n"
                                 "          BatteryData->WaitStatus.LowCapacity -> %u\n"
                                 "          BatteryData->WorkerBuffer.WorkerWaitStatus.LowCapacity -> %u\n"
                                 "          BatteryData->WaitStatus.HighCapacity -> %u\n"
                                 "          BatteryData->WorkerBuffer.WorkerWaitStatus.HighCapacity -> %u\n",
                                 BatteryData->WaitStatus.PowerState,
                                 BatteryData->WorkerBuffer.WorkerWaitStatus.PowerState,
                                 BatteryData->WaitStatus.LowCapacity,
                                 BatteryData->WorkerBuffer.WorkerWaitStatus.LowCapacity,
                                 BatteryData->WaitStatus.HighCapacity,
                                 BatteryData->WorkerBuffer.WorkerWaitStatus.HighCapacity);

                    IoCancelIrp(BatteryData->Irp);
                }
            }
            else
            {
                BatteryData->WaitStatus.LowCapacity = BATTERY_UNKNOWN_CAPACITY;
                BatteryData->WaitStatus.HighCapacity = BATTERY_UNKNOWN_CAPACITY;
            }
        }

        /* We are done with this battery */
        ExAcquireFastMutex(&DeviceExtension->Lock);
        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
    }

    /* Release the lock as we are no longer poking through the batteries list */
    ExReleaseFastMutex(&DeviceExtension->Lock);

    /* Ensure the composite battery did not incur in drastic changes of tag */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) ||
        (DeviceExtension->Tag != BatteryTag))
    {
        /*
         * Either the last battery was removed (in this case the composite is no
         * longer existing) or a battery was removed of which the whole battery
         * information must be recomputed and such.
         */
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Last battery or a battery was removed, the whole composite data must be recomputed\n");

        return STATUS_NO_SUCH_DEVICE;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Queries the battery status of each individiual connected battery with
 * the composite battery and combines all the retrieved data as one
 * single battery status for the composite battery.
 *
 * @param[in] DeviceExtension
 * A pointer to a device extension which describes the composite battery
 * itself. It is used to gather each connected battery in the list with
 * the composite battery.
 *
 * @param[in] Tag
 * A battery tag supplied by the caller. This is typically the tag of
 * the composite battery which is used to check against the cached tag
 * of the composite battery if it has changed or not.
 *
 * @param[out] BatteryStatus
 * A pointer to a battery status that contains the combined data, returned
 * to the caller. It serves as the battery status for the composite battery.
 *
 * @return
 * Returns STATUS_NO_SUCH_DEVICE if the supplied battery tag does not match
 * with that of the cached composite battery's tag or if the composite
 * battery currently does not have a tag assigned. Otherwise STATUS_SUCCESS
 * is returned, which it will also return success if the composite battery's
 * cached battery status is fresh which indicates it has already been computed.
 */
NTSTATUS
NTAPI
CompBattQueryStatus(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Tag,
    _Out_ PBATTERY_STATUS BatteryStatus)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    BATTERY_WAIT_STATUS Wait;
    PLIST_ENTRY ListHead, NextEntry;
    ULONGLONG LastReadTime, CurrentReadTime;
    NTSTATUS Status = STATUS_SUCCESS;

    /*
     * The caller wants to update the composite battery status but the composite
     * itself does not have a valid tag assigned, or the tag does not actually match.
     */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) ||
        (DeviceExtension->Tag != Tag))
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Composite battery tag not assigned or not matching (Tag -> %lu, Composite Tag -> %lu)\n",
                     Tag, DeviceExtension->Tag);

        return STATUS_NO_SUCH_DEVICE;
    }

    /* Initialize the status and wait fields with zeros */
    RtlZeroMemory(BatteryStatus, sizeof(*BatteryStatus));
    RtlZeroMemory(&Wait, sizeof(Wait));

    /*
     * The battery status was already updated when the caller queried for new
     * status. We do not need to update the status again for no reason.
     * Just give them the data outright.
     */
    CurrentReadTime = KeQueryInterruptTime();
    LastReadTime = CurrentReadTime - DeviceExtension->InterruptTime;
    if (LastReadTime < COMPBATT_FRESH_STATUS_TIME)
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Composite battery status data is fresh, no need to update it again\n");

        RtlCopyMemory(BatteryStatus, &DeviceExtension->BatteryStatus, sizeof(BATTERY_STATUS));
        return STATUS_SUCCESS;
    }

    /*
     * Initialize the battery status context with unknown defaults, until we get
     * to retrieve the real data from each battery and compute the exact status.
     * Assume the system is powered by AC source for now until we find out it is
     * not the case.
     */
    BatteryStatus->PowerState = BATTERY_POWER_ON_LINE;
    BatteryStatus->Capacity = BATTERY_UNKNOWN_CAPACITY;
    BatteryStatus->Voltage = BATTERY_UNKNOWN_VOLTAGE;
    BatteryStatus->Rate = BATTERY_UNKNOWN_RATE;

    /* Iterate over all the present linked batteries and retrieve their status */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Acquire the remove lock so this battery does not disappear under us */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
            continue;

        /* Now release the device lock since the battery can't go away */
        ExReleaseFastMutex(&DeviceExtension->Lock);

        /* Setup the battery tag for the status wait which is needed to send off the IOCTL */
        Wait.BatteryTag = BatteryData->Tag;

        /* Make sure this battery has a tag before we send off the IOCTL */
        if (BatteryData->Tag != BATTERY_TAG_INVALID)
        {
            /* Only query new battery status data if it is no longer fresh */
            LastReadTime = CurrentReadTime - BatteryData->InterruptTime;
            if (LastReadTime > COMPBATT_FRESH_STATUS_TIME)
            {
                RtlZeroMemory(&BatteryData->BatteryStatus,
                              sizeof(BatteryData->BatteryStatus));
                Status = BatteryIoctl(IOCTL_BATTERY_QUERY_STATUS,
                                      BatteryData->DeviceObject,
                                      &Wait,
                                      sizeof(Wait),
                                      &BatteryData->BatteryStatus,
                                      sizeof(BatteryData->BatteryStatus),
                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    /*
                     * If the device is being suddenly removed then we must invalidate
                     * both this battery and composite tags.
                     */
                    if (Status == STATUS_DEVICE_REMOVED)
                    {
                        Status = STATUS_NO_SUCH_DEVICE;
                    }

                    ExAcquireFastMutex(&DeviceExtension->Lock);
                    IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
                    break;
                }

                /* Update the timestamp of the current read of battery status */
                BatteryData->InterruptTime = CurrentReadTime;
            }

            /*
             * Now it is time to combine the data into the composite status.
             * The battery is either charging or discharging. AC is present
             * only if the charger supplies current to all batteries. And
             * the composite is deemed as critical if at least one battery
             * is discharging and it is in crtitical state.
             */
            BatteryStatus->PowerState |= (BatteryData->BatteryStatus.PowerState & (BATTERY_CHARGING | BATTERY_DISCHARGING));
            BatteryStatus->PowerState &= (BatteryData->BatteryStatus.PowerState | ~BATTERY_POWER_ON_LINE);
            if ((BatteryData->BatteryStatus.PowerState & BATTERY_CRITICAL) &&
                (BatteryData->BatteryStatus.PowerState & BATTERY_DISCHARGING))
            {
                BatteryStatus->PowerState |= BATTERY_CRITICAL;
            }

            /* Add up the battery capacity if it is not unknown */
            if (BatteryData->BatteryStatus.Capacity != BATTERY_UNKNOWN_CAPACITY)
            {
                if (BatteryStatus->Capacity != BATTERY_UNKNOWN_CAPACITY)
                {
                    BatteryStatus->Capacity += BatteryData->BatteryStatus.Capacity;
                }
                else
                {
                    BatteryStatus->Capacity = BatteryData->BatteryStatus.Capacity;
                }
            }

            /* Always pick up the greatest voltage for the composite battery */
            if (BatteryData->BatteryStatus.Voltage != BATTERY_UNKNOWN_VOLTAGE)
            {
                if (BatteryStatus->Voltage != BATTERY_UNKNOWN_VOLTAGE)
                {
                    BatteryStatus->Voltage = max(BatteryStatus->Voltage,
                                                 BatteryData->BatteryStatus.Voltage);
                }
                else
                {
                    BatteryStatus->Voltage = BatteryData->BatteryStatus.Voltage;
                }
            }

            /* Add up the battery discharge rate if it is not unknown */
            if (BatteryData->BatteryStatus.Rate != BATTERY_UNKNOWN_RATE)
            {
                if (BatteryStatus->Rate != BATTERY_UNKNOWN_RATE)
                {
                    BatteryStatus->Rate += BatteryData->BatteryStatus.Rate;
                }
                else
                {
                    BatteryStatus->Rate = BatteryData->BatteryStatus.Rate;
                }
            }
        }

        /* We are done combining data from this battery */
        ExAcquireFastMutex(&DeviceExtension->Lock);
        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
    }

    /* Release the lock as we are no longer poking through the batteries list */
    ExReleaseFastMutex(&DeviceExtension->Lock);

    /* Ensure the composite battery did not incur in drastic changes of tag */
    if (!(DeviceExtension->Flags & COMPBATT_TAG_ASSIGNED) ||
        (DeviceExtension->Tag != Tag))
    {
        /*
         * Either the last battery was removed (in this case the composite is no
         * longer existing) or a battery was removed of which the whole battery
         * information must be recomputed and such.
         */
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Last battery or a battery was removed, the whole composite data must be recomputed\n");

        return STATUS_NO_SUCH_DEVICE;
    }

    /*
     * If there is a battery that is charging while another one discharging,
     * then tell the caller the composite battery is actually discharging.
     * This is less likely to happen on a multi-battery system like laptops
     * as the charger would provide electricity to all the batteries.
     * Perhaps the most likely case scenario would be if the system were
     * to be powered by a UPS.
     */
    if ((BatteryStatus->PowerState & BATTERY_CHARGING) &&
        (BatteryStatus->PowerState & BATTERY_DISCHARGING))
    {
        BatteryStatus->PowerState &= ~BATTERY_CHARGING;
    }

    /* Copy the combined status information to the composite battery */
    if (NT_SUCCESS(Status))
    {
        RtlCopyMemory(&DeviceExtension->BatteryStatus,
                      BatteryStatus,
                      sizeof(DeviceExtension->BatteryStatus));

        /* Update the last read battery status timestamp as well */
        DeviceExtension->InterruptTime = CurrentReadTime;
    }

    return Status;
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

/**
 * @brief
 * Calculates the "At Rate" flow of the composite battery based on the
 * sum of all connected batteries, in order to retrieve the precise
 * battery time estimation.
 *
 * @param[in] DeviceExtension
 * A pointer to a device extension which describes the composite battery
 * itself. It is used to gather each connected battery in the list with
 * the composite battery.
 *
 * @return
 * Returns the computed "At Rate" flow to the caller.
 */
static
LONG
CompBattCalculateAtRateTime(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PCOMPBATT_BATTERY_DATA BatteryData;
    BATTERY_QUERY_INFORMATION QueryInformation;
    PLIST_ENTRY ListHead, NextEntry;
    ULONG Time;
    LONG ComputedAtRate = 0;

    /* Walk over the linked batteries list to poll for "At Rate" value of each battery */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Acquire the remove lock so this battery does not disappear under us */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
            continue;

        /* Now release the device lock since the battery can't go away */
        ExReleaseFastMutex(&DeviceExtension->Lock);

        /* Build the necessary information in order to query the battery estimated time */
        QueryInformation.BatteryTag = BatteryData->Tag;
        QueryInformation.InformationLevel = BatteryEstimatedTime;
        QueryInformation.AtRate = 0;

        /* Make sure this battery has a valid tag before issuing the IOCTL */
        if (BatteryData->Tag != BATTERY_TAG_INVALID)
        {
            /*
             * Now it is time to issue the IOCTL to the battery device.
             * We are calculating the "At Rate" counter based on each linked
             * battery that is discharging, one at a time. This ensures
             * that when we will actually retrieve the estimation time of each
             * individual battery and sum it all up as one time for the composite
             * battery, that the estimated time is accurate enough.
             */
            Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                  BatteryData->DeviceObject,
                                  &QueryInformation,
                                  sizeof(QueryInformation),
                                  &Time,
                                  sizeof(Time),
                                  FALSE);
            if (NT_SUCCESS(Status))
            {
                if ((Time != 0) && (Time != BATTERY_UNKNOWN_TIME))
                {
                    ComputedAtRate -= COMPUTE_ATRATE_DRAIN(BatteryData, Time);
                }
            }
        }

        /* We are done with this battery */
        ExAcquireFastMutex(&DeviceExtension->Lock);
        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
    }

    /* Release the lock as we are no longer poking through the batteries list */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    return ComputedAtRate;
}

/**
 * @brief
 * Retrieves the estimated time of the composite battery based on the
 * power drain rate of all the batteries present in the system.
 *
 * @param[out] Time
 * A pointer to the computed estimated time of the composite battery,
 * returned to caller. Note that if there are not any batteries that
 * are draining power, or if the system is powered by external AC source,
 * the estimated time is unknown
 *
 * @param[in] DeviceExtension
 * A pointer to a device extension which describes the composite battery
 * itself. It is used to gather each connected battery in the list with
 * the composite battery.
 *
 * @return
 * Returns STATUS_NO_SUCH_DEVICE if the supplied battery tag does not match
 * with that of the cached composite battery's tag or if the composite
 * battery currently does not have a tag assigned. Otherwise STATUS_SUCCESS
 * is returned.
 */
NTSTATUS
NTAPI
CompBattGetEstimatedTime(
    _Out_ PULONG Time,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PCOMPBATT_BATTERY_DATA BatteryData;
    BATTERY_STATUS BatteryStatus;
    BATTERY_QUERY_INFORMATION QueryInformation;
    PLIST_ENTRY ListHead, NextEntry;
    ULONG ReturnedTime;
    LONG ComputedAtRate;

    /* Assume the battery time is not estimated yet */
    *Time = BATTERY_UNKNOWN_TIME;

    /*
     * Before we are querying the composite estimated battery time we must
     * refresh the battery status cache if we have not done it so.
     */
    Status = CompBattQueryStatus(DeviceExtension,
                                 DeviceExtension->Tag,
                                 &BatteryStatus);
    if (!NT_SUCCESS(Status))
    {
        if (CompBattDebug & COMPBATT_DEBUG_ERR)
            DbgPrint("CompBatt: Failed to refresh composite battery's status (Status 0x%08lx)\n", Status);

        return Status;
    }

    /* Print out battery status data that has been polled */
    if (CompBattDebug & COMPBATT_DEBUG_INFO)
        DbgPrint("CompBatt: Latest composite battery status (when querying for estimated time)\n"
                 "          PowerState -> 0x%lx\n"
                 "          Capacity -> %u\n"
                 "          Voltage -> %u\n"
                 "          Rate -> %d\n",
                 BatteryStatus.PowerState,
                 BatteryStatus.Capacity,
                 BatteryStatus.Voltage,
                 BatteryStatus.Rate);

    /*
     * If the batteries are not being discharged and the system is directly
     * being powered by external AC source then it makes no sense to
     * compute the battery estimated time because that construct is for
     * WHEN the system is powered directly from batteries and it drains power.
     */
    if (DeviceExtension->BatteryStatus.PowerState & BATTERY_POWER_ON_LINE)
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
        {
            DbgPrint("CompBatt: The system is powered by AC source, estimated time is not available\n");
        }

        return STATUS_SUCCESS;
    }

    /* Determine the draining "At Rate" counter for all batteries */
    ComputedAtRate = CompBattCalculateAtRateTime(DeviceExtension);

    /*
     * A rate of 0 indicates none of the batteries that are linked with
     * the composite are being drained therefore we cannot estimate the
     * run time of the composite as it is not discharging.
     */
    if (ComputedAtRate == 0)
    {
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: No battery is discharging and no power is being drained, cannot estimate the run time\n");

        return STATUS_SUCCESS;
    }

    /* Walk over the linked batteries list and determine the exact estimated time */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Acquire the remove lock so this battery does not disappear under us */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!NT_SUCCESS(IoAcquireRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp)))
            continue;

        /* Now release the device lock since the battery can't go away */
        ExReleaseFastMutex(&DeviceExtension->Lock);

        /* Build the necessary information in order to query the battery estimated time */
        QueryInformation.BatteryTag = BatteryData->Tag;
        QueryInformation.InformationLevel = BatteryEstimatedTime;
        QueryInformation.AtRate = ComputedAtRate;

        /* Make sure this battery has a valid tag before issuing the IOCTL */
        if (BatteryData->Tag != BATTERY_TAG_INVALID)
        {
            Status = BatteryIoctl(IOCTL_BATTERY_QUERY_INFORMATION,
                                  BatteryData->DeviceObject,
                                  &QueryInformation,
                                  sizeof(QueryInformation),
                                  &ReturnedTime,
                                  sizeof(ReturnedTime),
                                  FALSE);
            if (!NT_SUCCESS(Status))
            {
                /*
                 * If the device is being suddenly removed then we must invalidate
                 * both this battery and composite tags.
                 */
                if (Status == STATUS_DEVICE_REMOVED)
                {
                    Status = STATUS_NO_SUCH_DEVICE;
                }

                ExAcquireFastMutex(&DeviceExtension->Lock);
                IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);

                /*
                 * In other places we are ceasing the execution of the loop but
                 * here we want to continue looking for other linked batteries.
                 * This is because we are querying for the estimated battery time
                 * at the time the last battery status was valid. Also bear in
                 * mind IOCTL_BATTERY_QUERY_INFORMATION with InformationLevel as
                 * BatteryEstimatedTime might not be a valid request supported
                 * by this battery.
                 */
                continue;
            }

            /* Now sum up the estimated battery time */
            if (ReturnedTime != BATTERY_UNKNOWN_TIME)
            {
                if (*Time != BATTERY_UNKNOWN_TIME)
                {
                    *Time += ReturnedTime;
                }
                else
                {
                    *Time = ReturnedTime;
                }
            }
        }

        /* We are done with this battery */
        ExAcquireFastMutex(&DeviceExtension->Lock);
        IoReleaseRemoveLock(&BatteryData->RemoveLock, BatteryData->Irp);
    }

    /* Release the lock as we are no longer poking through the batteries list */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    return Status;
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
