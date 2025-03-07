/*
 * PROJECT:     ReactOS Composite Battery Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Plug-and-Play IOCTL/IRP Handling
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group <ros.arm@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

#include <wdmguid.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CompBattPowerDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    if (CompBattDebug & COMPBATT_DEBUG_WARN) DbgPrint("CompBatt: PowerDispatch received power IRP.\n");

    /* Start the next IRP */
    PoStartNextPowerIrp(Irp);

    /* Call the next driver in the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->AttachedDevice, Irp);
}

PCOMPBATT_BATTERY_DATA
NTAPI
RemoveBatteryFromList(
    _In_ PCUNICODE_STRING BatteryName,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCOMPBATT_BATTERY_DATA BatteryData;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE)
        DbgPrint("CompBatt: ENTERING RemoveBatteryFromList\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and compare the name */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!RtlCompareUnicodeString(BatteryName, &BatteryData->BatteryName, TRUE))
        {
            /* Flush pending deletes and any lock waiters */
            IoAcquireRemoveLock(&BatteryData->RemoveLock, 0);
            ExReleaseFastMutex(&DeviceExtension->Lock);
            IoReleaseRemoveLockAndWait(&BatteryData->RemoveLock, 0);

            /* Remove the entry from the list */
            ExAcquireFastMutex(&DeviceExtension->Lock);
            RemoveEntryList(&BatteryData->BatteryLink);
            ExReleaseFastMutex(&DeviceExtension->Lock);
            return BatteryData;
        }

        /* Next */
        NextEntry = NextEntry->Flink;
    }

    /* Done */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING RemoveBatteryFromList\n");
    return NULL;
}

BOOLEAN
NTAPI
IsBatteryAlreadyOnList(
    _In_ PCUNICODE_STRING BatteryName,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCOMPBATT_BATTERY_DATA BatteryData;
    BOOLEAN Found = FALSE;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE)
        DbgPrint("CompBatt: ENTERING IsBatteryAlreadyOnList\n");

    /* Loop the battery list */
    ExAcquireFastMutex(&DeviceExtension->Lock);
    ListHead = &DeviceExtension->BatteryList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the battery information and compare the name */
        BatteryData = CONTAINING_RECORD(NextEntry, COMPBATT_BATTERY_DATA, BatteryLink);
        if (!RtlCompareUnicodeString(BatteryName, &BatteryData->BatteryName, TRUE))
        {
            /* Got it */
            Found = TRUE;
            break;
        }

        /* Next */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock and return search status */
    ExReleaseFastMutex(&DeviceExtension->Lock);
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING IsBatteryAlreadyOnList\n");
    return Found;
}

NTSTATUS
NTAPI
CompBattAddNewBattery(
    _In_ PUNICODE_STRING BatteryName,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCOMPBATT_BATTERY_DATA BatteryData;
    PIRP Irp;
    PIO_STACK_LOCATION IoStackLocation;
    PFILE_OBJECT FileObject;
    PAGED_CODE();
    if (CompBattDebug & COMPBATT_DEBUG_WARN)
        DbgPrint("CompBatt: ENTERING AddNewBattery \"%w\" \n", BatteryName->Buffer);

    /* Is this a new battery? */
    if (!IsBatteryAlreadyOnList(BatteryName, DeviceExtension))
    {
        /* Allocate battery data */
        BatteryData = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(COMPBATT_BATTERY_DATA) +
                                            BatteryName->Length,
                                            'CtaB');
        if (BatteryData)
        {
            /* Initialize the data and write the battery name */
            RtlZeroMemory(BatteryData, sizeof(COMPBATT_BATTERY_DATA));
            BatteryData->Tag = BATTERY_TAG_INVALID;
            BatteryData->BatteryName.MaximumLength = BatteryName->Length;
            BatteryData->BatteryName.Buffer = (PWCHAR)(BatteryData + 1);
            RtlCopyUnicodeString(&BatteryData->BatteryName, BatteryName);

            /* Get the device object */
            Status = CompBattGetDeviceObjectPointer(BatteryName,
                                                    FILE_ALL_ACCESS,
                                                    &FileObject,
                                                    &BatteryData->DeviceObject);
            if (NT_SUCCESS(Status))
            {
                /* Reference the DO and drop the FO */
                ObReferenceObject(BatteryData->DeviceObject);
                ObDereferenceObject(FileObject);

                /* Allocate the battery IRP */
                Irp = IoAllocateIrp(BatteryData->DeviceObject->StackSize + 1, 0);
                if (Irp)
                {
                    /* Save it */
                    BatteryData->Irp = Irp;

                    /* Setup the stack location */
                    IoStackLocation = IoGetNextIrpStackLocation(Irp);
                    IoStackLocation->Parameters.Others.Argument1 = DeviceExtension;
                    IoStackLocation->Parameters.Others.Argument2 = BatteryData;

                    /* Set IRP data */
                    IoSetNextIrpStackLocation(Irp);
                    Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;

                    /* Insert this battery in the list */
                    ExAcquireFastMutex(&DeviceExtension->Lock);
                    InsertTailList(&DeviceExtension->BatteryList,
                                   &BatteryData->BatteryLink);
                    ExReleaseFastMutex(&DeviceExtension->Lock);

                    /* Initialize the work item and delete lock */
                    IoInitializeRemoveLock(&BatteryData->RemoveLock, COMPBATT_TAG, 0, 0);
                    ExInitializeWorkItem(&BatteryData->WorkItem,
                                         (PVOID)CompBattMonitorIrpCompleteWorker,
                                         BatteryData);

                    /* Setup the IRP work entry */
                    CompBattMonitorIrpComplete(BatteryData->DeviceObject, Irp, NULL);
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    /* Fail, no memory */
                    if (CompBattDebug & COMPBATT_DEBUG_ERR)
                        DbgPrint("CompBatt: Couldn't allocate new battery Irp\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    ObDereferenceObject(BatteryData->DeviceObject);
                }
            }
            else if (CompBattDebug & COMPBATT_DEBUG_ERR)
            {
                /* Fail */
                DbgPrint("CompBattAddNewBattery: Failed to get device Object. status = %lx\n",
                         Status);
            }

            if (!NT_SUCCESS(Status))
            {
                /* Free the battery data */
                ExFreePool(BatteryData);
            }
        }
        else
        {
            /* Fail, no memory */
            if (CompBattDebug & COMPBATT_DEBUG_ERR)
                DbgPrint("CompBatt: Couldn't allocate new battery node\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* We're done */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING AddNewBattery\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattRemoveBattery(
    _In_ PUNICODE_STRING BatteryName,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PCOMPBATT_BATTERY_DATA BatteryData;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: RemoveBattery\n");

    /* Remove the entry */
    BatteryData = RemoveBatteryFromList(BatteryName, DeviceExtension);
    if (BatteryData)
    {
        /* Dereference and free it */
        ObDereferenceObject(BatteryData->DeviceObject);
        ExFreePool(BatteryData);

        /* Notify class driver */
        DeviceExtension->Flags = 0;
        BatteryClassStatusNotify(DeviceExtension->ClassData);
    }

    /* It's done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattGetBatteries(
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    PWCHAR p;
    NTSTATUS Status;
    PWCHAR LinkList;
    UNICODE_STRING LinkString;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING GetBatteries\n");

    /* Get all battery links */
    Status = IoGetDeviceInterfaces(&GUID_DEVICE_BATTERY, NULL, 0, &LinkList);
    p = LinkList;
    if (NT_SUCCESS(Status))
    {
        /* Loop all strings inside */
        while (TRUE)
        {
            /* Create the string */
            RtlInitUnicodeString(&LinkString, p);
            if (!LinkString.Length) break;

            /* Add this battery and move on */
            Status = CompBattAddNewBattery(&LinkString, DeviceExtension);
            p += (LinkString.Length / sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        }

        /* Parsing complete, clean up buffer */
        ExFreePool(LinkList);
    }
    else if (CompBattDebug & COMPBATT_DEBUG_ERR)
    {
        /* Fail */
        DbgPrint("CompBatt: Couldn't get list of batteries\n");
    }

    /* Done */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING GetBatteries\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattPnpEventHandler(
    _In_ PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification,
    _In_ PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING PnpEventHandler\n");
    if (CompBattDebug & COMPBATT_DEBUG_WARN) DbgPrint("CompBatt: Received device interface change notification\n");

    /* Check what happened */
    if (IsEqualGUIDAligned(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        /* Add the new battery */
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Received notification of battery arrival\n");
        CompBattAddNewBattery(Notification->SymbolicLinkName, DeviceExtension);
    }
    else if (IsEqualGUIDAligned(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        /* Don't do anything */
        if (CompBattDebug & COMPBATT_DEBUG_WARN)
            DbgPrint("CompBatt: Received notification of battery removal\n");
    }
    else
    {
        /* Shouldn't happen */
        if (CompBattDebug & COMPBATT_DEBUG_WARN) DbgPrint("CompBatt: Received unhandled PnP event\n");
    }

    /* Done, return success */
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING PnpEventHandler\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PdoDeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymbolicLinkName;
    BATTERY_MINIPORT_INFO MiniportInfo;
    if (CompBattDebug & COMPBATT_DEBUG_WARN) DbgPrint("CompBatt: Got an AddDevice - %x\n", PdoDeviceObject);

    /* Create the device */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\CompositeBattery");
    Status = IoCreateDevice(DriverObject,
                            sizeof(COMPBATT_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_BATTERY,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    /* Setup symbolic link for Win32 access */
    RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\CompositeBattery");
    IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

    /* Initialize the device extension */
    DeviceExtension = DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(COMPBATT_DEVICE_EXTENSION));

    /* Attach to device stack and set DO pointers */
    DeviceExtension->AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
                                                                  PdoDeviceObject);
    DeviceExtension->DeviceObject = DeviceObject;
    if (!DeviceExtension->AttachedDevice)
    {
        /* Fail */
        if (CompBattDebug & COMPBATT_DEBUG_ERR)
            DbgPrint("CompBattAddDevice: Could not attach to LowerDevice.\n");
        IoDeleteDevice(DeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    /* Set device object flags */
    DeviceObject->Flags |= (DO_POWER_PAGABLE | DO_BUFFERED_IO);
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Setup the device extension */
    ExInitializeFastMutex(&DeviceExtension->Lock);
    InitializeListHead(&DeviceExtension->BatteryList);
    DeviceExtension->Flags = 0;
    DeviceExtension->NextTag = 1;

    /* Setup the miniport data */
    RtlZeroMemory(&MiniportInfo, sizeof(MiniportInfo));
    MiniportInfo.MajorVersion = BATTERY_CLASS_MAJOR_VERSION;
    MiniportInfo.MinorVersion = BATTERY_CLASS_MINOR_VERSION;
    MiniportInfo.Context = DeviceExtension;
    MiniportInfo.DeviceName = &DeviceName;
    MiniportInfo.QueryTag = (BCLASS_QUERY_TAG)CompBattQueryTag;
    MiniportInfo.QueryInformation = (BCLASS_QUERY_INFORMATION)CompBattQueryInformation;
    MiniportInfo.SetInformation = NULL;
    MiniportInfo.QueryStatus = (BCLASS_QUERY_STATUS)CompBattQueryStatus;
    MiniportInfo.SetStatusNotify = (BCLASS_SET_STATUS_NOTIFY)CompBattSetStatusNotify;
    MiniportInfo.DisableStatusNotify = (BCLASS_DISABLE_STATUS_NOTIFY)CompBattDisableStatusNotify;
    MiniportInfo.Pdo = NULL;

    /* Register with the class driver */
    Status = BatteryClassInitializeDevice(&MiniportInfo,
                                          &DeviceExtension->ClassData);
    if (!NT_SUCCESS(Status))
    {
        /* Undo everything */
        IoDetachDevice(DeviceExtension->AttachedDevice);
        IoDeleteDevice(DeviceObject);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattPnpDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: ENTERING PnpDispatch\n");

    /* Set default error */
    Status = STATUS_NOT_SUPPORTED;

    /* Check what kind of PnP function this is */
    switch (IoStackLocation->MinorFunction)
    {
        case IRP_MN_START_DEVICE:

            /* Device is starting, register for new batteries and pick up current ones */
            Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                                    0,
                                                    (PVOID)&GUID_DEVICE_BATTERY,
                                                    DeviceObject->DriverObject,
                                                    (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)CompBattPnpEventHandler,
                                                    DeviceExtension,
                                                    &DeviceExtension->NotificationEntry);
            if (NT_SUCCESS(Status))
            {
                /* Now go get the batteries */
                if (CompBattDebug & COMPBATT_DEBUG_WARN)
                    DbgPrint("CompBatt: Successfully registered for PnP notification\n");
                Status = CompBattGetBatteries(DeviceExtension);
            }
            else
            {
                /* We failed */
                if (CompBattDebug & COMPBATT_DEBUG_ERR)
                    DbgPrint("CompBatt: Couldn't register for PnP notification - %x\n",
                             Status);
            }
             break;
        case IRP_MN_CANCEL_STOP_DEVICE:

            /* Explicitly say ok */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            /* Explicitly say ok */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:

            /* Explicitly say ok */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:

            /* Add this in */
            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            Status = STATUS_SUCCESS;
            break;

        default:

            /* Not supported */
            break;
    }

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
    if (CompBattDebug & COMPBATT_DEBUG_TRACE) DbgPrint("CompBatt: EXITING PnpDispatch\n");
    return Status;
}

/* EOF */
