/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG CmBattDebug;
PCALLBACK_OBJECT CmBattPowerCallBackObject;
PVOID CmBattPowerCallBackRegistration;
UNICODE_STRING GlobalRegistryPath;
KTIMER CmBattWakeDpcTimerObject;
KDPC CmBattWakeDpcObject;
PDEVICE_OBJECT AcAdapterPdo;
LARGE_INTEGER CmBattWakeDpcDelay;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CmBattPowerCallBack(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG Action,
                    IN ULONG Value)
{
    BOOLEAN Cancelled;
    PDEVICE_OBJECT DeviceObject;
    if (CmBattDebug & 0x10)
        DbgPrint("CmBattPowerCallBack: action: %d, value: %d \n", Action, Value);

    /* Check if a transition is going to happen */
    if (Action == PO_CB_SYSTEM_STATE_LOCK)
    {
        /* We have just re-entered S0: call the wake DPC in 10 seconds */
        if (Value == 1)
        {
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerCallBack: Calling CmBattWakeDpc after 10 seconds.\n");
            Cancelled = KeSetTimer(&CmBattWakeDpcTimerObject, CmBattWakeDpcDelay, &CmBattWakeDpcObject);
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerCallBack: timerCanceled = %d.\n", Cancelled);
        }
        else if (Value == 0)
        {
            /* We are exiting the S0 state: loop all devices to set the delay flag */
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerCallBack: Delaying Notifications\n");
            for (DeviceObject = DeviceExtension->DeviceObject;
                 DeviceObject;
                 DeviceObject = DeviceObject->NextDevice)
            {
                /* Set the delay flag */
                DeviceExtension = DeviceObject->DeviceExtension;
                DeviceExtension->DelayNotification = TRUE;
            }
        }
        else if (CmBattDebug & 0x10)
        {
            /* Unknown value */
            DbgPrint("CmBattPowerCallBack: unknown argument2 = %08x\n", Value);
        }
    }
}

VOID
NTAPI
CmBattWakeDpc(IN PKDPC Dpc,
              IN PCMBATT_DEVICE_EXTENSION FdoExtension,
              IN PVOID SystemArgument1,
              IN PVOID SystemArgument2)
{
    PDEVICE_OBJECT CurrentObject;
    BOOLEAN AcNotify = FALSE;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    ULONG ArFlag;
    if (CmBattDebug & 2) DbgPrint("CmBattWakeDpc: Entered.\n");

    /* Loop all device objects */
    for (CurrentObject = FdoExtension->DeviceObject;
         CurrentObject;
         CurrentObject = CurrentObject->NextDevice)
    {
        /* Turn delay flag off, we're back in S0 */
        DeviceExtension = CurrentObject->DeviceExtension;
        DeviceExtension->DelayNotification = 0;

        /* Check if this is an AC adapter */
        if (DeviceExtension->FdoType == CmBattAcAdapter)
        {
            /* Was there a pending notify? */
            if (DeviceExtension->ArFlag & CMBATT_AR_NOTIFY)
            {
                /* We'll send a notify on the next pass */
                AcNotify = TRUE;
                DeviceExtension->ArFlag = 0;
                if (CmBattDebug & 0x20)
                    DbgPrint("CmBattWakeDpc: AC adapter notified\n");
            }
        }
    }

    /* Loop the device objects again */
    for (CurrentObject = FdoExtension->DeviceObject;
         CurrentObject;
         CurrentObject = CurrentObject->NextDevice)
    {
        /* Check if this is a battery */
        DeviceExtension = CurrentObject->DeviceExtension;
        if (DeviceExtension->FdoType == CmBattBattery)
        {
            /* Check what ARs are pending */
            ArFlag = DeviceExtension->ArFlag;
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattWakeDpc: Performing delayed ARs: %01x\n", ArFlag);

            /* Insert notification, clear the lock value */
            if (ArFlag & CMBATT_AR_INSERT) InterlockedExchange(&DeviceExtension->ArLockValue, 0);

            /* Removal, clear the battery tag */
            if (ArFlag & CMBATT_AR_REMOVE) DeviceExtension->Tag = 0;

            /* Notification (or AC/DC adapter change from first pass above) */
            if ((ArFlag & CMBATT_AR_NOTIFY) || (AcNotify))
            {
                /* Notify the class driver */
                BatteryClassStatusNotify(DeviceExtension->ClassData);
            }
        }
    }
}

VOID
NTAPI
CmBattNotifyHandler(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG NotifyValue)
{
    ULONG ArFlag;
    PCMBATT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT DeviceObject;

    if (CmBattDebug & (CMBATT_ACPI_ASSERT | CMBATT_PNP_INFO))
        DbgPrint("CmBattNotifyHandler: CmBatt 0x%08x Type %d Number %d Notify Value: %x\n",
                 DeviceExtension,
                 DeviceExtension->FdoType,
                 DeviceExtension->DeviceId,
                 NotifyValue);

    /* Check what kind of notification was received */
    switch (NotifyValue)
    {
        /* ACPI Specification says is sends a "Bus Check" when power source changes */
        case ACPI_BUS_CHECK:

            /* We treat it as possible physical change */
            DeviceExtension->ArFlag |= (CMBATT_AR_NOTIFY | CMBATT_AR_INSERT);
            if ((DeviceExtension->Tag) &&
                (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING)))
                DbgPrint("CmBattNotifyHandler: Received battery #%x insertion, but tag was not invalid.\n",
                         DeviceExtension->DeviceId);
            break;

        /* Status of the battery has changed */
        case ACPI_BATT_NOTIFY_STATUS:

            /* All we'll do is notify the class driver */
            DeviceExtension->ArFlag |= CMBATT_AR_NOTIFY;
            break;

        /* Information on the battery has changed, such as physical presence */
        case ACPI_DEVICE_CHECK:
        case ACPI_BATT_NOTIFY_INFO:

            /* Reset all state and let the class driver re-evaluate it all */
            DeviceExtension->ArFlag |= (CMBATT_AR_NOTIFY |
                                        CMBATT_AR_INSERT |
                                        CMBATT_AR_REMOVE);
            break;

        default:

            if (CmBattDebug & CMBATT_PNP_INFO)
                DbgPrint("CmBattNotifyHandler: Unknown Notify Value: %x\n", NotifyValue);
    }

    /* Check if we're supposed to delay the notification till later */
    if (DeviceExtension->DelayNotification)
    {
        /* We'll handle this when we get a status query later on */
        if (CmBattDebug & CMBATT_PNP_INFO)
            DbgPrint("CmBattNotifyHandler: Notification delayed: ARs = %01x\n",
                      DeviceExtension->ArFlag);
        return;
    }

    /* We're going to handle this now */
    if (CmBattDebug & CMBATT_PNP_INFO)
        DbgPrint("CmBattNotifyHandler: Performing ARs: %01x\n", DeviceExtension->ArFlag);

    /* Check if this is a battery or AC adapter notification */
    if (DeviceExtension->FdoType == CmBattBattery)
    {
        /* Reset the current trip point */
        DeviceExtension->TripPointValue = BATTERY_UNKNOWN_CAPACITY;

        /* Check what ARs have to be done */
        ArFlag = DeviceExtension->ArFlag;

        /* New battery inserted, reset lock value */
        if (ArFlag & CMBATT_AR_INSERT) InterlockedExchange(&DeviceExtension->ArLockValue, 0);

        /* Check if the battery may have been removed */
        if (ArFlag & CMBATT_AR_REMOVE) DeviceExtension->Tag = 0;

        /* Check if there's been any sort of change to the battery */
        if (ArFlag & CMBATT_AR_NOTIFY)
        {
            /* We'll probably end up re-evaluating _BIF and _BST */
            DeviceExtension->NotifySent = TRUE;
            BatteryClassStatusNotify(DeviceExtension->ClassData);
        }
    }
    else if (DeviceExtension->ArFlag & CMBATT_AR_NOTIFY)
    {
        /* The only known notification is AC/DC change. Loop device objects. */
        for (DeviceObject = DeviceExtension->FdoDeviceObject->DriverObject->DeviceObject;
             DeviceObject;
             DeviceObject = DeviceObject->NextDevice)
        {
            /* Is this a battery? */
            FdoExtension = DeviceObject->DeviceExtension;
            if (FdoExtension->FdoType == CmBattBattery)
            {
                /* Send a notification to the class driver */
                FdoExtension->NotifySent = TRUE;
                BatteryClassStatusNotify(FdoExtension->ClassData);
            }
        }
    }

    /* ARs have been processed */
    DeviceExtension->ArFlag = 0;
}

VOID
NTAPI
CmBattUnload(IN PDRIVER_OBJECT DriverObject)
{
    if (CmBattDebug & CMBATT_GENERIC_INFO) DPRINT("CmBattUnload: \n");

    /* Check if we have a registered power callback */
    if (CmBattPowerCallBackObject)
    {
        /* Get rid of it */
        ExUnregisterCallback(CmBattPowerCallBackRegistration);
        ObDereferenceObject(CmBattPowerCallBackObject);
    }

    /* Free the registry buffer if it exists */
    if (GlobalRegistryPath.Buffer) ExFreePool(GlobalRegistryPath.Buffer);

    /* Make sure we don't still have references to the DO */
    if ((DriverObject->DeviceObject) && (CmBattDebug & CMBATT_GENERIC_WARNING))
    {
        DbgPrint("Unload called before all devices removed.\n");
    }
}

/**
 * @brief
 * Retrieves the static information of the battery.
 *
 * @param[in] DeviceExtension
 * A pointer to a Control Method (CM) battery device extension.
 * It is used for debugging purposes.
 *
 * @param[out] UseBix
 * A pointer to a boolean value, returned to caller. This can return
 * TRUE if this machine supports the _BIX method, FALSE otherwise.
 *
 * @param[out] BattInfo
 * A pointer to a structure that contains the static info of the
 * battery. ONLY ONE type of information is filled. See Remarks.
 *
 * @return
 * Returns STATUS_INSUFFICIENT_RESOURCES if there is no enough
 * memory to allocate for the static info buffer. Returns
 * STATUS_SUCCESS if the operation has succeeded. Otherwise an
 * error NTSTATUS code is returned.
 *
 * @remarks
 * It is important to note that a machine can only support one method,
 * _BIX or _BIF. Starting with ACPI 4.0, _BIF has become deprecated.
 * The caller MUST INSPECT the boolean value, ExtendedData, in order
 * to determine if the machine has returned the extended information
 * data or not.
 */
static
NTSTATUS
CmBattGetBattStaticInfo(
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _Out_ PBOOLEAN UseBix,
    _Outptr_ PACPI_BATT_STATIC_INFO *BattInfo)
{
    NTSTATUS Status;
    ACPI_BIF_DATA BifData;
    ACPI_BIX_DATA BixData;
    PACPI_BATT_STATIC_INFO Info;

    /* Allocate pool space for the static information */
    Info = ExAllocatePoolZero(PagedPool,
                              sizeof(*Info),
                              CMBATT_BATT_STATIC_INFO_TAG);
    if (Info == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Assume this machine supports the _BIX method */
    *UseBix = TRUE;

    /* Retrieve extended battery static info from _BIX method */
    Status = CmBattGetBixData(DeviceExtension, &BixData);
    if (!NT_SUCCESS(Status))
    {
        /*
         * It failed. This can be expected because not every machine supports
         * _BIX, especially the older machines which do not support ACPI 4.0.
         * Fallback to _BIF at this point.
         */
        Status = CmBattGetBifData(DeviceExtension, &BifData);
        if (!NT_SUCCESS(Status))
        {
            /* That failed too, time to punt */
            ExFreePoolWithTag(Info, CMBATT_BATT_STATIC_INFO_TAG);
            return Status;
        }

        /* Acknowledge the caller it will be going to use _BIF */
        *UseBix = FALSE;
        Info->BifData = BifData;
    }
    else
    {
        Info->BixData = BixData;
    }

    /* Return the battery static info to caller */
    Info->ExtendedData = *UseBix;
    *BattInfo = Info;
    return Status;
}

/**
 * @brief
 * Verifies the extended battery information (_BIX) and translates
 * such data to the BATTERY_INFORMATION structure.
 *
 * @param[in] DeviceExtension
 * A pointer to a Control Method (CM) battery device extension.
 * It is used to gather _BIX data.
 *
 * @param[in,out] Info
 * A pointer to a structure of which this function fills in
 * battery information that can be by other battery miniport
 * drivers, such as the Composite Battery driver.
 */
static
VOID
CmBattVerifyBixData(
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PBATTERY_INFORMATION Info)
{
    ULONG DesignVoltage;
    ACPI_BIX_DATA BixData = DeviceExtension->BattInfo.BixData;

    /* Copy the battery info data */
    Info->Technology = BixData.BatteryTechnology;
    Info->CycleCount = BixData.CycleCount;
    RtlCopyMemory(Info->Chemistry, BixData.BatteryType, 4);

    /* Check if the power stats are reported in ampere or watts */
    if (BixData.PowerUnit == ACPI_BATT_POWER_UNIT_AMPS)
    {
        /*
         * We have got power stats in milli-ampere but ReactOS expects the values
         * to be reported in milli-watts, so we have to convert them.
         * In order to do so we must expect the design voltage of the battery
         * is not unknown.
         */
        DesignVoltage = BixData.DesignVoltage;
        if ((DesignVoltage != BATTERY_UNKNOWN_VOLTAGE) && (DesignVoltage != 0))
        {
            /* Convert the design capacity */
            Info->DesignedCapacity = CONVERT_BATT_INFO(BixData.DesignCapacity, DesignVoltage);

            /* Convert the full charged capacity */
            Info->FullChargedCapacity = CONVERT_BATT_INFO(BixData.LastFullCapacity, DesignVoltage);

            /* Convert the low capacity alarm (DefaultAlert1) */
            Info->DefaultAlert1 = CONVERT_BATT_INFO(BixData.DesignCapacityLow, DesignVoltage);

            /* Convert the designed capacity warning alarm (DefaultAlert2) */
            Info->DefaultAlert2 = CONVERT_BATT_INFO(BixData.DesignCapacityWarning, DesignVoltage);
        }
        else
        {
            /*
             * Without knowing the nominal designed voltage of the battery
             * we cannot determine the power consumption of this battery.
             */
            Info->DesignedCapacity = BATTERY_UNKNOWN_CAPACITY;
            Info->FullChargedCapacity = BATTERY_UNKNOWN_CAPACITY;
            Info->DefaultAlert1 = BATTERY_UNKNOWN_CAPACITY;
            Info->DefaultAlert2 = BATTERY_UNKNOWN_CAPACITY;
        }
    }
    else
    {
        /* The stats are in milli-watts, use them directly */
        Info->DesignedCapacity = BixData.DesignCapacity;
        Info->FullChargedCapacity = BixData.LastFullCapacity;
        Info->DefaultAlert1 = BixData.DesignCapacityLow;
        Info->DefaultAlert2 = BixData.DesignCapacityWarning;
    }
}

/**
 * @brief
 * Verifies the battery information (_BIF) and translates
 * such data to the BATTERY_INFORMATION structure.
 *
 * @param[in] DeviceExtension
 * A pointer to a Control Method (CM) battery device extension.
 * It is used to gather _BIF data.
 *
 * @param[in,out] Info
 * A pointer to a structure of which this function fills in
 * battery information that can be by other battery miniport
 * drivers, such as the Composite Battery driver.
 */
static
VOID
CmBattVerifyBifData(
    _In_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PBATTERY_INFORMATION Info)
{
    ULONG DesignVoltage;
    ACPI_BIF_DATA BifData = DeviceExtension->BattInfo.BifData;

    /* Copy the battery info data, CycleCount is not supported in _BIF */
    Info->Technology = BifData.BatteryTechnology;
    Info->CycleCount = 0;
    RtlCopyMemory(Info->Chemistry, BifData.BatteryType, 4);

    /* Check if the power stats are reported in ampere or watts */
    if (BifData.PowerUnit == ACPI_BATT_POWER_UNIT_AMPS)
    {
        /*
         * We have got power stats in milli-ampere but ReactOS expects the values
         * to be reported in milli-watts, so we have to convert them.
         * In order to do so we must expect the design voltage of the battery
         * is not unknown.
         */
        DesignVoltage = BifData.DesignVoltage;
        if ((DesignVoltage != BATTERY_UNKNOWN_VOLTAGE) && (DesignVoltage != 0))
        {
            /* Convert the design capacity */
            Info->DesignedCapacity = CONVERT_BATT_INFO(BifData.DesignCapacity, DesignVoltage);

            /* Convert the full charged capacity */
            Info->FullChargedCapacity = CONVERT_BATT_INFO(BifData.LastFullCapacity, DesignVoltage);

            /* Convert the low capacity alarm (DefaultAlert1) */
            Info->DefaultAlert1 = CONVERT_BATT_INFO(BifData.DesignCapacityLow, DesignVoltage);

            /* Convert the designed capacity warning alarm (DefaultAlert2) */
            Info->DefaultAlert2 = CONVERT_BATT_INFO(BifData.DesignCapacityWarning, DesignVoltage);
        }
        else
        {
            /*
             * Without knowing the nominal designed voltage of the battery
             * we cannot determine the power consumption of this battery.
             */
            Info->DesignedCapacity = BATTERY_UNKNOWN_CAPACITY;
            Info->FullChargedCapacity = BATTERY_UNKNOWN_CAPACITY;
            Info->DefaultAlert1 = BATTERY_UNKNOWN_CAPACITY;
            Info->DefaultAlert2 = BATTERY_UNKNOWN_CAPACITY;
        }
    }
    else
    {
        /* The stats are in milli-watts, use them directly */
        Info->DesignedCapacity = BifData.DesignCapacity;
        Info->FullChargedCapacity = BifData.LastFullCapacity;
        Info->DefaultAlert1 = BifData.DesignCapacityLow;
        Info->DefaultAlert2 = BifData.DesignCapacityWarning;
    }
}

NTSTATUS
NTAPI
CmBattVerifyStaticInfo(
    _Inout_ PCMBATT_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG BatteryTag)
{
    NTSTATUS Status;
    BOOLEAN UseBix;
    PACPI_BATT_STATIC_INFO BattInfo;
    PBATTERY_INFORMATION Info = &DeviceExtension->BatteryInformation;

    /* FIXME: This function is not fully implemented, more checks need to be implemented */
    UNREFERENCED_PARAMETER(BatteryTag);

    /* Retrieve the battery static info */
    Status = CmBattGetBattStaticInfo(DeviceExtension, &UseBix, &BattInfo);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the battery information data */
        RtlZeroMemory(Info, sizeof(*Info));
        Info->Capabilities = BATTERY_SYSTEM_BATTERY;

        /* Copy the static information to the device extension of the battery */
        RtlCopyMemory(&DeviceExtension->BattInfo, BattInfo, sizeof(*BattInfo));

        /* Check if the data from _BIX has to be used or not */
        if (UseBix)
        {
            CmBattVerifyBixData(DeviceExtension, Info);
        }
        else
        {
            CmBattVerifyBifData(DeviceExtension, Info);
        }

        /* Free the static information buffer as we already copied it */
        ExFreePoolWithTag(BattInfo, CMBATT_BATT_STATIC_INFO_TAG);
    }

    return Status;
}

NTSTATUS
NTAPI
CmBattOpenClose(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    UCHAR Major;
    ULONG Count;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    PAGED_CODE();
    if (CmBattDebug & CMBATT_GENERIC_INFO) DPRINT("CmBattOpenClose\n");

    /* Grab the device extension and lock it */
    DeviceExtension = DeviceObject->DeviceExtension;
    ExAcquireFastMutex(&DeviceExtension->FastMutex);

    /* Check if someone is trying to open a device that doesn't exist yet */
    Count = DeviceExtension->HandleCount;
    if (Count == 0xFFFFFFFF)
    {
        /* Fail the request */
        Status = STATUS_NO_SUCH_DEVICE;
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Failed (UID = %x)(device being removed).\n",
                     DeviceExtension->Tag);
        }
        goto Complete;
    }

    /* Check if this is an open or close */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Major = IoStackLocation->MajorFunction;
    if (Major == IRP_MJ_CREATE)
    {
        /* Increment the open count */
        DeviceExtension->HandleCount = Count + 1;
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Open (DeviceNumber = %x)(count = %x).\n",
                     DeviceExtension->DeviceId, Count + 1);
        }
    }
    else if (Major == IRP_MJ_CLOSE)
    {
        /* Decrement the open count */
        DeviceExtension->HandleCount = Count - 1;
        if (CmBattDebug & CMBATT_PNP_INFO)
        {
            DbgPrint("CmBattOpenClose: Close (DeviceNumber = %x)(count = %x).\n",
                     DeviceExtension->DeviceId, Count + 1);
        }
    }

Complete:
    /* Release lock and complete request */
    ExReleaseFastMutex(&DeviceExtension->FastMutex);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
CmBattIoctl(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PCMBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    ULONG IoControlCode, OutputBufferLength, InputBufferLength;
    PAGED_CODE();
    if (CmBattDebug & 2) DbgPrint("CmBattIoctl\n");

    /* Acquire the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* It's too late, fail */
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    /* There's nothing to do for an AC adapter */
    if (DeviceExtension->FdoType == CmBattAcAdapter)
    {
        /* Pass it down, and release the remove lock */
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
        return Status;
    }

    /* Send to class driver */
    Status = BatteryClassIoctl(DeviceExtension->ClassData, Irp);
    if (Status == STATUS_NOT_SUPPORTED)
    {
        /* Read IOCTL information from IRP stack */
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
        OutputBufferLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
        InputBufferLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
        if (CmBattDebug & 4)
            DbgPrint("CmBattIoctl: Received  Direct Access IOCTL %x\n", IoControlCode);

        /* Handle internal IOCTLs */
        switch (IoControlCode)
        {
            case IOCTL_BATTERY_QUERY_UNIQUE_ID:

                /* Data is 4 bytes long */
                if (OutputBufferLength == sizeof(ULONG))
                {
                    /* Query it */
                    Status = CmBattGetUniqueId(DeviceExtension->PdoDeviceObject,
                                               Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) Irp->IoStatus.Information = sizeof(ULONG);
                }
                else
                {
                    /* Buffer size invalid */
                    Status = STATUS_INVALID_BUFFER_SIZE;
                }
                break;

            case IOCTL_BATTERY_QUERY_STA:

                /* Data is 4 bytes long */
                if (OutputBufferLength == sizeof(ULONG))
                {
                    /* Query it */
                    Status = CmBattGetStaData(DeviceExtension->PdoDeviceObject,
                                              Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) Irp->IoStatus.Information = sizeof(ULONG);
                }
                else
                {
                    /* Buffer size invalid */
                    Status = STATUS_INVALID_BUFFER_SIZE;
                }
                break;

            case IOCTL_BATTERY_QUERY_PSR:

                /* Data is 4 bytes long */
                if (OutputBufferLength == sizeof(ULONG))
                {
                    /* Do we have an AC adapter? */
                    if (AcAdapterPdo)
                    {
                        /* Query it */
                        Status = CmBattGetPsrData(AcAdapterPdo,
                                                  Irp->AssociatedIrp.SystemBuffer);
                        if (NT_SUCCESS(Status)) Irp->IoStatus.Information = sizeof(ULONG);
                    }
                    else
                    {
                        /* No adapter, just a battery, so fail */
                        Status = STATUS_NO_SUCH_DEVICE;
                    }
                }
                else
                {
                    /* Buffer size invalid */
                    Status = STATUS_INVALID_BUFFER_SIZE;
                }
                break;

            case IOCTL_BATTERY_SET_TRIP_POINT:

                /* Data is 4 bytes long */
                if (InputBufferLength == sizeof(ULONG))
                {
                    /* Query it */
                    Status = CmBattSetTripPpoint(DeviceExtension,
                                                 *(PULONG)Irp->AssociatedIrp.SystemBuffer);
                    Irp->IoStatus.Information = 0;
                }
                else
                {
                    /* Buffer size invalid */
                    Status = STATUS_INVALID_BUFFER_SIZE;
                }
                break;

            case IOCTL_BATTERY_QUERY_BIF_BIX:

                /* Return the battery static information to the caller depending on the supported ACPI method */
                if (DeviceExtension->BattInfo.ExtendedData)
                {
                    if (OutputBufferLength == sizeof(ACPI_BIX_DATA))
                    {
                        /* Query it */
                        Status = CmBattGetBixData(DeviceExtension,
                                                  Irp->AssociatedIrp.SystemBuffer);
                        if (NT_SUCCESS(Status))
                        {
                            Irp->IoStatus.Information = sizeof(ACPI_BIX_DATA);
                        }
                    }
                    else
                    {
                        /* Buffer size invalid */
                        Status = STATUS_INVALID_BUFFER_SIZE;
                    }
                }
                else
                {
                    if (OutputBufferLength == sizeof(ACPI_BIF_DATA))
                    {
                        /* Query it */
                        Status = CmBattGetBifData(DeviceExtension,
                                                  Irp->AssociatedIrp.SystemBuffer);
                        if (NT_SUCCESS(Status))
                        {
                            Irp->IoStatus.Information = sizeof(ACPI_BIF_DATA);
                        }
                    }
                    else
                    {
                        /* Buffer size invalid */
                        Status = STATUS_INVALID_BUFFER_SIZE;
                    }
                }
                break;

            case IOCTL_BATTERY_QUERY_BST:

                /* Data is 16 bytes long */
                if (OutputBufferLength == sizeof(ACPI_BST_DATA))
                {
                    /* Query it */
                    Status = CmBattGetBstData(DeviceExtension,
                                              Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) Irp->IoStatus.Information = sizeof(ACPI_BST_DATA);
                }
                else
                {
                    /* Buffer size invalid */
                    Status = STATUS_INVALID_BUFFER_SIZE;
                }
                break;

            default:

                /* Unknown, let us pass it on to ACPI */
                if (CmBattDebug & 0xC)
                    DbgPrint("CmBattIoctl: Unknown IOCTL %x\n", IoControlCode);
                break;
        }

        /* Did someone pick it up? */
        if (Status != STATUS_NOT_SUPPORTED)
        {
            /* Complete the request */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else
        {
            /* Still unsupported, try ACPI */
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
    }

    /* Release the remove lock and return status */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    return Status;
}

NTSTATUS
NTAPI
CmBattQueryTag(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
               OUT PULONG Tag)
{
    PDEVICE_OBJECT PdoDevice;
    ULONG StaData;
    NTSTATUS Status;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
      DbgPrint("CmBattQueryTag - Tag (%d), Battery %x, Device %d\n",
                *Tag, DeviceExtension, DeviceExtension->DeviceId);

    /* Get PDO and clear notification flag */
    PdoDevice = DeviceExtension->PdoDeviceObject;
    DeviceExtension->NotifySent = 0;

    /* Get _STA from PDO (we need the machine status, not the battery status) */
    Status = CmBattGetStaData(PdoDevice, &StaData);
    if (NT_SUCCESS(Status))
    {
        /* Is a battery present? */
        if (StaData & ACPI_STA_BATTERY_PRESENT)
        {
            /* Do we not have a tag yet? */
            if (DeviceExtension->Tag == BATTERY_TAG_INVALID)
            {
                /* Set the new tag value, reset tags if we reached the maximum */
                if (++DeviceExtension->TagData == BATTERY_TAG_INVALID)
                    DeviceExtension->TagData = 1;
                DeviceExtension->Tag = DeviceExtension->TagData;
                if (CmBattDebug & CMBATT_GENERIC_INFO)
                    DbgPrint("CmBattQueryTag - New Tag: (%d)\n", DeviceExtension->Tag);

                /* Reset trip point data */
                DeviceExtension->TripPointOld = 0;
                DeviceExtension->TripPointValue = BATTERY_UNKNOWN_CAPACITY;

                /* Clear AR lock and set new interrupt time */
                InterlockedExchange(&DeviceExtension->ArLockValue, 0);
                DeviceExtension->InterruptTime = KeQueryInterruptTime();
            }
        }
        else
        {
            /* No battery, so no tag */
            DeviceExtension->Tag = BATTERY_TAG_INVALID;
            Status = STATUS_NO_SUCH_DEVICE;
        }
    }

    /* Return the tag and status result */
    *Tag = DeviceExtension->Tag;
    if (CmBattDebug & CMBATT_ACPI_WARNING)
      DbgPrint("CmBattQueryTag: Returning Tag: 0x%x, status 0x%x\n", *Tag, Status);
    return Status;
}

NTSTATUS
NTAPI
CmBattDisableStatusNotify(IN PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PAGED_CODE();
    if (CmBattDebug & 0xA) DbgPrint("CmBattDisableStatusNotify\n");

    /* Do we have a trip point */
    if (DeviceExtension->TripPointSet)
    {
        /* Is there a current value set? */
        if (DeviceExtension->TripPointValue)
        {
            /* Reset it back to 0 */
            DeviceExtension->TripPointValue = 0;
            Status = CmBattSetTripPpoint(DeviceExtension, 0);
            if (!NT_SUCCESS(Status))
            {
                /* If it failed, set unknown/invalid value */
                DeviceExtension->TripPointValue = BATTERY_UNKNOWN_CAPACITY;
                if (CmBattDebug & 8)
                    DbgPrint("CmBattDisableStatusNotify: SetTripPoint failed - %x\n", Status);
            }
        }
        else
        {
            /* No trip point set, so this is a successful no-op */
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Nothing we can do */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattSetStatusNotify(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                      IN ULONG BatteryTag,
                      IN PBATTERY_NOTIFY BatteryNotify)
{
    NTSTATUS Status;
    ACPI_BST_DATA BstData;
    ULONG PowerUnit, Capacity, NewTripPoint, TripPoint, DesignVoltage;
    BOOLEAN Charging;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
        DbgPrint("CmBattSetStatusNotify: Tag (%d) Target(0x%x)\n",
                 BatteryTag, BatteryNotify->LowCapacity);

    /* Update any ACPI evaluations */
    Status = CmBattVerifyStaticInfo(DeviceExtension, BatteryTag);
    if (!NT_SUCCESS(Status)) return Status;

    /* Trip point not supported, fail */
    if (!DeviceExtension->TripPointSet) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Are both capacities known? */
    if ((BatteryNotify->HighCapacity == BATTERY_UNKNOWN_CAPACITY) ||
        (BatteryNotify->LowCapacity == BATTERY_UNKNOWN_CAPACITY))
    {
        /* We can't set trip points without these */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattSetStatusNotify: Failing request because of BATTERY_UNKNOWN_CAPACITY.\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Is the battery charging? */
    Charging = DeviceExtension->BstData.State & ACPI_BATT_STAT_CHARGING;
    if (Charging)
    {
        /* Then the trip point is when we hit the cap */
        Capacity = BatteryNotify->HighCapacity;
        NewTripPoint = BatteryNotify->HighCapacity;
    }
    else
    {
        /* Otherwise it's when we discharge to the bottom */
        Capacity = BatteryNotify->LowCapacity;
        NewTripPoint = BatteryNotify->LowCapacity;
    }

    /* Is this machine supporting _BIX or _BIF? */
    if (DeviceExtension->BattInfo.ExtendedData)
    {
        PowerUnit = DeviceExtension->BattInfo.BixData.PowerUnit;
        DesignVoltage = DeviceExtension->BattInfo.BixData.DesignVoltage;
    }
    else
    {
        PowerUnit = DeviceExtension->BattInfo.BifData.PowerUnit;
        DesignVoltage = DeviceExtension->BattInfo.BifData.DesignVoltage;
    }

    /* Do we have data in Amps or Watts? */
    if (PowerUnit == ACPI_BATT_POWER_UNIT_AMPS)
    {
        /* We need the voltage to do the conversion */
        if ((DesignVoltage != BATTERY_UNKNOWN_VOLTAGE) && (DesignVoltage))
        {
            /* Convert from mAh into Ah */
            TripPoint = 1000 * NewTripPoint;
            if (Charging)
            {
                /* Scale the high trip point */
                NewTripPoint = (TripPoint + 500) / DesignVoltage + ((TripPoint + 500) % DesignVoltage != 0);
            }
            else
            {
                /* Scale the low trip point */
                NewTripPoint = (TripPoint - 500) / DesignVoltage - ((TripPoint - 500) % DesignVoltage == 0);
            }
        }
        else
        {
            /* Without knowing the voltage, Amps are not enough data on consumption */
            Status = STATUS_NOT_SUPPORTED;
            if (CmBattDebug & CMBATT_ACPI_WARNING)
                DbgPrint("CmBattSetStatusNotify: Can't calculate BTP, DesignVoltage = 0x%08x\n",
                        DesignVoltage);
        }
    }
    else if (Charging)
    {
        /* Make it trip just one past the charge cap */
        ++NewTripPoint;
    }
    else if (NewTripPoint > 0)
    {
        /* Make it trip just one below the drain cap */
        --NewTripPoint;
    }

    /* Do we actually have a new trip point? */
    if (NewTripPoint == DeviceExtension->TripPointValue)
    {
        /* No, so there is no work to be done */
        if (CmBattDebug & CMBATT_GENERIC_STATUS)
            DbgPrint("CmBattSetStatusNotify: Keeping original setting: %X\n", DeviceExtension->TripPointValue);
        return STATUS_SUCCESS;
    }

    /* Set the trip point with ACPI and check for success */
    DeviceExtension->TripPointValue = NewTripPoint;
    Status = CmBattSetTripPpoint(DeviceExtension, NewTripPoint);
    if (!(NewTripPoint) && (Capacity)) Status = STATUS_NOT_SUPPORTED;
    if (!NT_SUCCESS(Status))
    {
        /* We failed to set the trip point, or there wasn't one settable */
        DeviceExtension->TripPointValue = BATTERY_UNKNOWN_CAPACITY;
        if (CmBattDebug & (CMBATT_GENERIC_WARNING | CMBATT_ACPI_WARNING))
            DbgPrint("CmBattSetStatusNotify: SetTripPoint failed - %x\n", Status);
        return Status;
    }

    /* Read the new BST data to see the latest state */
    Status = CmBattGetBstData(DeviceExtension, &BstData);
    if (!NT_SUCCESS(Status))
    {
        /* We'll return failure to the caller */
        if (CmBattDebug & (CMBATT_GENERIC_WARNING | CMBATT_ACPI_WARNING))
            DbgPrint("CmBattSetStatusNotify: GetBstData - %x\n", Status);
    }
    else if ((Charging) && (BstData.RemainingCapacity >= NewTripPoint))
    {
        /* We are charging and our capacity is past the trip point, so trip now */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattSetStatusNotify: Trip point already crossed (1): TP = %08x, remaining capacity = %08x\n",
                     NewTripPoint, BstData.RemainingCapacity);
        CmBattNotifyHandler(DeviceExtension, ACPI_BATT_NOTIFY_STATUS);
    }
    else if ((BstData.RemainingCapacity) && (Capacity))
    {
        /* We are discharging, and our capacity is below the trip point, trip now */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattSetStatusNotify: Trip point already crossed (1): TP = %08x, remaining capacity = %08x\n",
                     NewTripPoint, BstData.RemainingCapacity);
        CmBattNotifyHandler(DeviceExtension, ACPI_BATT_NOTIFY_STATUS);
    }

    /* All should've went well if we got here, unless BST failed... return! */
    if (CmBattDebug & CMBATT_GENERIC_STATUS)
          DbgPrint("CmBattSetStatusNotify: Want %X CurrentCap %X\n",
                    Capacity, DeviceExtension->RemainingCapacity);
    if (CmBattDebug & CMBATT_ACPI_WARNING)
          DbgPrint("CmBattSetStatusNotify: Set to: [%#08lx][%#08lx][%#08lx] Status %lx\n",
                    BatteryNotify->PowerState,
                    BatteryNotify->LowCapacity,
                    BatteryNotify->HighCapacity,
                    Status);
    return Status;
}

NTSTATUS
NTAPI
CmBattGetBatteryStatus(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       IN ULONG Tag)
{
    ULONG PsrData = 0;
    NTSTATUS Status;
    BOOLEAN WasDischarging;
    ULONG BstState;
    ULONG PowerUnit;
    ULONG DesignVoltage, PresentRate, RemainingCapacity;
    PAGED_CODE();
    if (CmBattDebug & CMBATT_GENERIC_INFO)
        DbgPrint("CmBattGetBatteryStatus - CmBatt (%08x) Tag (%d)\n", DeviceExtension, Tag);

    /* Validate ACPI data */
    Status = CmBattVerifyStaticInfo(DeviceExtension, Tag);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check for delayed status notifications */
    if (DeviceExtension->DelayNotification)
    {
        /* Process them now and don't do any other work */
        CmBattNotifyHandler(DeviceExtension, ACPI_BATT_NOTIFY_STATUS);
        return Status;
    }

    /* Get _BST from ACPI */
    Status = CmBattGetBstData(DeviceExtension, &DeviceExtension->BstData);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        InterlockedExchange(&DeviceExtension->ArLockValue, 0);
        return Status;
    }

    /* Remember if the battery was discharging at the time of querying new status */
    WasDischarging = !!(DeviceExtension->State & BATTERY_DISCHARGING);

    /* Clear current BST information */
    DeviceExtension->State = 0;
    DeviceExtension->RemainingCapacity = 0;
    DeviceExtension->PresentVoltage = 0;
    DeviceExtension->Rate = 0;

    /* Get battery state */
    BstState = DeviceExtension->BstData.State;

    /* Is the battery both charging and discharging? */
    if ((BstState & ACPI_BATT_STAT_DISCHARG) && (BstState & ACPI_BATT_STAT_CHARGING) &&
        (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING)))
            DbgPrint("************************ ACPI BIOS BUG ********************\n* "
                     "CmBattGetBatteryStatus: Invalid state: _BST method returned 0x%08x for Battery State.\n"
                     "* One battery cannot be charging and discharging at the same time.\n",
                     BstState);

    /* Is the battery discharging? */
    if (BstState & ACPI_BATT_STAT_DISCHARG)
    {
        /* Set power state and check if it just started discharging now */
        DeviceExtension->State |= BATTERY_DISCHARGING;
        if (!WasDischarging)
        {
            /* The battery is discharging now and not before, remember the time when the state changed */
            DeviceExtension->InterruptTime = KeQueryInterruptTime();
        }
    }
    else if (BstState & ACPI_BATT_STAT_CHARGING)
    {
        /* Battery is charging, update power state */
        DeviceExtension->State |= (BATTERY_CHARGING | BATTERY_POWER_ON_LINE);
    }

    /* Is the battery in a critical state? */
    if (BstState & ACPI_BATT_STAT_CRITICAL) DeviceExtension->State |= BATTERY_CRITICAL;

    /* Read the voltage data */
    DeviceExtension->PresentVoltage = DeviceExtension->BstData.PresentVoltage;

    /* Check if we have an A/C adapter */
    if (AcAdapterPdo)
    {
        /* Query information on it */
        CmBattGetPsrData(AcAdapterPdo, &PsrData);
    }
    else
    {
        /* Otherwise, check if the battery is charging */
        if (BstState & ACPI_BATT_STAT_CHARGING)
        {
            /* Then we'll assume there's a charger */
            PsrData = 1;
        }
        else
        {
            /* Assume no charger */
            PsrData = 0;
        }
    }

    /* Is there a charger? */
    if (PsrData)
    {
        /* Set the power state flag to reflect this */
        DeviceExtension->State |= BATTERY_POWER_ON_LINE;
        if (CmBattDebug & (CMBATT_GENERIC_INFO | CMBATT_GENERIC_STATUS))
            DbgPrint("CmBattGetBatteryStatus: AC adapter is connected\n");
    }
    else if (CmBattDebug & (CMBATT_GENERIC_INFO | CMBATT_GENERIC_STATUS))
    {
        DbgPrint("CmBattGetBatteryStatus: AC adapter is NOT connected\n");
    }

    /* Is this machine supporting _BIX or _BIF? */
    if (DeviceExtension->BattInfo.ExtendedData)
    {
        PowerUnit = DeviceExtension->BattInfo.BixData.PowerUnit;
        DesignVoltage = DeviceExtension->BattInfo.BixData.DesignVoltage;
    }
    else
    {
        PowerUnit = DeviceExtension->BattInfo.BifData.PowerUnit;
        DesignVoltage = DeviceExtension->BattInfo.BifData.DesignVoltage;
    }

    /* Get some data we'll need */
    PresentRate = DeviceExtension->BstData.PresentRate;
    RemainingCapacity = DeviceExtension->BstData.RemainingCapacity;

    /* Check if we have battery data in Watts instead of Amps */
    if (PowerUnit == ACPI_BATT_POWER_UNIT_WATTS)
    {
        /* Get the data from the BST */
        DeviceExtension->RemainingCapacity = RemainingCapacity;
        DeviceExtension->Rate = PresentRate;

        /* Check if the rate is invalid */
        if (PresentRate > CM_MAX_VALUE)
        {
            /* Set an unknown rate and don't touch the old value */
            DeviceExtension->Rate = BATTERY_UNKNOWN_RATE;
            if ((PresentRate != CM_UNKNOWN_VALUE) && (CmBattDebug & CMBATT_ACPI_WARNING))
            {
                DbgPrint("CmBattGetBatteryStatus - Rate is greater than CM_MAX_VALUE\n");
                DbgPrint("----------------------   PresentRate = 0x%08x\n", PresentRate);
            }
        }
    }
    else if ((DesignVoltage != CM_UNKNOWN_VALUE) && (DesignVoltage != 0)) // Same as doing PowerUnit == ACPI_BATT_POWER_UNIT_AMPS
    {
        /* We have voltage data, what about capacity? */
        if (RemainingCapacity == CM_UNKNOWN_VALUE)
        {
            /* Unable to calculate it */
            DeviceExtension->RemainingCapacity = BATTERY_UNKNOWN_CAPACITY;
            if (CmBattDebug & CMBATT_ACPI_WARNING)
            {
                DbgPrint("CmBattGetBatteryStatus - Can't calculate RemainingCapacity \n");
                DbgPrint("----------------------   RemainingCapacity = CM_UNKNOWN_VALUE\n");
            }
        }
        else
        {
            /* Compute the capacity with the information we have */
            DeviceExtension->RemainingCapacity = (DesignVoltage * RemainingCapacity + 500) / 1000;
        }

        /* Check if we have a rate */
        if (PresentRate != CM_UNKNOWN_VALUE)
        {
            /* Make sure the rate isn't too large */
            if (PresentRate > (-500 / DesignVoltage))
            {
                /* It is, so set unknown state */
                DeviceExtension->Rate = BATTERY_UNKNOWN_RATE;
                if (CmBattDebug & CMBATT_ACPI_WARNING)
                {
                    DbgPrint("CmBattGetBatteryStatus - Can't calculate Rate \n");
                    DbgPrint("----------------------   Overflow: PresentRate = 0x%08x\n", PresentRate);
                }
            }

            /* Compute the rate */
            DeviceExtension->Rate = (PresentRate * DesignVoltage + 500) / 1000;
        }
        else
        {
            /* We don't have a rate, so set unknown value */
            DeviceExtension->Rate = BATTERY_UNKNOWN_RATE;
            if (CmBattDebug & CMBATT_ACPI_WARNING)
            {
                DbgPrint("CmBattGetBatteryStatus - Can't calculate Rate \n");
                DbgPrint("----------------------   Present Rate = CM_UNKNOWN_VALUE\n");
            }
        }
    }
    else
    {
        /* We have no rate, and no capacity, set unknown values */
        DeviceExtension->Rate = BATTERY_UNKNOWN_RATE;
        DeviceExtension->RemainingCapacity = BATTERY_UNKNOWN_CAPACITY;
        if (CmBattDebug & CMBATT_ACPI_WARNING)
        {
            DbgPrint("CmBattGetBatteryStatus - Can't calculate RemainingCapacity and Rate \n");
            DbgPrint("----------------------   DesignVoltage = 0x%08x\n", DesignVoltage);
        }
    }

    /* Check if we have an unknown rate */
    if (DeviceExtension->Rate == BATTERY_UNKNOWN_RATE)
    {
        /* The battery is discharging but we don't know by how much... this is bad! */
        if ((BstState & ACPI_BATT_STAT_DISCHARG) &&
            (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_WARNING)))
            DbgPrint("CmBattGetBatteryStatus: battery rate is unknown when battery is not charging!\n");
    }
    else if (DeviceExtension->State & BATTERY_DISCHARGING)
    {
        /* The battery is discharging, so treat the rate as a negative rate */
        DeviceExtension->Rate = -(LONG)DeviceExtension->Rate;
    }
    else if (!(DeviceExtension->State & BATTERY_CHARGING) && (DeviceExtension->Rate))
    {
        /* We are not charging, not discharging, but have a rate? Ignore it! */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattGetBatteryStatus: battery is not charging or discharging, but rate = %x\n",
                     DeviceExtension->Rate);
        DeviceExtension->Rate = 0;
    }

    /* Done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattQueryInformation(IN PCMBATT_DEVICE_EXTENSION FdoExtension,
                       IN ULONG Tag,
                       IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
                       IN OPTIONAL LONG AtRate,
                       IN PVOID Buffer,
                       IN ULONG BufferLength,
                       OUT PULONG ReturnedLength)
{
    NTSTATUS Status;
    PVOID QueryData = NULL;
    ULONG QueryLength = 0;
    ULONG RemainingTime = 0;
    ANSI_STRING TempString;
    UNICODE_STRING TempString2;
    WCHAR InfoBuffer[256];
    WCHAR TempBuffer[256];
    UNICODE_STRING InfoString;
    ULONG RemainingCapacity;
    BATTERY_REPORTING_SCALE BatteryReportingScale[2];
    LONG Rate;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryInformation - Tag (%d) Device %d, Informationlevel %d\n",
                 Tag,
                 FdoExtension->DeviceId,
                 InfoLevel);

    /* Check ACPI Data */
    Status = CmBattVerifyStaticInfo(FdoExtension, Tag);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check what caller wants */
    switch (InfoLevel)
    {
        case BatteryInformation:
            /* Just return our static information */
            QueryData = &FdoExtension->BatteryInformation;
            QueryLength = sizeof(BATTERY_INFORMATION);
            break;

        case BatteryGranularityInformation:

            /* Return our static information, we have two scales */
            BatteryReportingScale[0].Granularity = FdoExtension->BatteryCapacityGranularity1;
            BatteryReportingScale[0].Capacity = FdoExtension->BatteryInformation.DefaultAlert1;
            BatteryReportingScale[1].Granularity = FdoExtension->BatteryCapacityGranularity2;
            BatteryReportingScale[1].Capacity = FdoExtension->BatteryInformation.DesignedCapacity;
            QueryData = BatteryReportingScale;
            QueryLength = sizeof(BATTERY_REPORTING_SCALE) * 2;
            break;

        case BatteryEstimatedTime:

            /* Check if it's been more than 15 seconds since the last change */
            if (KeQueryInterruptTime() > (FdoExtension->InterruptTime + CMBATT_DISCHARGE_TIME))
            {
                /* Get new battery status */
                CmBattGetBatteryStatus(FdoExtension, FdoExtension->Tag);

                /* If the caller didn't specify a rate, use our static one */
                Rate = AtRate;
                if (!Rate) Rate = FdoExtension->Rate;

                /* If we don't have a valid negative rate, use unknown value */
                if (Rate >= 0) Rate = BATTERY_UNKNOWN_RATE;

                /* Grab the remaining capacity */
                RemainingCapacity = FdoExtension->RemainingCapacity;

                /* Default time to unknown if we fail the request later */
                RemainingTime = BATTERY_UNKNOWN_TIME;

                /* See if we don't know one or the other */
                if ((Rate == BATTERY_UNKNOWN_RATE) ||
                    (RemainingCapacity == BATTERY_UNKNOWN_CAPACITY))
                {
                    /* If the battery is discharging, we can't give out a time */
                    if ((FdoExtension->BstData.State & ACPI_BATT_STAT_DISCHARG) &&
                        (CmBattDebug & CMBATT_GENERIC_WARNING))
                            DbgPrint("CmBattQueryInformation: Can't calculate EstimatedTime.\n");

                    /* Check if we don't have a rate and capacity is going down */
                    if ((FdoExtension->Rate == BATTERY_UNKNOWN_RATE) &&
                        (FdoExtension->BstData.State & ACPI_BATT_STAT_DISCHARG))
                    {
                        /* We have to fail, since we lack data */
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        if (CmBattDebug & CMBATT_GENERIC_WARNING)
                            DbgPrint("----------------------   PresentRate = BATTERY_UNKNOWN_RATE\n");
                    }

                    /* If we don't have capacity, the rate is useless */
                    if (RemainingCapacity == BATTERY_UNKNOWN_CAPACITY)
                    {
                        /* We have to fail the request */
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        if (CmBattDebug & CMBATT_GENERIC_WARNING)
                            DbgPrint("----------------------   RemainingCapacity = BATTERY_UNKNOWN_CAPACITY\n");
                    }
                }
                else
                {
                    /* We have data, but is it valid? */
                    if (RemainingCapacity > CMBATT_CAPACITY_BOGUS)
                    {
                        /* The capacity seems bogus, so don't use it */
                        if (CmBattDebug & CMBATT_ACPI_WARNING)
                            DbgPrint("CmBattQueryInformation: Data Overflow in calculating Remaining Capacity.\n");
                    }
                    else
                    {
                        /* Compute the remaining time in seconds, based on rate */
                        RemainingTime = (RemainingCapacity * 3600) / -Rate;
                    }
                }
            }
            else
            {
                RemainingTime = BATTERY_UNKNOWN_TIME;
            }

            /* Return the remaining time */
            QueryData = &RemainingTime;
            QueryLength = sizeof(ULONG);
            break;

        case BatteryDeviceName:

            /* Build the model number string */
            if (FdoExtension->BattInfo.ExtendedData)
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BixData.ModelNumber);
            }
            else
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BifData.ModelNumber);
            }

            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);
            Status = RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);

            /* Return the unicode buffer */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
            break;

        case BatteryTemperature:
        case BatteryManufactureDate:

            /* We don't support these */
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case BatteryManufactureName:

            /* Build the OEM info string */
            if (FdoExtension->BattInfo.ExtendedData)
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BixData.OemInfo);
            }
            else
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BifData.OemInfo);
            }

            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);
            Status = RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);

            /* Return the unicode buffer */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
            break;

        case BatteryUniqueID:

            /* Build the serial number string */
            if (FdoExtension->BattInfo.ExtendedData)
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BixData.SerialNumber);
            }
            else
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BifData.SerialNumber);
            }

            /* Convert it to Unicode */
            InfoString.Buffer = InfoBuffer;
            InfoString.MaximumLength = sizeof(InfoBuffer);
            RtlAnsiStringToUnicodeString(&InfoString, &TempString, 0);

            /* Setup a temporary string for concatenation */
            TempString2.Buffer = TempBuffer;
            TempString2.MaximumLength = sizeof(TempBuffer);

            /* Check if there's an OEM string */
            if ((FdoExtension->BattInfo.ExtendedData && FdoExtension->BattInfo.BixData.OemInfo[0]) ||
                FdoExtension->BattInfo.BifData.OemInfo[0])
            {
                /* Build the OEM info string */
                if (FdoExtension->BattInfo.ExtendedData)
                {
                    RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BixData.OemInfo);
                }
                else
                {
                    RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BifData.OemInfo);
                }

                /* Convert it to Unicode and append it */
                RtlAnsiStringToUnicodeString(&TempString2, &TempString, 0);
                RtlAppendUnicodeStringToString(&InfoString, &TempString2);
            }

            /* Build the model number string */
            if (FdoExtension->BattInfo.ExtendedData)
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BixData.ModelNumber);
            }
            else
            {
                RtlInitAnsiString(&TempString, FdoExtension->BattInfo.BifData.ModelNumber);
            }

            /* Convert it to Unicode and append it */
            RtlAnsiStringToUnicodeString(&TempString2, &TempString, 0);
            RtlAppendUnicodeStringToString(&InfoString, &TempString2);

            /* Return the final appended string */
            QueryData = InfoString.Buffer;
            QueryLength = InfoString.Length;
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
    return Status;
}

NTSTATUS
NTAPI
CmBattQueryStatus(IN PCMBATT_DEVICE_EXTENSION DeviceExtension,
                  IN ULONG Tag,
                  IN PBATTERY_STATUS BatteryStatus)
{
    NTSTATUS Status;
    PAGED_CODE();
    if (CmBattDebug & (CMBATT_ACPI_WARNING | CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryStatus - Tag (%d) Device %x\n", Tag, DeviceExtension->DeviceId);

    /* Query ACPI information */
    Status = CmBattGetBatteryStatus(DeviceExtension, Tag);
    if (NT_SUCCESS(Status))
    {
        BatteryStatus->PowerState = DeviceExtension->State;
        BatteryStatus->Capacity = DeviceExtension->RemainingCapacity;
        BatteryStatus->Voltage = DeviceExtension->PresentVoltage;
        BatteryStatus->Rate = DeviceExtension->Rate;
    }

    /* Return status */
    if (CmBattDebug & (CMBATT_GENERIC_INFO))
        DbgPrint("CmBattQueryStatus: Returning [%#08lx][%#08lx][%#08lx][%#08lx]\n",
                 BatteryStatus->PowerState,
                 BatteryStatus->Capacity,
                 BatteryStatus->Voltage,
                 BatteryStatus->Rate);
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PDRIVER_EXTENSION DriverExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CallbackName;

    /* Allocate registry path */
    GlobalRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    GlobalRegistryPath.Length = RegistryPath->Length;
    GlobalRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      GlobalRegistryPath.MaximumLength,
                                                      'MtaB');
    if (!GlobalRegistryPath.Buffer)
    {
        /* Fail if we're out of memory this early */
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBatt: Couldn't allocate pool for registry path.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Buffer allocated, copy the string */
    RtlCopyUnicodeString(&GlobalRegistryPath, RegistryPath);
    if (CmBattDebug & CMBATT_GENERIC_INFO)
        DbgPrint("CmBatt DriverEntry - Obj (%08x) Path \"%wZ\"\n",
                 DriverObject,
                 RegistryPath);

    /* Setup the major dispatchers */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CmBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CmBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CmBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = CmBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = CmBattPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CmBattSystemControl;

    /* And the unload routine */
    DriverObject->DriverUnload = CmBattUnload;

    /* And the add device routine */
    DriverExtension = DriverObject->DriverExtension;
    DriverExtension->AddDevice = CmBattAddDevice;

    /* Create a power callback */
    RtlInitUnicodeString(&CallbackName, L"\\Callback\\PowerState");
    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ExCreateCallback(&CmBattPowerCallBackObject, &ObjectAttributes, 0, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* No callback, fail */
        CmBattPowerCallBackObject = 0;
        if (CmBattDebug & CMBATT_GENERIC_WARNING)
            DbgPrint("CmBattRegisterPowerCallBack: failed status=0x%08x\n", Status);
    }
    else
    {
        /* Register the power callback now */
        CmBattPowerCallBackRegistration = ExRegisterCallback(CmBattPowerCallBackObject,
                                                             (PVOID)CmBattPowerCallBack,
                                                             DriverObject);
        if (CmBattPowerCallBackRegistration)
        {
            /* Last thing: setup our DPC and timer for battery wake */
            KeInitializeDpc(&CmBattWakeDpcObject, (PVOID)CmBattWakeDpc, DriverObject);
            KeInitializeTimer(&CmBattWakeDpcTimerObject);
        }
        else
        {
            ObDereferenceObject(CmBattPowerCallBackObject);
            if (CmBattDebug & CMBATT_GENERIC_WARNING)
                DbgPrint("CmBattRegisterPowerCallBack: ExRegisterCallback failed.\n");
        }

        /* All good */
        Status = STATUS_SUCCESS;
    }

    /* Return failure or success */
    return Status;
}

/* EOF */
