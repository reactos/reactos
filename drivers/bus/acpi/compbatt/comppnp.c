/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/comppnp.c
 * PURPOSE:         Plug-and-Play IOCTL/IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

/* FUNCTIONS ******************************************************************/
 
NTSTATUS
NTAPI
CompBattPowerDispatch(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    if (CompBattDebug & 1) DbgPrint("CompBatt: PowerDispatch recieved power IRP.\n");
     
    /* Start the next IRP */
    PoStartNextPowerIrp(Irp);
    
    /* Call the next driver in the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->AttachedDevice, Irp);
}

PCOMPBATT_BATTERY_ENTRY
NTAPI
RemoveBatteryFromList(IN PCUNICODE_STRING BatteryName,
                      IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
NTAPI
IsBatteryAlreadyOnList(IN PCUNICODE_STRING BatteryName,
                       IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
CompBattAddNewBattery(IN PCUNICODE_STRING BatteryName,
                      IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattRemoveBattery(IN PCUNICODE_STRING BatteryName,
                      IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteries(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattPnpEventHandler(IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification,
                        IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattAddDevice(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT PdoDeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING SymbolicLinkName;
    BATTERY_MINIPORT_INFO MiniportInfo;
    if (CompBattDebug & 2) DbgPrint("CompBatt: Got an AddDevice - %x\n", PdoDeviceObject);
    
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
    RtlZeroMemory(DeviceExtension, 0x1B0u);
    
    /* Attach to device stack and set DO pointers */
    DeviceExtension->AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
                                                                  PdoDeviceObject);
    DeviceExtension->DeviceObject = DeviceObject;
    if (!DeviceExtension->AttachedDevice)
    {
        /* Fail */
        if (CompBattDebug & 8)
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
CompBattPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING PnpDispatch\n");

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
                if (CompBattDebug & 2)
                    DbgPrint("CompBatt: Successfully registered for PnP notification\n");
                Status = CompBattGetBatteries(DeviceExtension);
            }
            else
            {
                /* We failed */
                if (CompBattDebug & 8)
                    DbgPrint("CompBatt: Couldn't register for PnP notification - %x\n",
                             Status);
            }
        
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
            Status = STATUS_INVALID_DEVICE_REQUEST;
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
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Release the remove lock and return status */
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING PnpDispatch\n");
    return Status;
}

/* EOF */
