/*
 * PROJECT:         ReactOS Composite Battery Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/compbatt/compbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "compbatt.h"

/* GLOBALS ********************************************************************/

ULONG CompBattDebug;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CompBattOpenClose(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PAGED_CODE();
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: ENTERING OpenClose\n");
    
    /* Complete the IRP with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IofCompleteRequest(Irp, IO_NO_INCREMENT);
    
    /* Return success */
    if (CompBattDebug & 0x100) DbgPrint("CompBatt: Exiting OpenClose\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CompBattSystemControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING System Control\n");
    
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
        IofCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CompBattMonitorIrpComplete(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp,
                           IN PKEVENT Event)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattMonitorIrpCompleteWorker(IN PCOMPBATT_BATTERY_ENTRY BatteryData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattRecalculateTag(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattIoctl(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PCOMPBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    if (CompBattDebug & 1) DbgPrint("CompBatt: ENTERING Ioctl\n");

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
    if (CompBattDebug & 1) DbgPrint("CompBatt: EXITING Ioctl\n");
    return Status;
}

NTSTATUS
NTAPI
CompBattQueryTag(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                 OUT PULONG Tag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattDisableStatusNotify(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattSetStatusNotify(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                        IN ULONG BatteryTag,
                        IN PBATTERY_NOTIFY BatteryNotify)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteryStatus(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                         IN ULONG Tag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattQueryStatus(IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension,
                    IN ULONG Tag,
                    IN PBATTERY_STATUS BatteryStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteryInformation(OUT PBATTERY_INFORMATION BatteryInformation,
                              IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetBatteryGranularity(OUT PBATTERY_REPORTING_SCALE ReportingScale,
                              IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattGetEstimatedTime(OUT PULONG Time,
                         IN PCOMPBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
    
NTSTATUS
NTAPI
CompBattQueryInformation(IN PCOMPBATT_DEVICE_EXTENSION FdoExtension,
                         IN ULONG Tag,
                         IN BATTERY_QUERY_INFORMATION_LEVEL InfoLevel,
                         IN OPTIONAL LONG AtRate,
                         IN PVOID Buffer,
                         IN ULONG BufferLength,
                         OUT PULONG ReturnedLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    /* Register add device routine */
    DriverObject->DriverExtension->AddDevice = CompBattAddDevice;
    
    /* Register other handlers */
    DriverObject->MajorFunction[0] = CompBattOpenClose;
    DriverObject->MajorFunction[2] = CompBattOpenClose;
    DriverObject->MajorFunction[14] = CompBattIoctl;
    DriverObject->MajorFunction[22] = CompBattPowerDispatch;
    DriverObject->MajorFunction[23] = CompBattSystemControl;
    DriverObject->MajorFunction[27] = CompBattPnpDispatch;
    
    return STATUS_SUCCESS;
}

/* EOF */
