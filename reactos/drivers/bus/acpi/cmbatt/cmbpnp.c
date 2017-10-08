/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbpnp.c
 * PURPOSE:         Plug-and-Play IOCTL/IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CmBattWaitWakeLoop(IN PDEVICE_OBJECT DeviceObject,
                   IN UCHAR MinorFunction,
                   IN POWER_STATE PowerState,
                   IN PVOID Context,
                   IN PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS Status;
    PCMBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    if (CmBattDebug & 0x20) DbgPrint("CmBattWaitWakeLoop: Entered.\n");

    /* Check for success */
    if ((NT_SUCCESS(IoStatusBlock->Status)) && (DeviceExtension->WaitWakeEnable))
    {
        /* Request a new power IRP */
        if (CmBattDebug & 2) DbgPrint("CmBattWaitWakeLoop: completed successfully\n");
        Status = PoRequestPowerIrp(DeviceObject,
                                   MinorFunction,
                                   PowerState,
                                   CmBattWaitWakeLoop,
                                   Context,
                                   &DeviceExtension->PowerIrp);
        if (CmBattDebug & 2)
            DbgPrint("CmBattWaitWakeLoop: PoRequestPowerIrp: status = 0x%08x.\n",
                     Status);
    }
    else
    {
        /* Clear the power IRP, we failed */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattWaitWakeLoop: failed: status = 0x%08x.\n",
                     IoStatusBlock->Status);
        DeviceExtension->PowerIrp = NULL;
    }
}

NTSTATUS
NTAPI
CmBattIoCompletion(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PKEVENT Event)
{
    if (CmBattDebug & 2) DbgPrint("CmBattIoCompletion: Event (%x)\n", Event);

    /* Set the completion event */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CmBattGetAcpiInterfaces(IN PDEVICE_OBJECT DeviceObject,
                        IN OUT PACPI_INTERFACE_STANDARD AcpiInterface)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    KEVENT Event;

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, 0);
    if (!Irp)
    {
        /* Fail */
        if (CmBattDebug & 0xC)
          DbgPrint("CmBattGetAcpiInterfaces: Failed to allocate Irp\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set default error code */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    /* Build the query */
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->MajorFunction = IRP_MJ_PNP;
    IoStackLocation->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStackLocation->Parameters.QueryInterface.InterfaceType = &GUID_ACPI_INTERFACE_STANDARD;
    IoStackLocation->Parameters.QueryInterface.Size = sizeof(ACPI_INTERFACE_STANDARD);
    IoStackLocation->Parameters.QueryInterface.Version = 1;
    IoStackLocation->Parameters.QueryInterface.Interface = (PINTERFACE)AcpiInterface;
    IoStackLocation->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    /* Set default ACPI interface data */
    AcpiInterface->Size = sizeof(ACPI_INTERFACE_STANDARD);
    AcpiInterface->Version = 1;

    /* Initialize our wait event */
    KeInitializeEvent(&Event, SynchronizationEvent, 0);

    /* Set the completion routine */
    IoSetCompletionRoutine(Irp,
                           (PVOID)CmBattIoCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Now call ACPI */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Irp->IoStatus.Status;
    }

    /* Free the IRP */
    IoFreeIrp(Irp);

    /* Return status */
    if (!(NT_SUCCESS(Status)) && (CmBattDebug & 0xC))
        DbgPrint("CmBattGetAcpiInterfaces: Could not get ACPI driver interfaces, status = %x\n", Status);
    return Status;
}

VOID
NTAPI
CmBattDestroyFdo(IN PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();
    if (CmBattDebug & 0x220) DbgPrint("CmBattDestroyFdo, Battery.\n");

    /* Delete the device */
    IoDeleteDevice(DeviceObject);
    if (CmBattDebug & 0x220) DbgPrint("CmBattDestroyFdo: done.\n");
}

NTSTATUS
NTAPI
CmBattRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    PVOID Context;
    DeviceExtension = DeviceObject->DeviceExtension;
    if (CmBattDebug & 2)
        DbgPrint("CmBattRemoveDevice: CmBatt (%x), Type %d, _UID %d\n",
                 DeviceExtension,
                 DeviceExtension->FdoType,
                 DeviceExtension->DeviceId);

    /* Make sure it's safe to go ahead */
    IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, 0);

    /* Check for pending power IRP */
    if (DeviceExtension->PowerIrp)
    {
        /* Cancel and clear */
        IoCancelIrp(DeviceExtension->PowerIrp);
        DeviceExtension->PowerIrp = NULL;
    }

    /* Check what type of FDO is being removed */
    Context = DeviceExtension->AcpiInterface.Context;
    if (DeviceExtension->FdoType == CmBattBattery)
    {
        /* Unregister battery FDO */
        DeviceExtension->AcpiInterface.UnregisterForDeviceNotifications(Context,
                                                                        (PVOID)CmBattNotifyHandler);
        CmBattWmiDeRegistration(DeviceExtension);
        if (!NT_SUCCESS(BatteryClassUnload(DeviceExtension->ClassData))) ASSERT(FALSE);
    }
    else
    {
        /* Unregister AC adapter FDO */
        DeviceExtension->AcpiInterface.UnregisterForDeviceNotifications(Context,
                                                                        (PVOID)CmBattNotifyHandler);
        CmBattWmiDeRegistration(DeviceExtension);
        AcAdapterPdo = NULL;
    }

    /* Detach and delete */
    IoDetachDevice(DeviceExtension->AttachedDevice);
    IoDeleteDevice(DeviceExtension->DeviceObject);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattPowerDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    if (CmBattDebug & 0x210) DbgPrint("CmBattPowerDispatch\n");

    /* Get stack location and device extension */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    switch (IoStackLocation->MinorFunction)
    {
        case IRP_MN_WAIT_WAKE:
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerDispatch: IRP_MN_WAIT_WAKE\n");
            break;

        case IRP_MN_POWER_SEQUENCE:
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerDispatch: IRP_MN_POWER_SEQUENCE\n");
            break;

        case IRP_MN_QUERY_POWER:
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerDispatch: IRP_MN_WAIT_WAKE\n");
            break;

        case IRP_MN_SET_POWER:
            if (CmBattDebug & 0x10)
                DbgPrint("CmBattPowerDispatch: IRP_MN_SET_POWER type: %d, State: %d \n",
                         IoStackLocation->Parameters.Power.Type,
                         IoStackLocation->Parameters.Power.State);
            break;

        default:

            if (CmBattDebug & 1)
                DbgPrint("CmBattPowerDispatch: minor %d\n", IoStackLocation->MinorFunction);
            break;
    }

    /* Start the next IRP and see if we're attached */
    PoStartNextPowerIrp(Irp);
    if (DeviceExtension->AttachedDevice)
    {
        /* Call ACPI */
        IoSkipCurrentIrpStackLocation(Irp);
        Status = PoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    else
    {
        /* Complete the request here */
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    KEVENT Event;
    PAGED_CODE();

    /* Get stack location and device extension */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    /* Set default error */
    Status = STATUS_NOT_SUPPORTED;

    /* Try to acquire the lock before doing anything */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* Complete the request */
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    /* What's the operation? */
    switch (IoStackLocation->MinorFunction)
    {
        case IRP_MN_QUERY_PNP_DEVICE_STATE:

            /* Initialize our wait event */
            KeInitializeEvent(&Event, SynchronizationEvent, 0);

            /* Set the completion routine */
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   (PVOID)CmBattIoCompletion,
                                   &Event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            /* Now call ACPI to inherit its PnP Device State */
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion */
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = Irp->IoStatus.Status;
            }

            /* However, a battery CAN be disabled */
            Irp->IoStatus.Information &= ~PNP_DEVICE_NOT_DISABLEABLE;

            /* Release the remove lock and complete the request */
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
            return Status;

        case IRP_MN_SURPRISE_REMOVAL:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_SURPRISE_REMOVAL\n");

            /* Lock the device extension and set the handle count to invalid */
            ExAcquireFastMutex(&DeviceExtension->FastMutex);
            DeviceExtension->HandleCount = -1;
            ExReleaseFastMutex(&DeviceExtension->FastMutex);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_START_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_START_DEVICE\n");

            /* Mark the extension as started */
            if (DeviceExtension->FdoType == CmBattBattery) DeviceExtension->Started = TRUE;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_STOP_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_STOP_DEVICE\n");

            /* Mark the extension as stopped */
            if (DeviceExtension->FdoType == CmBattBattery) DeviceExtension->Started = FALSE;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_QUERY_REMOVE_DEVICE\n");

            /* Lock the extension and get the current handle count */
            ExAcquireFastMutex(&DeviceExtension->FastMutex);
            if (DeviceExtension->HandleCount == 0)
            {
                /* No handles. Mark it as invalid since it'll be removed */
                DeviceExtension->HandleCount = -1;
                Status = STATUS_SUCCESS;
            }
            else if (DeviceExtension->HandleCount == -1)
            {
                /* Don't do anything, but this is strange since it's already removed */
                Status = STATUS_SUCCESS;
                if (CmBattDebug & 4)
                    DbgPrint("CmBattPnpDispatch: Received two consecutive QUERY_REMOVE requests.\n");
            }
            else
            {
                /* Fail because there's still open handles */
                Status = STATUS_UNSUCCESSFUL;
            }

            /* Release the lock and return */
            ExReleaseFastMutex(&DeviceExtension->FastMutex);
            break;

        case IRP_MN_REMOVE_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_REMOVE_DEVICE\n");

            /* Call the remove code */
            Status = CmBattRemoveDevice(DeviceObject, Irp);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE\n");

            /* Lock the extension and get the handle count */
            ExAcquireFastMutex(&DeviceExtension->FastMutex);
            if (DeviceExtension->HandleCount == -1)
            {
                /* A remove was in progress, set the handle count back to 0 */
                DeviceExtension->HandleCount = 0;
            }
            else if (CmBattDebug & 2)
            {
                /* Nop, but warn about it */
                DbgPrint("CmBattPnpDispatch: Received CANCEL_REMOVE when OpenCount == %x\n",
                         DeviceExtension->HandleCount);
            }

            /* Return success in all cases, and release the lock */
            Status = STATUS_SUCCESS;
            ExReleaseFastMutex(&DeviceExtension->FastMutex);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_QUERY_STOP_DEVICE\n");

            /* There's no real support for this */
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_CANCEL_STOP_DEVICE\n");

            /* There's no real support for this */
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_QUERY_CAPABILITIES:

            /* Initialize our wait event */
            KeInitializeEvent(&Event, SynchronizationEvent, 0);

            /* Set the completion routine */
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   (PVOID)CmBattIoCompletion,
                                   &Event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            /* Now call ACPI */
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion */
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = Irp->IoStatus.Status;
            }

            /* Get the wake power state */
            DeviceExtension->PowerState.SystemState = IoStackLocation->Parameters.DeviceCapabilities.Capabilities->SystemWake;
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES %d Capabilities->SystemWake = %x\n",
                         DeviceExtension->FdoType,
                         DeviceExtension->PowerState);

            /* Check if it's invalid */
            if (DeviceExtension->PowerState.SystemState == PowerSystemUnspecified)
            {
                /* Wait wake is not supported in this scenario */
                DeviceExtension->WaitWakeEnable = FALSE;
                if (CmBattDebug & 0x20)
                    DbgPrint("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES Wake not supported.\n");
            }
            else if (!(DeviceExtension->PowerIrp) &&
                     (DeviceExtension->WaitWakeEnable))
            {
                /* If it was requested in the registry, request the power IRP for it */
                PoRequestPowerIrp(DeviceExtension->DeviceObject,
                                  0,
                                  DeviceExtension->PowerState,
                                  CmBattWaitWakeLoop,
                                  0,
                                  &DeviceExtension->PowerIrp);
                if (CmBattDebug & 0x20)
                    DbgPrint("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES wait/Wake irp sent.\n");
            }

            /* Release the remove lock and complete the request */
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
            return Status;

        default:
            /* Unsupported */
            if (CmBattDebug & 0x20)
                DbgPrint("CmBattPnpDispatch: Unimplemented minor %0x\n",
                         IoStackLocation->MinorFunction);
            break;
    }

    /* Release the remove lock */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    /* Set IRP status if we have one */
    if (Status != STATUS_NOT_SUPPORTED) Irp->IoStatus.Status = Status;

    /* Did someone pick it up? */
    if ((NT_SUCCESS(Status)) || (Status == STATUS_NOT_SUPPORTED))
    {
        /* Still unsupported, try ACPI */
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    else
    {
        /* Complete the request */
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Release the remove lock and return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattCreateFdo(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT DeviceObject,
                IN ULONG DeviceExtensionSize,
                IN PDEVICE_OBJECT *NewDeviceObject)
{
    PDEVICE_OBJECT FdoDeviceObject;
    HANDLE KeyHandle;
    PCMBATT_DEVICE_EXTENSION FdoExtension;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)Buffer;
    NTSTATUS Status;
    UNICODE_STRING KeyString;
    ULONG UniqueId;
    ULONG ResultLength;
    PAGED_CODE();
    if (CmBattDebug & 0x220) DbgPrint("CmBattCreateFdo: Entered\n");

    /* Get unique ID */
    Status = CmBattGetUniqueId(DeviceObject, &UniqueId);
    if (!NT_SUCCESS(Status))
    {
        /* Assume 0 */
        UniqueId = 0;
        if (CmBattDebug & 2)
          DbgPrint("CmBattCreateFdo: Error %x from _UID, assuming unit #0\n", Status);
    }

    /* Create the FDO */
    Status = IoCreateDevice(DriverObject,
                            DeviceExtensionSize,
                            0,
                            FILE_DEVICE_BATTERY,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            &FdoDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattCreateFdo: error (0x%x) creating device object\n", Status);
        return Status;
    }

    /* Set FDO flags */
    FdoDeviceObject->Flags |= (DO_POWER_PAGABLE | DO_BUFFERED_IO);
    FdoDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Initialize the extension */
    FdoExtension = FdoDeviceObject->DeviceExtension;
    RtlZeroMemory(FdoExtension, DeviceExtensionSize);
    FdoExtension->DeviceObject = FdoDeviceObject;
    FdoExtension->FdoDeviceObject = FdoDeviceObject;
    FdoExtension->PdoDeviceObject = DeviceObject;

    /* Attach to ACPI */
    FdoExtension->AttachedDevice = IoAttachDeviceToDeviceStack(FdoDeviceObject,
                                                               DeviceObject);
    if (!FdoExtension->AttachedDevice)
    {
        /* Destroy and fail */
        CmBattDestroyFdo(FdoExtension->FdoDeviceObject);
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattCreateFdo: IoAttachDeviceToDeviceStack failed.\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Get ACPI interface for EVAL */
    Status = CmBattGetAcpiInterfaces(FdoExtension->AttachedDevice,
                                     &FdoExtension->AcpiInterface);
    if (!FdoExtension->AttachedDevice)
    {
        /* Detach, destroy, and fail */
        IoDetachDevice(FdoExtension->AttachedDevice);
        CmBattDestroyFdo(FdoExtension->FdoDeviceObject);
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattCreateFdo: Could not get ACPI interfaces: %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    /* Setup the rest of the extension */
    ExInitializeFastMutex(&FdoExtension->FastMutex);
    IoInitializeRemoveLock(&FdoExtension->RemoveLock, 'RbmC', 0, 0);
    FdoExtension->HandleCount = 0;
    FdoExtension->WaitWakeEnable = FALSE;
    FdoExtension->DeviceId = UniqueId;
    FdoExtension->DeviceName = NULL;
    FdoExtension->DelayNotification = FALSE;
    FdoExtension->ArFlag = 0;

    /* Open the device key */
    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        /* Read wait wake value */
        RtlInitUnicodeString(&KeyString, L"WaitWakeEnabled");
        Status = ZwQueryValueKey(KeyHandle,
                                 &KeyString,
                                 KeyValuePartialInformation,
                                 PartialInfo,
                                 sizeof(Buffer),
                                 &ResultLength);
        if (NT_SUCCESS(Status))
        {
            /* Set value */
            FdoExtension->WaitWakeEnable = ((*(PULONG)PartialInfo->Data) != 0);
        }

        /* Close the handle */
        ZwClose(KeyHandle);
    }

    /* Return success and the new FDO */
    *NewDeviceObject = FdoDeviceObject;
    if (CmBattDebug & 0x220)
        DbgPrint("CmBattCreateFdo: Created FDO %x\n", FdoDeviceObject);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattAddBattery(IN PDRIVER_OBJECT DriverObject,
                 IN PDEVICE_OBJECT DeviceObject)
{
    BATTERY_MINIPORT_INFO MiniportInfo;
    NTSTATUS Status;
    PDEVICE_OBJECT FdoDeviceObject;
    PCMBATT_DEVICE_EXTENSION FdoExtension;
    PAGED_CODE();
    if (CmBattDebug & 0x220)
        DbgPrint("CmBattAddBattery: pdo %x\n", DeviceObject);

    /* Create the FDO */
    Status = CmBattCreateFdo(DriverObject,
                             DeviceObject,
                             sizeof(CMBATT_DEVICE_EXTENSION),
                             &FdoDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddBattery: error (0x%x) creating Fdo\n", Status);
        return Status;
    }

    /* Build the FDO extension, check if we support trip points */
    FdoExtension = FdoDeviceObject->DeviceExtension;
    FdoExtension->FdoType = CmBattBattery;
    FdoExtension->Started = 0;
    FdoExtension->NotifySent = TRUE;
    InterlockedExchange(&FdoExtension->ArLockValue, 0);
    FdoExtension->TripPointValue = BATTERY_UNKNOWN_CAPACITY;
    FdoExtension->Tag = 0;
    FdoExtension->InterruptTime = KeQueryInterruptTime();
    FdoExtension->TripPointSet = CmBattSetTripPpoint(FdoExtension, 0) !=
                                 STATUS_OBJECT_NAME_NOT_FOUND;

    /* Setup the battery miniport information structure */
    RtlZeroMemory(&MiniportInfo, sizeof(MiniportInfo));
    MiniportInfo.Pdo = DeviceObject;
    MiniportInfo.MajorVersion = BATTERY_CLASS_MAJOR_VERSION;
    MiniportInfo.MinorVersion = BATTERY_CLASS_MINOR_VERSION;
    MiniportInfo.Context = FdoExtension;
    MiniportInfo.QueryTag = (PVOID)CmBattQueryTag;
    MiniportInfo.QueryInformation = (PVOID)CmBattQueryInformation;
    MiniportInfo.SetInformation = NULL;
    MiniportInfo.QueryStatus = (PVOID)CmBattQueryStatus;
    MiniportInfo.SetStatusNotify = (PVOID)CmBattSetStatusNotify;
    MiniportInfo.DisableStatusNotify = (PVOID)CmBattDisableStatusNotify;
    MiniportInfo.DeviceName = FdoExtension->DeviceName;

    /* Register with the class driver */
    Status = BatteryClassInitializeDevice(&MiniportInfo, &FdoExtension->ClassData);
    if (!NT_SUCCESS(Status))
    {
        IoDetachDevice(FdoExtension->AttachedDevice);
        CmBattDestroyFdo(FdoExtension->FdoDeviceObject);
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddBattery: error (0x%x) registering with class\n", Status);
        return Status;
    }

    /* Register WMI */
    Status = CmBattWmiRegistration(FdoExtension);
    if (!NT_SUCCESS(Status))
    {
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddBattery: Could not register as a WMI provider, status = %Lx\n", Status);
        return Status;
    }

    /* Register ACPI */
    Status = FdoExtension->AcpiInterface.RegisterForDeviceNotifications(FdoExtension->AcpiInterface.Context,
                                                                        (PVOID)CmBattNotifyHandler,
                                                                        FdoExtension);
    if (!NT_SUCCESS(Status))
    {
        CmBattWmiDeRegistration(FdoExtension);
        BatteryClassUnload(FdoExtension->ClassData);
        IoDetachDevice(FdoExtension->AttachedDevice);
        CmBattDestroyFdo(FdoExtension->FdoDeviceObject);
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddBattery: Could not register for battery notify, status = %Lx\n", Status);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmBattAddAcAdapter(IN PDRIVER_OBJECT DriverObject,
                   IN PDEVICE_OBJECT PdoDeviceObject)
{
    PDEVICE_OBJECT FdoDeviceObject;
    NTSTATUS Status;
    PCMBATT_DEVICE_EXTENSION DeviceExtension;
    PAGED_CODE();
    if (CmBattDebug & 0x220)
        DbgPrint("CmBattAddAcAdapter: pdo %x\n", PdoDeviceObject);

    /* Check if we already have an AC adapter */
    if (AcAdapterPdo)
    {
        /* Don't do anything */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBatt: Second AC adapter found.  Current version of driver only supports 1 adapter.\n");
    }
    else
    {
        /* Set this as the AC adapter's PDO */
        AcAdapterPdo = PdoDeviceObject;
    }

    /* Create the FDO for the adapter */
    Status = CmBattCreateFdo(DriverObject,
                             PdoDeviceObject,
                             sizeof(CMBATT_DEVICE_EXTENSION),
                             &FdoDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddAcAdapter: error (0x%x) creating Fdo\n", Status);
        return Status;
    }

    /* Set the type and do WMI registration */
    DeviceExtension = FdoDeviceObject->DeviceExtension;
    DeviceExtension->FdoType = CmBattAcAdapter;
    Status = CmBattWmiRegistration(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        /* We can go on without WMI */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddBattery: Could not register as a WMI provider, status = %Lx\n", Status);
    }

    /* Register with ACPI */
    Status = DeviceExtension->AcpiInterface.RegisterForDeviceNotifications(DeviceExtension->AcpiInterface.Context,
                                                                           (PVOID)CmBattNotifyHandler,
                                                                           DeviceExtension);
    if (!(NT_SUCCESS(Status)) && (CmBattDebug & 0xC))
        DbgPrint("CmBattAddAcAdapter: Could not register for power notify, status = %Lx\n", Status);

    /* Send the first manual notification */
    CmBattNotifyHandler(DeviceExtension, ACPI_BATT_NOTIFY_STATUS);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattAddDevice(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT PdoDeviceObject)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG ResultLength;
    UNICODE_STRING KeyString;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)Buffer;
    ULONG PowerSourceType;
    PAGED_CODE();
    if (CmBattDebug & 0x220)
        DbgPrint("CmBattAddDevice: Entered with pdo %x\n", PdoDeviceObject);

    /* Make sure we have a PDO */
    if (!PdoDeviceObject)
    {
        /* Should not be having as one */
        if (CmBattDebug & 0x24) DbgPrint("CmBattAddDevice: Asked to do detection\n");
        return STATUS_NO_MORE_ENTRIES;
    }

    /* Open the driver key */
    Status = IoOpenDeviceRegistryKey(PdoDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_READ,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddDevice: Could not get the software branch: %x\n", Status);
        return Status;
    }

    /* Read the power source type */
    RtlInitUnicodeString(&KeyString, L"PowerSourceType");
    Status = ZwQueryValueKey(KeyHandle,
                             &KeyString,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(Buffer),
                             &ResultLength);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        /* We need the data, fail without it */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddDevice: Could not read the power type identifier: %x\n", Status);
        return Status;
    }

    /* Check what kind of power source this is */
    PowerSourceType = *(PULONG)PartialInfo->Data;
    if (PowerSourceType == 1)
    {
        /* Create an AC adapter */
        Status = CmBattAddAcAdapter(DriverObject, PdoDeviceObject);
    }
    else if (PowerSourceType == 0)
    {
        /* Create a battery */
        Status = CmBattAddBattery(DriverObject, PdoDeviceObject);
    }
    else
    {
        /* Unknown type, fail */
        if (CmBattDebug & 0xC)
            DbgPrint("CmBattAddDevice: Invalid POWER_SOURCE_TYPE == %d \n", PowerSourceType);
        return STATUS_UNSUCCESSFUL;
    }

    /* Return whatever the FDO creation routine did */
    return Status;
}

/* EOF */
