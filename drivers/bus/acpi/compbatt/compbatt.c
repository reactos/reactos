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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattSystemControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
