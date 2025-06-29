/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Composite battery power management routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PPOP_BATTERY PopBattery;

/* COMPLETION ROUTINE *********************************************************/

static IO_COMPLETION_ROUTINE PopBatteryIrpCompletion;

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
PopIsSystemDrainingAcSource(
    _In_ ULONG PowerState,
    _Out_ PBOOLEAN Changed)
{
    PSYSTEM_POWER_POLICY SystemPowerPolicy;

    /* Assume the power policy has not changed */
    *Changed = FALSE;
    SystemPowerPolicy = PopDefaultPowerPolicy;

    /*
     * A fully charged up battery is indicated when the machine drains
     * electrical power from the AC source and not from the battery itself.
     */
    if (PowerState & BATTERY_POWER_ON_LINE)
    {
        /* The system is indeed consuming power from AC source, set the AC policy */
        PopSetDefaultPowerPolicy(PolicyAc);
    }
    else
    {
        /* The system is no longer accessing power from AC source, set the DC policy */
        PopSetDefaultPowerPolicy(PolicyDc);
    }

    /*
     * Now check if the power policy has changed at all. If it did, it indicates
     * the event of the system draining AC power is occurring just now.
     */
    if (SystemPowerPolicy != PopDefaultPowerPolicy)
    {
        *Changed = TRUE;
    }
}

static
NTSTATUS
NTAPI
PopBatteryIrpCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    NTSTATUS Status;
    UCHAR Mode;
    ULONG Tag;
    ULONG EstTime;
    BATTERY_STATUS BattStatus;
    BATTERY_INFORMATION BattInfo;
    BOOLEAN PolicyChanged;
    PPOP_DEVICE_POLICY_WORKITEM_DATA WorkItemData;

    /* We do not care about these as we already have them in the composite battery */
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    /* Check if our I/O request has succeeded */
    Status = PopBattery->Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_CANCELLED)
    {
        /*
         * It failed. However it could also be the battery has not yet been connected
         * or it has disappeared of which we have to wait on the Composite Battery to
         * add any upcoming ACPI batteries.
         */
        if (Status == STATUS_NO_SUCH_DEVICE)
        {
            DPRINT1("Battery not yet connect or it has disappeared, waiting for the battery to appear\n");
            PopBattery->Flags |= POP_CB_WAIT_ON_BATTERY_TAG;
            IoFreeIrp(PopBattery->Irp);
            PopBattery->Irp = NULL;
            goto InvokeHandler;
        }

        /*
         * We have failed the request by other means. There is nothing that we can do
         * at this point. We must inquire the removal process of the battery and jump
         * back to AC power policy and such.
         */
        DPRINT1("The composite battery I/O request has failed (Status 0x%08lx)\n", Status);
        PopBattery->Flags |= POP_CB_REMOVE_BATTERY;
        PopBattery->IoError = TRUE;
        goto InvokeHandler;
    }

    /*
     * Process this request only if the IRP has not been cancelled, otherwise we
     * must recycle the request and retry issuing it again.
     */
    if (Status != STATUS_CANCELLED &&
        !PopBattery->Irp->Cancel)
    {
        /* Process this request depending on what we actually asked */
        Mode = PopBattery->Mode;
        switch (Mode)
        {
            case POP_CB_READ_TAG_MODE:
            {
                /*
                 * We have gotten the battery tag of which we can use it to identify
                 * it and query critical battery data based on this tag. Take away
                 * the processing mode request.
                 */
                PopBattery->Flags &= ~POP_CB_PROCESSING_MODE_REQUEST;

                /* Copy the battery tag into the kernel structure */
                Tag = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
                PopBattery->BatteryTag = Tag;
                PopFreePool(Irp->AssociatedIrp.SystemBuffer, TAG_PO_BATTERY_IO_DATA);

                /* The system now has a composite battery, take away the "no battery" flag */
                PopBattery->Flags &= ~POP_CB_NO_BATTERY;

                /* FIXME: Once I implement battery trigger levels, the CB levels must be reset here */

                /*
                 * If this was a new battery candidate or we had to wait for the
                 * battery to have a tag assigned, take away these flags.
                 */
                if (PopBattery->Flags & POP_CB_PENDING_NEW_BATTERY)
                {
                    PopBattery->Flags &= ~POP_CB_PENDING_NEW_BATTERY;
                }

                if (PopBattery->Flags & POP_CB_WAIT_ON_BATTERY_TAG)
                {
                    PopBattery->Flags &= ~POP_CB_WAIT_ON_BATTERY_TAG;
                }

                /* Instruct the CB handler to query battery information */
                PopBattery->Mode = POP_CB_QUERY_INFORMATION_MODE;
                break;
            }

            case POP_CB_QUERY_INFORMATION_MODE:
            {
                /*
                 * We have the necessary battery information so that the battery
                 * status can be queried later. Take away the processing mode request
                 * and copy the information.
                 */
                PopBattery->Flags &= ~POP_CB_PROCESSING_MODE_REQUEST;
                BattInfo = *(PBATTERY_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
                RtlCopyMemory(&PopBattery->BattInfo, &BattInfo, sizeof(BattInfo));
                PopFreePool(Irp->AssociatedIrp.SystemBuffer, TAG_PO_BATTERY_IO_DATA);

                /* Report to the debugger the battery information */
#if DBG
                PopReportBatteryInformation(&PopBattery->BattInfo);
#endif

                /* Instruct the CB handler to query battery status */
                PopBattery->Mode = POP_CB_QUERY_STATUS_MODE;
                break;
            }

            case POP_CB_QUERY_STATUS_MODE:
            {
                /*
                 * We have queried the current battery status. Take away the
                 * processing mode request.
                 */
                PopBattery->Flags &= ~POP_CB_PROCESSING_MODE_REQUEST;

                /*
                 * Figure out if the current global power policy must be changed or
                 * not, to DC or AC. We have to determine that by checking if the
                 * machine is draining electrical power from the AC source and not
                 * from the battery.
                 */
                BattStatus = *(PBATTERY_STATUS)Irp->AssociatedIrp.SystemBuffer;
                PopIsSystemDrainingAcSource(BattStatus.PowerState, &PolicyChanged);

                /*
                 * The system power policy has changed. We need to recompute the thermal throttle
                 * gauges and the system idleness.
                 *
                 * FIXME: And of course code does not get written by itself.... a human can do that.
                 */
                if (PolicyChanged)
                {
                    DPRINT1("System power policy has been changed, recomputing system idleness and throttle not IMPLEMENTED yet\n");
                }

                /*
                 * Keep querying for battery status until one of the two
                 * conditions below are met.
                 */
                PopBattery->Mode = POP_CB_QUERY_STATUS_MODE;

                /*
                 * If the latest battery capacity has changed since we waited
                 * for the actual battery status data to come, then instruct
                 * the CB handler to query for battery estimated time based
                 * on the new status.
                 */
                if (PopBattery->Status.Capacity != BattStatus.Capacity)
                {
                    PopBattery->Mode = POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE;
                }

                /* Copy the currently queried battery status */
                RtlCopyMemory(&PopBattery->Status, &BattStatus, sizeof(BattStatus));
                PopFreePool(Irp->AssociatedIrp.SystemBuffer, TAG_PO_BATTERY_IO_DATA);

                /* Report to the debugger the battery status */
#if DBG
                PopReportBatteryStatus(&PopBattery->Status);
#endif
                break;
            }

            case POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE:
            {
                /*
                 * We have the estimated time remaining of the battery life.
                 * Take away the processing mode request.
                 */
                PopBattery->Flags &= ~POP_CB_PROCESSING_MODE_REQUEST;

                /* Copy the battery estimation time */
                EstTime = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
                PopBattery->EstimatedBatteryTime = EstTime;
                PopFreePool(Irp->AssociatedIrp.SystemBuffer, TAG_PO_BATTERY_IO_DATA);

                /* Instruct the CB handler to query battery status again */
                PopBattery->Mode = POP_CB_QUERY_STATUS_MODE;
                break;
            }

            case POP_CB_QUERY_TEMPERATURE_MODE:
            {
                DPRINT1("Battery query temperature isn't implemented yet\n");
                ASSERT(FALSE);
                break;
            }

            default:
            {
                /*
                 * We got an unknown battery request mode of which we have no idea
                 * what to do. This is a serious bug in our end so behead the system.
                 */
                KeBugCheckEx(INTERNAL_POWER_ERROR,
                             0x300,
                             POP_BATTERY_UNKNOWN_MODE_REQUEST,
                             (ULONG_PTR)DeviceObject,
                             (ULONG_PTR)Irp);
            }
        }
    }
    else
    {
        DPRINT1("IRP (0x%p) has BEEN CANCELLED, must re-issue the mode (%u) request again\n", PopBattery->Irp, PopBattery->Mode);
    }

    /* Free the current IRP so that the handler can issue a new one */
    IoFreeIrp(PopBattery->Irp);
    PopBattery->Irp = NULL;

InvokeHandler:
    /* Allocate a policy workitem data so that we can invoke the CB handler */
    WorkItemData = PopAllocatePool(sizeof(POP_DEVICE_POLICY_WORKITEM_DATA),
                                   FALSE,
                                   TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
    NT_ASSERT(WorkItemData != NULL);

    WorkItemData->PolicyData = NULL;
    WorkItemData->PolicyType = PolicyDeviceBattery;
    ExInitializeWorkItem(&WorkItemData->WorkItem,
                         PopCompositeBatteryHandler,
                         WorkItemData);

    /* Enqueue the control switch back to the handler */
    ExQueueWorkItem(&WorkItemData->WorkItem, DelayedWorkQueue);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
VOID
PopCalculateBatteryCapacityThresholds(
    _Inout_ PBATTERY_WAIT_STATUS WaitStatus)
{
    ULONG EstimatedCapacity;
    ULONG FullChargeCapacity;

    PAGED_CODE();

    /*
     * Calculate the estimated current charge capacity based on the amount of
     * resolutions the system must be notified of battery capacity state change
     * and the full charge capacity of this battery.
     */
    FullChargeCapacity = PopBattery->BattInfo.FullChargedCapacity;
    EstimatedCapacity = (FullChargeCapacity * PopDefaultPowerPolicy->BroadcastCapacityResolution) / 100;

    /*
     * It could happen the full charge capacity is really low therefore we might
     * get an estimated capacity of 0, in this case we have to take the maximum
     * capacity (that is being 1).
     */
    if (EstimatedCapacity == 0)
    {
        EstimatedCapacity = 1;
    }

    /*
     * Assign the low capacity if we are sure the reported current capacity
     * from the battery status is above the estimated capacity.
     */
    WaitStatus->LowCapacity = 0;
    if (PopBattery->Status.Capacity > EstimatedCapacity)
    {
        WaitStatus->LowCapacity = PopBattery->Status.Capacity - EstimatedCapacity;
    }

    /* Always set up the highest capacity, regardless if the current reported capacity is low */
    WaitStatus->HighCapacity = PopBattery->Status.Capacity + EstimatedCapacity;
    if (WaitStatus->HighCapacity < EstimatedCapacity)
    {
        /*
         * It may happen the estimated capacity is way above the calculated
         * high capacity, leading to overflow. We do not want that.
         */
        WaitStatus->HighCapacity = 0xFFFFFFFF;
    }
}

static
PULONG
PopReadBatteryTag(
    _In_ BOOLEAN WaitOnTag)
{
    ULONG Timeout;
    PULONG Buffer;

    /*
     * Read the battery tag immediately as soon as it gets assigned
     * otherwise we have to wait on the battery to get a tag for at least 1.5 s.
     */
    Timeout = (WaitOnTag == TRUE) ? 1500 : 0;

    /* Allocate the battery I/O data buffer to hold our information to the Battery Class driver */
    Buffer = PopAllocatePool(sizeof(ULONG),
                             TRUE,
                             TAG_PO_BATTERY_IO_DATA);
    if (Buffer == NULL)
    {
        DPRINT1("Battery I/O tag data allocation has failed\n");
        return NULL;
    }

    /* Copy the battery tag into the buffer */
    RtlCopyMemory(Buffer, &Timeout, sizeof(ULONG));

    /* Acknowledge the Power Manager the policy battery is processing a mode request */
    PopBattery->Flags |= POP_CB_PROCESSING_MODE_REQUEST;
    return Buffer;
}

static
PBATTERY_QUERY_INFORMATION
PopReadBatteryInformation(
    _In_ POP_BATTERY_INFORMATION_TYPE InfoType)
{
    BATTERY_QUERY_INFORMATION QueryInfo;
    PBATTERY_QUERY_INFORMATION Buffer;
    ULONG RequiredLength;

    /* The battery should have already processed this request */
    POP_ASSERT_NO_BATTERY_REQUEST_MODE();

    /*
     * By this point we should already have a battery tag that identifies this
     * battery, otherwise we got bogus data from the Battery Class driver.
     */
    ASSERT(PopBattery->BatteryTag != 0);

    /*
     * We must allocate a buffer that is big enough to hold our information
     * but also receive the queried battery information.
     */
    RequiredLength = sizeof(BATTERY_QUERY_INFORMATION) + sizeof(BATTERY_INFORMATION);

    /* Allocate the battery I/O data buffer to hold our information to the Battery Class driver */
    Buffer = PopAllocatePool(RequiredLength,
                             TRUE,
                             TAG_PO_BATTERY_IO_DATA);
    if (Buffer == NULL)
    {
        DPRINT1("Battery I/O query information data allocation has failed\n");
        return NULL;
    }

    /* Setup the right query battery information for this operation */
    RtlZeroMemory(&QueryInfo, sizeof(QueryInfo));
    QueryInfo.BatteryTag = PopBattery->BatteryTag;
    switch (InfoType)
    {
        case BatteryInfo:
        {
            QueryInfo.InformationLevel = BatteryInformation;
            break;
        }

        case BatteryEstTime:
        {
            QueryInfo.InformationLevel = BatteryEstimatedTime;
            QueryInfo.AtRate = 0;
            break;
        }

        case BatteryTemp:
        {
            DPRINT1("Battery query temperature isn't implemented yet\n");
            ASSERT(FALSE);
            break;
        }

        /* We should not reach here */
        DEFAULT_UNREACHABLE;
    }

    /* Copy the battery query information into the buffer */
    RtlCopyMemory(Buffer, &QueryInfo, sizeof(BATTERY_QUERY_INFORMATION));

    /* Acknowledge the Power Manager the policy battery is processing a mode request */
    PopBattery->Flags |= POP_CB_PROCESSING_MODE_REQUEST;
    return Buffer;
}

static
PBATTERY_WAIT_STATUS
PopReadBatteryStatus(VOID)
{
    BATTERY_WAIT_STATUS WaitStatus;
    PBATTERY_WAIT_STATUS Buffer;
    ULONG RequiredLength;

    /* The battery should have already processed this request */
    POP_ASSERT_NO_BATTERY_REQUEST_MODE();

    /*
     * We must allocate a buffer that is big enough to hold our information
     * but also receive the queried battery status.
     */
    RequiredLength = sizeof(BATTERY_WAIT_STATUS) + sizeof(BATTERY_STATUS);

    /* Allocate the battery I/O data buffer to hold our information to the Battery Class driver */
    Buffer = PopAllocatePool(RequiredLength,
                             TRUE,
                             TAG_PO_BATTERY_IO_DATA);
    if (Buffer == NULL)
    {
        DPRINT1("Battery I/O wait status data allocation has failed\n");
        return NULL;
    }

    /*
     * Fill in battery tag and the information required to read the
     * status of the battery.
     */
    RtlZeroMemory(&WaitStatus, sizeof(WaitStatus));
    WaitStatus.BatteryTag = PopBattery->BatteryTag;
    WaitStatus.Timeout = POP_CB_STATUS_WAIT_INTERVAL;
    WaitStatus.PowerState = PopBattery->Status.PowerState;

    /* Calculate the low and high capacities of which we should be notified of new battery status */
    PopCalculateBatteryCapacityThresholds(&WaitStatus);

    /* Allocate the battery I/O data buffer to hold our information to the Battery Class driver */
    RtlCopyMemory(Buffer, &WaitStatus, sizeof(BATTERY_WAIT_STATUS));

    /* Acknowledge the Power Manager the policy battery is processing a mode request */
    PopBattery->Flags |= POP_CB_PROCESSING_MODE_REQUEST;
    return Buffer;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopMarkNewBatteryPending(
    _In_ PUNICODE_STRING BatteryName)
{
    DPRINT1("Got ACPI notification of a new battery -> %wZ\n", BatteryName);
    PopBattery->Flags |= POP_CB_PENDING_NEW_BATTERY;
}

VOID
NTAPI
PopQueryBatteryState(
    _Out_ PSYSTEM_BATTERY_STATE BatteryState)
{
    PAGED_CODE();

    /* The policy lock must be owned as when quering battery state */
    POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP();

    /* Fill the battery state field with zeroes */
    RtlZeroMemory(BatteryState, sizeof(*BatteryState));

    /*
     * No composite battery was ever connected with the Power Manager,
     * or the battery disappeared at the time the caller requested
     * the state of the battery.
     */
    if (PopBattery->Flags & POP_CB_NO_BATTERY)
    {
        /*
         * Assume the system is powered up by AC source, unless the system
         * really had a battery but due to some problems while fetching
         * data from the battery the Power Manager had to disconnect the
         * composite battery, tell the user the system physically depends
         * on DC source.
         */
        BatteryState->AcOnLine = TRUE;
        if (PopBattery->IoError)
        {
            BatteryState->AcOnLine = FALSE;
        }

        return;
    }

    /*
     * If the system is powered by external AC source (power charging cord plugged in)
     * with battery fully charged, acknowledge that.
     */
    BatteryState->AcOnLine = FALSE;
    if (PopBattery->Status.PowerState & BATTERY_POWER_ON_LINE)
    {
        BatteryState->AcOnLine = TRUE;
    }

    /* At least one battery (or more) is present at this point */
    BatteryState->BatteryPresent = TRUE;

    /* If the battery is charging, acknowledge that */
    BatteryState->Charging = FALSE;
    if (PopBattery->Status.PowerState & BATTERY_CHARGING)
    {
        BatteryState->Charging = TRUE;
    }

    /* If the battery is discharging, acknowledge that */
    BatteryState->Discharging = FALSE;
    if (PopBattery->Status.PowerState & BATTERY_DISCHARGING)
    {
        BatteryState->Discharging = TRUE;
    }

    /* Return the rest of the battery state data to caller */
    BatteryState->MaxCapacity = PopBattery->BattInfo.FullChargedCapacity;
    BatteryState->RemainingCapacity = PopBattery->Status.Capacity;
    BatteryState->Rate = PopBattery->Status.Rate;
    BatteryState->EstimatedTime = PopBattery->EstimatedBatteryTime;
    BatteryState->DefaultAlert1 = PopBattery->BattInfo.DefaultAlert1;
    BatteryState->DefaultAlert2 = PopBattery->BattInfo.DefaultAlert2;
}

_Use_decl_annotations_
VOID
NTAPI
PopCompositeBatteryHandler(
    _In_ PVOID Parameter)
{
    PIRP Irp;
    UCHAR Mode;
    PVOID IoBuffer;
    ULONG IoControlCode;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_OBJECT DeviceObject;
    ULONG InputBufferLength, OutputBufferLength;
    PPOP_DEVICE_POLICY_WORKITEM_DATA WorkItemData = (PPOP_DEVICE_POLICY_WORKITEM_DATA)Parameter;
    BOOLEAN WaitOnTag = FALSE;

    PAGED_CODE();

    /* We entered into a device policy handler, acquire the policy lock */
    PopAcquirePowerPolicyLock();

    /*
     * Ensure we got the right policy device as this handler only processes
     * composite batteries and not anything else. We do not care about about
     * the policy data as the kernel holds the composite battery data into
     * a global variable, PopBattery.
     */
    ASSERT(WorkItemData->PolicyType == PolicyDeviceBattery);

    /*
     * A battery is about to be removed. This means we got an error of which no
     * possible solution could be thought to handle it, or the battery just
     * simply disappeared.
     */
    if (PopBattery->Flags & POP_CB_REMOVE_BATTERY)
    {
        PopDisconnectCompositeBattery();
        PopReleasePowerPolicyLock();
        PopFreePool(WorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
        return;
    }

    /*
     * Ensure the composite battery does not have an outstanding IRP that still needs
     * to be completed. The CB IRP completion handler is responsible to free it once
     * it has got data from the Battery Class driver of which the IRP is no longer needed.
     */
    ASSERT(PopBattery->Irp == NULL);

    /* Allocate a new fresh IRP to satisfy the required request */
    DeviceObject = PopBattery->DeviceObject;
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /*
         * Failing this request on our end is fatal. Most likely the I/O manager
         * tried so hard to allocate an IRP for us it failed on doing so, due to
         * a serious low memory condition. Not something that we can do to salvage
         * the system so kill it.
         */
        KeBugCheckEx(INTERNAL_POWER_ERROR,
                     0,
                     POP_DEVICE_POLICY_IRP_ALLOC_FAILED,
                     PolicyDeviceBattery,
                     (ULONG_PTR)DeviceObject);
    }

    /*
     * We do not have any composite battery policy connected and we got a battery
     * for the first time, or we got a new one. We must query the tag information
     * in order to gather battery information and other related data.
     */
    if ((PopBattery->Flags & POP_CB_NO_BATTERY) ||
        (PopBattery->Flags & POP_CB_PENDING_NEW_BATTERY))
    {
        PopBattery->Mode = POP_CB_READ_TAG_MODE;
    }

    /*
     * We attempted to read the composite tag but it was not assigned, therefore
     * we got kicked here to retry reading the battery tag. However this time
     * we must wait on the Composite Battery driver to signal Battery Class
     * a tag is ready to be assigned.
     */
    if (PopBattery->Flags & POP_CB_WAIT_ON_BATTERY_TAG)
    {
        WaitOnTag = TRUE;
        PopBattery->Mode = POP_CB_READ_TAG_MODE;
    }

    /*
     * Determine the battery operation mode and perform the appropriate
     * power I/O action for the Battery Class driver.
     */
    Mode = PopBattery->Mode;
    switch (Mode)
    {
        case POP_CB_READ_TAG_MODE:
        {
            /* Setup the necessary lengths for the battery information to query */
            DPRINT1("Connect battery in progress, requesting battery tag (Mode -> POP_CB_READ_TAG_MODE)\n");
            InputBufferLength = sizeof(ULONG);
            OutputBufferLength = sizeof(ULONG);

            /* Prepare the I/O buffer for the battery tag to the Battery Class driver */
            IoBuffer = PopReadBatteryTag(WaitOnTag);
            NT_ASSERT(IoBuffer != NULL);

            /* Setup the appropriate I/O control code afor this operation */
            IoControlCode = IOCTL_BATTERY_QUERY_TAG;
            break;
        }

        case POP_CB_QUERY_INFORMATION_MODE:
        {
            /* We have a tag, setup the appropriate lengths to query battery information */
            DPRINT1("Querying battery information in progress (Mode -> POP_CB_QUERY_INFORMATION_MODE)\n");
            InputBufferLength = sizeof(BATTERY_QUERY_INFORMATION);
            OutputBufferLength = sizeof(BATTERY_INFORMATION);

            /* Prepare the I/O buffer for the battery query information to the Battery Class driver */
            IoBuffer = PopReadBatteryInformation(BatteryInfo);
            NT_ASSERT(IoBuffer != NULL);

            /* Setup the appropriate I/O control for this operation */
            IoControlCode = IOCTL_BATTERY_QUERY_INFORMATION;
            break;
        }

        case POP_CB_QUERY_STATUS_MODE:
        {
            /* Setup the appropriate lengths to query battery status */
            DPRINT1("Querying battery status in progress (Mode -> POP_CB_QUERY_STATUS_MODE)\n");
            InputBufferLength = sizeof(BATTERY_WAIT_STATUS);
            OutputBufferLength = sizeof(BATTERY_STATUS);

            /* Prepare the I/O buffer for the battery wait status to the Battery Class driver */
            IoBuffer = PopReadBatteryStatus();
            NT_ASSERT(IoBuffer != NULL);

            /* Setup the appropriate I/O control code for this operation */
            IoControlCode = IOCTL_BATTERY_QUERY_STATUS;
            break;
        }

        case POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE:
        {
            /* Setup the appropriate lengths to query the battery estimation time */
            DPRINT1("Querying battery estimation time in progress (Mode -> POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE)\n");
            InputBufferLength = sizeof(BATTERY_QUERY_INFORMATION);
            OutputBufferLength = sizeof(ULONG);

            /* Prepare the I/O buffer for the battery estimated time to the Battery Class driver */
            IoBuffer = PopReadBatteryInformation(BatteryEstTime);
            NT_ASSERT(IoBuffer != NULL);

            /* Setup the appropriate I/O control for this operation */
            IoControlCode = IOCTL_BATTERY_QUERY_INFORMATION;
            break;
        }

        case POP_CB_QUERY_TEMPERATURE_MODE:
        {
            DPRINT1("Battery query temperature isn't implemented yet\n");
            ASSERT(FALSE);
            break;
        }

        default:
        {
            /*
             * We received an unknown operation mode request of which we have no
             * idea on what to do. This is a bug in our end, so punt the system.
             */
            KeBugCheckEx(INTERNAL_POWER_ERROR,
                         0x301,
                         POP_BATTERY_UNKNOWN_MODE_REQUEST,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)Irp);
        }
    }

    /* Setup the IRP stack parameters based on the requested operation */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpStack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    IrpStack->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    IrpStack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

    /* Setup the system buffer based on the requested operation */
    Irp->AssociatedIrp.SystemBuffer = IoBuffer;

    /*
     * Register the IRP completion CB handler and give the IRP to the composite battery,
     * so that when the Battery Class driver returns back our IRP the completion handler
     * will further process the CB operation event.
     */
    PopBattery->Irp = Irp;
    IoSetCompletionRoutine(Irp,
                           PopBatteryIrpCompletion,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    /*
     * We are going to leave the device policy handler soon, release the lock
     * before dispatching the IRP to the Battery Class driver.
     */
    PopReleasePowerPolicyLock();
    IoCallDriver(DeviceObject, Irp);

    /* Free the work item data as we no longer need it */
    PopFreePool(WorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
}

NTSTATUS
NTAPI
PopDisconnectCompositeBattery(VOID)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    DPRINT1("Disconnecting the composite battery in progress\n");

    /* The policy lock must be owned as we remove the composite battery */
    POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP();

    /* Enforce the AC power policy as the battery disappeared */
    if (PopDefaultPowerPolicy != &PopAcPowerPolicy)
    {
        PopSetDefaultPowerPolicy(PolicyAc);

        /* FIXME: We must recalculate the thermal throttle gauges and system idleness here */
    }

    /* Report to the system that we do not have system batteries globally */
    PopCapabilities.SystemBatteriesPresent = FALSE;

    /* Clear any of the flags that have been set before and enforce the "no battery" flag */
    PopBattery->Flags &= ~(POP_CB_PENDING_NEW_BATTERY | POP_CB_PROCESSING_MODE_REQUEST | POP_CB_WAIT_ON_BATTERY_TAG | POP_CB_REMOVE_BATTERY);
    PopBattery->Flags |= POP_CB_NO_BATTERY;

    /* Free the IRP and take the reference onto the battery device object we held away */
    Irp = PopBattery->Irp;
    IoFreeIrp(Irp);
    PopBattery->Irp = NULL;

    DeviceObject = PopBattery->DeviceObject;
    ObDereferenceObject(DeviceObject);
    PopBattery->DeviceObject = NULL;

    /* Reset the battery operation mode */
    PopBattery->Mode = POP_CB_NO_MODE;

    /* And finally reset all the battery information and status data */
    PopBattery->BatteryTag = 0;
    PopBattery->Temperature = 0;
    RtlZeroMemory(&PopBattery->Status, sizeof(PopBattery->Status));
    RtlZeroMemory(&PopBattery->BattInfo, sizeof(PopBattery->BattInfo));
    PopBattery->EstimatedBatteryTime = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PopConnectCompositeBattery(
    _In_ PDEVICE_OBJECT BatteryDevice)
{
    PAGED_CODE();

    DPRINT1("Connecting with the composite battery (COMPBATT.SYS) for the first time\n");

    /*
     * Report that system batteries are present to the system globally
     * if we have not done it before.
     */
    PopCapabilities.SystemBatteriesPresent = TRUE;

    /*
     * We are connecting with the composite battery, reset the I/O error
     * trigger because we have not even done anything.
     */
    PopBattery->IoError = FALSE;

    /*
     * The system must have been running on AC power at this point of
     * receiving a system battery. Switch the power policy to that of
     * DC policy.
     */
    ASSERT(PopDefaultPowerPolicy == &PopAcPowerPolicy);
    PopSetDefaultPowerPolicy(PolicyDc);

    /*
     * Associate the battery device with the composite battery (CB) kernel
     * structure. The device policy manager will kick the respective battery
     * policy handler to read the battery tag for the first time.
     */
    PopBattery->DeviceObject = BatteryDevice;
    return STATUS_SUCCESS;
}

/* EOF */
