/*
 * PROJECT:         ReactOS ACPI-Compliant Control Method Battery
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/drivers/bus/acpi/cmbatt/cmbatt.c
 * PURPOSE:         Main Initialization Code and IRP Handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "cmbatt.h"

/* GLOBALS ********************************************************************/

ULONG CmBattDebug;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CmBattPowerCallBack(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    PVOID Argument1,
                    PVOID Argument2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
CmBattWakeDpc(PKDPC Dpc,
              PCMBATT_DEVICE_EXTENSION FdoExtension,
              PVOID SystemArgument1,
              PVOID SystemArgument2)
{
    UNIMPLEMENTED;   
}

VOID
NTAPI
CmBattNotifyHandler(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                    ULONG NotifyValue)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
CmBattUnload(PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattVerifyStaticInfo(ULONG StaData,
                       ULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;   
}

NTSTATUS
NTAPI
CmBattOpenClose(PDEVICE_OBJECT DeviceObject,
                PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;   
}

NTSTATUS
NTAPI
CmBattIoctl(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattQueryTag(PCMBATT_DEVICE_EXTENSION DeviceExtension,
               PULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;   
}

NTSTATUS
NTAPI
CmBattDisableStatusNotify(PCMBATT_DEVICE_EXTENSION DeviceExtension)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattSetStatusNotify(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                      ULONG BatteryTag,
                      PBATTERY_NOTIFY BatteryNotify)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattGetBatteryStatus(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       ULONG BatteryTag)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CmBattQueryInformation(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                       ULONG BatteryTag,
                       BATTERY_QUERY_INFORMATION_LEVEL Level,
                       OPTIONAL LONG AtRate,
                       PVOID Buffer,
                       ULONG BufferLength,
                       PULONG ReturnedLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED; 
}

NTSTATUS
NTAPI
CmBattQueryStatus(PCMBATT_DEVICE_EXTENSION DeviceExtension,
                  ULONG BatteryTag,
                  PBATTERY_STATUS BatteryStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
