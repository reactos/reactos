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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CompBattPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
